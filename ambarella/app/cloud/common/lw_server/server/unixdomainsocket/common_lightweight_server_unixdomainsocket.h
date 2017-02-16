/**
 * common_lightweight_server_unixdomainsocket.h
 *
 * History:
 *  2014/12/30 - [Zhi He] create file
 *
 * Copyright (C) 2014 - 2024, the Ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the Ambarella Inc.
 */

#ifndef __COMMON_LIGHTWEIGHT_SERVER_UNIXDOMAINSOCKET_H__
#define __COMMON_LIGHTWEIGHT_SERVER_UNIXDOMAINSOCKET_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN

DCODE_DELIMITER;

class CILightweightServerUnixDomainSocket: public CObject, public IActiveObject, public ILightweightServer
{
public:
    static ILightweightServer *Create();
    virtual CObject *GetObject0() const;
    virtual CObject *GetObject() const;

protected:
    CILightweightServer();
    virtual ~CILightweightServer();
    EECode Construct();

public:
    virtual EECode CreateContext(TChar *url, TSocketPort port);
    virtual void DestroyContext();

    virtual EECode Start();
    virtual EECode Stop();

    virtual EECode SendData(SLWData *input_data);

public:
    virtual void Delete();
    virtual void OnRun();

private:
    bool processCmd(SCMD &cmd);
    EECode beginListen();
    EECode connectToServer();

private:
    CIWorkQueue *mpWorkQueue;

private:
    TSocketPort mPort;
    TU8 mbRunning;
    TU8 msState;

    TSocketHandler mSocket;

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
    void *mpReadCallBackContext;
    TIPCAgentReadCallBack mReadCallBack;
};

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif


