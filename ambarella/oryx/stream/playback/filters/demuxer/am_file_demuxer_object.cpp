/*******************************************************************************
 * am_file_demuxer_object.cpp
 *
 * History:
 *   2014-9-5 - [ypchang] created file
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

#include "am_demuxer_codec_if.h"
#include "am_file_demuxer_object.h"
#include "am_plugin.h"

AMFileDemuxerObject::AMFileDemuxerObject(const std::string& demuxer) :
  m_plugin(nullptr),
  get_demuxer_codec(nullptr),
  m_type(AM_DEMUXER_UNKNOWN),
  m_demuxer(demuxer)
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
        case AM_DEMUXER_RTP:
        case AM_DEMUXER_UNIX_DOMAIN :{
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
  for (auto &v : m_codec_map) {
    AM_DESTROY(v.second);
  }
  m_codec_map.clear();
  if (AM_LIKELY(m_plugin)) {
    m_plugin->destroy();
    m_plugin = NULL;
  }
}

AMIDemuxerCodec* AMFileDemuxerObject::get_demuxer(uint32_t stream_id)
{
  AMFileDemuxerObject::DemuxerCodecMap::iterator iter =
      m_codec_map.find(stream_id);
  AMIDemuxerCodec *codec = nullptr;
  if (AM_LIKELY(iter == m_codec_map.end())) {
    codec = get_demuxer_codec(stream_id);
    if (AM_LIKELY(codec)) {
      m_codec_map[stream_id] = codec;
    }
  } else {
    codec = iter->second;
  }

  return codec;
}
