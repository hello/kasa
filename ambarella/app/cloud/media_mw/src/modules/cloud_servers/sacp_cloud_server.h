
/**
 * sacp_cloud_server.h
 *
 * History:
 *    2013/12/01 - [Zhi He] create file
 *
 * Copyright (C) 2014 - 2024, the Ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the Ambarella Inc.
 */

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

