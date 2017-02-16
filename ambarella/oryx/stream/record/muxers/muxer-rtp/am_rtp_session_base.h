/*******************************************************************************
 * am_rtp_session_base.h
 *
 * History:
 *   2015-1-5 - [ypchang] created file
 *
 * Copyright (C) 2008-2015, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/
#ifndef ORYX_STREAM_RECORD_MUXERS_MUXER_RTP_AM_RTP_SESSION_BASE_H_
#define ORYX_STREAM_RECORD_MUXERS_MUXER_RTP_AM_RTP_SESSION_BASE_H_

#include "am_rtp_session_if.h"
#include <map>

struct MuxerRtpConfig;

class AMSpinLock;
class AMRtpMuxer;
class AMRtpClient;
class AMRtpSession: public AMIRtpSession
{
    friend class AMRtpMuxer;
    friend class AMRtpClient;
    typedef std::map<uint32_t, AMRtpDataQ*> AMClientDataMap;

  public:
    std::string& get_session_name();
    virtual std::string& get_sdp(std::string& host_ip_addr, uint16_t port,
                                 AM_IP_DOMAIN domain);
    virtual std::string get_param_sdp()    = 0;
    virtual std::string get_payload_type() = 0;
    virtual bool add_client(AMRtpClient *client);
    virtual bool del_client(uint32_t ssrc);
    virtual void set_client_enable(uint32_t ssrc, bool enabled);
    virtual std::string get_client_seq_ntp(uint32_t ssrc);
    virtual AM_SESSION_STATE process_data(AMPacket &packet,
                                          uint32_t max_tcp_payload_size);
    virtual AM_SESSION_TYPE type();
    virtual std::string type_string();
    virtual AMRtpClient* find_client_by_ssrc(uint32_t ssrc);
    virtual void set_config(void *config);

  public:
    AMRtpData* alloc_data(int64_t pts);
    void put_back(AMRtpData *data);
    uint64_t fake_ntp_time_us(uint64_t offset = 0);
    uint64_t get_clock_count_90k();
    uint64_t get_sys_time_mono();
    uint32_t current_rtp_timestamp();

  protected:
    AMRtpSession(const std::string &name,
                 uint32_t queue_size,
                 AM_SESSION_TYPE type);
    virtual ~AMRtpSession();
    bool init();
    virtual void generate_sdp(std::string& host_ip_addr, uint16_t port,
                              AM_IP_DOMAIN domain) = 0;
    void add_rtp_data_to_client(AMRtpData *data);

  private:
    AMRtpDataQ* get_data_queue_by_ssrc(uint32_t ssrc);
    AMRtpData* get_rtp_data_by_ssrc(uint32_t ssrc);
    bool delete_client_by_ssrc(uint32_t ssrc);
    void delete_all_clients();
    void empty_data_queue(AMRtpDataQ& queue);

  protected:
    AMSpinLock     *m_lock_ts;
    AMSpinLock     *m_lock_client;
    AMRtpData      *m_data_buffer;
    MuxerRtpConfig *m_config_rtp;
    AMRtpDataQ      m_data_queue;
    int64_t         m_prev_pts;
    uint32_t        m_queue_size;
    uint32_t        m_curr_time_stamp;
    int             m_hw_timer_fd;
    AM_SESSION_TYPE m_session_type;
    uint16_t        m_sequence_num;
    std::string     m_name;
    std::string     m_sdp;
    AMRtpClientList m_client_list;
    AMClientDataMap m_client_data_map;
};

#endif /* ORYX_STREAM_RECORD_MUXERS_MUXER_RTP_AM_RTP_SESSION_BASE_H_ */
