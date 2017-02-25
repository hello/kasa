/*******************************************************************************
 * am_service_manager.cpp
 *
 * History:
 *   2014-9-12 - [lysun] created file
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
#include "am_log.h"

#include "am_service_base.h"
#include "am_service_manager.h"

#define DECLARE_MUTEX(lck) std::lock_guard<AMMemLock> service_manager_lock(lck)

AMMemLock AMServiceManager::m_mutex;
AMServiceManager *AMServiceManager::m_instance = nullptr;

AMServiceManager::AMServiceManager()
{}

AMServiceManager::~AMServiceManager()
{
}

AMServiceManagerPtr AMServiceManager::get_instance()
{
  DECLARE_MUTEX(m_mutex);
  if (!m_instance) {
    m_instance = new AMServiceManager();
  }

  return m_instance;
}

void AMServiceManager::release()
{
  DECLARE_MUTEX(m_mutex);
  if((m_ref_counter) > 0 && (--m_ref_counter == 0)) {
    delete m_instance;
    m_instance = nullptr;
  }
}

void AMServiceManager::inc_ref()
{
  ++ m_ref_counter;
}

int AMServiceManager::add_service(AMServiceBase *service)
{
  DECLARE_MUTEX(m_mutex);
  if (!service) {
    ERROR("service is null!");
    return -1;
  }
  m_services.push_back(service);
  return 0;
}

//destroy all services and delete the object and clear the list
int AMServiceManager::clean_services()
{
  DECLARE_MUTEX(m_mutex);
  for (auto &v : m_services) {
    delete v; //delete object
  }
  m_services.clear(); //clear the list

  return 0;
}

AMServiceBase * AMServiceManager::find_service(AM_SERVICE_CMD_TYPE type)
{
  AMServiceBase *find = nullptr;
  for (auto &v : m_services) {
    if (v->get_type() == type) { //find the first, because services have different type
      find = v;
      break;
    }
  }
  return find;
}

int AMServiceManager::init_services() // setup IPC, and call each service's init function by IPC
{
  DECLARE_MUTEX(m_mutex);
  int ret = 0;

  do {
    if (m_state != AM_SERVICE_MANAGER_NOT_INIT) {
      ERROR("AMServiceManager: services are already initialized, state %d",
            m_state);
      ret = -1;
      break;
    }

    for (auto &v : m_services) {
      if (v->init() < 0) {
        ret = -1;
        ERROR("service %s init failed \n", v->get_name());
        break;
      }
    }

    m_state = (ret == 0) ? AM_SERVICE_MANAGER_READY : AM_SERVICE_MANAGER_ERROR;
  }while(0);

  return ret;
}

//some services will create their own IPC by their "init"
//this API calls the destroy however, it does not delete the service object,
//which means it can "init" again
int AMServiceManager::destroy_services() //the reverse operation of "init_all_services"
{
  DECLARE_MUTEX(m_mutex);
  int ret = 0;

  INFO("AMServiceManager::destroy_services\n");
  for (auto &v :  m_services) {
    if (v && (v->destroy() < 0)) {
      ret = -1;
      ERROR("Failed to destroy service %s", v->get_name());
    }
  }

  if (ret == 0) {
    m_state = AM_SERVICE_MANAGER_NOT_INIT;
  } else {
    m_state = AM_SERVICE_MANAGER_ERROR;
  }

  return ret;
}

int AMServiceManager::start_services() //call each service's start function
{
  DECLARE_MUTEX(m_mutex);
  int ret = 0;

  INFO("AMServiceManager::start_services");
  if (m_state != AM_SERVICE_MANAGER_READY) {
    ERROR("AMServiceManager: cannot start, current state %d", m_state);
    return -1;
  }

  for (auto &v : m_services)
  {
    if (v->start() < 0) {
      ret = -1;
      ERROR("Failed to start service %s", v->get_name());
      break;
    }
  }

  m_state = (ret == 0) ? AM_SERVICE_MANAGER_RUNNING : AM_SERVICE_MANAGER_ERROR;

  return ret;
}

int AMServiceManager::stop_services() //call each service's stop function
{
  DECLARE_MUTEX(m_mutex);
  int ret = 0;

  INFO("AMServiceManager::stop_services\n");
  if (m_state == AM_SERVICE_MANAGER_READY) {
    INFO("AMServiceManager: not started, no need stop, ignored\n");
    return 0;
  }

  //try to stop for any other conditions
  for(int i = m_services.size(); i > 0; --i) {
    if(m_services[i-1]->stop() < 0) {
      ret = -1;
      ERROR("Failed to stop service %s", m_services[i - 1]->get_name());
    }
  }

  m_state = (ret == 0) ? AM_SERVICE_MANAGER_READY : AM_SERVICE_MANAGER_ERROR;

  return ret;
}

int AMServiceManager::restart_services() //call each service's restart function
{
  DECLARE_MUTEX(m_mutex);
  int ret = 0;

  INFO("AMServiceManager::restart_services\n");
  for (auto &v : m_services)
  {
    if (v->restart() < 0) {
      ret = -1;
      ERROR("Failed to restart service %s", v->get_name());
      break;
    }
  }

  m_state = (ret == 0) ? AM_SERVICE_MANAGER_RUNNING : AM_SERVICE_MANAGER_ERROR;

  return ret;
}

int AMServiceManager::report_services() //report status of all services.
{
  DECLARE_MUTEX(m_mutex);
  int ret = 0;

  INFO("AMServiceManager::report_services\n");
  for (auto &v : m_services)
  {
    AM_SERVICE_STATE state;
    if (v->status(&state) < 0) {
      ret = -1;
      ERROR("Failed to report status of service %s", v->get_name());
      break;
    } else {
      PRINTF("service %s state is %d", v->get_name(), state);
    }
  }

  return ret;
}

