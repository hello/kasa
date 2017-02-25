/*******************************************************************************
 * am_rtp_session.h
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
#ifndef ORYX_STREAM_RECORD_MUXERS_MUXER_RTP_AM_RTP_SESSION_IF_H_
#define ORYX_STREAM_RECORD_MUXERS_MUXER_RTP_AM_RTP_SESSION_IF_H_

#define NTP_OFFSET    2208988800ULL
#define NTP_OFFSET_US (NTP_OFFSET * 1000000ULL)

#include "am_rtp_msg.h"
#include "am_queue.h"
#include "rtp.h"

#include <vector>
#include <atomic>

enum AM_NET_PACKET_HDR_SIZE
{
  HEADER_SIZE_TCP = 40,
  HEADER_SIZE_UDP = 28,
};

enum AM_SESSION_TYPE
{
  AM_SESSION_AAC,
  AM_SESSION_OPUS,
  AM_SESSION_G726,
  AM_SESSION_G711,
  AM_SESSION_SPEEX,
  AM_SESSION_H264,
  AM_SESSION_H265,
  AM_SESSION_MJPEG,
};

enum AM_SESSION_STATE
{
  AM_SESSION_OK,
  AM_SESSION_ERROR,
  AM_SESSION_IO_SLOW,
};

struct timespec;
struct AMRtpData;
struct AM_VIDEO_INFO;
struct AM_AUDIO_INFO;
struct MuxerRtpConfig;
typedef AMSafeQueue<AMRtpData*> AMRtpDataQ;

class AMPacket;
class AMRtpClient;
class AMIRtpSession
{
    friend class AMRtpMuxer;
    friend class AMRtpClient;
  public:
    typedef std::vector<AMRtpClient*> AMRtpClientList;

  public:
    virtual std::string& get_session_name()                               = 0;
    virtual std::string& get_sdp(std::string& host_ip_addr, uint16_t port,
                                 AM_IP_DOMAIN domain)                     = 0;
    virtual std::string get_param_sdp()                                   = 0;
    virtual std::string get_payload_type()                                = 0;
    virtual bool add_client(AMRtpClient *client)                          = 0;
    virtual bool del_client(uint32_t ssrc)                                = 0;
    virtual void del_clients_all(bool notify)                             = 0;
    virtual void set_client_enable(uint32_t ssrc, bool enabled)           = 0;
    virtual std::string get_client_seq_ntp(uint32_t ssrc)                 = 0;
    virtual uint32_t get_session_clock()                                  = 0;
    virtual AM_SESSION_STATE process_data(AMPacket &packet,
                                          uint32_t max_packet_size)       = 0;
    virtual AM_SESSION_TYPE type()                                        = 0;
    virtual std::string type_string()                                     = 0;
    virtual AMRtpClient* find_client_by_ssrc(uint32_t ssrc)               = 0;
    virtual void put_back(AMRtpData *data)                                = 0;
    virtual uint64_t fake_ntp_time_us(uint64_t *clock_count_90k = nullptr,
                                      uint64_t offset = 0)                = 0;
    virtual uint64_t get_clock_count_90k()                                = 0;
    virtual bool is_client_ready(uint32_t ssrc)                           = 0;
    virtual ~AMIRtpSession(){}

  private:
    virtual AMRtpData* get_rtp_data_by_ssrc(uint32_t ssrc)                = 0;
};

typedef AMIRtpSession* (*CreateRtpSession)(const char *session_name, \
                                           MuxerRtpConfig *rtp_muxer_config, \
                                           void *info);
#define CREATE_RTP_SESSION ((const char*)"create_rtp_session")

#endif /* ORYX_STREAM_RECORD_MUXERS_MUXER_RTP_AM_RTP_SESSION_IF_H_ */
