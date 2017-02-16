/*******************************************************************************
 * am_audio_codec_g726_config.cpp
 *
 * History:
 *   2014-11-12 - [ypchang] created file
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
#include "am_audio_type.h"

#include "am_audio_codec_g726_config.h"
#include "am_configure.h"

AMAudioCodecG726Config::AMAudioCodecG726Config() :
  m_config(NULL),
  m_g726_config(NULL)
{}

AMAudioCodecG726Config::~AMAudioCodecG726Config()
{
  delete m_config;
  delete m_g726_config;
}

AudioCodecG726Config* AMAudioCodecG726Config::get_config(
    const std::string& conf_file)
{
  AudioCodecG726Config *config = NULL;
  do {
    if (AM_LIKELY(!m_g726_config)) {
      m_g726_config = new AudioCodecG726Config();
    }
    if (AM_UNLIKELY(!m_g726_config)) {
      ERROR("Failed to create AudioCodecG726Config object!");
      break;
    }
    delete m_config;
    m_config = AMConfig::create(conf_file.c_str());
    if (AM_LIKELY(m_config)) {
      AMConfig &g726 = *m_config;

      if (AM_LIKELY(g726["decode_law"].exists())) {
        std::string decode_law = g726["decode_law"].get<std::string>("pcm16");
        if (is_str_equal(decode_law.c_str(), "pcm16")) {
          m_g726_config->decode_law = AM_SAMPLE_S16LE;
        } else if (is_str_equal(decode_law.c_str(), "alaw")) {
          m_g726_config->decode_law = AM_SAMPLE_ALAW;
        } else if (is_str_equal(decode_law.c_str(), "ulaw")) {
          m_g726_config->decode_law = AM_SAMPLE_ULAW;
        } else {
          NOTICE("Unsupported decoding law: %s, use default!",
                 decode_law.c_str());
          m_g726_config->decode_law = AM_SAMPLE_S16LE;
        }
      } else {
        NOTICE("\"decode_law\" is not specified, use default!");
        m_g726_config->decode_law = AM_SAMPLE_S16LE;
      }

      if (AM_LIKELY(g726["encode_rate"].exists())) {
        m_g726_config->encode_rate =
            G726_RATE(g726["encode_rate"].get<unsigned>(32));
        switch(m_g726_config->encode_rate) {
          case G726_16K:
          case G726_24K:
          case G726_32K:
          case G726_40K:break;
          default: {
            NOTICE("Invalid encode rate %u, reset to default(32)!",
                   m_g726_config->encode_rate);
            m_g726_config->encode_rate = G726_32K;
          }break;
        }
      } else {
        NOTICE("\"encode_rate\" is not specified, use default!");
        m_g726_config->encode_rate = G726_32K;
      }

      if (AM_LIKELY(g726["encode_frame_time_length"].exists())) {
        m_g726_config->encode_frame_time_length =
            g726["encode_frame_time_length"].get<unsigned>(80);
      } else {
        NOTICE("\"encode_frame_time_length\" is not specified, use default!");
        m_g726_config->encode_frame_time_length = 80;
      }

    } else {
      ERROR("Failed to load configuration file: %s!", conf_file.c_str());
      break;
    }
    config = m_g726_config;
  } while(0);

  return config;
}
