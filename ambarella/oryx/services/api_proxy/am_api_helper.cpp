/*******************************************************************************
 * am_api_helper.cpp
 *
 * History:
 *   Jul 29, 2016 - [Shupeng Ren] created file
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
 *
 ******************************************************************************/
#include "am_base_include.h"

#include "am_log.h"
#include "am_ipc_types.h"
#include "am_api_helper.h"

AMMemLock AMAPIHelper::m_lock;
AMAPIHelper *AMAPIHelper::m_instance = nullptr;

#define API_PROXY_UNIX_PATH "/tmp/API_PROXY.domain"
#define API_HELPER_SEM_PATH "/api_helper_uds"
#define SEMAPHORE_WAIT_TIME 20

AMAPIHelperPtr AMAPIHelper::get_instance()
{
  AUTO_MEM_LOCK(m_lock);
  if (!m_instance) {
    if ((m_instance = new AMAPIHelper()) &&
        (m_instance->construct() != AM_RESULT_OK)) {
      delete m_instance;
      m_instance = nullptr;
    }
  }
  return AMAPIHelperPtr(m_instance,
                        [](AMAPIHelper *p){delete p;});
}

void AMAPIHelper::method_call(uint32_t cmd_id,
                              void *msg_data,
                              int msg_data_size,
                              am_service_result_t *result_addr,
                              int result_max_size)
{
  //AUTO_MEM_LOCK(m_lock);
  uint8_t cmd_buf[AM_MAX_IPC_MESSAGE_SIZE] = {0};
  am_ipc_message_header_t msg_header = {0};
  timespec wait_time = {time(nullptr) + SEMAPHORE_WAIT_TIME, 0};

  do {
    if (sem_timedwait(m_sem, &wait_time) < 0) {
      result_addr->ret = -1;
      PERROR("sem_timedwait:");
      if (errno == ETIMEDOUT) {
        ERROR("method call time out, please try again!");
      }
      break;
    }

    if (msg_data_size + sizeof(am_ipc_message_header_t) >
        AM_MAX_IPC_MESSAGE_SIZE) {
      ERROR("unable to pack cmd 0x%x into AIR API container, total size %d\n",
            cmd_id, msg_data_size + sizeof(am_ipc_message_header_t));
      result_addr->ret = -1;
      break;
    }

    msg_header.msg_id = cmd_id;
    msg_header.header_size = sizeof(msg_header);
    msg_header.payload_size = msg_data_size;
    memcpy(cmd_buf, &msg_header, sizeof(msg_header));
    memcpy(cmd_buf + sizeof(msg_header), msg_data, msg_data_size);
    int send_size = sizeof(msg_header) + msg_data_size;
    if (m_client->send(cmd_buf, send_size) != AM_RESULT_OK) {
      result_addr->ret = -2;
      ERROR("Failed to send cmd to server!");
      break;
    }
    if (m_client->recv((uint8_t*)result_addr,
                       result_max_size) != AM_RESULT_OK) {
      result_addr->ret = -3;
      ERROR("Failed to receive result from server!");
      break;
    }
  } while (0);
  int sem_value = 0;
  sem_getvalue(m_sem, &sem_value);
  if (sem_value == 0) {
    sem_post(m_sem);
  }
}

int AMAPIHelper::register_notify_cb(AM_IPC_NOTIFY_CB cb)
{
  int ret = 0;
  do {
    if (cb) {
      if (m_client_notify->connect(API_PROXY_UNIX_PATH) != AM_RESULT_OK) {
        ret = -1;
        ERROR("Failed to connect to server!");
        break;
      } else {
        m_notify_cb = cb;
        m_client_notify->register_receive_cb(on_notify_callback,
                                             (uint32_t)this);

        am_ipc_message_header_t header = {0};
        header.header_size = sizeof(header);
        header.msg_id = UINT32_MAX;
        header.payload_size = 0;
        if (m_client_notify->send((uint8_t*)&header,
                                  sizeof(header)) != AM_RESULT_OK) {
          ret = -2;
          ERROR("Failed to send header to proxy!");
          break;
        }
      }
    } else {
      m_client_notify->distconnect();
      m_notify_cb = nullptr;
    }
  } while (0);
  return ret;
}

void AMAPIHelper::on_notify_callback(int32_t context, int fd)
{
  if (context) {
    AMAPIHelper *helper = (AMAPIHelper*)context;
    do {
      uint8_t data[1024];
      int32_t size = sizeof(data);
      helper->m_client_notify->recv(data, size);
      if (helper->m_notify_cb) {
        helper->m_notify_cb(data, size);
      }
    } while (0);
  }
}

AM_RESULT AMAPIHelper::construct()
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    if ((m_sem = sem_open(API_HELPER_SEM_PATH,
                          O_RDWR | O_CREAT,
                          S_IRWXU | S_IRWXG, 1)) == SEM_FAILED) {
      result = AM_RESULT_ERR_IO;
      PERROR("sem_open:");
      break;
    }

    if (!(m_client = AMIPCClientUDS::create())) {
      result = AM_RESULT_ERR_MEM;
      ERROR("Failed to create AMIPCClientUDS!");
      break;
    }

    if (!(m_client_notify = AMIPCClientUDS::create())) {
      result = AM_RESULT_ERR_MEM;
      ERROR("Failed to create notify AMIPCClientUDS!");
      break;
    }

    if (m_client->connect(API_PROXY_UNIX_PATH) != AM_RESULT_OK) {
      result =  AM_RESULT_ERR_IO;
      ERROR("Failed to connect to server!");
      break;
    }
  } while (0);
  return result;
}

AMAPIHelper::AMAPIHelper()
{
}

AMAPIHelper::~AMAPIHelper()
{
  m_client->distconnect();
  sem_close(m_sem);
  m_instance = nullptr;
}
