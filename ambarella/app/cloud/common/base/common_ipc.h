/*******************************************************************************
 * common_ipc.h
 *
 * History:
 *  2014/04/22 - [Zhi He] create file
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


