/*******************************************************************************
 * am_service_manager.cpp
 *
 * History:
 *   2014-9-12 - [lysun] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co,Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <mutex>  //using C++11 mutex
#include "am_service_base.h"
#include "am_log.h"
#include "am_service_manager.h"

//C++ 11 mutex (using non-recursive type)
static std::mutex m_mtx;
#define  DECLARE_MUTEX   std::lock_guard<std::mutex> lck (m_mtx);

AMServiceManager *AMServiceManager::m_instance = nullptr;

AMServiceManager::AMServiceManager() :
  m_state(AM_SERVICE_MANAGER_NOT_INIT),
  m_ref_counter(0)
{

}

AMServiceManager::~AMServiceManager()
{

}

AMServiceManagerPtr AMServiceManager::get_instance()
{
  DECLARE_MUTEX;
  if (!m_instance) {
    m_instance = new AMServiceManager();
  }

  return m_instance;
}

void AMServiceManager::release()
{
  DECLARE_MUTEX;
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
  DECLARE_MUTEX;
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
  DECLARE_MUTEX;
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
  DECLARE_MUTEX;
  int ret = 0;
  char buf[256] = {0};

  if (m_state != AM_SERVICE_MANAGER_NOT_INIT) {
    ERROR("AMServiceManager: cannot init, already inited, state %d\n", m_state);
    return -1;
  }

  for (auto &v : m_services)
  {
    if (v->init() < 0) {
      ret = -1;
      v->get_name(buf, 255);
      ERROR("service %s init failed \n", buf);
      break;
    }
  }

  if (ret == 0) {
    m_state = AM_SERVICE_MANAGER_READY;
  } else {
    m_state = AM_SERVICE_MANAGER_ERROR;
  }

  return ret;
}

//some services will create their own IPC by their "init"
//this API calls the destroy however, it does not delete the service object,
//which means it can "init" again
int AMServiceManager::destroy_services() //the reverse operation of "init_all_services"
{
  DECLARE_MUTEX;
  int ret = 0;
  char buf[256] = {0};

  INFO("AMServiceManager::destroy_services\n");
  for (auto &v :  m_services) {
    if (v) {
      if (v->destroy() < 0) {
        ret = -1;
        v->get_name(buf, 255);
        ERROR("service %s destroy failed \n", buf);
      }
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
  DECLARE_MUTEX;
  int ret = 0;
  char buf[256] = {0};

  INFO("AMServiceManager::start_services\n");
  if (m_state != AM_SERVICE_MANAGER_READY) {
    ERROR("AMServiceManager: cannot start, current state %d\n", m_state);
    return -1;
  }

  for (auto &v : m_services)
  {
    if (v->start() < 0) {
      ret = -1;
      v->get_name(buf, 255);
      ERROR("service %s start failed \n", buf);
      break;
    }
  }

  if (ret == 0) {
    m_state = AM_SERVICE_MANAGER_RUNNING;
  } else {
    m_state = AM_SERVICE_MANAGER_ERROR;
  }

  return ret;
}

int AMServiceManager::stop_services() //call each service's stop function
{
  DECLARE_MUTEX;
  int ret = 0;
  char buf[256] = {0};

  INFO("AMServiceManager::stop_services\n");
  if (m_state == AM_SERVICE_MANAGER_READY) {
    INFO("AMServiceManager: not started, no need stop, ignored\n");
    return 0;
  }

  //try to stop for any other conditions
  for(int i = m_services.size(); i > 0; --i) {
    m_services[i-1]->get_name(buf, 255);
    if(m_services[i-1]->stop() < 0) {
      ret = -1;
      m_services[i-1]->get_name(buf, 255);
      ERROR("service %s stop failed.", buf);
    }
  }

  if (ret == 0) {
    m_state = AM_SERVICE_MANAGER_READY;
  } else {
    m_state = AM_SERVICE_MANAGER_ERROR;
  }

  return ret;
}

int AMServiceManager::restart_services() //call each service's restart function
{
  DECLARE_MUTEX;
  int ret = 0;
  char buf[256] = {0};

  INFO("AMServiceManager::restart_services\n");
  for (auto &v : m_services)
  {
    if (v->restart() < 0) {
      ret = -1;
      v->get_name(buf, 255);
      ERROR("service %s restart failed \n", buf);
      break;
    }
  }

  if (ret == 0) {
    m_state = AM_SERVICE_MANAGER_RUNNING;
  } else {
    m_state = AM_SERVICE_MANAGER_ERROR;
  }

  return ret;
}

int AMServiceManager::report_services() //report status of all services.
{
  DECLARE_MUTEX;
  int ret = 0;
  char buf[256] = {0};

  INFO("AMServiceManager::report_services\n");
  for (auto &v : m_services)
  {
    AM_SERVICE_STATE state;
    v->get_name(buf, 255);
    if (v->status(&state) < 0) {
      ret = -1;
      ERROR("service %s report status failed \n", buf);
      break;
    } else {
      PRINTF("service %s state is %d \n", buf, state);
    }
  }

  return ret;
}
