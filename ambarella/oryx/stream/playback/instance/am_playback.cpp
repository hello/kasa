/*******************************************************************************
 * am_playback.cpp
 *
 * History:
 *   2014-7-25 - [ypchang] created file
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

#include "am_base_include.h"
#include "am_define.h"
#include "am_log.h"

#include "am_playback.h"

#include "am_amf_types.h"
#include "am_amf_interface.h"
#include "am_playback_engine_if.h"

AMIPlaybackPtr AMIPlayback::create()
{
  return AMPlayback::get_instance();
}

AMIPlayback* AMPlayback::get_instance()
{
  AMIPlayback *instance = new AMPlayback();
  if (AM_UNLIKELY(!instance)) {
    ERROR("Failed to create instance of AMPlayback!");
  }

  return instance;
}

bool AMPlayback::init()
{
  AUTO_MEM_LOCK(m_engine_lock);
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
  AUTO_MEM_LOCK(m_engine_lock);
  AM_DESTROY(m_engine);
  m_is_initialized = false;
  m_engine = AMIPlaybackEngine::create();
  m_is_initialized = m_engine && m_engine->create_graph();

  return m_is_initialized;
}

void AMPlayback::set_msg_callback(AMPlaybackCallback callback, void *data)
{
  AUTO_MEM_LOCK(m_engine_lock);
  if (AM_LIKELY(m_engine)) {
    m_engine->set_app_msg_callback(callback, data);
  }
}

void AMPlayback::set_aec_enabled(bool enabled)
{
  AUTO_MEM_LOCK(m_engine_lock);
  if (AM_LIKELY(m_engine)) {
    m_engine->set_aec_enabled(enabled);
  }
}

bool AMPlayback::is_playing()
{
  AUTO_MEM_LOCK(m_engine_lock);
  return (m_engine && (m_engine->get_engine_status() ==
      AMIPlaybackEngine::AM_PLAYBACK_ENGINE_PLAYING));
}

bool AMPlayback::is_paused()
{
  AUTO_MEM_LOCK(m_engine_lock);
  return (m_engine && (m_engine->get_engine_status() ==
      AMIPlaybackEngine::AM_PLAYBACK_ENGINE_PAUSED));
}

bool AMPlayback::is_idle()
{
  AUTO_MEM_LOCK(m_engine_lock);
  return (m_engine && (m_engine->get_engine_status() ==
      AMIPlaybackEngine::AM_PLAYBACK_ENGINE_IDLE));
}

bool AMPlayback::add_uri(const AMPlaybackUri& uri)
{
  AUTO_MEM_LOCK(m_engine_lock);
  return (m_engine && m_engine->add_uri(uri));
}

bool AMPlayback::play(AMPlaybackUri *uri)
{
  AUTO_MEM_LOCK(m_engine_lock);
  return (m_engine && m_engine->play((uri != NULL)? uri : NULL));
}

bool AMPlayback::pause(bool enable)
{
  AUTO_MEM_LOCK(m_engine_lock);
  return (m_engine && m_engine->pause(enable));
}

bool AMPlayback::stop()
{
  AUTO_MEM_LOCK(m_engine_lock);
  return (m_engine && m_engine->stop());
}

void AMPlayback::release()
{
  AUTO_MEM_LOCK(m_ref_lock);
  if (AM_LIKELY((m_ref_count >= 0) && (--m_ref_count <= 0))) {
    NOTICE("Last reference of AMPlayback's object %p, delete it!", this);
    delete this;
  }
}

void AMPlayback::inc_ref()
{
  AUTO_MEM_LOCK(m_ref_lock);
  ++ m_ref_count;
}

AMPlayback::AMPlayback()
{
}

AMPlayback::~AMPlayback()
{
  AM_DESTROY(m_engine);
  DEBUG("~AMPlayback");
}
