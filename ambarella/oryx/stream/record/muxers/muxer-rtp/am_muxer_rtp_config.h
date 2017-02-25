/*******************************************************************************
 * am_muxer_rtp_config.h
 *
 * History:
 *   2015-1-9 - [ypchang] created file
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
#ifndef ORYX_STREAM_RECORD_MUXERS_MUXER_RTP_AM_MUXER_RTP_CONFIG_H_
#define ORYX_STREAM_RECORD_MUXERS_MUXER_RTP_AM_MUXER_RTP_CONFIG_H_

#include <map>
struct RtpSessionType
{
    std::string codec;
    std::string name;
};
typedef std::map<int32_t, RtpSessionType> RtpSessionTypeMap;

struct MuxerRtpConfig
{
    uint16_t          muxer_id = 0;
    uint16_t          max_send_queue_size = 10;
    uint16_t          max_tcp_payload_size = 1448;
    uint16_t          max_proto_client = 2;
    uint16_t          max_alloc_wait_count = 20;
    uint16_t          alloc_wait_interval = 10;
    uint16_t          kill_session_timeout = 4000;
    uint16_t          sock_timeout = 10;
    bool              del_buffer_on_release = false;
    bool              dynamic_audio_pts_diff = false;
    bool              ntp_use_hw_timer = false;
    RtpSessionTypeMap rtp_session_map;
    MuxerRtpConfig()
    {
      rtp_session_map.clear();
    }
    ~MuxerRtpConfig()
    {
      rtp_session_map.clear();
    }
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

#ifdef BUILD_AMBARELLA_ORYX_MUXER_DIR
#define ORYX_MUXER_DIR ((const char*)BUILD_AMBARELLA_ORYX_MUXER_DIR)
#else
#define ORYX_MUXER_DIR ((const char*)"/usr/lib/oryx/muxer")
#endif

#endif /* ORYX_STREAM_RECORD_MUXERS_MUXER_RTP_AM_MUXER_RTP_CONFIG_H_ */
