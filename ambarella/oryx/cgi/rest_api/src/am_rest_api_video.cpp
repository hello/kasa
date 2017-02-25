/**
 * am_rest_api_handle.cpp
 *
 *  History:
 *		2015/08/19 - [Huaiqing Wang] created file
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
#include <string.h>
#include <dirent.h>
#include <unistd.h>

#include "am_rest_api_video.h"
#include "am_define.h"

using std::string;

AM_REST_RESULT AMRestAPIVideo::rest_api_handle()
{
  AM_REST_RESULT ret = AM_REST_RESULT_OK;
  do {
    string cmd_group;
    if ((m_utils->get_value("arg1", cmd_group)) != AM_REST_RESULT_OK) {
      ret = AM_REST_RESULT_ERR_URL;
      m_utils->set_response_msg(AM_REST_URL_ARG1_ERR,
                                "no video cmp group parameter");
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
      snprintf(msg, MAX_MSG_LENGHT, "invalid video cmp group: %s",
               cmd_group.c_str());
      m_utils->set_response_msg(AM_REST_URL_ARG1_ERR, msg);
    }
  }while(0);

  return ret;
}

AM_REST_RESULT  AMRestAPIVideo::video_load_all_cfg()
{
  AM_REST_RESULT ret = AM_REST_RESULT_OK;
  am_service_result_t service_result;

  METHOD_CALL(AM_IPC_MW_CMD_VIDEO_CFG_ALL_LOAD, nullptr, 0,
              &service_result, sizeof(service_result), ret);
  if (service_result.ret != 0) {
    ret = AM_REST_RESULT_ERR_SERVER;
    m_utils->set_response_msg(AM_REST_VIDEO_CFG_HANDLE_ERR,
                              "server load video configure file failed");
  }

  return ret;
}

AM_REST_RESULT AMRestAPIVideo::video_overlay_handle()
{
  AM_REST_RESULT ret = AM_REST_RESULT_OK;
  string action;
  do {
    if ((ret = m_utils->get_value("action", action)) != AM_REST_RESULT_OK) {
      m_utils->set_response_msg(AM_REST_OVERLAY_ACTION_ERR,
                                "no overlay action parameter");
      break;
    }

    if ("set" == action) {
      ret = overlay_set_handle();
    } else if ("get" == action) {
      ret = overlay_get_handle();
    } else if ("enable" == action || "disable" == action ||
        "delete" == action || "destory" == action) {
      ret = overlay_manipulate_handle(action);
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

AM_REST_RESULT AMRestAPIVideo::overlay_get_max_num(int32_t &value)
{
  AM_REST_RESULT ret = AM_REST_RESULT_OK;
  do {
    am_service_result_t service_result;
    METHOD_CALL(AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_MAX_NUM_GET, nullptr, 0,
                &service_result, sizeof(service_result), ret);
    if (ret != AM_REST_RESULT_OK || service_result.ret != 0) {
      value = 0;
      break;
    }
    am_overlay_limit_val_t val = {0};
    memcpy(&val, service_result.data, sizeof(am_overlay_limit_val_t));
    value = val.user_def_overlay_area_num_max;
  } while(0);

  return ret;
}

AM_REST_RESULT AMRestAPIVideo::overlay_set_handle()
{
  AM_REST_RESULT ret = AM_REST_RESULT_OK;
  do {
    string value;
    if (m_utils->get_value("arg3", value) != AM_REST_RESULT_OK) {
      ret = overlay_init_handle();
    } else {
      ret = overlay_data_manipulate_handle();
    }
  } while(0);

  return ret;
}

AM_REST_RESULT AMRestAPIVideo::overlay_init_handle()
{
  AM_REST_RESULT ret = AM_REST_RESULT_OK;
  am_service_result_t service_result;
  do {
    am_overlay_area_t area = {0};

    SET_BIT(area.enable_bits, AM_OVERLAY_INIT_EN_BIT);
    int32_t stream_id;
    if ((ret = get_srteam_id(stream_id, "arg2")) != AM_REST_RESULT_OK) {
      break;
    }
    area.stream_id = stream_id;

    string value;
    SET_BIT(area.enable_bits, AM_OVERLAY_ROTATE_EN_BIT);
    area.rotate = 1;
    if (m_utils->get_value("rotate", value) == AM_REST_RESULT_OK) {
      area.rotate = atoi(value.c_str());
    }

    if (m_utils->get_value("position_x", value) == AM_REST_RESULT_OK) {
      SET_BIT(area.enable_bits, AM_OVERLAY_RECT_EN_BIT);
      area.offset_x = atoi(value.c_str());
    }

    if (m_utils->get_value("position_y", value) == AM_REST_RESULT_OK) {
      SET_BIT(area.enable_bits, AM_OVERLAY_RECT_EN_BIT);
      area.offset_y = atoi(value.c_str());
    }

    if (m_utils->get_value("width", value) == AM_REST_RESULT_OK) {
      SET_BIT(area.enable_bits, AM_OVERLAY_RECT_EN_BIT);
      area.width = atoi(value.c_str());
    }

    if (m_utils->get_value("height", value) == AM_REST_RESULT_OK) {
      SET_BIT(area.enable_bits, AM_OVERLAY_RECT_EN_BIT);
      area.height = atoi(value.c_str());
    }

    if (m_utils->get_value("bg_color", value) == AM_REST_RESULT_OK) {
      SET_BIT(area.enable_bits, AM_OVERLAY_BG_COLOR_EN_BIT);
      area.bg_color = atoi(value.c_str());
    }

    if (m_utils->get_value("buf_num", value) == AM_REST_RESULT_OK) {
      SET_BIT(area.enable_bits, AM_OVERLAY_BUF_NUM_EN_BIT);
      area.buf_num = atoi(value.c_str());
    }

    METHOD_CALL(AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_INIT, &area, sizeof(area),
                &service_result, sizeof(service_result), ret);
    if (service_result.ret) {
      ret = AM_REST_RESULT_ERR_SERVER;
      m_utils->set_response_msg(AM_REST_OVERLAY_HANDLE_ERR,
                                "server init overlay area failed");
      break;
    }
  } while(0);

  return ret;
}

AM_REST_RESULT AMRestAPIVideo::overlay_data_manipulate_handle()
{
  AM_REST_RESULT ret = AM_REST_RESULT_OK;
  do {
    string data_id;
    if (m_utils->get_value("arg4", data_id) != AM_REST_RESULT_OK) {
      ret = overlay_data_add_handle();
    } else {
      char tmp[MAX_PARA_LENGHT];
      sscanf(data_id.c_str(),"block%[0-9]",tmp);
      ret = overlay_data_update_handle(atoi(tmp));
    }
  } while(0);

  return ret;
}

AM_REST_RESULT AMRestAPIVideo::overlay_data_update_handle(int32_t data_id)
{
  AM_REST_RESULT ret = AM_REST_RESULT_OK;
  am_service_result_t service_result;
  am_overlay_data_t  data = {0};
  do {
    if ((ret = overlay_parameters_init(data)) != AM_REST_RESULT_OK) {
      break;
    }

    SET_BIT(data.enable_bits, AM_OVERLAY_DATA_UPDATE_EN_BIT);
    data.id.data_index = data_id;

    METHOD_CALL(AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_DATA_UPDATE, &data, sizeof(data),
                &service_result, sizeof(service_result), ret);
    if (service_result.ret) {
      ret = AM_REST_RESULT_ERR_SERVER;
      m_utils->set_response_msg(AM_REST_OVERLAY_HANDLE_ERR, "server overlay add"
          " data to area failed");
      break;
    }
  } while(0);

  return ret;
}

AM_REST_RESULT AMRestAPIVideo::overlay_data_add_handle()
{
  AM_REST_RESULT ret = AM_REST_RESULT_OK;
  am_service_result_t service_result;
  am_overlay_data_t  data = {0};
  char *extension = nullptr;
  do {
    if ((ret = overlay_parameters_init(data)) != AM_REST_RESULT_OK) {
      break;
    }

    SET_BIT(data.enable_bits, AM_OVERLAY_DATA_ADD_EN_BIT);
    //check string or time type parameter
    if (data.type == 0 || data.type == 2) {
      if (data.width <= 0 || data.height <= 0) {
        ret =   AM_REST_RESULT_ERR_PARAM;
        m_utils->set_response_msg(AM_REST_OVERLAY_W_H_ERR,
                                  "invalid width or height");
        break;
      }
      if (TEST_BIT(data.enable_bits, AM_OVERLAY_FONT_TYPE_EN_BIT)) {
        if ((-1 == access(data.font_type, F_OK)) ||
            !(extension = strchr(data.font_type, '.')) ||
            (strcmp(extension, ".ttf") && strcmp(extension, ".ttc"))) {
          ret = AM_REST_RESULT_ERR_PARAM;
          char msg[MAX_MSG_LENGHT];
          snprintf(msg, MAX_MSG_LENGHT, "invalid font file:\"%s\"",
                   data.font_type);
          m_utils->set_response_msg(AM_REST_OVERLAY_FONT_ERR, msg);
          break;
        }
      }
    }

    if (0 == data.type) {
      if (TEST_BIT(data.enable_bits, AM_OVERLAY_STRING_EN_BIT)) {
        int32_t i = 0;
        int32_t len = strlen(data.str);
        while((i < len) && (' ' == data.str[i++]));
        if (i == len) {
          ret = AM_REST_RESULT_ERR_PARAM;
          char msg[MAX_MSG_LENGHT];
          snprintf(msg, MAX_MSG_LENGHT, "text string is blank:\"%s\"",data.str);
          m_utils->set_response_msg(AM_REST_OVERLAY_STR_ERR, msg);
          break;
        }
      } else {
        SET_BIT(data.enable_bits, AM_OVERLAY_STRING_EN_BIT);
        snprintf(data.str, OVERLAY_MAX_STRING,"Welcome to Ambarella Shanghai!");
      }
    }

    if (data.type == 1 || data.type == 3) {
      if (TEST_BIT(data.enable_bits, AM_OVERLAY_BMP_EN_BIT)) {
        if ((-1 == access(data.bmp, F_OK)) ||
            !(extension = strchr(data.bmp, '.')) ||
            strcmp(extension, ".bmp")) {
          ret = AM_REST_RESULT_ERR_PARAM;
          char msg[MAX_MSG_LENGHT];
          snprintf(msg, MAX_MSG_LENGHT, "invalid picture file:\"%s\"",data.bmp);
          m_utils->set_response_msg(AM_REST_OVERLAY_PIC_ERR, msg);
          break;
        }
      } else {
        SET_BIT(data.enable_bits, AM_OVERLAY_BMP_EN_BIT);
        if (data.type == 1) {
          snprintf(data.bmp, OVERLAY_MAX_FILENAME,
                   "/usr/share/oryx/video/overlay/Ambarella-256x128-8bit.bmp");
        } else {
          SET_BIT(data.enable_bits, AM_OVERLAY_ANIMATION_EN_BIT);
          snprintf(data.bmp, OVERLAY_MAX_FILENAME,
                   "/usr/share/oryx/video/overlay/Animation-logo.bmp");
          data.bmp_num = 4;
          data.interval = 10;
        }
      }
    }
    METHOD_CALL(AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_DATA_ADD, &data, sizeof(data),
                &service_result, sizeof(service_result), ret);
    if (service_result.ret) {
      ret = AM_REST_RESULT_ERR_SERVER;
      m_utils->set_response_msg(AM_REST_OVERLAY_HANDLE_ERR, "server overlay add"
          " data to area failed");
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
    if ((ret = m_utils->get_value("arg2", value)) != AM_REST_RESULT_OK) {
      m_utils->set_response_msg(AM_REST_URL_ARG2_ERR, "no overlay option "
          "parameter");
      break;
    } else {
      if ("font" == value) {
        ret = overlay_get_available_font();
      } else if ("bmp" == value) {
        ret = overlay_get_available_bmp();
      } else {
        ret = overlay_get_area_params();
      }
    }
  } while(0);
  return ret;
}

AM_REST_RESULT AMRestAPIVideo::overlay_get_area_params()
{
  AM_REST_RESULT ret = AM_REST_RESULT_OK;
  do {
    am_service_result_t service_result;
    am_overlay_id_t input_para = {0};
    int32_t stream_id, area_id;
    if ((ret = get_srteam_id(stream_id, "arg2")) != AM_REST_RESULT_OK){
      break;
    }
    if ((ret = get_area_id(area_id)) != AM_REST_RESULT_OK) {
      break;
    }
    int32_t max_area_num = 0;
    if ((ret=overlay_get_max_num(max_area_num)) != AM_REST_RESULT_OK) {
      break;
    }
    if ( area_id<0 || area_id>=max_area_num) {
      ret = AM_REST_RESULT_ERR_URL;
      char msg[MAX_MSG_LENGHT];
      snprintf(msg, MAX_MSG_LENGHT, "invalid area id: %d,overlay valid area id:"
          " 0~%d", area_id, max_area_num-1);
      m_utils->set_response_msg(AM_REST_OVERLAY_AREA_ERR, msg);
      break;
    }

    input_para.stream_id = stream_id;
    input_para.area_id = area_id;
    METHOD_CALL(AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_GET,
                &input_para, sizeof(input_para),
                &service_result, sizeof(service_result), ret);
    if (service_result.ret != 0) {
      ret = AM_REST_RESULT_ERR_SERVER;
      m_utils->set_response_msg(AM_REST_OVERLAY_HANDLE_ERR,
                                "server get overlay area parameters failed");
      break;
    }
    am_overlay_area_t area = {0};
    memcpy(&area, service_result.data, sizeof(am_overlay_area_t));

    m_utils->set_response_data("stream_id", stream_id);
    m_utils->set_response_data("area_id", area_id);
    char state[MAX_PARA_LENGHT];
    snprintf(state, MAX_PARA_LENGHT, "%s", (area.enable == 1) ? "enable" :
        (area.enable == 0) ? "disable" : "not created");
    m_utils->set_response_data("state", string(state));
    if (0 == area.enable || 1 == area.enable) {
      m_utils->set_response_data("position_x", int32_t(area.offset_x));
      m_utils->set_response_data("position_y", int32_t(area.offset_y));
      m_utils->set_response_data("width", int32_t(area.width));
      m_utils->set_response_data("height", int32_t(area.height));
      m_utils->set_response_data("rotate", area.rotate ? true : false);
      m_utils->set_response_data("bg_color", double(area.bg_color));
      m_utils->set_response_data("buf_num", int32_t(area.buf_num));
      m_utils->set_response_data("data_block_num", int32_t(area.num));
    }
    int32_t i = 0;
    for (; i < int32_t(area.num); ++i) {
      if (overlay_get_data_params(stream_id, area_id, i) != AM_REST_RESULT_OK) {
        break;
      }
      m_utils->save_tmp_obj_to_local_arr();
    }
    if (i > 0) {
      m_utils->set_local_arr_to_response("data");
    }
  } while(0);

  return ret;
}

AM_REST_RESULT AMRestAPIVideo::overlay_get_data_params(int32_t stream_id,
                                                       int32_t area_id,
                                                       int32_t &data_id)
{
  AM_REST_RESULT ret = AM_REST_RESULT_OK;
  do {
    am_service_result_t service_result;
    am_overlay_id_t input_para = {0};

    input_para.stream_id = stream_id;
    input_para.area_id = area_id;
    input_para.data_index = data_id;
    METHOD_CALL(AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_DATA_GET,
                &input_para, sizeof(input_para),
                &service_result, sizeof(service_result), ret);
    if (service_result.ret != 0) {
      ret = AM_REST_RESULT_ERR_SERVER;
      m_utils->set_response_msg(AM_REST_OVERLAY_HANDLE_ERR,
                                "server get overlay data parameters failed");
      break;
    }
    am_overlay_data_t area = {0};
    memcpy(&area, service_result.data, sizeof(am_overlay_data_t));

    if (area.type == 404) {
      m_utils->create_tmp_obj_data("type", int32_t(area.type));
      m_utils->save_tmp_obj_to_local_arr();
      ++data_id;
      continue;
    }
    m_utils->create_tmp_obj_data("type", int32_t(area.type));
    m_utils->create_tmp_obj_data("width", int32_t(area.width));
    m_utils->create_tmp_obj_data("height", int32_t(area.height));
    m_utils->create_tmp_obj_data("position_x", int32_t(area.offset_x));
    m_utils->create_tmp_obj_data("position_y", int32_t(area.offset_y));
    if (0 == area.type || 2 == area.type) {
      m_utils->create_tmp_obj_data("spacing", int32_t(area.spacing));
      m_utils->create_tmp_obj_data("font_size", int32_t(area.font_size));
      m_utils->create_tmp_obj_data("bg_color", double(area.font_color));
      m_utils->create_tmp_obj_data("font_color", double(area.font_color));
      m_utils->create_tmp_obj_data("font_outwidth",
                                   int32_t(area.font_outline_w));
      m_utils->create_tmp_obj_data("font_outline_color",
                                   double(area.font_outline_color));
      m_utils->create_tmp_obj_data("font_horbold", int32_t(area.font_hor_bold));
      m_utils->create_tmp_obj_data("font_verbold", int32_t(area.font_ver_bold));
      m_utils->create_tmp_obj_data("font_italic", int32_t(area.font_italic));
      if (2 == area.type) {
        m_utils->create_tmp_obj_data("en_msec", int32_t(area.msec_en));
      }
    } else if (1 == area.type || 3 == area.type) {
      m_utils->create_tmp_obj_data("color_key", double(area.color_key));
      m_utils->create_tmp_obj_data("color_range", int32_t(area.color_range));
      if (3 == area.type) {
        m_utils->create_tmp_obj_data("bmp_num", int32_t(area.bmp_num));
        m_utils->create_tmp_obj_data("interval", int32_t(area.interval));
      }
    }
    break;
  } while(1);

  return ret;
}

AM_REST_RESULT AMRestAPIVideo::overlay_manipulate_handle(const string &action)
{
  AM_REST_RESULT ret = AM_REST_RESULT_OK;
  do {
    if ("destory" == action) {
      ret = overlay_delete_all_areas();
      break;
    }
    int32_t stream_id;
    int32_t max_area_num = 0;
    if ((ret=overlay_get_max_num(max_area_num)) != AM_REST_RESULT_OK) {
      break;
    }
    if ((ret = get_srteam_id(stream_id, "arg2")) != AM_REST_RESULT_OK){
      break;
    }
    int32_t area_id;
    if ((ret = get_area_id(area_id)) != AM_REST_RESULT_OK) {
      break;
    } else if ( area_id<0 || area_id>max_area_num) {
      ret = AM_REST_RESULT_ERR_URL;
      char msg[MAX_MSG_LENGHT];
      snprintf(msg, MAX_MSG_LENGHT, "invalid area id: %d,overlay area valid"
               " manipulate range: 0~%d", area_id, max_area_num - 1);
      m_utils->set_response_msg(AM_REST_OVERLAY_AREA_ERR, msg);
      break;
    }
    ret = overlay_manipulate_by_area(stream_id, area_id, action);

  } while(0);

  return ret;
}

AM_REST_RESULT  AMRestAPIVideo::overlay_delete_all_areas()
{
  AM_REST_RESULT ret = AM_REST_RESULT_OK;
  do {
    am_service_result_t service_result;
    METHOD_CALL(AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_DESTROY, nullptr, 0,
                &service_result, sizeof(service_result), ret);
    if (service_result.ret){
      ret = AM_REST_RESULT_ERR_SERVER;
      m_utils->set_response_msg(AM_REST_OVERLAY_HANDLE_ERR,
                                "server delete all overlay areas failed");
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
    METHOD_CALL(AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_SAVE, nullptr, 0,
                &service_result, sizeof(service_result), ret);
    if (service_result.ret){
      ret = AM_REST_RESULT_ERR_SERVER;
      m_utils->set_response_msg(AM_REST_OVERLAY_HANDLE_ERR,
                                "server save overlay config failed");
      break;
    }
  } while(0);

  return ret;
}

AM_REST_RESULT AMRestAPIVideo::overlay_parameters_init(am_overlay_data_t &data)
{
  AM_REST_RESULT ret = AM_REST_RESULT_OK;
  string value;
  do {
    int32_t stream_id;
    if ((ret = get_srteam_id(stream_id, "arg2")) != AM_REST_RESULT_OK) {
      break;
    }
    data.id.stream_id = stream_id;

    int32_t max_area_num = 0;
    if ((ret=overlay_get_max_num(max_area_num)) != AM_REST_RESULT_OK) {
      break;
    }
    int32_t area_id;
    if ((ret = get_area_id(area_id)) != AM_REST_RESULT_OK) {
      break;
    } else if ( area_id<0 || area_id>=max_area_num) {
      ret = AM_REST_RESULT_ERR_URL;
      char msg[MAX_MSG_LENGHT];
      snprintf(msg, MAX_MSG_LENGHT, "invalid area id: %d,overlay area valid "
          "area range: 0~%d", area_id,max_area_num);
      m_utils->set_response_msg(AM_REST_OVERLAY_AREA_ERR, msg);
      break;
    }
    data.id.area_id = area_id;

    if (m_utils->get_value("position_x", value) == AM_REST_RESULT_OK) {
      SET_BIT(data.enable_bits, AM_OVERLAY_RECT_EN_BIT);
      data.offset_x = atoi(value.c_str());
    }
    if (m_utils->get_value("position_y", value) == AM_REST_RESULT_OK) {
      SET_BIT(data.enable_bits, AM_OVERLAY_RECT_EN_BIT);
      data.offset_y = atoi(value.c_str());
    }
    if (m_utils->get_value("width", value) == AM_REST_RESULT_OK) {
      SET_BIT(data.enable_bits, AM_OVERLAY_RECT_EN_BIT);
      data.width = atoi(value.c_str());
    }
    if (m_utils->get_value("height", value) == AM_REST_RESULT_OK) {
      SET_BIT(data.enable_bits, AM_OVERLAY_RECT_EN_BIT);
      data.height = atoi(value.c_str());
    }
    if (m_utils->get_value("type", value) == AM_REST_RESULT_OK) {
      SET_BIT(data.enable_bits, AM_OVERLAY_DATA_TYPE_EN_BIT);
      int32_t tmp = atoi(value.c_str());
      if (tmp<0 || tmp>4) {
        ret = AM_REST_RESULT_ERR_PARAM;
        char msg[MAX_MSG_LENGHT];
        snprintf(msg, MAX_MSG_LENGHT, "invalid overlay type: %d",tmp);
        m_utils->set_response_msg(AM_REST_OVERLAY_TYPE_ERR, msg);
        break;
      }
      data.type = tmp;
    }
    if (m_utils->get_value("text_str", value) == AM_REST_RESULT_OK) {
      SET_BIT(data.enable_bits, AM_OVERLAY_STRING_EN_BIT);
      snprintf(data.str, OVERLAY_MAX_STRING, "%s", value.c_str());
    }
    if (m_utils->get_value("pre_str", value) == AM_REST_RESULT_OK) {
      SET_BIT(data.enable_bits, AM_OVERLAY_TIME_EN_BIT);
      snprintf(data.pre_str, OVERLAY_MAX_STRING, "%s", value.c_str());
    }
    if (m_utils->get_value("suf_str", value) == AM_REST_RESULT_OK) {
      SET_BIT(data.enable_bits, AM_OVERLAY_TIME_EN_BIT);
      snprintf(data.suf_str, OVERLAY_MAX_STRING, "%s", value.c_str());
    }
    if (m_utils->get_value("en_msec", value) == AM_REST_RESULT_OK) {
      SET_BIT(data.enable_bits, AM_OVERLAY_TIME_EN_BIT);
      data.msec_en = atoi(value.c_str());
    }
    if (m_utils->get_value("spacing", value) == AM_REST_RESULT_OK) {
      SET_BIT(data.enable_bits, AM_OVERLAY_CHAR_SPACING_EN_BIT);
      data.spacing = atoi(value.c_str());
    }
    if (m_utils->get_value("bg_color", value) == AM_REST_RESULT_OK) {
      SET_BIT(data.enable_bits, AM_OVERLAY_BG_COLOR_EN_BIT);
      data.bg_color = atoi(value.c_str());
    }
    if (m_utils->get_value("font_type", value) == AM_REST_RESULT_OK) {
      SET_BIT(data.enable_bits, AM_OVERLAY_FONT_TYPE_EN_BIT);
      if (value.size() >= OVERLAY_MAX_FILENAME) {
        value[OVERLAY_MAX_FILENAME-1] = '\0';
      }
      if (access(value.c_str(), F_OK) != -1) {
        strncpy(data.font_type, value.c_str(), value.size());
      } else {
        snprintf(data.font_type, OVERLAY_MAX_FILENAME,
                 "/usr/share/fonts/%s", value.c_str());
      }
    }
    if (m_utils->get_value("font_color", value) == AM_REST_RESULT_OK) {
      SET_BIT(data.enable_bits, AM_OVERLAY_FONT_COLOR_EN_BIT);
      data.font_color = (uint32_t)atoll(value.c_str());
    }
    if (m_utils->get_value("font_size", value) == AM_REST_RESULT_OK) {
      SET_BIT(data.enable_bits, AM_OVERLAY_FONT_SIZE_EN_BIT);
      data.font_size = atoi(value.c_str());
    }
    if (m_utils->get_value("font_outwidth", value) == AM_REST_RESULT_OK) {
      SET_BIT(data.enable_bits, AM_OVERLAY_FONT_OUTLINE_EN_BIT);
      data.font_outline_w = atoi(value.c_str());
    }
    if (m_utils->get_value("font_outline_color", value) == AM_REST_RESULT_OK) {
      SET_BIT(data.enable_bits, AM_OVERLAY_FONT_OUTLINE_EN_BIT);
      data.font_outline_color = (uint32_t)atoll(value.c_str());
    }
    if (m_utils->get_value("font_horbold", value) == AM_REST_RESULT_OK) {
      SET_BIT(data.enable_bits, AM_OVERLAY_FONT_BOLD_EN_BIT);
      data.font_hor_bold = atoi(value.c_str());
    }
    if (m_utils->get_value("font_verbold", value) == AM_REST_RESULT_OK) {
      SET_BIT(data.enable_bits, AM_OVERLAY_FONT_BOLD_EN_BIT);
      data.font_ver_bold = atoi(value.c_str());
    }
    if (m_utils->get_value("font_italic", value) == AM_REST_RESULT_OK) {
      SET_BIT(data.enable_bits, AM_OVERLAY_FONT_ITALIC_EN_BIT);
      data.font_italic = atoi(value.c_str());
    }
    if (m_utils->get_value("bmp_file", value) == AM_REST_RESULT_OK) {
      SET_BIT(data.enable_bits, AM_OVERLAY_BMP_EN_BIT);
      if (value.size() >= OVERLAY_MAX_FILENAME) {
        value[OVERLAY_MAX_FILENAME-1] = '\0';
      }
      if (access(value.c_str(), F_OK) != -1) {
        strncpy(data.bmp, value.c_str(), value.size());
      } else {
        snprintf(data.bmp, OVERLAY_MAX_FILENAME,
            "/usr/share/oryx/video/overlay/%s", value.c_str());
      }
    }
    if (m_utils->get_value("bmp_num", value) == AM_REST_RESULT_OK) {
      SET_BIT(data.enable_bits, AM_OVERLAY_ANIMATION_EN_BIT);
      data.bmp_num = atoi(value.c_str());
    }
    if (m_utils->get_value("interval", value) == AM_REST_RESULT_OK) {
      SET_BIT(data.enable_bits, AM_OVERLAY_ANIMATION_EN_BIT);
      data.interval = atoi(value.c_str());
    }
    if (m_utils->get_value("color_key", value) == AM_REST_RESULT_OK) {
      SET_BIT(data.enable_bits, AM_OVERLAY_BMP_COLOR_EN_BIT);
      data.color_key = (uint32_t)atoll(value.c_str());
    }
    if (m_utils->get_value("color_range", value) == AM_REST_RESULT_OK) {
      SET_BIT(data.enable_bits, AM_OVERLAY_BMP_COLOR_EN_BIT);
      data.color_range = atoi(value.c_str());
    }
  } while(0);

  return ret;
}

AM_REST_RESULT AMRestAPIVideo::overlay_manipulate_by_area(int32_t stream_id,
                                                          int32_t area_id,
                                                          const string &action)
{
  AM_REST_RESULT ret = AM_REST_RESULT_OK;
  am_service_result_t service_result;
  do {
    am_overlay_id_s set_cfg = {0};
    set_cfg.stream_id = stream_id;
    set_cfg.area_id = area_id;
    if ("delete" == action) {
      string value;
      if ((ret = m_utils->get_value("arg4", value)) != AM_REST_RESULT_OK) {
        SET_BIT(set_cfg.enable_bits, AM_OVERLAY_REMOVE_EN_BIT);
      } else {
        char tmp[MAX_PARA_LENGHT];
        sscanf(value.c_str(),"block%[0-9]",tmp);
        set_cfg.data_index = atoi(tmp);
        SET_BIT(set_cfg.enable_bits, AM_OVERLAY_DATA_REMOVE_EN_BIT);
      }
    } else if ("enable" == action) {
      SET_BIT(set_cfg.enable_bits, AM_OVERLAY_ENABLE_EN_BIT);
    } else if ("disable" == action) {
      SET_BIT(set_cfg.enable_bits, AM_OVERLAY_DISABLE_EN_BIT);
    } else {
      ret = AM_REST_RESULT_ERR_PARAM;
      char msg[MAX_MSG_LENGHT];
      snprintf(msg, MAX_MSG_LENGHT, "invalid overlay action: %s",
               action.c_str());
      m_utils->set_response_msg(AM_REST_OVERLAY_ACTION_ERR, msg);
      break;
    }

    METHOD_CALL(AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_SET,&set_cfg, sizeof(set_cfg),
                &service_result, sizeof(service_result), ret);
    if (service_result.ret != 0) {
      ret = AM_REST_RESULT_ERR_SERVER;
      char msg[MAX_MSG_LENGHT];
      snprintf(msg, MAX_MSG_LENGHT, "server %s overlay failed", action.c_str());
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

    if ((dir=opendir("/usr/share/fonts/")) == nullptr) {
      break;
    }
    while ((ptr = readdir(dir)) != nullptr) {
      if(0 == strcmp(ptr->d_name, ".") || 0 == strcmp(ptr->d_name,"..")) {
        continue;
      } else if((font_extension = strchr(ptr->d_name, '.')) != nullptr) {
        if (0 == strcmp(font_extension, ".ttf") ||
            0 == strcmp(font_extension, ".ttc")) {
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

    if ((dir=opendir("/usr/share/oryx/video/overlay/")) == nullptr) {
      break;
    }
    while ((ptr = readdir(dir)) != nullptr) {
      if(0 == strcmp(ptr->d_name, ".") || 0 == strcmp(ptr->d_name,"..")) {
        continue;
      } else if((bmp_extension = strchr(ptr->d_name, '.')) != nullptr) {
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
      m_utils->set_response_msg(AM_REST_DPTZ_ACTION_ERR,
                                "no dptz action parameter");
      break;
    }
    if ("set" == action) {
      if ((ret=dptz_set_handle()) != AM_REST_RESULT_OK) {
        break;
      }
    } else if ("get" == action) {
      if ((ret=dptz_get_handle()) != AM_REST_RESULT_OK) {
        break;
      }
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
    bool ratio_flag = false;
    bool size_flag = false;
    am_dptz_ratio_t dptz_ratio = {0};
    am_dptz_size_t dptz_size = {0};
    string value;

    int32_t buffer_id;
    if ((ret = get_buffer_id(buffer_id)) != AM_REST_RESULT_OK) {
      break;
    }
    dptz_size.buffer_id = buffer_id;
    dptz_ratio.buffer_id = buffer_id;

    if (m_utils->get_value("dptz_x", value) == AM_REST_RESULT_OK) {
      SET_BIT(dptz_size.enable_bits, AM_DPTZ_SIZE_X_EN_BIT);
      dptz_size.x = atoi(value.c_str());
      size_flag = true;
    }
    if (m_utils->get_value("dptz_y", value) == AM_REST_RESULT_OK) {
      SET_BIT(dptz_size.enable_bits, AM_DPTZ_SIZE_Y_EN_BIT);
      dptz_size.y = atoi(value.c_str());
      size_flag = true;
    }
    if (m_utils->get_value("dptz_w", value) == AM_REST_RESULT_OK) {
      SET_BIT(dptz_size.enable_bits, AM_DPTZ_SIZE_W_EN_BIT);
      dptz_size.w = atoi(value.c_str());
      size_flag = true;
    }
    if (m_utils->get_value("dptz_h", value) == AM_REST_RESULT_OK) {
      SET_BIT(dptz_size.enable_bits, AM_DPTZ_SIZE_H_EN_BIT);
      dptz_size.h = atoi(value.c_str());
      size_flag = true;
    }

    if (size_flag) {
      METHOD_CALL(AM_IPC_MW_CMD_VIDEO_DYN_DPTZ_SIZE_SET, &dptz_size,
                  sizeof(dptz_size), &service_result,
                  sizeof(service_result), ret);
      if ((service_result.ret) != 0) {
        ret = AM_REST_RESULT_ERR_SERVER;
        m_utils->set_response_msg(AM_REST_DPTZ_HANDLE_ERR,
                                  "server set dptz size failed");
      }
      break;
    }

    if (m_utils->get_value("zoom_ratio", value) == AM_REST_RESULT_OK) {
      float tmp = atof(value.c_str());
      if (tmp<0.0 || tmp>8.0) {
        ret = AM_REST_RESULT_ERR_PARAM;
        char msg[MAX_MSG_LENGHT];
        snprintf(msg, MAX_MSG_LENGHT, "zoom_ratio value: %f is out of "
            "range: [0.0, 8.0]",tmp);
        m_utils->set_response_msg(AM_REST_DPTZ_ZOOM_RATIO_ERR, msg);
        break;
      }
      SET_BIT(dptz_ratio.enable_bits, AM_DPTZ_ZOOM_RATIO_EN_BIT);
      dptz_ratio.zoom_ratio = tmp;
      ratio_flag = true;
    }
    if (m_utils->get_value("pan_ratio", value) == AM_REST_RESULT_OK) {
      float tmp = atof(value.c_str());
      if (tmp<-1.0 || tmp>1.0) {
        ret = AM_REST_RESULT_ERR_PARAM;
        char msg[MAX_MSG_LENGHT];
        snprintf(msg, MAX_MSG_LENGHT, "pan_ratio value: %f is out of "
            "range: [-1.0, 1.0]",tmp);
        m_utils->set_response_msg(AM_REST_DPTZ_PAN_RATIO_ERR, msg);
        break;
      }
      SET_BIT(dptz_ratio.enable_bits, AM_DPTZ_PAN_RATIO_EN_BIT);
      dptz_ratio.pan_ratio = tmp;
      ratio_flag = true;
    }
    if (m_utils->get_value("tilt_ratio", value) == AM_REST_RESULT_OK) {
      float tmp = atof(value.c_str());
      if (tmp<-1.0 || tmp>1.0) {
        ret = AM_REST_RESULT_ERR_PARAM;
        char msg[MAX_MSG_LENGHT];
        snprintf(msg, MAX_MSG_LENGHT, "tilt_ratio value: %f is out of "
            "range: [-1.0, 1.0]",tmp);
        m_utils->set_response_msg(AM_REST_DPTZ_TILT_RATIO_ERR, msg);
        break;
      }
      SET_BIT(dptz_ratio.enable_bits, AM_DPTZ_TILT_RATIO_EN_BIT);
      dptz_ratio.tilt_ratio = tmp;
      ratio_flag = true;
    }

    if (ratio_flag) {
      METHOD_CALL(AM_IPC_MW_CMD_VIDEO_DYN_DPTZ_RATIO_SET, &dptz_ratio,
                  sizeof(dptz_ratio), &service_result,
                  sizeof(service_result), ret);
      if ((service_result.ret) != 0) {
        ret = AM_REST_RESULT_ERR_SERVER;
        m_utils->set_response_msg(AM_REST_DPTZ_HANDLE_ERR,
                                  "server set dptz ratio failed");
      }
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
    am_dptz_ratio_t ratio;
    am_dptz_size_t size;

    int32_t buffer_id;
    if ((ret = get_buffer_id(buffer_id)) != AM_REST_RESULT_OK) {
      break;
    }

    METHOD_CALL(AM_IPC_MW_CMD_VIDEO_DYN_DPTZ_RATIO_GET,
                &buffer_id, sizeof(buffer_id),
                &service_result, sizeof(service_result), ret);
    if (service_result.ret != 0) {
      ret = AM_REST_RESULT_ERR_SERVER;
      m_utils->set_response_msg(AM_REST_DPTZ_HANDLE_ERR,
                                "server get dptz ratio failed");
      break;
    }
    memcpy(&ratio, service_result.data, sizeof(am_dptz_ratio_t));

    memset(&service_result, 0, sizeof(service_result));
    METHOD_CALL(AM_IPC_MW_CMD_VIDEO_DYN_DPTZ_SIZE_GET,
                &buffer_id, sizeof(buffer_id),
                &service_result, sizeof(service_result), ret);
    if (service_result.ret != 0) {
      ret = AM_REST_RESULT_ERR_SERVER;
      m_utils->set_response_msg(AM_REST_DPTZ_HANDLE_ERR,
                                "server get dptz size failed");
      break;
    }
    memcpy(&size, service_result.data, sizeof(am_dptz_size_t));
    m_utils->set_response_data("buffer_id", buffer_id);
    m_utils->set_response_data("zoom_ratio", ratio.zoom_ratio);
    m_utils->set_response_data("pan_ratio", ratio.pan_ratio);
    m_utils->set_response_data("tilt_ratio", ratio.tilt_ratio);
    m_utils->set_response_data("dptz_x", (int32_t)size.x);
    m_utils->set_response_data("dptz_y", (int32_t)size.y);
    m_utils->set_response_data("dptz_w", (int32_t)size.w);
    m_utils->set_response_data("dptz_h", (int32_t)size.h);
  } while(0);

  return ret;
}

AM_REST_RESULT AMRestAPIVideo::video_dewarp_handle()
{
  AM_REST_RESULT ret = AM_REST_RESULT_OK;
  do {
    string action;
    if ((ret = m_utils->get_value("action", action)) != AM_REST_RESULT_OK) {
      m_utils->set_response_msg(AM_REST_DEWARP_ACTION_ERR,
                                "no dewarp action parameter");
      break;
    }
    if ("set" == action) {
      if ((ret=dewarp_set_handle()) != AM_REST_RESULT_OK) {
        break;
      }
    } else if ("get" == action) {
      if ((ret=dewarp_get_handle()) != AM_REST_RESULT_OK)
      {
        break;
      }
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
    am_warp_t warp_cfg = {0};
    string value;

    if (m_utils->get_value("ldc_mode", value) == AM_REST_RESULT_OK) {
      int32_t tmp = atoi(value.c_str());
      if (tmp < 0 || tmp > 8) {
        ret = AM_REST_RESULT_ERR_PARAM;
        char msg[MAX_MSG_LENGHT];
        snprintf(msg, MAX_MSG_LENGHT, "ldc_mode value: %d is out of range:"
            " [0, 8]",tmp);
        m_utils->set_response_msg(AM_REST_DEWARP_WARP_MODE_ERR, msg);
        break;
      }
      SET_BIT(warp_cfg.enable_bits, AM_WARP_LDC_WARP_MODE_EN_BIT);
      warp_cfg.warp_mode = tmp;
    }
    if (m_utils->get_value("ldc_strength", value) == AM_REST_RESULT_OK) {
      float tmp = atof(value.c_str());
      if (tmp<0.0 || tmp>36.0) {
        ret = AM_REST_RESULT_ERR_PARAM;
        char msg[MAX_MSG_LENGHT];
        snprintf(msg, MAX_MSG_LENGHT, "ldc_strength value: %f is out of range:"
            " [0.0, 36.0]",tmp);
        m_utils->set_response_msg(AM_REST_DEWARP_LDC_STRENGTH_ERR, msg);
        break;
      }
      SET_BIT(warp_cfg.enable_bits, AM_WARP_LDC_STRENGTH_EN_BIT);
      warp_cfg.ldc_strength = tmp;
    }
    if (m_utils->get_value("max_radius", value) == AM_REST_RESULT_OK) {
      int32_t tmp = atoi(value.c_str());
      if (tmp < 0) {
        ret = AM_REST_RESULT_ERR_PARAM;
        char msg[MAX_MSG_LENGHT];
        snprintf(msg, MAX_MSG_LENGHT, "invalid max_radius value: %d",tmp);
        m_utils->set_response_msg(AM_REST_DEWARP_MAX_RADIUS_ERR, msg);
        break;
      }
      SET_BIT(warp_cfg.enable_bits, AM_WARP_MAX_RADIUS_EN_BIT);
      warp_cfg.max_radius = tmp;
    }
    if (m_utils->get_value("pano_hfov_degree", value) == AM_REST_RESULT_OK) {
      float tmp = atof(value.c_str());
      if (tmp<1.0 || tmp>180.0) {
        ret = AM_REST_RESULT_ERR_PARAM;
        char msg[MAX_MSG_LENGHT];
        snprintf(msg, MAX_MSG_LENGHT, "pano_hfov_degree value: %f is out of "
            "range: [1.0, 180.0]",tmp);
        m_utils->set_response_msg(AM_REST_DEWARP_PANO_HFOV_ERR, msg);
        break;
      }
      SET_BIT(warp_cfg.enable_bits, AM_WARP_PANO_HFOV_DEGREE_EN_BIT);
      warp_cfg.pano_hfov_degree = tmp;
    }
    if (m_utils->get_value("warp_region_yaw", value) == AM_REST_RESULT_OK) {
      int32_t tmp = atoi(value.c_str());
      if (tmp < -90 || tmp > 90) {
        ret = AM_REST_RESULT_ERR_PARAM;
        char msg[MAX_MSG_LENGHT];
        snprintf(msg, MAX_MSG_LENGHT, "warp yaw value: %d is out of range:"
            "[-90, 90]", tmp);
        m_utils->set_response_msg(AM_REST_DEWARP_YAW_PITCH_HFOV_ERR, msg);
        break;
      }
      SET_BIT(warp_cfg.enable_bits, AM_WARP_REGION_YAW_PITCH_EN_BIT);
      warp_cfg.warp_region_yaw = tmp;
    }
    if (m_utils->get_value("warp_region_pitch", value) == AM_REST_RESULT_OK) {
      int32_t tmp = atoi(value.c_str());
      if (tmp < -90 || tmp > 90) {
        ret = AM_REST_RESULT_ERR_PARAM;
        char msg[MAX_MSG_LENGHT];
        snprintf(msg, MAX_MSG_LENGHT, "warp pitch value: %d is out of range:"
            "[-90, 90]", tmp);
        m_utils->set_response_msg(AM_REST_DEWARP_YAW_PITCH_HFOV_ERR, msg);
        break;
      }
      SET_BIT(warp_cfg.enable_bits, AM_WARP_REGION_YAW_PITCH_EN_BIT);
      warp_cfg.warp_region_pitch = tmp;
    }
    if (m_utils->get_value("warp_hor_zoom", value) == AM_REST_RESULT_OK) {
      int32_t num = 0, den = 0;
      if ((sscanf(value.c_str(), "%d/%d", &num, &den) == 2)
          && (num > 0) && (den > 0)) {
        SET_BIT(warp_cfg.enable_bits, AM_WARP_HOR_VER_ZOOM_EN_BIT);
        warp_cfg.hor_zoom = (num << 16) | (den & 0xffff);
      } else {
        ret = AM_REST_RESULT_ERR_PARAM;
        char msg[MAX_MSG_LENGHT];
        snprintf(msg, MAX_MSG_LENGHT, "invalid dwarp horizontal zoom ratio "
            "value: %s", value.c_str());
        m_utils->set_response_msg(AM_REST_DEWARP_ZOOM_ERR, msg);
        break;
      }
    }
    if (m_utils->get_value("warp_ver_zoom", value) == AM_REST_RESULT_OK) {
      int32_t num = 0, den = 0;
      if ((sscanf(value.c_str(), "%d/%d", &num, &den) == 2)
          && (num > 0) && (den > 0)) {
        SET_BIT(warp_cfg.enable_bits, AM_WARP_HOR_VER_ZOOM_EN_BIT);
        warp_cfg.ver_zoom = (num << 16) | (den & 0xffff);
      } else {
        ret = AM_REST_RESULT_ERR_PARAM;
        char msg[MAX_MSG_LENGHT];
        snprintf(msg, MAX_MSG_LENGHT, "invalid dwarp vertical zoom ratio "
            "value: %s", value.c_str());
        m_utils->set_response_msg(AM_REST_DEWARP_ZOOM_ERR, msg);
        break;
      }
    }

    METHOD_CALL(AM_IPC_MW_CMD_VIDEO_DYN_WARP_SET,
                &warp_cfg, sizeof(warp_cfg),
                &service_result, sizeof(service_result), ret);
    if ((service_result.ret) != 0) {
      ret = AM_REST_RESULT_ERR_SERVER;
      m_utils->set_response_msg(AM_REST_DEWARP_HANDLE_ERR,
                                "server set dewarp failed");
    }
  } while(0);

  return ret;
}

AM_REST_RESULT AMRestAPIVideo::dewarp_get_handle()
{
  AM_REST_RESULT ret = AM_REST_RESULT_OK;
  do {
    am_service_result_t service_result;
    am_warp_t *cfg = nullptr;

    int32_t buffer_id = 0;
    METHOD_CALL(AM_IPC_MW_CMD_VIDEO_DYN_WARP_GET, &buffer_id, sizeof(buffer_id),
                &service_result, sizeof(service_result), ret);
    if (service_result.ret != 0) {
      ret = AM_REST_RESULT_ERR_SERVER;
      m_utils->set_response_msg(AM_REST_DEWARP_HANDLE_ERR,
                                "server get dewarp failed");
      break;
    }
    cfg = (am_warp_t*)service_result.data;
    char tmp[128];
    m_utils->set_response_data("ldc_mode",cfg->warp_mode);
    m_utils->set_response_data("ldc_strength",cfg->ldc_strength);
    m_utils->set_response_data("max_radius",cfg->max_radius);
    m_utils->set_response_data("pano_hfov_degree",cfg->pano_hfov_degree);
    m_utils->set_response_data("warp_region_yaw",cfg->warp_region_yaw);
    m_utils->set_response_data("warp_region_pitch",cfg->warp_region_pitch);
    snprintf(tmp, 128, "%d/%d", cfg->hor_zoom >> 16, cfg->hor_zoom & 0xffff);
    m_utils->set_response_data("warp_hor_zoom", string(tmp));
    snprintf(tmp, 128, "%d/%d", cfg->ver_zoom >> 16, cfg->ver_zoom & 0xffff);
    m_utils->set_response_data("warp_ver_zoom", string(tmp));
  } while(0);

  return ret;
}

AM_REST_RESULT AMRestAPIVideo::video_enc_ctrl_handle()
{
  AM_REST_RESULT ret = AM_REST_RESULT_OK;
  do {
    string option;
    if ((m_utils->get_value("arg2", option)) != AM_REST_RESULT_OK) {
      ret = AM_REST_RESULT_ERR_URL;
      m_utils->set_response_msg(AM_REST_URL_ARG2_ERR, "no enc_ctrl option");
      break;
    }
    if ("encode" == option) {
      ret = enc_ctrl_encode();
    } else if (!strncmp(option.c_str(), "stream", 6)) {
      char tmp[MAX_PARA_LENGHT];
      sscanf(option.c_str(),"stream%[0-9]",tmp);
      int32_t stream_id = atoi(tmp);
      ret = enc_ctrl_stream(stream_id);
    } else {
      ret = AM_REST_RESULT_ERR_URL;
      char msg[MAX_MSG_LENGHT];
      snprintf(msg, MAX_MSG_LENGHT, "invalid enc_ctrl option action: %s",
               option.c_str());
      m_utils->set_response_msg(AM_REST_URL_ARG2_ERR, "invalid enc_ctrl option");
      break;
    }
  } while(0);

  return ret;
}

AM_REST_RESULT  AMRestAPIVideo::enc_ctrl_encode()
{
  AM_REST_RESULT ret = AM_REST_RESULT_OK;
  do {
    string action;
    if ((ret = m_utils->get_value("action", action)) != AM_REST_RESULT_OK) {
      m_utils->set_response_msg(AM_REST_ENC_CTRL_ACTION_ERR, "no encode "
                                "action, available action:[enable,disable,get]");
      break;
    }
    if ("enable" == action) {
      ret = enc_ctrl_start_encode();
    } else if ("disable" == action) {
      ret = enc_ctrl_stop_encode();
    } else if ("get" == action) {
      ret = enc_ctrl_get_encode_status();
    } else {
      ret = AM_REST_RESULT_ERR_PARAM;
      char msg[MAX_MSG_LENGHT];
      snprintf(msg, MAX_MSG_LENGHT, "invalid encode action: %s",
               action.c_str());
      m_utils->set_response_msg(AM_REST_ENC_CTRL_ACTION_ERR, msg);
      break;
    }
  } while(0);
  return ret;
}

AM_REST_RESULT AMRestAPIVideo::enc_ctrl_start_encode()
{
  AM_REST_RESULT ret = AM_REST_RESULT_OK;
  do {
    am_service_result_t service_result;
    METHOD_CALL(AM_IPC_MW_CMD_VIDEO_ENCODE_START, nullptr, 0,
                &service_result, sizeof(service_result), ret);
    if (service_result.ret != 0) {
      ret = AM_REST_RESULT_ERR_SERVER;
      m_utils->set_response_msg(AM_REST_ENC_CTRL_HANDLE_ERR,
                                "server start encode failed");
      break;
    }
  } while(0);
  return ret;
}

AM_REST_RESULT AMRestAPIVideo::enc_ctrl_stop_encode()
{
  AM_REST_RESULT ret = AM_REST_RESULT_OK;
  do {
    am_service_result_t service_result;
    METHOD_CALL(AM_IPC_MW_CMD_VIDEO_ENCODE_STOP, nullptr, 0,
                &service_result, sizeof(service_result), ret);
    if (service_result.ret != 0) {
      ret = AM_REST_RESULT_ERR_SERVER;
      m_utils->set_response_msg(AM_REST_ENC_CTRL_HANDLE_ERR,
                                "server stop encode failed");
      break;
    }
  } while(0);
  return ret;
}

AM_REST_RESULT  AMRestAPIVideo::enc_ctrl_get_encode_status()
{
  AM_REST_RESULT ret = AM_REST_RESULT_OK;
  do {
    am_service_result_t service_result;
    am_stream_status_t status = {0};
    METHOD_CALL(AM_IPC_MW_CMD_VIDEO_DYN_STREAM_STATUS_GET, nullptr, 0,
                &service_result, sizeof(service_result), ret);
    if (service_result.ret != 0) {
      ret = AM_REST_RESULT_ERR_SERVER;
      m_utils->set_response_msg(AM_REST_ENC_CTRL_HANDLE_ERR,
                                "server get encode status failed");
      break;
    }
    memcpy(&status, service_result.data, sizeof(am_stream_status_t));
    int32_t max = 0;
    if ((ret=get_stream_max_num(max)) != AM_REST_RESULT_OK) {
      break;
    }
    for (int32_t i = 0; i < max; ++i) {
      m_utils->set_response_data(bool(TEST_BIT(status.status, i)));
    }
  } while(0);

  return ret;
}

AM_REST_RESULT  AMRestAPIVideo::enc_ctrl_stream(int32_t stream_id)
{
  AM_REST_RESULT ret = AM_REST_RESULT_OK;
  do {
    int32_t max = 0;
    if ((ret=get_stream_max_num(max)) != AM_REST_RESULT_OK) {
      break;
    }
    if (stream_id >= max) {
      ret = AM_REST_RESULT_ERR_PARAM;
      char msg[MAX_MSG_LENGHT];
      snprintf(msg, MAX_MSG_LENGHT, "invalid video stream id: %d", stream_id);
      m_utils->set_response_msg(AM_REST_ENC_CTRL_STREAM_ERR, msg);
      break;
    }
    string option;
    if ((m_utils->get_value("arg3", option) == AM_REST_RESULT_OK) &&
        ("force_idr" == option)) {
      ret = enc_ctrl_force_idr(stream_id);
      break;
    }

    m_utils->get_value("action", option);
    if ("enable" == option) {
      if ((ret = enc_ctrl_start_streaming(stream_id)) != AM_REST_RESULT_OK) {
        break;
      }
    } else if ("disable" == option) {
      if ((ret = enc_ctrl_stop_streaming(stream_id)) != AM_REST_RESULT_OK) {
        break;
      }
    } else if ("set" == option) {
      if ((ret = enc_ctrl_stream_format(stream_id)) != AM_REST_RESULT_OK) {
        break;
      }
      if ((ret = enc_ctrl_stream_format_dyn(stream_id)) != AM_REST_RESULT_OK) {
        break;
      }
    } else if ("get" == option) {
      if ((ret = enc_ctrl_stream_info_get(stream_id)) != AM_REST_RESULT_OK) {
        break;
      }
    } else {
      ret = AM_REST_RESULT_ERR_PARAM;
      char msg[MAX_MSG_LENGHT];
      snprintf(msg, MAX_MSG_LENGHT, "invalid video stream action: %s",
               option.c_str());
      m_utils->set_response_msg(AM_REST_ENC_CTRL_ACTION_ERR, msg);
    }
  } while(0);

  return ret;
}

AM_REST_RESULT AMRestAPIVideo::enc_ctrl_start_streaming(int32_t stream_id)
{
  AM_REST_RESULT ret = AM_REST_RESULT_OK;
  do {
    am_service_result_t service_result;
    METHOD_CALL(AM_IPC_MW_CMD_VIDEO_DYN_STREAM_START,
                &stream_id, sizeof(int32_t),
                &service_result, sizeof(service_result), ret);
    if (service_result.ret != 0) {
      ret = AM_REST_RESULT_ERR_SERVER;
      m_utils->set_response_msg(AM_REST_ENC_CTRL_HANDLE_ERR,
                                "server start stream failed");
      break;
    }
  } while(0);
  return ret;
}

AM_REST_RESULT AMRestAPIVideo::enc_ctrl_stop_streaming(int32_t stream_id)
{
  AM_REST_RESULT ret = AM_REST_RESULT_OK;
  do {
    am_service_result_t service_result;
    METHOD_CALL(AM_IPC_MW_CMD_VIDEO_DYN_STREAM_STOP,
                &stream_id, sizeof(int32_t),
                &service_result, sizeof(service_result), ret);
    if (service_result.ret != 0) {
      ret = AM_REST_RESULT_ERR_SERVER;
      m_utils->set_response_msg(AM_REST_ENC_CTRL_HANDLE_ERR,
                                "server stop stream failed");
      break;
    }
  } while(0);
  return ret;
}

AM_REST_RESULT  AMRestAPIVideo::enc_ctrl_force_idr(int32_t stream_id)
{
  AM_REST_RESULT ret = AM_REST_RESULT_OK;
  do {
    am_service_result_t service_result;
    METHOD_CALL(AM_IPC_MW_CMD_VIDEO_DYN_STREAM_FORCE_IDR,
                &stream_id, sizeof(int32_t),
                &service_result, sizeof(service_result), ret);
    if (service_result.ret != 0) {
      ret = AM_REST_RESULT_ERR_SERVER;
      m_utils->set_response_msg(AM_REST_ENC_CTRL_HANDLE_ERR,
                                "server set force_idr failed");
      break;
    }
  } while(0);

  return ret;
}

AM_REST_RESULT  AMRestAPIVideo::enc_ctrl_stream_format(int32_t stream_id)
{
  AM_REST_RESULT ret = AM_REST_RESULT_OK;
  do {
    am_service_result_t service_result;
    am_stream_fmt_t stream_fmt = {0};
    am_buffer_fmt_t buffer_fmt = {0};
    string value;
    bool  has_setting = false;
    bool  buffer_setting = false;

    stream_fmt.stream_id = stream_id;

    am_stream_fmt_t fmt = {0};
    METHOD_CALL(AM_IPC_MW_CMD_VIDEO_CFG_STREAM_FMT_GET,
                &stream_id, sizeof(stream_id),
                &service_result, sizeof(service_result), ret);
    memcpy(&fmt, service_result.data, sizeof(fmt));
    buffer_fmt.buffer_id = fmt.source;

    if (m_utils->get_value("source", value) == AM_REST_RESULT_OK) {
      SET_BIT(stream_fmt.enable_bits, AM_STREAM_FMT_SOURCE_EN_BIT);
      stream_fmt.source = atoi(value.c_str());
      has_setting = true;
    }
    if (m_utils->get_value("flip_h", value) == AM_REST_RESULT_OK) {
      SET_BIT(stream_fmt.enable_bits, AM_STREAM_FMT_HFLIP_EN_BIT);
      stream_fmt.hflip = atoi(value.c_str());
      has_setting = true;
    }
    if (m_utils->get_value("flip_v", value) == AM_REST_RESULT_OK) {
      SET_BIT(stream_fmt.enable_bits, AM_STREAM_FMT_VFLIP_EN_BIT);
      stream_fmt.vflip = atoi(value.c_str());
      has_setting = true;
    }
    if (m_utils->get_value("rotate", value) == AM_REST_RESULT_OK) {
      SET_BIT(stream_fmt.enable_bits, AM_STREAM_FMT_ROTATE_EN_BIT);
      stream_fmt.rotate = atoi(value.c_str());
      has_setting = true;
    }

    if (has_setting) {
      if (buffer_setting) {
        METHOD_CALL(AM_IPC_MW_CMD_VIDEO_CFG_BUFFER_SET,
                    &buffer_fmt, sizeof(buffer_fmt),
                    &service_result, sizeof(service_result), ret);
        if (service_result.ret != 0) {
          ret = AM_REST_RESULT_ERR_SERVER;
          m_utils->set_response_msg(AM_REST_ENC_CTRL_HANDLE_ERR,
                                    "server set stream format failed");
          break;
        }
      }
      METHOD_CALL(AM_IPC_MW_CMD_VIDEO_CFG_STREAM_FMT_SET,
                  &stream_fmt, sizeof(stream_fmt),
                  &service_result, sizeof(service_result), ret);
      if (service_result.ret != 0) {
        ret = AM_REST_RESULT_ERR_SERVER;
        m_utils->set_response_msg(AM_REST_ENC_CTRL_HANDLE_ERR,
                                  "server set stream format failed");
        break;
      }

      if ((ret=enc_ctrl_stop_encode()) != AM_REST_RESULT_OK) {
        break;
      }
      if ((ret=video_load_all_cfg()) != AM_REST_RESULT_OK) {
        break;
      }
      if ((ret=enc_ctrl_start_encode()) != AM_REST_RESULT_OK) {
        break;
      }
    }
  } while(0);

  return ret;
}

AM_REST_RESULT  AMRestAPIVideo::enc_ctrl_stream_format_dyn(int32_t stream_id)
{
  AM_REST_RESULT ret = AM_REST_RESULT_OK;
  do {
    am_service_result_t service_result;
    string value, value2;
    bool has_setting = false;
    bool restart_flag = false;
    am_stream_parameter_t params = {0};
    params.stream_id = stream_id;

    if (m_utils->get_value("type", value) == AM_REST_RESULT_OK) {
      SET_BIT(params.enable_bits, AM_STREAM_DYN_CTRL_TYPE_EN_BIT);
      params.type = atoi(value.c_str());
      has_setting = true;
      restart_flag = true;
    }

    if (m_utils->get_value("framerate", value) == AM_REST_RESULT_OK) {
      SET_BIT(params.enable_bits, AM_STREAM_DYN_CTRL_FRAMERATE_EN_BIT);
      params.fps = atoi(value.c_str());
      has_setting = true;
    }

    if (m_utils->get_value("bitrate", value) == AM_REST_RESULT_OK) {
      SET_BIT(params.enable_bits, AM_STREAM_DYN_CTRL_BITRATE_EN_BIT);
      params.bitrate = atoi(value.c_str());
      has_setting = true;
    }

    if (m_utils->get_value("crop_x", value) == AM_REST_RESULT_OK) {
      SET_BIT(params.enable_bits, AM_STREAM_DYN_CTRL_OFFSET_EN_BIT);
      params.offset_x = atoi(value.c_str());
      has_setting = true;
    }

    if (m_utils->get_value("crop_y", value) == AM_REST_RESULT_OK) {
      SET_BIT(params.enable_bits, AM_STREAM_DYN_CTRL_OFFSET_EN_BIT);
      params.offset_y = atoi(value.c_str());
      has_setting = true;
    }

    if (m_utils->get_value("gop_n", value) == AM_REST_RESULT_OK) {
      SET_BIT(params.enable_bits, AM_STREAM_DYN_CTRL_GOP_N_EN_BIT);
      params.gop_n = atoi(value.c_str());
      has_setting = true;
    }

    if (m_utils->get_value("gop_idr", value) == AM_REST_RESULT_OK) {
      SET_BIT(params.enable_bits, AM_STREAM_DYN_CTRL_GOP_IDR_EN_BIT);
      params.idr_interval = atoi(value.c_str());
      has_setting = true;
    }

    if (m_utils->get_value("quality", value) == AM_REST_RESULT_OK) {
      SET_BIT(params.enable_bits, AM_STREAM_DYN_CTRL_ENC_QUALITY_EN_BIT);
      params.quality = atoi(value.c_str());
      has_setting = true;
    }

    if ((m_utils->get_value("width", value) == AM_REST_RESULT_OK) &&
        (m_utils->get_value("height", value2) == AM_REST_RESULT_OK)) {
      am_stream_fmt_t fmt = {0};
      am_buffer_fmt_t buffer = {0};

      SET_BIT(params.enable_bits, AM_STREAM_DYN_CTRL_SIZE_EN_BIT);
      params.width = ROUND_UP(atoi(value.c_str()), 16);
      params.height = ROUND_UP(atoi(value2.c_str()), 8);

      METHOD_CALL(AM_IPC_MW_CMD_VIDEO_CFG_STREAM_FMT_GET,
                  &(params.stream_id), sizeof(params.stream_id),
                  &service_result, sizeof(service_result), ret);
      if (service_result.ret != 0) {
        ret = AM_REST_RESULT_ERR_SERVER;
        m_utils->set_response_msg(AM_REST_ENC_CTRL_HANDLE_ERR,
                                  "server get stream format failed");
        break;
      }
      memcpy(&fmt, service_result.data, sizeof(fmt));

      if (fmt.source == AM_SOURCE_BUFFER_MAIN) {
        ret = AM_REST_RESULT_ERR_SERVER;
        m_utils->set_response_msg(AM_REST_ENC_CTRL_HANDLE_ERR,
                                  "Don't support change main buffer's size");
        break;
      }

      if ((ret=enc_ctrl_stop_streaming(stream_id)) != 0) {
        break;
      }
      restart_flag = true;

      METHOD_CALL(AM_IPC_MW_CMD_VIDEO_DYN_BUFFER_FMT_GET,
                  &fmt.source, sizeof(uint32_t),
                  &service_result, sizeof(service_result), ret);
      if (service_result.ret != 0) {
        ret = AM_REST_RESULT_ERR_SERVER;
        m_utils->set_response_msg(AM_REST_ENC_CTRL_HANDLE_ERR,
                                  "server get buffer format failed");
        break;
      }
      memcpy(&buffer, service_result.data, sizeof(buffer));

      uint32_t buffer_w, buffer_h;
      uint32_t buffer_input_w = buffer.input_width;
      uint32_t buffer_input_h = buffer.input_height;
      if ((m_utils->get_value("keep_full_fov", value) == AM_REST_RESULT_OK) &&
          (atoi(value.c_str()) != 0)) {
        buffer_w = params.width;
        buffer_h = params.height;
        params.offset_x = 0;
        params.offset_y = 0;
      } else {
        uint32_t source_buf_input_gcd = get_gcd(buffer_input_w, buffer_input_h);
        uint32_t stream_size_gcd = get_gcd(params.width, params.height);

        if (((buffer_input_w / source_buf_input_gcd) ==
            (params.width / stream_size_gcd)) &&
            ((buffer_input_h / source_buf_input_gcd) ==
                (params.height / stream_size_gcd))) {
          buffer_w = params.width;
          buffer_h = params.height;
          params.offset_x = 0;
          params.offset_y = 0;
        } else if ((float(buffer_input_w) / buffer_input_h) >
        (float(params.width) / params.height)) {
          //crop stream horizontal content
          buffer_h = params.height;
          buffer_w = (buffer_h * buffer_input_w) / buffer_input_h;
          buffer_w = ROUND_UP(buffer_w, 16);
          params.offset_x = (buffer_w - params.width) / 2;
        } else {
          //crop stream vertical content
          buffer_w = params.width;
          buffer_h = (buffer_w * buffer_input_h) / buffer_input_w;
          buffer_w = ROUND_UP(buffer_h, 8);
          params.offset_y = (buffer_h - params.height) / 2;
        }
      }

      buffer.buffer_id = fmt.source;
      SET_BIT(buffer.enable_bits, AM_BUFFER_FMT_WIDTH_EN_BIT);
      SET_BIT(buffer.enable_bits, AM_BUFFER_FMT_HEIGHT_EN_BIT);
      buffer.width = buffer_w;
      buffer.height = buffer_h;

      METHOD_CALL(AM_IPC_MW_CMD_VIDEO_DYN_BUFFER_FMT_SET,
                  &buffer, sizeof(buffer),
                  &service_result, sizeof(service_result), ret);
      if (service_result.ret != 0) {
        ret = AM_REST_RESULT_ERR_SERVER;
        m_utils->set_response_msg(AM_REST_ENC_CTRL_HANDLE_ERR,
                                  "server set buffer format failed");
        break;
      }

      SET_BIT(params.enable_bits, AM_STREAM_DYN_CTRL_OFFSET_EN_BIT);
      params.offset_x = ROUND_DOWN(params.offset_x, 2);
      params.offset_y = ROUND_DOWN(params.offset_y, 2);
      has_setting = true;
    }

    if (has_setting) {
      METHOD_CALL(AM_IPC_MW_CMD_VIDEO_DYN_STREAM_PARAMETERS_SET,
                  &params, sizeof(params),
                  &service_result, sizeof(service_result), ret);
      if (service_result.ret != 0) {
        ret = AM_REST_RESULT_ERR_SERVER;
        m_utils->set_response_msg(AM_REST_ENC_CTRL_HANDLE_ERR,
                                  "server set stream parameters failed");
        break;
      }
      if (restart_flag) {
        //restart stream to apply stream type or resolution parameter
        if (((ret=enc_ctrl_stop_streaming(stream_id)) != 0) ||
            ((ret=enc_ctrl_start_streaming(stream_id)) != 0)) {
          break;
        }
      }
    }

  } while(0);

  return ret;
}

AM_REST_RESULT  AMRestAPIVideo::enc_ctrl_stream_info_get(int32_t stream_id)
{
  AM_REST_RESULT ret = AM_REST_RESULT_OK;
  do {
    am_service_result_t service_result;
    string value;

    METHOD_CALL(AM_IPC_MW_CMD_VIDEO_DYN_STREAM_PARAMETERS_GET,
                &stream_id, sizeof(stream_id),
                &service_result, sizeof(service_result), ret);
    if (service_result.ret != 0) {
      ret = AM_REST_RESULT_ERR_SERVER;
      m_utils->set_response_msg(AM_REST_ENC_CTRL_HANDLE_ERR,
                                "server get stream info failed");
      break;
    }
    am_stream_parameter_t info = {0};
    memcpy(&info, service_result.data, sizeof(am_stream_parameter_t));
    m_utils->set_response_data("encode_type", int32_t(info.type));
    m_utils->set_response_data("width", int32_t(info.width));
    m_utils->set_response_data("height", int32_t(info.height));
    m_utils->set_response_data("framerate", int32_t(info.fps));
    if ((info.type == 1) || (info.type == 2)) {
      m_utils->set_response_data("bitrate", int32_t(info.bitrate));
      m_utils->set_response_data("gop_n", int32_t(info.gop_n));
      m_utils->set_response_data("gop_idr", int32_t(info.idr_interval));
    } else if (info.type == 3) {
      m_utils->set_response_data("quality", int32_t(info.quality));
    }
  } while(0);

  return ret;
}

AM_REST_RESULT  AMRestAPIVideo::get_stream_max_num(int32_t &value)
{
  AM_REST_RESULT ret = AM_REST_RESULT_OK;
  do {
    am_service_result_t service_result;
    METHOD_CALL(AM_IPC_MW_CMD_VIDEO_DYN_STREAM_MAX_NUM_GET, nullptr, 0,
                &service_result, sizeof(service_result), ret);
    if (ret != AM_REST_RESULT_OK || service_result.ret != 0) {
      value = 0;
      break;
    }
    value = (uint32_t)*service_result.data;
  } while(0);

  return ret;
}

AM_REST_RESULT  AMRestAPIVideo::get_buffer_max_num(int32_t &value)
{
  AM_REST_RESULT ret = AM_REST_RESULT_OK;
  do {
    am_service_result_t service_result;
    METHOD_CALL(AM_IPC_MW_CMD_VIDEO_DYN_BUFFER_MAX_NUM_GET, nullptr, 0,
                &service_result, sizeof(service_result), ret);
    if (ret != AM_REST_RESULT_OK || service_result.ret != 0) {
      value = 0;
      break;
    }
    value = (uint32_t)*service_result.data;
  } while(0);

  return ret;
}

AM_REST_RESULT  AMRestAPIVideo::get_srteam_id(int32_t &stream_id,
                                              const std::string &arg_name)
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
      if (!strncmp(value.c_str(), "stream", 6) && (value.size() == 7) &&
          (isdigit(value[6]))) {
        char tmp[MAX_PARA_LENGHT];
        sscanf(value.c_str(),"stream%[0-9]",tmp);
        stream_id = atoi(tmp);
      } else {
        ret = AM_REST_RESULT_ERR_URL;
        char msg[MAX_MSG_LENGHT];
        snprintf(msg, MAX_MSG_LENGHT, "invalid stream id format: %s",
                 value.c_str());
        m_utils->set_response_msg(err_code, msg);
        break;
      }
    }

    int32_t  s_num_max = 0;
    if ((ret=get_stream_max_num(s_num_max)) != AM_REST_RESULT_OK) {
      break;
    }
    if (stream_id<0 || stream_id>=s_num_max) {
      ret = AM_REST_RESULT_ERR_URL;
      char msg[MAX_MSG_LENGHT];
      snprintf(msg, MAX_MSG_LENGHT, "invalid stream id: %d, stream max id: %d",
               stream_id, s_num_max-1);
      m_utils->set_response_msg(err_code, msg);
      break;
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
      if (!strncmp(value.c_str(), "area", 4) && (value.size() == 5) &&
          (isdigit(value[4]))) {
        char tmp[MAX_PARA_LENGHT];
        sscanf(value.c_str(),"area%[0-9]",tmp);
        area_id = atoi(tmp);
      } else {
        ret = AM_REST_RESULT_ERR_URL;
        char msg[MAX_MSG_LENGHT];
        snprintf(msg, MAX_MSG_LENGHT, "invalid area id format: %s",
                 value.c_str());
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
      if (!strncmp(value.c_str(), "buffer", 6) && (value.size() == 7) &&
          (isdigit(value[6]))) {
        char tmp[MAX_PARA_LENGHT];
        sscanf(value.c_str(),"buffer%[0-9]",tmp);
        buffer_id = atoi(tmp);
      } else {
        ret = AM_REST_RESULT_ERR_URL;
        char msg[MAX_MSG_LENGHT];
        snprintf(msg, MAX_MSG_LENGHT, "invalid buffer id format: %s",
                 value.c_str());
        m_utils->set_response_msg(AM_REST_URL_ARG2_ERR, msg);
        break;
      }
    }
    int32_t b_num_max = 0;
    if ((ret=get_buffer_max_num(b_num_max)) != AM_REST_RESULT_OK) {
      break;
    }
    if (buffer_id<0 || buffer_id>=b_num_max) {
      ret = AM_REST_RESULT_ERR_URL;
      char msg[MAX_MSG_LENGHT];
      snprintf(msg, MAX_MSG_LENGHT, "invalid buffer id: %d, buffer max"
               "id: %d", buffer_id, b_num_max-1);
      m_utils->set_response_msg(AM_REST_URL_ARG2_ERR, msg);
      break;
    }
  } while(0);

  return ret;
}
