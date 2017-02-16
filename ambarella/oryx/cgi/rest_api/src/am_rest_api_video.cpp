/**
 * am_rest_api_handle.cpp
 *
 *  History:
 *		2015年8月19日 - [Huaiqing Wang] created file
 *
 * Copyright (C) 2007-2018, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>

#include "am_rest_api_video.h"
#include "am_video_types.h"
#include "am_define.h"

using std::string;

AM_REST_RESULT AMRestAPIVideo::rest_api_handle()
{
  AM_REST_RESULT ret = AM_REST_RESULT_OK;
  do {
    string cmd_group;
    if ((m_utils->get_value("arg1", cmd_group)) != AM_REST_RESULT_OK) {
      ret = AM_REST_RESULT_ERR_URL;
      m_utils->set_response_msg(AM_REST_URL_ARG1_ERR, "no video cmp group parameter");
      break;
    }
    if ("overlay" == cmd_group) {
      ret = video_overlay_handle();
    } else if ("dptz" == cmd_group) {
      ret = video_dptz_handle();
    } else if ("dewarp" == cmd_group) {
      ret = video_dewarp_handle();
    } else if ("enc_ctrl" == cmd_group) {
      ret = video_enc_ctrl_handle();
    } else {
      ret = AM_REST_RESULT_ERR_URL;
      char msg[MAX_MSG_LENGHT];
      snprintf(msg, MAX_MSG_LENGHT, "invalid video cmp group: %s", cmd_group.c_str());
      m_utils->set_response_msg(AM_REST_URL_ARG1_ERR, msg);
    }
  }while(0);

  return ret;
}

AM_REST_RESULT AMRestAPIVideo::video_overlay_handle()
{
  AM_REST_RESULT ret = AM_REST_RESULT_OK;
  string action;
  do {
    if ((ret = m_utils->get_value("action", action)) != AM_REST_RESULT_OK) {
      m_utils->set_response_msg(AM_REST_OVERLAY_ACTION_ERR, "no overlay action parameter");
      break;
    }
    if ("set" == action) {
      ret = overlay_add_handle();
    } else if ("get" == action) {
      ret = overlay_get_handle();
    } else if ("enable" == action || "disable" == action ||
        "delete" == action) {
      ret = overlay_manipulate_handle();
    } else if ("save" == action) {
      ret = overlay_save_handle();
    } else {
      ret = AM_REST_RESULT_ERR_PARAM;
      char msg[MAX_MSG_LENGHT];
      snprintf(msg, MAX_MSG_LENGHT, "invalid overlay action: %s", action.c_str());
      m_utils->set_response_msg(AM_REST_OVERLAY_ACTION_ERR, msg);
    }
  } while(0);

  return ret;
}

int32_t AMRestAPIVideo::overlay_get_max_num()
{
  int32_t max_num = 0;

  am_service_result_t service_result;
  gCGI_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_OVERLAY_GET_MAX_NUM, nullptr, 0,
                               &service_result, sizeof(service_result));
  if (service_result.ret != 0) {
    max_num = 0;
  } else {
    max_num = (uint32_t)*service_result.data;
  }
  return max_num;
}

AM_REST_RESULT AMRestAPIVideo::overlay_add_handle()
{
  AM_REST_RESULT ret = AM_REST_RESULT_OK;
  am_service_result_t service_result;
  am_overlay_s add_cfg = {0};
  char *extension = nullptr;
  do {
    if ((ret = overlay_parameters_init(&add_cfg)) != AM_REST_RESULT_OK) {
      break;
    }
    if ((add_cfg.area.type != 2) &&
        (add_cfg.area.width <= 0 || add_cfg.area.height <= 0)) {
      ret =   AM_REST_RESULT_ERR_PARAM;
      m_utils->set_response_msg(AM_REST_OVERLAY_W_H_ERR, "invalid width or height");
      break;
    }

    if ((add_cfg.area.type == 0 || add_cfg.area.type == 1) &&
        TEST_BIT(add_cfg.enable_bits, AM_FONT_TYPE_EN_BIT)) {
      if ((-1 == access(add_cfg.area.font_type, F_OK)) ||
          !(extension = strchr(add_cfg.area.font_type, '.')) ||
          (strcmp(extension, ".ttf") && strcmp(extension, ".ttc"))) {
        ret = AM_REST_RESULT_ERR_PARAM;
        char msg[MAX_MSG_LENGHT];
        snprintf(msg, MAX_MSG_LENGHT, "invalid font file:\"%s\"", add_cfg.area.font_type);
        m_utils->set_response_msg(AM_REST_OVERLAY_FONT_ERR, msg);
        break;
      }
    }

    if (0 == add_cfg.area.type) {
      if (TEST_BIT(add_cfg.enable_bits, AM_STRING_EN_BIT)) {
        int32_t i = 0;
        int32_t len = strlen(add_cfg.area.str);
        while((i < len) && (' ' == add_cfg.area.str[i++]));
        if (i == len) {
          ret = AM_REST_RESULT_ERR_PARAM;
          char msg[MAX_MSG_LENGHT];
          snprintf(msg, MAX_MSG_LENGHT, "text string is blank:\"%s\"", add_cfg.area.str);
          m_utils->set_response_msg(AM_REST_OVERLAY_STR_ERR, msg);
          break;
        }
      } else {
        SET_BIT(add_cfg.enable_bits, AM_STRING_EN_BIT);
        snprintf(add_cfg.area.str, OSD_MAX_STRING, "Welcome to Ambarella Shanghai!");
      }
    }

    if (add_cfg.area.type == 2) {
      if (TEST_BIT(add_cfg.enable_bits, AM_BMP_EN_BIT)) {
        if ((-1 == access(add_cfg.area.bmp, F_OK)) ||
            !(extension = strchr(add_cfg.area.bmp, '.')) ||
            strcmp(extension, ".bmp")) {
          ret = AM_REST_RESULT_ERR_PARAM;
          char msg[MAX_MSG_LENGHT];
          snprintf(msg, MAX_MSG_LENGHT, "invalid picture file:\"%s\"", add_cfg.area.bmp);
          m_utils->set_response_msg(AM_REST_OVERLAY_PIC_ERR, msg);
          break;
        }
      } else {
        SET_BIT(add_cfg.enable_bits, AM_BMP_EN_BIT);
        snprintf(add_cfg.area.bmp, OSD_MAX_FILENAME, "/usr/share/oryx/video/overlay/Ambarella-256x128-8bit.bmp");
      }
    }
    gCGI_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_OVERLAY_ADD,
                                 &add_cfg, sizeof(add_cfg),
                                 &service_result, sizeof(service_result));
    if (service_result.ret) {
      ret = AM_REST_RESULT_ERR_SERVER;
      m_utils->set_response_msg(AM_REST_OVERLAY_HANDLE_ERR, "server add overlay failed");
      break;
    }
  } while(0);

  return ret;
}

AM_REST_RESULT AMRestAPIVideo::overlay_get_handle()
{
  AM_REST_RESULT ret = AM_REST_RESULT_OK;
  string value;
  do {
    if ((ret = m_utils->get_value("arg2", value)) != AM_REST_RESULT_OK){
      m_utils->set_response_msg(AM_REST_URL_ARG2_ERR, "no overlay option parameter");
      break;
    } else {
      if ("font" == value) {
        ret = overlay_get_available_font();
      } else if ("bmp" == value) {
        ret = overlay_get_available_bmp();
      } else {
        ret = overlay_get_config();
      }
    }
  } while(0);
  return ret;
}

AM_REST_RESULT AMRestAPIVideo::overlay_get_config()
{
  AM_REST_RESULT ret = AM_REST_RESULT_OK;
  do {
    am_service_result_t service_result;
    am_overlay_id_t input_para = {0};
    int32_t stream_id, area_id;
    if ((ret = get_srteam_id(stream_id, "arg2")) != AM_REST_RESULT_OK){
      break;
    } else if (stream_id<0 || stream_id>=AM_STREAM_MAX_NUM) {
      ret = AM_REST_RESULT_ERR_URL;
      char msg[MAX_MSG_LENGHT];
      snprintf(msg, MAX_MSG_LENGHT, "invalid stream id: %d,overlay valid stream id: 0~%d",
               stream_id,AM_STREAM_MAX_NUM-1);
      m_utils->set_response_msg(AM_REST_OVERLAY_STREAM_ERR, msg);
      break;
    }

    if ((ret = get_area_id(area_id)) != AM_REST_RESULT_OK) {
      break;
    }
    int32_t max_area_num = overlay_get_max_num();
    if ( area_id<0 || area_id>=max_area_num) {
      ret = AM_REST_RESULT_ERR_URL;
      char msg[MAX_MSG_LENGHT];
      snprintf(msg, MAX_MSG_LENGHT, "invalid area id: %d,overlay valid area id: 0~%d",
               area_id, max_area_num-1);
      m_utils->set_response_msg(AM_REST_OVERLAY_AREA_ERR, msg);
      break;
    }

    input_para.stream_id = stream_id;
    input_para.area_id = area_id;
    gCGI_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_OVERLAY_GET,
                                 &input_para, sizeof(input_para),
                                 &service_result, sizeof(service_result));
    if (service_result.ret != 0) {
      ret = AM_REST_RESULT_ERR_SERVER;
      m_utils->set_response_msg(AM_REST_OVERLAY_HANDLE_ERR, "server get overlay config failed");
      break;
    }
    am_overlay_area_t *area = (am_overlay_area_t*)service_result.data;

    m_utils->set_response_data("stream_id", stream_id);
    m_utils->set_response_data("area_id", area_id);
    char state[MAX_PARA_LENGHT];
    snprintf(state, MAX_PARA_LENGHT, "%s", (area->enable == 1) ? "enable" :
        (area->enable == 0) ? "disable" : "not created");
    m_utils->set_response_data("state", area->enable == 1 ? "enable" :
        (area->enable == 0) ? "disable" : "not created");
    m_utils->set_response_data("state", (string)state);
    if (0 == area->enable || 1 == area->enable) {
      m_utils->set_response_data("type", (int32_t)area->type);
      m_utils->set_response_data("layout", (int32_t)area->layout);
      if (4 == area->layout) {
        m_utils->set_response_data("position_x", (int32_t)area->offset_x);
        m_utils->set_response_data("position_y", (int32_t)area->offset_y);
      }
      if (area->type != 2) {
        m_utils->set_response_data("width", (int32_t)area->width);
        m_utils->set_response_data("height", (int32_t)area->height);
      }
      m_utils->set_response_data("rotate", area->rotate?true:false);
      if (0 == area->type || 1 == area->type) {
        m_utils->set_response_data("font_size", (int32_t)area->font_size);
        m_utils->set_response_data("font_color", (double)area->font_color);
        m_utils->set_response_data("font_outwidth", (int32_t)area->font_outline_w);
        m_utils->set_response_data("font_horbold", (int32_t)area->font_hor_bold);
        m_utils->set_response_data("font_verbold", (int32_t)area->font_ver_bold);
        m_utils->set_response_data("font_italic", (int32_t)area->font_italic);
      } else if (2 == area->type) {
        m_utils->set_response_data("color_key", (double)area->color_key);
        m_utils->set_response_data("color_range", (int32_t)area->color_range);
      }
    }
  } while(0);

  return ret;
}

AM_REST_RESULT AMRestAPIVideo::overlay_manipulate_handle()
{
  AM_REST_RESULT ret = AM_REST_RESULT_OK;
  do {
    int32_t stream_id;
    int32_t max_area_num = overlay_get_max_num();
    if ((ret = get_srteam_id(stream_id, "arg2")) != AM_REST_RESULT_OK){
      break;
    }
    if ( stream_id>=0 && stream_id<AM_STREAM_MAX_NUM) {
      int32_t area_id;
      if ((ret = get_area_id(area_id)) != AM_REST_RESULT_OK) {
        break;
      } else if ( area_id<0 || area_id>max_area_num) {
        ret = AM_REST_RESULT_ERR_URL;
        char msg[MAX_MSG_LENGHT];
        snprintf(msg, MAX_MSG_LENGHT, "invalid area id: %d,overlay area valid manipulate range: 0~%d",
                 area_id,max_area_num);
        m_utils->set_response_msg(AM_REST_OVERLAY_AREA_ERR, msg);
        break;
      }
      ret = overlay_manipulate_by_stream(stream_id, area_id);
    } else if (AM_STREAM_MAX_NUM == stream_id) {
      for (int32_t i=0; i<AM_STREAM_MAX_NUM; ++i) {
        ret = overlay_manipulate_by_stream(i, max_area_num);
      }
    } else {
      ret = AM_REST_RESULT_ERR_URL;
      char msg[MAX_MSG_LENGHT];
      snprintf(msg, MAX_MSG_LENGHT, "invalid stream id: %d,overlay stream valid manipulate range: 0～%d",
               stream_id,AM_STREAM_MAX_NUM);
      m_utils->set_response_msg(AM_REST_OVERLAY_STREAM_ERR, msg);
      break;
    }
  } while(0);

  return ret;
}

AM_REST_RESULT AMRestAPIVideo::overlay_save_handle()
{
  AM_REST_RESULT ret = AM_REST_RESULT_OK;
  do {
    am_service_result_t service_result;
    gCGI_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_OVERLAY_SAVE, nullptr, 0,
                                 &service_result, sizeof(service_result));
    if (service_result.ret){
      ret = AM_REST_RESULT_ERR_SERVER;
      m_utils->set_response_msg(AM_REST_OVERLAY_HANDLE_ERR, "server save overlay config failed");
      break;
    }
  } while(0);

  return ret;
}

AM_REST_RESULT AMRestAPIVideo::overlay_parameters_init(am_overlay_s *add_cfg)
{
  AM_REST_RESULT ret = AM_REST_RESULT_OK;
  string value;
  do {
    if (!add_cfg) {
      ret = AM_REST_RESULT_ERR_SERVER;
      m_utils->set_response_msg(AM_REST_OVERLAY_HANDLE_ERR, "server occur error");
      break;
    }
    SET_BIT(add_cfg->enable_bits, AM_ADD_EN_BIT);
    int32_t stream_id;
    if ((ret = get_srteam_id(stream_id, "arg2")) != AM_REST_RESULT_OK) {
      break;
    } else if (stream_id<0 || stream_id>=AM_STREAM_MAX_NUM) {
      ret = AM_REST_RESULT_ERR_URL;
      char msg[MAX_MSG_LENGHT];
      snprintf(msg, MAX_MSG_LENGHT, "invalid stream id: %d",stream_id);
      m_utils->set_response_msg(AM_REST_OVERLAY_STREAM_ERR, msg);
      break;
    }
    add_cfg->stream_id = stream_id;

    SET_BIT(add_cfg->enable_bits, AM_ADD_EN_BIT);
    add_cfg->area.enable = 1;
    if (m_utils->get_value("state", value) == AM_REST_RESULT_OK) {
      if ("disable" == value) {
        add_cfg->area.enable = 0;
      }
    }

    SET_BIT(add_cfg->enable_bits, AM_ROTATE_EN_BIT);
    add_cfg->area.rotate = 1;
    if (m_utils->get_value("rotate", value) == AM_REST_RESULT_OK) {
      int32_t tmp = atoi(value.c_str());
      add_cfg->area.rotate = tmp;
    }

    SET_BIT(add_cfg->enable_bits, AM_TYPE_EN_BIT);
    if ((ret = m_utils->get_value("type", value)) == AM_REST_RESULT_OK) {
      int32_t tmp = atoi(value.c_str());
      if (tmp<0 || tmp>3) {
        ret = AM_REST_RESULT_ERR_PARAM;
        char msg[MAX_MSG_LENGHT];
        snprintf(msg, MAX_MSG_LENGHT, "invalid overlay type: %d",tmp);
        m_utils->set_response_msg(AM_REST_OVERLAY_TYPE_ERR, msg);
        break;
      }
      add_cfg->area.type = tmp;
    }

    SET_BIT(add_cfg->enable_bits, AM_LAYOUT_EN_BIT);
    if (m_utils->get_value("layout", value) == AM_REST_RESULT_OK) {
      int32_t tmp = atoi(value.c_str());
      if (tmp>=0 && tmp<=4) {
        add_cfg->area.layout = tmp;
      } else {
        ret = AM_REST_RESULT_ERR_PARAM;
        char msg[MAX_MSG_LENGHT];
        snprintf(msg, MAX_MSG_LENGHT, "invalid overlay layout: %d",tmp);
        m_utils->set_response_msg(AM_REST_OVERLAY_LAYOUT_ERR, msg);
        break;
      }
    }

    if (m_utils->get_value("position_x", value) == AM_REST_RESULT_OK) {
      SET_BIT(add_cfg->enable_bits, AM_OFFSETX_EN_BIT);
      int32_t tmp = atoi(value.c_str());
      add_cfg->area.offset_x = tmp;
    }

    if (m_utils->get_value("position_y", value) == AM_REST_RESULT_OK) {
      SET_BIT(add_cfg->enable_bits, AM_OFFSETY_EN_BIT);
      int32_t tmp = atoi(value.c_str());
      add_cfg->area.offset_y = tmp;
    }

    if (m_utils->get_value("width", value) == AM_REST_RESULT_OK) {
      SET_BIT(add_cfg->enable_bits, AM_WIDTH_EN_BIT);
      int32_t tmp = atoi(value.c_str());
      add_cfg->area.width = tmp;
    }

    if (m_utils->get_value("height", value) == AM_REST_RESULT_OK) {
      SET_BIT(add_cfg->enable_bits, AM_HEIGHT_EN_BIT);
      int32_t tmp = atoi(value.c_str());
      add_cfg->area.height = tmp;
    }

    if (m_utils->get_value("font_type", value) == AM_REST_RESULT_OK) {
      SET_BIT(add_cfg->enable_bits, AM_FONT_TYPE_EN_BIT);
      snprintf(add_cfg->area.font_type, OSD_MAX_FILENAME,
               "/usr/share/fonts/%s", value.c_str());
    }

    if (m_utils->get_value("font_size", value) == AM_REST_RESULT_OK) {
      SET_BIT(add_cfg->enable_bits, AM_FONT_SIZE_EN_BIT);
      int32_t tmp = atoi(value.c_str());
      add_cfg->area.font_size = tmp;
    }

    if (m_utils->get_value("font_color", value) == AM_REST_RESULT_OK) {
      SET_BIT(add_cfg->enable_bits, AM_FONT_COLOR_EN_BIT);
      uint32_t tmp = (uint32_t)atoll(value.c_str());
      add_cfg->area.font_color = tmp;
    }

    if (m_utils->get_value("font_outwidth", value) == AM_REST_RESULT_OK) {
      SET_BIT(add_cfg->enable_bits, AM_FONT_OUTLINE_W_EN_BIT);
      int32_t tmp = atoi(value.c_str());
      add_cfg->area.font_outline_w = tmp;
    }

    if (m_utils->get_value("font_horbold", value) == AM_REST_RESULT_OK) {
      SET_BIT(add_cfg->enable_bits, AM_FONT_HOR_BOLD_EN_BIT);
      int32_t tmp = atoi(value.c_str());
      add_cfg->area.font_hor_bold = tmp;
    }

    if (m_utils->get_value("font_verbold", value) == AM_REST_RESULT_OK) {
      SET_BIT(add_cfg->enable_bits, AM_FONT_VER_BOLD_EN_BIT);
      int32_t tmp = atoi(value.c_str());
      add_cfg->area.font_ver_bold = tmp;
    }

    if (m_utils->get_value("font_italic", value) == AM_REST_RESULT_OK) {
      SET_BIT(add_cfg->enable_bits, AM_FONT_ITALIC_EN_BIT);
      int32_t tmp = atoi(value.c_str());
      add_cfg->area.font_italic = tmp;
    }

    if (m_utils->get_value("color_key", value) == AM_REST_RESULT_OK) {
      SET_BIT(add_cfg->enable_bits, AM_COLOR_KEY_EN_BIT);
      uint32_t tmp = (uint32_t)atoll(value.c_str());
      add_cfg->area.color_key = tmp;
    }

    if (m_utils->get_value("color_range", value) == AM_REST_RESULT_OK) {
      SET_BIT(add_cfg->enable_bits, AM_COLOR_RANGE_EN_BIT);
      int32_t tmp = atoi(value.c_str());
      add_cfg->area.color_range = tmp;
    }

    if (m_utils->get_value("text_str", value) == AM_REST_RESULT_OK) {
      SET_BIT(add_cfg->enable_bits, AM_STRING_EN_BIT);
      snprintf(add_cfg->area.str, OSD_MAX_STRING, "%s", value.c_str());
    }

    if (m_utils->get_value("bmp_file", value) == AM_REST_RESULT_OK) {
      SET_BIT(add_cfg->enable_bits, AM_BMP_EN_BIT);
      snprintf(add_cfg->area.bmp, OSD_MAX_FILENAME,
               "/usr/share/oryx/video/overlay/%s", value.c_str());
    }
  } while(0);

  return ret;
}

AM_REST_RESULT AMRestAPIVideo::overlay_manipulate_by_stream(int32_t stream_id, int32_t area_id)
{
  AM_REST_RESULT ret = AM_REST_RESULT_OK;
  am_service_result_t service_result;
  string value;
  do {
    am_overlay_set_t set_cfg = {0};
    set_cfg.osd_id.stream_id = stream_id;
    set_cfg.osd_id.area_id = area_id;
    if ((ret = m_utils->get_value("action", value)) != AM_REST_RESULT_OK) {
      ret = AM_REST_RESULT_ERR_PARAM;
      m_utils->set_response_msg(AM_REST_OVERLAY_ACTION_ERR, "no overlay action");
      break;
    } else if ("delete" == value) {
      SET_BIT(set_cfg.enable_bits, AM_REMOVE_EN_BIT);
    } else if ("enable" == value) {
      SET_BIT(set_cfg.enable_bits, AM_ENABLE_EN_BIT);
    } else if ("disable" == value) {
      SET_BIT(set_cfg.enable_bits, AM_DISABLE_EN_BIT);
    } else {
      ret = AM_REST_RESULT_ERR_PARAM;
      char msg[MAX_MSG_LENGHT];
      snprintf(msg, MAX_MSG_LENGHT, "invalid overlay action: %s", value.c_str());
      m_utils->set_response_msg(AM_REST_OVERLAY_ACTION_ERR, msg);
      break;
    }

    gCGI_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_OVERLAY_SET,
                                 &set_cfg, sizeof(set_cfg),
                                 &service_result, sizeof(service_result));
    if (service_result.ret != 0) {
      ret = AM_REST_RESULT_ERR_SERVER;
      char msg[MAX_MSG_LENGHT];
      snprintf(msg, MAX_MSG_LENGHT, "server %s overlay failed", value.c_str());
      m_utils->set_response_msg(AM_REST_OVERLAY_HANDLE_ERR, msg);
      break;
    }
  } while(0);

  return ret;
}

AM_REST_RESULT  AMRestAPIVideo::overlay_get_available_font()
{
  AM_REST_RESULT ret = AM_REST_RESULT_OK;
  do {
    DIR *dir;
    struct dirent *ptr;
    char *font_extension = nullptr;

    if ((dir=opendir("/usr/share/fonts/")) == nullptr)
    {
      break;
    }
    while ((ptr = readdir(dir)) != nullptr)
    {
      if(0 == strcmp(ptr->d_name, ".") || 0 == strcmp(ptr->d_name,".."))
        continue;
      else if((DT_REG == ptr->d_type) &&
          (font_extension = strchr(ptr->d_name, '.')) != nullptr)
      {
        if (0 == strcmp(font_extension, ".ttf") || 0 == strcmp(font_extension, ".ttc")) {
          m_utils->set_response_data((string)ptr->d_name);
        }
      }
    }
    closedir(dir);
  } while(0);

  return ret;
}

AM_REST_RESULT  AMRestAPIVideo::overlay_get_available_bmp()
{
  AM_REST_RESULT ret = AM_REST_RESULT_OK;
  do {
    DIR *dir;
    struct dirent *ptr;
    char *bmp_extension = nullptr;

    if ((dir=opendir("/usr/share/oryx/video/overlay/")) == nullptr)
    {
      break;
    }
    while ((ptr = readdir(dir)) != nullptr)
    {
      if(0 == strcmp(ptr->d_name, ".") || 0 == strcmp(ptr->d_name,".."))
        continue;
      else if((DT_REG == ptr->d_type) &&
          (bmp_extension = strchr(ptr->d_name, '.')) != nullptr)
      {
        if (0 == strcmp(bmp_extension, ".bmp")) {
          m_utils->set_response_data((string)ptr->d_name);
        }
      }
    }
    closedir(dir);
  } while(0);

  return ret;
}

AM_REST_RESULT AMRestAPIVideo::video_dptz_handle()
{
  AM_REST_RESULT ret = AM_REST_RESULT_OK;
  do {
    string action;
    if ((ret = m_utils->get_value("action", action)) != AM_REST_RESULT_OK) {
      m_utils->set_response_msg(AM_REST_DPTZ_ACTION_ERR, "no dptz action parameter");
      break;
    }
    if ("set" == action) {
      ret = dptz_set_handle();
    } else if ("get" == action) {
      ret = dptz_get_handle();
    } else {
      ret = AM_REST_RESULT_ERR_PARAM;
      char msg[MAX_MSG_LENGHT];
      snprintf(msg, MAX_MSG_LENGHT, "invalid dptz action: %s", action.c_str());
      m_utils->set_response_msg(AM_REST_DPTZ_ACTION_ERR, msg);
    }
  } while(0);

  return ret;
}

AM_REST_RESULT AMRestAPIVideo::dptz_set_handle()
{
  AM_REST_RESULT ret = AM_REST_RESULT_OK;
  do {
    am_service_result_t service_result;
    am_dptz_warp_t dptz_warp_cfg = {0};
    string value;

    int32_t buffer_id;
    if ((ret = get_buffer_id(buffer_id)) != AM_REST_RESULT_OK) {
      break;
    } else if (buffer_id<0 || buffer_id>=AM_ENCODE_SOURCE_BUFFER_MAX_NUM) {
      ret = AM_REST_RESULT_ERR_URL;
      char msg[MAX_MSG_LENGHT];
      snprintf(msg, MAX_MSG_LENGHT, "invalid buffer id: %d, dptz buffer valid manipulate range: 0～%d",
               buffer_id,AM_ENCODE_SOURCE_BUFFER_MAX_NUM-1);
      m_utils->set_response_msg(AM_REST_DPTZ_BUFFER_ERR, msg);
      break;
    }
    dptz_warp_cfg.buf_id = buffer_id;

    if (m_utils->get_value("zoom_ratio", value) == AM_REST_RESULT_OK) {
      float tmp = atof(value.c_str());
      if (tmp<0.0 || tmp>8.0) {
        ret = AM_REST_RESULT_ERR_PARAM;
        char msg[MAX_MSG_LENGHT];
        snprintf(msg, MAX_MSG_LENGHT, "zoom_ratio value: %f is out of range: [0.0, 8.0]",tmp);
        m_utils->set_response_msg(AM_REST_DPTZ_ZOOM_RATIO_ERR, msg);
        break;
      }
      SET_BIT(dptz_warp_cfg.enable_bits, AM_DPTZ_WARP_ZOOM_RATIO_EN_BIT);
      dptz_warp_cfg.zoom_ratio = tmp;
    }
    if (m_utils->get_value("pan_ratio", value) == AM_REST_RESULT_OK) {
      float tmp = atof(value.c_str());
      if (tmp<-1.0 || tmp>1.0) {
        ret = AM_REST_RESULT_ERR_PARAM;
        char msg[MAX_MSG_LENGHT];
        snprintf(msg, MAX_MSG_LENGHT, "pan_ratio value: %f is out of range: [-1.0, 1.0]",tmp);
        m_utils->set_response_msg(AM_REST_DPTZ_PAN_RATIO_ERR, msg);
        break;
      }
      SET_BIT(dptz_warp_cfg.enable_bits, AM_DPTZ_WARP_PAN_RATIO_EN_BIT);
      dptz_warp_cfg.pan_ratio = tmp;
    }
    if (m_utils->get_value("tilt_ratio", value) == AM_REST_RESULT_OK) {
      float tmp = atof(value.c_str());
      if (tmp<-1.0 || tmp>1.0) {
        ret = AM_REST_RESULT_ERR_PARAM;
        char msg[MAX_MSG_LENGHT];
        snprintf(msg, MAX_MSG_LENGHT, "tilt_ratio value: %f is out of range: [-1.0, 1.0]",tmp);
        m_utils->set_response_msg(AM_REST_DPTZ_TILT_RATIO_ERR, msg);
        break;
      }
      SET_BIT(dptz_warp_cfg.enable_bits, AM_DPTZ_WARP_TILT_RATIO_EN_BIT);
      dptz_warp_cfg.tilt_ratio = tmp;
    }

    gCGI_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_DPTZ_WARP_SET,
                                 &dptz_warp_cfg, sizeof(dptz_warp_cfg),
                                 &service_result, sizeof(service_result));
    if ((service_result.ret) != 0) {
      ret = AM_REST_RESULT_ERR_SERVER;
      m_utils->set_response_msg(AM_REST_DPTZ_HANDLE_ERR, "server set dptz failed");
      break;
    }
  } while(0);

  return ret;
}

AM_REST_RESULT AMRestAPIVideo::dptz_get_handle()
{
  AM_REST_RESULT ret = AM_REST_RESULT_OK;
  do {
    am_service_result_t service_result;
    am_dptz_warp_t *cfg = nullptr;

    int32_t buffer_id;
    if ((ret = get_buffer_id(buffer_id)) != AM_REST_RESULT_OK) {
      break;
    } else if (buffer_id<0 || buffer_id>=AM_ENCODE_SOURCE_BUFFER_MAX_NUM) {
      ret = AM_REST_RESULT_ERR_URL;
      char msg[MAX_MSG_LENGHT];
      snprintf(msg, MAX_MSG_LENGHT, "invalid buffer id: %d, dptz buffer valid manipulate range: 0～%d",
               buffer_id,AM_ENCODE_SOURCE_BUFFER_MAX_NUM-1);
      m_utils->set_response_msg(AM_REST_DPTZ_BUFFER_ERR, msg);
      break;
    }

    gCGI_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_DPTZ_WARP_GET,
                              &buffer_id, sizeof(buffer_id),
                              &service_result, sizeof(service_result));
    if (service_result.ret != 0) {
      ret = AM_REST_RESULT_ERR_SERVER;
      m_utils->set_response_msg(AM_REST_DEWARP_HANDLE_ERR, "server get dptz failed");
      break;
    }
    cfg = (am_dptz_warp_t*)service_result.data;
    m_utils->set_response_data("buffer_id", buffer_id);
    m_utils->set_response_data("zoom_ratio", cfg->zoom_ratio);
    m_utils->set_response_data("pan_ratio", cfg->pan_ratio);
    m_utils->set_response_data("tilt_ratio", cfg->tilt_ratio);
  } while(0);

  return ret;
}

AM_REST_RESULT AMRestAPIVideo::video_dewarp_handle()
{
  AM_REST_RESULT ret = AM_REST_RESULT_OK;
  do {
    string action;
    if ((ret = m_utils->get_value("action", action)) != AM_REST_RESULT_OK) {
      m_utils->set_response_msg(AM_REST_DEWARP_ACTION_ERR, "no dewarp action parameter");
      break;
    }
    if ("set" == action) {
      ret = dewarp_set_handle();
    } else if ("get" == action) {
      ret = dewarp_get_handle();
    } else {
      ret = AM_REST_RESULT_ERR_PARAM;
      char msg[MAX_MSG_LENGHT];
      snprintf(msg, MAX_MSG_LENGHT, "invalid dewarp action: %s", action.c_str());
      m_utils->set_response_msg(AM_REST_DEWARP_ACTION_ERR, msg);
    }
  } while(0);

  return ret;
}

AM_REST_RESULT AMRestAPIVideo::dewarp_set_handle()
{
  AM_REST_RESULT ret = AM_REST_RESULT_OK;
  do {
    am_service_result_t service_result;
    am_dptz_warp_t dptz_warp_cfg = {0};
    string value;

    if (m_utils->get_value("ldc_strength", value) == AM_REST_RESULT_OK) {
      float tmp = atof(value.c_str());
      if (tmp<0.0 || tmp>20.0) {
        ret = AM_REST_RESULT_ERR_PARAM;
        char msg[MAX_MSG_LENGHT];
        snprintf(msg, MAX_MSG_LENGHT, "ldc_strength value: %f is out of range: [0.0, 20.0]",tmp);
        m_utils->set_response_msg(AM_REST_DEWARP_LDC_STRENGTH_ERR, msg);
        break;
      }
      SET_BIT(dptz_warp_cfg.enable_bits, AM_DPTZ_WARP_LDC_STRENGTH_EN_BIT);
      dptz_warp_cfg.ldc_strenght = tmp;
    }

    gCGI_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_DPTZ_WARP_SET,
                                 &dptz_warp_cfg, sizeof(dptz_warp_cfg),
                                 &service_result, sizeof(service_result));
    if ((service_result.ret) != 0) {
      ret = AM_REST_RESULT_ERR_SERVER;
      m_utils->set_response_msg(AM_REST_DEWARP_HANDLE_ERR, "server set dewarp failed");
    }
  } while(0);

  return ret;
}

AM_REST_RESULT AMRestAPIVideo::dewarp_get_handle()
{
  AM_REST_RESULT ret = AM_REST_RESULT_OK;
  do {
    am_service_result_t service_result;
    am_dptz_warp_t *cfg = nullptr;

    int32_t buffer_id = 0;
    gCGI_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_DPTZ_WARP_GET,
                              &buffer_id, sizeof(buffer_id),
                              &service_result, sizeof(service_result));
    if (service_result.ret != 0) {
      ret = AM_REST_RESULT_ERR_SERVER;
      m_utils->set_response_msg(AM_REST_DEWARP_HANDLE_ERR, "server get dewarp failed");
      break;
    }
    cfg = (am_dptz_warp_t*)service_result.data;
    m_utils->set_response_data("ldc_strength", cfg->ldc_strenght);
  } while(0);

  return ret;
}

AM_REST_RESULT AMRestAPIVideo::video_enc_ctrl_handle()
{
  AM_REST_RESULT ret = AM_REST_RESULT_OK;
  do {
    string option, action;
    if ((m_utils->get_value("arg2", option)) != AM_REST_RESULT_OK) {
      ret = AM_REST_RESULT_ERR_URL;
      m_utils->set_response_msg(AM_REST_URL_ARG2_ERR, "no enc_ctrl option");
      break;
    }
    if ("stream" == option) {
      if ((ret = m_utils->get_value("action", action)) != AM_REST_RESULT_OK) {
        m_utils->set_response_msg(AM_REST_ENC_CTRL_ACTION_ERR, "no stream action, available action:[enable,disable,get]");
        break;
      }
      if ("enable" == action) {
        ret = enc_ctrl_start_stream();
      } else if ("disable" == action) {
        ret = enc_ctrl_stop_stream();
      } else if ("get" == action) {
        ret = enc_ctrl_get_stream_status();
      } else {
        ret = AM_REST_RESULT_ERR_PARAM;
        char msg[MAX_MSG_LENGHT];
        snprintf(msg, MAX_MSG_LENGHT, "invalid stream action: %s", action.c_str());
        m_utils->set_response_msg(AM_REST_ENC_CTRL_ACTION_ERR, msg);
        break;
      }
    } else if ("force_idr" == option) {
      ret = enc_ctrl_force_idr();
    } else if (0) {
      //TODO:other enc_ctrl option to add
    } else {
      ret = AM_REST_RESULT_ERR_URL;
      char msg[MAX_MSG_LENGHT];
      snprintf(msg, MAX_MSG_LENGHT, "invalid enc_ctrl option action: %s", option.c_str());
      m_utils->set_response_msg(AM_REST_URL_ARG2_ERR, "invalid enc_ctrl option");
      break;
    }
  } while(0);

  return ret;
}

AM_REST_RESULT AMRestAPIVideo::enc_ctrl_start_stream()
{
  AM_REST_RESULT ret = AM_REST_RESULT_OK;
  do {
    am_service_result_t service_result;
    gCGI_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_ENCODE_START, nullptr, 0,
                                 &service_result, sizeof(service_result));
    if (service_result.ret != 0) {
      ret = AM_REST_RESULT_ERR_SERVER;
      m_utils->set_response_msg(AM_REST_ENC_CTRL_HANDLE_ERR, "server start stream failed");
      break;
    }
  } while(0);
  return ret;
}

AM_REST_RESULT AMRestAPIVideo::enc_ctrl_stop_stream()
{
  AM_REST_RESULT ret = AM_REST_RESULT_OK;
  do {
    am_service_result_t service_result;
    gCGI_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_ENCODE_STOP, nullptr, 0,
                              &service_result, sizeof(service_result));
    if (service_result.ret != 0) {
      ret = AM_REST_RESULT_ERR_SERVER;
      m_utils->set_response_msg(AM_REST_ENC_CTRL_HANDLE_ERR, "server stop stream failed");
      break;
    }
  } while(0);
  return ret;
}

AM_REST_RESULT  AMRestAPIVideo::enc_ctrl_get_stream_status()
{
  AM_REST_RESULT ret = AM_REST_RESULT_OK;
  do {
    am_service_result_t service_result;
    am_stream_status_t *status = nullptr;
    gCGI_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_STREAM_STATUS_GET, nullptr, 0,
                              &service_result, sizeof(service_result));
    if (service_result.ret != 0) {
      ret = AM_REST_RESULT_ERR_SERVER;
      m_utils->set_response_msg(AM_REST_ENC_CTRL_HANDLE_ERR, "server get stream status failed");
      break;
    }
    status = (am_stream_status_t*)service_result.data;
    for (uint32_t i = 0; i < AM_STREAM_MAX_NUM; ++i) {
      char streamid[MAX_PARA_LENGHT];
      snprintf(streamid, MAX_PARA_LENGHT, "stream[%d]", i);
      if (TEST_BIT(status->status, i)) {
        m_utils->set_response_data(streamid, (string)"encoding");
      } else {
        m_utils->set_response_data(streamid, (string)"not encoding");
      }
    }
  } while(0);

  return ret;
}

AM_REST_RESULT  AMRestAPIVideo::enc_ctrl_force_idr()
{
  AM_REST_RESULT ret = AM_REST_RESULT_OK;
  do {
    am_service_result_t service_result;
    int32_t stream_id;
    if ((ret = get_srteam_id(stream_id, "arg3")) != AM_REST_RESULT_OK){
      break;
    } else if (stream_id<0 || stream_id>=AM_STREAM_MAX_NUM) {
      ret = AM_REST_RESULT_ERR_URL;
      char msg[MAX_MSG_LENGHT];
      snprintf(msg, MAX_MSG_LENGHT, "invalid stream id: %d, valid stream id: 0~%d",
               stream_id,AM_STREAM_MAX_NUM-1);
      m_utils->set_response_msg(AM_REST_ENC_CTRL_STREAM_ERR, msg);
      break;
    }
    gCGI_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_ENCODE_FORCE_IDR,
                                 &stream_id, sizeof(int32_t),
                                 &service_result, sizeof(service_result));
    if (service_result.ret != 0) {
      ret = AM_REST_RESULT_ERR_SERVER;
      m_utils->set_response_msg(AM_REST_ENC_CTRL_HANDLE_ERR, "server set force_idr failed");
      break;
    }
  } while(0);

  return ret;
}

AM_REST_RESULT  AMRestAPIVideo::get_srteam_id(int32_t &stream_id, const std::string &arg_name)
{
  AM_REST_RESULT ret = AM_REST_RESULT_OK;
  string value;
  do {
    AM_REST_MSG_CODE err_code;
    if ("arg2" == arg_name) {
      err_code = AM_REST_URL_ARG2_ERR;
    } else if ("arg3" == arg_name) {
      err_code = AM_REST_URL_ARG3_ERR;
    }
    if ((ret = m_utils->get_value(arg_name, value)) != AM_REST_RESULT_OK) {
      ret = AM_REST_RESULT_ERR_URL;
      m_utils->set_response_msg(err_code, "no stream id");
      break;
    } else {
      if (!strncmp(value.c_str(), "stream", 6) && (value.size() == 7) && (isdigit(value[6]))) {
        char tmp[MAX_PARA_LENGHT];
        sscanf(value.c_str(),"stream%[0-9]",tmp);
        stream_id = atoi(tmp);
      } else {
        ret = AM_REST_RESULT_ERR_URL;
        char msg[MAX_MSG_LENGHT];
        snprintf(msg, MAX_MSG_LENGHT, "invalid stream id format: %s", value.c_str());
        m_utils->set_response_msg(err_code, msg);
        break;
      }
    }
  } while(0);

  return ret;
}

AM_REST_RESULT  AMRestAPIVideo::get_area_id(int32_t &area_id)
{
  AM_REST_RESULT ret = AM_REST_RESULT_OK;
  string value;
  do {
    if ((ret = m_utils->get_value("arg3", value)) != AM_REST_RESULT_OK) {
      ret = AM_REST_RESULT_ERR_URL;
      m_utils->set_response_msg(AM_REST_URL_ARG3_ERR, "no area id");
      break;
    } else {
      if (!strncmp(value.c_str(), "area", 4) && (value.size() == 5) && (isdigit(value[4]))) {
        char tmp[MAX_PARA_LENGHT];
        sscanf(value.c_str(),"area%[0-9]",tmp);
        area_id = atoi(tmp);
      } else {
        ret = AM_REST_RESULT_ERR_URL;
        char msg[MAX_MSG_LENGHT];
        snprintf(msg, MAX_MSG_LENGHT, "invalid area id format: %s", value.c_str());
        m_utils->set_response_msg(AM_REST_URL_ARG3_ERR, msg);
        break;
      }
    }
  } while(0);

  return ret;
}

AM_REST_RESULT  AMRestAPIVideo::get_buffer_id(int32_t &buffer_id)
{
  AM_REST_RESULT ret = AM_REST_RESULT_OK;
  string value;
  do {
    if ((ret = m_utils->get_value("arg2", value)) != AM_REST_RESULT_OK) {
      ret = AM_REST_RESULT_ERR_URL;
      m_utils->set_response_msg(AM_REST_URL_ARG2_ERR, "no buffer id");
      break;
    } else {
      if (!strncmp(value.c_str(), "buffer", 6) && (value.size() == 7) && (isdigit(value[6]))) {
        char tmp[MAX_PARA_LENGHT];
        sscanf(value.c_str(),"buffer%[0-9]",tmp);
        buffer_id = atoi(tmp);
      } else {
        ret = AM_REST_RESULT_ERR_URL;
        char msg[MAX_MSG_LENGHT];
        snprintf(msg, MAX_MSG_LENGHT, "invalid buffer id format: %s", value.c_str());
        m_utils->set_response_msg(AM_REST_URL_ARG2_ERR, msg);
        break;
      }
    }
  } while(0);

  return ret;
}
