/*******************************************************************************
 * test_config_set.cpp
 *
 * History:
 *   Apr 7, 2016 - [longli] created file
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
#include "am_config_set.h"
#include "getopt.h"

using namespace std;

#define CONFIG_SET_PATH "/etc/config_samples"
#define DEF_EXTRACT_DIR "/etc"
#define BASE_DIR        "/etc/oryx"

enum AM_MODE
{
  AM_NOT_SET = 0,
  AM_CREATE,
  AM_EXTRACT,
  AM_LIST_CFG

};

static AM_MODE am_mode = AM_NOT_SET;
static string cfg_set_path(CONFIG_SET_PATH);
static string cfg_extract_dir(DEF_EXTRACT_DIR);
static string cfg_name("");
static string src_list("");
static string base_dir(BASE_DIR);

/* options and usage */
#define NO_ARG    0
#define HAS_ARG   1
static const char *short_options = "b:s:C:f:cxlh";
static struct option long_options[] = {
  {"cfg_set_path", HAS_ARG, 0, 's'},
  {"create", NO_ARG, 0, 'c'},
  {"extract", NO_ARG, 0, 'x'},
  {"list", NO_ARG, 0, 'l'},
  {"cfg_name", HAS_ARG, 0, 'f'},
  {"extract_path", HAS_ARG, 0, 'C'},
  {"base_dir", HAS_ARG, 0, 'b'},
  {"help", NO_ARG, 0, 'h'},
  {0, 0, 0, 0}
};

struct hint_s {
    const char *arg;
    const char *str;
};

static const struct hint_s hint[] = {
  {"string", "\tspecify the location of the configuration set"},
  {"", "create a configure, without filepath default stored under \"/etc/config_samples\""},
  {"", "extract a configure, default extract to \"/etc\""},
  {"", "list all the configurations"},
  {"string", "\tthe configure name"},
  {"string", "where to extract configurations"},
  {"string", "\treserve the specified folder"},
  {"", "print out this help information"},
};

static void usage()
{
  uint32_t i = 0;

  printf("Usage: test_config_set [option...] [FILE]...\n");
  printf("\nOptions:\n");
  printf("\nOperation mode(exclusive options):\n");
  for (i = 1; i < sizeof(long_options) / sizeof(long_options[0]) - 1; i++) {
    if (long_options[i].val == 'f') {
      printf("\nFile/path selection:\n");
    } else if (long_options[i].val == 'h') {
      printf("\nOther options:\n");
    }
    if (isalpha(long_options[i].val))
      printf("-%c ", long_options[i].val);
    else
      printf("   ");
    printf("--%s", long_options[i].name);
    if (hint[i].arg[0] != 0)
      printf(" [%s]", hint[i].arg);
    printf("\t%s\n", hint[i].str);
  }

  printf("\ne.g. test_config_set -l \n");
  printf("     test_config_set -cf oryx_base.tar.xz /etc/oryx \n");
  printf("     test_config_set -cf init.d.tar.xz /etc/init.d -b /etc/init.d\n");
  printf("     test_config_set -cf set_2x_hdr.tar.xz /etc/oryx/video/features.acs \n");
  printf("     test_config_set -xf set_2x_hdr.tar.xz -C /etc/ \n");
  printf("     test_config_set -cf customise.tar.xz /etc/oryx/video/features.acs "
      "/etc/oryx/stream/muxer/muxer-mp4.acs\n");
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
          if (am_mode == AM_NOT_SET) {
            am_mode = AM_CREATE;
          } else {
            PRINTF("Set operation mode repeatedly\n");
            show_help = true;
          }
          break;
        case 'x':
          if (am_mode == AM_NOT_SET) {
            am_mode = AM_EXTRACT;
          } else {
            PRINTF("Set operation mode repeatedly\n");
            show_help = true;
          }
          break;
        case 's':
          if (optarg) {
            cfg_set_path = optarg;
          } else {
            show_help = true;
          }
          break;
        case 'f':
          if (optarg) {
            cfg_name = optarg;
          } else {
            show_help = true;
          }
          break;
        case 'C':
          if (optarg) {
            cfg_extract_dir = optarg;
          } else {
            show_help = true;
          }
          break;
        case 'b':
          if (optarg) {
            base_dir = optarg;
          } else {
            show_help = true;
          }
          break;
        case 'l':
          if (am_mode == AM_NOT_SET) {
            am_mode = AM_LIST_CFG;
          } else {
            PRINTF("Set operation mode repeatedly\n");
            show_help = true;
          }
          break;
        default:
          show_help = true;
          break;
      }
    }

    if (show_help) break;

    if (am_mode == AM_NOT_SET) {
      PRINTF("Operation mode must be specified.\n");
      show_help = true;
      break;
    }

    if (am_mode != AM_LIST_CFG && cfg_name.empty()) {
      PRINTF("configure name (-f) must be specified.\n");
      show_help = true;
      break;
    }

    if (am_mode == AM_CREATE) {
      para_index = optind;
      src_list.clear();
      while (para_index < argc) {
        src_list = src_list + argv[para_index] + ",";
        ++para_index;
      }

      if (src_list.empty()) {
        PRINTF("Compress src is empty\n");
        show_help = true;
      } else {
        INFO("am_src_list = %s\n", src_list.c_str());
      }
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

int32_t main(int32_t argc, char**argv)
{
  int32_t ret = 0;
  string name_list("");
  AMConfigSet *cfg_set = NULL;

  do {
    if (!init_param(argc, argv)) {
      ret = -1;
      break;
    }

    cfg_set = AMConfigSet::create(cfg_set_path);
    if (!cfg_set) {
      ret = -1;
      break;
    }

    switch (am_mode) {
      case AM_CREATE:
        if (cfg_set->create_config(cfg_name, src_list, base_dir)) {
          printf("create configure file %s done.\n", cfg_name.c_str());
        } else {
          printf("Failed to create configure file %s.\n", cfg_name.c_str());
          printf("Type %s -h for more help.\n", argv[0]);
        }
        break;
      case AM_EXTRACT:
        if (cfg_set->extract_config(cfg_name, cfg_extract_dir)) {
          printf("Extract configuration Done!\n");
        } else {
          printf("Failed to extract %s to %s\n",
                 cfg_name.c_str(), cfg_extract_dir.c_str());
          ret = -1;
        }
        break;
      case AM_LIST_CFG:
        if (cfg_set->get_config_list(name_list)) {
          if (!name_list.empty()) {
            string sub_str;
            uint32_t s_pos = 0;
            uint32_t e_pos = 0;

            printf("configure files list:\n");
            do {
              s_pos = name_list.find_first_not_of(',', s_pos);
              if (s_pos != string::npos) {
                e_pos = name_list.find_first_of(',', s_pos);
                if (e_pos != string::npos) {
                  sub_str = name_list.substr(s_pos, e_pos - s_pos);
                  s_pos = e_pos;
                } else {
                  sub_str = name_list.substr(s_pos, e_pos);
                }
              } else {
                break;
              }
              printf("--%s\n", sub_str.c_str());

            } while (e_pos !=string::npos);
          } else {
            PRINTF("No configure file in %s\n", cfg_set_path.c_str());
          }
        } else {
          printf("Failed to get config list\n");
          ret = -1;
        }
        break;
      default:
        PRINTF("Unknown mode.\n");
        break;
    }
  } while (0);

  delete cfg_set;
  cfg_set = NULL;

  return ret;
}
