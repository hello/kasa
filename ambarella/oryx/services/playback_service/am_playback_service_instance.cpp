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

#include <mutex>

#include "am_api_playback.h"

#include "am_playback_new_if.h"
#include "am_playback_service_instance.h"

AMPlaybackServiceInstance::AMPlaybackServiceInstance(int index)
  : m_engine_control(nullptr)
  , m_index(index)
  , m_state(AM_PLAYBACK_SERVICE_INSTANCE_STATE_IDLE)
{
  m_demuxer_id = 0;
  m_video_decoder_id = 0;
  m_audio_decoder_id = 0;
  m_video_renderer_id = 0;
  m_audio_renderer_id = 0;
  m_playback_pipeline_id = 0;

  m_is_live_streaming = 0;
  m_has_decode_error = 0;
  m_playback_complete = 0;
  m_trick_mode = AM_PLAYBACK_SERVICE_INSTANCE_TRICKMODE_RUN;

  m_speed = 0x0100;
  m_direction = 0;
  m_scan_mode = 0;
}

AMPlaybackServiceInstance::~AMPlaybackServiceInstance()
{
  clear();
}

int AMPlaybackServiceInstance::play(char *url, int enable_audio, char *prefer_demuxer,
  char *prefer_video_decoder, char *prefer_video_output,
  char *prefer_audio_decoder, char *prefer_audio_output,
  unsigned char use_hdmi, unsigned char use_digital, unsigned char use_cvbs, unsigned char rtsp_tcp)
{
  std::lock_guard<std::mutex> autolock(m_mtx);
  TGenericID connection_id = 0;
  int is_rtsp = 0;
  if (!url) {
    return AM_PLAYBACK_SERVICE_RETCODE_INVALID_PARAMETERS;
  }
  if ((!m_engine_control) && (AM_PLAYBACK_SERVICE_INSTANCE_STATE_IDLE == m_state)) {
    EECode err = EECode_OK;
    m_engine_control = CreateGenericMediaEngine();
    if (!m_engine_control) {
      return AM_PLAYBACK_SERVICE_RETCODE_NO_RESOURCE;
    }
    if (!strncmp(url, "rtsp://", strlen("rtsp://"))) {
      m_is_live_streaming = 1;
      is_rtsp = 1;
    }
    err = m_engine_control->begin_config_process_pipeline();
    if (is_rtsp) {
      err = m_engine_control->new_component(
        EGenericComponentType_Demuxer, m_demuxer_id, "RTSP");
    } else {
      err = m_engine_control->new_component(
        EGenericComponentType_Demuxer, m_demuxer_id, (const char *) prefer_demuxer);
    }
    err = m_engine_control->new_component(
      EGenericComponentType_VideoDecoder, m_video_decoder_id, prefer_video_decoder);
    err = m_engine_control->new_component(
      EGenericComponentType_VideoRenderer, m_video_renderer_id, prefer_video_output);
    if (enable_audio) {
      err = m_engine_control->new_component(
        EGenericComponentType_AudioDecoder, m_audio_decoder_id, prefer_audio_decoder);
      err = m_engine_control->new_component(
        EGenericComponentType_AudioRenderer, m_audio_renderer_id, prefer_audio_output);
    } else {
      m_audio_decoder_id = 0;
      m_audio_renderer_id = 0;
    }
    err = m_engine_control->connect_component(
      connection_id, m_demuxer_id, m_video_decoder_id,  StreamType_Video);
    err = m_engine_control->connect_component(
      connection_id, m_video_decoder_id, m_video_renderer_id,  StreamType_Video);
    if (enable_audio) {
      err = m_engine_control->connect_component(
        connection_id, m_demuxer_id, m_audio_decoder_id,  StreamType_Audio);
      err = m_engine_control->connect_component(
        connection_id, m_audio_decoder_id, m_audio_renderer_id,  StreamType_Audio);
    }
    err = m_engine_control->setup_playback_pipeline(
      m_playback_pipeline_id, m_demuxer_id, m_demuxer_id,
      m_video_decoder_id, m_audio_decoder_id,
      m_video_renderer_id, m_audio_renderer_id);
    SConfigVout config_vout;
    config_vout.check_field = EGenericEngineConfigure_ConfigVout;
    config_vout.b_digital_vout = use_digital;
    config_vout.b_hdmi_vout = use_hdmi;
    config_vout.b_cvbs_vout = use_cvbs;
    err = m_engine_control->generic_control(
      EGenericEngineConfigure_ConfigVout, &config_vout);
    if (is_rtsp && rtsp_tcp) {
      err = m_engine_control->generic_control(
        EGenericEngineConfigure_RTSPClientTryTCPModeFirst, nullptr);
    }
    err = m_engine_control->generic_control(
      EGenericEngineConfigure_EnableFastFWFastBWBackwardPlayback, nullptr);
    err = m_engine_control->finalize_config_process_pipeline();
    if (EECode_OK != err) {
      return AM_PLAYBACK_SERVICE_RETCODE_ACTION_FAIL;
    }
    err = m_engine_control->set_source_url(m_demuxer_id, url);
    if (EECode_OK != err) {
      return AM_PLAYBACK_SERVICE_RETCODE_ACTION_FAIL;
    }
    err = m_engine_control->run_processing();
    if (EECode_OK != err) {
      return AM_PLAYBACK_SERVICE_RETCODE_ACTION_FAIL;
    }
    err = m_engine_control->start();
    if (EECode_OK != err) {
      return AM_PLAYBACK_SERVICE_RETCODE_ACTION_FAIL;
    }
    m_state = AM_PLAYBACK_SERVICE_INSTANCE_STATE_RUNING;
    return AM_PLAYBACK_SERVICE_RETCODE_OK;
  }
  return AM_PLAYBACK_SERVICE_RETCODE_BAD_STATE;
}

int AMPlaybackServiceInstance::end_play()
{
  std::lock_guard<std::mutex> autolock(m_mtx);
  if (m_engine_control) {
    if (AM_PLAYBACK_SERVICE_INSTANCE_STATE_IDLE != m_state) {
      m_engine_control->stop();
      m_engine_control->exit_processing();
      m_engine_control->destroy();
      m_engine_control = nullptr;
    }
    clear();
    m_state = AM_PLAYBACK_SERVICE_INSTANCE_STATE_IDLE;
    return AM_PLAYBACK_SERVICE_RETCODE_OK;
  }
  return AM_PLAYBACK_SERVICE_RETCODE_BAD_STATE;
}

int AMPlaybackServiceInstance::pause()
{
  std::lock_guard<std::mutex> autolock(m_mtx);
  if (m_engine_control && (AM_PLAYBACK_SERVICE_INSTANCE_STATE_RUNING == m_state)) {
    if (AM_PLAYBACK_SERVICE_INSTANCE_TRICKMODE_RUN == m_trick_mode) {
      SUserParamPause pause;
      EECode err = EECode_OK;
      pause.check_field = EGenericEngineConfigure_Pause;
      pause.component_id = m_playback_pipeline_id;
      err = m_engine_control->generic_control(EGenericEngineConfigure_Pause, &pause);
      if (EECode_OK == err) {
        m_trick_mode = AM_PLAYBACK_SERVICE_INSTANCE_TRICKMODE_PAUSED;
        return AM_PLAYBACK_SERVICE_RETCODE_OK;
      }
      return AM_PLAYBACK_SERVICE_RETCODE_ACTION_FAIL;
    }
    return AM_PLAYBACK_SERVICE_RETCODE_BAD_STATE;
  }
  return AM_PLAYBACK_SERVICE_RETCODE_BAD_STATE;
}

int AMPlaybackServiceInstance::resume()
{
  std::lock_guard<std::mutex> autolock(m_mtx);
  if (m_engine_control && (AM_PLAYBACK_SERVICE_INSTANCE_STATE_RUNING == m_state)) {
    if ((AM_PLAYBACK_SERVICE_INSTANCE_TRICKMODE_PAUSED == m_trick_mode)
      || (AM_PLAYBACK_SERVICE_INSTANCE_TRICKMODE_STEP == m_trick_mode)) {
      SUserParamResume resume;
      EECode err = EECode_OK;
      resume.check_field = EGenericEngineConfigure_Resume;
      resume.component_id = m_playback_pipeline_id;
      err = m_engine_control->generic_control(EGenericEngineConfigure_Resume, &resume);
      if (EECode_OK == err) {
        m_trick_mode = AM_PLAYBACK_SERVICE_INSTANCE_TRICKMODE_RUN;
        return AM_PLAYBACK_SERVICE_RETCODE_OK;
      }
      return AM_PLAYBACK_SERVICE_RETCODE_ACTION_FAIL;
    }
    return AM_PLAYBACK_SERVICE_RETCODE_BAD_STATE;
  }
  return AM_PLAYBACK_SERVICE_RETCODE_BAD_STATE;
}

int AMPlaybackServiceInstance::step()
{
  std::lock_guard<std::mutex> autolock(m_mtx);
  if (m_engine_control && (AM_PLAYBACK_SERVICE_INSTANCE_STATE_RUNING == m_state)) {
    SUserParamStepPlay step;
    EECode err = EECode_OK;
    step.check_field = EGenericEngineConfigure_StepPlay;
    step.component_id = m_playback_pipeline_id;
    err = m_engine_control->generic_control(EGenericEngineConfigure_StepPlay, &step);
    if (EECode_OK == err) {
      m_trick_mode = AM_PLAYBACK_SERVICE_INSTANCE_TRICKMODE_STEP;
      return AM_PLAYBACK_SERVICE_RETCODE_OK;
    }
    return AM_PLAYBACK_SERVICE_RETCODE_ACTION_FAIL;
  }
  return AM_PLAYBACK_SERVICE_RETCODE_BAD_STATE;
}

int AMPlaybackServiceInstance::seek(unsigned long long target)
{
  std::lock_guard<std::mutex> autolock(m_mtx);
  if (m_engine_control && (AM_PLAYBACK_SERVICE_INSTANCE_STATE_RUNING == m_state)) {
    SPbSeek seek;
    EECode err = EECode_OK;
    seek.check_field = EGenericEngineConfigure_Seek;
    seek.component_id = m_playback_pipeline_id;
    seek.target = target;
    seek.position = ENavigationPosition_Begining;
    resume_pipeline();
    err = m_engine_control->generic_control(EGenericEngineConfigure_Seek, &seek);
    if (EECode_OK == err) {
      return AM_PLAYBACK_SERVICE_RETCODE_OK;
    }
    return AM_PLAYBACK_SERVICE_RETCODE_ACTION_FAIL;
  }
  return AM_PLAYBACK_SERVICE_RETCODE_BAD_STATE;
}

int AMPlaybackServiceInstance::query_current_position(unsigned long long *pos)
{
  * pos = 0;
  return AM_PLAYBACK_SERVICE_RETCODE_NOT_SUPPORT;
}

int AMPlaybackServiceInstance::fast_forward
  (unsigned short speed, unsigned char scan_mode, int from_begining)
{
  std::lock_guard<std::mutex> autolock(m_mtx);
  if (m_engine_control && (AM_PLAYBACK_SERVICE_INSTANCE_STATE_RUNING == m_state)) {
    SPbFeedingRule ff;
    EECode err = EECode_OK;
    if (!from_begining) {
      ff.check_field = EGenericEngineConfigure_FastForward;
    } else {
      ff.check_field = EGenericEngineConfigure_FastForwardFromBegin;
    }
    ff.component_id = m_playback_pipeline_id;
    ff.direction = 0;
    ff.feeding_rule = DecoderFeedingRule_IDROnly;
    ff.speed = speed;
    resume_pipeline();
    err = m_engine_control->generic_control(EGenericEngineConfigure_FastForward, &ff);
    if (EECode_OK == err) {
      return AM_PLAYBACK_SERVICE_RETCODE_OK;
    }
    return AM_PLAYBACK_SERVICE_RETCODE_ACTION_FAIL;
  }
  return AM_PLAYBACK_SERVICE_RETCODE_BAD_STATE;
}

int AMPlaybackServiceInstance::fast_backward
  (unsigned short speed, unsigned char scan_mode, int from_end)
{
  std::lock_guard<std::mutex> autolock(m_mtx);
  if (m_engine_control && (AM_PLAYBACK_SERVICE_INSTANCE_STATE_RUNING == m_state)) {
    SPbFeedingRule fb;
    EECode err = EECode_OK;
    if (!from_end) {
      fb.check_field = EGenericEngineConfigure_FastBackward;
    } else {
      fb.check_field = EGenericEngineConfigure_FastBackwardFromEnd;
    }
    fb.component_id = m_playback_pipeline_id;
    fb.direction = 0;
    fb.feeding_rule = DecoderFeedingRule_IDROnly;
    fb.speed = speed;
    resume_pipeline();
    err = m_engine_control->generic_control(EGenericEngineConfigure_FastBackward, &fb);
    if (EECode_OK == err) {
      return AM_PLAYBACK_SERVICE_RETCODE_OK;
    }
    return AM_PLAYBACK_SERVICE_RETCODE_ACTION_FAIL;
  }
  return AM_PLAYBACK_SERVICE_RETCODE_BAD_STATE;
}

void AMPlaybackServiceInstance::clear()
{

  if (m_engine_control) {
    m_engine_control->destroy();
    m_engine_control = nullptr;
  }

  m_state = AM_PLAYBACK_SERVICE_INSTANCE_STATE_IDLE;

  m_demuxer_id = 0;
  m_video_decoder_id = 0;
  m_audio_decoder_id = 0;
  m_video_renderer_id = 0;
  m_audio_renderer_id = 0;
  m_playback_pipeline_id = 0;

  m_is_live_streaming = 0;
  m_has_decode_error = 0;
  m_playback_complete = 0;
  m_trick_mode = AM_PLAYBACK_SERVICE_INSTANCE_TRICKMODE_RUN;

  m_speed = 0x0100;
  m_direction = 0;
  m_scan_mode = 0;
}

void AMPlaybackServiceInstance::resume_pipeline()
{
    if ((AM_PLAYBACK_SERVICE_INSTANCE_TRICKMODE_PAUSED == m_trick_mode)
      || (AM_PLAYBACK_SERVICE_INSTANCE_TRICKMODE_STEP == m_trick_mode)) {
      SUserParamResume resume;
      resume.check_field = EGenericEngineConfigure_Resume;
      resume.component_id = m_playback_pipeline_id;
      m_engine_control->generic_control(EGenericEngineConfigure_Resume, &resume);
      m_trick_mode = AM_PLAYBACK_SERVICE_INSTANCE_TRICKMODE_RUN;
    }
}

