/**
 * am_rest_api_media.cpp
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
#include <limits.h>

#include "am_rest_api_media.h"
#include "am_define.h"

AM_REST_RESULT AMRestAPIMedia::rest_api_handle()
{
  AM_REST_RESULT ret = AM_REST_RESULT_OK;
  do {
    string cmd_group;
    if ((m_utils->get_value("arg1", cmd_group)) != AM_REST_RESULT_OK) {
      ret = AM_REST_RESULT_ERR_URL;
      m_utils->set_response_msg(AM_REST_URL_ARG1_ERR,
                                "no media cmp group parameter");
      break;
    }
    if ("recording" == cmd_group) {
      string recording_type;
      if ((m_utils->get_value("arg2", recording_type)) != AM_REST_RESULT_OK) {
        ret = media_recording_handle();   //no recording type
      } else if ("file" == recording_type) {
        ret = media_file_recording_handle();
      } else if ("event" == recording_type) {
        ret = media_event_recording_handle();
      } else {
        ret = AM_REST_RESULT_ERR_URL;
        char msg[MAX_MSG_LENGHT];
        snprintf(msg, MAX_MSG_LENGHT, "invalid recording type: %s",
                 recording_type.c_str());
        m_utils->set_response_msg(AM_REST_RECORDING_TYPE_ERR, msg);
      }
    } else if ("playback" == cmd_group) {
      string playback_type;
      if ((m_utils->get_value("arg2", playback_type)) != AM_REST_RESULT_OK) {
        ret = AM_REST_RESULT_ERR_URL;
        m_utils->set_response_msg(AM_REST_URL_ARG1_ERR,
                                  "no media playback type parameter");
        break;
      }
      if ("audio" == playback_type) {
        ret = media_audio_playback_handle();
      } else {
        ret = AM_REST_RESULT_ERR_URL;
        char msg[MAX_MSG_LENGHT];
        snprintf(msg, MAX_MSG_LENGHT, "invalid playback type: %s",
                 playback_type.c_str());
        m_utils->set_response_msg(AM_REST_PLAYBACK_TYPE_ERR, msg);
      }
    } else {
      ret = AM_REST_RESULT_ERR_URL;
      char msg[MAX_MSG_LENGHT];
      snprintf(msg, MAX_MSG_LENGHT, "invalid media cmp group: %s",
               cmd_group.c_str());
      m_utils->set_response_msg(AM_REST_URL_ARG1_ERR, msg);
    }
  } while(0);

  return ret;
}

AM_REST_RESULT  AMRestAPIMedia::media_recording_handle()
{
  AM_REST_RESULT ret = AM_REST_RESULT_OK;
  do {
    string action;
    if ((ret = m_utils->get_value("action", action)) != AM_REST_RESULT_OK) {
      m_utils->set_response_msg(AM_REST_RECORDING_ACTION_ERR,
                                "no recording action parameter");
      break;
    }

    am_service_result_t result = {0};
    if ("start" == action) {
      METHOD_CALL(AM_IPC_MW_CMD_MEDIA_START_RECORDING, nullptr,
                  0, &result, sizeof(result), ret);
    } else if ("stop" == action) {
      METHOD_CALL(AM_IPC_MW_CMD_MEDIA_STOP_RECORDING, nullptr,
                  0, &result, sizeof(result), ret);
    } else {
      ret = AM_REST_RESULT_ERR_PARAM;
      char msg[MAX_MSG_LENGHT];
      snprintf(msg, MAX_MSG_LENGHT, "invalid recording action: %s",
               action.c_str());
      m_utils->set_response_msg(AM_REST_RECORDING_ACTION_ERR, msg);
      break;
    }

    if (result.ret){
      ret = AM_REST_RESULT_ERR_SERVER;
      m_utils->set_response_msg(AM_REST_RECORDING_HANDLE_ERR,
                                "server recording failed");
      break;
    }
  } while(0);

  return ret;
}

AM_REST_RESULT  AMRestAPIMedia::media_file_recording_handle()
{
  AM_REST_RESULT ret = AM_REST_RESULT_OK;
  do {
    string action, value;
    if ((ret = m_utils->get_value("action", action)) != AM_REST_RESULT_OK) {
      m_utils->set_response_msg(AM_REST_RECORDING_ACTION_ERR,
                                "no file recording action parameter");
      break;
    }
    if ((ret = m_utils->get_value("muxer_id", value)) != AM_REST_RESULT_OK) {
      m_utils->set_response_msg(AM_REST_RECORDING_ACTION_ERR,
                                "no file recording action parameter");
      break;
    }

    am_service_result_t result = {0};
    uint32_t muxer_id = atoi(value.c_str());
    if ("start" == action) {
      METHOD_CALL(AM_IPC_MW_CMD_MEDIA_START_FILE_RECORDING, &muxer_id,
                  sizeof(muxer_id), &result, sizeof(result), ret);
    } else if ("stop" == action) {
      METHOD_CALL(AM_IPC_MW_CMD_MEDIA_STOP_FILE_RECORDING, &muxer_id,
                  sizeof(muxer_id), &result, sizeof(result), ret);
    } else {
      ret = AM_REST_RESULT_ERR_PARAM;
      char msg[MAX_MSG_LENGHT];
      snprintf(msg, MAX_MSG_LENGHT, "invalid file recording action: %s",
               action.c_str());
      m_utils->set_response_msg(AM_REST_RECORDING_ACTION_ERR, msg);
      break;
    }

    if (result.ret){
      ret = AM_REST_RESULT_ERR_SERVER;
      m_utils->set_response_msg(AM_REST_RECORDING_HANDLE_ERR,
                                "server file recording failed");
      break;
    }
  } while(0);

  return ret;
}

AM_REST_RESULT  AMRestAPIMedia::media_event_recording_handle()
{
  AM_REST_RESULT ret = AM_REST_RESULT_OK;
  AMIApiMediaEvent* event = nullptr;
  do {
    am_service_result_t result = {0};
    event = AMIApiMediaEvent::create();
    if (!event) {
      ret = AM_REST_RESULT_ERR_SERVER;
      m_utils->set_response_msg(AM_REST_RECORDING_HANDLE_ERR, "server failed"
                                " to create AMIApiMediaEventStruct");
      break;
    }
    if ((ret=parse_event_arg(*event)) != AM_REST_RESULT_OK) {
      break;
    }
    METHOD_CALL(AM_IPC_MW_CMD_MEDIA_EVENT_RECORDING_START, event->get_data(),
                event->get_data_size(), &result, sizeof(result), ret);
    if (result.ret) {
      ret = AM_REST_RESULT_ERR_SERVER;
      m_utils->set_response_msg(AM_REST_RECORDING_HANDLE_ERR,
                                "server event recording failed");
      break;
    }
  } while(0);

  delete event;
  return ret;
}

AM_REST_RESULT  AMRestAPIMedia::parse_event_arg(AMIApiMediaEvent& event)
{
  AM_REST_RESULT ret = AM_REST_RESULT_OK;
  do {
    string value, id;
    if ((ret = m_utils->get_value("type", value)) != AM_REST_RESULT_OK) {
      m_utils->set_response_msg(AM_REST_RECORDING_ATTR_ERR,
                                "no file recording attribute parameter");
      break;
    }
    if ((ret = m_utils->get_value("event_id", id)) != AM_REST_RESULT_OK) {
      m_utils->set_response_msg(AM_REST_RECORDING_EVENT_ID_ERR,
                                "no file recording event id");
      break;
    }

    if ("h26x" == value) {
      event.set_attr_h26X();
      if (!event.set_stream_id(atoi(id.c_str()))) {
        ret = AM_REST_RESULT_ERR_SERVER;
        m_utils->set_response_msg(AM_REST_RECORDING_HANDLE_ERR,
                                  "Failed to set h26x event id");
        break;
      }
    } else if ("mjpeg" == value) {
      event.set_attr_mjpeg();
      if (!event.set_stream_id(atoi(id.c_str()))) {
        ret = AM_REST_RESULT_ERR_SERVER;
        m_utils->set_response_msg(AM_REST_RECORDING_HANDLE_ERR,
                                  "Failed to set mjpeg event id");
        break;
      }
      uint8_t num;
      num = (m_utils->get_value("pre_num", value) != AM_REST_RESULT_OK)
              ? 0 : atoi(value.c_str());
      if (!event.set_pre_cur_pts_num(num)) {
        ret = AM_REST_RESULT_ERR_SERVER;
        m_utils->set_response_msg(AM_REST_RECORDING_HANDLE_ERR,
                                  "Failed to set pre cur pts num");
        break;
      }

      num = (m_utils->get_value("post_num", value) != AM_REST_RESULT_OK)
              ? 0 : atoi(value.c_str());
      if (!event.set_after_cur_pts_num(num)) {
        ret = AM_REST_RESULT_ERR_SERVER;
        m_utils->set_response_msg(AM_REST_RECORDING_HANDLE_ERR,
                                  "Failed to set post cur pts num");
        break;
      }

      num = (m_utils->get_value("closest_num", value) != AM_REST_RESULT_OK)
              ? 0 : atoi(value.c_str());
      if (!event.set_closest_cur_pts_num(num)) {
        ret = AM_REST_RESULT_ERR_SERVER;
        m_utils->set_response_msg(AM_REST_RECORDING_HANDLE_ERR,
                                  "Failed to set closest pts num");
        break;
      }
    } else {
      ret = AM_REST_RESULT_ERR_PARAM;
      char msg[MAX_MSG_LENGHT];
      snprintf(msg, MAX_MSG_LENGHT, "invalid file recording attribute: %s",
               value.c_str());
      m_utils->set_response_msg(AM_REST_RECORDING_EVENT_ID_ERR, msg);
      break;
    }
  } while(0);

  return ret;
}

AM_REST_RESULT  AMRestAPIMedia::media_audio_playback_handle()
{
  AM_REST_RESULT ret = AM_REST_RESULT_OK;
  do {
    string action, id;
    int32_t play_id = 0;
    if ((ret = m_utils->get_value("action", action)) != AM_REST_RESULT_OK) {
      m_utils->set_response_msg(AM_REST_PLAYBACK_ACTION_ERR,
                                "no audio playback action parameter");
      break;
    }
    if (m_utils->get_value("play_id", id) == AM_REST_RESULT_OK) {
      play_id = atoi(id.c_str());
    }
    if ("set" == action) {
      ret = audio_playback_set_handle(play_id);
    } else if ("start" == action) {
      ret = audio_playback_start_handle(play_id);
    } else if ("stop" == action) {
      ret = audio_playback_stop_handle(play_id);
    } else if ("pause" == action) {
      ret = audio_playback_pause_handle(play_id);
    } else {
      ret = AM_REST_RESULT_ERR_PARAM;
      char msg[MAX_MSG_LENGHT];
      snprintf(msg, MAX_MSG_LENGHT, "invalid audio playback action: %s",
               action.c_str());
      m_utils->set_response_msg(AM_REST_PLAYBACK_ACTION_ERR, msg);
      break;
    }
  } while(0);

  return ret;
}

AM_REST_RESULT  AMRestAPIMedia::audio_playback_set_handle(int32_t play_id)
{
  AM_REST_RESULT ret = AM_REST_RESULT_OK;
  AMIApiPlaybackAudioFileList* audio_file;
  do {
    audio_file = AMIApiPlaybackAudioFileList::create();
    if(!audio_file) {
      ret = AM_REST_RESULT_ERR_SERVER;
      m_utils->set_response_msg(AM_REST_PLAYBACK_HANDLE_ERR,
                                "server audio playback handle failed");
      break;
    }
    string file;
    int32_t num = 0;
    am_service_result_t result = {0};
    char arg[MAX_PARA_LENGHT] = "file0", abs_path[PATH_MAX] = {0};
    while (m_utils->get_value(arg, file) == AM_REST_RESULT_OK) {
      if (nullptr != realpath(file.c_str(), abs_path)) {
        if (!audio_file->set_playback_id(play_id) ||
            !audio_file->add_file(string(abs_path))) {
          ret = AM_REST_RESULT_ERR_PARAM;
          char msg[MAX_MSG_LENGHT];
          snprintf(msg, MAX_MSG_LENGHT, "failed to add file %s to file list, "
              "file name maybe too long",abs_path);
          m_utils->set_response_msg(AM_REST_PLAYBACK_FILE_ERR, msg);
          num = 0;
          break;
        }
      } else {
        ret = AM_REST_RESULT_ERR_PARAM;
        char msg[MAX_MSG_LENGHT];
        snprintf(msg, MAX_MSG_LENGHT, "failed to add file to file list, file "
            "\"%s\" is not exist or have no permission to access it", abs_path);
        m_utils->set_response_msg(AM_REST_PLAYBACK_FILE_ERR, msg);
        num = 0;
        break;
      }

      if (audio_file->is_full()) {
        METHOD_CALL(AM_IPC_MW_CMD_MEDIA_ADD_AUDIO_FILE,
                    audio_file->get_file_list(),
                    audio_file->get_file_list_size(),
                    &result, sizeof(result), ret);
        if (result.ret) {
          ret = AM_REST_RESULT_ERR_SERVER;
          m_utils->set_response_msg(AM_REST_PLAYBACK_HANDLE_ERR,
                                    "server failed to set audio file");
          num = 0;
          break;
        }
        audio_file->clear_file();
      }
      snprintf(arg, MAX_PARA_LENGHT, "file%d", ++num);
    }
    if ((0 == num)) {
      //ret is ok means no user file, otherwise means parameter is wrong
      if (AM_REST_RESULT_OK == ret) {
        ret = AM_REST_RESULT_ERR_PARAM;
        m_utils->set_response_msg(AM_REST_PLAYBACK_FILE_ERR, "no audio file");
      }
      break;
    }

    //add the rest of file to list
    if (audio_file->get_file_number() > 0) {
      METHOD_CALL(AM_IPC_MW_CMD_MEDIA_ADD_AUDIO_FILE,
                  audio_file->get_file_list(),
                  audio_file->get_file_list_size(),
                  &result, sizeof(result), ret);
      if (result.ret) {
        ret = AM_REST_RESULT_ERR_SERVER;
        m_utils->set_response_msg(AM_REST_PLAYBACK_HANDLE_ERR,
                                  "server failed to add audio file");
        break;
      }
      audio_file->clear_file();
    }
  } while(0);
  delete audio_file;
  audio_file = nullptr;

  return ret;
}

AM_REST_RESULT  AMRestAPIMedia::audio_playback_start_handle(int32_t play_id)
{
  AM_REST_RESULT ret = AM_REST_RESULT_OK;
  do {
    am_service_result_t result = {0};
    METHOD_CALL( AM_IPC_MW_CMD_MEDIA_START_PLAYBACK_AUDIO_FILE,
                 &play_id, sizeof(int32_t),
                 &result, sizeof(result), ret);
    if(result.ret) {
      ret = AM_REST_RESULT_ERR_SERVER;
      m_utils->set_response_msg(AM_REST_PLAYBACK_HANDLE_ERR,
                                "server audio playback start handle failed");
      break;
    }
  } while(0);
  return ret;
}

AM_REST_RESULT  AMRestAPIMedia::audio_playback_stop_handle(int32_t play_id)
{
  AM_REST_RESULT ret = AM_REST_RESULT_OK;
  do {
    am_service_result_t result = {0};
    METHOD_CALL(AM_IPC_MW_CMD_MEDIA_STOP_PLAYBACK_AUDIO_FILE,
                 &play_id, sizeof(int32_t),
                 &result, sizeof(result), ret);
    if(result.ret) {
      ret = AM_REST_RESULT_ERR_SERVER;
      m_utils->set_response_msg(AM_REST_PLAYBACK_HANDLE_ERR,
                                "server audio playback stop handle failed");
      break;
    }
  } while(0);
  return ret;
}

AM_REST_RESULT  AMRestAPIMedia::audio_playback_pause_handle(int32_t play_id)
{
  AM_REST_RESULT ret = AM_REST_RESULT_OK;
  do {
    am_service_result_t result = {0};
    METHOD_CALL( AM_IPC_MW_CMD_MEDIA_PAUSE_PLAYBACK_AUDIO_FILE,
                 &play_id, sizeof(int32_t),
                 &result, sizeof(result), ret);
    if(result.ret) {
      ret = AM_REST_RESULT_ERR_SERVER;
      m_utils->set_response_msg(AM_REST_PLAYBACK_HANDLE_ERR,
                                "server audio playback puase handle failed");
      break;
    }
  } while(0);
  return ret;
}
