/*******************************************************************************
 * am_player.h
 *
 * History:
 *   2014-9-10 - [ypchang] created file
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
#ifndef AM_PLAYER_H_
#define AM_PLAYER_H_

#include "am_mutex.h"
#include "am_queue.h"
#include "am_audio_player_if.h"

#include <atomic>

typedef AMSafeQueue<AMPacket*> PacketQueue;

struct PlayerConfig;
class AMEvent;
class AMPlayerInput;
class AMPlayerAudio;
class AMPlayerConfig;
class AMPlayer: public AMPacketActiveFilter, public AMIPlayer
{
    typedef AMPacketActiveFilter inherited;
    friend class AMPlayerInput;

  public:
    static AMIPlayer* create(AMIEngine *engine, const std::string& config,
                             uint32_t input_num, uint32_t output_num);

  public:
    virtual void* get_interface(AM_REFIID refiid);
    virtual void destroy();
    virtual void get_info(INFO& info);
    virtual AMIPacketPin* get_input_pin(uint32_t id);
    virtual AMIPacketPin* get_output_pin(uint32_t id);
    virtual AM_STATE start();
    virtual AM_STATE stop();
    virtual AM_STATE pause(bool enabled);
    virtual void     set_aec_enabled(bool enabled);
    virtual uint32_t version();
    virtual void on_run();

  private:
    inline AM_STATE on_info(AMPacket *packet);
    inline AM_STATE on_data(AMPacket *packet);
    inline AM_STATE on_eof(AMPacket *packet);
    AM_STATE process_packet(AMPacket *packet);

  private:
    AMPlayer(AMIEngine *engine);
    virtual ~AMPlayer();
    AM_STATE init(const std::string& config,
                  uint32_t input_num,
                  uint32_t output_num);

  private:
    static void static_player_callback(AudioPlayer *data);
    void player_callback(AudioData *data);

  private:
    AMIAudioPlayer   *m_player_audio  = nullptr;
    AMPlayerConfig   *m_config        = nullptr;
    AMEvent          *m_event         = nullptr;
    PlayerConfig     *m_player_config = nullptr; /* No need to delete */
    AMPlayerInput   **m_input_pins    = nullptr;
    PacketQueue      *m_audio_queue   = nullptr;
    uint32_t          m_eos_map       = 0;
    uint32_t          m_input_num     = 0;
    uint32_t          m_output_num    = 0;
    std::atomic<bool> m_run           = {false};
    std::atomic<bool> m_is_paused     = {false};
    std::atomic<bool> m_wait_finish   = {false};
    AM_AUDIO_INFO     m_last_audio_info;
    AMMemLock         m_lock;
};

class AMPlayerInput: public AMPacketActiveInputPin
{
    typedef AMPacketActiveInputPin inherited;
    friend class AMPlayer;

  public:
    static AMPlayerInput* create(AMPacketFilter *filter, const char *name)
    {
      AMPlayerInput *result = new AMPlayerInput(filter);
      if (AM_UNLIKELY(result && (AM_STATE_OK != result->init(name)))) {
        delete result;
        result = NULL;
      }
      return result;
    }

  private:
    AMPlayerInput(AMPacketFilter *filter) :
      inherited(filter){}
    virtual ~AMPlayerInput(){}
    AM_STATE init(const char *name)
    {
      return inherited::init(name);
    }
    virtual AM_STATE process_packet(AMPacket *packet)
    {
      return ((AMPlayer*)m_filter)->process_packet(packet);
    }
};

#endif /* AM_PLAYER_H_ */
