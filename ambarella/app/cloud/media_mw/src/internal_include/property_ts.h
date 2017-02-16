/*
 * property_ts.h
 *
 * History:
 *    2014/09/15 - [Zhi He] create file
 *
 * Copyright (C) 2014 - 2024, the Ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the Ambarella Inc.
 */

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

