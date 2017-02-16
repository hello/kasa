/**
 * common_io.cpp
 *
 * History:
 *  2013/09/09 - [Zhi He] create file
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

#include "common_io.h"

#include "common_file_io.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

IIO *gfCreateIO(EIOType type)
{
    IIO *thiz = NULL;

    switch (type) {
        case EIOType_File:
            thiz = CFileIO::Create();
            break;
        case EIOType_HTTP:
            LOG_FATAL("not implement yet\n");
            break;
        default:
            LOG_FATAL("BAD EIOType %d\n", type);
            break;
    }

    return thiz;
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

