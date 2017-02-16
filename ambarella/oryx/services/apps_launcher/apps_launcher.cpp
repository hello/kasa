/*******************************************************************************
 * apps_launcher.cpp
 *
 * History:
 *   2014-9-28 - [lysun] created file
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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include "am_base_include.h"
#include "am_log.h"
#include "am_pid_lock.h"
#include "commands/am_service_impl.h"
#include "am_api_proxy.h"
#include "am_service_manager.h"
#include <string>
#include "am_configure.h"
#include "apps_launcher.h"
#include "am_define.h"
#include "am_watchdog.h"

#define DEFAULT_APPS_CONF_FILE "/etc/oryx/apps/apps_launcher.acs"

static AMWatchdogService* gWdInstance = NULL;

static void sigstop(int arg)
{
  INFO("signal caught\n");
}



static AMAppsLauncher *m_instance = NULL;

AMAppsLauncher::AMAppsLauncher():
    m_service_manager(NULL),
    m_api_proxy(NULL)
{
  memset(m_service_list, 0, sizeof(m_service_list));
}

AMAppsLauncher *AMAppsLauncher::get_instance()
{
  if (!m_instance) {
    m_instance = new AMAppsLauncher();
  }
  return m_instance;
}

AMAppsLauncher::~AMAppsLauncher()
{
  m_instance = NULL;
  delete m_api_proxy;
}


int AMAppsLauncher::load_config()
{

  return 0;
}



int AMAppsLauncher::init()
{
  int i = 0;
  int ret = 0;
  char oryx_conf_file[] = DEFAULT_APPS_CONF_FILE;
  AMConfig *config = NULL;
  std::string tmp;
  int service_type = -1;
  bool enable_flag = false;
  AMServiceBase *service = NULL;
  char filename[512] = {0};

  do {
 //use Config parser to parse acs file to get the service config to load
   m_service_manager = AMServiceManager::get_instance();
   if(!m_service_manager) {
     ERROR("AppsLauncher : unable to get service manager \n");
     ret = -1;
     break;
   }

   m_api_proxy = AMAPIProxy::get_instance();
   if(!m_api_proxy) {
       ERROR("AppsLauncher : unable to get api Proxy \n");
       ret = -2;
       break;
   }

   tmp = std::string(oryx_conf_file);

   config = AMConfig::create(tmp.c_str());
    if (!config) {
      ERROR("AppsLaucher: fail to create from config file\n");
      ret = -3;
      break;
    }

    AMConfig& apps = *config;

    for (i = 0; i < APP_LAUNCHER_MAX_SERVICE_NUM; i ++) {
      if (apps[i]["filename"].exists()) {
        tmp = apps[i]["filename"].get<std::string>("");
        strncpy(m_service_list[i].name,
                tmp.c_str(),
                AM_SERVICE_NAME_MAX_LENGTH);
        m_service_list[i].name[AM_SERVICE_NAME_MAX_LENGTH - 1] = '\0';
      }
      if (apps[i]["type"].exists()) {
        service_type = apps[i]["type"].get<int>(0);
        m_service_list[i].type = AM_SERVICE_CMD_TYPE(service_type);
      }
      if (apps[i]["enable"].exists()) {
        enable_flag = apps[i]["enable"].get<bool>(false);
        m_service_list[i].enable = enable_flag;
      }
      if (apps[i]["event_handle_button"].exists()) {
        enable_flag = apps[i]["event_handle_button"].get<bool>(false);
        m_service_list[i].handle_event_button = enable_flag;
      }
      if (apps[i]["event_handle_motion"].exists()) {
        enable_flag = apps[i]["event_handle_motion"].get<bool>(false);
        m_service_list[i].handle_event_motion = enable_flag;
      }
      if (apps[i]["event_handle_audio"].exists()) {
        enable_flag = apps[i]["event_handle_audio"].get<bool>(false);
        m_service_list[i].handle_event_audio = enable_flag;
      }

      //if a service is enabled, but some fields are invalid, report error
      if (m_service_list[i].enable) {
        if (strlen(m_service_list[i].name) == 0){
           ERROR("AppsLaucher: service %d with empty name \n", i);
           ret = -4;
           break;
        }

        if (m_service_list[i].type == 0) {
          ERROR("AppsLaucher: service %s with type not set, please set to be 'enum AM_SERVICE_CMD_TYPE' \n",
                m_service_list[i].name);
          ret = -5;
          break;
        }
      }
    }

    //now add services
    for (i = 0; i < APP_LAUNCHER_MAX_SERVICE_NUM; i ++) {
      if (m_service_list[i].enable) {
        sprintf(filename, "/usr/bin/%s", m_service_list[i].name);
        service = new AMServiceBase(m_service_list[i].name,
                                    filename,
                                    m_service_list[i].type);
        if (!service) {
          ERROR("AppsLaucher: unable to create AMServiceBase object \n");
          ret = -6;
          break;
        }
        m_service_manager->add_service(service);
      }
    }

    if (m_service_manager->init_services() < 0) {
      ERROR("AppsLaucher: server manager init service failed \n");
      ret = -7;
      break;
    }


    INFO("api proxy tries to init AIR API interface\n");
    if(m_api_proxy->init() < 0) {
      ERROR("AppsLaucher: api proxy init failed \n");
      ret = -8;
      break;
    }


  }while(0);

  //if incorrect, delete all objects
  if (ret !=0) {
    delete m_api_proxy;
    m_api_proxy = NULL;
  }
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
      ERROR("AppsLaucher: server manager start service failed \n");
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
      ERROR("service manager is unable to stop services\n");
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
       ERROR("service manager is unable to clean services\n");
     }
   } while (0);
  delete m_api_proxy;
  m_api_proxy = NULL;
  m_service_manager = NULL;

  return ret;
}


int main(int argc, char *argv[])
{
  AMAppsLauncher *apps_launcher;
  int ret = 0;
  int wdg_enable=0;
  signal(SIGINT, sigstop);
  signal(SIGQUIT, sigstop);
  signal(SIGTERM, sigstop);

  if((argc==2)&&(strcmp(argv[1],"-w")==0)){
    wdg_enable=1;
  } else {
    PRINTF("Watchdog function is not enable. Please type 'apps_launcher -w' to enable watchdog function! ");
  }

  AMPIDLock lock;
  if (lock.try_lock() < 0) {
     ERROR("unable to lock PID, same name process should be running already\n");
     return -1;
  }

  do {

    apps_launcher = AMAppsLauncher::get_instance();
    if (!apps_launcher) {
      ERROR("AppsLauncher: AMAppsLauncher created failed \n");
      ret = -1;
      break;
    }

    //do init
    if (apps_launcher->init() < 0) {
      ERROR("AppsLauncher: init failed \n");
      ret = -2;
      break;
    }
    //start running
    if (apps_launcher->start() < 0) {
      ERROR("AppsLauncher: start failed \n");
      ret = -2;
      break;
    }

    //start watchdog
    if( wdg_enable ==1){
      gWdInstance = new AMWatchdogService();
      if (AM_LIKELY(gWdInstance && gWdInstance->init(apps_launcher->m_service_list))) {
        gWdInstance->start();
        PRINTF(" 'apps_launcher -w' enabled watchdog function!");
      } else if (AM_LIKELY(!gWdInstance)) {
        ERROR("Failed to create watchdog service instance!");
        ret =-3;
        break;
      }
    }
  } while (0);

  //handling errors
  if (ret < 0) {
    ERROR("apps launcher met error, clean all services and quit \n");
    goto exit1;
  }

//////////////////// Handler for Ctrl+C Exit ///////////////////////////////
  PRINTF("press Ctrl+C to quit\n");
  pause();
exit1:
  if (apps_launcher)  {
    apps_launcher->stop();
    apps_launcher->clean();
    delete apps_launcher;
    if ( wdg_enable == 1)
      delete gWdInstance;
  }
  return ret;
}


