/*******************************************************************************
 * am_playback.cpp
 *
 * History:
 *   2014-7-25 - [ypchang] created file
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

#include "am_playback.h"

#include "am_amf_types.h"
#include "am_amf_interface.h"
#include "am_playback_engine_if.h"

static std::mutex g_lock;

AMIPlaybackPtr AMIPlayback::create()
{
  return AMPlayback::get_instance();
}

AMIPlayback* AMPlayback::get_instance()
{
  AMIPlayback *instance = nullptr;
  g_lock.lock();
  instance = new AMPlayback();
  if (AM_UNLIKELY(!instance)) {
    ERROR("Failed to create instance of AMPlayback!");
  }
  g_lock.unlock();

  return instance;
}

bool AMPlayback::init()
{
  if (AM_LIKELY(!m_engine)) {
    m_engine = AMIPlaybackEngine::create();
  }

  if (AM_LIKELY(!m_is_initialized)) {
    m_is_initialized = m_engine && m_engine->create_graph();
  }

  return m_is_initialized;
}

bool AMPlayback::reload()
{
  AM_DESTROY(m_engine);
  m_is_initialized = false;
  return init();
}

void AMPlayback::set_msg_callback(AMPlaybackCallback callback, void *data)
{
  if (AM_LIKELY(m_engine)) {
    m_engine->set_app_msg_callback(callback, data);
  }
}

bool AMPlayback::is_playing()
{
  return (m_engine && (m_engine->get_engine_status() ==
      AMIPlaybackEngine::AM_PLAYBACK_ENGINE_PLAYING));
}

bool AMPlayback::is_paused()
{
  return (m_engine && (m_engine->get_engine_status() ==
      AMIPlaybackEngine::AM_PLAYBACK_ENGINE_PAUSED));
}

bool AMPlayback::add_uri(const AMPlaybackUri& uri)
{
  return (m_engine && m_engine->add_uri(uri));
}

bool AMPlayback::play(AMPlaybackUri *uri)
{
  return (m_engine && m_engine->play((uri != NULL)? uri : NULL));
}

bool AMPlayback::pause(bool enable)
{
  return (m_engine && m_engine->pause(enable));
}

bool AMPlayback::stop()
{
  return (m_engine && m_engine->stop());
}

void AMPlayback::release()
{
  if (AM_LIKELY((m_ref_count >= 0) && (--m_ref_count <= 0))) {
    INFO("Last reference of AMPlayback's object %p, delete it!", this);
    delete this;
  }
}

void AMPlayback::inc_ref()
{
  ++ m_ref_count;
}

AMPlayback::AMPlayback() :
  m_engine(NULL),
  m_is_initialized(false),
  m_ref_count(0)
{
}

AMPlayback::~AMPlayback()
{
  AM_DESTROY(m_engine);
  DEBUG("~AMPlayback");
}
