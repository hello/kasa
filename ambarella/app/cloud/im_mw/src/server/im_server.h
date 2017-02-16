
/**
 * im_server.h
 *
 * History:
 *    2014/06/17 - [Zhi He] create file
 *
 * Copyright (C) 2014 - 2024, the Ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the Ambarella Inc.
 */

#ifndef __IM_SERVER_H__
#define __IM_SERVER_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN
DCODE_DELIMITER;

class CIMServer
    : public CObject
    , public IIMServer
{
    typedef CObject inherited;

protected:
    CIMServer(const TChar *name, EProtocolType type, const volatile SPersistCommonConfig *pPersistCommonConfig, IMsgSink *pMsgSink, IAccountManager *pAccountManager, TU16 server_port);
    virtual ~CIMServer();

    EECode Construct();

public:
    static IIMServer *Create(const TChar *name, EProtocolType type, const volatile SPersistCommonConfig *pPersistCommonConfig, IMsgSink *pMsgSink, IAccountManager *p_account_manager, TU16 server_port);
public:
    virtual void Delete();
    virtual void Destroy();

    virtual void PrintStatus();

public:
    virtual EECode HandleServerRequest(TSocketHandler &max, void *all_set);
    virtual EECode GetHandler(TInt &handle) const;

    virtual EECode GetServerState(EServerState &state) const;
    virtual EECode SetServerState(EServerState state);

protected:
    void replyToClient(TInt socket, EECode error_code, TU8 *header, TInt length);
    EECode authentication(TInt socket, SAccountInfoRoot *&p_account_info);

protected:
    const volatile SPersistCommonConfig *mpPersistCommonConfig;
    IMsgSink *mpMsgSink;
    IAccountManager *mpAccountManager;
    const CIClockReference *mpSystemClockReference;
    IMutex *mpMutex;
    IThreadPool *mpThreadPool;
    IScheduler *mpSimpleScheduler;
    TU32 mnThreadNum;

protected:
    IProtocolHeaderParser *mpProtocolHeaderParser;
    TU8 *mpReadBuffer;
    TU32 mnHeaderLength;
    TU32 mnMaxPayloadLength;

private:
    TU16 mListeningPort;
    TU16 mReserved0;

    EServerState msState;
    TInt mSocket;

    //private:
    //SDataNodeInfo* mpDataNode;
    //TU32 mMaxDataNodeCount;
    //TU32 mDataNodeCount;

    //TU32* mpDeviceGroup;
    //TU32 mMaxDeviceGroupCount;
    //TU32 mDeviceGroupCount;

    //TU32 mCurGroupIndex;
    //TU32 mDeviceCount;

    //private:
    //CIDoubleLinkedList* mpClientPortList;
    //IRunTimeConfigAPI* mpConfigAPI;
};

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif

