/*******************************************************************************
 * am_audio_source_codec.cpp
 *
 * History:
 *   2014-12-8 - [ypchang] created file
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
#include "am_amf_queue.h"
#include "am_amf_base.h"
#include "am_amf_packet.h"
#include "am_amf_packet_filter.h"
#include "am_amf_packet_pin.h"
#include "am_amf_packet_pool.h"

#include "am_audio_source_if.h"
#include "am_audio_source_codec.h"
#include "am_audio_source.h"
#include "am_audio_codec_if.h"
#include "am_plugin.h"
#include "am_thread.h"

#include <unistd.h>

#ifdef BUILD_AMBARELLA_ORYX_CODEC_DIR
#define ORYX_CODEC_DIR ((const char*)BUILD_AMBARELLA_ORYX_CODEC_DIR)
#else
#define ORYX_CODEC_DIR ((const char*)"/usr/lib/oryx/codec")
#endif

#ifdef BUILD_AMBARELLA_ORYX_CONF_DIR
#define ORYX_CODEC_CONF_DIR \
  (const char*)(BUILD_AMBARELLA_ORYX_CONF_DIR"/stream/codec")
#else
#define ORYX_CODEC_CONF_DIR ((const char*)"/etc/oryx/stream/codec")
#endif

AMAudioCodecObj::AMAudioCodecObj() :
  m_codec(NULL),
  m_plugin(NULL),
  m_pool(NULL),
  m_codec_info(NULL),
  m_thread(NULL),
  m_audio_source(NULL),
  m_last_sent_pts(0),
  m_id((uint32_t)-1),
  m_codec_type((uint16_t)-1),
  m_run(true)
{
  memset(&m_codec_required_info, 0, sizeof(m_codec_required_info));
  m_name.clear();
}

AMAudioCodecObj::~AMAudioCodecObj()
{
  stop();
  clear();
}

bool AMAudioCodecObj::is_valid()
{
  return (NULL != m_codec);
}

bool AMAudioCodecObj::is_running()
{
  return (NULL != m_codec) && (m_thread && m_thread->is_running());
}

bool AMAudioCodecObj::load_codec(std::string& name)
{
  bool ret = false;
  std::string codec = ORYX_CODEC_DIR;

  codec.append("/codec-").append(name).append(".so");
  m_plugin = AMPlugin::create(codec.c_str());

  if (AM_LIKELY(m_plugin)) {
    AudioCodecNew get_audio_codec =
        (AudioCodecNew)m_plugin->get_symbol(AUDIO_CODEC_NEW);

    if (AM_LIKELY(get_audio_codec)) {
      std::string codecConf  = ORYX_CODEC_CONF_DIR;
      codecConf.append("/codec-").append(name).append(".acs");

      m_codec = get_audio_codec(codecConf.c_str());
      if (AM_LIKELY(m_codec)) {
        AM_AUDIO_INFO &required = m_codec_required_info;

        if (AM_LIKELY(m_codec->get_encode_required_src_parameter(required))) {
          NOTICE("Audio codec %s is loaded!", name.c_str());
          m_name = m_codec->get_codec_name();
          ret = true;
        } else {
          ERROR("Failed to get codec %s required audio parameters!",
                name.c_str());
        }
      } else {
        ERROR("Failed to load audio codec %s!", name.c_str());
      }
    } else {
      ERROR("Failed to get symbol (%s) from %s!",
            AUDIO_CODEC_NEW, codec.c_str());
    }
  }
  if (AM_UNLIKELY(!ret)) {
    finalize();
  }

  return ret;
}

bool AMAudioCodecObj::initialize(AM_AUDIO_INFO *src_audio_info,
                                 uint32_t id,
                                 uint32_t packet_pool_size)
{
  bool ret = false;

  if (AM_LIKELY(m_codec)) {
    m_name = m_codec->get_codec_name();
    if (AM_UNLIKELY(!m_codec->initialize(src_audio_info,
                                         AM_AUDIO_CODEC_MODE_ENCODE))) {
      ERROR("Failed to initialize codec %s, abort!", m_name.c_str());
    } else {
      std::string poolName = m_name + "packet.pool";
      m_codec_info = m_codec->get_codec_audio_info();
      m_pool = AMFixedPacketPool::create(poolName.c_str(),
                                         packet_pool_size + 1,
                                         m_codec_info->chunk_size + 512);
      m_codec_info->pkt_pts_increment = src_audio_info->pkt_pts_increment;
      m_codec_info->chunk_size = src_audio_info->chunk_size;
      if (AM_UNLIKELY(!m_pool)) {
        ERROR("Failed to create packet pool for %s, abort!", m_name.c_str());
      } else {
        m_id = id;
        m_codec_type = (uint16_t)m_codec->get_codec_type();
        NOTICE("Audio codec %s(%hu) is initialized!", m_name.c_str(),
               m_codec_type);
        ret = true;
      }
    }
  }

  return ret;
}

void AMAudioCodecObj::finalize()
{
  AM_DESTROY(m_codec);
  AM_DESTROY(m_plugin);
  AM_DESTROY(m_pool);
  AM_DESTROY(m_thread);
}

void AMAudioCodecObj::push_queue(AMPacket *&packet)
{
  m_packet_q.push(packet);
}

void AMAudioCodecObj::clear()
{
  while(!m_packet_q.empty()) {
    m_packet_q.front()->release();
    m_packet_q.pop();
  }
  finalize();
}

bool AMAudioCodecObj::start(bool RTPrio, uint32_t priority)
{
  bool ret = false;
  if (AM_LIKELY(m_codec)) {
    std::string threadName = m_name + ".encode";
    m_pool->enable(true);
    m_thread = AMThread::create(threadName.c_str(),
                                static_encode,
                                this);
    ret = (NULL != m_thread);
    if (AM_LIKELY(ret)) {
      if (AM_LIKELY(RTPrio)) {
        if (AM_UNLIKELY(false == (ret = m_thread->set_priority(priority)))) {
          ERROR("Failed to set RT priority to thread %s!", threadName.c_str());
        }
      }
    }
    m_run = ret;
  }

  return ret;
}

void AMAudioCodecObj::stop()
{
  m_run = false;
  if (AM_LIKELY(m_pool)) {
    m_pool->enable(false);
  }
  AM_DESTROY(m_thread);
}

void AMAudioCodecObj::static_encode(void *data)
{
  ((AMAudioCodecObj*)data)->encode();
}

void AMAudioCodecObj::encode()
{
  AMPacket *info = NULL;
  bool info_sent = false;

  INFO("%s starts to run!", m_name.c_str());
  if (AM_LIKELY(m_pool->get_avail_packet_num() > 0)) {
    if (AM_LIKELY(m_pool->alloc_packet(info, sizeof(*info)))) {
      uint8_t *data = info->get_data_ptr();
      memcpy(data, m_codec_info, sizeof(*m_codec_info));
      info->set_type(AMPacket::AM_PAYLOAD_TYPE_INFO);
      info->set_attr(AMPacket::AM_PAYLOAD_ATTR_AUDIO);
      info->set_data_size(sizeof(*m_codec_info));
      info->set_stream_id((uint16_t)m_id);
      info->set_frame_type(m_codec_info->type);
      m_audio_source->send_packet(info);
      info_sent = true;
    }
  }

  if (AM_UNLIKELY(!info_sent)) {
    ERROR("Failed to send audio information! ABORT %s!",
          m_codec->get_codec_name().c_str());
    m_run = false;
  }

  while(m_run) {
    AMPacket *data   = NULL;
    AMPacket *packet = NULL;
    if (AM_LIKELY(m_packet_q.empty())) {
      usleep(5000);
      continue;
    }
    data = m_packet_q.front();
    m_packet_q.pop();
    if (AM_UNLIKELY(!data)) {
      continue;
    }
    if (AM_LIKELY(m_pool->get_avail_packet_num() > 1)) {
      if (AM_UNLIKELY(!m_pool->alloc_packet(packet, sizeof(*packet)))) {
        NOTICE("%s is disabled! Stop encoding!", m_pool->get_name());
        break;
      } else {
        AM_AUDIO_INFO &required = m_codec_required_info;
        uint32_t count     = data->get_data_size() / required.chunk_size;
        uint32_t total_out = 0;
        bool encodeOk      = (count > 0);

        packet->set_attr(AMPacket::AM_PAYLOAD_ATTR_AUDIO);
        packet->set_frame_count(count);
        for (uint32_t i = 0; i < count; ++ i) {
          uint32_t out_size = 0;
          uint8_t *input = data->get_data_ptr() + i * required.chunk_size;
          if (AM_LIKELY(m_codec->encode(input, required.chunk_size,
                                        packet->get_data_ptr() + total_out,
                                        &out_size))) {
            total_out += out_size;
          } else {
            encodeOk = false;
            ERROR("%s encode error!", m_name.c_str());
            break;
          }
        }
        if (AM_LIKELY(encodeOk)) {
          m_last_sent_pts = data->get_pts();
          packet->set_data_size(total_out);
          packet->set_pts(m_last_sent_pts);
          packet->set_stream_id(m_id);
          packet->set_type(AMPacket::AM_PAYLOAD_TYPE_DATA);
          packet->set_frame_type(m_codec_info->type);
          m_audio_source->send_packet(packet);
        } else {
          packet->release();
          m_run = false;
          if (AM_UNLIKELY(!count)) {
            ERROR("Received 0 bytes raw audio data!");
          }
        }
      }
    } else {
      NOTICE("Not enough packet! Drop %u bytes of audio! Is IO too slow?",
             data->get_data_size());
      usleep(5000);
    }
    data->release();
  } /* while(m_run) */

  if (AM_LIKELY(info_sent && !m_run)) {
    AMPacket *eos = NULL;
    if (AM_LIKELY((m_pool->get_avail_packet_num() > 0) &&
                  m_pool->alloc_packet(eos, sizeof(*eos)))) {
      eos->set_data_size(0);
      eos->set_pts(m_last_sent_pts + m_codec_required_info.pkt_pts_increment);
      eos->set_stream_id(m_id);
      eos->set_type(AMPacket::AM_PAYLOAD_TYPE_EOS);
      eos->set_attr(AMPacket::AM_PAYLOAD_ATTR_AUDIO);
      eos->set_frame_type(m_codec_info->type);
      m_audio_source->send_packet(eos);
      INFO("%s has sent EOS!", m_name.c_str());
    }
  }
  INFO("%s exits encode loop!", m_name.c_str());
}
