/**
 * lw_database_interface.cpp
 *
 * History:
 *  2014/05/27 - [Zhi He] create file
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

#include "lw_database_if.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

TUniqueID gfGenerateHashMainID(TChar *name)
{
    TUniqueID val = 0;

    if (DLikely(name)) {
        TChar new_name[DMAX_ACCOUNT_NAME_LENGTH_EX + DMAX_ACCOUNT_NAME_LENGTH_EX] = {0x55};

        TInt name_length = strlen(name);
        if (DUnlikely(name_length > DMAX_ACCOUNT_NAME_LENGTH)) {
            LOG_FATAL("%s too long\n", name);
            return DInvalidUniqueID;
        }
        TInt min_length = 8;
        if (name_length >= min_length) {
            memcpy(new_name, name, name_length);
        } else {
            for (TInt i = 0; i < (min_length / name_length + 1); i++) {
                memcpy(new_name + i * name_length, name, name_length);
            }
        }

        TInt length = DMAX_ACCOUNT_NAME_LENGTH;
        TChar *ptmp = new_name;
        TU8 v1 = new_name[0] + new_name[2] + new_name[3] + 50;
        TU8 v2 = 0;
        TU8 v3 = new_name[4] + new_name[5] - new_name[6] - new_name[7];

        do {
            if (0 == v2) {
                ptmp[name_length] = 0x0;
                v2 = 3;
            } else {
                ptmp[name_length] = v1;
                v1 += v3;
            }

            if (1 == v2) {
                ptmp[name_length + 1] = v1;
                v1 += v3;
                v2 = 2;
                length -= name_length + 2;
                ptmp += name_length + 2;
            } else if (2 == v2) {
                ptmp[name_length + 1] = v1;
                v1 += v3;
                ptmp[name_length + 2] = v1;
                v1 += v3;
                v2 = 3;
                length -= name_length + 3;
                ptmp += name_length + 3;
            } else {
                v2 = 1;
                length -= name_length + 1;
                ptmp += name_length + 1;
            }

        } while (length > 0);

        v1 = (new_name[0] - 31 + (new_name[4] & 0x1f)) | ((((TU8)new_name[8]) & 0x4) << 4);
        v2 = (new_name[1] - 31 + (new_name[5] & 0x1f)) | ((((TU8)new_name[9]) & 0x4) << 4);
        v3 = ((new_name[12] + new_name[13]) ^ (new_name[15] - new_name[16])) + new_name[20] - new_name[24];
        TU8 v4 = ((new_name[16] | new_name[17]) ^ (new_name[18] & new_name[19])) + new_name[21] - new_name[28];

        val = ((TUniqueID)v1 << 56) | ((TUniqueID)v2 << 48) | ((TUniqueID)v3 << 40) | ((TUniqueID)v4 << 32);

    } else {
        LOG_FATAL("NULL name\n");
        return DInvalidUniqueID;
    }

    return val;
}

extern ILightWeightDataBase *gfCreateLightWeightTwoLevelHashDatabase(const TChar *pName, const volatile SPersistCommonConfig *pPersistCommonConfig, IMsgSink *pMsgSink, TUint index);

ILightWeightDataBase *gfCreateLightWeightDataBase(ELightWeightDataBaseType type, const volatile SPersistCommonConfig *pPersistCommonConfig, IMsgSink *pMsgSink, TUint index)
{
    ILightWeightDataBase *thiz = NULL;

    switch (type) {

        case ELightWeightDataBaseType_twoLevelHash_v1:
            thiz = gfCreateLightWeightTwoLevelHashDatabase("CLWDataBaseTwoLevelHash", pPersistCommonConfig, pMsgSink, index);
            break;

        default:
            LOG_FATAL("not supported type %d\n", type);
            break;
    }

    return thiz;
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

