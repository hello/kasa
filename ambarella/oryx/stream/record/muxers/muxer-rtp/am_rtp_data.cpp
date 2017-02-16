/*******************************************************************************
 * am_rtp_data.cpp
 *
 * History:
 *   2015-1-6 - [ypchang] created file
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

#include "am_rtp_data.h"
#include "am_rtp_session_base.h"

AMRtpPacket::AMRtpPacket() :
  tcp_data(nullptr),
  udp_data(nullptr),
  total_size(0)
{}

void AMRtpPacket::lock()
{
  mutex.lock();
}

void AMRtpPacket::unlock()
{
  mutex.unlock();
}

uint8_t* AMRtpPacket::tcp()
{
  return tcp_data;
}

uint32_t AMRtpPacket::tcp_data_size()
{
  return total_size;
}

uint8_t* AMRtpPacket::udp()
{
  return udp_data;
}

uint32_t AMRtpPacket::udp_data_size()
{
  return (total_size - sizeof(AMRtpTcpHeader));
}

AMRtpData::AMRtpData() :
  pts(0LL),
  buffer(nullptr),
  packet(nullptr),
  owner(nullptr),
  id(0),
  buffer_size(0),
  data_size(0),
  payload_size(0),
  pkt_size(0),
  pkt_num(0),
  ref_count(0)
{}
AMRtpData::~AMRtpData()
{
  clear();
}
void AMRtpData::clear()
{
  delete[] buffer;
  delete[] packet;
  buffer = nullptr;
  packet = nullptr;
  pts = 0LL;
  pkt_size = 0;
  pkt_num = 0;
  buffer_size = 0;
  data_size = 0;
  ref_count = 0;
  owner = nullptr;
}
bool AMRtpData::create(AMRtpSession *session,
                       uint32_t datasize,
                       uint32_t payloadsize,
                       uint32_t packet_num)
{
  owner = session;
  payload_size = payloadsize;
  if (AM_LIKELY(datasize > 0)) {
    if (AM_UNLIKELY(datasize > buffer_size)) {
      delete[] buffer;
      buffer = nullptr;
      INFO("ID: %02u, Prev buffer size: %u, current buffer size: %u",
            id, buffer_size, datasize);
      buffer_size = ROUND_UP((5 * datasize / 4), 4);
    }
    data_size = datasize;
    if (AM_LIKELY(!buffer)) {
      buffer = new uint8_t[buffer_size];
    }
  }
  if (AM_LIKELY(packet_num > 0)) {
    if (AM_UNLIKELY(packet_num > pkt_size)) {
      delete[] packet;
      packet = nullptr;
      INFO("ID: %02u, Prev packet num: %u, current packet num: %u",
            id, pkt_size, packet_num);
      pkt_size = 5 * packet_num / 4;
    }
    pkt_num = packet_num;
    if (AM_LIKELY(!packet)) {
      packet = new AMRtpPacket[pkt_size];
    }
  }
  return (buffer && packet);
}
void AMRtpData::add_ref()
{
  ++ ref_count;
}
void AMRtpData::release()
{
  if (--ref_count <= 0) {
    owner->put_back(this);
    ref_count = 0;
  }
}
