/*******************************************************************************
 * test_service_manager.cpp
 *
 * History:
 *   2014-6 - [lysun] created file
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

#include "am_service_base.h"
#include "am_service_manager.h"
#include "commands/am_service_impl.h"

#include <signal.h>
static AMServiceManagerPtr g_service_manager = nullptr;

static am_service_attribute service_list[] = { { AM_SERVICE_SYSTEM_NAME,
                                                 AM_SERVICE_TYPE_SYSTEM,
                                                 true
                                               },
                                               { AM_SERVICE_VIDEO_NAME,
                                                 AM_SERVICE_TYPE_VIDEO,
                                                 true
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
