/*
 * am_ipc_msgproc.h
 *
 * History:
 *    2013/3/6 - [Louis Sun] Create
 *
 * Copyright (C) 2004-2013, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

/*! @file am_ipc_msgproc.h
 *  @brief This file defines AMIPCMsgProc
 */
#ifndef  AM_IPC_MSGPROC_H__
#define  AM_IPC_MSGPROC_H__
#include "am_ipc_base.h"

typedef void (*SYS_IPC_MSG_CALLBACK)(void *msg_data, int32_t msg_data_size, void *result_addr,  int32_t result_max_size);
//the implementation code will put this pointer to be cb_context

typedef void (*SYS_IPC_MSG_CALLBACK_CT)(uint32_t context, void *msg_data, int32_t msg_data_size, void * result_addr,  int32_t result_max_size);
//the implementation code will put this pointer to be cb_context

typedef struct am_msg_handler_s
{
    uint32_t   msgid;
    SYS_IPC_MSG_CALLBACK callback;
    uint32_t   context;
    SYS_IPC_MSG_CALLBACK_CT callback_ct;       //when context is not zero, use callback_ct instead
}am_msg_handler_t;

#define  REGISTER_MSG_MAP(map_name)             \
    register_msg_map(_am_msg_action_map##map_name,  \
                     (sizeof(_am_msg_action_map##map_name )/sizeof(am_msg_handler_t)));

#define  BEGIN_MSG_MAP(map_name)                \
    static am_msg_handler_t  _am_msg_action_map##map_name[] = {

#define  MSG_ACTION(x, y) {x, y, 0, NULL},

#define  END_MSG_MAP() };


#define MAX_MESSAGES_IN_MAP      256

class AMIPCMsgProc : public AMIPCBase
{
  protected:
    am_msg_handler_t message_map[MAX_MESSAGES_IN_MAP];
    int32_t           message_map_registered;

    uint32_t validate_msg(void *msg_buffer);
    //SYS_IPC_MSG_CALLBACK find_msg_callback(uint32_t msg_id);
    int32_t find_msg_handler(uint32_t msg_id,  am_msg_handler_t *handler);
    int32_t default_msg_callback(void *msg);

    virtual int32_t destroy();

  public:
    AMIPCMsgProc();
    virtual ~AMIPCMsgProc();

    //process msg should be overriden to process msg map
    virtual int32_t process_msg();

    virtual int32_t register_msg_proc(am_msg_handler_t *handler); //Notif Receive is  Msg handler mechanism

    // call register_msg_handler to register msg one by one.
    //default implementation should register some very basic handlers already, put pure virtual function here
    virtual int32_t register_msg_map(am_msg_handler_t *map, int32_t num_entry);
};

#endif //  AM_IPC_MSGPROC_H__
