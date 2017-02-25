/*******************************************************************************
 * engine_internal.cpp
 *
 * History:
 *    2014/10/28 - [Zhi He] create file
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
#include "common_network_utils.h"

#include "cloud_lib_if.h"
#include "sacp_types.h"

#include "storage_lib_if.h"
#include "media_mw_if.h"

#include "media_mw_utils.h"
#include "mw_internal.h"

#include "framework_interface.h"
#include "filters_interface.h"
#include "dsp_platform_interface.h"
#include "streaming_if.h"
#include "modules_interface.h"

#include "engine_internal.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

TUint isComponentIDValid(TGenericID id, TComponentType check_type)
{
    TComponentIndex index = DCOMPONENT_INDEX_FROM_GENERIC_ID(id);
    TComponentType type = DCOMPONENT_TYPE_FROM_GENERIC_ID(id);

    if (DLikely(IS_VALID_COMPONENT_ID(id))) {

        if (DUnlikely(check_type != type)) {
            LOG_ERROR("id %08x is not a %s, it's %s\n", id, gfGetComponentStringFromGenericComponentType(check_type), gfGetComponentStringFromGenericComponentType(type));
            return 0;
        }

        //rough check index
        if (DUnlikely(index > 1024)) {
            LOG_ERROR("id %08x's index(%d) exceed 256?\n", id, index);
            return 0;
        }
    } else {
        LOG_ERROR("id %08x' has no mark bit\n", id);
        return 0;
    }

    return 1;
}

TUint isComponentIDValid_2(TGenericID id, TComponentType check_type, TComponentType check_type_2)
{
    TComponentIndex index = DCOMPONENT_INDEX_FROM_GENERIC_ID(id);
    TComponentType type = DCOMPONENT_TYPE_FROM_GENERIC_ID(id);

    if (DLikely(IS_VALID_COMPONENT_ID(id))) {

        if (DUnlikely((check_type != type) && (check_type_2 != type))) {
            LOG_ERROR("id %08x is not a %s or %s, it's %s\n", id, gfGetComponentStringFromGenericComponentType(check_type), gfGetComponentStringFromGenericComponentType(check_type_2), gfGetComponentStringFromGenericComponentType(type));
            return 0;
        }

        //rough check index
        if (DUnlikely(index > 1024)) {
            LOG_ERROR("id %08x's index(%d) exceed 256?\n", id, index);
            return 0;
        }
    } else {
        LOG_ERROR("id %08x' has no mark bit\n", id);
        return 0;
    }

    return 1;
}

TUint isComponentIDValid_3(TGenericID id, TComponentType check_type, TComponentType check_type_2, TComponentType check_type_3)
{
    TComponentIndex index = DCOMPONENT_INDEX_FROM_GENERIC_ID(id);
    TComponentType type = DCOMPONENT_TYPE_FROM_GENERIC_ID(id);

    if (DLikely(IS_VALID_COMPONENT_ID(id))) {

        if (DUnlikely((check_type != type) && (check_type_2 != type) && (check_type_3 != type))) {
            LOG_ERROR("id %08x is not a %s or %s or %s, it's %s\n", id, gfGetComponentStringFromGenericComponentType(check_type), gfGetComponentStringFromGenericComponentType(check_type_2), gfGetComponentStringFromGenericComponentType(check_type_3), gfGetComponentStringFromGenericComponentType(type));
            return 0;
        }

        //rough check index
        if (DUnlikely(index > 1024)) {
            LOG_ERROR("id %08x's index(%d) exceed 256?\n", id, index);
            return 0;
        }
    } else {
        LOG_ERROR("id %08x' has no mark bit\n", id);
        return 0;
    }

    return 1;
}

TUint isComponentIDValid_4(TGenericID id, TComponentType check_type, TComponentType check_type_2, TComponentType check_type_3, TComponentType check_type_4)
{
    TComponentIndex index = DCOMPONENT_INDEX_FROM_GENERIC_ID(id);
    TComponentType type = DCOMPONENT_TYPE_FROM_GENERIC_ID(id);

    if (DLikely(IS_VALID_COMPONENT_ID(id))) {

        if (DUnlikely((check_type != type) && (check_type_2 != type) && (check_type_3 != type) && (check_type_4 != type))) {
            LOG_ERROR("id %08x is not a %s or %s or %s or %s, it's %s\n", id, gfGetComponentStringFromGenericComponentType(check_type), gfGetComponentStringFromGenericComponentType(check_type_2), gfGetComponentStringFromGenericComponentType(check_type_3), gfGetComponentStringFromGenericComponentType(check_type_4), gfGetComponentStringFromGenericComponentType(type));
            return 0;
        }

        //rough check index
        if (DUnlikely(index > 1024)) {
            LOG_ERROR("id %08x's index(%d) exceed 256?\n", id, index);
            return 0;
        }
    } else {
        LOG_ERROR("id %08x' has no mark bit\n", id);
        return 0;
    }

    return 1;
}

TUint isComponentIDValid_5(TGenericID id, TComponentType check_type, TComponentType check_type_2, TComponentType check_type_3, TComponentType check_type_4, TComponentType check_type_5)
{
    TComponentIndex index = DCOMPONENT_INDEX_FROM_GENERIC_ID(id);
    TComponentType type = DCOMPONENT_TYPE_FROM_GENERIC_ID(id);

    if (DLikely(IS_VALID_COMPONENT_ID(id))) {

        if (DUnlikely((check_type != type) && (check_type_2 != type) && (check_type_3 != type) && (check_type_4 != type) && (check_type_5 != type))) {
            LOG_ERROR("id %08x is not a %s or %s or %s or %s or %s, it's %s\n", id, gfGetComponentStringFromGenericComponentType(check_type), gfGetComponentStringFromGenericComponentType(check_type_2), gfGetComponentStringFromGenericComponentType(check_type_3), gfGetComponentStringFromGenericComponentType(check_type_4), gfGetComponentStringFromGenericComponentType(check_type_5), gfGetComponentStringFromGenericComponentType(type));
            return 0;
        }

        //rough check index
        if (DUnlikely(index > 1024)) {
            LOG_ERROR("id %08x's index(%d) exceed 256?\n", id, index);
            return 0;
        }
    } else {
        LOG_ERROR("id %08x' has no mark bit\n", id);
        return 0;
    }

    return 1;
}

TUint isComponentIDValid_6(TGenericID id, TComponentType check_type, TComponentType check_type_2, TComponentType check_type_3, TComponentType check_type_4, TComponentType check_type_5, TComponentType check_type_6)
{
    TComponentIndex index = DCOMPONENT_INDEX_FROM_GENERIC_ID(id);
    TComponentType type = DCOMPONENT_TYPE_FROM_GENERIC_ID(id);

    if (DLikely(IS_VALID_COMPONENT_ID(id))) {

        if (DUnlikely((check_type != type) && (check_type_2 != type) && (check_type_3 != type) && (check_type_4 != type) && (check_type_5 != type) && (check_type_6 != type))) {
            LOG_ERROR("id %08x is not a %s or %s or %s or %s or %s or %s, it's %s\n", id, gfGetComponentStringFromGenericComponentType(check_type), gfGetComponentStringFromGenericComponentType(check_type_2), gfGetComponentStringFromGenericComponentType(check_type_3), gfGetComponentStringFromGenericComponentType(check_type_4), gfGetComponentStringFromGenericComponentType(check_type_5), gfGetComponentStringFromGenericComponentType(check_type_6), gfGetComponentStringFromGenericComponentType(type));
            return 0;
        }

        //rough check index
        if (DUnlikely(index > 1024)) {
            LOG_ERROR("id %08x's index(%d) exceed 256?\n", id, index);
            return 0;
        }
    } else {
        LOG_ERROR("id %08x' has no mark bit\n", id);
        return 0;
    }

    return 1;
}

EFilterType __filterTypeFromGenericComponentType(TU8 generic_component_type, TU8 scheduled_video_decoder, TU8 scheduled_muxer)
{
    EFilterType type = EFilterType_Invalid;

    switch (generic_component_type) {
        case EGenericComponentType_Demuxer:
            type = EFilterType_Demuxer;
            break;

        case EGenericComponentType_VideoDecoder:
            if (!scheduled_video_decoder) {
                type = EFilterType_VideoDecoder;
            } else {
                type = EFilterType_ScheduledVideoDecoder;
            }
            break;

        case EGenericComponentType_AudioDecoder:
            type = EFilterType_AudioDecoder;
            break;

        case EGenericComponentType_VideoEncoder:
            type = EFilterType_VideoEncoder;
            break;

        case EGenericComponentType_AudioEncoder:
            type = EFilterType_AudioEncoder;
            break;

        case EGenericComponentType_VideoPostProcessor:
            type = EFilterType_VideoPostProcessor;
            break;

        case EGenericComponentType_AudioPostProcessor:
            type = EFilterType_AudioPostProcessor;
            break;

        case EGenericComponentType_VideoPreProcessor:
            type = EFilterType_VideoPreProcessor;
            break;

        case EGenericComponentType_AudioPreProcessor:
            type = EFilterType_AudioPreProcessor;
            break;

        case EGenericComponentType_VideoCapture:
            type = EFilterType_VideoCapture;
            break;

        case EGenericComponentType_AudioCapture:
            type = EFilterType_AudioCapture;
            break;

        case EGenericComponentType_VideoRenderer:
            type = EFilterType_VideoRenderer;
            break;

        case EGenericComponentType_AudioRenderer:
            type = EFilterType_AudioRenderer;
            break;

        case EGenericComponentType_Muxer:
            if (!scheduled_muxer) {
                type = EFilterType_Muxer;
            } else {
                type = EFilterType_ScheduledMuxer;
            }
            break;

        case EGenericComponentType_StreamingTransmiter:
            type = EFilterType_RTSPTransmiter;
            break;

        case EGenericComponentType_ConnecterPinMuxer:
            type = EFilterType_ConnecterPinmuxer;
            break;

        case EGenericComponentType_CloudConnecterServerAgent:
            type = EFilterType_CloudConnecterServerAgent;
            break;

        case EGenericComponentType_CloudConnecterClientAgent:
            type = EFilterType_CloudConnecterClientAgent;
            break;

        case EGenericComponentType_CloudConnecterCmdAgent:
            type = EFilterType_CloudConnecterCmdAgent;
            break;

        case EGenericComponentType_FlowController:
            type = EFilterType_FlowController;
            break;

        case EGenericComponentType_VideoOutput:
            type = EFilterType_VideoOutput;
            break;

        case EGenericComponentType_AudioOutput:
            type = EFilterType_AudioOutput;
            break;

        case EGenericComponentType_ExtVideoESSource:
            type = EFilterType_ExtVideoSource;
            break;

        case EGenericComponentType_ExtAudioESSource:
            type = EFilterType_ExtAudioSource;
            break;

        default:
            LOG_FATAL("(%x) not a filter type.\n", generic_component_type);
            break;
    }

    return type;
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END



