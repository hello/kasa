/*******************************************************************************
 * am_muxer_info.h
 *
 * History:
 *   2014-12-18 - [ccjing] created file
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
  AM_MUXER_CODEC_AV3,
  AM_MUXER_CODEC_TS,
  AM_MUXER_CODEC_RTP,
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
