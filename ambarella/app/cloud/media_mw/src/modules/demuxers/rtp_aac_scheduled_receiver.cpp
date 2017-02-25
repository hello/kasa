/*******************************************************************************
 * rtp_aac_scheduled_receiver.cpp
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

#include "rtp_aac_scheduled_receiver.h"
#include "rtsp_demuxer.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

static void __parseRTPPacketHeader(TU8 *p_packet, TU32 &time_stamp, TU16 &seq_number, TU32 &ssrc)
{
    DASSERT(p_packet);
    if (p_packet) {
        time_stamp = (p_packet[7]) | (p_packet[6] << 8) | (p_packet[5] << 16) | (p_packet[4] << 24);
        ssrc = (p_packet[11]) | (p_packet[10] << 8) | (p_packet[9] << 16) | (p_packet[8] << 24);
        seq_number = (p_packet[2] << 8) | (p_packet[3]);
    }
}

CRTPAACScheduledReceiver::CRTPAACScheduledReceiver(TUint index, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink)
    : inherited("CRTPAACScheduledReceiver", index)
    , mTrackID(0)
    , mbRun(1)
    , mbInitialized(0)
    , mType(StreamType_Audio)
    , mFormat(StreamFormat_AAC)
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
    , mRTPHeaderLength(12)
    , mpMemoryStart(NULL)
    , mReadLen(0)
    , mRTPTimeStamp(0)
    , mRTPCurrentSeqNumber(0)
    , mRTPLastSeqNumber(0)
    , mpBuffer(NULL)
    , mRequestMemorySize(2048)
    , mCmd(0)
    , mDebugWaitMempoolFlag(0)
    , mDebugWaitBufferpoolFlag(0)
    , mDebugWaitReadSocketFlag(0)
    , mbGetSSRC(0)
    , mAudioChannelNumber(DDefaultAudioChannelNumber)
    , mAudioSampleFormat(AudioSampleFMT_S16)
    , mbAudioChannelInterlave(0)
    , mbParseMultipleAccessUnit(0)
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
    mpMutex = gfCreateMutex();
    DASSERT(mpMutex);
}
CRTPAACScheduledReceiver::~CRTPAACScheduledReceiver()
{
    if (mpMutex) {
        mpMutex->Delete();
        mpMutex = NULL;
    }
    if (mpExtraData) {
        free(mpExtraData);
        mpExtraData = NULL;
    }
}

EECode CRTPAACScheduledReceiver::Initialize(SRTPContext *context, SStreamCodecInfo *p_stream_info, TUint number_of_content)
{
    DASSERT(context);
    DASSERT(p_stream_info);

    AUTO_LOCK(mpMutex);
    if (mbInitialized) {
        LOGM_FATAL("already initialized?\n");
        return EECode_BadState;
    }

    if (mpPersistMediaConfig) {
        mbParseMultipleAccessUnit = mpPersistMediaConfig->rtsp_client_config.parse_multiple_access_unit;
    }

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

    mPayloadType = -1;

    //TODO
    mbGetSSRC = 0;
    mRTPTimeStamp = 0;
    mRTPCurrentSeqNumber = 0;
    mRTPLastSeqNumber = 0;
    mpBuffer = NULL;
    mbSendSyncPointBuffer = 0;
    mPriority = 0;
    mBufferSessionNumber = 0;
    mLastRtcpNtpTime = 0;
    mFirstRtcpNtpTime = 0;
    mLastRtcpTimeStamp = 0;
    mRtcpTimeStampOffset = 0;
    mPacketCount = 0;
    mOctetCount = 0;
    mLastOctetCount = 0;
    mRTCPCurrentTick = 0;
    mRTCPCoolDown = 768;
    mRTCPDataLen = 0;
    mRTCPReadDataLen = 0;
    mFakePts = 0;
    mLastPrintLostCount = 0;
    msState = DATA_THREAD_STATE_READ_FIRST_RTP_PACKET;

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
        DASSERT(StreamFormat_AAC == mFormat);
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
        mPayloadType = p_stream_info->payload_type;
    }

    LOGM_INFO("mAudioChannelNumber %d, mAudioSamplerate %d, mAudioBitrate %d, mAudioSampleFormat %d, mbAudioChannelInterlave %d\n", mAudioChannelNumber, mAudioSamplerate, mAudioBitrate, mAudioSampleFormat, mbAudioChannelInterlave);

    return EECode_OK;
}

EECode CRTPAACScheduledReceiver::ReInitialize(SRTPContext *context, SStreamCodecInfo *p_stream_info, TUint number_of_content)
{
    {
        AUTO_LOCK(mpMutex);
        mbInitialized = 0;
    }
    return Initialize(context, p_stream_info, number_of_content);
}

EECode CRTPAACScheduledReceiver::Flush()
{
    LOGM_FATAL("need implement\n");
    return EECode_OK;
}

EECode CRTPAACScheduledReceiver::Purge()
{
    LOGM_FATAL("need implement\n");
    return EECode_OK;
}

EECode CRTPAACScheduledReceiver::SetExtraData(TU8 *p_extradata, TU32 extradata_size, TU32 index)
{
    LOGM_WARN("need implement\n");
    return EECode_OK;
}

void CRTPAACScheduledReceiver::SetVideoDataPostProcessingCallback(void *callback_context, void *callback)
{
    LOGM_FATAL("not support!\n");
    return;
}

void CRTPAACScheduledReceiver::SetAudioDataPostProcessingCallback(void *callback_context, void *callback)
{
    LOGM_FATAL("not support!\n");
    return;
}

int CRTPAACScheduledReceiver::checkRtpPacket(unsigned char *packet, int len)
{
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
    seq  = DREAD_BE16(buf + 2);
    //timestamp = AV_RB32(buf + 4);
    //ssrc = AV_RB32(buf + 8);

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
        ext = (DREAD_BE16(buf + 2) + 1) << 2;

        if (len < ext)
        { return -1; }
        // skip past RTP header extension
        len -= ext;
        buf += ext;
    }

    check_send_rr(recv_len);
    return rv;
}

EECode CRTPAACScheduledReceiver::Scheduling(TUint times, TU32 inout_mask)
{
    AUTO_LOCK(mpMutex);
    TU8 *p_tmp = NULL;
    TU16 au_number = 1;

    //debug assert
    //DASSERT(mbInitialized);
    //DASSERT(StreamType_Audio == mType);
    //DASSERT(StreamFormat_AAC == mFormat);
    //DASSERT(mpOutputPin);
    //DASSERT(mpBufferPool);
    //DASSERT(mpMemPool);
    //DASSERT(mRTPSocket > 0);
    //DASSERT(mRTCPSocket > 0);

    mRTCPCurrentTick ++;
    if (mRTCPCurrentTick > mRTCPCoolDown) {
        //LOGM_NOTICE("before checkSRsendRR()\n");
        //checkSRsendRR();
        mRTCPCurrentTick = 0;
    }

    //LOG_NOTICE("[Data thread]: CRTPAACScheduledReceiver Scheduling.\n");

    while (times-- > 0) {
        //LOGM_STATE("Scheduling(%d): msState %d\n", mFormat, msState);

        if (EECode_OK == gfOSPollFdReadable(mRTCPSocket)) {
            receiveSR();
        }

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
                    mpExtraData = gfGenerateAudioExtraData(StreamFormat_AAC, mAudioSamplerate, mAudioChannelNumber, mnExtraDataSize);
                    ptmp->SetDataPtr(mpExtraData);
                    ptmp->SetDataSize(mnExtraDataSize);

                    mBufferSessionNumber++;
                    mbSendSyncPointBuffer = 1;

                    //LOGM_NOTICE("[DEBUG]: before send audio extra data buffer\n");

                    ptmp->mSessionNumber = mBufferSessionNumber;
                    mDebugHeartBeat ++;
                    mpOutputPin->SendBuffer(ptmp);
                }

                //LOGM_NOTICE("[Data thread]: aac data.\n");
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

                if (checkRtpPacket(mpMemoryStart, mReadLen) < 0) {
                    break;//TODO
                }

                mRTPLastSeqNumber = mRTPCurrentSeqNumber;
                __parseRTPPacketHeader(mpMemoryStart, mRTPTimeStamp, mRTPCurrentSeqNumber, mPacketSSRC);

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

                mpBuffer->SetBufferNativePTS((TTime)mRTPTimeStamp);
                mpBuffer->SetBufferLinearPTS(mFakePts);
                mFakePts += mAudioFrameSize;

                //all in one rtp packet, first packet has marker bit
                if (!mbParseMultipleAccessUnit) {
                    mpBuffer->SetDataPtr(mpMemoryStart + mRTPHeaderLength);
                    mpBuffer->SetBufferType(EBufferType_AudioES);
                    mpBuffer->SetBufferFlags(CIBuffer::KEY_FRAME | CIBuffer::RTP_AAC);
                    mpBuffer->SetDataSize((TUint)(mReadLen - mRTPHeaderLength));
                    mpBuffer->SetDataPtrBase(mpMemoryStart);
                    mpBuffer->SetDataMemorySize(mReadLen);

                    mpMemPool->ReturnBackMemBlock(mRequestMemorySize - mReadLen, mpMemoryStart + mReadLen);

                    mpBuffer->mSessionNumber = mBufferSessionNumber;

                    mDebugHeartBeat ++;
                    //send packet
                    mpOutputPin->SendBuffer(mpBuffer);
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
                } else {

                    p_tmp = mpMemoryStart + mRTPHeaderLength;
                    au_number = ((p_tmp[0] << 8) | p_tmp[1]) >> 4;

                    if (1 == au_number) {
                        mpBuffer->SetDataPtr(mpMemoryStart + mRTPHeaderLength + 4);
                        mpBuffer->SetBufferType(EBufferType_AudioES);
                        mpBuffer->SetBufferFlags(CIBuffer::KEY_FRAME);
                        mpBuffer->SetDataSize((TUint)(mReadLen - mRTPHeaderLength - 4));
                        mpBuffer->SetDataPtrBase(mpMemoryStart);
                        mpBuffer->SetDataMemorySize(mReadLen);

                        mpMemPool->ReturnBackMemBlock(mRequestMemorySize - mReadLen, mpMemoryStart + mReadLen);

                        mpBuffer->mSessionNumber = mBufferSessionNumber;

                        mDebugHeartBeat ++;
                        //send packet
                        mpOutputPin->SendBuffer(mpBuffer);
                        mpBuffer = NULL;
                    } else {
                        //process multiple au
                        TU16 au_size[8] = {0};
                        TU16 au_offset[8] = {0};
                        TU16 current_au = 0;
                        DASSERT(au_number < 9);

                        p_tmp += 2;
                        au_offset[0] = au_number * 2 + 2;
                        for (current_au = 0; current_au < au_number; current_au ++) {
                            au_size[current_au] = ((p_tmp[0] << 8) | p_tmp[1]) >> 3;
                            p_tmp += 2;
                            if ((current_au + 1) < au_number) {
                                au_offset[current_au + 1] = au_offset[current_au] + au_size[current_au];
                            }

                            if (!au_size[current_au]) {
                                continue;
                            }

                            //TU8* print_p = mpMemoryStart + mRTPHeaderLength + au_offset[current_au];
                            //LOGM_PRINTF("index %d, au_size[] %d, au_offset[] %d, data %02x %02x %02x %02x\n", current_au, au_size[current_au], au_offset[current_au], print_p[0], print_p[1], print_p[2], print_p[3]);
                            mpBuffer->SetDataPtr(mpMemoryStart + mRTPHeaderLength + au_offset[current_au]);
                            mpBuffer->SetBufferType(EBufferType_AudioES);
                            mpBuffer->SetBufferFlags(CIBuffer::KEY_FRAME);
                            mpBuffer->SetDataSize((TUint)(au_size[current_au]));
                            if (!current_au) {
                                mpBuffer->SetDataPtrBase(mpMemoryStart);
                                mpBuffer->SetDataMemorySize(mRTPHeaderLength + au_size[0] + au_offset[0]);
                                //LOGM_PRINTF("index 0: %p, base %p, plus size %p, buffer %p\n", mpMemoryStart + mRTPHeaderLength + au_offset[current_au], mpMemoryStart, mpMemoryStart + mRTPHeaderLength + au_size[0] + au_offset[0], mpBuffer);
                            } else {
                                mpBuffer->SetDataPtrBase(mpMemoryStart + mRTPHeaderLength + au_offset[current_au]);
                                mpBuffer->SetDataMemorySize(au_size[current_au]);
                                //LOGM_PRINTF("index %d: %p, base %p, plus size %p, buffer %p\n", current_au, mpMemoryStart + mRTPHeaderLength + au_offset[current_au], mpMemoryStart + mRTPHeaderLength + au_offset[current_au], mpMemoryStart + mRTPHeaderLength + au_offset[current_au] + au_size[current_au], mpBuffer);
                            }
                            mpBuffer->mSessionNumber = mBufferSessionNumber;

                            mDebugHeartBeat ++;
                            //send packet
                            mpBuffer->mpCustomizedContent = (void *)mpMemPool;
                            mpBuffer->mCustomizedContentType = EBufferCustomizedContentType_RingBuffer;
                            mpOutputPin->SendBuffer(mpBuffer);
                            mpBuffer = NULL;

                            if ((current_au + 1) < au_number) {
                                //LOGM_PRINTF("before request buffer, current count %d, mpBufferPool %p\n", mpBufferPool->GetFreeBufferCnt(), mpBufferPool);
                                if (DUnlikely(!mpOutputPin->AllocBuffer(mpBuffer))) {
                                    LOG_ERROR("demuxer AllocBuffer Fail, must not comes here!\n");
                                    mbRun = 0;
                                    msState = DATA_THREAD_STATE_ERROR;
                                    break;
                                }
                                //LOGM_PRINTF("after request buffer, current count %d\n", mpBufferPool->GetFreeBufferCnt());
                                mpBuffer->mpCustomizedContent = (void *)mpMemPool;
                                mpBuffer->mCustomizedContentType = EBufferCustomizedContentType_RingBuffer;
                            }
                        }
                        mpMemPool->ReturnBackMemBlock(mRequestMemorySize - mReadLen, mpMemoryStart + mReadLen);
                    }

                    if (!mpBuffer) {
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
                    } else {
                        mDebugWaitBufferpoolFlag = 0;

                        mpBuffer->mpCustomizedContent = (void *)mpMemPool;
                        mpBuffer->mCustomizedContentType = EBufferCustomizedContentType_RingBuffer;
                        mDebugWaitMempoolFlag = 3;
                        mpMemoryStart = mpMemPool->RequestMemBlock(mRequestMemorySize);
                        mDebugWaitMempoolFlag = 0;
                    }
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

TInt CRTPAACScheduledReceiver::IsPassiveMode() const
{
    return 0;
}

TSchedulingHandle CRTPAACScheduledReceiver::GetWaitHandle() const
{
    return (TSchedulingHandle)mRTPSocket;
}

EECode CRTPAACScheduledReceiver::EventHandling(EECode err)
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

TSchedulingUnit CRTPAACScheduledReceiver::HungryScore() const
{
    return 1;
}

TU8 CRTPAACScheduledReceiver::GetPriority() const
{
    return mPriority;
}

CObject *CRTPAACScheduledReceiver::GetObject0() const
{
    return (CObject *) this;
}

void CRTPAACScheduledReceiver::PrintStatus()
{
    TInt expected_count = mRTPStatistics.cycles + mRTPStatistics.max_seq - mRTPStatistics.base_seq + 1;
    TInt actual_received_count = mRTPStatistics.received;
    float lose_percentage = 0;

    if (expected_count) {
        lose_percentage = ((float)(expected_count - actual_received_count)) / (float)expected_count;
    }

    LOGM_PRINTF("expected receive packet count %d, actual received count %d, loss count %d, loss percentage %f, heart beat %d\n", expected_count, actual_received_count, expected_count - actual_received_count, lose_percentage, mDebugHeartBeat);
    LOGM_PRINTF("state %d, %s, free buffer count %d\n", msState, gfGetRTPRecieverStateString(msState), mpBufferPool->GetFreeBufferCnt());

    mDebugHeartBeat = 0;
}

void CRTPAACScheduledReceiver::writeRR()
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

void CRTPAACScheduledReceiver::updateRR()
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

#if 0
void CRTPAACScheduledReceiver::do_calc_pts(TS64 &pts, TU32 curr_timestamp)
{
#define RTP_NOTS_VALUE ((TU32)-1)

    //if (pkt->pts != (TS64)DInvalidTimeStamp || pkt->dts != (TS64)DInvalidTimeStamp)
    //    return; /* Timestamp already set by depacketizer */
    if (curr_timestamp == RTP_NOTS_VALUE)
    { return; }

    if (last_rtcp_ntp_time != (TS64)DInvalidTimeStamp) {
        TS64 addend;
        int delta_timestamp;

        /* compute pts from timestamp with received ntp_time */
        delta_timestamp = curr_timestamp - last_rtcp_timestamp;
        /* convert to the PTS timebase */
        addend = my_av_rescale(last_rtcp_ntp_time - first_rtcp_ntp_time, mAudioSamplerate, (TU64)1 << 32);
        pts = range_start_offset + rtcp_ts_offset + addend + delta_timestamp;
        return;
    }

    if (!base_timestamp)
    { base_timestamp = curr_timestamp; }
    /* assume that the difference is INT32_MIN < x < INT32_MAX, but allow the first timestamp to exceed INT32_MAX */
    if (!timestamp)
    { unwrapped_timestamp += curr_timestamp; }
    else
    { unwrapped_timestamp += (TS32)(curr_timestamp - timestamp); }
    timestamp = curr_timestamp;
    pts = unwrapped_timestamp + range_start_offset - base_timestamp;
}
#endif

void CRTPAACScheduledReceiver::receiveSR()
{
    //DASSERT(mRTCPSocket);
    if (DLikely(mRTCPSocket > 0)) {
        mRTCPReadDataLen = gfNet_RecvFrom(mRTCPSocket, mRTCPReadBuffer, sizeof(mRTCPReadBuffer), 0, (void *)&mSrcRTCPAddr, (TSocketSize *)&mFromLen);
        //LOGM_NOTICE("[debug]: parse SR, to do, mRTCPReadDataLen %d\n", mRTCPReadDataLen);
        if (mRTCPReadDataLen < 12) {
            return;
        }
        unsigned char *buf = mRTCPReadBuffer;
        int len = mRTCPReadDataLen;
        if ((buf[0] & 0xc0) != (2/*RTP_VERSION*/ << 6)) {
            return;
        }
        if (buf[1]  < 200/*RTCP_SR*/ || buf[1]  > 204/*RTCP_APP*/) {
            return;
        }
        while (len >= 4) {
            TInt  payload_len = DMIN(len, (DREAD_BE16(buf + 2) + 1) * 4);
            switch (buf[1]) {
                case RTCP_SR:
                    if (payload_len < 20) {
                        printf("Invalid length for RTCP SR packet\n");
                        return;
                    }
                    last_rtcp_ntp_time = DREAD_BE64(buf + 8);
                    last_rtcp_timestamp = DREAD_BE32(buf + 16);
                    if (first_rtcp_ntp_time == (TS64)DInvalidTimeStamp) {
                        first_rtcp_ntp_time = last_rtcp_ntp_time;
                        if (!base_timestamp)
                        { base_timestamp = last_rtcp_timestamp; }
                        rtcp_ts_offset = last_rtcp_timestamp - base_timestamp;
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

void CRTPAACScheduledReceiver::sendRR()
{
    DASSERT(mRTCPSocket);
    if (mRTCPSocket > 0) {
        gfNet_SendTo(mRTCPSocket, mRTCPBuffer, mRTCPDataLen, 0, (const struct sockaddr *)&mSrcRTCPAddr, mFromLen);
        //LOGM_NOTICE("[debug]: send RR, mRTCPDataLen %d\n", mRTCPDataLen);
    }
}

void CRTPAACScheduledReceiver::check_send_rr(int oct_count)
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

