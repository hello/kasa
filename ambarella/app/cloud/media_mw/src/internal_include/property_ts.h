/*******************************************************************************
 * property_ts.h
 *
 * History:
 *    2014/09/15 - [Zhi He] create file
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

#ifndef __PROPERTY_TS_H__
#define __PROPERTY_TS_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN
DCODE_DELIMITER;

#define DTS_PMT_PID (0x100)
#define DTS_VIDEO_PID_START (0x120)
#define DTS_AUDIO_PID_START (0x140)
#define DTS_PRIDATA_PID_START (0x160)
#define DTS_SUBTITLE_PID_START (0x180)

#define DTS_DESCRIPTOR_TAG_VIDEO_START (0x54)
#define DTS_DESCRIPTOR_TAG_AUDIO_START (0x74)
#define DTS_DESCRIPTOR_TAG_PRIVATE_START (0x94)

/**********
  Typedefs
 **********/

// Stream Descriptor
typedef struct {
    TU32 pat  : 5; /*!< version number for pat table. */
    TU32 pmt  : 5; /*!< version number for pmt table. */
} STSMuxPsiVersion;


/**
 *
 * Stream configuration
 */
typedef struct {
    TU8 firstFrame;
    TU8 firstSlice;
    TU8 withPCR;
    TU8 reserved0;

    TU8 *pPlayload;
    TU32 payloadSize;
    TTime pts;
    TTime dts;
    TTime pcr_base;
    TU16 pcr_ext;
    TU16 reserved1;
} STSMuxPesPlayloadInfo;

/**
 *
 * Stream configuration
 */
typedef struct {
    TU16 pid; /*!< Packet ID to be used in the TS. */
    TU8  type;/*!< Program type, (MPEG_SI_STREAM_TYPE). */
    TU8  descriptor_tag;
    TUint descriptor_len;
    TU8  *pDescriptor;
} STSMuxPsiStreamInfo;

/**
 *
 * Program configuration.
 */
typedef struct {
    TU16 pidPMT; /*!< Packet ID to be used to store the Program Map Table [PMT]. */
    TU16 pidPCR; /*!< Packet ID to be used to store the Program Clock Referene [PCR]. */
    TU16 prgNum; /*!< Program Number to be used in Program Map Table [PMT]. */
    TU16 totStream; /*!< Total number of stream within the program. */
} STSMuxPsiPrgInfo;

/**
 *
 * Program Map Table [PMT] configuration.
 */
typedef struct {
    STSMuxPsiPrgInfo *prgInfo;

    STSMuxPsiStreamInfo stream[EConstMaxDemuxerMuxerStreamNumber]; /*!< List of stream configurations. */
    TU8  descriptor_tag;
    TU8 reserved0;
    TU8 reserved1;
    TU8 reserved2;
    TUint    descriptor_len;
    TU8 pDescriptor[8];
} STSMuxPsiPmtInfo;

/**
 *
 * Program Association table [PAT] configuration
 */
typedef struct {
    TU16 totPrg; /*!< Total number of valid programs. */
    TU16     reserved0;
    STSMuxPsiPrgInfo *prgInfo; /*!< List of program configurations. */
} STSMuxPsiPatInfo;

typedef struct {
    TU32 format;
    TU32 frame_tick;
    TU32 time_scale;
    TU32 width;
    TU32 height;
    TU32 bitrate;
    TU32 framerate_num;
    TU32 framerate_den;
    TU32 idr_interval;
} STSVideoInfo;

typedef struct {
    TU32 format;
    TU32 sample_rate;
    TU32 channels;
    TU32 sample_format;
    TU32 chunk_size;
    TU32 bitrate;
} STSAudioInfo;

typedef struct {
    TU8 stream_type;
    TU8 enable;
    TU16 elementary_PID;
} SStreamInfo;

typedef struct {
    TU8 streamIndex;
    TU8 frameType;
    TU8 reserved_1;
    TU8 reserved_2;
    TU8 *pBuffer;
    TUint bufferSize;
    TUint dataSize;
    TUint pesLength;
    TTime pts;
    TTime dts;
} SPESInfo;

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif

