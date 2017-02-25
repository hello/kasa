/*******************************************************************************
 * sacp_types.h
 *
 * History:
 *  2013/11/29 - [Zhi He] create file
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

#ifndef __SACP_TYPES_H__
#define __SACP_TYPES_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN

DCODE_DELIMITER;

typedef enum {
    ESACPTypeCategory_Invalid = 0x0,

    ESACPTypeCategory_ConnectionTag = 0x01,

    ESACPTypeCategory_AdminChannel = 0x02,

    ESACPTypeCategory_DataChannel = 0x04,

    //cmd between client and server
    ESACPTypeCategory_ClientCmdChannel = 0x10,

    //cmd between client and client
    ESACPTypeCategory_ClientRelayChannel = 0x11,

    //notfication to client
    ESACPTypeCategory_SystemNotification = 0x12,

} ESACPTypeClientCategory;

typedef enum {
    ESACPClientCmdSubType_Invalid = 0x0,

    //source device to authentication server
    //ESACPClientCmdSubType_SourceRegister = 0x01,
    //ESACPClientCmdSubType_SourceUnRegister = 0x02,
    ESACPClientCmdSubType_SourceAuthentication = 0x03,

    //sink device to authentication server
    ESACPClientCmdSubType_SinkRegister = 0x11,
    ESACPClientCmdSubType_SinkUnRegister = 0x12,
    ESACPClientCmdSubType_SinkAuthentication = 0x13,
    ESACPClientCmdSubType_SinkQuerySource = 0x14,
    ESACPClientCmdSubType_SinkQuerySourcePrivilege = 0x15,
    ESACPClientCmdSubType_SinkOwnSourceDevice = 0x16,
    ESACPClientCmdSubType_SinkDonateSourceDevice = 0x17,
    ESACPClientCmdSubType_SinkAcceptDeviceDonation = 0x18,
    ESACPClientCmdSubType_SinkInviteFriend = 0x19,
    ESACPClientCmdSubType_SinkAcceptFriend = 0x1a,
    ESACPClientCmdSubType_SinkRemoveFriend = 0x1b,
    ESACPClientCmdSubType_SinkUpdateFriendPrivilege = 0x1c,
    ESACPClientCmdSubType_SinkRetrieveFriendsList = 0x1d,
    ESACPClientCmdSubType_SinkRetrieveAdminDeviceList = 0x1e,
    ESACPClientCmdSubType_SinkRetrieveSharedDeviceList = 0x1f,
    ESACPClientCmdSubType_SinkQueryFriendInfo = 0x20,
    ESACPClientCmdSubType_SinkInviteFriendByID = 0x21,

    //source device to data server
    ESACPClientCmdSubType_SourceUpdateAudioParams = 0x24,
    ESACPClientCmdSubType_SourceUpdateVideoParams = 0x25,
    ESACPClientCmdSubType_DiscIndicator = 0x26,
    ESACPClientCmdSubType_VideoSync = 0x27,
    ESACPClientCmdSubType_AudioSync = 0x28,

    //sink device to data server
    ESACPClientCmdSubType_SinkStartPlayback = 0x32,
    ESACPClientCmdSubType_SinkStopPlayback = 0x33,
    ESACPClientCmdSubType_SinkUpdateStoragePlan = 0x34,
    ESACPClientCmdSubType_SinkSwitchSource = 0x35,
    ESACPClientCmdSubType_SinkQueryRecordHistory = 0x36,
    ESACPClientCmdSubType_SinkSeekTo = 0x37,
    ESACPClientCmdSubType_SinkFastForward = 0x38,
    ESACPClientCmdSubType_SinkFastBackward = 0x39,
    ESACPClientCmdSubType_SinkTrickplay = 0x3a,
    ESACPClientCmdSubType_SinkQueryCurrentStoragePlan = 0x3b,
    ESACPClientCmdSubType_SinkPTZControl = 0x3c,
    ESACPClientCmdSubType_SinkUpdateFramerate = 0x3d,
    ESACPClientCmdSubType_SinkUpdateBitrate = 0x3e,
    ESACPClientCmdSubType_SinkUpdateResolution = 0x3f,

    ESACPClientCmdSubType_SinkUpdateDisplayLayout = 0x40,
    ESACPClientCmdSubType_SinkZoom = 0x41,
    ESACPClientCmdSubType_SinkUpdateDisplayWindow = 0x42,
    ESACPClientCmdSubType_SinkFocusDisplayWindow = 0x43,
    ESACPClientCmdSubType_SinkSelectAudioSourceChannel = 0x44,
    ESACPClientCmdSubType_SinkSelectAudioTargetChannel = 0x45,
    ESACPClientCmdSubType_SinkSelectVideoHDChannel = 0x46,
    ESACPClientCmdSubType_SinkShowOtherWindows = 0x47,
    ESACPClientCmdSubType_SinkForceIDR = 0x48,
    ESACPClientCmdSubType_SinkSwapWindowContent = 0x49,
    ESACPClientCmdSubType_SinkCircularShiftWindowContent = 0x4a,
    ESACPClientCmdSubType_SinkZoomV2 = 0x4b,

    ESACPClientCmdSubType_SinkSwitchGroup = 0x50,
    ESACPClientCmdSubType_UpdateSinkNickname = 0x51,
    ESACPClientCmdSubType_UpdateSourceNickname = 0x52,
    ESACPClientCmdSubType_UpdateSinkPassword = 0x53,
    ESACPClientCmdSubType_SetSecureQuestion = 0x54,
    ESACPClientCmdSubType_GetSecureQuestion = 0x55,
    ESACPClientCmdSubType_ResetPassword = 0x56,
    ESACPClientCmdSubType_SourceSetOwner = 0x57,

    ESACPClientCmdSubType_PeerClose = 0x60,

    //misc
    ESACPClientCmdSubType_QueryVersion = 0x70,
    ESACPClientCmdSubType_QueryStatus = 0x71,
    ESACPClientCmdSubType_SyncStatus = 0x72,
    ESACPClientCmdSubType_QuerySourceList = 0x73,
    ESACPClientCmdSubType_AquireChannelControl = 0x74,
    ESACPClientCmdSubType_ReleaseChannelControl = 0x75,

    //for extention
    ESACPClientCmdSubType_CustomizedCommand = 0x80,

    //debug device to data server
    ESACPDebugCmdSubType_PrintCloudPipeline = 0x90,
    ESACPDebugCmdSubType_PrintStreamingPipeline = 0x91,
    ESACPDebugCmdSubType_PrintRecordingPipeline = 0x92,
    ESACPDebugCmdSubType_PrintSingleChannel = 0x93,
    ESACPDebugCmdSubType_PrintAllChannels = 0x94,

    ESACPClientCmdSubType_ResponceOnlyForRelay = 0x9a,

    ESACPClientCmdSubType_SystemNotifyFriendInvitation = 0xa0,
    ESACPClientCmdSubType_SystemNotifyDeviceDonation = 0xa1,
    ESACPClientCmdSubType_SystemNotifyDeviceSharing = 0xa2,
    ESACPClientCmdSubType_SystemNotifyUpdateOwnedDeviceList = 0xa3,
    ESACPClientCmdSubType_SystemNotifyDeviceOwnedByUser = 0xa4,

    ESACPClientCmdSubType_SinkAcquireDynamicCode = 0xa5,
    ESACPClientCmdSubType_SinkReleaseDynamicCode = 0xa6,

    ESACPClientCmdSubType_SystemNotifyTargetOffline = 0xa8,
    ESACPClientCmdSubType_SystemNotifyTargetNotReachable = 0xa9,

    //user customized protocol
    ESACPCmdSubType_UserCustomizedBase = 0xd0,

    ESACPCmdSubType_UserCustomizedDefaultTLV8 = ESACPCmdSubType_UserCustomizedBase,
    ESACPCmdSubType_UserCustomizedSmartSharing = 0xd5,
    // to be extended
} ESACPClientCmdSubType;

typedef enum {
    ESACPAdminSubType_Invalid = 0x0,

    //admin to authentication server
    ESACPAdminSubType_QuerySourceViaAccountName = 0x01,
    ESACPAdminSubType_QuerySourceViaAccountID = 0x02,
    //ESACPAdminSubType_QuerySourceViaAccountProductID = 0x03,
    ESACPAdminSubType_NewSourceAccount = 0x04,
    ESACPAdminSubType_DeleteSourceAccount = 0x05,
    ESACPAdminSubType_UpdateSourceAccount = 0x06,

    ESACPAdminSubType_QuerySinkViaAccountName = 0x21,
    ESACPAdminSubType_QuerySinkViaAccountID = 0x22,
    ESACPAdminSubType_QuerySinkViaAccountSecurityContact = 0x23,
    ESACPAdminSubType_NewSinkAccount = 0x24,
    ESACPAdminSubType_DeleteSinkAccount = 0x25,
    ESACPAdminSubType_UpdateSinkAccount = 0x26,

    ESACPAdminSubType_NewDataNode = 0x31,
    ESACPAdminSubType_UpdateDataNode = 0x32,
    ESACPAdminSubType_QueryAllDataNode = 0x33,
    ESACPAdminSubType_QueryDataNodeViaName = 0x34,
    ESACPAdminSubType_QueryDataNodeViaID = 0x35,

    //admin to data server
    ESACPAdminSubType_QueryDataServerStatus = 0x60,
    ESACPAdminSubType_AssignDataServerSourceDevice = 0x61,
    ESACPAdminSubType_RemoveDataServerSourceDevice = 0x62,
    ESACPAdminSubType_SetSourceDynamicPassword = 0x63,
    ESACPAdminSubType_AddSinkDynamicPassword = 0x64,
    ESACPAdminSubType_RemoveSinkDynamicPassword = 0x65,

    ESACPAdminSubType_UpdateDeviceDynamicCode = 0x67,// to be obsolete
    ESACPAdminSubType_UpdateUserDynamicCode = 0x68,// to be obsolete


    //admin query runtime info
    ESACPAdminSubType_QuerySourceAccountStatus = 0x81,
    ESACPAdminSubType_QuerySinkAccountStatus = 0x82,

    //admin to device
    ESACPAdminSubType_SetupDeviceFactoryInfo = 0xa1,
} ESACPAdminSubType;


#define DSACPTypeBit_Responce  (1 << 31)
#define DSACPTypeBit_Request    (1 << 30)
#define DSACPTypeBit_NeedReply    (1 << 29)
#define DSACPTypeBit_CatTypeMask    (0xff)
#define DSACPTypeBit_CatTypeBits    16
#define DSACPTypeBit_SubTypeMask    (0xffff)
#define DSACPTypeBit_SubTypeBits    0

#define DBuildSACPType(reqres_bits, cat, sub_type) \
    ((reqres_bits) | ((cat & DSACPTypeBit_CatTypeMask) << DSACPTypeBit_CatTypeBits) | ((sub_type & DSACPTypeBit_SubTypeMask) << DSACPTypeBit_SubTypeBits))

#ifdef BUILD_OS_WINDOWS
#pragma pack(push,1)
#endif
typedef struct {
    TU8 type_1;
    TU8 type_2;
    TU8 type_3;
    TU8 type_4;

    TU8 size_1;
    TU8 size_2;
    TU8 size_0;

    TU8 seq_count_0;
    TU8 seq_count_1;
    TU8 seq_count_2;

    TU8 encryption_type_1;
    TU8 encryption_type_2;
    //    TU8 encryption_type_3;
    //    TU8 encryption_type_4;

    TU8 flags;
    TU8 header_ext_type;
    TU8 header_ext_size_1;
    TU8 header_ext_size_2;
#ifdef BUILD_OS_WINDOWS
} SSACPHeader;
#pragma pack(pop)
#else
} __attribute__((packed)) SSACPHeader;
#endif


#define DGetSACPType(type, header) do { \
        SSACPHeader* h = (SSACPHeader*) (header); \
        type = (((TU32)h->type_1) << 24) | (((TU32)h->type_2) << 16) | (((TU32)h->type_3) << 8) | ((TU32)h->type_4); \
    } while (0)

#define DGetSACPSize(size, header) do { \
        SSACPHeader* h = (SSACPHeader*) (header); \
        size = (((TU32)h->size_0) << 16) | (((TU32)h->size_1) << 8) | ((TU32)h->size_2); \
    } while (0)

#define DGetSACPSeqNum(seq, header) do { \
        SSACPHeader* h = (SSACPHeader*) (header); \
        seq = (((TU32)h->seq_count_0) << 16) | (((TU32)h->seq_count_1) << 8) | ((TU32)h->seq_count_2); \
    } while (0)

#define DSetSACPType(type, header) do { \
        SSACPHeader* h = (SSACPHeader*) (header); \
        h->type_1 = ((type) >> 24) & 0xff; \
        h->type_2 = ((type) >> 16) & 0xff; \
        h->type_3 = ((type) >> 8) & 0xff; \
        h->type_4 = (type) & 0xff; \
    } while (0)

#define DSetSACPSize(size, header) do { \
        SSACPHeader* h = (SSACPHeader*) (header); \
        h->size_0 = ((size) >> 16) & 0xff; \
        h->size_1 = ((size) >> 8) & 0xff; \
        h->size_2 = (size) & 0xff; \
    } while (0)

#define DSetSACPRetFlag(flag, header) do { \
        SSACPHeader* h = (SSACPHeader*) (header); \
        h->flags = flag; \
    } while (0)

#define DSetSACPRetBits(bits, header) do { \
        SSACPHeader* h = (SSACPHeader*) (header); \
        h->type_1 = (bits >> 24) & 0xff; \
    } while (0)

//-----------------------------------------------------------------------
//
//  CICommunicationClientPort
//
//-----------------------------------------------------------------------
typedef EECode(*TCommunicationClientReadCallBack)(void *owner, TU8 *p_data, TU32 datasize, TU32 type, TU32 cat, TU32 subtype);

class CICommunicationClientPort
{
public:
    static CICommunicationClientPort *Create(void *p_context, TCommunicationClientReadCallBack callback);
    void Destroy();

protected:
    CICommunicationClientPort(void *p_context, TCommunicationClientReadCallBack callback);
    EECode Construct();
    ~CICommunicationClientPort();

public:
    EECode SetIdentifyString(TChar *url);
    EECode PrepareConnection(TChar *url, TU16 port, TChar *name, TChar *password);

    EECode ConnectServer();
    EECode LoopConnectServer(TU32 loop_interval);

    EECode ConnectLoop();

    EECode Disconnect();
    TInt Write(TU8 *data, TInt size);

    TU8 GetConnectionStatus() { return mbConnected;}

public:
    static EECode LoopConnectEntry(void *param);

public:
    //API to send cmd
    EECode UpdateUserDynamicCode(TChar *user_name, TChar *device_name, TU32 dynamic_code);
    EECode UpdateDeviceDynamicCode(TChar *name, TU32 dynamic_code);
    EECode AddDeviceChannel(TChar *name);

private:
    TInt write(TU8 *data, TInt size);
    EECode connectServer();
    void fillHeader(SSACPHeader *p_header, TU32 type, TU32 size, TU8 ext_type, TU16 ext_size, TU32 seq_count);
    void initSendBuffer(ESACPAdminSubType sub_type, EProtocolHeaderExtentionType ext_type, TU16 payload_size, ESACPTypeClientCategory cat_type);
    EECode updateDynamicCode(ESACPAdminSubType sub_type, TChar *user_name, TChar *device_name, TU32 dynamic_code);

private:
    TU8 mbStartConnecting;
    TU8 mbConnectMode;
    TU8 mbConnected;
    TU8 mbLoopConnecting;

    IMutex *mpMutex;
    TInt mSocket;

    TCommunicationClientReadCallBack mfReadCallBack;
    void *mpReadCallBackContext;
    TU32 mnRetryInterval;

private:
    TChar *mpServerUrl;
    TU32 mnServerUrlBufferLength;

    TU16 mServerPort;
    TU16 mNativePort;

    TChar mName[DMAX_ACCOUNT_NAME_LENGTH_EX];
    TChar mPassword[DMAX_ACCOUNT_NAME_LENGTH_EX];

private:
    SSACPHeader *mpSACPWriteHeader;
    TU8 *mpWriteBuffer;
    TU32 mnWriteBufferSize;

    SSACPHeader *mpSACPReadHeader;
    TU8 *mpReadBuffer;
    TU32 mnReadBufferSize;

    TU8 mIdentificationString[DDynamicInputStringLength + DIdentificationStringLength];

private:
    IThread *mpThread;

private:
    IProtocolHeaderParser *mpProtocolParser;
    TU8 mnHeaderLength;
    TU8 reserved0;
    TU8 reserved1;
    TU8 reserved2;
};

//-----------------------------------------------------------------------
//
//  CICommunicationServerPort
//
//-----------------------------------------------------------------------
typedef EECode(*TCommunicationServerReadCallBack)(void *owner, TChar *source, void *p_source, void *p_port, TU8 *p_data, TU32 datasize, TU32 type, TU32 cat, TU32 subtype);

typedef struct {
    TChar source[DMAX_ACCOUNT_NAME_LENGTH_EX];
    TChar password[DMAX_ACCOUNT_NAME_LENGTH_EX];
    TChar username_password_combo[DMAX_ACCOUNT_NAME_LENGTH_EX * 2 + 4];
    TChar id_string_combo[DDynamicInputStringLength + DIdentificationStringLength + 4];
    void *p;
    TInt socket;
} SCommunicationPortSource;

class CICommunicationServerPort : public CObject, public IActiveObject
{
    typedef CObject inherited;

public:
    static CICommunicationServerPort *Create(void *p_context, TCommunicationServerReadCallBack callback, TU16 port);
    void Destroy();
    virtual CObject *GetObject() const;

protected:
    CICommunicationServerPort(void *p_context, TCommunicationServerReadCallBack callback, TU16 port);
    EECode Construct();
    ~CICommunicationServerPort();

public:
    EECode AddSource(TChar *source, TChar *password, void *p_source, TChar *p_identification_string, void *p_context, SCommunicationPortSource *&p_port);
    EECode RemoveSource(SCommunicationPortSource *p);

    EECode Start(TU16 port);
    EECode Stop();

    EECode Reply(SCommunicationPortSource *p_port, TU8 *p_data, TU32 size);

public:
    void OnRun();

private:
    void processCmd(SCMD &cmd);
    void replyToClient(TInt socket, EECode error_code, SSACPHeader *header, TU32 reply_size);
    EECode authentication(TInt socket, SCommunicationPortSource *&p_authen_port);

private:
    CIDoubleLinkedList *mpSourceList;

protected:
    const volatile SPersistCommonConfig *mpPersistCommonConfig;
    IMsgSink *mpMsgSink;
    const CIClockReference *mpSystemClockReference;
    CIWorkQueue *mpWorkQueue;

private:
    TSocketHandler mListenSocket;
    TSocketPort mPort;
    TU8 mbRun;
    TU8 mReserved0;

    EServerManagerState msState;

    TSocketHandler mPipeFd[2];
    TSocketHandler mMaxFd;

    TCommunicationServerReadCallBack mfReadCallBack;
    void *mpReadCallBackContext;

private:
    SSACPHeader *mpSACPWriteHeader;
    TU8 *mpWriteBuffer;
    TU32 mnWriteBufferSize;

    SSACPHeader *mpSACPReadHeader;
    TU8 *mpReadBuffer;
    TU32 mnReadBufferSize;

private:
    IProtocolHeaderParser *mpProtocolParser;
    TU32 mnHeaderLength;
};

void gfSACPFillHeader(SSACPHeader *p_header, TU32 type, TU32 size, TU8 ext_type, TU16 ext_size, TU32 seq_count, TU8 flag);

const TChar *gfGetSACPClientCmdSubtypeString(ESACPClientCmdSubType sub_type);
const TChar *gfGetSACPAdminCmdSubtypeString(ESACPAdminSubType sub_type);

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif

