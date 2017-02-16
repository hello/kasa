/**
 * mw_internal.cpp
 *
 * History:
 *  2013/09/27 - [Zhi He] create file
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

#include "media_mw_if.h"

#include "media_mw_utils.h"
#include "mw_internal.h"

#include "framework_interface.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

TU8 *__validStartPoint(TU8 *start, TUint &size)
{
    TU8 start_code_prefix[4] = {0, 0, 0, 1};
    TU8 start_code_ex_prefix[3] = {0, 0, 1};

    while (size > 4) {
        if (memcmp(start, start_code_prefix, sizeof(start_code_prefix)) == 0) {
            start += 4;
            size -= 4;
            continue;
        } else if (memcmp(start, start_code_ex_prefix, sizeof(start_code_ex_prefix)) == 0) {
            start += 3;
            size -= 3;
            continue;
        } else {
            return start;
        }
    }

    return start;
}

ContainerType __guessMuxContainer(TChar *p)
{
    DASSERT(p);

    if (p) {
        TChar *p1 = p;

        p1 = strchr(p, '.');
        if (!p1) {
            return ContainerType_AUTO;
        } else {
            return gfGetContainerTypeFromString(p1 + 1);
        }
    }

    return ContainerType_Invalid;
}

const TChar *gfGetRTPRecieverStateString(TU32 state)
{
    switch (state) {
        case DATA_THREAD_STATE_READ_FIRST_RTP_PACKET:
            return "(DATA_THREAD_STATE_READ_FIRST_RTP_PACKET)";
            break;

        case DATA_THREAD_STATE_READ_REMANING_RTP_PACKET:
            return "(DATA_THREAD_STATE_READ_REMANING_RTP_PACKET)";
            break;

        case DATA_THREAD_STATE_WAIT_OUTBUFFER:
            return "(DATA_THREAD_STATE_WAIT_OUTBUFFER)";
            break;

        case DATA_THREAD_STATE_SKIP_DATA:
            return "(DATA_THREAD_STATE_SKIP_DATA)";
            break;

        case DATA_THREAD_STATE_READ_FRAME_HEADER:
            return "(DATA_THREAD_STATE_READ_FRAME_HEADER)";
            break;

        case DATA_THREAD_STATE_READ_RTP_HEADER:
            return "(DATA_THREAD_STATE_READ_RTP_HEADER)";
            break;

        case DATA_THREAD_STATE_READ_RTP_VIDEO_HEADER:
            return "(DATA_THREAD_STATE_READ_RTP_VIDEO_HEADER)";
            break;

        case DATA_THREAD_STATE_READ_RTP_AUDIO_HEADER:
            return "(DATA_THREAD_STATE_READ_RTP_AUDIO_HEADER)";
            break;

        case DATA_THREAD_STATE_READ_RTP_VIDEO_DATA:
            return "(DATA_THREAD_STATE_READ_RTP_VIDEO_DATA)";
            break;

        case DATA_THREAD_STATE_READ_RTP_AAC_HEADER:
            return "(DATA_THREAD_STATE_READ_RTP_AAC_HEADER)";
            break;

        case DATA_THREAD_STATE_READ_RTP_AUDIO_DATA:
            return "(DATA_THREAD_STATE_READ_RTP_AUDIO_DATA)";
            break;

        case DATA_THREAD_STATE_READ_RTCP:
            return "(DATA_THREAD_STATE_READ_RTCP)";
            break;

        case DATA_THREAD_STATE_COMPLETE:
            return "(DATA_THREAD_STATE_COMPLETE)";
            break;

        case DATA_THREAD_STATE_ERROR:
            return "(DATA_THREAD_STATE_ERROR)";
            break;

        default:
            LOG_ERROR("Unknown rtprcv state %d\n", state);
            break;
    }

    return "(Unknown rtprcv state)";
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

