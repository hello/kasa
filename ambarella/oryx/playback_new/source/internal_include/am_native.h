/**
 * am_native.h
 *
 * History:
 *  2015/08/07 - [Zhi He] create file
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
 */

#ifndef __AM_NATIVE_H__
#define __AM_NATIVE_H__

//common include

#include "stdlib.h"
#include "stdio.h"
#include "string.h"

#include "am_playback_new_if.h"

#define DInvalidSocketHandler (-1)
#define DIsSocketHandlerValid(x) (0 <= x)

#define DLEFOURCC(a, b, c, d) (((unsigned int) a) | ((unsigned int) (b << 8)) | ((unsigned int) (c << 16)) | ((unsigned int) (d << 24)))
#define DBEFOURCC(a, b, c, d) (((unsigned int) a << 24) | ((unsigned int) (b << 16)) | ((unsigned int) (c << 8)) | ((unsigned int) d))
#define DUSE_LITTLE_ENDIAN
#ifdef DUSE_LITTLE_ENDIAN
#define DFOURCC DLEFOURCC
#else
#define DFOURCC DBEFOURCC
#endif

#define DLikely(x)   __builtin_expect(!!(x),1)
#define DUnlikely(x)   __builtin_expect(!!(x),0)

#define DMaxStreamNumber  16

enum TimeUnit {
  TimeUnitDen_90khz = 90000,
  TimeUnitDen_27mhz = 27000000,
  TimeUnitDen_ms = 1000,
  TimeUnitDen_us = 1000000,
  TimeUnitDen_ns = 1000000000,
  TimeUnitNum_fps29dot97 = 3003,
};

enum VideoFrameRate {
  VideoFrameRate_MAX = 1024,
  VideoFrameRate_29dot97 = VideoFrameRate_MAX + 1,
  VideoFrameRate_59dot94,
  VideoFrameRate_23dot96,
  VideoFrameRate_240,
  VideoFrameRate_60,
  VideoFrameRate_30,
  VideoFrameRate_24,
  VideoFrameRate_15,
};

enum StreamTransportType {
  StreamTransportType_Invalid = 0,
  StreamTransportType_RTP,
};

enum ProtocolType {
  ProtocolType_Invalid = 0,
  ProtocolType_UDP,
  ProtocolType_TCP,
};

enum StreamingServerType {
  StreamingServerType_Invalid = 0,
  StreamingServerType_RTSP,
  StreamingServerType_RTMP,
  StreamingServerType_HTTP,
};

enum StreamingServerMode {
  StreamingServerMode_MulticastSetAddr = 0, //live streamming/boardcast
  StreamingServerMode_Unicast, //vod
  StreamingServerMode_MultiCastPickAddr, //join conference
};

enum ContainerType {
  ContainerType_Invalid = 0,
  ContainerType_AUTO = 1,
  ContainerType_MP4,
  ContainerType_3GP,
  ContainerType_TS,
  ContainerType_MOV,
  ContainerType_MKV,
  ContainerType_AVI,
  ContainerType_AMR,

  ContainerType_TotolNum,
};

enum PixFormat {
  PixFormat_YUV420P = 0,
  PixFormat_NV12,
  PixFormat_RGB565,
};

enum PrivateDataType {
  PrivateDataType_GPSInfo = 0,
  PrivateDataType_SensorInfo,
};

enum EntropyType {
  EntropyType_NOTSet = 0,
  EntropyType_H264_CABAC,
  EntropyType_H264_CAVLC,
};

enum AudioSampleFMT {
  AudioSampleFMT_NONE = -1,
  AudioSampleFMT_U8,          ///< unsigned 8 bits
  AudioSampleFMT_S16,         ///< signed 16 bits
  AudioSampleFMT_S32,         ///< signed 32 bits
  AudioSampleFMT_FLT,         ///< float
  AudioSampleFMT_DBL,         ///< double
  AudioSampleFMT_NB           ///< Number of sample formats. DO NOT USE if linking dynamically
};

enum MuxerSavingFileStrategy {
  MuxerSavingFileStrategy_Invalid = 0x00,
  MuxerSavingFileStrategy_AutoSeparateFile = 0x01, //muxer will auto separate file itself, with more time accuracy
  MuxerSavingFileStrategy_ManuallySeparateFile = 0x02,//engine/app will management the separeting file
  MuxerSavingFileStrategy_ToTalFile = 0x03,//not recommanded, cannot store file like .mp4 if storage have not enough space
};

enum MuxerSavingCondition {
  MuxerSavingCondition_Invalid = 0x00,
  MuxerSavingCondition_InputPTS = 0x01,
  MuxerSavingCondition_CalculatedPTS = 0x02,
  MuxerSavingCondition_FrameCount = 0x03,
};

enum MuxerAutoFileNaming {
  MuxerAutoFileNaming_Invalid = 0x00,
  MuxerAutoFileNaming_ByDateTime = 0x01,
  MuxerAutoFileNaming_ByNumber = 0x02,
};

typedef struct {
  unsigned int pic_width;
  unsigned int pic_height;
  unsigned int pic_offset_x;
  unsigned int pic_offset_y;
  unsigned int framerate_num;
  unsigned int framerate_den;
  float framerate;
  unsigned int M, N, IDRInterval;//control P, I, IDR percentage
  unsigned int sample_aspect_ratio_num;
  unsigned int sample_aspect_ratio_den;
  unsigned int bitrate;
  unsigned int lowdelay;
  EntropyType entropy_type;
} SVideoParams;

typedef struct {
  TGenericID id;
  unsigned int index;
  StreamFormat format;
  unsigned int pic_width;
  unsigned int pic_height;
  unsigned int bitrate;
  unsigned long long bitrate_pts;
  float framerate;

  unsigned char gop_M;
  unsigned char gop_N;
  unsigned char gop_idr_interval;
  unsigned char gop_structure;
  unsigned long long demand_idr_pts;
  unsigned char demand_idr_strategy;
  unsigned char framerate_reduce_factor;
  unsigned char framerate_integer;
  unsigned char reserved0;
} SVideoEncoderParam;

typedef struct {
  unsigned int width, height;
  unsigned int framerate_num, framerate_den;

  StreamFormat format;
} SVideoInjectParams;

typedef struct {
  unsigned int sample_rate;
  AudioSampleFMT sample_format;
  unsigned int channel_number;
  unsigned int channel_layout;
  unsigned int frame_size;
  unsigned int bitrate;
  unsigned int need_skip_adts_header;
  unsigned int pts_unit_num;//pts's unit
  unsigned int pts_unit_den;

  unsigned char is_channel_interlave;
  unsigned char is_big_endian;
  unsigned char reserved0, reserved1;

  unsigned int codec_format;
  unsigned int customized_codec_type;
} SAudioParams;

typedef union {
  SVideoParams video;
  SAudioParams audio;
} UFormatSpecific;

#endif

