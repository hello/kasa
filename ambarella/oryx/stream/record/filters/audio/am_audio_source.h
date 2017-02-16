/*******************************************************************************
 * am_audio_source.h
 *
 * History:
 *   2014-12-2 - [ypchang] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/
#ifndef AM_AUDIO_SOURCE_H_
#define AM_AUDIO_SOURCE_H_

#include <queue>
#include <vector>

struct AudioPacket;
struct AudioCapture;
struct AudioSourceConfig;

class AMEvent;
class AMPlugin;
class AMSpinLock;
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

  private:
    AMAudioSource(AMIEngine *engine);
    virtual ~AMAudioSource();
    AM_STATE init(const std::string& config,
                  uint32_t input_num,
                  uint32_t output_num);

  private:
    AudioSourceConfig   *m_aconfig; /* No need to delete */
    AMAudioSourceConfig *m_config;
    AMAudioCodecObj     *m_audio_codec;
    AMIAudioCapture     *m_audio_capture;
    AMFixedPacketPool   *m_packet_pool;
    AMSpinLock          *m_lock;
    AMEvent             *m_event;
    AMOutputPinList      m_outputs;
    AM_AUDIO_INFO        m_src_audio_info;
    uint32_t             m_input_num;
    uint32_t             m_output_num;
    bool                 m_run;
    bool                 m_started;
    bool                 m_abort;
    bool                 m_audio_disabled;
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
