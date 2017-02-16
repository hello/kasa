/*******************************************************************************
 * am_demuxer_aac.cpp
 *
 * History:
 *   2014-10-27 - [ypchang] created file
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

#include "am_file.h"
#include "am_mutex.h"

#include "am_amf_types.h"
#include "am_amf_interface.h"
#include "am_amf_queue.h"
#include "am_amf_base.h"
#include "am_amf_packet.h"
#include "am_amf_packet_pool.h"

#include "am_demuxer_codec_if.h"
#include "am_demuxer_codec.h"
#include "am_demuxer_aac.h"
#include "am_audio_type.h"
#include "am_audio_define.h"

#include "adts.h"

/* From sample code of libaacenc.a */
#define AAC_AUDIO_DATA_SIZE (6144 / 8 * 2 + 100)
#define PACKET_POOL_SIZE    32
#define FILE_BUFFER_SIZE    4096

uint32_t index_to_freq[] =
{
  96000,
  88200,
  64000,
  48000,
  44100,
  32000,
  24000,
  22050,
  16000,
  12000,
  11025,
  8000,
  7350,
  0,
  0,
  0
};

AM_DEMUXER_TYPE get_demuxer_codec_type()
{
  return AM_DEMUXER_AAC;
}

AMIDemuxerCodec* get_demuxer_codec(uint32_t streamid)
{
  return AMDemuxerAac::create(streamid);
}

AMIDemuxerCodec* AMDemuxerAac::create(uint32_t streamid)
{
  AMDemuxerAac *demuxer = new AMDemuxerAac(streamid);
  if (AM_UNLIKELY(demuxer && (AM_STATE_OK != demuxer->init()))) {
    delete demuxer;
    demuxer = NULL;
  }

  return ((AMIDemuxerCodec*)demuxer);
}

AM_DEMUXER_STATE AMDemuxerAac::get_packet(AMPacket *&packet)
{
  AM_DEMUXER_STATE state = AM_DEMUXER_OK;

  packet = NULL;
  while (!packet) {
    if (AM_UNLIKELY(!m_media)) {
      m_need_to_read = true;
      m_avail_size = 0;
      m_sent_size = 0;
      m_is_new_file = (NULL != (m_media = get_new_file()));
      if (AM_UNLIKELY(!m_media)) {
        state = AM_DEMUXER_NO_FILE;
        break;
      } else {
        m_remain_size = m_file_size;
      }
    }

    if (AM_LIKELY(m_media && (m_media->is_open() ||
                              m_media->open(AMFile::AM_FILE_READONLY)))) {
      AdtsHeader *adts = NULL;
      if (AM_LIKELY(m_need_to_read)) {
        ssize_t readSize = m_media->read(m_buffer, FILE_BUFFER_SIZE);
        if (AM_UNLIKELY(readSize <= 0)) {
          if (readSize < 0) {
            ERROR("%s: %s! Skip!", m_media->name(), strerror(errno));
          } else {
            INFO("%s EOF", m_media->name());
          }
          delete m_media;
          m_media = NULL;
          continue;
        } else {
          m_sent_size = 0;
          m_avail_size = readSize;
          m_need_to_read = false;
        }
      }
      adts = (AdtsHeader*) &m_buffer[m_sent_size];
      if (AM_LIKELY((m_avail_size > sizeof(AdtsHeader)) &&
                    adts->is_sync_word_ok() &&
                    (m_avail_size >= adts->frame_length()))) {
        if (AM_LIKELY(allocate_packet(packet))) {
          packet->set_attr(AMPacket::AM_PAYLOAD_ATTR_AUDIO);
          packet->set_frame_type(AM_AUDIO_CODEC_AAC);
          packet->set_stream_id(m_stream_id);
          packet->set_pts(0LL);
          if (AM_UNLIKELY(m_is_new_file)) {
            AM_AUDIO_INFO *aInfo = ((AM_AUDIO_INFO*) packet->get_data_ptr());
            aInfo->channels = adts->aac_channel_conf();
            aInfo->sample_rate = index_to_freq[adts->aac_frequency_index()];
            aInfo->type = AM_AUDIO_AAC;
            aInfo->chunk_size = AAC_AUDIO_DATA_SIZE;
            packet->set_type(AMPacket::AM_PAYLOAD_TYPE_INFO);
            packet->set_data_size(sizeof(AM_AUDIO_INFO));
            m_is_new_file = false;
          } else {
            uint16_t framelen = adts->frame_length();
            memcpy(packet->get_data_ptr(), &m_buffer[m_sent_size], framelen);
            m_sent_size += framelen;
            m_avail_size -= framelen;
            m_remain_size -= framelen;
            packet->set_type(AMPacket::AM_PAYLOAD_TYPE_DATA);
            packet->set_data_size(framelen);
            packet->set_data_offset(0);
            m_need_to_read = (m_avail_size == 0);
          }
        } else {
          state = AM_DEMUXER_NO_PACKET;
          break;
        }
      } else {
        uint32_t skipped = 0;
        while((m_avail_size >= sizeof(AdtsHeader)) &&
              !adts->is_sync_word_ok()) {
          -- m_avail_size;
          -- m_remain_size;
          ++ m_sent_size;
          ++ skipped;
          adts = (AdtsHeader*)&m_buffer[m_sent_size];
        }
        if (AM_LIKELY(skipped > 0)) {
          ERROR("Invalid ADTS, skipped %u bytes data!", skipped);
        }
        if (AM_LIKELY(m_remain_size <= sizeof(AdtsHeader))) {
          ERROR("%s is incomplete, remaining file size %llu is not bigger than "
                "ADTS header size %u",
                m_media->name(), m_remain_size, sizeof(AdtsHeader));
          delete m_media;
          m_media = NULL;
          continue;
        }
        if (AM_LIKELY((m_avail_size >= sizeof(AdtsHeader)) &&
                      adts->is_sync_word_ok() &&
                      (m_remain_size < adts->frame_length()))) {
          WARN("%s is incomplete, the last ADTS reports length is %u bytes, "
               "but available file length is %llu bytes!",
               m_media->name(), adts->frame_length(), m_remain_size);
          delete m_media;
          m_media = NULL;
          continue;
        }
        m_media->seek(-m_avail_size, AMFile::AM_FILE_SEEK_CUR);
        m_need_to_read = true;
      }
    }
  }

  return state;
}

void AMDemuxerAac::destroy()
{
  enable(false);
  inherited::destroy();
}


AMDemuxerAac::AMDemuxerAac(uint32_t streamid) :
    inherited(AM_DEMUXER_AAC, streamid),
    m_buffer(NULL),
    m_sent_size(0),
    m_avail_size(0),
    m_remain_size(0),
    m_need_to_read(true),
    m_is_new_file(true)
{}

AMDemuxerAac::~AMDemuxerAac()
{
  delete[] m_buffer;
  DEBUG("~AMDemuxerAac");
}

AM_STATE AMDemuxerAac::init()
{
  AM_STATE state = AM_STATE_OK;

  do {
    if (AM_UNLIKELY(AM_STATE_OK != (state = inherited::init()))) {
      break;
    }
    m_packet_pool = AMFixedPacketPool::create("AACDemuxerPacketPool",
                                              PACKET_POOL_SIZE,
                                              AAC_AUDIO_DATA_SIZE);
    if (AM_UNLIKELY(!m_packet_pool)) {
      ERROR("Failed to create packet pool for AAC demuxer!");
      state = AM_STATE_NO_MEMORY;
      break;
    }
    m_buffer = new char[FILE_BUFFER_SIZE];
    if (AM_UNLIKELY(!m_buffer)) {
      ERROR("Failed to create file read buffer!");
      state = AM_STATE_NO_MEMORY;
      break;
    }
    memset(m_buffer, 0, FILE_BUFFER_SIZE);

  }while(0);

  return state;
}
