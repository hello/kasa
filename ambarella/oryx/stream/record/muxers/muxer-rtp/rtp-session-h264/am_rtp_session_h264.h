/*******************************************************************************
 * am_rtp_session_h264.h
 *
 * History:
 *   2015-1-4 - [ypchang] created file
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
#ifndef ORYX_STREAM_RECORD_MUXERS_MUXER_RTP_AM_RTP_SESSION_H264_H_
#define ORYX_STREAM_RECORD_MUXERS_MUXER_RTP_AM_RTP_SESSION_H264_H_

#include "am_rtp_session_base.h"
#include "am_mutex.h"
#include <vector>

struct AM_H264_NALU;
struct AMRtpData;
struct AM_VIDEO_INFO;
class AMRtpSessionH264: public AMRtpSession
{
    typedef std::vector<AM_H264_NALU> AMNaluList;
    typedef AMRtpSession inherited;
  public:
    static AMIRtpSession* create(const std::string &name,
                                 MuxerRtpConfig *muxer_rtp_config,
                                 AM_VIDEO_INFO *vinfo);

  public:
    virtual uint32_t get_session_clock();
    virtual AM_SESSION_STATE process_data(AMPacket &packet,
                                          uint32_t max_tcp_payload_size);

  private:
    AMRtpSessionH264(const std::string &name, MuxerRtpConfig *muxer_rtp_config);
    virtual ~AMRtpSessionH264();
    bool init(AM_VIDEO_INFO *vinfo);
    virtual void generate_sdp(std::string& host_ip_addr, uint16_t port,
                              AM_IP_DOMAIN domain);
    virtual std::string get_param_sdp();
    virtual std::string get_payload_type();

  private:
    std::string& sps();
    std::string& pps();
    uint32_t profile_level_id();
    bool find_nalu(uint8_t *data, uint32_t len, H264_NALU_TYPE expect);
    uint32_t calc_h264_packet_num(AM_H264_NALU &nalu, uint32_t max_packet_size);
    void assemble_packet(AMRtpData *rtp_data,
                         int32_t pts_incr,
                         uint32_t max_tcp_payload_size,
                         uint32_t max_packet_size);

  private:
    AM_VIDEO_INFO *m_video_info;
    uint32_t       m_profile_level_id;
    AMNaluList     m_nalu_list;
    std::string    m_sps;
    std::string    m_pps;
    AMMemLock      m_lock;
};


#endif /* ORYX_STREAM_RECORD_MUXERS_MUXER_RTP_AM_RTP_SESSION_H264_H_ */
