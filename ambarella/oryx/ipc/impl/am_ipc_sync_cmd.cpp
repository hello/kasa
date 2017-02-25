
/*
 * am_ipc_sync_cmd.cpp
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
#include <time.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include "am_base_include.h"
#include "am_ipc_sync_cmd.h"
#include "am_define.h"
#include "am_log.h"

static inline int32_t check_identifier_name(const char *identifier_name)
{
  int32_t ret = 0;
  do {
    int32_t length;
    if (!identifier_name || (identifier_name[0] != '/')) {
      ret = -1;
      break;
    }
    length = strlen(identifier_name);
    if ((length >= AM_MAX_SEM_NAME_LENGTH - 4) || (length < 2)) {
      ret = -1;
      break;
    }
  } while (0);
  return ret;
}

/*************************************************************************

         Below is AMIPCSyncCmdClient

 **************************************************************************/

AMIPCSyncCmdClient::AMIPCSyncCmdClient()
{}

AMIPCSyncCmdClient::~AMIPCSyncCmdClient()
{
  destroy();
}

int32_t AMIPCSyncCmdClient::destroy()
{
  send_connection_state(0);

  //must call base class to destroy first, because the sem_post below will let all
  //waiting process to execute immediately. so put sem_post to be last, can avoid
  //race conditions
  AMIPCBase::destroy();

  if (m_create_sem){
    if (m_connected_flag) {
      if (sem_post(m_create_sem) < 0) {
        ERROR("sem_post error for %s!", m_sem_name);
        return -1;
      }
    }

    if (sem_close(m_create_sem) < 0) {
      ERROR("sem_close error for %s!", m_sem_name);
      return -1;
    }
  }

  //house keeping, because destroy is not deconstructor
  m_create_sem = nullptr;
  m_connected_flag = 0;
  m_sem_name[0] = '\0';
  return 0;
}

//state 0: disconnected 1 : connected()
int32_t AMIPCSyncCmdClient::send_connection_state(int32_t state)
{
  return method_call(AM_IPC_CMD_ID_CONNECTION_DONE,
                    &state, sizeof(int32_t), nullptr, 0);
}

int32_t AMIPCSyncCmdClient::create(const char *unique_identifier)
{
  int32_t ret = AM_IPC_CMD_SUCCESS;
  struct timespec wait_time;
  sem_t *p_sem = nullptr;
  bool sem_ok = false;

  do {
    if (AM_UNLIKELY(check_identifier_name(unique_identifier) < 0)) {
      ERROR("identifier name error %s!", unique_identifier);
      ret = AM_IPC_CMD_ERR_INVALID_ARG;
      break;
    }

    //retry 200 times
    for (int32_t i = 0; i < 200; ++ i) {
      p_sem = sem_open(unique_identifier, O_RDWR);
      if (p_sem == SEM_FAILED) {
        usleep(100*1000L);
      } else {
        sem_ok = true;
        break;
      }
    }

    if (AM_UNLIKELY(!sem_ok)) {
      ERROR("AMIPCSyncCmdClient: sem_open failed! (%s)!", strerror(errno));
      ERROR("AMIPCSyncCmdClient: try 100 times, but still unable to create the SEM for IPC!");
      ret = AM_IPC_CMD_ERR_INVALID_IPC;
      break;
    }

    m_create_sem = p_sem;

    strcpy(m_sem_name, unique_identifier);
    wait_time.tv_sec = time(nullptr) + AM_IPC_SYNC_CMD_MAX_DELAY;
    wait_time.tv_nsec = 0;

    if (sem_timedwait(m_create_sem, &wait_time) < 0) {
      ERROR("AMIPCSyncCmdClient:create:error on waiting for sem %s!",
            m_sem_name);
      perror("sem_timedwait \n");
      ret = AM_IPC_CMD_ERR_INVALID_IPC;
      break;
    }

    if (AM_UNLIKELY(AMIPCBase::create(nullptr, nullptr) < 0)) {
      ERROR("AMIPCSyncCmdClient:AMIPCBase:create failed!");
      ret = AM_IPC_CMD_ERR_INVALID_IPC;
      break;
    }

    if (bind(unique_identifier) < 0){
      ERROR("AMIPCSyncCmdClient:: bind failed!");
      ret = AM_IPC_CMD_ERR_INVALID_IPC;
      break;
    }

    //send connect done msg to receiver
    if (send_connection_state(1) < 0) {
      ERROR("AMIPCSyncCmdClient:: send connect done flag failed!");
      ret = AM_IPC_CMD_ERR_INVALID_IPC;
      break;
    }

    //now it's OK
    m_connected_flag  = 1;
  } while (0);

  return ret;
}

int32_t AMIPCSyncCmdClient::cleanup(const char *unique_identifier)
{
  if (unique_identifier) {
    sem_unlink(unique_identifier);
  }
  return 0;
}

int32_t AMIPCSyncCmdClient::bind(const char *unique_identifier)
{
  int32_t ret;
  char name_msg_queue[IPC_MAX_MSGQUEUE_NAME_LENGTH + 1] = {0};

  //make the msg queue names by appending 0 (for DOWN direction), and 1 (for UP direction)
  //this is receiver role,  so UP direction is for send, and DOWN direction is for receive
  sprintf(name_msg_queue, "%s0", unique_identifier);
  ret = AMIPCBase::bind_msg_queue(name_msg_queue,IPC_MSG_QUEUE_ROLE_SEND);
  if (ret < 0) {
    ERROR("AMIPCSyncCmdClient:bind msg queue %s role %d error!",
          name_msg_queue, IPC_MSG_QUEUE_ROLE_SEND);
    return -1;
  }
  sprintf(name_msg_queue, "%s1", unique_identifier);
  ret = AMIPCBase::bind_msg_queue(name_msg_queue, IPC_MSG_QUEUE_ROLE_RECEIVE);
  if (ret < 0) {
    ERROR("AMIPCSyncCmdClient:bind msg queue %s role %d error!",
          name_msg_queue, IPC_MSG_QUEUE_ROLE_RECEIVE);
    return -1;
  }

  return 0;
}

int32_t AMIPCSyncCmdClient::method_call(uint32_t cmd_id,
                                        void *cmd_arg_s,
                                        int32_t cmd_size,
                                        void *return_value_s,
                                        int32_t max_return_value_size)
{
  //AMIPCSyncCmdClient has connection state, when not connected, ignore the method_call and return faild
  //note that internal cmd IPC_CMD_ID_CONNECTION_DONE is used to setup connection, so it's handled differently
  if  (AM_LIKELY((m_connected_flag) || ( cmd_id == AM_IPC_CMD_ID_CONNECTION_DONE))) {
    return AMIPCCmdSender::method_call(cmd_id,
                                       cmd_arg_s, cmd_size,
                                       return_value_s, max_return_value_size);
  } else {
    char processname[256] = {0};
    debug_print_process_name(processname, 255);
    PRINTF("AMIPCSyncCmdClient: No active IPC connection, Ignore cmd, by Process %s tries to do method_call 0x%x while connection state is off!",
           processname, cmd_id);
    return AM_IPC_CMD_ERR_IGNORE;
  }
}

/*************************************************************************

         Below is AMIPCSyncCmdServer

 **************************************************************************/

AMIPCSyncCmdServer::AMIPCSyncCmdServer()
{}

AMIPCSyncCmdServer::~AMIPCSyncCmdServer()
{
  destroy();
}

int32_t AMIPCSyncCmdServer::destroy()
{
  m_connection_state = 0;
  AMIPCBase::destroy();

  if (m_create_sem){
    if (AM_UNLIKELY(sem_close(m_create_sem) < 0)) {
      PRINTF("sem_close error for %s \n", m_sem_name);
      return -1;
    }
  }

  //unlink it regardless if it's created by AMIPCSyncCmdServer or not
  if (AM_LIKELY(m_sem_name[0] != '\0')) {
    sem_unlink(m_sem_name);
  }

  //house keeping , because destroy is not deconstructor
  m_create_sem = nullptr;
  m_sem_name[0] = '\0';
  return 0;
}

void AMIPCSyncCmdServer::connection_callback(uint32_t context,
                                             void *msg_data,
                                             int32_t msg_data_size,
                                             void *result_addr,
                                             int32_t result_max_size)
{
  AMIPCSyncCmdServer *p_this = nullptr;

  if ((!context) || (!msg_data)) {
    ERROR("AMIPCSyncCmdServer::connection_callback error!");
  } else {
    p_this = (AMIPCSyncCmdServer*)context;
    if (p_this) {
      p_this->m_connection_state = *((int32_t*)msg_data);
    }
  }
}

int32_t AMIPCSyncCmdServer::create(const char *unique_identifier)
{
  int32_t ret = AM_IPC_CMD_SUCCESS;
  sem_t *p_sem = nullptr;
  am_msg_handler_t msg_handler;

  do {
    if (AM_UNLIKELY(check_identifier_name(unique_identifier) < 0)) {
      ERROR("identifier name error %s!", unique_identifier);
      ret = AM_IPC_CMD_ERR_INVALID_ARG;
      break;
    }

    sem_unlink(unique_identifier);
    p_sem = sem_open(unique_identifier,
                     O_RDWR | O_CREAT | O_EXCL,
                     S_IRWXU | S_IRWXG, 0);
    if (p_sem == SEM_FAILED) {
      perror("CIPCSsyncCmdReceiver create sem Error:");
      PRINTF("CIPCSsyncCmdReceiver:  %s!", unique_identifier);
      ret = AM_IPC_CMD_ERR_INVALID_IPC;
      break;
    }

    m_create_sem = p_sem;
    strcpy(m_sem_name, unique_identifier);

    std::string send_msg_queue = std::string(unique_identifier) + "1";
    std::string rece_msg_queue = std::string(unique_identifier) + "0";
    if (AMIPCBase::create(send_msg_queue.c_str(), rece_msg_queue.c_str()) < 0) {
      ERROR("AMIPCSyncCmdServer:create msg queue error!");
      ret = AM_IPC_CMD_ERR_INVALID_IPC;
      break;
    }

    msg_handler.msgid = AM_IPC_CMD_ID_CONNECTION_DONE;
    msg_handler.callback = nullptr;
    msg_handler.context =  (uint32_t)this;
    msg_handler.callback_ct = AMIPCSyncCmdServer::connection_callback;
    if (register_msg_proc(&msg_handler) < 0) {
      ret = AM_IPC_CMD_ERR_INVALID_IPC;
      break;
    }
  } while(0);

  return ret;
}

int32_t AMIPCSyncCmdServer::complete()
{
  if (AM_UNLIKELY(m_create_sem == nullptr)) {
    ERROR("AMIPCSyncCmdServer:notify_init_complete:sem not created!");
    return -1;
  }

  return sem_post(m_create_sem);
}

int32_t AMIPCSyncCmdServer::notify(uint32_t cmd_id,
                                   void *cmd_arg_s,
                                   int32_t cmd_size)
{
  if (m_connection_state) {
    return AMIPCCmdReceiver::notify(cmd_id, cmd_arg_s, cmd_size);
  } else {
    char cmdline[256];
    debug_print_process_name(cmdline, 255);
    INFO("Process %s send notification, but destination not ready, ignored!",
         cmdline);
    INFO("Warning: notify 0x%x dropped!", cmd_id);
    return AM_IPC_CMD_ERR_IGNORE;
  }
}
