/*******************************************************************************
 * am_muxer_AV3_builder.cpp
 *
 * History:
 *   2016-08-30 - [ccjing] created file
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

#include "am_muxer_codec_if.h"
#include "am_muxer_codec_info.h"
#include "am_file_sink_if.h"
#include "am_video_param_sets_parser_if.h"
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>

#include "am_utils.h"
#include "am_data_signature_if.h"
#include "am_muxer_av3_config.h"
#include "am_muxer_av3_file_writer.h"
#include "am_muxer_av3_index_writer.h"
#include "am_muxer_av3_builder.h"

/* seconds from 1904-01-01 00:00:00 UTC to 1969-12-31 24:00:00 UTC */
#define TIME_OFFSET    2082844800ULL

AMMuxerAv3Builder* AMMuxerAv3Builder::create(AMAv3FileWriter* file_writer,
                                             AMMuxerCodecAv3Config* config)
{
  AMMuxerAv3Builder *result = new AMMuxerAv3Builder();
  if (result && result->init(file_writer, config)
      != AM_STATE_OK) {
    delete result;
    result = NULL;
  }
  return result;
}

AMMuxerAv3Builder::AMMuxerAv3Builder()
{
  m_adts.clear();
  m_h264_nalu_list.clear();
  m_h265_nalu_list.clear();
  m_h265_nalu_frame_queue.clear();
}

AM_STATE AMMuxerAv3Builder::init(AMAv3FileWriter* file_writer,
                                 AMMuxerCodecAv3Config* config)
{
  AM_STATE ret = AM_STATE_OK;
  do {
    default_av3_file_setting();
    if (AM_UNLIKELY(!file_writer)) {
      ERROR("Invalid parameter.");
      ret = AM_STATE_ERROR;
      break;
    }
    bool event = (config->muxer_attr == AM_MUXER_FILE_NORMAL) ? false : true;
    if(AM_UNLIKELY((m_index_writer = AMAv3IndexWriter::create(
        config->write_index_frequency, event)) == nullptr)) {
      ERROR("Failed to create index writer.");
      ret = AM_STATE_ERROR;
      break;
    }
    if (AM_UNLIKELY((m_h265_frame_queue = new AMPacketDQ()) == nullptr)) {
      ERROR("Failed to create h265_frame_queue.");
      ret = AM_STATE_NO_MEMORY;
      break;
    }
    m_hash_sig = AMIDataSignature::create(config->key_file_location.c_str());
    if (!m_hash_sig) {
      ERROR("Failed to create AMIDataSignature");
      ret = AM_STATE_NO_MEMORY;
      break;
    }
    m_config = config;
    m_file_writer = file_writer;
  } while (0);
  return ret;
}

AMMuxerAv3Builder::~AMMuxerAv3Builder()
{
  m_file_writer = nullptr;
  while(!m_h265_frame_queue->empty()) {
    m_h265_frame_queue->front()->release();
    m_h265_frame_queue->pop_front();
  }
  delete m_h265_frame_queue;
  m_h264_nalu_list.clear();
  m_h265_nalu_list.clear();
  m_h265_nalu_frame_queue.clear();
  m_index_writer->destroy();
  m_hash_sig->destroy();
  delete[] m_avc_sps;
  delete[] m_avc_pps;
  delete[] m_hevc_vps;
  delete[] m_hevc_sps;
  delete[] m_hevc_pps;
}

AM_STATE AMMuxerAv3Builder::set_video_info(AM_VIDEO_INFO* video_info)
{
  AM_STATE ret = AM_STATE_OK;
  if(AM_UNLIKELY(!video_info)) {
    ERROR("Invalid parameter of set video info");
    ret = AM_STATE_ERROR;
  } else {
    memcpy(&m_video_info, video_info, sizeof(AM_VIDEO_INFO));
    if (m_config->reconstruct_enable) {
      m_index_writer->set_video_info(video_info);
    }
  }
  return ret;
}

AM_STATE AMMuxerAv3Builder::set_audio_info(AM_AUDIO_INFO* audio_info)
{
  AM_STATE ret = AM_STATE_OK;
  if(AM_UNLIKELY(!audio_info)) {
    ERROR("Invalid parameter of set audio info");
    ret = AM_STATE_ERROR;
  } else {
    memcpy(&m_audio_info, audio_info, sizeof(AM_AUDIO_INFO));
    if (m_config->reconstruct_enable) {
      m_index_writer->set_audio_info(audio_info);
    }
  }
  return ret;
}

AM_STATE AMMuxerAv3Builder::write_h264_video_data(AMPacket *packet)
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
              if(m_config->reconstruct_enable) {
                m_index_writer->set_sps(m_avc_sps, m_avc_sps_length);
              }
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
              if (m_config->reconstruct_enable) {
                m_index_writer->set_pps(m_avc_pps, m_avc_pps_length);
              }
              if(AM_UNLIKELY(!parse_h264_pps(m_avc_pps, m_avc_pps_length))) {
                ERROR("Failed to parse avc pps.");
                ret = AM_STATE_ERROR;
                break;
              }
            }
          }break;
          case H264_AUD_HEAD:
          case H264_SEI_HEAD:
          case H264_IBP_HEAD:
          case H264_IDR_HEAD: {
            if (AM_UNLIKELY(m_file_writer->write_be_u32(nalu.size) !=
                AM_STATE_OK)) {
              ERROR("Write nalu length error.");
              ret = AM_STATE_IO_ERROR;
              break;
            }
            if (m_config->digital_sig_enable) {
              if (!m_hash_sig->signature_update_u32(nalu.size)) {
                ERROR("Signature update u32 error.");
                ret = AM_STATE_ERROR;
                break;
              }
            }
            if (m_config->video_scramble_enable &&
                ((nalu.type == H264_IBP_HEAD) || (nalu.type == H264_IDR_HEAD))) {
              if (FCScrambleProc2(nalu.addr, m_video_delta_pts_sum,
                                  sync_sample, nalu.size) < 0) {
                ERROR("Video scramble error.");
              }
            }
            if (AM_UNLIKELY(m_file_writer->write_data(nalu.addr, nalu.size) !=
                AM_STATE_OK)) {
              ERROR("Failed to nalu data to file.");
              ret = AM_STATE_IO_ERROR;
              break;
            }
            if (m_config->digital_sig_enable) {
              if (!m_hash_sig->signature_update_data(nalu.addr, nalu.size)) {
                ERROR("Signature update data error.");
                ret = AM_STATE_ERROR;
                break;
              }
            }
          }break;
          default: {
            NOTICE("nalu type is not needed, drop this nalu");
          }break;
        }
      }
      int64_t delta_pts = 0;
      if(AM_UNLIKELY(ret != AM_STATE_OK)) {
        break;
      }
      m_overall_media_data_len += m_file_writer->get_file_offset() - chunk_offset;
      if(m_video_frame_number == 0) {
        delta_pts = 90000 * m_video_info.rate / m_video_info.scale;
        m_first_video_pts = packet->get_pts();
      } else {
        delta_pts = packet->get_pts() - m_last_video_pts;
      }
      m_last_video_pts = packet->get_pts();
      m_video_duration += delta_pts < 0 ? 0 : delta_pts;
      ++ m_video_frame_number;
      if (AM_UNLIKELY(!insert_video_frame_info(delta_pts, chunk_offset,
                m_file_writer->get_file_offset() - chunk_offset, sync_sample))) {
        ERROR("Failed to write video frame info.");
        ret = AM_STATE_ERROR;
        break;
      }
      if (m_config->reconstruct_enable) {
        if (!m_index_writer->write_video_information(delta_pts, chunk_offset,
               m_file_writer->get_file_offset() - chunk_offset, sync_sample)) {
          ERROR("Failed to write video info to AV3 index file.");
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

AM_STATE AMMuxerAv3Builder::write_h265_video_data(AMPacket *packet)
{
  AM_STATE ret = AM_STATE_OK;
  do {
    if(AM_LIKELY(find_h265_nalu(packet->get_data_ptr(), packet->get_data_size()))) {
      for(uint32_t i = 0;
          (i < m_h265_nalu_list.size()) && (ret == AM_STATE_OK); ++ i) {
        AM_H265_NALU &nalu = m_h265_nalu_list[i];
        switch (nalu.type) {
          case H265_VPS: {
            if (m_hevc_vps == nullptr) {
              m_hevc_vps = new uint8_t[nalu.size];
              m_hevc_vps_length = nalu.size;
              if (AM_UNLIKELY(!m_hevc_vps)) {
                ERROR("Failed to alloc memory for hevc vps info.");
                ret = AM_STATE_ERROR;
                break;
              }
              memcpy(m_hevc_vps, nalu.addr, nalu.size);
              if (m_config->reconstruct_enable) {
                m_index_writer->set_pps(m_hevc_vps, m_hevc_vps_length);
              }
              if(AM_UNLIKELY(!parse_h265_vps(nalu.addr, nalu.size))) {
                ERROR("Failed to parse h265 vps.");
                ret = AM_STATE_ERROR;
                break;
              }
            }
          }break;
          case H265_SPS: {
            if (m_hevc_sps == nullptr) {
              m_hevc_sps = new uint8_t[nalu.size];
              if (AM_UNLIKELY(!m_hevc_sps)) {
                ERROR("Failed to alloc memory for hevc sps info.");
                ret = AM_STATE_ERROR;
                break;
              }
              m_hevc_sps_length = nalu.size;
              memcpy(m_hevc_sps, nalu.addr, nalu.size);
              if (m_config->reconstruct_enable) {
                m_index_writer->set_sps(m_hevc_sps, m_hevc_sps_length);
              }
              if(AM_UNLIKELY(!parse_h265_sps(nalu.addr, nalu.size))) {
                ERROR("Failed to parse sps.");
                ret = AM_STATE_ERROR;
                break;
              }
            }
          }break;
          case H265_PPS : {
            if (m_hevc_pps == nullptr) {
              m_hevc_pps = new uint8_t[nalu.size];
              if (AM_UNLIKELY(!m_hevc_pps)) {
                ERROR("Failed to alloc memory for hevc pps info.");
                ret = AM_STATE_ERROR;
                break;
              }
              m_hevc_pps_length = nalu.size;
              memcpy(m_hevc_pps, nalu.addr, nalu.size);
              if (m_config->reconstruct_enable) {
                m_index_writer->set_pps(m_hevc_pps, m_hevc_pps_length);
              }
              if(AM_UNLIKELY(!parse_h265_pps(nalu.addr, nalu.size))) {
                ERROR("Failed to parse pps.");
                ret = AM_STATE_ERROR;
                break;
              }
            }
          } break;
          case H265_AUD:
          case H265_SEI_PREFIX :
          case H265_SEI_SUFFIX :
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
        int64_t   delta_pts = 0;
        int64_t   frame_pts = m_h265_frame_queue->front()->get_pts();
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
        if (AM_UNLIKELY(0 == m_video_frame_number)) {
          delta_pts = 90000 * m_video_info.rate / m_video_info.scale;
          m_first_video_pts = packet->get_pts();
        } else {
          delta_pts = frame_pts - m_last_video_pts;
        }
        m_video_duration += delta_pts < 0 ? 0 : delta_pts;
        ++ m_video_frame_number;
        m_last_video_pts = frame_pts;
        if (AM_UNLIKELY(!insert_video_frame_info(delta_pts, chunk_offset,
                                frame_size, sync_sample))) {
          ERROR("Failed to write video frame info.");
          break;
        }
        if (m_config->reconstruct_enable) {
          if(!m_index_writer->write_video_information(delta_pts, chunk_offset,
                                       frame_size, sync_sample)) {
            ERROR("Failed to write video info to AV3 index file.");
          }
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

AM_STATE AMMuxerAv3Builder::write_video_data(AMPacket *packet)
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

AM_STATE AMMuxerAv3Builder::write_audio_aac_data(AMPacket *packet,
                                                 bool new_chunk,
                                                 uint32_t frame_number)
{
  AM_STATE ret = AM_STATE_OK;
  uint8_t *frame_start;
  uint32_t frame_size;
  uint8_t* pdata = packet->get_data_ptr();
  uint32_t len = packet->get_data_size();
  uint32_t frame_count = packet->get_frame_count();
  do {
    if(!feed_stream_data(pdata, len, frame_count)) {
      ERROR("Feed stream data error.");
      ret = AM_STATE_ERROR;
      break;
    }
    uint32_t frame_num_in_pkt = 0;
    int32_t pts_diff_frame = m_audio_info.pkt_pts_increment / frame_count;
    int64_t delta_pts = 0;
    while (get_one_frame(&frame_start, &frame_size)) {
      ++ frame_num_in_pkt;
      if (frame_num_in_pkt < frame_number) {
        continue;
      }
      int64_t cur_frame_pts = packet->get_pts() -
          pts_diff_frame * (frame_count - frame_num_in_pkt);
      if (m_audio_frame_number == 0) {
        delta_pts = (m_first_video_pts < cur_frame_pts) ?
            (cur_frame_pts - m_first_video_pts) : pts_diff_frame;
        m_audio_duration += pts_diff_frame;
        delta_pts += pts_diff_frame;
      } else {
        delta_pts = cur_frame_pts - m_last_audio_pts;
        m_audio_duration += delta_pts;
      }
      m_last_audio_pts = cur_frame_pts;
      uint64_t chunk_offset = m_file_writer->get_file_offset();
      AdtsHeader *adts_header = (AdtsHeader*) frame_start;
      uint32_t header_length = sizeof(AdtsHeader);
      if (AM_UNLIKELY(frame_size < 7)) {
        ERROR("Audio frame size is too short.");
        ret = AM_STATE_ERROR;
        break;
      }
      if (AM_UNLIKELY(false == adts_header->is_sync_word_ok())) {
        ERROR("Adts sync word error.");
        ret = AM_STATE_ERROR;
        break;
      }
      if (AM_LIKELY(adts_header->aac_frame_number() == 0)) {
        if (adts_header->protection_absent() == 0) {
          header_length += 2;
        }
        if (AM_UNLIKELY(0xffff == m_aac_spec_config)) {
          m_aac_spec_config = (((adts_header->aac_audio_object_type() << 11) |
              (adts_header->aac_frequency_index() << 7)   |
              (adts_header->aac_channel_conf() << 3)) & 0xFFF8);
        }
        if (AM_UNLIKELY(m_file_writer->write_data(frame_start + header_length,
                                 frame_size - header_length) != AM_STATE_OK)) {
          ERROR("Failed to write data to file.");
          ret = AM_STATE_IO_ERROR;
          break;
        }
        if (m_config->digital_sig_enable) {
          if (!m_hash_sig->signature_update_data(frame_start + header_length,
                                                 frame_size - header_length)) {
            ERROR("Signature update data error.");
            ret = AM_STATE_ERROR;
            break;
          }
        }
      } else {
        NOTICE("Adts header aac frame number is not zero, Do not write "
            "this frame to file.");
      }
      m_overall_media_data_len += frame_size - header_length;
      ++ m_audio_frame_number;
      if (AM_UNLIKELY(!insert_audio_frame_info(delta_pts, chunk_offset,
                  (uint64_t)(m_file_writer->get_file_offset() - chunk_offset),
                  new_chunk))) {
        ERROR("Failed to write audio frame info.");
        ret = AM_STATE_ERROR;
        break;
      }
      new_chunk = false;
      if (m_config->reconstruct_enable) {
        if (!m_index_writer->write_audio_information(m_aac_spec_config,
                                                     delta_pts, frame_count,
                                                     chunk_offset,
                (uint64_t) (m_file_writer->get_file_offset() - chunk_offset))) {
          ERROR("Failed to write audio info into AV3 index file.");
        }
      }
    }
    if(ret != AM_STATE_OK) {
      break;
    }
    if (AM_UNLIKELY(m_write_media_data_started == 0)) {
      m_write_media_data_started = 1;
    }
  }  while(0);
  return ret;
}

AM_STATE AMMuxerAv3Builder::write_audio_data(AMPacket *packet,
                                             bool new_chunk,
                                             uint32_t frame_number)
{
  AM_STATE ret = AM_STATE_OK;
  switch (m_audio_info.type) {
    case AM_AUDIO_AAC : {
      ret = write_audio_aac_data(packet, new_chunk, frame_number);
    } break;
    default : {
      ERROR("The audio type %u is not supported currently", m_audio_info.type);
      ret = AM_STATE_ERROR;
    } break;
  }
  return ret;
}

AM_STATE AMMuxerAv3Builder::write_meta_data(AMPacket *packet, bool new_chunk)
{
  AM_STATE ret = AM_STATE_OK;
  uint8_t* pdata = packet->get_data_ptr();
  uint32_t len = packet->get_data_size();
  uint64_t chunk_offset = m_file_writer->get_file_offset();
  do {
    if (AM_UNLIKELY(m_file_writer->write_data(pdata, len) != AM_STATE_OK)) {
      ERROR("Failed to write meta data to file.");
      ret = AM_STATE_IO_ERROR;
      break;
    }
    if (m_config->digital_sig_enable) {
      if (!m_hash_sig->signature_update_data(pdata, len)) {
        ERROR("Signature update meta data error.");
        ret = AM_STATE_ERROR;
        break;
      }
    }
    m_overall_media_data_len += len;
    ++ m_meta_frame_number;
    if (AM_UNLIKELY(!insert_meta_frame_info(chunk_offset, len,
                                             new_chunk))) {
      ERROR("Failed to write audio frame info.");
      ret = AM_STATE_ERROR;
      break;
    }
    new_chunk = false;
    if (AM_UNLIKELY(m_write_media_data_started == 0)) {
      m_write_media_data_started = 1;
    }
  } while(0);
  return ret;
}

void AMMuxerAv3Builder::destroy()
{
  delete this;
}

AM_MUXER_CODEC_TYPE AMMuxerAv3Builder::get_muxer_type()
{
  return AM_MUXER_CODEC_AV3;
}

void AMMuxerAv3Builder::reset_video_params()
{
  delete[] m_avc_sps;
  m_avc_sps = nullptr;
  delete[] m_avc_pps;
  m_avc_pps = nullptr;
  delete[] m_hevc_vps;
  m_hevc_vps = nullptr;
  delete[] m_hevc_sps;
  m_hevc_sps = nullptr;
  delete[] m_hevc_pps;
  m_hevc_pps = nullptr;
}

void AMMuxerAv3Builder::clear_video_data()
{
  while(!m_h265_frame_queue->empty()) {
    m_h265_frame_queue->front()->release();
    m_h265_frame_queue->pop_front();
  }
  m_h264_nalu_list.clear();
  m_h265_nalu_list.clear();
  m_h265_nalu_frame_queue.clear();
}

void AMMuxerAv3Builder::default_av3_file_setting()
{
  m_used_version = 0;
  m_flags = 0;

  m_rate = 0x00010000;//fixed point 16.16, 1.0(0x00010000)is normal forward playback.
  m_volume = 0x0100;//fixed point 8.8, 1.0(0x0100) is full volume.

  m_matrix[1] = m_matrix[2] = m_matrix[3] = m_matrix[5] = m_matrix[6] =
      m_matrix[7] = 0;
  m_matrix[0] = m_matrix[4] = 0x00010000;
  m_matrix[8] = 0x40000000; //specified in spec

  m_audio_info.channels = 2;
  m_audio_info.sample_size = 16;
  m_audio_info.sample_rate = 48000;
}

bool AMMuxerAv3Builder::find_h264_nalu(uint8_t *data, uint32_t len,
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
bool AMMuxerAv3Builder::find_h265_nalu(uint8_t *data, uint32_t len)
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

bool AMMuxerAv3Builder::parse_h264_sps(uint8_t *sps, uint32_t size)
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
    memset(&m_avc_sps_struct, 0, sizeof(m_avc_sps_struct));
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

bool AMMuxerAv3Builder::parse_h264_pps(uint8_t *pps, uint32_t size)
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
    memset(&m_avc_pps_struct, 0, sizeof(m_avc_pps_struct));
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

void AMMuxerAv3Builder::filter_emulation_prevention_three_byte(
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

bool AMMuxerAv3Builder::parse_h265_vps(uint8_t *vps, uint32_t size)
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
    memset(&m_hevc_vps_struct, 0, sizeof(m_hevc_vps_struct));
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

bool AMMuxerAv3Builder::parse_h265_sps(uint8_t *sps, uint32_t size)
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
    memset(&m_hevc_sps_struct, 0, sizeof(m_hevc_sps_struct));
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

bool AMMuxerAv3Builder::parse_h265_pps(uint8_t *pps, uint32_t size)
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
    memset(&m_hevc_pps_struct, 0, sizeof(m_hevc_pps_struct));
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

void AMMuxerAv3Builder::find_adts (uint8_t *bs)
{
  m_adts.clear();
  for (uint32_t i = 0; i < m_audio_frame_count_per_packet; ++ i) {
    ADTS adts;
    adts.addr = bs;
    adts.size = ((AdtsHeader*)bs)->frame_length();
    bs += adts.size;
    m_adts.push_back(adts);
  }
}

bool AMMuxerAv3Builder::feed_stream_data(uint8_t *input_buf,
                            uint32_t input_size,  uint32_t frame_count)
{
  bool ret = true;
  m_audio_frame_count_per_packet =  frame_count;
  switch (m_audio_info.type) {
    case AM_AUDIO_AAC : {
      find_adts(input_buf);
      m_curr_adts_index = 0;
    } break;
    default : {
      ERROR("The audio type %u is not supported currently", m_audio_info.type);
      ret = false;
    } break;
  }
  return ret;
}

bool AMMuxerAv3Builder::get_one_frame(uint8_t **frame_start,
                                 uint32_t *frame_size)
{
  bool ret = true;
  do {
    switch (m_audio_info.type) {
      case AM_AUDIO_AAC : {
        if (m_curr_adts_index >= m_audio_frame_count_per_packet) {
          ret = false;
          break;
        }
        *frame_start  = m_adts[m_curr_adts_index].addr;
        *frame_size    = m_adts[m_curr_adts_index].size;
        ++ m_curr_adts_index;
      } break;
      default : {
        ERROR("The audio type %u is not supported currently", m_audio_info.type);
        ret = false;
      } break;
    }
    if(!ret) {
      break;
    }
  } while(0);
  return ret;
}

int AMMuxerAv3Builder::FCScrambleProc2(unsigned char *data, int timestamp,
                                       int extended, int len)
{
  return 1;
}

void AMMuxerAv3Builder::fill_file_type_box()
{
  AV3FileTypeBox& box = m_file_type_box;
  box.m_base_box.m_enable = true;
  box.m_base_box.m_type = AV3_FILE_TYPE_BOX_TAG;
  box.m_major_brand = AV3_BRAND_TAG_0;
  box.m_minor_version = 1;
  box.m_compatible_brands[0] = AV3_BRAND_TAG_0;
  box.m_compatible_brands[1] = AV3_BRAND_TAG_1;
  box.m_compatible_brands_number = 2;
  box.m_base_box.m_size = AV3_BASE_BOX_SIZE +
                          sizeof(m_file_type_box.m_major_brand) +
                          sizeof(m_file_type_box.m_minor_version) +
                (m_file_type_box.m_compatible_brands_number * AV3_TAG_SIZE);
}

void AMMuxerAv3Builder::fill_media_data_box()
{
  AV3MediaDataBox& box = m_media_data_box;
  box.m_base_box.m_enable = true;
  box.m_base_box.m_type = AV3_MEDIA_DATA_BOX_TAG;
  box.m_base_box.m_size = AV3_BASE_BOX_SIZE + sizeof(uint32_t) +
      sizeof(box.m_hash);
  box.m_flags = 0; // 1024 bits RSA
  if (m_config->digital_sig_enable) {
    box.m_flags |= 0x10000000;
  }
  if (m_config->video_scramble_enable) {
    box.m_flags |= 0x80000000;
  }
}

void AMMuxerAv3Builder::fill_movie_header_box()
{
  AV3MovieHeaderBox& box = m_movie_box.m_movie_header_box;
  box.m_full_box.m_base_box.m_enable = true;
  box.m_full_box.m_base_box.m_type = AV3_MOVIE_HEADER_BOX_TAG;
  box.m_full_box.m_version = m_used_version;
  box.m_full_box.m_flags = m_flags;
  box.m_creation_time = m_creation_time;
  box.m_modification_time = m_modification_time;
  box.m_timescale = 90000;
  box.m_duration = (m_video_duration >= m_audio_duration) ?
      m_video_duration : m_audio_duration;
  box.m_rate = m_rate;
  box.m_volume = m_volume;
  for (uint32_t i = 0; i < 9; i ++) {
    box.m_matrix[i] = m_matrix[i];
  }
  box.m_next_track_ID = 0x03;//video = 1, audio = 2
  box.m_full_box.m_base_box.m_size = AV3_MOVIE_HEADER_SIZE;
}

void AMMuxerAv3Builder::fill_video_track_header_box()
{
  AV3TrackHeaderBox& box = m_movie_box.m_video_track.m_track_header;
  box.m_full_box.m_base_box.m_enable = true;
  box.m_full_box.m_base_box.m_type = AV3_TRACK_HEADER_BOX_TAG;
  box.m_full_box.m_version = m_used_version;
  box.m_full_box.m_flags = 0x07;//track_enabled, track_in_movie, track_in_preview
  box.m_creation_time = m_creation_time;
  box.m_modification_time = m_modification_time;
  box.m_track_ID = m_video_track_id;
  box.m_duration = m_video_duration;
  box.m_layer = 0x00;
  box.m_alternate_group = 0x00;
  box.m_volume = 0x00;
  for (uint32_t i = 0; i < 9; i ++) {
    box.m_matrix[i] = m_matrix[i];
  }
  if (!m_rotate) {
    box.m_width_integer = m_video_info.width;
    box.m_width_decimal = 0;
    box.m_height_integer = m_video_info.height;
    box.m_height_decimal = 0;
  } else {
    box.m_width_integer = m_video_info.height;
    box.m_width_decimal = 0;
    box.m_height_integer = m_video_info.width;
    box.m_height_decimal = 0;
  }
  box.m_full_box.m_base_box.m_size = AV3_TRACK_HEADER_SIZE;
}

void AMMuxerAv3Builder::fill_audio_track_header_box()
{
  AV3TrackHeaderBox& box = m_movie_box.m_audio_track.m_track_header;
  box.m_full_box.m_base_box.m_enable = true;
  box.m_full_box.m_base_box.m_type = AV3_TRACK_HEADER_BOX_TAG;
  box.m_full_box.m_version = m_used_version;
  box.m_full_box.m_flags = 0x07;//track_enabled, track_in_movie, track_in_preview
  box.m_creation_time = m_creation_time;
  box.m_modification_time = m_modification_time;
  box.m_track_ID = m_audio_track_id;
  box.m_duration = m_audio_duration;
  box.m_layer = 0x00;
  box.m_alternate_group = 0x00;
  box.m_volume = m_volume;
  box.m_width_integer = 0;
  box.m_width_decimal = 0;
  box.m_height_integer = 0;
  box.m_height_decimal = 0;
  for (uint32_t i = 0; i < 9; i ++) {
    box.m_matrix[i] = m_matrix[i];
  }
  box.m_full_box.m_base_box.m_size = AV3_TRACK_HEADER_SIZE;
}

void AMMuxerAv3Builder::fill_media_header_box_for_video_track()
{
  AV3MediaHeaderBox& box = m_movie_box.m_video_track.m_media.m_media_header;
  box.m_full_box.m_base_box.m_enable = true;
  box.m_full_box.m_base_box.m_type = AV3_MEDIA_HEADER_BOX_TAG;
  box.m_full_box.m_version = m_used_version;
  box.m_full_box.m_flags = m_flags;
  box.m_creation_time = m_creation_time;
  box.m_modification_time = m_modification_time;
  box.m_timescale = 90000;//the number of time units that pass in one second.
  box.m_duration = m_video_duration;
  box.m_language = 0;
  box.m_full_box.m_base_box.m_size = AV3_MEDIA_HEADER_SIZE;
}

void AMMuxerAv3Builder::fill_media_header_box_for_audio_track()
{
  AV3MediaHeaderBox& box = m_movie_box.m_audio_track.m_media.m_media_header;
  box.m_full_box.m_base_box.m_enable = true;
  box.m_full_box.m_base_box.m_type = AV3_MEDIA_HEADER_BOX_TAG;
  box.m_full_box.m_version = m_used_version;
  box.m_full_box.m_flags = m_flags;
  box.m_creation_time = m_creation_time;
  box.m_modification_time = m_modification_time;
  box.m_timescale = 48000;
  box.m_duration = m_audio_duration;
  box.m_language = 0;
  box.m_full_box.m_base_box.m_size = AV3_MEDIA_HEADER_SIZE;
}

bool AMMuxerAv3Builder::fill_video_handler_reference_box()
{
  bool ret = true;
  do {
    AV3HandlerReferenceBox& box =
        m_movie_box.m_video_track.m_media.m_media_handler;
    box.m_full_box.m_base_box.m_enable = true;
    box.m_full_box.m_base_box.m_type = AV3_HANDLER_REFERENCE_BOX_TAG;
    box.m_full_box.m_version = m_used_version;
    box.m_full_box.m_flags = m_flags;
    box.m_handler_type = VIDEO_TRACK;
    box.m_name = nullptr;
    box.m_name_size = 0;
    box.m_full_box.m_base_box.m_size = AV3_HANDLER_REF_SIZE + box.m_name_size;
  } while(0);
  return ret;
}

bool AMMuxerAv3Builder::fill_audio_handler_reference_box()
{
  bool ret = true;
  do {
    AV3HandlerReferenceBox& box =
        m_movie_box.m_audio_track.m_media.m_media_handler;
    box.m_full_box.m_base_box.m_enable = true;
    box.m_full_box.m_base_box.m_type = AV3_HANDLER_REFERENCE_BOX_TAG;
    box.m_full_box.m_version = m_used_version;
    box.m_full_box.m_flags = m_flags;
    box.m_handler_type = AUDIO_TRACK;
    box.m_name = nullptr;
    box.m_name_size = 0;
    box.m_full_box.m_base_box.m_size = AV3_HANDLER_REF_SIZE + box.m_name_size;
  } while(0);
  return ret;
}

bool AMMuxerAv3Builder::fill_video_media_info_box()
{
  bool ret = true;
  AV3MediaInformationBox& box = m_movie_box.m_video_track.m_media.m_media_info;
  do {
    box.m_base_box.m_enable = true;
    box.m_base_box.m_type = AV3_MEDIA_INFORMATION_BOX_TAG;
    fill_video_media_info_header_box();
    fill_video_data_info_box();
    if (AM_UNLIKELY(!fill_video_sample_table_box())) {
      ERROR("Failed to fill video sample table box.");
      ret = false;
      break;
    }
  } while (0);
  return ret;
}

bool AMMuxerAv3Builder::fill_audio_media_info_box()
{
  bool ret = true;
  AV3MediaInformationBox& box = m_movie_box.m_audio_track.m_media.m_media_info;
  do {
    box.m_base_box.m_type = AV3_MEDIA_INFORMATION_BOX_TAG;
    box.m_base_box.m_enable = true;
    fill_sound_media_info_header_box();
    fill_audio_data_info_box();
    if (AM_UNLIKELY(!fill_sound_sample_table_box())) {
      ERROR("Failed to fill sound sample table box.");
      ret = false;
      break;
    }
  } while (0);
  return ret;
}

void AMMuxerAv3Builder::fill_video_media_info_header_box()
{
  AV3VideoMediaInfoHeaderBox& box =
      m_movie_box.m_video_track.m_media.m_media_info.m_video_info_header;
  box.m_full_box.m_base_box.m_enable = true;
  box.m_full_box.m_base_box.m_type = AV3_VIDEO_MEDIA_HEADER_BOX_TAG;
  box.m_full_box.m_version = m_used_version;
  box.m_full_box.m_flags = 0x01;
  box.m_graphicsmode = 0;
  box.m_opcolor[0] = box.m_opcolor[1] = box.m_opcolor[2] = 0;
  box.m_full_box.m_base_box.m_size = AV3_VIDEO_MEDIA_INFO_HEADER_SIZE;
}

void AMMuxerAv3Builder::fill_sound_media_info_header_box()
{
  AV3SoundMediaInfoHeaderBox& box =
      m_movie_box.m_audio_track.m_media.m_media_info.m_sound_info_header;
  box.m_full_box.m_base_box.m_enable = true;
  box.m_full_box.m_base_box.m_type = AV3_SOUND_MEDIA_HEADER_BOX_TAG;
  box.m_full_box.m_version = m_used_version;
  box.m_full_box.m_flags = m_flags;
  box.m_balanse = 0;
  box.m_full_box.m_base_box.m_size = AV3_SOUND_MEDIA_HEADER_SIZE;
}

void AMMuxerAv3Builder::fill_video_data_info_box()
{
  AV3DataInformationBox& box = m_movie_box.m_video_track.m_media.\
      m_media_info.m_data_info;
  box.m_base_box.m_enable = true;
  box.m_base_box.m_type = AV3_DATA_INFORMATION_BOX_TAG;
  fill_video_data_reference_box();
  box.m_base_box.m_size = box.m_data_ref.m_full_box.m_base_box.m_size +
      AV3_BASE_BOX_SIZE;
}

void AMMuxerAv3Builder::fill_audio_data_info_box()
{
  AV3DataInformationBox& box = m_movie_box.m_audio_track.m_media.\
      m_media_info.m_data_info;
  box.m_base_box.m_enable = true;
  box.m_base_box.m_type = AV3_DATA_INFORMATION_BOX_TAG;
  fill_audio_data_reference_box();
  box.m_base_box.m_size = box.m_data_ref.m_full_box.m_base_box.m_size +
      AV3_BASE_BOX_SIZE;
}

void AMMuxerAv3Builder::fill_video_data_reference_box()
{
  AV3DataReferenceBox& box = m_movie_box.m_video_track.m_media.\
      m_media_info.m_data_info.m_data_ref;
  box.m_full_box.m_base_box.m_enable = true;
  box.m_full_box.m_base_box.m_type = AV3_DATA_REFERENCE_BOX_TAG;
  box.m_full_box.m_version = m_used_version;
  box.m_full_box.m_flags = m_flags;
  box.m_entry_count = 1;
  box.m_url.m_full_box.m_base_box.m_type = AV3_DATA_ENTRY_URL_BOX_TAG;
  box.m_url.m_full_box.m_base_box.m_enable = true;
  box.m_url.m_full_box.m_version = m_used_version;
  box.m_url.m_full_box.m_flags = 1;
  box.m_url.m_full_box.m_base_box.m_size = AV3_FULL_BOX_SIZE;
  box.m_full_box.m_base_box.m_size = AV3_FULL_BOX_SIZE +
      box.m_entry_count * box.m_url.m_full_box.m_base_box.m_size
      + sizeof(uint32_t);
}

void AMMuxerAv3Builder::fill_audio_data_reference_box()
{
  AV3DataReferenceBox& box = m_movie_box.m_audio_track.m_media.\
      m_media_info.m_data_info.m_data_ref;
  box.m_full_box.m_base_box.m_enable = true;
  box.m_full_box.m_base_box.m_type = AV3_DATA_REFERENCE_BOX_TAG;
  box.m_full_box.m_version = m_used_version;
  box.m_full_box.m_flags = m_flags;
  box.m_entry_count = 1;
  box.m_url.m_full_box.m_base_box.m_type = AV3_DATA_ENTRY_URL_BOX_TAG;
  box.m_url.m_full_box.m_base_box.m_enable = true;
  box.m_url.m_full_box.m_version = m_used_version;
  box.m_url.m_full_box.m_flags = 1;
  box.m_url.m_full_box.m_base_box.m_size = AV3_FULL_BOX_SIZE;
  box.m_full_box.m_base_box.m_size = AV3_FULL_BOX_SIZE +
      box.m_entry_count * box.m_url.m_full_box.m_base_box.m_size
      + sizeof(uint32_t);
}

void AMMuxerAv3Builder::fill_video_chunk_offset32_box()
{
  AV3ChunkOffsetBox& box = m_movie_box.m_video_track.m_media.\
      m_media_info.m_sample_table.m_chunk_offset;
  box.m_full_box.m_base_box.m_enable = true;
  box.m_full_box.m_base_box.m_type = AV3_CHUNK_OFFSET_BOX_TAG;
  box.m_full_box.m_version = m_used_version;
  box.m_full_box.m_flags = m_flags;
}

void AMMuxerAv3Builder::fill_audio_chunk_offset32_box()
{
  AV3ChunkOffsetBox& box = m_movie_box.m_audio_track.m_media.\
      m_media_info.m_sample_table.m_chunk_offset;
  box.m_full_box.m_base_box.m_enable = true;
  box.m_full_box.m_base_box.m_type = AV3_CHUNK_OFFSET_BOX_TAG;
  box.m_full_box.m_version = m_used_version;
  box.m_full_box.m_flags = m_flags;
}

void AMMuxerAv3Builder::fill_video_sync_sample_box()
{
  AV3SyncSampleTableBox& box = m_movie_box.m_video_track.\
      m_media.m_media_info.m_sample_table.m_sync_sample;
  box.m_full_box.m_base_box.m_enable = true;
  box.m_full_box.m_base_box.m_type = AV3_SYNC_SAMPLE_TABLE_BOX_TAG;
  box.m_full_box.m_flags = m_flags;
  box.m_full_box.m_version = m_used_version;
}

bool AMMuxerAv3Builder::fill_video_sample_to_chunk_box()
{
  bool ret = true;
  AV3SampleToChunkBox& box = m_movie_box.m_video_track.m_media.\
      m_media_info.m_sample_table.m_sample_to_chunk;
  box.m_full_box.m_base_box.m_enable = true;
  do {
    box.m_full_box.m_base_box.m_type = AV3_SAMPLE_TO_CHUNK_BOX_TAG;
    box.m_full_box.m_version = m_used_version;
    box.m_full_box.m_flags = m_flags;
  } while (0);
  return ret;
}

bool AMMuxerAv3Builder::fill_audio_sample_to_chunk_box()
{
  bool ret = true;
  AV3SampleToChunkBox& box = m_movie_box.m_audio_track.m_media.\
      m_media_info.m_sample_table.m_sample_to_chunk;
  box.m_full_box.m_base_box.m_enable = true;
  do {
    box.m_full_box.m_base_box.m_type = AV3_SAMPLE_TO_CHUNK_BOX_TAG;
    box.m_full_box.m_version = m_used_version;
    box.m_full_box.m_flags = m_flags;
  } while (0);
  return ret;
}

void AMMuxerAv3Builder::fill_video_sample_size_box()
{
  AV3SampleSizeBox& box = m_movie_box.m_video_track.m_media.\
      m_media_info.m_sample_table.m_sample_size;
  box.m_full_box.m_base_box.m_enable = true;
  box.m_full_box.m_base_box.m_type = AV3_SAMPLE_SIZE_BOX_TAG;
  box.m_full_box.m_version = m_used_version;
  box.m_full_box.m_flags = m_flags;
  box.m_sample_size = 0;
}

void AMMuxerAv3Builder::fill_audio_sample_size_box()
{
  AV3SampleSizeBox& box = m_movie_box.m_audio_track.m_media.\
      m_media_info.m_sample_table.m_sample_size;
  box.m_full_box.m_base_box.m_enable = true;
  box.m_full_box.m_base_box.m_type = AV3_SAMPLE_SIZE_BOX_TAG;
  box.m_full_box.m_version = m_used_version;
  box.m_full_box.m_flags = m_flags;
  box.m_sample_size = 0x0;
}

bool AMMuxerAv3Builder::fill_avc_decoder_configuration_record_box()
{
  bool ret = true;
  do {
    AV3AVCDecoderConfigurationRecord& box =
        m_movie_box.m_video_track.m_media.m_media_info.
        m_sample_table.m_sample_description.m_visual_entry.m_avc_config;
    box.m_base_box.m_enable = true;
    box.m_base_box.m_type = AV3_AVC_DECODER_CONFIGURATION_RECORD_TAG;
    box.m_config_version = 1;
    box.m_profile_indication = m_avc_sps_struct.profile_idc;
    /*sps->8bit value between profile_idc and level_idc, set 0 currently.*/
    box.m_profile_compatibility = 0;
    box.m_level_indication = m_avc_sps_struct.level_idc;
    box.m_len_size_minus_one = 3;
    box.m_sps_num = 1;
    if ((box.m_sps == nullptr) ||
        ((box.m_sps != nullptr) && (box.m_sps_len != m_avc_sps_length)) ||
        ((box.m_sps != nullptr) && (memcmp(box.m_sps, m_avc_sps,
                                           m_avc_sps_length) != 0))) {
      NOTICE("Reset avc sps in AV3 builder.");
      delete[] box.m_sps;
      box.m_sps_len = m_avc_sps_length;
      box.m_sps = new uint8_t[m_avc_sps_length];
      if(!box.m_sps) {
        ERROR("Failed to malloc memory when fill avc_decoder configuration "
            "record box");
        ret = false;
        break;
      }
      if(m_avc_sps == nullptr) {
        ERROR("m_avc_sps is nullptr");
        ret = false;
        break;
      }
      memcpy(box.m_sps, m_avc_sps, m_avc_sps_length);
    }
    box.m_pps_num = 1;
    if ((box.m_pps == nullptr) ||
        ((box.m_pps != nullptr) && (box.m_pps_len != m_avc_pps_length)) ||
        ((box.m_pps != nullptr) && (memcmp(box.m_pps, m_avc_pps,
                                          m_avc_pps_length) != 0))) {
      NOTICE("Reset avc pps in AV3 builder.");
      delete[] box.m_pps;
      box.m_pps_len = m_avc_pps_length;
      box.m_pps = new uint8_t[m_avc_pps_length];
      if(!box.m_pps) {
        ERROR("Failed to malloc memory when fill avc_decoder configuration "
            "record box");
        ret = false;
        break;
      }
      if(m_avc_pps == nullptr) {
        ERROR("m_avc_pps is nullptr");
        ret = false;
        break;
      }
      memcpy(box.m_pps, m_avc_pps, m_avc_pps_length);
    }
    box.m_base_box.m_size = AV3_AVC_DECODER_CONFIG_RECORD_SIZE +
        box.m_sps_len + box.m_pps_len;
    uint32_t& size = box.m_base_box.m_size;
    if ((size % 4) != 0) {
      box.m_padding_count = 4 - size % 4;
    }
    size += box.m_padding_count;
  } while(0);
  return ret;
}

bool AMMuxerAv3Builder::fill_visual_sample_description_box()
{
  bool ret = true;
  do {
    AV3SampleDescriptionBox& box = m_movie_box.m_video_track.\
        m_media.m_media_info.m_sample_table.m_sample_description;
    box.m_full_box.m_base_box.m_enable = true;
    box.m_full_box.m_base_box.m_type = AV3_SAMPLE_DESCRIPTION_BOX_TAG;
    box.m_full_box.m_version = m_used_version;
    box.m_full_box.m_flags = m_flags;
    box.m_entry_count = 1;
    switch (m_video_info.type) {
      case AM_VIDEO_H264 : {
        box.m_visual_entry.m_base_box.m_type = AV3_H264_ENTRY_TAG;
        box.m_visual_entry.m_base_box.m_enable = true;
        memset(box.m_visual_entry.m_compressorname, 0,
               sizeof(box.m_visual_entry.m_compressorname));
        strncpy(box.m_visual_entry.m_compressorname,
                AV3_VIDEO_H264_COMPRESSORNAME,
                (sizeof(box.m_visual_entry.m_compressorname) - 1));
        if(AM_UNLIKELY(!fill_avc_decoder_configuration_record_box())) {
          ERROR("Failed to fill avc decoder configuration record box.");
          ret = false;
          break;
        }
      } break;
      default : {
        ERROR("The video type %u is not supported currently.");
        ret = false;
        break;
      }
    }
    if(!ret) {
      break;
    }
    box.m_visual_entry.m_data_reference_index = 1;
    box.m_visual_entry.m_width = m_video_info.width;
    box.m_visual_entry.m_height = m_video_info.height;
    box.m_visual_entry.m_horizresolution = 0x00480000;
    box.m_visual_entry.m_vertresolution = 0x00480000;
    box.m_visual_entry.m_frame_count = 1;
    box.m_visual_entry.m_depth = 0x0018;
    box.m_visual_entry.m_pre_defined_2 = 0xffff;
    box.m_full_box.m_base_box.m_size = AV3_FULL_BOX_SIZE +
        sizeof(uint32_t) + box.m_visual_entry.m_base_box.m_size;
  } while(0);
  return ret;
}

void AMMuxerAv3Builder::fill_aac_description_box()
{
  AV3AACDescriptorBox& box = m_movie_box.m_audio_track.m_media.m_media_info.\
      m_sample_table.m_sample_description.m_audio_entry.m_aac;
  box.m_full_box.m_base_box.m_enable = true;
  box.m_full_box.m_base_box.m_type = AV3_AAC_DESCRIPTOR_BOX_TAG;
  box.m_full_box.m_version = m_used_version;
  box.m_full_box.m_flags = m_flags;
  /*ES descriptor*/
  box.m_es_descriptor_tag = 0x03;
  box.m_es_descriptor_tag_size = 25;
  box.m_es_id = 0x40;
  box.m_stream_dependency_flag = 0;
  box.m_URL_flag = 0;
  box.m_OCR_stream_flag = 0;
  box.m_stream_priority = 0;
  /*decoder config descriptor*/
  box.m_decoder_config_descriptor_tag = 0x04;;
  box.m_decoder_config_descriptor_tag_size = 17;
  box.m_object_type_id = 0x40;//MPEG-4 AAC = 64;
  box.m_stream_type = 5;//5 means audio stream
  box.m_up_stream = 0;
  box.m_reserved = 1;
  box.m_buffer_size_DB = 0x00;//3 bytes
  box.m_max_bitrate = 128000;//0x0001f400
  box.m_avg_bitrate = 128000;
  /*decoder specific info */
  box.m_decoder_specific_info_tag = 5;
  box.m_decoder_specific_info_tag_size = 2;
  box.m_audio_object_type = 2;
  box.m_sampling_frequency_index = 3;
  box.m_channel_configuration = 1;
  box.m_frame_length_flag = 0;
  box.m_depends_on_core_coder = 0;
  box.m_extension_flag = 0;
  /*SL descriptor*/
  box.m_SL_config_descriptor_tag = 6;
  box.m_SL_config_descriptor_tag_size = 1;
  box.m_predefined = 0x02;
  box.m_full_box.m_base_box.m_size = AV3_AAC_DESCRIPTOR_SIZE;
}

bool AMMuxerAv3Builder::fill_audio_sample_description_box()
{
  bool ret = true;
  do {
    AV3SampleDescriptionBox& box = m_movie_box.m_audio_track.m_media.\
        m_media_info.m_sample_table.m_sample_description;
    box.m_full_box.m_base_box.m_enable = true;
    box.m_full_box.m_base_box.m_type = AV3_SAMPLE_DESCRIPTION_BOX_TAG;
    box.m_full_box.m_version = m_used_version;
    box.m_full_box.m_flags = m_flags;
    box.m_entry_count = 1;
    switch (m_audio_info.type) {
      case AM_AUDIO_AAC : {
        box.m_audio_entry.m_base_box.m_type = AV3_AAC_ENTRY_TAG;
      } break;
      default : {
        ERROR("The audio type %u is not supported currently.");
        ret = false;
      } break;
    }
    if(!ret) {
      break;
    }
    box.m_audio_entry.m_base_box.m_enable = true;
    box.m_audio_entry.m_data_reference_index = 1;
    box.m_audio_entry.m_sound_version = 0;
    box.m_audio_entry.m_channels = m_audio_info.channels;
    box.m_audio_entry.m_sample_size = 0x0010;
    box.m_audio_entry.m_packet_size = 0;
    box.m_audio_entry.m_time_scale = 48000;
    switch (m_audio_info.type) {
      case AM_AUDIO_AAC : {
        fill_aac_description_box();
        box.m_audio_entry.m_base_box.m_size = AV3_AUDIO_SAMPLE_ENTRY_SIZE +
            box.m_audio_entry.m_aac.m_full_box.m_base_box.m_size;
      } break;
      default :
        ERROR("Audio type %u is not supported currently.", m_audio_info.type);
        ret = false;
        break;
    }
    if(!ret) {
      break;
    }
    box.m_full_box.m_base_box.m_size = AV3_FULL_BOX_SIZE +
        sizeof(uint32_t) + box.m_audio_entry.m_base_box.m_size;
    INFO("audio sample description size is %u when fill it.",
          box.m_full_box.m_base_box.m_size);
  } while(0);
  return ret;
}

bool AMMuxerAv3Builder::fill_video_decoding_time_to_sample_box()
{
  bool ret = true;
  AV3DecodingTimeToSampleBox& box =
      m_movie_box.m_video_track.m_media.m_media_info.m_sample_table.m_stts;
  do {
    box.m_full_box.m_base_box.m_enable = true;
    box.m_full_box.m_base_box.m_type =
        AV3_DECODING_TIME_TO_SAMPLE_TABLE_BOX_TAG;
    box.m_full_box.m_flags = m_flags;
    box.m_full_box.m_version = m_used_version;
  } while (0);
  return ret;
}

bool AMMuxerAv3Builder::fill_audio_decoding_time_to_sample_box()
{
  bool ret = true;
  AV3DecodingTimeToSampleBox& box =
      m_movie_box.m_audio_track.m_media.m_media_info.m_sample_table.m_stts;
  do {
    box.m_full_box.m_base_box.m_enable = true;
    box.m_full_box.m_base_box.m_type =
        AV3_DECODING_TIME_TO_SAMPLE_TABLE_BOX_TAG;
    box.m_full_box.m_flags = m_flags;
    box.m_full_box.m_version = m_used_version;
  } while (0);
  return ret;
}

bool AMMuxerAv3Builder::fill_video_sample_table_box()
{
  bool ret = true;
  AV3SampleTableBox& box =
      m_movie_box.m_video_track.m_media.m_media_info.m_sample_table;
  do {
    box.m_base_box.m_enable = true;
    box.m_base_box.m_type = AV3_SAMPLE_TABLE_BOX_TAG;
    if (!fill_visual_sample_description_box()) {
      ERROR("Failed to fill visual sample description box.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!fill_video_decoding_time_to_sample_box())) {
      ERROR("Failed to fill video decoding time to sample box.");
      ret = false;
      break;
    }
    fill_video_sample_size_box();
    if (AM_UNLIKELY(!fill_video_sample_to_chunk_box())) {
      ERROR("Failed to fill sample to chunk box.");
      ret = false;
      break;
    }
    fill_video_chunk_offset32_box();
    fill_video_sync_sample_box();
  } while (0);
  return ret;
}

bool AMMuxerAv3Builder::fill_sound_sample_table_box()
{
  bool ret = true;
  AV3SampleTableBox& box =
      m_movie_box.m_audio_track.m_media.m_media_info.m_sample_table;
  do {
    box.m_base_box.m_enable = true;
    box.m_base_box.m_type = AV3_SAMPLE_TABLE_BOX_TAG;
    if(!fill_audio_sample_description_box()) {
      ERROR("Fill audio sample description box error.");
      ret = AM_STATE_ERROR;
      break;
    }
    if(AM_UNLIKELY(!fill_audio_decoding_time_to_sample_box())) {
      ERROR("Failed to fill audio decoding time to sample box.");
      ret = AM_STATE_ERROR;
      break;
    }
    fill_audio_sample_size_box();
    if(AM_UNLIKELY(!fill_audio_sample_to_chunk_box())) {
      ERROR("Failed to fill sample to chunk box.");
      ret = AM_STATE_ERROR;
      break;
    }
    fill_audio_chunk_offset32_box();
  } while (0);
  return ret;
}

bool AMMuxerAv3Builder::fill_video_media_box()
{
  bool ret = true;
  AV3MediaBox& box = m_movie_box.m_video_track.m_media;
  do {
    box.m_base_box.m_enable = true;
    box.m_base_box.m_type = AV3_MEDIA_BOX_TAG;
    fill_media_header_box_for_video_track();
    if(!fill_video_handler_reference_box()) {
      ERROR("Fill video hander reference box error.");
      ret = false;
      break;
    }
    if(AM_UNLIKELY(!fill_video_media_info_box())) {
      ERROR("Failed to fill video media information box.");
      ret = false;
      break;
    }
  } while (0);
  return ret;
}

bool AMMuxerAv3Builder::fill_audio_media_box()
{
  bool ret = true;
  AV3MediaBox& box = m_movie_box.m_audio_track.m_media;
  do {
    box.m_base_box.m_enable = true;
    box.m_base_box.m_type = AV3_MEDIA_BOX_TAG;
    fill_media_header_box_for_audio_track();
    if(!fill_audio_handler_reference_box()) {
      ERROR("Fill audio hander reference box error.");
      ret = false;
      break;
    }
    if(AM_UNLIKELY(!fill_audio_media_info_box())) {
      ERROR("Failed to fill audio media information box.");
      ret = false;
      break;
    }
  } while (0);
  return ret;
}

bool AMMuxerAv3Builder::fill_video_track_uuid_box()
{
  bool ret = true;
  AV3TrackUUIDBox& box = m_movie_box.m_video_track.m_uuid;
  do {
    box.m_base_box.m_enable = true;
    box.m_base_box.m_type = AV3_UUID_BOX_TAG;
    if (!fill_video_psmt_box()) {
      ERROR("Failed to fill video psmt box.");
      ret = false;
      break;
    }
  } while(0);
  return ret;
}

bool AMMuxerAv3Builder::fill_video_psmt_box()
{
  bool ret = true;
  AV3PSMTBox& box = m_movie_box.m_video_track.m_uuid.m_psmt_box;
  do {
    box.m_base_box.m_enable = true;
    box.m_base_box.m_type = AV3_PSMT_BOX_TAG;
    if (!fill_video_psmh_box()) {
      ERROR("Failed to fill video psmh box.");
      ret = false;
      break;
    }
    if (m_meta_frame_number > 0) {
      INFO("Meta data frame count is %u", m_meta_frame_number);
      if (!fill_meta_sample_table_box()) {
        ERROR("Failed to fill meta sample table box.");
        ret = false;
        break;
      }
    } else {
      NOTICE("There is no meta data in av3 file");
    }
  } while(0);
  return ret;
}

bool AMMuxerAv3Builder::fill_video_psmh_box()
{
  bool ret = true;
  AV3PSMHBox& box = m_movie_box.m_video_track.m_uuid.m_psmt_box.m_psmh_box;
  do {
    box.m_base_box.m_enable = true;
    box.m_base_box.m_type = AV3_PSMH_BOX_TAG;
    box.m_meta_id = 0x01;
  } while(0);
  return ret;
}

bool AMMuxerAv3Builder::fill_meta_sample_table_box()
{
  bool ret = true;
  AV3SampleTableBox& box =
      m_movie_box.m_video_track.m_uuid.m_psmt_box.m_meta_sample_table;
  do {
    box.m_base_box.m_enable = true;
    box.m_base_box.m_type = AV3_SAMPLE_TABLE_BOX_TAG;
    fill_meta_sample_size_box();
    if (AM_UNLIKELY(!fill_meta_sample_to_chunk_box())) {
      ERROR("Failed to fill meta sample to chunk box.");
      ret = false;
      break;
    }
    fill_meta_chunk_offset_box();
  } while (0);
  return ret;
}

bool AMMuxerAv3Builder::fill_meta_sample_to_chunk_box()
{
  bool ret = true;
  AV3SampleToChunkBox& box = m_movie_box.m_video_track.m_uuid.\
      m_psmt_box.m_meta_sample_table.m_sample_to_chunk;
  box.m_full_box.m_base_box.m_enable = true;
  do {
    box.m_full_box.m_base_box.m_type = AV3_SAMPLE_TO_CHUNK_BOX_TAG;
    box.m_full_box.m_version = m_used_version;
    box.m_full_box.m_flags = m_flags;
  } while (0);
  return ret;
}

void AMMuxerAv3Builder::fill_meta_sample_size_box()
{
  AV3SampleSizeBox& box = m_movie_box.m_video_track.m_uuid.\
      m_psmt_box.m_meta_sample_table.m_sample_size;
  box.m_full_box.m_base_box.m_enable = true;
  box.m_full_box.m_base_box.m_type = AV3_SAMPLE_SIZE_BOX_TAG;
  box.m_full_box.m_version = m_used_version;
  box.m_full_box.m_flags = m_flags;
  box.m_sample_size = 0;
}

void AMMuxerAv3Builder::fill_meta_chunk_offset_box()
{
  AV3ChunkOffsetBox& box = m_movie_box.m_video_track.m_uuid.\
      m_psmt_box.m_meta_sample_table.m_chunk_offset;
  box.m_full_box.m_base_box.m_enable = true;
  box.m_full_box.m_base_box.m_type = AV3_CHUNK_OFFSET_BOX_TAG;
  box.m_full_box.m_version = m_used_version;
  box.m_full_box.m_flags = m_flags;
}


bool AMMuxerAv3Builder::fill_audio_track_uuid_box()
{
  bool ret = true;
  AV3TrackUUIDBox& box = m_movie_box.m_audio_track.m_uuid;
  do {
    box.m_base_box.m_enable = true;
    box.m_base_box.m_type = AV3_UUID_BOX_TAG;
    if (!fill_audio_psmt_box()) {
      ERROR("Failed to fill audio psmt box.");
      ret = false;
      break;
    }
  } while(0);
  return ret;
}

bool AMMuxerAv3Builder::fill_audio_psmt_box()
{
  bool ret = true;
  AV3PSMTBox& box = m_movie_box.m_audio_track.m_uuid.m_psmt_box;
  do {
    box.m_base_box.m_enable = true;
    box.m_base_box.m_type = AV3_PSMT_BOX_TAG;
    if (!fill_audio_psmh_box()) {
      ERROR("Failed to fill audio psmh box.");
      ret = false;
      break;
    }
  } while(0);
  return ret;
}

bool AMMuxerAv3Builder::fill_audio_psmh_box()
{
  bool ret = true;
  AV3PSMHBox& box = m_movie_box.m_audio_track.m_uuid.m_psmt_box.m_psmh_box;
  do {
    box.m_base_box.m_enable = true;
    box.m_base_box.m_type = AV3_PSMH_BOX_TAG;
    box.m_meta_id = 0x01;
  } while(0);
  return ret;
}

bool AMMuxerAv3Builder::fill_video_track_box()
{
  bool ret = true;
  AV3TrackBox& box = m_movie_box.m_video_track;
  do {
    box.m_base_box.m_enable = true;
    box.m_base_box.m_type = AV3_TRACK_BOX_TAG;
    fill_video_track_header_box();
    if(AM_UNLIKELY(!fill_video_media_box())) {
      ERROR("Failed to fill video media box.");
      ret = false;
      break;
    }
    if (!fill_video_track_uuid_box()) {
      ERROR("Failed to fill video track uuid box.");
      ret = false;
      break;
    }
  } while (0);
  return ret;
}

bool AMMuxerAv3Builder::fill_audio_track_box()
{
  bool ret = true;
  AV3TrackBox& box = m_movie_box.m_audio_track;
  do {
    box.m_base_box.m_enable = true;
    box.m_base_box.m_type = AV3_TRACK_BOX_TAG;
    fill_audio_track_header_box();
    if(AM_UNLIKELY(!fill_audio_media_box())) {
      ERROR("Failed to fill audio media box.");
      ret = false;
      break;
    }
    if (!fill_audio_track_uuid_box()) {
      ERROR("Failed to fill audio track uuid box.");
      ret = false;
      break;
    }
  } while (0);
  return ret;
}

bool AMMuxerAv3Builder::fill_movie_box()
{
  bool ret = true;
  AV3MovieBox& box = m_movie_box;
  do {
    box.m_base_box.m_enable = true;
    box.m_base_box.m_type = AV3_MOVIE_BOX_TAG;
    fill_movie_header_box();
    if(m_video_frame_number > 0) {
      if (AM_UNLIKELY(!fill_video_track_box())) {
        ERROR("Failed to fill video track box.");
        ret = false;
        break;
      }
    } else {
      ERROR("The video frame number is zero, there are no video in this file");
    }
    if(m_audio_frame_number > 0) {
      if (AM_UNLIKELY(!fill_audio_track_box())) {
        ERROR("Failed to fill audio track box.");
        ret = false;
        break;
      }
    } else {
      NOTICE("The audio frame number is zero, there are no audio in this file.");
    }
  } while (0);
  return ret;
}

bool AMMuxerAv3Builder::fill_uuid_box()
{
  bool ret = true;
  AV3UUIDBox& box = m_uuid_box;
  do {
    box.m_base_box.m_enable = true;
    box.m_base_box.m_type = AV3_UUID_BOX_TAG;
    box.m_ip_address = 0;
    box.m_num_event_log = 0;
    if (!fill_psfm_box()) {
      ERROR("Failed to fill psfm box.");
      ret = false;
      break;
    }
    if (!fill_xml_box()) {
      ERROR("Failed to fill xml box.");
      ret = false;
      break;
    }
    box.m_base_box.m_size = 48 + box.m_psfm_box.m_base_box.m_size +
        box.m_xml_box.m_full_box.m_base_box.m_size;
  } while(0);
  return ret;
}

bool AMMuxerAv3Builder::fill_psfm_box()
{
  bool ret = true;
  AV3PSFMBox& box = m_uuid_box.m_psfm_box;
  do {
    box.m_base_box.m_enable = true;
    box.m_base_box.m_size = 12;
    box.m_base_box.m_type = AV3_PSFM_BOX_TAG;
    box.m_reserved = 0;
  } while(0);
  return ret;
}

bool AMMuxerAv3Builder::fill_xml_box()
{
  bool ret = true;
  AV3XMLBox& box = m_uuid_box.m_xml_box;
  do {
    box.m_full_box.m_base_box.m_enable = true;
    box.m_full_box.m_base_box.m_size = 12;
    box.m_full_box.m_base_box.m_type = AV3_XML_BOX_TAG;
  } while(0);
  return ret;
}

bool AMMuxerAv3Builder::insert_video_decoding_time_to_sample_box(
         int64_t delta_pts)
{
  bool ret = true;
  AV3DecodingTimeToSampleBox& box = m_movie_box.m_video_track.\
        m_media.m_media_info.m_sample_table.m_stts;
  do{
    if(m_video_frame_number == 1) {
      box.m_entry_count = 0;
    }
    if(m_video_frame_number >= box.m_entry_buf_count) {
      AV3TimeEntry* tmp = new AV3TimeEntry[box.m_entry_buf_count +
                                           AV3_VIDEO_FRAME_COUNT];
      if(AM_UNLIKELY(!tmp)) {
        ERROR("Failed to malloc memory.");
        ret = false;
        break;
      }
      if (box.m_entry_count > 0) {
        memcpy(tmp, box.m_entry_ptr, box.m_entry_count * sizeof(AV3TimeEntry));
      }
      delete[] box.m_entry_ptr;
      box.m_entry_ptr = tmp;
      box.m_entry_buf_count += AV3_VIDEO_FRAME_COUNT;
    }
    box.m_entry_ptr[box.m_entry_count].sample_count = 1;
    box.m_entry_ptr[box.m_entry_count].sample_delta = (int32_t)delta_pts;
    ++ box.m_entry_count;
  }while(0);
  return ret;
}

bool AMMuxerAv3Builder::insert_audio_decoding_time_to_sample_box(
    int64_t delta_pts)
{
  bool ret = true;
  AV3DecodingTimeToSampleBox& box = m_movie_box.m_audio_track.\
      m_media.m_media_info.m_sample_table.m_stts;
  do{
    if(m_audio_frame_number == 1) {
      box.m_entry_count = 0;
    }
    if(m_audio_frame_number >= box.m_entry_buf_count) {
      AV3TimeEntry* tmp = new AV3TimeEntry[box.m_entry_buf_count +
                                           AV3_AUDIO_FRAME_COUNT];
      if(AM_UNLIKELY(!tmp)) {
        ERROR("Failed to malloc memory.");
        ret = false;
        break;
      }
      if (box.m_entry_count > 0) {
        memcpy(tmp, box.m_entry_ptr, box.m_entry_count * sizeof(AV3TimeEntry));
      }
      delete[] box.m_entry_ptr;
      box.m_entry_ptr = tmp;
      box.m_entry_buf_count += AV3_AUDIO_FRAME_COUNT;
    }
    box.m_entry_ptr[box.m_entry_count].sample_count = 1;
    box.m_entry_ptr[box.m_entry_count].sample_delta = delta_pts * 48000 / 90000;
    ++ box.m_entry_count;
  }while(0);
  return ret;
}

bool AMMuxerAv3Builder::insert_video_chunk_offset32_box(uint64_t offset)
{
  bool ret = true;
  AV3ChunkOffsetBox& box =
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
    uint32_t* ptmp = new uint32_t[box.m_buf_count + AV3_VIDEO_FRAME_COUNT];
    if (ptmp) {
      if (box.m_entry_count > 0) {
        memcpy(ptmp, box.m_chunk_offset, box.m_entry_count * sizeof(uint32_t));
      }
      delete[] box.m_chunk_offset;
      box.m_buf_count += AV3_VIDEO_FRAME_COUNT;
      box.m_chunk_offset = ptmp;
      box.m_chunk_offset[box.m_entry_count] = offset;
      box.m_entry_count ++;
    } else {
      ERROR("no memory\n");
      ret = false;
    }
  }
  return ret;
}

bool AMMuxerAv3Builder::insert_audio_chunk_offset32_box(uint64_t offset)
{
  bool ret = true;
  AV3ChunkOffsetBox& box = m_movie_box.m_audio_track.m_media.m_media_info.\
                               m_sample_table.m_chunk_offset;
  DEBUG("insert audio chunk offset box, offset = %llu, frame count = %u",
        offset, box.m_entry_count);
  if (m_audio_frame_number == 1) {
    box.m_entry_count = 0;
  }
  if (box.m_entry_count < box.m_buf_count) {
    box.m_chunk_offset[box.m_entry_count] = offset;
    box.m_entry_count ++;
  } else {
    uint32_t *ptmp = new uint32_t[box.m_buf_count + AV3_AUDIO_FRAME_COUNT];
    if (ptmp) {
      if (box.m_entry_count > 0) {
        memcpy(ptmp, box.m_chunk_offset, box.m_entry_count * sizeof(uint32_t));
      }
      delete[] box.m_chunk_offset;
      box.m_buf_count += AV3_AUDIO_FRAME_COUNT;
      box.m_chunk_offset = ptmp;
      box.m_chunk_offset[box.m_entry_count] = offset;
      box.m_entry_count ++;
    } else {
      ERROR("no memory\n");
      ret = false;
    }
  }
  return ret;
}

bool AMMuxerAv3Builder::insert_meta_chunk_offset_box(uint64_t offset)
{
  bool ret = true;
  AV3ChunkOffsetBox& box = m_movie_box.m_video_track.m_uuid.\
      m_psmt_box.m_meta_sample_table.m_chunk_offset;
  if (m_meta_frame_number == 1) {
    box.m_entry_count = 0;
  }
  if (box.m_entry_count < box.m_buf_count) {
    box.m_chunk_offset[box.m_entry_count] = offset;
    box.m_entry_count ++;
  } else {
    uint32_t* ptmp = new uint32_t[box.m_buf_count + AV3_VIDEO_FRAME_COUNT];
    if (ptmp) {
      if (box.m_entry_count > 0) {
        memcpy(ptmp, box.m_chunk_offset, box.m_entry_count * sizeof(uint32_t));
      }
      delete[] box.m_chunk_offset;
      box.m_buf_count += AV3_VIDEO_FRAME_COUNT;
      box.m_chunk_offset = ptmp;
      box.m_chunk_offset[box.m_entry_count] = offset;
      box.m_entry_count ++;
    } else {
      ERROR("no memory\n");
      ret = false;
    }
  }
  return ret;
}

bool AMMuxerAv3Builder::insert_video_sample_size_box(uint32_t size)
{
  bool ret = true;
  AV3SampleSizeBox& box = m_movie_box.m_video_track.m_media.m_media_info.\
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
                                  AV3_VIDEO_FRAME_COUNT];
    if (ptmp) {
      if (box.m_sample_count > 0) {
        memcpy(ptmp, box.m_size_array, box.m_sample_count * sizeof(uint32_t));
      }
      delete[] box.m_size_array;
      box.m_buf_count += AV3_VIDEO_FRAME_COUNT;
      box.m_size_array = ptmp;
      box.m_size_array[box.m_sample_count] = (uint32_t) size;
      ++ box.m_sample_count;
    } else {
      ERROR("no memory\n");
      ret = false;
    }
  }
  return ret;
}

bool AMMuxerAv3Builder::insert_audio_sample_size_box(uint32_t size)
{
  bool ret = true;
  AV3SampleSizeBox& box = m_movie_box.m_audio_track.m_media.m_media_info.\
      m_sample_table.m_sample_size;
  DEBUG("Insert audio sample size box, size = %u", size);
  if(m_audio_frame_number == 1) {
    box.m_sample_count = 0;
  }
  if (box.m_sample_count < box.m_buf_count) {
    box.m_size_array[box.m_sample_count] = (uint32_t) size;
    box.m_sample_count ++;
  } else {
    uint32_t *ptmp = new uint32_t[box.m_buf_count +
                                  AV3_AUDIO_FRAME_COUNT];
    if (ptmp) {
      if (box.m_sample_count > 0) {
        memcpy(ptmp, box.m_size_array, box.m_sample_count * sizeof(uint32_t));
      }
      delete[] box.m_size_array;
      box.m_buf_count += AV3_AUDIO_FRAME_COUNT;
      box.m_size_array = ptmp;
      box.m_size_array[box.m_sample_count] = (uint32_t) size;
      box.m_sample_count ++;
    } else {
      ERROR("no memory\n");
      ret = false;
    }
  }
  return ret;
}

bool AMMuxerAv3Builder::insert_meta_sample_size_box(uint32_t size)
{
  bool ret = true;
  AV3SampleSizeBox& box = m_movie_box.m_video_track.m_uuid.\
      m_psmt_box.m_meta_sample_table.m_sample_size;
  DEBUG("Insert meta sample size box, size = %u", size);
  if(m_meta_frame_number == 1) {
    box.m_sample_count = 0;
  }
  if (box.m_sample_count < box.m_buf_count) {
    box.m_size_array[box.m_sample_count] = (uint32_t) size;
    box.m_sample_count ++;
  } else {
    uint32_t* ptmp = new uint32_t[box.m_buf_count +
                                  AV3_VIDEO_FRAME_COUNT];
    if (ptmp) {
      if (box.m_sample_count > 0) {
        memcpy(ptmp, box.m_size_array, box.m_sample_count * sizeof(uint32_t));
      }
      delete[] box.m_size_array;
      box.m_buf_count += AV3_VIDEO_FRAME_COUNT;
      box.m_size_array = ptmp;
      box.m_size_array[box.m_sample_count] = (uint32_t) size;
      ++ box.m_sample_count;
    } else {
      ERROR("no memory\n");
      ret = false;
    }
  }
  return ret;
}

bool AMMuxerAv3Builder::insert_video_sync_sample_box(
    uint32_t video_frame_number)
{
  bool ret = true;
  AV3SyncSampleTableBox& box = m_movie_box.m_video_track.m_media.\
      m_media_info.m_sample_table.m_sync_sample;
  do {
    if(m_video_frame_number == 1) {
      box.m_stss_count = 0;
    }
    if (box.m_stss_count >= box.m_buf_count) {
      uint32_t *tmp = new uint32_t[box.m_buf_count + AV3_VIDEO_FRAME_COUNT];
      if (AM_UNLIKELY(!tmp)) {
        ERROR("malloc memory error.");
        ret = false;
        break;
      }
      if(box.m_stss_count > 0) {
        memcpy(tmp, box.m_sync_sample_table, box.m_stss_count * sizeof(uint32_t));
      }
      delete[] box.m_sync_sample_table;
      box.m_sync_sample_table = tmp;
      box.m_buf_count += AV3_VIDEO_FRAME_COUNT;
    }
    box.m_sync_sample_table[box.m_stss_count] = video_frame_number;
    ++ box.m_stss_count;
  } while (0);
  return ret;
}

bool AMMuxerAv3Builder::insert_video_sample_to_chunk_box(bool new_chunk)
{
  bool ret = true;
  AV3SampleToChunkBox& box = m_movie_box.m_video_track.m_media.m_media_info.\
      m_sample_table.m_sample_to_chunk;
  do {
    if (m_video_frame_number == 1) {
      box.m_entry_count = 0;
    }
    if (new_chunk) {
      if (box.m_entry_count >= box.m_entry_buf_count) {
        AV3SampleToChunkEntry *tmp =
       new AV3SampleToChunkEntry[box.m_entry_buf_count + AV3_VIDEO_FRAME_COUNT];
        if (!tmp) {
          ERROR("Failed to malloc memory for sample to chunk entry");
          ret = false;
          break;
        }
        if (box.m_entry_count > 0) {
          memcpy(tmp, box.m_entrys, box.m_entry_count * sizeof(AV3SampleToChunkBox));
        }
        delete[] box.m_entrys;
        box.m_entrys = tmp;
        box.m_entry_buf_count += AV3_VIDEO_FRAME_COUNT;
      }
      box.m_entrys[box.m_entry_count].m_first_chunk = box.m_entry_count + 1;
      box.m_entrys[box.m_entry_count].m_sample_per_chunk = 1;
      box.m_entrys[box.m_entry_count].m_sample_description_index = 1;
      ++ box.m_entry_count;
    } else {
      if (box.m_entry_count < 1) {
        ERROR("Entry count of sample to chunk box error.");
        ret = false;
        break;
      }
      box.m_entrys[box.m_entry_count - 1].m_sample_per_chunk += 1;
    }
  } while(0);
  return ret;
}

bool AMMuxerAv3Builder::insert_audio_sample_to_chunk_box(bool new_chunk)
{
  bool ret = true;
  AV3SampleToChunkBox& box = m_movie_box.m_audio_track.m_media.m_media_info.\
      m_sample_table.m_sample_to_chunk;
  do {
    if (m_audio_frame_number == 1) {
      box.m_entry_count = 0;
    }
    if (new_chunk) {
      if (box.m_entry_count >= box.m_entry_buf_count) {
        AV3SampleToChunkEntry *tmp =
            new AV3SampleToChunkEntry[box.m_entry_buf_count + AV3_AUDIO_FRAME_COUNT];
        if (!tmp) {
          ERROR("Failed to malloc memory for sample to chunk entry");
          ret = false;
          break;
        }
        if (box.m_entry_count > 0) {
          memcpy(tmp, box.m_entrys, box.m_entry_count * sizeof(AV3SampleToChunkBox));
        }
        delete[] box.m_entrys;
        box.m_entrys = tmp;
        box.m_entry_buf_count += AV3_AUDIO_FRAME_COUNT;
      }
      box.m_entrys[box.m_entry_count].m_first_chunk = box.m_entry_count + 1;
      box.m_entrys[box.m_entry_count].m_sample_per_chunk = 1;
      box.m_entrys[box.m_entry_count].m_sample_description_index = 1;
      ++ box.m_entry_count;
    } else {
      if (box.m_entry_count < 1) {
        ERROR("Entry count of sample to chunk box error, entry count is %u.",
              box.m_entry_count);
        ret = false;
        break;
      }
      box.m_entrys[box.m_entry_count - 1].m_sample_per_chunk += 1;
    }
  } while(0);
  return ret;
}

bool AMMuxerAv3Builder::insert_meta_sample_to_chunk_box(bool new_chunk)
{
  bool ret = true;
  AV3SampleToChunkBox& box = m_movie_box.m_video_track.m_uuid.\
      m_psmt_box.m_meta_sample_table.m_sample_to_chunk;
  do {
    if (m_meta_frame_number == 1) {
      box.m_entry_count = 0;
    }
    if (new_chunk) {
      if (box.m_entry_count >= box.m_entry_buf_count) {
        AV3SampleToChunkEntry *tmp =
       new AV3SampleToChunkEntry[box.m_entry_buf_count + AV3_VIDEO_FRAME_COUNT];
        if (!tmp) {
          ERROR("Failed to malloc memory for sample to chunk entry");
          ret = false;
          break;
        }
        if (box.m_entry_count > 0) {
          memcpy(tmp, box.m_entrys, box.m_entry_count * sizeof(AV3SampleToChunkBox));
        }
        delete[] box.m_entrys;
        box.m_entrys = tmp;
        box.m_entry_buf_count += AV3_VIDEO_FRAME_COUNT;
      }
      box.m_entrys[box.m_entry_count].m_first_chunk = box.m_entry_count + 1;
      box.m_entrys[box.m_entry_count].m_sample_per_chunk = 1;
      box.m_entrys[box.m_entry_count].m_sample_description_index = 1;
      ++ box.m_entry_count;
    } else {
      if (box.m_entry_count < 1) {
        ERROR("Entry count of sample to chunk box error.");
        ret = false;
        break;
      }
      box.m_entrys[box.m_entry_count - 1].m_sample_per_chunk += 1;
    }
  } while(0);
  return ret;
}

bool AMMuxerAv3Builder::insert_video_psmh_box(uint32_t sample_num, int32_t sec,
                           int16_t msec, int64_t time_stamp)
{
  bool ret = true;
  AV3PSMHBox& box = m_movie_box.m_video_track.m_uuid.m_psmt_box.m_psmh_box;
  do {
    if (m_video_frame_number == 1) {
      box.m_entry_count = 0;
    }
    if (box.m_entry_count >= box.m_entry_buf_cnt) {
      AV3MetaDataEntry* tmp =
          new AV3MetaDataEntry[box.m_entry_buf_cnt + AV3_VIDEO_FRAME_COUNT];
      if (!tmp) {
        ERROR("Failed to malloc memory for psmh box.");
        ret = false;
        break;
      }
      if (box.m_entry_count > 0) {
        memcpy(tmp, box.m_entry, box.m_entry_count * sizeof(AV3MetaDataEntry));
      }
      delete[] box.m_entry;
      box.m_entry = tmp;
      box.m_entry_buf_cnt += AV3_VIDEO_FRAME_COUNT;
    }
    box.m_entry[box.m_entry_count].m_sample_number = sample_num;
    box.m_entry[box.m_entry_count].m_sec = sec;
    box.m_entry[box.m_entry_count].m_msec = msec;
    box.m_entry[box.m_entry_count].m_time_stamp = (int32_t)time_stamp;
    ++ box.m_entry_count;
  } while(0);
  return ret;
}

bool AMMuxerAv3Builder::insert_audio_psmh_box(uint32_t sample_num, int32_t sec,
                           int16_t msec, int64_t time_stamp)
{
  bool ret = true;
  AV3PSMHBox& box = m_movie_box.m_audio_track.m_uuid.m_psmt_box.m_psmh_box;
  do {
    if (m_audio_frame_number == 1) {
      box.m_entry_count = 0;
    }
    if (box.m_entry_count >= box.m_entry_buf_cnt) {
      AV3MetaDataEntry* tmp =
          new AV3MetaDataEntry[box.m_entry_buf_cnt + AV3_AUDIO_FRAME_COUNT];
      if (!tmp) {
        ERROR("Failed to malloc memory for psmh box.");
        ret = false;
        break;
      }
      if (box.m_entry_count > 0) {
        memcpy(tmp, box.m_entry, box.m_entry_count * sizeof(AV3MetaDataEntry));
      }
      delete[] box.m_entry;
      box.m_entry = tmp;
      box.m_entry_buf_cnt += AV3_AUDIO_FRAME_COUNT;
    }
    box.m_entry[box.m_entry_count].m_sample_number = sample_num;
    box.m_entry[box.m_entry_count].m_sec = sec;
    box.m_entry[box.m_entry_count].m_msec = msec;
    box.m_entry[box.m_entry_count].m_time_stamp = (int32_t)time_stamp;
    ++ box.m_entry_count;
  } while(0);
  return ret;
}

bool AMMuxerAv3Builder::insert_video_frame_info(int64_t delta_pts,
                          uint64_t offset, uint64_t size, uint8_t sync_sample)
{
  bool ret = true;
  DEBUG("write video frame info, offset = %llu, size = %llu", offset, size);
  do {
    if (AM_UNLIKELY(!insert_video_decoding_time_to_sample_box(
        delta_pts))) {
      ERROR("Failed to insert video decoding time to sample box.");
      ret = false;
      break;
    }
    if (sync_sample) {
      if (AM_UNLIKELY(!insert_video_chunk_offset32_box(offset))) {
        ERROR("Failed to insert video chunk offset box.");
        ret = false;
        break;
      }
    }
    bool new_chunk = (sync_sample > 0);
    if (!insert_video_sample_to_chunk_box(new_chunk)) {
      ERROR("Failed to insert video sample to chunk box.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!insert_video_sample_size_box(size))) {
      ERROR("Failed to insert video sample size box.");
      ret = false;
      break;
    }
    int32_t msec = 0;
    int32_t sec = 0;
    if (m_video_frame_number == 1) {
      m_time_val.tv_sec = 0;
      m_time_val.tv_usec = 0;
      m_video_delta_pts_sum = delta_pts;
      if (gettimeofday(&m_time_val, nullptr) < 0) {
        ERROR("Failed to gettimeofday.");
      }
      sec = m_time_val.tv_sec;
      msec = m_time_val.tv_usec / 1000;
    } else {
      msec = ((m_time_val.tv_usec +
          m_video_delta_pts_sum * 1000000 / 90000) / 1000) % 1000;
      sec = m_time_val.tv_sec + ((m_time_val.tv_usec +
          m_video_delta_pts_sum * 1000000 / 90000) / 1000) / 1000;
      m_video_delta_pts_sum += delta_pts;
    }
    if (!insert_video_psmh_box(m_video_frame_number - 1, sec, msec,
                               m_video_delta_pts_sum - delta_pts)) {
      ERROR("Failed to insert video psmh box.");
      ret = false;
      break;
    }
    if (sync_sample) {
      if (AM_UNLIKELY(!insert_video_sync_sample_box(m_video_frame_number))) {
        ERROR("Failed to insert video sync sample box.");
        ret = false;
        break;
      }
    }
  } while (0);
  return ret;
}

bool AMMuxerAv3Builder::insert_audio_frame_info(int64_t delta_pts,
                                                uint64_t offset,
                                                uint64_t size,
                                                bool new_chunk)
{
  bool ret = true;
  DEBUG("write audio frame info, offset = %llu, size = %llu", offset, size);
  do {
    if (new_chunk) {
      if (AM_UNLIKELY(!insert_audio_chunk_offset32_box(offset))) {
        ERROR("Failed to insert audio chunk offset box.");
        ret = false;
        break;
      }
    }
    if (!insert_audio_sample_to_chunk_box(new_chunk)) {
      ERROR("Failed to insert audio sample to chunk box.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!insert_audio_sample_size_box(size))) {
      ERROR("Failed to insert audio sample size box.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!insert_audio_decoding_time_to_sample_box(delta_pts))) {
      ERROR("Failed to insert audio decoding time to sample box.");
      ret = false;
      break;
    }

    int32_t msec = 0;
    int32_t sec = 0;
    if (m_audio_frame_number == 1) {
      m_audio_delta_pts_sum = delta_pts;
    } else {
      m_audio_delta_pts_sum += delta_pts;
    }
    msec = ((m_time_val.tv_usec +
        m_audio_delta_pts_sum * 1000000 / 90000) / 1000) % 1000;
    sec = m_time_val.tv_sec + ((m_time_val.tv_usec +
        m_audio_delta_pts_sum * 1000000 / 90000) / 1000) / 1000;
    if (!insert_audio_psmh_box(m_audio_frame_number - 1, sec, msec,
                       (m_audio_delta_pts_sum - delta_pts) * 48000 / 90000)) {
      ERROR("Failed to insert audio psmh box.");
      ret = false;
      break;
    }
  } while (0);
  return ret;
}

bool AMMuxerAv3Builder::insert_meta_frame_info(uint64_t offset,
                                               uint64_t size, bool new_chunk)
{
  bool ret = true;
  DEBUG("write meta frame info, offset = %llu, size = %llu, new chunk is %s",
       offset, size, new_chunk ? "true" : "false");
  do {
    if (new_chunk) {
      if (AM_UNLIKELY(!insert_meta_chunk_offset_box(offset))) {
        ERROR("Failed to insert meta chunk offset box.");
        ret = false;
        break;
      }
    }
    if (!insert_meta_sample_to_chunk_box(new_chunk)) {
      ERROR("Failed to insert meta sample to chunk box.");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!insert_meta_sample_size_box(size))) {
      ERROR("Failed to insert meta sample size box.");
      ret = false;
      break;
    }
  } while (0);
  return ret;
}

AM_STATE AMMuxerAv3Builder::update_media_box_size()
{
  AM_STATE ret = AM_STATE_OK;
  uint32_t offset = m_file_type_box.m_base_box.m_size;
  do {
    m_media_data_box.m_base_box.m_size += (uint32_t) m_overall_media_data_len;
    uint32_t& size = m_media_data_box.m_base_box.m_size;
    if ((size % 4) != 0) {
      uint32_t padding_count = 4 - size %4;
      size += padding_count;
      for (uint32_t i = 0; i < padding_count; ++ i) {
        if (AM_UNLIKELY(m_file_writer->write_u8(0) != AM_STATE_OK)) {
          ERROR("Failed to write padding data to media data box.");
          ret = AM_STATE_ERROR;
          break;
        }
        if (m_config->digital_sig_enable) {
          uint8_t data = 0;
          if (!m_hash_sig->signature_update_data(&data, sizeof(uint8_t))) {
            ERROR("Failed to signature update data.");
            ret = AM_STATE_ERROR;
            break;
          }
        }
      }
      if (ret != AM_STATE_OK) {
        break;
      }
    }
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
    offset = m_file_type_box.m_base_box.m_size +
        m_media_data_box.m_base_box.m_size;
    if (m_file_writer->seek_data(offset, SEEK_SET) != AM_STATE_OK) {
      ERROR("Failed to seek data.");
      ret = AM_STATE_ERROR;
      break;
    }
  } while (0);
  return ret;
}

AM_STATE AMMuxerAv3Builder::begin_file()
{
  AM_STATE ret = AM_STATE_OK;
  do {
    m_write_media_data_started = 0;
    m_overall_media_data_len = 0;
    m_video_duration = 0;
    m_audio_duration = 0;
    m_last_video_pts = 0;
    m_first_video_pts = 0;
    m_last_audio_pts = 0;
    m_video_frame_number = 0;
    m_audio_frame_number = 0;
    m_meta_frame_number = 0;
    m_audio_frame_count_per_packet = 0;
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
    if (m_config->digital_sig_enable) {
      if (!m_hash_sig->init_signature()) {
        ERROR("Failed to init signature.");
        ret = AM_STATE_ERROR;
        break;
      }
    }
    if (AM_UNLIKELY((ret = m_file_writer->write_file_type_box(m_file_type_box))
                     != AM_STATE_OK)) {
      ERROR("Failed to write file type box.");
      break;
    }
    if (AM_UNLIKELY((ret = m_file_writer->write_media_data_box(
        m_media_data_box)) != AM_STATE_OK)) {
      ERROR("Failed to write media data box.");
      break;
    }
    if (m_config->reconstruct_enable) {
      char* file_name = m_file_writer->get_current_file_name();
      if(!m_index_writer->set_AV3_file_name(file_name,
                                            (uint32_t)strlen(file_name))) {
        ERROR("Failed to set AV3 file name.");
      }
      if (!m_index_writer->open_file()) {
        ERROR("Failed to open AV3 index file.");
      }
    }
  } while (0);
  return ret;
}

AM_STATE AMMuxerAv3Builder::end_file()
{
  AM_STATE ret = AM_STATE_OK;
  do {
    if (m_video_frame_number > 0) {
      m_video_track_id = m_next_track_id ++;
    }
    if (m_audio_frame_number > 0) {
      m_audio_track_id = m_next_track_id ++;
    }
    if (m_config->reconstruct_enable) {
      m_index_writer->close_file(true);
    }
    if (AM_LIKELY(m_write_media_data_started)) {
      m_write_media_data_started = 0;
      if (AM_UNLIKELY((ret = update_media_box_size()) != AM_STATE_OK)) {
        ERROR("Failed to end file");
        break;
      }
      if (AM_UNLIKELY(!fill_movie_box())) {
        ERROR("Failed to fill movie box.");
        ret = AM_STATE_ERROR;
        break;
      }
      m_movie_box.update_movie_box_size();
      if (AM_UNLIKELY((ret = m_file_writer->write_movie_box(
          m_movie_box, m_config->digital_sig_enable)) != AM_STATE_OK)) {
        ERROR("Failed to write tail.");
        break;
      }
      if (!fill_uuid_box()) {
        ERROR("Failed to fill uuid box.");
        ret = AM_STATE_ERROR;
        break;
      }
      INFO("m_uuid_box length is %u", m_uuid_box.m_base_box.m_size);
      if ((ret = m_file_writer->write_uuid_box(m_uuid_box,
                                m_config->digital_sig_enable)) != AM_STATE_OK) {
        ERROR("Failed to write uuid box.");
        break;
      }
      if (m_config->digital_sig_enable) {
        const uint8_t *movie_uuid_data = m_file_writer->get_data_buffer();
        uint32_t movie_uuid_data_len = m_file_writer->get_data_size();
        if (!m_hash_sig->signature_update_data(movie_uuid_data,
                                               movie_uuid_data_len)) {
          ERROR("Signature update data error.");
          ret = AM_STATE_ERROR;
          break;
        }
        if (!m_hash_sig->signature_final()) {
          ERROR("Signature final error.");
          ret = AM_STATE_ERROR;
          break;
        }

        uint32_t hash_pos = m_file_type_box.m_base_box.m_size + 12;
        if (m_file_writer->seek_data(hash_pos, SEEK_SET) != AM_STATE_OK) {
          ERROR("Failed to seek data.");
          ret = AM_STATE_ERROR;
          break;
        }
        const uint8_t *hash_data = nullptr;
        uint32_t hash_len = 0;
        hash_data = m_hash_sig->get_signature(hash_len);
        INFO("hash len is %u", hash_len);
        if (m_file_writer->write_data(hash_data, hash_len) != AM_STATE_OK) {
          ERROR("Failed to write data.");
          ret = AM_STATE_ERROR;
          break;
        }
        m_hash_sig->clean_md_ctx();
      } else {
        NOTICE("The digital signature is not enabled.");
      }
      m_file_writer->clear_buffer();
    }
  } while (0);
  return ret;
}
