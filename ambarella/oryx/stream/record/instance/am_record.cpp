/*******************************************************************************
 * am_record.cpp
 *
 * History:
 *   2014-12-31 - [ypchang] created file
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

#include "am_record.h"

#include "am_amf_types.h"
#include "am_amf_interface.h"
#include "am_record_engine_if.h"

AMRecord *AMRecord::m_instance = nullptr;
AMMemLock AMRecord::m_lock;

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
  AUTO_MEM_LOCK(m_engine_lock);
  return (m_engine && m_engine->record());
}

bool AMRecord::stop()
{
  AUTO_MEM_LOCK(m_engine_lock);
  return (m_engine && m_engine->stop());
}

bool AMRecord::start_file_muxer()
{
  AUTO_MEM_LOCK(m_engine_lock);
  return (m_engine && m_engine->start_file_muxer());
}

bool AMRecord::start_file_recording(uint32_t muxer_id_bit_map)
{
  AUTO_MEM_LOCK(m_engine_lock);
  return (m_engine && m_engine->start_file_recording(muxer_id_bit_map));
}

bool AMRecord::stop_file_recording(uint32_t muxer_id_bit_map)
{
  AUTO_MEM_LOCK(m_engine_lock);
  return (m_engine && m_engine->stop_file_recording(muxer_id_bit_map));
}

bool AMRecord::set_recording_file_num(uint32_t muxer_id_bit_map, uint32_t file_num)
{
  AUTO_MEM_LOCK(m_engine_lock);
  return (m_engine && m_engine->set_recording_file_num(muxer_id_bit_map, file_num));
}

bool AMRecord::set_recording_duration(uint32_t muxer_id_bit_map, int32_t duration)
{
  AUTO_MEM_LOCK(m_engine_lock);
  return (m_engine && m_engine->set_recording_duration(muxer_id_bit_map, duration));
}

bool AMRecord::set_file_duration(uint32_t muxer_id_bit_map, int32_t file_duration,
                                 bool apply_conf_file)
{
  AUTO_MEM_LOCK(m_engine_lock);
  return (m_engine && m_engine->set_file_duration(muxer_id_bit_map, file_duration,
                                                  apply_conf_file));
}

bool AMRecord::set_muxer_param(AMMuxerParam &param)
{
  AUTO_MEM_LOCK(m_engine_lock);
  return (m_engine && m_engine->set_muxer_param(param));
}

bool AMRecord::is_recording()
{
  AUTO_MEM_LOCK(m_engine_lock);
  return (m_engine && (m_engine->get_engine_status() ==
      AMIRecordEngine::AM_RECORD_ENGINE_RECORDING));
}

bool AMRecord::enable_audio_codec(AM_AUDIO_TYPE type, uint32_t sample_rate,
                            bool enable)
{
  AUTO_MEM_LOCK(m_engine_lock);
  return (m_engine && (m_engine->enable_audio_codec(type, sample_rate, enable)));
}

bool AMRecord::set_file_operation_callback(uint32_t muxer_id_bit_map,
                                           AM_FILE_OPERATION_CB_TYPE type,
                                           AMFileOperationCB callback)
{
  return (m_engine && (m_engine->set_file_operation_callback(muxer_id_bit_map,
                                                             type, callback)));
}

bool AMRecord::update_image_3A_info(AMImage3AInfo *image_3A)
{
  return (m_engine && (m_engine->update_image_3A_info(image_3A)));
}

void AMRecord::set_aec_enabled(bool enabled)
{
  AUTO_MEM_LOCK(m_engine_lock);
  if (AM_LIKELY(m_engine)) {
    m_engine->set_aec_enabled(enabled);
  }
}

bool AMRecord::is_ready_for_event(AMEventStruct& event)
{
  AUTO_MEM_LOCK(m_engine_lock);
  return (m_engine && (m_engine->is_ready_for_event(event)));
}

bool AMRecord::send_event(AMEventStruct& event)
{
  AUTO_MEM_LOCK(m_engine_lock);
  return (m_engine && m_engine->send_event(event));
}

bool AMRecord::init()
{
  AUTO_MEM_LOCK(m_engine_lock);
  if (AM_LIKELY(!m_engine)) {
    m_engine = AMIRecordEngine::create();
  }

  if (AM_LIKELY(!m_is_initialized)) {
    m_is_initialized = m_engine && m_engine->create_graph();
  }

  return m_is_initialized;
}

bool AMRecord::reload()
{
  AUTO_MEM_LOCK(m_engine_lock);
  AM_DESTROY(m_engine);
  m_is_initialized = false;
  m_engine = AMIRecordEngine::create();
  m_is_initialized = m_engine && m_engine->create_graph();

  return m_is_initialized;
}

void AMRecord::set_msg_callback(AMRecordCallback callback, void *data)
{
  AUTO_MEM_LOCK(m_engine_lock);
  if (AM_LIKELY(m_engine)) {
    m_engine->set_app_msg_callback(callback, data);
  }
}

void AMRecord::release()
{
  AUTO_MEM_LOCK(m_ref_lock);
  if ((m_ref_count >= 0) && (--m_ref_count <= 0)) {
    NOTICE("Last reference of AMRecord's object %p, release it!", m_instance);
    delete m_instance;
    m_instance = nullptr;
  }
}

void AMRecord::inc_ref()
{
  AUTO_MEM_LOCK(m_ref_lock);
  ++ m_ref_count;
}

AMRecord::AMRecord()
{}

AMRecord::~AMRecord()
{
  AM_DESTROY(m_engine);
  DEBUG("~AMRecord");
}
