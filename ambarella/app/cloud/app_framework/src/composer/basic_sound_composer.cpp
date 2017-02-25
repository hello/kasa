/*******************************************************************************
 * sound_composer.cpp
 *
 * History:
 *    2014/11/20 - [Zhi He] create file
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

#define DDUMP_PCM_PB
//#define DAUTO_RESAMPLE
#define DAUTO_SILENCE

#include "common_config.h"
#include "common_types.h"
#include "common_osal.h"
#include "common_utils.h"
#include "common_log.h"

#include "common_base.h"

#ifdef DAUTO_RESAMPLE
#include "media_mw_if.h"
#endif

#include "platform_al_if.h"
#include "app_framework_if.h"

#include "basic_sound_composer.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

static void __float2U16(float *p_src, TS16 *p_dst, TU32 num)
{
    float v = 0;
    for (TU32 i = 0; i < num; i ++) {
        v = p_src[0] * (float)(1 << 15);
        if (v > 32767) {
            p_dst[0] = 32767;
        } else if (v < (-32768)) {
            p_dst[0] = -32768;
        } else {
            p_dst[0] = (TS16) v;
        }
        p_dst ++;
        p_src ++;
    }
}

static void __float2U16scale(float *p_src, TS16 *p_dst, TU32 num, TU32 scale)
{
    float v = 0;
    for (TU32 i = 0; i < num; i ++) {
        v = p_src[0] * (float)(1 << 15) * scale;
        if (v > 32767) {
            p_dst[0] = 32767;
        } else if (v < (-32768)) {
            p_dst[0] = -32768;
        } else {
            p_dst[0] = (TS16) v;
        }
        p_dst ++;
        p_src ++;
    }
}

ISoundComposer *gfCreateBasicSoundComposer(SPersistCommonConfig *p_common_config, SSharedComponent *p_shared_component)
{
    return CBasicSoundComposer::Create(p_common_config, p_shared_component);
}

//-----------------------------------------------------------------------
//
// CBasicSoundComposer
//
//-----------------------------------------------------------------------
CBasicSoundComposer *CBasicSoundComposer::Create(SPersistCommonConfig *p_common_config, SSharedComponent *p_shared_component)
{
    CBasicSoundComposer *result = new CBasicSoundComposer(p_common_config, p_shared_component);
    if (DUnlikely((result) && (EECode_OK != result->Construct()))) {
        result->Destroy();
        result = NULL;
    }
    return result;
}

CObject *CBasicSoundComposer::GetObject() const
{
    return (CObject *) this;
}

void CBasicSoundComposer::Delete()
{
    if (mpWorkQueue) {
        mpWorkQueue->Delete();
        mpWorkQueue = NULL;
    }

    if (mpTrackList) {
        delete mpTrackList;
        mpTrackList = NULL;
    }

    if (mpMutex) {
        mpMutex->Delete();
        mpMutex = NULL;
    }

    if (mpSoundOutput) {
        mpSoundOutput->Destroy();
        mpSoundOutput = NULL;
    }

    if (mpComposeBuffer) {
        DDBG_FREE(mpComposeBuffer, "FRSB");
        mpComposeBuffer = NULL;
    }

    if (mpComposeSilentBuffer) {
        DDBG_FREE(mpComposeSilentBuffer, "FRSB");
        mpComposeSilentBuffer = NULL;
    }

    if (mpComposeResamplerBuffer) {
        DDBG_FREE(mpComposeResamplerBuffer, "FRSB");
        mpComposeResamplerBuffer = NULL;
    }

    if (mpClockReference) {
        mpClockReference->Delete();
        mpClockReference = NULL;
    }

#ifdef DAUTO_RESAMPLE
    if (mpUnderflowResampler1) {
        gfDestroyOPTCAudioResamplerProxy(mpUnderflowResampler1);
        mpUnderflowResampler1 = NULL;
    }

    if (mpUnderflowResampler2) {
        gfDestroyOPTCAudioResamplerProxy(mpUnderflowResampler2);
        mpUnderflowResampler2 = NULL;
    }

    if (mpOverflowResampler1) {
        gfDestroyOPTCAudioResamplerProxy(mpOverflowResampler1);
        mpOverflowResampler1 = NULL;
    }

    if (mpOverflowResampler2) {
        gfDestroyOPTCAudioResamplerProxy(mpOverflowResampler2);
        mpOverflowResampler2 = NULL;
    }
#endif

    CObject::Delete();
}

CBasicSoundComposer::CBasicSoundComposer(SPersistCommonConfig *p_common_config, SSharedComponent *p_shared_component)
    : inherited("CBasicSoundComposer", 0)
    , mpPersistCommonConfig(p_common_config)
    , mpWorkQueue(NULL)
    , mpTrackList(NULL)
    , mpMutex(NULL)
    , mpSoundOutput(NULL)
    , mHandle(0)
    , mpComposeBuffer(NULL)
    , mComposeBufferSize(0)
    , mpComposeSilentBuffer(NULL)
    , mComposeSilentBufferSize(0)
    , mpComposeResamplerBuffer(NULL)
    , mComposeResamplerBufferSize(0)
    , mPrepareSize(DDefaultAVSYNCAudioLatencyFrameCount *DDefaultAudioFrameSize *DDefaultAudioChannelNumber *sizeof(float))
    , mCurrentPrepareSize(0)
    , mOutputBufferSize(0)
    , mPlayWriteMinimumGap(1 * DDefaultAudioFrameSize *DDefaultAudioChannelNumber *sizeof(TS16))
    , mPlayWriteAdjustGap(6 * DDefaultAudioFrameSize *DDefaultAudioChannelNumber *sizeof(TS16))
    , mNormalGap(0)
    , mResample1Gap(0)
    , mResample2Gap(0)
    , mbLoop(1)
    , mbStartOutput(0)
    , mbCompensateUnderflow(0)
    , mbStartCompensate(0)
    , mUnderflowSamplerate1(DDefaultAudioSampleRate - 3900)
    , mUnderflowSamplerate2(DDefaultAudioSampleRate - 5600)
    , mOverflowSamplerate1(DDefaultAudioSampleRate + 1000)
    , mOverflowSamplerate2(DDefaultAudioSampleRate + 2000)
    , mpUnderflowResampler1(NULL)
    , mpUnderflowResampler2(NULL)
    , mpOverflowResampler1(NULL)
    , mpOverflowResampler2(NULL)
    , mResampleMaxPacketNumber(48)
    , mResampleCurPacketCount(0)
    , msState(ESoundComposerState_idle)
{
    if (p_shared_component) {
        mpClockManager = p_shared_component->p_clock_manager;
        mpMsgQueue = p_shared_component->p_msg_queue;
    } else {
        mpClockManager = NULL;
        mpMsgQueue = NULL;
    }
    mpClockReference = NULL;

#ifdef DDUMP_PCM_PB
    mpDumpFile = NULL;
#endif
}

EECode CBasicSoundComposer::Construct()
{
    mNormalGap = 32 * DDefaultAudioFrameSize * DDefaultAudioChannelNumber;
    mResample1Gap = 24 * DDefaultAudioFrameSize * DDefaultAudioChannelNumber;
    mResample2Gap = 4 * DDefaultAudioFrameSize * DDefaultAudioChannelNumber;

    if (DLikely(mpClockManager)) {
        mpClockReference = CIClockReference::Create();
        if (DUnlikely(!mpClockReference)) {
            LOGM_FATAL("CIClockReference::Create() fail\n");
            return EECode_NoMemory;
        }

        EECode ret = mpClockManager->RegisterClockReference(mpClockReference);
        DASSERT_OK(ret);
    } else {
        LOGM_FATAL("NULL mpClockManager\n");
        return EECode_BadState;
    }

    mpTrackList = new CIDoubleLinkedList();
    if (DUnlikely(!mpTrackList)) {
        LOG_FATAL("alloc CIDoubleLinkedList fail\n");
        return EECode_NoMemory;
    }

    DSET_MODULE_LOG_CONFIG(ELogAPPFrameworkSoundComposer);
    //mConfigLogLevel = ELogLevel_Debug;
    //mConfigLogOption = 0xffffffff;

    if (mpPersistCommonConfig && mpPersistCommonConfig->sound_output.sound_output_precache_count) {
        mPrepareSize = DDefaultAudioFrameSize * sizeof(float) * mpPersistCommonConfig->sound_output.sound_output_precache_count;
    }
    LOG_NOTICE("[config]: sound output pre cache size %d\n", mPrepareSize);

    mpMutex = gfCreateMutex();
    if (DUnlikely(!mpMutex)) {
        LOGM_ERROR("Create gfCreateMutex fail.\n");
        return EECode_NoMemory;
    }

    mComposeBufferSize = 4096 * 32;
    mpComposeBuffer = (TU8 *) DDBG_MALLOC(mComposeBufferSize, "FRSB");
    if (DUnlikely(!mpComposeBuffer)) {
        LOGM_ERROR("can not get memory(size = %d).\n", mComposeBufferSize);
        return EECode_NoMemory;
    }

    mComposeSilentBufferSize = 1024 * 1 * 16 * sizeof(TS16);
    mpComposeSilentBuffer = (TU8 *) DDBG_MALLOC(mComposeSilentBufferSize, "FRSB");
    if (DUnlikely(!mpComposeSilentBuffer)) {
        LOGM_ERROR("can not get memory(size = %d).\n", mComposeSilentBufferSize);
        return EECode_NoMemory;
    }
    memset(mpComposeSilentBuffer, 0x0, mComposeSilentBufferSize);

    mComposeResamplerBufferSize = 4096 * 32;
    mpComposeResamplerBuffer = (TU8 *) DDBG_MALLOC(mComposeResamplerBufferSize, "FRSB");
    if (DUnlikely(!mpComposeResamplerBuffer)) {
        LOGM_ERROR("can not get memory(size = %d).\n", mComposeResamplerBufferSize);
        return EECode_NoMemory;
    }

    mpWorkQueue = CIWorkQueue::Create((IActiveObject *)this);
    if (DUnlikely(!mpWorkQueue)) {
        LOGM_ERROR("Create CIWorkQueue fail.\n");
        return EECode_NoMemory;
    }

    LOGM_NOTICE("before mpWorkQueue->Run().\n");
    DASSERT(mpWorkQueue);
    mbLoop = 1;
    mpWorkQueue->Run();
    LOGM_NOTICE("after mpWorkQueue->Run().\n");

#ifdef DDUMP_PCM_PB
    if (DUnlikely(mpPersistCommonConfig->debug_dump.dump_debug_sound_output_pcm)) {
        mpDumpFile = fopen("play.pcm", "wb");
    }
#endif

    return EECode_OK;
}

CBasicSoundComposer::~CBasicSoundComposer()
{

}

EECode CBasicSoundComposer::SetHandle(TULong handle)
{
    mHandle = handle;
    return EECode_OK;
}

EECode CBasicSoundComposer::SetConfig(TU32 samplerate, TU32 framesize, TU32 channel_number, TU32 sample_format)
{
    SCMD cmd;
    cmd.code = ECMDType_ConfigAudio;
    cmd.res32_1 = samplerate;
    cmd.res64_1 = channel_number;
    return mpWorkQueue->SendCmd(cmd);
}

EECode CBasicSoundComposer::RegisterTrack(CISoundTrack *track)
{
    AUTO_LOCK(mpMutex);

    if (DLikely(mpTrackList)) {
        track->SetSoundComposer(this);
        mpTrackList->InsertContent(NULL, (void *) track, 0);
    } else {
        LOG_FATAL("mpTrackList is NULL\n");
        return EECode_InternalLogicalBug;
    }

    return EECode_OK;
}

void CBasicSoundComposer::UnRegisterTrack(CISoundTrack *track)
{
    AUTO_LOCK(mpMutex);

    if (DLikely(mpTrackList)) {
        mpTrackList->RemoveContent((void *) track);
    } else {
        LOG_FATAL("mpTrackList is NULL\n");
    }
}

void CBasicSoundComposer::DataNotify()
{
    mpWorkQueue->PostMsg(EMSGType_DataNotify, NULL);
}

void CBasicSoundComposer::ControlNotify(TU32 control_type)
{
    SCMD cmd;
    cmd.code = EMSGType_ControlNotify;
    cmd.res32_1 = control_type;
    mpWorkQueue->PostMsg(cmd);
}

EECode CBasicSoundComposer::StartRunning()
{
    EECode err;

    LOGM_NOTICE("StartRunning start.\n");
    err = mpWorkQueue->SendCmd(ECMDType_Start, NULL);
    LOGM_NOTICE("StartRunning end, ret %d, %s.\n", err, gfGetErrorCodeString(err));

    return err;
}

EECode CBasicSoundComposer::ExitRunning()
{
    LOGM_NOTICE("ExitRunning start.\n");
    EECode err = mpWorkQueue->SendCmd(ECMDType_Stop, NULL);
    LOGM_NOTICE("ExitRunning end, ret %d, %s.\n", err, gfGetErrorCodeString(err));

    return err;
}

void CBasicSoundComposer::OnRun()
{
    SCMD cmd;
    CIDoubleLinkedList::SNode *pnode = NULL;
    mpWorkQueue->CmdAck(EECode_OK);

    LOGM_NOTICE("OnRun() loop, start\n");

    while (mbLoop) {

        //LOGM_STATE("msState(%d)\n", msState);
        switch (msState) {

            case ESoundComposerState_idle:
                mpWorkQueue->WaitMsg(&cmd, sizeof(cmd));
                processCmd(cmd);
                break;

            case ESoundComposerState_wait_config:
                mpWorkQueue->WaitMsg(&cmd, sizeof(cmd));
                processCmd(cmd);
                break;

            case ESoundComposerState_preparing:
                mpWorkQueue->WaitMsg(&cmd, sizeof(cmd));
                processCmd(cmd);

                mCurrentPrepareSize += composeSound();
                if (mCurrentPrepareSize >= mPrepareSize) {
                    LOGM_NOTICE("pre-cache(%d) done, start play\n", mCurrentPrepareSize);
                    startOutput();
                    mCurrentPrepareSize = 0;
                    msState = ESoundComposerState_running;
                    break;
                }
                break;

            case ESoundComposerState_running:
                mpWorkQueue->WaitMsg(&cmd, sizeof(cmd));
                processCmd(cmd);
                if (DUnlikely(ESoundComposerState_running != msState)) {
                    break;
                }

                composeSound();
#ifdef DAUTO_RESAMPLE
                if (DLikely(mResampleCurPacketCount < 4)) {
                    mResampleCurPacketCount ++;
                } else {
                    mResampleCurPacketCount = 0;
                    adjustResample();
                }
#elif defined (DAUTO_SILENCE)
                if (DLikely(mResampleCurPacketCount < 4)) {
                    mResampleCurPacketCount ++;
                } else {
                    mResampleCurPacketCount = 0;
                    adjustPlayPoint();
                }
#endif
                break;

#ifdef DAUTO_RESAMPLE
            case ESoundComposerState_resample_1:
                mpWorkQueue->WaitMsg(&cmd, sizeof(cmd));
                processCmd(cmd);
                if (DUnlikely(ESoundComposerState_resample_1 != msState)) {
                    break;
                }

                if (DLikely(mResampleCurPacketCount < mResampleMaxPacketNumber)) {
                    composeSoundResample1(0);
                    mResampleCurPacketCount ++;
                } else {
                    composeSoundResample1(1);
                    mResampleCurPacketCount = 0;
                    adjustResample();
                }
                break;

            case ESoundComposerState_resample_2:
                mpWorkQueue->WaitMsg(&cmd, sizeof(cmd));
                processCmd(cmd);
                if (DUnlikely(ESoundComposerState_resample_2 != msState)) {
                    break;
                }

                if (DLikely(mResampleCurPacketCount < mResampleMaxPacketNumber)) {
                    composeSoundResample2(0);
                    mResampleCurPacketCount ++;
                } else {
                    composeSoundResample2(1);
                    mResampleCurPacketCount = 0;
                    adjustResample();
                }
                break;
#endif

            case ESoundComposerState_pending:
                mpWorkQueue->WaitMsg(&cmd, sizeof(cmd));
                processCmd(cmd);
                break;

            case ESoundComposerState_error:
                mpWorkQueue->WaitMsg(&cmd, sizeof(cmd));
                processCmd(cmd);
                break;

            default:
                LOGM_FATAL("Unexpected state(%d)\n", msState);
                break;
        }
    }

    LOGM_NOTICE("OnRun() loop, exit\n");
}

void CBasicSoundComposer::Destroy()
{
#ifdef DDUMP_PCM_PB
    if (mpDumpFile) {
        fclose(mpDumpFile);
        mpDumpFile = NULL;
    }
#endif

    Delete();
}

void CBasicSoundComposer::processCmd(SCMD &cmd)
{
    switch (cmd.code) {

        case ECMDType_Start:
            msState = ESoundComposerState_wait_config;
            mpWorkQueue->CmdAck(EECode_OK);
            break;

        case ECMDType_Stop:
            mbLoop = 0;
            msState = ESoundComposerState_pending;
            stopOutput();
            mpWorkQueue->CmdAck(EECode_OK);
            break;

        case ECMDType_ConfigAudio:
            if (!mpSoundOutput) {
                DASSERT(ESoundComposerState_wait_config == msState);
                EECode err = createSoundOutput(cmd.res32_1, (TU32)cmd.res64_1);
                if (DUnlikely(EECode_OK != err)) {
                    LOGM_ERROR("createSoundOutput() fail, ret %d, %s.\n", err, gfGetErrorCodeString(err));
                    msState = ESoundComposerState_error;
                    mpWorkQueue->CmdAck(err);
                    break;
                }
            }
            msState = ESoundComposerState_preparing;
            mpWorkQueue->CmdAck(EECode_OK);
            break;

        case EMSGType_ControlNotify:
            if (DCONTROL_NOTYFY_DATA_PAUSE == cmd.res32_1) {
                stopOutput();
                mCurrentPrepareSize = 0;
                msState = ESoundComposerState_idle;
            } else if (DCONTROL_NOTYFY_DATA_RESUME == cmd.res32_1) {
                msState = ESoundComposerState_preparing;
            }
            break;

        case EMSGType_DataNotify:
            break;

        default:
            LOGM_ERROR("wrong cmd.code: %d", cmd.code);
            break;
    }

    return;
}

EECode CBasicSoundComposer::createSoundOutput(TU32 smplerate, TU32 channel_number)
{
    if (!mpSoundOutput) {
        mpSoundOutput = gfCreatePlatformSoundOutput(ESoundSetting_directsound, mpPersistCommonConfig);
        if (!mpSoundOutput) {
            LOG_FATAL("gfCreatePlatformSoundOutput() fail\n");
            return EECode_OSError;
        }
    }

    mSoundConfig.channels = channel_number;
    mSoundConfig.sample_format = 0;
    mSoundConfig.is_channel_interlave = 0;
    mSoundConfig.bytes_per_sample = 2;
    mSoundConfig.sample_frequency = smplerate;
    mSoundConfig.frame_size = DDefaultAudioFrameSize;
    EECode err = mpSoundOutput->SetupContext(&mSoundConfig, mHandle);

    if (DUnlikely(EECode_OK != err)) {
        LOG_FATAL("mpSoundOutput->SetupContext() fail\n");
        return err;
    }

    mpSoundOutput->QueryBufferSize(mOutputBufferSize);

    return EECode_OK;
}

TU32 CBasicSoundComposer::composeSound()
{
    CISoundTrack *p_track = NULL;
    TU8 *p;
    TU32 size;
    TU32 write_size = 0;

    CIDoubleLinkedList::SNode *p_node = mpTrackList->FirstNode();
    while (p_node) {
        p_track = (CISoundTrack *) p_node->p_context;
        if (DLikely(p_track)) {
            p_track->Lock();
            p_track->QueryData(p, size);
            if (DLikely(size)) {
                write_size += size;
                writeData(p_track, p, size);
            } else {
                p_track->Unlock();
            }
        }
        p_node = mpTrackList->NextNode(p_node);
    }

    return write_size;
}

#ifdef DAUTO_RESAMPLE
TU32 CBasicSoundComposer::composeSoundResample1(TU32 clean_remain)
{
    CISoundTrack *p_track = NULL;
    TU8 *p;
    TU32 size;
    TU32 write_size = 0;

    CIDoubleLinkedList::SNode *p_node = mpTrackList->FirstNode();
    while (p_node) {
        p_track = (CISoundTrack *) p_node->p_context;
        if (DLikely(p_track)) {
            p_track->Lock();
            p_track->QueryData(p, size);
            if (DLikely(size)) {
                write_size += size;
                writeDataResample(p_track, p, size, mpUnderflowResampler1, clean_remain);
            } else {
                p_track->Unlock();
            }
        }
        p_node = mpTrackList->NextNode(p_node);
    }

    return write_size;
}

TU32 CBasicSoundComposer::composeSoundResample2(TU32 clean_remain)
{
    CISoundTrack *p_track = NULL;
    TU8 *p;
    TU32 size;
    TU32 write_size = 0;

    CIDoubleLinkedList::SNode *p_node = mpTrackList->FirstNode();
    while (p_node) {
        p_track = (CISoundTrack *) p_node->p_context;
        if (DLikely(p_track)) {
            p_track->Lock();
            p_track->QueryData(p, size);
            if (DLikely(size)) {
                write_size += size;
                writeDataResample(p_track, p, size, mpUnderflowResampler2, clean_remain);
            } else {
                p_track->Unlock();
            }
        }
        p_node = mpTrackList->NextNode(p_node);
    }

    return write_size;
}
#endif

void CBasicSoundComposer::startOutput()
{
    if (mpSoundOutput) {
        if (!mbStartOutput) {
            LOGM_NOTICE("before mpSoundOutput->Start()\n");
            mpSoundOutput->Start();
            mbStartOutput = 1;
        }
    }
}

void CBasicSoundComposer::stopOutput()
{
    if (mpSoundOutput) {
        if (mbStartOutput) {
            LOGM_NOTICE("before mpSoundOutput->Stop()\n");
            mpSoundOutput->Stop();
            mbStartOutput = 0;
        }
    }
}

void CBasicSoundComposer::writeData(CISoundTrack *p_track, TU8 *p, TU32 size)
{
    if (1) {
        __float2U16((float *)p, (TS16 *)mpComposeBuffer, (size >> 2));
    } else {
        __float2U16scale((float *)p, (TS16 *)mpComposeBuffer, (size >> 2), 3);
    }
    p_track->ClearData();
    p_track->Unlock();

    size = size >> 1;

    mpSoundOutput->WriteSoundData(mpComposeBuffer, size);

#ifdef DDUMP_PCM_PB
    if (DUnlikely(mpPersistCommonConfig->debug_dump.dump_debug_sound_output_pcm)) {
        if (mpDumpFile) {
            fwrite(mpComposeBuffer, 1, size, mpDumpFile);
            fflush(mpDumpFile);
        }
    }
#endif

}

#ifdef DAUTO_RESAMPLE
void CBasicSoundComposer::writeDataResample(CISoundTrack *p_track, TU8 *p, TU32 size, void *p_resampler, TU32 clean_remain)
{
    size = size >> 2;

    if (1) {
        __float2U16((float *)p, (TS16 *)mpComposeResamplerBuffer, size);
    } else {
        __float2U16scale((float *)p, (TS16 *)mpComposeResamplerBuffer, size, 3);
    }
    p_track->ClearData();
    p_track->Unlock();

    int remain_samples = 0;
    int output_samples = 0;

    //LOGM_PRINTF("[resample in]: size %d\n", size);
    output_samples = gfOPTCAudioResampleProxy(p_resampler, (short *)mpComposeBuffer, (short *)mpComposeResamplerBuffer, size, mComposeBufferSize >> 1, 1, &remain_samples);
    //LOGM_PRINTF("[resample out]: output_samples %d, remain_samples %d\n", output_samples, remain_samples);
    if (clean_remain && remain_samples) {
        output_samples += gfOPTCAudioResampleGetRemainSamplesProxy(p_resampler, (short *)mpComposeBuffer + output_samples);
    }

    size = ((TU32) output_samples) << 1;
    mpSoundOutput->WriteSoundData(mpComposeBuffer, size);

#ifdef DDUMP_PCM_PB
    if (DUnlikely(mpPersistCommonConfig->debug_dump.dump_debug_sound_output_pcm)) {
        if (mpDumpFile) {
            fwrite(mpComposeBuffer, 1, size, mpDumpFile);
            fflush(mpDumpFile);
        }
    }
#endif

}
#endif

void CBasicSoundComposer::adjustPlayPoint()
{
    TULong cur_play_offset = 0;
    TULong cur_write_offset = 0;
    TULong gap = 0;

    mpSoundOutput->QueryCurrentPosition(cur_write_offset, cur_play_offset);

    if (cur_write_offset > cur_play_offset) {
        gap = cur_write_offset - cur_play_offset;
    } else {
        gap = cur_write_offset + mOutputBufferSize - cur_play_offset;
    }

    //LOGM_PRINTF("gap(%ld), cur offset %ld, cur write offset %ld\n", gap, cur_play_offset, cur_write_offset);

    if (DUnlikely(gap < mPlayWriteMinimumGap)) {
#if 0
        LOGM_PRINTF("gap(%ld) too small, cur offset %ld, cur write offset %ld, reset it\n", gap, cur_play_offset, cur_write_offset);
        if (cur_write_offset >= mPlayWriteAdjustGap) {
            cur_play_offset = cur_write_offset - mPlayWriteAdjustGap;
        } else {
            cur_play_offset = cur_write_offset + mOutputBufferSize - mPlayWriteAdjustGap;
        }
        mpSoundOutput->SetCurrentPlayPosition(cur_play_offset);
        LOGM_PRINTF("reset cur offset %ld\n", cur_play_offset);
#else
        mpSoundOutput->WriteSoundData(mpComposeSilentBuffer, mComposeSilentBufferSize);
        LOGM_PRINTF("fill silent buffer\n");
#endif
    }
}

void CBasicSoundComposer::adjustResample()
{
#ifdef DAUTO_RESAMPLE
    TULong cur_play_offset = 0;
    TULong cur_write_offset = 0;
    TULong gap = 0;

    mpSoundOutput->QueryCurrentPosition(cur_write_offset, cur_play_offset);

    if (cur_write_offset > cur_play_offset) {
        gap = cur_write_offset - cur_play_offset;
    } else {
        gap = cur_write_offset + mOutputBufferSize - cur_play_offset;
    }

    //LOGM_PRINTF("gap(%ld), cur offset %ld, cur write offset %ld\n", gap, cur_play_offset, cur_write_offset);

    if (DUnlikely(gap < mResample2Gap)) {
        LOGM_PRINTF("[to resample 2]: gap(%ld), cur offset %ld, cur write offset %ld\n", gap, cur_play_offset, cur_write_offset);
        msState = ESoundComposerState_resample_2;
        if (DUnlikely(!mpUnderflowResampler2)) {
            mpUnderflowResampler2 = gfCreateOPTCAudioResamplerProxy(mSoundConfig.sample_frequency, mUnderflowSamplerate2, 16, 10, 0, 0.8);
            if (DUnlikely(!mpUnderflowResampler2)) {
                LOGM_FATAL("gfCreateOPTCAudioResampler fail\n");
                msState = ESoundComposerState_error;
            }
        }
    } else if (DUnlikely(gap < mResample1Gap)) {
        LOGM_PRINTF("[to resample 1]: gap(%ld), cur offset %ld, cur write offset %ld\n", gap, cur_play_offset, cur_write_offset);
        msState = ESoundComposerState_resample_1;
        if (DUnlikely(!mpUnderflowResampler1)) {
            mpUnderflowResampler1 = gfCreateOPTCAudioResamplerProxy(mSoundConfig.sample_frequency, mUnderflowSamplerate1, 16, 10, 0, 0.8);
            if (DUnlikely(!mpUnderflowResampler1)) {
                LOGM_FATAL("gfCreateOPTCAudioResampler fail\n");
                msState = ESoundComposerState_error;
            }
        }
    } else if (gap > mNormalGap) {
        LOGM_PRINTF("[to normal]: gap(%ld), cur offset %ld, cur write offset %ld\n", gap, cur_play_offset, cur_write_offset);
        msState = ESoundComposerState_running;
    }
#endif
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

