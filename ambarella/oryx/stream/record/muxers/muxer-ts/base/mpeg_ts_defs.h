/*
 * ts_muxer.h
 *
 * History:
 *    2008/10/07 - [Kaiming Xie] created file
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
 */
#ifndef MPEG_TS_DEFS_H_
#define MPEG_TS_DEFS_H_

/*********************
       Defines
*********************/

#define MPEG_TS_TP_PACKET_SIZE                  (188)

#define MPEG_TS_TP_PACKET_HEADER_SIZE           (4)

#define MPEG_TS_TP_SYNC_BYTE                    (0x47)

#define MPEG_TS_PAT_PID                         (0x0000)

#define MPEG_TS_NIT_PID                         (0x0010)

#define MPEG_TS_NULL_PID                        (0x1FFF)

#define PTS_DTS_CLOCK                           (90000)

#define PCR_CLOCK                               (27E6)



/*********************
       Enums
*********************/


/**
 *
 * Video frame rate codes as per "ISO/IEC 13818-1"
 */
typedef enum {

    MPEG_TS_FRAMERATE_FORBIDDEN = 0,    /* Forbidden video framerate index. */
    MPEG_TS_FRAMERATE_23_976 = 1,       /* 23.976Hz video framerate. */
    MPEG_TS_FRAMERATE_24 = 2,           /* 24Hz or 23.976Hz video framerate. */
    MPEG_TS_FRAMERATE_25 = 3,           /* 25Hz video framerate. */
    MPEG_TS_FRAMERATE_29_97 = 4,        /* 29.97Hz or 23.976Hz video framerate. */
    MPEG_TS_FRAMERATE_30 = 5,           /* 30Hz, 29.97Hz, 24Hz or 23.976Hz video framerate. */
    MPEG_TS_FRAMERATE_50 = 6,           /* 50Hz or 25Hz video framerate. */
    MPEG_TS_FRAMERATE_59_94 = 7,        /* 59.94Hz, 29.97Hz or 23.976Hz video framerate. */
    MPEG_TS_FRAMERATE_60 = 8,           /* 60Hz, 59.94Hz, 30Hz, 29.97Hz, 24Hz, or 23.976Hz video framerate. */
    MPEG_TS_FRAMERATE_RESERVED          /* Reserved video framerate index. */

} MPEG_TS_FRAMERATE_CODE;


/**
 *
 * Transport stream flags as per "ISO/IEC 13818-1"
 */
typedef enum {

    MPEG_TS_TRANSPORT_ERROR_INDICATOR_NO_ERRORS = 0,    /* No errors. */
    MPEG_TS_TRANSPORT_ERROR_INDICATOR_ERRORS = 1        /* Errors in the stream. */

} MPEG_TS_TRANSPORT_ERROR_INDICATOR;


/**
 *
 * Payload start indicator flags as per "ISO/IEC 13818-1"
 */
typedef enum {

    /**
     * If the stream carries PES, No PES starts in the current packet.
     * If the stream carries PSI, the first byte of the packet payload carries the
     * remaining of a previously started section.
     */
    MPEG_TS_PAYLOAD_UNIT_START_INDICATOR_NORMAL = 0,

    /**
     * If the stream carries PES, a PES starts in the current packet at the beginning of the payload.
     * If the stream carries PSI, the packet carries the first byte of a data section.
     */
    MPEG_TS_PAYLOAD_UNIT_START_INDICATOR_START = 1

} MPEG_TS_PAYLOAD_UNIT_START_INDICATOR;


/**
 *
 * Transport stream priority flags as per "ISO/IEC 13818-1"
 */
typedef enum {

    MPEG_TS_TRANSPORT_PRIORITY_NO_PRIORITY = 0, /* No priority data. */
    MPEG_TS_TRANSPORT_PRIORITY_PRIORITY = 1 /* Priority data. */

} MPEG_TS_TRANSPORT_PRIORITY;


/**
 *
 * Scrambling control flags as per "ISO/IEC 13818-1"
 */
typedef enum {

    MPEG_TS_SCRAMBLING_CTRL_NOT_SCRAMBLED = 0, /* Stream not scrambled. */
    MPEG_TS_SCRAMBLING_CTRL_1 = 0x01,          /* User defined. */
    MPEG_TS_SCRAMBLING_CTRL_2 = 0x02,          /* User defined. */
    MPEG_TS_SCRAMBLING_CTRL_3 = 0x03           /* User defined. */

} MPEG_TS_SCRAMBLING_CTRL;


/**
 *
 * Adaptation field flags as per "ISO/IEC 13818-1"
 */
typedef enum {

    MPEG_TS_ADAPTATION_FIELD_RESERVED = 0,           /* Reserved for future use by ISO/IEC. */
    MPEG_TS_ADAPTATION_FIELD_PAYLOAD_ONLY = 0x01,    /* No adaptation field, payload only. */
    MPEG_TS_ADAPTATION_FIELD_ADAPTATION_ONLY = 0x02, /* Adaptation field only, no payload. */
    MPEG_TS_ADAPTATION_FIELD_BOTH = 0x03             /* Adaptation field followed by payload. */

} MPEG_TS_ADAPTATION_FIELD;


/**
 *
 * Discontinuity indicator flags as per "ISO/IEC 13818-1"
 */
typedef enum {

    MPEG_TS_DISCONTINUITY_INDICATOR_NO_DISCONTINUTY = 0, /* No discontinuity in the stream. */
    MPEG_TS_DISCONTINUITY_INDICATOR_DISCONTINUTY = 1     /*Discontinuity in the field. */

} MPEG_TS_DISCONTINUITY_INDICATOR;


/**
 *
 * Random access indicator flags as per "ISO/IEC 13818-1"
 */
typedef enum {

    MPEG_TS_RANDOM_ACCESS_INDICATOR_NOT_PRESENT = 0, /* The next data in the stream does not carry information that aids random access. */
    MPEG_TS_RANDOM_ACCESS_INDICATOR_PRESENT = 1      /* The next data in the stream carries information that aids random access. */

} MPEG_TS_RANDOM_ACCESS_INDICATOR;


/**
 *
 * Elementary stream priority indicator flags as per "ISO/IEC 13818-1"
 */
typedef enum {

    /** The data carried in this transport
     *  packet doesn't have a higher priority compared with other transport packet with the same PID.
     */
    MPEG_TS_ELEMENTARY_STREAM_PRIORITY_NO_PRIORITY = 0,
    /** The data carried by this transport packet
     *  has higher priority compared with other transport packets with the same PID.
     */
    MPEG_TS_ELEMENTARY_STREAM_PRIORITY_PRIORITY = 1

} MPEG_TS_ELEMENTARY_STREAM_PRIORITY_INDICATOR;


/**
 *
 * PCR flags as per "ISO/IEC 13818-1"
 */
typedef enum {

    MPEG_TS_PCR_FLAG_NOT_PRESENT = 0, /* The adaptation field does not contain a PCR. */
    MPEG_TS_PCR_FLAG_PRESENT = 1      /* The adaptation field contains a PCR. */

} MPEG_TS_PCR_FLAG;


/**
 *
 * OPCR flags as per "ISO/IEC 13818-1"
 */
typedef enum {

    MPEG_TS_OPCR_FLAG_NOT_PRESENT = 0, /* The adaptation field does not contain a OPCR. */
    MPEG_TS_OPCR_FLAG_PRESENT = 1      /* The adaptation field contains a OPCR. */

} MPEG_TS_OPCR_FLAG;


/**
 *
 * Splicing point flags as per "ISO/IEC 13818-1"
 */
typedef enum {

    MPEG_TS_SPLICING_POINT_FLAG_NOT_PRESENT = 0, /* The adaptation field does not contain a splicing countdown field. */
    MPEG_TS_SPLICING_POINT_FLAG_PRESENT = 1      /* The adaptation field does contain a splicing countdown field. */

} MPEG_TS_SPLICING_POINT_FLAG;


/**
 *
 * Transport private data flags as per "ISO/IEC 13818-1"
 */
typedef enum {

    MPEG_TS_TRANSPORT_PRIVATE_DATA_NOT_PRESENT = 0, /* The adaptation field does not contain one or more private bytes. */
    MPEG_TS_TRANSPORT_PRIVATE_DATA_PRESENT = 1      /* The adaptation field contains private bytes. */

} MPEG_TS_TRANSPORT_PRIVATE_DATA_FLAG;


/**
 *
 * Transport private data flags as per "ISO/IEC 13818-1"
 */
typedef enum {

    MPEG_TS_ADAPTATION_FIELD_EXTENSION_NOT_PRESENT = 0, /* There is no adaptation field extension. */
    MPEG_TS_ADAPTATION_FIELD_EXTENSION_PRESENT = 1      /* It indicates the presence of an adaptation field extension. */

} MPEG_TS_ADAPTATION_FIELD_EXTENSION_FLAG;


/**
 *
 * MPEG stream IDs as per "ISO/IEC 13818-1"
 * note that stream_id is DIFFERENT from stream_type:
 * stream_id is present in PES header and belongs to the PES packet layer
 * stream_type is defined in PMT Program element loop and belongs to Transport Stream layer
 */
typedef enum {

    MPEG_TS_STREAM_ID_PROGRAM_STREAM_MAP = 0xBC, /* Program stream map. */
    MPEG_TS_STREAM_ID_PRIVATE_STREAM_1 = 0xBD, /* Private stream 1. */
    MPEG_TS_STREAM_ID_PADDING_STREAM = 0xBE, /* Padding stream. */
    MPEG_TS_STREAM_ID_PRIVATE_STREAM_2 = 0xBF, /* Private stream 2. */
    MPEG_TS_STREAM_ID_AUDIO_00 = 0xC0, /* ISO/IEC 13818-3 or ISO11172-3 audio stream number 0. */
    MPEG_TS_STREAM_ID_AUDIO_01 = 0xC1, /* ISO/IEC 13818-3 or ISO11172-3 audio stream number 1. */
    MPEG_TS_STREAM_ID_AUDIO_02 = 0xC2, /* ISO/IEC 13818-3 or ISO11172-3 audio stream number 2. */
    MPEG_TS_STREAM_ID_AUDIO_03 = 0xC3, /* ISO/IEC 13818-3 or ISO11172-3 audio stream number 3. */
    MPEG_TS_STREAM_ID_AUDIO_04 = 0xC4, /* ISO/IEC 13818-3 or ISO11172-3 audio stream number 4. */
    MPEG_TS_STREAM_ID_AUDIO_05 = 0xC5, /* ISO/IEC 13818-3 or ISO11172-3 audio stream number 5. */
    MPEG_TS_STREAM_ID_AUDIO_06 = 0xC6, /* ISO/IEC 13818-3 or ISO11172-3 audio stream number 6. */
    MPEG_TS_STREAM_ID_AUDIO_07 = 0xC7, /* ISO/IEC 13818-3 or ISO11172-3 audio stream number 7. */
    MPEG_TS_STREAM_ID_AUDIO_08 = 0xC8, /* ISO/IEC 13818-3 or ISO11172-3 audio stream number 8. */
    MPEG_TS_STREAM_ID_AUDIO_09 = 0xC9, /* ISO/IEC 13818-3 or ISO11172-3 audio stream number 9. */
    MPEG_TS_STREAM_ID_AUDIO_0A = 0xCA, /* ISO/IEC 13818-3 or ISO11172-3 audio stream number 10. */
    MPEG_TS_STREAM_ID_AUDIO_0B = 0xCB, /* ISO/IEC 13818-3 or ISO11172-3 audio stream number 11. */
    MPEG_TS_STREAM_ID_AUDIO_0C = 0xCC, /* ISO/IEC 13818-3 or ISO11172-3 audio stream number 12. */
    MPEG_TS_STREAM_ID_AUDIO_0D = 0xCD, /* ISO/IEC 13818-3 or ISO11172-3 audio stream number 13. */
    MPEG_TS_STREAM_ID_AUDIO_0E = 0xCE, /* ISO/IEC 13818-3 or ISO11172-3 audio stream number 14. */
    MPEG_TS_STREAM_ID_AUDIO_0F = 0xCF, /* ISO/IEC 13818-3 or ISO11172-3 audio stream number 15. */
    MPEG_TS_STREAM_ID_AUDIO_10 = 0xD0, /* ISO/IEC 13818-3 or ISO11172-3 audio stream number 16. */
    MPEG_TS_STREAM_ID_AUDIO_11 = 0xD1, /* ISO/IEC 13818-3 or ISO11172-3 audio stream number 17. */
    MPEG_TS_STREAM_ID_AUDIO_12 = 0xD2, /* ISO/IEC 13818-3 or ISO11172-3 audio stream number 18. */
    MPEG_TS_STREAM_ID_AUDIO_13 = 0xD3, /* ISO/IEC 13818-3 or ISO11172-3 audio stream number 19. */
    MPEG_TS_STREAM_ID_AUDIO_14 = 0xD4, /* ISO/IEC 13818-3 or ISO11172-3 audio stream number 20. */
    MPEG_TS_STREAM_ID_AUDIO_15 = 0xD5, /* ISO/IEC 13818-3 or ISO11172-3 audio stream number 21. */
    MPEG_TS_STREAM_ID_AUDIO_16 = 0xD6, /* ISO/IEC 13818-3 or ISO11172-3 audio stream number 22. */
    MPEG_TS_STREAM_ID_AUDIO_17 = 0xD7, /* ISO/IEC 13818-3 or ISO11172-3 audio stream number 23. */
    MPEG_TS_STREAM_ID_AUDIO_18 = 0xD8, /* ISO/IEC 13818-3 or ISO11172-3 audio stream number 24. */
    MPEG_TS_STREAM_ID_AUDIO_19 = 0xD9, /* ISO/IEC 13818-3 or ISO11172-3 audio stream number 25. */
    MPEG_TS_STREAM_ID_VIDEO_00 = 0xE0, /* ITU-T Rec. H.262 | ISO/IEC 13818-2 or ISO/IEC 11172-2 video stream 25 number 0. */
    MPEG_TS_STREAM_ID_VIDEO_01 = 0xE1, /* ITU-T Rec. H.262 | ISO/IEC 13818-2 or ISO/IEC 11172-2 video stream 25 number 1. */
    MPEG_TS_STREAM_ID_VIDEO_02 = 0xE2, /* ITU-T Rec. H.262 | ISO/IEC 13818-2 or ISO/IEC 11172-2 video stream 25 number 2. */
    MPEG_TS_STREAM_ID_VIDEO_03 = 0xE3, /* ITU-T Rec. H.262 | ISO/IEC 13818-2 or ISO/IEC 11172-2 video stream 25 number 3. */
    MPEG_TS_STREAM_ID_VIDEO_04 = 0xE4, /* ITU-T Rec. H.262 | ISO/IEC 13818-2 or ISO/IEC 11172-2 video stream 25 number 4. */
    MPEG_TS_STREAM_ID_VIDEO_05 = 0xE5, /* ITU-T Rec. H.262 | ISO/IEC 13818-2 or ISO/IEC 11172-2 video stream 25 number 5. */
    MPEG_TS_STREAM_ID_VIDEO_06 = 0xE6, /* ITU-T Rec. H.262 | ISO/IEC 13818-2 or ISO/IEC 11172-2 video stream 25 number 6. */
    MPEG_TS_STREAM_ID_VIDEO_07 = 0xE7, /* ITU-T Rec. H.262 | ISO/IEC 13818-2 or ISO/IEC 11172-2 video stream 25 number 7. */
    MPEG_TS_STREAM_ID_VIDEO_08 = 0xE8, /* ITU-T Rec. H.262 | ISO/IEC 13818-2 or ISO/IEC 11172-2 video stream 25 number 8. */
    MPEG_TS_STREAM_ID_VIDEO_09 = 0xE9, /* ITU-T Rec. H.262 | ISO/IEC 13818-2 or ISO/IEC 11172-2 video stream 25 number 9. */
    MPEG_TS_STREAM_ID_VIDEO_0A = 0xEA, /* ITU-T Rec. H.262 | ISO/IEC 13818-2 or ISO/IEC 11172-2 video stream 25 number 10. */
    MPEG_TS_STREAM_ID_VIDEO_0B = 0xEB, /* ITU-T Rec. H.262 | ISO/IEC 13818-2 or ISO/IEC 11172-2 video stream 25 number 11. */
    MPEG_TS_STREAM_ID_VIDEO_0C = 0xEC, /* ITU-T Rec. H.262 | ISO/IEC 13818-2 or ISO/IEC 11172-2 video stream 25 number 12. */
    MPEG_TS_STREAM_ID_VIDEO_0D = 0xED, /* ITU-T Rec. H.262 | ISO/IEC 13818-2 or ISO/IEC 11172-2 video stream 25 number 13. */
    MPEG_TS_STREAM_ID_VIDEO_0E = 0xEE, /* ITU-T Rec. H.262 | ISO/IEC 13818-2 or ISO/IEC 11172-2 video stream 25 number 14. */
    MPEG_TS_STREAM_ID_VIDEO_0F = 0xEF, /* ITU-T Rec. H.262 | ISO/IEC 13818-2 or ISO/IEC 11172-2 video stream 25 number 15. */
    MPEG_TS_STREAM_ID_ECM_STREAM = 0xF0, /* ECM stream. */
    MPEG_TS_STREAM_ID_EMM_STREAM = 0xF1, /* EMM stream. */
    MPEG_TS_STREAM_ID_H2220_DSMCC = 0xF2, /* ITU-T Rec. H.222.0 | ISO/IEC 13818-1 Annex A or ISO/IEC 11818-6 DSMCC stream. */
    MPEG_TS_STREAM_ID_ISOIEC_13522 = 0xF3, /* ISO/IEC 13522 stream. */
    MPEG_TS_STREAM_ID_H2221_TYPE_A = 0xF4, /* ITU-T Rec. H.222 type A. */
    MPEG_TS_STREAM_ID_H2221_TYPE_B = 0xF5, /* ITU-T Rec. H.222 type B. */
    MPEG_TS_STREAM_ID_H2221_TYPE_C = 0xF6, /* ITU-T Rec. H.222 type C. */
    MPEG_TS_STREAM_ID_H2221_TYPE_D = 0xF7, /* ITU-T Rec. H.222 type D. */
    MPEG_TS_STREAM_ID_H2221_TYPE_E = 0xF8, /* ITU-T Rec. H.222 type E. */
    MPEG_TS_STREAM_ID_ANCILLARY = 0xF9, /* Ancillary stream. */
    MPEG_TS_STREAM_ID_RESERVED_00 = 0xFA, /* Reserved data stream. */
    MPEG_TS_STREAM_ID_RESERVED_01 = 0xFB, /* Reserved data stream. */
    MPEG_TS_STREAM_ID_RESERVED_02 = 0xFC, /* Reserved data stream. */
    MPEG_TS_STREAM_ID_RESERVED_03 = 0xFD, /* Reserved data stream. */
    MPEG_TS_STREAM_ID_RESERVED_04 = 0xFE, /* Reserved data stream. */
    MPEG_TS_STREAM_ID_PROGRAM_DIRECTORY = 0xFF, /* Program stream directory. */

} MPEG_TS_STREAM_ID;


/**
 *
 * Scrambling control flags as per "ISO/IEC 13818-1"
 */
typedef enum {

    MPEG_TS_PES_SCRAMBLING_CTRL_NOT_SCRAMBLED = 0, /* Stream not scrambled. */
    MPEG_TS_PES_SCRAMBLING_CTRL_1 = 1,             /* User defined. */
    MPEG_TS_PES_SCRAMBLING_CTRL_2 = 2,             /* User defined. */
    MPEG_TS_PES_SCRAMBLING_CTRL_3 = 3              /* User defined. */

} MPEG_TS_PES_SCRAMBLING_CTRL;


/**
 *
 * PES priority flags as per "ISO/IEC 13818-1"
 */
typedef enum {

    MPEG_TS_PES_PRIORITY_PRIORITY = 0,   /* Priority data. */
    MPEG_TS_PES_PRIORITY_NO_PRIORITY = 1 /* No priority data. */

} MPEG_TS_PES_TRANSPORT_PRIORITY;


/**
 *
 * PES alignment flags as per "ISO/IEC 13818-1"
 */
typedef enum {

    MPEG_TS_PES_ALIGNMENT_CONTROL_UNKNOWN = 0,  /* No information about presence of startcodes. */
    MPEG_TS_PES_ALIGNMENT_CONTROL_STARTCODE = 1 /* The PES header packet is immediatelly followed by a video start code or audio syncword. */

} MPEG_TS_PES_ALIGNMENT_CONTROL;


/**
 *
 * PES alignment flags as per "ISO/IEC 13818-1"
 */
typedef enum {

    MPEG_TS_PES_COPYRIGHT_UNDEFINED = 0, /* The copyrights might be defined in a separate descriptor. */
    MPEG_TS_PES_COPYRIGHT_PROTECTED = 1  /* Material protected by copyrights. */

} MPEG_TS_PES_COPYRIGHT;


/**
 *
 * PES alignment flags as per "ISO/IEC 13818-1"
 */
typedef enum {

    MPEG_TS_PES_ORIGINAL_OR_COPY_COPY = 0,    /* Copied material. */
    MPEG_TS_PES_ORIGINAL_OR_COPY_ORIGINAL = 1 /* Original material. */

} MPEG_TS_PES_ORIGINAL_OR_COPY;


/**
 *
 * PTS and DTS flags as per "ISO/IEC 13818-1"
 */
typedef enum {

    MPEG_TS_PTS_DTS_NO_PTSDTS = 0,  /* No PTS or DTS in the stream. */
    MPEG_TS_PTS_DTS_FORBIDDEN = 1,  /* Forbidden value. */
    MPEG_TS_PTS_DTS_PTS_ONLY = 2,   /* PTS only in the stream. */
    MPEG_TS_PTS_DTS_PTSDTS_BOTH = 3 /* Both PTS and DTS in the stream. */

} MPEG_TS_PTS_DTS_FLAG;


/**
 *
 * PES ESCR flags as per "ISO/IEC 13818-1"
 */
typedef enum {

    MPEG_TS_PES_ESCR_NOT_PRESENT = 0, /* ESCR base and extension fields are not present. */
    MPEG_TS_PES_ESCR_PRESENT = 1      /* ESCR base and extension fields are present. */

} MPEG_TS_PES_ESCR_FLAG;


/**
 *
 * Elementary stream flags as per "ISO/IEC 13818-1"
 */
typedef enum {

    MPEG_TS_PES_ES_NOT_PRESENT = 0, /* Elementary stream rate field is not present. */
    MPEG_TS_PES_ES_PRESENT = 1      /* Elementary stream rate field is present. */

} MPEG_TS_PES_ES_FLAG;


/**
 *
 * DSM trick mode flags as per "ISO/IEC 13818-1"
 */
typedef enum {

    MPEG_TS_PES_DSM_TRICK_MODE_NOT_PRESENT = 0, /* 8-bit trick mode field is not present. */
    MPEG_TS_PES_DSM_TRICK_MODE_PRESENT = 1      /* 8-bit trick mode field is present. */

} MPEG_TS_PES_DSM_TRICK_MODE_FLAG;


/**
 *
 * Add copy field flags as per "ISO/IEC 13818-1"
 */
typedef enum {

    MPEG_TS_PES_ADD_COPY_INFO_NOT_PRESENT = 0, /* Additional copy field is not present. */
    MPEG_TS_PES_ADD_COPY_INFO_PRESENT = 1      /* Additional copy mode field is present. */

} MPEG_TS_PES_ADD_COPY_INFO_FLAG;


/**
 *
 * CRC flags as per "ISO/IEC 13818-1"
 */
typedef enum {

    MPEG_TS_PES_CRC_NOT_PRESENT = 0, /* CRC field is not present. */
    MPEG_TS_PES_CRC_PRESENT = 1      /* CRC field is present. */

} MPEG_TS_PES_CRC_FLAG;


/**
 *
 * Extension flags as per "ISO/IEC 13818-1"
 */
typedef enum {

    MPEG_TS_PES_EXT_NOT_PRESENT = 0, /* Extension field is not present. */
    MPEG_TS_PES_EXT_PRESENT = 1      /* Extension field is present. */

} MPEG_TS_PES_EXT_FLAG;



/**
 *
 * MPEG stream TYPEs as per "ISO/IEC 13818-1"
 * note that stream_type is DIFFERENT from stream_id:
 * stream_type is defined in PMT Program element loop and belongs to Transport Stream layer
 * stream_id is present in PES header and belongs to the PES packet layer
 */
typedef enum {

    MPEG_SI_STREAM_TYPE_RESERVED_0        = 0x00,       /* MPEG2 Systems Reserved. */
    MPEG_SI_STREAM_TYPE_MPEG1_VIDEO       = 0x01,       /* ISO/IEC 11172 Video */
    MPEG_SI_STREAM_TYPE_MPEG2_VIDEO       = 0x02,       /* ITU-T Rec. H.262 | ISO/IEC 13818-2 Video or ISO/IEC 11172 constrained parameter video stream*/
    MPEG_SI_STREAM_TYPE_MPEG1_AUDIO       = 0x03,       /* ISO/IEC 11172 Audio */
    MPEG_SI_STREAM_TYPE_MPEG2_AUDIO       = 0x04,       /* ISO/IEC 13818-3 Audio */
    MPEG_SI_STREAM_TYPE_PRIVATE_SECTION   = 0x05,       /* ITU-T Rec. H.222.0 | ISO/IEC 13818-1 private_sections */
    MPEG_SI_STREAM_TYPE_PRIVATE_PES       = 0x06,       /* ITU-T Rec. H.222.0 | ISO/IEC 13818-1 pes packets containing private data */
    MPEG_SI_STREAM_TYPE_MHEG              = 0x07,       /* ISO/IEC 13522 MHEG */
    MPEG_SI_STREAM_TYPE_DSM_CC            = 0x08,       /* ITU-T Rec. H.222.0 | ISO/IEC 13818-1 Annex A DSM-CC */
    MPEG_SI_STREAM_TYPE_RESERVED_9        = 0x09,       /* ITU-T Rec. H.222.1 */
    MPEG_SI_STREAM_TYPE_RESERVED_10       = 0x0A,       /* ISO/IEC 13818-6 type A */
    MPEG_SI_STREAM_TYPE_RESERVED_11       = 0x0B,       /* ISO/IEC 13818-6 type B */
    MPEG_SI_STREAM_TYPE_RESERVED_12       = 0x0C,       /* ISO/IEC 13818-6 type C */
    MPEG_SI_STREAM_TYPE_RESERVED_13       = 0x0D,       /* ISO/IEC 13818-6 type D */
    MPEG_SI_STREAM_TYPE_RESERVED_14       = 0x0E,       /* ISO/IEC 13818-6 type auxiliary */
    MPEG_SI_STREAM_TYPE_AAC               = 0x0F,       /* ISO/IEC 13818-7 Audio (AAC) */
    MPEG_SI_STREAM_TYPE_MPEG4_AUDIO       = 0x10,       /* MPEG4 Audio elementary stream (MPEG2 PES based) */
    MPEG_SI_STREAM_TYPE_MPEG4_VIDEO       = 0x11,       /* MPEG4 visual elementary stream (MPEG2 PES based) */
    MPEG_SI_STREAM_TYPE_MPEG4_SYSTEM1     = 0x12,       /* MPEG4 SL-packetized stream of FlexMux Stream in MPEG2 PES */
    MPEG_SI_STREAM_TYPE_MPEG4_SYSTEM2     = 0x13,       /* MPEG4 SL-packetized stream of FlexMux Stream ISO/IEC 14496 (MPEG4) */
    MPEG_SI_STREAM_TYPE_RESERVED_20       = 0x14,       /* ISO/IEC 13818-6 DSM-CC Synchronized Download protocol */
    MPEG_SI_STREAM_TYPE_RESERVED_21       = 0x15,       /* MPEG2 Systems Reserved. */
    MPEG_SI_STREAM_TYPE_RESERVED_22       = 0x16,       /* MPEG2 Systems Reserved. */
    MPEG_SI_STREAM_TYPE_RESERVED_23       = 0x17,       /* MPEG2 Systems Reserved. */
    MPEG_SI_STREAM_TYPE_RESERVED_24       = 0x18,       /* MPEG2 Systems Reserved. */
    MPEG_SI_STREAM_TYPE_RESERVED_25       = 0x19,       /* MPEG2 Systems Reserved. */
    MPEG_SI_STREAM_TYPE_RESERVED_26       = 0x1A,       /* MPEG2 Systems Reserved. */
    MPEG_SI_STREAM_TYPE_AVC_VIDEO         = 0x1B,       /* ITU-T Rec. H.264 ISO/IEC 14496-10 Video */
    MPEG_SI_STREAM_TYPE_HEVC_VIDEO        = 0x24,
    MPEG_SI_STREAM_TYPE_RESERVED_28       = 0x1C,       /* MPEG2 Systems Reserved. */
    MPEG_SI_STREAM_TYPE_RESERVED_127      = 0x7F,       /* MPEG2 Systems Reserved. */

    MPEG_SI_STREAM_TYPE_LPCM_AUDIO = 0x80,
    //MPEG_SI_STREAM_TYPE_DIGICIPHER2_VIDEO = 0x80,       /* General Instrument's DCII compression standard. */
    MPEG_SI_STREAM_TYPE_AC3_AUDIO_OR_TTX  = 0x81

} MPEG_SI_STREAM_TYPE;


/*********************
       Structures
*********************/


/**
 * Transport packet header.
 */

typedef struct {
#ifdef BIGENDIAN
    /*Btype 3*/
    uint8_t transport_scrambling_control :2; /* TS Scrambling Control.        */
    uint8_t adaptation_field_control     :2;
    uint8_t continuity_counter           :4; /* Countinuity counter.          */
    /*Btype 2*/
    uint8_t pid_7to0                     :8; /* Program ID, bits 7:0.         */
    /*Btype 1*/
    uint8_t transport_error_indicator    :1; /* TS Error Indicator.           */
    uint8_t payload_unit_start_indicator :1;
    uint8_t transport_priority           :1; /* TS Transport Priority.        */
    uint8_t pid_12to8                    :5; /* Program ID, bits 12:8.        */
    /*Btype 0*/
    uint8_t sync_byte                    :8; /* Synchronization byte.         */
#else
    /*Btype 0*/
    uint8_t sync_byte                    :8; /* Synchronization byte.         */
    /*Btype 1*/
    uint8_t pid_12to8                    :5; /* Program ID, bits 12:8.        */
    uint8_t transport_priority           :1; /* TS Transport Priority.        */
    uint8_t payload_unit_start_indicator :1; /* Payload Unit Start Indicator. */
    uint8_t transport_error_indicator    :1; /* TS Error Indicator.           */
    /*Btype 2*/
    uint8_t pid_7to0                     :8; /* Program ID, bits 7:0.         */
    /*Btype 3*/
    uint8_t continuity_counter           :4; /* Countinuity counter.          */
    uint8_t adaptation_field_control     :2;
    uint8_t transport_scrambling_control :2; /* TS Scrambling Control.        */
#endif
} MPEG_TS_TP_HEADER;

/**
 * Adaptation field header.
 */
typedef struct {
#ifdef BIGENDIAN
    /*Byte 1*/
    uint8_t discontinuity_indicator               :1; /* Discontinuity indicator. */
    uint8_t random_access_indicator               :1; /* Random access indicator. */
    uint8_t elementary_stream_priority_indicator  :1; /* Elementary stream priority indicator. */
    uint8_t pcr_flag                              :1; /* PCR flag. */
    uint8_t opcr_flag                             :1; /* OPCR flag. */
    uint8_t splicing_point_flag                   :1; /* Splicing point flag. */
    uint8_t transport_private_data_flag           :1; /* Transport private data flag. */
    uint8_t adaptation_field_extension_flag       :1; /* Adaptation field extension flag. */
    /*Bype 0*/
    uint8_t adaptation_field_length               :8; /* Adaptation field lenght, this field not included. */
#else
    /*Byte 0*/
    uint8_t adaptation_field_length               :8; /* Adaptation field length, this field not included. */
    /*Byte 1*/
    uint8_t adaptation_field_extension_flag       :1; /* Adaptation field extension flag. */
    uint8_t transport_private_data_flag           :1; /* Transport private data flag. */
    uint8_t splicing_point_flag                   :1; /* Splicing point flag. */
    uint8_t opcr_flag                             :1; /* OPCR flag. */
    uint8_t pcr_flag                              :1; /* PCR flag. */
    uint8_t elementary_stream_priority_indicator  :1; /* Elementary stream priority indicator*/
    uint8_t random_access_indicator               :1; /* Random access indicator. */
    uint8_t discontinuity_indicator               :1; /* Discontinuity indicator. */
#endif
} MPEG_TS_TP_ADAPTATION_FIELD_HEADER;

/**
 * PES header.
 */
typedef struct {
#ifdef BIGENDIAN
    /*Byte 9*/
    uint8_t header_data_length       :8; /* Length of optional fields and stuffing bytes. */
    /*Byte 8*/
    uint8_t pts_dts_flags            :2; /* PTS/DTS flag. */
    uint8_t escr_flag                :1; /* ESCR flag. */
    uint8_t es_rate_flag             :1; /* Elementary stream flag. */
    uint8_t dsm_trick_mode_flag      :1; /* DSM trick mode flag. */
    uint8_t add_copy_info_flag       :1; /* Additional copy info flag*/
    uint8_t pes_crc_flag             :1; /* PES CRC flag. */
    uint8_t pes_ext_flag             :1; /* PES extension flag. */
    /*Byte 7*/
    uint8_t marker_bits              :2; /* Marker bits, the should be set to 0x02. */
    uint8_t pes_scrambling_control   :2; /* PES scrambling control. */
    uint8_t pes_priority             :1; /* PES priority. */
    uint8_t data_alignment_indicator :1; /* Data alignment control. */
    uint8_t copyright                :1; /* Copyright flag. */
    uint8_t original_or_copy         :1; /* Original or copy flag. */
    /*Byte 6*/
    uint8_t pes_packet_length_7to0   :8; /* Packet length. Bits 15:8. */
    /*Byte 5*/
    uint8_t pes_packet_length_15to8  :8; /* Packet length. Bits 7:0.  */
    /*Byte 4*/
    uint8_t stream_id                :8; /* Stream ID. */
    /*Byte 3*/
    uint8_t packet_start_code_7to0   :8; /* Packet start code. Bits 7:0. */
    /*Byte 2*/
    uint8_t packet_start_code_15to8  :8; /* Packet start code. Bits 15:8.  */
    /*Byte 1*/
    uint8_t packet_start_code_23to16 :8; /* Packet start code. Bits 23:16.   */
#else
    /*Byte 1*/
    uint8_t packet_start_code_23to16 :8; /* Packet start code. Bits 23:16.   */
    /*Byte 2*/
    uint8_t packet_start_code_15to8  :8; /* Packet start code. Bits 15:8.  */
    /*Byte 3*/
    uint8_t packet_start_code_7to0   :8; /* Packet start code. Bits 7:0. */
    /*Byte 4*/
    uint8_t stream_id                :8; /* Stream ID. */
    /*Byte 5*/
    uint8_t pes_packet_length_15to8  :8; /* Packet length. Bits 15:8. */
    /*Byte 6*/
    uint8_t pes_packet_length_7to0   :8; /* Packet length. Bits 7:0.  */
    /*Byte 7*/
    uint8_t original_or_copy         :1; /* Original or copy flag. */
    uint8_t copyright                :1; /* Copyright flag. */
    uint8_t data_alignment_indicator :1; /* Data alignment control. */
    uint8_t pes_priority             :1; /* PES priority. */
    uint8_t pes_scrambling_control   :2; /* PES scrambling control. */
    uint8_t marker_bits              :2; /* Marker bits, the should be set to 0x02. */
    /*Byte 8*/
    uint8_t pes_ext_flag             :1; /* PES extension flag. */
    uint8_t pes_crc_flag             :1; /* PES CRC flag. */
    uint8_t add_copy_info_flag       :1; /* Additional copy info flag*/
    uint8_t dsm_trick_mode_flag      :1; /* DSM trick mode flag. */
    uint8_t es_rate_flag             :1; /* Elementary stream flag. */
    uint8_t escr_flag                :1; /* ESCR flag. */
    uint8_t pts_dts_flags            :2; /* PTS/DTS flag. */
    /*Byte 9*/
    uint8_t header_data_length       :8; /* Length of optional fields and stuffing bytes. */
#endif
} MPEG_TS_TP_PES_HEADER;


/*********************
       Macros
*********************/


/**
 * Macro to set the transport packet PID into a transport packet
 * header.  Note that the casting has been intentionally omitted
 * to make sure that the developer is passing the correct parameters.
 */
#define MPEG_TS_TP_HEADER_PID_SET(tpHdr, pid) \
    { \
      (tpHdr)->pid_12to8 = ((pid) >> 8) & 0x1F; \
      (tpHdr)->pid_7to0 = (pid) & 0xFF; \
    }



#endif // MPEG_TS_DEFS_H_

