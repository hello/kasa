/*******************************************************************************
 * am_rtp_session_base.h
 *
 * History:
 *   2015-1-5 - [ypchang] created file
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
#ifndef ORYX_STREAM_RECORD_MUXERS_MUXER_RTP_AM_RTP_SESSION_BASE_H_
#define ORYX_STREAM_RECORD_MUXERS_MUXER_RTP_AM_RTP_SESSION_BASE_H_

#include "am_rtp_session_if.h"
#include "am_mutex.h"
#include <map>

struct RtpClientDataQ
{
    AMRtpDataQ data_q;
    RtpClientDataQ();
    ~RtpClientDataQ();
    void clear();
    void push(AMRtpData *data);
    AMRtpData* front_pop();
};

struct MuxerRtpConfig;

class AMRtpMuxer;
class AMRtpClient;
class AMRtpSession: public AMIRtpSession
{
    friend class AMRtpMuxer;
    friend class AMRtpClient;
    typedef std::map<uint32_t, RtpClientDataQ*> AMClientDataMap;

  public:
    std::string& get_session_name();
    virtual std::string& get_sdp(std::string& host_ip_addr, uint16_t port,
                                 AM_IP_DOMAIN domain);
    virtual std::string get_param_sdp()    = 0;
    virtual std::string get_payload_type() = 0;
    virtual bool add_client(AMRtpClient *client);
    virtual bool del_client(uint32_t ssrc);
    virtual void del_clients_all(bool notify);
    virtual void set_client_enable(uint32_t ssrc, bool enabled);
    virtual std::string get_client_seq_ntp(uint32_t ssrc);
    virtual AM_SESSION_STATE process_data(AMPacket &packet,
                                          uint32_t max_tcp_payload_size);
    virtual AM_SESSION_TYPE type();
    virtual std::string type_string();
    virtual AMRtpClient* find_client_by_ssrc(uint32_t ssrc);
    virtual void put_back(AMRtpData *data);
    virtual uint64_t fake_ntp_time_us(uint64_t *clock_count_90k = nullptr,
                                      uint64_t offset = 0);
    virtual uint64_t get_clock_count_90k();
    virtual bool is_client_ready(uint32_t ssrc);

  public:
    AMRtpData* alloc_data(int64_t pts);
    uint64_t get_sys_time_mono();
    uint32_t current_rtp_timestamp();

  protected:
    AMRtpSession(const std::string &name,
                 MuxerRtpConfig *config,
                 AM_SESSION_TYPE type);
    virtual ~AMRtpSession();
    bool init();
    virtual void generate_sdp(std::string& host_ip_addr, uint16_t port,
                              AM_IP_DOMAIN domain) = 0;
    void add_rtp_data_to_client(AMRtpData *data);

  private:
    virtual AMRtpData* get_rtp_data_by_ssrc(uint32_t ssrc);
    bool delete_client_by_ssrc(uint32_t ssrc);
    void delete_all_clients(bool send_notify = false);

  protected:
    int64_t         m_prev_pts;
    AMRtpData      *m_data_buffer;
    MuxerRtpConfig *m_config_rtp;
    uint32_t        m_curr_time_stamp;
    int             m_hw_timer_fd;
    AM_SESSION_TYPE m_session_type;
    uint16_t        m_sequence_num;
    std::string     m_name;
    std::string     m_sdp;
    AMRtpClientList m_client_list;
    AMClientDataMap m_client_data_map;
    AMRtpDataQ      m_data_queue;
    AMMemLock       m_lock_ts;
    AMMemLock       m_lock_client;
};

#endif /* ORYX_STREAM_RECORD_MUXERS_MUXER_RTP_AM_RTP_SESSION_BASE_H_ */
