/*******************************************************************************
 * im_server.h
 *
 * History:
 *    2014/06/17 - [Zhi He] create file
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

