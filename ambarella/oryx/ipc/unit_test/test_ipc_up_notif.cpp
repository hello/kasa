
/*
 * test_ipc_up_notif.cpp
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
  PRINTF("Got Up Direction Notification...  no return\n");
}

int main(int argc, char *argv[])
{
  char  send_msg_queue_name[128];
  char  receive_msg_queue_name[128];
  int test_ipc_id = 0;
  am_msg_handler_t  handler;

  signal(SIGINT, sigstop);
  signal(SIGQUIT, sigstop);
  signal(SIGTERM, sigstop);

  if (argc < 2) {
    PRINTF(" test_ipc_up_notif 0 or 1,  run two process at same time,  0 is server, 1 is client  \n");
    return 0;
  }

  test_ipc_id = atoi(argv[1]);
  PRINTF("This is IPC ID %d \n", test_ipc_id);

  if (test_ipc_id==0) {
    strcpy(send_msg_queue_name,"/MSGQUEUE_TEST0");
    strcpy(receive_msg_queue_name,  "/MSGQUEUE_TEST1");
  } else {
    strcpy(send_msg_queue_name,"/MSGQUEUE_TEST1");
    strcpy(receive_msg_queue_name,  "/MSGQUEUE_TEST0");
  }

  if (test_ipc_id ==0) {
    //server
    PRINTF(" start AMIPCCmdReceiver (server ) \n");
    ipc_cmd_obj = new AMIPCCmdReceiver;
    ipc_cmd_obj ->create(send_msg_queue_name, receive_msg_queue_name);

  } else {
    //client
    PRINTF(" start AMIPCCmdSender (client ) \n");
    ipc_cmd_obj = new AMIPCCmdSender;
    ipc_cmd_obj->create(NULL, NULL);  //does not create new queues, but bind to other queues
    if (ipc_cmd_obj ->bind_msg_queue(send_msg_queue_name, 0) < 0) {
      PRINTF("bind send queue failed \n");
      return -1;
    }

    if (ipc_cmd_obj ->bind_msg_queue(receive_msg_queue_name, 1) < 0) {
      PRINTF("bind receive queue failed \n");
      return -1;
    }

    PRINTF("Client side bind msg queue OK, register msg proc \n");
    //register proc
    memset(&handler, 0, sizeof(handler));
    handler.msgid = AM_IPC_CMD_ID_TEST_NOTIF;
    handler.callback = handler1;
    ipc_cmd_obj->register_msg_proc(&handler);
    PRINTF("Client side register msg proc done\n");
  }

  //now the bind should be OK.  try method call.

  if (test_ipc_id == 0) {
    //this is server

    AMIPCCmdReceiver  *psender = (AMIPCCmdReceiver *)ipc_cmd_obj;
    sleep(3);  //wait until  AMIPCCmdSender has been created and registered
    PRINTF("Server side sends UP Notification\n");
    if (psender->notify(AM_IPC_CMD_ID_TEST_NOTIF,  NULL, 0) < 0) {
      PRINTF("method_call failed \n");
      return -1;
    }
    PRINTF("Server side finishes and waiting for exit\n");
    pause();
    PRINTF("Server Exit\n");
  } else {
    //this is client
    pause();
    PRINTF("Client Exit\n");
  }

  delete ipc_cmd_obj;
  return 0;
}
