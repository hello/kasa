/*******************************************************************************
 * am_api_proxy.cpp
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

#include <signal.h>
#include <algorithm>

#include "am_base_include.h"
#include "am_log.h"
#include "am_result.h"
#include "am_service_manager.h"
#include "am_api_proxy.h"
#include "commands/am_service_impl.h"

AMAPIProxy *AMAPIProxy::m_instance = nullptr;
AMMemLock AMAPIProxy::m_lock;

void AMAPIProxy::on_static_receive_from_app(int32_t context, int fd)
{
  if (context) {
    uint8_t data[1024];
    int32_t size = sizeof(data);
    AMAPIProxy *proxy = (AMAPIProxy*)context;
    if (proxy->m_server->recv(fd, data, size) != AM_RESULT_OK) {
      ERROR("Failed to receive data from Client[%d]!", fd);
    } else {
      proxy->on_receive_from_app(fd, data, size);
    }
  }
}

void AMAPIProxy::on_receive_from_app(int fd, const uint8_t *data, int32_t size)
{
  if (!data) {
    ERROR("Data is null!");
    return;
  }
  am_ipc_message_header_t *msg_header = (am_ipc_message_header_t*)data;
  uint32_t msg_id = msg_header->msg_id;

  if ((msg_id == UINT32_MAX) && (msg_header->payload_size == 0)) {
    AUTO_MEM_LOCK(m_lock);
    m_notify_fds.push_back(fd);
    INFO("Client[%d] egister callback", fd);
    return;
  }

  void *payload = (void*)((uint8_t*)data + msg_header->header_size);
  uint32_t payload_size = msg_header->payload_size;
  AM_SERVICE_CMD_TYPE type = AM_SERVICE_CMD_TYPE(GET_IPC_MSG_TYPE(msg_id));

  AM_SERVICE_MANAGER_STATE state;
  am_service_result_t service_result = {0};
  AMServiceBase *service = nullptr;
  AMServiceManager *service_manager = AMServiceManager::get_instance();
  if (service_manager) {
    if ((state = service_manager->get_state()) != AM_SERVICE_MANAGER_RUNNING) {
      ERROR("Service_manager is not running, state: %d. Reject AIR API msg 0x%x!",
            state, msg_id);
      service_result.ret = -1;
    } else if ((service = service_manager->find_service(type))) {
      service->method_call(msg_id, payload, payload_size,
                           &service_result, sizeof(service_result));
      if (m_server->send(fd, (uint8_t*)&service_result,
                         sizeof(service_result)) != AM_RESULT_OK) {
        ERROR("Failed to send result to client: %d", fd);
      }
    } else {
      ERROR("Can't find matching service type: %d!", type);
    }
  }
}

void AMAPIProxy::on_static_client_disconnect(int32_t context, int fd)
{
  AMAPIProxy *proxy = (AMAPIProxy*)context;
  if (proxy) {
    proxy->on_client_disconnect(fd);
  }
}

void AMAPIProxy::on_client_disconnect(int fd)
{
  INFO("Client[%d] is disconnected!", fd);
  AUTO_MEM_LOCK(m_lock);
  auto it = std::find_if(m_notify_fds.begin(), m_notify_fds.end(),
                           [=](int f) -> bool {return (f == fd);});
  if (it != m_notify_fds.end()) {
    m_notify_fds.erase(it);
  }
}

AMAPIProxyPtr AMAPIProxy::get_instance()
{
  AUTO_MEM_LOCK(m_lock);
  if (!m_instance) {
    if ((m_instance = new AMAPIProxy()) &&
        (m_instance->construct() != AM_RESULT_OK)) {
      delete m_instance;
      m_instance = nullptr;
    }
  }
  return AMAPIProxyPtr(m_instance,
                        [](AMAPIProxy *p){delete p;});
}

int AMAPIProxy::register_notify_cb(AM_SERVICE_CMD_TYPE type)
{
  do {
    AMServiceManagerPtr service_manager = AMServiceManager::get_instance();
    if (service_manager) {
      AMServiceBase *service = service_manager->find_service(type);
      if (service) {
        service->register_svc_notify_cb(this, on_notify);
      }
    }
  } while (0);
  return 0;
}

int AMAPIProxy::on_notify(void *ptr, const void *msg_data, int msg_data_size)
{
  int ret = 0;
  do {
    if (!ptr) {
      ret = -1;
      break;
    }
    AMAPIProxy *proxy = (AMAPIProxy*)ptr;
    AUTO_MEM_LOCK(m_lock);
    for (auto &v : proxy->m_notify_fds) {
      proxy->m_server->send(v, (uint8_t*)msg_data, msg_data_size);
    }
  } while (0);
  return ret;
}

AM_RESULT AMAPIProxy::construct()
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    signal(SIGPIPE, SIG_IGN);
    if (!(m_server = AMIPCServerUDS::create("/tmp/API_PROXY.domain"))) {
      result = AM_RESULT_ERR_IO;
      ERROR("Failed to create UNIX domain soket server!");
      break;
    }
    m_server->register_receive_cb(on_static_receive_from_app, (int32_t)this);
    m_server->register_disconnect_cb(on_static_client_disconnect,
                                     (int32_t)this);
  } while (0);
  return result;
}

AMAPIProxy::AMAPIProxy()
{
}

AMAPIProxy::~AMAPIProxy()
{
  m_instance = nullptr;
}
