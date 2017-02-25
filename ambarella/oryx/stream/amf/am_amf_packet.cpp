/*******************************************************************************
 * am_amf_packet.cpp
 *
 * History:
 *   2014-7-23 - [ypchang] created file
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

/*
 * AMPacket
 */
AMPacket::AMPacket() :
  m_pool(nullptr),
  m_next(nullptr),
  m_payload(nullptr),
  m_packet_type(AM_PACKET_TYPE_NORMAL),
  m_ref_count(0)
{}

void AMPacket::add_ref()
{
  m_pool->add_ref(this);
}

void AMPacket::release()
{
  m_pool->release(this);
}

AMPacket::Payload* AMPacket::get_payload( AMPacket::AM_PAYLOAD_TYPE type ,
                                          AMPacket::AM_PAYLOAD_ATTR attr)
{
  if (AM_LIKELY(m_payload)) {
    m_payload->m_header.m_payload_type = type;
    m_payload->m_header.m_payload_attr = attr;
  }

  return m_payload;
}

AMPacket::Payload* AMPacket::get_payload()
{
  return m_payload;
}

void AMPacket::set_payload(void *payload)
{
  m_payload = (AMPacket::Payload*)payload;
}

void AMPacket::set_data_ptr(uint8_t *ptr)
{
  m_payload->m_data.m_buffer = ptr;
}

uint8_t* AMPacket::get_data_ptr()
{
  return m_payload->m_data.m_buffer;
}

uint32_t AMPacket::get_data_size()
{
  return m_payload->m_data.m_size;
}

void AMPacket::set_data_size(uint32_t size)
{
  m_payload->m_data.m_size = size;
}

uint32_t AMPacket::get_data_offset()
{
  return m_payload->m_data.m_offset;
}

void AMPacket::set_data_offset(uint32_t offset)
{
  m_payload->m_data.m_offset = (offset>=m_payload->m_data.m_size) ? 0 : offset;
}

uint32_t AMPacket::get_addr_offset()
{
  return m_payload->m_data.m_addr_offset;
}

void AMPacket::set_addr_offset(uint32_t offset)
{
  m_payload->m_data.m_addr_offset = offset;
}

uint32_t AMPacket::get_packet_type()
{
  return m_packet_type;
}

void AMPacket::set_packet_type(uint32_t type)
{
  m_packet_type = type;
}

AMPacket::AM_PAYLOAD_TYPE AMPacket::get_type()
{
  return AMPacket::AM_PAYLOAD_TYPE(m_payload->m_header.m_payload_type);
}

void AMPacket::set_type(AMPacket::AM_PAYLOAD_TYPE type)
{
  m_payload->m_header.m_payload_type = type;
}

AMPacket::AM_PAYLOAD_ATTR AMPacket::get_attr()
{
  return AMPacket::AM_PAYLOAD_ATTR(m_payload->m_header.m_payload_attr);
}

void AMPacket::set_attr(AMPacket::AM_PAYLOAD_ATTR attr)
{
  m_payload->m_header.m_payload_attr =
      static_cast<AMPacket::AM_PAYLOAD_ATTR>(attr);;
}

int64_t AMPacket::get_pts()
{
  return m_payload->m_data.m_payload_pts;
}

void AMPacket::set_pts(int64_t pts)
{
  m_payload->m_data.m_payload_pts = pts;
}

uint8_t AMPacket::get_event_id()
{
  return m_payload->m_data.m_event_id;
}

void AMPacket::set_event_id(uint8_t id)
{
  m_payload->m_data.m_event_id = id;
}

uint8_t AMPacket::get_stream_id()
{
  return m_payload->m_data.m_stream_id;
}

void AMPacket::set_stream_id(uint8_t id)
{
  m_payload->m_data.m_stream_id = id;
}

uint8_t AMPacket::get_video_type()
{
  return m_payload->m_data.m_video_type;
}

void AMPacket::set_video_type(uint8_t type)
{
  m_payload->m_data.m_video_type = type;
}

uint8_t AMPacket::get_frame_type()
{
  return m_payload->m_data.m_frame_type;
}

void AMPacket::set_frame_type(uint8_t type)
{
  m_payload->m_data.m_frame_type = type;
}

uint32_t AMPacket::get_frame_attr()
{
  return m_payload->m_data.m_frame_attr;
}

void AMPacket::set_frame_attr(uint32_t attr)
{
  m_payload->m_data.m_frame_attr = attr;
}

uint32_t AMPacket::get_frame_number()
{
  return m_payload->m_data.m_frame_num;
}

void AMPacket::set_frame_number(uint32_t num)
{
  m_payload->m_data.m_frame_num = num;
}

uint32_t AMPacket::get_frame_count()
{
  return m_payload->m_data.m_frame_count;
}

void AMPacket::set_frame_count(uint32_t count)
{
  m_payload->m_data.m_frame_count = count;
}
