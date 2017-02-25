/*******************************************************************************
 * am_audio_player_pulse.cpp
 *
 * History:
 *   Jul 8, 2016 - [ypchang] created file
 *
 * Copyright (c) 2015 Ambarella, Inc.
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

#include "am_audio_player_pulse.h"

struct PaData {
    AMAudioPlayerPulse *adev;
    void               *data;
    PaData(AMAudioPlayerPulse *dev, void *context) :
      adev(dev),
      data(context)
    {}
    PaData() :
      adev(nullptr),
      data(nullptr)
    {}
};

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

AMIAudioPlayer* AMAudioPlayerPulse::create(void *owner,
                                           const std::string &name,
                                           AudioPlayerCallback callback)
{
  AMAudioPlayerPulse *result = new AMAudioPlayerPulse();
  if (AM_UNLIKELY(result && (!result->init(owner, name, callback)))) {
    delete result;
    result = nullptr;
  }

  return ((AMIAudioPlayer*)result);
}

void AMAudioPlayerPulse::destroy()
{
  delete this;
}

void AMAudioPlayerPulse::set_echo_cancel_enabled(bool enabled)
{
  m_is_aec_enable = enabled;
}

bool AMAudioPlayerPulse::start(int32_t volume)
{
  AUTO_MEM_LOCK(m_lock);
  AUTO_MEM_LOCK(m_lock_info);
  m_sample_spec.channels = m_audio_info.channels;
  m_sample_spec.rate     = m_audio_info.sample_rate;
  m_sample_spec.format   =
      smp_fmt_to_pa_fmt(AM_AUDIO_SAMPLE_FORMAT(m_audio_info.sample_format));
  m_stream_volume.channels = m_sample_spec.channels;

  for (uint32_t i = 0; i < m_stream_volume.channels; ++ i) {
    m_stream_volume.values[i] = (volume >= 0) ?
        (PA_VOLUME_NORM * volume / 100) : PA_VOLUME_NORM;
  }
  if (AM_LIKELY(!m_is_player_started || m_is_restart_needed)) {
    if (AM_LIKELY(m_main_loop && m_stream_playback)) {
      pa_threaded_mainloop_lock(m_main_loop);
      pa_stream_disconnect(m_stream_playback);
      pa_stream_unref(m_stream_playback);
      pa_threaded_mainloop_unlock(m_main_loop);
      m_stream_playback = nullptr;
      m_is_player_started = false;
    }
    m_is_draining = false;
    m_is_player_started = start_player(volume);
  }
  return m_is_player_started.load();
}

bool AMAudioPlayerPulse::stop(bool wait)
{
  bool ret = true;
  AUTO_MEM_LOCK(m_lock);

  if (AM_LIKELY(m_is_player_started)) {
    if (AM_LIKELY(wait)) {
      m_event->wait();
      NOTICE("Player buffer is draining!");
      pa_operation *op = nullptr;
      pa_threaded_mainloop_lock(m_main_loop);
      op = pa_stream_drain(m_stream_playback,
                           static_player_pa_drain,
                           m_drain_data);
      while (op && (pa_operation_get_state(op) == PA_OPERATION_RUNNING)) {
        pa_threaded_mainloop_wait(m_main_loop);
      }
      pa_operation_unref(op);
      pa_threaded_mainloop_unlock(m_main_loop);
      NOTICE("Player buffer is drained!");
    }
    pa_threaded_mainloop_stop(m_main_loop);
    finalize();
  }

  return ret;
}

bool AMAudioPlayerPulse::pause(bool enable)
{
  AUTO_MEM_LOCK(m_lock);
  bool ret = true;

  if (AM_LIKELY(m_main_loop && m_stream_playback && m_is_player_started)) {
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
      ret = ((opState == PA_OPERATION_DONE) &&
          ((enable && (1 == pa_stream_is_corked(m_stream_playback))) ||
           (!enable && (0 == pa_stream_is_corked(m_stream_playback)))));
      NOTICE("%s %s!", (enable ? "Pause" : "Resume"),
             (ret ? "Succeeded" : "Failed"));
      if (AM_LIKELY(ret && !enable)) {
        op = pa_stream_trigger(m_stream_playback, nullptr, nullptr);
        if (AM_LIKELY(op)) {
          pa_operation_unref(op);
        }
      }
    }
    pa_threaded_mainloop_unlock(m_main_loop);
  } else {
    NOTICE("Invalid operation! Player is already stopped!");
    ret = false;
  }

  return ret;
}

bool AMAudioPlayerPulse::set_volume(int32_t volume)
{
  bool ret = false;
  if (AM_LIKELY(m_main_loop &&
                m_context &&
                m_stream_playback &&
                (volume <= 100))) {
    PaData volumeData(this, &ret);
    pa_cvolume cvolume = {0};
    pa_operation *op = nullptr;
    pa_operation_state_t opState = PA_OPERATION_CANCELLED;

    cvolume.channels = m_channel_map.channels;

    for (uint8_t i = 0; i < cvolume.channels; ++ i) {
      cvolume.values[i] = (volume >= 0) ? PA_VOLUME_NORM * volume / 100 :
          PA_VOLUME_NORM;
    }

    pa_threaded_mainloop_lock(m_main_loop);
    op = pa_context_set_sink_input_volume(
        m_context,
        pa_stream_get_index(m_stream_playback),
        &cvolume,
        static_player_set_volume_callback,
        (void*)&volumeData);
    if (AM_LIKELY(op)) {
      pa_threaded_mainloop_wait(m_main_loop);
      pa_threaded_mainloop_accept(m_main_loop);
      opState = pa_operation_get_state(op);
      if (AM_LIKELY(opState == PA_OPERATION_CANCELLED)) {
        WARN("Setting sink volume is cancelled!");
        ret = false;
      }
      pa_operation_unref(op);
    } else {
      ret = false;
    }
    pa_threaded_mainloop_unlock(m_main_loop);
    if (AM_LIKELY(ret)) {
      NOTICE("Volume of playback stream %s is set to %s",
             m_context_name.c_str(),
             (volume > 0) ? std::to_string(volume).c_str() : "default volume");
    }
  } else if (AM_UNLIKELY(volume > 100)) {
    WARN("Volume(%d) is not in range(0 ~ 100]!", volume);
  } else {
    WARN("%s is not running!", m_context_name.c_str());
  }

  return ret;
}

bool AMAudioPlayerPulse::is_player_running()
{
  bool ret = false;
  if (AM_LIKELY(m_main_loop)) {
    pa_threaded_mainloop_lock(m_main_loop);
    ret = (m_is_player_started && m_stream_playback &&
        (PA_STREAM_READY == pa_stream_get_state(m_stream_playback)));
    pa_threaded_mainloop_unlock(m_main_loop);
  }
  return ret;
}

void AMAudioPlayerPulse::set_audio_info(AM_AUDIO_INFO &ainfo)
{
  AUTO_MEM_LOCK(m_lock_info);
  m_is_restart_needed = ((m_audio_info.channels != ainfo.channels) ||
                         (m_audio_info.sample_rate != ainfo.sample_rate) ||
                         (m_audio_info.sample_format != ainfo.sample_format));
  memcpy(&m_audio_info, &ainfo, sizeof(m_audio_info));
}

void AMAudioPlayerPulse::set_player_default_latency(uint32_t ms)
{
  AUTO_MEM_LOCK(m_lock_info);
  m_play_latency_us = ms * 1000;
}

void AMAudioPlayerPulse::set_player_callback(AudioPlayerCallback callback)
{
  AUTO_MEM_LOCK(m_lock_cb);
  m_player_callback = callback;
}

bool AMAudioPlayerPulse::start_player(int32_t volume)
{
  bool ret = true;
  do {
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
                                    static_player_pa_server_info,
                                    &info);
    if (AM_LIKELY(op)) {
      while((opstate = pa_operation_get_state(op)) != PA_OPERATION_DONE) {
        if (AM_UNLIKELY(opstate == PA_OPERATION_CANCELLED)) {
          NOTICE("Get server info operation cancelled!");
          break;
        }
        pa_threaded_mainloop_wait(m_main_loop);
      }
      pa_operation_unref(op);
    } else {
      opstate = PA_OPERATION_CANCELLED;
    }
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

    if (AM_LIKELY(m_is_aec_enable)) {
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
                                            static_player_pa_sink_info,
                                            &info);
      if (AM_LIKELY(op)) {
        while((opstate = pa_operation_get_state(op)) != PA_OPERATION_DONE) {
          if (AM_UNLIKELY(opstate == PA_OPERATION_CANCELLED)) {
            NOTICE("Get sink info operation cancelled!");
            break;
          }
          pa_threaded_mainloop_wait(m_main_loop);
        }
        pa_operation_unref(op);
      } else {
        opstate = PA_OPERATION_CANCELLED;
      }
      pa_threaded_mainloop_unlock(m_main_loop);
      if (AM_LIKELY(opstate != PA_OPERATION_DONE)) {
        ERROR("Failed to get information of sink %s!",
              aec_sink_name.c_str());
        ret = false;
        break;
      }
      m_def_sink_name = found ? aec_sink_name : def_sink_name;
    }

    m_stream_playback = pa_stream_new(m_context,
                                      m_context_name.c_str(),
                                      &m_sample_spec,
                                      &m_channel_map);
    if (AM_UNLIKELY(!m_stream_playback)) {
      ERROR("Failed to create playback stream: %s",
            pa_strerror(pa_context_errno(m_context)));
      ret = false;
      break;
    }
    m_buffer_attr.maxlength = pa_usec_to_bytes(m_play_latency_us,
                                               &m_sample_spec) * 2;
    m_buffer_attr.minreq    = ((uint32_t)-1);
    m_buffer_attr.prebuf    = pa_usec_to_bytes(m_play_latency_us,
                                               &m_sample_spec);
    m_buffer_attr.tlength   = pa_usec_to_bytes(m_play_latency_us,
                                               &m_sample_spec);
    m_buffer_attr.fragsize  = m_buffer_attr.tlength;

    m_stream_data->data = m_main_loop;
    pa_stream_set_underflow_callback(m_stream_playback,
                                     static_player_pa_underflow,
                                     m_underflow);
    pa_stream_set_write_callback(m_stream_playback,
                                 static_player_pa_write,
                                 m_write_data);
    pa_stream_set_state_callback(m_stream_playback,
                                 static_player_pa_stream_state,
                                 m_stream_data);

    if (AM_UNLIKELY(pa_stream_connect_playback(
        m_stream_playback,
        m_def_sink_name.c_str(),
        &m_buffer_attr,
        ((pa_stream_flags)(PA_STREAM_INTERPOLATE_TIMING |
                           PA_STREAM_ADJUST_LATENCY     |
                           PA_STREAM_AUTO_TIMING_UPDATE)),
        &m_stream_volume,
        nullptr) < 0)) {
      ERROR("Failed to connect to playback stream!");
      ret = false;
      break;
    } else {
      pa_threaded_mainloop_lock(m_main_loop);
      while ((m_stream_state = pa_stream_get_state(m_stream_playback)) !=
             PA_STREAM_READY) {
        if (AM_UNLIKELY((m_stream_state == PA_STREAM_FAILED) ||
                        (m_stream_state == PA_STREAM_TERMINATED))) {
          break;
        }
        pa_threaded_mainloop_wait(m_main_loop);
      }
      pa_threaded_mainloop_unlock(m_main_loop);

      pa_stream_set_state_callback(m_stream_playback, nullptr, nullptr);
      switch(m_stream_state) {
        case PA_STREAM_READY: {
          const pa_buffer_attr *bufAttr = nullptr;
          std::string stream_mainloop_name = m_context_name + ".play";
          pa_threaded_mainloop_lock(m_main_loop);
          pa_threaded_mainloop_set_name(m_main_loop,
                                        stream_mainloop_name.c_str());
          bufAttr = pa_stream_get_buffer_attr(m_stream_playback);
          pa_threaded_mainloop_unlock(m_main_loop);
          if (AM_UNLIKELY(!set_volume(volume))) {
            ERROR("Failed to set %s volume to %s",
                  stream_mainloop_name.c_str(),
                  (volume >= 0) ?
                      std::to_string(volume).c_str() : "default volume");
          }
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
        }break;
        default: {
          ERROR("Failed to connect playback stream to PulseAudio server!");
        }break;
      }
    }

  }while(0);

  return ret;
}

bool AMAudioPlayerPulse::initialize()
{
  bool ret = false;

  do {
    PaData st(this, nullptr);
    if (AM_LIKELY(!m_main_loop)) {
      setenv("PULSE_RUNTIME_PATH", "/var/run/pulse/", 1);
      m_main_loop = pa_threaded_mainloop_new();
      if (AM_UNLIKELY(!m_main_loop)) {
        ERROR("Failed to create threaded mainloop!");
        break;
      }
      st.data = m_main_loop;
      m_context = pa_context_new(pa_threaded_mainloop_get_api(m_main_loop),
                                 m_context_name.c_str());
      if (AM_UNLIKELY(!m_context)) {
        ERROR("Failed to create context %s!", m_context_name.c_str());
        break;
      }

      pa_context_set_state_callback(m_context, static_player_pa_state, &st);
      pa_context_connect(m_context, nullptr, /* Default PulseAudio server */
                         PA_CONTEXT_NOFLAGS, nullptr);
      pa_threaded_mainloop_start(m_main_loop);
      pa_threaded_mainloop_lock(m_main_loop);
      while((m_context_state = pa_context_get_state(m_context)) !=
            PA_CONTEXT_READY) {
        if (AM_UNLIKELY((m_context_state == PA_CONTEXT_FAILED) ||
                        (m_context_state == PA_CONTEXT_TERMINATED))) {
          break;
        }
        /* will be signaled in pa_state() */
        pa_threaded_mainloop_wait(m_main_loop);
      }
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
        ERROR("Failed to connect context %s!", m_context_name.c_str());
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

void AMAudioPlayerPulse::finalize()
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
  m_play_latency_us = 20000;
  m_underflow_count = 0;
  m_is_player_started = false;
  m_is_draining = false;

  INFO("AudioPlayerPulse's resources have been released!");
}

void AMAudioPlayerPulse::pa_state(pa_context *context, void *data)
{
  DEBUG("pa_state is called!");
  pa_threaded_mainloop_signal((pa_threaded_mainloop*)data, 0);
}

void AMAudioPlayerPulse::pa_server_info_cb(pa_context *context,
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
    INFO("                Channel[%2u]: %s",
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

void AMAudioPlayerPulse::pa_sink_info_cb(pa_context *conext,
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

void AMAudioPlayerPulse::pa_set_volume_cb(pa_context *context,
                                          int success,
                                          void *data)
{
  *((bool*)data) = (0 != success);
  if (AM_LIKELY(0 != success)) {
    NOTICE("Set volume of %s is done!", m_context_name.c_str());
  } else {
    WARN("Failed to set volume of %s!", m_context_name.c_str());
  }
  pa_threaded_mainloop_signal(m_main_loop, 1);
}

void AMAudioPlayerPulse::pa_write(pa_stream *stream, size_t bytes, void *data)
{

  if (AM_LIKELY(!m_is_draining)) {
    int ret = 0;
    if (AM_LIKELY(0 == (ret = pa_stream_begin_write(stream,
                                                    &data,
                                                    &bytes)))) {
      AudioPlayer a_player;
      m_lock_cb.lock();
      if (AM_LIKELY(m_owner && m_player_callback)) {
        a_player.owner = m_owner;
        a_player.data.data = (uint8_t*)data;
        a_player.data.need_size = bytes;
        a_player.data.written_size = 0;
        a_player.data.drain = false;
        m_player_callback(&a_player); /* get audio data */
      } else {
        /* player callback is not set, just play silence */
        memset(a_player.data.data, 0, a_player.data.need_size);
        a_player.data.written_size = a_player.data.need_size;
      }
      m_lock_cb.unlock();

      if (AM_LIKELY(a_player.data.written_size > 0)) {
        pa_stream_write(stream, a_player.data.data,
                        a_player.data.written_size, nullptr,
                        0, PA_SEEK_RELATIVE);
      } else {
        pa_stream_cancel_write(stream);
      }

      if (AM_LIKELY(a_player.data.drain)) {
        m_is_draining = true;
        /* Disable write callback to make drain work */
        pa_stream_set_write_callback(stream, nullptr, nullptr);
        /* Disable underflow callback when draining the buffer */
        pa_stream_set_underflow_callback(stream, nullptr, nullptr);
        m_event->signal();
      }
    } else {
      ERROR("Failed to begin writing: %s", pa_strerror(ret));
    }
  }
}

void AMAudioPlayerPulse::pa_underflow(pa_stream *stream, void *data)
{
  if (AM_UNLIKELY((++ m_underflow_count >= 6) &&
                  (m_play_latency_us < 5000000))) {
    m_play_latency_us = m_play_latency_us * 2;
    m_buffer_attr.maxlength = pa_usec_to_bytes(m_play_latency_us,
                                               &m_sample_spec);
    m_buffer_attr.tlength = pa_usec_to_bytes(m_play_latency_us, &m_sample_spec);
    pa_stream_set_buffer_attr(stream,
                              (const pa_buffer_attr*)&m_buffer_attr,
                              nullptr,
                              nullptr);
    m_underflow_count = 0;
    NOTICE("Underflow, increase latency to %u ms!", m_play_latency_us / 1000);
  }
}

void AMAudioPlayerPulse::pa_drain(pa_stream *stream, int success, void *data)
{
  pa_operation *op = nullptr;
  if (AM_UNLIKELY(!success)) {
    ERROR("Failed to drain stream: %s",
          pa_strerror(pa_context_errno(m_context)));
    pa_threaded_mainloop_signal(m_main_loop, 0);
  } else {
    NOTICE("Stream Drained!");
    if (AM_LIKELY(m_stream_playback)) {
      pa_stream_disconnect(m_stream_playback);
      pa_stream_unref(m_stream_playback);
      m_stream_playback = nullptr;
    }
    NOTICE("Context Draining!");
    op = pa_context_drain(m_context,
                          static_player_pa_context_drain,
                          m_drain_data);
    if (AM_UNLIKELY(!op)) {
      pa_context_disconnect(m_context);
      m_is_ctx_connected = false;
      pa_threaded_mainloop_signal(m_main_loop, 0);
    } else {
      pa_operation_unref(op);
    }
  }
}

void AMAudioPlayerPulse::pa_ctx_drain(pa_context *context, void *data)
{
  if (AM_LIKELY(m_is_ctx_connected)) {
    if (AM_UNLIKELY(context != m_context)) {
      ERROR("Impossible!");
    }
    pa_context_disconnect(m_context);
    m_is_ctx_connected = false;
  }
  pa_threaded_mainloop_signal(m_main_loop, 0);
  NOTICE("Context Drained!");
}

void AMAudioPlayerPulse::pa_stream_state(pa_stream *stream, void *data)
{
  DEBUG("pa_stream_state is called!");
  pa_threaded_mainloop_signal((pa_threaded_mainloop*)data, 0);
}

void AMAudioPlayerPulse::static_player_pa_state(pa_context *context, void *data)
{
  ((PaData*)data)->adev->pa_state(context, ((PaData*)data)->data);
}

void AMAudioPlayerPulse::static_player_pa_server_info(
    pa_context *context,
    const pa_server_info *info,
    void *data)
{
  ((PaData*)data)->adev->pa_server_info_cb(context, info,
                                           ((PaData*)data)->data);
}

void AMAudioPlayerPulse::static_player_pa_sink_info(pa_context *context,
                                                    const pa_sink_info *info,
                                                    int eol,
                                                    void *data)
{
  ((PaData*)data)->adev->pa_sink_info_cb(context, info, eol,
                                         ((PaData*)data)->data);
}

void AMAudioPlayerPulse::static_player_pa_underflow(pa_stream *stream,
                                                    void *data)
{
  ((PaData*)data)->adev->pa_underflow(stream, ((PaData*)data)->data);
}
void AMAudioPlayerPulse::static_player_pa_drain(pa_stream *stream,
                                                int success,
                                                void *data)
{
  ((PaData*)data)->adev->pa_drain(stream, success, ((PaData*)data)->data);
}

void AMAudioPlayerPulse::static_player_pa_context_drain(pa_context *stream,
                                                        void *data)
{
  ((PaData*)data)->adev->pa_ctx_drain(stream, ((PaData*)data)->data);
}

void AMAudioPlayerPulse::static_player_pa_write(pa_stream *stream,
                                                size_t bytes,
                                                void *data)
{
  ((PaData*)data)->adev->pa_write(stream, bytes, ((PaData*)data)->data);
}

void AMAudioPlayerPulse::static_player_pa_stream_state(pa_stream *stream,
                                                       void *data)
{
  ((PaData*)data)->adev->pa_stream_state(stream, ((PaData*)data)->data);
}

void AMAudioPlayerPulse::static_player_set_volume_callback(pa_context *context,
                                                           int success,
                                                           void *data)
{
  ((PaData*)data)->adev->pa_set_volume_cb(context,
                                          success,
                                          ((PaData*)data)->data);
}


AMAudioPlayerPulse::AMAudioPlayerPulse()
{
  m_def_sink_name.clear();
  m_context_name.clear();
  memset(&m_sample_spec, 0, sizeof(m_sample_spec));
  memset(&m_channel_map, 0, sizeof(m_channel_map));
  memset(&m_buffer_attr, 0, sizeof(m_buffer_attr));
}

AMAudioPlayerPulse::~AMAudioPlayerPulse()
{
  stop(false);
  finalize();
  AM_DESTROY(m_event);
  delete m_write_data;
  delete m_underflow;
  delete m_drain_data;
  delete m_stream_data;
}

bool AMAudioPlayerPulse::init(void *owner,
                              const std::string& name,
                              AudioPlayerCallback callback)
{
  bool ret = false;

  do {
    m_owner = owner;
    m_player_callback = callback;
    m_context_name = name;

    if (AM_UNLIKELY(!m_owner)) {
      ERROR("Invalid owner of this object!");
      break;
    }

    if (AM_UNLIKELY(!m_player_callback)) {
      WARN("Audio capture callback function is not set!");
    }

    if (AM_UNLIKELY(nullptr == (m_event = AMEvent::create()))) {
      ERROR("Failed to create event!");
      break;
    }

    if (AM_UNLIKELY(nullptr == (m_write_data = new PaData(this, nullptr)))) {
      ERROR("Failed to create write data!");
      break;
    }

    if (AM_UNLIKELY(nullptr == (m_underflow = new PaData(this, nullptr)))) {
      ERROR("Failed to create underflow data!");
      break;
    }

    if (AM_UNLIKELY(nullptr == (m_drain_data = new PaData(this, nullptr)))) {
      ERROR("Failed to create drain data!");
      break;
    }

    if (AM_UNLIKELY(nullptr == (m_stream_data = new PaData(this, nullptr)))) {
      ERROR("Failed to create stream data!");
      break;
    }
    ret = true;

  } while(0);

  return ret;
}
