/*
 * ipc_cmd.h
 *
 * History:
 *    2013/2/22 - [Louis Sun] Create
 *
 * Copyright (C) 2004-2013, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

/*! @file am_ipc_cmd.h
 *  @brief This file defines AMIPCCmdSender and AMIPCCmdReceiver
 *
 *  By CMD/RESULT pair design principal, it's required for CMD/RESULT message
 *  queue to use msg queue name in pair. If message queue should be /msgqueue,
 *  then CMD_SEND msg queue name will be /msgqueue0, and RESULT_RETURN msg queue
 *  name will be /msgqueue1 the role of CmdSender and CmdReceiver are not
 *  exchangeable
 */
#ifndef  AM_IPC_CMD_H__
#define  AM_IPC_CMD_H__
#include "am_ipc_msgproc.h"

#define AM_MAX_CMD_PARAM_SIZE \
(AM_MAX_IPC_MESSAGE_SIZE - sizeof(am_ipc_message_header_t))

#define AM_MAX_CMD_RESULT_SIZE \
(AM_MAX_IPC_MESSAGE_SIZE - sizeof(am_ipc_message_header_t))

#include <semaphore.h>

#define ERROR_NOT_SUPPORTED_CMD     (-10)

class AMIPCCmdSender: public AMIPCMsgProc
{
  public:
    AMIPCCmdSender();
    virtual ~AMIPCCmdSender();
    //process msg should be overriden to process msg map
    virtual int32_t process_msg();
    //virtual int32_t register_msg_proc( am_msg_handler_t handler) ;
    virtual int32_t register_msg_map(am_msg_handler_t *map, int32_t num_entry);

    //blocking call to run method call
    virtual int32_t method_call(uint32_t cmd_id,
                            void *cmd_arg_s,
                            int32_t cmd_size,
                            void *return_value_s,
                            int32_t max_return_value_size);
  protected:
    int32_t no_return_cmd_proc(am_ipc_message_t *ipc_cmd);
    virtual int32_t destroy();

  protected:
    sem_t cmd_sem;
    am_ipc_message_t *ipc_cmd;
    am_ipc_message_t *ipc_cmd_return;
};

class AMIPCCmdReceiver: public AMIPCMsgProc
{
  public:

    AMIPCCmdReceiver();
    virtual ~AMIPCCmdReceiver();

    virtual int32_t process_msg();
    //virtual int32_t register_msg_proc( am_msg_handler_t handler) ;
    virtual int32_t register_msg_map(am_msg_handler_t *map, int32_t num_entry);
    virtual int32_t notify(uint32_t cmd_id, void *cmd_arg_s , int32_t cmd_size);

  protected:
    int32_t process_group_cmd();
    virtual int32_t destroy();

  protected:
    uint32_t last_cmd_id;
    uint8_t last_cmd_param_data[AM_MAX_CMD_PARAM_SIZE];
    uint8_t last_cmd_result_data[AM_MAX_CMD_RESULT_SIZE];
    am_ipc_message_t *ipc_cmd;
    am_ipc_message_t *ipc_cmd_return;
};

#endif //AM_IPC_CMD_H__
