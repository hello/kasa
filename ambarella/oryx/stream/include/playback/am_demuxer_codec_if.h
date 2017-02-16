/*******************************************************************************
 * am_demuxer_codec_if.h
 *
 * History:
 *   2014-9-1 - [ypchang] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
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
