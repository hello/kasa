/*******************************************************************************
 * am_player_audio.cpp
 *
 * History:
 *   2014-9-18 - [ypchang] created file
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

#include "am_player_audio.h"
#include "am_player_config.h"
#include "am_audio_define.h"

#include "am_mutex.h"
#include "am_event.h"

#include <unistd.h>

class LockPaMainloop
{
  public:
    LockPaMainloop(pa_threaded_mainloop *mainloop) :
      m_mainloop(mainloop)
    {
      pa_threaded_mainloop_lock(m_mainloop);
    }
    ~LockPaMainloop()
    {
      pa_threaded_mainloop_unlock(m_mainloop);
    }
  private:
    pa_threaded_mainloop *m_mainloop;
};
#define PA_MAIN_LOOP_AUTO_LOCK(a) LockPaMainloop __lock_pa_main_loop__(a)

struct PaData {
    AMPlayerAudioPulse *adev;
    void               *data;
    PaData(AMPlayerAudioPulse *dev, void *context) :
      adev(dev),
      data(context)
    {}
    PaData() :
      adev(nullptr),
      data(nullptr)
    {}
};

AMPlayerAudioPulse* AMPlayerAudioPulse::create(const AudioPlayerConfig &config)
{
  AMPlayerAudioPulse *result = new AMPlayerAudioPulse();
  if (AM_UNLIKELY(result && (AM_STATE_OK != result->init(config)))) {
    delete result;
    result = nullptr;
  }

  return result;
}

void AMPlayerAudioPulse::add_packet(AMPacket *packet)
{
  if (AM_LIKELY(packet)) {
    AUTO_SPIN_LOCK(m_lock_queue);
    packet->add_ref();
    m_audio_queue->push(packet);
  }
}

#if 0
bool AMPlayerAudioPulse::feed_data()
{
  bool ret = false;
  size_t writableSize = 0;
  if (AM_LIKELY(m_stream_playback &&
                (pa_stream_get_state(m_stream_playback) ==
                    PA_STREAM_READY) && !m_is_draining &&
                ((writableSize =
                    pa_stream_writable_size(m_stream_playback)) > 0))) {
    pa_write(m_stream_playback, writableSize, &ret);
  } else {
    usleep(100000);
  }

  return ret;
}
#endif

static pa_sample_format_t smp_fmt_to_pa_fmt(AM_AUDIO_SAMPLE_FORMAT fmt)
{
  pa_sample_format_t format = PA_SAMPLE_INVALID;

  switch(fmt) {
    case AM_SAMPLE_U8: {
      format = PA_SAMPLE_U8;
    }break;
    case AM_SAMPLE_ALAW: {
      format = PA_SAMPLE_ALAW;
    }break;
    case AM_SAMPLE_ULAW: {
      format = PA_SAMPLE_ULAW;
    }break;
    case AM_SAMPLE_S16LE: {
      format = PA_SAMPLE_S16LE;
    }break;
    case AM_SAMPLE_S16BE: {
      format = PA_SAMPLE_S16BE;
    }break;
    case AM_SAMPLE_S32LE: {
      format = PA_SAMPLE_S32LE;
    }break;
    case AM_SAMPLE_S32BE: {
      format = PA_SAMPLE_S32BE;
    }break;
    default: {
      format = PA_SAMPLE_INVALID;
    }break;
  }
  return format;
}

AM_STATE AMPlayerAudioPulse::start(AM_AUDIO_INFO& audioinfo)
{
  AM_STATE state = AM_STATE_OK;
  pa_sample_format_t sample_format =
      smp_fmt_to_pa_fmt(AM_AUDIO_SAMPLE_FORMAT(audioinfo.sample_format));
  bool needRestart = ((audioinfo.channels != m_sample_spec.channels) ||
                      (audioinfo.sample_rate != m_sample_spec.rate)  ||
                      (sample_format != m_sample_spec.format));
  uint32_t channels = audioinfo.channels;
  if (AM_UNLIKELY(channels > PA_CHANNELS_MAX)) {
    NOTICE("Invalid channel number: %u, "
           "maximum supported channel number is %u, reset to %u!",
           channels, m_audio_config->def_channel_num);
    channels = m_audio_config->def_channel_num;
  }
  m_sample_spec.channels = channels;
  m_sample_spec.rate     = audioinfo.sample_rate;
  m_sample_spec.format   = sample_format;
  m_play_latency = m_audio_config->buffer_delay_ms * 1000;
  m_chunk_bytes = pa_usec_to_bytes(m_play_latency, &m_sample_spec);
  m_channel_volume.channels = channels;
  for (uint32_t i = 0; i < channels; ++ i) {
    m_channel_volume.values[i] =
        (uint32_t)(PA_VOLUME_NORM * m_audio_config->initial_volume / 100);
  }
  if (AM_LIKELY(!m_is_player_started || needRestart)) {
    if (AM_LIKELY(m_stream_playback && m_main_loop)) {
      PA_MAIN_LOOP_AUTO_LOCK(m_main_loop);
      pa_stream_disconnect(m_stream_playback);
      pa_stream_unref(m_stream_playback);
      m_stream_playback = nullptr;
      m_is_player_started = false;
    }
    m_is_player_started = start_player();
    state = m_is_player_started ? AM_STATE_OK : AM_STATE_ERROR;
    m_is_draining = false;
  }

  return state;
}

AM_STATE AMPlayerAudioPulse::stop(bool wait)
{
  AUTO_SPIN_LOCK(m_lock);
  AM_STATE state = AM_STATE_OK;

  if (AM_LIKELY(m_is_player_started)) {
    if (AM_LIKELY(wait)) {
      m_event->wait();
      NOTICE("Player buffer is drained!");
    }
    m_is_player_started = false;
    pa_threaded_mainloop_stop(m_main_loop);
    finalize();
  }
  m_lock_queue->lock();
  while (m_audio_queue && !m_audio_queue->empty()) {
    m_audio_queue->front()->release();
    m_audio_queue->pop();
  }
  m_lock_queue->unlock();

  return state;
}

AM_STATE AMPlayerAudioPulse::pause(bool enable)
{
  AUTO_SPIN_LOCK(m_lock);
  AM_STATE state = AM_STATE_OK;

  if (AM_LIKELY(m_stream_playback && m_is_player_started)) {
    pa_threaded_mainloop_lock(m_main_loop);
    if (AM_LIKELY((enable &&
                   (0 == pa_stream_is_corked(m_stream_playback))) ||
                  (!enable &&
                   (1 == pa_stream_is_corked(m_stream_playback))))) {
      pa_operation_state_t opState = PA_OPERATION_CANCELLED;
      pa_operation *op = pa_stream_cork(m_stream_playback,
                                        (enable ? 1 : 0), nullptr, nullptr);
      pa_threaded_mainloop_unlock(m_main_loop);
      while ((opState = pa_operation_get_state(op)) != PA_OPERATION_DONE) {
        if (AM_UNLIKELY(opState == PA_OPERATION_CANCELLED)) {
          NOTICE("Pause stream operation cancelled!");
          break;
        }
        usleep(10000);
      }
      pa_operation_unref(op);
      pa_threaded_mainloop_lock(m_main_loop);
      state = ((opState == PA_OPERATION_DONE) &&
          ((enable && (1 == pa_stream_is_corked(m_stream_playback))) ||
           (!enable && (0 == pa_stream_is_corked(m_stream_playback))))) ?
               AM_STATE_OK : AM_STATE_ERROR;
      NOTICE("%s %s!", (enable ? "Pause" : "Resume"),
             ((AM_STATE_OK == state) ? "Succeeded" : "Failed"));
      if (AM_LIKELY((AM_STATE_OK == state) && !enable)) {
        op = pa_stream_trigger(m_stream_playback, nullptr, nullptr);
        if (AM_LIKELY(op)) {
          pa_operation_unref(op);
        }
      }
    }
    pa_threaded_mainloop_unlock(m_main_loop);
  } else {
    NOTICE("Invalid operation! Player is already stopped!");
    state = AM_STATE_BAD_COMMAND;
  }

  return state;
}

bool AMPlayerAudioPulse::is_player_running()
{
  return (m_is_player_started && m_stream_playback &&
      (PA_STREAM_READY == pa_stream_get_state(m_stream_playback)));
}

void AMPlayerAudioPulse::destroy()
{
  inherited::destroy();
}

AMPlayerAudioPulse::AMPlayerAudioPulse() :
    inherited(AUDIO_PLAYER_PULSE),
    m_main_loop(nullptr),
    m_context(nullptr),
    m_stream_playback(nullptr),
    m_lock(nullptr),
    m_lock_queue(nullptr),
    m_event(nullptr),
    m_write_data(nullptr),
    m_underflow(nullptr),
    m_drain_data(nullptr),
    m_stream_data(nullptr),
    m_audio_queue(nullptr),
    m_play_latency(0),
    m_underflow_count(0),
    m_context_state(PA_CONTEXT_UNCONNECTED),
    m_is_ctx_connected(false),
    m_is_player_started(false),
    m_is_draining(false)
{
  m_def_sink_name.clear();
  memset(&m_sample_spec, 0, sizeof(m_sample_spec));
  memset(&m_channel_map, 0, sizeof(m_channel_map));
  memset(&m_buffer_attr, 0, sizeof(m_buffer_attr));
  memset(&m_channel_volume, 0, sizeof(m_channel_volume));
}

AMPlayerAudioPulse::~AMPlayerAudioPulse()
{
  finalize();
  m_lock_queue->lock();
  while (m_audio_queue && !m_audio_queue->empty()) {
    m_audio_queue->front()->release();
    m_audio_queue->pop();
  }
  m_lock_queue->unlock();
  if (AM_LIKELY(m_lock)) {
    m_lock->destroy();
  }
  if (AM_LIKELY(m_lock_queue)) {
    m_lock_queue->destroy();
  }
  if (AM_LIKELY(m_event)) {
    m_event->destroy();
  }
  delete m_write_data;
  delete m_underflow;
  delete m_drain_data;
  delete m_stream_data;
  delete m_audio_config;
  delete m_audio_queue;
}

AM_STATE AMPlayerAudioPulse::init(const AudioPlayerConfig &config)
{
  AM_STATE state = AM_STATE_OK;
  do {
    m_audio_config = new AudioPlayerConfig(config);
    if (AM_UNLIKELY(nullptr == m_audio_config)) {
      state = AM_STATE_NO_MEMORY;
      ERROR("Failed to allocate memory for audio player config!");
      break;
    }

    if (AM_UNLIKELY(nullptr == (m_lock = AMSpinLock::create()))) {
      state = AM_STATE_NO_MEMORY;
      ERROR("Failed to create lock!");
      break;
    }

    if (AM_UNLIKELY(nullptr == (m_lock_queue = AMSpinLock::create()))) {
      state = AM_STATE_NO_MEMORY;
      ERROR("Failed to create lock for packet queue!");
      break;
    }

    if (AM_UNLIKELY(nullptr == (m_event = AMEvent::create()))) {
      state = AM_STATE_NO_MEMORY;
      ERROR("Failed to create event!");
      break;
    }

    if (AM_UNLIKELY(nullptr == (m_write_data = new PaData(this, nullptr)))) {
      state = AM_STATE_NO_MEMORY;
      ERROR("Failed to create write data!");
      break;
    }

    if (AM_UNLIKELY(nullptr == (m_underflow = new PaData(this, nullptr)))) {
      state = AM_STATE_NO_MEMORY;
      ERROR("Failed to create underflow data!");
      break;
    }

    if (AM_UNLIKELY(nullptr == (m_drain_data = new PaData(this, nullptr)))) {
      state = AM_STATE_NO_MEMORY;
      ERROR("Failed to create drain data!");
      break;
    }

    if (AM_UNLIKELY(nullptr == (m_stream_data = new PaData(this, nullptr)))) {
      state = AM_STATE_NO_MEMORY;
      ERROR("Failed to create stream data!");
      break;
    }

    if (AM_UNLIKELY(nullptr == (m_audio_queue = new PacketQueue()))) {
      state = AM_STATE_NO_MEMORY;
      ERROR("Failed to create audio queue!");
      break;
    }
  }while(0);

  return state;
}

bool AMPlayerAudioPulse::start_player()
{
  bool ret = true;
  AUTO_SPIN_LOCK(m_lock);
  do {
    const pa_cvolume *volume = (m_audio_config->initial_volume < 0) ?
        nullptr : (const pa_cvolume*)&m_channel_volume;
    PaData info(this, nullptr);
    pa_operation *op = nullptr;
    pa_operation_state_t opstate;

    if (AM_UNLIKELY(!initialize())) {
      ERROR("Failed to initialize audio player!");
      ret = false;
      break;
    }

    /* Check Server Info */
    pa_threaded_mainloop_lock(m_main_loop);
    info.data = m_main_loop;
    op = pa_context_get_server_info(m_context,
                                    static_pa_server_info,
                                    &info);
    while((opstate = pa_operation_get_state(op)) != PA_OPERATION_DONE) {
      if (AM_UNLIKELY(opstate == PA_OPERATION_CANCELLED)) {
        NOTICE("Get server info operation cancelled!");
        break;
      }
      pa_threaded_mainloop_wait(m_main_loop);
    }
    pa_operation_unref(op);
    pa_threaded_mainloop_unlock(m_main_loop);
    if (AM_UNLIKELY(opstate != PA_OPERATION_DONE)) {
      ERROR("Failed to get audio server information!");
      ret = false;
      break;
    }

    if (AM_UNLIKELY(m_def_sink_name.empty())) {
      ERROR("System doesn't have audio output device! Abort!");
      ret = false;
      break;
    }

    if (AM_LIKELY(m_audio_config->enable_aec)) {
      /* Check Sink Info */
      bool found = false;
      std::string::size_type pos =
          m_def_sink_name.find(".echo-cancel", m_def_sink_name.size() -
                               strlen(".echo-cancel"));
      std::string aec_sink_name = m_def_sink_name;
      std::string def_sink_name = m_def_sink_name;
      if (AM_LIKELY(pos == std::string::npos)) {
        aec_sink_name.append(".echo-cancel");
      } else {
        def_sink_name = def_sink_name.substr(0, m_def_sink_name.size() -
                                             strlen(".echo-cancel"));
      }

      pa_threaded_mainloop_lock(m_main_loop);
      info.data = &found;
      op = pa_context_get_sink_info_by_name(m_context,
                                            aec_sink_name.c_str(),
                                            static_pa_sink_info,
                                            &info);
      while((opstate = pa_operation_get_state(op)) != PA_OPERATION_DONE) {
        if (AM_UNLIKELY(opstate == PA_OPERATION_CANCELLED)) {
          NOTICE("Get sink info operation cancelled!");
          break;
        }
        pa_threaded_mainloop_wait(m_main_loop);
      }
      pa_operation_unref(op);
      pa_threaded_mainloop_unlock(m_main_loop);
      if (AM_LIKELY(opstate != PA_OPERATION_DONE)) {
        ERROR("Failed to get sink information!");
        ret = false;
        break;
      }
      m_def_sink_name = found ? aec_sink_name : def_sink_name;
    }

    m_stream_playback = pa_stream_new(m_context,
                                      "AudioStream",
                                      &m_sample_spec,
                                      &m_channel_map);
    if (AM_UNLIKELY(!m_stream_playback)) {
      ERROR("Failed to create playback stream: %s",
            pa_strerror(pa_context_errno(m_context)));
      ret = false;
      break;
    }
    int retval = -1;
    m_buffer_attr.fragsize  = ((uint32_t)-1);
    m_buffer_attr.maxlength = pa_usec_to_bytes(m_play_latency,
                                                  &m_sample_spec) * 2;
    m_buffer_attr.minreq    = ((uint32_t)-1);
    m_buffer_attr.prebuf    = pa_usec_to_bytes(m_play_latency,
                                                  &m_sample_spec) / 2;
    m_buffer_attr.tlength   = pa_usec_to_bytes(m_play_latency,
                                                  &m_sample_spec);
    pa_stream_set_underflow_callback(m_stream_playback,
                                     static_pa_underflow,
                                     m_underflow);
    pa_stream_set_write_callback(m_stream_playback,
                                 static_pa_write,
                                 m_write_data);
    if (AM_UNLIKELY(pa_stream_connect_playback(
        m_stream_playback,
        m_def_sink_name.c_str(),
        &m_buffer_attr,
        ((pa_stream_flags)(PA_STREAM_INTERPOLATE_TIMING |
                           PA_STREAM_ADJUST_LATENCY     |
                           PA_STREAM_AUTO_TIMING_UPDATE)),
        volume,
        nullptr) < 0)) {
      ERROR("Failed to connect to playback stream!");
      ret = false;
      break;
    } else {
      pa_stream_state_t playState;
      while(PA_STREAM_READY !=
          (playState = pa_stream_get_state(m_stream_playback))) {
        if (AM_UNLIKELY((playState == PA_STREAM_FAILED) ||
                        (playState == PA_STREAM_TERMINATED))) {
          break;
        }
      }
      if (AM_LIKELY(PA_STREAM_READY == playState)) {
        const pa_buffer_attr *bufAttr =
            pa_stream_get_buffer_attr(m_stream_playback);
        m_is_player_started = true;
        INFO("\n Client request max length: %u"
            "\n        min require length: %u"
            "\n         pre buffer length: %u"
            "\n             target length: %u"
            "\nServer returned max length: %u"
            "\n        min require length: %u"
            "\n         pre buffer length: %u"
            "\n             target length: %u",
            m_buffer_attr.maxlength,
            m_buffer_attr.minreq,
            m_buffer_attr.prebuf,
            m_buffer_attr.tlength,
            bufAttr->maxlength,
            bufAttr->minreq,
            bufAttr->prebuf,
            bufAttr->tlength);
        memcpy(&m_buffer_attr, bufAttr, sizeof(m_buffer_attr));
        if (AM_LIKELY(m_audio_config->initial_volume >= 0)) {
          pa_context_set_sink_input_volume(
              m_context, pa_stream_get_index(m_stream_playback),
              (const pa_cvolume*)&m_channel_volume, nullptr, nullptr);
        }
      } else {
        ERROR("Failed to connect playback stream to PulseAudio server: %s",
              pa_strerror(retval));
      }
    }

  }while(0);

  return ret;
}

bool AMPlayerAudioPulse::initialize()
{
  bool ret = false;

  do {
    PaData st(this, nullptr);
    if (AM_LIKELY(!m_main_loop)) {
      m_main_loop = pa_threaded_mainloop_new();
      if (AM_UNLIKELY(!m_main_loop)) {
        ERROR("Failed to create threaded mainloop!");
        break;
      }
      st.data = m_main_loop;
      m_context = pa_context_new(pa_threaded_mainloop_get_api(m_main_loop),
                                    (const char*) "AudioPlayback");
      if (AM_UNLIKELY(!m_context)) {
        ERROR("Failed to create PulseAudio context!");
        break;
      }

      pa_context_set_state_callback(m_context, static_pa_state, &st);
      pa_context_connect(m_context, nullptr, /* Default PulseAudio server */
                         PA_CONTEXT_NOFLAGS, nullptr);
      pa_threaded_mainloop_start(m_main_loop);
      pa_threaded_mainloop_lock(m_main_loop);
      /* will be signaled in pa_state() */
      pa_threaded_mainloop_wait(m_main_loop);
      pa_threaded_mainloop_unlock(m_main_loop);
      /* Disable context state callback, not used in the future */
      pa_context_set_state_callback(m_context, nullptr, nullptr);

      m_is_ctx_connected = (PA_CONTEXT_READY == m_context_state);
      if (AM_UNLIKELY(!m_is_ctx_connected)) {
        if ((PA_CONTEXT_TERMINATED == m_context_state) ||
            (PA_CONTEXT_FAILED == m_context_state)) {
          pa_threaded_mainloop_lock(m_main_loop);
          pa_context_disconnect(m_context);
          pa_threaded_mainloop_unlock(m_main_loop);
        }
        ERROR("pa_context_connect failed: %u!", m_context_state);
        break;
      }
    } else {
      INFO("Threaded mainloop is already created!");
    }
    ret = m_is_ctx_connected;
  }while(0);

  if (AM_UNLIKELY(!ret && m_main_loop)) {
    pa_threaded_mainloop_stop(m_main_loop);
  }

  return ret;
}

void AMPlayerAudioPulse::finalize()
{
  if (AM_LIKELY(m_stream_playback)) {
    pa_stream_disconnect(m_stream_playback);
    pa_stream_unref(m_stream_playback);
    m_stream_playback = nullptr;
  }

  if (AM_LIKELY(m_is_ctx_connected)) {
    pa_context_set_state_callback(m_context, nullptr, nullptr);
    pa_context_disconnect(m_context);
    m_is_ctx_connected = false;
  }

  if (AM_LIKELY(m_context)) {
    pa_context_unref(m_context);
    m_context = nullptr;
  }

  if (AM_LIKELY(m_main_loop)) {
    pa_threaded_mainloop_free(m_main_loop);
    m_main_loop = nullptr;
  }

  m_def_sink_name.clear();
  m_play_latency = 20000;
  m_underflow_count = 0;
  m_is_player_started = false;
  m_is_draining = false;
  INFO("PulseAudio's resources are released!");
}

inline bool AMPlayerAudioPulse::is_queue_empty(PacketQueue &queue)
{
  AUTO_SPIN_LOCK(m_lock_queue);
  return queue.empty();
}

inline void AMPlayerAudioPulse::pop_queue(PacketQueue &queue)
{
  AUTO_SPIN_LOCK(m_lock_queue);
  queue.pop();
}

inline void AMPlayerAudioPulse::pa_state(pa_context *context, void *data)
{
  if (AM_LIKELY(context)) {
    m_context_state = pa_context_get_state(context);
  }
  if (AM_LIKELY(!context ||
                (m_context_state == PA_CONTEXT_READY) ||
                (m_context_state == PA_CONTEXT_FAILED) ||
                (m_context_state == PA_CONTEXT_TERMINATED))) {
    pa_threaded_mainloop_signal((pa_threaded_mainloop*)data, 0);
  }
}

inline void AMPlayerAudioPulse::pa_server_info_cb(pa_context *context,
                                                  const pa_server_info *info,
                                                  void *data)
{
  m_def_sink_name = info->default_sink_name ? info->default_sink_name : "";

  INFO("   Audio Server Information:");
  INFO("             Server Version: %s",   info->server_version);
  INFO("                Server Name: %s",   info->server_name);
  INFO("        Default Source Name: %s",   info->default_source_name);
  INFO("          Default Sink Name: %s",   info->default_sink_name);
  INFO("                  Host Name: %s",   info->host_name);
  INFO("                  User Name: %s",   info->user_name);
  INFO("                   Channels: %hhu", info->sample_spec.channels);
  INFO("                       Rate: %u",   info->sample_spec.rate);
  INFO("                 Frame Size: %u",   pa_frame_size(&info->sample_spec));
  INFO("                Sample Size: %u",   pa_sample_size(&info->sample_spec));
  INFO("Default ChannelMap Channels: %hhu", info->channel_map.channels);
  for (uint32_t i = 0; i < info->channel_map.channels; ++ i) {
    INFO("               Channel[%2u]: %s",
         i, pa_channel_position_to_string(info->channel_map.map[i]));
  }
  INFO("        This channel map is: %s",
         pa_channel_map_valid((const pa_channel_map*)info->channel_map.map) ?
             "Valid" : "Invalid");

  memcpy(&m_channel_map, &info->channel_map, sizeof(m_channel_map));

  switch(m_sample_spec.channels) {
    case 1: {
      pa_channel_map_init_mono(&m_channel_map);
    }break;
    case 2: {
      pa_channel_map_init_stereo(&m_channel_map);
    }break;
    default: {
      pa_channel_map_init_auto(&m_channel_map,
                               m_sample_spec.channels,
                               PA_CHANNEL_MAP_DEFAULT);
    }break;
  }
  INFO("                     Cookie: %08x", info->cookie);
  INFO("          Audio Information:");
  INFO("                   Channels: %hhu", m_sample_spec.channels);
  INFO("                Sample Rate: %u",   m_sample_spec.rate);
  pa_threaded_mainloop_signal((pa_threaded_mainloop*)data, 0);
}

inline void AMPlayerAudioPulse::pa_sink_info_cb(pa_context *conext,
                                                const pa_sink_info *info,
                                                int eol,
                                                void *data)
{
  if (AM_LIKELY(eol == 0)) {
    INFO("Audio Echo Cancellation is enabled!");
    INFO("Echo Cancellation Sink Name: %s", info->name);
    INFO("%s", info->description);
    *((bool*)data) = true;
  } else if (AM_LIKELY(eol < 0)) {
    WARN("Cannot find Audio Echo Cancellation Sink!");
    WARN("module-echo-cancel.so is probably not loaded by PulseAudio Server!");
  }

  if (AM_LIKELY(eol != 0)) {
    pa_threaded_mainloop_signal(m_main_loop, 0);
  }
}

inline void AMPlayerAudioPulse::pa_write(pa_stream *stream,
                                         size_t bytes,
                                         void *data)
{
  AMPacket     *packet = nullptr;
  uint8_t     *dataPtr = nullptr;
  uint8_t   *dataWrite = nullptr;
  uint32_t   availSize = 0;
  uint32_t  dataOffset = 0;
  uint32_t writtenSize = 0;
  size_t      dataSize = bytes;
  int              ret = 0;

  if (AM_LIKELY(0 == (ret = pa_stream_begin_write(stream,
                                                  (void**)&dataWrite,
                                                  &dataSize)))) {
    bool needDrain = false;
    while(writtenSize < dataSize) {
      if (AM_UNLIKELY(is_queue_empty(*m_audio_queue))) {
        /* If audio packet queue is empty, just feed audio server empty data */
        memset(dataWrite + writtenSize, 0, dataSize - writtenSize);
        writtenSize = dataSize;
        continue;
      }
      packet = m_audio_queue->front();
      if (AM_LIKELY(packet->get_type() == AMPacket::AM_PAYLOAD_TYPE_DATA)) {
        dataOffset = packet->get_data_offset();
        dataPtr = (uint8_t*)(packet->get_data_ptr() + dataOffset);
        availSize = packet->get_data_size() - dataOffset;
        availSize = (availSize < (dataSize - writtenSize)) ? availSize :
            (dataSize - writtenSize);
        memcpy(dataWrite + writtenSize, dataPtr, availSize);
        writtenSize += availSize;
        dataOffset += availSize;
        if (AM_LIKELY(dataOffset == packet->get_data_size())) {
          pop_queue(*m_audio_queue);
          packet->set_data_offset(0);
          packet->release();
        } else {
          packet->set_data_offset(dataOffset);
        }
      } else if (packet->get_type() == AMPacket::AM_PAYLOAD_TYPE_EOF) {
        needDrain = true;
        pop_queue(*m_audio_queue);
        packet->release();
        break;
      }
    }
    if (AM_LIKELY(writtenSize > 0)) {
      pa_stream_write(stream, dataWrite,
                      writtenSize, nullptr,
                      0, PA_SEEK_RELATIVE);
    } else {
      pa_stream_cancel_write(stream);
    }
    if (AM_UNLIKELY(needDrain)) {
      pa_operation *op = nullptr;
      m_is_draining = true;
      /* Disable underflow callback when draining the buffer */
      pa_stream_set_underflow_callback(stream, nullptr, nullptr);
      pa_stream_set_write_callback(stream, nullptr, nullptr);
      op = pa_stream_drain(stream, static_pa_drain, m_drain_data);
      if (AM_LIKELY(op)) {
        pa_operation_unref(op);
      } else {
        m_event->signal();
      }
      NOTICE("Draining...");
    }
  } else {
    ERROR("Failed to begin writting: %s", pa_strerror(ret));
  }
}

inline void AMPlayerAudioPulse::pa_underflow(pa_stream *stream, void *data)
{
  if (AM_UNLIKELY((++ m_underflow_count >= 6) && (m_play_latency < 5000000))) {
    m_play_latency = m_play_latency * 2;
    m_buffer_attr.maxlength = pa_usec_to_bytes(m_play_latency,
                                                  &m_sample_spec);
    m_buffer_attr.tlength = pa_usec_to_bytes(m_play_latency,
                                                &m_sample_spec);
    pa_stream_set_buffer_attr(stream,
                              (const pa_buffer_attr*)&m_buffer_attr,
                              nullptr,
                              nullptr);
    m_underflow_count = 0;
    NOTICE("Underflow, increase latency to %u ms!", m_play_latency / 1000);
  }
}

inline void AMPlayerAudioPulse::pa_drain(pa_stream *stream,
                                         int success,
                                         void *data)
{
  m_event->signal();
}

void AMPlayerAudioPulse::static_pa_state(pa_context *context, void *data)
{
  ((PaData*)data)->adev->pa_state(context, ((PaData*)data)->data);
}

void AMPlayerAudioPulse::static_pa_server_info(pa_context *context,
                                               const pa_server_info *info,
                                               void *data)
{
  ((PaData*)data)->adev->pa_server_info_cb(context, info,
                                           ((PaData*)data)->data);
}

void AMPlayerAudioPulse::static_pa_sink_info(pa_context *context,
                                             const pa_sink_info *info,
                                             int eol,
                                             void *data)
{
  ((PaData*)data)->adev->pa_sink_info_cb(context, info, eol,
                                         ((PaData*)data)->data);
}

void AMPlayerAudioPulse::static_pa_underflow(pa_stream *stream, void *data)
{
  ((PaData*)data)->adev->pa_underflow(stream, ((PaData*)data)->data);
}

void AMPlayerAudioPulse::static_pa_drain(pa_stream *stream,
                                         int success, void *data)
{
  ((PaData*)data)->adev->pa_drain(stream, success, ((PaData*)data)->data);
}

void AMPlayerAudioPulse::static_pa_write(pa_stream *stream,
                                         size_t bytes, void *data)
{
  ((PaData*)data)->adev->pa_write(stream, bytes, ((PaData*)data)->data);
}

/*
 * AMPlayerAudio
 */
AMPlayerAudio* AMPlayerAudio::get_player(const AudioPlayerConfig& config)
{
  AMPlayerAudio *player = nullptr;
  if (is_str_equal(config.interface.c_str(), "pulse")) {
    player = AMPlayerAudioPulse::create(config);
  } else if (is_str_equal(config.interface.c_str(), "alsa")) {
    ERROR("Audio player using ALSA is not implemented yet!");
  } else {
    player = nullptr;
    ERROR("Unknown audio player interface type: %s!",
          config.interface.c_str());
  }

  return player;
}
