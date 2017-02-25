/*******************************************************************************
 * am_sip_ua_server_msg_action.cpp
 *
 * History:
 *   2015-1-28 - [Shiming Dong] created file
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
#include "am_service_impl.h"
#include "am_sip_ua_server_if.h"
#include "am_service_frame_if.h"
#include "am_sip_ua_server_protocol_data_structure.h"
#include <signal.h>

extern AM_SERVICE_STATE g_service_state;
extern AMISipUAServerPtr g_sip_ua_server;
extern AMIServiceFrame *g_service_frame;

void ON_SERVICE_INIT(void *msg_data,
                     int msg_data_size,
                     void *result_addr,
                     int result_max_size)
{
  am_service_result_t *service_result = (am_service_result_t *)result_addr;
  service_result->ret = 0;
  service_result->state = g_service_state;
  switch(g_service_state) {
    case AM_SERVICE_STATE_INIT_DONE: {
      INFO("SIP service Init Done...");
    }break;
    case AM_SERVICE_STATE_ERROR: {
      ERROR("Failed to initialize SIP service...");
    }break;
    case AM_SERVICE_STATE_NOT_INIT: {
      INFO("SIP service is still initializing...");
    }break;
    default:break;
  }
}

void ON_SERVICE_DESTROY(void *msg_data,
                        int msg_data_size,
                        void *result_addr,
                        int result_max_size)
{
  int ret = 0;
  PRINTF("ON SIP SERVICE DESTROY.");
  if (!g_sip_ua_server) {
    g_service_state = AM_SERVICE_STATE_ERROR;
    ret = -1;
    ERROR("SIP service: Failed to get AMSipUAServer instance!");
  } else {
    g_sip_ua_server->stop();
    g_service_state = AM_SERVICE_STATE_STOPPED;
  }
  ((am_service_result_t*)result_addr)->ret = ret;
  ((am_service_result_t*)result_addr)->state = g_service_state;
  g_service_frame->quit(); /* make run() in main function quit */
}

void ON_SERVICE_START(void *msg_data,
                      int msg_data_size,
                      void *result_addr,
                      int result_max_size)
{
  int ret = 0;
  PRINTF("ON SIP SERVICE START.");
  am_service_result_t *service_result = (am_service_result_t *) result_addr;
  if (!g_sip_ua_server) {
    ret = -1;
    g_service_state = AM_SERVICE_STATE_ERROR;
    ERROR("SIP service: Failed to get AMSipUAServer instance!");
  } else if (!g_sip_ua_server->start()) {
    ERROR("SIP UA Server: start failed!");
    ret = -2;
  } else {
    g_service_state = AM_SERVICE_STATE_STARTED;
  }

  service_result->ret = ret;
  service_result->state = g_service_state;
}

void ON_SERVICE_STOP(void *msg_data,
                     int msg_data_size,
                     void *result_addr,
                     int result_max_size)
{
  int ret = 0;
  PRINTF("ON SIP SERVICE STOP.");
  am_service_result_t *service_result = (am_service_result_t *) result_addr;
  if (!g_sip_ua_server) {
    ret = -1;
    g_service_state = AM_SERVICE_STATE_ERROR;
    ERROR("SIP service: Failed to get AMSipUAServer instance!");
  } else {
    g_sip_ua_server->stop();
    g_service_state = AM_SERVICE_STATE_STOPPED;
  }
  service_result->ret = ret;
  service_result->state = g_service_state;
}

void ON_SERVICE_RESTART(void *msg_data,
                        int msg_data_size,
                        void *result_addr,
                        int result_max_size)
{
  am_service_result_t *service_result = (am_service_result_t *)result_addr;
  PRINTF("ON SIP SERVICE RESTART.");
  g_sip_ua_server->stop();
  g_service_state = AM_SERVICE_STATE_STOPPED;
  if (g_sip_ua_server->start()) {
    g_service_state = AM_SERVICE_STATE_STARTED;
    service_result->ret = 0;
  } else {
    g_service_state = AM_SERVICE_STATE_ERROR;
    service_result->ret = -1;
  }
  service_result->state = g_service_state;
}

void ON_SERVICE_STATUS(void *msg_data,
                       int msg_data_size,
                       void *result_addr,
                       int result_max_size)
{
  PRINTF("ON SIP SERVICE STATUS.");
  ((am_service_result_t*)result_addr)->ret = 0;
  ((am_service_result_t*)result_addr)->state = g_service_state;
}

void ON_AM_IPC_MW_CMD_SET_SIP_REGISTRATION(void *msg_data,
                                           int msg_data_size,
                                           void *result_addr,
                                           int result_max_size)
{
  PRINTF("ON_AM_IPC_MW_CMD_SET_SIP_REGISTRATION");
  int32_t ret = 0;
  am_service_result_t *service_result = (am_service_result_t *) result_addr;
  AMSipRegisterParameter *sip_register = (AMSipRegisterParameter*)msg_data;
  do {
    if (!msg_data || (0 == msg_data_size) || !result_addr) {
      ret = -1;
      ERROR("Invalid parameters!");
      break;
    }
    if (!g_sip_ua_server) {
      ret = -1;
      ERROR("g_sip_ua_server is NULL!");
      break;
    } else if(!g_sip_ua_server->set_sip_registration_parameter(sip_register)) {
      ret = -1;
      ERROR("set_sip_registration_parameter error!");
      break;
    }
  } while(0);
  service_result->ret = ret;

}

void ON_AM_IPC_MW_CMD_SET_SIP_CONFIGURATION(void *msg_data,
                                            int msg_data_size,
                                            void *result_addr,
                                            int result_max_size)
{
  PRINTF("ON_AM_IPC_MW_CMD_SET_SIP_CONFIGURATION");
  int32_t ret = 0;
  am_service_result_t *service_result = (am_service_result_t *) result_addr;
  AMSipConfigParameter *sip_config = (AMSipConfigParameter*)msg_data;
  do {
    if (!msg_data || (0 == msg_data_size) || !result_addr) {
      ret = -1;
      ERROR("Invalid parameters!");
      break;
    }
    if (!g_sip_ua_server) {
      ret = -1;
      ERROR("g_sip_ua_server is NULL!");
      break;
    } else if(!g_sip_ua_server->set_sip_config_parameter(sip_config)) {
      ret = -1;
      ERROR("set_sip_config_parameter error!");
      break;
    }
  } while(0);
  service_result->ret = ret;

}

void ON_AM_IPC_MW_CMD_SET_SIP_MEDIA_PRIORITY(void *msg_data,
                                             int msg_data_size,
                                             void *result_addr,
                                             int result_max_size)
{
  PRINTF("ON_AM_IPC_MW_CMD_SET_SIP_MEDIA_PRIORITY");
  int32_t ret = 0;
  am_service_result_t *service_result = (am_service_result_t *) result_addr;
  AMMediaPriorityList *media_list = (AMMediaPriorityList*)msg_data;
  do {
    if (!msg_data || (0 == msg_data_size) || !result_addr) {
      ret = -1;
      ERROR("Invalid parameters!");
      break;
    }
    if (!g_sip_ua_server) {
      ret = -1;
      ERROR("g_sip_ua_server is NULL!");
      break;
    } else if(!g_sip_ua_server->set_sip_media_priority_list(media_list)) {
      ret = -1;
      ERROR("set_sip_media_priority_list error!");
      break;
    }
  } while(0);
  service_result->ret = ret;

}

void ON_AM_IPC_MW_CMD_SET_SIP_CALLEE_ADDRESS(void *msg_data,
                                             int msg_data_size,
                                             void *result_addr,
                                             int result_max_size)
{
  PRINTF("ON_AM_IPC_MW_CMD_SET_SIP_CALLEE_ADDRESS");
  int32_t ret = 0;
  am_service_result_t *service_result = (am_service_result_t*)result_addr;
  AMSipCalleeAddress *address = (AMSipCalleeAddress*)msg_data;
  do {
    if (!msg_data || (0 == msg_data_size) || !result_addr) {
      ret = -1;
      ERROR("Invalid parameters!");
      break;
    }
    if (!g_sip_ua_server) {
      ret = -1;
      ERROR("g_sip_ua_server is NULL!");
      break;
    } else if(!g_sip_ua_server->initiate_sip_call(address)) {
      ret = -1;
      ERROR("Initiate sip invite call error!");
      break;
    }
  } while(0);
  service_result->ret = ret;

}

void ON_AM_IPC_MW_CMD_SET_SIP_HANGUP_USERNAME(void *msg_data,
                                              int msg_data_size,
                                              void *result_addr,
                                              int result_max_size)
{
  PRINTF("ON_AM_IPC_MW_CMD_SET_SIP_HANGUP_USERNAME");
  int32_t ret = 0;
  am_service_result_t *service_result = (am_service_result_t*)result_addr;
  AMSipHangupUsername *username = (AMSipHangupUsername*)msg_data;
  do {
    if (!msg_data || (0 == msg_data_size) || !result_addr) {
      ret = -1;
      ERROR("Invalid parameters!");
      break;
    }
    if (!g_sip_ua_server) {
      ret = -1;
      ERROR("g_sip_ua_server is NULL");
      break;
    } else if(!g_sip_ua_server->hangup_sip_call(username)) {
      ret = -1;
      ERROR("Hangup sip call error");
      break;
    }
  } while(0);
  service_result->ret = ret;
}
