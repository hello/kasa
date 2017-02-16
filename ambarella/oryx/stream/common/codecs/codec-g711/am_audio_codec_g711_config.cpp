/*******************************************************************************
 * am_audio_codec_g711_config.cpp
 *
 * History:
 *   2015-1-28 - [ypchang] created file
 *
 * Copyright (C) 2008-2015, Ambarella Co, Ltd.
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
#include "am_audio_type.h"

#include "am_audio_codec_g711_config.h"
#include "am_configure.h"

AMAudioCodecG711Config::AMAudioCodecG711Config() :
  m_config(NULL),
  m_g711_config(NULL)
{}

AMAudioCodecG711Config::~AMAudioCodecG711Config()
{
  delete m_config;
  delete m_g711_config;
}

AudioCodecG711Config* AMAudioCodecG711Config::get_config(
    const std::string& conf_file)
{
  AudioCodecG711Config *config = NULL;
  do {
    if (AM_LIKELY(!m_g711_config)) {
      m_g711_config = new AudioCodecG711Config();
    }
    if (AM_UNLIKELY(!m_g711_config)) {
      ERROR("Failed to create AudioCodecG711Config object!");
      break;
    }
    delete m_config;
    m_config = AMConfig::create(conf_file.c_str());
    if (AM_LIKELY(m_config)) {
      AMConfig &g711 = *m_config;

      if (AM_LIKELY(g711["encode_law"].exists())) {
        std::string encode_law = g711["encode_law"].get<std::string>("alaw");
        if (is_str_equal(encode_law.c_str(), "alaw")) {
          m_g711_config->encode_law = AM_SAMPLE_ALAW;
        } else if (is_str_equal(encode_law.c_str(), "ulaw")) {
          m_g711_config->encode_law = AM_SAMPLE_ULAW;
        } else {
          NOTICE("Unsupported decoding law: %s, use default!",
                 encode_law.c_str());
          m_g711_config->encode_law = AM_SAMPLE_ALAW;
        }
      } else {
        NOTICE("\"encode_law\" is not specified, use default!");
        m_g711_config->encode_law = AM_SAMPLE_ALAW;
      }


      if (AM_LIKELY(g711["encode_rate"].exists())) {
        m_g711_config->encode_rate =
            g711["encode_rate"].get<unsigned>(16000);
      } else {
        NOTICE("\"encode_rate\" is not specified, use default!");
        m_g711_config->encode_rate = 16000;
      }

      if (AM_LIKELY(g711["encode_frame_time_length"].exists())) {
        m_g711_config->encode_frame_time_length =
            g711["encode_frame_time_length"].get<unsigned>(80);
      } else {
        NOTICE("\"encode_frame_time_length\" is not specified, use default!");
        m_g711_config->encode_frame_time_length = 80;
      }

    } else {
      ERROR("Failed to load configuration file: %s!", conf_file.c_str());
      break;
    }
    config = m_g711_config;
  } while(0);

  return config;
}
