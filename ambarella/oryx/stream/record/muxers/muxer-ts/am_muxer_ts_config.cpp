/*******************************************************************************
 * am_muxer_ts_config.cpp
 *
 * History:
 *   2014-12-23 - [ccjing] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/

#include "am_base_include.h"
#include "am_define.h"
#include "am_log.h"

#include "am_muxer_codec_info.h"
#include "am_muxer_ts_config.h"
#include "am_configure.h"

AMMuxerTsConfig::AMMuxerTsConfig() :
    m_config(NULL),
    m_muxer_ts_config(NULL)
{
}

AMMuxerTsConfig::~AMMuxerTsConfig()
{
  delete m_config;
  delete m_muxer_ts_config;
}

bool AMMuxerTsConfig::set_config(AMMuxerCodecTSConfig* config)
{
  bool ret = true;
  do{
    if(AM_UNLIKELY(config == NULL)) {
      ERROR("Input config struct is NULL.");
      ret = false;
      break;
    }
    AMConfig &config_ts = *m_config;
    /* file_duration */
    if(AM_LIKELY(config_ts["file_duration"].exists())) {
      config_ts["file_duration"] = config->file_duration;
    }else {
      NOTICE("\"file_duration\" is not exist.");
    }
    /*smallest_free_space*/
    if(AM_LIKELY(config_ts["smallest_free_space"].exists())) {
      config_ts["smallest_free_space"] = config->smallest_free_space;
    }else {
      NOTICE("\"smallest_free_space\" is not exist.");
    }
    /*file_name_prefix*/
    if(AM_LIKELY(config_ts["file_name_prefix"].exists())) {
      config_ts["file_name_prefix"] = config->file_name_prefix;
    }else{
      NOTICE("\"file_name_prefix\" is not exist.");
    }
    /*max_file_number*/
    if(AM_LIKELY(config_ts["max_file_number"].exists())) {
      config_ts["max_file_number"] = config->max_file_number;
    } else {
      NOTICE("\"max_file_number\" is not exist.");
    }
    /*file_location*/
    if(AM_LIKELY(config_ts["file_location"].exists())) {
      config_ts["file_location"] = config->file_location;
    } else {
      NOTICE("\"file_location\" is not exist.");
    }
    /*file_location_auto_parse*/
    if(AM_LIKELY(config_ts["file_location_auto_parse"].exists())) {
      config_ts["file_location_auto_parse"] = config->file_location_auto_parse;
    } else {
      NOTICE("\"file_location_auto_parse\" is not exist.");
    }
    /*hls_enable*/
    if(AM_LIKELY(config_ts["hls_enable"].exists())) {
      config_ts["hls_enable"] = config->hls_enable;
    } else {
      NOTICE("\"hls_enable\" is not exist.");
    }
    /*auto_file_writing*/
    if(AM_LIKELY(config_ts["auto_file_writing"].exists())) {
      config_ts["auto_file_writing"] = config->hls_enable;
    } else {
      NOTICE("\"auto_file_writing\" is not exist.");
    }
    if (m_muxer_ts_config->muxer_attr == AM_MUXER_FILE_NORMAL) {
      /*video id*/
      if (AM_LIKELY(config_ts["video_id"].exists())) {
        config_ts["video_id"] = config->video_id;
      } else {
        NOTICE("\"video_id\" is not exist.");
      }
      /*audio_type*/
      if (AM_LIKELY(config_ts["audio_type"].exists())) {
        if (config->audio_type != AM_AUDIO_AAC) {
          WARN("Currently, mp4 muxer only support AAC audio! reset to AAC.");
        }
        config_ts["audio_type"] = std::string("aac");
      }
    }
    /*file type*/
    if (AM_LIKELY(config_ts["muxer_attr"].exists())) {
      if (config->muxer_attr == AM_MUXER_FILE_NORMAL) {
        config_ts["muxer_attr"] = std::string("normal");
      } else if (config->muxer_attr == AM_MUXER_FILE_EVENT) {
        config_ts["muxer_attr"] = std::string("event");
      } else {
        WARN("File type error, set normal as default.");
        config_ts["muxer_attr"] = std::string("normal");
      }
    }
    if(AM_UNLIKELY(!config_ts.save())) {
      ERROR("Failed to save config information.");
      ret = false;
      break;
    }
  } while(0);
  return ret;
}

AMMuxerCodecTSConfig* AMMuxerTsConfig::get_config(const std::string& config_file)
{
  AMMuxerCodecTSConfig *config = NULL;
  do {
    if (AM_LIKELY(NULL == m_muxer_ts_config)) {
      m_muxer_ts_config = new AMMuxerCodecTSConfig();
    }
    if (AM_UNLIKELY(!m_muxer_ts_config)) {
      ERROR("Failed to create AM_MUXER_TS_CONFIG struct.");
      break;
    }
    delete m_config;
    m_config = AMConfig::create(config_file.c_str());
    if (AM_LIKELY(m_config)) {
      AMConfig &config_ts = *m_config;
      /* file_duration */
      if (AM_LIKELY(config_ts["muxer_attr"].exists())) {
        std::string muxer_attr =
            config_ts["muxer_attr"].get < std::string> ("normal");
        if (muxer_attr == "normal") {
          m_muxer_ts_config->muxer_attr = AM_MUXER_FILE_NORMAL;
        } else if (muxer_attr == "event") {
          m_muxer_ts_config->muxer_attr = AM_MUXER_FILE_EVENT;
        } else {
          NOTICE("muxer_attr is error, use default.");
          m_muxer_ts_config->muxer_attr = AM_MUXER_FILE_NORMAL;
        }
      } else {
        NOTICE("file type is not specified, use default!");
        m_muxer_ts_config->muxer_attr = AM_MUXER_FILE_NORMAL;
      }
      /* file_duration */
      if (AM_LIKELY(config_ts["file_duration"].exists())) {
        m_muxer_ts_config->file_duration =
            config_ts["file_duration"].get<unsigned>(300);
      } else {
        NOTICE("\"file_duration\" is not specified, use default!");
        m_muxer_ts_config->file_duration = 300;
      }
      /*file_location_auto_parse*/
      if (AM_LIKELY(config_ts["file_location_auto_parse"].exists())) {
        m_muxer_ts_config->file_location_auto_parse =
            config_ts["file_location_auto_parse"].get<bool>(true);
      } else {
        NOTICE("\"file_location_auto_parse\" is not specified, use default!");
        m_muxer_ts_config->file_location_auto_parse = true;
      }
      /*hls_enable*/
      if (AM_LIKELY(config_ts["hls_enable"].exists())) {
        m_muxer_ts_config->hls_enable =
            config_ts["hls_enable"].get<bool>(false);
      } else {
        NOTICE("\"hls_enable\" is not specified, use default!");
        m_muxer_ts_config->hls_enable = false;
      }
      /*auto_file_writing*/
      if (AM_LIKELY(config_ts["auto_file_writing"].exists())) {
        m_muxer_ts_config->auto_file_writing =
            config_ts["auto_file_writing"].get<bool>(true);
      } else {
        NOTICE("\"auto_file_writing\" is not specified, use default!");
        m_muxer_ts_config->auto_file_writing = true;
      }
      /*smallest_free_space*/
      if (AM_LIKELY(config_ts["smallest_free_space"].exists())) {
        m_muxer_ts_config->smallest_free_space =
            config_ts["smallest_free_space"].get<unsigned>(20);
      } else {
        NOTICE("\"smallest_free_space\" is not specified, use default!");
        m_muxer_ts_config->smallest_free_space = 20;
      }
      /*file_name_prefix*/
      if (AM_LIKELY(config_ts["file_name_prefix"].exists())) {
        m_muxer_ts_config->file_name_prefix =
            config_ts["file_name_prefix"].get < std::string > ("S2L");
      } else {
        NOTICE("\"file_name_prefix\" is not specified, use default!");
        m_muxer_ts_config->file_name_prefix = "S2L";
      }
      /*max_file_number*/
      if (AM_LIKELY(config_ts["max_file_number"].exists())) {
        m_muxer_ts_config->max_file_number =
            config_ts["max_file_number"].get<unsigned>(512);
      } else {
        NOTICE("\"max_file_number\" is not specified, use default!");
        m_muxer_ts_config->max_file_number = 1;
      }
      /*file_location*/
      if (AM_LIKELY(config_ts["file_location"].exists())) {
        m_muxer_ts_config->file_location =
            config_ts["file_location"].get< std::string > ("/storage/sda1/video");
      } else {
        NOTICE("\"file_location\" is not specified, use default!");
        m_muxer_ts_config->file_location = "/storage/sda1/video";
      }
      /* video_id */
      if (m_muxer_ts_config->muxer_attr == AM_MUXER_FILE_NORMAL) {
        if (AM_LIKELY(config_ts["video_id"].exists())) {
          m_muxer_ts_config->video_id =
              config_ts["video_id"].get<unsigned>(0);
        } else {
          NOTICE("\"video_id\" is not specified, use default!");
          m_muxer_ts_config->video_id = 0;
        }
        /* audio_type */
        if (AM_LIKELY(config_ts["audio_type"].exists())) {
          std::string audio = config_ts["audio_type"].get < std::string> ("aac");
          if (audio != "aac") {
            WARN("Currently MP4 muxer only supports AAC audio! reset to AAC!");
          }
          m_muxer_ts_config->audio_type = AM_AUDIO_AAC;
        } else {
          NOTICE("\"audio_type\" is not specified, use default!");
          m_muxer_ts_config->audio_type = AM_AUDIO_AAC;
        }
      }
    } else {
      ERROR("Failed to create AMConfig object.");
      break;
    }
    config = m_muxer_ts_config;

  } while (0);

  return config;
}
