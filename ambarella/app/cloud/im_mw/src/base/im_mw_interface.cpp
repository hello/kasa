/**
 * im_mw_interface.cpp
 *
 * History:
 *  2014/06/12 - [Zhi He] create file
 *
 * Copyright (C) 2014 - 2024, the Ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the Ambarella Inc.
 */

#include "common_config.h"
#include "common_types.h"
#include "common_osal.h"
#include "common_utils.h"

#include "common_log.h"

#include "common_base.h"

#include "cloud_lib_if.h"

#include "im_mw_if.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

extern IAccountManager *gfCreateGenericAccountManager(const TChar *pName, const volatile SPersistCommonConfig *pPersistCommonConfig, IMsgSink *pMsgSink, TUint index);

IAccountManager *gfCreateAccountManager(EAccountManagerType type, const volatile SPersistCommonConfig *pPersistCommonConfig, IMsgSink *pMsgSink, TUint index)
{
    IAccountManager *thiz = NULL;

    switch (type) {

        case EAccountManagerType_generic:
            thiz = gfCreateGenericAccountManager("CGenericAccountManager", pPersistCommonConfig, pMsgSink, index);
            break;

        default:
            LOG_FATAL("EAccountManagerType %d not implemented\n", type);
            break;
    }

    return thiz;
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

