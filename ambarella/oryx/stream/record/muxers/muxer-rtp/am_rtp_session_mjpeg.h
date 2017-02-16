/*******************************************************************************
 * am_rtp_session_mjpeg.h
 *
 * History:
 *   2015-1-16 - [ypchang] created file
 *
 * Copyright (C) 2008-2015, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/
#ifndef ORYX_STREAM_RECORD_MUXERS_MUXER_RTP_AM_RTP_SESSION_MJPEG_H_
#define ORYX_STREAM_RECORD_MUXERS_MUXER_RTP_AM_RTP_SESSION_MJPEG_H_

#include "am_rtp_session_base.h"

struct AMJpegParams;
struct AMRtpData;
struct AM_VIDEO_INFO;

class AMRtpSessionMjpeg: public AMRtpSession
{
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
    AMRtpSessionMjpeg(const std::string &name, uint32_t queue_size);
    virtual ~AMRtpSessionMjpeg();
    bool init(AM_VIDEO_INFO *vinfo);
    virtual void generate_sdp(std::string& host_ip_addr, uint16_t port,
                              AM_IP_DOMAIN domain);
    virtual std::string get_param_sdp();
    virtual std::string get_payload_type();

  private:
    bool parse_jpeg(AMJpegParams *jpeg, uint8_t *data, uint32_t len);
    void assemble_packet(AMRtpData *rtp_data,
                         int32_t pts_incr,
                         uint32_t max_tcp_payload_size,
                         uint8_t q_factor,
                         bool need_q_table);

  private:
    AM_VIDEO_INFO *m_video_info;
    AMJpegParams  *m_jpeg_param;
};


#endif /* ORYX_STREAM_RECORD_MUXERS_MUXER_RTP_AM_RTP_SESSION_MJPEG_H_ */
