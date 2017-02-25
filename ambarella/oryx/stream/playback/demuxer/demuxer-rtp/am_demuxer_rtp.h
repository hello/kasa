/*******************************************************************************
 * am_demuxer_rtp.h
 *
 * History:
 *   2014-11-16 - [ccjing] created file
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
#ifndef AM_DEMUXER_RTP_H_
#define AM_DEMUXER_RTP_H_

#include "am_audio_type.h"
#include "am_queue.h"
#include "am_mutex.h"
#include <atomic>

typedef AMSafeQueue<AMPacket*> packet_queue;
struct AMPlaybackRtpUri;

class AMDemuxerRtp: public AMDemuxer
{
    typedef AMDemuxer inherited;
  public:
    static AMIDemuxerCodec* create(uint32_t streamid);

  public:
    virtual void destroy();
    virtual void enable(bool enabled);//rewrite enable
    virtual bool is_drained();
    virtual bool is_play_list_empty();//rewrite is_play_list_empty
    virtual bool add_uri(const AMPlaybackUri& uri);//rewrite add_uri
    virtual AM_DEMUXER_STATE get_packet(AMPacket *&packet);

    void stop();
    bool add_udp_port(const AMPlaybackRtpUri& rtp_uri); /*create socket and thread*/
    static void thread_entry(void *p);
    void main_loop();

  protected:
    AMDemuxerRtp(uint32_t streamid);
    virtual ~AMDemuxerRtp();
    AM_STATE init();
    std::string sample_format_to_string(AM_AUDIO_SAMPLE_FORMAT format);
    std::string audio_type_to_string(AM_AUDIO_TYPE type);

  private:
    AM_AUDIO_INFO      *m_audio_info       = nullptr;
    char               *m_recv_buf         = nullptr;
    AMThread           *m_thread           = nullptr;//create in the add_udp_port
    packet_queue       *m_packet_queue     = nullptr;
    int                 m_sock_fd          = -1;
    uint32_t            m_packet_count     = 0;
    AM_AUDIO_CODEC_TYPE m_audio_codec_type = AM_AUDIO_CODEC_NONE; /*used in packet->set_frame_type()*/
    bool                m_is_new_sock      = true;
    bool                m_is_info_ready    = false;
    uint8_t             m_audio_rtp_type   = (uint8_t)-1;/*parse rtp packet header*/
    std::atomic_bool    m_run              = {false};/* running flag of the thread*/
    AMMemLock           m_sock_lock;
};

#endif /* AM_DEMUXER_RTP_H_ */
