/*******************************************************************************
 * am_api_proxy_main.cpp
 *
 * History:
 *   2014-9-16 - [lysun] created file
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

//the universal router_api will handle all of  AIR API method_call,
//so that api proxy it self has no need to change when you add new AIR APIs
//you only need to add the AIR API definition, and add in the service,
//the API proxy will make them transparent
void router_api(void *msg_data,
                int msg_data_size,
                void *result_addr,
                int result_max_size)
{
  am_ipc_message_header_t *msg_header = (am_ipc_message_header_t*)msg_data;
  uint32_t msg_id = msg_header->msg_id;
  void *payload = (void*)(msg_header->header_size + (uint8_t*) msg_data);
  uint32_t payload_size = msg_header->payload_size;
  AM_SERVICE_CMD_TYPE type = AM_SERVICE_CMD_TYPE(GET_IPC_MSG_TYPE(msg_id));
  AM_SERVICE_MANAGER_STATE state;
  am_service_result_t *service_result;

  INFO("api proxy: router_api:  msg type %d \n", type);

  AMServiceBase *service = NULL;
  AMServiceManagerPtr service_manager = AMServiceManager::get_instance();
  if (service_manager) {
    state = service_manager->get_state();
    if (state != AM_SERVICE_MANAGER_RUNNING) {
      ERROR("Service_manager not running, state %d. Reject AIR API msg 0x%x\n",
            state, msg_id);
      service_result = (am_service_result_t*)result_addr;
      service_result->ret = -1; //put failed result
      return;
    }

    //find the service name based on type, and then forward the cmd
    service = service_manager->find_service(type);
    if (service) {
      service->method_call(msg_id,
                           payload, payload_size,
                           result_addr, result_max_size);
    } else {
      ERROR("Can not find matching service tyep: %d!\n", type);
    }
  }
}

BEGIN_MSG_MAP(API_PROXY)
MSG_ACTION(AM_IPC_MW_CMD_AIR_API_CONTAINER, router_api)
END_MSG_MAP()

static AMAPIProxy *m_instance = NULL;
AMAPIProxy::AMAPIProxy():
        m_air_api_ipc(NULL)
{

}

AMAPIProxy * AMAPIProxy::get_instance()
{
  if (!m_instance)
    m_instance = new AMAPIProxy();
  return m_instance;
}

int AMAPIProxy::init()
{
  //create IPC for system service connection
  m_air_api_ipc = new AMIPCSyncCmdServer();
  if (!m_air_api_ipc)
    return -1;

  if (m_air_api_ipc->create(AM_IPC_SYSTEM_NAME) < 0) {
    ERROR("ipc create failed for system service \n");
    delete m_air_api_ipc;
    return -2;
  }
  m_air_api_ipc->REGISTER_MSG_MAP(API_PROXY);
  INFO("IPC create done for API Proxy\n");
  m_air_api_ipc->complete();
  return 0;
}

AMAPIProxy::~AMAPIProxy()
{
  delete m_air_api_ipc;
  m_instance = NULL;
}
