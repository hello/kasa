/**
 * amba_cloud_agent.h
 *
 * History:
 *    2015/03/03 - [Zhi He] create file
 *
 * Copyright (C) 2014 - 2024, the Ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the Ambarella Inc.
 */

#ifndef __AMBA_CLOUD_AGENT_H__
#define __AMBA_CLOUD_AGENT_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN

DCODE_DELIMITER;

#define DDeviceSyncPointFlag 0x01
#define DDeviceMaxMsgs 256
#define DRetryIntervalUS 2000000

#define D_ENABLE_ENCRYPTION

typedef enum {
    EAgentState_Bootup = 0x00,
    EAgentState_WaitSetup = 0x01,
    EAgentState_Connected = 0x02,
    EAgentState_Error = 0x03,
} EAgentState;

typedef struct {
    TInt num_readers;
    TInt num_writers;

    pthread_mutex_t mutex;
    pthread_cond_t  cond_read;
    pthread_cond_t  cond_write;
} SAgentMutexCond;

typedef struct {
    TInt    cmd;
    void    *ptr;
    TU32 flags;
} SAgentMsg;

typedef struct _SAgentMsgQueue_s {
    SAgentMsg   msgs[DDeviceMaxMsgs];
    TInt read_index;
    TInt write_index;
    volatile TInt num_msgs;

    SAgentMutexCond *p_mutex_cond;

    TU8 is_master;
    TU8 reserved1;
    TU8 reserved2;
    TU8 reserved3;

    struct _SAgentMsgQueue_s *child_list;
    struct _SAgentMsgQueue_s *p_next;
} SAgentMsgQueue;

class CAmbaCloudAgent
{
public:
    static CAmbaCloudAgent *Create(const TChar *account_filename);
    virtual void Destroy();

protected:
    EECode Construct();
    CAmbaCloudAgent(const TChar *account_filename);
    virtual ~CAmbaCloudAgent();

public:
    EECode Start(TU8 stream_index = 1, TU8 audio_disable = 0, TU8 m = 1, TU8 print_pts_seq = 0, TU8 fps = 0);
    EECode Stop();
    EECode ControlDataEncryption(TU8 enable);
    EECode UpdateDataEncryptionSetting(TChar *key, TChar *iv, TU32 cypher_type, TU32 cypher_mode);

public:
    static void ReceiveMsgCallback(void *owner, TUniqueID id, TU8 *p_message, TU32 message_length, TU32 need_reply);
    static void SystemNotifyCallBack(void *owner, ESystemNotify notify, void *notify_context);
    static void ErrorCallback(void *owner, EECode err);
    static EECode AdminCallback(void *owner, TChar *source, void *p_source, void *p_port, TU8 *p_data, TU32 datasize, TU32 type, TU32 cat, TU32 subtype);

private:
    static void *MainThreadEntry(void *param);
    void MainThread(void *param);
    static void *readDataThreadEntry(void *param);
    void readDataThread(void *param);
    static void *uploadThreadEntry(void *param);
    void uploadThread(void *param);

    void processReceiveMsgCallback(TUniqueID id, TU8 *p_message, TU32 message_length, TU32 need_reply);
    void processSystemNotifyCallBack(ESystemNotify notify, void *notify_context);
    void processErrorCallback(EECode err);
    EECode processAdminCallback(TChar *source, void *p_source, void *p_port, TU8 *p_data, TU32 datasize, TU32 type, TU32 cat, TU32 subtype);

private:
    EECode createCloudAgent();
    EECode cloudAgentConnectServer();
    void cloudAgentDisconnect();
    EECode uploadH264stream(TU8 *pdata, TMemSize data_size, TU32 seq_num, TU32 keyframe);

#ifdef D_ENABLE_ENCRYPTION
    EECode encryptData(TU8 *pdata, TMemSize data_size);
    void resetCypher();
    EECode uploadEncryptedH264stream(TU8 *pdata, TMemSize data_size, TU32 seq_num, TU32 keyframe);
#endif

    EECode uploadAACstream(TU8 *pdata, TMemSize data_size, TU32 seq_num);

private:
    EECode readConfigureFile();
    EECode writeConfigureFile();
    EECode openAdminPort();
    void closeAdminPort();

private:
    AMExportPacket *allocateDataPacket();
    void releaseDataPacket(void *p);
    EECode allocateMemoryInPacket(AMExportPacket *packet, TU32 memsize);
    void checkAudioQueue(SAgentMsgQueue *q);
    TInt checkVideoQueue(SAgentMsgQueue *q);


private:
    const TChar *mpAccountConfFilename;

    TUniqueID mDeviceUniqueID;
    TChar mDeviceAccountName[DMAX_ACCOUNT_NAME_LENGTH_EX];
    TChar mDeviceAccountPassword[DIdentificationStringLength];
    TChar mDeviceDynamicPassword[20];

    ESACPDataChannelSubType mVideoFormat;
    ESACPDataChannelSubType mAudioFormat;

    TU8 mbReadDataThreadRun, mbMainThreadRun, mbUploadThreadRun;
    TU8 mbApplicationStarted;

    TU8 mbEnableVideo;
    TU8 mbEnableAudio;
    TU8 mbVideoUploadingStarted;
    TU8 mbAudioUploadingStarted;

    TU8 mVideoStreamFormat;
    TU8 mAudioStreamFormat;
    TU8 mVideoStreamIndex;
    TU8 mAudioStreamIndex;

    TU8 mDebugPrintPTSSeqNum;
    TU8 mbEnableVideoEncryption;
    TU8 mbStartedVideoEncryption;
    TU8 mReserved2;

    TU32 mAudioChannelNumber;
    TU32 mAudioSampleFrequency;
    TU32 mAudioBitrate;
    TU8 *mpAudioExtraData;
    TMemSize mAudioExtraDataSize;

    TU32 mVideoFramerateDen;
    TU32 mVideoFramerate;
    TU32 mVideoResWidth;
    TU32 mVideoResHeight;
    TU32 mVideoBitrate;
    TU8 *mpVideoExtraData;
    TMemSize mVideoExtraDataSize;

    TU16 mVideoGOPN;
    TU8 mVideoGOPM;
    TU8 mVideoGOPIDRInterval;

    pthread_t mReadDataThread;
    pthread_t mMainThread;
    pthread_t mUploadThread;

    EAgentState msState;

    SAgentMsgQueue *mpDeviceCommandQueue;
    SAgentMsgQueue *mpDeviceVideoDataQueue;
    SAgentMsgQueue *mpDeviceAudioDataQueue;

    TChar mIMServerUrl[DMaxServerUrlLength];
    TChar *mpCloudServerUrl;
    TSocketPort mIMServerPort;
    TSocketPort mCloudServerPort;

    CICommunicationServerPort *mpAdminServerPort;
    IIMClientAgent *mpIMClientAgent;
    ICloudClientAgentV2 *mpCloudClientAgentV2;

    TU64 mVideoSeqNum;
    TU64 mAudioSeqNum;

    //sync
    TU64 mBeginTimeStamp;
    TU64 mLastVideoPTS;
    TU64 mLastAudioPTS;

    pthread_mutex_t mPacketMutex;
    TU32 mTotalPacketCount;
    CIDoubleLinkedList *mpFreePacketList;

    AMIExportClient *mpDataExportClient;

private:
#ifdef D_ENABLE_ENCRYPTION
    s_symmetric_cypher *mpDataCypher;
    TChar mDataCypherKey[DMAX_SYMMETRIC_KEY_LENGTH_BYTES];
    TChar mDataCypherIV[DMAX_SYMMETRIC_KEY_LENGTH_BYTES];

    TInt mCypherBufferSize;
    TInt mDataBufferSize;
    TU8 *mpCypherInputBuffer;
    TU8 *mpCypherOutputBuffer;
#endif
};

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif
