/*******************************************************************************
 * pba_upgrade.cpp
 *
 * History:
 *   2015-1-13 - [longli] created file
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
#include "am_libarchive_if.h"

#if defined(SECURE_BOOT)
#include "cryptography_if.h"
#include "cryptochip_library_if.h"
#endif

#define BUFFER_SIZE             256
#define STR_LEN                 128
#define UPDATE_PATH             "/dev/update_img/"
#define UCODE_DIR               "/lib/firmware/"
#define MTD_DEVICE_PATH         "/dev/mtd"
#define SD_DEVICE_PATH          "/dev/mmcblk0"
#define FLASH_ERASE_CMD         "/usr/sbin/flash_eraseall"
#define MTD_BLD                 "bld"
#define MTD_PRI                 "pri"
#define MTD_LNX                 "lnx"
#define MTD_SWP                 "swp"
#define UPGRADE_DTS_CMD         "/usr/sbin/upgrade_partition -d"
#define UPGRADE_BLD_CMD         "/usr/sbin/upgrade_partition -p -G"
#define UPGRADE_PRI_CMD         "/usr/sbin/upgrade_partition -p -K"
#define UPGRADE_LNX_CMD         "/usr/sbin/upgrade_partition -p -W -F 1"
#define ENTER_MAIN_CMDLINE      "/usr/sbin/upgrade_partition -S 0 -C"
#define SET_UPGRADE_FLAG        "/usr/sbin/upgrade_partition -P"
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

static AMConfig *config = nullptr;
static bool update_dts = false;
static bool update_bld = false;
static bool update_kernel = false;
static bool update_system = false;
static char bld_fw_path[STR_LEN] = BLD_NAME;
static char dts_dtb_path[STR_LEN] = {0};
static char kernel_fw_path[STR_LEN] = KERNEL_NAME;
static char filesys_fw_path[STR_LEN] = UBIFS_NAME;
/* device id just for add and adc */
static char dev_id[2][4] = {"251", "250"};
static int32_t dev_seq = 0;

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

/* mount sdcard with fs type "vfat" or "exfat" */
static bool mount_sdcard(const string &mount_dir)
{
  bool ret = false;
  DIR *dirp;
  struct dirent *direntp;
  string sd_sys_dir("/sys/class/mmc_host/mmc0/");
  string dev_node("");
  string cmdline("/bin/mount");

  do {
    if ((dirp = opendir(sd_sys_dir.c_str())) == nullptr) {
      ERROR("Check SD card existence error\n");
      break;
    }

    while ((direntp = readdir(dirp)) != nullptr) {
      /* the folder name is like ubi1_0 after ubimkvol, so just check "_" */
      if ( !strncmp(direntp->d_name, "mmc0", 4)) {
        int32_t status = 0;
        sd_sys_dir = sd_sys_dir + direntp->d_name + "/block/mmcblk0/mmcblk0p1";
        if (!access(sd_sys_dir.c_str(), F_OK)) {
          if (access("/dev/mmcblk0p1", F_OK)) {
            status = system("/bin/mknod /dev/mmcblk0p1 b 179 1");
            if (WEXITSTATUS(status)) {
              ERROR("mknod mmcblk0p1 failed\n");
              break;
            }
          }
          dev_node = "/dev/mmcblk0p1";
        } else {
          if (access("/dev/mmcblk0", F_OK)) {
            status = system("/bin/mknod /dev/mmcblk0 b 179 0");
            if (WEXITSTATUS(status)) {
              ERROR("mknod mmcblk0 failed\n");
              break;
            }
          }
          dev_node = "/dev/mmcblk0";
        }

        if (access(mount_dir.c_str(), F_OK)) {
          if (!create_dir(mount_dir.c_str())) {
            printf("failed to create dir %s\n", mount_dir.c_str());
            break;
          }
        }

        if (mount(dev_node.c_str(), mount_dir.c_str(), "vfat", 0, 0)) {
          /* try to mount with exfat */
          printf("Try to mount sdcard with exfat...\n");
          cmdline = cmdline + " -t exfat " + dev_node + " " + mount_dir;
          status = system(cmdline.c_str());
          if (WEXITSTATUS(status)) {
            ERROR("Mount %s to %s failed.\n",
                  dev_node.c_str(), mount_dir.c_str());
            break;
          }
        }

        ret = true;
        break;
      }
    }

    closedir(dirp);
  } while (0);

  return ret;
}

static bool get_partition_index_str(const string &partition_name,
                                    string &index_str)
{
  bool ret = false;
  string line_str;
  string p_name;
  ifstream in_file;

  do {
    if (partition_name.empty()) {
      printf("partition_name is NULL\n");
      break;
    }

    in_file.open(PROC_MTD_FILE, ios::in);
    if (!in_file.is_open()) {
      printf("Failed to open %s\n", PROC_MTD_FILE);
      break;
    }

    p_name = "\"" + partition_name + "\"";

    while (!in_file.eof()) {
      getline(in_file, line_str);
      if (line_str.find(p_name) != string::npos) {
        size_t s_pos;
        size_t e_pos;

        s_pos = line_str.find("mtd");
        if (s_pos == string::npos) {
          printf("Can not find \"mtd\" in %s\n", line_str.c_str());
          break;
        }

        e_pos = line_str.find_first_of(':', s_pos);
        if (e_pos == string::npos) {
          printf("Can not find \":\" in %s\n", line_str.c_str());
          break;
        }

        s_pos += 3;
        index_str = line_str.substr(s_pos, e_pos - s_pos);
        if (index_str.empty()) {
          printf("Not find mtd index of %s\n", partition_name.c_str());
          ret = false;
          break;
        }

        ret = true;
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

void get_ubi_node(const string &index_str, string &ubi_node)
{
  char file_path[STR_LEN] = {0};
  string line_str;
  ifstream in_file;
  DIR *dirp;
  struct dirent *direntp;

  ubi_node.clear();

  do {
    if ((dirp = opendir("/sys/class/ubi/")) == nullptr) {
      printf("Failed to open folder /sys/class/ubi/.\n");
      break;
    }

    while ((direntp = readdir(dirp)) != nullptr) {
      /* the folder name is like ubi1_0 after ubimkvol, so just check "_" */
      if ( !strncmp(direntp->d_name, "ubi", 3) && !strchr(direntp->d_name, '_')) {
        snprintf(file_path, STR_LEN - 1,"/sys/class/ubi/%s/mtd_num", direntp->d_name);
        in_file.open(file_path, ios::in);
        if (!in_file.is_open()) {
          printf("Failed to open %s\n", file_path);
          break;
        }

        if (!in_file.eof()) {
          getline(in_file, line_str);
          if (line_str == index_str) {
            ubi_node = direntp->d_name;
            in_file.close();
            break;
          }
        }
        in_file.close();
      }
    }
    closedir(dirp);
  } while (0);
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

    if (fs_type_str == UBIFS_FS_TYPE) {
      int32_t status = 0;
      string cmd_line("");
      string ubi_nod("");
      string ubi_nod_path("");
      string sys_ubi_nod_path("");

      if (access("/dev/ubi_ctrl", F_OK)) {
#ifdef CONFIG_ARCH_S5
        status = system("/bin/mknod /dev/ubi_ctrl c 10 59");
#else
        status = system("/bin/mknod /dev/ubi_ctrl c 10 60");
#endif
        if (WEXITSTATUS(status)) {
          NOTICE("mknod ubi_ctrl failed\n");
          ret = false;
          break;
        }
      }

      get_ubi_node(index_str, ubi_nod);
      if (ubi_nod.empty()) {
        cmd_line = "/usr/sbin/ubiattach /dev/ubi_ctrl -m " + index_str;
        status = system(cmd_line.c_str());
        if (WEXITSTATUS(status)) {
          printf("%s :failed\n", cmd_line.c_str());
          break;
        }

        get_ubi_node(index_str, ubi_nod);
        if (ubi_nod.empty()) {
          printf("failed to ubiattach to %s partition\n", device_name.c_str());
          break;
        }
      }

      sys_ubi_nod_path = "/sys/class/ubi/" + ubi_nod +"_0";
      if (access(sys_ubi_nod_path.c_str(), F_OK)) {
        ubi_nod_path = "/dev/"+ ubi_nod;
        if (access(ubi_nod_path.c_str(), F_OK)) {
          cmd_line.clear();
          cmd_line = "/bin/mknod " + ubi_nod_path + " c " + dev_id[dev_seq] + " 0";
          status = system(cmd_line.c_str());
          if (WEXITSTATUS(status)) {
            printf("%s :failed\n", cmd_line.c_str());
            break;
          }
        }

        cmd_line.clear();
        cmd_line = "/usr/sbin/ubimkvol "+ ubi_nod_path + " -N " + device_name + " -m";
        status = system(cmd_line.c_str());
        if (WEXITSTATUS(status)) {
          printf("%s :failed\n", cmd_line.c_str());
          break;
        }
      }

      device_path.clear();
      device_path = "/dev/"+ ubi_nod + "_0";

      if (access(device_path.c_str(), F_OK)) {
        cmd_line.clear();
        cmd_line = "/bin/mknod " + device_path + " c " + dev_id[dev_seq] + " 1";
        if (++dev_seq > 1) dev_seq = 0;
        status = system(cmd_line.c_str());
        if (WEXITSTATUS(status)) {
          NOTICE("%s :failed\n", cmd_line.c_str());
          ret = false;
          break;
        }
      }
    }

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
    if (update_bld) {
      printf("Updating bld...\n");
      cmd_line = FLASH_ERASE_CMD;
      devicepath = MTD_DEVICE_PATH;
      if (!get_partition_index_str(MTD_BLD, index_str)) {
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
      cmd_line = UPGRADE_BLD_CMD;
      cmd_line = cmd_line + " " + devicepath + " " + bld_fw_path;
      status = system(cmd_line.c_str());
      if (WEXITSTATUS(status)) {
        printf("%s :failed\n", cmd_line.c_str());
        ret = false;
        break;
      }

      printf("Update bld done!\n");
    }

    if (update_dts) {
      printf("Updating dts...\n");

      cmd_line.clear();
      cmd_line = UPGRADE_DTS_CMD;
      cmd_line = cmd_line + " " + dts_dtb_path;
      status = system(cmd_line.c_str());
      if (WEXITSTATUS(status)) {
        printf("%s :failed\n", cmd_line.c_str());
        ret = false;
        break;
      }

      printf("Update dts done!\n");
    }

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
      cmd_line = cmd_line + " " + devicepath + " " + kernel_fw_path;
      status = system(cmd_line.c_str());
      if (WEXITSTATUS(status)) {
        printf("%s :failed\n", cmd_line.c_str());
        ret = false;
        break;
      }

      printf("Update kernel done!\n");
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
      cmd_line = cmd_line + " " + devicepath + " " + filesys_fw_path;
      status = system(cmd_line.c_str());
      if (WEXITSTATUS(status)) {
        printf("%s :failed\n", cmd_line.c_str());
        ret = false;
        break;
      }

      printf("Updating filesystem done!\n");
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
    printf("%s partition not found. skip!\n", MTD_SWP);
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

static bool get_use_sdcard_flag(uint32_t &use_sdcard)
{
  bool ret = true;

  do {
    if (!config) {
      printf("lua config is NULL.\n");
      ret = false;
      break;
    }

    if ((*config)[USE_SDCARD].exists()) {
      use_sdcard = (*config)[USE_SDCARD].get<int32_t>(0);
    } else {
      printf("config use %s does not exist.\n", USE_SDCARD);
      ret = false;
    }

  } while (0);

  return ret;
}

static bool get_fw_path(const bool use_sdcard, string &fw_path)
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
      size_t pos = 0;

      pos = fw_path.find_last_of('/');
      if (pos != string::npos) {
        sub_str = fw_path.substr(pos);
      } else {
        sub_str = "/" + fw_path;
      }

      if (use_sdcard) {
        fw_path = SD_MOUNT_DIR + sub_str;
      } else {
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

#if defined(SECURE_BOOT)
static int32_t verify_rsapubkey_hwsignature(sf_cryptochip_interface *chip_interface, void* crypto_handle, char *rsa_pubkey)
{
  struct pubkey_s {
      unsigned char rsa_key_n[256 + 4];
      unsigned char rsa_key_e[16];
  };

  pubkey_s pubkey;
  char read_string[512] = {0};
  char *pstring = nullptr;
  int32_t copy_length = 0;
  int32_t ret = 0;
  unsigned char digest[32] = {0};

  if (!chip_interface || !crypto_handle || !rsa_pubkey) {
    printf("NULL chip_interface %p, or NULL crypto_handle %p, or NULL rsa_pubkey %p\n", chip_interface, crypto_handle, rsa_pubkey);
    return (-1);
  }

  FILE *file = fopen(rsa_pubkey, "rt");
  if (file) {
    printf("open rsa pubkey file (%s) fail\n", rsa_pubkey);
    return (-2);
  }

  pstring = fgets(&read_string[0], sizeof(read_string), file);
  if (!pstring) {
    printf("bad rsa pubkey file content\n");
    ret = (-3);
    goto __verify_rsapubkey_hwsignature_exit;
  }
  if (strncmp(pstring, "N = ", strlen("N = "))) {
    printf("bad rsa pubkey file content\n");
    ret = (-4);
    goto __verify_rsapubkey_hwsignature_exit;
  }
  pstring += strlen("N = ");
  copy_length = strlen(pstring);
  while ((1 < copy_length) && (('\r' == pstring[copy_length -1]) || ('\n' == pstring[copy_length -1]))) {
    copy_length --;
  }
  memcpy(pubkey.rsa_key_n, pstring, copy_length);

  memset(read_string, 0, sizeof(read_string));
  pstring = fgets(&read_string[0], sizeof(read_string), file);
  if (!pstring) {
    printf("bad rsa pubkey file content\n");
    ret = (-5);
    goto __verify_rsapubkey_hwsignature_exit;
  }
  if (strncmp(pstring, "E = ", strlen("E = "))) {
    printf("bad rsa pubkey file content\n");
    ret = (-6);
    goto __verify_rsapubkey_hwsignature_exit;
  }
  pstring += strlen("E = ");
  copy_length = strlen(pstring);
  while ((1 < copy_length) && (('\r' == pstring[copy_length -1]) || ('\n' == pstring[copy_length -1]))) {
    copy_length --;
  }
  memcpy(pubkey.rsa_key_e, pstring, copy_length);

  digest_sha256((const unsigned char *)&pubkey, 256 + 4 + 16, digest);

  ret = chip_interface->f_verify_hwsign(crypto_handle, digest, 13);
  if (ret) {
    printf("rsa key not match with hw signature in cryptochip\n");
    ret = (-10);
  }

  __verify_rsapubkey_hwsignature_exit:
  if (file) {
    fclose(file);
    file = nullptr;
  }

  return ret;
}

static int32_t update_kernel_hwsignature(sf_cryptochip_interface *chip_interface, void* crypto_handle, char *kernel_file)
{
  int32_t ret = 0;
  unsigned char *p_kernel = nullptr;
  unsigned char digest[32] = {0};

  if (!chip_interface || !crypto_handle || !kernel_file) {
    printf("NULL chip_interface %p, or NULL crypto_handle %p, or NULL kernel_file %p\n", chip_interface, crypto_handle, kernel_file);
    return (-1);
  }

  FILE *file = fopen(kernel_file, "rb");
  int32_t filesize = 0;
  if (file) {
    printf("open kernel_file (%s) fail\n", kernel_file);
    return (-2);
  }

  fseek(file, 0x0, SEEK_END);
  filesize = ftell(file);
  fseek(file, 0x0, SEEK_SET);

  p_kernel = (unsigned char *) malloc(filesize);
  if (!p_kernel) {
    printf("no memory, request size %d\n", filesize);
    ret = (-3);
    goto __update_kernel_hwsignature_exit;
  }
  fread(p_kernel, 1, filesize, file);

  digest_sha256(p_kernel, filesize, digest);

  ret = chip_interface->f_gen_hwsign(crypto_handle, p_kernel, 12);
  if (ret) {
    printf("generate kernel's hw signature fail\n");
    ret = (-10);
  }

  __update_kernel_hwsignature_exit:
  if (file) {
    fclose(file);
    file = nullptr;
  }

  if (p_kernel) {
    free(p_kernel);
    p_kernel = nullptr;
  }

  return ret;
}
#endif

int32_t main(int32_t argc, char **argv)
{
  int32_t ret = 0;
  uint32_t use_sd = 0;
  string fw_path("");
  string config_file(ADD_MOUNT_DIR);
  string name_list;
  string f_name;
  AMIArchive *dec = nullptr;
  char d_sign_file[STR_LEN] = {0};
  char b_sign_file[STR_LEN] = {0};
  char k_sign_file[STR_LEN] = {0};
  char f_sign_file[STR_LEN] = {0};
  char public_key[STR_LEN] = {0};
  bool d_sign_exist = false;
  bool b_sign_exist = false;
  bool k_sign_exist = false;
  bool f_sign_exist = false;
  bool key_exist = false;

#if defined(SECURE_BOOT)
  sf_cryptochip_interface cryptochip_interface;
  void *cryptochip_handle = nullptr;
  ret = get_cryptochip_library_interface(&cryptochip_interface, ECRYPTOCHIP_TYPE_ATMEL_ATSHA204);
  if (ret) {
    printf("get cryptochip library interface fail\n");
    return ret;
  }
  cryptochip_handle = cryptochip_interface.f_create_handle();
  if (!cryptochip_handle) {
    printf("cryptochip create handle fail\n");
    return (-3);
  }
#endif

  do {
    if (access(UCODE_DIR, F_OK) == 0) {
      printf("this app can not run in main system.\n");
      ret = -1;
      break;
    }

    time_t start = time(nullptr);
    printf("Starting upgrade in PBA on %s\n", ctime(&start));

    if (!mount_partition(UPGRADE_CFG_PARTITION_NAME,
                         ADD_MOUNT_DIR, MOUNT_FS_TYPE)) {
      ret = -1;
      break;
    }

    config_file = config_file + "/" + UPGRADE_CFG_FILE_NAME;
    if (!init_config(config_file)) {
      ret = -1;
      break;
    }

    if(!get_use_sdcard_flag(use_sd)) {
      ret = -1;
      break;
    }

    if (use_sd){
      if (!mount_sdcard(SD_MOUNT_DIR)) {
        ret = -1;
        break;
      }
    } else {
      if (!mount_partition(UPGRADE_ADC_PARTITION,
                           ADC_MOUNT_DIR, MOUNT_FS_TYPE)) {
        ret = -1;
        break;
      }
    }

    if (!get_fw_path(use_sd, fw_path)) {
      ret = -1;
      break;
    }

    if (!set_upgrade_status(AM_UNPACKING_FW)) {
      ret = -1;
      break;
    }

    printf("Checking needed files ...\n");
    dec = AMIArchive::create();
    if (!dec) {
      ERROR("Failed to create AMIArchive instance.");
      ret = false;
      break;
    }

    if (!dec->get_file_list(fw_path, name_list)) {
      printf("Failed to get file names in archive %s.\n",
             fw_path.c_str());
      ret = -1;
      break;
    }

    if (!name_list.empty()) {
      string tmp_str, pub_str;
      size_t pos = 0, spos = 0, epos = 0;

      snprintf(d_sign_file, STR_LEN - 1, "%s%s", DTS_FILE_SUFFIX, SIGN_SUFFIX);
      d_sign_file[STR_LEN - 1] = '\0';
      snprintf(b_sign_file, STR_LEN - 1, "%s%s", BLD_NAME, SIGN_SUFFIX);
      b_sign_file[STR_LEN - 1] = '\0';
      snprintf(k_sign_file, STR_LEN - 1, "%s%s", KERNEL_NAME, SIGN_SUFFIX);
      k_sign_file[STR_LEN - 1] = '\0';
      snprintf(f_sign_file, STR_LEN - 1, "%s%s", UBIFS_NAME, SIGN_SUFFIX);
      f_sign_file[STR_LEN - 1] = '\0';

      while ((epos = name_list.find(",", spos)) != string::npos) {
        tmp_str = name_list.substr(spos, epos - spos);

        pos = tmp_str.find_last_of('/');
        if (pos != string::npos) {
          f_name = tmp_str.substr(pos + 1);
        } else {
          f_name = tmp_str;
        }
        if (bld_fw_path == f_name) {
          snprintf(bld_fw_path, STR_LEN - 1, "%s%s",
                   UPDATE_PATH, tmp_str.c_str());
          bld_fw_path[STR_LEN - 1] = '\0';
          update_bld = true;
        } else if (kernel_fw_path == f_name) {
          snprintf(kernel_fw_path, STR_LEN - 1, "%s%s",
                   UPDATE_PATH, tmp_str.c_str());
          kernel_fw_path[STR_LEN - 1] = '\0';
          update_kernel = true;
        } else if (filesys_fw_path == f_name) {
          snprintf(filesys_fw_path, STR_LEN - 1, "%s%s",
                   UPDATE_PATH, tmp_str.c_str());
          filesys_fw_path[STR_LEN - 1] = '\0';
          update_system = true;
        } else if (b_sign_file == f_name){
          snprintf(b_sign_file, STR_LEN - 1, "%s%s",
                   UPDATE_PATH, tmp_str.c_str());
          b_sign_file[STR_LEN - 1] = '\0';
          b_sign_exist = true;
        }else if (k_sign_file == f_name) {
          snprintf(k_sign_file, STR_LEN - 1, "%s%s",
                   UPDATE_PATH, tmp_str.c_str());
          k_sign_file[STR_LEN - 1] = '\0';
          k_sign_exist = true;
        } else if (f_sign_file == f_name) {
          snprintf(f_sign_file, STR_LEN - 1, "%s%s",
                   UPDATE_PATH, tmp_str.c_str());
          f_sign_file[STR_LEN - 1] = '\0';
          f_sign_exist = true;
        } else if (f_name.find(d_sign_file) != string::npos){
          snprintf(d_sign_file, STR_LEN - 1, "%s%s",
                   UPDATE_PATH, tmp_str.c_str());
          d_sign_file[STR_LEN - 1] = '\0';
          d_sign_exist = true;
        } else if (f_name.find(DTS_FILE_SUFFIX) != string::npos) {
          snprintf(dts_dtb_path, STR_LEN - 1, "%s%s",
                   UPDATE_PATH, tmp_str.c_str());
          dts_dtb_path[STR_LEN - 1] = '\0';
          update_dts = true;
        } else {
          if (f_name.size() > 9) {
            spos = f_name.size() - 10;
            pub_str = f_name.substr(spos);
            if (pub_str == PUBLIC_KEY_SUFFIX) {
              snprintf(public_key, STR_LEN - 1, "%s%s",
                       UPDATE_PATH, tmp_str.c_str());
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
         !(update_system && f_sign_exist) &&
         !(update_bld && b_sign_exist) &&
         !(update_dts && d_sign_exist))) {
      printf("updating files are incomplete !\n");
      set_upgrade_status(AM_FW_INVALID);
      ret = -1;
      break;
    }

    printf("Extracting upgrade file...\n");
    if (!dec->decompress(fw_path, UPDATE_PATH)) {
      printf("Failed to extract 7z file.\n");
      set_upgrade_status(AM_UNPACK_FW_FAIL);
      ret = -1;
      break;
    }

    if (!set_upgrade_status(AM_UNPACK_FW_DONE)) {
      ret = -1;
      break;
    }

#if defined(SECURE_BOOT)
    ret = verify_rsapubkey_hwsignature(&cryptochip_interface, cryptochip_handle, public_key);
    if (ret) {
      printf("rsa public key not match with crypto chip's hardware signature\n");
      return ret;
    }
#endif
    if (update_dts) {
      printf("Verifying dts signature ...\n");
      if (verify_signature(dts_dtb_path,
                           d_sign_file,
                           public_key,
                           SHA_TYPE_SHA256) != SU_ECODE_OK) {
        printf("dts signature is not consistent!\n");
        ret = -1;
        break;
      }
    }

    if (update_bld) {
      printf("Verifying bld signature ...\n");
      if (verify_signature(bld_fw_path,
                           b_sign_file,
                           public_key,
                           SHA_TYPE_SHA256) != SU_ECODE_OK) {
        printf("bld signature is not consistent!\n");
        ret = -1;
        break;
      }
    }

    if (update_kernel) {
      printf("Verifying kernel signature ...\n");
      if (verify_signature(kernel_fw_path,
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
      if (verify_signature(filesys_fw_path,
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

#if defined(SECURE_BOOT)
    ret = update_kernel_hwsignature(&cryptochip_interface, cryptochip_handle, kernel_fw_path);
    if (ret) {
      printf("update_kernel_hwsignature fail, ret %d\n", ret);
    } else {
      printf("Everything is OKey!\n");
    }
#else
    printf("Everything is OKey!\n");
#endif

  }

  delete dec;
  delete config;

  if (use_sd) {
    umount_dir(SD_MOUNT_DIR);
  } else {
    umount_dir(ADC_MOUNT_DIR);
  }
  umount_dir(ADD_MOUNT_DIR);
  sync();
  system("/sbin/reboot");

  return ret;
}
