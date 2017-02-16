/*
 * am_ipc_msgproc.cpp
 *
 * History:
 *    2014/09/09 - [Louis Sun] Create
 *
 * Copyright (C) 2004-2013, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include "am_base_include.h"
#include "am_ipc_msgproc.h"
#include "am_define.h"
#include "am_log.h"


AMIPCMsgProc::AMIPCMsgProc(void)
{
  message_map_registered = 0;
}

AMIPCMsgProc::~AMIPCMsgProc(void)
{
  destroy();
}

int32_t AMIPCMsgProc::destroy(void)
{
  //PRINTF("AMIPCMsgProc::destroy \n");
  return 0;
}

//return msg callback from id, if not found, return NULL
/*
SYS_IPC_MSG_CALLBACK AMIPCMsgProc::find_msg_callback(uint32_t msg_id)
{
        int32_t i;

        for (i=0; i< message_map_registered; i++) {
            if (message_map[i].msgid == msg_id) {
                    return message_map[i].callback;
             }
        }
        return NULL;
}
 */

int32_t AMIPCMsgProc::find_msg_handler(uint32_t msg_id, am_msg_handler_t *handler)
{
  int32_t i;
  for (i = 0; i< message_map_registered; i++) {
    if (message_map[i].msgid == msg_id) {
      //if found it, and handler not NULL, then copy
      if (handler)  {
        *handler = message_map[i];
      }
      //if found, regardless handler NULL or not, return 0 (to indicate found )
      return 0;
    }
  }
  return -1;
}

//currently register_msg_proc is designed to be statically registered, so the msg/proc cannot be unregistered
//if you want to register a msgproc again to a msg id, then the new msgproc will overwrite the existing one
int32_t AMIPCMsgProc::register_msg_proc( am_msg_handler_t *handler) //Notif Receive is  Msg handler mechanism
{
  int32_t i;
  int32_t msg_id_registered  = 0;
  int32_t msg_slot = -1;
  uint32_t msg_id;

  if (!handler) {
    PRINTF("AMIPCMsgProc: Cannot register null msg proc \n");
    return -1;
  }

  msg_id = handler->msgid;

  //look up message_map to see whether the msg has been registered before
  for (i=0; i < message_map_registered; i++) {
    if (message_map[i].msgid == msg_id) {
      //if found, overwrite the msg handler with the new handler, so message map entry count does not change
      WARN("overwrite existing msg proc for msgid %d, cmdId= %d, type=%d \n", msg_id,
             GET_IPC_MSG_CMD_ID(msg_id),  GET_IPC_MSG_TYPE(msg_id));
      msg_id_registered = 1;
      msg_slot = i;
      break;
    }
  }

  if (AM_LIKELY(!msg_id_registered))    {
    //if no return, then not found , append it
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
  PRINTF("process_msg\n");
  uint32_t msg_id;
  am_msg_handler_t msg_handler;
  char *receive_buffer = get_receive_buffer();

  if (AM_UNLIKELY((msg_id = validate_msg(receive_buffer)) < 0)) {
    PRINTF("wrong msg \n");
    return -1;
  }

  //now process it by MSG/Action
  if (find_msg_handler(msg_id, &msg_handler) == 0 ) { // equal 0 means success/found
    if (msg_handler.context == 0) {
      msg_handler.callback (receive_buffer, m_receive_buffer_bytes, NULL, 0);
    } else if (msg_handler.callback_ct != NULL) {
      //when there is context, call with context
      msg_handler.callback_ct(msg_handler.context, receive_buffer,
                              m_receive_buffer_bytes, NULL, 0);
    } else {
      //context not NULL, and callback_ct is NULL, do nothing.
    }

    return 0;
  } else {
    PRINTF("no callback for this MSG ID in process_msg, run default \n");
    default_msg_callback(receive_buffer);
    return 0;
  }
}
