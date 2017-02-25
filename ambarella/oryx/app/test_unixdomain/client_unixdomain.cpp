/*******************************************************************************
 * client_unixdomain.cpp
 *
 * History:
 *   2016-07-18 - [Guohua Zheng]  created file
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
 *
 ******************************************************************************/

#include "am_base_include.h"
#include "am_define.h"
#include "am_log.h"

#include "am_export_if.h"

#include <signal.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <condition_variable>
#include <sys/time.h>
#include <getopt.h>

#define DEXPORT_PATH ("/run/oryx/export.socket")

#define NO_ARG 0
#define HAS_ARG 1

int m_socket_fd = -1;
sockaddr_un m_addr = {0};
bool running_flag = true;
bool print_flag = true;
int buffer_size = 1024;
int cycle_num = 500;

static void show_usage()
{
  printf("test_client_unixdomain usage:\n");
  printf("\t-p [%%s]: specify packet size of transmitted, "
      "recommend 64 Byte for small, 1024 Byte for large, "
      "same with '--packet'\n");
  printf("\t-s [%%s]: specify statistics frequency, "
      "same with '--statistics'\n");
  printf("\t-l: specify output mode, 0: on-loop; 1: loop,"
      "same with '--loop'\n");
  printf("\t-n: specify no print,"
        "same with '--no-print'\n");
  printf("\t--help: show usage\n");
};

static struct option long_options[] =
{
 {"help",               NO_ARG,   0,  'h'},
 {"packet",             HAS_ARG,  0,  'p'},
 {"statistics",         HAS_ARG,  0,  's'},
 {"loop" ,              HAS_ARG,  0,  'l'},
 {"no-print",           NO_ARG,   0,  'n'}
};

static const char *short_options = "hp:s:l:n";

static int init_param(int argc, char **argv)
{
  int ch;
  int option_index = 0;
  while ((ch = getopt_long(argc, argv,
                           short_options,
                           long_options,
                           &option_index)) != -1) {
    switch (ch) {
      case 'h':
        show_usage();
        return -1;
      case 'p':
        buffer_size = atoi(optarg);
        break;
      case 's':
        cycle_num = atoi(optarg);
        break;
      case 'l':
        running_flag = atoi(optarg);
        break;
      case 'n':
        print_flag = false;
        break;
    }
  }
  return 0;

};

static bool connect_server(const char *url)
{
  bool ret = false;
  do {
    if (!url) {
      ERROR("Connect(url == NULL)");
      break;
    }
    if (access(url, F_OK)) {
      ERROR("Failed to access(%s, F_OK). server does not run?", url);
      break;
    }
    if ((m_socket_fd = socket(PF_UNIX, SOCK_SEQPACKET, 0)) < 0) {
      ERROR("socket(PF_UNIX, SOCK_SEQPACKET, 0) failed");
      break;;
    }
    memset(&m_addr, 0, sizeof(sockaddr_un));
    m_addr.sun_family = AF_UNIX;
    snprintf(m_addr.sun_path, sizeof(m_addr.sun_path), "%s", url);
    if (connect(m_socket_fd, (sockaddr*)&m_addr, sizeof(sockaddr_un)) < 0) {
      ERROR("connect failed: %s", strerror(errno));
      break;
    }
    ret = true;
  } while (0);
  return ret;
};

static bool send_content(int socket_fd)
{
  char *content = new char[buffer_size];
  for (int i = 0; i < buffer_size -1; i++) {
    content[i] = 'a';
  }
  content[buffer_size - 1] = '\0';
  if ((write(socket_fd, content, buffer_size)) != buffer_size) {
    delete [] content;
    return false;
  } else {
    delete [] content;
    return true;
  }
};

static bool receive_content(int socket_fd)
{
  char *content = new char[buffer_size];
  if ((read(socket_fd, content, buffer_size)) != buffer_size) {
    delete [] content;
    return false;
  } else {
    delete [] content;
    return true;
  }
};

static void __sigstop(int i)
{
  running_flag = false;
};


int main(int argc, char *argv[])
{
  if (argc < 2) {
    show_usage();
    return -1;
  }
  int ret = 0;
  int success_num = 0;
  double sec_t = 0.0;
  double usec_t = 0.0;
  double ret_final = 0.0;
  bool init_flag = true;
  signal(SIGINT, __sigstop);
  signal(SIGQUIT, __sigstop);
  signal(SIGTERM, __sigstop);
  if (init_param(argc, argv) < 0) {
    return -1;
  }
  do {
    if ((connect_server(DEXPORT_PATH))) {
      do {
        for (int i = 0; i < cycle_num; i++) {
          struct timeval send_t, receive_t;
          gettimeofday(&send_t, NULL);
          if (send_content(m_socket_fd)) {
            if (receive_content(m_socket_fd)) {
              gettimeofday(&receive_t, NULL);
              usec_t += receive_t.tv_usec - send_t.tv_usec;
              sec_t += receive_t.tv_sec - send_t.tv_sec;
              success_num++;
            }
          }
        }
        if (print_flag) {
          printf("statistic %d with packet size %d:  %f ms\n",
                 cycle_num,
                 buffer_size,
                 ((usec_t / 1000 + sec_t * 1000) / success_num ));
        }
        if (init_flag) {
          ret_final = (usec_t / 1000 + sec_t * 1000) / success_num;
          init_flag = false;
        } else {
          ret_final += (usec_t / 1000 + sec_t * 1000) / success_num;
          ret_final /= 2;
        }
        success_num = 0;
        usec_t = 0;
        sec_t = 0;
      } while(running_flag);
    } else {
      ERROR("connect failed");
    }
  } while (0);
  printf("Final statistic is %f ms\n", ret_final);
  return ret;
}
