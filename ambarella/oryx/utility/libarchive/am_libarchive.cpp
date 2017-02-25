/*******************************************************************************
 * am_archive.cpp
 *
 * History:
 *   Apr 15, 2016 - [longli] created file
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
 ******************************************************************************/

#include "am_base_include.h"
#include "am_log.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include "am_libarchive.h"
#include "am_dec7z_if.h"

#define DIR_LEN 256
#define BUF_SIZE 16384

using namespace std;

char buff[BUF_SIZE] = {0};

AMIArchive* AMIArchive::create()
{
  return (AMIArchive *)AMArchive::create();
}

AMArchive* AMArchive::create()
{
  AMArchive *ins = new AMArchive();
  if (!ins) {
    ERROR("Failed to create AMArchive instance.");
  }

  return ins;
}

bool AMArchive::add_files_into_archive(struct archive *a,
                                       const string &sub_src)
{
  bool ret = true;
  bool is_disk_open = false;
  ssize_t len = 0;
  int32_t result = 0;
  int32_t fd = -1;
  struct archive *disk = nullptr;

  do {
    struct archive_entry *entry = nullptr;
    if (nullptr == (disk = archive_read_disk_new())) {
      ret = false;
      ERROR("Failed to create disk archive!");
      break;
    }

    result = archive_read_disk_open(disk, sub_src.c_str());
    if (result != ARCHIVE_OK) {
      ERROR(archive_error_string(disk));
      ret = false;
      break;
    }
    is_disk_open = true;

    for (;;) {
      if (nullptr == (entry = archive_entry_new())) {
        ERROR("Failed to create archive entry!");
        ret = false;
        break;
      }

      result = archive_read_next_header2(disk, entry);
      if (result == ARCHIVE_EOF) {
        break;
      }
      if (result != ARCHIVE_OK) {
        ERROR(archive_error_string(disk));
        ret = false;
        break;
      }
      if (ARCHIVE_OK != archive_read_disk_descend(disk)) {
        ERROR(archive_error_string(disk));
        ret = false;
        break;
      }

      INFO(archive_entry_pathname(entry));
      result = archive_write_header(a, entry);
      if (result < ARCHIVE_OK) {
        WARN(archive_error_string(a));
      }
      if (result == ARCHIVE_FATAL) {
        ret = false;
        break;
      }
      if (result > ARCHIVE_FAILED) {
        /* use a simpler loop to copy data
         * into the target archive. */
        if (0 > (fd = open(archive_entry_sourcepath(entry), O_RDONLY))) {
          ERROR("failed to open archive entry source\n");
          ret = false;
          break;
        }

        len = read(fd, buff, sizeof(buff));
        while (len > 0) {
          if (0 > archive_write_data(a, buff, len)) {
            ERROR("Failed to write archive data.");
            ret = false;
            break;
          }
          len = read(fd, buff, sizeof(buff));
        }
        close(fd);
        if (!ret) {
          break;
        }
      }
      archive_entry_free(entry);
      entry = nullptr;
    }
    if (entry) {
      archive_entry_free(entry);
    }
  } while (0);

  if (disk) {
    if (is_disk_open) {
      archive_read_close(disk);
    }
    archive_read_free(disk);
  }

  return ret;
}

void AMArchive::remove_duplicated_slash(string &str)
{
  bool last_is_slash = false;
  string::iterator it;
  for (it = str.begin(); it != str.end(); ++it) {
    if (*it == '/') {
      if (last_is_slash) {
        str.erase(it);
      } else {
        last_is_slash = true;
      }
    } else {
      last_is_slash = false;
    }
  }
}

bool AMArchive::compress(const string &dst_path,
                         const string &src_list,
                         const AM_COMPRESS_TYPE type,
                         const std::string &base_dir)
{
  char old_dir[DIR_LEN] = {0};
  string sub_str;
  string base_dir_path;
  string f_path;
  string sub_src;
  struct archive *a = nullptr;
  struct archive *disk = nullptr;
  DIR *dirp = nullptr;
  struct dirent *direntp = nullptr;
  size_t s_pos = 0;
  size_t e_pos = 0;
  size_t pos = 0;
  int32_t len = 0;
  bool ret = true;
  bool ch_dir = false;
  bool use_base_dir = false;
  bool is_archive_open = false;

  do {
    if (src_list.empty() ||
        dst_path.empty() ||
        !getcwd(old_dir, sizeof(old_dir)) ||
        old_dir[sizeof(old_dir) - 1] != '\0') {
      ERROR("src/dst file is not specified or dir length is too long.");
      ret = false;
      break;
    }

    INFO("archive: %s\nsources: %s\nmode: %d\n base_dir: %s\n",
         dst_path.c_str(), src_list.c_str(), type, base_dir.c_str());

    pos = dst_path.find_last_of('/');
    if (pos != string::npos) {
      f_path = dst_path.substr(0, pos + 1);
      if (access(f_path.c_str(), F_OK)) {
        if (create_dir(f_path)) {
          ERROR("Failed to create dir %s\n", f_path.c_str());
          ret = false;
          break;
        }
      }
    }

    if (!base_dir.empty()) {
      struct stat statbuff;

      use_base_dir = true;
      if (stat(base_dir.c_str(), &statbuff) < 0) {
        base_dir_path = base_dir;
        remove_duplicated_slash(base_dir_path);
        len = base_dir_path.length();
        if (len > 1 && base_dir_path[len - 1] == '/') {
          base_dir_path.erase(len - 1);
        }
        INFO("base_dir_path=%s\n", base_dir_path.c_str());
      } else {
        if (S_ISDIR(statbuff.st_mode)) {
          char base_dir_str[DIR_LEN] = {0};

          if (chdir(base_dir.c_str())) {
            ERROR("faid to change dir to %s", base_dir.c_str());
            ret = false;
            break;
          }
          ch_dir = true;
          if (!getcwd(base_dir_str, sizeof(base_dir_str)) ||
              base_dir_str[sizeof(base_dir_str) - 1] != '\0') {
            PRINTF("get base dir path error, stop using base dir\n");
            use_base_dir = false;
            ret = false;
            break;
          } else {
            base_dir_path = base_dir_str;
            INFO("base_dir_path=%s\n", base_dir_path.c_str());
          }
          if (chdir(old_dir)) {
            INFO("faid to change dir to %s", old_dir);
          } else {
            ch_dir = false;
          }
        } else {
          use_base_dir = false;
          PRINTF("base folder %s is invalid, stop using base dir.\n",
                 base_dir.c_str());
          ret = false;
          break;
        }
      }
    }

    if (nullptr == (a = archive_write_new())) {
      ERROR("Failed to create archive object!");\
      ret = false;
      break;
    }

    switch (type) {
      case AM_COMPRESS_TAR:
        ret = (ARCHIVE_OK == archive_write_add_filter_none(a));
        break;
      case AM_COMPRESS_AUTO:
      case AM_COMPRESS_XZ:
        ret = (ARCHIVE_OK == archive_write_add_filter_xz(a));
        break;
      case AM_COMPRESS_BZIP2:
        ret = (ARCHIVE_OK == archive_write_add_filter_bzip2(a));
        break;
      case AM_COMPRESS_GZIP:
        ret = (ARCHIVE_OK == archive_write_add_filter_gzip(a));
        break;
      case AM_COMPRESS_COMPRESS:
        ret = (ARCHIVE_OK == archive_write_add_filter_compress(a));
        break;
      default: {
        if (type == AM_COMPRESS_7Z) {
          PRINTF("Only 7z uncompressing is supported! Abort!");
        } else {
          PRINTF("Unsupported compressing method(%d)! Aborted!", type);
        }
        ret = false;
      }break;
    }

    if (!ret) {
      switch(type) {
        case AM_COMPRESS_TAR:
        case AM_COMPRESS_AUTO:
        case AM_COMPRESS_XZ:
        case AM_COMPRESS_BZIP2:
        case AM_COMPRESS_GZIP:
        case AM_COMPRESS_COMPRESS:
          ERROR(archive_error_string(a));
          break;
        default: break;
      }
      break;
    }

    if (ARCHIVE_OK != archive_write_set_format_ustar(a)) {
      ret = false;
      ERROR(archive_error_string(a));
      break;
    }

    if (ARCHIVE_OK != archive_write_open_filename(a, dst_path.c_str())) {
      ret = false;
      ERROR(archive_error_string(a));
      break;
    }
    is_archive_open = true;

    if (nullptr == (disk = archive_read_disk_new())) {
      ret = false;
      ERROR("Failed to create disk archive!");
      break;
    }

    if (ARCHIVE_OK != archive_read_disk_set_standard_lookup(disk)) {
      ret = false;
      ERROR(archive_error_string(disk));
      break;
    }

    do {
      s_pos = src_list.find_first_not_of(",", s_pos);
      if (s_pos != string::npos) {
        e_pos = src_list.find_first_of(',', s_pos);
        if (e_pos != string::npos) {
          sub_str = src_list.substr(s_pos, e_pos - s_pos);
          s_pos = e_pos;
        } else {
          sub_str = src_list.substr(s_pos, e_pos);
        }
      } else {
        break;
      }

      INFO("compress path: %s\n", sub_str.c_str());
      remove_duplicated_slash(sub_str);

      if (!use_base_dir) {

        len = sub_str.length();
        if (sub_str[len - 1] == '*') {
          pos = sub_str.find_last_of('/');
          if (pos != string::npos) {
            if (sub_str.find_last_not_of('/', pos) != string::npos) {
              f_path = sub_str.substr(0, pos + 1);
              if (access(f_path.c_str(), F_OK)) {
                printf("%s not found, ignore.\n", f_path.c_str());
                continue;
              }
              INFO("chdir to %s\n", f_path.c_str());
              if(chdir(f_path.c_str())) {
                ERROR("failed to chdir to %s", f_path.c_str());
                ret = false;
                break;
              } else {
                ch_dir = true;
              }
            } else {
              /* ignore */
              PRINTF("ignore: compress rootdir \"/\"!");
              continue;
            }
          }

          if ((dirp = opendir("./")) == NULL) {
            ERROR("Open folder error\n");
            ret = false;
            break;
          }

          while ((direntp = readdir(dirp)) != NULL) {
            if (strncmp(direntp->d_name, ".", 1) &&
                strncmp(direntp->d_name, "..", 2)) {
              INFO("add %s to archive\n", direntp->d_name);
              ret = add_files_into_archive(a, direntp->d_name);
              if (!ret) {
                ERROR("Failed to add %s into archive", direntp->d_name);
              }
            }
          }
        } else {
          if (access(sub_str.c_str(), F_OK)) {
            printf("%s not found, ignore.\n", sub_str.c_str());
            continue;
          }

          pos = sub_str.find_last_not_of('/');
          if (pos == string::npos) {
            /* ignore */
            PRINTF("ignore: compress rootdir \"/\"!");
            continue;
          } else {
            pos = sub_str.find_last_of('/', pos);
            if (pos != string::npos) {
              f_path = sub_str.substr(0, pos + 1);
              INFO("chdir to %s\n", f_path.c_str());
              if (chdir(f_path.c_str())) {
                ERROR("failed to chdir to %s", f_path.c_str());
                ret = false;
                break;
              } else {
                ch_dir = true;
              }
              sub_src = sub_str.substr(pos + 1);
            } else {
              sub_src = sub_str;
            }
            /* compress dir sub_src */
            INFO("add %s to archive\n", sub_src.c_str());
            ret = add_files_into_archive(a, sub_src);
            if (!ret) {
              ERROR("Failed to add %s into archive", sub_src.c_str());
            }
          }
        }
      } else { /* use_base_dir = true */
        char dir_path[DIR_LEN] = {0};
        struct stat statbuf;
        bool is_dir = false;

        len = sub_str.length();
        if (sub_str[len - 1] == '*') {
          sub_str.erase(len - 1);
        }

        if (access(sub_str.c_str(), F_OK)) {
          printf("%s not found, ignore.\n", sub_str.c_str());
          continue;
        }

        if (stat(sub_str.c_str(), &statbuf) < 0) {
          printf("get %s stat error, add into archive: ignore",
                 sub_str.c_str());
          continue;
        }

        if (S_ISDIR(statbuf.st_mode)) {
          f_path = sub_str;
          is_dir = true;
        } else {
          is_dir = false;
          pos = sub_str.find_last_of('/');
          if (pos != string::npos) {
            f_path = sub_str.substr(0, pos + 1);
            sub_src = sub_str.substr(pos + 1);
          } else {
            f_path = ".";
            sub_src = sub_str;
          }
        }

        INFO("chdir to %s\n", f_path.c_str());
        if (chdir(f_path.c_str())) {
          ERROR("failed to chdir to %s", f_path.c_str());
          ret = false;
          break;
        } else {
          ch_dir = true;
        }

        if (getcwd(dir_path, sizeof(dir_path)) &&
            dir_path[sizeof(dir_path) - 1] == '\0') {
          string tmp_compress = dir_path;
          string rest_path;
          size_t pos_t = 0;

          INFO("current path = %s\n", dir_path);

          pos = tmp_compress.find(base_dir_path);
          if (pos != string::npos) {

            pos_t = base_dir_path.find_last_not_of('/');
            if (pos_t != string::npos) {
              pos_t = base_dir_path.find_last_of('/', pos_t);
              if (pos_t != string::npos) {
                pos = pos + pos_t + 1;
                f_path = tmp_compress.substr(0, pos);
                rest_path = tmp_compress.substr(pos);
              } else {
                f_path = tmp_compress.substr(0, pos);
                rest_path = tmp_compress.substr(pos);
              }
            } else {
              f_path = "/";
              rest_path = tmp_compress.substr(1);
            }

            if (!access(f_path.c_str(), F_OK)) {
              INFO("chdir to %s\n", f_path.c_str());
              if (chdir(f_path.c_str())) {
                ERROR("failed to chdir to %s", f_path.c_str());
                ret = false;
                break;
              } else {
                ch_dir = true;
              }
              if (!is_dir) {
                rest_path = rest_path + "/" + sub_src;
              }
              INFO("add %s to archive\n", rest_path.c_str());
              ret = add_files_into_archive(a, rest_path);
              if (!ret) {
                ERROR("Failed to add %s into archive", rest_path.c_str());
              }
            } else {
              ERROR("Invalid base folder %s for %s\n",
                    base_dir.c_str(), sub_str.c_str());
              ret = false;
            }
          } else {
            ERROR("Invalid base folder %s for %s\n",
                  base_dir.c_str(), sub_str.c_str());
            ret = false;
          }
        } else {
          PRINTF("Get base dir path error, stopped.\n");
          ret = false;
        }
      }

      if (ch_dir) {
        INFO("chdir back to %s\n", old_dir);
        if (!chdir(old_dir)) {
          ch_dir = false;
        } else {
          ret = false;
          ERROR("Failed to change directory back to %s\n", old_dir);
        }
      }

      if (!ret) {
        break;
      }

    } while(e_pos != string::npos);

  } while (0);

  if (ch_dir) {
    if (chdir(old_dir)) {
      ERROR("failed to chdir to %s", old_dir);
    } else {
      ch_dir = false;
    }
  }

  if (disk) {
    archive_read_free(disk);
  }

  if (a) {
    if (is_archive_open) {
      archive_write_close(a);
    }
    archive_write_free(a);
  }

  if (!ret && (0 == access(dst_path.c_str(), F_OK))) {
    remove(dst_path.c_str());
  }

  return ret;
}

bool AMArchive::get_file_list(const string &src_path,
                              string &name_list,
                              const AM_COMPRESS_TYPE type)
{
  return extract(src_path, name_list, false, type);
}

bool AMArchive::decompress(const std::string &src_path,
                           const std::string &dst_path,
                           const AM_COMPRESS_TYPE type)
{
  string extarc_path(dst_path);

  return extract(src_path, extarc_path, true, type);
}


int32_t AMArchive::copy_data(struct archive *ar,
                             struct archive *aw)
{
  int32_t ret;
  const void *buff;
  size_t size;
  int64_t offset;

  for (;;) {
    ret = archive_read_data_block(ar, &buff, &size, &offset);
    if (ret == ARCHIVE_EOF) {
      INFO("get file eof");
      ret = ARCHIVE_OK;
      break;
    }
    if (ret != ARCHIVE_OK) {
      break;
    }
    ret = archive_write_data_block(aw, buff, size, offset);
    if (ret != ARCHIVE_OK) {
      ERROR(archive_error_string(ar));
      break;
    }
  }

  return ret;
}

bool AMArchive::create_dir(const std::string &path)
{
  bool ret = true;
  int32_t res = 0;
  size_t s_pos = 0;
  size_t e_pos = 0;
  string sub_str;

  do {

    if (path.empty()) break;

    while (s_pos != string::npos) {
      s_pos = path.find_first_not_of('/', s_pos);
      if (s_pos != string::npos) {
        e_pos = path.find_first_of('/', s_pos);
        sub_str = path.substr(0, e_pos);
        if (access(sub_str.c_str(), F_OK)) {
          res = mkdir(sub_str.c_str(), 0755) == 0 ? 0 : errno;
          if (res) {
            ERROR("Failed to create folder: %s\n", sub_str.c_str());
            ret = false;
            break;
          }
        }
        s_pos = e_pos;
      }
    }
  } while (0);

  return ret;
}

bool AMArchive::extract(const string &src_path,
                        string &dst_or_list,
                        const bool do_extract,
                        const AM_COMPRESS_TYPE type)
{
  bool ret = true;
  bool ch_dir = false;
  bool is_archive_open = false;
  int32_t flags;
  struct archive *a = nullptr;
  struct archive *ext = nullptr;
  struct archive_entry *entry = nullptr;
  int32_t result;
  char current_dir[DIR_LEN] = {0};

  do {
    if (src_path.empty() || access(src_path.c_str(), F_OK)) {
      printf("Filename is empty or file not found.\n");
      ret = false;
      break;
    }

    if (type > AM_COMPRESS_COMPRESS) {
      INFO("Try to use 7z to uncompress...");
      AMIDec7zPtr dec = AMIDec7z::create(src_path);
      if (dec) {
        if (do_extract) {
          ret = dec->dec7z(dst_or_list);
        } else {
          ret = dec->get_filename_in_7z(dst_or_list);
        }
        if (ret) {
          break;
        } else {
          if (type == AM_COMPRESS_AUTO) {
            INFO("Archive is not in 7z format, try other methods to uncompress.");
            ret = true;
          } else {
            if (do_extract) {
              printf("Failed to extract 7z file.\n");
            } else {
              printf("Failed to get file name 7z archive.\n");
            }
            break;
          }
        }
      } else if (type != AM_COMPRESS_AUTO){
        ret = false;
        break;
      }
      INFO("Try to use other uncompress method ...");
    }

    flags = ARCHIVE_EXTRACT_TIME;
    flags |= ARCHIVE_EXTRACT_PERM;
    flags |= ARCHIVE_EXTRACT_ACL;
    flags |= ARCHIVE_EXTRACT_FFLAGS;

    if (nullptr == (a = archive_read_new())) {
      ERROR("Failed to create archive object!");\
      ret = false;
      break;
    }

    if (nullptr ==(ext = archive_write_disk_new())) {
      ERROR("Failed to create archive disk object!");\
      ret = false;
      break;
    }

    if (ARCHIVE_OK != archive_write_disk_set_options(ext, flags)) {
      WARN("Failed to set archive write disk options");
    }

    if (ARCHIVE_OK != archive_read_support_format_tar(a)) {
      ERROR("Failed to set tar filter, err: %s", archive_error_string(a));
      ret = false;
      break;
    }

    switch(type) {
      case AM_COMPRESS_TAR:
        break;
      case AM_COMPRESS_XZ:
        ret = (ARCHIVE_OK == archive_read_support_filter_xz(a));
        break;
      case AM_COMPRESS_BZIP2:
        ret = (ARCHIVE_OK == archive_read_support_filter_bzip2(a));
        break;
      case AM_COMPRESS_GZIP:
        ret = (ARCHIVE_OK == archive_read_support_filter_gzip(a));
        break;
      case AM_COMPRESS_COMPRESS:
        ret = (ARCHIVE_OK == archive_read_support_filter_compress(a));
        break;
      case AM_COMPRESS_AUTO:
        ret = (ARCHIVE_OK == archive_read_support_filter_xz(a));
        ret &= (ARCHIVE_OK == archive_read_support_filter_bzip2(a));
        ret &= (ARCHIVE_OK == archive_read_support_filter_gzip(a));
        ret &= (ARCHIVE_OK == archive_read_support_filter_compress(a));
        break;
      default:
        ret = false;
        break;
    }

    if (!ret) {
      ERROR("Failed to add archive uncompress filter or "
          "specified archive format not supported");
      break;
    }

    if (ARCHIVE_OK != archive_write_disk_set_standard_lookup(ext)) {
      ret = false;
      ERROR(archive_error_string(ext));
      break;
    }

    if ((result = archive_read_open_filename(a, src_path.c_str(), 10240))) {
      PRINTF(archive_error_string(a));
      ret = false;
      break;
    }

    is_archive_open = true;

    if (do_extract) {
      if (!dst_or_list.empty()) {
        if (access(dst_or_list.c_str(), F_OK)) {
          if (!create_dir(dst_or_list)) {
            ERROR("Failed to create dir %s\n", dst_or_list.c_str());
            ret = false;
            break;
          }
        }

        if (!getcwd(current_dir, sizeof(current_dir)) ||
            current_dir[sizeof(current_dir) - 1] != '\0') {
          ERROR("current dir length is too long.");
          ret = false;
          break;
        }
        INFO("change dir to %s", dst_or_list.c_str());
        if (chdir(dst_or_list.c_str())) {
          ERROR("failed to chdir to %s", dst_or_list.c_str());
          ret = false;
          break;
        } else {
          ch_dir = true;
        }
      }
    } else {
      dst_or_list.clear();
    }

    for (;;) {
      result = archive_read_next_header(a, &entry);
      if (result == ARCHIVE_EOF)
        break;
      if (result != ARCHIVE_OK) {
        PRINTF(archive_error_string(a));
        ret = false;
        break;
      }

      if (!do_extract) {
        dst_or_list = dst_or_list + archive_entry_pathname(entry) + ",";
      } else {
        result = archive_write_header(ext, entry);
        if (result != ARCHIVE_OK) {
          PRINTF(archive_error_string(a));
        } else {
          copy_data(a, ext);
        }
      }
    }

    if (ch_dir) {
      /* change dir back */
      INFO("change dir back to %s", current_dir);
      if (chdir(current_dir)) {
        ERROR("failed to chdir to %s", dst_or_list.c_str());
        ret = false;
        break;
      } else {
        ch_dir = false;
      }
    }
  } while (0);

  if (ext) {
    archive_write_free(ext);
  }

  if (a) {
    if (is_archive_open) {
      archive_read_close(a);
    }
    archive_read_free(a);
  }

  return ret;
}
