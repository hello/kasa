/*******************************************************************************
 * sacp_cloud_server.h
 *
 * History:
 *    2013/12/01 - [Zhi He] create file
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

#ifndef __SACP_CLOUD_SERVER_H__
#define __SACP_CLOUD_SERVER_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN
DCODE_DELIMITER;

class CSACPCouldServer
    : virtual public CObject
    , virtual public ICloudServer
{
    typedef CObject inherited;

protected:
    CSACPCouldServer(const TChar *name, CloudServerType type, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink, TU16 server_port);
    virtual ~CSACPCouldServer();

    EECode Construct();

public:
    static ICloudServer *Create(const TChar *name, CloudServerType type, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink, TU16 server_port);
    virtual CObject *GetObject0() const;

public:
    virtual void Delete();
    virtual void PrintStatus();

public:
    virtual EECode HandleServerRequest(TSocketHandler &max, void *all_set);
    virtual EECode AddCloudContent(SCloudContent *content);
    virtual EECode RemoveCloudContent(SCloudContent *content);
    virtual EECode GetHandler(TSocketHandler &handle) const;
    virtual EECode RemoveCloudClient(void *filter_owner);

    virtual EECode GetServerState(EServerState &state) const;
    virtual EECode SetServerState(EServerState state);

    virtual EECode GetServerID(TGenericID &id, TComponentIndex &index, TComponentType &type) const;
    virtual EECode SetServerID(TGenericID id, TComponentIndex index, TComponentType type);

protected:
    void replyToClient(TInt socket, EECode error_code, SSACPHeader *header, TInt length);
    EECode authentication(TInt socket, SCloudContent *&p_authen_content);
    EECode newClient(SCloudContent *p_content, TInt socket);
    void removeClient(SCloudContent *p_content);

protected:
    const volatile SPersistMediaConfig *mpPersistMediaConfig;
    IMsgSink *mpMsgSink;
    const CIClockReference *mpSystemClockReference;
    IMutex *mpMutex;
    TU8 *mpReadBuffer;
    TU32 mnMaxPayloadLength;

private:
    TGenericID mId;

    TComponentIndex mIndex;
    TSocketPort mListeningPort;

    EServerState msState;
    TSocketHandler mSocket;

    CIDoubleLinkedList *mpContentList;

    TU32 mbEnableHardwareAuthenticate;
};

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif

