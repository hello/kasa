/*******************************************************************************
 * am_record.h
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
#ifndef AM_RECORD_H_
#define AM_RECORD_H_

#include "am_record_if.h"
#include <atomic>
#include <mutex>

class AMIRecordEngine;
class AMRecord: public AMIRecord
{
  public:
    static AMIRecord* get_instance();

  public:
    bool start();
    bool stop();
    bool start_file_recording();
    bool stop_file_recording();
    bool is_recording();

  public:
    bool init();
    void set_msg_callback(AMRecordCallback callback, void *data);
    bool send_event();
    bool is_ready_for_event();

  protected:
    virtual void release();
    virtual void inc_ref();

  private:
    explicit AMRecord();
    virtual ~AMRecord();

  private:
    AMIRecordEngine   *m_engine;
    bool               m_is_initialized;
    std::atomic_int    m_ref_count;

  private:
    static AMRecord   *m_instance;
    static std::mutex  m_lock;
};

#endif /* AM_RECORD_H_ */
