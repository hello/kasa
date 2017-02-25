/*
 * test_ipc_sync_cmd.cpp
 *
 * History:
 *    2013/3/15 - [Louis Sun] Create
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include "am_base_include.h"
#include "am_ipc_sync_cmd.h"
#include "am_log.h"

static AMIPCBase  *ipc_cmd_obj = nullptr;

//capture the signal but does nothing (no exit either)
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

  PRINTF("try to run %d+%d \n",  a, b );

  *result = a + b;
}

int main(int argc, char *argv[])
{
  char  msg_queue_pair_name[128];
  int test_ipc_id = 0;
  int i;

  AMIPCSyncCmdServer * pcmdreceiver;
  AMIPCSyncCmdClient  *pcmdsender;

  am_msg_handler_t  handler;
  signal(SIGINT, sigstop);
  signal(SIGQUIT, sigstop);
  signal(SIGTERM, sigstop);

  if (argc < 2) {
    PRINTF(" test_ipc_sync_cmd 0 or 1,  run two process at same time,  0 is server, 1 is client  \n");
    return 0;
  }

  test_ipc_id = atoi(argv[1]);

  PRINTF("This is IPC ID %d \n", test_ipc_id);

  strcpy(msg_queue_pair_name,"/MSGQUEUE_TEST_SYNC_CMD");

  if (test_ipc_id ==0) {
    PRINTF(" start AMIPCSyncCmdServer (server ) \n");
    pcmdreceiver = new AMIPCSyncCmdServer;
    if (pcmdreceiver->create(msg_queue_pair_name) < 0) {
      PRINTF("receiver created failed \n");
    }
    ipc_cmd_obj = pcmdreceiver; //copy to global for "sigterm handler"

    //register proc
    memset(&handler, 0, sizeof(handler));
    handler.msgid = AM_IPC_CMD_ID_TEST_ONLY;
    handler.callback = handler1;
    pcmdreceiver->register_msg_proc(&handler);
    pcmdreceiver->complete();
  } else {
    PRINTF(" start CIPCCmdSender (client ) \n");
    pcmdsender = new AMIPCSyncCmdClient;
    if (pcmdsender->create(msg_queue_pair_name) < 0) {
      PRINTF("sender create failed \n");
      return -1;
    }
    ipc_cmd_obj = pcmdsender;   //copy to global for "sigterm handler"
  }

  //now the bind should be OK.  try method call.
  if (test_ipc_id ==1) {
    {
      int cmd_arg[2];
      int cmd_result[2];
      int ret;
      cmd_arg[0]= 100;
      cmd_arg[1]=99;
      cmd_result[0]=0;
      cmd_result[1]=0;

      PRINTF("Perform method_call for 50 times: \n");
      for (i=0;i<50;i++) {
        ret = pcmdsender->method_call(AM_IPC_CMD_ID_TEST_ONLY,  cmd_arg, 2* sizeof(int), cmd_result, sizeof(int));
        PRINTF("cmd_result[0] = %d ,  ret = %d\n",  cmd_result[0],   ret);
      }
    }

    //this is client
    PRINTF("Client Exit\n");
  } else {
    //this is server
    pause();
    PRINTF("Server Exit\n");
  }

  //unified exit
  if (ipc_cmd_obj)
      delete ipc_cmd_obj;

  return 0;
}
