/*******************************************************************************
 * af_value_server.cpp
 *
 * Histroy:
 *   2014-3-06 - [HuaiShun Hu] created file
 *
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
 */


#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/time.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <errno.h>

#include "basetypes.h"
#include "ambas_imgproc_arch.h"
#include "ambas_imgproc_ioctl_arch.h"

#define SERVER_PORT 5678
#define MAX_BUFFER_SIZE 200
#define ONE_SECOND_IN_US (1000 * 1000)
#define PREFIX  "[AF VALUE SERVER] "
#define SCENE_ROI_WIDTH 3
#define SCENE_ROI_HEIGHT 3

enum {
  STOP_SERVER = 0,
  START_SERVER = 1,
  HEARTBEAT = 2,
  AF_DATA = 3,
  SWITCH_IR_CUTTER = 4,
};

typedef struct {
  int left;
  int right;
  int top;
  int bottom;
} af_roi_info_t;

typedef struct __attribute__((__packed__)) {
  uint8_t id;
  char data[MAX_BUFFER_SIZE];
} af_msg_t;

static int fd_iav = -1;

static void process_connection(int connfd);
static int get_af_value(char *af_values, int size);

static uint8_t irCutterSwitch = '1';
static int gpio_fd = -1;
#define IR_CUTTER_GPIO_PATH "/sys/class/gpio/gpio35/value"
static int switch_ir_cutter(void);

int main(int argc, char **argv)
{
  int sockfd, connfd;
  int op = 1;
  struct sockaddr_in servr_addr, client_addr;
  socklen_t length = sizeof(client_addr);

  signal(SIGPIPE, SIG_IGN);

  if ((fd_iav = open("/dev/iav", O_RDWR, 0)) < 0) {
    perror("open /dev/iav");
    return -1;
  }

  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    perror("socket");
    return -1;
  }

  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &op, sizeof(op)) < 0) {
    perror("setsockopt");
    return -1;
  }

  bzero(&servr_addr, sizeof(servr_addr));
  servr_addr.sin_family = AF_INET;
  servr_addr.sin_port = htons(SERVER_PORT);
  servr_addr.sin_addr.s_addr = htonl(INADDR_ANY);

  if (bind(sockfd, (struct sockaddr*) &servr_addr, sizeof(servr_addr)) < 0) {
    perror("bind");
    return -1;
  }

  if (listen(sockfd, 1) < 0) {
    perror("listen");
    return -1;
  }

  while (1) {
    printf(PREFIX "WAITING FOR CONNECTION\n");
    if ((connfd = accept(sockfd, (struct sockaddr*) &client_addr, &length)) < 0) {
      perror("accept");
      continue;
    }
    printf(PREFIX "CONNECTTED\n");
    process_connection(connfd);
    close(connfd);
    printf(PREFIX "DISCONNECTTED\n");
  }

  return 0;
}

static void process_connection(int connfd)
{
  int ret;
  int str_size;
  struct timeval delay;
  fd_set set;
  af_msg_t msg;

  memset(&delay, 0, sizeof(delay));
  delay.tv_sec  = 5;
  delay.tv_usec = 0; //1ms

  while (1) {
    memset(&msg, 0, sizeof(msg));
    //read user commands
    FD_ZERO(&set);
    FD_SET(connfd, &set);
    if ((ret = select(connfd+1, &set, NULL, NULL, &delay)) <= 0) {
      if (ret < 0) {
        perror("select");
        continue;
      }
      if (ret == 0) {
        printf(PREFIX "CONNECTION TIMEOUT, RESET FOR NEW ONE\n");
        return;
      }
    }

    if (FD_ISSET(connfd, &set)) {
      if ((ret = recv(connfd, &msg.id, sizeof(msg.id), 0)) <= 0) {
        if (ret < 0) {
          perror("recv");
        }
        //ret == 0, connection closed
        return;
      } else {
        switch (msg.id) {
        case STOP_SERVER:
          printf(PREFIX "STOP SERVER\n");
          break;
        case START_SERVER:
          printf(PREFIX "START SERVER\n");
          break;
        case HEARTBEAT:
          printf(PREFIX "-->HEARTBEAT\n");
          if (send(connfd, &msg.id, sizeof(msg.id), 0) < 0) {
            if (errno == EPIPE || errno == ECONNRESET) {
              printf(PREFIX "CONNECTION CLOSED BY PEER");
            } else {
              perror("send");
            }
            return;
          }
          break;
        case AF_DATA:
          printf(PREFIX "GET AF DATA\n");
          memset(&msg.data, 0, sizeof(msg.data));
          if ((str_size = get_af_value(msg.data, sizeof(msg.data))) >= 0) {
              if (send(connfd, &msg, sizeof(msg), 0) < 0) {
                if (errno == EPIPE || errno == ECONNRESET) {
                  printf(PREFIX "CONNECTION CLOSED BY PEER\n");
                } else {
                  perror("send");
                }
                return;
            }
          } else {
            fprintf(stderr, "failed to get af statistics data\n");
          }
          break;
        case SWITCH_IR_CUTTER:
          printf(PREFIX "SWITCH IR CUTTER\n");
          switch_ir_cutter();
          break;
        default:
          printf(PREFIX "??? UNKNOWN MSG\n");
        }
      }
    }

    usleep(200 * 1000);
  }
}

static int get_af_value(char *buf, int size)
{
  static int first_run = 1;
  static af_roi_info_t af_rois[SCENE_ROI_WIDTH][SCENE_ROI_HEIGHT];
  uint32_t af_value[SCENE_ROI_WIDTH][SCENE_ROI_HEIGHT];

  if (!buf) {
    fprintf(stderr, "Invalid input at line %d\n", __LINE__);
    return -1;
  }

  struct img_statistics stats;
  struct cfa_aaa_stat cfa_aaa;
  struct rgb_aaa_stat rgb_aaa;
  struct rgb_histogram_stat hist_aaa;
  u8 cfa_valid, rgb_valid, hist_valid;
  u16 af_tile_col, af_tile_row;

  memset(&stats, 0, sizeof(stats));
  memset(&cfa_aaa, 0, sizeof(cfa_aaa));
  memset(&rgb_aaa, 0, sizeof(rgb_aaa));
  memset(&hist_aaa, 0, sizeof(hist_aaa));

  cfa_valid = rgb_valid = hist_valid = 0;
  stats.cfa_data_valid = &cfa_valid;
  stats.rgb_data_valid = &rgb_valid;
  stats.hist_data_valid = &hist_valid;
  stats.cfa_statis = &cfa_aaa;
  stats.rgb_statis = &rgb_aaa;
  stats.hist_statis = &hist_aaa;

  if (ioctl(fd_iav, IAV_IOC_IMG_GET_STATISTICS, &stats) < 0) {
    perror("IAV_IOC_IMG_GET_STATISTICS");
    return -1;
  }

  af_tile_row = cfa_aaa.aaa_tile_info.af_tile_num_row;
  af_tile_col = cfa_aaa.aaa_tile_info.af_tile_num_col;

  if (first_run) {
    printf("total af tiles: %d X %d\n", af_tile_row, af_tile_col);

    int edge_height, edge_width, center_height, center_width;
    edge_height = center_height = af_tile_row / SCENE_ROI_HEIGHT;
    edge_width  = center_width  = af_tile_col / SCENE_ROI_WIDTH;
    if ((af_tile_row % SCENE_ROI_HEIGHT) == 1) {
      center_height += 1;
    }
    if ((af_tile_col % SCENE_ROI_WIDTH) == 1) {
      center_width += 1;
    }
    if ((af_tile_row % SCENE_ROI_HEIGHT) == 2) {
      edge_height += 1;
    }
    if ((af_tile_col % SCENE_ROI_WIDTH) == 2) {
      edge_width += 1;
    }

    for (int i = 0; i < SCENE_ROI_WIDTH; i++) {
      for (int j = 0; j < SCENE_ROI_HEIGHT; j++) {
        if (i == 1) {
          af_rois[i][j].top    = edge_height;
          af_rois[i][j].bottom = edge_height + center_height - 1;
        } else if (i == 0) {
          af_rois[i][j].top    = 0;
          af_rois[i][j].bottom = edge_height - 1;
        } else{
          af_rois[i][j].top    = edge_height + center_height;
          af_rois[i][j].bottom = af_tile_row - 1;
        }

        if (j == 1) {
          af_rois[i][j].left   = edge_width;
          af_rois[i][j].right  = edge_width + center_width - 1;
        } else if (j == 0) {
          af_rois[i][j].left   = 0;
          af_rois[i][j].right  = edge_width - 1;
        } else {
          af_rois[i][j].left   = edge_width + center_width;
          af_rois[i][j].right  = af_tile_col - 1;
        }
        printf("TILE[%d][%d]: left %d, right %d, top %d, bottom %d\n",
               i, j, af_rois[i][j].left, af_rois[i][j].right,
               af_rois[i][j].top, af_rois[i][j].bottom);
      }
    }

    first_run = 0;
  }

  memset(af_value, 0, sizeof(af_value));
  for (int i = 0; i < SCENE_ROI_WIDTH; i++) {
    for (int j = 0; j < SCENE_ROI_HEIGHT; j++) {
      for (int l = af_rois[i][j].left; l <= af_rois[i][j].right; l++) {
        for (int m = af_rois[i][j].top; m <= af_rois[i][j].bottom; m++) {
          int tile_index = m * af_tile_col + l;
          af_value[i][j] += cfa_aaa.af_stat[tile_index].sum_fv2;
        }
      }
      af_value[i][j] /= ((af_rois[i][j].right - af_rois[i][j].left + 1) *
                         (af_rois[i][j].bottom - af_rois[i][j].top + 1));

      // af_value[i][j] & 0B1111 0000 0000 0000 (0xF000)
      if (af_value[i][j] & 0xF000) {
        //af_value will overflow after left shifting 4 bits, so set to 0xffff
        af_value[i][j] = 0xFFFF;
      } else {
        af_value[i][j] <<= 4;
      }
    }
  }

  snprintf(buf, size, "%hu %hu %hu %hu %hu %hu %hu %hu %hu",
           (uint16_t) af_value[0][0],
           (uint16_t) af_value[0][1],
           (uint16_t) af_value[0][2],
           (uint16_t) af_value[1][0],
           (uint16_t) af_value[1][1],
           (uint16_t) af_value[1][2],
           (uint16_t) af_value[2][0],
           (uint16_t) af_value[2][1],
           (uint16_t) af_value[2][2]);

  return (strlen(buf) + 1);
}

static int switch_ir_cutter(void)
{
  if (gpio_fd < 0) {
    if ((gpio_fd = open(IR_CUTTER_GPIO_PATH, O_RDWR)) < 0) {
      perror("open");
      return -1;
    }
  }

  if (irCutterSwitch == '1') {
    irCutterSwitch = '0';
  } else {
    irCutterSwitch = '1';
  }
  if (write(gpio_fd, &irCutterSwitch, sizeof(irCutterSwitch)) < 0) {
    printf("write %u failed\n", irCutterSwitch);
    perror("write");
    return -1;
  }

  return 0;
}
