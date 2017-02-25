/*
 *am_muxer_ts_builder.h
 *
 * History:
 *    2011/4/17 - [Kaiming Xie] created file
 *    2014/12/16 - [Chengcai Jing] modified file
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
#ifndef __AM_MUXER_TS_BUILDER_H__
#define __AM_MUXER_TS_BUILDER_H__

#include "am_muxer_codec_info.h"
#include "mpeg_ts_defs.h"
#include "am_audio_define.h"
#include "am_video_types.h"

/******
  Defines
 ******/

#define PAT_HDR_SIZE            (8)
#define PAT_ELEMENT_SIZE        (4)
#define PMT_HDR_SIZE            (12)
#define PMT_ELEMENT_SIZE        (5)
#define PMT_STREAM_DSC_SIZE     (3)

#define PES_HEADER_LEN          (9)
#define ADAPT_HEADER_LEN         (2)

#define CTSMUXPSI_STREAM_TOT     (2) //audio & video

#define PCR_FIELD_SIZE		(6)
#define PTS_FIELD_SIZE		(5)
#define DTS_FIELD_SIZE		(5)


/**********
  Typedefs
 **********/

// Stream Descriptor
typedef struct {
   uint32_t pat  :5; /*!< version number for pat table. */
   uint32_t pmt  :5; /*!< version number for pmt table. */
} AM_TS_MUXER_PSI_VER;


/**
 *
 * Stream configuration
 */
typedef struct {
    int64_t  pts;
    int64_t  dts;
    int64_t  pcr_base;
    uint8_t* p_payload;
    int32_t  payload_size;
    uint16_t pcr_ext;
    uint8_t  first_frame;
    uint8_t  first_slice;
    uint8_t  with_pcr;
} AM_TS_MUXER_PES_PAYLOAD_INFO;

/**
 *
 * Stream configuration
 */
typedef struct {
   uint8_t            *descriptor;
   MPEG_SI_STREAM_TYPE type; /*!< Program type. */
   uint32_t            descriptor_len;
   uint16_t            pid; /*!< Packet ID to be used in the TS. */
   uint8_t             descriptor_tag;
} AM_TS_MUXER_PSI_STREAM_INFO;

/**
 *
 * Program configuration.
 */
typedef struct {
   uint16_t pid_pmt; /*!< Packet ID to be used to store the Program Map Table [PMT],13 bit. */
   uint16_t pid_pcr; /*!< Packet ID to be used to store the Program Clock Referene [PCR]. */
   uint16_t prg_num; /*!< Program Number to be used in Program Map Table [PMT]. */
} AM_TS_MUXER_PSI_PRG_INFO;

/**
 *
 * Program Map Table [PMT] configuration.
 */
typedef struct {
    AM_TS_MUXER_PSI_PRG_INFO     *prg_info;
    uint8_t                      *descriptor;
    /*!< List of stream configurations. */
    AM_TS_MUXER_PSI_STREAM_INFO  *stream[CTSMUXPSI_STREAM_TOT];
    uint32_t                      descriptor_len;
   /*!< Total number of stream within the program. */
   uint16_t                       total_stream;
   uint8_t                        descriptor_tag;
} AM_TS_MUXER_PSI_PMT_INFO;

/**
 *
 * Program Association table [PAT] configuration
 */
typedef struct {
    AM_TS_MUXER_PSI_PRG_INFO *prg_info; /*!< List of program configurations. */
    uint16_t                  total_prg; /*!< Total number of valid programs. */
} AM_TS_MUXER_PSI_PAT_INFO;


/**
 *
 * Header for a Program Association table [PAT]
 * \ref MPEG-2 Systems Spec ISO/IEC 13818-1
 */
typedef struct {
#ifdef BIGENDIAN
   /*Btype 7*/
   uint8_t last_section_number      : 8;
   /*Btype 6*/
   uint8_t section_number           : 8;
   /*Btype 5*/
   uint8_t reserved1                : 2;
   uint8_t version_number           : 5;
   uint8_t current_next_indicator   : 1;
   /*Btype 4*/
   uint8_t transport_stream_id_l    : 8;
   /*Btype 3*/
   uint8_t transport_stream_id_h    : 8;
   /*Btype 2*/
   uint8_t section_length0to7       : 8;
   /*Btype 1*/
   uint8_t section_syntax_indicator : 1;
   uint8_t b0                       : 1;
   uint8_t reserved0                : 2;
   uint8_t section_length8to11      : 4;
   /*Btype 0*/
   uint8_t table_id                 : 8;
#else
   /*Btype 0*/
   uint8_t table_id                 : 8;
   /*Btype 1*/
   uint8_t section_length8to11      : 4;
   uint8_t reserved0                : 2;
   uint8_t b0                       : 1;
   uint8_t section_syntax_indicator : 1;
   /*Btype 2*/
   uint8_t section_length0to7       : 8;
   /*Btype 3*/
   uint8_t transport_stream_id_h    : 8;
   /*Btype 4*/
   uint8_t transport_stream_id_l    : 8;
   /*Btype 5*/
   uint8_t current_next_indicator   : 1;
   uint8_t version_number           : 5;
   uint8_t reserved1                : 2;
   /*Btype 6*/
   uint8_t section_number           : 8;
   /*Btype 7*/
   uint8_t last_section_number      : 8;
#endif
} PAT_HDR;

/**
 *
 * Program Association table [PAT] Element
 * \ref MPEG-2 Systems Spec ISO/IEC 13818-1
 */
typedef struct {
#ifdef BIGENDIAN
   /* Btype 3 */
   uint8_t program_map_pid_l : 8;
   /* Btype 2 */
   uint8_t reserved2         : 3;
   uint8_t program_map_pid_h : 5;
   /* Btype 1 */
   uint8_t program_number_l  : 8;
   /* Btype 0 */
   uint8_t program_number_h  : 8;
#else
   /* Btype 0 */
   uint8_t program_number_h  : 8;
   /* Btype 1 */
   uint8_t program_number_l  : 8;
   /* Btype 2 */
   uint8_t program_map_pid_h : 5;
   uint8_t reserved2         : 3;
   /* Btype 3 */
   uint8_t program_map_pid_l : 8;
#endif
} PAT_ELEMENT;
/**
 *
 * Header for a Program Map Table [PMT]
 * \ref MPEG-2 Systems Spec ISO/IEC 13818-1
 */
typedef struct {
#ifdef BIGENDIAN
   /*Btype 11*/
   uint8_t program_info_length0to7  : 8;
   /*Btype 10*/
   uint8_t reserved3                : 4;
   uint8_t program_info_length8to11 : 4;
   /*Btype 9*/
   uint8_t pcr_pid0to7              : 8;
   /*Btype 8*/
   uint8_t reserved2                : 3;
   uint8_t pcr_pid8to12             : 5;
   /*Btype 7*/
   uint8_t last_section_number      : 8;
   /*Btype 6*/
   uint8_t section_number           : 8;
   /*Btype 5*/
   uint8_t reserved1                : 2;
   uint8_t version_number           : 5;
   uint8_t current_next_indicator   : 1;
   /*Btype 4*/
   uint8_t program_number_l         : 8;
   /*Btype 3*/
   uint8_t program_number_h         : 8;
   /*Btype 2*/
   uint8_t section_length0to7       : 8;
   /*Btype 1*/
   uint8_t section_syntax_indicator : 1;
   uint8_t b0                       : 1;
   uint8_t reserved0                : 2;
   uint8_t section_length8to11      : 4;
   /*Btype 0*/
   uint8_t table_id                 : 8;
#else
   /*Btype 0*/
   uint8_t table_id                 : 8;
   /*Btype 1*/
   uint8_t section_length8to11      : 4;
   uint8_t reserved0                : 2;
   uint8_t b0                       : 1;
   uint8_t section_syntax_indicator : 1;
   /*Btype 2*/
   uint8_t section_length0to7       : 8;
   /*Btype 3*/
   uint8_t program_number_h         : 8;
   /*Btype 4*/
   uint8_t program_number_l         : 8;
   /*Btype 5*/
   uint8_t current_next_indicator   : 1;
   uint8_t version_number           : 5;
   uint8_t reserved1                : 2;
   /*Btype 6*/
   uint8_t section_number           : 8;
   /*Btype 7*/
   uint8_t last_section_number      : 8;
   /*Btype 8*/
   uint8_t pcr_pid8to12             : 5;
   uint8_t reserved2                : 3;
   /*Btype 9*/
   uint8_t pcr_pid0to7              : 8;
   /*Btype 10*/
   uint8_t program_info_length8to11 : 4;
   uint8_t reserved3                : 4;
   /*Btype 11*/
   uint8_t program_info_length0to7  : 8;
#endif
} PMT_HDR;

/**
 *
 * Program Map Table [PMT] element
 * \ref MPEG-2 Systems Spec ISO/IEC 13818-1
 */
typedef struct {
#ifdef BIGENDIAN
   /*Btype 4*/
   uint8_t es_info_length_l    : 8;
   /*Btype 3*/
   uint8_t reserved1           : 4;
   uint8_t es_info_length_h    : 4;
   /*Btype 2*/
   uint8_t elementary_pid0to7  : 8;
   /*Btype 1*/
   uint8_t reserved0           : 3;
   uint8_t elementary_pid8to12 : 5;
   /*Btype 0*/
   uint8_t stream_type         : 8;
#else
   /*Btype 0*/
   uint8_t stream_type         : 8;
   /*Btype 1*/
   uint8_t elementary_pid8to12 : 5;
   uint8_t reserved0           : 3;
   /*Btype 2*/
   uint8_t elementary_pid0to7  : 8;
   /*Btype 3*/
   uint8_t es_info_length_h    : 4;
   uint8_t reserved1           : 4;
   /*Btype 4*/
   uint8_t ES_info_length_l    : 8;
#endif
} PMT_ELEMENT;
/**
 *
 * Stream Descriptor for Program Map Table [PMT] element
 * \ref MPEG-2 Systems Spec ISO/IEC 13818-1
 */
typedef struct {
   uint32_t descriptor_tag:8;
   uint32_t descriptor_length:8;
   uint32_t component_tag:8;
} PMT_STREAM_DSC;

class AMTsBuilder
{
  public:
    AMTsBuilder();
    ~AMTsBuilder(){};
    void destroy();

    /**
     * This method creates TS packet with Program Association table [PAT].
     *
     * @param pat_info informaion required for PAT table.
     * @param pat_buf  pointer to the destination of newly created PAT.
     *
     */
    AM_STATE create_pat(AM_TS_MUXER_PSI_PAT_INFO *pat_info, uint8_t * pat_buf);

    /**
     * This method creates TS packet with Program Map Table [PMT].
     *
     * @param pmt_info informaion required for PAT table.
     * @param pmt_buf  pointer to the destination of newly created PMT.
     *
     */
    AM_STATE create_pmt(AM_TS_MUXER_PSI_PMT_INFO *pmt_info, uint8_t * pmt_buf);

    /**
     * This method creates TS packet with Program Clock Reference packet [PCR].
     *
     * @param pcr_pid pid to be used for PCR packet.
     * @param pcr_buf  pointer to the destination of newly created PCR packet.
     *
     */
    AM_STATE create_pcr(uint16_t pcr_pid, uint8_t * pcr_buf);

    /**
     * This method creates TS NULL packet.
     *
     * @param buf  pointer to the destination of newly created NULL packet.
     *
     */
    AM_STATE create_null(uint8_t * buf);

    int  create_transport_packet(AM_TS_MUXER_PSI_STREAM_INFO *psi_str_info,
                               AM_TS_MUXER_PES_PAYLOAD_INFO* pes_payload_info,
                               uint8_t *pes_buf);

    AM_STATE update_psi_cc(uint8_t * ts_buf); //continuity_counter increment

    int cal_crc32(uint8_t *buf, int size);
    void crc32_byte(int *preg, int x);
    void set_audio_info(AM_AUDIO_INFO* audio_info);
    void set_video_info(AM_VIDEO_INFO* video_info);

  private:
    AM_TS_MUXER_PSI_VER m_ver;  /*!< version info for psi tables. */
    AM_AUDIO_INFO       m_audio_info;
    AM_VIDEO_INFO       m_video_info;
};

#endif
