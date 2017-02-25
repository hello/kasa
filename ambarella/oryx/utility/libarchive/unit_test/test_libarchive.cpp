/*******************************************************************************
 * test_libarchive.cpp
 *
 * History:
 *   Apr 18, 2016 - [longli] created file
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
#include "getopt.h"
#include "am_log.h"
#include "am_libarchive_if.h"

using namespace std;

enum AM_COMPRESS_MODE
{
  AM_UNKNOWN = 0,
  AM_CREATE,
  AM_EXTRACT,
  AM_LIST
};

static AM_COMPRESS_MODE am_mode = AM_UNKNOWN;
static AM_COMPRESS_TYPE am_type = AM_COMPRESS_TAR;
static string am_archive("");
static string am_src_list("");
static string am_extract_path("");
static string am_base_dir("");

/* options and usage */
#define NO_ARG    0
#define HAS_ARG   1
static const char *short_options = "b:f:C:scxjJzZth";
static struct option long_options[] = {
  {"create", NO_ARG, 0, 'c'},
  {"extract", NO_ARG, 0, 'x'},
  {"list", NO_ARG, 0, 't'},
  {"bzip2", NO_ARG, 0, 'j'},
  {"xz", NO_ARG, 0, 'J'},
  {"gzip", NO_ARG, 0, 'z'},
  {"compress", NO_ARG, 0, 'Z'},
  {"7z", NO_ARG, 0, 's'},
  {"file", HAS_ARG, 0, 'f'},
  {"extract_dir", HAS_ARG, 0, 'C'},
  {"base_dir", HAS_ARG, 0, 'b'},
  {"help", NO_ARG, 0, 'h'},
  {0, 0, 0, 0}
};

struct hint_s {
    const char *arg;
    const char *str;
};

static const struct hint_s hint[] = {
  {"", "create a new archive"},
  {"", "extract files from an archive"},
  {"", "list the contents of an archive"},
  {"", "filter the archive through bzip2"},
  {"", "\tfilter the archive through xz"},
  {"", "filter the archive through gzip"},
  {"", "filter the archive through compress"},
  {"", "\tfilter the archive through 7z, only uncompress available"},
  {"str", "\tspecify the archive file"},
  {"str", "extract the archive to directory DIR"},
  {"str", "reserve the specified folder in the archive"},
  {"", "print out this help information"},
};

static void usage()
{
  uint32_t i = 0;

  printf("Usage: test_libarchive [option...] [FILE]...\n");
  printf("\nOptions:\n");
  printf("\nMain operation mode(exclusive options):\n");
  for (i = 0; i < sizeof(long_options) / sizeof(long_options[0]) - 1; i++) {
    if (isalpha(long_options[i].val)) {
      if (long_options[i].val == 'j') {
        printf("\nCompression options:\n");
      } else if (long_options[i].val == 'f') {
        printf("\nFile/path selection:\n");
      } else if (long_options[i].val == 'h') {
        printf("\nOther options:\n");
      }
      printf("-%c ", long_options[i].val);
    } else {
      printf("   ");
    }
    printf("--%s", long_options[i].name);
    if (hint[i].arg[0] != 0)
      printf(" [%s]", hint[i].arg);
    printf("\t%s\n", hint[i].str);
  }
  printf("\nNote: This APP default compresses the lowest level folder into the archive if compress a folder.\n"
      "      If compress a file, it will only compress the file not including the path.\n"
      "      If you want reserve the path, please specify \"-b\" to indicate which folder to reserve.\n");
  printf("\ne.g. test_libarchive -cJf oryx.tar.xz /etc/oryx/\n");
  printf("     test_libarchive -cJf oryx.tar.xz /etc/oryx/*\n");
  printf("     test_libarchive -cJf etc.tar.xz /etc/oryx /etc/init.d\n");
  printf("     test_libarchive -cJf etc.tar.xz /etc/oryx,/etc/init.d\n");
  printf("     test_libarchive -cJf oryx.tar.xz "
      "/etc/oryx/video/features.acs,/etc/oryx/video/lbr.acs -b /etc/oryx\n");
  printf("     test_libarchive -xJf oryx.tar.xz -C /etc/\n");

}

static bool init_param(int argc, char **argv)
{
  bool ret = true;
  bool show_help = false;
  int32_t ch;
  int32_t opt_index;
  int32_t para_index = 0;

  do {
    if (argc == 1) {
      show_help = true;
      break;
    }

    while ((ch = getopt_long(argc,
                             argv,
                             short_options,
                             long_options,
                             &opt_index)) != -1) {
      switch (ch) {
        case 'c':
          if (am_mode == AM_UNKNOWN) {
            am_mode = AM_CREATE;
          } else {
            PRINTF("Specify more than one operation mode.\n");
            show_help = true;
          }
          break;
        case 'x':
          if (am_mode == AM_UNKNOWN) {
            am_mode = AM_EXTRACT;
          } else {
            PRINTF("Specify more than one operation mode.\n");
            show_help = true;
          }
          break;
        case 't':
          if (am_mode == AM_UNKNOWN) {
            am_mode = AM_LIST;
          } else {
            PRINTF("Specify more than one operation mode.\n");
            show_help = true;
          }
          break;
        case 'f':
          if (optarg) {
            am_archive = optarg;
          } else {
            show_help = true;
          }
          break;
        case 'C':
          if (optarg) {
            am_extract_path = optarg;
          } else {
            show_help = true;
          }
          break;
        case 'b':
          if (optarg) {
            am_base_dir = optarg;
          } else {
            show_help = true;
          }
          break;
        case 'J':
          am_type = AM_COMPRESS_XZ;
          break;
        case 'j':
          am_type = AM_COMPRESS_BZIP2;
          break;
        case 'z':
          am_type = AM_COMPRESS_GZIP;
          break;
        case 'Z':
          am_type = AM_COMPRESS_COMPRESS;
          break;
        case 's':
          am_type = AM_COMPRESS_7Z;
          break;
        default:
          show_help = true;
          break;
      }
    }

    if (show_help) break;

    if (am_mode == AM_UNKNOWN) {
      PRINTF("Operation mode must be specified.\n");
      show_help = true;
      break;
    }

    if (am_archive.empty()) {
      PRINTF("the archive file (-f) must be specified.\n");
      show_help = true;
      break;
    } else if (am_mode != AM_CREATE && access(am_archive.c_str(), F_OK)){
      PRINTF("the archive file %s not found.\n", am_archive.c_str());
      break;
    }

    if (am_mode == AM_CREATE) {
      if (am_type == AM_COMPRESS_7Z) {
        PRINTF("This program cannot create only 7z archive, "
            "only uncompress 7z available.\n");
        show_help = true;
        break;
      }
      para_index = optind;
      am_src_list.clear();
      while (para_index < argc) {
        am_src_list = am_src_list + argv[para_index] + ",";
        ++para_index;
      }
      if (am_src_list.empty()) {
        PRINTF("Please specify the source file(s) to be compressed.\n");
        show_help = true;
        break;
      }
      INFO("am_src_list = %s\n", am_src_list.c_str());
    } else {
      if (optind < argc) {
        printf("More parameters than need, ignore.\n");
      }
    }

  } while (0);

  if (show_help) {
    usage();
    ret = false;
  }

  return ret;
}

int32_t main(int32_t argc, char** argv)
{
  int32_t ret = 0;
  AMIArchive *ins = NULL;

  do {
    if (!init_param(argc, argv)) {
      ret = -1;
      break;
    }

    ins = AMIArchive::create();
    if (!ins) {
      printf("Failed to create AMIArchive instance.\n");
      ret = -1;
      break;
    }

    switch (am_mode) {
      case AM_CREATE:
        if (!ins->compress(am_archive, am_src_list, am_type, am_base_dir)) {
          printf("Failed to create %s\nsources: %s\n",
                 am_archive.c_str(), am_src_list.c_str());
          ret = -1;
        }
        break;
      case AM_EXTRACT:
        if (!ins->decompress(am_archive, am_extract_path, am_type)) {
          if (am_extract_path.empty()) {
            printf("Failed to extract %s\n", am_archive.c_str());
          } else {
            printf("Failed to extract %s to %s\n",
                   am_archive.c_str(), am_extract_path.c_str());
          }
          ret = -1;
        }
        break;
      case AM_LIST:
        if (!ins->get_file_list(am_archive, am_src_list, am_type)) {
          printf("Failed to get file list from %s\n", am_archive.c_str());
          ret = -1;
        } else {
          if (!am_src_list.empty()) {
            string sub_str;
            uint32_t s_pos = 0;
            uint32_t e_pos = 0;

            do {
              s_pos = am_src_list.find_first_not_of(',', s_pos);
              if (s_pos != string::npos) {
                e_pos = am_src_list.find_first_of(',', s_pos);
                if (e_pos != string::npos) {
                  sub_str = am_src_list.substr(s_pos, e_pos - s_pos);
                  s_pos = e_pos;
                } else {
                  sub_str = am_src_list.substr(s_pos, e_pos);
                }
              } else {
                break;
              }
              printf("%s\n", sub_str.c_str());

            } while (e_pos !=string::npos);
          } else {
            PRINTF("No file in %s\n", am_archive.c_str());
          }
        }
        break;
      default:
        printf("Unknown mode.\n");
        break;
    }
  } while (0) ;

  delete ins;

  return ret;
}
