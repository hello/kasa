/*******************************************************************************
 * ts_scheduled_demuxer.cpp
 *
 * History:
 *    2014/09/15 - [Zhi He] create file
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
#include "ts_scheduled_demuxer.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

#define DNATIVE_TS_TRY_READ_PACKET_NUMBER (1024)
#define DNATIVE_TS_TRY_REREAD_PACKET_NUMBER (16)

#ifdef BUILD_WITH_UNDER_DEVELOP_COMPONENT

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

static EECode __ts_scheduled_demuxer_release_ring_buffer(CIBuffer *pBuffer)
{
    DASSERT(pBuffer);
    if (DLikely(pBuffer)) {
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

IDemuxer *gfCreateSceduledTSDemuxerModule(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TUint index)
{
    return CTSScheduledDemuxer::Create(pName, pPersistMediaConfig, pEngineMsgSink, index);
}

IDemuxer *CTSScheduledDemuxer::Create(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink, TUint index)
{
    CTSScheduledDemuxer *result = new CTSScheduledDemuxer(pname, pPersistMediaConfig, pMsgSink, index);
    if (DUnlikely(result && result->Construct() != EECode_OK)) {
        delete result;
        result = NULL;
    }

    return result;
}

CTSScheduledDemuxer::CTSScheduledDemuxer(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink, TUint index)
    : inherited(pname, index)
    , mpPersistMediaConfig(pPersistMediaConfig)
    , mpMsgSink(NULL)
    , mpIO(NULL)
    , mpIOTail(NULL)
    , mpMutex(NULL)
    , mpStorageManager(NULL)
    , mpBufferBase(NULL)
    , mpBufferAligned(NULL)
    , mpCurrentReadPointer(NULL)
    , mpTmpBufferBase(NULL)
    , mpTmpBufferAligned(NULL)
    , mTmpBufferSize(64)
    , mnRemainTsPacketNumber(0)
    , mnCurrentTsPacketIndex(0)
    , mnBaseTsPacketIndex(0)
    , mpFastNaviIndexBuffer(NULL)
    , mnFastNaviIndexBufferMaxCount(1024)
    , mnFastNaviIndexCount(0)
    , mPmtPID(0)
    , mVideoPID(0)
    , mAudioPID(0)
    , mNativePrivatePID(0)
    , mVideoStreamType(0)
    , mAudioStreamType(0)
    , mNativePrivatedStreamType(0)
    , mbHaveNativePrivatedData(0)
    , mbHaveNavigationIndex(0)
    , mNextFrameType(0)
    , mbFileOpened(0)
    , mbExtraDataSent(0)
    , mbEnableVideo(0)
    , mbEnableAudio(0)
    , mpVideoExtraData(NULL)
    , mVideoExtraDataSize(0)
    , mpAudioExtraData(NULL)
    , mAudioExtraDataSize(0)
    , mVideoFramerateNum(DDefaultVideoFramerateNum)
    , mVideoFramerateDen(DDefaultVideoFramerateDen)
    , mAudioSamplerate(DDefaultAudioSampleRate)
    , mAudioFrameLength(1024)
    , mCurFileDuration(180)
    , mEstimatedGOPDuration(4)
{
    TU32 i = 0;
    for (i = 0; i < EConstMaxDemuxerMuxerStreamNumber; i ++) {
        mpOutputPins[i] = NULL;
        mpBufferPool[i] = NULL;
        mpMemPool[i] = NULL;
    }

    memset(mCurrentFilename, 0x0, sizeof(mCurrentFilename));
    memset(mChannelName, 0x0, sizeof(mChannelName));

    memset(&mStreamCodecInfos, 0x0, sizeof(mStreamCodecInfos));
    memset(&mDemuxerInfo, 0x0, sizeof(mDemuxerInfo));

    memset(&msCurFileStartTime, 0x0, sizeof(msCurFileStartTime));
}

EECode CTSScheduledDemuxer::Construct()
{
    DSET_MODULE_LOG_CONFIG(ELogModuleNativeTSDemuxer);

    mpBufferBase = (TU8 *)DDBG_MALLOC((DNATIVE_TS_TRY_READ_PACKET_NUMBER + DNATIVE_TS_TRY_REREAD_PACKET_NUMBER) * MPEG_TS_TP_PACKET_SIZE + 31, "dmFB");
    if (DUnlikely(!mpBufferBase)) {
        LOGM_FATAL("no memory\n");
        return EECode_NoMemory;
    }
    mpBufferAligned = (TU8 *)(((TULong)(mpBufferBase + 31)) & (~31));

    mpTmpBufferBase = (TU8 *)DDBG_MALLOC(mTmpBufferSize * MPEG_TS_TP_PACKET_SIZE + 31, "dmTB");
    if (DUnlikely(!mpTmpBufferBase)) {
        LOGM_FATAL("no memory\n");
        return EECode_NoMemory;
    }
    mpTmpBufferAligned = (TU8 *)(((TULong)(mpTmpBufferBase + 31)) & (~31));

    mpIO = gfCreateIO(EIOType_File);
    if (DUnlikely(!mpIO)) {
        LOGM_FATAL("no memory\n");
        return EECode_NoMemory;
    }

    mpIOTail = gfCreateIO(EIOType_File);
    if (DUnlikely(!mpIOTail)) {
        LOGM_FATAL("no memory\n");
        return EECode_NoMemory;
    }

    mpMutex = gfCreateMutex();
    if (DUnlikely(!mpMutex)) {
        LOGM_FATAL("no memory\n");
        return EECode_NoMemory;
    }

    mpStorageManager = mpPersistMediaConfig->p_storage_manager;

    return inherited::Construct();
}

CObject *CTSScheduledDemuxer::GetObject0() const
{
    (CObject *) this;
}

void CTSScheduledDemuxer::Delete()
{
    if (mpMemPool[EDemuxerVideoOutputPinIndex]) {
        mpMemPool[EDemuxerVideoOutputPinIndex]->Delete();
        mpMemPool[EDemuxerVideoOutputPinIndex] = NULL;
    }

    if (mpMemPool[EDemuxerAudioOutputPinIndex]) {
        mpMemPool[EDemuxerAudioOutputPinIndex]->Delete();
        mpMemPool[EDemuxerAudioOutputPinIndex] = NULL;
    }

    if (mpBufferBase) {
        free(mpBufferBase);
        mpBufferBase = NULL;
        mpBufferAligned = NULL;
    }

    if (mpIO) {
        mpIO->Delete();
        mpIO = NULL;
    }

    if (mpIOTail) {
        mpIOTail->Delete();
        mpIOTail = NULL;
    }

    if (mpVideoExtraData) {
        free(mpVideoExtraData);
        mpVideoExtraData = NULL;
    }

    if (mpAudioExtraData) {
        free(mpAudioExtraData);
        mpAudioExtraData = NULL;
    }

    if (mpMutex) {
        mpMutex->Delete();
        mpMutex = NULL;
    }

    mpStorageManager = NULL;

    inherited::Delete();
}

CTSScheduledDemuxer::~CTSScheduledDemuxer()
{
    if (mpMemPool[EDemuxerVideoOutputPinIndex]) {
        mpMemPool[EDemuxerVideoOutputPinIndex]->Delete();
        mpMemPool[EDemuxerVideoOutputPinIndex] = NULL;
    }

    if (mpMemPool[EDemuxerAudioOutputPinIndex]) {
        mpMemPool[EDemuxerAudioOutputPinIndex]->Delete();
        mpMemPool[EDemuxerAudioOutputPinIndex] = NULL;
    }

    if (mpBufferBase) {
        free(mpBufferBase);
        mpBufferBase = NULL;
        mpBufferAligned = NULL;
    }

    if (mpIO) {
        mpIO->Delete();
        mpIO = NULL;
    }

    if (mpIOTail) {
        mpIOTail->Delete();
        mpIOTail = NULL;
    }

    if (mpVideoExtraData) {
        free(mpVideoExtraData);
        mpVideoExtraData = NULL;
    }

    if (mpAudioExtraData) {
        free(mpAudioExtraData);
        mpAudioExtraData = NULL;
    }

    if (mpMutex) {
        mpMutex->Delete();
        mpMutex = NULL;
    }

    mpStorageManager = NULL;
}

EECode CTSScheduledDemuxer::SetupOutput(COutputPin *p_output_pins[], CBufferPool *p_bufferpools[],  IMemPool *p_mempools[], IMsgSink *p_msg_sink)
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
        mpBufferPool[EDemuxerVideoOutputPinIndex]->SetReleaseBufferCallBack(__ts_scheduled_demuxer_release_ring_buffer);
        mpMemPool[EDemuxerVideoOutputPinIndex] = CRingMemPool::Create(2 * 1024 * 1024);
        mbEnableVideo = 1;
    }

    if (mpOutputPins[EDemuxerAudioOutputPinIndex] && mpBufferPool[EDemuxerAudioOutputPinIndex]) {
        mpBufferPool[EDemuxerAudioOutputPinIndex]->SetReleaseBufferCallBack(__ts_scheduled_demuxer_release_ring_buffer);
        mpMemPool[EDemuxerAudioOutputPinIndex] = CRingMemPool::Create(128 * 1024);
        mbEnableAudio = 1;
    }

    if (!mbEnableVideo && !mbEnableAudio) {
        LOGM_ERROR("neither video nor audio is enabled!?\n");
        return EECode_Error;
    }

    return EECode_OK;
}

EECode CTSScheduledDemuxer::SetupContext(TChar *url, void *p_agent, TU8 priority, TU32 request_receive_buffer_size, TU32 request_send_buffer_size)
{
    DASSERT(!url);

    return EECode_OK;
}

EECode CTSScheduledDemuxer::DestroyContext()
{
    return destroyContext();
}

EECode CTSScheduledDemuxer::ReconnectServer()
{
    return EECode_OK;
}

EECode CTSScheduledDemuxer::Seek(TTime &target_time, ENavigationPosition position)
{
    return EECode_OK;
}

EECode CTSScheduledDemuxer::Start()
{

}

EECode CTSScheduledDemuxer::Stop()
{

}

EECode CTSScheduledDemuxer::Suspend()
{
    return EECode_OK;
}

EECode CTSScheduledDemuxer::Pause()
{
    LOGM_FATAL("not supported\n");
    return EECode_NotSupported;
}

EECode CTSScheduledDemuxer::Resume()
{
    LOGM_FATAL("not supported\n");
    return EECode_NotSupported;
}

EECode CTSScheduledDemuxer::Flush()
{
    LOGM_FATAL("CTSScheduledDemuxer::Flush TO DO\n");
    return EECode_NotSupported;
}

EECode CTSScheduledDemuxer::ResumeFlush()
{
    LOGM_FATAL("CTSScheduledDemuxer::ResumeFlush TO DO\n");
    return EECode_NotSupported;
}

EECode CTSScheduledDemuxer::Purge()
{
    LOGM_FATAL("CTSScheduledDemuxer::Purge TO DO\n");
    return EECode_NotSupported;
}

EECode CTSScheduledDemuxer::SetPbRule(TU8 direction, TU8 feeding_rule, TU16 speed)
{
    LOGM_FATAL("CTSScheduledDemuxer::SetPbRule TO DO\n");
    return EECode_NotSupported;
}

EECode CTSScheduledDemuxer::SetPbLoopMode(TU32 *p_loop_mode)
{
    LOGM_FATAL("CTSScheduledDemuxer::SetPbLoopMode TO DO\n");
    return EECode_NotSupported;
}

EECode CTSScheduledDemuxer::EnableVideo(TU32 enable)
{
    LOGM_ERROR("not implemented in this module.\n");
    return EECode_NoImplement;
}

EECode CTSScheduledDemuxer::EnableAudio(TU32 enable)
{
    LOGM_ERROR("not implemented in this module.\n");
    return EECode_NoImplement;
}

void CTSScheduledDemuxer::Scheduling()
{
    CIBuffer *pBuffer = NULL;
    EECode err = EECode_OK;
    MPEG_TS_TP_HEADER *p_ts_header = NULL;
    TU8 *p_payload = NULL;
    TU16 pid = 0;

    if (DUnlikely(!mNextFrameType)) {
        if (DUnlikely(mnRemainTsPacketNumber <= DNATIVE_TS_TRY_REREAD_PACKET_NUMBER)) {
            err = readDataBlock();
            if (DUnlikely(EECode_OK != err)) {
                LOGM_ERROR("readDataBlock() fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
                return err;
            }
        }

        while (mnRemainTsPacketNumber) {
            p_ts_header = (MPEG_TS_TP_HEADER *)mpCurrentReadPointer;
            pid = ((TU16)p_ts_header->pid_12to8 << 8) | (TU16)p_ts_header->pid_7to0;
            if (mVideoPID == pid) {
                TU8 adaption_field_length = 0;
                if (p_ts_header->adaptation_field_control == MPEG_TS_ADAPTATION_FIELD_PAYLOAD_ONLY) {
                    p_payload = mpCurrentReadPointer + 5;
                } else if (p_ts_header->adaptation_field_control == MPEG_TS_ADAPTATION_FIELD_BOTH) {
                    adaption_field_length = mpCurrentReadPointer[4];
                    p_payload = mpCurrentReadPointer + 5 + adaption_field_length;
                }

                PAT_HDR *p_pat = (PAT_HDR *)p_payload;
                TU32 section_length = ((p_pat->section_length8to11 << 8) | (p_pat->section_length0to7)) + 3;
                TU32 program_length = section_length - PAT_HDR_SIZE - 4;
                PAT_ELEMENT *p_pat_element = (PAT_ELEMENT *)(p_payload + PAT_HDR_SIZE);
                mPmtPID = ((p_pat_element->program_map_PID_h << 8) | p_pat_element->program_map_PID_l);
                LOGM_NOTICE("Find PAT and PMT PID 0x%x\n", mPmtPID);
                break;
            }
            mnRemainTsPacketNumber --;
            mnCurrentTsPacketIndex ++;
            mpCurrentReadPointer += MPEG_TS_TP_PACKET_SIZE;

            if (DUnlikely(mnRemainTsPacketNumber <= DNATIVE_TS_TRY_REREAD_PACKET_NUMBER)) {
                err = readDataBlock();
                if (DUnlikely(EECode_OK != err)) {
                    LOGM_ERROR("readDataBlock() fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
                    return err;
                }
            }

        }

        if ()
        }



}

void CTSScheduledDemuxer::PrintStatus()
{
}

void CTSScheduledDemuxer::EventNotify(EEventType type, TU64 param1, TPointer param2)
{
    EECode err = EECode_OK;
    DASSERT(mpWorkQ);

    LOGM_FLOW("before mpWorkQ->PostMsg(ECMDType_NotifyBufferRelease)...\n");
    err = mpWorkQ->PostMsg(ECMDType_NotifyBufferRelease, NULL);
    LOGM_FLOW("after mpWorkQ->PostMsg(ECMDType_NotifyBufferRelease), ret %d\n", err);

    DASSERT_OK(err);

    return;
}

EECode CTSScheduledDemuxer::QueryContentInfo(const SStreamCodecInfos *&pinfos) const
{
    pinfos = (const SStreamCodecInfos *)&mStreamCodecInfos;
    return EECode_OK;
}

EECode CTSScheduledDemuxer::UpdateContext(SContextInfo *pContext)
{
    EECode err = EECode_OK;
    DASSERT(mpWorkQ);

    LOGM_FLOW("before mpWorkQ->SendCmd(ECMDType_UpdateUrl)...\n");
    err = mpWorkQ->SendCmd(ECMDType_UpdateUrl, (void *)pContext);
    LOGM_FLOW("after mpWorkQ->SendCmd(ECMDType_UpdateUrl), ret %d\n", err);

    DASSERT_OK(err);

    return err;
}

EECode CTSScheduledDemuxer::GetExtraData(SStreamingSessionContent *pContent)
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

EECode CTSScheduledDemuxer::readVideoFrame()
{
    MPEG_TS_TP_HEADER *p_ts_header = NULL;
    TU16 pid = 0;
    EECode err = EECode_OK;

    while (mnRemainTsPacketNumber) {
        p_ts_header = (MPEG_TS_TP_HEADER *)mpCurrentReadPointer;
        pid = ((TU16)p_ts_header->pid_12to8 << 8) | (TU16)p_ts_header->pid_7to0;
        if (DLikely(mVideoPID == pid)) {
            if (DLikely(p_ts_header->payload_unit_start_indicator)) {
                break;
            }
        }
        LOGM_WARN("skip packet in video path, mVideoPID %x, pid %x, indicator %d\n", mVideoPID, pid, p_ts_header->payload_unit_start_indicator);
        mnRemainTsPacketNumber --;
        mnCurrentTsPacketIndex ++;
        mpCurrentReadPointer += MPEG_TS_TP_PACKET_SIZE;

        if (DUnlikely(mnRemainTsPacketNumber <= DNATIVE_TS_TRY_REREAD_PACKET_NUMBER)) {
            err = readDataBlock();
            if (DUnlikely(EECode_OK != err)) {
                LOGM_ERROR("readDataBlock() fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
                return err;
            }
        }
    }

    TU8 *p_payload = NULL;

    while (mnRemainTsPacketNumber) {
        p_ts_header = (MPEG_TS_TP_HEADER *)mpCurrentReadPointer;
        pid = ((TU16)p_ts_header->pid_12to8 << 8) | (TU16)p_ts_header->pid_7to0;
        if (DLikely(mVideoPID == pid)) {

            TU8 adaption_field_length = 0;
            if (p_ts_header->adaptation_field_control == MPEG_TS_ADAPTATION_FIELD_BOTH) {
                p_payload = mpCurrentReadPointer + 4;
            } else {
                adaption_field_length = mpCurrentReadPointer[4];
                p_payload = mpCurrentReadPointer + 5 + adaption_field_length;
            }

            if (pPesInfo->pesLength > 0) {
                pPesInfo->bufferSize = pPesInfo->pesLength - totalPesHeaerLength + 6;
                pPesInfo->pBuffer = mpMemPool[pPesInfo->streamIndex]->RequestMemBlock(pPesInfo->bufferSize);
            } else {
                pPesInfo->pBuffer = mpMemPool[pPesInfo->streamIndex]->RequestMemBlock(512 * 1024); //hardcode
                pPesInfo->bufferSize = 512 * 1024;
            }
            pPesInfo->pesLength = (pPesHeader->pes_packet_length_15to8 << 8) | pPesHeader->pes_packet_length_7to0;

            PMT_HDR *p_pmt = (PMT_HDR *)p_payload;
            TU32 section_length = ((p_pmt->section_length8to11 << 8) | (p_pmt->section_length0to7)) + 3;
            TUint program_length = section_length - PMT_HDR_SIZE - 4;
            PMT_ELEMENT *p_pmt_element = NULL;
            TU8 stream_type = 0;
            for (TUint i = 0; i < program_length;) {
                p_pmt_element = (PMT_ELEMENT *)(p_payload + PMT_HDR_SIZE + i);
                stream_type = p_pmt_element->stream_type;
                switch (stream_type) {
                    case MPEG_SI_STREAM_TYPE_AAC:
                        mbEnableAudio = 1;
                        mAudioStreamType = MPEG_SI_STREAM_TYPE_AAC;
                        mAudioPID = (p_pmt_element->elementary_PID8to12 << 8) | (p_pmt_element->elementary_PID0to7);
                        i += PMT_ELEMENT_SIZE;
                        LOGM_NOTICE("Has AAC Stream!\n");
                        break;

                    case MPEG_SI_STREAM_TYPE_AVC_VIDEO:
                        mbEnableVideo = 1;
                        mVideoStreamType = MPEG_SI_STREAM_TYPE_AVC_VIDEO;
                        mVideoPID = (p_pmt_element->elementary_PID8to12 << 8) | (p_pmt_element->elementary_PID0to7);
                        i += PMT_ELEMENT_SIZE;
                        LOGM_NOTICE("Has AVC Stream!\n");
                        break;

                    case MPEG_SI_STREAM_TYPE_PRIVATE_SECTION:
                        //todo
                        //parse private section
                        i += PMT_ELEMENT_SIZE;
                        LOGM_NOTICE("TODO: Parse Private Section\n");
                        break;

                    case MPEG_SI_STREAM_TYPE_PRIVATE_PES:
                        mbHaveNativePrivatedData = 1;
                        mNativePrivatedStreamType = MPEG_SI_STREAM_TYPE_PRIVATE_PES;
                        mNativePrivatePID = (p_pmt_element->elementary_PID8to12 << 8) | (p_pmt_element->elementary_PID0to7);
                        i += PMT_ELEMENT_SIZE;
                        LOGM_NOTICE("Has Private PES!\n");
                        break;

                    default:
                        i++;
                        LOGM_NOTICE("Find unknown stream type: 0x%x!\n", stream_type);
                        break;
                }
            }
        }

        mnRemainTsPacketNumber --;
        mnCurrentTsPacketIndex ++;
        mpCurrentReadPointer += MPEG_TS_TP_PACKET_SIZE;

        if (DUnlikely(mnRemainTsPacketNumber <= DNATIVE_TS_TRY_REREAD_PACKET_NUMBER)) {
            err = readDataBlock();
            if (DUnlikely(EECode_OK != err)) {
                LOGM_ERROR("readDataBlock() fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
                return err;
            }
        }

    }

    if (mbHaveNativePrivatedData) {
        TU32 ori_remain = mnRemainTsPacketNumber;
        TU32 ori_cur_index = mnCurrentTsPacketIndex;
        TU8 *ori_cur_pointer = mpCurrentReadPointer;

        while (mnRemainTsPacketNumber) {
            p_ts_header = (MPEG_TS_TP_HEADER *)mpCurrentReadPointer;
            pid = ((TU16)p_ts_header->pid_12to8 << 8) | (TU16)p_ts_header->pid_7to0;
            if (DLikely(mNativePrivatePID == pid)) {

                TU8 adaption_field_length = 0;
                if (p_ts_header->adaptation_field_control == MPEG_TS_ADAPTATION_FIELD_PAYLOAD_ONLY) {
                    p_payload = mpCurrentReadPointer + 5;
                } else if (p_ts_header->adaptation_field_control == MPEG_TS_ADAPTATION_FIELD_BOTH) {
                    adaption_field_length = mpCurrentReadPointer[4];
                    p_payload = mpCurrentReadPointer + 5 + adaption_field_length;
                }
                parseNativePrivatePacketHeader(p_payload);
                break;
            }
            mnRemainTsPacketNumber --;
            mnCurrentTsPacketIndex ++;
            mpCurrentReadPointer += MPEG_TS_TP_PACKET_SIZE;

            if (DUnlikely(mnRemainTsPacketNumber <= DNATIVE_TS_TRY_REREAD_PACKET_NUMBER)) {
                LOGM_WARN("can not find pid %x, skip\n", mNativePrivatePID);
                mnRemainTsPacketNumber = ori_remain;
                mnCurrentTsPacketIndex = ori_cur_index;
                mpCurrentReadPointer = ori_cur_pointer;
                break;
            }

        }
    }

    return EECode_OK;
}

EECode CTSScheduledDemuxer::parseNativePrivatePacketHeader(TU8 *p_data)
{
    STSPrivateDataPesPacketHeader *p_pes_header = (STSPrivateDataPesPacketHeader *)p_data;
    DASSERT(p_pes_header->start_code_prefix_23to16 == 0x00);
    DASSERT(p_pes_header->start_code_prefix_15to8 == 0x00);
    DASSERT(p_pes_header->start_code_prefix_7to0 == 0x01);
    TU16 packetLength = (p_pes_header->packet_len_15to8 << 8) | (p_pes_header->packet_len_7to0);
    TU8 *p_payload = p_data + sizeof(STSPrivateDataPesPacketHeader);

    STSPrivateDataHeader *p_pridata_header = NULL;
    TU16 length = 0;
    for (TU16 i = 0; i < packetLength;) {
        p_pridata_header = (STSPrivateDataHeader *)(p_payload + i);
        length = (p_pridata_header->data_length_15to8 << 8) | (p_pridata_header->data_length_7to0);
        i += sizeof(STSPrivateDataHeader);
        DASSERT(p_pridata_header->data_type_4cc_31to24 == 0x54);//T
        DASSERT(p_pridata_header->data_type_4cc_23to16 == 0x53);//S
        if ((p_pridata_header->data_type_4cc_15to8 == 0x53) && (p_pridata_header->data_type_4cc_7to0 == 0x54)) {//ST
            //start time
            STSPrivateDataStartTime *p_start_time = (STSPrivateDataStartTime *)(p_payload + i);
            msCurFileStartTime = *p_start_time;
        } else if ((p_pridata_header->data_type_4cc_15to8 == 0x44) && (p_pridata_header->data_type_4cc_7to0 == 0x55)) {//DU
            //durtaion (s)
            STSPrivateDataDuration *pDuration = (STSPrivateDataDuration *)(p_payload + i);
            mCurFileDuration = (pDuration->file_duration_15to8 << 8) | (pDuration->file_duration_7to0);
        } else if ((p_pridata_header->data_type_4cc_15to8 == 0x43) && (p_pridata_header->data_type_4cc_7to0 == 0x4E)) {//CN
            //channel name, todo
        } else {
            LOGM_FATAL("unknown private data type: 'TS%c%c'\n", (TChar)p_pridata_header->data_type_4cc_15to8, (TChar)p_pridata_header->data_type_4cc_7to0);
            return EECode_NotSupported;
        }
        i += length;
    }

    return EECode_OK;
}

EECode CTSScheduledDemuxer::parseNativePrivatePacketTail()
{
    DASSERT(mpIOTail);

    EECode err = mpIOTail->Open(mCurrentFilename, EIOFlagBit_Read | EIOFlagBit_Binary);
    if (DUnlikely(EECode_OK != err)) {
        LOGM_ERROR("open(%s) fail, ret %d, %s\n", mCurrentFilename, err, gfGetErrorCodeString(err));
        return err;
    }

    TIOSize i = MPEG_TS_TP_PACKET_SIZE * mTmpBufferSize;
    err = mpIOTail->Seek((0 - i), EIOPosition_End);
    if (DUnlikely(EECode_OK != err)) {
        LOGM_ERROR("(%s)seek fail, ret %d, %s\n", mCurrentFilename, err, gfGetErrorCodeString(err));
        return err;
    }

    i = mTmpBufferSize;
    err = mpIOTail->Read(mpTmpBufferAligned, MPEG_TS_TP_PACKET_SIZE, i);
    if (DUnlikely(EECode_OK != err)) {
        LOGM_ERROR("(%s)read fail, ret %d, %s\n", mCurrentFilename, err, gfGetErrorCodeString(err));
        return err;
    }
    mpIOTail->Close();

    TU32 remain_number = mTmpBufferSize;
    TU8 *p_buffer = mpTmpBufferAligned, *ptmp = NULL;
    TU16 pid = 0;

    MPEG_TS_TP_HEADER *p_ts_header = NULL;
    TU32 read_count = 0, ii = 0, max = 0;

    while (remain_number) {
        p_ts_header = (MPEG_TS_TP_HEADER *) p_buffer;
        pid = ((TU16)p_ts_header->pid_12to8 << 8) | (TU16)p_ts_header->pid_7to0;
        if (DLikely(mNativePrivatePID == pid)) {
            if (DUnlikely(!p_ts_header->payload_unit_start_indicator)) {
                LOGM_ERROR("too many packets\n");
                return EECode_NotSupported;
            }

            //STSPrivateDataPesPacketHeader* pridata_pes_header = (STSPrivateDataPesPacketHeader*)(p_buffer + MPEG_TS_TP_PACKET_HEADER_SIZE);
            STSPrivateDataHeader *p_header = (STSPrivateDataHeader *)(p_buffer + MPEG_TS_TP_PACKET_HEADER_SIZE + sizeof(STSPrivateDataPesPacketHeader));

            TU32 total_count = (((TU32)(p_header->data_length_15to8) << 8) | ((TU32)p_header->data_length_7to0)) >> 2;
            if (DUnlikely(total_count > mnFastNaviIndexBufferMaxCount)) {
                if (mpFastNaviIndexBuffer) {
                    free(mpFastNaviIndexBuffer);
                }
                mpFastNaviIndexBuffer = (TU32 *) DDBG_MALLOC(total_count * sizeof(TU32), "dmFn");
                if (DUnlikely(!mpFastNaviIndexBuffer)) {
                    LOGM_ERROR("no memory\n");
                    return EECode_NoMemory;
                }
                mnFastNaviIndexBufferMaxCount = total_count;
            }
            mnFastNaviIndexCount = total_count;
            read_count = 0;
            max = (TU32)(MPEG_TS_TP_PACKET_SIZE - MPEG_TS_TP_PACKET_HEADER_SIZE - sizeof(STSPrivateDataPesPacketHeader) - sizeof(STSPrivateDataHeader)) >> 2;
            ptmp = (TU8 *)p_header + sizeof(STSPrivateDataHeader);
            for (ii = 0; ii < max; ii ++) {
                DBER32(mpFastNaviIndexBuffer[read_count], ptmp);
                read_count ++;
                ptmp += 4;
            }
            break;
        }
        remain_number --;
        p_buffer += MPEG_TS_TP_PACKET_SIZE;
    }

    if (DUnlikely(!remain_number)) {
        LOGM_WARN("not found\n");
        return EECode_NotExist;
    }

    max = (TU32)(MPEG_TS_TP_PACKET_SIZE - MPEG_TS_TP_PACKET_HEADER_SIZE) >> 2;

    while (remain_number) {
        p_ts_header = (MPEG_TS_TP_HEADER *) p_buffer;
        pid = ((TU16)p_ts_header->pid_12to8 << 8) | (TU16)p_ts_header->pid_7to0;
        if (DLikely(mNativePrivatePID == pid)) {
            DASSERT(!p_ts_header->payload_unit_start_indicator);

            ptmp = p_buffer + MPEG_TS_TP_PACKET_HEADER_SIZE;
            if (DUnlikely(1 == remain_number) {
            max = mnFastNaviIndexCount - read_count;
            for (ii = 0; ii < max; ii ++) {
                    DBER32(mpFastNaviIndexBuffer[read_count], ptmp);
                    read_count ++;
                    ptmp += 4;
                }
                break;
            } else {
                for (ii = 0; ii < max; ii ++) {
                    DBER32(mpFastNaviIndexBuffer[read_count], ptmp);
                    read_count ++;
                    ptmp += 4;
                }
            }
        } else {
            LOGM_ERROR("unexpected pid %x, pri %x\n", pid, mNativePrivatePID);
            mpIOTail->Close();
            return EECode_Error;
        }
        remain_number --;
        p_buffer += MPEG_TS_TP_PACKET_SIZE;
    }

    return EECode_OK;
}

EECode CTSScheduledDemuxer::beginReadDataBlock()
{
    if (DUnlikely(!mpIO)) {
        LOGM_FATAL("NULL mpIO\n");
        return EECode_BadState;
    }

    if (DUnlikely(mbFileOpened)) {
        mpIO->Close();
        mbFileOpened = 0;
    }

    EECode err = mpIO->Open(mCurrentFilename, EIOFlagBit_Read | EIOFlagBit_Binary);
    if (DUnlikely(EECode_OK != err)) {
        LOGM_ERROR("open(%s) fail, ret %d, %s\n", mCurrentFilename, err, gfGetErrorCodeString(err));
        return err;
    }
    mbFileOpened = 1;

    mnCurrentTsPacketIndex = 0;
    mnRemainTsPacketNumber = 0;
    mnBaseTsPacketIndex = 0;
    mpCurrentReadPointer = NULL;

    TIOSize try_read_count = DNATIVE_TS_TRY_READ_PACKET_NUMBER;

    err = mpIO->Read(mpBufferAligned, MPEG_TS_TP_PACKET_SIZE, try_read_count);
    if (DLikely(EECode_OK == err)) {
        mnRemainTsPacketNumber = (TU32)try_read_count;
        mpCurrentReadPointer = mpBufferAligned;
    } else {
        LOGM_ERROR("read fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    }

    return EECode_OK;
}

EECode CTSScheduledDemuxer::readDataBlock()
{
    TIOSize try_read_count = 0;
    TU32 remain_data_size = mnRemainTsPacketNumber * MPEG_TS_TP_PACKET_SIZE;
    EECode err = EECode_OK;

    if (DUnlikely(mnRemainTsPacketNumber > DNATIVE_TS_TRY_REREAD_PACKET_NUMBER)) {
        LOGM_ERROR("should not comes here\n");
        if (DLikely(mnRemainTsPacketNumber < (DNATIVE_TS_TRY_READ_PACKET_NUMBER))) {
            try_read_count = (DNATIVE_TS_TRY_REREAD_PACKET_NUMBER + DNATIVE_TS_TRY_READ_PACKET_NUMBER) - mnRemainTsPacketNumber;
            if (DLikely(remain_data_size)) {
                memmove(mpBufferAligned, mpBufferAligned + remain_data_size, remain_data_size);
            }
            mnBaseTsPacketIndex += mnCurrentTsPacketIndex;
            mnCurrentTsPacketIndex = 0;
            mpCurrentReadPointer = mpBufferAligned;

            err = mpIO->Read((TU8 *)(mpBufferAligned + remain_data_size), MPEG_TS_TP_PACKET_SIZE, try_read_count);
            if (DLikely(EECode_OK == err)) {
                mnRemainTsPacketNumber += (TU32)try_read_count;
            } else {
                LOGM_ERROR("read fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
                return err;
            }
        } else {
            LOGM_FATAL("should have logic bug, please check code\n");
            return EECode_InternalLogicalBug;
        }
    } else {
        if (DLikely(remain_data_size)) {
            memmove(mpBufferAligned, mpBufferAligned + remain_data_size, remain_data_size);
        }
        try_read_count = DNATIVE_TS_TRY_READ_PACKET_NUMBER;
        mnBaseTsPacketIndex += mnCurrentTsPacketIndex;
        mnCurrentTsPacketIndex = 0;
        mpCurrentReadPointer = mpBufferAligned;

        err = mpIO->Read((TU8 *)(mpBufferAligned + remain_data_size), MPEG_TS_TP_PACKET_SIZE, try_read_count);
        if (DLikely(EECode_OK == err)) {
            mnRemainTsPacketNumber += (TU32)try_read_count;
        } else {
            LOGM_ERROR("read fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
            return err;
        }
    }

    return EECode_OK;
}

EECode CTSScheduledDemuxer::readHeader()
{
    DASSERT(!mbFileOpened);

    EECode err = beginReadDataBlock(mCurrentFilename);
    if (DUnlikely(EECode_OK != err)) {
        LOGM_FATAL("beginReadDataBlock(%s) fail, ret %d, %s\n", mCurrentFilename, err, gfGetErrorCodeString(err));
        return err;
    }

    MPEG_TS_TP_HEADER *p_ts_header = NULL;
    TU8 *p_payload = NULL;
    TU16 pid = 0;

    while (mnRemainTsPacketNumber) {
        p_ts_header = (MPEG_TS_TP_HEADER *)mpCurrentReadPointer;
        pid = ((TU16)p_ts_header->pid_12to8 << 8) | (TU16)p_ts_header->pid_7to0;
        if (DLikely(MPEG_TS_PAT_PID == pid)) {

            TU8 adaption_field_length = 0;
            if (p_ts_header->adaptation_field_control == MPEG_TS_ADAPTATION_FIELD_PAYLOAD_ONLY) {
                p_payload = mpCurrentReadPointer + 5;
            } else if (p_ts_header->adaptation_field_control == MPEG_TS_ADAPTATION_FIELD_BOTH) {
                adaption_field_length = mpCurrentReadPointer[4];
                p_payload = mpCurrentReadPointer + 5 + adaption_field_length;
            }

            PAT_HDR *p_pat = (PAT_HDR *)p_payload;
            TU32 section_length = ((p_pat->section_length8to11 << 8) | (p_pat->section_length0to7)) + 3;
            TU32 program_length = section_length - PAT_HDR_SIZE - 4;
            PAT_ELEMENT *p_pat_element = (PAT_ELEMENT *)(p_payload + PAT_HDR_SIZE);
            mPmtPID = ((p_pat_element->program_map_PID_h << 8) | p_pat_element->program_map_PID_l);
            LOGM_NOTICE("Find PAT and PMT PID 0x%x\n", mPmtPID);
            break;
        }
        mnRemainTsPacketNumber --;
        mnCurrentTsPacketIndex ++;
        mpCurrentReadPointer += MPEG_TS_TP_PACKET_SIZE;

        if (DUnlikely(mnRemainTsPacketNumber <= DNATIVE_TS_TRY_REREAD_PACKET_NUMBER)) {
            err = readDataBlock();
            if (DUnlikely(EECode_OK != err)) {
                LOGM_ERROR("readDataBlock() fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
                return err;
            }
        }

    }

    while (mnRemainTsPacketNumber) {
        p_ts_header = (MPEG_TS_TP_HEADER *)mpCurrentReadPointer;
        pid = ((TU16)p_ts_header->pid_12to8 << 8) | (TU16)p_ts_header->pid_7to0;
        if (DLikely(mPmtPID == pid)) {

            TU8 adaption_field_length = 0;
            if (p_ts_header->adaptation_field_control == MPEG_TS_ADAPTATION_FIELD_PAYLOAD_ONLY) {
                p_payload = mpCurrentReadPointer + 5;
            } else if (p_ts_header->adaptation_field_control == MPEG_TS_ADAPTATION_FIELD_BOTH) {
                adaption_field_length = mpCurrentReadPointer[4];
                p_payload = mpCurrentReadPointer + 5 + adaption_field_length;
            }

            PMT_HDR *p_pmt = (PMT_HDR *)p_payload;
            TU32 section_length = ((p_pmt->section_length8to11 << 8) | (p_pmt->section_length0to7)) + 3;
            TUint program_length = section_length - PMT_HDR_SIZE - 4;
            PMT_ELEMENT *p_pmt_element = NULL;
            TU8 stream_type = 0;
            for (TUint i = 0; i < program_length;) {
                p_pmt_element = (PMT_ELEMENT *)(p_payload + PMT_HDR_SIZE + i);
                stream_type = p_pmt_element->stream_type;
                switch (stream_type) {
                    case MPEG_SI_STREAM_TYPE_AAC:
                        mbEnableAudio = 1;
                        mAudioStreamType = MPEG_SI_STREAM_TYPE_AAC;
                        mAudioPID = (p_pmt_element->elementary_PID8to12 << 8) | (p_pmt_element->elementary_PID0to7);
                        i += PMT_ELEMENT_SIZE;
                        LOGM_NOTICE("Has AAC Stream!\n");
                        break;

                    case MPEG_SI_STREAM_TYPE_AVC_VIDEO:
                        mbEnableVideo = 1;
                        mVideoStreamType = MPEG_SI_STREAM_TYPE_AVC_VIDEO;
                        mVideoPID = (p_pmt_element->elementary_PID8to12 << 8) | (p_pmt_element->elementary_PID0to7);
                        i += PMT_ELEMENT_SIZE;
                        LOGM_NOTICE("Has AVC Stream!\n");
                        break;

                    case MPEG_SI_STREAM_TYPE_PRIVATE_SECTION:
                        //todo
                        //parse private section
                        i += PMT_ELEMENT_SIZE;
                        LOGM_NOTICE("TODO: Parse Private Section\n");
                        break;

                    case MPEG_SI_STREAM_TYPE_PRIVATE_PES:
                        mbHaveNativePrivatedData = 1;
                        mNativePrivatedStreamType = MPEG_SI_STREAM_TYPE_PRIVATE_PES;
                        mNativePrivatePID = (p_pmt_element->elementary_PID8to12 << 8) | (p_pmt_element->elementary_PID0to7);
                        i += PMT_ELEMENT_SIZE;
                        LOGM_NOTICE("Has Private PES!\n");
                        break;

                    default:
                        i++;
                        LOGM_NOTICE("Find unknown stream type: 0x%x!\n", stream_type);
                        break;
                }
            }
        }

        mnRemainTsPacketNumber --;
        mnCurrentTsPacketIndex ++;
        mpCurrentReadPointer += MPEG_TS_TP_PACKET_SIZE;

        if (DUnlikely(mnRemainTsPacketNumber <= DNATIVE_TS_TRY_REREAD_PACKET_NUMBER)) {
            err = readDataBlock();
            if (DUnlikely(EECode_OK != err)) {
                LOGM_ERROR("readDataBlock() fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
                return err;
            }
        }

    }

    if (mbHaveNativePrivatedData) {
        TU32 ori_remain = mnRemainTsPacketNumber;
        TU32 ori_cur_index = mnCurrentTsPacketIndex;
        TU8 *ori_cur_pointer = mpCurrentReadPointer;

        while (mnRemainTsPacketNumber) {
            p_ts_header = (MPEG_TS_TP_HEADER *)mpCurrentReadPointer;
            pid = ((TU16)p_ts_header->pid_12to8 << 8) | (TU16)p_ts_header->pid_7to0;
            if (DLikely(mNativePrivatePID == pid)) {

                TU8 adaption_field_length = 0;
                if (p_ts_header->adaptation_field_control == MPEG_TS_ADAPTATION_FIELD_PAYLOAD_ONLY) {
                    p_payload = mpCurrentReadPointer + 5;
                } else if (p_ts_header->adaptation_field_control == MPEG_TS_ADAPTATION_FIELD_BOTH) {
                    adaption_field_length = mpCurrentReadPointer[4];
                    p_payload = mpCurrentReadPointer + 5 + adaption_field_length;
                }
                parseNativePrivatePacketHeader(p_payload);
                break;
            }
            mnRemainTsPacketNumber --;
            mnCurrentTsPacketIndex ++;
            mpCurrentReadPointer += MPEG_TS_TP_PACKET_SIZE;

            if (DUnlikely(mnRemainTsPacketNumber <= DNATIVE_TS_TRY_REREAD_PACKET_NUMBER)) {
                LOGM_WARN("can not find pid %x, skip\n", mNativePrivatePID);
                mnRemainTsPacketNumber = ori_remain;
                mnCurrentTsPacketIndex = ori_cur_index;
                mpCurrentReadPointer = ori_cur_pointer;
                break;
            }

        }
    }

    return EECode_OK;
}

EECode CTSScheduledDemuxer::prepare()
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

        pTsPacketEnd = pTsPacketStart + MPEG_TS_TP_PACKET_SIZE;
        pTsHeader = (MPEG_TS_TP_HEADER *)pTsPacketStart;
        DASSERT(pTsHeader->sync_byte == MPEG_TS_TP_SYNC_BYTE);

        TU8 adaption_field_length = 0;
        if (pTsHeader->payload_unit_start_indicator != 0x01) {// 0x01 pes or psi
            mDataRemainSize -= MPEG_TS_TP_PACKET_SIZE;
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
        mDataRemainSize -= MPEG_TS_TP_PACKET_SIZE;

        //check quit
        if (patFind && pmtFind && priPES) {
            //prepare done, normal quit
            break;
        }
    }

    return (patFind && pmtFind) ? EECode_OK : EECode_Error;
}

EECode CTSScheduledDemuxer::readPacket()
{
    if (mbFileEOF) {
        LOGM_NOTICE("readPacket(), has been EOF!\n");
        return EECode_Error;
    }

    EECode err = EECode_OK;
    TUint bufferRemainSize = mBufferSize - mDataRemainSize;
    TIOSize tryreadCount = bufferRemainSize;

    if (mDataRemainSize) {
        memmove(mpBufferAligned, mpBufferAligned + bufferRemainSize, mDataRemainSize);
    }

    if ((err = mpIO->Read((TU8 *)(mpBufferAligned + mDataRemainSize), 1, (TIOSize &)tryreadCount)) != EECode_OK) {
        LOGM_ERROR("readPacket(), read file fail!\n");
        return err;
    }
    if (tryreadCount < bufferRemainSize) {
        LOGM_NOTICE("readPacket(), eof now! tryreadCount %lld, bufferRemainSize %u\n", tryreadCount, bufferRemainSize);
        mbFileEOF = 1;
    }
    mDataRemainSize += tryreadCount;
    if (mDataRemainSize < MPEG_TS_TP_PACKET_SIZE) {
        if (mbFileEOF) {
            LOGM_ERROR("readPacket(), the last packet size is %u (< 188), invalid packet!\n", mDataRemainSize);
        } else {
            LOGM_ERROR("readPacket(), Check Me!\n");
        }
        err = EECode_Error;
    }
    mDataSize = mDataRemainSize;
    return err;
}

EECode CTSScheduledDemuxer::readESData(MPEG_TS_TP_HEADER *pTsHeader, SPESInfo *pPesInfo, TU8 *pPayload)
{
    if (!pTsHeader || !pPesInfo || !pPayload) {
        LOGM_ERROR("readESPacket(), NULL input!\n");
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
            //  mDataRemainSize -= MPEG_TS_TP_PACKET_SIZE;
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

    //mDataRemainSize -= MPEG_TS_TP_PACKET_SIZE;
    if (!pPesInfo->pBuffer) {
        LOGM_NOTICE("lost pes header?!\n");
        mDataRemainSize -= MPEG_TS_TP_PACKET_SIZE;
        return EECode_TryAgain;
    }
    TU8 *pTsPacketEnd = (TU8 *)pTsHeader + MPEG_TS_TP_PACKET_SIZE; //188
    DASSERT(pTsPacketEnd > pPayload);
    TUint dataLength = pTsPacketEnd - pPayload;
    memcpy(pPesInfo->pBuffer + pPesInfo->dataSize, pPayload, dataLength);
    pPesInfo->dataSize += dataLength;

    if (pPesInfo->pesLength > 0) {
        if (pPesInfo->dataSize >= pPesInfo->bufferSize) {
            LOGM_DEBUG("Get one Frame: frame size %u, buffer size %u\n", pPesInfo->dataSize, pPesInfo->bufferSize);
            mDataRemainSize -= MPEG_TS_TP_PACKET_SIZE;
            return EECode_OK;
        }
    }
    mDataRemainSize -= MPEG_TS_TP_PACKET_SIZE;
    return EECode_TryAgain;
}

EECode CTSScheduledDemuxer::readStream(TU8 type, SPESInfo *&pPesInfo, TU8 flags)
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
        //pTsPacketEnd = pTsPacketStart + MPEG_TS_TP_PACKET_SIZE;
        pTsHeader = (MPEG_TS_TP_HEADER *)pTsPacketStart;
        DASSERT(pTsHeader->sync_byte == MPEG_TS_TP_SYNC_BYTE);

        TU8 adaption_field_length = 0;
        if (pTsHeader->adaptation_field_control == MPEG_TS_ADAPTATION_FIELD_BOTH) {
            adaption_field_length = pTsPacketStart[4] + 1;
        }
        pTsPayload =  pTsPacketStart + 4 + adaption_field_length;

        TU32 PID = (pTsHeader->pid_12to8 << 8) | pTsHeader->pid_7to0;
        if (type == MPEG_SI_STREAM_TYPE_PRIVATE_PES) {
            if (PID != msPrivatePES.elementary_PID) {
                mDataRemainSize -= MPEG_TS_TP_PACKET_SIZE;
                continue;
            }
            //read private pes
            pPesInfo->pBuffer = pTsPayload;
            pPesInfo->dataSize = MPEG_TS_TP_PACKET_SIZE - (sizeof(MPEG_TS_TP_HEADER) + adaption_field_length);
            mDataRemainSize -= MPEG_TS_TP_PACKET_SIZE;
            return EECode_OK;
        } else if (PID != msVideoStream.elementary_PID && PID != msAudioStream.elementary_PID) {
            mDataRemainSize -= MPEG_TS_TP_PACKET_SIZE;
            continue;
        }

        if (PID == msVideoStream.elementary_PID) {
            if (enableVideo) {
                pPesInfo = &msVideoPES;
            } else {
                mDataRemainSize -= MPEG_TS_TP_PACKET_SIZE;
                continue;
            }
        } else if (PID == msAudioStream.elementary_PID) {
            if (enableAudio) {
                pPesInfo = &msAudioPES;
            } else {
                mDataRemainSize -= MPEG_TS_TP_PACKET_SIZE;
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
        LOGM_ERROR("CTSScheduledDemuxer::readFrame() fail!\n");
    }
    return err;
}

EECode CTSScheduledDemuxer::readFrame(SPESInfo *&pPesInfo, TU8 flags)
{

    TU8 type = msVideoStream.stream_type | msAudioStream.stream_type;
    return readStream(type, pPesInfo, flags);
}

EECode CTSScheduledDemuxer::freeFrame(SPESInfo *pPesInfo)
{
    DASSERT(pPesInfo);
    if (pPesInfo->pBuffer) {
        mpMemPool[pPesInfo->streamIndex]->ReturnBackMemBlock(pPesInfo->bufferSize, pPesInfo->pBuffer);
        pPesInfo->pBuffer = NULL;
    }
    _clear_pes_info(pPesInfo);
    return EECode_OK;
}

EECode CTSScheduledDemuxer::setupContext(TChar *url)
{
    DASSERT(mpIO);
    if (!url) {
        LOGM_FATAL("NULL url in CTSScheduledDemuxer::SetupContext!\n");
        return EECode_BadParam;
    }

    destroyContext();

    if (mpIO->Open(url, EIOFlagBit_Read | EIOFlagBit_Binary) != EECode_OK) {
        LOGM_ERROR("CTSScheduledDemuxer::setupContext open file fail!\n");
        return EECode_Error;
    }
    mbFileOpened = 1;

    return EECode_OK;
}

EECode CTSScheduledDemuxer::destroyContext()
{
    if (mbFileOpened) {
        mpIO->Close();
        mbFileOpened = 0;
    }

    return EECode_OK;
}

EECode CTSScheduledDemuxer::getExtraData(SStreamingSessionContent *pContent)
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
                TU8 has_sps, has_pps, has_idr;
                TU8 *pPps = NULL;
                TU8 *pSps = NULL;
                TU8 *pIdr = NULL;
                TU8 *pPpsEnd = NULL;
                gfFindH264SpsPpsIdr(pDataBase, pPesInfo->bufferSize, has_sps, has_pps, has_idr, pSps, pPps, pPpsEnd, pIdr);
                if (has_sps && has_pps) {
                    TU32 extraDataSize = pPpsEnd - pSps;
                    mpVideoExtraData = (TU8 *)DDBG_MALLOC(extraDataSize + 4, "dmvE");
                    memcpy(mpVideoExtraData, pSps, extraDataSize);
                    mpVideoExtraData[extraDataSize] = '\0';
                    mVideoExtraDataSize = extraDataSize;
                    bVideo = 0;
                    LOGM_INFO("get video\n");
                }
            } else if (pPesInfo->streamIndex == EDemuxerAudioOutputPinIndex) {

                TU8 *pDataBase = pPesInfo->pBuffer;
                p_adts_header = (SADTSFixedHeader *)pDataBase;
                SSimpleAudioSpecificConfig *p_header = (SSimpleAudioSpecificConfig *)DDBG_MALLOC(sizeof(SSimpleAudioSpecificConfig), "dmaE");
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
                    LOGM_WARN("CTSScheduledDemuxer::getExtraData, aac profile is 'reserved'?\n");
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

EECode CTSScheduledDemuxer::sendExtraData()
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

EECode CTSScheduledDemuxer::processUpdateContext(SContextInfo *pContext)
{
    DASSERT(pContext || mpCurUrl);

    SDateTime startTime;
    TU16 fileDuration = 0;
    if (pContext) {
        if (DUnlikely((!pContext->pChannelName) || (DMaxChannelNameLength <= strlen(pContext->pChannelName)))) {
            LOG_ERROR("BAD parameters\n");
            return EECode_BadParam;
        }
        strcpy(mChannelName, pContext->pChannelName);

        if (!pContext->pRequestTime) {
            if (mpStorageManager->RequestExistUintSequentially(mChannelName, NULL, 1,  mpCurUrl, &startTime, fileDuration) != EECode_OK) {
                LOGM_ERROR("RequestExistUintSequentially fail!\n");
                return EECode_InternalLogicalBug;
            }
        } else {
            SDateTime *pTime = (SDateTime *)(pContext->pRequestTime);
            LOGM_NOTICE("reqeust time: %u, %u, %u, %u, %u, %u\n", pTime->year, pTime->month, pTime->day, pTime->hour, pTime->minute, pTime->seconds);
            if (mpStorageManager->RequestExistUint(mChannelName, pTime, mpCurUrl, &startTime, fileDuration) != EECode_OK) {
                LOGM_ERROR("RequestExistUint fail!\n");
                return EECode_InternalLogicalBug;
            }
            LOGM_NOTICE("start time: %u, %u, %u, %u, %u, %u\n", startTime.year, startTime.month, startTime.day, startTime.hour, startTime.minute, startTime.seconds);
        }
    } else if (mpCurUrl) {
        TChar *pNextUrl = NULL;
        if (EECode_OK != mpStorageManager->ReleaseExistUint(mChannelName, mpCurUrl)) {
            LOGM_ERROR("ReleaseExistUint(%s) fail!\n", mpCurUrl);
            return EECode_InternalLogicalBug;
        }
        if (mpStorageManager->RequestExistUintSequentially(mChannelName, mpCurUrl, 0, pNextUrl, &startTime, fileDuration) != EECode_OK) {
            LOGM_ERROR("RequestExistUintSequentially(%s) fail!\n", mpCurUrl);
            return EECode_InternalLogicalBug;
        }
        mpCurUrl = pNextUrl;
    } else {
        LOGM_ERROR("Check Me!\n");
        return EECode_InternalLogicalBug;
    }

    LOGM_NOTICE("mpCurUrl %s\n", mpCurUrl);

    if (setupContext(mpCurUrl) != EECode_OK) {
        return EECode_InternalLogicalBug;
    }

    if (EECode_OK != prepare()) {
        return EECode_InternalLogicalBug;
    }

    if (pContext) {
        if (EECode_OK != seekToRequestTime((const SDateTime *)&startTime, (const SDateTime *)(pContext->pRequestTime))) {
            LOGM_ERROR("seekToRequestTime fail!\n");
            return EECode_InternalLogicalBug;
        }
    }

    return EECode_OK;
}

EECode CTSScheduledDemuxer::parsePrivatePacket(TU8 *pData)
{
    STSPrivateDataPesPacketHeader *pPriPesHeader = (STSPrivateDataPesPacketHeader *)pData;
    DASSERT(pPriPesHeader->start_code_prefix_23to16 == 0x00);
    DASSERT(pPriPesHeader->start_code_prefix_15to8 == 0x00);
    DASSERT(pPriPesHeader->start_code_prefix_7to0 == 0x01);
    TU16 packetLength = (pPriPesHeader->packet_len_15to8 << 8) | (pPriPesHeader->packet_len_7to0);
    TU8 *pPayload = pData + sizeof(STSPrivateDataPesPacketHeader);

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
            mCurFileDuration = (pDuration->file_duration_15to8 << 8) | (pDuration->file_duration_7to0);
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


    return EECode_OK;
}

EECode CTSScheduledDemuxer::readPrivatePacket(TU8 type, SPESInfo *pPesInfo)
{
    if (msPrivatePES.enable == 0) {
        LOGM_ERROR("No Private Stream!\n");
        return EECode_Error;
    }
    pPesInfo->pBuffer = NULL;
    pPesInfo->dataSize = 0;
    return readStream(type, pPesInfo, 0);
}

EECode CTSScheduledDemuxer::seekToRequestTime(const SDateTime *pStart, const SDateTime *pRequest)
{
    DASSERT(pStart);
    DASSERT(pRequest);

    TTime diffSeconds = 0;
    if (DLikely((pRequest->year == pStart->year) && (pRequest->month == pStart->month) && (pRequest->day == pStart->day))) {
        diffSeconds = (TTime)(pRequest->hour - pStart->hour) * 3600 + (TTime)pRequest->minute * 60 - (TTime)pStart->minute * 60 + (TTime)pRequest->seconds - (TTime)pStart->seconds;
    } else {
        //diffSeconds = (TU32)(24 - pStart->hour)*3600 + pStart->minute*60 - pStart->minute*60 + pStart->seconds - pStart->seconds;
    }
    if (diffSeconds <= 0) {
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

    //TUint gopIndexMaxLength = MPEG_TS_TP_PACKET_SIZE - MPEG_TS_TP_PACKET_HEADER_SIZE - PES_HEADER_LEN - sizeof(STSPrivateDataHeader);
    TIOSize targetPosition = mCurDuration * 2;
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
                    targetPosition = offset * MPEG_TS_TP_PACKET_SIZE;
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

#endif

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

