/*******************************************************************************
 * am_video_source_new.cpp
 *
 * History:
 *   Sep 11, 2015 - [ypchang] created file
 *
 * Copyright (C) 2008-2015, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
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

#include "am_video_reader_if.h"
#include "am_video_address_if.h"
#include "am_video_source_if.h"
#include "am_video_source_new.h"
#include "am_video_source_config.h"
#include "am_video_source_version.h"

#include "am_mutex.h"
#include "am_event.h"

#include <unistd.h>

struct AMVideoPtsInfo
{
    int64_t  pts;
    uint32_t streamid;
    bool     is_eos;
    AMVideoPtsInfo() :
      pts(0),
      streamid(0),
      is_eos(false)
    {}
};

AMIInterface* create_filter(AMIEngine *engine,
                            const char *config,
                            uint32_t input_num,
                            uint32_t output_num)
{
  return (AMIInterface*)AMVideoSource::create(engine, config,
                                              input_num, output_num);
}

AMIVideoSource* AMVideoSource::create(AMIEngine *engine,
                                      const std::string& config,
                                      uint32_t input_num,
                                      uint32_t output_num)
{
  AMVideoSource *result = new AMVideoSource(engine);
  if (AM_UNLIKELY(result && (AM_STATE_OK != result->init(config,
                                                         input_num,
                                                         output_num)))) {
    delete result;
    result = NULL;
  }

  return result;
}

void* AMVideoSource::get_interface(AM_REFIID ref_iid)
{
  return (ref_iid == IID_AMIVideoSource) ? (AMIVideoSource*)this :
      inherited::get_interface(ref_iid);
}

void AMVideoSource::destroy()
{
  inherited::destroy();
}

void AMVideoSource::get_info(INFO& info)
{
  info.num_in = m_input_num;
  info.num_out = m_output_num;
  info.name = m_name;
}

AMIPacketPin* AMVideoSource::get_input_pin(uint32_t index)
{
  ERROR("%s doesn't have input pin!", m_name);
  return NULL;
}

AMIPacketPin* AMVideoSource::get_output_pin(uint32_t index)
{
  AMIPacketPin *pin = (index < m_output_num) ? m_outputs[index] : nullptr;
  if (AM_UNLIKELY(!pin)) {
    ERROR("No such output pin [index:%u]", index);
  }

  return pin;
}

AM_STATE AMVideoSource::start()
{
  AUTO_SPIN_LOCK(m_lock);
  m_lock_state->lock();
  if (AM_LIKELY((m_filter_state == AM_VIDEO_SRC_STOPPED) ||
                (m_filter_state == AM_VIDEO_SRC_INITTED) ||
                (m_filter_state == AM_VIDEO_SRC_WAITING))) {
    m_packet_pool->enable(true);
    m_event->signal();
  }
  m_lock_state->unlock();
  return AM_STATE_OK;
}

AM_STATE AMVideoSource::stop()
{
  AUTO_SPIN_LOCK(m_lock);
  m_lock_state->lock();
  if (AM_UNLIKELY(m_filter_state == AM_VIDEO_SRC_WAITING)) {
    /* on_run is blocked on m_event->wait(), need unblock it */
    m_event->signal();
  }
  m_lock_state->unlock();
  m_run = false;
  m_packet_pool->enable(false);
  for(uint32_t i = m_video_info.size(); i > 0; -- i) {
    delete[] (m_video_info[i - 1]);
    m_video_info[i - 1] = NULL;
  }
  m_video_info.clear();
  return inherited::stop();
}

uint32_t AMVideoSource::version()
{
  return VSOURCE_VERSION_NUMBER;
}

void AMVideoSource::on_run()
{
  AmMsg  engine_msg(AMIEngine::ENG_MSG_OK);
  engine_msg.p0 = (int_ptr)(get_interface(IID_AMIInterface));

  ack(AM_STATE_OK);
  post_engine_msg(engine_msg);

  m_lock_state->lock();
  m_filter_state = AM_VIDEO_SRC_WAITING;
  m_lock_state->unlock();

  m_event->wait();

  m_run = true;

  m_lock_state->lock();
  m_filter_state = AM_VIDEO_SRC_RUNNING;
  m_lock_state->unlock();

  while(m_run) {
    AMPacket *packet = nullptr;
    if (AM_LIKELY(!m_packet_pool->alloc_packet(packet, 0))) {
      if (AM_LIKELY(!m_run)) {
        /* !m_run means stop is called */
        INFO("Stop is called!");
      } else {
        /* otherwise, allocating packet is wrong */
        ERROR("Failed to allocate packet!");
      }
      break;
    } else {
      bool isOK = true;
      AM_RESULT result = m_video_reader->query_video_frame(
          *m_frame_desc, m_vconfig->query_op_timeout);
      if (AM_LIKELY(AM_RESULT_OK == result)) {
        packet->set_attr(AMPacket::AM_PAYLOAD_ATTR_VIDEO);
        switch(m_frame_desc->type) {
          case AM_DATA_FRAME_TYPE_VIDEO: {
            AMVideoFrameDesc &video = m_frame_desc->video;
            if (AM_LIKELY((video.type != AM_VIDEO_FRAME_TYPE_SJPEG) &&
                          (video.type != AM_VIDEO_FRAME_TYPE_NONE))) {
              isOK = process_video(*m_frame_desc, packet);
              if (AM_LIKELY(isOK)) {
                send_packet(packet);
              } else {
                packet->release();
              }
            } else {
              ERROR("Invalid video type: %d", video.type);
            }
          }break;
          case AM_DATA_FRAME_TYPE_YUV:
          case AM_DATA_FRAME_TYPE_ME1:
          case AM_DATA_FRAME_TYPE_RAW:
          case AM_DATA_FRAME_TYPE_VCA:
          default: {
            packet->release();
            NOTICE("Invalid frame type: %d", m_frame_desc->type);
          }break;
        }
      } else if (AM_RESULT_ERR_AGAIN == result) {
        packet->release();
      } else {
        isOK = false;
        packet->release();
        ERROR("Error occurred! Abort!");
      }
      if (AM_UNLIKELY(!isOK)) {
        break;
      }
    }
  }

  m_lock_state->lock();
  m_filter_state = AM_VIDEO_SRC_STOPPED;
  m_lock_state->unlock();

  m_event->clear();
  if (AM_LIKELY(!m_run)) {
    NOTICE("%s exits mainloop!", m_name);
  } else {
    NOTICE("%s aborted!", m_name);
    engine_msg.code = AMIEngine::ENG_MSG_ABORT;
    post_engine_msg(engine_msg);
  }
}

AMVideoSource::AMVideoSource(AMIEngine *engine) :
    inherited(engine),
    m_vconfig(nullptr),
    m_config(nullptr),
    m_packet_pool(nullptr),
    m_lock(nullptr),
    m_lock_state(nullptr),
    m_event(nullptr),
    m_frame_desc(nullptr),
    m_video_reader(nullptr),
    m_input_num(0),
    m_output_num(0),
    m_filter_state(AM_VIDEO_SRC_CREATED),
    m_run(false)
{
  m_video_info.clear();
  m_last_pts.clear();
  m_outputs.clear();
}

AMVideoSource::~AMVideoSource()
{
  AM_DESTROY(m_packet_pool);
  AM_DESTROY(m_lock);
  AM_DESTROY(m_lock_state);
  AM_DESTROY(m_event);
  for (uint32_t i = 0; i < m_outputs.size(); ++ i) {
    AM_DESTROY(m_outputs[i]);
  }
  m_outputs.clear();
  delete m_config;
  delete m_frame_desc;
  m_video_reader = nullptr;
  m_video_address = nullptr;
  for (uint32_t i = 0; i < m_video_info.size(); ++ i) {
    delete m_video_info[i];
    m_video_info[i] = nullptr;
  }
  m_video_info.clear();

  for (uint32_t i = 0; i < m_last_pts.size(); ++ i) {
    delete m_last_pts[i];
    m_last_pts[i] = nullptr;
  }
  m_last_pts.clear();
}

AM_STATE AMVideoSource::init(const std::string& config,
                             uint32_t input_num,
                             uint32_t output_num)
{
  AM_STATE state = AM_STATE_OK;
  m_input_num = input_num;
  m_output_num = output_num;
  do {
    m_config = new AMVideoSourceConfig();
    if (AM_UNLIKELY(!m_config)) {
      ERROR("Failed to create config module for VideoSource filter!");
      state = AM_STATE_NO_MEMORY;
      break;
    }

    m_vconfig = m_config->get_config(config);
    if (AM_UNLIKELY(!m_vconfig)) {
      ERROR("Can not get configuration from file %s, please check!",
            config.c_str());
      state = AM_STATE_ERROR;
      break;
    }

    m_lock = AMSpinLock::create();
    if (AM_UNLIKELY(!m_lock)) {
      ERROR("Failed to create lock!");
      state = AM_STATE_ERROR;
      break;
    }

    m_lock_state = AMSpinLock::create();
    if (AM_UNLIKELY(!m_lock_state)) {
      ERROR("Failed to create lock for filter state!");
      state = AM_STATE_ERROR;
      break;
    }

    m_event = AMEvent::create();
    if (AM_UNLIKELY(!m_event)) {
      ERROR("Failed to create event!");
      state = AM_STATE_ERROR;
      break;
    }

    state = inherited::init((const char*)m_vconfig->name.c_str(),
                            m_vconfig->real_time.enabled,
                            m_vconfig->real_time.priority);
    if (AM_LIKELY(AM_STATE_OK != state)) {
      break;
    } else {
      std::string poolName = m_vconfig->name + ".packet.pool";
      m_packet_pool = AMFixedPacketPool::create(poolName.c_str(),
                                                m_vconfig->packet_pool_size,
                                                0);
      if (AM_UNLIKELY(!m_packet_pool)) {
        ERROR("Failed to create packet pool for %s", m_vconfig->name.c_str());
        state = AM_STATE_ERROR;
        break;
      }

      if (AM_UNLIKELY(m_input_num)) {
        WARN("%s should not have input, but input num is %u, reset to 0!",
             m_name, m_input_num);
        m_input_num = 0;
      }

      if (AM_UNLIKELY(0 == m_output_num)) {
        WARN("%s should have at least 1 output, but output num is 0, "
             "reset to 1!", m_name);
        m_output_num = 1;
      }

      for (uint32_t i = 0; i < m_output_num; ++ i) {
        AMVideoSourceOutput *output = AMVideoSourceOutput::create(this);
        if (AM_UNLIKELY(!output)) {
          ERROR("Failed to create output pin[%u] for %s", i, m_name);
          state = AM_STATE_ERROR;
          break;
        }
        m_outputs.push_back(output);
      }
      if (AM_UNLIKELY(AM_STATE_OK != state)) {
        break;
      }
    }

    m_video_reader = AMIVideoReader::get_instance();
    if (!m_video_reader) {
      ERROR("Failed to get instance of VideoReader!");
      state = AM_STATE_ERROR;
      break;
    }

    m_video_address = AMIVideoAddress::get_instance();
    if (!m_video_address) {
      ERROR("Failed to get instance of VideoAddress!");
      state = AM_STATE_ERROR;
      break;
    }

    m_frame_desc = new AMQueryFrameDesc();
    if (AM_UNLIKELY(!m_frame_desc)) {
      ERROR("Failed to create frame description object!");
      state = AM_STATE_NO_MEMORY;
      break;
    }
    memset(m_frame_desc, 0, sizeof(*m_frame_desc));
    m_lock_state->lock();
    m_filter_state = AM_VIDEO_SRC_INITTED;
    m_lock_state->unlock();
  }while(0);

  return state;
}

AM_VIDEO_INFO* AMVideoSource::find_video_info(uint32_t streamid)
{
  AM_VIDEO_INFO *info = nullptr;
  for (uint32_t i = 0; i < m_video_info.size(); ++ i) {
    if (AM_LIKELY(m_video_info[i]->stream_id == streamid)) {
      info = m_video_info[i];
      break;
    }
  }

  return info;
}

AMVideoPtsInfo* AMVideoSource::find_video_last_pts(uint32_t streamid)
{
  AMVideoPtsInfo *pts = nullptr;

  for (uint32_t i = 0; i < m_last_pts.size(); ++ i) {
    if (AM_LIKELY(m_last_pts[i]->streamid == streamid)) {
      pts = m_last_pts[i];
      break;
    }
  }

  return pts;
}

static const char* video_type_to_string(AM_VIDEO_TYPE type)
{
  const char *str = "Unknown";
  switch(type) {
    case AM_VIDEO_H264  : str = "H.264"; break;
    case AM_VIDEO_H265  : str = "H.265"; break;
    case AM_VIDEO_MJPEG : str = "MJPEG"; break;
    case AM_VIDEO_NULL:
    default:break;
  }
  return str;
}

bool AMVideoSource::process_video(AMQueryFrameDesc &frame,
                                  AMPacket *&packet)
{
  bool ret = true;
  AMVideoFrameDesc  &video = frame.video;
  AM_VIDEO_INFO     *vinfo = find_video_info(video.stream_id);
  AMVideoPtsInfo *last_pts = find_video_last_pts(video.stream_id);

  do {
    if (AM_UNLIKELY(!vinfo)) {
      /* Add video info for this stream id, if it doesn't exist */
      AM_VIDEO_INFO *video_info = new AM_VIDEO_INFO();
      if (AM_LIKELY(video_info)) {
        memset(video_info, 0, sizeof(*video_info));
        video_info->stream_id = video.stream_id;
        video_info->type = AM_VIDEO_NULL;
        /* Add Video info for current stream */
        m_video_info.push_back(video_info);
        vinfo = video_info;
      } else {
        ERROR("Failed to allocate AM_VIDEO_INFO for stream%u", video.stream_id);
        ret = false;
        break;
      }
    }

    if (AM_UNLIKELY(!last_pts)) {
      /* Add one for this stream id, if it doesn't exist */
      AMVideoPtsInfo *pts_info = new AMVideoPtsInfo();
      if (AM_LIKELY(pts_info)) {
        pts_info->streamid = video.stream_id;
        pts_info->pts = (int64_t)frame.pts;
        m_last_pts.push_back(pts_info);
        last_pts = find_video_last_pts(video.stream_id);
      } else {
        ERROR("Failed to allocate AMVideoPtsInfo for stream%u",
              video.stream_id);
        ret = false;
        break;
      }
    }

    last_pts->pts = (int64_t)frame.pts;
    if (AM_UNLIKELY(video.stream_end_flag)) {
      packet->set_type(AMPacket::AM_PAYLOAD_TYPE_EOS);
      packet->set_attr(AMPacket::AM_PAYLOAD_ATTR_VIDEO);
      packet->set_data_ptr(nullptr);
      packet->set_data_size(0);
      packet->set_pts(last_pts->pts);
      packet->set_stream_id(video.stream_id);
      packet->set_frame_type(vinfo->type);
      last_pts->is_eos = true;
    } else {
      bool send_info = false;
      bool is_new_session = false;
      AM_VIDEO_TYPE vtype = AM_VIDEO_NULL;
      AMAddress vaddr = {0};

      if (AM_UNLIKELY(m_video_address->video_addr_get(frame, vaddr) !=
                      AM_RESULT_OK)) {
        ERROR("Failed to get video data address!");
        ret = false;
        break;
      }
      switch(video.type) {
        case AM_VIDEO_FRAME_TYPE_MJPEG: {
          vtype = AM_VIDEO_MJPEG;
        }break;
        case AM_VIDEO_FRAME_TYPE_H264_IDR:
        case AM_VIDEO_FRAME_TYPE_H264_I:
        case AM_VIDEO_FRAME_TYPE_H264_B:
        case AM_VIDEO_FRAME_TYPE_H264_P: {
          vtype = AM_VIDEO_H264;
        }break;
        /* todo: H.265 ? */
        default:break; /* Won't come here */
      }
      is_new_session = m_video_address->is_new_video_session(
          video.session_id,
          AM_STREAM_ID(video.stream_id));
      send_info = (((vinfo->type != vtype) && (vtype != AM_VIDEO_NULL)) ||
                    (vinfo->width != video.width) ||
                    (vinfo->height != video.height));

      if (AM_UNLIKELY(send_info || is_new_session)) {
        AMPacket *info = nullptr;
        if (AM_LIKELY(!m_packet_pool->alloc_packet(info, 0))) {
          if (AM_LIKELY(!m_run)) {
            /* !m_run means stop is called */
            INFO("Stop is called!");
            break;
          } else {
            /* otherwise, allocating packet is wrong */
            ERROR("Failed to allocate packet!");
            ret = false;
            break;
          }
        } else {
          AMStreamInfo stream_info;
          stream_info.stream_id = AM_STREAM_ID(video.stream_id);
          if (AM_LIKELY(AM_RESULT_OK ==
              m_video_reader->query_stream_info(stream_info))) {
            vinfo->type   = vtype;
            vinfo->width  = video.width;
            vinfo->height = video.height;
            vinfo->M      = stream_info.m;
            vinfo->N      = stream_info.n;
            vinfo->mul    = stream_info.mul;
            vinfo->div    = stream_info.div;
            vinfo->rate   = stream_info.rate;
            vinfo->scale  = stream_info.scale;
            info->set_type(AMPacket::AM_PAYLOAD_TYPE_INFO);
            info->set_attr(AMPacket::AM_PAYLOAD_ATTR_VIDEO);
            info->set_data_ptr((uint8_t*)vinfo);
            info->set_data_size(sizeof(*vinfo));
            info->set_stream_id(video.stream_id);
            INFO("\nVideo Stream%u Information:"
                 "\nSessionID: %08X"
                 "\n    Width: %u"
                 "\n   Height: %u"
                 "\n     Type: %s"
                 "\n        M: %hu"
                 "\n        N: %hu"
                 "\n     Rate: %u"
                 "\n    Scale: %u"
                 "\n      Mul: %hu"
                 "\n      Div: %hu",
                 video.stream_id,
                 video.session_id,
                 video.width,
                 video.height,
                 video_type_to_string(vtype),
                 vinfo->M,
                 vinfo->N,
                 vinfo->rate,
                 vinfo->scale,
                 vinfo->mul,
                 vinfo->div);
            send_packet(info);
          } else {
            info->release();
            ret = false;
            break;
          }
        }
      }
      packet->set_type(AMPacket::AM_PAYLOAD_TYPE_DATA);
      packet->set_attr(AMPacket::AM_PAYLOAD_ATTR_VIDEO);
      packet->set_pts((int64_t)frame.pts);
      packet->set_stream_id(video.stream_id);
      packet->set_frame_attr(video.jpeg_quality);
      packet->set_frame_type(video.type);
      packet->set_data_size(video.data_size);
      packet->set_data_ptr(vaddr.data);
      packet->set_addr_offset(video.data_offset);
      packet->set_frame_count(1);
      last_pts->is_eos = false;
    }
  } while(0);

  return ret;
}

void AMVideoSource::send_packet(AMPacket *packet)
{
  if (AM_LIKELY(packet)) {
    for (uint32_t i = 0; i < m_output_num; ++ i) {
      packet->add_ref();
      m_outputs[i]->send_packet(packet);
    }
    packet->release();
  }
}

