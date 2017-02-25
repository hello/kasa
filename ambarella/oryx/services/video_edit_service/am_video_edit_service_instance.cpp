/*******************************************************************************
 * am_playback_service_instance.cpp
 *
 * History:
 *   2016-04-14 - [Zhi He] created file
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

#include <mutex>

#include "am_log.h"
#include "am_service_frame_if.h"
#include "commands/am_api_cmd_common.h"
#include "am_api_video_edit.h"

#include "am_playback_new_if.h"
#include "am_video_edit_service_instance.h"

extern AMIPCSyncCmdServer g_ipc;

static int __post_video_edit_eos_app_event()
{
  int cmd_result = 0;
  am_service_notify_payload payload;
  am_video_edit_msg_t msg;

  memset(&payload, 0, sizeof(am_service_notify_payload));
  SET_BIT(payload.dest_bits, AM_SERVICE_TYPE_GENERIC);
  payload.msg_id = AM_IPC_MW_CMD_VIDEO_EDIT_FINISH_TO_APP;
  payload.type = AM_VIDEO_EDIT_SERVICE_NOTIFY;
  msg.instance_id = 0;
  msg.msg_type = VE_APP_MSG_FINISH;
  payload.data_size = sizeof(msg);
  memcpy((void *)payload.data, &msg, sizeof(msg));

  cmd_result = g_ipc.notify(AM_IPC_SERVICE_NOTIF, &payload,
                            payload.header_size() + payload.data_size);
  if (cmd_result != AM_IPC_CMD_SUCCESS) {
    ERROR("post video edit finish event to app failed");
  }

  return cmd_result;
}

static void __video_edit_instance_app_msg_callback
  (void *context, SMSG &msg)
{
  switch (msg.code) {
    case EMSGType_VideoEditEOS: {
        printf("get video edit eos, post it to app\n");
        __post_video_edit_eos_app_event();
      }
      break;
    default:
      break;
  }
}

AMVideoEditServiceInstance::AMVideoEditServiceInstance(int index)
  : m_engine_control(NULL)
  , m_index(index)
  , m_state(AM_VIDEO_EDIT_SERVICE_INSTANCE_STATE_IDLE)
{
  m_source_id = 0;
  m_video_decoder_id = 0;
  m_efm_injecter_id = 0;
  m_video_encoder_id = 0;
  m_sink_id = 0;
}

AMVideoEditServiceInstance::~AMVideoEditServiceInstance()
{
  clear();
}

int AMVideoEditServiceInstance::es_2_es(char *input_url,
                                        char *output_url,
                                        uint8_t stream_index)
{
  std::lock_guard<std::mutex> autolock(m_mtx);
  if ((!input_url) || (!output_url)) {
    return AM_VIDEO_EDIT_SERVICE_RETCODE_INVALID_PARAMETERS;
  }
  if ((!m_engine_control)
    && (AM_VIDEO_EDIT_SERVICE_INSTANCE_STATE_IDLE == m_state)) {
    EECode err = EECode_OK;
    TGenericID connection_id = 0;
    m_engine_control = CreateGenericMediaEngine();
    if (!m_engine_control) {
      return AM_VIDEO_EDIT_SERVICE_RETCODE_NO_RESOURCE;
    }
    m_engine_control->set_app_msg_callback(
      __video_edit_instance_app_msg_callback, (void *) this);
    err = m_engine_control->begin_config_process_pipeline();
    err = m_engine_control->new_component(
      EGenericComponentType_Demuxer,
      m_source_id, DNameTag_PrivateVideoES);
    err = m_engine_control->new_component(
      EGenericComponentType_VideoDecoder,
      m_video_decoder_id, DNameTag_FFMpeg);
    err = m_engine_control->new_component(
      EGenericComponentType_VideoInjecter,
      m_efm_injecter_id, DNameTag_AMBAEFM);
    err = m_engine_control->new_component(
      EGenericComponentType_VideoEncoder,
      m_video_encoder_id, DNameTag_AMBA);
    err = m_engine_control->new_component(
      EGenericComponentType_VideoESSink, m_sink_id, "");
    err = m_engine_control->connect_component(
      connection_id, m_source_id, m_video_decoder_id,
      StreamType_Video);
    err = m_engine_control->connect_component(
      connection_id, m_video_decoder_id, m_efm_injecter_id,
      StreamType_Video);
    err = m_engine_control->connect_component(
      connection_id, m_video_encoder_id, m_sink_id,
      StreamType_Video);
    err = m_engine_control->finalize_config_process_pipeline();
    if (EECode_OK != err) {
      printf("finalize_config_process_pipeline fail\n");
      return AM_VIDEO_EDIT_SERVICE_RETCODE_ACTION_FAIL;
    }
    SConfigVideoStreamID config_stream_id;
    config_stream_id.check_field = EGenericEngineConfigure_VideoStreamID;
    config_stream_id.stream_id = 1 << stream_index;
    config_stream_id.component_id = m_efm_injecter_id;
    err = m_engine_control->generic_control(EGenericEngineConfigure_VideoStreamID, &config_stream_id);
    config_stream_id.component_id = m_video_encoder_id;
    err = m_engine_control->generic_control(EGenericEngineConfigure_VideoStreamID, &config_stream_id);
    err = m_engine_control->set_source_url(m_source_id, input_url);
    if (EECode_OK != err) {
      printf("set_source_url fail\n");
      return AM_VIDEO_EDIT_SERVICE_RETCODE_ACTION_FAIL;
    }
    err = m_engine_control->set_sink_url(m_sink_id, output_url);
    if (EECode_OK != err) {
      printf("set_sink_url fail\n");
      return AM_VIDEO_EDIT_SERVICE_RETCODE_ACTION_FAIL;
    }
    err = m_engine_control->run_processing();
    if (EECode_OK != err) {
      printf("run_processing fail\n");
      return AM_VIDEO_EDIT_SERVICE_RETCODE_ACTION_FAIL;
    }
    err = m_engine_control->start();
    if (EECode_OK != err) {
      printf("start fail\n");
      return AM_VIDEO_EDIT_SERVICE_RETCODE_ACTION_FAIL;
    }
    m_state = AM_VIDEO_EDIT_SERVICE_INSTANCE_STATE_RUNING;
    return AM_VIDEO_EDIT_SERVICE_RETCODE_OK;
  }
  return AM_VIDEO_EDIT_SERVICE_RETCODE_INSTANCE_BUSY;
}

int AMVideoEditServiceInstance::es_2_mp4(char *input_url, char *output_url, uint8_t stream_index)
{
  std::lock_guard<std::mutex> autolock(m_mtx);
  if ((!input_url) || (!output_url)) {
    return AM_VIDEO_EDIT_SERVICE_RETCODE_INVALID_PARAMETERS;
  }
  if ((!m_engine_control)
    && (AM_VIDEO_EDIT_SERVICE_INSTANCE_STATE_IDLE == m_state)) {
    EECode err = EECode_OK;
    TGenericID connection_id = 0;
    m_engine_control = CreateGenericMediaEngine();
    if (!m_engine_control) {
      return AM_VIDEO_EDIT_SERVICE_RETCODE_NO_RESOURCE;
    }
    m_engine_control->set_app_msg_callback(
      __video_edit_instance_app_msg_callback, (void *) this);
    err = m_engine_control->begin_config_process_pipeline();
    err = m_engine_control->new_component(
      EGenericComponentType_Demuxer, m_source_id,
      DNameTag_PrivateVideoES);
    err = m_engine_control->new_component(
      EGenericComponentType_VideoDecoder, m_video_decoder_id,
      DNameTag_FFMpeg);
    err = m_engine_control->new_component(
      EGenericComponentType_VideoInjecter, m_efm_injecter_id,
      DNameTag_AMBAEFM);
    err = m_engine_control->new_component(
      EGenericComponentType_VideoEncoder, m_video_encoder_id,
      DNameTag_AMBA);
    err = m_engine_control->new_component(
      EGenericComponentType_Muxer, m_sink_id,
      DNameTag_PrivateMP4);
    err = m_engine_control->connect_component(
      connection_id, m_source_id, m_video_decoder_id,
      StreamType_Video);
    err = m_engine_control->connect_component(
      connection_id, m_video_decoder_id, m_efm_injecter_id,
      StreamType_Video);
    err = m_engine_control->connect_component(
      connection_id, m_video_encoder_id, m_sink_id,
      StreamType_Video);
    err = m_engine_control->finalize_config_process_pipeline();
    if (EECode_OK != err) {
      printf("finalize_config_process_pipeline fail\n");
      return AM_VIDEO_EDIT_SERVICE_RETCODE_ACTION_FAIL;
    }
    SConfigVideoStreamID config_stream_id;
    config_stream_id.check_field = EGenericEngineConfigure_VideoStreamID;
    config_stream_id.stream_id = 1 << stream_index;
    config_stream_id.component_id = m_efm_injecter_id;
    err = m_engine_control->generic_control(EGenericEngineConfigure_VideoStreamID, &config_stream_id);
    config_stream_id.component_id = m_video_encoder_id;
    err = m_engine_control->generic_control(EGenericEngineConfigure_VideoStreamID, &config_stream_id);
    err = m_engine_control->set_source_url(m_source_id, input_url);
    if (EECode_OK != err) {
      printf("set_source_url fail\n");
      return AM_VIDEO_EDIT_SERVICE_RETCODE_ACTION_FAIL;
    }
    err = m_engine_control->set_sink_url(m_sink_id, output_url);
    if (EECode_OK != err) {
      printf("set_sink_url fail\n");
      return AM_VIDEO_EDIT_SERVICE_RETCODE_ACTION_FAIL;
    }
    err = m_engine_control->run_processing();
    if (EECode_OK != err) {
      printf("run_processing fail\n");
      return AM_VIDEO_EDIT_SERVICE_RETCODE_ACTION_FAIL;
    }
    err = m_engine_control->start();
    if (EECode_OK != err) {
      printf("start fail\n");
      return AM_VIDEO_EDIT_SERVICE_RETCODE_ACTION_FAIL;
    }
    m_state = AM_VIDEO_EDIT_SERVICE_INSTANCE_STATE_RUNING;
    return AM_VIDEO_EDIT_SERVICE_RETCODE_OK;
  }
  return AM_VIDEO_EDIT_SERVICE_RETCODE_INSTANCE_BUSY;
}

int AMVideoEditServiceInstance::feed_efm(char *input_url, uint8_t stream_index)
{
  std::lock_guard<std::mutex> autolock(m_mtx);
  if (!input_url) {
    return AM_VIDEO_EDIT_SERVICE_RETCODE_INVALID_PARAMETERS;
  }
  if ((!m_engine_control)
    && (AM_VIDEO_EDIT_SERVICE_INSTANCE_STATE_IDLE == m_state)) {
    EECode err = EECode_OK;
    TGenericID connection_id = 0;
    m_engine_control = CreateGenericMediaEngine();
    if (!m_engine_control) {
      return AM_VIDEO_EDIT_SERVICE_RETCODE_NO_RESOURCE;
    }
    m_engine_control->set_app_msg_callback(
      __video_edit_instance_app_msg_callback, (void *) this);
    err = m_engine_control->begin_config_process_pipeline();
    err = m_engine_control->new_component(
      EGenericComponentType_Demuxer, m_source_id,
      DNameTag_PrivateVideoES);
    err = m_engine_control->new_component(
      EGenericComponentType_VideoDecoder, m_video_decoder_id,
      DNameTag_FFMpeg);
    err = m_engine_control->new_component(
      EGenericComponentType_VideoInjecter, m_efm_injecter_id,
      DNameTag_AMBAEFM);
    err = m_engine_control->connect_component(
      connection_id, m_source_id, m_video_decoder_id,
      StreamType_Video);
    err = m_engine_control->connect_component(
      connection_id, m_video_decoder_id, m_efm_injecter_id,
      StreamType_Video);
    err = m_engine_control->finalize_config_process_pipeline();
    if (EECode_OK != err) {
      printf("finalize_config_process_pipeline fail\n");
      return AM_VIDEO_EDIT_SERVICE_RETCODE_ACTION_FAIL;
    }
    SConfigVideoStreamID config_stream_id;
    config_stream_id.check_field = EGenericEngineConfigure_VideoStreamID;
    config_stream_id.stream_id = 1 << stream_index;
    config_stream_id.component_id = m_efm_injecter_id;
    err = m_engine_control->generic_control(EGenericEngineConfigure_VideoStreamID, &config_stream_id);
    config_stream_id.component_id = m_video_encoder_id;
    err = m_engine_control->generic_control(EGenericEngineConfigure_VideoStreamID, &config_stream_id);
    err = m_engine_control->set_source_url(m_source_id, input_url);
    if (EECode_OK != err) {
      printf("set_source_url fail\n");
      return AM_VIDEO_EDIT_SERVICE_RETCODE_ACTION_FAIL;
    }
    err = m_engine_control->run_processing();
    if (EECode_OK != err) {
      printf("run_processing fail\n");
      return AM_VIDEO_EDIT_SERVICE_RETCODE_ACTION_FAIL;
    }
    err = m_engine_control->start();
    if (EECode_OK != err) {
      printf("start fail\n");
      return AM_VIDEO_EDIT_SERVICE_RETCODE_ACTION_FAIL;
    }
    m_state = AM_VIDEO_EDIT_SERVICE_INSTANCE_STATE_RUNING;
    return AM_VIDEO_EDIT_SERVICE_RETCODE_OK;
  }
  return AM_VIDEO_EDIT_SERVICE_RETCODE_INSTANCE_BUSY;
}

int AMVideoEditServiceInstance::end_processing()
{
  std::lock_guard<std::mutex> autolock(m_mtx);
  if (m_engine_control) {
    if (AM_VIDEO_EDIT_SERVICE_INSTANCE_STATE_IDLE != m_state) {
      m_engine_control->stop();
      m_engine_control->exit_processing();
      m_engine_control->destroy();
      m_engine_control = NULL;
    }
    clear();
    m_state = AM_VIDEO_EDIT_SERVICE_INSTANCE_STATE_IDLE;
    return AM_VIDEO_EDIT_SERVICE_RETCODE_OK;
  }
  return AM_VIDEO_EDIT_SERVICE_RETCODE_OK;
}

void AMVideoEditServiceInstance::clear()
{
  if (m_engine_control) {
    m_engine_control->destroy();
    m_engine_control = NULL;
  }
  m_state = AM_VIDEO_EDIT_SERVICE_INSTANCE_STATE_IDLE;
  m_source_id = 0;
  m_video_decoder_id = 0;
  m_efm_injecter_id = 0;
  m_video_encoder_id = 0;
  m_sink_id = 0;
}

