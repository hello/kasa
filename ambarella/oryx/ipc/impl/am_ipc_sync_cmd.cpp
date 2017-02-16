
/*
 * am_ipc_sync_cmd.cpp
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
//return 0 if msg queue name is OK,  return -1 if not OK
static inline int32_t check_identifier_name(const char *identifier_name)
{
  int32_t length;
  //check name
  if (identifier_name == NULL)
    return -1;
  if (identifier_name[0]!='/')
    return -1;
  length = strlen(identifier_name);
  if ((length >= AM_MAX_SEM_NAME_LENGTH - 4) || (length < 2))
    return -1;

  return 0;
}

/*************************************************************************

         Below is AMIPCSyncCmdClient

 **************************************************************************/

AMIPCSyncCmdClient::AMIPCSyncCmdClient()
{
  m_create_sem = NULL;
  m_sem_name[0] = '\0';
  m_connected_flag = 0;
}

AMIPCSyncCmdClient::~AMIPCSyncCmdClient()
{
  destroy();
}


int32_t AMIPCSyncCmdClient::destroy()
{
  //PRINTF("AMIPCSyncCmdClient:: destroy 1 \n");
  //discconnect to server first
  send_connection_state(0);

  //must call base class to destroy first, because the sem_post below will let all
  //waiting process to execute immediately.  so put sem_post to be last, can avoid
  //race conditions
  AMIPCBase::destroy();
  //    PRINTF("AMIPCSyncCmdClient:: destroy 2 \n");

  if (m_create_sem){
    if (m_connected_flag) {
      // PRINTF("AMIPCSyncCmdClient::destroy: sem_post(m_create_sem) \n");
      if (sem_post(m_create_sem) < 0) {
        PRINTF("sem_post error for %s \n",m_sem_name);
        return -1;
      } else {
        //PRINTF("AMIPCSyncCmdClient: sem_post for next Sender to connect \n");
      }
    }

    if (sem_close(m_create_sem) < 0) {
      PRINTF("sem_close error for %s \n", m_sem_name);
      return -1;
    }
  }

  //house keeping, because destroy is not deconstructor
  m_create_sem = NULL;
  m_connected_flag = 0;
  m_sem_name[0] = '\0';
  return 0;
}

//state 0: disconnected 1 : connected()
int32_t AMIPCSyncCmdClient::send_connection_state(int32_t state)
{
  int32_t ret;
 // INFO("AMIPCSyncCmdClient:: try to send_connection_done \n");
  ret = method_call(AM_IPC_CMD_ID_CONNECTION_DONE, &state, sizeof(int32_t), NULL, 0);
  //INFO("AMIPCSyncCmdClient:: send_connection_done finish\n");
  return ret;
}

int32_t AMIPCSyncCmdClient::create(const char *unique_identifier)
{
  struct timespec wait_time;
  sem_t * p_sem=NULL;
  int32_t ret;
  //    char cmdline[256];
  int32_t i;
  int32_t sem_ok = 0;

  //INFO("AMIPCSyncCmdClient:: create %s \n", unique_identifier);
  if (AM_UNLIKELY(check_identifier_name(unique_identifier) < 0)) {
    PRINTF("identifier name error %s \n", unique_identifier);
    return AM_IPC_CMD_ERR_INVALID_ARG;
  }
  //    debug_print_process_name(cmdline, sizeof(cmdline));
  //    PRINTF("%s::AMIPCSyncCmdClient:: try to create... \n", cmdline);

  //retry 200 times
  for (i=0; i<200; i++) {
    p_sem = sem_open(unique_identifier, O_RDWR);
    if (p_sem == SEM_FAILED) {
      //sleep 100ms and retry
      //INFO("AMIPCSyncCmdClient:  sem_open failed, sleep and retry\n");
      usleep(100*1000L);
    } else {
      //INFO("AMIPCSyncCmdClient:  sem_open OK\n");
      sem_ok = 1;
      break;
    }
  }

  if (AM_UNLIKELY(!sem_ok)) {
    PRINTF("AMIPCSyncCmdClient : sem_open failed! (%s)\n", strerror(errno));
    PRINTF("AMIPCSyncCmdClient : try 100  times, but still unable to create the SEM for IPC \n");
    return AM_IPC_CMD_ERR_INVALID_IPC;
  }

  //now m_create_sem must be valid
  m_create_sem = p_sem;
  strcpy(m_sem_name, unique_identifier);
  wait_time.tv_sec = time(NULL) + AM_IPC_SYNC_CMD_MAX_DELAY;
  wait_time.tv_nsec = 0;


  ret = sem_timedwait(m_create_sem, &wait_time);
  if (ret < 0) {
    PRINTF("AMIPCSyncCmdClient:create:error on waiting for sem %s \n", m_sem_name);
    perror("sem_timedwait \n");
    return AM_IPC_CMD_ERR_INVALID_IPC;
  }


  if (AM_UNLIKELY((ret = AMIPCBase::create(NULL, NULL)) < 0)) {
    PRINTF("AMIPCSyncCmdClient:AMIPCBase:create failed \n");
    return AM_IPC_CMD_ERR_INVALID_IPC;
  }

  if (bind(unique_identifier) < 0){
    PRINTF("AMIPCSyncCmdClient:: bind failed \n");
    return AM_IPC_CMD_ERR_INVALID_IPC;
  }


  //send connect done msg to receiver
  if (send_connection_state(1) < 0) {
    PRINTF("AMIPCSyncCmdClient:: send connect done flag failed \n");
    return AM_IPC_CMD_ERR_INVALID_IPC;
  }

  //now it's OK
  m_connected_flag  = 1;

  //statistics
  //debug_print_queue_info();
  return AM_IPC_CMD_SUCCESS;
}

int32_t AMIPCSyncCmdClient::cleanup(const char*unique_identifier)
{
  //clean up the semaphore
  if (unique_identifier) {
    //INFO("AMIPCSyncCmdClient: unlink the semaphore %s\n", unique_identifier);
    sem_unlink(unique_identifier);
  }
  return 0;
}


int32_t AMIPCSyncCmdClient::bind(const char *unique_identifier)
{
  int32_t ret;
  char name_msg_queue[IPC_MAX_MSGQUEUE_NAME_LENGTH +1] = {0};

  //make the msg queue names by appending 0 (for DOWN direction), and 1 (for UP direction)
  //this is receiver role,  so UP direction is for send, and DOWN direction is for receive
  sprintf(name_msg_queue, "%s0",unique_identifier);
  ret = AMIPCBase::bind_msg_queue(name_msg_queue,IPC_MSG_QUEUE_ROLE_SEND);
  if (ret < 0) {
    PRINTF("AMIPCSyncCmdClient:bind msg queue %s role %d error \n", name_msg_queue,IPC_MSG_QUEUE_ROLE_SEND);
    return -1;
  }
  sprintf(name_msg_queue,  "%s1",unique_identifier);
  ret = AMIPCBase::bind_msg_queue(name_msg_queue,IPC_MSG_QUEUE_ROLE_RECEIVE);
  if (ret < 0) {
    PRINTF("AMIPCSyncCmdClient:bind msg queue %s role %d error \n", name_msg_queue,IPC_MSG_QUEUE_ROLE_RECEIVE);
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
    return AMIPCCmdSender::method_call(cmd_id, cmd_arg_s, cmd_size, return_value_s, max_return_value_size);
  } else {
    char processname[256] = {0};
    debug_print_process_name(processname, 255);
    PRINTF("AMIPCSyncCmdClient: No active IPC connection, Ignore cmd, by Process %s tries to do method_call 0x%x while connection state is off \n", processname, cmd_id);
    return AM_IPC_CMD_ERR_IGNORE;
  }
}

/*************************************************************************

         Below is AMIPCSyncCmdServer

 **************************************************************************/

AMIPCSyncCmdServer::AMIPCSyncCmdServer()
{
  m_create_sem = NULL;
  m_sem_name[0] = '\0';
  m_connection_state = 0; //connection is by default "disconnected"
}

AMIPCSyncCmdServer::~AMIPCSyncCmdServer()
{
  destroy();
}

int32_t AMIPCSyncCmdServer::destroy()
{
  //PRINTF("AMIPCSyncCmdServer:: destroy \n");
  //disconnect notify
  m_connection_state = 0;
  //call base class's destroy method first, to prevent race condition.
  AMIPCBase::destroy();

  if (m_create_sem){
    if (AM_UNLIKELY(sem_close(m_create_sem) < 0)) {
      PRINTF("sem_close error for %s \n", m_sem_name);
      return -1;
    }
  }

  //unlink it regardless if it's created by AMIPCSyncCmdServer or not
  if (AM_LIKELY(m_sem_name[0]!='\0')) {
    //INFO("AMIPCSyncCmdServer::destroy and unlink sem %s\n", m_sem_name);
    sem_unlink(m_sem_name);
  }

  //house keeping , because destroy is not deconstructor
  m_create_sem = NULL;
  m_sem_name[0] = '\0';
  return 0;
}


void AMIPCSyncCmdServer::connection_callback(uint32_t context, void *msg_data,
                                             int32_t msg_data_size,
                                             void *result_addr,
                                             int32_t result_max_size)
{
  AMIPCSyncCmdServer * p_this;

  if ((!context) || (!msg_data)) {
    PRINTF("AMIPCSyncCmdServer::connection_callback error\n");
  } else {
    p_this = (AMIPCSyncCmdServer *) context;
    p_this->m_connection_state =*((int32_t*)msg_data);
  }
}

int32_t AMIPCSyncCmdServer::create(const char *unique_identifier)
{
  int32_t ret_value = -1;
  int32_t ret = AM_IPC_CMD_SUCCESS;
  sem_t *p_sem=NULL;
  char name_msg_queue[IPC_MAX_MSGQUEUE_NAME_LENGTH +1] = {0};

  am_msg_handler_t msg_handler;

  do {
    if (AM_UNLIKELY(check_identifier_name(unique_identifier) < 0)) {
      PRINTF("identifier name error %s \n", unique_identifier);
      ret = AM_IPC_CMD_ERR_INVALID_ARG;
      break;
    }

    //remove the sem
    //INFO("AMIPCSyncCmdServer: unlink the semaphore %s\n", unique_identifier);
    sem_unlink(unique_identifier);
    //try to create as EXCL, and put initial value to 0
    p_sem = sem_open(unique_identifier, O_RDWR | O_CREAT | O_EXCL , S_IRWXU | S_IRWXG,  0  );
    if (p_sem == SEM_FAILED) {
      perror("CIPCSsyncCmdReceiver create sem Error:");
      PRINTF("CIPCSsyncCmdReceiver :  %s\n", unique_identifier);
      ret = AM_IPC_CMD_ERR_INVALID_IPC;
      break;
    }

    m_create_sem = p_sem;
    strcpy(m_sem_name, unique_identifier);

    //make the msg queue names by appending 0 (for DOWN direction), and 1 (for UP direction)
    //this is receiver role,  so UP direction is for send, and DOWN direction is for receive
    sprintf(name_msg_queue, "%s1",unique_identifier);
    ret_value = AMIPCBase::create_msg_queue(name_msg_queue, IPC_MSG_QUEUE_ROLE_SEND);
    if (ret_value < 0) {
      PRINTF("AMIPCSyncCmdServer:create msg queue %s, role %d error \n",name_msg_queue, IPC_MSG_QUEUE_ROLE_SEND);
      ret = AM_IPC_CMD_ERR_INVALID_IPC;
      break;
    }

    sprintf(name_msg_queue, "%s0",unique_identifier);
    ret_value = AMIPCBase::create_msg_queue(name_msg_queue, IPC_MSG_QUEUE_ROLE_RECEIVE);
    if (ret_value < 0) {
      PRINTF("AMIPCSyncCmdServer:create msg queue %s, role %d error \n",name_msg_queue, IPC_MSG_QUEUE_ROLE_RECEIVE);
      ret = AM_IPC_CMD_ERR_INVALID_IPC;
      break;
    }

    ret_value = AMIPCBase::create(NULL, NULL);
    if (ret_value < 0) {
      PRINTF("AMIPCSyncCmdServer:create msg queue NULL, NULL error \n");
      ret = AM_IPC_CMD_ERR_INVALID_IPC;
      break;
    }

    //add default message handler for IPC_CMD_ID_CONNECTION_DONE

    msg_handler.msgid = AM_IPC_CMD_ID_CONNECTION_DONE;
    msg_handler.callback = NULL;
    msg_handler.context =  (uint32_t)this;
    msg_handler.callback_ct = AMIPCSyncCmdServer::connection_callback;
    ret_value = register_msg_proc(&msg_handler);
    if (ret_value < 0) {
      ret = AM_IPC_CMD_ERR_INVALID_IPC;
      break;
    }

  } while(0);

  //INFO("AMIPCSyncCmdServer:: create %s OK \n",unique_identifier);
  return ret;
}

int32_t AMIPCSyncCmdServer::complete()
{
  if (AM_UNLIKELY(m_create_sem == NULL)) {
    PRINTF("AMIPCSyncCmdServer:notify_init_complete:sem not created \n");
    return -1;
  }

  //debug_print_queue_info();
  //INFO("AMIPCSyncCmdServer:: complete \n");
  return sem_post(m_create_sem);
}

int32_t AMIPCSyncCmdServer::notify(uint32_t cmd_id,
                                   void *cmd_arg_s,
                                   int32_t cmd_size)
{
  if (m_connection_state) {
    return AMIPCCmdReceiver::notify(cmd_id, cmd_arg_s, cmd_size);
  } else {
    //for debug only, for notify missing
    char cmdline[256];
    debug_print_process_name(cmdline, 255);
    PRINTF("Process %s send notification, but destination not ready , ignored \n",
           cmdline);
    PRINTF("Warning: notify 0x%x dropped \n", cmd_id);
    return AM_IPC_CMD_ERR_IGNORE;
  }
}
