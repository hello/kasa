/**
 * licence_encryption_interface.cpp
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
#include "licence_encryption_if.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

extern ILicenceEncryptor *gfCreateCustomizedV1Encryptor(TU32 seed1, TU32 seed2);

ILicenceEncryptor *gfCreateLicenceEncryptor(ELicenceEncryptionType type, TU32 seed1, TU32 seed2)
{
    ILicenceEncryptor *thiz = NULL;

    switch (type) {

        case ELicenceEncryptionType_CustomizedV1:
            thiz = gfCreateCustomizedV1Encryptor(seed1, seed2);
            break;

        default:
            LOG_FATAL("un expected ELicenceEncryptionType %d\n", type);
            break;
    }

    return thiz;
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

