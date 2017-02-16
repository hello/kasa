/*******************************************************************************
 * am_muxer.h
 *
 * History:
 *   2014-12-26 - [ypchang] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/
#ifndef ORYX_STREAM_RECORD_FILTERS_MUXER_AM_MUXER_H_
#define ORYX_STREAM_RECORD_FILTERS_MUXER_AM_MUXER_H_

#include <vector>
struct MuxerConfig;

class AMMuxerInput;
class AMMuxerConfig;
class AMMuxerCodecObj;

class AMMuxer: public AMPacketActiveFilter, public AMIMuxer
{
    friend class AMMuxerCodecObj;
    friend class AMMuxerInput;
    typedef AMPacketActiveFilter inherited;
    typedef std::vector<AMMuxerInput*> AMMuxerInputList;

  public:
    static AMIMuxer* create(AMIEngine *engine, const std::string& config,
                            uint32_t input_num, uint32_t output_num);

  public:
    virtual void* get_interface(AM_REFIID ref_iid);
    virtual void destroy();
    virtual void get_info(INFO& info);
    virtual AMIPacketPin* get_input_pin(uint32_t index);
    virtual AMIPacketPin* get_output_pin(uint32_t index);
    virtual AM_STATE start();
    virtual AM_STATE stop();
    virtual bool start_file_recording();
    virtual bool stop_file_recording();
    virtual uint32_t version();
    virtual AM_MUXER_TYPE type();

  private:
    virtual void on_run();

  private:
    AM_STATE load_muxer_codec();
    void destroy_muxer_codec();

  private:
    AMMuxer(AMIEngine *engine);
    virtual ~AMMuxer();
    AM_STATE init(const std::string& config,
                  uint32_t input_num,
                  uint32_t output_num);

  private:
    MuxerConfig     *m_muxer_config; /* No need to delete */
    AMMuxerConfig   *m_config;
    AMMuxerCodecObj *m_muxer_codec;
    AMMuxerInputList m_inputs;
    uint32_t         m_input_num;
    uint32_t         m_output_num;
    bool             m_run;
    bool             m_started;
};

class AMMuxerInput: public AMPacketQueueInputPin
{
    typedef AMPacketQueueInputPin inherited;
    friend class AMMuxer;

  public:
    static AMMuxerInput* create(AMPacketFilter *filter)
    {
      AMMuxerInput *result = new AMMuxerInput(filter);
      if (AM_UNLIKELY(result && (AM_STATE_OK != result->init()))) {
        delete result;
        result = NULL;
      }
      return result;
    }

  private:
    AMMuxerInput(AMPacketFilter *filter) :
      inherited(filter)
    {}
    virtual ~AMMuxerInput(){}
    AM_STATE init()
    {
      return inherited::init(((AMMuxer*)m_filter)->msgQ());
    }
};

#endif /* ORYX_STREAM_RECORD_FILTERS_MUXER_AM_MUXER_H_ */
