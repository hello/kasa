/*******************************************************************************
 * rtp_mpeg12audio_scheduled_receiver.cpp
 *
 * History:
 *    2013/07/19 - [Zhi He] create file
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

#include "common_network_utils.h"

#include "media_mw_if.h"
#include "media_mw_utils.h"

#include "framework_interface.h"
#include "mw_internal.h"
#include "dsp_platform_interface.h"
#include "streaming_if.h"
#include "modules_interface.h"

#include "rtp_mpeg12audio_scheduled_receiver.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

static void __parseRTPPacketHeader(TU8 *p_packet, TTime &time_stamp, TU16 &seq_number, TU32 &ssrc)
{
    DASSERT(p_packet);
    if (p_packet) {
        time_stamp = (p_packet[7]) | (p_packet[6] << 8) | (p_packet[5] << 16) | (p_packet[4] << 24);
        ssrc = (p_packet[11]) | (p_packet[10] << 8) | (p_packet[9] << 16) | (p_packet[8] << 24);
        seq_number = (p_packet[2] << 8) | (p_packet[3]);
    }
}

CRTPMpeg12AudioScheduledReceiver::CRTPMpeg12AudioScheduledReceiver(TUint index, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink)
    : inherited("CRTPMpeg12AudioScheduledReceiver", index)
    , mTrackID(0)
    , mbRun(1)
    , mbInitialized(0)
    , mType(StreamType_Audio)
    , mFormat(StreamFormat_MPEG12Audio)
    , mpMsgSink(pMsgSink)
    , mpPersistMediaConfig(pPersistMediaConfig)
    , mpOutputPin(NULL)
    , mpBufferPool(NULL)
    , mpMemPool(NULL)
    , mRTSPSocket(-1)
    , mRTPSocket(-1)
    , mRTCPSocket(-1)
    , mpCmdSimpleQueue(NULL)
    , msState(DATA_THREAD_STATE_READ_FIRST_RTP_PACKET)
    , mRTPHeaderLength(16)
    , mpMemoryStart(NULL)
    , mReadLen(0)
    , mRTPTimeStamp(0)
    , mRTPCurrentSeqNumber(0)
    , mRTPLastSeqNumber(0)
    , mpBuffer(NULL)
    , mRequestMemorySize(1024)
    , mCmd(0)
    , mDebugWaitMempoolFlag(0)
    , mDebugWaitBufferpoolFlag(0)
    , mDebugWaitReadSocketFlag(0)
    , mbGetSSRC(0)
    , mAudioChannelNumber(DDefaultAudioChannelNumber)
    , mAudioSampleFormat(AudioSampleFMT_S16)
    , mbAudioChannelInterlave(0)
    , mAudioSamplerate(DDefaultAudioSampleRate)
    , mAudioBitrate(12800)
    , mAudioFrameSize(1024)
    , mbSendSyncPointBuffer(0)
    , mPriority(0)
    , mBufferSessionNumber(0)
    , mpExtraData(NULL)
    , mnExtraDataSize(0)
    , mLastRtcpNtpTime(0)
    , mFirstRtcpNtpTime(0)
    , mLastRtcpTimeStamp(0)
    , mRtcpTimeStampOffset(0)
    , mPacketCount(0)
    , mOctetCount(0)
    , mLastOctetCount(0)
    , mRTCPCurrentTick(0)
    , mRTCPCoolDown(768)
    , mRTCPDataLen(0)
    , mRTCPReadDataLen(0)
    , mFakePts(0)
    , mLastPrintLostCount(0)
{
    memset(&mRTPStatistics, 0x0, sizeof(mRTPStatistics));

    mRTPStatistics.probation = 1;
}

CRTPMpeg12AudioScheduledReceiver::~CRTPMpeg12AudioScheduledReceiver()
{
    if (mpExtraData) {
        free(mpExtraData);
        mpExtraData = NULL;
    }
}

EECode CRTPMpeg12AudioScheduledReceiver::Initialize(SRTPContext *context, SStreamCodecInfo *p_stream_info, TUint number_of_content)
{
    DASSERT(context);
    DASSERT(p_stream_info);

    if (mbInitialized) {
        LOGM_FATAL("already initialized?\n");
        return EECode_BadState;
    }

    if (context) {
        struct sockaddr_in *pAddr = NULL;

        mType = context->type;
        mFormat = context->format;
        mpOutputPin = context->outpin;
        mpBufferPool = context->bufferpool;
        mpMemPool = context->mempool;
        mRTPSocket = context->rtp_socket;
        mRTCPSocket = context->rtcp_socket;
        mPriority = context->priority;

        memset(&mSrcAddr, 0x0, sizeof(mSrcAddr));
        pAddr = (struct sockaddr_in *)&mSrcAddr;
        pAddr->sin_family = AF_INET;
        pAddr->sin_port = htons(context->server_rtp_port);
        pAddr->sin_addr.s_addr = inet_addr(context->server_addr);
        LOGM_INFO("audio rtp port %hu, addr %s, mServerAddrIn.sin_addr.s_addr 0x%x.\n", context->server_rtp_port, context->server_addr, pAddr->sin_addr.s_addr);

        memset(&mSrcRTCPAddr, 0x0, sizeof(mSrcRTCPAddr));
        pAddr = (struct sockaddr_in *)&mSrcRTCPAddr;
        pAddr->sin_family = AF_INET;
        pAddr->sin_port = htons(context->server_rtcp_port);
        pAddr->sin_addr.s_addr = inet_addr(context->server_addr);
        LOGM_INFO("audio rtcp port %hu, addr %s, mServerAddrIn.sin_addr.s_addr 0x%x.\n", context->server_rtcp_port, context->server_addr, pAddr->sin_addr.s_addr);

        //debug assert
        DASSERT(StreamType_Audio == mType);
        DASSERT(StreamFormat_MPEG12Audio == mFormat);
        DASSERT(mpOutputPin);
        DASSERT(mpBufferPool);
        DASSERT(mpMemPool);
        DASSERT(mRTPSocket > 0);
        DASSERT(mRTCPSocket > 0);

        memset(mCName, 0x0, sizeof(mCName));
        gfOSGetHostName(mCName, sizeof(mCName));
        DASSERT(strlen(mCName) < (sizeof(mCName) + 16));
        sprintf(mCName, "%s_%d", mCName, context->index);
        //LOGM_NOTICE("[debug]: mCName %s\n", mCName);

        mbInitialized = 1;

        if (mpBufferPool->GetFreeBufferCnt()) {
            mDebugWaitBufferpoolFlag = 1;
            if (!mpOutputPin->AllocBuffer(mpBuffer)) {
                LOG_ERROR("demuxer AllocBuffer Fail, must not comes here!\n");
                mbRun = 0;
            }
            mDebugWaitBufferpoolFlag = 0;

            mDebugWaitMempoolFlag = 1;
            mpMemoryStart = mpMemPool->RequestMemBlock(mRequestMemorySize);
            mDebugWaitMempoolFlag = 0;
            mpBuffer->mpCustomizedContent = (void *)mpMemPool;
            mpBuffer->mCustomizedContentType = EBufferCustomizedContentType_RingBuffer;

            DASSERT(mpMemoryStart);
            msState = DATA_THREAD_STATE_READ_FIRST_RTP_PACKET;
        } else {
            LOG_ERROR("why no free buffer at start?\n");
            msState = DATA_THREAD_STATE_WAIT_OUTBUFFER;
        }

    } else {
        LOGM_FATAL("NULL params\n");
        return EECode_BadParam;
    }

    if (p_stream_info) {
        DASSERT(p_stream_info->stream_format == mFormat);
        DASSERT(StreamType_Audio == p_stream_info->stream_type);
        DASSERT(p_stream_info->spec.audio.sample_rate);
        DASSERT(p_stream_info->spec.audio.channel_number);
        //DASSERT(p_stream_info->spec.audio.bitrate);

        mAudioChannelNumber = p_stream_info->spec.audio.channel_number;
        mAudioSamplerate = p_stream_info->spec.audio.sample_rate;
        if (p_stream_info->spec.audio.bitrate) {
            mAudioBitrate = p_stream_info->spec.audio.bitrate;
        }
        if (p_stream_info->spec.audio.sample_format) {
            mAudioSampleFormat = p_stream_info->spec.audio.sample_format;
        }
        mbAudioChannelInterlave = p_stream_info->spec.audio.is_channel_interlave;
    }

    LOGM_INFO("mAudioChannelNumber %d, mAudioSamplerate %d, mAudioBitrate %d, mAudioSampleFormat %d, mbAudioChannelInterlave %d\n", mAudioChannelNumber, mAudioSamplerate, mAudioBitrate, mAudioSampleFormat, mbAudioChannelInterlave);

    return EECode_OK;
}

EECode CRTPMpeg12AudioScheduledReceiver::ReInitialize(SRTPContext *context, SStreamCodecInfo *p_stream_info, TUint number_of_content)
{
    //LOGM_FATAL("\n");
    return EECode_OK;
}

EECode CRTPMpeg12AudioScheduledReceiver::Flush()
{
    LOGM_FATAL("need implement\n");
    return EECode_OK;
}

EECode CRTPMpeg12AudioScheduledReceiver::Purge()
{
    LOGM_FATAL("need implement\n");
    return EECode_OK;
}

EECode CRTPMpeg12AudioScheduledReceiver::SetExtraData(TU8 *p_extradata, TU32 extradata_size, TU32 index)
{
    LOGM_WARN("need implement\n");
    return EECode_OK;
}

EECode CRTPMpeg12AudioScheduledReceiver::Scheduling(TUint times, TU32 inout_mask)
{
    //debug assert
    //DASSERT(mbInitialized);
    //DASSERT(StreamType_Audio == mType);
    //DASSERT(StreamFormat_MPEG12Audio == mFormat);
    //DASSERT(mpOutputPin);
    //DASSERT(mpBufferPool);
    //DASSERT(mpMemPool);
    //DASSERT(mRTPSocket > 0);
    //DASSERT(mRTCPSocket > 0);

    mRTCPCurrentTick ++;
    if (mRTCPCurrentTick > mRTCPCoolDown) {
        //LOGM_NOTICE("before checkSRsendRR()\n");
        checkSRsendRR();
        mRTCPCurrentTick = 0;
    }

    //LOG_NOTICE("[Data thread]: CRTPMpeg12AudioScheduledReceiver Scheduling.\n");

    while (times-- > 0) {
        //LOGM_STATE("Scheduling(%d): msState %d\n", mFormat, msState);
        switch (msState) {

            case DATA_THREAD_STATE_READ_FIRST_RTP_PACKET:
                //DASSERT(mpBuffer);

                //DASSERT(mpMemoryStart);

                if (DUnlikely(!mbSendSyncPointBuffer)) {
                    CIBuffer *ptmp = NULL;

                    if (DUnlikely(!mpOutputPin->AllocBuffer(ptmp))) {
                        LOG_ERROR("demuxer AllocBuffer Fail, must not comes here!\n");
                        mbRun = 0;
                    }

                    ptmp->SetBufferType(EBufferType_AudioExtraData);
                    ptmp->SetBufferFlagBits(CIBuffer::SYNC_POINT | CIBuffer::KEY_FRAME);
                    ptmp->mContentFormat = mFormat;
                    ptmp->mAudioSampleRate = mAudioSamplerate;
                    ptmp->mAudioBitrate = mAudioBitrate;
                    ptmp->mAudioChannelNumber = mAudioChannelNumber;
                    ptmp->mAudioSampleFormat = mAudioSampleFormat;
                    ptmp->mbAudioPCMChannelInterlave = mbAudioChannelInterlave;
                    ptmp->mAudioFrameSize = mAudioFrameSize;

                    ptmp->mpCustomizedContent = NULL;
                    ptmp->mCustomizedContentType = EBufferCustomizedContentType_Invalid;

                    DASSERT(!mpExtraData);
                    DASSERT(!mnExtraDataSize);
                    mpExtraData = gfGenerateAudioExtraData(StreamFormat_MPEG12Audio, mAudioSamplerate, mAudioChannelNumber, mnExtraDataSize);
                    if (mpExtraData) {
                        ptmp->SetDataPtr(mpExtraData);
                        ptmp->SetDataSize(mnExtraDataSize);
                    } else {
                        ptmp->SetDataPtr(NULL);
                        ptmp->SetDataSize(0);
                    }

                    mBufferSessionNumber++;
                    mbSendSyncPointBuffer = 1;

                    //LOGM_NOTICE("[DEBUG]: before send audio extra data buffer\n");

                    ptmp->mSessionNumber = mBufferSessionNumber;
                    mpOutputPin->SendBuffer(ptmp);
                    mDebugHeartBeat ++;
                }

                //LOGM_NOTICE("[Data thread]: mpa data.\n");
                mFromLen = sizeof(struct sockaddr_in);
                mDebugWaitReadSocketFlag = 1;
                mReadLen = gfNet_RecvFrom(mRTPSocket, mpMemoryStart, mRequestMemorySize, 0, (void *)&mSrcAddr, (TSocketSize *)&mFromLen);
                mDebugWaitReadSocketFlag = 0;
                if (DUnlikely(mReadLen < 0)) {
                    perror("recvfrom");
                    LOG_ERROR("recvfrom fail, ret %d, mRTPSocket %d, sa_family %hu, data %02x %02x %02x %02x, %02x %02x %02x %02x, %02x %02x %02x %02x, %02x %02x.\n", mReadLen, mRTPSocket, mSrcAddr.sa_family, \
                              mSrcAddr.sa_data[0], mSrcAddr.sa_data[1], mSrcAddr.sa_data[2], mSrcAddr.sa_data[3], \
                              mSrcAddr.sa_data[4], mSrcAddr.sa_data[5], mSrcAddr.sa_data[6], mSrcAddr.sa_data[7], \
                              mSrcAddr.sa_data[8], mSrcAddr.sa_data[9], mSrcAddr.sa_data[10], mSrcAddr.sa_data[11], \
                              mSrcAddr.sa_data[12], mSrcAddr.sa_data[13]);
                    msState = DATA_THREAD_STATE_ERROR;
                    break;
                }
                //LOG_NOTICE("[Data thread_1 %d]: mReadLen %d.\n", mFormat, mReadLen);
                //DASSERT(mReadLen <= ((TInt)mRequestMemorySize));

                mRTPLastSeqNumber = mRTPCurrentSeqNumber;
                __parseRTPPacketHeader(mpMemoryStart, mRTPTimeStamp, mRTPCurrentSeqNumber, mPacketSSRC);

                //to do, do check on seq number
                if (DLikely(mRTPLastSeqNumber < mRTPCurrentSeqNumber)) {
                    //DASSERT((mRTPLastSeqNumber + 1) == mRTPCurrentSeqNumber);
                    if (DUnlikely((mRTPLastSeqNumber + 1) != mRTPCurrentSeqNumber)) {
                        LOGM_NOTICE("mRTPLastSeqNumber %d, mRTPCurrentSeqNumber %d\n", mRTPLastSeqNumber, mRTPCurrentSeqNumber);
                    }
                }

                if (mRTPStatistics.probation) {
                    mRTPStatistics.probation = 0;
                    mRTPStatistics.base_seq = mRTPCurrentSeqNumber;
                }

                if (mRTPLastSeqNumber > mRTPCurrentSeqNumber) {
                    mRTPStatistics.cycles += (1 << 16);
                }
                mRTPStatistics.received ++;
                mRTPStatistics.max_seq = mRTPCurrentSeqNumber;

                if (DLikely(mbGetSSRC)) {
                    //DASSERT(mPacketSSRC == mServerSSRC);
                    //if (mPacketSSRC != mServerSSRC) {
                    //    LOGM_NOTICE("mPacketSSRC 0x%08x, mServerSSRC 0x%08x\n", mPacketSSRC, mServerSSRC);
                    //}
                } else {
                    mServerSSRC = mPacketSSRC;
                    mSSRC = mServerSSRC + 16;
                    mbGetSSRC = 1;
                    //LOGM_NOTICE("!!mPacketSSRC 0x%08x\n", mPacketSSRC);
                }

                mpBuffer->SetBufferNativePTS(mRTPTimeStamp);
                mpBuffer->SetBufferPTS(mFakePts);
                mFakePts += mAudioFrameSize;

#if 0
                //pttt = mpCurrentPointer;
                //LOGM_NOTICE("start point, pts %08x data %02x %02x %02x %02x, %02x %02x %02x %02x, %02x %02x %02x %02x, %02x %02x %02x %02x.\n", mRTPTimeStamp,
                //        pttt[0], pttt[1], pttt[2], pttt[3],
                //        pttt[4], pttt[5], pttt[6], pttt[7],
                //        pttt[8], pttt[9], pttt[10], pttt[11],
                //        pttt[12], pttt[13], pttt[14], pttt[15]);
#endif

                //DASSERT(mpMemoryStart[1] & 0x80);

                //all in one rtp packet, first packet has marker bit

                //LOG_NOTICE("[Data thread_1, 1]: h264 data one packet, mReadLen %d.\n", mReadLen);

                mpBuffer->SetDataPtr(mpMemoryStart + mRTPHeaderLength);
                mpBuffer->SetBufferType(EBufferType_AudioES);
                mpBuffer->SetBufferFlags(CIBuffer::KEY_FRAME);
                mpBuffer->SetDataSize((TUint)(mReadLen - mRTPHeaderLength));
                mpBuffer->SetDataPtrBase(mpMemoryStart);
                mpBuffer->SetDataMemorySize(mReadLen);

                mpMemPool->ReturnBackMemBlock(mRequestMemorySize - mReadLen, mpMemoryStart + mReadLen);

                //LOG_NOTICE("[Data thread_1, 1]: mTotalWritenLength %d.\n", mReadLen + (mStartCodeLength -1) + 2 - mReservedDataLength);

#ifdef DLOCAL_DEBUG_VERBOSE
                TU8 *pttt = mpMemoryStart;
                LOG_NOTICE("packet is within one rtp packet, mpBuffer %p, data %p, size %d, mpMemoryStart %p\n", mpBuffer, mpBuffer->GetDataPtr(), mpBuffer->GetDataSize(), mpMemoryStart);
                LOG_NOTICE("data: %02x %02x %02x %02x, %02x %02x %02x %02x, %02x %02x %02x %02x, %02x %02x %02x %02x.\n",
                           pttt[0], pttt[1], pttt[2], pttt[3],
                           pttt[4], pttt[5], pttt[6], pttt[7],
                           pttt[8], pttt[9], pttt[10], pttt[11],
                           pttt[12], pttt[13], pttt[14], pttt[15]);
#endif

                mpBuffer->mSessionNumber = mBufferSessionNumber;

                //send packet
                mpOutputPin->SendBuffer(mpBuffer);
                mDebugHeartBeat ++;
                mpBuffer = NULL;

                if (DLikely(mpBufferPool->GetFreeBufferCnt())) {
                    mDebugWaitBufferpoolFlag = 3;
                    if (DUnlikely(!mpOutputPin->AllocBuffer(mpBuffer))) {
                        LOG_ERROR("demuxer AllocBuffer Fail, must not comes here!\n");
                        mbRun = 0;
                        msState = DATA_THREAD_STATE_ERROR;
                        break;
                    }
                    mDebugWaitBufferpoolFlag = 0;

                    mpBuffer->mpCustomizedContent = (void *)mpMemPool;
                    mpBuffer->mCustomizedContentType = EBufferCustomizedContentType_RingBuffer;
                    mDebugWaitMempoolFlag = 3;
                    mpMemoryStart = mpMemPool->RequestMemBlock(mRequestMemorySize);
                    mDebugWaitMempoolFlag = 0;
                } else {
                    mpMemoryStart = NULL;
                    mpBuffer = NULL;
                    msState = DATA_THREAD_STATE_WAIT_OUTBUFFER;
                }
                break;

            case DATA_THREAD_STATE_WAIT_OUTBUFFER:
                //DASSERT(!mpBuffer);

                if (DLikely(mpBufferPool->GetFreeBufferCnt())) {
                    mDebugWaitBufferpoolFlag = 6;
                    if (DUnlikely(!mpOutputPin->AllocBuffer(mpBuffer))) {
                        LOG_ERROR("demuxer AllocBuffer Fail, must not comes here!\n");
                        mbRun = 0;
                    }
                    mDebugWaitBufferpoolFlag = 0;

                    msState = DATA_THREAD_STATE_READ_FIRST_RTP_PACKET;
                    mpBuffer->mpCustomizedContent = (void *)mpMemPool;
                    mpBuffer->mCustomizedContentType = EBufferCustomizedContentType_RingBuffer;
                    mDebugWaitMempoolFlag = 6;
                    mpMemoryStart = mpMemPool->RequestMemBlock(mRequestMemorySize);
                    mDebugWaitMempoolFlag = 0;
                    times ++;
                    break;
                } else {
                    return EECode_NotRunning;
                }
                break;

            case DATA_THREAD_STATE_SKIP_DATA:
                // to do, skip till next IDR
                LOG_ERROR("add implenment.\n");
                break;

            case DATA_THREAD_STATE_ERROR:
                // to do, error case
                LOG_ERROR("error state, DATA_THREAD_STATE_ERROR\n");
                break;

            default:
                LOG_ERROR("unexpected msState %d\n", msState);
                break;
        }
    }

    return EECode_OK;
}

TInt CRTPMpeg12AudioScheduledReceiver::IsPassiveMode() const
{
    return 0;
}

TSchedulingHandle CRTPMpeg12AudioScheduledReceiver::GetWaitHandle() const
{
    return (TSchedulingHandle)mRTPSocket;
}

EECode CRTPMpeg12AudioScheduledReceiver::EventHandling(EECode err)
{
    SMSG msg;
    DASSERT(mpMsgSink);

    if (mpMsgSink) {
        if (EECode_TimeOut == err) {
            memset(&msg, 0x0, sizeof(msg));
            msg.code = EMSGType_Timeout;
            msg.owner_id = DGENERIC_ID_FROM_COMPONENT_TYPE_INDEX(EGenericComponentType_Demuxer, mIndex);
            msg.owner_type = EGenericComponentType_Demuxer;
            msg.owner_index = mIndex;

            msg.pExtra = NULL;
            msg.needFreePExtra = 0;

            mpMsgSink->MsgProc(msg);
            return EECode_OK;
        }
    } else {
        return EECode_BadState;
    }

    return EECode_NoImplement;
}

TSchedulingUnit CRTPMpeg12AudioScheduledReceiver::HungryScore() const
{
    return 1;
}

TU8 CRTPMpeg12AudioScheduledReceiver::GetPriority() const
{
    return mPriority;
}

CObject *CRTPMpeg12AudioScheduledReceiver::GetObject0() const
{
    return (CObject *) this;
}

void CRTPMpeg12AudioScheduledReceiver::SetVideoDataPostProcessingCallback(void *callback_context, void *callback)
{
    LOGM_FATAL("not support!\n");
    return;
}

void CRTPMpeg12AudioScheduledReceiver::SetAudioDataPostProcessingCallback(void *callback_context, void *callback)
{
    LOGM_FATAL("not support!\n");
    return;
}

void CRTPMpeg12AudioScheduledReceiver::PrintStatus()
{
    TInt expected_count = mRTPStatistics.cycles + mRTPStatistics.max_seq - mRTPStatistics.base_seq + 1;
    TInt actual_received_count = mRTPStatistics.received;
    float lose_percentage = 0;

    //LOGM_PRINTF("mRTPStatistics.max_seq %d, mRTPStatistics.base_seq %d, mRTPStatistics.cycles %d\n", mRTPStatistics.max_seq, mRTPStatistics.base_seq, mRTPStatistics.cycles);

    if (expected_count) {
        lose_percentage = ((float)(expected_count - actual_received_count)) / (float)expected_count;
    }

    //LOGM_PRINTF("recent packet lose count %d\n", expected_count - actual_received_count - mLastPrintLostCount);
    //mLastPrintLostCount = expected_count - actual_received_count;

    //LOGM_PRINTF("msState %d, mpBufferPool->GetFreeBufferCnt() %d, mDebugWaitMempoolFlag %d, mDebugWaitBufferpoolFlag %d, mDebugWaitReadSocketFlag %d\n", msState, mpBufferPool->GetFreeBufferCnt(), mDebugWaitMempoolFlag, mDebugWaitBufferpoolFlag, mDebugWaitReadSocketFlag);
    LOGM_PRINTF("state %d, %s, expected receive packet count %d, actual received count %d, loss count %d, loss percentage %f, heart beat %d\n", msState, gfGetRTPRecieverStateString(msState), expected_count, actual_received_count, expected_count - actual_received_count, lose_percentage, mDebugHeartBeat);
    mDebugHeartBeat = 0;
}

void CRTPMpeg12AudioScheduledReceiver::writeRR()
{
    TU32 lost;
    TU32 extended_max;
    TU32 expected_interval;
    TU32 received_interval;
    TU32 lost_interval;
    TU32 expected;
    TU32 fraction;
    TU32 len = 0;
    TU32 tmp = 0;
    TU32 jitter = 0;
    TU32 delay_since_last = 0;

    DASSERT(mbGetSSRC);
    if (!mbGetSSRC) {
        return;
    }

    mRTCPDataLen = 0;

    mRTCPBuffer[0] = (RTP_VERSION << 6) + 1;
    mRTCPBuffer[1] = RTCP_RR;

    mRTCPBuffer[2] = 0;
    mRTCPBuffer[3] = 7;

    mRTCPBuffer[4] = (mSSRC >> 24) & 0xff;
    mRTCPBuffer[5] = (mSSRC >> 16) & 0xff;
    mRTCPBuffer[6] = (mSSRC >> 8) & 0xff;
    mRTCPBuffer[7] = (mSSRC) & 0xff;

    mRTCPBuffer[8] = (mServerSSRC >> 24) & 0xff;
    mRTCPBuffer[9] = (mServerSSRC >> 16) & 0xff;
    mRTCPBuffer[10] = (mServerSSRC >> 8) & 0xff;
    mRTCPBuffer[11] = (mServerSSRC) & 0xff;

    //mRTPStatistics.max_seq = mRTPCurrentSeqNumber;
    extended_max = mRTPStatistics.cycles + mRTPStatistics.max_seq;
    expected = extended_max - mRTPStatistics.base_seq + 1;
    lost = expected - mRTPStatistics.received;
    if (lost > 0xffffff) {
        lost = 0xffffff;
    }

    expected_interval = expected - mRTPStatistics.expected_prior;
    mRTPStatistics.expected_prior = expected;
    received_interval = mRTPStatistics.received - mRTPStatistics.received_prior;
    mRTPStatistics.received_prior = mRTPStatistics.received;
    lost_interval = expected_interval - received_interval;
    if (expected_interval == 0 || lost_interval <= 0) {
        fraction = 0;
    } else {
        DASSERT(expected_interval);
        fraction = (lost_interval << 8) / expected_interval;
    }

    mRTCPBuffer[12] = fraction & 0xff;
    mRTCPBuffer[13] = (lost >> 16) & 0xff;
    mRTCPBuffer[14] = (lost >> 8) & 0xff;
    mRTCPBuffer[15] = lost & 0xff;

    mRTCPBuffer[16] = (extended_max >> 24) & 0xff;
    mRTCPBuffer[17] = (extended_max >> 16) & 0xff;
    mRTCPBuffer[18] = (extended_max >> 8) & 0xff;
    mRTCPBuffer[19] = extended_max & 0xff;

    jitter = mRTPStatistics.jitter >> 4;
    mRTCPBuffer[20] = (jitter >> 24) & 0xff;
    mRTCPBuffer[21] = (jitter >> 16) & 0xff;
    mRTCPBuffer[22] = (jitter >> 8) & 0xff;
    mRTCPBuffer[23] = jitter & 0xff;

    tmp = mLastRtcpNtpTime >> 16;
    delay_since_last = 0; //ntp_time - last_rtcp_ntp_time;
    mRTCPBuffer[24] = (tmp >> 24) & 0xff;
    mRTCPBuffer[25] = (tmp >> 16) & 0xff;
    mRTCPBuffer[26] = (tmp >> 8) & 0xff;
    mRTCPBuffer[27] = tmp & 0xff;
    mRTCPBuffer[28] = (delay_since_last >> 24) & 0xff;
    mRTCPBuffer[29] = (delay_since_last >> 16) & 0xff;
    mRTCPBuffer[30] = (delay_since_last >> 8) & 0xff;
    mRTCPBuffer[31] = delay_since_last & 0xff;

    mRTCPBuffer[32] = (RTP_VERSION << 6) + 1;
    mRTCPBuffer[33] = RTCP_SDES;

    len = strlen(mCName);
    tmp = (6 + len + 3) / 4;
    mRTCPBuffer[34] = (tmp >> 8) & 0xff;
    mRTCPBuffer[35] = tmp & 0xff;

    mRTCPBuffer[36] = (mSSRC >> 24) & 0xff;
    mRTCPBuffer[37] = (mSSRC >> 16) & 0xff;
    mRTCPBuffer[38] = (mSSRC >> 8) & 0xff;
    mRTCPBuffer[39] = (mSSRC) & 0xff;

    mRTCPBuffer[40] = 0x01;
    mRTCPBuffer[41] = len & 0xff;
    DASSERT(len);
    DASSERT((len + 42) < (256 - 8));
    memcpy(mRTCPBuffer + 42, mCName, len);

    mRTCPDataLen = (len + 42 + 3) & (~0x3);
    DASSERT(mRTCPDataLen >= (len + 42));
}

void CRTPMpeg12AudioScheduledReceiver::updateRR()
{
    TU32 lost;
    TU32 extended_max;
    TU32 expected_interval;
    TU32 received_interval;
    TU32 lost_interval;
    TU32 expected;
    TU32 fraction;
    //TU32 len= 0;
    TU32 tmp = 0;
    TU32 jitter = 0;
    TU32 delay_since_last = 0;

    //DASSERT(mbGetSSRC);
    if (!mbGetSSRC) {
        return;
    }

    //DASSERT(mRTCPDataLen);
    if (!mRTCPDataLen) {
        writeRR();
    }

    //mRTPStatistics.max_seq = mRTPCurrentSeqNumber;
    extended_max = mRTPStatistics.cycles + mRTPStatistics.max_seq;
    expected = extended_max - mRTPStatistics.base_seq + 1;
    lost = expected - mRTPStatistics.received;
    if (lost > 0xffffff) {
        lost = 0xffffff;
    }

    expected_interval = expected - mRTPStatistics.expected_prior;
    mRTPStatistics.expected_prior = expected;
    received_interval = mRTPStatistics.received - mRTPStatistics.received_prior;
    mRTPStatistics.received_prior = mRTPStatistics.received;
    lost_interval = expected_interval - received_interval;
    if (expected_interval == 0 || lost_interval <= 0) {
        fraction = 0;
    } else {
        DASSERT(expected_interval);
        fraction = (lost_interval << 8) / expected_interval;
    }

    mRTCPBuffer[12] = fraction & 0xff;
    mRTCPBuffer[13] = (lost >> 16) & 0xff;
    mRTCPBuffer[14] = (lost >> 8) & 0xff;
    mRTCPBuffer[15] = lost & 0xff;

    mRTCPBuffer[16] = (extended_max >> 24) & 0xff;
    mRTCPBuffer[17] = (extended_max >> 16) & 0xff;
    mRTCPBuffer[18] = (extended_max >> 8) & 0xff;
    mRTCPBuffer[19] = extended_max & 0xff;

    jitter = mRTPStatistics.jitter >> 4;
    mRTCPBuffer[20] = (jitter >> 24) & 0xff;
    mRTCPBuffer[21] = (jitter >> 16) & 0xff;
    mRTCPBuffer[22] = (jitter >> 8) & 0xff;
    mRTCPBuffer[23] = jitter & 0xff;

    tmp = mLastRtcpNtpTime >> 16;
    delay_since_last = 0; //ntp_time - last_rtcp_ntp_time;
    mRTCPBuffer[24] = (tmp >> 24) & 0xff;
    mRTCPBuffer[25] = (tmp >> 16) & 0xff;
    mRTCPBuffer[26] = (tmp >> 8) & 0xff;
    mRTCPBuffer[27] = tmp & 0xff;
    mRTCPBuffer[28] = (delay_since_last >> 24) & 0xff;
    mRTCPBuffer[29] = (delay_since_last >> 16) & 0xff;
    mRTCPBuffer[30] = (delay_since_last >> 8) & 0xff;
    mRTCPBuffer[31] = delay_since_last & 0xff;
}

void CRTPMpeg12AudioScheduledReceiver::receiveSR()
{
    DASSERT(mRTCPSocket);
    if (mRTCPSocket > 0) {
        mRTCPReadDataLen = gfNet_RecvFrom(mRTCPSocket, mRTCPReadBuffer, sizeof(mRTCPReadBuffer), 0, (void *)&mSrcRTCPAddr, (TSocketSize *)&mFromLen);
        //LOGM_NOTICE("[debug]: parse SR, to do, mRTCPReadDataLen %d\n", mRTCPReadDataLen);
    }
}

void CRTPMpeg12AudioScheduledReceiver::sendRR()
{
    DASSERT(mRTCPSocket);
    if (mRTCPSocket > 0) {
        gfNet_SendTo(mRTCPSocket, mRTCPBuffer, mRTCPDataLen, 0, (const struct sockaddr *)&mSrcRTCPAddr, mFromLen);
        //LOGM_NOTICE("[debug]: send RR, mRTCPDataLen %d\n", mRTCPDataLen);
    }
}

void CRTPMpeg12AudioScheduledReceiver::checkSRsendRR()
{
#if 0
    struct pollfd fd;
    fd.fd = mRTCPSocket;
    fd.events = POLLIN | POLLOUT;

    poll(&fd, 1, 5);

    if (fd.revents & POLLIN) {
        receiveSR();
        if (fd.revents & POLLOUT) {
            updateRR();
            sendRR();
        }
    }
#endif
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

