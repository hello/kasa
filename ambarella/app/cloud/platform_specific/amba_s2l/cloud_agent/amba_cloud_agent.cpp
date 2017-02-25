/*******************************************************************************
 * amba_cloud_agent.cpp
 *
 * History:
 *  2015/03/03 - [Zhi He] create file
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

#include <sys/time.h>
#include <signal.h>

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <sched.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <linux/rtc.h>
#include <time.h>
#include <assert.h>
#include <basetypes.h>
#include <iav_ioctl.h>

#include <semaphore.h>

#include <pthread.h>

#include "am_export_if.h"

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

#include "security_lib_wrapper.h"

#include "amba_cloud_agent.h"

#define D_PRINT_AVSYNC

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

    while (1) {

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

static TU32 __clientAgentAuthenticateCallBack(TU8 *p_input, TU32 input_length)
{
    DASSERT((DDynamicInputStringLength) == input_length);

    return gfHashAlgorithmV3(p_input, input_length);
}

CAmbaCloudAgent *CAmbaCloudAgent::Create(const TChar *account_filename)
{
    CAmbaCloudAgent *result = new CAmbaCloudAgent(account_filename);
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
    mpFreePacketList = new CIDoubleLinkedList();
    if (DUnlikely(!mpFreePacketList)) {
        LOG_FATAL("new CIDoubleLinkedList() fail\n");
        return EECode_NoMemory;
    }

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

    mpDeviceSecondVideoDataQueue = (SAgentMsgQueue *)DDBG_MALLOC(sizeof(SAgentMsgQueue), "CSDQ");
    if (DUnlikely(!mpDeviceSecondVideoDataQueue)) {
        LOG_FATAL("DDBG_MALLOC mpDeviceAudioDataQueue failed\n");
        return EECode_NoMemory;
    }
    if (DUnlikely(0 != __msg_slave_queue_init(mpDeviceSecondVideoDataQueue, mpDeviceVideoDataQueue))) {
        LOG_FATAL("init mpDeviceSecondVideoDataQueue failed\n");
        return EECode_NoMemory;
    }

    return EECode_OK;
}

#define DINVALID_STREAM_INDEX 0xff

CAmbaCloudAgent::CAmbaCloudAgent(const TChar *account_filename)
    : mpAccountConfFilename(account_filename)
    , mDeviceUniqueID(0)
    , mVideoFormat(ESACPDataChannelSubType_H264_NALU)
    , mSecondVideoFormat(ESACPDataChannelSubType_H264_NALU)
    , mAudioFormat(ESACPDataChannelSubType_AAC)
    , mbReadDataThreadRun(0)
    , mbMainThreadRun(0)
    , mbUploadThreadRun(0)
    , mbApplicationStarted(0)
    , mbEnableVideo(1)
    , mbEnableAudio(0)
    , mbVideoUploadingStarted(0)
    , mbAudioUploadingStarted(0)
    , mVideoStreamFormat(AM_EXPORT_PACKET_FORMAT_AVC)
    , mAudioStreamFormat(AM_EXPORT_PACKET_FORMAT_AAC_48KHZ)
    , mVideoStreamIndex(DINVALID_STREAM_INDEX)
    , mAudioStreamIndex(DINVALID_STREAM_INDEX)
    , mbEnableVideoEncryption(1)
    , mbStartedVideoEncryption(0)
    , mAudioChannelNumber(1)
    , mAudioSampleFrequency(16000)
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
    , mpDeviceSecondVideoDataQueue(NULL)
    , mpCloudServerUrl(NULL)
    , mIMServerPort(DDefaultSACPServerPort)
    , mCloudServerPort(DDefaultSACPServerPort)
    , mpAdminServerPort(NULL)
    , mpIMClientAgent(NULL)
    , mpCloudClientAgentV2(NULL)
    , mVideoSeqNum(0)
    , mAudioSeqNum(0)
    , mBeginTimeStamp(0)
{
    memset(mDeviceAccountName, 0x0, sizeof(mDeviceAccountName));
    memset(mDeviceAccountPassword, 0x0, sizeof(mDeviceAccountPassword));
    memset(mDeviceDynamicPassword, 0x0, sizeof(mDeviceDynamicPassword));

    memset(mIMServerUrl, 0x0, sizeof(mIMServerUrl));

    mpDataExportClient = NULL;

    mbEnableSecondStream = 0;
    mbEnableSecondStreamAudio = 0;
    mbSecondstreamVideoUploadingStarted = 0;
    mbSecondstreamAudioUploadingStarted = 0;
    mbSecondStreamAgentConnected = 0;
    mpCloudClientAgentV2SecStream = NULL;

    mSecondVideoStreamFormat = AM_EXPORT_PACKET_FORMAT_AVC;
    mSecondVideoStreamIndex = 1;

    mSecondVideoFramerateDen = DDefaultVideoFramerateDen;
    mSecondVideoFramerate = 30;
    mSecondVideoResWidth = 640;
    mSecondVideoResHeight = 480;
    mSecondVideoBitrate = 0;
    mpSecondVideoExtraData = NULL;
    mSecondVideoExtraDataSize = 0;

    mSecondVideoGOPN = 120;
    mSecondVideoGOPM = 1;
    mSecondVideoGOPIDRInterval = 1;

    mSecondVideoSeqNum = 0;
    mSecondBeginTimeStamp = 0;
    mSecondLastVideoPTS = 0;

#ifdef D_ENABLE_ENCRYPTION
    mpDataCypher = NULL;
    mpDataCypherSecondStream = NULL;
    snprintf(mDataCypherKey, DMAX_SYMMETRIC_KEY_LENGTH_BYTES, "%s", "123456789abcdef0");
    snprintf(mDataCypherIV, DMAX_SYMMETRIC_KEY_LENGTH_BYTES, "%s", "abcdef0123456789");

    mDataBufferSize = 0;
    mpCypherOutputBuffer = NULL;
#endif

}

CAmbaCloudAgent::~CAmbaCloudAgent()
{
    if (mpDeviceCommandQueue) {
        __msg_queue_deinit(mpDeviceCommandQueue);
        DDBG_FREE(mpDeviceCommandQueue, "CDCQ");
        mpDeviceCommandQueue = NULL;
    }
    if (mpDeviceVideoDataQueue) {
        __msg_queue_deinit(mpDeviceVideoDataQueue);
        DDBG_FREE(mpDeviceVideoDataQueue, "CVDQ");
        mpDeviceVideoDataQueue = NULL;
    }

    if (mpDeviceAudioDataQueue) {
        __msg_queue_deinit(mpDeviceAudioDataQueue);
        DDBG_FREE(mpDeviceAudioDataQueue, "CVAQ");
        mpDeviceAudioDataQueue = NULL;
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

    if (mpCloudClientAgentV2SecStream) {
        mpCloudClientAgentV2SecStream->Delete();
        mpCloudClientAgentV2SecStream = NULL;
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

    if (mpSecondVideoExtraData) {
        DDBG_FREE(mpSecondVideoExtraData, "CVED");
        mpSecondVideoExtraData = NULL;
        mSecondVideoExtraDataSize = 0;
    }

    if (mpFreePacketList) {
        delete mpFreePacketList;
        mpFreePacketList = NULL;
    }

    if (mpDataExportClient) {
        mpDataExportClient->destroy();
        mpDataExportClient = NULL;
    }

#ifdef D_ENABLE_ENCRYPTION
    if (mpDataCypher) {
        destroy_symmetric_cypher(mpDataCypher);
        mpDataCypher = NULL;
    }

    if (mpDataCypherSecondStream) {
        destroy_symmetric_cypher(mpDataCypherSecondStream);
        mpDataCypherSecondStream = NULL;
    }

    if (mpCypherOutputBuffer) {
        DDBG_FREE(mpCypherOutputBuffer, "CYOB");
        mpCypherOutputBuffer = NULL;
    }
#endif

}

EECode CAmbaCloudAgent::Start(TU8 stream_index, TU8 audio_disable, TU8 m, TU8 print_pts_seq, TU8 fps, TU8 enable_second_stream)
{
    if (DUnlikely(mbMainThreadRun)) {
        LOG_WARN("CAmbaCloudAgent already started\n");
        return EECode_BadState;
    }
    mbMainThreadRun = 1;
    if (!enable_second_stream) {
        mVideoStreamIndex = stream_index;
        mbEnableSecondStream = 0;
    } else {
        mVideoStreamIndex = 0;
        mSecondVideoStreamIndex = 1;
        mbEnableSecondStream = 1;
    }
    mbEnableAudio = !audio_disable;
    mbEnableSecondStreamAudio = mbEnableAudio;
    mDebugPrintPTSSeqNum = print_pts_seq;

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

    mVideoGOPM = m;

#ifndef D_REMOVE_HARD_CODE

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

    //hard code here
    if (!fps) {
        if (1 == stream_index) {
            mVideoFramerateDen = 6006;
            mVideoFramerate = 30;
            mVideoResWidth = 640;
            mVideoResHeight = 480;
        } else {
            mVideoFramerateDen = 3003;
            mVideoFramerate = 30;
            mVideoResWidth = 1920;
            mVideoResHeight = 1080;
        }
    } else {
        if (30 == fps) {
            mVideoFramerateDen = 3003;
            mVideoFramerate = 30;
        } else if (15 == fps) {
            mVideoFramerateDen = 6006;
            mVideoFramerate = 15;
        } else {
            mVideoFramerateDen = 90000 / fps;
            mVideoFramerate = fps;
        }
        if (1 == stream_index) {
            mVideoResWidth = 640;
            mVideoResHeight = 480;
        } else {
            mVideoResWidth = 1920;
            mVideoResHeight = 1080;
        }
    }
    LOG_NOTICE("video index %d, frame rate %f, width %d, height %d\n", stream_index, (float)90000 / (float)mVideoFramerateDen, mVideoResWidth, mVideoResHeight);

#endif

#ifdef D_ENABLE_ENCRYPTION
    mpDataCypher = create_symmetric_cypher(CYPHER_TYPE_AES128, BLOCK_CYPHER_MODE_CTR);
    if (DUnlikely(!mpDataCypher)) {
        LOG_WARN("create_symmetric_cypher fail!\n");
    } else {
        if (enable_second_stream) {
            mpDataCypherSecondStream = create_symmetric_cypher(CYPHER_TYPE_AES128, BLOCK_CYPHER_MODE_CTR);
            if (DUnlikely(!mpDataCypherSecondStream)) {
                LOG_WARN("create_symmetric_cypher fail!\n");
            }
        }
        mDataBufferSize = 2 * 1024 * 1024;
        mpCypherOutputBuffer = (TU8 *) DDBG_MALLOC(mDataBufferSize, "CACB");
        if (!mpCypherOutputBuffer) {
            LOG_WARN("no memory!\n");
            return EECode_NoMemory;
        }
    }
#endif

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

EECode CAmbaCloudAgent::ControlDataEncryption(TU8 enable)
{
    mbEnableVideoEncryption = enable;
    return EECode_OK;
}

EECode CAmbaCloudAgent::UpdateDataEncryptionSetting(TChar *key, TChar *iv, TU32 cypher_type, TU32 cypher_mode)
{
    LOG_FATAL("add support!\n");
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
    bool ret = 0;
    TInt wait_first_idr = 1;
    TInt video_started = 0;
    TInt sec_wait_first_idr = 1;
    TInt sec_video_started = 0;
    TInt size = 0;
    TInt send_size = 0;
    unsigned long long pts = 0;

    SAgentMsg msg;
    AMExportPacket *p_packet = NULL;
    AMExportConfig config = {0};
    TU8 *ptmp = NULL;

    if (DUnlikely(mpDataExportClient)) {
        LOG_ERROR("destroy previous client!\n");
        mpDataExportClient->destroy();
        mpDataExportClient = NULL;
    }

wait_apps_laucher:
    mpDataExportClient = am_create_export_client(AM_EXPORT_TYPE_UNIX_DOMAIN_SOCKET, &config);
    if (DUnlikely(!mpDataExportClient)) {
        LOG_ERROR("am_create_export_client(), sdk not started?\n");
        usleep(100000);
        goto wait_apps_laucher;
    }

    ret = mpDataExportClient->connect_server((char *) DEXPORT_PATH);
    if (DUnlikely(!ret)) {
        LOG_ERROR("p_client->connect_server() fail\n");
        mpDataExportClient->destroy();
        usleep(100000);
        goto wait_apps_laucher;
    }

    DASSERT(mpDeviceVideoDataQueue);
    DASSERT(mpDeviceAudioDataQueue);

    mVideoSeqNum = 0;
    mAudioSeqNum = 0;

    LOG_NOTICE("readDataThread begin\n");
    while (mbReadDataThreadRun) {
        TInt to_sync = checkVideoQueue(mpDeviceVideoDataQueue);
        if (DUnlikely(-1 != to_sync)) {
            LOG_NOTICE("readDataThread, flush = %d\n", to_sync);
            if (to_sync) {
                wait_first_idr = 0;
            } else {
                wait_first_idr = 1;
            }
        }
        checkAudioQueue(mpDeviceAudioDataQueue);
        if (!p_packet) {
            p_packet = allocateDataPacket();
            p_packet->user_alloc_memory = 0;
        } else {
            LOG_WARN("why not NULL\n");
        }
        ret = mpDataExportClient->receive(p_packet);
        if (DLikely(ret)) {
            p_packet->user_alloc_memory = 0;
            if (AM_EXPORT_PACKET_TYPE_VIDEO_DATA == p_packet->packet_type) {
                if ((DINVALID_STREAM_INDEX == p_packet->stream_index) || (!mbEnableVideo)) {
                    releaseDataPacket(p_packet);
                    p_packet = NULL;
                    continue;
                }
                if ((!mVideoResWidth) || (!mVideoResHeight)) {
                    releaseDataPacket(p_packet);
                    p_packet = NULL;
                    continue;
                }

                if (mVideoStreamIndex == p_packet->stream_index) {
                    if (DUnlikely(wait_first_idr)) {
                        if (!p_packet->is_key_frame) {
                            LOG_PRINTF("wait video IDR ...\n");
                            releaseDataPacket(p_packet);
                            p_packet = NULL;
                            continue;
                        } else {
                            wait_first_idr = 0;
                            msg.flags = DDeviceSyncPointFlag;
                            LOG_PRINTF("wait video IDR done\n");
                            video_started = 1;
                        }
                    } else if (p_packet->is_key_frame) {
                        msg.flags = DDeviceSyncPointFlag;
                    } else {
                        msg.flags = 0;
                    }
                    msg.ptr = (void *) p_packet;

                    if (DUnlikely(0 == mVideoSeqNum)) {
                        mBeginTimeStamp = p_packet->pts;
                        LOG_NOTICE("mBeginTimeStamp %lld\n", mBeginTimeStamp);
                    }
                    //LOG_NOTICE("video pts %lld, offset %lld\n", p_packet->pts, p_packet->pts - mBeginTimeStamp);
                    //LOG_NOTICE("read video packet, format %02x, index %d, size %ld\n", p_packet->packet_format, p_packet->stream_index, p_packet->data_size);
                    mLastVideoPTS = p_packet->pts;
                    p_packet->seq_num = mVideoSeqNum ++;
                    msg.ptr = (void *) p_packet;
                    //LOG_NOTICE("read video packet, size %d, seq %d\n", p_packet->data_size, p_packet->seq_num);
                    __msg_queue_put(mpDeviceVideoDataQueue, &msg);
                    p_packet = NULL;
                } else if (mbEnableSecondStream && (mSecondVideoStreamIndex == p_packet->stream_index)) {

                    if (DUnlikely(sec_wait_first_idr)) {
                        if (!p_packet->is_key_frame) {
                            LOG_PRINTF("wait video IDR ...\n");
                            releaseDataPacket(p_packet);
                            p_packet = NULL;
                            continue;
                        } else {
                            sec_wait_first_idr = 0;
                            msg.flags = DDeviceSyncPointFlag;
                            LOG_PRINTF("wait video IDR done (sec)\n");
                            sec_video_started = 1;
                        }
                    } else if (p_packet->is_key_frame) {
                        msg.flags = DDeviceSyncPointFlag;
                    } else {
                        msg.flags = 0;
                    }
                    msg.ptr = (void *) p_packet;

                    if (DUnlikely(0 == mSecondVideoSeqNum)) {
                        mSecondBeginTimeStamp = p_packet->pts;
                        LOG_NOTICE("mSecondBeginTimeStamp %lld\n", mSecondBeginTimeStamp);
                    }
                    //LOG_NOTICE("video pts %lld, offset %lld\n", p_packet->pts, p_packet->pts - mBeginTimeStamp);
                    //LOG_NOTICE("read video packet, format %02x, index %d, size %ld\n", p_packet->packet_format, p_packet->stream_index, p_packet->data_size);
                    mSecondLastVideoPTS = p_packet->pts;
                    p_packet->seq_num = mSecondVideoSeqNum ++;
                    msg.ptr = (void *) p_packet;
                    //LOG_NOTICE("read video packet, size %d, seq %d\n", p_packet->data_size, p_packet->seq_num);
                    __msg_queue_put(mpDeviceSecondVideoDataQueue, &msg);
                    p_packet = NULL;
                } else {
                    releaseDataPacket(p_packet);
                    p_packet = NULL;
                    continue;
                }

            } else if (AM_EXPORT_PACKET_TYPE_AUDIO_DATA == p_packet->packet_type) {

                if ((!mbEnableAudio) || (p_packet->packet_format != mAudioStreamFormat) || (!mpAudioExtraData)) {
                    releaseDataPacket(p_packet);
                    p_packet = NULL;
                    continue;
                }

                if (DUnlikely(!video_started)) {
                    LOG_NOTICE("wait video start\n");
                    releaseDataPacket(p_packet);
                    p_packet = NULL;
                    continue;
                }

                //LOG_NOTICE("audio pts %lld, offset %lld\n", p_packet->pts, p_packet->pts - mBeginTimeStamp);
                //LOG_NOTICE("read audio packet, format %02x, index %d, size %ld\n", p_packet->packet_format, p_packet->stream_index, p_packet->data_size);
                mLastAudioPTS = p_packet->pts;
                ptmp = p_packet->data_ptr;

                TU32 frame_length = __adts_length(ptmp);
                if (frame_length == p_packet->data_size) {
                    //LOG_NOTICE("single audio packet\n");
                    p_packet->seq_num = mAudioSeqNum ++;
                    msg.ptr = (void *) p_packet;
                    __msg_queue_put(mpDeviceAudioDataQueue, &msg);
                    p_packet = NULL;
                } else {
                    AMExportPacket *p_new_packet = NULL;
                    //LOG_NOTICE("multiple audio packet\n");
                    do {
                        p_new_packet = allocateDataPacket();
                        *p_new_packet = *p_packet;
                        p_new_packet->user_alloc_memory = 1;
                        allocateMemoryInPacket(p_new_packet, frame_length);
                        memcpy((void *)p_new_packet->data_ptr, ptmp, frame_length);
                        p_new_packet->data_size = frame_length;
                        p_new_packet->seq_num = mAudioSeqNum ++;
                        p_new_packet->pts = mLastAudioPTS;
                        mLastAudioPTS += 1920;
                        msg.ptr = (void *) p_new_packet;
                        __msg_queue_put(mpDeviceAudioDataQueue, &msg);

                        ptmp += frame_length;
                        if ((unsigned long)(p_packet->data_ptr + p_packet->data_size) < (unsigned long)(ptmp + 10)) {
                            break;
                        }
                        frame_length = __adts_length(ptmp);
                        if ((unsigned long)(p_packet->data_ptr + p_packet->data_size) < (unsigned long)(ptmp + frame_length)) {
                            LOG_ERROR("data orruption?\n");
                            break;
                        }
                    } while (1);
                    releaseDataPacket(p_packet);
                    p_packet = NULL;
                }

            } else {
#ifdef D_REMOVE_HARD_CODE
                if (AM_EXPORT_PACKET_TYPE_AUDIO_INFO == p_packet->packet_type) {
                    if (p_packet->packet_format == mAudioStreamFormat) {
                        AMExportAudioInfo *audio_info = (AMExportAudioInfo *) p_packet->data_ptr;

                        if (audio_info && (!mpAudioExtraData)) {
                            TU32 generated_config_size = 0;
                            TU8 *generated_aac_config = NULL;

                            mAudioSampleFrequency = audio_info->samplerate;
                            mAudioChannelNumber = audio_info->channels;

                            LOG_PRINTF("audio sample rate %d, channels %d\n", mAudioSampleFrequency, mAudioChannelNumber);
                            generated_aac_config= gfGenerateAACExtraData(mAudioSampleFrequency, mAudioChannelNumber, generated_config_size);

                            if (DLikely(generated_aac_config && (16 > generated_config_size))) {
                                mpAudioExtraData = generated_aac_config;
                                mAudioExtraDataSize = (TMemSize)generated_config_size;
                                LOG_WARN("generate mAudioExtraDataSize=%lu, mpAudioExtraData first 2 bytes: 0x%x 0x%x\n", mAudioExtraDataSize, mpAudioExtraData[0], mpAudioExtraData[1]);
                            } else {
                                LOG_FATAL("no memory\n");
                            }
                        }

                    }
                } else if (AM_EXPORT_PACKET_TYPE_VIDEO_INFO == p_packet->packet_type) {
                    if ((mVideoStreamIndex == p_packet->stream_index) && (p_packet->packet_format == mVideoStreamFormat)) {
                        AMExportVideoInfo *video_info = (AMExportVideoInfo *) p_packet->data_ptr;

                        if (video_info) {
                            if ((1 < video_info->framerate_num) && (1 < video_info->framerate_den)) {
                                TU32 frnum = video_info->framerate_num;
                                mVideoFramerateDen = video_info->framerate_den;
                                if (mVideoFramerateDen && frnum) {
                                    mVideoFramerate = (frnum + mVideoFramerateDen / 2) / mVideoFramerateDen;

                                    if (90000 != frnum) {
                                        mVideoFramerateDen = (TU32) ((TU64)(((TU64) 90000 * (TU64) mVideoFramerateDen) / (TU64)frnum));
                                    }
                                }
                            } else {
                                mVideoFramerate = 30;
                                mVideoFramerateDen = 3003;
                                LOG_WARN("use default 90000/3003 = 29.97\n");
                            }
                            mVideoResWidth = video_info->width;
                            mVideoResHeight = video_info->height;
                            LOG_PRINTF("video resolution %dx%d, fr num %d, den %d, calculated fps %d, 90000/%d\n", mVideoResWidth, mVideoResHeight, video_info->framerate_num, video_info->framerate_den, mVideoFramerate, mVideoFramerateDen);
                        }

                    } else if (mbEnableSecondStream && (mSecondVideoStreamIndex == p_packet->stream_index) && (p_packet->packet_format == mSecondVideoStreamFormat)) {

                        AMExportVideoInfo *video_info = (AMExportVideoInfo *) p_packet->data_ptr;

                        if (video_info) {
                            if ((1 < video_info->framerate_num) && (1 < video_info->framerate_den)) {
                                TU32 frnum = video_info->framerate_num;
                                mSecondVideoFramerateDen = video_info->framerate_den;
                                if (mSecondVideoFramerateDen && frnum) {
                                    mSecondVideoFramerate = (frnum + mSecondVideoFramerateDen / 2) / mSecondVideoFramerateDen;

                                    if (90000 != frnum) {
                                        mSecondVideoFramerateDen = (TU32) ((TU64)(((TU64) 90000 * (TU64) mSecondVideoFramerateDen) / (TU64)frnum));
                                    }
                                }
                            } else {
                                mSecondVideoFramerate = 30;
                                mSecondVideoFramerateDen = 3003;
                                LOG_WARN("use default 90000/3003 = 29.97\n");
                            }
                            mSecondVideoResWidth = video_info->width;
                            mSecondVideoResHeight = video_info->height;
                            LOG_PRINTF("video (second stream) resolution %dx%d, fr num %d, den %d, calculated fps %d, 90000/%d\n", mSecondVideoResWidth, mSecondVideoResHeight, video_info->framerate_num, video_info->framerate_den, mSecondVideoFramerate, mSecondVideoFramerateDen);
                        }

                    }
                }
#endif
                releaseDataPacket(p_packet);
                p_packet = NULL;
            }
        } else {
            LOG_WARN("receive_packet fail, ret %d, server shut down\n");
            break;
        }
    }

    LOG_INFO("readDataThread end.\n");
    return;
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
    EECode err = EECode_OK;
    AMExportPacket *p_packet = NULL;

    TU8 extra_flag = 0;
    TU8 connected = 0;
    TU8 *pdata = NULL;
    TU32 data_size = 0;
    TU32 skip_size = 0;
    TU8 audio_extradata_send = 0, video_extradata_send = 0, sec_video_extradata_send = 0, discard_non_idr = 1, sec_discard_non_idr = 1;
    SAgentMsg msg;
    SAgentMsgQueue *queue = NULL;

    TU32 kr_count = 0;
    TU32 sync_video = 0;

    TU32 sec_kr_count = 0;
    TU32 sec_sync_video = 0;

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
        p_packet = (AMExportPacket *) msg.ptr;
        if (queue == mpDeviceVideoDataQueue) {
            pdata = p_packet->data_ptr;
            skip_size = __skip_delimter_size(pdata);
            pdata += skip_size;
            data_size = p_packet->data_size - skip_size;

            extra_flag = 0;
            sync_video = 0;
            if (DUnlikely(AM_EXPORT_VIDEO_FRAME_TYPE_IDR == p_packet->frame_type)) {
                extra_flag |= DSACPHeaderFlagBit_PacketKeyFrameIndicator;
                if (DUnlikely(!video_extradata_send)) {
                    if ((!mpVideoExtraData) || (!mVideoExtraDataSize)) {
                        TU8 *p_extradata = NULL;
                        TU32 extradata_size = 0;
                        err = gfGetH264Extradata(pdata, data_size, p_extradata, extradata_size);
                        if (DUnlikely(EECode_OK != err)) {
                            LOG_FATAL("can not find extra data, corrupted h264 data?\n");
                            gfPrintMemory(pdata, 64);
                            releaseDataPacket((void *) p_packet);
                            p_packet = NULL;
                            continue;
                        }
                        mVideoExtraDataSize = extradata_size;
                        mpVideoExtraData = (TU8 *) DDBG_MALLOC(mVideoExtraDataSize + 16, "CVED");
                        if (DUnlikely(!mpVideoExtraData)) {
                            LOG_FATAL("no memory, request size %ld\n", (mVideoExtraDataSize + 16));
                            releaseDataPacket((void *) p_packet);
                            p_packet = NULL;
                            continue;
                        }
                        memcpy(mpVideoExtraData, p_extradata, mVideoExtraDataSize);
                    }

                    LOG_NOTICE("video extra size %d\n", (TU32)mVideoExtraDataSize);
                    gfPrintMemory(mpVideoExtraData, (TU32)mVideoExtraDataSize);

                    err = mpCloudClientAgentV2->SetupVideoParams(mVideoFormat, ((mVideoFramerateDen) << 8) | (mVideoFramerate & 0xff), mVideoResWidth, mVideoResHeight, mVideoBitrate, mpVideoExtraData, (TU16)mVideoExtraDataSize, DBuildGOP(mVideoGOPM, mVideoGOPN, mVideoGOPIDRInterval));
                    if (DUnlikely(EECode_OK != err)) {
                        LOG_ERROR("CAmbaCloudAgent::uploadThread, setVideoParams() failed, err = %d, %s\n", err, gfGetErrorCodeString(err));
                        cloudAgentDisconnect();
                        connected = 0;
                        audio_extradata_send = 0;
                        video_extradata_send = 0;
                        sec_video_extradata_send = 0;
                        releaseDataPacket((void *) p_packet);
                        p_packet = NULL;
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
                LOG_WARN("skip non-idr data, type %d\n", p_packet->frame_type);
                releaseDataPacket((void *) p_packet);
                p_packet = NULL;
                continue;
            }

            if (((0x01 == pdata[2]) && (0x07 == (0x1f & pdata[3]))) || ((0x01 == pdata[3]) && (0x07 == (0x1f & pdata[4])))) {
                TU8 nal_type = 0;
                TU8 *ptmp = gfNALUFindFirstAVCSliceHeaderType(pdata, data_size, nal_type);
                if (ptmp) {
                    if ((nal_type & 0x1f) != 0x05) {
                        LOG_WARN("skip sps pps before non-IDR frame\n");
                        data_size -= (TU32)(ptmp - pdata);
                        pdata = ptmp;
                        extra_flag = 0;
                    } else {
                        extra_flag |= DSACPHeaderFlagBit_PacketExtraDataIndicator;
                    }
                } else {
                    LOG_FATAL("data corruption\n");
                }
            }

            TU64 seq_num = p_packet->seq_num;
            TU64 cur_pts = p_packet->pts;

#ifdef D_PRINT_AVSYNC
            if (mDebugPrintPTSSeqNum) {
                printf("    upload h264(%d, %lld)\n", (TU32)seq_num, cur_pts);
            }
#endif

            if (DUnlikely(!mbVideoUploadingStarted)) {
                LOG_NOTICE("[pts]: video upload begin pts %lld\n", cur_pts);
                mpCloudClientAgentV2->VideoSync((TTime)cur_pts, seq_num);
                mbVideoUploadingStarted = 1;
                err = uploadH264stream(pdata, data_size, (TU32)seq_num, extra_flag);
                if (DUnlikely(EECode_OK != err)) {
                    LOG_ERROR("uploadH264stream() failed, err = %d, %s\n", err, gfGetErrorCodeString(err));
                    cloudAgentDisconnect();
                    connected = 0;
                    audio_extradata_send = 0;
                    video_extradata_send = 0;
                    sec_video_extradata_send = 0;
                    releaseDataPacket((void *) p_packet);
                    p_packet = NULL;
                    usleep(DRetryIntervalUS);
                    continue;
                }
            } else {
                err = uploadH264stream(pdata, data_size, (TU32)seq_num, extra_flag);
                if (DUnlikely(EECode_OK != err)) {
                    LOG_ERROR("uploadH264stream() failed, err = %d, %s\n", err, gfGetErrorCodeString(err));
                    cloudAgentDisconnect();
                    connected = 0;
                    audio_extradata_send = 0;
                    video_extradata_send = 0;
                    sec_video_extradata_send = 0;
                    releaseDataPacket((void *) p_packet);
                    p_packet = NULL;
                    usleep(DRetryIntervalUS);
                    continue;
                }
                if (DUnlikely(sync_video)) {
#ifdef D_PRINT_AVSYNC
                    if (mDebugPrintPTSSeqNum) {
                        printf("[sync video]: (%d, %lld)\n", (TU32)seq_num, cur_pts);
                    }
#endif
                    mpCloudClientAgentV2->VideoSync((TTime)cur_pts, seq_num);
                }
            }
            discard_non_idr = 0;
            releaseDataPacket((void *) p_packet);
            p_packet = NULL;
        } else if (queue == mpDeviceAudioDataQueue) {
            //printf("get audio %p, len %d, seq %llu\n", msg.ptr, p_header->dataLen, p_header->seqNum);
            if (!audio_extradata_send) {
                err = mpCloudClientAgentV2->SetupAudioParams(mAudioFormat, mAudioChannelNumber, mAudioSampleFrequency, mAudioBitrate, mpAudioExtraData, (TU16)mAudioExtraDataSize);
                if (DUnlikely(EECode_OK != err)) {
                    LOG_ERROR("setAudioParams() failed, err = %d, %s\n", err, gfGetErrorCodeString(err));
                    cloudAgentDisconnect();
                    connected = 0;
                    audio_extradata_send = 0;
                    video_extradata_send = 0;
                    sec_video_extradata_send = 0;
                    releaseDataPacket((void *) p_packet);
                    p_packet = NULL;
                    usleep(DRetryIntervalUS);
                    continue;
                }
                if (mbEnableSecondStream && mbSecondStreamAgentConnected) {
                    err = mpCloudClientAgentV2SecStream->SetupAudioParams(mAudioFormat, mAudioChannelNumber, mAudioSampleFrequency, mAudioBitrate, mpAudioExtraData, (TU16)mAudioExtraDataSize);
                    if (DUnlikely(EECode_OK != err)) {
                        LOG_ERROR("setAudioParams() failed, err = %d, %s\n", err, gfGetErrorCodeString(err));
                        cloudAgentDisconnect();
                        connected = 0;
                        audio_extradata_send = 0;
                        video_extradata_send = 0;
                        sec_video_extradata_send = 0;
                        releaseDataPacket((void *) p_packet);
                        p_packet = NULL;
                        usleep(DRetryIntervalUS);
                        continue;
                    }
                }
                audio_extradata_send = 1;
            }

            pdata = (TU8 *) p_packet->data_ptr;
            data_size = p_packet->data_size;

            TU64 seq_num = p_packet->seq_num;
            TU64 cur_pts = p_packet->pts;

#ifdef D_PRINT_AVSYNC
            if (mDebugPrintPTSSeqNum) {
                printf("    upload aac(%d, %lld)\n", (TU32)seq_num, cur_pts);
            }
#endif

            if (DUnlikely(!mbAudioUploadingStarted)) {
                mbAudioUploadingStarted = 1;
                LOG_NOTICE("[pts]: audio upload begin pts %lld\n", cur_pts);
                mpCloudClientAgentV2->AudioSync((TTime)cur_pts, seq_num);

                if (mbEnableSecondStream && mbEnableSecondStreamAudio && mbSecondStreamAgentConnected) {
                    mpCloudClientAgentV2SecStream->AudioSync((TTime)cur_pts, seq_num);
                }

                err = uploadAACstream(pdata, data_size, seq_num);
                if (DUnlikely(EECode_OK != err)) {
                    LOG_ERROR("uploadAACstream(%d) failed, err=%d, %s\n", data_size, err, gfGetErrorCodeString(err));
                    cloudAgentDisconnect();
                    connected = 0;
                    audio_extradata_send = 0;
                    video_extradata_send = 0;
                    sec_video_extradata_send = 0;
                    releaseDataPacket((void *) p_packet);
                    p_packet = NULL;
                    usleep(DRetryIntervalUS);
                    continue;
                }
                if (mbEnableSecondStream && mbEnableSecondStreamAudio && mbSecondStreamAgentConnected) {
                    err = uploadAACstreamSecondStream(pdata, data_size, seq_num);
                    if (DUnlikely(EECode_OK != err)) {
                        LOG_ERROR("uploadAACstreamSecondStream(%d) failed, err=%d, %s\n", data_size, err, gfGetErrorCodeString(err));
                        cloudAgentDisconnect();
                        connected = 0;
                        audio_extradata_send = 0;
                        video_extradata_send = 0;
                        sec_video_extradata_send = 0;
                        releaseDataPacket((void *) p_packet);
                        p_packet = NULL;
                        usleep(DRetryIntervalUS);
                        continue;
                    }
                }
            } else {
                err = uploadAACstream(pdata, data_size, seq_num);
                if (DUnlikely(EECode_OK != err)) {
                    LOG_ERROR("uploadAACstream(%d) failed, err=%d, %s\n", data_size, err, gfGetErrorCodeString(err));
                    cloudAgentDisconnect();
                    connected = 0;
                    audio_extradata_send = 0;
                    video_extradata_send = 0;
                    sec_video_extradata_send = 0;
                    releaseDataPacket((void *) p_packet);
                    p_packet = NULL;
                    usleep(DRetryIntervalUS);
                    continue;
                }

                if (mbEnableSecondStream && mbEnableSecondStreamAudio && mbSecondStreamAgentConnected) {
                    err = uploadAACstreamSecondStream(pdata, data_size, seq_num);
                    if (DUnlikely(EECode_OK != err)) {
                        LOG_ERROR("uploadAACstreamSecondStream(%d) failed, err=%d, %s\n", data_size, err, gfGetErrorCodeString(err));
                        cloudAgentDisconnect();
                        connected = 0;
                        audio_extradata_send = 0;
                        video_extradata_send = 0;
                        sec_video_extradata_send = 0;
                        releaseDataPacket((void *) p_packet);
                        p_packet = NULL;
                        usleep(DRetryIntervalUS);
                        continue;
                    }
                }

                if (DUnlikely(DClientNeedSync(seq_num))) {
#ifdef D_PRINT_AVSYNC
                    if (mDebugPrintPTSSeqNum) {
                        printf("[sync audio]: (%d, %lld)\n", (TU32)seq_num, cur_pts);
                    }
#endif
                    mpCloudClientAgentV2->AudioSync((TTime)cur_pts, seq_num);
                    if (mbEnableSecondStream && mbEnableSecondStreamAudio && mbSecondStreamAgentConnected) {
                        mpCloudClientAgentV2SecStream->AudioSync((TTime)cur_pts, seq_num);
                    }
                }
            }

            releaseDataPacket((void *) p_packet);
            p_packet = NULL;
        } else if (queue == mpDeviceSecondVideoDataQueue) {
            pdata = p_packet->data_ptr;
            skip_size = __skip_delimter_size(pdata);
            pdata += skip_size;
            data_size = p_packet->data_size - skip_size;

            extra_flag = 0;
            sync_video = 0;
            if (DUnlikely(AM_EXPORT_VIDEO_FRAME_TYPE_IDR == p_packet->frame_type)) {
                extra_flag |= DSACPHeaderFlagBit_PacketKeyFrameIndicator;
                if (DUnlikely(!video_extradata_send)) {
                    if ((!mpSecondVideoExtraData) || (!mSecondVideoExtraDataSize)) {
                        TU8 *p_extradata = NULL;
                        TU32 extradata_size = 0;
                        err = gfGetH264Extradata(pdata, data_size, p_extradata, extradata_size);
                        if (DUnlikely(EECode_OK != err)) {
                            LOG_FATAL("can not find extra data, corrupted h264 data?\n");
                            gfPrintMemory(pdata, 64);
                            releaseDataPacket((void *) p_packet);
                            p_packet = NULL;
                            continue;
                        }
                        mSecondVideoExtraDataSize = extradata_size;
                        mpSecondVideoExtraData = (TU8 *) DDBG_MALLOC(mSecondVideoExtraDataSize + 16, "CVED");
                        if (DUnlikely(!mpSecondVideoExtraData)) {
                            LOG_FATAL("no memory, request size %ld\n", (mSecondVideoExtraDataSize + 16));
                            releaseDataPacket((void *) p_packet);
                            p_packet = NULL;
                            continue;
                        }
                        memcpy(mpSecondVideoExtraData, p_extradata, mSecondVideoExtraDataSize);
                    }

                    LOG_NOTICE("video extra size (second) %d\n", (TU32)mSecondVideoExtraDataSize);
                    gfPrintMemory(mpSecondVideoExtraData, (TU32)mSecondVideoExtraDataSize);

                    err = mpCloudClientAgentV2SecStream->SetupVideoParams(mSecondVideoFormat, ((mSecondVideoFramerateDen) << 8) | (mSecondVideoFramerate & 0xff), mSecondVideoResWidth, mSecondVideoResHeight, mSecondVideoBitrate, mpSecondVideoExtraData, (TU16)mSecondVideoExtraDataSize, DBuildGOP(mSecondVideoGOPM, mSecondVideoGOPN, mSecondVideoGOPIDRInterval));
                    if (DUnlikely(EECode_OK != err)) {
                        LOG_ERROR("setVideoParams() failed, err = %d, %s\n", err, gfGetErrorCodeString(err));
                        cloudAgentDisconnect();
                        connected = 0;
                        audio_extradata_send = 0;
                        video_extradata_send = 0;
                        sec_video_extradata_send = 0;
                        releaseDataPacket((void *) p_packet);
                        p_packet = NULL;
                        usleep(DRetryIntervalUS);
                        continue;
                    }
                    sec_video_extradata_send = 1;
                }
                sec_kr_count ++;
                if (DUnlikely(!(sec_kr_count & 0xf))) {
                    sec_sync_video = 1;
                }
            } else if (DUnlikely(sec_discard_non_idr)) {
                LOG_WARN("skip non-idr data, type %d\n", p_packet->frame_type);
                releaseDataPacket((void *) p_packet);
                p_packet = NULL;
                continue;
            }

            if (((0x01 == pdata[2]) && (0x07 == (0x1f & pdata[3]))) || ((0x01 == pdata[3]) && (0x07 == (0x1f & pdata[4])))) {
                TU8 nal_type = 0;
                TU8 *ptmp = gfNALUFindFirstAVCSliceHeaderType(pdata, data_size, nal_type);
                if (ptmp) {
                    if ((nal_type & 0x1f) != 0x05) {
                        LOG_WARN("skip sps pps before non-IDR frame\n");
                        data_size -= (TU32)(ptmp - pdata);
                        pdata = ptmp;
                        extra_flag = 0;
                    } else {
                        extra_flag |= DSACPHeaderFlagBit_PacketExtraDataIndicator;
                    }
                } else {
                    LOG_FATAL("data corruption\n");
                }
            }

            TU64 seq_num = p_packet->seq_num;
            TU64 cur_pts = p_packet->pts;

#ifdef D_PRINT_AVSYNC
            if (mDebugPrintPTSSeqNum) {
                printf("    upload h264(%d, %lld)\n", (TU32)seq_num, cur_pts);
            }
#endif

            if (DUnlikely(!mbSecondstreamVideoUploadingStarted)) {
                LOG_NOTICE("[pts]: video (second) upload begin pts %lld\n", cur_pts);
                mpCloudClientAgentV2SecStream->VideoSync((TTime)cur_pts, seq_num);
                mbSecondstreamVideoUploadingStarted = 1;
                err = uploadH264streamSecondStream(pdata, data_size, (TU32)seq_num, extra_flag);
                if (DUnlikely(EECode_OK != err)) {
                    LOG_ERROR("uploadH264streamSecondStream() failed, err = %d, %s\n", err, gfGetErrorCodeString(err));
                    cloudAgentDisconnect();
                    connected = 0;
                    audio_extradata_send = 0;
                    video_extradata_send = 0;
                    sec_video_extradata_send = 0;
                    releaseDataPacket((void *) p_packet);
                    p_packet = NULL;
                    usleep(DRetryIntervalUS);
                    continue;
                }
            } else {
                err = uploadH264streamSecondStream(pdata, data_size, (TU32)seq_num, extra_flag);
                if (DUnlikely(EECode_OK != err)) {
                    LOG_ERROR("uploadH264streamSecondStream() failed, err = %d, %s\n", err, gfGetErrorCodeString(err));
                    cloudAgentDisconnect();
                    connected = 0;
                    audio_extradata_send = 0;
                    video_extradata_send = 0;
                    releaseDataPacket((void *) p_packet);
                    p_packet = NULL;
                    usleep(DRetryIntervalUS);
                    continue;
                }
                if (DUnlikely(sec_sync_video)) {
#ifdef D_PRINT_AVSYNC
                    if (mDebugPrintPTSSeqNum) {
                        printf("[sync video, sec]: (%d, %lld)\n", (TU32)seq_num, cur_pts);
                    }
#endif
                    mpCloudClientAgentV2SecStream->VideoSync((TTime)cur_pts, seq_num);
                }
            }
            sec_discard_non_idr = 0;
            releaseDataPacket((void *) p_packet);
            p_packet = NULL;
        } else {
            //LOG_INFO("skip non h264/aac type %d\n", p_header->dataType);
            LOG_FATAL("must not comes here\n");
            releaseDataPacket((void *) p_packet);
            p_packet = NULL;
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

        if (mbEnableSecondStream) {
            mpCloudClientAgentV2SecStream = gfCreateCloudClientAgentV2(EProtocolType_SACP);
            if (DUnlikely(!mpCloudClientAgentV2SecStream)) {
                LOG_FATAL("gfCreateCloudClientAgentV2() fail\n");
                return EECode_NoMemory;
            }
            mpCloudClientAgentV2SecStream->SetHardwareAuthenticateCallback(__clientAgentAuthenticateCallBack);
        }
    } else {
        LOG_WARN("already created\n");
    }
    return EECode_OK;
}

EECode CAmbaCloudAgent::cloudAgentConnectServer()
{
    if (DLikely(mpCloudClientAgentV2)) {
        EECode err = EECode_OK;
        TU32 need_send_discontinous_indicator = 0;
        if (!mbApplicationStarted) {
            need_send_discontinous_indicator = 1;
        }
        err = mpCloudClientAgentV2->ConnectToServer(mpCloudServerUrl, mCloudServerPort, mDeviceAccountName, mDeviceDynamicPassword);
        if (EECode_OK == err) {
            if (DUnlikely(need_send_discontinous_indicator)) {
                err = mpCloudClientAgentV2->DiscontiousIndicator();
                if (DLikely(EECode_OK != err)) {
                    LOG_ERROR("DiscontiousIndicator fail! ret %d, %s\n", err, gfGetErrorCodeString(err));
                    return err;
                }
            }
        } else {
            LOG_WARN("connect data server(%s, %d) fail, name %s, dynamic password %s, ret %d %s\n", mpCloudServerUrl, mCloudServerPort, mDeviceAccountName, mDeviceDynamicPassword, err, gfGetErrorCodeString(err));
            return err;
        }

        if (mbEnableSecondStream && mpCloudClientAgentV2SecStream) {
            TChar second_stream_name[DMAX_ACCOUNT_NAME_LENGTH_EX] = {0};
            snprintf(second_stream_name, sizeof(second_stream_name), "%s_sec", mDeviceAccountName);

            err = mpCloudClientAgentV2SecStream->ConnectToServer(mpCloudServerUrl, mCloudServerPort, second_stream_name, mDeviceDynamicPassword);
            if (EECode_OK == err) {
                if (DUnlikely(need_send_discontinous_indicator)) {
                    err = mpCloudClientAgentV2SecStream->DiscontiousIndicator();
                    if (DLikely(EECode_OK != err)) {
                        LOG_ERROR("DiscontiousIndicator fail! ret %d, %s\n", err, gfGetErrorCodeString(err));
                        return err;
                    }
                }
                mbSecondStreamAgentConnected = 1;
            } else {
                LOG_WARN("connect data server(%s, %d) fail, name %s, dynamic password %s, ret %d %s\n", mpCloudServerUrl, mCloudServerPort, second_stream_name, mDeviceDynamicPassword, err, gfGetErrorCodeString(err));
                return err;
            }
        }
        mbApplicationStarted = 1;
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

    if (mbEnableSecondStream) {
        if (DLikely(mpCloudClientAgentV2SecStream)) {
            mpCloudClientAgentV2SecStream->DisconnectToServer();
        } else {
            LOG_FATAL("NULL mpCloudClientAgentV2SecStream\n");
        }
        mbSecondStreamAgentConnected = 0;
    }
}

EECode CAmbaCloudAgent::uploadH264stream(TU8 *pdata, TMemSize data_size, TU32 seq_num, TU32 keyframe)
{
    if (DLikely(mpCloudClientAgentV2)) {
#ifdef D_ENABLE_ENCRYPTION
        if (keyframe) {
            mbStartedVideoEncryption = mbEnableVideoEncryption;
        }

        if (mpDataCypher && mbStartedVideoEncryption) {
            //LOG_PRINTF("upload h264, encryption, size %d, seq %d\n", data_size, seq_num);
            return uploadEncryptedH264stream(pdata, data_size, seq_num, keyframe);
        }
#endif
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

EECode CAmbaCloudAgent::uploadH264streamSecondStream(TU8 *pdata, TMemSize data_size, TU32 seq_num, TU32 keyframe)
{
    if (DLikely(mpCloudClientAgentV2SecStream)) {
#ifdef D_ENABLE_ENCRYPTION
        if (keyframe) {
            mbStartedVideoEncryptionSecondStream = mbEnableVideoEncryption;
        }

        if (mpDataCypher && mbStartedVideoEncryptionSecondStream) {
            return uploadSecondEncryptedH264stream(pdata, data_size, seq_num, keyframe);
        }
#endif
        if (DUnlikely(keyframe)) {
            return mpCloudClientAgentV2SecStream->Uploading(pdata, data_size, ESACPDataChannelSubType_H264_NALU, seq_num, DSACPHeaderFlagBit_PacketKeyFrameIndicator);
        } else {
            return mpCloudClientAgentV2SecStream->Uploading(pdata, data_size, ESACPDataChannelSubType_H264_NALU, seq_num, 0);
        }
    } else {
        LOG_FATAL("NULL mpCloudClientAgentV2SecStream\n");
        return EECode_InternalLogicalBug;
    }
}

#ifdef D_ENABLE_ENCRYPTION
EECode CAmbaCloudAgent::encryptData(s_symmetric_cypher *cypher, TU8 *pdata, TMemSize data_size)
{
    if (DLikely(cypher)) {
        int outsize = 0;
        int processed_size = 0;
        int pre_size = 0;
        int remain_size = 0;
        unsigned char *p_in = pdata;
        unsigned char *p_out = mpCypherOutputBuffer;

        unsigned char *ptmp = gfNALUFindFirstAVCSliceHeader(pdata, data_size);
        if (ptmp) {
            pre_size = (int)(ptmp + 8 - pdata);
            if ((pre_size + cypher->block_length_in_bytes) <= data_size) {
            } else {
                return EECode_OK_SKIP;
            }
        } else {
            LOG_ERROR("might have data corruption\n");
            return EECode_OK_SKIP;
        }

        memcpy(p_out, p_in, pre_size);
        processed_size += pre_size;
        p_out += pre_size;
        p_in += pre_size;

        remain_size = data_size - pre_size;
        remain_size &= cypher->block_length_mask;

        while (remain_size >= 1024) {
            outsize = symmetric_cryption(cypher, p_in, 1024, p_out, 1);
            if (outsize != 1024) {
                LOG_ERROR("why in out size not match\n");
                break;
            }
            p_in += 1024;
            p_out += 1024;
            processed_size += 1024;
            remain_size -= 1024;
        }

        if (remain_size) {
            outsize = symmetric_cryption(cypher, p_in, remain_size, p_out, 1);
            if (outsize != remain_size) {
                LOG_ERROR("why in out size not match\n");
            }
            p_in += remain_size;
            p_out += remain_size;
            processed_size += remain_size;
        }

        if (data_size > processed_size) {
            memcpy(p_out, p_in, data_size - processed_size);
        }

    } else {
        LOG_FATAL("NULL cypher\n");
        return EECode_OK_SKIP;
    }

    return EECode_OK;
}

int CAmbaCloudAgent::resetCypher(s_symmetric_cypher *cypher)
{
    if (DLikely(cypher)) {
        int ret = 0;

        //LOG_NOTICE("[reset cypher], state %d\n", cypher->state);
        if (cypher->state) {
            end_symmetric_cryption(cypher);
        }

        ret = set_symmetric_key_iv(cypher, mDataCypherKey, mDataCypherIV);
        if (ret) {
            destroy_symmetric_cypher(cypher);
            LOG_FATAL("set_symmetric_key_iv fail, disable encryption\n");
            return (-1);
        }

        ret = begin_symmetric_cryption(cypher, 1);
        if (ret) {
            destroy_symmetric_cypher(cypher);
            LOG_FATAL("begin_symmetric_cryption fail, disable encryption\n");
            return (-1);
        }
    } else {
        LOG_WARN("NULL cypher\n");
    }
    return 0;
}

EECode CAmbaCloudAgent::uploadEncryptedH264stream(TU8 *pdata, TMemSize data_size, TU32 seq_num, TU32 keyframe)
{
    if (DLikely(mpCloudClientAgentV2)) {
        if (DUnlikely(keyframe)) {
            int ret = resetCypher(mpDataCypher);
            if (ret) {
                mpDataCypher = NULL;
            }
        }
        if (DUnlikely(keyframe)) {
            if (mpDataCypher) {
                if (EECode_OK == encryptData(mpDataCypher, pdata, data_size)) {
                    return mpCloudClientAgentV2->Uploading(mpCypherOutputBuffer, data_size, ESACPDataChannelSubType_H264_NALU, seq_num, DSACPHeaderFlagBit_PacketKeyFrameIndicator);
                }
            }
            return mpCloudClientAgentV2->Uploading(pdata, data_size, ESACPDataChannelSubType_H264_NALU, seq_num, DSACPHeaderFlagBit_PacketKeyFrameIndicator);
        } else {
            if (mpDataCypher) {
                if (EECode_OK == encryptData(mpDataCypher, pdata, data_size)) {
                    return mpCloudClientAgentV2->Uploading(mpCypherOutputBuffer, data_size, ESACPDataChannelSubType_H264_NALU, seq_num, 0);
                }
            }
            return mpCloudClientAgentV2->Uploading(pdata, data_size, ESACPDataChannelSubType_H264_NALU, seq_num, 0);
        }
    } else {
        LOG_FATAL("NULL mpCloudClientAgentV2\n");
        return EECode_InternalLogicalBug;
    }
}

EECode CAmbaCloudAgent::uploadSecondEncryptedH264stream(TU8 *pdata, TMemSize data_size, TU32 seq_num, TU32 keyframe)
{
    if (DLikely(mpCloudClientAgentV2SecStream)) {
        if (DUnlikely(keyframe)) {
            int ret = resetCypher(mpDataCypherSecondStream);
            if (ret) {
                mpDataCypherSecondStream = NULL;
            }
        }
        if (DUnlikely(keyframe)) {
            if (mpDataCypherSecondStream) {
                if (EECode_OK == encryptData(mpDataCypherSecondStream, pdata, data_size)) {
                    return mpCloudClientAgentV2SecStream->Uploading(mpCypherOutputBuffer, data_size, ESACPDataChannelSubType_H264_NALU, seq_num, DSACPHeaderFlagBit_PacketKeyFrameIndicator);
                }
            }
            return mpCloudClientAgentV2SecStream->Uploading(pdata, data_size, ESACPDataChannelSubType_H264_NALU, seq_num, DSACPHeaderFlagBit_PacketKeyFrameIndicator);
        } else {
            if (mpDataCypher) {
                if (EECode_OK == encryptData(mpDataCypherSecondStream, pdata, data_size)) {
                    return mpCloudClientAgentV2SecStream->Uploading(mpCypherOutputBuffer, data_size, ESACPDataChannelSubType_H264_NALU, seq_num, 0);
                }
            }
            return mpCloudClientAgentV2SecStream->Uploading(pdata, data_size, ESACPDataChannelSubType_H264_NALU, seq_num, 0);
        }
    } else {
        LOG_FATAL("NULL mpCloudClientAgentV2SecStream\n");
        return EECode_InternalLogicalBug;
    }
}
#endif

EECode CAmbaCloudAgent::uploadAACstream(TU8 *pdata, TMemSize data_size, TU32 seq_num)
{
    if (DLikely(mpCloudClientAgentV2)) {
        return mpCloudClientAgentV2->Uploading(pdata, data_size, ESACPDataChannelSubType_AAC, seq_num, 0);
    } else {
        LOG_FATAL("NULL mpCloudClientAgentV2\n");
        return EECode_InternalLogicalBug;
    }
}

EECode CAmbaCloudAgent::uploadAACstreamSecondStream(TU8 *pdata, TMemSize data_size, TU32 seq_num)
{
    if (DLikely(mpCloudClientAgentV2SecStream)) {
        return mpCloudClientAgentV2SecStream->Uploading(pdata, data_size, ESACPDataChannelSubType_AAC, seq_num, 0);
    } else {
        LOG_FATAL("NULL mpCloudClientAgentV2SecStream\n");
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
    p_config_file->Destroy();

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

    p_config_file->Destroy();

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

AMExportPacket *CAmbaCloudAgent::allocateDataPacket()
{
    CIDoubleLinkedList::SNode *node = NULL;
    AMExportPacket *packet = NULL;

    pthread_mutex_lock(&mPacketMutex);
    node = mpFreePacketList->FirstNode();
    if (node) {
        packet = (AMExportPacket *)(node->p_context);
        mpFreePacketList->FastRemoveContent(node, node->p_context);
        pthread_mutex_unlock(&mPacketMutex);
        memset(packet, 0x0, sizeof(AMExportPacket));
        return packet;
    }

    packet = (AMExportPacket *) malloc(sizeof(AMExportPacket));
    mTotalPacketCount ++;
    //LOG_NOTICE("alloc new data packet %d\n", mTotalPacketCount);
    if (packet) {
        memset(packet, 0x0, sizeof(AMExportPacket));
    } else {
        LOG_FATAL("no memory\n");
    }
    pthread_mutex_unlock(&mPacketMutex);

    return packet;
}

void CAmbaCloudAgent::releaseDataPacket(void *p)
{
    AMExportPacket *packet = (AMExportPacket *) p;
    if (packet->user_alloc_memory) {
        free((void *) packet->data_ptr);
    } else {
        if (mpDataExportClient) {
            mpDataExportClient->release(packet);
        }
    }

    packet->data_ptr = 0;
    packet->data_size = 0;

    pthread_mutex_lock(&mPacketMutex);
    mpFreePacketList->InsertContent(NULL, p, 0);
    pthread_mutex_unlock(&mPacketMutex);
}

EECode CAmbaCloudAgent::allocateMemoryInPacket(AMExportPacket *packet, TU32 memsize)
{
    DASSERT(packet);
    packet->data_ptr = (unsigned char *) malloc(memsize);
    if (DLikely(packet->data_ptr)) {
        return EECode_OK;
    }

    LOG_FATAL("no memory, request size %ld\n", memsize);
    return EECode_NoMemory;
}

TInt CAmbaCloudAgent::checkVideoQueue(SAgentMsgQueue *q)
{
    TInt skip_number = 0;

    pthread_mutex_lock(&q->p_mutex_cond->mutex);
    if (DUnlikely(q->num_msgs > (DDeviceMaxMsgs - 4))) {
        printf("!!!!start to flush, video, %d\n", q->num_msgs);
        while (1) {
            if (q->num_msgs > 0) {

                if ((skip_number > (DDeviceMaxMsgs / 2)) && (q->msgs[q->read_index].flags & DDeviceSyncPointFlag)) {
                    pthread_mutex_unlock(&q->p_mutex_cond->mutex);
                    return 1;
                }

                releaseDataPacket(q->msgs[q->read_index].ptr);

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

    pthread_mutex_unlock(&q->p_mutex_cond->mutex);
    LOG_FATAL("why comes here\n");
    //should not comes here
    return -3;
}

void CAmbaCloudAgent::checkAudioQueue(SAgentMsgQueue *q)
{
    pthread_mutex_lock(&q->p_mutex_cond->mutex);
    if (DUnlikely(q->num_msgs > (DDeviceMaxMsgs - 12))) {
        printf("!!!!start to flush, audio, %d\n", q->num_msgs);
        while (1) {
            if (q->num_msgs > (DDeviceMaxMsgs - 32)) {
                releaseDataPacket(q->msgs[q->read_index].ptr);
                if (++q->read_index == DDeviceMaxMsgs) {
                    q->read_index = 0;
                }
                q->num_msgs--;
            } else {
                break;
            }
        }
    }
    pthread_mutex_unlock(&q->p_mutex_cond->mutex);
}


DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

