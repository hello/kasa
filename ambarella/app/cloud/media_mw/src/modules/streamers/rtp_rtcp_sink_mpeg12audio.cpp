/*******************************************************************************
 * rtp_rtcp_sink_mpeg12audio.cpp
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
#include "mw_internal.h"
#include "dsp_platform_interface.h"
#include "streaming_if.h"
#include "modules_interface.h"

#include "rtp_rtcp_sink_mpeg12audio.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

IStreamingTransmiter *gfCreateMpeg12AudioRtpRtcpTransmiter(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink)
{
    return CMpeg12AudioRTPRTCPSink::Create(pname, pPersistMediaConfig, pMsgSink);
}

IStreamingTransmiter *CMpeg12AudioRTPRTCPSink::Create(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink)
{
    CMpeg12AudioRTPRTCPSink *result = new CMpeg12AudioRTPRTCPSink(pname, pPersistMediaConfig, pMsgSink);
    if (result && result->Construct() != EECode_OK) {
        delete result;
        result = NULL;
    }
    return result;
}

CMpeg12AudioRTPRTCPSink::CMpeg12AudioRTPRTCPSink(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink)
    : inherited(pname)
    , mStreamingType(StreamTransportType_RTP)
    , mProtocolType(ProtocolType_UDP)
    , mFormat(StreamFormat_MPEG12Audio)
    , mType(StreamType_Audio)
    , mpClockReference(NULL)
    , mpMutex(NULL)
    , mpConfig(pPersistMediaConfig)
    , mpMsgSink(pMsgSink)
    , mbRTPSocketSetup(0)
    , mPayloadType(RTP_PT_AAC)
    , mRTPSocket(DInvalidSocketHandler)
    , mRTCPSocket(DInvalidSocketHandler)
    , mRTPPort(1800)
    , mRTCPPort(1801)
    , mpRTPBuffer(NULL)
    , mRTPBufferLength(0)
    , mRTPBufferTotalLength(DRecommandMaxUDPPayloadLength + 32)
    , mpExtraData(NULL)
    , mExtraDataSize(0)
{
    memset(&mInfo, 0x0, sizeof(mInfo));
    new(&mSubSessionList) CIDoubleLinkedList();
}

EECode CMpeg12AudioRTPRTCPSink::Construct()
{
    DSET_MODULE_LOG_CONFIG(ELogModuleStreamingTransmiter);

    if ((mpMutex = gfCreateMutex()) == NULL) {
        LOGM_FATAL("CMpeg12AudioRTPRTCPSink::Construct(), gfCreateMutex() fail.\n");
        return EECode_NoMemory;
    }

    mpRTPBuffer = (TU8 *)DDBG_MALLOC(DRecommandMaxUDPPayloadLength, "RTbf");
    if (mpRTPBuffer) {
        mRTPBufferTotalLength = DRecommandMaxUDPPayloadLength;
    } else {
        LOGM_FATAL("NO Memory.\n");
    }

    return EECode_OK;
}

void CMpeg12AudioRTPRTCPSink::Destroy()
{
    Delete();
}

void CMpeg12AudioRTPRTCPSink::Delete()
{
    if (mpMutex) {
        mpMutex->Delete();
        mpMutex = NULL;
    }

    if (mpRTPBuffer) {
        DDBG_FREE(mpRTPBuffer, "RTbf");
        mpRTPBuffer = NULL;
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
        free(mpExtraData);
        mpExtraData = NULL;
    }

    inherited::Delete();
}

CMpeg12AudioRTPRTCPSink::~CMpeg12AudioRTPRTCPSink()
{
    if (mpMutex) {
        mpMutex->Delete();
        mpMutex = NULL;
    }

    if (mpRTPBuffer) {
        free(mpRTPBuffer);
        mpRTPBuffer = NULL;
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
        free(mpExtraData);
        mpExtraData = NULL;
    }

    mSubSessionList.~CIDoubleLinkedList();
}

EECode CMpeg12AudioRTPRTCPSink::SetupContext(StreamTransportType type, ProtocolType protocol_type, const SStreamCodecInfos *infos, TTime start_time)
{
    return EECode_OK;
}

EECode CMpeg12AudioRTPRTCPSink::DestroyContext()
{
    return EECode_OK;
}

EECode CMpeg12AudioRTPRTCPSink::UpdateStreamFormat(StreamType type, StreamFormat format)
{
    DASSERT(StreamType_Audio == type);
    DASSERT(StreamFormat_MPEG12Audio == format);

    return EECode_OK;
}

EECode CMpeg12AudioRTPRTCPSink::Start()
{
    return EECode_OK;
}

EECode CMpeg12AudioRTPRTCPSink::Stop()
{
    return EECode_OK;
}

void CMpeg12AudioRTPRTCPSink::udpSendData(TU8 *p_data, TMemSize size, TTime pts)
{
    CIDoubleLinkedList::SNode *p_node = mSubSessionList.FirstNode();
    SSubSessionInfo *sub_session = NULL;

    while (p_node) {
        sub_session = (SSubSessionInfo *)p_node->p_context;
        if (DLikely(sub_session)) {
            TU32 value = sub_session->cur_time;
            value = pts;
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
            sub_session->cur_time += 1024;
            sub_session->seq_number ++;
        }
        p_node = mSubSessionList.NextNode(p_node);
    }
}

EECode CMpeg12AudioRTPRTCPSink::SetExtraData(TU8 *pdata, TMemSize size)
{
    if ((mpExtraData) && (mExtraDataSize < size)) {
        free(mpExtraData);
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

EECode CMpeg12AudioRTPRTCPSink::SendData(CIBuffer *pBuffer)
{
    TMemSize data_size = 0;
    TU8 *p_data = NULL;

    AUTO_LOCK(mpMutex);

    if (DLikely(pBuffer)) {
        if (DUnlikely(!mSubSessionList.NumberOfNodes())) {
            return EECode_OK;
        }
        data_size = pBuffer->GetDataSize();
        p_data = pBuffer->GetDataPtr();

        p_data += 7;
        data_size -= 7;

        if (DLikely(data_size <= DRecommandMaxRTPPayloadLength)) {
            mpRTPBuffer[0] = (RTP_VERSION << 6);
            mpRTPBuffer[1] = (mPayloadType & 0x7f) | 0x80;

            mpRTPBuffer[12] = 0x00;
            mpRTPBuffer[13] = 0x10;
            mpRTPBuffer[14] = (data_size & 0x1fe0) >> 5;
            mpRTPBuffer[15] = (data_size & 0x1f) << 3;

            memcpy(mpRTPBuffer + 16, p_data, data_size);

            udpSendData(mpRTPBuffer, data_size + 16, pBuffer->GetBufferPTS());
        } else {
            LOGM_ERROR("should not comes here\n");
        }
    } else {
        LOGM_FATAL("NULL pBuffer\n");
        return EECode_BadParam;
    }

    return EECode_OK;
}

EECode CMpeg12AudioRTPRTCPSink::AddSubSession(SSubSessionInfo *p_sub_session)
{
    AUTO_LOCK(mpMutex);

    if (p_sub_session) {
        memset(&p_sub_session->statistics, 0, sizeof(p_sub_session->statistics));
        p_sub_session->wait_first_key_frame = 1;

        LOGM_NOTICE("CMpeg12AudioRTPRTCPSink::AddSubSession(): addr %d, socket 0x%x, rtp port %u, rtcp port %u.\n", p_sub_session->addr, p_sub_session->socket, p_sub_session->client_rtp_port, p_sub_session->client_rtcp_port);
        mSubSessionList.InsertContent(NULL, (void *)p_sub_session, 0);

        return EECode_OK;
    }
    LOGM_FATAL("NULL pointer in CMpeg12AudioRTPRTCPSink::AddSubSession.\n");
    return EECode_BadParam;
}

EECode CMpeg12AudioRTPRTCPSink::RemoveSubSession(SSubSessionInfo *p_sub_session)
{
    AUTO_LOCK(mpMutex);
    if (p_sub_session) {
        LOGM_NOTICE("CMpeg12AudioRTPRTCPSink::RemoveSubSession(): addr %d, socket 0x%x, port %u, port_ext %u.\n", p_sub_session->addr, p_sub_session->socket, p_sub_session->client_rtp_port, p_sub_session->client_rtcp_port);
        mSubSessionList.RemoveContent((void *)p_sub_session);
        return EECode_OK;
    }
    LOGM_FATAL("NULL pointer in CMpeg12AudioRTPRTCPSink::RemoveSubSession.\n");
    return EECode_BadParam;
}

EECode CMpeg12AudioRTPRTCPSink::SetSrcPort(TSocketPort port, TSocketPort port_ext)
{
    AUTO_LOCK(mpMutex);
    //DASSERT(StreamType_Video == mType);

    if (!mbRTPSocketSetup) {
        LOGM_NOTICE("CMpeg12AudioRTPRTCPSink::SetSrcPort[video](%hu, %hu).\n", port, port_ext);
        mRTPPort = port;
        mRTCPPort = port_ext;

        setupUDPSocket();
        return EECode_OK;
    } else {
        LOGM_FATAL("RTP socket(video) has been setup, port %hu, %hu.\n", mRTPPort, mRTCPPort);
        return EECode_Error;
    }

    LOGM_FATAL("BAD type %d.\n", mType);
    return EECode_Error;
}

EECode CMpeg12AudioRTPRTCPSink::GetSrcPort(TSocketPort &port, TSocketPort &port_ext)
{
    AUTO_LOCK(mpMutex);
    //DASSERT(StreamType_Video == mType);

    port = mRTPPort;
    port_ext = mRTCPPort;
    LOGM_NOTICE("CMpeg12AudioRTPRTCPSink::GetSrcPort(%hu, %hu).\n", port, port_ext);

    return EECode_OK;
}

EECode CMpeg12AudioRTPRTCPSink::setupUDPSocket()
{
    //DASSERT(mType == StreamType_Video);

    if (0 == mbRTPSocketSetup) {
        mbRTPSocketSetup = 1;
    } else {
        LOGM_WARN("socket has been setup, please check code.\n");
        return EECode_OK;
    }

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

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END
#endif

