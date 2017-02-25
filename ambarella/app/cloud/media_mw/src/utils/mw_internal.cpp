/*******************************************************************************
 * mw_internal.cpp
 *
 * History:
 *  2013/09/27 - [Zhi He] create file
 *
 * Copyright (c) 2016 Ambarella, Inc.
 *
 * This file and its contents ("Software") are protected by intellectual
 * property rights including, without limitation, U.S. and/or foreign
 * copyrights. This Software is also the confidential and proprietary
 * information of Ambarella, Inc. and its licensors. You may not use, reproduce,
 * disclose, distribute, modify, or otherwise prepare derivative works of this
 * Software or any portion thereof except pursuant to a signed license agreement
 * or nondisclosure agreement with Ambarella, Inc. or its authorized affiliates.
 * In the absence of such an agreement, you agree to promptly notify and return
 * this Software to Ambarella, Inc.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF NON-INFRINGEMENT,
 * MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL AMBARELLA, INC. OR ITS AFFILIATES BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; COMPUTER FAILURE OR MALFUNCTION; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************/

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

