/**
 * am_amba_video_renderer.cpp
 *
 * History:
 *    2013/04/09 - [Zhi He] create file
 *
 * Copyright (c) 2015 Ambarella, Inc.
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
 */

#include "am_native.h"
#include "am_native_log.h"

#include "am_internal.h"

#include "am_osal.h"
#include "am_framework.h"
#include "am_modules_interface.h"

#include "am_amba_dsp.h"

#ifdef BUILD_MODULE_AMBA_DSP

#include "am_amba_video_renderer.h"

IVideoRenderer *gfCreateAmbaVideoRendererModule(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink)
{
  return CAmbaVideoRenderer::Create(pName, pPersistMediaConfig, pEngineMsgSink);
}

//-----------------------------------------------------------------------
//
// CAmbaVideoRenderer
//
//-----------------------------------------------------------------------
CAmbaVideoRenderer::CAmbaVideoRenderer(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink)
  : inherited(pname)
  , mpPersistMediaConfig(pPersistMediaConfig)
  , mpMsgSink(pMsgSink)
  , mIavFd(-1)
{
}

IVideoRenderer *CAmbaVideoRenderer::Create(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink)
{
  CAmbaVideoRenderer *result = new CAmbaVideoRenderer(pname, pPersistMediaConfig, pMsgSink);
  if (result && result->Construct() != EECode_OK) {
    delete result;
    result = NULL;
  }
  return result;
}

void CAmbaVideoRenderer::Destroy()
{
  Delete();
}

EECode CAmbaVideoRenderer::Construct()
{
  DSET_MODULE_LOG_CONFIG(ELogModuleAmbaVideoRenderer);
  gfSetupDSPAlContext(&mfDSPAL);
  DASSERT(mpPersistMediaConfig);
  mIavFd = mpPersistMediaConfig->dsp_config.device_fd;
  return EECode_OK;
}

CAmbaVideoRenderer::~CAmbaVideoRenderer()
{
}

EECode CAmbaVideoRenderer::SetupContext(SVideoParams *param)
{
  return EECode_OK;
}

EECode CAmbaVideoRenderer::DestroyContext()
{
  return EECode_OK;
}

EECode CAmbaVideoRenderer::Start(TUint index)
{
  return EECode_OK;
}

EECode CAmbaVideoRenderer::Stop(TUint index)
{
  return EECode_OK;
}

EECode CAmbaVideoRenderer::Flush(TUint index)
{
  return EECode_OK;
}

EECode CAmbaVideoRenderer::Render(CIBuffer *p_buffer, TUint index)
{
  return EECode_OK;
}

EECode CAmbaVideoRenderer::Pause(TUint index)
{
  if (mfDSPAL.f_trickplay) {
    mfDSPAL.f_trickplay(mIavFd, (TU8) index, EAMDSP_TRICK_PLAY_PAUSE);
  } else {
    LOGM_ERROR("NULL mfDSPAL.f_trickplay\n");
    return EECode_BadParam;
  }
  return EECode_OK;
}

EECode CAmbaVideoRenderer::Resume(TUint index)
{
  if (mfDSPAL.f_trickplay) {
    mfDSPAL.f_trickplay(mIavFd, (TU8) index, EAMDSP_TRICK_PLAY_RESUME);
  } else {
    LOGM_ERROR("NULL mfDSPAL.f_trickplay\n");
    return EECode_BadParam;
  }
  return EECode_OK;
}

EECode CAmbaVideoRenderer::StepPlay(TUint index)
{
  if (mfDSPAL.f_trickplay) {
    mfDSPAL.f_trickplay(mIavFd, (TU8) index, EAMDSP_TRICK_PLAY_STEP);
  } else {
    LOGM_ERROR("NULL mfDSPAL.f_trickplay\n");
    return EECode_BadParam;
  }
  return EECode_OK;
}

EECode CAmbaVideoRenderer::SyncTo(TTime pts, TUint index)
{
  return EECode_OK;
}

EECode CAmbaVideoRenderer::QueryLastShownTimeStamp(TTime &pts, TUint index)
{
  if (mfDSPAL.f_query_decode_status) {
    SAmbaDSPDecodeStatus dec_status;
    dec_status.decoder_id = (TU8) index;
    mfDSPAL.f_query_decode_status(mIavFd, &dec_status);
    pts = dec_status.last_pts;
  } else {
    LOGM_ERROR("NULL mfDSPAL.f_query_decode_status\n");
    return EECode_BadParam;
  }
  return EECode_OK;
}

TU32 CAmbaVideoRenderer::IsMonitorMode(TUint index) const
{
  if (mfDSPAL.f_query_decode_config) {
    SAmbaDSPQueryDecodeConfig dec_config;
    mfDSPAL.f_query_decode_config(mIavFd, &dec_config);
    if (dec_config.rendering_monitor_mode) {
      return 1;
    }
    return 0;
  }
  LOGM_ERROR("NULL mfDSPAL.f_query_decode_config\n");
  return 0;
}

EECode CAmbaVideoRenderer::WaitReady(TUint index)
{
  if (mfDSPAL.f_decode_wait_vout_dormant) {
    TInt ret = mfDSPAL.f_decode_wait_vout_dormant(mIavFd, (TU8) index);
    if (!ret) {
      return EECode_OK;
    }
    gfOSmsleep(30);
    if (0 > ret) {
      return EECode_OSError;
    }
    return EECode_TryAgain;
  }
  LOGM_ERROR("NULL mfDSPAL.f_decode_wait_vout_dormant\n");
  return EECode_BadState;
}

EECode CAmbaVideoRenderer::Wakeup(TUint index)
{
  if (mfDSPAL.f_decode_wake_vout) {
    TInt ret = mfDSPAL.f_decode_wake_vout(mIavFd, (TU8) index);
    if (!ret) {
      return EECode_OK;
    }
    LOGM_FATAL("wake vout fail\n");
    return EECode_OSError;
  }
  LOGM_ERROR("NULL mfDSPAL.f_decode_wake_vout\n");
  return EECode_BadState;
}

EECode CAmbaVideoRenderer::WaitEosMsg(TTime &last_pts, TUint index)
{
  if (mfDSPAL.f_decode_wait_eos_flag) {
    TInt ret = 0;
    SAmbaDSPDecodeEOSTimestamp eos_timestamp;
    eos_timestamp.decoder_id = (TU8) index;
    ret = mfDSPAL.f_decode_wait_eos_flag(mIavFd, &eos_timestamp);
    if (!ret) {
      last_pts = (TTime) (((TTime) eos_timestamp.last_pts_high) << 32) | (TTime) ((TTime) eos_timestamp.last_pts_low);
      return EECode_OK;
    } else if (DDECODER_STOPPED == ret) {
      return EECode_OK_AlreadyStopped;
    } else if (0 > ret) {
      LOGM_FATAL("wait eos flag fail\n");
      return EECode_OSError;
    }
    return EECode_TryAgain;
  }
  LOGM_ERROR("NULL mfDSPAL.f_decode_wait_eos_flag\n");
  return EECode_BadState;
}

EECode CAmbaVideoRenderer::WaitLastShownTimeStamp(TTime &last_pts, TUint index)
{
  if (mfDSPAL.f_decode_wait_eos) {
    TInt ret = 0;
    SAmbaDSPDecodeEOSTimestamp eos_timestamp;
    eos_timestamp.decoder_id = (TU8) index;
    eos_timestamp.last_pts_high = (TU32) ((last_pts >> 32) & 0xffffffff);
    eos_timestamp.last_pts_low = (TU32) (last_pts & 0xffffffff);
    ret = mfDSPAL.f_decode_wait_eos(mIavFd, &eos_timestamp);
    if (!ret) {
      return EECode_OK;
    } else if (0 > ret) {
      LOGM_FATAL("wait eos fail\n");
      return EECode_OSError;
    }
    return EECode_TryAgain;
  }
  LOGM_ERROR("NULL mfDSPAL.f_decode_wait_eos\n");
  return EECode_BadState;
}

#endif

