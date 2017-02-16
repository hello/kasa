/*******************************************************************************
 * am_rtp_session_g711.h
 *
 * History:
 *   2015-1-28 - [ypchang] created file
 *
 * Copyright (C) 2008-2015, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/
#ifndef ORYX_STREAM_RECORD_MUXERS_MUXER_RTP_AM_RTP_SESSION_G711_H_
#define ORYX_STREAM_RECORD_MUXERS_MUXER_RTP_AM_RTP_SESSION_G711_H_

#include "am_rtp_session_base.h"

struct AMRtpData;
struct AM_AUDIO_INFO;

class AMRtpSessionG711: public AMRtpSession
{
    typedef AMRtpSession inherited;

  public:
    static AMIRtpSession* create(const std::string &name,
                                 uint32_t queue_size,
                                 AM_AUDIO_INFO *ainfo);

  public:
    virtual uint32_t get_session_clock();
    virtual AM_SESSION_STATE process_data(AMPacket &packet,
                                          uint32_t max_tcp_payload_size);

  private:
    AMRtpSessionG711(const std::string &name, uint32_t queue_size);
    virtual ~AMRtpSessionG711();
    bool init(AM_AUDIO_INFO *ainfo);
    virtual void generate_sdp(std::string& host_ip_addr, uint16_t port,
                              AM_IP_DOMAIN domain);
    virtual std::string get_param_sdp();
    virtual std::string get_payload_type();

  private:
    void assemble_packet(uint8_t *data,
                         uint32_t data_size,
                         uint32_t frame_count,
                         AMRtpData *rtp_data,
                         int32_t pts_incr);

  private:
    AM_AUDIO_INFO *m_audio_info;
    uint32_t       m_media_id;
    std::string    m_media_type;
};

#endif /* ORYX_STREAM_RECORD_MUXERS_MUXER_RTP_AM_RTP_SESSION_G711_H_ */
