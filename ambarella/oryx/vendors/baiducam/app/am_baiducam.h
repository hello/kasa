/*
 * am_baiducam.h
 *
 *  History:
 *    Apr 9, 2015 - [Shupeng Ren] created file
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
 */
#ifndef _AM_BAIDUCAM_H_
#define _AM_BAIDUCAM_H_

#include <deque>
#include "h264.h"
#include "adts.h"

enum AM_BD_AUDIO_FREQ_INDEX {
  AM_BD_AUDIO_96k     = 0,
  AM_BD_AUDIO_48k     = 3,
  AM_BD_AUDIO_44_1k   = 4,
  AM_BD_AUDIO_32k     = 5,
  AM_BD_AUDIO_24k     = 6,
  AM_BD_AUDIO_16k     = 8,
  AM_BD_AUDIO_8k      = 11,
};

struct AM_BD_DataBuffer {
    uint32_t size;
    char *buffer;
    AM_BD_DataBuffer() :
      size(0),
      buffer(nullptr)
    {}
};

struct AM_BD_SpsPpsData {
    uint32_t sps_len;
    uint8_t  sps[128];
    uint32_t pps_len;
    uint8_t  pps[64];
};

enum AM_BD_PROCESS_STATE {
  AM_BD_STATE_WAIT_FOR_INFOS = 0,
  AM_BD_STATE_WAIT_FOR_SPS_PPS,
  AM_BD_STATE_SEND_DATA,
  AM_BD_STATE_RECONNECT_TO_BLS
};

class AMBaiduCam {
  public:
    static AMBaiduCam* create(uint32_t stream_id);
    void destroy();

  public:
    AMBaiduCam(uint32_t stream_id);
    ~AMBaiduCam();

  public:
    void set_auth_info(std::string &url,
                       std::string &devid,
                       std::string &access_token,
                       std::string &stream_name);
    bool connect_bls();
    bool connect_bls(std::string &session_token);
    bool connect_bls(std::string &url,
                     std::string &devid,
                     std::string &access_token,
                     std::string &stream_name,
                     std::string &session_token);

    bool disconnect_bls();
    bool reconnect_bls();
    bool isconnect_bls();
    bool process_packet(AMExportPacket *packet);

  private:
    bool init();
    bool usrcmd_callback(const char *cmd);
    bool write_audio_header();
    bool write_video_header();
    bool write_audio_data(AMExportPacket *packet, uint64_t pts);
    bool write_video_data(AMExportPacket *packet, uint64_t pts);
    bool get_nalus(AMExportPacket *packet);

    static void thread_entry(void *arg);
    static bool static_usercmd_callback(const char *cmd, void *arg);

  private:
    AMBLSRtmp *m_rtmp;

    AM_BD_DataBuffer m_video_buffer;
    AM_BD_DataBuffer m_audio_buffer;
    AM_BD_SpsPpsData m_sps_pps;

    AM_RTMP_VideoMetaData m_video_meta;
    AM_RTMP_AudioMetaData m_audio_meta;

    std::deque<AM_H264_NALU> m_nalu_list;
    bool m_sps_come_flag;
    bool m_pps_come_flag;
    bool m_video_info_come_flag;
    bool m_audio_info_come_flag;
    uint32_t m_video_stream_id;
    AM_BD_PROCESS_STATE m_state;
    std::pair<bool, uint64_t> m_first_pts;
    uint64_t m_last_pts;

    bool        m_thread_exit;
    AMEvent    *m_event;
    AMThread   *m_usrcmd_thread;

  private:
    std::string m_url;
    std::string m_devid;
    std::string m_access_token;
    std::string m_stream_name;

    std::string m_session_token;
};

#endif /* _AM_BAIDUCAM_H_ */
