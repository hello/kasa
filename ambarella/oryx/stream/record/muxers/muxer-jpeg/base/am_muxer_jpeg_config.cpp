/*******************************************************************************
 * am_muxer_jpeg_config.cpp
 *
 * History:
 *   2015-10-8 - [ccjing] created file
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
#include "am_muxer_jpeg_config.h"

AMMuxerJpegConfig::AMMuxerJpegConfig()
{
}

AMMuxerJpegConfig::~AMMuxerJpegConfig()
{
  delete m_config_jpeg;
  delete m_config_exif;
  delete m_muxer_jpeg_config;
  delete m_exif_config;
}

bool AMMuxerJpegConfig::set_config(AMMuxerCodecJpegConfig* config)
{
  bool ret = true;
  do{
    if(AM_UNLIKELY(config == nullptr)) {
      ERROR("Input config struct is NULL.");
      ret = false;
      break;
    }
    AMConfig &config_jpeg = *m_config_jpeg;
    /*smallest_free_space*/
    if(AM_LIKELY(config_jpeg["smallest_free_space"].exists())) {
      config_jpeg["smallest_free_space"] = config->smallest_free_space;
    }else {
      NOTICE("\"smallest_free_space\" is not exist.");
    }
    /*file_name_prefix*/
    if(AM_LIKELY(config_jpeg["file_name_prefix"].exists())) {
      config_jpeg["file_name_prefix"] = config->file_name_prefix;
    }else{
      NOTICE("\"file_name_prefix\" is not exist.");
    }
    /*max_file_number*/
    if(AM_LIKELY(config_jpeg["max_file_number"].exists())) {
      config_jpeg["max_file_number"] = config->max_file_number;
    } else {
      NOTICE("\"max_file_number\" is not exist.");
    }
    /*recording_file_num*/
    if(AM_LIKELY(config_jpeg["recording_file_num"].exists())) {
      config_jpeg["recording_file_num"] = config->recording_file_num;
    } else {
      NOTICE("\"recording_file_num\" is not exist.");
    }
    /*recording_duration*/
    if(AM_LIKELY(config_jpeg["recording_duration"].exists())) {
      config_jpeg["recording_duration"] = config->recording_duration;
    } else {
      NOTICE("\"recording_duration\" is not exist.");
    }
    /*muxer_id*/
    if (AM_LIKELY(config_jpeg["muxer_id"].exists())) {
      config_jpeg["muxer_id"] = config->muxer_id;
    } else {
      NOTICE("\"muxer_id\" is not exist.");
    }
    /*video_id*/
    if (AM_LIKELY(config_jpeg["video_id"].exists())) {
      config_jpeg["video_id"] = config->video_id;
    } else {
      NOTICE("\"video_id\" is not exist.");
    }
    /*buffer_pkt_num*/
    if (AM_LIKELY(config_jpeg["buffer_pkt_num"].exists())) {
      config_jpeg["buffer_pkt_num"] = config->buffer_pkt_num;
    } else {
      NOTICE("\"buffer_pkt_num\" is not exist.");
    }
    /*buffer_size*/
    if (AM_LIKELY(config_jpeg["buffer_size"].exists())) {
      config_jpeg["buffer_size"] = config->buffer_size;
    } else {
      NOTICE("\"buffer_size\" is not exist.");
    }
    /*muxer_attr*/
    if(AM_LIKELY(config_jpeg["muxer_attr"].exists())) {
      if(config->muxer_attr == AM_MUXER_FILE_NORMAL) {
        config_jpeg["muxer_attr"] = std::string("normal");
      } else if(config->muxer_attr == AM_MUXER_FILE_EVENT) {
        config_jpeg["muxer_attr"] = std::string("event");
      } else {
        WARN("File type error, set normal as default.");
        config_jpeg["muxer_attr"] = std::string("normal");
      }
    }
    /*file_location*/
    if(AM_LIKELY(config_jpeg["file_location"].exists())) {
      config_jpeg["file_location"] = config->file_location;
    } else {
      NOTICE("\"file_location\" is not exist.");
    }
    /*file_location_auto_parse*/
    if(AM_LIKELY(config_jpeg["file_location_auto_parse"].exists())) {
      config_jpeg["file_location_auto_parse"] = config->file_location_auto_parse;
    } else {
      NOTICE("\"file_location_auto_parse\" is not exist");
    }
    /*auto_file_writing*/
    if(AM_LIKELY(config_jpeg["auto_file_writing"].exists())) {
      config_jpeg["auto_file_writing"] = config->auto_file_writing;
    } else {
      NOTICE("\"auto_file_writing\" is not exist.");
    }
    if(AM_UNLIKELY(!config_jpeg.save())) {
      ERROR("Failed to save config information.");
      ret = false;
      break;
    }
  } while(0);

  return ret;
}

AMMuxerCodecJpegConfig* AMMuxerJpegConfig::get_config(const std::string& config_file)
{
  AMMuxerCodecJpegConfig *config = nullptr;
  do {
    if (AM_LIKELY(nullptr == m_muxer_jpeg_config)) {
      m_muxer_jpeg_config = new AMMuxerCodecJpegConfig();
    }
    if (AM_UNLIKELY(!m_muxer_jpeg_config)) {
      ERROR("Failed to create Am_muxer_jpeg_config struct.");
      break;
    }
    delete m_config_jpeg;
    m_config_jpeg = AMConfig::create(config_file.c_str());
    if (AM_LIKELY(m_config_jpeg)) {
      AMConfig &config_jpeg = *m_config_jpeg;
      /*smallest_free_space*/
      if (AM_LIKELY(config_jpeg["smallest_free_space"].exists())) {
        m_muxer_jpeg_config->smallest_free_space =
            config_jpeg["smallest_free_space"].get<unsigned>(20);
      } else {
        NOTICE("\"smallest_free_space\" is not specified, use default!");
        m_muxer_jpeg_config->smallest_free_space = 20;
      }
      /*file_name_prefix*/
      if (AM_LIKELY(config_jpeg["file_name_prefix"].exists())) {
        m_muxer_jpeg_config->file_name_prefix =
            config_jpeg["file_name_prefix"].get<std::string>("S2L");
      } else {
        NOTICE("\"file_name_prefix\" is not specified, use default!");
        m_muxer_jpeg_config->file_name_prefix = "S2L";
      }
      /*file_location*/
      if (AM_LIKELY(config_jpeg["file_location"].exists())) {
        m_muxer_jpeg_config->file_location =
            config_jpeg["file_location"].get<std::string>("/storage/sda1/video");
      } else {
        NOTICE("\"file_location\" is not specified, use default!");
        m_muxer_jpeg_config->file_location = "/storage/sda1/video";
      }
      /*file_location_auto_parse*/
      if(AM_LIKELY(config_jpeg["file_location_auto_parse"].exists())) {
        m_muxer_jpeg_config->file_location_auto_parse =
            config_jpeg["file_location_auto_parse"].get<bool>(true);
      } else {
        NOTICE("\"file_location_auto_parse\" is not specified, use default!");
        m_muxer_jpeg_config->file_location_auto_parse = true;
      }
      /*max_file_number*/
      if (AM_LIKELY(config_jpeg["max_file_number"].exists())) {
        m_muxer_jpeg_config->max_file_number =
            config_jpeg["max_file_number"].get<unsigned>(0);
      } else {
        NOTICE("\"max_file_number\" is not specified, use default!");
        m_muxer_jpeg_config->max_file_number = 0;
      }
      /*recording_file_num*/
      if (AM_LIKELY(config_jpeg["recording_file_num"].exists())) {
        m_muxer_jpeg_config->recording_file_num =
            config_jpeg["recording_file_num"].get<unsigned>(0);
      } else {
        NOTICE("\"recording_file_num\" is not specified, use 0 as default!");
        m_muxer_jpeg_config->recording_file_num = 0;
      }
      /*recording_duration*/
      if (AM_LIKELY(config_jpeg["recording_duration"].exists())) {
        m_muxer_jpeg_config->recording_duration =
            config_jpeg["recording_duration"].get<unsigned>(0);
      } else {
        NOTICE("\"recording_duration\" is not specified, use 0 as default!");
        m_muxer_jpeg_config->recording_duration = 0;
      }
      if (AM_LIKELY(config_jpeg["muxer_attr"].exists())) {
        std::string muxer_attr =
            config_jpeg["muxer_attr"].get<std::string>("normal");
        if (muxer_attr == "normal") {
          m_muxer_jpeg_config->muxer_attr = AM_MUXER_FILE_NORMAL;
        } else if (muxer_attr == "event") {
          m_muxer_jpeg_config->muxer_attr = AM_MUXER_FILE_EVENT;
        } else {
          NOTICE("muxer_attr is error, use default.");
          m_muxer_jpeg_config->muxer_attr = AM_MUXER_FILE_NORMAL;
        }
      } else {
        NOTICE("file type is not specified, use default!");
        m_muxer_jpeg_config->muxer_attr = AM_MUXER_FILE_NORMAL;
      }
      /*muxer_id*/
      if (AM_LIKELY(config_jpeg["muxer_id"].exists())) {
        m_muxer_jpeg_config->muxer_id =
            config_jpeg["muxer_id"].get<unsigned>(0);
      } else {
        NOTICE("\"muxer_id\" is not specified, use default!");
        m_muxer_jpeg_config->muxer_id = 0;
      }
      /*video_id*/
      if (AM_LIKELY(config_jpeg["video_id"].exists())) {
        m_muxer_jpeg_config->video_id =
            config_jpeg["video_id"].get<unsigned>(0);
      } else {
        NOTICE("\"video_id\" is not specified, use default!");
        m_muxer_jpeg_config->video_id = 0;
      }
      /*buffer_pkt_num*/
      if (AM_LIKELY(config_jpeg["buffer_pkt_num"].exists())) {
        m_muxer_jpeg_config->buffer_pkt_num =
            config_jpeg["buffer_pkt_num"].get<unsigned>(0);
      } else {
        NOTICE("\"buffer_pkt_num\" is not specified, use default!");
        m_muxer_jpeg_config->buffer_pkt_num = 0;
      }
      /*buffer_size*/
      if (AM_LIKELY(config_jpeg["buffer_size"].exists())) {
        m_muxer_jpeg_config->buffer_size =
            config_jpeg["buffer_size"].get<unsigned>(3);
      } else {
        NOTICE("\"buffer_size\" is not specified, use default!");
        m_muxer_jpeg_config->buffer_size = 3;
      }
      /*auto_file_writing*/
      if(AM_LIKELY(config_jpeg["auto_file_writing"].exists())) {
        m_muxer_jpeg_config->auto_file_writing =
            config_jpeg["auto_file_writing"].get<bool>(true);
      } else {
        NOTICE("\"auto_file_writing\" is not specified, use default.");
        m_muxer_jpeg_config->auto_file_writing = true;
      }
    } else {
      ERROR("Failed to create AMConfig object.");
      break;
    }
    config = m_muxer_jpeg_config;
  } while (0);

  return config;
}

AMJpegExifConfig* AMMuxerJpegConfig::get_exif_config(const std::string&
                                                     config_file)
{
  AMJpegExifConfig *config = nullptr;
  do {
    if (nullptr == m_exif_config) {
      m_exif_config = new AMJpegExifConfig();
    }
    if (!m_exif_config) {
      ERROR("Failed to create AMJpegExifConfig object!");
      break;
    }

    delete m_config_exif;
    m_config_exif = AMConfig::create(config_file.c_str());
    if (m_config_exif) {
      AMConfig &exif = *m_config_exif;

      if (AM_LIKELY(exif["enable"].exists())) {
        m_exif_config->enable = exif["enable"].get<bool>(true);
      }

      if (AM_LIKELY(exif["make"].exists())) {
        m_exif_config->make = exif["make"].get<std::string>("Ambarella");
      }

      if (AM_LIKELY(exif["model"].exists())) {
        m_exif_config->model = exif["model"].get<std::string>("Ironman");
      }

      if (AM_LIKELY(exif["artist"].exists())) {
        m_exif_config->artist = exif["artist"].get<std::string>("ambarella");
      }

      if (AM_LIKELY(exif["image_description"].exists())) {
        m_exif_config->image_description =
            exif["image_description"].get<std::string>("image_description");
      }

      if (AM_LIKELY(exif["focal_length"].exists())) {
        m_exif_config->focal_length.num =
                                 exif["focal_length"]["num"].get<int>(0);
        m_exif_config->focal_length.denom =
                                 exif["focal_length"]["den"].get<int>(1);
      }

      if (AM_LIKELY(exif["aperture_value"].exists())) {
        m_exif_config->aperture_value.num =
                                 exif["aperture_value"]["num"].get<int>(0);
        m_exif_config->aperture_value.denom =
                                 exif["aperture_value"]["den"].get<int>(1);
      }

      config = m_exif_config;

    } else {
      ERROR("Failed to load configuration file: %s", config_file.c_str());
    }

  } while(0);

  return config;

}
