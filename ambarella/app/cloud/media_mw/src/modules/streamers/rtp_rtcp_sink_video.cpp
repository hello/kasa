/*******************************************************************************
 * rtp_rtcp_sink_video.cpp
 *
 * History:
 *    2013/11/24 - [Zhi He] create file
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

#include "common_network_utils.h"

#include "media_mw_if.h"
#include "media_mw_utils.h"

#include "framework_interface.h"
#include "codec_misc.h"
#include "mw_internal.h"
#include "dsp_platform_interface.h"
#include "streaming_if.h"
#include "modules_interface.h"

#include "rtp_rtcp_sink_video.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

IStreamingTransmiter *gfCreateVideoRtpRtcpTransmiter(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink, TComponentIndex index)
{
    return CVideoRTPRTCPSink::Create(pname, pPersistMediaConfig, pMsgSink, index);
}

IStreamingTransmiter *CVideoRTPRTCPSink::Create(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink, TComponentIndex index)
{
    CVideoRTPRTCPSink *result = new CVideoRTPRTCPSink(pname, pPersistMediaConfig, pMsgSink, index);
    if (result && result->Construct() != EECode_OK) {
        delete result;
        result = NULL;
    }
    return result;
}

CVideoRTPRTCPSink::CVideoRTPRTCPSink(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink, TComponentIndex index)
    : inherited(pname, index)
    , mStreamingType(StreamTransportType_RTP)
    , mProtocolType(ProtocolType_UDP)
    , mFormat(StreamFormat_H264)
    , mType(StreamType_Video)
    , mpClockReference(NULL)
    , mpConfig(pPersistMediaConfig)
    , mpMsgSink(pMsgSink)
    , mbRTPSocketSetup(0)
    , mPayloadType(RTP_PT_H264)
    , mbSrBuilt(0)
    , mbUsePulseTransfer(0)
    , mRTPSocket(DInvalidSocketHandler)
    , mRTCPSocket(DInvalidSocketHandler)
    , mRTPPort(1800)
    , mRTCPPort(1801)
    , mpRTPBuffer(NULL)
    , mRTPBufferLength(0)
    , mRTPBufferTotalLength(DRecommandMaxUDPPayloadLength + 32)
    , mpRTCPBuffer(NULL)
    , mRTCPBufferLength(0)
    , mRTCPBufferTotalLength(128)
    , mpExtraData(NULL)
    , mExtraDataSize(0)
{
    new(&mSubSessionList) CIDoubleLinkedList();
    new(&mTCPSubSessionList) CIDoubleLinkedList();
}

EECode CVideoRTPRTCPSink::Construct()
{
    DSET_MODULE_LOG_CONFIG(ELogModuleStreamingTransmiter);
    //mConfigLogLevel = ELogLevel_Info;
#if 0
    if ((mpMutex = gfCreateMutex()) == NULL) {
        LOGM_FATAL("CVideoRTPRTCPSink::Construct(), gfCreateMutex() fail.\n");
        return EECode_NoMemory;
    }
#endif
    mpRTPBuffer = (TU8 *)DDBG_MALLOC(mRTPBufferTotalLength, "RTbf");
    if (DUnlikely(!mpRTPBuffer)) {
        LOGM_FATAL("NO Memory.\n");
        return EECode_NoMemory;
    }

    mpRTCPBuffer = (TU8 *)DDBG_MALLOC(mRTCPBufferTotalLength, "RTcb");
    if (DUnlikely(!mpRTCPBuffer)) {
        LOGM_FATAL("NO Memory.\n");
        return EECode_NoMemory;
    }

    mpClockReference = mpConfig->p_system_clock_reference;

    //mpTCPPulseSender = gfGetTCPPulseSender(mpConfig, mIndex);
    //mbUsePulseTransfer = mpConfig->network_config.mbUseTCPPulseSender;

#if 0
    //clear dump file
    TChar filename[128] = {0};
    snprintf(filename, 127, "streaming_in_%d.h264", mIndex);
    mpDumpFile = fopen(filename, "wb");

    if (DLikely(mpDumpFile)) {
        fclose(mpDumpFile);
        mpDumpFile = NULL;
    } else {
        LOG_WARN("open mpDumpFile fail.\n");
    }

    //clear dump file
    snprintf(filename, 127, "streaming_out_udp_%d.h264", mIndex);
    mpDumpFile1 = fopen(filename, "wb");

    if (DLikely(mpDumpFile1)) {
        fclose(mpDumpFile1);
        mpDumpFile1 = NULL;
    } else {
        LOG_WARN("open mpDumpFile1 fail.\n");
    }

    snprintf(filename, 127, "streaming_out_tcp_%d.h264", mIndex);
    mpDumpFile1 = fopen(filename, "wb");

    if (DLikely(mpDumpFile1)) {
        fclose(mpDumpFile1);
        mpDumpFile1 = NULL;
    } else {
        LOG_WARN("open mpDumpFile1 fail.\n");
    }
#endif

    return EECode_OK;
}

void CVideoRTPRTCPSink::Destroy()
{
    Delete();
}

void CVideoRTPRTCPSink::Delete()
{
#if 0
    if (mpMutex) {
        mpMutex->Delete();
        mpMutex = NULL;
    }
#endif

    if (mpRTPBuffer) {
        DDBG_FREE(mpRTPBuffer, "RTbf");
        mpRTPBuffer = NULL;
    }

    if (mpRTCPBuffer) {
        DDBG_FREE(mpRTCPBuffer, "RTcb");
        mpRTCPBuffer = NULL;
    }

    if (DIsSocketHandlerValid(mRTPSocket)) {
        gfNetCloseSocket(mRTPSocket);
        mRTPSocket = DInvalidSocketHandler;
    }

    if (DIsSocketHandlerValid(mRTCPSocket)) {
        gfNetCloseSocket(mRTCPSocket);
        mRTCPSocket = DInvalidSocketHandler;
    }

    if (mpExtraData) {
        DDBG_FREE(mpExtraData, "RTex");
        mpExtraData = NULL;
    }

    inherited::Delete();
}

CVideoRTPRTCPSink::~CVideoRTPRTCPSink()
{
    mSubSessionList.~CIDoubleLinkedList();
    mTCPSubSessionList.~CIDoubleLinkedList();
}

EECode CVideoRTPRTCPSink::SetupContext(StreamTransportType type, ProtocolType protocol_type, const SStreamCodecInfos *infos, TTime start_time)
{
    return EECode_OK;
}

EECode CVideoRTPRTCPSink::DestroyContext()
{
    return EECode_OK;
}

EECode CVideoRTPRTCPSink::UpdateStreamFormat(StreamType type, StreamFormat format)
{
    DASSERT(StreamType_Video == type);
    DASSERT(StreamFormat_H264 == format);
    return EECode_OK;
}

EECode CVideoRTPRTCPSink::Start()
{
    return EECode_OK;
}

EECode CVideoRTPRTCPSink::Stop()
{
    return EECode_OK;
}

void CVideoRTPRTCPSink::udpSendData(TU8 *p_data, TMemSize size, TTime pts, TUint update_time_stamp, TUint key_frame)
{
    CIDoubleLinkedList::SNode *p_node = mSubSessionList.FirstNode();
    SSubSessionInfo *sub_session = NULL;
    while (p_node) {
        sub_session = (SSubSessionInfo *)p_node->p_context;
        if (DLikely(sub_session)) {
            if (DUnlikely(sub_session->is_closed)) {
                LOGM_WARN("sub_session %p is_closed\n", sub_session);
                p_node = mSubSessionList.NextNode(p_node);
                continue;
            }

            if (DUnlikely(sub_session->wait_first_key_frame)) {
                if (!key_frame) {
                    LOGM_INFO("wait key frame, skip\n");
                    p_node = mSubSessionList.NextNode(p_node);
                    continue;
                } else if (1 == key_frame) {
                    sub_session->wait_first_key_frame = 0;
                } else if (DLikely(2 == key_frame)) {
                    sub_session->send_extradata = 1;
                }
            } else {
                //DASSERT(1 == sub_session->send_extradata);
            }

            if (DUnlikely(!sub_session->is_started)) {
                sub_session->start_time = 0;//TODO
                sub_session->cur_time = sub_session->start_time;
                sub_session->is_started = 1;
            }
            TU32 timestamp = sub_session->cur_time;
            updateSendRtcp(sub_session, timestamp, (TU32)size);

            p_data[2] = sub_session->seq_number >> 8;
            p_data[3] = sub_session->seq_number & 0xff;
            p_data[4] = (timestamp >> 24) & 0xff;
            p_data[5] = (timestamp >> 16) & 0xff;
            p_data[6] = (timestamp >> 8) & 0xff;
            p_data[7] = timestamp & 0xff;
            TU32 ssrc = sub_session->rtp_ssrc;
            p_data[8] = (ssrc >> 24) & 0xff;
            p_data[9] = (ssrc >> 16) & 0xff;
            p_data[10] = (ssrc >> 8) & 0xff;
            p_data[11] = ssrc & 0xff;

            sub_session->seq_number ++;

            TInt ret = gfNet_SendTo(mRTPSocket, p_data, size, 0, (const void *)&sub_session->addr_port, (TSocketSize)sizeof(struct sockaddr_in));
            if (DUnlikely(ret < 0)) {
                LOGM_ERROR("udpSendData failed, mRTPSocket %d\n", mRTPSocket);

                sub_session->is_closed = 1;
                {
                    LOGM_INFO("send EMSGType_StreamingError_TCPSocketConnectionClose msg, sub_session %p\n", sub_session);
                    SMSG msg;
                    memset(&msg, 0x0, sizeof(msg));
                    msg.owner_id = DGENERIC_ID_FROM_COMPONENT_TYPE_INDEX(EGenericComponentType_StreamingTransmiter, 0);
                    msg.owner_type = EGenericComponentType_StreamingTransmiter;
                    msg.owner_index = 0;
                    msg.pExtra = NULL;
                    msg.needFreePExtra = 0;

                    msg.code = EMSGType_StreamingError_UDPSocketInvalidArgument;
                    msg.p0 = (TULong)sub_session->parent;
                    msg.p1 = (TULong)sub_session->parent->p_streaming_transmiter_filter;
                    msg.p2 = (TULong)sub_session->parent->p_server;
                    mpMsgSink->MsgProc(msg);
                }
            } else {
                rtcp_stat_t *s = &sub_session->rtcp_stat;
                s->octet_count += size;
                ++s->packet_count;
            }

            if (update_time_stamp) {
                if (DLikely(sub_session->p_content && (DDefaultVideoFramerateNum == sub_session->p_content->video_framerate_num) && (sub_session->p_content->video_framerate_den))) {
                    sub_session->cur_time += sub_session->p_content->video_framerate_den;
                    //LOGM_PRINTF("sub_session->cur_time, framerate den %d\n", sub_session->p_content->video_framerate_den);
                } else {
                    sub_session->cur_time += DDefaultVideoFramerateDen;
                }
#if 0
                TTime time_diff = ((TU64)(mpClockReference->GetCurrentTime() - sub_session->begin_time)) >> 7;
                TTime pts_diff = ((TU64)(sub_session->cur_time - sub_session->start_time)) >> 7;

                time_diff = time_diff * 9;
                pts_diff = pts_diff * 100;
                // 1/70000
                if ((21000 + time_diff) < pts_diff) {
                    LOGM_INFO("force time drift backword, time_diff %lld, pts_diff %lld, diff %lld\n", time_diff, pts_diff, pts_diff - time_diff);
                    sub_session->cur_time -= (sub_session->p_content->video_framerate_den) >> 2;
                } else if ((21000 + pts_diff) < time_diff) {
                    LOGM_INFO("force pts drift forward, time_diff %lld, pts_diff %lld, diff %lld\n", time_diff, pts_diff, time_diff - pts_diff);
                    sub_session->cur_time += (sub_session->p_content->video_framerate_den) >> 2;
                }
#endif
            }
        }
        p_node = mSubSessionList.NextNode(p_node);
    }
}

void CVideoRTPRTCPSink::tcpSendData(TU8 *p_data, TMemSize size, TTime pts, TUint update_time_stamp, TUint key_frame)
{
    CIDoubleLinkedList::SNode *p_node = mTCPSubSessionList.FirstNode();
    SSubSessionInfo *sub_session = NULL;
    while (p_node) {
        sub_session = (SSubSessionInfo *)p_node->p_context;
        if (DLikely(sub_session)) {

            if (DUnlikely(sub_session->is_closed)) {
                LOGM_WARN("sub_session %p is_closed\n", sub_session);
                p_node = mTCPSubSessionList.NextNode(p_node);
                continue;
            }

            if (DUnlikely(sub_session->wait_first_key_frame)) {
                if (!key_frame) {
                    LOGM_INFO("wait key frame, skip\n");
                    p_node = mTCPSubSessionList.NextNode(p_node);
                    continue;
                } else if (1 == key_frame) {
                    sub_session->wait_first_key_frame = 0;
                } else if (DLikely(2 == key_frame)) {
                    sub_session->send_extradata = 1;
                }
            } else {
                if (DLikely(2 == key_frame)) {
                    sub_session->send_extradata = 1;
                }
                //DASSERT(1 == sub_session->send_extradata);
            }

            if (DUnlikely(!sub_session->is_started)) {

                sub_session->statistics.first_seq_num = sub_session->seq_number;
                sub_session->statistics.cur_seq_num = sub_session->seq_number;

                sub_session->statistics.octet_count = size;
                sub_session->statistics.packet_count = 1;
                sub_session->statistics.last_octet_count = size;

                sub_session->rtcp_sr_cooldown = 128;
                sub_session->rtcp_sr_current_count = 0;

                sub_session->cur_time = sub_session->start_time;
            }

            DASSERT(sub_session->rtp_over_rtsp);
            p_data[1] = sub_session->rtp_channel;

            TTime value = sub_session->cur_time;
            p_data[6] = sub_session->seq_number >> 8;
            p_data[7] = sub_session->seq_number & 0xff;
            p_data[8] = (value >> 24) & 0xff;
            p_data[9] = (value >> 16) & 0xff;
            p_data[10] = (value >> 8) & 0xff;
            p_data[11] = value & 0xff;
            value = sub_session->rtp_ssrc;
            p_data[12] = (value >> 24) & 0xff;
            p_data[13] = (value >> 16) & 0xff;
            p_data[14] = (value >> 8) & 0xff;
            p_data[15] = value & 0xff;

            //if (DLikely(!(sub_session->p_transfer_channel && mpTCPPulseSender))) {
            TInt ret = gfNet_Send_timeout(sub_session->rtsp_fd, p_data, size, 0, 0);
            if (DUnlikely(ret != ((TInt) size))) {
                LOGM_ERROR("gfNet_Send error, ret %d, expected %ld, mpMsgSink %p\n", ret, size, mpMsgSink);
                if ((!sub_session->is_closed) && (0 == ret)) {
                    LOGM_INFO("send EMSGType_StreamingError_TCPSocketConnectionClose msg, sub_session %p\n", sub_session);
                    SMSG msg;
                    memset(&msg, 0x0, sizeof(msg));
                    msg.owner_id = DGENERIC_ID_FROM_COMPONENT_TYPE_INDEX(EGenericComponentType_StreamingTransmiter, 0);
                    msg.owner_type = EGenericComponentType_StreamingTransmiter;
                    msg.owner_index = 0;
                    msg.pExtra = NULL;
                    msg.needFreePExtra = 0;

                    msg.code = EMSGType_StreamingError_TCPSocketConnectionClose;
                    msg.p0 = (TULong)sub_session->parent;
                    msg.p1 = (TULong)sub_session->parent->p_streaming_transmiter_filter;
                    msg.p2 = (TULong)sub_session->parent->p_server;
                    mpMsgSink->MsgProc(msg);
                }
                sub_session->is_closed = 1;

                p_node = mTCPSubSessionList.NextNode(p_node);
                continue;
            }
#if 0
        } else {
            EECode err = mpTCPPulseSender->SendData(sub_session->p_transfer_channel, p_data, size);
            if (DUnlikely(EECode_OK != err)) {
                if (!sub_session->is_closed) {
                    LOGM_WARN("send EMSGType_StreamingError_TCPSocketConnectionClose msg, sub_session %p\n", sub_session);
                    SMSG msg;
                    memset(&msg, 0x0, sizeof(msg));
                    msg.owner_id = DGENERIC_ID_FROM_COMPONENT_TYPE_INDEX(EGenericComponentType_StreamingTransmiter, 0);
                    msg.owner_type = EGenericComponentType_StreamingTransmiter;
                    msg.owner_index = 0;
                    msg.pExtra = NULL;
                    msg.needFreePExtra = 0;

                    msg.code = EMSGType_StreamingError_TCPSocketConnectionClose;
                    msg.p0 = (TULong)sub_session->parent;
                    msg.p1 = (TULong)sub_session->parent->p_streaming_transmiter_filter;
                    msg.p2 = (TULong)sub_session->parent->p_server;
                    mpMsgSink->MsgProc(msg);
                }
                sub_session->is_closed = 1;

                p_node = mTCPSubSessionList.NextNode(p_node);
                continue;
            }
        }
#endif

        if (DUnlikely(!sub_session->is_started)) {
            sub_session->is_started = 1;
            sub_session->ntp_time = sub_session->ntp_time_base = mpClockReference->GetCurrentTime();
            LOGM_INFO("video ntp time:\t %lld\n", sub_session->ntp_time);
            //processRTCP(sub_session, sub_session->ntp_time, (TU32)sub_session->start_time);
            sub_session->rtcp_sr_current_count = 0;
        } else {
            sub_session->statistics.cur_seq_num = sub_session->seq_number;
            sub_session->statistics.last_octet_count = size;

            sub_session->statistics.octet_count += size;
            sub_session->statistics.packet_count ++;

            sub_session->rtcp_sr_current_count ++;

            if (DUnlikely(sub_session->rtcp_sr_current_count > sub_session->rtcp_sr_cooldown)) {
                sub_session->ntp_time = mpClockReference->GetCurrentTime();
                LOGM_DEBUG("video ntp time:\t %lld\n", sub_session->ntp_time);
                //processRTCP(sub_session, sub_session->ntp_time, sub_session->cur_time);
                sub_session->rtcp_sr_current_count = 0;
            }
        }

        if (update_time_stamp) {
            if (DLikely(sub_session->p_content && (DDefaultVideoFramerateNum == sub_session->p_content->video_framerate_num) && (sub_session->p_content->video_framerate_den))) {
                sub_session->cur_time += sub_session->p_content->video_framerate_den;
                //LOGM_PRINTF("sub_session->cur_time, framerate den %d\n", sub_session->p_content->video_framerate_den);
            } else {
                sub_session->cur_time += DDefaultVideoFramerateDen;
            }

        }
        sub_session->seq_number ++;
    }
    p_node = mTCPSubSessionList.NextNode(p_node);
}
}

void CVideoRTPRTCPSink::udpSendSr(SSubSessionInfo *sub_session)
{
    DASSERT(sub_session);
    TInt ret = gfNet_SendTo(mRTCPSocket, mpRTCPBuffer, mRTCPBufferLength, 0, (const void *)&sub_session->addr_port, (TSocketSize)sizeof(struct sockaddr_in));
    if (ret < 0) {
        LOGM_ERROR("udpSendSr failed --- mRTPSocket %d\n", mRTPSocket);
    }
}

EECode CVideoRTPRTCPSink::SetExtraData(TU8 *pdata, TMemSize size)
{
    //LOGM_WARN("SetExtraData, size %ld\n", size);
    if ((mpExtraData) && (mExtraDataSize < size)) {
        DDBG_FREE(mpExtraData, "RTex");
        mpExtraData = NULL;
        mExtraDataSize = 0;
    }

    if (!mpExtraData) {
        mpExtraData = (TU8 *) DDBG_MALLOC(size, "RTex");
        if (DUnlikely(!mpExtraData)) {
            return EECode_NoMemory;
        }
        mExtraDataSize = size;
    }

    memcpy(mpExtraData, pdata, mExtraDataSize);

    return EECode_OK;
}

EECode CVideoRTPRTCPSink::SendData(CIBuffer *pBuffer)
{
    EECode ret = EECode_OK;

    if (DLikely(pBuffer)) {

        if (DUnlikely((!mSubSessionList.NumberOfNodes()) && (!mTCPSubSessionList.NumberOfNodes()))) {
            //LOGM_DEBUG("idle loop\n");
            return EECode_OK;
        }

        if (DLikely(StreamFormat_H264 == mFormat)) {

            TU8 *p_data = pBuffer->GetDataPtr();
            TMemSize data_size = pBuffer->GetDataSize();
            TTime pts = pBuffer->GetBufferPTS();
            TUint is_extradata = 0;
            TUint keyframe = 0;

            if (DUnlikely(EBufferType_VideoExtraData == pBuffer->GetBufferType())) {
                is_extradata = 1;
                keyframe = 2;
            } else if (DUnlikely(CIBuffer::KEY_FRAME & pBuffer->GetBufferFlags())) {
                DASSERT(EBufferType_VideoES == pBuffer->GetBufferType());
                keyframe = 1;
            }

            if (mSubSessionList.NumberOfNodes()) {
                ret = sendDataH264(p_data, data_size, pts, is_extradata, keyframe);
                DASSERT_OK(ret);
            }

            if (mTCPSubSessionList.NumberOfNodes()) {
                ret = sendDataH264TCP(p_data, data_size, pts, is_extradata, keyframe);
                DASSERT_OK(ret);
            }

        } else {
            LOGM_FATAL("video format is not h264?\n");
            ret = EECode_NotSupported;
        }
    } else {
        LOGM_FATAL("NULL pBuffer\n");
        ret = EECode_BadParam;
    }

    return ret;
}

EECode CVideoRTPRTCPSink::sendDataH264(TU8 *p_data, TMemSize data_size, TTime pts, TUint is_extradata, TUint key_frame)
{

    if (DUnlikely(is_extradata)) {
        TU8 *p1 = NULL;
        TMemSize size1 = 0;

        //debug assert
        DASSERT(!p_data[0]);
        DASSERT(!p_data[1]);
        DASSERT(!p_data[2]);
        DASSERT(1 == p_data[3]);
        p_data += 4;
        data_size -= 4;

        p1 = gfNALUFindNextStartCode(p_data, data_size);

        DASSERT(data_size <= DRecommandMaxRTPPayloadLength);

        if (DLikely(p1)) {
            mpRTPBuffer[0] = (RTP_VERSION << 6);
            mpRTPBuffer[1] = (RTP_PT_H264 & 0x7f) | 0x80;
            size1 = (TMemSize)(p1 - p_data - 4);
            DASSERT(((TMemSize)(p1 - 4)) > ((TMemSize)(p_data)));

            memcpy(mpRTPBuffer + 12, p_data, size1);
            udpSendData(mpRTPBuffer, size1 + 12, pts, 0, 2);

            size1 = (TMemSize)(p_data + data_size - p1);
            DASSERT(((TMemSize)(p_data + data_size)) > ((TMemSize)(p1)));

            memcpy(mpRTPBuffer + 12, p1, size1);
            udpSendData(mpRTPBuffer, size1 + 12, pts, 0, 2);
        } else {
            LOGM_FATAL("with pps?\n");
        }
        return EECode_OK;
    }

    //LOGM_DEBUG("[debug flow]: streamer send size %ld, start data: %02x %02x %02x %02x, %02x %02x %02x %02x, end data %02x %02x %02x %02x\n", data_size, p_data[0], p_data[1], p_data[2], p_data[3], p_data[4], p_data[5], p_data[6], p_data[7], p_data[data_size - 4], p_data[data_size - 3], p_data[data_size - 2], p_data[data_size - 1]);

    if (DLikely((0x0 == p_data[0]) && (0x0 == p_data[1]) && (0x0 == p_data[2]) && (0x1 == p_data[3]))) {
        //LOGM_VERBOSE("4 start code %02x %02x %02x %02x, %2x %2x\n", p_data[0], p_data[1], p_data[2], p_data[3], p_data[4], p_data[5]);
        p_data += 4;
        data_size -= 4;
    } else if (DLikely((0x0 == p_data[0]) && (0x0 == p_data[1]) && (0x1 == p_data[2]))) {
        //LOGM_VERBOSE("3 start code %02x %02x %02x %02x, %2x %2x\n", p_data[0], p_data[1], p_data[2], p_data[3], p_data[4], p_data[5]);
        p_data += 3;
        data_size -= 3;
    } else {
        //LOGM_VERBOSE("no start code's case, %02x %02x %02x %02x, %02x %02x %02x %02x\n", p_data[0], p_data[1], p_data[2], p_data[3], p_data[4], p_data[5], p_data[6], p_data[7]);
        p_data += 4;
        data_size -= 4;
    }

    if (data_size <= DRecommandMaxRTPPayloadLength) {
        mpRTPBuffer[0] = (RTP_VERSION << 6);
        mpRTPBuffer[1] = (RTP_PT_H264 & 0x7f) | 0x80;
        memcpy(mpRTPBuffer + 12, p_data, data_size);
        udpSendData(mpRTPBuffer, data_size + 12, pts, 1, key_frame);
    } else {
        TU8 type = p_data[0] & 0x1F;
        TU8 nri = p_data[0] & 0x60;
        TU8 indicator = 28 | nri;
        TU8 fu_header = type | (1 << 7);
        p_data += 1;
        data_size -= 1;

        while ((data_size + 2) > DRecommandMaxRTPPayloadLength) {
            //RTP header
            mpRTPBuffer[0] = (RTP_VERSION << 6);
            mpRTPBuffer[1] = (RTP_PT_H264 & 0x7f);//m and payload type
            mpRTPBuffer[12] = indicator;
            mpRTPBuffer[13] = fu_header;
            memcpy(mpRTPBuffer + 14, p_data, DRecommandMaxRTPPayloadLength - 2);
            udpSendData(mpRTPBuffer, DRecommandMaxRTPPayloadLength + 12, pts, 0, key_frame);
            p_data += DRecommandMaxRTPPayloadLength - 2;
            data_size -= DRecommandMaxRTPPayloadLength - 2;
            fu_header &= ~(1 << 7);
        }

        fu_header |= 1 << 6;
        mpRTPBuffer[0] = (RTP_VERSION << 6);
        mpRTPBuffer[1] = (RTP_PT_H264 & 0x7f) | 0x80;//m and payload type
        mpRTPBuffer[12] = indicator;
        mpRTPBuffer[13] = fu_header;
        memcpy(&mpRTPBuffer[14], p_data, data_size);
        udpSendData(mpRTPBuffer, data_size + 14, pts, 1, key_frame);
    }

    return EECode_OK;
}

EECode CVideoRTPRTCPSink::sendDataH264TCP(TU8 *p_data, TMemSize data_size, TTime pts, TUint is_extradata, TUint key_frame)
{
    if (DUnlikely(is_extradata)) {
        TU8 *p1 = NULL;
        TMemSize size1 = 0;

        //debug assert
        DASSERT(!p_data[0]);
        DASSERT(!p_data[1]);
        DASSERT(!p_data[2]);
        DASSERT(1 == p_data[3]);
        p_data += 4;
        data_size -= 4;

        p1 = gfNALUFindNextStartCode(p_data, data_size);
        //DASSERT(data_size <= DRecommandMaxRTPPayloadLength);

        if (DLikely(p1)) {
            mpRTPBuffer[0] = DRTP_OVER_RTSP_MAGIC;
            mpRTPBuffer[1] = 0;

            mpRTPBuffer[4] = (RTP_VERSION << 6);
            mpRTPBuffer[5] = (RTP_PT_H264 & 0x7f) | 0x80;

            size1 = (TMemSize)(p1 - p_data - 4);
            DASSERT(((TMemSize)(p1 - 4)) > ((TMemSize)(p_data)));

            mpRTPBuffer[2] = ((size1 + 12) >> 8) & 0xff;
            mpRTPBuffer[3] = (size1 + 12) & 0xff;

            memcpy(mpRTPBuffer + 12 + 4, p_data, size1);
            tcpSendData(mpRTPBuffer, size1 + 12 + 4, pts, 0, 2);

            size1 = (TMemSize)(p_data + data_size - p1);
            DASSERT(((TMemSize)(p_data + data_size)) > ((TMemSize)(p1)));

            mpRTPBuffer[2] = ((size1 + 12) >> 8) & 0xff;
            mpRTPBuffer[3] = (size1 + 12) & 0xff;

            memcpy(mpRTPBuffer + 12 + 4, p1, size1);
            tcpSendData(mpRTPBuffer, size1 + 12 + 4, pts, 0, 2);
        }
        return EECode_OK;
    }

    //LOGM_DEBUG("[debug flow]: streamer send size %ld, start data: %02x %02x %02x %02x, %02x %02x %02x %02x, end data %02x %02x %02x %02x\n", data_size, p_data[0], p_data[1], p_data[2], p_data[3], p_data[4], p_data[5], p_data[6], p_data[7], p_data[data_size - 4], p_data[data_size - 3], p_data[data_size - 2], p_data[data_size - 1]);
    if (DLikely((0x0 == p_data[0]) && (0x0 == p_data[1]) && (0x0 == p_data[2]) && (0x1 == p_data[3]))) {
        //LOGM_VERBOSE("4 start code %02x %02x %02x %02x, %2x %2x\n", p_data[0], p_data[1], p_data[2], p_data[3], p_data[4], p_data[5]);
        p_data += 4;
        data_size -= 4;
    } else if (DLikely((0x0 == p_data[0]) && (0x0 == p_data[1]) && (0x1 == p_data[2]))) {
        //LOGM_VERBOSE("3 start code %02x %02x %02x %02x, %2x %2x\n", p_data[0], p_data[1], p_data[2], p_data[3], p_data[4], p_data[5]);
        p_data += 3;
        data_size -= 3;
    } else {
        //LOGM_VERBOSE("no start code's case, %02x %02x %02x %02x, %02x %02x %02x %02x\n", p_data[0], p_data[1], p_data[2], p_data[3], p_data[4], p_data[5], p_data[6], p_data[7]);
        p_data += 4;
        data_size -= 4;
    }

    mpRTPBuffer[0] = DRTP_OVER_RTSP_MAGIC;
    mpRTPBuffer[1] = 0;

    if (data_size <= DRecommandMaxRTPPayloadLength) {
        mpRTPBuffer[4] = (RTP_VERSION << 6);
        mpRTPBuffer[5] = (RTP_PT_H264 & 0x7f) | 0x80;

        mpRTPBuffer[2] = ((data_size + 12) >> 8) & 0xff;
        mpRTPBuffer[3] = (data_size + 12) & 0xff;

        memcpy(mpRTPBuffer + 12 + 4, p_data, data_size);
        tcpSendData(mpRTPBuffer, data_size + 12 + 4, pts, 1, key_frame);
    } else {
        TU8 type = p_data[0] & 0x1F;
        TU8 nri = p_data[0] & 0x60;
        TU8 indicator = 28 | nri;
        TU8 fu_header = type | (1 << 7);
        p_data += 1;
        data_size -= 1;

        while ((data_size + 2) > DRecommandMaxRTPPayloadLength) {
            //RTP header
            mpRTPBuffer[4] = (RTP_VERSION << 6);
            mpRTPBuffer[5] = (RTP_PT_H264 & 0x7f);//m and payload type
            mpRTPBuffer[16] = indicator;
            mpRTPBuffer[17] = fu_header;

            mpRTPBuffer[2] = ((DRecommandMaxRTPPayloadLength + 12) >> 8) & 0xff;
            mpRTPBuffer[3] = (DRecommandMaxRTPPayloadLength + 12) & 0xff;

            memcpy(mpRTPBuffer + 18, p_data, DRecommandMaxRTPPayloadLength - 2);
            tcpSendData(mpRTPBuffer, DRecommandMaxRTPPayloadLength + 16, pts, 0, key_frame);
            p_data += DRecommandMaxRTPPayloadLength - 2;
            data_size -= DRecommandMaxRTPPayloadLength - 2;
            fu_header &= ~(1 << 7);
        }

        fu_header |= 1 << 6;
        mpRTPBuffer[4] = (RTP_VERSION << 6);
        mpRTPBuffer[5] = (RTP_PT_H264 & 0x7f) | 0x80;//m and payload type
        mpRTPBuffer[16] = indicator;
        mpRTPBuffer[17] = fu_header;

        mpRTPBuffer[2] = ((data_size + 14) >> 8) & 0xff;
        mpRTPBuffer[3] = (data_size + 14) & 0xff;

        memcpy(&mpRTPBuffer[18], p_data, data_size);
        tcpSendData(mpRTPBuffer, data_size + 18, pts, 1, key_frame);
    }

    return EECode_OK;
}

EECode CVideoRTPRTCPSink::AddSubSession(SSubSessionInfo *p_sub_session)
{
    //AUTO_LOCK(mpMutex);

    if (DLikely(p_sub_session)) {
        memset(&p_sub_session->statistics, 0, sizeof(p_sub_session->statistics));
        p_sub_session->wait_first_key_frame = 1;
        memset(&p_sub_session->rtcp_stat, 0, sizeof(p_sub_session->rtcp_stat));
        p_sub_session->rtcp_stat.first_packet = 1;
        p_sub_session->is_started = 0;
        p_sub_session->is_closed = 0;

        if (!p_sub_session->rtp_over_rtsp) {
            LOGM_INFO("CVideoRTPRTCPSink::AddSubSession(), udp mode: addr 0x%08x, socket 0x%x, rtp port %u, rtcp port %u.\n", p_sub_session->addr, p_sub_session->socket, p_sub_session->client_rtp_port, p_sub_session->client_rtcp_port);
            mSubSessionList.InsertContent(NULL, (void *)p_sub_session, 0);
        } else {
            LOGM_INFO("CVideoRTPRTCPSink::AddSubSession(), tcp mode, over rtsp: addr 0x%08x, rtsp_fd %d.\n", p_sub_session->addr, p_sub_session->rtsp_fd);
#if 0
            if (mbUsePulseTransfer && mpTCPPulseSender) {
                p_sub_session->p_transfer_channel = mpTCPPulseSender->AddClient(p_sub_session->rtsp_fd, mpConfig->network_config.mPulseTransferMemSize, mpConfig->network_config.mPulseTransferMaxFramecount);
                if (DLikely(p_sub_session->p_transfer_channel)) {
                    TInt flags = fcntl(p_sub_session->rtsp_fd, F_GETFL, 0);
                    if (!(flags & O_NONBLOCK)) {
                        fcntl(p_sub_session->rtsp_fd, F_SETFL, flags | O_NONBLOCK);
                    }
                }
            }
#endif
            mTCPSubSessionList.InsertContent(NULL, (void *)p_sub_session, 0);
        }

        return EECode_OK;
    }
    LOGM_FATAL("NULL pointer in CVideoRTPRTCPSink::AddSubSession.\n");
    return EECode_BadParam;
}

EECode CVideoRTPRTCPSink::RemoveSubSession(SSubSessionInfo *p_sub_session)
{
    //AUTO_LOCK(mpMutex);
    if (DLikely(p_sub_session)) {
        //send RTCP:BYE to client before remove session
        if (p_sub_session->is_started) {
            processBYE(p_sub_session);
        }
        if (!p_sub_session->rtp_over_rtsp) {
            LOGM_INFO("CVideoRTPRTCPSink::RemoveSubSession(), udp mode: addr 0x%08x, socket 0x%x, rtp port %u, rtcp port %u.\n", p_sub_session->addr, p_sub_session->socket, p_sub_session->client_rtp_port, p_sub_session->client_rtcp_port);
            mSubSessionList.RemoveContent((void *)p_sub_session);
        } else {
            LOGM_INFO("CVideoRTPRTCPSink::RemoveSubSession(), tcp mode, over rtsp: addr 0x%08x, rtsp_fd %d.\n", p_sub_session->addr, p_sub_session->rtsp_fd);
            //if (p_sub_session->p_transfer_channel) {
            //    mpTCPPulseSender->RemoveClient(p_sub_session->p_transfer_channel);
            //}
            mTCPSubSessionList.RemoveContent((void *)p_sub_session);
        }
        return EECode_OK;
    }
    LOGM_FATAL("NULL pointer in CVideoRTPRTCPSink::RemoveSubSession.\n");
    return EECode_BadParam;
}

EECode CVideoRTPRTCPSink::SetSrcPort(TSocketPort port, TSocketPort port_ext)
{
    //AUTO_LOCK(mpMutex);

    if (!mbRTPSocketSetup) {
        LOGM_INFO("CVideoRTPRTCPSink::SetSrcPort[video](%hu, %hu).\n", port, port_ext);
        mbRTPSocketSetup = 1;
        mRTPPort = port;
        mRTCPPort = port_ext;

        setupUDPSocket();
        return EECode_OK;
    } else {
        LOGM_INFO("RTP socket(video) has been setup, port %hu, %hu.\n", mRTPPort, mRTCPPort);
    }

    return EECode_Error;
}

EECode CVideoRTPRTCPSink::GetSrcPort(TSocketPort &port, TSocketPort &port_ext)
{
    //AUTO_LOCK(mpMutex);
    //DASSERT(StreamType_Video == mType);

    port = mRTPPort;
    port_ext = mRTCPPort;
    LOGM_INFO("CVideoRTPRTCPSink::GetSrcPort(%hu, %hu).\n", port, port_ext);

    return EECode_OK;
}

EECode CVideoRTPRTCPSink::setupUDPSocket()
{
    //DASSERT(mType == StreamType_Video);

    if (DIsSocketHandlerValid(mRTPSocket)) {
        LOGM_WARN("close previous socket here, %d.\n", mRTPSocket);
        gfNetCloseSocket(mRTPSocket);
        mRTPSocket = DInvalidSocketHandler;
    }

    if (DIsSocketHandlerValid(mRTCPSocket)) {
        LOGM_WARN("close previous socket here, %d.\n", mRTCPSocket);
        gfNetCloseSocket(mRTCPSocket);
        mRTCPSocket = DInvalidSocketHandler;
    }

    LOGM_INFO(" before SetupDatagramSocket, port %d.\n", mRTPPort);
    mRTPSocket = gfNet_SetupDatagramSocket(INADDR_ANY, mRTPPort, 0, 0, 0);
    LOGM_INFO(" mRTPSocket %d.\n", mRTPSocket);
    if (!DIsSocketHandlerValid(mRTPSocket)) {
        //need change port and retry?
        LOGM_FATAL("SetupDatagramSocket(RTP) fail, port %d, socket %d, need change port and retry, todo?\n", mRTPPort, mRTPSocket);
        return EECode_Error;
    }

    EECode err = gfSocketSetSendBufferSize(mRTPSocket, 32768);
    if (DUnlikely(EECode_OK != err)) {
        LOGM_FATAL("gfSocketSetSendBufferSize fail\n");
        return EECode_Error;
    }

    LOGM_INFO(" before SetupDatagramSocket, port %d.\n", mRTCPPort);
    mRTCPSocket = gfNet_SetupDatagramSocket(INADDR_ANY, mRTCPPort, 0, 0, 0);
    LOGM_INFO(" mRTCPSocket %d.\n", mRTCPSocket);
    if (!DIsSocketHandlerValid(mRTCPSocket)) {
        //need change port and retry?
        LOGM_FATAL("SetupDatagramSocket(RTCP) fail, port %d, socket %d, need change port and retry, todo?\n", mRTCPPort, mRTCPSocket);
        return EECode_Error;
    }

    err = gfSocketSetSendBufferSize(mRTCPSocket, 4096);
    if (DUnlikely(EECode_OK != err)) {
        LOGM_FATAL("gfSocketSetSendBufferSize fail\n");
        return EECode_Error;
    }

    return EECode_OK;
}

#if 0
typedef struct AVRational {
    int num; ///< numerator
    int den; ///< denominator
} AVRational;

static TS64 my_av_rescale_q(TS64 a, AVRational bq, AVRational cq)
{
    TS64 b = bq.num * (TS64)cq.den;
    TS64 c = cq.num * (TS64)bq.den;
    return my_av_rescale(a, b, c);
}
#endif

int CVideoRTPRTCPSink::updateSendRtcp(SSubSessionInfo *sub_session, TU32 timestamp, TU32 len)
{
    TTime ntp_time = gfGetNTPTime();

    rtcp_stat_t *s = &sub_session->rtcp_stat;
    TS32 rtcp_bytes = (s->octet_count - s->last_octet_count) * 5 / 1000;
    if (s->first_packet || ((rtcp_bytes >= 28) &&
                            (ntp_time - s->last_rtcp_ntp_time > 5000000) && (s->timestamp != timestamp))) {

        if (s->first_packet) {
            s->first_packet = 0;
        }

        TU32 rtp_ts = timestamp;
        buildSr(sub_session->rtp_ssrc, ntp_time, rtp_ts, s->packet_count, s->octet_count);
        udpSendSr(sub_session);

        //AMLOG_WARN("updateSendRtcp---- is_video = %d\n",is_video);
        s->last_octet_count = s->octet_count;
        s->last_rtcp_ntp_time = ntp_time;
        s->timestamp = timestamp;
    }
    return 0;
}

void CVideoRTPRTCPSink::processRTCP(SSubSessionInfo *sub_session, TTime ntp_time, TU32 timestamp)
{
    DASSERT(sub_session);

    if (DUnlikely(!mbSrBuilt)) {
        mbSrBuilt = 1;
        buildSr(sub_session->rtp_ssrc, ntp_time, timestamp, sub_session->statistics.packet_count, sub_session->statistics.octet_count);
    } else {
        updateSr(sub_session->rtp_ssrc, ntp_time, timestamp, sub_session->statistics.packet_count, sub_session->statistics.octet_count);
    }

    udpSendSr(sub_session);
}

void CVideoRTPRTCPSink::buildSr(TUint ssrc, TS64 ntp_time, TUint timestamp, TUint packet_count, TUint octet_count)
{
    if (DUnlikely((!mpRTCPBuffer) || (mRTCPBufferTotalLength < 28))) {
        LOGM_FATAL("NULL mpRTCPBuffer, mRTCPBuffer %p, TotalLength %ld\n", mpRTCPBuffer, mRTCPBufferTotalLength);
        return;
    }

    mpRTCPBuffer[0]  = (2 << 6); //RTP_VERSION
    mpRTCPBuffer[1]  = (200); //RTCP_SR
    mpRTCPBuffer[2]  = 0;
    mpRTCPBuffer[3]  = 6; // length in words - 1
    mpRTCPBuffer[4]  = (ssrc >> 24) & 0xff;
    mpRTCPBuffer[5]  = (ssrc >> 16) & 0xff;
    mpRTCPBuffer[6]  = (ssrc >> 8) & 0xff;
    mpRTCPBuffer[7]  = (ssrc >> 0) & 0xff;
    TUint ntp_time_1 = ntp_time / 1000000;
    TUint ntp_time_2 = ((ntp_time % 1000000) << 32) / 1000000;
    mpRTCPBuffer[8]  = (ntp_time_1 >> 24) & 0xff;
    mpRTCPBuffer[9]  = (ntp_time_1 >> 16) & 0xff;
    mpRTCPBuffer[10]  = (ntp_time_1 >> 8) & 0xff;
    mpRTCPBuffer[11]  = (ntp_time_1 >> 0) & 0xff;
    mpRTCPBuffer[12]  = (ntp_time_2 >> 24) & 0xff;
    mpRTCPBuffer[13]  = (ntp_time_2 >> 16) & 0xff;
    mpRTCPBuffer[14]  = (ntp_time_2 >> 8) & 0xff;
    mpRTCPBuffer[15]  = (ntp_time_2 >> 0) & 0xff;
    mpRTCPBuffer[16]  = (timestamp >> 24) & 0xff;
    mpRTCPBuffer[17]  = (timestamp >> 16) & 0xff;
    mpRTCPBuffer[18]  = (timestamp >> 8) & 0xff;
    mpRTCPBuffer[19]  = (timestamp >> 0) & 0xff;
    mpRTCPBuffer[20]  = (packet_count >> 24) & 0xff;
    mpRTCPBuffer[21]  = (packet_count >> 16) & 0xff;
    mpRTCPBuffer[22]  = (packet_count >> 8) & 0xff;
    mpRTCPBuffer[23]  = (packet_count >> 0) & 0xff;
    mpRTCPBuffer[24]  = (octet_count >> 24) & 0xff;
    mpRTCPBuffer[25]  = (octet_count >> 16) & 0xff;
    mpRTCPBuffer[26]  = (octet_count >> 8) & 0xff;
    mpRTCPBuffer[27]  = (octet_count >> 0) & 0xff;

    mRTCPBufferLength = 28;
}

void CVideoRTPRTCPSink::updateSr(TUint ssrc, TS64 ntp_time, TUint timestamp, TUint packet_count, TUint octet_count)
{
    if (DUnlikely((!mpRTCPBuffer) || (mRTCPBufferTotalLength < 28))) {
        LOGM_FATAL("NULL mpRTCPBuffer, mRTCPBuffer %p, TotalLength %ld\n", mpRTCPBuffer, mRTCPBufferTotalLength);
        return;
    }

    mpRTCPBuffer[4]  = (ssrc >> 24) & 0xff;
    mpRTCPBuffer[5]  = (ssrc >> 16) & 0xff;
    mpRTCPBuffer[6]  = (ssrc >> 8) & 0xff;
    mpRTCPBuffer[7]  = (ssrc >> 0) & 0xff;
    TUint ntp_time_1 = ntp_time / 1000000;
    TUint ntp_time_2 = ((ntp_time % 1000000) << 32) / 1000000;
    mpRTCPBuffer[8]  = (ntp_time_1 >> 24) & 0xff;
    mpRTCPBuffer[9]  = (ntp_time_1 >> 16) & 0xff;
    mpRTCPBuffer[10]  = (ntp_time_1 >> 8) & 0xff;
    mpRTCPBuffer[11]  = (ntp_time_1 >> 0) & 0xff;
    mpRTCPBuffer[12]  = (ntp_time_2 >> 24) & 0xff;
    mpRTCPBuffer[13]  = (ntp_time_2 >> 16) & 0xff;
    mpRTCPBuffer[14]  = (ntp_time_2 >> 8) & 0xff;
    mpRTCPBuffer[15]  = (ntp_time_2 >> 0) & 0xff;
    mpRTCPBuffer[16]  = (timestamp >> 24) & 0xff;
    mpRTCPBuffer[17]  = (timestamp >> 16) & 0xff;
    mpRTCPBuffer[18]  = (timestamp >> 8) & 0xff;
    mpRTCPBuffer[19]  = (timestamp >> 0) & 0xff;
    mpRTCPBuffer[20]  = (packet_count >> 24) & 0xff;
    mpRTCPBuffer[21]  = (packet_count >> 16) & 0xff;
    mpRTCPBuffer[22]  = (packet_count >> 8) & 0xff;
    mpRTCPBuffer[23]  = (packet_count >> 0) & 0xff;
    mpRTCPBuffer[24]  = (octet_count >> 24) & 0xff;
    mpRTCPBuffer[25]  = (octet_count >> 16) & 0xff;
    mpRTCPBuffer[26]  = (octet_count >> 8) & 0xff;
    mpRTCPBuffer[27]  = (octet_count >> 0) & 0xff;

}

void CVideoRTPRTCPSink::buildBye(TUint ssrc)
{
    if (DUnlikely((!mpRTCPBuffer) || (mRTCPBufferTotalLength < 28))) {
        LOGM_FATAL("NULL mpRTCPBuffer, mRTCPBuffer %p, TotalLength %ld\n", mpRTCPBuffer, mRTCPBufferTotalLength);
        return;
    }

    mpRTCPBuffer[0] = (2 << 6); //RTP_VERSION
    mpRTCPBuffer[0] = (mpRTCPBuffer[0] & 0xe0) | 0x1;//ssrc count
    mpRTCPBuffer[1] = 203;//BYE
    mpRTCPBuffer[2] = 0;
    mpRTCPBuffer[3] = 1;
    mpRTCPBuffer[4]  = (ssrc >> 24) & 0xff;
    mpRTCPBuffer[5]  = (ssrc >> 16) & 0xff;
    mpRTCPBuffer[6]  = (ssrc >> 8) & 0xff;
    mpRTCPBuffer[7]  = (ssrc >> 0) & 0xff;

    mRTCPBufferLength = 8;
}

void CVideoRTPRTCPSink::processBYE(SSubSessionInfo *sub_session)
{
    buildBye(sub_session->rtp_ssrc);
    if (!sub_session->rtp_over_rtsp) {
        udpSendSr(sub_session);
    } else {
        //rtp over rtsp
    }
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

#endif

