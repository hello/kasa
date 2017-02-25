/**
 * am_alsa_al.cpp
 *
 * History:
 *    2014/01/05 - [Zhi He] create file
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

#include "alsa/asoundlib.h"

static TInt __alsa_capture_init(snd_pcm_t **pp_handle, TChar *device_name)
{
  TInt ret = 0;
  if ((!device_name) || (!device_name[0])) {
    ret = snd_pcm_open(pp_handle, "default", SND_PCM_STREAM_CAPTURE, 0);
    if (ret < 0) {
      LOG_NOTICE("snd_pcm_opne by default failed! try MICALL.\n");
      ret = snd_pcm_open(pp_handle, "MICALL", SND_PCM_STREAM_CAPTURE, 0);
    }
  } else {
    ret = snd_pcm_open(pp_handle, device_name, SND_PCM_STREAM_CAPTURE, 0);
    LOG_NOTICE("snd_pcm_open(%s), capture\n", device_name);
  }
  if (ret < 0) {
    LOG_FATAL("snd_pcm_open() fail, ret: %d\n", ret);
    return ret;
  }
  return 0;
}

static TInt __alsa_playback_init(snd_pcm_t **pp_handle, TChar *device_name)
{
  TInt ret = 0;
  if ((!device_name) || (!device_name[0])) {
    ret = snd_pcm_open(pp_handle, "default", SND_PCM_STREAM_PLAYBACK, 0);
  } else {
    ret = snd_pcm_open(pp_handle, device_name, SND_PCM_STREAM_PLAYBACK, 0);
    LOG_NOTICE("snd_pcm_open(%s), playback\n", device_name);
  }
  if (ret < 0) {
    LOG_ERROR("snd_pcm_open by default failed, ret %d\n", ret);
    return ret;
  }
  return 0;
}

static void __alsa_close(void *p_handle)
{
  snd_pcm_close((snd_pcm_t *) p_handle);
}

static TInt __alsa_set_params(snd_pcm_t *p_handle, int stream, snd_pcm_format_t pcmFormat, TUint sampleRate, TUint numOfChannels, TULong frame_size)
{
  TInt err;
  snd_pcm_hw_params_t *params;
  snd_pcm_sw_params_t *swparams;
  snd_pcm_uframes_t buffer_size;
  TUint start_threshold, stop_threshold;
  TUint buffer_time = 0;
  snd_pcm_hw_params_alloca(&params);
  snd_pcm_sw_params_alloca(&swparams);
  err = snd_pcm_hw_params_any(p_handle, params);
  if (err < 0) {
    LOG_FATAL("Broken configuration for this PCM: no configurations available\n");
    return err;
  }
  err = snd_pcm_hw_params_set_access(p_handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
  if (err < 0) {
    LOG_FATAL("Access type not available\n");
    return err;
  }
  err = snd_pcm_hw_params_set_format(p_handle, params, pcmFormat);
  if (err < 0) {
    LOG_FATAL("Sample format non available\n");
    return err;
  }
  err = snd_pcm_hw_params_set_channels(p_handle, params, numOfChannels);
  if (err < 0) {
    LOG_FATAL("Channels count(%d) not available\n", numOfChannels);
    return err;
  }
  err = snd_pcm_hw_params_set_rate_near(p_handle, params, &sampleRate, 0);
  err = snd_pcm_hw_params_get_buffer_time_max(params, &buffer_time, 0);
  if (buffer_time > 500000) {
    buffer_time = 500000;
  }
  LOG_INFO("audio frame_size %ld\n", frame_size);
  err = snd_pcm_hw_params_set_period_size(p_handle, params, frame_size, 0);
  err = snd_pcm_hw_params_set_buffer_time_near(p_handle, params, &buffer_time, 0);
  err = snd_pcm_hw_params(p_handle, params);
  if (err < 0) {
    LOG_FATAL("Unable to install hw params:\n");
    return EECode_Error;
  }
  snd_pcm_hw_params_get_period_size(params, &frame_size, 0);
  snd_pcm_hw_params_get_buffer_size(params, &buffer_size);
  snd_pcm_sw_params_current(p_handle, swparams);
  err = snd_pcm_sw_params_set_avail_min(p_handle, swparams, frame_size);
  /* round up to closest transfer boundary */
  if (stream == SND_PCM_STREAM_PLAYBACK)
  { start_threshold = (buffer_size / frame_size) * frame_size; }
  else
  { start_threshold = 1; }
  err = snd_pcm_sw_params_set_start_threshold(p_handle, swparams, start_threshold);
  stop_threshold = buffer_size;
  err = snd_pcm_sw_params_set_stop_threshold(p_handle, swparams, stop_threshold);
  err = snd_pcm_sw_params(p_handle, swparams);
  if (err < 0) {
    LOG_FATAL("unable to install sw params:\n");
    return err;
  }
  //mBitsPerSample = snd_pcm_format_physical_width(pcmFormat);
  //mBitsPerFrame = mBitsPerSample * numOfChannels;
  //mChunkBytes = frame_size * mBitsPerFrame / 8;
  LOG_INFO("Support Pause? %d\n", snd_pcm_hw_params_can_pause(params));
  return 0;
}

static TInt __alsa_xrun(snd_pcm_t *p_handle)
{
  snd_pcm_status_t *status;
  int err;
  snd_pcm_status_alloca(&status);
  if ((err = snd_pcm_status(p_handle, status)) < 0) {
    LOG_FATAL("status error: %s\n", snd_strerror(err));
    return err;
  }
  if (snd_pcm_status_get_state(status) == SND_PCM_STATE_XRUN) {
    //AM_WARNING("%s!!!\n", stream == SND_PCM_STREAM_PLAYBACK ?
    //"Playback underrun":"Capture overrun");
    if ((err = snd_pcm_prepare(p_handle)) < 0) {
      LOG_FATAL("xrun: prepare error: %s", snd_strerror(err));
      return err;
    }
    return 0;       // ok, data should be accepted again
  }
  if (snd_pcm_status_get_state(status) == SND_PCM_STATE_DRAINING) {
    LOG_INFO("capture stream format change? attempting recover...\n");
    if ((err = snd_pcm_prepare(p_handle)) < 0) {
      LOG_FATAL("xrun(DRAINING): prepare error: %s\n", snd_strerror(err));
      return err;
    }
    return 0;
  }
  LOG_FATAL("read/write error, state = %s\n", snd_pcm_state_name(snd_pcm_status_get_state(status)));
  return (-1);
}

static TInt __alsa_suspend(snd_pcm_t *p_handle)
{
  int err;
  while ((err = snd_pcm_resume(p_handle)) == -EAGAIN)
  { ::sleep(1); }
  if (err < 0) {
    if ((err = snd_pcm_prepare(p_handle)) < 0) {
      LOG_FATAL("suspend: prepare error: %s", snd_strerror(err));
      return err;
    }
  }
  return 0;
}

static TInt __alsa_pcm_read(snd_pcm_t *p_handle, TU8 *pData, TUint rcount)
{
  TInt ret = 0;
  TInt r;
  TU32 result = 0;
  TU32 count = rcount;
  int nowait = 0;
  if (count != 1024) {
    count = 1024;
  }
  while (count > 0) {
    r = snd_pcm_readi(p_handle, pData, count);
    if (r == -EAGAIN || (r >= 0 && (TUint)r < count)) {
      if (!nowait)
      { snd_pcm_wait(p_handle, 100); }
    } else if (r == -EPIPE) {                   // an overrun occurred
      if ((ret = __alsa_xrun(p_handle)) != 0)
      { return -1; }
    } else if (r == -ESTRPIPE) {            // a suspend event occurred
      if ((ret = __alsa_suspend(p_handle)) != 0)
      { return -1; }
    } else if (r < 0) {
      if (r == -EIO)
      { LOG_INFO("-EIO error!\n"); }
      else if (r == -EINVAL)
      { LOG_INFO("-EINVAL error!\n"); }
      else if (r == -EINTR)
      { LOG_INFO("-EINTR error!\n"); }
      else
      { LOG_FATAL("Read error: %s(%d)\n", snd_strerror(r), r); }
      return -1;
    }
    if (r > 0) {
      result += r;
      count -= r;
      pData += r * 2;
    }
  }
  return result;
}

static TInt __alsa_pcm_write(snd_pcm_t *p_handle, TU8 *pData, TUint count)
{
  TInt ret = 0;
  TInt r;
  TInt result = 0;
  TUint count_bak = count;
  TInt nowait = 0;
  //if (count < mChunkSize) {
  //    snd_pcm_format_set_silence(mPcmFormat, pData + count * mBitsPerFrame/ 8, (mChunkSize - count) * mNumOfChannels);
  //    count = mChunkSize;
  //}
  while (count > 0) {
    r = snd_pcm_writei(p_handle, pData, count);
    if (r == -EAGAIN || (r >= 0 && (size_t)r < count)) {
      if (!nowait)
      { snd_pcm_wait(p_handle, 100); }
    } else if (r == -EPIPE) {               // an overrun occurred
      if ((ret = __alsa_xrun(p_handle)) != 0)
      { goto __restart_alsa; }
    } else if (r == -ESTRPIPE) {        // a suspend event occurred
      if ((ret = __alsa_suspend(p_handle)) != 0)
      { goto __restart_alsa; }
    } else if (r == -EIO || r == -EBADFD) {
      //AM_WARNING("PcmWrite -- write error: %s\n", snd_strerror(r));
      goto __restart_alsa;
    } else if (r < 0) {
      //AM_WARNING("PcmWrite -- write error: %s\n", snd_strerror(r));
      if (r == -EINVAL)
      { LOG_INFO("-EINVAL error!\n"); }
      else
      { LOG_INFO("unknown error!\n"); }
      return -1;
    }
    if (r > 0) {
      result += r;
      count -= r;
      pData += r * 2;
    }
    continue;
__restart_alsa:
    LOG_ERROR("write fail\n");
  }
  if ((TUint)result > count_bak) {
    result = count_bak;
  }
  return result;
}

static void *__alsa_open_capture(TChar *audio_device, TU32 sample_rate, TU32 channels, TU32 frame_size)
{
  TInt ret = 0;
  snd_pcm_t *p_handle = NULL;
  snd_pcm_format_t format = SND_PCM_FORMAT_S16_LE;
  ret = __alsa_capture_init(&p_handle, audio_device);
  if (!ret) {
    ret = __alsa_set_params(p_handle, SND_PCM_STREAM_CAPTURE, format, sample_rate, channels, frame_size);
    if (0 > ret) {
      LOG_FATAL("__alsa_set_params(format %d, sample_rate %d, channel_number %d, frame_size %d) fail, ret %d\n", format, sample_rate, channels, frame_size, ret);
      return NULL;
    }
  } else {
    LOG_FATAL("__alsa_capture_init() fail, ret %d\n", ret);
    return NULL;
  }
  return (void *) p_handle;
}

static void *__alsa_open_playback(TChar *audio_device, TU32 sample_rate, TU32 channels, TU32 frame_size)
{
  TInt ret = 0;
  snd_pcm_t *p_handle = NULL;
  snd_pcm_format_t format = SND_PCM_FORMAT_S16_LE;
  ret = __alsa_playback_init(&p_handle, audio_device);
  if (!ret) {
    ret = __alsa_set_params(p_handle, SND_PCM_STREAM_PLAYBACK, format, sample_rate, channels, frame_size);
    if (0 > ret) {
      LOG_FATAL("__alsa_set_params(format %d, sample_rate %d, channel_number %d, frame_size %d) fail, ret %d\n", format, sample_rate, channels, frame_size, ret);
      return NULL;
    }
  } else {
    LOG_FATAL("__alsa_playback_init() fail, ret %d\n", ret);
    return NULL;
  }
  return (void *) p_handle;
}

static TInt __alsa_start_capture(void *p_handle)
{
  TInt ret = 0;
  snd_pcm_t *p_h = (snd_pcm_t *) p_handle;
  snd_pcm_status_t *status;
  if ((ret = snd_pcm_start(p_h)) < 0) {
    LOG_FATAL("PCM start error: %d\n", ret);
    return ret;
  }
  snd_pcm_status_alloca(&status);
  if ((ret = snd_pcm_status(p_h, status)) < 0) {
    LOG_FATAL("Get PCM status error: %d\n", ret);
    return ret;
  }
  //snd_pcm_status_get_trigger_tstamp(status, &mStartTimeStamp);
  return 0;
}

static TInt __alsa_render_data(void *p_handle, TU8 *pData, TUint dataSize, TUint *pNumFrames)
{
  TInt frm_cnt;
  frm_cnt = __alsa_pcm_write((snd_pcm_t *) p_handle, pData, dataSize / 2);
  if (frm_cnt <= 0) {
    LOG_ERROR("__alsa_pcm_write(pData %p, dataSize %u) fail, ret %d\n", pData, dataSize, frm_cnt);
    return (-1);
  }
  *pNumFrames = frm_cnt;
  return 0;
}

static TInt __alsa_read_data(void *p_handle, TU8 *pData, TUint dataSize, TUint *pNumFrames, TU64 *pTimeStampUs)
{
  TInt err;
  snd_pcm_status_t *status;
  TInt frm_cnt;
  //snd_timestamp_t now, diff, trigger;
  DASSERT(dataSize >= 2048);      // make sure buffer can hold one chunck of data
  frm_cnt = __alsa_pcm_read((snd_pcm_t *) p_handle, pData, 1024);
  if (frm_cnt <= 0)
  { return (-1); }
  DASSERT(frm_cnt == 1024);
  *pNumFrames = frm_cnt;
  snd_pcm_status_alloca(&status);
  if ((err = snd_pcm_status((snd_pcm_t *) p_handle, status)) < 0) {
    LOG_FATAL("Get PCM status error: %s\n", snd_strerror(err));
    return (-2);
  }
  //snd_pcm_status_get_tstamp(status, &now);
  //snd_pcm_status_get_trigger_tstamp(status, &trigger);
  //timersub(&now, &mStartTimeStamp, &diff);
  //*pTimeStampUs = diff.tv_sec * 1000000 + diff.tv_usec;
  //pts to with 90khz unit
  //*pTimeStampUs = (mToTSample*TimeUnitDen_90khz)/mSampleRate;
  //mToTSample += frm_cnt;
  return 0;
}

#endif

void gfSetupAlsaAlContext(SFAudioAL *al)
{
#ifdef BUILD_MODULE_ALSA
  al->f_open_capture = __alsa_open_capture;
  al->f_open_playback = __alsa_open_playback;
  al->f_close = __alsa_close;
  al->f_start_capture = __alsa_start_capture;
  al->f_render_data = __alsa_render_data;
  al->f_read_data = __alsa_read_data;
#else
  LOG_FATAL("not compile alsa\n");
#endif
}


