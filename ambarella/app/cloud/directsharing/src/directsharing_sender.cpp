/*******************************************************************************
 * directsharing_sender.cpp
 *
 * History:
 *    2015/03/10 - [Zhi He] create file
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

#include "directsharing_sender.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

IDirectSharingSender *gfCreateDirectSharingSender(TSocketHandler socket, const volatile SPersistCommonConfig *pconfig, IMsgSink *pMsgSink, const CIClockReference *p_system_clock_reference)
{
    return (IDirectSharingSender *)CIDirectSharingSender::Create(socket, pconfig, pMsgSink, p_system_clock_reference);
}

CIDirectSharingSender::CIDirectSharingSender(TSocketHandler socket, const volatile SPersistCommonConfig *pconfig, IMsgSink *pMsgSink, const CIClockReference *p_system_clock_reference)
    : mpPersistCommonConfig(pconfig)
    , mpSystemClockReference(p_system_clock_reference)
    , mpMsgSink(pMsgSink)
    , mSocket(socket)
    , mbWaitFirstKeyFrame(1)
{
}

CIDirectSharingSender::~CIDirectSharingSender()
{
}

CIDirectSharingSender *CIDirectSharingSender::Create(TSocketHandler socket, const volatile SPersistCommonConfig *pconfig, IMsgSink *pMsgSink, const CIClockReference *p_system_clock_reference)
{
    CIDirectSharingSender *result = new CIDirectSharingSender(socket, pconfig, pMsgSink, p_system_clock_reference);
    if (result && result->Construct() != EECode_OK) {
        delete result;
        result = NULL;
    }
    return result;
}

EECode CIDirectSharingSender::Construct()
{
    return EECode_OK;
}

void CIDirectSharingSender::Destroy()
{
    if (DIsSocketHandlerValid(mSocket)) {
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
    }
}

EDirectSharingStatusCode CIDirectSharingSender::SendData(TU8 *pdata, TU32 datasize, TU8 data_flag)
{
    if (mbWaitFirstKeyFrame) {
        if ((data_flag & DDirectSharingPayloadFlagKeyFrameIndicator)) {
            mbWaitFirstKeyFrame = 0;
            gfPrintMemory(pdata, 256);
        } else {
            LOG_NOTICE("skip before first key frame\n");
            return EDirectSharingStatusCode_OK;
        }
    }
    SDirectSharingHeader header;

    header.header_type = EDirectSharingHeaderType_Data;
    header.header_flag = 0;
    header.val0 = 0;
    header.val1 = 0;

    header.payload_flag = data_flag;
    header.payload_length_0 = (datasize >> 16) & 0xff;
    header.payload_length_1 = (datasize >> 8) & 0xff;
    header.payload_length_2 = datasize & 0xff;

    //LOG_INFO("send data, %p, size %d, flag %02x ...\n", pdata, datasize, data_flag);

    TInt ret = gfNet_Send_timeout(mSocket, (TU8 *)&header, (TInt)sizeof(SDirectSharingHeader), 0, 4);
    if (DUnlikely(ret != (TInt)(sizeof(SDirectSharingHeader)))) {
        LOG_ERROR("send header fail\n");
        return EDirectSharingStatusCode_SendHeaderFail;
    }

    TU32 remain_datasize = datasize;
    TU32 cur_send_size = 0;
    TU8 *cur_ptr = pdata;
    while (remain_datasize) {
        if (remain_datasize > 1280) {
            cur_send_size = 1280;
        } else {
            cur_send_size = remain_datasize;
        }

        ret = gfNet_Send_timeout(mSocket, cur_ptr, cur_send_size, 0, 6);
        if (DUnlikely(ret != (TInt)cur_send_size)) {
            LOG_ERROR("send payload fail\n");
            return EDirectSharingStatusCode_SendPayloadFail;
        }

        cur_ptr += cur_send_size;
        remain_datasize -= cur_send_size;
    }

    //LOG_INFO("send data, %p, size %d, flag %02x, done\n", pdata, datasize, data_flag);

    return EDirectSharingStatusCode_OK;
}

EDirectSharingStatusCode CIDirectSharingSender::StartSendFile(TChar *filename, TFileTransferFinishCallBack callback, void *callback_context)
{
    LOG_FATAL("not supported\n");
    return EDirectSharingStatusCode_NotSuppopted;
}

void CIDirectSharingSender::SetProgressCallBack(void *owner, TTransferUpdateProgressCallBack progress_callback)
{
    LOG_FATAL("not supported\n");
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END


