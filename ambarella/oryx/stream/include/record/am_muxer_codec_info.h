/*******************************************************************************
 * am_muxer_info.h
 *
 * History:
 *   2014-12-18 - [ccjing] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/
#ifndef AM_MUXER_CODEC_INFO_H_
#define AM_MUXER_CODEC_INFO_H_
typedef int64_t    AM_PTS;

enum AM_STREAM_DATA_TYPE
{
  AUDIO_STREAM = 0x1 << 0,
  VIDEO_STREAM = 0x1 << 1,
};

enum AM_MUXER_ATTR {
  AM_MUXER_FILE_NORMAL      = 0,
  AM_MUXER_FILE_EVENT       = 1,
  AM_MUXER_EXPORT_NORMAL    = 2,
  AM_MUXER_NETWORK_NORMAL   = 3,
};

enum AM_MUXER_CODEC_TYPE
{
  AM_MUXER_CODEC_INVALID,
  AM_MUXER_CODEC_UNKNOWN,
  AM_MUXER_CODEC_MP4,
  AM_MUXER_CODEC_TS,
  AM_MUXER_CODEC_EXPORT,
};

struct AMMuxerCodecConfig
{
    AM_MUXER_CODEC_TYPE  type;
    uint32_t             size;
    char                 data[];
};

struct AMDateTime
{
  uint16_t year;
  uint8_t month;
  uint8_t day;

  uint8_t hour;
  uint8_t minute;
  uint8_t seconds;
};

/*struct AMMuxerCodecDataInfo
{
  uint64_t pts, dts;

  uint8_t is_key_frame;
  uint8_t reserved0;
  uint8_t has_pts;
  uint8_t has_dts;
};
*/
struct AMMuxerCodecSpecifiedInfo
{
  uint8_t enable_video;
  uint8_t enable_audio;
  uint8_t b_video_const_framerate;
  uint8_t b_video_have_b_frame;

  uint8_t video_format;
  uint8_t audio_format;
  uint8_t reserved0;
  uint8_t reserved1;

  uint32_t timescale;
  uint32_t video_frametick;
  uint32_t audio_frametick;

  AMDateTime create_time;
  AMDateTime modification_time;

  char video_compressorname[32];
  char audio_compressorname[32];
} ;

#endif /* AM_MUXER_CODEC_INFO_H_ */
