/*******************************************************************************
 * common_osal_interface.cpp
 *
 * History:
 *  2014/07/24 - [Zhi He] create file
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

#include <semaphore.h>
#include <pthread.h>

#include "common_config.h"
#include "common_types.h"
#include "common_osal.h"
#include "common_utils.h"

#include "common_log.h"

#include "common_base.h"

#include "generic/common_osal_generic.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

//-----------------------------------------------------------------------
//
// CIAutoLock
//
//-----------------------------------------------------------------------

CIAutoLock::CIAutoLock(IMutex *pMutex): _pMutex(pMutex)
{
    if (_pMutex) {
        _pMutex->Lock();
    }
}

CIAutoLock::~CIAutoLock()
{
    if (_pMutex) {
        _pMutex->Unlock();
    }
}



IMutex *gfCreateMutex()
{
    return CIMutex::Create(false);
}

ICondition *gfCreateCondition()
{
    return CICondition::Create();
}

IEvent *gfCreateEvent()
{
    return CIEvent::Create();
}

IThread *gfCreateThread(const TChar *pName, TF_THREAD_FUNC entry, void *pParam, TUint schedule, TUint priority, TUint affinity)
{
    return CIThread::Create(pName, entry, pParam, schedule, priority, affinity);
}

extern IClockSource *gfCreateGenericClockSource();

IClockSource *gfCreateClockSource(EClockSourceType type)
{
    IClockSource *thiz = NULL;
    switch (type) {
        case EClockSourceType_generic:
            thiz = gfCreateGenericClockSource();
            break;

        default:
            LOG_FATAL("BAD EClockSourceType %d\n", type);
            break;
    }

    return thiz;
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

