/**
 * am_rest_api_system.cpp

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

#include "am_rest_api_system.h"
#include "am_define.h"
#include "config.h"

using std::string;

AM_REST_RESULT AMRestAPISystem::rest_api_handle()
{
  AM_REST_RESULT ret = AM_REST_RESULT_OK;
  do {
    string cmd_group;
    if ((m_utils->get_value("arg1", cmd_group)) != AM_REST_RESULT_OK) {
      ret = AM_REST_RESULT_ERR_URL;
      m_utils->set_response_msg(AM_REST_URL_ARG1_ERR,
                                "no system cmp group parameter");
      break;
    }
    if ("sdcard" == cmd_group) {
      ret = system_sdcard_handle();
    } else {
      ret = AM_REST_RESULT_ERR_URL;
      char msg[MAX_MSG_LENGHT];
      snprintf(msg, MAX_MSG_LENGHT, "invalid system cmp group: %s",
               cmd_group.c_str());
      m_utils->set_response_msg(AM_REST_URL_ARG1_ERR, msg);
    }
  } while(0);
  return ret;
}

AM_REST_RESULT AMRestAPISystem::system_sdcard_handle()
{
  AM_REST_RESULT ret = AM_REST_RESULT_OK;
  string action;
  do {
      string sd_op_type;
      m_utils->get_value("arg2", sd_op_type);

      if ("info" == sd_op_type) {
        ret = system_sdcard_info_handle();
      } else if ("mode" == sd_op_type) {
        ret = system_sdcard_mode_handle();
      } else {
        ret = AM_REST_RESULT_ERR_URL;
        char msg[MAX_MSG_LENGHT];
        snprintf(msg, MAX_MSG_LENGHT, "invalid sdcard operate type: %s",
                 sd_op_type.c_str());
        m_utils->set_response_msg(AM_REST_SD_OP_TYPE_ERR, msg);
      }
  } while(0);

  return ret;
}

#ifdef BUILD_AMBARELLA_ORYX_WEB_SONY_SD
#include "sonysd.h"

AM_REST_RESULT  AMRestAPISystem::system_sdcard_info_handle()
{
  AM_REST_RESULT ret = AM_REST_RESULT_OK;
  do {
    int32_t result = -1;
    string info_type;
    string state;
    m_utils->get_value("arg3", info_type);

    if ("life" == info_type) {
      sonysd_info info = {0};

      result = sonysd_get_info(&info);
      if (result == SONYSD_SUCCESS) {
        //modify it if needed
        int32_t write_rate = 10;//Mbps
        int32_t life_percent = 0;
        int32_t days = int32_t((double(info.life_information_den -
                                       info.life_information_num)
            / (write_rate * 1024 * 1024) / (60 * 60 *24)) *
                               (info.data_size_per_unit * 512 * 8));
        life_percent = int32_t(100.0 * (double(info.life_information_num) /
                double(info.life_information_den)));
        if (info.spare_block_rate >= 100) {
          state = "Has Reached its Lifetime";
          life_percent = 100;
          days = 0;
        } else if (info.life_information_den / 10 * 9 <=
            info.life_information_num) {
          state = "Replacement Recommended";
        } else {
          PRINTF("Normally Functioning\n");
          state = "Normally Functioning";
        }
        m_utils->set_response_data("state", state);
        m_utils->set_response_data("life_percent", life_percent);
        m_utils->set_response_data("life_information_num",
                                   int64_t(info.life_information_num));
        m_utils->set_response_data("life_information_den",
                                   int64_t(info.life_information_den));
        m_utils->set_response_data("data_size_per_unit",
                                   int32_t(info.data_size_per_unit));
        m_utils->set_response_data("usable_working_days", days);
        m_utils->set_response_data("spare_block_rate",
                                   int32_t(info.spare_block_rate));
        m_utils->set_response_data("num_of_sudden_power_failure",
                                   int32_t(info.num_of_sudden_power_failure));
        m_utils->set_response_data("operation_mode",
                                   int32_t(info.operation_mode));
        break;
      } else if(result == SONYSD_ERR_NOCARD) {
        state = "Card Not Inserted";
      } else if(result == SONYSD_ERR_UNSUPPORT) {
        state = "Lifetime Notification Unsupported";
      }
      else {
        state = "Failed to Get Status";
      }

      ret = AM_REST_RESULT_ERR_SERVER;
      m_utils->set_response_msg(AM_REST_SD_HANDLE_ERR, state.c_str());
      break;
    } else if ("err" == info_type) {
      sonysd_errlog log;
      result = sonysd_get_errlog(&log);
      if(result == SONYSD_SUCCESS) {
        m_utils->set_response_data("error_count", log.count);
        for(int32_t i = log.count - 1; i >= 0 ; --i) {
          for(uint32_t j = 0; j < sizeof(log.items[0].command) /
          sizeof(log.items[0].command[0]); ++j) {
            m_utils->create_tmp_obj_data("cmd", log.items[i].command[j].cmd);
            m_utils->create_tmp_obj_data("sector_addr",
                                         int64_t(log.items[i].command[j].addr));
            m_utils->create_tmp_obj_data("resp",
                                         int64_t(log.items[i].command[j].resp));
            m_utils->move_tmp_obj_to_tmp_arr();
          }
          m_utils->create_tmp_obj_data("error_no", log.items[i].error_no);
          m_utils->create_tmp_obj_data("error_type", log.items[i].error_type);
          m_utils->create_tmp_obj_data("block_type", log.items[i].block_type);
          m_utils->create_tmp_obj_data("logical_CE", log.items[i].logical_CE);
          m_utils->create_tmp_obj_data("physical_plane",
                                       log.items[i].physical_plane);
          m_utils->create_tmp_obj_data("physical_block",
                                       log.items[i].physical_block);
          m_utils->create_tmp_obj_data("physical_page",
                                       log.items[i].physical_page);
          m_utils->create_tmp_obj_data("error_type_of_timeout",
                                       log.items[i].error_type_of_timeout);
          m_utils->create_tmp_obj_data("erase_block_count",
                                       int32_t(log.items[i].erase_block_count));
          m_utils->create_tmp_obj_data("write_page_error_count",
                                       log.items[i].write_page_error_count);
          m_utils->create_tmp_obj_data("read_page_error_count",
                                       log.items[i].read_page_error_count);
          m_utils->create_tmp_obj_data("erase_block_error_count",
                                       log.items[i].erase_block_error_count);
          m_utils->create_tmp_obj_data("timeout_count",
                                       log.items[i].timeout_count);
          m_utils->create_tmp_obj_data("crc_error_count",
                                       log.items[i].crc_error_count);
          m_utils->move_tmp_arr_to_tmp_obj("history");
          m_utils->save_tmp_obj_to_local_arr();
        }

        m_utils->set_local_arr_to_response("err");
        break;
      } else if (result == SONYSD_ERR_NOCARD) {
        state = "Card Not Inserted";
      } else if (result == SONYSD_ERR_UNSUPPORT) {
        state = "Lifetime Notification Unsupported";
      } else {
        state = "Failed to Get Status";
      }
      ret = AM_REST_RESULT_ERR_SERVER;
      m_utils->set_response_msg(AM_REST_SD_HANDLE_ERR, state.c_str());
      break;
    } else {
      ret = AM_REST_RESULT_ERR_PARAM;
      char msg[MAX_MSG_LENGHT];
      snprintf(msg, MAX_MSG_LENGHT, "invalid sdcard info type: %s",
               info_type.c_str());
      m_utils->set_response_msg(AM_REST_SD_INFO_TYPE_ERR, msg);
      break;
    }

  } while(0);

  return ret;
}

AM_REST_RESULT  AMRestAPISystem::system_sdcard_mode_handle()
{
  AM_REST_RESULT ret = AM_REST_RESULT_OK;
  do {
    int32_t result = -1;
    string mode;
    string state;
    m_utils->get_value("arg3", mode);

    if ("on" == mode || "off" == mode) {
      int32_t m = (mode == "on") ? 1 : 0;
      result = sonysd_set_operation_mode(m);

      if(result == SONYSD_SUCCESS) {
        break;
      } else if (result == SONYSD_ERR_NOCARD) {
        state = "Card Not Inserted";
      } else if (result == SONYSD_ERR_UNSUPPORT) {
        state = "Lifetime Notification Unsupported";
      } else {
        state = "Failed to Get Status";
      }

      ret = AM_REST_RESULT_ERR_SERVER;
      m_utils->set_response_msg(AM_REST_SD_HANDLE_ERR, state.c_str());
      break;
    } else {
      ret = AM_REST_RESULT_ERR_PARAM;
      char msg[MAX_MSG_LENGHT];
      snprintf(msg, MAX_MSG_LENGHT, "invalid sdcard mode manipulate option: %s",
               mode.c_str());
      m_utils->set_response_msg(AM_REST_SD_INFO_TYPE_ERR, msg);
      break;
    }

  } while(0);

  return ret;
}
#else
AM_REST_RESULT  AMRestAPISystem::system_sdcard_info_handle()
{
  m_utils->set_response_msg(AM_REST_SD_ACTION_ERR,
                            "Server doesn't support that manipulation");
  return AM_REST_RESULT_ERR_SERVER;
}

AM_REST_RESULT  AMRestAPISystem::system_sdcard_mode_handle()
{
  m_utils->set_response_msg(AM_REST_SD_ACTION_ERR,
                            "Server doesn't support that manipulation");
  return AM_REST_RESULT_ERR_SERVER;
}
#endif
