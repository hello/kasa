/*******************************************************************************
 * am_event_monitor.cpp
 *
 * Histroy:
 *  2012-08-09      [zkyang] created file
 *  2014-10-21      [Louis Sun] modified the file,
 *                  and change to am_event_monitor.cpp
 *  2014-11-20     [Dongge Wu] reconstruct and refine
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

#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <dlfcn.h>
#include <mutex>

#include "am_base_include.h"
#include "am_log.h"
#include "am_event_types.h"
#include "am_base_event_plugin.h"
#include "am_event_monitor_if.h"
#include "am_event_monitor.h"

#define PATH_LEN 256

#ifdef BUILD_AMBARELLA_ORYX_EVENT_PLUGIN_DIR
#define CAMERA_EVENT_PLUGIN_DIR  BUILD_AMBARELLA_ORYX_EVENT_PLUGIN_DIR
#else
#define CAMERA_EVENT_PLUGIN_DIR  "/usr/lib/oryx/event"
#endif

#define PLUGIN_PATH(s) (CAMERA_EVENT_PLUGIN_DIR s)
#define AUTO_LOCK_EVENT(mtx) std::lock_guard<std::mutex> lck(mtx)

static std::mutex event_mtx;

AMEventMonitor * AMEventMonitor::this_instance = nullptr;

AMIEventMonitorPtr AMIEventMonitor::get_instance()
{
  return AMEventMonitor::get_instance();
}

AMEventMonitor* AMEventMonitor::get_instance()
{
  AUTO_LOCK_EVENT(event_mtx);
  if (!this_instance) {
    if ((this_instance = new AMEventMonitor())
        && (!this_instance->construct())) {
      delete this_instance;
      this_instance = nullptr;
    }
  }
  return this_instance;
}

void AMEventMonitor::inc_ref()
{
  ++ ref_counter;
}

void AMEventMonitor::release()
{
  if ((ref_counter > 0) && (-- ref_counter == 0)) {
    delete this_instance;
    this_instance = nullptr;
  }
}

/* Constructor && Destructor */
AMEventMonitor::AMEventMonitor() :
    ref_counter(0)
{

}

AMEventMonitor::~AMEventMonitor()
{
  INFO("~AMEventMonitor!\n");
  int32_t i = 0;
  for (i = 0; i < EV_ALL_MODULE_NUM; i ++) {
    if (m_module_files[i]) {
      free((void *) m_module_files[i]);
      m_module_files[i] = nullptr;
    }
  }
}

bool AMEventMonitor::construct()
{
  bool ret = true;

  memset(m_module_files, 0, sizeof(m_module_files[0]) * EV_ALL_MODULE_NUM);
  memset(m_module_handles, 0, sizeof(m_module_handles[0]) * EV_ALL_MODULE_NUM);
  memset(m_event_plugin, 0, sizeof(m_event_plugin[0]) * EV_ALL_MODULE_NUM);

  //scan plugin modules in CAMERA_EVENT_PLUGIN_DIR
  DIR *module_dir = nullptr;
  struct dirent *file = nullptr;
  struct stat file_info;
  void *handle = nullptr;
  EVENT_MODULE_ID id;
  char *module_path = nullptr;
  AMIEventPlugin *event_plugin = nullptr;

  module_dir = opendir(CAMERA_EVENT_PLUGIN_DIR);
  if (!module_dir) {
    ERROR("Can not open dir %s\n", CAMERA_EVENT_PLUGIN_DIR);
    return false;
  }

  while ((file = readdir(module_dir)) != nullptr) {
    if (strncmp(file->d_name, ".", 1) == 0)
      continue;

    module_path = (char *) calloc(PATH_LEN, 1);
    sprintf(module_path, "%s/%s", CAMERA_EVENT_PLUGIN_DIR, file->d_name);
    if (stat(module_path, &file_info) >= 0 && S_ISREG(file_info.st_mode)) {
      INFO("dlopen %s \n", module_path);
      handle = dlopen(module_path, RTLD_NOW);

      if (handle == nullptr) {
        ERROR("dlopen: can not open %s\n", file->d_name);
        ERROR("%s\n", dlerror());
        ret = false;
        free(module_path);
        break;
      }

      event_plugin = *(AMIEventPlugin **) dlsym(handle, "instance");
      if (event_plugin == nullptr) {
        ERROR("dlsym: can not load symbol instance");
        ret = false;
        free(module_path);
        break;
      }

      id = event_plugin->get_plugin_ID();
      if (!valid_module_id(id)) {
        ERROR("Invalid module id : %d!\n", id);
        ret = false;
        break;
      }

      m_module_files[id] = module_path;
      m_module_handles[id] = handle;
      m_event_plugin[id] = event_plugin;
    }
  }

  closedir(module_dir);
  return ret;
}

bool AMEventMonitor::register_module(EVENT_MODULE_ID id,
                                     const char *module_path)
{
  if (!valid_module_id(id)) {
    WARN("Invalid module id!\n");
    return false;
  }

  if (module_path == nullptr) {
    WARN("Module path is NULL!\n");
    return false;
  }

  AUTO_LOCK_EVENT(event_mtx);
  if (m_module_files[id] != nullptr
      && strcmp(m_module_files[id], module_path)) {
    dlclose(m_module_handles[id]);
    m_module_handles[id] = nullptr;
    m_event_plugin[id] = nullptr;
  }

  m_module_files[id] = module_path;
  return true;
}

bool AMEventMonitor::is_module_registered(EVENT_MODULE_ID id)
{
  if (!valid_module_id(id)) {
    WARN("Invalid module id!\n");
    return false;
  }
  DEBUG("m_module_files[%d] : %s\n", id, m_module_files[id]);
  return (m_module_files[id] != nullptr);
}

bool AMEventMonitor::start_event_monitor(EVENT_MODULE_ID id)
{
  if (!valid_module_id(id)) {
    WARN("Invalid module id!\n");
    return false;
  }

  if (get_event_plugin(id) == nullptr) {
    NOTICE("Module[%d] is not loaded!\n", id);
    return true;
  }

  return m_event_plugin[id]->start_plugin();
}

bool AMEventMonitor::stop_event_monitor(EVENT_MODULE_ID id)
{
  if (!valid_module_id(id)) {
    WARN("Invalid module id!\n");
    return false;
  }

  if (m_event_plugin[id] == nullptr) {
    NOTICE("Module[%d] is not started !\n", id);
    return true;
  }

  return m_event_plugin[id]->stop_plugin();
}

bool AMEventMonitor::destroy_event_monitor(EVENT_MODULE_ID id)
{
  if (!valid_module_id(id)) {
    WARN("Invalid module id!\n");
    return false;
  }

  AUTO_LOCK_EVENT(event_mtx);
  if (m_module_files[id] == nullptr) {
    WARN("This module[id:%d] is not registered !\n", id);
    return true;
  }

  if (m_module_handles[id] == nullptr || m_event_plugin[id] == nullptr) {
    WARN("This module[id:%d] is not started !\n", id);
    return true;
  }

  if (!m_event_plugin[id]->stop_plugin()) {
    ERROR("Stop event module %d failed!\n", id);
    return false;
  }

  dlclose(m_module_handles[id]);
  m_module_handles[id] = nullptr;
  m_event_plugin[id] = nullptr;

  return true;
}

bool AMEventMonitor::start_all_event_monitor()
{
  bool ret = true;
  int32_t id = 0;

  for (id = 0; id < EV_ALL_MODULE_NUM; id ++) {
    if (m_module_files[id]) {
      ret = start_event_monitor(EVENT_MODULE_ID(id));
    }
    if (!ret) {
      ERROR("Fail to start module[%d]!\n", id);
      return ret;
    }
  }

  return true;
}

bool AMEventMonitor::stop_all_event_monitor()
{
  bool ret = true;
  int32_t id = 0;

  for (id = 0; id < EV_ALL_MODULE_NUM; id ++) {
    if (m_module_files[id]) {
      ret = stop_event_monitor(EVENT_MODULE_ID(id));
    }
    if (!ret) {
      ERROR("Fail to stop module[%d]!\n", id);
      return ret;
    }
  }

  return true;
}

bool AMEventMonitor::destroy_all_event_monitor()
{
  bool ret = true;
  int32_t id = 0;

  for (id = 0; id < EV_ALL_MODULE_NUM; id ++) {
    if (m_module_files[id]) {
      ret = destroy_event_monitor((EVENT_MODULE_ID) id);
    }
    if (!ret) {
      ERROR("Fail to destroy module[%d]!\n", id);
      return ret;
    }
  }

  return true;
}

bool AMEventMonitor::set_monitor_config(EVENT_MODULE_ID id,
                                        EVENT_MODULE_CONFIG *config)
{
  if (!valid_module_id(id)) {
    WARN("Invalid module id!\n");
    return false;
  }

  if (config == nullptr || config->value == nullptr) {
    WARN("Invalid parameter, config is NULL!\n");
    return false;
  }

  if (get_event_plugin(id) == nullptr) {
    ERROR("Can not get event plugin handler!\n");
    return false;
  }

  return m_event_plugin[id]->set_plugin_config(config);
}

bool AMEventMonitor::get_monitor_config(EVENT_MODULE_ID id,
                                        EVENT_MODULE_CONFIG *config)
{
  if (!valid_module_id(id)) {
    WARN("Invalid module id!\n");
    return -1;
  }

  if (config == nullptr || config->value == nullptr) {
    WARN("Invalid parameter, config is NULL!\n");
    return false;
  }

  if (get_event_plugin(id) == nullptr) {
    ERROR("Can not get event plugin handler!\n");
    return false;
  }

  return m_event_plugin[id]->get_plugin_config(config);
}

inline AMIEventPlugin* AMEventMonitor::get_event_plugin(EVENT_MODULE_ID id)
{
  AUTO_LOCK_EVENT(event_mtx);
  if (m_module_files[id] == nullptr) {
    WARN("This module[id:%d] is not registered !\n", id);
    return nullptr;
  }

  if (!m_module_handles[id]) {
    m_module_handles[id] = dlopen(m_module_files[id], RTLD_NOW);

    if (m_module_handles[id] == nullptr) {
      ERROR("dlerror: %s\n", dlerror());
      ERROR("dlopen:Can not load %s !\n", m_module_files[id]);
      return nullptr;
    }
  }

  if (!m_event_plugin[id]) {
    m_event_plugin[id] = *(AMIEventPlugin**) dlsym(m_module_handles[id],
                                                   "instance");
    if (m_event_plugin[id] == nullptr) {
      ERROR("dlsym:Can not load symbol instance");
      return nullptr;
    }
  }

  return m_event_plugin[id];
}

inline bool AMEventMonitor::valid_module_id(EVENT_MODULE_ID id)
{
  return (id >= EV_AUDIO_ALERT_DECT) && (id < EV_ALL_MODULE_NUM);
}
