/*******************************************************************************
 * generic_account_manager.h
 *
 * History:
 *  2014/06/12 - [Zhi He] create file
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

#ifndef __GENERIC_ACCOUNT_MANAGER_H__
#define __GENERIC_ACCOUNT_MANAGER_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN
DCODE_DELIMITER;

//-----------------------------------------------------------------------
//
//  CGenericAccountManager
//
//-----------------------------------------------------------------------

class CGenericAccountManager: public CObject, public IAccountManager
{
    typedef CObject inherited;

public:
    static IAccountManager *Create(const TChar *pName, const volatile SPersistCommonConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TUint index);

protected:
    CGenericAccountManager(const TChar *pName, const volatile SPersistCommonConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TUint index);

    EECode Construct();
    virtual ~CGenericAccountManager();

public:
    virtual void Delete();
    virtual void PrintStatus();
    virtual void PrintStatus0();

public:
    //virtual EECode UpdateNetworkAgent(SAccountInfoRoot* root, void* agent, void* pre_agent);
    virtual EECode NewAccount(TChar *account, TChar *password, EAccountGroup group, TUniqueID &id, EAccountCompany company, TChar *product_code = NULL, TU32 product_code_length = 0);
    virtual EECode DeleteAccount(TChar *account, EAccountGroup group);
    virtual EECode DeleteAccount(TUniqueID id);

    virtual EECode QueryAccount(TUniqueID id, SAccountInfoRoot *&info);
    virtual EECode QuerySourceDeviceAccountByName(TChar *name, SAccountInfoSourceDevice *&info);
    virtual EECode QueryUserAccountByName(TChar *name, SAccountInfoUser *&info);
    virtual EECode QueryDataNodeByName(TChar *name, SAccountInfoCloudDataNode *&info);
    //virtual EECode Authenticate(TUniqueID id, TChar* password);

    //admin/cloud related, remote agent must not invoke those API
    virtual EECode NewDataNode(TChar *url, TChar *password, TUniqueID &id, TSocketPort admin_port, TSocketPort cloud_port, TSocketPort rtsp_streaming_port, TU32 max_channel_number, TU32 cur_used_channel_number);
    virtual EECode DeleteDataNode(TChar *url, TSocketPort admin_port);
    virtual EECode QueryAllDataNodeID(TUniqueID *&p_id, TU32 &data_node_number);
    virtual EECode QueryAllDataNodeInfo(SAccountInfoCloudDataNode ** &p_info, TU32 &data_node_number);
    virtual EECode QueryDataNodeInfoByIndex(TU32 index, SAccountInfoCloudDataNode *&p_info);

    virtual EECode NewUserAccount(TChar *account, TChar *password, TUniqueID &id, EAccountCompany company, SAccountInfoUser *&p_user);

    virtual EECode ConnectDataNode(TUniqueID id);
    virtual EECode ConnectAllDataNode();
    virtual EECode DisconnectAllDataNode();
    virtual EECode SendDataToDataNode(TU32 index, TU8 *p_data, TU32 size);
    //send cmd to data node
    virtual EECode UpdateUserDynamicCode(TU32 index, TChar *user_name, TChar *device_name, TU32 dynamic_code);
    virtual EECode UpdateDeviceDynamicCode(TU32 index, TChar *name, TU32 dynamic_code);
    virtual EECode UpdateDeviceDynamicCode(const SAccountInfoCloudDataNode *datanode, TChar *name, TU32 dynamic_code);

public:
    //for user account related API blow, should only be invoked after userid authentication(or make sure the userid is exactly same with agent's id)

    //check the device's ownership via "product code"
    virtual EECode OwnSourceDevice(TUniqueID userid, TUniqueID deviceid);
    virtual EECode OwnSourceDeviceByUserContext(SAccountInfoUser *p_user, TUniqueID deviceid);
    virtual EECode SourceSetOwner(SAccountInfoSourceDevice *p_device, SAccountInfoUser *p_user);

    virtual EECode InviteFriend(SAccountInfoUser *p_user, TChar *name, TUniqueID &friendid);
    virtual EECode InviteFriendByID(SAccountInfoUser *p_user, TUniqueID friendid, TChar *name);
    virtual EECode AcceptFriend(SAccountInfoUser *p_user, TUniqueID friendid);
    virtual EECode RemoveFriend(SAccountInfoUser *p_user, TUniqueID friendid);

    //update privilege include sharing, remove sharing
    virtual EECode UpdateUserPrivilege(TUniqueID userid, TUniqueID target_userid, TU32 privilege_mask, TU32 number_of_devices, TUniqueID *p_list_of_devices);
    virtual EECode UpdateUserPrivilegeSingleDevice(TUniqueID userid, TUniqueID target_userid, TU32 privilege_mask, TUniqueID deviceid);
    virtual EECode UpdateUserPrivilegeSingleDeviceByContext(SAccountInfoUser *p_user, TUniqueID target_userid, TU32 privilege_mask, TUniqueID deviceid);

    //if target_userid is 0, then the device will back to without owner's stage, then the product code can prove device's ownship again, it's not very safe so is not recommanded.
    virtual EECode DonateSourceDeviceOwnship(TUniqueID userid, TUniqueID target_userid, TUniqueID deviceid);
    virtual EECode DonateSourceDeviceOwnshipByContext(SAccountInfoUser *p_user, TUniqueID target_userid, TUniqueID deviceid);

    //
    virtual EECode AcceptSharing(TUniqueID userid, TUniqueID deviceid);
    virtual EECode AcceptDonation(TUniqueID userid, TUniqueID deviceid);

    virtual EECode RetrieveUserOwnedDeviceList(TUniqueID userid, TU32 &count, TUniqueID *&id_list);
    virtual EECode RetrieveUserFriendList(TUniqueID userid, TU32 &count, TUniqueID *&id_list);
    virtual EECode RetrieveUserSharedDeviceList(TUniqueID userid, TU32 &count, TUniqueID *&id_list);

public:
    virtual EECode LoadFromDataBase(TChar *source_device_file_name, TChar *source_device_ext_file_name, TChar *user_account_filename, TChar *user_account_ext_filename, TChar *data_node_filename, TChar *data_node_ext_filename, TChar *data_node_list_filename, TChar *control_node_filename, TChar *control_node_ext_filename, TUniqueID id = 0);
    virtual EECode StoreToDataBase(TChar *source_device_file_name, TChar *source_device_ext_file_name, TChar *user_account_filename, TChar *user_account_ext_filename, TChar *data_node_filename, TChar *data_node_ext_filename, TChar *data_node_list_filename, TChar *control_node_filename, TChar *control_node_ext_filename, TUniqueID id = 0);

public:
    virtual EECode PrivilegeQueryDevice(TUniqueID user_id, TUniqueID device_id, SAccountInfoSourceDevice *&p_device);
    virtual EECode PrivilegeQueryUser(SAccountInfoUser *p_user, TUniqueID friend_id, SAccountInfoUser *&p_friend);

public:
    virtual void Destroy();

public:
    static EECode CallBackReadSourceAccount(void *context, IIO *p_io, TU32 datasize, void *p_node);
    static EECode CallBackReadUserAccount(void *context, IIO *p_io, TU32 datasize, void *p_node);
    static EECode CallBackReadCloudDataNode(void *context, IIO *p_io, TU32 datasize, void *p_node);

    static EECode CallBackWriteSourceAccount(void *context, IIO *p_io, void *p_node);
    static EECode CallBackWriteUserAccount(void *context, IIO *p_io, void *p_node);
    static EECode CallBackWriteCloudDataNode(void *context, IIO *p_io, void *p_node);

    static EECode CallBackInitialize(void *context, void *p_node);

private:
    EECode newSourceDeviceAccount(TChar *account, TChar *password, SAccountInfoSourceDevice *&ret);
    EECode newUserAccount(TChar *account, TChar *password, SAccountInfoUser *&ret);
    EECode newCloudDataNodeAccount(TChar *account, TChar *password, TSocketPort admin_port, SAccountInfoCloudDataNode *&ret);
    EECode newCloudControlNodeAccount(TChar *account, TChar *password, SAccountInfoCloudControlNode *&ret);
    EECode queryAccount(TUniqueID id, SAccountInfoRoot *&p_info);

    EECode querySourceDeviceAccount(TUniqueID id, SAccountInfoSourceDevice *&p_info);
    EECode querySourceDeviceAccountByName(TChar *name, SAccountInfoSourceDevice *&info);
    EECode queryUserAccount(TUniqueID id, SAccountInfoUser *&p_info);
    EECode queryUserAccountByName(TChar *name, SAccountInfoUser *&info);
    EECode queryDataNodeAccount(TUniqueID id, SAccountInfoCloudDataNode *&p_info);
    EECode queryDataNodeAccountByName(TChar *name, SAccountInfoCloudDataNode *&info);
    EECode queryControlNodeAccount(TUniqueID id, SAccountInfoCloudControlNode *&p_info);
    EECode userAddFriend(SAccountInfoUser *p_user, TUniqueID friendid);
    void userRemoveFriend(SAccountInfoUser *p_user, TUniqueID friendid);
    EECode userAddOwnedDevice(SAccountInfoUser *p_user, TUniqueID deviceid);
    void userRemoveOwnedDevice(SAccountInfoUser *p_user, TUniqueID deviceid);
    EECode userAddSharedDevice(SAccountInfoUser *p_user, TUniqueID deviceid);
    void userRemoveSharedDevice(SAccountInfoUser *p_user, TUniqueID deviceid);
    EECode deviceAddOrUpdateSharedUser(SAccountInfoSourceDevice *p_device, TUniqueID friendid, TU32 privilege_mask);
    void deviceRemoveSharedUser(SAccountInfoSourceDevice *p_device, TUniqueID friendid);
    TU32 isInDeviceSharedlist(SAccountInfoSourceDevice *p_device, TUniqueID friendid);
    TU32 isInUserFriendList(SAccountInfoUser *p_user, TUniqueID friendid);

    EECode getCurrentAvaiableGroupIndex(TU32 &ret_index, SAccountInfoCloudDataNode *&p_data_node);
    EECode dataNodeAddChannel(SAccountInfoCloudDataNode *p_data_node, SAccountInfoSourceDevice *p_device);

    EECode connectDataNode(SAccountInfoCloudDataNode *p_datanode_info);

private:
    const volatile SPersistCommonConfig *mpPersistCommonConfig;
    IMsgSink *mpMsgSink;
    IMutex *mpMutex;

private:
    ILightWeightDataBase *mpSourceDeviceAccount;
    ILightWeightDataBase *mpUserAccount;
    ILightWeightDataBase *mpCloudDataNodeAccount;
    ILightWeightDataBase *mpCloudControlNodeAccount;

private:
    SAccountInfoCloudDataNode **mpDataNodes;
    TUniqueID *mpDataNodesID;
    TU32 mMaxDataNodes;
    TU32 mCurrentDataNodes;

};


DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif

