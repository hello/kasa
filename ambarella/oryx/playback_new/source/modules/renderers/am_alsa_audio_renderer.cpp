/**
 * am_alsa_audio_renderer.cpp
 *
 * History:
 *    2013/05/17 - [Zhi He] create file
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

#include "am_audio_al.h"

#ifdef BUILD_MODULE_ALSA

#include "am_alsa_audio_renderer.h"

IAudioRenderer *gfCreateAudioRendererAlsa(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink)
{
  return CAudioRendererAlsa::Create(pName, pPersistMediaConfig, pEngineMsgSink);
}

//-----------------------------------------------------------------------
//
// CAudioRendererModule
//
//-----------------------------------------------------------------------
CAudioRendererAlsa::CAudioRendererAlsa(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink)
  : inherited(pname)
  , mpPersistMediaConfig(pPersistMediaConfig)
  , mpMsgSink(pMsgSink)
  , mpAudioAL(NULL)
{
  memset(&mfAudioAL, 0x0, sizeof(mfAudioAL));
}

IAudioRenderer *CAudioRendererAlsa::Create(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink)
{
  CAudioRendererAlsa *result = new CAudioRendererAlsa(pname, pPersistMediaConfig, pMsgSink);
  if (result && result->Construct() != EECode_OK) {
    LOG_FATAL("CAudioRendererModule->Construct() fail\n");
    delete result;
    result = NULL;
  }
  return result;
}

void CAudioRendererAlsa::Destroy()
{
  Delete();
}

EECode CAudioRendererAlsa::Construct()
{
  DSET_MODULE_LOG_CONFIG(ELogModuleAudioRenderer);
  gfSetupAlsaAlContext(&mfAudioAL);
  return EECode_OK;
}

CAudioRendererAlsa::~CAudioRendererAlsa()
{
}

EECode CAudioRendererAlsa::SetupContext(SAudioParams *param)
{
  DASSERT(mfAudioAL.f_open_playback);
  LOGM_INFO("mfAudioAL.f_open_playback...\n");
  mpAudioAL = mfAudioAL.f_open_playback((TChar *)mpPersistMediaConfig->audio_device.audio_device_name, param->sample_rate, param->channel_number, param->frame_size);
  if (!mpAudioAL) {
    LOGM_ERROR("mfAudioAL.f_open_playback fail\n");
    return EECode_Error;
  }
  return EECode_OK;
}

EECode CAudioRendererAlsa::DestroyContext()
{
  if (mfAudioAL.f_close && mpAudioAL) {
    mfAudioAL.f_close(mpAudioAL);
  }
  mpAudioAL = NULL;
  return EECode_OK;
}

EECode CAudioRendererAlsa::Start(TUint index)
{
  return EECode_OK;
}

EECode CAudioRendererAlsa::Stop(TUint index)
{
  return EECode_OK;
}

EECode CAudioRendererAlsa::Flush(TUint index)
{
  return EECode_OK;
}

EECode CAudioRendererAlsa::Render(CIBuffer *p_buffer, TUint index)
{
  TU8 *data = (TU8 *)(p_buffer->GetDataPtr());
  TUint size = p_buffer->GetDataSize();
  TUint num = 0;
  TInt ret = 0;
  DASSERT(mfAudioAL.f_render_data);
  ret = mfAudioAL.f_render_data(mpAudioAL, data, size / 2, &num);
  if (!ret) {
    return EECode_OK;
  }
  LOGM_ERROR("rener fail, ret %d\n", ret);
  return EECode_Error;
}

EECode CAudioRendererAlsa::Pause(TUint index)
{
  return EECode_OK;
}

EECode CAudioRendererAlsa::Resume(TUint index)
{
  return EECode_OK;
}

EECode CAudioRendererAlsa::SyncTo(TTime pts, TUint index)
{
  LOGM_ERROR("TO DO\n");
  return EECode_OK;
}

EECode CAudioRendererAlsa::SyncMultipleStream(TUint master_index)
{
  LOGM_ERROR("TO DO\n");
  return EECode_OK;
}

#endif

