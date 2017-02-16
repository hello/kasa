/*******************************************************************************
 * test_upgrade_fw.cpp
 *
 * History:
 *   2015-1-12 - [longli] created file
 *
 * Copyright (C) 2008-2015, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/

#include "am_base_include.h"
#include "getopt.h"
#include <sys/reboot.h>
#include <string>
#include "am_upgrade_if.h"

using namespace std;

#define DEFAULT_MODE        (AM_DOWNLOAD_AND_UPGRADE)
#define DEVICE_NAME         "adc"
#define DST_DIR             "/dev/adc"
#define MOUNT_FS_TYPE       "jffs2"

static AM_UPGRADE_MODE mode = DEFAULT_MODE;
static bool re_boot = false;
static uint32_t timeout = 5;
static string url("");
static string dst_path(DST_DIR);
static string fw_path("");
static string user_name("");
static string passwd("");
// options and usage
#define NO_ARG    0
#define HAS_ARG   1
static const char *short_options = "m:f:s:t:u:p:rh";
static struct option long_options[] = {
  {"mode", HAS_ARG, 0, 'm'},
  {"fw_path", HAS_ARG, 0, 's'},
  {"dst_file", HAS_ARG, 0, 'f'},
  {"timeout", HAS_ARG, 0, 't'},
  {"user", HAS_ARG, 0, 'u'},
  {"passwd", HAS_ARG, 0, 'p'},
  {"reboot", NO_ARG, 0, 'r'},
  {0, 0, 0, 0}
};

struct hint_s {
    const char *arg;
    const char *str;
};

static const struct hint_s hint[] = {
  {"0~2", "\t\tspecify upgrade mode:\n\t\t\t\t  0: only download firmware\n"
   "\t\t\t\t  1: only upgrade\n\t\t\t\t  "
   "2: download and upgrade (default mode)"},
  {"string", "\tset the pathname of upgrade firmware"},
  {"string", "\trename the download file to dst_file"},
  {"num", "\tAbort the connect if not connect to server within num seconds,"
  "default: 5s"},
  {"string", "\tauthentication user name"},
  {"string", "\tauthentication password"},
  {"", "\t\tauto reboot to upgrade"},
};

static void usage()
{
  uint32_t i = 0;

  printf("Usage: test_upgrade_fw [options] <source url>\n");
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
  printf("\nWhen not in mode 1, <source url> must be set.\n");
  printf("Every mode supported options list:\n");
  printf("  mode 0: -f, -t, -u, -p\n");
  printf("  mode 1: -s, -r\n");
  printf("  mode 2: -f, -t, -u, -p, -r\n");
  printf("e.g. test_upgrade -m2 http://10.0.0.1:8080/s2lm_ironman.7z -f 123.7z "
      "-u username -p passwd -t 5 -r\n");
}

static bool init_param(int argc, char **argv)
{
  bool ret = true;
  bool show_help = false;
  int32_t ch;
  int32_t opt_index;

  while ((ch = getopt_long(argc,
                           argv,
                           short_options,
                           long_options,
                           &opt_index)) != -1) {
    switch (ch) {
      case 'm':
        if (optarg && isdigit(optarg[0])) {
          mode = (AM_UPGRADE_MODE)atoi(optarg);
          if (mode < 0 || mode > 2) {
            printf("Invalid upgrade mode: %d\n", mode);
            show_help = true;
          }
        } else {
          show_help = true;
        }
        break;
      case 'f':
        if (optarg) {
          dst_path = dst_path +"/" + optarg;
        } else {
          show_help = true;
        }
        break;
      case 't':
        if (optarg && isdigit(optarg[0])) {
          timeout = (uint32_t)atoi(optarg);
        } else {
          show_help = true;
        }
        break;
      case 's':
        if (optarg) {
          fw_path = optarg;
        } else {
          show_help = true;
        }
        break;
      case 'u':
        if (optarg) {
          user_name = optarg;
        } else {
          show_help = true;
        }
        break;
      case 'p':
        if (optarg) {
          passwd = optarg;
        } else {
          show_help = true;
        }
        break;
      case 'r':
        re_boot = true;
        break;
      default:
        show_help = true;
        break;
    }
  }

  if (mode != AM_UPGRADE_ONLY) {
    if (argc > optind) {
      url = argv[argc - 1];
    } else {
      show_help = true;
    }
  }

  if (show_help) {
    usage();
    ret = false;
  }

  return ret;
}

int32_t main(int argc, char **argv)
{
  int32_t ret = 0;
  AM_MOUNT_STATUS mounted_status = AM_NOT_MOUNTED;
  AMIFWUpgradePtr upgrade_ptr = NULL;
  string device_name(DEVICE_NAME);
  string dst_dir(DST_DIR);

  do {
    if (!init_param(argc, argv)) {
      ret = -1;
      break;
    }

    upgrade_ptr = AMIFWUpgrade::get_instance();
    if (!upgrade_ptr) {
      printf("Get instance failed.\n");
      ret = -1;
      break;
    }

    printf("Initializing mode %d...\n", mode);
    if (!upgrade_ptr->set_mode(mode)) {
      printf("Initializing mode %d failed.\n", mode);
      ret = -1;
      break;
    }

    printf("Mount %s to %s...\n", device_name.c_str(), dst_dir.c_str());
    mounted_status = upgrade_ptr->mount_partition(device_name,
                                                  dst_dir,
                                                  MOUNT_FS_TYPE);
    if (mounted_status != AM_MOUNTED &&
        mounted_status != AM_ALREADY_MOUNTED) {
      printf("Mount %s to %s failed!\n",
             device_name.c_str(), dst_dir.c_str());
      ret = -1;
      break;
    }

    if (mode != AM_UPGRADE_ONLY) {
      printf("Setting upgrade source file path to %s\n", url.c_str());
      if (!upgrade_ptr->set_fw_url(url, dst_path)) {
        printf("Setting upgrade source file url to %s failed.\n", url.c_str());
        ret = -1;
        break;
      }

      if (!user_name.empty()) {
        printf("Setting authentication...\n");
        if (!upgrade_ptr->set_dl_user_passwd(user_name, passwd)) {
          printf("Setting authentication (%s) failed.\n", user_name.c_str());
          ret = -1;
          break;
        }
      }

      if (timeout) {
        printf("Setting connecting timeout value...\n");
        if (!upgrade_ptr->set_dl_connect_timeout(timeout)) {
          printf("Setting connecting timeout to %u second(s) failed.\n",
                 timeout);
          ret = -1;
          break;
        }
      }
    }

    if (mode == AM_UPGRADE_ONLY) {
      if (!fw_path.empty()) {
        printf("Setting upgrade firmware path...\n");
        if (!upgrade_ptr->set_fw_path(fw_path)) {
          printf("Setting upgrade firmware path to %s failed.\n",
                 fw_path.c_str());
          ret = -1;
          break;
        }
      }
      printf("Begin to load firmware ...\n");
    } else {
      printf("Begin to download firmware ...\n");
    }

    if (!upgrade_ptr->upgrade()) {
      if (mode == AM_UPGRADE_ONLY) {
        printf("Upgrade firmware failed.\n");
      } else {
        printf("Download upgrade firmware failed.\n");
      }
      ret = -1;
      break;
    }

    if (mounted_status == AM_MOUNTED) {
      printf("Umount %s\n", dst_dir.c_str());
      if (!upgrade_ptr->umount_dir(dst_dir)) {
        printf("Umount %s failed.\n", dst_dir.c_str());
        ret = -1;
        break;
      }
      mounted_status = AM_NOT_MOUNTED;
    }

    if (re_boot && mode != AM_DOWNLOAD_ONLY) {
      PRINTF("Rebooting to complete upgrade ...\n");
      sync();
      reboot(RB_AUTOBOOT);
    } else {
      if (mode != AM_DOWNLOAD_ONLY) {
        PRINTF("Please reboot to complete upgrade!\n");
      }
    }
  } while (0);

  if (mounted_status == AM_MOUNTED) {
    upgrade_ptr->umount_dir(dst_dir);
  }

  return ret;
}
