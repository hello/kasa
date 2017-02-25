/*
 * ipc_cmd.h
 *
 * History:
 *    2013/2/22 - [Louis Sun] Create
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

#define ERROR_NOT_SUPPORTED_CMD (-10)

class AMIPCCmdSender: public AMIPCMsgProc
{
  public:
    AMIPCCmdSender();
    virtual ~AMIPCCmdSender();
    //process msg should be overriden to process msg map
    virtual int32_t process_msg();
    virtual int32_t register_msg_map(am_msg_handler_t *map, int32_t num_entry);

    virtual int32_t method_call(uint32_t cmd_id,
                                void *cmd_arg_s,
                                int32_t cmd_size,
                                void *return_value_s,
                                int32_t max_return_value_size);
  protected:
    int32_t no_return_cmd_proc(am_ipc_message_t *ipc_cmd);
    virtual int32_t destroy();

  protected:
    am_ipc_message_t *ipc_cmd = nullptr;
    am_ipc_message_t *ipc_cmd_return = nullptr;
    pthread_mutex_t mc_mutex;
    sem_t cmd_sem;
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
    am_ipc_message_t *ipc_cmd = nullptr;
    am_ipc_message_t *ipc_cmd_return = nullptr;
    uint32_t last_cmd_id = 0;
    uint8_t last_cmd_param_data[AM_MAX_CMD_PARAM_SIZE];
    uint8_t last_cmd_result_data[AM_MAX_CMD_RESULT_SIZE];
};

#endif //AM_IPC_CMD_H__
