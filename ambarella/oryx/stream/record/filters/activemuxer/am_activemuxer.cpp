/*******************************************************************************
 * am_muxer.cpp
 *
 * History:
 *   2014-12-29 - [ypchang] created file
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
#include "am_amf_interface.h"
#include "am_amf_queue.h"
#include "am_amf_base.h"
#include "am_amf_packet.h"
#include "am_amf_packet_filter.h"
#include "am_amf_packet_pin.h"
#include "am_amf_packet_pool.h"
#include "am_event.h"
#include "am_muxer_if.h"
#include "am_avqueue.h"
#include "am_activemuxer.h"
#include "am_activemuxer_config.h"
#include "am_activemuxer_version.h"
#include "am_activemuxer_codec_obj.h"
#include "am_muxer_codec_if.h"
#include "am_file_operation_if.h"
#include "am_jpeg_exif_3a_if.h"
#include "am_record_event_sender.h"

#define MAX_ERROR_COUNTER   15
AMIInterface* create_filter(AMIEngine *engine, const char *config,
                            uint32_t input_num, uint32_t output_num)
{
  return (AMIInterface*)AMActiveMuxer::create(engine,
                                              config,
                                              input_num,
                                              output_num);
}

AMIMuxer* AMActiveMuxer::create(AMIEngine *engine,
                                const std::string& config,
                                uint32_t input_num,
                                uint32_t output_num)
{
  AMActiveMuxer *result = new AMActiveMuxer(engine);
  if (AM_UNLIKELY(result && (AM_STATE_OK != result->init(config,
                                                         input_num,
                                                         output_num)))) {
    delete result;
    result = NULL;
  }
  return result;
}

void* AMActiveMuxer::get_interface(AM_REFIID ref_iid)
{
  return (ref_iid == IID_AMIMuxer) ? ((AMIMuxer*)this) :
      inherited::get_interface(ref_iid);
}

void AMActiveMuxer::destroy()
{
  inherited::destroy();
}

void AMActiveMuxer::get_info(INFO& info)
{
  info.num_in = m_input_num;
  info.num_out = m_output_num;
  info.name = m_name;
}

AMIPacketPin* AMActiveMuxer::get_input_pin(uint32_t index)
{
  AMIPacketPin *pin = (index < m_input_num) ? m_inputs[index] : nullptr;
  if (AM_UNLIKELY(!pin)) {
    ERROR("No such input pin [index: %u]", index);
  }
  return pin;
}

AMIPacketPin* AMActiveMuxer::get_output_pin(uint32_t index)
{
  ERROR("%s doesn't have output pin!", m_name);
  return NULL;
}

AM_STATE AMActiveMuxer::start()
{
  return AM_STATE_OK;
}

AM_STATE AMActiveMuxer::stop()
{
  AM_STATE state = AM_STATE_OK;
  m_run = false;
  state = inherited::stop();
  return state;
}

bool AMActiveMuxer::start_send_normal_pkt()
{
  bool ret = true;
  do {
    if (!m_run) {
      ERROR("%s is not running.");
      ret = false;
      break;
    }
    if (!m_avqueue) {
      ERROR("The avqueue is not enabled in %s", m_name);
      ret = false;
      break;
    }
    if (m_avqueue->is_sending_normal_pkt()) {
      ERROR("Sending normal pkt function has been started already");
      ret = false;
      break;
    }
    if (!m_avqueue->start_send_normal_pkt()) {
      ERROR("Failed to start send normal pkt.");
      ret = false;
      break;
    }
  } while(0);
  return ret;
}

bool AMActiveMuxer::start_file_recording(uint32_t muxer_id_bit_map)
{
  bool ret = true;
  INFO("begin to start file recording.");
  for (uint32_t i = 0; i < m_muxer_config->media_type_num; ++ i) {
    if (AM_LIKELY((m_muxer_codec[i].is_valid()) &&
                  (((muxer_id_bit_map >> m_muxer_codec[i].
                                         m_codec->get_muxer_id()) & 0x01) ==
                  0x01))) {
      AMIFileOperation *codec_file_op = (AMIFileOperation*)
          m_muxer_codec[i].m_codec->
          get_muxer_codec_interface(IID_AMIFileOperation);
      if (AM_LIKELY(codec_file_op)) {
        if(AM_UNLIKELY(!codec_file_op->start_file_writing())) {
          ERROR("file muxer %s start file writing error, muxer id is %u.",
                m_muxer_codec[i].m_name.c_str(),
                m_muxer_codec[i].m_codec->get_muxer_id());
          ret = false;
          break;
        }
      } else {
        ERROR("Should not start file recording for %s, muxer id is %u",
              m_muxer_codec[i].m_name.c_str(),
              m_muxer_codec[i].m_codec->get_muxer_id());
        ret = false;
        break;
      }
    }
  }
  if(ret) {
    INFO("start file recording success.");
  }
  return ret;
}

bool AMActiveMuxer::stop_file_recording(uint32_t muxer_id_bit_map)
{
  bool ret = true;
  INFO("Begin to stop file recording for muxer id bit map %u",
       muxer_id_bit_map);
  for (uint32_t i = 0; i < m_muxer_config->media_type_num; ++ i) {
    if (AM_LIKELY((m_muxer_codec[i].is_valid()) &&
                  (((muxer_id_bit_map >> m_muxer_codec[i].
                                         m_codec->get_muxer_id()) & 0x01) ==
                  0x01))) {
      AMIFileOperation *codec_file_op = (AMIFileOperation*)
          m_muxer_codec[i].m_codec->
          get_muxer_codec_interface(IID_AMIFileOperation);
      if (AM_LIKELY(codec_file_op)) {
        if(AM_UNLIKELY(!codec_file_op->stop_file_writing())) {
          ERROR("file muxer %s stop error, muxer id is %u.",
                m_muxer_codec[i].m_name.c_str(),
                m_muxer_codec[i].m_codec->get_muxer_id());
          ret = false;
          break;
        }
      } else {
        ERROR("Should not stop file recording for %s, muxer id is %u",
              m_muxer_codec[i].m_name.c_str(),
              m_muxer_codec[i].m_codec->get_muxer_id());
        ret = false;
        break;
      }
    }
  }
  if(ret) {
    INFO("stop file recording success.");
  }
  return ret;
}

bool AMActiveMuxer::set_recording_file_num(uint32_t muxer_id_bit_map,
                                           uint32_t file_num)
{
  bool ret = true;
  INFO("begin to set recording file num, muxer id bit map is %u, "
       "file num is %u", muxer_id_bit_map, file_num);
  for (uint32_t i = 0; i < m_muxer_config->media_type_num; ++ i) {
    if (AM_LIKELY((m_muxer_codec[i].is_valid()) &&
                  (((muxer_id_bit_map >> m_muxer_codec[i].
                                         m_codec->get_muxer_id()) & 0x01) ==
                  0x01))) {
      AMIFileOperation *codec_file_op = (AMIFileOperation*)
          m_muxer_codec[i].m_codec->
          get_muxer_codec_interface(IID_AMIFileOperation);
      if (AM_LIKELY(codec_file_op)) {
        if(AM_UNLIKELY(!codec_file_op->set_recording_file_num(file_num))) {
          ERROR("Set recording file num : %u for %s error, muxer id is %u",
                file_num, m_muxer_codec[i].m_name.c_str(),
                m_muxer_codec[i].m_codec->get_muxer_id());
          ret = false;
          break;
        }
      } else {
        ERROR("Should not set recording file num for %s, muxer id is %u",
              m_muxer_codec[i].m_name.c_str(),
              m_muxer_codec[i].m_codec->get_muxer_id());
        ret = false;
        break;
      }
    }
  }
  if(ret) {
    INFO("Set recording file num success.");
  }
  return ret;
}

bool AMActiveMuxer::set_recording_duration(uint32_t muxer_id_bit_map,
                                           int32_t duration)
{
  bool ret = true;
  INFO("Begin to set recording duration, muxer id bit map is %u, duration is %d",
       muxer_id_bit_map, duration);
  for (uint32_t i = 0; i < m_muxer_config->media_type_num; ++ i) {
    if (AM_LIKELY((m_muxer_codec[i].is_valid()) &&
                  (((muxer_id_bit_map >> m_muxer_codec[i].
                                         m_codec->get_muxer_id()) & 0x01) ==
                  0x01))) {
      AMIFileOperation *codec_file_op = (AMIFileOperation*)
          m_muxer_codec[i].m_codec->
          get_muxer_codec_interface(IID_AMIFileOperation);
      if (AM_LIKELY(codec_file_op)) {
        if(AM_UNLIKELY(!codec_file_op->set_recording_duration(duration))) {
          ERROR("Set recording duration : %u for %s error! muxer id is %u",
                duration, m_muxer_codec[i].m_name.c_str(),
                m_muxer_codec[i].m_codec->get_muxer_id());
          ret = false;
          break;
        }
      } else {
        ERROR("Should not set recording duration for %s, muxer id is %u",
              m_muxer_codec[i].m_name.c_str(),
              m_muxer_codec[i].m_codec->get_muxer_id());
        ret = false;
        break;
      }
    }
  }
  if(ret) {
    INFO("Set recording duration success.");
  }
  return ret;
}

bool AMActiveMuxer::set_file_duration(uint32_t muxer_id_bit_map,
                                      int32_t file_duration,
                                      bool apply_conf_file)
{
  bool ret = true;
  INFO("Begin to set file duration, muxer id bit map is %u, file_duration is %d,"
      "apply_conf_file is %s",
       muxer_id_bit_map, file_duration, apply_conf_file ? "true" : "false");
  for (uint32_t i = 0; i < m_muxer_config->media_type_num; ++ i) {
    if (AM_LIKELY((m_muxer_codec[i].is_valid()) &&
                  (((muxer_id_bit_map >> m_muxer_codec[i].
                                         m_codec->get_muxer_id()) & 0x01) ==
                  0x01))) {
      AMIFileOperation *codec_file_op = (AMIFileOperation*)
          m_muxer_codec[i].m_codec->
          get_muxer_codec_interface(IID_AMIFileOperation);
      if (AM_LIKELY(codec_file_op)) {
        if(AM_UNLIKELY(!codec_file_op->set_file_duration(file_duration,
                                                         apply_conf_file))) {
          ERROR("Set file duration : %u for %s error! muxer id is %u",
                file_duration, m_muxer_codec[i].m_name.c_str(),
                m_muxer_codec[i].m_codec->get_muxer_id());
          ret = false;
          break;
        }
      } else {
        ERROR("Should not set file duration for %s, muxer id is %u",
              m_muxer_codec[i].m_name.c_str(),
              m_muxer_codec[i].m_codec->get_muxer_id());
        ret = false;
        break;
      }
    }
  }
  if(ret) {
    INFO("Set file duration success.");
  }
  return ret;
}

bool AMActiveMuxer::set_muxer_param(AMMuxerParam &param)
{
  bool ret = true;
  for (uint32_t i = 0; i < m_muxer_config->media_type_num; ++ i) {
    if (AM_LIKELY((m_muxer_codec[i].is_valid()) &&
                  (((param.muxer_id_bit_map >> m_muxer_codec[i].
                      m_codec->get_muxer_id()) & 0x01) == 0x01))) {
      AMIFileOperation *codec_file_op = (AMIFileOperation*)
                m_muxer_codec[i].m_codec->
                get_muxer_codec_interface(IID_AMIFileOperation);
      if (AM_LIKELY(codec_file_op)) {
        if(AM_UNLIKELY(!codec_file_op->set_muxer_param(param))) {
          ERROR("Set %s muxer param error error!",
                 m_muxer_codec[i].m_name.c_str());
          ret = false;
          break;
        }
      } else {
        ERROR("Should not set muxer param for %s",
              m_muxer_codec[i].m_name.c_str());
        ret = false;
        break;
      }
    }
  }
  if(ret) {
    INFO("Set muxer param success.");
  }
  return ret;
}

bool AMActiveMuxer::set_file_operation_callback(uint32_t muxer_id_bit_map,
                                                AM_FILE_OPERATION_CB_TYPE type,
                                                AMFileOperationCB callback)
{
  bool ret = true;
  do {
    for (uint32_t i = 0; i < m_muxer_config->media_type_num; ++ i) {
      if (AM_LIKELY((m_muxer_codec[i].is_valid()) &&
                    (((muxer_id_bit_map >> m_muxer_codec[i].
                                           m_codec->get_muxer_id()) & 0x01) ==
                    0x01))) {
        AMIFileOperation *codec_file_op = (AMIFileOperation*)
            m_muxer_codec[i].m_codec->
            get_muxer_codec_interface(IID_AMIFileOperation);
        if (AM_LIKELY(codec_file_op)) {
          if(AM_UNLIKELY(!codec_file_op->set_file_operation_callback(type,
                                                                     callback))) {
            ERROR("Failed to set file operation callback function in %s",
                  m_muxer_codec[i].m_name.c_str());
            ret = false;
            break;
          }
        } else {
          ERROR("Should not set file operation callback for %s, muxer id is %u",
                m_muxer_codec[i].m_name.c_str(),
                m_muxer_codec[i].m_codec->get_muxer_id());
          ret = false;
          break;
        }
      }
    }
  } while(0);
  return ret;
}

bool AMActiveMuxer::is_ready_for_event(AMEventStruct& event)
{
  bool ret = true;
  do {
    if (event.h26x.stream_id != m_muxer_config->video_id) {
      ret = false;
      break;
    }
    if (m_avqueue) {
      ret = m_avqueue->is_ready_for_event(event);
    } else {
      ERROR("The avqueue is not exist, is event not enabled?");
      ret = false;
    }
  } while(0);
  return ret;
}

bool AMActiveMuxer::update_image_3a_info(AMImage3AInfo *image_3a)
{
  bool ret = true;
  for (uint32_t i = 0; i < m_muxer_config->media_type_num; ++ i) {
    if (AM_LIKELY((m_muxer_codec[i].is_valid()))) {
      AMIJpegExif3A *jpeg = (AMIJpegExif3A*)
          m_muxer_codec[i].m_codec->
          get_muxer_codec_interface(IID_AMIJpegExif3A);
      if (AM_UNLIKELY(jpeg && !jpeg->update_image_3A_info(image_3a))) {
        ERROR("Failed to update image 3A info to %s!",
              m_muxer_codec[i].m_codec->name());
        ret = false;
      }
    }
  }
  return ret;
}

uint32_t AMActiveMuxer::version()
{
  return MUXER_VERSION_NUMBER;
}

AM_MUXER_TYPE AMActiveMuxer::type()
{
  return m_muxer_config ? m_muxer_config->type : AM_MUXER_TYPE_NONE;
}

void AMActiveMuxer::avqueue_cb(AMPacket *packet)
{
  for (uint32_t i = 0; i < m_muxer_config->media_type_num; ++ i) {
    if (AM_LIKELY(m_muxer_codec[i].is_valid())) {
      if (((packet->get_packet_type() & AMPacket::AM_PACKET_TYPE_EVENT)
          != 0) && (m_muxer_codec[i].m_codec->get_muxer_attr()
              == AM_MUXER_FILE_EVENT)) {
        if(packet->get_attr() == AMPacket::AM_PAYLOAD_ATTR_VIDEO) {
          if(((0x01 << packet->get_stream_id()) &
              (m_muxer_codec[i].m_codec->get_muxer_codec_stream_id()))
              == 0) {
            continue;
          }
        }
        if(packet->get_attr() == AMPacket::AM_PAYLOAD_ATTR_AUDIO) {
          if(m_muxer_codec[i].m_codec->get_state() !=
              AM_MUXER_CODEC_RUNNING) {
            continue;
          }
        }
      }
      switch (m_muxer_codec[i].m_codec->get_state()) {
        case AM_MUXER_CODEC_ERROR: {
          ERROR("Muxer %s: unrecoverable error occurred, destroy!",
                m_muxer_codec[i].m_name.c_str());
          m_muxer_codec[i].destroy();
        } break;
        case AM_MUXER_CODEC_INIT:
        case AM_MUXER_CODEC_STOPPED:
          if (AM_UNLIKELY(AM_STATE_OK !=
              m_muxer_codec[i].m_codec->start())) {
            ERROR("Failed to start muxer %s, destroy!",
                  m_muxer_codec[i].m_name.c_str());
            m_muxer_codec[i].destroy();
          } /* muxer enters RUNNING state, if start OK */
          /* no breaks */
        case AM_MUXER_CODEC_RUNNING: {
          if (AM_LIKELY(m_muxer_codec[i].is_valid())) {
            /* check again, muxer may be destroyed when failed to start */
            packet->add_ref();
            m_muxer_codec[i].m_codec->feed_data(packet);
          }
        } break;
        default :
          break;
      }
    }
  }
  packet->release();
}

bool AMActiveMuxer::create_resource()
{
  bool ret = true;
  do {
    if (m_muxer_config->type == AM_MUXER_TYPE_FILE) {
      std::string name = std::string(m_name);
      m_avqueue = AMAVQueue::create(m_avqueue_config, name,
                                    m_muxer_config->auto_start,
                                    std::bind(&AMActiveMuxer::avqueue_cb,
                                              this, std::placeholders::_1));
      if (!m_avqueue) {
        ERROR("Failed to create avqueue in %s.", m_name);
        ret = false;
        break;
      }
    }
  } while(0);
  return ret;
}

void AMActiveMuxer::release_resource()
{
  for (uint32_t i = 0; i < m_muxer_config->media_type_num; ++ i) {
    if (AM_LIKELY(m_muxer_codec[i].is_valid())) {
      INFO("%s is stopping %s!", m_name, m_muxer_codec[i].m_name.c_str());
      m_muxer_codec[i].m_codec->stop();
      INFO("%s is stopped!", m_muxer_codec[i].m_name.c_str());
    }
  }
  NOTICE("%s has stopped %u %s!", m_name,
         m_muxer_config->media_type_num,
         (m_muxer_config->media_type_num > 1) ? "muxers" : "muxer");
  m_avqueue = nullptr;
}

void AMActiveMuxer::work_loop()
{
  AMPacketQueueInputPin *input_pin = NULL;
  AMPacket              *input_pkt = NULL;
  uint32_t count = 0;
  if (create_resource()) {
    m_run = true;
    while(m_run.load()) {
      if (AM_UNLIKELY(!wait_input_packet(input_pin, input_pkt))) {
        if (AM_LIKELY(!m_run.load())) {
          NOTICE("Stop of %s is called!", m_name);
        } else {
          NOTICE("Filter is aborted!");
        }
        break;
      }
      if (AM_LIKELY(input_pkt)) {
        if (m_muxer_config->type == AM_MUXER_TYPE_FILE) {
          input_pkt->add_ref();
          if (m_avqueue->process_packet(input_pkt) != AM_STATE_OK) {
            ERROR("Failed to process packet.");
            ++ count;
            if (count >= MAX_ERROR_COUNTER) {
              ERROR("Process packet error count is %u, exit mainloop", count);
              m_run = false;
              continue;
            } else {
              count = 0;
            }
          }
        } else {
          input_pkt->add_ref();
          avqueue_cb(input_pkt);
        }
        input_pkt->release();
      } else {
        ERROR("Invalid packet!");
      }
    }
  }
  release_resource();
  NOTICE("Exit mainloop in %s", m_name);
}

void AMActiveMuxer::on_run()
{
  AmMsg  engine_msg(AMIEngine::ENG_MSG_OK);
  engine_msg.p0 = (int_ptr)(get_interface(IID_AMIInterface));

  ack(AM_STATE_OK);
  post_engine_msg(engine_msg);
  INFO("%s starts to run!", m_name);
  work_loop();
  if (AM_LIKELY(!m_run.load())) {
    NOTICE("%s posts EOS!", m_name);
    engine_msg.code = AMIEngine::ENG_MSG_EOS;
    post_engine_msg(engine_msg);
  } else {
    NOTICE("%s posts ABORT!", m_name);
    engine_msg.code = AMIEngine::ENG_MSG_ABORT;
    post_engine_msg(engine_msg);
  }
  m_run = false;
  INFO("%s exits mainloop!", m_name);
}

AM_STATE AMActiveMuxer::load_muxer_codec()
{
  AM_STATE state = AM_STATE_ERROR;
  if (AM_LIKELY(m_muxer_config)) {
    destroy_muxer_codec();
    m_muxer_codec = new AMActiveMuxerCodecObj[m_muxer_config->media_type_num];
    if (AM_LIKELY(m_muxer_codec)) {
      bool muxer_codec_loaded = false;
      for (uint32_t i = 0; i < m_muxer_config->media_type_num; ++ i) {
        muxer_codec_loaded =
            (m_muxer_codec[i].load_codec(m_muxer_config->media_type[i]) ||
             muxer_codec_loaded);
      }
      state = muxer_codec_loaded ? AM_STATE_OK : AM_STATE_ERROR;
    } else {
      ERROR("Failed to allodate memory for muxer codec object!");
    }
  } else {
    ERROR("Muxer config is not loaded!");
  }

  return state;
}

void AMActiveMuxer::destroy_muxer_codec()
{
  delete[] m_muxer_codec;
  m_muxer_codec = NULL;
}

AMActiveMuxer::AMActiveMuxer(AMIEngine *engine) :
    inherited(engine)
{
  m_inputs.clear();
}

AMActiveMuxer::~AMActiveMuxer()
{
  for (uint32_t i = 0; i < m_inputs.size(); ++ i) {
    AM_DESTROY(m_inputs[i]);
  }
  m_inputs.clear();
  delete m_config;
  destroy_muxer_codec();
}

AM_STATE AMActiveMuxer::init(const std::string& config,
                       uint32_t input_num,
                       uint32_t output_num)
{
  AM_STATE state = AM_STATE_OK;
  m_input_num = input_num;
  m_output_num = output_num;

  do {
    m_config = new AMActiveMuxerConfig();
    if (AM_UNLIKELY(!m_config)) {
      ERROR("Failed to create config module for Muxer filter!");
      state = AM_STATE_NO_MEMORY;
      break;
    }

    m_muxer_config = m_config->get_config(config);
    if (AM_UNLIKELY(!m_muxer_config)) {
      ERROR("Can not get configuration from file %s, please check!",
            config.c_str());
      state = AM_STATE_ERROR;
      break;
    }
    if (m_muxer_config->type == AM_MUXER_TYPE_FILE) {
      get_avqueue_config(*m_muxer_config, m_avqueue_config);
    }
    state = load_muxer_codec();
    if (AM_UNLIKELY(AM_STATE_OK != state)) {
      ERROR("Failed to load muxer codecs!");
      break;
    }
    state = inherited::init((const char*)m_muxer_config->name.c_str(),
                            m_muxer_config->real_time.enabled,
                            m_muxer_config->real_time.priority);
    if (AM_LIKELY(state == AM_STATE_OK)) {
      if (AM_UNLIKELY(0 == m_input_num)) {
        ERROR("%s doesn't have input! Invalid configuration! Abort!");
        state = AM_STATE_ERROR;
        break;
      }
      if (AM_UNLIKELY(m_output_num)) {
        WARN("%s should not have output, but output num is %u, reset to 0!",
             m_name, m_output_num);
        m_output_num = 0;
      }
      for (uint32_t i = 0; i < m_input_num; ++ i) {
        AMActiveMuxerInput *input = AMActiveMuxerInput::create(this);
        if (AM_UNLIKELY(!input)) {
          state = AM_STATE_ERROR;
          ERROR("Failed to create input pin[%u] for %s!", i, m_name);
          break;
        }
        m_inputs.push_back(input);
      }
      if (AM_UNLIKELY(AM_STATE_OK != state)) {
        break;
      }
    }
  }while(0);
  return state;
}

void AMActiveMuxer::get_avqueue_config(ActiveMuxerConfig& muxer_config,
                                       AVQueueConfig& avqueue_config)
{
  do {
    avqueue_config.video_id = muxer_config.video_id;
    avqueue_config.event_enable = muxer_config.event_enable;
    avqueue_config.event_max_history_duration =
        muxer_config.event_max_history_duration;
    avqueue_config.pkt_pool_size = muxer_config.avqueue_pkt_pool_size;
    for (auto &audio : muxer_config.audio_types) {
      for(auto &sam_rate : audio.second) {
        avqueue_config.audio_types[audio.first].push_back(sam_rate);
      }
    }
    avqueue_config.event_audio.first = muxer_config.event_audio.first;
    avqueue_config.event_audio.second = muxer_config.event_audio.second;
    avqueue_config.event_gsensor_enable = muxer_config.event_gsensor_enable;
  } while(0);
}
