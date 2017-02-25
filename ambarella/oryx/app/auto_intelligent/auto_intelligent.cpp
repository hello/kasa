/*******************************************************************************
 * auto_int32_telligent.cpp
 *
 * History:
 *   Nov 11, 2015 - [Huaiqing Wang] created file
 *
 * Copyright (c) 2016 Ambarella, Inc.
 *
 * This file and its contents ("Software") are protected by int32_tellectual
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
#include <getopt.h>
#include <sys/stat.h>
#include <mutex>

#include "am_log.h"
#include "common.h"
#include "air_api_handle.h"
#include "frame_buffer.h"

static bool g_show_help = false;
static int32_t g_vout_osd_color = AMFrameBuffer::FB_COLOR_GREEN;
static char g_file_path[FILE_NAME_NUM_MAX];
static uint32_t g_frames_num = 0;
static int32_t g_overlay_area_id = -1;
static bool g_run = true;
static bool main_loop_exited = false;
static AMFrameBufferPtr g_draw = nullptr;

#define NO_ARG    0
#define HAS_ARG   1

struct option long_options[] = { { "help", NO_ARG, 0, 'h' },
                                 { "file_path", HAS_ARG, 0, 'p' },
                                 { "frames", HAS_ARG, 0, 'f' },
                                 { "color", HAS_ARG, 0, 'c' },
                                 { 0, 0, 0, 0 }, };

const char *short_options = "h:p:f:c:";

struct hint_s
{
    const char *arg;
    const char *str;
};

static const struct hint_s hint[] =
{
 { "", "Show usage\n" },
 { "", "save y frame to file in this path\n" },
 { "", "save how many frames if file_path is specified\n" },
 { "",
   "vout osd color,0:transparent;1:semi-transparent,2:red;3:green;4:blue;5:white;6:black\n" },
};

static void usage(void)
{
  printf("This program is used to test vehicle intelligent algorithm, options are:\n");
  for (uint32_t i = 0; i < sizeof(long_options) / sizeof(long_options[0]) - 1;
      i ++) {
    if (isalpha(long_options[i].val)) {
      printf("-%c ", long_options[i].val);
    } else {
      printf("  ");
    }
    printf("--%s", long_options[i].name);
    if (hint[i].arg[0] != 0) {
      printf(" [%s]", hint[i].arg);
    }
    printf("\t%s\n", hint[i].str);
  }
}

static int32_t parse_parameters(int argc, char **argv)
{
  int32_t ret = 0;
  int32_t c;
  while (1) {
    c = getopt_long(argc, argv, short_options, long_options, nullptr);
    if (c < 0) {
      break;
    }

    switch (c) {
      case 'h':
        g_show_help = true;
        break;

      case 'p':
        snprintf(g_file_path, FILE_NAME_NUM_MAX, "%s", optarg);
        break;

      case 'f':
        g_frames_num = atoi(optarg);
        break;

      case 'c':
        g_vout_osd_color = atoi(optarg);
        break;

      default:
        printf("Unknown parameter %c!\n", c);
        ret = -1;
        break;
    }
  }

  return ret;
}

//alg init interface
static int32_t intelligent_alg_init_interface(buffer_crop_desc *rect)
{
  int32_t ret = 0;

  //vehicle intelligent algo make initialization here
  //initialization process...

  //use algo ouptput fill source buffer input crop parameter
  rect->offset.x = 100;
  rect->offset.y = 100;
  rect->size.width = 720;
  rect->size.height = 480;

  return ret;
}

static int32_t stream_overlay_init()
{
  int32_t ret = 0;
  //firstly, init a large area, size<(2M-16k)/16,(2M is the default size
  //alloc for overlay, if you change it before, it may different)
  point_desc offset = { 100, 100 };
  size_desc size = { 1100, 100 };
  g_overlay_area_id = init_bmp_overlay_area(offset, size);
  if (g_overlay_area_id < 0) {
    ERROR("init a bmp overlay area failed!!!");
    return -1;
  } else {
    INFO("the created area id = %d\n", g_overlay_area_id);
  }

  //second, add some bmp to the area init by above action
  bmp_overlay_manipulate_desc bmp = { 0 };
  bmp.area_id = g_overlay_area_id;
  bmp.color_key = 0x80800000;
  bmp.color_range = 20;
  //add bmp file 1
  bmp.offset.x = 0;
  bmp.offset.y = 0;
  snprintf(bmp.bmp_file,
  FILE_NAME_NUM_MAX,
           "/usr/share/oryx/video/overlay/file0.bmp");
  add_bmp_to_area(bmp);
  //add bmp file 2
  bmp.offset.x = 220;
  bmp.offset.y = 10;
  snprintf(bmp.bmp_file,
  FILE_NAME_NUM_MAX,
           "/usr/share/oryx/video/overlay/file1.bmp");
  add_bmp_to_area(bmp);
  //add bmp file 3
  bmp.offset.x = 400;
  bmp.offset.y = 10;
  snprintf(bmp.bmp_file,
  FILE_NAME_NUM_MAX,
           "/usr/share/oryx/video/overlay/file2.bmp");
  add_bmp_to_area(bmp);
  //add bmp file 4
  bmp.offset.x = 600;
  bmp.offset.y = 10;
  snprintf(bmp.bmp_file,
  FILE_NAME_NUM_MAX,
           "/usr/share/oryx/video/overlay/file3.bmp");
  add_bmp_to_area(bmp);
  //add bmp file 5
  bmp.offset.x = 800;
  bmp.offset.y = 10;
  snprintf(bmp.bmp_file,
  FILE_NAME_NUM_MAX,
           "/usr/share/oryx/video/overlay/file4.bmp");
  add_bmp_to_area(bmp);
  //add bmp file 6
  bmp.offset.x = 950;
  bmp.offset.y = 10;
  snprintf(bmp.bmp_file,
  FILE_NAME_NUM_MAX,
           "/usr/share/oryx/video/overlay/file5.bmp");
  add_bmp_to_area(bmp);

  //now, render the area to display all above bmps
  if (render_bmp_overlay_area(g_overlay_area_id) < 0) {
    ERROR("render_bmp_overlay_area failed!!!");
    ret = -1;
  }

  return ret;
}

static int32_t set_vout_osd(vout_osd_desc *osd, uint32_t buf_w, uint32_t buf_h)
{
  int32_t ret = 0;
  do {
    if (!osd) {
      ret = -1;
      break;
    }

    if (0 == osd->osd_type) {
      if (g_draw->draw_dot(osd->offset, osd->n, osd->color) != AM_RESULT_OK) {
        ret = -1;
        break;
      }
    } else if (1 == osd->osd_type) {
      if (osd->n < 2) {
        ret = -1;
        ERROR("invalid parametre!");
        break;
      }
      point_desc &p0 = osd->offset[0];
      point_desc &p1 = osd->offset[1];
      if (g_draw->draw_line(p0, p1, osd->color) != AM_RESULT_OK) {
        ret = -1;
        break;
      }
    } else if (2 == osd->osd_type) {
      if (osd->n < 2) {
        ret = -1;
        ERROR("invalid parametre!");
        break;
      }
      if (g_draw->draw_polyline(osd->offset, osd->n, osd->color)
          != AM_RESULT_OK) {
        ret = -1;
        break;
      }
    } else if (3 == osd->osd_type) {
      if (osd->size.width > buf_w) {
        osd->size.width = buf_w;
      }
      if (osd->size.height > buf_h) {
        osd->size.height = buf_h;
      }
      for (uint32_t i = 0; i < osd->n; ++ i) {
        if (osd->offset[i].x + osd->size.width > buf_w) {
          osd->offset[i].x = buf_w - osd->size.width;
        }
        if (osd->offset[i].y + osd->size.height > buf_h) {
          osd->offset[i].y = buf_h - osd->size.height;
        }
      }

      if (g_draw->draw_straight_line(osd) != AM_RESULT_OK) {
        ret = -1;
        break;
      }
    } else if (4 == osd->osd_type) {
      if (osd->size.width > buf_w) {
        osd->size.width = buf_w;
      }
      if (osd->size.height > buf_h) {
        osd->size.height = buf_h;
      }
      for (uint32_t i = 0; i < osd->n; ++ i) {
        if (osd->offset[i].x + osd->size.width > buf_w) {
          osd->offset[i].x = buf_w - osd->size.width;
        }
        if (osd->offset[i].y + osd->size.height > buf_h) {
          osd->offset[i].y = buf_h - osd->size.height;
        }
      }
      if (g_draw->draw_curve(osd) != AM_RESULT_OK) {
        ret = -1;
        break;
      }
    } else if (5 == osd->osd_type) {
      if (osd->n < 2) {
        ret = -1;
        ERROR("invalid parametre!");
        break;
      }
      point_desc &p0 = osd->offset[0];
      point_desc &p1 = osd->offset[1];
      if (g_draw->draw_rectangle(p0, p1, osd->color) != AM_RESULT_OK) {
        ret = -1;
        break;
      }
    } else if (6 == osd->osd_type) {
      if (g_draw->draw_clear() != AM_RESULT_OK) {
        ret = -1;
        break;
      }
    }
    if (ret != 0) {
      break;
    }

  } while (0);

  return ret;
}

//alg handle interface function
int32_t intelligent_alg_handle_interface(const gray_frame_desc *data,
                                         vout_osd_desc *osd,
                                         overlay_manipulate_desc *overlay)
{
  int32_t ret = 0;

  //process data, runing vehicle intelligent algo here
  //algo process...

  //use algo output fill vout osd parameter
  osd->n = 5;
  osd->offset = new point_desc[osd->n];
  for (uint32_t i = 0; i < osd->n; ++ i) {
    osd->offset[i].x += 50 * (i + 1);
    osd->offset[i].y += 30 * (i + 1);
  }
  osd->size.width = 500;
  osd->size.height = 400;
  osd->color = g_vout_osd_color;
  osd->k = -0.001;
  osd->b = 500;

  //use algo output fill encode stream overlay parameter
  overlay->manipulate_flag = 0;
  overlay->layout.layout_id = 1;
  snprintf(overlay->bmp_file,
  FILE_NAME_NUM_MAX,
           "/usr/share/oryx/video/overlay/Ambarella-256x128-8bit.bmp");

  return ret;
}

int32_t intelligent_alg_handle_interface(const gray_frame_desc *data,
                                         vout_osd_desc *osd,
                                         bmp_overlay_manipulate_desc *bmp,
                                         bool *overlay_update_flag)
{
  int32_t ret = 0;
  static uint32_t overlay_count, osd_count;
  static int32_t file_idx = 1;

  //process data, runing vehicle intelligent algo here
  //algo process...

  //use algo output fill vout osd parameter
  osd->n = 5;
  osd->offset = new point_desc[osd->n];
  osd->offset[0].x = 50;
  osd->offset[0].y = 30;
  osd->offset[1].x = 500;
  osd->offset[1].y = 30;
  osd->offset[2].x = 500;
  osd->offset[2].y = 100;
  osd->offset[3].x = 200;
  osd->offset[3].y = 250;
  osd->offset[4].x = 600;
  osd->offset[4].y = 450;
  osd->size.width = 500;
  osd->size.height = 400;
  osd->color = g_vout_osd_color;
  if (0 == (osd_count % 99)) {
    osd->osd_type = 0;
  } else if (0 == (osd_count % 199)) {
    osd->osd_type = 1;
  } else if (0 == (osd_count % 299)) {
    osd->osd_type = 2;
  } else if (0 == (osd_count % 399)) {
    osd->k = 1;
    osd->b = 50;
    osd->osd_type = 3;
  } else if (0 == (osd_count % 499)) {
    osd->k = -0.01;
    osd->b = 500;
    osd->osd_type = 4;
  } else if (0 == (osd_count % 599)) {
    osd->osd_type = 5;
    osd->offset[0].x = 50;
    osd->offset[0].y = 50;
    osd->offset[1].x = 300;
    osd->offset[1].y = 300;
  } else if (0 == (osd_count % 699)) {
    osd->osd_type = 6; //clear osd
    osd_count = 0;
  }
  ++ osd_count;

  *overlay_update_flag = false;
  //use algo output fill encode stream overlay parameter
  if (0 == (overlay_count % 100)) {
    bmp->area_id = g_overlay_area_id;
    bmp->color_key = 0x80800000;
    bmp->color_range = 20;
    bmp->offset.x = 950;
    bmp->offset.y = 10;
    snprintf(bmp->bmp_file,
    FILE_NAME_NUM_MAX,
             "/usr/share/oryx/video/overlay/file%d.bmp", file_idx);
    *overlay_update_flag = true;
    if ((++ file_idx) > 8) {
      file_idx = 1;
    }
    printf("change overlay bmp!!!\n");
  }
  ++ overlay_count;

  return ret;
}

static int32_t intelligent_alg_main_loop()
{
  int32_t ret = 0;
  int32_t file_fd = -1;
  while (g_run) {
    gray_frame_desc data;
    if ((ret = get_gray_frame(QUERY_SOURCE_BUFFER_ID, &data)) != 0) {
      ERROR("get frame data failed!!!");
      break;
    }

    vout_osd_desc osd;
//    overlay_manipulate_desc overlay;
//    if ((ret=intelligent_alg_handle_interface(&data, &osd, &overlay)) != 0) {
    bool overlay_update_flag = false;
    bmp_overlay_manipulate_desc bmp; //use bmp type overlay api
    if ((ret = intelligent_alg_handle_interface(&data,
                                                &osd,
                                                &bmp,
                                                &overlay_update_flag)) != 0) {
      ERROR("call intelligent_alg_handle_interface error!!!");
      break;
    }

    if ((ret = set_vout_osd(&osd, data.width, data.height)) != 0) {
      delete[] osd.offset;
      ERROR("set vout osd error!!!");
      break;
    }
    delete[] osd.offset;

    if (overlay_update_flag) {
      printf("update overlay!!!\n");
      update_bmp_overlay_area(bmp);
    }
//    if (3 == overlay.manipulate_flag) {
//      if ((ret=destroy_stream_overlay()) != 0) {
//        ERROR("destroy overlay error!!!");
//        break;
//      }
//    } else if (2 == overlay.manipulate_flag) {
//      if ((ret=delete_stream_overlay(overlay.area_id))!= 0) {
//        ERROR("delete overlay error!!!");
//        break;
//      }
//    } else if (1 == overlay.manipulate_flag) {
//      if ((ret=add_stream_overlay(&overlay.layout,
//                                  overlay.bmp_file)) != 0) {
//        ERROR("set_vout_osd error!!!");
//        break;
//      }
//    }

    if (strlen(g_file_path)) {
      char file_fullname[512];
      if (1 == g_frames_num) {
        g_file_path[0] = '\0';
      }
      if (0 == g_frames_num) {
        sprintf(file_fullname, "%s%d_%dx%d.y", "/mnt/buf",
        QUERY_SOURCE_BUFFER_ID,
                data.width, data.height);
      } else {
        sprintf(file_fullname, "%s%d_%dx%d_%d.y", "/mnt/buf",
        QUERY_SOURCE_BUFFER_ID,
                data.width, data.height, g_frames_num --);
      }
      //create the file only when needed
      if (file_fd < 0) {
        file_fd = open(file_fullname, O_CREAT | O_RDWR, 0644);
        if (file_fd < 0) {
          ERROR("open file failed\n");
          ret = -4;
          break;
        }
      }
      printf("cap timestamp: %lld, seq num %d \n", data.mono_pts, data.seq_num);

      for (uint32_t i = 0; i < data.height; i ++) {
        if (write(file_fd, data.addr + i * data.pitch, data.width)
            != (int32_t) data.width) {
          ERROR("write y file error \n");
          ret = -5;
          break;
        }
      }
      if (g_frames_num > 1 && file_fd > 0) {
        close(file_fd);
        file_fd = -1;
      }
    }
  };
  if (file_fd > 0) {
    close(file_fd);
    file_fd = -1;
  }
  main_loop_exited = true;

  return ret;
}

static void sigstop(int32_t signo)
{
  INFO("catch signal: %d\n", signo);
  g_run = false;
  g_draw = nullptr;
}

int32_t main(int32_t argc, char *argv[])
{
  int32_t ret = 0;
  do {
    signal(SIGINT, sigstop);
    signal(SIGQUIT, sigstop);
    signal(SIGTERM, sigstop);

    ret = parse_parameters(argc, argv);

    if ((ret != 0) || (g_show_help == true)) {
      usage();
      break;
    }

    if ((ret = api_helper_init()) != 0) {
      ERROR("unable to get AMAPIHelper instance\n");
      ret = -1;
      break;
    }

    buffer_crop_desc rect;
    if ((ret = intelligent_alg_init_interface(&rect)) != 0) {
      ERROR("call intelligent_alg_init_interface failed!!!");
      break;
    }

    if ((ret = set_input_size_dynamic(QUERY_SOURCE_BUFFER_ID, &rect)) != 0) {
      ERROR("set_source_buffer_size failed!!!");
      break;
    }

    if (!(g_draw = AMFrameBuffer::get_instance())) {
      ERROR("get draw instance failed!!!");
      ret = -1;
      break;
    }

    if ((ret = stream_overlay_init()) != 0) {
      ERROR("stream_overlay_init failed!!!");
      break;
    }

    if ((ret = intelligent_alg_main_loop()) != 0) {
      ERROR("intelligent_alg_handle failed!!!");
      break;
    }

  } while (0);

  return ret;
}
