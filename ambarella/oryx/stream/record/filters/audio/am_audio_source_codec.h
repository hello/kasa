/*******************************************************************************
 * am_audio_source_codec.h
 *
 * History:
 *   2014-12-8 - [ypchang] created file
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
#ifndef AM_AUDIO_SOURCE_CODEC_H_
#define AM_AUDIO_SOURCE_CODEC_H_

#include "am_queue.h"
#include "am_audio_define.h"

class AMEvent;
class AMPlugin;
class AMthread;
class AMIAudioCodec;
class AMAudioSource;
class AMFixedPacketPool;

class AMAudioCodecObj
{
    typedef AMSafeQueue<AMPacket*> PacketQ;

    friend class AMAudioSource;

  public:
    AMAudioCodecObj();
    virtual ~AMAudioCodecObj();

  public:
    bool is_valid();
    bool is_running();
    bool load_codec(std::string& name);
    bool initialize(AM_AUDIO_INFO *src_audio_info,
                    uint32_t id,
                    uint32_t packet_pool_size);
    void finalize();
    void push_queue(AMPacket *&packet);
    void clear();
    bool start(bool RTPrio, uint32_t priority);
    void stop();
    void set_stream_id(uint32_t id);
    AM_AUDIO_TYPE get_type();

  private:
    static void static_encode(void *data);
    void encode();

  private:
    int64_t            m_last_sent_pts = 0LL;
    AMIAudioCodec     *m_codec         = nullptr;
    AMPlugin          *m_plugin        = nullptr;
    AMFixedPacketPool *m_pool          = nullptr;
    AM_AUDIO_INFO     *m_codec_info    = nullptr;
    AMThread          *m_thread        = nullptr;
    AMEvent           *m_encode_event  = nullptr;
    AMAudioSource     *m_audio_source  = nullptr;
    uint32_t           m_id            = ((uint32_t)-1);
    uint32_t           m_multiple      = 1;
    uint16_t           m_codec_type    = ((uint16_t)-1);
    std::atomic<bool>  m_run           = {false};
    std::string        m_name;
    PacketQ            m_packet_q;
    AM_AUDIO_INFO      m_codec_required_info;
};

#endif /* AM_AUDIO_SOURCE_CODEC_H_ */
