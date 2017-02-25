/*******************************************************************************
 * audio_alsa.cpp
 *
 * History:
 * 2013/5/07 - [Zhi He] create file
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

#include <alsa/asoundlib.h>

#include "common_config.h"
#include "common_types.h"
#include "common_osal.h"
#include "common_utils.h"
#include "common_log.h"

#include "common_base.h"

#include "media_mw_if.h"

#include "audio_platform_interface.h"
#include "audio_alsa.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN

IAudioHAL *gfCreateAlsaAuidoHAL(const volatile SPersistMediaConfig *p_config)
{
    IAudioHAL *hal = CAudioALSA::Create(p_config);

    if (hal) {
        return hal;
    } else {
        LOG_FATAL("new CAudioALSA() fail\n");
    }

    return NULL;
}


#ifndef timersub
#define timersub(a, b, result) \
    do { \
        (result)->tv_sec = (a)->tv_sec - (b)->tv_sec; \
        (result)->tv_usec = (a)->tv_usec - (b)->tv_usec; \
        if ((result)->tv_usec < 0) { \
            --(result)->tv_sec; \
            (result)->tv_usec += 1000000; \
        } \
    } while (0)
#endif

#ifndef timermsub
#define timermsub(a, b, result) \
    do { \
        (result)->tv_sec = (a)->tv_sec - (b)->tv_sec; \
        (result)->tv_nsec = (a)->tv_nsec - (b)->tv_nsec; \
        if ((result)->tv_nsec < 0) { \
            --(result)->tv_sec; \
            (result)->tv_nsec += 1000000000L; \
        } \
    } while (0)
#endif

IAudioHAL *CAudioALSA::Create(const volatile SPersistMediaConfig *p_config)
{
    CAudioALSA *result = new CAudioALSA(p_config);
    if (result && result->Construct() != EECode_OK) {
        LOG_FATAL("NO memory\n");
        delete(result);
        result = NULL;
    }
    return result;
}

CAudioALSA::CAudioALSA(const volatile SPersistMediaConfig *p_config)
    : inherited("CAudioALSA", 0)
    , mpAudioHandle(NULL)
    , mpLog(NULL)
    , mOpened(0)
    , mbNoWait(1)
    , mSampleRate(DDefaultAudioSampleRate)
    , mNumOfChannels(DDefaultAudioChannelNumber)
    , mChunkBytes(128000)
    , mBitsPerSample(16)
    , mBitsPerFrame(1024 * 16)
    , mChunkSize(1024)
    , mStream(SND_PCM_STREAM_PLAYBACK)
    , mPcmFormat(SND_PCM_FORMAT_S16_LE)
    , mToTSample(0)
{
}

EECode CAudioALSA::Construct()
{
    DSET_MODULE_LOG_CONFIG(ELogModuleAudioRenderer);

    return EECode_OK;
}

CAudioALSA::~CAudioALSA()
{
    pcmDeinit();
}

void CAudioALSA::Delete()
{
    delete this;
}

EECode CAudioALSA::OpenDevice(SAudioParams *pParam, TUint *pLatency, TUint *pAudiosize, TUint source_or_sink)
{
    snd_pcm_stream_t stream;
    snd_pcm_format_t format;
    EECode err = EECode_OK;

    DASSERT(pParam);

    LOGM_INFO("[audio parameters]: in CAudioALSA::OpenDevice\n");

    if (mOpened == false) {

        if (source_or_sink) {
            LOGM_INFO("CAudioALSA::OpenDevice(source_or_sink %d) stream = SND_PCM_STREAM_PLAYBACK\n", source_or_sink);
            stream = SND_PCM_STREAM_PLAYBACK;
        } else {
            LOGM_INFO("CAudioALSA::OpenDevice(source_or_sink %d) stream = SND_PCM_STREAM_CAPTURE\n", source_or_sink);
            stream = SND_PCM_STREAM_CAPTURE;
        }

        if (AudioSampleFMT_S16 == pParam->sample_format) {
            if (!pParam->is_big_endian) {
                LOGM_INFO("[audio parameters]: SND_PCM_FORMAT_S16_LE\n");
                format = SND_PCM_FORMAT_S16_LE;
            } else {
                LOGM_INFO("[audio parameters]: SND_PCM_FORMAT_S16_BE\n");
                format = SND_PCM_FORMAT_S16_BE;
            }
        } else {
            LOGM_FATAL("pParam->sample_format %d is not supported!\n", pParam->sample_format);
            return EECode_BadParam;
        }

        LOGM_INFO("[audio parameters]: pParam->sample_rate %d, pParam->channel_number %d\n", pParam->sample_rate, pParam->channel_number);

        err = pcmInit(stream);
        if (EECode_OK == err) {
            err = setParams(format, pParam->sample_rate, pParam->channel_number, pParam->frame_size);
            if (EECode_OK == err) {
                mOpened = true;
            } else {
                LOGM_FATAL("setParams(format %d, pParam->sample_rate %d, pParam->channel_number %d, pParam->frame_size %d) fail, ret %d, %s\n", format, pParam->sample_rate, pParam->channel_number, pParam->frame_size, err, gfGetErrorCodeString(err));
                return err;
            }
        } else {
            LOGM_FATAL("pcmInit() fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
            return err;
        }
    } else {
        LOGM_WARN("already opened\n");
    }

    return err;
}

EECode CAudioALSA::CloseDevice()
{
    pcmDeinit();
    return EECode_OK;
}

EECode CAudioALSA::Start()
{
    TInt err;
    snd_pcm_status_t *status;

    if (mStream == SND_PCM_STREAM_PLAYBACK) {
        return EECode_OK;
    }
    if ((err = snd_pcm_start(mpAudioHandle)) < 0) {
        LOG_FATAL("PCM start error: %s\n", snd_strerror(err));
        return EECode_Error;
    }

    snd_pcm_status_alloca(&status);

    if ((err = snd_pcm_status(mpAudioHandle, status)) < 0) {
        LOG_FATAL("Get PCM status error: %s\n", snd_strerror(err));
        return EECode_Error;
    }
    snd_pcm_status_get_trigger_tstamp(status, &mStartTimeStamp);

    return EECode_OK;
}

EECode CAudioALSA::RenderAudio(TU8 *pData, TUint dataSize, TUint *pNumFrames)
{
    TInt frm_cnt;
    //LOG_PRINTF("RenderAudio(%d)\n", dataSize);
    frm_cnt = pcmWrite(pData, dataSize * 8 / mBitsPerFrame);

    if (frm_cnt <= 0) {
        LOGM_ERROR("pcmWrite(pData %p, dataSize %u) fail, ret %d\n", pData, dataSize, frm_cnt);
        return EECode_Error;
    }

    /*
    AM_ASSERT(frm_cnt == mChunkSize);
    if (frm_cnt != mChunkSize) {
        LOG_FATAL("frm_cnt %d != mChunkSize %d, dataSize %d.\n", frm_cnt, mChunkSize, dataSize);
    }
    */

    *pNumFrames = frm_cnt;
    return EECode_OK;
}

EECode CAudioALSA::ReadStream(TU8 *pData, TUint dataSize, TUint *pNumFrames, TU64 *pTimeStampUs)
{
    TInt err;
    snd_pcm_status_t *status;
    TInt frm_cnt;
    snd_timestamp_t now, diff, trigger;

    DASSERT(dataSize >= mChunkSize * mBitsPerFrame / 8);      // make sure buffer can hold one chunck of data
    frm_cnt = pcmRead(pData, mChunkSize);

    if (frm_cnt <= 0)
    { return EECode_Error; }

    DASSERT(frm_cnt == (TInt)mChunkSize);

    *pNumFrames = frm_cnt;

    snd_pcm_status_alloca(&status);
    if ((err = snd_pcm_status(mpAudioHandle, status)) < 0) {
        LOG_FATAL("Get PCM status error: %s\n", snd_strerror(err));
        return EECode_Error;
    }

    snd_pcm_status_get_tstamp(status, &now);
    snd_pcm_status_get_trigger_tstamp(status, &trigger);

    timersub(&now, &mStartTimeStamp, &diff);
    *pTimeStampUs = diff.tv_sec * 1000000 + diff.tv_usec;

    //pts to with 90khz unit
    *pTimeStampUs = (mToTSample * TimeUnitDen_90khz) / mSampleRate;
    mToTSample += frm_cnt;

    //    printf("avail %d\n", snd_pcm_status_get_avail(status));
    //    printf("PCM status: avail %d, time %d:%d - %d:%d, trigger %d:%d, PTS %lld\n", snd_pcm_status_get_avail(status),
    //        now.tv_sec, now.tv_usec, diff.tv_sec, diff.tv_usec, trigger.tv_sec, trigger.tv_usec, *pTimeStampUs);
    return EECode_OK;
}

EECode CAudioALSA::pcmInit(snd_pcm_stream_t stream)
{
    TInt err;

    mStream = stream;
    err = snd_output_stdio_attach(&mpLog, stderr, 0);
    //AM_ASSERT(err >= 0);

    if (mStream == SND_PCM_STREAM_PLAYBACK) {
        //open default to support 1channel for playback.
        err = snd_pcm_open(&mpAudioHandle, "default", mStream, 0);
        if (err < 0) {
            LOG_FATAL("Capture audio open error: %s\n", snd_strerror(err));
            return EECode_Error;
        }
    } else {
        err = snd_pcm_open(&mpAudioHandle, "default", mStream, 0);
        if (err < 0) {
            LOGM_INFO("snd_pcm_opne by default failed! Use MICALL.\n");
            err = snd_pcm_open(&mpAudioHandle, "MICALL", mStream, 0);
        }
        if (err < 0) {
            LOG_FATAL("Capture audio open error: %s\n", snd_strerror(err));
            return EECode_Error;
        }
    }

    LOGM_INFO("Open %s Audio Device Successful\n",
              mStream == SND_PCM_STREAM_PLAYBACK ? "Playback" : "Capture");

    return EECode_OK;
}

EECode CAudioALSA::pcmDeinit()
{
    if (mpLog != NULL) {
        snd_output_close(mpLog);
        mpLog = NULL;
    }
    if (mpAudioHandle != NULL) {
        snd_pcm_close(mpAudioHandle);
        mpAudioHandle = NULL;
    }

    return EECode_OK;
}

EECode CAudioALSA::setParams(snd_pcm_format_t pcmFormat, TUint sampleRate, TUint numOfChannels, TUint frame_size)
{
    TInt err;

    snd_pcm_hw_params_t *params;
    snd_pcm_sw_params_t *swparams;
    snd_pcm_uframes_t buffer_size;
    TUint start_threshold, stop_threshold;

    //    TUint period_time = 0;
    TUint buffer_time = 0;

    mPcmFormat = pcmFormat;
    mSampleRate = sampleRate;
    mNumOfChannels = numOfChannels;

    snd_pcm_hw_params_alloca(&params);
    snd_pcm_sw_params_alloca(&swparams);

    err = snd_pcm_hw_params_any(mpAudioHandle, params);
    if (err < 0) {
        LOG_FATAL("Broken configuration for this PCM: no configurations available\n");
        return EECode_Error;
    }

    err = snd_pcm_hw_params_set_access(mpAudioHandle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    if (err < 0) {
        LOG_FATAL("Access type not available\n");
        return EECode_Error;
    }

    err = snd_pcm_hw_params_set_format(mpAudioHandle, params, mPcmFormat);
    if (err < 0) {
        LOG_FATAL("Sample format non available\n");
        return EECode_Error;
    }

    err = snd_pcm_hw_params_set_channels(mpAudioHandle, params, mNumOfChannels);
    if (err < 0) {
        LOG_FATAL("Channels count(%d) not available\n", mNumOfChannels);
        return EECode_Error;
    }

    err = snd_pcm_hw_params_set_rate_near(mpAudioHandle, params, &mSampleRate, 0);
    //AM_ASSERT(err >= 0);

    err = snd_pcm_hw_params_get_buffer_time_max(params, &buffer_time, 0);
    //AM_ASSERT(err >= 0);
    if (buffer_time > 500000)
    { buffer_time = 500000; }

    //    period_time = buffer_time / 4;

    // set period size to 1024 frames, to meet aac encoder requirement
    mChunkSize = frame_size;
    LOG_INFO("audio frame_size %d\n", frame_size);
    err = snd_pcm_hw_params_set_period_size(mpAudioHandle, params, mChunkSize, 0);

    //AM_ASSERT(err >= 0);
    //    err = snd_pcm_hw_params_set_period_time_near(mpAudioHandle, params, &period_time, 0);
    //    AM_ASSERT(err >= 0);

    err = snd_pcm_hw_params_set_buffer_time_near(mpAudioHandle, params, &buffer_time, 0);
    //AM_ASSERT(err >= 0);

    //snd_pcm_wait(mpAudioHandle, 1000);
    err = snd_pcm_hw_params(mpAudioHandle, params);
    if (err < 0) {
        LOG_FATAL("Unable to install hw params:\n");
        snd_pcm_hw_params_dump(params, mpLog);
        return EECode_Error;
    }

    snd_pcm_hw_params_get_period_size(params, &mChunkSize, 0);
    snd_pcm_hw_params_get_buffer_size(params, &buffer_size);
    if (mChunkSize == buffer_size) {
        LOG_FATAL("Can't use period equal to buffer size (%d == %d)\n",
                  (TInt)mChunkSize, (TInt)buffer_size);
        return EECode_Error;
    }

    snd_pcm_sw_params_current(mpAudioHandle, swparams);

    err = snd_pcm_sw_params_set_avail_min(mpAudioHandle, swparams, mChunkSize);

    /* round up to closest transfer boundary */

    if (mStream == SND_PCM_STREAM_PLAYBACK)
    { start_threshold = (buffer_size / mChunkSize) * mChunkSize; }
    else
    { start_threshold = 1; }

    err = snd_pcm_sw_params_set_start_threshold(mpAudioHandle, swparams, start_threshold);
    //AM_ASSERT(err >= 0);

    stop_threshold = buffer_size;

    err = snd_pcm_sw_params_set_stop_threshold(mpAudioHandle, swparams, stop_threshold);
    //AM_ASSERT(err >= 0);

    if (snd_pcm_sw_params(mpAudioHandle, swparams) < 0) {
        LOG_FATAL("unable to install sw params:\n");
        snd_pcm_sw_params_dump(swparams, mpLog);
        return EECode_Error;
    }

    if (VERBOSE)
    { snd_pcm_dump(mpAudioHandle, mpLog); }

    mBitsPerSample = snd_pcm_format_physical_width(mPcmFormat);
    mBitsPerFrame = mBitsPerSample * mNumOfChannels;
    mChunkBytes = mChunkSize * mBitsPerFrame / 8;

    //AM_WARNING("chunk_size = %d, chunk_bytes = %d, buffer_size = %d\n", (TInt)mChunkSize, (TInt)mChunkBytes, (TInt)buffer_size);
    LOGM_INFO("Support Pause? %d\n", snd_pcm_hw_params_can_pause(params));
    return EECode_OK;
}


// I/O error handler /
EECode CAudioALSA::xrun(snd_pcm_stream_t stream)
{
    snd_pcm_status_t *status;
    int err;

    snd_pcm_status_alloca(&status);
    if ((err = snd_pcm_status(mpAudioHandle, status)) < 0) {
        LOG_FATAL("status error: %s\n", snd_strerror(err));
        return EECode_Error;
    }

    if (snd_pcm_status_get_state(status) == SND_PCM_STATE_XRUN) {
        //AM_WARNING("%s!!!\n", stream == SND_PCM_STREAM_PLAYBACK ?
        //"Playback underrun":"Capture overrun");

        if ((err = snd_pcm_prepare(mpAudioHandle)) < 0) {
            LOG_FATAL("xrun: prepare error: %s", snd_strerror(err));
            return EECode_Error;
        }
        return EECode_OK;       // ok, data should be accepted again
    }

    if (snd_pcm_status_get_state(status) == SND_PCM_STATE_DRAINING) {
        LOGM_INFO("capture stream format change? attempting recover...\n");
        if ((err = snd_pcm_prepare(mpAudioHandle)) < 0) {
            LOG_FATAL("xrun(DRAINING): prepare error: %s\n", snd_strerror(err));
            return EECode_Error;
        }
        return EECode_OK;
    }
    LOG_FATAL("read/write error, state = %s\n", snd_pcm_state_name(snd_pcm_status_get_state(status)));
    return EECode_Error;
}

EECode CAudioALSA::suspend(void)
{
    int err;

    //AM_WARNING("Suspended. Trying resume. ");
    while ((err = snd_pcm_resume(mpAudioHandle)) == -EAGAIN)
    { ::sleep(1); } /* wait until suspend flag is released */
    if (err < 0) {
        //AM_WARNING("Failed. Restarting stream. ");
        if ((err = snd_pcm_prepare(mpAudioHandle)) < 0) {
            LOG_FATAL("suspend: prepare error: %s", snd_strerror(err));
            return EECode_Error;
        }
    }
    //AM_WARNING("Suspend Done.\n");
    return EECode_OK;
}

TInt CAudioALSA::pcmRead(TU8 *pData, TUint rcount)
{
    EECode err;
    TInt r;
    TU32 result = 0;
    TU32 count = rcount;

    if (count != mChunkSize) {
        count = mChunkSize;
    }

    while (count > 0) {
        r = snd_pcm_readi(mpAudioHandle, pData, count);

        if (r == -EAGAIN || (r >= 0 && (TUint)r < count)) {
            if (!mbNoWait)
            { snd_pcm_wait(mpAudioHandle, 100); }
        } else if (r == -EPIPE) {                   // an overrun occurred
            if ((err = xrun(SND_PCM_STREAM_CAPTURE)) != EECode_OK)
            { return -1; }
        } else if (r == -ESTRPIPE) {            // a suspend event occurred
            if ((err = suspend()) != EECode_OK)
            { return -1; }
        } else if (r < 0) {
            if (r == -EIO)
            { LOGM_INFO("-EIO error!\n"); }
            else if (r == -EINVAL)
            { LOGM_INFO("-EINVAL error!\n"); }
            else if (r == -EINTR)
            { LOGM_INFO("-EINTR error!\n"); }
            else
            { LOG_FATAL("Read error: %s(%d)\n", snd_strerror(r), r); }
            return -1;
        }
        if (r > 0) {
            result += r;
            count -= r;
            pData += r * mBitsPerFrame / 8;      // convert frame num to bytes
        }
    }

    return result;
}

TInt CAudioALSA::pcmWrite(TU8 *pData, TUint count)
{
    EECode err;
    TInt r;
    TInt result = 0;
    TUint count_bak = count;

    //if (count < mChunkSize) {
    //    snd_pcm_format_set_silence(mPcmFormat, pData + count * mBitsPerFrame/ 8, (mChunkSize - count) * mNumOfChannels);
    //    count = mChunkSize;
    //}
    while (count > 0) {
        r = snd_pcm_writei(mpAudioHandle, pData, count);
        if (r == -EAGAIN || (r >= 0 && (size_t)r < count)) {
            if (!mbNoWait)
            { snd_pcm_wait(mpAudioHandle, 100); }
        } else if (r == -EPIPE) {               // an overrun occurred
            if ((err = xrun(SND_PCM_STREAM_PLAYBACK)) != EECode_OK)
            { goto __restart_alsa; }
        } else if (r == -ESTRPIPE) {        // a suspend event occurred
            if ((err = suspend()) != EECode_OK)
            { goto __restart_alsa; }
        } else if (r == -EIO || r == -EBADFD) {
            //AM_WARNING("PcmWrite -- write error: %s\n", snd_strerror(r));
            goto __restart_alsa;
        } else if (r < 0) {
            //AM_WARNING("PcmWrite -- write error: %s\n", snd_strerror(r));
            if (r == -EINVAL)
            { LOGM_INFO("-EINVAL error!\n"); }
            else
            { LOGM_INFO("unknown error!\n"); }
            return -1;
        }
        if (r > 0) {
            result += r;
            count -= r;
            pData += r * mBitsPerFrame / 8;
        }
        continue;
__restart_alsa:
        //Somehow the stream is in a bad state. The driver probably has a bug
        //    and recovery(Xrun() or Suspend()) doesn't seem to handle this.
        ////AM_WARNING("PcmWrite -- write error: %s, restart \n", snd_strerror(r));
        pcmDeinit();
        pcmInit(mStream);
        setParams(mPcmFormat, mSampleRate, mNumOfChannels, mChunkSize);
        Start();
    }

    if ((TUint)result > count_bak) {
        result = count_bak;
    }
    return result;
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

