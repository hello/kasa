/*
 * ipc_base.h
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

/*! @file am_ipc_base.h
 *  @brief This file defines AMIPCBase.
 */

#ifndef AM_IPC_BASE_H__
#define  AM_IPC_BASE_H__

#include <mqueue.h>
#include <stdint.h>
#include "am_ipc_types.h"

#define IPC_MAX_MESSAGES 5
/*! 1KB is max size of POSIX message queue supported by Linux */
#define IPC_MESSAGE_SIZE AM_MAX_IPC_MESSAGE_SIZE
#define IPC_MAX_MSGQUEUE_NAME_LENGTH 256

/*! @enum AM_IPC_MSG_QUEUE_ROLE
 *  @brief Define IPC message queue role type.
 */
enum AM_IPC_MSG_QUEUE_ROLE
{
  IPC_MSG_QUEUE_ROLE_SEND = 0,   //!< Sender
  IPC_MSG_QUEUE_ROLE_RECEIVE = 1,//!< Receiver
};

/*! @typedef THREADFUNC
 */
typedef void (*THREADFUNC)(sigval_t sv);

struct am_msg_handler_s;
class AMIPCBase
{
  public:
    AMIPCBase();
    virtual ~AMIPCBase();
    /*! Create connections
     * each AMIPCBase object can choose to either create with msg queue,
     * or bind to an existing msg queue to use or create one and bind another.
     * ipcbase may choose to create both msgqueue name.
     * @param send_msgqueue_name
     * @param receive_msgqueue_name
     * @return 0 on success, -1 on failure.
     */
    virtual int32_t create(const char *send_msgqueue_name,
                           const char *receive_msgqueue_name);
    /*! Build connection with a msg queue by name.
     * @param msg_queue_name C style string indicates the message queue name.
     * @param role - 0: send - 1: receive
     * @return 0 on success, -1 on failure.
     */
    virtual int32_t bind_msg_queue(char *msg_queue_name, int32_t role);

    /*! Return receive MSG queue.
     * @param msg_queue_role IPC message queue role,
     * see @ref AM_IPC_MSG_QUEUE_ROLE for message queue role definition.
     * @return message queue fd, or -1 if no fd.
     */
    virtual mqd_t get_msg_queue(int32_t msg_queue_role);

    /*! Send binary data including both header and possible payload
     * @param data binary data
     * @param length data length
     * @return 0 on success, -1 on failure.
     */
    virtual int32_t send(void *data, int32_t length);

    /*! if registered, it will use ASYNC NOTIFICATION protocol,
     *  otherwise, the user needs to poll or select.
     * @return 0 on success, value less than 0 indicates an error.
     */
    virtual int32_t register_receive_callback();

    /*!
     * receive all of the msgs in queue, and process one by one
     * @return
     *         -  0: success,
     *         - -1: receive queue is not setup
     *         - -2: receive buffer is invalid
     *         - -3: error occurred during the process of receiving message
     *         - -4: mq_receive error occurred
     *         - -5: object is destroyed
     */
    virtual int32_t receive();

    /*!
     * Retrieve error from last communication
     * @return the error code
     */
    virtual int32_t get_last_error();

    /*!
     * called by receive , never directly be called by user
     * @return 0
     */
    virtual int32_t process_msg();

    /*! Notify Receive is MSG handler mechanism
     * @param handler am_msg_handler_s type
     * @return 0
     */
    virtual int32_t register_msg_proc( struct am_msg_handler_s *handler)
    { return 0; };

  protected:
    //not directly used by user
    virtual int32_t create_msg_queue(const char *msg_queue_name, int32_t role);
    virtual int32_t destroy_msg_queue(int32_t role);

    virtual int32_t destroy();

    const static int32_t max_messages = IPC_MAX_MESSAGES;
    const static int32_t max_message_size = IPC_MESSAGE_SIZE;
    const static int32_t max_message_queue_name = IPC_MAX_MSGQUEUE_NAME_LENGTH;
    virtual char* get_receive_buffer();
    void dump_receive_buffer();
    int32_t register_receive_callback_first_time(char *mq_name);
    int32_t debug_print_queue_info();

  protected:
    //use two msg queues for two roles,
    //0 is for role "send", 1 is for role "receive"
    mqd_t m_msg_queue[2] = {-1, -1};
    char *m_msg_queue_name[2] = {nullptr, nullptr};

    char *m_receive_buffer = nullptr;
    int32_t m_receive_buffer_bytes = 0;
    int32_t m_priority;
    int32_t m_error_code = 0;

  private:
    int32_t m_receive_notif_setup = 0;
    int32_t m_create_flag = 0;
};

int32_t debug_print_process_name(char *cmdline, int32_t max_size);

#endif  //AM_IPC_BASE_H__
