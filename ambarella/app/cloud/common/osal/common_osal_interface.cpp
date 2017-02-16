/**
 * common_osal_interface.cpp
 *
 * History:
 *  2014/07/24 - [Zhi He] create file
 *
 * Copyright (C) 2014 - 2024, the Ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the Ambarella Inc.
 */

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

