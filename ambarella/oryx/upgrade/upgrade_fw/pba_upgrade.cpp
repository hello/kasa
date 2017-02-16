/*******************************************************************************
 * pba_upgrade.cpp
 *
 * History:
 *   2015-1-13 - [longli] created file
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
#include "am_log.h"
#include "am_configure.h"
#include <time.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mount.h>
#include <sys/reboot.h>
#include <fstream>
#include "digital_signature_if.h"
#include "pba_upgrade.h"
#include "am_dec7z.h"

#define BUFFER_SIZE             256
#define STR_LEN                 128
#define UPDATE_PATH             "/dev/update_img/"
#define UCODE_DIR               "/lib/firmware/"
#define KERNERL_IMG             "/dev/update_img/Image"
#define UBIFS_IMG               "/dev/update_img/ubifs"
#define MTD_DEVICE_PATH         "/dev/mtd"
#define FLASH_ERASE_CMD         "flash_eraseall"
#define MTD_PRI                 "pri"
#define MTD_LNX                 "lnx"
#define MTD_SWP                 "swp"
#define UPGRADE_PRI_CMD         "upgrade_partition -p -K"
#define UPGRADE_LNX_CMD         "upgrade_partition -p -W -F 1"
#define ENTER_MAIN_CMDLINE      "upgrade_partition -S 0 -C"
#define SET_UPGRADE_FLAG        "upgrade_partition -P"
#define MAINSYS_CMD_LINE "console=ttyS0 ubi.mtd=lnx root=ubi0:rootfs rw rootfstype=ubifs init=/linuxrc"
//#define PBA_CMD_LINE "console=ttyS0 rootfs=ramfs root=/dev/ram rw rdinit=/linuxrc"

using namespace std;

enum AM_UPGRADE_STATUS
{
  AM_NOT_KNOWN                = 0,
  AM_UPGRADE_PREPARE,
  AM_UPGRADE_PREPARE_FAIL,
  AM_DOWNLOADING_FW,
  AM_DOWNLOAD_FW_COMPLETE,
  AM_DOWNLOAD_FW_FAIL,
  AM_INIT_PBA_SYS_DONE,
  AM_INIT_PBA_SYS_FAIL,
  AM_UNPACKING_FW,
  AM_UNPACK_FW_DONE,
  AM_UNPACK_FW_FAIL,
  AM_FW_INVALID,
  AM_WRITEING_FW_TO_FLASH,
  AM_WRITE_FW_TO_FLASH_DONE,
  AM_WRITE_FW_TO_FLASH_FAIL,
  AM_INIT_MAIN_SYS,
  AM_UPGRADE_SUCCESS,
};

static AMConfig *config = NULL;
static bool update_kernel = false;
static bool update_system = false;

static bool create_dir(const char *path)
{
  bool ret = true;
  int32_t res;
  res = mkdir(path, 0777) == 0 ? 0 : errno;
  if (!res || res == EEXIST) {
    ret = true;
  } else {
    ret = false;
  }

  return ret;
}

static bool get_partition_index_str(const string &partition_name,
                             string &index_str)
{
  bool ret = true;
  string line_str;
  string p_name;
  ifstream in_file;

  do {
    if (partition_name.empty()) {
      printf("partition_name is NULL\n");
      ret = false;
      break;
    }

    in_file.open(PROC_MTD_FILE, ios::in);
    if (!in_file.is_open()) {
      printf("Failed to open %s\n", PROC_MTD_FILE);
      ret = false;
      break;
    }

    p_name = "\"" + partition_name + "\"";

    while (!in_file.eof()) {
      getline(in_file, line_str);
      if (line_str.find(p_name) != string::npos) {
        uint32_t s_pos;
        uint32_t e_pos;

        s_pos = line_str.find("mtd");
        if (s_pos == string::npos) {
          printf("Can not find \"mtd\" in %s\n", line_str.c_str());
          ret = false;
          break;
        }

        e_pos = line_str.find_first_of(':', s_pos);
        if (e_pos == string::npos) {
          printf("Can not find \":\" in %s\n", line_str.c_str());
          ret = false;
          break;
        }

        s_pos += 3;
        index_str = line_str.substr(s_pos, e_pos - s_pos);
        if (index_str.empty()) {
          printf("Not find mtd index of %s\n", partition_name.c_str());
          ret = false;
          break;
        }

        break;
      }
    }
  } while (0);

  if (in_file.is_open()) {
    in_file.close();
  }

  return ret;
}

static bool umount_dir(const string &umount_dir)
{
  bool ret = true;

  if (!umount_dir.empty()) {
    if (umount(umount_dir.c_str())) {
      printf("Umount %s failed.\n", umount_dir.c_str());
      ret = false;
    }
  }

  return ret;
}

bool mount_partition(const string &device_name,
                     const string &mount_dst,
                     const string &fs_type)
{
  bool ret = true;
  string index_str("");
  string fs_type_str;
  string device_path(MTDBLOCK_DEVICE_PATH);

  do {
    if (mount_dst.empty()) {
      printf("mount_dst is \"\".\n");
      ret = false;
      break;
    }

    if (fs_type.empty()) {
      fs_type_str = DEFAULT_MOUNT_FS_TYPE;
    } else {
      fs_type_str = fs_type;
    }

    if (access(mount_dst.c_str(), F_OK)) {
      if (!create_dir(mount_dst.c_str())) {
        printf("failed to create dir %s\n", mount_dst.c_str());
        ret = false;
        break;
      }
    }

    if (!get_partition_index_str(device_name, index_str)) {
      ret = false;
      break;
    }

    if (index_str.empty()) {
      printf("Device partition %s not found.\n", device_name.c_str());
      ret = false;
      break;
    }

    device_path += index_str;
    if (mount(device_path.c_str(), mount_dst.c_str(),
              fs_type_str.c_str(), 0, 0)) {
      printf("Mount %s to %s failed.\n",
             device_path.c_str(), mount_dst.c_str());
      perror("");
      ret = false;
      break;
    }
  }while (0);

  return ret;
}


static bool do_update()
{
  bool ret = true;
  string index_str("");
  string cmd_line;
  string devicepath;
  int32_t status;

  do {
    if (update_kernel) {
      printf("Updating kernel...\n");
      cmd_line = FLASH_ERASE_CMD;
      devicepath = MTD_DEVICE_PATH;
      if (!get_partition_index_str(MTD_PRI, index_str)) {
        printf("grep pri:upgrading can not go on\n");
        ret = false;
        break;
      }

      devicepath += index_str;
      cmd_line = cmd_line + " " + devicepath;
      status = system(cmd_line.c_str());
      if (WEXITSTATUS(status)) {
        printf("%s :failed\n", cmd_line.c_str());
        ret = false;
        break;
      }

      cmd_line.clear();
      cmd_line = UPGRADE_PRI_CMD;
      cmd_line = cmd_line + " " + devicepath + " " + KERNERL_IMG;
      status = system(cmd_line.c_str());
      if (WEXITSTATUS(status)) {
        printf("%s :failed\n", cmd_line.c_str());
        ret = false;
        break;
      }

      printf("Update kernel done\n");
    }

    if (update_system) {
      printf("Updating filesystem...\n");
      cmd_line = FLASH_ERASE_CMD;
      devicepath = MTD_DEVICE_PATH;
      if (!get_partition_index_str(MTD_LNX, index_str)) {
        printf("grep pri:upgrading can not go on\n");
        ret = false;
        break;
      }

      devicepath += index_str;
      cmd_line = cmd_line + " " + devicepath;

      status = system(cmd_line.c_str());
      if (WEXITSTATUS(status)) {
        printf("%s :failed\n", cmd_line.c_str());
        ret = false;
        break;
      }

      cmd_line = UPGRADE_LNX_CMD;
      cmd_line = cmd_line + " " + devicepath + " " + UBIFS_IMG;
      status = system(cmd_line.c_str());
      if (WEXITSTATUS(status)) {
        printf("%s :failed\n", cmd_line.c_str());
        ret = false;
        break;
      }

      printf("Updating filesystem done\n");
    }

  } while (0);

  return ret;
}

static bool set_bois_to_main_sys()
{
  char cmd[BUFFER_SIZE] = { 0 };
  int32_t status;
  bool ret = true;

  sprintf(cmd, "%s \"%s\"", ENTER_MAIN_CMDLINE, MAINSYS_CMD_LINE);
  status = system(cmd);
  if (WEXITSTATUS(status)) {
    printf("%s :failed\n", cmd);
    ret = false;
  }

  return ret;
}

static bool set_upgrade_status(const AM_UPGRADE_STATUS status)
{
  bool ret = true;
  if (config) {
    (*config)[UPGRADE_STATE_NAME].set<int32_t>((int32_t)status);
    if (!config->save()) {
      printf("Failed to save upgrade status.\n");
      ret = false;
    }
  } else {
    printf("m_config is NULL.\n");
    ret = false;
  }

  return ret;
}

static bool clear_partition_swp()
{
  bool ret = true;
  string index_str("");
  int32_t status;

  if (!get_partition_index_str(MTD_SWP, index_str)) {
    printf("Failed to get %s partition index.\n", MTD_SWP);
    ret = false;
  } else {
    printf("clear partition \"swp\" for new main system...\n");
    string cmd_line(FLASH_ERASE_CMD);
    cmd_line = cmd_line + " " + MTD_DEVICE_PATH + index_str;
    status = system(cmd_line.c_str());
    if (WEXITSTATUS(status)) {
      printf("%s :failed\n", cmd_line.c_str());
      ret = false;
    }
  }

  return ret;
}

static bool init_config(const string &file_name)
{
  bool ret = true;

  do {
    if (access(file_name.c_str(), F_OK)) {
      printf("File %s does not exist", file_name.c_str());
      ret = false;
      break;
    }

    config = AMConfig::create(file_name.c_str());
    if (!config) {
      printf("Fail to create AMConfig from config file %s\n", file_name.c_str());
      ret = false;
      break;
    }

  } while (0);

  return ret;
}

static bool get_fw_path(string &fw_path)
{
  bool ret = true;

  do {
    if (!config) {
      printf("lua config is NULL.\n");
      ret = false;
      break;
    }

    fw_path = (*config)[FW_FILE_PATH].get<string>("");
    if (fw_path.empty()) {
      printf("Firmware path is empty.\n");
      ret = false;
      break;
    }

    if (access(fw_path.c_str(), F_OK)) {
      string sub_str;
      uint32_t pos = 0;

      pos = fw_path.find_last_of('/');
      if (pos != string::npos) {
        sub_str = fw_path.substr(pos);
        fw_path = ADC_MOUNT_DIR + sub_str;
      } else {
        sub_str = "/" + fw_path;
        fw_path = ADC_MOUNT_DIR + sub_str;
      }

      if (access(fw_path.c_str(), F_OK)) {
        printf("Not find upgrade firmware file.\n");
        ret = false;
        break;
      }
    }
  } while (0);

  return ret;
}

int32_t main(int32_t argc, char **argv)
{
  int32_t ret = 0;
  string fw_path("");
  string config_file(ADD_MOUNT_DIR);
  string name_list;
  string f_name;
  char kernel_name[STR_LEN] = KERNERL_NAME;
  char filesys_name[STR_LEN] = UBIFS_NAME;
  char k_sign_file[STR_LEN] = {0};
  char f_sign_file[STR_LEN] = {0};
  char public_key[STR_LEN] = {0};
  bool k_sign_exist = false;
  bool f_sign_exist = false;
  bool key_exist = false;
  AMDec7z *dec = NULL;

  do {
    if (access(UCODE_DIR, F_OK) == 0) {
      printf("this app can not run in main system.\n");
      ret = -1;
      break;
    }

    time_t start = time(NULL);
    printf("Starting upgrade in PBA on %s\n", ctime(&start));

    if (!mount_partition(UPGRADE_CFG_PARTITION_NAME,
                         ADD_MOUNT_DIR, DEFAULT_MOUNT_FS_TYPE)) {
      ret = -1;
      break;
    }

    if (!mount_partition(UPGRADE_ADC_PARTITION,
                         ADC_MOUNT_DIR, DEFAULT_MOUNT_FS_TYPE)) {
      ret = -1;
      break;
    }

    config_file = config_file + "/" + UPGRADE_CFG_FILE_NAME;
    if (!init_config(config_file)) {
      ret = -1;
      break;
    }

    if (!get_fw_path(fw_path)) {
      ret = -1;
      break;
    }

    if (!set_upgrade_status(AM_UNPACKING_FW)) {
      ret = -1;
      break;
    }

    dec = AMDec7z::create(fw_path);
    if (!dec) {
      ret = -1;
      break;
    }

    if (!dec->get_filename_in_7z(name_list)) {
      printf("Failed to get 7z content file name.\n");
      ret = -1;
      break;
    }

    if (!name_list.empty()) {
      string tmp_str;
      uint32_t spos = 0, epos = 0;

      snprintf(k_sign_file, STR_LEN - 1, "%s%s", KERNERL_NAME, SIGN_SUFFIX);
      k_sign_file[STR_LEN - 1] = '\0';
      snprintf(f_sign_file, STR_LEN - 1, "%s%s", UBIFS_NAME, SIGN_SUFFIX);
      f_sign_file[STR_LEN - 1] = '\0';

      while ((epos = name_list.find(":", spos)) != string::npos) {
        f_name = name_list.substr(spos, epos - spos);
        if (kernel_name == f_name) {
          snprintf(kernel_name, STR_LEN - 1, "%s%s",
                   UPDATE_PATH, KERNERL_NAME);
          kernel_name[STR_LEN - 1] = '\0';
          update_kernel = true;
        } else if (filesys_name == f_name) {
          snprintf(filesys_name, STR_LEN - 1, "%s%s",
                   UPDATE_PATH, UBIFS_NAME);
          filesys_name[STR_LEN - 1] = '\0';
          update_system = true;
        } else if (k_sign_file == f_name) {
          snprintf(k_sign_file, STR_LEN - 1, "%s%s%s",
                   UPDATE_PATH, KERNERL_NAME, SIGN_SUFFIX);
          k_sign_file[STR_LEN - 1] = '\0';
          k_sign_exist = true;
        } else if (f_sign_file == f_name) {
          snprintf(f_sign_file, STR_LEN - 1, "%s%s%s",
                   UPDATE_PATH, UBIFS_NAME, SIGN_SUFFIX);
          f_sign_file[STR_LEN - 1] = '\0';
          f_sign_exist = true;
        } else {
          if (f_name.size() > 9) {
            spos = f_name.size() - 10;
            tmp_str = f_name.substr(spos);
            if (tmp_str == PUBLIC_KEY_SUFFIX) {
              snprintf(public_key, STR_LEN - 1, "%s%s",
                       UPDATE_PATH, f_name.c_str());
              public_key[STR_LEN - 1] = '\0';
              key_exist = true;
            } else {
              printf("find unrecognized file name: %s\n", f_name.c_str());
            }
          } else {
            printf("find unrecognized file name: %s\n", f_name.c_str());
          }
        }
        spos = epos + 1;
      }
    }

    if (!key_exist ||
        (!(update_kernel && k_sign_exist) &&
            !(update_system && f_sign_exist))) {
      printf("updating files are incomplete !\n");
      set_upgrade_status(AM_FW_INVALID);
      ret = -1;
      break;
    }

    printf("Extracting 7z file...\n");
    if (!dec->dec7z(UPDATE_PATH)) {
      printf("Failed to extract 7z file.\n");
      set_upgrade_status(AM_UNPACK_FW_FAIL);
      ret = -1;
      break;
    }

    dec->destroy();
    dec = NULL;

    if (!set_upgrade_status(AM_UNPACK_FW_DONE)) {
      ret = -1;
      break;
    }

    if (update_kernel) {
      printf("Verifying kernel signature ...\n");
      if (verify_signature(kernel_name,
                           k_sign_file,
                           public_key,
                           SHA_TYPE_SHA256) != SU_ECODE_OK) {
        printf("Kernel signature is not consistent!\n");
        ret = -1;
        break;
      }
    }

    if (update_system) {
      printf("Verifying filesystem signature ...\n");
      if (verify_signature(filesys_name,
                           f_sign_file,
                           public_key,
                           SHA_TYPE_SHA256) != SU_ECODE_OK) {
        printf("Filesystem signature is not consistent!\n");
        ret = -1;
        break;
      }
    }

    if (!set_upgrade_status(AM_WRITEING_FW_TO_FLASH)) {
      ret = -1;
      break;
    }

    printf("Begin to upgrade\n");
    if (!do_update()) {
      set_upgrade_status(AM_WRITE_FW_TO_FLASH_FAIL);
      ret = -1;
      break;
    }

    printf("update success!\n");

    if (!set_upgrade_status(AM_WRITE_FW_TO_FLASH_DONE)) {
      ret = -1;
      break;
    }

    clear_partition_swp();
    if (!set_upgrade_status(AM_INIT_MAIN_SYS)) {
      ret = -1;
      break;
    }
  } while (0);

  if(set_bois_to_main_sys() && !ret) {
    set_upgrade_status(AM_UPGRADE_SUCCESS);
    printf("Everything is OKey!\n");
  }

  if (dec) {
    dec->destroy();
    dec = NULL;
  }

  if (config) {
    delete config;
    config = NULL;
  }

  umount_dir(ADC_MOUNT_DIR);
  umount_dir(ADD_MOUNT_DIR);
  sync();
  reboot(RB_AUTOBOOT);

  return ret;
}
