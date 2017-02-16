/*******************************************************************************
 * am_rtp_client_config.h
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
#ifndef ORYX_STREAM_RECORD_MUXERS_MUXER_RTP_AM_RTP_CLIENT_CONFIG_H_
#define ORYX_STREAM_RECORD_MUXERS_MUXER_RTP_AM_RTP_CLIENT_CONFIG_H_

struct RtpClientConfig
{
    uint32_t client_timeout;
    uint32_t client_timeout_count;
    uint32_t wait_interval;
    uint32_t rtcp_sr_interval;
    uint32_t resend_count;
    uint32_t resend_interval;
    uint32_t statistics_count;
    uint32_t send_buffer_size; /* bytes */
    uint32_t block_timeout;
    uint32_t report_block_cnt; /* max is 31 */
    bool     blocked;
    bool     print_rtcp_stat;
    bool     rtcp_incremental_ts;
    RtpClientConfig() :
      client_timeout(30),
      client_timeout_count(10),
      wait_interval(100),
      rtcp_sr_interval(5),
      resend_count(30),
      resend_interval(5),
      statistics_count(20000),
      send_buffer_size(8*1024*1024),
      block_timeout(10),
      report_block_cnt(4),
      blocked(false),
      print_rtcp_stat(true),
      rtcp_incremental_ts(true)
    {}
};

class AMConfig;
class AMRtpClient;
class AMRtpClientConfig
{
    friend class AMRtpClient;
  public:
    AMRtpClientConfig();
    virtual ~AMRtpClientConfig();
    RtpClientConfig* get_config(const std::string& conf);

  private:
    RtpClientConfig *m_client_config;
    AMConfig        *m_config;
};

#endif /* ORYX_STREAM_RECORD_MUXERS_MUXER_RTP_AM_RTP_CLIENT_CONFIG_H_ */
