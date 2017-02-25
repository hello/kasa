/*******************************************************************************
 * am_rtp_data.h
 *
 * History:
 *   2015-1-6 - [ypchang] created file
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
#ifndef ORYX_STREAM_RECORD_MUXERS_MUXER_RTP_AM_RTP_DATA_H_
#define ORYX_STREAM_RECORD_MUXERS_MUXER_RTP_AM_RTP_DATA_H_

#include "am_mutex.h"

struct AMRtpPacket
{
    uint8_t *tcp_data   = nullptr;
    uint8_t *udp_data   = nullptr;
    uint32_t total_size = 0;
};

class AMIRtpSession;
class AMRtpSession;
class AMRtpData
{
    friend class AMRtpSession;

  public:
    AMRtpData();
    ~AMRtpData();

  public:
    bool create(AMIRtpSession *session,
                uint32_t datasize,
                uint32_t payloadsize,
                uint32_t packet_num);
    void add_ref();
    void release();
    int64_t      get_pts();
    uint8_t*     get_buffer();
    AMRtpPacket* get_packet();
    uint32_t     get_packet_number();
    uint32_t     get_payload_size();
    bool         is_key_frame();
    void         set_key_frame(bool key);

  protected:
    void clear();

  protected:
    int64_t           m_pts          = 0LL;
    uint8_t          *m_buffer       = nullptr;
    AMRtpPacket      *m_packet       = nullptr;
    AMIRtpSession    *m_owner        = nullptr;
    uint32_t          m_id           = 0;;
    uint32_t          m_buffer_size  = 0;
    uint32_t          m_payload_size = 0;
    uint32_t          m_pkt_size     = 0;
    uint32_t          m_pkt_num      = 0;
    std::atomic<int>  m_ref_count    = {0};
    std::atomic<bool> m_is_key_frame = {true};
    AMMemLock         m_lock;
    AMMemLock         m_ref_lock;
};

#endif /* ORYX_STREAM_RECORD_MUXERS_MUXER_RTP_AM_RTP_DATA_H_ */
