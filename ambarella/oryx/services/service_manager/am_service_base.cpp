/*******************************************************************************
 * am_service_base.cpp
 *
 * History:
 *   2014-9-12 - [lysun] created file
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
#include "am_define.h"
#include "am_ipc_sync_cmd.h"
#include "am_service_base.h"
#include "am_service_manager.h"

#include <sys/types.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <signal.h>

static std::mutex m_mtx;
#define  DECLARE_MUTEX   std::lock_guard<std::mutex> lck (m_mtx);

#define  WORKQ_CMD_POST   'p'
#define  WORKQ_CMD_FLUSH  'f'
#define  WORKQ_CMD_QUIT   'q'

AMServiceBase::AMServiceBase(const char *service_name,
                             const char *full_pathname,
                             AM_SERVICE_CMD_TYPE type) :
  m_type(type),
  m_name(service_name),
  m_exe_filename(full_pathname)
{
}

AMServiceBase::~AMServiceBase()
{
  if (m_state != AM_SERVICE_STATE_NOT_INIT) {
    destroy();
  }
}

const char* AMServiceBase::get_name()
{
  return m_name.empty() ? ((const char*)"Unknown") : m_name.c_str();
}

AM_SERVICE_CMD_TYPE AMServiceBase::get_type()
{
  return m_type;
}

int AMServiceBase::init()
{
  DECLARE_MUTEX
  int ret = 0;
  am_service_result_t service_result;
  do {
    if (m_exe_filename.empty()) {
      ERROR("AMServiceBase, empty exe filename\n");
      ret = -1;
      break;
    }

    if (m_name.empty()) {
      ERROR("AMServiceBase, empty service name\n");
      ret = -3;
      break;
    }

    if (cleanup_ipc() < 0) {
      ERROR("AMServiceBase, unable to cleanup ipc \n");
      ret = -6;
      break;
    }

    if (create_process() < 0) {
      ERROR("AMServiceBase, unable to create process %s\n",
            m_exe_filename.c_str());
      ret = -2;
      break;
    }

    if (create_ipc() < 0) {
      ERROR("AMServiceBase, unable to create ipc\n");
      ret = -4;
      break;
    }

    if (workq_create() < 0) {
      ERROR("AMServiceBase, unable to create workq\n");
      ret = -5;
      break;
    }

    do {
      //call init again if init result is not ready,
      //until init ready, then break at INIT_DONE
      if (m_ipc->method_call(AM_IPC_SERVICE_INIT,
                             nullptr,
                             0,
                             &service_result,
                             sizeof(service_result)) < 0) {
        ERROR("AMServiceBase, ipc call SERVICE INIT error!");
        ret = -5;
        break;
      }

      DEBUG("AMServiceBase,  method_call AM_IPC_SERVICE_INIT returns\n");
      if (service_result.ret < 0) {
        ERROR("AMService %s found init error from result %d\n",
              m_name.c_str(), service_result.ret);
        ret = -6;
        break;
      }
      if (service_result.state != AM_SERVICE_STATE_INIT_DONE
          && service_result.state != AM_SERVICE_STATE_STARTED) {
        INFO("AMServiceBase: check service %s state not done\n",
             m_name.c_str());
        usleep(20 * 1000);
      } else {
        m_state = AM_SERVICE_STATE_INIT_DONE;
        break;
      }
    } while (1);
  } while (0);

  //init process is very special, must use method call to query init
  //until it is init done. cannot declare here

  return ret;
}

int AMServiceBase::destroy()
{
  DECLARE_MUTEX
  int ret = 0;
  am_service_result_t service_result;
  do {
    if (!m_ipc) {
      ERROR("AMServiceBase: destroy, ipc not setup yet\n ");
      ret = -1;
      break;
    }

    //call destroy function
    if (m_ipc->method_call(AM_IPC_SERVICE_DESTROY,
                           nullptr,
                           0,
                           &service_result,
                           sizeof(service_result)) < 0) {
      ERROR("AMServiceBase, ipc call SERVICE DESTROY error \n");
      ret = -5;
      break;
    }

    if (service_result.ret < 0) {
      ERROR("AMService %s found destroy error from result %d\n",
            service_result.ret);
      ret = -6;
      break;
    }

    workq_destroy();

    delete m_ipc;
    m_ipc = nullptr;

    //Hopefully, the destroy IPC will let child process to quit,
    //and we have blocked signals, so child process usually won't quit by signal
    //now let's wait for child to exit
    if (wait_process() < 0) {
      ret = -7;
      break;
    }
  } while (0);

  m_state = AM_SERVICE_STATE_NOT_INIT;
  return ret;
}

int AMServiceBase::start()
{
  DECLARE_MUTEX
  int ret = 0;
  am_service_result_t service_result;
  DEBUG("AMServiceBase:start\n");
  do {
    if (!m_ipc) {
      ERROR("AMServiceBase: IPC not created, not ready\n");
      ret = -1;
      break;
    }

    if ((m_state != AM_SERVICE_STATE_INIT_DONE)
        && (m_state != AM_SERVICE_STATE_STOPPED)) {
      ERROR("AMServiceBase: state not ready to start %d\n", m_state);
      ret = -2;
      break;
    }

    if (m_ipc->method_call(AM_IPC_SERVICE_START,
                           nullptr,
                           0,
                           &service_result,
                           sizeof(service_result)) < 0) {
      ERROR("AMServiceBase, ipc call SERVICE START error \n");
      ret = -3;
      break;
    }

    if (service_result.ret < 0) {
      ERROR("AMServiceBase, ipc call SERVICE START return value %d \n",
            service_result.ret);
      ret = -4;
      break;
    }

    m_state = AM_SERVICE_STATE_STARTED;
  } while (0);
  return ret;
}

int AMServiceBase::stop()
{
  DECLARE_MUTEX
  int ret = 0;
  am_service_result_t service_result;
  DEBUG("AMServiceBase:stop\n");
  do {
    if (!m_ipc) {
      ERROR("AMServiceBase: IPC not created, not ready\n");
      ret = -1;
      break;
    }

    if ((m_state == AM_SERVICE_STATE_INIT_DONE)
        || (m_state == AM_SERVICE_STATE_STOPPED)) {
      INFO("AMServiceBase: no need to stop, state is %d\n", m_state);
    } else if (m_state != AM_SERVICE_STATE_STARTED) {
      ERROR("AMServiceBase: state cannot stop %d\n", m_state);
      ret = -2;
      break;
    }
    if (m_ipc->method_call(AM_IPC_SERVICE_STOP,
                           nullptr,
                           0,
                           &service_result,
                           sizeof(service_result)) < 0) {
      ERROR("AMServiceBase, ipc call SERVICE STOP error \n");
      ret = -3;
      break;
    }
    if (service_result.ret < 0) {
      ERROR("AMServiceBase, ipc call SERVICE STOP return value %d \n",
            service_result.ret);
      ret = -4;
      break;
    }

    m_state = AM_SERVICE_STATE_STOPPED;
  } while (0);
  return ret;
}

int AMServiceBase::restart()
{
  DECLARE_MUTEX
  int ret = 0;
  am_service_result_t service_result;
  INFO("AMServiceBase:restart\n");
  do {
    if (!m_ipc) {
      ERROR("AMServiceBase: IPC not created, not ready\n");
      ret = -1;
      break;
    }

    if ((m_state != AM_SERVICE_STATE_INIT_DONE)
        && (m_state != AM_SERVICE_STATE_STOPPED)) {
      ERROR("AMServiceBase: not ready to start from state%d\n", m_state);
      ret = -2;
      break;
    }

    PRINTF("try to restart from state %d\n", m_state);
    if (m_ipc->method_call(AM_IPC_SERVICE_RESTART,
                           nullptr,
                           0,
                           &service_result,
                           sizeof(service_result)) < 0) {
      ERROR("AMServiceBase, ipc call SERVICE RESTART error \n");
      ret = -3;
      break;
    }

    if (service_result.ret < 0) {
      ERROR("AMServiceBase, ipc call SERVICE RESTART return value %d \n",
            service_result.ret);
      ret = -4;
      break;
    }

    m_state = AM_SERVICE_STATE_STARTED;
  } while (0);
  return ret;
}

int AMServiceBase::status(AM_SERVICE_STATE *state)
{
  DECLARE_MUTEX
  int ret = 0;
  am_service_result_t service_result;
  INFO("AMServiceBase:status\n");
  do {
    if (!m_ipc) {
      ERROR("AMServiceBase: IPC not created, not ready\n");
      ret = -1;
      break;
    }

    if (m_ipc->method_call(AM_IPC_SERVICE_STATUS,
                           nullptr,
                           0,
                           &service_result,
                           sizeof(service_result)) < 0) {
      ERROR("AMServiceBase, ipc call SERVICE STATUS error \n");
      ret = -3;
      break;
    }

    if (service_result.ret < 0) {
      ERROR("AMServiceBase, ipc call SERVICE STATUS return value %d \n",
            service_result.ret);
      ret = -4;
      break;
    }

    PRINTF("AMServiceBase: service %s query state is %d, local state is %d\n",
           m_name.c_str(),
           service_result.state,
           m_state);

  } while (0);
  return ret;
}

void AMServiceBase::static_workq_thread(void *data)
{
  ((AMServiceBase*)data)->workq_thread();
}

void AMServiceBase::static_on_notif_callback(uintptr_t  context,
                                             void      *msg_data,
                                             int        msg_data_size,
                                             void      *result_addr,
                                             int        result_max_size)
{
  ((AMServiceBase*)context)->on_notif_callback(msg_data,
                                               msg_data_size,
                                               result_addr,
                                               result_max_size);
}

void AMServiceBase::workq_thread()
{
  fd_set allset;
  fd_set fdset;

  int maxfd = WORKQ_CTRL_READ;
  FD_ZERO(&allset);
  FD_SET(WORKQ_CTRL_READ, &allset);

  while (m_workq_loop) {
    int ret_val = -1;
    fdset = allset;
    if ((ret_val = select(maxfd + 1 , &fdset, nullptr, nullptr, nullptr)) > 0) {
      if (FD_ISSET(WORKQ_CTRL_READ, &fdset)) {
        char      cmd[1] = {0};
        uint32_t read_cnt = 0;
        ssize_t read_ret = 0;
        workq_ctx_t ctx;
        do {
          read_ret = read(WORKQ_CTRL_READ, &cmd, sizeof(cmd));
        } while ((++ read_cnt < 5) && ((read_ret == 0) || ((read_ret < 0) &&
                 ((errno == EAGAIN) || (errno == EWOULDBLOCK) ||
                 (errno == EINTR)))));
        if (AM_UNLIKELY(read_ret <= 0)) {
          PERROR("read");
        } else if (cmd[0] == WORKQ_CMD_POST) {
          if (!m_workq_ctx_q.empty()) {
            ctx = m_workq_ctx_q.front();
            m_workq_ctx_q.pop_front();
            if (workq_task_exec(&ctx, sizeof(ctx)) < 0) {
              ERROR("workq_task_exec failed");
            }
          } else {
            WARN("m_workq_ctx_list is empty");
          }
        } else if (cmd[0] == WORKQ_CMD_FLUSH) {
          while (!m_workq_ctx_q.empty()) {
            ctx = m_workq_ctx_q.front();
            m_workq_ctx_q.pop_front();
            if (workq_task_exec(&ctx, sizeof(ctx)) < 0) {
              ERROR("workq_task_exec failed");
            }
          }
        } else if (cmd[0] == WORKQ_CMD_QUIT) {
          m_workq_loop = false;
        } else {
          ERROR("workq cmd unknown");
        }
      }
    } else {
      PERROR("select");
    }
  }
}

int AMServiceBase::workq_create()
{
  int ret = 0;
  do {
    if (pipe2(m_workq_ctrl_fd, O_CLOEXEC) < 0) {
      PERROR("pipe2");
      ret = -1;
      break;
    }
    char thread_name[32] = {0};
    snprintf(thread_name, sizeof(thread_name), "Notify.%-8s", m_name.c_str());
    m_workq_loop = true;
    m_workq_thread = AMThread::create(thread_name,
                                      static_workq_thread,
                                      this);
    if (!m_workq_thread) {
      ERROR("create workq_thread failed");
      ret = -1;
      break;
    }
  } while (0);
  return ret;
}

void AMServiceBase::workq_destroy()
{
  workq_task_flush();
  workq_task_quit();
  AM_DESTROY(m_workq_thread);
}

int AMServiceBase::workq_task_flush()
{
  int ret = 0;
  char cmd[1] = {WORKQ_CMD_FLUSH};
  int count = 0;
  while ((++ count < 5) && (1 != write(WORKQ_CTRL_WRITE, cmd, 1))) {
    if (AM_LIKELY((errno != EAGAIN) &&
                  (errno != EWOULDBLOCK) &&
                  (errno != EINTR))) {
      PERROR("write");
      ret = -1;
      break;
    }
  }
  return ret;
}

int AMServiceBase::workq_task_quit()
{
  int ret = 0;
  char cmd[1] = {WORKQ_CMD_QUIT};
  int count = 0;
  while ((++ count < 5) && (1 != write(WORKQ_CTRL_WRITE, cmd, 1))) {
    if (AM_LIKELY((errno != EAGAIN) &&
                  (errno != EWOULDBLOCK) &&
                  (errno != EINTR))) {
      PERROR("write");
      ret = -1;
      break;
    }
  }
  return ret;
}

int AMServiceBase::workq_task_post(void *data, size_t len)
{
  int ret = 0;
  workq_ctx_t *ctx = (workq_ctx_t*)data;
  m_workq_ctx_q.push_back(*ctx);

  char cmd[1] = {WORKQ_CMD_POST};
  int count = 0;
  while ((++ count < 5) && (1 != write(WORKQ_CTRL_WRITE, cmd, 1))) {
    if (AM_LIKELY((errno != EAGAIN) &&
                  (errno != EWOULDBLOCK) &&
                  (errno != EINTR))) {
      PERROR("write");
      /* If POST command is failed to send, remove the last notification */
      m_workq_ctx_q.pop_back();
      ret = -1;
      break;
    }
  }
  return ret;
}

int AMServiceBase::workq_task_exec(void *data, size_t len)
{
  workq_ctx_t *ctx = (workq_ctx_t *)data;
  am_service_notify_payload *payload = &ctx->payload;
  AMServiceBase *service = ctx->service;
  NOTICE("Notification handler of %s is being called in thread %s",
         service->m_name.c_str(),
         m_workq_thread->name());
  return service->method_call(payload->msg_id,
                       payload->data, payload->data_size,
                       nullptr, 0);
}

int AMServiceBase::method_call(uint32_t cmd_id,
                               void *msg_data,
                               int msg_data_size,
                               void *result_addr,
                               int result_max_size)
{
  DECLARE_MUTEX
  int ret = 0;
  do {
    if (!m_ipc) {
      ERROR("AMServiceBase: IPC not created, not ready\n");
      ret = -1;
      break;
    }

    if (m_ipc->method_call(cmd_id,
                           msg_data,
                           msg_data_size,
                           result_addr,
                           result_max_size) < 0) {
      ERROR("AMServiceBase, ipc call SERVICE STATUS error \n");
      ret = -3;
      break;
    }
  } while (0);
  return ret;
}

int AMServiceBase::create_process() //run all apps
{
  int ret = 0;

  /* Preparing Child Process program and parameters */
  const char *cmdline = m_exe_filename.c_str();
  uint32_t cmdlen = strlen(cmdline);
  char str[cmdlen + 1];  //copy, before strtok_r will modify str
  const char *delimiter = " ";    //delimiter is space
  char *argv[AM_SERVICE_MAX_APPS_ARGS_NUM] = {0};
  char *saveptr = nullptr;
  char *start = str;

  strncpy(str, cmdline, cmdlen);
  str[cmdlen] = '\0';
  for (int i = 0; i < AM_SERVICE_MAX_APPS_ARGS_NUM; ++ i) {
    argv[i] = strtok_r(start, delimiter, &saveptr);
    start = nullptr; //for the next token search
  }
  NOTICE("Creating process %s", argv[0]);

  m_service_pid = vfork();
  switch(m_service_pid) {
    case 0: { /* Child Process */
      execvp(argv[0], argv);
      ERROR("Failed to run %s: %s", cmdline, strerror(errno));
      exit(1);
    }break;
    case -1 : {
      ERROR("Fork service %s failed: %s", m_name.c_str(), strerror(errno));
      ret = -1;
    }break;
    default: { /* Parent Process */
      std::string sem_file = std::string("/dev/shm/sem.") + m_name;
      uint32_t count = 0;

      NOTICE("Waiting for service %s(%d) to start!",
             m_name.c_str(), m_service_pid);

      while ((0 != access(sem_file.c_str(), F_OK)) && (++ count < 200)) {
        usleep(100 * 1000L);
      }
      if ((count >= 200) && (0 != access(sem_file.c_str(), F_OK))) {
        ERROR("Service %s failed to create semphore: %s!",
              m_name.c_str(), sem_file.c_str());
        ret = -1;
      } else {
        NOTICE("%s(%d) has started, sem file %s is created! %ums",
               m_name.c_str(), m_service_pid, sem_file.c_str(), count * 100);
      }
    }break;
  }

  if (ret == 0) {
    m_state = AM_SERVICE_STATE_INIT_PROCESS_CREATED;
  }

  return ret;
}

//wait for child process to exit
int AMServiceBase::wait_process()
{
  int ret = 0;
  int status = 0;
  do {
    //wait pid to collect its dead body
    //INFO("AMServiceBase:waitpid\n");
    if (waitpid(m_service_pid, &status, 0) < 0) {
      ERROR("AMServiceBase: waitpid failed \n");
    }
    if (WIFEXITED(status)) {
      DEBUG("child process pid %d exit with code %d\n",
            m_service_pid,
            WEXITSTATUS(status));
    }
  } while (0);
  return ret;
}

//this process is to make sure the ipc has been cleaned up,
//for example, the semaphore used to setup connection has been cleaned,
//otherwise,the client/server start may have connection problem.
int AMServiceBase::cleanup_ipc()
{
  std::string ipc_msg_queue_name = "/" + m_name;
  //clean up the message box, and also the semaphore
  return AMIPCSyncCmdClient::cleanup(ipc_msg_queue_name.c_str());
}

int AMServiceBase::create_ipc()
{
  int ret = 0;
  std::string ipc_msg_queue_name = "/" + m_name;

  do {
    if (!(m_ipc = new AMIPCSyncCmdClient())) {
      ret = -2;
      break;
    }

    //register msg map even before setup connection
    if (register_msg_map() < 0) {
      ERROR("AMServiceBase, unable to register msg map\n");
      ret = -7;
      break;
    }

    if (m_ipc->create(ipc_msg_queue_name.c_str()) < 0) {
      ERROR("AMServiceBase: ipc name %s created failed\n",
            ipc_msg_queue_name.c_str());
      ret = -1;
      break;
    }
  } while (0);

  if (ret == 0) {
    m_state = AM_SERVICE_STATE_INIT_IPC_CONNECTED;
  } else {
    //if create ipc failed, then delete the object and clear mem
    delete m_ipc;
    m_ipc = nullptr;
  }

  return ret;
}

void AMServiceBase::on_notif_callback(void *msg_data,
                                      int msg_data_size,
                                      void *result_addr,
                                      int result_max_size)
{
  am_service_notify_payload *payload = nullptr;
  AM_SERVICE_NOTIFY_RESULT result;
  if (msg_data
      && ((uint32_t)msg_data_size >=
          offsetof(am_service_notify_payload, data))) {
    payload = (am_service_notify_payload*)msg_data;
    result = payload->result;
  } else {
    result = AM_SERVICE_NOTIFY_FAILED;
    ERROR("Get notif failed");
    return;
  }

  INFO("AMServiceBase: %s got notif from actual process, result = %d\n",
        m_name.c_str(), result);
  AMServiceManagerPtr service_manager = AMServiceManager::get_instance();
  if (service_manager) {
    AM_SERVICE_MANAGER_STATE state = service_manager->get_state();
    if (state == AM_SERVICE_MANAGER_RUNNING) {
      AMServiceBase *service = nullptr;
      //use AM_SERVICE_TYPE_GENERIC as notify BIT
      if (TEST_BIT(payload->dest_bits, AM_SERVICE_TYPE_GENERIC)) {
        if (m_notify_cb) {
          m_notify_cb(m_notify_cb_data, msg_data, msg_data_size);
        }
      }
      for (uint32_t type = AM_SERVICE_TYPE_API_PROXY_SERVER;
          type < AM_SERVICE_TYPE_OTHERS; ++type) {
        if (payload->dest_bits && !TEST_BIT(payload->dest_bits, type)) {
          continue;
        }
        if ((service = service_manager->
                       find_service(AM_SERVICE_CMD_TYPE(type))) &&
            (service != this)) {
          NOTICE("%s is sending notification to %s",
                 m_name.c_str(), service->get_name());
          workq_ctx_t wq_ctx;
          wq_ctx.service = service;
          memcpy(&wq_ctx.payload, payload, sizeof(*payload));
          if (service->workq_task_post(&wq_ctx, sizeof(wq_ctx)) < 0) {
            ERROR("Add notification to %s notification queue failed! "
                  "One notification from %s is lost",
                  service->get_name(),
                  m_name.c_str());
          }
        }
      }
    }
  }
}

int AMServiceBase::register_msg_map()
{
  int ret = 0;
  am_msg_handler_t msg_handler;
  DEBUG("AMServiceBase:register_msg_map for %s\n", m_name.c_str());
  do {
    if (!m_ipc) {
      ret = -1;
      break;
    }

    //register a special msg handler with this pointer as context
    memset(&msg_handler, 0, sizeof(msg_handler));
    msg_handler.msgid = AM_IPC_SERVICE_NOTIF;
    msg_handler.callback = nullptr;

    //by using context callback, we can let the callback function
    //knows which instance of AMServiceBase should handle such message
    msg_handler.context = ((uintptr_t)this);
    msg_handler.callback_ct = static_on_notif_callback;

    //register the single entry msg map
    if (m_ipc->register_msg_map(&msg_handler, 1) < 0) {
      ERROR("AMServiceBase: register msg map \n");
      ret = -2;
      break;
    }
  } while (0);

  DEBUG("AMServiceBase:register_msg_map OK for %s\n", m_name.c_str());
  return ret;
}

int AMServiceBase::register_svc_notify_cb(void *proxy_ptr, AM_SVC_NOTIFY_CB cb)
{
  m_notify_cb_data = proxy_ptr;
  m_notify_cb = cb;
  return 0;
}
