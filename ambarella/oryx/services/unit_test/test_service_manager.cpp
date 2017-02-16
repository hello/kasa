/*******************************************************************************
 * test_service_manager.cpp
 *
 * History:
 *   2014�?�?6�?- [lysun] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co,Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/
#include "am_base_include.h"
#include "am_log.h"

#include "am_service_base.h"
#include "am_service_manager.h"
#include "commands/am_service_impl.h"

#include <signal.h>
static AMServiceManagerPtr g_service_manager = NULL;

static am_service_attribute service_list[] = { { AM_SERVICE_SYSTEM_NAME,
                                                 AM_SERVICE_TYPE_SYSTEM,
                                                 true, false, false, false
                                                },
                                               { AM_SERVICE_VIDEO_NAME,
                                                 AM_SERVICE_TYPE_VIDEO,
                                                 true, false, false, false
                                                }, };

static void sigstop(int arg)
{
  INFO("signal caught\n");
}

int main()
{
  AMServiceBase *service;
  int service_count;
  int i;
  char filename[512];
  signal(SIGINT, sigstop);
  signal(SIGQUIT, sigstop);
  signal(SIGTERM, sigstop);

  g_service_manager = AMServiceManager::get_instance();
  if (!g_service_manager) {
    ERROR("service manager created failed \n");
    return -1;
  }

  service_count = sizeof(service_list) / sizeof(service_list[0]);

  for (i = 0; i < service_count; i ++) {
    sprintf(filename, "/usr/bin/%s", service_list[i].name);
    //INFO("service filename is %s\n", filename);
    service = new AMServiceBase(service_list[i].name,
                                filename,
                                service_list[i].type);
    g_service_manager->add_service(service);
  }

  //service init creates process and IPC from caller to the actual process
  if (g_service_manager->init_services() < 0) {
    ERROR("server manager init service failed \n");
    return -2;
  }

  PRINTF("press Ctrl+C to quit\n");
  //just use sigstop to capture SIGNAL and trigger pause
  pause();
  INFO("service manager clean all services now\n");
  if (g_service_manager->clean_services() < 0) {
    ERROR("service manager is unable to clean services\n");
  }

  return 0;
}



