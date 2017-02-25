/*******************************************************************************
 * am_audio_source.h
 *
 * History:
 *   2014-12-2 - [ypchang] created file
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
#ifndef AM_AUDIO_SOURCE_H_
#define AM_AUDIO_SOURCE_H_

#include "am_mutex.h"
#include <queue>
#include <atomic>
#include <vector>

struct AudioPacket;
struct AudioCapture;
struct AudioSourceConfig;

class AMEvent;
class AMPlugin;
class AMIAudioCapture;
class AMAudioCodecObj;
class AMAudioSourceOutput;
class AMAudioSourceConfig;

class AMAudioSource: public AMPacketActiveFilter, public AMIAudioSource
{
    friend class AMAudioCodecObj;
    typedef AMPacketActiveFilter inherited;
    typedef std::vector<AMPacketOutputPin*> AMOutputPinList;

  public:
    static AMIAudioSource* create(AMIEngine *engine, const std::string& config,
                                  uint32_t input_num, uint32_t output_num);

  public:
    virtual void* get_interface(AM_REFIID ref_iid);
    virtual void destroy();
    virtual void get_info(INFO& info);
    virtual AMIPacketPin* get_input_pin(uint32_t index);
    virtual AMIPacketPin* get_output_pin(uint32_t index);
    virtual AM_STATE start();
    virtual AM_STATE stop();
    virtual AM_STATE enable_codec(AM_AUDIO_TYPE type, bool enable);
    virtual uint32_t get_audio_number();
    virtual uint32_t get_audio_sample_rate();
    virtual void     set_stream_id(uint32_t base);
    virtual void     set_aec_enabled(bool enabled);
    virtual uint32_t version();

  private:
    void abort();
    virtual void on_run();

  private:
    AM_STATE load_audio_codec();
    void destroy_audio_codec();
    static void static_audio_capture(AudioCapture *data);
    void audio_capture(AudioPacket *data);
    bool set_audio_parameters();
    void send_packet(AMPacket *packet);
    AMAudioCodecObj* get_codec_by_type(AM_AUDIO_TYPE type);
    std::string audio_type_to_string(AM_AUDIO_TYPE type);

  private:
    AMAudioSource(AMIEngine *engine);
    virtual ~AMAudioSource();
    AM_STATE init(const std::string& config,
                  uint32_t input_num,
                  uint32_t output_num);

  private:
    AudioSourceConfig   *m_aconfig        = nullptr; /* No need to delete */
    AMAudioSourceConfig *m_config         = nullptr;
    AMAudioCodecObj     *m_audio_codec    = nullptr;
    AMIAudioCapture     *m_audio_capture  = nullptr;
    AMFixedPacketPool   *m_packet_pool    = nullptr;
    AMEvent             *m_event          = nullptr;
    uint32_t             m_input_num      = 0;
    uint32_t             m_output_num     = 0;
    std::atomic_bool     m_run            = {false};
    std::atomic_bool     m_started        = {false};
    std::atomic_bool     m_abort          = {false};
    std::atomic_bool     m_audio_disabled = {false};
    std::atomic_bool     m_codec_enable   = {false};
    AMOutputPinList      m_outputs;
    AM_AUDIO_INFO        m_src_audio_info;
    AMMemLock            m_lock;
};

class AMAudioSourceOutput: public AMPacketOutputPin
{
    typedef AMPacketOutputPin inherited;
    friend class AMAudioSource;

  public:
    static AMAudioSourceOutput* create(AMPacketFilter *filter)
    {
      return (new AMAudioSourceOutput(filter));
    }

  private:
    AMAudioSourceOutput(AMPacketFilter *filter) :
      inherited(filter){}
    virtual ~AMAudioSourceOutput(){}
};

#endif /* AM_AUDIO_SOURCE_H_ */
