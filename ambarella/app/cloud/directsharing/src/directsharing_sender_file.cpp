/*******************************************************************************
 * directsharing_sender_file.cpp
 *
 * History:
 *    2015/03/24 - [Zhi He] create file
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

#include "common_config.h"

#include "common_types.h"
#include "common_osal.h"
#include "common_utils.h"

#include "common_log.h"

#include "common_base.h"

#include "common_network_utils.h"

#include "directsharing_if.h"

#include "directsharing_sender_file.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

static EECode __thread_proc(void *p)
{
    CIDirectSharingSenderFile *thiz = (CIDirectSharingSenderFile *) p;
    if (thiz) {
        thiz->MainLoop();
    }

    return EECode_OK;
}

IDirectSharingSender *gfCreateDirectSharingSenderFile(TSocketHandler socket, const volatile SPersistCommonConfig *pconfig, IMsgSink *pMsgSink, const CIClockReference *p_system_clock_reference)
{
    return (IDirectSharingSender *)CIDirectSharingSenderFile::Create(socket, pconfig, pMsgSink, p_system_clock_reference);
}

CIDirectSharingSenderFile::CIDirectSharingSenderFile(TSocketHandler socket, const volatile SPersistCommonConfig *pconfig, IMsgSink *pMsgSink, const CIClockReference *p_system_clock_reference)
    : mpPersistCommonConfig(pconfig)
    , mpSystemClockReference(p_system_clock_reference)
    , mpMsgSink(pMsgSink)
    , mSocket(socket)
    , mpThread(NULL)
    , mpFile(NULL)
    , mpReadBuffer(NULL)
    , mnReadBufferSize(0)
    , mpCallbackContext(NULL)
    , mCallback(NULL)
    , mbRunning(0)
    , mpProgressCallbackContext(NULL)
    , mfProgressCallback(NULL)
    , mnTotalBytes(0)
    , mnSentBytes(0)
    , mnProgress(0)
{
}

CIDirectSharingSenderFile::~CIDirectSharingSenderFile()
{
}

CIDirectSharingSenderFile *CIDirectSharingSenderFile::Create(TSocketHandler socket, const volatile SPersistCommonConfig *pconfig, IMsgSink *pMsgSink, const CIClockReference *p_system_clock_reference)
{
    CIDirectSharingSenderFile *result = new CIDirectSharingSenderFile(socket, pconfig, pMsgSink, p_system_clock_reference);
    if (result && result->Construct() != EECode_OK) {
        delete result;
        result = NULL;
    }
    return result;
}

EECode CIDirectSharingSenderFile::Construct()
{
    return EECode_OK;
}

void CIDirectSharingSenderFile::Destroy()
{
    mbRunning = 0;
    if (mpThread) {
        mpThread->Delete();
        mpThread = NULL;
    }

    if (DIsSocketHandlerValid(mSocket)) {
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
    }

    if (mpReadBuffer) {
        free(mpReadBuffer);
        mpReadBuffer = NULL;
    }

    if (mpFile) {
        fclose(mpFile);
        mpFile = NULL;
    }
}

EDirectSharingStatusCode CIDirectSharingSenderFile::SendData(TU8 *pdata, TU32 datasize, TU8 data_flag)
{
    LOG_FATAL("not supported\n");
    return EDirectSharingStatusCode_NotSuppopted;
}

EDirectSharingStatusCode CIDirectSharingSenderFile::StartSendFile(TChar *filename, TFileTransferFinishCallBack callback, void *callback_context)
{
    if ((!filename) || (!callback) || (!callback_context)) {
        LOG_FATAL("NULL parameters\n");
        return EDirectSharingStatusCode_InvalidArgument;
    }

    mnReadBufferSize = 4 * 1024;
    mpReadBuffer = (TU8 *) malloc(mnReadBufferSize);
    if (!mpReadBuffer) {
        LOG_FATAL("No memory\n");
        return EDirectSharingStatusCode_NoAvailableResource;
    }

    mCallback = callback;
    mpCallbackContext = callback_context;

    mpFile = fopen(filename, "rb");
    if (!mpFile) {
        LOG_FATAL("open file fail\n");
        return EDirectSharingStatusCode_NoAvailableResource;
    }

    fseek(mpFile, 0, SEEK_END);
    mnTotalBytes = ftell(mpFile);
    DASSERT(mnTotalBytes);
    fseek(mpFile, 0, SEEK_SET);

    mnSentBytes = 0;
    mnProgress = 0;

    mbRunning = 1;
    mpThread = gfCreateThread("send file", __thread_proc, (void *) this);
    return EDirectSharingStatusCode_OK;
}

void CIDirectSharingSenderFile::SetProgressCallBack(void *owner, TTransferUpdateProgressCallBack progress_callback)
{
    mpProgressCallbackContext = owner;
    mfProgressCallback = progress_callback;
}

void CIDirectSharingSenderFile::MainLoop()
{
    TInt len = 0, ret = 0;
    SDirectSharingHeader header;

    header.header_type = EDirectSharingHeaderType_Data;
    header.header_flag = 0;
    header.val0 = 0;
    header.val1 = 0;

    header.payload_flag = 0; //DDirectSharingPayloadFlagEOS
    header.payload_length_0 = (mnReadBufferSize >> 16) & 0xff;
    header.payload_length_1 = (mnReadBufferSize >> 8) & 0xff;
    header.payload_length_2 = mnReadBufferSize & 0xff;

    while (mbRunning) {

        len = fread(mpReadBuffer, 1, mnReadBufferSize, mpFile);
        if (len > 0) {
            header.payload_length_0 = (len >> 16) & 0xff;
            header.payload_length_1 = (len >> 8) & 0xff;
            header.payload_length_2 = len & 0xff;
            ret = gfNet_Send_timeout(mSocket, (TU8 *) &header, sizeof(header), 0, 5);
            if (ret != (TInt)(sizeof(header))) {
                LOG_ERROR("send header fail, ret %d\n", ret);
                mCallback(mpCallbackContext, (void *)(this), EECode_Error);
                break;
            }

            ret = gfNet_Send_timeout(mSocket, mpReadBuffer, len, 0, 5);
            if (ret != len) {
                LOG_ERROR("send data fail, ret %d\n", ret);
                mCallback(mpCallbackContext, (void *)(this), EECode_Error);
                break;
            }

            if (mpProgressCallbackContext && mfProgressCallback) {
                mnSentBytes += len;
                TInt new_progress = (TInt)(((TU64) mnSentBytes * 100) / (TU64)mnTotalBytes);
                if (new_progress > mnProgress) {
                    mfProgressCallback(mpProgressCallbackContext, new_progress);
                    mnProgress = new_progress;
                }
            }
        } else {
            LOG_NOTICE("file eos, ret %d\n", ret);
            header.header_type = EDirectSharingHeaderType_Data;
            header.payload_flag = DDirectSharingPayloadFlagEOS;

            header.payload_length_0 = 0;
            header.payload_length_1 = 0;
            header.payload_length_2 = 0;

            ret = gfNet_Send_timeout(mSocket, (TU8 *) &header, sizeof(header), 0, 5);
            if (ret != (TInt)(sizeof(header))) {
                LOG_ERROR("send eos fail, ret %d\n", ret);
                mCallback(mpCallbackContext, (void *)(this), EECode_Error);
            }
            break;
        }
    }
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END


