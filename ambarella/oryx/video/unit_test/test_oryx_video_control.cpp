/**
 * test_oryx_video.cpp
 *
 *  History:
 *    Jul 31, 2015 - [Shupeng Ren] created file
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

#include <signal.h>
#include <vector>
#include <stdio.h>
#include <map>
#include "am_base_include.h"
#include "am_log.h"
#include "am_video_camera_if.h"
#include "am_encode_warp_if.h"
#include "am_encode_overlay_if.h"
#include "am_dptz_if.h"

static void show_main_menu(bool prompt)
{
  if (!prompt) {
    printf("> ");
    return;
  }

  PRINTF("\n");
  PRINTF("================================================");
  PRINTF("  r -- Run Camera");
  PRINTF("  s -- Stop Camera");
  PRINTF("  i -- Goto idle");
  PRINTF("  p -- Enter preview");
  PRINTF("  o -- Test overlay");
  PRINTF("  d -- Test DPTZ");
  PRINTF("  w -- Test Warp");
  PRINTF("  S -- Test Stream");
  PRINTF("  q -- Quit Camera");
  PRINTF("================================================\n\n");
  printf("> ");
}

void print_dptz_param_menu()
{
  PRINTF("\n================================================\n");
  PRINTF("  b -- set dptz buffer id (default 0)\n");
  PRINTF("  s -- set dptz ratio pan(-1.0~ 1.0), tilt(-1.0~ 1.0), zoom(1.0~ 8.0) format: p,t,z\n");
  PRINTF("  S -- set dptz size x, y, w, h\n");
  PRINTF("  g -- get dptz ratio and size\n");
  PRINTF("  x -- go back to main menu");
  PRINTF("\n================================================\n\n");
  PRINTF("> ");
}

void print_warp_param_menu()
{
  PRINTF("\n================================================\n");
  PRINTF("  r -- specify the region number");
  PRINTF("  s -- set warp ldc strength: (0.0 ~ 20.0)");
  PRINTF("  g -- get warp param");
  PRINTF("  x -- go back to main menu");
  PRINTF("\n================================================\n\n");
  PRINTF("> ");
}

void print_stream_param_menu()
{
  PRINTF("\n================================================\n");
  PRINTF("  e -- start encoding stream (0 ~ 3)");
  PRINTF("  s -- stop encoding stream (0 ~ 3)");
  PRINTF("  x -- go back to main menu");
  PRINTF("\n================================================\n\n");
  PRINTF("> ");
}

void show_stream_menu(AMIVideoCameraPtr camera)
{
  int quit_flag = 0;
  int error_opt = 0;
  int stream_id = -1;
  print_stream_param_menu();
  do {
    char ch = getchar();
    while (ch) {
      quit_flag = 0;
      switch(ch) {
        case 'e': {
          PRINTF("Input stream id: (0 ~ 3)");
          scanf("%d", &stream_id);
          if ((stream_id < 0) || (stream_id > 3)) {
            ERROR("Invalid stream id");
            quit_flag = 1;
          } else {
            if (AM_RESULT_OK != camera->start_stream(AM_STREAM_ID(stream_id))) {
              ERROR("Failed to start stream %d", stream_id);
            }
          }
        }break;
        case 's': {
          PRINTF("Input stream id: (0 ~ 3)");
          scanf("%d", &stream_id);
          if ((stream_id < 0) || (stream_id > 3)) {
            ERROR("Invalid stream id");
            quit_flag = 1;
          } else {
            if (AM_RESULT_OK != camera->stop_stream(AM_STREAM_ID(stream_id))) {
              ERROR("Failed to stop stream %d", stream_id);
            }
          }
        }break;
        case 'x': {
          quit_flag = 1;
        }break;
        default: {
          error_opt = 1;
        }break;
      }
      if (quit_flag) {
        break;
      }
      if (error_opt) {
        print_stream_param_menu();
      }
      ch = getchar();
    }
  }while(0);
}

void show_dptz_menu(AMIVideoCameraPtr camera)
{
  int quit_flag = 0;
  int error_opt = 0;
  int buf_id = 0;
  int x,y,w,h;
  float zoom_ratio = 1.0f;
  float pan_ratio = 0.0f;
  float tilt_ratio = 0.0f;
  AMDPTZRatio ratio;
  AMDPTZSize rect;

  print_dptz_param_menu();
  do {
    AMIDPTZ *dptz = (AMIDPTZ *)camera->get_video_plugin(VIDEO_PLUGIN_DPTZ);
    if (!dptz) {
      ERROR("Video Plugin \"%s\" is not loaded!", VIDEO_PLUGIN_DPTZ);
      break;
    }
    char ch = getchar();
    while (ch) {
      quit_flag = 0;
      error_opt = 0;
      switch (ch) {
        case 'b':
          PRINTF("choose a buffer id: (0~3)\n");
          scanf("%d", &buf_id);
          if ((buf_id < 0) || (buf_id > 3)) {
            ERROR("buf id Error \n");
            quit_flag = 1;
          } else {
            PRINTF("buf id %d chosen\n", buf_id);
          }
          break;
        case 's':
          PRINTF("input dptz ratio pan(-1.0~1.0),tilt(-1.0~1.0),zoom(0.1~8.0): p,t,z\n");
          scanf("%f,%f,%f", &pan_ratio, &tilt_ratio, &zoom_ratio);
          if ((pan_ratio < -1.0) || (pan_ratio > 1.0)
             || (tilt_ratio < -1.0) || (tilt_ratio > 1.0)
             || (zoom_ratio < 1.0) || (zoom_ratio > 8.0)) {
            ERROR("pan_ratio/tilt_ratio/zoom_ratio Error \n");
            quit_flag = 1;
            break;
          }
          PRINTF("chosen %f,%f,%f\n", pan_ratio, tilt_ratio, zoom_ratio);

          ratio.pan.first = true;
          ratio.pan.second = pan_ratio;
          ratio.tilt.first = true;
          ratio.tilt.second = tilt_ratio;
          ratio.zoom.first = true;
          ratio.zoom.second = zoom_ratio;

          if (dptz->set_ratio(AM_SOURCE_BUFFER_ID(buf_id), ratio) == false) {
            ERROR("Set DPTZ Error \n");
            quit_flag = 1;
            break;
          }
          break;
        case 'S':
          PRINTF("input dptz size : x,y,w,h\n");
          scanf("%d,%d,%d,%d", &x, &y, &w, &h);
          if ((x < 0) || (y < 0) || (w < 0) || (h < 0)) {
            ERROR("x/y/w/h Error \n");
            quit_flag = 1;
            break;
          }
          PRINTF("chosen %f,%f,%f\n", pan_ratio, tilt_ratio, zoom_ratio);

          rect.x.first = true;
          rect.x.second = x;
          rect.y.first = true;
          rect.y.second = y;
          rect.w.first = true;
          rect.w.second = w;
          rect.h.first = true;
          rect.h.second = h;

          if (dptz->set_size(AM_SOURCE_BUFFER_ID(buf_id), rect) == false) {
            ERROR("Set DPTZ Error \n");
            quit_flag = 1;
            break;
          }
          break;
        case 'g':
          if (dptz->get_ratio(AM_SOURCE_BUFFER_ID(buf_id), ratio) == false) {
            ERROR("Get DPTZ Error \n");
            quit_flag = 1;
            break;
          }
          PRINTF("pan_ratio = %f, tilt_ratio = %f, zoom_ratio = %f\n",
                 ratio.pan.second,
                 ratio.tilt.second,
                 ratio.zoom.second);
          break;
        case 'x':
          quit_flag = 1;
          break;
        default:
          error_opt = 1;
          break;
      }

      if (quit_flag)
        break;
      if (error_opt)
        print_dptz_param_menu();
      ch = getchar();
    }
  } while (0);
}

void show_warp_menu(AMIVideoCameraPtr camera)
{
  int quit_flag = 0;
  int error_opt = 0;
  int region_id = -1;
  float ldc_strength = 1.0f;
  AM_RESULT result = AM_RESULT_OK;
  AMIEncodeWarp *warp =
      (AMIEncodeWarp*)camera->get_video_plugin(VIDEO_PLUGIN_WARP);
  print_warp_param_menu();

  if (warp) {
    do {
      char ch = getchar();
      while (ch) {
        quit_flag = 0;
        error_opt = 0;
        switch (ch) {
          case 'r': {
            PRINTF("input region number");
            scanf("%d", &region_id);
            if (region_id < 0) {
              ERROR("iput region ID error!");
              quit_flag = 1;
              break;
            }
            PRINTF("specify region number %d", region_id);
          }break;
          case 's': {
            float orig_ldc_strength = 1.0f;
            PRINTF("input ldc_strength(0.0~20.0)");
            scanf("%f", &ldc_strength);
            if ((ldc_strength < 0.0) || (ldc_strength > 20.0)) {
              ERROR("ldc_strength Error \n");
              quit_flag = 1;
              break;
            }
            PRINTF("chosen %f\n", ldc_strength);
            if ((result = warp->get_ldc_strength(region_id, orig_ldc_strength))
                != AM_RESULT_OK) {
              ERROR("Get original WARP LDC strength error!");
              quit_flag = 1;
              break;
            }
            if ((result = warp->set_ldc_strength(region_id, ldc_strength)) !=
                AM_RESULT_OK) {
              ERROR("Set WARP LDC strength error!");
              quit_flag = 1;
              break;
            }
            if ((result = warp->apply()) != AM_RESULT_OK) {
              ERROR("Failed to apply WARP parameters! Reset to original value!");
              if ((result = warp->set_ldc_strength(region_id, orig_ldc_strength))
                  != AM_RESULT_OK) {
                ERROR("Set WARP LDC strength error!");
              }
              quit_flag = 1;
              break;
            }
          } break;
          case 'g': {
            float pano_hfov_degree = 1.0f;
            if ((result = warp->get_ldc_strength(region_id, ldc_strength)) !=
                AM_RESULT_OK) {
              ERROR("Failed to get WARP LDC strength!");
              quit_flag = 1;
              break;
            }
            if ((result = warp->get_pano_hfov_degree(region_id,
                pano_hfov_degree)) != AM_RESULT_OK) {
              ERROR("Failed to get WARP pano hfov degree!");
              quit_flag = 1;
              break;
            }
            PRINTF("ldc_strength = %f, pano_hfov_degree = %f",
                   ldc_strength, pano_hfov_degree);
          }break;
          case 'x':
            quit_flag = 1;
            break;
          default:
            error_opt = 1;
            break;
        }
        if (quit_flag)
          break;
        if (error_opt)
          print_warp_param_menu();
        ch = getchar();
      }

    } while (0);
  } else {
    WARN("Video Plugin %s is not loaded!", VIDEO_PLUGIN_WARP_SO);
  }
}
static void signal_handler(int sig)
{

}

using std::map;
using std::pair;
using std::make_pair;
typedef map<pair<AM_STREAM_ID, pair<int32_t, int32_t>>,
    AM_OVERLAY_DATA_TYPE> OverlayDataTypeMap;
static OverlayDataTypeMap g_data_type;
static void show_overlay_main_menu()
{
  PRINTF("Please choose a test option:");
  PRINTF("\n================================================\n");
  PRINTF("  R -- delete all overlay areas\n");
  PRINTF("  D -- delete a overlay area\n");
  PRINTF("  a -- add a overlay area\n");
  PRINTF("  p -- disable a overlay area\n");
  PRINTF("  e -- enable a overlay area\n");
  PRINTF("  n -- add a overlay area data block\n");
  PRINTF("  d -- delete a overlay area data block\n");
  PRINTF("  u -- update a overlay area data block\n");
  PRINTF("  s -- save overlay parameter to config\n");
  PRINTF("  x -- go back to main menu");
  PRINTF("\n================================================\n\n");
  PRINTF(">");
}

/*mode=0,delete; mode=1,disable; mode=2,enable;*/
static void overlay_area_manipulate(AMIVideoCameraPtr &camera, int mode)
{
  int32_t stream_id = 0;
  int32_t area_id = 0;
  AMIEncodeOverlay *ol = (AMIEncodeOverlay*)camera->\
          get_video_plugin(VIDEO_PLUGIN_OVERLAY);

  PRINTF("Choose a overlay stream to %s: \n",
         (mode==0) ? "delete" : ((mode==1) ? "disable" : "enable"));
  scanf("%d", &stream_id);

  PRINTF("Choose overlay area id to %s: \n",
         (mode==0) ? "delete" : ((mode==1) ? "disable" : "enable"));
  scanf("%d", &area_id);

  AM_OVERLAY_STATE state = AM_OVERLAY_DISABLE;
  if (0 == mode) {
    state = AM_OVERLAY_DELETE;
  } else if (1 == mode) {
    state = AM_OVERLAY_DISABLE;
  } else if (2 == mode) {
    state = AM_OVERLAY_ENABLE;
  }
  if (!ol) {
    NOTICE("Video Plugin \"%s\" is not loaded!", VIDEO_PLUGIN_OVERLAY);
  } else if (AM_RESULT_OK ==
             ol->change_state(AM_STREAM_ID(stream_id), area_id, state)) {
    PRINTF("Change Overlay state OK!");
  }
}

void add_data_to_overlay_area(AMIVideoCameraPtr &camera,
                             int32_t stream_id, int32_t area_id)
{
  int32_t op = -1;
  int32_t tmp = 0;
  static int32_t count = 1;
  AMOverlayAreaData data;
  AMOffset &offset = data.rect.offset;
  AMResolution &size = data.rect.size;
  AMOverlayPicture &pic = data.pic;
  AMOverlayTextBox &text = data.text;
  AMIEncodeOverlay *ol = (AMIEncodeOverlay*)camera->\
          get_video_plugin(VIDEO_PLUGIN_OVERLAY);

  PRINTF("Choose a data type: 0(picture); 1(string);\n");
  scanf("%d", &op);
  PRINTF("data block startX in area: ");
  scanf("%d", &(offset.x));
  PRINTF("data block startY in area: ");
  scanf("%d", &(offset.y));
  if (0 == op) {
    data.type = AM_OVERLAY_DATA_TYPE_PICTURE;
    char file[128];
    pic.filename = "/usr/share/oryx/video/overlay/";
    snprintf(file, 128, "file%d.bmp", count ++);
    pic.filename.append(file);

    if (count > 8) {
      count = 1;
    }
  } else {
    PRINTF("data block width : ");
    scanf("%d", &(size.width));
    PRINTF("data block height : ");
    scanf("%d", &(size.height));
    PRINTF("Please choose a font size :");
    scanf("%d",&(text.font.width));
    PRINTF("Please choose char spacing :");
    scanf("%d",&(text.spacing));
    PRINTF("Please choose a font color(0:white 1:black 2:red \n"
        "3:blue 4:green 5:yellow 6:cyan 7:magenta 8:custom)\n");
    scanf("%d",&tmp);
    text.font_color.id = tmp;
    if (tmp > 7) {
      text.font_color.color = {0,0,0,128};
    }
    data.type = AM_OVERLAY_DATA_TYPE_STRING;
    PRINTF("Please input a string to insert:");
    char str[128];
    while((fgets(str, 128, stdin) == nullptr) ||
        (str[0] == '\n') || (str[0] == EOF)) {
      PRINTF("Wrong input, again:");
    }
    str[strlen(str)-1] = '\0';
    text.str = str;
    PRINTF("string = %s\n", text.str.c_str());
  }

  int32_t index = ol ?
      ol->add_data_to_area(AM_STREAM_ID(stream_id), area_id, data) : -1;

  if (index >= 0) {
    PRINTF("data index: %d\n",index);
    g_data_type[make_pair(AM_STREAM_ID(stream_id),
                          make_pair(area_id, index))] = data.type;
  } else {
    PRINTF("Add data to overlay area failed!");
  }
}

/*mode=0,delete; mode=1,update; mode=2,add*/
static void overlay_area_data_manipulate(AMIVideoCameraPtr &camera, int mode)
{
  int32_t stream_id = 0, area_id = 0, index = 0;
  static int32_t count = 1;
  AMIEncodeOverlay *ol = (AMIEncodeOverlay*)camera->\
              get_video_plugin(VIDEO_PLUGIN_OVERLAY);

  PRINTF("Choose a overlay stream to %s: \n",
         (mode==0) ? "delete" : ((mode==1) ? "update" : "add"));
  scanf("%d", &stream_id);

  PRINTF("Choose overlay area id to %s: \n",
         (mode==0) ? "delete" : ((mode==1) ? "update" : "add"));
  scanf("%d", &area_id);

  if (mode == 0 || mode == 1) {
    PRINTF("Choose overlay area data index to %s: \n",
           (mode==0) ? "delete" : "update");
    scanf("%d", &index);
    OverlayDataTypeMap::iterator it = g_data_type.find(
        make_pair(AM_STREAM_ID(stream_id), make_pair(area_id, index)));
    if (it == g_data_type.end()) {
      PRINTF("data index:%d in stream%d area%d is not existed",
             index, stream_id, area_id);
      return;
    }
  }

  if (0 == mode) { //delete
    if (ol) {
      if (ol->delete_area_data(AM_STREAM_ID(stream_id),
                               area_id, index) == AM_RESULT_OK) {
        PRINTF("delete overlay area data ok");
        g_data_type.erase(
            g_data_type.find(make_pair(
                AM_STREAM_ID(stream_id), make_pair(area_id, index))));
      }
    } else {
      NOTICE("Video Plugin \"%s\" is not loaded!", VIDEO_PLUGIN_OVERLAY);
    }
  } else if (1 == mode) { //update
    AMOverlayAreaData data;

    AM_OVERLAY_DATA_TYPE type = g_data_type[make_pair(
            AM_STREAM_ID(stream_id), make_pair(area_id, index))];
    if (type == AM_OVERLAY_DATA_TYPE_PICTURE) {
      AMOverlayPicture &pic = data.pic;
      data.type = AM_OVERLAY_DATA_TYPE_PICTURE;
      char file[128];
      pic.filename = "/usr/share/oryx/video/overlay/";
      snprintf(file, 128, "file%d.bmp", count ++);
      pic.filename.append(file);
      if (count > 8) {
        count = 1;
      }
    } else if (type == AM_OVERLAY_DATA_TYPE_STRING) {
      AMOverlayTextBox &text = data.text;
      data.type = AM_OVERLAY_DATA_TYPE_STRING;
      text.font_color.id = 4;
      text.font.width = 25;
      text.spacing = 0;
      PRINTF("Please input a string to update:");
      char str[128];
      while((fgets(str, 128, stdin) == nullptr) ||
          (str[0] == '\n') || (str[0] == EOF)) {
        PRINTF("Wrong input, again:");
      }
      str[strlen(str)-1] = '\0';
      text.str = str;
    } else {
      PRINTF("Wrong type");
      return;
    }

    if (ol) {
      if (ol->update_area_data(AM_STREAM_ID_0, area_id,
                               index, data) == AM_RESULT_OK) {
        PRINTF("change overlay state ok");
      }
    } else {
      NOTICE("Video Plugin \"%s\" is not loaded!", VIDEO_PLUGIN_OVERLAY);
    }
  } else { //add
    add_data_to_overlay_area(camera, stream_id, area_id);
  }
}

void add_overlay_area(AMIVideoCameraPtr &camera)
{
  int32_t stream_id = 0, area_id = -1;
  AMOverlayAreaAttr attr;
  AMIEncodeOverlay *ol = (AMIEncodeOverlay*)camera->\
      get_video_plugin(VIDEO_PLUGIN_OVERLAY);

  PRINTF("Choose a overlay stream to add: \n");
  scanf("%d", &stream_id);

  PRINTF("area startX : ");
  scanf("%d", &(attr.rect.offset.x));
  PRINTF("area startY : ");
  scanf("%d", &(attr.rect.offset.y));
  PRINTF("area width : ");
  scanf("%d", &(attr.rect.size.width));
  PRINTF("area height : ");
  scanf("%d", &(attr.rect.size.height));
  int32_t tmp = 0;
  PRINTF("Please choose a area background color(0:black "
      "1:white with semi-transparent 2:full-transparent)\n");
  scanf("%d",&tmp);
  if (0 == tmp) {
    attr.bg_color = {128,128,16,255};
  } else if (1 == tmp) {
    attr.bg_color = {128,128,255,128};
  } else {
    attr.bg_color = {0,0,0,0};
  }

  area_id = ol ? ol->init_area(AM_STREAM_ID(stream_id), attr) : -1;
  if (area_id < 0) {
    if (ol) {
      PRINTF("init overlay area failed!!!");
    } else {
      NOTICE("Video Plugin \"%s\" is not loaded!", VIDEO_PLUGIN_OVERLAY);
    }
    return;
  }
  PRINTF("area id = %d\n", area_id);

  int32_t add = 0;
  do {
    add_data_to_overlay_area(camera, stream_id, area_id);
    PRINTF("Continue add a data to area(1) or break(0)?");
    scanf("%d", &add);
  } while(add);
}

static void osd_overlay_demo(AMIVideoCameraPtr &camera)
{
  AM_RESULT result;
  char op;
  bool quit_flag = false;
  AMIEncodeOverlay *ol = (AMIEncodeOverlay*)camera->\
      get_video_plugin(VIDEO_PLUGIN_OVERLAY);
  do{
    show_overlay_main_menu();
    op = getchar();
    if (op == '\n' || op == EOF)
      op = getchar();
    switch (op) {
      case 'R':
        if (ol) {
          PRINTF("delete overlay!!!");
          if ((result=ol->destroy_overlay()) == AM_RESULT_OK) {
            PRINTF("destory overlay ok!!");
          }
        } else {
          NOTICE("Video Plugin \"%s\" is not loaded!", VIDEO_PLUGIN_OVERLAY);
        }
        break;
      case 'D':
        PRINTF("test overlay remove！\n");
        overlay_area_manipulate(camera, 0);
        break;
      case 'p':
        PRINTF("test overlay disable！\n");
        overlay_area_manipulate(camera, 1);
        break;
      case 'e':
        PRINTF("test overlay enable！\n");
        overlay_area_manipulate(camera, 2);
        break;
      case 'a':
        PRINTF("test add overlay area！ \n");
        add_overlay_area(camera);
        break;
      case 'd':
        PRINTF("test delete a overlay area data！ \n");
        overlay_area_data_manipulate(camera, 0);
        break;
      case 'u':
        PRINTF("test update a overlay area data！ \n");
        overlay_area_data_manipulate(camera, 1);
        break;
      case 'n':
        PRINTF("test add a overlay area data！ \n");
        overlay_area_data_manipulate(camera, 2);
        break;
      case 's':
        if (ol) {
          ol->save_param_to_config();
        } else {
          NOTICE("Video Plugin \"%s\" is not loaded!", VIDEO_PLUGIN_OVERLAY);
        }
        break;
      case 'x':
        quit_flag = 1;
        break;
      default:
        break;
    }
    if (1 == quit_flag)
      break;
  } while(1);

}

int main(int argc, char **argv)
{
  signal(SIGINT, signal_handler);
  signal(SIGQUIT, signal_handler);
  signal(SIGTERM, signal_handler);

  int ret = 0;
  char ch = 0;
  bool prompt = true;
  bool run_flag = true;
  bool show_menu = true;
  AM_RESULT result = AM_RESULT_OK;
  AMIVideoCameraPtr camera = nullptr;

  do {
    if (!(camera = AMIVideoCamera::get_instance())) {
      ERROR("Failed to create AMVideoCamera!");
      break;
    }

    while (run_flag) {
      if (show_menu) {
        show_main_menu(prompt);
        prompt = true;
      } else {
        show_menu = true;
      }

      switch (ch = getchar()) {
        case 'r':
          if ((result = camera->start()) != AM_RESULT_OK) {
            ERROR("Failed to start encoding!");
          }
          break;
        case 's':
          if ((result = camera->stop()) != AM_RESULT_OK) {
            ERROR("Failed to stop encoding!");
          }
          break;
        case 'i':
          if ((result = camera->idle()) != AM_RESULT_OK) {
            ERROR("Failed to go to idle!");
          }
          break;
        case 'p':
          if ((result = camera->preview()) != AM_RESULT_OK) {
            ERROR("Failed to enter preview!");
          }
          break;
        case 'o':
          osd_overlay_demo(camera);
          break;

        case 'd':
          PRINTF("Show DPTZ menu\n");
          show_dptz_menu(camera);
          break;

        case 'w':
          PRINTF("Show Warp menu\n");
          show_warp_menu(camera);
          break;
        case 'S':
          PRINTF("Show Stream menu\n");
          show_stream_menu(camera);
          break;
        case 'q':
          run_flag = false;
          break;
        case '\n':
          prompt = false;
          break;
        default:
          show_menu = false;
          break;
      }
      if (result != AM_RESULT_OK) {
        ret = -1;
        break;
      }
      if (run_flag && prompt) {
        getchar();
      }
    }
    camera->stop();
  } while (0);
  return ret;
}
