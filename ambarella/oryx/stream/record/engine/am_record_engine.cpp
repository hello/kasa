/*******************************************************************************
 * am_record_engine.cpp
 *
 * History:
 *   2014-12-30 - [ypchang] created file
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
#include "am_amf_engine_frame.h"

#include "am_record_engine.h"
#include "am_record_engine_config.h"
#include "am_gps_source_if.h"
#include "am_gsensor_source_if.h"
#include "am_audio_source_if.h"
#include "am_video_source_if.h"
#include "am_event_sender_if.h"
#include "am_av_queue_if.h"
#include "am_muxer_if.h"

#include "am_mutex.h"
#include "am_event.h"
#include "am_thread.h"
#include "am_plugin.h"

#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <signal.h>

AMIRecordEngine* AMIRecordEngine::create()
{
  std::string conf(ORYX_CONF_DIR);
  conf.append("/stream/engine/record-engine.acs");

  return (AMIRecordEngine*)AMRecordEngine::create(conf);
}

AMRecordEngine* AMRecordEngine::create(const std::string& config)
{
  AMRecordEngine *result = new AMRecordEngine();
  if (AM_UNLIKELY(result && (AM_STATE_OK != result->init(config)))) {
    delete result;
    result = NULL;
  }

  return result;
}

void* AMRecordEngine::get_interface(AM_REFIID ref_iid)
{
  return (IID_AMIRecordEngine == ref_iid) ? ((AMIRecordEngine*)this) :
      inherited::get_interface(ref_iid);
}

void AMRecordEngine::destroy()
{
  inherited::destroy();
}

AMIRecordEngine::AM_RECORD_ENGINE_STATUS AMRecordEngine::get_engine_status()
{
  AUTO_MEM_LOCK(m_lock);
  return m_status;
}

bool AMRecordEngine::create_graph()
{
  if (AM_LIKELY(!m_graph_created)) {
    bool ret = true;

    for (uint32_t i = 0; i < m_engine_config->filter_num; ++ i) {
      ConnectionConfig& connection = m_engine_config->connections[i];
      AMIPacketFilter *this_filter = get_filter_by_name(connection.filter);
      if (AM_LIKELY(this_filter)) {
        for (uint32_t j = 0; j < connection.input_number; ++ j) {
          /* This filter has up stream filter[s] */
          ConnectionConfig *&in_conn = connection.input[j];
          AMIPacketFilter *in_filter = get_filter_by_name(in_conn->filter);
          if (AM_LIKELY(in_filter)) {
            AMIPacketPin  *in_pin = this_filter->get_input_pin(j);
            AMIPacketPin *out_pin = NULL;
            uint32_t k = 0;
            for (k = 0; k < in_conn->output_number; ++ k) {
              out_pin = in_filter->get_output_pin(k);
              if (AM_LIKELY(out_pin && !out_pin->is_connected())) {
                break;
              } else {
                out_pin = NULL;
              }
            }
            if (AM_LIKELY(out_pin && in_pin)) {
              NOTICE("Filter[%20s] output pin[%u] ==> "
                     "Filter[%20s] input pin[%u]",
                     out_pin->filter_name(), k,
                     in_pin->filter_name(), j);
              ret = (AM_STATE_OK == create_connection(out_pin, in_pin));
            } else {
              if (AM_LIKELY(!out_pin)) {
                ERROR("Failed to get an available output pin from %s",
                      in_conn->filter.c_str());
              }
              if (AM_LIKELY(!in_pin)) {
                ERROR("Failed to get input pin[%u] from %s",
                      j, connection.filter.c_str());
              }
              ret = false;
            }
          } else {
            ERROR("Failed to get AMIPacketFilter interface from %s",
                  in_conn->filter.c_str());
            ret = false;
          }
          if (AM_UNLIKELY(!ret)) {
            break;
          }
        }
        if (AM_UNLIKELY(!ret)) {
          break;
        }
      } else {
        ERROR("Failed to get AMIPacketFilter interface from %s",
              connection.filter.c_str());
        ret = false;
        break;
      }
    }
    m_graph_created = ret;
    if (AM_LIKELY(m_graph_created)) {
      m_thread = AMThread::create("RecordENGLoop", static_mainloop, this);
      if (AM_UNLIKELY(!m_thread)) {
        delete_all_connections();
        remove_all_filters();
        m_graph_created = false;
      }
    }
  }

  return m_graph_created;
}

bool AMRecordEngine::record()
{
  bool ret = false;
  uint32_t count = 0;
  while(!m_mainloop_run.load() && (count < 30)) {
    m_sem->wait(100000);
    ++ count;
  }
  if(AM_LIKELY(m_mainloop_run.load())) {
    ret = send_engine_cmd(AM_ENGINE_CMD_START);
  } else {
    ERROR("The thread of engine is not running.");
    ret = false;
  }
  return ret;
}

bool AMRecordEngine::stop()
{
  bool ret = true;
  if(AM_LIKELY(m_mainloop_run.load())) {
    ret = send_engine_cmd(AM_ENGINE_CMD_STOP);
  } else {
    NOTICE("The engine is already stopped.");
  }
  return ret;
}

bool AMRecordEngine::start_file_recording(uint32_t muxer_id_bit_map)
{
  bool ret = true;
  do {
    INFO("Start muxer%u file recording in engine.", muxer_id_bit_map);
    AMFilterVector filter_vector = get_filter_vector_by_iid(IID_AMIMuxer);
    for (auto &v : filter_vector) {
      AMIMuxer *muxer = (AMIMuxer*)v;
      if (muxer) {
        if (!muxer->start_file_recording(muxer_id_bit_map)) {
          ERROR("Failed to start file recording.");
          ret = false;
          break;
        }
      } else {
        ERROR("Muxer is invalid.");
      }
    }
  } while(0);
  return ret;
}

bool AMRecordEngine::stop_file_recording(uint32_t muxer_id_bit_map)
{
  bool ret = true;
  do {
    INFO("Stop muxer%u file recording in engine.", muxer_id_bit_map);
    AMFilterVector filter_vector = get_filter_vector_by_iid(IID_AMIMuxer);
    for (auto &v : filter_vector) {
      AMIMuxer *muxer = (AMIMuxer*)v;
      if (muxer) {
        if (!muxer->stop_file_recording(muxer_id_bit_map)) {
          ERROR("Failed to stop file recording.");
          ret = false;
          break;
        }
      } else {
        ERROR("Muxer is invalid.");
      }
    }
  } while(0);
  return ret;
}

bool AMRecordEngine::start_file_muxer()
{
  bool ret = false;
  do {
    AMFilterVector muxer =
        get_filter_vector_by_iid(IID_AMIMuxer);
    for (auto &v : muxer) {
      if (((AMIMuxer*)v)->type() ==   AM_MUXER_TYPE_FILE) {
        ret = ((AMIMuxer*)v)->start_send_normal_pkt();
      }
    }
  } while(0);
  return ret;
}

bool AMRecordEngine::set_recording_file_num(uint32_t muxer_id_bit_map,
                                            uint32_t file_num)
{
  bool ret = true;
  do {
    INFO("Set recording file num :%u", file_num);
    AMFilterVector filter_vector = get_filter_vector_by_iid(IID_AMIMuxer);
    for (auto &v : filter_vector) {
      AMIMuxer *muxer = (AMIMuxer*)v;
      if (muxer) {
        if (!muxer->set_recording_file_num(muxer_id_bit_map, file_num)) {
          ERROR("Failed to set recording file num.");
          ret = false;
          break;
        }
      } else {
        ERROR("Muxer is invalid.");
      }
    }
  } while(0);
  return ret;
}

bool AMRecordEngine::set_recording_duration(uint32_t muxer_id_bit_map,
                                            int32_t duration)
{
  bool ret = true;
  do {
    INFO("Set recording duration :%u", duration);
    AMFilterVector filter_vector = get_filter_vector_by_iid(IID_AMIMuxer);
    for (auto &v : filter_vector) {
      AMIMuxer *muxer = (AMIMuxer*)v;
      if (muxer) {
        if (!muxer->set_recording_duration(muxer_id_bit_map, duration)) {
          ERROR("Failed to set recording duration.");
          ret = false;
          break;
        }
      } else {
        ERROR("Muxer is invalid.");
      }
    }
  } while(0);
  return ret;
}

bool AMRecordEngine::set_file_duration(uint32_t muxer_id_bit_map,
                                       int32_t file_duration,
                                       bool apply_conf_file)
{
  bool ret = true;
  do {
    INFO("Set file duration :%u", file_duration);
    AMFilterVector filter_vector = get_filter_vector_by_iid(IID_AMIMuxer);
    for (auto &v : filter_vector) {
      AMIMuxer *muxer = (AMIMuxer*)v;
      if (muxer) {
        if (!muxer->set_file_duration(muxer_id_bit_map, file_duration,
                                      apply_conf_file)) {
          ERROR("Failed to set file duration.");
          ret = false;
          break;
        }
      } else {
        ERROR("Muxer is invalid.");
      }
    }
  } while(0);
  return ret;
}

bool AMRecordEngine::set_muxer_param(AMMuxerParam &param)
{
  bool ret = true;
  do {
    AMFilterVector filter_vector = get_filter_vector_by_iid(IID_AMIMuxer);
    for (auto &v : filter_vector) {
      AMIMuxer *muxer = (AMIMuxer*)v;
      if (muxer) {
        if (!muxer->set_muxer_param(param)) {
          ERROR("Failed to set muxer param.");
          ret = false;
          break;
        }
      } else {
        ERROR("Muxer is invalid.");
      }
    }
  } while(0);
  return ret;
}

void AMRecordEngine::set_app_msg_callback(AMRecordCallback callback, void *data)
{
  m_app_callback = callback;
  m_app_data     = data;
}

void AMRecordEngine::set_aec_enabled(bool enabled)
{
  AMFilterVector muxer = get_filter_vector_by_iid(IID_AMIAudioSource);
  for (auto &v : muxer) {
    ((AMIAudioSource*)v)->set_aec_enabled(enabled);
  }
}

bool AMRecordEngine::set_file_operation_callback(uint32_t muxer_id_bit_map,
                                                 AM_FILE_OPERATION_CB_TYPE type,
                                                 AMFileOperationCB callback)
{
  bool ret = true;
  do {
    AMFilterVector muxer = get_filter_vector_by_iid(IID_AMIMuxer);
    for (auto &v : muxer) {
      if (!(((AMIMuxer*)v)->set_file_operation_callback(
          muxer_id_bit_map, type, callback))) {
        ERROR("Failed to set file operation callback function to muxer.");
        ret = false;
        break;
      }
    }
  } while(0);
  return ret;
}

bool AMRecordEngine::update_image_3A_info(AMImage3AInfo *image_3A)
{
  bool ret = true;
  AMFilterVector muxer = get_filter_vector_by_iid(IID_AMIMuxer);
  for (auto &v : muxer) {
    if (!((AMIMuxer*)v)->update_image_3a_info(image_3A)) {
      ERROR("Failed to update image 3A info!");
      ret = false;
    }
  }
  return ret;
}

bool AMRecordEngine::is_ready_for_event(AMEventStruct& event)
{
  bool ret = false;
  do {
    AMIAVQueue *avqueue = (AMIAVQueue*)get_filter_by_iid(IID_AMIAVQueue);
    if (avqueue) {
      ret = avqueue->is_ready_for_event(event);
    } else {
      if ((event.attr == AM_EVENT_MJPEG) ||
          (event.attr == AM_EVENT_PERIODIC_MJPEG)) {
        ret = true;
        break;
      }
      AMFilterVector muxer = get_filter_vector_by_iid(IID_AMIMuxer);
      for (auto &v : muxer) {
        AMIMuxer *muxer = (AMIMuxer*)v;
        if (muxer && muxer->type() == AM_MUXER_TYPE_FILE) {
          if (muxer->is_ready_for_event(event)) {
            ret = true;
            break;
          }
        }
      }
    }
  } while(0);
  return ret;
}

bool AMRecordEngine::send_event(AMEventStruct& event)
{
  AMIEventSender *sender =
      (AMIEventSender*)get_filter_by_iid(IID_AMIEventSender);
  std::string name;
  switch(event.attr) {
    case AM_EVENT_MJPEG : {
      name = "AM_EVENT_MJPEG";
      INFO("Send event in record engine, event attr is %s,"
          " stream id is %u, pre num is %u, after num is %u, closest num is %u",
          name.c_str(),event.mjpeg.stream_id, event.mjpeg.pre_cur_pts_num,
          event.mjpeg.after_cur_pts_num, event.mjpeg.closest_cur_pts_num);
    } break;
    case AM_EVENT_H26X : {
      name = "AM_EVENT_H26X";
      INFO("Send event in record engine, event attr is %s,"
          " stream id is %u, history duration is %u, future duration is %u",
          name.c_str(),event.h26x.stream_id, event.h26x.history_duration,
          event.h26x.future_duration);
    } break;
    case AM_EVENT_PERIODIC_MJPEG : {
      name = "AM_EVENT_PERIODIC_MJPEG";
      INFO("Send event in record engine, event attr is %s,"
          " stream id is %u, inverval second is %u, start time is %u-%u-%u"
          " end time is %u-%u-%u", name.c_str(), event.periodic_mjpeg.stream_id,
          event.periodic_mjpeg.interval_second,
          event.periodic_mjpeg.start_time_hour,
          event.periodic_mjpeg.start_time_minute,
          event.periodic_mjpeg.start_time_second,
          event.periodic_mjpeg.end_time_hour,
          event.periodic_mjpeg.end_time_minute,
          event.periodic_mjpeg.end_time_second);
    } break;
    case AM_EVENT_STOP_CMD : {
      name = "AM_EVENT_STOP_CMD";
      INFO("Send event in record engine, event attr is %s, stream_id is %u",
           name.c_str(), event.stop_cmd.stream_id);
    } break;
    default : {
      ERROR("Event attr error.");
    } break;
  }
  return sender ? sender->send_event(event) : false;
}

bool AMRecordEngine::enable_audio_codec(AM_AUDIO_TYPE type,
                                        uint32_t sample_rate, bool enable)
{
  bool ret = false;
  std::string filter_name;
  do {
    AMFilterVector audio_source_vector =
        get_filter_vector_by_iid(IID_AMIAudioSource);
    if (audio_source_vector.size() ==0) {
      ERROR("Failed to get audio source filter.");
      break;
    }
    bool find_filter = false;
    for (auto &a : audio_source_vector) {
      if (((AMIAudioSource*)a)->get_audio_sample_rate() == sample_rate) {
        find_filter = true;
        if (((AMIAudioSource*)a)->enable_codec(type, enable) == AM_STATE_OK) {
          ret = true;
        } else {
          WARN("Failed to enable codec for audio type %u, sample rate %u",
                type, sample_rate);
        }
      }
    }
    if (!find_filter) {
      ERROR("Proper audio source filters are not found.");
      break;
    }
  } while(0);
  return ret;
}

void AMRecordEngine::static_app_msg_callback(void *context, AmMsg& msg)
{
  ((AMRecordEngine*)context)->app_msg_callback(
      ((AMRecordEngine*)context)->m_app_data, msg);
}

void AMRecordEngine::app_msg_callback(void *context, AmMsg& msg)
{
  if (AM_LIKELY(m_app_callback)) {
    m_app_msg.data = context;
    m_app_msg.msg  = AM_RECORD_MSG(msg.code);
    m_app_callback(m_app_msg);
  }
}

void AMRecordEngine::msg_proc(AmMsg& msg)
{
  if (AM_LIKELY(is_session_msg(msg))) {
    const char *name = get_filter_name_by_pointer((AMIInterface*)msg.p0);
    switch(msg.code) {
      case ENG_MSG_ERROR: {
        NOTICE("Received error message from filter %s!",
               name ? name : "Unknown");
        m_status = AM_RECORD_ENGINE_ERROR;
        post_app_msg(AM_RECORD_MSG_ERROR);
      }break;
      case ENG_MSG_EOS: {
        NOTICE("Received EOS message from filter %s!",
               name ? name : "Unknown");
      }break;
      case ENG_MSG_ABORT: {
        NOTICE("Received abort request from filter %s!",
               name ? name : "Unknown");
        m_status = AM_RECORD_ENGINE_ERROR;
        send_engine_cmd(AM_ENGINE_CMD_ABORT, false);
        post_app_msg(AM_RECORD_MSG_ABORT);
      }break;
      case ENG_MSG_OK: {
        NOTICE("Received start message from filter %s!",
               name ? name : "Unknown");
      }break;
      default: {
        ERROR("Invalid message!");
        post_app_msg(AM_RECORD_MSG_NULL);
      }break;
    }
  }
}

bool AMRecordEngine::change_engine_status(
    AMIRecordEngine::AM_RECORD_ENGINE_STATUS targetStatus)
{
  bool ret = true;

  DEBUG("Target status is %u", targetStatus);
  do {
    switch(m_status) {
      case AM_RECORD_ENGINE_ERROR: {
        stop_all_filters();
        purge_all_filters();
        m_status = AM_RECORD_ENGINE_STOPPED;
        ret = false;
      }break;
      case AM_RECORD_ENGINE_TIMEOUT: {
        m_status = AM_RECORD_ENGINE_STOPPED;
        post_app_msg(AM_RECORD_MSG_TIMEOUT);
        ret = false;
      }break;
      case AM_RECORD_ENGINE_RECORDING: {
        switch(targetStatus) {
          case AM_RECORD_ENGINE_STOPPED:
          case AM_RECORD_ENGINE_ABORT: {
            AMFilterVector asrcv =
                get_filter_vector_by_iid(IID_AMIAudioSource);
            AMFilterVector vsrcv =
                get_filter_vector_by_iid(IID_AMIVideoSource);
            AMIGsensorSource *gsensor_src =
                (AMIGsensorSource*)get_filter_by_iid(IID_AMIGsensorSource);
            AMIGpsSource *gps_src =
                (AMIGpsSource*)get_filter_by_iid(IID_AMIGpsSource);
            for (auto &a : asrcv) {
              ((AMIAudioSource*)a)->stop();
            }
            for (auto &v : vsrcv) {
              ((AMIVideoSource*)v)->stop();
            }
            if (gsensor_src) {
              gsensor_src->stop();
            }
            if (gps_src) {
              gps_src->stop();
            }
            m_status = AM_RECORD_ENGINE_STOPPING;
            /* Filter's stopping sequence is from the last created filter to
             * the first created filter;
             * Filter's creating sequence is defined in engine config ACS file
             * in "filters" table, the very first one in the "filters" table is
             * the first created filter.
             */
            stop_all_filters();
            purge_all_filters();
            m_status = AM_RECORD_ENGINE_STOPPED;
            post_app_msg((targetStatus == AM_RECORD_ENGINE_ABORT) ?
                AM_RECORD_MSG_ABORT : AM_RECORD_MSG_STOP_OK);
          }break;
          case AM_RECORD_ENGINE_RECORDING: {
            NOTICE("Already recording!");
          }break;
          default: {
            ERROR("Invalid operation when engine is recording!");
          }break;
        }
      }break;
      case AM_RECORD_ENGINE_STOPPED: {
        switch(targetStatus) {
          case AM_RECORD_ENGINE_RECORDING: {
            m_status = AM_RECORD_ENGINE_STARTING;
            if (AM_LIKELY(AM_STATE_OK == run_all_filters())) {
              AMFilterVector asrcv =
                  get_filter_vector_by_iid(IID_AMIAudioSource);
              AMFilterVector vsrcv =
                  get_filter_vector_by_iid(IID_AMIVideoSource);
              AMIGsensorSource *gsensor_src =
                  (AMIGsensorSource*)get_filter_by_iid(IID_AMIGsensorSource);
              AMIGpsSource *gps_src =
                  (AMIGpsSource*)get_filter_by_iid(IID_AMIGpsSource);
              if (AM_UNLIKELY(asrcv.empty() && vsrcv.empty())) {
                ERROR("Both audio and video source filters are not loaded!");
                m_status = AM_RECORD_ENGINE_ERROR;
                post_app_msg(AM_RECORD_MSG_ERROR);
              } else {
                bool audio_ok = false;
                bool video_ok = false;
                for (auto &a : asrcv) {
                  audio_ok = ((AM_STATE_OK == ((AMIAudioSource*)a)->start()) ||
                              audio_ok);
                }
                for (auto &v : vsrcv) {
                  video_ok = ((AM_STATE_OK == ((AMIVideoSource*)v)->start()) ||
                              video_ok);
                }

                if (gsensor_src) {
                  if (AM_STATE_OK != gsensor_src->start()) {
                    ERROR("Failed to start gsensor source filter");
                  }
                }
                if (gps_src) {
                  if (AM_STATE_OK != gps_src->start()) {
                    ERROR("Failed to start gps source filter");
                  }
                }
                if (AM_LIKELY(!asrcv.empty() && !audio_ok)) {
                  ERROR("Failed to start audio source!");
                }
                if (AM_LIKELY(!vsrcv.empty() && !video_ok)) {
                  ERROR("Failed to start video source!");
                }
                if (AM_UNLIKELY((!asrcv.empty() && !audio_ok) ||
                                (!vsrcv.empty() && !video_ok))) {
                  m_status = AM_RECORD_ENGINE_ERROR;
                  post_app_msg(AM_RECORD_MSG_ERROR);
                } else {
                  m_status = AM_RECORD_ENGINE_RECORDING;
                  post_app_msg(AM_RECORD_MSG_START_OK);
                }
              }
            } else {
              ERROR("Failed to run all filters!");
              m_status = AM_RECORD_ENGINE_ERROR;
            }
          }break;
          case AM_RECORD_ENGINE_STOPPED: {
            NOTICE("Already stopped!");
          }break;
          case AM_RECORD_ENGINE_ABORT:
          default: {
            ERROR("Invalid operation when engine is stopped!");
            m_status = AM_RECORD_ENGINE_ERROR;
          }break;
        }
      }break;
      /* Intermediate status */
      case AM_RECORD_ENGINE_STARTING: {
        uint32_t count = 0;
        do {
          if (AM_LIKELY((m_status == AM_RECORD_ENGINE_RECORDING) ||
                        (m_status == AM_RECORD_ENGINE_ERROR))) {
            if (AM_UNLIKELY(m_status == AM_RECORD_ENGINE_ERROR)) {
              ERROR("Failed to start recording!");
            }
            break;
          }
          usleep(100000);
          ++ count;
        }while(count < m_engine_config->op_timeout * 10);
        if (AM_UNLIKELY((count >= m_engine_config->op_timeout*10) &&
                        (m_status == AM_RECORD_ENGINE_STARTING))) {
          ERROR("START operation timed out!");
          m_status = AM_RECORD_ENGINE_TIMEOUT;
        }
      }break;
      case AM_RECORD_ENGINE_STOPPING: {
        uint32_t count = 0;
        do {
          if (AM_LIKELY((m_status == AM_RECORD_ENGINE_STOPPED) ||
                        (m_status == AM_RECORD_ENGINE_ERROR))) {
            if (AM_UNLIKELY(m_status == AM_RECORD_ENGINE_ERROR)) {
              ERROR("Failed to stop!");
            }
            break;
          }
          usleep(100000);
          ++ count;
        }while(count < m_engine_config->op_timeout * 10);
        if (AM_UNLIKELY((count >= m_engine_config->op_timeout*10) &&
                        (m_status == AM_RECORD_ENGINE_STOPPING))) {
          ERROR("STOP operation timed out!");
          m_status = AM_RECORD_ENGINE_TIMEOUT;
        }
      }break;
      default: {
        ret = false;
        ERROR("Invalid engine status!");
      }break;
    }
  } while(ret && (m_status != targetStatus));

  if (AM_LIKELY(ret && (m_status == targetStatus))) {
    DEBUG("Target %u finished!", targetStatus);
  } else {
    DEBUG("Current status %u, target status %u", m_status, targetStatus);
  }

  return ret;
}

AMRecordEngine::AMRecordEngine() :
    m_config(nullptr),
    m_engine_config(nullptr),
    m_engine_filter(nullptr),
    m_app_data(nullptr),
    m_thread(nullptr),
    m_event(nullptr),
    m_sem(nullptr),
    m_app_callback(nullptr),
    m_status(AMIRecordEngine::AM_RECORD_ENGINE_STOPPED),
    m_graph_created(false),
    m_mainloop_run(false)
{
  MSG_R = -1;
  MSG_W = -1;
}

AMRecordEngine::~AMRecordEngine()
{
  send_engine_cmd(AM_ENGINE_CMD_EXIT);
  clear_graph();/* Must be called in the destructor of sub-class */
  delete m_config;
  delete[] m_engine_filter;
  AM_DESTROY(m_thread);
  AM_DESTROY(m_event);
  AM_DESTROY(m_sem);
  if (AM_LIKELY(MSG_R)) {
    close(MSG_R);
  }
  if (AM_LIKELY(MSG_W)) {
    close(MSG_W);
  }
  DEBUG("~AMRecordEngine");
}

AM_STATE AMRecordEngine::init(const std::string& config)
{
  AM_STATE state = AM_STATE_ERROR;

  do {
    if (AM_UNLIKELY(socketpair(AF_UNIX, SOCK_STREAM, IPPROTO_IP,
                               m_msg_ctrl) < 0)) {
      PERROR("socketpair");
      state = AM_STATE_ERROR;
      break;
    }
    m_event = AMEvent::create();
    if (AM_UNLIKELY(NULL == m_event)) {
      ERROR("Failed to create event!");
      break;
    }
    m_sem = AMEvent::create();
    if (AM_UNLIKELY(NULL == m_sem)) {
      ERROR("Failed to create event!");
      break;
    }
    m_config = new AMRecordEngineConfig();
    if (AM_UNLIKELY(NULL == m_config)) {
      ERROR("Failed to create config module for record engine!");
      state = AM_STATE_NO_MEMORY;
      break;
    }
    m_engine_config = m_config->get_config(config);
    if (AM_UNLIKELY(!m_engine_config)) {
      ERROR("Can not get configuration from file %s, please check!",
            config.c_str());
      break;
    } else {
      uint32_t connection_num = 0;
      for (uint32_t i = 0; i < m_engine_config->filter_num; ++ i) {
        connection_num += m_engine_config->connections[i].input_number;
      }
      state = inherited::init(m_engine_config->filter_num, connection_num);
      if (AM_UNLIKELY(AM_STATE_OK != state)) {
        ERROR("Failed to initialize base engine!");
        break;
      }
      state = load_all_filters();
      if (AM_UNLIKELY(AM_STATE_OK != state)) {
        ERROR("Failed to load filters needed by Record Engine!");
        break;
      } else {
        for (uint32_t i = 0; i < m_engine_config->filter_num; ++ i) {
          AMIPacketFilter *filter = (AMIPacketFilter*)
              m_engine_filter[i].filter_obj->get_interface(IID_AMIPacketFilter);
          if (AM_UNLIKELY(AM_STATE_OK
              != (state = add_filter(filter, m_engine_filter[i].so)))) {
            break;
          }
        }
        if (AM_UNLIKELY(AM_STATE_OK != state)) {
          break;
        }
      }
      state = inherited::set_app_msg_callback(static_app_msg_callback, this);
    }
  } while (0);

  return state;
}

AM_STATE AMRecordEngine::load_all_filters()
{
  AM_STATE state = AM_STATE_OK;
  uint32_t next_audio_stream_id_base = 0;

  delete[] m_engine_filter;
  m_engine_filter = new EngineFilter[m_engine_config->filter_num];
  if (AM_LIKELY(m_engine_filter)) {
    for (uint32_t i = 0; i < m_engine_config->filter_num; ++ i) {
      std::string filter = ORYX_FILTER_DIR;
      std::string filter_conf = ORYX_CONF_DIR;

      m_engine_filter[i].filter = m_engine_config->filters[i];
      filter.append("/filter-").append(m_engine_filter[i].filter).
          append(".so");
      filter_conf.append("/stream/filter/filter-").
          append(m_engine_filter[i].filter).append(".acs");
      m_engine_filter[i].so = AMPlugin::create(filter.c_str());

      if (AM_LIKELY(m_engine_filter[i].so)) {
        ConnectionConfig *con =
                  get_connection_conf_by_name(m_engine_filter[i].filter);
        GetFilterObject create_filter =
            (GetFilterObject)m_engine_filter[i].so->get_symbol(GET_FILTER_OBJ);
        if (AM_LIKELY(create_filter && con)) {
          m_engine_filter[i].filter_obj = create_filter(this,
                                                        filter_conf.c_str(),
                                                        con->input_number,
                                                        con->output_number);
          DEBUG("Filter %s is at %p", m_engine_filter[i].filter.c_str(),
                m_engine_filter[i].filter_obj);
          if (AM_UNLIKELY(!m_engine_filter[i].filter_obj)) {
            ERROR("Failed to create filter: %s",
                  m_engine_filter[i].filter.c_str());
            state = AM_STATE_ERROR;
            break;
          }
          AMIAudioSource *asrc = ((AMIAudioSource*)m_engine_filter[i].\
              filter_obj->get_interface(IID_AMIAudioSource));
          if (AM_LIKELY(asrc)) {
            asrc->set_stream_id(next_audio_stream_id_base);
            next_audio_stream_id_base += asrc->get_audio_number();
          }
        } else {
          ERROR("Invalid filter plugin: %s", filter.c_str());
          state = AM_STATE_ERROR;
          break;
        }
      } else {
        ERROR("Failed to load filter: %s", filter.c_str());
        state = AM_STATE_ERROR;
        break;
      }
    }
  } else {
    state = AM_STATE_NO_MEMORY;
  }

  return state;
}

AMIPacketFilter* AMRecordEngine::get_filter_by_name(std::string& name)
{
  AMIPacketFilter *filter = NULL;
  for (uint32_t i = 0; i < m_engine_config->filter_num; ++ i) {
    if (AM_LIKELY(m_engine_filter[i].filter == name)) {
      filter = (AMIPacketFilter*)m_engine_filter[i].filter_obj->\
          get_interface(IID_AMIPacketFilter);
      break;
    }
  }

  return filter;
}

AMIMuxer* AMRecordEngine::get_muxer_filter_by_name(std::string& name)
{
  AMIMuxer *filter = NULL;
  for (uint32_t i = 0; i < m_engine_config->filter_num; ++ i) {
    if (AM_LIKELY(m_engine_filter[i].filter == name)) {
      filter = (AMIMuxer*)m_engine_filter[i].filter_obj->\
          get_interface(IID_AMIMuxer);
      break;
    }
  }
  return filter;
}

const char* AMRecordEngine::get_filter_name_by_pointer(AMIInterface *filter)
{
  const char *name = NULL;
  for (uint32_t i = 0; i < m_engine_config->filter_num; ++ i) {
    if (AM_LIKELY(m_engine_filter[i].filter_obj->\
                  get_interface(IID_AMIInterface) == filter)) {
      name = m_engine_filter[i].filter.c_str();
      break;
    }
  }

  return name;
}

ConnectionConfig* AMRecordEngine::get_connection_conf_by_name(std::string& name)
{
  ConnectionConfig *connection = NULL;
  for (uint32_t i = 0; i < m_engine_config->filter_num; ++ i) {
    if (AM_LIKELY(m_engine_config->connections[i].filter == name)) {
      connection = &m_engine_config->connections[i];
      break;
    }
  }

  return connection;
}

void* AMRecordEngine::get_filter_by_iid(AM_REFIID iid)
{
  void *filter = NULL;
  for (uint32_t i = 0; i < m_engine_config->filter_num; ++ i) {
    filter = m_engine_filter[i].filter_obj->get_interface(iid);
    if (AM_LIKELY(filter)) {
      break;
    }
  }

  return filter;
}

AMFilterVector AMRecordEngine::get_filter_vector_by_iid(AM_REFIID iid)
{
  AMFilterVector filter_vector;
  for (uint32_t i = 0; i < m_engine_config->filter_num; ++ i) {
    void *filter = m_engine_filter[i].filter_obj->get_interface(iid);
    if (AM_LIKELY(filter)) {
      filter_vector.push_back(filter);
    }
  }

  return filter_vector;
}

void AMRecordEngine::static_mainloop(void *data)
{
  ((AMRecordEngine*)data)->mainloop();
}

void AMRecordEngine::mainloop()
{
  fd_set fdset;
  sigset_t mask;
  sigset_t mask_orig;

  int maxfd = MSG_R;

  /* Block out interrupts */
  sigemptyset(&mask);
  sigaddset(&mask, SIGINT);
  sigaddset(&mask, SIGTERM);
  sigaddset(&mask, SIGQUIT);

  if (AM_UNLIKELY(sigprocmask(SIG_BLOCK, &mask, &mask_orig) < 0)) {
    PERROR("sigprocmask");
  }
  m_mainloop_run = true;
  m_sem->signal();

  while(m_mainloop_run.load()) {
    char cmd[1] = {0};
    FD_ZERO(&fdset);
    FD_SET(MSG_R, &fdset);

    if (AM_LIKELY(pselect(maxfd + 1, &fdset,
                          nullptr, nullptr,
                          nullptr, &mask) > 0)) {
      if (AM_LIKELY(FD_ISSET(MSG_R, &fdset))) {
        if (AM_UNLIKELY(read(MSG_R, cmd, 1) < 0)) {
          ERROR("Failed to read command! ABORT!");
          cmd[0] = AM_ENGINE_CMD_ABORT;
        }
      }
    } else {
      if (AM_LIKELY(errno != EINTR)) {
        PERROR("select");
        cmd[0] = AM_ENGINE_CMD_ABORT;
      }
    }
    switch(cmd[0]) {
      case AM_ENGINE_CMD_ABORT : {
        DEBUG("Received ABORT!");
        change_engine_status(AM_RECORD_ENGINE_ABORT);
        m_mainloop_run = false;
      }break;
      case AM_ENGINE_CMD_START : {
        DEBUG("Received START!");
        change_engine_status(AM_RECORD_ENGINE_RECORDING);
      }break;
      case AM_ENGINE_CMD_STOP  : {
        DEBUG("Received STOP!");
        change_engine_status(AM_RECORD_ENGINE_STOPPED);
      }break;
      case AM_ENGINE_CMD_EXIT: {
        DEBUG("Received EXIT!");
        m_mainloop_run = false;
      }break;
    }
    m_event->signal();
  }
  INFO("Record Engine Mainloop exits!");
}

bool AMRecordEngine::send_engine_cmd(AM_RECORD_ENGINE_CMD cmd, bool block)
{
  AUTO_MEM_LOCK(m_lock);
  bool ret = true;
  char command = cmd;
  if (AM_UNLIKELY(write(MSG_W, &command, sizeof(command)) != sizeof(command))) {
    ERROR("Failed to send command '%c'", command);
    ret = false;
  } else if (block) {
    m_event->wait();
    switch(cmd) {
      case AM_ENGINE_CMD_START :
        ret = (m_status == AM_RECORD_ENGINE_RECORDING); break;
      case AM_ENGINE_CMD_STOP  :
        ret = (m_status == AM_RECORD_ENGINE_STOPPED);   break;
      case AM_ENGINE_CMD_EXIT  :
      case AM_ENGINE_CMD_ABORT :
        ret = true;
        break;
    }
  }

  return ret;
}

