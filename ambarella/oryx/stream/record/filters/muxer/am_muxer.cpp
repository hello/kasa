/*******************************************************************************
 * am_muxer.cpp
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
#include "am_amf_interface.h"
#include "am_amf_queue.h"
#include "am_amf_base.h"
#include "am_amf_packet.h"
#include "am_amf_packet_filter.h"
#include "am_amf_packet_pin.h"
#include "am_amf_packet_pool.h"

#include "am_muxer_if.h"
#include "am_muxer.h"
#include "am_muxer_config.h"
#include "am_muxer_version.h"

#include "am_muxer_codec_obj.h"

#include "am_muxer_codec_if.h"

AMIInterface* create_filter(AMIEngine *engine, const char *config,
                            uint32_t input_num, uint32_t output_num)
{
  return (AMIInterface*)AMMuxer::create(engine, config, input_num, output_num);
}

AMIMuxer* AMMuxer::create(AMIEngine *engine, const std::string& config,
                          uint32_t input_num, uint32_t output_num)
{
  AMMuxer *result = new AMMuxer(engine);
  if (AM_UNLIKELY(result && (AM_STATE_OK != result->init(config,
                                                         input_num,
                                                         output_num)))) {
    delete result;
    result = NULL;
  }
  return result;
}

void* AMMuxer::get_interface(AM_REFIID ref_iid)
{
  return (ref_iid == IID_AMIMuxer) ? ((AMIMuxer*)this) :
      inherited::get_interface(ref_iid);
}

void AMMuxer::destroy()
{
  inherited::destroy();
}

void AMMuxer::get_info(INFO& info)
{
  info.num_in = m_input_num;
  info.num_out = m_output_num;
  info.name = m_name;
}

AMIPacketPin* AMMuxer::get_input_pin(uint32_t index)
{
  AMIPacketPin *pin = (index < m_input_num) ? m_inputs[index] : nullptr;
  if (AM_UNLIKELY(!pin)) {
    ERROR("No such input pin [index: %u]", index);
  }
  return pin;
}

AMIPacketPin* AMMuxer::get_output_pin(uint32_t index)
{
  ERROR("%s doesn't have output pin!", m_name);
  return NULL;
}

AM_STATE AMMuxer::start()
{
  /* todo: is this API needed? */
  return AM_STATE_OK;
}

AM_STATE AMMuxer::stop()
{
  m_run = false;
  return inherited::stop();
}

bool AMMuxer::start_file_recording()
{
  bool ret = true;
  INFO("begin to start file recording.");
  for (uint32_t i = 0; i < m_muxer_config->media_type_num; ++ i) {
    if (AM_LIKELY((m_muxer_codec[i].is_valid()) &&
                  (m_muxer_codec[i].m_codec->get_muxer_attr()
                   == AM_MUXER_FILE_NORMAL))) {
      if(AM_UNLIKELY(!m_muxer_codec[i].m_codec->start_file_writing())) {
        ERROR("file muxer %s start file writing error.", m_muxer_codec[i].m_name.c_str());
        ret = false;
        break;
      }
    }
  }
  if(ret) {
    INFO("start file recording success.");
  }
  return ret;
}

bool AMMuxer::stop_file_recording()
{
  bool ret = true;
  INFO("begin to stop file recording");
  for (uint32_t i = 0; i < m_muxer_config->media_type_num; ++ i) {
    if (AM_LIKELY((m_muxer_codec[i].is_valid()) &&
                  (m_muxer_codec[i].m_codec->is_running()))) {
      if(AM_UNLIKELY(!m_muxer_codec[i].m_codec->stop_file_writing())) {
        ERROR("file muxer %s stop error.", m_muxer_codec[i].m_name.c_str());
        ret = false;
        break;
      }
    }
  }
  if(ret) {
    INFO("stop file recording success.");
  }
  return ret;
}

uint32_t AMMuxer::version()
{
  return MUXER_VERSION_NUMBER;
}

AM_MUXER_TYPE AMMuxer::type()
{
  return m_muxer_config ? m_muxer_config->type : AM_MUXER_TYPE_NONE;
}

void AMMuxer::on_run()
{
  AMPacketQueueInputPin *input_pin = NULL;
  AMPacket              *input_pkt = NULL;
  AmMsg  engine_msg(AMIEngine::ENG_MSG_OK);
  engine_msg.p0 = (int_ptr)(get_interface(IID_AMIInterface));

  ack(AM_STATE_OK);
  m_run = true;

  INFO("%s starts to run!", m_name);
  post_engine_msg(engine_msg);

  while(m_run) {
    if (AM_UNLIKELY(!wait_input_packet(input_pin, input_pkt))) {
      if (AM_LIKELY(!m_run)) {
        NOTICE("Stop is called!");
      } else {
        NOTICE("Filter is aborted!");
      }
      break;
    }
    if (AM_LIKELY(input_pkt)) {
      uint32_t count = 0;
      for (uint32_t i = 0; i < m_muxer_config->media_type_num; ++ i) {
        if (AM_LIKELY(m_muxer_codec[i].is_valid())) {
          if (((input_pkt->get_packet_type() & AMPacket::AM_PACKET_TYPE_EVENT)
              != 0) && (m_muxer_codec[i].m_codec->get_muxer_attr()
                  == AM_MUXER_FILE_NORMAL)) {
            continue;
          }
          if (((input_pkt->get_packet_type() & AMPacket::AM_PACKET_TYPE_NORMAL)
              != 0) && (m_muxer_codec[i].m_codec->get_muxer_attr()
                  == AM_MUXER_FILE_EVENT)) {
            continue;
          }
          switch (m_muxer_codec[i].m_codec->get_state()) {
            case AM_MUXER_CODEC_ERROR: {
              ERROR("Muxer %s: unrecoverable error occurred, destroy!",
                    m_muxer_codec[i].m_name.c_str());
              m_muxer_codec[i].destroy();
            } break;
            case AM_MUXER_CODEC_INIT:
            case AM_MUXER_CODEC_STOPPED:
              if (AM_UNLIKELY(AM_STATE_OK != m_muxer_codec[i].m_codec->start())) {
                ERROR("Failed to start muxer %s, destroy!",
                      m_muxer_codec[i].m_name.c_str());
                m_muxer_codec[i].destroy();
              } /* muxer enters RUNNING state, if start OK */
              /* no breaks */
            case AM_MUXER_CODEC_RUNNING: {
              if (AM_LIKELY(m_muxer_codec[i].is_valid())) {
                /* check again, muxer may be destroyed when failed to start */
                input_pkt->add_ref();
                ++ count;
                m_muxer_codec[i].m_codec->feed_data(input_pkt);
              }
            } break;
            default :
              break;
          }
        }
      }
      input_pkt->release();
#if 0
      /*In order to make other muxer filtr continue running,
        the abort cmd should not be sent to engine when no muxer is running.
      */
      if (AM_UNLIKELY(!count)) {
        ERROR("No muxer is running! ABORT!");
        break;
      }
#endif
    } else {
      ERROR("Invalid packet!");
    }
  }
  for (uint32_t i = 0; i < m_muxer_config->media_type_num; ++ i) {
    if (AM_LIKELY(m_muxer_codec[i].is_valid())) {
      m_muxer_codec[i].m_codec->stop();
    }
  }
  if (AM_LIKELY(!m_run)) {
    NOTICE("%s posts EOS!", m_name);
  } else {
    NOTICE("%s posts ABORT!", m_name);
    engine_msg.code = AMIEngine::ENG_MSG_ABORT;
    post_engine_msg(engine_msg);
  }
  m_run = false;

  INFO("%s exits mainloop!", m_name);
}

AM_STATE AMMuxer::load_muxer_codec()
{
  AM_STATE state = AM_STATE_ERROR;
  if (AM_LIKELY(m_muxer_config)) {
    destroy_muxer_codec();
    m_muxer_codec = new AMMuxerCodecObj[m_muxer_config->media_type_num];
    if (AM_LIKELY(m_muxer_codec)) {
      bool muxer_codec_loaded = false;
      for (uint32_t i = 0; i < m_muxer_config->media_type_num; ++ i) {
        muxer_codec_loaded =
            (m_muxer_codec[i].load_codec(m_muxer_config->media_type[i]) ||
             muxer_codec_loaded);
      }
      state = muxer_codec_loaded ? AM_STATE_OK : AM_STATE_ERROR;
    } else {
      ERROR("Failed to allodate memory for muxer codec object!");
    }
  } else {
    ERROR("Muxer config is not loaded!");
  }

  return state;
}

void AMMuxer::destroy_muxer_codec()
{
  delete[] m_muxer_codec;
  m_muxer_codec = NULL;
}

AMMuxer::AMMuxer(AMIEngine *engine) :
    inherited(engine),
    m_muxer_config(nullptr),
    m_config(nullptr),
    m_muxer_codec(nullptr),
    m_input_num(0),
    m_output_num(0),
    m_run(false),
    m_started(false)
{
  m_inputs.clear();
}

AMMuxer::~AMMuxer()
{
  for (uint32_t i = 0; i < m_inputs.size(); ++ i) {
    AM_DESTROY(m_inputs[i]);
  }
  m_inputs.clear();
  delete m_config;
  destroy_muxer_codec();
}

AM_STATE AMMuxer::init(const std::string& config,
                       uint32_t input_num,
                       uint32_t output_num)
{
  AM_STATE state = AM_STATE_OK;
  m_input_num = input_num;
  m_output_num = output_num;

  do {
    m_config = new AMMuxerConfig();
    if (AM_UNLIKELY(!m_config)) {
      ERROR("Failed to create config module for Muxer filter!");
      state = AM_STATE_NO_MEMORY;
      break;
    }

    m_muxer_config = m_config->get_config(config);
    if (AM_UNLIKELY(!m_muxer_config)) {
      ERROR("Can not get configuration from file %s, please check!",
            config.c_str());
      state = AM_STATE_ERROR;
      break;
    }
    state = load_muxer_codec();
    if (AM_UNLIKELY(AM_STATE_OK != state)) {
      ERROR("Failed to load muxer codecs!");
      break;
    }
    state = inherited::init((const char*)m_muxer_config->name.c_str(),
                            m_muxer_config->real_time.enabled,
                            m_muxer_config->real_time.priority);
    if (AM_LIKELY(state == AM_STATE_OK)) {
      if (AM_UNLIKELY(0 == m_input_num)) {
        ERROR("%s doesn't have input! Invalid configuration! Abort!");
        state = AM_STATE_ERROR;
        break;
      }
      if (AM_UNLIKELY(m_output_num)) {
        WARN("%s should not have output, but output num is %u, reset to 0!",
             m_name, m_output_num);
        m_output_num = 0;
      }
      for (uint32_t i = 0; i < m_input_num; ++ i) {
        AMMuxerInput *input = AMMuxerInput::create(this);
        if (AM_UNLIKELY(!input)) {
          state = AM_STATE_ERROR;
          ERROR("Failed to create input pin[%u] for %s!", i, m_name);
          break;
        }
        m_inputs.push_back(input);
      }
      if (AM_UNLIKELY(AM_STATE_OK != state)) {
        break;
      }
    }
  }while(0);
  return state;
}
