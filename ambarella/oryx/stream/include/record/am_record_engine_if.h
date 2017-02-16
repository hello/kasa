/*******************************************************************************
 * am_record_engine_if.h
 *
 * History:
 *   2014-12-30 - [ypchang] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/
#ifndef ORYX_STREAM_INCLUDE_RECORD_AM_RECORD_ENGINE_IF_H_
#define ORYX_STREAM_INCLUDE_RECORD_AM_RECORD_ENGINE_IF_H_

#include "am_record_msg.h"

extern const AM_IID IID_AMIRecordEngine;

struct AMRecordMsg;
class AMIRecordEngine: public AMIInterface
{
  public:
    enum AM_RECORD_ENGINE_STATUS
    {
      AM_RECORD_ENGINE_ERROR,
      AM_RECORD_ENGINE_TIMEOUT,
      AM_RECORD_ENGINE_STARTING,
      AM_RECORD_ENGINE_RECORDING,
      AM_RECORD_ENGINE_STOPPING,
      AM_RECORD_ENGINE_STOPPED
    };

    static AMIRecordEngine* create();

  public:
    DECLARE_INTERFACE(AMIRecordEngine, IID_AMIRecordEngine);
    virtual AM_RECORD_ENGINE_STATUS get_engine_status() = 0;
    virtual bool create_graph()                         = 0;
    virtual bool record()                               = 0;
    virtual bool stop()                                 = 0;
    virtual bool start_file_recording()                 = 0;
    virtual bool stop_file_recording()                  = 0;
    virtual void set_app_msg_callback(AMRecordCallback callback,
                                      void *data)       = 0;
    virtual bool send_event()                           = 0;
    virtual bool is_ready_for_event()                   = 0;
};

#endif /* ORYX_STREAM_INCLUDE_RECORD_AM_RECORD_ENGINE_IF_H_ */
