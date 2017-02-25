/*******************************************************************************
 * common_remote_utils.cpp
 *
 * History:
 *  2014/05/04 - [Zhi He] create file
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

#include <stdarg.h>

#include "common_config.h"

#include "common_types.h"
#include "common_osal.h"
#include "common_utils.h"

#include "common_log.h"

#include "common_base.h"

#include "common_remote_utils.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

CIRemoteLogServer *CIRemoteLogServer::Create()
{
    CIRemoteLogServer *result = new CIRemoteLogServer();
    if (result && result->Construct() != EECode_OK) {
        delete result;
        result = NULL;
    }
    return result;
}

CIRemoteLogServer::CIRemoteLogServer()
    : CObject("CIRemoteLogServer", 0)
    , mpAgent(NULL)
    , mpMutex(NULL)
    , mPort(0)
    , mRemotePort(0)
    , mpTextBuffer(NULL)
    , mTextBufferSize(0)
    , mbCreateContext(0)
    , mbBufferFull(0)
{
}

CIRemoteLogServer::~CIRemoteLogServer()
{
}

EECode CIRemoteLogServer::Construct()
{
    mpAgent = gfIPCAgentFactory(EIPCAgentType_UnixDomainSocket);
    if (DUnlikely(!mpAgent)) {
        LOGM_ERROR("Create mpAgent fail.\n");
        return EECode_NoMemory;
    }

    mTextBufferSize = 16 * 1024;
    mpTextBuffer = (TChar *) DDBG_MALLOC(mTextBufferSize, "C0RS");
    if (DUnlikely(!mpTextBuffer)) {
        LOGM_FATAL("alloc(%d) fail.\n", mTextBufferSize);
        return EECode_NoMemory;
    }
    mRemainingSize = mTextBufferSize;
    mpCurrentWriteAddr = mpTextBuffer;

    mpMutex = gfCreateMutex();
    if (DUnlikely(!mpMutex)) {
        LOGM_FATAL("gfCreateMutex() fail.\n");
        return EECode_NoMemory;
    }

    return EECode_OK;
}

EECode CIRemoteLogServer::CreateContext(TChar *tag, TU16 native_port, TU16 remote_port)
{
    AUTO_LOCK(mpMutex);

    if (DUnlikely(!tag)) {
        LOG_ERROR("NULL tag\n");
        return EECode_BadParam;
    }

    if (DUnlikely(mbCreateContext)) {
        LOGM_WARN("already create context\n");
        return EECode_BadState;
    }

    if (DLikely(mpAgent)) {
        mpAgent->CreateContext(tag, 1, NULL, NULL, native_port, remote_port);
        mpAgent->Start();
        mbCreateContext = 1;
    } else {
        LOGM_FATAL("NULL mpAgent\n");
    }

    return EECode_OK;
}

void CIRemoteLogServer::DestroyContext()
{
    AUTO_LOCK(mpMutex);

    if (DLikely(mpAgent)) {
        if (DUnlikely(!mbCreateContext)) {
            LOGM_WARN("context already destroyed\n");
            return;
        }
        mpAgent->Stop();
        mpAgent->DestroyContext();
        mbCreateContext = 0;
    } else {
        LOGM_FATAL("NULL mpAgent\n");
    }
}

EECode CIRemoteLogServer::WriteLog(const TChar *log, ...)
{
    AUTO_LOCK(mpMutex);

    if (DUnlikely(mbBufferFull)) {
        LOGM_WARN("write buffer full\n");
        return EECode_OK;
    }

    EECode err = EECode_OK;
    TInt length = 0;

    va_list vlist;

    va_start(vlist, log);
    length = vsnprintf(mpCurrentWriteAddr, mRemainingSize, log, vlist);
    if (DLikely((0 < length) && (length < (mRemainingSize - 4)))) {
        //LOGM_VERBOSE("length %d, mRemainingSize %d, mpCurrentWriteAddr [%s]\n", length, mRemainingSize, mpCurrentWriteAddr);
        mpCurrentWriteAddr += length;
        mRemainingSize -= length;
        err = EECode_OK;
    } else if (length >= (mRemainingSize - 4)) {
        LOGM_ERROR("length %d, mRemainingSize %d\n", length, mRemainingSize);
        err = EECode_NoMemory;
    } else {
        LOGM_ERROR("length %d, mRemainingSize %d\n", length, mRemainingSize);
        err = EECode_InternalLogicalBug;
    }
    va_end(vlist);

    return err;
}

EECode CIRemoteLogServer::SyncLog()
{
    AUTO_LOCK(mpMutex);

    if (DLikely(mpAgent)) {
        //LOGM_VERBOSE("mpTextBuffer [%s], mTextBufferSize %d, mRemainingSize %d\n", mpTextBuffer, mTextBufferSize, mRemainingSize);
        mpAgent->Write((TU8 *)mpTextBuffer, mTextBufferSize - mRemainingSize);
    } else {
        LOGM_FATAL("NULL mpAgent\n");
    }

    mpCurrentWriteAddr = mpTextBuffer;
    mRemainingSize = mTextBufferSize;
    mbBufferFull = 0;

    return EECode_OK;
}

void CIRemoteLogServer::Delete()
{
    if (mpAgent) {
        mpAgent->Destroy();
        mpAgent = NULL;
    }

    if (mpTextBuffer) {
        DDBG_FREE(mpTextBuffer, "C0RS");
        mpTextBuffer = NULL;
    }

    if (mpMutex) {
        mpMutex->Delete();
        mpMutex = NULL;
    }
}

void CIRemoteLogServer::Destroy()
{
    Delete();
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END


