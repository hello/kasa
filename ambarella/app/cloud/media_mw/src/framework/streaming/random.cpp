/**
 * random.cpp
 *
 * History:
 *    2014/06/15 - [Zhi He] create file
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

#include "random.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

#define DRANDOM_SEED_1 17
#define DRANDOM_SEED_2 39
#define DRANDOM_SEED_3 89
#define DRANDOM_SEED_4 71

TU32 gfRandom32(void)
{
    static TU32 random_count = 0;

    SDateTime datetime;
    gfGetCurrentDateTime(&datetime);
    TU32 value = datetime.seconds;
    value += (DRANDOM_SEED_1 * random_count);

    value &= (((DRANDOM_SEED_2 * random_count) & 0xff) << 8);
    value |= (((DRANDOM_SEED_3 * random_count) & 0xff) << 16);

    value += DRANDOM_SEED_4 + random_count;

    random_count ++;
    return value;
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

