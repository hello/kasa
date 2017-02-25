/*******************************************************************************
 * rtp_rtcp_sink_audio.cpp
 *
 * History:
 *    2013/11/07 - [Zhi He] create file
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

#include "rtp_rtcp_sink_audio.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

IStreamingTransmiter *gfCreateAudioRtpRtcpTransmiter(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink)
{
    return CAudioRTPRTCPSink::Create(pname, pPersistMediaConfig, pMsgSink);
}

IStreamingTransmiter *CAudioRTPRTCPSink::Create(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink)
{
    CAudioRTPRTCPSink *result = new CAudioRTPRTCPSink(pname, pPersistMediaConfig, pMsgSink);
    if (result && result->Construct() != EECode_OK) {
        delete result;
        result = NULL;
    }
    return result;
}

CAudioRTPRTCPSink::CAudioRTPRTCPSink(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink)
    : inherited(pname)
    , mStreamingType(StreamTransportType_RTP)
    , mProtocolType(ProtocolType_UDP)
    , mFormat(StreamFormat_AAC)
    , mType(StreamType_Audio)
    , mpClockReference(NULL)
    //, mpMutex(NULL)
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
    , mpExtraData(NULL)
    , mExtraDataSize(0)
{
    new(&mSubSessionList) CIDoubleLinkedList();
}

EECode CAudioRTPRTCPSink::Construct()
{
    DSET_MODULE_LOG_CONFIG(ELogModuleStreamingTransmiter);
#if 0
    if ((mpMutex = gfCreateMutex()) == NULL) {
        LOGM_FATAL("CAudioRTPRTCPSink::Construct(), gfCreateMutex() fail.\n");
        return EECode_NoMemory;
    }
#endif

    mpRTPBuffer = (TU8 *)DDBG_MALLOC(DRecommandMaxUDPPayloadLength, "RTBF");
    if (mpRTPBuffer) {
        mRTPBufferTotalLength = DRecommandMaxUDPPayloadLength;
    } else {
        LOGM_FATAL("NO Memory.\n");
    }

    mpRTCPBuffer = (TU8 *)DDBG_MALLOC(mRTCPBufferTotalLength, "RCTB");
    if (DUnlikely(!mpRTCPBuffer)) {
        LOGM_FATAL("NO Memory.\n");
        return EECode_NoMemory;
    }

    mpClockReference = mpConfig->p_system_clock_reference;

    return EECode_OK;
}

void CAudioRTPRTCPSink::Destroy()
{
    Delete();
}

void CAudioRTPRTCPSink::Delete()
{
#if 0
    if (mpMutex) {
        mpMutex->Delete();
        mpMutex = NULL;
    }
#endif

    if (mpRTPBuffer) {
        DDBG_FREE(mpRTPBuffer, "RTBF");
        mpRTPBuffer = NULL;
    }

    if (mpRTCPBuffer) {
        DDBG_FREE(mpRTCPBuffer, "RTCB");
        mpRTCPBuffer = NULL;
    }

    if (mRTPSocket >= 0) {
        gfNetCloseSocket(mRTPSocket);
        mRTPSocket = -1;
    }

    if (mRTCPSocket >= 0) {
        gfNetCloseSocket(mRTCPSocket);
        mRTCPSocket = -1;
    }

    if (mpExtraData) {
        DDBG_FREE(mpExtraData, "RTVE");
        mpExtraData = NULL;
    }

    inherited::Delete();
}

CAudioRTPRTCPSink::~CAudioRTPRTCPSink()
{
#if 0
    if (mpMutex) {
        mpMutex->Delete();
        mpMutex = NULL;
    }
#endif

    if (mpRTPBuffer) {
        DDBG_FREE(mpRTPBuffer, "RTBF");
        mpRTPBuffer = NULL;
    }

    if (mpRTCPBuffer) {
        DDBG_FREE(mpRTCPBuffer, "RTCB");
        mpRTCPBuffer = NULL;
    }

    if (mRTPSocket >= 0) {
        gfNetCloseSocket(mRTPSocket);
        mRTPSocket = -1;
    }

    if (mRTCPSocket >= 0) {
        gfNetCloseSocket(mRTCPSocket);
        mRTCPSocket = -1;
    }

    if (mpExtraData) {
        DDBG_FREE(mpExtraData, "RTVE");
        mpExtraData = NULL;
    }

    mSubSessionList.~CIDoubleLinkedList();
}

EECode CAudioRTPRTCPSink::SetupContext(StreamTransportType type, ProtocolType protocol_type, const SStreamCodecInfos *infos, TTime start_time)
{
    return EECode_OK;
}

EECode CAudioRTPRTCPSink::DestroyContext()
{
    return EECode_OK;
}

EECode CAudioRTPRTCPSink::UpdateStreamFormat(StreamType type, StreamFormat format)
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

EECode CAudioRTPRTCPSink::Start()
{
    return EECode_OK;
}

EECode CAudioRTPRTCPSink::Stop()
{
    return EECode_OK;
}
void CAudioRTPRTCPSink::udpSendData(TU8 *p_data, TMemSize size, TTime pts)
{
    CIDoubleLinkedList::SNode *p_node = mSubSessionList.FirstNode();
    SSubSessionInfo *sub_session = NULL;

    while (p_node) {
        sub_session = (SSubSessionInfo *)p_node->p_context;
        if (DLikely(sub_session)) {
            if (DUnlikely(!sub_session->is_started)) {
                sub_session->start_time = 0;
                sub_session->cur_time = sub_session->start_time;
                sub_session->is_started = 1;
            }

            TU32 timestamp = sub_session->cur_time;
            updateSendRtcp(sub_session, timestamp, (TU32)size);
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

            TInt ret = gfNet_SendTo(mRTPSocket, p_data, size, 0, (const void *)&sub_session->addr_port, (TSocketSize)sizeof(struct sockaddr_in));
            if (DUnlikely(ret < 0)) {
                LOGM_ERROR("rtp_send_data failed\n");
            } else {
                rtcp_stat_t *s = &sub_session->rtcp_stat;
                s->octet_count += size;
                ++s->packet_count;
            }
        }
        p_node = mSubSessionList.NextNode(p_node);
    }
}

EECode CAudioRTPRTCPSink::SetExtraData(TU8 *pdata, TMemSize size)
{
    //AUTO_LOCK(mpMutex);
    if ((mpExtraData) && (mExtraDataSize < size)) {
        DDBG_FREE(mpExtraData, "RTVE");
        mpExtraData = NULL;
        mExtraDataSize = 0;
    }

    if (!mpExtraData) {
        mpExtraData = (TU8 *) DDBG_MALLOC(size, "RTVE");
        if (DUnlikely(!mpExtraData)) {
            return EECode_NoMemory;
        }
        mExtraDataSize = size;
    }
    LOGM_WARN("CAudioRTPRTCPSink::SetExtraData, size=%lu, pdata: 0x%x 0x%x\n", size, pdata[0], pdata[1]);
    memcpy(mpExtraData, pdata, mExtraDataSize);
    return EECode_OK;
}

EECode CAudioRTPRTCPSink::SendData(CIBuffer *pBuffer)
{
    //AUTO_LOCK(mpMutex);
    EECode ret = EECode_OK;
    //    static TU8 extradata_sent = 0;

    if (DLikely(pBuffer)) {
        if (DUnlikely(!mSubSessionList.NumberOfNodes())) {
            return EECode_OK;
        }

        /*if (!extradata_sent) {
            mpRTPBuffer[0] = (RTP_VERSION << 6);
            mpRTPBuffer[1] = (mPayloadType & 0x7f) | 0x80;
            memcpy(mpRTPBuffer + 12, mpExtraData, mExtraDataSize);
            udpSendData(mpRTPBuffer, mExtraDataSize + 12, pBuffer->GetBufferPTS());
            LOGM_WARN("CAudioRTPRTCPSink audio extradata sent, size=%lu, mpExtraData=0x%x 0x%x.\n",mExtraDataSize,mpExtraData[0],mpExtraData[1]);
            extradata_sent = 1;
        }*/

        if (StreamFormat_AAC == mFormat) {
            ret = sendDataAAC(pBuffer);
        } else if ((StreamFormat_PCMU == mFormat) || (StreamFormat_PCMA == mFormat)) {
            ret = sendDataPCM(pBuffer);
        } else if (StreamFormat_MPEG12Audio == mFormat) {
            ret = sendDataMpeg12Audio(pBuffer);
        } else {
            ret = EECode_NotSupported;
            LOGM_FATAL("BAD stream format %d\n", mFormat);
        }
    } else {
        LOGM_FATAL("NULL pBuffer\n");
        ret = EECode_BadParam;
    }

    return ret;
}

EECode CAudioRTPRTCPSink::sendDataAAC(CIBuffer *pBuffer)
{
    TMemSize data_size = 0;
    TU8 *p_data = NULL;

    data_size = pBuffer->GetDataSize();
    p_data = pBuffer->GetDataPtr();

    //LOGM_PRINTF("[aac data]: %02x %02x %02x %02x, %02x %02x %02x %02x\n", p_data[0], p_data[1], p_data[2], p_data[3], p_data[4], p_data[5], p_data[6], p_data[7]);
    if ((0xff == p_data[0]) && (0xf0 == (p_data[1] & 0xf0))) {
        p_data += 7;
        data_size -= 7;
        //LOGM_PRINTF("[aac raw data]: %02x %02x\n", p_data[0], p_data[1]);
    }

    if (DLikely(data_size <= DRecommandMaxRTPPayloadLength)) {
        mpRTPBuffer[0] = (RTP_VERSION << 6);
        mpRTPBuffer[1] = (mPayloadType & 0x7f) | 0x80;

        if (pBuffer->GetBufferFlags() & CIBuffer::RTP_AAC) {
            memcpy(mpRTPBuffer + 12, p_data, data_size);
            udpSendData(mpRTPBuffer, data_size + 12, pBuffer->GetBufferLinearPTS());
        } else {
            mpRTPBuffer[12] = 0x00;
            mpRTPBuffer[13] = 0x10;
            mpRTPBuffer[14] = (data_size & 0x1fe0) >> 5;
            mpRTPBuffer[15] = (data_size & 0x1f) << 3;
            memcpy(mpRTPBuffer + 16, p_data, data_size);
            udpSendData(mpRTPBuffer, data_size + 16, pBuffer->GetBufferLinearPTS());
        }
    } else {
        LOGM_ERROR("should not comes here\n");
    }

    return EECode_OK;
}

EECode CAudioRTPRTCPSink::sendDataPCM(CIBuffer *pBuffer)
{
    TMemSize data_size = 0;
    TU8 *p_data = NULL;

    data_size = pBuffer->GetDataSize();
    p_data = pBuffer->GetDataPtr();

    if (DLikely(data_size <= DRecommandMaxRTPPayloadLength)) {
        mpRTPBuffer[0] = (RTP_VERSION << 6);
        mpRTPBuffer[1] = (mPayloadType & 0x7f) | 0x80;

        memcpy(mpRTPBuffer + 12, p_data, data_size);

        udpSendData(mpRTPBuffer, data_size + 12, pBuffer->GetBufferPTS());
    } else {
        LOGM_ERROR("should not comes here\n");
    }

    return EECode_OK;
}

EECode CAudioRTPRTCPSink::sendDataMpeg12Audio(CIBuffer *pBuffer)
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

        udpSendData(mpRTPBuffer, data_size + 16, pBuffer->GetBufferPTS());
    } else {
        LOGM_ERROR("should not comes here\n");
    }

    return EECode_OK;
}

EECode CAudioRTPRTCPSink::AddSubSession(SSubSessionInfo *p_sub_session)
{
    //AUTO_LOCK(mpMutex);

    if (p_sub_session) {
        memset(&p_sub_session->statistics, 0, sizeof(p_sub_session->statistics));
        p_sub_session->wait_first_key_frame = 1;
        memset(&p_sub_session->rtcp_stat, 0, sizeof(p_sub_session->rtcp_stat));
        p_sub_session->rtcp_stat.first_packet = 1;
        p_sub_session->is_started = 0;
        p_sub_session->is_closed = 0;

        LOGM_NOTICE("CAudioRTPRTCPSink::AddSubSession(): addr %d, socket 0x%x, rtp port %u, rtcp port %u.\n", p_sub_session->addr, p_sub_session->socket, p_sub_session->client_rtp_port, p_sub_session->client_rtcp_port);
        mSubSessionList.InsertContent(NULL, (void *)p_sub_session, 0);

        return EECode_OK;
    }
    LOGM_FATAL("NULL pointer in CAudioRTPRTCPSink::AddSubSession.\n");
    return EECode_BadParam;
}

EECode CAudioRTPRTCPSink::RemoveSubSession(SSubSessionInfo *p_sub_session)
{
    //AUTO_LOCK(mpMutex);
    if (p_sub_session) {
        LOGM_NOTICE("CAudioRTPRTCPSink::RemoveSubSession(): addr %d, socket 0x%x, port %u, port_ext %u.\n", p_sub_session->addr, p_sub_session->socket, p_sub_session->client_rtp_port, p_sub_session->client_rtcp_port);
        mSubSessionList.RemoveContent((void *)p_sub_session);
        return EECode_OK;
    }
    LOGM_FATAL("NULL pointer in CAudioRTPRTCPSink::RemoveSubSession.\n");
    return EECode_BadParam;
}

EECode CAudioRTPRTCPSink::SetSrcPort(TSocketPort port, TSocketPort port_ext)
{
    //AUTO_LOCK(mpMutex);

    if (!mbRTPSocketSetup) {
        LOGM_NOTICE("CAudioRTPRTCPSink::SetSrcPort[audio](%hu, %hu).\n", port, port_ext);
        mRTPPort = port;
        mRTCPPort = port_ext;

        setupUDPSocket();
    } else {
        LOGM_NOTICE("RTP socket(audio) has been setup, port %hu, %hu.\n", mRTPPort, mRTCPPort);
    }

    return EECode_OK;
}

EECode CAudioRTPRTCPSink::GetSrcPort(TSocketPort &port, TSocketPort &port_ext)
{
    //AUTO_LOCK(mpMutex);

    port = mRTPPort;
    port_ext = mRTCPPort;
    LOGM_NOTICE("CAudioRTPRTCPSink::GetSrcPort(%hu, %hu).\n", port, port_ext);

    return EECode_OK;
}

EECode CAudioRTPRTCPSink::setupUDPSocket()
{
    if (0 == mbRTPSocketSetup) {
        mbRTPSocketSetup = 1;
    } else {
        LOGM_WARN("socket has been setup, please check code.\n");
        return EECode_OK;
    }

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

void CAudioRTPRTCPSink::udpSendSr(SSubSessionInfo *sub_session)
{
    DASSERT(sub_session);

    TInt ret = gfNet_SendTo(mRTCPSocket, mpRTCPBuffer, mRTCPBufferLength, 0, (const void *)&sub_session->addr_port, (TSocketSize)sizeof(struct sockaddr_in));
    if (DUnlikely(ret < 0)) {
        LOGM_ERROR("udpSendSr failed\n");
    }
}

int CAudioRTPRTCPSink::updateSendRtcp(SSubSessionInfo *sub_session, TU32 timestamp, TU32 len)
{
    TTime ntp_time = gfGetNTPTime();

    rtcp_stat_t *s = &sub_session->rtcp_stat;
    TS32 rtcp_bytes = (s->octet_count - s->last_octet_count) * 5 / 1000;
    if (s->first_packet || ((rtcp_bytes >= 28) &&
                            (ntp_time - s->last_rtcp_ntp_time > 5000000) && (s->timestamp != timestamp))) {

        if (s->first_packet) {
            s->first_rtcp_ntp_time = ntp_time;
            s->base_timestamp = timestamp;
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

void CAudioRTPRTCPSink::processRTCP(SSubSessionInfo *sub_session, TTime ntp_time, TU32 timestamp)
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

void CAudioRTPRTCPSink::buildSr(TUint ssrc, TS64 ntp_time, TUint timestamp, TUint packet_count, TUint octet_count)
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

void CAudioRTPRTCPSink::updateSr(TUint ssrc, TS64 ntp_time, TUint timestamp, TUint packet_count, TUint octet_count)
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

void CAudioRTPRTCPSink::buildBye(TUint ssrc)
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

void CAudioRTPRTCPSink::processBYE(SSubSessionInfo *sub_session)
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

