/*******************************************************************************
 * am_audio_capture_pulse.cpp
 *
 * History:
 *   2014-11-28 - [ypchang] created file
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
#include "am_audio_capture_pulse.h"

#define HW_TIMER ((const char*)"/proc/ambarella/ambarella_hwtimer")

struct PaData
{
    AMAudioCapturePulse *adev;
    void                *data;
    PaData(AMAudioCapturePulse *dev, void *userdata) :
      adev(dev),
      data(userdata)
    {}
    PaData() :
      adev(nullptr),
      data(nullptr)
    {}
};

AMIAudioCapture* AMAudioCapturePulse::create(void *owner,
                                             const std::string& name,
                                             AudioCaptureCallback callback)
{
  AMAudioCapturePulse *result = new AMAudioCapturePulse();
  if (AM_UNLIKELY(result && (!result->init(owner, name, callback)))) {
    delete result;
    result = nullptr;
  }

  return ((AMIAudioCapture*)result);
}

bool AMAudioCapturePulse::set_capture_callback(AudioCaptureCallback callback)
{
  AUTO_MEM_LOCK(m_lock);
  INFO("Audio capture callback of %s is set to %p",
       m_context_name.c_str(), callback);
  m_capture_callback = callback;
  return (nullptr != m_capture_callback);
}

void AMAudioCapturePulse::destroy()
{
  delete this;
}

void AMAudioCapturePulse::set_echo_cancel_enabled(bool enabled)
{
  m_is_aec_enabled = enabled;
}

bool AMAudioCapturePulse::start(int32_t volume)
{
  bool ret = true;
  AUTO_MEM_LOCK(m_lock);
  do {
    pa_operation *paOp = nullptr;
    pa_operation_state_t opState;
    PaData info(this, nullptr);
    pa_buffer_attr bufAttr = { (uint32_t) -1 };
    std::string stream_record_name = m_context_name + ".record";

    if (AM_UNLIKELY(!initialize())) {
      ERROR("Failed to initialize audio capture of %s", m_context_name.c_str());
      ret = false;
      break;
    }

    /* Check Server Info */
    pa_threaded_mainloop_lock(m_main_loop);
    info.data = m_main_loop;
    paOp = pa_context_get_server_info(m_context,
                                      static_pa_server_info,
                                      &info);
    if (AM_LIKELY(paOp)) {
      while ((opState = pa_operation_get_state(paOp)) != PA_OPERATION_DONE) {
        if (AM_LIKELY(opState == PA_OPERATION_CANCELLED)) {
          NOTICE("Get server info operation cancelled!");
          break;
        }
        pa_threaded_mainloop_wait(m_main_loop);
      }
      pa_operation_unref(paOp);
    } else {
      ERROR("Failed to get server info!");
      opState = PA_OPERATION_CANCELLED;
    }
    pa_threaded_mainloop_unlock(m_main_loop);

    if (AM_UNLIKELY(opState != PA_OPERATION_DONE)) {
      ERROR("Failed to get audio server information!");
      ret = false;
      break;
    }

    if (AM_UNLIKELY(m_def_src_name.empty())) {
      ERROR("System doesn't have audio source device! Abort!");
      ret = false;
      break;
    }

    if (AM_LIKELY(m_is_aec_enabled)) {
      bool found = false;
      std::string::size_type pos = m_def_src_name.find(".echo-cancel",
                                                       m_def_src_name.size() -
                                                       strlen(".echo-cancel"));
      std::string aec_source_name = m_def_src_name;
      std::string def_source_name = m_def_src_name;
      if (AM_LIKELY(pos == std::string::npos)) {
        aec_source_name.append(".echo-cancel");
      } else {
        def_source_name = def_source_name.substr(0, m_def_src_name.size() -
                                                    strlen(".echo-cancel"));
      }

      pa_threaded_mainloop_lock(m_main_loop);
      info.data = &found;
      paOp = pa_context_get_source_info_by_name(m_context,
                                                aec_source_name.c_str(),
                                                static_pa_source_info,
                                                &info);
      if (AM_LIKELY(paOp)) {
        while ((opState = pa_operation_get_state(paOp)) != PA_OPERATION_DONE) {
          if (AM_LIKELY(opState == PA_OPERATION_CANCELLED)) {
            WARN("Getting source information operation is cancelled!");
            break;
          }
          pa_threaded_mainloop_wait(m_main_loop);
        }
        pa_operation_unref(paOp);
      } else {
        opState = PA_OPERATION_CANCELLED;
      }
      pa_threaded_mainloop_unlock(m_main_loop);

      if (AM_UNLIKELY(opState != PA_OPERATION_DONE)) {
        ERROR("Failed to get information of source %s!",
              aec_source_name.c_str());
        ret = false;
        break;
      }
      m_def_src_name = found ? aec_source_name : def_source_name;
    }

    m_stream_record = pa_stream_new(m_context,
                                    stream_record_name.c_str(),
                                    &m_sample_spec,
                                    &m_channel_map);
    if (AM_UNLIKELY(!m_stream_record)) {
      ERROR("Failed to create record stream %s: %s",
            stream_record_name.c_str(),
            pa_strerror(pa_context_errno(m_context)));
      ERROR("Call set_sample_rate(), set_sample_format(), "
            "set_channel() and set_chunk_bytes() before calling start()!");
      ret = false;
      break;
    }
    bufAttr.fragsize = m_chunk_bytes * 5;
    m_read_data->data = m_main_loop;
    m_over_flow->data = m_main_loop;
    m_stream_data->data = m_main_loop;

    pa_stream_set_overflow_callback(m_stream_record,
                                    static_pa_over_flow,
                                    m_over_flow);
    pa_stream_set_state_callback(m_stream_record,
                                 static_pa_stream_state,
                                 m_stream_data);
    if (AM_UNLIKELY(pa_stream_connect_record(
        m_stream_record,
        m_def_src_name.c_str(),
        &bufAttr,
        ((pa_stream_flags)(PA_STREAM_INTERPOLATE_TIMING |
                           PA_STREAM_ADJUST_LATENCY     |
                           PA_STREAM_AUTO_TIMING_UPDATE))) < 0)) {
      ERROR("Failed connecting record stream %s to audio server!",
            stream_record_name.c_str());
      ret = false;
      break;
    } else {
      pa_threaded_mainloop_lock(m_main_loop);
      while((m_stream_state = pa_stream_get_state(m_stream_record)) !=
            PA_STREAM_READY) {
        if (AM_UNLIKELY((m_stream_state == PA_STREAM_FAILED) ||
                        (m_stream_state == PA_STREAM_TERMINATED))) {
          break;
        }
        pa_threaded_mainloop_wait(m_main_loop);
      }
      /* Disable stream state callback */
      pa_stream_set_state_callback(m_stream_record, nullptr, nullptr);
      pa_threaded_mainloop_unlock(m_main_loop);

      switch(m_stream_state) {
        case PA_STREAM_READY: {
          const pa_buffer_attr *attr = nullptr;
          std::string stream_mainloop_name = m_context_name + ".cap";
          pa_threaded_mainloop_lock(m_main_loop);
          pa_threaded_mainloop_set_name(m_main_loop,
                                        stream_mainloop_name.c_str());
          attr = pa_stream_get_buffer_attr(m_stream_record);
          pa_threaded_mainloop_unlock(m_main_loop);
          m_is_capture_running = true;
          if (AM_LIKELY((volume > 0) && !set_volume(volume))) {
            ERROR("Failed to set volume to %d", volume);
          }
          if (AM_LIKELY(attr)) {
            INFO("Client requested fragment size : %u", bufAttr.fragsize);
            INFO("Server returned fragment size  : %u", attr->fragsize);
          } else {
            ret = false;
            ERROR("Failed to get buffer's attribute of stream %s!",
                  stream_record_name.c_str());
          }
          pa_stream_set_read_callback(m_stream_record,
                                      static_pa_read,
                                      m_read_data);
        }break;
        default: {
          ret = false;
          ERROR("Failed to connect record stream %s to PulseAudio server!",
                stream_record_name.c_str());
        }break;
      }
    }
  }while(0);

  return ret;
}

bool AMAudioCapturePulse::stop()
{
  AUTO_MEM_LOCK(m_lock);
  if (AM_LIKELY(m_is_capture_running)) {
    pa_threaded_mainloop_stop(m_main_loop);
    finalize();
    m_is_capture_running = false;
  } else {
    NOTICE("Audio capture of %s is already stopped!", m_context_name.c_str());
  }

  return !m_is_capture_running;
}

bool AMAudioCapturePulse::set_channel(uint32_t channel)
{
  AUTO_MEM_LOCK(m_lock);
  INFO("Audio channel of %s is set to %u.", m_context_name.c_str(), channel);
  m_channel = channel;
  m_sample_spec.channels = m_channel;
  return (m_channel > 0);
}

bool AMAudioCapturePulse::set_sample_rate(uint32_t sample_rate)
{
  AUTO_MEM_LOCK(m_lock);
  INFO("Audio sample rate of %s is set to %u.",
       m_context_name.c_str(), sample_rate);
  m_sample_rate = sample_rate;
  m_sample_spec.rate = m_sample_rate;
  return (m_sample_rate > 0);
}

bool AMAudioCapturePulse::set_chunk_bytes(uint32_t chunk_bytes)
{
  AUTO_MEM_LOCK(m_lock);
  INFO("Audio chunk bytes of %s is set to %u.",
       m_context_name.c_str(), chunk_bytes);
  m_chunk_bytes = chunk_bytes;
  return (m_chunk_bytes > 0);
}

bool AMAudioCapturePulse::set_sample_format(AM_AUDIO_SAMPLE_FORMAT format)
{
  AUTO_MEM_LOCK(m_lock);
  bool ret = true;
  switch(format) {
    case AM_SAMPLE_U8: {
      m_sample_format = PA_SAMPLE_U8;
    }break;
    case AM_SAMPLE_ALAW: {
      m_sample_format = PA_SAMPLE_ALAW;
    }break;
    case AM_SAMPLE_ULAW: {
      m_sample_format = PA_SAMPLE_ULAW;
    }break;
    case AM_SAMPLE_S16LE: {
      m_sample_format = PA_SAMPLE_S16LE;
    }break;
    case AM_SAMPLE_S16BE: {
      m_sample_format = PA_SAMPLE_S16BE;
    }break;
    case AM_SAMPLE_S32LE: {
      m_sample_format = PA_SAMPLE_S32LE;
    }break;
    case AM_SAMPLE_S32BE: {
      m_sample_format = PA_SAMPLE_S32BE;
    }break;
    default: {
      m_sample_format = PA_SAMPLE_INVALID;
      ret = false;
    }break;
  }
  m_sample_spec.format = m_sample_format;

  return ret;
}

bool AMAudioCapturePulse::set_volume(uint32_t volume)
{
  bool ret = false;

  if (AM_LIKELY(m_context && m_stream_record &&
                (volume > 0) && (volume <= 100))) {
    PaData volumeData(this, &ret);
    pa_cvolume cvolume = {0};
    pa_operation *op = nullptr;
    pa_operation_state_t opState = PA_OPERATION_CANCELLED;

    cvolume.channels = m_channel_map.channels;

    for (uint8_t i = 0; i < cvolume.channels; ++ i) {
      cvolume.values[i] = PA_VOLUME_NORM * volume / 100;
    }

    pa_threaded_mainloop_lock(m_main_loop);
    op = pa_context_set_source_output_volume(
        m_context,
        pa_stream_get_index(m_stream_record),
        &cvolume,
        static_set_volume_callback,
        (void*)&volumeData);
    if (AM_LIKELY(op)) {
      pa_threaded_mainloop_wait(m_main_loop);
      pa_threaded_mainloop_accept(m_main_loop);
      opState = pa_operation_get_state(op);
      if (AM_LIKELY(opState == PA_OPERATION_CANCELLED)) {
        WARN("Setting source volume is cancelled!");
        ret = false;
      }
      pa_operation_unref(op);
    } else {
      ret = false;
    }
    pa_threaded_mainloop_unlock(m_main_loop);
    if (AM_LIKELY(ret)) {
      NOTICE("Volume of record stream %s is set to %u",
             m_context_name.c_str(), volume);
    }
  } else if ((volume == 0) || (volume > 100)) {
    WARN("Volume(%u) is not in range(0 ~ 100)!", volume);
  }

  return ret;
}

uint32_t AMAudioCapturePulse::get_channel()
{
  AUTO_MEM_LOCK(m_lock);
  return m_channel;
}

uint32_t AMAudioCapturePulse::get_sample_rate()
{
  AUTO_MEM_LOCK(m_lock);
  return m_sample_rate;
}

uint32_t AMAudioCapturePulse::get_chunk_bytes()
{
  AUTO_MEM_LOCK(m_lock);
  return m_chunk_bytes;
}

uint32_t AMAudioCapturePulse::get_sample_size()
{
  AUTO_MEM_LOCK(m_lock);
  return pa_sample_size(&m_sample_spec);
}

int64_t AMAudioCapturePulse::get_chunk_pts()
{
  AUTO_MEM_LOCK(m_lock);
  m_frame_bytes  = pa_frame_size(&m_sample_spec) * m_sample_spec.rate;
  m_fragment_pts = ((int64_t)(90000LL * m_chunk_bytes) / m_frame_bytes);
  return m_fragment_pts;
}

AM_AUDIO_SAMPLE_FORMAT AMAudioCapturePulse::get_sample_format()
{
  AUTO_MEM_LOCK(m_lock);
  AM_AUDIO_SAMPLE_FORMAT format = AM_SAMPLE_INVALID;
  switch(m_sample_format) {
    case PA_SAMPLE_U8:
      format = AM_SAMPLE_U8;
      break;
    case PA_SAMPLE_ALAW:
      format = AM_SAMPLE_ALAW;
      break;
    case PA_SAMPLE_ULAW:
      format = AM_SAMPLE_ULAW;
      break;
    case PA_SAMPLE_S16LE:
      format = AM_SAMPLE_S16LE;
      break;
    case PA_SAMPLE_S16BE:
      format = AM_SAMPLE_S16BE;
      break;
    case PA_SAMPLE_S32LE:
      format = AM_SAMPLE_S32LE;
      break;
    case PA_SAMPLE_S32BE:
      format = AM_SAMPLE_S32BE;
      break;
    case PA_SAMPLE_INVALID:
    default:
      format = AM_SAMPLE_INVALID;
      break;
  }

  return format;
}

void AMAudioCapturePulse::pa_state(pa_context *context, void *data)
{
  DEBUG("pa_state is called!");
  pa_threaded_mainloop_signal((pa_threaded_mainloop*)data, 0);
}

void AMAudioCapturePulse::pa_server_info_cb(pa_context *context,
                                            const pa_server_info *info,
                                            void *data)
{
  INFO("========Audio Server Information========");
  INFO("       Server Version: %s",   info->server_version);
  INFO("          Server Name: %s",   info->server_name);
  INFO("  Default Source Name: %s",   info->default_source_name);
  INFO("    Default Sink Name: %s",   info->default_sink_name);
  INFO("            Host Name: %s",   info->host_name);
  INFO("            User Name: %s",   info->user_name);
  INFO("             Channels: %hhu", info->sample_spec.channels);
  INFO("                 Rate: %u",   info->sample_spec.rate);
  INFO("           Frame Size: %u",   pa_frame_size(&info->sample_spec));
  INFO("          Sample Size: %u",   pa_sample_size(&info->sample_spec));
  INFO("  ChannelMap Channels: %hhu", info->channel_map.channels);

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
  m_def_src_name = info->default_source_name ? info->default_source_name : "";
  INFO("==========Client Configuration==========");
  INFO("             Channels: %hhu", m_sample_spec.channels);
  INFO("                 Rate: %u",   m_sample_spec.rate);
  INFO("           Frame Size: %u",   pa_frame_size(&m_sample_spec));
  INFO("          Sample Size: %u",   pa_sample_size(&m_sample_spec));
  INFO("  ChannelMap Channels: %hhu", m_channel_map.channels);

  m_frame_bytes  = pa_frame_size(&m_sample_spec) * m_sample_spec.rate;
  if (AM_LIKELY(m_chunk_bytes > 0)) {
    m_audio_buffer_size = get_lcm(m_chunk_bytes, m_frame_bytes);
    m_audio_buffer = new uint8_t[m_audio_buffer_size];
    m_fragment_pts = ((int64_t)(90000ULL * m_chunk_bytes) / m_frame_bytes);
    INFO("         Fragment PTS: %llu", m_fragment_pts);

    if (AM_LIKELY(m_audio_buffer)) {
      m_audio_ptr_r = m_audio_buffer;
      m_audio_ptr_w = m_audio_buffer;
      INFO("Allocated %u bytes audio buffer, this will buffer %u seconds!",
           m_audio_buffer_size, m_audio_buffer_size / m_frame_bytes);
    } else {
      ERROR("Failed to allocate audio buffer!");
    }
  } else {
    ERROR("Invalid audio data chunk size %u bytes!", m_chunk_bytes);
  }

  pa_threaded_mainloop_signal((pa_threaded_mainloop*)data, 0);
}

void AMAudioCapturePulse::pa_source_info_cb(pa_context *context,
                                            const pa_source_info *info,
                                            int eol,
                                            void *data)
{
  if (AM_LIKELY(eol == 0)) {
    INFO("Audio Echo Cancellation is enabled!");
    INFO("Echo Cancellation Source Name: %s", info->name);
    INFO("%s", info->description);
    *((bool*)data) = true;
  } else if (AM_LIKELY(eol < 0)) {
    WARN("Cannot find Audio Echo Cancellation Source!");
    WARN("module-echo-cancel.so is probably not loaded by PulseAudio Server!");
    m_is_aec_enabled = false;
  }
  if (AM_LIKELY(0 != eol)) {
    pa_threaded_mainloop_signal(m_main_loop, 0);
  }
}

void AMAudioCapturePulse::pa_set_volume_cb(pa_context *context,
                                           int success,
                                           void *data)
{
  *((bool*)data) = (0 != success);
  if (AM_LIKELY(0 != success)) {
    NOTICE("Set volume done!");
  } else {
    WARN("Failed to set volume!");
  }
  pa_threaded_mainloop_signal(m_main_loop, 1);
}

void AMAudioCapturePulse::pa_read(pa_stream *stream, size_t bytes, void *data)
{
  const void *audio_data = nullptr;
  uint32_t avail_data_size = 0;
  int64_t current_pts = get_current_pts();

  if (AM_UNLIKELY(pa_stream_peek(stream, &audio_data, &bytes) < 0)) {
    ERROR("pa_stream_peek failed: %s",
          pa_strerror(pa_context_errno(m_context)));
  } else if (AM_LIKELY(audio_data && (bytes > 0))) {
    size_t remain = bytes;
    while (remain > 0) {
      size_t size = (size_t)(m_audio_ptr_w + remain - m_audio_buffer);
      size_t write_size = (size > m_audio_buffer_size) ?
          (m_audio_buffer_size + remain - size) : remain;
      memcpy(m_audio_ptr_w, ((uint8_t*)audio_data + bytes - remain),
             write_size);
      m_audio_ptr_w = m_audio_buffer +
          (((m_audio_ptr_w - m_audio_buffer) + write_size) %
              m_audio_buffer_size);
      remain -= write_size;
    }
  }
  if (AM_LIKELY(bytes > 0)) {
    pa_stream_drop(stream);
  }
  avail_data_size = get_available_data_size();
  m_last_pts = ((m_last_pts == 0) ?
      current_pts - ((avail_data_size * m_fragment_pts) / m_chunk_bytes) :
      m_last_pts);

  if (AM_LIKELY(avail_data_size >= m_chunk_bytes)) {
    int64_t real_pts_inc = current_pts - m_last_pts;
    int64_t curr_pts_seg = ((m_chunk_bytes * real_pts_inc) / avail_data_size);
    uint32_t packet_num = avail_data_size / m_chunk_bytes;

    for (uint32_t i = 0; i < packet_num; ++ i) {
      m_last_pts += curr_pts_seg;
      if (AM_LIKELY(m_owner && m_capture_callback)) {
        AudioCapture a_capture;
        a_capture.owner = m_owner;
        a_capture.packet.data = m_audio_ptr_r;
        a_capture.packet.length = m_chunk_bytes;
        a_capture.packet.pts = m_last_pts;
        m_capture_callback(&a_capture);
      }
      m_audio_ptr_r = m_audio_buffer +
          (((m_audio_ptr_r - m_audio_buffer) + m_chunk_bytes) %
              m_audio_buffer_size);
    }
  }
}

void AMAudioCapturePulse::pa_over_flow(pa_stream *stream, void *data)
{
  ERROR("Audio data over flow! Is I/O too slow?");
}

void AMAudioCapturePulse::pa_stream_state(pa_stream *stream, void *data)
{
  DEBUG("pa_stream_state is called!");
  pa_threaded_mainloop_signal((pa_threaded_mainloop*)data, 0);
}

void AMAudioCapturePulse::static_pa_state(pa_context *context, void *data)
{
  ((PaData*)data)->adev->pa_state(context, ((PaData*)data)->data);
}

void AMAudioCapturePulse::static_pa_server_info(pa_context *context,
                                                const pa_server_info *info,
                                                void *data)
{
  ((PaData*)data)->adev->pa_server_info_cb(context,
                                           info,
                                           ((PaData*)data)->data);
}

void AMAudioCapturePulse::static_pa_source_info(pa_context *context,
                                                const pa_source_info *info,
                                                int eol,
                                                void *data)
{
  ((PaData*)data)->adev->pa_source_info_cb(context, info, eol,
                                           ((PaData*)data)->data);
}

void AMAudioCapturePulse::static_pa_read(pa_stream *stream,
                                         size_t bytes,
                                         void *data)
{
  ((PaData*)data)->adev->pa_read(stream, bytes, ((PaData*)data)->data);
}

void AMAudioCapturePulse::static_pa_over_flow(pa_stream *stream, void *data)
{
  ((PaData*)data)->adev->pa_over_flow(stream, ((PaData*)data)->data);
}

void AMAudioCapturePulse::static_pa_stream_state(pa_stream *stream, void *data)
{
  ((PaData*)data)->adev->pa_stream_state(stream, ((PaData*)data)->data);
}

void AMAudioCapturePulse::static_set_volume_callback(pa_context *context,
                                                     int success,
                                                     void *data)
{
  ((PaData*)data)->adev->pa_set_volume_cb(context,
                                          success,
                                          ((PaData*)data)->data);
}

bool AMAudioCapturePulse::initialize()
{
  bool ret = false;

  do {
    if (AM_LIKELY(!m_main_loop)) {
      PaData stateData(this, nullptr);
      setenv("PULSE_RUNTIME_PATH", "/var/run/pulse/", 1);
      m_main_loop = pa_threaded_mainloop_new();
      if (AM_UNLIKELY(!m_main_loop)) {
        ERROR("Failed to create threaded mainloop!");
        break;
      }
      m_context = pa_context_new(pa_threaded_mainloop_get_api(m_main_loop),
                                 (const char*)m_context_name.c_str());
      if (AM_UNLIKELY(!m_context))  {
        ERROR("Failed to create context %s", m_context_name.c_str());
        break;
      }
      stateData.data = m_main_loop;
      pa_context_set_state_callback(m_context, static_pa_state, &stateData);
      pa_context_connect(m_context, nullptr, /* Default PulseAudio server */
                         PA_CONTEXT_NOFLAGS, nullptr);
      pa_threaded_mainloop_start(m_main_loop);
      pa_threaded_mainloop_lock(m_main_loop);
      while ((m_context_state = pa_context_get_state(m_context)) !=
             PA_CONTEXT_READY) {
        if (AM_UNLIKELY((m_context_state == PA_CONTEXT_FAILED) ||
                        (m_context_state == PA_CONTEXT_TERMINATED))) {
          break;
        }
        /* Will be signaled in pa_state()*/
        pa_threaded_mainloop_wait(m_main_loop);
      }
      pa_threaded_mainloop_unlock(m_main_loop);
      /* Disable context state callback, not used in the future */
      pa_context_set_state_callback(m_context, nullptr, nullptr);

      m_is_context_connected = (PA_CONTEXT_READY == m_context_state);
      if (AM_UNLIKELY(!m_is_context_connected)) {
        if (AM_LIKELY((PA_CONTEXT_TERMINATED == m_context_state) ||
                      (PA_CONTEXT_FAILED == m_context_state))) {
          pa_threaded_mainloop_lock(m_main_loop);
          pa_context_disconnect(m_context);
          pa_threaded_mainloop_unlock(m_main_loop);
        }
        ERROR("Failed to connect context %s", m_context_name.c_str());
        break;
      }
    } else {
      INFO("Threaded mainloop is already created!");
    }
    ret = m_is_context_connected;
  }while(0);

  if (AM_LIKELY(!ret && m_main_loop)) {
    pa_threaded_mainloop_stop(m_main_loop);
  }

  return ret;
}

void AMAudioCapturePulse::finalize()
{
  if (AM_LIKELY(m_stream_record)) {
    pa_stream_disconnect(m_stream_record);
    DEBUG("pa_stream_disconnect(m_stream_record)");
    pa_stream_unref(m_stream_record);
    m_stream_record = nullptr;
    DEBUG("pa_stream_unref(m_stream_record)");
  }

  if (AM_LIKELY(m_is_context_connected)) {
    pa_context_set_state_callback(m_context, nullptr, nullptr);
    pa_context_disconnect(m_context);
    m_is_context_connected = false;
  }

  if (AM_LIKELY(m_context)) {
    pa_context_unref(m_context);
    m_context = nullptr;
    DEBUG("pa_context_unref(m_context)");
  }

  if (AM_LIKELY(m_main_loop)) {
    pa_threaded_mainloop_free(m_main_loop);
    m_main_loop = nullptr;
    DEBUG("pa_threaded_mainloop_free(m_main_loop)");
  }

  delete[] m_audio_buffer;
  m_def_src_name.clear();
  m_audio_buffer = nullptr;
  m_audio_ptr_r = nullptr;
  m_audio_ptr_w = nullptr;
  m_is_capture_running = false;

  INFO("AudioCapturePulse's resources have been released!");
}

int64_t AMAudioCapturePulse::get_current_pts()
{
  char pts[32] = {0};
  int64_t cur_pts = m_last_pts;
  if (AM_LIKELY(m_hw_timer_fd >= 0)) {
    if (AM_UNLIKELY(read(m_hw_timer_fd, pts, sizeof(pts)) < 0)) {
      ERROR("read: %d %s", m_hw_timer_fd, strerror(errno));
    } else {
      cur_pts = strtoll(pts, nullptr, 10);
    }
  }

  return cur_pts;
}

uint32_t AMAudioCapturePulse::get_available_data_size()
{
  return (uint32_t)((m_audio_ptr_w >= m_audio_ptr_r) ?
      (m_audio_ptr_w - m_audio_ptr_r) :
      (m_audio_buffer_size + m_audio_ptr_w - m_audio_ptr_r));
}

AMAudioCapturePulse::AMAudioCapturePulse() :
    m_last_pts(0LL),
    m_fragment_pts(0LL),
    m_capture_callback(nullptr),
    m_owner(nullptr),
    m_read_data(nullptr),
    m_over_flow(nullptr),
    m_stream_data(nullptr),
    m_main_loop(nullptr),
    m_context(nullptr),
    m_stream_record(nullptr),
    m_audio_buffer(nullptr),
    m_audio_ptr_r(nullptr),
    m_audio_ptr_w(nullptr),
    m_context_state(PA_CONTEXT_UNCONNECTED),
    m_stream_state(PA_STREAM_UNCONNECTED),
    m_sample_format(PA_SAMPLE_INVALID),
    m_sample_rate(0),
    m_channel(0),
    m_chunk_bytes(0),
    m_audio_buffer_size(0),
    m_frame_bytes(0),
    m_hw_timer_fd(-1),
    m_is_context_connected(false),
    m_is_capture_running(false),
    m_is_aec_enabled(false)
{
  memset(&m_sample_spec, 0, sizeof(m_sample_spec));
  memset(&m_channel_map, 0, sizeof(m_channel_map));
  m_context_name.clear();
  m_def_src_name.clear();
}

AMAudioCapturePulse::~AMAudioCapturePulse()
{
  finalize();
  delete m_read_data;
  delete m_over_flow;
  delete m_stream_data;
  if (AM_LIKELY(m_hw_timer_fd >= 0)) {
    close(m_hw_timer_fd);
    m_hw_timer_fd = -1;
  }
}

bool AMAudioCapturePulse::init(void *owner,
                               const std::string& name,
                               AudioCaptureCallback callback)
{
  bool ret = false;
  do {
    m_owner = owner;
    m_capture_callback = callback;
    m_context_name = name;
    if (AM_UNLIKELY(!m_owner)) {
      ERROR("Invalid owner of this object!");
      break;
    }

    if (AM_UNLIKELY(!m_capture_callback)) {
      WARN("Audio capture callback function is not set!");
    }

    m_read_data = new PaData(this, nullptr);
    if (AM_UNLIKELY(!m_read_data)) {
      ERROR("Failed to create m_read_data!");
      break;
    }

    m_over_flow = new PaData(this, nullptr);
    if (AM_UNLIKELY(!m_over_flow)) {
      ERROR("Failed to create m_over_flow!");
      break;
    }

    m_stream_data = new PaData(this, nullptr);
    if (AM_UNLIKELY(!m_stream_data)) {
      ERROR("Failed to create m_stream_state_data!");
      break;
    }

    if (AM_UNLIKELY((m_hw_timer_fd = open(HW_TIMER, O_RDONLY)) < 0)) {
      ERROR("Failed to open %s: %s", HW_TIMER, strerror(errno));
      break;
    }

    ret = true;
  } while(0);
  return ret;
}
