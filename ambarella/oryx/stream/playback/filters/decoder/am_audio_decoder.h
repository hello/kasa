/*******************************************************************************
 * am_audio_decoder.h
 *
 * History:
 *   2014-9-24 - [ypchang] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/
#ifndef AM_AUDIO_DECODER_H_
#define AM_AUDIO_DECODER_H_

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
    bool                      m_run;
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
