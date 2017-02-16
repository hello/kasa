/**
 * app_framework_interface.cpp
 *
 * History:
 *  2014/10/08 - [Zhi He] create file
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

#include "app_framework_if.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

extern ISceneDirector *gfCreateBasicSceneDirector(EVisualSettingPrefer visual_platform_prefer, SPersistCommonConfig *p_common_config, SSharedComponent *p_shared_component);
extern ISoundComposer *gfCreateBasicSoundComposer(SPersistCommonConfig *p_common_config, SSharedComponent *p_shared_component);
extern CGUIArea *gfCreateGUIAreaYUVTexture(IVisualDirectRendering *&p_direct_rendering);
//extern ISceneDirector* gfCreateDirectVoutSceneDirector(EVisualSettingPrefer visual_platform_prefer, SPersistCommonConfig* p_common_config, SSharedComponent* p_shared_component);

ISceneDirector *gfCreateSceneDirector(ESceneDirectorType type, EVisualSettingPrefer visual_platform_prefer, SPersistCommonConfig *p_common_config, SSharedComponent *p_shared_component)
{
    ISceneDirector *thiz = NULL;
    switch (type) {

        case ESceneDirectorType_basic:
            thiz = gfCreateBasicSceneDirector(visual_platform_prefer, p_common_config, p_shared_component);
            //thiz = gfCreateDirectVoutSceneDirector(visual_platform_prefer, p_common_config, p_shared_component);
            break;

        default:
            LOG_ERROR("gfCreateSceneDirector: not suported type %d\n", type);
            break;
    }

    return thiz;
}

ISoundComposer *gfCreateSoundComposer(ESoundComposerType type, SPersistCommonConfig *p_common_config, SSharedComponent *p_shared_component)
{
    ISoundComposer *thiz = NULL;
    switch (type) {

        case ESoundComposerType_basic:
            thiz = gfCreateBasicSoundComposer(p_common_config, p_shared_component);
            break;

        default:
            LOG_ERROR("gfCreateSoundComposer: not suported type %d\n", type);
            break;
    }

    return thiz;
}

CGUIArea *gfCreateGUIArea(EGUIAreaType type, IVisualDirectRendering *&p_direct_rendering)
{
    CGUIArea *thiz = NULL;
    switch (type) {

        case EGUIAreaType_Texture2D:
            thiz = gfCreateGUIAreaYUVTexture(p_direct_rendering);
            break;

        default:
            LOG_FATAL("not supported EGUIAreaType %d\n", type);
            break;
    }

    return thiz;
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

