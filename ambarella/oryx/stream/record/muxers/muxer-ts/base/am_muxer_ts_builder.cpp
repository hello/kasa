/*
 * build_ts.cpp
 *
 * History:
 *	2011/4/17 - [Yi Zhu] created file
 *	2014/12/16 - [Chengcai Jing] modified file
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

#include "am_base_include.h"
#include "am_define.h"
#include "am_log.h"

#include "am_amf_types.h"
#include "am_muxer_ts_builder.h"

#include <sys/ioctl.h> //for ioctl
#include <fcntl.h>     //for open O_* flags
#include <unistd.h>    //for read/write/lseek
#include <time.h>

AMTsBuilder::AMTsBuilder()
{
  m_ver.pat = 0;
  m_ver.pmt = 0;
  memset(&m_audio_info, 0, sizeof(m_audio_info));
  memset(&m_video_info, 0, sizeof(m_video_info));
}

void AMTsBuilder::destroy()
{
  delete this;
}

AM_STATE AMTsBuilder::create_pat(AM_TS_MUXER_PSI_PAT_INFO *pat_info,
                                 uint8_t *pat_buf)
{
  MPEG_TS_TP_HEADER *ts_header = (MPEG_TS_TP_HEADER *) pat_buf;
  /*TS hdr size(4B) + PSI pointer field(1B)*/
  PAT_HDR *pat_header = (PAT_HDR *) (pat_buf + 5);
  uint8_t *pat_content = (uint8_t *) (((uint8_t *) pat_header) + PAT_HDR_SIZE);
  int crc32 = 0;

  // TS HEADER
  ts_header->sync_byte = MPEG_TS_TP_SYNC_BYTE;
  ts_header->transport_error_indicator =
      MPEG_TS_TRANSPORT_ERROR_INDICATOR_NO_ERRORS;
  ts_header->payload_unit_start_indicator =
      MPEG_TS_PAYLOAD_UNIT_START_INDICATOR_START;
  ts_header->transport_priority = MPEG_TS_TRANSPORT_PRIORITY_PRIORITY;
  ts_header->transport_scrambling_control = MPEG_TS_SCRAMBLING_CTRL_NOT_SCRAMBLED;
  ts_header->adaptation_field_control = MPEG_TS_ADAPTATION_FIELD_PAYLOAD_ONLY;
  ts_header->continuity_counter = 0;
  MPEG_TS_TP_HEADER_PID_SET(ts_header, MPEG_TS_PAT_PID);

  // Set PSI pointer field
  pat_buf[4] = 0x00;

  // PAT Header
  pat_header->table_id = 0x00;
  pat_header->section_syntax_indicator = 1;
  pat_header->b0 = 0;
  pat_header->reserved0 = 0x3;
  pat_header->transport_stream_id_l = 0x00;
  pat_header->transport_stream_id_h = 0x00;
  pat_header->reserved1 = 0x3;
  pat_header->version_number = m_ver.pat;
  pat_header->current_next_indicator = 1;
  pat_header->section_number = 0x0;
  pat_header->last_section_number = 0x0;
  pat_header->section_length0to7 = 0; //Update later
  pat_header->section_length8to11 = 0; //Update later

  // add informations for all programs	(only one for now)
  pat_content[0] = (pat_info->prg_info->prg_num >> 8) & 0xff;
  pat_content[1] = pat_info->prg_info->prg_num & 0xff;
  pat_content[2] = 0xE0 | ((pat_info->prg_info->pid_pmt & 0x1fff) >> 8);
  pat_content[3] = pat_info->prg_info->pid_pmt & 0xff;
  pat_content += 4;

  // update patHdr.section_length
  uint16_t section_len = pat_content + 4 - (uint8_t *) pat_header - 3;
  pat_header->section_length8to11 = (section_len & 0x0fff) >> 8;
  pat_header->section_length0to7 = (section_len & 0x00ff);

  // Calc CRC32
  crc32 = cal_crc32((uint8_t*) pat_header,
                    (int) (pat_content - (uint8_t *) pat_header));
  pat_content[0] = (crc32 >> 24) & 0xff;
  pat_content[1] = (crc32 >> 16) & 0xff;
  pat_content[2] = (crc32 >> 8) & 0xff;
  pat_content[3] = crc32 & 0xff;

  // Stuff rest of the packet
  memset(pat_content + 4, /*Stuffing Btypes*/
         0xff, MPEG_TS_TP_PACKET_SIZE - (pat_content + 4 - pat_buf));

  return AM_STATE_OK;
}

AM_STATE AMTsBuilder::update_psi_cc(uint8_t * buf_ts)
{
  ((MPEG_TS_TP_HEADER *) buf_ts)->continuity_counter ++;
  return AM_STATE_OK;
}

AM_STATE AMTsBuilder::create_pmt(AM_TS_MUXER_PSI_PMT_INFO *pmt_info, uint8_t *pmt_buf)
{
  MPEG_TS_TP_HEADER *ts_header = (MPEG_TS_TP_HEADER *) pmt_buf;
  PMT_HDR *PMT_header = (PMT_HDR *) (pmt_buf + 5);
  uint8_t *PMT_content = (uint8_t *) (((uint8_t *) PMT_header) + PMT_HDR_SIZE);
  int crc32;

  // TS HEADER
  ts_header->sync_byte = MPEG_TS_TP_SYNC_BYTE;
  ts_header->transport_error_indicator =
      MPEG_TS_TRANSPORT_ERROR_INDICATOR_NO_ERRORS;
  ts_header->payload_unit_start_indicator =
      MPEG_TS_PAYLOAD_UNIT_START_INDICATOR_START;
  ts_header->transport_priority = MPEG_TS_TRANSPORT_PRIORITY_PRIORITY;

  MPEG_TS_TP_HEADER_PID_SET(ts_header, pmt_info->prg_info->pid_pmt);

  ts_header->transport_scrambling_control = MPEG_TS_SCRAMBLING_CTRL_NOT_SCRAMBLED;
  ts_header->adaptation_field_control = MPEG_TS_ADAPTATION_FIELD_PAYLOAD_ONLY;
  ts_header->continuity_counter = 0;

  // Set PSI poiter field
  pmt_buf[4] = 0x00;

  // PMT HEADER
  PMT_header->table_id = 0x02;
  PMT_header->section_syntax_indicator = 1;
  PMT_header->b0 = 0;
  PMT_header->reserved0 = 0x3;
  PMT_header->section_length0to7 = 0; //update later
  PMT_header->section_length8to11 = 0; //update later
  PMT_header->program_number_h = (pmt_info->prg_info->prg_num >> 8) & 0xff;
  PMT_header->program_number_l = pmt_info->prg_info->prg_num & 0xff;
  PMT_header->reserved1 = 0x3;
  PMT_header->version_number = m_ver.pmt;
  PMT_header->current_next_indicator = 1;
  PMT_header->section_number = 0x0;
  PMT_header->last_section_number = 0x0;
  PMT_header->reserved2 = 0x7;
  PMT_header->pcr_pid8to12 = (pmt_info->prg_info->pid_pcr >> 8) & 0x1f;
  PMT_header->pcr_pid0to7 = pmt_info->prg_info->pid_pcr & 0xff;
  PMT_header->reserved3 = 0xf;
  if (pmt_info->descriptor_len == 0) {
    PMT_header->program_info_length0to7 = 0;
    PMT_header->program_info_length8to11 = 0;
  } else {
    PMT_header->program_info_length8to11 = ((2 + pmt_info->descriptor_len) >> 8)
                                           & 0x0f;
    PMT_header->program_info_length0to7 = ((2 + pmt_info->descriptor_len)
                                           & 0xff);
  }

  if (pmt_info->descriptor_len > 0) {
    PMT_content[0] = pmt_info->descriptor_tag;
    PMT_content[1] = pmt_info->descriptor_len;
    memcpy(&PMT_content[2], pmt_info->descriptor, pmt_info->descriptor_len);
    PMT_content += (2 + pmt_info->descriptor_len);
  }

  // Add all stream elements
  for (uint16_t strNum = 0; strNum < pmt_info->total_stream; ++ strNum) {
    AM_TS_MUXER_PSI_STREAM_INFO * str_info = pmt_info->stream[strNum];
    PMT_ELEMENT *PMT_element = (PMT_ELEMENT *) PMT_content;
    PMT_element->stream_type = str_info->type;
    PMT_element->reserved0 = 0x7; // 3 bits
    PMT_element->elementary_pid8to12 = ((str_info->pid & 0x1fff) >> 8);
    PMT_element->elementary_pid0to7 = (str_info->pid & 0xff);
    PMT_element->reserved1 = 0xf; // 4 bits
    PMT_element->es_info_length_h = (((str_info->descriptor_len + 2) >> 8) & 0x0f);
    PMT_element->ES_info_length_l = (str_info->descriptor_len + 2) & 0xff;

    PMT_content += PMT_ELEMENT_SIZE;
    PMT_content[0] = str_info->descriptor_tag;	//descriptor_tag
    PMT_content[1] = str_info->descriptor_len;	//descriptor_length

    if (str_info->descriptor_len > 0) {
      memcpy(&PMT_content[2], str_info->descriptor, str_info->descriptor_len);
    }
    PMT_content += (2 + str_info->descriptor_len);
  }

  // update pmtHdr.section_length
  uint16_t section_len = PMT_content + 4 - ((uint8_t *) PMT_header + 3);
  PMT_header->section_length8to11 = (section_len >> 8) & 0x0f;
  PMT_header->section_length0to7 = (section_len & 0xff);

  // Calc CRC32
  crc32 = cal_crc32((uint8_t*) PMT_header,
                    (int) (PMT_content - (uint8_t*) PMT_header));
  PMT_content[0] = (crc32 >> 24) & 0xff;
  PMT_content[1] = (crc32 >> 16) & 0xff;
  PMT_content[2] = (crc32 >> 8) & 0xff;
  PMT_content[3] = crc32 & 0xff;
  // Stuff rest of the packet
  memset((PMT_content + 4), 0xff,
         (MPEG_TS_TP_PACKET_SIZE - (((uint8_t*) PMT_content + 4) - pmt_buf)));

  return AM_STATE_OK;
}

AM_STATE AMTsBuilder::create_pcr(uint16_t pcr_pid, uint8_t *pcr_buf)
{
  MPEG_TS_TP_HEADER *ts_header = (MPEG_TS_TP_HEADER *) pcr_buf;
  uint8_t *adaptation_field = (uint8_t*) (pcr_buf +
  MPEG_TS_TP_PACKET_HEADER_SIZE);

  // TS HEADER
  ts_header->sync_byte = MPEG_TS_TP_SYNC_BYTE;
  ts_header->transport_error_indicator =
      MPEG_TS_TRANSPORT_ERROR_INDICATOR_NO_ERRORS;
  ts_header->payload_unit_start_indicator =
      MPEG_TS_PAYLOAD_UNIT_START_INDICATOR_NORMAL;
  ts_header->transport_priority = MPEG_TS_TRANSPORT_PRIORITY_NO_PRIORITY;
  MPEG_TS_TP_HEADER_PID_SET(ts_header, pcr_pid);
  ts_header->transport_scrambling_control =
      MPEG_TS_SCRAMBLING_CTRL_NOT_SCRAMBLED;
  ts_header->adaptation_field_control = MPEG_TS_SCRAMBLING_CTRL_NOT_SCRAMBLED;
  ts_header->continuity_counter = 0;

  // Set adaptation_field
  adaptation_field[0] = 183; /* adapt_length : 1+6+176 bytes */
  adaptation_field[1] = 0x10; /* discont - adapt_ext_flag : PCR on */

  memset(&adaptation_field[2], 0xcc, 6); /* dummy PCR */
  memset(&adaptation_field[8], 0xff, 176); /* stuffing_byte */

  return AM_STATE_OK;
}

AM_STATE AMTsBuilder::create_null(uint8_t * buf)
{
  MPEG_TS_TP_HEADER *ts_header = (MPEG_TS_TP_HEADER *) buf;

  // TS HEADER
  ts_header->sync_byte = MPEG_TS_TP_SYNC_BYTE;
  ts_header->transport_error_indicator =
      MPEG_TS_TRANSPORT_ERROR_INDICATOR_NO_ERRORS;
  ts_header->payload_unit_start_indicator =
      MPEG_TS_PAYLOAD_UNIT_START_INDICATOR_NORMAL;
  ts_header->transport_priority = MPEG_TS_TRANSPORT_PRIORITY_NO_PRIORITY;
  MPEG_TS_TP_HEADER_PID_SET(ts_header, MPEG_TS_NULL_PID);
  ts_header->transport_scrambling_control =
      MPEG_TS_SCRAMBLING_CTRL_NOT_SCRAMBLED;

  // No adaptation field payload only.
  ts_header->adaptation_field_control = MPEG_TS_ADAPTATION_FIELD_PAYLOAD_ONLY;
  ts_header->continuity_counter = 0;

  // Stuff bytes
  memset((buf + MPEG_TS_TP_PACKET_HEADER_SIZE), 0xff,
  MPEG_TS_TP_PACKET_SIZE - MPEG_TS_TP_PACKET_HEADER_SIZE);

  return AM_STATE_OK;
}

int AMTsBuilder::create_transport_packet(AM_TS_MUXER_PSI_STREAM_INFO *str_info,
                                         AM_TS_MUXER_PES_PAYLOAD_INFO *payload_info,
                                         uint8_t *pes_buf)
{
  MPEG_TS_TP_HEADER *ts_header = (MPEG_TS_TP_HEADER *) pes_buf;
  uint8_t *pes_packet = (uint8_t *) (pes_buf + MPEG_TS_TP_PACKET_HEADER_SIZE);
  uint8_t *tmp_write_ptr = NULL;

  /* pre-calculate pesHeaderDataLength so that
   * Adapt filed can decide if stuffing is required
   */
  uint32_t pes_pts_dts_flag = MPEG_TS_PTS_DTS_NO_PTSDTS;
  uint32_t PES_header_data_length = 0; /* PES optional field data length */
  if (payload_info->first_slice) { /*the PES packet will start in the first slice*/
    /* check PTS DTS delta */
    if ((payload_info->pts <= (payload_info->dts + 1)) &&
        (payload_info->dts <= (payload_info->pts + 1))) {
      pes_pts_dts_flag = MPEG_TS_PTS_DTS_PTS_ONLY;
      PES_header_data_length = PTS_FIELD_SIZE;
    } else {
      pes_pts_dts_flag = MPEG_TS_PTS_DTS_PTSDTS_BOTH;
      PES_header_data_length = PTS_FIELD_SIZE + DTS_FIELD_SIZE;
    }
  }

  // TS header
  ts_header->sync_byte = MPEG_TS_TP_SYNC_BYTE;
  ts_header->transport_error_indicator =
      MPEG_TS_TRANSPORT_ERROR_INDICATOR_NO_ERRORS;
  ts_header->payload_unit_start_indicator = /*a PES starts in the current pkt*/
  (payload_info->first_slice ? MPEG_TS_PAYLOAD_UNIT_START_INDICATOR_START :
                     MPEG_TS_PAYLOAD_UNIT_START_INDICATOR_NORMAL);
  ts_header->transport_priority = MPEG_TS_TRANSPORT_PRIORITY_NO_PRIORITY;
  ts_header->transport_scrambling_control =
      MPEG_TS_SCRAMBLING_CTRL_NOT_SCRAMBLED;
  ts_header->continuity_counter = ((payload_info->first_frame &&
      payload_info->first_slice) ? 0 :
      ((pes_buf[3] + 1) & 0x0f)); /* increase the counter */
  MPEG_TS_TP_HEADER_PID_SET(ts_header, str_info->pid);

  /* Adaptation field */
  /* For Transport Stream packets carrying PES packets,
   * stuffing is needed when there is insufficient
   * PES packet data to completely fill the Transport
   * Stream packet payload bytes */

  MPEG_TS_TP_ADAPTATION_FIELD_HEADER *adapt_header =
      (MPEG_TS_TP_ADAPTATION_FIELD_HEADER *) (pes_buf +
      MPEG_TS_TP_PACKET_HEADER_SIZE);
  tmp_write_ptr = (uint8_t *) adapt_header; /* point to adapt header */

  // fill the common fields
  adapt_header->adaptation_field_length = 0;
  adapt_header->adaptation_field_extension_flag =
      MPEG_TS_ADAPTATION_FIELD_EXTENSION_NOT_PRESENT;
  adapt_header->transport_private_data_flag =
      MPEG_TS_TRANSPORT_PRIVATE_DATA_NOT_PRESENT;
  adapt_header->splicing_point_flag = MPEG_TS_SPLICING_POINT_FLAG_NOT_PRESENT;
  adapt_header->opcr_flag = MPEG_TS_OPCR_FLAG_NOT_PRESENT;
  adapt_header->pcr_flag = MPEG_TS_PCR_FLAG_NOT_PRESENT;
  adapt_header->elementary_stream_priority_indicator =
      MPEG_TS_ELEMENTARY_STREAM_PRIORITY_NO_PRIORITY;
  adapt_header->random_access_indicator =
      MPEG_TS_RANDOM_ACCESS_INDICATOR_NOT_PRESENT;
  adapt_header->discontinuity_indicator = /* assume no discontinuity occur */
  MPEG_TS_DISCONTINUITY_INDICATOR_NO_DISCONTINUTY;

  int32_t initial_adapt_field_size = 0;
  if (payload_info->with_pcr) {
    //# of bytes following adaptation_field_length (1 + 6 pcr size)
    adapt_header->adaptation_field_length = 1 + PCR_FIELD_SIZE;
    adapt_header->pcr_flag = MPEG_TS_PCR_FLAG_PRESENT;
    adapt_header->random_access_indicator = MPEG_TS_RANDOM_ACCESS_INDICATOR_PRESENT;

    /*skip adapt hdr and point to optional field (PCR field)*/
    tmp_write_ptr += ADAPT_HEADER_LEN;
    tmp_write_ptr[0] = payload_info->pcr_base >> 25;
    tmp_write_ptr[1] = (payload_info->pcr_base & 0x1fe0000) >> 17;
    tmp_write_ptr[2] = (payload_info->pcr_base & 0x1fe00) >> 9;
    tmp_write_ptr[3] = (payload_info->pcr_base & 0x1fe) >> 1;
    tmp_write_ptr[4] = ((payload_info->pcr_base & 0x1) << 7) | (0x7e) |
                       (payload_info->pcr_ext >> 8);
    tmp_write_ptr[5] = payload_info->pcr_ext & 0xff;
    /*point to the start of potential stuffing area*/
    tmp_write_ptr += PCR_FIELD_SIZE;
    initial_adapt_field_size = adapt_header->adaptation_field_length + 1;
  }

  // check if stuffing is required and calculate stuff size
  uint32_t pes_header_size = 0;
  int32_t stuff_size = 0;

  if (payload_info->first_slice) {
    pes_header_size = PES_HEADER_LEN + PES_header_data_length;
  }

  int32_t pes_payload_space = MPEG_TS_TP_PACKET_SIZE -
  MPEG_TS_TP_PACKET_HEADER_SIZE - initial_adapt_field_size - pes_header_size;
  if (payload_info->payload_size < pes_payload_space) {
    stuff_size = pes_payload_space - payload_info->payload_size;
  }

  if (stuff_size > 0) { // need stuffing
    int32_t real_stuff_size = stuff_size;

    if (initial_adapt_field_size == 0) { // adapt header is not written yet
      if (stuff_size == 1) {
        tmp_write_ptr ++;   // write the adapt_field_length byte (=0)
        real_stuff_size --;
      } else {
        tmp_write_ptr += 2; // write the two byte adapt header
        real_stuff_size -= 2;
        // adaptation_field_length 0 --> 1
        adapt_header->adaptation_field_length = 1;
      }
    }

    if (real_stuff_size > 0) { // stuff size should be >= 2
      // tmpWritePtr should point to the start of stuffing area
      memset(tmp_write_ptr, 0xff, real_stuff_size);
      tmp_write_ptr += real_stuff_size;
      adapt_header->adaptation_field_length += real_stuff_size;
    }
  }

  // update adaptation_field_control of TS header
  ts_header->adaptation_field_control =
      (payload_info->with_pcr || stuff_size > 0) ?
       MPEG_TS_ADAPTATION_FIELD_BOTH : MPEG_TS_ADAPTATION_FIELD_PAYLOAD_ONLY;

  // calcuate the start addr of PES packet
  pes_packet = tmp_write_ptr;
  //printf("FirstSlice %d, withPCR %d, payload %d, adaptField %d, stuff %d\n",
  //       pFd->firstSlice, pFd->withPCR, pFd->payloadSize,
  //       pesPacket - (uint8_t*)adaptHdr, stuffSize);

  /* TS packet payload (PES header + PES payload or PES playload only) */
  if (ts_header->payload_unit_start_indicator) {
    // one PES packet is started in this transport packet
    MPEG_TS_TP_PES_HEADER *PES_header = (MPEG_TS_TP_PES_HEADER *) pes_packet;

    PES_header->packet_start_code_23to16 = 0;
    PES_header->packet_start_code_15to8 = 0;
    PES_header->packet_start_code_7to0 = 0x01;
    PES_header->marker_bits = 2;
    PES_header->pes_scrambling_control =
        MPEG_TS_PES_SCRAMBLING_CTRL_NOT_SCRAMBLED;
    PES_header->pes_priority = 0;
    PES_header->data_alignment_indicator =
        MPEG_TS_PES_ALIGNMENT_CONTROL_STARTCODE;
    PES_header->copyright = MPEG_TS_PES_COPYRIGHT_UNDEFINED;
    PES_header->original_or_copy = MPEG_TS_PES_ORIGINAL_OR_COPY_COPY;
    PES_header->escr_flag = MPEG_TS_PES_ESCR_NOT_PRESENT;
    PES_header->es_rate_flag = MPEG_TS_PES_ES_NOT_PRESENT;
    PES_header->dsm_trick_mode_flag = MPEG_TS_PES_DSM_TRICK_MODE_NOT_PRESENT;
    PES_header->add_copy_info_flag = MPEG_TS_PES_ADD_COPY_INFO_NOT_PRESENT;
    PES_header->pes_crc_flag = MPEG_TS_PES_CRC_NOT_PRESENT;
    PES_header->pes_ext_flag = MPEG_TS_PES_EXT_NOT_PRESENT;
    PES_header->pts_dts_flags = pes_pts_dts_flag;
    PES_header->header_data_length = PES_header_data_length;

    /*Set stream_id & pes_packet_size*/
    uint16_t PES_packet_size;
    if ((str_info->type == MPEG_SI_STREAM_TYPE_AVC_VIDEO) || (
        str_info->type == MPEG_SI_STREAM_TYPE_HEVC_VIDEO)) {
      PES_header->stream_id = MPEG_TS_STREAM_ID_VIDEO_00;
      /* TS header + adaptation field */
      if (payload_info->payload_size
          < (MPEG_TS_TP_PACKET_SIZE - (((uint8_t *) pes_packet) - pes_buf) -
          PES_HEADER_LEN - PES_header->header_data_length)) {
        // 3 bytes following PES_packet_length field
        PES_packet_size = 3 + PES_header->header_data_length +
                          (payload_info->payload_size);
      } else {
        PES_packet_size = 0;
      }
    } else if (str_info->type == MPEG_SI_STREAM_TYPE_AAC) {
      PES_header->stream_id = MPEG_TS_STREAM_ID_AUDIO_00;
      // 3 bytes following PES_packet_length field
      PES_packet_size = 3 + PES_header->header_data_length +
                        (payload_info->payload_size);
    } else if (str_info->type == MPEG_SI_STREAM_TYPE_LPCM_AUDIO) {
      PES_header->stream_id = MPEG_TS_STREAM_ID_PRIVATE_STREAM_1;
      // add 4 bytes for LPCMAudioDataHeader
      PES_packet_size = 3 + PES_header->header_data_length +
          (payload_info->payload_size) + 4;
    } else {
      ERROR("Stream type error.");
      return -1;
    }

    PES_header->pes_packet_length_15to8 = ((PES_packet_size) >> 8) & 0xff;
    PES_header->pes_packet_length_7to0 = (PES_packet_size) & 0xff;

    // point to PES header optional field
    tmp_write_ptr += PES_HEADER_LEN;

    switch (PES_header->pts_dts_flags) {
      case MPEG_TS_PTS_DTS_PTS_ONLY: {
        tmp_write_ptr[0] = 0x21 | (((payload_info->pts >> 30) & 0x07) << 1);
        tmp_write_ptr[1] = ((payload_info->pts >> 22) & 0xff);
        tmp_write_ptr[2] = (((payload_info->pts >> 15) & 0x7f) << 1) | 0x1;
        tmp_write_ptr[3] = ((payload_info->pts >> 7) & 0xff);
        tmp_write_ptr[4] = ((payload_info->pts & 0x7f) << 1) | 0x1;
      }
        break;
      case MPEG_TS_PTS_DTS_PTSDTS_BOTH: {
        tmp_write_ptr[0] = 0x31 | (((payload_info->pts >> 30) & 0x07) << 1);
        tmp_write_ptr[1] = ((payload_info->pts >> 22) & 0xff);
        tmp_write_ptr[2] = (((payload_info->pts >> 15) & 0x7f) << 1) | 0x1;
        tmp_write_ptr[3] = ((payload_info->pts >> 7) & 0xff);
        tmp_write_ptr[4] = ((payload_info->pts & 0x7f) << 1) | 0x1;

        tmp_write_ptr[5] = 0x11 | (((payload_info->dts >> 30) & 0x07) << 1);
        tmp_write_ptr[6] = ((payload_info->dts >> 22) & 0xff);
        tmp_write_ptr[7] = (((payload_info->dts >> 15) & 0x7f) << 1) | 0x1;
        tmp_write_ptr[8] = ((payload_info->dts >> 7) & 0xff);
        tmp_write_ptr[9] = ((payload_info->dts & 0x7f) << 1) | 0x1;
      }
        break;
      case MPEG_TS_PTS_DTS_NO_PTSDTS:
      default:
        break;
    }
    tmp_write_ptr += PES_header->header_data_length;

    if (str_info->type == MPEG_SI_STREAM_TYPE_LPCM_AUDIO) {
      //AVCHDLPCMAudioDataHeader() {
      //      AudioDataPayloadSize 16
      //      ChannelAssignment     4      3 for stereo
      //      SamplingFrequency     4      1 for 48kHz
      //      BitPerSample          2      1 for 16-bit
      //      StartFlag             1
      //      reserved              5
      //}
      tmp_write_ptr[0] = (payload_info->payload_size >> 8) & 0xFF;       //0x03
      tmp_write_ptr[1] = payload_info->payload_size & 0xFF;       //0xC0;
      if (m_audio_info.channels == 2) {
        tmp_write_ptr[2] = 0x31; //48kHz stereo
      } else if (m_audio_info.channels == 1) {
        tmp_write_ptr[2] = 0x11; //48kHz mono
      }
      tmp_write_ptr[3] = 0x60; //16 bits per sample
      tmp_write_ptr += 4;
    }
  }

  // tmpWritePtr should be the latest write point
  int payload_fillsize = MPEG_TS_TP_PACKET_SIZE - (tmp_write_ptr - pes_buf);

  AM_ASSERT(payload_fillsize <= payload_info->payload_size);
  memcpy(tmp_write_ptr, payload_info->p_payload, payload_fillsize);
  return payload_fillsize;
}

inline int AMTsBuilder::cal_crc32(uint8_t * buf, int size)
{
  int crc = 0xffffffffL;

  for (int i = 0; i < size; ++ i) {
    crc32_byte(&crc, (int) buf[i]);
  }
  return (crc);
}

inline void AMTsBuilder::crc32_byte(int *preg, int x)
{
  int i;
  for (i = 0, x <<= 24; i < 8; ++ i, x <<= 1) {
    (*preg) = ((*preg) << 1) ^ (((x ^ (*preg)) >> 31) & 0x04C11DB7);
  }
}

void AMTsBuilder::set_audio_info(AM_AUDIO_INFO* audio_info)
{
  if (AM_LIKELY(audio_info)) {
    memcpy(&m_audio_info, audio_info, sizeof(m_audio_info));
  } else {
    memset(&m_audio_info, 0, sizeof(m_audio_info));
  }
}

void AMTsBuilder::set_video_info(AM_VIDEO_INFO* video_info)
{
  if (AM_LIKELY(video_info)) {
    memcpy(&m_video_info, video_info, sizeof(m_video_info));
  } else {
    memset(&m_video_info, 0, sizeof(m_video_info));
  }
}

