/*******************************************************************************
 * amba_cloud_agent.cpp
 *
 * History:
 *  2014/09/11 - [Zhi He] create file
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

#include "am_include.h"
#include "am_utility.h"
#include "transfer/am_data_transfer.h"
#include "transfer/am_transfer_client.h"

#include <sys/time.h>
#include <signal.h>

#include <unistd.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>

#include <pthread.h>

#include "common_config.h"
#include "common_types.h"
#include "common_osal.h"
#include "common_utils.h"
#include "common_log.h"

#include "common_base.h"

#include "cloud_lib_if.h"
#include "sacp_types.h"
#include "user_protocol_default_if.h"
#include "security_utils_if.h"

#include "amba_cloud_agent.h"

//#define D_PRINT_AVSYNC

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;


typedef enum {
    EAgentCmd_Quit = 0x00,
    EAgentCmd_SetupDone = 0x01,
} EAgentCmd;

static TU32 __adts_length(TU8 *p)
{
    return (((TU32)(p[3] & 0x3)) << 11) | (((TU32)p[4]) << 3) | (((TU32)p[5] >> 5) & 0x7);
}

static TInt __msg_queue_init(SAgentMsgQueue *q)
{
    q->read_index = 0;
    q->write_index = 0;
    q->num_msgs = 0;
    q->child_list = NULL;
    q->p_next = NULL;

    q->is_master = 1;

    q->p_mutex_cond = (SAgentMutexCond *) DDBG_MALLOC(sizeof(SAgentMutexCond), "CMTX");
    if (DUnlikely(!q->p_mutex_cond)) {
        LOG_FATAL("no memory\n");
        return (-1);
    }

    memset(q->p_mutex_cond, 0x0, sizeof(SAgentMutexCond));

    pthread_mutex_init(&q->p_mutex_cond->mutex, NULL);
    pthread_cond_init(&q->p_mutex_cond->cond_read, NULL);
    pthread_cond_init(&q->p_mutex_cond->cond_write, NULL);

    q->p_mutex_cond->num_readers = 0;
    q->p_mutex_cond->num_writers = 0;

    return 0;
}

static void __msg_queue_deinit(SAgentMsgQueue *q)
{
    if (q->is_master) {
        pthread_mutex_destroy(&q->p_mutex_cond->mutex);
        pthread_cond_destroy(&q->p_mutex_cond->cond_read);
        pthread_cond_destroy(&q->p_mutex_cond->cond_write);

        DDBG_FREE(q->p_mutex_cond, "CMTX");
        q->p_mutex_cond = NULL;
    }
}

static TInt __msg_slave_queue_init(SAgentMsgQueue *q, SAgentMsgQueue *master_q)
{
    if (DUnlikely((!q) || (!master_q))) {
        LOG_ERROR("NULL param\n");
        return (-1);
    }

    if (DUnlikely(!master_q->is_master)) {
        LOG_ERROR("not master queue\n");
        return (-2);
    }

    q->read_index = 0;
    q->write_index = 0;
    q->num_msgs = 0;

    q->is_master = 0;

    DASSERT(master_q->p_mutex_cond);

    q->p_mutex_cond = master_q->p_mutex_cond;

    q->p_next = master_q->child_list;
    master_q->child_list = q;

    return 0;
}

static void __msg_queue_put(SAgentMsgQueue *q, SAgentMsg *msg)
{
    pthread_mutex_lock(&q->p_mutex_cond->mutex);
    while (1) {
        if (q->num_msgs < DDeviceMaxMsgs) {
            q->msgs[q->write_index] = *msg;

            if (++q->write_index == DDeviceMaxMsgs)
            { q->write_index = 0; }

            q->num_msgs++;

            if (q->p_mutex_cond->num_readers > 0) {
                q->p_mutex_cond->num_readers--;
                pthread_cond_signal(&q->p_mutex_cond->cond_read);
            }

            pthread_mutex_unlock(&q->p_mutex_cond->mutex);
            return;
        }

        q->p_mutex_cond->num_writers++;
        pthread_cond_wait(&q->p_mutex_cond->cond_write, &q->p_mutex_cond->mutex);
    }
}

static TInt __msg_queue_peek(SAgentMsgQueue *q, SAgentMsg *msg)
{
    TInt ret = 0;
    pthread_mutex_lock(&q->p_mutex_cond->mutex);

    if (q->num_msgs > 0) {
        *msg = q->msgs[q->read_index];

        if (++q->read_index == DDeviceMaxMsgs)
        { q->read_index = 0; }

        q->num_msgs--;

        if (q->p_mutex_cond->num_writers > 0) {
            q->p_mutex_cond->num_writers--;
            pthread_cond_signal(&q->p_mutex_cond->cond_write);
        }

        ret = 1;
    }

    pthread_mutex_unlock(&q->p_mutex_cond->mutex);
    return ret;
}

static void __msg_queue_get(SAgentMsgQueue *q, SAgentMsg *msg)
{
    pthread_mutex_lock(&q->p_mutex_cond->mutex);
    while (1) {
        if (q->num_msgs > 0) {
            *msg = q->msgs[q->read_index];

            if (++q->read_index == DDeviceMaxMsgs)
            { q->read_index = 0; }

            q->num_msgs--;

            if (q->p_mutex_cond->num_writers > 0) {
                q->p_mutex_cond->num_writers--;
                pthread_cond_signal(&q->p_mutex_cond->cond_write);
            }

            pthread_mutex_unlock(&q->p_mutex_cond->mutex);
            return;
        }

        q->p_mutex_cond->num_readers++;
        pthread_cond_wait(&q->p_mutex_cond->cond_read, &q->p_mutex_cond->mutex);
    }
}

static SAgentMsgQueue *__msg_queue_get_all(SAgentMsgQueue *master_q, SAgentMsg *msg)
{
    SAgentMsgQueue *cur_q = NULL;

    pthread_mutex_lock(&master_q->p_mutex_cond->mutex);
    if (master_q->num_msgs) {
        *msg = master_q->msgs[master_q->read_index];

        if (++master_q->read_index == DDeviceMaxMsgs)
        { master_q->read_index = 0; }

        master_q->num_msgs--;

        if (master_q->p_mutex_cond->num_writers > 0) {
            master_q->p_mutex_cond->num_writers--;
            pthread_cond_signal(&master_q->p_mutex_cond->cond_write);
        }

        pthread_mutex_unlock(&master_q->p_mutex_cond->mutex);
        return master_q;
    }

    while (1) {

        cur_q = master_q->child_list;

        while (cur_q) {

            if (cur_q->num_msgs > 0) {
                *msg = cur_q->msgs[cur_q->read_index];

                if (++cur_q->read_index == DDeviceMaxMsgs)
                { cur_q->read_index = 0; }

                cur_q->num_msgs--;

                if (master_q->p_mutex_cond->num_writers > 0) {
                    master_q->p_mutex_cond->num_writers--;
                    pthread_cond_signal(&master_q->p_mutex_cond->cond_write);
                }

                pthread_mutex_unlock(&master_q->p_mutex_cond->mutex);
                return cur_q;
            }

            cur_q = cur_q->p_next;
        }

        master_q->p_mutex_cond->num_readers++;
        pthread_cond_wait(&master_q->p_mutex_cond->cond_read, &master_q->p_mutex_cond->mutex);
    }

    //should not comes here
    return NULL;
}

static TU32 __skip_delimter_size(TU8 *p)
{
    if ((0 == p[0]) && (0 == p[1]) && (0 == p[2]) && (1 == p[3]) && (0x09 == p[4])) {
        return 6;
    }

    return 0;
}

static void __release_data_packet(void *p, IMemPool *pool)
{
    AmTransferPacketHeader *header = (AmTransferPacketHeader *) p;
    pool->ReleaseMemBlock((TMemSize)(sizeof(AmTransferPacketHeader) + header->dataLen), (TU8 *)p);
}

static TInt __video_queue_check(SAgentMsgQueue *q, IMemPool *p_mem_pool)
{
    TInt skip_number = 0;

    pthread_mutex_lock(&q->p_mutex_cond->mutex);
    if (DUnlikely(q->num_msgs > (DDeviceMaxMsgs - 2))) {
        printf("!!!!start to flush, %d\n", q->num_msgs);
        while (1) {
            if (q->num_msgs > 0) {

                if ((skip_number > (DDeviceMaxMsgs / 2)) && (q->msgs[q->read_index].flags & DDeviceSyncPointFlag)) {
                    pthread_mutex_unlock(&q->p_mutex_cond->mutex);
                    return 1;
                }

                __release_data_packet(q->msgs[q->read_index].ptr, p_mem_pool);

                if (++q->read_index == DDeviceMaxMsgs) {
                    q->read_index = 0;
                }

                q->num_msgs--;

                skip_number++;
            } else {
                printf("!!!!end flush, skip %d, remain %d\n", skip_number, q->num_msgs);
                pthread_mutex_unlock(&q->p_mutex_cond->mutex);
                return 0;
            }
        }
    } else {
        pthread_mutex_unlock(&q->p_mutex_cond->mutex);
        return -1;
    }

    //should not comes here
    return -3;
}

static void __audio_queue_check(SAgentMsgQueue *q, IMemPool *p_mem_pool)
{
    pthread_mutex_lock(&q->p_mutex_cond->mutex);
    if (DUnlikely(q->num_msgs > (DDeviceMaxMsgs - 6))) {
        printf("discard audio packet, %d\n", q->num_msgs);
        __release_data_packet(q->msgs[q->read_index].ptr, p_mem_pool);

        if (++q->read_index == DDeviceMaxMsgs)
        { q->read_index = 0; }

        q->num_msgs--;
    }
    pthread_mutex_unlock(&q->p_mutex_cond->mutex);
}

static TU32 __clientAgentAuthenticateCallBack(TU8 *p_input, TU32 input_length)
{
    DASSERT((DDynamicInputStringLength) == input_length);

    return gfHashAlgorithmV3(p_input, input_length);
}

CAmbaCloudAgent *CAmbaCloudAgent::Create(const TChar *user_info_file_path, const TChar *new_owner_file_path)
{
    CAmbaCloudAgent *result = new CAmbaCloudAgent(user_info_file_path, new_owner_file_path);
    if ((result) && (EECode_OK != result->Construct())) {
        result->Destroy();
        result = NULL;
    }
    return result;
}

void CAmbaCloudAgent::Destroy()
{
    delete this;
}

EECode CAmbaCloudAgent::Construct()
{
    mpDeviceCommandQueue = (SAgentMsgQueue *)DDBG_MALLOC(sizeof(SAgentMsgQueue), "CDCQ");
    if (DUnlikely(!mpDeviceCommandQueue)) {
        LOG_FATAL("DDBG_MALLOC mpDeviceCommandQueue failed\n");
        return EECode_NoMemory;
    }
    if (DUnlikely(0 != __msg_queue_init(mpDeviceCommandQueue))) {
        LOG_FATAL("init mpDeviceCommandQueue failed\n");
        return EECode_NoMemory;
    }

    mpDeviceVideoDataQueue = (SAgentMsgQueue *)DDBG_MALLOC(sizeof(SAgentMsgQueue), "CVDQ");
    if (DUnlikely(!mpDeviceVideoDataQueue)) {
        LOG_FATAL("DDBG_MALLOC mpDeviceVideoDataQueue failed\n");
        return EECode_NoMemory;
    }
    if (DUnlikely(0 != __msg_queue_init(mpDeviceVideoDataQueue))) {
        LOG_FATAL("init mpDeviceVideoDataQueue failed\n");
        return EECode_NoMemory;
    }

    mpDeviceAudioDataQueue = (SAgentMsgQueue *)DDBG_MALLOC(sizeof(SAgentMsgQueue), "CADQ");
    if (DUnlikely(!mpDeviceAudioDataQueue)) {
        LOG_FATAL("DDBG_MALLOC mpDeviceAudioDataQueue failed\n");
        return EECode_NoMemory;
    }
    if (DUnlikely(0 != __msg_slave_queue_init(mpDeviceAudioDataQueue, mpDeviceVideoDataQueue))) {
        LOG_FATAL("init mpDeviceAudioDataQueue failed\n");
        return EECode_NoMemory;
    }

    mpTransferClient  = AmTransferClient::Create();
    if (DUnlikely(!mpTransferClient)) {
        LOG_ERROR("AmTransferClient::Create() fail, make sure SDK is proper initialized\n");
        return EECode_Error;
    }

    mpIMClientAgent = gfCreateIMClientAgent(EProtocolType_SACP);
    if (DUnlikely(!mpIMClientAgent)) {
        LOG_ERROR("gfCreateIMClientAgent() fail\n");
        return EECode_InternalLogicalBug;
    }

    EECode err = createCloudAgent();
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("createCloudAgent fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    }

    mpRingMemPool = CRingMemPool::Create((TMemSize)((TMemSize)1024 * (TMemSize)1024 * (TMemSize)8));
    if (DUnlikely(!mpRingMemPool)) {
        LOG_ERROR("CRingMemPool::Create() fail, no memory?\n");
        return EECode_NoMemory;
    }

    mpAudioRingMemPool = CRingMemPool::Create((TMemSize)((TMemSize)1024 * (TMemSize)256));
    if (DUnlikely(!mpAudioRingMemPool)) {
        LOG_ERROR("CRingMemPool::Create() fail, no memory?\n");
        return EECode_NoMemory;
    }

    return EECode_OK;
}

CAmbaCloudAgent::CAmbaCloudAgent(const TChar *account_filename, const TChar *owner_filename)
    : mpAccountConfFilename(account_filename)
    , mpNewOwnerFilename(owner_filename)
    , mDeviceUniqueID(0)
    , mVideoFormat(ESACPDataChannelSubType_H264_NALU)
    , mAudioFormat(ESACPDataChannelSubType_AAC)
    , mbReadDataThreadRun(0)
    , mbMainThreadRun(0)
    , mbUploadThreadRun(0)
    , mbApplicationStarted(0)
    , mbEnableVideo(1)
    , mbEnableAudio(1)
    , mbVideoUploadingStarted(0)
    , mbAudioUploadingStarted(0)
    , mUploadingStreamIndex(1)
    , mAudioChannelNumber(1)
    , mAudioSampleFrequency(DDefaultAudioSampleRate)
    , mAudioBitrate(48000)
    , mpAudioExtraData(NULL)
    , mAudioExtraDataSize(0)
    , mVideoFramerateDen(DDefaultVideoFramerateDen)
    , mVideoFramerate(30)
    , mVideoResWidth(640)
    , mVideoResHeight(480)
    , mVideoBitrate(128000)
    , mpVideoExtraData(NULL)
    , mVideoExtraDataSize(0)
    , mVideoGOPN(120)
    , mVideoGOPM(1)
    , mVideoGOPIDRInterval(1)
    , msState(EAgentState_Bootup)
    , mpDeviceCommandQueue(NULL)
    , mpDeviceVideoDataQueue(NULL)
    , mpDeviceAudioDataQueue(NULL)
    , mpCloudServerUrl(NULL)
    , mIMServerPort(DDefaultSACPServerPort)
    , mCloudServerPort(DDefaultSACPServerPort)
    , mpTransferClient(NULL)
    , mpAdminServerPort(NULL)
    , mpIMClientAgent(NULL)
    , mpCloudClientAgentV2(NULL)
    , mVideoSeqNum(0)
    , mAudioSeqNum(0)
    , mpRingMemPool(NULL)
    , mpAudioRingMemPool(NULL)
    , mBeginTimeStamp(0)
{
    memset(mDeviceAccountName, 0x0, sizeof(mDeviceAccountName));
    memset(mDeviceAccountPassword, 0x0, sizeof(mDeviceAccountPassword));
    memset(mDeviceDynamicPassword, 0x0, sizeof(mDeviceDynamicPassword));

    memset(mIMServerUrl, 0x0, sizeof(mIMServerUrl));
}

CAmbaCloudAgent::~CAmbaCloudAgent()
{
    if (mpDeviceCommandQueue) {
        __msg_queue_deinit(mpDeviceCommandQueue);
        DDBG_FREE(mpDeviceCommandQueue, "CDCQ");
        mpDeviceCommandQueue = NULL;
    }
    if (mpDeviceVideoDataQueue) {
        __msg_queue_deinit(mpDeviceVideoDataQueue, "CVDQ");
        DDBG_FREE(mpDeviceVideoDataQueue);
        mpDeviceVideoDataQueue = NULL;
    }

    if (mpDeviceAudioDataQueue) {
        __msg_queue_deinit(mpDeviceAudioDataQueue, "CVAQ");
        DDBG_FREE(mpDeviceAudioDataQueue);
        mpDeviceAudioDataQueue = NULL;
    }

    if (mpTransferClient) {
        mpTransferClient->Close();
        delete mpTransferClient;
        mpTransferClient = NULL;
    }
    if (mpAdminServerPort) {
        mpAdminServerPort->Stop();
        mpAdminServerPort->Destroy();
        mpAdminServerPort = NULL;
    }

    if (mpIMClientAgent) {
        mpIMClientAgent->Destroy();
        mpIMClientAgent = NULL;
    }

    if (mpCloudClientAgentV2) {
        mpCloudClientAgentV2->Delete();
        mpCloudClientAgentV2 = NULL;
    }

    if (mpVideoExtraData) {
        DDBG_FREE(mpVideoExtraData, "CVED");
        mpVideoExtraData = NULL;
        mVideoExtraDataSize = 0;
    }
    if (mpAudioExtraData) {
        DDBG_FREE(mpAudioExtraData, "CAED");
        mpAudioExtraData = NULL;
        mAudioExtraDataSize = 0;
    }
}

EECode CAmbaCloudAgent::Start(TU8 stream_index, TU8 audio_disable, TU8 m)
{
    if (DUnlikely(mbMainThreadRun)) {
        LOG_WARN("CAmbaCloudAgent already started\n");
        return EECode_BadState;
    }
    mbMainThreadRun = 1;
    mUploadingStreamIndex = stream_index;
    mbEnableAudio = !audio_disable;

    mVideoGOPM = m;

    if (mbEnableAudio) {
        TU32 generated_config_size = 0;
        TU8 *generated_aac_config = gfGenerateAACExtraData(mAudioSampleFrequency, mAudioChannelNumber, generated_config_size);

        if (DLikely(generated_aac_config && (16 > generated_config_size))) {
            mpAudioExtraData = generated_aac_config;
            mAudioExtraDataSize = (TMemSize)generated_config_size;
            LOG_WARN("generate mAudioExtraDataSize=%lu, mpAudioExtraData first 2 bytes: 0x%x 0x%x\n", mAudioExtraDataSize, mpAudioExtraData[0], mpAudioExtraData[1]);
        } else {
            LOG_ERROR("gfGenerateAudioExtraData() fail, format=%hu, samplerate=%u, channel_number%hu\n", mAudioFormat, mAudioSampleFrequency, mAudioChannelNumber);
            mbMainThreadRun = 0;
            return EECode_BadParam;
        }
    }

    pthread_create(&mMainThread, NULL, MainThreadEntry, (void *)(this));

    return EECode_OK;
}

EECode CAmbaCloudAgent::Stop()
{
    if (DUnlikely(!mbMainThreadRun)) {
        LOG_WARN("CAmbaCloudAgent already stopped\n");
        return EECode_BadState;
    }
    mbMainThreadRun = 0;

    if (mbUploadThreadRun) {
        mbUploadThreadRun = 0;
        pthread_join(mUploadThread, NULL);
    }
    if (mbReadDataThreadRun) {
        mbReadDataThreadRun = 0;
        pthread_join(mReadDataThread, NULL);
    }

    SAgentMsg msg = {0};
    msg.cmd = EAgentCmd_Quit;
    __msg_queue_put(mpDeviceCommandQueue, &msg);
    pthread_join(mMainThread, NULL);

    if (mbEnableAudio && mpAudioExtraData) {
        DDBG_FREE(mpAudioExtraData, "CAED");
        mpAudioExtraData = NULL;
        mAudioExtraDataSize = 0;
    }

    return EECode_OK;
}

void CAmbaCloudAgent::ReceiveMsgCallback(void *owner, TUniqueID id, TU8 *p_message, TU32 message_length, TU32 need_reply)
{
    CAmbaCloudAgent *thiz = (CAmbaCloudAgent *)owner;
    if (DUnlikely(!thiz)) {
        LOG_ERROR("NULL pointer\n");
        return;
    }
    thiz->processReceiveMsgCallback(id, p_message, message_length, need_reply);
}

void CAmbaCloudAgent::SystemNotifyCallBack(void *owner, ESystemNotify notify, void *notify_context)
{
    CAmbaCloudAgent *thiz = (CAmbaCloudAgent *)owner;
    if (!thiz) {
        LOG_ERROR("NULL pointer\n");
        return;
    }
    thiz->processSystemNotifyCallBack(notify, notify_context);
}

void CAmbaCloudAgent::ErrorCallback(void *owner, EECode err)
{
    CAmbaCloudAgent *thiz = (CAmbaCloudAgent *)owner;
    if (!thiz) {
        LOG_ERROR("NULL pointer\n");
        return;
    }
    thiz->processErrorCallback(err);
}

EECode CAmbaCloudAgent::AdminCallback(void *owner, TChar *source, void *p_source, void *p_port, TU8 *p_data, TU32 datasize, TU32 type, TU32 cat, TU32 subtype)
{
    CAmbaCloudAgent *thiz = (CAmbaCloudAgent *)owner;
    if (DUnlikely(!thiz)) {
        LOG_ERROR("NULL pointer\n");
        return EECode_BadCommand;
    }
    return thiz->processAdminCallback(source, p_source, p_port, p_data, datasize, type, cat, subtype);
}

void *CAmbaCloudAgent::MainThreadEntry(void *param)
{
    CAmbaCloudAgent *thiz = (CAmbaCloudAgent *)param;
    thiz->MainThread(NULL);

    LOG_NOTICE("after MainThread\n");
    return NULL;
}

void CAmbaCloudAgent::MainThread(void *param)
{
    TU32 running = 1;
    SAgentMsg msg = {0};

    EECode ret = EECode_OK;
    DASSERT(mpIMClientAgent);

    ret = readConfigureFile();
    if (DLikely(EECode_OK == ret)) {
        msState = EAgentState_Bootup;
    } else {
        LOG_WARN("readConfigureFile fail, ret %d, %s\n", ret, gfGetErrorCodeString(ret));
        openAdminPort();
        msState = EAgentState_WaitSetup;
    }

    while (running) {

        LOG_STATE("CAmbaCloudAgent::MainThread, msState=%d\n", msState);

        switch (msState) {

            case EAgentState_Bootup: {

                    TU64 dynamic_code_input = 0;
                    TU32 dynamic_code = 0;
                    TU8 hash_input[DDynamicInputStringLength + DIdentificationStringLength];

                    mpIMClientAgent->SetCallBack((void *)this, ReceiveMsgCallback, SystemNotifyCallBack, ErrorCallback);
                    ret = mpIMClientAgent->Login(mDeviceUniqueID, mDeviceAccountPassword, mIMServerUrl, mIMServerPort, mpCloudServerUrl, mCloudServerPort, dynamic_code_input);

                    if (DLikely((EECode_OK == ret) || (EECode_OK_NeedSetOwner == ret))) {

                        memset(hash_input, 0x0, sizeof(hash_input));
                        DBEW64(dynamic_code_input, hash_input);
                        strncpy((TChar *)(hash_input + DDynamicInputStringLength), mDeviceAccountPassword, DIdentificationStringLength);
                        dynamic_code = gfHashAlgorithmV6(hash_input, DDynamicInputStringLength + DIdentificationStringLength);
                        sprintf(mDeviceDynamicPassword, "%08x", dynamic_code);

                        if (DLikely(!mbUploadThreadRun)) {
                            mbUploadThreadRun = 1;
                            pthread_create(&mUploadThread, NULL, uploadThreadEntry, (void *)(this));
                        } else {
                            LOG_ERROR("uploading thread already run?\n");
                        }
                        if (DLikely(!mbReadDataThreadRun)) {
                            mbReadDataThreadRun = 1;
                            pthread_create(&mReadDataThread, NULL, readDataThreadEntry, (void *)(this));
                        } else {
                            LOG_ERROR("reading thread already run?\n");
                        }
                        msState = EAgentState_Connected;
                        break;
                    } else {
                        switch (ret) {
                            case EECode_ServerReject_NoSuchChannel:
                            case EECode_ServerReject_BadRequestFormat:
                            case EECode_ServerReject_CorruptedProtocol:
                            case EECode_ServerReject_AuthenticationDataTooLong:
                            case EECode_ServerReject_NotProperPassword:
                            case EECode_ServerReject_NotSupported:
                            case EECode_ServerReject_AuthenticationFail:
                            case EECode_ServerReject_Unknown:
                                LOG_ERROR("server reject, ret %d, %s\n", ret, gfGetErrorCodeString(ret));
                                msState = EAgentState_Error;
                                openAdminPort();
                                break;

                            case EECode_ServerReject_ChannelIsBusy:
                            case EECode_ServerReject_TimeOut:
                            case EECode_NetSendHeader_Fail:
                            case EECode_NetSendPayload_Fail:
                            case EECode_NetReceiveHeader_Fail:
                            case EECode_NetReceivePayload_Fail:
                                LOG_ERROR("network is poor, ret %d, %s, try later\n", ret, gfGetErrorCodeString(ret));
                                usleep(3000000);
                                break;

                            case EECode_NetConnectFail:
                                LOG_ERROR("network is not ready, ret %d, %s, try later\n", ret, gfGetErrorCodeString(ret));
                                usleep(3000000);
                                break;

                            default:
                                LOG_ERROR("Login() fail ret %d, %s, not expected error, go to error state\n", ret, gfGetErrorCodeString(ret));
                                msState = EAgentState_Error;
                                openAdminPort();
                                break;
                        }
                    }
                }
                break;

            case EAgentState_Error:
            case EAgentState_WaitSetup: {
                    __msg_queue_get(mpDeviceCommandQueue, &msg);
                    if (EAgentCmd_Quit == msg.cmd) {
                        LOG_NOTICE("get EAgentCmd_Quit, MainThread will quit\n");
                        running = 0;
                        break;
                    } else if (EAgentCmd_SetupDone == msg.cmd) {
                        closeAdminPort();
                        msState = EAgentState_Bootup;
                        LOG_NOTICE("switch to boot state, from %d\n", msState);
                        break;
                    } else {
                        LOG_WARN("not processed cmd %d\n", msg.cmd);
                        break;
                    }
                }
                break;

            case EAgentState_Connected: {
                    __msg_queue_get(mpDeviceCommandQueue, &msg);
                    if (EAgentCmd_Quit == msg.cmd) {
                        LOG_NOTICE("get EAgentCmd_Quit, MainThread will quit\n");
                        running = 0;
                        break;
                    } else {
                        LOG_WARN("not processed cmd %d\n", msg.cmd);
                        break;
                    }
                }
                break;

            default:
                running = 0;
                LOG_FATAL("BAD msState=%d, quit.\n", msState);
                break;
        }

    }

    LOG_INFO("MainThread end.\n");
}

void *CAmbaCloudAgent::readDataThreadEntry(void *param)
{
    CAmbaCloudAgent *thiz = (CAmbaCloudAgent *)param;
    thiz->readDataThread(NULL);

    LOG_NOTICE("after readDataThread\n");
    return NULL;
}

void CAmbaCloudAgent::readDataThread(void *param)
{
    TInt ret = 0;
    TInt wait_first_idr = 1;
    TInt start_reading = 0;
    TInt video_start_count = 0;
    TInt audio_start_count = 0;
    TInt size = 0;
    TInt send_size = 0;
    unsigned long long pts = 0;

    SAgentMsg msg;
    AmTransferPacketHeader *p_header = NULL;
    TU8 *p_cur = NULL, *p_audio_cur = NULL, *ptmp = NULL;

    DASSERT(mpRingMemPool);
    DASSERT(mpDeviceVideoDataQueue);
    DASSERT(mpDeviceAudioDataQueue);
    DASSERT(mpTransferClient);

    mVideoSeqNum = 0;
    mAudioSeqNum = 0;

    LOG_NOTICE("readDataThread begin\n");
    while (mbReadDataThreadRun) {
        TInt to_sync = __video_queue_check(mpDeviceVideoDataQueue, mpRingMemPool);
        if (DUnlikely(-1 != to_sync)) {
            LOG_NOTICE("readDataThread, flush = %d\n", to_sync);
            if (to_sync) {
                wait_first_idr = 0;
            } else {
                wait_first_idr = 1;
            }
        }
        __audio_queue_check(mpDeviceAudioDataQueue, mpAudioRingMemPool);
        p_cur = mpRingMemPool->RequestMemBlock((TMemSize)sizeof(AmTransferPacket));
        p_header = (AmTransferPacketHeader *)p_cur;

retryread:
        ret = mpTransferClient->ReceivePacket((AmTransferPacket *)p_cur);
        if (DUnlikely(ret)) {
            LOG_ERROR("mpTransferClient->ReceivePacket fail, ret %d\n", ret);
            goto retryread;
        }
        if (p_header->streamId == mUploadingStreamIndex) {
            if (AM_TRANSFER_PACKET_TYPE_H264 == p_header->dataType) {
                //printf("read video, seq %lld\n", p_header->seqNum);

                if (DUnlikely(wait_first_idr)) {
                    if (p_header->frameType != AM_TRANSFER_FRAME_IDR_VIDEO) {
                        LOG_NOTICE("wait video IDR\n");
                        goto retryread;
                    } else if (p_header->frameType == AM_TRANSFER_FRAME_IDR_VIDEO) {
                        wait_first_idr = 0;
                        msg.flags = DDeviceSyncPointFlag;
                    }
                } else if (p_header->frameType == AM_TRANSFER_FRAME_IDR_VIDEO) {
                    msg.flags = DDeviceSyncPointFlag;
                } else {
                    msg.flags = 0;
                }
                msg.ptr = (void *)p_cur;

                if (DUnlikely(!start_reading)) {
                    start_reading = 1;
                    mBeginTimeStamp = p_header->dataPTS;
                    mVideoSeqNum = 0;
                } else if (DUnlikely(!video_start_count)) {
                    TU64 start_gap = p_header->dataPTS - mBeginTimeStamp;
                    if (DLikely(p_header->dataPTS >= mBeginTimeStamp)) {
                        mVideoSeqNum = start_gap / DDefaultVideoFramerateDen;
                        LOG_NOTICE("video start gap, %lld, seq %lld\n", start_gap, mVideoSeqNum);
                    }
                }

                if (DUnlikely(!video_start_count)) {
                    video_start_count = 1;
                } else {
#if 0
                    TTime frame_gap = p_header->dataPTS - mLastVideoPTS;
                    if (DUnlikely(frame_gap >= (2 * DDefaultVideoFramerateDen - 200))) {
                        mVideoSeqNum += (frame_gap + 100 - DDefaultVideoFramerateDen) / DDefaultVideoFramerateDen;
                        LOG_WARN("video pts jump gap %lld, cur %lld, last %lld\n", frame_gap, p_header->dataPTS, mLastVideoPTS);
                    }
#endif
                }
                mLastVideoPTS = p_header->dataPTS;

                p_header->seqNum = mVideoSeqNum ++;

                send_size = sizeof(AmTransferPacketHeader) + p_header->dataLen;

                //printf("send video %p, len %d, seq %llu\n", p_cur, p_header->dataLen, p_header->seqNum);

                mpRingMemPool->ReturnBackMemBlock((TMemSize)(sizeof(AmTransferPacket) - send_size), p_cur + send_size);
                __msg_queue_put(mpDeviceVideoDataQueue, &msg);
            } else if (mbEnableAudio && (AM_TRANSFER_PACKET_TYPE_AAC == p_header->dataType)) {

                if (DUnlikely(!video_start_count)) {
                    LOG_NOTICE("wait video start\n");
                    goto retryread;
                }

                size = p_header->dataLen;
                ptmp = p_cur + sizeof(AmTransferPacketHeader);
                pts = p_header->dataPTS;

                TU32 frame_length = 0;
                AmTransferPacketHeader *p_audio_header = NULL;
                do {

                    if (DUnlikely(!start_reading)) {
                        start_reading = 1;
                        mBeginTimeStamp = p_header->dataPTS;
                    } else if (DUnlikely(!audio_start_count)) {
                        TU64 start_gap = p_header->dataPTS - mBeginTimeStamp;
                        if (DLikely(p_header->dataPTS >= mBeginTimeStamp)) {
                            mAudioSeqNum = start_gap / 1920;
                            LOG_NOTICE("audio start gap, %lld, seq %lld\n", start_gap, mAudioSeqNum);
                        } else {
                            mAudioSeqNum = 0;
                            LOG_WARN("audio start gap is not expected?, pts %lld, begin time %lld\n", p_header->dataPTS, mBeginTimeStamp);
                        }
                        audio_start_count = 1;
                    }

                    frame_length = __adts_length(ptmp);
                    send_size = frame_length + sizeof(AmTransferPacketHeader);
                    p_audio_cur = mpAudioRingMemPool->RequestMemBlock((TMemSize)send_size);

                    memcpy(p_audio_cur, p_cur, sizeof(AmTransferPacketHeader));

                    p_audio_header = (AmTransferPacketHeader *) p_audio_cur;
                    p_audio_header->seqNum = mAudioSeqNum ++;
                    p_audio_header->dataLen = frame_length;
                    p_audio_header->dataPTS = pts;
                    pts += 1920;
                    //printf("read audio, seq %lld\n", p_audio_header->seqNum);

                    memcpy(p_audio_cur + sizeof(AmTransferPacketHeader), ptmp, frame_length);

                    ptmp += frame_length;
                    size -= frame_length;

                    msg.ptr = (void *)p_audio_cur;

                    __msg_queue_put(mpDeviceAudioDataQueue, &msg);

                } while (size > 0);
                mpRingMemPool->ReturnBackMemBlock((TMemSize)sizeof(AmTransferPacket), p_cur);
            } else {
                //LOG_INFO("skip non h264/aac type %d\n", p_cur_data_packet->header.dataType);
                goto retryread;
            }
        } else {
            //LOG_INFO("skip id %d\n", p_cur_data_packet->header.streamId);
            goto retryread;
        }
    }

    LOG_INFO("readDataThread end.\n");
}

void *CAmbaCloudAgent::uploadThreadEntry(void *param)
{
    CAmbaCloudAgent *thiz = (CAmbaCloudAgent *)param;
    thiz->uploadThread(NULL);

    LOG_NOTICE("after uploadThread\n");
    return NULL;
}

void CAmbaCloudAgent::uploadThread(void *param)
{
    AmTransferPacketHeader *p_header = NULL;

    EECode err = EECode_OK;
    TU8 extra_flag = 0;
    TU8 connected = 0;
    TU8 *pdata = NULL;
    TU32 data_size = 0;
    TU32 skip_size = 0;
    TU8 audio_extradata_send = 0, video_extradata_send = 0, discard_non_idr = 1;
    SAgentMsg msg;
    SAgentMsgQueue *queue = NULL;

    TU32 kr_count = 0;
    TU32 sync_video = 0;

    DASSERT(mpCloudClientAgentV2);
    DASSERT(mpDeviceVideoDataQueue);
    DASSERT(mpDeviceAudioDataQueue);

    LOG_NOTICE("uploadThread begin.\n");
    while (mbUploadThreadRun) {
        if (DUnlikely(!connected)) {
            err = cloudAgentConnectServer();
            if (EECode_OK != err) {
                // handle typical error code
                LOG_ERROR("connect server fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
                cloudAgentDisconnect();
                connected = 0;
                usleep(2000000);
                continue;
            }
            connected = 1;
        }
        queue = __msg_queue_get_all(mpDeviceVideoDataQueue, &msg);
        p_header = (AmTransferPacketHeader *) msg.ptr;
        if (queue == mpDeviceVideoDataQueue) {
            DASSERT(AM_TRANSFER_PACKET_TYPE_H264 == p_header->dataType);
            pdata = ((TU8 *)msg.ptr) + sizeof(AmTransferPacketHeader);
            skip_size = __skip_delimter_size(pdata);
            pdata += skip_size;
            data_size = p_header->dataLen - skip_size;

            //printf("get video %p, len %d, seq %llu\n", msg.ptr, p_header->dataLen, p_header->seqNum);

            extra_flag = 0;
            sync_video = 0;
            if (DUnlikely(p_header->frameType == AM_TRANSFER_FRAME_IDR_VIDEO)) {
                extra_flag |= DSACPHeaderFlagBit_PacketKeyFrameIndicator;

                if (DUnlikely(!video_extradata_send)) {
                    if ((!mpVideoExtraData) || (!mVideoExtraDataSize)) {
                        TU8 *p_extradata = NULL;
                        TU32 extradata_size = 0;
                        err = gfGetH264Extradata(pdata, data_size, p_extradata, extradata_size);
                        if (DUnlikely(EECode_OK != err)) {
                            LOG_FATAL("can not find extra data, corrupted h264 data?\n");
                            gfPrintMemory(pdata, 64);
                            __release_data_packet(msg.ptr, mpRingMemPool);
                            continue;
                        }
                        mVideoExtraDataSize = extradata_size;
                        mpVideoExtraData = (TU8 *) DDBG_MALLOC(mVideoExtraDataSize + 16, "CVED");
                        if (DUnlikely(!mpVideoExtraData)) {
                            LOG_FATAL("no memory, request size %ld\n", (mVideoExtraDataSize + 16));
                            __release_data_packet(msg.ptr, mpRingMemPool);
                            continue;
                        }
                        memcpy(mpVideoExtraData, p_extradata, mVideoExtraDataSize);
                    }

                    LOG_NOTICE("video extra size %d\n", (TU32)mVideoExtraDataSize);
                    gfPrintMemory(mpVideoExtraData, (TU32)mVideoExtraDataSize);

                    err = mpCloudClientAgentV2->SetupVideoParams(mVideoFormat, ((DDefaultVideoFramerateDen) << 8) | (mVideoFramerate & 0xff), mVideoResWidth, mVideoResHeight, mVideoBitrate, mpVideoExtraData, (TU16)mVideoExtraDataSize, DBuildGOP(mVideoGOPM, mVideoGOPN, mVideoGOPIDRInterval));
                    if (DUnlikely(EECode_OK != err)) {
                        LOG_ERROR("CAmbaCloudAgent::uploadThread, setVideoParams() failed, err = %d, %s\n", err, gfGetErrorCodeString(err));
                        cloudAgentDisconnect();
                        connected = 0;
                        audio_extradata_send = 0;
                        video_extradata_send = 0;
                        __release_data_packet(msg.ptr, mpRingMemPool);
                        usleep(DRetryIntervalUS);
                        continue;
                    }
                    video_extradata_send = 1;
                }
                kr_count ++;
                if (DUnlikely(!(kr_count & 0xf))) {
                    sync_video = 1;
                }
            } else if (DUnlikely(discard_non_idr)) {
                LOG_WARN("skip non-idr data\n");
                __release_data_packet(msg.ptr, mpRingMemPool);
                continue;
            }

            if ((0x01 == pdata[2]) && (0x07 == (0x1f & pdata[3]))) {
                extra_flag |= DSACPHeaderFlagBit_PacketExtraDataIndicator;
            }

            TU64 seq_num = p_header->seqNum;
            TU64 cur_pts = p_header->dataPTS;

#ifdef D_PRINT_AVSYNC
            printf("             uploadH264stream(%d, %lld)\n", (TU32)seq_num, cur_pts);
#endif

            if (DUnlikely(!mbVideoUploadingStarted)) {
                mpCloudClientAgentV2->VideoSync((TTime)cur_pts - mBeginTimeStamp, seq_num);
                mbVideoUploadingStarted = 1;
                err = uploadH264stream(pdata, data_size, (TU32)seq_num, extra_flag);
                if (DUnlikely(EECode_OK != err)) {
                    LOG_ERROR("CAmbaCloudAgent::uploadThread, uploadH264stream() failed, err = %d, %s\n", err, gfGetErrorCodeString(err));
                    cloudAgentDisconnect();
                    connected = 0;
                    audio_extradata_send = 0;
                    video_extradata_send = 0;
                    __release_data_packet(msg.ptr, mpRingMemPool);
                    usleep(DRetryIntervalUS);
                    continue;
                }
            } else {
                err = uploadH264stream(pdata, data_size, (TU32)seq_num, extra_flag);
                if (DUnlikely(EECode_OK != err)) {
                    LOG_ERROR("CAmbaCloudAgent::uploadThread, uploadH264stream() failed, err = %d, %s\n", err, gfGetErrorCodeString(err));
                    cloudAgentDisconnect();
                    connected = 0;
                    audio_extradata_send = 0;
                    video_extradata_send = 0;
                    __release_data_packet(msg.ptr, mpRingMemPool);
                    usleep(DRetryIntervalUS);
                    continue;
                }
                if (DUnlikely(sync_video)) {
#ifdef D_PRINT_AVSYNC
                    printf("[sync video]: (%d, %lld)\n", (TU32)seq_num, cur_pts);
#endif
                    mpCloudClientAgentV2->VideoSync((TTime)cur_pts - mBeginTimeStamp, seq_num);
                }
            }
            discard_non_idr = 0;
            __release_data_packet(msg.ptr, mpRingMemPool);
        } else if (queue == mpDeviceAudioDataQueue) {
            DASSERT(mbEnableAudio && (AM_TRANSFER_PACKET_TYPE_AAC == p_header->dataType));

            //printf("get audio %p, len %d, seq %llu\n", msg.ptr, p_header->dataLen, p_header->seqNum);

            if (!audio_extradata_send) {
                err = mpCloudClientAgentV2->SetupAudioParams(mAudioFormat, mAudioChannelNumber, mAudioSampleFrequency, mAudioBitrate, mpAudioExtraData, (TU16)mAudioExtraDataSize);
                if (DUnlikely(EECode_OK != err)) {
                    LOG_ERROR("CAmbaCloudAgent::uploadThread, setAudioParams() failed, err = %d, %s\n", err, gfGetErrorCodeString(err));
                    cloudAgentDisconnect();
                    connected = 0;
                    audio_extradata_send = 0;
                    video_extradata_send = 0;
                    __release_data_packet(msg.ptr, mpAudioRingMemPool);
                    usleep(DRetryIntervalUS);
                    continue;
                }
                audio_extradata_send = 1;
            }
            pdata = (TU8 *) msg.ptr + sizeof(AmTransferPacketHeader);
            data_size = p_header->dataLen;

            TU64 seq_num = p_header->seqNum;
            TU64 cur_pts = p_header->dataPTS;

#ifdef D_PRINT_AVSYNC
            printf("             uploadAACstream(%d, %lld)\n", (TU32)seq_num, cur_pts);
#endif

            if (DUnlikely(!mbAudioUploadingStarted)) {
                mbAudioUploadingStarted = 1;
                mpCloudClientAgentV2->AudioSync((TTime)cur_pts - mBeginTimeStamp, seq_num);

                err = uploadAACstream(pdata, data_size, seq_num);
                if (DUnlikely(EECode_OK != err)) {
                    LOG_ERROR("CAmbaCloudAgent::uploadThread, uploadAACstream(%d) failed, err=%d, %s\n", data_size, err, gfGetErrorCodeString(err));
                    cloudAgentDisconnect();
                    connected = 0;
                    audio_extradata_send = 0;
                    video_extradata_send = 0;
                    __release_data_packet(msg.ptr, mpAudioRingMemPool);
                    usleep(DRetryIntervalUS);
                    continue;
                }
            } else {
                err = uploadAACstream(pdata, data_size, seq_num);
                if (DUnlikely(EECode_OK != err)) {
                    LOG_ERROR("CAmbaCloudAgent::uploadThread, uploadAACstream(%d) failed, err=%d, %s\n", data_size, err, gfGetErrorCodeString(err));
                    cloudAgentDisconnect();
                    connected = 0;
                    audio_extradata_send = 0;
                    video_extradata_send = 0;
                    __release_data_packet(msg.ptr, mpAudioRingMemPool);
                    usleep(DRetryIntervalUS);
                    continue;
                }
                if (DUnlikely(DClientNeedSync(seq_num))) {
#ifdef D_PRINT_AVSYNC
                    printf("[sync audio]: (%d, %lld)\n", (TU32)seq_num, cur_pts);
#endif
                    mpCloudClientAgentV2->AudioSync((TTime)cur_pts - mBeginTimeStamp, seq_num);
                }
            }

            __release_data_packet(msg.ptr, mpAudioRingMemPool);
        } else {
            //LOG_INFO("skip non h264/aac type %d\n", p_header->dataType);
            LOG_FATAL("must not comes here\n");
        }
    }

    LOG_INFO("uploadThread end.\n");
}

void CAmbaCloudAgent::processReceiveMsgCallback(TUniqueID id, TU8 *p_message, TU32 message_length, TU32 need_reply)
{
    LOG_NOTICE("receive message, id %llx, message length %d, need_reply %d\n", id, message_length, need_reply);
    gfPrintMemory(p_message, message_length);
}

void CAmbaCloudAgent::processSystemNotifyCallBack(ESystemNotify notify, void *notify_context)
{
    switch (notify) {

        default:
            LOG_ERROR("processSystemNotifyCallBack, notify=0x%x, notify_context=%p not supported.\n", notify, notify_context);
            break;
    }
}

void CAmbaCloudAgent::processErrorCallback(EECode err)
{
    LOG_WARN("processErrorCallback, err=%d, %s\n", err, gfGetErrorCodeString(err));
}

EECode CAmbaCloudAgent::processAdminCallback(TChar *source, void *p_source, void *p_port, TU8 *p_data, TU32 datasize, TU32 type, TU32 cat, TU32 subtype)
{
    LOG_NOTICE("processAdminCallback, datasize [%u]\n", datasize);
    EECode err = EECode_OK;
    TU8 flag = ESACPConnectResult_OK;
    TU8 *p_cur = p_data;

    if (DUnlikely(ESACPTypeCategory_AdminChannel != cat || !p_data)) {
        LOG_ERROR("BAD cmd, type %08x, cat %08x, subtype %08x, p_data=%p\n", type, cat, subtype, p_data);
        return EECode_BadCommand;
    }

    switch (subtype) {

        case ESACPAdminSubType_SetupDeviceFactoryInfo: {
                LOG_NOTICE("processAdminCallback, ESACPAdminSubType_SetupDeviceFactoryInfo start\n");

                TU16 tlv16type = 0, tlv16len = 0;
                TChar read_string[32] = {0};
                DBER16(tlv16type, p_cur);
                DBER16(tlv16len, (p_cur + 2));
                if (DUnlikely(ETLV16Type_IMServerUrl != tlv16type || 0 == tlv16len || tlv16len >= sizeof(mIMServerUrl))) {
                    LOG_ERROR("parse ETLV16Type_IMServerUrl failed, tlv16type = %hu, tlv16len_imsrvurl = %hu\n", tlv16type, tlv16len);
                    flag = ESACPRequestResult_BadParam;
                    break;
                }
                memcpy(mIMServerUrl, p_cur + 4, tlv16len);
                p_cur += 4 + tlv16len;

                DBER16(tlv16type, p_cur);
                DBER16(tlv16len, (p_cur + 2));
                if (DUnlikely(ETLV16Type_IMServerPort != tlv16type || sizeof(mIMServerPort) != tlv16len)) {
                    LOG_ERROR("parse ETLV16Type_IMServerPort failed, tlv16type=%hu, tlv16len=%hu\n", tlv16type, tlv16len);
                    flag = ESACPRequestResult_BadParam;
                    break;
                }
                mIMServerPort = (((TSocketPort)p_cur[4]) << 8) | ((TSocketPort)p_cur[5]);
                p_cur += 4 + tlv16len;

                DBER16(tlv16type, p_cur);
                DBER16(tlv16len, (p_cur + 2));
                if (DUnlikely(ETLV16Type_DeviceID != tlv16type || sizeof(mDeviceUniqueID) != tlv16len)) {
                    LOG_ERROR("parse ETLV16Type_DeviceID failed, tlv16type=%hu, tlv16len=%hu\n", tlv16type, tlv16len);
                    flag = ESACPRequestResult_BadParam;
                    break;
                }
                p_cur += 4;
                DBER64(mDeviceUniqueID, p_cur);
                p_cur += tlv16len;

                DBER16(tlv16type, p_cur);
                DBER16(tlv16len, (p_cur + 2));
                if (DUnlikely(ETLV16Type_DeviceName != tlv16type || 0 == tlv16len || tlv16len >= sizeof(mDeviceAccountName))) {
                    LOG_ERROR("parse ETLV16Type_DeviceName failed, tlv16type=%hu, tlv16len_devname=%hu\n", tlv16type, tlv16len);
                    flag = ESACPRequestResult_BadParam;
                    break;
                }
                memcpy(mDeviceAccountName, p_cur + 4, tlv16len);
                p_cur += 4 + tlv16len;

                DBER16(tlv16type, p_cur);
                DBER16(tlv16len, (p_cur + 2));
                if (DUnlikely(ETLV16Type_DevicePassword != tlv16type || 0 == tlv16len || tlv16len >= DIdentificationStringLength)) {
                    LOG_ERROR("parse ETLV16Type_DevicePassword failed, tlv16type=%hu, password length =%hu(should<=%d)\n", tlv16type, tlv16len, DIdentificationStringLength);
                    flag = ESACPRequestResult_BadParam;
                    break;
                }
                memcpy(mDeviceAccountPassword, p_cur + 4, tlv16len);
                p_cur += 4 + tlv16len;

                writeConfigureFile();

                SAgentMsg msg = {0};
                msg.cmd = EAgentCmd_SetupDone;
                __msg_queue_put(mpDeviceCommandQueue, &msg);
            }
            break;

        default: {
                LOG_ERROR("Bad subtype %d\n", subtype);
                flag = ESACPRequestResult_ServerNotSupport;
            }
            break;

    }

    return EECode_OK;
}

EECode CAmbaCloudAgent::createCloudAgent()
{
    if (DLikely(!mpCloudClientAgentV2)) {
        mpCloudClientAgentV2 = gfCreateCloudClientAgentV2(EProtocolType_SACP);
        if (DUnlikely(!mpCloudClientAgentV2)) {
            LOG_FATAL("gfCreateCloudClientAgentV2() fail\n");
            return EECode_NoMemory;
        }
        mpCloudClientAgentV2->SetHardwareAuthenticateCallback(__clientAgentAuthenticateCallBack);
    } else {
        LOG_WARN("already created\n");
    }
    return EECode_OK;
}

EECode CAmbaCloudAgent::cloudAgentConnectServer()
{
    if (DLikely(mpCloudClientAgentV2)) {
        EECode err = EECode_OK;
        err = mpCloudClientAgentV2->ConnectToServer(mpCloudServerUrl, mCloudServerPort, mDeviceAccountName, mDeviceDynamicPassword);
        if (EECode_OK == err) {
            if (DUnlikely(!mbApplicationStarted)) {
                err = mpCloudClientAgentV2->DiscontiousIndicator();
                if (DLikely(EECode_OK == err)) {
                    mbApplicationStarted = 1;
                    return EECode_OK;
                }
                LOG_ERROR("DiscontiousIndicator fail! ret %d, %s\n", err, gfGetErrorCodeString(err));
                return err;
            }
            return EECode_OK;
        } else {
            LOG_WARN("connect data server(%s, %d) fail, name %s, dynamic password %s, ret %d %s\n", mpCloudServerUrl, mCloudServerPort, mDeviceAccountName, mDeviceDynamicPassword, err, gfGetErrorCodeString(err));
            return err;
        }
    } else {
        LOG_FATAL("NULL mpCloudClientAgentV2\n");
        return EECode_InternalLogicalBug;
    }
    return EECode_OK;
}

void CAmbaCloudAgent::cloudAgentDisconnect()
{
    if (DLikely(mpCloudClientAgentV2)) {
        mpCloudClientAgentV2->DisconnectToServer();
    } else {
        LOG_FATAL("NULL mpCloudClientAgentV2\n");
    }
}

EECode CAmbaCloudAgent::uploadH264stream(TU8 *pdata, TMemSize data_size, TU32 seq_num, TU32 keyframe)
{
    if (DLikely(mpCloudClientAgentV2)) {
        if (DUnlikely(keyframe)) {
            return mpCloudClientAgentV2->Uploading(pdata, data_size, ESACPDataChannelSubType_H264_NALU, seq_num, DSACPHeaderFlagBit_PacketKeyFrameIndicator);
        } else {
            return mpCloudClientAgentV2->Uploading(pdata, data_size, ESACPDataChannelSubType_H264_NALU, seq_num, 0);
        }
    } else {
        LOG_FATAL("NULL mpCloudClientAgentV2\n");
        return EECode_InternalLogicalBug;
    }
}

EECode CAmbaCloudAgent::uploadAACstream(TU8 *pdata, TMemSize data_size, TU32 seq_num)
{
    if (DLikely(mpCloudClientAgentV2)) {
        return mpCloudClientAgentV2->Uploading(pdata, data_size, ESACPDataChannelSubType_AAC, seq_num, 0);
    } else {
        LOG_FATAL("NULL mpCloudClientAgentV2\n");
        return EECode_InternalLogicalBug;
    }
}

EECode CAmbaCloudAgent::readConfigureFile()
{
    EECode err = EECode_OK;
    TChar read_string[256] = {0};
    TInt read_len = 0;

    IRunTimeConfigAPI *p_config_file = gfRunTimeConfigAPIFactory(ERunTimeConfigType_SimpleINI, 128);
    if (DUnlikely(!p_config_file)) {
        LOG_ERROR("gfRunTimeConfigAPIFactory(ERunTimeConfigType_SimpleINI) fail.\n");
        return EECode_NoMemory;
    }

    err = p_config_file->OpenConfigFile(mpAccountConfFilename, 0, 0, 1);
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("OpenConfigFile(%s) fail, ret %d, %s.\n", mpAccountConfFilename, err, gfGetErrorCodeString(err));
        return err;
    }

    err = p_config_file->GetContentValue("imsrvurl", (TChar *)read_string);
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("GetContentValue(imsrvurl) fail, ret %d, %s.\n", err, gfGetErrorCodeString(err));
        p_config_file->CloseConfigFile();
        return err;
    } else {
        read_len = snprintf(mIMServerUrl, sizeof(mIMServerUrl), "%s", read_string);
        LOG_NOTICE("[read configfile]: imsrvurl:%s, read_len=%d\n", mIMServerUrl, read_len);
    }

    err = p_config_file->GetContentValue("imsrvport", (TChar *)read_string);
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("GetContentValue(imsrvport) fail, ret %d, %s.\n", err, gfGetErrorCodeString(err));
        p_config_file->CloseConfigFile();
        return err;
    } else {
        read_len = sscanf(read_string, "%hu", &mIMServerPort);
        if (DUnlikely(1 != read_len)) {
            LOG_ERROR("GetContentValue(imsrvport) fail, %s, read_len=%d.\n", read_string, read_len);
            p_config_file->CloseConfigFile();
            return err;
        }
        LOG_NOTICE("[read configfile]: imsrvport:%hu\n", mIMServerPort);
    }

    err = p_config_file->GetContentValue("username", (TChar *)read_string);
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("GetContentValue(username) fail, ret %d, %s.\n", err, gfGetErrorCodeString(err));
        err = p_config_file->CloseConfigFile();
        return err;
    } else {
        read_len = snprintf(mDeviceAccountName, sizeof(mDeviceAccountName), "%s",  read_string);
        LOG_NOTICE("[read configfile]: username:%s\n", mDeviceAccountName);
    }

    err = p_config_file->GetContentValue("uniqueid", (TChar *)read_string);
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("GetContentValue(uniqueid) fail, ret %d, %s.\n", err, gfGetErrorCodeString(err));
        p_config_file->CloseConfigFile();
        return err;
    } else {
        read_len = sscanf(read_string, "%llx", &mDeviceUniqueID);
        if (DUnlikely(1 != read_len)) {
            LOG_ERROR("GetContentValue(uniqueid) fail, read_len=%d.\n", read_len);
            p_config_file->CloseConfigFile();
            return err;
        }
        LOG_NOTICE("[read configfile]: uniqueid:%llx\n", mDeviceUniqueID);
    }

    err = p_config_file->GetContentValue("password", (TChar *)read_string);
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("GetContentValue(password) fail, ret %d, %s.\n", err, gfGetErrorCodeString(err));
        p_config_file->CloseConfigFile();
        return err;
    } else {
        read_len = strlen(read_string);
        if (DUnlikely(read_len >= DIdentificationStringLength)) {
            LOG_FATAL("too long password %d, %s\n", read_len, read_string);
            return EECode_Error;
        }
        sprintf(mDeviceAccountPassword, "%s", read_string);
        LOG_NOTICE("[read configfile]: password:%s\n", mDeviceAccountPassword);
    }

    p_config_file->CloseConfigFile();
    p_config_file->Delete();

    return EECode_OK;
}

EECode CAmbaCloudAgent::writeConfigureFile()
{
    EECode err = EECode_OK;
    TChar read_string[256] = {0};
    TInt read_len = 0;

    IRunTimeConfigAPI *p_config_file = gfRunTimeConfigAPIFactory(ERunTimeConfigType_SimpleINI, 128);
    if (DUnlikely(!p_config_file)) {
        LOG_ERROR("gfRunTimeConfigAPIFactory(ERunTimeConfigType_SimpleINI) fail.\n");
        return EECode_NoMemory;
    }
    err = p_config_file->OpenConfigFile(mpAccountConfFilename, 1, 0, 1);
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("OpenConfigFile(%s) fail, ret %d, %s.\n", mpAccountConfFilename, err, gfGetErrorCodeString(err));
    } else {
        err = p_config_file->SetContentValue("imsrvurl", (TChar *)mIMServerUrl);
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("SetContentValue(imsrvurl) fail, ret %d, %s.\n", err, gfGetErrorCodeString(err));
        }
        sprintf(read_string, "%hu", mIMServerPort);
        err = p_config_file->SetContentValue("imsrvport", (TChar *)read_string);
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("SetContentValue(imsrvport) fail, ret %d, %s.\n", err, gfGetErrorCodeString(err));
        }
        err = p_config_file->SetContentValue("username", (TChar *)mDeviceAccountName);
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("SetContentValue(username) fail, ret %d, %s.\n", err, gfGetErrorCodeString(err));
        }
        sprintf(read_string, "%llx", mDeviceUniqueID);
        err = p_config_file->SetContentValue("uniqueid", (TChar *)read_string);
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("cSetContentValue(uniqueid) fail, ret %d, %s.\n", err, gfGetErrorCodeString(err));
        }
        err = p_config_file->SetContentValue("password", (TChar *)mDeviceAccountPassword);
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("SetContentValue(password) fail, ret %d, %s.\n", err, gfGetErrorCodeString(err));
        }
        err = p_config_file->SaveConfigFile(NULL);
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("SaveConfigFile(%s) fail, ret %d, %s.\n", mpAccountConfFilename, err, gfGetErrorCodeString(err));
        }
        err = p_config_file->CloseConfigFile();
        if (DUnlikely(EECode_OK != err)) {
            LOG_ERROR("CloseConfigFile(%s) fail, ret %d, %s.\n", mpAccountConfFilename, err, gfGetErrorCodeString(err));
        }
    }

    p_config_file->Delete();

    return EECode_OK;
}

EECode CAmbaCloudAgent::openAdminPort()
{
    LOG_NOTICE("openAdminPort()\n");

    if (DUnlikely(mpAdminServerPort)) {
        LOG_ERROR("already opened\n");
        return EECode_BadState;
    }

    mpAdminServerPort = CICommunicationServerPort::Create((void *)this, AdminCallback, DDefaultSACPAdminPort);
    if (DUnlikely(!mpAdminServerPort)) {
        LOG_ERROR("CICommunicationServerPort::Create fail\n");
        return EECode_NoMemory;
    }

    SCommunicationPortSource *source_port = NULL;
    EECode err = mpAdminServerPort->AddSource((TChar *)"testdevice", (TChar *)"123456", NULL, (TChar *)"21436587", NULL, source_port);
    if (DUnlikely((EECode_OK != err) || (!source_port))) {
        LOG_ERROR("mpAdminServerPort->AddSource fail, ret %d, %s, source_port %p\n", err, gfGetErrorCodeString(err), source_port);
        return err;
    }

    err = mpAdminServerPort->Start(DDefaultSACPAdminPort);
    if (DUnlikely(EECode_OK != err)) {
        LOG_ERROR("mpAdminServerPort->Start fail, ret %d, %s\n", err, gfGetErrorCodeString(err));
        return err;
    }

    return EECode_OK;
}

void CAmbaCloudAgent::closeAdminPort()
{
    LOG_NOTICE("closeAdminPort()\n");

    if (DLikely(mpAdminServerPort)) {
        mpAdminServerPort->Stop();
        mpAdminServerPort->Destroy();
        mpAdminServerPort = NULL;
    } else {
        LOG_WARN("not open yet\n");
    }
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

