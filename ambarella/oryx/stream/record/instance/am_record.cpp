/*******************************************************************************
 * am_record.cpp
 *
 * History:
 *   2014-12-31 - [ypchang] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/

#include "am_base_include.h"
#include "am_define.h"
#include "am_log.h"

#include "am_record.h"

#include "am_amf_types.h"
#include "am_amf_interface.h"
#include "am_record_engine_if.h"

AMRecord *AMRecord::m_instance = nullptr;
std::mutex AMRecord::m_lock;

AMIRecordPtr AMIRecord::create()
{
  return AMRecord::get_instance();
}

AMIRecord* AMRecord::get_instance()
{
  m_lock.lock();

  if (AM_LIKELY(!m_instance)) {
    m_instance = new AMRecord();
    if (AM_UNLIKELY(!m_instance)) {
      ERROR("Failed to create instance of AMRecord!");
    }
  }

  m_lock.unlock();

  return m_instance;
}

bool AMRecord::start()
{
  return (m_engine && m_engine->record());
}

bool AMRecord::stop()
{
  return (m_engine && m_engine->stop());
}

bool AMRecord::start_file_recording()
{
  INFO("start file recording");
  return (m_engine && m_engine->start_file_recording());
}

bool AMRecord::stop_file_recording()
{
  INFO("stop file recording.");
  return (m_engine && m_engine->stop_file_recording());
}

bool AMRecord::is_recording()
{
  return (m_engine && (m_engine->get_engine_status() ==
      AMIRecordEngine::AM_RECORD_ENGINE_RECORDING));
}

bool AMRecord::is_ready_for_event()
{
  return (m_engine && (m_engine->is_ready_for_event()));
}

bool AMRecord::send_event()
{
  return m_engine->send_event();
}

bool AMRecord::init()
{
  if (AM_LIKELY(!m_engine)) {
    m_engine = AMIRecordEngine::create();
  }

  if (AM_LIKELY(!m_is_initialized)) {
    m_is_initialized = m_engine && m_engine->create_graph();
  }

  return m_is_initialized;
}

void AMRecord::set_msg_callback(AMRecordCallback callback, void *data)
{
  if (AM_LIKELY(m_engine)) {
    m_engine->set_app_msg_callback(callback, data);
  }
}

void AMRecord::release()
{
  if ((m_ref_count >= 0) && (--m_ref_count <= 0)) {
    NOTICE("Last reference of AMRecord's object %p, release it!", m_instance);
    delete m_instance;
    m_instance = nullptr;
    m_ref_count = 0;
  }
}

void AMRecord::inc_ref()
{
  ++ m_ref_count;
}

AMRecord::AMRecord() :
    m_engine(NULL),
    m_is_initialized(false),
    m_ref_count(0)
{}

AMRecord::~AMRecord()
{
  AM_DESTROY(m_engine);
  DEBUG("~AMRecord");
}
