/*******************************************************************************
 * am_audio_capture_pulse.h
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
#ifndef AM_AUDIO_CAPTURE_PULSE_H_
#define AM_AUDIO_CAPTURE_PULSE_H_

#include "am_mutex.h"
#include "am_audio_capture_if.h"
#include <pulse/pulseaudio.h>

struct PaData;
class AMAudioCapturePulse: public AMIAudioCapture
{
  public:
    static AMIAudioCapture* create(void *owner,
                                   const std::string& name,
                                   AudioCaptureCallback callback);

  public:
    virtual bool set_capture_callback(AudioCaptureCallback callback);
    virtual void destroy();
    virtual void set_echo_cancel_enabled(bool enabled);
    virtual bool start(int32_t volume = -1);
    virtual bool stop();
    virtual bool set_channel(uint32_t channel);
    virtual bool set_sample_rate(uint32_t sample_rate);
    virtual bool set_chunk_bytes(uint32_t chunk_bytes);
    virtual bool set_sample_format(AM_AUDIO_SAMPLE_FORMAT format);
    virtual bool set_volume(uint32_t volume);
    virtual uint32_t get_channel();
    virtual uint32_t get_sample_rate();
    virtual uint32_t get_chunk_bytes();
    virtual uint32_t get_sample_size();
    virtual int64_t  get_chunk_pts();
    virtual AM_AUDIO_SAMPLE_FORMAT get_sample_format();

  private:
    void pa_state(pa_context *context, void *data);
    void pa_server_info_cb(pa_context *context,
                           const pa_server_info *info,
                           void *data);
    void pa_source_info_cb(pa_context *context,
                           const pa_source_info *info,
                           int eol,
                           void *data);
    void pa_set_volume_cb(pa_context *context, int success, void *data);
    void pa_read(pa_stream *stream, size_t bytes, void *data);
    void pa_over_flow(pa_stream *stream, void *data);
    void pa_stream_state(pa_stream *stream, void *data);

  private:
    static void static_pa_state(pa_context *context, void *data);
    static void static_pa_server_info(pa_context *context,
                                      const pa_server_info *info,
                                      void *data);
    static void static_pa_source_info(pa_context *context,
                                      const pa_source_info *info,
                                      int eol,
                                      void *data);
    static void static_pa_read(pa_stream *stream, size_t bytes, void *data);
    static void static_pa_over_flow(pa_stream *stream, void *data);
    static void static_pa_stream_state(pa_stream *stream, void *data);
    static void static_set_volume_callback(pa_context *context,
                                           int success,
                                           void *data);

  private:
    bool initialize();
    void finalize();
    int64_t get_current_pts();
    uint32_t get_available_data_size();

  private:
    AMAudioCapturePulse();
    virtual ~AMAudioCapturePulse();
    bool init(void *owner,
              const std::string& name,
              AudioCaptureCallback callback);

  private:
    int64_t               m_last_pts;
    int64_t               m_fragment_pts;
    AudioCaptureCallback  m_capture_callback;
    void                 *m_owner;     /* Ower of this class object */
    PaData               *m_read_data;
    PaData               *m_over_flow;
    PaData               *m_stream_data;
    pa_threaded_mainloop *m_main_loop;
    pa_context           *m_context;
    pa_stream            *m_stream_record;
    uint8_t              *m_audio_buffer;
    uint8_t              *m_audio_ptr_r;
    uint8_t              *m_audio_ptr_w;
    pa_context_state_t    m_context_state;
    pa_stream_state_t     m_stream_state;
    pa_sample_format_t    m_sample_format;
    uint32_t              m_sample_rate;
    uint32_t              m_channel;
    uint32_t              m_chunk_bytes;
    uint32_t              m_audio_buffer_size;
    uint32_t              m_frame_bytes;
    int                   m_hw_timer_fd;
    std::atomic<bool>     m_is_context_connected;
    std::atomic<bool>     m_is_capture_running;
    std::atomic<bool>     m_is_aec_enabled;
    std::string           m_context_name;
    std::string           m_def_src_name;
    pa_sample_spec        m_sample_spec;
    pa_channel_map        m_channel_map;
    AMMemLock             m_lock;
};

#endif /* AM_AUDIO_CAPTURE_PULSE_H_ */
