/*******************************************************************************
 * im_sacp_client_agent.h
 *
 * History:
 *  2014/06/21 - [Zhi He] create file
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

#ifndef __IM_SACP_CLIENT_AGENT_H__
#define __IM_SACP_CLIENT_AGENT_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN

DCODE_DELIMITER;

//-----------------------------------------------------------------------
//
//  CSACPIMClientAgentAsync
//
//-----------------------------------------------------------------------

class CSACPIMClientAgentAsync: public IIMClientAgentAsync
{
public:
    static CSACPIMClientAgentAsync *Create();

protected:
    EECode Construct();

    CSACPIMClientAgentAsync();
    virtual ~CSACPIMClientAgentAsync();

public:
    virtual EECode Register(TChar *name, TChar *password, const TChar *server_url, TU16 serverport);

public:
    virtual EECode AcquireOwnership(TChar *device_code);
    virtual EECode DonateOwnership(TChar *donate_target, TChar *device_code);

public:
    virtual EECode GrantPrivilegeSingleDevice(TUniqueID device_id, TUniqueID usr_id, TU32 privilege = 0);
    virtual EECode GrantPrivilege(TUniqueID *device_id, TU32 device_count, TUniqueID usr_id, TU32 privilege = 0);
    virtual EECode AddFriend(TChar *friend_user_name);
    virtual EECode AcceptFriend(TUniqueID friend_id);
    virtual EECode RemoveFriend(TUniqueID friend_id);

    virtual EECode RetrieveFriendList();
    virtual EECode RetrieveAdminDeviceList();
    virtual EECode RetrieveSharedDeviceList();
    virtual EECode QueryDeivceInfo(TUniqueID device_id);

public:
    virtual void SetIdentificationString(TChar *p_id_string, TU32 length = DIdentificationStringLength);

public:
    virtual EECode Login(TUniqueID id, TChar *authencation_string, TChar *server_url, TU16 server_port, TU16 local_port, TChar *&cloud_server_url, TU16 &cloud_server_port, TU64 &dynamic_code_input);
    virtual EECode LoginUserAccount(TChar *name, TChar *password, const TChar *server_url, TU16 serverport);
    virtual EECode Logout();

public:
    virtual EECode SetReceiveMessageCallBack(void *owner, TReceiveMessageCallBack callback);

public:
    virtual EECode PostMessage(TUniqueID id, TU8 *message, TU32 message_length);
    virtual EECode SendMessage(TUniqueID id, TU8 *message, TU32 message_length);

public:
    virtual void Destroy();

public:
    static EECode __Entry(void *param);
    EECode ReadLoop(void *param);

private:
    EECode logout();
    EECode startReadLoop();
    void stopReadLoop();
    void fillHeader(TU32 type, TU32 size, TU16 ext_size, TU8 ext_type);

private:
    EECode sendData(TU8 *data, TU16 size, TU8 maxcount = 2);
    EECode recvData(TU8 *data, TU16 size, TU8 maxcount = 2);
    void initSendBuffer(ESACPClientCmdSubType sub_type, EProtocolHeaderExtentionType ext_type, TU32 payload_size, ESACPTypeClientCategory cat_type = ESACPTypeCategory_ConnectionTag);
    EECode handleMessage(TU32 reqres_bits, TU32 cat, TU32 sub_type, TU8 *payload, TU16 payload_size);

private:
    TU16 mLocalPort;
    TU16 mServerPort;

    TU8 mbConnected;
    TU8 mbAuthenticated;
    TU8 mbReceiveConnectStatus;
    TU8 mbStartReadLoop;

    TInt mSocket;

    TU32 mSeqCount;
    TU8 mDataType;
    TU8 mnHeaderLength;

    TU8 mEncryptionType1;
    TU8 mEncryptionType2;

    TChar mIdentityString[DIdentificationStringLength + 1];
    TU32 mDeviceIdentityStringLen;
    TU8 mReserved0, mReserved1, mReserved2;
    TChar mCloudServerUrl[DMaxServerUrlLength];

private:
    TU8 *mpSenderBuffer;
    TU32 mSenderBufferSize;

private:
    TU8 *mpReadBuffer;
    TU32 mReadBufferSize;

    TU8 *mpPayloadBuffer;
    TU32 mPayloadBufferSize;

private:
    SSACPHeader *mpSACPHeader;

private:
    TU32 mLastErrorHint1;
    TU32 mLastErrorHint2;
    EECode mLastErrorCode;

private:
    void *mpMessageCallbackOwner;
    TReceiveMessageCallBack mpMessageCallback;

private:
    IThread *mpThread;

    TSocketHandler mPipeFd[2];
    TSocketHandler mMaxFd;

private:
    IProtocolHeaderParser *mpProtocolHeaderParser;
    TU16 mnExtensionLength;

    TUniqueID mUniqueID;

    TUniqueID *mpIDListBuffer;
    TU32 mnIDListBufferLength;
};

//-----------------------------------------------------------------------
//
//  CSACPIMClientAgent
//
//-----------------------------------------------------------------------

class CSACPIMClientAgent: public IIMClientAgent
{
public:
    static CSACPIMClientAgent *Create();

protected:
    EECode Construct();

    CSACPIMClientAgent();
    virtual ~CSACPIMClientAgent();

public:
    virtual EECode Register(TChar *name, TChar *password, const TChar *server_url, TU16 server_port, TUniqueID &id);

public:
    virtual EECode AcquireOwnership(TChar *device_code, TUniqueID &device_id);
    virtual EECode DonateOwnership(TChar *donate_target, TUniqueID device_id);

public:
    virtual EECode GrantPrivilegeSingleDevice(TUniqueID device_id, TUniqueID usr_id, TU32 privilege);
    virtual EECode GrantPrivilege(TUniqueID *device_id, TU32 device_count, TUniqueID usr_id, TU32 privilege);
    virtual EECode AddFriend(TChar *friend_user_name, TUniqueID &friend_id);
    virtual EECode AcceptFriend(TUniqueID friend_id);
    virtual EECode RemoveFriend(TUniqueID friend_id);

    virtual EECode RetrieveFriendList(SAgentIDListBuffer *&p_id_list);
    virtual EECode RetrieveAdminDeviceList(SAgentIDListBuffer *&p_id_list);
    virtual EECode RetrieveSharedDeviceList(SAgentIDListBuffer *&p_id_list);
    //release SAgentIDListBuffer for upper three functions
    virtual void FreeIDList(SAgentIDListBuffer *id_list);

    virtual EECode QueryDeivceProfile(TUniqueID device_id, SDeviceProfile *p_profile);

public:
    virtual EECode Login(TUniqueID id, TChar *password, TChar *server_url, TU16 server_port, TChar *&cloud_server_url, TU16 &cloud_server_port, TU64 &dynamic_code_input);
    virtual EECode LoginUserAccount(TChar *name, TChar *password, const TChar *server_url, TU16 serverport);
    virtual EECode LoginByUserID(TUniqueID id, TChar *password, const TChar *server_url, TU16 server_port);
    virtual EECode Logout();

public:
    virtual EECode PostMessage(TUniqueID id, TU8 *message, TU32 message_length);
    virtual EECode SendMessage(TUniqueID id, TU8 *message, TU32 message_length);

public:
    virtual void GetLastConnectErrorCode(ESACPConnectResult &connect_error_code);

public:
    virtual void Destroy();

private:
    EECode logout();
    void fillHeader(TU32 type, TU32 size, TU16 ext_size, TU8 ext_type);
    EECode sendData(TU8 *data, TU16 size, TU8 maxcount = 2);
    EECode recvData(TU8 *data, TU16 size, TU8 maxcount = 2);
    void initSendBuffer(ESACPClientCmdSubType sub_type, EProtocolHeaderExtentionType ext_type, TU32 payload_size, ESACPTypeClientCategory cat_type = ESACPTypeCategory_ConnectionTag);
    EECode readLoop(TU32 &payload_size, TU32 &reqres_bits, TU32 &cat, TU32 &sub_type);
    EECode retrieveIdList(ESACPClientCmdSubType sub_type, SAgentIDListBuffer *&p_id_list);

private:
    TU16 mLocalPort;
    TU16 mServerPort;

    TU8 mbConnected;
    TU8 mbAuthenticated;
    TU8 mbReceiveConnectStatus;
    TU8 mbStartReadLoop;

    TInt mSocket;

    TU32 mSeqCount;
    TU8 mDataType;
    TU8 mnHeaderLength;

    TU8 mEncryptionType1;
    TU8 mEncryptionType2;

    TU8 mIdentityString[DDynamicInputStringLength + DIdentificationStringLength + 4];
    TChar mCloudServerUrl[256];

private:
    TU8 *mpSenderBuffer;
    TU32 mSenderBufferSize;

private:
    TU8 *mpReadBuffer;
    TU32 mReadBufferSize;

    TU8 *mpPayloadBuffer;
    TU32 mPayloadBufferSize;

private:
    SSACPHeader *mpSACPHeader;

private:
    TU32 mLastErrorHint1;
    TU32 mLastErrorHint2;
    EECode mLastErrorCode;

private:
    void *mpMessageCallbackOwner;
    TReceiveMessageCallBack mpMessageCallback;

private:
    IThread *mpThread;

    TSocketHandler mPipeFd[2];
    TSocketHandler mMaxFd;

private:
    IProtocolHeaderParser *mpProtocolHeaderParser;
    TU16 mnExtensionLength;

    TUniqueID mUniqueID;

    TUniqueID *mpIDListBuffer;
    TU32 mnIDListBufferLength;

    TU32 mLastConnectErrorCode;
    SAgentIDListBuffer mIDListBuffer;
};

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif

