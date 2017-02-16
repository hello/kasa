/*******************************************************************************
 * am_rtp_data.h
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
#ifndef ORYX_STREAM_RECORD_MUXERS_MUXER_RTP_AM_RTP_DATA_H_
#define ORYX_STREAM_RECORD_MUXERS_MUXER_RTP_AM_RTP_DATA_H_

#include <atomic>
#include <mutex>

struct AMRtpPacket
{
    uint8_t *tcp_data;
    uint8_t *udp_data;
    uint32_t total_size;
    std::mutex mutex;
    AMRtpPacket();
    void lock();
    void unlock();
    uint8_t* tcp();
    uint32_t tcp_data_size();
    uint8_t* udp();
    uint32_t udp_data_size();
};

class AMRtpSession;
struct AMRtpData
{
    int64_t         pts;
    uint8_t        *buffer;
    AMRtpPacket    *packet;
    AMRtpSession   *owner;
    uint32_t        id;
    uint32_t        buffer_size;
    uint32_t        data_size;
    uint32_t        payload_size;
    uint32_t        pkt_size;
    uint32_t        pkt_num;
    std::atomic_int ref_count;
    AMRtpData();
    ~AMRtpData();
    void clear();
    bool create(AMRtpSession *session, uint32_t datasize,
                uint32_t payloadsize,  uint32_t packet_num);
    void add_ref();
    void release();
};

#endif /* ORYX_STREAM_RECORD_MUXERS_MUXER_RTP_AM_RTP_DATA_H_ */
