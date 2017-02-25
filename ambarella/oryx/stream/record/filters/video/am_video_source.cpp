/*******************************************************************************
 * am_video_source.cpp
 *
 * History:
 *   Sep 11, 2015 - [ypchang] created file
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

#include "am_video_reader_if.h"
#include "am_video_address_if.h"
#include "am_video_source_if.h"
#include "am_video_source.h"
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

struct AMVideoFrameNumInfo
{
    uint32_t  frame_num;
    uint32_t  streamid;
    AMVideoFrameNumInfo() :
      frame_num(0),
      streamid(0)
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
  AUTO_MEM_LOCK(m_lock);
  m_lock_state.lock();
  if (AM_LIKELY((m_filter_state == AM_VIDEO_SRC_STOPPED) ||
                (m_filter_state == AM_VIDEO_SRC_INITTED) ||
                (m_filter_state == AM_VIDEO_SRC_WAITING))) {
    m_packet_pool->enable(true);
    m_event->signal();
  }
  m_lock_state.unlock();
  return AM_STATE_OK;
}

AM_STATE AMVideoSource::stop()
{
  AUTO_MEM_LOCK(m_lock);
  m_lock_state.lock();
  if (AM_UNLIKELY(m_filter_state == AM_VIDEO_SRC_WAITING)) {
    /* on_run is blocked on m_event->wait(), need unblock it */
    m_event->signal();
  }
  m_lock_state.unlock();
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

  m_lock_state.lock();
  m_filter_state = AM_VIDEO_SRC_WAITING;
  m_lock_state.unlock();

  m_event->wait();

  m_run = true;

  m_lock_state.lock();
  m_filter_state = AM_VIDEO_SRC_RUNNING;
  m_lock_state.unlock();

  while(m_run.load()) {
    AMPacket *packet = nullptr;
    if (AM_LIKELY(!m_packet_pool->alloc_packet(packet, 0))) {
      if (AM_LIKELY(!m_run.load())) {
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
            if (AM_LIKELY(video.type != AM_VIDEO_FRAME_TYPE_SJPEG)) {
              //check_video_pts_and_frame_num(*m_frame_desc);
              isOK = process_video(*m_frame_desc, packet);
              if (AM_LIKELY(isOK)) {
                packet->add_ref();
                send_packet(packet);
              }
            } else {
              WARN("Invalid video type: %d", video.type);
            }
          }break;
          case AM_DATA_FRAME_TYPE_YUV:
          case AM_DATA_FRAME_TYPE_ME1:
          case AM_DATA_FRAME_TYPE_RAW:
          case AM_DATA_FRAME_TYPE_VCA:
          default: {
            NOTICE("Invalid frame type: %d", m_frame_desc->type);
          }break;
        }
      } else if (AM_RESULT_ERR_AGAIN != result) {
        isOK = false;
        ERROR("Error occurred! Abort!");
      }
      packet->release();
      if (AM_UNLIKELY(!isOK)) {
        break;
      }
    }
  }

  m_lock_state.lock();
  m_filter_state = AM_VIDEO_SRC_STOPPED;
  m_lock_state.unlock();

  m_event->clear();
  if (AM_LIKELY(!m_run.load())) {
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
    m_event(nullptr),
    m_frame_desc(nullptr),
    m_video_reader(nullptr),
    m_video_address(nullptr),
    m_input_num(0),
    m_output_num(0),
    m_filter_state(AM_VIDEO_SRC_CREATED),
    m_run(false)
{
  m_video_info.clear();
  m_last_pts.clear();
  m_last_frame_num.clear();
  m_outputs.clear();
}

AMVideoSource::~AMVideoSource()
{
  AM_DESTROY(m_packet_pool);
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
  for (uint32_t i = 0; i < m_last_frame_num.size(); ++ i) {
    delete m_last_frame_num[i];
    m_last_frame_num[i] = nullptr;
  }
  m_last_pts.clear();
  m_last_frame_num.clear();
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
    m_lock_state.lock();
    m_filter_state = AM_VIDEO_SRC_INITTED;
    m_lock_state.unlock();
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

AMVideoFrameNumInfo* AMVideoSource::find_video_last_frame_num(uint32_t streamid)
{
  AMVideoFrameNumInfo *frame_num = nullptr;

  for (uint32_t i = 0; i < m_last_frame_num.size(); ++ i) {
    if (AM_LIKELY(m_last_frame_num[i]->streamid == streamid)) {
      frame_num = m_last_frame_num[i];
      break;
    }
  }
  return frame_num;
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
  AMVideoFrameNumInfo *last_frame_num = find_video_last_frame_num(video.stream_id);

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
        pts_info->pts = frame.pts;
        m_last_pts.push_back(pts_info);
        last_pts = find_video_last_pts(video.stream_id);
      } else {
        ERROR("Failed to allocate AMVideoPtsInfo for stream%u",
              video.stream_id);
        ret = false;
        break;
      }
    }

    if (AM_UNLIKELY(!last_frame_num)) {
      /* Add one for this stream id, if it doesn't exist */
      AMVideoFrameNumInfo *frame_num_info = new AMVideoFrameNumInfo();
      if (AM_LIKELY(frame_num_info)) {
        frame_num_info->streamid = video.stream_id;
        frame_num_info->frame_num = video.frame_num;
        m_last_frame_num.push_back(frame_num_info);
        last_frame_num = find_video_last_frame_num(video.stream_id);
      } else {
        ERROR("Failed to allocate AMVideoFrameNumInfo for stream%u",
              video.stream_id);
        ret = false;
        break;
      }
    }

    last_frame_num->frame_num = video.frame_num;
    if (AM_UNLIKELY(video.stream_end_flag)) {
      packet->set_type(AMPacket::AM_PAYLOAD_TYPE_EOS);
      packet->set_attr(AMPacket::AM_PAYLOAD_ATTR_VIDEO);
      packet->set_data_ptr(nullptr);
      packet->set_data_size(0);
      packet->set_pts((frame.pts < 0) ? last_pts->pts : frame.pts);
      packet->set_video_type(vinfo->type);
      packet->set_stream_id(video.stream_id);
      last_pts->pts = frame.pts;
      last_pts->is_eos = true;
    } else if (AM_LIKELY(video.type != AM_VIDEO_FRAME_TYPE_NONE)) {
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
      switch(video.stream_type) {
        case AM_STREAM_TYPE_MJPEG: {
          vtype = AM_VIDEO_MJPEG;
        }break;
        case AM_STREAM_TYPE_H264: {
          vtype = AM_VIDEO_H264;
        }break;
        case AM_STREAM_TYPE_H265: {
          vtype = AM_VIDEO_H265;
        }break;
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
          if (AM_LIKELY(!m_run.load())) {
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
            vinfo->type      = vtype;
            vinfo->width     = video.width;
            vinfo->height    = video.height;
            vinfo->slice_num = video.tile_slice.slice_num;
            vinfo->tile_num  = video.tile_slice.tile_num;
            vinfo->M         = stream_info.m;
            vinfo->N         = stream_info.n;
            vinfo->mul       = stream_info.mul;
            vinfo->div       = stream_info.div;
            vinfo->rate      = stream_info.rate;
            vinfo->scale     = stream_info.scale;
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
                 "\nSlice Num: %hu"
                 "\n Tile Num: %hu"
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
                 vinfo->slice_num,
                 vinfo->tile_num,
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
      packet->set_pts(frame.pts);
      packet->set_stream_id(video.stream_id);
      packet->set_frame_attr(video.jpeg_quality);
      packet->set_frame_type(video.type);
      packet->set_video_type(vtype);
      packet->set_data_size(video.data_size);
      packet->set_data_ptr(vaddr.data);
      packet->set_addr_offset(video.data_offset);
      packet->set_frame_number(video.frame_num);
      packet->set_frame_count(((video.tile_slice.slice_num << 24) |
                               (video.tile_slice.slice_id  << 16) |
                               (video.tile_slice.tile_num  <<  8) |
                               video.tile_slice.tile_id));
      last_pts->is_eos = false;
      last_pts->pts = frame.pts;
    } else {
      ret = false;
      ERROR("Invalid video type!");
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

void AMVideoSource::check_video_pts_and_frame_num(AMQueryFrameDesc &frame)
{
  AMVideoFrameDesc  &video = frame.video;
  AMVideoPtsInfo *last_pts = find_video_last_pts(video.stream_id);
  AMVideoFrameNumInfo *last_frame_num = find_video_last_frame_num(video.stream_id);
  do {/*for invalid pts*/
    if ((video.type == AM_VIDEO_FRAME_TYPE_MJPEG) ||
        (video.type == AM_VIDEO_FRAME_TYPE_SJPEG) ||
        (video.type == AM_VIDEO_FRAME_TYPE_NONE)) {
      break;
    }
    if (last_pts && !video.stream_end_flag) {
      if ((frame.pts - last_pts->pts) < 0) { // pts diff < 0
        if (video.type == AM_VIDEO_FRAME_TYPE_B) {
          break;
        } else {
          WARN("\n Stream%u Video PTS abnormal!!!"
               "\n          last PTS: %lld"
               "\n       Current PTS: %lld"
               "\n          PTS Diff: %lld"
               "\n    Last Frame Num: %u"
               "\n Current Frame Num: %u,"
               "\n    Frame Num Diff: %d",
               video.stream_id,
               last_pts->pts,
               frame.pts,
               (frame.pts - last_pts->pts),
               last_frame_num->frame_num,
               video.frame_num,
               (int32_t )(video.frame_num - last_frame_num->frame_num));
          break;
        }
      }
      if ((frame.pts - last_pts->pts) > 10000) { // pts diff too large
        WARN("\n Stream%u Video PTS abnormal!!!"
             "\n          last PTS: %lld"
             "\n       Current PTS: %lld"
             "\n          PTS Diff: %lld"
             "\n    Last Frame Num: %u"
             "\n Current Frame Num: %u,"
             "\n    Frame Num Diff: %d",
             video.stream_id,
             last_pts->pts,
             frame.pts,
             (frame.pts - last_pts->pts),
             last_frame_num->frame_num,
             video.frame_num,
             (int32_t )(video.frame_num - last_frame_num->frame_num));
        break;
      } // pts diff too large
    }
#if 0
    if (last_frame_num) {
      if ((video.frame_num - last_frame_num->frame_num) != 1) {
        WARN("video frame num abnormal, stream %u, last pts : %lld, current pts : %lld,"
             "pts diff : %lld, last frame number : %u, current frame number"
             " : %u, frame number diff : %d", video.stream_id,
             last_pts->pts, frame.pts, (frame.pts - last_pts->pts),
             last_frame_num->frame_num, video.frame_num,
             (int32_t )(video.frame_num - last_frame_num->frame_num));
        break;
      }
    }
#endif
  } while (0);
}
