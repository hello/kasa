/*******************************************************************************
 * im_sacp_client_agent_v2.h
 *
 * History:
 *  2014/08/08 - [Zhi He] create file
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

#ifndef __IM_SACP_CLIENT_AGENT_V2_H__
#define __IM_SACP_CLIENT_AGENT_V2_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN

DCODE_DELIMITER;

//-----------------------------------------------------------------------
//
//  CSACPIMClientAgentV2
//
//-----------------------------------------------------------------------

typedef struct {
    TU8 *p_send_buffer;
    TU32 send_buffer_size;
} SAgentSendBuffer;

class CSACPIMClientAgentV2: public IIMClientAgent
{
public:
    static CSACPIMClientAgentV2 *Create();

protected:
    EECode Construct();

    CSACPIMClientAgentV2();
    virtual ~CSACPIMClientAgentV2();

public:
    virtual EECode Register(TChar *name, TChar *password, const TChar *server_url, TSocketPort serverport, TUniqueID &id);
    virtual EECode GetSecurityQuestion(TChar *name, SSecureQuestions *p_questions, const TChar *server_url, TSocketPort serverport);
    virtual EECode ResetPasswordBySecurityAnswer(TChar *name, SSecureQuestions *p_questions, TChar *new_password, TUniqueID &id, const TChar *server_url, TU16 serverport);

public:
    virtual EECode AcquireOwnership(TChar *device_code, TUniqueID &id);
    virtual EECode DonateOwnership(TChar *donate_target, TUniqueID device_id);

public:
    virtual EECode GrantPrivilegeSingleDevice(TUniqueID device_id, TUniqueID usr_id, TU32 privilege = 0);
    virtual EECode GrantPrivilege(TUniqueID *device_id, TU32 device_count, TUniqueID usr_id, TU32 privilege = 0);
    virtual EECode AddFriend(TChar *friend_user_name, TUniqueID &friend_id);
    virtual EECode AddFriendByID(TUniqueID friend_id, TChar *friend_user_name);
    virtual EECode AcceptFriend(TUniqueID friend_id);
    virtual EECode RemoveFriend(TUniqueID friend_id);

    virtual EECode RetrieveFriendList(SAgentIDListBuffer *&id_list);
    virtual EECode RetrieveAdminDeviceList(SAgentIDListBuffer *&id_list);
    virtual EECode RetrieveSharedDeviceList(SAgentIDListBuffer *&id_list);
    virtual EECode QueryDeivceProfile(TUniqueID device_id, SDeviceProfile *p_profile);
    virtual EECode QueryFriendInfo(TUniqueID friend_id, SFriendInfo *info);

    virtual void FreeIDList(SAgentIDListBuffer *id_list);

public:
    virtual EECode Login(TUniqueID id, TChar *password, TChar *server_url, TSocketPort server_port, TChar *&cloud_server_url, TSocketPort &cloud_server_port, TU64 &dynamic_code_input);
    virtual EECode LoginUserAccount(TChar *name, TChar *password, const TChar *server_url, TSocketPort server_port, TUniqueID &id);
    virtual EECode LoginByUserID(TUniqueID id, TChar *password, const TChar *server_url, TSocketPort server_port);
    virtual EECode Logout();

public:
    virtual EECode SetOwner(TChar *owner);

public:
    virtual EECode PostMessage(TUniqueID id, TU8 *message, TU32 message_length, TU8 user_protocol);
    virtual EECode SendMessage(TUniqueID id, TU8 *message, TU32 message_length, TU8 user_protocol);

public:
    virtual EECode UpdateNickname(TChar *nickname);
    virtual EECode UpdateDeviceNickname(TUniqueID device_id, TChar *nickname);
    virtual EECode UpdatePassword(TChar *ori_password, TChar *password);
    virtual EECode UpdateSecurityQuestion(SSecureQuestions *p_questions, TChar *password);

public:
    virtual EECode AcquireDynamicCode(TUniqueID device_id, TChar *password, TU32 &dynamic_code);
    virtual EECode ReleaseDynamicCode(TUniqueID device_id);

public:
    virtual void SetCallBack(void *owner, TReceiveMessageCallBack callback, TSystemNotifyCallBack notify_callback, TErrorCallBack error_callback);

public:
    virtual void Destroy();

public:
    static EECode __Entry(void *param);
    EECode ReadLoop();

private:
    EECode startReadLoop();
    void stopReadLoop();

private:
    void handleServerCmd(TU32 need_reply, TU32 sub_type, TU32 session_number, TU8 *payload, TU32 payload_size, ESACPConnectResult ret_flag);
    void handleSystemNotification(TU32 sub_type, TU8 *payload, TU32 payload_size);

private:
    CIConditionSlot *allocMsgSlot();
    void releaseMsgSlot(CIConditionSlot *slot);
    SAgentSendBuffer *allocSendBuffer(TU32 max_buffer_size);
    void releaseSendBuffer(SAgentSendBuffer *buffer);
    SAgentIDListBuffer *allocIDListBuffer(TU32 max_id_number);
    void releaseIDListBuffer(SAgentIDListBuffer *buffer);

private:
    void releasePendingAPI();
    void releaseExpiredPendingAPI(SDateTime *time);
    CIConditionSlot *matchedMsgSlot(TU32 session_number, TU32 check_field);

private:
    IMutex *mpMutex;
    IMutex *mpResourceMutex;

private:
    TU16 mnHeaderLength;
    TU16 mServerPort;

    TU8 mbConnected;
    TU8 mbStartReadLoop;
    TU16 mSeqCount;

    TInt mSocket;

    TU8 mIdentityString[DDynamicInputStringLength + DIdentificationStringLength + 4];
    TChar mCloudServerUrl[256];

private:
    TU8 *mpReadBuffer;
    TU32 mReadBufferSize;

private:
    SSACPHeader *mpSACPHeader;

private:
    void *mpCallbackOwner;
    TReceiveMessageCallBack mpMessageCallback;
    TSystemNotifyCallBack mpSystemNotifyCallback;
    TErrorCallBack mpErrorCallback;

private:
    IThread *mpThread;

    TSocketHandler mPipeFd[2];
    TSocketHandler mMaxFd;

private:
    CIDoubleLinkedList *mpMsgSlotList;
    CIDoubleLinkedList *mpFreeMsgSlotList;
    CIDoubleLinkedList *mpSendBufferList;
    CIDoubleLinkedList *mpIDListBufferList;
};

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif

