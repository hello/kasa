/*******************************************************************************
 * am_muxer_rtp_config.h
 *
 * History:
 *   2015-1-9 - [ypchang] created file
 *
 * Copyright (C) 2008-2015, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/
#ifndef ORYX_STREAM_RECORD_MUXERS_MUXER_RTP_AM_MUXER_RTP_CONFIG_H_
#define ORYX_STREAM_RECORD_MUXERS_MUXER_RTP_AM_MUXER_RTP_CONFIG_H_

struct MuxerRtpConfig
{
    uint16_t max_send_queue_size;
    uint16_t max_tcp_payload_size;
    uint16_t max_proto_client;
    uint16_t sock_timeout;
    bool     dynamic_audio_pts_diff;
    bool     ntp_use_hw_timer;
    MuxerRtpConfig() :
      max_send_queue_size(10),
      max_tcp_payload_size(1448),
      max_proto_client(2),
      sock_timeout(10),
      dynamic_audio_pts_diff(true),
      ntp_use_hw_timer(false)
    {}
};

class AMConfig;
class AMMuxerRtp;
class AMMuxerRtpConfig
{
    friend class AMMuxerRtp;
  public:
    AMMuxerRtpConfig();
    virtual ~AMMuxerRtpConfig();
    MuxerRtpConfig* get_config(const std::string& conf);

  private:
    MuxerRtpConfig *m_muxer_config;
    AMConfig       *m_config;
};

#endif /* ORYX_STREAM_RECORD_MUXERS_MUXER_RTP_AM_MUXER_RTP_CONFIG_H_ */
