/*******************************************************************************
 * test_ipc_uds.cpp
 *
 * History:
 *   Jul 28, 2016 - [Shupeng Ren] created file
 *
 * Copyright (c) 2015 Ambarella, Inc.
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

#include <signal.h>

#include "am_base_include.h"
#include "am_log.h"
#include "am_ipc_uds.h"

#define UNIX_DOMAIN_PATH "/tmp/ipc_unix_domain_test"

bool run_flag = true;
AMIPCServerUDSPtr server = nullptr;
AMIPCClientUDSPtr client = nullptr;

static void connect_handler(int32_t context, int fd)
{
  PRINTF("Connect a client: %d", fd);
}

static void server_receive_handler(int32_t context, int fd)
{
  PRINTF("Get fd[%d] %d bytes msg: %s", fd);
  //server->send(fd, data, size);
}

void client_receive_handler(int32_t context, int fd)
{
PRINTF("Get fd[%d]", fd);
}

static void usage(int argc, char **argv)
{
  PRINTF(" \"%s s\" run as a server.\n \"%s c\" run as a client.",
         argv[0], argv[0]);
}

static void signal_handler(int sig)
{
  run_flag = false;
}

int main(int argc, char **argv)
{
  int ret = 0;
  enum {
    ROLE_INIT = 0,
    ROLE_SERVER = 1,
    ROLE_CLIENT = 2
  } role = ROLE_INIT;

  do {
    if (argc != 2) {
      ret = -1;
      usage(argc, argv);
      break;
    }
    signal(SIGINT, signal_handler);
    signal(SIGQUIT, signal_handler);
    signal(SIGTERM, signal_handler);

    if (!strcmp(argv[1], "s")) { //Run as server
      role = ROLE_SERVER;
    } else if (!strcmp(argv[1], "c")) { //Run as client
      role = ROLE_CLIENT;
    } else {
      ret = -2;
      usage(argc, argv);
      break;
    }

    if (role == ROLE_SERVER) {
      if (!(server = AMIPCServerUDS::create(UNIX_DOMAIN_PATH))) {
        ret = -3;
        ERROR("Failed to create server instance!");
        break;
      }
      server->register_connect_cb(connect_handler);
      server->register_receive_cb(server_receive_handler);
      while (run_flag) {
        sleep(3);
      }
    } else if (role == ROLE_CLIENT) {
      if (!(client = AMIPCClientUDS::create())) {
        ret = -4;
        ERROR("Failed to create client instance!");
        break;
      }
      sleep(2);
      if (client->connect(UNIX_DOMAIN_PATH) != AM_RESULT_OK) {
        ret = -5;
        break;
      }
      client->register_receive_cb(client_receive_handler);
      uint8_t send_data[] = "Data from client!";
      uint8_t recv_data[128] = {0};
      int32_t send_size = sizeof(send_data);
      int32_t recv_size = sizeof(recv_data);
      while (run_flag) {
        if (client->send(send_data, send_size) != AM_RESULT_OK) {
          ret = -6;
          break;
        }

        if (client->recv(recv_data, recv_size) != AM_RESULT_OK) {
          ret = -7;
          break;
        }
        PRINTF("Get fd %d bytes msg: %s", recv_size, recv_data);
        //sleep(2);
      }
      client->distconnect();
    }
  } while (0);

  return ret;
}
