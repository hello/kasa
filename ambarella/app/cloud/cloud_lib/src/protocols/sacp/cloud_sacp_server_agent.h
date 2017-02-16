/**
 * cloud_sacp_server_agent.h
 *
 * History:
 *  2013/11/28 - [Zhi He] create file
 *
 * Copyright (C) 2014 - 2024, the Ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the Ambarella Inc.
 */

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

