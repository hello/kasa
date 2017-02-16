/*******************************************************************************
 * am_player_audio.h
 *
 * History:
 *   2014-9-11 - [ypchang] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/
#ifndef AM_PLAYER_AUDIO_H_
#define AM_PLAYER_AUDIO_H_

#include <queue>
#include <pulse/pulseaudio.h>

enum AUDIO_PLAYER_INTERFACE
{
  AUDIO_PLAYER_UNKNOWN,
  AUDIO_PLAYER_PULSE,
  AUDIO_PLAYER_ALSA,
};

typedef std::queue<AMPacket*> PacketQueue;

/*
 * AMPlayerAudio
 */
struct AM_AUDIO_INFO;
struct AudioPlayerConfig;
class AMPlayerAudio
{
  public:
    static AMPlayerAudio* get_player(const AudioPlayerConfig &config);

  public:
    virtual void add_packet(AMPacket *packet) = 0;
#if 0
    virtual bool feed_data() = 0;
#endif
    virtual AM_STATE start(AM_AUDIO_INFO& audioinfo) = 0;
    virtual AM_STATE stop(bool wait = true)  = 0;
    virtual AM_STATE pause(bool enable) = 0;
    virtual bool is_player_running() = 0;
    virtual void destroy() {delete this;}

  protected:
    AMPlayerAudio(AUDIO_PLAYER_INTERFACE type) :
      m_chunk_bytes(0),
      m_interface_type(type),
      m_audio_config(NULL){}
    virtual ~AMPlayerAudio(){}

  protected:
    uint32_t               m_chunk_bytes;
    AUDIO_PLAYER_INTERFACE m_interface_type;
    AudioPlayerConfig     *m_audio_config;
};

/*
 * AMPlayerAudioPulse
 */
struct PaData;
class AMEvent;
class AMSpinLock;
class AMPlayerAudio;
class AMPlayerAudioPulse: public AMPlayerAudio
{
    typedef AMPlayerAudio inherited;
  public:
    static AMPlayerAudioPulse* create(const AudioPlayerConfig &config);

  public:
    virtual void add_packet(AMPacket *packet);
#if 0
    virtual bool feed_data();
#endif
    virtual AM_STATE start(AM_AUDIO_INFO& audioinfo);
    virtual AM_STATE stop(bool wait = true);
    virtual AM_STATE pause(bool enable);
    virtual bool is_player_running();
    virtual void destroy();

  private:
    AMPlayerAudioPulse();
    virtual ~AMPlayerAudioPulse();
    AM_STATE init(const AudioPlayerConfig &config);

  private:
    bool start_player();
    bool initialize();
    void finalize();

  private:
    inline bool is_queue_empty(PacketQueue &queue);
    inline void pop_queue(PacketQueue &queue);

  private:
    inline void pa_state(pa_context *context, void *data);
    inline void pa_server_info_cb(pa_context *context,
                                  const pa_server_info *info,
                                  void *data);
    inline void pa_sink_info_cb(pa_context *conext,
                                const pa_sink_info *info,
                                int eol,
                                void *data);
    inline void pa_write(pa_stream *stream, size_t bytes, void *data);
    inline void pa_underflow(pa_stream *stream, void *data);
    inline void pa_drain(pa_stream *stream, int success, void *data);

  private:
    static void static_pa_state(pa_context *context, void *data);
    static void static_pa_server_info(pa_context *context,
                                      const pa_server_info *info,
                                      void *data);
    static void static_pa_sink_info(pa_context *context,
                                    const pa_sink_info *info,
                                    int eol,
                                    void *data);
    static void static_pa_underflow(pa_stream *stream, void *data);
    static void static_pa_drain(pa_stream *stream, int success, void *data);
    static void static_pa_write(pa_stream *stream, size_t bytes, void *data);

  private:
    pa_threaded_mainloop *m_main_loop;
    pa_context           *m_context;
    pa_stream            *m_stream_playback;
    AMSpinLock           *m_lock;
    AMSpinLock           *m_lock_queue;
    AMEvent              *m_event;
    PaData               *m_write_data;
    PaData               *m_underflow;
    PaData               *m_drain_data;
    PaData               *m_stream_data;
    PacketQueue          *m_audio_queue;
    std::string           m_def_sink_name;
    uint32_t              m_play_latency;
    uint32_t              m_underflow_count;
    pa_sample_spec        m_sample_spec;
    pa_channel_map        m_channel_map;
    pa_buffer_attr        m_buffer_attr;
    pa_cvolume            m_channel_volume;
    pa_context_state      m_context_state;
    bool                  m_is_ctx_connected;
    bool                  m_is_player_started;
    bool                  m_is_draining;
};

#endif /* AM_PLAYER_AUDIO_H_ */
