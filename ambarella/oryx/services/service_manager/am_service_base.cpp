/*******************************************************************************
 * am_service_base.cpp
 *
 * History:
 *   2014-9-12 - [lysun] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co,Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <signal.h>
#include <mutex>  //using C++11 recursive mutex
#include "am_log.h"
#include "am_define.h"
#include "am_ipc_sync_cmd.h"
#include "am_service_base.h"
#include "am_service_manager.h"

//C++ 11 mutex (using non-recursive type)
static std::mutex m_mtx;
#define  DECLARE_MUTEX   std::lock_guard<std::mutex> lck (m_mtx);

AMServiceBase::AMServiceBase(const char *service_name,
                             const char *full_pathname,
                             AM_SERVICE_CMD_TYPE type) :
  m_service_pid(-1),
  m_state(),
  m_ipc(NULL),
  m_type(type)
{
  strncpy(m_name, service_name, AM_SERVICE_NAME_MAX_LENGTH);
  m_name[AM_SERVICE_NAME_MAX_LENGTH - 1] = '\0';
  strncpy(m_exe_filename, full_pathname, AM_SERVICE_EXE_FILENAME_MAX_LENGTH);
  m_exe_filename[AM_SERVICE_EXE_FILENAME_MAX_LENGTH - 1] = '\0';
}

AMServiceBase::~AMServiceBase()
{
  if (m_state != AM_SERVICE_STATE_NOT_INIT) {
    destroy();
  }
}

int AMServiceBase::get_name(char *service_name, int max_len)
{
  if ((!service_name) || (max_len < 1))
    return -1;

  strncpy(service_name, m_name, max_len - 1);
  service_name[max_len - 1] = '\0';

  return 0;
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
  //  INFO("AMServiceBase:init\n");
  do {
    //check exe file name
    if (strlen(m_exe_filename) == 0) {
      ERROR("AMServiceBase, empty exe filename\n");
      ret = -1;
      break;
    }

    //check service name
    if (strlen(m_name) == 0) {
      ERROR("AMServiceBase, empty service name\n");
      ret = -3;
      break;
    }

    if (cleanup_ipc() < 0) {
      ERROR("AMServiceBase, unable to cleanup ipc \n");
      ret = -6;
      break;
    }

    //create process
    if (create_process() < 0) {
      ERROR("AMServiceBase, unable to create process %s\n", m_exe_filename);
      ret = -2;
      break;
    }

    //create IPC
    if (create_ipc() < 0) {
      ERROR("AMServiceBase, unable to create ipc\n");
      ret = -4;
      break;
    }

    do {
      //call init again if init result is not ready,
      //until init ready, then break at INIT_DONE
      if (m_ipc->method_call(AM_IPC_SERVICE_INIT,
                             NULL,
                             0,
                             &service_result,
                             sizeof(service_result)) < 0) {
        ERROR("AMServiceBase, ipc call SERVICE INIT error \n");
        ret = -5;
        break;
      }

      DEBUG("AMServiceBase,  method_call AM_IPC_SERVICE_INIT returns\n");
      if (service_result.ret < 0) {
        ERROR("AMService %s found init error from result %d\n",
              m_name,
              service_result.ret);
        ret = -6;
        break;
      }
      if (service_result.state != AM_SERVICE_STATE_INIT_DONE
          && service_result.state != AM_SERVICE_STATE_STARTED) {
        INFO("AMServiceBase: check service %s state not done\n", m_name);
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
  //  INFO("AMServiceBase:destroy\n");
  do {
    if (!m_ipc) {
      ERROR("AMServiceBase: destroy, ipc not setup yet\n ");
      ret = -1;
      break;
    }

    //call destroy function
    if (m_ipc->method_call(AM_IPC_SERVICE_DESTROY,
                           NULL,
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

    //destroy the ipc
    delete m_ipc;
    m_ipc = NULL;

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
                           NULL,
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
                           NULL,
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
        && (m_state != AM_SERVICE_STATE_STOPPED)
        && (m_state != AM_SERVICE_STATE_STOPPED)) {
      ERROR("AMServiceBase: not ready to start from state%d\n", m_state);
      ret = -2;
      break;
    }

    PRINTF("try to restart from state %d\n", m_state);

    if (m_ipc->method_call(AM_IPC_SERVICE_RESTART,
                           NULL,
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
                           NULL,
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
           m_name,
           service_result.state,
           m_state);

  } while (0);
  return ret;
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

//use strtok_r to parse the tokens
static inline int parse_exec_cmd(char * cmdline, char *argv[])
{
  char str[AM_SERVICE_EXE_FILENAME_MAX_LENGTH + 1];
  const char *delimiter = " ";    //delimiter is space
  char *saveptr = NULL;
  char *token = NULL;
  char *start = str;

  //copy, before strtok_r will modify str
  strcpy(str, cmdline);
  for (int i = 0; i < AM_SERVICE_MAX_APPS_ARGS_NUM; i ++) {
    token = strtok_r(start, delimiter, &saveptr);
    if (token == NULL) {
      argv[i] = NULL;
      //PRINTF("argv[%d] is %s\n", i,  argv[i]);
      break;
    } else {
      argv[i] = token;
      //PRINTF("argv[%d] is %s\n", i,  token);
    }
    start = NULL; //for the next token search
  }

  execvp(argv[0], argv);
  //if it returns, means exec failed
  PERROR("AMServiceBase:execvp\n");
  exit(1);

  return 0;
}

int AMServiceBase::create_process() //run all apps
{
  int pid = 0;
  char *argv[AM_SERVICE_MAX_APPS_ARGS_NUM] =
  {0};
  int ret = 0;
  //  INFO("AMServiceBase:create_process %s\n", m_name);
  do {
    pid = fork();
    if (pid < 0) {
      PERROR("AMServiceBase: fork\n");
      ret = -1;
      break;
    } else if (pid == 0) {
      //it's the child
      INFO("AMServiceBase::run  %s \n", m_exe_filename);
      parse_exec_cmd(m_exe_filename, argv);
    } else {
      // it's the parent
      m_service_pid = pid;
    }
  } while (0);

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
  char ipc_msg_queue_name[AM_SERVICE_NAME_MAX_LENGTH + 1];

  sprintf(ipc_msg_queue_name, "/%s", m_name);
  //clean up the message box, and also the semaphore
  return AMIPCSyncCmdClient::cleanup(ipc_msg_queue_name);
}

int AMServiceBase::create_ipc()
{
  int ret = 0;
  char ipc_msg_queue_name[AM_SERVICE_NAME_MAX_LENGTH + 1];

  do {
    m_ipc = new AMIPCSyncCmdClient;
    if (!m_ipc) {
      ret = -2;
      break;
    }

    //register msg map even before setup connection
    if (register_msg_map() < 0) {
      ERROR("AMServiceBase, unable to register msg map\n");
      ret = -7;
      break;
    }

    sprintf(ipc_msg_queue_name, "/%s", m_name);
    if (m_ipc->create(ipc_msg_queue_name) < 0) {
      ERROR("AMServiceBase: ipc name %s created failed\n", ipc_msg_queue_name);
      ret = -1;
      break;
    }
    //   INFO("AMServiceBase:create_ipc %s OK\n", m_name);
  } while (0);

  if (ret == 0) {
    m_state = AM_SERVICE_STATE_INIT_IPC_CONNECTED;
  } else {
    //if create ipc failed, then delete the object and clear mem
    delete m_ipc;
    m_ipc = NULL;
  }

  return ret;
}

void AMServiceBase::on_notif_callback(uint32_t context,
                                      void *msg_data,
                                      int msg_data_size,
                                      void *result_addr,
                                      int result_max_size)
{
  char buf[AM_SERVICE_NAME_MAX_LENGTH + 1] = {0};
  AMServiceBase *pThis = (AMServiceBase*)context;
  am_service_notify_payload *payload = nullptr;
  AM_SERVICE_NOTIFY_RESULT result;
  am_service_result_t service_result;
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

  pThis->get_name(buf, AM_SERVICE_NAME_MAX_LENGTH);
  INFO("AMServiceBase: %s got notif from actual process, result = %d\n",
        buf, result);
  AMServiceManagerPtr service_manager = AMServiceManager::get_instance();
  if (service_manager) {
    AM_SERVICE_MANAGER_STATE state = service_manager->get_state();
    if (state == AM_SERVICE_MANAGER_RUNNING) {
      AMServiceBase *service = nullptr;
      for (uint32_t type = AM_SERVICE_TYPE_API_PROXY_SERVER;
          type < AM_SERVICE_TYPE_OTHERS; ++type) {
        if (payload->dest_bits && !TEST_BIT(payload->dest_bits, type)) {
          continue;
        }
        if ((service =
            service_manager->find_service(AM_SERVICE_CMD_TYPE(type)))) {
          if (service == pThis) {
            continue;
          }
          service->get_name(buf, AM_SERVICE_NAME_MAX_LENGTH);
          INFO("Notify Service: %s", buf);
          service->method_call(payload->msg_id,
                               payload->data, payload->data_size,
                               &service_result, sizeof(service_result));
        }
      }
    }
  }
}

int AMServiceBase::register_msg_map()
{
  int ret = 0;
  am_msg_handler_t msg_handler;
  DEBUG("AMServiceBase:register_msg_map for %s\n", m_name);
  do {
    if (!m_ipc) {
      ret = -1;
      break;
    }

    //register a special msg handler with this pointer as context
    memset(&msg_handler, 0, sizeof(msg_handler));
    msg_handler.msgid = AM_IPC_SERVICE_NOTIF;
    msg_handler.callback = NULL;

    //by using context callback, we can let the callback function
    //knows which instance of AMServiceBase should handle such message
    msg_handler.context = (uint32_t) this;
    msg_handler.callback_ct = AMServiceBase::on_notif_callback;

    //register the single entry msg map
    if (m_ipc->register_msg_map(&msg_handler, 1) < 0) {
      ERROR("AMServiceBase: register msg map \n");
      ret = -2;
      break;
    }
  } while (0);

  DEBUG("AMServiceBase:register_msg_map OK for %s\n", m_name);
  return ret;
}
