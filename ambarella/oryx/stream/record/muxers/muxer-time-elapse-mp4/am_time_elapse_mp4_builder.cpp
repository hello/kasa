/*******************************************************************************
 * am_time_elapse_mp4_builder.cpp
 *
 * History:
 *   2016-05-12 - [ccjing] created file
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
#include "am_base_include.h"
#include "am_define.h"
#include "am_log.h"

#include "am_amf_types.h"
#include "am_amf_packet.h"

#include "am_utils.h"
#include "am_muxer_codec_if.h"
#include "am_muxer_codec_info.h"
#include "am_file_sink_if.h"
#include "am_video_param_sets_parser_if.h"
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>

#include "am_time_elapse_mp4_builder.h"
#include "am_time_elapse_mp4_config.h"
#include "am_time_elapse_mp4_file_writer.h"

#define CHUNK_OFFSET64_ENABLE           0
#define OBJECT_DESC_BOX_ENABLE          0
#define MP4_TRACK_HEADER_BOX_PRE_LENGTH 1500
#define MP4_SAMPLE_TABLE_PRE_LENGTH     32
#define AM_PTS_DEFINE                   90000

/* seconds from 1904-01-01 00:00:00 UTC to 1969-12-31 24:00:00 UTC */
#define TIME_OFFSET    2082844800ULL

AMTimeElapseMp4Builder* AMTimeElapseMp4Builder::create(
    AMTimeElapseMp4FileWriter* file_writer,
    TimeElapseMp4ConfigStruct* config)
{
  AMTimeElapseMp4Builder *result = new AMTimeElapseMp4Builder();
  if (result && result->init(file_writer, config)
      != AM_STATE_OK) {
    delete result;
    result = NULL;
  }
  return result;
}

AMTimeElapseMp4Builder::AMTimeElapseMp4Builder()
{
  m_h264_nalu_list.clear();
  m_h265_nalu_list.clear();
  m_h265_nalu_frame_queue.clear();
}

AM_STATE AMTimeElapseMp4Builder::init(AMTimeElapseMp4FileWriter* file_writer,
                                      TimeElapseMp4ConfigStruct* config)
{
  AM_STATE ret = AM_STATE_OK;
  do {
    default_mp4_file_setting();
    if (AM_UNLIKELY(!file_writer)) {
      ERROR("Invalid parameter.");
      ret = AM_STATE_ERROR;
      break;
    }
    if (AM_UNLIKELY((m_sei_queue = new AMPacketDQ()) == nullptr)) {
      ERROR("Failed to create sei_queue.");
      ret = AM_STATE_NO_MEMORY;
      break;
    }
    if (AM_UNLIKELY((m_h265_frame_queue = new AMPacketDQ()) == nullptr)) {
      ERROR("Failed to create h265_frame_queue.");
      ret = AM_STATE_NO_MEMORY;
      break;
    }
    m_config = config;
    m_file_writer = file_writer;
  } while (0);
  return ret;
}

AMTimeElapseMp4Builder::~AMTimeElapseMp4Builder()
{
  m_file_writer = NULL;
  while (!m_sei_queue->empty()) {
    m_sei_queue->front()->release();
    m_sei_queue->pop_front();
  }
  delete m_sei_queue;
  while(!m_h265_frame_queue->empty()) {
    m_h265_frame_queue->front()->release();
    m_h265_frame_queue->pop_front();
  }
  delete m_h265_frame_queue;
  m_h264_nalu_list.clear();
  m_h265_nalu_list.clear();
  m_h265_nalu_frame_queue.clear();
  delete[] m_avc_sps;
  delete[] m_avc_pps;
  delete[] m_hevc_vps;
  delete[] m_hevc_sps;
  delete[] m_hevc_pps;
}

AM_STATE AMTimeElapseMp4Builder::set_video_info(AM_VIDEO_INFO* video_info)
{
  AM_STATE ret = AM_STATE_OK;
  if(AM_UNLIKELY(!video_info)) {
    ERROR("Invalid parameter of set video info");
    ret = AM_STATE_ERROR;
  } else {
    memcpy(&m_video_info, video_info, sizeof(AM_VIDEO_INFO));
  }
  return ret;
}

AM_STATE AMTimeElapseMp4Builder::write_h264_video_data(AMPacket *packet)
{
  AM_STATE ret = AM_STATE_OK;
  uint32_t chunk_offset = m_file_writer->get_file_offset();
  H264_NALU_TYPE expect_type = H264_IBP_HEAD;
  uint8_t sync_sample = 0;
  do {
    switch(packet->get_frame_type()) {
      case AM_VIDEO_FRAME_TYPE_I:
      case AM_VIDEO_FRAME_TYPE_B:
      case AM_VIDEO_FRAME_TYPE_P:
        expect_type = H264_IBP_HEAD;
        break;
      case AM_VIDEO_FRAME_TYPE_IDR:
        expect_type = H264_IDR_HEAD;
        sync_sample = 1;
        break;
      default:
        break;
    }
    if(AM_LIKELY(find_h264_nalu(packet->get_data_ptr(), packet->get_data_size(),
                           expect_type))) {
      for(uint32_t i = 0;
          (i < m_h264_nalu_list.size()) && (ret == AM_STATE_OK); ++ i) {
        AM_H264_NALU &nalu = m_h264_nalu_list[i];
        switch (nalu.type) {
          case H264_SPS_HEAD: {
            if (m_avc_sps == nullptr) {
              m_avc_sps = new uint8_t[nalu.size];
              if(!m_avc_sps) {
                ERROR("Failed to malloc memory for avc sps.");
                ret = AM_STATE_ERROR;
                break;
              }
              m_avc_sps_length = nalu.size;
              memcpy(m_avc_sps, nalu.addr, nalu.size);
              if(AM_UNLIKELY(!parse_h264_sps(m_avc_sps, m_avc_sps_length))) {
                ERROR("Failed to parse avc sps.");
                ret = AM_STATE_ERROR;
                break;
              }
            }
          }break;
          case H264_PPS_HEAD: {
            if (m_avc_pps == nullptr) {
              m_avc_pps = new uint8_t[nalu.size];
              if(!m_avc_pps) {
                ERROR("Failed to malloc memory for avc pps.");
                ret = AM_STATE_ERROR;
                break;
              }
              m_avc_pps_length = nalu.size;
              memcpy(m_avc_pps, nalu.addr, nalu.size);
              if(AM_UNLIKELY(!parse_h264_pps(nalu.addr, nalu.size))) {
                ERROR("Failed to parse avc pps.");
                ret = AM_STATE_ERROR;
                break;
              }
            }
          }break;
          case H264_AUD_HEAD: {
            while(!m_sei_queue->empty()) {
              AMPacket *sei = m_sei_queue->front();
              if (sei) {
                uint32_t length = sei->get_data_size() - 4;
                if (AM_UNLIKELY(m_file_writer->write_be_u32(length) !=
                    AM_STATE_OK)) {
                  ERROR("Failed to write sei data length.");
                  ret = AM_STATE_IO_ERROR;
                }
                if (AM_UNLIKELY(m_file_writer->write_data(
                    sei->get_data_ptr() + 4, length) != AM_STATE_OK)) {
                  ERROR("Failed to write sei data to file.");
                  ret = AM_STATE_IO_ERROR;
                }
                sei->release();
                INFO("Get usr SEI %d", length);
              }
              m_sei_queue->pop_front();
            }
          }break;
          case H264_IBP_HEAD:
          case H264_IDR_HEAD: {
            if (AM_UNLIKELY(m_file_writer->write_be_u32(nalu.size) !=
                AM_STATE_OK)) {
              ERROR("Write nalu length error.");
              ret = AM_STATE_IO_ERROR;
              break;
            }
            if (AM_UNLIKELY(m_file_writer->write_data(nalu.addr, nalu.size) !=
                AM_STATE_OK)) {
              ERROR("Failed to nalu data to file.");
              ret = AM_STATE_IO_ERROR;
              break;
            }
          }break;
          default: {
            NOTICE("nalu type is not needed, drop this nalu");
          }break;
        }
      }
      if(AM_UNLIKELY(ret != AM_STATE_OK)) {
        break;
      }
      m_overall_media_data_len += m_file_writer->get_file_offset() - chunk_offset;
      m_video_duration += (AM_PTS_DEFINE / m_config->frame_rate);
      ++ m_video_frame_number;
      if (AM_UNLIKELY((ret = write_video_frame_info(chunk_offset,
                m_file_writer->get_file_offset() - chunk_offset, sync_sample))
                      != AM_STATE_OK)) {
        ERROR("Failed to write video frame info.");
        break;
      }
    } else {
      ERROR("Find nalu error.");
      ret = AM_STATE_ERROR;
      break;
    }
    if (AM_UNLIKELY(m_write_media_data_started == 0)) {
      m_write_media_data_started = 1;
    }
  } while(0);
  return ret;
}

AM_STATE AMTimeElapseMp4Builder::write_h265_video_data(AMPacket *packet)
{
  AM_STATE ret = AM_STATE_OK;
  do {
    if(AM_LIKELY(find_h265_nalu(packet->get_data_ptr(), packet->get_data_size()))) {
      for(uint32_t i = 0;
          (i < m_h265_nalu_list.size()) && (ret == AM_STATE_OK); ++ i) {
        AM_H265_NALU &nalu = m_h265_nalu_list[i];
        switch (nalu.type) {
          case H265_VPS: {
            if (m_hevc_vps == NULL) {
              m_hevc_vps = new uint8_t[nalu.size];
              m_hevc_vps_length = nalu.size;
              INFO("hevc vps length is %u", m_hevc_vps_length);
              if (AM_UNLIKELY(!m_hevc_vps)) {
                ERROR("Failed to alloc memory for hevc vps info.");
                ret = AM_STATE_ERROR;
                break;
              }
              memcpy(m_hevc_vps, nalu.addr, nalu.size);
              if(AM_UNLIKELY(!parse_h265_vps(nalu.addr, nalu.size))) {
                ERROR("Failed to parse h265 vps.");
                ret = AM_STATE_ERROR;
                break;
              }
            }
          }break;
          case H265_SPS: {
            if (m_hevc_sps == NULL) {
              m_hevc_sps = new uint8_t[nalu.size];
              if (AM_UNLIKELY(!m_hevc_sps)) {
                ERROR("Failed to alloc memory for hevc sps info.");
                ret = AM_STATE_ERROR;
                break;
              }
              m_hevc_sps_length = nalu.size;
              INFO("hevc sps length is %u", m_hevc_sps_length);
              memcpy(m_hevc_sps, nalu.addr, nalu.size);
              if(AM_UNLIKELY(!parse_h265_sps(nalu.addr, nalu.size))) {
                ERROR("Failed to parse sps.");
                ret = AM_STATE_ERROR;
                break;
              }
            }
          }break;
          case H265_PPS : {
            if (m_hevc_pps == NULL) {
              m_hevc_pps = new uint8_t[nalu.size];
              if (AM_UNLIKELY(!m_hevc_pps)) {
                ERROR("Failed to alloc memory for hevc pps info.");
                ret = AM_STATE_ERROR;
                break;
              }
              m_hevc_pps_length = nalu.size;
              INFO("hevc pps length is %u", m_hevc_pps_length);
              memcpy(m_hevc_pps, nalu.addr, nalu.size);
              if(AM_UNLIKELY(!parse_h265_pps(nalu.addr, nalu.size))) {
                ERROR("Failed to parse pps.");
                ret = AM_STATE_ERROR;
                break;
              }
            }
          } break;
          case H265_AUD:
          case H265_SEI_PREFIX :
          case H265_SEI_SUFFIX :{
            while (!m_sei_queue->empty()) {
              AMPacket *sei = m_sei_queue->front();
              if (sei){
                uint32_t length = sei->get_data_size() - 4;
                if (AM_UNLIKELY(m_file_writer->write_be_u32(length)
                    != AM_STATE_OK)) {
                  ERROR("Failed to write sei data length.");
                  ret = AM_STATE_IO_ERROR;
                }
                if (AM_UNLIKELY(m_file_writer->write_data(sei->get_data_ptr()
                                + 4, length) != AM_STATE_OK)) {
                  ERROR("Failed to write sei data to file.");
                  ret = AM_STATE_IO_ERROR;
                }
                INFO("Get usr SEI %d", length);
              }
            }
            while (!m_sei_queue->empty()) {
              m_sei_queue->front()->release();
              m_sei_queue->pop_front();
            }
          }break;
          case H265_TRAIL_N:
          case H265_TRAIL_R:
          case H265_TSA_N :
          case H265_TSA_R :
          case H265_STSA_N :
          case H265_STSA_R :
          case H265_RADL_N :
          case H265_RADL_R :
          case H265_BLA_W_LP :
          case H265_BLA_W_RADL :
          case H265_BLA_N_LP :
          case H265_IDR_W_RADL :
          case H265_IDR_N_LP :
          case H265_CRA_NUT :
          case H265_EOS_NUT :
          case H265_EOB_NUT :
          case H265_FD_NUT : {
            uint32_t frame_count = packet->get_frame_count();
            uint8_t slice_id = (uint8_t)((frame_count & 0x00ff0000) >> 16);
            uint8_t tile_id = (uint8_t)((frame_count & 0x000000ff));
            if ((slice_id == 0) && (tile_id == 0)) {
              while(!m_h265_frame_queue->empty()) {
                m_h265_frame_queue->front()->release();
                m_h265_frame_queue->pop_front();
              }
              m_hevc_slice_num = ((uint8_t)((frame_count & 0xff000000) >> 24)
                  == 0) ? 1 : (uint8_t)((frame_count & 0xff000000) >> 24);
              m_hevc_tile_num = ((uint8_t)((frame_count & 0x0000ff00) >> 8)
                  == 0) ? 1 : (uint8_t)((frame_count & 0x0000ff00) >> 8);
            }
            if ((slice_id == (m_hevc_slice_num - 1)) &&
                (tile_id == (m_hevc_tile_num - 1))) {
              m_h265_frame_state = AM_H265_FRAME_END;
              DEBUG("frame end silce num is %u, slice id is %u\n"
                     "tile num is %u, tile id is %u",
                     m_hevc_slice_num, slice_id, m_hevc_tile_num, tile_id);
            } else {
              m_h265_frame_state = AM_H265_FRAME_SLICE;
            }
            if (AM_LIKELY(AM_H265_FRAME_NONE != m_h265_frame_state)) {
              m_h265_nalu_frame_queue.push_back(nalu);
            }
          } break;
          default: {
            NOTICE("nalu type is not needed, drop this nalu");
          } break;
        }
      }
      if (AM_LIKELY(AM_H265_FRAME_NONE != m_h265_frame_state)) {
        packet->add_ref();
        m_h265_frame_queue->push_back(packet);
      }
      if(AM_UNLIKELY(ret != AM_STATE_OK)) {
        break;
      }
      if (AM_LIKELY(AM_H265_FRAME_END == m_h265_frame_state)) {
        uint32_t  frame_size = 0;
        uint32_t  chunk_offset = m_file_writer->get_file_offset();
        uint8_t   sync_sample = (AM_VIDEO_FRAME_TYPE_IDR ==
                                 m_h265_frame_queue->front()->get_frame_type());
        while (!m_h265_nalu_frame_queue.empty()) {
          AM_H265_NALU nalu = m_h265_nalu_frame_queue.front();
          m_h265_nalu_frame_queue.pop_front();
          if (AM_UNLIKELY(m_file_writer->write_be_u32(nalu.size) !=
              AM_STATE_OK)) {
            ERROR("Write nalu length error.");
            ret = AM_STATE_IO_ERROR;
            break;
          }
          if (AM_UNLIKELY(m_file_writer->write_data(nalu.addr, nalu.size) !=
              AM_STATE_OK)) {
            ERROR("Failed to nalu data to file.");
            ret = AM_STATE_IO_ERROR;
            break;
          }
        }
        while(!m_h265_frame_queue->empty()) {
          m_h265_frame_queue->front()->release();
          m_h265_frame_queue->pop_front();
        }
        frame_size = m_file_writer->get_file_offset() - chunk_offset;
        m_overall_media_data_len += frame_size;
        m_video_duration += (AM_PTS_DEFINE / m_config->frame_rate);
        ++ m_video_frame_number;
        if (AM_UNLIKELY((ret = write_video_frame_info(chunk_offset,
                                frame_size, sync_sample)) != AM_STATE_OK)) {
          ERROR("Failed to write video frame info.");
          break;
        }
      }
    } else {
      ERROR("Find nalu error.");
      ret = AM_STATE_ERROR;
      break;
    }
    if (AM_UNLIKELY(m_write_media_data_started == 0)) {
      m_write_media_data_started = 1;
    }
  } while(0);
  return ret;
}

AM_STATE AMTimeElapseMp4Builder::write_video_data(AMPacket *packet)
{
  AM_STATE ret = AM_STATE_OK;
  m_have_B_frame = (packet->get_frame_type() == AM_VIDEO_FRAME_TYPE_B) ?
      true : m_have_B_frame;
  switch (m_video_info.type) {
    case AM_VIDEO_H264 : {
      ret = write_h264_video_data(packet);
    } break;
    case AM_VIDEO_H265 : {
      ret = write_h265_video_data(packet);
    } break;
    default : {
      ERROR("The video type %u is not supported currently.", m_video_info.type);
      ret = AM_STATE_ERROR;
    } break;
  }
  return ret;
}

AM_STATE AMTimeElapseMp4Builder::write_SEI_data(AMPacket *usr_sei)
{
  AM_STATE ret = AM_STATE_OK;
  do {
    if(m_sei_queue) {
      usr_sei->add_ref();
      m_sei_queue->push_back(usr_sei);
    } else {
      ERROR("Can not write SEI data.");
      ret = AM_STATE_ERROR;
      break;
    }
  } while(0);
  return ret;
}

void AMTimeElapseMp4Builder::destroy()
{
  delete this;
}

AM_MUXER_CODEC_TYPE AMTimeElapseMp4Builder::get_muxer_type()
{
  return AM_MUXER_CODEC_MP4;
}

void AMTimeElapseMp4Builder::clear_video_data()
{
  while (!m_sei_queue->empty()) {
    m_sei_queue->front()->release();
    m_sei_queue->pop_front();
  }
  while(!m_h265_frame_queue->empty()) {
    m_h265_frame_queue->front()->release();
    m_h265_frame_queue->pop_front();
  }
  m_h264_nalu_list.clear();
  m_h265_nalu_list.clear();
  m_h265_nalu_frame_queue.clear();
}

void AMTimeElapseMp4Builder::default_mp4_file_setting()
{
  m_used_version = 0;
  m_flags = 0;

  m_rate = 0x00010000;//fixed point 16.16, 1.0(0x00010000)is normal forward playback.
  m_volume = 0x0100;//fixed point 8.8, 1.0(0x0100) is full volume.

  m_matrix[1] = m_matrix[2] = m_matrix[3] = m_matrix[5] = m_matrix[6] =
      m_matrix[7] = 0;
  m_matrix[0] = m_matrix[4] = 0x00010000;
  m_matrix[8] = 0x40000000; //specified in spec
}

bool AMTimeElapseMp4Builder::find_h264_nalu(uint8_t *data, uint32_t len,
                                       H264_NALU_TYPE expect)
{
  bool ret = false;
  m_h264_nalu_list.clear();
  if (AM_LIKELY(data && (len > 4))) {
    int32_t index = -1;
    uint8_t *bs = data;
    uint8_t *last_header = bs + len - 4;

    while (bs <= last_header) {
      if (AM_UNLIKELY(0x00000001
          == ((bs[0] << 24) | (bs[1] << 16) | (bs[2] << 8) | bs[3]))) {
        AM_H264_NALU nalu;
        ++ index;
        bs += 4;
        nalu.type = H264_NALU_TYPE((int32_t) (0x1F & bs[0]));
        nalu.addr = bs;
        m_h264_nalu_list.push_back(nalu);
        if (AM_LIKELY(index > 0)) {
          AM_H264_NALU &prev_nalu = m_h264_nalu_list[index - 1];
          AM_H264_NALU &curr_nalu = m_h264_nalu_list[index];
          /* calculate the previous NALU's size */
          prev_nalu.size = curr_nalu.addr - prev_nalu.addr - 4;
        }
        if (AM_LIKELY(nalu.type == expect)) {
          /* The last NALU has found */
          break;
        }
      } else if (bs[3] != 0) {
        bs += 4;
      } else if (bs[2] != 0) {
        bs += 3;
      } else if (bs[1] != 0) {
        bs += 2;
      } else {
        bs += 1;
      }
    }
    if (AM_LIKELY(index >= 0)) {
      /* calculate the last NALU's size */
      AM_H264_NALU &last_nalu = m_h264_nalu_list[index];
      last_nalu.size = data + len - last_nalu.addr;
      ret = true;
    }
  } else {
    if (AM_LIKELY(!data)) {
      ERROR("Invalid bit stream!");
    }
    if (AM_LIKELY(len <= 4)) {
      ERROR("Bit stream is less equal than 4 bytes!");
    }
  }
  return ret;
}

#define DELIMITER_SIZE 3
bool AMTimeElapseMp4Builder::find_h265_nalu(uint8_t *data, uint32_t len)
{
  bool ret = true;
  m_h265_nalu_list.clear();
  do {
    if (AM_LIKELY(data && (len > DELIMITER_SIZE))) {
      int32_t index = -1;
      uint8_t *bs = data;
      uint8_t *last_header = bs + len - DELIMITER_SIZE;

      while ((bs <= last_header) && ret) {
        if (AM_UNLIKELY(0x00000001 ==
            (0 | (bs[0] << 16) | (bs[1] << 8) | bs[2]))) {
          AM_H265_NALU nalu;
          ++ index;
          bs += DELIMITER_SIZE;
          nalu.type = (int32_t)((0x7E & bs[0]) >> 1);
          nalu.addr = bs;
          m_h265_nalu_list.push_back(nalu);
          if (AM_LIKELY(index > 0)) {
            AM_H265_NALU &prev_nalu = m_h265_nalu_list[index - 1];
            AM_H265_NALU &curr_nalu = m_h265_nalu_list[index];
            /* calculate the previous NALU's size */
            prev_nalu.size = curr_nalu.addr - prev_nalu.addr - DELIMITER_SIZE;
          }
        } else if (bs[2] != 0) {
          bs += 3;
        } else if (bs[1] != 0) {
          bs += 2;
        } else {
          bs += 1;
        }
      }
      if(!ret) {
        break;
      }
      if (AM_LIKELY(index >= 0)) {
        /* calculate the last NALU's size */
        AM_H265_NALU &last_nalu = m_h265_nalu_list[index];
        last_nalu.size = data + len - last_nalu.addr;
      }
    } else {
      if (AM_LIKELY(!data)) {
        ERROR("Invalid bit stream!");
        ret = false;
        break;
      }
      if (AM_LIKELY(len <= DELIMITER_SIZE)) {
        ERROR("Bit stream is less equal than %d bytes!", DELIMITER_SIZE);
        ret = false;
        break;
      }
    }
  } while(0);
  return ret;
}

bool AMTimeElapseMp4Builder::parse_h264_sps(uint8_t *sps, uint32_t size)
{
  bool ret = true;
  AMIVideoParamSetsParser* video_param_sets_parser =
      AMIVideoParamSetsParser::create();
  do {
    if(AM_UNLIKELY(!video_param_sets_parser)) {
      ERROR("Failed to create video param sets parser.");
      ret = false;
      break;
    }
    uint8_t out[size];
    uint32_t outsize = 0;
    memset(out, 0, size);
    filter_emulation_prevention_three_byte(sps, out, size, outsize);
    if(AM_UNLIKELY(!video_param_sets_parser->parse_avc_sps(out, outsize,
                                                        m_avc_sps_struct))) {
      ERROR("Failed to parse avc sps.");
      ret = false;
      break;
    }
  } while(0);
  delete video_param_sets_parser;
  return ret;
}

bool AMTimeElapseMp4Builder::parse_h264_pps(uint8_t *pps, uint32_t size)
{
  bool ret = true;
  AMIVideoParamSetsParser* video_param_sets_parser =
      AMIVideoParamSetsParser::create();
  do {
    uint8_t out[size];
    uint32_t outsize = 0;
    memset(out, 0, size);
    filter_emulation_prevention_three_byte(pps, out, size, outsize);
    if(AM_UNLIKELY(!video_param_sets_parser)) {
      ERROR("Failed to create video param sets parser.");
      ret = false;
      break;
    }
    if(AM_UNLIKELY(!video_param_sets_parser->parse_avc_pps(
        out, outsize, m_avc_pps_struct, m_avc_sps_struct))) {
      ERROR("Failed to parse avc pps.");
      ret = false;
      break;
    }
  } while(0);
  delete video_param_sets_parser;

  return ret;
}

void AMTimeElapseMp4Builder::filter_emulation_prevention_three_byte(
    uint8_t *input, uint8_t *output, uint32_t insize, uint32_t& outsize)
{
  enum {
    PARSE_STAGE_0, /* Normal byte */
    PARSE_STAGE_1, /* 1st 0 */
    PARSE_STAGE_2, /* 2nd 0 */
  } parse_stage = PARSE_STAGE_0;
  uint32_t count = 0;
  uint32_t index = 0;

  while(index < insize) {
    switch(parse_stage) {
      case PARSE_STAGE_0: {
        output[count] = input[index];
        parse_stage = (input[index] == 0) ? PARSE_STAGE_1 : PARSE_STAGE_0;
        ++ count;
        ++ index;
      }break;
      case PARSE_STAGE_1: {
        output[count] = input[index];
        parse_stage = (input[index] == 0) ? PARSE_STAGE_2 : PARSE_STAGE_0;
        ++ count;
        ++ index;
      }break;
      case PARSE_STAGE_2: {
        if (input[index] != 3) {
          output[count] = input[index];
          ++ count;
        }
        ++ index;
        parse_stage = PARSE_STAGE_0;
      }break;
    }
  }
  outsize = count;
}

bool AMTimeElapseMp4Builder::parse_h265_vps(uint8_t *vps, uint32_t size)
{
  bool ret = true;
  AMIVideoParamSetsParser* video_param_sets_parser =
          AMIVideoParamSetsParser::create();
  do {
    uint8_t out[size];
    uint32_t outsize = 0;
    memset(out, 0, size);
    filter_emulation_prevention_three_byte(vps, out, size, outsize);
    if(AM_UNLIKELY(!video_param_sets_parser)) {
      ERROR("Failed to create video param sets parser.");
      ret = false;
      break;
    }
    if(AM_UNLIKELY(!video_param_sets_parser->parse_hevc_vps(out, outsize,
                                                     m_hevc_vps_struct))) {
      ERROR("Failed to parse hevc vps.");
      ret = false;
      break;
    }
  } while(0);
  delete video_param_sets_parser;

  return ret;
}

bool AMTimeElapseMp4Builder::parse_h265_sps(uint8_t *sps, uint32_t size)
{
  bool ret = true;
  AMIVideoParamSetsParser* video_param_sets_parser =
      AMIVideoParamSetsParser::create();
  do {
    uint8_t out[size];
    uint32_t outsize = 0;
    memset(out, 0, size);
    filter_emulation_prevention_three_byte(sps, out, size, outsize);
    if(AM_UNLIKELY(!video_param_sets_parser)) {
      ERROR("Failed to create video param sets parser.");
      ret = false;
      break;
    }
    if(AM_UNLIKELY(!video_param_sets_parser->parse_hevc_sps(
        out, outsize, m_hevc_sps_struct))) {
      ERROR("Failed to parse hevc sps.");
      ret = false;
      break;
    }
  } while(0);
  delete video_param_sets_parser;

  return ret;
}

bool AMTimeElapseMp4Builder::parse_h265_pps(uint8_t *pps, uint32_t size)
{
  bool ret = true;
  AMIVideoParamSetsParser* video_param_sets_parser =
      AMIVideoParamSetsParser::create();
  do {
    uint8_t out[size];
    uint32_t outsize = 0;
    memset(out, 0, size);
    filter_emulation_prevention_three_byte(pps, out, size, outsize);
    if(AM_UNLIKELY(!video_param_sets_parser)) {
      ERROR("Failed to create video param sets parser.");
      ret = false;
      break;
    }
    if(AM_UNLIKELY(!video_param_sets_parser->parse_hevc_pps(out, outsize,
                                      m_hevc_pps_struct, m_hevc_sps_struct))) {
      ERROR("Failed to parse hevc pps.");
      ret = false;
      break;
    }
  } while(0);
  delete video_param_sets_parser;

  return ret;
}

void AMTimeElapseMp4Builder::fill_file_type_box()
{
  FileTypeBox& box = m_file_type_box;
  box.m_base_box.m_enable = true;
  box.m_base_box.m_type = DISOM_FILE_TYPE_BOX_TAG;
  box.m_major_brand = DISOM_BRAND_V0_TAG;
  box.m_minor_version = 0;
  box.m_compatible_brands[0] = DISOM_BRAND_V0_TAG;
  box.m_compatible_brands[1] = DISOM_BRAND_V1_TAG;
  box.m_compatible_brands[2] = DISOM_BRAND_AVC_TAG;
  box.m_compatible_brands[3] = DISOM_BRAND_MPEG4_TAG;
  box.m_compatible_brands_number = 4;
  box.m_base_box.m_size = DISOM_BASE_BOX_SIZE +
                          sizeof(m_file_type_box.m_major_brand) +
                          sizeof(m_file_type_box.m_minor_version) +
                (m_file_type_box.m_compatible_brands_number * DISOM_TAG_SIZE);
}

void AMTimeElapseMp4Builder::fill_free_box(uint32_t size)
{
  m_free_box.m_base_box.m_type = DISOM_FREE_BOX_TAG;
  m_free_box.m_base_box.m_size = size;
  m_free_box.m_base_box.m_enable = true;
}

void AMTimeElapseMp4Builder::fill_media_data_box()
{
  MediaDataBox& box = m_media_data_box;
  box.m_base_box.m_enable = true;
  box.m_base_box.m_type = DISOM_MEDIA_DATA_BOX_TAG;
  box.m_base_box.m_size = DISOM_BASE_BOX_SIZE;
}

void AMTimeElapseMp4Builder::fill_movie_header_box()
{
  MovieHeaderBox& box = m_movie_box.m_movie_header_box;
  box.m_full_box.m_base_box.m_enable = true;
  box.m_full_box.m_base_box.m_type = DISOM_MOVIE_HEADER_BOX_TAG;
  box.m_full_box.m_version = m_used_version;
  box.m_full_box.m_flags = m_flags;
  box.m_creation_time = m_creation_time;
  box.m_modification_time = m_modification_time;
  box.m_timescale = AM_PTS_DEFINE;
  box.m_duration = m_video_duration;
  box.m_rate = m_rate;
  box.m_volume = m_volume;
  for (uint32_t i = 0; i < 9; i ++) {
    box.m_matrix[i] = m_matrix[i];
  }
  box.m_next_track_ID = m_next_track_id;
  if (!m_used_version) {
    box.m_full_box.m_base_box.m_size = DISOM_MOVIE_HEADER_SIZE;
  } else {
    box.m_full_box.m_base_box.m_size = DISOM_MOVIE_HEADER_64_SIZE;
  }
}

void AMTimeElapseMp4Builder::fill_object_desc_box()
{
  ObjectDescBox& box = m_movie_box.m_iods_box;
  box.m_full_box.m_base_box.m_enable = true;
  box.m_full_box.m_base_box.m_size = 24;
  box.m_full_box.m_base_box.m_type = DIDOM_OBJECT_DESC_BOX_TAG;
  box.m_iod_type_tag = 0x10;
  box.m_extended_desc_type[0] = box.m_extended_desc_type[1] =
      box.m_extended_desc_type[2] = 0x80;
  box.m_desc_type_length = 7;
  box.m_od_id[0] = 0;
  box.m_od_id[1] = 0x4f;
  box.m_od_profile_level = 0xff;
  box.m_scene_profile_level = 0xff;
  box.m_audio_profile_level = 0x02;
  box.m_video_profile_level = 0x01;
  box.m_graphics_profile_level = 0xff;
}

void AMTimeElapseMp4Builder::fill_video_track_header_box()
{
  TrackHeaderBox& box = m_movie_box.m_video_track.m_track_header;
  box.m_full_box.m_base_box.m_enable = true;
  box.m_full_box.m_base_box.m_type = DISOM_TRACK_HEADER_BOX_TAG;
  box.m_full_box.m_version = m_used_version;
  box.m_full_box.m_flags = 0x01;
  box.m_creation_time = m_creation_time;
  box.m_modification_time = m_modification_time;
  box.m_track_ID = m_video_track_id;
  box.m_duration = m_video_duration;
  for (uint32_t i = 0; i < 9; i ++) {
    box.m_matrix[i] = m_matrix[i];
  }
  box.m_width_integer = m_video_info.width;
  box.m_width_decimal = 0;
  box.m_height_integer = m_video_info.height;
  box.m_height_decimal = 0;

  if (!m_used_version) {
    box.m_full_box.m_base_box.m_size = DISOM_TRACK_HEADER_SIZE;
  } else {
    box.m_full_box.m_base_box.m_size = DISOM_TRACK_HEADER_64_SIZE;
  }
}

void AMTimeElapseMp4Builder::fill_media_header_box_for_video_track()
{
  MediaHeaderBox& box = m_movie_box.m_video_track.m_media.m_media_header;
  box.m_full_box.m_base_box.m_enable = true;
  box.m_full_box.m_base_box.m_type = DISOM_MEDIA_HEADER_BOX_TAG;
  box.m_full_box.m_version = m_used_version;
  box.m_full_box.m_flags = m_flags;
  box.m_creation_time = m_creation_time;
  box.m_modification_time = m_modification_time;
  box.m_timescale = AM_PTS_DEFINE;//the number of time units that pass in one second.
  box.m_duration = m_video_duration;
  box.m_language[0] = 21;
  box.m_language[1] = 14;
  box.m_language[2] = 4;
  if (!m_used_version) {
    box.m_full_box.m_base_box.m_size = DISOM_MEDIA_HEADER_SIZE;
  } else {
    box.m_full_box.m_base_box.m_size = DISOM_MEDIA_HEADER_64_SIZE;
  }
}

AM_STATE AMTimeElapseMp4Builder::fill_video_handler_reference_box()
{
  AM_STATE ret = AM_STATE_OK;
  do {
    HandlerReferenceBox& box =
        m_movie_box.m_video_track.m_media.m_media_handler;
    box.m_full_box.m_base_box.m_enable = true;
    box.m_full_box.m_base_box.m_type = DISOM_HANDLER_REFERENCE_BOX_TAG;
    box.m_full_box.m_version = m_used_version;
    box.m_full_box.m_flags = m_flags;
    box.m_handler_type = VIDEO_TRACK;
    switch (m_video_info.type) {
      case AM_VIDEO_H264 : {
        if (!box.m_name) {
          box.m_name_size = strlen(DVIDEO_H264_COMPRESSORNAME);
          box.m_name = new char[box.m_name_size];
          if (!box.m_name) {
            ERROR("Failed to malloc memory in fill video hander reference box.");
            ret = AM_STATE_ERROR;
            break;
          } else {
            memset(box.m_name, 0, box.m_name_size);
          }
          memcpy(box.m_name, DVIDEO_H264_COMPRESSORNAME, box.m_name_size);
        }
      } break;
      case AM_VIDEO_H265 : {
        if (!box.m_name) {
          box.m_name_size = strlen(DVIDEO_H265_COMPRESSORNAME);
          box.m_name = new char[box.m_name_size];
          if (!box.m_name) {
            ERROR("Failed to malloc memory in fill video hander reference box.");
            ret = AM_STATE_ERROR;
            break;
          } else {
            memset(box.m_name, 0, box.m_name_size);
          }
          memcpy(box.m_name, DVIDEO_H265_COMPRESSORNAME, box.m_name_size);
        }
      }break;
      default : {
        ERROR("The video type %u is not supported currently.");
        ret = AM_STATE_ERROR;
      }break;
    }
    if(ret != AM_STATE_OK) {
      break;
    }
    box.m_full_box.m_base_box.m_size = DISOM_HANDLER_REF_SIZE + box.m_name_size;
  } while(0);
  return ret;
}

void AMTimeElapseMp4Builder::fill_video_chunk_offset32_box()
{
  ChunkOffsetBox& box = m_movie_box.m_video_track.m_media.\
      m_media_info.m_sample_table.m_chunk_offset;
  box.m_full_box.m_base_box.m_enable = true;
  box.m_full_box.m_base_box.m_type = DISOM_CHUNK_OFFSET_BOX_TAG;
  box.m_full_box.m_version = m_used_version;
  box.m_full_box.m_flags = m_flags;
}

void AMTimeElapseMp4Builder::fill_video_chunk_offset64_box()
{
  ChunkOffset64Box& box = m_movie_box.m_video_track.m_media.\
      m_media_info.m_sample_table.m_chunk_offset64;
  box.m_full_box.m_base_box.m_enable = true;
  box.m_full_box.m_base_box.m_type = DISOM_CHUNK_OFFSET64_BOX_TAG;
  box.m_full_box.m_version = m_used_version;
  box.m_full_box.m_flags = m_flags;
}

void AMTimeElapseMp4Builder::fill_video_sync_sample_box()
{
  SyncSampleTableBox& box = m_movie_box.m_video_track.\
      m_media.m_media_info.m_sample_table.m_sync_sample;
  box.m_full_box.m_base_box.m_enable = true;
  box.m_full_box.m_base_box.m_type = DISOM_SYNC_SAMPLE_TABLE_BOX_TAG;
  box.m_full_box.m_flags = m_flags;
  box.m_full_box.m_version = m_used_version;
}

AM_STATE AMTimeElapseMp4Builder::fill_video_sample_to_chunk_box()
{
  AM_STATE ret = AM_STATE_OK;
  SampleToChunkBox& box = m_movie_box.m_video_track.m_media.\
      m_media_info.m_sample_table.m_sample_to_chunk;
  box.m_full_box.m_base_box.m_enable = true;
  do {
    box.m_full_box.m_base_box.m_type = DISOM_SAMPLE_TO_CHUNK_BOX_TAG;
    box.m_full_box.m_version = m_used_version;
    box.m_full_box.m_flags = m_flags;
    box.m_entry_count = 1;
    if (!box.m_entrys) {
      box.m_entrys = new SampleToChunkEntry[box.m_entry_count];
      if (AM_UNLIKELY(!box.m_entrys)) {
        ERROR("fill_sample_to_chunk_box: no memory\n");
        ret = AM_STATE_ERROR;
        break;
      }
    }
    box.m_entrys[0].m_first_chunk = 1;
    box.m_entrys[0].m_sample_per_chunk = 1;
    box.m_entrys[0].m_sample_description_index = 1;
  } while (0);
  return ret;
}

void AMTimeElapseMp4Builder::fill_video_sample_size_box()
{
  SampleSizeBox& box = m_movie_box.m_video_track.m_media.\
      m_media_info.m_sample_table.m_sample_size;
  box.m_full_box.m_base_box.m_enable = true;
  box.m_full_box.m_base_box.m_type = DISOM_SAMPLE_SIZE_BOX_TAG;
  box.m_full_box.m_version = m_used_version;
  box.m_full_box.m_flags = m_flags;
}

AM_STATE AMTimeElapseMp4Builder::fill_avc_decoder_configuration_record_box()
{
  AM_STATE ret = AM_STATE_OK;
  do {
    AVCDecoderConfigurationRecord& box =
        m_movie_box.m_video_track.m_media.m_media_info.
        m_sample_table.m_sample_description.m_visual_entry.m_avc_config;
    box.m_base_box.m_enable = true;
    box.m_base_box.m_type = DISOM_AVC_DECODER_CONFIGURATION_RECORD_TAG;
    box.m_config_version = 1;
    box.m_profile_indication = m_avc_sps_struct.profile_idc;
    box.m_profile_compatibility = 0;
    box.m_level_indication = m_avc_sps_struct.level_idc;
    box.m_reserved = 0xff;
    box.m_len_size_minus_one = 3;
    box.m_reserved_1 = 0xff;
    box.m_sps_num = 1;
    box.m_sps_len = m_avc_sps_length;
    if(box.m_sps == nullptr) {
      box.m_sps = new uint8_t[m_avc_sps_length];
      if(!box.m_sps) {
        ERROR("Failed to malloc memory when fill avc_decoder configuration "
            "record box");
        ret = AM_STATE_ERROR;
        break;
      }
      if(m_avc_sps == nullptr) {
        ERROR("m_avc_sps is nullptr");
        ret = AM_STATE_ERROR;
        break;
      }
      memcpy(box.m_sps, m_avc_sps, m_avc_sps_length);
    }
    box.m_pps_num = 1;
    box.m_pps_len = m_avc_pps_length;
    if(box.m_pps == nullptr) {
      box.m_pps = new uint8_t[m_avc_pps_length];
      if(!box.m_pps) {
        ERROR("Failed to malloc memory when fill avc decoder configuration "
            "record box.");
        ret = AM_STATE_ERROR;
        break;
      }
      if(m_avc_pps == nullptr) {
        ERROR("m_avc_pps is nullptr");
        ret = AM_STATE_ERROR;
        break;
      }
      memcpy(box.m_pps, m_avc_pps, m_avc_pps_length);
    }
    box.m_base_box.m_size = DISOM_AVC_DECODER_CONFIG_RECORD_SIZE +
        box.m_sps_len + box.m_pps_len;
  } while(0);
  return ret;
}

AM_STATE AMTimeElapseMp4Builder::fill_hevc_decoder_configuration_record_box()
{
  AM_STATE ret = AM_STATE_OK;
  do {
    HEVCDecoderConfigurationRecord& box = m_movie_box.m_video_track.\
        m_media.m_media_info.m_sample_table.m_sample_description.\
        m_visual_entry.m_HEVC_config;
    box.m_base_box.m_enable = true;
    box.m_base_box.m_type = DISOM_HEVC_DECODER_CONFIGURATION_RECORD_TAG;
    box.m_configuration_version = 1;
    box.m_general_profile_space = m_hevc_vps_struct.ptl.general_profile_space;
    box.m_general_tier_flag = m_hevc_vps_struct.ptl.general_tier_flag;
    box.m_general_profile_idc = m_hevc_vps_struct.ptl.general_profile_idc;
    for(uint32_t i = 0; i < 32; ++ i) {
      box.m_general_profile_compatibility_flags |=
       m_hevc_vps_struct.ptl.general_profile_compatibility_flag[i] << (31 - i);
    }
    box.m_general_constraint_indicator_flags_high =
   (((uint16_t)m_hevc_vps_struct.ptl.general_progressive_source_flag) << 15) |
   (((uint16_t)m_hevc_vps_struct.ptl.general_interlaced_source_flag) << 14) |
   (((uint16_t)m_hevc_vps_struct.ptl.general_non_packed_constraint_flag) << 13) |
   (((uint16_t)m_hevc_vps_struct.ptl.general_frame_only_constraint_flag) << 12);
    box.m_general_constraint_indicator_flags_low = 0;
    box.m_general_level_idc = m_hevc_vps_struct.ptl.general_level_idc;
    box.m_min_spatial_segmentation_idc =
        (uint16_t)m_hevc_sps_struct.vui.min_spatial_segmentation_idc;
    box.m_parallelism_type = 0;
    box.m_chroma_format = (uint8_t)m_hevc_sps_struct.chroma_format_idc;
    box.m_bit_depth_luma_minus8 = m_hevc_sps_struct.pcm.bit_depth_chroma - 8;
    box.m_bit_depth_chroma_minus8 = m_hevc_sps_struct.pcm.bit_depth_chroma - 8;
    box.m_avg_frame_rate = 0;
    box.m_constant_frame_rate = 0;
    box.m_num_temporal_layers = m_hevc_sps_struct.num_temporal_layers;
    box.m_temporal_id_nested = m_hevc_vps_struct.vps_temporal_id_nesting_flag;
    box.m_length_size_minus_one = 3;
    box.m_num_of_arrays = 3;//vps,sps, pps.if have SEI, this value should be 4.
    if (!box.m_HEVC_config_array) {
      box.m_HEVC_config_array = new HEVCConArray[3];
      if (AM_UNLIKELY(!box.m_HEVC_config_array)) {
        ERROR("Failed to malloc memory when fill hevc decoder configuration "
              "record box.");
        ret = AM_STATE_ERROR;
        break;
      }
    }
    box.m_HEVC_config_array[0].m_array_completeness = 1;
    box.m_HEVC_config_array[0].m_reserved = 0;
    box.m_HEVC_config_array[0].m_NAL_unit_type = H265_VPS;
    box.m_HEVC_config_array[0].m_num_nalus = 1;
    if (!box.m_HEVC_config_array[0].m_array_units) {
      box.m_HEVC_config_array[0].m_array_units = new HEVCConArrayNal[1];
      if (!box.m_HEVC_config_array[0].m_array_units) {
        ERROR("Failed to malloc memory when "
              "fill_hevc_decoder_configuration_record_box");
        ret = AM_STATE_ERROR;
        break;
      }
      box.m_HEVC_config_array[0].m_array_units[0].m_nal_unit_length =
          m_hevc_vps_length;
      delete[] box.m_HEVC_config_array[0].m_array_units[0].m_nal_unit;
      box.m_HEVC_config_array[0].m_array_units[0].m_nal_unit =
          new uint8_t[m_hevc_vps_length];
      if (AM_UNLIKELY(!box.m_HEVC_config_array[0].m_array_units[0].m_nal_unit)) {
        ERROR("Failed to malloc memory for vps");
        ret = AM_STATE_ERROR;
        break;
      }
      memcpy(box.m_HEVC_config_array[0].m_array_units[0].m_nal_unit,
             m_hevc_vps, m_hevc_vps_length);
    }
    box.m_HEVC_config_array[1].m_array_completeness = 1;
    box.m_HEVC_config_array[1].m_reserved = 0;
    box.m_HEVC_config_array[1].m_NAL_unit_type = H265_SPS;
    box.m_HEVC_config_array[1].m_num_nalus = 1;
    if (!box.m_HEVC_config_array[1].m_array_units) {
      box.m_HEVC_config_array[1].m_array_units = new HEVCConArrayNal[1];
      if (!box.m_HEVC_config_array[1].m_array_units) {
        ERROR("Failed to malloc memory when "
              "fill_hevc_decoder_configuration_record_box");
        ret = AM_STATE_ERROR;
        break;
      }
      box.m_HEVC_config_array[1].m_array_units[0].m_nal_unit_length =
          m_hevc_sps_length;
      delete[] box.m_HEVC_config_array[1].m_array_units[0].m_nal_unit;
      box.m_HEVC_config_array[1].m_array_units[0].m_nal_unit =
          new uint8_t[m_hevc_sps_length];
      if (AM_UNLIKELY(!box.m_HEVC_config_array[1].m_array_units[0].m_nal_unit)) {
        ERROR("Failed to malloc memory for sps");
        ret = AM_STATE_ERROR;
        break;
      }
      memcpy(box.m_HEVC_config_array[1].m_array_units[0].m_nal_unit,
             m_hevc_sps, m_hevc_sps_length);
    }
    box.m_HEVC_config_array[2].m_array_completeness = 1;
    box.m_HEVC_config_array[2].m_reserved = 0;
    box.m_HEVC_config_array[2].m_NAL_unit_type = H265_PPS;
    box.m_HEVC_config_array[2].m_num_nalus = 1;
    if (!box.m_HEVC_config_array[2].m_array_units) {
      box.m_HEVC_config_array[2].m_array_units = new HEVCConArrayNal[1];
      if (!box.m_HEVC_config_array[2].m_array_units) {
        ERROR("Failed to malloc memory when "
              "fill_hevc_decoder_configuration_record_box");
        ret = AM_STATE_ERROR;
        break;
      }
      box.m_HEVC_config_array[2].m_array_units[0].m_nal_unit_length =
          m_hevc_pps_length;
      delete[] box.m_HEVC_config_array[2].m_array_units[0].m_nal_unit;
      box.m_HEVC_config_array[2].m_array_units[0].m_nal_unit =
          new uint8_t[m_hevc_pps_length];
      if (AM_UNLIKELY(!box.m_HEVC_config_array[2].m_array_units[0].m_nal_unit)) {
        ERROR("Failed to malloc memory for pps");
        ret = AM_STATE_ERROR;
        break;
      }
      memcpy(box.m_HEVC_config_array[2].m_array_units[0].m_nal_unit,
             m_hevc_pps, m_hevc_pps_length);
    }
  } while (0);
  return ret;
}

AM_STATE AMTimeElapseMp4Builder::fill_visual_sample_description_box()
{
  AM_STATE ret = AM_STATE_OK;
  do {
    SampleDescriptionBox& box = m_movie_box.m_video_track.\
        m_media.m_media_info.m_sample_table.m_sample_description;
    box.m_full_box.m_base_box.m_enable = true;
    box.m_full_box.m_base_box.m_type = DISOM_SAMPLE_DESCRIPTION_BOX_TAG;
    box.m_full_box.m_version = m_used_version;
    box.m_full_box.m_flags = m_flags;
    box.m_entry_count = 1;
    switch (m_video_info.type) {
      case AM_VIDEO_H264 : {
        box.m_visual_entry.m_base_box.m_type = DISOM_H264_ENTRY_TAG;
        box.m_visual_entry.m_base_box.m_enable = true;
        memset(box.m_visual_entry.m_compressorname, 0,
               sizeof(box.m_visual_entry.m_compressorname));
        strncpy(box.m_visual_entry.m_compressorname,
                DVIDEO_H264_COMPRESSORNAME,
                (sizeof(box.m_visual_entry.m_compressorname) - 1));
        if(AM_UNLIKELY(fill_avc_decoder_configuration_record_box()
                       != AM_STATE_OK)) {
          ERROR("Failed to fill avc decoder configuration record box.");
          ret = AM_STATE_ERROR;
          break;
        }
      } break;
      case AM_VIDEO_H265 : {
        box.m_visual_entry.m_base_box.m_type = DISOM_H265_ENTRY_TAG;
        box.m_visual_entry.m_base_box.m_enable = true;
        memset(box.m_visual_entry.m_compressorname, 0,
               sizeof(box.m_visual_entry.m_compressorname));
        strncpy(box.m_visual_entry.m_compressorname,
                DVIDEO_H265_COMPRESSORNAME,
                (sizeof(box.m_visual_entry.m_compressorname) - 1));
        if(AM_UNLIKELY(fill_hevc_decoder_configuration_record_box() != AM_STATE_OK)) {
          ERROR("Failed to fill hevc decoder configuration record box.");
          ret = AM_STATE_ERROR;
          break;
        }
      } break;
      default : {
        ERROR("The video type %u is not supported currently.");
        ret = AM_STATE_ERROR;
        break;
      }
    }
    if(ret != AM_STATE_OK) {
      break;
    }
    box.m_visual_entry.m_data_reference_index = 1;
    box.m_visual_entry.m_width = m_video_info.width;
    box.m_visual_entry.m_height = m_video_info.height;
    box.m_visual_entry.m_horizresolution = 0x00480000;
    box.m_visual_entry.m_vertresolution = 0x00480000;
    box.m_visual_entry.m_frame_count = 1;
    box.m_visual_entry.m_depth = 0x0018;
    box.m_visual_entry.m_pre_defined_2 = -1;
    box.m_full_box.m_base_box.m_size = DISOM_FULL_BOX_SIZE +
        sizeof(uint32_t) + box.m_visual_entry.m_base_box.m_size;
  } while(0);
  return ret;
}

AM_STATE AMTimeElapseMp4Builder::fill_video_decoding_time_to_sample_box()
{
  AM_STATE ret = AM_STATE_OK;
  DecodingTimeToSampleBox& box =
      m_movie_box.m_video_track.m_media.m_media_info.m_sample_table.m_stts;
  do {
    box.m_full_box.m_base_box.m_enable = true;
    box.m_full_box.m_base_box.m_type =
    DISOM_DECODING_TIME_TO_SAMPLE_TABLE_BOX_TAG;
    box.m_full_box.m_flags = m_flags;
    box.m_full_box.m_version = m_used_version;
  } while (0);
  return ret;
}

AM_STATE AMTimeElapseMp4Builder::fill_video_composition_time_to_sample_box()
{
  AM_STATE ret = AM_STATE_OK;
  CompositionTimeToSampleBox& box =
      m_movie_box.m_video_track.m_media.m_media_info.m_sample_table.m_ctts;
  do{
    box.m_full_box.m_base_box.m_enable = true;
    box.m_full_box.m_base_box.m_type =
           DISOM_COMPOSITION_TIME_TO_SAMPLE_TABLE_BOX_TAG;
    box.m_full_box.m_flags = m_flags;
    box.m_full_box.m_version = m_used_version;
  }while(0);
  return ret;
}

AM_STATE AMTimeElapseMp4Builder::fill_video_sample_table_box()
{
  AM_STATE ret = AM_STATE_OK;
  SampleTableBox& box =
      m_movie_box.m_video_track.m_media.m_media_info.m_sample_table;
  do {
    box.m_base_box.m_enable = true;
    box.m_base_box.m_type = DISOM_SAMPLE_TABLE_BOX_TAG;
    if (fill_visual_sample_description_box() != AM_STATE_OK) {
      ERROR("Failed to fill visual sample description box.");
      ret = AM_STATE_ERROR;
      break;
    }
    if (AM_UNLIKELY(fill_video_decoding_time_to_sample_box() != AM_STATE_OK)) {
      ERROR("Failed to fill video decoding time to sample box.");
      ret = AM_STATE_ERROR;
      break;
    }
    if (m_have_B_frame) {
      if (AM_UNLIKELY(fill_video_composition_time_to_sample_box()
          != AM_STATE_OK)) {
        ERROR("Failed to fill video composition time to sample box.");
        ret = AM_STATE_ERROR;
        break;
      }
    }
    fill_video_sample_size_box();
    if (AM_UNLIKELY(fill_video_sample_to_chunk_box() != AM_STATE_OK)) {
      ERROR("Failed to fill sample to chunk box.");
      ret = AM_STATE_ERROR;
      break;
    }
    if (CHUNK_OFFSET64_ENABLE) {
      fill_video_chunk_offset64_box();
    } else {
      fill_video_chunk_offset32_box();
    }
    fill_video_sync_sample_box();
  } while (0);
  return ret;
}

void AMTimeElapseMp4Builder::fill_video_data_reference_box()
{
  DataReferenceBox& box = m_movie_box.m_video_track.m_media.\
      m_media_info.m_data_info.m_data_ref;
  box.m_full_box.m_base_box.m_enable = true;
  box.m_full_box.m_base_box.m_type = DISOM_DATA_REFERENCE_BOX_TAG;
  box.m_full_box.m_version = m_used_version;
  box.m_full_box.m_flags = m_flags;
  box.m_entry_count = 1;
  box.m_url.m_full_box.m_base_box.m_type = DISOM_DATA_ENTRY_URL_BOX_TAG;
  box.m_url.m_full_box.m_base_box.m_enable = true;
  box.m_url.m_full_box.m_version = m_used_version;
  box.m_url.m_full_box.m_flags = 1;
  box.m_url.m_full_box.m_base_box.m_size = DISOM_FULL_BOX_SIZE;
  box.m_full_box.m_base_box.m_size = DISOM_FULL_BOX_SIZE +
      box.m_entry_count * box.m_url.m_full_box.m_base_box.m_size
      + sizeof(uint32_t);
}

void AMTimeElapseMp4Builder::fill_video_data_info_box()
{
  DataInformationBox& box = m_movie_box.m_video_track.m_media.\
      m_media_info.m_data_info;
  box.m_base_box.m_enable = true;
  box.m_base_box.m_type = DISOM_DATA_INFORMATION_BOX_TAG;
  fill_video_data_reference_box();
  box.m_base_box.m_size = box.m_data_ref.m_full_box.m_base_box.m_size +
      DISOM_BASE_BOX_SIZE;
}

void AMTimeElapseMp4Builder::fill_video_media_info_header_box()
{
  VideoMediaInfoHeaderBox& box =
      m_movie_box.m_video_track.m_media.m_media_info.m_video_info_header;
  box.m_full_box.m_base_box.m_enable = true;
  box.m_full_box.m_base_box.m_type = DISOM_VIDEO_MEDIA_HEADER_BOX_TAG;
  box.m_full_box.m_version = m_used_version;
  box.m_full_box.m_flags = m_flags;
  box.m_graphicsmode = 0;
  box.m_opcolor[0] = box.m_opcolor[1] = box.m_opcolor[2] = 0;
  box.m_full_box.m_base_box.m_size = DISOM_VIDEO_MEDIA_INFO_HEADER_SIZE;
}

AM_STATE AMTimeElapseMp4Builder::fill_video_media_info_box()
{
  AM_STATE ret = AM_STATE_OK;
  MediaInformationBox& box = m_movie_box.m_video_track.m_media.m_media_info;
  do {
    box.m_base_box.m_enable = true;
    box.m_base_box.m_type = DISOM_MEDIA_INFORMATION_BOX_TAG;
    fill_video_media_info_header_box();
    fill_video_data_info_box();
    fill_video_sample_table_box();
  } while (0);
  return ret;
}

AM_STATE AMTimeElapseMp4Builder::fill_video_media_box()
{
  AM_STATE ret = AM_STATE_OK;
  MediaBox& box = m_movie_box.m_video_track.m_media;
  do {
    box.m_base_box.m_enable = true;
    box.m_base_box.m_type = DISOM_MEDIA_BOX_TAG;
    fill_media_header_box_for_video_track();
    if(fill_video_handler_reference_box() != AM_STATE_OK) {
      ERROR("Fill video hander reference box error.");
      ret = AM_STATE_ERROR;
      break;
    }
    if(AM_UNLIKELY(fill_video_media_info_box() != AM_STATE_OK)) {
      ERROR("Failed to fill video media information box.");
      ret = AM_STATE_ERROR;
      break;
    }
  } while (0);
  return ret;
}

AM_STATE AMTimeElapseMp4Builder::fill_video_track_box()
{
  AM_STATE ret = AM_STATE_OK;
  TrackBox& box = m_movie_box.m_video_track;
  do {
    box.m_base_box.m_enable = true;
    box.m_base_box.m_type = DISOM_TRACK_BOX_TAG;
    fill_video_track_header_box();
    if(AM_UNLIKELY(fill_video_media_box() != AM_STATE_OK)) {
      ERROR("Failed to fill video media box.");
      ret = AM_STATE_ERROR;
      break;
    }
  } while (0);
  return ret;
}

AM_STATE AMTimeElapseMp4Builder::fill_movie_box()
{
  AM_STATE ret = AM_STATE_OK;
  MovieBox& box = m_movie_box;
  do {
    box.m_base_box.m_enable = true;
    box.m_base_box.m_type = DISOM_MOVIE_BOX_TAG;
    fill_movie_header_box();
    if (OBJECT_DESC_BOX_ENABLE) {
      fill_object_desc_box();
    }
    if(m_video_frame_number > 0) {
      if (AM_UNLIKELY(fill_video_track_box()
                      != AM_STATE_OK)) {
        ERROR("Failed to fill video track box.");
        ret = AM_STATE_ERROR;
        break;
      }
    } else {
      ERROR("The video frame number is zero, there are no video in this file");
    }
  } while (0);
  return ret;
}

AM_STATE AMTimeElapseMp4Builder::insert_video_composition_time_to_sample_box(int64_t
                                                          delta_pts)
{
  AM_STATE ret = AM_STATE_OK;
  CompositionTimeToSampleBox& box = m_movie_box.m_video_track.\
      m_media.m_media_info.m_sample_table.m_ctts;
  do{
    int32_t ctts = 0;
    if(AM_UNLIKELY(m_video_frame_number == 1)) {
      ctts = 0;
      box.m_entry_count = 0;
    } else {
      ctts = m_last_ctts + (delta_pts - (AM_PTS_DEFINE * m_video_info.rate /
          m_video_info.scale));
      if(AM_UNLIKELY(ctts < 0)) {
        for(int32_t i = m_video_frame_number - 2; i >= 0; --i) {
          box.m_entry_ptr[i].sample_delta =
              box.m_entry_ptr[i].sample_delta - ctts;
        }
        ctts = 0;
      }
    }
    if(m_video_frame_number >= box.m_entry_buf_count) {
      TimeEntry* tmp = new TimeEntry[box.m_entry_buf_count +
                                         DVIDEO_FRAME_COUNT];
      if(AM_UNLIKELY(!tmp)) {
        ERROR("Failed to malloc memory.");
        ret = AM_STATE_ERROR;
        break;
      }
      if (box.m_entry_count > 0) {
        memcpy(tmp, box.m_entry_ptr, box.m_entry_count * sizeof(TimeEntry));
      }
      delete[] box.m_entry_ptr;
      box.m_entry_ptr = tmp;
      box.m_entry_buf_count += DVIDEO_FRAME_COUNT;
    }
    box.m_entry_ptr[box.m_entry_count].sample_count = 1;
    box.m_entry_ptr[box.m_entry_count].sample_delta = ctts;
    ++ box.m_entry_count;
    m_last_ctts = ctts;
  }while(0);
  return ret;
}

AM_STATE AMTimeElapseMp4Builder::insert_video_decoding_time_to_sample_box(uint32_t delta_pts)
{
  AM_STATE ret = AM_STATE_OK;
  DecodingTimeToSampleBox& box = m_movie_box.m_video_track.\
        m_media.m_media_info.m_sample_table.m_stts;
  do{
    if(m_video_frame_number == 1) {
      box.m_entry_count = 0;
    }
    if(m_video_frame_number >= box.m_entry_buf_count) {
      TimeEntry* tmp = new TimeEntry[box.m_entry_buf_count +
                                         DVIDEO_FRAME_COUNT];
      if(AM_UNLIKELY(!tmp)) {
        ERROR("Failed to malloc memory.");
        ret = AM_STATE_ERROR;
        break;
      }
      if (box.m_entry_count > 0) {
        memcpy(tmp, box.m_entry_ptr, box.m_entry_count * sizeof(TimeEntry));
      }
      delete[] box.m_entry_ptr;
      box.m_entry_ptr = tmp;
      box.m_entry_buf_count += DVIDEO_FRAME_COUNT;
    }
    box.m_entry_ptr[box.m_entry_count].sample_count = 1;
    box.m_entry_ptr[box.m_entry_count].sample_delta = delta_pts;
    ++ box.m_entry_count;
  }while(0);
  return ret;
}

AM_STATE AMTimeElapseMp4Builder::insert_video_chunk_offset64_box(uint64_t offset)
{
  AM_STATE ret = AM_STATE_OK;
  ChunkOffset64Box& box = m_movie_box.m_video_track.m_media.m_media_info.\
             m_sample_table.m_chunk_offset64;
  DEBUG("Insert video chunk offset box, offset = %llu", offset);
  if(m_video_frame_number == 1) {
    box.m_entry_count = 0;
  }
  if (box.m_entry_count < box.m_buf_count) {
    box.m_chunk_offset[box.m_entry_count] = offset;
    box.m_entry_count ++;
  } else {
    uint64_t* ptmp = new uint64_t[box.m_buf_count + DVIDEO_FRAME_COUNT];
    if (ptmp) {
      if (box.m_entry_count > 0) {
        memcpy(ptmp, box.m_chunk_offset, box.m_entry_count * sizeof(uint64_t));
      }
      delete[] box.m_chunk_offset;
      box.m_buf_count += DVIDEO_FRAME_COUNT;
      box.m_chunk_offset = ptmp;
      box.m_chunk_offset[box.m_entry_count] = offset;
      box.m_entry_count ++;
    } else {
      ERROR("no memory\n");
      ret = AM_STATE_ERROR;
    }
  }
  return ret;
}

AM_STATE AMTimeElapseMp4Builder::insert_video_chunk_offset32_box(uint64_t offset)
{
  AM_STATE ret = AM_STATE_OK;
  ChunkOffsetBox& box =
      m_movie_box.m_video_track.m_media.m_media_info.\
                            m_sample_table.m_chunk_offset;
  DEBUG("Insert video chunk offset box, offset = %llu", offset);
  if (m_video_frame_number == 1) {
    box.m_entry_count = 0;
  }
  if (box.m_entry_count < box.m_buf_count) {
    box.m_chunk_offset[box.m_entry_count] = offset;
    box.m_entry_count ++;
  } else {
    uint32_t* ptmp = new uint32_t[box.m_buf_count + DVIDEO_FRAME_COUNT];
    if (ptmp) {
      if (box.m_entry_count > 0) {
        memcpy(ptmp, box.m_chunk_offset, box.m_entry_count * sizeof(uint32_t));
      }
      delete[] box.m_chunk_offset;
      box.m_buf_count += DVIDEO_FRAME_COUNT;
      box.m_chunk_offset = ptmp;
      box.m_chunk_offset[box.m_entry_count] = offset;
      box.m_entry_count ++;
    } else {
      ERROR("no memory\n");
      ret = AM_STATE_ERROR;
    }
  }
  return ret;
}

AM_STATE AMTimeElapseMp4Builder::insert_video_sample_size_box(uint32_t size)
{
  AM_STATE ret = AM_STATE_OK;
  SampleSizeBox& box = m_movie_box.m_video_track.m_media.m_media_info.\
      m_sample_table.m_sample_size;
  DEBUG("Insert video sample size box, size = %u", size);
  if(m_video_frame_number == 1) {
    box.m_sample_count = 0;
  }
  if (box.m_sample_count < box.m_buf_count) {
    box.m_size_array[box.m_sample_count] = (uint32_t) size;
    box.m_sample_count ++;
  } else {
    uint32_t* ptmp = new uint32_t[box.m_buf_count +
                                    DVIDEO_FRAME_COUNT];
    if (ptmp) {
      if (box.m_sample_count > 0) {
        memcpy(ptmp, box.m_size_array, box.m_sample_count * sizeof(uint32_t));
      }
      delete[] box.m_size_array;
      box.m_buf_count += DVIDEO_FRAME_COUNT;
      box.m_size_array = ptmp;
      box.m_size_array[box.m_sample_count] = (uint32_t) size;
      ++ box.m_sample_count;
    } else {
      ERROR("no memory\n");
      ret = AM_STATE_ERROR;
    }
  }
  return ret;
}

AM_STATE AMTimeElapseMp4Builder::insert_video_sync_sample_box(
    uint32_t video_frame_number)
{
  AM_STATE ret = AM_STATE_OK;
  SyncSampleTableBox& box = m_movie_box.m_video_track.m_media.\
      m_media_info.m_sample_table.m_sync_sample;
  do {
    if(m_video_frame_number == 1) {
      box.m_stss_count = 0;
    }
    if (box.m_stss_count >= box.m_buf_count) {
      uint32_t *tmp = new uint32_t[box.m_buf_count + DVIDEO_FRAME_COUNT];
      if (AM_UNLIKELY(!tmp)) {
        ERROR("malloc memory error.");
        ret = AM_STATE_NO_MEMORY;
        break;
      }
      if(box.m_stss_count > 0) {
        memcpy(tmp, box.m_sync_sample_table, box.m_stss_count * sizeof(uint32_t));
      }
      delete[] box.m_sync_sample_table;
      box.m_sync_sample_table = tmp;
      box.m_buf_count += DVIDEO_FRAME_COUNT;
    }
    box.m_sync_sample_table[box.m_stss_count] = video_frame_number;
    ++ box.m_stss_count;
  } while (0);
  return ret;
}

AM_STATE AMTimeElapseMp4Builder::write_video_frame_info(uint64_t offset,
                                         uint64_t size, uint8_t sync_sample)
{
  AM_STATE ret = AM_STATE_OK;
  DEBUG("write video frame info, offset = %llu, size = %llu", offset, size);
  do {
    if (AM_UNLIKELY(insert_video_composition_time_to_sample_box(
        AM_PTS_DEFINE / m_config->frame_rate) != AM_STATE_OK)) {
      ERROR("Failed to insert video composition time to sample box.");
      ret = AM_STATE_ERROR;
      break;
    }
    if (AM_UNLIKELY(insert_video_decoding_time_to_sample_box(
        AM_PTS_DEFINE / m_config->frame_rate) != AM_STATE_OK)) {
      ERROR("Failed to insert video decoding time to sample box.");
      ret = AM_STATE_ERROR;
      break;
    }
    if (CHUNK_OFFSET64_ENABLE) {
      if (AM_UNLIKELY(insert_video_chunk_offset64_box(offset) != AM_STATE_OK)) {
        ERROR("Failed to insert video chunk offset64 box.");
        ret = AM_STATE_ERROR;
        break;
      }
    } else {
      if (AM_UNLIKELY(insert_video_chunk_offset32_box(offset) != AM_STATE_OK)) {
        ERROR("Failed to insert video chunk offset box.");
        ret = AM_STATE_ERROR;
        break;
      }
    }
    if (AM_UNLIKELY(insert_video_sample_size_box(size) != AM_STATE_OK)) {
      ERROR("Failed to insert video sample size box.");
      ret = AM_STATE_ERROR;
      break;
    }
    if (sync_sample) {
      if (AM_UNLIKELY(insert_video_sync_sample_box(m_video_frame_number)
          != AM_STATE_OK)) {
        ERROR("Failed to insert video sync sample box.");
        ret = AM_STATE_ERROR;
        break;
      }
    }
  } while (0);
  return ret;
}

AM_STATE AMTimeElapseMp4Builder::update_media_box_size()
{
  AM_STATE ret = AM_STATE_OK;
  uint32_t offset = m_file_type_box.m_base_box.m_size;
  do {
    offset += m_movie_box_pre_length;
    m_media_data_box.m_base_box.m_size += (uint32_t) m_overall_media_data_len;
    if (AM_UNLIKELY(m_file_writer->seek_data(offset, SEEK_SET)
                    != AM_STATE_OK)) {
      ERROR("Failed to seek data.");
      ret = AM_STATE_IO_ERROR;
      break;
    }
    if (AM_UNLIKELY(m_file_writer->write_be_u32(
        m_media_data_box.m_base_box.m_size) != AM_STATE_OK)) {
      ERROR("Failed to write data to file");
      ret = AM_STATE_IO_ERROR;
      break;
    }
  } while (0);
  return ret;
}

AM_STATE AMTimeElapseMp4Builder::begin_file()
{
  AM_STATE ret = AM_STATE_OK;
  do {
    m_write_media_data_started = 0;
    m_overall_media_data_len = 0;
    m_video_duration = 0;
    m_last_ctts = 0;
    m_video_frame_number = 0;
    m_have_B_frame = false;
    m_next_track_id = 1;
    m_h265_frame_state = AM_H265_FRAME_NONE;
    struct timeval val;
    if (gettimeofday(&val, nullptr) != 0) {
      PERROR("Get time of day :");
    } else {
      m_creation_time = val.tv_sec + TIME_OFFSET;
      m_modification_time = val.tv_sec + TIME_OFFSET;
    }
    fill_file_type_box();
    fill_media_data_box();
    m_movie_box_pre_length = MP4_TRACK_HEADER_BOX_PRE_LENGTH * 2;
    m_movie_box_pre_length += MP4_SAMPLE_TABLE_PRE_LENGTH *
                   m_config->frame_rate * m_config->file_duration * 2;
    if (AM_UNLIKELY((ret = m_file_writer->write_file_type_box(m_file_type_box))
                     != AM_STATE_OK)) {
      ERROR("Failed to write file type box.");
      break;
    }
    uint8_t buf[512] = { 0 };
    for(uint32_t i = 0; i < (m_movie_box_pre_length / 512); ++ i) {
      if(m_file_writer->write_data(buf, 512) != AM_STATE_OK) {
        ERROR("Failed to write data to mp4 file.");
        ret = AM_STATE_IO_ERROR;
        break;
      }
    }
    uint32_t size = m_movie_box_pre_length % 512;
    if(m_file_writer->write_data(buf, size) != AM_STATE_OK) {
      ERROR("Failed to write data to mp4 file.");
      ret = AM_STATE_IO_ERROR;
      break;
    }
    if (AM_UNLIKELY((ret = m_file_writer->write_media_data_box(
        m_media_data_box)) != AM_STATE_OK)) {
      ERROR("Failed to write media data box.");
      break;
    }
  } while (0);
  return ret;
}

AM_STATE AMTimeElapseMp4Builder::end_file()
{
  AM_STATE ret = AM_STATE_OK;
  do {
    if(m_video_frame_number > 0) {
      m_video_track_id = m_next_track_id ++;
    }
    if (AM_LIKELY(m_write_media_data_started)) {
      m_write_media_data_started = 0;
      if (AM_UNLIKELY(fill_movie_box() != AM_STATE_OK)) {
        ERROR("Failed to fill movie box.");
        ret = AM_STATE_ERROR;
        break;
      }
      m_movie_box.update_movie_box_size();
      if(m_file_writer->seek_data(m_file_type_box.m_base_box.m_size, SEEK_SET)
          != AM_STATE_OK) {
        ERROR("Failed to seek data");
        ret = AM_STATE_IO_ERROR;
        break;
      }
      uint32_t free_box_length = m_movie_box_pre_length -
          m_movie_box.m_base_box.m_size;
      NOTICE("movie box pre length is %u", m_movie_box_pre_length);
      NOTICE("m_movie box length is %u", m_movie_box.m_base_box.m_size);
      NOTICE("free box length is %u", free_box_length);
      fill_free_box(free_box_length);
      if (AM_UNLIKELY((ret = m_file_writer->write_movie_box(
          m_movie_box)) != AM_STATE_OK)) {
        ERROR("Failed to write tail.");
        break;
      }
      if (AM_UNLIKELY((ret = m_file_writer->write_free_box(
          m_free_box)) != AM_STATE_OK)) {
        ERROR("Failed to write free box");
        break;
      }
      if (AM_UNLIKELY((ret = update_media_box_size()) != AM_STATE_OK)) {
        ERROR("Failed to end file");
        break;
      }
    }
  } while (0);
  return ret;
}
