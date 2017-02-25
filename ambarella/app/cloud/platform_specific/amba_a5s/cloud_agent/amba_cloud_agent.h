/*******************************************************************************
 * amba_cloud_agent.h
 *
 * History:
 *    2014/09/11 - [Zhi He] create file
 *
 * Copyright (c) 2015 Ambarella, Inc.
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

#ifndef __AMBA_CLOUD_AGENT_H__
#define __AMBA_CLOUD_AGENT_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN

DCODE_DELIMITER;

#define DDeviceSyncPointFlag 0x01
#define DDeviceMaxMsgs 256
#define DRetryIntervalUS 2000000

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
    static CAmbaCloudAgent *Create(const TChar *user_info_file_path, const TChar *new_owner_file_path);
    virtual void Destroy();

protected:
    EECode Construct();
    CAmbaCloudAgent(const TChar *account_filename, const TChar *owner_filename);
    virtual ~CAmbaCloudAgent();

public:
    EECode Start(TU8 stream_index = 1, TU8 audio_disable = 0, TU8 m = 1);
    EECode Stop();

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
    EECode uploadAACstream(TU8 *pdata, TMemSize data_size, TU32 seq_num);

private:
    EECode readConfigureFile();
    EECode writeConfigureFile();
    EECode openAdminPort();
    void closeAdminPort();

private:
    const TChar *mpAccountConfFilename;
    const TChar *mpNewOwnerFilename;

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

    TU8 mUploadingStreamIndex;
    TU8 mReserved0;
    TU8 mReserved1;
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

    AmTransferClient *mpTransferClient;
    CICommunicationServerPort *mpAdminServerPort;
    IIMClientAgent *mpIMClientAgent;
    ICloudClientAgentV2 *mpCloudClientAgentV2;

    TU64 mVideoSeqNum;
    TU64 mAudioSeqNum;

    //ring memory pool
    IMemPool *mpRingMemPool;
    IMemPool *mpAudioRingMemPool;

    //sync
    TU64 mBeginTimeStamp;
    TU64 mLastVideoPTS;
};

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif
