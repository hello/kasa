/*******************************************************************************
 * ts_demuxer.cpp
 *
 * History:
 *    2014/04/21 - [Zhi He] create file
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
#include "common_io.h"
#include "storage_lib_if.h"

#include "media_mw_if.h"
#include "media_mw_utils.h"
#include "framework_interface.h"
#include "codec_misc.h"
#include "mw_internal.h"
#include "dsp_platform_interface.h"
#include "streaming_if.h"
#include "modules_interface.h"

#include "mpeg_ts_defs.h"
#include "property_ts.h"

#include "common_private_data.h"
#include "ts_demuxer.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

static const TInt aac_sample_rates[16] = {
    96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000, 7350
};

static TU8 *_ts_packet_start(TU8 *pData, TUint &length)
{
    TU8 *pRet = pData;
    while (length) {
        if (*(pRet) == MPEG_TS_TP_SYNC_BYTE) {
            if (length > MPEG_TS_TP_PACKET_SIZE) {
                if (*(pRet + MPEG_TS_TP_PACKET_SIZE) == MPEG_TS_TP_SYNC_BYTE) {
                    break;
                }
            } else {
                //last packet in cache buffer
                break;
            }
        }
        pRet++;
        length--;
    }

    return (length) ?  pRet : NULL;
#if 0
    TUint parsedLength = 0;
    while (parsedLength <= length) {
        if (pData[parsedLength] == 0x47) {
            break;
        }
        parsedLength++;
    }

    return (parsedLength > length) ? NULL : (pData + parsedLength);
#endif
}

static TTime _get_ptsordts(TU8 *pBuffer)
{
    TTime PTSorDTS = ((((pBuffer[0] & 0x0f) >> 1) & 0x07) << 29)
                     | ((((pBuffer[1] << 8) | pBuffer[2]) >> 1) << 15)
                     | (((pBuffer[3] << 8) | pBuffer[4]) >> 1);

    return PTSorDTS;
}

static void _clear_pes_info(SPESInfo *pPes)
{
    TU8 streamIndex = pPes->streamIndex;
    memset((void *)pPes, 0x0, sizeof(SPESInfo));
    pPes->streamIndex = streamIndex;
    return;
}

static EECode _release_ring_buffer(CIBuffer *pBuffer)
{
    DASSERT(pBuffer);
    if (pBuffer) {
        if (EBufferCustomizedContentType_RingBuffer == pBuffer->mCustomizedContentType) {
            IMemPool *pool = (IMemPool *)pBuffer->mpCustomizedContent;
            DASSERT(pool);
            if (pool) {
                pool->ReleaseMemBlock((TULong)(pBuffer->GetDataMemorySize()), pBuffer->GetDataPtrBase());
            }
        }
    } else {
        LOG_FATAL("NULL pBuffer!\n");
        return EECode_InternalLogicalBug;
    }

    return EECode_OK;
}

static TU8 _parse_stream_type(TU8 input_type)
{
    switch (input_type) {
        case MPEG_SI_STREAM_TYPE_AAC:
            return StreamFormat_AAC;

        case MPEG_SI_STREAM_TYPE_AVC_VIDEO:
            return StreamFormat_H264;

        default:
            LOG_FATAL("Add new type parse here, input type: 0x%x\n", input_type);
            break;
    }

    return StreamFormat_Invalid;
}
////////////////////////////////////////////////////////////////////////////////
IDemuxer *gfCreateTSDemuxerModule(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TUint index)
{
    return CTSDemuxer::Create(pName, pPersistMediaConfig, pEngineMsgSink, index);
}

IDemuxer *CTSDemuxer::Create(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink, TUint index)
{
    CTSDemuxer *result = new CTSDemuxer(pname, pPersistMediaConfig, pMsgSink, index);
    if (result && result->Construct() != EECode_OK) {
        delete result;
        result = NULL;
    }

    return result;
}

CObject *CTSDemuxer::GetObject0() const
{
    return (CObject *) this;
}

CTSDemuxer::CTSDemuxer(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink, TUint index)
    : inherited(pname, index)
    , mpPersistMediaConfig(pPersistMediaConfig)
    , mpMsgSink(NULL)
    , mpBufferBase(NULL)
    , mpBufferAligned(NULL)
    , mBufferSize(0)
    , mDataRemainSize(0)
    , mDataSize(0)
    , mTsPacketNum(4096)
    , mTsPacketSize(MPEG_TS_TP_PACKET_SIZE)
    , mpIO(NULL)
    , mbFileOpened(0)
    , mbFileEOF(0)
    , mFileTotalSize(0)
    , mpStorageManager(NULL)
    , mpCurUrl(NULL)
    , mpChannelName(NULL)
    , mCurDuration(0)
    , mbDebugPTSGap(0)//TODO
    , mbEnableVideo(0)
    , mbEnableAudio(0)
    , mbVodMode(0)
    , mbExtraDataSent(1)
    , mpVideoExtraData(NULL)
    , mVideoExtraDataSize(0)
    , mpAudioExtraData(NULL)
    , mAudioExtraDataSize(0)
    , mFakeVideoPTS(0)
    , mFakeAudioPTS(0)
    , mVideoFrameCount(0)
    , mAudioFrameCount(0)
    , mVideoFakePTSNum(DDefaultVideoFramerateDen)//hard code here
    , mVideoFakePTSDen(DDefaultVideoFramerateNum)//hard code here
    , mAudioFakePTSNum(1024)//hard code here
    , mAudioFakePTSDen(DDefaultAudioSampleRate)//hard code here
    , mFramRate(30)
    , mGOPSize(120)
{
}

EECode CTSDemuxer::Construct()
{
    DSET_MODULE_LOG_CONFIG(ELogModuleNativeTSDemuxer);

    for (TUint i = 0; i < EConstMaxDemuxerMuxerStreamNumber; i ++) {
        mpOutputPins[i] = NULL;
        mpBufferPool[i] = NULL;
        mpMemPool[i] = NULL;
    }

    mpBufferBase = (TU8 *)DDBG_MALLOC(mTsPacketNum * MPEG_TS_TP_PACKET_SIZE + 31, "DMFB");
    if (!mpBufferBase) {
        return EECode_NoMemory;
    }
    memset(mpBufferBase, 0x0, mTsPacketNum * MPEG_TS_TP_PACKET_SIZE + 31);
    mBufferSize = mTsPacketNum * MPEG_TS_TP_PACKET_SIZE;

    mpBufferAligned = mpBufferBase;

    _clear_pes_info(&msVideoPES);
    _clear_pes_info(&msAudioPES);

    if ((TULong)mpBufferAligned & 0x1f) {
        mpBufferAligned = (TU8 *)(((TULong)mpBufferAligned + 0x1f) & (~0x1f));
    }

    mpIO = gfCreateIO(EIOType_File);
    if (!mpIO) {
        return EECode_InternalLogicalBug;
    }

    mpStorageManager = mpPersistMediaConfig->p_storage_manager;
    msAudioStream.enable = 0;
    msVideoStream.enable = 0;

    for (TInt i = 0; i < EConstMaxDemuxerMuxerStreamNumber; i++) {
        mStreamLastPTS[i] = 0;
        mStreamFirstPTS[i] = -1;
        mPTSDiff[i] = 0;
    }

    return inherited::Construct();
}

void CTSDemuxer::Delete()
{
    if (mpWorkQ) {
        mpWorkQ->SendCmd(ECMDType_ExitRunning);
    }

    if (mpMemPool[EDemuxerVideoOutputPinIndex]) {
        mpMemPool[EDemuxerVideoOutputPinIndex]->GetObject0()->Delete();
        mpMemPool[EDemuxerVideoOutputPinIndex] = NULL;
    }

    if (mpMemPool[EDemuxerAudioOutputPinIndex]) {
        mpMemPool[EDemuxerAudioOutputPinIndex]->GetObject0()->Delete();
        mpMemPool[EDemuxerAudioOutputPinIndex] = NULL;
    }

    if (mpBufferBase) {
        DDBG_FREE(mpBufferBase, "DMFB");
        mpBufferBase = NULL;
        mpBufferAligned = NULL;
        mBufferSize = 0;
    }

    if (mpIO) {
        mpIO->Delete();
        mpIO = NULL;
    }

    if (mpVideoExtraData) {
        DDBG_FREE(mpVideoExtraData, "dmVE");
        mpVideoExtraData = NULL;
    }

    if (mpAudioExtraData) {
        DDBG_FREE(mpAudioExtraData, "dmAE");
        mpAudioExtraData = NULL;
    }

    if (mpCurUrl) {
        mpStorageManager->ReleaseExistUint(mpChannelName, mpCurUrl);
        mpCurUrl = NULL;
    }
    mpStorageManager = NULL;
    if (mpChannelName) {
        DDBG_FREE(mpChannelName, "dmcn");
        mpChannelName = NULL;
    }

    inherited::Delete();
}

CTSDemuxer::~CTSDemuxer()
{
}

EECode CTSDemuxer::SetupOutput(COutputPin *p_output_pins [ ], CBufferPool *p_bufferpools [ ],  IMemPool *p_mempools[], IMsgSink *p_msg_sink)
{
    DASSERT(!mpBufferPool[EDemuxerVideoOutputPinIndex]);
    DASSERT(!mpBufferPool[EDemuxerAudioOutputPinIndex]);
    DASSERT(!mpOutputPins[EDemuxerVideoOutputPinIndex]);
    DASSERT(!mpOutputPins[EDemuxerAudioOutputPinIndex]);
    DASSERT(!mpMsgSink);

    mpOutputPins[EDemuxerVideoOutputPinIndex] = (COutputPin *)p_output_pins[EDemuxerVideoOutputPinIndex];
    mpOutputPins[EDemuxerAudioOutputPinIndex] = (COutputPin *)p_output_pins[EDemuxerAudioOutputPinIndex];
    mpBufferPool[EDemuxerVideoOutputPinIndex] = (CBufferPool *)p_bufferpools[EDemuxerVideoOutputPinIndex];
    mpBufferPool[EDemuxerAudioOutputPinIndex] = (CBufferPool *)p_bufferpools[EDemuxerAudioOutputPinIndex];

    mpMsgSink = p_msg_sink;

    if (mpOutputPins[EDemuxerVideoOutputPinIndex] && mpBufferPool[EDemuxerVideoOutputPinIndex]) {
        mpBufferPool[EDemuxerVideoOutputPinIndex]->SetReleaseBufferCallBack(_release_ring_buffer);
        mpBufferPool[EDemuxerVideoOutputPinIndex]->AddBufferNotifyListener((IEventListener *)this);
        mpMemPool[EDemuxerVideoOutputPinIndex] = CRingMemPool::Create(2 * 1024 * 1024);
        mbEnableVideo = 1;
    }

    if (mpOutputPins[EDemuxerAudioOutputPinIndex] && mpBufferPool[EDemuxerAudioOutputPinIndex]) {
        mpBufferPool[EDemuxerAudioOutputPinIndex]->SetReleaseBufferCallBack(_release_ring_buffer);
        mpBufferPool[EDemuxerAudioOutputPinIndex]->AddBufferNotifyListener((IEventListener *)this);
        mpMemPool[EDemuxerAudioOutputPinIndex] = CRingMemPool::Create(128 * 1024);
        mbEnableAudio = 1;
    }

    if (!mbEnableVideo && !mbEnableAudio) {
        LOGM_ERROR("neither video nor audio is enabled!?\n");
        return EECode_Error;
    }

    return EECode_OK;
}

EECode CTSDemuxer::SetupContext(TChar *url, void *p_agent, TU8 priority, TU32 request_receive_buffer_size, TU32 request_send_buffer_size)
{
    TChar *pFormat = strrchr(url, '.');
    if (pFormat && (!strcmp(pFormat, ".ts"))) {
        if (setupContext(url) != EECode_OK) {
            return EECode_InternalLogicalBug;
        }
    }

    if (mpWorkQ) {
        LOGM_FLOW("before mpWorkQ->SendCmd(ECMDType_StartRunning)...\n");
        EECode err = mpWorkQ->SendCmd(ECMDType_StartRunning);
        LOGM_FLOW("after mpWorkQ->SendCmd(ECMDType_StartRunning), ret %d\n", err);
        DASSERT_OK(err);
    } else {
        LOG_FATAL("NULL workQ?!\n");
        return EECode_InternalLogicalBug;
    }

    return EECode_OK;
}

EECode CTSDemuxer::DestroyContext()
{
    return destroyContext();
}

EECode CTSDemuxer::ReconnectServer()
{
    return EECode_OK;
}

EECode CTSDemuxer::Seek(TTime &target_time, ENavigationPosition position)
{
    return EECode_OK;
}

EECode CTSDemuxer::Start()
{
    EECode err = EECode_OK;

    DASSERT(mpWorkQ);

    LOGM_FLOW("before mpWorkQ->SendCmd(ECMDType_Start)...\n");
    err = mpWorkQ->SendCmd(ECMDType_Start);
    LOGM_FLOW("after mpWorkQ->SendCmd(ECMDType_Start), ret %d\n", err);

    DASSERT_OK(err);

    return err;
}

EECode CTSDemuxer::Stop()
{
    EECode err = EECode_OK;

    DASSERT(mpWorkQ);

    LOGM_FLOW("before mpWorkQ->SendCmd(ECMDType_Stop)...\n");
    err = mpWorkQ->SendCmd(ECMDType_Stop);
    LOGM_FLOW("after mpWorkQ->SendCmd(ECMDType_Stop), ret %d\n", err);

    DASSERT_OK(err);

    return err;
}

EECode CTSDemuxer::Suspend()
{
    return EECode_OK;
}

EECode CTSDemuxer::Pause()
{
    EECode err = EECode_OK;

    DASSERT(mpWorkQ);

    LOGM_FLOW("before mpWorkQ->PostMsg(ECMDType_Pause)...\n");
    err = mpWorkQ->PostMsg(ECMDType_Pause);
    LOGM_FLOW("after mpWorkQ->PostMsg(ECMDType_Pause), ret %d\n", err);

    DASSERT_OK(err);

    return err;
}

EECode CTSDemuxer::Resume()
{
    EECode err = EECode_OK;

    DASSERT(mpWorkQ);

    LOGM_FLOW("before mpWorkQ->PostMsg(ECMDType_Resume)...\n");
    err = mpWorkQ->PostMsg(ECMDType_Resume);
    LOGM_FLOW("after mpWorkQ->PostMsg(ECMDType_Resume), ret %d\n", err);

    DASSERT_OK(err);

    return err;
}

EECode CTSDemuxer::Flush()
{
    EECode err = EECode_OK;
    return err;
}

EECode CTSDemuxer::ResumeFlush()
{
    EECode err = EECode_OK;
    return err;
}

EECode CTSDemuxer::Purge()
{
    EECode err = EECode_OK;
    return err;
}

EECode CTSDemuxer::SetPbRule(TU8 direction, TU8 feeding_rule, TU16 speed)
{
    EECode err = EECode_OK;
    SCMD cmd;

    if (DLikely(mpWorkQ)) {
        cmd.code = ECMDType_UpdatePlaybackSpeed;
        cmd.pExtra = NULL;
        cmd.res32_1 = direction;
        cmd.res64_1 = feeding_rule;
        err = mpWorkQ->PostMsg(cmd);
    } else {
        LOGM_FATAL("NULL mpWorkQ or NULL p_feedingrule\n");
        return EECode_BadState;
    }

    return err;
}

EECode CTSDemuxer::SetPbLoopMode(TU32 *p_loop_mode)
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

EECode CTSDemuxer::EnableVideo(TU32 enable)
{
    LOGM_ERROR("not implemented in this module.\n");
    return EECode_NoImplement;
}

EECode CTSDemuxer::EnableAudio(TU32 enable)
{
    LOGM_ERROR("not implemented in this module.\n");
    return EECode_NoImplement;
}

void CTSDemuxer::OnRun()
{
    SCMD cmd;
    CIBuffer *pBuffer = NULL;
    mbRun = 1;

    DASSERT(1 == mbRun);
    CmdAck(EECode_OK);

    SPESInfo *pPesInfo = NULL;
    EECode err = EECode_OK;
    //DASSERT(mbFileOpened == 1);
    msState = /*(mbFileOpened == 1)? EModuleState_Preparing : */EModuleState_WaitCmd;

    TU8 flags = (mbEnableVideo ? 1 : 0) | (mbEnableAudio ? 2 : 0); //(1<<EDemuxerVideoOutputPinIndex) || (1<<EDemuxerAudioOutputPinIndex);

    while (mbRun) {
        LOGM_STATE("CTSDemuxer::OnRun(), state(%s, %d)\n", gfGetModuleStateString(msState), msState);
        switch (msState) {

            case EModuleState_Preparing:
                if ((EECode_OK != prepare())/* || (EECode_OK != getExtraData())*/) {
                    LOGM_FATAL("ts-demuxer prepare (parse stream info && get extra data) fail!\n");
                    msState = EModuleState_Error;
                    break;
                }
                //send extradata;
                //sendExtraData();
                msState = EModuleState_WaitCmd;
                break;

            case EModuleState_WaitCmd:
                mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
                processCmd(cmd);
                break;

            case EModuleState_Idle:
                //if (mbExtraDataSent == 0) sendExtraData();/////////
                if (mpWorkQ->PeekCmd(cmd)) {
                    processCmd(cmd);
                } else {
                    err = readFrame(pPesInfo, flags);
                    if (err == EECode_OK) {
                        msState = EModuleState_HasInputData;
                        DASSERT(pPesInfo);
                        mpCurBufferPool = mpBufferPool[pPesInfo->streamIndex];
                        mpCurOutputPin = mpOutputPins[pPesInfo->streamIndex];
                    } else {
                        if (mbFileEOF) {
                            msState = EModuleState_Demuxer_UpdatingContext;//EModuleState_Completed;
                        } else {
                            LOGM_ERROR("CTSDemuxer::OnRun(), switch state from IDLE to ERROR!\n");
                            msState = EModuleState_Error;
                        }
                        //sendFlowControlBuffer(EBufferType_FlowControl_EOS);
                    }
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
                DASSERT(!pBuffer);
                pBuffer = NULL;
                if (!mpCurOutputPin->AllocBuffer(pBuffer)) {
                    LOGM_FATAL("AllocBuffer() fail? must have internal bugs.\n");
                    msState = EModuleState_Error;
                    break;
                }

                if (pPesInfo->streamIndex == EDemuxerAudioOutputPinIndex) {
                    if (DUnlikely(pPesInfo->pts == 0 && mStreamFirstPTS[pPesInfo->streamIndex] != -1)) {
                        pPesInfo->pts = mStreamLastPTS[pPesInfo->streamIndex] + mAudioFakePTSNum * DDefaultTimeScale / mAudioFakePTSDen;
                    } else if (mbDebugPTSGap) {
                        pPesInfo->pts += mPTSDiff[pPesInfo->streamIndex];
                        //TODO
                        if (DUnlikely((pPesInfo->pts + 1920 * 3) <= mStreamLastPTS[pPesInfo->streamIndex])) {
                            LOGM_NOTICE("[Debug], pts diff, pPesInfo->pts %lld, new file?\n", pPesInfo->pts);
                            pPesInfo->pts += mStreamLastPTS[pPesInfo->streamIndex] - mPTSDiff[pPesInfo->streamIndex];
                            mPTSDiff[pPesInfo->streamIndex] = mStreamLastPTS[pPesInfo->streamIndex];
                            LOGM_NOTICE("[Debug], pts diff, mPTSDiff %lld, new file?\n", mPTSDiff[pPesInfo->streamIndex]);
                        }
                    }

                    if (mStreamFirstPTS[pPesInfo->streamIndex] == -1) { mStreamFirstPTS[pPesInfo->streamIndex] = pPesInfo->pts; }

                    TTime pts_fixed = pPesInfo->pts - mStreamFirstPTS[pPesInfo->streamIndex];
                    mFakeAudioPTS = pts_fixed * 1000000 / DDefaultTimeScale;
                    pBuffer->SetBufferPTS(mFakeAudioPTS);
                    mStreamLastPTS[pPesInfo->streamIndex] = pPesInfo->pts;
                    pBuffer->SetBufferNativePTS(pts_fixed);
                    pBuffer->SetBufferLinearPTS(pts_fixed * mAudioFakePTSDen / DDefaultTimeScale);

                    if (pPesInfo->dts >= 0) {
                        pBuffer->SetBufferDTS(pPesInfo->dts);
                        pBuffer->SetBufferNativeDTS(pPesInfo->dts);
                    }
                    pBuffer->SetBufferType(EBufferType_AudioES);
                    pBuffer->mCustomizedContentType = EBufferCustomizedContentType_RingBuffer;
                    pBuffer->mpCustomizedContent = (void *)mpMemPool[pPesInfo->streamIndex];
                } else if (pPesInfo->streamIndex == EDemuxerVideoOutputPinIndex) {
                    if (DUnlikely(pPesInfo->pts == 0)) {
                        pPesInfo->pts = mStreamLastPTS[pPesInfo->streamIndex] + mVideoFakePTSNum;
                    } else if (mbDebugPTSGap) {
                        pPesInfo->pts += mPTSDiff[pPesInfo->streamIndex];
                        //TODO
                        if (DUnlikely((pPesInfo->pts + 9000) <= mStreamLastPTS[pPesInfo->streamIndex])) {
                            LOGM_NOTICE("[Debug], pts diff, pPesInfo->pts %lld, new file?\n", pPesInfo->pts);
                            pPesInfo->pts += mStreamLastPTS[pPesInfo->streamIndex] - mPTSDiff[pPesInfo->streamIndex];
                            mPTSDiff[pPesInfo->streamIndex] = mStreamLastPTS[pPesInfo->streamIndex];
                            LOGM_NOTICE("[Debug], pts diff, mPTSDiff %lld, new file?\n", mPTSDiff[pPesInfo->streamIndex]);
                        }
                    }

                    if (mStreamFirstPTS[pPesInfo->streamIndex] == -1) { mStreamFirstPTS[pPesInfo->streamIndex] = pPesInfo->pts; }

                    TTime pts_fixed = pPesInfo->pts - mStreamFirstPTS[pPesInfo->streamIndex];
                    mFakeVideoPTS = pts_fixed * 1000000 / mVideoFakePTSDen;
                    pBuffer->SetBufferPTS(mFakeVideoPTS);
                    mStreamLastPTS[pPesInfo->streamIndex] = pPesInfo->pts;
                    pBuffer->SetBufferNativePTS(pts_fixed);
                    pBuffer->SetBufferLinearPTS(pts_fixed);

                    if (pPesInfo->dts >= 0) {
                        pBuffer->SetBufferDTS(pPesInfo->dts);
                        pBuffer->SetBufferNativeDTS(pPesInfo->dts);
                    }
                    pBuffer->SetBufferType(EBufferType_VideoES);
                    TU8 *pDataBase = pPesInfo->pBuffer;
                    TU8 has_sps = 0, has_pps = 0, has_idr = 0;
                    TU8 *pPps = NULL;
                    TU8 *pSps = NULL;
                    TU8 *pIdr = NULL;
                    TU8 *pPpsEnd = NULL;
                    gfFindH264SpsPpsIdr(pDataBase, pPesInfo->dataSize, has_sps, has_pps, has_idr, pSps, pPps, pPpsEnd, pIdr);
                    if (has_idr) {
                        pBuffer->SetBufferFlags(CIBuffer::WITH_EXTRA_DATA | CIBuffer::KEY_FRAME);
                    }
                    pBuffer->mCustomizedContentType = EBufferCustomizedContentType_RingBuffer;
                    pBuffer->mpCustomizedContent = (void *)mpMemPool[pPesInfo->streamIndex];
                }

                pBuffer->SetDataPtr(pPesInfo->pBuffer);
                pBuffer->SetDataSize(pPesInfo->dataSize);
                pBuffer->SetDataPtrBase(pPesInfo->pBuffer);
                pBuffer->SetDataMemorySize(pPesInfo->dataSize);
                pBuffer->mVideoFrameType = pPesInfo->frameType;

                if (pPesInfo->bufferSize > pPesInfo->dataSize) {
                    mpMemPool[pPesInfo->streamIndex]->ReturnBackMemBlock((pPesInfo->bufferSize - pPesInfo->dataSize), pPesInfo->pBuffer + pPesInfo->dataSize);
                }
                //LOG_PRINTF("Send buffer(%d), pts %lld, free count %d\n", pBuffer->GetBufferType(), pBuffer->GetBufferPTS(), mpCurOutputPin->GetBufferPool()->GetFreeBufferCnt());
                mpCurOutputPin->SendBuffer(pBuffer);
                pBuffer = NULL;
                _clear_pes_info(pPesInfo);
                pPesInfo = NULL;
                msState = EModuleState_Idle;
                break;

            case EModuleState_Completed:
            case EModuleState_Pending:
            case EModuleState_Error:
                mpWorkQ->WaitMsg(&cmd, sizeof(cmd));
                processCmd(cmd);
                break;

            case EModuleState_Demuxer_UpdatingContext: {
                    DASSERT(mbFileEOF);
                    EECode err = EECode_OK;
                    msState = EModuleState_Completed;
                    if (mpStorageManager) {
                        err = processUpdateContext(NULL);
                        if (err == EECode_OK) {
                            msState = EModuleState_Idle;
                        } else {
                            LOGM_ERROR("TS-demuxer update context fail!\n");
                        }
                    }
                }
                break;

            default:
                LOGM_ERROR("Check Me.\n");
                break;
        }
    }

    LOGM_NOTICE("GFFMpegDemuxer OnRun Exit.\n");
    CmdAck(EECode_OK);
}

EECode CTSDemuxer::SetVideoPostProcessingCallback(void *callback_context, void *callback)
{
    LOGM_FATAL("TO DO\n");
    return EECode_NoImplement;
}

EECode CTSDemuxer::SetAudioPostProcessingCallback(void *callback_context, void *callback)
{
    LOGM_FATAL("TO DO\n");
    return EECode_NoImplement;
}

void CTSDemuxer::PrintStatus()
{
}

void CTSDemuxer::EventNotify(EEventType type, TU64 param1, TPointer param2)
{
    EECode err = EECode_OK;
    DASSERT(mpWorkQ);

    LOGM_FLOW("before mpWorkQ->PostMsg(ECMDType_NotifyBufferRelease)...\n");
    err = mpWorkQ->PostMsg(ECMDType_NotifyBufferRelease, NULL);
    LOGM_FLOW("after mpWorkQ->PostMsg(ECMDType_NotifyBufferRelease), ret %d\n", err);

    DASSERT_OK(err);

    return;
}

EECode CTSDemuxer::QueryContentInfo(const SStreamCodecInfos *&pinfos) const
{
    pinfos = (const SStreamCodecInfos *)&mStreamCodecInfos;
    return EECode_OK;
}

EECode CTSDemuxer::UpdateContext(SContextInfo *pContext)
{
    EECode err = EECode_OK;
    DASSERT(mpWorkQ);

    LOGM_FLOW("before mpWorkQ->SendCmd(ECMDType_UpdateUrl)...\n");
    err = mpWorkQ->SendCmd(ECMDType_UpdateUrl, (void *)pContext);
    LOGM_FLOW("after mpWorkQ->SendCmd(ECMDType_UpdateUrl), ret %d\n", err);

    DASSERT_OK(err);

    return err;
}

EECode CTSDemuxer::GetExtraData(SStreamingSessionContent *pContent)
{
    //DASSERT(pVideo);
    //DASSERT(pAudio);
    DASSERT(mbFileOpened == 1);

    if (EECode_OK != prepare()) {
        LOGM_FATAL("parse extra data fail!\n");
        return EECode_Error;
    }

    if (EECode_OK != getExtraData(pContent)) {
        LOGM_FATAL("parse extra data fail!\n");
        return EECode_Error;
    }

    return EECode_OK;
}

EECode CTSDemuxer::NavigationSeek(SContextInfo *info)
{
    EECode err = EECode_OK;
    DASSERT(mpWorkQ);

    LOGM_NOTICE("before mpWorkQ->SendCmd(ECMDType_NavigationSeek)...\n");
    err = mpWorkQ->SendCmd(ECMDType_NavigationSeek, (void *)info);
    LOGM_NOTICE("after mpWorkQ->SendCmd(ECMDType_NavigationSeek), ret %d\n", err);

    DASSERT_OK(err);

    return err;
}

EECode CTSDemuxer::ResumeChannel()
{
    EECode err = EECode_OK;
    DASSERT(mpWorkQ);

    LOGM_NOTICE("before mpWorkQ->SendCmd(ECMDType_ResumeChannel)...\n");
    err = mpWorkQ->SendCmd(ECMDType_ResumeChannel, NULL);
    LOGM_NOTICE("after mpWorkQ->SendCmd(ECMDType_ResumeChannel), ret %d\n", err);

    DASSERT_OK(err);

    return err;
}

/////////////////////////////////////////////////////////////////////////////////
EECode CTSDemuxer::processCmd(SCMD &cmd)
{
    EECode err = EECode_OK;

    switch (cmd.code) {
        case ECMDType_ExitRunning:
            mbRun = 0;
            break;

        case ECMDType_Start:
            DASSERT(EModuleState_WaitCmd == msState);
            //msState = (mbFileOpened == 1)? EModuleState_Idle : EModuleState_WaitCmd;
            msState = EModuleState_Pending;
            LOGM_FLOW("ECMDType_Start comes\n");
            CmdAck(EECode_OK);
            break;

        case ECMDType_Stop:
            mbTobeStopped = 1;
            CmdAck(EECode_OK);
            break;

        case ECMDType_Pause:
            mbPaused = 1;
            msState = EModuleState_Pending;
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

        case ECMDType_NotifyBufferRelease:
            if (mpCurBufferPool && mpCurBufferPool->GetFreeBufferCnt() > 0 && msState == EModuleState_HasInputData) {
                msState = EModuleState_Running;
            }
            break;

        case ECMDType_UpdatePlaybackSpeed:
            break;

        case ECMDType_UpdatePlaybackLoopMode:
            break;

        case ECMDType_UpdateUrl:
            if (processUpdateContext((SContextInfo *)cmd.pExtra) != EECode_OK) {
                LOGM_ERROR("processUpdateContext fail\n");
                msState = EModuleState_Error;
                CmdAck(EECode_Error);
            } else {
                msState = mbExtraDataSent ? EModuleState_Idle : EModuleState_Preparing;
                CmdAck(EECode_OK);
            }
            break;

        case ECMDType_NavigationSeek:
            err = processNavigation((SContextInfo *)cmd.pExtra);
            if (DUnlikely(EECode_OK != err)) {
                LOGM_ERROR("processNavigation fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
            }
            msState = EModuleState_Pending;
            CmdAck(err);
            break;

        case ECMDType_ResumeChannel:
            msState = EModuleState_Idle;
            CmdAck(err);
            break;

        default:
            break;
    }

    return err;
}

EECode CTSDemuxer::prepare()
{
    DASSERT(0 == mDataRemainSize);

    TU8 patFind = 0;
    TU8 pmtFind = 0;
    TU8 priPES = 1;//if has
    MPEG_TS_TP_HEADER *pTsHeader = NULL;
    TU8 *pTsPacketStart = NULL;
    TU8 *pTsPacketEnd = NULL;
    TU8 *pTsPayload = NULL;

    while (1) {
        if (!(pTsPacketStart = syncPacket())) {
            LOGM_FATAL("CTSDemuxer::prepare(), syncPacket fail!\n");
            break;
        }
        pTsPacketEnd = pTsPacketStart + mTsPacketSize;
        pTsHeader = (MPEG_TS_TP_HEADER *)pTsPacketStart;
        DASSERT(pTsHeader->sync_byte == MPEG_TS_TP_SYNC_BYTE);

        TU8 adaption_field_length = 0;
        if (pTsHeader->payload_unit_start_indicator != 0x01) {// 0x01 pes or psi
            mDataRemainSize -= mTsPacketSize;
            continue;
        }

        if (pTsHeader->adaptation_field_control == MPEG_TS_ADAPTATION_FIELD_PAYLOAD_ONLY) {
            pTsPayload = pTsPacketStart + 5;
        } else if (pTsHeader->adaptation_field_control == MPEG_TS_ADAPTATION_FIELD_BOTH) {
            adaption_field_length = pTsPacketStart[4];
            pTsPayload = pTsPacketStart + 5 + adaption_field_length;
        }

        TUint pid = (pTsHeader->pid_12to8 << 8) | pTsHeader->pid_7to0;
        if (!patFind && pid == MPEG_TS_PAT_PID) {//PAT
            if (pTsPayload >= pTsPacketEnd) {
                LOGM_FATAL("Check Me!\n");
                break;
            }
            PAT_HDR *pPAT = (PAT_HDR *)pTsPayload;
            TUint sectionLength = ((pPAT->section_length8to11 << 8) | (pPAT->section_length0to7)) + 3;
            TUint programLength = sectionLength - PAT_HDR_SIZE - 4;// 4: length of crc
            mProgramCount = programLength / PAT_ELEMENT_SIZE;
            PAT_ELEMENT *pPatElement = (PAT_ELEMENT *)(pTsPayload + PAT_HDR_SIZE);
            mPmtPID = ((pPatElement->program_map_PID_h << 8) | pPatElement->program_map_PID_l);
            LOGM_NOTICE("Find PAT and PMT PID 0x%x\n", mPmtPID);
            patFind = 1;
        } else if (patFind && !pmtFind && pid == mPmtPID) {
            PMT_HDR *pPMT = (PMT_HDR *)pTsPayload;
            TUint sectionLength = ((pPMT->section_length8to11 << 8) | (pPMT->section_length0to7)) + 3;
            TUint programLength = sectionLength - PMT_HDR_SIZE - 4;// 4: length of crc
            PMT_ELEMENT *pPmtElement = NULL;
            TU8 streamType = 0;
            for (TUint i = 0; i < programLength;) {
                pPmtElement = (PMT_ELEMENT *)(pTsPayload + PMT_HDR_SIZE + i);
                streamType = pPmtElement->stream_type;
                switch (streamType) {
                    case MPEG_SI_STREAM_TYPE_AAC:
                        msAudioStream.enable = 1;
                        msAudioStream.stream_type = MPEG_SI_STREAM_TYPE_AAC;
                        msAudioPES.streamIndex = EDemuxerAudioOutputPinIndex;
                        msAudioStream.elementary_PID = (pPmtElement->elementary_PID8to12 << 8) | (pPmtElement->elementary_PID0to7);
                        i += PMT_ELEMENT_SIZE;
                        mStreamCount++;
                        pmtFind = 1;
                        LOGM_NOTICE("Has AAC Stream!\n");
                        break;

                    case MPEG_SI_STREAM_TYPE_AVC_VIDEO:
                        msVideoStream.enable = 1;
                        msVideoStream.stream_type = MPEG_SI_STREAM_TYPE_AVC_VIDEO;
                        msVideoPES.streamIndex = EDemuxerVideoOutputPinIndex;
                        msVideoStream.elementary_PID = (pPmtElement->elementary_PID8to12 << 8) | (pPmtElement->elementary_PID0to7);
                        i += PMT_ELEMENT_SIZE;
                        mStreamCount++;
                        pmtFind = 1;
                        LOGM_NOTICE("Has AVC Stream!\n");
                        break;

                    case MPEG_SI_STREAM_TYPE_PRIVATE_SECTION:
                        //todo
                        //parse private section
                        i += PMT_ELEMENT_SIZE;
                        LOGM_NOTICE("TODO: Parse Private Section\n");
                        break;

                    case MPEG_SI_STREAM_TYPE_PRIVATE_PES:
                        priPES = 0;//read one more packet to parse private pes info
                        msPrivatePES.enable = 1;
                        msPrivatePES.stream_type = MPEG_SI_STREAM_TYPE_PRIVATE_PES;
                        msPrivatePES.elementary_PID = (pPmtElement->elementary_PID8to12 << 8) | (pPmtElement->elementary_PID0to7);
                        i += PMT_ELEMENT_SIZE;
                        mStreamCount++;
                        LOGM_NOTICE("Has Private PES!\n");
                        break;

                    default:
                        i++;
                        LOGM_NOTICE("Find new stream type: 0x%x!Check Me!\n", streamType);
                        break;
                }
            }

            //debug
            if (msAudioStream.enable || msVideoStream.enable) {
                //pmtFind = 1;
                LOGM_NOTICE("msAudioStream.elementary_PID 0x%x, msVideoStream.elementary_PID 0x%x\n", msAudioStream.elementary_PID, msVideoStream.elementary_PID);
            } else {
                LOGM_ERROR("Check Me!\n");
            }
        } else if (msPrivatePES.enable == 1) {
            //parse private pes
            if (pid == msPrivatePES.elementary_PID) {
                parsePrivatePacket(msPrivatePES.stream_type, pTsPayload);
            } else {
                LOGM_FATAL("No Private PES packet!! PID: 0x%x, Private PID 0x%x\n", pid, msPrivatePES.elementary_PID);
                msPrivatePES.enable = 0;
            }
            priPES = 1;
        }
        mDataRemainSize -= mTsPacketSize;

        //check quit
        if (patFind && pmtFind && priPES) {
            //prepare done, normal quit
            break;
        }
    }

    return (patFind && pmtFind) ? EECode_OK : EECode_Error;
}

EECode CTSDemuxer::readPacket()
{
    if (mbFileEOF) {
        LOGM_NOTICE("CTSDemuxer::readPacket(), has been EOF!\n");
        return EECode_Error;
    }

    EECode err = EECode_OK;
    TUint bufferRemainSize = mBufferSize - mDataRemainSize;
    TIOSize tryreadCount = bufferRemainSize;

    if (mDataRemainSize) {
        memmove(mpBufferAligned, mpBufferAligned + bufferRemainSize, mDataRemainSize);
    }

    if ((err = mpIO->Read((TU8 *)(mpBufferAligned + mDataRemainSize), 1, (TIOSize &)tryreadCount)) != EECode_OK) {
        LOGM_ERROR("CTSDemuxer::readPacket(), read file fail!\n");
        return err;
    }
    if (tryreadCount < bufferRemainSize) {
        LOGM_NOTICE("CTSDemuxer::readPacket(), eof now! tryreadCount %lld, bufferRemainSize %u\n", tryreadCount, bufferRemainSize);
        mbFileEOF = 1;
    }
    mDataRemainSize += tryreadCount;
    if (mDataRemainSize < mTsPacketSize) {
        if (mbFileEOF) {
            LOGM_ERROR("CTSDemuxer::readPacket(), the last packet size is %u (< 188), invalid packet!\n", mDataRemainSize);
        } else {
            LOGM_ERROR("CTSDemuxer::readPacket(), Check Me!\n");
        }
        err = EECode_Error;
    }
    mDataSize = mDataRemainSize;
    return err;
}

TU8 *CTSDemuxer::syncPacket()
{
    TU8 *pBufferSynced = NULL;
    TU8 *pBufferStart = NULL;

    while (!pBufferSynced || (mDataRemainSize < mTsPacketSize)) {
        if (mDataRemainSize < mTsPacketSize) {
            if (readPacket() != EECode_OK) {
                if (mbFileEOF != 1) { LOGM_ERROR("CTSDemuxer::syncPacket(), readPacket fail!\n"); }
                break;
            }
            continue;
        }
        pBufferStart = mpBufferAligned + mDataSize - mDataRemainSize;
        pBufferSynced = _ts_packet_start(pBufferStart, mDataRemainSize);
    }

    return pBufferSynced;
}

EECode CTSDemuxer::readESData(MPEG_TS_TP_HEADER *pTsHeader, SPESInfo *pPesInfo, TU8 *pPayload)
{
    if (!pTsHeader || !pPesInfo || !pPayload) {
        LOGM_ERROR("CTSDemuxer::readESPacket(), NULL input!\n");
        return EECode_BadParam;
    }

    TUint totalPesHeaerLength = 0;

    if (pTsHeader->payload_unit_start_indicator == 0x01) {
        MPEG_TS_TP_PES_HEADER *pPesHeader = (MPEG_TS_TP_PES_HEADER *)pPayload;
        DASSERT(pPesHeader->packet_start_code_23to16 == 0x00);
        DASSERT(pPesHeader->packet_start_code_15to8 == 0x00);
        DASSERT(pPesHeader->packet_start_code_7to0 == 0x01);
        if (pPesInfo->pBuffer) {
            DASSERT(pPesInfo->pesLength == 0);
            DASSERT(pPesInfo->dataSize > 0);
            //LOGM_NOTICE("Get one Frame: frame size %u, buffer size %u\n", pPesInfo->dataSize, pPesInfo->bufferSize);

            if (pPesInfo->dataSize > pPesInfo->bufferSize) {
                //LOGM_NOTICE("Get one Frame: frame size %u, buffer size %u, pes length %u, Lost one pes header?\n", pPesInfo->dataSize, pPesInfo->bufferSize, pPesInfo->pesLength);
            }
            return EECode_OK;
        } else {
            pPesInfo->pesLength = (pPesHeader->pes_packet_length_15to8 << 8) | pPesHeader->pes_packet_length_7to0;
            if (pPesHeader->pts_dts_flags == 0x02) {
                DASSERT(pPesHeader->header_data_length == PTS_FIELD_SIZE);
                pPesInfo->pts = _get_ptsordts(pPayload + PES_HEADER_LEN);
            } else if (pPesHeader->pts_dts_flags == 0x03) {
                DASSERT(pPesHeader->header_data_length == PTS_FIELD_SIZE + DTS_FIELD_SIZE);
                pPesInfo->pts = _get_ptsordts(pPayload + PES_HEADER_LEN);
                pPesInfo->dts = _get_ptsordts(pPayload + PES_HEADER_LEN + PTS_FIELD_SIZE);
            }
            //check pts
            //if (pPesInfo->pts < mStreamLastPTS[pPesInfo->streamIndex]) {
            //  mDataRemainSize -= mTsPacketSize;
            //  _clear_pes_info(pPesInfo);
            //  return EECode_TryAgain;
            //}
            totalPesHeaerLength = PES_HEADER_LEN +  pPesHeader->header_data_length;
            if (pPesInfo->pesLength > 0) {
                pPesInfo->bufferSize = pPesInfo->pesLength - totalPesHeaerLength + 6;
                pPesInfo->pBuffer = mpMemPool[pPesInfo->streamIndex]->RequestMemBlock(pPesInfo->bufferSize);
            } else {
                pPesInfo->pBuffer = mpMemPool[pPesInfo->streamIndex]->RequestMemBlock(512 * 1024); //hardcode
                pPesInfo->bufferSize = 512 * 1024;
            }
            DASSERT(pPesInfo->pBuffer);
            pPesInfo->dataSize = 0;
        }
        pPayload += totalPesHeaerLength;
    }

    //mDataRemainSize -= mTsPacketSize;
    if (!pPesInfo->pBuffer) {
        LOGM_NOTICE("lost pes header?!\n");
        mDataRemainSize -= mTsPacketSize;
        return EECode_TryAgain;
    }
    TU8 *pTsPacketEnd = (TU8 *)pTsHeader + mTsPacketSize; //188
    DASSERT(pTsPacketEnd > pPayload);
    TUint dataLength = pTsPacketEnd - pPayload;
    memcpy(pPesInfo->pBuffer + pPesInfo->dataSize, pPayload, dataLength);
    pPesInfo->dataSize += dataLength;

    if (pPesInfo->pesLength > 0) {
        if (pPesInfo->dataSize >= pPesInfo->bufferSize) {
            LOGM_DEBUG("Get one Frame: frame size %u, buffer size %u\n", pPesInfo->dataSize, pPesInfo->bufferSize);
            mDataRemainSize -= mTsPacketSize;
            return EECode_OK;
        }
    }
    mDataRemainSize -= mTsPacketSize;
    return EECode_TryAgain;
}

EECode CTSDemuxer::readStream(TU8 type, SPESInfo *&pPesInfo, TU8 flags)
{
    EECode err = EECode_Error;
    MPEG_TS_TP_HEADER *pTsHeader = NULL;
    TU8 *pTsPacketStart = NULL;
    //TU8* pTsPacketEnd = NULL;
    TU8 *pTsPayload = NULL;

    TU8 enableVideo = flags & (1 << EDemuxerVideoOutputPinIndex);
    TU8 enableAudio = flags & (1 << EDemuxerAudioOutputPinIndex);

    while ((pTsPacketStart = syncPacket())) {
        //LOG_PRINTF("[Debug]: 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x\n", pTsPacketStart[0], pTsPacketStart[1], pTsPacketStart[2], pTsPacketStart[3], pTsPacketStart[48], pTsPacketStart[49], pTsPacketStart[50], pTsPacketStart[51]);
        //pTsPacketEnd = pTsPacketStart + mTsPacketSize;
        pTsHeader = (MPEG_TS_TP_HEADER *)pTsPacketStart;
        DASSERT(pTsHeader->sync_byte == MPEG_TS_TP_SYNC_BYTE);

        TU8 adaption_field_length = 0;
        if (pTsHeader->adaptation_field_control == MPEG_TS_ADAPTATION_FIELD_BOTH) {
            adaption_field_length = pTsPacketStart[4] + 1;
        }
        pTsPayload =  pTsPacketStart + sizeof(MPEG_TS_TP_HEADER) + adaption_field_length;

        TUint PID = (pTsHeader->pid_12to8 << 8) | pTsHeader->pid_7to0;
        if (type == MPEG_SI_STREAM_TYPE_PRIVATE_PES) {
            if (PID != msPrivatePES.elementary_PID) {
                mDataRemainSize -= mTsPacketSize;
                continue;
            }
            //read private pes
            pPesInfo->pBuffer = pTsPayload;
            pPesInfo->dataSize = mTsPacketSize - (sizeof(MPEG_TS_TP_HEADER) + adaption_field_length);
            mDataRemainSize -= mTsPacketSize;
            return EECode_OK;
        } else if (PID != msVideoStream.elementary_PID && PID != msAudioStream.elementary_PID) {
            mDataRemainSize -= mTsPacketSize;
            continue;
        }

        if (PID == msVideoStream.elementary_PID) {
            if (enableVideo) {
                pPesInfo = &msVideoPES;
            } else {
                mDataRemainSize -= mTsPacketSize;
                continue;
            }
        } else if (PID == msAudioStream.elementary_PID) {
            if (enableAudio) {
                pPesInfo = &msAudioPES;
            } else {
                mDataRemainSize -= mTsPacketSize;
                continue;
            }
        } else {
            LOGM_ERROR("Check Me!\n");
            pPesInfo = NULL;
            break;
        }
        err = readESData(pTsHeader, pPesInfo, pTsPayload);
        if (err == EECode_OK) {
            return err;
        } else if (err == EECode_BadParam) {
            LOGM_ERROR("Check Me!\n");
            return err;
        }
    }
    if (mbFileEOF) {
        if (err == EECode_TryAgain) {
            //last frame
            err = EECode_OK;
        }
    } else {
        LOGM_ERROR("CTSDemuxer::readFrame() fail!\n");
    }
    return err;
}

EECode CTSDemuxer::readFrame(SPESInfo *&pPesInfo, TU8 flags)
{

    TU8 type = msVideoStream.stream_type | msAudioStream.stream_type;
    return readStream(type, pPesInfo, flags);
}

EECode CTSDemuxer::freeFrame(SPESInfo *pPesInfo)
{
    DASSERT(pPesInfo);
    if (pPesInfo->pBuffer) {
        mpMemPool[pPesInfo->streamIndex]->ReturnBackMemBlock(pPesInfo->bufferSize, pPesInfo->pBuffer);
        pPesInfo->pBuffer = NULL;
    }
    _clear_pes_info(pPesInfo);
    return EECode_OK;
}

EECode CTSDemuxer::setupContext(TChar *url)
{
    DASSERT(mpIO);
    if (!url) {
        LOGM_FATAL("NULL url in CTSDemuxer::SetupContext!\n");
        return EECode_BadParam;
    }

    destroyContext();

    if (mpIO->Open(url, EIOFlagBit_Read | EIOFlagBit_Binary) != EECode_OK) {
        LOGM_ERROR("CTSDemuxer::setupContext open file fail!\n");
        return EECode_Error;
    }
    mbFileOpened = 1;

    return EECode_OK;
}

EECode CTSDemuxer::destroyContext()
{
    if (mbFileOpened) {
        mpIO->Close();
        mbFileOpened = 0;
    }

    freeFrame(&msVideoPES);
    freeFrame(&msAudioPES);

    memset(mpBufferAligned, 0x0, mBufferSize);
    mDataSize = 0;
    mDataRemainSize = 0;
    mbFileEOF = 0;

    return EECode_OK;
}

EECode CTSDemuxer::getExtraData(SStreamingSessionContent *pContent)
{
    TIOSize fileTotalSize, curPosition1, curPosition2;
    mpIO->Query(fileTotalSize, curPosition1);
    mFileTotalSize = (TS64)fileTotalSize;
    TUint remainSize = mDataRemainSize;

    SPESInfo *pPesInfo = NULL;
    SADTSFixedHeader *p_adts_header = NULL;
    TU8 flags = 0;
    TU8 bVideo = (mbEnableVideo && msVideoStream.enable) ? 1 : 0;
    TU8 bAudio = (mbEnableAudio && msAudioStream.enable) ? 1 : 0;
    EECode err = EECode_OK;

    LOGM_INFO("bVideo %u, bAudio %u\n", bVideo, bAudio);

    do {
        flags = (bVideo ? 1 : 0) | (bAudio ? 2 : 0);
        err = readFrame(pPesInfo, flags);
        if (err == EECode_OK) {
            if (pPesInfo->streamIndex == EDemuxerVideoOutputPinIndex) {

                TU8 *pDataBase = pPesInfo->pBuffer;
                TU8 has_sps = 0, has_pps = 0, has_idr = 0;
                TU8 *pPps = NULL;
                TU8 *pSps = NULL;
                TU8 *pIdr = NULL;
                TU8 *pPpsEnd = NULL;
                gfFindH264SpsPpsIdr(pDataBase, pPesInfo->bufferSize, has_sps, has_pps, has_idr, pSps, pPps, pPpsEnd, pIdr);
                if (has_sps && has_pps) {
                    TU32 extraDataSize = pPpsEnd - pSps;
                    mpVideoExtraData = (TU8 *)DDBG_MALLOC(extraDataSize + 4, "dmVE");
                    memcpy(mpVideoExtraData, pSps, extraDataSize);
                    mpVideoExtraData[extraDataSize] = '\0';
                    mVideoExtraDataSize = extraDataSize;
                    bVideo = 0;
                    LOGM_INFO("get video\n");
                }
            } else if (pPesInfo->streamIndex == EDemuxerAudioOutputPinIndex) {

                TU8 *pDataBase = pPesInfo->pBuffer;
                p_adts_header = (SADTSFixedHeader *)pDataBase;
                SSimpleAudioSpecificConfig *p_header = (SSimpleAudioSpecificConfig *)DDBG_MALLOC(sizeof(SSimpleAudioSpecificConfig), "dmAE");
                DASSERT(p_header);

                //check sync word
                TUint sync_word = (p_adts_header->syncword_11to4 << 4) | (p_adts_header->syncword_3to0);
                DASSERT(sync_word == 0xFFF);

                if (p_adts_header->profile == 0) {
                    p_header->audioObjectType = eAudioObjectType_AAC_MAIN;
                } else if (p_adts_header->profile == 1) {
                    p_header->audioObjectType = eAudioObjectType_AAC_LC;
                } else if (p_adts_header->profile == 2) {
                    p_header->audioObjectType = eAudioObjectType_AAC_SSR;
                } else if (p_adts_header->profile == 3) {
                    LOGM_WARN("CTSDemuxer::getExtraData, aac profile is 'reserved'?\n");
                }

                TU8 samplerate = 0;
                switch (aac_sample_rates[p_adts_header->sampling_frequency_index]) {
                    case 44100:
                        samplerate = eSamplingFrequencyIndex_44100;
                        break;
                    case 48000:
                        samplerate = eSamplingFrequencyIndex_48000;
                        break;
                    case 24000:
                        samplerate = eSamplingFrequencyIndex_24000;
                        break;
                    case 16000:
                        samplerate = eSamplingFrequencyIndex_16000;
                        break;
                    case 8000:
                        samplerate = eSamplingFrequencyIndex_8000;
                        break;
                    case 12000:
                        samplerate = eSamplingFrequencyIndex_12000;
                        break;
                    case 32000:
                        samplerate = eSamplingFrequencyIndex_32000;
                        break;
                    case 22050:
                        samplerate = eSamplingFrequencyIndex_22050;
                        break;
                    case 11025:
                        samplerate = eSamplingFrequencyIndex_11025;
                        break;
                    default:
                        LOGM_FATAL("ADD support here, audio sample rate %d.\n", aac_sample_rates[p_adts_header->sampling_frequency_index]);
                        break;
                }

                p_header->samplingFrequencyIndex_high = samplerate >> 1;
                p_header->samplingFrequencyIndex_low = samplerate & 0x1;
                p_header->channelConfiguration = (p_adts_header->channel_configuration_2 << 2) | p_adts_header->channel_configuration_1to0;
                p_header->bitLeft = 0;
                mpAudioExtraData = (TU8 *)p_header;
                mAudioExtraDataSize = sizeof(SSimpleAudioSpecificConfig);
                bAudio = 0;
                LOGM_INFO("get audio\n");
            }
            freeFrame(pPesInfo);
        } else {
            LOGM_FATAL("readFrame fail\n");
            break;
        }
    } while ((bVideo || bAudio) && err == EECode_OK);

    if (DUnlikely(!mpAudioExtraData)) {
        pContent->has_audio = 0;
        pContent->sub_session_content[ESubSession_audio]->enabled = 0;
    } else {
        err = gfSDPProcessAudioExtraData(pContent->sub_session_content[ESubSession_audio], mpAudioExtraData, mAudioExtraDataSize);
        if (DUnlikely(EECode_OK != err)) {
            LOGM_FATAL("process audio extradata fail!\n");
        } else {
            pContent->sub_session_content[ESubSession_audio]->enabled = 1;
            pContent->sub_session_content[ESubSession_audio]->format = _parse_stream_type(msAudioStream.stream_type);
            pContent->sub_session_content[ESubSession_audio]->audio_channel_number = (p_adts_header->channel_configuration_2 << 2) | p_adts_header->channel_configuration_1to0;
            pContent->sub_session_content[ESubSession_audio]->audio_frame_size = mAudioFakePTSNum;
            pContent->sub_session_content[ESubSession_audio]->audio_sample_format = AudioSampleFMT_S16;
            pContent->sub_session_content[ESubSession_audio]->audio_sample_rate = aac_sample_rates[p_adts_header->sampling_frequency_index];
        }
    }

    if (DUnlikely(!mpVideoExtraData)) {
        pContent->has_video = 0;
        pContent->sub_session_content[ESubSession_video]->enabled = 0;
    } else {
        err = gfSDPProcessVideoExtraData(pContent->sub_session_content[ESubSession_video], mpVideoExtraData, mVideoExtraDataSize);
        if (DUnlikely(EECode_OK != err)) {
            LOGM_FATAL("process video extradata fail!\n");
        } else {
            pContent->sub_session_content[ESubSession_video]->enabled = 1;
            pContent->sub_session_content[ESubSession_video]->format = _parse_stream_type(msVideoStream.stream_type);
            pContent->sub_session_content[ESubSession_video]->video_framerate_den = mVideoFakePTSNum;
            pContent->sub_session_content[ESubSession_video]->video_framerate_num = mVideoFakePTSDen;
        }
    }

    //seek to start
    mpIO->Query(fileTotalSize, curPosition2);
    if (DUnlikely(curPosition2 != curPosition1)) {
        err = mpIO->Seek((TIOSize)(curPosition1 - remainSize), EIOPosition_Begin);
        mDataRemainSize = 0;
    } else {
        mDataRemainSize = remainSize;
    }
    return err;
}

EECode CTSDemuxer::sendExtraData()
{
    CIBuffer *pBuffer = NULL;

    if (mbEnableVideo && mpVideoExtraData) {
        mpCurOutputPin = mpOutputPins[EDemuxerVideoOutputPinIndex];
        if (!mpCurOutputPin->AllocBuffer(pBuffer)) {
            LOGM_FATAL("AllocBuffer() fail? must have internal bugs.\n");
            return EECode_Error;
        }

        pBuffer->SetBufferType(EBufferType_VideoExtraData);
        pBuffer->mCustomizedContentType = EBufferCustomizedContentType_Invalid;
        pBuffer->SetDataPtr(mpVideoExtraData);
        pBuffer->SetDataSize(mVideoExtraDataSize);
        mpCurOutputPin->SendBuffer(pBuffer);
        mpCurOutputPin = NULL;
        pBuffer = NULL;
    }

    if (mbEnableAudio && mpAudioExtraData) {
        mpCurOutputPin = mpOutputPins[EDemuxerAudioOutputPinIndex];
        if (!mpCurOutputPin->AllocBuffer(pBuffer)) {
            LOGM_FATAL("AllocBuffer() fail? must have internal bugs.\n");
            return EECode_Error;
        }

        pBuffer->SetBufferType(EBufferType_AudioExtraData);
        pBuffer->mCustomizedContentType = EBufferCustomizedContentType_Invalid;
        pBuffer->SetDataPtr(mpAudioExtraData);
        pBuffer->SetDataSize(mAudioExtraDataSize);
        mpCurOutputPin->SendBuffer(pBuffer);
        mpCurOutputPin = NULL;
        pBuffer = NULL;
    }
    mbExtraDataSent = 1;
    LOGM_NOTICE("sendExtraData done\n");
    return EECode_OK;
}

EECode CTSDemuxer::processUpdateContext(SContextInfo *pContext)
{
    DASSERT(pContext || mpCurUrl);

    SDateTime startTime;
    TU16 fileDuration = 0;

    TChar *pPreUrl = mpCurUrl;
    if (pContext) {
        DASSERT(pContext->pChannelName);
        if (!mpChannelName) {
            mChannelNameSize = strlen(pContext->pChannelName) + 4;
            mpChannelName = (TChar *)DDBG_MALLOC(mChannelNameSize, "dmcn");
            if (!mpChannelName) {
                return EECode_NoMemory;
            }
            snprintf(mpChannelName, mChannelNameSize, "%s", pContext->pChannelName);
            mpChannelName[mChannelNameSize - 4] = '\0';
        }

        LOGM_NOTICE("reqeust time: %u, %u, %u, %u, %u, %u\n", pContext->starttime->year, pContext->starttime->month, pContext->starttime->day, pContext->starttime->hour, pContext->starttime->minute, pContext->starttime->seconds);
        if (mpStorageManager->RequestExistUint(mpChannelName, pContext->starttime, mpCurUrl, &startTime, fileDuration) != EECode_OK) {
            LOGM_ERROR("RequestExistUint fail!\n");
            return EECode_InternalLogicalBug;
        }
        LOGM_NOTICE("start time: %u, %u, %u, %u, %u, %u\n", startTime.year, startTime.month, startTime.day, startTime.hour, startTime.minute, startTime.seconds);

        for (TInt i = 0; i < EConstMaxDemuxerMuxerStreamNumber; i++) {
            mStreamLastPTS[i] = 0;
            mStreamFirstPTS[i] = -1;
            mPTSDiff[i] = 0;
        }

    } else if (mpCurUrl) {
        TChar *pNextUrl = NULL;
        if (mpStorageManager->RequestExistUintSequentially(mpChannelName, mpCurUrl, 0,  pNextUrl, &startTime, fileDuration) != EECode_OK) {
            LOGM_ERROR("RequestExistUintSequentially(%s) fail!\n", mpCurUrl);
            return EECode_InternalLogicalBug;
        }
        mpCurUrl = pNextUrl;
    } else {
        LOGM_ERROR("Check Me!\n");
        return EECode_InternalLogicalBug;
    }

    if (pPreUrl) {
        if (EECode_OK != mpStorageManager->ReleaseExistUint(mpChannelName, pPreUrl)) {
            LOGM_ERROR("ReleaseExistUint(%s) fail!\n", pPreUrl);
            return EECode_InternalLogicalBug;
        }
        pPreUrl = NULL;
    }

    LOGM_NOTICE("mpCurUrl %s\n", mpCurUrl);

    if (setupContext(mpCurUrl) != EECode_OK) {
        return EECode_InternalLogicalBug;
    }

    if (EECode_OK != prepare()) {
        return EECode_InternalLogicalBug;
    }

    if (pContext) {
        if (EECode_OK != seekToRequestTime((const SDateTime *)&startTime, (const SDateTime *)(pContext->starttime))) {
            LOGM_ERROR("seekToRequestTime fail!\n");
            return EECode_InternalLogicalBug;
        }
    }

    return EECode_OK;
}

EECode CTSDemuxer::processNavigation(SContextInfo *pContext)
{
    DASSERT(pContext || mpCurUrl);

    SDateTime startTime;
    TU16 fileDuration = 0;

    if (mpCurUrl) {
        if (EECode_OK != mpStorageManager->ReleaseExistUint(mpChannelName, mpCurUrl)) {
            LOGM_ERROR("ReleaseExistUint(%s) fail!\n", mpCurUrl);
            return EECode_InternalLogicalBug;
        }
        mpCurUrl = NULL;
    }

    DASSERT(pContext->pChannelName);
    if (!mpChannelName) {
        mChannelNameSize = strlen(pContext->pChannelName) + 4;
        mpChannelName = (TChar *)DDBG_MALLOC(mChannelNameSize, "cMcn");
        if (!mpChannelName) {
            return EECode_NoMemory;
        }
        snprintf(mpChannelName, mChannelNameSize, "%s", pContext->pChannelName);
        mpChannelName[mChannelNameSize - 4] = '\0';
    }

    LOGM_NOTICE("reqeust time: %u, %u, %u, %u, %u, %u\n", pContext->targettime->year, pContext->targettime->month, pContext->targettime->day, pContext->targettime->hour, pContext->targettime->minute, pContext->targettime->seconds);
    if (mpStorageManager->RequestExistUint(mpChannelName, pContext->targettime, mpCurUrl, &startTime, fileDuration) != EECode_OK) {
        LOGM_ERROR("RequestExistUint fail!\n");
        return EECode_InternalLogicalBug;
    }
    LOGM_NOTICE("start time: %u, %u, %u, %u, %u, %u\n", startTime.year, startTime.month, startTime.day, startTime.hour, startTime.minute, startTime.seconds);

    if (DUnlikely(!mpCurUrl)) {
        LOGM_ERROR("do not have file\n");
        return EECode_NotExist;
    }
    LOGM_NOTICE("mpCurUrl %s\n", mpCurUrl);

    if (setupContext(mpCurUrl) != EECode_OK) {
        return EECode_InternalLogicalBug;
    }

    if (EECode_OK != seekToRequestTime((const SDateTime *)&startTime, (const SDateTime *)(pContext->targettime))) {
        LOGM_ERROR("seekToRequestTime fail!\n");
        return EECode_InternalLogicalBug;
    }

    TTime time = 0;
    EECode err = getFirstTime(time);
    if (DLikely(EECode_OK == err)) {
        pContext->current_time = (time - mStreamFirstPTS[0]) * 100 / 9;
    }
    //LOG_DEBUG("pContext->current_time %lld, mStreamFirstPTS[0] %lld, err %d\n", pContext->current_time, mStreamFirstPTS[0], err);

    return EECode_OK;
}

EECode CTSDemuxer::parsePrivatePacket(TU8 type, TU8 *pData)
{
    STSPrivateDataPesPacketHeader *pPriPesHeader = (STSPrivateDataPesPacketHeader *)pData;
    DASSERT(pPriPesHeader->start_code_prefix_23to16 == 0x00);
    DASSERT(pPriPesHeader->start_code_prefix_15to8 == 0x00);
    DASSERT(pPriPesHeader->start_code_prefix_7to0 == 0x01);
    TU16 packetLength = (pPriPesHeader->packet_len_15to8 << 8) | (pPriPesHeader->packet_len_7to0);
    TU8 *pPayload = pData + sizeof(STSPrivateDataPesPacketHeader);

    switch (type) {
        case MPEG_SI_STREAM_TYPE_PRIVATE_PES: {
                STSPrivateDataHeader *pPriDataHeader = NULL;
                TU16 length = 0;
                for (TU16 i = 0; i < packetLength;) {
                    pPriDataHeader = (STSPrivateDataHeader *)(pPayload + i);
                    length = (pPriDataHeader->data_length_15to8 << 8) | (pPriDataHeader->data_length_7to0);
                    i += sizeof(STSPrivateDataHeader);
                    DASSERT(pPriDataHeader->data_type_4cc_31to24 == 0x54);//T
                    DASSERT(pPriDataHeader->data_type_4cc_23to16 == 0x53);//S
                    if ((pPriDataHeader->data_type_4cc_15to8 == 0x53) && (pPriDataHeader->data_type_4cc_7to0 == 0x54)) {//ST
                        //start time
                        STSPrivateDataStartTime *pStartTime = (STSPrivateDataStartTime *)(pPayload + i);
                        msCurFileStartTime = *pStartTime;
                    } else if ((pPriDataHeader->data_type_4cc_15to8 == 0x44) && (pPriDataHeader->data_type_4cc_7to0 == 0x55)) {//DU
                        //durtaion (s)
                        STSPrivateDataDuration *pDuration = (STSPrivateDataDuration *)(pPayload + i);
                        mCurDuration = (pDuration->file_duration_15to8 << 8) | (pDuration->file_duration_7to0);
                    } else if ((pPriDataHeader->data_type_4cc_15to8 == 0x43) && (pPriDataHeader->data_type_4cc_7to0 == 0x4E)) {//CN
                        //channel name, todo
                    } else if ((pPriDataHeader->data_type_4cc_15to8 == 0x50) && (pPriDataHeader->data_type_4cc_7to0 == 0x49)) {//PI
                        //gop index, todo
                        mnGOPCount = length >> 2;
                        mpGOPIndex = pPayload + i;
                        LOG_PRINTF("0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x\n", mpGOPIndex[0], mpGOPIndex[1], mpGOPIndex[2], mpGOPIndex[3], mpGOPIndex[4], mpGOPIndex[5], mpGOPIndex[6], mpGOPIndex[7]);
                    } else {
                        LOGM_FATAL("unknown private data type: 0x%x, 0x%x\n", pPriDataHeader->data_type_4cc_15to8, pPriDataHeader->data_type_4cc_7to0);
                    }
                    i += length;
                }
            }
            break;
        default:
            LOGM_WARN("Please add new private pes type: 0x%x\n", type);
            break;
    }

    return EECode_OK;
}

EECode CTSDemuxer::readPrivatePacket(TU8 type, SPESInfo *pPesInfo)
{
    if (msPrivatePES.enable == 0) {
        LOGM_ERROR("No Private Stream!\n");
        return EECode_Error;
    }
    pPesInfo->pBuffer = NULL;
    pPesInfo->dataSize = 0;
    return readStream(type, pPesInfo, 0);
}

EECode CTSDemuxer::seekToRequestTime(const SDateTime *pStart, const SDateTime *pRequest)
{
    DASSERT(pStart);
    DASSERT(pRequest);

    TTime diffSeconds = 0;
    if (DLikely((pRequest->year == pStart->year) && (pRequest->month == pStart->month) && (pRequest->day == pStart->day))) {
        diffSeconds = (TTime)(pRequest->hour - pStart->hour) * 3600 + (TTime)pRequest->minute * 60 - (TTime)pStart->minute * 60 + (TTime)pRequest->seconds - (TTime)pStart->seconds;
    } else {
        //diffSeconds = (TU32)(24 - pStart->hour)*3600 + pStart->minute*60 - pStart->minute*60 + pStart->seconds - pStart->seconds;
    }
    if (diffSeconds <= (mGOPSize / mFramRate)) {
        //don't need to seek
        return EECode_OK;
    }

    if (DUnlikely(mCurDuration <= diffSeconds)) {
        LOGM_FATAL("seekToRequestTime, mCurDuration invalid %u and diffSeconds %lld\n", mCurDuration, diffSeconds);
        return EECode_Error;
    }

    TU32 gopIndex = (TU32)diffSeconds * mFramRate / mGOPSize; //todo
    LOG_PRINTF("[Debug]: seekToRequestTime, diffSeconds %lld, target gop index %u\n", diffSeconds, gopIndex);

    TIOSize fileTotalSize, curPosition1, curPosition2;
    mpIO->Query(fileTotalSize, curPosition1);
    TUint remainSize = mDataRemainSize;

    //TUint gopIndexMaxLength = mTsPacketSize - MPEG_TS_TP_PACKET_HEADER_SIZE - PES_HEADER_LEN - sizeof(STSPrivateDataHeader);
    TIOSize targetPosition = mTsPacketSize * mCurDuration / 168 + mTsPacketSize; //hardcode
    LOG_PRINTF("[Debug]: seekToRequestTime, targetPosition %lld\n", targetPosition);
    mpIO->Seek(0 - targetPosition, EIOPosition_End);
    mDataRemainSize = 0;
    targetPosition = 0;

    SPESInfo sPrivatePESInfo;
    sPrivatePESInfo.dataSize = 0;
    sPrivatePESInfo.pBuffer = NULL;
    while (readPrivatePacket(msPrivatePES.stream_type, &sPrivatePESInfo) == EECode_OK) {
        DASSERT(sPrivatePESInfo.pBuffer);
        if (DLikely(EECode_OK == parsePrivatePacket(msPrivatePES.stream_type, sPrivatePESInfo.pBuffer))) {
            if (DLikely(mpGOPIndex)) {
                LOG_PRINTF("[Debug]: seekToRequestTime, mnGOPCount %u\n", mnGOPCount);
                if ((gopIndex + 1) <= mnGOPCount) {
                    TU32 index = gopIndex * sizeof(TU32);
                    TU32 offset = (mpGOPIndex[index] << 24) | (mpGOPIndex[index + 1] << 16) | (mpGOPIndex[index + 2] << 8) | (mpGOPIndex[index + 3]);
                    targetPosition = offset * mTsPacketSize;
                    break;
                } else {
                    gopIndex -= mnGOPCount;
                }
            }
        } else {
            LOGM_ERROR("parsePrivatePacket fail: type %u\n", msPrivatePES.stream_type);
            break;
        }
    }

    LOG_PRINTF("[Debug]: seekToRequestTime done, targetPosition %lld\n", targetPosition);

    EECode err = EECode_OK;
    if (targetPosition) {
        err = mpIO->Seek(targetPosition, EIOPosition_Begin);
        mDataRemainSize = 0;
        memset(mpBufferAligned, 0x0, mBufferSize);
        mDataSize = 0;
        mDataRemainSize = 0;
        mbFileEOF = 0;
    } else {
        mpIO->Query(fileTotalSize, curPosition2);
        if (curPosition2 != curPosition1) {
            err = mpIO->Seek((curPosition1 - remainSize), EIOPosition_Begin);
            mDataRemainSize = 0;
        } else {
            mDataRemainSize = remainSize;
        }
        mbFileEOF = 0;
    }

    return err;
}

EECode CTSDemuxer::getFirstTime(TTime &time)
{
    //DASSERT(!mbFileOpened);

    readPacket();

    MPEG_TS_TP_HEADER *p_ts_header = (MPEG_TS_TP_HEADER *)mpBufferAligned;
    TU8 *p_payload = NULL;
    TU16 pid = ((TU16)p_ts_header->pid_12to8 << 8) | (TU16)p_ts_header->pid_7to0;

    if (DLikely(msVideoStream.elementary_PID == pid)) {
        if (DLikely(p_ts_header->adaptation_field_control == MPEG_TS_ADAPTATION_FIELD_PAYLOAD_ONLY)) {
            p_payload = mpBufferAligned + 4;
        } else {
            LOG_ERROR("not as expected\n");
            return EECode_InternalLogicalBug;
        }

        MPEG_TS_TP_PES_HEADER *pPesHeader = (MPEG_TS_TP_PES_HEADER *)p_payload;
        DASSERT(pPesHeader->packet_start_code_23to16 == 0x00);
        DASSERT(pPesHeader->packet_start_code_15to8 == 0x00);
        DASSERT(pPesHeader->packet_start_code_7to0 == 0x01);

        if (pPesHeader->pts_dts_flags == 0x02) {
            DASSERT(pPesHeader->header_data_length == PTS_FIELD_SIZE);
            time = _get_ptsordts(p_payload + PES_HEADER_LEN);
            //LOGM_DEBUG("time %lld\n", time);
        } else if (pPesHeader->pts_dts_flags == 0x03) {
            DASSERT(pPesHeader->header_data_length == PTS_FIELD_SIZE + DTS_FIELD_SIZE);
            time = _get_ptsordts(p_payload + PES_HEADER_LEN);
        } else {
            LOG_ERROR("no time info\n");
            return EECode_BadState;
        }

    } else {
        LOG_ERROR("pid not expected, video pid %d, pid %d\n", msVideoStream.elementary_PID, pid);
        return EECode_InternalLogicalBug;
    }

    return EECode_OK;
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END
