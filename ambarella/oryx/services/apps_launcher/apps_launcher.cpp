/*******************************************************************************
 * apps_launcher.cpp
 *
 * History:
 *   2014-9-28 - [lysun] created file
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
#include "am_define.h"
#include "am_signal.h"
#include "am_log.h"
#include "am_file.h"

#include "am_pid_lock.h"
#include "commands/am_service_impl.h"
#include "am_api_proxy.h"
#include "am_service_manager.h"
#include "am_configure.h"
#include "apps_launcher.h"
#include "am_watchdog.h"
#include "am_config_set.h"

#include <fstream>
#include <sys/types.h>
#include <dirent.h>
#include <sys/mount.h>

#define DEFAULT_APPS_CONF_FILE  "/etc/oryx/apps/apps_launcher.acs"
#define FIRST_RUN "/etc/oryx/apps/first_run"
#define PROC_MTD  "/proc/mtd"
#define PROC_MOUNTS_FILE  "/proc/mounts"
#define MTDBLOCK_DEVICE_PATH  "/dev/mtdblock"
#define DEFAULT_MOUNT_FS_TYPE "ubifs"
#ifdef BUILD_AMBARELLA_ORYX_UPGRADE_MTD_MOUNT_FS_TYPE
#define MOUNT_FS_TYPE ((const char*)BUILD_AMBARELLA_ORYX_UPGRADE_MTD_MOUNT_FS_TYPE)
#else
#define MOUNT_FS_TYPE DEFAULT_MOUNT_FS_TYPE
#endif

#define PARTITION_NAME  "add"
#define MOUNT_DIR "/dev/add"
#define ORYX_PATH "/etc/oryx"
#define ORYX_BASE "oryx_base.tar.xz"
#define PATH_LEN 128

using namespace std;

AMAppsLauncher *AMAppsLauncher::m_instance = nullptr;
static AMWatchdogService* gWdInstance = nullptr;
static bool g_service_running = false;

static void sigstop(int arg)
{
  g_service_running = false;
  INFO("Apps_Launcher receives signal %d", arg);
}


AMAppsLauncher::AMAppsLauncher()
{
}

AMAppsLauncher::~AMAppsLauncher()
{
  m_instance = nullptr;
}

void AMAppsLauncher::destroy()
{
  delete this;
}

const AMServiceAttrList& AMAppsLauncher::get_service_list()
{
  return m_service_list;
}

AMAppsLauncher *AMAppsLauncher::get_instance()
{
  if (!m_instance) {
    m_instance = new AMAppsLauncher();
  }
  return m_instance;
}

int AMAppsLauncher::load_config()
{

  return 0;
}

void AMAppsLauncher::get_ubi_node(const string &index_str, string &ubi_node)
{
  char file_path[PATH_LEN] = {0};
  string line_str;
  ifstream in_file;
  DIR *dirp;
  struct dirent *direntp;

  do {
    ubi_node.clear();

    if ((dirp = opendir("/sys/class/ubi/")) == nullptr) {
      ERROR("Failed to open folder /sys/class/ubi/.");
      break;
    }

    while ((direntp = readdir(dirp)) != nullptr) {
      /* the folder name is like ubi1_0 after ubimkvol, so just check "_" */
      if ( !strncmp(direntp->d_name, "ubi", 3) && !strchr(direntp->d_name, '_')) {
        snprintf(file_path, PATH_LEN - 1,"/sys/class/ubi/%s/mtd_num", direntp->d_name);
        in_file.open(file_path, ios::in);
        if (!in_file.is_open()) {
          ERROR("Failed to open %s", file_path);
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

bool AMAppsLauncher::get_partition_index_str(const string &partition_name,
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

    in_file.open(PROC_MTD, ios::in);
    if (!in_file.is_open()) {
      NOTICE("Failed to open %s", PROC_MTD);
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

    in_file.close();
  } while (0);

  return ret;
}


bool AMAppsLauncher::is_mount(const string &mount_dst)
{
  bool ret = false;
  bool last_is_slash = false;
  int32_t len = 0;
  ifstream in_file;
  string mount_dir(mount_dst);
  string line_str;
  string::iterator it;

  for (it = mount_dir.begin(); it != mount_dir.end(); ++it) {
    if (*it == '/') {
      if (last_is_slash) {
        mount_dir.erase(it);
      } else {
        last_is_slash = true;
      }
    } else {
      last_is_slash = false;
    }
  }

  len = mount_dir.length();
  if (len > 1 && mount_dir[len - 1] == '/') {
    mount_dir = mount_dir.substr(0, len - 1);
  }

  in_file.open(PROC_MOUNTS_FILE, ios::in);
  if (!in_file.is_open()) {
    NOTICE("Failed to open %s", PROC_MOUNTS_FILE);
    ret = true;
  } else {
    while (!in_file.eof()) {
      getline(in_file, line_str);
      if (line_str.find(mount_dir) != string::npos) {
        ret = true;
        break;
      }
    }
    in_file.close();
  }

  return ret;
}

bool AMAppsLauncher::umount_dir(const string &mount_dst)
{
  bool ret = true;

  if (umount(mount_dst.c_str())) {
    PRINTF("Failed to umount %s\n", mount_dst.c_str());
    ret = false;
  }

  return ret;
}

bool AMAppsLauncher::mount_partition(const string &device_name,
                                     const string &mount_dst,
                                     const string &fs_type)
{
  string index_str("");
  string fs_type_str;
  string device_path(MTDBLOCK_DEVICE_PATH);
  bool ret = false;
  bool mount_flag = false;

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

    if (!get_partition_index_str(device_name, index_str)) {
      break;
    }

    if (index_str.empty()) {
      PRINTF("Device partition %s not found.", device_name.c_str());
      break;
    }

    device_path += index_str;

    if (access(mount_dst.c_str(), F_OK)) {
      if (!AMFile::create_path(mount_dst.c_str())) {
        PRINTF("Failed to create dir %s\n", mount_dst.c_str());
        break;
      }
    } else {
      mount_flag = is_mount(mount_dst);
      if (mount_flag) {
        if (umount(mount_dst.c_str())) {
          PRINTF("Failed to umount %s\n", mount_dst.c_str());
          break;
        }
      }
    }

    if (fs_type_str == "ubifs") {
      int32_t status = 0;
      string cmd_line("");
      string ubi_nod("");
      string ubi_nod_path("");

      if (access("/dev/ubi_ctrl", F_OK)) {
        ERROR("mount ubifs: /dev/ubi_ctrl not found!");
        break;
      }

      get_ubi_node(index_str, ubi_nod);
      if (ubi_nod.empty()) {
        cmd_line = cmd_line + "ubiattach /dev/ubi_ctrl -m " + index_str;
        status = system(cmd_line.c_str());
        if (WEXITSTATUS(status)) {
          ERROR("%s :failed!", cmd_line.c_str());
          break;
        }

        get_ubi_node(index_str, ubi_nod);
        if (ubi_nod.empty()) {
          ERROR("failed to ubiattach to %s partition", device_name.c_str());
          break;
        }
      }

      device_path.clear();
      device_path = "/dev/"+ ubi_nod + "_0";
      if (access(device_path.c_str(), F_OK)) {
        ubi_nod_path = "/dev/"+ ubi_nod;
        cmd_line.clear();
        cmd_line = "ubimkvol "+ ubi_nod_path + " -N " + device_name + " -m";
        status = system(cmd_line.c_str());
        if (WEXITSTATUS(status)) {
          ERROR("%s :failed!", cmd_line.c_str());
          break;
        }
      }
    }

    if (mount(device_path.c_str(), mount_dst.c_str(),
              fs_type_str.c_str(), 0, 0)) {
      PRINTF("Failed mount %s to %s, mount fs_type: %s.",
             device_path.c_str(), mount_dst.c_str(), fs_type_str.c_str());
      PERROR("Mount failed.");
      break;
    } else {
      ret = true;
    }
  }while (0);

  return ret;
}

bool AMAppsLauncher::check_partition_exist(string partition_name)
{
  bool ret = false;
  std::ifstream in_file;
  std::string line_str;

  do {
    if (partition_name.empty()) {
      NOTICE("partition_name is NULL");
      break;
    }

    in_file.open(PROC_MTD, std::ios::in);
    if (!in_file.is_open()) {
      NOTICE("Failed to open %s", PROC_MTD);
      break;
    }

    while (!in_file.eof()) {
      std::getline(in_file, line_str);
      if (line_str.rfind(partition_name) != std::string::npos) {
        ret = true;
        break;
      }
    }

    in_file.close();

  } while (0);

  return ret;
}

bool AMAppsLauncher::touch_file(const string filename)
{
  bool ret = true;
  ofstream out_file;

  out_file.open(filename, ios::out);
  if (!out_file.is_open()) {
    NOTICE("Failed to open %s", filename.c_str());
    ret = false;
  } else {
    out_file.close();
  }

  return ret;
}

bool AMAppsLauncher::restore_config(const std::string &config_name,
                                    const std::string &config_path,
                                    const std::string &extract_path)
{
  bool ret = true;
  AMConfigSet *cfg_set = nullptr;

  do {
    cfg_set = AMConfigSet::create(config_path);
    if (!cfg_set) {
      ERROR("Failed to create AMConfigSet instance.");
      ret = false;
      break;
    }

    if (!cfg_set->extract_config(config_name, extract_path)) {
      ERROR("Failed to extract %s to %s.",
            config_name.c_str(), extract_path.c_str());
      ret = false;
      break;
    }

  } while (0);

  delete cfg_set;
  cfg_set = nullptr;

  return ret;
}

bool AMAppsLauncher::backup_config(const std::string &config_name,
                                   const std::string &config_list,
                                   const std::string &path_base,
                                   const std::string &store_path)
{
  bool ret = true;
  AMConfigSet *cfg_set = nullptr;

  do {
    cfg_set = AMConfigSet::create(store_path);
    if (!cfg_set) {
      ERROR("Failed to create AMConfigSet instance.");
      ret = false;
      break;
    }

    if (!cfg_set->create_config(config_name, config_list, path_base)) {
      ERROR("Failed to create %s.", config_name.c_str());
      ret = false;
      break;
    }
  } while (0);

  delete cfg_set;
  cfg_set = nullptr;

  return ret;
}

bool AMAppsLauncher::check_config(const string &config_path)
{
  bool ret = true;
  DIR *dirp;
  struct dirent *direntp;
  struct stat buf;
  string t_path;
  string c_path;
  vector<string> dir_list;
  int32_t len = 0;

  do {
    if (config_path.empty()) break;

    if (stat(config_path.c_str(), &buf) < 0) {
      PRINTF("stat %s error.!", config_path.c_str());
      ret = false;
      break;
    } else {
      if (S_ISDIR(buf.st_mode)) {
        dir_list.push_back(config_path);
      } else {
        ERROR("%s is not a folder.", config_path.c_str());
        break;
      }
    }

    while (!dir_list.empty()) {
      c_path = dir_list.back();
      dir_list.pop_back();
      INFO("folder path=%s\n", c_path.c_str());
      if ((dirp = opendir(c_path.c_str())) == nullptr) {
        ERROR("Failed to open folder %s.", c_path.c_str());
        ret = false;
        break;
      }
      while ((direntp = readdir(dirp)) != nullptr) {
        /* the folder name is like ubi1_0 after ubimkvol, so just check "_" */
        if (strcmp(direntp->d_name, ".") && strcmp(direntp->d_name, "..")){
          t_path = c_path + "/" + direntp->d_name;
          INFO("file/path=%s\n", t_path.c_str());
          if (stat(t_path.c_str(), &buf) < 0) {
            PRINTF("get %s stat error.", t_path.c_str());
            ret = false;
            break;
          }

          if (S_ISDIR(buf.st_mode)) {
            dir_list.push_back(t_path);
          } else if (S_ISREG(buf.st_mode)){
            len = strlen(direntp->d_name);
            if (len > 4 && !strcmp(&(direntp->d_name[len - 4]), ".acs")) {
              if (buf.st_size < 10) {
                ret = false;
                break;
              }
            }
          }
        }
      }
      closedir(dirp);
      if (!ret) break;
    }
  } while (0);

  dir_list.clear();

  return ret;
}

bool AMAppsLauncher::init_prepare()
{
  bool ret = true;
  bool mount_f = false;

  do {
    if (check_partition_exist(PARTITION_NAME)) {
      if (!access(FIRST_RUN, F_OK)) {
        if (!check_config(ORYX_PATH)) {
          PRINTF("Oryx configures are incomplete, "
              "begin to restore oryx configures to original...\n");
          mount_f = mount_partition(PARTITION_NAME, MOUNT_DIR,
                                    MOUNT_FS_TYPE);
          if (mount_f) {
            if (!restore_config(ORYX_BASE, MOUNT_DIR,"/etc")) {
              ret = false;
              break;
            }
            PRINTF("Restore to original Done.\n");
          } else {
            ERROR("Failed to mount %s partition", PARTITION_NAME);
            ret = false;
            break;
          }
        }
      } else {
        PRINTF("First run, begin to backup oryx configures...\n");
        mount_f = mount_partition(PARTITION_NAME, MOUNT_DIR,
                                  MOUNT_FS_TYPE);
        if (mount_f) {
          if (!touch_file(FIRST_RUN)) {
            ret = false;
            break;
          }

          if (!backup_config(ORYX_BASE, ORYX_PATH, ORYX_PATH, MOUNT_DIR)) {
            remove(FIRST_RUN);
            ret = false;
            break;
          }
          PRINTF("Backup Done.\n");
        }
      }
    }
  } while (0);

  if (mount_f) {
    umount_dir(MOUNT_DIR);
  }

  return ret;
}

int AMAppsLauncher::init()
{
  int ret = 0;

  do {
    AMConfig *config = AMConfig::create(DEFAULT_APPS_CONF_FILE);
    if (!config) {
      ERROR("AppsLaucher: fail to create from config file\n");
      ret = -3;
      break;
    }

    AMConfig &apps = *config;

    //Determine whether or not backup;
    if (apps["backup_config"].exists()) {
      if (apps["backup_config"].get<bool>(false)) {
        init_prepare();
      }
    }

    //use Config parser to parse acs file to get the service config to load
    m_service_manager = AMServiceManager::get_instance();
    if(!m_service_manager) {
      ERROR("AppsLauncher : unable to get service manager!");
      ret = -1;
      break;
    }

    if(!(m_api_proxy = AMAPIProxy::get_instance())) {
      ERROR("AppsLauncher : unable to get api Proxy \n");
      ret = -2;
      break;
    }
    int32_t apps_num = apps.length();
    m_service_list.clear();
    for (int32_t i = 0; i < apps_num; ++ i) {
      am_service_attribute svc = {0};
      if (apps[i]["name"].exists()) {
        std::string name = apps[i]["name"].get<std::string>("");
        strncpy(svc.name, name.c_str(), AM_SERVICE_NAME_MAX_LENGTH);
      }
      if (apps[i]["filename"].exists()) {
        std::string file_name = apps[i]["filename"].get<std::string>("");
        strncpy(svc.filename, file_name.c_str(), AM_SERVICE_NAME_MAX_LENGTH);
      }
      if (apps[i]["type"].exists()) {
        svc.type = AM_SERVICE_CMD_TYPE(apps[i]["type"].get<int>(0));
      }
      if (apps[i]["enable"].exists()) {
        svc.enable = apps[i]["enable"].get<bool>(false);
      }

      //if a service is enabled, but some fields are invalid, report error
      if (svc.enable) {
        if (0 == strlen(svc.name)) {
          ERROR("AppsLauncher: service[%d] has emtpy name!", i);
          ret = -4;
          break;
        }
        if (0 == svc.type) {
          ERROR("AppsLauncher: service[%s] type is not set! "
                "Please set type to one of 'enum AM_SERVICE_CMD_TYPE'",
                svc.name);
          ret = -5;
          break;
        }
        m_service_list.push_back(svc);
      }
    }
    delete config;

    if (ret != 0) {
      break;
    }

    //now add services
    for (auto &service : m_service_list) {
      if (service.enable) {
        std::string filename = std::string("/usr/bin/") + service.filename;
        AMServiceBase *service_base = new AMServiceBase(service.filename,
                                                        filename.c_str(),
                                                        service.type);
        if (!service_base) {
          ERROR("AppsLaucher: unable to create AMServiceBase object!");
          ret = -6;
          break;
        }
        m_service_manager->add_service(service_base);
      }
    }

    if (m_service_manager->init_services() < 0) {
      ERROR("AppsLaucher: server manager init service failed!");
      ret = -7;
      break;
    }

    if (m_api_proxy->register_notify_cb(AM_SERVICE_TYPE_EVENT) < 0) {
      WARN("AppsLaucher: register notify callback on event_svc failed");
    }
    if (m_api_proxy->register_notify_cb(AM_SERVICE_TYPE_MEDIA) < 0) {
      WARN("AppsLaucher: register notify callback on media_svc failed");
    }
    if (m_api_proxy->register_notify_cb(AM_SERVICE_TYPE_IMAGE) < 0) {
      WARN("AppsLaucher: register notify callback on image_svc failed");
    }
  } while(0);

  return ret;
}

int AMAppsLauncher::start()
{

  int ret = 0;
  do {
    if (!m_service_manager) {
      ret = -1;
      break;
    }
    //start all of the services
    if (m_service_manager->start_services() < 0) {
      ERROR("AppsLauncher: server manager start service failed!");
      ret = -2;
      break;
    }
  } while (0);

  return ret;
}


int AMAppsLauncher::stop()
{
  int ret = 0;
  do {
    if (!m_service_manager) {
      ret = -1;
      break;
    }
    if (m_service_manager->stop_services() < 0) {
      ERROR("service manager is unable to stop services!");
    }
  } while (0);
  return ret;
}

int AMAppsLauncher::clean()
{
  int ret = 0;
  do {
    if (!m_service_manager) {
      ret = -1;
      break;
    }
    PRINTF("clean services.");
    if (m_service_manager->clean_services() < 0) {
      ERROR("service manager is unable to clean services!");
    }
  } while (0);
  m_api_proxy = nullptr;
  m_service_manager = nullptr;

  return ret;
}


int main(int argc, char *argv[])
{
  AMAppsLauncher *apps_launcher;
  int ret = 0;
  bool wdg_enable = false;

  signal(SIGINT,  SIG_IGN);
  signal(SIGQUIT, SIG_IGN);
  signal(SIGTERM, SIG_IGN);
  register_critical_error_signal_handler();

  if ((argc == 2) && (strcmp(argv[1], "-w") == 0)) {
    wdg_enable = true;
  } else {
    PRINTF("Watchdog function is not enable. "
        "Please type 'apps_launcher -w' to enable watchdog function!");
  }

  AMPIDLock lock;
  if (lock.try_lock() < 0) {
    ERROR("unable to lock PID, same name process should be running already!");
    return -1;
  }

  do {
    if (!(apps_launcher = AMAppsLauncher::get_instance())) {
      ERROR("AppsLauncher: Failed to create instance of AMAppsLauncher!");
      ret = -1;
      break;
    }

    if (apps_launcher->init() < 0) {
      ERROR("AppsLauncher: init failed!");
      ret = -2;
      break;
    }

    if (apps_launcher->start() < 0) {
      ERROR("AppsLauncher: start failed!");
      ret = -2;
      break;
    }
    g_service_running = true;

    //start watchdog
    if (wdg_enable) {
      gWdInstance = new AMWatchdogService();
      if (AM_LIKELY(gWdInstance &&
                    gWdInstance->init(apps_launcher->get_service_list()))) {
        gWdInstance->start();
        PRINTF(" 'apps_launcher -w' enabled watchdog function!");
      } else if (AM_LIKELY(!gWdInstance)) {
        ERROR("Failed to create watchdog service instance!");
        ret =-3;
        break;
      }
    }
    signal(SIGINT,  sigstop);
    signal(SIGQUIT, sigstop);
    signal(SIGTERM, sigstop);
    //handling errors
    if (ret < 0) {
      ERROR("apps launcher met error, clean all services and quit!");
      break;
    }
    //////////////////// Handler for Ctrl+C Exit ///////////////////////////////
    PRINTF("press Ctrl+C to quit\n");
    while (g_service_running) {
      pause();
    }
  } while (0);

  signal(SIGINT,  SIG_IGN);
  signal(SIGQUIT, SIG_IGN);
  signal(SIGTERM, SIG_IGN);

  if (apps_launcher)  {
    PRINTF("Stop all the services!");
    apps_launcher->stop();
    apps_launcher->clean();
    apps_launcher->destroy();
    delete gWdInstance;
  }
  return ret;
}
