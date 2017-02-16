/**
 * common_ipc.h
 *
 * History:
 *  2014/04/22 - [Zhi He] create file
 *
 * Copyright (C) 2014 - 2024, the Ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the Ambarella Inc.
 */

#ifndef __COMMON_IPC_H__
#define __COMMON_IPC_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN

DCODE_DELIMITER;

enum {
    EIPCAgentState_idle,
    EIPCAgentState_server_listening,
    //EIPCAgentState_client_connecting,
    EIPCAgentState_server_running,
    EIPCAgentState_client_running,
    EIPCAgentState_error,
};

#define DPathMax 256

class CIPCUnixDomainSocketAgent: public CObject, public IActiveObject, public IIPCAgent
{
public:
    static IIPCAgent *Create();
    virtual CObject *GetObject() const;

protected:
    CIPCUnixDomainSocketAgent();
    virtual ~CIPCUnixDomainSocketAgent();
    EECode Construct();

public:
    virtual EECode CreateContext(TChar *tag, TUint is_server, void *p_context, TIPCAgentReadCallBack callback, TU16 native_port = 0, TU16 remote_port = 0);
    virtual void DestroyContext();

    virtual TInt GetHandle() const;

    virtual EECode Start();
    virtual EECode Stop();

    virtual EECode Write(TU8 *data, TInt size);

public:
    virtual void Delete();
    virtual void OnRun();

public:
    virtual void Destroy();

private:
    bool processCmd(SCMD &cmd);
    EECode beginListen();
    EECode connectToServer();

private:
    CIWorkQueue *mpWorkQueue;

private:
    TU16 mPort;
    TU16 mRemotePort;

    TU8 mbRunning;
    TU8 msState;
    TU8 mIsServerMode;
    TU8 mbConnecting;

    TSocketHandler mSocket;
    TSocketHandler mTransferSocket;

    TInt mMaxFd;
    TInt mMaxWriteFd;
    TInt mPipeFd[2];

    fd_set mAllSet;
    fd_set mAllWriteSet;
    fd_set mReadSet;
    fd_set mWriteSet;

private:
    TChar mTag[DPathMax];

    struct sockaddr_un mAddr;

private:
    TU8 *mpReadBuffer;
    TInt mReadBufferSize;
    TInt mReadDataSize;

private:
    void *mpReadCallBackContext;
    TIPCAgentReadCallBack mReadCallBack;
};

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif


