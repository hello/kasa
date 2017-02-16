
/**
 * directsharing_server.h
 *
 * History:
 *    2015/03/08 - [Zhi He] create file
 *
 * Copyright (C) 2014 - 2024, the Ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the Ambarella Inc.
 */

#ifndef __DIRECTSHARING_SERVER_H__
#define __DIRECTSHARING_SERVER_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN
DCODE_DELIMITER;

class CIDirectSharingServer
    : public CObject
    , public IActiveObject
    , public IDirectSharingServer
{
    typedef CObject inherited;

protected:
    CIDirectSharingServer(TSocketPort port, const volatile SPersistCommonConfig *pconfig, IMsgSink *pMsgSink, const CIClockReference *p_system_clock_reference);
    virtual ~CIDirectSharingServer();
    EECode Construct();

public:
    static CIDirectSharingServer *Create(TSocketPort port, const volatile SPersistCommonConfig *pconfig, IMsgSink *pMsgSink, const CIClockReference *p_system_clock_reference);
    virtual CObject *GetObject() const;

public:
    virtual void Destroy();
    virtual void Delete();
    virtual void OnRun();

    virtual EECode StartServer();
    virtual EECode StopServer();

    virtual EECode AddDispatcher(IDirectSharingDispatcher *dispatcher);
    virtual EECode RemoveDispatcher(IDirectSharingDispatcher *dispatcher);

    virtual void PrintStatus();

private:
    void processCmd(SCMD &cmd);
    void handleServerRequest();
    EDirectSharingStatusCode handshake(TSocketHandler socket, IDirectSharingDispatcher *&dispatcher, IDirectSharingSender *&p_sender, IDirectSharingReceiver *&p_receiver);
    IDirectSharingDispatcher *findDispatcher(TU8 type, TU8 index);

protected:
    const volatile SPersistCommonConfig *mpPersistCommonConfig;
    IMsgSink *mpMsgSink;
    const CIClockReference *mpSystemClockReference;
    CIWorkQueue *mpWorkQueue;
    IMutex *mpMutex;

private:
    TU8 mbRun;
    TU8 msState;
    TU8 mbStarted;
    TU8 mReserved0;

    TU16 mReserved1;
    TSocketPort mServerPort;

    TSocketHandler mSocket;

private:
    TU8 *mpReadBuffer;
    TU32 mnMaxPayloadLength;

private:
    TSocketHandler mPipeFd[2];
    TSocketHandler mMaxFd;

    fd_set mAllSet;
    fd_set mReadSet;

    CIDoubleLinkedList *mpDispatcherList;
};

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif

