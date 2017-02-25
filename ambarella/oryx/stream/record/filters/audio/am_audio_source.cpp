/*******************************************************************************
 * am_audio_source.cpp
 *
 * History:
 *   2014-12-2 - [ypchang] created file
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
#include "am_audio_source_config.h"
#include "am_audio_source_version.h"

#include "am_audio_capture_if.h"

#include "am_mutex.h"
#include "am_event.h"

AMIInterface* create_filter(AMIEngine *engine, const char *config,
                            uint32_t input_num, uint32_t output_num)
{
  return (AMIInterface*)AMAudioSource::create(engine, config,
                                              input_num, output_num);
}

AMIAudioSource* AMAudioSource::create(AMIEngine *engine,
                                      const std::string& config,
                                      uint32_t input_num,
                                      uint32_t output_num)
{
  AMAudioSource *result = new AMAudioSource(engine);
  if (AM_UNLIKELY(result && (AM_STATE_OK != result->init(config,
                                                         input_num,
                                                         output_num)))) {
    delete result;
    result = NULL;
  }

  return result;
}

void* AMAudioSource::get_interface(AM_REFIID ref_iid)
{
  return (ref_iid == IID_AMIAudioSource) ? (AMIAudioSource*)this :
      inherited::get_interface(ref_iid);
}

void AMAudioSource::destroy()
{
  inherited::destroy();
}

void AMAudioSource::get_info(INFO& info)
{
  info.num_in = m_input_num;
  info.num_out = m_output_num;
  info.name = m_name;
}

AMIPacketPin* AMAudioSource::get_input_pin(uint32_t index)
{
  ERROR("%s doesn't have input pin!", m_name);
  return NULL;
}

AMIPacketPin* AMAudioSource::get_output_pin(uint32_t index)
{
  AMIPacketPin *pin = (index < m_output_num) ? m_outputs[index] : nullptr;
  if (AM_UNLIKELY(!pin)) {
    ERROR("No such output pin [index:%u]", index);
  }

  return pin;
}

AM_STATE AMAudioSource::start()
{
  AUTO_MEM_LOCK(m_lock);
  AM_STATE state = AM_STATE_ERROR;
  bool started = (NULL != m_audio_capture);
  do {
    if (!m_codec_enable) {
      NOTICE("The codec is not enabled in %s", m_name);
      state = AM_STATE_OK;
      break;
    }
    m_packet_pool->enable(true);
    if (AM_LIKELY(started)) {
      started = false;
      if (AM_LIKELY(!m_audio_disabled)) {
        for (uint32_t i = 0; i < m_aconfig->audio_type_num; ++ i) {
          if (AM_LIKELY(m_audio_codec[i].is_valid())) {
            if (AM_LIKELY(m_audio_codec[i].
                          start(m_aconfig->real_time.enabled,
                                m_aconfig->real_time.priority))) {
              started = true;
            } else {
              ERROR("Failed to start %s", m_audio_codec[i].m_name.c_str());
            }
          }
        }
      }
      m_audio_capture->set_echo_cancel_enabled(m_aconfig->enable_aec);
      m_started = (m_audio_disabled || (started && m_audio_capture->
          start(m_aconfig->initial_volume)));
      m_run = m_started.load();
      m_abort = false;
      if (AM_UNLIKELY(!m_started.load() && !m_audio_disabled &&
                      !(started && m_audio_capture->
                          start(m_aconfig->initial_volume)))) {
        m_event->signal();
        m_abort = true;
      }
    }
    state = m_started ? AM_STATE_OK : AM_STATE_ERROR;
  } while(0);
  return state;
}

AM_STATE AMAudioSource::stop()
{
  AM_STATE state = AM_STATE_OK;
  AUTO_MEM_LOCK(m_lock);

  if (AM_LIKELY(m_started.load())) {
    m_started = false;
    m_abort = false;
    if (AM_LIKELY(!m_audio_disabled.load())) {
      m_audio_capture->stop();
      for (uint32_t i = 0; i < m_aconfig->audio_type_num; ++ i) {
        m_audio_codec[i].stop();
      }
    }
    m_packet_pool->enable(false);
  } else {
    NOTICE("%s is already stopped!", m_name);
  }
  if (AM_LIKELY(m_run)) {
    m_run = false;
    m_event->signal();
    state = inherited::stop();
  }

  return state;
}

AM_STATE AMAudioSource::enable_codec(AM_AUDIO_TYPE type, bool enable)
{
  AM_STATE ret = AM_STATE_OK;
  bool need_start = false;
  bool need_stop = false;
  AMAudioCodecObj *obj = nullptr;
  do {
    if (m_audio_disabled) {
      ERROR("There are no codecs in %s", m_name);
      ret = AM_STATE_ERROR;
      break;
    }
    obj = get_codec_by_type(type);
    if (!obj) {
      ERROR("Failed to find %s codec in %s", audio_type_to_string(type).c_str(),
            m_name);
      ret = AM_STATE_ERROR;
      break;
    }
    if (enable) {//true
      if (obj->is_running()) {
        NOTICE("%s is already running in %s.", audio_type_to_string(type).c_str(),
               m_name);
        break;
      } else { // not running
        NOTICE("Starting %s codec in %s", audio_type_to_string(type).c_str(),
               m_name);
        need_start = true;
      }
    } else {//false
      if (obj->is_running()) {
        NOTICE("Stopping %s codec in %s", audio_type_to_string(type).c_str(),
               m_name);
        need_stop = true;
      } else { //not running
        NOTICE("%s is already stopped in %s", audio_type_to_string(type).c_str(),
               m_name);
        break;
      }
    }
    if (need_start) {
      if (!m_codec_enable) {
        m_packet_pool->enable(true);
      }
      bool started = false;
      if (AM_LIKELY(obj->is_valid())) {
        if (AM_LIKELY(obj->start(m_aconfig->real_time.enabled,
                                 m_aconfig->real_time.priority))) {
          started = true;
        } else {
          ERROR("Failed to start %s in %s", obj->m_name.c_str(), m_name);
        }
      }
      if (!m_codec_enable) {
        m_started = (started && m_audio_capture->start(m_aconfig->initial_volume));
        if (AM_UNLIKELY((!m_started.load()) && (!started))) {
          ERROR("Start audio capture failed!");
          m_abort = true;
          m_event->signal();
        }
        m_codec_enable = true;
        ret = m_started ? AM_STATE_OK : AM_STATE_ERROR;
      }
    }
    if (need_stop) {
      if (obj->is_valid() && obj->is_running()) {
        obj->stop();
      }
      bool all_stopped = true;
      for (uint32_t i = 0; i < m_aconfig->audio_type_num; ++ i) {
        if (m_audio_codec[i].is_running()) {
          all_stopped = false;
          break;
        }
      }
      if (all_stopped) {
        m_started = false;
        m_audio_capture->stop();
        m_packet_pool->enable(false);
        m_codec_enable = false;
      }
    }
  } while(0);
  return ret;
}

uint32_t AMAudioSource::get_audio_number()
{
  return m_aconfig ? m_aconfig->audio_type_num : 0;
}

uint32_t AMAudioSource::get_audio_sample_rate()
{
  return m_src_audio_info.sample_rate;
}

void AMAudioSource::set_stream_id(uint32_t base)
{
  if (AM_LIKELY(!m_audio_disabled)) {
    uint32_t id = base;
    for (uint32_t i = 0; i < m_aconfig->audio_type_num; ++ i) {
      if (AM_LIKELY(m_audio_codec[i].is_valid())) {
        m_audio_codec[i].set_stream_id(id);
        ++ id;
      }
    }
  }
}

void AMAudioSource::set_aec_enabled(bool enabled)
{
  if (AM_LIKELY(m_aconfig)) {
    m_aconfig->enable_aec = enabled;
  }
}

uint32_t AMAudioSource::version()
{
  return ASOURCE_VERSION_NUMBER;
}

void AMAudioSource::abort()
{
  AUTO_MEM_LOCK(m_lock);
  if (AM_LIKELY(m_started)) {
    m_started = false;
    m_abort = true;
  }
  if (AM_LIKELY(m_run)) {
    m_run = false;
    m_event->signal();
  }
}

void AMAudioSource::on_run()
{
  AmMsg  engine_msg(AMIEngine::ENG_MSG_OK);
  engine_msg.p0 = (int_ptr)(get_interface(IID_AMIInterface));

  ack(AM_STATE_OK);
  m_run = true;
  post_engine_msg(engine_msg);

  while (m_run.load()) {
    /* todo: Add audio codec encoder's statistics codes */
    m_event->wait();
  }
  if (AM_UNLIKELY(m_abort.load())) {
    m_audio_capture->stop();
    engine_msg.code = AMIEngine::ENG_MSG_ABORT;
    post_engine_msg(engine_msg);
  }
  NOTICE("%s exits mainloop!", m_name);
}

AM_STATE AMAudioSource::load_audio_codec()
{
  AM_STATE state = AM_STATE_ERROR;

  if (AM_LIKELY(m_aconfig)) {
    destroy_audio_codec();
    m_audio_codec = new AMAudioCodecObj[m_aconfig->audio_type_num];

    if (AM_LIKELY(m_audio_codec)) {
      bool audio_codec_loaded = false;
      AMStringList &audio_type =
          *m_aconfig->audio_type_map[m_aconfig->audio_profile];
      m_audio_disabled = ((0 == m_aconfig->audio_type_num) ||
          is_str_equal(m_aconfig->audio_profile.c_str(), "none"));

      NOTICE("Audio is using profile: %s", m_aconfig->audio_profile.c_str());
      for (uint32_t i = 0; i < m_aconfig->audio_type_num; ++ i) {
        DEBUG("Loading audio codec[%u]: %s",
              i, audio_type[i].c_str());
        audio_codec_loaded =
            (m_audio_codec[i].load_codec(audio_type[i]) ||
             audio_codec_loaded);
      }
      if (AM_LIKELY(audio_codec_loaded)) {
        /* Check if all the audio codec's required source audio parameters are
         * the same, if not, only keep the first audio codec.
         * The first audio codec has the highest priority
         */
        int32_t def = -1;
        for (uint32_t idx = 0; idx < m_aconfig->audio_type_num; ++ idx) {
          if (AM_LIKELY((def < 0) && m_audio_codec[idx].is_valid())) {
            def = idx;
            memcpy(&m_src_audio_info,
                   &m_audio_codec[def].m_codec_required_info,
                   sizeof(m_src_audio_info));
            NOTICE("Audio codec %s has the highest priority!",
                   m_audio_codec[idx].m_name.c_str());
          }
          if (AM_LIKELY((def >= 0) && ((uint32_t)def != idx) &&
                        m_audio_codec[idx].is_valid())) {
            AM_AUDIO_INFO &defInfo = m_audio_codec[def].m_codec_required_info;
            AM_AUDIO_INFO    &info = m_audio_codec[idx].m_codec_required_info;
            const char *def_name = m_audio_codec[def].m_name.c_str();
            const char     *name = m_audio_codec[idx].m_name.c_str();

            if (AM_UNLIKELY((info.channels      != defInfo.channels)    ||
                            (info.sample_rate   != defInfo.sample_rate) ||
                            (info.sample_format != defInfo.sample_format))) {
              WARN("%s          Channels: %8u, %s          Channels: %8u",
                   def_name, defInfo.channels, name, info.channels);
              WARN("%s       Sample Rate: %8u, %s       Sample Rate: %8u",
                   def_name, defInfo.sample_rate, name, info.sample_rate);
              WARN("%s Audio Format Type: %8u, %s Audio Format Type: %8u",
                   def_name, defInfo.sample_format, name, info.sample_format);
              WARN("%s and %s require different audio parameters respectively, "
                   "disable %s", def_name, name, name);
              m_audio_codec[idx].finalize();
            }
          }
        }

        /* Get the least common multiple of all the audio codec's
         * input data chunk size*/
        m_src_audio_info.chunk_size = 1;
        for (uint32_t i = 0; i < m_aconfig->audio_type_num; ++ i) {
          if (AM_LIKELY(m_audio_codec[i].is_valid())) {
            INFO("%s's chunk size is %u bytes",
                 m_audio_codec[i].m_name.c_str(),
                 m_audio_codec[i].m_codec_required_info.chunk_size);
            m_src_audio_info.chunk_size =
                get_lcm(m_src_audio_info.chunk_size,
                        m_audio_codec[i].m_codec_required_info.chunk_size);
            m_audio_codec[i].m_audio_source = this;
          }
        }
        for (uint32_t i = 0; i < m_aconfig->audio_type_num; ++ i) {
          m_audio_codec[i].m_multiple = m_src_audio_info.chunk_size /
              m_audio_codec[i].m_codec_required_info.chunk_size;
        }
        INFO("The least common multiple of all the codec's chunk size is %u",
             m_src_audio_info.chunk_size);
      }
      state = (audio_codec_loaded || m_audio_disabled) ?
          AM_STATE_OK : AM_STATE_ERROR;
    } else {
      ERROR("Failed to allocate memory for audio codec object!");
    }
  } else {
    ERROR("AudioSourceConfig is not loaded!");
  }
  return state;
}

void AMAudioSource::destroy_audio_codec()
{
  delete[] m_audio_codec;
  m_audio_codec = NULL;
}

void AMAudioSource::static_audio_capture(AudioCapture *data)
{
  return ((AMAudioSource*)data->owner)->audio_capture(&data->packet);
}

void AMAudioSource::audio_capture(AudioPacket *data)
{
  if (AM_LIKELY(m_packet_pool->get_avail_packet_num() >
                m_aconfig->audio_type_num)) {
    AMPacket *packet = NULL;
    if (AM_LIKELY(m_packet_pool->alloc_packet(packet, 0))) {
      uint32_t count = 0;

      memcpy(packet->get_data_ptr(), data->data, data->length);
      packet->set_pts(data->pts);
      packet->set_data_size(data->length);
      packet->set_type(AMPacket::AM_PAYLOAD_TYPE_DATA);
      for (uint32_t i = 0; i < m_aconfig->audio_type_num; ++ i) {
        if (AM_LIKELY(m_audio_codec[i].is_running())) {
          /* packet reference count must be increased before
           * sending the packet to any working thread
           */
          packet->add_ref();
          ++ count;
          m_audio_codec[i].push_queue(packet);
        }
      }
      packet->release();
      if (AM_UNLIKELY(!count)) {
        WARN("All the audio encoder are stopped! audio capture should be stopped!");
      }
    } else {
      ERROR("Failed to allocate packet! Dropping %u bytes of data!",
            data->length);
    }
  } else {
    NOTICE("Packet is not enough, %u bytes of audio data is dropped! "
           "Is IO too slow?", data->length);
  }
}

bool AMAudioSource::set_audio_parameters()
{
  bool ret = (m_audio_capture &&
              m_audio_capture->set_channel(m_src_audio_info.channels) &&
              m_audio_capture->set_sample_rate(m_src_audio_info.sample_rate) &&
              m_audio_capture->set_chunk_bytes(m_src_audio_info.chunk_size) &&
              m_audio_capture->set_sample_format(
                  AM_AUDIO_SAMPLE_FORMAT(m_src_audio_info.sample_format)));
  if (AM_LIKELY(ret)) {
    m_src_audio_info.sample_size = m_audio_capture->get_sample_size();
    m_src_audio_info.pkt_pts_increment =
        (uint32_t)m_audio_capture->get_chunk_pts();
    for (uint32_t i = 0; i < m_aconfig->audio_type_num; ++ i) {
      m_audio_codec[i].m_codec_required_info.pkt_pts_increment =
          m_src_audio_info.pkt_pts_increment / m_audio_codec[i].m_multiple;
    }
  } else {
    ERROR("Failed to set audio parameters!");
  }

  return ret;
}

void AMAudioSource::send_packet(AMPacket *packet)
{
  if (AM_LIKELY(packet)) {
    for (uint32_t i = 0; i < m_output_num; ++ i) {
      packet->add_ref();
      m_outputs[i]->send_packet(packet);
    }
    /* packet's default reference count is 1, need to release */
    packet->release();
  }
}

AMAudioCodecObj* AMAudioSource::get_codec_by_type(AM_AUDIO_TYPE type)
{
  AMAudioCodecObj *obj = nullptr;
  for (uint32_t i = 0; i < m_aconfig->audio_type_num; ++ i) {
    if (m_audio_codec[i].get_type() == type) {
      obj = &m_audio_codec[i];
      break;
    }
  }
  return obj;
}

AMAudioSource::AMAudioSource(AMIEngine *engine) :
    inherited(engine)
{
  memset(&m_src_audio_info, 0, sizeof(m_src_audio_info));
  m_outputs.clear();
}

AMAudioSource::~AMAudioSource()
{
  destroy_audio_codec();
  AM_DESTROY(m_audio_capture);
  AM_DESTROY(m_event);
  for (uint32_t i = 0; i < m_outputs.size(); ++ i) {
    AM_DESTROY(m_outputs[i]);
  }
  m_outputs.clear();
  AM_DESTROY(m_packet_pool);
  delete m_config;
}

AM_STATE AMAudioSource::init(const std::string& config,
                             uint32_t input_num,
                             uint32_t output_num)
{
  AM_STATE state = AM_STATE_OK;

  do {
    m_input_num = input_num;
    m_output_num = output_num;
    m_config = new AMAudioSourceConfig();
    if (AM_UNLIKELY(NULL == m_config)) {
      ERROR("Failed to create config module for AudioSource filter!");
      state = AM_STATE_NO_MEMORY;
      break;
    }
    m_aconfig = m_config->get_config(config);
    if (AM_UNLIKELY(!m_aconfig)) {
      ERROR("Can not get configuration file %s, please check!",
            config.c_str());
      state = AM_STATE_ERROR;
      break;
    }
    m_codec_enable = m_aconfig->codec_enable;
    m_event = AMEvent::create();
    if (AM_UNLIKELY(!m_event)) {
      ERROR("Failed to create event!");
      state = AM_STATE_ERROR;
      break;
    }
    state = inherited::init(m_aconfig->name.c_str(),
                            m_aconfig->real_time.enabled,
                            m_aconfig->real_time.priority);
    if (AM_LIKELY(AM_STATE_OK != state)) {
      break;
    }

    if (AM_UNLIKELY(m_input_num)) {
      WARN("%s should not have input, but input number is set to %u, "
           "reset input to 0!", m_name, m_input_num);
      m_input_num = 0;
    }

    if (AM_UNLIKELY(!m_output_num)) {
      WARN("%s should have at least 1 output, but output is 0, set output to 1",
           m_name);
      m_output_num = 1;
    }

    state = load_audio_codec();
    if (AM_UNLIKELY(AM_STATE_OK == state)) {
      std::string poolName = m_aconfig->name + ".packet.pool";
      m_packet_pool = AMFixedPacketPool::create(
          poolName.c_str(),
          m_aconfig->packet_pool_size + m_aconfig->audio_type_num,
          m_src_audio_info.chunk_size);
      if (AM_UNLIKELY(!m_packet_pool)) {
        ERROR("Failed to create packet pool for %s", m_aconfig->name.c_str());
        state = AM_STATE_ERROR;
        break;
      }

      for (uint32_t i = 0; i < m_output_num; ++ i) {
        AMAudioSourceOutput *output = AMAudioSourceOutput::create(this);
        if (AM_UNLIKELY(!output)) {
          ERROR("Failed to create output pin[%u] for %s!", i, m_name);
          state = AM_STATE_ERROR;
          break;
        }
        m_outputs.push_back(output);
      }
      if (AM_UNLIKELY(AM_STATE_OK != state)) {
        break;
      }
    } else {
      ERROR("Failed to load audio codecs!");
      state = AM_STATE_ERROR;
      break;
    }

    m_audio_capture = create_audio_capture(m_aconfig->interface,
                                           m_aconfig->name,
                                           this,
                                           static_audio_capture);
    if (AM_UNLIKELY(!m_audio_capture)) {
      ERROR("Failed to create audio capture!");
      state = AM_STATE_ERROR;
      break;
    }

    if (AM_LIKELY(!m_audio_disabled)) {
      if (AM_LIKELY(!set_audio_parameters())) {
        ERROR("Failed to set audio parameters!");
        state = AM_STATE_ERROR;
        break;
      } else {
        uint32_t id = 0;
        for (uint32_t i = 0; i < m_aconfig->audio_type_num; ++ i) {
          if (AM_LIKELY(m_audio_codec[i].is_valid())) {
            if (AM_UNLIKELY(!m_audio_codec[i].initialize(
                &m_src_audio_info, id, m_aconfig->packet_pool_size))) {
              state = AM_STATE_ERROR;
              ERROR("Failed to initialize audio codec %s",
                    m_audio_codec[i].m_name.c_str());
              break;
            }
            ++ id;
          }
        }
      }
    }
    if (AM_STATE_OK != state) {
      break;
    }

  }while(0);

  return state;
}

std::string AMAudioSource::audio_type_to_string(AM_AUDIO_TYPE type)
{
  std::string type_string;
  switch (type) {
    case AM_AUDIO_NULL : {
      type_string = "AM_AUDIO_NULL";
    } break;
    case AM_AUDIO_LPCM : {
      type_string = "AM_AUDIO_LPCM";
    } break;
    case AM_AUDIO_BPCM : {
      type_string = "AM_AUDIO_BPCM";
    } break;
    case AM_AUDIO_G711A : {
      type_string = "AM_AUDIO_G711A";
    } break;
    case AM_AUDIO_G711U : {
      type_string = "AM_AUDIO_G711U";
    } break;
    case AM_AUDIO_G726_40 : {
      type_string = "AM_AUDIO_G726_40";
    } break;
    case AM_AUDIO_G726_32 : {
      type_string = "AM_AUDIO_G726_32";
    } break;
    case AM_AUDIO_G726_24 : {
      type_string = "AM_AUDIO_G726_24";
    } break;
    case AM_AUDIO_G726_16 : {
      type_string = "AM_AUDIO_G726_16";
    } break;
    case AM_AUDIO_AAC : {
      type_string = "AM_AUDIO_AAC";
    } break;
    case AM_AUDIO_OPUS : {
      type_string = "AM_AUDIO_OPUS";
    } break;
    case AM_AUDIO_SPEEX : {
      type_string = "AM_AUDIO_SPEEX";
    } break;
    default : {
      type_string = "Invalid audio type";
    } break;
  }
  return type_string;
}
