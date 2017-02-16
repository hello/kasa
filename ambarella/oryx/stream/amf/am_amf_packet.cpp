/*******************************************************************************
 * am_amf_packet.cpp
 *
 * History:
 *   2014-7-23 - [ypchang] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co, Ltd.
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

/*
 * AMPacket
 */
AMPacket::AMPacket() :
  m_ref_count(0),
  m_packet_type(AM_PACKET_TYPE_NORMAL),
  m_pool(NULL),
  m_next(NULL),
  m_payload(NULL)
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
  m_payload->m_header.m_payload_type =
      static_cast<AMPacket::AM_PAYLOAD_TYPE>(type);
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

uint16_t AMPacket::get_stream_id()
{
  return m_payload->m_data.m_stream_id;
}

void AMPacket::set_stream_id(uint16_t id)
{
  m_payload->m_data.m_stream_id = id;
}

uint16_t AMPacket::get_frame_type()
{
  return m_payload->m_data.m_frame_type;
}

void AMPacket::set_frame_type(uint16_t type)
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

uint32_t AMPacket::get_frame_count()
{
  return m_payload->m_data.m_frame_count;
}

void AMPacket::set_frame_count(uint32_t count)
{
  m_payload->m_data.m_frame_count = count;
}
