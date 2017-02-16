/*******************************************************************************
 * am_api_helper.cpp
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
#include "am_base_include.h"
#include "am_log.h"

#include "am_api_helper.h"
#include "am_ipc_sync_cmd.h"

#include <mutex>
#include <signal.h>

#include "commands/am_service_impl.h"

AMAPIHelper * AMAPIHelper::m_instance = NULL;

static std::mutex m_mtx;
#define  DECLARE_MUTEX  std::lock_guard<std::mutex> lck (m_mtx);

AMAPIHelper::AMAPIHelper() :
    m_air_api_ipc(NULL),
    m_ref_counter(0)
{

}

int AMAPIHelper::construct()
{
  int ret = 0;
  do {
    m_air_api_ipc = new AMIPCSyncCmdClient();
    if (!m_air_api_ipc) {
      ret = -1;
      ERROR("fail to create air api ipc\n");
      break;
    } else {
      if (m_air_api_ipc->create(AM_IPC_SYSTEM_NAME) < 0) {
        ret = -2;
        ERROR("fail to create air api ipc \n");
        break;
      }
    }
  } while (0);

  if (ret < 0) {
    delete m_air_api_ipc;
    m_air_api_ipc = NULL;
  }

  return ret;
}

AMAPIHelperPtr AMAPIHelper::get_instance()
{
  DECLARE_MUTEX;
  if (!m_instance) {
    if ((m_instance = new AMAPIHelper()) && m_instance->construct() > 0) {
      ERROR("AMAPIHelper: construct error\n");
      delete m_instance;
      m_instance = nullptr;
    }
  }

  return m_instance;
}

void AMAPIHelper::release()
{
  DECLARE_MUTEX;
  if((m_ref_counter) > 0 && (--m_ref_counter == 0)) {
    delete m_instance;
    m_instance = nullptr;
  }
}

void AMAPIHelper::inc_ref()
{
  ++ m_ref_counter;
}

AMAPIHelper::~AMAPIHelper()
{
  delete m_air_api_ipc;
}

void AMAPIHelper::method_call(uint32_t cmd_id,
                              void *msg_data,
                              int msg_data_size,
                              void *result_addr,
                              int result_max_size)
{
  DECLARE_MUTEX;
  uint8_t cmd_buf[AM_MAX_IPC_MESSAGE_SIZE] = {0};
  am_ipc_message_header_t msg_header;
  if (!m_air_api_ipc) {
    ERROR("air api not setup, cannot do method call\n");
    return;
  }

  //pack the cmd into uniform method call

  if (msg_data_size + sizeof(am_ipc_message_header_t) > AM_MAX_IPC_MESSAGE_SIZE) {
    ERROR("unable to pack cmd 0x%x into AIR API container, total size %d\n",
        cmd_id, msg_data_size + sizeof(am_ipc_message_header_t));
    return;
  }

  memset(&msg_header, 0, sizeof(msg_header));
  msg_header.msg_id = cmd_id;
  msg_header.header_size = sizeof(msg_header);
  msg_header.payload_size = msg_data_size;
  //msg_header.time_stamp =
  memcpy(cmd_buf, &msg_header, sizeof(msg_header));//copy cmd header
  memcpy(cmd_buf + sizeof(msg_header), msg_data, msg_data_size);//copy cmd payload

  //call universal cmd that can pack the original cmd into AM_IPC_MW_CMD_AIR_API_CONTAINER
  m_air_api_ipc->method_call(AM_IPC_MW_CMD_AIR_API_CONTAINER,
      cmd_buf, sizeof(msg_header) + msg_data_size,
      result_addr, result_max_size);
}

