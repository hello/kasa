/*******************************************************************************
 * am_muxer_periodic_mjpeg_config.cpp
 *
 * History:
 *   2016-04-20 - [ccjing] created file
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
#include "am_muxer_periodic_mjpeg_config.h"

AMMuxerPeriodicMjpegConfig::AMMuxerPeriodicMjpegConfig()
{
}

AMMuxerPeriodicMjpegConfig::~AMMuxerPeriodicMjpegConfig()
{
  delete m_config;
  delete m_periodic_mjpeg_config;
}

bool AMMuxerPeriodicMjpegConfig::set_config(AMMuxerCodecPeriodicMjpegConfig* config)
{
  bool ret = true;
  do{
    if(AM_UNLIKELY(config == NULL)) {
      ERROR("Input config struct is NULL.");
      ret = false;
      break;
    }
    AMConfig &config_jpeg = *m_config;
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
    /*muxer_id*/
    if (AM_LIKELY(config_jpeg["muxer_id"].exists())) {
      config_jpeg["muxer_id"] = config->muxer_id;
    } else {
      NOTICE("\"muxer_id\" is not exist.");
    }
    /*max file size*/
    if (AM_LIKELY(config_jpeg["max_file_size"].exists())) {
      config_jpeg["max_file_size"] = config->max_file_size;
    } else {
      NOTICE("\"max_file_size\" is not exist.");
    }
    /*video_id*/
    if (AM_LIKELY(config_jpeg["video_id"].exists())) {
      config_jpeg["video_id"] = config->video_id;
    } else {
      NOTICE("\"video_id\" is not exist.");
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

AMMuxerCodecPeriodicMjpegConfig* AMMuxerPeriodicMjpegConfig::get_config(
    const std::string& config_file)
{
  AMMuxerCodecPeriodicMjpegConfig *config = NULL;
  do {
    if (AM_LIKELY(NULL == m_periodic_mjpeg_config)) {
      m_periodic_mjpeg_config = new AMMuxerCodecPeriodicMjpegConfig();
    }
    if (AM_UNLIKELY(!m_periodic_mjpeg_config)) {
      ERROR("Failed to create AMMuxerCodecPeriodicMjpegConfig struct.");
      break;
    }
    delete m_config;
    m_config = AMConfig::create(config_file.c_str());
    if (AM_LIKELY(m_config)) {
      AMConfig &config = *m_config;
      /*smallest_free_space*/
      if (AM_LIKELY(config["smallest_free_space"].exists())) {
        m_periodic_mjpeg_config->smallest_free_space =
            config["smallest_free_space"].get<unsigned>(20);
      } else {
        NOTICE("\"smallest_free_space\" is not specified, use default!");
        m_periodic_mjpeg_config->smallest_free_space = 20;
      }
      /*file_name_prefix*/
      if (AM_LIKELY(config["file_name_prefix"].exists())) {
        m_periodic_mjpeg_config->file_name_prefix =
            config["file_name_prefix"].get<std::string>("S2L");
      } else {
        NOTICE("\"file_name_prefix\" is not specified, use default!");
        m_periodic_mjpeg_config->file_name_prefix = "S2L";
      }
      /*file_location*/
      if (AM_LIKELY(config["file_location"].exists())) {
        m_periodic_mjpeg_config->file_location =
            config["file_location"].get<std::string>("/storage/sda1/video");
      } else {
        NOTICE("\"file_location\" is not specified, use default!");
        m_periodic_mjpeg_config->file_location = "/storage/sda1/video";
      }
      /*file_location_auto_parse*/
      if(AM_LIKELY(config["file_location_auto_parse"].exists())) {
        m_periodic_mjpeg_config->file_location_auto_parse =
            config["file_location_auto_parse"].get<bool>(true);
      } else {
        NOTICE("\"file_location_auto_parse\" is not specified, use default!");
        m_periodic_mjpeg_config->file_location_auto_parse = true;
      }
      /*muxer_id*/
      if (AM_LIKELY(config["muxer_id"].exists())) {
        m_periodic_mjpeg_config->muxer_id =
            config["muxer_id"].get<unsigned>(10);
      } else {
        NOTICE("\"muxer_id\" is not specified, use 10 as default!");
        m_periodic_mjpeg_config->muxer_id = 10;
      }
      /*max file size*/
      if (AM_LIKELY(config["max_file_size"].exists())) {
        m_periodic_mjpeg_config->max_file_size =
            config["max_file_size"].get<unsigned>(1024);
      } else {
        NOTICE("\"max_file_size\" is not specified, use 1024 as default!");
        m_periodic_mjpeg_config->max_file_size = 1024;
      }
      /*video_id*/
      if (AM_LIKELY(config["video_id"].exists())) {
        m_periodic_mjpeg_config->video_id =
            config["video_id"].get<unsigned>(0);
      } else {
        NOTICE("\"video_id\" is not specified, use default!");
        m_periodic_mjpeg_config->video_id = 0;
      }
      /*auto_file_writing*/
      if(AM_LIKELY(config["auto_file_writing"].exists())) {
        m_periodic_mjpeg_config->auto_file_writing =
            config["auto_file_writing"].get<bool>(true);
      } else {
        NOTICE("\"auto_file_writing\" is not specified, use default.");
        m_periodic_mjpeg_config->auto_file_writing = true;
      }
    } else {
      ERROR("Failed to create AMConfig object.");
      break;
    }
    config = m_periodic_mjpeg_config;
  } while (0);

  return config;
}


