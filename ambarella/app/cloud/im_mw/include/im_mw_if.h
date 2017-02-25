/*******************************************************************************
 * im_mw_if.h
 *
 * History:
 *  2014/05/26 - [Zhi He] create file
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

#ifndef __IM_MW_IF_H__
#define __IM_MW_IF_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN
DCODE_DELIMITER;

typedef enum {
    EAccountGroup_None = 0x0,
    EAccountGroup_SourceDevice = 0x01,
    EAccountGroup_UserAccount = 0x02,
    EAccountGroup_CloudDataNode = 0x03,
    //EAccountGroup_CloudMsgNode = 0x04,
    EAccountGroup_CloudControlNode = 0x05,
    EAccountGroup_Admin = 0x06,
    EAccountGroup_AdminDev = 0x07,
} EAccountGroup;

typedef enum {
    EAccountCompany_Ambarella = 0x0,
} EAccountCompany;

//-----------------------------------------------------------------------
//
// tricky struct definition, do not modify them
//
//-----------------------------------------------------------------------

typedef struct {
    TChar name[DMAX_ACCOUNT_NAME_LENGTH_EX];
    TChar nickname[DMAX_ACCOUNT_NAME_LENGTH_EX];
    TChar password[DIdentificationStringLength];
} SAccountInfoBase;

typedef struct {
    TChar chipinfo[16];
    TUniqueID ownerid;
    TChar production_code[DMAX_PRODUCTION_CODE_LENGTH];

    //TChar dataserver_address[DMaxServerUrlLength];
    //TSocketPort cloud_port;
    //TSocketPort rtsp_streaming_port;

    TU32 storage_capacity;// N x 24hours
} SAccountExtensionSourceDevice;

typedef struct {
    TChar secure_question1[DMAX_SECURE_QUESTION_LENGTH];
    TChar secure_question2[DMAX_SECURE_QUESTION_LENGTH];
    TChar secure_answer1[DMAX_SECURE_ANSWER_LENGTH];
    TChar secure_answer2[DMAX_SECURE_ANSWER_LENGTH];

    //TChar phone[32];
    //TChar mail[DMAX_ACCOUNT_INFO_LENGTH];
} SAccountExtensionUser;

typedef struct {
    TChar url[DMaxServerUrlLength];

    TSocketPort admin_port;
    TSocketPort cloud_port;
    TSocketPort rtsp_streaming_port;
    TU16 reserved0;
    TU32 node_index;
} SAccountExtensionCloudDataNode;

typedef struct {
    TChar address[32];// ip + port
} SAccountExtensionCloudControlNode;

typedef struct {
    TU8 can_control_data_node;
    TU8 can_control_control_node;
    TU8 can_debug;
    TU8 reserved0;
} SAccountExtensionAdmin;

typedef struct {
    TChar nickname[DMAX_ACCOUNT_NAME_LENGTH_EX];
} SAccountExtraInfoSourceDevice;

#define DFLAG1_ACCOUNT_GENERATED_UNIQUEID (0x1 << 0)

typedef struct {
    SMemVariableHeader header;

    SAccountInfoBase base;

    void *p_agent;
} SAccountInfoRoot;

typedef struct {
    SAccountInfoRoot root;
    SAccountExtensionSourceDevice ext;

    TU16 friendsnum;
    TU16 friendsnum_max;
    //variable length, load/store needed, store format use tlv16
    TUniqueID *p_sharedfriendsid;
    TU32 *p_privilege_mask;
} SAccountInfoSourceDevice;

typedef struct {
    SAccountInfoRoot root;
    SAccountExtensionUser ext;

    TU16 admindevicenum;
    TU16 admindevicenum_max;
    TU16 shareddevicenum;
    TU16 shareddevicenum_max;
    TU16 friendsnum;
    TU16 friendsnum_max;

    //variable length, load/store needed, store format use tlv16
    TUniqueID *p_admindeviceid;
    TUniqueID *p_shareddeviceid;
    TUniqueID *p_friendsid;

    //offline message
    TU32 offline_message_length;
    TU32 offline_message_buffer_length;
    TU8 *p_offline_message;
} SAccountInfoUser;

typedef struct {
    SAccountInfoRoot root;
    SAccountExtensionCloudDataNode ext;

    TU16 max_channel_number;
    TU16 current_channel_number;
    TUniqueID *p_channel_id;
} SAccountInfoCloudDataNode;

typedef struct {
    SAccountInfoRoot root;
    SAccountExtensionCloudControlNode ext;
} SAccountInfoCloudControlNode;

typedef struct {
    SAccountInfoRoot root;
    SAccountExtensionAdmin ext;
} SAccountInfoAdmin;

class CICommunicationClientPort;
typedef struct {
    TChar address[32];
    TSocketPort admin_port;
    TSocketPort cloud_port;
    TSocketPort rtsp_streaming_port;
    TU16 id;
    TU32 max_connection_num;
    TU32 connection_num;

    CICommunicationClientPort *p_clientport;
} SDataNodeInfo;

//-----------------------------------------------------------------------
//
// IAccountManager
//
//-----------------------------------------------------------------------

class IAccountManager
{
public:
    virtual void PrintStatus0() = 0;

public:
    //virtual EECode UpdateNetworkAgent(SAccountInfoRoot* root, void* agent, void* pre_agent) = 0;

    virtual EECode NewAccount(TChar *account, TChar *password, EAccountGroup group, TUniqueID &id, EAccountCompany company, TChar *product_code = NULL, TU32 product_code_length = 0) = 0;
    virtual EECode DeleteAccount(TUniqueID id) = 0;
    virtual EECode DeleteAccount(TChar *account, EAccountGroup group) = 0;

    virtual EECode QueryAccount(TUniqueID id, SAccountInfoRoot *&info) = 0;
    virtual EECode QuerySourceDeviceAccountByName(TChar *name, SAccountInfoSourceDevice *&info) = 0;
    virtual EECode QueryUserAccountByName(TChar *name, SAccountInfoUser *&info) = 0;
    virtual EECode QueryDataNodeByName(TChar *name, SAccountInfoCloudDataNode *&info) = 0;
    //virtual EECode Authenticate(TUniqueID id, TChar* password) = 0;

    //admin/cloud related, remote agent must not invoke those API
    virtual EECode NewDataNode(TChar *url, TChar *password, TUniqueID &id, TSocketPort admin_port, TSocketPort cloud_port, TSocketPort rtsp_streaming_port, TU32 max_channel_number, TU32 cur_used_channel_number) = 0;
    virtual EECode DeleteDataNode(TChar *url, TSocketPort admin_port) = 0;
    virtual EECode QueryAllDataNodeID(TUniqueID *&p_id, TU32 &data_node_number) = 0;
    virtual EECode QueryAllDataNodeInfo(SAccountInfoCloudDataNode ** &p_info, TU32 &data_node_number) = 0;
    virtual EECode QueryDataNodeInfoByIndex(TU32 index, SAccountInfoCloudDataNode *&p_info) = 0;

    virtual EECode NewUserAccount(TChar *account, TChar *password, TUniqueID &id, EAccountCompany company, SAccountInfoUser *&p_user) = 0;

    virtual EECode ConnectDataNode(TUniqueID id) = 0;
    virtual EECode ConnectAllDataNode() = 0;
    virtual EECode DisconnectAllDataNode() = 0;
    virtual EECode SendDataToDataNode(TU32 index, TU8 *p_data, TU32 size) = 0;
    virtual EECode UpdateUserDynamicCode(TU32 index, TChar *user_name, TChar *device_name, TU32 dynamic_code) = 0;
    virtual EECode UpdateDeviceDynamicCode(TU32 index, TChar *name, TU32 dynamic_code) = 0;
    virtual EECode UpdateDeviceDynamicCode(const SAccountInfoCloudDataNode *datanode, TChar *name, TU32 dynamic_code) = 0;

public:
    //for user account related API blow, should only be invoked after userid authentication(or make sure the userid is exactly same with agent's id)

    //check the device's ownership via "product code"
    virtual EECode OwnSourceDevice(TUniqueID userid, TUniqueID deviceid) = 0;
    virtual EECode OwnSourceDeviceByUserContext(SAccountInfoUser *p_user, TUniqueID deviceid) = 0;
    virtual EECode SourceSetOwner(SAccountInfoSourceDevice *p_device, SAccountInfoUser *p_user) = 0;

    virtual EECode InviteFriend(SAccountInfoUser *p_user, TChar *name, TUniqueID &friendid) = 0;
    virtual EECode InviteFriendByID(SAccountInfoUser *p_user, TUniqueID friendid, TChar *name) = 0;
    virtual EECode AcceptFriend(SAccountInfoUser *p_user, TUniqueID friendid) = 0;
    virtual EECode RemoveFriend(SAccountInfoUser *p_user, TUniqueID friendid) = 0;

    //update privilege include sharing, remove sharing
    virtual EECode UpdateUserPrivilege(TUniqueID userid, TUniqueID target_userid, TU32 privilege_mask, TU32 number_of_devices, TUniqueID *p_list_of_devices) = 0;
    virtual EECode UpdateUserPrivilegeSingleDevice(TUniqueID userid, TUniqueID target_userid, TU32 privilege_mask, TUniqueID deviceid) = 0;
    virtual EECode UpdateUserPrivilegeSingleDeviceByContext(SAccountInfoUser *p_user, TUniqueID target_userid, TU32 privilege_mask, TUniqueID deviceid) = 0;

    //if target_userid is 0, then the device will back to without owner's stage, then the product code can prove device's ownship again, it's not very safe so is not recommanded.
    virtual EECode DonateSourceDeviceOwnship(TUniqueID userid, TUniqueID target_userid, TUniqueID deviceid) = 0;
    virtual EECode DonateSourceDeviceOwnshipByContext(SAccountInfoUser *p_user, TUniqueID target_userid, TUniqueID deviceid) = 0;

    //
    virtual EECode AcceptSharing(TUniqueID userid, TUniqueID deviceid) = 0;
    virtual EECode AcceptDonation(TUniqueID userid, TUniqueID deviceid) = 0;

    virtual EECode RetrieveUserOwnedDeviceList(TUniqueID userid, TU32 &count, TUniqueID *&id_list) = 0;
    virtual EECode RetrieveUserFriendList(TUniqueID userid, TU32 &count, TUniqueID *&id_list) = 0;
    virtual EECode RetrieveUserSharedDeviceList(TUniqueID userid, TU32 &count, TUniqueID *&id_list) = 0;

public:
    virtual EECode LoadFromDataBase(TChar *source_device_file_name, TChar *source_device_ext_file_name, TChar *user_account_filename, TChar *user_account_ext_filename, TChar *data_node_filename, TChar *data_node_ext_filename, TChar *data_node_list_filename, TChar *control_node_filename, TChar *control_node_ext_filename, TUniqueID id = 0) = 0;
    virtual EECode StoreToDataBase(TChar *source_device_file_name, TChar *source_device_ext_file_name, TChar *user_account_filename, TChar *user_account_ext_filename, TChar *data_node_filename, TChar *data_node_ext_filename, TChar *data_node_list_filename, TChar *control_node_filename, TChar *control_node_ext_filename, TUniqueID id = 0) = 0;

    //privilege check
public:
    virtual EECode PrivilegeQueryDevice(TUniqueID user_id, TUniqueID device_id, SAccountInfoSourceDevice *&p_device) = 0;
    virtual EECode PrivilegeQueryUser(SAccountInfoUser *p_user, TUniqueID friend_id, SAccountInfoUser *&p_friend) = 0;

public:
    virtual void Destroy() = 0;

protected:
    virtual ~IAccountManager() {}
};

typedef enum {
    EAccountManagerType_none = 0x0,

    EAccountManagerType_generic = 0x01,
} EAccountManagerType;

IAccountManager *gfCreateAccountManager(EAccountManagerType type, const volatile SPersistCommonConfig *pPersistCommonConfig, IMsgSink *pMsgSink, TUint index);

//-----------------------------------------------------------------------
//
// CNetworkAgent
//
//-----------------------------------------------------------------------

class CNetworkAgent: public IScheduledClient
{
public:
    static CNetworkAgent *Create(IAccountManager *p_account_manager, SAccountInfoRoot *p_account, EProtocolType type = EProtocolType_SACP);
    virtual void Destroy();

protected:
    CNetworkAgent(IAccountManager *p_account_manager, SAccountInfoRoot *p_account);
    virtual ~CNetworkAgent();
    EECode Construct(EProtocolType type);

public:
    EAccountGroup GetAccountGroup() const;
    TUniqueID GetUniqueID() const;
    EECode SetUniqueID(TUniqueID id);
    TSocketHandler GetSocket() const;
    EECode SetSocket(TSocketHandler socket);

public:
    EECode SendData(TU8 *p_data, TU32 size);

public:
    virtual EECode Scheduling(TUint times = 1, TU32 inout_mask = 0);

public:
    virtual TInt IsPassiveMode() const;
    virtual TSchedulingHandle GetWaitHandle() const;
    virtual TU8 GetPriority() const;

    virtual EECode CheckTimeout();
    virtual EECode EventHandling(EECode err);

public:
    virtual TSchedulingUnit HungryScore() const;

private:
    CNetworkAgent *findTargetInRecentList(TUniqueID target);
    EECode relay(TUniqueID target, TUniqueID source, TU8 *p_data, TU32 size);

    EECode sendData(TU8 *p_data, TU32 size);
    EECode handleRequestFromClient(TU8 *payload, TU16 payload_size, TU8 sub_type);
    void replyToClient(EECode error_code, TU8 *header, TU16 length);
    void notifyToClient(TU8 error_code, TU8 *header, TU16 length);
    void notifyTargetNotReachable(TUniqueID target_id);
    void notifyAllFriendsOffline();

protected:
    IAccountManager *mpAccountManager;
    IProtocolHeaderParser *mpProtocolHeaderParser;
    SAccountInfoRoot *mpAccountInfo;
    IMutex *mpMutex;

protected:
    TUniqueID mUniqueID;
    TSocketHandler mSocket;

protected:
    CIDoubleLinkedList *mpContectList;

protected:
    TU8 *mpReadBuffer;
    TU8 *mpCurrentReadPtr;
    TU32 mnRemainingSize;
    TU32 mnHeaderLength;

    TU8 *mpWriteBuffer;
    TU32 mnWriteBufferLength;

    TUniqueID *mpIDListBuffer;
    TU32 mnIDListBufferLength;

private:
    TU8 mbTobeMandatoryDelete;
    TU8 mReserved0;
    TU8 mReserved1;
    TU8 mReserved2;
};

//-----------------------------------------------------------------------
//
// IIMServer
//
//-----------------------------------------------------------------------

class IIMServer
{
public:
    virtual void Destroy() = 0;
    virtual void PrintStatus() = 0;

protected:
    virtual ~IIMServer() {}

public:
    virtual EECode HandleServerRequest(TSocketHandler &max, void *all_set) = 0;

    virtual EECode GetHandler(TInt &handle) const = 0;

    virtual EECode GetServerState(EServerState &state) const = 0;
    virtual EECode SetServerState(EServerState state) = 0;
};

//-----------------------------------------------------------------------
//
// IIMServerManager
//
//-----------------------------------------------------------------------

class IIMServerManager
{
public:
    virtual void Destroy() = 0;
    virtual void PrintStatus0() = 0;

protected:
    virtual ~IIMServerManager() {}

public:
    virtual EECode StartServerManager() = 0;
    virtual EECode StopServerManager() = 0;
    virtual EECode AddServer(IIMServer *server) = 0;
    virtual EECode RemoveServer(IIMServer *server) = 0;
};

IIMServerManager *gfCreateIMServerManager(const volatile SPersistCommonConfig *pconfig, IMsgSink *pMsgSink, const CIClockReference *p_system_clock_reference);

IIMServer *gfCreateIMServer(EProtocolType type, const volatile SPersistCommonConfig *pPersistCommonConfig, IMsgSink *pMsgSink, IAccountManager *p_account_manager, TU16 server_port);

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif

