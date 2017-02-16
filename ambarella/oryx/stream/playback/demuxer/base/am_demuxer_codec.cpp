/*******************************************************************************
 * am_demuxer_codec.cpp
 *
 * History:
 *   2014-9-1 - [ypchang] created file
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
#include "am_amf_packet.h"

#include "am_playback_info.h"
#include "am_demuxer_codec_if.h"
#include "am_demuxer_codec.h"
#include "am_file.h"
#include "am_mutex.h"

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
  AUTO_SPIN_LOCK(m_lock);
  if (AM_LIKELY(m_packet_pool && (m_is_enabled != enabled))) {
    m_is_enabled = enabled;
    m_packet_pool->enable(enabled);
    NOTICE("%s Demuxer %d!", enabled ? "Enable" : "Disable", m_demuxer_type);
  }
  if (AM_LIKELY(!enabled)) {
    delete m_media;
    m_media = NULL;
    while (!is_play_list_empty()) {
      delete m_play_list->front();
      m_play_list->pop();
    }
  }
}

bool AMDemuxer::is_play_list_empty()
{
  return m_play_list->empty() && !m_media;
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
  AUTO_SPIN_LOCK(m_lock);
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
    m_demuxer_type(type),
    m_contents(AM_DEMUXER_CONTENT_NONE),
    m_stream_id(streamid),
    m_file_size(0),
    m_is_enabled(false),
    m_media(NULL),
    m_lock(NULL),
    m_play_list(NULL),
    m_packet_pool(NULL)
{}

AMDemuxer::~AMDemuxer()
{
  if (AM_LIKELY(m_play_list)) {
    while(!m_play_list->empty()) {
      delete m_play_list->front();
      m_play_list->pop();
    }
    delete m_play_list;
  }
  delete m_media;
  if (AM_LIKELY(m_packet_pool)) {
    m_packet_pool->release();
  }
  AM_DESTROY(m_lock);
  DEBUG("~AMDemuxer");
}

AM_STATE AMDemuxer::init()
{
  m_play_list = new FileQueue();
  m_lock = AMSpinLock::create();
  return (m_play_list && m_lock) ? AM_STATE_OK : AM_STATE_NO_MEMORY;
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
      packet = NULL;
    }
  }

  return (packet != NULL);
}

AMFile* AMDemuxer::get_new_file()
{
  AMFile *file = NULL;
  while ((NULL == file) && !m_play_list->empty()) {
    file = m_play_list->front();
    m_play_list->pop();
    if (AM_UNLIKELY(file && !file->exists())) {
      ERROR("%s does not exist!", file->name());
      delete file;
      file = NULL;
      m_file_size = 0;
    } else {
      m_file_size = file->size();
      NOTICE("File %s is open, size: %llu bytes!", file->name(), m_file_size);
    }
  }

  return file;
}
