/*******************************************************************************
 * directsharing_receiver.cpp
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

#include "directsharing_receiver.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

enum {
    EDirectSharingReceiverState_Idle = 0x00,
    EDirectSharingReceiverState_ReceiveHeader = 0x01,
    EDirectSharingReceiverState_ReceivePayload = 0x02,
    EDirectSharingReceiverState_Error = 0x03,
    EDirectSharingReceiverState_Pending = 0x04,
};

IDirectSharingReceiver *gfCreateDirectSharingReceiver(TSocketPort port, const volatile SPersistCommonConfig *pconfig, IMsgSink *pMsgSink, const CIClockReference *p_system_clock_reference)
{
    return (IDirectSharingReceiver *)CIDirectSharingReceiver::Create(port, pconfig, pMsgSink, p_system_clock_reference);
}

CIDirectSharingReceiver::CIDirectSharingReceiver(TSocketHandler socket, const volatile SPersistCommonConfig *pconfig, IMsgSink *pMsgSink, const CIClockReference *p_system_clock_reference)
    : CObject("CIDirectSharingReceiver", 0)
    , mpPersistCommonConfig(pconfig)
    , mpMsgSink(pMsgSink)
    , mpSystemClockReference(p_system_clock_reference)
    , mbRun(1)
    , msState(EDirectSharingReceiverState_Idle)
    , mpCallbackContext(NULL)
    , mfRequestMemoryCallback(NULL)
    , mfReceiveCallback(NULL)
    , mSocket(socket)
{
}

CIDirectSharingReceiver::~CIDirectSharingReceiver()
{
}

CIDirectSharingReceiver *CIDirectSharingReceiver::Create(TSocketHandler socket, const volatile SPersistCommonConfig *pconfig, IMsgSink *pMsgSink, const CIClockReference *p_system_clock_reference)
{
    CIDirectSharingReceiver *result = new CIDirectSharingReceiver(socket, pconfig, pMsgSink, p_system_clock_reference);
    if (result && result->Construct() != EECode_OK) {
        delete result;
        result = NULL;
    }
    return result;
}

CObject *CIDirectSharingReceiver::GetObject() const
{
    return (CObject *) this;
}

EECode CIDirectSharingReceiver::Construct()
{
    DSET_MODULE_LOG_CONFIG(ELogModuleDirectSharingServer);
    //mConfigLogLevel = ELogLevel_Debug;
    //mConfigLogOption = 0xffffffff;

    mpWorkQueue = CIWorkQueue::Create((IActiveObject *)this);
    if (DUnlikely(!mpWorkQueue)) {
        LOGM_ERROR("Create CIWorkQueue fail.\n");
        return EECode_NoMemory;
    }

    EECode err = gfOSCreatePipe(mPipeFd);
    DASSERT(EECode_OK == err);

    //init
    FD_ZERO(&mAllSet);
    FD_SET(mPipeFd[0], &mAllSet);
    FD_SET(mSocket, &mAllSet);

    if (mSocket > mPipeFd[0]) {
        mMaxFd = mSocket;
    } else {
        mMaxFd = mPipeFd[0];
    }

    LOGM_NOTICE("before Run().\n");
    DASSERT(mpWorkQueue);
    mpWorkQueue->Run();
    LOGM_NOTICE("after Run().\n");

    return EECode_OK;
}

void CIDirectSharingReceiver::Destroy()
{
    Delete();
}
void CIDirectSharingReceiver::Delete()
{
    if (DIsSocketHandlerValid(mSocket)) {
        gfNetCloseSocket(mSocket);
        mSocket = DInvalidSocketHandler;
    }
}

void CIDirectSharingReceiver::OnRun()
{
    SCMD cmd;
    TInt nready = 0;

    SDirectSharingHeader header;
    TInt total_length = 0;
    TInt remain_length = 0;
    TInt read_len = 0;
    TU8 *p_base = NULL;
    TU8 *p_cur = NULL;

    TU8 payload_flag = 0;

    //LOGM_NOTICE("CIDirectSharingReceiver::OnRun, before cmdack\n");
    mpWorkQueue->CmdAck(EECode_OK);
    //LOGM_NOTICE("CIDirectSharingReceiver::OnRun, after cmdack\n");

    total_length = remain_length = sizeof(header);
    p_cur = p_base = (TU8 *) &header;

    while (mbRun) {
        //LOGM_STATE("receiver state %d\n", msState);
        switch (msState) {

            case EDirectSharingReceiverState_Pending:
            case EDirectSharingReceiverState_Error:
            case EDirectSharingReceiverState_Idle:
                mReadSet = mAllSet;

                nready = select(mMaxFd + 1, &mReadSet, NULL, NULL, NULL);
                if (DUnlikely(0 > nready)) {
                    LOGM_FATAL("select fail, nready %d\n", nready);
                    mbRun = 0;
                    break;
                } else if (nready == 0) {
                    continue;
                }

                if (FD_ISSET(mPipeFd[0], &mReadSet)) {
                    mpWorkQueue->WaitMsg(&cmd, sizeof(cmd));
                    processCmd(cmd);
                    nready --;
                    if (nready <= 0) {
                        break;
                    }
                }
                break;

            case EDirectSharingReceiverState_ReceiveHeader:
                mReadSet = mAllSet;
                //LOGM_NOTICE("before select 1\n");
                nready = select(mMaxFd + 1, &mReadSet, NULL, NULL, NULL);
                if (DUnlikely(0 > nready)) {
                    LOGM_FATAL("select fail\n");
                    mbRun = 0;
                    break;
                } else if (nready == 0) {
                    break;
                }

                if (FD_ISSET(mPipeFd[0], &mReadSet)) {
                    mpWorkQueue->WaitMsg(&cmd, sizeof(cmd));
                    processCmd(cmd);
                    nready --;
                    if (nready <= 0) {
                        break;
                    }
                }

                if (FD_ISSET(mSocket, &mReadSet)) {
                    read_len = gfNet_Recv(mSocket, p_cur, remain_length, 0);
                    if (DUnlikely(remain_length != read_len)) {
                        if (0 < read_len) {
                            p_cur += read_len;
                            remain_length -= read_len;
                        } else {
                            LOG_ERROR("gfNet_Recv error, ret %d\n", read_len);
                            msState = EDirectSharingReceiverState_Error;
                        }
                        break;
                    } else {
                        if (header.header_flag & DDirectSharingHeaderFlagIsReply) {
                            LOGM_WARN("read reply here?\n");
                            total_length = remain_length = sizeof(header);
                            p_cur = p_base = (TU8 *)&header;
                            break;
                        }
                        if ((EDirectSharingHeaderType_EOS == header.header_type) || (DDirectSharingPayloadFlagEOS & header.payload_flag)) {
                            LOG_NOTICE("file recieve finished!\n");
                            mfReceiveCallback(mpCallbackContext, NULL, 0, EECode_OK_EOF, header.payload_flag);
                            FD_CLR(mSocket, &mAllSet);
                            msState = EDirectSharingReceiverState_Pending;
                            break;
                        }
                        payload_flag = header.payload_flag;
                        total_length = remain_length = (header.payload_length_0 << 16) | (header.payload_length_1 << 8) | (header.payload_length_2);
                        if (!total_length) {
                            LOG_FATAL("0 length?\n");
                            gfPrintMemory((TU8 *)&header, sizeof(header));
                        }
                        mfRequestMemoryCallback(mpCallbackContext, p_base, total_length);
                        p_cur = p_base;
                        msState = EDirectSharingReceiverState_ReceivePayload;
                    }
                }
                break;

            case EDirectSharingReceiverState_ReceivePayload:
                mReadSet = mAllSet;
                //LOGM_NOTICE("before select 2\n");
                nready = select(mMaxFd + 1, &mReadSet, NULL, NULL, NULL);
                if (DUnlikely(0 > nready)) {
                    LOGM_FATAL("select fail\n");
                    mbRun = 0;
                    break;
                } else if (nready == 0) {
                    break;
                }

                if (FD_ISSET(mPipeFd[0], &mReadSet)) {
                    mpWorkQueue->WaitMsg(&cmd, sizeof(cmd));
                    processCmd(cmd);
                    nready --;
                    if (nready <= 0) {
                        break;
                    }
                }

                if (FD_ISSET(mSocket, &mReadSet)) {
                    read_len = gfNet_Recv(mSocket, p_cur, remain_length, 0);
                    if (DUnlikely(remain_length != read_len)) {
                        if (0 < read_len) {
                            p_cur += read_len;
                            remain_length -= read_len;
                        } else {
                            LOG_ERROR("gfNet_Recv error, ret %d\n", read_len);
                            msState = EDirectSharingReceiverState_Error;
                        }
                        break;
                    } else {
                        mfReceiveCallback(mpCallbackContext, p_base, total_length, EECode_OK, payload_flag);

                        total_length = remain_length = sizeof(header);
                        p_cur = p_base = (TU8 *)&header;
                        msState = EDirectSharingReceiverState_ReceiveHeader;
                    }
                }
                break;

            default:
                LOG_FATAL("BAD state %d\n", msState);
                msState = EDirectSharingReceiverState_Error;
                break;
        }
        mDebugHeartBeat ++;
    }

}

void CIDirectSharingReceiver::PrintStatus()
{
}

void CIDirectSharingReceiver::SetCallBack(void *owner, TRequestMemoryCallBack request_memory_callback, TDataReceiveCallBack receive_callback)
{
    mpCallbackContext = owner;
    mfRequestMemoryCallback = request_memory_callback;
    mfReceiveCallback = receive_callback;
}

EECode CIDirectSharingReceiver::Start()
{
    EECode err;
    TChar wake_char = 'a';

    DASSERT(mpWorkQueue);
    LOGM_NOTICE("Start() begin.\n");

    gfOSWritePipe(mPipeFd[1], wake_char);
    err = mpWorkQueue->SendCmd(ECMDType_Start, NULL);
    LOGM_NOTICE("Start() end, ret %d.\n", err);
    return err;
}

EECode CIDirectSharingReceiver::Stop()
{
    EECode err;
    TChar wake_char = 'b';

    DASSERT(mpWorkQueue);
    LOGM_NOTICE("Stop() begin.\n");

    gfOSWritePipe(mPipeFd[1], wake_char);
    err = mpWorkQueue->SendCmd(ECMDType_Stop, NULL);
    LOGM_NOTICE("Stop() end, ret %d.\n", err);
    return err;
}

void CIDirectSharingReceiver::processCmd(SCMD &cmd)
{
    TChar char_buffer;

    gfOSReadPipe(mPipeFd[0], char_buffer);

    switch (cmd.code) {

        case ECMDType_Start:
            msState = EDirectSharingReceiverState_ReceiveHeader;
            mpWorkQueue->CmdAck(EECode_OK);
            break;

        case ECMDType_Stop:
            mbRun = 0;
            msState = EDirectSharingReceiverState_Pending;
            mpWorkQueue->CmdAck(EECode_OK);
            break;

        default:
            LOGM_ERROR("wrong cmd.code: %d", cmd.code);
            break;
    }

    //LOGM_FLOW("****CIDirectSharingServer::ProcessCmd, cmd.code %d end.\n", cmd.code);
    return;
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END


