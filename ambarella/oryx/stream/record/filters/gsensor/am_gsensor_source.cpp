/*******************************************************************************
 * am_gsensor_source.cpp
 *
 * History:
 *   2016-11-28 - [ccjing] created file
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

#include "am_mutex.h"
#include "am_event.h"
#include "am_gsensor_source.h"
#include "am_gsensor_reader.h"
#include "am_gsensor_source_config.h"
#include "am_gsensor_source_version.h"

#define CTRL_READ_GSENSOR    m_ctrl_gsensor[0]
#define CTRL_WRITE_GSENSOR   m_ctrl_gsensor[1]
#define PACKET_BUF_SIZE      1024

#define HW_TIMER  ((const char*)"/proc/ambarella/ambarella_hwtimer")
#define GSENSOR_DEV   ((const char*)"/gsensor.tmp")

AMIInterface* create_filter(AMIEngine *engine,
                            const char *config,
                            uint32_t input_num,
                            uint32_t output_num)
{
  return (AMIInterface*)AMGsensorSource::create(engine, config,
                                            input_num, output_num);
}

AMIGsensorSource* AMGsensorSource::create(AMIEngine *engine,
                                  const std::string& config,
                                  uint32_t input_num,
                                  uint32_t output_num)
{
  AMGsensorSource *result = new AMGsensorSource(engine);
  if (AM_UNLIKELY(result && (AM_STATE_OK != result->init(config,
                                                         input_num,
                                                         output_num)))) {
    delete result;
    result = NULL;
  }
  return result;
}

void* AMGsensorSource::get_interface(AM_REFIID ref_iid)
{
  return (ref_iid == IID_AMIGsensorSource) ? (AMIGsensorSource*)this :
      inherited::get_interface(ref_iid);
}

void AMGsensorSource::destroy()
{
  inherited::destroy();
}

void AMGsensorSource::get_info(INFO& info)
{
  info.num_in = m_input_num;
  info.num_out = m_output_num;
  info.name = m_name;
}

AMIPacketPin* AMGsensorSource::get_input_pin(uint32_t index)
{
  ERROR("%s doesn't have input pin!", m_name);
  return NULL;
}

AMIPacketPin* AMGsensorSource::get_output_pin(uint32_t index)
{
  AMIPacketPin *pin = (index < m_output_num) ? m_outputs[index] : nullptr;
  if (AM_UNLIKELY(!pin)) {
    ERROR("No such output pin [index:%u]", index);
  }

  return pin;
}

AM_STATE AMGsensorSource::start()
{
  AUTO_MEM_LOCK(m_lock);
  m_state_lock.lock();
  if (AM_LIKELY((m_filter_state == AM_GSENSOR_SRC_STOPPED) ||
                (m_filter_state == AM_GSENSOR_SRC_INITTED) ||
                (m_filter_state == AM_GSENSOR_SRC_WAITING))) {
    m_packet_pool->enable(true);
    m_event->signal();
  }
  m_state_lock.unlock();
  return AM_STATE_OK;
}

AM_STATE AMGsensorSource::stop()
{
  AUTO_MEM_LOCK(m_lock);
  if (m_run.load() && (CTRL_WRITE_GSENSOR > 0)) {
    write(CTRL_WRITE_GSENSOR, "s", 1);
  }
  m_state_lock.lock();
  if (AM_UNLIKELY(m_filter_state == AM_GSENSOR_SRC_WAITING)) {
    /* on_run is blocked on m_event->wait(), need unblock it */
    m_event->signal();
  }
  m_state_lock.unlock();
  m_packet_pool->enable(false);
  return inherited::stop();
}

uint32_t AMGsensorSource::version()
{
  return GSENSOR_SOURCE_VERSION_NUMBER;
}

void AMGsensorSource::on_run()
{
  fd_set real_fd;
  fd_set reset_fd;
  int32_t max_fd = -1;
  int32_t gsensor_reader_fd = -1;
  AmMsg  engine_msg(AMIEngine::ENG_MSG_OK);
  engine_msg.p0 = (int_ptr)(get_interface(IID_AMIInterface));

  ack(AM_STATE_OK);
  post_engine_msg(engine_msg);
  m_state_lock.lock();
  m_filter_state = AM_GSENSOR_SRC_WAITING;
  m_state_lock.unlock();
  m_event->wait();
  m_state_lock.lock();
  m_filter_state = AM_GSENSOR_SRC_RUNNING;
  m_state_lock.unlock();
  if (!create_resource()) {
    ERROR("Failed to create resource for gsensor filter.");
    m_run = false;
  } else {
    gsensor_reader_fd = m_gsensor_reader->get_gsensor_fd();
    m_run = true;
    FD_ZERO(&reset_fd);
    FD_SET(gsensor_reader_fd, &reset_fd);
    FD_SET(CTRL_READ_GSENSOR, &reset_fd);
    max_fd = (gsensor_reader_fd > CTRL_READ_GSENSOR) ?
        gsensor_reader_fd : CTRL_READ_GSENSOR;
  }
  while(m_run.load()) {
    real_fd = reset_fd;
    int sret = select(max_fd + 1, &real_fd, NULL, NULL, NULL);
    if (sret <= 0) {
      PERROR("Select");
      continue;
    }
    if (FD_ISSET(CTRL_READ_GSENSOR, &real_fd)) {
      char cmd[1] = { 0 };
      if (AM_LIKELY(read(CTRL_READ_GSENSOR, cmd, sizeof(cmd)) < 0)) {
        PERROR("Read form CTRL_READ_GSENSOR");
        continue;
      } else if (cmd[0] == 's') {
        INFO("Receive stop cmd in gsensor filter on_run.");
        m_run = false;
        continue;
      } else {
        ERROR("Received invalid cmd in %s", m_name);
        continue;
      }
    }
    if (FD_ISSET(gsensor_reader_fd, &real_fd)) {
      AMPacket *packet = nullptr;
      if (!m_info_send) {
        AMPacket *info_pkt = nullptr;
        if (AM_UNLIKELY(!m_packet_pool->alloc_packet(info_pkt, 0))) {
          ERROR("Failed to malloc info pkt in %s", m_name);
          break;
        }
        uint32_t info_size = sizeof(AM_GSENSOR_INFO);
        m_info.stream_id = 0;
        m_info.type = AM_SEI_GSENSOR;
        m_info.sample_size = 24;
        m_info.sample_rate = 30;
        m_info.pkt_pts_increment = 90000 / m_info.sample_rate;
        memcpy(info_pkt->get_data_ptr(), &m_info, info_size);
        info_pkt->set_attr(AMPacket::AM_PAYLOAD_ATTR_GSENSOR);
        info_pkt->set_data_size(info_size);
        info_pkt->set_pts(get_current_pts());
        info_pkt->set_type(AMPacket::AM_PAYLOAD_TYPE_INFO);
        send_packet(info_pkt);
        m_info_send = true;
      }
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
        uint32_t data_size = 24;
        bool result = m_gsensor_reader->query_gsensor_frame(
            packet->get_data_ptr(), data_size);
        if ((!result) || (data_size >= PACKET_BUF_SIZE)) {
          ERROR("Failed to query gsensor frame, data size is %u", data_size);
          packet->release();
          continue;
        } else {
          packet->set_attr(AMPacket::AM_PAYLOAD_ATTR_GSENSOR);
          packet->set_data_size(data_size);
          packet->set_pts(get_current_pts());
          packet->set_type(AMPacket::AM_PAYLOAD_TYPE_DATA);
          send_packet(packet);
        }
        usleep(30000);//for debug, sleep 30 ms
      }
    }
  }

  m_state_lock.lock();
  m_filter_state = AM_GSENSOR_SRC_STOPPED;
  m_state_lock.unlock();
  m_event->clear();
  release_resource();
  if (AM_LIKELY(!m_run.load())) {
    NOTICE("%s exits mainloop!", m_name);
  } else {
    NOTICE("%s aborted!", m_name);
    engine_msg.code = AMIEngine::ENG_MSG_ABORT;
    post_engine_msg(engine_msg);
  }
}

int64_t AMGsensorSource::get_current_pts()
{
  uint8_t pts[32] = {0};
  int64_t current_pts = m_last_pts;
  if (m_hw_timer_fd  < 0) {
    return current_pts;
  }
  if (read(m_hw_timer_fd, pts, sizeof(pts)) < 0) {
    PERROR("Read");
  } else {
    current_pts = strtoull((const char*)pts, (char**)NULL, 10);
    m_last_pts = current_pts;
  }
  return current_pts;
}

AMGsensorSource::AMGsensorSource(AMIEngine *engine) :
    inherited(engine)
{
  m_outputs.clear();
}

AMGsensorSource::~AMGsensorSource()
{
  AM_DESTROY(m_packet_pool);
  AM_DESTROY(m_event);
  for (uint32_t i = 0; i < m_outputs.size(); ++ i) {
    AM_DESTROY(m_outputs[i]);
  }
  m_outputs.clear();
  delete m_config;
  if (AM_LIKELY(CTRL_READ_GSENSOR >= 0)) {
    if (close(CTRL_READ_GSENSOR) < 0) {
      PERROR("Failed to close CTRL_READ_GSENSOR:");
    }
    CTRL_READ_GSENSOR = -1;
  }
  if (AM_LIKELY(CTRL_WRITE_GSENSOR >= 0)) {
    if (close(CTRL_WRITE_GSENSOR) < 0) {
      PERROR("Failed to close CTRL_WRITE_GSENSOR:");
    }
    CTRL_WRITE_GSENSOR = -1;
  }
}

AM_STATE AMGsensorSource::init(const std::string& config,
                             uint32_t input_num,
                             uint32_t output_num)
{
  AM_STATE state = AM_STATE_OK;
  m_input_num = input_num;
  m_output_num = output_num;
  do {
    m_config = new AMGsensorSourceConfig();
    if (AM_UNLIKELY(!m_config)) {
      ERROR("Failed to create config module for gsensor filter!");
      state = AM_STATE_NO_MEMORY;
      break;
    }
    m_config_struct = m_config->get_config(config);
    if (AM_UNLIKELY(!m_config_struct)) {
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
    if (AM_UNLIKELY(pipe(m_ctrl_gsensor) < 0)) {
      PERROR("Failed to create pipe:");
      state = AM_STATE_ERROR;
      break;
    }
    state = inherited::init((const char*)m_config_struct->name.c_str(),
                            m_config_struct->real_time.enabled,
                            m_config_struct->real_time.priority);
    if (AM_LIKELY(AM_STATE_OK != state)) {
      break;
    } else {
      std::string poolName = m_config_struct->name + ".packet.pool";
      m_packet_pool = AMFixedPacketPool::create(poolName.c_str(),
                                          m_config_struct->packet_pool_size,
                                          PACKET_BUF_SIZE);
      if (AM_UNLIKELY(!m_packet_pool)) {
        ERROR("Failed to create packet pool for %s",
              m_config_struct->name.c_str());
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
        AMGsensorSourceOutput *output = AMGsensorSourceOutput::create(this);
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
    m_state_lock.lock();
    m_filter_state = AM_GSENSOR_SRC_INITTED;
    m_state_lock.unlock();
  }while(0);
  return state;
}

void AMGsensorSource::send_packet(AMPacket *packet)
{
  if (AM_LIKELY(packet)) {
    for (uint32_t i = 0; i < m_output_num; ++ i) {
      packet->add_ref();
      m_outputs[i]->send_packet(packet);
    }
    packet->release();
  }
}

bool AMGsensorSource::create_resource()
{
  bool ret = true;
  do {
    AM_DESTROY(m_gsensor_reader);
    m_gsensor_reader = AMGsensorReader::create(std::string(GSENSOR_DEV));
    if (!m_gsensor_reader) {
      ERROR("Failed to create gsensor reader.");
      ret = false;
      break;
    }
    m_hw_timer_fd = open(HW_TIMER, O_RDONLY);
    if (m_hw_timer_fd < 0) {
      PERROR("Failed to open hardware timer:");
      ret = false;
      break;
    }
    m_info_send = false;
  } while(0);
  return ret;
}

void AMGsensorSource::release_resource()
{
  AM_DESTROY(m_gsensor_reader);
  if (m_hw_timer_fd >= 0) {
    if (close(m_hw_timer_fd) < 0) {
      PERROR("Failed to close hardware timer:");
    }
    m_hw_timer_fd = -1;
  }
}
