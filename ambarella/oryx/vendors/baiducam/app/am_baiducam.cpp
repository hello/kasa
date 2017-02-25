/*
 * am_baiducam.cpp
 *
 *  History:
 *    Apr 9, 2015 - [Shupeng Ren] created file
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

#include "json.h"
#include "am_base_include.h"
#include "am_define.h"
#include "am_log.h"

#include "am_export_if.h"
#include "am_bls_rtmp.h"
#include "am_baiducam.h"

#define AM_DESTROY(_obj) \
    do { if (_obj) {(_obj)->destroy(); _obj = NULL;}} while(0)

AMBaiduCam* AMBaiduCam::create(uint32_t stream_id)
{
  AMBaiduCam *result = new AMBaiduCam(stream_id);
  if (result && !result->init()) {
    delete result;
    result = nullptr;
  }
  return result;
}

bool AMBaiduCam::init()
{
  bool ret = true;
  do {
    if (!(m_rtmp = AMBLSRtmp::create())) {
      ret = false;
    }

    if (!(m_event = AMEvent::create())) {
      ERROR("Failed to create event!");
      break;
    }
    if (!(m_usrcmd_thread = AMThread::create("process_cmd",
                                             thread_entry,
                                             this))) {
      ERROR("Failed to create thread!");
      break;
    }
  } while (0);
  return ret;
}

void AMBaiduCam::destroy()
{
  delete this;
}

AMBaiduCam::AMBaiduCam(uint32_t stream_id) :
  m_rtmp(nullptr),
  m_sps_come_flag(false),
  m_pps_come_flag(false),
  m_video_info_come_flag(false),
  m_audio_info_come_flag(false),
  m_video_stream_id(stream_id),
  m_state(AM_BD_STATE_WAIT_FOR_INFOS),
  m_first_pts(false, 0),
  m_last_pts(0),
  m_thread_exit(false),
  m_event(nullptr),
  m_usrcmd_thread(nullptr),
  m_session_token("")
{
}

AMBaiduCam::~AMBaiduCam()
{
  if (m_rtmp) {
    m_rtmp->destroy();
  }
  m_thread_exit = true;
  m_event->signal();
  AM_DESTROY(m_usrcmd_thread);
  AM_DESTROY(m_event);
  m_nalu_list.clear();
}

void AMBaiduCam::set_auth_info(std::string &url,
                               std::string &devid,
                               std::string &access_token,
                               std::string &stream_name)
{
  m_url = url;
  m_devid = devid;
  m_access_token = access_token;
  m_stream_name = stream_name;
}

bool AMBaiduCam::connect_bls()
{
  bool ret =  m_rtmp->connect(m_url, m_devid, m_access_token,
                              m_stream_name, m_session_token);
  m_event->signal();
  return ret;
}

bool AMBaiduCam::connect_bls(std::string &session_token)
{
  bool ret = m_rtmp->connect(m_url, m_devid, m_access_token,
                             m_stream_name, session_token);
  m_event->signal();
  return ret;
}

bool AMBaiduCam::connect_bls(std::string &url,
                             std::string &devid,
                             std::string &access_token,
                             std::string &stream_name,
                             std::string &session_token)
{
  bool ret = m_rtmp->connect(url, devid, access_token,
                             stream_name, session_token);
  m_event->signal();
  return ret;
}

bool AMBaiduCam::disconnect_bls()
{
  return m_rtmp->disconnect();
}

bool AMBaiduCam::reconnect_bls()
{
  return (disconnect_bls() && connect_bls());
}

bool AMBaiduCam::isconnect_bls()
{
  return m_rtmp->isconnected();
}

void AMBaiduCam::thread_entry(void *arg)
{
  if (!arg) {
    ERROR("Invalid thread argument: %p", arg);
  } else {
    AMBaiduCam *bCam = ((AMBaiduCam*)arg);
    do {
      bCam->m_event->wait();
      while (bCam->isconnect_bls()) {
        if (!bCam->m_rtmp->process_usr_cmd(bCam->static_usercmd_callback, arg)) {
          break;
        }
      }
    } while (!bCam->m_thread_exit);
  }
}

bool AMBaiduCam::static_usercmd_callback(const char *cmd, void *arg)
{
  return (arg && ((AMBaiduCam*)arg)->usrcmd_callback(cmd));
}

bool AMBaiduCam::usrcmd_callback(const char *cmd)
{
  bool ret = true;
  json_object *jobj = json_tokener_parse(cmd);
  json_object_iterator it = json_object_iter_begin(jobj);
  json_object_iterator itend = json_object_iter_end(jobj);

  while (!json_object_iter_equal(&it, &itend)) {
    json_object *tmp_obj = nullptr;
    const char *s = json_object_iter_peek_name(&it);
    if (json_object_object_get_ex(jobj, s, &tmp_obj)) {
      switch (json_object_get_type(tmp_obj)) {
        case json_type_null:
          break;
        case json_type_boolean:
          INFO("%s: %s", s,
               json_object_get_boolean(tmp_obj) ? "true" : "false");
          break;
        case json_type_int:
          INFO("%s: %d", s, json_object_get_int(tmp_obj));
          break;
        case json_type_double:
          INFO("%s: %f", s, json_object_get_double(tmp_obj));
          break;
        case json_type_string:
          INFO("%s: %s", s, json_object_get_string(tmp_obj));
          break;
        default:
          INFO("%s: not found!", s);
          break;
      }
    }
    json_object_iter_next(&it);
  }
  json_object_put(jobj);
  return ret;
}

bool AMBaiduCam::process_packet(AMExportPacket *packet)
{
  bool ret = true;
  switch (m_state) {
    case AM_BD_STATE_WAIT_FOR_INFOS:
      if ((packet->packet_type == AM_EXPORT_PACKET_TYPE_VIDEO_INFO) &&
          (packet->stream_index == m_video_stream_id)) {
        AMExportVideoInfo *video_info = (AMExportVideoInfo*)packet->data_ptr;
        m_video_meta.videocodecid = 7;
        m_video_meta.width = video_info->width;
        m_video_meta.height = video_info->height;
        m_video_meta.framerate = 30;
        m_rtmp->set_video_meta(&m_video_meta);
        m_video_info_come_flag = true;
      } else if ((packet->packet_type == AM_EXPORT_PACKET_TYPE_AUDIO_INFO) &&
          (packet->packet_format == AM_EXPORT_PACKET_FORMAT_AAC)) {
        AMExportAudioInfo *audio_info = (AMExportAudioInfo*)packet->data_ptr;
        m_audio_meta.audiocodecid = 0x10;
        m_audio_meta.samplesize = audio_info->sample_size;
        m_audio_meta.samplerate = audio_info->samplerate;
        m_audio_meta.channel = audio_info->channels;
        m_audio_meta.stereo = (audio_info->channels == 1) ? 0 : 1;
        m_rtmp->set_audio_meta(&m_audio_meta);
        m_audio_info_come_flag = true;
      }
      if (m_video_info_come_flag && m_audio_info_come_flag) {
        if (!m_rtmp->write_meta()) {
          m_state = AM_BD_STATE_RECONNECT_TO_BLS;
          ERROR("Failed to write meta!");
          break;
        }
        INFO("Write Meta OK!");
        m_state = AM_BD_STATE_WAIT_FOR_SPS_PPS;
        m_video_info_come_flag = false;
        m_audio_info_come_flag = false;
      }
      break;
    case AM_BD_STATE_WAIT_FOR_SPS_PPS:
      if ((packet->packet_type == AM_EXPORT_PACKET_TYPE_VIDEO_DATA) &&
          (packet->stream_index == m_video_stream_id)) {
        if (!get_nalus(packet)) {
          break;
        }
      }
      if (!m_sps_come_flag || !m_pps_come_flag) {
        break;
      } else {
        INFO("Begin to write header!");
        if (!write_video_header()) {
          m_state = AM_BD_STATE_RECONNECT_TO_BLS;
          ERROR("Failed to write video header!");
          break;
        }
        INFO("Write video header OK!");
        if (!write_audio_header()) {
          m_state = AM_BD_STATE_RECONNECT_TO_BLS;
          ERROR("Failed to write audio header!");
          break;
        }
        INFO("Write audio header OK!");
        m_sps_come_flag = false;
        m_pps_come_flag = false;
        m_first_pts.first = false;
        m_state = AM_BD_STATE_SEND_DATA;
        INFO("Begin to write data!");
      }
      //no break for some cases
    case AM_BD_STATE_SEND_DATA:
      if ((packet->packet_type != AM_EXPORT_PACKET_TYPE_VIDEO_DATA) &&
          (packet->packet_type != AM_EXPORT_PACKET_TYPE_AUDIO_DATA)) {
        break;
      }
      if (m_last_pts > packet->pts) {
        ERROR("PTS(%lld) < last PTS(%lld), drop packet!",
              packet->pts, m_last_pts);
        break;
      } else {
        m_last_pts = packet->pts;
      }
      if ((packet->packet_type == AM_EXPORT_PACKET_TYPE_VIDEO_DATA) &&
          (packet->stream_index == m_video_stream_id)) {
        if (!get_nalus(packet)) {
          break;
        }
        if (!m_first_pts.first) {
          m_first_pts.second = packet->pts;
          m_first_pts.first = true;
        }
        if (!write_video_data(packet, packet->pts - m_first_pts.second)) {
          m_state = AM_BD_STATE_RECONNECT_TO_BLS;
        }
      } else if ((packet->packet_type == AM_EXPORT_PACKET_TYPE_AUDIO_DATA) &&
          (packet->packet_format == AM_EXPORT_PACKET_FORMAT_AAC)) {
        if (m_first_pts.first) {
          if (!write_audio_data(packet, packet->pts - m_first_pts.second)) {
            m_state = AM_BD_STATE_RECONNECT_TO_BLS;
          }
        }
      }
      break;
    case AM_BD_STATE_RECONNECT_TO_BLS:
      INFO("Reconnect!");
      if (reconnect_bls() && m_rtmp->write_meta()){
        m_state = AM_BD_STATE_WAIT_FOR_SPS_PPS;
      } break;
    default:
      break;
  }

  return ret;
}

bool AMBaiduCam::write_audio_header()
{
  char header[4];
  uint32_t temp;
  uint32_t i = 0;

  header[i++] = 0x0a << 4 | 3 << 2 | 1 << 1 | 0;
  header[i++] = 0;

  temp = 2 << 11 | AM_BD_AUDIO_16k << 7 | 1 << 3;

  header[i++] = (temp >> 8) & 0xff;
  header[i++] = temp & 0xff;

  return m_rtmp->write_audio(header, i, 0);
}

bool AMBaiduCam::write_video_header()
{
  char header[512] = {0};
  uint32_t i = 0;

  header[i++] = 0x17;
  header[i++] = 0x00;
  header[i++] = 0x00;
  header[i++] = 0x00;
  header[i++] = 0x00; //0x1700000000 fiexed header

  header[i++] = 0x01; //configurationVersion
  header[i++] = m_sps_pps.sps[1]; //AVCProfileIndication
  header[i++] = m_sps_pps.sps[2]; //profile_compatibility
  header[i++] = m_sps_pps.sps[3]; //AVCLevelIndication
  header[i++] = 0xff; //lengthSizeMinusOne

  header[i++] = 0xe1; //Number of SPS & 0x1f
  header[i++] = m_sps_pps.sps_len >> 8;
  header[i++] = m_sps_pps.sps_len & 0xff; //SPS data length
  memcpy(header + i, m_sps_pps.sps, m_sps_pps.sps_len);
  i += m_sps_pps.sps_len;

  header[i++] = 0x01; //0xe1, Number of PPS
  header[i++] = m_sps_pps.pps_len >> 8;
  header[i++] = m_sps_pps.pps_len & 0xff; //PPS data length
  memcpy(header + i, m_sps_pps.pps, m_sps_pps.pps_len);
  i += m_sps_pps.pps_len;

  return m_rtmp->write_video(header, i, 0);
}

bool AMBaiduCam::write_audio_data(AMExportPacket *packet, uint64_t pts)
{
  bool ret = true;

  AdtsHeader *adts;
  uint8_t *head = packet->data_ptr;
  uint8_t *ptr = head;

  while (ptr - head < (int32_t)packet->data_size) {
    adts = (AdtsHeader*)ptr;
    uint32_t frame_len = adts->frame_length();
    uint8_t channel_conf = adts->aac_channel_conf();
    uint32_t buf_size = frame_len + 8;
    if (m_audio_buffer.size < buf_size) {
      delete m_audio_buffer.buffer;
      m_audio_buffer.size = buf_size;
      m_audio_buffer.buffer = new char[buf_size];
    }
    uint32_t i = 0;
    m_audio_buffer.buffer[i++] =
        (0x0a << 4) | (3 << 2) | (1 << 1) | ((channel_conf == 1) ? 0 : 1);
    m_audio_buffer.buffer[i++] = 0x01;
    memcpy(&m_audio_buffer.buffer[i++], ptr + sizeof(AdtsHeader),
           frame_len - sizeof(AdtsHeader));
    ptr += frame_len;
    if (!(ret = m_rtmp->write_audio(m_audio_buffer.buffer,
                                    frame_len - sizeof(AdtsHeader) + 2,
                                    pts))) {
      break;
    }
  }

  return ret;
}

bool AMBaiduCam::write_video_data(AMExportPacket *packet, uint64_t pts)
{
  bool ret = true;
  for (auto &q : m_nalu_list) {
    if (q.type != H264_IDR_HEAD &&
        q.type != H264_IBP_HEAD) {
      continue;
    }
    if (m_video_buffer.size < q.size + 16) {
      delete m_video_buffer.buffer;
      m_video_buffer.size = q.size + 16;
      m_video_buffer.buffer = new char[m_video_buffer.size];
    }
    uint32_t i = 0;
    if (q.type == H264_IDR_HEAD) {
      m_video_buffer.buffer[i++] = 0x17;
    } else if (q.type == H264_IBP_HEAD) {
      m_video_buffer.buffer[i++] = 0x27;
    }
    m_video_buffer.buffer[i++] = 0x01;
    m_video_buffer.buffer[i++] = 0x00;
    m_video_buffer.buffer[i++] = 0x00;
    m_video_buffer.buffer[i++] = 0x21;
    m_video_buffer.buffer[i++] = (q.size >> 24) & 0xff;
    m_video_buffer.buffer[i++] = (q.size) & 0xff;
    m_video_buffer.buffer[i++] = (q.size >> 8) & 0xff;
    m_video_buffer.buffer[i++] = (q.size & 0xff);
    memcpy(m_video_buffer.buffer + i, q.addr, q.size);
    i += q.size;
    if (!(ret = m_rtmp->write_video(m_video_buffer.buffer, i, pts))) {
      break;
    }
  }
  return ret;
}

bool AMBaiduCam::get_nalus(AMExportPacket *packet)
{
  bool ret = true;

  uint8_t *head = packet->data_ptr;
  if ((head[0] << 24 | head[1] << 16 | head[2] << 8 | head[3]) != 0x00000001) {
    ERROR("NALU start code must be 0x00000001");
    return false;
  }
  m_nalu_list.clear();

  uint8_t *tail = head + packet->data_size;
  uint8_t *ptr = tail - 4;
  AM_H264_NALU nalu;

  while (ptr >= head) {
    if ((ptr[0] << 24 | ptr[1] << 16 | ptr[2] << 8 | ptr[3]) == 0x00000001) {
      nalu.type = H264_NALU_TYPE(ptr[4] & 0x1f);
      nalu.addr = ptr + 4;

      if (m_nalu_list.size() > 0) {
        nalu.size = m_nalu_list[0].addr - nalu.addr - 4;
      } else {
        nalu.size = tail - nalu.addr;
      }

      if (!m_sps_come_flag && nalu.type == H264_SPS_HEAD) {
        m_sps_pps.sps_len = nalu.size;
        memcpy(m_sps_pps.sps, nalu.addr, nalu.size);
        m_sps_come_flag = true;
      } else if (!m_pps_come_flag && nalu.type == H264_PPS_HEAD) {
        m_sps_pps.pps_len = nalu.size;
        memcpy(m_sps_pps.pps, nalu.addr, nalu.size);
        m_pps_come_flag = true;
      }
      m_nalu_list.push_front(nalu);
      ptr -= 4;
    } else if (ptr[-1]) {
      ptr -= 4;
    } else if (ptr[0]) {
      ptr -= 3;
    } else if (ptr[1]) {
      ptr -= 2;
    } else {
      --ptr;
    }
  }

  return ret;
}
