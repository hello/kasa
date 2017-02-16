/*******************************************************************************
 * am_muxer_config.cpp
 *
 * History:
 *   2014-12-26 - [ypchang] created file
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

#include "am_amf_types.h"
#include "am_amf_interface.h"
#include "am_muxer_if.h"

#include "am_muxer_config.h"
#include "am_configure.h"

AMMuxerConfig::AMMuxerConfig() :
  m_config(NULL),
  m_muxer_config(NULL)
{}

AMMuxerConfig::~AMMuxerConfig()
{
  delete m_config;
  delete m_muxer_config;
}

MuxerConfig* AMMuxerConfig::get_config(const std::string& conf)
{
  MuxerConfig *config = NULL;

  do {
    if (AM_LIKELY(!m_muxer_config)) {
      m_muxer_config = new MuxerConfig();
    }

    if (AM_UNLIKELY(!m_muxer_config)) {
      ERROR("Failed to create MuxerConfig object!");
      break;
    }

    delete m_config;
    m_config = AMConfig::create(conf.c_str());
    if (AM_LIKELY(m_config)) {
      AMConfig &muxer = *m_config;

      if (AM_LIKELY(muxer["name"].exists())) {
        m_muxer_config->name = muxer["name"].get<std::string>("Muxer");
      } else {
        NOTICE("\"name\" is not specified for this filter, use default!");
        m_muxer_config->name = "Muxer";
      }

      if (AM_LIKELY(muxer["rt_config"].exists())) {
        m_muxer_config->real_time.enabled =
            muxer["rt_config"]["enabled"].get<bool>(false);
        m_muxer_config->real_time.priority =
            muxer["rt_config"]["priority"].get<unsigned>(10);
      } else {
        NOTICE("Thread priority configuration is not found, "
               "use default value(Realtime thread: disabled)!");
        m_muxer_config->real_time.enabled = false;
        m_muxer_config->real_time.priority = 10;
      }

      if (AM_LIKELY(muxer["muxer_type"].exists())) {
        std::string type = muxer["muxer_type"].get<std::string>("");
        if (is_str_equal(type.c_str(), "direct")) {
          m_muxer_config->type = AM_MUXER_TYPE_DIRECT;
        } else if (is_str_equal(type.c_str(), "file")) {
          m_muxer_config->type = AM_MUXER_TYPE_FILE;
        } else {
          ERROR("Unknown muxer type: %s", type.c_str());
          m_muxer_config->type = AM_MUXER_TYPE_NONE;
        }
      } else {
        NOTICE("\"muxer_type\" is not specified!, use default!");
        m_muxer_config->type = AM_MUXER_TYPE_NONE;
      }

      if (AM_LIKELY(muxer["media_type"].exists())) {
        uint32_t &media_type_num = m_muxer_config->media_type_num;
        media_type_num = muxer["media_type"].length();
        if (AM_LIKELY(media_type_num)) {
          uint32_t num = 0;
          std::string media_type[media_type_num];
          for (uint32_t i = 0; i < media_type_num; ++ i) {
            std::string temp = muxer["media_type"][i].get<std::string>("rtp");
            /* Remove redundant media types */
            for (uint32_t j = 0; j < media_type_num; ++ j) {
              if (AM_LIKELY(!media_type[j].empty() &&
                            (media_type[j] == temp))) {
                NOTICE("%s is already added!", temp.c_str());
              } else if (AM_LIKELY(media_type[j].empty())) {
                media_type[j] = temp;
                ++ num;
                break;
              }
            }
          }
          media_type_num = num;
          m_muxer_config->media_type = new std::string[media_type_num];
          if (AM_LIKELY(m_muxer_config->media_type)) {
            for (uint32_t i = 0; i < media_type_num; ++ i) {
              m_muxer_config->media_type[i] = media_type[i];
            }
          } else {
            ERROR("Failed to allocate memory for media type!");
            media_type_num = 0;
            break;
          }
        } else {
          ERROR("Media type is not specified!");
          break;
        }
      } else {
        ERROR("\"media_type\" is not specified!");
        break;
      }

      config = m_muxer_config;
    } else {
      ERROR("Failed to load configuration file: %s", conf.c_str());
    }
  }while(0);

  return config;
}
