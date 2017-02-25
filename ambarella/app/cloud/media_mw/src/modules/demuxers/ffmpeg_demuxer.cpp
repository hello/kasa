/*******************************************************************************
 * ffmpeg_demuxer.cpp
 *
 * History:
 *    2013/4/27 - [Zhi He] create file
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

#include "common_config.h"

#ifdef BUILD_MODULE_FFMPEG

#include "common_types.h"
#include "common_osal.h"
#include "common_utils.h"

#include "common_log.h"

#include "common_base.h"

#include "media_mw_if.h"
#include "media_mw_utils.h"

#include "framework_interface.h"
#include "mw_internal.h"
#include "dsp_platform_interface.h"
#include "streaming_if.h"
#include "modules_interface.h"
extern "C" {
#include "libavutil/avstring.h"
#include "libavformat/avformat.h"
}

#include "ffmpeg_demuxer.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

IDemuxer *gfCreateFFMpegDemuxerModule(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TUint index)
{
    return CFFMpegDemuxer::Create(pName, pPersistMediaConfig, pEngineMsgSink, index);
}

static EECode __release_av_packet(CIBuffer *pBuffer)
{
    DASSERT(pBuffer);
    if (pBuffer) {
        if (EBufferCustomizedContentType_FFMpegAVPacket == pBuffer->mCustomizedContentType) {
            AVPacket *pkt = (AVPacket *)(pBuffer->mpCustomizedContent);
            DASSERT(pkt);
            if (pkt != NULL) {
                av_free_packet(pkt);
            }
        } else if ((EBufferType_VideoExtraData == pBuffer->GetBufferType()) || (EBufferType_AudioExtraData == pBuffer->GetBufferType())) {
        } else {
            LOG_FATAL("why comes here? (EBufferCustomizedContentType_FFMpegAVPacket != pBuffer->mCustomizedContentType)?\n");
            return EECode_InternalLogicalBug;
        }
    } else {
        LOG_FATAL("NULL pBuffer!\n");
        return EECode_InternalLogicalBug;
    }

    return EECode_OK;
}

static StreamFormat __convert_ffmpeg_codec_id(AVCodecID id)
{
    switch (id) {
        case AV_CODEC_ID_H264:
            return StreamFormat_H264;
            break;
        case AV_CODEC_ID_HEVC:
            return StreamFormat_H265;
            break;
        case AV_CODEC_ID_AAC:
            return StreamFormat_AAC;
            break;
        case AV_CODEC_ID_AC3:
            return StreamFormat_AC3;
            break;
        case AV_CODEC_ID_MP2:
            return StreamFormat_MP2;
            break;
        case AV_CODEC_ID_MP3:
            return StreamFormat_MP3;
            break;
        default:
            LOG_ERROR("id 0x%08x, in StreamFormat_FFMpegCustomized\n", id);
            break;
    }

    return StreamFormat_FFMpegCustomized;
}

static AudioSampleFMT __convert_audio_sample_format_from_FFMpeg(enum AVSampleFormat audio_sample_format)
{
    switch (audio_sample_format) {
        case AV_SAMPLE_FMT_S16:
        case AV_SAMPLE_FMT_S16P:
            return AudioSampleFMT_S16;
            break;
        case AV_SAMPLE_FMT_U8:
        case AV_SAMPLE_FMT_U8P:
            return AudioSampleFMT_U8;
            break;
        case AV_SAMPLE_FMT_S32:
        case AV_SAMPLE_FMT_S32P:
            return AudioSampleFMT_S32;
            break;
        case AV_SAMPLE_FMT_FLT:
        case AV_SAMPLE_FMT_FLTP:
            return AudioSampleFMT_FLT;
            break;
        case AV_SAMPLE_FMT_DBL:
        case AV_SAMPLE_FMT_DBLP:
            return AudioSampleFMT_DBL;
            break;
        default:
            break;
    }

    LOG_FATAL("BAD AVSampleFormat %d\n", audio_sample_format);
    return AudioSampleFMT_NONE;
}

IDemuxer *CFFMpegDemuxer::Create(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink, TUint index)
{
    CFFMpegDemuxer *result = new CFFMpegDemuxer(pname, pPersistMediaConfig, pMsgSink, index);
    if (result && result->Construct() != EECode_OK) {
        delete result;
        result = NULL;
    }
    return result;
}

CObject *CFFMpegDemuxer::GetObject0() const
{
    return (CObject *) this;
}

CFFMpegDemuxer::CFFMpegDemuxer(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink, TUint index)
    : inherited(pname, index)
    , mpPersistMediaConfig(pPersistMediaConfig)
    , mpAVFormat(NULL)
    , mVideo(-1)
    , mAudio(-1)
    , mPTSVideo_Num(1)
    , mPTSVideo_Den(1)
    , mPTSAudio_Num(1)
    , mPTSAudio_Den(1)
    , mVideoFramerateNum(DDefaultVideoFramerateNum)
    , mVideoFramerateDen(DDefaultVideoFramerateDen)
    , mVideoFramerate(((float)DDefaultVideoFramerateNum) / (float)(DDefaultVideoFramerateDen))
    , mFakePts(0)
    , mpMsgSink(NULL)
    , mpCurOutputPin(NULL)
    , mpCurBufferPool(NULL)
    , mpVideoExtraData(NULL)
    , mVideoExtraDataSize(0)
    , mpAudioExtraData(NULL)
    , mAudioExtraDataSize(0)
    , mbSendVideoExtraData(0)
    , mbSendAudioExtraData(0)
    , mbIsBackward(0)
    , mfwbwFeedingRule(DecoderFeedingRule_NotValid)
    , mCurrentSpeed(1)
    , mCurrentSpeedFrac(0)
    , mEstimatedKeyFrameInterval(100000)
    , mBWLastPTS(0)
    , mbBWEOS(0)
    , mPlaybackLoopMode(0)
{
    for (TUint i = 0; i < EConstMaxDemuxerMuxerStreamNumber; i ++) {
        mpOutputPins[i] = NULL;
    }
    for (TUint i = 0; i < EConstMaxDemuxerMuxerStreamNumber; i ++) {
        mpBufferPool[i] = NULL;
    }
}

EECode CFFMpegDemuxer::Construct()
{
    DSET_MODULE_LOG_CONFIG(ELogModuleFFMpegDemuxer);
    //mConfigLogLevel = ELogLevel_Debug;
    //mConfigLogOption = DCAL_BITMASK(ELogOption_State) | DCAL_BITMASK(ELogOption_Resource) | DCAL_BITMASK(ELogOption_Flow);

    LOGM_RESOURCE("CFFMpegDemuxer Construct!\n");
    EECode err = inherited::Construct();

    mPersistEOSBuffer.SetBufferType(EBufferType_FlowControl_EOS);
    mPersistPauseBuffer.SetBufferType(EBufferType_FlowControl_Pause);
    mPersistResumeBuffer.SetBufferType(EBufferType_FlowControl_Resume);
    mPersistFinalizeFileBuffer.SetBufferType(EBufferType_FlowControl_FinalizeFile);

    if (err != EECode_OK) {
        LOGM_FATAL("inherited::Construct() fail, ret %d\n", err);
        return err;
    }

    return EECode_OK;
}

EECode CFFMpegDemuxer::ReconnectServer()
{
    //LOGM_FATAL("TO DO\n");
    return EECode_OK;
}

EECode CFFMpegDemuxer::openUrl(TChar *url)
{
    TInt ret = 0;
    TInt i = 0;
    AVCodecContext *avctx = NULL;

    LOGM_INFO("openUrl(url = %s)\n", url);
    av_register_all();

    int rval = avformat_open_input(&mpAVFormat, url, NULL, NULL);
    if (rval < 0) {
        LOGM_ERROR("does not recognize %s\n", url);
        return EECode_Error;
    }

    //ret = av_find_stream_info(mpAVFormat);
    ret = avformat_find_stream_info(mpAVFormat, NULL);
    if (ret < 0) {
        LOGM_ERROR("av_find_stream_info fail, ret %d\n", ret);
        return EECode_Error;
    }

    memset(&mStreamCodecInfos, 0x0, sizeof(mStreamCodecInfos));

    DASSERT(mpAVFormat);
    for (i = 0; i < (TInt)mpAVFormat->nb_streams; i++) {
        avctx = mpAVFormat->streams[i]->codec;
        if (AVMEDIA_TYPE_VIDEO == avctx->codec_type) {
            mVideo = i;
            mStreamCodecInfos.info[i].codec_id = avctx->codec_id;
            mStreamCodecInfos.info[i].stream_type = StreamType_Video;
            mStreamCodecInfos.info[i].stream_format = __convert_ffmpeg_codec_id(avctx->codec_id);
            mStreamCodecInfos.info[i].spec.video.pic_width = avctx->width;
            mStreamCodecInfos.info[i].spec.video.pic_height = avctx->height;
            mStreamCodecInfos.info[i].spec.video.bitrate = avctx->bit_rate;

            if (avctx->extradata) {
                if (DUnlikely(mpVideoExtraData)) {
                    DDBG_FREE(mpVideoExtraData, "DMVE");
                    mpVideoExtraData = NULL;
                    mVideoExtraDataSize = 0;
                }
                //DASSERT(!mpVideoExtraData);
                //DASSERT(!mVideoExtraDataSize);

                mVideoExtraDataSize = avctx->extradata_size;
                LOGM_INFO("video extra data size %d, first 8 bytes: 0x%02x 0x%02x 0x%02x 0x%02x, 0x%02x 0x%02x 0x%02x 0x%02x\n", avctx->extradata_size,
                          avctx->extradata[0], avctx->extradata[1], avctx->extradata[2], avctx->extradata[3], \
                          avctx->extradata[4], avctx->extradata[5], avctx->extradata[6], avctx->extradata[7]);
                mpVideoExtraData = (TU8 *) DDBG_MALLOC(mVideoExtraDataSize, "DMVE");
                if (mpVideoExtraData) {
                    memcpy(mpVideoExtraData, avctx->extradata, avctx->extradata_size);
                } else {
                    LOGM_FATAL("NO memory\n");
                    return EECode_NoMemory;
                }
            } else {
                LOGM_ERROR("NULL avctx->extradata\n");
            }
        } else if (AVMEDIA_TYPE_AUDIO == avctx->codec_type) {
            mAudio = i;

            mStreamCodecInfos.info[i].codec_id = avctx->codec_id;
            mStreamCodecInfos.info[i].stream_type = StreamType_Audio;
            mStreamCodecInfos.info[i].stream_format = __convert_ffmpeg_codec_id(avctx->codec_id);
            mStreamCodecInfos.info[i].spec.audio.sample_rate = avctx->sample_rate;
            mStreamCodecInfos.info[i].spec.audio.sample_format = __convert_audio_sample_format_from_FFMpeg(avctx->sample_fmt);
            mStreamCodecInfos.info[i].spec.audio.channel_number = avctx->channels;
            mStreamCodecInfos.info[i].spec.audio.channel_layout = avctx->channel_layout;
            mStreamCodecInfos.info[i].spec.audio.bitrate = avctx->bit_rate;
            mStreamCodecInfos.info[i].spec.audio.codec_format = avctx->codec_id;
            mStreamCodecInfos.info[i].spec.audio.customized_codec_type = 0;//to do

            if (avctx->extradata) {
                if (DUnlikely(mpAudioExtraData)) {
                    DDBG_FREE(mpAudioExtraData, "DMAE");
                    mpAudioExtraData = NULL;
                    mAudioExtraDataSize = 0;
                }
                //DASSERT(!mpAudioExtraData);
                //DASSERT(!mAudioExtraDataSize);

                mAudioExtraDataSize = avctx->extradata_size;
                LOGM_INFO("audio extra data size %d, first 2 bytes: 0x%02x 0x%02x\n", avctx->extradata_size,
                          avctx->extradata[0], avctx->extradata[1]);
                mpAudioExtraData = (TU8 *) DDBG_MALLOC(mAudioExtraDataSize, "DMAE");
                if (mpAudioExtraData) {
                    memcpy(mpAudioExtraData, avctx->extradata, avctx->extradata_size);
                } else {
                    LOGM_FATAL("NO memory\n");
                    return EECode_NoMemory;
                }
            } else {
                LOGM_ERROR("NULL avctx->extradata\n");
            }
        } else {
            LOGM_WARN("media stream type %d, TODO\n", avctx->codec_type);
        }
    }

    if (mVideo < 0 && mAudio < 0) {
        LOGM_ERROR("No video, no audio, Check me!\n");
        return EECode_Error;
    }
    LOGM_INFO("media info: video[%d], audio[%d].\n", mVideo, mAudio);

    av_dump_format(mpAVFormat, 0, url, 0);

    updatePTSConvertor();

    return EECode_OK;
}

void CFFMpegDemuxer::closeUrl()
{
    if (mpAVFormat) {
        avformat_close_input(&mpAVFormat);
        mpAVFormat = NULL;
    }
}

void CFFMpegDemuxer::Delete()
{
    if (mpWorkQ) {
        LOGM_INFO("before send stop.\n");
        mpWorkQ->SendCmd(ECMDType_ExitRunning);
        LOGM_INFO("after send stop.\n");
    }

    closeUrl();

    if (mpVideoExtraData) {
        DDBG_FREE(mpVideoExtraData, "DMVE");
        mpVideoExtraData = NULL;
        mVideoExtraDataSize = 0;
    }

    if (mpAudioExtraData) {
        DDBG_FREE(mpAudioExtraData, "DMAE");
        mpAudioExtraData = NULL;
        mAudioExtraDataSize = 0;
    }

    if (mpBufferPool[EDemuxerVideoOutputPinIndex]) {
        mpBufferPool[EDemuxerVideoOutputPinIndex]->AddBufferNotifyListener(NULL);
    }

    if (mpBufferPool[EDemuxerAudioOutputPinIndex]) {
        mpBufferPool[EDemuxerAudioOutputPinIndex]->AddBufferNotifyListener(NULL);
    }

    inherited::Delete();
}

EECode CFFMpegDemuxer::SetupOutput(COutputPin *p_output_pins[], CBufferPool *p_bufferpools[],  IMemPool *p_mempools[], IMsgSink *p_msg_sink)
{
    //debug assert
    DASSERT(!mpBufferPool[EDemuxerVideoOutputPinIndex]);
    DASSERT(!mpBufferPool[EDemuxerAudioOutputPinIndex]);
    DASSERT(!mpOutputPins[EDemuxerVideoOutputPinIndex]);
    DASSERT(!mpOutputPins[EDemuxerAudioOutputPinIndex]);
    DASSERT(!mpMsgSink);

    mpOutputPins[EDemuxerVideoOutputPinIndex] = (COutputPin *)p_output_pins[EDemuxerVideoOutputPinIndex];
    mpOutputPins[EDemuxerAudioOutputPinIndex] = (COutputPin *)p_output_pins[EDemuxerAudioOutputPinIndex];
    mpBufferPool[EDemuxerVideoOutputPinIndex] = p_bufferpools[EDemuxerVideoOutputPinIndex];
    mpBufferPool[EDemuxerAudioOutputPinIndex] = p_bufferpools[EDemuxerAudioOutputPinIndex];

    mpMsgSink = p_msg_sink;

    if (mpOutputPins[EDemuxerVideoOutputPinIndex] && mpBufferPool[EDemuxerVideoOutputPinIndex]) {
        LOGM_INFO("before CRingMemPool::Create() for video\n");
        mpBufferPool[EDemuxerVideoOutputPinIndex]->SetReleaseBufferCallBack(__release_av_packet);
        mpBufferPool[EDemuxerVideoOutputPinIndex]->AddBufferNotifyListener((IEventListener *)this);
        //mpMemPool[EDemuxerVideoOutputPinIndex] = CRingMemPool::Create(4 * 1024 * 1024);
    }

    if (mpOutputPins[EDemuxerAudioOutputPinIndex] && mpBufferPool[EDemuxerAudioOutputPinIndex]) {
        mpBufferPool[EDemuxerAudioOutputPinIndex]->SetReleaseBufferCallBack(__release_av_packet);
        mpBufferPool[EDemuxerAudioOutputPinIndex]->AddBufferNotifyListener((IEventListener *)this);
        LOGM_INFO("before CFixedMemPool::Create() for audio\n");
        //mpMemPool[EDemuxerAudioOutputPinIndex] = CFixedMemPool::Create(1024, 128);
    }

    return EECode_OK;
}

EECode CFFMpegDemuxer::SetupContext(TChar *url, void *p_agent, TU8 priority, TU32 request_receive_buffer_size, TU32 request_send_buffer_size)
{
    EECode err;

    if (url) {
        err = openUrl(url);
        DASSERT_OK(err);
        if (EECode_OK != err) {
            LOGM_ERROR("openUrl(%s) fail, return %d\n", url, err);
            return err;
        }

        DASSERT(mpWorkQ);

        if (mpWorkQ) {
            LOGM_FLOW("before mpWorkQ->SendCmd(ECMDType_StartRunning)...\n");
            err = mpWorkQ->SendCmd(ECMDType_StartRunning);
            LOGM_FLOW("after mpWorkQ->SendCmd(ECMDType_StartRunning), ret %d\n", err);
            DASSERT_OK(err);
        } else {
            LOG_FATAL("NULL mpWorkQ?\n");
            return EECode_InternalLogicalBug;
        }

    } else {
        LOGM_FATAL("NULL input in CFFMpegDemuxer::SetupContext\n");
        return EECode_BadParam;
    }

    return err;
}

EECode CFFMpegDemuxer::DestroyContext()
{
    EECode err = EECode_OK;

    DASSERT(mpWorkQ);

    if (mpWorkQ) {
        LOGM_FLOW("before mpWorkQ->SendCmd(ECMDType_ExitRunning)...\n");
        err = mpWorkQ->SendCmd(ECMDType_ExitRunning);
        LOGM_FLOW("after mpWorkQ->SendCmd(ECMDType_ExitRunning), ret %d\n", err);
        DASSERT_OK(err);
    } else {
        LOG_FATAL("NULL mpWorkQ?\n");
        err = EECode_InternalLogicalBug;
    }

    closeUrl();

    return err;
}

CFFMpegDemuxer::~CFFMpegDemuxer()
{
    if (mpVideoExtraData) {
        free(mpVideoExtraData);
        mpVideoExtraData = NULL;
        mVideoExtraDataSize = 0;
    }

    if (mpAudioExtraData) {
        free(mpAudioExtraData);
        mpAudioExtraData = NULL;
        mAudioExtraDataSize = 0;
    }
}

EECode CFFMpegDemuxer::Start()
{
    EECode err = EECode_OK;

    DASSERT(mpWorkQ);

    LOGM_FLOW("before mpWorkQ->SendCmd(ECMDType_Start)...\n");
    err = mpWorkQ->SendCmd(ECMDType_Start);
    LOGM_FLOW("after mpWorkQ->SendCmd(ECMDType_Start), ret %d\n", err);

    DASSERT_OK(err);

    return err;
}

EECode CFFMpegDemuxer::Stop()
{
    EECode err = EECode_OK;

    DASSERT(mpWorkQ);

    LOGM_FLOW("before mpWorkQ->SendCmd(ECMDType_Stop)...\n");
    err = mpWorkQ->SendCmd(ECMDType_Stop);
    LOGM_FLOW("after mpWorkQ->SendCmd(ECMDType_Stop), ret %d\n", err);

    DASSERT_OK(err);

    return err;
}

EECode CFFMpegDemuxer::Pause()
{
    EECode err = EECode_OK;

    DASSERT(mpWorkQ);

    LOGM_FLOW("before mpWorkQ->PostMsg(ECMDType_Pause)...\n");
    err = mpWorkQ->PostMsg(ECMDType_Pause);
    LOGM_FLOW("after mpWorkQ->PostMsg(ECMDType_Pause), ret %d\n", err);

    DASSERT_OK(err);

    return err;
}

EECode CFFMpegDemuxer::Resume()
{
    EECode err = EECode_OK;

    DASSERT(mpWorkQ);

    LOGM_FLOW("before mpWorkQ->PostMsg(ECMDType_Resume)...\n");
    err = mpWorkQ->PostMsg(ECMDType_Resume);
    LOGM_FLOW("after mpWorkQ->PostMsg(ECMDType_Resume), ret %d\n", err);

    DASSERT_OK(err);

    return err;
}

EECode CFFMpegDemuxer::Flush()
{
    EECode err = EECode_OK;

    DASSERT(mpWorkQ);

    LOGM_FLOW("before mpWorkQ->SendCmd(ECMDType_Flush)...\n");
    err = mpWorkQ->SendCmd(ECMDType_Flush);
    LOGM_FLOW("after mpWorkQ->SendCmd(ECMDType_Flush), ret %d\n", err);

    DASSERT_OK(err);

    return err;
}

EECode CFFMpegDemuxer::ResumeFlush()
{
    EECode err = EECode_OK;

    DASSERT(mpWorkQ);

    LOGM_FLOW("before mpWorkQ->SendCmd(ECMDType_ResumeFlush)...\n");
    err = mpWorkQ->SendCmd(ECMDType_ResumeFlush);
    LOGM_FLOW("after mpWorkQ->SendCmd(ECMDType_ResumeFlush), ret %d\n", err);

    DASSERT_OK(err);

    return err;
}

EECode CFFMpegDemuxer::Suspend()
{
    EECode err = EECode_OK;

    /*    DASSERT(mpWorkQ);

        LOGM_FLOW("before mpWorkQ->SendCmd(ECMDType_Flush)...\n");
        err = mpWorkQ->SendCmd(ECMDType_Flush);
        LOGM_FLOW("after mpWorkQ->SendCmd(ECMDType_Flush), ret %d\n", err);

        DASSERT_OK(err);*/

    return err;
}

EECode CFFMpegDemuxer::Purge()
{
    EECode err = EECode_OK;

    /*    DASSERT(mpWorkQ);

        LOGM_FLOW("before mpWorkQ->SendCmd(ECMDType_Flush)...\n");
        err = mpWorkQ->SendCmd(ECMDType_Flush);
        LOGM_FLOW("after mpWorkQ->SendCmd(ECMDType_Flush), ret %d\n", err);

        LOGM_FLOW("before mpWorkQ->SendCmd(ECMDType_ResumeFlush)...\n");
        err = mpWorkQ->SendCmd(ECMDType_ResumeFlush);
        LOGM_FLOW("after mpWorkQ->SendCmd(ECMDType_ResumeFlush), ret %d\n", err);

        DASSERT_OK(err);*/

    return err;
}

EECode CFFMpegDemuxer::SetPbRule(TU8 direction, TU8 feeding_rule, TU16 speed)
{
    EECode err = EECode_OK;
    SCMD cmd;
    DASSERT(mpWorkQ);

    cmd.code = ECMDType_UpdatePlaybackSpeed;
    cmd.res32_1 = direction;
    cmd.res64_1 = feeding_rule;
    cmd.res64_2 = speed;
    err = mpWorkQ->PostMsg(cmd);

    return err;
}

EECode CFFMpegDemuxer::SetPbLoopMode(TU32 *p_loop_mode)
{
    EECode err = EECode_OK;

    DASSERT(mpWorkQ);
    DASSERT(p_loop_mode);

    LOGM_FLOW("before mpWorkQ->PostMsg(ECMDType_UpdatePlaybackLoopMode)...\n");
    err = mpWorkQ->PostMsg(ECMDType_UpdatePlaybackLoopMode, (void *)p_loop_mode);
    LOGM_FLOW("after mpWorkQ->PostMsg(ECMDType_UpdatePlaybackLoopMode), ret %d\n", err);

    DASSERT_OK(err);

    return err;
}

EECode CFFMpegDemuxer::EnableVideo(TU32 enable)
{
    LOGM_ERROR("not implemented in this module.\n");
    return EECode_NoImplement;
}

EECode CFFMpegDemuxer::EnableAudio(TU32 enable)
{
    LOGM_ERROR("not implemented in this module.\n");
    return EECode_NoImplement;
}

EECode CFFMpegDemuxer::processCmd(SCMD &cmd)
{
    EECode err = EECode_OK;

    switch (cmd.code) {

        case ECMDType_ExitRunning:
            mbRun = 0;
            break;

        case ECMDType_Start:
            DASSERT(EModuleState_WaitCmd == msState);
            msState = EModuleState_Idle;
            LOGM_FLOW("ECMDType_Start comes\n");
            CmdAck(EECode_OK);
            break;

        case ECMDType_Stop:
            mbTobeStopped = 1;
            CmdAck(EECode_OK);
            break;

        case ECMDType_Pause:
            mbPaused = 1;
            break;

        case ECMDType_Resume:
            if (EModuleState_Pending == msState) {
                msState = EModuleState_Idle;
            } else if (EModuleState_Completed == msState) {
                msState = EModuleState_Idle;
            } else if (EModuleState_Stopped == msState) {
                msState = EModuleState_Idle;
            }
            mbPaused = 0;
            break;

        case ECMDType_Flush:
            if (EModuleState_HasInputData == msState) {
                av_free_packet(&mPkt);
            }
            msState = EModuleState_Pending;
            CmdAck(EECode_OK);
            break;

        case ECMDType_ResumeFlush:
            DASSERT(EModuleState_Pending == msState);
            msState = EModuleState_Idle;
            CmdAck(EECode_OK);
            break;

        case ECMDType_NotifyBufferRelease:
            if (mpCurBufferPool && mpCurBufferPool->GetFreeBufferCnt() > 0 && msState == EModuleState_HasInputData) {
                msState = EModuleState_Running;
            }
            break;

        case ECMDType_UpdatePlaybackSpeed: {
                SPbFeedingRule *p_feedingrule = (SPbFeedingRule *)cmd.pExtra;
                LOGM_NOTICE("demuxer module pb speed, %s, %hu, rule=%hu\n", p_feedingrule->direction ? "bw" : "fw", p_feedingrule->speed, p_feedingrule->feeding_rule);
                mbIsBackward = p_feedingrule->direction;
                mfwbwFeedingRule = p_feedingrule->feeding_rule;
                mCurrentSpeed = p_feedingrule->speed;
                if (!mbIsBackward) {
                    //clear bw related field
                    mEstimatedKeyFrameInterval = 100000;
                    mBWLastPTS = 0;
                    mbBWEOS = 0;
                }
            }
            break;

        case ECMDType_UpdatePlaybackLoopMode: {
                TU32 *p_loop_mode = (TU32 *)cmd.pExtra;
                if (*p_loop_mode != mPlaybackLoopMode) {
                    mPlaybackLoopMode = *p_loop_mode;
                    LOGM_NOTICE("demuxer module update playback loop mode to %u\n", mPlaybackLoopMode);
                } else {
                    LOGM_NOTICE("demuxer module playback loop mode %u, not changed.\n", mPlaybackLoopMode);
                }
            }
            break;

        default:
            LOGM_ERROR("wrong cmd %d.\n", cmd.code);
            break;
    }

    return err;
}

void CFFMpegDemuxer::OnRun()
{
    TInt ret = 0;
    SCMD cmd;
    CIBuffer *buffer = NULL;
    EBufferType buffer_type = EBufferType_Invalid;
    TU8 video_frame_type = EPredefinedPictureType_IDR;
    mbRun = 1;

    DASSERT(1 == mbRun);
    CmdAck(EECode_OK);

    msState = EModuleState_WaitCmd;

    //send extra data first
    if (mpOutputPins[EDemuxerVideoOutputPinIndex] && (0 <= mVideo)) {
        AVCodecContext *codec = NULL;

        mpOutputPins[EDemuxerVideoOutputPinIndex]->AllocBuffer(buffer);

        DASSERT(buffer);
        DASSERT(mpVideoExtraData);
        DASSERT(mVideoExtraDataSize);

        buffer->SetDataPtr(mpVideoExtraData);
        buffer->SetDataSize(mVideoExtraDataSize);
        buffer->SetBufferType(EBufferType_VideoExtraData);
        buffer->SetBufferFlags(CIBuffer::SYNC_POINT);

        codec = mpAVFormat->streams[mVideo]->codec;
        DASSERT(codec);

        buffer->mVideoOffsetX = 0;
        buffer->mVideoOffsetY = 0;
        buffer->mVideoWidth = codec->width;
        buffer->mVideoHeight = codec->height;
        buffer->mVideoBitrate = codec->bit_rate;
        buffer->mContentFormat = __convert_ffmpeg_codec_id(codec->codec_id);
        buffer->mCustomizedContentFormat = codec->codec_id;

        postVideoSizeMsg(buffer->mVideoWidth, buffer->mVideoHeight);

        if (DLikely(!mpPersistMediaConfig->try_requested_video_framerate)) {
            buffer->mVideoFrameRateNum = mpAVFormat->streams[mVideo]->r_frame_rate.num;
            buffer->mVideoFrameRateDen = mpAVFormat->streams[mVideo]->r_frame_rate.den;
        } else {
            buffer->mVideoFrameRateNum = mpPersistMediaConfig->requested_video_framerate_num;
            buffer->mVideoFrameRateDen = mpPersistMediaConfig->requested_video_framerate_den;
        }

        if (buffer->mVideoFrameRateNum && buffer->mVideoFrameRateDen) {
            mVideoFramerate = buffer->mVideoRoughFrameRate = (float)buffer->mVideoFrameRateNum / (float)buffer->mVideoFrameRateDen;
            mVideoFramerateDen = mVideoFramerateNum * buffer->mVideoFrameRateDen / buffer->mVideoFrameRateNum;
        } else {
            buffer->mVideoFrameRateNum = mVideoFramerateNum;
            buffer->mVideoFrameRateDen = mVideoFramerateDen;
            mVideoFramerate = buffer->mVideoRoughFrameRate = (float)buffer->mVideoFrameRateNum / (float)buffer->mVideoFrameRateDen;
        }

        buffer->mVideoSampleAspectRatioNum = codec->sample_aspect_ratio.num;
        buffer->mVideoSampleAspectRatioDen = codec->sample_aspect_ratio.den;

        LOGM_NOTICE("before send video extra data, %dx%d, rough framerate %f, mVideoFramerateDen %u\n", buffer->mVideoWidth, buffer->mVideoHeight, buffer->mVideoRoughFrameRate, mVideoFramerateDen);
        mpOutputPins[EDemuxerVideoOutputPinIndex]->SendBuffer(buffer);
        buffer = NULL;
    }

    if (mpOutputPins[EDemuxerAudioOutputPinIndex] && (0 <= mAudio)) {
        AVCodecContext *codec = NULL;

        mpOutputPins[EDemuxerAudioOutputPinIndex]->AllocBuffer(buffer);

        DASSERT(buffer);
        DASSERT(mpAudioExtraData);
        DASSERT(mAudioExtraDataSize);

        buffer->SetDataPtr(mpAudioExtraData);
        buffer->SetDataSize(mAudioExtraDataSize);
        buffer->SetBufferType(EBufferType_AudioExtraData);
        buffer->SetBufferFlags(CIBuffer::SYNC_POINT);

        codec = mpAVFormat->streams[mAudio]->codec;
        DASSERT(codec);

        buffer->mAudioChannelNumber = codec->channels;
        buffer->mAudioSampleRate = codec->sample_rate;
        buffer->mAudioFrameSize = codec->frame_size;
        buffer->mAudioSampleFormat = __convert_audio_sample_format_from_FFMpeg(codec->sample_fmt);

        buffer->mAudioBitrate = codec->bit_rate;

        buffer->mbAudioPCMChannelInterlave = 0;
        buffer->mpCustomizedContent = NULL;
        buffer->mCustomizedContentType = EBufferCustomizedContentType_Invalid;
        buffer->mContentFormat = __convert_ffmpeg_codec_id(codec->codec_id);
        buffer->mCustomizedContentFormat = codec->codec_id;

        LOGM_NOTICE("before send audio extra data, codec %d\n", buffer->mContentFormat);
        mpOutputPins[EDemuxerAudioOutputPinIndex]->SendBuffer(buffer);
        buffer = NULL;
    }

    while (mbRun) {

        LOGM_STATE("CFFMpegDemuxer::OnRun(), state(%s, %d)\n", gfGetModuleStateString(msState), msState);

        switch (msState) {

            case EModuleState_WaitCmd:
                mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
                processCmd(cmd);
                break;

            case EModuleState_Idle:
                if (mpWorkQ->PeekCmd(cmd)) {
                    processCmd(cmd);
                } else {
                    //bw case
                    if (mbIsBackward) {
                        if ((mBWLastPTS) && (mBWLastPTS >= mEstimatedKeyFrameInterval)) {
                            LOGM_NOTICE("bw, do seek %llu, %llu\n", mBWLastPTS, mEstimatedKeyFrameInterval);
                            AVRational bq = {0, 0}, cq = {0, 0};
                            TTime seekpts = 0;
                            EECode err = EECode_OK;
                            bq.num = 1;
                            bq.den = TimeUnitDen_90khz;
                            cq.num = 1;
                            cq.den = AV_TIME_BASE;
                            seekpts = av_rescale_q(mBWLastPTS - mEstimatedKeyFrameInterval, bq, cq);
                            err = DoSeek(seekpts);
                            DASSERT(EECode_OK == err);
                        }

                        LOGM_NOTICE("bw, start searching for video key frame...\n");
                        while (1) {
                            av_init_packet(&mPkt);
                            ret = av_read_frame(mpAVFormat, &mPkt);
                            if (ret < 0) {
                                LOGM_NOTICE("av_read_frame()<0, stream end? ret=%d.\n", ret);
                                av_free_packet(&mPkt);
                                msState = EModuleState_Completed;
                                sendFlowControlBuffer(EBufferType_FlowControl_EOS);
                                mbBWEOS = 1;
                                break;
                            }

                            if (mPkt.stream_index != mVideo) {
                                LOGM_INFO("bw, discard none video packet.\n");
                                av_free_packet(&mPkt);
                                continue;
                            }

                            if (!(mPkt.flags & AV_PKT_FLAG_KEY)) {
                                LOGM_INFO("bw, discard non-key video packet.\n");
                                av_free_packet(&mPkt);
                                continue;
                            }

                            break;
                        }
                        LOGM_NOTICE("bw, searching for video key frame DONE. mbBWEOS=%hu\n", mbBWEOS);

                        if (mbBWEOS) {
                            DASSERT(EModuleState_Completed == msState);
                            break;
                        }

                        msState = EModuleState_HasInputData;
                        break;
                    }

                    //fw case
                    av_init_packet(&mPkt);
                    ret = av_read_frame(mpAVFormat, &mPkt);
                    if (ret < 0) {
                        av_free_packet(&mPkt);
                        if (mPlaybackLoopMode) {
                            TTime time = 0;
                            Seek(time);
                        } else {
                            msState = EModuleState_Completed;
                            sendFlowControlBuffer(EBufferType_FlowControl_EOS);
                            LOGM_NOTICE("av_read_frame()<0, stream end? ret=%d.\n", ret);
                        }
                        break;
                    }

                    if (mPkt.stream_index == mVideo) {
                        mpCurBufferPool = mpBufferPool[EDemuxerVideoOutputPinIndex];
                        mpCurOutputPin = mpOutputPins[EDemuxerVideoOutputPinIndex];

                        if (!mpCurBufferPool || !mpCurOutputPin) {
                            av_free_packet(&mPkt);
                            break;
                        }

                        /*if (mPkt.pts >= 0) {
                            convertVideoPts(mPkt.pts);
                        } else {
                            convertVideoPts(mPkt.dts);
                        }*/

                        buffer_type = EBufferType_VideoES;

                        //tmp set here
                        if (mPkt.flags & AV_PKT_FLAG_KEY) {
                            video_frame_type = EPredefinedPictureType_IDR;//IDR is a concept of h264, use the name for keyframe of other video codec here
                            LOGM_DEBUG("video key frame comes\n");
                        } else {
                            video_frame_type = EPredefinedPictureType_P;
                            if (DecoderFeedingRule_IOnly == mfwbwFeedingRule) {
                                LOGM_DEBUG("mfwbwFeedingRule I Only now, non-keyframs dropped.\n");
                                av_free_packet(&mPkt);
                                break;
                            }
                        }

                    } else if (mPkt.stream_index == mAudio) {

                        if (1 != mCurrentSpeed || 0 != mCurrentSpeedFrac || 0 != mbIsBackward) { //dest speed is not fw 1x, disable audio
                            av_free_packet(&mPkt);
                            break;
                        }

                        mpCurBufferPool = mpBufferPool[EDemuxerAudioOutputPinIndex];
                        mpCurOutputPin = mpOutputPins[EDemuxerAudioOutputPinIndex];
                        if (!mpCurBufferPool || !mpCurOutputPin) {
                            av_free_packet(&mPkt);
                            break;
                        }

                        convertAudioPts(mPkt.pts);
                        buffer_type = EBufferType_AudioES;
                    } else {
                        av_free_packet(&mPkt);
                        break;
                    }

                    msState = EModuleState_HasInputData;
                }
                break;

            case EModuleState_HasInputData:
                DASSERT(mpCurBufferPool);
                if (mpCurBufferPool->GetFreeBufferCnt() > 0) {
                    //LOGM_VERBOSE(" mpCurBufferPool->GetFreeBufferCnt() %d\n", mpCurBufferPool->GetFreeBufferCnt());
                    msState = EModuleState_Running;
                } else {
                    mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
                    processCmd(cmd);
                }
                break;

            case EModuleState_Running:
                DASSERT(!buffer);
                buffer = NULL;

                if (!mpCurOutputPin->AllocBuffer(buffer)) {
                    LOGM_FATAL("AllocBuffer() fail? must have internal bugs.\n");
                    av_free_packet(&mPkt);
                    msState = EModuleState_Error;
                    break;
                }

                if (mPkt.stream_index == mVideo) {
                    mFakePts += mVideoFramerateDen;
                    buffer->SetBufferLinearPTS(mFakePts);
                    if (mPkt.pts >= 0) {
                        buffer->SetBufferPTS(convertVideoPts(mPkt.pts));
                        buffer->SetBufferNativePTS(mPkt.pts);
                    } else {
                        buffer->SetBufferPTS(DINVALID_VALUE_TAG_64);
                        buffer->SetBufferNativePTS(DINVALID_VALUE_TAG_64);
                    }
                    if (mPkt.dts >= 0) {
                        buffer->SetBufferDTS(convertVideoPts(mPkt.dts));
                        buffer->SetBufferNativeDTS(mPkt.dts);
                    } else {
                        buffer->SetBufferDTS(DINVALID_VALUE_TAG_64);
                        buffer->SetBufferNativeDTS(DINVALID_VALUE_TAG_64);
                    }
                } else if (mPkt.stream_index == mAudio) {
                    if (mPkt.pts >= 0) {
                        buffer->SetBufferPTS(convertAudioPts(mPkt.pts));
                        buffer->SetBufferNativePTS(mPkt.pts);
                    } else {
                        buffer->SetBufferPTS(DINVALID_VALUE_TAG_64);
                        buffer->SetBufferNativePTS(DINVALID_VALUE_TAG_64);
                    }

                    //TODO,
                    //      for rtsp-streaming, do not convert PTS
                    buffer->SetBufferPTS(mPkt.pts);

                    if (mPkt.dts >= 0) {
                        buffer->SetBufferDTS(convertAudioPts(mPkt.dts));
                        buffer->SetBufferNativeDTS(mPkt.dts);
                    } else {
                        buffer->SetBufferDTS(DINVALID_VALUE_TAG_64);
                        buffer->SetBufferNativeDTS(DINVALID_VALUE_TAG_64);
                    }
                } else {
                    LOGM_FATAL("BAD stream index %d, mVideo %d, mAudio %d\n", mPkt.stream_index, mVideo, mAudio);
                    break;
                }

                buffer->SetBufferType(buffer_type);
                buffer->SetDataSize((TUint)mPkt.size);
                if (!buffer->mpCustomizedContent) {
                    buffer->mpCustomizedContent = (void *) DDBG_MALLOC(sizeof(AVPacket), "DMPT");
                    DASSERT(buffer->mpCustomizedContent);
                    if (!buffer->mpCustomizedContent) {
                        LOGM_FATAL("NO Memory, request size %d\n", sizeof(AVPacket));
                        msState = EModuleState_Error;
                        break;
                    }
                } else {
                    DASSERT(EBufferCustomizedContentType_FFMpegAVPacket == buffer->mCustomizedContentType);
                }

                buffer->mCustomizedContentType = EBufferCustomizedContentType_FFMpegAVPacket;
                av_dup_packet(&mPkt);
                ::memcpy(buffer->mpCustomizedContent, &mPkt, sizeof(AVPacket));

                buffer->SetDataPtr((TU8 *)mPkt.data);
                buffer->SetDataSize((TUint)mPkt.size);
                buffer->mVideoFrameType = video_frame_type;

                if (mPkt.flags & AV_PKT_FLAG_KEY) {
                    buffer->SetBufferFlags(CIBuffer::KEY_FRAME);
                    if (mPkt.stream_index == mVideo) {//for demuxer notify video decoder on the fly
                        buffer->SetBufferFlagBits(CIBuffer::KEY_FRAME);
                        AVCodecContext *codec = mpAVFormat->streams[mVideo]->codec;
                        DASSERT(codec);
                        buffer->mVideoWidth = codec->width;
                        buffer->mVideoHeight = codec->height;
                        buffer->mVideoFrameRateNum = mpAVFormat->streams[mVideo]->r_frame_rate.num;
                        buffer->mVideoFrameRateDen = mpAVFormat->streams[mVideo]->r_frame_rate.den;
                    }
                } else {
                    buffer->SetBufferFlags(0);
                }

                DASSERT(mpCurOutputPin);
                mpCurOutputPin->SendBuffer(buffer);

                //bw case
                if (mPkt.stream_index == mVideo && mbIsBackward) {
                    if (mBWLastPTS) {
                        if (buffer->GetBufferPTS() < mBWLastPTS) {
                            mBWLastPTS = buffer->GetBufferPTS();
                        } else if (buffer->GetBufferPTS() == mBWLastPTS) {
                            mEstimatedKeyFrameInterval += 1000;
                            LOGM_NOTICE("bw, searching interval increased to %llu\n", mEstimatedKeyFrameInterval);
                        } else {
                            LOGM_NOTICE("bw, playback to end(to file header?) current pts=%lld greater than mBWLastPTS=%lld, abort bw playback\n", buffer->GetBufferPTS(), mBWLastPTS);
                            buffer = NULL;
                            msState = EModuleState_Pending;
                            break;
                        }
                    } else {
                        mBWLastPTS = buffer->GetBufferPTS();
                    }
                    LOGM_NOTICE("bw, get key frame, pts %llu\n", mBWLastPTS);
                }

                buffer = NULL;
                msState = EModuleState_Idle;
                break;

            case EModuleState_Completed:
            case EModuleState_Pending:
            case EModuleState_Error:
                mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
                processCmd(cmd);
                break;

            default:
                LOGM_ERROR("Check Me.\n");
                break;
        }
    }

    LOGM_NOTICE("GFFMpegDemuxer OnRun Exit.\n");
    CmdAck(EECode_OK);
}

void CFFMpegDemuxer::sendFlowControlBuffer(EBufferType flowcontrol_type)
{
    TUint i;

    if (EBufferType_FlowControl_EOS == flowcontrol_type) {
        for (i = 0; i < EConstMaxDemuxerMuxerStreamNumber; i ++) {
            if (mpOutputPins[i]) {
                mpOutputPins[i]->SendBuffer(&mPersistEOSBuffer);
            }
        }
    } else if (EBufferType_FlowControl_Pause == flowcontrol_type) {
        for (i = 0; i < EConstMaxDemuxerMuxerStreamNumber; i ++) {
            if (mpOutputPins[i]) {
                mpOutputPins[i]->SendBuffer(&mPersistPauseBuffer);
            }
        }
    } else if (EBufferType_FlowControl_Resume == flowcontrol_type) {
        for (i = 0; i < EConstMaxDemuxerMuxerStreamNumber; i ++) {
            if (mpOutputPins[i]) {
                mpOutputPins[i]->SendBuffer(&mPersistResumeBuffer);
            }
        }
    } else if (EBufferType_FlowControl_FinalizeFile == flowcontrol_type) {
        for (i = 0; i < EConstMaxDemuxerMuxerStreamNumber; i ++) {
            if (mpOutputPins[i]) {
                mpOutputPins[i]->SendBuffer(&mPersistFinalizeFileBuffer);
            }
        }
    } else {
        LOGM_FATAL("BAD flowcontrol_type %d\n", flowcontrol_type);
    }

    return;
}

EECode CFFMpegDemuxer::Seek(TTime &target_time, ENavigationPosition position)
{
#if 0
    DASSERT(AV_TIME_BASE == 1000000);
    TTime seek_target = target_time * 1000;
    TInt ret = 0;

    //seek by time
    if ((ret = av_seek_frame(mpAVFormat, -1, seek_target, 0)) < 0) {
        LOGM_ERROR("seek by time fail, ret =%d, try by bytes.\n", ret);
        //try seek by byte
        seek_target = seek_target * mpAVFormat->file_size / mpAVFormat->duration;
        if ((ret = av_seek_frame(mpAVFormat, -1, seek_target, AVSEEK_FLAG_BYTE)) < 0) {
            LOGM_ERROR("seek by bytes fail, ret =%d, Seek return error.\n", ret);
            return EECode_Error;
        }
    }
#endif
    return EECode_OK;
}

void CFFMpegDemuxer::updatePTSConvertor()
{
    if (mVideo >= 0) {
        DASSERT(mpAVFormat->streams[mVideo]->time_base.num == 1);
        mPTSVideo_Num = (TU64)mpAVFormat->streams[mVideo]->time_base.num * (TU64)DDefaultTimeScale;
        mPTSVideo_Den = (TU64)mpAVFormat->streams[mVideo]->time_base.den * (TU64)DDefaultTimeScale;
        if ((mPTSVideo_Num % mPTSVideo_Den) == 0) {
            mPTSVideo_Num /= mPTSVideo_Den;
            mPTSVideo_Den = 1;
        }
        LOGM_INFO("[updatePTSConvertor]: mpAVFormat->streams[mVideo]->time_base.num %d, den %d\n", mpAVFormat->streams[mVideo]->time_base.num, mpAVFormat->streams[mVideo]->time_base.den);
    }
    LOGM_INFO("[updatePTSConvertor]: mVideo %d: mPTSVideo_Num %lld, mPTSVideo_Den %lld\n", mVideo, mPTSVideo_Num, mPTSVideo_Den);
    if (mAudio >= 0) {
        DASSERT(mpAVFormat->streams[mAudio]->time_base.num == 1);
        mPTSAudio_Num = (TU64)mpAVFormat->streams[mAudio]->time_base.num * (TU64)DDefaultTimeScale;
        mPTSAudio_Den = (TU64)mpAVFormat->streams[mAudio]->time_base.den * (TU64)DDefaultTimeScale;
        if ((mPTSAudio_Num % mPTSAudio_Den) == 0) {
            mPTSAudio_Num /= mPTSAudio_Den;
            mPTSAudio_Den = 1;
        }
        LOGM_INFO("[updatePTSConvertor]: mpAVFormat->streams[mAudio]->time_base.num %d, den %d\n", mpAVFormat->streams[mAudio]->time_base.num, mpAVFormat->streams[mAudio]->time_base.den);
    }
    LOGM_INFO("[updatePTSConvertor]: mAudio %d: mPTSAudio_Num %lld, mPTSAudio_Den %lld\n", mAudio, mPTSAudio_Num, mPTSAudio_Den);
}

TTime CFFMpegDemuxer::convertVideoPts(TTime pts)
{
    pts *= mPTSVideo_Num;
    if (mPTSVideo_Den != 1) {
        pts /=  mPTSVideo_Den;
    }
    return pts;
}

TTime CFFMpegDemuxer::convertAudioPts(TTime pts)
{
    pts *= mPTSAudio_Num;
    if (mPTSAudio_Den != 1) {
        pts /=  mPTSAudio_Den;
    }
    return pts;
}

void CFFMpegDemuxer::postVideoSizeMsg(TU32 width, TU32 height)
{
    SMSG msg;
    memset(&msg, 0x0, sizeof(msg));
    msg.code = EMSGType_VideoSize;
    msg.p1 = width;
    msg.p2 = height;
    if (mpMsgSink) {
        mpMsgSink->MsgProc(msg);
    }
}

EECode CFFMpegDemuxer::DoSeek(TTime target)
{
#if 0
    TInt ret = 0;
    TU64 targeByte;
    TU64 timeMs;

    if ((ret = av_seek_frame(mpAVFormat, -1, target, 0)) < 0) {
        timeMs = target * 1000 * (mpAVFormat->streams[mVideo]->time_base.num) / (mpAVFormat->streams[mVideo]->time_base.den);
        targeByte = target * mpAVFormat->file_size / mpAVFormat->duration;
        LOGM_NOTICE("bw, seek by timestamp(%llu) fail, ret =%d, try by bytes(%lld), timeMs(%llu)--duration:%lld.\n", target, ret, targeByte, timeMs, mpAVFormat->duration);
        if ((ret = av_seek_frame(mpAVFormat, -1, target, AVSEEK_FLAG_BYTE)) < 0) {
            LOGM_ERROR("bw, seek by bytes fail, ret =%d, Seek return error.\n", ret);
            return EECode_Error;
        }
    } else {
        LOGM_NOTICE("bw, DoSeek, seek by timestamp(%llu), ret =%d, duration:%lld, pos=%lld.\n",
                    target, ret, mpAVFormat->duration, mpAVFormat->data_offset);
    }
#endif

    return EECode_OK;
}

void CFFMpegDemuxer::EventNotify(EEventType type, TU64 param1, TPointer param2)
{
    if (EEventType_BufferReleaseNotify == type) {
        DASSERT(mpWorkQ);
        if (mpWorkQ) {
            mpWorkQ->PostMsg(ECMDType_NotifyBufferRelease);
        }
    } else {
        LOGM_FATAL("TO DO\n");
    }
}

EECode CFFMpegDemuxer::QueryContentInfo(const SStreamCodecInfos *&pinfos) const
{
    pinfos = (const SStreamCodecInfos *)&mStreamCodecInfos;
    return EECode_OK;
}

EECode CFFMpegDemuxer::UpdateContext(SContextInfo *pContext)
{
    LOGM_FATAL("TO DO\n");
    return EECode_NoImplement;
}

EECode CFFMpegDemuxer::GetExtraData(SStreamingSessionContent *pContent)
{
    LOGM_FATAL("TO DO\n");
    return EECode_NoImplement;
}

EECode CFFMpegDemuxer::NavigationSeek(SContextInfo *info)
{
    LOGM_FATAL("TO DO\n");
    return EECode_NoImplement;
}

EECode CFFMpegDemuxer::ResumeChannel()
{
    LOGM_FATAL("TO DO\n");
    return EECode_NoImplement;
}

EECode CFFMpegDemuxer::SetVideoPostProcessingCallback(void *callback_context, void *callback)
{
    LOGM_FATAL("TO DO\n");
    return EECode_NoImplement;
}

EECode CFFMpegDemuxer::SetAudioPostProcessingCallback(void *callback_context, void *callback)
{
    LOGM_FATAL("TO DO\n");
    return EECode_NoImplement;
}

void CFFMpegDemuxer::PrintStatus()
{
    LOGM_PRINTF("[CFFMpegDemuxer::PrintStatus()], state(%s, %d), heart beat %d\n", gfGetModuleStateString(msState), msState, mDebugHeartBeat);
    mDebugHeartBeat = 0;
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

#endif

