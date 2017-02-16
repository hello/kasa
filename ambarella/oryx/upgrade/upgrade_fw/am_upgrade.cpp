/*******************************************************************************
 * am_upgrade.cpp
 *
 * History:
 *   2015-1-8 - [longli] created file
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
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <sys/reboot.h>
#include <mutex>
#include <string>
#include <fstream>
#include <thread>
#include "am_log.h"
#include "am_upgrade_if.h"
#include "am_upgrade.h"

#include "pba_upgrade.h"
#include "am_dec7z.h"

using namespace std;

static mutex m_mtx_i;
static mutex m_mtx;
#define  DECLARE_MUTEX_I  lock_guard<mutex> lck_i(m_mtx_i);
#define  DECLARE_MUTEX  lock_guard<mutex> lck(m_mtx);

#define BUFF_LEN                4096
#define LOW_SPEED_LIMIT         5
#define LOW_SPEED_TIMEOUT       5
#define SHOW_DOWNLOAD_PROGRESS  true
#define PROC_MOUNTS_FILE        "/proc/mounts"
#define DEFAULT_CONFIG_CONTENT  \
  ("abc={\nfilepath=\"/dev/adc/CloudCamUpgrade.7z\",\n"\
  "status=0,\n}\n return abc\n")
#define PBA_CMD_LINE \
  ("console=ttyS0 rootfs=ramfs root=/dev/ram rw "\
  "rdinit=/linuxrc mem=220M dsp=0xC0000000,0x00000000 "\
  "bsb=0xC0000000,0x00000000")

AMFWUpgrade *AMFWUpgrade::m_instance = NULL;

AMFWUpgrade::AMFWUpgrade():
  m_fw_path(""),
  m_adc_dir(ADC_MOUNT_DIR),
  m_ref_counter(0),
  m_mode(AM_MODE_NOT_SET),
  m_state(AM_NOT_KNOWN),
  m_config(NULL),
  m_updl(NULL),
  m_init(false),
  m_dl_ready(false),
  m_mounted(false),
  m_in_progress(false)
{
}

AMFWUpgrade::~AMFWUpgrade()
{
  NOTICE("~AMFWUpgrade is called.");
  if (m_updl) {
    m_updl->destroy();
    m_updl = NULL;
  }

  if (m_config) {
    delete m_config;
    m_config = NULL;
  }

  if (m_mounted) {
    umount_dir(ADD_MOUNT_DIR);
    m_mounted = false;
  }

  m_instance = NULL;
}

AMIFWUpgradePtr AMIFWUpgrade::get_instance()
{
  return ((AMIFWUpgrade*)AMFWUpgrade::get_instance());
}

AMFWUpgrade *AMFWUpgrade::get_instance()
{
  DECLARE_MUTEX_I;
  INFO("AMFWUpgrade::get_instance() is called.");

  if (!m_instance) {
    m_instance = new AMFWUpgrade();
    if (!m_instance) {
      ERROR("AMFWUpgrade::get_instance() init failed.");
    }
  }

  return m_instance;
}

void AMFWUpgrade::inc_ref()
{
  ++m_ref_counter;
}

void AMFWUpgrade::release()
{
  DECLARE_MUTEX_I;
  DECLARE_MUTEX;
  INFO("AMFWUpgrade release is called.");
  if ((m_ref_counter >= 0) && (--m_ref_counter <= 0)) {
    NOTICE("This is the last reference of AMFWUpgrade's object, "
        "delete object instance 0x%p", m_instance);
    delete m_instance;
    m_instance = NULL;
  }
}

bool AMFWUpgrade::init()
{
  bool ret = true;

  INFO("Enter AMFWUpgrade::init...");
  if (!m_init) {
    if (!init_config_file(UPGRADE_CFG_PARTITION_NAME,
                          ADD_MOUNT_DIR,
                          DEFAULT_MOUNT_FS_TYPE,
                          UPGRADE_CFG_FILE_NAME)) {
      ERROR("AMFWUpgrade::init() fail.");
      ret = false;
    }
    m_init = ret;
  }

  return ret;
}

bool AMFWUpgrade::init_config_file(const string &parti_name,
                                   const string &dst_dir,
                                   const string &fs_type,
                                   const string &filename)
{
  bool ret = true;
  AM_MOUNT_STATUS mount_status = AM_NOT_MOUNTED;
  string config_file(dst_dir);

  INFO("AMFWUpgrade::init_config_file() is called.");
  do {
    mount_status = m_mount_partition(parti_name,
                                     dst_dir,
                                     fs_type);
    if (mount_status == AM_MOUNTED) {
      m_mounted = true;
    } else if (mount_status != AM_ALREADY_MOUNTED) {
      ERROR("Failed to mount partition %s to %s",
            parti_name.c_str(), dst_dir.c_str());
      ret = false;
      break;
    }

    config_file = config_file + "/" + filename;
    if (access(config_file.c_str(), F_OK)) {
      ofstream out_file;
      out_file.open(config_file.c_str(), ios::out);
      if (!out_file.is_open()) {
        ERROR("Failed to create file %s", config_file.c_str());
        ret = false;
        break;
      }
      string content(DEFAULT_CONFIG_CONTENT);
      out_file.write(content.c_str(), content.size());
      out_file.close();
    }

    if (m_config) {
      delete m_config;
      m_config = NULL;
    }

    m_config = AMConfig::create(config_file.c_str());
    if (!m_config) {
      ERROR("AMFWUpgrade::save_fw_path: fail to create from lua config file\n");
      ret = false;
      break;
    }

    if ((m_config->serialize()).empty()) {
      ERROR("AMFWUpgrade::save_fw_path: fail to serialize lua table\n");
      ret = false;
      delete m_config;
      m_config = NULL;
      break;
    }
  } while (0);

  return ret;
}

bool AMFWUpgrade::set_upgrade_status(const AM_UPGRADE_STATUS state)
{
  bool ret = true;

  INFO("AMFWUpgrade::set_upgrade_status %d", state);
  if (state != m_state) {
    if (m_config) {
      m_state = state;
      (*m_config)[UPGRADE_STATE_NAME].set<int32_t>((int32_t)m_state);
      if (!m_config->save()) {
        ERROR("Failed to save upgrade config.");
        ret = false;
      }
    } else {
      ERROR("m_config is NULL.");
      ret = false;
    }
  }

  return ret;
}

bool AMFWUpgrade::set_mode(AM_UPGRADE_MODE mode)
{
  DECLARE_MUTEX;
  bool ret = true;

  do {
    INFO("AMFWUpgrade::set_mode %d", mode);
    if (!init()) {
      ret = false;
      break;
    }

    set_upgrade_status(AM_UPGRADE_PREPARE);
    switch (mode) {
      case AM_DOWNLOAD_ONLY:
      case AM_DOWNLOAD_AND_UPGRADE:
        if (!download_init()) {
          ret = false;
        }
        break;
      case AM_UPGRADE_ONLY:
        //do nothing
        break;
      default:
        ret = false;
        ERROR("AMFWUpgrade unknown mode %d", mode);
        break;
    }
  } while (0);

  if (ret) {
    m_mode = mode;
  } else {
    set_upgrade_status(AM_UPGRADE_PREPARE_FAIL);
  }

  return ret;
}

bool AMFWUpgrade::set_fw_url(const string &fw_url, const string &fw_save_path)
{
  DECLARE_MUTEX;
  bool ret = true;
  struct stat buf;
  string fw_path(fw_save_path);

  INFO("fw_path=%s save_path=%s", fw_url.c_str(), fw_save_path.c_str());
  do {
    if (!init()) {
      ret = false;
      break;
    }

    set_upgrade_status(AM_UPGRADE_PREPARE);
    if (m_updl && m_mode != AM_UPGRADE_ONLY) {
      if (fw_url.empty()) {
        ERROR("fw_url is empty.");
        ret = false;
        break;
      }

      if (!fw_save_path.empty()) {
        if (stat(fw_save_path.c_str(), &buf)) {
          if (errno != ENOENT) {
            ERROR("stat error");
            ret = false;
            break;
          }
        }
        if (S_ISDIR(buf.st_mode)) {
          if (fw_save_path.find_last_of('/') != (fw_save_path.size() - 1)) {
            fw_path += "/";
          }
        }
      }

      /* set dst_file according to src_file */
      if (fw_save_path.empty() || S_ISDIR(buf.st_mode)) {
        uint32_t pos = fw_url.find_last_of('/');
        if (pos != string::npos) {
          if (pos != (fw_url.size() - 1)) {
            fw_path += fw_url.substr(pos + 1);
          } else {
            pos = fw_url.find_last_not_of('/');
            if (pos != string::npos) {
              uint32_t s_pos = fw_url.find_last_of('/', pos);
              if (s_pos != string::npos) {
                fw_path += fw_url.substr(s_pos + 1, pos - s_pos);
              } else {
                fw_path += fw_url.substr(0, pos + 1);
              }
            } else {
              ERROR("Invalid fw_url.");
              ret = false;
              break;
            }
          }
        } else {
          fw_path += fw_url;
        }
      }

      if (stat(fw_path.c_str(), &buf)) {
        if (errno != ENOENT) {
          NOTICE("stat error");
          ret = false;
          break;
        }
      } else {
        if (S_ISDIR(buf.st_mode)) {
          NOTICE("%s is a directory and cannot be a firmware dst_save_file",
                 fw_path.c_str());
          ret = false;
          break;
        }
      }

      if (m_updl->set_url(fw_url, fw_path)) {
        ret = save_fw_path(fw_path);
        if (ret) {
          m_dl_ready = true;
        }
      } else {
        ret = false;
      }
    } else {
      ret = false;
      ERROR("AM_UPGRADE_ONLY mode is not support to set_fw_url()");
    }
  } while (0);

  if (!ret) {
    set_upgrade_status(AM_UPGRADE_PREPARE_FAIL);
  }

  return ret;
}

bool AMFWUpgrade::set_fw_path(const string &fw_path)
{
  DECLARE_MUTEX;
  bool ret = true;

  INFO("fw_path=%s", fw_path.c_str());
  do {
    if (!init()) {
      ret = false;
      break;
    }

    set_upgrade_status(AM_UPGRADE_PREPARE);
    ret = save_fw_path(fw_path);
  } while (0);

  if (!ret) {
    set_upgrade_status(AM_UPGRADE_PREPARE_FAIL);
  }

  return ret;
}

void AMFWUpgrade::remove_previous_fw(const string &fw_path)
{
  string last_fw_path;
  string last_fw_name;
  string fw_name;

  //remove previous image -- only check adc partition
  last_fw_path = (*m_config)[FW_FILE_PATH].get<string>("");
  if (!last_fw_path.empty()) {
    uint32_t pos = 0;
    pos = last_fw_path.find_last_of("/");
    if (pos != string::npos && (pos != (last_fw_path.size() - 1))) {
      last_fw_name = last_fw_path.substr(pos + 1);
    } else if (pos == string::npos) {
      last_fw_name = last_fw_path;
    }
  }

  if (!fw_path.empty()) {
    uint32_t pos = 0;
    pos = fw_path.find_last_of("/");
    if (pos != string::npos && (pos != (fw_path.size() - 1))) {
      fw_name = fw_path.substr(pos + 1);
    } else if (pos == string::npos) {
      fw_name = fw_path;
    }
  }

  if (!last_fw_name.empty() && !m_adc_dir.empty()) {
    last_fw_path = m_adc_dir + last_fw_name;

    if (fw_name != last_fw_name && !access(last_fw_path.c_str(), F_OK)) {
      printf("removing previous old firmware ...\n");
      remove(last_fw_path.c_str());
    }
  }
}

bool AMFWUpgrade::save_fw_path(const string &fw_path)
{


  bool ret = true;

  INFO("fw_path=%s", fw_path.c_str());
  do {
    if (fw_path.empty()) {
      ERROR("fw_path is empty.");
      ret = false;
      break;
    }
    remove_previous_fw(fw_path);
    m_fw_path = fw_path;
    (*m_config)[FW_FILE_PATH].set<string>(fw_path);
    m_config->save();

  } while (0);

  return ret;
}

bool AMFWUpgrade::set_dl_user_passwd(const string &user, const string &passwd)
{
  DECLARE_MUTEX;
  bool ret = true;

  INFO("username=%s passwd=%s", user.c_str(), passwd.c_str());
  do {
    if (!init()) {
      ret = false;
      break;
    }

    set_upgrade_status(AM_UPGRADE_PREPARE);
    if (m_updl && m_mode != AM_UPGRADE_ONLY) {
      if (!m_updl->set_dl_user_passwd(user, passwd)) {
        ret = false;
      }
    } else {
      ret = false;
      ERROR("AM_UPGRADE_ONLY mode is not support to set_dl_user_passwd()");
    }
  } while (0);

  if (!ret) {
    set_upgrade_status(AM_UPGRADE_PREPARE_FAIL);
  }

  return ret;
}

bool AMFWUpgrade::set_dl_connect_timeout(uint32_t timeout)
{
  DECLARE_MUTEX;
  bool ret = true;

  INFO("timeout=%d", timeout);
  do {
    if (!init()) {
      ret = false;
      break;
    }

    set_upgrade_status(AM_UPGRADE_PREPARE);
    if (m_updl && m_mode != AM_UPGRADE_ONLY) {
      if (!m_updl->set_dl_connect_timeout(timeout)) {
        ret = false;
      }
    } else {
      ret = false;
      ERROR("AM_UPGRADE_ONLY mode is not support to set_dl_connect_timeout()");
    }
  } while (0);

  if (!ret) {
    set_upgrade_status(AM_UPGRADE_PREPARE_FAIL);
  }

  return ret;
}

bool AMFWUpgrade::set_upgrade_cmd()
{
  bool ret = true;
  string cmd_line("");

  INFO("AMFWUpgrade::set_upgrade_cmd() is called.");
  cmd_line = cmd_line + ENTER_PBA_CMDLINE + " \"" + PBA_CMD_LINE + "\"";
  int32_t status = system(cmd_line.c_str());
  if (WEXITSTATUS(status)) {
    NOTICE("%s :failed\n", cmd_line.c_str());
    ret = false;
  }

  return ret;
}

bool AMFWUpgrade::check_dl_file()
{
  bool ret = true;

  do {
    if (!m_fw_path.empty()) {
      string files_list;

      AMDec7z *dec = AMDec7z::create(m_fw_path);
      if (!dec) {
        struct stat statbuff;

        ret = false;
        if (stat(m_fw_path.c_str(), &statbuff) < 0) {
          printf("get %s stat error.", m_fw_path.c_str());
          break;
        } else {
          if (statbuff.st_size < 4096) {
            ifstream in_file;
            string line_str;
            uint32_t s_pos = string::npos;
            uint32_t e_pos = string::npos;

            in_file.open(m_fw_path.c_str(), ios::in);
            if (!in_file.is_open()) {
              ERROR("Failed to open %s", m_fw_path.c_str());
              break;
            }

            while (getline(in_file, line_str)) {
              if (s_pos == string::npos) {
                s_pos = line_str.find("<title>");
                if (s_pos != string::npos) {
                  s_pos = s_pos + 7;
                  e_pos = line_str.find("</title>", s_pos);
                  if (e_pos != string::npos) {
                    printf("download error: %s\n",
                           line_str.substr(s_pos , e_pos - s_pos).c_str());
                    break;
                  } else {
                    printf("download error: %s",
                           line_str.substr(s_pos).c_str());
                  }
                }
              } else if (e_pos == string::npos){
                e_pos = line_str.find("</title>", 0);
                if (e_pos == string::npos) {
                  printf("%s", line_str.c_str());
                } else {
                  printf("%s\n", line_str.substr(0, e_pos).c_str());
                  break;
                }
              } else {
                break;
              }
            }

            in_file.close();
          }
        }
        break;
      }

      dec->get_filename_in_7z(files_list);
      if (files_list.empty() ||
          (files_list.find(KERNERL_NAME) == string::npos &&
              files_list.find(UBIFS_NAME) == string::npos)) {
        printf("Upgrade firmware invalid. No '%s' or '%s' found.\n",
               KERNERL_NAME, UBIFS_NAME);
        ret = false;
      }

      dec->destroy();
    }
  } while (0);

  return ret;
}

bool AMFWUpgrade::copy_file(const string &src_file, const string &dst_file)
{
  bool ret = true;
  ifstream in_file;
  ofstream out_file;
  char buff[BUFF_LEN];
  int32_t count = 0;
  int64_t total_size = 1;
  int64_t load_size = 0;
  int32_t percent = 0;

  INFO("copy_file %s to %s", src_file.c_str(), dst_file.c_str());
  do {
    in_file.open(src_file.c_str(), ios::binary);
    if (!in_file.is_open()) {
      ERROR("Failed to open %s", src_file.c_str());
      ret = false;
      break;
    }

    out_file.open(dst_file.c_str(), ios::binary|ios::trunc|ios::out);
    if (!out_file.is_open()) {
      in_file.close();
      ERROR("Failed to open %s", dst_file.c_str());
      ret = false;
      break;
    }

    in_file.seekg(0, in_file.end);
    total_size = in_file.tellg();
    if (total_size <= 0) {
      ERROR("Failed to get file size: %s", src_file.c_str());
      ret = false;
      break;
    }
    in_file.seekg(0, in_file.beg);

    while (!in_file.eof()) {
      in_file.read(buff, sizeof(buff));
      count = in_file.gcount();
      out_file.write(buff, count);
      load_size += count;
      percent = ((load_size * 100) / total_size);
      printf("\rloading ... %d%%\t", percent);
      if (load_size > 1024) {
        printf("%dkB done.", (int32_t)(load_size/1024));
      } else {
        printf("%dB done.", (int32_t)load_size);
      }
    }
    printf("\n");

    in_file.close();
    out_file.close();

  } while (0);

  return ret;
}

bool AMFWUpgrade::copy_fw_to_ADC()
{
  bool ret = true;
  string fw_path;
  string file_name;
  string adc_path;

  do {
    if (m_fw_path.empty()) {
      fw_path = (*m_config)[FW_FILE_PATH].get<string>("");
      if (fw_path.empty()) {
        ret = false;
        ERROR("Firmware path is not set before.");
        break;
      }
      //remove
      remove_previous_fw(fw_path);
    } else {
      fw_path = m_fw_path;
    }
    if (!access(fw_path.c_str(), F_OK)) {
      string sub_str;
      uint32_t pos = 0;

      pos = fw_path.find_last_of('/');
      if (pos != string::npos) {
        sub_str = fw_path.substr(pos);
        adc_path = m_adc_dir + sub_str;
      } else {
        sub_str = "/" + fw_path;
        adc_path = m_adc_dir + sub_str;
      }

      if (fw_path != adc_path) {
        if (!copy_file(fw_path, adc_path)) {
          ret = false;
          break;
        }
      }
    } else {
      ret = false;
      ERROR("Not find upgrade firmware file.");
    }
  } while (0);

  return ret;
}

bool AMFWUpgrade::upgrade()
{
  DECLARE_MUTEX;
  bool ret = true;

  INFO("AMFWUpgrade::upgrade() is called.");
  do {
    if (!init()) {
      ret = false;
      break;
    }

    switch (m_mode) {
      case AM_DOWNLOAD_ONLY:
        if (m_dl_ready) {
          set_upgrade_status(AM_DOWNLOADING_FW);
          if (!m_updl->download()) {
            ret = false;
            set_upgrade_status(AM_DOWNLOAD_FW_FAIL);
            PRINTF("Download upgrade firmware failed.");
          } else {
            if (check_dl_file()) {
              PRINTF("Download upgrade firmware successfully.");
              set_upgrade_status(AM_DOWNLOAD_FW_COMPLETE);
            } else {
              ret = false;
              set_upgrade_status(AM_DOWNLOAD_FW_FAIL);
              PRINTF("Download upgrade firmware failed.");
            }
          }
        } else {
          ret = false;
          ERROR("Upgrade firmware path is not set, current mode is %d",
                m_mode);
        }
        break;
      case AM_UPGRADE_ONLY:
        if (copy_fw_to_ADC()) {
          if (!set_upgrade_cmd()) {
            ret = false;
            set_upgrade_status(AM_INIT_PBA_SYS_FAIL);
            ERROR("set_upgrade_cmd failed.");
          } else {
            set_upgrade_status(AM_INIT_PBA_SYS_DONE);
          }
        } else {
          ret = false;
          set_upgrade_status(AM_DOWNLOAD_FW_FAIL);
        }
        break;
      case AM_DOWNLOAD_AND_UPGRADE:
        if (m_dl_ready) {
          set_upgrade_status(AM_DOWNLOADING_FW);
          if (m_updl->download()) {
            if (check_dl_file()) {
              PRINTF("Download upgrade firmware successfully.");
              set_upgrade_status(AM_DOWNLOAD_FW_COMPLETE);
              if (!set_upgrade_cmd()) {
                ret = false;
                set_upgrade_status(AM_INIT_PBA_SYS_FAIL);
                ERROR("set_upgrade_cmd failed.");
              } else {
                set_upgrade_status(AM_INIT_PBA_SYS_DONE);
              }
            } else {
              ret = false;
              set_upgrade_status(AM_DOWNLOAD_FW_FAIL);
              PRINTF("Download upgrade firmware failed.");
            }

          } else {
            ret = false;
            set_upgrade_status(AM_DOWNLOAD_FW_FAIL);
            PRINTF("Download upgrade firmware failed.");
          }
        } else {
          ret = false;
          ERROR("Upgrade firmware path is not set, current mode is %d",
                m_mode);
        }
        break;
      default:
        ret = false;
        ERROR("No such upgrade mode, please set upgrade mode and try again.");
        break;
    }
  } while (0);

  return ret;
}

void AMFWUpgrade::run_upgrade_thread(void *paras, uint32_t paras_size)
{
  if (paras_size != sizeof(AMUpgradeArgs)) {
    ERROR("Invalid upgrade parameters.");
  } else {
    INFO("Begin upgrade thread.");
    if (!m_in_progress) {
      m_in_progress = true;
      thread upgrade_th(upgrade_thread, paras);
      upgrade_th.detach();
    } else {
      PRINTF("Upgrade is in progress, please try again later!");
    }
  }

}

bool AMFWUpgrade::download_init()
{
  bool ret = true;

  INFO("AMFWUpgrade::download_init() is called.");
  if (!m_updl) {
    do {
      m_updl = AMDownload::create();
      if (!m_updl) {
        ret = false;
        break;
      }

      m_updl->set_low_speed_limit(LOW_SPEED_LIMIT, LOW_SPEED_TIMEOUT);
      m_updl->set_show_progress(SHOW_DOWNLOAD_PROGRESS);
    } while (0);
  }

  return ret;
}

bool AMFWUpgrade::create_dir(const string &dir_path)
{
  bool ret = true;
  int32_t res;

  do {
    if (dir_path.empty()) {
      NOTICE("Failed to create dir: \"%s\" is empty.", dir_path.c_str());
      ret = false;
      break;
    }
    res = mkdir(dir_path.c_str(), 0777) == 0 ? 0 : errno;
    if (!res || res == EEXIST) {
      ret = true;
    } else {
      ret = false;
    }
    break;
  } while (0);

  return ret;
}

bool AMFWUpgrade::get_partition_index_str(const string &partition_name,
                                          string &index_str)
{
  bool ret = true;
  string line_str;
  string p_name;
  ifstream in_file;

  INFO("AMFWUpgrade::get_partition_index_str() is called.");
  do {
    if (partition_name.empty()) {
      NOTICE("partition_name is NULL");
      ret = false;
      break;
    }

    in_file.open(PROC_MTD_FILE, ios::in);
    if (!in_file.is_open()) {
      NOTICE("Failed to open %s", PROC_MTD_FILE);
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
          NOTICE("Can not find \"mtd\" in %s", line_str.c_str());
          ret = false;
          break;
        }

        e_pos = line_str.find_first_of(':', s_pos);
        if (e_pos == string::npos) {
          NOTICE("Can not find \":\" in %s", line_str.c_str());
          ret = false;
          break;
        }

        s_pos += 3;
        index_str = line_str.substr(s_pos, e_pos - s_pos);
        if (index_str.empty()) {
          NOTICE("Not find mtd index of %s", partition_name.c_str());
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

AM_MOUNT_STATUS AMFWUpgrade::is_mounted(const string &device_path,
                                        const string &mount_dst)
{
  AM_MOUNT_STATUS ret = AM_NOT_MOUNTED;
  bool last_is_slash = false;
  int32_t last_char = 0;
  string line_str;
  string dst_dir("");
  ifstream in_file;

  INFO("device_path=%s mount_dst=%s", device_path.c_str(), mount_dst.c_str());
  do {
    if (mount_dst.empty()) {
      NOTICE("partition_name is NULL.");
      ret = AM_UNKNOWN;
      break;
    }

    last_char = mount_dst.length() - 1;
    //remove duplicated slash
    for(int i = 0; i < last_char; ++i) {
      if(mount_dst[i] != '/') {
        dst_dir += mount_dst[i];
        last_is_slash = false;
      } else {
        if (!last_is_slash) {
          dst_dir += mount_dst[i];
          last_is_slash = true;
        }
      }
    }

    /* if the last character is '/' */
    if (mount_dst[last_char] != '/' || 0 == last_char) {
      dst_dir += mount_dst[last_char];
    }

    dst_dir += " ";
    in_file.open(PROC_MOUNTS_FILE, ios::in);
    if (!in_file.is_open()) {
      NOTICE("Failed to open %s", PROC_MOUNTS_FILE);
      ret = AM_UNKNOWN;
      break;
    }

    while (!in_file.eof()) {
      getline(in_file, line_str);
      if (line_str.find(dst_dir) != string::npos) {
        if (device_path.empty() || line_str.find(device_path) == string::npos) {
          ret = AM_MOUNTED_OTHER;
          break;
        } else {
          ret = AM_ALREADY_MOUNTED;
          break;
        }
      }
    }
  } while (0);

  if (in_file.is_open()) {
    in_file.close();
  }

  return ret;
}

bool AMFWUpgrade::umount_dir(const string &umount_dir)
{
  bool ret = true;

  INFO("AMFWUpgrade::umount_dir(%s) is called.", umount_dir.c_str());
  if (!umount_dir.empty() && is_mounted("", umount_dir) > 0) {
    if (umount(umount_dir.c_str())) {
      NOTICE("Umount %s failed.", umount_dir.c_str());
      ret = false;
    }
  }

  return ret;
}

bool AMFWUpgrade::is_fs_supported(const std::string &fs_type)
{
  bool ret = false;
  string line_str;
  ifstream in_file;

  do {
    if (fs_type.empty()) {
      NOTICE("fs_type is empty.");
      break;
    }

    in_file.open(PROC_FILESYSTEMS, ios::in);
    if (!in_file.is_open()) {
      NOTICE("Failed to open %s", PROC_FILESYSTEMS);
      break;
    }

    while (!in_file.eof()) {
      getline(in_file, line_str);
      if (line_str.find(fs_type) != string::npos) {
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

AM_MOUNT_STATUS AMFWUpgrade::mount_partition(const string &device_name,
                                             const string &mount_dst,
                                             const string &fs_type)
{
  DECLARE_MUTEX;
  AM_MOUNT_STATUS ret = AM_NOT_MOUNTED;

  INFO("device_name=%s mount_dst=%s fs_type=%s",
       device_name.c_str(), mount_dst.c_str(), fs_type.c_str());
  set_upgrade_status(AM_UPGRADE_PREPARE);
  ret = m_mount_partition(device_name, mount_dst, fs_type);

  //record adc partition mount dir
  if (ret == AM_MOUNTED || ret == AM_ALREADY_MOUNTED) {
    if (device_name == UPGRADE_ADC_PARTITION) {
      uint32_t pos = mount_dst.find_last_not_of("/");
      if (pos == string::npos) {
        m_adc_dir = "/";
      } else {
        m_adc_dir = mount_dst.substr(0, pos + 1) + "/";
      }
    }
  }

  return ret;
}

AM_UPGRADE_STATUS AMFWUpgrade::get_upgrade_status()
{
  DECLARE_MUTEX_I;
  AM_UPGRADE_STATUS state = AM_NOT_KNOWN;

  INFO("AMFWUpgrade::get_upgrade_status() is called.");
  if (m_state != AM_NOT_KNOWN) {
    state = m_state;
  } else {
    if (init() && m_config) {
      AMConfig& cfg = *m_config;
      state = (AM_UPGRADE_STATUS)(cfg[UPGRADE_STATE_NAME].get<int32_t>(0));
    }
  }

  return state;
}

AM_MOUNT_STATUS AMFWUpgrade::m_mount_partition(const string &device_name,
                                               const string &mount_dst,
                                               const string &fs_type)
{
  AM_MOUNT_STATUS ret = AM_NOT_MOUNTED;
  string index_str("");
  string fs_type_str;
  string device_path(MTDBLOCK_DEVICE_PATH);

  INFO("AMFWUpgrade::m_mount_partition() is called.");
  do {
    if (mount_dst.empty()) {
      NOTICE("mount_dst is \"\".");
      break;
    }

    if (fs_type.empty()) {
      fs_type_str = DEFAULT_MOUNT_FS_TYPE;
    } else {
      fs_type_str = fs_type;
    }

    if (!is_fs_supported(fs_type_str)) {
      PRINTF("Filesystem_type(%s) is not supported.", fs_type_str.c_str());
      break;
    }

    if (access(mount_dst.c_str(), F_OK)) {
      if (!create_dir(mount_dst)) {
        break;
      }
    }

    if (!get_partition_index_str(device_name, index_str)) {
      break;
    }

    if (index_str.empty()) {
      PRINTF("Device partition %s not found.", device_name.c_str());
      break;
    }

    device_path += index_str;
    ret = is_mounted(device_path, mount_dst);
    if (ret == AM_MOUNTED_OTHER || ret == AM_UNKNOWN) {
      PRINTF("%s is already mounted to another device or check mount error",
             mount_dst.c_str());
      break;
    }

    if (ret == AM_ALREADY_MOUNTED) {
      NOTICE("%s is already mounted to %s",
             device_name.c_str(), mount_dst.c_str());
      break;
    } else { /* ret == AM_NOT_MOUNTED */
      if (mount(device_path.c_str(), mount_dst.c_str(),
                fs_type_str.c_str(), 0, 0)) {
        PRINTF("Failed mount %s to %s.",
               device_path.c_str(), mount_dst.c_str());
        PERROR("Mount failed.");
        break;
      } else {
        ret = AM_MOUNTED;
      }
    }
  }while (0);

  return ret;
}

bool AMFWUpgrade::is_in_progress()
{
  DECLARE_MUTEX_I;
  return m_in_progress;
}

void AMFWUpgrade::upgrade_thread(void *upgrade_args)
{
  AM_MOUNT_STATUS mounted_status = AM_NOT_MOUNTED;
  AM_UPGRADE_MODE mode = AM_DOWNLOAD_AND_UPGRADE;
  string device_name(UPGRADE_ADC_PARTITION);
  string dst_dir(ADC_MOUNT_DIR);
  string mount_fs_type(DEFAULT_MOUNT_FS_TYPE);
  int32_t array_len = 0;
  AMUpgradeArgs upgrade_paras;
  AMFWUpgrade *upgrade_ptr = NULL;

  do {
    memcpy(&upgrade_paras, upgrade_args, sizeof(upgrade_paras));
    if (upgrade_paras.upgrade_mode < AM_DOWNLOAD_ONLY ||
        upgrade_paras.upgrade_mode > AM_DOWNLOAD_AND_UPGRADE) {
      ERROR("Upgrade mode is invalid.\n");
      break;
    }

    upgrade_ptr = AMFWUpgrade::get_instance();
    if (!upgrade_ptr) {
      ERROR("AMIFWUpgrade::get_instance() failed.");
      break;
    }

    upgrade_ptr->inc_ref();
    mode = (AM_UPGRADE_MODE)(upgrade_paras.upgrade_mode);
    if (!upgrade_ptr->set_mode(mode)) {
      ERROR("Initializing mode %d failed.\n", upgrade_paras.upgrade_mode);
      break;
    }

    printf("Mount %s to %s ...\n", device_name.c_str(), dst_dir.c_str());
    mounted_status = upgrade_ptr->mount_partition(device_name,
                                                  dst_dir,
                                                  mount_fs_type);
    if (mounted_status != AM_MOUNTED &&
        mounted_status != AM_ALREADY_MOUNTED) {
      ERROR("Mount %s to %s failed!\n",
            device_name.c_str(), dst_dir.c_str());
      break;
    }

    array_len = sizeof(upgrade_paras.path_to_upgrade_file);
    upgrade_paras.path_to_upgrade_file[array_len - 1] = '\0';
    if (mode == AM_UPGRADE_ONLY) {
      if (upgrade_paras.path_to_upgrade_file[0]) {
        printf("Setting upgrade firmware path ...\n");
        if (!upgrade_ptr->set_fw_path(upgrade_paras.path_to_upgrade_file)) {
          ERROR("Setting upgrade firmware path to %s failed.\n",
                upgrade_paras.path_to_upgrade_file);
          break;
        }
      }
    } else {
      printf("Setting upgrade source file path to %s\n",
             upgrade_paras.path_to_upgrade_file);
      if (!upgrade_ptr->set_fw_url(upgrade_paras.path_to_upgrade_file,
                                   dst_dir)) {
        ERROR("Setting upgrade source file url to %s failed.\n",
              upgrade_paras.path_to_upgrade_file);
        break;
      }

      if (upgrade_paras.user_name[0]) {
        array_len = sizeof(upgrade_paras.user_name);
        upgrade_paras.user_name[array_len - 1] = '\0';
        upgrade_paras.passwd[array_len - 1] = '\0';
        printf("Setting authentication ...\n");
        INFO("Setting authentication:%s:%s\n",
             upgrade_paras.user_name, upgrade_paras.passwd);
        if (!upgrade_ptr->set_dl_user_passwd(upgrade_paras.user_name,
                                             upgrade_paras.passwd)) {
          ERROR("Setting authentication (%s) failed.\n",
                upgrade_paras.user_name);
          break;
        }
      }

      if (upgrade_paras.timeout) {
        printf("Setting connecting timeout ...\n");
        if (!upgrade_ptr->set_dl_connect_timeout(upgrade_paras.timeout)) {
          ERROR("Setting connecting timeout to %u second(s) failed.\n",
                upgrade_paras.timeout);
          break;
        }
      }
    }

    if (mode != AM_UPGRADE_ONLY) {
      printf("Begin to download firmware ...\n");
    } else {
      printf("Begin to load firmware ...\n");
    }

    if (!upgrade_ptr->upgrade()) {
      if (mode == AM_UPGRADE_ONLY) {
        PRINTF("Upgrade firmware failed.\n");
      }
      break;
    }

    if (mounted_status == AM_MOUNTED) {
      printf("Umount %s\n", dst_dir.c_str());
      if (!upgrade_ptr->umount_dir(dst_dir)) {
        ERROR("Umount %s failed.\n", dst_dir.c_str());
        break;
      }
      mounted_status = AM_NOT_MOUNTED;
    }

    if (mode != AM_DOWNLOAD_ONLY) {
      PRINTF("Rebooting to complete upgrade ...\n");
      sync();
      reboot(RB_AUTOBOOT);
    }
  } while (0);

  if (upgrade_ptr) {
    if (mounted_status == AM_MOUNTED) {
      upgrade_ptr->umount_dir(dst_dir);
    }

    upgrade_ptr->m_in_progress = false;
    upgrade_ptr->release();
  }
}
