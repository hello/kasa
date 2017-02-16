
/*
 * am_ipc_base.cpp
 *
 * History:
 *    2014/09/09 - [Louis Sun] Create
 *
 * Copyright (C) 2004-2013, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

/*#############################################################

  This File is implement of AMIPCBase, which is base library to simplify POSIX message queue
  creation and use and notification mechanism.
  This is a C++ library and used by "Air" APPs to call and use it as inter process communication.

  #############################################################*/

/*##################  Below are Public members #########################*/


#include <assert.h>
#include <stdio.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>
#include <signal.h>
#include <pthread.h>
#define ASSERT  assert
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include "am_base_include.h"
#include "am_ipc_base.h"
#include "am_log.h"
#include "am_define.h"

#define NOTIFY_SIG  SIGRTMIN
#define DEFAULT_IPC_PRIORITY    10


int32_t debug_print_process_name(char *cmdline,  int32_t max_size)
{
  char tmp[32];
  FILE *fp;
  pid_t current_pid = getpid();

  if (current_pid <= 0)
    return -1;

  //get process name from proc filesys
  sprintf(tmp, "/proc/%d/cmdline", (int32_t)current_pid);
  fp = fopen(tmp, "r");
  if (!fp) {
    PRINTF("open file %s error \n", tmp);
    return -1;
  }
  fgets((char *)cmdline, max_size -1, fp);
  fclose(fp);
  return 0;
}

//AMIPCBase is just to simply construct the object
//still need to call 'create' to create all the stuffs
AMIPCBase::AMIPCBase() :
    m_receive_buffer(nullptr),
    m_receive_buffer_bytes(0),
    m_priority(DEFAULT_IPC_PRIORITY),
    m_error_code(0),
    m_receive_notif_setup(0),
    m_create_flag(0)
{
  //clear the handles to invalid
  m_msg_queue[IPC_MSG_QUEUE_ROLE_SEND] = -1;
  m_msg_queue[IPC_MSG_QUEUE_ROLE_RECEIVE] = -1;

  m_msg_queue_name[IPC_MSG_QUEUE_ROLE_SEND] = NULL;
  m_msg_queue_name[IPC_MSG_QUEUE_ROLE_RECEIVE] = NULL;
}

AMIPCBase::~AMIPCBase()
{
  destroy();
}

//return 0 if msg queue name is OK,  return -1 if not OK
static inline int32_t check_msg_queue_name(const char * msg_queue_name)
{
  int32_t length;
  //check name
  if (AM_UNLIKELY(msg_queue_name == NULL))
    return -1;
  if (AM_UNLIKELY(msg_queue_name[0]!='/'))
    return -1;
  length = strlen((char *)msg_queue_name);
  if (AM_UNLIKELY((length >= IPC_MAX_MSGQUEUE_NAME_LENGTH) || (length < 2)))
    return -1;
  return 0;
}

//sometimes the receive queue is somehow not empty,  so it will never trigger
//notification again,  so we must empty the receive queue first, during setup queue

//sometimes the receive queue is somehow not empty,  so it will never trigger
//notification again,  so we must empty the receive queue first, during setup queue
static inline int32_t poll_queue_till_empty(mqd_t receive_queue)
{
  char receive_buffer[IPC_MESSAGE_SIZE] = {0};
  struct mq_attr attr;

  int32_t i;
  //poll 1000 times, to make sure queue is empty
  if  (mq_getattr(receive_queue, &attr) < 0)
    return -1;

  //check how many current msgs,  receive all of them to empty it

  if (attr.mq_curmsgs > 0) {
    //PRINTF("queue is not empty, cur num %ld , now empty it \n", attr.mq_curmsgs);
  }

  for (i = 0; i <  attr.mq_curmsgs ; i++) {
    if (mq_receive(receive_queue, receive_buffer, IPC_MESSAGE_SIZE, NULL) < 0)
      return -1;
  }
  return 0;
}

int32_t AMIPCBase::create(const char *send_msgqueue_name,
                     const char *receive_msgqueue_name) //ipcbase may choose to create both msgqueue name
{
  if (m_create_flag) {
    PRINTF("AMIPCBase: already created, cannot create again \n");
    return -1;
  }

  if (send_msgqueue_name != NULL) {
    if (create_msg_queue ( send_msgqueue_name, IPC_MSG_QUEUE_ROLE_SEND) < 0)
      return -1;
  }

  if (receive_msgqueue_name != NULL) {
    if (create_msg_queue ( receive_msgqueue_name, IPC_MSG_QUEUE_ROLE_RECEIVE) < 0)
      return -1;
  }

  m_receive_buffer = new char [IPC_MESSAGE_SIZE + 1];
  if (AM_UNLIKELY(!m_receive_buffer)) {
    PRINTF(" AMIPCBase::create: receive buffer create failed \n");
    return -1;
  }

  m_create_flag = 1;
  return 0;
}

mqd_t AMIPCBase::get_msg_queue(int32_t msg_queue_role) //return receive msg queue, if no, return -1
{
  return m_msg_queue[msg_queue_role];
}

int32_t AMIPCBase::send(void *data,  int32_t length)
{
  ASSERT(data!=NULL);

  if (AM_UNLIKELY((length <=0 ) ||(length > IPC_MESSAGE_SIZE)))  {
    PRINTF("AMIPCBase: send length %d is wrong \n", length);
    return -1;
  }

  if (AM_UNLIKELY(m_msg_queue[IPC_MSG_QUEUE_ROLE_SEND] < 0)) {
    PRINTF("AMIPCBase: send msg queue invalid \n");
    return -1;
  }

  if (0 != mq_send (m_msg_queue[IPC_MSG_QUEUE_ROLE_SEND], (char *)data, length, m_priority)) {
    PERROR ("AMIPCBase: mq_send");
    mq_close (m_msg_queue[IPC_MSG_QUEUE_ROLE_SEND]);
    return -1;
  } else {
    // print_queue_status(m_msg_queue[IPC_MSG_QUEUE_ROLE_SEND]);
    return 0;
  }
}


//receive function
static void receive_cb(union sigval sv)
{
  AMIPCBase * pThis;
  // cast the callback arg back to this pointer,
  //pThis is same as  this pointer of this object
  pThis  = (AMIPCBase *)sv.sival_ptr;

  //must register receive cb again before receive, otherwise will miss notification
  //check "man mq_notify"
  pThis->register_receive_callback();

  //debug thread id and process id
  // PRINTF("receive thread id is %d , tid is %ld \n", (int32_t)pthread_self(),  syscall(SYS_gettid));

  //PRINTF("receive_cb \n");
  pThis->receive();
}


int32_t AMIPCBase::receive()
{
  //msg queue to receive is msg_queue_receive
  //handle receive from this msg queue
  int32_t bytes_read;
  //mainloop of receive loop , in a receive signal thread (which is not AMIPCBase object's main thread )
  //PRINTF("Notif receive, try mq_receive \n");
  int32_t ret = 0;
  int32_t ret_code;

  // PRINTF("AMIPCBase::receive called \n");
  do {
    //when the sem has been got, but object has been destroyed, just return error
    if (AM_UNLIKELY(!m_create_flag)) {
      ret = -5;
      break;
    }

    if (AM_UNLIKELY(m_msg_queue[IPC_MSG_QUEUE_ROLE_RECEIVE] < 0)) {
      PRINTF("AMIPCBase: receive queue not setup \n");
      ret = -1;
      break;
    }
    if (AM_UNLIKELY(m_receive_buffer==NULL)) {
      PRINTF("AMIPCBase: Null receive buffer \n");
      ret = -2;
      break;
    }
    while ((bytes_read = mq_receive(m_msg_queue[IPC_MSG_QUEUE_ROLE_RECEIVE],
                                    (char *)m_receive_buffer,
                                    max_message_size, NULL)) >= 0) {
      //gettimeofday(&time_current, NULL);
      //time_remote = atoll(receivebuffer);
      //time_diff =  (time_current.tv_sec * 1000000L + time_current.tv_usec) - time_remote;
      //fprintf(stderr, "time_diff is %ld\n",  time_diff);
      m_receive_buffer_bytes = bytes_read - sizeof(am_ipc_message_header_t);
      //    PRINTF("received one msg \n");
      if (AM_UNLIKELY((ret_code = process_msg()) != 0))  {
        //if the error is trivial and can be ignored, then ignore it ,
        //otehrwise, treat it as an error
        if(ret_code != AM_IPC_CMD_ERR_IGNORE) {
          PRINTF(" AMIPCBase: error in process received msg, err code %d \n",
                 ret_code);
          ret = -3;
          break;
        }
      }
    }

    if (AM_UNLIKELY ((ret < 0) && (errno != EAGAIN )))  {
      //PERROR("mq_receive");
      ret = -4;
      break;
    }
  }while(0);

  return ret;
}

char *AMIPCBase::get_receive_buffer()
{
  if (AM_UNLIKELY(!m_create_flag))
    return NULL;
  else
    return m_receive_buffer;
}

int32_t AMIPCBase::process_msg()
{
  //try to dump it as string , let derived class to handle it as IPC msg
  PRINTF("AMIPCBase: got a MSG string %s \n",  m_receive_buffer);
  return 0;
}

int32_t AMIPCBase::get_last_error()
{
  return m_error_code;
}

/*##################  Below are Protected members #########################*/

int32_t AMIPCBase::register_receive_callback_first_time(char * mq_name)
{
  int32_t ret = -1;

  struct mq_attr attr;
  char receive_buffer[IPC_MESSAGE_SIZE] = {0};

  while (1) {
    //check queue status
    if (m_msg_queue[IPC_MSG_QUEUE_ROLE_RECEIVE] != -1) {
      //drain mq to empty so that async notification mechanism can work always
      while(1) {
        if (mq_receive(m_msg_queue[IPC_MSG_QUEUE_ROLE_RECEIVE], receive_buffer, IPC_MESSAGE_SIZE, NULL) < 0) {
          if (errno == EAGAIN) {
            break;
          } else {
            PRINTF("register_receive_callback_first_time:  drain mq failed  \n");
            return -1;
          }
        }
      }
      //register it first.
      if (register_receive_callback() < 0) {
        char tmp[256];
        debug_print_process_name(tmp, 255);
        PRINTF("Process %s register_receive_callback_first_time failed, register call back failed for mq %s \n", tmp, mq_name);
        ret = -1;
        break;
      }

      if (mq_getattr(m_msg_queue[IPC_MSG_QUEUE_ROLE_RECEIVE] , &attr) < 0) {
        PRINTF("register_receive_callback_first_time:  get msg queue attr error \n");
        ret = -1;
        break;
      }
      if (attr.mq_curmsgs == 0) {
        // PRINTF(" msg queue %s drained to  empty  \n",  mq_name);
        ret = 0;
        break;
      }
    } else {
      PRINTF("register_receive_callback_first_time failed, null receive queue \n");
      break;
    }
  }

  return ret;
}


int32_t AMIPCBase::register_receive_callback()
{

  int32_t ret = -1;

  struct sigevent sev;

  sev.sigev_notify = SIGEV_THREAD; /* Notify via thread */
  sev.sigev_notify_function = receive_cb;
  sev.sigev_notify_attributes = NULL;
  /* Could be pointer to pthread_attr_t structure */
  sev.sigev_value.sival_ptr = this; /* Argument to threadFunc, Pass this pointer */

  if (AM_UNLIKELY(m_msg_queue[IPC_MSG_QUEUE_ROLE_RECEIVE] < 0)) {
    // PRINTF("AMIPCBase: Error in register_receive_callback, receive queue not setup \n");
    return -1;
  }

  if (AM_UNLIKELY(mq_notify(m_msg_queue[IPC_MSG_QUEUE_ROLE_RECEIVE], &sev) == -1)) {
    // PERROR("AMIPCBase: mq_notify");
    ret = errno;
  } else {
    ret = 0;
  }

  return ret;
}

//if msg queue role is not valid ( <0 ), it can create and assign
//other , report msg queue is already created error
int32_t AMIPCBase::create_msg_queue(const char *msg_queue_name, int32_t role)      //not directly used by user
{
  mqd_t msg_queue_new;
  struct mq_attr attr;
  int32_t oflag;

  //check name
  if (AM_UNLIKELY(check_msg_queue_name(msg_queue_name) < 0)) {
    PRINTF("AMIPCBase: msg queue name check failed\n");
    return -1;
  }

  if (AM_UNLIKELY((role< 0) || (role > 1))) {
    return -1;
  }

  //check role
  if ((m_msg_queue[role] >=0)) {
    //valid msg queue.  cannot create it again
    PRINTF("AMIPCBase: msg queue exist for role %d, cannot create it again \n", role);
    return -1;
  }

  //now create it (and delete it if existing before)

  attr.mq_flags = 0;
  attr.mq_maxmsg = max_messages;
  attr.mq_msgsize = max_message_size;
  attr.mq_curmsgs = 0;

  //PRINTF("AMIPCBase: unlink msg queue %s \n", m_msgqueue_name);
  mq_unlink ((char *)msg_queue_name);

  //the reason to use block send is to prevent send queue error because of queue full
  //the reason to use non block receive is to read all messages and quit the receive thread.

  if (role == IPC_MSG_QUEUE_ROLE_SEND)
    oflag = O_RDWR | O_CREAT | O_EXCL ; //send is  block send
  else
    oflag = O_RDWR | O_CREAT | O_EXCL | O_NONBLOCK; //receive is nonblock receive

  msg_queue_new = mq_open(msg_queue_name, oflag , S_IRWXU | S_IRWXG, &attr);

  if (msg_queue_new < 0 )  {
    PRINTF("AMIPCBase: Error Opening msg queue %s, msg queue may be not exist, or attr incorrect. ", msg_queue_name);
    PERROR("AMIPCBase: mq_open");
    return -1;
  } else {
    //copy the name
    m_msg_queue_name[role] = new char[IPC_MAX_MSGQUEUE_NAME_LENGTH+1];
    strcpy(m_msg_queue_name[role], msg_queue_name);

    m_msg_queue[role] = msg_queue_new;

    poll_queue_till_empty( m_msg_queue[role]);
    if (role == IPC_MSG_QUEUE_ROLE_RECEIVE) {
      if (register_receive_callback() < 0) {
        return -1;
      }
    }
  }

  //PRINTF("AMIPCBase: create msg queue %s Done,  queue id %d,  role %d  \n", msg_queue_name,  m_msg_queue[role], role);
  //print_queue_status(m_msg_queue[role]);

  return 0;
}

//bind to existing msg queue that created by other process
int32_t AMIPCBase::bind_msg_queue(char *msg_queue_name, int32_t role)
{
  mqd_t msg_queue_new;
  int32_t oflag;

  if (AM_UNLIKELY(!m_create_flag))
    return -1;

  //check name
  if (AM_UNLIKELY(check_msg_queue_name(msg_queue_name) < 0)) {
    PRINTF("AMIPCBase: msg queue name check failed\n");
    return -1;
  }

  if (AM_UNLIKELY((role< 0) || (role > 1))) {
    return -1;
  }

  //check role
  if (m_msg_queue[role] >=0) {
    //valid msg queue.  cannot bind it again
    PRINTF("AMIPCBase: msg queue exist for role %d, cannot bind it again \n", role);
    return -1;
  }

  //now bind it (must exist)
  //the reason to use block send is to prevent send queue error because of queue full
  //the reason to use non block receive is to read all messages and quit the receive thread.

  if (role == IPC_MSG_QUEUE_ROLE_SEND)
    oflag = O_WRONLY;        //send is write-only ,  ( no create flag),  block mode send
  else
    oflag = O_RDONLY  | O_NONBLOCK;        //receive is Non block receive

  msg_queue_new=  mq_open(msg_queue_name, oflag , S_IRWXU | S_IRWXG, NULL);
  if (msg_queue_new == -1)  {
    PERROR ("AMIPCBase: Error Opening msg queue in bind \n");
    return -1;
  } else {
    m_msg_queue[role] = msg_queue_new;
    if (role == IPC_MSG_QUEUE_ROLE_RECEIVE) {
      register_receive_callback_first_time(msg_queue_name);
    }
  }
  //PRINTF("AMIPCBase: bind msg queue %s Done, msg_queue id %d,  role %d \n", msg_queue_name,  m_msg_queue[role], role);
  return 0;
}

int32_t AMIPCBase::destroy_msg_queue(int32_t role)
{
  if (AM_UNLIKELY((role < 0 ) || (role > 1)))
    return -1;

  if (m_msg_queue[role] >= 0) {
    //      PRINTF("close  role %d !\n ", role);
    mq_close(m_msg_queue[role]);
    m_msg_queue[role] = -1;
  }

  //only if the msg queue is created,  m_msg_queue_name is not NULL
  //if it is binded to another msg queue created by someone else,
  //m_msg_queue_name is NULL
  if (m_msg_queue_name[role]!=NULL) {
    //remove the msg queue
    //PRINTF("AMIPCBase::destroy_msg_queue: Unlink %s \n",m_msg_queue_name[role]);
    //PRINTF("unlink  role %d !\n ", role);
    mq_unlink(m_msg_queue_name[role]);
    //free mem
    delete[] m_msg_queue_name[role];
    m_msg_queue_name[role] = NULL;
  }

  return 0;
}


int32_t AMIPCBase::destroy()
{
  //PRINTF("AMIPCBase::destroy \n");
  //mark the class is being destroyed , so it will not handle more async event
  //because of this check, a AMIPCBase object can be "destroyed" many times,
  //if it has been destroyed or not created, then it will just return -1
  if (AM_UNLIKELY(!m_create_flag))
    return -1;

  m_create_flag = 0;

  if (destroy_msg_queue(IPC_MSG_QUEUE_ROLE_SEND) < 0) {
    PRINTF("AMIPCBase: unable to destroy send queue \n");
    return -1;
  }

  if (destroy_msg_queue(IPC_MSG_QUEUE_ROLE_RECEIVE) < 0) {
    PRINTF("AMIPCBase: unable to destroy receive queue \n");
    return -1;
  }

  if (m_receive_buffer) {
    delete[] m_receive_buffer;
    m_receive_buffer = NULL;
  }

  //when m_create_flag has been cleared, the object has been deleted,
  //so following receive function will not be successful

  return 0;
}

void AMIPCBase::dump_receive_buffer()
{
  int32_t i, j;
  uint8_t * pdata = (uint8_t *)m_receive_buffer;

  PRINTF("RECEIVE BUFFER DUMP: \n");
  for (i = 0; i< 16 ; i++ )  {
    for ( j =0; j< 16; j++) {
      PRINTF("%4x ", pdata[i* 16 + j]);
    }
    PRINTF("\n");
  }
}

int32_t AMIPCBase::debug_print_queue_info()
{
  char tempbuffer[256];
  struct mq_attr attr;

  debug_print_process_name(tempbuffer, 255);
  if ( m_msg_queue[IPC_MSG_QUEUE_ROLE_SEND] != -1) {
    if  (mq_getattr(m_msg_queue[IPC_MSG_QUEUE_ROLE_SEND] , &attr) < 0) {
      PRINTF("AMIPCBase:  get msg queue attr error \n");
      return -1;
    }
    PRINTF("DEBUG IPC: Process %s,  Send IPC current msgs %ld \n", tempbuffer, attr.mq_curmsgs );
  }

  if (m_msg_queue[IPC_MSG_QUEUE_ROLE_RECEIVE] != -1) {
    if (mq_getattr(m_msg_queue[IPC_MSG_QUEUE_ROLE_RECEIVE], &attr) < 0) {
      PRINTF("AMIPCBase:  get msg queue attr error \n");
      return -1;
    }
    PRINTF("DEBUG IPC: Process %s,  Receive IPC current msgs %ld \n", tempbuffer, attr.mq_curmsgs );
  }

  return 0;
}
