
/**
 * directsharing_receiver.h
 *
 * History:
 *    2015/03/10 - [Zhi He] create file
 *
 * Copyright (C) 2014 - 2024, the Ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the Ambarella Inc.
 */

#ifndef __DIRECTSHARING_RECEIVER_H__
#define __DIRECTSHARING_RECEIVER_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN
DCODE_DELIMITER;

class CIDirectSharingReceiver
    : public CObject
    , public IActiveObject
    , public IDirectSharingReceiver
{
protected:
    CIDirectSharingReceiver(TSocketHandler socket, const volatile SPersistCommonConfig *pconfig, IMsgSink *pMsgSink, const CIClockReference *p_system_clock_reference);
    virtual ~CIDirectSharingReceiver();
    EECode Construct();

public:
    static CIDirectSharingReceiver *Create(TSocketHandler socket, const volatile SPersistCommonConfig *pconfig, IMsgSink *pMsgSink, const CIClockReference *p_system_clock_reference);

public:
    virtual CObject *GetObject() const;
    virtual void Destroy();
    virtual void Delete();
    virtual void OnRun();

    virtual void PrintStatus();

public:
    virtual void SetCallBack(void *owner, TRequestMemoryCallBack request_memory_callback, TDataReceiveCallBack receive_callback);
    virtual EECode Start();
    virtual EECode Stop();

private:
    void processCmd(SCMD &cmd);


private:
    const volatile SPersistCommonConfig *mpPersistCommonConfig;
    IMsgSink *mpMsgSink;
    const CIClockReference *mpSystemClockReference;
    CIWorkQueue *mpWorkQueue;

private:
    TU8 mbRun;
    TU8 msState;
    TU16 mReserved0;

private:
    void *mpCallbackContext;
    TRequestMemoryCallBack mfRequestMemoryCallback;
    TDataReceiveCallBack mfReceiveCallback;

private:
    TSocketHandler mPipeFd[2];
    TSocketHandler mMaxFd;

    fd_set mAllSet;
    fd_set mReadSet;

    TSocketHandler mSocket;
};

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif

