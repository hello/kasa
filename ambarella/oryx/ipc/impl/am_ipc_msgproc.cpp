/*
 * am_ipc_msgproc.cpp
 *
 * History:
 *    2014/09/09 - [Louis Sun] Create
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
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include "am_base_include.h"
#include "am_ipc_msgproc.h"
#include "am_define.h"
#include "am_log.h"

AMIPCMsgProc::AMIPCMsgProc(void)
{}

AMIPCMsgProc::~AMIPCMsgProc(void)
{
  destroy();
}

int32_t AMIPCMsgProc::destroy(void)
{
  return 0;
}

int32_t AMIPCMsgProc::find_msg_handler(uint32_t msg_id, am_msg_handler_t *handler)
{
  for (int32_t i = 0; i < message_map_registered; i++) {
    if (message_map[i].msgid == msg_id) {
      if (handler) {
        *handler = message_map[i];
      }
      return 0;
    }
  }
  return -1;
}

//currently register_msg_proc is designed to be statically registered, so the msg/proc cannot be unregistered
//if you want to register a msgproc again to a msg id, then the new msgproc will overwrite the existing one
int32_t AMIPCMsgProc::register_msg_proc(am_msg_handler_t *handler) //Notif Receive is  Msg handler mechanism
{
  bool msg_id_registered = false;
  int32_t msg_slot = -1;
  uint32_t msg_id;

  if (!handler) {
    PRINTF("AMIPCMsgProc: Cannot register null msg proc \n");
    return -1;
  }

  msg_id = handler->msgid;

  //look up message_map to see whether the msg has been registered before
  for (int32_t i = 0; i < message_map_registered; i++) {
    if (message_map[i].msgid == msg_id) {
      WARN("overwrite existing msg proc for msgid %d, cmdId = %d, type = %d!",
           msg_id, GET_IPC_MSG_CMD_ID(msg_id), GET_IPC_MSG_TYPE(msg_id));
      msg_id_registered = true;
      msg_slot = i;
      break;
    }
  }

  if (AM_LIKELY(!msg_id_registered))    {
    if (AM_UNLIKELY(message_map_registered == MAX_MESSAGES_IN_MAP)) {
      PRINTF("too many msg proc registered \n");
      return -1;
    }
    //increase the count
    msg_slot = message_map_registered;
    message_map_registered++;
  }

  //if the handler registered is NULL, then just fill NULL handler
  message_map[msg_slot] = *handler;

  return 0;
}

// call register_msg_handler to register msg one by one.
//default implementation should register some very basic handlers already, put pure virtual function here
int32_t AMIPCMsgProc::register_msg_map(am_msg_handler_t *map, int32_t num_entry)
{
  PRINTF("AMIPCMsgProc::register_msg_map\n");
  return 0;
}

uint32_t AMIPCMsgProc::validate_msg(void *msg_buffer)
{
  return 0;
}

int32_t AMIPCMsgProc::default_msg_callback(void *msg)
{
  return 0;
}

//process msg should be overriden to process msg map
int32_t AMIPCMsgProc::process_msg(void)
{
  PRINTF("process_msg!");
  uint32_t msg_id;
  am_msg_handler_t msg_handler;
  char *receive_buffer = get_receive_buffer();

  if (AM_UNLIKELY((msg_id = validate_msg(receive_buffer)) < 0)) {
    ERROR("wrong msg!");
    return -1;
  }

  if (find_msg_handler(msg_id, &msg_handler) == 0) {
    if (msg_handler.context == 0) {
      msg_handler.callback(receive_buffer, m_receive_buffer_bytes, nullptr, 0);
    } else if (msg_handler.callback_ct != nullptr) {
      msg_handler.callback_ct(msg_handler.context, receive_buffer,
                              m_receive_buffer_bytes, nullptr, 0);
    }
    return 0;
  } else {
    PRINTF("no callback for this MSG ID in process_msg, run default!");
    default_msg_callback(receive_buffer);
    return 0;
  }
}
