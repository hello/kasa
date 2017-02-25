/*******************************************************************************
 * am_demuxer_codec_if.h
 *
 * History:
 *   2014-9-1 - [ypchang] created file
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
#ifndef AM_DEMUXER_CODEC_IF_H_
#define AM_DEMUXER_CODEC_IF_H_

#include "am_playback_info.h"
enum AM_DEMUXER_TYPE
{
  AM_DEMUXER_INVALID,
  AM_DEMUXER_UNKNOWN,
  AM_DEMUXER_AAC,
  AM_DEMUXER_WAV,
  AM_DEMUXER_OGG,
  AM_DEMUXER_MP4,
  AM_DEMUXER_TS,
  AM_DEMUXER_ES,
  AM_DEMUXER_RTP,
  AM_DEMUXER_UNIX_DOMAIN,
};

enum AM_DEMUXER_STATE
{
  AM_DEMUXER_NONE,
  AM_DEMUXER_OK,
  AM_DEMUXER_NO_FILE,
  AM_DEMUXER_NO_PACKET
};

enum AM_DEMUXER_CONTENT_TYPE
{
  AM_DEMUXER_CONTENT_NONE     = 0,
  AM_DEMUXER_CONTENT_AUDIO    = 1 << 0,
  AM_DEMUXER_CONTENT_VIDEO    = 1 << 1,
  AM_DEMUXER_CONTENT_SUBTITLE = 1 << 2
};

class AMPacket;
class AMIDemuxerCodec
{
  public:
    virtual AM_DEMUXER_TYPE get_demuxer_type()             = 0;
    virtual void destroy()                                 = 0;
    virtual AM_DEMUXER_STATE get_packet(AMPacket*& packet) = 0;
    virtual void enable(bool enabled)                      = 0;
    virtual bool is_play_list_empty()                      = 0;
    virtual bool is_drained()                              = 0;
    virtual uint32_t stream_id()                           = 0;
    virtual uint32_t get_contents_type()                   = 0;
    virtual bool add_uri(const AMPlaybackUri& uri)         = 0;
    virtual ~AMIDemuxerCodec(){}
};

#ifdef __cplusplus
extern "C" {
#endif
AMIDemuxerCodec* get_demuxer_codec(uint32_t streamid);
AM_DEMUXER_TYPE  get_demuxer_codec_type();
#ifdef __cplusplus
}
#endif

typedef AMIDemuxerCodec* (*DemuxerCodecNew)(uint32_t id);
typedef AM_DEMUXER_TYPE  (*DemuxerCodecType)();

#define DEMUXER_CODEC_NEW  ((const char*)"get_demuxer_codec")
#define DEMUXER_CODEC_TYPE ((const char*)"get_demuxer_codec_type")

#endif /* AM_DEMUXER_CODEC_IF_H_ */
