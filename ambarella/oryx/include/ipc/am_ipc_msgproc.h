/*
 * am_ipc_msgproc.h
 *
 * History:
 *    2013/3/6 - [Louis Sun] Create
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

/*! @file am_ipc_msgproc.h
 *  @brief This file defines AMIPCMsgProc
 */
#ifndef  AM_IPC_MSGPROC_H__
#define  AM_IPC_MSGPROC_H__
#include "am_ipc_base.h"

typedef void (*SYS_IPC_MSG_CALLBACK)(void *msg_data,
                                     int32_t msg_data_size,
                                     void *result_addr,
                                     int32_t result_max_size);

typedef void (*SYS_IPC_MSG_CALLBACK_CT)(uintptr_t context,
                                        void *msg_data,
                                        int32_t msg_data_size,
                                        void * result_addr,
                                        int32_t result_max_size);

typedef struct am_msg_handler_s
{
    uint32_t   msgid;
    SYS_IPC_MSG_CALLBACK callback;
    uintptr_t  context;
    SYS_IPC_MSG_CALLBACK_CT callback_ct; //when context is not zero, use callback_ct instead
} am_msg_handler_t;

#define  REGISTER_MSG_MAP(map_name) \
    register_msg_map(_am_msg_action_map##map_name, \
                     (sizeof(_am_msg_action_map##map_name)/sizeof(am_msg_handler_t)));

#define  BEGIN_MSG_MAP(map_name) \
    static am_msg_handler_t  _am_msg_action_map##map_name[] = {

#define  MSG_ACTION(x, y) {x, y, 0, NULL},

#define  END_MSG_MAP() };

#define MAX_MESSAGES_IN_MAP      256

class AMIPCMsgProc : public AMIPCBase
{
  protected:
    int32_t          message_map_registered = 0;
    am_msg_handler_t message_map[MAX_MESSAGES_IN_MAP];

    uint32_t validate_msg(void *msg_buffer);
    int32_t find_msg_handler(uint32_t msg_id, am_msg_handler_t *handler);
    int32_t default_msg_callback(void *msg);

    virtual int32_t destroy();

  public:
    AMIPCMsgProc();
    virtual ~AMIPCMsgProc();

    //process msg should be overriden to process msg map
    virtual int32_t process_msg();

    virtual int32_t register_msg_proc(am_msg_handler_t *handler);
    virtual int32_t register_msg_map(am_msg_handler_t *map, int32_t num_entry);
};

#endif //  AM_IPC_MSGPROC_H__
