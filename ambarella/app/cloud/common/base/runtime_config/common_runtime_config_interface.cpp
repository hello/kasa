/**
 * common_runtime_config_interface.cpp
 *
 * History:
 *    2013/05/24 - [Zhi He] create file
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

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

//#ifdef BUILD_MODULE_LIBXML2
//extern IRunTimeConfigAPI* gfCreateXMLRunTimeConfigAPI();
//#endif

extern IRunTimeConfigAPI *gfCreateSimpleINIRunTimeConfigAPI(TU32 max);

IRunTimeConfigAPI *gfRunTimeConfigAPIFactory(ERunTimeConfigType config_type, TU32 max)
{
    IRunTimeConfigAPI *api = NULL;

    switch (config_type) {
        case ERunTimeConfigType_XML:
            //#ifdef BUILD_MODULE_LIBXML2
            //            api = gfCreateXMLRunTimeConfigAPI();
            //#else
            LOG_NOTICE("libxml2 related module is not enabled\n");
            //#endif
            break;

        case ERunTimeConfigType_SimpleINI:
            api = gfCreateSimpleINIRunTimeConfigAPI(max);
            break;

        default:
            LOG_FATAL("BAD ERunTimeConfigType %d\n", config_type);
            break;
    }

    return api;
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

