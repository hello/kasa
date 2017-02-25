/*******************************************************************************
 * rtsp_server.h
 *
 * History:
 *    2013/11/15 - [Zhi He] create file
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

#ifndef __RTSP_SERVER_H__
#define __RTSP_SERVER_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN
DCODE_DELIMITER;

class CRTSPStreamingServer
    : virtual public CObject
    , virtual public IStreamingServer
{
    typedef CObject inherited;

protected:
    CRTSPStreamingServer(const TChar *name, StreamingServerType type, StreamingServerMode mode, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink, TU16 server_port, TU8 enable_video, TU8 enable_audio);
    virtual ~CRTSPStreamingServer();

    EECode Construct();

public:
    static IStreamingServer *Create(const TChar *name, StreamingServerType type, StreamingServerMode mode, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink, TU16 server_port, TU8 enable_video, TU8 enable_audio);
    virtual CObject *GetObject0() const;

public:
    virtual void Delete();
    virtual void PrintStatus();

public:
    virtual EECode HandleServerRequest(TSocketHandler &max, void *all_set);
    virtual void ScanClientList(TInt &nready, void *read_set, void *all_set);
    virtual EECode AddStreamingContent(SStreamingSessionContent *content);
    virtual EECode RemoveStreamingContent(SStreamingSessionContent *content);
    virtual EECode RemoveStreamingClientSession(SClientSessionInfo *session);
    virtual EECode ClearStreamingClients(SStreamingSessionContent *related_content);
    virtual EECode GetHandler(TInt &handle) const;

    virtual EECode GetServerState(EServerState &state) const;
    virtual EECode SetServerState(EServerState state);

    virtual EECode GetServerID(TGenericID &id, TComponentIndex &index, TComponentType &type) const;
    virtual EECode SetServerID(TGenericID id, TComponentIndex index, TComponentType type);

protected:
    EECode handleClientRequest(SClientSessionInfo *p_client);

    //    EECode checkServerValid(SStreamingServerInfo* p_server);
    //    EECode checkClientValid(SClientSessionInfo* p_client);
    bool parseRTSPRequestString(TChar const *reqStr, TU32 reqStrSize, TChar *resultCmdName, TU32 resultCmdNameMaxSize, TChar *resultURLPreSuffix,
                                TU32 resultURLPreSuffixMaxSize, TChar *resultURLSuffix, TU32 resultURLSuffixMaxSize, TChar *resultCSeq, TU32 resultCSeqMaxSize);

    bool parseAuthorizationHeader(TChar const *buf,
                                  TChar const *&username,
                                  TChar const *&realm,
                                  TChar const *&nonce, TChar const *&uri,
                                  TChar const *&response);


    void handleCommandBad(TChar const *cseq);
    void handleCommandNotSupported(TChar const *cseq);
    void handleCommandNotFound(TChar const *cseq);
    void handleCommandUnsupportedTransport(TChar const *cseq);
    void handleCommandServerInternalError(TChar const *cseq);
    void handleCommandOptions(TChar const *cseq);
    void handleCommandDescribe(SClientSessionInfo *p_client, TChar const *cseq, TChar const *urlSuffix, TChar const *fullRequestStr);
    void handleCommandSetup(SClientSessionInfo *p_client, TChar const *cseq, TChar const *urlPreSuffix, TChar const *urlSuffix, TChar const *fullRequestStr);
    TInt handleCommandPlay(SClientSessionInfo *p_client, TChar const *cseq, TChar const *fullRequestStr);
    TInt handleCommandRePlay(SClientSessionInfo *p_client, TChar const *cseq, TChar const *fullRequestStr);
    void handleCommandTeardown(SClientSessionInfo *p_client, TChar const *cseq, TChar const *fullRequestStr);
    void handleCommandPause(SClientSessionInfo *p_client, TChar const *cseq, TChar const *fullRequestStr);
    void handleCommandResume(SClientSessionInfo *p_client, TChar const *cseq, TChar const *fullRequestStr);
    void handleCommandGetParameter(SClientSessionInfo *p_client, TChar const *cseq, TChar const *fullRequestStr);
    void handleCommandSetParameter(SClientSessionInfo *p_client, TChar const *cseq, TChar const *fullRequestStr);
    EECode handleCommandInSession(SClientSessionInfo *p_client, TChar const *cmdName, TChar const *urlPreSuffix, TChar const *urlSuffix, TChar const *cseq, TChar const *fullRequestStr);

private:
    EECode authentication(SClientSessionInfo *client_session, const TChar *p_url);
    EECode parseRequestUrl(SClientSessionInfo *p_client_session, const TChar *url, TU8 &content_type);
    EECode authenticationUser(SClientSessionInfo *p_client_session, const TChar *cmd, const TChar *cseq, TChar const *fullRequestStr);
    EECode setRandomNonce(SClientSessionInfo *p_client_session);
    EECode computeDigestResponse(SClientSessionInfo *p_client_session, const TChar *cmd, const TChar *url, TChar *&p_result);
    void deleteClientSession(SClientSessionInfo *p_session);
    void deleteSDPSession(void *);

private:
    //parse rtsp request
    enum e_parse_state {
        STATE_ERROR,
        STATE_IDLE,
        STATE_N_1,
        STATE_R_2,
        STATE_N_2,
        STATE_CHANNEL,
        STATE_LEN_1,
        STATE_LEN_2,
        STATE_BINARY
    };

    TTime  getCurrentSystemTime(void);
    TInt  onRtspRequest(SClientSessionInfo *p_client, TU8 *buf, TInt len);
    TInt  handleRtspRequest(SClientSessionInfo *p_client);
    TInt  tcpWrite(TInt fd, const TChar *buf, TInt len);
    TInt parseRequest(SClientSessionInfo *p_client);

protected:
    const volatile SPersistMediaConfig *mpPersistMediaConfig;
    IMsgSink *mpMsgSink;
    const CIClockReference *mpSystemClockReference;
    IMutex *mpMutex;

private:
    TGenericID mId;

    TComponentIndex mIndex;
    TU8 mbEnableVideo;
    TU8 mbEnableAudio;

    TU16 mRtspSessionId;
    TU16 mListeningPort;

    TU16 mDataPortBase;

    StreamingServerType mType;
    StreamingServerMode mMode;
    EServerState msState;

    TUint mRtspSSRC;
    TSocketHandler mSocket;

    CIDoubleLinkedList *mpClientSessionList;
    CIDoubleLinkedList *mpContentList;
    CIDoubleLinkedList *mpVodContentList;
    CIDoubleLinkedList *mpSDPSessionList;

    TChar *mpRtspUrl;
    TUint mRtspUrlLength;

private:
    TChar mRequestBuffer[RTSP_MAX_BUFFER_SIZE];
    TInt mRequestBufferLen;
    TChar mResponseBuffer[RTSP_MAX_BUFFER_SIZE];
    TChar mDateBuffer[RTSP_MAX_DATE_BUFFER_SIZE];
};

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif

