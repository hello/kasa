/*******************************************************************************
 * am_audio_source_codec.cpp
 *
 * History:
 *   2014-12-8 - [ypchang] created file
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
#include "am_event.h"

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

AMAudioCodecObj::AMAudioCodecObj()
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

  do {
    m_name = m_codec->get_codec_name() + "-" +
        std::to_string(src_audio_info->sample_rate / 1000) + "k";
    m_encode_event = AMEvent::create();
    if (AM_UNLIKELY(!m_encode_event)) {
      ERROR("Failed to create AMEvent object!");
      break;
    }
    if (AM_UNLIKELY(!m_codec)) {
      ERROR("Audio codec is not loaded!");
      break;
    }
    if (AM_UNLIKELY(!m_codec->initialize(src_audio_info,
                                         AM_AUDIO_CODEC_MODE_ENCODE))) {
      ERROR("Failed to initialize codec %s, abort!", m_name.c_str());
      break;
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
        break;
      }
      set_stream_id(id);
      m_codec_type = (uint16_t)m_codec->get_codec_type();
      NOTICE("Audio codec %s(%hu) is initialized!", m_name.c_str(),
             m_codec_type);
      ret = true;
    }
  } while(0);

  return ret;
}

void AMAudioCodecObj::finalize()
{
  AM_DESTROY(m_codec);
  AM_DESTROY(m_plugin);
  AM_DESTROY(m_pool);
  AM_DESTROY(m_thread);
  AM_DESTROY(m_encode_event);
}

void AMAudioCodecObj::push_queue(AMPacket *&packet)
{
  m_packet_q.push(packet);
}

void AMAudioCodecObj::clear()
{
  while(!m_packet_q.empty()) {
    m_packet_q.front_pop()->release();
  }
  finalize();
}

bool AMAudioCodecObj::start(bool RTPrio, uint32_t priority)
{
  bool ret = false;
  if (AM_LIKELY(m_codec)) {
    std::string threadName = m_name + ".encode";
    m_pool->enable(true);
    m_run = true; /* Make sure encode main loop will run */
    m_thread = AMThread::create(threadName.c_str(),
                                static_encode,
                                this);
    ret = (NULL != m_thread);
    m_run = ret;
    if (AM_LIKELY(ret)) {
      if (AM_LIKELY(RTPrio)) {
        if (AM_UNLIKELY(false == (ret = m_thread->set_priority(priority)))) {
          ERROR("Failed to set RT priority to thread %s!", threadName.c_str());
        }
      }
      m_encode_event->wait();
    }
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

void AMAudioCodecObj::set_stream_id(uint32_t id)
{
  m_id = id;
}

AM_AUDIO_TYPE AMAudioCodecObj::get_type()
{
  return m_codec_info->type;
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
  m_encode_event->signal();
  if (AM_LIKELY(m_pool->get_avail_packet_num() > 0)) {
    if (AM_LIKELY(m_pool->alloc_packet(info, sizeof(*info)))) {
      uint8_t *data = info->get_data_ptr();
      memcpy(data, m_codec_info, sizeof(*m_codec_info));
      AM_AUDIO_INFO *tmp_info = (AM_AUDIO_INFO*)data;
      tmp_info->chunk_size /= m_multiple;
      tmp_info->pkt_pts_increment /= m_multiple;
      info->set_type(AMPacket::AM_PAYLOAD_TYPE_INFO);
      info->set_attr(AMPacket::AM_PAYLOAD_ATTR_AUDIO);
      info->set_data_size(sizeof(*m_codec_info));
      info->set_stream_id((uint16_t)m_id);
      info->set_frame_type(m_codec_info->type);
      m_audio_source->send_packet(info);
      INFO("%s has sent INFO!", m_name.c_str());
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
    data = m_packet_q.front_pop();
    if (AM_UNLIKELY(!data)) {
      continue;
    }
    for(uint32_t i = 0; i < m_multiple; ++ i) {
      if (AM_LIKELY(m_pool->get_avail_packet_num() > 1)) {
        if (AM_UNLIKELY(!m_pool->alloc_packet(packet, sizeof(*packet)))) {
          NOTICE("%s is disabled! Stop encoding!", m_pool->get_name());
          break;
        } else {
          AM_AUDIO_INFO &required = m_codec_required_info;
          bool encodeOk      = (data->get_data_size() > 0);

          packet->set_attr(AMPacket::AM_PAYLOAD_ATTR_AUDIO);
          packet->set_frame_count(1);
            uint32_t out_size = 0;
            uint8_t *input = data->get_data_ptr() + i * required.chunk_size;
            if (AM_LIKELY(m_codec->encode(input, required.chunk_size,
                                          packet->get_data_ptr(),
                                          &out_size))) {
            } else {
              encodeOk = false;
              ERROR("%s encode error!", m_name.c_str());
              break;
            }
          if (AM_LIKELY(encodeOk)) {
            m_last_sent_pts = data->get_pts() -
                required.pkt_pts_increment * (m_multiple - i - 1);
            packet->set_data_size(out_size);
            packet->set_pts(m_last_sent_pts);
            packet->set_stream_id(m_id);
            packet->set_type(AMPacket::AM_PAYLOAD_TYPE_DATA);
            packet->set_frame_type(m_codec_info->type);
            packet->set_frame_attr(m_codec_info->sample_rate);
            m_audio_source->send_packet(packet);
          } else {
            packet->release();
            m_run = false;
            if (AM_UNLIKELY(data->get_data_size() <= 0)) {
              ERROR("Received 0 bytes raw audio data!");
            } else {
              ERROR("Encoding error!");
            }
          }
        }
      } else {
        NOTICE("Not enough packet! Drop %u bytes of audio! Is IO too slow?",
               data->get_data_size());
        usleep(5000);
      }
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
      eos->set_frame_attr(m_codec_info->sample_rate);
      m_audio_source->send_packet(eos);
      INFO("%s has sent EOS!", m_name.c_str());
    }
  }
  INFO("%s exits encode loop!", m_name.c_str());
}
