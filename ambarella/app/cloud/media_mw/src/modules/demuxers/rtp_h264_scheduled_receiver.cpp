/*******************************************************************************
 * rtp_h264_scheduled_receiver.cpp
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
#include "codec_misc.h"
#include "dsp_platform_interface.h"
#include "streaming_if.h"
#include "modules_interface.h"

#include "rtp_h264_scheduled_receiver.h"
#include "rtsp_demuxer.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

static void __appendVideoExtradata(TU8 *p_data, TInt len, TUint data_type, TU8 *&mpVideoExtraData, TInt &mVideoExtraDataLen, TInt &mVideoExtraDataBufLen)
{
    TInt mlen = len + 8;
    TU8 *pstart;

    if (mlen < 320) {
        mlen = 320;
    }
    if (!mpVideoExtraData) {
        mpVideoExtraData = (TU8 *)DDBG_MALLOC(mlen, "DMVE");
        mVideoExtraDataBufLen = mlen;
        DASSERT(!mVideoExtraDataLen);
        mVideoExtraDataLen = 0;
    }

    if ((mVideoExtraDataLen + 8 + len) > mVideoExtraDataBufLen) {
        LOG_WARN("!!extra data buffer not enough, is it correct here, mVideoExtraDataLen %d, len %d, mVideoExtraDataBufLen %d?\n", mVideoExtraDataLen, len, mVideoExtraDataBufLen);
        mVideoExtraDataBufLen = mVideoExtraDataLen + 8 + len;
        pstart = (TU8 *)DDBG_MALLOC(mVideoExtraDataBufLen, "DMVE");
        DASSERT(pstart);
        memcpy(pstart, mpVideoExtraData, mVideoExtraDataLen);
        DDBG_FREE(mpVideoExtraData, "DMVE");
        mpVideoExtraData = pstart;
    }

    pstart = mpVideoExtraData + mVideoExtraDataLen;

    memcpy(pstart, p_data, len);
    mVideoExtraDataLen += len;
    DASSERT(mVideoExtraDataLen < mVideoExtraDataBufLen);

}

static void __parseRTPPacketHeader(TU8 *p_packet, TU32 &time_stamp, TU16 &seq_number, TU32 &ssrc)
{
    DASSERT(p_packet);
    if (p_packet) {
        time_stamp = (p_packet[7]) | (p_packet[6] << 8) | (p_packet[5] << 16) | (p_packet[4] << 24);
        ssrc = (p_packet[11]) | (p_packet[10] << 8) | (p_packet[9] << 16) | (p_packet[8] << 24);
        seq_number = (p_packet[2] << 8) | (p_packet[3]);
    }
}

CRTPH264ScheduledReceiver::CRTPH264ScheduledReceiver(TUint index, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink)
    : inherited("CRTPH264ScheduledReceiver", index)
    , mTrackID(0)
    , mbRun(1)
    , mbInitialized(0)
    , mType(StreamType_Video)
    , mFormat(StreamFormat_H264)
    , mVideoWidth(720)
    , mVideoHeight(480)
    , mVideoFramerateNum(DDefaultVideoFramerateNum)
    , mVideoFramerateDen(DDefaultVideoFramerateDen)
    , mVideoFramerate(((float)DDefaultVideoFramerateNum) / ((float)DDefaultVideoFramerateDen))
    , mReconnectTag(0)
    , mpMsgSink(pMsgSink)
    , mpPersistMediaConfig(pPersistMediaConfig)
    , mpSystemClockReference(NULL)
    , mpWatchDog(NULL)
    , mpOutputPin(NULL)
    , mpBufferPool(NULL)
    , mpMemPool(NULL)
    , mRTSPSocket(-1)
    , mRTPSocket(-1)
    , mRTCPSocket(-1)
    , msState(DATA_THREAD_STATE_READ_FIRST_RTP_PACKET)
    , mReservedDataLength(0)
    , mStartCodeLength(4 + 1)
    , mRTPHeaderLength(14)
    , mpMemoryStart(NULL)
    , mpCurrentPointer(NULL)
    , mTotalWritenLength(0)
    , mReadLen(0)
    , mpVideoExtraData(NULL)
    , mVideoExtraDataLen(0)
    , mVideoExtraDataBufLen(0)
    , mH264DataFmt(H264_FMT_INVALID)
    , mH264AVCCNaluLen(0)
    , mpVideoExtraDataFromSDP(NULL)
    , mVideoExtraDataSize(0)
    , mpH264spspps(NULL)
    , mH264spsppsSize(0)
    , mRTPTimeStamp(0)
    , mRTPCurrentSeqNumber(0)
    , mRTPLastSeqNumber(0)
    , mpBuffer(NULL)
    , mNalType(0)
    , mNri(0)
    , mbWaitFirstSpsPps(1)
    , mbWaitRTPDelimiter(1)
    , mRequestMemorySize(512 * 1024)
    , mCurrentMemorySize(0)
    , mCmd(0)
    , mDebugWaitMempoolFlag(0)
    , mDebugWaitBufferpoolFlag(0)
    , mDebugWaitReadSocketFlag(0)
    , mbGetSSRC(0)
    , mbSendSyncPointBuffer(0)
    , mPriority(1)
    , mBufferSessionNumber(0)
    , mLastAliveTimeStamp(0)
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
    , mbSetTimeStamp(0)
    , mbBrokenFrame(0)
    , mbSendSequenceData(0)
    , mFakePts(0)
{
    //memset(&mRTPStatistics, 0x0, sizeof(mRTPStatistics));
    //mRTPStatistics.probation = 1;

    mpMutex = gfCreateMutex();
    DASSERT(mpMutex);

    DASSERT(mpPersistMediaConfig);
    if (mpPersistMediaConfig) {
        mpSystemClockReference = mpPersistMediaConfig->p_system_clock_reference;
    }
    DASSERT(mpSystemClockReference);
}

CRTPH264ScheduledReceiver::~CRTPH264ScheduledReceiver()
{
    if (DLikely(mpSystemClockReference && mpWatchDog)) {
        mpSystemClockReference->RemoveTimer(mpWatchDog);
    }

    if (mpVideoExtraData) {
        free(mpVideoExtraData);
        mpVideoExtraData = NULL;
    }

    if (mpVideoExtraDataFromSDP) {
        free(mpVideoExtraDataFromSDP);
        mpVideoExtraDataFromSDP = NULL;
    }

    if (mpH264spspps) {
        free(mpH264spspps);
        mpH264spspps = NULL;
    }
    if (mpMutex) {
        mpMutex->Delete();
        mpMutex = NULL;
    }
}

EECode CRTPH264ScheduledReceiver::Initialize(SRTPContext *context, SStreamCodecInfo *p_stream_info, TUint number_of_content)
{
    DASSERT(context);
    DASSERT(p_stream_info);

    AUTO_LOCK(mpMutex);
    if (mbInitialized) {
        LOGM_FATAL("already initialized?\n");
        return EECode_BadState;
    }

    //TODO
    msState = DATA_THREAD_STATE_READ_FIRST_RTP_PACKET;
    mbWaitFirstSpsPps = 1;
    mbWaitRTPDelimiter = 1;
    mTotalWritenLength = 0;
    mbGetSSRC = 0;
    mbSendSyncPointBuffer = 0;
    mPacketCount = 0;
    mOctetCount = 0;
    mLastOctetCount = 0;
    mRTCPCurrentTick = 0;
    mbSetTimeStamp = 0;
    mbBrokenFrame = 0;
    mbSendSequenceData = 0;
    mLastRtcpNtpTime = 0;
    mFirstRtcpNtpTime = 0;
    mLastRtcpTimeStamp = 0;
    mRtcpTimeStampOffset = 0;

    RtpDecUtils::rtp_init_statistics(&mRTPStatistics, 0); // do we know the initial sequence from sdp?
    //init_rtp_demuxer
    last_rtcp_ntp_time  = DInvalidTimeStamp;
    first_rtcp_ntp_time = DInvalidTimeStamp;
    base_timestamp      = 0;
    timestamp           = 0;
    unwrapped_timestamp = 0;
    rtcp_ts_offset      = 0;
    range_start_offset = 0;//TODO
    octet_count = 0;
    last_octet_count = 0;

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
        LOGM_INFO("video rtp port %hu, addr %s, mServerAddrIn.sin_addr.s_addr 0x%x.\n", context->server_rtp_port, context->server_addr, pAddr->sin_addr.s_addr);

        memset(&mSrcRTCPAddr, 0x0, sizeof(mSrcRTCPAddr));
        pAddr = (struct sockaddr_in *)&mSrcRTCPAddr;
        pAddr->sin_family = AF_INET;
        pAddr->sin_port = htons(context->server_rtcp_port);
        pAddr->sin_addr.s_addr = inet_addr(context->server_addr);
        LOGM_INFO("video rtcp port %hu, addr %s, mServerAddrIn.sin_addr.s_addr 0x%x.\n", context->server_rtcp_port, context->server_addr, pAddr->sin_addr.s_addr);

        //debug assert
        DASSERT(StreamType_Video == mType);
        DASSERT(StreamFormat_H264 == mFormat);
        DASSERT(mpOutputPin);
        DASSERT(mpBufferPool);
        DASSERT(mpMemPool);
        DASSERT(mRTPSocket > 0);
        DASSERT(mRTCPSocket > 0);

        memset(mCName, 0x0, sizeof(mCName));
        gfOSGetHostName(mCName, sizeof(mCName));
        DASSERT(strlen(mCName) < (sizeof(mCName) + 16));
        sprintf(mCName, "%s_%d_%d", mCName, mReconnectTag++, context->index);
        //LOGM_DEBUG("[debug]: mCName %s\n", mCName);

        mbInitialized = 1;

        mReservedDataLength = mRTPHeaderLength;

        if (mpBufferPool->GetFreeBufferCnt()) {
            mDebugWaitBufferpoolFlag = 1;
            if (!mpOutputPin->AllocBuffer(mpBuffer)) {
                LOG_ERROR("demuxer AllocBuffer Fail, must not comes here!\n");
                mbRun = 0;
            } else {
                mpBuffer->SetBufferFlags(0);
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
        DASSERT(StreamType_Video == p_stream_info->stream_type);
        DASSERT(p_stream_info->spec.video.pic_width);
        DASSERT(p_stream_info->spec.video.pic_height);
        if (DLikely(p_stream_info->spec.video.pic_width && p_stream_info->spec.video.pic_height)) {
            mVideoWidth = p_stream_info->spec.video.pic_width;
            mVideoHeight = p_stream_info->spec.video.pic_height;
        }
        if (DLikely(p_stream_info->spec.video.framerate_num && p_stream_info->spec.video.framerate_den)) {
            mVideoFramerateNum = p_stream_info->spec.video.framerate_num;
            mVideoFramerateDen = p_stream_info->spec.video.framerate_den;
        }
        if (DLikely(p_stream_info->spec.video.framerate)) {
            mVideoFramerate = p_stream_info->spec.video.framerate;
        }
        mPayloadType = p_stream_info->payload_type;
    }

    LOGM_INFO("mVideoWidth %d, mVideoHeight %d\n", mVideoWidth, mVideoHeight);

    if (DLikely(mpSystemClockReference)) {
        DASSERT(!mpWatchDog);
        mpWatchDog = mpSystemClockReference->AddTimer(this, EClockTimerType_repeat_infinite, mpSystemClockReference->GetCurrentTime() + 20000000, 10000000, 0);
        DASSERT(mpWatchDog);
        mLastAliveTimeStamp = mpSystemClockReference->GetCurrentTime();
    }

    return EECode_OK;
}

EECode CRTPH264ScheduledReceiver::ReInitialize(SRTPContext *context, SStreamCodecInfo *p_stream_info, TUint number_of_content)
{
    DASSERT(context);
    DASSERT(p_stream_info);
    AUTO_LOCK(mpMutex);
    DASSERT(mbInitialized);

    if (context) {
        msState = DATA_THREAD_STATE_READ_FIRST_RTP_PACKET;
        mbWaitFirstSpsPps = 1;
        mbWaitRTPDelimiter = 1;
        mTotalWritenLength = 0;
        mbGetSSRC = 0;
        mbSendSyncPointBuffer = 0;
        mPacketCount = 0;
        mOctetCount = 0;
        mLastOctetCount = 0;
        mRTCPCurrentTick = 0;
        mbSetTimeStamp = 0;
        mbBrokenFrame = 0;
        mbSendSequenceData = 0;

        mLastRtcpNtpTime = 0;
        mFirstRtcpNtpTime = 0;
        mLastRtcpTimeStamp = 0;
        mRtcpTimeStampOffset = 0;

        //memset(&mRTPStatistics, 0x0, sizeof(mRTPStatistics));
        //mRTPStatistics.probation = 1;
        RtpDecUtils::rtp_init_statistics(&mRTPStatistics, 0); // do we know the initial sequence from sdp?
        //init_rtp_demuxer
        last_rtcp_ntp_time  = DInvalidTimeStamp;
        first_rtcp_ntp_time = DInvalidTimeStamp;
        base_timestamp      = 0;
        timestamp           = 0;
        unwrapped_timestamp = 0;
        rtcp_ts_offset      = 0;
        range_start_offset = 0;//TODO
        octet_count = 0;
        last_octet_count = 0;
        mFakePts = 0;

        struct sockaddr_in *pAddr = NULL;

        DASSERT(mType == context->type);
        DASSERT(mFormat == context->format);
        DASSERT(mpOutputPin == context->outpin);
        DASSERT(mpBufferPool == context->bufferpool);
        DASSERT(mpMemPool == context->mempool);

        mRTPSocket = context->rtp_socket;
        mRTCPSocket = context->rtcp_socket;
        DASSERT(mPriority == context->priority);

        memset(&mSrcAddr, 0x0, sizeof(mSrcAddr));
        pAddr = (struct sockaddr_in *)&mSrcAddr;
        pAddr->sin_family = AF_INET;
        pAddr->sin_port = htons(context->server_rtp_port);
        pAddr->sin_addr.s_addr = inet_addr(context->server_addr);
        LOGM_INFO("video rtp port %hu, addr %s, mServerAddrIn.sin_addr.s_addr 0x%x.\n", context->server_rtp_port, context->server_addr, pAddr->sin_addr.s_addr);

        memset(&mSrcRTCPAddr, 0x0, sizeof(mSrcRTCPAddr));
        pAddr = (struct sockaddr_in *)&mSrcRTCPAddr;
        pAddr->sin_family = AF_INET;
        pAddr->sin_port = htons(context->server_rtcp_port);
        pAddr->sin_addr.s_addr = inet_addr(context->server_addr);
        LOGM_INFO("video rtcp port %hu, addr %s, mServerAddrIn.sin_addr.s_addr 0x%x.\n", context->server_rtcp_port, context->server_addr, pAddr->sin_addr.s_addr);

        //debug assert
        DASSERT(StreamType_Video == mType);
        DASSERT(StreamFormat_H264 == mFormat);
        DASSERT(mpOutputPin);
        DASSERT(mpBufferPool);
        DASSERT(mpMemPool);
        DASSERT(mRTPSocket > 0);
        DASSERT(mRTCPSocket > 0);

        memset(mCName, 0x0, sizeof(mCName));
        gfOSGetHostName(mCName, sizeof(mCName));
        DASSERT(strlen(mCName) < (sizeof(mCName) + 16));
        sprintf(mCName, "%s_%d_%d", mCName, mReconnectTag++, context->index);
        //LOGM_INFO("mCName %s\n", mCName);

        if (!mpBuffer) {
            if (mpBufferPool->GetFreeBufferCnt()) {
                mDebugWaitBufferpoolFlag = 1;
                if (!mpOutputPin->AllocBuffer(mpBuffer)) {
                    LOG_ERROR("demuxer AllocBuffer Fail, must not comes here!\n");
                    mbRun = 0;
                } else {
                    mpBuffer->SetBufferFlags(0);
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
                LOGM_WARN("no free buffer here\n");
                msState = DATA_THREAD_STATE_WAIT_OUTBUFFER;
            }
        }

    } else {
        LOGM_FATAL("NULL params\n");
        return EECode_BadParam;
    }

    if (p_stream_info) {
        DASSERT(p_stream_info->stream_format == mFormat);
        DASSERT(StreamType_Video == p_stream_info->stream_type);
        DASSERT(p_stream_info->spec.video.pic_width);
        DASSERT(p_stream_info->spec.video.pic_height);
        if (DLikely(p_stream_info->spec.video.pic_width && p_stream_info->spec.video.pic_height)) {
            mVideoWidth = p_stream_info->spec.video.pic_width;
            mVideoHeight = p_stream_info->spec.video.pic_height;
        }
        if (DLikely(p_stream_info->spec.video.framerate_num && p_stream_info->spec.video.framerate_den)) {
            mVideoFramerateNum = p_stream_info->spec.video.framerate_num;
            mVideoFramerateDen = p_stream_info->spec.video.framerate_den;
        }
        if (DLikely(p_stream_info->spec.video.framerate)) {
            mVideoFramerate = p_stream_info->spec.video.framerate;
        }
        mPayloadType = p_stream_info->payload_type;
    }

    LOGM_INFO("mVideoWidth %d, mVideoHeight %d\n", mVideoWidth, mVideoHeight);

    if (DLikely(mpSystemClockReference)) {
        if (DUnlikely(mpWatchDog)) {
            LOGM_ERROR("mpWatchDog not NULL\n");
            mpSystemClockReference->RemoveTimer(mpWatchDog);
            mpWatchDog = NULL;
        }
        mpWatchDog = mpSystemClockReference->AddTimer(this, EClockTimerType_repeat_infinite, mpSystemClockReference->GetCurrentTime() + 20000000, 10000000, 0);
        DASSERT(mpWatchDog);

        mLastAliveTimeStamp = mpSystemClockReference->GetCurrentTime();
    }

    return EECode_OK;
}

EECode CRTPH264ScheduledReceiver::Flush()
{
    LOGM_FATAL("need implement\n");
    return EECode_OK;
}

EECode CRTPH264ScheduledReceiver::Purge()
{
    LOGM_FATAL("need implement\n");
    return EECode_OK;
}

EECode CRTPH264ScheduledReceiver::SetExtraData(TU8 *p_extradata, TU32 extradata_size, TU32 index)
{
    AUTO_LOCK(mpMutex);
    return do_SetExtraData(p_extradata, extradata_size);
}
EECode CRTPH264ScheduledReceiver::do_SetExtraData(TU8 *p_extradata, TMemSize extradata_size)
{
    static const TU8 startcode[4] = {0, 0, 0, 0x01};
    TUint sps, pps;
    TU8 *pH264spspps_start = NULL;

    //DASSERT(!mpVideoExtraDataFromSDP);

    //LOGM_NOTICE("SetExtraData\n");

    if (mpVideoExtraDataFromSDP && (mVideoExtraDataSize == extradata_size)) {
        if (!memcmp(mpVideoExtraDataFromSDP, p_extradata, extradata_size)) {
            //LOGM_WARN("same\n");
            return EECode_OK;
        }
    }

    if (mpVideoExtraDataFromSDP) {
        free(mpVideoExtraDataFromSDP);
        mpVideoExtraDataFromSDP = NULL;
        mVideoExtraDataSize = 0;
    }

    if (mpH264spspps) {
        free(mpH264spspps);
        mpH264spspps = NULL;
    }

    DASSERT(!mpVideoExtraDataFromSDP);
    DASSERT(!mVideoExtraDataSize);

    mVideoExtraDataSize = extradata_size;
    LOGM_INFO("video extra data size %d, first 8 bytes: 0x%02x 0x%02x 0x%02x 0x%02x, 0x%02x 0x%02x 0x%02x 0x%02x\n", mVideoExtraDataSize, \
              p_extradata[0], p_extradata[1], p_extradata[2], p_extradata[3], \
              p_extradata[4], p_extradata[5], p_extradata[6], p_extradata[7]);
    mpVideoExtraDataFromSDP = (TU8 *)DDBG_MALLOC(mVideoExtraDataSize, "DMVe");
    if (mpVideoExtraDataFromSDP) {
        memcpy(mpVideoExtraDataFromSDP, p_extradata, extradata_size);
    } else {
        LOGM_FATAL("NO memory\n");
        return EECode_NoMemory;
    }

    if (p_extradata[0] != 0x01) {
        LOGM_INFO("extradata is annex-b format.\n");
        mH264DataFmt = H264_FMT_ANNEXB;
    } else {

        mH264spsppsSize = 0;
        DASSERT(!mpH264spspps);

        mH264DataFmt = H264_FMT_AVCC;
        mH264AVCCNaluLen = 1 + (mpVideoExtraDataFromSDP[4] & 3);
        LOGM_INFO("extradata is AVCC format, mH264AVCCNaluLen %d.\n", mH264AVCCNaluLen);

        mpH264spspps = (TU8 *)DDBG_MALLOC(mVideoExtraDataSize + 16, "DMvE");
        if (mpH264spspps) {
            pH264spspps_start = mpH264spspps;
            sps = BE_16(mpVideoExtraDataFromSDP + 6);
            memcpy(mpH264spspps, startcode, sizeof(startcode));
            mpH264spspps += sizeof(startcode);
            mH264spsppsSize += sizeof(startcode);
            memcpy(mpH264spspps, mpVideoExtraDataFromSDP + 8, sps);
            mpH264spspps += sps;
            mH264spsppsSize += sps;

            pps = BE_16(mpVideoExtraDataFromSDP + 6 + 2 + sps + 1);
            memcpy(mpH264spspps, startcode, sizeof(startcode));
            mpH264spspps += sizeof(startcode);
            mH264spsppsSize += sizeof(startcode);
            memcpy(mpH264spspps, mpVideoExtraDataFromSDP + 6 + 2 + sps + 1 + 2, pps);
            mH264spsppsSize += pps;
            mpH264spspps = pH264spspps_start;
        } else {
            LOGM_FATAL("NO memory\n");
            return EECode_NoMemory;
        }

        LOGM_INFO("mH264spsppsSize %d\n", mH264spsppsSize);
    }

    return EECode_OK;
}

int CRTPH264ScheduledReceiver::checkRtpPacket(unsigned char *packet, int len)
{
#define D_RB16(x)                           \
    ((((const unsigned char*)(x))[0] << 8) |          \
     ((const unsigned char*)(x))[1])

    int recv_len = len;
    unsigned char *buf = packet;
    int payload_type, seq;
    int ext;
    int rv = 0;

    if (len < 12) {
        return -1;
    }
    if ((buf[0] & 0xc0) != (2/*RTP_VERSION*/ << 6)) {
        return -1;
    }

    ext = buf[0] & 0x10;
    payload_type = buf[1] & 0x7f;
    seq  = D_RB16(buf + 2);

    /* NOTE: we can handle only one payload type */
    if (mPayloadType != payload_type)
    { return -1; }

    // only do something with this if all the rtp checks pass...
    if (!RtpDecUtils::rtp_valid_packet_in_sequence(&mRTPStatistics, seq)) {
        return -1;
    }

    if (buf[0] & 0x20) {
        int padding = buf[len - 1];
        if (len >= 12 + padding)
        { len -= padding; }
    }

    len -= 12;
    buf += 12;

    /* RFC 3550 Section 5.3.1 RTP Header Extension handling */
    if (ext) {
        if (len < 4)
        { return -1; }
        /* calculate the header extension length (stored as number
         * of 32-bit words) */
        ext = (D_RB16(buf + 2) + 1) << 2;

        if (len < ext)
        { return -1; }
        // skip past RTP header extension
        len -= ext;
        buf += ext;
    }

    check_send_rr(recv_len);
    return rv;
}

//#define  GET_FRAME_BY_TIMESTAMP

EECode CRTPH264ScheduledReceiver::Scheduling(TUint times, TU32 inout_mask)
{
    mRTCPCurrentTick ++;
    if (mRTCPCurrentTick > mRTCPCoolDown) {
        //LOGM_NOTICE("before checkSRsendRR()\n");
        //checkSRsendRR();
        mRTCPCurrentTick = 0;
    }

    if (DLikely(mpSystemClockReference)) {
        mLastAliveTimeStamp = mpSystemClockReference->GetCurrentTime();
    }

    while (times-- > 0) {
        AUTO_LOCK(mpMutex);

        if (DUnlikely(DATA_THREAD_STATE_ERROR == msState)) {
            SMSG msg;
            DASSERT(mpMsgSink);

            if (mpMsgSink) {
                memset(&msg, 0x0, sizeof(msg));
                msg.code = EMSGType_Timeout;
                msg.owner_id = DGENERIC_ID_FROM_COMPONENT_TYPE_INDEX(EGenericComponentType_Demuxer, mIndex);
                msg.owner_type = EGenericComponentType_Demuxer;
                msg.owner_index = mIndex;

                msg.pExtra = NULL;
                msg.needFreePExtra = 0;

                mpMsgSink->MsgProc(msg);
            }
            return EECode_NotRunning;
        }

        if (EECode_OK == gfOSPollFdReadable(mRTCPSocket)) {
            receiveSR();
        }

#if 0
        if (!(fds[1].revents & POLLIN)) {
            if (msState == DATA_THREAD_STATE_READ_FIRST_RTP_PACKET || msState == DATA_THREAD_STATE_READ_REMANING_RTP_PACKET) {
                return EECode_OK;
            }
        }
#endif

        //LOGM_STATE("Scheduling(%d) msState %d\n", mIndex, msState);
        switch (msState) {

            case DATA_THREAD_STATE_READ_FIRST_RTP_PACKET:
                //DASSERT(mpBuffer);

                if (DLikely(!mTotalWritenLength)) {
                    //write from start
                    mpCurrentPointer = mpMemoryStart;
                    //DASSERT(mpCurrentPointer);
                } else {
                    LOGM_ERROR("mTotalWritenLength %d, mpCurrentPointer %p should be zero, at the beginning.\n", mTotalWritenLength, mpCurrentPointer);
                    mTotalWritenLength = 0;
                    //mpCurrentPointer = mpBuffer->GetDataPtrBase();
                    mpCurrentPointer = mpMemoryStart;
                }

                mFromLen = sizeof(struct sockaddr_in);
                mDebugWaitReadSocketFlag = 1;
                mbBrokenFrame = 0;
                mReadLen = gfNet_RecvFrom(mRTPSocket, mpCurrentPointer, mRequestMemorySize - mTotalWritenLength, 0, (void *)&mSrcAddr, (TSocketSize *)&mFromLen);
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
                if (checkRtpPacket(mpCurrentPointer, mReadLen) < 0) {
                    break;//TODO
                }

                //DASSERT(mReadLen <= ((TInt)(mRequestMemorySize) - mTotalWritenLength));

                if (DUnlikely(mbWaitRTPDelimiter)) {
                    if (DUnlikely(mpCurrentPointer[1] & 0x80)) {
                        mbWaitRTPDelimiter = 0;

                        //mRTPStatistics.probation = 0;
                        //parse rtp header, get some information
                        __parseRTPPacketHeader(mpCurrentPointer, mRTPTimeStamp, mRTPCurrentSeqNumber, mPacketSSRC);
                        //mRTPStatistics.base_seq = mRTPCurrentSeqNumber;
                        //mRTPStatistics.max_seq = mRTPCurrentSeqNumber;
                        LOGM_INFO("wait mbWaitRTPDelimiter done\n");
                    } else {
                        mRTPLastSeqNumber = mRTPCurrentSeqNumber;
                        __parseRTPPacketHeader(mpCurrentPointer, mRTPTimeStamp, mRTPCurrentSeqNumber, mPacketSSRC);
                        if (DLikely(mbGetSSRC)) {
                            DASSERT(mPacketSSRC == mServerSSRC);
                            if (DUnlikely(mPacketSSRC != mServerSSRC)) {
                                LOGM_NOTICE("mPacketSSRC 0x%08x, mServerSSRC 0x%08x\n", mPacketSSRC, mServerSSRC);
                            }
                        } else {
                            mServerSSRC = mPacketSSRC;
                            mSSRC = mServerSSRC + 16;
                            mbGetSSRC = 1;
                            LOGM_INFO("get SSRC 0x%08x\n", mPacketSSRC);
                        }
                        LOGM_INFO("wait mbWaitRTPDelimiter...\n");
                        continue;
                    }
                } else {
                    mRTPLastSeqNumber = mRTPCurrentSeqNumber;
                    __parseRTPPacketHeader(mpCurrentPointer, mRTPTimeStamp, mRTPCurrentSeqNumber, mPacketSSRC);
                    //to do, do check on seq number
#if 1
                    if (DLikely(mRTPLastSeqNumber < mRTPCurrentSeqNumber)) {
                        //DASSERT((mRTPLastSeqNumber + 1) == mRTPCurrentSeqNumber);
                        if (DUnlikely((mRTPLastSeqNumber + 1) != mRTPCurrentSeqNumber)) {
                            LOGM_ERROR("last seq %d, cur seq %d, data packet lost!\n", mRTPLastSeqNumber, mRTPCurrentSeqNumber);
                            mbBrokenFrame = 1;
                        }
                    }
#endif
                }
                /*
                                if (DUnlikely(mRTPLastSeqNumber > mRTPCurrentSeqNumber)) {
                                    mRTPStatistics.cycles += (1 << 16);
                                }
                                mRTPStatistics.received ++;
                                mRTPStatistics.max_seq = mRTPCurrentSeqNumber;
                */
                if (DLikely(mbGetSSRC)) {
                    DASSERT(mPacketSSRC == mServerSSRC);
                    if (DUnlikely(mPacketSSRC != mServerSSRC)) {
                        LOGM_NOTICE("mPacketSSRC 0x%08x, mServerSSRC 0x%08x\n", mPacketSSRC, mServerSSRC);
                    }
                } else {
                    mServerSSRC = mPacketSSRC;
                    mSSRC = mServerSSRC + 16;
                    mbGetSSRC = 1;
                }

                if (DLikely(!mbSetTimeStamp)) {
                    mpBuffer->SetBufferNativePTS((TTime)mRTPTimeStamp);
                    mpBuffer->SetBufferLinearPTS(mFakePts);

                    /*calc pts according to timestamp&timebase, RTCP SR info*/
                    //TS64 pts = -1;
                    //do_calc_pts(pts,mRTPTimeStamp);
                    mpBuffer->SetBufferPTS(mFakePts);
                    //printf("PTS ---- ts [%u], fake_pts [%lld], pts [%lld]\n",mRTPTimeStamp,(TS64)mFakePts,pts);
                    mbSetTimeStamp = 1;
                }

                if (DUnlikely(mpCurrentPointer[1] & 0x80)) {
                    if (DUnlikely(mbWaitFirstSpsPps)) {
                        mNalType = mpCurrentPointer[4 + mReservedDataLength - 2 - (mStartCodeLength - 1)] & 0x1f;

                        if (DLikely((ENalType_SPS != mNalType) && (ENalType_PPS != mNalType))) {
                            LOGM_DEBUG("wait sps pps..., type %d\n", mNalType);
                            if (ENalType_IDR == mNalType) {
                                LOGM_INFO("wait first sps pps 1... IDR comes mNalType %d, mpH264spspps %p, mpVideoExtraDataFromSDP %p\n", mNalType, mpH264spspps, mpVideoExtraDataFromSDP);
                                if (DLikely(mpH264spspps)) {
                                    mPersistExtradataBuffer.SetDataPtr(mpH264spspps);
                                    mPersistExtradataBuffer.SetDataSize(mH264spsppsSize);
                                    mPersistExtradataBuffer.SetDataPtrBase(NULL);
                                    mPersistExtradataBuffer.SetDataMemorySize(0);
                                    mPersistExtradataBuffer.SetBufferType(EBufferType_VideoExtraData);
                                    mPersistExtradataBuffer.mContentFormat = StreamFormat_H264;
                                    //mPersistExtradataBuffer.SetBufferFlagBits(CIBuffer::SYNC_POINT);
                                    mpOutputPin->SendBuffer(&mPersistExtradataBuffer);
                                    mDebugHeartBeat ++;
                                    mbWaitFirstSpsPps = 0;
                                } else if (DLikely(mpVideoExtraDataFromSDP)) {
                                    mPersistExtradataBuffer.SetDataPtr(mpVideoExtraDataFromSDP);
                                    mPersistExtradataBuffer.SetDataSize(mVideoExtraDataSize);
                                    mPersistExtradataBuffer.SetDataPtrBase(NULL);
                                    mPersistExtradataBuffer.SetDataMemorySize(0);
                                    mPersistExtradataBuffer.SetBufferType(EBufferType_VideoExtraData);
                                    //mPersistExtradataBuffer.SetBufferFlagBits(CIBuffer::SYNC_POINT);
                                    mPersistExtradataBuffer.mContentFormat = StreamFormat_H264;
                                    mpOutputPin->SendBuffer(&mPersistExtradataBuffer);
                                    mDebugHeartBeat ++;
                                    mbWaitFirstSpsPps = 0;
                                } else {
                                    LOGM_ERROR("no extra data!\n");
                                }
                            } else {
                                continue;
                            }
                        }
                        mbWaitFirstSpsPps = 0;
                        LOGM_NOTICE("[flow]: wait sps pps done, mNalType %d\n", mNalType);
                    }

                    //fill start code prefix
                    mpCurrentPointer += mReservedDataLength - 2 - (mStartCodeLength - 1);//no nal type and mNri byte
                    mpCurrentPointer[0] = 0;
                    mpCurrentPointer[1] = 0;
                    mpCurrentPointer[2] = 0;
                    mpCurrentPointer[3] = 0x1; //start code prefix

                    mNalType = mpCurrentPointer[4] & 0x1f;//start code not changed
                    if (DUnlikely(ENalType_IDR == mNalType)) {
                        mpBuffer->SetBufferFlags(CIBuffer::KEY_FRAME);
                        mpBuffer->mVideoFrameType = EPredefinedPictureType_IDR;
                    } else if ((ENalType_SPS == mNalType) || (ENalType_PPS == mNalType)) {
                        if (mbBrokenFrame) {
                            LOGM_ERROR("lose pcaket in sps/pps!\n");
                            break;
                        }

                        if (ENalType_SPS == mNalType) {
                            mVideoExtraDataLen = 0;
                        }
                        /*
                        if (mbSendSequenceData) {
                            break;
                        }
                        */
                        __appendVideoExtradata(mpCurrentPointer, mReadLen - mReservedDataLength + 2 + mStartCodeLength - 1, mNalType, mpVideoExtraData, mVideoExtraDataLen, mVideoExtraDataBufLen);

                        if (ENalType_PPS == mNalType) {
                            //send extra data
                            //DASSERT(mpBuffer->GetDataPtrBase());
                            DASSERT(mRequestMemorySize > mVideoExtraDataLen);
                            DASSERT(mpVideoExtraData && mVideoExtraDataLen);
                            memcpy(mpMemoryStart, mpVideoExtraData, mVideoExtraDataLen);
                            mpBuffer->SetDataPtr(mpMemoryStart);
                            mpBuffer->SetDataSize(mVideoExtraDataLen);
                            mpBuffer->SetDataPtrBase(mpMemoryStart);
                            mpBuffer->SetDataMemorySize(mVideoExtraDataLen);
                            mpBuffer->SetBufferType(EBufferType_VideoExtraData);
                            mpMemPool->ReturnBackMemBlock(mRequestMemorySize - mVideoExtraDataLen, mpMemoryStart + mVideoExtraDataLen);

                            if (!mbSendSyncPointBuffer) {
                                mpBuffer->SetBufferFlagBits(CIBuffer::SYNC_POINT);
                                mpBuffer->mContentFormat = mFormat;
                                mpBuffer->mVideoWidth = mVideoWidth;
                                mpBuffer->mVideoHeight = mVideoHeight;
                                mpBuffer->mVideoOffsetX = 0;
                                mpBuffer->mVideoOffsetY = 0;

                                mpBuffer->mVideoFrameRateNum = mVideoFramerateNum;
                                mpBuffer->mVideoFrameRateDen = mVideoFramerateDen;
                                mpBuffer->mVideoRoughFrameRate = mVideoFramerate;

                                mpBuffer->mVideoSampleAspectRatioNum = 1;
                                mpBuffer->mVideoSampleAspectRatioDen = 1;

                                mBufferSessionNumber++;
                                //mbSendSyncPointBuffer = 1;//for demuxer notify video decoder on the fly
                            }
                            mpBuffer->mSessionNumber = mBufferSessionNumber;
                            mpBuffer->mContentFormat = StreamFormat_H264;
                            //send packet
                            //LOGM_DEBUG("send buffer 1 %p, data %p, size %d, flag 0x%x, type %d\n", mpBuffer, mpBuffer->GetDataPtr(), mpBuffer->GetDataSize(), mpBuffer->GetBufferFlags(), mpBuffer->GetBufferType());
                            mpOutputPin->SendBuffer(mpBuffer);
                            //printf("xxxxxxxxx  Send SPS PPS, timestamp [%u], pts [%lld]\n", (TU32)mpBuffer->GetBufferNativePTS(),(TS64)mpBuffer->GetBufferPTS());

                            mDebugHeartBeat ++;
                            mpBuffer = NULL;
                            mbSetTimeStamp = 0;
                            mbSendSequenceData = 1;

                            if (mpBufferPool->GetFreeBufferCnt()) {
                                mDebugWaitBufferpoolFlag = 2;
                                if (DUnlikely(!mpOutputPin->AllocBuffer(mpBuffer))) {
                                    LOG_ERROR("demuxer AllocBuffer Fail, must not comes here!\n");
                                    mbRun = 0;
                                    msState = DATA_THREAD_STATE_ERROR;
                                    break;
                                } else {
                                    mpBuffer->SetBufferFlags(0);
                                }
                                mDebugWaitBufferpoolFlag = 0;
                                mpBuffer->mpCustomizedContent = (void *)mpMemPool;
                                mpBuffer->mCustomizedContentType = EBufferCustomizedContentType_RingBuffer;
                                mDebugWaitMempoolFlag = 2;
                                mpMemoryStart = mpMemPool->RequestMemBlock(mRequestMemorySize);
                                mDebugWaitMempoolFlag = 0;
                            } else {
                                msState = DATA_THREAD_STATE_WAIT_OUTBUFFER;
                            }
                            break;
                        } else {
                            //continue wait
                            //LOG_ERROR(" not sps type %d.\n", mNalType);
                        }
                        break;
                    } else {
                        mpBuffer->SetBufferFlags(0);
                        mpBuffer->mVideoFrameType = EPredefinedPictureType_P;
                    }

                    mpBuffer->SetDataPtr(mpCurrentPointer);
                    mpBuffer->SetBufferType(EBufferType_VideoES);
                    mpBuffer->SetDataSize((TUint)(mReadLen + (mStartCodeLength - 1) + 2 - mReservedDataLength));
                    mpBuffer->SetDataPtrBase(mpMemoryStart);
                    mCurrentMemorySize = mReadLen;
                    mpBuffer->SetDataMemorySize(mCurrentMemorySize);
                    mpMemPool->ReturnBackMemBlock(mRequestMemorySize - mCurrentMemorySize, mpMemoryStart + mCurrentMemorySize);

                    //send packet
                    mpBuffer->mSessionNumber = mBufferSessionNumber;
                    mpBuffer->mContentFormat = StreamFormat_H264;

                    if (DUnlikely(mbBrokenFrame)) {
                        mpBuffer->SetBufferFlagBits(CIBuffer::BROKEN_FRAME);
                    }
                    //LOGM_DEBUG("send buffer 2 %p, data %p, size %d, flag 0x%x, type %d\n", mpBuffer, mpBuffer->GetDataPtr(), mpBuffer->GetDataSize(), mpBuffer->GetBufferFlags(), mpBuffer->GetBufferType());
                    mpOutputPin->SendBuffer(mpBuffer);
                    mDebugHeartBeat ++;
                    mpBuffer = NULL;
                    mbSetTimeStamp = 0;
                    mFakePts += mVideoFramerateDen;

                    if (DLikely(mpBufferPool->GetFreeBufferCnt())) {
                        mDebugWaitBufferpoolFlag = 3;
                        if (DUnlikely(!mpOutputPin->AllocBuffer(mpBuffer))) {
                            LOG_ERROR("demuxer AllocBuffer Fail, must not comes here!\n");
                            mbRun = 0;
                            msState = DATA_THREAD_STATE_ERROR;
                            break;
                        } else {
                            mpBuffer->SetBufferFlags(0);
                        }
                        mDebugWaitBufferpoolFlag = 0;
                        mpBuffer->mpCustomizedContent = (void *)mpMemPool;
                        mpBuffer->mCustomizedContentType = EBufferCustomizedContentType_RingBuffer;
                        mDebugWaitMempoolFlag = 3;
                        mpMemoryStart = mpMemPool->RequestMemBlock(mRequestMemorySize);
                        mDebugWaitMempoolFlag = 0;
                    } else {
                        mpBuffer = NULL;
                        msState = DATA_THREAD_STATE_WAIT_OUTBUFFER;
                    }

                    //clear
                    mTotalWritenLength = 0;
                    break;
                } else {
                    if (DUnlikely(mbWaitFirstSpsPps)) {
                        mNalType = (mpCurrentPointer[mRTPHeaderLength - 1]) & 0x1f;
                        //DASSERT(mpCurrentPointer[mRTPHeaderLength - 1] & 0x80);

                        if (ENalType_IDR == mNalType) {
                            LOGM_INFO("wait first sps pps 2... IDR comes mNalType %d, mpH264spspps %p, mpVideoExtraDataFromSDP %p\n", mNalType, mpH264spspps, mpVideoExtraDataFromSDP);
                            if (DLikely(mpH264spspps)) {
                                mPersistExtradataBuffer.SetDataPtr(mpH264spspps);
                                mPersistExtradataBuffer.SetDataSize(mH264spsppsSize);
                                mPersistExtradataBuffer.SetDataPtrBase(NULL);
                                mPersistExtradataBuffer.SetDataMemorySize(0);
                                mPersistExtradataBuffer.SetBufferType(EBufferType_VideoExtraData);
                                //mPersistExtradataBuffer.SetBufferFlagBits(CIBuffer::SYNC_POINT);
                                if (!mbSendSyncPointBuffer) {
                                    mPersistExtradataBuffer.SetBufferFlagBits(CIBuffer::SYNC_POINT);
                                    mPersistExtradataBuffer.mContentFormat = mFormat;
                                    mPersistExtradataBuffer.mVideoWidth = mVideoWidth;
                                    mPersistExtradataBuffer.mVideoHeight = mVideoHeight;
                                    mPersistExtradataBuffer.mVideoOffsetX = 0;
                                    mPersistExtradataBuffer.mVideoOffsetY = 0;

                                    mPersistExtradataBuffer.mVideoFrameRateNum = mVideoFramerateNum;
                                    mPersistExtradataBuffer.mVideoFrameRateDen = mVideoFramerateDen;
                                    mPersistExtradataBuffer.mVideoRoughFrameRate = mVideoFramerate;

                                    mPersistExtradataBuffer.mVideoSampleAspectRatioNum = 1;
                                    mPersistExtradataBuffer.mVideoSampleAspectRatioDen = 1;

                                    mBufferSessionNumber++;
                                    //mbSendSyncPointBuffer = 1;//for demuxer notify video decoder on the fly
                                }
                                mpBuffer->mContentFormat = StreamFormat_H264;
                                mpOutputPin->SendBuffer(&mPersistExtradataBuffer);
                                mDebugHeartBeat ++;
                                mbWaitFirstSpsPps = 0;
                            } else if (DLikely(mpVideoExtraDataFromSDP)) {
                                mPersistExtradataBuffer.SetDataPtr(mpVideoExtraDataFromSDP);
                                mPersistExtradataBuffer.SetDataSize(mVideoExtraDataSize);
                                mPersistExtradataBuffer.SetDataPtrBase(NULL);
                                mPersistExtradataBuffer.SetDataMemorySize(0);
                                mPersistExtradataBuffer.SetBufferType(EBufferType_VideoExtraData);
                                //mPersistExtradataBuffer.SetBufferFlagBits(CIBuffer::SYNC_POINT);
                                if (!mbSendSyncPointBuffer) {
                                    mPersistExtradataBuffer.SetBufferFlagBits(CIBuffer::SYNC_POINT);
                                    mPersistExtradataBuffer.mContentFormat = mFormat;
                                    mPersistExtradataBuffer.mVideoWidth = mVideoWidth;
                                    mPersistExtradataBuffer.mVideoHeight = mVideoHeight;
                                    mPersistExtradataBuffer.mVideoOffsetX = 0;
                                    mPersistExtradataBuffer.mVideoOffsetY = 0;

                                    mPersistExtradataBuffer.mVideoFrameRateNum = mVideoFramerateNum;
                                    mPersistExtradataBuffer.mVideoFrameRateDen = mVideoFramerateDen;
                                    mPersistExtradataBuffer.mVideoRoughFrameRate = mVideoFramerate;

                                    mPersistExtradataBuffer.mVideoSampleAspectRatioNum = 1;
                                    mPersistExtradataBuffer.mVideoSampleAspectRatioDen = 1;

                                    mBufferSessionNumber++;
                                    //mbSendSyncPointBuffer = 1;//for demuxer notify video decoder on the fly
                                }
                                mPersistExtradataBuffer.mContentFormat = StreamFormat_H264;
                                mpOutputPin->SendBuffer(&mPersistExtradataBuffer);
                                mDebugHeartBeat ++;
                                mbWaitFirstSpsPps = 0;
                            } else {
                                LOGM_ERROR("no extra data!\n");
                            }
                        } else {
                            LOGM_DEBUG("wait first sps pps... mNalType %d\n", mNalType);
                            continue;
                        }
                    }
                }

                mpBuffer->SetDataPtr(mpCurrentPointer + mReservedDataLength - mStartCodeLength);
                mpBuffer->SetBufferType(EBufferType_VideoES);

                //nal type
                mNalType = mpCurrentPointer[mReservedDataLength - 1] & 0x1f;
                mNri = mpCurrentPointer[mReservedDataLength - 2] & 0x60;
                mpCurrentPointer[mReservedDataLength - 1] = mNalType | mNri;

                //LOGM_VERBOSE("mNalType %d, %02x %02x, %02x %02x %02x %02x\n", mNalType, mpCurrentPointer[mReservedDataLength - 2], mpCurrentPointer[mReservedDataLength - 1], mpCurrentPointer[mReservedDataLength], mpCurrentPointer[mReservedDataLength + 1], mpCurrentPointer[mReservedDataLength + 2], mpCurrentPointer[mReservedDataLength + 3]);
                if (DUnlikely(ENalType_IDR == mNalType)) {
                    mpBuffer->SetBufferFlags(CIBuffer::KEY_FRAME);
                    mpBuffer->mVideoFrameType = EPredefinedPictureType_IDR;
                } else if (DUnlikely((ENalType_SPS == mNalType) || (ENalType_PPS == mNalType))) {
                    mpBuffer->SetBufferFlags(CIBuffer::KEY_FRAME);
                    mpBuffer->mVideoFrameType = EPredefinedPictureType_IDR;
                } else {
                    mpBuffer->mVideoFrameType = EPredefinedPictureType_P;
                }

                //fill start code prefix with first packet
                mpCurrentPointer[mReservedDataLength - mStartCodeLength] = 0;
                mpCurrentPointer[mReservedDataLength - mStartCodeLength + 1] = 0;
                mpCurrentPointer[mReservedDataLength - mStartCodeLength + 2] = 0;
                mpCurrentPointer[mReservedDataLength - mStartCodeLength + 3] = 0x1;

                //update write pointer
                mpCurrentPointer += mReadLen;

                //reserve data
                mpCurrentPointer -= mReservedDataLength;
                memcpy(mReservedData, mpCurrentPointer, mReservedDataLength);
                mTotalWritenLength += mReadLen - mReservedDataLength + mStartCodeLength;
                msState = DATA_THREAD_STATE_READ_REMANING_RTP_PACKET;
                break;

            case DATA_THREAD_STATE_READ_REMANING_RTP_PACKET:
                //DASSERT(mpBuffer);

                //debug assert
                //DASSERT(mTotalWritenLength);
                //DASSERT(mpBuffer);
                //DASSERT(mpCurrentPointer != mpMemoryStart);

                mFromLen = sizeof(mSrcAddr);
                mDebugWaitReadSocketFlag = 2;
                mReadLen = gfNet_RecvFrom(mRTPSocket, mpCurrentPointer, mRequestMemorySize - mTotalWritenLength - mReservedDataLength, 0, (void *)&mSrcAddr, (TSocketSize *)&mFromLen);
                mDebugWaitReadSocketFlag = 0;
                //DASSERT(mReadLen <= (((TInt)mRequestMemorySize) - mTotalWritenLength - mReservedDataLength));
                if (checkRtpPacket(mpCurrentPointer, mReadLen) < 0) {
                    break;//TODO
                }

                //parse rtp header, get some information
                mRTPLastSeqNumber = mRTPCurrentSeqNumber;
                __parseRTPPacketHeader(mpCurrentPointer, mRTPTimeStamp, mRTPCurrentSeqNumber, mPacketSSRC);
                mLastRtcpTimeStamp = mRTPTimeStamp;

                /*
                                if (mRTPLastSeqNumber > mRTPCurrentSeqNumber) {
                                    mRTPStatistics.cycles += (1 << 16);
                                }
                                mRTPStatistics.received ++;
                                mRTPStatistics.max_seq = mRTPCurrentSeqNumber;
                */

#if 1
                if (DLikely(mRTPLastSeqNumber < mRTPCurrentSeqNumber)) {
                    //DASSERT((mRTPLastSeqNumber + 1) == mRTPCurrentSeqNumber);
                    if (DUnlikely((mRTPLastSeqNumber + 1) != mRTPCurrentSeqNumber)) {
                        LOGM_ERROR("last seq %d, cur seq %d, lose data packet!\n", mRTPLastSeqNumber, mRTPCurrentSeqNumber);
                        mbBrokenFrame = 1;
                    }
                }
#endif

                if (DUnlikely(mpCurrentPointer[1] & 0x80)) {
                    //DASSERT(mTotalWritenLength);
                    if (mTotalWritenLength) {
                        memcpy(mpCurrentPointer, mReservedData, mReservedDataLength);
                    }

                    mTotalWritenLength += mReadLen - mReservedDataLength;
                    mpBuffer->SetDataPtrBase(mpMemoryStart);
                    mCurrentMemorySize = (TInt)(mpCurrentPointer + mReadLen - mpMemoryStart);
                    mpBuffer->SetDataMemorySize((TUint)mCurrentMemorySize);

                    mpBuffer->SetBufferType(EBufferType_VideoES);
                    mpBuffer->SetDataSize((TUint)(mTotalWritenLength));

                    mpMemPool->ReturnBackMemBlock(mRequestMemorySize - mCurrentMemorySize, mpMemoryStart + mCurrentMemorySize);

                    //send packet
                    mpBuffer->mSessionNumber = mBufferSessionNumber;
                    if (DUnlikely(mbBrokenFrame)) {
                        mpBuffer->SetBufferFlagBits(CIBuffer::BROKEN_FRAME);
                    }

                    //LOGM_DEBUG("send buffer 3 %p, data %p, size %d, flag 0x%x, type %d\n", mpBuffer, mpBuffer->GetDataPtr(), mpBuffer->GetDataSize(), mpBuffer->GetBufferFlags(), mpBuffer->GetBufferType());
                    mDebugHeartBeat ++;
                    mpBuffer->mContentFormat = StreamFormat_H264;
                    mpOutputPin->SendBuffer(mpBuffer);
                    mpBuffer = NULL;
                    mbSetTimeStamp = 0;
                    mFakePts += mVideoFramerateDen;
                    if (DLikely(mpBufferPool->GetFreeBufferCnt())) {
                        mDebugWaitBufferpoolFlag = 5;
                        if (DUnlikely(!mpOutputPin->AllocBuffer(mpBuffer))) {
                            LOG_ERROR("demuxer AllocBuffer Fail, must not comes here!\n");
                            mbRun = 0;
                            msState = DATA_THREAD_STATE_ERROR;
                            break;
                        } else {
                            mpBuffer->SetBufferFlags(0);
                        }
                        mDebugWaitBufferpoolFlag = 0;

                        mpBuffer->mpCustomizedContent = (void *)mpMemPool;
                        mpBuffer->mCustomizedContentType = EBufferCustomizedContentType_RingBuffer;
                        mDebugWaitMempoolFlag = 5;
                        mpMemoryStart = mpMemPool->RequestMemBlock(mRequestMemorySize);
                        mDebugWaitMempoolFlag = 0;
                        msState = DATA_THREAD_STATE_READ_FIRST_RTP_PACKET;
                    } else {
                        msState = DATA_THREAD_STATE_WAIT_OUTBUFFER;
                    }
                    mTotalWritenLength = 0;
                    break;
                } else {
                    //DASSERT(mTotalWritenLength);
                    if (DLikely(mTotalWritenLength)) {
                        memcpy(mpCurrentPointer, mReservedData, mReservedDataLength);
                    }

                    mpCurrentPointer += mReadLen;
                    mpCurrentPointer -= mReservedDataLength;
                    memcpy(mReservedData, mpCurrentPointer, mReservedDataLength);
                    mTotalWritenLength += mReadLen - mReservedDataLength;
                }
                break;

            case DATA_THREAD_STATE_WAIT_OUTBUFFER:
                DASSERT(!mpBuffer);

                if (mpBufferPool->GetFreeBufferCnt()) {
                    mDebugWaitBufferpoolFlag = 6;
                    if (DUnlikely(!mpOutputPin->AllocBuffer(mpBuffer))) {
                        LOG_ERROR("demuxer AllocBuffer Fail, must not comes here!\n");
                        mbRun = 0;
                    } else {
                        mpBuffer->SetBufferFlags(0);
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
                    //LOGM_DEBUG("EECode_NotRunning\n");
                    return EECode_NotRunning;
                }
                break;

            case DATA_THREAD_STATE_SKIP_DATA:
                // to do, skip till next IDR
                LOGM_ERROR("add implenment.\n");
                break;

            case DATA_THREAD_STATE_ERROR:
                // to do, error case
                LOGM_ERROR("error state, DATA_THREAD_STATE_ERROR\n");
                break;

            default:
                LOGM_ERROR("unexpected msState %d\n", msState);
                break;
        }
    }

    return EECode_OK;
}

TInt CRTPH264ScheduledReceiver::IsPassiveMode() const
{
    return 0;
}

TSchedulingHandle CRTPH264ScheduledReceiver::GetWaitHandle() const
{
    return (TSchedulingHandle)mRTPSocket;
}

TSchedulingUnit CRTPH264ScheduledReceiver::HungryScore() const
{
    return 1;
}

TU8 CRTPH264ScheduledReceiver::GetPriority() const
{
    return mPriority;
}

CObject *CRTPH264ScheduledReceiver::GetObject0() const
{
    return (CObject *) this;
}

EECode CRTPH264ScheduledReceiver::EventHandling(EECode err)
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

void CRTPH264ScheduledReceiver::SetVideoDataPostProcessingCallback(void *callback_context, void *callback)
{
    LOGM_FATAL("not support!\n");
    return;
}

void CRTPH264ScheduledReceiver::SetAudioDataPostProcessingCallback(void *callback_context, void *callback)
{
    LOGM_FATAL("not support!\n");
    return;
}

void CRTPH264ScheduledReceiver::PrintStatus()
{
    TInt expected_count = mRTPStatistics.cycles + mRTPStatistics.max_seq - mRTPStatistics.base_seq + 1;
    TInt actual_received_count = mRTPStatistics.received;
    float lose_percentage = 0;

    if (expected_count) {
        lose_percentage = ((float)(expected_count - actual_received_count)) / (float)expected_count;
    }

    LOGM_PRINTF("expected receive packet count %d, actual received count %d, loss count %d, loss percentage %f, heart beat %d\n", expected_count, actual_received_count, expected_count - actual_received_count, lose_percentage, mDebugHeartBeat);
    LOGM_PRINTF("state %d, %s, free buffer count %d, reconnect count %d\n", msState, gfGetRTPRecieverStateString(msState), mpBufferPool->GetFreeBufferCnt(), mReconnectTag - 1);

    mDebugHeartBeat = 0;
}

void CRTPH264ScheduledReceiver::EventNotify(EEventType type, TU64 param1, TPointer param2)
{
    switch (type) {
        case EEventType_Timer:
            if (DLikely(mpSystemClockReference)) {
                if (DLikely(mpWatchDog)) {
                    TTime cur_time = (TTime)param1;//mpSystemClockReference->GetCurrentTime();
                    if (DUnlikely(cur_time > (mLastAliveTimeStamp + mpPersistMediaConfig->streaming_timeout_threashold))) {
                        SMSG msg;

                        LOGM_NOTICE("time out detected, cur_time %lld, mLastAliveTimeStamp %lld, diff %lld\n", cur_time, mLastAliveTimeStamp, cur_time - mLastAliveTimeStamp);
                        memset(&msg, 0x0, sizeof(msg));
                        msg.code = EMSGType_Timeout;
                        msg.owner_id = DGENERIC_ID_FROM_COMPONENT_TYPE_INDEX(EGenericComponentType_Demuxer, mIndex);
                        msg.owner_type = EGenericComponentType_Demuxer;
                        msg.owner_index = mIndex;

                        msg.pExtra = NULL;
                        msg.needFreePExtra = 0;
                        mpSystemClockReference->GuardedRemoveTimer(mpWatchDog);
                        mpWatchDog = NULL;
                        mpMsgSink->MsgProc(msg);
                    } else {
                        LOGM_INFO("alive\n");
                    }
                } else {
                    LOGM_FATAL("NULL mpWatchDog\n");
                }
            }
            break;

        default:
            LOGM_FATAL("BAD type %d\n", type);
            break;
    }
}

void CRTPH264ScheduledReceiver::writeRR()
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

    if (last_rtcp_ntp_time == DInvalidTimeStamp) {
        tmp = 0; /* last SR timestamp */
        delay_since_last = 0; /* delay since last SR */
    } else {
        tmp = last_rtcp_ntp_time >> 16; // this is valid, right? do we need to handle 64 bit values special?
        delay_since_last = 0;
    }
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

    mRTCPDataLen = len + 42;
    // padding
    unsigned char *padding = &mRTCPBuffer[42 + len];
    for (len = (6 + len) % 4; len % 4; len++) {
        *padding++ = 0;
        ++mRTCPDataLen;
    }
    DASSERT(mRTCPDataLen >= (len + 42));
}

void CRTPH264ScheduledReceiver::updateRR()
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

    //LOGM_DEBUG("[debug]: mRTPStatistics.expected_prior %d, mRTPStatistics.received_prior %d, mRTPStatistics.cycles %d, mRTPStatistics.max_seq %d, mRTPStatistics.base_seq %d\n", mRTPStatistics.expected_prior, mRTPStatistics.received_prior, mRTPStatistics.cycles, mRTPStatistics.max_seq, mRTPStatistics.base_seq);

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

    if (last_rtcp_ntp_time == DInvalidTimeStamp) {
        tmp = 0; /* last SR timestamp */
        delay_since_last = 0; /* delay since last SR */
    } else {
        tmp = last_rtcp_ntp_time >> 16; // this is valid, right? do we need to handle 64 bit values special?
        delay_since_last = 0;
    }
    mRTCPBuffer[24] = (tmp >> 24) & 0xff;
    mRTCPBuffer[25] = (tmp >> 16) & 0xff;
    mRTCPBuffer[26] = (tmp >> 8) & 0xff;
    mRTCPBuffer[27] = tmp & 0xff;
    mRTCPBuffer[28] = (delay_since_last >> 24) & 0xff;
    mRTCPBuffer[29] = (delay_since_last >> 16) & 0xff;
    mRTCPBuffer[30] = (delay_since_last >> 8) & 0xff;
    mRTCPBuffer[31] = delay_since_last & 0xff;
}

void CRTPH264ScheduledReceiver::receiveSR()
{
    //DASSERT(mRTCPSocket);
    if (DLikely(mRTCPSocket > 0)) {
        mRTCPReadDataLen = gfNet_RecvFrom(mRTCPSocket, mRTCPReadBuffer, sizeof(mRTCPReadBuffer), 0, (void *)&mSrcRTCPAddr, (TSocketSize *)&mFromLen);
        if (mRTCPReadDataLen < 12) {
            return;
        }
        //LOGM_NOTICE("[debug]: parse SR, to do, mRTCPReadDataLen %d\n", mRTCPReadDataLen);
        unsigned char *buf = mRTCPReadBuffer;
        int len = mRTCPReadDataLen;
        if ((buf[0] & 0xc0) != (2/*RTP_VERSION*/ << 6)) {
            return;
        }
        if (buf[1]  < 200/*RTCP_SR*/ || buf[1]  > 204/*RTCP_APP*/) {
            return;
        }
        while (len >= 4) {
            int payload_len = DMIN(len, (DREAD_BE16(buf + 2) + 1) * 4);
            switch (buf[1]) {
                case RTCP_SR:
                    if (payload_len < 20) {
                        printf("Invalid length for RTCP SR packet\n");
                        return;
                    }
                    last_rtcp_ntp_time = DREAD_BE64(buf + 8);
                    last_rtcp_timestamp = DREAD_BE32(buf + 16);
                    //printf("CRTPH264ScheduledReceiver::receiveSR --- last_rtcp_ntp_time[%lld]  last_rtcp_timestamp [%u]\n",last_rtcp_ntp_time,last_rtcp_timestamp);
                    if (first_rtcp_ntp_time == (TS64)DInvalidTimeStamp) {
                        first_rtcp_ntp_time = last_rtcp_ntp_time;
                        if (!base_timestamp)
                        { base_timestamp = last_rtcp_timestamp; }
                        rtcp_ts_offset = last_rtcp_timestamp - base_timestamp;
                        //printf("CRTPH264ScheduledReceiver::receiveSR --- base_timestamp [%u], last_rtcp_timestamp[%u],rtcp_ts_offset[%lld]\n",base_timestamp,last_rtcp_timestamp,rtcp_ts_offset);
                    }
                    break;
                case RTCP_BYE:
                    return;// -RTCP_BYE;
            }
            buf += payload_len;
            len -= payload_len;
        }
    }
}

void CRTPH264ScheduledReceiver::sendRR()
{
    //DASSERT(mRTCPSocket);
    if (DLikely(mRTCPSocket > 0)) {
        gfNet_SendTo(mRTCPSocket, mRTCPBuffer, mRTCPDataLen, 0, (const struct sockaddr *)&mSrcRTCPAddr, mFromLen);
        //LOGM_NOTICE("[debug]: send RR, mRTCPDataLen %d, %02x %02x %02x %02x, %02x %02x %02x %02x, %02x %02x %02x %02x, %02x %02x %02x %02x\n", mRTCPDataLen, mRTCPBuffer[0], mRTCPBuffer[1], mRTCPBuffer[2], mRTCPBuffer[3], mRTCPBuffer[4], mRTCPBuffer[5], mRTCPBuffer[6], mRTCPBuffer[7], mRTCPBuffer[8], mRTCPBuffer[9], mRTCPBuffer[10], mRTCPBuffer[11], mRTCPBuffer[12], mRTCPBuffer[13], mRTCPBuffer[14], mRTCPBuffer[15]);
    }
}

void CRTPH264ScheduledReceiver::check_send_rr(int oct_count)
{
#define RTCP_TX_RATIO_NUM 5
#define RTCP_TX_RATIO_DEN 1000
    /* TODO: I think this is way too often; RFC 1889 has algorithm for this */
    /* XXX: mpeg pts hardcoded. RTCP send every 0.5 seconds */
    octet_count += oct_count;
    int rtcp_bytes = ((octet_count - last_octet_count) * RTCP_TX_RATIO_NUM) / RTCP_TX_RATIO_DEN;
    rtcp_bytes /= 100;//50; // mmu_man: that's enough for me... VLC sends much less btw !?
    if (rtcp_bytes < 28) {
        return;
    }
    last_octet_count = octet_count;

    updateRR();
    sendRR();
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

