/*******************************************************************************
 * muxer.cpp
 *
 * History:
 *    2013/02/19 - [Zhi He] create file
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

#ifdef DCONFIG_COMPILE_OLD_VERSION_FILE

#include "common_types.h"
#include "common_osal.h"
#include "common_utils.h"
#include "common_log.h"

#include "common_base.h"

#include "storage_lib_if.h"
#include "media_mw_if.h"
#include "media_mw_utils.h"
#include "mw_internal.h"

#include "framework_interface.h"
#include "filters_interface.h"
#include "dsp_platform_interface.h"
#include "streaming_if.h"
#include "modules_interface.h"

#include "muxer.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

IFilter *gfCreateMuxerFilter(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index)
{
    return CMuxer::Create(pName, pPersistMediaConfig, pEngineMsgSink, index);
}

IFilter *gfCreateScheduledMuxerFilter(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index)
{
    return CScheduledMuxer::Create(pName, pPersistMediaConfig, pEngineMsgSink, index);
}

static TU8 _pinNeedSync(StreamType type)
{
    //only sync video/audio with precise PTS in current case
    if (StreamType_Audio == type || StreamType_Video == type) {
        return 1;
    }
    return 0;
}

static TU8 _isMuxerExtraData(EBufferType type)
{
    if ((EBufferType_VideoExtraData == type) || (EBufferType_AudioExtraData == type)) {
        return 1;
    }

    return 0;
}

static TU8 _isMuxerInputData(EBufferType type)
{
    if ((EBufferType_AudioES == type) || (EBufferType_VideoES == type) || (EBufferType_PrivData == type)) {
        return 1;
    }

    return 0;
}

static TU8 _isFlowControlBuffer(EBufferType type)
{
    if ((EBufferType_FlowControl_EOS == type) || (EBufferType_FlowControl_Pause == type) || (EBufferType_FlowControl_Resume == type) || (EBufferType_FlowControl_FinalizeFile == type)) {
        return 1;
    }

    return 0;
}

static TU8 _isTmeStampNear(TTime t0, TTime t1, TTime max_gap)
{
    if ((t0 + max_gap < t1) || (t1 + max_gap < t0)) {
        return 0;
    }
    return 1;
}

static EMuxerModuleID _guess_muxer_type_from_string(TChar *string)
{
    if (!string) {
        LOG_WARN("NULL input in _guess_muxer_type_from_string, choose default\n");
        return EMuxerModuleID_FFMpeg;
    }

    if (!strncmp(string, DNameTag_FFMpeg, strlen(DNameTag_FFMpeg))) {
        return EMuxerModuleID_FFMpeg;
    } else if (!strncmp(string, DNameTag_PrivateTS, strlen(DNameTag_PrivateTS))) {
        return EMuxerModuleID_PrivateTS;
    } else if (!strncmp(string, DNameTag_PrivateMP4, strlen(DNameTag_PrivateMP4))) {
        return EMuxerModuleID_PrivateMP4;
    }

    LOG_WARN("unknown string tag(%s) in _guess_muxer_type_from_string, choose default\n", string);
    return EMuxerModuleID_FFMpeg;
}

CMuxerInput *CMuxerInput::Create(const TChar *pName, IFilter *pFilter, CIQueue *pQueue, TUint type, TUint stream_index)
{
    CMuxerInput *result = new CMuxerInput(pName, pFilter, type, stream_index);
    if (result && (result->Construct(pQueue) != EECode_OK)) {
        delete result;
        result = NULL;
    }
    return result;
}

EECode CMuxerInput::Construct(CIQueue *pQueue)
{
    LOGM_DEBUG("CMuxerInput::Construct.\n");
    DASSERT(mpFilter);
    EECode err = inherited::Construct(pQueue);
    return err;
}

void CMuxerInput::Delete()
{
    return inherited::Delete();
}

TU32 CMuxerInput::GetDataCnt() const
{
    return mpBufferQ->GetDataCnt();
}

TUint CMuxerInput::NeedDiscardPacket()
{
    return mDiscardPacket;
}

void CMuxerInput::DiscardPacket(TUint discard)
{
    mDiscardPacket = discard;
}

void CMuxerInput::PrintState()
{
    LOGM_NOTICE("CMuxerInput state: mDiscardPacket %d, mType %d, mStreamIndex %d.\n", mDiscardPacket, mType, mStreamIndex);
}

void CMuxerInput::PrintTimeInfo()
{
    LOGM_NOTICE(" Time info: mStartPTS %lld, mCurrentPTS %lld, mCurrentDTS %lld.\n", mStartPTS, mCurrentPTS, mCurrentDTS);
    LOGM_NOTICE("      mCurrentFrameCount %llu, mTotalFrameCount %llu, mSession %u, mDuration%lld.\n", mCurrentFrameCount, mTotalFrameCount, mSession, mDuration);
}

void CMuxerInput::timeDiscontinue()
{
    mSession ++;
    mCurrentFrameCount = 0;
}

CMuxerInput::~CMuxerInput()
{
    LOGM_RESOURCE("CMuxerInput::~CMuxerInput.\n");
}

//-----------------------------------------------------------------------
//
// CMuxer
//
//-----------------------------------------------------------------------
IFilter *CMuxer::Create(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TUint index)
{
    CMuxer *result = new CMuxer(pName, pPersistMediaConfig, pEngineMsgSink, index);
    if (result && result->Construct() != EECode_OK) {
        delete result;
        result = NULL;
    }
    return result;
}

CMuxer::CMuxer(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TUint index)
    : inherited(pName, index, pPersistMediaConfig, pEngineMsgSink)
    , mpMuxer(NULL)
    , mpCurrentInputPin(NULL)
    , mnInputPinsNumber(0)
    , mRequestContainerType(ContainerType_TS)
    , mRequestStartDTS(0)
    , mRequestStartPTS(12012)
    , mCurMuxerID(EMuxerModuleID_Invalid)
    , mpBuffer(NULL)
    , mbMuxerContentSetup(0)
    , mbMuxerPaused(0)
    , mbMuxerStarted(0)
    , mbMuxerVideoEnabled(1)
    , mbMuxerAudioEnabled(1)
    , mbMuxerPridataEnabled(0)
    , mbRecievedEosSignal(0)
    , mbEos(0)
    , mpOutputFileName(NULL)
    , mpThumbNailFileName(NULL)
    , mOutputFileNameLength(0)
    , mThumbNailFileNameLength(0)
    , mnSession(0)
    , mStreamStartPTS(0)
    , mFileNameHanding1(0)
    , mFileNameHanding2(0)
    , mbMasterStarted(0)
    , mbExtraDataRecieved(0)
    , mpMasterInput(NULL)
    , mCurrentFileIndex(0)
    , mFirstFileIndexCanbeDeleted(0)
    , mTotalFileNumber(0)
    , mMaxTotalFileNumber(0)
    , mTotalRecFileNumber(0)
    , mpOutputFileNameBase(NULL)
    , mpThumbNailFileNameBase(NULL)
    , mOutputFileNameBaseLength(0)
    , mThumbNailFileNameBaseLength(0)
    , mSavingFileStrategy(MuxerSavingFileStrategy_AutoSeparateFile)
    , mSavingCondition(MuxerSavingCondition_InputPTS)
    , mAutoFileNaming(MuxerAutoFileNaming_ByNumber)
    , mAutoSaveFrameCount(180 * 30)
    , mAutoSavingTimeDuration(180 * TimeUnitDen_90khz)
    , mNextFileTimeThreshold(0)
    , mAudioNextFileTimeThreshold(0)
    , mIDRFrameInterval(0)
    , mVideoFirstPTS(0)
    , mbNextFileTimeThresholdSet(0)
    , mbPTSStartFromZero(1)
    , mbUrlSet(0)
    , mbMuxerRunning(0)
    , mPresetVideoMaxFrameCnt(0)
    , mPresetAudioMaxFrameCnt(0)
    , mPresetMaxFilesize(0)
    , mCurrentTotalFilesize(0)
    , mPresetCheckFlag(0)
    , mPresetReachType(0)
{
    TUint i = 0;
    for (i = 0; i < EConstMaxDemuxerMuxerStreamNumber; i ++) {
        mpInputPins[i] = NULL;
    }

    for (i = 0; i < EConstMaxDemuxerMuxerStreamNumber; i ++) {
        mpExtraData[i] = NULL;
        mExtraDataSize[i] = 0;
    }

    memset(&mStreamCodecInfos, 0x0, sizeof(mStreamCodecInfos));
    memset(&mMuxerFileInfo, 0x0, sizeof(mMuxerFileInfo));

    mpStorageManager = pPersistMediaConfig->p_storage_manager;
}

EECode CMuxer::Construct()
{
    DSET_MODULE_LOG_CONFIG(ELogModuleMuxerFilter);
    return inherited::Construct();
}

CMuxer::~CMuxer()
{
    TUint i = 0;

    LOGM_RESOURCE("CMuxer::~CMuxer(), before delete inputpins.\n");
    for (i = 0; i < EConstMaxDemuxerMuxerStreamNumber; i ++) {
        if (mpInputPins[i]) {
            mpInputPins[i]->Delete();
            mpInputPins[i] = NULL;
        }
    }

    LOGM_RESOURCE("CMuxer::~CMuxer(), before mpMuxer->Delete().\n");
    if (mpMuxer) {
        mpMuxer->Delete();
        mpMuxer = NULL;
    }

    for (i = 0; i < EConstMaxDemuxerMuxerStreamNumber; i ++) {
        if (mpExtraData[i]) {
            free(mpExtraData[i]);
            mpExtraData[i] = NULL;
        }
        mExtraDataSize[i] = 0;
    }

    if (mpOutputFileName) {
        free(mpOutputFileName);
        mpOutputFileName = NULL;
    }

    if (mpOutputFileNameBase) {
        free(mpOutputFileNameBase);
        mpOutputFileNameBase = NULL;
    }

    if (mpThumbNailFileName) {
        free(mpThumbNailFileName);
        mpThumbNailFileName = NULL;
    }

    if (mpThumbNailFileNameBase) {
        free(mpThumbNailFileNameBase);
        mpThumbNailFileNameBase = NULL;
    }

    DASSERT(!mpMuxer);
    DASSERT(!mbMuxerStarted);
    if (mpMuxer && mbMuxerStarted) {
        LOGM_WARN("finalize file here\n");
        finalizeFile();
    }

    LOGM_RESOURCE("CMuxer::~CMuxer(), end.\n");
}

void CMuxer::Delete()
{
    TUint i = 0;

    LOGM_RESOURCE("CMuxer::Delete(), before delete inputpins.\n");
    for (i = 0; i < EConstMaxDemuxerMuxerStreamNumber; i ++) {
        if (mpInputPins[i]) {
            mpInputPins[i]->Delete();
            mpInputPins[i] = NULL;
        }
    }

    LOGM_RESOURCE("CMuxer::Delete(), before mpMuxer->Delete().\n");
    if (mpMuxer) {
        mpMuxer->Delete();
        mpMuxer = NULL;
    }

    for (i = 0; i < EConstMaxDemuxerMuxerStreamNumber; i ++) {
        if (mpExtraData[i]) {
            free(mpExtraData[i]);
            mpExtraData[i] = NULL;
        }
        mExtraDataSize[i] = 0;
    }

    if (mpOutputFileName) {
        free(mpOutputFileName);
        mpOutputFileName = NULL;
    }

    if (mpOutputFileNameBase) {
        free(mpOutputFileNameBase);
        mpOutputFileNameBase = NULL;
    }

    if (mpThumbNailFileName) {
        free(mpThumbNailFileName);
        mpThumbNailFileName = NULL;
    }

    if (mpThumbNailFileNameBase) {
        free(mpThumbNailFileNameBase);
        mpThumbNailFileNameBase = NULL;
    }

    DASSERT(!mpMuxer);
    DASSERT(!mbMuxerStarted);
    if (mpMuxer && mbMuxerStarted) {
        LOGM_WARN("finalize file here\n");
        finalizeFile();
    }

    LOGM_RESOURCE("CMuxer::Delete(), end.\n");

    inherited::Delete();
}

EECode CMuxer::initialize(TChar *p_param)
{
    EMuxerModuleID id;
    id = _guess_muxer_type_from_string(p_param);

    LOGM_INFO("EMuxerModuleID %d\n", id);

    if (mbMuxerContentSetup) {

        DASSERT(mpMuxer);
        if (!mpMuxer) {
            LOGM_FATAL("[Internal bug], why the mpMuxer is NULL here?\n");
            return EECode_InternalLogicalBug;
        }

        LOGM_WARN("There's a mpMuxer %p already, mbMuxerStarted %d, delete it first\n", mpMuxer, mbMuxerStarted);

        if (mbMuxerStarted) {
            LOGM_INFO("before mpMuxer->FinalizeFile()\n");
            getFileInformation();
            mpMuxer->FinalizeFile(&mMuxerFileInfo);
            mbMuxerStarted = 0;
        }

        LOGM_INFO("before mpMuxer->DestroyContext()\n");
        if (mbMuxerContentSetup) {
            mpMuxer->DestroyContext();
            mbMuxerContentSetup = 0;
        }

        if (id != mCurMuxerID) {
            LOGM_INFO("before mpMuxer->Delete(), cur id %d, request id %d\n", mCurMuxerID, id);
            mpMuxer->Delete();
            mpMuxer = NULL;
        }
    }

    if (!mpMuxer) {
        LOGM_INFO("before gfModuleFactoryMuxer(%d)\n", id);
        mpMuxer = gfModuleFactoryMuxer(id, mpPersistMediaConfig, mpEngineMsgSink);
        if (mpMuxer) {
            mCurMuxerID = id;
        } else {
            mCurMuxerID = EMuxerModuleID_Auto;
            LOGM_FATAL("[Internal bug], request gfModuleFactoryMuxer(%d) fail\n", id);
            return EECode_InternalLogicalBug;
        }
    }

    LOGM_INFO("[Muxer flow], before mpMuxer->SetupContext()\n");
    mpMuxer->SetupContext();
    mbMuxerContentSetup = 1;

    LOGM_INFO("[Muxer flow], CMuxer::initialize() done\n");

    return EECode_OK;
}

EECode CMuxer::Initialize(TChar *p_param)
{
    DASSERT(!mbMuxerRunning);
    DASSERT(!mbStarted);

    return initialize(p_param);
}

void CMuxer::finalize()
{
    if (mbMuxerContentSetup) {

        if (DUnlikely(!mpMuxer)) {
            LOGM_WARN("[Internal bug], why the mpMuxer is NULL here?\n");
            return;
        }

        if (mbMuxerStarted) {
            LOGM_INFO("before mpMuxer->FinalizeFile()\n");
            getFileInformation();
            mpMuxer->FinalizeFile(&mMuxerFileInfo);
            mbMuxerStarted = 0;
        }

        LOGM_INFO("before mpMuxer->DestroyContext()\n");
        if (mbMuxerContentSetup) {
            mpMuxer->DestroyContext();
            mbMuxerContentSetup = 0;
        }

    }

    return;
}

EECode CMuxer::Finalize()
{
    if (mpMuxer) {
        getFileInformation();
        if (mbMuxerStarted) {
            LOGM_INFO("before mpMuxer->FinalizeFile()\n");
            mpMuxer->FinalizeFile(&mMuxerFileInfo);
            mbMuxerStarted = 0;
        }

        LOGM_INFO("before mpMuxer->DestroyContext()\n");
        DASSERT(mbMuxerContentSetup);
        if (mbMuxerContentSetup) {
            mpMuxer->DestroyContext();
            mbMuxerContentSetup = 0;
        }

        LOGM_INFO("before mpMuxer->Delete(), cur id %d\n", mCurMuxerID);
        mpMuxer->Delete();
        mpMuxer = NULL;
    }

    return EECode_OK;
}

void CMuxer::initializeTimeInfo()
{
    TUint i = 0;
    CMuxerInput *pInput = NULL;

    for (i = 0; i < mnInputPinsNumber; i++) {
        pInput = mpInputPins[i];
        DASSERT(mStreamCodecInfos.info[i].inited);
        DASSERT(pInput);
        if (pInput && mStreamCodecInfos.info[i].inited) {
            if (StreamType_Video == mStreamCodecInfos.info[i].stream_type) {
                pInput->mStartPTS = mStreamStartPTS;
                pInput->mSessionPTSStartPoint = mStreamStartPTS;
                pInput->mCurrentPTS = mStreamStartPTS;
                pInput->mCurrentDTS = mStreamStartPTS;
                pInput->mSessionInputPTSStartPoint = 0;

                pInput->mCurrentFrameCount = 0;
                pInput->mTotalFrameCount = 0;
                pInput->mSession = 0;
                DASSERT(mStreamCodecInfos.info[i].spec.video.framerate_den);
                DASSERT(mStreamCodecInfos.info[i].spec.video.framerate_num);
                if ((mStreamCodecInfos.info[i].spec.video.framerate_den) && (mStreamCodecInfos.info[i].spec.video.framerate_num)) {
                    pInput->mDuration = mStreamCodecInfos.info[i].spec.video.framerate_den * TimeUnitDen_90khz / mStreamCodecInfos.info[i].spec.video.framerate_num;
                } else {
                    //default one
                    mStreamCodecInfos.info[i].spec.video.framerate_num = TimeUnitDen_90khz;
                    mStreamCodecInfos.info[i].spec.video.framerate_den = TimeUnitNum_fps29dot97;
                    pInput->mDuration = TimeUnitNum_fps29dot97;
                }
                pInput->mAVNormalizedDuration = pInput->mDuration;
                pInput->mPTSDTSGap = pInput->mPTSDTSGapDivideDuration * pInput->mDuration;
            } else if (StreamType_Audio == mStreamCodecInfos.info[i].stream_type) {
                pInput->mStartPTS = mStreamStartPTS;
                pInput->mSessionPTSStartPoint = mStreamStartPTS;
                pInput->mCurrentPTS = mStreamStartPTS;
                pInput->mCurrentDTS = mStreamStartPTS;
                pInput->mSessionInputPTSStartPoint = 0;

                pInput->mCurrentFrameCount = 0;
                pInput->mTotalFrameCount = 0;
                pInput->mSession = 0;

                if (mStreamCodecInfos.info[i].spec.audio.frame_size) {
                    pInput->mDuration = mStreamCodecInfos.info[i].spec.audio.frame_size;
                    pInput->mAVNormalizedDuration = mStreamCodecInfos.info[i].spec.audio.frame_size;
                } else {
                    //default here
                    mStreamCodecInfos.info[i].spec.audio.frame_size = 1024;
                    pInput->mAVNormalizedDuration = mStreamCodecInfos.info[i].spec.audio.frame_size;
                }
                pInput->mPTSDTSGap = pInput->mPTSDTSGapDivideDuration * pInput->mDuration;
            } else {
                LOGM_FATAL("NOT support here\n");
            }
        }
    }

}

EECode CMuxer::initializeFile(TChar *p_param, ContainerType container_type)
{
    EECode err = EECode_OK;

    LOGM_INFO("start, p_param %s, container_type %d\n", p_param, container_type);

    if (!p_param) {
        LOGM_ERROR("NULL p_param\n");
        return EECode_BadParam;
    }

    if (mbMuxerContentSetup) {

        DASSERT(mpMuxer);
        if (!mpMuxer) {
            LOGM_FATAL("[Internal bug], why the mpMuxer is NULL here?\n");
            return EECode_InternalLogicalBug;
        }

        LOGM_INFO("mbMuxerStarted %d, before mpMuxer->InitializeFile(&mStreamCodecInfos, p_param, container_type);\n", mbMuxerStarted);

        DASSERT(!mbMuxerStarted);

        err = mpMuxer->InitializeFile(&mStreamCodecInfos, p_param, container_type);
        if (EECode_OK != err) {
            LOGM_ERROR("mpMuxer->InitializeFile(&mStreamCodecInfos, p_param %s, container_type %d), fail, return %d, %s\n", p_param, container_type, err, gfGetErrorCodeString(err));
            return err;
        }

        initializeTimeInfo();
        mbMuxerStarted = 1;
    } else {
        LOGM_FATAL("[Internal bug], why the mbMuxerContentSetup is %d, mpMuxer %p?\n", mbMuxerContentSetup, mpMuxer);
        return EECode_InternalLogicalBug;
    }

    return EECode_OK;
}

EECode CMuxer::finalizeFile()
{
    if (mpMuxer) {
        if (mbMuxerStarted) {
            getFileInformation();
            LOGM_INFO("before mpMuxer->FinalizeFile()\n");
            mpMuxer->FinalizeFile(&mMuxerFileInfo);
            mbMuxerStarted = 0;
        } else {
            LOGM_WARN("mbMuxerStarted %d?\n", mbMuxerStarted);
            return EECode_OK;
        }
    } else {
        LOGM_FATAL("[Internal bug], NULL mpMuxer %p?\n", mpMuxer);
        return EECode_InternalLogicalBug;
    }

    return EECode_OK;
}

EECode CMuxer::Run()
{
    //debug assert
    DASSERT(mpWorkQ);
    DASSERT(mpMuxer);
    DASSERT(mpEngineMsgSink);
    DASSERT(mpPersistMediaConfig);
    DASSERT(mpInputPins[0]);
    DASSERT(!mbMuxerRunning);
    DASSERT(!mbMuxerStarted);

    DASSERT(mbMuxerContentSetup);

    mpWorkQ->SendCmd(ECMDType_StartRunning);
    mbMuxerRunning = 1;

    return EECode_OK;
}

EECode CMuxer::Start()
{
    //debug assert
    DASSERT(mpWorkQ);
    DASSERT(mpMuxer);
    DASSERT(mpEngineMsgSink);
    DASSERT(mpPersistMediaConfig);
    DASSERT(mpInputPins[0]);
    DASSERT(mbMuxerRunning);
    DASSERT(!mbMuxerStarted);

    DASSERT(mbMuxerContentSetup);

    mpWorkQ->SendCmd(ECMDType_Start);
    mbMuxerStarted = 1;

    return EECode_OK;
}

EECode CMuxer::Stop()
{
    //debug assert
    DASSERT(mpWorkQ);
    DASSERT(mpMuxer);
    DASSERT(mpEngineMsgSink);
    DASSERT(mpPersistMediaConfig);
    DASSERT(mpInputPins[0]);
    DASSERT(!mbMuxerRunning);
    DASSERT(mbMuxerStarted);

    DASSERT(mbMuxerContentSetup);

    mpWorkQ->SendCmd(ECMDType_Stop);
    mbMuxerStarted = 0;

    return EECode_OK;
}

void CMuxer::Pause()
{
    //debug assert
    DASSERT(mpWorkQ);
    DASSERT(mpMuxer);
    DASSERT(mpEngineMsgSink);
    DASSERT(mpPersistMediaConfig);
    DASSERT(mpInputPins[0]);

    DASSERT(mbMuxerContentSetup);

    mpWorkQ->PostMsg(ECMDType_Pause);

    return;
}

void CMuxer::Resume()
{
    //debug assert
    DASSERT(mpWorkQ);
    DASSERT(mpMuxer);
    DASSERT(mpEngineMsgSink);
    DASSERT(mpPersistMediaConfig);
    DASSERT(mpInputPins[0]);

    DASSERT(mbMuxerContentSetup);

    mpWorkQ->PostMsg(ECMDType_Resume);

    return;
}

void CMuxer::Flush()
{
    //debug assert
    DASSERT(mpMuxer);
    DASSERT(mpEngineMsgSink);
    DASSERT(mpPersistMediaConfig);
    DASSERT(mpInputPins[0]);

    DASSERT(mbMuxerContentSetup);

    mpWorkQ->SendCmd(ECMDType_Flush);

    return;
}

void CMuxer::ResumeFlush()
{
    LOGM_FATAL("TO DO\n");
    return;
}

EECode CMuxer::Suspend(TUint release_context)
{
    LOGM_FATAL("TO DO\n");
    return EECode_NoImplement;
}

EECode CMuxer::ResumeSuspend(TComponentIndex input_index)
{
    LOGM_FATAL("TO DO\n");
    return EECode_NoImplement;
}

EECode CMuxer::SwitchInput(TComponentIndex focus_input_index)
{
    LOGM_FATAL("TO DO\n");
    return EECode_NoImplement;
}

EECode CMuxer::FlowControl(EBufferType flowcontrol_type)
{
    LOGM_FATAL("TO DO\n");

    return EECode_NoImplement;
}

EECode CMuxer::AddOutputPin(TUint &index, TUint &sub_index, TUint type)
{
    LOGM_FATAL("CMuxer do not have output pin\n");
    return EECode_InternalLogicalBug;
}

EECode CMuxer::AddInputPin(TUint &index, TUint type)
{
    if (StreamType_Audio == type) {
        if (mnInputPinsNumber >= EConstMaxDemuxerMuxerStreamNumber) {
            LOGM_ERROR("Max input pin number reached.\n");
            return EECode_InternalLogicalBug;
        }

        index = mnInputPinsNumber;
        DASSERT(!mpInputPins[mnInputPinsNumber]);
        if (mpInputPins[mnInputPinsNumber]) {
            LOGM_FATAL("mpInputPins[mnInputPinsNumber] here, must have problems!!! please check it\n");
        }

        mpInputPins[mnInputPinsNumber] = CMuxerInput::Create("[Audio input pin for CMuxer]", (IFilter *) this, mpWorkQ->MsgQ(), type, mnInputPinsNumber);
        DASSERT(mpInputPins[mnInputPinsNumber]);

        mpInputPins[mnInputPinsNumber]->mType = StreamType_Audio;
        mnInputPinsNumber ++;
        return EECode_OK;
    } else if (StreamType_Video == type) {
        if (mnInputPinsNumber >= EConstMaxDemuxerMuxerStreamNumber) {
            LOGM_ERROR("Max input pin number reached.\n");
            return EECode_InternalLogicalBug;
        }

        index = mnInputPinsNumber;
        DASSERT(!mpInputPins[mnInputPinsNumber]);
        if (mpInputPins[mnInputPinsNumber]) {
            LOGM_FATAL("mpInputPins[mnInputPinsNumber] here, must have problems!!! please check it\n");
        }

        mpInputPins[mnInputPinsNumber] = CMuxerInput::Create("[Video input pin for CMuxer]", (IFilter *) this, mpWorkQ->MsgQ(), (StreamType) type, mnInputPinsNumber);
        DASSERT(mpInputPins[mnInputPinsNumber]);

        mpInputPins[mnInputPinsNumber]->mType = StreamType_Video;
        mnInputPinsNumber ++;
        return EECode_OK;
    } else {
        LOGM_FATAL("BAD input pin type %d\n", type);
        return EECode_InternalLogicalBug;
    }

    return EECode_InternalLogicalBug;
}

IOutputPin *CMuxer::GetOutputPin(TUint index, TUint sub_index)
{
    LOGM_FATAL("CMuxer do not have output pin\n");
    return NULL;
}

IInputPin *CMuxer::GetInputPin(TUint index)
{
    if (EConstAudioRendererMaxInputPinNumber > index) {
        return mpInputPins[index];
    } else {
        LOGM_ERROR("BAD index %d, max value %d, in CMuxer::GetInputPin()\n", index, EConstAudioRendererMaxInputPinNumber);
    }

    return NULL;
}

EECode CMuxer::FilterProperty(EFilterPropertyType property, TUint is_set, void *p_param)
{
    EECode ret = EECode_OK;

    switch (property) {

        case EFilterPropertyType_update_sink_url:
            if (DLikely(p_param)) {
                if (is_set) {
                    if (mbMuxerRunning) {
                        SCMD cmd;

                        cmd.code = ECMDType_UpdateUrl;
                        cmd.pExtra = (void *) p_param;
                        cmd.needFreePExtra = 0;

                        mpWorkQ->SendCmd(cmd);
                    } else {
                        if (p_param) {
                            ret = setOutputFile((TChar *)p_param);
                            DASSERT_OK(ret);

                            DASSERT(mpOutputFileNameBase);
                            analyseFileNameFormat(mpOutputFileNameBase);
                        } else {

                        }
                    }
                } else {
                    LOGM_ERROR("EFilterPropertyType_update_sink_url: TODO\n");
                }
            }
            break;

        case EFilterPropertyType_muxer_stop:
            LOGM_ERROR("TO DO\n");
            break;

        case EFilterPropertyType_muxer_start:
            LOGM_ERROR("TO DO\n");
            break;

        case EFilterPropertyType_muxer_finalize_file:
            LOGM_ERROR("TO DO\n");
            break;

        case EFilterPropertyType_muxer_saving_strategy:
            if (DLikely(p_param)) {
                if (is_set) {
                    if (mbMuxerRunning) {
                        SCMD cmd;

                        cmd.code = ECMDType_UpdateRecSavingStrategy;
                        cmd.pExtra = p_param;
                        cmd.needFreePExtra = 0;

                        ret = mpWorkQ->SendCmd(cmd);
                    } else {
                        LOGM_ERROR("EFilterPropertyType_muxer_saving_strategy, muxer not running\n");
                        ret = EECode_BadState;
                    }
                } else {
                    LOGM_ERROR("EFilterPropertyType_muxer_saving_strategy: TODO\n");
                }
            } else {
                LOGM_ERROR("NULL p_param\n");
                ret = EECode_BadParam;
            }
            break;

        default:
            LOGM_FATAL("BAD property 0x%08x\n", property);
            ret = EECode_InternalLogicalBug;
            break;
    }

    return ret;
}

void CMuxer::GetInfo(INFO &info)
{
    info.mPriority = 0;
    info.mFlags = 0;
    info.nInput = mnInputPinsNumber;
    info.nOutput = 1;

    info.pName = mpModuleName;
}

void CMuxer::PrintStatus()
{
    TUint i = 0;

    LOGM_PRINTF("msState=%d, %s, mnInputPinsNumber %d, mpMuxer %p, heart beat %d\n", msState, gfGetModuleStateString(msState), mnInputPinsNumber, mpMuxer, mDebugHeartBeat);

    for (i = 0; i < mnInputPinsNumber; i ++) {
        LOGM_PRINTF("       inputpin[%p, %d]'s status:\n", mpInputPins[i], i);
        if (mpInputPins[i]) {
            mpInputPins[i]->PrintStatus();
        }
    }

    if (mpMuxer) {
        mpMuxer->PrintStatus();
    }

    mDebugHeartBeat = 0;

    return;
}

EECode CMuxer::setOutputFile(TChar *pFileName)
{
    //param check
    if (!pFileName) {
        LOGM_FATAL("NULL params in CMuxer::SetOutputFile.\n");
        return EECode_BadParam;
    }

    TUint currentLen = strlen(pFileName) + 1;

    LOGM_INFO("CMuxer::SetOutputFile len %d, filename %s.\n", currentLen, pFileName);
    if (!mpOutputFileNameBase || (currentLen + 4) > mOutputFileNameBaseLength) {
        if (mpOutputFileNameBase) {
            free(mpOutputFileNameBase);
            mpOutputFileNameBase = NULL;
        }

        mOutputFileNameBaseLength = currentLen + 4;
        if ((mpOutputFileNameBase = (char *)malloc(mOutputFileNameBaseLength)) == NULL) {
            mOutputFileNameBaseLength = 0;
            return EECode_NoMemory;
        }
    }

    DASSERT(mpOutputFileNameBase);
    strncpy(mpOutputFileNameBase, pFileName, currentLen - 1);
    mpOutputFileNameBase[currentLen - 1] = 0x0;
    LOGM_INFO("CMuxer::SetOutputFile done, len %d, filenamebase %s.\n", mOutputFileNameBaseLength, mpOutputFileNameBase);
    return EECode_OK;
}

EECode CMuxer::saveStreamInfo(StreamType type, TUint stream_index, CIBuffer *p_buffer)
{
    DASSERT(stream_index < EConstMaxDemuxerMuxerStreamNumber);

    DASSERT(p_buffer);
    if (!p_buffer) {
        LOGM_FATAL("NULL p_buffer %p\n", p_buffer);
        return EECode_BadParam;
    }

    LOGM_INFO("saveStreamInfo[%d]\n", stream_index);
    if (stream_index < EConstMaxDemuxerMuxerStreamNumber) {
        if (!mStreamCodecInfos.info[stream_index].inited) {
            mStreamCodecInfos.info[stream_index].inited = 1;
            mStreamCodecInfos.total_stream_number ++;
        }
        DASSERT(mStreamCodecInfos.total_stream_number <= EConstMaxDemuxerMuxerStreamNumber);

        mStreamCodecInfos.info[stream_index].stream_type = type;

        switch (type) {

            case StreamType_Video:
                mStreamCodecInfos.info[stream_index].stream_index = stream_index;
                mStreamCodecInfos.info[stream_index].stream_enabled = 1;
                mStreamCodecInfos.info[stream_index].stream_format = StreamFormat_H264;//hard code here

                mStreamCodecInfos.info[stream_index].spec.video.pic_width = p_buffer->mVideoWidth;
                mStreamCodecInfos.info[stream_index].spec.video.pic_height = p_buffer->mVideoHeight;
                mStreamCodecInfos.info[stream_index].spec.video.pic_offset_x = 0;
                mStreamCodecInfos.info[stream_index].spec.video.pic_offset_y = 0;
                mStreamCodecInfos.info[stream_index].spec.video.sample_aspect_ratio_num = p_buffer->mVideoSampleAspectRatioNum;
                mStreamCodecInfos.info[stream_index].spec.video.sample_aspect_ratio_den = p_buffer->mVideoSampleAspectRatioDen;

                DASSERT(p_buffer->mVideoFrameRateDen);
                if (p_buffer->mVideoFrameRateDen && p_buffer->mVideoFrameRateNum) {
                    mStreamCodecInfos.info[stream_index].spec.video.framerate = (p_buffer->mVideoFrameRateNum / p_buffer->mVideoFrameRateDen);
                    mStreamCodecInfos.info[stream_index].spec.video.framerate_num = p_buffer->mVideoFrameRateNum;
                    mStreamCodecInfos.info[stream_index].spec.video.framerate_den = p_buffer->mVideoFrameRateDen;
                } else {
                    mStreamCodecInfos.info[stream_index].spec.video.framerate = 30;
                    mStreamCodecInfos.info[stream_index].spec.video.framerate_num = DDefaultVideoFramerateNum;
                    mStreamCodecInfos.info[stream_index].spec.video.framerate_den = DDefaultVideoFramerateDen;
                }
                mStreamCodecInfos.info[stream_index].spec.video.bitrate = p_buffer->mVideoBitrate;

                if (mpInputPins[stream_index]) {
                    mpInputPins[stream_index]->mBitrate = p_buffer->mVideoBitrate;
                }
                break;

            case StreamType_Audio:
                mStreamCodecInfos.info[stream_index].stream_index = stream_index;
                mStreamCodecInfos.info[stream_index].stream_enabled = 1;
                mStreamCodecInfos.info[stream_index].stream_format = StreamFormat_AAC;//hard code here

                mStreamCodecInfos.info[stream_index].spec.audio.sample_rate = p_buffer->mAudioSampleRate;
                mStreamCodecInfos.info[stream_index].spec.audio.sample_format = (AudioSampleFMT)p_buffer->mAudioSampleFormat;
                mStreamCodecInfos.info[stream_index].spec.audio.channel_number = p_buffer->mAudioChannelNumber;
                mStreamCodecInfos.info[stream_index].spec.audio.frame_size = p_buffer->mAudioFrameSize;
                mStreamCodecInfos.info[stream_index].spec.audio.bitrate = p_buffer->mAudioBitrate;
                if (mpInputPins[stream_index]) {
                    mpInputPins[stream_index]->mBitrate = p_buffer->mAudioBitrate;
                }
                break;

            case StreamType_Subtitle:
                LOGM_FATAL("TO DO\n");
                return EECode_NoImplement;
                break;

            case StreamType_PrivateData:
                LOGM_FATAL("TO DO\n");
                return EECode_NoImplement;
                break;

            default:
                LOGM_FATAL("BAD StreamType %d\n", type);
                return EECode_InternalLogicalBug;
                break;
        }
    } else {
        LOGM_FATAL("BAD stream_index %d\n", stream_index);
        return EECode_BadParam;
    }

    return EECode_OK;
}

EECode CMuxer::saveExtraData(StreamType type, TUint stream_index, void *p_data, TUint data_size)
{
    DASSERT(stream_index < EConstMaxDemuxerMuxerStreamNumber);

    DASSERT(p_data);
    DASSERT(data_size);
    if (!p_data || !data_size) {
        LOGM_FATAL("NULL p_data %p, or zero data_size %d\n", p_data, data_size);
        return EECode_BadParam;
    }

    LOGM_INFO("saveExtraData[%d], data %p, data size %d\n", stream_index, p_data, data_size);
    if (stream_index < EConstMaxDemuxerMuxerStreamNumber) {
        if (mpExtraData[stream_index]) {
            if (mExtraDataSize[stream_index] >= data_size) {
                LOGM_INFO("replace extra data\n");
                memcpy(mpExtraData[stream_index], p_data, data_size);
                mExtraDataSize[stream_index] = data_size;
            } else {
                LOGM_INFO("replace extra data, with larger size\n");
                free(mpExtraData[stream_index]);
                mpExtraData[stream_index] = (TU8 *)malloc(data_size);
                if (mpExtraData[stream_index]) {
                    memcpy(mpExtraData[stream_index], p_data, data_size);
                    mExtraDataSize[stream_index] = data_size;
                } else {
                    LOGM_FATAL("malloc(%d) fail\n", data_size);
                    return EECode_NoMemory;
                }
            }
        } else {
            mpExtraData[stream_index] = (TU8 *)malloc(data_size);
            if (mpExtraData[stream_index]) {
                memcpy(mpExtraData[stream_index], p_data, data_size);
                mExtraDataSize[stream_index] = data_size;
            } else {
                LOGM_FATAL("malloc(%d) fail\n", data_size);
                return EECode_NoMemory;
            }
        }
    } else {
        LOGM_FATAL("BAD stream_index %d\n", stream_index);
        return EECode_BadParam;
    }

    return EECode_OK;
}

EECode CMuxer::setExtraData()
{
    TUint i = 0;
    EECode err = EECode_OK;

    DASSERT(mpMuxer);
    if (mpMuxer) {
        DASSERT(mnInputPinsNumber);
        for (i = 0; i < mnInputPinsNumber; i++) {
            DASSERT(mpExtraData[i]);
            DASSERT(mpInputPins[i]);
            if (mpExtraData[i]) {
                err = mpMuxer->SetExtraData(mpInputPins[i]->mType, i, mpExtraData[i], mExtraDataSize[i]);
                DASSERT_OK(err);
            }
        }
    } else {
        LOGM_FATAL("NULL mpMuxer\n");
        return EECode_BadParam;
    }

    return EECode_OK;
}

void CMuxer::deletePartialFile(TUint file_index)
{
    TChar *cmd = (char *)malloc(mOutputFileNameLength + 16);
    TInt ret = 0;

    if (NULL == cmd) {
        LOGM_ERROR("Cannot malloc memory in CMuxer::deletePartialFile, request size %d,\n", mOutputFileNameLength + 16);
        return;
    }
    memset(cmd, 0, mOutputFileNameLength + 16);
    LOGM_INFO("start remove file %s_%06d.%s.\n", mpOutputFileNameBase, file_index, gfGetContainerStringFromType(mRequestContainerType));
    snprintf(cmd, mOutputFileNameLength + 16, "rm %s_%06d.%s", mpOutputFileNameBase, file_index, gfGetContainerStringFromType(mRequestContainerType));
    LOGM_INFO("cmd: %s.\n", cmd);
    ret = system(cmd);
    DASSERT(0 == ret);
    LOGM_INFO("end remove file %s_%06d.%s.\n", mpOutputFileNameBase, file_index, gfGetContainerStringFromType(mRequestContainerType));
    free(cmd);
}

void CMuxer::analyseFileNameFormat(TChar *pstart)
{
    char *pcur = NULL;
    ContainerType type_fromname;

    if (!pstart) {
        LOGM_ERROR("NULL mpOutputFileNameBase.\n");
        return;
    }

    LOGM_NOTICE("[analyseFileNameFormat], input %s.\n", pstart);

    //check if need append
    pcur = strrchr(pstart, '.');
    if (pcur) {
        pcur ++;
        type_fromname = gfGetContainerTypeFromString(pcur);
        if (ContainerType_Invalid == type_fromname) {
            LOGM_WARN("[analyseFileNameFormat], not supported extention %s, append correct file externtion.\n", pcur);
            mFileNameHanding1 = eFileNameHandling_appendExtention;
        } else {
            LOGM_INFO("[analyseFileNameFormat], have valid file name externtion %s.\n", pcur);
            mFileNameHanding1 = eFileNameHandling_noAppendExtention;
            DASSERT(type_fromname == mRequestContainerType);
            if (type_fromname != mRequestContainerType) {
                LOGM_WARN("[analyseFileNameFormat], container type(%d) from file name not match preset value %d, app need invoke setOutputFormat(to AMRecorder), and make sure setOutput(to engine)'s parameters consist?\n", type_fromname,  mRequestContainerType);
            }
        }
    } else {
        LOGM_INFO("[analyseFileNameFormat], have no file name externtion.\n");
        mFileNameHanding1 = eFileNameHandling_appendExtention;
    }

    //check if need insert, only process one %, todo
    pcur = strchr(pstart, '%');
    //have '%'
    if (pcur) {
        pcur ++;
        if ('t' == (*pcur)) {
            //if have '%t'
            LOGM_INFO("[analyseFileNameFormat], file name is insertDataTime mode.\n");
            mFileNameHanding2 = eFileNameHandling_insertDateTime;
            *pcur = 's';//use '%s', to do
        } else if (('d' == (*pcur)) || ('d' == (*(pcur + 1))) || ('d' == (*(pcur + 2)))) {
            //if have '%d' or '%06d'
            LOGM_INFO("[analyseFileNameFormat], file name is insertFileNumber mode.\n");
            mFileNameHanding2 = eFileNameHandling_insertFileNumber;
        }
    } else {
        LOGM_INFO("[analyseFileNameFormat], file name not need insert something.\n");
        mFileNameHanding2 = eFileNameHandling_noInsert;
    }

    LOGM_INFO("[analyseFileNameFormat], done, input %s, result mFileNameHanding1 %d, mFileNameHanding2 %d.\n", pstart, mFileNameHanding1, mFileNameHanding2);
}

//update file name and thumbnail name
void CMuxer::updateFileName(TUint file_index)
{
    LOGM_INFO("before append extntion: Muxing file name %s, mFileNameHanding1 %d, mFileNameHanding2 %d.\n", mpOutputFileName, mFileNameHanding1, mFileNameHanding2);

    //ffmpeg use extention to specify container type
    /*if (strncmp("sharedfd://", mpOutputFileName, strlen("sharedfd://")) == 0) {
        LOGM_WARN("shared fd, not append file externtion.\n");
        return;
    }*/

    if (file_index > 30) {
        file_index = 0;
    }

    if (!mpOutputFileName || mOutputFileNameLength < (strlen(mpOutputFileNameBase) + DMaxFileExterntionLength + DMaxFileIndexLength + 4)) {
        if (mpOutputFileName) {
            //debug log
            LOGM_WARN("filename buffer(%p) not enough, remalloc it, or len %d, request len %lu.\n", mpOutputFileName, mOutputFileNameLength, (TULong)(strlen(mpOutputFileNameBase) + DMaxFileExterntionLength + DMaxFileIndexLength + 4));
            free(mpOutputFileName);
            mpOutputFileName = NULL;
        }
        mOutputFileNameLength = strlen(mpOutputFileNameBase) + DMaxFileExterntionLength + DMaxFileIndexLength + 4;
        if ((mpOutputFileName = (char *)malloc(mOutputFileNameLength)) == NULL) {
            mOutputFileNameLength = 0;
            LOGM_ERROR("No Memory left, how to handle this issue.\n");
            return;
        }
    }

    if (eFileNameHandling_appendExtention == mFileNameHanding1) {
        if (eFileNameHandling_noInsert == mFileNameHanding2) {
            if (MuxerSavingFileStrategy_ToTalFile == mSavingFileStrategy) {
                snprintf(mpOutputFileName, mOutputFileNameLength, "%s.%s", mpOutputFileNameBase, gfGetContainerStringFromType(mRequestContainerType));
            } else {
                if (MuxerAutoFileNaming_ByNumber == mAutoFileNaming) {
                    snprintf(mpOutputFileName, mOutputFileNameLength, "%s_%06d.%s", mpOutputFileNameBase, file_index, gfGetContainerStringFromType(mRequestContainerType));
                } else if (MuxerAutoFileNaming_ByDateTime == mAutoFileNaming) {
                    char datetime_buffer[128];
                    gfGetSTRFTimeString(datetime_buffer, sizeof(datetime_buffer));
                    snprintf(mpOutputFileName, mOutputFileNameLength, "%s%s.%s", mpOutputFileNameBase, datetime_buffer, gfGetContainerStringFromType(mRequestContainerType));
                }
            }
        } else if (eFileNameHandling_insertFileNumber == mFileNameHanding2) {
            snprintf(mpOutputFileName, mOutputFileNameLength - 8, mpOutputFileNameBase, file_index);
            strncat(mpOutputFileName, ".", mOutputFileNameLength);
            strncat(mpOutputFileName, gfGetContainerStringFromType(mRequestContainerType), mOutputFileNameLength);
        } else if (eFileNameHandling_insertDateTime == mFileNameHanding2) {
            char datetime_buffer[128];
            gfGetSTRFTimeString(datetime_buffer, sizeof(datetime_buffer));

            snprintf(mpOutputFileName, mOutputFileNameLength - 8, mpOutputFileNameBase, datetime_buffer);
            strncat(mpOutputFileName, ".", mOutputFileNameLength);
            strncat(mpOutputFileName, gfGetContainerStringFromType(mRequestContainerType), mOutputFileNameLength);
        } else {
            DASSERT(0);
        }
    } else if (eFileNameHandling_noAppendExtention == mFileNameHanding1) {
        if (eFileNameHandling_insertFileNumber == mFileNameHanding2) {
            snprintf(mpOutputFileName, mOutputFileNameLength, mpOutputFileNameBase, file_index);
        } else if (eFileNameHandling_insertDateTime == mFileNameHanding2) {
            char datetime_buffer[128];
            gfGetSTRFTimeString(datetime_buffer, sizeof(datetime_buffer));
            snprintf(mpOutputFileName, mOutputFileNameLength, mpOutputFileNameBase, datetime_buffer);
        } else {
            if (eFileNameHandling_noInsert != mFileNameHanding2) {
                LOGM_ERROR("ERROR parameters(mFileNameHanding2 %d) here, please check.\n", mFileNameHanding2);
            }

            if (MuxerSavingFileStrategy_ToTalFile == mSavingFileStrategy) {
                //do nothing
                strncpy(mpOutputFileName, mpOutputFileNameBase, mOutputFileNameLength - 1);
                mpOutputFileName[mOutputFileNameLength - 1] = 0x0;
            } else {
                char *ptmp = strrchr(mpOutputFileNameBase, '.');
                DASSERT(ptmp);
                if (ptmp) {
                    *ptmp++ = '\0';

                    //insert if auto cut
                    if (MuxerAutoFileNaming_ByNumber == mAutoFileNaming) {
                        snprintf(mpOutputFileName, mOutputFileNameLength, "%s%06d.%s", mpOutputFileNameBase, file_index, ptmp);
                    } else if (MuxerAutoFileNaming_ByDateTime == mAutoFileNaming) {
                        char datetime_buffer[128];
                        gfGetSTRFTimeString(datetime_buffer, sizeof(datetime_buffer));
                        snprintf(mpOutputFileName, mOutputFileNameLength, "%s%s.%s", mpOutputFileNameBase, datetime_buffer, ptmp);
                    } else {
                        DASSERT(0);
                    }
                    *(ptmp - 1) = '.';
                } else {
                    LOGM_ERROR("SHOULD not comes here, please check code.\n");
                    strncpy(mpOutputFileName, mpOutputFileNameBase, mOutputFileNameLength - 1);
                    mpOutputFileName[mOutputFileNameLength - 1] = 0x0;
                }
            }
        }
    }

    LOGM_INFO("after append extntion: Muxing file name %s\n", mpOutputFileName);
    DASSERT((TUint)(strlen(mpOutputFileName)) <= mOutputFileNameLength);

    if (mpPersistMediaConfig->encoding_mode_config.thumbnail_enabled && mpThumbNailFileNameBase) {
        LOGM_INFO("before append extntion: Thumbnail file name %s", mpThumbNailFileName);
        //param check
        if (!mpThumbNailFileName || mThumbNailFileNameLength < (strlen(mpThumbNailFileNameBase) + DMaxFileExterntionLength + DMaxFileIndexLength + 4)) {
            if (mpThumbNailFileName) {
                //debug log
                LOGM_WARN("thumbnail file name buffer(%p) not enough, remalloc it, or len %d, request len %lu.\n", mpThumbNailFileName, mThumbNailFileNameLength, (TULong)(strlen(mpThumbNailFileNameBase) + DMaxFileExterntionLength + DMaxFileIndexLength + 4));
                free(mpThumbNailFileName);
                mpThumbNailFileName = NULL;
            }
            mThumbNailFileNameLength = strlen(mpThumbNailFileNameBase) + DMaxFileExterntionLength + DMaxFileIndexLength + 4;
            if ((mpThumbNailFileName = (char *)malloc(mThumbNailFileNameLength)) == NULL) {
                mThumbNailFileNameLength = 0;
                LOGM_ERROR("No Memory left, how to handle this issue.\n");
                return;
            }
        }

        //the name of thumbnail is like as .XXXXfile_index.thm
        snprintf(mpThumbNailFileName, mThumbNailFileNameLength, mpThumbNailFileNameBase, file_index);
        LOGM_INFO("after append extntion: Thumbnail file name %s\n", mpThumbNailFileName);
    }
}

TU8 CMuxer::isCommingPTSNotContinuous(CMuxerInput *pInput, CIBuffer *pBuffer)
{
    DASSERT(pInput);
    DASSERT(pBuffer);

    TTime pts = pBuffer->GetBufferLinearPTS() - pInput->mSessionInputPTSStartPoint + pInput->mSessionPTSStartPoint;
    if ((pInput->mCurrentDTS > (pts + MaxDTSDrift * pInput->mDuration)) || (pts > (pInput->mCurrentDTS + MaxDTSDrift * pInput->mDuration))) {
        LOGM_WARN("PTS gap more than %d x duration, DTS %lld, PTS %lld, BufferPTS %lld ,duration %lld, pInput->mSessionPTSStartPoint %lld, pInput->mSessionPTSStartPoint %lld, up-stream filter discarded packet? re-align pts.\n", \
                  MaxDTSDrift, pInput->mCurrentDTS, pts, pBuffer->GetBufferLinearPTS(), pInput->mDuration, pInput->mSessionPTSStartPoint, pInput->mSessionPTSStartPoint);
        return 1;
    }
    return 0;
}

TU8 CMuxer::isCommingBufferAutoFileBoundary(CMuxerInput *pInputPin, CIBuffer *pBuffer)
{
    TTime pts = 0;
    DASSERT(mpMasterInput);
    DASSERT(pInputPin);
    DASSERT(pBuffer);

    pts = pBuffer->GetBufferLinearPTS();
    if ((pInputPin == mpMasterInput) && (EPredefinedPictureType_IDR == pBuffer->mVideoFrameType)) {
        //master video input pin
        DASSERT(pInputPin->mType == StreamType_Video);
        LOGM_DEBUG("[AUTO-CUT]master's IDR comes.\n");
        if (!mbMasterStarted) {
            return 0;
        }
        if (mSavingCondition == MuxerSavingCondition_FrameCount) {
            if ((pInputPin->mCurrentFrameCount + 1) < mAutoSaveFrameCount) {
                return 0;
            }
            return 1;
        } else if (mSavingCondition == MuxerSavingCondition_InputPTS) {
            //check input pts
            if (pts < mNextFileTimeThreshold) {
                return 0;
            }
            //AMLOG_WARN(" Last  PTS = %llu, mAutoSavingTimeDuration = %llu\n", pBuffer->GetBufferLinearPTS(), mAutoSavingTimeDuration);
            return 1;
        } else if (mSavingCondition == MuxerSavingCondition_CalculatedPTS) {
            //check calculated pts
            TS64 comming_pts = pts - pInputPin->mSessionInputPTSStartPoint + pInputPin->mSessionPTSStartPoint;
            if (comming_pts < mNextFileTimeThreshold) {
                return 0;
            }
            //AMLOG_WARN(" Last  PTS = %llu, mAutoSavingTimeDuration = %llu\n", pBuffer->GetBufferLinearPTS(), mAutoSavingTimeDuration);
            return 1;
        } else {
            LOGM_ERROR("NEED implement.\n");
            return 0;
        }
    } else if ((pInputPin != mpMasterInput) && (pInputPin->mType != StreamType_Video) && (_pinNeedSync((StreamType)pInputPin->mType))) {

        if (!mpPersistMediaConfig->cutfile_with_precise_pts) {
            //ignore non-master(non-video)'s pts
            return 0;
        }

        //no non-key frame condition
        if (pts < mAudioNextFileTimeThreshold) {
            return 0;
        }
        return 1;
    } else if ((pInputPin != mpMasterInput) && (pInputPin->mType == StreamType_Video)) {
        //other cases, like multi-video inputpin's case, need implement
        LOGM_ERROR("NEED implement.\n");
    }

    return 0;
}

TU8 CMuxer::isReachPresetConditions(CMuxerInput *pinput)
{
    if ((StreamType_Video == pinput->mType) && (mPresetCheckFlag & EPresetCheckFlag_videoframecount)) {
        if (pinput->mTotalFrameCount >= mPresetVideoMaxFrameCnt) {
            mPresetReachType = EPresetCheckFlag_videoframecount;
            return 1;
        }
    } else if ((StreamType_Audio == pinput->mType) && (mPresetCheckFlag & EPresetCheckFlag_audioframecount)) {
        if (pinput->mTotalFrameCount >= mPresetAudioMaxFrameCnt) {
            mPresetReachType = EPresetCheckFlag_audioframecount;
            return 1;
        }
    } else if ((mPresetCheckFlag & EPresetCheckFlag_filesize) && (mCurrentTotalFilesize >= mPresetMaxFilesize)) {
        mPresetReachType = EPresetCheckFlag_filesize;
        return 1;
    }
    return 0;
}

TU8 CMuxer::hasPinNeedReachBoundary(TUint &i)
{
    for (i = 0; i < mnInputPinsNumber; i++) {
        if (mpInputPins[i]) {
            if (_pinNeedSync((StreamType)mpInputPins[i]->mType) && (0 == mpInputPins[i]->mbAutoBoundaryReached)) {
                return 1;
            }
        }
    }
    i = 0;
    return 0;
}

TU8 CMuxer::allInputEos()
{
    TUint index;
    for (index = 0; index < mnInputPinsNumber; index ++) {
        DASSERT(mpInputPins[index]);
        if (mpInputPins[index]) {
            if (mpInputPins[index]->mEOSComes == 0) {
                return 0;
            }
        } else {
            LOGM_FATAL("NULL mpInputPins[%d], should have error.\n", index);
        }
    }
    return 1;
}

void CMuxer::postMsg(TUint msg_code)
{
    DASSERT(mpEngineMsgSink);
    if (mpEngineMsgSink) {
        SMSG msg;
        msg.code = msg_code;
        LOGM_INFO("Post msg_code %d to engine.\n", msg_code);
        mpEngineMsgSink->MsgProc(msg);
        LOGM_INFO("Post msg_code %d to engine done.\n", msg_code);
    }
}

TU8 CMuxer::readInputData(CMuxerInput *inputpin, EBufferType &type)
{
    DASSERT(!mpBuffer);
    DASSERT(inputpin);
    EECode ret;

    if (!inputpin->PeekBuffer(mpBuffer)) {
        LOGM_FATAL("No buffer?\n");
        return 0;
    }

    type = mpBuffer->GetBufferType();

    if (EBufferType_FlowControl_EOS == type) {
        LOGM_INFO("CMuxer %p get EOS.\n", this);
        inputpin->mEOSComes = 1;
        return 1;
    } else if (EBufferType_FlowControl_Pause == type) {
        LOGM_INFO("pause packet comes.\n");
        inputpin->timeDiscontinue();
        inputpin->mbSkiped = 1;

        if (mbMuxerPaused == 0) {
            mbMuxerPaused = 1;
        } else {
            //get second stream's pause packet, finalize file
            LOGM_INFO("Muxer stream get FlowControl_pause packet, finalize current file!\n");
            ret = finalizeFile();
            if (ret != EECode_OK) {
                LOGM_ERROR("FlowControl_pause processing, finalize file failed!\n");
            }
        }
    } else if (EBufferType_FlowControl_Resume == type) {
        LOGM_INFO("resume packet comes.\n");
        inputpin->mbSkiped = 0;

        if (mbMuxerPaused) {
            LOGM_INFO("Muxer get FlowControl_resume packet, initialize new file!\n");
            ++mCurrentFileIndex;
            updateFileName(mCurrentFileIndex);
            ret = initializeFile(mpOutputFileName, mRequestContainerType);
            if (ret != EECode_OK) {
                LOGM_ERROR("FlowControl_resume processing, initialize file failed!\n");
                inputpin->mbSkiped = 1;
            }
            mbMuxerPaused = 0;
        }
    } else if (_isMuxerInputData(type)) {
        if (((TTime)DINVALID_VALUE_TAG_64) == mpBuffer->GetBufferLinearPTS()) {
            LOGM_WARN("no pts, estimate it\n");
            mpBuffer->SetBufferPTS(inputpin->mCurrentPTS + inputpin->mDuration);
        }
    }


    //DASSERT(inputpin->mbSkiped == 0);
    if (inputpin->mbSkiped == 1) {
        LOGM_WARN("recieved data between flow_control pause and flow_control resume, discard packet here.\n");
        mpBuffer->Release();
        mpBuffer = NULL;
        return 0;
    }

    DASSERT(mpBuffer);
    DASSERT(mpBuffer->GetDataPtr());
#ifdef AM_DEBUG
    if (!mpBuffer->GetDataPtr()) {
        LOGM_WARN("  Buffer type %d, size %d.\n", mpBuffer->GetBufferType(), mpBuffer->GetDataSize());
    }
#endif
    DASSERT(mpBuffer->GetDataSize());
    return 1;
}

EECode CMuxer::writeData(CIBuffer *pBuffer, CMuxerInput *pPin)
{
    SMuxerDataTrunkInfo data_trunk_info;
    TTime buffer_pts = 0;
    EECode err = EECode_OK;
    DASSERT(pBuffer);
    DASSERT(pPin);

    buffer_pts = pBuffer->GetBufferLinearPTS();
    DASSERT(buffer_pts >= 0);

    if (pPin->mpCachedBuffer) {
        DASSERT(pPin->mpCachedBuffer == pBuffer);
    }

    data_trunk_info.native_dts = pBuffer->GetBufferNativeDTS();
    data_trunk_info.native_pts = pBuffer->GetBufferNativePTS();
    pPin->mInputEndPTS = buffer_pts;

    //LOGM_VERBOSE("[DEBUG]: pPin->mType %d, pPin->mCurrentFrameCount %lld, pBuffer->mVideoFrameType %d\n", pPin->mType, pPin->mCurrentFrameCount, pBuffer->mVideoFrameType);

    if (StreamType_Audio == pPin->mType) {

        //ensure first frame are key frame
        if (pPin->mCurrentFrameCount == 0) {
            DASSERT(pPin->mDuration);
            //estimate current pts
            pPin->mSessionInputPTSStartPoint = buffer_pts;
            if (pPin->mCurrentPTS != mStreamStartPTS) {
                pPin->mSessionPTSStartPoint = pPin->mCurrentPTS;
            }
            LOGM_INFO(" audio mSessionInputPTSStartPoint %lld, mSessionPTSStartPoint %lld, pInput->mDuration %lld.\n", pPin->mSessionInputPTSStartPoint, pPin->mSessionPTSStartPoint, pPin->mDuration);
        }

        if (!pPin->mbAutoBoundaryStarted) {
            pPin->mbAutoBoundaryStarted = true;
            pPin->mInputSatrtPTS = buffer_pts;
            LOGM_INFO("[cut file, boundary start(audio), pts %llu]", pPin->mInputSatrtPTS);
        }

        pPin->mCurrentPTS = buffer_pts - pPin->mSessionInputPTSStartPoint + pPin->mSessionPTSStartPoint;
        //LOGM_PTS("[PTS]:audio mCurrentPTS %lld, pBuffer->GetBufferLinearPTS() %lld, pInput->mTimeUintNum %d, pInput->mTimeUintDen %d.\n", pPin->mCurrentPTS, buffer_pts, pPin->mTimeUintNum, pPin->mTimeUintDen);

        data_trunk_info.pts = data_trunk_info.dts = pPin->mCurrentPTS + pPin->mPTSDTSGap;

        //LOGM_PTS("[PTS]:audio write to file pts %lld, dts %lld, pts gap %lld, duration %lld, pInput->mPTSDTSGap %d.\n", data_trunk_info.pts, data_trunk_info.dts, data_trunk_info.pts - pPin->mLastPTS, pPin->mDuration, pPin->mPTSDTSGap);
        pPin->mLastPTS = data_trunk_info.pts;
        data_trunk_info.duration = pPin->mDuration;
        data_trunk_info.av_normalized_duration = pPin->mAVNormalizedDuration;
        mCurrentTotalFilesize += pBuffer->GetDataSize();

        data_trunk_info.is_key_frame = 1;
        data_trunk_info.stream_index = pPin->mStreamIndex;
        //NOTE: need convert audio PTS from samplerate unit to tick unit, otherwise muxed audio PTS almost less than PcrBase, then audio data will be dropped by VLC
        if (mStreamCodecInfos.info[pPin->mStreamIndex].spec.audio.sample_rate) {
            data_trunk_info.pts = data_trunk_info.dts = data_trunk_info.pts * TICK_PER_SECOND / mStreamCodecInfos.info[pPin->mStreamIndex].spec.audio.sample_rate;
        }
        err = mpMuxer->WriteAudioBuffer(pBuffer, &data_trunk_info);
        DASSERT_OK(err);
    } else if (StreamType_Video == pPin->mType) {

        //master pin
        if ((!mbNextFileTimeThresholdSet) && (MuxerSavingFileStrategy_AutoSeparateFile == mSavingFileStrategy)) {
            mVideoFirstPTS = buffer_pts;
            if (MuxerSavingCondition_InputPTS == mSavingCondition) {
                //start from input pts
                mNextFileTimeThreshold = buffer_pts + mAutoSavingTimeDuration;
            } else if (MuxerSavingCondition_CalculatedPTS == mSavingCondition) {
                //linear to start from 0
                mNextFileTimeThreshold = mAutoSavingTimeDuration;
            }
            mbNextFileTimeThresholdSet = 1;
            LOGM_INFO("Get first Video PTS %llu, mNextFileTimeThreshold %llu.\n", mVideoFirstPTS, mNextFileTimeThreshold);
        }

        LOGM_PTS("video buffer pts %lld\n", buffer_pts);

        //ensure first frame are key frame
        if (0 == pPin->mCurrentFrameCount) {
            //DASSERT(EPredefinedPictureType_IDR == pBuffer->mVideoFrameType);
            if (EPredefinedPictureType_IDR == pBuffer->mVideoFrameType) {
                LOGM_INFO("IDR picture comes, pInput->mDuration %lld, pBuffer->GetBufferLinearPTS() %lld.\n", pPin->mDuration, buffer_pts);

                DASSERT(pPin->mDuration);
                //refresh session pts related
                pPin->mSessionInputPTSStartPoint = buffer_pts;
                if (pPin->mCurrentPTS != mStreamStartPTS) {
                    pPin->mSessionPTSStartPoint = pPin->mCurrentPTS;
                }
                LOGM_INFO(" video pInput->mSessionInputPTSStartPoint %lld, pInput->mSessionPTSStartPoint %lld.\n", pPin->mSessionInputPTSStartPoint, pPin->mSessionPTSStartPoint);
                if (!mbMasterStarted) {
                    LOGM_INFO("master started.\n");
                    mbMasterStarted = 1;
                }
            } else {
                LOGM_INFO("non-IDR frame (%d), data comes here, seq num [%d, %d], when stream not started, discard them.\n", pBuffer->mVideoFrameType, pBuffer->mBufferSeqNumber0, pBuffer->mBufferSeqNumber1);
                return EECode_OK;
            }
        } else if (1 == isCommingPTSNotContinuous(pPin, pBuffer)) {
            //refresh pts, connected to previous
            pPin->mSessionInputPTSStartPoint = pBuffer->GetBufferLinearPTS();
            pPin->mSessionPTSStartPoint = pPin->mCurrentPTS + pPin->mDuration;
            pPin->mCurrentFrameCount = 0;
        }

        pPin->mFrameCountFromLastBPicture ++;

        if (!pPin->mbAutoBoundaryStarted) {
            pPin->mbAutoBoundaryStarted = 1;
            pPin->mInputSatrtPTS = pBuffer->GetBufferLinearPTS();
            LOGM_INFO("[cut file, boundary start(video), pts %llu]", pPin->mInputSatrtPTS);
        }

        if (pBuffer->GetBufferLinearPTS() < pPin->mSessionInputPTSStartPoint) {
            DASSERT(pBuffer->mVideoFrameType == EPredefinedPictureType_B);
            LOGM_WARN(" PTS(%lld) < first IDR PTS(%lld), something should be wrong. or skip b frame(pBuffer->mVideoFrameType %d) after first IDR, these b frame should be skiped, when pause--->resume.\n", buffer_pts, pPin->mSessionInputPTSStartPoint, pBuffer->mVideoFrameType);
            return EECode_OK;
        }

        DASSERT(buffer_pts >= pPin->mSessionInputPTSStartPoint);
        pPin->mCurrentPTS = pBuffer->GetBufferLinearPTS() - pPin->mSessionInputPTSStartPoint + pPin->mSessionPTSStartPoint;
        LOGM_PTS("[PTS]:video mCurrentPTS %lld, mCurrentDTS %lld, pBuffer->GetPTS() %lld, mSessionInputPTSStartPoint %lld, mSessionPTSStartPoint %lld.\n", pPin->mCurrentPTS, pPin->mCurrentDTS, buffer_pts, pPin->mSessionInputPTSStartPoint, pPin->mSessionPTSStartPoint);

        data_trunk_info.dts = pPin->mCurrentDTS;
        data_trunk_info.pts = pPin->mCurrentPTS + pPin->mPTSDTSGap;
        data_trunk_info.duration = pPin->mDuration;
        data_trunk_info.av_normalized_duration = pPin->mAVNormalizedDuration;

        pPin->mCurrentDTS = pPin->mCurrentDTS + pPin->mDuration;

        LOGM_PTS("[PTS]:video write to file pts %lld, dts %lld, pts gap %lld, duration %lld, pInput->mPTSDTSGap %d.\n", data_trunk_info.pts, data_trunk_info.dts, data_trunk_info.pts - pPin->mLastPTS, pPin->mDuration, pPin->mPTSDTSGap);
        pPin->mLastPTS = data_trunk_info.pts;

        data_trunk_info.is_key_frame = ((EPredefinedPictureType_IDR == pBuffer->mVideoFrameType) || (EPredefinedPictureType_I == pBuffer->mVideoFrameType));
        data_trunk_info.stream_index = pPin->mStreamIndex;

        err = mpMuxer->WriteVideoBuffer(pBuffer, &data_trunk_info);
        DASSERT_OK(err);
    } else if (StreamType_PrivateData == pPin->mType) {
        data_trunk_info.pts = buffer_pts;
        data_trunk_info.dts = buffer_pts;
        data_trunk_info.is_key_frame = 1;
        data_trunk_info.stream_index = pPin->mStreamIndex;
        err = mpMuxer->WritePridataBuffer(pBuffer, &data_trunk_info);
        DASSERT_OK(err);
    } else {
        LOGM_ERROR("BAD pin type %d.\n", pPin->mType);
        return EECode_BadFormat;
    }

    pPin->mCurrentFrameCount ++;
    pPin->mTotalFrameCount ++;

    return err;
}

void CMuxer::getFileInformation()
{
    TUint i = 0, tot_validpins = 0;
    TUint estimated_bitrate = 0;
    TTime first_stream_start_pts, first_stream_end_pts, first_stream_duration, first_input_duration;
    TTime diff_start_pts, diff_end_pts, diff_duration, diff_input_duration;

    //avsync related print: start pts's difference, end pts's difference, and duration's difference
    if (mpInputPins[0]) {
        //get first stream's
        first_stream_start_pts = mpInputPins[0]->mInputSatrtPTS;
        first_stream_end_pts = mpInputPins[0]->mInputEndPTS;
        first_input_duration = mpInputPins[0]->mInputEndPTS - mpInputPins[0]->mInputSatrtPTS + mpInputPins[0]->mDuration;
        first_stream_duration = mpInputPins[0]->mCurrentPTS - mpInputPins[0]->mSessionPTSStartPoint + mpInputPins[0]->mDuration;

        LOGM_INFO("[file info]: avsync, first stream start input pts %llu, end input pts %llu, duration %llu.\n", first_stream_start_pts, first_stream_end_pts, first_stream_duration);
        for (i = 1; i < mnInputPinsNumber; i++) {
            if (mpInputPins[i]) {
                LOGM_INFO("[file info]: avsync, stream(%d) start input pts %llu, end input pts %llu, duration %llu.\n", i, mpInputPins[i]->mInputSatrtPTS, mpInputPins[i]->mInputEndPTS, mpInputPins[i]->mCurrentPTS + mpInputPins[i]->mDuration - mpInputPins[i]->mSessionPTSStartPoint);

                if (StreamType_Audio == mpInputPins[i]->mType) {
                    //audio has a duration gap, buffer in encoder
                    diff_start_pts = mpInputPins[i]->mInputSatrtPTS - mpInputPins[i]->mDuration - first_stream_start_pts;
                    diff_end_pts = mpInputPins[i]->mInputEndPTS - mpInputPins[i]->mDuration - first_stream_end_pts;
                } else {
                    diff_start_pts = mpInputPins[i]->mInputSatrtPTS - first_stream_start_pts;
                    diff_end_pts = mpInputPins[i]->mInputEndPTS - first_stream_end_pts;
                }

                diff_input_duration = mpInputPins[i]->mInputEndPTS - mpInputPins[i]->mInputSatrtPTS + mpInputPins[i]->mDuration - first_input_duration;
                diff_duration = mpInputPins[i]->mCurrentPTS - mpInputPins[i]->mSessionPTSStartPoint + mpInputPins[i]->mDuration - first_stream_duration;
                LOGM_INFO("[file info]: avsync, stream(%d) differ with first stream: start input pts's diff %lld, end input pts's diff %lld, duration's diff %lld, input duration diff %lld.\n", i, diff_start_pts, diff_end_pts, diff_duration, diff_input_duration);
            }
        }
    }

    //file duration
    mMuxerFileInfo.file_duration = 0;
    for (i = 0; i < mnInputPinsNumber; i++) {
        if (mpInputPins[i]) {
            if ((StreamType_Video == mpInputPins[i]->mType) || (StreamType_Audio == mpInputPins[i]->mType)) {
                mMuxerFileInfo.file_duration += mpInputPins[i]->mCurrentPTS - mpInputPins[i]->mSessionPTSStartPoint + mpInputPins[i]->mDuration;
                LOGM_INFO("[file info]: pin(%d)'s duration %llu.\n", i, mpInputPins[i]->mCurrentPTS - mpInputPins[i]->mSessionPTSStartPoint + mpInputPins[i]->mDuration);
                tot_validpins ++;
            }
        }
    }

    if (tot_validpins) {
        mMuxerFileInfo.file_duration /= tot_validpins;
    }
    LOGM_INFO("[file info]: avg duration is %llu, number of valid pins %d.\n", mMuxerFileInfo.file_duration, tot_validpins);

    //file size
    //mCurrentTotalFilesize;
    LOGM_INFO("[file info]: mCurrentTotalFilesize %llu.\n", mCurrentTotalFilesize);

    //file bitrate
    estimated_bitrate = 0;
    for (i = 0; i < mnInputPinsNumber; i++) {
        if (mpInputPins[i]) {
            if ((StreamType_Video == mpInputPins[i]->mType) || (StreamType_Audio == mpInputPins[i]->mType)) {
                estimated_bitrate += mpInputPins[i]->mBitrate;
                LOGM_INFO("[file info]: pin(%d)'s bitrate %u.\n", i, mpInputPins[i]->mBitrate);
            }
        }
    }

    //DASSERT(estimated_bitrate);
    DASSERT(mMuxerFileInfo.file_duration);
    mMuxerFileInfo.file_bitrate = 0;
    if (mMuxerFileInfo.file_duration) {
        mMuxerFileInfo.file_bitrate = (TUint)(((float)mCurrentTotalFilesize * 8 * TimeUnitDen_90khz) / ((float)mMuxerFileInfo.file_duration));
        //LOGM_INFO("((float)mCurrentTotalFilesize*8*TimeUnitDen_90khz) %f .\n", ((float)mCurrentTotalFilesize*8*TimeUnitDen_90khz));
        //LOGM_INFO("result %f.\n", (((float)mCurrentTotalFilesize*8*TimeUnitDen_90khz)/((float)mFileDuration)));
        //LOGM_INFO("mFileBitrate %d.\n", mFileBitrate);
    }

    LOGM_INFO("[file info]: pre-estimated bitrate %u, calculated bitrate %u.\n", estimated_bitrate, mMuxerFileInfo.file_bitrate);
}

TU8 CMuxer::processCmd(SCMD &cmd)
{
    LOGM_INFO("cmd.code %d, state %d.\n", cmd.code, msState);
    switch (cmd.code) {
        case ECMDType_ExitRunning:
            mbRun = 0;
            LOGM_INFO("ECMDType_ExitRunning cmd.\n");
            flushInputData();
            finalizeFile();
            msState = EModuleState_Halt;
            mpWorkQ->CmdAck(EECode_OK);
            break;

        case ECMDType_Stop:
            flushInputData();
            finalizeFile();
            msState = EModuleState_WaitCmd;
            mpWorkQ->CmdAck(EECode_OK);
            break;

        case ECMDType_Start:
            if (EModuleState_WaitCmd == msState) {
                msState = EModuleState_Muxer_WaitExtraData;
            }
            mpWorkQ->CmdAck(EECode_OK);
            break;

        case ECMDType_Pause:
            //todo
            DASSERT(!mbPaused);
            mbPaused = 1;
            mpWorkQ->CmdAck(EECode_OK);
            break;

        case ECMDType_Resume:
            //todo
            if (msState == EModuleState_Pending) {
                msState = EModuleState_Idle;
            }
            mbPaused = 0;
            mpWorkQ->CmdAck(EECode_OK);
            break;

        case ECMDType_Flush:
            //should not comes here
            DASSERT(0);
            msState = EModuleState_Pending;
            if (mpBuffer) {
                mpBuffer->Release();
                mpBuffer = NULL;
            }
            mpWorkQ->CmdAck(EECode_OK);
            break;

        case ECMDType_DeleteFile:
            LOGM_INFO("recieve cmd, delete file here, mTotalFileNumber %d, mFirstFileIndexCanbeDeleted %d.\n", mTotalFileNumber, mFirstFileIndexCanbeDeleted);
            deletePartialFile(mFirstFileIndexCanbeDeleted);
            mTotalFileNumber --;
            mFirstFileIndexCanbeDeleted ++;
            break;

        case ECMDType_UpdateUrl: {
                EECode ret = EECode_OK;

                if (cmd.pExtra) {
                    EECode ret = setOutputFile((TChar *)cmd.pExtra);
                    DASSERT_OK(ret);

                    DASSERT(mpOutputFileNameBase);
                    analyseFileNameFormat(mpOutputFileNameBase);

                    if (mpBuffer) {
                        mpBuffer->Release();
                        mpBuffer = NULL;
                    }

                    if (EMuxerModuleID_FFMpeg == mCurMuxerID) {
                        initialize((TChar *)"FFMpeg");
                    } else if (EMuxerModuleID_PrivateTS == mCurMuxerID) {
                        initialize((TChar *)"TS");
                    } else {
                        LOGM_FATAL("no such type %d\n", mCurMuxerID);
                    }
                    LOGM_NOTICE("set filename(%s) here\n", (TChar *)cmd.pExtra);

                    if (EModuleState_WaitCmd == msState) {
                        TUint index = 0;
                        for (index = 0; index < EConstMaxDemuxerMuxerStreamNumber; index ++) {
                            if (mpInputPins[index]) {
                                mpInputPins[index]->Enable(1);
                            }
                        }
                        msState = EModuleState_Idle;
                    }

                } else {
                    if (mpOutputFileName && strlen(mpOutputFileName)) {
                        finalize();
                        TUint index = 0;
                        for (index = 0; index < EConstMaxDemuxerMuxerStreamNumber; index ++) {
                            if (mpInputPins[index]) {
                                mpInputPins[index]->Purge(1);
                            }
                        }
                    }
                    msState = EModuleState_WaitCmd;
                }
                mpWorkQ->CmdAck(ret);
            }
            break;

        case ECMDType_UpdateRecSavingStrategy: {
                SRecSavingStrategy *p_rec_strategy = (SRecSavingStrategy *)cmd.pExtra;
                if (p_rec_strategy) {
                    mSavingFileStrategy = p_rec_strategy->strategy;
                    mAutoFileNaming = p_rec_strategy->naming;
                    if (MuxerSavingCondition_InputPTS == p_rec_strategy->condition
                            || MuxerSavingCondition_CalculatedPTS == p_rec_strategy->condition) {
                        mAutoSavingTimeDuration = p_rec_strategy->param;
                    } else if (MuxerSavingCondition_FrameCount == p_rec_strategy->condition) {
                        mAutoSaveFrameCount = p_rec_strategy->param;
                    } else {
                        mpWorkQ->CmdAck(EECode_BadParam);
                        break;
                    }
                    mSavingCondition = p_rec_strategy->condition;
                    mpWorkQ->CmdAck(EECode_OK);
                } else {
                    mpWorkQ->CmdAck(EECode_BadParam);
                }
            }
            break;

        default:
            LOGM_FATAL("wrong cmd %d.\n", cmd.code);
    }
    return 0;
}

void CMuxer::OnRun()
{
    SCMD cmd;
    CIQueue::QType type;
    CIQueue::WaitResult result;
    CMuxerInput *pPin = NULL;
    //SMSG msg;
    EBufferType buffer_type;
    EECode err;

    mbRun = 1;
    msState = EModuleState_WaitCmd;

    mpWorkQ->CmdAck(EECode_OK);
    LOGM_INFO("OnRun loop, start\n");

    //saving file's variables
    TUint inpin_index = 0;
    CIBuffer *tmp_buffer = NULL;
    TTime new_time_threshold = 0;
    TU64 threshold_diff = 0;

    DASSERT(mpMuxer);
    DASSERT(mpEngineMsgSink);
    DASSERT(mpPersistMediaConfig);
    DASSERT(mpInputPins[0]);

    DASSERT(mbMuxerContentSetup);
    DASSERT(!mbMuxerStarted);


    mpMasterInput = mpInputPins[0];//hard code here

    while (mbRun) {
        LOGM_STATE("OnRun: start switch, msState=%d, %s, mpInputPins[0]->mpBufferQ->GetDataCnt() %d, mnInputPinsNumber %d.\n", msState, gfGetModuleStateString(msState), mpInputPins[0]->mpBufferQ->GetDataCnt(), mnInputPinsNumber);

        switch (msState) {

            case EModuleState_Halt:
                mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
                processCmd(cmd);
                break;

            case EModuleState_Muxer_WaitExtraData:
                if (inpin_index < mnInputPinsNumber) {
                    DASSERT(mpInputPins[inpin_index]);
                    if (mpInputPins[inpin_index]->mbExtraDataComes) {
                        inpin_index ++;
                        break;
                    }

                    type = mpWorkQ->WaitDataMsgWithSpecifiedQueue(&cmd, sizeof(cmd), mpInputPins[inpin_index]->GetBufferQ());
                    if (type == CIQueue::Q_MSG) {
                        processCmd(cmd);
                    } else {
                        if (readInputData(mpInputPins[inpin_index], buffer_type)) {

                            DASSERT(inpin_index == mpInputPins[inpin_index]->mStreamIndex);
                            //LOGM_INFO("mSavingFileStrategy %d, pPin %p, mpMasterInput %p, mpBuffer->GetBufferLinearPTS() %llu, mNextFileTimeThreshold %llu.\n", mSavingFileStrategy, pPin, mpMasterInput, mpBuffer->GetBufferLinearPTS(), mNextFileTimeThreshold);

                            if (_isMuxerInputData(buffer_type) && mpBuffer) {
                                LOGM_WARN("first buffer not with extra data, inpin_index %d\n", inpin_index);
                                //mpInputPins[inpin_index]->Enable(0);
                            } else if (_isMuxerExtraData(buffer_type) && mpBuffer) {
                                err = saveExtraData(mpInputPins[inpin_index]->mType, mpInputPins[inpin_index]->mStreamIndex, (void *)mpBuffer->GetDataPtr(), mpBuffer->GetDataSize());
                                DASSERT_OK(err);
                                err = saveStreamInfo(mpInputPins[inpin_index]->mType, inpin_index, mpBuffer);
                                DASSERT_OK(err);
                                mpInputPins[inpin_index]->mbExtraDataComes = 1;
                            } else if (_isFlowControlBuffer(buffer_type) && mpBuffer) {
                                LOGM_INFO("ignore them\n");
                            } else {
                                LOGM_FATAL("Why comes here, BAD buffer_type %d\n", buffer_type);
                                msState = EModuleState_Error;
                                break;
                            }

                            mpBuffer->Release();
                            mpBuffer = NULL;
                        }
                    }
                } else {
                    if (mpOutputFileNameBase && strlen(mpOutputFileNameBase)) {
                        LOGM_INFO("all streams recieved extra data, start muxing\n");
                        setExtraData();
                        updateFileName(mCurrentFileIndex);
                        initializeFile(mpOutputFileName, mRequestContainerType);
                        msState = EModuleState_Idle;
                    } else {
                        //disable all pins, wait set file name
                        LOGM_INFO("disable all inputpins, wait set filename\n");
                        TUint index = 0;
                        for (index = 0; index < EConstMaxDemuxerMuxerStreamNumber; index ++) {
                            if (mpInputPins[index]) {
                                mpInputPins[index]->Purge(1);
                            }
                        }
                        msState = EModuleState_WaitCmd;
                    }
                }
                break;

            case EModuleState_WaitCmd: {
                    mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
                    processCmd(cmd);
                }
                break;

            case EModuleState_Idle:
                if (mbPaused) {
                    msState = EModuleState_Pending;
                    break;
                }

                type = mpWorkQ->WaitDataMsgCircularly(&cmd, sizeof(cmd), &result);
                if (type == CIQueue::Q_MSG) {
                    processCmd(cmd);
                } else {
                    pPin = (CMuxerInput *)result.pOwner;
                    DASSERT(pPin);
                    if (readInputData(pPin, buffer_type)) {
                        LOGM_DEBUG("mSavingFileStrategy %d, pPin %p, mpMasterInput %p, mpBuffer->GetBufferLinearPTS() %llu, mNextFileTimeThreshold %llu.\n", mSavingFileStrategy, pPin, mpMasterInput, mpBuffer->GetBufferLinearPTS(), mNextFileTimeThreshold);
                        if (_isMuxerInputData(buffer_type) && mpBuffer) {

                            //detect auto saving condition
                            if (MuxerSavingFileStrategy_AutoSeparateFile == mSavingFileStrategy) {
                                if (1 == isCommingBufferAutoFileBoundary(pPin, mpBuffer)) {
                                    if (!mpPersistMediaConfig->cutfile_with_precise_pts) {
                                        DASSERT(NULL == pPin->mpCachedBuffer);
                                        DASSERT(mpMasterInput == pPin);
                                        if (mpMasterInput == pPin) {
                                            DASSERT(EPredefinedPictureType_IDR == mpBuffer->mVideoFrameType);
                                            new_time_threshold = mpBuffer->GetBufferLinearPTS();
                                            LOGM_INFO("[Muxer], detected AUTO saving file, new video IDR threshold %llu, mNextFileTimeThreshold %llu, diff (%lld).\n", new_time_threshold, mNextFileTimeThreshold, new_time_threshold - mNextFileTimeThreshold);
                                            pPin->mpCachedBuffer = mpBuffer;
                                            mpBuffer = NULL;
                                            pPin->mbAutoBoundaryReached = 1;
                                            msState = EModuleState_Muxer_SavingPartialFile;
                                        } else {
                                            LOGM_ERROR("[Muxer], detected AUTO saving file, wrong mpPersistMediaConfig->cutfile_with_precise_pts setting %d.\n", mpPersistMediaConfig->cutfile_with_precise_pts);
                                            writeData(mpBuffer, pPin);
                                            mpBuffer->Release();
                                            mpBuffer = NULL;
                                            msState = EModuleState_Muxer_SavingPartialFilePeekRemainingOnMasterPin;
                                            pPin->mbAutoBoundaryReached = 1;
                                        }
                                    } else {
                                        DASSERT(NULL == pPin->mpCachedBuffer);
                                        if (mpMasterInput == pPin) {
                                            DASSERT(EPredefinedPictureType_IDR == mpBuffer->mVideoFrameType);
                                            new_time_threshold = mpBuffer->GetBufferLinearPTS();
                                            LOGM_INFO("[Muxer], detected AUTO saving file, new video IDR threshold %llu, mNextFileTimeThreshold %llu, diff (%lld).\n", new_time_threshold, mNextFileTimeThreshold, new_time_threshold - mNextFileTimeThreshold);
                                            pPin->mpCachedBuffer = mpBuffer;
                                            mpBuffer = NULL;
                                            pPin->mbAutoBoundaryReached = 1;
                                            msState = EModuleState_Muxer_SavingPartialFilePeekRemainingOnNonMasterPin;
                                        } else {
                                            new_time_threshold = mNextFileTimeThreshold;
                                            LOGM_INFO("[Muxer], detected AUTO saving file, non-master pin detect boundary, pts %llu, mAudioNextFileTimeThreshold %llu, diff %lld, write buffer here.\n", mpBuffer->GetBufferLinearPTS(), mAudioNextFileTimeThreshold, mpBuffer->GetBufferLinearPTS() - mAudioNextFileTimeThreshold);
                                            writeData(mpBuffer, pPin);
                                            mpBuffer->Release();
                                            mpBuffer = NULL;
                                            msState = EModuleState_Muxer_SavingPartialFilePeekRemainingOnMasterPin;
                                            pPin->mbAutoBoundaryReached = 1;
                                        }
                                    }
                                    break;
                                }
                            } else if (mPresetCheckFlag) {
                                //detect preset condition is reached
                                if (1 == isReachPresetConditions(pPin)) {
                                    msState = EModuleState_Muxer_SavingWholeFile;
                                    LOGM_INFO("[Muxer], detected Reach Preset condition, saving file, mCurrentTotalFrameCnt %llu, mCurrentTotalFilesize %llu, mPresetMaxFilesize %llu, mPresetCheckFlag 0x%x.\n", pPin->mTotalFrameCount, mCurrentTotalFilesize, mPresetMaxFilesize, mPresetCheckFlag);
                                    break;
                                }
                            }

                            msState = EModuleState_Running;

                        } else if (_isMuxerExtraData(buffer_type)) {
                            DASSERT(mpBuffer);
                            //LOGM_INFO("extra data comes again, handle it!\n");
                            if (mpBuffer) {
                                if (EBufferType_VideoExtraData == buffer_type) {
                                    DASSERT(mpMasterInput);
                                    DASSERT(pPin->mType == StreamType_Video);
                                    DASSERT(NULL == pPin->mpCachedBuffer);

                                    //LOGM_INFO("extra data(video) comes again\n");
                                    mpBuffer->Release();
                                    mpBuffer = NULL;
                                    break;
                                } else if (EBufferType_AudioExtraData == buffer_type) {
                                    DASSERT(mpMasterInput);
                                    DASSERT(pPin->mType == StreamType_Audio);
                                    DASSERT(NULL == pPin->mpCachedBuffer);

                                    //LOGM_INFO("extra data(audio) comes again\n");
                                    mpBuffer->Release();
                                    mpBuffer = NULL;
                                    break;
                                } else {
                                    mpBuffer->Release();
                                    mpBuffer = NULL;
                                }
                            }
                        } else if (_isFlowControlBuffer(buffer_type)) {
                            DASSERT(mpBuffer);
                            if (mpBuffer) {
                                if (EBufferType_FlowControl_FinalizeFile == buffer_type) {
                                    DASSERT(mpMasterInput);
                                    DASSERT(pPin->mType == StreamType_Video);
                                    DASSERT(NULL == pPin->mpCachedBuffer);
                                    //mpMasterInput = pPin;
                                    pPin->mbAutoBoundaryReached = 1;
                                    mNextFileTimeThreshold = mpBuffer->GetBufferLinearPTS();
                                    new_time_threshold = mNextFileTimeThreshold;
                                    //
                                    mAudioNextFileTimeThreshold = mNextFileTimeThreshold;
                                    LOGM_INFO("[Muxer] manually saving file buffer comes, PTS %llu.\n", new_time_threshold);
                                    mpBuffer->Release();
                                    mpBuffer = NULL;
                                    msState = EModuleState_Muxer_SavingPartialFile;
                                    break;
                                } else if (EBufferType_FlowControl_EOS == buffer_type) {
                                    LOGM_INFO(" [Muxer] FlowControl EOS buffer comes.\n");
                                    mpBuffer->Release();
                                    mpBuffer = NULL;
                                    if (allInputEos()) {
                                        finalizeFile();
                                        postMsg(EMSGType_RecordingEOS);
                                        msState = EModuleState_Completed;
                                        break;
                                    }
                                } else {
                                    mpBuffer->Release();
                                    mpBuffer = NULL;
                                }
                            }
                        } else {
                            if (!_isMuxerInputData(buffer_type)) {
                                LOGM_ERROR("BAD buffer type, %d.\n", buffer_type);
                            }
                            DASSERT(0);
                            if (mpBuffer) {
                                mpBuffer->Release();
                                mpBuffer = NULL;
                            }
                        }

                    } else if (pPin->mbSkiped != 1) {
                        //peek buffer fail. must not comes here
                        LOGM_ERROR("peek buffer fail. must not comes here.\n");
                        msState = EModuleState_Error;
                    }
                }

                break;

            case EModuleState_Running:
                DASSERT(pPin);
                DASSERT(mpBuffer);
                LOGM_DEBUG("ready to write data, type %d.\n", pPin->mType);
                if (pPin->mType == StreamType_Video) {
                    if (pPin->mpCachedBuffer) {
                        writeData(pPin->mpCachedBuffer, pPin);
                        pPin->mpCachedBuffer->Release();
                        pPin->mpCachedBuffer = NULL;
                        if (msState != EModuleState_Running) {
                            if (mpBuffer) {
                                mpBuffer->Release();
                                mpBuffer = NULL;
                            }
                            //LOGM_WARN("something error happened, not goto idle state.\n");
                            break;
                        }
                    }
                    writeData(mpBuffer, pPin);
                    if (msState != EModuleState_Running) {
                        mpBuffer->Release();
                        mpBuffer = NULL;
                        //LOGM_WARN("something error happened, not goto idle state.\n");
                        break;
                    }
                } else if (pPin->mType == StreamType_Audio) {
                    if (pPin->mpCachedBuffer) {
                        writeData(pPin->mpCachedBuffer, pPin);
                        pPin->mpCachedBuffer->Release();
                        pPin->mpCachedBuffer = NULL;
                    }
                    writeData(mpBuffer, pPin);
                } else if (pPin->mType == StreamType_Subtitle) {

                } else if (pPin->mType == StreamType_PrivateData) {
                    writeData(mpBuffer, pPin);
                } else {
                    LOGM_FATAL("must not comes here, BAD input type(%d)? pin %p, buffer %p.\n", pPin->mType, pPin, mpBuffer);
                    msState = EModuleState_Error;
                    break;
                }
                pPin = NULL;

                mpBuffer->Release();
                mpBuffer = NULL;
                if (msState != EModuleState_Running) {
                    LOGM_WARN("something error happened, not goto idle state.\n");
                    break;
                }
                msState = EModuleState_Idle;
                break;

            case EModuleState_Muxer_FlushExpiredFrame: {
                    TTime cur_time = 0;
                    //TUint find_idr = 0;
                    DASSERT(pPin);
                    DASSERT(!mpBuffer);
                    if (mpBuffer) {
                        mpBuffer->Release();
                        mpBuffer = NULL;
                    }
                    if (pPin->mpCachedBuffer) {
                        pPin->mpCachedBuffer->Release();
                        pPin->mpCachedBuffer = NULL;
                    }
                    //only video have this case
                    DASSERT(StreamType_Video == pPin->mType);
                    if (mpClockReference) {
                        cur_time = mpClockReference->GetCurrentTime();
                    }

                    //discard data to next IDR
                    while (1 == readInputData(pPin, buffer_type)) {
                        if (_isMuxerInputData(buffer_type)) {
                            //LOGM_WARN("Discard frame, expired time %llu, cur time %llu, frame type %d, Seq num[%d, %d].\n", mpBuffer->mExpireTime, cur_time, mpBuffer->mVideoFrameType, mpBuffer->mOriSeqNum, mpBuffer->mSeqNum);
                            if (EPredefinedPictureType_IDR != mpBuffer->mVideoFrameType) {
                                mpBuffer->Release();
                                mpBuffer = NULL;
                            } else {
                                if (mpBuffer->mExpireTime > cur_time) {
                                    msState = EModuleState_Running;//continue write data
                                    LOGM_INFO("Get valid IDR... cur time %llu, start time %llu, expird time %llu.\n", mpClockReference->GetCurrentTime(), cur_time, mpBuffer->mExpireTime);
                                    break;
                                } else {
                                    mpBuffer->Release();
                                    mpBuffer = NULL;
                                }
                            }
                        } else if (_isFlowControlBuffer(buffer_type)) {
                            DASSERT(mpBuffer);
                            if (mpBuffer) {
                                if (EBufferType_FlowControl_FinalizeFile == buffer_type) {
                                    DASSERT(mpMasterInput);
                                    DASSERT(pPin->mType == StreamType_Video);
                                    DASSERT(NULL == pPin->mpCachedBuffer);
                                    //mpMasterInput = pPin;
                                    pPin->mbAutoBoundaryReached = 1;
                                    msState = EModuleState_Muxer_SavingPartialFile;
                                    mNextFileTimeThreshold = mpBuffer->GetBufferLinearPTS();
                                    new_time_threshold = mNextFileTimeThreshold;
                                    LOGM_INFO("[Muxer] manually saving file buffer comes, PTS %llu.\n", new_time_threshold);
                                    mpBuffer->Release();
                                    mpBuffer = NULL;
                                    break;
                                } else if (EBufferType_FlowControl_EOS == buffer_type) {
                                    LOGM_INFO(" [Muxer] FlowControl EOS buffer comes.\n");
                                    mpBuffer->Release();
                                    mpBuffer = NULL;
                                    if (allInputEos()) {
                                        finalizeFile();
                                        postMsg(EMSGType_RecordingEOS);
                                        msState = EModuleState_Completed;
                                        break;
                                    }
                                } else {
                                    mpBuffer->Release();
                                    mpBuffer = NULL;
                                }
                            }
                            break;
                        } else {
                            DASSERT(0);
                            if (mpBuffer) {
                                mpBuffer->Release();
                                mpBuffer = NULL;
                            }
                            break;
                        }
                    }

                    //state not changed
                    if (EModuleState_Muxer_FlushExpiredFrame == msState) {
                        LOGM_WARN("Flush all data in input pin, no valid IDR... cur time %llu, start time %llu.\n", mpClockReference->GetCurrentTime(), cur_time);
                        msState = EModuleState_Idle;//clear all video buffers now
                    }
                }
                break;

                //if direct from EModuleState_Idle
                //use robust but not very strict PTS strategy, target: cost least time, not BLOCK some real time task, like streamming out, capture data from driver, etc
                //report PTS gap, if gap too large, post warnning msg
                //need stop recording till IDR comes, audio's need try its best to match video's PTS

                //if from previous STATE_SAVING_PARTIAL_FILE_XXXX, av sync related pin should with more precise PTS boundary, but would be with more latency
            case EModuleState_Muxer_SavingPartialFile:
                DASSERT(new_time_threshold >= mNextFileTimeThreshold);

                //write remainning packet
                LOGM_INFO("[muxer %p] start saving patial file, peek all packet if PTS less than threshold.\n", this);
                for (inpin_index = 0; inpin_index < mnInputPinsNumber; inpin_index++) {
                    if (mpMasterInput == mpInputPins[inpin_index] || mpInputPins[inpin_index]->mbAutoBoundaryReached) {
                        continue;
                    }

                    tmp_buffer = NULL;
                    while (1 == mpInputPins[inpin_index]->PeekBuffer(tmp_buffer)) {
                        DASSERT(tmp_buffer);
                        LOGM_INFO("[muxer %p, inpin %d] peek packet, PTS %llu, PTS threshold %llu.\n", this, inpin_index, tmp_buffer->GetBufferLinearPTS(), new_time_threshold);

                        writeData(tmp_buffer, mpInputPins[inpin_index]);

                        if (tmp_buffer->GetBufferLinearPTS() > new_time_threshold) {
                            tmp_buffer->Release();
                            tmp_buffer = NULL;
                            break;
                        }
                        tmp_buffer->Release();
                        tmp_buffer = NULL;
                    }

                }

                LOGM_INFO("[muxer %p] finalize current file, %s.\n", this, mpOutputFileName);

                err = finalizeFile();

                if (err != EECode_OK) {
                    LOGM_FATAL("CMuxer::finalizeFile fail, enter error state.\n");
                    msState = EModuleState_Error;
                    break;
                }

                if (mMaxTotalFileNumber) {
                    if (mTotalFileNumber >= mMaxTotalFileNumber) {
                        deletePartialFile(mFirstFileIndexCanbeDeleted);
                        mFirstFileIndexCanbeDeleted ++;
                        mTotalFileNumber --;
                    }
                } else {
                    //do nothing
                }

                //corner case, little chance to comes here
                if (allInputEos()) {
                    LOGM_WARN(" allInputEos() here, should not comes here, please check!!!!.\n");
                    //clear all cached buffer
                    for (inpin_index = 0; inpin_index < mnInputPinsNumber; inpin_index++) {
                        if (mpInputPins[inpin_index]) {
                            if (mpInputPins[inpin_index]->mpCachedBuffer) {
                                mpInputPins[inpin_index]->mpCachedBuffer->Release();
                                mpInputPins[inpin_index]->mpCachedBuffer = NULL;
                            }
                        }
                    }

                    LOGM_INFO("EOS buffer comes in state %d, enter pending.\n", msState);
                    postMsg(EMSGType_RecordingEOS);
                    msState = EModuleState_Completed;
                    break;
                }
                if (mTotalRecFileNumber > 0 && mTotalFileNumber >= mTotalRecFileNumber) {
                    LOGM_INFO("the num of rec file(%d) has reached the limit(%d), so stop recording\n", mTotalFileNumber, mTotalRecFileNumber);
                    postMsg(EMSGType_RecordingReachPresetTotalFileNumbers);
                    msState = EModuleState_Completed;
                    break;
                }

                mNextFileTimeThreshold += mAutoSavingTimeDuration;
                if (mNextFileTimeThreshold < new_time_threshold) {
                    LOGM_WARN("mNextFileTimeThreshold(%llu) + mAutoSavingTimeDuration(%llu) still less than new_time_threshold(%llu).\n", mNextFileTimeThreshold, mAutoSavingTimeDuration, new_time_threshold);
                    mNextFileTimeThreshold += mAutoSavingTimeDuration;
                }
                //DASSERT(mIDRFrameInterval);
                if (mIDRFrameInterval) {
                    threshold_diff = mNextFileTimeThreshold % mIDRFrameInterval;
                } else {
                    threshold_diff = 0;
                }
                if (threshold_diff == 0) {
                    mAudioNextFileTimeThreshold = mNextFileTimeThreshold - mVideoFirstPTS;
                } else {
                    mAudioNextFileTimeThreshold = mNextFileTimeThreshold - mVideoFirstPTS - threshold_diff + mIDRFrameInterval;
                }

                ++mCurrentFileIndex;
                updateFileName(mCurrentFileIndex);
                LOGM_INFO("[muxer %p] initialize new file %s.\n", this, mpOutputFileName);

                //generate thumbnail file here
                if (mpPersistMediaConfig->encoding_mode_config.thumbnail_enabled && mpThumbNailFileName) {
                    postMsg(EMSGType_NotifyThumbnailFileGenerated);
                }

                err = initializeFile(mpOutputFileName, mRequestContainerType);
                DASSERT(err == EECode_OK);
                if (err != EECode_OK) {
                    LOGM_FATAL("CMuxer::initializeFile fail, enter error state.\n");
                    msState = EModuleState_Error;
                    break;
                }
                LOGM_INFO("[muxer %p] end saving patial file.\n", this);
                msState = EModuleState_Idle;
                break;

                //with more precise pts, but with much latency for write new file/streaming
            case EModuleState_Muxer_SavingPartialFilePeekRemainingOnMasterPin:
                //wait master pin's gap(video IDR)
                DASSERT(new_time_threshold >= mNextFileTimeThreshold);
                DASSERT(pPin != mpMasterInput);
                DASSERT(0 == mpMasterInput->mbAutoBoundaryReached);

                LOGM_INFO("[avsync]: (%p) PEEK buffers in master pin start.\n", this);

                if (pPin != mpMasterInput) {
                    //get master's time threshold
                    //peek master input(video) first, refresh the threshold, the duration maybe not very correct because h264's IDR time interval maybe large (related to M/N/IDR interval settings)
                    DASSERT(StreamType_Video != pPin->mType);
                    DASSERT(StreamType_Video == mpMasterInput->mType);
                    while (1 == mpMasterInput->PeekBuffer(tmp_buffer)) {
                        DASSERT(tmp_buffer);
                        if (_isMuxerInputData(tmp_buffer->GetBufferType())) {
                            LOGM_INFO("[master inpin] peek packet, PTS %llu, PTS threshold %llu.\n", tmp_buffer->GetBufferLinearPTS(), mNextFileTimeThreshold);
                            if ((tmp_buffer->GetBufferLinearPTS() >  mNextFileTimeThreshold) && (EPredefinedPictureType_IDR == tmp_buffer->mVideoFrameType)) {
                                DASSERT(!mpMasterInput->mpCachedBuffer);
                                if (mpMasterInput->mpCachedBuffer) {
                                    LOGM_WARN("why comes here, release cached buffer, pts %llu, type %d.\n", mpMasterInput->mpCachedBuffer->GetBufferLinearPTS(), mpMasterInput->mpCachedBuffer->mVideoFrameType);
                                    mpMasterInput->mpCachedBuffer->Release();
                                }
                                mpMasterInput->mpCachedBuffer = tmp_buffer;
                                if (!_isTmeStampNear(new_time_threshold, tmp_buffer->GetBufferLinearPTS(), TimeUnitNum_fps29dot97)) {
                                    //refresh pPin's threshold
                                    if ((StreamType)pPin->mType != StreamType_Audio) {
                                        pPin->mbAutoBoundaryReached = 0;
                                    }
                                }
                                new_time_threshold = tmp_buffer->GetBufferLinearPTS();
                                LOGM_INFO("get IDR boundary, pts %llu, mNextFileTimeThreshold %llu, diff %lld.\n", new_time_threshold, mNextFileTimeThreshold, new_time_threshold - mNextFileTimeThreshold);
                                mpMasterInput->mbAutoBoundaryReached = 1;
                                tmp_buffer = NULL;
                                break;
                            }
                            writeData(tmp_buffer, mpMasterInput);
                        } else {
                            if (EBufferType_FlowControl_EOS == tmp_buffer->GetBufferType()) {
                                mpMasterInput->mEOSComes = 1;
                                tmp_buffer->Release();
                                tmp_buffer = NULL;
                                break;
                            } else {
                                //ignore
                            }
                        }
                        tmp_buffer->Release();
                        tmp_buffer = NULL;
                    }

                    if (1 == mpMasterInput->mbAutoBoundaryReached) {
                        LOGM_INFO("[avsync]: PEEK buffers in master pin done, go to non-master pin.\n");
                        msState = EModuleState_Muxer_SavingPartialFilePeekRemainingOnNonMasterPin;
                    } else {
                        LOGM_INFO("[avsync]: PEEK buffers in master pin not finished, go to wait master boundary(IDR).\n");
                        msState = EModuleState_Muxer_SavingPartialFileWaitMasterPin;
                    }

                } else {
                    LOGM_ERROR("MUST not comes here, please check logic.\n");
                    msState = EModuleState_Muxer_SavingPartialFile;
                }
                break;

            case EModuleState_Muxer_SavingPartialFileWaitMasterPin:
                LOGM_INFO("[avsync]: (%p) WAIT next IDR boundary start.\n", this);
                DASSERT(0 == mpMasterInput->mbAutoBoundaryReached);
                DASSERT(!mpMasterInput->mpCachedBuffer);
                if (mpMasterInput->mpCachedBuffer) {
                    LOGM_WARN("Already have get next IDR, goto next stage.\n");
                    mpMasterInput->mbAutoBoundaryReached = 1;
                    msState = EModuleState_Muxer_SavingPartialFilePeekRemainingOnNonMasterPin;
                    break;
                }

                if (0 == mpMasterInput->mbAutoBoundaryReached) {
                    type = mpWorkQ->WaitDataMsgWithSpecifiedQueue(&cmd, sizeof(cmd), mpMasterInput->mpBufferQ);
                    if (type == CIQueue::Q_MSG) {
                        processCmd(cmd);
                    } else {
                        if (1 == mpMasterInput->PeekBuffer(tmp_buffer)) {
                            if (_isMuxerInputData(tmp_buffer->GetBufferType())) {
                                if (EPredefinedPictureType_IDR == tmp_buffer->mVideoFrameType && tmp_buffer->GetBufferLinearPTS() >= new_time_threshold) {
                                    mpMasterInput->mpCachedBuffer = tmp_buffer;
                                    if (!_isTmeStampNear(new_time_threshold, tmp_buffer->GetBufferLinearPTS(), TimeUnitNum_fps29dot97)) {
                                        //refresh new time threshold, need recalculate non-masterpin
                                        if ((StreamType)pPin->mType != StreamType_Audio) {
                                            pPin->mbAutoBoundaryReached = 0;
                                        }
                                    }
                                    LOGM_INFO("[avsync]: wait new IDR, pts %llu, previous time threshold %llu, diff %lld.\n", tmp_buffer->GetBufferLinearPTS(), new_time_threshold, tmp_buffer->GetBufferLinearPTS() - new_time_threshold);
                                    new_time_threshold = tmp_buffer->GetBufferLinearPTS();
                                    mpMasterInput->mbAutoBoundaryReached = 1;
                                    msState = EModuleState_Muxer_SavingPartialFilePeekRemainingOnNonMasterPin;

                                    tmp_buffer = NULL;
                                    break;
                                }
                                writeData(tmp_buffer, mpMasterInput);
                                tmp_buffer->Release();
                                tmp_buffer = NULL;
                            } else if (EBufferType_FlowControl_EOS == tmp_buffer->GetBufferType()) {
                                LOGM_WARN("EOS comes here, how to handle it, msState %d.\n", msState);
                                mpMasterInput->mEOSComes = 1;
                                tmp_buffer->Release();
                                tmp_buffer = NULL;
                                mpMasterInput->mbAutoBoundaryReached = 1;
                                msState = EModuleState_Muxer_SavingPartialFilePeekRemainingOnNonMasterPin;
                            } else {
                                //ignore
                                LOGM_ERROR("not processed buffer type %d.\n", tmp_buffer->GetBufferType());
                                tmp_buffer->Release();
                                tmp_buffer = NULL;
                            }
                        } else {
                            LOGM_ERROR("MUST not comes here, please check WaitDataMsgWithSpecifiedQueue()\n");
                            msState = EModuleState_Muxer_SavingPartialFilePeekRemainingOnNonMasterPin;
                            mpMasterInput->mbAutoBoundaryReached = 1;
                        }
                    }
                }
                break;

            case EModuleState_Muxer_SavingPartialFilePeekRemainingOnNonMasterPin:
                //for non-master pin
                DASSERT(new_time_threshold >= mNextFileTimeThreshold);

                LOGM_INFO("[avsync]: PEEK non-master pin(%p) start.\n", this);
                if (!hasPinNeedReachBoundary(inpin_index)) {
                    msState = EModuleState_Muxer_SavingPartialFile;
                    LOGM_INFO("[avsync]: all pin(need sync) reaches boundary.\n");
                    break;
                }
                if (inpin_index >= mnInputPinsNumber || !mpInputPins[inpin_index]) {
                    LOGM_ERROR("safe check fail, check code, inpin_index %d, mnInputPinsNumber %d.\n", inpin_index, mnInputPinsNumber);
                    msState = EModuleState_Muxer_SavingPartialFile;
                    break;
                }

                pPin = mpInputPins[inpin_index];
                DASSERT(pPin);
                DASSERT(pPin != mpMasterInput);
                DASSERT(StreamType_Video != pPin->mType);
                DASSERT(!pPin->mpCachedBuffer);
                if (pPin->mpCachedBuffer) {
                    LOGM_ERROR("why comes here, check code.\n");
                    pPin->mpCachedBuffer->Release();
                    pPin->mpCachedBuffer = NULL;
                }

                if (pPin != mpMasterInput) {
                    //peek non-master input
                    DASSERT(StreamType_Video != pPin->mType);
                    while (1 == pPin->PeekBuffer(tmp_buffer)) {
                        DASSERT(tmp_buffer);
                        if (_isMuxerInputData(tmp_buffer->GetBufferType())) {
                            LOGM_INFO("[non-master inpin] (%p) peek packet, PTS %llu, PTS threshold %llu.\n", this, tmp_buffer->GetBufferLinearPTS(), mNextFileTimeThreshold);

                            writeData(tmp_buffer, pPin);

                            if (StreamType_Audio != pPin->mType) {
                                if (tmp_buffer->GetBufferLinearPTS() >=  mNextFileTimeThreshold) {
                                    LOGM_INFO("[avsync]: non-master pin (%p) get boundary, pts %llu, mNextFileTimeThreshold %llu, diff %lld.\n", this, new_time_threshold, mNextFileTimeThreshold, new_time_threshold - mNextFileTimeThreshold);
                                    pPin->mbAutoBoundaryReached = 1;
                                    tmp_buffer->Release();
                                    tmp_buffer = NULL;
                                    break;
                                }
                            } else {//audio stream
                                if (tmp_buffer->GetBufferLinearPTS() >=  mAudioNextFileTimeThreshold) {
                                    LOGM_INFO("[avsync]: audio input pin (%p) get boundary, pts %llu, mAudioNextFileTimeThreshold %llu, diff %lld.\n", this, new_time_threshold, mAudioNextFileTimeThreshold, new_time_threshold - mAudioNextFileTimeThreshold);
                                    pPin->mbAutoBoundaryReached = 1;
                                    tmp_buffer->Release();
                                    tmp_buffer = NULL;
                                    break;
                                }
                            }
                            tmp_buffer->Release();
                            tmp_buffer = NULL;
                        } else {
                            if (EBufferType_FlowControl_EOS == tmp_buffer->GetBufferType()) {
                                pPin->mEOSComes = 1;
                                tmp_buffer->Release();
                                tmp_buffer = NULL;
                                break;
                            } else {
                                //ignore
                            }
                            tmp_buffer->Release();
                            tmp_buffer = NULL;
                        }
                    }

                    if (0 == pPin->mbAutoBoundaryReached) {
                        LOGM_INFO("[avsync]: non-master pin need wait.\n");
                        msState = EModuleState_Muxer_SavingPartialFileWaitNonMasterPin;
                    }

                } else {
                    LOGM_ERROR("MUST not comes here, please check logic.\n");
                    msState = EModuleState_Muxer_SavingPartialFile;
                }
                break;

            case EModuleState_Muxer_SavingPartialFileWaitNonMasterPin:
                LOGM_INFO("[avsync]: (%p)WAIT till boundary(non-master pin).\n", this);

                //debug assert
                DASSERT(pPin);
                DASSERT(pPin != mpMasterInput);
                DASSERT(StreamType_Video != pPin->mType);
                DASSERT(!pPin->mpCachedBuffer);

                DASSERT(0 == pPin->mbAutoBoundaryReached);
                DASSERT(!pPin->mpCachedBuffer);
                if (pPin->mpCachedBuffer) {
                    LOGM_WARN("[avsync]: Already have get cached buffer, goto next stage.\n");
                    pPin->mbAutoBoundaryReached = 1;
                    msState = EModuleState_Muxer_SavingPartialFilePeekRemainingOnNonMasterPin;
                    break;
                }

                if (0 == pPin->mbAutoBoundaryReached) {
                    type = mpWorkQ->WaitDataMsgWithSpecifiedQueue(&cmd, sizeof(cmd), pPin->mpBufferQ);
                    if (type == CIQueue::Q_MSG) {
                        processCmd(cmd);
                    } else {
                        if (1 == pPin->PeekBuffer(tmp_buffer)) {
                            if (_isMuxerInputData(tmp_buffer->GetBufferType())) {
                                writeData(tmp_buffer, pPin);

                                if (tmp_buffer->GetBufferLinearPTS() >= new_time_threshold) {
                                    LOGM_INFO("[avsync]: non-master pin(%p) wait new boundary, pts %llu, time threshold %llu, diff %lld.\n", this, tmp_buffer->GetBufferLinearPTS(), new_time_threshold, tmp_buffer->GetBufferLinearPTS() - new_time_threshold);
                                    pPin->mbAutoBoundaryReached = 1;
                                    msState = EModuleState_Muxer_SavingPartialFilePeekRemainingOnNonMasterPin;
                                    tmp_buffer->Release();
                                    tmp_buffer = NULL;
                                    break;
                                }
                                tmp_buffer->Release();
                                tmp_buffer = NULL;
                            } else if (_isFlowControlBuffer(tmp_buffer->GetBufferType())) {
                                LOGM_WARN("EOS comes here, how to handle it, msState %d.\n", msState);
                                pPin->mEOSComes = 1;
                                tmp_buffer->Release();
                                tmp_buffer = NULL;
                                pPin->mbAutoBoundaryReached = 1;
                                if (allInputEos()) {
                                    finalizeFile();
                                    postMsg(EMSGType_RecordingEOS);
                                    msState = EModuleState_Completed;
                                } else {
                                    msState = EModuleState_Muxer_SavingPartialFilePeekRemainingOnNonMasterPin;
                                }
                                break;
                            } else {
                                //ignore
                                LOGM_ERROR("not processed buffer type %d.\n", tmp_buffer->GetBufferType());
                                tmp_buffer->Release();
                                tmp_buffer = NULL;
                            }
                        } else {
                            LOGM_ERROR("MUST not comes here, please check WaitDataMsgWithSpecifiedQueue()\n");
                            msState = EModuleState_Muxer_SavingPartialFilePeekRemainingOnNonMasterPin;
                            pPin->mbAutoBoundaryReached = 1;
                        }
                    }
                }
                break;

            case EModuleState_Muxer_SavingWholeFile:
                //write data
                DASSERT(mpBuffer);
                DASSERT(pPin);
                DASSERT(pPin->mType == StreamType_Video);

                LOGM_INFO("[muxer %p] EModuleState_Muxer_SavingWholeFile, write remainning packet start.\n", this);
                if (pPin->mType == StreamType_Video) {
                    if (pPin->mpCachedBuffer) {
                        writeData(pPin->mpCachedBuffer, pPin);
                        pPin->mpCachedBuffer->Release();
                        pPin->mpCachedBuffer = NULL;
                    }
                    writeData(mpBuffer, pPin);
                } else if (pPin->mType == StreamType_Audio) {
                    if (pPin->mpCachedBuffer) {
                        DASSERT(0);//current code should not come to this case
                        LOGM_FATAL("current code should not come to this case.\n");
                        writeData(pPin->mpCachedBuffer, pPin);
                        pPin->mpCachedBuffer->Release();
                        pPin->mpCachedBuffer = NULL;
                    }
                    writeData(mpBuffer, pPin);
                } else if (pPin->mType == StreamType_Subtitle) {

                } else if (pPin->mType == StreamType_PrivateData) {
                    writeData(mpBuffer, pPin);
                } else {
                    DASSERT(0);
                    LOGM_FATAL("must not comes here, BAD input type(%d)? pin %p, buffer %p.\n", pPin->mType, pPin, mpBuffer);
                }
                pPin = NULL;

                mpBuffer->Release();
                mpBuffer = NULL;
                msState = EModuleState_Completed;

                //write remainning packet, todo

                //saving file
                LOGM_INFO("[muxer %p] EModuleState_Muxer_SavingWholeFile, write remainning packet end, finalizeFile() start.\n", this);
                finalizeFile();
                LOGM_INFO("[muxer %p] EModuleState_Muxer_SavingWholeFile, write remainning packet end, finalizeFile() done.\n", this);

                LOGM_INFO("Post MSG_INTERNAL_ENDING to engine.\n");
                if (mPresetReachType | (EPresetCheckFlag_videoframecount | EPresetCheckFlag_audioframecount)) {
                    postMsg(EMSGType_RecordingReachPresetDuration);
                } else if (mPresetReachType == EPresetCheckFlag_filesize) {
                    postMsg(EMSGType_RecordingReachPresetFilesize);
                } else {
                    DASSERT(0);
                };
                LOGM_INFO("[muxer %p] EModuleState_Muxer_SavingWholeFile, MSG_INTERNAL_ENDING done.\n", this);

                break;

            case EModuleState_Pending:
            case EModuleState_Completed:
            case EModuleState_Bypass:
            case EModuleState_Error:
                //discard all data, wait eos
                type = mpWorkQ->WaitDataMsg(&cmd, sizeof(cmd), &result);
                if (type == CIQueue::Q_MSG) {
                    processCmd(cmd);
                } else {
                    pPin = (CMuxerInput *)result.pOwner;
                    DASSERT(pPin);
                    if (readInputData(pPin, buffer_type)) {
                        if (mpBuffer) {
                            DASSERT(mpBuffer);
                            mpBuffer->Release();
                            mpBuffer = NULL;
                        }
                        if (EBufferType_FlowControl_EOS == buffer_type) {
                            //check eos
                            LOGM_INFO("EOS buffer comes.\n");
                            if (allInputEos()) {
                                if (msState != EModuleState_Error) {
                                    finalizeFile();
                                }
                                postMsg(EMSGType_RecordingEOS);
                            }
                        } else {
                            //LOGM_ERROR("BAD buffer type, %d.\n", buffer_type);
                        }
                    } else {
                        //peek buffer fail. must not comes here
                        LOGM_ERROR("peek buffer fail. must not comes here.\n");
                    }
                }
                break;

            default:
                LOGM_ERROR("CMuxer::OnRun, error state 0x%x\n", (TUint)msState);
                msState = EModuleState_Error;
                break;
        }

        mDebugHeartBeat ++;

    }

    //for safe here
    //finalizeFile();//131108. roy temp mdf, not thread safe with CMuxer::Finalize()

    LOGM_INFO("OnRun loop end.\n");
}

EECode CMuxer::flushInputData()
{
    TUint index = 0;

    if (mpBuffer) {
        mpBuffer->Release();
        mpBuffer = NULL;
    }

    LOGM_INFO("before purge input pins\n");

    for (index = 0; index < EConstMaxDemuxerMuxerStreamNumber; index ++) {
        if (mpInputPins[index]) {
            mpInputPins[index]->Purge();
        }
    }

    LOGM_INFO("after purge input pins\n");

    return EECode_OK;
}

//-----------------------------------------------------------------------
//
// CScheduledMuxer
//
//-----------------------------------------------------------------------
IFilter *CScheduledMuxer::Create(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TUint index)
{
    CScheduledMuxer *result = new CScheduledMuxer(pName, pPersistMediaConfig, pEngineMsgSink, index);
    if (result && result->Construct() != EECode_OK) {
        delete result;
        result = NULL;
    }
    return result;
}

CScheduledMuxer::CScheduledMuxer(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TUint index)
    : inherited(pName, index, pPersistMediaConfig, pEngineMsgSink)
    , mpMuxer(NULL)
    , mpCurrentInputPin(NULL)
    , mnInputPinsNumber(0)
    , mpScheduler(NULL)
    , mRequestContainerType(ContainerType_TS)
    , mRequestStartDTS(0)
    , mRequestStartPTS(12012)
    , mCurMuxerID(EMuxerModuleID_Invalid)
    , mpBuffer(NULL)
    , mbMuxerContentSetup(0)
    , mbMuxerPaused(0)
    , mbMuxerStarted(0)
    , mbMuxerVideoEnabled(1)
    , mbMuxerAudioEnabled(1)
    , mbMuxerPridataEnabled(0)
    , mbRecievedEosSignal(0)
    , mbEos(0)
    , mpOutputFileName(NULL)
    , mpThumbNailFileName(NULL)
    , mOutputFileNameLength(0)
    , mThumbNailFileNameLength(0)
    , mnSession(0)
    , mStreamStartPTS(0)
    , mFileNameHanding1(0)
    , mFileNameHanding2(0)
    , mbMasterStarted(0)
    , mbExtraDataRecieved(0)
    , mpMasterInput(NULL)
    , mCurrentFileIndex(0)
    , mFirstFileIndexCanbeDeleted(0)
    , mTotalFileNumber(0)
    , mMaxTotalFileNumber(0)
    , mTotalRecFileNumber(0)
    , mpOutputFileNameBase(NULL)
    , mpThumbNailFileNameBase(NULL)
    , mOutputFileNameBaseLength(0)
    , mThumbNailFileNameBaseLength(0)
    , mSavingFileStrategy(MuxerSavingFileStrategy_AutoSeparateFile)
    , mSavingCondition(MuxerSavingCondition_InputPTS)
    , mAutoFileNaming(MuxerAutoFileNaming_ByNumber)
    , mAutoSaveFrameCount(180 * 30)
    , mAutoSavingTimeDuration(180 * TimeUnitDen_90khz)
    , mNextFileTimeThreshold(0)
    , mAudioNextFileTimeThreshold(0)
    , mIDRFrameInterval(0)
    , mVideoFirstPTS(0)
    , mbNextFileTimeThresholdSet(0)
    , mbPTSStartFromZero(1)
    , mPresetVideoMaxFrameCnt(0)
    , mPresetAudioMaxFrameCnt(0)
    , mPresetMaxFilesize(0)
    , mCurrentTotalFilesize(0)
    , mPresetCheckFlag(0)
    , mPresetReachType(0)
    , mPriority(0)
    , mpStorageManager(NULL)
    , mpChannelName(NULL)
{
    TUint i = 0;
    for (i = 0; i < EConstMaxDemuxerMuxerStreamNumber; i ++) {
        mpInputPins[i] = NULL;
    }

    for (i = 0; i < EConstMaxDemuxerMuxerStreamNumber; i ++) {
        mpExtraData[i] = NULL;
        mExtraDataSize[i] = 0;
    }

    msState = EModuleState_WaitCmd;

    memset(&mStreamCodecInfos, 0x0, sizeof(mStreamCodecInfos));
    memset(&mMuxerFileInfo, 0x0, sizeof(mMuxerFileInfo));

    mpStorageManager = pPersistMediaConfig->p_storage_manager;
}

EECode CScheduledMuxer::Construct()
{
    DSET_MODULE_LOG_CONFIG(ELogModuleMuxerFilter);

    mpScheduler = gfGetFileIOWriterScheduler((const volatile SPersistMediaConfig *)mpPersistMediaConfig, mIndex);
    DASSERT(mpScheduler);

    return inherited::Construct();
}

CScheduledMuxer::~CScheduledMuxer()
{
    TUint i = 0;

    LOGM_RESOURCE("CScheduledMuxer::~CScheduledMuxer(), before delete inputpins.\n");
    for (i = 0; i < EConstMaxDemuxerMuxerStreamNumber; i ++) {
        if (mpInputPins[i]) {
            mpInputPins[i]->Delete();
            mpInputPins[i] = NULL;
        }
    }

    LOGM_RESOURCE("CScheduledMuxer::~CScheduledMuxer(), before mpMuxer->Delete().\n");
    if (mpMuxer) {
        mpMuxer->Delete();
        mpMuxer = NULL;
    }

    for (i = 0; i < EConstMaxDemuxerMuxerStreamNumber; i ++) {
        if (mpExtraData[i]) {
            free(mpExtraData[i]);
            mpExtraData[i] = NULL;
        }
        mExtraDataSize[i] = 0;
    }

    if (mpOutputFileName) {
        if (mpStorageManager) {
            LOGM_NOTICE("before quit release %s\n", mpOutputFileName);
            mpStorageManager->FinalizeNewUint(mpChannelName, mpOutputFileName);
            mpStorageManager = NULL;
        } else {
            free(mpOutputFileName);
        }
        mpOutputFileName = NULL;
    }

    if (mpOutputFileNameBase) {
        free(mpOutputFileNameBase);
        mpOutputFileNameBase = NULL;
    }

    if (mpThumbNailFileName) {
        free(mpThumbNailFileName);
        mpThumbNailFileName = NULL;
    }

    if (mpThumbNailFileNameBase) {
        free(mpThumbNailFileNameBase);
        mpThumbNailFileNameBase = NULL;
    }

    DASSERT(!mpMuxer);
    DASSERT(!mbMuxerStarted);
    if (mpMuxer && mbMuxerStarted) {
        LOGM_WARN("finalize file here\n");
        finalizeFile();
    }

    if (mpChannelName) {
        free(mpChannelName);
        mpChannelName = NULL;
    }

    LOGM_RESOURCE("CScheduledMuxer::~CScheduledMuxer(), end.\n");
}

void CScheduledMuxer::Delete()
{
    TUint i = 0;

    LOGM_RESOURCE("CScheduledMuxer::Delete(), before delete inputpins.\n");
    for (i = 0; i < EConstMaxDemuxerMuxerStreamNumber; i ++) {
        if (mpInputPins[i]) {
            mpInputPins[i]->Delete();
            mpInputPins[i] = NULL;
        }
    }

    LOGM_RESOURCE("CScheduledMuxer::Delete(), before mpMuxer->Delete().\n");
    if (mpMuxer) {
        mpMuxer->Delete();
        mpMuxer = NULL;
    }

    for (i = 0; i < EConstMaxDemuxerMuxerStreamNumber; i ++) {
        if (mpExtraData[i]) {
            free(mpExtraData[i]);
            mpExtraData[i] = NULL;
        }
        mExtraDataSize[i] = 0;
    }

    if (mpOutputFileName) {
        if (mpStorageManager) {
            LOGM_NOTICE("before quit release %s\n", mpOutputFileName);
            mpStorageManager->FinalizeNewUint(mpChannelName, mpOutputFileName);
            mpStorageManager = NULL;
        } else {
            free(mpOutputFileName);
        }
        mpOutputFileName = NULL;
    }

    if (mpOutputFileNameBase) {
        free(mpOutputFileNameBase);
        mpOutputFileNameBase = NULL;
    }

    if (mpThumbNailFileName) {
        free(mpThumbNailFileName);
        mpThumbNailFileName = NULL;
    }

    if (mpThumbNailFileNameBase) {
        free(mpThumbNailFileNameBase);
        mpThumbNailFileNameBase = NULL;
    }

    DASSERT(!mpMuxer);
    DASSERT(!mbMuxerStarted);
    if (mpMuxer && mbMuxerStarted) {
        LOGM_WARN("finalize file here\n");
        finalizeFile();
    }

    if (mpChannelName) {
        free(mpChannelName);
        mpChannelName = NULL;
    }

    LOGM_RESOURCE("CScheduledMuxer::Delete(), end.\n");

    inherited::Delete();
}

EECode CScheduledMuxer::Initialize(TChar *p_param)
{
    EMuxerModuleID id;
    id = _guess_muxer_type_from_string(p_param);

    LOGM_INFO("EMuxerModuleID %d\n", id);

    if (mbMuxerContentSetup) {

        DASSERT(mpMuxer);
        if (!mpMuxer) {
            LOGM_FATAL("[Internal bug], why the mpMuxer is NULL here?\n");
            return EECode_InternalLogicalBug;
        }

        LOGM_WARN("There's a mpMuxer %p already, mbMuxerStarted %d, delete it first\n", mpMuxer, mbMuxerStarted);

        if (mbMuxerStarted) {
            LOGM_INFO("before mpMuxer->FinalizeFile()\n");
            getFileInformation();
            mpMuxer->FinalizeFile(&mMuxerFileInfo);
            mbMuxerStarted = 0;
        }

        LOGM_INFO("before mpMuxer->DestroyContext()\n");
        if (mbMuxerContentSetup) {
            mpMuxer->DestroyContext();
            mbMuxerContentSetup = 0;
        }

        if (id != mCurMuxerID) {
            LOGM_INFO("before mpMuxer->Delete(), cur id %d, request id %d\n", mCurMuxerID, id);
            mpMuxer->Delete();
            mpMuxer = NULL;
        }
    }

    if (!mpMuxer) {
        LOGM_INFO("before gfModuleFactoryMuxer(%d)\n", id);
        mpMuxer = gfModuleFactoryMuxer(id, mpPersistMediaConfig, mpEngineMsgSink);
        if (mpMuxer) {
            mCurMuxerID = id;
        } else {
            mCurMuxerID = EMuxerModuleID_Auto;
            LOGM_FATAL("[Internal bug], request gfModuleFactoryMuxer(%d) fail\n", id);
            return EECode_InternalLogicalBug;
        }
    }

    LOGM_INFO("[Muxer flow], before mpMuxer->SetupContext()\n");
    mpMuxer->SetupContext();
    mbMuxerContentSetup = 1;

    LOGM_INFO("[Muxer flow], CScheduledMuxer::Initialize() done\n");

    return EECode_OK;
}

EECode CScheduledMuxer::Finalize()
{
    if (mpMuxer) {
        getFileInformation();
        if (mbMuxerStarted) {
            LOGM_INFO("before mpMuxer->FinalizeFile()\n");
            mpMuxer->FinalizeFile(&mMuxerFileInfo);
            mbMuxerStarted = 0;
        }

        LOGM_INFO("before mpMuxer->DestroyContext()\n");
        DASSERT(mbMuxerContentSetup);
        if (mbMuxerContentSetup) {
            mpMuxer->DestroyContext();
            mbMuxerContentSetup = 0;
        }

        if (mpStorageManager && mpOutputFileName) {
            mpStorageManager->FinalizeNewUint(mpChannelName, mpOutputFileName);
            mpStorageManager = NULL;
            mpOutputFileName = NULL;
        }

        LOGM_INFO("before mpMuxer->Delete(), cur id %d\n", mCurMuxerID);
        mpMuxer->Delete();
        mpMuxer = NULL;
    }

    return EECode_OK;
}

EECode CScheduledMuxer::initialize(TChar *p_param)
{
    EMuxerModuleID id;
    id = _guess_muxer_type_from_string(p_param);

    LOGM_INFO("EMuxerModuleID %d\n", id);

    if (mbMuxerContentSetup) {

        DASSERT(mpMuxer);
        if (!mpMuxer) {
            LOGM_FATAL("[Internal bug], why the mpMuxer is NULL here?\n");
            return EECode_InternalLogicalBug;
        }

        LOGM_WARN("There's a mpMuxer %p already, mbMuxerStarted %d, delete it first\n", mpMuxer, mbMuxerStarted);

        if (mbMuxerStarted) {
            LOGM_INFO("before mpMuxer->FinalizeFile()\n");
            getFileInformation();
            mpMuxer->FinalizeFile(&mMuxerFileInfo);
            mbMuxerStarted = 0;
        }

        LOGM_INFO("before mpMuxer->DestroyContext()\n");
        if (mbMuxerContentSetup) {
            mpMuxer->DestroyContext();
            mbMuxerContentSetup = 0;
        }

        if (id != mCurMuxerID) {
            LOGM_INFO("before mpMuxer->Delete(), cur id %d, request id %d\n", mCurMuxerID, id);
            mpMuxer->Delete();
            mpMuxer = NULL;
        }
    }

    if (!mpMuxer) {
        LOGM_INFO("before gfModuleFactoryMuxer(%d)\n", id);
        mpMuxer = gfModuleFactoryMuxer(id, mpPersistMediaConfig, mpEngineMsgSink);
        if (mpMuxer) {
            mCurMuxerID = id;
        } else {
            mCurMuxerID = EMuxerModuleID_Auto;
            LOGM_FATAL("[Internal bug], request gfModuleFactoryMuxer(%d) fail\n", id);
            return EECode_InternalLogicalBug;
        }
    }

    LOGM_INFO("[Muxer flow], before mpMuxer->SetupContext()\n");
    mpMuxer->SetupContext();
    mbMuxerContentSetup = 1;

    LOGM_INFO("[Muxer flow], CMuxer::initialize() done\n");

    return EECode_OK;
}

void CScheduledMuxer::finalize()
{
    if (mbMuxerContentSetup) {

        if (DUnlikely(!mpMuxer)) {
            LOGM_WARN("[Internal bug], why the mpMuxer is NULL here?\n");
            return;
        }

        if (mbMuxerStarted) {
            LOGM_INFO("before mpMuxer->FinalizeFile()\n");
            getFileInformation();
            mpMuxer->FinalizeFile(&mMuxerFileInfo);
            mbMuxerStarted = 0;
        }

        LOGM_INFO("before mpMuxer->DestroyContext()\n");
        if (mbMuxerContentSetup) {
            mpMuxer->DestroyContext();
            mbMuxerContentSetup = 0;
        }

    }

    return;
}


void CScheduledMuxer::initializeTimeInfo()
{
    TUint i = 0;
    CMuxerInput *pInput = NULL;

    for (i = 0; i < mnInputPinsNumber; i++) {
        pInput = mpInputPins[i];
        DASSERT(mStreamCodecInfos.info[i].inited);
        DASSERT(pInput);
        if (pInput && mStreamCodecInfos.info[i].inited) {
            if (StreamType_Video == mStreamCodecInfos.info[i].stream_type) {
                pInput->mStartPTS = mStreamStartPTS;
                pInput->mSessionPTSStartPoint = mStreamStartPTS;
                pInput->mCurrentPTS = mStreamStartPTS;
                pInput->mCurrentDTS = mStreamStartPTS;
                pInput->mSessionInputPTSStartPoint = 0;

                pInput->mCurrentFrameCount = 0;
                pInput->mTotalFrameCount = 0;
                pInput->mSession = 0;
                DASSERT(mStreamCodecInfos.info[i].spec.video.framerate_den);
                DASSERT(mStreamCodecInfos.info[i].spec.video.framerate_num);
                if ((mStreamCodecInfos.info[i].spec.video.framerate_den) && (mStreamCodecInfos.info[i].spec.video.framerate_num)) {
                    pInput->mDuration = mStreamCodecInfos.info[i].spec.video.framerate_den * TimeUnitDen_90khz / mStreamCodecInfos.info[i].spec.video.framerate_num;
                } else {
                    //default one
                    mStreamCodecInfos.info[i].spec.video.framerate_num = TimeUnitDen_90khz;
                    mStreamCodecInfos.info[i].spec.video.framerate_den = TimeUnitNum_fps29dot97;
                    pInput->mDuration = TimeUnitNum_fps29dot97;
                }
                pInput->mAVNormalizedDuration = pInput->mDuration;
                pInput->mPTSDTSGap = pInput->mPTSDTSGapDivideDuration * pInput->mDuration;
            } else if (StreamType_Audio == mStreamCodecInfos.info[i].stream_type) {
                pInput->mStartPTS = mStreamStartPTS;
                pInput->mSessionPTSStartPoint = mStreamStartPTS;
                pInput->mCurrentPTS = mStreamStartPTS;
                pInput->mCurrentDTS = mStreamStartPTS;
                pInput->mSessionInputPTSStartPoint = 0;

                pInput->mCurrentFrameCount = 0;
                pInput->mTotalFrameCount = 0;
                pInput->mSession = 0;

                if (mStreamCodecInfos.info[i].spec.audio.frame_size) {
                    pInput->mDuration = mStreamCodecInfos.info[i].spec.audio.frame_size;
                    pInput->mAVNormalizedDuration = mStreamCodecInfos.info[i].spec.audio.frame_size;
                } else {
                    //default here
                    pInput->mDuration = mStreamCodecInfos.info[i].spec.audio.frame_size = 1024;
                    pInput->mAVNormalizedDuration = mStreamCodecInfos.info[i].spec.audio.frame_size;
                }
                pInput->mPTSDTSGap = pInput->mPTSDTSGapDivideDuration * pInput->mDuration;
            } else {
                LOGM_FATAL("NOT support here\n");
            }
        }
    }

}

EECode CScheduledMuxer::initializeFile(TChar *p_param, ContainerType container_type)
{
    EECode err = EECode_OK;

    LOGM_INFO("start, p_param %s, container_type %d\n", p_param, container_type);

    if (!p_param) {
        LOGM_ERROR("NULL p_param\n");
        return EECode_BadParam;
    }

    if (mbMuxerContentSetup) {

        DASSERT(mpMuxer);
        if (!mpMuxer) {
            LOGM_FATAL("[Internal bug], why the mpMuxer is NULL here?\n");
            return EECode_InternalLogicalBug;
        }

        LOGM_INFO("mbMuxerStarted %d, before mpMuxer->InitializeFile(&mStreamCodecInfos, p_param, container_type);\n", mbMuxerStarted);

        DASSERT(!mbMuxerStarted);

        TU64 tmp_duration_seconds = mAutoSavingTimeDuration / TimeUnitDen_90khz;
        err = mpMuxer->SetPrivateDataDurationSeconds(&tmp_duration_seconds, 8);
        if (EECode_OK != err) {
            LOGM_ERROR("mpMuxer->SetPrivateDataDurationSeconds(void* p_data, TUint data_size), fail, return %d, %s\n", err, gfGetErrorCodeString(err));
            return err;
        }
        err = mpMuxer->SetPrivateDataChannelName(mpChannelName, mChannelNameBufferSize);
        if (EECode_OK != err) {
            LOGM_ERROR("mpMuxer->SetPrivateDataChannelName(%s, %ld), fail, return %d, %s\n", mpChannelName, mChannelNameBufferSize, err, gfGetErrorCodeString(err));
            return err;
        }
        err = mpMuxer->InitializeFile(&mStreamCodecInfos, p_param, container_type);
        if (EECode_OK != err) {
            LOGM_ERROR("mpMuxer->InitializeFile(&mStreamCodecInfos, p_param %s, container_type %d), fail, return %d, %s\n", p_param, container_type, err, gfGetErrorCodeString(err));
            return err;
        }

        initializeTimeInfo();
        mbMuxerStarted = 1;
    } else {
        LOGM_FATAL("[Internal bug], why the mbMuxerContentSetup is %d, mpMuxer %p?\n", mbMuxerContentSetup, mpMuxer);
        return EECode_InternalLogicalBug;
    }

    return EECode_OK;
}

EECode CScheduledMuxer::finalizeFile()
{
    if (mpMuxer) {
        if (mbMuxerStarted) {
            getFileInformation();
            LOGM_INFO("before mpMuxer->FinalizeFile()\n");
            mpMuxer->FinalizeFile(&mMuxerFileInfo);
            mbMuxerStarted = 0;
        } else {
            LOGM_WARN("mbMuxerStarted %d?\n", mbMuxerStarted);
            return EECode_OK;
        }
    } else {
        LOGM_FATAL("[Internal bug], NULL mpMuxer %p?\n", mpMuxer);
        return EECode_InternalLogicalBug;
    }

    return EECode_OK;
}

EECode CScheduledMuxer::Start()
{
    //debug assert
    DASSERT(mpCmdQueue);
    DASSERT(mpMuxer);
    DASSERT(mpEngineMsgSink);
    DASSERT(mpPersistMediaConfig);
    DASSERT(mpInputPins[0]);
    DASSERT(mbMuxerContentSetup);
    DASSERT(mpScheduler);

    DASSERT(!mbStarted);

    if ((!mbStarted) && mpScheduler) {
        mbStarted = 1;

        mInputIndex = 0;
        msState = EModuleState_Muxer_WaitExtraData;
        mBufferType = EBufferType_Invalid;
        mpPin = NULL;
        mPreferentialPinIndex = 0;

        mInputIndex = 0;
        mNewTimeThreshold = 0;
        mThreshDiff = 0;
        mpMasterInput = mpInputPins[0];//hard code here

        return mpScheduler->AddScheduledCilent((IScheduledClient *)this);
    }

    LOGM_FATAL("NULL mpScheduler\n");
    return EECode_BadState;
}

EECode CScheduledMuxer::Stop()
{
    //debug assert
    DASSERT(mpCmdQueue);
    DASSERT(mpMuxer);
    DASSERT(mpEngineMsgSink);
    DASSERT(mpPersistMediaConfig);
    DASSERT(mpInputPins[0]);
    DASSERT(mbMuxerContentSetup);
    DASSERT(mpScheduler);

    DASSERT(mbStarted);

    if (mbStarted && mpScheduler) {
        mbStarted = 0;
        flushInputData();
        finalizeFile();
        msState = EModuleState_Halt;
        return mpScheduler->RemoveScheduledCilent((IScheduledClient *)this);
    }

    LOGM_FATAL("NULL mpScheduler\n");
    return EECode_BadState;
}

EECode CScheduledMuxer::SwitchInput(TComponentIndex focus_input_index)
{
    LOGM_FATAL("TO DO\n");
    return EECode_NoImplement;
}

EECode CScheduledMuxer::FlowControl(EBufferType flowcontrol_type)
{
    LOGM_FATAL("TO DO\n");
    return EECode_NoImplement;
}

EECode CScheduledMuxer::AddOutputPin(TUint &index, TUint &sub_index, TUint type)
{
    LOGM_FATAL("CScheduledMuxer do not have output pin\n");
    return EECode_InternalLogicalBug;
}

EECode CScheduledMuxer::AddInputPin(TUint &index, TUint type)
{
    if (StreamType_Audio == type) {
        if (mnInputPinsNumber >= EConstMaxDemuxerMuxerStreamNumber) {
            LOGM_ERROR("Max input pin number reached.\n");
            return EECode_InternalLogicalBug;
        }

        index = mnInputPinsNumber;
        DASSERT(!mpInputPins[mnInputPinsNumber]);
        if (mpInputPins[mnInputPinsNumber]) {
            LOGM_FATAL("mpInputPins[mnInputPinsNumber] here, must have problems!!! please check it\n");
        }

        mpInputPins[mnInputPinsNumber] = CMuxerInput::Create("[Audio input pin for CScheduledMuxer]", (IFilter *) this, mpCmdQueue, (StreamType) type, mnInputPinsNumber);
        DASSERT(mpInputPins[mnInputPinsNumber]);

        mpInputPins[mnInputPinsNumber]->mType = StreamType_Audio;
        //LOGM_NOTICE("[DEBUG]: add audio pin %d\n", mnInputPinsNumber);
        mnInputPinsNumber ++;
        return EECode_OK;
    } else if (StreamType_Video == type) {
        if (mnInputPinsNumber >= EConstMaxDemuxerMuxerStreamNumber) {
            LOGM_ERROR("Max input pin number reached.\n");
            return EECode_InternalLogicalBug;
        }

        index = mnInputPinsNumber;
        DASSERT(!mpInputPins[mnInputPinsNumber]);
        if (mpInputPins[mnInputPinsNumber]) {
            LOGM_FATAL("mpInputPins[mnInputPinsNumber] here, must have problems!!! please check it\n");
        }

        mpInputPins[mnInputPinsNumber] = CMuxerInput::Create("[Video input pin for CScheduledMuxer]", (IFilter *) this, mpCmdQueue, (StreamType) type, mnInputPinsNumber);
        DASSERT(mpInputPins[mnInputPinsNumber]);

        mpInputPins[mnInputPinsNumber]->mType = StreamType_Video;
        //LOGM_NOTICE("[DEBUG]: add video pin %d\n", mnInputPinsNumber);
        mnInputPinsNumber ++;
        return EECode_OK;
    } else {
        LOGM_FATAL("BAD input pin type %d\n", type);
        return EECode_InternalLogicalBug;
    }

    return EECode_InternalLogicalBug;
}

IOutputPin *CScheduledMuxer::GetOutputPin(TUint index, TUint sub_index)
{
    LOGM_FATAL("CScheduledMuxer do not have output pin\n");
    return NULL;
}

IInputPin *CScheduledMuxer::GetInputPin(TUint index)
{
    if (EConstAudioRendererMaxInputPinNumber > index) {
        return mpInputPins[index];
    } else {
        LOGM_ERROR("BAD index %d, max value %d, in CScheduledMuxer::GetInputPin()\n", index, EConstAudioRendererMaxInputPinNumber);
    }

    return NULL;
}

EECode CScheduledMuxer::FilterProperty(EFilterPropertyType property, TUint is_set, void *p_param)
{
    EECode ret = EECode_OK;

    switch (property) {

        case EFilterPropertyType_update_sink_url:
            if (mbStarted) {
                SCMD cmd;

                cmd.code = ECMDType_UpdateUrl;
                cmd.pExtra = (void *) p_param;
                cmd.needFreePExtra = 0;

                mpCmdQueue->SendMsg(&cmd, sizeof(cmd));
            } else {
                if (p_param) {
                    ret = setOutputFile((TChar *)p_param);
                    DASSERT_OK(ret);

                    DASSERT(mpOutputFileNameBase);
                    analyseFileNameFormat(mpOutputFileNameBase);
                } else {

                }
            }
            break;

        case EFilterPropertyType_muxer_stop:
            LOGM_ERROR("TO DO\n");
            break;

        case EFilterPropertyType_muxer_start:
            LOGM_ERROR("TO DO\n");
            break;

        case EFilterPropertyType_muxer_finalize_file:
            LOGM_ERROR("TO DO\n");
            break;

        case EFilterPropertyType_muxer_saving_strategy:
            if (DLikely(p_param)) {
                if (is_set) {
                    if (mbStarted) {
                        SCMD cmd;

                        cmd.code = ECMDType_UpdateRecSavingStrategy;
                        cmd.pExtra = p_param;
                        cmd.needFreePExtra = 0;

                        ret = mpCmdQueue->SendMsg(&cmd, sizeof(cmd));
                    } else {
                        LOGM_ERROR("EFilterPropertyType_muxer_saving_strategy, muxer not started.\n");
                        ret = EECode_BadState;
                    }
                } else {
                    LOGM_ERROR("EFilterPropertyType_muxer_saving_strategy: TODO\n");
                }
            } else {
                LOGM_ERROR("NULL p_param\n");
                ret = EECode_BadParam;
            }
            break;

        case EFilterPropertyType_muxer_set_channel_name: {
                if (DLikely(p_param)) {
                    TChar *pChannelname = static_cast<TChar *>(p_param);
                    if (is_set) {
                        if (mpChannelName) {
                            if (mChannelNameBufferSize > strlen(pChannelname)) {
                                snprintf(mpChannelName, mChannelNameBufferSize, "%s", pChannelname);
                            } else {
                                free(mpChannelName);
                                mpChannelName = NULL;
                            }
                        }
                        mpChannelName = (TChar *)malloc(strlen(pChannelname) + 4);
                        DASSERT(mpChannelName);
                        mChannelNameBufferSize = strlen(pChannelname) + 4;
                        snprintf(mpChannelName, mChannelNameBufferSize, "%s", pChannelname);
                    }
                }
            }
            break;

        default:
            LOGM_FATAL("BAD property 0x%08x\n", property);
            ret = EECode_InternalLogicalBug;
            break;
    }

    return ret;
}

void CScheduledMuxer::GetInfo(INFO &info)
{
    info.mPriority = 0;
    info.mFlags = 0;
    info.nInput = mnInputPinsNumber;
    info.nOutput = 1;

    info.pName = mpModuleName;
}

void CScheduledMuxer::PrintStatus()
{
    TUint i = 0;

    LOGM_PRINTF("msState=%d, %s, mnInputPinsNumber %d, mpMuxer %p, heart beat %d\n", msState, gfGetModuleStateString(msState), mnInputPinsNumber, mpMuxer, mDebugHeartBeat);

    for (i = 0; i < mnInputPinsNumber; i ++) {
        LOGM_PRINTF("       inputpin[%p, %d]'s status:\n", mpInputPins[i], i);
        if (mpInputPins[i]) {
            mpInputPins[i]->PrintStatus();
        }
    }

    if (mpMuxer) {
        mpMuxer->PrintStatus();
    }

    mDebugHeartBeat = 0;

    return;
}

EECode CScheduledMuxer::setOutputFile(TChar *pFileName)
{
    //param check
    if (!pFileName) {
        LOGM_FATAL("NULL params in CScheduledMuxer::SetOutputFile.\n");
        return EECode_BadParam;
    }

    TUint currentLen = strlen(pFileName) + 1;

    LOGM_INFO("CScheduledMuxer::SetOutputFile len %d, filename %s.\n", currentLen, pFileName);
    if (!mpOutputFileNameBase || (currentLen + 4) > mOutputFileNameBaseLength) {
        if (mpOutputFileNameBase) {
            free(mpOutputFileNameBase);
            mpOutputFileNameBase = NULL;
        }

        mOutputFileNameBaseLength = currentLen + 4;
        if ((mpOutputFileNameBase = (char *)malloc(mOutputFileNameBaseLength)) == NULL) {
            mOutputFileNameBaseLength = 0;
            return EECode_NoMemory;
        }
    }

    DASSERT(mpOutputFileNameBase);
    strncpy(mpOutputFileNameBase, pFileName, currentLen - 1);
    mpOutputFileNameBase[currentLen - 1] = 0x0;
    LOGM_INFO("CScheduledMuxer::SetOutputFile done, len %d, filenamebase %s.\n", mOutputFileNameBaseLength, mpOutputFileNameBase);
    return EECode_OK;
}

EECode CScheduledMuxer::saveStreamInfo(StreamType type, TUint stream_index, CIBuffer *p_buffer)
{
    DASSERT(stream_index < EConstMaxDemuxerMuxerStreamNumber);

    DASSERT(p_buffer);
    if (!p_buffer) {
        LOGM_FATAL("NULL p_buffer %p\n", p_buffer);
        return EECode_BadParam;
    }

    LOGM_INFO("saveStreamInfo[%d]\n", stream_index);
    if (stream_index < EConstMaxDemuxerMuxerStreamNumber) {
        if (!mStreamCodecInfos.info[stream_index].inited) {
            mStreamCodecInfos.info[stream_index].inited = 1;
            mStreamCodecInfos.total_stream_number ++;
        }
        DASSERT(mStreamCodecInfos.total_stream_number <= EConstMaxDemuxerMuxerStreamNumber);

        mStreamCodecInfos.info[stream_index].stream_type = type;

        switch (type) {

            case StreamType_Video:
                mStreamCodecInfos.info[stream_index].stream_index = stream_index;
                mStreamCodecInfos.info[stream_index].stream_enabled = 1;
                mStreamCodecInfos.info[stream_index].stream_format = StreamFormat_H264;//hard code here

                mStreamCodecInfos.info[stream_index].spec.video.pic_width = p_buffer->mVideoWidth;
                mStreamCodecInfos.info[stream_index].spec.video.pic_height = p_buffer->mVideoHeight;
                mStreamCodecInfos.info[stream_index].spec.video.pic_offset_x = 0;
                mStreamCodecInfos.info[stream_index].spec.video.pic_offset_y = 0;
                mStreamCodecInfos.info[stream_index].spec.video.framerate_num = p_buffer->mVideoFrameRateNum;
                mStreamCodecInfos.info[stream_index].spec.video.framerate_den = p_buffer->mVideoFrameRateDen;
                mStreamCodecInfos.info[stream_index].spec.video.sample_aspect_ratio_num = p_buffer->mVideoSampleAspectRatioNum;
                mStreamCodecInfos.info[stream_index].spec.video.sample_aspect_ratio_den = p_buffer->mVideoSampleAspectRatioDen;

                mStreamCodecInfos.info[stream_index].spec.video.framerate = (p_buffer->mVideoFrameRateNum / p_buffer->mVideoFrameRateDen);
                mStreamCodecInfos.info[stream_index].spec.video.bitrate = p_buffer->mVideoBitrate;

                if (mpInputPins[stream_index]) {
                    mpInputPins[stream_index]->mBitrate = p_buffer->mVideoBitrate;
                }
                break;

            case StreamType_Audio:
                mStreamCodecInfos.info[stream_index].stream_index = stream_index;
                mStreamCodecInfos.info[stream_index].stream_enabled = 1;
                mStreamCodecInfos.info[stream_index].stream_format = StreamFormat_AAC;//hard code here

                mStreamCodecInfos.info[stream_index].spec.audio.sample_rate = p_buffer->mAudioSampleRate;
                mStreamCodecInfos.info[stream_index].spec.audio.sample_format = (AudioSampleFMT)p_buffer->mAudioSampleFormat;
                mStreamCodecInfos.info[stream_index].spec.audio.channel_number = p_buffer->mAudioChannelNumber;
                mStreamCodecInfos.info[stream_index].spec.audio.frame_size = p_buffer->mAudioFrameSize;
                mStreamCodecInfos.info[stream_index].spec.audio.bitrate = p_buffer->mAudioBitrate;
                if (mpInputPins[stream_index]) {
                    mpInputPins[stream_index]->mBitrate = p_buffer->mAudioBitrate;
                }
                break;

            case StreamType_Subtitle:
                LOGM_FATAL("TO DO\n");
                return EECode_NoImplement;
                break;

            case StreamType_PrivateData:
                LOGM_FATAL("TO DO\n");
                return EECode_NoImplement;
                break;

            default:
                LOGM_FATAL("BAD StreamType %d\n", type);
                return EECode_InternalLogicalBug;
                break;
        }
    } else {
        LOGM_FATAL("BAD stream_index %d\n", stream_index);
        return EECode_BadParam;
    }

    return EECode_OK;
}

EECode CScheduledMuxer::saveExtraData(StreamType type, TUint stream_index, void *p_data, TUint data_size)
{
    DASSERT(stream_index < EConstMaxDemuxerMuxerStreamNumber);

    DASSERT(p_data);
    DASSERT(data_size);
    if (!p_data || !data_size) {
        LOGM_FATAL("NULL p_data %p, or zero data_size %d\n", p_data, data_size);
        return EECode_BadParam;
    }

    LOGM_INFO("saveExtraData[%d], data %p, data size %d\n", stream_index, p_data, data_size);
    if (stream_index < EConstMaxDemuxerMuxerStreamNumber) {
        if (mpExtraData[stream_index]) {
            if (mExtraDataSize[stream_index] >= data_size) {
                LOGM_INFO("replace extra data\n");
                memcpy(mpExtraData[stream_index], p_data, data_size);
                mExtraDataSize[stream_index] = data_size;
            } else {
                LOGM_INFO("replace extra data, with larger size\n");
                free(mpExtraData[stream_index]);
                mpExtraData[stream_index] = (TU8 *)malloc(data_size);
                if (mpExtraData[stream_index]) {
                    memcpy(mpExtraData[stream_index], p_data, data_size);
                    mExtraDataSize[stream_index] = data_size;
                } else {
                    LOGM_FATAL("malloc(%d) fail\n", data_size);
                    return EECode_NoMemory;
                }
            }
        } else {
            mpExtraData[stream_index] = (TU8 *)malloc(data_size);
            if (mpExtraData[stream_index]) {
                memcpy(mpExtraData[stream_index], p_data, data_size);
                mExtraDataSize[stream_index] = data_size;
            } else {
                LOGM_FATAL("malloc(%d) fail\n", data_size);
                return EECode_NoMemory;
            }
        }
    } else {
        LOGM_FATAL("BAD stream_index %d\n", stream_index);
        return EECode_BadParam;
    }

    return EECode_OK;
}

EECode CScheduledMuxer::setExtraData()
{
    TUint i = 0;
    EECode err = EECode_OK;

    DASSERT(mpMuxer);
    if (mpMuxer) {
        DASSERT(mnInputPinsNumber);
        for (i = 0; i < mnInputPinsNumber; i++) {
            DASSERT(mpExtraData[i]);
            DASSERT(mpInputPins[i]);
            if (mpExtraData[i]) {
                err = mpMuxer->SetExtraData(mpInputPins[i]->mType, i, mpExtraData[i], mExtraDataSize[i]);
                DASSERT_OK(err);
            }
        }
    } else {
        LOGM_FATAL("NULL mpMuxer\n");
        return EECode_BadParam;
    }

    return EECode_OK;
}

void CScheduledMuxer::deletePartialFile(TUint file_index)
{
    TChar *cmd = (char *)malloc(mOutputFileNameLength + 16);
    TInt ret = 0;

    if (NULL == cmd) {
        LOGM_ERROR("Cannot malloc memory in CScheduledMuxer::deletePartialFile, request size %d,\n", mOutputFileNameLength + 16);
        return;
    }
    memset(cmd, 0, mOutputFileNameLength + 16);
    LOGM_INFO("start remove file %s_%06d.%s.\n", mpOutputFileNameBase, file_index, gfGetContainerStringFromType(mRequestContainerType));
    snprintf(cmd, mOutputFileNameLength + 16, "rm %s_%06d.%s", mpOutputFileNameBase, file_index, gfGetContainerStringFromType(mRequestContainerType));
    LOGM_INFO("cmd: %s.\n", cmd);
    ret = system(cmd);
    DASSERT(0 == ret);
    LOGM_INFO("end remove file %s_%06d.%s.\n", mpOutputFileNameBase, file_index, gfGetContainerStringFromType(mRequestContainerType));
    free(cmd);
}

void CScheduledMuxer::analyseFileNameFormat(TChar *pstart)
{
    TChar *pcur = NULL;
    ContainerType type_fromname;

    if (!pstart) {
        LOGM_ERROR("NULL mpOutputFileNameBase.\n");
        return;
    }

    LOGM_INFO("[analyseFileNameFormat], input %s.\n", pstart);

    //check if need append
    pcur = strrchr(pstart, '.');
    if (pcur) {
        pcur ++;
        type_fromname = gfGetContainerTypeFromString(pcur);
        if (ContainerType_Invalid == type_fromname) {
            LOGM_WARN("[analyseFileNameFormat], not supported extention %s, append correct file externtion.\n", pcur);
            mFileNameHanding1 = eFileNameHandling_appendExtention;
        } else {
            LOGM_INFO("[analyseFileNameFormat], have valid file name externtion %s.\n", pcur);
            mFileNameHanding1 = eFileNameHandling_noAppendExtention;
            //DASSERT(type_fromname == mRequestContainerType);
            if (type_fromname != mRequestContainerType) {
                LOGM_WARN("[analyseFileNameFormat], container type(%d) from file name not match preset value %d, app need invoke setOutputFormat(to AMRecorder), and make sure setOutput(to engine)'s parameters consist?\n", type_fromname,  mRequestContainerType);
            }
        }
    } else {
        LOGM_INFO("[analyseFileNameFormat], have no file name externtion.\n");
        mFileNameHanding1 = eFileNameHandling_appendExtention;
    }

    //check if need insert, only process one %, todo
    pcur = strchr(pstart, '%');
    //have '%'
    if (pcur) {
        pcur ++;
        if ('t' == (*pcur)) {
            //if have '%t'
            LOGM_INFO("[analyseFileNameFormat], file name is insertDataTime mode.\n");
            mFileNameHanding2 = eFileNameHandling_insertDateTime;
            *pcur = 's';//use '%s', to do
        } else if (('d' == (*pcur)) || ('d' == (*(pcur + 1))) || ('d' == (*(pcur + 2)))) {
            //if have '%d' or '%06d'
            LOGM_INFO("[analyseFileNameFormat], file name is insertFileNumber mode.\n");
            mFileNameHanding2 = eFileNameHandling_insertFileNumber;
        }
    } else {
        LOGM_INFO("[analyseFileNameFormat], file name not need insert something.\n");
        mFileNameHanding2 = eFileNameHandling_noInsert;
    }

    LOGM_INFO("[analyseFileNameFormat], done, input %s, result mFileNameHanding1 %d, mFileNameHanding2 %d.\n", pstart, mFileNameHanding1, mFileNameHanding2);
}

//update file name and thumbnail name
void CScheduledMuxer::updateFileName(TUint file_index)
{
    LOGM_INFO("before append extntion: Muxing file name %s, mFileNameHanding1 %d, mFileNameHanding2 %d.\n", mpOutputFileName, mFileNameHanding1, mFileNameHanding2);

    //ffmpeg use extention to specify container type
    /*if (strncmp("sharedfd://", mpOutputFileName, strlen("sharedfd://")) == 0) {
        LOGM_WARN("shared fd, not append file externtion.\n");
        return;
    }*/

    if (mpStorageManager && mpChannelName) {
        EECode err = EECode_OK;
        if (mpOutputFileName) { err = mpStorageManager->FinalizeNewUint(mpChannelName, mpOutputFileName); }
        DASSERT(err == EECode_OK);
        mpOutputFileName = NULL;
        err = mpStorageManager->AcquireNewUint(mpChannelName, mpOutputFileName);
        if (err == EECode_OK) {
            LOGM_NOTICE("update file name done, file name %s\n", mpOutputFileName);
        } else {
            LOGM_FATAL("update file name fail!\n");
        }
        return;
    }

    if (file_index > 30) {
        file_index = 0;
    }

    if (!mpOutputFileName || mOutputFileNameLength < (strlen(mpOutputFileNameBase) + DMaxFileExterntionLength + DMaxFileIndexLength + 4)) {
        if (mpOutputFileName) {
            //debug log
            LOGM_WARN("filename buffer(%p) not enough, remalloc it, or len %d, request len %lu.\n", mpOutputFileName, mOutputFileNameLength, (TULong)(strlen(mpOutputFileNameBase) + DMaxFileExterntionLength + DMaxFileIndexLength + 4));
            free(mpOutputFileName);
            mpOutputFileName = NULL;
        }
        mOutputFileNameLength = strlen(mpOutputFileNameBase) + DMaxFileExterntionLength + DMaxFileIndexLength + 4;
        if ((mpOutputFileName = (char *)malloc(mOutputFileNameLength)) == NULL) {
            mOutputFileNameLength = 0;
            LOGM_ERROR("No Memory left, how to handle this issue.\n");
            return;
        }
    }

    if (eFileNameHandling_appendExtention == mFileNameHanding1) {
        if (eFileNameHandling_noInsert == mFileNameHanding2) {
            if (MuxerSavingFileStrategy_ToTalFile == mSavingFileStrategy) {
                snprintf(mpOutputFileName, mOutputFileNameLength, "%s.%s", mpOutputFileNameBase, gfGetContainerStringFromType(mRequestContainerType));
            } else {
                if (MuxerAutoFileNaming_ByNumber == mAutoFileNaming) {
                    snprintf(mpOutputFileName, mOutputFileNameLength, "%s_%06d.%s", mpOutputFileNameBase, file_index, gfGetContainerStringFromType(mRequestContainerType));
                } else if (MuxerAutoFileNaming_ByDateTime == mAutoFileNaming) {
                    TChar datetime_buffer[128];
                    gfGetSTRFTimeString(datetime_buffer, sizeof(datetime_buffer));
                    snprintf(mpOutputFileName, mOutputFileNameLength, "%s%s.%s", mpOutputFileNameBase, datetime_buffer, gfGetContainerStringFromType(mRequestContainerType));
                }
            }
        } else if (eFileNameHandling_insertFileNumber == mFileNameHanding2) {
            snprintf(mpOutputFileName, mOutputFileNameLength - 8, mpOutputFileNameBase, file_index);
            strncat(mpOutputFileName, ".", mOutputFileNameLength);
            strncat(mpOutputFileName, gfGetContainerStringFromType(mRequestContainerType), mOutputFileNameLength);
        } else if (eFileNameHandling_insertDateTime == mFileNameHanding2) {
            char datetime_buffer[128];
            gfGetSTRFTimeString(datetime_buffer, sizeof(datetime_buffer));

            snprintf(mpOutputFileName, mOutputFileNameLength - 8, mpOutputFileNameBase, datetime_buffer);
            strncat(mpOutputFileName, ".", mOutputFileNameLength);
            strncat(mpOutputFileName, gfGetContainerStringFromType(mRequestContainerType), mOutputFileNameLength);
        } else {
            DASSERT(0);
        }
    } else if (eFileNameHandling_noAppendExtention == mFileNameHanding1) {
        if (eFileNameHandling_insertFileNumber == mFileNameHanding2) {
            snprintf(mpOutputFileName, mOutputFileNameLength, mpOutputFileNameBase, file_index);
        } else if (eFileNameHandling_insertDateTime == mFileNameHanding2) {
            char datetime_buffer[128];
            gfGetSTRFTimeString(datetime_buffer, sizeof(datetime_buffer));
            snprintf(mpOutputFileName, mOutputFileNameLength, mpOutputFileNameBase, datetime_buffer);
        } else {
            if (eFileNameHandling_noInsert != mFileNameHanding2) {
                LOGM_ERROR("ERROR parameters(mFileNameHanding2 %d) here, please check.\n", mFileNameHanding2);
            }

            if (MuxerSavingFileStrategy_ToTalFile == mSavingFileStrategy) {
                //do nothing
                strncpy(mpOutputFileName, mpOutputFileNameBase, mOutputFileNameLength - 1);
                mpOutputFileName[mOutputFileNameLength - 1] = 0x0;
            } else {
                char *ptmp = strrchr(mpOutputFileNameBase, '.');
                DASSERT(ptmp);
                if (ptmp) {
                    *ptmp++ = '\0';

                    //insert if auto cut
                    if (MuxerAutoFileNaming_ByNumber == mAutoFileNaming) {
                        snprintf(mpOutputFileName, mOutputFileNameLength, "%s%06d.%s", mpOutputFileNameBase, file_index, ptmp);
                    } else if (MuxerAutoFileNaming_ByDateTime == mAutoFileNaming) {
                        char datetime_buffer[128];
                        memset(datetime_buffer, 0, sizeof(datetime_buffer));
                        gfGetSTRFTimeString(datetime_buffer, sizeof(datetime_buffer));
                        snprintf(mpOutputFileName, mOutputFileNameLength, "%s%s.%s", mpOutputFileNameBase, datetime_buffer, ptmp);
                    } else {
                        DASSERT(0);
                    }
                    *(ptmp - 1) = '.';
                } else {
                    LOGM_ERROR("SHOULD not comes here, please check code.\n");
                    strncpy(mpOutputFileName, mpOutputFileNameBase, mOutputFileNameLength - 1);
                    mpOutputFileName[mOutputFileNameLength - 1] = 0x0;
                }
            }
        }
    }

    LOGM_INFO("after append extntion: Muxing file name %s\n", mpOutputFileName);
    DASSERT((TUint)(strlen(mpOutputFileName)) <= mOutputFileNameLength);

    if (mpPersistMediaConfig->encoding_mode_config.thumbnail_enabled && mpThumbNailFileNameBase) {
        LOGM_INFO("before append extntion: Thumbnail file name %s", mpThumbNailFileName);
        //param check
        if (!mpThumbNailFileName || mThumbNailFileNameLength < (strlen(mpThumbNailFileNameBase) + DMaxFileExterntionLength + DMaxFileIndexLength + 4)) {
            if (mpThumbNailFileName) {
                //debug log
                LOGM_WARN("thumbnail file name buffer(%p) not enough, remalloc it, or len %d, request len %lu.\n", mpThumbNailFileName, mThumbNailFileNameLength, (TULong)(strlen(mpThumbNailFileNameBase) + DMaxFileExterntionLength + DMaxFileIndexLength + 4));
                free(mpThumbNailFileName);
                mpThumbNailFileName = NULL;
            }
            mThumbNailFileNameLength = strlen(mpThumbNailFileNameBase) + DMaxFileExterntionLength + DMaxFileIndexLength + 4;
            if ((mpThumbNailFileName = (char *)malloc(mThumbNailFileNameLength)) == NULL) {
                mThumbNailFileNameLength = 0;
                LOGM_ERROR("No Memory left, how to handle this issue.\n");
                return;
            }
        }

        //the name of thumbnail is like as .XXXXfile_index.thm
        snprintf(mpThumbNailFileName, mThumbNailFileNameLength, mpThumbNailFileNameBase, file_index);
        LOGM_INFO("after append extntion: Thumbnail file name %s\n", mpThumbNailFileName);
    }
}

TU8 CScheduledMuxer::isCommingPTSNotContinuous(CMuxerInput *pInput, CIBuffer *pBuffer)
{
    DASSERT(pInput);
    DASSERT(pBuffer);

    TTime pts = pBuffer->GetBufferLinearPTS() - pInput->mSessionInputPTSStartPoint + pInput->mSessionPTSStartPoint;
    if ((pInput->mCurrentDTS > (pts + MaxDTSDrift * pInput->mDuration)) || (pts > (pInput->mCurrentDTS + MaxDTSDrift * pInput->mDuration))) {
        LOGM_WARN("PTS gap more than %d x duration, DTS %lld, PTS %lld, BufferPTS %lld ,duration %lld, up-stream filter discarded packet? re-align pts.\n", \
                  MaxDTSDrift, pInput->mCurrentDTS, pts, pBuffer->GetBufferLinearPTS(), pInput->mDuration);
        return 1;
    }
    return 0;
}

TU8 CScheduledMuxer::isCommingBufferAutoFileBoundary(CMuxerInput *pInputPin, CIBuffer *pBuffer)
{
    TTime pts = 0;
    DASSERT(mpMasterInput);
    DASSERT(pInputPin);
    DASSERT(pBuffer);

    pts = pBuffer->GetBufferLinearPTS();
    if ((pInputPin == mpMasterInput) && (EPredefinedPictureType_IDR == pBuffer->mVideoFrameType)) {
        //master video input pin
        DASSERT(pInputPin->mType == StreamType_Video);
        LOGM_DEBUG("[AUTO-CUT]master's IDR comes.\n");
        if (!mbMasterStarted) {
            return 0;
        }
        if (mSavingCondition == MuxerSavingCondition_FrameCount) {
            if ((pInputPin->mCurrentFrameCount + 1) < mAutoSaveFrameCount) {
                return 0;
            }
            return 1;
        } else if (mSavingCondition == MuxerSavingCondition_InputPTS) {
            //check input pts
            if (pts < mNextFileTimeThreshold) {
                return 0;
            }
            //AMLOG_WARN(" Last  PTS = %llu, mAutoSavingTimeDuration = %llu\n", pBuffer->GetBufferLinearPTS(), mAutoSavingTimeDuration);
            return 1;
        } else if (mSavingCondition == MuxerSavingCondition_CalculatedPTS) {
            //check calculated pts
            TS64 comming_pts = pts - pInputPin->mSessionInputPTSStartPoint + pInputPin->mSessionPTSStartPoint;
            if (comming_pts < mNextFileTimeThreshold) {
                return 0;
            }
            //AMLOG_WARN(" Last  PTS = %llu, mAutoSavingTimeDuration = %llu\n", pBuffer->GetBufferLinearPTS(), mAutoSavingTimeDuration);
            return 1;
        } else {
            LOGM_ERROR("NEED implement.\n");
            return 0;
        }
    } else if ((pInputPin != mpMasterInput) && (pInputPin->mType != StreamType_Video) && (_pinNeedSync((StreamType)pInputPin->mType))) {

        if (!mpPersistMediaConfig->cutfile_with_precise_pts) {
            //ignore non-master(non-video)'s pts
            return 0;
        }

        //no non-key frame condition
        if (pts < mAudioNextFileTimeThreshold) {
            return 0;
        }
        return 1;
    } else if ((pInputPin != mpMasterInput) && (pInputPin->mType == StreamType_Video)) {
        //other cases, like multi-video inputpin's case, need implement
        LOGM_ERROR("NEED implement.\n");
    }

    return 0;
}

TU8 CScheduledMuxer::isReachPresetConditions(CMuxerInput *pinput)
{
    if ((StreamType_Video == pinput->mType) && (mPresetCheckFlag & EPresetCheckFlag_videoframecount)) {
        if (pinput->mTotalFrameCount >= mPresetVideoMaxFrameCnt) {
            mPresetReachType = EPresetCheckFlag_videoframecount;
            return 1;
        }
    } else if ((StreamType_Audio == pinput->mType) && (mPresetCheckFlag & EPresetCheckFlag_audioframecount)) {
        if (pinput->mTotalFrameCount >= mPresetAudioMaxFrameCnt) {
            mPresetReachType = EPresetCheckFlag_audioframecount;
            return 1;
        }
    } else if ((mPresetCheckFlag & EPresetCheckFlag_filesize) && (mCurrentTotalFilesize >= mPresetMaxFilesize)) {
        mPresetReachType = EPresetCheckFlag_filesize;
        return 1;
    }
    return 0;
}

TU8 CScheduledMuxer::hasPinNeedReachBoundary(TUint &i)
{
    for (i = 0; i < mnInputPinsNumber; i++) {
        if (mpInputPins[i]) {
            if (_pinNeedSync((StreamType)mpInputPins[i]->mType) && (0 == mpInputPins[i]->mbAutoBoundaryReached)) {
                return 1;
            }
        }
    }
    i = 0;
    return 0;
}

TU8 CScheduledMuxer::allInputEos()
{
    TUint index;
    for (index = 0; index < mnInputPinsNumber; index ++) {
        DASSERT(mpInputPins[index]);
        if (mpInputPins[index]) {
            if (mpInputPins[index]->mEOSComes == 0) {
                return 0;
            }
        } else {
            LOGM_FATAL("NULL mpInputPins[%d], should have error.\n", index);
        }
    }
    return 1;
}

void CScheduledMuxer::postMsg(TUint msg_code)
{
    DASSERT(mpEngineMsgSink);
    if (mpEngineMsgSink) {
        SMSG msg;
        msg.code = msg_code;
        LOGM_INFO("Post msg_code %d to engine.\n", msg_code);
        mpEngineMsgSink->MsgProc(msg);
        LOGM_INFO("Post msg_code %d to engine done.\n", msg_code);
    }
}

TU8 CScheduledMuxer::readInputData(CMuxerInput *inputpin, EBufferType &type)
{
    DASSERT(!mpBuffer);
    DASSERT(inputpin);
    EECode ret;

    if (!inputpin->PeekBuffer(mpBuffer)) {
        LOGM_FATAL("No buffer? msState=%d\n", msState);
        return 0;
    }

    type = mpBuffer->GetBufferType();

    if (EBufferType_FlowControl_EOS == type) {
        LOGM_INFO("CScheduledMuxer %p get EOS.\n", this);
        inputpin->mEOSComes = 1;
        return 1;
    } else if (EBufferType_FlowControl_Pause == type) {
        LOGM_INFO("pause packet comes.\n");
        inputpin->timeDiscontinue();
        inputpin->mbSkiped = 1;

        if (mbMuxerPaused == 0) {
            mbMuxerPaused = 1;
        } else {
            //get second stream's pause packet, finalize file
            LOGM_INFO("Muxer stream get FlowControl_pause packet, finalize current file!\n");
            ret = finalizeFile();
            if (ret != EECode_OK) {
                LOGM_ERROR("FlowControl_pause processing, finalize file failed!\n");
            }
        }
    } else if (EBufferType_FlowControl_Resume == type) {
        LOGM_INFO("resume packet comes.\n");
        inputpin->mbSkiped = 0;

        if (mbMuxerPaused) {
            LOGM_INFO("Muxer get FlowControl_resume packet, initialize new file!\n");
            ++mCurrentFileIndex;
            updateFileName(mCurrentFileIndex);
            ret = initializeFile(mpOutputFileName, mRequestContainerType);
            if (ret != EECode_OK) {
                LOGM_ERROR("FlowControl_resume processing, initialize file failed!\n");
                inputpin->mbSkiped = 1;
            }
            mbMuxerPaused = 0;
        }
    } else if (_isMuxerInputData(type)) {
        if (((TTime)DINVALID_VALUE_TAG_64) == mpBuffer->GetBufferLinearPTS()) {
            LOGM_WARN("no pts, estimate it\n");
            mpBuffer->SetBufferPTS(inputpin->mCurrentPTS + inputpin->mDuration);
        }
    }


    //DASSERT(inputpin->mbSkiped == 0);
    if (inputpin->mbSkiped == 1) {
        LOGM_WARN("recieved data between flow_control pause and flow_control resume, discard packet here.\n");
        mpBuffer->Release();
        mpBuffer = NULL;
        return 0;
    }

    DASSERT(mpBuffer);
    DASSERT(mpBuffer->GetDataPtr());
    DASSERT(mpBuffer->GetDataSize());
    return 1;
}

EECode CScheduledMuxer::writeData(CIBuffer *pBuffer, CMuxerInput *pPin)
{
    SMuxerDataTrunkInfo data_trunk_info;
    TTime buffer_pts = 0;
    EECode err = EECode_OK;
    DASSERT(pBuffer);
    DASSERT(pPin);

    buffer_pts = pBuffer->GetBufferLinearPTS();
    DASSERT(buffer_pts >= 0);

    data_trunk_info.native_dts = pBuffer->GetBufferNativeDTS();
    data_trunk_info.native_pts = pBuffer->GetBufferNativePTS();
    pPin->mInputEndPTS = buffer_pts;

    if (StreamType_Audio == pPin->mType) {

        //ensure first frame are key frame
        if (pPin->mCurrentFrameCount == 0) {
            DASSERT(pPin->mDuration);
            //estimate current pts
            pPin->mSessionInputPTSStartPoint = buffer_pts;
            if (pPin->mCurrentPTS != mStreamStartPTS) {
                pPin->mSessionPTSStartPoint = pPin->mCurrentPTS;
            }
            LOGM_INFO(" audio mSessionInputPTSStartPoint %lld, mSessionPTSStartPoint %lld, pInput->mDuration %lld.\n", pPin->mSessionInputPTSStartPoint, pPin->mSessionPTSStartPoint, pPin->mDuration);
        }

        if (!pPin->mbAutoBoundaryStarted) {
            pPin->mbAutoBoundaryStarted = true;
            pPin->mInputSatrtPTS = buffer_pts;
            LOGM_INFO("[cut file, boundary start(audio), pts %llu]", pPin->mInputSatrtPTS);
        }

        pPin->mCurrentPTS = buffer_pts - pPin->mSessionInputPTSStartPoint + pPin->mSessionPTSStartPoint;
        LOGM_PTS("[PTS]:audio mCurrentPTS %lld, pBuffer->GetBufferLinearPTS() %lld, pInput->mTimeUintNum %d, pInput->mTimeUintDen %d.\n", pPin->mCurrentPTS, buffer_pts, pPin->mTimeUintNum, pPin->mTimeUintDen);

        data_trunk_info.pts = data_trunk_info.dts = pPin->mCurrentPTS + pPin->mPTSDTSGap;

        LOGM_PTS("[PTS]:audio write to file pts %lld, dts %lld, pts gap %lld, duration %lld, pInput->mPTSDTSGap %d.\n", data_trunk_info.pts, data_trunk_info.dts, data_trunk_info.pts - pPin->mLastPTS, pPin->mDuration, pPin->mPTSDTSGap);
        pPin->mLastPTS = data_trunk_info.pts;
        data_trunk_info.duration = pPin->mDuration;
        data_trunk_info.av_normalized_duration = pPin->mAVNormalizedDuration;
        mCurrentTotalFilesize += pBuffer->GetDataSize();

        data_trunk_info.is_key_frame = 1;
        data_trunk_info.stream_index = pPin->mStreamIndex;
        //NOTE: need convert audio PTS from samplerate unit to tick unit, otherwise muxed audio PTS almost less than PcrBase, then audio data will be dropped by VLC
        if (mStreamCodecInfos.info[pPin->mStreamIndex].spec.audio.sample_rate) {
            data_trunk_info.pts = data_trunk_info.dts = data_trunk_info.pts * TICK_PER_SECOND / mStreamCodecInfos.info[pPin->mStreamIndex].spec.audio.sample_rate;
        }
        err = mpMuxer->WriteAudioBuffer(pBuffer, &data_trunk_info);
        DASSERT_OK(err);
    } else if (StreamType_Video == pPin->mType) {

        //master pin
        if ((!mbNextFileTimeThresholdSet) && (MuxerSavingFileStrategy_AutoSeparateFile == mSavingFileStrategy)) {
            mVideoFirstPTS = buffer_pts;
            if (MuxerSavingCondition_InputPTS == mSavingCondition) {
                //start from input pts
                mNextFileTimeThreshold = buffer_pts + mAutoSavingTimeDuration;
            } else if (MuxerSavingCondition_CalculatedPTS == mSavingCondition) {
                //linear to start from 0
                mNextFileTimeThreshold = mAutoSavingTimeDuration;
            }
            mbNextFileTimeThresholdSet = 1;
            LOGM_INFO("Get first Video PTS %llu, mNextFileTimeThreshold %llu.\n", mVideoFirstPTS, mNextFileTimeThreshold);
        }

        //ensure first frame are key frame
        if (0 == pPin->mCurrentFrameCount) {
            //DASSERT(EPredefinedPictureType_IDR == pBuffer->mVideoFrameType);
            if (EPredefinedPictureType_IDR == pBuffer->mVideoFrameType) {
                LOGM_INFO("IDR picture comes, pInput->mDuration %lld, pBuffer->GetBufferLinearPTS() %lld.\n", pPin->mDuration, buffer_pts);

                DASSERT(pPin->mDuration);
                //refresh session pts related
                pPin->mSessionInputPTSStartPoint = buffer_pts;
                if (pPin->mCurrentPTS != mStreamStartPTS) {
                    pPin->mSessionPTSStartPoint = pPin->mCurrentPTS;
                }
                LOGM_INFO(" video pInput->mSessionInputPTSStartPoint %lld, pInput->mSessionPTSStartPoint %lld.\n", pPin->mSessionInputPTSStartPoint, pPin->mSessionPTSStartPoint);
                if (!mbMasterStarted) {
                    LOGM_INFO("master started.\n");
                    mbMasterStarted = 1;
                }
            } else {
                LOGM_INFO("non-IDR frame (%d), data comes here, seq num [%d, %d], when stream not started, discard them.\n", pBuffer->mVideoFrameType, pBuffer->mBufferSeqNumber0, pBuffer->mBufferSeqNumber1);
                return EECode_OK;
            }
        } else if (1 == isCommingPTSNotContinuous(pPin, pBuffer)) {
            //refresh pts, connected to previous
            pPin->mSessionInputPTSStartPoint = pBuffer->GetBufferLinearPTS();
            pPin->mSessionPTSStartPoint = pPin->mCurrentPTS + pPin->mDuration;
            pPin->mCurrentFrameCount = 0;
        }

        pPin->mFrameCountFromLastBPicture ++;

        if (!pPin->mbAutoBoundaryStarted) {
            pPin->mbAutoBoundaryStarted = 1;
            pPin->mInputSatrtPTS = pBuffer->GetBufferLinearPTS();
            LOGM_INFO("[cut file, boundary start(video), pts %llu]", pPin->mInputSatrtPTS);
        }

        if (pBuffer->GetBufferLinearPTS() < pPin->mSessionInputPTSStartPoint) {
            DASSERT(pBuffer->mVideoFrameType == EPredefinedPictureType_B);
            LOGM_WARN(" PTS(%lld) < first IDR PTS(%lld), something should be wrong. or skip b frame(pBuffer->mVideoFrameType %d) after first IDR, these b frame should be skiped, when pause--->resume.\n", buffer_pts, pPin->mSessionInputPTSStartPoint, pBuffer->mVideoFrameType);
            return EECode_OK;
        }

        DASSERT(buffer_pts >= pPin->mSessionInputPTSStartPoint);
        pPin->mCurrentPTS = pBuffer->GetBufferLinearPTS() - pPin->mSessionInputPTSStartPoint + pPin->mSessionPTSStartPoint;
        LOGM_PTS("[PTS]:video mCurrentPTS %lld, mCurrentDTS %lld, pBuffer->GetPTS() %lld, mSessionInputPTSStartPoint %lld, mSessionPTSStartPoint %lld.\n", pPin->mCurrentPTS, pPin->mCurrentDTS, buffer_pts, pPin->mSessionInputPTSStartPoint, pPin->mSessionPTSStartPoint);

        data_trunk_info.dts = pPin->mCurrentDTS;
        data_trunk_info.pts = pPin->mCurrentPTS + pPin->mPTSDTSGap;
        data_trunk_info.duration = pPin->mDuration;
        data_trunk_info.av_normalized_duration = pPin->mAVNormalizedDuration;

        pPin->mCurrentDTS = pPin->mCurrentDTS + pPin->mDuration;

        LOGM_PTS("[PTS]:video write to file pts %lld, dts %lld, pts gap %lld, duration %lld, pInput->mPTSDTSGap %d.\n", data_trunk_info.pts, data_trunk_info.dts, data_trunk_info.pts - pPin->mLastPTS, pPin->mDuration, pPin->mPTSDTSGap);
        pPin->mLastPTS = data_trunk_info.pts;

        data_trunk_info.is_key_frame = ((EPredefinedPictureType_IDR == pBuffer->mVideoFrameType) || (EPredefinedPictureType_I == pBuffer->mVideoFrameType));
        data_trunk_info.stream_index = pPin->mStreamIndex;

        err = mpMuxer->WriteVideoBuffer(pBuffer, &data_trunk_info);
        DASSERT_OK(err);
    } else if (StreamType_PrivateData == pPin->mType) {
        data_trunk_info.pts = buffer_pts;
        data_trunk_info.dts = buffer_pts;
        data_trunk_info.is_key_frame = 1;
        data_trunk_info.stream_index = pPin->mStreamIndex;
        mpMuxer->WritePridataBuffer(pBuffer, &data_trunk_info);
    } else {
        LOGM_ERROR("BAD pin type %d.\n", pPin->mType);
        return EECode_BadFormat;
    }

    pPin->mCurrentFrameCount ++;
    pPin->mTotalFrameCount ++;

    return EECode_OK;
}

void CScheduledMuxer::getFileInformation()
{
    TUint i = 0, tot_validpins = 0;
    TUint estimated_bitrate = 0;
    TTime first_stream_start_pts, first_stream_end_pts, first_stream_duration, first_input_duration;
    TTime diff_start_pts, diff_end_pts, diff_duration, diff_input_duration;

    //avsync related print: start pts's difference, end pts's difference, and duration's difference
    if (mpInputPins[0]) {
        //get first stream's
        first_stream_start_pts = mpInputPins[0]->mInputSatrtPTS;
        first_stream_end_pts = mpInputPins[0]->mInputEndPTS;
        first_input_duration = mpInputPins[0]->mInputEndPTS - mpInputPins[0]->mInputSatrtPTS + mpInputPins[0]->mDuration;
        first_stream_duration = mpInputPins[0]->mCurrentPTS - mpInputPins[0]->mSessionPTSStartPoint + mpInputPins[0]->mDuration;

        LOGM_INFO("[file info]: avsync, first stream start input pts %llu, end input pts %llu, duration %llu.\n", first_stream_start_pts, first_stream_end_pts, first_stream_duration);
        for (i = 1; i < mnInputPinsNumber; i++) {
            if (mpInputPins[i]) {
                LOGM_INFO("[file info]: avsync, stream(%d) start input pts %llu, end input pts %llu, duration %llu.\n", i, mpInputPins[i]->mInputSatrtPTS, mpInputPins[i]->mInputEndPTS, mpInputPins[i]->mCurrentPTS + mpInputPins[i]->mDuration - mpInputPins[i]->mSessionPTSStartPoint);

                if (StreamType_Audio == mpInputPins[i]->mType) {
                    //audio has a duration gap, buffer in encoder
                    diff_start_pts = mpInputPins[i]->mInputSatrtPTS - mpInputPins[i]->mDuration - first_stream_start_pts;
                    diff_end_pts = mpInputPins[i]->mInputEndPTS - mpInputPins[i]->mDuration - first_stream_end_pts;
                } else {
                    diff_start_pts = mpInputPins[i]->mInputSatrtPTS - first_stream_start_pts;
                    diff_end_pts = mpInputPins[i]->mInputEndPTS - first_stream_end_pts;
                }

                diff_input_duration = mpInputPins[i]->mInputEndPTS - mpInputPins[i]->mInputSatrtPTS + mpInputPins[i]->mDuration - first_input_duration;
                diff_duration = mpInputPins[i]->mCurrentPTS - mpInputPins[i]->mSessionPTSStartPoint + mpInputPins[i]->mDuration - first_stream_duration;
                LOGM_INFO("[file info]: avsync, stream(%d) differ with first stream: start input pts's diff %lld, end input pts's diff %lld, duration's diff %lld, input duration diff %lld.\n", i, diff_start_pts, diff_end_pts, diff_duration, diff_input_duration);
            }
        }
    }

    //file duration
    mMuxerFileInfo.file_duration = 0;
    for (i = 0; i < mnInputPinsNumber; i++) {
        if (mpInputPins[i]) {
            if ((StreamType_Video == mpInputPins[i]->mType) || (StreamType_Audio == mpInputPins[i]->mType)) {
                mMuxerFileInfo.file_duration += mpInputPins[i]->mCurrentPTS - mpInputPins[i]->mSessionPTSStartPoint + mpInputPins[i]->mDuration;
                LOGM_INFO("[file info]: pin(%d)'s duration %llu.\n", i, mpInputPins[i]->mCurrentPTS - mpInputPins[i]->mSessionPTSStartPoint + mpInputPins[i]->mDuration);
                tot_validpins ++;
            }
        }
    }

    if (tot_validpins) {
        mMuxerFileInfo.file_duration /= tot_validpins;
    }
    LOGM_INFO("[file info]: avg duration is %llu, number of valid pins %d.\n", mMuxerFileInfo.file_duration, tot_validpins);

    //file size
    //mCurrentTotalFilesize;
    LOGM_INFO("[file info]: mCurrentTotalFilesize %llu.\n", mCurrentTotalFilesize);

    //file bitrate
    estimated_bitrate = 0;
    for (i = 0; i < mnInputPinsNumber; i++) {
        if (mpInputPins[i]) {
            if ((StreamType_Video == mpInputPins[i]->mType) || (StreamType_Audio == mpInputPins[i]->mType)) {
                estimated_bitrate += mpInputPins[i]->mBitrate;
                LOGM_INFO("[file info]: pin(%d)'s bitrate %u.\n", i, mpInputPins[i]->mBitrate);
            }
        }
    }

    //DASSERT(estimated_bitrate);
    DASSERT(mMuxerFileInfo.file_duration);
    mMuxerFileInfo.file_bitrate = 0;
    if (mMuxerFileInfo.file_duration) {
        mMuxerFileInfo.file_bitrate = (TUint)(((float)mCurrentTotalFilesize * 8 * TimeUnitDen_90khz) / ((float)mMuxerFileInfo.file_duration));
        //LOGM_INFO("((float)mCurrentTotalFilesize*8*TimeUnitDen_90khz) %f .\n", ((float)mCurrentTotalFilesize*8*TimeUnitDen_90khz));
        //LOGM_INFO("result %f.\n", (((float)mCurrentTotalFilesize*8*TimeUnitDen_90khz)/((float)mFileDuration)));
        //LOGM_INFO("mFileBitrate %d.\n", mFileBitrate);
    }

    LOGM_INFO("[file info]: pre-estimated bitrate %u, calculated bitrate %u.\n", estimated_bitrate, mMuxerFileInfo.file_bitrate);
}

TU8 CScheduledMuxer::processCmd(SCMD &cmd)
{
    DASSERT(mpCmdQueue);
    LOGM_INFO("cmd.code %d, state %d.\n", cmd.code, msState);

    switch (cmd.code) {
        case ECMDType_ExitRunning:
            mbRun = 0;
            LOGM_INFO("ECMDType_ExitRunning cmd.\n");
            flushInputData();
            finalizeFile();
            msState = EModuleState_Halt;
            mpCmdQueue->Reply(EECode_OK);
            break;

        case ECMDType_Stop:
            flushInputData();
            finalizeFile();
            msState = EModuleState_WaitCmd;
            mpCmdQueue->Reply(EECode_OK);
            break;

        case ECMDType_Start:
            if (EModuleState_WaitCmd == msState) {
                msState = EModuleState_Muxer_WaitExtraData;
            }
            mpCmdQueue->Reply(EECode_OK);
            break;

        case ECMDType_Pause:
            //todo
            DASSERT(!mbPaused);
            mbPaused = 1;
            //mpCmdQueue->Reply(EECode_OK);
            break;

        case ECMDType_Resume:
            //todo
            if (msState == EModuleState_Pending) {
                msState = EModuleState_Idle;
            }
            mbPaused = 0;
            //mpCmdQueue->Reply(EECode_OK);
            break;

        case ECMDType_Flush:
            //should not comes here
            DASSERT(0);
            msState = EModuleState_Pending;
            if (mpBuffer) {
                mpBuffer->Release();
                mpBuffer = NULL;
            }
            mpCmdQueue->Reply(EECode_OK);
            break;

        case ECMDType_DeleteFile:
            LOGM_INFO("recieve cmd, delete file here, mTotalFileNumber %d, mFirstFileIndexCanbeDeleted %d.\n", mTotalFileNumber, mFirstFileIndexCanbeDeleted);
            deletePartialFile(mFirstFileIndexCanbeDeleted);
            mTotalFileNumber --;
            mFirstFileIndexCanbeDeleted ++;
            break;

        case ECMDType_UpdateUrl: {
                EECode ret = EECode_OK;

                if (cmd.pExtra) {
                    EECode ret = setOutputFile((TChar *)cmd.pExtra);
                    DASSERT_OK(ret);

                    DASSERT(mpOutputFileNameBase);
                    analyseFileNameFormat(mpOutputFileNameBase);

                    if (mpBuffer) {
                        mpBuffer->Release();
                        mpBuffer = NULL;
                    }

                    if (EMuxerModuleID_FFMpeg == mCurMuxerID) {
                        initialize((TChar *)"FFMpeg");
                    } else if (EMuxerModuleID_PrivateTS == mCurMuxerID) {
                        initialize((TChar *)"TS");
                    } else {
                        LOGM_FATAL("no such type %d\n", mCurMuxerID);
                    }
                    LOGM_NOTICE("set filename(%s) here\n", (TChar *)cmd.pExtra);
                    if (EModuleState_WaitCmd == msState) {
                        TUint index = 0;
                        for (index = 0; index < EConstMaxDemuxerMuxerStreamNumber; index ++) {
                            if (mpInputPins[index]) {
                                mpInputPins[index]->Enable(1);
                            }
                        }
                        msState = EModuleState_Idle;
                    }
                } else {
                    if (mpOutputFileName && strlen(mpOutputFileName)) {
                        TUint index = 0;
                        for (index = 0; index < EConstMaxDemuxerMuxerStreamNumber; index ++) {
                            if (mpInputPins[index]) {
                                mpInputPins[index]->Purge(1);
                            }
                        }
                        finalize();
                    }
                    msState = EModuleState_WaitCmd;
                }
                mpCmdQueue->Reply(ret);
            }
            break;

        case ECMDType_UpdateRecSavingStrategy: {
                SRecSavingStrategy *p_rec_strategy = (SRecSavingStrategy *)cmd.pExtra;
                if (p_rec_strategy) {
                    mSavingFileStrategy = p_rec_strategy->strategy;
                    mAutoFileNaming = p_rec_strategy->naming;
                    if (MuxerSavingCondition_InputPTS == p_rec_strategy->condition
                            || MuxerSavingCondition_CalculatedPTS == p_rec_strategy->condition) {
                        mAutoSavingTimeDuration = p_rec_strategy->param;
                    } else if (MuxerSavingCondition_FrameCount == p_rec_strategy->condition) {
                        mAutoSaveFrameCount = p_rec_strategy->param;
                    } else {
                        mpCmdQueue->Reply(EECode_BadParam);
                        break;
                    }
                    mSavingCondition = p_rec_strategy->condition;
                    mpCmdQueue->Reply(EECode_OK);
                } else {
                    mpCmdQueue->Reply(EECode_BadParam);
                }
            }
            break;

        default:
            LOGM_FATAL("wrong cmd %d.\n", cmd.code);
    }

    return 0;
}

EECode CScheduledMuxer::Scheduling(TUint times, TU32 inout_mask)
{
    EECode err;
    SCMD cmd;
    CIBuffer *tmp_buffer = NULL;
    TInt process_cmd_count = 0;
    TUint i = 0, index = 0;

    DASSERT(mpMuxer);
    DASSERT(mpEngineMsgSink);
    DASSERT(mpPersistMediaConfig);
    DASSERT(mpInputPins[0]);

    DASSERT(mbMuxerContentSetup);
    //DASSERT(!mbMuxerStarted);

    //while (times --) {

    LOGM_STATE("Scheduling(%d): start switch, msState=%d, %s, mpInputPins[0]->mpBufferQ->GetDataCnt() %d.\n", times, msState, gfGetModuleStateString(msState), mpInputPins[0]->mpBufferQ->GetDataCnt());
    mDebugHeartBeat ++;

    switch (msState) {

        case EModuleState_Halt:
            return EECode_NotRunning;
            break;

        case EModuleState_Muxer_WaitExtraData:

            while (mpCmdQueue->PeekMsg(&cmd, sizeof(cmd))) {
                processCmd(cmd);
                process_cmd_count ++;
            }
            if (process_cmd_count) {
                process_cmd_count = 0;
                break;
            }

            LOGM_NOTICE("[DEBUG]: begin, mInputIndex %u, mnInputPinsNumber %u\n", mInputIndex, mnInputPinsNumber);

            if (mInputIndex < mnInputPinsNumber) {
                DASSERT(mpInputPins[mInputIndex]);
                if (mpInputPins[mInputIndex]->mbExtraDataComes) {
                    mInputIndex ++;
                    break;
                }
                LOGM_PRINTF("[DEBUG]: %u, buffers %p\n", mInputIndex, mpInputPins[mInputIndex]->GetBufferQ());

                if (mpInputPins[mInputIndex]->GetDataCnt()) {

                    if (readInputData(mpInputPins[mInputIndex], mBufferType)) {
                        DASSERT(mInputIndex == mpInputPins[mInputIndex]->mStreamIndex);
                        //LOGM_INFO("mSavingFileStrategy %d, mpPin %p, mpMasterInput %p, mpBuffer->GetBufferLinearPTS() %llu, mNextFileTimeThreshold %llu.\n", mSavingFileStrategy, mpPin, mpMasterInput, mpBuffer->GetBufferLinearPTS(), mNextFileTimeThreshold);

                        if (_isMuxerInputData(mBufferType) && mpBuffer) {
                            LOGM_ERROR("first buffer not with extra data, must have problems, %s[inputPin %d] will be disabled\n", (EBufferType_VideoExtraData == mBufferType) ? "video" : ((EBufferType_AudioExtraData == mBufferType) ? "audio" : "other"), mInputIndex);
                            mpInputPins[mInputIndex]->Enable(0);
                        } else if (_isMuxerExtraData(mBufferType) && mpBuffer) {
                            err = saveExtraData(mpInputPins[mInputIndex]->mType, mpInputPins[mInputIndex]->mStreamIndex, (void *)mpBuffer->GetDataPtr(), mpBuffer->GetDataSize());
                            DASSERT_OK(err);
                            err = saveStreamInfo(mpInputPins[mInputIndex]->mType, mInputIndex, mpBuffer);
                            DASSERT_OK(err);
                            mpInputPins[mInputIndex]->mbExtraDataComes = 1;
                            LOGM_NOTICE("%s[inputPin %d] extra data comes.\n", (EBufferType_VideoExtraData == mBufferType) ? "video" : ((EBufferType_AudioExtraData == mBufferType) ? "audio" : "other"), mInputIndex);
                        } else if (_isFlowControlBuffer(mBufferType) && mpBuffer) {
                            LOGM_INFO("ignore them\n");
                        } else {
                            LOGM_FATAL("Why comes here, BAD mBufferType %d\n", mBufferType);
                            msState = EModuleState_Error;
                            break;
                        }

                        mpBuffer->Release();
                        mpBuffer = NULL;
                    }
                } else {
                    return EECode_NotRunning;
                }
            } else {

                if (mpOutputFileNameBase && strlen(mpOutputFileNameBase)) {
                    LOGM_INFO("all streams recieved extra data, start muxing\n");
                    setExtraData();
                    updateFileName(mCurrentFileIndex);
                    initializeFile(mpOutputFileName, mRequestContainerType);
                    msState = EModuleState_Idle;
                } else {
                    //disable all pins, wait set file name
                    LOGM_INFO("disable all inputpins, wait set filename\n");
                    TUint index = 0;
                    for (index = 0; index < EConstMaxDemuxerMuxerStreamNumber; index ++) {
                        if (mpInputPins[index]) {
                            mpInputPins[index]->Purge(1);
                        }
                    }
                    msState = EModuleState_WaitCmd;
                }
            }
            break;

        case EModuleState_WaitCmd:
            while (mpCmdQueue->PeekMsg(&cmd, sizeof(cmd))) {
                processCmd(cmd);
                process_cmd_count ++;
            }
            if (!process_cmd_count) {
                return EECode_NotRunning;
            }
            break;

        case EModuleState_Idle:

            while (mpCmdQueue->PeekMsg(&cmd, sizeof(cmd))) {
                processCmd(cmd);
                process_cmd_count ++;
            }
            if (process_cmd_count) {
                process_cmd_count = 0;
                break;
            }

            if (mbPaused) {
                msState = EModuleState_Pending;
                break;
            }

            //TODO: switch preferential input pin for feed video/audio data to mux module equably, need optimized
            mPreferentialPinIndex++;
            mPreferentialPinIndex = mPreferentialPinIndex % mnInputPinsNumber;
            for (i = 0; i < mnInputPinsNumber; i ++) {
                index = (i + mPreferentialPinIndex) % mnInputPinsNumber; //Circularly operate all input pins
                mpPin = mpInputPins[index];
                if (!mpPin || !mpPin->mpBufferQ->GetDataCnt()) {
                    continue;
                }

                if (readInputData(mpPin, mBufferType)) {
                    LOGM_DEBUG("mSavingFileStrategy %d, mpPin %p, mpMasterInput %p, mpBuffer->GetBufferLinearPTS() %llu, mNextFileTimeThreshold %llu.\n", mSavingFileStrategy, mpPin, mpMasterInput, mpBuffer->GetBufferLinearPTS(), mNextFileTimeThreshold);
                    if (_isMuxerInputData(mBufferType) && mpBuffer) {

                        //detect auto saving condition
                        if (MuxerSavingFileStrategy_AutoSeparateFile == mSavingFileStrategy) {
                            if (1 == isCommingBufferAutoFileBoundary(mpPin, mpBuffer)) {
                                if (!mpPersistMediaConfig->cutfile_with_precise_pts) {
                                    DASSERT(NULL == mpPin->mpCachedBuffer);
                                    DASSERT(mpMasterInput == mpPin);
                                    if (mpMasterInput == mpPin) {
                                        DASSERT(EPredefinedPictureType_IDR == mpBuffer->mVideoFrameType);
                                        mNewTimeThreshold = mpBuffer->GetBufferLinearPTS();
                                        LOGM_INFO("[Muxer], detected AUTO saving file, new video IDR threshold %llu, mNextFileTimeThreshold %llu, diff (%lld).\n", mNewTimeThreshold, mNextFileTimeThreshold, mNewTimeThreshold - mNextFileTimeThreshold);
                                        mpPin->mpCachedBuffer = mpBuffer;
                                        mpBuffer = NULL;
                                        mpPin->mbAutoBoundaryReached = 1;
                                        msState = EModuleState_Muxer_SavingPartialFile;
                                    } else {
                                        LOGM_ERROR("[Muxer], detected AUTO saving file, wrong mpPersistMediaConfig->cutfile_with_precise_pts setting %d.\n", mpPersistMediaConfig->cutfile_with_precise_pts);
                                        writeData(mpBuffer, mpPin);
                                        mpBuffer->Release();
                                        mpBuffer = NULL;
                                        msState = EModuleState_Muxer_SavingPartialFilePeekRemainingOnMasterPin;
                                        mpPin->mbAutoBoundaryReached = 1;
                                    }
                                } else {
                                    DASSERT(NULL == mpPin->mpCachedBuffer);
                                    if (mpMasterInput == mpPin) {
                                        DASSERT(EPredefinedPictureType_IDR == mpBuffer->mVideoFrameType);
                                        mNewTimeThreshold = mpBuffer->GetBufferLinearPTS();
                                        LOGM_INFO("[Muxer], detected AUTO saving file, new video IDR threshold %llu, mNextFileTimeThreshold %llu, diff (%lld).\n", mNewTimeThreshold, mNextFileTimeThreshold, mNewTimeThreshold - mNextFileTimeThreshold);
                                        mpPin->mpCachedBuffer = mpBuffer;
                                        mpBuffer = NULL;
                                        mpPin->mbAutoBoundaryReached = 1;
                                        msState = EModuleState_Muxer_SavingPartialFilePeekRemainingOnNonMasterPin;
                                    } else {
                                        mNewTimeThreshold = mNextFileTimeThreshold;
                                        LOGM_INFO("[Muxer], detected AUTO saving file, non-master pin detect boundary, pts %llu, mAudioNextFileTimeThreshold %llu, diff %lld, write buffer here.\n", mpBuffer->GetBufferLinearPTS(), mAudioNextFileTimeThreshold, mpBuffer->GetBufferLinearPTS() - mAudioNextFileTimeThreshold);
                                        writeData(mpBuffer, mpPin);
                                        mpBuffer->Release();
                                        mpBuffer = NULL;
                                        msState = EModuleState_Muxer_SavingPartialFilePeekRemainingOnMasterPin;
                                        mpPin->mbAutoBoundaryReached = 1;
                                    }
                                }
                                break;
                            }
                        } else if (mPresetCheckFlag) {
                            //detect preset condition is reached
                            if (1 == isReachPresetConditions(mpPin)) {
                                msState = EModuleState_Muxer_SavingWholeFile;
                                LOGM_INFO("[Muxer], detected Reach Preset condition, saving file, mCurrentTotalFrameCnt %llu, mCurrentTotalFilesize %llu, mPresetMaxFilesize %llu, mPresetCheckFlag 0x%x.\n", mpPin->mTotalFrameCount, mCurrentTotalFilesize, mPresetMaxFilesize, mPresetCheckFlag);
                                break;
                            }
                        }

                        DASSERT(mpBuffer);
                        LOGM_DEBUG("ready to write data, type %d.\n", mpPin->mType);
                        if (mpPin->mpCachedBuffer) {
                            writeData(mpPin->mpCachedBuffer, mpPin);
                            mpPin->mpCachedBuffer->Release();
                            mpPin->mpCachedBuffer = NULL;
                        }
                        writeData(mpBuffer, mpPin);
                        mpPin = NULL;
                        mpBuffer->Release();
                        mpBuffer = NULL;
                    } else if (_isMuxerExtraData(mBufferType)) {
                        DASSERT(mpBuffer);
                        //LOGM_INFO("extra data comes again, handle it!\n");
                        if (mpBuffer) {
                            if (EBufferType_VideoExtraData == mBufferType) {
                                DASSERT(mpMasterInput);
                                DASSERT(mpPin->mType == StreamType_Video);
                                DASSERT(NULL == mpPin->mpCachedBuffer);

                                //LOGM_INFO("extra data(video) comes again\n");
                                mpBuffer->Release();
                                mpBuffer = NULL;
                                break;
                            } else if (EBufferType_AudioExtraData == mBufferType) {
                                DASSERT(mpMasterInput);
                                DASSERT(mpPin->mType == StreamType_Audio);
                                DASSERT(NULL == mpPin->mpCachedBuffer);

                                //LOGM_INFO("extra data(audio) comes again\n");
                                mpBuffer->Release();
                                mpBuffer = NULL;
                                break;
                            } else {
                                mpBuffer->Release();
                                mpBuffer = NULL;
                            }
                        }
                    } else if (_isFlowControlBuffer(mBufferType)) {
                        DASSERT(mpBuffer);
                        if (mpBuffer) {
                            if (EBufferType_FlowControl_FinalizeFile == mBufferType) {
                                DASSERT(mpMasterInput);
                                DASSERT(mpPin->mType == StreamType_Video);
                                DASSERT(NULL == mpPin->mpCachedBuffer);
                                //mpMasterInput = mpPin;
                                mpPin->mbAutoBoundaryReached = 1;
                                mNextFileTimeThreshold = mpBuffer->GetBufferLinearPTS();
                                mNewTimeThreshold = mNextFileTimeThreshold;
                                //
                                mAudioNextFileTimeThreshold = mNextFileTimeThreshold;
                                LOGM_INFO("[Muxer] manually saving file buffer comes, PTS %llu.\n", mNewTimeThreshold);
                                mpBuffer->Release();
                                mpBuffer = NULL;
                                msState = EModuleState_Muxer_SavingPartialFile;
                                break;
                            } else if (EBufferType_FlowControl_EOS == mBufferType) {
                                LOGM_INFO(" [Muxer] FlowControl EOS buffer comes.\n");
                                mpBuffer->Release();
                                mpBuffer = NULL;
                                if (allInputEos()) {
                                    finalizeFile();
                                    postMsg(EMSGType_RecordingEOS);
                                    msState = EModuleState_Completed;
                                    break;
                                }
                            } else {
                                mpBuffer->Release();
                                mpBuffer = NULL;
                            }
                        }
                    } else {
                        if (!_isMuxerInputData(mBufferType)) {
                            LOGM_ERROR("BAD buffer type, %d.\n", mBufferType);
                        }
                        DASSERT(0);
                        if (mpBuffer) {
                            mpBuffer->Release();
                            mpBuffer = NULL;
                        }
                    }

                } else if (mpPin->mbSkiped != 1) {
                    //peek buffer fail. must not comes here
                    LOGM_ERROR("peek buffer fail. must not comes here.\n");
                    msState = EModuleState_Error;
                }
            }
            break;

        case EModuleState_Muxer_FlushExpiredFrame: {
                while (mpCmdQueue->PeekMsg(&cmd, sizeof(cmd))) {
                    processCmd(cmd);
                    process_cmd_count ++;
                }
                if (process_cmd_count) {
                    process_cmd_count = 0;
                    break;
                }

                TTime cur_time = 0;
                //TUint find_idr = 0;
                DASSERT(mpPin);
                DASSERT(!mpBuffer);
                if (mpBuffer) {
                    mpBuffer->Release();
                    mpBuffer = NULL;
                }
                if (mpPin->mpCachedBuffer) {
                    mpPin->mpCachedBuffer->Release();
                    mpPin->mpCachedBuffer = NULL;
                }
                //only video have this case
                DASSERT(StreamType_Video == mpPin->mType);
                if (mpClockReference) {
                    cur_time = mpClockReference->GetCurrentTime();
                }

                //discard data to next IDR
                while (1 == readInputData(mpPin, mBufferType)) {
                    if (_isMuxerInputData(mBufferType)) {
                        //LOGM_WARN("Discard frame, expired time %llu, cur time %llu, frame type %d, Seq num[%d, %d].\n", mpBuffer->mExpireTime, cur_time, mpBuffer->mVideoFrameType, mpBuffer->mOriSeqNum, mpBuffer->mSeqNum);
                        if (EPredefinedPictureType_IDR != mpBuffer->mVideoFrameType) {
                            mpBuffer->Release();
                            mpBuffer = NULL;
                        } else {
                            if (mpBuffer->mExpireTime > cur_time) {
                                msState = EModuleState_Idle;//continue write data
                                LOGM_INFO("Get valid IDR... cur time %llu, start time %llu, expird time %llu.\n", mpClockReference->GetCurrentTime(), cur_time, mpBuffer->mExpireTime);
                                break;
                            } else {
                                mpBuffer->Release();
                                mpBuffer = NULL;
                            }
                        }
                    } else if (_isFlowControlBuffer(mBufferType)) {
                        DASSERT(mpBuffer);
                        if (mpBuffer) {
                            if (EBufferType_FlowControl_FinalizeFile == mBufferType) {
                                DASSERT(mpMasterInput);
                                DASSERT(mpPin->mType == StreamType_Video);
                                DASSERT(NULL == mpPin->mpCachedBuffer);
                                //mpMasterInput = mpPin;
                                mpPin->mbAutoBoundaryReached = 1;
                                msState = EModuleState_Muxer_SavingPartialFile;
                                mNextFileTimeThreshold = mpBuffer->GetBufferLinearPTS();
                                mNewTimeThreshold = mNextFileTimeThreshold;
                                LOGM_INFO("[Muxer] manually saving file buffer comes, PTS %llu.\n", mNewTimeThreshold);
                                mpBuffer->Release();
                                mpBuffer = NULL;
                                break;
                            } else if (EBufferType_FlowControl_EOS == mBufferType) {
                                LOGM_INFO(" [Muxer] FlowControl EOS buffer comes.\n");
                                mpBuffer->Release();
                                mpBuffer = NULL;
                                if (allInputEos()) {
                                    finalizeFile();
                                    postMsg(EMSGType_RecordingEOS);
                                    msState = EModuleState_Completed;
                                    break;
                                }
                            } else {
                                mpBuffer->Release();
                                mpBuffer = NULL;
                            }
                        }
                        break;
                    } else {
                        DASSERT(0);
                        if (mpBuffer) {
                            mpBuffer->Release();
                            mpBuffer = NULL;
                        }
                        break;
                    }
                }

                //state not changed
                if (EModuleState_Muxer_FlushExpiredFrame == msState) {
                    LOGM_WARN("Flush all data in input pin, no valid IDR... cur time %llu, start time %llu.\n", mpClockReference->GetCurrentTime(), cur_time);
                    msState = EModuleState_Idle;//clear all video buffers now
                }
            }
            break;

            //if direct from EModuleState_Idle
            //use robust but not very strict PTS strategy, target: cost least time, not BLOCK some real time task, like streamming out, capture data from driver, etc
            //report PTS gap, if gap too large, post warnning msg
            //need stop recording till IDR comes, audio's need try its best to match video's PTS

            //if from previous STATE_SAVING_PARTIAL_FILE_XXXX, av sync related pin should with more precise PTS boundary, but would be with more latency
        case EModuleState_Muxer_SavingPartialFile:

            while (mpCmdQueue->PeekMsg(&cmd, sizeof(cmd))) {
                processCmd(cmd);
                process_cmd_count ++;
            }
            if (process_cmd_count) {
                process_cmd_count = 0;
                break;
            }

            DASSERT(mNewTimeThreshold >= mNextFileTimeThreshold);

            //write remainning packet
            LOGM_INFO("[muxer %p] start saving patial file, peek all packet if PTS less than threshold.\n", this);
            for (mInputIndex = 0; mInputIndex < mnInputPinsNumber; mInputIndex++) {
                if (mpMasterInput == mpInputPins[mInputIndex] || mpInputPins[mInputIndex]->mbAutoBoundaryReached) {
                    continue;
                }

                if (mpInputPins[mInputIndex]->mpCachedBuffer) {
                    writeData(mpInputPins[mInputIndex]->mpCachedBuffer, mpInputPins[mInputIndex]);
                    mpInputPins[mInputIndex]->mpCachedBuffer = NULL;
                }

                tmp_buffer = NULL;
                while (1 == mpInputPins[mInputIndex]->PeekBuffer(tmp_buffer)) {
                    DASSERT(tmp_buffer);
                    LOGM_INFO("[muxer %p, inpin %d] peek packet, PTS %llu, PTS threshold %llu.\n", this, mInputIndex, tmp_buffer->GetBufferLinearPTS(), mNewTimeThreshold);

                    writeData(tmp_buffer, mpInputPins[mInputIndex]);

                    if (tmp_buffer->GetBufferLinearPTS() > mNewTimeThreshold) {
                        tmp_buffer->Release();
                        tmp_buffer = NULL;
                        break;
                    }
                    tmp_buffer->Release();
                    tmp_buffer = NULL;
                }

            }

            LOGM_INFO("[muxer %p] finalize current file, %s.\n", this, mpOutputFileName);

            err = finalizeFile();

            if (err != EECode_OK) {
                LOGM_FATAL("CScheduledMuxer::finalizeFile fail, enter error state.\n");
                msState = EModuleState_Error;
                break;
            }
            if (mMaxTotalFileNumber) {
                if (mTotalFileNumber >= mMaxTotalFileNumber) {
                    deletePartialFile(mFirstFileIndexCanbeDeleted);
                    mFirstFileIndexCanbeDeleted ++;
                    mTotalFileNumber --;
                }
            } else {
                //do nothing
            }

            //corner case, little chance to comes here
            if (allInputEos()) {
                LOGM_WARN(" allInputEos() here, should not comes here, please check!!!!.\n");
                //clear all cached buffer
                for (mInputIndex = 0; mInputIndex < mnInputPinsNumber; mInputIndex++) {
                    if (mpInputPins[mInputIndex]) {
                        if (mpInputPins[mInputIndex]->mpCachedBuffer) {
                            mpInputPins[mInputIndex]->mpCachedBuffer->Release();
                            mpInputPins[mInputIndex]->mpCachedBuffer = NULL;
                        }
                    }
                }

                LOGM_INFO("EOS buffer comes in state %d, enter pending.\n", msState);
                postMsg(EMSGType_RecordingEOS);
                msState = EModuleState_Completed;
                break;
            }

            if (mTotalRecFileNumber > 0 && mTotalFileNumber >= mTotalRecFileNumber) {
                LOGM_INFO("the num of rec file(%d) has reached the limit(%d), so stop recording\n", mTotalFileNumber, mTotalRecFileNumber);
                postMsg(EMSGType_RecordingReachPresetTotalFileNumbers);
                msState = EModuleState_Completed;
                break;
            }

            mNextFileTimeThreshold += mAutoSavingTimeDuration;
            if (mNextFileTimeThreshold < mNewTimeThreshold) {
                LOGM_WARN("mNextFileTimeThreshold(%llu) + mAutoSavingTimeDuration(%llu) still less than mNewTimeThreshold(%llu).\n", mNextFileTimeThreshold, mAutoSavingTimeDuration, mNewTimeThreshold);
                mNextFileTimeThreshold += mAutoSavingTimeDuration;
            }
            //DASSERT(mIDRFrameInterval);
            if (mIDRFrameInterval) {
                mThreshDiff = mNextFileTimeThreshold % mIDRFrameInterval;
            } else {
                mThreshDiff = 0;
            }
            if (mThreshDiff == 0) {
                mAudioNextFileTimeThreshold = mNextFileTimeThreshold - mVideoFirstPTS;
            } else {
                mAudioNextFileTimeThreshold = mNextFileTimeThreshold - mVideoFirstPTS - mThreshDiff + mIDRFrameInterval;
            }

            ++mCurrentFileIndex;
            updateFileName(mCurrentFileIndex);
            LOGM_INFO("[muxer %p] initialize new file %s.\n", this, mpOutputFileName);

            //generate thumbnail file here
            if (mpPersistMediaConfig->encoding_mode_config.thumbnail_enabled && mpThumbNailFileName) {
                postMsg(EMSGType_NotifyThumbnailFileGenerated);
            }

            err = initializeFile(mpOutputFileName, mRequestContainerType);
            DASSERT(err == EECode_OK);
            if (err != EECode_OK) {
                LOGM_FATAL("CScheduledMuxer::initializeFile fail, enter error state.\n");
                msState = EModuleState_Error;
                break;
            }
            LOGM_INFO("[muxer %p] end saving patial file.\n", this);
            msState = EModuleState_Idle;
            break;

            //with more precise pts, but with much latency for write new file/streaming
        case EModuleState_Muxer_SavingPartialFilePeekRemainingOnMasterPin:
            while (mpCmdQueue->PeekMsg(&cmd, sizeof(cmd))) {
                processCmd(cmd);
                process_cmd_count ++;
            }
            if (process_cmd_count) {
                process_cmd_count = 0;
                break;
            }

            //wait master pin's gap(video IDR)
            DASSERT(mNewTimeThreshold >= mNextFileTimeThreshold);
            DASSERT(mpPin != mpMasterInput);
            DASSERT(0 == mpMasterInput->mbAutoBoundaryReached);

            LOGM_INFO("[avsync]: (%p) PEEK buffers in master pin start.\n", this);

            if (mpPin != mpMasterInput) {
                //get master's time threshold
                //peek master input(video) first, refresh the threshold, the duration maybe not very correct because h264's IDR time interval maybe large (related to M/N/IDR interval settings)
                DASSERT(StreamType_Video != mpPin->mType);
                DASSERT(StreamType_Video == mpMasterInput->mType);
                while (1 == mpMasterInput->PeekBuffer(tmp_buffer)) {
                    DASSERT(tmp_buffer);
                    if (_isMuxerInputData(tmp_buffer->GetBufferType())) {
                        LOGM_INFO("[master inpin] peek packet, PTS %llu, PTS threshold %llu.\n", tmp_buffer->GetBufferLinearPTS(), mNextFileTimeThreshold);
                        if ((tmp_buffer->GetBufferLinearPTS() >  mNextFileTimeThreshold) && (EPredefinedPictureType_IDR == tmp_buffer->mVideoFrameType)) {
                            DASSERT(!mpMasterInput->mpCachedBuffer);
                            if (mpMasterInput->mpCachedBuffer) {
                                LOGM_WARN("why comes here, release cached buffer, pts %llu, type %d.\n", mpMasterInput->mpCachedBuffer->GetBufferLinearPTS(), mpMasterInput->mpCachedBuffer->mVideoFrameType);
                                mpMasterInput->mpCachedBuffer->Release();
                            }
                            mpMasterInput->mpCachedBuffer = tmp_buffer;
                            if (!_isTmeStampNear(mNewTimeThreshold, tmp_buffer->GetBufferLinearPTS(), TimeUnitNum_fps29dot97)) {
                                //refresh mpPin's threshold
                                if ((StreamType)mpPin->mType != StreamType_Audio) {
                                    mpPin->mbAutoBoundaryReached = 0;
                                }
                            }
                            mNewTimeThreshold = tmp_buffer->GetBufferLinearPTS();
                            LOGM_INFO("get IDR boundary, pts %llu, mNextFileTimeThreshold %llu, diff %lld.\n", mNewTimeThreshold, mNextFileTimeThreshold, mNewTimeThreshold - mNextFileTimeThreshold);
                            mpMasterInput->mbAutoBoundaryReached = 1;
                            tmp_buffer = NULL;
                            break;
                        }
                        writeData(tmp_buffer, mpMasterInput);
                    } else {
                        if (EBufferType_FlowControl_EOS == tmp_buffer->GetBufferType()) {
                            mpMasterInput->mEOSComes = 1;
                            tmp_buffer->Release();
                            tmp_buffer = NULL;
                            break;
                        } else {
                            //ignore
                        }
                    }
                    tmp_buffer->Release();
                    tmp_buffer = NULL;
                }

                if (1 == mpMasterInput->mbAutoBoundaryReached) {
                    LOGM_INFO("[avsync]: PEEK buffers in master pin done, go to non-master pin.\n");
                    msState = EModuleState_Muxer_SavingPartialFilePeekRemainingOnNonMasterPin;
                } else {
                    LOGM_INFO("[avsync]: PEEK buffers in master pin not finished, go to wait master boundary(IDR).\n");
                    msState = EModuleState_Muxer_SavingPartialFileWaitMasterPin;
                }

            } else {
                LOGM_ERROR("MUST not comes here, please check logic.\n");
                msState = EModuleState_Muxer_SavingPartialFile;
            }
            break;

        case EModuleState_Muxer_SavingPartialFileWaitMasterPin:
            LOGM_INFO("[avsync]: (%p) WAIT next IDR boundary start.\n", this);
            while (mpCmdQueue->PeekMsg(&cmd, sizeof(cmd))) {
                processCmd(cmd);
                process_cmd_count ++;
            }
            if (process_cmd_count) {
                process_cmd_count = 0;
                break;
            }
            DASSERT(0 == mpMasterInput->mbAutoBoundaryReached);
            DASSERT(!mpMasterInput->mpCachedBuffer);
            if (mpMasterInput->mpCachedBuffer) {
                LOGM_WARN("Already have get next IDR, goto next stage.\n");
                mpMasterInput->mbAutoBoundaryReached = 1;
                msState = EModuleState_Muxer_SavingPartialFilePeekRemainingOnNonMasterPin;
                break;
            }

            if (0 == mpMasterInput->mbAutoBoundaryReached) {

                while (1 == mpMasterInput->PeekBuffer(tmp_buffer)) {
                    if (_isMuxerInputData(tmp_buffer->GetBufferType())) {
                        if (EPredefinedPictureType_IDR == tmp_buffer->mVideoFrameType && tmp_buffer->GetBufferLinearPTS() >= mNewTimeThreshold) {
                            mpMasterInput->mpCachedBuffer = tmp_buffer;
                            if (!_isTmeStampNear(mNewTimeThreshold, tmp_buffer->GetBufferLinearPTS(), TimeUnitNum_fps29dot97)) {
                                //refresh new time threshold, need recalculate non-masterpin
                                if ((StreamType)mpPin->mType != StreamType_Audio) {
                                    mpPin->mbAutoBoundaryReached = 0;
                                }
                            }
                            LOGM_INFO("[avsync]: wait new IDR, pts %llu, previous time threshold %llu, diff %lld.\n", tmp_buffer->GetBufferLinearPTS(), mNewTimeThreshold, tmp_buffer->GetBufferLinearPTS() - mNewTimeThreshold);
                            mNewTimeThreshold = tmp_buffer->GetBufferLinearPTS();
                            mpMasterInput->mbAutoBoundaryReached = 1;
                            msState = EModuleState_Muxer_SavingPartialFilePeekRemainingOnNonMasterPin;

                            tmp_buffer = NULL;
                            break;
                        }
                        writeData(tmp_buffer, mpMasterInput);
                        tmp_buffer->Release();
                        tmp_buffer = NULL;
                    } else if (EBufferType_FlowControl_EOS == tmp_buffer->GetBufferType()) {
                        LOGM_WARN("EOS comes here, how to handle it, msState %d.\n", msState);
                        mpMasterInput->mEOSComes = 1;
                        tmp_buffer->Release();
                        tmp_buffer = NULL;
                        mpMasterInput->mbAutoBoundaryReached = 1;
                        msState = EModuleState_Muxer_SavingPartialFilePeekRemainingOnNonMasterPin;
                    } else {
                        //ignore
                        LOGM_ERROR("not processed buffer type %d.\n", tmp_buffer->GetBufferType());
                        tmp_buffer->Release();
                        tmp_buffer = NULL;
                    }
                }
            }
            break;

        case EModuleState_Muxer_SavingPartialFilePeekRemainingOnNonMasterPin:
            //for non-master pin
            DASSERT(mNewTimeThreshold >= mNextFileTimeThreshold);
            while (mpCmdQueue->PeekMsg(&cmd, sizeof(cmd))) {
                processCmd(cmd);
                process_cmd_count ++;
            }
            if (process_cmd_count) {
                process_cmd_count = 0;
                break;
            }

            LOGM_INFO("[avsync]: PEEK non-master pin(%p) start.\n", this);
            if (!hasPinNeedReachBoundary(mInputIndex)) {
                msState = EModuleState_Muxer_SavingPartialFile;
                LOGM_INFO("[avsync]: all pin(need sync) reaches boundary.\n");
                break;
            }
            if (mInputIndex >= mnInputPinsNumber || !mpInputPins[mInputIndex]) {
                LOGM_ERROR("safe check fail, check code, mInputIndex %d, mnInputPinsNumber %d.\n", mInputIndex, mnInputPinsNumber);
                msState = EModuleState_Muxer_SavingPartialFile;
                break;
            }

            mpPin = mpInputPins[mInputIndex];
            DASSERT(mpPin);
            DASSERT(mpPin != mpMasterInput);
            DASSERT(StreamType_Video != mpPin->mType);
            DASSERT(!mpPin->mpCachedBuffer);
            if (mpPin->mpCachedBuffer) {
                LOGM_ERROR("why comes here, check code.\n");
                mpPin->mpCachedBuffer->Release();
                mpPin->mpCachedBuffer = NULL;
            }

            if (mpPin != mpMasterInput) {
                //peek non-master input
                DASSERT(StreamType_Video != mpPin->mType);
                while (1 == mpPin->PeekBuffer(tmp_buffer)) {
                    DASSERT(tmp_buffer);
                    if (_isMuxerInputData(tmp_buffer->GetBufferType())) {
                        LOGM_INFO("[non-master inpin] (%p) peek packet, PTS %llu, PTS threshold %llu.\n", this, tmp_buffer->GetBufferLinearPTS(), mNextFileTimeThreshold);

                        writeData(tmp_buffer, mpPin);

                        if (StreamType_Audio != mpPin->mType) {
                            if (tmp_buffer->GetBufferLinearPTS() >=  mNextFileTimeThreshold) {
                                LOGM_INFO("[avsync]: non-master pin (%p) get boundary, pts %llu, mNextFileTimeThreshold %llu, diff %lld.\n", this, mNewTimeThreshold, mNextFileTimeThreshold, mNewTimeThreshold - mNextFileTimeThreshold);
                                mpPin->mbAutoBoundaryReached = 1;
                                tmp_buffer->Release();
                                tmp_buffer = NULL;
                                break;
                            }
                        } else {//audio stream
                            if (tmp_buffer->GetBufferLinearPTS() >=  mAudioNextFileTimeThreshold) {
                                LOGM_INFO("[avsync]: audio input pin (%p) get boundary, pts %llu, mAudioNextFileTimeThreshold %llu, diff %lld.\n", this, mNewTimeThreshold, mAudioNextFileTimeThreshold, mNewTimeThreshold - mAudioNextFileTimeThreshold);
                                mpPin->mbAutoBoundaryReached = 1;
                                tmp_buffer->Release();
                                tmp_buffer = NULL;
                                break;
                            }
                        }
                        tmp_buffer->Release();
                        tmp_buffer = NULL;
                    } else {
                        if (EBufferType_FlowControl_EOS == tmp_buffer->GetBufferType()) {
                            mpPin->mEOSComes = 1;
                            tmp_buffer->Release();
                            tmp_buffer = NULL;
                            break;
                        } else {
                            //ignore
                        }
                        tmp_buffer->Release();
                        tmp_buffer = NULL;
                    }
                }

                if (0 == mpPin->mbAutoBoundaryReached) {
                    LOGM_INFO("[avsync]: non-master pin need wait.\n");
                    msState = EModuleState_Muxer_SavingPartialFileWaitNonMasterPin;
                }

            } else {
                LOGM_ERROR("MUST not comes here, please check logic.\n");
                msState = EModuleState_Muxer_SavingPartialFile;
            }
            break;

        case EModuleState_Muxer_SavingPartialFileWaitNonMasterPin:
            LOGM_INFO("[avsync]: (%p)WAIT till boundary(non-master pin).\n", this);
            while (mpCmdQueue->PeekMsg(&cmd, sizeof(cmd))) {
                processCmd(cmd);
                process_cmd_count ++;
            }
            if (process_cmd_count) {
                process_cmd_count = 0;
                break;
            }

            //debug assert
            DASSERT(mpPin);
            DASSERT(mpPin != mpMasterInput);
            DASSERT(StreamType_Video != mpPin->mType);
            DASSERT(!mpPin->mpCachedBuffer);

            DASSERT(0 == mpPin->mbAutoBoundaryReached);
            DASSERT(!mpPin->mpCachedBuffer);
            if (mpPin->mpCachedBuffer) {
                LOGM_WARN("[avsync]: Already have get cached buffer, goto next stage.\n");
                mpPin->mbAutoBoundaryReached = 1;
                msState = EModuleState_Muxer_SavingPartialFilePeekRemainingOnNonMasterPin;
                break;
            }

            if (0 == mpPin->mbAutoBoundaryReached) {
                while (1 == mpPin->PeekBuffer(tmp_buffer)) {
                    if (_isMuxerInputData(tmp_buffer->GetBufferType())) {
                        writeData(tmp_buffer, mpPin);

                        if (tmp_buffer->GetBufferLinearPTS() >= mNewTimeThreshold) {
                            LOGM_INFO("[avsync]: non-master pin(%p) wait new boundary, pts %llu, time threshold %llu, diff %lld.\n", this, tmp_buffer->GetBufferLinearPTS(), mNewTimeThreshold, tmp_buffer->GetBufferLinearPTS() - mNewTimeThreshold);
                            mpPin->mbAutoBoundaryReached = 1;
                            msState = EModuleState_Muxer_SavingPartialFilePeekRemainingOnNonMasterPin;
                            tmp_buffer->Release();
                            tmp_buffer = NULL;
                            break;
                        }
                        tmp_buffer->Release();
                        tmp_buffer = NULL;
                    } else if (_isFlowControlBuffer(tmp_buffer->GetBufferType())) {
                        LOGM_WARN("EOS comes here, how to handle it, msState %d.\n", msState);
                        mpPin->mEOSComes = 1;
                        tmp_buffer->Release();
                        tmp_buffer = NULL;
                        mpPin->mbAutoBoundaryReached = 1;
                        if (allInputEos()) {
                            finalizeFile();
                            postMsg(EMSGType_RecordingEOS);
                            msState = EModuleState_Completed;
                        } else {
                            msState = EModuleState_Muxer_SavingPartialFilePeekRemainingOnNonMasterPin;
                        }
                        break;
                    } else {
                        //ignore
                        LOGM_ERROR("not processed buffer type %d.\n", tmp_buffer->GetBufferType());
                        tmp_buffer->Release();
                        tmp_buffer = NULL;
                    }
                }

            }
            break;

        case EModuleState_Muxer_SavingWholeFile:
            while (mpCmdQueue->PeekMsg(&cmd, sizeof(cmd))) {
                processCmd(cmd);
                process_cmd_count ++;
            }
            if (process_cmd_count) {
                process_cmd_count = 0;
                break;
            }
            //write data
            DASSERT(mpBuffer);
            DASSERT(mpPin);
            DASSERT(mpPin->mType == StreamType_Video);

            LOGM_INFO("[muxer %p] EModuleState_Muxer_SavingWholeFile, write remainning packet start.\n", this);
            if (mpPin->mType == StreamType_Video) {
                if (mpPin->mpCachedBuffer) {
                    writeData(mpPin->mpCachedBuffer, mpPin);
                    mpPin->mpCachedBuffer->Release();
                    mpPin->mpCachedBuffer = NULL;
                }
                writeData(mpBuffer, mpPin);
            } else if (mpPin->mType == StreamType_Audio) {
                if (mpPin->mpCachedBuffer) {
                    DASSERT(0);//current code should not come to this case
                    LOGM_FATAL("current code should not come to this case.\n");
                    writeData(mpPin->mpCachedBuffer, mpPin);
                    mpPin->mpCachedBuffer->Release();
                    mpPin->mpCachedBuffer = NULL;
                }
                writeData(mpBuffer, mpPin);
            } else if (mpPin->mType == StreamType_Subtitle) {

            } else if (mpPin->mType == StreamType_PrivateData) {
                writeData(mpBuffer, mpPin);
            } else {
                DASSERT(0);
                LOGM_FATAL("must not comes here, BAD input type(%d)? pin %p, buffer %p.\n", mpPin->mType, mpPin, mpBuffer);
            }
            mpPin = NULL;

            mpBuffer->Release();
            mpBuffer = NULL;
            msState = EModuleState_Completed;

            //write remainning packet, todo

            //saving file
            LOGM_INFO("[muxer %p] EModuleState_Muxer_SavingWholeFile, write remainning packet end, finalizeFile() start.\n", this);
            finalizeFile();
            LOGM_INFO("[muxer %p] EModuleState_Muxer_SavingWholeFile, write remainning packet end, finalizeFile() done.\n", this);

            LOGM_INFO("Post MSG_INTERNAL_ENDING to engine.\n");
            if (mPresetReachType | (EPresetCheckFlag_videoframecount | EPresetCheckFlag_audioframecount)) {
                postMsg(EMSGType_RecordingReachPresetDuration);
            } else if (mPresetReachType == EPresetCheckFlag_filesize) {
                postMsg(EMSGType_RecordingReachPresetFilesize);
            } else {
                DASSERT(0);
            };
            LOGM_INFO("[muxer %p] EModuleState_Muxer_SavingWholeFile, MSG_INTERNAL_ENDING done.\n", this);

            break;

        case EModuleState_Pending:
        case EModuleState_Completed:
        case EModuleState_Bypass:
        case EModuleState_Error:
            while (mpCmdQueue->PeekMsg(&cmd, sizeof(cmd))) {
                processCmd(cmd);
            }
            break;

        default:
            LOGM_ERROR("CScheduledMuxer:: bad state 0x%x\n", (TUint)msState);
            msState = EModuleState_Error;
            break;
    }

    //for safe here
    //finalizeFile();

    //LOGM_DEBUG("Scheduling end.\n");

    return EECode_OK;
}

TInt CScheduledMuxer::IsPassiveMode() const
{
    return 1;
}

TSchedulingHandle CScheduledMuxer::GetWaitHandle() const
{
    return (-1);
}

TSchedulingUnit CScheduledMuxer::HungryScore() const
{
    TUint i = 0, data_number = 0, cmd_number = 0;

    for (i = 0; i < mnInputPinsNumber; i ++) {
        if (mpInputPins[i]) {
            data_number += mpInputPins[i]->mpBufferQ->GetDataCnt();
        }
    }

    if (mpCmdQueue) {
        cmd_number = mpCmdQueue->GetDataCnt();
    }

    if (data_number || cmd_number) {
        return 1;
    }

    return 0;
}

TU8 CScheduledMuxer::GetPriority() const
{
    return mPriority;
}

void CScheduledMuxer::Destroy()
{
    Delete();
}

EECode CScheduledMuxer::flushInputData()
{
    TUint index = 0;

    if (mpBuffer) {
        mpBuffer->Release();
        mpBuffer = NULL;
    }

    LOGM_INFO("before purge input pins\n");

    for (index = 0; index < EConstMaxDemuxerMuxerStreamNumber; index ++) {
        if (mpInputPins[index]) {
            mpInputPins[index]->Purge();
        }
    }

    LOGM_INFO("after purge input pins\n");

    return EECode_OK;
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

#endif

