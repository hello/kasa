/*******************************************************************************
 * am_muxer_jpeg_normal.cpp
 *
 * History:
 *   2016-1-27 - [ccjing] created file
 *
 * Copyright (c) 2015 Ambarella, Inc.
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
#include "am_amf_interface.h"
#include "am_amf_packet.h"
#include "am_log.h"
#include "am_define.h"
#include "am_mutex.h"
#include "am_thread.h"
#include "am_file_sink_if.h"
#include "am_muxer_codec_info.h"
#include "am_file.h"
#include <time.h>
#include <unistd.h>
#include <sys/statfs.h>
#include <iostream>
#include <fstream>
#include <ctype.h>
#include "am_muxer_jpeg_file_writer.h"
#include "am_muxer_jpeg_normal.h"

AMIMuxerCodec* get_muxer_codec (const char* config_file)
{
  return (AMIMuxerCodec*)(AMMuxerJpegNormal::create(config_file));
}

AMMuxerJpegNormal* AMMuxerJpegNormal::create(const char* config_name)
{
  AMMuxerJpegNormal* result = new AMMuxerJpegNormal();
  if (result && (result->init(config_name) != AM_STATE_OK)) {
    ERROR("Failed to create AMMuxerJpegNormal.");
    delete result;
    result = nullptr;
  }
  return result;
}

AMMuxerJpegNormal::AMMuxerJpegNormal()
{
}

AMMuxerJpegNormal::~AMMuxerJpegNormal()
{
  INFO("%s has been destroyed.", m_muxer_name.c_str());
}

AM_STATE AMMuxerJpegNormal::init(const char* config_file)
{
  AM_STATE ret = AM_STATE_OK;
  do {
    if(AM_UNLIKELY((ret = inherited::init(config_file)) != AM_STATE_OK)) {
      ERROR("Failed to init AMMuxerJpegBase object");
      break;
    }
    m_muxer_name = "JpegMuxerNormal";
    INFO("Init %s success.", m_muxer_name.c_str());
  } while(0);
  return ret;
}

AM_MUXER_ATTR AMMuxerJpegNormal::get_muxer_attr()
{
  return AM_MUXER_FILE_NORMAL;
}

void AMMuxerJpegNormal::feed_data(AMPacket* packet)
{
  bool add = false;
  do {
    if ((packet->get_packet_type() & AMPacket::AM_PACKET_TYPE_NORMAL) == 0) {
      /*just need normal packet*/
      break;
    }
    if ((packet->get_type() == AMPacket::AM_PAYLOAD_TYPE_INFO) &&
        (packet->get_attr() == AMPacket::AM_PAYLOAD_ATTR_VIDEO) &&
        (packet->get_stream_id() == m_curr_config.video_id)) {
        AM_VIDEO_INFO* video_info = (AM_VIDEO_INFO*)(packet->get_data_ptr());
        m_exif_image_width = video_info->width;
        m_exif_image_height = video_info->height;
        add = true;
    }

    if ((packet->get_type() == AMPacket::AM_PAYLOAD_TYPE_DATA) &&
        (packet->get_attr() == AMPacket::AM_PAYLOAD_ATTR_VIDEO)) {
      AM_VIDEO_FRAME_TYPE vtype =
          AM_VIDEO_FRAME_TYPE(packet->get_frame_type());
      add = (((vtype == AM_VIDEO_FRAME_TYPE_MJPEG) ||
          (vtype == AM_VIDEO_FRAME_TYPE_SJPEG)) &&
          (packet->get_stream_id() == m_curr_config.video_id));
    }
  } while(0);
  if (AM_LIKELY(add)) {
    packet->add_ref();
    m_packet_queue.push_back(packet);
  }
  packet->release();
}

AM_STATE AMMuxerJpegNormal::generate_file_name(char file_name[])
{
  AM_STATE ret = AM_STATE_OK;
  do {
    if (AM_UNLIKELY(!get_proper_file_location(m_file_location))) {
      ERROR("Failed to get proper file location in %s", m_muxer_name.c_str());
      ret = AM_STATE_ERROR;
      break;
    }
    char time_string[32] = { 0 };
    if (AM_UNLIKELY(!(get_current_time_string(time_string,
                                              sizeof(time_string),
                                              FILENAME_TIME_FORMAT)))) {
      ERROR("Get current time string error in %s.", m_muxer_name.c_str());
      ret = AM_STATE_ERROR;
      break;
    }
    sprintf(file_name, "%s/%s_%s_stream%u",
            m_file_location.c_str(),
            m_curr_config.file_name_prefix.c_str(),
            time_string, m_curr_config.video_id);
    INFO("Generate file success: %s in %s", file_name, m_muxer_name.c_str());
  } while(0);
  return ret;
}

void AMMuxerJpegNormal::main_loop()
{
  bool is_ok = true;
  uint32_t statfs_number = 0;
  m_run = true;
  uint32_t error_count = 0;
  is_ok = (AM_MUXER_CODEC_RUNNING == create_resource());
  while(m_run.load() && is_ok) {
    AMPacket* packet = NULL;
    if (m_packet_queue.empty()) {
      DEBUG("Packet queue is empty, sleep 100 ms.");
      usleep(30000);
      continue;
    }
    packet = m_packet_queue.front_and_pop();
    if (AM_LIKELY(is_ok && m_file_writing)) {
      switch (packet->get_type()) {
        case AMPacket::AM_PAYLOAD_TYPE_INFO: {
          if (AM_UNLIKELY(on_info_packet(packet) != AM_STATE_OK)) {
            ERROR("On av info error, exit the main loop.");
            AUTO_MEM_LOCK(m_state_lock);
            m_state = AM_MUXER_CODEC_ERROR;
            is_ok = false;
          }
        } break;
        case AMPacket::AM_PAYLOAD_TYPE_DATA: {
          if (AM_UNLIKELY(on_data_packet(packet) != AM_STATE_OK)) {
            ++ error_count;
            WARN("On normal data packet error, error count is %u!", error_count);
            if(error_count == JPEG_DATA_PKT_ERROR_NUM) {
              ERROR("On normal data packet error, exit the main loop");
              AUTO_MEM_LOCK(m_state_lock);
              m_state = AM_MUXER_CODEC_ERROR;
              is_ok = false;
            }
            break;
          }
          error_count = 0;
          ++ statfs_number;
          if ((statfs_number >= CHECK_FREE_SPACE_FREQUENCY_FOR_JPEG) &&
              m_file_writing) {
            statfs_number = 0;
            check_storage_free_space();
          }
        } break;
        case AMPacket::AM_PAYLOAD_TYPE_EOS: {
          NOTICE("receive eos packet");
          if (AM_UNLIKELY(on_eos_packet(packet) != AM_STATE_OK)) {
            ERROR("On eos packet error, exit the main loop.");
            AUTO_MEM_LOCK(m_state_lock);
            m_state = AM_MUXER_CODEC_ERROR;
            is_ok = false;
          }
        } break;
        default: {
          WARN("Unknown packet type: %u!", packet->get_type());
        } break;
      }
    }
    packet->release();
  }
  release_resource();
  if (AM_UNLIKELY(!m_run.load())) {
    AUTO_MEM_LOCK(m_state_lock);
    m_state = AM_MUXER_CODEC_STOPPED;
  }
  NOTICE("Jpeg muxer exit mainloop.");
}

