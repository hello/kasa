/*******************************************************************************
 * am__video_camera_test.cpp
 *
 * History:
 *   2014-8-8 - [lysun] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co,Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#include <stdio.h>
#include <iostream>
#include <string>
#include <unistd.h>
#include <signal.h>
#include "am_log.h"
#include "am_encode_device_if.h"
#include "am_dptz_warp_if.h"
#include "am_video_camera_if.h"
#include "am_osd_overlay_if.h"

void show_menu()
{
  PRINTF("\n================================================\n");
  PRINTF("  m -- choose Camera encode mode \n");
  PRINTF("  r -- Run Camera  \n");
  PRINTF("  s -- Stop Camera \n");
  PRINTF("  p -- Parameter Change\n");
  PRINTF("  l -- load all Params from config file \n");
  PRINTF("  u -- Update with loaded params \n");
  PRINTF("  w -- write current Params to config file \n");
  PRINTF("  d -- Test DPTZ Warp \n");
  PRINTF("  o -- Test overlay \n");
  PRINTF("  q -- Quit Camera");
  PRINTF("\n================================================\n\n");
  PRINTF("> ");
}

void print_dptz_warp_praram_menu()
{
  PRINTF("\n================================================\n");
  PRINTF("  b -- set zoom buffer id (default 0)\n");
  PRINTF("  s -- set dptz ratio(1.0~ 8.0) and ratio-center(-1.0~1.0) format: x,y\n");
  PRINTF("  g -- get dptz ratio \n");
  PRINTF("  p -- easy zoom-in to 1:1 pixel\n");
  PRINTF("  f -- easy zoom-out to full FOV\n");
  PRINTF("  w -- set dptz by input window, format: x,y,w,h \n");
  PRINTF("  l -- Correct Lens Distortion \n");
  PRINTF("  L -- Smart LDC Demo");
  PRINTF("  x -- go back to main menu");
  PRINTF("\n================================================\n\n");
  PRINTF("> ");
}

void print_param_menu()
{

  PRINTF("\n================================================\n");
  PRINTF("  a -- choose stream id\n");
  PRINTF("  b -- start encoding on selected stream \n");
  PRINTF("  c -- stop encoding on selected stream  \n");
  PRINTF("  d -- get info of selected stream \n");
  PRINTF("  e -- change bitrate of selected stream \n");
  PRINTF("  f -- change fps of selected stream \n");
  PRINTF("  g -- choose buffer id \n");
  PRINTF("  h -- get info of selected buffer \n");
  PRINTF("  x -- go back to main menu");
  PRINTF("\n================================================\n\n");
  PRINTF("> ");

}

void show_dptz_warp_menu(AMIVideoCamera * camera)
{
  int quit_flag = 0;
  int error_opt = 0;
  int buf_id = 0;
  float zoom_ratio = 1.0f;
  float ratio_center_x = 0;
  float ratio_center_y = 0;
  AM_RESULT result = AM_RESULT_OK;
  AMIDPTZWarp *dptz_warp = camera->get_encode_device()->get_dptz_warp();
  AMDPTZParam dptz_param;
  AMDPTZRatio dptz_ratio;
  AMRect  input_rect;
  char ch;
  AMWarpParam ldc_warp;
  float strength = 0;
  memset(&ldc_warp, 0, sizeof(ldc_warp));
  print_dptz_warp_praram_menu();

  ch = getchar();
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

      case 'w':
        PRINTF("choose dptz input window: x,y,w,h \n");
        scanf("%d,%d,%d,%d",
              &input_rect.x,
              &input_rect.y,
              &input_rect.width,
              &input_rect.height);
        if ((input_rect.x < 0) || (input_rect.y < 0) || (input_rect.width <= 64)
            || (input_rect.height <= 64)) {
          ERROR("input rect error \n");
          quit_flag = 1;
          break;
        }
        PRINTF("chosen input window x=%d,y=%d,w=%d,h=%d\n",
               input_rect.x,
               input_rect.y,
               input_rect.width,
               input_rect.height);

        //apply DZ
        result =
            dptz_warp->set_dptz_input_window(AM_ENCODE_SOURCE_BUFFER_ID(buf_id),
                                             &input_rect);
        if (result != AM_RESULT_OK) {
          ERROR("Set DPTZ Error \n");
          quit_flag = 1;
          break;
        }
        //set needs apply
        result = dptz_warp->apply_dptz_warp();
        if (result != AM_RESULT_OK) {
          ERROR("Apply DPTZ Error \n");
          quit_flag = 1;
          break;
        }
        break;
      case 's':
        PRINTF("choose a DZoom ratio (1.0~ 8.0)\n");
        scanf("%f", &zoom_ratio);
        if ((zoom_ratio < 0.25) || (zoom_ratio > 8.0)) {
          ERROR("zoom_ratio Error \n");
          quit_flag = 1;
          break;
        }
        PRINTF("DZoom ratio %f chosen\n\n", zoom_ratio);

        PRINTF("input dptz ratio-center-x and y (-1.0~1.0): x,y\n");
        scanf("%f,%f", &ratio_center_x, &ratio_center_y);
        if ((ratio_center_x < -1.0) || (ratio_center_x > 1.0)||
            (ratio_center_y < -1.0) || (ratio_center_y > 1.0)){
          ERROR("ratio_center_x or y Error \n");
          quit_flag = 1;
          break;
        }
        PRINTF("chosen %f,%f\n", ratio_center_x, ratio_center_y);
        memset(&dptz_ratio, 0, sizeof(dptz_ratio));
        dptz_ratio.zoom_center_pos_x = ratio_center_x;
        dptz_ratio.zoom_center_pos_y = ratio_center_y;
        dptz_ratio.zoom_factor_x = zoom_ratio;
        dptz_ratio.zoom_factor_y = zoom_ratio;
        result = dptz_warp->set_dptz_ratio(AM_ENCODE_SOURCE_BUFFER_ID(buf_id),
                                           &dptz_ratio);
        if (result != AM_RESULT_OK) {
          ERROR("Set DPTZ Error \n");
          quit_flag = 1;
          break;
        }
        //set needs apply
        result = dptz_warp->apply_dptz_warp();
        if (result != AM_RESULT_OK) {
          ERROR("Apply DPTZ Error \n");
          quit_flag = 1;
          break;
        }

        break;
      case 'g':
        result = dptz_warp->get_dptz_param(AM_ENCODE_SOURCE_BUFFER_ID(buf_id),
                                           &dptz_param);
        if (result != AM_RESULT_OK) {
          ERROR("Get DPTZ Error \n");
          quit_flag = 1;
          break;
        }
        PRINTF("DZoom ratio is x = %f, y = %f \n",
               dptz_param.dptz_ratio.zoom_factor_x,
               dptz_param.dptz_ratio.zoom_factor_y);
        PRINTF("DZoom Center is (%f, %f)\n",
               dptz_param.dptz_ratio.zoom_center_pos_x,
               dptz_param.dptz_ratio.zoom_center_pos_y);
        break;

      case 'p':
        PRINTF("Zoom to 1:1 pixel for buf %d\n", buf_id);
        result =
            dptz_warp->set_dptz_easy(AM_ENCODE_SOURCE_BUFFER_ID(buf_id),
                                           AM_EASY_ZOOM_MODE_PIXEL_TO_PIXEL);
        if (result != AM_RESULT_OK) {
          ERROR("Set Easy Zoom In to 1:1 Error \n");
          quit_flag = 1;
          break;
        }

        //set needs apply
        result = dptz_warp->apply_dptz_warp();
        if (result != AM_RESULT_OK) {
          ERROR("Apply DPTZ Error \n");
          quit_flag = 1;
          break;
        }
        break;
      case 'f':
        PRINTF("Zoom to Full FOV for buf %d\n", buf_id);
        result =
            dptz_warp->set_dptz_easy(AM_ENCODE_SOURCE_BUFFER_ID(buf_id),
                                           AM_EASY_ZOOM_MODE_FULL_FOV);
        if (result != AM_RESULT_OK) {
          ERROR("Set Easy Zoom Out to Full FOV Error \n");
          quit_flag = 1;
          break;
        }
        //set needs apply
        result = dptz_warp->apply_dptz_warp();
        if (result != AM_RESULT_OK) {
          ERROR("Apply DPTZ Error \n");
          quit_flag = 1;
          break;
        }
        break;
      case 'l':
        PRINTF("Input LDC strength: (0.0 ~ 20.0)\n");
        scanf("%f", &strength);
        if (strength < 0 || strength > 20) {
          ERROR("Strength Error \n");
          quit_flag = 1;
          break;
        }
        ldc_warp.lens_param.max_hfov_degree = strength * 10;
        ldc_warp.lens_param.efl_mm = 2.1;
        ldc_warp.proj_mode = AM_WARP_PROJRECTION_EQUIDISTANT;
        ldc_warp.warp_mode = AM_WARP_MODE_RECTLINEAR;
        result = dptz_warp->set_warp_param(&ldc_warp);
        if (result != AM_RESULT_OK) {
          ERROR("set_warp_param Fail\n");
          quit_flag = 1;
          break;
        }
        result = dptz_warp->apply_dptz_warp();
        if (result != AM_RESULT_OK) {
          ERROR("apply_dptz_warp Fail\n");
          quit_flag = 1;
          break;
        }
        break;
      case 'L':
        PRINTF("Smart LDC Demo\n");
        PRINTF("Input Correction Mode (1-Normal, 2-Pano) :\n");
        scanf("%d", (int *)&ldc_warp.warp_mode);
        if (ldc_warp.warp_mode == AM_WARP_MODE_PANORAMA) {
          PRINTF("Input Pano H-FOV :\n");
          scanf("%f", &ldc_warp.lens_param.pano_hfov_degree);
        }

        PRINTF("Input LDC strength: (0.0 ~ 20.0)\n");
        scanf("%f", &strength);
        if (strength < 0 || strength > 20) {
          ERROR("Strength Error \n");
          quit_flag = 1;
          break;
        }

        PRINTF("Input Zoom Factor: (0.25 ~ 3)\n");
        scanf("%f", &zoom_ratio);
        if ((zoom_ratio < 0.25) || (zoom_ratio > 4)) {
          ERROR("zoom_ratio Error \n");
          quit_flag = 1;
          break;
        }
        PRINTF("Done\n");

        ldc_warp.lens_param.max_hfov_degree = strength * 10;
        ldc_warp.proj_mode = AM_WARP_PROJRECTION_EQUIDISTANT;
        result = dptz_warp->set_warp_param(&ldc_warp);
        if (result != AM_RESULT_OK) {
          ERROR("set_warp_param Fail\n");
          quit_flag = 1;
          break;
        }

        memset(&dptz_ratio, 0, sizeof(dptz_ratio));
        dptz_ratio.zoom_center_pos_x = 0;
        dptz_ratio.zoom_center_pos_y = 0;
        dptz_ratio.zoom_factor_x = zoom_ratio;
        dptz_ratio.zoom_factor_y = zoom_ratio;
        result = dptz_warp->set_dptz_ratio(AM_ENCODE_SOURCE_BUFFER_MAIN,
                                           &dptz_ratio);
        if (result != AM_RESULT_OK) {
          ERROR("Set DPTZ Error \n");
          quit_flag = 1;
          break;
        }

        result = dptz_warp->apply_dptz_warp();
        if (result != AM_RESULT_OK) {
          ERROR("apply_dptz_warp Fail\n");
          quit_flag = 1;
          break;
        }
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
      print_dptz_warp_praram_menu();

    ch = getchar();
  }
}

void show_param_menu(AMIVideoCamera * camera)
{
  int quit_flag = 0;
  int error_opt = 0;
  int stream_id = 0;
  int buf_id = 0;
  uint32_t bitrate_target = 0;
  uint32_t fps_target = 0;
  AM_RESULT result = AM_RESULT_OK;
  AMIEncodeDevice *encode_device = camera->get_encode_device();

  char ch;
  print_param_menu();

  ch = getchar();
  while (ch) {
    quit_flag = 0;
    error_opt = 0;
    switch (ch) {
      case 'a':
        PRINTF("Choose a stream id to work with: (0~3)\n");
        scanf("%d", &stream_id);
        PRINTF("stream id %d chosen\n", stream_id);
        break;
      case 'b':
        result = encode_device->start_encode_stream(AM_STREAM_ID(stream_id));
        if (result != AM_RESULT_OK) {
          ERROR("Start Encoding stream id %d failed\n ", stream_id);
        } else {
          PRINTF("Start Encoding stream id %d OK.\n ", stream_id);
        }
        break;
      case 'c':
        result = encode_device->stop_encode_stream(AM_STREAM_ID(stream_id));
        if (result != AM_RESULT_OK) {
          ERROR("Stop Encoding stream id %d failed\n ", stream_id);
        } else {
          PRINTF("Stop Encoding stream id %d OK.\n ", stream_id);
        }
        break;
      case 'd':
        PRINTF("Show info for stream id %d \n ", stream_id);
        result = encode_device->show_stream_info(AM_STREAM_ID(stream_id));
        break;
      case 'e':
        PRINTF("input new bitrate target for stream id %d OK.\n ", stream_id);
        scanf("%d", &bitrate_target);
        if (bitrate_target <= 100000) {
            ERROR("bitrate target too low %d \n", bitrate_target);
        }else {
            result = encode_device->set_stream_bitrate(AM_STREAM_ID(stream_id),
                                                       bitrate_target);
        }

        break;
      case 'f':
        PRINTF("input new fps target for stream id %d OK \n ", stream_id);
        scanf("%d", &fps_target);
        //usually, stream fps means "frame factor" of VIN
        if (fps_target <= 5) {
            ERROR("fps_target too low %d \n", fps_target);
        }else {
            result = encode_device->set_stream_fps(AM_STREAM_ID(stream_id),
                                                   fps_target);
        }
        break;
      case 'g':
        PRINTF("choose buffer id to work with: (0~3)\n");
        scanf("%d", &buf_id);
        PRINTF("buffer id %d chosen\n", buf_id);
        break;
      case 'h':
        PRINTF("Show info for buf id %d \n ", buf_id);
        result = encode_device->show_source_buffer_info(
            AM_ENCODE_SOURCE_BUFFER_ID(buf_id));

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
      print_param_menu();

    ch = getchar();

  }

}

void show_overlay_main_menu()
{
  PRINTF("Please choose a test option:");
  PRINTF("\n================================================\n");
  PRINTF("  d -- destroy osd all osd area \n");
  PRINTF("  a -- add osd area \n");
  PRINTF("  r -- remove osd area \n");
  PRINTF("  p -- pause(disable) osd overlay \n");
  PRINTF("  e -- enable osd overlay \n");
  PRINTF("  i -- show overlay information \n");
  PRINTF("  x -- go back to main menu");
  PRINTF("\n================================================\n\n");
  PRINTF(">");
}

void show_overlay_area_option()
{
  PRINTF("\n================================================\n");
  PRINTF("  b -- test bmp overlay \n");
  PRINTF("  p -- test pattern overlay \n");
  PRINTF("  s -- test string overlay \n");
  PRINTF("  t -- test time string overlay \n");
  PRINTF("\n================================================\n\n");
  PRINTF(">");
}

void osd_param_choose(AMOSDInfo *overlay_info, int area_id)
{
  char op;
  uint32_t tmp;
  AMOSDAttribute *attr = &(overlay_info->attribute[area_id]);
  AMOSDArea *area = &(overlay_info->overlay_area[area_id]);
  AMOSDFont *font = &(attr->osd_text_box.font);
  do{
    PRINTF("Please choose area: %d test option:",area_id);
    show_overlay_area_option();
    op = getchar();
    if (op == '\n' || op == EOF)
      op = getchar();

    PRINTF("Enable or disable area osd rotate function(1:enable,0:diable):");
    scanf("%d",(int *)&(attr->enable_rotate));
    switch (op) {
      case 'b':
        attr->type = AM_OSD_TYPE_PICTURE;
        //specify white is the full transparent color
        attr->osd_pic.colorkey[0].color.v = 128;
        attr->osd_pic.colorkey[0].color.u = 128;
        attr->osd_pic.colorkey[0].color.y = 235;
        attr->osd_pic.colorkey[0].color.a = 0;
        attr->osd_pic.colorkey[0].range = 20;
//        attr->osd_pic.colorkey[1].color.v = 110;
//        attr->osd_pic.colorkey[1].color.u = 240;
//        attr->osd_pic.colorkey[1].color.y = 41;
//        attr->osd_pic.colorkey[1].color.a = 0;
//        attr->osd_pic.colorkey[1].range = 20;
        break;
      case 'p':
        attr->type = AM_OSD_TYPE_TEST_PATTERN;
        break;
      case 's':
      {
        attr->type = AM_OSD_TYPE_GENERIC_STRING;
        PRINTF("Please choose a font size :");
        scanf("%d",&(font->size));
        PRINTF("Please choose a font color(0:white 1:black 2:red \n"
              "3:blue 4:green 5:yellow 6:cyan 7:magenta 8:custom)\n");
        scanf("%d",&tmp);
        attr->osd_text_box.font_color.id = tmp;
        if (tmp > 7) {
          memset(&attr->osd_text_box.font_color.color, 0, sizeof(AMOSDCLUT));
          attr->osd_text_box.font_color.color.a = 255;
        }
        break;
      }
      case 't':
        attr->type = AM_OSD_TYPE_TIME_STRING;
        PRINTF("Please choose a font size :");
        scanf("%d",&(font->size));
        break;
      default:
        PRINTF("Wrong input! \n");
        continue;
        break;
    }
    PRINTF("choose osd layout: \n"
        "0:AUTO_LEFT_TOP; \n"
        "1:AUTO_RIGHT_TOP; \n"
        "2:AUTO_LEFT_BOTTOM; \n"
        "3:AUTO_RIGHT_BOTTOM; \n"
        "4:MANUAL \n");
    scanf("%d", (int *)&(area->area_position.style));
    if (AM_OSD_LAYOUT_MANUAL == area->area_position.style) {
      PRINTF("area %d startX : ",area_id);
      scanf("%d",(int *)&(area->area_position.offset_x));
      PRINTF("area %d startY : ",area_id);
      scanf("%d",(int *)&(area->area_position.offset_x));
    }
    if (op != 'b'){
      PRINTF("area %d width : ",area_id);
      scanf("%d",(int *)&(area->width));
      PRINTF("area %d height : ",area_id);
      scanf("%d",(int *)&(area->height));
    }
    area->enable = 1;
    break;
  } while(1);
}


AMIOSDOverlay *Osd_overlay_obj;
//mode=0,add; mode=1,remove; mode=2,disable; mode=3,enable
void osd_area_manipulate(int32_t mode)
{
  int32_t stream_id = 0, area_id = 0, area_num = 0;
  AMOSDInfo overlay_info = {};
  if (Osd_overlay_obj) {
    PRINTF("Choose a osd stream to %s: \n",
           (mode==0)?"add":((mode==1)?"remove":((mode==2)?"disable":"enable")));
    scanf("%d", &stream_id);
    if (stream_id < 0 || stream_id > AM_STREAM_MAX_NUM){
      PRINTF("Wrong stream ID: %d, set to defalut value(0)!!!\n",stream_id);
      stream_id = 0;
    }

    if (0 == mode) {
      PRINTF("Choose osd area num to add: \n");
      scanf("%d", &area_num);
      if (area_num <= 0 || area_num > OSD_AREA_MAX_NUM){
        PRINTF("Wrong area number: %d, use default value(1)!!!\n",area_num);
        area_num = 1;
      }
      PRINTF("encode stream is %d, area number is %d \n",stream_id, area_num);
      overlay_info.area_num = area_num;
      for (int i=0; i<area_num; i++){
        osd_param_choose(&overlay_info, i);
      }
      Osd_overlay_obj->add((AM_STREAM_ID)stream_id, &overlay_info);
    } else {
      PRINTF("Choose osd area id to %s: \n",
             (mode==1)?"remove":((mode==2)?"disable":"enable"));
      scanf("%d", &area_id);
      if (area_id < 0 || area_id > OSD_AREA_MAX_NUM){
        PRINTF("Wrong area id: %d, use default action!!!\n",area_id);
        if (1 == mode) {
          Osd_overlay_obj->remove((AM_STREAM_ID)stream_id);
        } else if (2 == mode) {
          Osd_overlay_obj->disable((AM_STREAM_ID)stream_id);
        } else {
          Osd_overlay_obj->enable((AM_STREAM_ID)stream_id);
        }
      } else {
        if (1 == mode) {
          Osd_overlay_obj->remove((AM_STREAM_ID)stream_id, area_id);
        } else if (2 == mode) {
          Osd_overlay_obj->disable((AM_STREAM_ID)stream_id, area_id);
        } else {
          Osd_overlay_obj->enable((AM_STREAM_ID)stream_id, area_id);
        }
      }
    }
  } else {
    PRINTF("OSD instance is not created, please create it first!!!");
  }
}

void osd_overlay_demo()
{
  int stream_id = 0;
  char op;
  bool quit_flag = false;

  if (!Osd_overlay_obj) {
    PRINTF("osd hand is NULL!!! \n");
    return;
  }
  do{
    show_overlay_main_menu();
    op = getchar();
    if (op == '\n' || op == EOF)
      op = getchar();
    switch (op){
      case 'd':
        PRINTF("delete all osd area!!!");
          Osd_overlay_obj->destroy();
        break;
      case 'a':
        PRINTF("test add area osd！ \n");
        osd_area_manipulate(0);
        break;
      case 'r':
        PRINTF("test osd remove！\n");
        osd_area_manipulate(1);
        break;
      case 'p':
        PRINTF("test osd disable！\n");
        osd_area_manipulate(2);
        break;
      case 'e':
        PRINTF("test osd enable！\n");
        osd_area_manipulate(3);
        break;
      case 'i':
        PRINTF("Choose a osd stream to print: \n");
        scanf("%d", &stream_id);
        if (stream_id < 0 || stream_id > AM_STREAM_MAX_NUM){
          PRINTF("Wrong stream ID: %d, set to defalut value!!!\n",stream_id);
          stream_id = AM_STREAM_MAX_NUM;
        }
        Osd_overlay_obj->print_osd_infomation((AM_STREAM_ID)stream_id);
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

void clean_function(int signal_id)
{
  PRINTF("catch a signal:%d!", signal_id);
  if (Osd_overlay_obj) {
    Osd_overlay_obj->destroy();
  }
  exit(0);
}

int main()
{
  char ch;
  int quit_flag;
  int error_opt;
  AM_RESULT result = AM_RESULT_OK;
  signal(SIGINT, clean_function);
  signal(SIGQUIT, clean_function);
  signal(SIGTERM, clean_function);

  AMIVideoCameraPtr video_camera = AMIVideoCamera::get_instance();
  do {

    if (!video_camera) {
      ERROR("Videocamera create failed \n");
      result = AM_RESULT_ERR_MEM;
      break;
    }

    result = video_camera->init();
    if (result != AM_RESULT_OK) {
      ERROR("VideoCamera Init failed \n");
      break;
    }

    Osd_overlay_obj = video_camera->get_encode_device()->get_osd_overlay();

    INFO("VideoCamera is running \n");
    show_menu();

    ch = getchar();
    while(ch) {
      quit_flag = 0;
      error_opt = 0;
      switch(ch) {
        case 'm':
          int encode_mode;
          PRINTF("select encode mode for VideoCamera to run\n");
          scanf("%d", &encode_mode);
          PRINTF("encode mode %d selected \n",encode_mode);
          //now try to init video camera with this mode
           //destroy current one, if running, then stop it
          video_camera->destroy();
          result = video_camera->init(AM_ENCODE_MODE(encode_mode));
          if (result != AM_RESULT_OK) {
            ERROR("VideoCamera init by mode %d failed \n", encode_mode);
            break;
          } else {
            PRINTF("VideoCamera changes to use mode %d \n", encode_mode);
          }
          break;

        case 'r':
          INFO("VideoCamera runs \n");
          result = video_camera->start();
          if (result != AM_RESULT_OK) {
               ERROR("VideoCamera runs failed \n");
           }
          break;
        case 's':
          INFO("VideoCamera stops \n");
          result = video_camera->stop();
          if (result != AM_RESULT_OK) {
               ERROR("VideoCamera stops failed \n");
           }
          break;
        case 'p':
          PRINTF("Set more VideoCamera params: \n");
          show_param_menu(video_camera);
          break;

         //disable load config option
        case 'l':
          //load config from file
          result = video_camera->load_config();
          if (result != AM_RESULT_OK) {
            ERROR("VideoCamera load config failed \n");
          }
          break;


        case 'u':
          //Update camera with loaded configuration file
          result = video_camera->update();
          if (result == AM_RESULT_ERR_PERM) {
            WARN("VideoCamera update operation not permitted, ignored.\n");

          } else if (result == AM_RESULT_OK) {
            INFO("VideoCamera update operation OK\n");

          } else {
            ERROR("VideoCamera update operation failed\n");
          }
          break;
        case 'w':
          //write config to file
          result = video_camera->save_config();
          if (result != AM_RESULT_OK) {
             ERROR("VideoCamera save config failed \n");
          }
          break;

        case 'd':
          //do DPTZWarp
          PRINTF("Show DPTZWarp menu \n");
          show_dptz_warp_menu(video_camera);
          break;

        case 'o':
          //do overlay
          PRINTF("Show overlay menu \n");
          osd_overlay_demo();
          break;

        case 'q':
          INFO("VideoCamera quit \n");
          result = video_camera->destroy();
          if (result != AM_RESULT_OK) {
            ERROR("VideoCamera quit & destroy failed \n");
          }
          quit_flag = 1;
          break;
        default:
          error_opt = 1;
          break;
      }

      //quit too if function has error
      if (result != AM_RESULT_OK)
        quit_flag = 1;

      if (quit_flag)
        break;
      if (error_opt == 0)
       show_menu();

      ch = getchar();
    }

  }while(0);

  return result;
}
