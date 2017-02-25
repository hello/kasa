/*******************************************************************************
 * muxer_v2.cpp
 *
 * History:
 *    2014/08/25 - [Zhi He] create file
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

#include "muxer_v2.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

IFilter *gfCreateScheduledMuxerV2Filter(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TUint index)
{
    return CScheduledMuxerV2::Create(pName, pPersistMediaConfig, pEngineMsgSink, index);
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

IFilter *gfCreateScheduledMuxerFilterV2(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index)
{
    return CScheduledMuxerV2::Create(pName, pPersistMediaConfig, pEngineMsgSink, index);
}

CMuxerInputV2 *CMuxerInputV2::Create(const TChar *pName, IFilter *pFilter, CIQueue *pQueue, TUint type, TUint stream_index)
{
    CMuxerInputV2 *result = new CMuxerInputV2(pName, pFilter, type, stream_index);
    if (result && (result->Construct(pQueue) != EECode_OK)) {
        delete result;
        result = NULL;
    }
    return result;
}

EECode CMuxerInputV2::Construct(CIQueue *pQueue)
{
    LOGM_DEBUG("CMuxerInputV2::Construct.\n");
    DASSERT(mpFilter);
    EECode err = inherited::Construct(pQueue);
    return err;
}

void CMuxerInputV2::Delete()
{
    return inherited::Delete();
}

TU32 CMuxerInputV2::GetDataCnt() const
{
    return mpBufferQ->GetDataCnt();
}

TUint CMuxerInputV2::NeedDiscardPacket()
{
    return mDiscardPacket;
}

void CMuxerInputV2::DiscardPacket(TUint discard)
{
    mDiscardPacket = discard;
}

void CMuxerInputV2::PrintState()
{
    LOGM_NOTICE("CMuxerInputV2 state: mDiscardPacket %d, mType %d, mStreamIndex %d.\n", mDiscardPacket, mType, mStreamIndex);
}

void CMuxerInputV2::PrintTimeInfo()
{
    LOGM_NOTICE(" Time info: mStartPTS %lld, mCurrentPTS %lld, mCurrentDTS %lld.\n", mStartPTS, mCurrentPTS, mCurrentDTS);
    LOGM_NOTICE("      mCurrentFrameCount %llu, mTotalFrameCount %llu, mSession %u, mDuration%lld.\n", mCurrentFrameCount, mTotalFrameCount, mSession, mDuration);
}

void CMuxerInputV2::timeDiscontinue()
{
    mSession ++;
    mCurrentFrameCount = 0;
}

CMuxerInputV2::~CMuxerInputV2()
{
    LOGM_RESOURCE("CMuxerInputV2::~CMuxerInputV2.\n");
}

//-----------------------------------------------------------------------
//
// CScheduledMuxerV2
//
//-----------------------------------------------------------------------
IFilter *CScheduledMuxerV2::Create(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index)
{
    CScheduledMuxerV2 *result = new CScheduledMuxerV2(pName, pPersistMediaConfig, pEngineMsgSink, index);
    if (result && result->Construct() != EECode_OK) {
        delete result;
        result = NULL;
    }
    return result;
}

CScheduledMuxerV2::CScheduledMuxerV2(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index)
    : inherited(pName, index, pPersistMediaConfig, pEngineMsgSink)
    , mpMuxer(NULL)
    , mpCurrentInputPin(NULL)
    , mnInputPinsNumber(0)
    , mpScheduler(NULL)
    , mRequestContainerType(ContainerType_TS)
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
    , mbUseSourcePTS(1)
    , mbSuspend(0)
    , mpOutputFileName(NULL)
    , mOutputFileNameLength(0)
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
    , mOutputFileNameBaseLength(0)
    , mSavingFileStrategy(MuxerSavingFileStrategy_AutoSeparateFile)
    , mSavingCondition(MuxerSavingCondition_InputPTS)
    , mAutoFileNaming(MuxerAutoFileNaming_ByNumber)
    , mAutoSaveFrameCount(180 * 30)
    , mAutoSavingTimeDuration(180 * TimeUnitDen_90khz)
    , mNextFileTimeThreshold(0)
    , mbNextFileTimeThresholdSet(0)
    , mbPTSStartFromZero(1)
    , mbAutoName(0)
    , mbFileCreated(0)
    , mCurrentTotalFilesize(0)
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

EECode CScheduledMuxerV2::Construct()
{
    DSET_MODULE_LOG_CONFIG(ELogModuleMuxerFilter);

    mpScheduler = gfGetFileIOWriterScheduler((const volatile SPersistMediaConfig *)mpPersistMediaConfig, mIndex);
    DASSERT(mpScheduler);

    if (MuxerSavingFileStrategy_Invalid != mpPersistMediaConfig->auto_cut.record_strategy) {
        mSavingFileStrategy = (MuxerSavingFileStrategy) mpPersistMediaConfig->auto_cut.record_strategy;
    }

    if (MuxerSavingCondition_Invalid != mpPersistMediaConfig->auto_cut.record_autocut_condition) {
        mSavingCondition = (MuxerSavingCondition) mpPersistMediaConfig->auto_cut.record_autocut_condition;
    }

    if (MuxerAutoFileNaming_Invalid != mpPersistMediaConfig->auto_cut.record_autocut_naming) {
        mAutoFileNaming = (MuxerAutoFileNaming) mpPersistMediaConfig->auto_cut.record_autocut_naming;
    }

    if (mpPersistMediaConfig->auto_cut.record_autocut_framecount) {
        mAutoSaveFrameCount = mpPersistMediaConfig->auto_cut.record_autocut_framecount;
    }

    if (mpPersistMediaConfig->auto_cut.record_autocut_duration) {
        mAutoSavingTimeDuration = mpPersistMediaConfig->auto_cut.record_autocut_duration;
    }

    return inherited::Construct();
}

CScheduledMuxerV2::~CScheduledMuxerV2()
{
    TUint i = 0;

    LOGM_RESOURCE("CScheduledMuxerV2::~CScheduledMuxerV2(), before delete inputpins.\n");
    for (i = 0; i < EConstMaxDemuxerMuxerStreamNumber; i ++) {
        if (mpInputPins[i]) {
            mpInputPins[i]->Delete();
            mpInputPins[i] = NULL;
        }
    }

    LOGM_RESOURCE("CScheduledMuxerV2::~CScheduledMuxerV2(), before mpMuxer->Delete().\n");
    if (mpMuxer) {
        mpMuxer->Destroy();
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

    LOGM_RESOURCE("CScheduledMuxerV2::~CScheduledMuxerV2(), end.\n");
}

void CScheduledMuxerV2::Delete()
{
    TUint i = 0;

    LOGM_RESOURCE("CScheduledMuxerV2::Delete(), before delete inputpins.\n");
    for (i = 0; i < EConstMaxDemuxerMuxerStreamNumber; i ++) {
        if (mpInputPins[i]) {
            mpInputPins[i]->Delete();
            mpInputPins[i] = NULL;
        }
    }

    LOGM_RESOURCE("CScheduledMuxerV2::Delete(), before mpMuxer->Delete().\n");
    if (mpMuxer) {
        mpMuxer->Destroy();
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

    LOGM_RESOURCE("CScheduledMuxerV2::Delete(), end.\n");

    inherited::Delete();
}

EECode CScheduledMuxerV2::Initialize(TChar *p_param)
{
    EMuxerModuleID id;
    id = _guess_muxer_type_from_string(p_param);

    if (EMuxerModuleID_PrivateMP4 == id) {
        mRequestContainerType = ContainerType_MP4;
    } else if (EMuxerModuleID_PrivateTS == id) {
        mRequestContainerType = ContainerType_TS;
    }

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
            mpMuxer->Destroy();
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

    LOGM_INFO("[Muxer flow], CScheduledMuxerV2::Initialize() done\n");

    return EECode_OK;
}

EECode CScheduledMuxerV2::Finalize()
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
        mpMuxer->Destroy();
        mpMuxer = NULL;
    }

    return EECode_OK;
}

EECode CScheduledMuxerV2::Start()
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
        mpMasterInput = mpInputPins[0];//hard code here

        return mpScheduler->AddScheduledCilent((IScheduledClient *)this);
    }

    LOGM_FATAL("NULL mpScheduler\n");
    return EECode_BadState;
}

EECode CScheduledMuxerV2::Stop()
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
        msState = EModuleState_WaitCmd;
        return mpScheduler->RemoveScheduledCilent((IScheduledClient *)this);
    }

    LOGM_FATAL("NULL mpScheduler\n");
    return EECode_BadState;
}

EECode CScheduledMuxerV2::SwitchInput(TComponentIndex focus_input_index)
{
    LOGM_FATAL("TO DO\n");
    return EECode_NoImplement;
}

EECode CScheduledMuxerV2::FlowControl(EBufferType flowcontrol_type)
{
    LOGM_FATAL("TO DO\n");
    return EECode_NoImplement;
}

EECode CScheduledMuxerV2::AddOutputPin(TUint &index, TUint &sub_index, TUint type)
{
    LOGM_FATAL("CScheduledMuxerV2 do not have output pin\n");
    return EECode_InternalLogicalBug;
}

EECode CScheduledMuxerV2::AddInputPin(TUint &index, TUint type)
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

        mpInputPins[mnInputPinsNumber] = CMuxerInputV2::Create("[Audio input pin for CScheduledMuxerV2]", (IFilter *) this, mpCmdQueue, (StreamType) type, mnInputPinsNumber);
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

        mpInputPins[mnInputPinsNumber] = CMuxerInputV2::Create("[Video input pin for CScheduledMuxerV2]", (IFilter *) this, mpCmdQueue, (StreamType) type, mnInputPinsNumber);
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

IOutputPin *CScheduledMuxerV2::GetOutputPin(TUint index, TUint sub_index)
{
    LOGM_FATAL("CScheduledMuxerV2 do not have output pin\n");
    return NULL;
}

IInputPin *CScheduledMuxerV2::GetInputPin(TUint index)
{
    if (EConstAudioRendererMaxInputPinNumber > index) {
        return mpInputPins[index];
    } else {
        LOGM_ERROR("BAD index %d, max value %d, in CScheduledMuxerV2::GetInputPin()\n", index, EConstAudioRendererMaxInputPinNumber);
    }

    return NULL;
}

EECode CScheduledMuxerV2::FilterProperty(EFilterPropertyType property, TUint is_set, void *p_param)
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
                    LOGM_ERROR("EFilterPropertyType_muxer_saving_strategy TODO\n");
                }
            } else {
                LOGM_ERROR("NULL p_param\n");
                ret = EECode_BadParam;
            }
            break;

        case EFilterPropertyType_muxer_suspend:
            mbSuspended = 1;
            if (mpMuxer) {
                mpMuxer->SetState(1);
            }
            break;

        case EFilterPropertyType_muxer_resume_suspend:
            mbSuspended = 0;
            if (mpMuxer) {
                mpMuxer->SetState(0);
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

        case EFilterPropertyType_muxer_set_specified_info:
            if (mpMuxer) {
                mpMuxer->SetSpecifiedInfo((SRecordSpecifiedInfo *)p_param);
            }
            break;

        case EFilterPropertyType_muxer_get_specified_info:
            if (mpMuxer) {
                mpMuxer->GetSpecifiedInfo((SRecordSpecifiedInfo *)p_param);
            }
            break;

        default:
            LOGM_FATAL("BAD property 0x%08x\n", property);
            ret = EECode_InternalLogicalBug;
            break;
    }

    return ret;
}

void CScheduledMuxerV2::GetInfo(INFO &info)
{
    info.mPriority = 0;
    info.mFlags = 0;
    info.nInput = mnInputPinsNumber;
    info.nOutput = 1;

    info.pName = mpModuleName;
}

void CScheduledMuxerV2::PrintStatus()
{
    TUint i = 0;

    LOGM_PRINTF("msState=%d, %s, mnInputPinsNumber %d, mpMuxer %p, heart beat %d\n", msState, gfGetModuleStateString(msState), mnInputPinsNumber, mpMuxer, mDebugHeartBeat);

    for (i = 0; i < mnInputPinsNumber; i ++) {
        LOGM_PRINTF("       inputpin[%p, %d]'s status:\n", mpInputPins[i], i);
        if (mpInputPins[i]) {
            mpInputPins[i]->PrintStatus();
        }
    }

    mDebugHeartBeat = 0;

    return;
}

EECode CScheduledMuxerV2::Scheduling(TUint times, TU32 inout_mask)
{
    EECode err;
    SCMD cmd;
    CIBuffer *tmp_buffer = NULL;
    TInt process_cmd_count = 0;
    TU32 i = 0, index = 0;

    DASSERT(mpMuxer);
    DASSERT(mpEngineMsgSink);
    DASSERT(mpPersistMediaConfig);
    DASSERT(mpInputPins[0]);

    DASSERT(mbMuxerContentSetup);
    //DASSERT(!mbMuxerStarted);

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

            DASSERT(mpMasterInput);
            process_cmd_count = 0;
            if (DLikely(!mpMasterInput->mbExtraDataComes)) {
                if (readInputData(mpMasterInput, mBufferType)) {
                    if (_isMuxerExtraData(mBufferType) && mpBuffer) {
                        err = saveExtraData(mpMasterInput->mType, mpMasterInput->mStreamIndex, (void *)mpBuffer->GetDataPtr(), mpBuffer->GetDataSize());
                        DASSERT_OK(err);
                        err = saveStreamInfo(mpMasterInput->mType, 0, mpBuffer);
                        DASSERT_OK(err);
                        mpMasterInput->mbExtraDataComes = 1;
                        LOGM_NOTICE("master pin's extra data comes.\n");
                        mpBuffer->Release();
                        mpBuffer = NULL;

                        for (i = 0; i < mnInputPinsNumber; i ++) {
                            if (mpInputPins[i] == mpMasterInput) {
                                continue;
                            }
                            if (mpInputPins[i]->mbExtraDataComes) {
                                continue;
                            }

                            if (readInputData(mpInputPins[i], mBufferType)) {
                                if (_isMuxerExtraData(mBufferType) && mpBuffer) {
                                    err = saveExtraData(mpInputPins[i]->mType, mpInputPins[i]->mStreamIndex, (void *)mpBuffer->GetDataPtr(), mpBuffer->GetDataSize());
                                    DASSERT_OK(err);
                                    err = saveStreamInfo(mpInputPins[i]->mType, i, mpBuffer);
                                    DASSERT_OK(err);
                                    mpInputPins[i]->mbExtraDataComes = 1;
                                    LOGM_NOTICE("non-master pin's extra data comes.\n");
                                } else {
                                    mpInputPins[i]->Enable(0);
                                    LOGM_WARN("buffer not extra data(%p, %d)? disable stream %d, since it's extra da not comes\n", mpBuffer, i, mBufferType);
                                }
                                mpBuffer->Release();
                                mpBuffer = NULL;
                            } else {
                                mpInputPins[i]->Enable(0);
                                LOGM_WARN("disable stream %d, since it's extra da not comes\n", i);
                            }
                        }

                        if (!mbSuspended) {
                            if (DLikely(!mbFileCreated)) {
                                if (DLikely((!mbAutoName) && mpStorageManager && mpChannelName)) {
                                    updateFileNameFromDatabase();
                                } else {
                                    updateFileName(mCurrentFileIndex);
                                }
                                LOGM_INFO("[muxer] start running, initialize new file %s.\n", mpOutputFileName);

                                err = initializeFile(mpOutputFileName, mRequestContainerType);
                                if (DUnlikely(err != EECode_OK)) {
                                    LOGM_FATAL("CScheduledMuxerV2::initializeFile fail, enter error state. ret %d, %s\n", err, gfGetErrorCodeString(err));

                                    for (i = 0; i < mnInputPinsNumber; i ++) {
                                        if (mpInputPins[i]) {
                                            mpInputPins[i]->Purge(1);
                                        }
                                    }
                                    msState = EModuleState_Error;
                                    break;
                                }
                                mbFileCreated = 1;
                            }
                        }

                        setExtraData();
                        msState = EModuleState_Idle;
                        break;
                    } else if (_isFlowControlBuffer(mBufferType) && mpBuffer) {
                        LOGM_WARN("ignore them\n");
                    } else {
                        LOGM_WARN("Why comes here, ignore mBufferType %d\n", mBufferType);
                    }
                    mpBuffer->Release();
                    mpBuffer = NULL;
                    break;
                } else {
                    for (i = 0; i < mnInputPinsNumber; i ++) {
                        if (mpInputPins[i] == mpMasterInput) {
                            continue;
                        }

                        if (readInputData(mpInputPins[i], mBufferType)) {
                            process_cmd_count ++;
                            if (!mpInputPins[i]->mbExtraDataComes && _isMuxerExtraData(mBufferType) && mpBuffer) {
                                err = saveExtraData(mpInputPins[i]->mType, mpInputPins[i]->mStreamIndex, (void *)mpBuffer->GetDataPtr(), mpBuffer->GetDataSize());
                                DASSERT_OK(err);
                                err = saveStreamInfo(mpInputPins[i]->mType, i, mpBuffer);
                                DASSERT_OK(err);
                                mpInputPins[i]->mbExtraDataComes = 1;
                                LOGM_NOTICE("non-master pin(%d)'s extra data comes.\n", i);
                            }
                            mpBuffer->Release();
                            mpBuffer = NULL;
                        }
                    }
                }
            }
            if (!process_cmd_count) {
                return EECode_NotRunning;
            }
            return EECode_OK;
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
                index = (i + mPreferentialPinIndex) % mnInputPinsNumber;//Circularly operate all input pins
                mpPin = mpInputPins[index];
                if (!mpPin || !mpPin->mpBufferQ->GetDataCnt()) {
                    continue;
                }

                if (readInputData(mpPin, mBufferType)) {
                    //LOGM_DEBUG("mSavingFileStrategy %d, mpPin %p, mpMasterInput %p, mpBuffer->GetBufferNativePTS() %llu, mNextFileTimeThreshold %llu.\n", mSavingFileStrategy, mpPin, mpMasterInput, mpBuffer->GetBufferNativePTS(), mNextFileTimeThreshold);
                    if (DLikely(_isMuxerInputData(mBufferType) && mpBuffer)) {
                        if (DLikely(MuxerSavingFileStrategy_AutoSeparateFile == mSavingFileStrategy)) {
                            if (DUnlikely(1 == isCommingBufferAutoFileBoundary(mpPin, mpBuffer))) {
                                if (!mpPersistMediaConfig->cutfile_with_precise_pts) {
                                    DASSERT(NULL == mpPin->mpCachedBuffer);
                                    if (DLikely(mpMasterInput == mpPin)) {
                                        DASSERT(EPredefinedPictureType_IDR == mpBuffer->mVideoFrameType);
                                        mNewTimeThreshold = mpBuffer->GetBufferNativePTS();
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
                                        mNewTimeThreshold = mpBuffer->GetBufferNativePTS();
                                        LOGM_INFO("[Muxer], detected AUTO saving file, new video IDR threshold %llu, mNextFileTimeThreshold %llu, diff (%lld).\n", mNewTimeThreshold, mNextFileTimeThreshold, mNewTimeThreshold - mNextFileTimeThreshold);
                                        mpPin->mpCachedBuffer = mpBuffer;
                                        mpBuffer = NULL;
                                        mpPin->mbAutoBoundaryReached = 1;
                                        msState = EModuleState_Muxer_SavingPartialFilePeekRemainingOnNonMasterPin;
                                    } else {
                                        mNewTimeThreshold = mNextFileTimeThreshold;
                                        LOGM_INFO("[Muxer], detected AUTO saving file, non-master pin detect boundary, pts %llu, mAudioNextFileTimeThreshold %llu, diff %lld, write buffer here.\n", mpBuffer->GetBufferNativePTS(), mNextFileTimeThreshold, mpBuffer->GetBufferNativePTS() - mNextFileTimeThreshold);
                                        writeData(mpBuffer, mpPin);
                                        mpBuffer->Release();
                                        mpBuffer = NULL;
                                        msState = EModuleState_Muxer_SavingPartialFilePeekRemainingOnMasterPin;
                                        mpPin->mbAutoBoundaryReached = 1;
                                    }
                                }
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
                    } else if (DUnlikely(_isMuxerExtraData(mBufferType))) {
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
                    } else if (DUnlikely(_isFlowControlBuffer(mBufferType))) {
                        if (DLikely(mpBuffer)) {
                            if (EBufferType_FlowControl_FinalizeFile == mBufferType) {
                                DASSERT(mpMasterInput);
                                DASSERT(mpPin->mType == StreamType_Video);
                                DASSERT(NULL == mpPin->mpCachedBuffer);
                                //mpMasterInput = mpPin;
                                mpPin->mbAutoBoundaryReached = 1;
                                mNextFileTimeThreshold = mpBuffer->GetBufferNativePTS();
                                mNewTimeThreshold = mNextFileTimeThreshold;

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
                        } else {
                            LOGM_FATAL("why NULL mpBuffer\n");
                        }
                    } else {
                        LOGM_FATAL("BAD buffer type, %d.\n", mBufferType);
                        if (mpBuffer) {
                            mpBuffer->Release();
                            mpBuffer = NULL;
                        }
                    }

                } else if (DUnlikely(mpPin->mbSkiped != 1)) {
                    //peek buffer fail. must not comes here
                    LOGM_FATAL("peek buffer fail. must not comes here.\n");
                    msState = EModuleState_Error;
                }
            }
            break;

        case EModuleState_Muxer_SavingPartialFile:

            while (mpCmdQueue->PeekMsg(&cmd, sizeof(cmd))) {
                processCmd(cmd);
                process_cmd_count ++;
            }
            if (process_cmd_count) {
                process_cmd_count = 0;
                break;
            }

            //DASSERT(mNewTimeThreshold >= mNextFileTimeThreshold);

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
                    LOGM_INFO("[muxer %p, inpin %d] peek packet, PTS %llu, PTS threshold %llu.\n", this, mInputIndex, tmp_buffer->GetBufferNativePTS(), mNewTimeThreshold);

                    writeData(tmp_buffer, mpInputPins[mInputIndex]);

                    if (tmp_buffer->GetBufferNativePTS() > mNewTimeThreshold) {
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

            if (DUnlikely(err != EECode_OK)) {
                LOGM_FATAL("CScheduledMuxerV2::finalizeFile fail, enter error state, ret %d %s.\n", err, gfGetErrorCodeString(err));
                msState = EModuleState_Error;
                break;
            }

            if (DUnlikely(mbAutoName)) {
                if (DUnlikely(mMaxTotalFileNumber)) {
                    if (mTotalFileNumber >= mMaxTotalFileNumber) {
                        deletePartialFile(mFirstFileIndexCanbeDeleted);
                        mFirstFileIndexCanbeDeleted ++;
                        mTotalFileNumber --;
                    }
                }
            }

            //corner case, little chance to comes here
            if (DUnlikely(allInputEos())) {
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

            if (DUnlikely(mTotalRecFileNumber > 0 && mTotalFileNumber >= mTotalRecFileNumber)) {
                LOGM_NOTICE("the num of rec file(%d) has reached the limit(%d), so stop recording\n", mTotalFileNumber, mTotalRecFileNumber);
                postMsg(EMSGType_RecordingReachPresetTotalFileNumbers);
                msState = EModuleState_Completed;
                break;
            }

            mNextFileTimeThreshold += mAutoSavingTimeDuration;
            if (DUnlikely(mNextFileTimeThreshold < mNewTimeThreshold)) {
                LOGM_WARN("mNextFileTimeThreshold(%llu) + mAutoSavingTimeDuration(%llu) still less than mNewTimeThreshold(%llu).\n", mNextFileTimeThreshold, mAutoSavingTimeDuration, mNewTimeThreshold);
                mNextFileTimeThreshold += mAutoSavingTimeDuration;
            }

            if (!mbSuspended) {
                ++mCurrentFileIndex;
                if (DLikely((!mbAutoName) && mpStorageManager && mpChannelName)) {
                    updateFileNameFromDatabase();
                } else {
                    updateFileName(mCurrentFileIndex);
                }
                LOGM_NOTICE("[muxer] initialize new file %s.\n", mpOutputFileName);

                err = initializeFile(mpOutputFileName, mRequestContainerType);
                if (DUnlikely(err != EECode_OK)) {
                    LOGM_FATAL("CScheduledMuxerV2::initializeFile fail, enter error state.\n");
                    msState = EModuleState_Error;
                    break;
                }
            }
            LOGM_INFO("[muxer %p] end saving patial file.\n", this);
            msState = EModuleState_Idle;
            break;

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
                        LOGM_INFO("[master inpin] peek packet, PTS %llu, PTS threshold %llu.\n", tmp_buffer->GetBufferNativePTS(), mNextFileTimeThreshold);
                        if ((tmp_buffer->GetBufferNativePTS() >  mNextFileTimeThreshold) && (EPredefinedPictureType_IDR == tmp_buffer->mVideoFrameType)) {
                            if (mpMasterInput->mpCachedBuffer) {
                                LOGM_WARN("why comes here, release cached buffer, pts %llu, type %d.\n", mpMasterInput->mpCachedBuffer->GetBufferNativePTS(), mpMasterInput->mpCachedBuffer->mVideoFrameType);
                                mpMasterInput->mpCachedBuffer->Release();
                            }
                            mpMasterInput->mpCachedBuffer = tmp_buffer;
                            if (!_isTmeStampNear(mNewTimeThreshold, tmp_buffer->GetBufferNativePTS(), TimeUnitNum_fps29dot97)) {
                                //refresh mpPin's threshold
                                mpPin->mbAutoBoundaryReached = 0;
                                LOGM_WARN("refresh boundary!\n");
                            }
                            mNewTimeThreshold = tmp_buffer->GetBufferNativePTS();
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
            if (mpMasterInput->mpCachedBuffer) {
                LOGM_WARN("Already have get next IDR, goto next stage.\n");
                mpMasterInput->mbAutoBoundaryReached = 1;
                msState = EModuleState_Muxer_SavingPartialFilePeekRemainingOnNonMasterPin;
                break;
            }

            if (0 == mpMasterInput->mbAutoBoundaryReached) {

                while (1 == mpMasterInput->PeekBuffer(tmp_buffer)) {
                    if (_isMuxerInputData(tmp_buffer->GetBufferType())) {
                        if (EPredefinedPictureType_IDR == tmp_buffer->mVideoFrameType && tmp_buffer->GetBufferNativePTS() >= mNewTimeThreshold) {
                            mpMasterInput->mpCachedBuffer = tmp_buffer;
                            if (!_isTmeStampNear(mNewTimeThreshold, tmp_buffer->GetBufferNativePTS(), TimeUnitNum_fps29dot97)) {
                                //refresh new time threshold, need recalculate non-masterpin
                                if ((StreamType)mpPin->mType != StreamType_Audio) {
                                    mpPin->mbAutoBoundaryReached = 0;
                                }
                            }
                            LOGM_INFO("[avsync]: wait new IDR, pts %llu, previous time threshold %llu, diff %lld.\n", tmp_buffer->GetBufferNativePTS(), mNewTimeThreshold, tmp_buffer->GetBufferNativePTS() - mNewTimeThreshold);
                            mNewTimeThreshold = tmp_buffer->GetBufferNativePTS();
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
            if (DUnlikely(!hasPinNeedReachBoundary(mInputIndex))) {
                msState = EModuleState_Muxer_SavingPartialFile;
                LOGM_INFO("[avsync]: all pin(need sync) reaches boundary.\n");
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

            if (DLikely(mpPin != mpMasterInput)) {
                //peek non-master input
                DASSERT(StreamType_Video != mpPin->mType);
                while (1 == mpPin->PeekBuffer(tmp_buffer)) {
                    DASSERT(tmp_buffer);
                    if (_isMuxerInputData(tmp_buffer->GetBufferType())) {
                        LOGM_INFO("[non-master inpin] (%p) peek packet, PTS %llu, PTS threshold %llu.\n", this, tmp_buffer->GetBufferNativePTS(), mNextFileTimeThreshold);

                        writeData(tmp_buffer, mpPin);

                        if (tmp_buffer->GetBufferNativePTS() >=  mNextFileTimeThreshold) {
                            LOGM_INFO("[avsync]: non-master pin (%p) get boundary, pts %llu, mNextFileTimeThreshold %llu, diff %lld.\n", this, mNewTimeThreshold, mNextFileTimeThreshold, mNewTimeThreshold - mNextFileTimeThreshold);
                            mpPin->mbAutoBoundaryReached = 1;
                            tmp_buffer->Release();
                            tmp_buffer = NULL;
                            break;
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
            LOGM_INFO("[avsync]: WAIT till boundary(non-master pin).\n");
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

            if (DLikely(0 == mpPin->mbAutoBoundaryReached)) {
                while (1 == mpPin->PeekBuffer(tmp_buffer)) {
                    if (DLikely(_isMuxerInputData(tmp_buffer->GetBufferType()))) {
                        writeData(tmp_buffer, mpPin);

                        if (tmp_buffer->GetBufferNativePTS() >= mNewTimeThreshold) {
                            LOGM_INFO("[avsync]: non-master pin(%p) wait new boundary, pts %llu, time threshold %llu, diff %lld.\n", this, tmp_buffer->GetBufferNativePTS(), mNewTimeThreshold, tmp_buffer->GetBufferNativePTS() - mNewTimeThreshold);
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
            LOGM_ERROR("CScheduledMuxerV2: bad state 0x%x\n", (TUint)msState);
            msState = EModuleState_Error;
            break;
    }

    return EECode_OK;
}

TInt CScheduledMuxerV2::IsPassiveMode() const
{
    return 1;
}

TSchedulingHandle CScheduledMuxerV2::GetWaitHandle() const
{
    return (-1);
}

TSchedulingUnit CScheduledMuxerV2::HungryScore() const
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

TU8 CScheduledMuxerV2::GetPriority() const
{
    return 1;
}

void CScheduledMuxerV2::Destroy()
{
    Delete();
}

EECode CScheduledMuxerV2::initialize(TChar *p_param)
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
            mpMuxer->Destroy();
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

void CScheduledMuxerV2::finalize()
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

EECode CScheduledMuxerV2::setOutputFile(TChar *pFileName)
{
    //param check
    if (!pFileName) {
        LOGM_FATAL("NULL params in CScheduledMuxerV2::SetOutputFile.\n");
        return EECode_BadParam;
    }

    TUint currentLen = strlen(pFileName) + 1;

    LOGM_INFO("CScheduledMuxerV2::SetOutputFile len %d, filename %s.\n", currentLen, pFileName);
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
    LOGM_INFO("CScheduledMuxerV2::SetOutputFile done, len %d, filenamebase %s.\n", mOutputFileNameBaseLength, mpOutputFileNameBase);
    return EECode_OK;
}

void CScheduledMuxerV2::postMsg(TUint msg_code)
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

TU32 CScheduledMuxerV2::processCmd(SCMD &cmd)
{
    DASSERT(mpCmdQueue);
    LOGM_INFO("cmd.code %d, state %d.\n", cmd.code, msState);

    switch (cmd.code) {
        case ECMDType_ExitRunning:
            mbRun = 0;
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

EECode CScheduledMuxerV2::writeData(CIBuffer *pBuffer, CMuxerInputV2 *pPin)
{
    SMuxerDataTrunkInfo data_trunk_info;
    TTime buffer_pts = 0;
    EECode err = EECode_OK;
    DASSERT(pBuffer);
    DASSERT(pPin);

    buffer_pts = pBuffer->GetBufferNativePTS();
    DASSERT(buffer_pts >= 0);

    data_trunk_info.native_dts = pBuffer->GetBufferNativeDTS();
    data_trunk_info.native_pts = pBuffer->GetBufferNativePTS();
    pPin->mInputEndPTS = buffer_pts;

    if (StreamType_Audio == pPin->mType) {

        //ensure first frame are key frame
        if (DUnlikely(pPin->mCurrentFrameCount == 0)) {
            //DASSERT(pPin->mDuration);
            //estimate current pts
            pPin->mSessionInputPTSStartPoint = buffer_pts;
            pPin->mSessionPTSStartPoint = pPin->mCurrentPTS;
            LOGM_INFO(" audio mSessionInputPTSStartPoint %lld, mSessionPTSStartPoint %lld, pInput->mDuration %lld.\n", pPin->mSessionInputPTSStartPoint, pPin->mSessionPTSStartPoint, pPin->mDuration);
        }

        if (!pPin->mbAutoBoundaryStarted) {
            pPin->mbAutoBoundaryStarted = 1;
            pPin->mInputSatrtPTS = buffer_pts;
            LOGM_INFO("[cut file, boundary start(audio), pts %llu]", pPin->mInputSatrtPTS);
        }

        pPin->mCurrentPTS = buffer_pts - pPin->mSessionInputPTSStartPoint + pPin->mSessionPTSStartPoint;
        LOGM_PTS("[PTS]:audio mCurrentPTS %lld, buffer_pts %lld, pInput->mTimeUintNum %d, pInput->mTimeUintDen %d.\n", pPin->mCurrentPTS, buffer_pts, pPin->mTimeUintNum, pPin->mTimeUintDen);

        if (DLikely(mbUseSourcePTS)) {
            data_trunk_info.pts = data_trunk_info.native_pts;
            data_trunk_info.dts = data_trunk_info.native_dts;
        } else {
            data_trunk_info.pts = data_trunk_info.dts = pPin->mCurrentPTS + pPin->mPTSDTSGap;
        }

        LOGM_PTS("[PTS]:audio write to file pts %lld, dts %lld, pts gap %lld, duration %lld, pInput->mPTSDTSGap %d.\n", data_trunk_info.pts, data_trunk_info.dts, data_trunk_info.pts - pPin->mLastPTS, pPin->mDuration, pPin->mPTSDTSGap);
        pPin->mLastPTS = data_trunk_info.pts;
        data_trunk_info.duration = pPin->mDuration;
        data_trunk_info.av_normalized_duration = pPin->mAVNormalizedDuration;
        mCurrentTotalFilesize += pBuffer->GetDataSize();

        data_trunk_info.is_key_frame = 1;
        data_trunk_info.stream_index = pPin->mStreamIndex;
        err = mpMuxer->WriteAudioBuffer(pBuffer, &data_trunk_info);
        DASSERT_OK(err);
    } else if (StreamType_Video == pPin->mType) {

        //master pin
        if ((!mbNextFileTimeThresholdSet) && (MuxerSavingFileStrategy_AutoSeparateFile == mSavingFileStrategy)) {
            if (MuxerSavingCondition_InputPTS == mSavingCondition) {
                mNextFileTimeThreshold = buffer_pts + mAutoSavingTimeDuration;
            } else if (MuxerSavingCondition_CalculatedPTS == mSavingCondition) {
                mNextFileTimeThreshold = mAutoSavingTimeDuration;
            }
            mbNextFileTimeThresholdSet = 1;
            LOGM_NOTICE("Get first Video buffer_pts %llu, mNextFileTimeThreshold %llu.\n", buffer_pts, mNextFileTimeThreshold);
        }

        //ensure first frame are key frame
        if (DUnlikely(0 == pPin->mCurrentFrameCount)) {
            //DASSERT(EPredefinedPictureType_IDR == pBuffer->mVideoFrameType);
            if (EPredefinedPictureType_IDR == pBuffer->mVideoFrameType) {
                LOGM_NOTICE("IDR picture comes, pInput->mDuration %lld, buffer_pts %lld.\n", pPin->mDuration, buffer_pts);

                //DASSERT(pPin->mDuration);
                //refresh session pts related
                pPin->mSessionInputPTSStartPoint = buffer_pts;
                if (pPin->mCurrentPTS != mStreamStartPTS) {
                    pPin->mSessionPTSStartPoint = pPin->mCurrentPTS;
                }
                LOGM_INFO(" video pInput->mSessionInputPTSStartPoint %lld, pInput->mSessionPTSStartPoint %lld.\n", pPin->mSessionInputPTSStartPoint, pPin->mSessionPTSStartPoint);
                if (!mbMasterStarted) {
                    LOGM_NOTICE("master started.\n");
                    mbMasterStarted = 1;
                }
            } else {
                LOGM_INFO("non-IDR frame (%d), data comes here, seq num [%d, %d], when stream not started, discard them.\n", pBuffer->mVideoFrameType, pBuffer->mBufferSeqNumber0, pBuffer->mBufferSeqNumber1);
                return EECode_OK;
            }
        }

        pPin->mFrameCountFromLastBPicture ++;

        if (DUnlikely(!pPin->mbAutoBoundaryStarted)) {
            pPin->mbAutoBoundaryStarted = 1;
            pPin->mInputSatrtPTS = buffer_pts;
            LOGM_INFO("[cut file, boundary start(video), pts %llu]\n", pPin->mInputSatrtPTS);
        }

        if (DUnlikely(buffer_pts < pPin->mSessionInputPTSStartPoint)) {
            DASSERT(pBuffer->mVideoFrameType == EPredefinedPictureType_B);
            LOGM_WARN(" PTS(%lld) < first IDR PTS(%lld), something should be wrong. or skip b frame(pBuffer->mVideoFrameType %d) after first IDR, these b frame should be skiped, when pause--->resume.\n", buffer_pts, pPin->mSessionInputPTSStartPoint, pBuffer->mVideoFrameType);
            return EECode_OK;
        }

        DASSERT(buffer_pts >= pPin->mSessionInputPTSStartPoint);
        pPin->mCurrentPTS = buffer_pts - pPin->mSessionInputPTSStartPoint + pPin->mSessionPTSStartPoint;
        LOGM_PTS("[PTS]:video mCurrentPTS %lld, mCurrentDTS %lld, pBuffer->GetPTS() %lld, mSessionInputPTSStartPoint %lld, mSessionPTSStartPoint %lld.\n", pPin->mCurrentPTS, pPin->mCurrentDTS, buffer_pts, pPin->mSessionInputPTSStartPoint, pPin->mSessionPTSStartPoint);

        if (DLikely(mbUseSourcePTS)) {
            data_trunk_info.pts = data_trunk_info.native_pts;
            data_trunk_info.dts = data_trunk_info.native_dts;
        } else {
            data_trunk_info.dts = pPin->mCurrentDTS;
            data_trunk_info.pts = pPin->mCurrentPTS + pPin->mPTSDTSGap;
        }

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

TU32 CScheduledMuxerV2::readInputData(CMuxerInputV2 *inputpin, EBufferType &type)
{
    DASSERT(!mpBuffer);
    DASSERT(inputpin);
    EECode ret;

    if (!inputpin->PeekBuffer(mpBuffer)) {
        //LOGM_WARN("No buffer? msState=%d\n", msState);
        return 0;
    }

    type = mpBuffer->GetBufferType();

    if (EBufferType_FlowControl_EOS == type) {
        LOGM_INFO("CScheduledMuxerV2 %p get EOS.\n", this);
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
        if (((TTime)DINVALID_VALUE_TAG_64) == mpBuffer->GetBufferNativePTS()) {
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

EECode CScheduledMuxerV2::flushInputData()
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

void CScheduledMuxerV2::deletePartialFile(TUint file_index)
{
    TChar *cmd = (char *)malloc(mOutputFileNameLength + 16);
    TInt ret = 0;

    if (NULL == cmd) {
        LOGM_ERROR("Cannot malloc memory in CScheduledMuxerV2::deletePartialFile, request size %d,\n", mOutputFileNameLength + 16);
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

void CScheduledMuxerV2::analyseFileNameFormat(TChar *pstart)
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

//update file name
void CScheduledMuxerV2::updateFileName(TUint file_index)
{
    LOGM_INFO("before append extntion: Muxing file name %s, mFileNameHanding1 %d, mFileNameHanding2 %d.\n", mpOutputFileName, mFileNameHanding1, mFileNameHanding2);

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
            memset(datetime_buffer, 0, sizeof(datetime_buffer));
            gfGetSTRFTimeString(datetime_buffer, sizeof(datetime_buffer));
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
}

void CScheduledMuxerV2::updateFileNameFromDatabase()
{
    EECode err = EECode_OK;
    if (mpOutputFileName) { err = mpStorageManager->FinalizeNewUint(mpChannelName, mpOutputFileName); }
    DASSERT(err == EECode_OK);
    mpOutputFileName = NULL;
    err = mpStorageManager->AcquireNewUint(mpChannelName, mpOutputFileName);
    if (DLikely(err == EECode_OK)) {
        LOGM_NOTICE("update file name done, file name %s\n", mpOutputFileName);
    } else {
        LOGM_FATAL("update file name fail!\n");
    }
}

TU32 CScheduledMuxerV2::isCommingBufferAutoFileBoundary(CMuxerInputV2 *pInputPin, CIBuffer *pBuffer)
{
    TTime pts = 0;
    DASSERT(mpMasterInput);
    DASSERT(pInputPin);
    DASSERT(pBuffer);

    pts = pBuffer->GetBufferNativePTS();
    if (DUnlikely((pInputPin == mpMasterInput) && (EPredefinedPictureType_IDR == pBuffer->mVideoFrameType))) {
        LOGM_DEBUG("[AUTO-CUT]master's IDR comes.\n");
        if (DUnlikely(!mbMasterStarted)) {
            LOGM_WARN("master pin not started yet\n");
            return 0;
        }
        if (DUnlikely(mSavingCondition == MuxerSavingCondition_FrameCount)) {
            if (DLikely((pInputPin->mCurrentFrameCount + 1) < mAutoSaveFrameCount)) {
                return 0;
            }
            return 1;
        } else if (DLikely(mSavingCondition == MuxerSavingCondition_InputPTS)) {
            if (DLikely(pts < mNextFileTimeThreshold)) {
                return 0;
            }
            return 1;
        } else if (DUnlikely(mSavingCondition == MuxerSavingCondition_CalculatedPTS)) {
            TTime comming_pts = pts - pInputPin->mSessionInputPTSStartPoint + pInputPin->mSessionPTSStartPoint;
            if (DLikely(comming_pts < mNextFileTimeThreshold)) {
                return 0;
            }
            return 1;
        } else {
            LOGM_ERROR("NEED implement.\n");
            return 0;
        }
    } else if ((pInputPin != mpMasterInput) && (_pinNeedSync((StreamType)pInputPin->mType))) {
        DASSERT(pInputPin->mType != StreamType_Video);
        if (!mpPersistMediaConfig->cutfile_with_precise_pts) {
            return 0;
        }

        if (DLikely(pts < mNextFileTimeThreshold)) {
            return 0;
        }
        return 1;
    }

    return 0;
}

TU32 CScheduledMuxerV2::hasPinNeedReachBoundary(TUint &i)
{
    for (i = 0; i < mnInputPinsNumber; i++) {
        if (DLikely(mpInputPins[i])) {
            if (_pinNeedSync((StreamType)mpInputPins[i]->mType) && (0 == mpInputPins[i]->mbAutoBoundaryReached)) {
                return 1;
            }
        }
    }
    i = 0;
    return 0;
}

TU32 CScheduledMuxerV2::allInputEos()
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

EECode CScheduledMuxerV2::initializeFile(TChar *p_param, ContainerType container_type)
{
    EECode err = EECode_OK;

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

EECode CScheduledMuxerV2::finalizeFile()
{
    if (mpMuxer) {
        if (mbMuxerStarted) {
            getFileInformation();
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

void CScheduledMuxerV2::getFileInformation()
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
    //DASSERT(mMuxerFileInfo.file_duration);
    mMuxerFileInfo.file_bitrate = 0;
    if (mMuxerFileInfo.file_duration) {
        mMuxerFileInfo.file_bitrate = (TUint)(((float)mCurrentTotalFilesize * 8 * TimeUnitDen_90khz) / ((float)mMuxerFileInfo.file_duration));
        //LOGM_INFO("((float)mCurrentTotalFilesize*8*TimeUnitDen_90khz) %f .\n", ((float)mCurrentTotalFilesize*8*TimeUnitDen_90khz));
        //LOGM_INFO("result %f.\n", (((float)mCurrentTotalFilesize*8*TimeUnitDen_90khz)/((float)mFileDuration)));
        //LOGM_INFO("mFileBitrate %d.\n", mFileBitrate);
    }

    LOGM_INFO("[file info]: pre-estimated bitrate %u, calculated bitrate %u.\n", estimated_bitrate, mMuxerFileInfo.file_bitrate);
}

void CScheduledMuxerV2::initializeTimeInfo()
{
    TUint i = 0;
    CMuxerInputV2 *pInput = NULL;

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

EECode CScheduledMuxerV2::saveStreamInfo(StreamType type, TUint stream_index, CIBuffer *p_buffer)
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
                if (p_buffer->mVideoFrameRateNum && p_buffer->mVideoFrameRateDen) {
                    mStreamCodecInfos.info[stream_index].spec.video.framerate = (p_buffer->mVideoFrameRateNum / p_buffer->mVideoFrameRateDen);
                } else {
                    mStreamCodecInfos.info[stream_index].spec.video.framerate = 29.97;
                    LOGM_WARN("CScheduledMuxerV2: framerate not set, use default framerate %f\n", mStreamCodecInfos.info[stream_index].spec.video.framerate);
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

EECode CScheduledMuxerV2::saveExtraData(StreamType type, TUint stream_index, void *p_data, TUint data_size)
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

EECode CScheduledMuxerV2::setExtraData()
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

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END


