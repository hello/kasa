/**
 * io.cpp
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

#include "stdlib.h"
#include "stdio.h"
#include "string.h"

#include "lwmd_if.h"
#include "lwmd_log.h"

#include "internal.h"

#include "file_io.h"

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

