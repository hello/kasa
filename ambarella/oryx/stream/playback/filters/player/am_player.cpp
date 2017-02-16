/*******************************************************************************
 * am_player.cpp
 *
 * History:
 *   2014-9-10 - [ypchang] created file
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
#include "am_audio_define.h"
#include "am_player_if.h"
#include "am_player.h"
#include "am_player_audio.h"
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
  AUTO_SPIN_LOCK(m_lock);
  AM_STATE state = AM_STATE_OK;
  if (AM_UNLIKELY(m_is_paused)) {
    for (uint32_t i = 0; i < m_input_num; ++ i) {
      m_input_pins[i]->enable(true);
    }
    m_is_paused = false;
  }
  if (AM_LIKELY(m_run)) {
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
  AUTO_SPIN_LOCK(m_lock);
  AM_STATE state = AM_STATE_ERROR;
  if (AM_LIKELY(m_audio_player->is_player_running())) {
    state = m_audio_player->pause(enabled);
    m_is_paused = (AM_STATE_OK == state) ? enabled : m_is_paused;
    for (uint32_t i = 0; i < m_input_num; ++ i) {
      m_input_pins[i]->enable(!m_is_paused);
    }
  }
  post_engine_msg((AM_STATE_OK == state) ?
      AMIEngine::ENG_MSG_OK : AMIEngine::ENG_MSG_ERROR);

  return state;
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

  m_audio_player->stop(false /* Don't wait */);
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
      m_eos_map |= 1 << 0;
      state = m_audio_player->start(*audioInfo);
      INFO("\n%s received audio information:"
           "\n                     Channels: %u"
           "\n                  Sample Rate: %u"
           "\n                   PCM Format: %u",
           m_name,
           audioInfo->channels,
           audioInfo->sample_rate,
           audioInfo->sample_format);
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
      m_audio_player->add_packet(packet);
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
    m_audio_player->add_packet(packet);
    m_audio_player->stop(true /* Wait player to stop */);
  }

  return post_engine_msg(AMIEngine::ENG_MSG_EOF);
}

AM_STATE AMPlayer::process_packet(AMPacket *packet)
{
//  AUTO_SPIN_LOCK(m_lock);
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
    inherited(engine),
    m_lock(nullptr),
    m_audio_player(nullptr),
    m_config(nullptr),
    m_event(nullptr),
    m_player_config(nullptr),
    m_input_pins(nullptr),
    m_eos_map(0),
    m_input_num(0),
    m_output_num(0),
    m_run(false),
    m_is_paused(false)
{
}

AMPlayer::~AMPlayer()
{
  AM_DESTROY(m_lock);
  AM_DESTROY(m_audio_player);
  AM_DESTROY(m_event);
  for (uint32_t i = 0; i < m_input_num; ++ i) {
    AM_DESTROY(m_input_pins[i]);
  }
  delete[] m_input_pins;
  delete m_config;
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
        m_lock = AMSpinLock::create();
        if (AM_UNLIKELY(!m_lock)) {
          ERROR("Failed to create AMSpinLock!");
          state = AM_STATE_NO_MEMORY;
          break;
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
        m_audio_player =
            AMPlayerAudio::get_player(m_player_config->audio);
        if (AM_UNLIKELY(!m_audio_player)) {
          ERROR("Failed to create audio player!");
          state = AM_STATE_ERROR;
          break;
        }
      }
    }
  }while(0);

  return state;
}
