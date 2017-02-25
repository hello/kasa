/*******************************************************************************
 * rtp_rtcp_sink_audio_v2.cpp
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

#include "common_network_utils.h"

#include "media_mw_if.h"
#include "media_mw_utils.h"

#include "framework_interface.h"
#include "mw_internal.h"
#include "dsp_platform_interface.h"
#include "streaming_if.h"
#include "modules_interface.h"

#include "rtp_rtcp_sink_audio_v2.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

IStreamingTransmiter *gfCreateAudioRtpRtcpTransmiterV2(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink)
{
    return CAudioRTPRTCPSinkV2::Create(pname, pPersistMediaConfig, pMsgSink);
}

IStreamingTransmiter *CAudioRTPRTCPSinkV2::Create(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink)
{
    CAudioRTPRTCPSinkV2 *result = new CAudioRTPRTCPSinkV2(pname, pPersistMediaConfig, pMsgSink);
    if (result && result->Construct() != EECode_OK) {
        delete result;
        result = NULL;
    }
    return result;
}

CAudioRTPRTCPSinkV2::CAudioRTPRTCPSinkV2(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink)
    : inherited(pname)
    , mFormat(StreamFormat_AAC)
    , mpClockReference(NULL)
    , mpConfig(pPersistMediaConfig)
    , mpMsgSink(pMsgSink)
    , mbRTPSocketSetup(0)
    , mPayloadType(RTP_PT_AAC)
    , mbSrBuilt(0)
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
    , mCurBufferLinearTime(0)
    , mCurBufferNativeTime(0)
{
    mpSubSessionList = NULL;
    mpTCPSubSessionList = NULL;
}

EECode CAudioRTPRTCPSinkV2::Construct()
{
    DSET_MODULE_LOG_CONFIG(ELogModuleStreamingTransmiter);

    mpSubSessionList = new CIDoubleLinkedList();
    if (DUnlikely(!mpSubSessionList)) {
        LOGM_FATAL("new CIDoubleLinkedList() fail\n");
        return EECode_NoMemory;
    }

    mpTCPSubSessionList = new CIDoubleLinkedList();
    if (DUnlikely(!mpTCPSubSessionList)) {
        LOGM_FATAL("new CIDoubleLinkedList() fail\n");
        return EECode_NoMemory;
    }

    mpRTPBuffer = (TU8 *)DDBG_MALLOC(DRecommandMaxUDPPayloadLength, "RTBF");
    if (mpRTPBuffer) {
        mRTPBufferTotalLength = DRecommandMaxUDPPayloadLength;
    } else {
        LOGM_FATAL("NO Memory.\n");
    }

    mpRTCPBuffer = (TU8 *)DDBG_MALLOC(mRTCPBufferTotalLength, "RTCB");
    if (DUnlikely(!mpRTCPBuffer)) {
        LOGM_FATAL("NO Memory.\n");
        return EECode_NoMemory;
    }

    mpClockReference = mpConfig->p_system_clock_reference;

    return EECode_OK;
}

void CAudioRTPRTCPSinkV2::Destroy()
{
    Delete();
}

void CAudioRTPRTCPSinkV2::Delete()
{
    if (mpRTPBuffer) {
        DDBG_FREE(mpRTPBuffer, "RTBF");
        mpRTPBuffer = NULL;
    }

    if (mpRTCPBuffer) {
        DDBG_FREE(mpRTCPBuffer, "RTCB");
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

    if (mpSubSessionList) {
        delete mpSubSessionList;
        mpSubSessionList = NULL;
    }

    if (mpTCPSubSessionList) {
        delete mpTCPSubSessionList;
        mpTCPSubSessionList = NULL;
    }

    inherited::Delete();
}

CAudioRTPRTCPSinkV2::~CAudioRTPRTCPSinkV2()
{

}

EECode CAudioRTPRTCPSinkV2::SetupContext(StreamTransportType type, ProtocolType protocol_type, const SStreamCodecInfos *infos, TTime start_time)
{
    return EECode_OK;
}

EECode CAudioRTPRTCPSinkV2::DestroyContext()
{
    return EECode_OK;
}

EECode CAudioRTPRTCPSinkV2::UpdateStreamFormat(StreamType type, StreamFormat format)
{
    DASSERT(StreamType_Audio == type);
    mFormat = format;
    if (StreamFormat_AAC == mFormat) {
        mPayloadType = RTP_PT_AAC;
    } else if (StreamFormat_PCMU == mFormat) {
        mPayloadType = RTP_PT_G711_PCMU;
    } else if (StreamFormat_PCMA == mFormat) {
        mPayloadType = RTP_PT_G711_PCMA;
    } else {
        LOGM_FATAL("BAD stream format %d\n", mFormat);
        return EECode_NotSupported;
    }
    return EECode_OK;
}

EECode CAudioRTPRTCPSinkV2::Start()
{
    return EECode_OK;
}

EECode CAudioRTPRTCPSinkV2::Stop()
{
    return EECode_OK;
}

EECode CAudioRTPRTCPSinkV2::SetExtraData(TU8 *pdata, TMemSize size)
{
    return EECode_OK;
}

EECode CAudioRTPRTCPSinkV2::SendData(CIBuffer *pBuffer)
{
    if (DUnlikely(!pBuffer)) {
        LOGM_FATAL("NULL buffer\n");
        return EECode_BadParam;
    }

    EECode err = EECode_OK;

    mCurBufferLinearTime = pBuffer->GetBufferLinearPTS();
    mCurBufferNativeTime = pBuffer->GetBufferNativePTS();

    updateTimeStamp();

    if (mpSubSessionList->NumberOfNodes()) {
        sendDataUDP(pBuffer);
        DASSERT_OK(err);
    }

    if (DLikely(mpTCPSubSessionList->NumberOfNodes())) {
        err = sendDataTCP(pBuffer);
        DASSERT_OK(err);
    }

    return err;
}

EECode CAudioRTPRTCPSinkV2::AddSubSession(SSubSessionInfo *p_sub_session)
{
    if (p_sub_session) {
        memset(&p_sub_session->statistics, 0, sizeof(p_sub_session->statistics));
        p_sub_session->wait_first_key_frame = 1;
        //memset(&p_sub_session->rtcp_stat, 0, sizeof(p_sub_session->rtcp_stat));
        //p_sub_session->rtcp_stat.first_packet = 1;
        p_sub_session->is_started = 0;
        p_sub_session->is_closed = 0;

        if (!p_sub_session->rtp_over_rtsp) {
            LOGM_INFO("AddSubSession(), udp mode: addr 0x%08x, socket 0x%x, rtp port %u, rtcp port %u.\n", p_sub_session->addr, p_sub_session->socket, p_sub_session->client_rtp_port, p_sub_session->client_rtcp_port);
            mpSubSessionList->InsertContent(NULL, (void *)p_sub_session, 0);
        } else {
            LOGM_INFO("AddSubSession(), tcp mode, over rtsp: addr 0x%08x, rtsp_fd %d.\n", p_sub_session->addr, p_sub_session->rtsp_fd);
            mpTCPSubSessionList->InsertContent(NULL, (void *)p_sub_session, 0);
        }

        return EECode_OK;
    }
    LOGM_FATAL("NULL pointer in CAudioRTPRTCPSinkV2::AddSubSession.\n");
    return EECode_BadParam;
}

EECode CAudioRTPRTCPSinkV2::RemoveSubSession(SSubSessionInfo *p_sub_session)
{
    if (p_sub_session) {
        if (p_sub_session->is_started) {
            processBYE(p_sub_session);
        }
        if (!p_sub_session->rtp_over_rtsp) {
            LOGM_INFO("RemoveSubSession(), udp mode: addr 0x%08x, socket 0x%x, rtp port %u, rtcp port %u.\n", p_sub_session->addr, p_sub_session->socket, p_sub_session->client_rtp_port, p_sub_session->client_rtcp_port);
            mpSubSessionList->RemoveContent((void *)p_sub_session);
        } else {
            LOGM_INFO("RemoveSubSession(), tcp mode, over rtsp: addr 0x%08x, rtsp_fd %d.\n", p_sub_session->addr, p_sub_session->rtsp_fd);
            mpTCPSubSessionList->RemoveContent((void *)p_sub_session);
        }
        return EECode_OK;
    }
    LOGM_FATAL("NULL pointer in CAudioRTPRTCPSinkV2::RemoveSubSession.\n");
    return EECode_BadParam;
}

EECode CAudioRTPRTCPSinkV2::SetSrcPort(TSocketPort port, TSocketPort port_ext)
{
    if (!mbRTPSocketSetup) {
        //LOGM_NOTICE("CAudioRTPRTCPSinkV2::SetSrcPort[audio](%hu, %hu).\n", port, port_ext);
        mRTPPort = port;
        mRTCPPort = port_ext;

        setupUDPSocket();
    } else {
        LOGM_WARN("RTP socket(audio) has been setup, port %hu, %hu.\n", mRTPPort, mRTCPPort);
    }

    return EECode_OK;
}

EECode CAudioRTPRTCPSinkV2::GetSrcPort(TSocketPort &port, TSocketPort &port_ext)
{
    port = mRTPPort;
    port_ext = mRTCPPort;
    LOGM_NOTICE("CAudioRTPRTCPSinkV2::GetSrcPort(%hu, %hu).\n", port, port_ext);

    return EECode_OK;
}

EECode CAudioRTPRTCPSinkV2::sendDataUDP(CIBuffer *pBuffer)
{
    if (StreamFormat_AAC == mFormat) {
        return sendDataAAC(pBuffer);
    } else if ((StreamFormat_PCMU == mFormat) || (StreamFormat_PCMA == mFormat)) {
        return sendDataPCM(pBuffer);
    } else if (StreamFormat_MPEG12Audio == mFormat) {
        return sendDataMpeg12Audio(pBuffer);
    } else {
        LOGM_FATAL("BAD stream format %d\n", mFormat);
    }

    return EECode_NotSupported;
}

EECode CAudioRTPRTCPSinkV2::sendDataTCP(CIBuffer *pBuffer)
{
    if (StreamFormat_AAC == mFormat) {
        return sendDataAACTCP(pBuffer);
    } else if ((StreamFormat_PCMU == mFormat) || (StreamFormat_PCMA == mFormat)) {
        return sendDataPCMTCP(pBuffer);
    } else if (StreamFormat_MPEG12Audio == mFormat) {
        return sendDataMpeg12AudioTCP(pBuffer);
    } else {
        LOGM_FATAL("BAD stream format %d\n", mFormat);
    }

    return EECode_NotSupported;
}

EECode CAudioRTPRTCPSinkV2::setupUDPSocket()
{
    if (0 == mbRTPSocketSetup) {
        mbRTPSocketSetup = 1;
    } else {
        LOGM_WARN("socket has been setup, please check code.\n");
        return EECode_OK;
    }

    if (DUnlikely(DIsSocketHandlerValid(mRTPSocket))) {
        LOGM_WARN("close previous socket here, %d.\n", mRTPSocket);
        gfNetCloseSocket(mRTPSocket);
        mRTPSocket = DInvalidSocketHandler;
    }

    if (DUnlikely(DIsSocketHandlerValid(mRTCPSocket))) {
        LOGM_WARN("close previous socket here, %d.\n", mRTCPSocket);
        gfNetCloseSocket(mRTCPSocket);
        mRTCPSocket = DInvalidSocketHandler;
    }

    //LOGM_NOTICE(" before SetupDatagramSocket, port %d.\n", mRTPPort);
    mRTPSocket = gfNet_SetupDatagramSocket(INADDR_ANY, mRTPPort, 0, 0, 0);
    //LOGM_NOTICE(" mRTPSocket %d.\n", mRTPSocket);
    if (DUnlikely(!DIsSocketHandlerValid(mRTPSocket))) {
        //need change port and retry?
        LOGM_FATAL("SetupDatagramSocket(RTP) fail, port %d, socket %d, need change port and retry, todo?\n", mRTPPort, mRTPSocket);
        return EECode_Error;
    }

    EECode err = gfSocketSetSendBufferSize(mRTPSocket, 32768);
    if (DUnlikely(EECode_OK != err)) {
        LOGM_FATAL("gfSocketSetSendBufferSize fail\n");
        return EECode_Error;
    }

    LOGM_NOTICE(" before SetupDatagramSocket, port %d.\n", mRTCPPort);
    mRTCPSocket = gfNet_SetupDatagramSocket(INADDR_ANY, mRTCPPort, 0, 0, 0);
    LOGM_NOTICE(" mRTCPSocket %d.\n", mRTCPSocket);
    if (DUnlikely(!DIsSocketHandlerValid(mRTCPSocket))) {
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

void CAudioRTPRTCPSinkV2::udpSendData(TU8 *p_data, TMemSize size)
{
    CIDoubleLinkedList::SNode *p_node = mpSubSessionList->FirstNode();
    SSubSessionInfo *sub_session = NULL;

    while (p_node) {
        sub_session = (SSubSessionInfo *)p_node->p_context;
        if (DLikely(sub_session)) {
            if (DUnlikely(sub_session->is_closed)) {
                LOGM_INFO("sub_session %p is_closed\n", sub_session);
                p_node = mpSubSessionList->NextNode(p_node);
                continue;
            }

            if (DUnlikely(!sub_session->is_started)) {
                if (DLikely(sub_session->parent)) {
                    if (sub_session->parent->master_index != ESubSession_audio) {
                        if (!sub_session->parent->sub_session[sub_session->parent->master_index].is_started) {
                            LOGM_INFO("master is not started\n");
                            p_node = mpSubSessionList->NextNode(p_node);
                            continue;
                        }
                    }
                    sub_session->cur_time = sub_session->start_time;
                    sub_session->is_started = 1;

                    sub_session->statistics.first_seq_num = sub_session->seq_number;
                    sub_session->statistics.cur_seq_num = sub_session->seq_number;

                    sub_session->statistics.octet_count = size;
                    sub_session->statistics.packet_count = 1;
                    sub_session->statistics.last_octet_count = size;

                    sub_session->rtcp_sr_cooldown = 384;
                    sub_session->rtcp_sr_current_count = 0;

                    sub_session->avsync_begin_native_timestamp = mCurBufferNativeTime;
                    sub_session->avsync_begin_linear_timestamp = mCurBufferLinearTime;
                    LOGM_NOTICE("begin %lld, %lld\n", mCurBufferNativeTime, mCurBufferLinearTime);

#ifdef DCONFIG_ENABLE_REALTIME_PRINT
                    if (DUnlikely(mpConfig->common_config.debug_dump.debug_print_audio_realtime)) {
                        sub_session->debug_sub_session_packet_count = 0;
                        if (sub_session->p_content) {
                            sub_session->debug_sub_session_duration_num = sub_session->p_content->audio_frame_size;
                            sub_session->debug_sub_session_duration_den = sub_session->p_content->audio_sample_rate;
                        } else {
                            sub_session->debug_sub_session_duration_num = DDefaultAudioFrameSize;
                            sub_session->debug_sub_session_duration_den = DDefaultAudioSampleRate;
                        }
                        LOGM_NOTICE("[audio print realtime config]: num %lld, den %lld\n", sub_session->debug_sub_session_duration_num, sub_session->debug_sub_session_duration_den);
                        sub_session->debug_sub_session_begin_time = mpClockReference->GetCurrentTime();
                    }
#endif

                    if (sub_session->parent->master_index != ESubSession_audio) {
                        sub_session->avsync_initial_drift = sub_session->avsync_begin_native_timestamp - sub_session->parent->sub_session[sub_session->parent->master_index].avsync_begin_native_timestamp;
                        LOGM_NOTICE("initial drift %lld (native)\n", sub_session->avsync_initial_drift);

                        sub_session->avsync_initial_drift = sub_session->avsync_initial_drift * sub_session->p_content->audio_sample_rate / DDefaultTimeScale;
                        LOGM_NOTICE("initial drift %lld (linear), audio sample rate %d\n", sub_session->avsync_initial_drift, sub_session->p_content->audio_sample_rate);
                    } else {
                        sub_session->avsync_initial_drift = 0;
                    }
                } else {
                    LOGM_FATAL("NULL sub_session->parent!\n");
                    p_node = mpSubSessionList->NextNode(p_node);
                    continue;
                }
            }

            TU32 timestamp = sub_session->cur_time;

            p_data[2] = (sub_session->seq_number >> 8) & 0xff;
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

#ifdef DCONFIG_ENABLE_REALTIME_PRINT
            if (DUnlikely(mpConfig->common_config.debug_dump.debug_print_audio_realtime)) {
                static TTime last_time = 0;
                TTime cur_time = mpClockReference->GetCurrentTime();
                TTime play_time = sub_session->debug_sub_session_packet_count * 1000000 * sub_session->debug_sub_session_duration_num / sub_session->debug_sub_session_duration_den;
                LOGM_PRINTF("audio time stamp %lld, real time stamp last frame gap %lld, longterm gap %lld\n", sub_session->cur_time, cur_time - last_time, play_time + sub_session->debug_sub_session_begin_time - cur_time);
                last_time = cur_time;
                sub_session->debug_sub_session_packet_count ++;
            }
#endif

            TInt ret = gfNet_SendTo(mRTPSocket, p_data, size, 0, (const void *)&sub_session->addr_port, (TSocketSize)sizeof(struct sockaddr_in));
            if (DUnlikely(ret < 0)) {

                if (!sub_session->is_closed) {
                    sub_session->is_closed = 1;
                    LOGM_NOTICE("send EMSGType_StreamingError_UDPSocketInvalidArgument msg, sub_session %p\n", sub_session);
                    SMSG msg;
                    memset(&msg, 0x0, sizeof(msg));

                    msg.code = EMSGType_StreamingError_UDPSocketInvalidArgument;
                    msg.p0 = (TULong)sub_session->parent;
                    msg.p1 = (TULong)sub_session->parent->p_streaming_transmiter_filter;
                    msg.p2 = (TULong)sub_session->parent->p_server;
                    mpMsgSink->MsgProc(msg);
                }

                p_node = mpSubSessionList->NextNode(p_node);
                continue;
            }

            if (DUnlikely(!sub_session->is_started)) {
                sub_session->is_started = 1;
                //sub_session->ntp_time = sub_session->ntp_time_base = gfGetNTPTime();
                //LOGM_INFO("audio ntp time:\t %lld\n", sub_session->ntp_time);
                //processRTCP(sub_session, sub_session->ntp_time, (TU32)sub_session->start_time);
                sub_session->rtcp_sr_current_count = 0;
            } else {
                sub_session->statistics.cur_seq_num = sub_session->seq_number;
                sub_session->statistics.last_octet_count = size;

                sub_session->statistics.octet_count += size;
                sub_session->statistics.packet_count ++;

                sub_session->rtcp_sr_current_count ++;

                if (DUnlikely(sub_session->rtcp_sr_current_count > sub_session->rtcp_sr_cooldown)) {
                    sub_session->ntp_time = gfGetNTPTime();
                    processRTCP(sub_session, sub_session->ntp_time, sub_session->cur_time);
                    sub_session->rtcp_sr_current_count = 0;
                }
            }

        }
        p_node = mpSubSessionList->NextNode(p_node);
    }

}

void CAudioRTPRTCPSinkV2::tcpSendData(TU8 *p_data, TMemSize size)
{
    CIDoubleLinkedList::SNode *p_node = mpTCPSubSessionList->FirstNode();
    SSubSessionInfo *sub_session = NULL;

    while (p_node) {
        sub_session = (SSubSessionInfo *)p_node->p_context;
        if (DLikely(sub_session)) {
            if (DUnlikely(sub_session->is_closed)) {
                LOGM_INFO("sub_session %p is_closed\n", sub_session);
                p_node = mpTCPSubSessionList->NextNode(p_node);
                continue;
            }

            if (DUnlikely(!sub_session->is_started)) {
                if (DLikely(sub_session->parent)) {
                    if (sub_session->parent->master_index != ESubSession_audio) {
                        if (!sub_session->parent->sub_session[sub_session->parent->master_index].is_started) {
                            LOGM_INFO("master is not started, index %d\n", sub_session->parent->master_index);
                            p_node = mpTCPSubSessionList->NextNode(p_node);
                            continue;
                        }
                    }
                    sub_session->cur_time = sub_session->start_time;
                    sub_session->is_started = 1;

                    sub_session->statistics.first_seq_num = sub_session->seq_number;
                    sub_session->statistics.cur_seq_num = sub_session->seq_number;

                    sub_session->statistics.octet_count = size;
                    sub_session->statistics.packet_count = 1;
                    sub_session->statistics.last_octet_count = size;

                    sub_session->rtcp_sr_cooldown = 384;
                    sub_session->rtcp_sr_current_count = 0;

                    sub_session->avsync_begin_native_timestamp = mCurBufferNativeTime;
                    sub_session->avsync_begin_linear_timestamp = mCurBufferLinearTime;
                    LOGM_NOTICE("begin %lld, %lld\n", mCurBufferNativeTime, mCurBufferLinearTime);

#ifdef DCONFIG_ENABLE_REALTIME_PRINT
                    if (DUnlikely(mpConfig->common_config.debug_dump.debug_print_audio_realtime)) {
                        sub_session->debug_sub_session_packet_count = 0;
                        if (sub_session->p_content) {
                            sub_session->debug_sub_session_duration_num = sub_session->p_content->audio_frame_size;
                            sub_session->debug_sub_session_duration_den = sub_session->p_content->audio_sample_rate;
                        } else {
                            sub_session->debug_sub_session_duration_num = DDefaultAudioFrameSize;
                            sub_session->debug_sub_session_duration_den = DDefaultAudioSampleRate;
                        }
                        LOGM_NOTICE("[audio print realtime config]: num %lld, den %lld\n", sub_session->debug_sub_session_duration_num, sub_session->debug_sub_session_duration_den);
                        sub_session->debug_sub_session_begin_time = mpClockReference->GetCurrentTime();
                    }
#endif

                    if (sub_session->parent->master_index != ESubSession_audio) {
                        sub_session->avsync_initial_drift = sub_session->avsync_begin_native_timestamp - sub_session->parent->sub_session[sub_session->parent->master_index].avsync_begin_native_timestamp;
                        LOGM_NOTICE("initial drift %lld (native)\n", sub_session->avsync_initial_drift);

                        sub_session->avsync_initial_drift = sub_session->avsync_initial_drift * sub_session->p_content->audio_sample_rate / DDefaultTimeScale;
                        LOGM_NOTICE("initial drift %lld (linear), audio sample rate %d\n", sub_session->avsync_initial_drift, sub_session->p_content->audio_sample_rate);
                    } else {
                        sub_session->avsync_initial_drift = 0;
                    }
                } else {
                    LOGM_FATAL("NULL sub_session->parent!\n");
                    p_node = mpTCPSubSessionList->NextNode(p_node);
                    continue;
                }
            }

            DASSERT(sub_session->rtp_over_rtsp);
            p_data[1] = sub_session->rtp_channel;

            TU32 timestamp = sub_session->cur_time;

            p_data[6] = (sub_session->seq_number >> 8) & 0xff;
            p_data[7] = sub_session->seq_number & 0xff;
            p_data[8] = (timestamp >> 24) & 0xff;
            p_data[9] = (timestamp >> 16) & 0xff;
            p_data[10] = (timestamp >> 8) & 0xff;
            p_data[11] = timestamp & 0xff;
            TU32 ssrc = sub_session->rtp_ssrc;
            p_data[12] = (ssrc >> 24) & 0xff;
            p_data[13] = (ssrc >> 16) & 0xff;
            p_data[14] = (ssrc >> 8) & 0xff;
            p_data[15] = ssrc & 0xff;
            sub_session->seq_number ++;

#ifdef DCONFIG_ENABLE_REALTIME_PRINT
            if (DUnlikely(mpConfig->common_config.debug_dump.debug_print_audio_realtime)) {
                static TTime last_time = 0;
                TTime cur_time = mpClockReference->GetCurrentTime();
                TTime play_time = sub_session->debug_sub_session_packet_count * 1000000 * sub_session->debug_sub_session_duration_num / sub_session->debug_sub_session_duration_den;
                LOGM_PRINTF("audio time stamp %lld, real time stamp last frame gap %lld, longterm gap %lld\n", sub_session->cur_time, cur_time - last_time, play_time + sub_session->debug_sub_session_begin_time - cur_time);
                last_time = cur_time;
                sub_session->debug_sub_session_packet_count ++;
            }
#endif

            TInt ret = gfNet_Send_timeout(sub_session->rtsp_fd, p_data, size, 0, 3);
            if (DUnlikely(ret != ((TInt) size))) {

                if (!sub_session->is_closed) {
                    sub_session->is_closed = 1;
                    LOGM_NOTICE("send EMSGType_StreamingError_TCPSocketConnectionClose msg, sub_session %p\n", sub_session);
                    SMSG msg;
                    memset(&msg, 0x0, sizeof(msg));

                    msg.code = EMSGType_StreamingError_TCPSocketConnectionClose;
                    msg.p0 = (TULong)sub_session->parent;
                    msg.p1 = (TULong)sub_session->parent->p_streaming_transmiter_filter;
                    msg.p2 = (TULong)sub_session->parent->p_server;
                    mpMsgSink->MsgProc(msg);
                }

                p_node = mpTCPSubSessionList->NextNode(p_node);
                continue;
            }

            if (DUnlikely(!sub_session->is_started)) {
                sub_session->is_started = 1;
                //sub_session->ntp_time = sub_session->ntp_time_base = gfGetNTPTime();
                //LOGM_INFO("audio ntp time:\t %lld\n", sub_session->ntp_time);
                //processRTCP(sub_session, sub_session->ntp_time, (TU32)sub_session->start_time);
                sub_session->rtcp_sr_current_count = 0;
            } else {
                sub_session->statistics.cur_seq_num = sub_session->seq_number;
                sub_session->statistics.last_octet_count = size;

                sub_session->statistics.octet_count += size;
                sub_session->statistics.packet_count ++;

                sub_session->rtcp_sr_current_count ++;

                if (DUnlikely(sub_session->rtcp_sr_current_count > sub_session->rtcp_sr_cooldown)) {
                    sub_session->ntp_time = gfGetNTPTime();
                    processRTCP(sub_session, sub_session->ntp_time, sub_session->cur_time);
                    sub_session->rtcp_sr_current_count = 0;
                }
            }

        }
        p_node = mpTCPSubSessionList->NextNode(p_node);
    }

}

EECode CAudioRTPRTCPSinkV2::sendDataAAC(CIBuffer *pBuffer)
{
    TMemSize data_size = 0;
    TU8 *p_data = NULL;

    data_size = pBuffer->GetDataSize();
    p_data = pBuffer->GetDataPtr();

    if ((0xff == p_data[0]) && (0xf0 == (p_data[1] & 0xf0))) {
        p_data += 7;
        data_size -= 7;
    }

    if (DLikely(data_size <= DRecommandMaxRTPPayloadLength)) {
        mpRTPBuffer[0] = (RTP_VERSION << 6);
        mpRTPBuffer[1] = (mPayloadType & 0x7f) | 0x80;

        if (pBuffer->GetBufferFlags() & CIBuffer::RTP_AAC) {
            memcpy(mpRTPBuffer + 12, p_data, data_size);
            udpSendData(mpRTPBuffer, data_size + 12);
        } else {
            mpRTPBuffer[12] = 0x00;
            mpRTPBuffer[13] = 0x10;
            mpRTPBuffer[14] = (data_size & 0x1fe0) >> 5;
            mpRTPBuffer[15] = (data_size & 0x1f) << 3;
            memcpy(mpRTPBuffer + 16, p_data, data_size);
            udpSendData(mpRTPBuffer, data_size + 16);
        }
    } else {
        LOGM_ERROR("should not comes here\n");
        return EECode_InternalLogicalBug;
    }

    return EECode_OK;
}

EECode CAudioRTPRTCPSinkV2::sendDataAACTCP(CIBuffer *pBuffer)
{
    TMemSize data_size = 0;
    TU8 *p_data = NULL;

    data_size = pBuffer->GetDataSize();
    p_data = pBuffer->GetDataPtr();

    //LOGM_NOTICE("sendDataAACTCP, size %d\n", data_size);
    //gfPrintMemory(p_data, data_size);
    if ((0xff == p_data[0]) && (0xf0 == (p_data[1] & 0xf0))) {
        p_data += 7;
        data_size -= 7;
    }

    mpRTPBuffer[0] = DRTP_OVER_RTSP_MAGIC;
    mpRTPBuffer[1] = 0;

    if (DLikely(data_size <= DRecommandMaxRTPPayloadLength)) {
        mpRTPBuffer[4] = (RTP_VERSION << 6);
        mpRTPBuffer[5] = (mPayloadType & 0x7f) | 0x80;

        if (pBuffer->GetBufferFlags() & CIBuffer::RTP_AAC) {
            LOGM_NOTICE("why here\n");
            mpRTPBuffer[2] = ((data_size + 12) >> 8) & 0xff;
            mpRTPBuffer[3] = (data_size + 12) & 0xff;
            memcpy(mpRTPBuffer + 12, p_data, data_size);
            tcpSendData(mpRTPBuffer, data_size + 12 + 4);
        } else {
            mpRTPBuffer[2] = ((data_size + 16) >> 8) & 0xff;
            mpRTPBuffer[3] = (data_size + 16) & 0xff;

            mpRTPBuffer[16] = 0x00;
            mpRTPBuffer[17] = 0x10;
            mpRTPBuffer[18] = (data_size & 0x1fe0) >> 5;
            mpRTPBuffer[19] = (data_size & 0x1f) << 3;
            memcpy(mpRTPBuffer + 16 + 4, p_data, data_size);
            tcpSendData(mpRTPBuffer, data_size + 16 + 4);
        }
    } else {
        LOGM_ERROR("should not comes here\n");
        return EECode_InternalLogicalBug;
    }

    return EECode_OK;
}

EECode CAudioRTPRTCPSinkV2::sendDataPCM(CIBuffer *pBuffer)
{
    TMemSize data_size = 0;
    TU8 *p_data = NULL;

    data_size = pBuffer->GetDataSize();
    p_data = pBuffer->GetDataPtr();

    if (DLikely(data_size <= DRecommandMaxRTPPayloadLength)) {
        mpRTPBuffer[0] = (RTP_VERSION << 6);
        mpRTPBuffer[1] = (mPayloadType & 0x7f) | 0x80;

        memcpy(mpRTPBuffer + 12, p_data, data_size);

        udpSendData(mpRTPBuffer, data_size + 12);
    } else {
        LOGM_ERROR("should not comes here\n");
        return EECode_InternalLogicalBug;
    }

    return EECode_OK;
}

EECode CAudioRTPRTCPSinkV2::sendDataPCMTCP(CIBuffer *pBuffer)
{
    TMemSize data_size = 0;
    TU8 *p_data = NULL;

    data_size = pBuffer->GetDataSize();
    p_data = pBuffer->GetDataPtr();

    mpRTPBuffer[0] = DRTP_OVER_RTSP_MAGIC;
    mpRTPBuffer[1] = 0;

    if (DLikely(data_size <= DRecommandMaxRTPPayloadLength)) {
        mpRTPBuffer[4] = (RTP_VERSION << 6);
        mpRTPBuffer[5] = (mPayloadType & 0x7f) | 0x80;

        mpRTPBuffer[2] = ((data_size + 12) >> 8) & 0xff;
        mpRTPBuffer[3] = (data_size + 12) & 0xff;

        memcpy(mpRTPBuffer + 16, p_data, data_size);

        tcpSendData(mpRTPBuffer, data_size + 16);
    } else {
        LOGM_ERROR("should not comes here\n");
        return EECode_InternalLogicalBug;
    }

    return EECode_OK;
}

EECode CAudioRTPRTCPSinkV2::sendDataMpeg12Audio(CIBuffer *pBuffer)
{
    TMemSize data_size = 0;
    TU8 *p_data = NULL;

    data_size = pBuffer->GetDataSize();
    p_data = pBuffer->GetDataPtr();

    if (DLikely(data_size <= DRecommandMaxRTPPayloadLength)) {
        mpRTPBuffer[0] = (RTP_VERSION << 6);
        mpRTPBuffer[1] = (mPayloadType & 0x7f) | 0x80;

        mpRTPBuffer[12] = 0x00;
        mpRTPBuffer[13] = 0x00;
        mpRTPBuffer[14] = 0;
        mpRTPBuffer[15] = 0;

        memcpy(mpRTPBuffer + 16, p_data, data_size);

        udpSendData(mpRTPBuffer, data_size + 16);
    } else {
        LOGM_ERROR("should not comes here\n");
        return EECode_InternalLogicalBug;
    }

    return EECode_OK;
}

EECode CAudioRTPRTCPSinkV2::sendDataMpeg12AudioTCP(CIBuffer *pBuffer)
{
    TMemSize data_size = 0;
    TU8 *p_data = NULL;

    data_size = pBuffer->GetDataSize();
    p_data = pBuffer->GetDataPtr();

    mpRTPBuffer[0] = DRTP_OVER_RTSP_MAGIC;
    mpRTPBuffer[1] = 0;

    if (DLikely(data_size <= DRecommandMaxRTPPayloadLength)) {
        mpRTPBuffer[4] = (RTP_VERSION << 6);
        mpRTPBuffer[5] = (mPayloadType & 0x7f) | 0x80;

        mpRTPBuffer[2] = ((data_size + 12) >> 8) & 0xff;
        mpRTPBuffer[3] = (data_size + 12) & 0xff;

        mpRTPBuffer[16] = 0x00;
        mpRTPBuffer[17] = 0x00;
        mpRTPBuffer[18] = 0;
        mpRTPBuffer[19] = 0;

        memcpy(mpRTPBuffer + 20, p_data, data_size);

        tcpSendData(mpRTPBuffer, data_size + 20);
    } else {
        LOGM_ERROR("should not comes here\n");
        return EECode_InternalLogicalBug;
    }

    return EECode_OK;
}

void CAudioRTPRTCPSinkV2::processRTCP(SSubSessionInfo *sub_session, TTime ntp_time, TU32 timestamp)
{
    DASSERT(sub_session);

    if (DUnlikely(!mbSrBuilt)) {
        mbSrBuilt = 1;
        buildSr(sub_session->rtp_ssrc, ntp_time, timestamp, sub_session->statistics.packet_count, sub_session->statistics.octet_count);
    } else {
        updateSr(sub_session->rtp_ssrc, ntp_time, timestamp, sub_session->statistics.packet_count, sub_session->statistics.octet_count);
    }

    if (DUnlikely(!sub_session->rtp_over_rtsp)) {
        udpSendSr(sub_session);
    } else {
        tcpSendSr(sub_session);
    }
}

void CAudioRTPRTCPSinkV2::udpSendSr(SSubSessionInfo *sub_session)
{
    DASSERT(sub_session);

    TInt ret = gfNet_SendTo(mRTCPSocket, mpRTCPBuffer, mRTCPBufferLength, 0, (const void *)&sub_session->addr_port, (TSocketSize)sizeof(struct sockaddr_in));
    if (DUnlikely(ret < 0)) {
        LOGM_ERROR("udpSendSr failed, ret %d\n", ret);
    }
}

void CAudioRTPRTCPSinkV2::tcpSendSr(SSubSessionInfo *sub_session)
{
    mpRTCPBuffer[0] = DRTP_OVER_RTSP_MAGIC;
    mpRTCPBuffer[1] = sub_session->rtcp_channel;
    mpRTCPBuffer[2] = (mRTCPBufferLength >> 8) & 0xff;
    mpRTCPBuffer[3] = mRTCPBufferLength & 0xff;

    TInt ret = gfNet_Send_timeout(sub_session->rtsp_fd, mpRTCPBuffer, mRTCPBufferLength + 4, 0, 6);
    if (DUnlikely(ret < 0)) {
        LOGM_ERROR("tcpSendSr failed, ret %d\n", ret);
    }
}

void CAudioRTPRTCPSinkV2::buildSr(TUint ssrc, TS64 ntp_time, TUint timestamp, TUint packet_count, TUint octet_count)
{
    if (DUnlikely(!mpRTCPBuffer)) {
        LOGM_FATAL("NULL mpRTCPBuffer\n");
        return;
    }

    mpRTCPBuffer[4]  = (2 << 6); //RTP_VERSION
    mpRTCPBuffer[5]  = ERTCPType_SR; //RTCP_SR
    mpRTCPBuffer[6]  = 0;
    mpRTCPBuffer[7]  = 6; // length in words - 1
    mpRTCPBuffer[8]  = (ssrc >> 24) & 0xff;
    mpRTCPBuffer[9]  = (ssrc >> 16) & 0xff;
    mpRTCPBuffer[10]  = (ssrc >> 8) & 0xff;
    mpRTCPBuffer[11]  = (ssrc >> 0) & 0xff;
    TUint ntp_time_1 = ntp_time / 1000000;
    TUint ntp_time_2 = ((ntp_time % 1000000) << 32) / 1000000;
    mpRTCPBuffer[12]  = (ntp_time_1 >> 24) & 0xff;
    mpRTCPBuffer[13]  = (ntp_time_1 >> 16) & 0xff;
    mpRTCPBuffer[14]  = (ntp_time_1 >> 8) & 0xff;
    mpRTCPBuffer[15]  = (ntp_time_1 >> 0) & 0xff;
    mpRTCPBuffer[16]  = (ntp_time_2 >> 24) & 0xff;
    mpRTCPBuffer[17]  = (ntp_time_2 >> 16) & 0xff;
    mpRTCPBuffer[18]  = (ntp_time_2 >> 8) & 0xff;
    mpRTCPBuffer[19]  = (ntp_time_2 >> 0) & 0xff;
    mpRTCPBuffer[20]  = (timestamp >> 24) & 0xff;
    mpRTCPBuffer[21]  = (timestamp >> 16) & 0xff;
    mpRTCPBuffer[22]  = (timestamp >> 8) & 0xff;
    mpRTCPBuffer[23]  = (timestamp >> 0) & 0xff;
    mpRTCPBuffer[24]  = (packet_count >> 24) & 0xff;
    mpRTCPBuffer[25]  = (packet_count >> 16) & 0xff;
    mpRTCPBuffer[26]  = (packet_count >> 8) & 0xff;
    mpRTCPBuffer[27]  = (packet_count >> 0) & 0xff;
    mpRTCPBuffer[28]  = (octet_count >> 24) & 0xff;
    mpRTCPBuffer[29]  = (octet_count >> 16) & 0xff;
    mpRTCPBuffer[30]  = (octet_count >> 8) & 0xff;
    mpRTCPBuffer[31]  = (octet_count >> 0) & 0xff;

    mRTCPBufferLength = 28;
}

void CAudioRTPRTCPSinkV2::updateSr(TUint ssrc, TS64 ntp_time, TUint timestamp, TUint packet_count, TUint octet_count)
{
    if (DUnlikely(!mpRTCPBuffer)) {
        LOGM_FATAL("NULL mpRTCPBuffer\n");
        return;
    }

    mpRTCPBuffer[4]  = (2 << 6); //RTP_VERSION
    mpRTCPBuffer[5]  = ERTCPType_SR; //RTCP_SR
    mpRTCPBuffer[6]  = 0;
    mpRTCPBuffer[7]  = 6; // length in words - 1

    mpRTCPBuffer[8]  = (ssrc >> 24) & 0xff;
    mpRTCPBuffer[9]  = (ssrc >> 16) & 0xff;
    mpRTCPBuffer[10]  = (ssrc >> 8) & 0xff;
    mpRTCPBuffer[11]  = (ssrc >> 0) & 0xff;
    TUint ntp_time_1 = ntp_time / 1000000;
    TUint ntp_time_2 = ((ntp_time % 1000000) << 32) / 1000000;
    mpRTCPBuffer[12]  = (ntp_time_1 >> 24) & 0xff;
    mpRTCPBuffer[13]  = (ntp_time_1 >> 16) & 0xff;
    mpRTCPBuffer[14]  = (ntp_time_1 >> 8) & 0xff;
    mpRTCPBuffer[15]  = (ntp_time_1 >> 0) & 0xff;
    mpRTCPBuffer[16]  = (ntp_time_2 >> 24) & 0xff;
    mpRTCPBuffer[17]  = (ntp_time_2 >> 16) & 0xff;
    mpRTCPBuffer[18]  = (ntp_time_2 >> 8) & 0xff;
    mpRTCPBuffer[19]  = (ntp_time_2 >> 0) & 0xff;
    mpRTCPBuffer[20]  = (timestamp >> 24) & 0xff;
    mpRTCPBuffer[21]  = (timestamp >> 16) & 0xff;
    mpRTCPBuffer[22]  = (timestamp >> 8) & 0xff;
    mpRTCPBuffer[23]  = (timestamp >> 0) & 0xff;
    mpRTCPBuffer[24]  = (packet_count >> 24) & 0xff;
    mpRTCPBuffer[25]  = (packet_count >> 16) & 0xff;
    mpRTCPBuffer[26]  = (packet_count >> 8) & 0xff;
    mpRTCPBuffer[27]  = (packet_count >> 0) & 0xff;
    mpRTCPBuffer[28]  = (octet_count >> 24) & 0xff;
    mpRTCPBuffer[29]  = (octet_count >> 16) & 0xff;
    mpRTCPBuffer[30]  = (octet_count >> 8) & 0xff;
    mpRTCPBuffer[31]  = (octet_count >> 0) & 0xff;

    mRTCPBufferLength = 28;
}

void CAudioRTPRTCPSinkV2::buildBye(TUint ssrc)
{
    if (DUnlikely(!mpRTCPBuffer)) {
        LOGM_FATAL("NULL mpRTCPBuffer\n");
        return;
    }

    mpRTCPBuffer[4] = (2 << 6); //RTP_VERSION
    mpRTCPBuffer[4] = (mpRTCPBuffer[4] & 0xe0) | 0x1;//ssrc count
    mpRTCPBuffer[5] = ERTCPType_BYE;//BYE
    mpRTCPBuffer[6] = 0;
    mpRTCPBuffer[7] = 1;
    mpRTCPBuffer[8]  = (ssrc >> 24) & 0xff;
    mpRTCPBuffer[9]  = (ssrc >> 16) & 0xff;
    mpRTCPBuffer[10]  = (ssrc >> 8) & 0xff;
    mpRTCPBuffer[11]  = (ssrc >> 0) & 0xff;

    mRTCPBufferLength = 8;
}

void CAudioRTPRTCPSinkV2::processBYE(SSubSessionInfo *sub_session)
{
    buildBye(sub_session->rtp_ssrc);
    if (DUnlikely(!sub_session->rtp_over_rtsp)) {
        udpSendSr(sub_session);
    } else {
        tcpSendSr(sub_session);
    }
}

void CAudioRTPRTCPSinkV2::updateTimeStamp()
{
    CIDoubleLinkedList::SNode *p_node = mpTCPSubSessionList->FirstNode();
    SSubSessionInfo *sub_session = NULL;

    while (p_node) {
        sub_session = (SSubSessionInfo *)p_node->p_context;
        if (DLikely(sub_session)) {
            if (DUnlikely((sub_session->is_closed) || (!sub_session->is_started))) {
                p_node = mpTCPSubSessionList->NextNode(p_node);
                continue;
            }

            if (DUnlikely(sub_session->need_resync)) {
                sub_session->avsync_begin_linear_timestamp = mCurBufferLinearTime;
                sub_session->cur_time = sub_session->start_time;
                sub_session->need_resync = 0;
            } else {
                sub_session->cur_time = mCurBufferLinearTime - sub_session->avsync_begin_linear_timestamp + sub_session->start_time;
            }
        }
        p_node = mpTCPSubSessionList->NextNode(p_node);
    }

    p_node = mpSubSessionList->FirstNode();

    while (p_node) {
        sub_session = (SSubSessionInfo *)p_node->p_context;
        if (DLikely(sub_session)) {
            if (DUnlikely((sub_session->is_closed) || (!sub_session->is_started))) {
                p_node = mpSubSessionList->NextNode(p_node);
                continue;
            }

            if (DUnlikely(sub_session->need_resync)) {
                sub_session->avsync_begin_linear_timestamp = mCurBufferLinearTime;
                sub_session->cur_time = sub_session->start_time;
                sub_session->need_resync = 0;
            } else {
                sub_session->cur_time = mCurBufferLinearTime - sub_session->avsync_begin_linear_timestamp + sub_session->start_time;
            }
        }
        p_node = mpSubSessionList->NextNode(p_node);
    }

}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END


