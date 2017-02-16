
/*
 * test_ipc_cmd.cpp
 *
 * History:
 *    2013/3/12 - [Louis Sun] Create
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
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include "am_base_include.h"
#include "am_ipc_cmd.h"
#include "am_log.h"

AMIPCBase  *ipc_cmd_obj = NULL;

static void sigstop(int arg)
{

}

static void handler1(void *msg_data, int msg_data_size, void * result_addr,  int result_max_size)
{
  uint32_t  a, b;
  uint32_t * p_input = (uint32_t *)msg_data;
  uint32_t * result = (uint32_t *)result_addr;

  a = p_input[0];
  b = p_input[1];

  printf("try to run %d+%d \n",  a, b );

  *result = a + b;

}

int main(int argc,  char *argv[])
{
  char  send_msg_queue_name[128];
  char  receive_msg_queue_name[128];

  int test_ipc_id = 0;

  am_msg_handler_t  handler;

  signal(SIGINT, sigstop);
  signal(SIGQUIT, sigstop);
  signal(SIGTERM, sigstop);

  if (argc < 2) {
    printf(" test_ipc_cmd 0 or 1, run two process at same time,  0 is server, 1 is client  \n");
    return 0;
  }

  test_ipc_id = atoi(argv[1]);

  printf("This is IPC ID %d \n", test_ipc_id);

  if (test_ipc_id== 0) {
    strcpy(send_msg_queue_name,"/MSGQUEUE_TEST0");
    strcpy(receive_msg_queue_name,  "/MSGQUEUE_TEST1");
  } else {
    strcpy(send_msg_queue_name,"/MSGQUEUE_TEST1");
    strcpy(receive_msg_queue_name,  "/MSGQUEUE_TEST0");
  }

  if (test_ipc_id == 0) {
    printf(" start AMIPCCMdReceiver (server ) \n");
    ipc_cmd_obj = new AMIPCCmdReceiver;
    ipc_cmd_obj ->create(send_msg_queue_name, receive_msg_queue_name);

    //register proc
    memset(&handler, 0, sizeof(handler));
    handler.msgid = AM_IPC_CMD_ID_TEST_ONLY;
    handler.callback = handler1;

    ipc_cmd_obj->register_msg_proc(&handler);
  } else {
    printf(" start AMIPCCmdSender (client ) \n");
    ipc_cmd_obj = new AMIPCCmdSender;
    ipc_cmd_obj->create(NULL, NULL);  //does not create new queues, but bind to other queues
    if (ipc_cmd_obj ->bind_msg_queue(send_msg_queue_name, 0) < 0) {
      printf("bind send queue failed \n");
      return -1;
    }

    if (ipc_cmd_obj ->bind_msg_queue(receive_msg_queue_name, 1) < 0) {
      printf("bind receive queue failed \n");
      return -1;
    }
  }

  //now the bind should be OK.  try method call.

  if (test_ipc_id ==1) {
    {
      int cmd_arg[2];
      int cmd_result[2];
      int ret;
      AMIPCCmdSender * psender= (AMIPCCmdSender *) ipc_cmd_obj;
      cmd_arg[0]= 100;
      cmd_arg[1]=99;
      cmd_result[0]=0;
      cmd_result[1]=0;

      //make sure method_call is after bind connection
      ret = psender->method_call(AM_IPC_CMD_ID_TEST_ONLY,  cmd_arg, 2* sizeof(int), cmd_result, sizeof(int));
      printf("cmd_result[0] = %d ,  ret = %d\n",  cmd_result[0],   ret);
    }

    //this is client
    PRINTF("Client Exit\n");
  } else {
    //this is server
    pause();
    PRINTF("Server Exit\n");
  }

  delete ipc_cmd_obj;


  return 0;
}
