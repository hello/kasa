/**
 * storage_lib_interface.h
 *
 * History:
 *  2014/02/11 - [Zhi He] create file
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

#include "storage_lib_if.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

extern IStorageManagement *gfCreateSimpleStorageManagement();

IStorageManagement *gfCreateStorageManagement(EStorageMnagementType type)
{
    IStorageManagement *thiz = NULL;

    switch (type) {

        case EStorageMnagementType_Simple:
#ifdef BUILD_OS_WINDOWS
            LOG_ERROR("not implement in windows yet\n");
            return NULL;
#else
            thiz = gfCreateSimpleStorageManagement();
#endif
            break;

        default:
            LOG_FATAL("EStorageMnagementType %d not implemented yet\n", type);
            break;
    }

    return thiz;
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

