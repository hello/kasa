/*******************************************************************************
 * am_audio_capture_pulse.h
 *
 * History:
 *   2014-11-28 - [ypchang] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/
#ifndef AM_AUDIO_CAPTURE_PULSE_H_
#define AM_AUDIO_CAPTURE_PULSE_H_

#include <pulse/pulseaudio.h>

struct PaData;

class AMSpinLock;
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
    void pa_read(pa_stream *stream, size_t bytes, void *data);
    void pa_over_flow(pa_stream *stream, void *data);

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
    AudioCaptureCallback  m_capture_callback;
    AMSpinLock           *m_lock;
    void                 *m_owner;     /* Ower of this class object */
    PaData               *m_read_data;
    PaData               *m_over_flow;
    pa_threaded_mainloop *m_main_loop;
    pa_mainloop_api      *m_main_loop_api;
    pa_context           *m_context;
    pa_stream            *m_stream_record;
    uint8_t              *m_audio_buffer;
    uint8_t              *m_audio_ptr_r;
    uint8_t              *m_audio_ptr_w;
    pa_sample_spec        m_sample_spec;
    pa_channel_map        m_channel_map;
    pa_context_state      m_context_state;
    pa_sample_format_t    m_sample_format;
    uint32_t              m_sample_rate;
    uint32_t              m_channel;
    uint32_t              m_chunk_bytes;
    uint32_t              m_audio_buffer_size;
    uint32_t              m_frame_bytes;
    int64_t               m_last_pts;
    int64_t               m_fragment_pts;
    int                   m_hw_timer_fd;
    bool                  m_is_context_connected;
    bool                  m_is_capture_running;
    bool                  m_is_aec_enabled;
    std::string           m_context_name;
    std::string           m_def_src_name;
};

#endif /* AM_AUDIO_CAPTURE_PULSE_H_ */
