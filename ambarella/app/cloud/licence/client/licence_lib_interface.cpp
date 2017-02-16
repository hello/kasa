/**
 * licence_lib_interface.cpp
 *
 * History:
 *  2014/03/07 - [Zhi He] create file
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

#include "licence_lib_if.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

extern ILicenceClient *gfCreateStandaloneLicence();

ILicenceClient *gfCreateLicenceClient(ELicenceType type)
{
    ILicenceClient *thiz = NULL;

    switch (type) {

        case ELicenceType_StandAlone:
            thiz = gfCreateStandaloneLicence();
            break;

        default:
            LOG_FATAL("not supported type %d\n", type);
            break;
    }

    return thiz;
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

