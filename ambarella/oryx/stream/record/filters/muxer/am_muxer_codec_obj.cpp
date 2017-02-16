/*******************************************************************************
 * am_muxer_codec_obj.cpp
 *
 * History:
 *   2014-12-29 - [ypchang] created file
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

#include "am_muxer_codec_obj.h"
#include "am_muxer_codec_if.h"

#include "am_plugin.h"

AMMuxerCodecObj::AMMuxerCodecObj() :
  m_codec(NULL),
  m_plugin(NULL)
{
  m_name.clear();
}

AMMuxerCodecObj::~AMMuxerCodecObj()
{
  destroy();
}

bool AMMuxerCodecObj::is_valid()
{
  return (NULL != m_codec);
}

bool AMMuxerCodecObj::load_codec(std::string& name)
{
  bool ret = false;
  std::string codec = ORYX_MUXER_DIR;
  codec.append("/muxer-").append(name).append(".so");

  m_plugin = AMPlugin::create(codec.c_str());

  if (AM_LIKELY(m_plugin)) {
    MuxerCodecNew get_muxer_codec =
        (MuxerCodecNew)m_plugin->get_symbol(MUXER_CODEC_NEW);
    if (AM_LIKELY(get_muxer_codec)) {
      std::string codecConf = ORYX_MUXER_CONF_DIR;
      codecConf.append("/muxer-").append(name).append(".acs");

      m_codec = get_muxer_codec(codecConf.c_str());
      if (AM_UNLIKELY(!m_codec)) {
        ERROR("Failed to load muxer %s", name.c_str());
      } else {
        NOTICE("Muxer plugin %s is loaded!", name.c_str());
        m_name = name;
        ret = true;
      }
    } else {
      ERROR("Failed to get symbol (%s) from %s!",
            MUXER_CODEC_NEW, codec.c_str());
    }
  }

  if (AM_UNLIKELY(!ret)) {
    destroy();
  }

  return ret;
}

void AMMuxerCodecObj::destroy()
{
  if (AM_LIKELY(m_codec)) {
    m_codec->stop();
  }
  m_name.clear();
  delete m_codec;
  m_codec = NULL;
  AM_DESTROY(m_plugin);
}
