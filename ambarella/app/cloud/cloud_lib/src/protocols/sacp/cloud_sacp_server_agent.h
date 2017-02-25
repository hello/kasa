/*******************************************************************************
 * cloud_sacp_server_agent.h
 *
 * History:
 *  2013/11/28 - [Zhi He] create file
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

#ifndef __CLOUD_SACP_SERVER_AGENT_H__
#define __CLOUD_SACP_SERVER_AGENT_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN

DCODE_DELIMITER;

//-----------------------------------------------------------------------
//
//  CSACPCloudServerAgent
//
//-----------------------------------------------------------------------

class CSACPCloudServerAgent: public ICloudServerAgent
{
public:
    static CSACPCloudServerAgent *Create();

protected:
    EECode Construct();

    CSACPCloudServerAgent();
    virtual ~CSACPCloudServerAgent();

public:
    virtual EECode Scheduling(TUint times = 1, TU32 inout_mask = 0);

    virtual TInt IsPassiveMode() const;
    virtual TSchedulingHandle GetWaitHandle() const;
    virtual TU8 GetPriority() const;

    virtual EECode CheckTimeout();
    virtual EECode EventHandling(EECode err);

    virtual TSchedulingUnit HungryScore() const;
    virtual void Destroy();

public:
    virtual EECode AcceptClient(TInt socket);
    virtual EECode CloseConnection();

    virtual EECode ReadData(TU8 *p_data, TMemSize size);
    virtual EECode WriteData(TU8 *p_data, TMemSize size);

    virtual EECode SetProcessCMDCallBack(void *owner, TServerAgentCmdCallBack callback);
    virtual EECode SetProcessDataCallBack(void *owner, TServerAgentDataCallBack callback);

    virtual EECode SetProcessCMDCallBackV2(void *owner, TServerAgentV2CmdCallBack callback);
    virtual EECode SetProcessDataCallBackV2(void *owner, TServerAgentV2DataCallBack callback);

public:
    virtual void Delete();

private:
    EECode processClientCmd(ESACPClientCmdSubType sub_type, TU16 payload_size);
    EECode processCmd(TU8 cat, TU16 sub_type, TU16 payload_size);

private:
    TU8 mbAccepted;
    TU8 mbAuthenticated;
    TU8 mReserved0;
    TU8 mbPeerClosed;

    TSocketHandler mSocket;

    TU16 mLastSeqCount;
    TU8 mDataType;
    TU8 mHeaderExtentionType;

    TU8 mEncryptionType1;
    TU8 mEncryptionType2;
    TU8 mEncryptionType3;
    TU8 mEncryptionType4;

private:
    void *mpCmdCallbackOwner;
    TServerAgentCmdCallBack mpCmdCallback;

    void *mpDataCallbackOwner;
    TServerAgentDataCallBack mpDataCallback;

private:
    TU8 *mpSenderBuffer;
    TMemSize mSenderBufferSize;

private:
    SSACPHeader *mpSACPHeader;
    TU8 *mpSACPPayload;

private:
    ESACPDataChannelSubType mDataSubType;
};


DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif

