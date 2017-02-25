/*
 * cali_fisheye_center.c
 *
 * Histroy:
 *   Dec 5, 2013 - [Shupeng Ren] created file
 *
 * Copyright (C) 2015 Ambarella, Inc.
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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <ctype.h>
#include <fcntl.h>
#include <math.h>

#include "dlist.h"

#define NO_ARG    0
#define HAS_ARG   1

static char yuv_filename[128];
static int edge_num = 0;
static int W = 1920, H = 1080;
static float A, B, R;
static float a1 = 0.0, b1 = 0.0;
static float a2 = 0.0, b2 = 0.0;
static float a3 = 0.0, b3 = 0.0;
static float c11 = 0.0, c12 = 0.0, c21 = 0.0;
static float var_aver = 0.0;
static double var_sum = 0.0;

static struct list_info point_list;
static unsigned char *buffer = NULL;

static struct option long_options[] = {
  {"filename", HAS_ARG, 0, 'f'},
  {"width", HAS_ARG, 0, 'w'},
  {"height", HAS_ARG, 0, 'h'},
  {0, 0, 0, 0}
};

struct hint_s {
    const char *arg;
    const char *str;
};

static const char *short_options = "f:w:h:";

static const struct hint_s hint[] = {
  {"", "\t\tYUV file name"},
  {"", "\t\tYUV width, default: 1920"},
  {"", "\t\tYUV height, default: 1080"}
};

static void usage(int argc, char **argv) {
  int i;

  printf("\n%s usage:\n", argv[0]);
  for (i = 0; i < sizeof(long_options) / sizeof(long_options[0]) - 1; i++) {
    if (isalpha(long_options[i].val))
      printf("-%c ", long_options[i].val);
    else
      printf("   ");
    printf("--%s", long_options[i].name);
    if (hint[i].arg[0] != 0)
      printf(" [%s]", hint[i].arg);
    printf("\t%s\n", hint[i].str);
  }
}

static int init_param(int argc, char **argv)
{
  int ch;
  int option_index = 0;
  opterr = 0;

  if (argc < 2) {
    usage(argc, argv);
    return -1;
  }

  while ((ch = getopt_long(argc, argv,
                           short_options,
                           long_options,
                           &option_index)) != -1) {
    switch (ch) {
      case 'f':
        snprintf(yuv_filename, sizeof(yuv_filename), "%s", optarg);
        break;
      case 'w':
        W = atoi(optarg);
        break;
      case 'h':
        H = atoi(optarg);
        break;
      default:
        printf("Unknown option %d.\n", ch);
        return -1;
        break;
    }
  }
  if ((W <= 0) || (H <= 0)) {
    printf("YUV width [%d] and height [%d] must be greater than 0.\n",
            W, H);
    return -1;
  }
  return 0;
}

static int read_y_data(char *filename, unsigned char *buffer,
                       int width, int height)
{
  int ret = 0;
  if (filename == NULL || buffer == NULL || width == 0 || height == 0) {
    printf("Please check the parameters!\n");
    return -1;
  }
  int fd = -1, nbytes = width * height;

  if ((fd = open(filename, O_RDONLY)) < 0) {
    perror("open");
    return -1;
  }

  do {
    if(read(fd, buffer, nbytes) != nbytes) {
      perror("read");
      ret = -1;
      break;
    }
  } while(0);
  close(fd);

  return ret;
}

static void process_edge_callback(struct list_info *info,
                                  struct node_info *node)
{
  float x = node->node.x / 2000;
  float y = node->node.y / 2000;

  a1 += x;
  b1 += y;

  a2 += x * x;
  b2 += y * y;

  a3 += x * x * x;
  b3 += y * y * y;

  c11 += x * y;
  c12 += x * y * y;
  c21 += x * x * y;
  ++edge_num;
}

static void remove_points_callback(struct list_info *info,
                                   struct node_info *node)
{
  float tmp1 = node->node.x - A;
  float tmp2 = node->node.y - B;
  float tmp3 = sqrt(tmp1*tmp1 + tmp2*tmp2) - R;
  float tmp4 = abs(tmp3);
  if(tmp4 > R/4) {
    info->del(info, node);
  }
}

static void check_error_callback(struct list_info *info,
                            struct node_info *node)
{
  float tmp1 = node->node.x - A;
  float tmp2 = node->node.y - B;
  float tmp3 = sqrt(tmp1*tmp1 + tmp2*tmp2) - R;

  var_sum += tmp3 * tmp3;
}

static void fitting_circle()
{
  float a, b, c;
  float C, D, E, G, H, N;

  N = edge_num;

  C = N*a2 - a1*a1;
  D = N*c11 - a1*b1;
  E = N*a3 + N*c12 - (a2+b2)*a1;
  G = N*b2 - b1*b1;
  H = N*c21 + N*b3 - (a2+b2)*b1;

  a = (H*D-E*G)/(C*G-D*D);
  b = (H*C-E*D)/(D*D-G*C);
  c = -(a*a1 + b*b1 + a2 + b2)/N;

  A = a/(-2)*2000;
  B = b/(-2)*2000;
  R = sqrt(a*a + b*b - 4*c)/2*2000;
}

static int process_y_data(unsigned char *buffer, int width, int height)
{
  int i, j;
  int addi, addj, num = 0;
  int nbytes = width * height;
  unsigned int count[256] = {0};
  unsigned char threshold = 0;

  for (i = 0; i < nbytes; ++i) {
    ++count[buffer[i]];
  }

  int N0 = 0, N1 = 0;
  float w0, w1;
  float u0 = 0.0, u1 = 0.0;
  float g, max_g = 0.0;

  for (i = 10; i < 250; ++i) {
    for (j = 0; j < 256; ++j) {
      if (j < i) {
        N0 += count[j];
      } else {
        N1 += count[j];
      }
    }
    w0 = (float)N0 / nbytes;
    w1 = (float)N1 / nbytes;

    for (j = 0; j < 256; ++j) {
      if (j < i) {
        u0 += (float)j * count[j] / N0;
      } else {
        u1 += (float)j * count[j] / N1;
      }
    }

    g = w0 * w1 * (u0 - u1) * (u0 -u1);
    if (g > max_g) {
      max_g = g;
      threshold = i;
    }

    N0 = 0;
    N1 = 0;
    u0 = 0.0;
    u1 = 0.0;
  }

  int max1 = 0, max2 = 0;
  int maxi1 = 0, maxi2 = 0;
  int min = nbytes;

  for (i = 0; i < 256; ++i) {
    if (i < threshold) {
      if (count[i] > max1) {
        max1 = count[i];
        maxi1 = i;
      }
    } else {
      if (count[i] > max2) {
        max2 = count[i];
        maxi2 = i;
      }
    }
  }

  for (i = maxi1; i < maxi2; ++i) {
    if (count[i] < min) {
      min = count[i];
      threshold = i;
    }
  }

  for (i = 0; i < nbytes; ++i) {
    if (buffer[i] < threshold) {
      buffer[i] = 0x00;
    } else {
      buffer[i] = 0xff;
    }
  }

//==============================================================================

  for (i = 1; i < height-1; ++i) {
    for (j = 1; j < width-1; ++j) {
      for (addi = -1; addi <= 1; ++addi) {
        for (addj = -1; addj <= 1; ++addj) {
          if (addi == 0 && addj == 0) {
            continue;
          }
          if (buffer[(i+addi)*width+(j+addj)] == buffer[i*width+j])  {
            ++num;
          }
        }
      }
      if (num < 4) {
        buffer[i*width+j] = ~buffer[i*width+j];
      }
      num = 0;
    }
  }

  for (i = 1; i < height-1; ++i) {
    for (j = 1; j < width-1; ++j) {
      for (addi = -1; addi <= 1; ++addi) {
        for (addj = -1; addj <= 1; ++addj) {
          if (addi == 0 && addj == 0) {
            continue;
          }
          if (buffer[(i+addi)*width+(j+addj)] == buffer[i*width+j])  {
            ++num;
          }
        }
      }
      if (num < 6 && num > 0) {
        list_node tmp;
        tmp.x = j;
        tmp.y = i;
        point_list.add_tail(&point_list, &tmp);
        ++edge_num;
      }
      num = 0;
    }
  }

  edge_num = 0;
  a1 = b1 = a2 = b2 = a3 = b3 = c11 = c12 = c21 = 0.0;
  point_list.for_each(&point_list, process_edge_callback);
  fitting_circle();

  point_list.for_each_safe(&point_list, remove_points_callback);

  edge_num = 0;
  a1 = b1 = a2 = b2 = a3 = b3 = c11 = c12 = c21 = 0.0;
  point_list.for_each(&point_list, process_edge_callback);
  fitting_circle();

  printf("\033[32m\033[1m"
      "============================================\n"
      "Center is (%.1f, %.1f), Radius: %.1f\n"
      "============================================"
      "\033[0m""\n", A, B, R);

  var_sum = 0;
  point_list.for_each(&point_list, check_error_callback);
  var_aver = var_sum / edge_num;

  if (var_aver > 20) {
    printf("\033[31m\033[1m"
        "============================================\n"
        "                 WARNING!\n"
        "       Failed to find fisheye center\n"
        "    The result of center is unreliable\n"
        "============================================"
        "\033[0m""\n");
  }

  return 0;
}

int main(int argc, char **argv)
{
  if (init_param(argc, argv) < 0) {
    return -1;
  }

  list_init(&point_list);
  if ((buffer = (unsigned char*)malloc(W * H)) == NULL) {
    printf("malloc buffer failed!\n");
    return -1;
  }

  do {
    if (read_y_data(yuv_filename, buffer, W, H) < 0) {
      break;
    }
    if (process_y_data(buffer, W, H) < 0) {
      break;
    }
  } while(0);
  free(buffer);
  point_list.destroy(&point_list);

  return 0;
}
