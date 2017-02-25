/*******************************************************************************
 * am_audio_decoder.h
 *
 * History:
 *   2014-9-24 - [ypchang] created file
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
#ifndef AM_AUDIO_DECODER_H_
#define AM_AUDIO_DECODER_H_

#include <atomic>

class AMAudioDecoderInput;
class AMAudioDecoderOutput;
class AMAudioDecoderPacketPool;
class AMAudioCodec;
class AMAudioDecoderConfig;
class AMPlugin;
struct AudioDecoderConfig;

class AMAudioDecoder: public AMPacketActiveFilter, public AMIAudioDecoder
{
    typedef AMPacketActiveFilter inherited;
    friend class AMAudioDecoderInput;

  public:
    static AMIAudioDecoder* create(AMIEngine *engine,
                                   const std::string& config,
                                   uint32_t input_num,
                                   uint32_t output_num);

  public:
    virtual AM_STATE load_audio_codec(AM_AUDIO_CODEC_TYPE codec);
    virtual uint32_t version();
    virtual void* get_interface(AM_REFIID refiid);
    virtual void destroy();
    virtual void get_info(INFO& info);
    virtual AMIPacketPin* get_input_pin(uint32_t index);
    virtual AMIPacketPin* get_output_pin(uint32_t index);
    virtual AM_STATE stop();
    virtual void on_run();

  private:
    inline AM_AUDIO_CODEC_TYPE audio_type_to_codec_type(AM_AUDIO_TYPE type);
    inline AM_STATE on_info(AMPacket *packet);
    inline AM_STATE on_data(AMPacket *packet);
    inline AM_STATE on_eof(AMPacket *packet);

  private:
    AMAudioDecoder(AMIEngine *engine);
    virtual ~AMAudioDecoder();
    AM_STATE init(const std::string& config,
                  uint32_t input_num,
                  uint32_t output_num);
    void send_packet(AMPacket *packet);
    std::string sample_format_to_string(AM_AUDIO_SAMPLE_FORMAT format);

  private:
    AMPlugin                 *m_plugin;
    AMIAudioCodec            *m_audio_codec;
    AMAudioDecoderInput     **m_input_pins;
    AMAudioDecoderOutput    **m_output_pins;
    AMAudioDecoderPacketPool *m_packet_pool;
    AMAudioDecoderConfig     *m_config;
    AudioDecoderConfig       *m_decoder_config;     /* No need to delete */
    uint8_t                  *m_buffer;
    uint32_t                  m_audio_data_size;
    uint32_t                  m_input_num;
    uint32_t                  m_output_num;
    std::atomic_bool          m_run;
    bool                      m_is_info_sent;
    bool                      m_is_pass_through;
    bool                      m_is_codec_changed;
};

class AMAudioDecoderInput: public AMPacketQueueInputPin
{
    typedef AMPacketQueueInputPin inherited;
    friend class AMAudioDecoder;

  public:
    static AMAudioDecoderInput* create(AMPacketFilter *filter)
    {
      AMAudioDecoderInput *result = new AMAudioDecoderInput(filter);
      if (AM_UNLIKELY(result && (AM_STATE_OK != result->init()))) {
        delete result;
        result = NULL;
      }

      return result;
    }

  private:
    AMAudioDecoderInput(AMPacketFilter *filter) :
      inherited(filter)
    {}
    virtual ~AMAudioDecoderInput(){}
    AM_STATE init()
    {
      return inherited::init(((AMAudioDecoder*)m_filter)->msgQ());
    }
};

class AMAudioDecoderOutput: public AMPacketOutputPin
{
    typedef AMPacketOutputPin inherited;
    friend class AMAudioDecoder;

  public:
    static AMAudioDecoderOutput* create(AMPacketFilter *filter)
    {
      AMAudioDecoderOutput *result = new AMAudioDecoderOutput(filter);
      if (AM_UNLIKELY(result && (AM_STATE_OK != result->init()))) {
        delete result;
        result = NULL;
      }
      return result;
    }

  private:
    AMAudioDecoderOutput(AMPacketFilter *filter) :
      inherited(filter)
    {}
    virtual ~AMAudioDecoderOutput(){}
    AM_STATE init(){return AM_STATE_OK;}
};

class AMAudioDecoderPacketPool: public AMSimplePacketPool
{
    typedef AMSimplePacketPool inherited;
    friend class AMAudioDecoder;
  public:
    static AMAudioDecoderPacketPool* create(const char *name,
                                            uint32_t count);
  public:
    bool alloc_packet(AMPacket*& packet, uint32_t size);
    AM_STATE set_buffer(uint8_t *buf, uint32_t dataSize);
    void clear_buffer();

  private:
    AMAudioDecoderPacketPool();
    virtual ~AMAudioDecoderPacketPool();
    AM_STATE init(const char *name, uint32_t count);

  private:
    AMPacket::Payload *m_payload_mem;
    uint8_t           *m_data_mem;
    uint32_t           m_packet_count;
};
#endif /* AM_AUDIO_DECODER_H_ */
