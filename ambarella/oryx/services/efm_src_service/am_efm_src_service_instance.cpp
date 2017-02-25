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
#include <mutex>
#include <sys/time.h>
#include <string>

#include "am_base_include.h"
#include "am_define.h"
#include "am_playback_new_if.h"
#include "am_efm_src_service_instance.h"

static void __callback_for_video_injecter(void *context, TTime mono_pts, TTime dsp_pts)
{
}

AMEFMSourceServiceInstance::AMEFMSourceServiceInstance(int index)
  : m_engine_control(NULL)
  , m_index(index)
  , m_state(AM_EFM_SRC_SERVICE_INSTANCE_STATE_IDLE)
{
  m_source_id = 0;
  m_video_decoder_id = 0;
  m_efm_injecter_id = 0;
  m_dump_efm_src_id = 0;
}

AMEFMSourceServiceInstance::~AMEFMSourceServiceInstance()
{
  if (m_engine_control && (AM_EFM_SRC_SERVICE_INSTANCE_STATE_IDLE != m_state)) {
    m_engine_control->stop();
    m_engine_control->exit_processing();
  }
  clear();
}

int AMEFMSourceServiceInstance::feed_efm_from_es(char *input_url, uint8_t stream_index)
{
  std::lock_guard<std::mutex> autolock(m_mtx);
  if (!input_url) {
    return AM_EFM_SRC_SERVICE_RETCODE_INVALID_PARAMETERS;
  }
  if ((!m_engine_control)
    && (AM_EFM_SRC_SERVICE_INSTANCE_STATE_IDLE == m_state)) {
    EECode err = EECode_OK;
    TGenericID connection_id = 0;
    m_engine_control = CreateGenericMediaEngine();
    if (!m_engine_control) {
      return AM_EFM_SRC_SERVICE_RETCODE_NO_RESOURCE;
    }
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
      clear();
      return AM_EFM_SRC_SERVICE_RETCODE_ACTION_FAIL;
    }
    SConfigVideoStreamID config_stream_id;
    config_stream_id.check_field = EGenericEngineConfigure_VideoStreamID;
    config_stream_id.stream_id = 1 << stream_index;
    config_stream_id.component_id = m_efm_injecter_id;
    err = m_engine_control->generic_control(EGenericEngineConfigure_VideoStreamID, &config_stream_id);
    if (EECode_OK != err) {
      printf("set stream id (0x%08x) fail\n", config_stream_id.stream_id);
      clear();
      return AM_EFM_SRC_SERVICE_RETCODE_ACTION_FAIL;
    }
    err = m_engine_control->set_source_url(m_source_id, input_url);
    if (EECode_OK != err) {
      printf("set_source_url fail\n");
      clear();
      return AM_EFM_SRC_SERVICE_RETCODE_ACTION_FAIL;
    }
    err = m_engine_control->run_processing();
    if (EECode_OK != err) {
      printf("run_processing fail\n");
      clear();
      return AM_EFM_SRC_SERVICE_RETCODE_ACTION_FAIL;
    }
    err = m_engine_control->start();
    if (EECode_OK != err) {
      printf("start fail\n");
      m_engine_control->exit_processing();
      clear();
      return AM_EFM_SRC_SERVICE_RETCODE_ACTION_FAIL;
    }
    m_state = AM_EFM_SRC_SERVICE_INSTANCE_STATE_RUNING;
    return AM_EFM_SRC_SERVICE_RETCODE_OK;
  }
  return AM_EFM_SRC_SERVICE_RETCODE_INSTANCE_BUSY;
}

int AMEFMSourceServiceInstance::feed_efm_from_usb_camera(char *input_url, uint8_t stream_index,
  char *dump_efm_src_filename, uint8_t format, uint32_t width, uint32_t height, uint32_t fps)
{
  std::lock_guard<std::mutex> autolock(m_mtx);
  if (!input_url) {
    return AM_EFM_SRC_SERVICE_RETCODE_INVALID_PARAMETERS;
  }
  if ((!m_engine_control)
    && (AM_EFM_SRC_SERVICE_INSTANCE_STATE_IDLE == m_state)) {
    EECode err = EECode_OK;
    TGenericID connection_id = 0;
    m_engine_control = CreateGenericMediaEngine();
    if (!m_engine_control) {
      return AM_EFM_SRC_SERVICE_RETCODE_NO_RESOURCE;
    }
    err = m_engine_control->begin_config_process_pipeline();
    err = m_engine_control->new_component(
      EGenericComponentType_VideoCapture, m_source_id,
      DNameTag_LinuxUVC);
    err = m_engine_control->new_component(
      EGenericComponentType_VideoDecoder, m_video_decoder_id,
      DNameTag_FFMpeg);
    err = m_engine_control->new_component(
      EGenericComponentType_VideoInjecter, m_efm_injecter_id,
      DNameTag_AMBAEFM);
    if (dump_efm_src_filename) {
      err = m_engine_control->new_component(
        EGenericComponentType_VideoRawSink, m_dump_efm_src_id);
    }
    err = m_engine_control->connect_component(
      connection_id, m_source_id, m_video_decoder_id,
      StreamType_Video);
    err = m_engine_control->connect_component(
      connection_id, m_video_decoder_id, m_efm_injecter_id,
      StreamType_Video);
    if (dump_efm_src_filename) {
      err = m_engine_control->connect_component(
        connection_id, m_video_decoder_id, m_dump_efm_src_id,
        StreamType_Video);
    }
    err = m_engine_control->finalize_config_process_pipeline();
    if (EECode_OK != err) {
      printf("finalize_config_process_pipeline fail\n");
      clear();
      return AM_EFM_SRC_SERVICE_RETCODE_ACTION_FAIL;
    }
    SConfigVideoStreamID config_stream_id;
    config_stream_id.check_field = EGenericEngineConfigure_VideoStreamID;
    config_stream_id.stream_id = 1 << stream_index;
    config_stream_id.component_id = m_efm_injecter_id;
    err = m_engine_control->generic_control(EGenericEngineConfigure_VideoStreamID, &config_stream_id);
    if (EECode_OK != err) {
      printf("set stream id (0x%08x) fail\n", config_stream_id.stream_id);
      clear();
      return AM_EFM_SRC_SERVICE_RETCODE_ACTION_FAIL;
    }
    SVideoCaptureParams cap_params;
    memset(&cap_params, 0x0, sizeof(cap_params));
    cap_params.check_field = EGenericEngineConfigure_VideoCapture_Params;
    cap_params.module_id = m_source_id;
    cap_params.cap_win_width = width;
    cap_params.cap_win_height = height;
    if (fps) {
      cap_params.framerate_num = 90000;
      cap_params.framerate_den = cap_params.framerate_num / fps;
    }
    if (AM_EFM_SRC_FORMAT_MJPEG == format) {
      cap_params.format = StreamFormat_JPEG;
    } else if (AM_EFM_SRC_FORMAT_YUYV == format) {
      cap_params.format = StreamFormat_PixelFormat_YUYV;
    }
    err = m_engine_control->generic_control(EGenericEngineConfigure_VideoCapture_Params, (void *) &cap_params);
    if (EECode_OK != err) {
      printf("EGenericEngineConfigure_VideoCapture_Params fail\n");
      clear();
      return (-1);
    }
    err = m_engine_control->set_source_url(m_source_id, input_url);
    if (EECode_OK != err) {
      printf("set_source_url fail\n");
      clear();
      return AM_EFM_SRC_SERVICE_RETCODE_ACTION_FAIL;
    }
    if (dump_efm_src_filename) {
      err = m_engine_control->set_sink_url(m_dump_efm_src_id, dump_efm_src_filename);
      if (EECode_OK != err) {
        printf("dump_efm_src_filename fail\n");
        clear();
        return AM_EFM_SRC_SERVICE_RETCODE_ACTION_FAIL;
      }
    }
    err = m_engine_control->run_processing();
    if (EECode_OK != err) {
      printf("run_processing fail\n");
      clear();
      return AM_EFM_SRC_SERVICE_RETCODE_ACTION_FAIL;
    }
    err = m_engine_control->start();
    if (EECode_OK != err) {
      printf("start fail\n");
      m_engine_control->exit_processing();
      clear();
      return AM_EFM_SRC_SERVICE_RETCODE_ACTION_FAIL;
    }
    m_state = AM_EFM_SRC_SERVICE_INSTANCE_STATE_RUNING;
    return AM_EFM_SRC_SERVICE_RETCODE_OK;
  }
  return AM_EFM_SRC_SERVICE_RETCODE_INSTANCE_BUSY;
}

int AMEFMSourceServiceInstance::end_feed()
{
  std::lock_guard<std::mutex> autolock(m_mtx);
  if (m_engine_control) {
    if (AM_EFM_SRC_SERVICE_INSTANCE_STATE_IDLE != m_state) {
      m_engine_control->stop();
      m_engine_control->exit_processing();
      m_engine_control->destroy();
      m_engine_control = NULL;
    }
    clear();
    return AM_EFM_SRC_SERVICE_RETCODE_OK;
  }
  return AM_EFM_SRC_SERVICE_RETCODE_OK;
}

int AMEFMSourceServiceInstance::setup_yuv_capture_pipeline(uint8_t buffer_index,
                                                           uint8_t stream_index)
{
  std::lock_guard<std::mutex> autolock(m_mtx);
  if ((!m_engine_control)
    && (AM_EFM_SRC_SERVICE_INSTANCE_STATE_IDLE == m_state)) {
    EECode err = EECode_OK;
    TGenericID connection_id = 0;
    m_engine_control = CreateGenericMediaEngine();
    if (!m_engine_control) {
      printf("create media engine fail\n");
      return AM_EFM_SRC_SERVICE_RETCODE_NO_RESOURCE;
    }
    err = m_engine_control->begin_config_process_pipeline();
    err = m_engine_control->new_component(
      EGenericComponentType_VideoCapture, m_source_id,
      DNameTag_AMBA);
    err = m_engine_control->new_component(
      EGenericComponentType_VideoInjecter, m_efm_injecter_id,
      DNameTag_AMBAEFM);
    err = m_engine_control->connect_component(
      connection_id, m_source_id, m_efm_injecter_id,
      StreamType_Video);
    err = m_engine_control->finalize_config_process_pipeline();
    if (EECode_OK != err) {
      printf("finalize_config_process_pipeline fail\n");
      clear();
      return AM_EFM_SRC_SERVICE_RETCODE_ACTION_FAIL;
    }
    SConfigVideoStreamID config_stream_id;
    config_stream_id.check_field = EGenericEngineConfigure_VideoStreamID;
    config_stream_id.stream_id = 1 << stream_index;
    config_stream_id.component_id = m_efm_injecter_id;
    err = m_engine_control->generic_control(EGenericEngineConfigure_VideoStreamID, &config_stream_id);
    if (EECode_OK != err) {
      printf("set stream id (0x%08x) fail\n", config_stream_id.stream_id);
      clear();
      return AM_EFM_SRC_SERVICE_RETCODE_ACTION_FAIL;
    }
    SVideoCaptureParams cap_params;
    memset(&cap_params, 0x0, sizeof(cap_params));
    cap_params.check_field = EGenericEngineConfigure_VideoCapture_Params;
    cap_params.module_id = m_source_id;
    cap_params.buffer_id = buffer_index;
    cap_params.b_snapshot_mode = 1;
    err = m_engine_control->generic_control(EGenericEngineConfigure_VideoCapture_Params, (void *) &cap_params);
    if (EECode_OK != err) {
      printf("EGenericEngineConfigure_VideoCapture_Params fail\n");
      clear();
      return AM_EFM_SRC_SERVICE_RETCODE_ACTION_FAIL;
    }
    SVideoInjecterCallback video_injecter_callback;
    video_injecter_callback.callback = __callback_for_video_injecter;
    video_injecter_callback.callback_context = nullptr;
    video_injecter_callback.check_field = EGenericEngineConfigure_VideoInjecter_SetCallback;
    video_injecter_callback.component_id = m_efm_injecter_id;
    err = m_engine_control->generic_control(EGenericEngineConfigure_VideoInjecter_SetCallback, (void *) &video_injecter_callback);
    if (EECode_OK != err) {
      printf("EGenericEngineConfigure_VideoInjecter_SetCallback fail\n");
      clear();
      return AM_EFM_SRC_SERVICE_RETCODE_ACTION_FAIL;
    }
    SConfigSourceBufferID source_buffer_id;
    source_buffer_id.check_field = EGenericEngineConfigure_SourceBufferID;
    source_buffer_id.component_id = m_efm_injecter_id;
    source_buffer_id.source_buffer_id = buffer_index;
    err = m_engine_control->generic_control(EGenericEngineConfigure_SourceBufferID, (void *) &source_buffer_id);
    if (EECode_OK != err) {
      printf("EGenericEngineConfigure_SourceBufferID fail\n");
      clear();
      return AM_EFM_SRC_SERVICE_RETCODE_ACTION_FAIL;
    }
    err = m_engine_control->run_processing();
    if (EECode_OK != err) {
      printf("run_processing fail\n");
      clear();
      return AM_EFM_SRC_SERVICE_RETCODE_ACTION_FAIL;
    }
    err = m_engine_control->start();
    if (EECode_OK != err) {
      printf("start fail\n");
      m_engine_control->exit_processing();
      clear();
      return AM_EFM_SRC_SERVICE_RETCODE_ACTION_FAIL;
    }
    m_state = AM_EFM_SRC_SERVICE_INSTANCE_STATE_RUNING;
  } else {
    printf("already setup? engine %p, state %d\n", m_engine_control, m_state);
  }
  return AM_EFM_SRC_SERVICE_RETCODE_OK;
}

int AMEFMSourceServiceInstance::destroy_yuv_capture_pipeline()
{
  std::lock_guard<std::mutex> autolock(m_mtx);
  if (m_engine_control) {
    if (AM_EFM_SRC_SERVICE_INSTANCE_STATE_IDLE != m_state) {
      m_engine_control->stop();
      m_engine_control->exit_processing();
      m_engine_control->destroy();
      m_engine_control = NULL;
    }
    clear();
  }
  return AM_EFM_SRC_SERVICE_RETCODE_OK;
}

int AMEFMSourceServiceInstance::yuv_capture_for_efm(uint8_t number_of_yuv_frames)
{
  std::lock_guard<std::mutex> autolock(m_mtx);
  if (m_engine_control) {
    if (AM_EFM_SRC_SERVICE_INSTANCE_STATE_IDLE != m_state) {
      EECode err = EECode_OK;
#if 0
      SVideoCaptureSnapshotParams cap;
      memset(&cap, 0x0, sizeof(cap));
      cap.check_field = EGenericEngineConfigure_VideoCapture_Snapshot;
      cap.module_id = m_source_id;
      cap.snapshot_framenumber = number_of_yuv_frames;
      err = m_engine_control->generic_control(EGenericEngineConfigure_VideoCapture_Snapshot, &cap);
      if (EECode_OK != err) {
          printf("snapshot cap %d fail, err 0x%08x, %s\n", cap.snapshot_framenumber, err, gfGetErrorCodeString(err));
          return AM_EFM_SRC_SERVICE_RETCODE_ACTION_FAIL;
      }
#else
      SVideoInjecterBatchInject batch_inject;
      memset(&batch_inject, 0x0, sizeof(batch_inject));
      batch_inject.check_field = EGenericEngineConfigure_VideoInjecter_BatchInject;
      batch_inject.component_id = m_efm_injecter_id;
      batch_inject.num_of_frame = number_of_yuv_frames;
      err = m_engine_control->generic_control(EGenericEngineConfigure_VideoInjecter_BatchInject, &batch_inject);
      if (EECode_OK != err) {
          printf("batch capture %d fail, err 0x%08x, %s\n", batch_inject.num_of_frame, err, gfGetErrorCodeString(err));
          return AM_EFM_SRC_SERVICE_RETCODE_ACTION_FAIL;
      }
#endif
    } else {
      printf("unexpected state %d\n", m_state);
      return AM_EFM_SRC_SERVICE_RETCODE_BAD_STATE;
    }
    return AM_EFM_SRC_SERVICE_RETCODE_OK;
  } else {
    printf("pipeline not setup yet\n");
    return AM_EFM_SRC_SERVICE_RETCODE_BAD_STATE;
  }
  return AM_EFM_SRC_SERVICE_RETCODE_OK;
}

void AMEFMSourceServiceInstance::clear()
{
  if (m_engine_control) {
    m_engine_control->destroy();
    m_engine_control = NULL;
  }
  m_state = AM_EFM_SRC_SERVICE_INSTANCE_STATE_IDLE;
  m_source_id = 0;
  m_video_decoder_id = 0;
  m_efm_injecter_id = 0;
  m_dump_efm_src_id = 0;
}

