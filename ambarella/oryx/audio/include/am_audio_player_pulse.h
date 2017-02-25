/*******************************************************************************
 * am_audio_player_pulse.h
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
#ifndef AM_AUDIO_PLAYER_PULSE_H_
#define AM_AUDIO_PLAYER_PULSE_H_

#include "am_event.h"
#include "am_mutex.h"
#include "am_queue.h"
#include "am_audio_player_if.h"
#include <pulse/pulseaudio.h>

struct PaData;

class AMAudioPlayerPulse: public AMIAudioPlayer
{
  public:
    static AMIAudioPlayer* create(void *owner,
                                  const std::string& name,
                                  AudioPlayerCallback callback);

  public:
    virtual void destroy();
    virtual void set_echo_cancel_enabled(bool enabled);
    virtual bool start(int32_t volume = -1);
    virtual bool stop(bool wait = true);
    virtual bool pause(bool enable);
    virtual bool set_volume(int32_t volume);
    virtual bool is_player_running();
    virtual void set_audio_info(AM_AUDIO_INFO &ainfo);
    virtual void set_player_default_latency(uint32_t ms);
    virtual void set_player_callback(AudioPlayerCallback callback);

  private:
    bool start_player(int32_t volume);
    bool initialize();
    void finalize();

  private:
    void pa_state(pa_context *context, void *data);
    void pa_server_info_cb(pa_context *context,
                           const pa_server_info *info,
                           void *data);
    void pa_sink_info_cb(pa_context *conext,
                         const pa_sink_info *info,
                         int eol,
                         void *data);
    void pa_set_volume_cb(pa_context *context, int success, void *data);
    void pa_write(pa_stream *stream, size_t bytes, void *data);
    void pa_underflow(pa_stream *stream, void *data);
    void pa_drain(pa_stream *stream, int success, void *data);
    void pa_ctx_drain(pa_context *context, void *data);
    void pa_stream_state(pa_stream *stream, void *data);

  private:
    static void static_player_pa_state(pa_context *context, void *data);
    static void static_player_pa_server_info(pa_context *context,
                                             const pa_server_info *info,
                                             void *data);
    static void static_player_pa_sink_info(pa_context *context,
                                           const pa_sink_info *info,
                                           int eol,
                                           void *data);
    static void static_player_pa_underflow(pa_stream *stream, void *data);
    static void static_player_pa_drain(pa_stream *stream,
                                       int success,
                                       void *data);
    static void static_player_pa_context_drain(pa_context *stream, void *data);
    static void static_player_pa_write(pa_stream *stream,
                                       size_t bytes,
                                       void *data);
    static void static_player_pa_stream_state(pa_stream *stream, void *data);
    static void static_player_set_volume_callback(pa_context *context,
                                                  int success,
                                                  void *data);

  private:
    AMAudioPlayerPulse();
    virtual ~AMAudioPlayerPulse();
    bool init(void *owner,
              const std::string& name,
              AudioPlayerCallback callback);

  private:
    AudioPlayerCallback   m_player_callback   = nullptr;
    void                 *m_owner             = nullptr;
    pa_threaded_mainloop *m_main_loop         = nullptr;
    pa_context           *m_context           = nullptr;
    pa_stream            *m_stream_playback   = nullptr;
    AMEvent              *m_event             = nullptr;
    PaData               *m_write_data        = nullptr;
    PaData               *m_underflow         = nullptr;
    PaData               *m_drain_data        = nullptr;
    PaData               *m_stream_data       = nullptr;
    uint32_t              m_play_latency_us   = 0;
    uint32_t              m_underflow_count   = 0;
    pa_context_state_t    m_context_state     = PA_CONTEXT_UNCONNECTED;
    pa_stream_state_t     m_stream_state      = PA_STREAM_UNCONNECTED;
    std::atomic<bool>     m_is_ctx_connected  = {false};
    std::atomic<bool>     m_is_player_started = {false};
    std::atomic<bool>     m_is_draining       = {false};
    std::atomic<bool>     m_is_restart_needed = {false};
    std::atomic<bool>     m_is_aec_enable     = {true};
    AM_AUDIO_INFO         m_audio_info;
    std::string           m_context_name;
    std::string           m_def_sink_name;
    pa_sample_spec        m_sample_spec;
    pa_channel_map        m_channel_map;
    pa_buffer_attr        m_buffer_attr;
    pa_cvolume            m_stream_volume;
    AMMemLock             m_lock;
    AMMemLock             m_lock_cb;
    AMMemLock             m_lock_info;
};
#endif /* AM_AUDIO_PLAYER_PULSE_H_ */
