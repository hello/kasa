/*******************************************************************************
 * app_framework_interface.cpp
 *
 * History:
 *  2014/10/08 - [Zhi He] create file
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

