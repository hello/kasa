/*******************************************************************************
 * am_rtp_session_speex.h
 *
 * History:
 *   Jul 25, 2015 - [ypchang] created file
 *
 * Copyright (C) 2008-2015, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/
#ifndef AM_RTP_SESSION_SPEEX_H_
#define AM_RTP_SESSION_SPEEX_H_

#include "am_rtp_session_base.h"

struct AMRtpData;
struct AM_AUDIO_INFO;

class AMRtpSessionSpeex: public AMRtpSession
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
    AMRtpSessionSpeex(const std::string &name,
                      uint32_t queue_size);
    virtual ~AMRtpSessionSpeex();
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
    uint32_t       m_ptime;
};


#endif /* AM_RTP_SESSION_SPEEX_H_ */
