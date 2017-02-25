/*******************************************************************************
 * rtsp_server_v2.cpp
 *
 * History:
 *    2014/10/28 - [Zhi He] create file
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

#include "security_utils_if.h"

#include "media_mw_if.h"
#include "media_mw_utils.h"
#include "framework_interface.h"
#include "mw_internal.h"
#include "dsp_platform_interface.h"
#include "streaming_if.h"
#include "modules_interface.h"
#include "rtsp_server_v2.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

static TChar _RTSPallowedCommandNames[] = "OPTIONS, DESCRIBE, SETUP, PLAY, PAUSE, RESUME, TEARDOWN";

static void retrieveHostString(TChar *buf, TChar *dest, TUint max_len)
{
    TU32 part1, part2, part3, part4;
    TChar *ptmp;
    //find "rtsp://" prefix
    ptmp = strchr((TChar *)buf, ':');
    if (ptmp) {
        DASSERT(((TPointer)ptmp) > ((TPointer)(buf) + 4));
        if (((TPointer)ptmp) > ((TPointer)(buf) + 4)) {
            DASSERT(ptmp[-4] == 'r');
            DASSERT(ptmp[-3] == 't');
            DASSERT(ptmp[-2] == 's');
            DASSERT(ptmp[-1] == 'p');
            DASSERT(ptmp[1] == '/');
            DASSERT(ptmp[2] == '/');
            ptmp += 3;
            if (4 == sscanf((const TChar *)ptmp, "%u.%u.%u.%u", &part1, &part2, &part3, &part4)) {
                LOG_INFO("Get host string: %u.%u.%u.%u.\n", part1, part2, part3, part4);
                snprintf((TChar *)dest, max_len, "%u.%u.%u.%u", part1, part2, part3, part4);
            } else {
                LOG_ERROR(" get host string(%s, %s) fail, use default 10.0.0.2.\n", buf, ptmp);
                strncpy((TChar *)dest, "10.0.0.2", max_len);
            }
            return;
        }
    }

    LOG_ERROR("may be not valid request string, please check, use 10.0.0.2 as default.\n");
    strncpy((TChar *)dest, "10.0.0.2", max_len);
}

static TChar *find_string(const TChar *p, TU32 length, const TChar *target)
{
    if (DUnlikely((!p) || (!length) || (!target))) {
        LOG_FATAL("BAD params\n");
        return NULL;
    }

    TU32 target_len = strlen(target);
    if (DUnlikely(target_len > length)) {
        LOG_FATAL("BAD params %d, %d\n", target_len, length);
        return NULL;
    }

    length -= target_len;
    while (length) {
        if (DUnlikely(!strncmp(p, target, target_len))) {
            return (TChar *)p;
        }
        p ++;
        length --;
    }

    return NULL;
}

IStreamingServer *gfCreateRtspStreamingServerV2(const TChar *name, StreamingServerType type, StreamingServerMode mode, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink, TU16 server_port, TU8 enable_video, TU8 enable_audio)
{
    return (IStreamingServer *)CRTSPStreamingServerV2::Create(name, type, mode, pPersistMediaConfig, pMsgSink, server_port, enable_video, enable_audio);
}

EECode CRTSPStreamingServerV2::HandleServerRequest(TSocketHandler &max, void *all_set)
{
    SClientSessionInfo *client_session = NULL;
    socklen_t   clilen;
    TUint i;
    fd_set *mAllSet = (fd_set *)all_set;
    EECode err = EECode_OK;

    LOGM_DEBUG("HandleServerRequest server type %d, server mode %d.\n", mType, mMode);
    mDebugHeartBeat ++;

    switch (mMode) {
        case StreamingServerMode_Unicast:
        case StreamingServerMode_MulticastSetAddr:
            //new client
            if (DUnlikely(mpClientSessionList->NumberOfNodes() > EMaxStreamingSessionNumber)) {
                LOGM_ERROR(" Reject NEW client: Too many clients\n");
                return EECode_TooMany;
            }

            client_session = (SClientSessionInfo *)DDBG_MALLOC(sizeof(SClientSessionInfo), "SVCS");
            if (NULL == client_session) {
                LOGM_ERROR("NOT enough memory.\n");
                return EECode_NoMemory;
            }
            memset(client_session, 0x0, sizeof(SClientSessionInfo));
            //pthread_mutex_init(&client_session->lock, NULL);
            client_session->p_server = this;
            client_session->speed_combo = 8;
            clilen = sizeof(struct sockaddr_in);
do_retry:
            client_session->tcp_socket = gfNet_Accept(mSocket, &client_session->client_addr, (TSocketSize *)&clilen, err);
            if (DUnlikely(client_session->tcp_socket < 0)) {
                if (EECode_TryAgain == err) {
                    LOGM_WARN("non-blocking call?\n");
                    goto do_retry;
                }
                LOGM_ERROR("accept fail, return %d.\n", client_session->tcp_socket);
                DDBG_FREE(client_session, "SVCS");
                client_session = NULL;
            } else {
                LOGM_INFO(" NEW client: %s, port %d, socket %d.\n", inet_ntoa(client_session->client_addr.sin_addr), ntohs(client_session->client_addr.sin_port), client_session->tcp_socket);
                DASSERT(client_session->tcp_socket >= 0);

                if (mpPersistMediaConfig->rtsp_server_config.enable_nonblock_timeout) {
                    gfSocketSetNonblocking(client_session->tcp_socket);
                }
                gfSocketSetRecvBufferSize(client_session->tcp_socket, 8 * 1024);
                gfSocketSetSendBufferSize(client_session->tcp_socket, 512 * 1024);
                gfSocketSetNoDelay(client_session->tcp_socket);
                gfSocketSetLinger(client_session->tcp_socket, 1, 0, 20);

                //add socket to listenning list
                FD_SET(client_session->tcp_socket, mAllSet);
                if (client_session->tcp_socket > max) {
                    LOGM_INFO(" update mMaxFd(ori %d) socket %d.\n", max, client_session->tcp_socket);
                    max = client_session->tcp_socket;
                }

                client_session->enable_video = mbEnableVideo;
                client_session->enable_audio = mbEnableAudio;
                client_session->state = ESessionServerState_Init;//wait authenticate
                client_session->protocol_type = ProtocolType_UDP;
                client_session->session_id = 0; //hard code
                client_session->vod_mode = 0;
                client_session->speed_combo = 8;
                client_session->p_pipeline = NULL;
                client_session->authenticator.realm = DSRTING_RTSP_REALM;
                memset((void *)client_session->authenticator.password, 0x0, DMaxRtspAuthenPasswordLength);

                //initial sub sessions
                for (i = 0; i < ESubSession_tot_count; i++) {
                    client_session->sub_session[i].parent = client_session;
                    client_session->sub_session[i].track_id = i;
                    client_session->sub_session[i].addr = (TU32)ntohl(client_session->client_addr.sin_addr.s_addr);
                    client_session->sub_session[i].socket = client_session->tcp_socket;
                }
                mpClientSessionList->InsertContent(NULL, (void *)client_session, 1);
            }
            break;

        case StreamingServerMode_MultiCastPickAddr:
            LOGM_FATAL("conference request? server pickup multicast addr.\n");
            //todo
            break;

        default:
            LOGM_ERROR("BAD server->mode %d.\n", mMode);
            break;
    }


    return EECode_OK;
}

bool CRTSPStreamingServerV2::parseRTSPRequestString(TChar const *reqStr,
        TU32 reqStrSize,
        TChar *resultCmdName,
        TU32 resultCmdNameMaxSize,
        TChar *resultURLPreSuffix,
        TU32 resultURLPreSuffixMaxSize,
        TChar *resultURLSuffix,
        TU32 resultURLSuffixMaxSize,
        TChar *resultCSeq,
        TU32 resultCSeqMaxSize)
{
    bool parseSucceeded = false;
    TU32 i, j , k;
    for (i = 0; i < resultCmdNameMaxSize - 1 && i < reqStrSize; ++i) {
        TChar c = reqStr[i];
        if (c == ' ' || c == '\t') {
            parseSucceeded = true;
            break;
        }
        resultCmdName[i] = c;
    }
    resultCmdName[i] = '\0';
    if (!parseSucceeded) {
        LOGM_ERROR("parer cmd Error.\n");
        return false;
    }

    j = i + 1;
    while (j < reqStrSize && (reqStr[j] == ' ' || reqStr[j] == '\t')) { ++j; }

    for (; (TInt)j < (TInt)(reqStrSize - 8); ++j) {
        if ((reqStr[j] == 'r' || reqStr[j] == 'R')
                && (reqStr[j + 1] == 't' || reqStr[j + 1] == 'T')
                && (reqStr[j + 2] == 's' || reqStr[j + 2] == 'S')
                && (reqStr[j + 3] == 'p' || reqStr[j + 3] == 'P')
                && reqStr[j + 4] == ':' && reqStr[j + 5] == '/') {
            j += 6;
            if (reqStr[j] == '/') {
                ++j;
                while (j < reqStrSize && reqStr[j] != '/' && reqStr[j] != ' ') { ++j; }
            } else {
                --j;
            }
            i = j;
            break;
        }
    }

    parseSucceeded = false;
    for (k = i + 1; (TInt)k < (TInt)(reqStrSize - 5); ++k) {

        if (reqStr[k] == 'R' && reqStr[k + 1] == 'T' && reqStr[k + 2] == 'S' && reqStr[k + 3] == 'P' && reqStr[k + 4] == '/') {

            while (--k >= i && reqStr[k] == ' ') {}
            TU32 k1 = k;
            while (k1 > i && reqStr[k1] != '/') { --k1; }

            if (k - k1 + 1 > resultURLSuffixMaxSize) { return false; }
            TU32 n = 0, k2 = k1 + 1;
            while (k2 <= k) { resultURLSuffix[n++] = reqStr[k2++]; }
            resultURLSuffix[n] = '\0';

            unsigned k3 = (k1 == 0) ? 0 : --k1;
            while (k3 > i && reqStr[k3] != '/') { --k3; }

            if (k1 - k3 + 1 > resultURLPreSuffixMaxSize) { return false; }
            n = 0; k2 = k3 + 1;
            while (k2 <= k1) { resultURLPreSuffix[n++] = reqStr[k2++]; }
            resultURLPreSuffix[n] = '\0';

            i = k + 7;
            parseSucceeded = true;
            break;
        }
    }

    if (DUnlikely(!parseSucceeded)) {
        LOGM_ERROR("parer url Error.\n");
        return false;
    }

    parseSucceeded = false;
    for (j = i; (TInt)j < (TInt)(reqStrSize - 5); ++j) {

        if (reqStr[j] == 'C' && reqStr[j + 1] == 'S' && reqStr[j + 2] == 'e' &&
                reqStr[j + 3] == 'q' && reqStr[j + 4] == ':') {
            j += 5;
            unsigned n;
            while (j < reqStrSize && (reqStr[j] ==  ' ' || reqStr[j] == '\t')) { ++j; } // skip over any additional white space
            for (n = 0; n < resultCSeqMaxSize - 1 && j < reqStrSize; ++n, ++j) {
                TChar c = reqStr[j];
                if (c == '\r' || c == '\n') {
                    parseSucceeded = true;
                    break;
                }
                resultCSeq[n] = c;
            }
            resultCSeq[n] = '\0';
            break;
        }
    }

    if (DUnlikely(!parseSucceeded)) {
        LOGM_ERROR("parer CSeq Error.\n");
        return false;
    }

    return true;
}

bool CRTSPStreamingServerV2::parseAuthorizationHeader(TChar const *buf,
        TChar const *&username,
        TChar const *&realm,
        TChar const *&nonce, TChar const *&uri,
        TChar const *&response)
{
    DASSERT(buf);
    username = realm = nonce = uri = response = NULL;

    for (;;) {
        if (*buf == '\0') { return false; }
        if ((strncmp(buf, "Authorization: Digest ", 22) == 0) && (mpPersistMediaConfig->rtsp_authentication_mode == 1)) {
            break;
        }

        if ((strncmp(buf, "Authorization: Basic ", 21) == 0) && (mpPersistMediaConfig->rtsp_authentication_mode == 0)) {
            break;
        }
        ++buf;
    }

    if (mpPersistMediaConfig->rtsp_authentication_mode == 0) {
        TChar const *fields = buf + 21;
        while (*fields == ' ') { ++fields; }
        TU32 len = strlen(fields) + 1;
        TChar *p_buffer = (TChar *) DDBG_MALLOC(len, "SVTB");
        DASSERT(p_buffer);
        memcpy(p_buffer, fields, len);
        response = p_buffer;
        for (;;) {
            if (*p_buffer == '\r' && *(p_buffer + 1) == '\n') {
                *p_buffer = '\0';
                break;
            }
            ++p_buffer;
        }
        return true;
    }

    TChar const *fields = buf + 22;
    while (*fields == ' ') { ++fields; }
    TU32 len = strlen(fields) + 1;
    TChar *parameter = (TChar *) DDBG_MALLOC(len, "SVTP");
    TChar *value = (TChar *) DDBG_MALLOC(len, "SVTV");
    DASSERT(parameter);
    DASSERT(value);

    TChar *p_buffer = NULL;
    for (;;) {
        value[0] = '\0';
        if (sscanf(fields, "%[^=]=\"%[^\"]\"", parameter, value) != 2 && sscanf(fields, "%[^=]=\"\"", parameter) != 1) {
            break;
        }

        len = strlen(value) + 1;
        p_buffer = (TChar *) DDBG_MALLOC(len, "SVTB");
        DASSERT(p_buffer);
        memcpy(p_buffer, value, len);

        if (strncmp(parameter, "username", 8) == 0) {
            username = p_buffer;
        } else if (strncmp(parameter, "realm", 5) == 0) {
            realm = p_buffer;
        } else if (strncmp(parameter, "nonce", 5) == 0) {
            nonce = p_buffer;
        } else if (strncmp(parameter, "uri", 3) == 0) {
            uri = p_buffer;
        } else if (strncmp(parameter, "response", 8) == 0) {
            response = p_buffer;
        }

        fields += strlen(parameter) + 2 /*="*/ + strlen(value) + 1 /*"*/;
        while (*fields == ',' || *fields == ' ') { ++fields; }
        // skip over any separating ',' and ' ' chars
        if (*fields == '\0' || *fields == '\r' || *fields == '\n') { break; }
    }

    DDBG_FREE(parameter, "SVTP");
    DDBG_FREE(value, "SVTV");

    return true;
}

EECode CRTSPStreamingServerV2::authentication(SClientSessionInfo *client_session, const TChar *p_url)
{
    CIDoubleLinkedList::SNode *p_node = mpContentList->FirstNode();
    SStreamingSessionContent *p_content = NULL;
    TMemSize string_size = 0, string_size1 = 0;
    TU8 content_type = 0;
    EECode err = parseRequestUrl(client_session, p_url, content_type);
    if (DUnlikely(EECode_OK != err)) {
        LOGM_ERROR("parseRequestUrl fail, request url [%s], ret %d, %s\n", p_url, err, gfGetErrorCodeString(err));
        return err;
    }
    TChar *p_channel_name = client_session->authenticator.devicename;
    string_size1 = strlen(p_channel_name);

    LOGM_DEBUG("[debug rtsp]: request url [%s], string_size1 %ld, mContentList.NumberOfNodes() %d\n", p_channel_name, string_size1, mpContentList->NumberOfNodes());

    while (p_node) {
        p_content = (SStreamingSessionContent *)p_node->p_context;
        DASSERT(p_content);
        if (DLikely(p_content)) {
            string_size = strlen(p_content->session_name);

            LOGM_DEBUG("p_url %s, p_content->session_name %s, string_size %ld, string_size1 %ld\n", p_channel_name, p_content->session_name, string_size, string_size1);
            if (p_content->sub_session_content[ESubSession_video]) {
                LOGM_DEBUG("parameters_setup %d, data comes %d\n", p_content->sub_session_content[ESubSession_video]->parameters_setup, p_content->sub_session_content[ESubSession_video]->data_comes);
            }
            if (string_size1 == string_size) {
                if ((p_content->content_type == content_type) && !memcmp(p_content->session_name, p_channel_name, string_size1)) {
                    LOGM_DEBUG("p_url %s, p_content->session_name %s, string_size %ld, string_size1 %ld\n", p_channel_name, p_content->session_name, string_size, string_size1);

                    if (!p_content->has_video) {
                        LOGM_DEBUG("content no video, p_content->has_video %d, b_content_setup %d, sub_session_count %d\n", p_content->has_video, p_content->b_content_setup, p_content->sub_session_count);
                        client_session->enable_video = 0;
                    }

                    if (!p_content->has_audio) {
                        LOGM_DEBUG("content no audio, p_content->has_audio %d, b_content_setup %d, sub_session_count %d\n", p_content->has_audio, p_content->b_content_setup, p_content->sub_session_count);
                        client_session->enable_audio = 0;
                    }

                    if ((!p_content->content_type) && (client_session->enable_video) && (p_content->sub_session_content[ESubSession_video]) && (!p_content->sub_session_content[ESubSession_video]->parameters_setup)) {
                        LOGM_WARN("[request '%s'] video data not comes, content %s, content id %08x, index %d, sub session %p\n", p_channel_name, p_content->session_name, p_content->id, p_content->content_index, p_content->sub_session_content[ESubSession_video]);
                        client_session->enable_video = 0;
                    }
                    if ((!p_content->content_type) && (client_session->enable_audio) && (p_content->sub_session_content[ESubSession_audio]) && (!p_content->sub_session_content[ESubSession_audio]->parameters_setup)) {
                        LOGM_WARN("[request '%s'] audio data not comes, content %s, content id %08x, index %d\n", p_channel_name, p_content->session_name, p_content->id, p_content->content_index);
                        client_session->enable_audio = 0;
                    }

                    if (!client_session->enable_audio && !client_session->enable_video) {
                        LOGM_ERROR("[RTSP]: both video and audio are not avaiable now!\n");
                        return EECode_BadState;
                    }

                    client_session->p_session_name = &p_content->session_name[0];

                    client_session->server_port = mListeningPort;
                    client_session->p_content = p_content;
                    client_session->sub_session[ESubSession_video].p_content = p_content->sub_session_content[ESubSession_video];
                    client_session->sub_session[ESubSession_audio].p_content = p_content->sub_session_content[ESubSession_audio];

                    //###VOD###
                    LOGM_DEBUG("p_content->content_type %u\n", p_content->content_type);
                    if (p_content->content_type != 0) {
                        //get idle pipeline
                        client_session->p_pipeline = NULL;
                        EECode err = EECode_OK;
                        //check extradata is ready or not
                        if ((client_session->enable_video) && (!p_content->sub_session_content[ESubSession_video]->parameters_setup)) {
                            err = mpPersistMediaConfig->p_engine_v3->ParseContentExtraData((void *)p_content);
                            if (err != EECode_OK) {
                                return EECode_BadState;
                            }
                        } else if ((client_session->enable_audio) && (!p_content->sub_session_content[ESubSession_audio]->parameters_setup)) {
                            err = mpPersistMediaConfig->p_engine_v3->ParseContentExtraData((void *)p_content);
                            if (err != EECode_OK) {
                                return EECode_BadState;
                            }
                        }

                        err = mpPersistMediaConfig->p_engine_v3->GetPipeline(1, client_session->p_pipeline);
                        if (DUnlikely(err != EECode_OK)) {
                            LOGM_FATAL("No Idle Pipeline!\n");
                            return err;
                        }
                        SVODPipeline *p_pipeline = (SVODPipeline *)client_session->p_pipeline;
                        p_pipeline->p_data_transmiter->p_filter->FilterProperty(EFilterPropertyType_streaming_set_content, 1, (void *)client_session);

                        client_session->last_cmd_time = getCurrentSystemTime();

                        client_session->vod_mode = 1;
                        client_session->p_streaming_transmiter_filter = p_pipeline->p_data_transmiter->p_filter;

                        return EECode_OK;
                    } else {
                        client_session->p_streaming_transmiter_filter = p_content->p_streaming_transmiter_filter;
                        client_session->vod_mode = 0;
                    }
                    //###VOD###
                    if (DLikely((client_session->enable_video) && p_content->sub_session_content[ESubSession_video] && p_content->sub_session_content[ESubSession_video]->enabled)) {
                        if (DLikely(client_session->sub_session[ESubSession_video].p_content->p_transmiter)) {
                            client_session->sub_session[ESubSession_video].server_rtp_port = p_content->sub_session_content[ESubSession_video]->server_rtp_port;
                            client_session->sub_session[ESubSession_video].server_rtcp_port = p_content->sub_session_content[ESubSession_video]->server_rtcp_port;

                            client_session->sub_session[ESubSession_video].p_content->p_transmiter->SetSrcPort(client_session->sub_session[ESubSession_video].server_rtp_port, client_session->sub_session[ESubSession_video].server_rtcp_port);
                        } else {
                            LOGM_WARN("NULL p_transmiter\n");
                        }
                    }

                    if (DLikely((client_session->enable_audio) && p_content->sub_session_content[ESubSession_audio] && p_content->sub_session_content[ESubSession_audio]->enabled)) {
                        if (DLikely(client_session->sub_session[ESubSession_audio].p_content->p_transmiter)) {
                            client_session->sub_session[ESubSession_audio].server_rtp_port = p_content->sub_session_content[ESubSession_audio]->server_rtp_port;
                            client_session->sub_session[ESubSession_audio].server_rtcp_port = p_content->sub_session_content[ESubSession_audio]->server_rtcp_port;

                            client_session->sub_session[ESubSession_audio].p_content->p_transmiter->SetSrcPort(client_session->sub_session[ESubSession_audio].server_rtp_port, client_session->sub_session[ESubSession_audio].server_rtcp_port);
                        } else {
                            LOGM_WARN("NULL p_transmiter\n");
                        }
                    }

                    client_session->last_cmd_time = getCurrentSystemTime();
                    return EECode_OK;
                }
            }
        } else {
            LOGM_FATAL("NULL p_content, content %s\n", p_url);
            return EECode_InternalLogicalBug;
        }
        p_node = mpContentList->NextNode(p_node);
    }

    LOGM_INFO("no streaming content [%s], blow is APP configured\n", p_url);
    //debug print
    p_node = mpContentList->FirstNode();
    while (p_node) {
        p_content = (SStreamingSessionContent *)p_node->p_context;
        DASSERT(p_content);
        if (DLikely(p_content)) {
            string_size = strlen(p_content->session_name);
            LOGM_INFO("p_content(id %08x, index %d)->session_name %s, string_size %ld\n", p_content->id, p_content->content_index, p_content->session_name, string_size);
        } else {
            LOGM_FATAL("NULL p_content\n");
        }
        p_node = mpContentList->NextNode(p_node);
    }
    return EECode_BadParam;
}

EECode CRTSPStreamingServerV2::parseRequestUrl(SClientSessionInfo *p_client_session, const TChar *url, TU8 &content_type)
{
    TU32 len1 = strlen(url);

    TChar *p_time = (TChar *)strchr(url, ':');
    if (p_time == NULL) {
        if (DUnlikely(DMAX_ACCOUNT_NAME_LENGTH_EX <= len1)) {
            LOGM_ERROR("too long url, %d\n", len1);
            return EECode_BadParam;
        }
        memcpy(p_client_session->authenticator.devicename, url, len1);
        p_client_session->authenticator.devicename[len1] = 0x0;
        content_type = 0;
        return EECode_OK;
    }

    content_type = 1;
    ++ p_time;
    TU32 len2 = strlen(p_time);
    TU32 len3 = len1 - len2 - 1;
    if (DUnlikely(DMAX_ACCOUNT_NAME_LENGTH_EX <= len3)) {
        LOGM_ERROR("too long url, %d\n", len3);
        return EECode_BadParam;
    }
    memcpy(p_client_session->authenticator.devicename, url, len3);
    p_client_session->authenticator.devicename[len3] = 0x0;

    if (len2) {
        TU32 year = 0, month = 0, day = 0, hour = 0, minute = 0, seconds = 0;
        TU32 tmp0 = 0, tmp1 = 0;
        tmp0 = sscanf(p_time, "T%d-%d-%d-%d-%d-%d", &year, &month, &day, &hour, &minute, &seconds);
        if (DUnlikely(6 != tmp0)) {
            LOGM_ERROR("bad begin time info, ret %d\n", tmp0);
            return EECode_BadParam;
        }
        p_client_session->starttime.year = (TU16)year;
        p_client_session->starttime.month = (TU8)month;
        p_client_session->starttime.day = (TU8)day;
        p_client_session->starttime.hour = (TU8)hour;
        p_client_session->starttime.minute = (TU8)minute;
        p_client_session->starttime.seconds = (TU8)seconds;

        p_client_session->vod_mode = 1;

        p_time = (TChar *)strchr(p_time, ':');
        if (p_time) {
            if ('S' == p_time[1]) {
                tmp0 = sscanf(p_time, ":S%d", &tmp1);
                if (DUnlikely(1 != tmp0)) {
                    LOGM_ERROR("bad speed info, ret %d\n", tmp0);
                    return EECode_BadParam;
                }

                p_client_session->speed_combo = tmp1;
                p_client_session->vod_has_end = 0;
                LOG_NOTICE("VOD request from %d-%d-%d-%d-%d-%d, no end, speed_combo %d\n", p_client_session->starttime.year, \
                           p_client_session->starttime.month, \
                           p_client_session->starttime.day, \
                           p_client_session->starttime.hour, \
                           p_client_session->starttime.minute, \
                           p_client_session->starttime.seconds, \
                           p_client_session->speed_combo);
                return EECode_OK;
            } else if ('E' != p_time[1]) {
                LOGM_ERROR("no end time info\n");
                return EECode_BadParam;
            }

            p_time += 2;
            tmp0 = sscanf(p_time, "%d-%d-%d-%d-%d-%d", &year, &month, &day, &hour, &minute, &seconds);
            if (DUnlikely(6 != tmp0)) {
                LOGM_ERROR("bad end time info, ret %d\n", tmp0);
                return EECode_BadParam;
            }

            p_client_session->endtime.year = (TU16)year;
            p_client_session->endtime.month = (TU8)month;
            p_client_session->endtime.day = (TU8)day;
            p_client_session->endtime.hour = (TU8)hour;
            p_client_session->endtime.minute = (TU8)minute;
            p_client_session->endtime.seconds = (TU8)seconds;
            p_client_session->vod_has_end = 1;

            SDateTimeGap time_gap;
            gfDateCalculateGap(p_client_session->endtime, p_client_session->starttime, time_gap);
            p_client_session->vod_content_length = time_gap.overall_seconds;
            LOGM_NOTICE("request content length %lld\n", time_gap.overall_seconds);

            p_time = (TChar *)strchr(p_time, ':');
            if (p_time && ('S' == p_time[1])) {
                tmp0 = sscanf(p_time, ":S%d", &tmp1);
                if (DUnlikely(1 != tmp0)) {
                    LOGM_ERROR("bad speed info, ret %d\n", tmp0);
                    return EECode_BadParam;
                }
                p_client_session->speed_combo = tmp1;
            }

            LOG_NOTICE("VOD request from %d-%d-%d-%d-%d-%d, to %d-%d-%d-%d-%d-%d, speed_combo %d\n", p_client_session->starttime.year, \
                       p_client_session->starttime.month, \
                       p_client_session->starttime.day, \
                       p_client_session->starttime.hour, \
                       p_client_session->starttime.minute, \
                       p_client_session->starttime.seconds, \
                       p_client_session->endtime.year, \
                       p_client_session->endtime.month, \
                       p_client_session->endtime.day, \
                       p_client_session->endtime.hour, \
                       p_client_session->endtime.minute, \
                       p_client_session->endtime.seconds, \
                       p_client_session->speed_combo);
        } else {
            p_client_session->vod_has_end = 0;
            LOG_NOTICE("VOD request from %d-%d-%d-%d-%d-%d, no end, no speed info\n", p_client_session->starttime.year, \
                       p_client_session->starttime.month, \
                       p_client_session->starttime.day, \
                       p_client_session->starttime.hour, \
                       p_client_session->starttime.minute, \
                       p_client_session->starttime.seconds);
        }
    }

    return EECode_OK;
}

EECode CRTSPStreamingServerV2::authenticationUser(SClientSessionInfo *p_client_session, const TChar *cmd, const TChar *cseq, const TChar *fullRequestStr)
{
    if (mpPersistMediaConfig->enable_rtsp_authentication == 0) {
        LOGM_NOTICE("authenticationUser, rtsp authentication is disabled\n");
        return EECode_OK;
    }

    EECode err = EECode_OK;
    TU8 success = 0;
    TChar const *username = NULL;
    TChar const *realm = NULL;
    TChar const *nonce  = NULL;
    TChar const *uri = NULL; TChar const *response = NULL;

    do {
        if (mpPersistMediaConfig->rtsp_authentication_mode == 1) {
            LOGM_WARN("authenticationUser: no nonce?\n");
            break;
        }

        if (!parseAuthorizationHeader(fullRequestStr, username, realm, nonce, uri, response)) {
            LOG_FATAL("authenticationUser, parseAuthorizationHeader fail\n");
            break;
        }

        if (mpPersistMediaConfig->rtsp_authentication_mode == 0) {
            if (DUnlikely(response == NULL)) {
                LOGM_FATAL("authenticationUser, Basic authentication, parseAuthorizationHeader fail, reponse is NULL\n");
                break;
            }

            TU32 length = (strlen(response) >> 2) * 3;
            TChar *p_account = (TChar *) DDBG_MALLOC(length + 1, "SVAC");
            DASSERT(p_account);
            p_account[length] = 0x0;
            gfDecodingBase64((TU8 *)p_account, (const TU8 *)response, length);
            TChar *p_password = strchr(p_account, ':');
            if (DUnlikely(!p_password)) {
                LOGM_FATAL("authenticationUser, Basic authentication fail, p_account: %s\n", p_account);
                DDBG_FREE(p_account, "SVAC");
                break;
            }

            *p_password = '\0';
            ++p_password;
            TU32 password = 0;
            err = mpPersistMediaConfig->p_engine_v3->LookupPassword(p_account, p_client_session->authenticator.devicename, password);
            if (DUnlikely(EECode_OK != err)) {
                LOGM_FATAL("authenticationUser, Basic authentication, LookupPassword fail, err %d, %s\n", err, gfGetErrorCodeString(err));
                DDBG_FREE(p_account, "SVAC");
                break;
            }
            memset((void *)p_client_session->authenticator.password, 0x0, DMaxRtspAuthenPasswordLength);
            snprintf(p_client_session->authenticator.password, DMaxRtspAuthenPasswordLength, "%x", password);

            if (strcmp(p_password, p_client_session->authenticator.password) == 0) {
                success = 1;
            } else {
                LOGM_FATAL("authenticationUser, invalid input password: %s, %s\n", p_password, p_client_session->authenticator.password);
            }
            DDBG_FREE(p_account, "SVAC");
            break;
        }

        if (username == NULL || realm == NULL || strcmp(realm, p_client_session->authenticator.realm) != 0
                || nonce == NULL || strcmp(nonce, p_client_session->authenticator.nonce) != 0
                || uri == NULL || response == NULL) {
            LOGM_FATAL("authenticationUser, parseAuthorizationHeader fail, parameter error, fullRequestStr: %s\n", fullRequestStr);
            break;
        }

        //check password
        TU32 password = 0;
        err = mpPersistMediaConfig->p_engine_v3->LookupPassword(username, p_client_session->authenticator.devicename, password);
        if (DUnlikely(EECode_OK != err)) {
            LOGM_FATAL("authenticationUser, LookupPassword fail, err %d, %s\n", err, gfGetErrorCodeString(err));
            break;
        }

        TU32 name_len = strlen(username) + 1;
        memcpy((void *)p_client_session->authenticator.username, username, name_len);
        p_client_session->authenticator.username[name_len] = 0x0;

        memset((void *)p_client_session->authenticator.password, 0x0, DMaxRtspAuthenPasswordLength);
        snprintf(p_client_session->authenticator.password, DMaxRtspAuthenPasswordLength, "%x", password);

        TChar *p_our_response = NULL;
        err = computeDigestResponse(p_client_session, cmd, uri, p_our_response);
        if (DUnlikely(EECode_OK != err)) {
            LOGM_FATAL("authenticationUser, computeDigestResponse fail, err %d, %s\n", err, gfGetErrorCodeString(err));
            break;
        }

        if (strcmp(p_our_response, response) == 0) {
            success = 1;
        }
        DDBG_FREE(p_our_response, "HSOT");
        p_our_response = NULL;
    } while (0);

    DDBG_FREE((void *)username, "SVTB");
    DDBG_FREE((void *)realm, "SVTB");
    DDBG_FREE((void *)nonce, "SVTB");
    DDBG_FREE((void *)uri, "SVTB");
    DDBG_FREE((void *)response, "SVTB");

    if (success == 0) {
        gfGetDateString(mDateBuffer, sizeof(mDateBuffer));
        if (mpPersistMediaConfig->rtsp_authentication_mode == 0) {
            //basic authentication
            snprintf((TChar *)mResponseBuffer, sizeof(mResponseBuffer),
                     "RTSP/1.0 401 Unauthorized\r\n"
                     "Server: %s\r\n"
                     "CSeq: %s\r\n"
                     "%s"
                     "WWW-Authenticate: Basic realm=\"%s\"\r\n\r\n",
                     DSRTING_RTSP_SERVER_TAG,
                     cseq,
                     mDateBuffer,
                     p_client_session->authenticator.realm);
        } else {
            //digest authentication
            setRandomNonce(p_client_session);
            snprintf((TChar *)mResponseBuffer, sizeof(mResponseBuffer),
                     "RTSP/1.0 401 Unauthorized\r\n"
                     "Server: %s\r\n"
                     "CSeq: %s\r\n"
                     "%s"
                     "WWW-Authenticate: Digest realm=\"%s\", nonce=\"%s\"\r\n\r\n",
                     DSRTING_RTSP_SERVER_TAG,
                     cseq,
                     mDateBuffer,
                     p_client_session->authenticator.realm, p_client_session->authenticator.nonce);
        }

        return EECode_Error;
    }

    return EECode_OK;
}

EECode CRTSPStreamingServerV2::setRandomNonce(SClientSessionInfo *p_client_session)
{
    struct {
        TTime timestamp;
        TU32 counter;
    } seedData;
    seedData.timestamp = gfGetNTPTime();
    static TU32 counter = 0;
    seedData.counter = ++counter;

    TChar *p_nonce = p_client_session->authenticator.nonce;
    gfStandardHashMD5Oneshot((TU8 *)(&seedData), (TU32)sizeof(seedData), p_nonce);

    return EECode_OK;
}

EECode CRTSPStreamingServerV2::computeDigestResponse(SClientSessionInfo *p_client_session, const TChar *cmd, const TChar *url, TChar *&p_result)
{
    //##refer to live555##
    // The "response" field is computed as:
    //    md5(md5(<username>:<realm>:<password>):<nonce>:md5(<cmd>:<url>))
    // or, if "fPasswordIsMD5" is True:
    //    md5(<password>:<nonce>:md5(<cmd>:<url>))
    static TU8 p_data[512];
    static TChar ha1Result[33] = {0};
    TChar *ptmp = NULL;
    TU32 data_length = 0;
    p_result = NULL;
    if (0/*fPasswordIsMD5*/) {
        strncpy(ha1Result, p_client_session->authenticator.password, 32);
        ha1Result[33] = '\0';
    } else {
        data_length = strlen(p_client_session->authenticator.username) + 1 + strlen(p_client_session->authenticator.realm) + 1 + strlen(p_client_session->authenticator.password);
        if (DUnlikely(data_length > 511)) {
            LOG_ERROR("too long username and password, data_length %d\n", data_length);
            return EECode_BadParam;
        }
        sprintf((TChar *)p_data, "%s:%s:%s", p_client_session->authenticator.username, p_client_session->authenticator.realm, p_client_session->authenticator.password);
        ptmp = (TChar *) ha1Result;
        gfStandardHashMD5Oneshot(p_data, data_length, ptmp);
    }

    data_length = strlen(cmd) + 1 + strlen(url);
    if (DUnlikely(data_length > 511)) {
        LOG_ERROR("too long string, data_length %d\n", data_length);
        return EECode_BadParam;
    }
    sprintf((TChar *)p_data, "%s:%s", cmd, url);
    TChar ha2Result[33] = {0};
    ptmp = (TChar *) ha2Result;
    gfStandardHashMD5Oneshot(p_data, data_length, ptmp);

    data_length = 32 + 1 + strlen(p_client_session->authenticator.nonce) + 1 + 32;
    if (DUnlikely(data_length > 511)) {
        LOG_ERROR("too long nonce, data_length %d\n", data_length);
        return EECode_BadParam;
    }
    sprintf((char *)p_data, "%s:%s:%s", ha1Result, p_client_session->authenticator.nonce, ha2Result);

    gfStandardHashMD5Oneshot(p_data, data_length, p_result);
    if (DUnlikely(!p_result)) {
        LOGM_FATAL("computeDigestResponse, gfStandardHashMD5Oneshot last result fail\n");
        return EECode_Error;
    }

    return EECode_OK;
}

EECode CRTSPStreamingServerV2::handleClientRequest(SClientSessionInfo *p_client)
{
    EECode err = EECode_OK;

#define RTSP_PARAM_STRING_MAX   256
    TChar cmdName[RTSP_PARAM_STRING_MAX];
    TChar urlPreSuffix[RTSP_PARAM_STRING_MAX];
    TChar urlSuffix[RTSP_PARAM_STRING_MAX];
    TChar cseq[RTSP_PARAM_STRING_MAX];

    LOGM_DEBUG("get Request:len[%d]\n\t%s\n", mRequestBufferLen, mRequestBuffer);

    mResponseBuffer[0] = '\0';
    if (parseRTSPRequestString((TChar *)mRequestBuffer, mRequestBufferLen,
                               cmdName, sizeof cmdName,
                               urlPreSuffix, sizeof urlPreSuffix,
                               urlSuffix, sizeof urlSuffix,
                               cseq, sizeof cseq)) {

        LOGM_DEBUG("cmdName %s.\n", cmdName);
        LOGM_DEBUG("urlPreSuffix %s.\n", urlPreSuffix);
        LOGM_DEBUG("urlSuffix [%s].\n", urlSuffix);
        LOGM_DEBUG("cseq %s.\n", cseq);
        LOGM_DEBUG("mRequestBuffer %s.\n", mRequestBuffer);

        if (strcmp(cmdName, "OPTIONS") == 0) {
            LOGM_DEBUG("before handleCommandOptions\n");
            handleCommandOptions(cseq);
            LOGM_DEBUG("response(%ld):%s\n", (TPointer)strlen(mResponseBuffer), mResponseBuffer);
            if (gfNet_Send(p_client->tcp_socket, (TU8 *)mResponseBuffer, (TInt)strlen(mResponseBuffer), 0) < 0) {
                LOGM_ERROR("write fail\n");
                return EECode_Closed;
            }
            p_client->last_cmd_time = getCurrentSystemTime();
            return err;
        }
        if (DUnlikely(ESessionServerState_Init == p_client->state)) {
            err = authentication(p_client, (const TChar *)urlSuffix);
            if (DUnlikely(EECode_OK != err)) {
                LOGM_ERROR("authentication %s fail, return %d, %s\n", urlSuffix, err, gfGetErrorCodeString(err));
                switch (err) {
                    case EECode_BadParam: handleCommandNotFound(cseq); break; //url not found
                    case EECode_InternalLogicalBug:
                    case EECode_BadState:
                    default: handleCommandServerInternalError(cseq); break;
                }
                if (gfNet_Send(p_client->tcp_socket, (TU8 *)mResponseBuffer, (TInt)strlen(mResponseBuffer), 0) < 0) {
                    LOGM_FATAL("write fail\n");
                    return EECode_Closed;
                }
                return err;
            }
            if (mpPersistMediaConfig->rtsp_server_config.enable_nonblock_timeout) {
                gfSocketSetTimeout(p_client->tcp_socket, DPresetSocketTimeOutUintSeconds, DPresetSocketTimeOutUintUSeconds);
            }
            p_client->state = ESessionServerState_authenticationDone;
        }
        if (strcmp(cmdName, "DESCRIBE") == 0) {
            LOGM_DEBUG("before handleCommandDescribe.\n");
            handleCommandDescribe(p_client, cseq, urlSuffix, (TChar const *)mRequestBuffer);
            if ((!strstr(mResponseBuffer, "RTSP/1.0 200 OK")) && (!strstr(mResponseBuffer, "RTSP/1.0 401 Unauthorized"))) {
                LOGM_ERROR("error DESCRIBE responce %s\n", mResponseBuffer);
                err = EECode_Error;
            }
        } else if (strcmp(cmdName, "SETUP") == 0) {
            LOGM_DEBUG("before handleCommandSetup\n");
            handleCommandSetup(p_client, cseq, urlPreSuffix, urlSuffix, (TChar const *)mRequestBuffer);
            if (DUnlikely(!strstr(mResponseBuffer, "RTSP/1.0 200 OK"))) {
                LOGM_ERROR("error SETUP responce %s\n", mResponseBuffer);
                err = EECode_Error;
            } else {
                p_client->state = ESessionServerState_Ready;
            }
        } else if (strcmp(cmdName, "TEARDOWN") == 0
                   || strcmp(cmdName, "PLAY") == 0
                   || strcmp(cmdName, "PAUSE") == 0
                   || strcmp(cmdName, "GET_PARAMETER") == 0
                   || strcmp(cmdName, "SET_PARAMETER") == 0) {
            //for withSession case, response TInt handleCmd
            LOGM_DEBUG("before handleCommandInSession, cmdName %s\n", cmdName);
            err = handleCommandInSession(p_client, cmdName, urlPreSuffix, urlSuffix, cseq, (TChar const *)mRequestBuffer);
            p_client->last_cmd_time = getCurrentSystemTime();
            return err;
        } else {
            handleCommandNotSupported(cseq);
            err = EECode_NotSupported;
        }
        p_client->last_cmd_time = getCurrentSystemTime();
    } else {
        LOGM_ERROR("parseRTSPRequestString() failed\n");
        handleCommandBad(cseq);
        err = EECode_BadParam;
    }

    LOGM_DEBUG("response(%ld):%s\n", (TPointer)strlen(mResponseBuffer), mResponseBuffer);

    if (gfNet_Send(p_client->tcp_socket, (TU8 *)mResponseBuffer, (TInt)strlen(mResponseBuffer), 0) < 0) {
        LOGM_FATAL("write fail\n");
        return EECode_Closed;
    }

    memset(mResponseBuffer, 0x0, sizeof(mResponseBuffer));
    return err;
}

CRTSPStreamingServerV2::CRTSPStreamingServerV2(const TChar *name, StreamingServerType type, StreamingServerMode mode, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink, TU16 server_port, TU8 enable_video, TU8 enable_audio)
    : inherited(name, 0)
    , mpPersistMediaConfig(pPersistMediaConfig)
    , mpMsgSink(pMsgSink)
    , mpMutex(NULL)
    , mId(0)
    , mIndex(0)
    , mbEnableVideo(enable_video)
    , mbEnableAudio(enable_audio)
    , mRtspSessionId(0)
    , mListeningPort(server_port)
    , mDataPortBase(DefaultRTPServerPortBase)
    , mType(type)
    , mMode(mode)
    , msState(EServerState_closed)
    , mRtspSSRC(0x12436587)
    , mSocket(-1)
    , mpRtspUrl(NULL)
    , mRtspUrlLength(0)
{
    if (DLikely(mpPersistMediaConfig)) {
        mpSystemClockReference = mpPersistMediaConfig->p_system_clock_reference;
    } else {
        mpSystemClockReference = NULL;
        LOGM_FATAL("NULL mpPersistMediaConfig\n");
    }

    mpClientSessionList = NULL;
    mpContentList = NULL;
    mpVodContentList = NULL;
    mpSDPSessionList = NULL;
}

CRTSPStreamingServerV2::~CRTSPStreamingServerV2()
{

}

IStreamingServer *CRTSPStreamingServerV2::Create(const TChar *name, StreamingServerType type, StreamingServerMode mode, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink, TU16 server_port, TU8 enable_video, TU8 enable_audio)
{
    CRTSPStreamingServerV2 *result = new CRTSPStreamingServerV2(name, type, mode, pPersistMediaConfig, pMsgSink, server_port, enable_video, enable_audio);
    if (result && result->Construct() != EECode_OK) {
        delete result;
        result = NULL;
    }
    return result;
}

CObject *CRTSPStreamingServerV2::GetObject0() const
{
    return (CObject *) this;
}

EECode CRTSPStreamingServerV2::Construct()
{
    DSET_MODULE_LOG_CONFIG(ELogModuleRTSPStreamingServer);
    //mConfigLogLevel = ELogLevel_Debug;
    //mConfigLogOption = 0xffffffff;

    mpMutex = gfCreateMutex();
    if (DUnlikely(!mpMutex)) {
        LOGM_FATAL("No Memory\n");
        return EECode_NoMemory;
    }

    mSocket = gfNet_SetupStreamSocket(INADDR_ANY, mListeningPort, 1);
    if (DUnlikely(0 > mSocket)) {
        LOGM_FATAL("setup socket fail, ret %d.\n", mSocket);
        return EECode_Error;
    }
    LOGM_INFO("gfNet_SetupStreamSocket, socket %d.\n", mSocket);

    gfSocketSetDeferAccept(mSocket, 2);

    if (mpPersistMediaConfig) {
        mDataPortBase = mpPersistMediaConfig->rtsp_server_config.rtp_rtcp_port_start;
    } else {
        mDataPortBase = DefaultRTPServerPortBase;
    }

    mpClientSessionList = new CIDoubleLinkedList();
    if (DUnlikely(!mpClientSessionList)) {
        LOGM_FATAL("new CIDoubleLinkedList() fail.\n");
        return EECode_NoMemory;
    }

    mpContentList = new CIDoubleLinkedList();
    if (DUnlikely(!mpContentList)) {
        LOGM_FATAL("new CIDoubleLinkedList() fail.\n");
        return EECode_NoMemory;
    }

    mpVodContentList = new CIDoubleLinkedList();
    if (DUnlikely(!mpVodContentList)) {
        LOGM_FATAL("new CIDoubleLinkedList() fail.\n");
        return EECode_NoMemory;
    }

    mpSDPSessionList = new CIDoubleLinkedList();
    if (DUnlikely(!mpSDPSessionList)) {
        LOGM_FATAL("new CIDoubleLinkedList() fail.\n");
        return EECode_NoMemory;
    }

    msState = EServerState_running;

    return EECode_OK;
}

void CRTSPStreamingServerV2::deleteClientSession(SClientSessionInfo *p_session)
{
    SStreamingTransmiterCombo combo;

    if (!p_session) {
        return;
    }

    DASSERT(p_session);
    DASSERT(p_session->p_server == ((IStreamingServer *)this));

    if (!mpClientSessionList->IsContentInList(p_session)) {
        if (p_session->p_sdp_info) {
            DDBG_FREE(p_session->p_sdp_info, "RSDP");
            p_session->p_sdp_info = NULL;
        }

        gfNetCloseSocket(p_session->tcp_socket);
        DDBG_FREE(p_session, "RCSN");
        return;
    }

    if (DLikely(((p_session->p_streaming_transmiter_filter) || (p_session->vod_mode == 1)) && (p_session->add_to_transmiter))) {
        p_session->add_to_transmiter = 0;

        SVODPipeline *p_pipeline = NULL;
        if (p_session->vod_mode == 1) {
            p_pipeline = (SVODPipeline *)p_session->p_pipeline;
            p_pipeline->p_video_source[0]->p_filter->Pause();
            p_pipeline->p_flow_controller->p_filter->FilterProperty(EFilterPropertyType_flow_controller_stop, 1, NULL);
        }

        if (p_session->enable_video) {
            combo.p_transmiter = (IStreamingTransmiter *)p_session->p_content->sub_session_content[ESubSession_video]->p_transmiter;
            combo.input_index = p_session->sub_session[ESubSession_video].p_content->transmiter_input_pin_index;
            combo.p_session = p_session;
            combo.p_sub_session = &p_session->sub_session[ESubSession_video];
            p_session->p_streaming_transmiter_filter->FilterProperty(EFilterPropertyType_streaming_remove_subsession, 1, (void *) &combo);
        }

        if (p_session->enable_audio) {
            combo.p_transmiter = (IStreamingTransmiter *)p_session->p_content->sub_session_content[ESubSession_audio]->p_transmiter;
            combo.input_index = p_session->sub_session[ESubSession_audio].p_content->transmiter_input_pin_index;
            combo.p_session = p_session;
            combo.p_sub_session = &p_session->sub_session[ESubSession_audio];
            p_session->p_streaming_transmiter_filter->FilterProperty(EFilterPropertyType_streaming_remove_subsession, 1, (void *) &combo);
        }

        if (p_session->vod_mode == 1) {
            p_pipeline->p_flow_controller->p_filter->Flush();
            TU8 type = (p_session->vod_mode == 1) ? 1 : 0;
            mpPersistMediaConfig->p_engine_v3->FreePipeline(type, p_session->p_pipeline);
            p_session->p_pipeline = NULL;
            p_session->speed_combo = 8;
        }
    }

    mpClientSessionList->RemoveContent(p_session);

    if (p_session->p_sdp_info) {
        DDBG_FREE(p_session->p_sdp_info, "RSDP");
        p_session->p_sdp_info = NULL;
    }

    gfNetCloseSocket(p_session->tcp_socket);
    DDBG_FREE(p_session, "RCSN");
}

void CRTSPStreamingServerV2::removeClientSession(SClientSessionInfo *p_session)
{
    SStreamingTransmiterCombo combo;

    if (!p_session) {
        return;
    }

    if (!mpClientSessionList->IsContentInList(p_session)) {
        return;
    }

    if (DLikely(((p_session->p_streaming_transmiter_filter) || (p_session->vod_mode == 1)) && (p_session->add_to_transmiter))) {
        p_session->add_to_transmiter = 0;

        SVODPipeline *p_pipeline = NULL;
        if (p_session->vod_mode == 1) {
            p_pipeline = (SVODPipeline *)p_session->p_pipeline;
            p_pipeline->p_video_source[0]->p_filter->Pause();
            p_pipeline->p_flow_controller->p_filter->FilterProperty(EFilterPropertyType_flow_controller_stop, 1, NULL);
        }

        if (p_session->enable_video) {
            combo.p_transmiter = (IStreamingTransmiter *)p_session->p_content->sub_session_content[ESubSession_video]->p_transmiter;
            combo.input_index = p_session->sub_session[ESubSession_video].p_content->transmiter_input_pin_index;
            combo.p_session = p_session;
            combo.p_sub_session = &p_session->sub_session[ESubSession_video];
            p_session->p_streaming_transmiter_filter->FilterProperty(EFilterPropertyType_streaming_remove_subsession, 1, (void *) &combo);
        }

        if (p_session->enable_audio) {
            combo.p_transmiter = (IStreamingTransmiter *)p_session->p_content->sub_session_content[ESubSession_audio]->p_transmiter;
            combo.input_index = p_session->sub_session[ESubSession_audio].p_content->transmiter_input_pin_index;
            combo.p_session = p_session;
            combo.p_sub_session = &p_session->sub_session[ESubSession_audio];
            p_session->p_streaming_transmiter_filter->FilterProperty(EFilterPropertyType_streaming_remove_subsession, 1, (void *) &combo);
        }

        if (p_session->vod_mode == 1) {
            p_pipeline->p_flow_controller->p_filter->Flush();
            TU8 type = (p_session->vod_mode == 1) ? 1 : 0;
            mpPersistMediaConfig->p_engine_v3->FreePipeline(type, p_session->p_pipeline);
            p_session->p_pipeline = NULL;
            p_session->speed_combo = 8;
        }
    }

    mpClientSessionList->RemoveContent(p_session);
}

void CRTSPStreamingServerV2::deleteSDPSession(void *)
{
    //todo
    DASSERT(0);
}

void CRTSPStreamingServerV2::Delete()
{
    CIDoubleLinkedList::SNode *pnode;
    if (mpMutex) {
        mpMutex->Delete();
        mpMutex = NULL;
    }

    if (mpClientSessionList) {
        SClientSessionInfo *p_client;
        pnode = mpClientSessionList->FirstNode();
        while (pnode) {
            p_client = (SClientSessionInfo *)(pnode->p_context);
            pnode = mpClientSessionList->NextNode(pnode);
            DASSERT(p_client);
            if (p_client) {
                deleteClientSession(p_client);
            } else {
                DASSERT("NULL pointer here, something would be wrong.\n");
            }
        }
    }

    if (DIsSocketHandlerValid(mSocket)) {
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
    }

    if (mpRtspUrl) {
        DDBG_FREE(mpRtspUrl, "SVUL");
        mpRtspUrl = NULL;
    }

    if (mpClientSessionList) {
        delete mpClientSessionList;
        mpClientSessionList = NULL;
    }

    if (mpContentList) {
        delete mpContentList;
        mpContentList = NULL;
    }

    if (mpVodContentList) {
        delete mpVodContentList;
        mpVodContentList = NULL;
    }

    if (mpSDPSessionList) {
        delete mpSDPSessionList;
        mpSDPSessionList = NULL;
    }

}

void CRTSPStreamingServerV2::PrintStatus()
{
    if (!(mpPersistMediaConfig->runtime_print_mask & DLOG_MASK_TO_REMOTE)) {
        LOGM_PRINTF("heart beat %d\n", mDebugHeartBeat);
    }
    mDebugHeartBeat = 0;

    CIDoubleLinkedList::SNode *pnode = NULL;
    SClientSessionInfo *p_client = NULL;

    CIRemoteLogServer *p_log_server = mpPersistMediaConfig->p_remote_log_server;
    if (DLikely((p_log_server) && (mpPersistMediaConfig->runtime_print_mask & DLOG_MASK_TO_REMOTE))) {
        p_log_server->WriteLog("\033[40;35m\033[1m\n\tclients to rtsp server:\n\033[0m");
    }
    TU32 print_count = 0;

    pnode = mpClientSessionList->FirstNode();
    while (pnode) {
        p_client = (SClientSessionInfo *)(pnode->p_context);
        pnode = mpClientSessionList->NextNode(pnode);
        if (DLikely(p_client)) {
            if (DLikely(p_client->p_content)) {
                if (!(mpPersistMediaConfig->runtime_print_mask & DLOG_MASK_TO_REMOTE)) {
                    LOGM_PRINTF("connected client: %s, port %d, socket %d, content index %d, url %s.\n", inet_ntoa(p_client->client_addr.sin_addr), ntohs(p_client->client_addr.sin_port), p_client->tcp_socket, p_client->p_content->content_index, p_client->p_content->session_name);
                }

                if (DLikely((p_log_server) && (mpPersistMediaConfig->runtime_print_mask & DLOG_MASK_TO_REMOTE))) {
                    if (!((print_count + 1) % 5)) {
                        p_log_server->WriteLog("%s:%d,%d,%s\n", inet_ntoa(p_client->client_addr.sin_addr), ntohs(p_client->client_addr.sin_port), p_client->p_content->content_index, p_client->p_content->session_name);
                    } else {
                        p_log_server->WriteLog("%s:%d,%d,%s\t", inet_ntoa(p_client->client_addr.sin_addr), ntohs(p_client->client_addr.sin_port), p_client->p_content->content_index, p_client->p_content->session_name);
                    }
                    print_count ++;
                }
            } else {
                if (!(mpPersistMediaConfig->runtime_print_mask & DLOG_MASK_TO_REMOTE)) {
                    LOGM_PRINTF("connected client: %s, port %d, socket %d, no content\n", inet_ntoa(p_client->client_addr.sin_addr), ntohs(p_client->client_addr.sin_port), p_client->tcp_socket);
                }

                if (DLikely((p_log_server) && (mpPersistMediaConfig->runtime_print_mask & DLOG_MASK_TO_REMOTE))) {
                    if (!((print_count + 1) & 0x3)) {
                        p_log_server->WriteLog("\033[40;31m\033[1m%s:%d,connecting?\n\033[0m", inet_ntoa(p_client->client_addr.sin_addr), ntohs(p_client->client_addr.sin_port));
                    } else {
                        p_log_server->WriteLog("\033[40;31m\033[1m%s:%d,connecting?\t\033[0m", inet_ntoa(p_client->client_addr.sin_addr), ntohs(p_client->client_addr.sin_port));
                    }
                    print_count ++;
                }
            }
        } else {
            LOGM_ERROR("NULL pointer here, something would be wrong.\n");
        }
    }

}

void CRTSPStreamingServerV2::ScanClientList(TInt &nready, void *read_set, void *all_set)
{
    CIDoubleLinkedList::SNode *pnode;
    SClientSessionInfo *p_client;
    fd_set *n_all_set = (fd_set *)all_set;
    fd_set *n_read_set = (fd_set *)read_set;

    mDebugHeartBeat ++;

    pnode = mpClientSessionList->FirstNode();
    while (pnode) {
        p_client = (SClientSessionInfo *)(pnode->p_context);
        pnode = mpClientSessionList->NextNode(pnode);
        if (DLikely(p_client)) {
            //process client
            if (FD_ISSET(p_client->tcp_socket, n_read_set)) {
                if (EECode_Closed == handleRtspRequest(p_client)) {
                    FD_CLR(p_client->tcp_socket, n_read_set);
                    FD_CLR(p_client->tcp_socket, n_all_set);
                    deleteClientSession(p_client);
                }
                nready--;
            }
        } else {
            LOGM_ERROR("NULL pointer here, something would be wrong.\n");
        }

        if (nready <= 0) {
            //read done
            break;
        }
    }

}

EECode CRTSPStreamingServerV2::AddStreamingContent(SStreamingSessionContent *content)
{
    TInt audio_enabled = 0, video_enabled = 0;

    LOGM_INFO("ECMDType_AddContent, content index %d, id 0x%08x, %s\n", content->content_index, content->id, content->session_name);
    AUTO_LOCK(mpMutex);

    mpContentList->InsertContent(NULL, (void *) content, 0);

    if (DLikely(mbEnableVideo && content->sub_session_content[ESubSession_video] && content->sub_session_content[ESubSession_video]->enabled)) {
        if (!content->sub_session_content[ESubSession_video]->server_rtp_port) {
            content->sub_session_content[ESubSession_video]->server_rtp_port = mDataPortBase;
            content->sub_session_content[ESubSession_video]->server_rtcp_port = mDataPortBase + 1;
            mDataPortBase += 2;
        }
        video_enabled = 1;
    } else {
        if (content->sub_session_content[ESubSession_video]) {
            content->sub_session_content[ESubSession_video]->enabled = 0;
        }
    }

    if (DLikely(mbEnableAudio && content->sub_session_content[ESubSession_audio] && content->sub_session_content[ESubSession_audio]->enabled)) {
        if (!content->sub_session_content[ESubSession_audio]->server_rtp_port) {
            content->sub_session_content[ESubSession_audio]->server_rtp_port = mDataPortBase;
            content->sub_session_content[ESubSession_audio]->server_rtcp_port = mDataPortBase + 1;
            mDataPortBase += 2;
        }
        audio_enabled = 1;
    } else {
        if (content->sub_session_content[ESubSession_audio]) {
            content->sub_session_content[ESubSession_audio]->enabled = 0;
        }
    }

    LOGM_INFO("ECMDType_AddContent end, content index %d, id 0x%08x, %s, audio enabled %d, video enabled %d\n", content->content_index, content->id, content->session_name, audio_enabled, video_enabled);

    DASSERT(audio_enabled || video_enabled);
    return EECode_OK;
}

void CRTSPStreamingServerV2::handleCommandBad(TChar const *cseq)
{
    gfGetDateString(mDateBuffer, sizeof(mDateBuffer));
    snprintf((TChar *)mResponseBuffer, sizeof(mResponseBuffer), "RTSP/1.0 400 Bad Request\r\nServer: %s\r\n%sAllow: %s\r\n\r\n", DSRTING_RTSP_SERVER_TAG, mDateBuffer, _RTSPallowedCommandNames);
}

void CRTSPStreamingServerV2::handleCommandNotSupported(TChar const *cseq)
{
    gfGetDateString(mDateBuffer, sizeof(mDateBuffer));
    snprintf((TChar *)mResponseBuffer, sizeof(mResponseBuffer), "RTSP/1.0 405 Method Not Allowed\r\nServer: %s\r\nCSeq: %s\r\n%sAllow: %s\r\n\r\n", DSRTING_RTSP_SERVER_TAG, cseq, mDateBuffer, _RTSPallowedCommandNames);
}

void CRTSPStreamingServerV2::handleCommandNotFound(TChar const *cseq)
{
    gfGetDateString(mDateBuffer, sizeof(mDateBuffer));
    snprintf((TChar *)mResponseBuffer, sizeof(mResponseBuffer), "RTSP/1.0 404 Stream Not Found\r\nServer: %s\r\nCSeq: %s\r\n%s\r\n", DSRTING_RTSP_SERVER_TAG, cseq, mDateBuffer);
}

void CRTSPStreamingServerV2::handleCommandUnsupportedTransport(TChar const *cseq)
{
    gfGetDateString(mDateBuffer, sizeof(mDateBuffer));
    snprintf((TChar *)mResponseBuffer, sizeof(mResponseBuffer), "RTSP/1.0 461 Unsupported Transport\r\nServer: %s\r\nCSeq: %s\r\n%s\r\n", DSRTING_RTSP_SERVER_TAG, cseq, mDateBuffer);
}

void CRTSPStreamingServerV2::handleCommandServerInternalError(TChar const *cseq)
{
    gfGetDateString(mDateBuffer, sizeof(mDateBuffer));
    snprintf((TChar *)mResponseBuffer, sizeof(mResponseBuffer), "RTSP/1.0 500 Internal Server Error\r\nServer: %s\r\nCSeq: %s\r\n%s\r\n", DSRTING_RTSP_SERVER_TAG, cseq, mDateBuffer);
}

void CRTSPStreamingServerV2::handleCommandOptions(TChar const *cseq)
{
    gfGetDateString(mDateBuffer, sizeof(mDateBuffer));
    snprintf((TChar *)mResponseBuffer, sizeof(mResponseBuffer), "RTSP/1.0 200 OK\r\nServer: %s\r\nCSeq: %s\r\n%sPublic: %s\r\n\r\n", DSRTING_RTSP_SERVER_TAG, cseq, mDateBuffer, _RTSPallowedCommandNames);
}

void CRTSPStreamingServerV2::handleCommandDescribe(SClientSessionInfo *p_client, TChar const *cseq, TChar const *urlSuffix, TChar const *fullRequestStr)
{
    TChar rtspURL[128];

    DASSERT(p_client);
    DASSERT(p_client->p_session_name);
    LOGM_DEBUG("p_client->p_session_name %p.\n", p_client->p_session_name);
    gfGetDateString(mDateBuffer, sizeof(mDateBuffer));

    if (!p_client->get_host_string) {
        retrieveHostString((TChar *)fullRequestStr, (TChar *)p_client->host_addr, sizeof(p_client->host_addr));
        p_client->get_host_string = 1;
    }

    snprintf(rtspURL, sizeof rtspURL, "rtsp://%s:%d/%s", p_client->host_addr, p_client->server_port, p_client->p_session_name);

    if (DUnlikely(authenticationUser(p_client, "DESCRIBE", cseq, fullRequestStr)) != EECode_OK) {
        LOGM_WARN("handleCommandDescribe, authenticationUser fail\n");
        return;
    }

    if (!p_client->p_sdp_info) {
        if (DUnlikely(!p_client->p_content)) {
            LOG_FATAL("NULL p_client->p_content, please check code\n");
            return;
        }

        TUint has_video_sdp = 1;
        TUint has_audio_sdp = 1;
        if (!p_client->enable_video) {
            has_video_sdp = 0;
        } else if (!p_client->p_content->sub_session_content[ESubSession_video]->parameters_setup) {
            has_video_sdp = 0;
        }

        if (!p_client->enable_audio) {
            has_audio_sdp = 0;
        } else if (!p_client->p_content->sub_session_content[ESubSession_audio]->parameters_setup) {
            has_audio_sdp = 0;
        }

        LOGM_DEBUG("p_client->enable_video %d, p_client->enable_audio %d, has_video_sdp %d, has_audio_sdp %d\n", p_client->enable_video, p_client->enable_audio, has_video_sdp, has_audio_sdp);

        if (has_video_sdp && has_audio_sdp) {
            if (StreamFormat_AAC == p_client->p_content->sub_session_content[ESubSession_audio]->format) {
                p_client->p_sdp_info = generate_sdp_description_h264_aac_sps_params(p_client->p_session_name, p_client->p_session_name, DSRTING_RTSP_SERVER_SDP_TAG, (const TChar *)p_client->host_addr, rtspURL, 0, 0, \
                                       RTP_PT_H264, \
                                       p_client->p_content->sub_session_content[ESubSession_video]->video_width, \
                                       p_client->p_content->sub_session_content[ESubSession_video]->video_height, \
                                       p_client->p_content->sub_session_content[ESubSession_video]->video_framerate, \
                                       RTP_PT_AAC, \
                                       p_client->p_content->sub_session_content[ESubSession_audio]->audio_sample_rate, \
                                       p_client->p_content->sub_session_content[ESubSession_audio]->audio_channel_number, \
                                       p_client->p_content->sub_session_content[ESubSession_video]->profile_level_id_base16, \
                                       p_client->p_content->sub_session_content[ESubSession_video]->sps_base64, \
                                       p_client->p_content->sub_session_content[ESubSession_video]->pps_base64, \
                                       p_client->p_content->sub_session_content[ESubSession_audio]->aac_config_base16, \
                                       p_client->vod_content_length);
            } else if (StreamFormat_PCMU == p_client->p_content->sub_session_content[ESubSession_audio]->format) {
                p_client->p_sdp_info = generate_sdp_description_h264_pcm_g711_mu_sps_params(p_client->p_session_name, p_client->p_session_name, DSRTING_RTSP_SERVER_SDP_TAG, (const TChar *)p_client->host_addr, rtspURL, 0, 0, \
                                       RTP_PT_H264, \
                                       p_client->p_content->sub_session_content[ESubSession_video]->video_width, \
                                       p_client->p_content->sub_session_content[ESubSession_video]->video_height, \
                                       p_client->p_content->sub_session_content[ESubSession_video]->video_framerate, \
                                       p_client->p_content->sub_session_content[ESubSession_video]->profile_level_id_base16, \
                                       p_client->p_content->sub_session_content[ESubSession_video]->sps_base64, \
                                       p_client->p_content->sub_session_content[ESubSession_video]->pps_base64, \
                                       p_client->vod_content_length);
            } else if (StreamFormat_PCMA == p_client->p_content->sub_session_content[ESubSession_audio]->format) {
                p_client->p_sdp_info = generate_sdp_description_h264_pcm_g711_a_sps_params(p_client->p_session_name, p_client->p_session_name, DSRTING_RTSP_SERVER_SDP_TAG, (const TChar *)p_client->host_addr, rtspURL, 0, 0, \
                                       RTP_PT_H264, \
                                       p_client->p_content->sub_session_content[ESubSession_video]->video_width, \
                                       p_client->p_content->sub_session_content[ESubSession_video]->video_height, \
                                       p_client->p_content->sub_session_content[ESubSession_video]->video_framerate, \
                                       p_client->p_content->sub_session_content[ESubSession_video]->profile_level_id_base16, \
                                       p_client->p_content->sub_session_content[ESubSession_video]->sps_base64, \
                                       p_client->p_content->sub_session_content[ESubSession_video]->pps_base64, \
                                       p_client->vod_content_length);
            } else {
                LOGM_ERROR("[RTSP server, sdp]: not supported audio format %d, sub session %p, TODO\n", p_client->p_content->sub_session_content[ESubSession_audio]->format, &p_client->p_content->sub_session_content[ESubSession_audio]);
                p_client->enable_audio = 0;
                p_client->p_sdp_info = generate_sdp_description_h264_sps_params(p_client->p_session_name, p_client->p_session_name, DSRTING_RTSP_SERVER_SDP_TAG, (const TChar *)p_client->host_addr, rtspURL, 0, 0, \
                                       RTP_PT_H264, \
                                       p_client->p_content->sub_session_content[ESubSession_video]->video_width, \
                                       p_client->p_content->sub_session_content[ESubSession_video]->video_height, \
                                       p_client->p_content->sub_session_content[ESubSession_video]->video_framerate, \
                                       p_client->p_content->sub_session_content[ESubSession_video]->profile_level_id_base16, \
                                       p_client->p_content->sub_session_content[ESubSession_video]->sps_base64, \
                                       p_client->p_content->sub_session_content[ESubSession_video]->pps_base64, \
                                       p_client->vod_content_length);
            }
            p_client->master_index = ESubSession_video;
        } else if (has_video_sdp) {
            p_client->p_sdp_info = generate_sdp_description_h264_sps_params(p_client->p_session_name, p_client->p_session_name, DSRTING_RTSP_SERVER_SDP_TAG, (const TChar *)p_client->host_addr, rtspURL, 0, 0, \
                                   RTP_PT_H264, \
                                   p_client->p_content->sub_session_content[ESubSession_video]->video_width, \
                                   p_client->p_content->sub_session_content[ESubSession_video]->video_height, \
                                   p_client->p_content->sub_session_content[ESubSession_video]->video_framerate, \
                                   p_client->p_content->sub_session_content[ESubSession_video]->profile_level_id_base16, \
                                   p_client->p_content->sub_session_content[ESubSession_video]->sps_base64, \
                                   p_client->p_content->sub_session_content[ESubSession_video]->pps_base64, \
                                   p_client->vod_content_length);
            p_client->master_index = ESubSession_video;
        } else if (has_audio_sdp) {
            if (StreamFormat_AAC == p_client->p_content->sub_session_content[ESubSession_audio]->format) {
                p_client->p_sdp_info = generate_sdp_description_aac(p_client->p_session_name, p_client->p_session_name, DSRTING_RTSP_SERVER_SDP_TAG, (const TChar *)p_client->host_addr, rtspURL, 0, 0, \
                                       RTP_PT_AAC, \
                                       p_client->p_content->sub_session_content[ESubSession_audio]->audio_sample_rate, \
                                       p_client->p_content->sub_session_content[ESubSession_audio]->audio_channel_number, \
                                       p_client->p_content->sub_session_content[ESubSession_audio]->aac_config_base16, \
                                       p_client->vod_content_length);
            } else if (StreamFormat_PCMU == p_client->p_content->sub_session_content[ESubSession_audio]->format) {
                p_client->p_sdp_info = generate_sdp_description_pcm_g711_mu(p_client->p_session_name, p_client->p_session_name, DSRTING_RTSP_SERVER_SDP_TAG, (const TChar *)p_client->host_addr, rtspURL, 0, 0, p_client->vod_content_length);
            } else if (StreamFormat_PCMA == p_client->p_content->sub_session_content[ESubSession_audio]->format) {
                p_client->p_sdp_info = generate_sdp_description_pcm_g711_a(p_client->p_session_name, p_client->p_session_name, DSRTING_RTSP_SERVER_SDP_TAG, (const TChar *)p_client->host_addr, rtspURL, 0, 0, p_client->vod_content_length);
            } else {
                LOGM_ERROR("[RTSP server, sdp]: not supported audio format %d, TODO\n", p_client->p_content->sub_session_content[ESubSession_audio]->format);
                p_client->enable_audio = 0;
                return;
            }
            p_client->master_index = ESubSession_audio;
        } else {
            LOGM_ERROR("!!!video audio are not enabled or not ready?\n");
            return;
        }

        p_client->sdp_info_len = strlen(p_client->p_sdp_info);
    } else {
        LOGM_WARN("Already have SDP string.\n");
    }

    LOGM_DEBUG(" SDP %d:%s\n", p_client->sdp_info_len, p_client->p_sdp_info);
    LOGM_DEBUG(" p_client->p_session_name %s.\n", p_client->p_session_name);

    snprintf((TChar *)mResponseBuffer, sizeof(mResponseBuffer),
             "RTSP/1.0 200 OK\r\nServer: %s\r\nCSeq: %s\r\n"
             "%s"
             "Content-Base: %s/\r\n"
             "Content-Type: application/sdp\r\n"
             "Content-Length: %d\r\n\r\n"
             "%s",
             DSRTING_RTSP_SERVER_TAG,
             cseq,
             mDateBuffer,
             rtspURL,
             p_client->sdp_info_len,
             p_client->p_sdp_info);
}

void CRTSPStreamingServerV2::handleCommandSetup(SClientSessionInfo *p_client, TChar const *cseq,
        TChar const *urlPreSuffix, TChar const *urlSuffix,
        TChar const *fullRequestStr)
{
    TUint track_id = 0;

    gfGetDateString(mDateBuffer, sizeof(mDateBuffer));
    ProtocolType streamingMode;
    TChar streamingModeString[16]; // set when RAW_UDP streaming is specified
    TChar clientsDestinationAddressStr[16];
    TChar clientsDestinationTTL;
    TU16 clientRTPPort, clientRTCPPort;
    TChar rtpChannelId, rtcpChannelId;

    parseTransportHeader(fullRequestStr, streamingMode, streamingModeString,
                         clientsDestinationAddressStr, &clientsDestinationTTL,
                         &clientRTPPort, &clientRTCPPort,
                         &rtpChannelId, &rtcpChannelId);

    if (urlSuffix) {
        //LOGM_DEBUG("before getTrackID, url %s.\n", urlSuffix);
        track_id = getTrackID((TChar *)urlSuffix, (TChar *)urlSuffix + (strlen(urlSuffix) - 4));
        //LOGM_DEBUG("after getTrackID, get track id %d.\n", track_id);
    } else {
        LOGM_ERROR("NULL urlSuffix, please check code.\n");
    }

    //check track id
    if (track_id >= ESubSession_tot_count) {
        LOGM_ERROR("BAD track id %d.\n", track_id);
        if (urlSuffix) {
            LOGM_ERROR("urlSuffix: %s.\n", urlSuffix);
        }
        track_id = 0;
    }

    //LOGM_DEBUG("[rtsp setup]: track id %d, track type %d.\n", track_id, track_type);
    if (!p_client->get_host_string) {
        retrieveHostString((TChar *)fullRequestStr, p_client->host_addr, sizeof(p_client->host_addr));
        p_client->get_host_string = 1;
    }
    //LOGM_DEBUG("get host string %s.\n", p_client->host_addr);

    TU16 sessionId;
    if (!parseSessionHeader(fullRequestStr, sessionId)) {
        p_client->session_id = mRtspSessionId++;
    } else {
        p_client->session_id = sessionId;
    }

    p_client->sub_session[track_id].rtp_ssrc = gfRandom32();
    p_client->rtp_over_rtsp = (streamingMode == ProtocolType_TCP);
    if (streamingMode == ProtocolType_UDP) {
        p_client->sub_session[track_id].rtp_over_rtsp  = 0;
        p_client->sub_session[track_id].client_rtp_port = clientRTPPort;
        p_client->sub_session[track_id].client_rtcp_port = clientRTCPPort;

        DASSERT(track_id < 2);

        // Fill in the response:
        snprintf((TChar *)mResponseBuffer, sizeof(mResponseBuffer),
                 "RTSP/1.0 200 OK\r\n"
                 "Server: %s\r\n"
                 "CSeq: %s\r\n"
                 "%s"
                 "Transport: RTP/AVP/UDP;unicast;destination=%s;source=%s;client_port=%d-%d;server_port=%d-%d;ssrc=%08X\r\n"
                 "Session: %08X\r\n\r\n",
                 DSRTING_RTSP_SERVER_TAG,
                 cseq,
                 mDateBuffer,
                 inet_ntoa(p_client->client_addr.sin_addr), p_client->host_addr, clientRTPPort, clientRTCPPort,
                 p_client->sub_session[track_id].server_rtp_port, p_client->sub_session[track_id].server_rtcp_port,
                 p_client->sub_session[track_id].rtp_ssrc,
                 p_client->session_id);
    } else {
        //rtp over rtsp
        p_client->sub_session[track_id].rtp_over_rtsp  = 1;
        p_client->sub_session[track_id].rtp_channel = rtpChannelId;
        p_client->sub_session[track_id].rtcp_channel = rtcpChannelId;
        p_client->sub_session[track_id].rtsp_fd = p_client->tcp_socket;
        // Fill in the response:
        snprintf((TChar *)mResponseBuffer, sizeof(mResponseBuffer),
                 "RTSP/1.0 200 OK\r\n"
                 "Server: %s\r\n"
                 "CSeq: %s\r\n"
                 "%s"
                 "Transport: RTP/AVP/TCP;unicast;interleaved=%d-%d;ssrc=%08X\r\n"
                 "Session: %08X\r\n\r\n",
                 DSRTING_RTSP_SERVER_TAG,
                 cseq,
                 mDateBuffer,
                 rtpChannelId, rtcpChannelId,
                 p_client->sub_session[track_id].rtp_ssrc,
                 p_client->session_id);
    }
}

TInt CRTSPStreamingServerV2::handleCommandPlay(SClientSessionInfo *p_client, TChar const *cseq, TChar const *fullRequestStr)
{
    TUint i;
    TInt ret = 0;

    if (!p_client->get_host_string) {
        retrieveHostString((TChar *)fullRequestStr, (TChar *)p_client->host_addr, sizeof(p_client->host_addr));
        p_client->get_host_string = 1;
    }

    if (!mpRtspUrl) {
        mRtspUrlLength = 128;
        mpRtspUrl = (TChar *)DDBG_MALLOC(mRtspUrlLength, "SVUL");
        snprintf(mpRtspUrl, mRtspUrlLength, "rtsp://%s:%hu", p_client->host_addr, mListeningPort);
    } else {
        LOGM_INFO("Already have rtsp url.\n");
    }

    //LOGM_DEBUG("server_info->p_rtsp_url(%d):%s.\n", mRtspUrlLength, mpRtspUrl);

    gfGetDateString(mDateBuffer, sizeof(mDateBuffer));

    TChar scaleHeader[32] = {0};
    snprintf(scaleHeader, 32, "Scale: %f\r\n", (float)p_client->speed_combo / 8);

    TChar rangeHeader[128] = {0};
    if (p_client->vod_has_end) {
        snprintf(rangeHeader, 128, "Range: npt=%f-%f\r\n", (float)p_client->current_position, (float)p_client->vod_content_length);
    } else {
        snprintf(rangeHeader, 128, "Range: npt=%f-\r\n", (float)p_client->current_position);
    }

    const TChar *rtpInfoTrackFmt =
        "url=%s/%s/trackID=%d"
        ";seq=%d"
        ";rtptime=%u";

    TU32 rtpTrackInfoSize = strlen(rtpInfoTrackFmt)
                            + strlen(mpRtspUrl) + p_client->session_name_len
                            + 5
                            + 10
                            + 2
                            + 32;

    TChar *rtpVideoTrackInfo = NULL;
    TChar *rtpAudioTrackInfo = NULL;

    if (p_client->enable_video) {
#if 1
        p_client->sub_session[ESubSession_video].start_time = 0;
        p_client->sub_session[ESubSession_video].seq_number = 0;
#else
        p_client->sub_session[ESubSession_video].start_time = gfRandom32();
        p_client->sub_session[ESubSession_video].seq_number = (gfRandom32() >> 16) & 0xffff;
#endif

        rtpVideoTrackInfo = (TChar *) DDBG_MALLOC(rtpTrackInfoSize, "SVVT");
        DASSERT(rtpVideoTrackInfo);

        memset(rtpVideoTrackInfo, 0x0, rtpTrackInfoSize);
        snprintf(rtpVideoTrackInfo, rtpTrackInfoSize, rtpInfoTrackFmt, mpRtspUrl, p_client->p_session_name, ESubSession_video,
                 p_client->sub_session[ESubSession_video].seq_number,
                 (TU32)p_client->sub_session[ESubSession_video].start_time);
        if (p_client->p_content && p_client->p_content->p_video_source_filter) {
            p_client->p_content->p_video_source_filter->FilterProperty(EFilterPropertyType_refresh_sequence, 1, NULL);
        }
    }

    if (p_client->enable_audio) {
#if 1
        p_client->sub_session[ESubSession_audio].start_time = 0;
        p_client->sub_session[ESubSession_audio].seq_number = 0;
#else
        p_client->sub_session[ESubSession_audio].start_time = gfRandom32();
        p_client->sub_session[ESubSession_audio].seq_number = (gfRandom32() >> 16) & 0xffff;
#endif
        rtpAudioTrackInfo = (TChar *) DDBG_MALLOC(rtpTrackInfoSize, "SVAT");
        DASSERT(rtpAudioTrackInfo);

        memset(rtpAudioTrackInfo, 0x0, rtpTrackInfoSize);
        snprintf(rtpAudioTrackInfo, rtpTrackInfoSize, rtpInfoTrackFmt, mpRtspUrl, p_client->p_session_name, ESubSession_audio,
                 p_client->sub_session[ESubSession_audio].seq_number,
                 (TU32)p_client->sub_session[ESubSession_audio].start_time);
    }

    TU32 rtpInfoSize = 32 + rtpTrackInfoSize * ESubSession_tot_count;

    TChar *rtpinfoall = (TChar *) DDBG_MALLOC(rtpInfoSize, "SVTI");
    DASSERT(rtpinfoall);

    if (p_client->enable_video && p_client->enable_audio) {
        snprintf(rtpinfoall, rtpInfoSize, "RTP-Info: %s,%s\r\n", rtpVideoTrackInfo, rtpAudioTrackInfo);
    } else if (p_client->enable_video) {
        snprintf(rtpinfoall, rtpInfoSize, "RTP-Info: %s\r\n", rtpVideoTrackInfo);
    } else if (p_client->enable_audio) {
        snprintf(rtpinfoall, rtpInfoSize, "RTP-Info: %s\r\n", rtpAudioTrackInfo);
    } else {
        LOGM_ERROR("why audio/video are all disabled?\n");
    }

    memset(mResponseBuffer, 0, sizeof(mResponseBuffer));

    snprintf((TChar *)mResponseBuffer, sizeof(mResponseBuffer),
             "RTSP/1.0 200 OK\r\n"
             "Server: %s\r\n"
             "CSeq: %s\r\n"
             "%s"
             "%s"
             "%s"
             "Session: %08X\r\n"
             "%s\r\n",
             DSRTING_RTSP_SERVER_TAG,
             cseq,
             mDateBuffer,
             scaleHeader,
             rangeHeader,
             p_client->session_id,
             rtpinfoall);

    free(rtpinfoall);
    if (rtpAudioTrackInfo) {
        free(rtpAudioTrackInfo);
    }
    if (rtpVideoTrackInfo) {
        free(rtpVideoTrackInfo);
    }

    SStreamingTransmiterCombo combo;

    if (DLikely(p_client->p_streaming_transmiter_filter || (p_client->vod_mode == 1))) {

        if (p_client->rtp_over_rtsp) {
            LOGM_INFO("RTP over RTSP (tcp mode)\n");
            DASSERT(p_client);
            DASSERT(p_client->p_content);

            ret = gfNet_Send(p_client->tcp_socket, (TU8 *)mResponseBuffer, (TInt)strlen(mResponseBuffer), 0);
            if (DUnlikely(ret < 0)) {
                LOGM_ERROR("write responce of PLAY(rtp over rtsp) fail, ret %d\n", ret);
                return ret;
            }

            if (p_client->enable_video) {
                DASSERT(p_client->sub_session[ESubSession_video].rtp_over_rtsp);
                combo.p_transmiter = (IStreamingTransmiter *)p_client->p_content->sub_session_content[ESubSession_video]->p_transmiter;
                combo.input_index = p_client->sub_session[ESubSession_video].p_content->transmiter_input_pin_index;
                combo.p_session = p_client;
                combo.p_sub_session = &p_client->sub_session[ESubSession_video];
                p_client->p_streaming_transmiter_filter->FilterProperty(EFilterPropertyType_streaming_add_subsession, 1, (void *) &combo);
            }

            if (p_client->enable_audio) {
                DASSERT(p_client->sub_session[ESubSession_audio].rtp_over_rtsp);
                combo.p_transmiter = (IStreamingTransmiter *)p_client->p_content->sub_session_content[ESubSession_audio]->p_transmiter;
                DASSERT(combo.p_sub_session);
                combo.input_index = p_client->sub_session[ESubSession_audio].p_content->transmiter_input_pin_index;
                combo.p_session = p_client;
                combo.p_sub_session = &p_client->sub_session[ESubSession_audio];
                p_client->p_streaming_transmiter_filter->FilterProperty(EFilterPropertyType_streaming_add_subsession, 1, (void *) &combo);
            }

            if (p_client->vod_mode == 1) {
                SVODPipeline *p_pipeline = (SVODPipeline *)p_client->p_pipeline;
                SFlowControlSetting params;
                //memset(&params, 0x0, sizeof(params));
                params.direction = 1;
                params.speed = p_client->speed_combo >> 3;
                params.speed_frac = (p_client->speed_combo & 0x7) << 5;
                params.navigation_begin_time = 0;
                p_pipeline->p_flow_controller->p_filter->FilterProperty(EFilterPropertyType_flow_controller_start, 1, (void *)&params);

                SContextInfo contextInfo;
                contextInfo.pChannelName = p_client->p_content->session_name;
                contextInfo.rtp_over_rtsp = p_client->rtp_over_rtsp;
                contextInfo.is_vod_mode = 1;
                contextInfo.vod_has_end = p_client->vod_has_end;
                contextInfo.speed_combo = p_client->speed_combo;
                contextInfo.starttime = &p_client->starttime;
                contextInfo.endtime = &p_client->endtime;

                if (DLikely(EECode_OK == p_pipeline->p_video_source[0]->p_filter->FilterProperty(EFilterPropertyType_demuxer_update_source_url, 1, (void *)&contextInfo))) {
                    p_pipeline->p_video_source[0]->p_filter->Resume();
                }
            }
            p_client->add_to_transmiter = 1;
            return 0;
        }

        for (i = 0; i < ESubSession_tot_count; i++) {
            memset(&p_client->sub_session[i].addr_port, 0, sizeof(struct sockaddr_in));
            p_client->sub_session[i].addr_port.sin_family      = AF_INET;
            p_client->sub_session[i].addr_port.sin_addr.s_addr = htonl(p_client->sub_session[i].addr);
            p_client->sub_session[i].addr_port.sin_port        = htons(p_client->sub_session[i].client_rtp_port);
            memset(&p_client->sub_session[i].addr_port_ext, 0, sizeof(struct sockaddr_in));
            p_client->sub_session[i].addr_port_ext.sin_family      = AF_INET;
            p_client->sub_session[i].addr_port_ext.sin_addr.s_addr = htonl(p_client->sub_session[i].addr);
            p_client->sub_session[i].addr_port_ext.sin_port        = htons(p_client->sub_session[i].client_rtcp_port);
            p_client->sub_session[i].parent = p_client;
        }

        ret = gfNet_Send(p_client->tcp_socket, (TU8 *)mResponseBuffer, (TInt)strlen(mResponseBuffer), 0);

        if (DUnlikely(ret < 0)) {
            LOGM_ERROR("write responce of PLAY(rtp over udp) fail, ret %d\n", ret);
            return ret;
        }

        if (p_client->enable_video) {
            DASSERT(!p_client->sub_session[ESubSession_video].rtp_over_rtsp);
            combo.p_transmiter = (IStreamingTransmiter *)p_client->p_content->sub_session_content[ESubSession_video]->p_transmiter;
            combo.input_index = p_client->sub_session[ESubSession_video].p_content->transmiter_input_pin_index;
            combo.p_session = p_client;
            combo.p_sub_session = &p_client->sub_session[ESubSession_video];
            p_client->p_streaming_transmiter_filter->FilterProperty(EFilterPropertyType_streaming_add_subsession, 1, (void *) &combo);
        }

        if (p_client->enable_audio) {
            DASSERT(!p_client->sub_session[ESubSession_audio].rtp_over_rtsp);
            combo.p_transmiter = (IStreamingTransmiter *)p_client->p_content->sub_session_content[ESubSession_audio]->p_transmiter;
            DASSERT(combo.p_sub_session);
            combo.input_index = p_client->sub_session[ESubSession_audio].p_content->transmiter_input_pin_index;
            combo.p_session = p_client;
            combo.p_sub_session = &p_client->sub_session[ESubSession_audio];
            p_client->p_streaming_transmiter_filter->FilterProperty(EFilterPropertyType_streaming_add_subsession, 1, (void *) &combo);
        }

        if (p_client->vod_mode == 1) {
            SVODPipeline *p_pipeline = (SVODPipeline *)p_client->p_pipeline;
            SFlowControlSetting params;
            params.direction = 1;
            params.speed = p_client->speed_combo >> 3;
            params.speed_frac = (p_client->speed_combo & 0x7) << 5;
            p_pipeline->p_flow_controller->p_filter->FilterProperty(EFilterPropertyType_flow_controller_start, 1, (void *)&params);

            SContextInfo contextInfo;
            contextInfo.pChannelName = p_client->p_content->session_name;
            contextInfo.rtp_over_rtsp = p_client->rtp_over_rtsp;
            contextInfo.is_vod_mode = 1;
            contextInfo.vod_has_end = p_client->vod_has_end;
            contextInfo.speed_combo = p_client->speed_combo;
            contextInfo.starttime = &p_client->starttime;
            contextInfo.endtime = &p_client->endtime;
            if (DLikely(EECode_OK == p_pipeline->p_video_source[0]->p_filter->FilterProperty(EFilterPropertyType_demuxer_update_source_url, 1, (void *)&contextInfo))) {
                p_pipeline->p_video_source[0]->p_filter->Resume();
            }
        }
        p_client->add_to_transmiter = 1;
    } else {
        LOGM_FATAL("NULL p_streaming_transmiter_filter!\n");
    }

    return 0;
}

TInt CRTSPStreamingServerV2::handleCommandRePlay(SClientSessionInfo *p_client, TChar const *cseq, TChar const *fullRequestStr)
{
    TU32 do_scale = 0, do_seek = 0;
    TInt ret = 0;

    gfGetDateString(mDateBuffer, sizeof(mDateBuffer));

    TChar *p_range = find_string((const TChar *) fullRequestStr, RTSP_MAX_BUFFER_SIZE, "Range:");
    TChar *p_scale = find_string((const TChar *) fullRequestStr, RTSP_MAX_BUFFER_SIZE, "Scale:");

    if (p_scale) {
        float speed = 1;
        ret = sscanf(p_scale, "Scale: %f", &speed);
        if (DLikely(1 == ret)) {
            do_scale = 1;
            p_client->speed_combo = (TU8)(speed * 8);
            LOG_NOTICE("replay speed %f, %x\n", speed, p_client->speed_combo);
        } else {
            LOG_ERROR("Bad speed request: [%s]\n", p_scale);
        }
    }

    if (p_range) {
        ret = sscanf(p_range, "Range: npt=%f", &p_client->current_position);
        if (DLikely(1 == ret)) {
            do_seek = 1;
            gfDateCalculateNextTime(p_client->seektime, p_client->starttime, p_client->current_position);
            LOG_NOTICE("seek to date %d-%d-%d %d-%d-%d, begin date %d-%d-%d %d-%d-%d, position %f\n", \
                       p_client->seektime.year, \
                       p_client->seektime.month, \
                       p_client->seektime.day, \
                       p_client->seektime.hour, \
                       p_client->seektime.minute, \
                       p_client->seektime.seconds, \
                       p_client->starttime.year, \
                       p_client->starttime.month, \
                       p_client->starttime.day, \
                       p_client->starttime.hour, \
                       p_client->starttime.minute, \
                       p_client->starttime.seconds, \
                       p_client->current_position);
            p_client->current_time = (TTime)(p_client->current_position * 1000000);
        } else {
            p_client->current_position = 0;
            p_client->current_time = 0;
            LOG_ERROR("Bad seek request:[%s]\n", p_range);
        }
    }

    SVODPipeline *p_pipeline = (SVODPipeline *)p_client->p_pipeline;
    SFlowControlSetting params;
    if (do_seek) {

        if (p_pipeline->p_video_source[0]) {
            p_pipeline->p_video_source[0]->p_filter->Pause();
        }
        if (p_pipeline->p_audio_source[0] && (p_pipeline->p_audio_source[0] != p_pipeline->p_video_source[0])) {
            p_pipeline->p_audio_source[0]->p_filter->Pause();
        }

        if (p_pipeline->p_data_transmiter) {
            p_pipeline->p_data_transmiter->p_filter->FilterProperty(EFilterPropertyType_purge_channel, 1, (void *) p_client);
        }

        if (p_pipeline->p_flow_controller) {
            p_pipeline->p_flow_controller->p_filter->FilterProperty(EFilterPropertyType_purge_channel, 1, (void *) p_client);
        }

        SContextInfo contextInfo;
        contextInfo.pChannelName = p_client->p_content->session_name;
        contextInfo.rtp_over_rtsp = p_client->rtp_over_rtsp;
        contextInfo.is_vod_mode = 1;
        contextInfo.vod_has_end = p_client->vod_has_end;
        contextInfo.speed_combo = p_client->speed_combo;
        contextInfo.starttime = &p_client->starttime;
        contextInfo.targettime = &p_client->seektime;
        contextInfo.endtime = &p_client->endtime;
        contextInfo.current_time = p_client->current_time;

        if (p_pipeline->p_video_source[0]) {
            p_pipeline->p_video_source[0]->p_filter->FilterProperty(EFilterPropertyType_demuxer_navigation_seek, 1, (void *) &contextInfo);
        }
        if (p_pipeline->p_audio_source[0] && (p_pipeline->p_audio_source[0] != p_pipeline->p_video_source[0])) {
            p_pipeline->p_audio_source[0]->p_filter->FilterProperty(EFilterPropertyType_demuxer_navigation_seek, 1, (void *) &contextInfo);
        }
        //LOG_DEBUG("contextInfo.current_time %lld, p_client->current_time %lld\n", contextInfo.current_time, p_client->current_time);

        if (((contextInfo.current_time + 4000000) > p_client->current_time) || contextInfo.current_time <= p_client->current_time) {
            p_client->current_time = contextInfo.current_time;
            p_client->current_position = (float)p_client->current_time / 1000000;
        } else {
            LOGM_WARN("time from demuxer not correct, %lld, target %lld\n", contextInfo.current_time, p_client->current_time);
        }
        //LOG_DEBUG("p_client->current_time %lld, p_client->current_position %f\n", p_client->current_time, p_client->current_position);

    }

    TChar scaleHeader[32] = {0};
    snprintf(scaleHeader, 32, "Scale: %f\r\n", (float)p_client->speed_combo / 8);

    TChar rangeHeader[128] = {0};
    if (p_client->vod_has_end) {
        snprintf(rangeHeader, 128, "Range: npt=%f-%f\r\n", (float)p_client->current_position, (float)p_client->vod_content_length);
    } else {
        snprintf(rangeHeader, 128, "Range: npt=%f-\r\n", (float)p_client->current_position);
    }

    const TChar *rtpInfoTrackFmt =
        "url=%s/%s/trackID=%d"
        ";seq=%d"
        ";rtptime=%u";

    TU32 rtpTrackInfoSize = strlen(rtpInfoTrackFmt)
                            + strlen(mpRtspUrl) + p_client->session_name_len
                            + 5
                            + 10
                            + 2
                            + 32;

    TChar *rtpVideoTrackInfo = NULL;
    TChar *rtpAudioTrackInfo = NULL;

    if (p_client->enable_video) {

        rtpVideoTrackInfo = (TChar *) DDBG_MALLOC(rtpTrackInfoSize, "SVVT");
        DASSERT(rtpVideoTrackInfo);

        p_client->sub_session[ESubSession_video].start_time = p_client->sub_session[ESubSession_video].cur_time + DDefaultVideoFramerateDen;
        p_client->sub_session[ESubSession_video].need_resync = 1;
        p_client->sub_session[ESubSession_video].seq_number ++;

        memset(rtpVideoTrackInfo, 0x0, rtpTrackInfoSize);
        snprintf(rtpVideoTrackInfo, rtpTrackInfoSize, rtpInfoTrackFmt, mpRtspUrl, p_client->p_session_name, ESubSession_video,
                 p_client->sub_session[ESubSession_video].seq_number,
                 (TU32)p_client->sub_session[ESubSession_video].start_time);
    }

    if (p_client->enable_audio) {

        rtpAudioTrackInfo = (TChar *) DDBG_MALLOC(rtpTrackInfoSize, "SVAT");
        DASSERT(rtpAudioTrackInfo);

        p_client->sub_session[ESubSession_audio].start_time = p_client->sub_session[ESubSession_audio].cur_time + 1024;
        p_client->sub_session[ESubSession_audio].need_resync = 1;
        p_client->sub_session[ESubSession_audio].seq_number ++;

        memset(rtpAudioTrackInfo, 0x0, rtpTrackInfoSize);
        snprintf(rtpAudioTrackInfo, rtpTrackInfoSize, rtpInfoTrackFmt, mpRtspUrl, p_client->p_session_name, ESubSession_audio,
                 p_client->sub_session[ESubSession_audio].seq_number,
                 (TU32)p_client->sub_session[ESubSession_audio].start_time);
    }

    TU32 rtpInfoSize = 32 + rtpTrackInfoSize * ESubSession_tot_count;

    TChar *rtpinfoall = (TChar *) DDBG_MALLOC(rtpInfoSize, "SVTI");
    DASSERT(rtpinfoall);

    if (p_client->enable_video && p_client->enable_audio) {
        snprintf(rtpinfoall, rtpInfoSize, "RTP-Info: %s,%s\r\n", rtpVideoTrackInfo, rtpAudioTrackInfo);
    } else if (p_client->enable_video) {
        snprintf(rtpinfoall, rtpInfoSize, "RTP-Info: %s\r\n", rtpVideoTrackInfo);
    } else if (p_client->enable_audio) {
        snprintf(rtpinfoall, rtpInfoSize, "RTP-Info: %s\r\n", rtpAudioTrackInfo);
    } else {
        LOGM_ERROR("why audio/video are all disabled?\n");
    }

    memset(mResponseBuffer, 0, sizeof(mResponseBuffer));

    if (do_seek && do_scale) {
        snprintf((TChar *)mResponseBuffer, sizeof(mResponseBuffer),
                 "RTSP/1.0 200 OK\r\n"
                 "Server: %s\r\n"
                 "CSeq: %s\r\n"
                 "%s"
                 "%s"
                 "%s"
                 "Session: %08X\r\n"
                 "%s\r\n",
                 DSRTING_RTSP_SERVER_TAG,
                 cseq,
                 mDateBuffer,
                 scaleHeader,
                 rangeHeader,
                 p_client->session_id,
                 rtpinfoall);
    } else if (do_seek) {
        snprintf((TChar *)mResponseBuffer, sizeof(mResponseBuffer),
                 "RTSP/1.0 200 OK\r\n"
                 "Server: %s\r\n"
                 "CSeq: %s\r\n"
                 "%s"
                 "%s"
                 "Session: %08X\r\n"
                 "%s\r\n",
                 DSRTING_RTSP_SERVER_TAG,
                 cseq,
                 mDateBuffer,
                 rangeHeader,
                 p_client->session_id,
                 rtpinfoall);
    } else if (do_scale) {
        snprintf((TChar *)mResponseBuffer, sizeof(mResponseBuffer),
                 "RTSP/1.0 200 OK\r\n"
                 "Server: %s\r\n"
                 "CSeq: %s\r\n"
                 "%s"
                 "%s"
                 "Session: %08X\r\n"
                 "%s\r\n",
                 DSRTING_RTSP_SERVER_TAG,
                 cseq,
                 mDateBuffer,
                 scaleHeader,
                 p_client->session_id,
                 rtpinfoall);
    } else {
        LOGM_ERROR("no scale, no seek, might have problem\n");
    }

    DDBG_FREE(rtpinfoall, "SVTI");
    if (rtpAudioTrackInfo) {
        DDBG_FREE(rtpAudioTrackInfo, "SVAT");
    }
    if (rtpVideoTrackInfo) {
        DDBG_FREE(rtpVideoTrackInfo, "SVVT");
    }

    ret = gfNet_Send(p_client->tcp_socket, (TU8 *)mResponseBuffer, (TInt)strlen(mResponseBuffer), 0);
    if (DUnlikely(ret < 0)) {
        LOGM_ERROR("write responce of PLAY(rtp over rtsp) fail, ret %d\n", ret);
        return ret;
    }

    if (do_seek) {
        if (p_pipeline->p_video_source[0]) {
            p_pipeline->p_video_source[0]->p_filter->Resume();
        }
        if (p_pipeline->p_audio_source[0] && (p_pipeline->p_audio_source[0] != p_pipeline->p_video_source[0])) {
            p_pipeline->p_audio_source[0]->p_filter->Resume();
        }

        if (p_pipeline->p_data_transmiter) {
            p_pipeline->p_data_transmiter->p_filter->FilterProperty(EFilterPropertyType_resume_channel, 1, (void *) p_client);
        }

        if (p_pipeline->p_flow_controller) {
            //memset(&params, 0x0, sizeof(params));
            params.direction = 1;
            params.speed = p_client->speed_combo >> 3;
            params.speed_frac = (p_client->speed_combo & 0x7) << 5;
            params.navigation_begin_time = p_client->current_time;
            p_pipeline->p_flow_controller->p_filter->FilterProperty(EFilterPropertyType_flow_controller_start, 1, (void *)&params);
            p_pipeline->p_flow_controller->p_filter->FilterProperty(EFilterPropertyType_resume_channel, 1, (void *) p_client);
        }
    } else if (do_scale) {
        if (p_pipeline->p_flow_controller) {
            params.speed = p_client->speed_combo >> 3;
            params.speed_frac = (p_client->speed_combo & 0x7) << 5;
            p_pipeline->p_flow_controller->p_filter->FilterProperty(EFilterPropertyType_flow_controller_update_speed, 1, (void *)&params);
        }
    }

    return 0;
}

void CRTSPStreamingServerV2::handleCommandPause(SClientSessionInfo *p_client, TChar const *cseq, TChar const *fullRequestStr)
{
    // Fill in the response:
    snprintf((TChar *)mResponseBuffer, sizeof(mResponseBuffer),
             "RTSP/1.0 200 OK\r\n"
             "Server: %s\r\n"
             "CSeq: %s\r\n"
             "Session: %08X\r\n\r\n",
             DSRTING_RTSP_SERVER_TAG,
             cseq,
             p_client->session_id);

    TInt ret = gfNet_Send(p_client->tcp_socket, (TU8 *)mResponseBuffer, (TInt)strlen(mResponseBuffer), 0);
    if (DUnlikely(ret < 0)) {
        LOGM_ERROR("write responce of PAUSE fail, ret %d\n", ret);
    }

}

void CRTSPStreamingServerV2::handleCommandResume(SClientSessionInfo *p_client, TChar const *cseq, TChar const *fullRequestStr)
{
    // Fill in the response:
    snprintf((TChar *)mResponseBuffer, sizeof(mResponseBuffer),
             "RTSP/1.0 200 OK\r\n"
             "Server: %s\r\n"
             "CSeq: %s\r\n"
             "Session: %08X\r\n\r\n",
             DSRTING_RTSP_SERVER_TAG,
             cseq,
             p_client->session_id);

    TInt ret = gfNet_Send(p_client->tcp_socket, (TU8 *)mResponseBuffer, (TInt)strlen(mResponseBuffer), 0);
    if (DUnlikely(ret < 0)) {
        LOGM_ERROR("write responce of PAUSE fail, ret %d\n", ret);
    }

}

void CRTSPStreamingServerV2::handleCommandGetParameter(SClientSessionInfo *p_client, TChar const *cseq, TChar const *fullRequestStr)
{
    gfGetDateString(mDateBuffer, sizeof(mDateBuffer));
    snprintf((TChar *)mResponseBuffer, sizeof(mResponseBuffer), "RTSP/1.0 200 OK\r\nServer: %s\r\nCSeq: %s\r\n%sSession: %08X\r\n\r\n", DSRTING_RTSP_SERVER_TAG, cseq, mDateBuffer, p_client->session_id);
}

void CRTSPStreamingServerV2::handleCommandSetParameter(SClientSessionInfo *p_client, TChar const *cseq, TChar const *fullRequestStr)
{
    gfGetDateString(mDateBuffer, sizeof(mDateBuffer));
    snprintf((TChar *)mResponseBuffer, sizeof(mResponseBuffer), "RTSP/1.0 200 OK\r\nServer: %s\r\nCSeq: %s\r\n%s\r\nSession: %08X\r\n\r\n", DSRTING_RTSP_SERVER_TAG, cseq, mDateBuffer, p_client->session_id);
}
void CRTSPStreamingServerV2::handleCommandTeardown(SClientSessionInfo *p_client, TChar const *cseq, TChar const *fullRequestStr)
{
    gfGetDateString(mDateBuffer, sizeof(mDateBuffer));
    snprintf((TChar *)mResponseBuffer, sizeof(mResponseBuffer), "RTSP/1.0 200 OK\r\nServer: %s\r\nCSeq: %s\r\n%sSession: %08X\r\n\r\n", DSRTING_RTSP_SERVER_TAG, cseq, mDateBuffer, p_client->session_id);
}

EECode CRTSPStreamingServerV2::handleCommandInSession(SClientSessionInfo *p_client, TChar const *cmdName,
        TChar const *urlPreSuffix, TChar const *urlSuffix,
        TChar const *cseq, TChar const *fullRequestStr)
{
    TInt ret = 0;
    gfGetDateString(mDateBuffer, sizeof(mDateBuffer));
    if (urlPreSuffix[0] == '\0' && urlSuffix[0] == '*' && urlSuffix[1] == '\0') {
        if (strcmp(cmdName, "GET_PARAMETER") == 0) {
            handleCommandGetParameter(p_client, cseq, fullRequestStr);
        } else if (strcmp(cmdName, "SET_PARAMETER") == 0) {
            handleCommandSetParameter(p_client, cseq, fullRequestStr);
        } else {
            handleCommandNotSupported(cseq);
        }
        ret = gfNet_Send(p_client->tcp_socket, (TU8 *)mResponseBuffer, (TInt)strlen(mResponseBuffer), 0);
        if (ret < 0) { return EECode_Closed; }
        return EECode_OK;
    } else if (p_client == NULL) { // no previous SETUP?
        handleCommandNotSupported(cseq);
        ret = gfNet_Send(p_client->tcp_socket, (TU8 *)mResponseBuffer, (TInt)strlen(mResponseBuffer), 0);
        if (ret < 0) { return EECode_Closed; }
        return EECode_OK;
    }

    if (strcmp(cmdName, "TEARDOWN") == 0) {
        p_client->state = ESessionServerState_Init;
        removeClientSession(p_client);
        //handleCommandTeardown(p_client, cseq, fullRequestStr);
        //ret = gfNet_Send(p_client->tcp_socket, (TU8*)mResponseBuffer, (TInt)strlen(mResponseBuffer), 0);
        return EECode_Closed;
    } else if (strcmp(cmdName, "PLAY") == 0) {
        if (ESessionServerState_Playing == p_client->state) {
            ret = handleCommandRePlay(p_client, cseq, fullRequestStr);
        } else {
            ret = handleCommandPlay(p_client, cseq, fullRequestStr);
            p_client->state = ESessionServerState_Playing;
        }
    } else if (strcmp(cmdName, "PAUSE") == 0) {
        handleCommandPause(p_client, cseq, fullRequestStr);
    } else if (strcmp(cmdName, "RESUME") == 0) {
        handleCommandResume(p_client, cseq, fullRequestStr);
    } else if (strcmp(cmdName, "GET_PARAMETER") == 0) {
        handleCommandGetParameter(p_client, cseq, fullRequestStr);
        ret = gfNet_Send(p_client->tcp_socket, (TU8 *)mResponseBuffer, (TInt)strlen(mResponseBuffer), 0);
    } else if (strcmp(cmdName, "SET_PARAMETER") == 0) {
        handleCommandSetParameter(p_client, cseq, fullRequestStr);
    }
    if (ret < 0) {
        LOGM_ERROR("tcp write fail, ret %d\n", ret);
        return EECode_Closed;
    }
    return EECode_OK;
}

TTime CRTSPStreamingServerV2::getCurrentSystemTime(void)
{
    if (DLikely(mpSystemClockReference)) {
        return mpSystemClockReference->GetCurrentTime();
    } else {
        LOGM_FATAL("NULL mpSystemClockReference\n");
    }
    return 0;
}

TInt CRTSPStreamingServerV2::onRtspRequest(SClientSessionInfo *p_client, TU8 *buf, TInt len)
{
    if (buf[0] == '$') {
        p_client->last_cmd_time = getCurrentSystemTime();
        return EECode_OK;
    } else {
        memcpy(mRequestBuffer, buf, len);
        mRequestBuffer[len] = '\0';
        mRequestBufferLen = len;
        return handleClientRequest(p_client);
    }
}

TInt CRTSPStreamingServerV2::handleRtspRequest(SClientSessionInfo *p_client)
{
    TInt bytesRead = gfNet_Recv(p_client->tcp_socket, (TU8 *)p_client->mRecvBuffer + p_client->mRecvLen, RTSP_MAX_BUFFER_SIZE - 1 - p_client->mRecvLen, 0);
    if (bytesRead < 0) {
        LOGM_NOTICE("handleRtspRequest, recv fail, ret %d, recvLen = %d\n", bytesRead, p_client->mRecvLen);
        if (DLikely(p_client->p_content)) {
            LOGM_PRINTF("client: %s, port %d, socket %d, content index %d, url %s.\n", inet_ntoa(p_client->client_addr.sin_addr), ntohs(p_client->client_addr.sin_port), p_client->tcp_socket, p_client->p_content->content_index, p_client->p_content->session_name);
        } else {
            LOGM_PRINTF("client: %s, port %d, socket %d, no content\n", inet_ntoa(p_client->client_addr.sin_addr), ntohs(p_client->client_addr.sin_port), p_client->tcp_socket);
        }
        return EECode_Closed;
    } else if (bytesRead == 0) {
        LOGM_NOTICE("handleRtspRequest, Client close socket, recvLen = %d\n", p_client->mRecvLen);
        if (DLikely(p_client->p_content)) {
            LOGM_PRINTF("client: %s, port %d, socket %d, content index %d, url %s.\n", inet_ntoa(p_client->client_addr.sin_addr), ntohs(p_client->client_addr.sin_port), p_client->tcp_socket, p_client->p_content->content_index, p_client->p_content->session_name);
        } else {
            LOGM_PRINTF("client: %s, port %d, socket %d, no content\n", inet_ntoa(p_client->client_addr.sin_addr), ntohs(p_client->client_addr.sin_port), p_client->tcp_socket);
        }
        return EECode_Closed;
    }
    p_client->mRecvLen += bytesRead;

    if (p_client->rtp_over_rtsp) {
        gfSocketSetQuickACK(p_client->tcp_socket, 0);
    }
    p_client->mRecvBuffer[p_client->mRecvLen] = '\0';

    if (p_client->mRecvLen > 4) {
        TInt parse_len = parseRequest(p_client);
        if (parse_len < 0) {
            return EECode_Closed; //send data failed
        }
        if (parse_len < p_client->mRecvLen) {
            memmove(p_client->mRecvBuffer, p_client->mRecvBuffer + parse_len, p_client->mRecvLen - parse_len);
        }
        p_client->mRecvLen -= parse_len;
    }

    return EECode_TryAgain;
}

TInt CRTSPStreamingServerV2::parseRequest(SClientSessionInfo *p_client)
{
    TU8 *buf = (TU8 *)p_client->mRecvBuffer;
    TInt len = p_client->mRecvLen;

    TInt mDataLen = 0;
    e_parse_state mState = STATE_IDLE;
    TU8 *packet = NULL;
    TInt parse_len = 0;

    for (TInt i = 0; i < len; i ++) {
        if (!packet) {
            packet = &buf[i];
        }
        TChar c = buf[i];
        switch (mState) {
            case STATE_IDLE: {
                    if (c == '$')  {
                        mState = STATE_CHANNEL;
                    } else if (c == '\r') {
                        mState = STATE_N_1;
                    }
                } break;
            case STATE_N_1: {
                    if (c == '\n') {
                        mState = STATE_R_2;
                    } else {
                        mState = STATE_ERROR;
                    }
                } break;
            case STATE_R_2: {
                    if (c == '\r') {
                        mState = STATE_N_2;
                    } else {
                        mState = STATE_IDLE;
                    }
                } break;
            case STATE_N_2: {
                    if (c == '\n') {
                        if (onRtspRequest(p_client, packet, &buf[i] - packet + 1) != EECode_OK) {
                            return -1;
                        }
                        parse_len = i + 1;
                        packet = NULL;
                        mState = STATE_IDLE;
                    } else {
                        mState = STATE_ERROR;
                    }
                } break;
            case STATE_CHANNEL: {
                    mState = STATE_LEN_1;
                } break;
            case STATE_LEN_1: {
                    mDataLen = buf[i];
                    mState = STATE_LEN_2;
                } break;
            case STATE_LEN_2: {
                    mDataLen = ((mDataLen << 8) & 0xff00) + buf[i];
                    mState = STATE_BINARY;
                } break;
            case STATE_BINARY: {
                    --mDataLen;
                    if (!mDataLen) {
                        if (onRtspRequest(p_client, packet, &buf[i] - packet + 1) != EECode_OK) {
                            return -1;
                        }
                        parse_len = i + 1;
                        packet = NULL;
                        mState = STATE_IDLE;
                    }
                } break;
            case STATE_ERROR:
            default: {
                    //what should we do?
                    LOGM_WARN("doParseRequest: STATE_ERROR\n");
                    parse_len = i + 1;
                } break;
        }
    }
    //LOGM_WARN("doParseRequest: parse_len = %d\n",parse_len);
    return parse_len;
}

EECode CRTSPStreamingServerV2::RemoveStreamingContent(SStreamingSessionContent *content)
{
    LOGM_FATAL("TO DO\n");
    return EECode_NoImplement;
}

EECode CRTSPStreamingServerV2::RemoveStreamingClientSession(SClientSessionInfo *session)
{
    deleteClientSession(session);
    return EECode_OK;
}

EECode CRTSPStreamingServerV2::ClearStreamingClients(SStreamingSessionContent *related_content)
{
    LOGM_FATAL("TO DO\n");
    return EECode_NoImplement;
}

EECode CRTSPStreamingServerV2::GetHandler(TInt &handle) const
{
    handle = mSocket;
    return EECode_OK;
}

EECode CRTSPStreamingServerV2::GetServerState(EServerState &state) const
{
    state = msState;
    return EECode_OK;
}

EECode CRTSPStreamingServerV2::SetServerState(EServerState state)
{
    msState = state;
    return EECode_OK;
}

EECode CRTSPStreamingServerV2::GetServerID(TGenericID &id, TComponentIndex &index, TComponentType &type) const
{
    id = mId;
    index = mIndex;
    type = EGenericComponentType_StreamingServer;
    return EECode_OK;
}

EECode CRTSPStreamingServerV2::SetServerID(TGenericID id, TComponentIndex index, TComponentType type)
{
    mId = id;
    mIndex = index;
    DASSERT(EGenericComponentType_StreamingServer == type);
    return EECode_OK;
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END


