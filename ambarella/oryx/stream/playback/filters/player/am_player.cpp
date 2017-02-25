/*******************************************************************************
 * am_player.cpp
 *
 * History:
 *   2014-9-10 - [ypchang] created file
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
#include "am_audio_define.h"
#include "am_player_if.h"
#include "am_player.h"
#include "am_player_config.h"
#include "am_player_version.h"

#include "am_mutex.h"
#include "am_event.h"

#include <unistd.h>

AMIInterface* create_filter(AMIEngine *engine, const char *config,
                            uint32_t input_num, uint32_t output_num)
{
  return (AMIInterface*)AMPlayer::create(engine, config, input_num, output_num);
}

AMIPlayer* AMPlayer::create(AMIEngine *engine, const std::string& config,
                            uint32_t input_num, uint32_t output_num)
{
  AMPlayer *result = new AMPlayer(engine);
  if (AM_UNLIKELY(result && (AM_STATE_OK != result->init(config,
                                                         input_num,
                                                         output_num)))) {
    delete result;
    result = NULL;
  }

  return (AMIPlayer*)result;
}

void* AMPlayer::get_interface(AM_REFIID refiid)
{
  return (refiid == IID_AMIPlayer) ? (AMIPlayer*)this :
      inherited::get_interface(refiid);
}

void AMPlayer::destroy()
{
  inherited::destroy();
}

void AMPlayer::get_info(INFO& info)
{
  info.num_in = m_input_num;
  info.num_out = m_output_num;
  info.name = m_name;
}

AMIPacketPin* AMPlayer::get_input_pin(uint32_t id)
{
  AMIPacketPin *pin = (id < m_input_num) ? m_input_pins[id] : nullptr;
  if (AM_UNLIKELY(!pin)) {
    ERROR("No such input pin(pin index %u)!", id);
  }
  return pin;
}

AMIPacketPin* AMPlayer::get_output_pin(uint32_t id)
{
  ERROR("%s doesn't have output pin!", m_name);
  return NULL;
}

AM_STATE AMPlayer::start()
{
  return AM_STATE_OK;
}

AM_STATE AMPlayer::stop()
{
  AUTO_MEM_LOCK(m_lock);
  AM_STATE state = AM_STATE_OK;
  if (AM_UNLIKELY(m_is_paused.load())) {
    for (uint32_t i = 0; i < m_input_num; ++ i) {
      m_input_pins[i]->enable(true);
    }
    m_is_paused = false;
  }
  if (AM_LIKELY(m_run.load())) {
    m_run = false;
    m_event->signal();
    for (uint32_t i = 0; i < m_input_num; ++ i) {
      m_input_pins[i]->enable(false);
      m_input_pins[i]->stop();
    }
    state = inherited::stop();
  }

  return state;
}

AM_STATE AMPlayer::pause(bool enabled)
{
  AUTO_MEM_LOCK(m_lock);
  bool ret = false;
  if (AM_LIKELY(m_player_audio->is_player_running())) {
    ret = m_player_audio->pause(enabled);
    m_is_paused = ret ? enabled : m_is_paused.load();
    for (uint32_t i = 0; i < m_input_num; ++ i) {
      m_input_pins[i]->enable(!m_is_paused.load());
    }
  }
  post_engine_msg(ret ? AMIEngine::ENG_MSG_OK : AMIEngine::ENG_MSG_ERROR);

  return (ret ? AM_STATE_OK : AM_STATE_ERROR);
}

void AMPlayer::set_aec_enabled(bool enabled)
{
  if (AM_LIKELY(m_player_config)) {
    m_player_config->audio.enable_aec = enabled;
  }
}

uint32_t AMPlayer::version()
{
  return PLAYER_VERSION_NUMBER;
}

void AMPlayer::on_run()
{
  ack(AM_STATE_OK);
  for (uint32_t i = 0; i < m_input_num; ++ i) {
    m_input_pins[i]->enable(true);
    m_input_pins[i]->run();
  }
  m_run = true;

  INFO("%s starts to run!", m_name);
  while (m_run) {
    m_event->wait();
  }

  m_player_audio->stop(false/* Don't wait */);
  while (m_audio_queue && !m_audio_queue->empty()) {
    m_audio_queue->front_pop()->release();
  }
  if (AM_LIKELY(!m_run)) {
    NOTICE("%s posts EOS!", m_name);
    post_engine_msg(AMIEngine::ENG_MSG_EOS);
  } else {
    NOTICE("%s posts ABORT!", m_name);
    post_engine_msg(AMIEngine::ENG_MSG_ABORT);
  }
  m_run = false;
  INFO("%s exits mainloop!", m_name);
}

inline AM_STATE AMPlayer::on_info(AMPacket *packet)
{
  AM_STATE state = AM_STATE_ERROR;
  switch(packet->get_attr()) {
    case AMPacket::AM_PAYLOAD_ATTR_AUDIO: {
      AM_AUDIO_INFO *audioInfo = (AM_AUDIO_INFO*)(packet->get_data_ptr());
      INFO("\n%s received audio information:"
           "\n                     Channels: %u"
           "\n                  Sample Rate: %u"
           "\n                   PCM Format: %u",
           m_name,
           audioInfo->channels,
           audioInfo->sample_rate,
           audioInfo->sample_format);
      m_eos_map |= 1 << 0;
      m_wait_finish =
          ((m_last_audio_info.channels != audioInfo->channels) ||
           (m_last_audio_info.sample_rate != audioInfo->sample_rate) ||
           (m_last_audio_info.sample_format != audioInfo->sample_format));
      m_player_audio->set_audio_info(*audioInfo);
      m_player_audio->set_player_default_latency(m_player_config->\
                                                 audio.buffer_delay_ms);
      m_player_audio->set_echo_cancel_enabled(m_player_config->\
                                              audio.enable_aec);
      if (AM_LIKELY(m_wait_finish && !m_audio_queue->empty())) {
        NOTICE("Waiting for last file to finish playing!");
        m_player_audio->stop();
      }
      m_wait_finish = false;
      memcpy(&m_last_audio_info, audioInfo, sizeof(m_last_audio_info));
      state = m_player_audio->start(m_player_config->audio.initial_volume) ?
          AM_STATE_OK : AM_STATE_ERROR;
    }break;
    case AMPacket::AM_PAYLOAD_ATTR_VIDEO: {
      /* todo: Add video playback */
      m_eos_map |= 1 << 1;
    }break;
    default: {
      ERROR("Only audio and video are supported!");
    }break;
  }

  return state;
}

inline AM_STATE AMPlayer::on_data(AMPacket *packet)
{
  AM_STATE state = AM_STATE_OK;
  switch(packet->get_attr()) {
    case AMPacket::AM_PAYLOAD_ATTR_AUDIO: {
      packet->add_ref();
      m_audio_queue->push(packet);
    }break;
    case AMPacket::AM_PAYLOAD_ATTR_VIDEO: {
      /* todo: Add video player */
    }break;
    default: {
      ERROR("Only audio and video can be played!");
      state = AM_STATE_ERROR;
    }break;
  }

  return state;
}

AM_STATE AMPlayer::on_eof(AMPacket *packet)
{
  NOTICE("%s received EOF!", m_name);
  if (AM_LIKELY(m_eos_map & (1 << 1))) {
    NOTICE("Prepera to stop video!");
    /* todo: Add video player */
    m_eos_map &= ~(1 << 1);
  }
  if (AM_LIKELY(m_eos_map & (1 << 0))) {
    NOTICE("Prepare to stop audio!");
    m_eos_map &= ~(1 << 0);
    packet->add_ref();
    m_audio_queue->push(packet);
    m_player_audio->stop(true /* Wait player to stop */);
  }

  return (packet->get_type() == AMPacket::AM_PAYLOAD_TYPE_EOF) ?
      post_engine_msg(AMIEngine::ENG_MSG_EOF) :
      post_engine_msg(AMIEngine::ENG_MSG_EOL);
}

AM_STATE AMPlayer::process_packet(AMPacket *packet)
{
  AM_STATE state = AM_STATE_OK;
  if (AM_LIKELY(packet)) {
    switch(packet->get_type()) {
      case AMPacket::AM_PAYLOAD_TYPE_INFO: {
        state = on_info(packet);
        if (AM_LIKELY(AM_STATE_OK == state)) {
          post_engine_msg(AMIEngine::ENG_MSG_OK);
        }
      }break;
      case AMPacket::AM_PAYLOAD_TYPE_DATA: {
        state = on_data(packet);
      }break;
      case AMPacket::AM_PAYLOAD_TYPE_EOL:
      case AMPacket::AM_PAYLOAD_TYPE_EOF: {
        state = on_eof(packet);
      }break;
      default: {
        ERROR("Unknown packet type: %u!", packet->get_type());
      }break;
    }
    packet->release();
  }

  return state;
}

AMPlayer::AMPlayer(AMIEngine *engine) :
  inherited(engine)
{
  memset(&m_last_audio_info, 0, sizeof(m_last_audio_info));
}

AMPlayer::~AMPlayer()
{
  AM_DESTROY(m_player_audio);
  AM_DESTROY(m_event);
  while (m_audio_queue && !m_audio_queue->empty()) {
    m_audio_queue->front_pop()->release();
  }
  for (uint32_t i = 0; i < m_input_num; ++ i) {
    AM_DESTROY(m_input_pins[i]);
  }
  delete[] m_input_pins;
  delete m_config;
  delete m_audio_queue;
  DEBUG("~AMPlayer");
}

AM_STATE AMPlayer::init(const std::string& config,
                        uint32_t input_num,
                        uint32_t output_num)
{
  AM_STATE state = AM_STATE_OK;
  m_input_num  = input_num;
  m_output_num = output_num;
  do {
    m_event = AMEvent::create();
    if (AM_UNLIKELY(!m_event)) {
      ERROR("Failed to create event!");
      state = AM_STATE_NO_MEMORY;
      break;
    }
    m_config = new AMPlayerConfig();
    if (AM_UNLIKELY(NULL == m_config)) {
      ERROR("Failed to create config module for Player filter!");
      state = AM_STATE_NO_MEMORY;
      break;
    }
    m_player_config = m_config->get_config(config);
    if (AM_UNLIKELY(NULL == m_player_config)) {
      ERROR("Can not get configuration from file %s, please check!",
            config.c_str());
      state = AM_STATE_ERROR;
      break;
    } else {
      state = inherited::init((const char*)m_player_config->name.c_str(),
                              m_player_config->real_time.enabled,
                              m_player_config->real_time.priority);
      if (AM_LIKELY(AM_STATE_OK == state)) {
        if (AM_UNLIKELY(!m_input_num)) {
          ERROR("%s doesn't have input! Invalid configuration! Abort!", m_name);
          state = AM_STATE_ERROR;
          break;
        }
        if (AM_UNLIKELY(m_output_num)) {
          WARN("%s should not have output, but output num is %u, reset to 0!",
               m_name, m_output_num);
          m_output_num = 0;
        }
        m_input_pins = new AMPlayerInput*[m_input_num];
        if (AM_UNLIKELY(!m_input_pins)) {
          ERROR("Failed to allocate memory PlayerInputPin pointers!");
          state = AM_STATE_NO_MEMORY;
          break;
        }
        memset(m_input_pins, 0, sizeof(AMPlayerInput*) * m_input_num);
        for (uint32_t i = 0; i < m_input_num; ++ i) {
          std::string pin_name = "PlayerInputPin[" + std::to_string(i) + "]";
          m_input_pins[i] = AMPlayerInput::create(this, pin_name.c_str());
          if (AM_UNLIKELY(!m_input_pins[i])) {
            ERROR("Failed to create %s!", pin_name.c_str());
            state = AM_STATE_ERROR;
            break;
          }
        }
        if (AM_UNLIKELY(AM_STATE_OK != state)) {
          break;
        }
        m_audio_queue = new PacketQueue();
        if (AM_UNLIKELY(!m_audio_queue)) {
          state = AM_STATE_NO_MEMORY;
          ERROR("Failed to create audio queue!");
          break;
        }
        m_player_audio = create_audio_player(m_player_config->audio.interface,
                                             m_player_config->name + "." +
                                             m_player_config->audio.interface,
                                             this,
                                             static_player_callback);
        if (AM_UNLIKELY(!m_player_audio)) {
          ERROR("Failed to create audio player!");
          state = AM_STATE_ERROR;
          break;
        }
      }
    }
  }while(0);

  return state;
}

void AMPlayer::static_player_callback(AudioPlayer *data)
{
  ((AMPlayer*)data->owner)->player_callback(&data->data);
}

void AMPlayer::player_callback(AudioData *data)
{

  if (AM_LIKELY(data)) {
    uint8_t   *data_write = data->data;
    uint32_t   &need_size = data->need_size;
    uint32_t written_size = 0;
    while(written_size < need_size) {
      AMPacket *packet = nullptr;
      if (AM_UNLIKELY(m_audio_queue->empty())) {
        /* If audio packet queue is empty,
         * just feed audio server empty data
         */
        switch(m_last_audio_info.sample_format) {
          case AM_SAMPLE_U8:
            memset(data_write + written_size, 0x80, (need_size - written_size));
            break;
          case AM_SAMPLE_ALAW:
          case AM_SAMPLE_ULAW:
          default:
            memset(data_write + written_size, 0xff, (need_size - written_size));
            break;
        }
        written_size = need_size;
        data->written_size = written_size;
        data->drain = m_wait_finish.load();
        continue;
      }
      data->drain = false;
      packet = m_audio_queue->front();
      if (AM_LIKELY(packet->get_type() == AMPacket::AM_PAYLOAD_TYPE_DATA)) {
        uint32_t    space = (need_size - written_size);
        uint32_t data_offset = packet->get_data_offset();
        uint32_t avail_size = packet->get_data_size() - data_offset;
        uint8_t *data_ptr = (uint8_t*)(packet->get_data_ptr() + data_offset);

        avail_size = (avail_size < space) ? avail_size : space;
        memcpy(data_write + written_size, data_ptr, avail_size);
        written_size += avail_size;
        data_offset += avail_size;
        if (AM_LIKELY(data_offset == packet->get_data_size())) {
          m_audio_queue->pop();
          packet->set_data_offset(0);
          packet->release();
        } else {
          packet->set_data_offset(data_offset);
        }
      } else if ((packet->get_type() == AMPacket::AM_PAYLOAD_TYPE_EOF) ||
                 (packet->get_type() == AMPacket::AM_PAYLOAD_TYPE_EOL)) {
        data->drain = true;
        m_audio_queue->pop();
        packet->release();
        break;
      }
    }
    data->written_size = written_size;
  }
}
