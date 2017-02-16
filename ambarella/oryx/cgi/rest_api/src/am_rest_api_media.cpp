/**
 * am_rest_api_media.cpp
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
#include <limits.h>

#include "am_rest_api_media.h"
#include "am_api_media.h"
#include "am_define.h"

using std::string;

AM_REST_RESULT AMRestAPIMedia::rest_api_handle()
{
  AM_REST_RESULT ret = AM_REST_RESULT_OK;
  do {
    string cmd_group;
    if ((m_utils->get_value("arg1", cmd_group)) != AM_REST_RESULT_OK) {
      ret = AM_REST_RESULT_ERR_URL;
      m_utils->set_response_msg(AM_REST_URL_ARG1_ERR, "no media cmp group parameter");
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
        snprintf(msg, MAX_MSG_LENGHT, "invalid recording type: %s", recording_type.c_str());
        m_utils->set_response_msg(AM_REST_RECORDING_TYPE_ERR, msg);
      }
    } else if ("playback" == cmd_group) {
      string playback_type;
      if ((m_utils->get_value("arg2", playback_type)) != AM_REST_RESULT_OK) {
        ret = AM_REST_RESULT_ERR_URL;
        m_utils->set_response_msg(AM_REST_URL_ARG1_ERR, "no media playback type parameter");
        break;
      }
      if ("audio" == playback_type) {
        ret = media_audio_playback_handle();
      } else {
        ret = AM_REST_RESULT_ERR_URL;
        char msg[MAX_MSG_LENGHT];
        snprintf(msg, MAX_MSG_LENGHT, "invalid playback type: %s", playback_type.c_str());
        m_utils->set_response_msg(AM_REST_PLAYBACK_TYPE_ERR, msg);
      }
    } else {
      ret = AM_REST_RESULT_ERR_URL;
      char msg[MAX_MSG_LENGHT];
      snprintf(msg, MAX_MSG_LENGHT, "invalid media cmp group: %s", cmd_group.c_str());
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
      m_utils->set_response_msg(AM_REST_RECORDING_ACTION_ERR, "no recording action parameter");
      break;
    }

    int32_t result = 0;
    if ("start" == action) {
      gCGI_api_helper->method_call(AM_IPC_MW_CMD_MEDIA_START_RECORDING, nullptr, 0,
                                   &result, sizeof(result));
    } else if ("stop" == action) {
      gCGI_api_helper->method_call(AM_IPC_MW_CMD_MEDIA_STOP_RECORDING, nullptr, 0,
                                   &result, sizeof(result));
    } else {
      ret = AM_REST_RESULT_ERR_PARAM;
      char msg[MAX_MSG_LENGHT];
      snprintf(msg, MAX_MSG_LENGHT, "invalid recording action: %s", action.c_str());
      m_utils->set_response_msg(AM_REST_RECORDING_ACTION_ERR, msg);
      break;
    }

    if (result){
      ret = AM_REST_RESULT_ERR_SERVER;
      m_utils->set_response_msg(AM_REST_RECORDING_HANDLE_ERR, "server recording failed");
      break;
    }
  } while(0);

  return ret;
}

AM_REST_RESULT  AMRestAPIMedia::media_file_recording_handle()
{
  AM_REST_RESULT ret = AM_REST_RESULT_OK;
  do {
    string action;
    if ((ret = m_utils->get_value("action", action)) != AM_REST_RESULT_OK) {
      m_utils->set_response_msg(AM_REST_RECORDING_ACTION_ERR, "no file recording action parameter");
      break;
    }

    int32_t result = 0;
    if ("start" == action) {
      gCGI_api_helper->method_call(AM_IPC_MW_CMD_MEDIA_START_FILE_RECORDING, nullptr, 0,
                                   &result, sizeof(result));
    } else if ("stop" == action) {
      gCGI_api_helper->method_call(AM_IPC_MW_CMD_MEDIA_STOP_FILE_RECORDING, nullptr, 0,
                                   &result, sizeof(result));
    } else {
      ret = AM_REST_RESULT_ERR_PARAM;
      char msg[MAX_MSG_LENGHT];
      snprintf(msg, MAX_MSG_LENGHT, "invalid file recording action: %s", action.c_str());
      m_utils->set_response_msg(AM_REST_RECORDING_ACTION_ERR, msg);
      break;
    }

    if (result){
      ret = AM_REST_RESULT_ERR_SERVER;
      m_utils->set_response_msg(AM_REST_RECORDING_HANDLE_ERR, "server file recording failed");
      break;
    }
  } while(0);

  return ret;
}

AM_REST_RESULT  AMRestAPIMedia::media_event_recording_handle()
{
  AM_REST_RESULT ret = AM_REST_RESULT_OK;
  do {
    int32_t result = 0;
    gCGI_api_helper->method_call(AM_IPC_MW_CMD_MEDIA_EVENT_RECORDING_START, nullptr, 0,
                                 &result, sizeof(result));
    if (result){
      ret = AM_REST_RESULT_ERR_SERVER;
      m_utils->set_response_msg(AM_REST_RECORDING_HANDLE_ERR, "server event recording failed");
      break;
    }
  } while(0);

  return ret;
}

AM_REST_RESULT  AMRestAPIMedia::media_audio_playback_handle()
{
  AM_REST_RESULT ret = AM_REST_RESULT_OK;
  do {
    string action;
    if ((ret = m_utils->get_value("action", action)) != AM_REST_RESULT_OK) {
      m_utils->set_response_msg(AM_REST_PLAYBACK_ACTION_ERR, "no audio playback action parameter");
      break;
    }
    if ("set" == action) {
      ret = audio_playback_set_handle();
    } else if ("start" == action) {
      ret = audio_playback_start_handle();
    } else if ("stop" == action) {
      ret = audio_playback_stop_handle();
    } else if ("pause" == action) {
      ret = audio_playback_pause_handle();
    } else {
      ret = AM_REST_RESULT_ERR_PARAM;
      char msg[MAX_MSG_LENGHT];
      snprintf(msg, MAX_MSG_LENGHT, "invalid audio playback action: %s", action.c_str());
      m_utils->set_response_msg(AM_REST_PLAYBACK_ACTION_ERR, msg);
      break;
    }
  } while(0);

  return ret;
}

AM_REST_RESULT  AMRestAPIMedia::audio_playback_set_handle()
{
  AM_REST_RESULT ret = AM_REST_RESULT_OK;
  AMIApiPlaybackAudioFileList* audio_file;
  do {
    audio_file = AMIApiPlaybackAudioFileList::create();
    if(!audio_file) {
      ret = AM_REST_RESULT_ERR_SERVER;
      m_utils->set_response_msg(AM_REST_PLAYBACK_HANDLE_ERR, "server audio playback handle failed");
      break;
    }
    string file;
    int32_t num = 0, result = 0;
    char arg[MAX_PARA_LENGHT] = "file0", abs_path[PATH_MAX] = {0};
    while (m_utils->get_value(arg, file) == AM_REST_RESULT_OK) {
      if (nullptr != realpath(file.c_str(), abs_path)) {
        if (!audio_file->add_file(string(abs_path))) {
          ret = AM_REST_RESULT_ERR_PARAM;
          char msg[MAX_MSG_LENGHT];
          snprintf(msg, MAX_MSG_LENGHT, "failed to add file %s to file list, file name maybe too long",abs_path);
          m_utils->set_response_msg(AM_REST_PLAYBACK_FILE_ERR, msg);
          num = 0;
          break;
        }
      } else {
        ret = AM_REST_RESULT_ERR_PARAM;
        char msg[MAX_MSG_LENGHT];
        snprintf(msg, MAX_MSG_LENGHT, "failed to add file to file list, file \"%s\" is not exist or "
                 "have no permission to access it", abs_path);
        m_utils->set_response_msg(AM_REST_PLAYBACK_FILE_ERR, msg);
        num = 0;
        break;
      }

      if (audio_file->is_full()) {
        gCGI_api_helper->method_call(AM_IPC_MW_CMD_MEDIA_ADD_AUDIO_FILE, audio_file->get_file_list(),
                                     audio_file->get_file_list_size(), &result, sizeof(result));
        if (result) {
          ret = AM_REST_RESULT_ERR_SERVER;
          m_utils->set_response_msg(AM_REST_PLAYBACK_HANDLE_ERR, "server failed to set audio file");
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
      gCGI_api_helper->method_call(AM_IPC_MW_CMD_MEDIA_ADD_AUDIO_FILE, audio_file->get_file_list(),
                                   audio_file->get_file_list_size(), &result, sizeof(result));
      if (result) {
        ret = AM_REST_RESULT_ERR_SERVER;
        m_utils->set_response_msg(AM_REST_PLAYBACK_HANDLE_ERR, "server failed to add audio file");
        break;
      }
      audio_file->clear_file();
    }
  } while(0);
  delete audio_file;
  audio_file = nullptr;

  return ret;
}

AM_REST_RESULT  AMRestAPIMedia::audio_playback_start_handle()
{
  AM_REST_RESULT ret = AM_REST_RESULT_OK;
  do {
    int32_t result = 0;
    gCGI_api_helper->method_call(
        AM_IPC_MW_CMD_MEDIA_START_PLAYBACK_AUDIO_FILE, nullptr, 0,
        &result, sizeof(result));
    if(result) {
      ret = AM_REST_RESULT_ERR_SERVER;
      m_utils->set_response_msg(AM_REST_PLAYBACK_HANDLE_ERR, "server audio playback start handle failed");
      break;
    }
  } while(0);
  return ret;
}

AM_REST_RESULT  AMRestAPIMedia::audio_playback_stop_handle()
{
  AM_REST_RESULT ret = AM_REST_RESULT_OK;
  do {
    int32_t result = 0;
    gCGI_api_helper->method_call( AM_IPC_MW_CMD_MEDIA_STOP_PLAYBACK_AUDIO_FILE, nullptr, 0,
                         &result, sizeof(result));
    if(result) {
      ret = AM_REST_RESULT_ERR_SERVER;
      m_utils->set_response_msg(AM_REST_PLAYBACK_HANDLE_ERR, "server audio playback stop handle failed");
      break;
    }
  } while(0);
  return ret;
}

AM_REST_RESULT  AMRestAPIMedia::audio_playback_pause_handle()
{
  AM_REST_RESULT ret = AM_REST_RESULT_OK;
  do {
    int32_t result = 0;
    gCGI_api_helper->method_call( AM_IPC_MW_CMD_MEDIA_PAUSE_PLAYBACK_AUDIO_FILE, nullptr, 0,
                         &result, sizeof(result));
    if(result) {
      ret = AM_REST_RESULT_ERR_SERVER;
      m_utils->set_response_msg(AM_REST_PLAYBACK_HANDLE_ERR, "server audio playback puase handle failed");
      break;
    }
  } while(0);
  return ret;
}
