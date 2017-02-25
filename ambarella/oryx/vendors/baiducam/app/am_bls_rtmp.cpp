/*
 * am_bls_rtmp.cpp
 *
 *  History:
 *    Mar 25, 2015 - [Shupeng Ren] created file
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

#include "string.h"
#include <stdio.h>
#include <stdint.h>
#include <string>
#include <map>
#include <mutex>
#include "json.h"

#include "am_log.h"
#include "bls_flv.h"

#include "am_bls_rtmp.h"

static std::recursive_mutex m_mtx;
#define  DECLARE_MUTEX  std::lock_guard<std::recursive_mutex> lck(m_mtx);

#define AM_DESTROY(_obj) \
    do { if (_obj) {(_obj)->destroy(); _obj = NULL;}} while(0)

#define PTS_SCALE 90000

AMBLSRtmp* AMBLSRtmp::create()
{
  AMBLSRtmp *result = new AMBLSRtmp();
  if (result && !result->init()) {
    delete result;
    result = nullptr;
  }
  return result;
}

void AMBLSRtmp::destroy()
{
  delete this;
}

AMBLSRtmp::AMBLSRtmp() :
  m_ortmp(nullptr),
  m_server_time(0),
  m_stream_id(0),
  m_proflag(0x17)
{
}

AMBLSRtmp::~AMBLSRtmp()
{
  INFO("delete");
}

bool AMBLSRtmp::init()
{
  bool ret = false;
  do {
    ret = true;
  } while (0);
  return ret;
}

bool AMBLSRtmp::process_usr_cmd(AM_BLS_USRCMD_CALLBACK cb, void *data)
{
  bool ret = true;
  if (bls_isconnected(m_ortmp)) {
    int val;
    bls_packet *read_packet = nullptr;
    if ((val = bls_read_packet(m_ortmp, &read_packet, 0))) {
      ERROR("Failed to read packet, ret = %d", val);
      return false;
    }

    if (read_packet) {
      if (bls_packet_type(read_packet) == BLS_RTMP_TYPE_COMMAND) {
        if (!strcmp(bls_packet_commandname(read_packet), "userCmd")) {
          char result_s[] = "success";
          char result_f[] = "failed";
          char *cmd = bls_packet_data(read_packet);
          INFO("Command: %s", cmd);
          char *result = cb(cmd, data) ? result_s : result_f;
          if (bls_write_usercmd_result(m_ortmp, result, read_packet)) {
            ERROR("Failed to write user cmd result!");
            ret = false;
          }
        }
      }
      bls_free_packet(read_packet);
    }
  }
  return ret;
}

bool AMBLSRtmp::connect(std::string &url,
                        std::string &devid,
                        std::string &access_token,
                        std::string &stream_name,
                        std::string &session_token)
{
  bool ret = false;
  int r = 0;
  do {
    if (!(m_ortmp = bls_rtmp_create(url.c_str(),
                                    devid.c_str(),
                                    access_token.c_str(),
                                    session_token.c_str(),
                                    "extjson"))) {
      ERROR("Failed to create bls rtmp!");
      break;
    }
    if ((r = bls_connect_app(m_ortmp, &m_server_time))) {
      ERROR("Failed to connect app: %d!", r);
      break;
    }
    INFO("Connect APP OK!");
    if (bls_publish_stream(m_ortmp,
                           stream_name.c_str(),
                           &m_stream_id)) {
      ERROR("Failed to publish stream!");
      break;
    }
    INFO("RTMP StreamID: %d", m_stream_id);
    if (bls_set_streamproperty(m_ortmp, m_proflag)) {
      ERROR("Failed to set stream property!");
      break;
    }
    INFO("Set stream property OK!");
    ret = true;
  } while (0);
  return ret;
}

bool AMBLSRtmp::disconnect()
{
  bool ret = true;
  do {
    bls_unpublish_stream(m_ortmp, m_stream_id);
    bls_rtmp_destroy(m_ortmp);
  } while (0);
  return ret;
}

bool AMBLSRtmp::isconnected()
{
  return bls_isconnected(m_ortmp);
}

bool AMBLSRtmp::write_meta()
{
  bool ret = true;
  char *meta_data = nullptr;
  bls_amf0_t label;
  bls_amf0_t meta_info;
  bls_amf0_t creator;

  do {
    label = bls_amf0_create_str("onMetaData");
    meta_info = bls_amf0_create_ecma_array();
    creator = bls_amf0_create_number(0);
    bls_amf0_ecma_array_property_set(meta_info, "duration", creator);
    creator = bls_amf0_create_number(m_video_meta.width);
    bls_amf0_ecma_array_property_set(meta_info, "width", creator);
    creator = bls_amf0_create_number(m_video_meta.height);
    bls_amf0_ecma_array_property_set(meta_info, "height", creator);
    creator = bls_amf0_create_number(m_video_meta.framerate);
    bls_amf0_ecma_array_property_set(meta_info, "framerate", creator);
    creator = bls_amf0_create_number(m_video_meta.videocodecid);
    bls_amf0_ecma_array_property_set(meta_info, "videocodecid", creator);

    creator = bls_amf0_create_number(m_audio_meta.bitrate);
    bls_amf0_ecma_array_property_set(meta_info, "audiodatarate", creator);
    creator = bls_amf0_create_number(m_audio_meta.samplerate);
    bls_amf0_ecma_array_property_set(meta_info, "audiosamplerate", creator);
    creator = bls_amf0_create_number(m_audio_meta.samplesize);
    bls_amf0_ecma_array_property_set(meta_info, "audiosamplesize", creator);
    creator = bls_amf0_create_number(m_audio_meta.audiocodecid);

    creator = bls_amf0_create_number(m_audio_meta.stereo);
    bls_amf0_ecma_array_property_set(meta_info, "stereo", creator);
    creator = bls_amf0_create_number(m_audio_meta.audiocodecid);
    bls_amf0_ecma_array_property_set(meta_info, "audiocodecid", creator);
    creator = bls_amf0_create_str("Ambarella");
    bls_amf0_ecma_array_property_set(meta_info, "encoder", creator);

    creator = bls_amf0_create_number(0);
    bls_amf0_ecma_array_property_set(meta_info, "filesize", creator);

    creator = bls_amf0_create_number(0);
    bls_amf0_ecma_array_property_set(meta_info, "absRecordTime", creator);

    uint32_t label_size = bls_amf0_size(label);
    uint32_t info_size = bls_amf0_size(meta_info);

    if (!(meta_data = new char[label_size + info_size])) {
      ret = false;
      ERROR("Failed to new meta data!");
      break;
    }

    bls_amf0_serialize(label, meta_data, label_size);
    bls_amf0_serialize(meta_info, meta_data + label_size, info_size);

#if 1
    int size = 0;
    PRINTF("The label info: %s\n",
           bls_amf0_human_print(label, nullptr, &size));
    PRINTF("The meta info: %s\n",
           bls_amf0_human_print(meta_info, nullptr, &size));
#endif

    if (bls_write_meta(m_ortmp, meta_data,
                       label_size + info_size, m_stream_id)) {
      ret = false;
      ERROR("Failed to write meta data!");
      break;
    }
    INFO("Write meta OK!");
  } while (0);
  bls_amf0_free(label);
  bls_amf0_free(meta_info);
  delete [] meta_data;
  return ret;
}

bool AMBLSRtmp::write_video(char* data, int32_t size, uint64_t pts)
{
  bool ret = true;
  int val;
  do {
    DECLARE_MUTEX
    uint32_t timestamp = (uint32_t)((double)pts / PTS_SCALE * 1000);

    if (!bls_isconnected(m_ortmp)) {
      ERROR("BLS is disconnected!");
      ret = false;
      break;
    }
    if ((val = bls_write_video(m_ortmp, timestamp,
                               data, size, m_stream_id))) {
      ERROR("Failed to write video: %d!", val);
      ret = false;
      break;
    }
    DEBUG("Write Video, PTS: %d, size: %d!", timestamp, size);
  } while (0);
  return ret;
}

bool AMBLSRtmp::write_audio(char* data, int32_t size, uint64_t pts)
{
  bool ret = true;
  int val;
  do {
    DECLARE_MUTEX
    uint32_t timestamp = (uint32_t)((double)pts / PTS_SCALE * 1000);

    if (!bls_isconnected(m_ortmp)) {
      ERROR("BLS is disconnected!");
      ret = false;
      break;
    }
    if ((val = bls_write_audio(m_ortmp, timestamp,
                               data, size, m_stream_id))) {
      ERROR("Failed to write audio: %d!", val);
      ret = false;
      break;
    }
    DEBUG("Write Audio, PTS: %d, size: %d!", timestamp, size);
  } while (0);
  return ret;
}

void AMBLSRtmp::set_audio_meta(AM_RTMP_AudioMetaData *data)
{
  m_audio_meta = *data;
}

void AMBLSRtmp::set_video_meta(AM_RTMP_VideoMetaData *data)
{
  m_video_meta = *data;
}
