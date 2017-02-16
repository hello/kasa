/*******************************************************************************
 * am_rtp_session_h265.h
 *
 * History:
 *   2015-3-18 - [ypchang] created file
 *
 * Copyright (C) 2008-2015, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/
#ifndef ORYX_STREAM_RECORD_MUXERS_MUXER_RTP_AM_RTP_SESSION_H265_H_
#define ORYX_STREAM_RECORD_MUXERS_MUXER_RTP_AM_RTP_SESSION_H265_H_

#include "am_rtp_session_base.h"

#include <vector>

struct AM_H265_NALU;
struct AMRtpData;
struct AM_VIDEO_INFO;
class AMSpinLock;
class AMRtpSessionH265: public AMRtpSession
{
    typedef std::vector<AM_H265_NALU> AMNaluList;
    typedef AMRtpSession inherited;
  public:
    static AMIRtpSession* create(const std::string &name,
                                 uint32_t queue_size,
                                 AM_VIDEO_INFO *vinfo);

  public:
    virtual uint32_t get_session_clock();
    virtual AM_SESSION_STATE process_data(AMPacket &packet,
                                          uint32_t max_tcp_payload_size);

  private:
    AMRtpSessionH265(const std::string &name, uint32_t queue_size);
    virtual ~AMRtpSessionH265();
    bool init(AM_VIDEO_INFO *vinfo);
    virtual void generate_sdp(std::string& host_ip_addr, uint16_t port,
                              AM_IP_DOMAIN domain);
    virtual std::string get_param_sdp();
    virtual std::string get_payload_type();

  private:
    std::string& sps();
    std::string& pps();
    std::string& vps();
    void parse_vps(uint8_t *vps, uint32_t size);
    bool find_nalu(uint8_t *data, uint32_t len, int32_t expect);
    uint32_t calc_h265_packet_num(AM_H265_NALU &nalu, uint32_t max_packet_size);
    void assemble_packet(AMRtpData *rtp_data,
                         int32_t pts_incr,
                         uint32_t max_tcp_payload_size,
                         uint32_t max_packet_size);

  private:
    AMSpinLock    *m_lock;
    AM_VIDEO_INFO *m_video_info;
    AMNaluList     m_nalu_list;
    std::string    m_sps;
    std::string    m_pps;
    std::string    m_vps;
    std::string    m_profile_compatibility_indicator;
    std::string    m_interop_constraints;
    /* Store NALU Header + 4 bytes + profile_tier_level */
    uint8_t        m_profile_tier_level[H265_PROFILE_TIER_LEVEL_SIZE + 6];
    uint8_t        m_profile_space;
    uint8_t        m_profile_id;
    uint8_t        m_tier_flag;
    uint8_t        m_level_id;
};

#endif /* ORYX_STREAM_RECORD_MUXERS_MUXER_RTP_AM_RTP_SESSION_H265_H_ */
