/*******************************************************************************
 * am_muxer_mp4_index_writer.cpp
 *
 * History:
 *   2015-12-9 - [ccjing] created file
 *
 * Copyright (c) 2016 Ambarella, Inc.
 *
 * This file and its contents (“Software”) are protected by intellectual
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
#include "am_amf_types.h"
#include "am_log.h"
#include "am_define.h"

#include "am_audio_define.h"
#include "am_video_types.h"
#include "am_muxer_mp4_index_writer.h"
AMMp4IndexWriter* AMMp4IndexWriter::create(uint32_t write_frequency, bool event)
{
  AMMp4IndexWriter* result = new AMMp4IndexWriter();
  if (AM_UNLIKELY(result && (!result->init(write_frequency, event)))) {
    delete result;
    result = NULL;
  }
  return result;
}

AMMp4IndexWriter::AMMp4IndexWriter() :
    m_fd(-1),
    m_write_frequency(0),
    m_vps_size(0),
    m_sps_size(0),
    m_pps_size(0),
    m_video_info(nullptr),
    m_audio_info(nullptr),
    m_vps(nullptr),
    m_sps(nullptr),
    m_pps(nullptr),
    m_index_buf(nullptr),
    m_buf_offset(0),
    m_frame_count(0),
    m_info_map(0),
    m_param_map(0),
    m_event(false)
{
  memset(m_file_name, 0, 128);
}

AMMp4IndexWriter::~AMMp4IndexWriter()
{
  delete m_video_info;
  delete m_audio_info;
  delete[] m_vps;
  delete[] m_sps;
  delete[] m_pps;
  delete[] m_index_buf;
}

bool AMMp4IndexWriter::init(uint32_t write_frequency, bool event)
{
  bool ret = true;
  do {
    m_write_frequency = write_frequency;
    m_event = event;
    m_index_buf = new uint8_t[write_frequency * 64 + 512];
    if (!m_index_buf) {
      ERROR("Faield to malloc memory when init mp4 index writer.");
      ret = false;
      break;
    }
  } while (0);
  return ret;
}

void AMMp4IndexWriter::destroy()
{
  delete this;
}

bool AMMp4IndexWriter::open_file()
{
  bool ret = true;
  do {
    std::string file_name = "/.mp4_index";
    if (m_event) {
      file_name += "_event";
    }
    m_fd = open(file_name.c_str(), O_WRONLY | O_CREAT | O_TRUNC);
    if (m_fd < 0) {
      ERROR("Failed to open mp4 index file.");
      ret = false;
      break;
    }
    memcpy(m_index_buf, m_file_name, (uint32_t)strlen(m_file_name));
    m_buf_offset += strlen(m_file_name);
    memcpy(m_index_buf + m_buf_offset, "\n", 1);
    m_buf_offset += 1;
    m_frame_count = m_write_frequency;
  } while (0);
  m_info_map = 0;
  m_param_map = 0;
  return ret;
}

void AMMp4IndexWriter::close_file(bool need_sync)
{
  int32_t result = -1;
  uint8_t *data_addr = m_index_buf;
  while (m_buf_offset > 0) {
    if ((result = write(m_fd, data_addr, m_buf_offset)) > 0) {
      m_buf_offset -= result;
      data_addr += result;
    } else {
      ERROR("Write file error!");
      break;
    }
  }
  if (need_sync) {
    fsync(m_fd);
  }
  close(m_fd);
  m_fd = -1;
}

bool AMMp4IndexWriter::set_mp4_file_name(char *file_name, uint32_t length)
{
  bool ret = true;
  do {
    if(length > 128) {
      ERROR("The length of file name is too long.");
      ret = false;
      break;
    } else {
      memset(m_file_name, 0, 128);
      memcpy(m_file_name, file_name, length);
    }
  } while(0);
  return ret;
}

bool AMMp4IndexWriter::set_video_info(AM_VIDEO_INFO *video_info)
{
  bool ret = true;
  do {
    if (!m_video_info) {
      if ((m_video_info = new AM_VIDEO_INFO()) == nullptr) {
        ERROR("Failed to malloc memory when set video info");
        ret = false;
        break;
      }
      memcpy(m_video_info, video_info, sizeof(AM_VIDEO_INFO));
    } else if (memcmp(m_video_info, video_info, sizeof(AM_VIDEO_INFO)) != 0) {
      NOTICE("The video info value is changed, reset it.");
      memcpy(m_video_info, video_info, sizeof(AM_VIDEO_INFO));
    } else {
      INFO("The video info have been set and the value is not changed.");
    }
  } while (0);
  return ret;
}

bool AMMp4IndexWriter::set_audio_info(AM_AUDIO_INFO *audio_info)
{
  bool ret = true;
  do {
    if (!m_audio_info) {
      if ((m_audio_info = new AM_AUDIO_INFO()) == nullptr) {
        ERROR("Failed to malloc memory when set audio info");
        ret = false;
        break;
      }
      memcpy(m_audio_info, audio_info, sizeof(AM_AUDIO_INFO));
    } else if (memcmp(m_audio_info, audio_info, sizeof(AM_AUDIO_INFO)) != 0) {
      NOTICE("The audio info have been changed, reset it.");
      memcpy(m_audio_info, audio_info, sizeof(AM_AUDIO_INFO));
    } else {
      INFO("The audio info have been set.");
    }
  } while (0);
  return ret;
}

bool AMMp4IndexWriter::set_vps(uint8_t *vps, uint32_t size)
{
  bool ret = true;
  do {
    if (!m_vps) {
      if ((m_vps = new uint8_t[size]) == nullptr) {
        ERROR("Failed to malloc memory for vps.");
        ret = false;
        break;
      }
      m_vps_size = size;
      memcpy(m_vps, vps, size);
    } else if ((m_vps_size != size) || (memcmp(m_vps, vps, size) != 0)) {
      if (m_vps_size < size) {
        delete[] m_vps;
        if ((m_vps = new uint8_t[size]) == nullptr) {
          ERROR("Failed to malloc memory for vps.");
          ret = false;
          break;
        }
      }
      memcpy(m_vps, vps, size);
      m_vps_size = size;
    }
  } while (0);
  return ret;
}

bool AMMp4IndexWriter::set_sps(uint8_t *sps, uint32_t size)
{
  bool ret = true;
  do {
    if (!m_sps) {
      if ((m_sps = new uint8_t[size]) == nullptr) {
        ERROR("Failed to malloc memory for sps.");
        ret = false;
        break;
      }
      m_sps_size = size;
      memcpy(m_sps, sps, size);
    } else if ((m_sps_size != size) || (memcmp(m_sps, sps, size) != 0)) {
      if (m_sps_size < size) {
        delete[] m_sps;
        if ((m_sps = new uint8_t[size]) == nullptr) {
          ERROR("Failed to malloc memory for sps.");
          ret = false;
          break;
        }
      }
      memcpy(m_sps, sps, size);
      m_sps_size = size;
    }
  } while (0);
  return ret;
}

bool AMMp4IndexWriter::set_pps(uint8_t *pps, uint32_t size)
{
  bool ret = true;
  do {
    if (!m_pps) {
      if ((m_pps = new uint8_t[size]) == nullptr) {
        ERROR("Failed to malloc memory for pps.");
        ret = false;
        break;
      }
      m_pps_size = size;
      memcpy(m_pps, pps, size);
    } else if ((m_pps_size != size) || (memcmp(m_pps, pps, size) != 0)) {
      if (m_pps_size < size) {
        delete[] m_pps;
        if ((m_pps = new uint8_t[size]) == nullptr) {
          ERROR("Failed to malloc memory for pps.");
          ret = false;
          break;
        }
      }
      memcpy(m_pps, pps, size);
      m_pps_size = size;
    }
  } while (0);
  return ret;
}

bool AMMp4IndexWriter::write_video_information(int64_t delta_pts, uint64_t offset,
                                 uint64_t size, uint8_t sync_sample)
{
  bool ret = true;
  do {
    /*write video info into index file*/
    if (((m_info_map & 0x01) == 0) && m_video_info) {
      memcpy(m_index_buf + m_buf_offset, "vinf", 4);
      m_buf_offset += 4;
      memcpy(m_index_buf + m_buf_offset, m_video_info, sizeof(AM_VIDEO_INFO));
      m_buf_offset += sizeof(AM_VIDEO_INFO);
      memcpy(m_index_buf + m_buf_offset, "\n", 1);
      m_buf_offset += 1;
      m_info_map |= 0x01;
      m_frame_count = m_write_frequency;
    }
    /*write parameter sets into index file*/
    if (((m_param_map & 0x01) == 0) && m_vps) {
      memcpy(m_index_buf + m_buf_offset, "vps:", 4);
      m_buf_offset += 4;
      memcpy(m_index_buf + m_buf_offset, m_vps, m_vps_size);
      m_buf_offset += m_vps_size;
      memcpy(m_index_buf + m_buf_offset, "\n", 1);
      m_buf_offset += 1;
      m_param_map |= 0x01;
      m_frame_count = m_write_frequency;
    }
    if (((m_param_map & 0x02) == 0) && m_sps) {
      memcpy(m_index_buf + m_buf_offset, "sps:", 4);
      m_buf_offset += 4;
      memcpy(m_index_buf + m_buf_offset, m_sps, m_sps_size);
      m_buf_offset += m_sps_size;
      memcpy(m_index_buf + m_buf_offset, "\n", 1);
      m_buf_offset += 1;
      m_param_map |= 0x02;
      m_frame_count = m_write_frequency;
    }
    if (((m_param_map & 0x04) == 0) && m_pps) {
      memcpy(m_index_buf + m_buf_offset, "pps:", 4);
      m_buf_offset += 4;
      memcpy(m_index_buf + m_buf_offset, m_pps, m_pps_size);
      m_buf_offset += m_pps_size;
      memcpy(m_index_buf + m_buf_offset, "\n", 1);
      m_buf_offset += 1;
      m_param_map |= 0x04;
      m_frame_count = m_write_frequency;
    }
    /*write frame index into index file*/
    char video_index[36] = { 0 };
    snprintf(video_index, 36, "vind%lld,%llu,%llu,%u\n",
             delta_pts, offset, size, sync_sample);
    memcpy((m_index_buf + m_buf_offset), video_index, strlen(video_index));
    m_buf_offset += strlen(video_index);
    ++ m_frame_count;
    if (m_frame_count >= m_write_frequency) {
      int32_t result = -1;
      uint8_t *data_addr = m_index_buf;
      while (m_buf_offset > 0) {
        if ((result = write(m_fd, data_addr, m_buf_offset)) > 0) {
          m_buf_offset -= result;
          data_addr += result;
        } else {
          ERROR("Write file error!");
          ret = false;
          break;
        }
      }
      m_buf_offset = 0;
      m_frame_count = 0;
    }
  } while(0);
  return ret;
}

bool AMMp4IndexWriter::write_audio_information(uint16_t aac_spec_config,
                                               int64_t delta_pts,
                                               uint32_t packet_frame_count,
                                               uint64_t offset, uint64_t size)
{
  bool ret = true;
  do {
    if (((m_info_map & 0x02) == 0) && m_audio_info) {
      memcpy(m_index_buf + m_buf_offset, "ainf", 4);
      m_buf_offset += 4;
      memcpy(m_index_buf + m_buf_offset, m_audio_info, sizeof(AM_AUDIO_INFO));
      m_buf_offset += sizeof(AM_AUDIO_INFO);
      char audio_param[48] = { 0 };
      snprintf(audio_param, 48, ",pkt_frame_count%u,aac_spec_config%u",
               packet_frame_count, aac_spec_config);
      memcpy(m_index_buf + m_buf_offset, audio_param, strlen(audio_param));
      m_buf_offset += strlen(audio_param);
      memcpy(m_index_buf + m_buf_offset, "\n", 1);
      m_buf_offset += 1;
      m_info_map |= 0x02;
      m_frame_count = m_write_frequency;
    }
    char audio_index[36] = { 0 };
    snprintf(audio_index, 36, "aind%lld,%llu,%llu\n",delta_pts, offset, size);
    memcpy((m_index_buf + m_buf_offset), audio_index, strlen(audio_index));
    m_buf_offset += strlen(audio_index);
    ++ m_frame_count;
    if (m_frame_count >= m_write_frequency) {
      int32_t result = -1;
      uint8_t *data_addr = m_index_buf;
      while (m_buf_offset > 0) {
        if ((result = write(m_fd, data_addr, m_buf_offset)) > 0) {
          m_buf_offset -= result;
          data_addr += result;
        } else {
          ERROR("Write file error!");
          ret = false;
          break;
        }
      }
      m_buf_offset = 0;
      m_frame_count = 0;
    }
  } while (0);
  return ret;
}

bool AMMp4IndexWriter::write_gps_information(int64_t delta_pts,
                                             uint64_t offset, uint64_t size)
{
  bool ret = true;
  do {
    char gps_index[36] = { 0 };
    snprintf(gps_index, 36, "gind%llu,%llu,%llu\n", delta_pts, offset, size);
    memcpy((m_index_buf + m_buf_offset), gps_index, strlen(gps_index));
    m_buf_offset += strlen(gps_index);
    ++ m_frame_count;
    if (m_frame_count >= m_write_frequency) {
      int32_t result = -1;
      uint8_t *data_addr = m_index_buf;
      while (m_buf_offset > 0) {
        if ((result = write(m_fd, data_addr, m_buf_offset)) > 0) {
          m_buf_offset -= result;
          data_addr += result;
        } else {
          ERROR("Write file error!");
          ret = false;
          break;
        }
      }
      m_buf_offset = 0;
      m_frame_count = 0;
    }
  } while (0);
  return ret;
}
