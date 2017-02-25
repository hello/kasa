/*******************************************************************************
 * am_muxer_mp4_config.cpp
 *
 * History:
 *   2014-12-23 - [ccjing] created file
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
 *
 ******************************************************************************/
#include "am_base_include.h"
#include "am_define.h"
#include "am_log.h"

#include "am_configure.h"
#include "am_muxer_codec_info.h"
#include "am_muxer_mp4_config.h"

AMMuxerMp4Config::AMMuxerMp4Config() :
    m_config(nullptr),
    m_muxer_mp4_config(nullptr)
{
}

AMMuxerMp4Config::~AMMuxerMp4Config()
{
  delete m_config;
  delete m_muxer_mp4_config;
}

bool AMMuxerMp4Config::set_config(AMMuxerCodecMp4Config* config)
{
  bool ret = true;
  do{
    if(AM_UNLIKELY(config == nullptr)) {
      ERROR("Input config struct is NULL.");
      ret = false;
      break;
    }
    AMConfig &config_mp4 = *m_config;
    /* file_duration */
    if(AM_LIKELY(config_mp4["file_duration"].exists())) {
      config_mp4["file_duration"] = config->file_duration;
    }else {
      NOTICE("\"file_duration\" is not exist.");
    }
    /*smallest_free_space*/
    if(AM_LIKELY(config_mp4["smallest_free_space"].exists())) {
      config_mp4["smallest_free_space"] = config->smallest_free_space;
    }else {
      NOTICE("\"smallest_free_space\" is not exist.");
    }
    /*file_name_prefix*/
    if(AM_LIKELY(config_mp4["file_name_prefix"].exists())) {
      config_mp4["file_name_prefix"] = config->file_name_prefix;
    }else{
      NOTICE("\"file_name_prefix\" is not exist.");
    }
    /*max_file_num_per_folder*/
    if(AM_LIKELY(config_mp4["max_file_num_per_folder"].exists())) {
      config_mp4["max_file_num_per_folder"] = config->max_file_num_per_folder;
    } else {
      NOTICE("\"max_file_num_per_folder\" is not exist.");
    }
    /*recording_file_num*/
    if(AM_LIKELY(config_mp4["recording_file_num"].exists())) {
      config_mp4["recording_file_num"] = config->recording_file_num;
    } else {
      NOTICE("\"recording_file_num\" is not exist.");
    }
    /*recording_duration*/
    if(AM_LIKELY(config_mp4["recording_duration"].exists())) {
      config_mp4["recording_duration"] = config->recording_duration;
    } else {
      NOTICE("\"recording_duration\" is not exist.");
    }
    /*write_index_frequency*/
    if (AM_LIKELY(config_mp4["write_index_frequency"].exists())) {
      config_mp4["write_index_frequency"] =
          config->write_index_frequency;
    } else {
      NOTICE("\"write_index_frequency\" is not exist.");
    }
    /*file_location*/
    if(AM_LIKELY(config_mp4["file_location"].exists())) {
      config_mp4["file_location"] = config->file_location;
    } else {
      NOTICE("\"file_location\" is not exist.");
    }
    /*file_location_auto_parse*/
    if(AM_LIKELY(config_mp4["file_location_auto_parse"].exists())) {
      config_mp4["file_location_auto_parse"] = config->file_location_auto_parse;
    } else {
      NOTICE("\"file_location_auto_parse\" is not exist");
    }
    /*hls_enable*/
    if(AM_LIKELY(config_mp4["hls_enable"].exists())) {
      config_mp4["hls_enable"] = config->hls_enable;
    } else {
      NOTICE("\"hls_enable\" is not exist.");
    }
    /*write_sync_enable*/
    if(AM_LIKELY(config_mp4["write_sync_enable"].exists())) {
      config_mp4["write_sync_enable"] = config->write_sync_enable;
    } else {
      NOTICE("\"write_sync_enable\" is not exist.");
    }
    /*reconstruct_enable*/
    if (AM_LIKELY(config_mp4["reconstruct_enable"].exists())) {
      config_mp4["reconstruct_enable"] = config->reconstruct_enable;
    } else {
      NOTICE("\"reconstruct_enable\" is not exist.");
    }
    /*auto_file_writing*/
    if(AM_LIKELY(config_mp4["auto_file_writing"].exists())) {
      config_mp4["auto_file_writing"] = config->auto_file_writing;
    } else {
      NOTICE("\"auto_file_writing\" is not exist.");
    }
    /*video id*/
    if (AM_LIKELY(config_mp4["video_id"].exists())) {
      config_mp4["video_id"] = config->video_id;
    } else {
      NOTICE("\"video_id\" is not exist.");
    }
    /*audio_sample_rate*/
    if (AM_LIKELY(config_mp4["audio_sample_rate"].exists())) {
      config_mp4["audio_sample_rate"] = config->audio_sample_rate;
    } else {
      NOTICE("\"audio_sample_rate\" is not exist.");
    }
    /*muxer id*/
    if (AM_LIKELY(config_mp4["muxer_id"].exists())) {
      config_mp4["muxer_id"] = config->muxer_id;
    } else {
      NOTICE("\"muxer_id\" is not exist.");
    }
    /*audio_type*/
    if (AM_LIKELY(config_mp4["audio_type"].exists())) {
      switch (config->audio_type) {
        case AM_AUDIO_AAC : {
          config_mp4["audio_type"] = std::string("aac");
        } break;
        case AM_AUDIO_OPUS : {
          config_mp4["audio_type"] = std::string("opus");
        } break;
        case AM_AUDIO_G726_40 : {
          config_mp4["audio_type"] = std::string("g726_40");
        } break;
        case AM_AUDIO_G726_32 : {
          config_mp4["audio_type"] = std::string("g726_32");
        } break;
        case AM_AUDIO_G726_24 : {
          config_mp4["audio_type"] = std::string("g726_24");
        } break;
        case AM_AUDIO_G726_16 : {
          config_mp4["audio_type"] = std::string("g726_16");
        } break;
        case AM_AUDIO_NULL : {
          config_mp4["audio_type"] = std::string("none");
        } break;
        default : {
          WARN("The audio type %u is not supported, set none as default.");
          config_mp4["audio_type"] = std::string("none");
        }
      }
    }
    /*muxer_attr*/
    if(AM_LIKELY(config_mp4["muxer_attr"].exists())) {
      if(config->muxer_attr == AM_MUXER_FILE_NORMAL) {
        config_mp4["muxer_attr"] = std::string("normal");
      } else if(config->muxer_attr == AM_MUXER_FILE_EVENT) {
        config_mp4["muxer_attr"] = std::string("event");
      } else {
        WARN("File type error, set normal as default.");
        config_mp4["muxer_attr"] = std::string("normal");
      }
    }
    if(AM_UNLIKELY(!config_mp4.save())) {
      ERROR("Failed to save config information.");
      ret = false;
      break;
    }
  } while(0);

  return ret;
}

AMMuxerCodecMp4Config* AMMuxerMp4Config::get_config(const std::string& config_file)
{
  AMMuxerCodecMp4Config *config = NULL;
  do {
    if (AM_LIKELY(NULL == m_muxer_mp4_config)) {
      m_muxer_mp4_config = new AMMuxerCodecMp4Config();
    }
    if (AM_UNLIKELY(!m_muxer_mp4_config)) {
      ERROR("Failed to create Am_muxer_mp4_config struct.");
      break;
    }
    delete m_config;
    m_config = AMConfig::create(config_file.c_str());
    if (AM_LIKELY(m_config)) {
      AMConfig &config_mp4 = *m_config;
      /*muxer attr*/
      if (AM_LIKELY(config_mp4["muxer_attr"].exists())) {
        std::string muxer_attr =
            config_mp4["muxer_attr"].get<std::string>("normal");
        if (muxer_attr == "normal") {
          m_muxer_mp4_config->muxer_attr = AM_MUXER_FILE_NORMAL;
        } else if (muxer_attr == "event") {
          m_muxer_mp4_config->muxer_attr = AM_MUXER_FILE_EVENT;
        } else {
          NOTICE("muxer_attr is error, use default.");
          m_muxer_mp4_config->muxer_attr = AM_MUXER_FILE_NORMAL;
        }
      } else {
        NOTICE("file type is not specified, use default!");
        m_muxer_mp4_config->muxer_attr = AM_MUXER_FILE_NORMAL;
      }
      /* file_duration */
      if (AM_LIKELY(config_mp4["file_duration"].exists())) {
        m_muxer_mp4_config->file_duration =
            config_mp4["file_duration"].get<signed>(300);
      } else {
        NOTICE("\"file_duration\" is not specified, use default!");
        m_muxer_mp4_config->file_duration = 300;
      }
      /*smallest_free_space*/
      if (AM_LIKELY(config_mp4["smallest_free_space"].exists())) {
        m_muxer_mp4_config->smallest_free_space =
            config_mp4["smallest_free_space"].get<unsigned>(20);
      } else {
        NOTICE("\"smallest_free_space\" is not specified, use default!");
        m_muxer_mp4_config->smallest_free_space = 20;
      }
      /*file_name_prefix*/
      if (AM_LIKELY(config_mp4["file_name_prefix"].exists())) {
        m_muxer_mp4_config->file_name_prefix =
            config_mp4["file_name_prefix"].get<std::string>("S2L");
      } else {
        NOTICE("\"file_name_prefix\" is not specified, use default!");
        m_muxer_mp4_config->file_name_prefix = "S2L";
      }
      /*max_file_num_per_folder*/
      if (AM_LIKELY(config_mp4["max_file_num_per_folder"].exists())) {
        m_muxer_mp4_config->max_file_num_per_folder =
            config_mp4["max_file_num_per_folder"].get<unsigned>(512);
      } else {
        NOTICE("\"max_file_num_per_folder\" is not specified, use 512 as default!");
        m_muxer_mp4_config->max_file_num_per_folder = 512;
      }
      /*recording_file_num*/
      if (AM_LIKELY(config_mp4["recording_file_num"].exists())) {
        m_muxer_mp4_config->recording_file_num =
            config_mp4["recording_file_num"].get<unsigned>(0);
      } else {
        NOTICE("\"recording_file_num\" is not specified, use 0 as default!");
        m_muxer_mp4_config->recording_file_num = 0;
      }
      /*recording_duration*/
      if (AM_LIKELY(config_mp4["recording_duration"].exists())) {
        m_muxer_mp4_config->recording_duration =
            config_mp4["recording_duration"].get<unsigned>(0);
      } else {
        NOTICE("\"recording_duration\" is not specified, use 0 as default!");
        m_muxer_mp4_config->recording_duration = 0;
      }
      /*file_location*/
      if (AM_LIKELY(config_mp4["file_location"].exists())) {
        m_muxer_mp4_config->file_location =
            config_mp4["file_location"].get<std::string>("/storage/sda1/video");
      } else {
        NOTICE("\"file_location\" is not specified, use default!");
        m_muxer_mp4_config->file_location = "/storage/sda1/video";
      }
      /*file_location_auto_parse*/
      if(AM_LIKELY(config_mp4["file_location_auto_parse"].exists())) {
        m_muxer_mp4_config->file_location_auto_parse =
            config_mp4["file_location_auto_parse"].get<bool>(true);
      } else {
        NOTICE("\"file_location_auto_parse\" is not specified, use default!");
        m_muxer_mp4_config->file_location_auto_parse = true;
      }
      /*hls_enable*/
      if(AM_LIKELY(config_mp4["hls_enable"].exists())) {
        m_muxer_mp4_config->hls_enable =
            config_mp4["hls_enable"].get<bool>(false);
      } else {
        NOTICE("\"hls_enable\" is not specified, use default.");
        m_muxer_mp4_config->hls_enable = false;
      }
      /*write_sync_enable*/
      if(AM_LIKELY(config_mp4["write_sync_enable"].exists())) {
        m_muxer_mp4_config->write_sync_enable =
            config_mp4["write_sync_enable"].get<bool>(false);
      } else {
        NOTICE("\"write_sync_enable\" is not specified, use default.");
        m_muxer_mp4_config->write_sync_enable = false;
      }
      /*reconstruct_enable*/
      if (AM_LIKELY(config_mp4["reconstruct_enable"].exists())) {
        m_muxer_mp4_config->reconstruct_enable =
            config_mp4["reconstruct_enable"].get<bool>(false);
      } else {
        NOTICE("\"reconstruct_enable\" is not specified, use default.");
        m_muxer_mp4_config->reconstruct_enable = false;
      }
      /*auto_file_writing*/
      if(AM_LIKELY(config_mp4["auto_file_writing"].exists())) {
        m_muxer_mp4_config->auto_file_writing =
            config_mp4["auto_file_writing"].get<bool>(true);
      } else {
        NOTICE("\"auto_file_writing\" is not specified, use default.");
        m_muxer_mp4_config->auto_file_writing = true;
      }
      /* video_id */
      if (AM_LIKELY(config_mp4["video_id"].exists())) {
        m_muxer_mp4_config->video_id =
            config_mp4["video_id"].get<unsigned>(0);
      } else {
        NOTICE("\"video_id\" is not specified, use default!");
        m_muxer_mp4_config->video_id = 0;
      }
      /* muxer_id */
      if (AM_LIKELY(config_mp4["muxer_id"].exists())) {
        m_muxer_mp4_config->muxer_id = config_mp4["muxer_id"].get<unsigned>(0);
      } else {
        NOTICE("\"muxer_id\" is not specified, use default!");
        m_muxer_mp4_config->muxer_id = 0;
      }
      /* write_index_frequency */
      if (AM_LIKELY(config_mp4["write_index_frequency"].exists())) {
        m_muxer_mp4_config->write_index_frequency =
            config_mp4["write_index_frequency"].get<unsigned>(0);
      } else {
        NOTICE("\"write_index_frequency\" is not specified, use default!");
        m_muxer_mp4_config->write_index_frequency = 0;
      }
      /* max file size */
      if (AM_LIKELY(config_mp4["max_file_size"].exists())) {
        m_muxer_mp4_config->max_file_size =
            config_mp4["max_file_size"].get<unsigned>(1024);
      } else {
        NOTICE("\"max_file_size\" is not specified, use default!");
        m_muxer_mp4_config->max_file_size = 1024;
      }
      /*audio_sample_rate*/
      if (AM_LIKELY(config_mp4["audio_sample_rate"].exists())) {
        m_muxer_mp4_config->audio_sample_rate =
            config_mp4["audio_sample_rate"].get<unsigned>(0);
      } else {
        NOTICE("\"audio_sample_rate\" is not specified, use 0(auto) as default!");
        m_muxer_mp4_config->audio_sample_rate = 0;
      }
      /*audio_type*/
      if (AM_LIKELY(config_mp4["audio_type"].exists())) {
        std::string audio = config_mp4["audio_type"].get<std::string>("aac");
        if(audio == "aac") {
          m_muxer_mp4_config->audio_type = AM_AUDIO_AAC;
        } else if(audio == "opus") {
          m_muxer_mp4_config->audio_type = AM_AUDIO_OPUS;
        } else if (audio == "g726_16") {
          m_muxer_mp4_config->audio_type = AM_AUDIO_G726_16;
        } else if (audio == "g726_24") {
          m_muxer_mp4_config->audio_type = AM_AUDIO_G726_24;
        } else if (audio == "g726_32") {
          m_muxer_mp4_config->audio_type = AM_AUDIO_G726_32;
        } else if (audio == "g726_40") {
          m_muxer_mp4_config->audio_type = AM_AUDIO_G726_40;
        } else if (audio == "none") {
          m_muxer_mp4_config->audio_type = AM_AUDIO_NULL;
        } else {
          m_muxer_mp4_config->audio_type = AM_AUDIO_NULL;
          WARN("audio type %s is not supported, set NULL as default.",
               audio.c_str());
        }
      } else {
        NOTICE("\"audio_type\" is not specified, use default!");
        m_muxer_mp4_config->audio_type = AM_AUDIO_NULL;
      }
    } else {
      ERROR("Failed to create AMConfig object.");
      break;
    }
    config = m_muxer_mp4_config;

  } while (0);

  return config;
}
