/*******************************************************************************
 * uploading_receiver.cpp
 *
 * History:
 *    2013/12/02 - [Zhi He] create file
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

#ifdef DCONFIG_COMPILE_OBSOLETE

#include "common_types.h"
#include "common_osal.h"
#include "common_utils.h"

#include "common_log.h"

#include "common_base.h"

#include "common_network_utils.h"

#include "cloud_lib_if.h"

#include "media_mw_if.h"
#include "media_mw_utils.h"

#include "framework_interface.h"
#include "mw_internal.h"
#include "codec_misc.h"
#include "dsp_platform_interface.h"
#include "streaming_if.h"
#include "modules_interface.h"

#include "uploading_receiver.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

//#define DLOCAL_DEBUG_VERBOSE
static TUint getDemuxerIndexFromDataType(TU16 type)
{
    switch (type) {

        case ESACPDataChannelSubType_H264_NALU:
            return EDemuxerVideoOutputPinIndex;
            break;

        case ESACPDataChannelSubType_AAC:
        case ESACPDataChannelSubType_MPEG12Audio:
        case ESACPDataChannelSubType_G711_PCMU:
        case ESACPDataChannelSubType_G711_PCMA:
            return EDemuxerAudioOutputPinIndex;
            break;

        default:
            LOG_FATAL("BAD data type %d\n", type);
            break;
    }

    return EDemuxerOutputPinNumber;
}

static EECode __release_ring_buffer(CIBuffer *pBuffer)
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

static void appendVideoExtradata(TU8 *p_data, TInt len, TU8 *&mpVideoExtraData, TMemSize &mVideoExtraDataLen, TMemSize &mVideoExtraDataBufLen, TUint plus_0)
{
    TInt mlen = len + 8;
    TU8 *pstart;

    if (mlen < 320) {
        mlen = 320;
    }
    if (!mpVideoExtraData) {
        mpVideoExtraData = (TU8 *)DDBG_MALLOC(mlen, "dmVe");
        mVideoExtraDataBufLen = mlen;
        DASSERT(!mVideoExtraDataLen);
        mVideoExtraDataLen = 0;
    }

    if ((mVideoExtraDataLen + 8 + len) > mVideoExtraDataBufLen) {
        LOG_WARN("!!extra data buffer not enough, is it correct here, mVideoExtraDataLen %ld, len %d, mVideoExtraDataBufLen %ld?\n", mVideoExtraDataLen, len, mVideoExtraDataBufLen);
        mVideoExtraDataBufLen = mVideoExtraDataLen + 8 + len;
        pstart = (TU8 *)DDBG_MALLOC(mVideoExtraDataBufLen, "dmVe");
        DASSERT(pstart);
        if (plus_0) {
            pstart[0] = 0;
            memcpy(pstart + 1, mpVideoExtraData, mVideoExtraDataLen);
            mVideoExtraDataLen += 1;
        } else {
            memcpy(pstart, mpVideoExtraData, mVideoExtraDataLen);
        }
        free(mpVideoExtraData);
        mpVideoExtraData = pstart;
    }

    pstart = mpVideoExtraData + mVideoExtraDataLen;

    memcpy(pstart, p_data, len);
    mVideoExtraDataLen += len;
    DASSERT(mVideoExtraDataLen < mVideoExtraDataBufLen);

}

IDemuxer *gfCreateUploadingReceiver(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TUint index)
{
    return CUploadingReceiver::Create(pName, pPersistMediaConfig, pEngineMsgSink, index);
}

//-----------------------------------------------------------------------
//
// CUploadingReceiver
//
//-----------------------------------------------------------------------
CUploadingReceiver::CUploadingReceiver(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink, TUint index)
    : inherited(pname, index)
    , mpPersistMediaConfig(pPersistMediaConfig)
    , mpMsgSink(pMsgSink)
    , mbAddedToScheduler(0)
    , mbRunning(0)
    , mbUpdateParams(0)
    , mpScheduler(NULL)
    , mpAgent(NULL)
    , mVideoWidth(320)
    , mVideoHeight(240)
    , mVideoFramerateNum(DDefaultVideoFramerateNum)
    , mVideoFramerateDen(DDefaultVideoFramerateDen)
    , mVideoFramerate(((float)DDefaultVideoFramerateNum) / ((float)DDefaultVideoFramerateDen))
    , mVideoBitrate(100000)
    , mpVideoExtraData(NULL)
    , mVideoExtraDataSize(0)
    , mVideoExtraBufferSize(0)
    , mpAudioExtraData(NULL)
    , mAudioExtraDataSize(0)
    , mAudioExtraBufferSize(0)
    //    , mpDumpFile(NULL)
{
    TUint i = 0;
    memset(&mStreamCodecInfos, 0x0, sizeof(SStreamCodecInfos));

    for (i = 0; i < EDemuxerOutputPinNumber; i ++) {
        mpOutputpins[i] = NULL;
        mpBufferpools[i] = NULL;

        mpDataPacketStart[i] = NULL;
        mPacketRemainningSize[i] = 0;
        mPacketSize[i] = 0;

        mbPacketStarted[i] = 0;
    }

    mPacketMaxSize[EDemuxerVideoOutputPinIndex] = 256 * 1024;
    mPacketMaxSize[EDemuxerAudioOutputPinIndex] = 16 * 1024;
    mPacketMaxSize[EDemuxerSubtitleOutputPinIndex] = 8 * 1024;
    mPacketMaxSize[EDemuxerPrivateDataOutputPinIndex] = 8 * 1024;
}

IDemuxer *CUploadingReceiver::Create(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink, TUint index)
{
    CUploadingReceiver *result = new CUploadingReceiver(pname, pPersistMediaConfig, pMsgSink, index);
    if (result && result->Construct() != EECode_OK) {
        LOG_ERROR("CUploadingReceiver->Construct() fail\n");
        delete result;
        result = NULL;
    }
    return result;
}

CObject *CUploadingReceiver::GetObject0() const
{
    (CObject *) this;
}

EECode CUploadingReceiver::Construct()
{
    DSET_MODULE_LOG_CONFIG(ELogModuleUploadingReceiver);

    mpScheduler = gfGetNetworkReceiverTCPScheduler(mpPersistMediaConfig, mIndex);

#if 0
    //clear dump file
    mpDumpFile = fopen("dump.h264", "wb");

    if (DLikely(mpDumpFile)) {
        fclose(mpDumpFile);
        mpDumpFile = NULL;
    } else {
        LOG_WARN("open mpDumpFile fail.\n");
    }
#endif

    return EECode_OK;
}

CUploadingReceiver::~CUploadingReceiver()
{
    if (mpVideoExtraData) {
        free(mpVideoExtraData);
        mpVideoExtraData = NULL;
    }

    if (mpAudioExtraData) {
        free(mpAudioExtraData);
        mpAudioExtraData = NULL;
    }
}

EECode CUploadingReceiver::SetupOutput(COutputPin *p_output_pins[], CBufferPool *p_bufferpools[],  IMemPool *p_mempools[], IMsgSink *p_msg_sink)
{
    //debug assert
    DASSERT(!mpOutputpins[EDemuxerVideoOutputPinIndex]);
    DASSERT(!mpOutputpins[EDemuxerAudioOutputPinIndex]);
    DASSERT(!mpBufferpools[EDemuxerVideoOutputPinIndex]);
    DASSERT(!mpBufferpools[EDemuxerAudioOutputPinIndex]);
    DASSERT(p_msg_sink);

    mpOutputpins[EDemuxerVideoOutputPinIndex] = p_output_pins[EDemuxerVideoOutputPinIndex];
    mpOutputpins[EDemuxerAudioOutputPinIndex] = p_output_pins[EDemuxerAudioOutputPinIndex];
    mpBufferpools[EDemuxerVideoOutputPinIndex] = p_bufferpools[EDemuxerVideoOutputPinIndex];
    mpBufferpools[EDemuxerAudioOutputPinIndex] = p_bufferpools[EDemuxerAudioOutputPinIndex];

    if (p_msg_sink) {
        mpMsgSink = p_msg_sink;
    }

    if (mpOutputpins[EDemuxerVideoOutputPinIndex] && mpBufferpools[EDemuxerVideoOutputPinIndex]) {
        mpBufferpools[EDemuxerVideoOutputPinIndex]->SetReleaseBufferCallBack(__release_ring_buffer);
        mpMemorypools[EDemuxerVideoOutputPinIndex] = CRingMemPool::Create(2 * 1024 * 1024);
    }

    if (mpOutputpins[EDemuxerAudioOutputPinIndex] && mpBufferpools[EDemuxerAudioOutputPinIndex]) {
        mpBufferpools[EDemuxerAudioOutputPinIndex]->SetReleaseBufferCallBack(__release_ring_buffer);
        mpMemorypools[EDemuxerAudioOutputPinIndex] = CRingMemPool::Create(256 * 1024);
    }

    return EECode_OK;
}

EECode CUploadingReceiver::SetupContext(TChar *url, void *p_agent, TU8 priority, TU32 request_receive_buffer_size, TU32 request_send_buffer_size)
{
    DASSERT(p_agent);
    DASSERT(!mpAgent);
    DASSERT(mpScheduler);

    if (DUnlikely(mpAgent && mpScheduler)) {
        if (mbAddedToScheduler) {
            mpScheduler->RemoveScheduledCilent((IScheduledClient *) mpAgent);
            mbAddedToScheduler = 0;
        }
        mpAgent = NULL;
    }

    mpAgent = (ICloudServerAgent *) p_agent;

    if (DLikely(mpAgent && mpScheduler)) {
        mpAgent->SetProcessCMDCallBack((void *)this, CmdCallback);
        mpAgent->SetProcessDataCallBack((void *)this, DataCallback);

        DASSERT(!mbAddedToScheduler);
        mpScheduler->AddScheduledCilent((IScheduledClient *) mpAgent);
        mbAddedToScheduler = 1;
    }

    return EECode_OK;
}

EECode CUploadingReceiver::DestroyContext()
{
    DASSERT(mpScheduler);

    if (DLikely(mpAgent && mpScheduler)) {
        if (mbAddedToScheduler) {
            mpScheduler->RemoveScheduledCilent((IScheduledClient *) mpAgent);
            mbAddedToScheduler = 0;
        }
        mpAgent = NULL;
    }

    mbUpdateParams = 0;

    if (mpVideoExtraData) {
        free(mpVideoExtraData);
        mpVideoExtraData = NULL;
    }
    mVideoExtraDataSize = 0;
    mVideoExtraBufferSize = 0;

    if (mpAudioExtraData) {
        free(mpAudioExtraData);
        mpAudioExtraData = NULL;
    }
    mAudioExtraDataSize = 0;
    mAudioExtraBufferSize = 0;

    TUint i = 0;
    for (i = 0; i < EDemuxerOutputPinNumber; i ++) {
        if (mpDataPacketStart[i] && mPacketSize[i]) {
            if (DLikely(mpMemorypools[i])) {
                mpMemorypools[i]->ReturnBackMemBlock(mPacketSize[i], mpDataPacketStart[i]);
            }
        }

        mpDataPacketStart[i] = NULL;
        mPacketRemainningSize[i] = 0;
        mPacketSize[i] = 0;

        mbPacketStarted[i] = 0;
    }

    return EECode_OK;
}

EECode CUploadingReceiver::ReconnectServer()
{
    LOGM_WARN("to do\n");
    return EECode_OK;
}

EECode CUploadingReceiver::Start()
{
    if (DLikely(mpAgent && mpScheduler)) {
        if (!mbAddedToScheduler) {
            mpScheduler->AddScheduledCilent((IScheduledClient *) mpAgent);
            mbAddedToScheduler = 1;
        } else {
            LOGM_WARN("Already added to scheduler?\n");
        }
    } else {
        LOGM_ERROR("NULL mpAgent %p or mpScheduler %p\n", mpAgent, mpScheduler);
        //return EECode_BadState;
    }

    return EECode_OK;
}

EECode CUploadingReceiver::Stop()
{
    if (DLikely(mpAgent && mpScheduler)) {
        if (mbAddedToScheduler) {
            mpScheduler->RemoveScheduledCilent((IScheduledClient *) mpAgent);
            mbAddedToScheduler = 0;
        } else {
            LOGM_WARN("Already added to scheduler?\n");
        }
    } else {
        LOGM_ERROR("NULL mpAgent %p or mpScheduler %p\n", mpAgent, mpScheduler);
        //return EECode_BadState;
    }

    return EECode_OK;
}

EECode CUploadingReceiver::Suspend()
{
    LOGM_WARN("Suspend() is not implemented yet\n");
    return EECode_OK;
}

EECode CUploadingReceiver::Pause()
{
    LOGM_WARN("Pause() is not implemented yet\n");
    return EECode_OK;
}

EECode CUploadingReceiver::Resume()
{
    LOGM_WARN("Resume() is not implemented yet\n");
    return EECode_OK;
}

EECode CUploadingReceiver::Purge()
{
    LOGM_WARN("Purge() is not implemented yet\n");
    return EECode_OK;
}

EECode CUploadingReceiver::Flush()
{
    LOGM_NOTICE("not implemented in this module.\n");
    return EECode_NoImplement;
}

EECode CUploadingReceiver::ResumeFlush()
{
    LOGM_NOTICE("not implemented in this module.\n");
    return EECode_NoImplement;
}

EECode CUploadingReceiver::SetPbRule(TU8 direction, TU8 feeding_rule, TU16 speed)
{
    LOGM_NOTICE("not implemented in this module.\n");
    return EECode_NoImplement;
}

EECode CUploadingReceiver::SetPbLoopMode(TU32 *p_loop_mode)
{
    LOGM_NOTICE("not implemented in this module.\n");
    return EECode_NoImplement;
}

EECode CUploadingReceiver::EnableVideo(TU32 enable)
{
    LOGM_ERROR("not implemented in this module.\n");
    return EECode_NoImplement;
}

EECode CUploadingReceiver::EnableAudio(TU32 enable)
{
    LOGM_ERROR("not implemented in this module.\n");
    return EECode_NoImplement;
}

void CUploadingReceiver::Delete()
{
    LOGM_INFO("CUploadingReceiver::Delete().\n");

    if (DLikely(mpAgent && mpScheduler)) {
        if (mbAddedToScheduler) {
            mpScheduler->RemoveScheduledCilent((IScheduledClient *) mpAgent);
            mbAddedToScheduler = 0;
        }
    }

    if (mpVideoExtraData) {
        free(mpVideoExtraData);
        mpVideoExtraData = NULL;
    }

    if (mpAudioExtraData) {
        free(mpAudioExtraData);
        mpAudioExtraData = NULL;
    }

}

EECode CUploadingReceiver::GetInfo(SStreamCodecInfos *&pinfos)
{
    pinfos = &mStreamCodecInfos;
    return EECode_OK;
}

EECode CUploadingReceiver::Seek(TTime &ms, ENavigationPosition position)
{
    LOGM_INFO("CUploadingReceiver not support seek\n");
    return EECode_OK;
}

EECode CUploadingReceiver::QueryContentInfo(const SStreamCodecInfos *&pinfos) const
{
    pinfos = &mStreamCodecInfos;
    return EECode_OK;
}

void CUploadingReceiver::PrintStatus()
{
    LOGM_PRINTF("heart beat %d\n", mDebugHeartBeat);
    mDebugHeartBeat = 0;
}

EECode CUploadingReceiver::CmdCallback(void *owner, TU32 cmd_type, TU32 params0, TU32 params1, TU32 params2, TU32 params3, TU32 params4, TU8 *pcmd, TU32 cmdsize)
{
    CUploadingReceiver *thiz = (CUploadingReceiver *)owner;

    if (DLikely(thiz)) {
        return thiz->ProcessCmdCallback(cmd_type, params0, params1, params2, params3, params4);
    }

    LOG_FATAL("NULL onwer\n");
    return EECode_BadState;
}

EECode CUploadingReceiver::DataCallback(void *owner, TMemSize read_length, TU16 data_type, TU8 extra_flag)
{
    CUploadingReceiver *thiz = (CUploadingReceiver *)owner;

    if (DLikely(thiz)) {
        return thiz->ProcessDataCallback(read_length, data_type, extra_flag);
    }

    LOG_FATAL("NULL onwer\n");
    return EECode_BadState;
}

EECode CUploadingReceiver::ProcessCmdCallback(TU32 cmd_type, TU32 params0, TU32 params1, TU32 params2, TU32 params3, TU32 params4)
{
    if (DLikely(mpMsgSink)) {

        ECMDType type = gfSACPClientSubCmdTypeToGenericCmdType((TU16)cmd_type);

        if (DUnlikely(ECMDType_CloudSinkClient_UpdateFrameRate == type)) {
            if (DLikely(params0 && (params0 < 256))) {
                mVideoFramerate = params0;
                mVideoFramerateDen = mVideoFramerateNum / params0;
                mbUpdateParams = 1;
                LOGM_NOTICE("[cloud uploader]: set framerate %d\n", params0);
            } else {
                LOGM_ERROR("BAD framerate %d\n", params0);
            }
        }

        SMSG msg;
        mDebugHeartBeat ++;

        memset(&msg, 0x0, sizeof(SMSG));
        msg.code = type;
        msg.owner_index = mIndex;
        msg.owner_type = EGenericComponentType_Demuxer;

        msg.p_agent_pointer = (TULong)mpAgent;

        msg.p0 = params0;
        msg.p1 = params1;
        msg.p2 = params2;
        msg.p3 = params3;
        msg.p4 = params4;

        return mpMsgSink->MsgProc(msg);
    }

    LOG_FATAL("NULL msg sink\n");
    return EECode_BadState;
}

EECode CUploadingReceiver::ProcessDataCallback(TMemSize read_length, TU16 data_type, TU8 extra_flag)
{
    TUint index = getDemuxerIndexFromDataType(data_type);
    mDebugHeartBeat ++;

    if (DUnlikely((index != EDemuxerVideoOutputPinIndex) && (index != EDemuxerAudioOutputPinIndex))) {
        LOGM_FATAL("client bugs, invalid data type %d\n", data_type);
        return EECode_BadParam;
    }

    //LOGM_INFO("begin: mpDataPacketStart[%d], %p\n", index, mpDataPacketStart[index]);

    if (DUnlikely(read_length >= mPacketMaxSize[index])) {
        if (DUnlikely(read_length >= (mPacketMaxSize[index] * 2))) {
            if (DUnlikely(read_length >= (mPacketMaxSize[index] * 4))) {
                LOGM_FATAL("too big packet, please check code, %ld, %ld\n", read_length, mPacketMaxSize[index]);
                return EECode_BadParam;
            } else {
                mPacketMaxSize[index] = mPacketMaxSize[index] * 4;
                LOGM_WARN("too big packet, enlarge pre alloc memory block X4, %ld, %ld\n", read_length, mPacketMaxSize[index]);
            }
        } else {
            mPacketMaxSize[index] = mPacketMaxSize[index] * 2;
            LOGM_WARN("too big packet, enlarge pre alloc memory block X2, %ld, %ld\n", read_length, mPacketMaxSize[index]);
        }
    }

    DASSERT(mpBufferpools[index]);
    DASSERT(mpOutputpins[index]);
    DASSERT(mpMemorypools[index]);

    EECode err = EECode_OK;
    CIBuffer *p_buffer = NULL;

    //audio path
    if (DUnlikely(EDemuxerAudioOutputPinIndex == index)) {
        mpDataPacketStart[index] = mpMemorypools[index]->RequestMemBlock(read_length);
        err = mpAgent->ReadData(mpDataPacketStart[index], read_length);
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("try read %ld fail, %s\n", read_length, gfGetErrorCodeString(err));
            return err;
        }

        //to do
#if 0
        LOGM_NOTICE("mpOutputpins[%d] %p\n", index, mpOutputpins[index]);
        mpOutputpins[index]->AllocBuffer(p_buffer);
        p_buffer->SetDataPtr(mpDataPacketStart[index]);
        p_buffer->SetDataSize(read_length);

        p_buffer->SetBufferType(EBufferType_AudioPCMBuffer);
        p_buffer->SetBufferFlags(0);

        p_buffer->mpCustomizedContent = (void *)mpMemorypools[index];
        p_buffer->mCustomizedContentType = EBufferCustomizedContentType_RingBuffer;
        p_buffer->SetDataPtrBase(mpDataPacketStart[index]);
        p_buffer->SetDataMemorySize(read_length);
        mpOutputpins[index]->SendBuffer(p_buffer);

        mPacketSize[index] = 0;
        mpDataPacketStart[index] = NULL;
        mPacketRemainningSize[index] = 0;
#endif

        return EECode_OK;
    }

    if (DLikely(extra_flag & DSACPHeaderFlagBit_PacketStartIndicator)) {
        DASSERT(!mpDataPacketStart[index]);
        mpDataPacketStart[index] = mpMemorypools[index]->RequestMemBlock(mPacketMaxSize[index]);
        mPacketRemainningSize[index] = mPacketMaxSize[index];
        mPacketSize[index] = 0;
    } else {
        if (DUnlikely(NULL == mpDataPacketStart[index])) {
            LOGM_ERROR("NULL mpDataPacketStart[%d] %p, and not packet start, is_first flag wrong?\n", index, mpDataPacketStart[index]);
            return EECode_BadParam;
        }

        if (DUnlikely(mPacketRemainningSize[index] < read_length)) {
            LOGM_ERROR("pre allocated memory not enough? please check code, read_length %ld, mPacketRemainningSize[index] %ld\n", read_length, mPacketRemainningSize[index]);
            return EECode_BadParam;
        }
    }

    //LOGM_PRINTF("[debug flow]: read data, current size %ld, new size %ld, is_first %d, is_end %d\n", mPacketSize[index], read_length, is_first, is_last);
    err = mpAgent->ReadData(mpDataPacketStart[index] + mPacketSize[index], read_length);
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("try read %ld fail, %s\n", read_length, gfGetErrorCodeString(err));
        return err;
    }
    //LOGM_PRINTF("[debug flow]: read data done\n");

    mPacketSize[index] += read_length;
    mPacketRemainningSize[index] -= read_length;

    if (DLikely(extra_flag & DSACPHeaderFlagBit_PacketEndIndicator)) {
        p_buffer = NULL;
        TUint plus_0 = 0;

        mpMemorypools[index]->ReturnBackMemBlock(mPacketRemainningSize[index], (mpDataPacketStart[index] + mPacketSize[index]));

        TUint h264_nalu_type = 0;
        if (DLikely(mpDataPacketStart[index][2] == 0x01)) {
            h264_nalu_type = mpDataPacketStart[index][3] & 0x1f;
            plus_0 = 1;
        } else if (DLikely(mpDataPacketStart[index][3] == 0x01)) {
            h264_nalu_type = mpDataPacketStart[index][4] & 0x1f;
        } else {
            LOGM_ERROR("no start code? %02x %02x %02x %02x %02x\n", mpDataPacketStart[index][0], mpDataPacketStart[index][1], mpDataPacketStart[index][2], mpDataPacketStart[index][3], mpDataPacketStart[index][4]);
            return EECode_OK;
        }

        if (DUnlikely(ENalType_SPS == h264_nalu_type)) {
            DASSERT(extra_flag & DSACPHeaderFlagBit_PacketStartIndicator);
            //LOGM_NOTICE("sps comes, size %ld\n", mPacketSize[index]);
            if (mPacketSize[index] > 256) {
                LOGM_NOTICE("sps comes, total size %ld, try find idr?\n", mPacketSize[index]);
                TU8 *p_idr = gfNALUFindIDR(mpDataPacketStart[index], (TU32)mPacketSize[index]);
                TU8 *p_pps_end = NULL;
                if (p_idr) {
                    TMemSize estimate_extra_data_size = (TMemSize)(p_idr - mpDataPacketStart[index]);
                    p_pps_end = gfNALUFindPPSEnd(mpDataPacketStart[index], (TU32)estimate_extra_data_size);
                    if (p_pps_end) {
                        estimate_extra_data_size = (TMemSize)(p_pps_end - mpDataPacketStart[index]);
                        mVideoExtraDataSize = 0;
                        appendVideoExtradata(mpDataPacketStart[index], estimate_extra_data_size, mpVideoExtraData, mVideoExtraDataSize, mVideoExtraBufferSize, plus_0);

                        mpOutputpins[index]->AllocBuffer(p_buffer);
                        p_buffer->SetDataPtr(mpVideoExtraData);
                        p_buffer->SetDataSize(mVideoExtraDataSize);

                        p_buffer->SetBufferType(EBufferType_VideoExtraData);
                        p_buffer->SetBufferFlags(CIBuffer::SYNC_POINT | CIBuffer::WITH_EXTRA_DATA);

                        p_buffer->mVideoWidth = mVideoWidth;
                        p_buffer->mVideoHeight = mVideoHeight;
                        p_buffer->mVideoRoughFrameRate = mVideoFramerate;
                        if ((1 <= mVideoFramerate) && (240 >= mVideoFramerate)) {
                            p_buffer->mVideoFrameRateNum = DDefaultTimeScale;
                            p_buffer->mVideoFrameRateDen = p_buffer->mVideoFrameRateNum / mVideoFramerate;
                        } else {
                            p_buffer->mVideoFrameRateNum = mVideoFramerateNum;
                            p_buffer->mVideoFrameRateDen = mVideoFramerateDen;
                        }
                        p_buffer->mVideoBitrate = mVideoBitrate;

                        p_buffer->mpCustomizedContent = NULL;
                        p_buffer->mCustomizedContentType = EBufferCustomizedContentType_Invalid;

                        mpOutputpins[index]->SendBuffer(p_buffer);
                    }

                    mpOutputpins[index]->AllocBuffer(p_buffer);
                    if (DLikely(p_idr[2] == 0x01)) {
                        h264_nalu_type = p_idr[3] & 0x1f;
                        plus_0 = 1;
                    } else if (DLikely(p_idr[3] == 0x01)) {
                        h264_nalu_type = p_idr[4] & 0x1f;
                    } else {
                        LOGM_ERROR("no start code? %02x %02x %02x %02x %02x\n", p_idr[0], p_idr[1], p_idr[2], p_idr[3], p_idr[4]);
                        return EECode_OK;
                    }
                    if (DUnlikely(ENalType_IDR == h264_nalu_type)) {
                        p_buffer->SetBufferType(EBufferType_VideoES);
                        p_buffer->SetBufferFlags(CIBuffer::KEY_FRAME);
                        p_buffer->mVideoFrameType = EPredefinedPictureType_IDR;
                    } else {
                        p_buffer->SetBufferType(EBufferType_VideoES);
                        p_buffer->SetBufferFlags(0);
                        p_buffer->mVideoFrameType = EPredefinedPictureType_P;
                    }
                    p_buffer->SetDataPtr(p_idr);
                    p_buffer->SetDataSize(mPacketSize[index] - ((TMemSize)(p_idr - mpDataPacketStart[index])));

                    if (DUnlikely(mbUpdateParams)) {
                        mbUpdateParams = 0;
                        p_buffer->SetBufferFlagBits(CIBuffer::SYNC_POINT);

                        p_buffer->mVideoWidth = mVideoWidth;
                        p_buffer->mVideoHeight = mVideoHeight;
                        p_buffer->mVideoRoughFrameRate = mVideoFramerate;
                        if ((1 <= mVideoFramerate) && (256 >= mVideoFramerate)) {
                            p_buffer->mVideoFrameRateNum = DDefaultTimeScale;
                            p_buffer->mVideoFrameRateDen = p_buffer->mVideoFrameRateNum / mVideoFramerate;
                        } else {
                            p_buffer->mVideoFrameRateNum = mVideoFramerateNum;
                            p_buffer->mVideoFrameRateDen = mVideoFramerateDen;
                        }
                        p_buffer->mVideoBitrate = mVideoBitrate;

                    }
                    //LOGM_PRINTF("[debug flow]: uploader receiver, total size %ld, data %02x %02x %02x %02x, %02x %02x %02x %02x, end data %02x %02x %02x %02x\n", mPacketSize[index], mpDataPacketStart[index][0], mpDataPacketStart[index][1], mpDataPacketStart[index][2], mpDataPacketStart[index][3], mpDataPacketStart[index][4], mpDataPacketStart[index][5], mpDataPacketStart[index][6], mpDataPacketStart[index][7], mpDataPacketStart[index][mPacketSize[index] - 4], mpDataPacketStart[index][mPacketSize[index] - 3], mpDataPacketStart[index][mPacketSize[index] - 2], mpDataPacketStart[index][mPacketSize[index] - 1]);

#if 0
                    //debug dump file
                    mpDumpFile = fopen("dump.h264", "ab");

                    if (DLikely(mpDumpFile)) {
                        fwrite(p_buffer->GetDataPtr(), 1, p_buffer->GetDataSize(), mpDumpFile);
                        fclose(mpDumpFile);
                        mpDumpFile = NULL;
                    } else {
                        LOGM_WARN("open  mpDumpFile fail.\n");
                    }
#endif
                    p_buffer->mpCustomizedContent = (void *)mpMemorypools[index];
                    p_buffer->mCustomizedContentType = EBufferCustomizedContentType_RingBuffer;
                    p_buffer->SetDataPtrBase(mpDataPacketStart[index]);
                    p_buffer->SetDataMemorySize(mPacketSize[index]);
                    mpOutputpins[index]->SendBuffer(p_buffer);

                    mPacketSize[index] = 0;
                    mpDataPacketStart[index] = NULL;
                    mPacketRemainningSize[index] = 0;
                    return EECode_OK;
                } else {
                    LOGM_ERROR("NULL p_idr, when size is %ld\n", mPacketSize[index]);
                    return EECode_OK;
                }
            }
            mVideoExtraDataSize = 0;
            appendVideoExtradata(mpDataPacketStart[index], mPacketSize[index], mpVideoExtraData, mVideoExtraDataSize, mVideoExtraBufferSize, plus_0);

            mpMemorypools[index]->ReturnBackMemBlock(mPacketSize[index], mpDataPacketStart[index]);

            mPacketSize[index] = 0;
            mpDataPacketStart[index] = NULL;
            mPacketRemainningSize[index] = 0;
            return EECode_OK;
        } else if (DUnlikely(ENalType_PPS == h264_nalu_type)) {
            DASSERT(extra_flag & DSACPHeaderFlagBit_PacketStartIndicator);
            if (mPacketSize[index] > 256) {
                LOGM_NOTICE("pps comes, total size %ld, try find idr?\n", mPacketSize[index]);
                TU8 *p_idr = gfNALUFindIDR(mpDataPacketStart[index], (TU32)mPacketSize[index]);
                TU8 *p_pps_end = NULL;
                if (p_idr) {
                    TMemSize estimate_extra_data_size = (TMemSize)(p_idr - mpDataPacketStart[index]);
                    p_pps_end = gfNALUFindPPSEnd(mpDataPacketStart[index], (TU32)estimate_extra_data_size);
                    if (p_pps_end) {
                        estimate_extra_data_size = (TMemSize)(p_pps_end - mpDataPacketStart[index]);
                        appendVideoExtradata(mpDataPacketStart[index], estimate_extra_data_size, mpVideoExtraData, mVideoExtraDataSize, mVideoExtraBufferSize, 0);

                        mpOutputpins[index]->AllocBuffer(p_buffer);
                        p_buffer->SetDataPtr(mpVideoExtraData);
                        p_buffer->SetDataSize(mVideoExtraDataSize);

                        p_buffer->SetBufferType(EBufferType_VideoExtraData);
                        p_buffer->SetBufferFlags(CIBuffer::SYNC_POINT | CIBuffer::WITH_EXTRA_DATA);

                        p_buffer->mVideoWidth = mVideoWidth;
                        p_buffer->mVideoHeight = mVideoHeight;
                        p_buffer->mVideoRoughFrameRate = mVideoFramerate;
                        if ((1 <= mVideoFramerate) && (240 >= mVideoFramerate)) {
                            p_buffer->mVideoFrameRateNum = DDefaultTimeScale;
                            p_buffer->mVideoFrameRateDen = p_buffer->mVideoFrameRateNum / mVideoFramerate;
                        } else {
                            p_buffer->mVideoFrameRateNum = mVideoFramerateNum;
                            p_buffer->mVideoFrameRateDen = mVideoFramerateDen;
                        }
                        p_buffer->mVideoBitrate = mVideoBitrate;

                        p_buffer->mpCustomizedContent = NULL;
                        p_buffer->mCustomizedContentType = EBufferCustomizedContentType_Invalid;

                        mpOutputpins[index]->SendBuffer(p_buffer);
                    }

                    mpOutputpins[index]->AllocBuffer(p_buffer);
                    if (DLikely(p_idr[2] == 0x01)) {
                        h264_nalu_type = p_idr[3] & 0x1f;
                        plus_0 = 1;
                    } else if (DLikely(p_idr[3] == 0x01)) {
                        h264_nalu_type = p_idr[4] & 0x1f;
                    } else {
                        LOGM_ERROR("no start code? %02x %02x %02x %02x %02x\n", p_idr[0], p_idr[1], p_idr[2], p_idr[3], p_idr[4]);
                        return EECode_OK;
                    }
                    if (DUnlikely(ENalType_IDR == h264_nalu_type)) {
                        p_buffer->SetBufferType(EBufferType_VideoES);
                        p_buffer->SetBufferFlags(CIBuffer::KEY_FRAME);
                        p_buffer->mVideoFrameType = EPredefinedPictureType_IDR;
                    } else {
                        p_buffer->SetBufferType(EBufferType_VideoES);
                        p_buffer->SetBufferFlags(0);
                        p_buffer->mVideoFrameType = EPredefinedPictureType_P;
                    }
                    p_buffer->SetDataPtr(p_idr);
                    p_buffer->SetDataSize(mPacketSize[index] - ((TMemSize)(p_idr - mpDataPacketStart[index])));

                    if (DUnlikely(mbUpdateParams)) {
                        mbUpdateParams = 0;
                        p_buffer->SetBufferFlagBits(CIBuffer::SYNC_POINT);

                        p_buffer->mVideoWidth = mVideoWidth;
                        p_buffer->mVideoHeight = mVideoHeight;
                        p_buffer->mVideoRoughFrameRate = mVideoFramerate;
                        if ((1 <= mVideoFramerate) && (256 >= mVideoFramerate)) {
                            p_buffer->mVideoFrameRateNum = DDefaultTimeScale;
                            p_buffer->mVideoFrameRateDen = p_buffer->mVideoFrameRateNum / mVideoFramerate;
                        } else {
                            p_buffer->mVideoFrameRateNum = mVideoFramerateNum;
                            p_buffer->mVideoFrameRateDen = mVideoFramerateDen;
                        }
                        p_buffer->mVideoBitrate = mVideoBitrate;

                    }
                    //LOGM_PRINTF("[debug flow]: uploader receiver, total size %ld, data %02x %02x %02x %02x, %02x %02x %02x %02x, end data %02x %02x %02x %02x\n", mPacketSize[index], mpDataPacketStart[index][0], mpDataPacketStart[index][1], mpDataPacketStart[index][2], mpDataPacketStart[index][3], mpDataPacketStart[index][4], mpDataPacketStart[index][5], mpDataPacketStart[index][6], mpDataPacketStart[index][7], mpDataPacketStart[index][mPacketSize[index] - 4], mpDataPacketStart[index][mPacketSize[index] - 3], mpDataPacketStart[index][mPacketSize[index] - 2], mpDataPacketStart[index][mPacketSize[index] - 1]);

#if 0
                    //debug dump file
                    mpDumpFile = fopen("dump.h264", "ab");

                    if (DLikely(mpDumpFile)) {
                        fwrite(p_buffer->GetDataPtr(), 1, p_buffer->GetDataSize(), mpDumpFile);
                        fclose(mpDumpFile);
                        mpDumpFile = NULL;
                    } else {
                        LOGM_WARN("open  mpDumpFile fail.\n");
                    }
#endif
                    p_buffer->mpCustomizedContent = (void *)mpMemorypools[index];
                    p_buffer->mCustomizedContentType = EBufferCustomizedContentType_RingBuffer;
                    p_buffer->SetDataPtrBase(mpDataPacketStart[index]);
                    p_buffer->SetDataMemorySize(mPacketSize[index]);
                    mpOutputpins[index]->SendBuffer(p_buffer);

                    mPacketSize[index] = 0;
                    mpDataPacketStart[index] = NULL;
                    mPacketRemainningSize[index] = 0;
                    return EECode_OK;
                } else {
                    LOGM_ERROR("NULL p_idr, when size is %ld\n", mPacketSize[index]);
                    return EECode_OK;
                }
            }
            //LOGM_NOTICE("pps comes, size %ld\n", mPacketSize[index]);
            appendVideoExtradata(mpDataPacketStart[index], mPacketSize[index], mpVideoExtraData, mVideoExtraDataSize, mVideoExtraBufferSize, 0);

            mpMemorypools[index]->ReturnBackMemBlock(mPacketSize[index], mpDataPacketStart[index]);

            mpOutputpins[index]->AllocBuffer(p_buffer);
            p_buffer->SetDataPtr(mpVideoExtraData);
            p_buffer->SetDataSize(mVideoExtraDataSize);

            p_buffer->SetBufferType(EBufferType_VideoExtraData);
            p_buffer->SetBufferFlags(CIBuffer::SYNC_POINT | CIBuffer::WITH_EXTRA_DATA);

            p_buffer->mVideoWidth = mVideoWidth;
            p_buffer->mVideoHeight = mVideoHeight;
            p_buffer->mVideoRoughFrameRate = mVideoFramerate;
            if ((1 <= mVideoFramerate) && (240 >= mVideoFramerate)) {
                p_buffer->mVideoFrameRateNum = DDefaultTimeScale;
                p_buffer->mVideoFrameRateDen = p_buffer->mVideoFrameRateNum / mVideoFramerate;
            } else {
                p_buffer->mVideoFrameRateNum = mVideoFramerateNum;
                p_buffer->mVideoFrameRateDen = mVideoFramerateDen;
            }
            p_buffer->mVideoBitrate = mVideoBitrate;

            p_buffer->mpCustomizedContent = NULL;
            p_buffer->mCustomizedContentType = EBufferCustomizedContentType_Invalid;

            mpOutputpins[index]->SendBuffer(p_buffer);

            //LOGM_NOTICE("[debug flow]: uploader receiver, sps+pps total size %ld, first bytes %02x %02x %02x %02x, %02x %02x %02x %02x\n", mVideoExtraDataSize, mpVideoExtraData[0], mpVideoExtraData[1], mpVideoExtraData[2], mpVideoExtraData[3], mpVideoExtraData[4], mpVideoExtraData[5], mpVideoExtraData[6], mpVideoExtraData[7]);

#if 0
            //debug dump file
            mpDumpFile = fopen("dump.h264", "ab");

            if (DLikely(mpDumpFile)) {
                fwrite(p_buffer->GetDataPtr(), 1, p_buffer->GetDataSize(), mpDumpFile);
                fclose(mpDumpFile);
                mpDumpFile = NULL;
            } else {
                LOGM_WARN("open  mpDumpFile fail.\n");
            }
#endif

            mPacketSize[index] = 0;
            mpDataPacketStart[index] = NULL;
            mPacketRemainningSize[index] = 0;
            return EECode_OK;
        }

        mpOutputpins[index]->AllocBuffer(p_buffer);
        if (DUnlikely(ENalType_IDR == h264_nalu_type)) {
            p_buffer->SetBufferType(EBufferType_VideoES);
            p_buffer->SetBufferFlags(CIBuffer::KEY_FRAME);
            p_buffer->mVideoFrameType = EPredefinedPictureType_IDR;
        } else {
            p_buffer->SetBufferType(EBufferType_VideoES);
            p_buffer->SetBufferFlags(0);
            p_buffer->mVideoFrameType = EPredefinedPictureType_P;
        }
        p_buffer->SetDataPtr(mpDataPacketStart[index]);
        p_buffer->SetDataSize(mPacketSize[index]);

        if (DUnlikely(mbUpdateParams)) {
            mbUpdateParams = 0;
            p_buffer->SetBufferFlagBits(CIBuffer::SYNC_POINT);

            p_buffer->mVideoWidth = mVideoWidth;
            p_buffer->mVideoHeight = mVideoHeight;
            p_buffer->mVideoRoughFrameRate = mVideoFramerate;
            if ((1 <= mVideoFramerate) && (256 >= mVideoFramerate)) {
                p_buffer->mVideoFrameRateNum = DDefaultTimeScale;
                p_buffer->mVideoFrameRateDen = p_buffer->mVideoFrameRateNum / mVideoFramerate;
            } else {
                p_buffer->mVideoFrameRateNum = mVideoFramerateNum;
                p_buffer->mVideoFrameRateDen = mVideoFramerateDen;
            }
            p_buffer->mVideoBitrate = mVideoBitrate;

        }
        //LOGM_PRINTF("[debug flow]: uploader receiver, total size %ld, data %02x %02x %02x %02x, %02x %02x %02x %02x, end data %02x %02x %02x %02x\n", mPacketSize[index], mpDataPacketStart[index][0], mpDataPacketStart[index][1], mpDataPacketStart[index][2], mpDataPacketStart[index][3], mpDataPacketStart[index][4], mpDataPacketStart[index][5], mpDataPacketStart[index][6], mpDataPacketStart[index][7], mpDataPacketStart[index][mPacketSize[index] - 4], mpDataPacketStart[index][mPacketSize[index] - 3], mpDataPacketStart[index][mPacketSize[index] - 2], mpDataPacketStart[index][mPacketSize[index] - 1]);

#if 0
        //debug dump file
        mpDumpFile = fopen("dump.h264", "ab");

        if (DLikely(mpDumpFile)) {
            fwrite(p_buffer->GetDataPtr(), 1, p_buffer->GetDataSize(), mpDumpFile);
            fclose(mpDumpFile);
            mpDumpFile = NULL;
        } else {
            LOGM_WARN("open  mpDumpFile fail.\n");
        }
#endif
        p_buffer->mpCustomizedContent = (void *)mpMemorypools[index];
        p_buffer->mCustomizedContentType = EBufferCustomizedContentType_RingBuffer;
        p_buffer->SetDataPtrBase(mpDataPacketStart[index]);
        p_buffer->SetDataMemorySize(mPacketSize[index]);
        mpOutputpins[index]->SendBuffer(p_buffer);

        mPacketSize[index] = 0;
        mpDataPacketStart[index] = NULL;
        mPacketRemainningSize[index] = 0;
    }

    //LOGM_INFO("end: mpDataPacketStart[%d], %p\n", index, mpDataPacketStart[index]);

    return EECode_OK;
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

#endif

