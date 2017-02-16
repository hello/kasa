/*******************************************************************************
 * am_file_demuxer_object.cpp
 *
 * History:
 *   2014-9-5 - [ypchang] created file
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

#include "am_demuxer_codec_if.h"
#include "am_file_demuxer_object.h"
#include "am_plugin.h"

AMFileDemuxerObject::AMFileDemuxerObject(const std::string& demuxer) :
  m_plugin(NULL),
  m_demuxer(demuxer),
  m_type(AM_DEMUXER_UNKNOWN),
  get_demuxer_codec(NULL)
{
}

AMFileDemuxerObject::~AMFileDemuxerObject()
{
  close();
}

bool AMFileDemuxerObject::open()
{
  bool ret = false;
  m_plugin = AMPlugin::create(m_demuxer.c_str());
  if (AM_LIKELY(m_plugin)) {
    DemuxerCodecType get_demuxer_codec_type =
        (DemuxerCodecType)m_plugin->get_symbol(DEMUXER_CODEC_TYPE);
    if (AM_LIKELY(get_demuxer_codec_type)) {
      m_type = get_demuxer_codec_type();
      switch(m_type) {
        case AM_DEMUXER_AAC:
        case AM_DEMUXER_WAV:
        case AM_DEMUXER_OGG:
        case AM_DEMUXER_MP4:
        case AM_DEMUXER_TS:
        case AM_DEMUXER_ES:
        case AM_DEMUXER_RTP: {
          ret = true;
        }break;
        case AM_DEMUXER_INVALID:
        case AM_DEMUXER_UNKNOWN:
        default: {
          ret = false;
          ERROR("Unknown codec type!");
        }break;
      }
    }
    if (AM_LIKELY(ret)) {
      get_demuxer_codec =
          (DemuxerCodecNew)m_plugin->get_symbol(DEMUXER_CODEC_NEW);
      ret = (get_demuxer_codec != NULL);
    }
  }

  return ret;
}

void AMFileDemuxerObject::close()
{
  if (AM_LIKELY(m_plugin)) {
    m_plugin->destroy();
    m_plugin = NULL;
  }
}
