/*******************************************************************************
 * pulse_audio.h
 *
 * History:
 * 2014/02/19 - [Zhi He] create file
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

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>

#include <pthread.h>
#include <pulse/pulseaudio.h>

#include "common_config.h"
#include "common_types.h"
#include "common_osal.h"
#include "common_utils.h"
#include "common_log.h"

#include "common_base.h"

#include "media_mw_if.h"

#include "audio_platform_interface.h"
#include "pulse_audio.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
///////////////Pulse Audio Callback Function//////////////////////
static void context_state_cb(pa_context *c, void *userdata)
{
    SPulseAudioData *pa_data = (SPulseAudioData *)userdata;
    DASSERT(c);
    DASSERT(pa_data);

    switch (pa_context_get_state(c)) {
        case PA_CONTEXT_READY:
        case PA_CONTEXT_TERMINATED:
        case PA_CONTEXT_FAILED:
            pa_threaded_mainloop_signal(pa_data->mainloop, 0);
            break;

        case PA_CONTEXT_UNCONNECTED:
        case PA_CONTEXT_CONNECTING:
        case PA_CONTEXT_AUTHORIZING:
        case PA_CONTEXT_SETTING_NAME:
            break;
    }

    return;
}

static void stream_state_cb(pa_stream *s, void *userdata)
{
    SPulseAudioData *pa_data = (SPulseAudioData *)userdata;
    DASSERT(s);
    DASSERT(pa_data);

    switch (pa_stream_get_state(s)) {

        case PA_STREAM_READY:
        case PA_STREAM_FAILED:
        case PA_STREAM_TERMINATED:
            pa_threaded_mainloop_signal(pa_data->mainloop, 0);
            break;

        case PA_STREAM_UNCONNECTED:
        case PA_STREAM_CREATING:
            break;
    }
}

#if 0
static void stream_request_cb(pa_stream *s, size_t length, void *userdata)
{
    SPulseAudioData *pa_data = (SPulseAudioData *)userdata;
    DASSERT(pa_data);

    pa_threaded_mainloop_signal(pa_data->mainloop, 0);
}
#endif

static void stream_latency_update_cb(pa_stream *s, void *userdata)
{
    SPulseAudioData *pa_data = (SPulseAudioData *)userdata;

    DASSERT(pa_data);

    pa_threaded_mainloop_signal(pa_data->mainloop, 0);
}

static void success_cb(pa_stream *s, int success, void *userdata)
{
    SPulseAudioData *pa_data = (SPulseAudioData *)userdata;

    DASSERT(s);
    DASSERT(pa_data);

    //pa_data->operation_success = success;
    pa_threaded_mainloop_signal(pa_data->mainloop, 0);
}

static void server_info_cb(pa_context *c, const pa_server_info *i, void *userdata)
{
    SPulseAudioData *pa_data = (SPulseAudioData *)userdata;

    *(pa_data->channelmap) = i->channel_map;
    switch (pa_data->samplespec->channels) {
        case 1 :
            pa_channel_map_init_mono(pa_data->channelmap);
            break;
        case 2 :
            pa_channel_map_init_stereo(pa_data->channelmap);
            break;
        default:
            pa_channel_map_init_auto(pa_data->channelmap, pa_data->samplespec->channels, PA_CHANNEL_MAP_ALSA);
            break;
    }
    LOG_PRINTF("Audio Server Information:\n");
    LOG_PRINTF("          Server Version: %s\n",   i->server_version);
    LOG_PRINTF("          Server Name: %s\n",   i->server_name);
    LOG_PRINTF("          Default Source Name: %s\n",   i->default_source_name);
    LOG_PRINTF("          Default Sink Name: %s\n",   i->default_sink_name);
    LOG_PRINTF("          Host Name: %s\n",   i->host_name);
    LOG_PRINTF("          User Name: %s\n",   i->user_name);
    LOG_PRINTF("          Channels: %hhu\n", i->sample_spec.channels);
    LOG_PRINTF("          Rate: %u\n",   i->sample_spec.rate);
    LOG_PRINTF("          Frame Size: %u\n",   pa_frame_size(&i->sample_spec));
    LOG_PRINTF("          Sample Size: %u\n",   pa_sample_size(&i->sample_spec));
    LOG_PRINTF("          Def ChannelMap Channels: %hhu\n", i->channel_map.channels);
    LOG_PRINTF("          Cookie: %08x\n", i->cookie);
    LOG_PRINTF("Audio irmation:\n");
    LOG_PRINTF("          Channels: %hhu\n", pa_data->samplespec->channels);
    LOG_PRINTF("          Sample Rate: %u\n",   pa_data->samplespec->rate);
    LOG_PRINTF("          ChannelMap Channels: %hhu\n", pa_data->channelmap->channels);
    pa_threaded_mainloop_signal(pa_data->mainloop, 0);
}

static void stream_underflow_cb(pa_stream *p, void *userdata)
{
    SPulseAudioData *pa_data = (SPulseAudioData *)userdata;
#if 1
    if ((++pa_data->underflow_count) > 20) {
        pa_data->underflow_count = 0;
        LOG_PRINTF("Pulse Audio Underflow!!\n");
    }
#else
    if ((++pa_data->underflow_count) > 4 && (*(pa_data->latency) < 5000000)) {
        *(pa_data->latency) = *(pa_data->latency) * 2;
        pa_data->bufferattr->maxlength = pa_usec_to_bytes(*(pa_data->latency), pa_data->samplespec);
        pa_data->bufferattr->tlength = pa_usec_to_bytes(*(pa_data->latency), pa_data->samplespec);
        pa_stream_set_buffer_attr(pa_data->stream, (const pa_buffer_attr *)(pa_data->bufferattr), NULL, NULL);
        pa_data->underflow_count = 0;
        LOG_PRINTF("latency %u\n", *(pa_data->latency));
    }
#endif
}
/////////////////////////////////////////////////////////////
IAudioHAL *gfCreatePulseAuidoHAL(const volatile SPersistMediaConfig *p_config)
{
    IAudioHAL *hal = CPulseAudio::Create(p_config);

    if (hal) {
        return hal;
    } else {
        LOG_FATAL("new CPulseAudio() fail\n");
    }

    return NULL;
}

IAudioHAL *CPulseAudio::Create(const volatile SPersistMediaConfig *p_config)
{
    CPulseAudio *result = new CPulseAudio(p_config);
    if (result && result->Construct() != EECode_OK) {
        LOG_FATAL("NO memory\n");
        delete(result);
        result = NULL;
    }
    return result;
}

CPulseAudio::CPulseAudio(const volatile SPersistMediaConfig *p_config)
    : inherited("CPulseAudio", 0)
    , mpPaMainLoop(NULL)
    , mpPaContext(NULL)
    , mpPaStream(NULL)
    , mPaDirection(PA_STREAM_PLAYBACK)
    , mPaLatency(300000)
    , mpDataBuffer(NULL)
    , mDataSize(0)
    , mbOpened(false)
    , mbPaused(false)
{
    memset(&mPaChannelMap, 0, sizeof(mPaChannelMap));
    memset(&mPaSampleSpec, 0, sizeof(mPaSampleSpec));
    memset(&mPaBufferAttr, 0, sizeof(mPaBufferAttr));
    mPaData.channelmap = &mPaChannelMap;
    mPaData.samplespec = &mPaSampleSpec;
    mPaData.bufferattr = &mPaBufferAttr;
    mPaData.direction = &mPaDirection;
    mPaData.latency = &mPaLatency;
}

CPulseAudio::~CPulseAudio()
{
}

EECode CPulseAudio::Construct()
{
    DSET_MODULE_LOG_CONFIG(ELogModuleAudioRenderer);

    return EECode_OK;
}

void CPulseAudio::Delete()
{
    if (mpDataBuffer) {
        delete[] mpDataBuffer;
        mpDataBuffer = NULL;
        mDataSize = 0;
    }
    delete this;
}

EECode CPulseAudio::OpenDevice(SAudioParams *pParam, TUint *pLatency, TUint *pAudiosize, TUint source_or_sink)
{
    DASSERT(pParam);

    LOGM_INFO("[audio parameters]: in CPulseAudio::OpenDevice\n");

    if (DLikely(mbOpened == false)) {

        if (source_or_sink) {
        } else {
            LOGM_FATAL("now capture audio by pulse audio is not supported!\n");
            return EECode_BadParam;
        }

        if (AudioSampleFMT_S16 == pParam->sample_format) {
            if (!pParam->is_big_endian) {
                LOGM_INFO("[audio parameters]: PA_SAMPLE_S16LE\n");
                mPaSampleSpec.format = PA_SAMPLE_S16LE;
            } else {
                LOGM_INFO("[audio parameters]: PA_SAMPLE_S16BE\n");
                mPaSampleSpec.format = PA_SAMPLE_S16BE;
            }
        } else {
            LOGM_FATAL("pParam->sample_format %d is not supported!\n", pParam->sample_format);
            return EECode_BadParam;
        }

        LOGM_NOTICE("[audio parameters]: pParam->sample_rate %u, pParam->channel_number %u\n", pParam->sample_rate, pParam->channel_number);

        mPaSampleSpec.rate = pParam->sample_rate;
        mPaSampleSpec.channels = (TU8)pParam->channel_number;

#if 0
        mpPaSimpleAPI = pa_simple_new(NULL, "CPulseAudio", PA_STREAM_PLAYBACK, NULL, "playback", &pa_spec, NULL, NULL, &pa_error);
        if (!mpPaSimpleAPI) {
            LOGM_FATAL("pa_simple_new fail, error %s", pa_strerror(pa_error));
            return EECode_Error;
        }
#endif
        return paInitialize();
    } else {
        LOGM_WARN("already opened\n");
    }
    mbOpened = true;
    return EECode_OK;
}

EECode CPulseAudio::CloseDevice()
{
#if 0
    TInt pa_error;
    if (mOpened == true) {
        DASSERT(mpPaSimpleAPI);
        if (pa_simple_flush(mpPaSimpleAPI, &pa_error) != 0) {
            LOGM_WARN("pa_simple_flush fail, error %s\n", pa_strerror(pa_error));
        }
        pa_simple_free(mpPaSimpleAPI);
        mpPaSimpleAPI = NULL;
        mOpened = false;
    }
#endif
    if (DLikely(mbOpened == true)) {
        paFinalize();
        mbOpened = false;
    }
    return EECode_OK;
}

EECode CPulseAudio::RenderAudio(TU8 *pData, TUint dataSize, TUint *pNumFrames)
{
    size_t writable_size = 0;
    DASSERT((mDataSize + dataSize) < (mPaBufferAttr.maxlength + MAX_AUDIO_BUFFER_SIZE));
    memcpy(mpDataBuffer + mDataSize, pData, dataSize);
    mDataSize += dataSize;
    if (DUnlikely(!(writable_size = pa_stream_writable_size(mpPaStream)))) {
        usleep(60000);
        return EECode_OK;
    } else {
        return (mDataSize >= writable_size) ? paWrite(writable_size) : EECode_OK;
    }
}

EECode CPulseAudio::ReadStream(TU8 *pData, TUint dataSize, TUint *pNumFrames, TU64 *pTimeStamp)
{
    return EECode_OK;
}

EECode CPulseAudio::Start()
{
    return EECode_OK;
}

EECode CPulseAudio::Pause()
{
    return paCork(1);
}

EECode CPulseAudio::Resume()
{
    return paCork(0);
}

EECode CPulseAudio::Stop()
{
    return paDrain();
}

EECode CPulseAudio::paInitialize()
{
    TU8 bUnlock = false;
    TInt ret = 0;
    TU8 bBadState = false;

    do {
        if (DUnlikely((mpPaMainLoop = pa_threaded_mainloop_new()) == NULL)) {
            LOGM_ERROR("pa_threaded_mainloop_new fail!\n");
            break;
        }
        if (DUnlikely((mpPaContext = pa_context_new(pa_threaded_mainloop_get_api(mpPaMainLoop), "AudioPlayback")) == NULL)) {
            LOGM_ERROR("pa_context_new fail!\n");
            break;
        }

        mPaData.mainloop = mpPaMainLoop;
        mPaData.context = mpPaContext;
        pa_context_set_state_callback(mpPaContext, context_state_cb, &mPaData);

        if (DUnlikely(pa_context_connect(mpPaContext, NULL, PA_CONTEXT_NOFLAGS, NULL) < 0)) {
            LOGM_ERROR("pa_context_connect fail, error %s\n", pa_strerror(pa_context_errno(mpPaContext)));
            break;
        }

        pa_threaded_mainloop_lock(mpPaMainLoop);
        bUnlock = true;

        if (DUnlikely(pa_threaded_mainloop_start(mpPaMainLoop) < 0)) {
            LOGM_ERROR("pa_threaded_mainloop_start fail!\n");
            break;
        }

        bBadState = false;
        for (;;) {
            pa_context_state_t state;
            state = pa_context_get_state(mpPaContext);
            if (state == PA_CONTEXT_READY) { break; }
            if (DUnlikely(!(PA_CONTEXT_IS_GOOD(state)))) {
                LOGM_ERROR("pa context bad state, error %s\n", pa_strerror(pa_context_errno(mpPaContext)));
                bBadState = true;
                break;
            }
            /* Wait until the context is ready */
            pa_threaded_mainloop_wait(mpPaMainLoop);
        }
        if (DUnlikely(bBadState == true)) {
            break;
        }

        pa_operation *paOp = NULL;
        pa_operation_state_t opState;
        paOp = pa_context_get_server_info(mpPaContext, server_info_cb, &mPaData);

        while ((opState = pa_operation_get_state(paOp)) != PA_OPERATION_DONE) {
            if (DUnlikely(opState == PA_OPERATION_CANCELLED)) {
                LOGM_INFO("Get server info operation canceled!\n");
                break;
            }
            pa_threaded_mainloop_wait(mpPaMainLoop);
        }
        pa_operation_unref(paOp);

        if (DUnlikely((mpPaStream = pa_stream_new(mpPaContext, "AudioPlayback", &mPaSampleSpec, &mPaChannelMap)) == NULL)) {
            LOGM_ERROR("pa_stream_new fail, error %s\n", pa_strerror(pa_context_errno(mpPaContext)));
            break;
        }
        mPaData.stream = mpPaStream;

        mPaLatency = pa_bytes_to_usec(MAX_AUDIO_BUFFER_SIZE, &mPaSampleSpec) * 4;
        mPaBufferAttr.fragsize  = ((TU32) - 1);
        mPaBufferAttr.maxlength = pa_usec_to_bytes(mPaLatency, &mPaSampleSpec);
        mPaBufferAttr.minreq    = pa_usec_to_bytes(0, &mPaSampleSpec);
        mPaBufferAttr.prebuf    = ((uint32_t) - 1);
        mPaBufferAttr.tlength   = pa_usec_to_bytes(mPaLatency, &mPaSampleSpec);

        pa_stream_set_state_callback(mpPaStream, stream_state_cb, &mPaData);
        //pa_stream_set_read_callback(mpPaStream, stream_request_cb, &mPaData);
        //pa_stream_set_write_callback(mpPaStream, stream_request_cb, &mPaData);
        pa_stream_set_latency_update_callback(mpPaStream, stream_latency_update_cb, &mPaData);
        pa_stream_set_underflow_callback(mpPaStream, stream_underflow_cb, &mPaData);

        if (mPaDirection == PA_STREAM_PLAYBACK) {
            ret = pa_stream_connect_playback(mpPaStream, NULL, (const pa_buffer_attr *)&mPaBufferAttr,
                                             (pa_stream_flags)(PA_STREAM_INTERPOLATE_TIMING
                                                     | PA_STREAM_ADJUST_LATENCY
                                                     | PA_STREAM_AUTO_TIMING_UPDATE), NULL, NULL);
        } else {
            ret = pa_stream_connect_record(mpPaStream, NULL, NULL,
                                           (pa_stream_flags)(PA_STREAM_INTERPOLATE_TIMING
                                                   | PA_STREAM_ADJUST_LATENCY
                                                   | PA_STREAM_AUTO_TIMING_UPDATE));
        }
        if (DUnlikely(ret < 0)) {
            LOGM_ERROR("pa_stream_connect fail, error %s\n", pa_strerror(pa_context_errno(mpPaContext)));
            break;
        }

        bBadState = false;
        for (;;) {
            pa_stream_state_t state;
            state = pa_stream_get_state(mpPaStream);
            if (state == PA_STREAM_READY) { break; }
            if (DUnlikely(!PA_STREAM_IS_GOOD(state))) {
                LOGM_ERROR("pa stream bad state, error %s\n", pa_strerror(pa_context_errno(mpPaContext)));
                bBadState = true;
                break;
            }
            /* Wait until the stream is ready */
            pa_threaded_mainloop_wait(mpPaMainLoop);
        }
        if (DUnlikely(bBadState == true)) {
            break;
        }

        const pa_buffer_attr *attr = pa_stream_get_buffer_attr(mpPaStream);
        if (DLikely(attr)) {
            LOGM_INFO("Client request max length: %u\n"
                      "min require length: %u\n"
                      "pre buffer length: %u\n"
                      "target length: %u\n"
                      "Server returned max length: %u\n"
                      "min require length: %u\n"
                      "pre buffer length: %u\n"
                      "target length: %u\n",
                      mPaBufferAttr.maxlength, mPaBufferAttr.minreq,
                      mPaBufferAttr.prebuf, mPaBufferAttr.tlength,
                      attr->maxlength, attr->minreq, attr->prebuf, attr->tlength);
            memcpy(&mPaBufferAttr, attr, sizeof(mPaBufferAttr));
        }
        pa_threaded_mainloop_unlock(mpPaMainLoop);

        mpDataBuffer = new TU8[mPaBufferAttr.maxlength + MAX_AUDIO_BUFFER_SIZE];
        mDataSize = 0;

        return EECode_OK;
    } while (0);

    if (bUnlock == true) {
        pa_threaded_mainloop_unlock(mpPaMainLoop);
    }

    paFinalize();

    return EECode_Error;
}

void CPulseAudio::paFinalize()
{
    if (mpPaMainLoop) {
        pa_threaded_mainloop_stop(mpPaMainLoop);
    }
    if (mpPaStream) {
        pa_stream_unref(mpPaStream);
    }
    if (mpPaContext) {
        pa_context_disconnect(mpPaContext);
        pa_context_unref(mpPaContext);
    }
    if (mpPaMainLoop) {
        pa_threaded_mainloop_free(mpPaMainLoop);
    }

    return;
}

EECode CPulseAudio::paWrite(TUint size)
{
#if 0
    TInt pa_error;
    TInt ret = 0;

    if ((ret = pa_simple_write(mpPaSimpleAPI, pData, (size_t)dataSize, &pa_error)) < 0) {
        LOGM_ERROR("pa_simple_write fail, error %s\n", pa_strerror(pa_error));
        return ret;
    }

    if (last == true) {
        //Make sure that every single sample was played
        if ((ret = pa_simple_drain(mpPaSimpleAPI, &pa_error)) < 0) {
            LOGM_ERROR("pa_simple_drain fail, error %s\n", pa_strerror(pa_error));
            return ret;
        }
    }
#endif
    EECode err = EECode_OK;
    TU8 *bufferPtr = NULL;

    if (DUnlikely(mbPaused)) { Resume(); }

    pa_threaded_mainloop_lock(mpPaMainLoop);

    if (DUnlikely(0 != (pa_stream_begin_write(mpPaStream, (void **)&bufferPtr, &size)))) {
        LOGM_ERROR("pa_stream_begin_write fail!\n");
        err = EECode_Error;
    } else {
        memcpy(bufferPtr, mpDataBuffer, size);
        if ((mDataSize -= size) > 0) {
            memmove(mpDataBuffer, mpDataBuffer + size, mDataSize);
        }

        if (DUnlikely(0 > (pa_stream_write(mpPaStream, bufferPtr, size, NULL, 0LL, PA_SEEK_RELATIVE)))) {
            LOGM_ERROR("pa_stream_write fail\n");
            err = EECode_Error;
        }
    }

    pa_threaded_mainloop_unlock(mpPaMainLoop);

    return err;
}

EECode CPulseAudio::paDrain()
{
    DASSERT(mPaDirection == PA_STREAM_PLAYBACK);

    EECode err = EECode_OK;
    pa_operation *o = NULL;

    LOGM_INFO("paDrain\n");

    pa_threaded_mainloop_lock(mpPaMainLoop);

    do {
        if (DUnlikely(!(o = pa_stream_drain(mpPaStream, success_cb, &mPaData)))) {
            LOGM_ERROR("pa_stream_drain fail!\n");
            err = EECode_Error;
            break;
        }
        while (pa_operation_get_state(o) == PA_OPERATION_RUNNING) {
            pa_threaded_mainloop_wait(mpPaMainLoop);
        }
        pa_operation_unref(o);
    } while (0);

    pa_threaded_mainloop_unlock(mpPaMainLoop);

    return err;
}

EECode CPulseAudio::paCork(TInt isPause)
{
    EECode err = EECode_OK;
    pa_operation *o = NULL;

    if ((mbPaused && (!!isPause)) || ((!mbPaused) && (!isPause))) {
        return err;
    }

    LOGM_INFO("paCork, %s stream\n", isPause ? "pause" : "resume");

    pa_threaded_mainloop_lock(mpPaMainLoop);

    do {
        if (DUnlikely(!(o = pa_stream_cork(mpPaStream, isPause, NULL, NULL)))) {
            LOGM_ERROR("pa_stream_cork fail!\n");
            err = EECode_Error;
            break;
        }
        while (pa_operation_get_state(o) != PA_OPERATION_DONE) {
            pa_threaded_mainloop_wait(mpPaMainLoop);
        }
        pa_operation_unref(o);
    } while (0);

    pa_threaded_mainloop_unlock(mpPaMainLoop);

    mbPaused = isPause ? true : false;

    return err;
}
DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END
