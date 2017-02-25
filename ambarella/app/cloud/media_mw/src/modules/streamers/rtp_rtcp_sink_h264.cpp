/*******************************************************************************
 * rtp_rtcp_sink_h264.cpp
 *
 * History:
 *    2013/11/04 - [Zhi He] create file
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
#include "mw_internal.h"
#include "dsp_platform_interface.h"
#include "streaming_if.h"
#include "modules_interface.h"

#include "rtp_rtcp_sink_h264.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

IStreamingTransmiter *gfCreateH264RtpRtcpTransmiter(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink)
{
    return CH264RTPRTCPSink::Create(pname, pPersistMediaConfig, pMsgSink);
}

IStreamingTransmiter *CH264RTPRTCPSink::Create(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink)
{
    CH264RTPRTCPSink *result = new CH264RTPRTCPSink(pname, pPersistMediaConfig, pMsgSink);
    if (result && result->Construct() != EECode_OK) {
        delete result;
        result = NULL;
    }
    return result;
}

CH264RTPRTCPSink::CH264RTPRTCPSink(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink)
    : inherited(pname)
    , mStreamingType(StreamTransportType_RTP)
    , mProtocolType(ProtocolType_UDP)
    , mFormat(StreamFormat_H264)
    , mType(StreamType_Video)
    , mpClockReference(NULL)
    , mpMutex(NULL)
    , mpConfig(pPersistMediaConfig)
    , mpMsgSink(pMsgSink)
    , mbRTPSocketSetup(0)
    , mPayloadType(RTP_PT_H264)
    , mbSrBuilt(0)
    , mRTPSocket(DInvalidSocketHandler)
    , mRTCPSocket(DInvalidSocketHandler)
    , mRTPPort(1800)
    , mRTCPPort(1801)
    , mCurrentTime(0)
    , mpRTPBuffer(NULL)
    , mRTPBufferLength(0)
    , mRTPBufferTotalLength(DRecommandMaxUDPPayloadLength + 32)
    , mpRTCPBuffer(NULL)
    , mRTCPBufferLength(0)
    , mRTCPBufferTotalLength(128)
    , mpExtraData(NULL)
    , mExtraDataSize(0)
{
    memset(&mInfo, 0x0, sizeof(mInfo));
    new(&mSubSessionList) CIDoubleLinkedList();
}

EECode CH264RTPRTCPSink::Construct()
{
    DSET_MODULE_LOG_CONFIG(ELogModuleStreamingTransmiter);
    mConfigLogLevel = ELogLevel_Debug;

    if ((mpMutex = gfCreateMutex()) == NULL) {
        LOGM_FATAL("CH264RTPRTCPSink::Construct(), gfCreateMutex() fail.\n");
        return EECode_NoMemory;
    }

    mpRTPBuffer = (TU8 *)DDBG_MALLOC(mRTPBufferTotalLength, "RTBF");
    if (DUnlikely(!mpRTPBuffer)) {
        LOGM_FATAL("NO Memory.\n");
        return EECode_NoMemory;
    }

    mpRTCPBuffer = (TU8 *)DDBG_MALLOC(mRTCPBufferTotalLength, "RTCB");
    if (DUnlikely(!mpRTCPBuffer)) {
        LOGM_FATAL("NO Memory.\n");
        return EECode_NoMemory;
    }

    mpClockReference = mpConfig->p_system_clock_reference;

    return EECode_OK;
}

void CH264RTPRTCPSink::Destroy()
{
    Delete();
}

void CH264RTPRTCPSink::Delete()
{
    if (mpMutex) {
        mpMutex->Delete();
        mpMutex = NULL;
    }

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

    if (mpExtraData) {
        DDBG_FREE(mpExtraData);
        mpExtraData = NULL;
    }

    inherited::Delete();
}

CH264RTPRTCPSink::~CH264RTPRTCPSink()
{
    if (mpMutex) {
        mpMutex->Delete();
        mpMutex = NULL;
    }

    if (mpRTPBuffer) {
        DDBG_FREE(mpRTPBuffer);
        mpRTPBuffer = NULL;
    }

    if (mpRTCPBuffer) {
        DDBG_FREE(mpRTCPBuffer);
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
        DDBG_FREE(mpExtraData);
        mpExtraData = NULL;
    }

    mSubSessionList.~CIDoubleLinkedList();
}

EECode CH264RTPRTCPSink::SetupContext(StreamTransportType type, ProtocolType protocol_type, const SStreamCodecInfos *infos, TTime start_time)
{
    return EECode_OK;
}

EECode CH264RTPRTCPSink::DestroyContext()
{
    return EECode_OK;
}

EECode CH264RTPRTCPSink::UpdateStreamFormat(StreamType type, StreamFormat format)
{
    DASSERT(StreamType_Video == type);
    DASSERT(StreamFormat_H264 == format);

    return EECode_OK;
}

EECode CH264RTPRTCPSink::Start()
{
    return EECode_OK;
}

EECode CH264RTPRTCPSink::Stop()
{
    return EECode_OK;
}

void CH264RTPRTCPSink::udpSendData(TU8 *p_data, TMemSize size, TTime pts, TUint update_time_stamp)
{
    CIDoubleLinkedList::SNode *p_node = mSubSessionList.FirstNode();
    SSubSessionInfo *sub_session = NULL;

    while (p_node) {
        sub_session = (SSubSessionInfo *)p_node->p_context;
        if (DLikely(sub_session)) {

            if (DUnlikely(sub_session->wait_first_key_frame)) {
                p_node = mSubSessionList.NextNode(p_node);
                continue;
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

            TU32 value = sub_session->cur_time;
            p_data[2] = sub_session->seq_number >> 8;
            p_data[3] = sub_session->seq_number & 0xff;
            p_data[4] = (value >> 24) & 0xff;
            p_data[5] = (value >> 16) & 0xff;
            p_data[6] = (value >> 8) & 0xff;
            p_data[7] = value & 0xff;
            value = sub_session->rtp_ssrc;
            p_data[8] = (value >> 24) & 0xff;
            p_data[9] = (value >> 16) & 0xff;
            p_data[10] = (value >> 8) & 0xff;
            p_data[11] = value & 0xff;

            TInt ret = gfNet_SendTo(mRTPSocket, p_data, size, 0, (const void *)&sub_session->addr_port, (TSocketSize)sizeof(struct sockaddr_in));
            if (DUnlikely(ret < 0)) {
                LOGM_ERROR("rtp_send_data failed\n");
            }

            if (DUnlikely(!sub_session->is_started)) {
                sub_session->is_started = 1;
                sub_session->ntp_time = 90 * mpClockReference->GetCurrentTime();
                processRTCP(sub_session, sub_session->ntp_time, (TU32)sub_session->start_time);
                sub_session->rtcp_sr_current_count = 0;
            } else {
                sub_session->statistics.cur_seq_num = sub_session->seq_number;
                sub_session->statistics.last_octet_count = size;

                sub_session->statistics.octet_count += size;
                sub_session->statistics.packet_count ++;

                sub_session->rtcp_sr_current_count ++;
#if 1
                if (DUnlikely(sub_session->rtcp_sr_current_count > sub_session->rtcp_sr_cooldown)) {
                    sub_session->ntp_time = 90 * mpClockReference->GetCurrentTime();
                    processRTCP(sub_session, sub_session->ntp_time, sub_session->cur_time);
                    sub_session->rtcp_sr_current_count = 0;
                }
#endif
            }

            if (update_time_stamp) {
                if (DLikely(sub_session->p_content && (DDefaultVideoFramerateNum == sub_session->p_content->video_framerate_num) && (sub_session->p_content->video_framerate_den))) {
                    sub_session->cur_time += sub_session->p_content->video_framerate_den;
                } else {
                    sub_session->cur_time += DDefaultVideoFramerateDen;
                }
            }
            sub_session->seq_number ++;
        }
        p_node = mSubSessionList.NextNode(p_node);
    }
}

void CH264RTPRTCPSink::udpSendSr(SSubSessionInfo *sub_session)
{
    DASSERT(sub_session);

    TInt ret = gfNet_SendTo(mRTCPSocket, mpRTCPBuffer, mRTCPBufferLength, 0, (const void *)&sub_session->addr_port_ext, (TSocketSize)sizeof(struct sockaddr_in));
    if (ret < 0) {
        LOGM_ERROR("udpSendSr failed, mRTPSocket %d\n", mRTPSocket);
    }
}

EECode CH264RTPRTCPSink::SetExtraData(TU8 *pdata, TMemSize size)
{
    LOGM_NOTICE("SetExtraData, size %ld\n", size);
    if ((mpExtraData) && (mExtraDataSize < size)) {
        DDBG_FREE(mpExtraData);
        mpExtraData = NULL;
        mExtraDataSize = 0;
    }

    if (!mpExtraData) {
        mpExtraData = (TU8 *) DDBG_MALLOC(size);
        if (DUnlikely(!mpExtraData)) {
            return EECode_NoMemory;
        }
        mExtraDataSize = size;
    }

    memcpy(mpExtraData, pdata, mExtraDataSize);
    return EECode_OK;
}

EECode CH264RTPRTCPSink::SendData(CIBuffer *pBuffer)
{
    TMemSize data_size = 0;
    TU8 *p_data = NULL;

    AUTO_LOCK(mpMutex);

    if (DLikely(pBuffer)) {
        if (DUnlikely(!mSubSessionList.NumberOfNodes())) {
            //LOGM_DEBUG("idle loop\n");
            return EECode_OK;
        }

        if (DUnlikely(EBufferType_VideoExtraData == pBuffer->GetBufferType())) {
            TU8 *p1 = NULL;
            TMemSize size1 = 0;

            data_size = pBuffer->GetDataSize();
            p_data = pBuffer->GetDataPtr();

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
                udpSendData(mpRTPBuffer, size1 + 12, pBuffer->GetBufferPTS(), 0);

                size1 = (TMemSize)(p_data + data_size - p1);
                DASSERT(((TMemSize)(p_data + data_size)) > ((TMemSize)(p1)));

                memcpy(mpRTPBuffer + 12, p1, size1);
                udpSendData(mpRTPBuffer, size1 + 12, pBuffer->GetBufferPTS(), 0);
            }
            return EECode_OK;
        } else if (DUnlikely(CIBuffer::KEY_FRAME & pBuffer->GetBufferFlags())) {
            CIDoubleLinkedList::SNode *p_node = mSubSessionList.FirstNode();
            SSubSessionInfo *sub_session = NULL;

            LOGM_INFO("key frame comes\n");
            DASSERT(mExtraDataSize);
            DASSERT(mpExtraData);

            {

                while (p_node) {
                    sub_session = (SSubSessionInfo *)p_node->p_context;
                    if (DLikely(sub_session)) {
                        if (DUnlikely(sub_session->wait_first_key_frame)) {
                            if (DUnlikely((!mExtraDataSize) && (!mpExtraData))) {
                                LOGM_WARN("no extra data: mExtraDataSize %ld, mpExtraData %p\n", mExtraDataSize, mpExtraData);
                                continue;
                            }
                            sub_session->wait_first_key_frame = 0;

                            TU8 *p1 = NULL;
                            //TU8* p2 = mpExtraData;
                            TMemSize size1 = 0;

                            data_size = mExtraDataSize - 4;
                            p_data = mpExtraData + 4;
#if 0
                            mpRTPBuffer[0] = (RTP_VERSION << 6);
                            mpRTPBuffer[1] = (RTP_PT_H264 & 0x7f) | 0x80;

                            memcpy(mpRTPBuffer + 12, p_data, data_size);
                            udpSendData(mpRTPBuffer, data_size + 12, 0, 0);
#endif
                            p1 = gfNALUFindNextStartCode(p_data, data_size);
                            //p1 = p_data + data_size - 4;
                            DASSERT(data_size <= DRecommandMaxRTPPayloadLength);
                            if (DUnlikely(data_size >= DRecommandMaxRTPPayloadLength)) {
                                LOGM_FATAL("data_size %ld, p_data %p\n", data_size, p_data);
                                sub_session->wait_first_key_frame = 1;
                                return EECode_Error;
                            }

#if 1
                            if (DLikely(p1)) {
                                mpRTPBuffer[0] = (RTP_VERSION << 6);
                                mpRTPBuffer[1] = (RTP_PT_H264 & 0x7f) | 0x80;
                                size1 = (TMemSize)(p1 - p_data - 4);
                                DASSERT(((TMemSize)(p1 - 4)) > ((TMemSize)(p_data)));

                                memcpy(mpRTPBuffer + 12, p_data, size1);
                                udpSendData(mpRTPBuffer, size1 + 12, 0, 0);

                                size1 = (TMemSize)(p_data + data_size - p1);
                                DASSERT(((TMemSize)(p_data + data_size)) > ((TMemSize)(p1)));

                                memcpy(mpRTPBuffer + 12, p1, size1);
                                udpSendData(mpRTPBuffer, size1 + 12, 0, 0);
                            } else {
                                LOGM_FATAL("bad extra data\n");
                            }
#endif
                        }
                    }
                    p_node = mSubSessionList.NextNode(p_node);
                }
            }
        }

        //LOGM_DEBUG("udp send data, size %ld, time %lld\n", data_size, mCurrentTime);
        data_size = pBuffer->GetDataSize();
        p_data = pBuffer->GetDataPtr();
        DASSERT(!p_data[0]);
        DASSERT(!p_data[1]);
        DASSERT(!p_data[2]);
        DASSERT(1 == p_data[3]);
        p_data += 4;
        data_size -= 4;

        if (data_size <= DRecommandMaxRTPPayloadLength) {
            mpRTPBuffer[0] = (RTP_VERSION << 6);
            mpRTPBuffer[1] = (RTP_PT_H264 & 0x7f) | 0x80;
            memcpy(mpRTPBuffer + 12, p_data, data_size);
            udpSendData(mpRTPBuffer, data_size + 12, pBuffer->GetBufferPTS(), 1);
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
                udpSendData(mpRTPBuffer, DRecommandMaxRTPPayloadLength + 12, pBuffer->GetBufferPTS(), 0);
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
            udpSendData(mpRTPBuffer, data_size + 14, pBuffer->GetBufferPTS(), 1);
        }

        mCurrentTime += DDefaultVideoFramerateDen;
    } else {
        LOGM_FATAL("NULL pBuffer\n");
        return EECode_BadParam;
    }


    return EECode_OK;
}

EECode CH264RTPRTCPSink::AddSubSession(SSubSessionInfo *p_sub_session)
{
    AUTO_LOCK(mpMutex);

    if (p_sub_session) {
        memset(&p_sub_session->statistics, 0, sizeof(p_sub_session->statistics));
        p_sub_session->wait_first_key_frame = 1;

        LOGM_INFO("CH264RTPRTCPSink::AddSubSession(): addr 0x%08x, socket 0x%x, rtp port %u, rtcp port %u.\n", p_sub_session->addr, p_sub_session->socket, p_sub_session->client_rtp_port, p_sub_session->client_rtcp_port);
        mSubSessionList.InsertContent(NULL, (void *)p_sub_session, 0);

        return EECode_OK;
    }
    LOGM_FATAL("NULL pointer in CH264RTPRTCPSink::AddSubSession.\n");
    return EECode_BadParam;
}

EECode CH264RTPRTCPSink::RemoveSubSession(SSubSessionInfo *p_sub_session)
{
    AUTO_LOCK(mpMutex);
    if (p_sub_session) {
        LOGM_INFO("CH264RTPRTCPSink::RemoveSubSession(): addr 0x%08x, socket 0x%x, rtp port %u, rtcp port %u.\n", p_sub_session->addr, p_sub_session->socket, p_sub_session->client_rtp_port, p_sub_session->client_rtcp_port);
        mSubSessionList.RemoveContent((void *)p_sub_session);
        return EECode_OK;
    }
    LOGM_FATAL("NULL pointer in CH264RTPRTCPSink::RemoveSubSession.\n");
    return EECode_BadParam;
}

EECode CH264RTPRTCPSink::SetSrcPort(TSocketPort port, TSocketPort port_ext)
{
    AUTO_LOCK(mpMutex);

    if (!mbRTPSocketSetup) {
        LOGM_NOTICE("CH264RTPRTCPSink::SetSrcPort[video](%hu, %hu).\n", port, port_ext);
        mbRTPSocketSetup = 1;
        mRTPPort = port;
        mRTCPPort = port_ext;

        setupUDPSocket();
        return EECode_OK;
    } else {
        LOGM_NOTICE("RTP socket(video) has been setup, port %hu, %hu.\n", mRTPPort, mRTCPPort);
    }

    return EECode_Error;
}

EECode CH264RTPRTCPSink::GetSrcPort(TSocketPort &port, TSocketPort &port_ext)
{
    AUTO_LOCK(mpMutex);
    //DASSERT(StreamType_Video == mType);

    port = mRTPPort;
    port_ext = mRTCPPort;
    LOGM_NOTICE("CH264RTPRTCPSink::GetSrcPort(%hu, %hu).\n", port, port_ext);

    return EECode_OK;
}

EECode CH264RTPRTCPSink::setupUDPSocket()
{
    //DASSERT(mType == StreamType_Video);

    if (mRTPSocket >= 0) {
        LOGM_WARN("close previous socket here, %d.\n", mRTPSocket);
        gfNetCloseSocket(mRTPSocket);
        mRTPSocket = -1;
    }

    if (mRTCPSocket >= 0) {
        LOGM_WARN("close previous socket here, %d.\n", mRTCPSocket);
        gfNetCloseSocket(mRTCPSocket);
        mRTCPSocket = -1;
    }

    LOGM_NOTICE(" before SetupDatagramSocket, port %d.\n", mRTPPort);
    mRTPSocket = gfNet_SetupDatagramSocket(INADDR_ANY, mRTPPort, 0, 0, 0);
    LOGM_NOTICE(" mRTPSocket %d.\n", mRTPSocket);
    if (mRTPSocket < 0) {
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
    if (mRTCPSocket < 0) {
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

void CH264RTPRTCPSink::processRTCP(SSubSessionInfo *sub_session, TTime ntp_time, TU32 timestamp)
{
    DASSERT(sub_session);

    if (DUnlikely(mbSrBuilt)) {
        mbSrBuilt = 1;
        buildSr(sub_session->rtp_ssrc, ntp_time, timestamp, sub_session->statistics.packet_count, sub_session->statistics.octet_count);
        mbSrBuilt = 0;
    } else {
        updateSr(ntp_time, timestamp, sub_session->statistics.packet_count, sub_session->statistics.octet_count);
    }

    udpSendSr(sub_session);
}

void CH264RTPRTCPSink::buildSr(TUint ssrc, TS64 ntp_time, TUint timestamp, TUint packet_count, TUint octet_count)
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

void CH264RTPRTCPSink::updateSr(TS64 ntp_time, TUint timestamp, TUint packet_count, TUint octet_count)
{
    if (DUnlikely((!mpRTCPBuffer) || (mRTCPBufferTotalLength < 28))) {
        LOGM_FATAL("NULL mpRTCPBuffer, mRTCPBuffer %p, TotalLength %ld\n", mpRTCPBuffer, mRTCPBufferTotalLength);
        return;
    }

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

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END
#endif


