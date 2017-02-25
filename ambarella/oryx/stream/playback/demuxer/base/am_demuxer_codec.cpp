/*******************************************************************************
 * am_demuxer_codec.cpp
 *
 * History:
 *   2014-9-1 - [ypchang] created file
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

#include "am_amf_types.h"
#include "am_amf_interface.h"
#include "am_amf_packet.h"

#include "am_playback_info.h"
#include "am_demuxer_codec_if.h"
#include "am_demuxer_codec.h"
#include "am_file.h"

static const char* demuxer_type_to_string(AM_DEMUXER_TYPE type)
{
  const char *str = "Unknown";
  switch(type) {
    case AM_DEMUXER_INVALID:     str = "Invalid";    break;
    case AM_DEMUXER_UNKNOWN:     str = "Unknown";    break;
    case AM_DEMUXER_AAC:         str = "AAC";        break;
    case AM_DEMUXER_WAV:         str = "WAV";        break;
    case AM_DEMUXER_OGG:         str = "OGG";        break;
    case AM_DEMUXER_MP4:         str = "MP4";        break;
    case AM_DEMUXER_TS:          str = "TS";         break;
    case AM_DEMUXER_ES:          str = "ES";         break;
    case AM_DEMUXER_RTP:         str = "RTP";        break;
    case AM_DEMUXER_UNIX_DOMAIN: str = "UnixDomain"; break;
    default: break;
  }

  return str;
}

AM_DEMUXER_TYPE AMDemuxer::get_demuxer_type()
{
  return m_demuxer_type;
}

void AMDemuxer::destroy()
{
  delete this;
}

void AMDemuxer::enable(bool enabled)
{
  AUTO_MEM_LOCK(m_lock);
  if (AM_LIKELY(m_packet_pool && (m_is_enabled != enabled))) {
    m_is_enabled = enabled;
    m_packet_pool->enable(enabled);
    NOTICE("%s Demuxer %s!", enabled ? "Enable" : "Disable",
           demuxer_type_to_string(m_demuxer_type));
  }
  if (AM_LIKELY(!enabled)) {
    delete m_media;
    m_media = nullptr;
    while (!is_play_list_empty()) {
      delete m_play_list->front_pop();
    }
  }
}

bool AMDemuxer::is_play_list_empty()
{
  return m_play_list->empty() && !m_media;
}

bool AMDemuxer::is_drained()
{
  return true;
}

uint32_t AMDemuxer::stream_id()
{
  return m_stream_id;
}

uint32_t AMDemuxer::get_contents_type()
{
  return m_contents;
}

bool AMDemuxer::add_uri(const AMPlaybackUri& uri)
{
  AUTO_MEM_LOCK(m_lock);
  bool ret = true;
  do{
    /*this function has been rewritten in rtp demuxer*/
    if(uri.type != AM_PLAYBACK_URI_FILE) {
      ERROR("uri type error.");
      ret = false;
      break;
    }
    const std::string file_name = uri.media.file;
    ret = (!file_name.empty() && AMFile::exists(file_name.c_str()));
    if (AM_LIKELY(ret)) {
      m_play_list->push(new AMFile(file_name.c_str()));
    } else {
      if (file_name.empty()) {
        ERROR("Empty uri!");
      } else {
        ERROR("%s doesn't exist!", file_name.c_str());
      }
    }
  }while(0);
  return ret;
}

AM_DEMUXER_STATE AMDemuxer::get_packet(AMPacket*& packet)
{
  ERROR("Dummy demuxer codec, should be implemented in sub-class!");
  return AM_DEMUXER_NONE;
}

AMDemuxer::AMDemuxer(AM_DEMUXER_TYPE type, uint32_t streamid) :
    m_file_size(0),
    m_media(nullptr),
    m_play_list(nullptr),
    m_packet_pool(nullptr),
    m_demuxer_type(type),
    m_contents(AM_DEMUXER_CONTENT_NONE),
    m_stream_id(streamid),
    m_is_enabled(false)
{}

AMDemuxer::~AMDemuxer()
{
  INFO("Demuxer codec %s is destroyed!",
       demuxer_type_to_string(m_demuxer_type));
  if (AM_LIKELY(m_play_list)) {
    while(!m_play_list->empty()) {
      delete m_play_list->front_pop();
    }
    delete m_play_list;
  }
  delete m_media;
  if (AM_LIKELY(m_packet_pool)) {
    m_packet_pool->release();
  }
}

AM_STATE AMDemuxer::init()
{
  m_play_list = new FileQueue();
  return m_play_list? AM_STATE_OK : AM_STATE_NO_MEMORY;
}


uint32_t AMDemuxer::available_packet_num()
{
  return m_packet_pool->get_avail_packet_num();
}

bool AMDemuxer::allocate_packet(AMPacket*& packet)
{
  if (AM_LIKELY(m_packet_pool->get_avail_packet_num())) {
    if (AM_UNLIKELY(!m_packet_pool->alloc_packet(packet, sizeof(packet)))) {
      ERROR("Failed to allocate packet from %s", m_packet_pool->get_name());
      packet = nullptr;
    }
  }

  return (packet != nullptr);
}

AMFile* AMDemuxer::get_new_file()
{
  AMFile *file = nullptr;
  while ((nullptr == file) && !m_play_list->empty()) {
    file = m_play_list->front_pop();
    if (AM_UNLIKELY(file && !file->exists())) {
      ERROR("%s does not exist!", file->name());
      delete file;
      file = nullptr;
      m_file_size = 0;
    } else {
      m_file_size = file->size();
      NOTICE("File %s is open, size: %llu bytes!", file->name(), m_file_size);
    }
  }

  return file;
}
