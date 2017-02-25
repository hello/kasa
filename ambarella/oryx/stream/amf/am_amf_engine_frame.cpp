/*******************************************************************************
 * am_amf_engine_frame.cpp
 *
 * History:
 *   2014-7-24 - [ypchang] created file
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
#include "am_plugin.h"

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

void AMEngineFrame::clear_graph()
{
  if (AM_LIKELY(m_filter_num > 0)) {
    INFO("^^^^^^ Clearing Engine ^^^^^^");
    stop_all_filters();
    purge_all_filters();
    delete_all_connections();
    remove_all_filters();
    INFO("^^^^^^ Engine Cleared ^^^^^^");
  }
}

AM_STATE AMEngineFrame::run_all_filters()
{
  AM_STATE state = AM_STATE_OK;

  if (AM_LIKELY(!m_is_filter_running)) {
    INFO("~~~~~~ Start Running All Filters ~~~~~~");
    for (uint32_t i = 0; i < m_filter_num; ++ i) {
      AMIPacketFilter *filter = m_filters[i].filter;
      INFO("~~~~~~ Starting Filter %s (%u)...", get_filter_name(filter), i + 1);
      enable_output_pins(filter);
      if (AM_UNLIKELY(AM_STATE_OK != (state = filter->run()))) {
        ERROR("~~~~~~ Filter %s (%u) failed to run, returned %d",
              get_filter_name(filter), i + 1, state);
        /* Stop all the already started filters */
        for (uint32_t j = i - 1; j >= 0; -- j) {
          NOTICE("~~~~~~ Stopping %s (%u)",
                 get_filter_name(m_filters[j].filter), j + 1);
          disable_output_pins(m_filters[j].filter);
          m_filters[j].filter->stop();
        }
        break;
      }
    }
    if (AM_LIKELY(state == AM_STATE_OK)) {
      m_is_filter_running = true;
      INFO("~~~~~~ All Filters Are Started ~~~~~~");
    }
  }

  return state;
}

void AMEngineFrame::stop_all_filters()
{
  if (AM_LIKELY(m_is_filter_running)) {
    AM_STATE state = AM_STATE_OK;
    INFO("====== Stopping All Filters ======");
    for (uint32_t i = m_filter_num; i > 0; -- i) {
      AMIPacketFilter *filter = m_filters[i - 1].filter;
      INFO("====== Stopping Filter %s (%u)...", get_filter_name(filter), i);
      disable_output_pins(filter);
      state = filter->stop();
      if (AM_UNLIKELY(AM_STATE_OK != state)) {
        ERROR("====== Filter %s (%u) returned %d",
              get_filter_name(filter), i + 1, state);
      }
    }
    m_is_filter_running = false;
    INFO("====== All Filters Are Stopped ======");
  }
}

void AMEngineFrame::purge_all_filters()
{
  INFO("<<<<<< Purging All Filters <<<<<<");
  for (uint32_t i = 0; i < m_filter_num; ++ i) {
    INFO("<<<<<< Purging Filter %s (%u)...",
         get_filter_name(m_filters[i].filter), i + 1);
    m_filters[i].filter->purge();
  }
  INFO("<<<<<< All Filters Are Purged <<<<<<");
}

void AMEngineFrame::delete_all_connections()
{
  INFO("`````` Deleting All Connections ``````");
  for (uint32_t i = m_connection_num; i > 0; -- i) {
    m_connections[i - 1].pin_out->disconnect();
  }
  m_connection_num = 0;
  INFO("`````` All Connections Are Deleted ``````");
}

void AMEngineFrame::remove_all_filters()
{
  INFO("###### Removing All Filters ######");
  for (uint32_t i = m_filter_num; i > 0; -- i) {
    INFO("###### Removing Filter %s (%u)...",
         get_filter_name(m_filters[i - 1].filter), i);
    AM_DESTROY(m_filters[i - 1].filter);
    AM_DESTROY(m_filters[i - 1].so);
  }
  m_filter_num = 0;
  INFO("###### All Filters Are Removed ######");
}

void AMEngineFrame::enable_output_pins(AMIPacketFilter *filter)
{
  set_output_pins_enabled(filter, true);
}

void AMEngineFrame::disable_output_pins(AMIPacketFilter *filter)
{
  set_output_pins_enabled(filter, false);
}

AM_STATE AMEngineFrame::add_filter(AMIPacketFilter *filter, AMPlugin *so)
{
  AM_STATE state = AM_STATE_OK;
  if (AM_LIKELY(m_filter_num < m_filter_num_max)) {
    filter->add_ref();
    so->add_ref();
    m_filters[m_filter_num].so     = so;
    m_filters[m_filter_num].filter = filter;
    m_filters[m_filter_num].flags = 0;
    ++ m_filter_num;
    INFO("@@@ Filter(%u) %s is added...",
         m_filter_num, get_filter_name(filter));
  } else {
    ERROR("!!! Too many filters!");
    state = AM_STATE_TOO_MANY;
  }
  return state;
}

AM_STATE AMEngineFrame::connect(AMIPacketFilter *upStreamFilter,
                                uint32_t indexUp,
                                AMIPacketFilter *downStreamFilter,
                                uint32_t indexDown)
{
  AM_STATE state = AM_STATE_OK;
  AMIPacketPin *out = upStreamFilter->get_output_pin(indexUp);
  AMIPacketPin *in  = downStreamFilter->get_input_pin(indexDown);
  do {
    if (AM_UNLIKELY(!out)) {
      ERROR("No such pin: %s[%u]...",
            get_filter_name(upStreamFilter), indexUp);
      state = AM_STATE_ERROR;
      break;
    }
    if (AM_UNLIKELY(!in)) {
      ERROR("No such pin: %s[%u]...",
            get_filter_name(downStreamFilter), indexDown);
      state = AM_STATE_ERROR;
      break;
    }
    state = create_connection(out, in);

  }while(0);

  return state;
}

AM_STATE AMEngineFrame::create_connection(AMIPacketPin *out, AMIPacketPin *in)
{
  AM_STATE state = AM_STATE_ERROR;
  if (AM_UNLIKELY(m_connection_num >= m_connection_num_max)) {
    ERROR("Too many connections: %u, %u!",
          m_connection_num, m_connection_num_max);
    state = AM_STATE_TOO_MANY;
  } else {
    state = out->connect(in);
    if (AM_LIKELY(AM_STATE_OK == state)) {
      m_connections[m_connection_num].pin_out = out;
      m_connections[m_connection_num].pin_in  = in;
      ++ m_connection_num;
    } else {
      ERROR("Failed to connect output pin %p to input pin %p!", out, in);
    }
  }

  return state;
}

const char* AMEngineFrame::get_filter_name(AMIPacketFilter *filter)
{
  AMIPacketFilter::INFO info;
  filter->get_info(info);
  return info.name;
}

AMEngineFrame::AMEngineFrame() :
    m_filters(nullptr),
    m_connections(nullptr),
    m_filter_num(0),
    m_filter_num_max(0),
    m_connection_num(0),
    m_connection_num_max(0),
    m_is_filter_running(false)
{}

AMEngineFrame::~AMEngineFrame()
{
  clear_graph();
  delete[] m_filters;
  delete[] m_connections;
}

AM_STATE AMEngineFrame::init(uint32_t filter_num, uint32_t connection_num)
{
  AM_STATE state = AM_STATE_ERROR;;
  do {
    if (AM_UNLIKELY(AM_STATE_OK != inherited::init())) {
      break;
    }
    if (AM_UNLIKELY(!filter_num || !connection_num)) {
      if (AM_LIKELY(!filter_num)) {
        ERROR("Max filter number is 0!");
      }
      if (AM_LIKELY(!connection_num)) {
        ERROR("Max connection number is 0!");
      }
      ERROR("Please check engine configuration file!");
      break;
    }
    m_filter_num_max = filter_num;
    m_connection_num_max = connection_num;
    m_filters = new Filter[m_filter_num_max];
    if (AM_UNLIKELY(NULL == m_filters)) {
      ERROR("Failed to allocate memory for filters!");
      state = AM_STATE_NO_MEMORY;
      break;
    }
    m_connections = new Connection[m_connection_num_max];
    if (AM_UNLIKELY(NULL == m_connections)) {
      ERROR("Failed to allocate memory for connections!");
      state = AM_STATE_NO_MEMORY;
      break;
    }
    state = AM_STATE_OK;
  } while(0);

  return state;
}

void AMEngineFrame::set_output_pins_enabled(AMIPacketFilter *filter,
                                            bool enabled)
{
  AMIPacketFilter::INFO info;
  filter->get_info(info);
  for (uint32_t i = 0; i < info.num_out; ++ i) {
    AMIPacketPin *pin = filter->get_output_pin(i);
    if (AM_LIKELY(pin)) {
      pin->enable(enabled);
    }
  }
}
