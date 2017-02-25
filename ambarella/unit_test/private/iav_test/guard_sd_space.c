/*
 * guard_sd_space.c
 *
 * History:
 *   Jun 18, 2015 - [longli] created file
 *
 * Copyright (C) 2015 Ambarella, Inc.
 *
 * This file and its contents ("Software") are protected by intellectual
 * property rights including, without limitation, U.S. and/or foreign
 * copyrights. This Software is also the confidential and proprietary
 * information of Ambarella, Inc. and its licensors. You may not use, reproduce,
 * disclose, distribute, modify, or otherwise prepare derivative works of this
 * Software or any portion thereof except pursuant to a signed license agreement
 * or nondisclosure agreement with Ambarella, Inc. or its authorized affiliates.
 * In the absence of such an agreement, you agree to promptly notify and return
 * this Software to Ambarella, Inc.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF NON-INFRINGEMENT,
 * MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL AMBARELLA, INC. OR ITS AFFILIATES BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; COMPUTER FAILURE OR MALFUNCTION; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <dirent.h>
#include <fcntl.h>
#include <errno.h>
#include <getopt.h>
#include <sys/inotify.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/vfs.h>
#include <signal.h>

#define EVENT_BUF_LEN (10 * (sizeof(struct inotify_event) + NAME_MAX + 1))
#define SD_DEFAULT_RESERVED_SPACE (128) // keep SD free space 128MB
#define MAX_STRING_LENGTH (512)
#define DIR_LEN (128)
#define WAIT_INTERVAL 3
#define LOCKMODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
#define GUARDCONFIG "/etc/guard.conf"
#define USB_MOUNT_PATH "/storage/sda1/"
#define SDCARD_MOUNT_PATH "/sdcard/"

char file_path[MAX_STRING_LENGTH] = {0};

typedef struct dir_list{
    char dir_path[DIR_LEN];
    char dir_name[DIR_LEN];
    struct dir_list *next;
}am_dir_list;

am_dir_list *dir_head = NULL;
char scanned_dir[MAX_STRING_LENGTH] = {0};
char config_file[DIR_LEN] = GUARDCONFIG;
char **ignored_item = NULL;
int reserved_space = SD_DEFAULT_RESERVED_SPACE;
int has_para = 0;
// options and usage
#define NO_ARG    0
#define HAS_ARG   1
static const char *short_options = "m:i:r:n:c:h";
static struct option long_options[] = {
  {"monitor", HAS_ARG, 0, 'm'},
  {"ignore", HAS_ARG, 0, 'i'},
  {"reserved_space", HAS_ARG, 0, 'r'},
  {"config", HAS_ARG, 0, 'c'},
  {"help", NO_ARG, 0, 'h'},
  {0, 0, 0, 0}
};

struct hint_s {
    const char *arg;
    const char *str;
};

static const struct hint_s hint[] = {
  {"string", "\tthe directory to be monitored"},
  {"string", "\tFile(s) that will not be removed, without path"},
  {"num", "reserved space (MB), default 128MB"},
  {"string", "\tthe path of the configuration file, "
      "default \"/etc/guard.conf\""},
  {"", "\t\tshow help"},
};

static void usage()
{
  uint32_t i = 0;

  printf("Usage: guard_sd_space [options]\n");
  printf("options:\n");
  for (i = 0; i < sizeof(long_options) / sizeof(long_options[0]) - 1; i++) {
    if (isalpha(long_options[i].val))
      printf("-%c ", long_options[i].val);
    else
      printf("   ");
    printf("--%s", long_options[i].name);
    if (hint[i].arg[0] != 0)
      printf(" [%s]", hint[i].arg);
    printf("\t%s\n", hint[i].str);
  }

  printf("e.g. guard_sd_space -m /storage/sda1 -i event,test.txt "
      "-c /etc/guard.conf -r 128\n");
}

static void release_ignored_item()
{
  char **ptr = ignored_item;
  if (ptr) {
    while (*ptr) {
      free(*ptr);
      *ptr = NULL;
      ++ptr;
    }
    free(ignored_item);
    ignored_item = NULL;
  }
}

static int add_ignore_iterm(const char *string_list)
{
  int ret = 0;
  int i = 0, j = 0, k = 0;;
  int count = 0, len = 0;

  do {
    release_ignored_item();
    while (1) {
      if (string_list[i] == ',' || string_list[i] == '\0') {
        while (isspace(string_list[j])) {
          ++j;
        }
        if (j != i) {
          ++count;
          j = i;
        }

        if (string_list[i] == '\0') {
          break;
        }

        ++j;
      }
      ++i;
    }

    i = 0;
    j = 0;
    ignored_item = (char **)malloc(sizeof(char *) * (count + 1));
    if (!ignored_item) {
      printf("add_ignore_iterm[%d]: malloc error\n", __LINE__);
      ret = -1;
      break;
    }
    count = 0;
    while (1) {
      if (string_list[i] == ',' || string_list[i] == '\0') {
        while (isspace(string_list[j])) {
          ++j;
        }
        if (j != i) {
          len = i - j;
          k = i - 1;
          while (k > j) {
            if (!isspace(string_list[k])) {
              break;
            }
            --k;
          }
          len = k - j + 1;
          ignored_item[count] = (char *)malloc(len + 1);
          if (!ignored_item[count]) {
            printf("add_ignore_iterm[%d]: malloc error\n", __LINE__);
            while(count > 0) {
              free(ignored_item[count - 1]);
              ignored_item[count - 1] = NULL;
              --count;
            }
            free(ignored_item);
            ignored_item = NULL;
            ret = -1;
            break;
          }

          strncpy(ignored_item[count], &string_list[j], len);
          ignored_item[count][len] = '\0';
          ++count;
          j = i;
        }
        if (string_list[i] == '\0') break;
        ++j;
      }
      ++i;
    }
    if (ignored_item) {
      ignored_item[count] = NULL;
    }
  } while (0);

  return ret;
}

static void print_ignore_list()
{
  if (ignored_item) {
    char **ptr = ignored_item;
    if (*ptr) {
      printf("Ignore iterm list:    %s", *ptr);
      ++ptr;
    }
    while (*ptr) {
      printf(", %s", *ptr);
      *ptr = NULL;
      ++ptr;
    }
    printf("\n");
  }
}

static int init_param(int argc, char **argv)
{
  int ret = 0;
  int show_help = 0;
  int32_t ch;
  int32_t opt_index;

  while ((ch = getopt_long(argc,
                           argv,
                           short_options,
                           long_options,
                           &opt_index)) != -1) {
    has_para = 1;
    switch (ch) {
      case 'm':
        if (optarg) {
          strncpy(scanned_dir, optarg, sizeof(scanned_dir) - 1);
        } else {
          show_help = 1;
        }
        break;
      case 'i':
        if (optarg) {
          if (add_ignore_iterm(optarg)) {
            ret = 1;
            break;
          }
        } else {
          show_help = 1;
        }
        break;
      case 'r':
        if (optarg && isdigit(optarg[0])) {
          reserved_space = (int32_t)atoi(optarg);
        } else {
          show_help = 1;
        }
        break;
      case 'c':
        if (optarg) {
          strncpy(config_file, optarg, sizeof(config_file) - 1);
        } else {
          show_help = 1;
        }
        break;
      default:
        show_help = 1;
        break;
    }
  }

  if (show_help) {
    usage();
    ret = 1;
  }

  return ret;
}

static int add_to_dir_list(char *dir_name)
{
  int ret = 0;
  int dir_len, path_len;
  am_dir_list *dir_node = NULL;

  do {
    if (!dir_name) ret = -1;
    dir_node = (am_dir_list *)malloc(sizeof(am_dir_list));
    if (!dir_node) {
      ret = -1;
      printf("line(%d): malloc error\n", __LINE__);
      break;
    }

    dir_len = sizeof(dir_node->dir_path);
    path_len = sizeof(dir_node->dir_path);
    if ((strlen(file_path) > (path_len - 1)) ||
        (strlen(dir_name) > (dir_len - 1))) {
      ret = -1;
      free(dir_node);
      printf("file_path or dir_name length is too long\n");
      break;
    }
    snprintf(dir_node->dir_name, sizeof(dir_node->dir_name)-1,"%s", dir_name);
    snprintf(dir_node->dir_path, sizeof(dir_node->dir_path)-1,"%s", file_path);
    dir_node->dir_name[DIR_LEN - 1] = '\0';
    dir_node->dir_path[DIR_LEN - 1] = '\0';
    printf("skip dir %s/%s\n", dir_node->dir_path,dir_node->dir_name);

    if (dir_head) {
      dir_node->next = dir_head->next;
      dir_head->next = dir_node;
    } else {
      dir_head = dir_node;
      dir_head->next = NULL;
    }

  } while (0);

  return ret;
}

static void free_dir_list()
{
  am_dir_list *dir_node = NULL;

  while (dir_head) {
    dir_node = dir_head;
    dir_head = dir_head->next;
    free(dir_node);
    dir_node = NULL;
  }

  dir_head = NULL;
}

static int save_guard_config(const char *scan_dir)
{
  struct stat statbuf;
  FILE *fp = NULL;
  int ret = 0;

  do {
    if (stat(scan_dir, &statbuf) == 0) {
      fp = fopen(config_file, "wb");
      if (!fp) {
        printf("fopen %s file fail\n", config_file);
        ret = -1;
        break;
      }
      fprintf(fp, "monitor:%s", scan_dir);

      if (ignored_item) {
        if (ignored_item[0]) {
          int i = 1;

          fprintf(fp, "\nignore:%s", ignored_item[0]);
          while (ignored_item[i]) {
            fprintf(fp, ",%s", ignored_item[i]);
            ++i;
          }
        }
      }

      fprintf(fp, "\nreserve:%d", reserved_space);

      fclose(fp);
      fp = NULL;
    } else {
      // can not detect SD card
      if (strcmp(scan_dir, USB_MOUNT_PATH)) {
        printf("***Please insert SD card or remount SD card to : %s \n",
               scan_dir);
      } else {
        printf("***Please insert SD card or remount SD card to "
            "default directory: %s \n", scan_dir);
      }
      ret = -1;
      break;
    }
  } while (0);

  return ret;
}

static int guard_read_config()
{
  char buf[MAX_STRING_LENGTH] = {0};
  char *buf_ptr = NULL;
  struct stat statbuf;
  FILE *fp = NULL;
  int ret = 0;

  do {
    if (scanned_dir != NULL ) {
      memset(scanned_dir, 0, sizeof(scanned_dir));
    } else {
      printf("guard_read_config: scanned_dir is NULL. \n");
      ret = -1;
      break;
    }

    if (access(config_file, R_OK) == 0) {
      fp = fopen(config_file, "rt");
      if (!fp) {
        printf("fopen %s file fail\n", config_file);
        ret = -1;
        break;
      }

      while (fgets(buf, sizeof(buf) - 1, fp) != NULL) {
        int i = strlen(buf);
        for (i = i - 1; i >= 0; --i) {
          if (isspace(buf[i])) {
            buf[i] = '\0';
          } else {
            break;
          }
        }
        buf_ptr = buf;
        while (isspace(*buf_ptr)) {
          ++buf_ptr;
        }

        if (!strncasecmp(buf_ptr, "monitor:", 8)) {
          buf_ptr += 8;
          while (isspace(*buf_ptr)) {
            ++buf_ptr;
          }
          strncpy(scanned_dir, buf_ptr, sizeof(scanned_dir) - 1);
        } else if (!strncasecmp(buf_ptr, "reserve:", 8)) {
          buf_ptr += 8;
          while (isspace(*buf_ptr)) {
            ++buf_ptr;
          }
          reserved_space = atoi(buf_ptr);

        } else if (!strncasecmp(buf_ptr, "ignore:", 7)) {
          buf_ptr += 7;
          while (isspace(*buf_ptr)) {
            ++buf_ptr;
          }

          add_ignore_iterm(buf_ptr);
        }
      }

      fclose(fp);
      fp = NULL;

      if (!scanned_dir[0] || stat(scanned_dir, &statbuf)) {
        ret = -1;
        printf("Monitored dir string is empty or does not exist\n");
        break;
      }
    } else {
      ret = -2;
    }

  } while (0);

  return ret;
}

static int init_inotify(int *inotify_fd, const char *file_path)
{
  int ret = 0;
  struct statfs fs;
  int wd = -1;

  do {
    if (!file_path) {
      ret = -1;
      printf("init_inotify: file_path is null string\n");
      break;
    }

    *inotify_fd = inotify_init();
    if (*inotify_fd == -1) {
      ret = -1;
      printf("inotify init error, exit!");
      break;
    }

    int flags = fcntl(*inotify_fd, F_GETFL, 0);
    if (fcntl(*inotify_fd, F_SETFL, flags | O_NONBLOCK)) {
      ret = -1;
      printf("set inotify fd nonblock error\n");
      break;
    }

    if (statfs(file_path, &fs) < 0) {
      ret = -1;
      printf("statfs %s error, errno=%d\n", file_path, errno);
      break;
    }

    wd = inotify_add_watch(*inotify_fd, file_path, IN_ALL_EVENTS);
    if (wd == -1) {
      printf("inotify_add_watch error, errno = %d\n", errno);
      ret = -1;
      break;
    }
  } while (0);

  return ret;
}

static int is_config_changed(int inotify_fd, const char *file_path)
{
  char *p;
  int ret = 0;
  ssize_t num_read = 0;
  struct inotify_event *event;
  char event_buf[EVENT_BUF_LEN];

  do {
    num_read = read(inotify_fd, event_buf, sizeof(event_buf));
    if (num_read == 0) {
      printf("read() from inotify fd returned 0");
    }
    if (num_read == -1 && errno != EAGAIN) {
      perror("read");
      ret = -1;
      break;
    }
    if (num_read > 0) {
      for (p = event_buf; p < event_buf + num_read; ) {
        event =  (struct inotify_event *)p;
        if (event->mask & IN_MODIFY) {
          ret = 1;
          break;
        } else if (event->mask & IN_IGNORED) {
          int wd = -1;
          ret = 1;
          if (!file_path) {
            printf("is_config_changed: File_path is null string\n");
            ret = -1;
            break;
          }
          wd = inotify_add_watch(inotify_fd, file_path, IN_ALL_EVENTS);
          if (wd == -1) {
            printf("Inotify_add_watch error, errno = %d\n", errno);
            ret = -1;
            break;
          }
        }
        p += sizeof(struct inotify_event) + event->len;
      }
    }
  } while (0);

  return ret;
}

static int scandir_filter(const struct dirent *p_dir)
{
  struct stat st;
  char **ptr = ignored_item;

  if (ptr) {
    while(*ptr) {
      if (!strcmp(p_dir->d_name, *ptr))
        return 0;
      ++ptr;
    }
  }

  stat(p_dir->d_name, &st);
  if (S_ISDIR(st.st_mode)) {
    if (strcmp(p_dir->d_name, ".") && strcmp(p_dir->d_name, "..")) {
      /* whether directory is empty, if empty, remove it */
      int num = 0;
      DIR *dirp;

      if (dir_head) {
        am_dir_list *dir_node = dir_head;
        while (dir_node) {
          if (!strcmp(p_dir->d_name, dir_node->dir_name) &&
              !strcmp(file_path, dir_node->dir_path)) {
            /* skip this dir */
            return 0;
          }
          dir_node = dir_node->next;
        }
      }

      dirp = opendir(p_dir->d_name);
      if (!dirp) return 0;
      while (readdir(dirp) != NULL) {
        ++num;
        if (num > 2) break;
      }
      closedir(dirp);
      if (num > 2) {
        return 1;
      } else {
        printf("Delete empty dir %s\n", p_dir->d_name);
        remove(p_dir->d_name);
        return 0;
      }
    } else {
      //skip directory . and ..
      return 0;
    }
  }

  return 1;
}

static int time_increase_order(const struct dirent **p1, const struct dirent **p2)
{
  struct stat st1, st2;
  stat((*p1)->d_name, &st1);
  stat((*p2)->d_name, &st2);
  return (st1.st_mtime - st2.st_mtime);
}

static int check_and_get_oddest_files_list(const char *dir_path,
                                           char *file_path,
                                           struct dirent ***name_list,
                                           int *namelist_num)
{
  int ret = 0;
  int files_num = 0;
  int list_num = 0;
  int dir_deep = 0;
  char current_dir[256] = {0};
  char scan_dir_name[256] = {0};
  struct dirent **namelist = *name_list;

  if (!dir_path || !file_path || !getcwd(current_dir, 256))
    return -1;

  if (chdir(dir_path)) {
    printf("%s does not exist\n", dir_path);
    return -1;
  }

  if (strncmp(dir_path, "./", 2)) {
    strcpy(file_path, dir_path);
  } else {
    snprintf(file_path, MAX_STRING_LENGTH - 1,"%s%s",
             current_dir, dir_path + 1);
  }

  *namelist_num = 0;

  while (files_num < 1) {
    if ((list_num = scandir(".",
                            name_list,
                            scandir_filter,
                            time_increase_order)) > 0) {
      struct stat st;
      namelist = *name_list;

      for (files_num = 0; files_num < list_num; ++files_num) {
        stat(namelist[files_num]->d_name, &st);
        if (S_ISDIR(st.st_mode)) break;
      }

      if (files_num > 0) {
        /* find the oddest file(s) list in this directory */
        ret = 0;
        *namelist_num = files_num;
        /* free the rest of the malloc space */
        for (; files_num < list_num; ++files_num) {
          free(namelist[files_num]);
          namelist[files_num] = NULL;
        }
      } else {
        //the oddest file is non-empty dir, scan it
        int str_len = strlen(file_path);
        int left_len = MAX_STRING_LENGTH - 1- str_len;
        chdir(namelist[files_num]->d_name);
        strncpy(scan_dir_name,
                namelist[files_num]->d_name,
                sizeof(scan_dir_name) - 1);
        ++dir_deep;
        str_len = snprintf(&file_path[str_len],
                           left_len,
                           "/%s",
                           namelist[files_num]->d_name);
        /* free all malloc space */
        while (--list_num) {
          free(namelist[list_num]);
          namelist[list_num] = NULL;
        }
        free(namelist);
        namelist = NULL;

        if (str_len == left_len) {
          printf("Directory string %s... is too long, not support currently\n",
                 file_path);
          ret = -1;
          break;
        }
      }
    } else {
      if (list_num == 0) {
        if (dir_deep > 0) {
          char *s_ptr = strrchr(file_path, '/');
          if (s_ptr) {
            *s_ptr = '\0';
            if (*(s_ptr + 1) != '\0') {
              add_to_dir_list(s_ptr + 1);
            }
            chdir("..");
            --dir_deep;
          } else {
            printf("Fail to go back to upper dir\n");
            ret = -1;
            break;
          }
        } else {
          printf("Directory is empty or there is no file to be deleted\n");
          break;
        }
      } else {
        printf("Scan directory '%s' error\n", file_path);
        ret = -1;
        break;
      }
    }
  }

  free_dir_list();
  chdir(current_dir);
  return ret;
}

static void print_guard_config()
{
  printf("Monitoring directory: %s\n", scanned_dir);
  print_ignore_list();
  printf("Reserved space:       %d MB\n", reserved_space);
}

static int free_space(const char *dir_path)
{
  struct statfs s;
  int free_bsz;

  if (!dir_path)
    return -1;

  statfs(dir_path, &s);
  free_bsz = (s.f_bsize >> 8) * s.f_bavail;
  return (free_bsz >> 12); // MiB
}

static void release_all_name_list(int *used_num,
                                  int *namelist_num,
                                  struct dirent ***namelist)
{
  if ((*namelist_num > 0) && (namelist) && (*namelist)) {
    for (;*used_num < *namelist_num; ++(*used_num)) {
      free((*namelist)[*used_num]);
      (*namelist)[*used_num] = NULL;
    }
    free(*namelist);
    *namelist = NULL;
    free(*namelist);
    *namelist = NULL;
  }
  *used_num = 0;
  *namelist_num = 0;
}

void sig_process(int signo)
{
  printf("got SIG %d, exiting...\n", signo);
  exit(0);
}

int main(int argc, char**argv)
{

  struct sigaction sa;
  sigset_t sig_mask;
  char old_scanned_dir[MAX_STRING_LENGTH] = {0};
  char remove_file_path[MAX_STRING_LENGTH];
  struct dirent **namelist = NULL;
  int namelist_num = 0;
  int used_num = 0;
  int inotify_fd;
  int ret = 0;
  int running_flag = 1;
  int i = 0;

  sigfillset(&sig_mask);
  sigdelset(&sig_mask, SIGINT);
  sigdelset(&sig_mask, SIGTERM);
  sigprocmask(SIG_BLOCK, &sig_mask, NULL);
  sa.sa_flags = SA_RESTART;
  sa.sa_handler = sig_process;
  sigemptyset(&sa.sa_mask);
  sigaction(SIGINT, &sa, NULL);
  sigaction(SIGTERM, &sa, NULL);

  do {
    ret = guard_read_config();
    if(init_param(argc, argv)) {
      break;
    }

    if (has_para) {
      /* specify monitor dir when run this program */
      while(save_guard_config(scanned_dir)) {
        sleep(WAIT_INTERVAL);
      }
    } else {
      if (ret) {
        while (1) {
          if (guard_read_config() < -1) {
            /* try to set default dir for guard config */
            ret = save_guard_config(USB_MOUNT_PATH);
            if (!ret) {
              strncpy(scanned_dir, USB_MOUNT_PATH, sizeof(scanned_dir) - 1);
              scanned_dir[sizeof(scanned_dir) - 1] = '\0';
              break;
            }
          }

          sleep(WAIT_INTERVAL);
        }
      }
    }

    /* remove the ending slash '/' if has */
    i = strlen(scanned_dir);
    for (i = i - 1; i > 0; --i) {
      if (scanned_dir[i] == '/') {
        scanned_dir[i] = '\0';
      } else {
        break;
      }
    }
    strcpy(old_scanned_dir, scanned_dir);

    ret = init_inotify(&inotify_fd, config_file);
    if (ret) {
      break;
    }

    print_guard_config();
    while (running_flag) {
      ret = is_config_changed(inotify_fd, config_file);
      if (ret) {
        if (ret == -1) {
          break;
        }

        printf("Configure file has been changed, begin to reload config ...\n");
        while (1) {
          if (guard_read_config() < -1) {
            /* try to set default dir for guard config */
            ret = save_guard_config(USB_MOUNT_PATH);
            if (!ret) {
              strncpy(scanned_dir, USB_MOUNT_PATH, sizeof(scanned_dir) - 1);
              scanned_dir[sizeof(scanned_dir) - 1] = '\0';
              break;
            }
          }

          sleep(WAIT_INTERVAL);
        }

        print_guard_config();
        if (strcmp(scanned_dir, old_scanned_dir)) {
          strcpy(old_scanned_dir, scanned_dir);
          /* free all the rest of the malloc space */
          release_all_name_list(&used_num, &namelist_num, &namelist);
        }
      }

      if (access(scanned_dir, F_OK)) {
        printf("Directory %s disappears!\n", scanned_dir);
        release_all_name_list(&used_num, &namelist_num, &namelist);
        sleep(WAIT_INTERVAL);
        continue;
      }

      while (free_space(scanned_dir) < reserved_space) {
        if (used_num >= namelist_num) {
          if (namelist_num > 0) {
            free(namelist);
            namelist = NULL;
          }

          if (check_and_get_oddest_files_list(scanned_dir,
                                              file_path,
                                              &namelist,
                                              &namelist_num)) {
            printf("%d: got error return\n", __LINE__);
            running_flag = 0;
            break;
          }

          used_num = 0;
        }

        if (namelist_num > 0) {
          if (snprintf(remove_file_path,
                       MAX_STRING_LENGTH - 1,
                       "%s/%s",
                       file_path,
                       namelist[used_num]->d_name) == (MAX_STRING_LENGTH - 1)) {
            printf("guard_sd_space: File_path string %s... is too long, "
                "not support currently, exit!\n", remove_file_path);
            running_flag = 0;
            release_all_name_list(&used_num, &namelist_num, &namelist);
            break;
          }

          remove(remove_file_path);
          printf("Free space is smaller than %d MB, delete file %s\n",
                 reserved_space, remove_file_path);
          free(namelist[used_num]);
          namelist[used_num] = NULL;
          ++used_num;
        }
      }

      sleep(WAIT_INTERVAL);
    }
  } while (0);
  release_ignored_item();

  return ret;
}
