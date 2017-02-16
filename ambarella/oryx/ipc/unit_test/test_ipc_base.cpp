
/*
 * test_ipc_base.cpp
 *
 * History:
 *    2013/2/26 - [Louis Sun] Create
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
#include "am_ipc_base.h"
#include "am_log.h"

AMIPCBase  *ipc_base_obj = NULL;

static void sigstop(int arg)
{

}

int main(int argc, char *argv[])
{
  char send_msg_queue_name[128];
  char receive_msg_queue_name[128];
  char buffer[128];
  int test_ipc_id = 0;
  int i;

  ipc_base_obj = new AMIPCBase;

  signal(SIGINT, sigstop);
  signal(SIGQUIT, sigstop);
  signal(SIGTERM, sigstop);

  if (argc < 2) {
    PRINTF(" test_ipc_base 0 or 1 \n");
    return 0;
  }

  test_ipc_id = atoi(argv[1]);

  PRINTF("This is IPC ID %d \n", test_ipc_id);

  if (test_ipc_id==0) {
    strcpy(send_msg_queue_name,"/MSGQUEUE_TEST0");
    strcpy(receive_msg_queue_name, "/MSGQUEUE_TEST1");
  } else {
    strcpy(send_msg_queue_name,"/MSGQUEUE_TEST1");
    strcpy(receive_msg_queue_name, "/MSGQUEUE_TEST0");
  }

  if (ipc_base_obj->create(send_msg_queue_name, NULL) < 0) {
    PRINTF("create ipc_base obj failed \n");
    return -1;
  }

  sleep(2);

  while (ipc_base_obj->bind_msg_queue(receive_msg_queue_name, IPC_MSG_QUEUE_ROLE_RECEIVE) < 0) {
    usleep(1000 * 10);
  }

  //simple way to sync in test only
  sleep(2);

  for (i=0; i< 20; i++) {
    sprintf(buffer, "hello from ipc %d,  No. %d \n", test_ipc_id,  i+1);
    if (ipc_base_obj->send(buffer, sizeof(buffer)) < 0) {
      PRINTF("send error \n");
    }

  }
  pause();
  PRINTF("exit IPC %d \n", test_ipc_id);


  delete ipc_base_obj;

  return 0;
}
