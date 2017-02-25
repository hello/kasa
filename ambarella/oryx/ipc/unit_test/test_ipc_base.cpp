
/*
 * test_ipc_base.cpp
 *
 * History:
 *    2013/2/26 - [Louis Sun] Create
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
#include "am_ipc_base.h"
#include "am_log.h"

AMIPCBase  *ipc_base_obj = nullptr;

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

  if (ipc_base_obj->create(send_msg_queue_name, nullptr) < 0) {
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
