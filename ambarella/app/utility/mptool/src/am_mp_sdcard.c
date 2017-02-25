/*******************************************************************************
 * am_mp_sdcard.c
 *
 * History:
 *   Jun 17, 2015 - [longli] created file
 *
 *
 * Copyright (c) 2016 Ambarella, Inc.
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
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/vfs.h>
#include <sys/mount.h>
#include <errno.h>
#include "am_mp_server.h"
#include "am_mp_sdcard.h"

#define WRITE_SDCARD_SIZE (64)
#define WRITE_SDCARD_COUNT (1024 * 1600)
#define MOUNT_FS_TYPE "vfat"
#define SDCARD_PATH "/sdcard/"
#define USB_PATH "/storage/sda1"
#define SD_DEVICE_PATH "/dev/mmcblk0p1"
#define USB_DEVICE_PATH "/dev/sda1"

enum {
  SD_TEST_BEGIN         = 0,
  SD_TEST_RESULT_SAVE   = 1
};

struct mp_sd_test_msg {
    int32_t read_speed;
    int32_t write_speed;
};

static int32_t check_mount(const char *device_path,
                           char *dir,
                           const int32_t dir_length)
{
  FILE *fp = NULL;
  char cmd[BUF_LEN];
  char buf[BUF_LEN];
  int32_t ret = 0;

  do {
    if (!device_path || !dir || dir_length < 2) {
      ret = -1;
      break;
    }

    if (access(device_path, F_OK)) {
      ret = -1;
      break;
    }

    snprintf(cmd, sizeof (cmd),
             "cat /proc/mounts | grep '%s' | awk -F ' ' '{print $2}'",
             device_path);

    if ((fp = popen(cmd, "r")) == NULL) {
      printf ("Failed to create a pipe with sh.\n");
      return -1;
    }

    if (fgets(buf, BUF_LEN, fp) != NULL) {
      if (!strcmp(dir, buf)) {
        strncpy(dir, buf, dir_length - 1);
        dir[dir_length - 1] = '\0';
      }
    } else {
      if (access(dir, F_OK)) {
        int res;
        res = mkdir(dir, 0777) == 0 ? 0 : errno;
        if (res && res != EEXIST) {
          printf("mkdir %s error", dir);
          ret = -1;
          break;
        }
      }

      if (mount(device_path, dir, MOUNT_FS_TYPE, 0, 0)) {
        printf("Failed mount %s to %s type: %s\n",
               device_path, dir, MOUNT_FS_TYPE);
        perror("Mount failed.");
        ret = -1;
        break;
      }
    }
  } while (0);

  if (fp) {
    pclose (fp);
  }

  return ret;
}

static long time_diff(struct timeval *start_time, struct timeval *end_time)
{
  long msec;
  msec = (end_time->tv_sec - start_time->tv_sec) * 1000;
  msec += (end_time->tv_usec - start_time->tv_usec) / 1000;
  return msec;
}

static int32_t sdcard_test(const char *mount_dir,
                           const int32_t size, const int32_t count,
                           int32_t *r_speed, int32_t *w_speed)
{
  int32_t i = 0;
  int32_t fd = -1;
  int32_t ret = 0;
  char *buf = NULL;
  char test_file[BUF_LEN];
  long long byte_count = size * count;
  long diff;
  struct timeval begin, end;

  do {
    if (!mount_dir) {
      printf("mount dir is NULL\n");
      ret = -1;
      break;
    }

    buf = (char*)malloc(size);
    if (!buf) {
      printf("malloc error\n");
      ret = -1;
      break;
    }

    snprintf(test_file, sizeof(test_file) - 1,"%stest_file", mount_dir);
    int32_t flags = O_CREAT | O_TRUNC | O_RDWR;
    if ((fd = open(test_file, flags, 0666)) < 0) {
      perror("open");
      ret = -1;
      break;
    }

    gettimeofday(&begin, NULL);
    for (i = 0; i < count; ++i) {
      if (write(fd, buf, size) != size) {
        perror("write");
        ret = -1;
        break;
      }
    }
    sync();
    gettimeofday(&end, NULL);
    diff = time_diff(&begin, &end);
    *w_speed = (byte_count * 1000) / diff;
    printf("SD card write speed: %dBytes/s\n", *w_speed);

    if (lseek(fd, 0, SEEK_SET) < 0) {
      perror("lseek");
      ret = -1;
      break;
    }

    gettimeofday(&begin, NULL);
    for (i = 0; i < count; ++i) {
      if (read(fd, buf, size) != size) {
        perror("read");
        ret = -1;
        break;
      }
    }
    gettimeofday(&end, NULL);
    diff = time_diff(&begin, &end);
    *r_speed = (byte_count * 1000) / diff;
    printf("SD card read speed: %dBytes/s\n", *r_speed);
  } while (0);

  if (buf) {
    free(buf);
    buf = NULL;
  }

  if (fd > -1) {
    close(fd);
    fd = -1;
    unlink(test_file);
  }

  return ret;
}

am_mp_err_t mptool_sd_test_handler(am_mp_msg_t *from_msg, am_mp_msg_t *to_msg)
{
  struct mp_sd_test_msg sd_test_msg;
  struct statfs fs;
  char mount_dir[BUF_LEN] = SDCARD_PATH;

  to_msg->result.ret = MP_OK;
  switch (from_msg->stage) {
    case SD_TEST_BEGIN:
      if (check_mount(SD_DEVICE_PATH, mount_dir, sizeof(mount_dir))) {
        strncpy(mount_dir, USB_PATH, sizeof(mount_dir) - 1);
        if (check_mount(USB_DEVICE_PATH, mount_dir, sizeof(mount_dir))) {
          to_msg->result.ret = MP_ERROR;
          sprintf(to_msg->msg, "Failed to test storage device");
          break;
        }
      }

      if (statfs(mount_dir, &fs) < 0) {
        perror("statfs");
        to_msg->result.ret = MP_ERROR;
        strcpy(to_msg->msg, strerror(errno));
        break;
      }

      if (fs.f_bfree * fs.f_bsize <=
          WRITE_SDCARD_SIZE * WRITE_SDCARD_COUNT) {
        printf("SD card free space is not enough\n");
        to_msg->result.ret = MP_ERROR;
        sprintf(to_msg->msg, "SD card free space is not enough");
        break;
      }

      if (sdcard_test(mount_dir,
                      WRITE_SDCARD_SIZE,
                      WRITE_SDCARD_COUNT,
                      &sd_test_msg.read_speed,
                      &sd_test_msg.write_speed)) {
        printf("SD card test failed\n");
        to_msg->result.ret = MP_ERROR;
        sprintf(to_msg->msg, "SD card test failed");
        break;
      }

      memcpy(to_msg->msg, &sd_test_msg, sizeof(sd_test_msg));
      break;
    case SD_TEST_RESULT_SAVE:
      if (mptool_save_data(from_msg->result.type,
                           from_msg->result.ret, SYNC_FILE) != MP_OK) {
        to_msg->result.ret = MP_ERROR;
      }
      break;
    default:
      to_msg->result.ret = MP_NOT_SUPPORT;
      break;
  }

  return MP_OK;
}
