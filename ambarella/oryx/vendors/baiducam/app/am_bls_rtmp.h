/*
 * am_bls_rtmp.h
 *
 *  History:
 *    Mar 25, 2015 - [Shupeng Ren] created file
 *
 * Copyright (C) 2007-2015, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#ifndef _BLS_RTMP_H_
#define _BLS_RTMP_H_

#include "am_event.h"
#include "am_thread.h"
#include "bls_librtmp.h"

struct AM_RTMP_VideoMetaData
{
    uint32_t width;
    uint32_t height;
    uint32_t framerate; //fps
    uint32_t bitrate;
    uint32_t videocodecid;
};

struct AM_RTMP_AudioMetaData
{
    uint32_t samplerate;
    uint32_t samplesize;
    uint32_t channel;
    uint32_t stereo;
    uint32_t bitrate;
    uint32_t audiocodecid;
};

typedef bool (*AM_BLS_USRCMD_CALLBACK)(const char *cmd, void *arg);

using std::pair;
class AMBLSRtmp {
  public:
    static AMBLSRtmp* create();
    void destroy();
  public:
    AMBLSRtmp();
    ~AMBLSRtmp();

  public:
    bool init();
    bool connect(std::string &url,
                 std::string &devid,
                 std::string &access_token,
                 std::string &stream_name,
                 std::string &session_token);
    bool disconnect();
    bool isconnected();
    bool write_meta();
    bool write_video(char *data, int32_t size, uint64_t pts);
    bool write_audio(char *data, int32_t size, uint64_t pts);
    void set_audio_meta(AM_RTMP_AudioMetaData *data);
    void set_video_meta(AM_RTMP_VideoMetaData *data);
    bool process_usr_cmd(AM_BLS_USRCMD_CALLBACK cb, void *data);

  private:
    bls_rtmp_t                m_ortmp;

    AM_RTMP_VideoMetaData     m_video_meta;
    AM_RTMP_AudioMetaData     m_audio_meta;

    uint32_t                  m_server_time;
    int32_t                   m_stream_id;
    double                    m_proflag;
    pair<bool, uint32_t>      m_first_pts;
};

#endif /* _BLS_RTMP_H_ */
