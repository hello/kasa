
/*
 * am_ipc_base.cpp
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

/*#############################################################

  This File is implement of AMIPCBase, which is base library to simplify POSIX message queue
  creation and use and notification mechanism.

  #############################################################*/

#include <stdio.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include "am_base_include.h"
#include "am_ipc_base.h"

#include "am_log.h"
#include "am_define.h"

#define NOTIFY_SIG  SIGRTMIN
#define DEFAULT_IPC_PRIORITY    10

/*##################  Below are Public members #########################*/

int32_t debug_print_process_name(char *cmdline, int32_t max_size)
{
  int32_t ret = 0;
  char tmp[32];
  FILE *fp = nullptr;
  do {
    pid_t current_pid = getpid();
    if (current_pid <= 0) {
      ret = -1;
      break;
    }

    //get process name from proc filesys
    sprintf(tmp, "/proc/%d/cmdline", (int32_t)current_pid);
    if (!(fp = fopen(tmp, "r"))) {
      ERROR("open file %s error \n", tmp);
      ret = -1;
      break;
    }
    if (fgets(cmdline, max_size -1, fp) == nullptr) {
      PERROR("fgets");
    }
  } while (0);
  if (fp) {
    fclose(fp);
  }
  return ret;
}

AMIPCBase::AMIPCBase() :
    m_priority(DEFAULT_IPC_PRIORITY)
{}

AMIPCBase::~AMIPCBase()
{
  destroy();
}

static inline int32_t check_msg_queue_name(const char *msg_queue_name)
{
  int32_t ret = 0;
  do {
    if (!msg_queue_name || (msg_queue_name[0] != '/')) {
      ret = -1;
      break;
    }
    int32_t length = strlen((char*)msg_queue_name);
    if (AM_UNLIKELY((length >= IPC_MAX_MSGQUEUE_NAME_LENGTH) || (length < 2))) {
      ret = -1;
      break;
    }
  } while (0);
  return ret;
}

//sometimes the receive queue is somehow not empty, so it will never trigger
//notification again, so we must empty the receive queue first, during setup queue
static inline int32_t poll_queue_till_empty(mqd_t receive_queue)
{
  int32_t ret = 0;
  char receive_buffer[IPC_MESSAGE_SIZE] = {0};
  mq_attr attr;

  do {
    if (mq_getattr(receive_queue, &attr) < 0) {
      ret = -1;
      break;
    }

    for (int32_t i = 0; i < attr.mq_curmsgs ; ++i) {
      if (mq_receive(receive_queue, receive_buffer,
                     IPC_MESSAGE_SIZE, nullptr) < 0) {
        ret = -1;
        break;
      }
    }
  } while (0);
  return ret;
}

int32_t AMIPCBase::create(const char *send_msgqueue_name,
                          const char *receive_msgqueue_name)
{
  int32_t ret = 0;
  do {
    if (m_create_flag) {
      WARN("AMIPCBase: already created, cannot create again!");
      ret = -1;
      break;
    }

    if (send_msgqueue_name &&
        (create_msg_queue(send_msgqueue_name, IPC_MSG_QUEUE_ROLE_SEND) < 0)) {
      ret = -1;
      break;
    }

    if (receive_msgqueue_name &&
        (create_msg_queue(receive_msgqueue_name, IPC_MSG_QUEUE_ROLE_RECEIVE) < 0)) {
      ret = -1;
      break;
    }

    if (!(m_receive_buffer = new char[IPC_MESSAGE_SIZE + 1])) {
      ERROR("AMIPCBase::create: receive buffer create failed!");
      ret = -1;
      break;
    }

    m_create_flag = 1;
  } while (0);
  return ret;
}

mqd_t AMIPCBase::get_msg_queue(int32_t msg_queue_role)
{
  return m_msg_queue[msg_queue_role];
}

int32_t AMIPCBase::send(void *data, int32_t length)
{
  int32_t ret = 0;

  do {
    if (!data) {
      break;
    }
    if (AM_UNLIKELY((length <= 0) || (length > IPC_MESSAGE_SIZE)))  {
      ERROR("AMIPCBase: send length %d is wrong!", length);
      ret = -1;
      break;
    }

    if (AM_UNLIKELY(m_msg_queue[IPC_MSG_QUEUE_ROLE_SEND] < 0)) {
      ERROR("AMIPCBase: send msg queue invalid!");
      ret = -1;
      break;
    }

    if (0 != mq_send(m_msg_queue[IPC_MSG_QUEUE_ROLE_SEND],
                     (char*)data, length, m_priority)) {
      PERROR("AMIPCBase: mq_send");
      mq_close(m_msg_queue[IPC_MSG_QUEUE_ROLE_SEND]);
      m_msg_queue[IPC_MSG_QUEUE_ROLE_SEND] = -1;
      ret = -1;
      break;
    }
  } while (0);
  return ret;
}

static void receive_cb(sigval_t sv)
{
  // cast the callback arg back to this pointer,
  //pThis is same as this pointer of this object
  AMIPCBase *pThis = (AMIPCBase*)sv.sival_ptr;
  if (pThis) {
    //must register receive cb again before receive, otherwise will miss notification
    //check "man mq_notify"
    pThis->register_receive_callback();
    pThis->receive();
  }
}

int32_t AMIPCBase::receive()
{
  int32_t bytes_read;
  int32_t ret = 0;
  int32_t ret_code;
  mq_attr attr;

  do {
    //when the sem has been got, but object has been destroyed, just return error
    if (AM_UNLIKELY(!m_create_flag)) {
      ret = -5;
      break;
    }

    if (AM_UNLIKELY(m_msg_queue[IPC_MSG_QUEUE_ROLE_RECEIVE] < 0)) {
      ERROR("AMIPCBase: receive queue not setup!");
      ret = -1;
      break;
    }
    if (AM_UNLIKELY(m_receive_buffer == nullptr)) {
      ERROR("AMIPCBase: Null receive buffer!");
      ret = -2;
      break;
    }
    if (-1 == mq_getattr(m_msg_queue[IPC_MSG_QUEUE_ROLE_RECEIVE], &attr)) {
      PERROR("mq_getattr");
      ret = -3;
      break;
    }
    while (attr.mq_curmsgs > 0) {
      bytes_read = mq_receive(m_msg_queue[IPC_MSG_QUEUE_ROLE_RECEIVE],
                              m_receive_buffer,
                              max_message_size, nullptr);
      if (bytes_read < 0) {
        if (errno != EAGAIN) {
          ERROR("mq_receive %d:%s", errno, strerror(errno));
        } else {
          WARN("system interrupt");
        }
      } else if (bytes_read > 0) {
        m_receive_buffer_bytes = bytes_read - sizeof(am_ipc_message_header_t);
        if (AM_UNLIKELY((ret_code = process_msg()) != 0))  {
          if(ret_code != AM_IPC_CMD_ERR_IGNORE) {
            ERROR("AMIPCBase: error in process received msg, err code %d!",
                  ret_code);
          }
        }
      }
      if (-1 == mq_getattr(m_msg_queue[IPC_MSG_QUEUE_ROLE_RECEIVE], &attr)) {
        ret = -4;
        break;
      }
    }
  } while(0);
  return ret;
}

char* AMIPCBase::get_receive_buffer()
{
  return m_create_flag ? m_receive_buffer : nullptr;
}

int32_t AMIPCBase::process_msg()
{
  PRINTF("AMIPCBase: got a MSG string %s", m_receive_buffer);
  return 0;
}

int32_t AMIPCBase::get_last_error()
{
  return m_error_code;
}

/*##################  Below are Protected members #########################*/

int32_t AMIPCBase::register_receive_callback_first_time(char *mq_name)
{
  int32_t ret = -1;

  mq_attr attr;
  char receive_buffer[IPC_MESSAGE_SIZE] = {0};

  while (1) {
    //check queue status
    if (m_msg_queue[IPC_MSG_QUEUE_ROLE_RECEIVE] != -1) {
      //drain mq to empty so that async notification mechanism can work always
      while(true) {
        if (mq_receive(m_msg_queue[IPC_MSG_QUEUE_ROLE_RECEIVE],
                       receive_buffer, IPC_MESSAGE_SIZE, nullptr) < 0) {
          if (errno == EAGAIN) {
            break;
          } else {
            PRINTF("register_receive_callback_first_time: drain mq failed!");
            return -1;
          }
        }
      }
      //register it first.
      if (register_receive_callback() < 0) {
        char tmp[256];
        debug_print_process_name(tmp, 255);
        ERROR("Process %s register_receive_callback_first_time failed, register call back failed for mq %s \n", tmp, mq_name);
        ret = -1;
        break;
      }

      if (mq_getattr(m_msg_queue[IPC_MSG_QUEUE_ROLE_RECEIVE] , &attr) < 0) {
        ERROR("register_receive_callback_first_time: get msg queue attr error!");
        ret = -1;
        break;
      }
      if (attr.mq_curmsgs == 0) {
        // PRINTF(" msg queue %s drained to  empty  \n",  mq_name);
        ret = 0;
        break;
      }
    } else {
      ERROR("register_receive_callback_first_time failed, null receive queue!");
      break;
    }
  }

  return ret;
}

int32_t AMIPCBase::register_receive_callback()
{
  int32_t ret = 0;
  sigevent sev;

  sev.sigev_notify = SIGEV_THREAD; /* Notify via thread */
  sev.sigev_notify_function = receive_cb;
  sev.sigev_notify_attributes = nullptr;
  /* Could be pointer to pthread_attr_t structure */
  sev.sigev_value.sival_ptr = this; /* Argument to threadFunc, Pass this pointer */

  do {
    if (AM_UNLIKELY(m_msg_queue[IPC_MSG_QUEUE_ROLE_RECEIVE] < 0)) {
      ret = -1;
      break;
    }

    if (AM_UNLIKELY(mq_notify(m_msg_queue[IPC_MSG_QUEUE_ROLE_RECEIVE],
                              &sev) == -1)) {
      PERROR("AMIPCBase: mq_notify");
      ret = errno;
    }
  } while (0);

  return ret;
}

int32_t AMIPCBase::create_msg_queue(const char *msg_queue_name, int32_t role)
{
  int32_t ret = 0;
  mqd_t msg_queue_new;
  mq_attr attr;
  int32_t oflag;

  do {
    if (AM_UNLIKELY(check_msg_queue_name(msg_queue_name) < 0)) {
      PRINTF("AMIPCBase: msg queue name check failed\n");
      ret = -1;
      break;
    }

    if (AM_UNLIKELY((role< 0) || (role > 1))) {
      ret = -1;
      break;
    }

    //check role
    if ((m_msg_queue[role] >= 0)) {
      PRINTF("AMIPCBase: msg queue exist for role %d, cannot create it again!",
             role);
      ret = -1;
      break;
    }

    attr.mq_flags = 0;
    attr.mq_maxmsg = max_messages;
    attr.mq_msgsize = max_message_size;
    attr.mq_curmsgs = 0;

    mq_unlink((char*)msg_queue_name);

    //the reason to use block send is to prevent send queue error because of queue full
    //the reason to use non block receive is to read all messages and quit the receive thread.

    if (role == IPC_MSG_QUEUE_ROLE_SEND) {
      oflag = O_RDWR | O_CREAT | O_EXCL ; //send is block send
    } else {
      oflag = O_RDWR | O_CREAT | O_EXCL | O_NONBLOCK; //receive is nonblock receive
    }

    msg_queue_new = mq_open(msg_queue_name, oflag, S_IRWXU | S_IRWXG, &attr);
    if (msg_queue_new < 0)  {
      ERROR("AMIPCBase: Error Opening msg queue %s, msg queue may be not exist, or attr incorrect. ", msg_queue_name);
      PERROR("AMIPCBase: mq_open");
      ret = -1;
      break;
    } else {
      m_msg_queue_name[role] = new char[IPC_MAX_MSGQUEUE_NAME_LENGTH+1];
      strcpy(m_msg_queue_name[role], msg_queue_name);

      m_msg_queue[role] = msg_queue_new;
      poll_queue_till_empty(m_msg_queue[role]);
      if (role == IPC_MSG_QUEUE_ROLE_RECEIVE) {
        if (register_receive_callback() < 0) {
          ret = -1;
          break;
        }
      }
    }
  } while (0);
  return ret;
}

//bind to existing msg queue that created by other process
int32_t AMIPCBase::bind_msg_queue(char *msg_queue_name, int32_t role)
{
  int32_t ret = 0;
  mqd_t msg_queue_new;
  int32_t oflag;

  do {
    if (AM_UNLIKELY(!m_create_flag)) {
      ret = -1;
      break;
    }

    if (AM_UNLIKELY(check_msg_queue_name(msg_queue_name) < 0)) {
      ERROR("AMIPCBase: msg queue name check failed!");
      ret = -1;
      break;
    }

    if (AM_UNLIKELY((role < 0) || (role > 1))) {
      ret = -1;
      break;
    }

    if (m_msg_queue[role] >= 0) {
      ERROR("AMIPCBase: msg queue exist for role %d, cannot bind it again!",
            role);
      ret = -1;
      break;
    }

    //now bind it (must exist)
    //the reason to use block send is to prevent send queue error because of queue full
    //the reason to use non block receive is to read all messages and quit the receive thread.

    if (role == IPC_MSG_QUEUE_ROLE_SEND) {
      oflag = O_WRONLY; //send is write-only, (no create flag), block mode send
    } else {
      oflag = O_RDONLY | O_NONBLOCK; //receive is Non block receive
    }

    msg_queue_new = mq_open(msg_queue_name, oflag , S_IRWXU | S_IRWXG, nullptr);
    if (msg_queue_new == -1)  {
      PERROR ("AMIPCBase: Error Opening msg queue in bind!");
      ret = -1;
      break;
    } else {
      m_msg_queue[role] = msg_queue_new;
      if (role == IPC_MSG_QUEUE_ROLE_RECEIVE) {
        register_receive_callback_first_time(msg_queue_name);
      }
    }
  } while (0);
  return ret;
}

int32_t AMIPCBase::destroy_msg_queue(int32_t role)
{
  if (AM_UNLIKELY((role < 0 ) || (role > 1))) {
    return -1;
  }

  if (m_msg_queue[role] >= 0) {
    mq_close(m_msg_queue[role]);
    m_msg_queue[role] = -1;
  }

  //only if the msg queue is created, m_msg_queue_name is not NULL
  //if it is binded to another msg queue created by someone else,
  //m_msg_queue_name is NULL
  if (m_msg_queue_name[role] != nullptr) {
    mq_unlink(m_msg_queue_name[role]);
    delete[] m_msg_queue_name[role];
    m_msg_queue_name[role] = nullptr;
  }

  return 0;
}

int32_t AMIPCBase::destroy()
{
  int32_t ret = 0;
  //mark the class is being destroyed, so it will not handle more async event
  //because of this check, a AMIPCBase object can be "destroyed" many times,
  //if it has been destroyed or not created, then it will just return -1
  do {
    if (AM_UNLIKELY(!m_create_flag)) {
      ret = -1;
      break;
    }

    m_create_flag = 0;
    if (destroy_msg_queue(IPC_MSG_QUEUE_ROLE_SEND) < 0) {
      ERROR("AMIPCBase: unable to destroy send queue!");
      ret = -1;
      break;
    }

    if (destroy_msg_queue(IPC_MSG_QUEUE_ROLE_RECEIVE) < 0) {
      ERROR("AMIPCBase: unable to destroy receive queue!");
      ret = -1;
      break;
    }

    if (m_receive_buffer) {
      delete[] m_receive_buffer;
      m_receive_buffer = nullptr;
    }
  } while (0);
  return ret;
}

void AMIPCBase::dump_receive_buffer()
{
  uint8_t *pdata = (uint8_t*)m_receive_buffer;

  PRINTF("RECEIVE BUFFER DUMP: \n");
  for (int32_t i = 0; i < 16 ; i++ )  {
    for (int32_t j = 0; j < 16; j++) {
      PRINTF("%4x ", pdata[i * 16 + j]);
    }
    PRINTF("\n");
  }
}

int32_t AMIPCBase::debug_print_queue_info()
{
  int32_t ret = 0;
  char tempbuffer[256];
  mq_attr attr;

  do {
    debug_print_process_name(tempbuffer, 255);
    if (m_msg_queue[IPC_MSG_QUEUE_ROLE_SEND] != -1) {
      if (mq_getattr(m_msg_queue[IPC_MSG_QUEUE_ROLE_SEND], &attr) < 0) {
        ERROR("AMIPCBase:  get msg queue attr error!");
        ret = -1;
        break;
      }
      PRINTF("DEBUG IPC: Process %s, Send IPC current msgs %ld",
             tempbuffer, attr.mq_curmsgs );
    }

    if (m_msg_queue[IPC_MSG_QUEUE_ROLE_RECEIVE] != -1) {
      if (mq_getattr(m_msg_queue[IPC_MSG_QUEUE_ROLE_RECEIVE], &attr) < 0) {
        PRINTF("AMIPCBase:  get msg queue attr error \n");
        ret = -1;
        break;
      }
      PRINTF("DEBUG IPC: Process %s, Receive IPC current msgs %ld",
             tempbuffer, attr.mq_curmsgs );
    }
  } while (0);

  return ret;
}
