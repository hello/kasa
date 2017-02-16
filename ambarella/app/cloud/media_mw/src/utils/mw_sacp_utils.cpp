/**
 * mw_sacp_utils.cpp
 *
 * History:
 *  2014/08/25 - [Zhi He] create file
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
#include "sacp_types.h"

#include "media_mw_if.h"

#include "media_mw_utils.h"

#include "mw_sacp_utils.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

StreamFormat gfGetStreamingFormatFromSACPSubType(ESACPDataChannelSubType sub_type)
{
    switch (sub_type) {

        case ESACPDataChannelSubType_H264_NALU:
            return StreamFormat_H264;
            break;

        case ESACPDataChannelSubType_AAC:
            return StreamFormat_AAC;
            break;

        case ESACPDataChannelSubType_MPEG12Audio:
            return StreamFormat_MPEG12Audio;
            break;

        case ESACPDataChannelSubType_G711_PCMU:
            return StreamFormat_PCMU;
            break;

        case ESACPDataChannelSubType_G711_PCMA:
            return StreamFormat_PCMA;
            break;

        default:
            LOG_FATAL("bad sub_type %d\n", sub_type);
            break;
    }

    return StreamFormat_Invalid;
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

