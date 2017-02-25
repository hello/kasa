/*******************************************************************************
 * am_rtp_client_config.h
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
#ifndef ORYX_STREAM_RECORD_MUXERS_MUXER_RTP_AM_RTP_CLIENT_CONFIG_H_
#define ORYX_STREAM_RECORD_MUXERS_MUXER_RTP_AM_RTP_CLIENT_CONFIG_H_

struct RtpClientConfig
{
    uint32_t client_timeout = 30;
    uint32_t client_timeout_count = 10;
    uint32_t wait_interval = 100;
    uint32_t rtcp_sr_interval = 5;
    uint32_t resend_count = 30;
    uint32_t resend_interval = 5;
    uint32_t statistics_count = 20000;
    uint32_t send_buffer_size = 512*1024; /* bytes */
    uint32_t block_timeout = 10;
    uint32_t report_block_cnt = 4; /* max is 31 */
    bool     blocked = false;
    bool     print_rtcp_stat = true;
    bool     rtcp_incremental_ts = true;
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
