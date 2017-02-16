/**
 * platform_al_interface.cpp
 *
 * History:
 *  2014/11/11 - [Zhi He] create file
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

#include "platform_al_if.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

#ifdef BUILD_OS_WINDOWS
extern IPlatformWindow *gfCreatePlatformWindowWinOpenGL(CIQueue *p_msg_queue);
extern ISoundOutput *gfCreateSoundOutputWinDS(SPersistCommonConfig *pPersistCommonConfig);
#endif

#ifdef BUILD_MODULE_FREE_GULT
extern IPlatformWindow *gfCreatePlatformWindowFreeGLUT(CIQueue *p_msg_queue);
#endif

extern ITexture2D *gfCreate2DTextureYUV420p();

#ifdef BUILD_MODULE_FONTFT
extern IFont *gfCreateLWFontFT();
#endif

ITexture2D *gfCreate2DTexture(ETextureSourceType type)
{
    ITexture2D *thiz = NULL;

    switch (type) {

        case ETextureSourceType_RGBA32:
            break;

        case ETextureSourceType_YUV420p:
            thiz = gfCreate2DTextureYUV420p();
            break;

        default:
            LOG_FATAL("gfCreate2DTexture: not supported type %d\n", type);
            break;
    }

    return thiz;
}

ISoundOutput *gfCreatePlatformSoundOutput(ESoundSetting setting, SPersistCommonConfig *pPersistCommonConfig)
{
    ISoundOutput *thiz = NULL;

    switch (setting) {

        case ESoundSetting_directsound:
#ifdef BUILD_OS_WINDOWS
            thiz = gfCreateSoundOutputWinDS(pPersistCommonConfig);
#else
            LOG_FATAL("not compiled in non-windows env\n");
#endif
            break;

        default:
            LOG_FATAL("gfCreatePlatformSound: not supported setting %d\n", setting);
            break;
    }

    return thiz;
}

IPlatformWindow *gfCreatePlatformWindow(CIQueue *p_msg_queue, EVisualSetting setting, EVisualSettingPrefer prefer)
{
    IPlatformWindow *thiz = NULL;

    switch (setting) {

        case EVisualSetting_opengl:
            if (EVisualSettingPrefer_freeglut == prefer) {
#ifdef BUILD_MODULE_FREE_GULT
                thiz = gfCreatePlatformWindowFreeGLUT(p_msg_queue);
#else
                LOG_FATAL("not compiled free gult\n");
#endif
            } else {
#ifdef BUILD_OS_WINDOWS
                thiz = gfCreatePlatformWindowWinOpenGL(p_msg_queue);
#else
                LOG_FATAL("not compiled in non-windows env\n");
#endif
            }
            break;

        default:
            LOG_FATAL("gfCreatePlatformWindow: not supported setting %d\n", setting);
            break;
    }

    return thiz;
}

IFont *gfCreateLWFont(ELWFontType type)
{
    IFont *thiz = NULL;
    switch (type) {

        case ELWFontType_FT2:
#ifdef BUILD_MODULE_FONTFT
            thiz = gfCreateLWFontFT();
#else
            LOG_FATAL("not compile ft\n");
#endif
            break;

        default:
            LOG_FATAL("bad type %d\n", type);
            break;
    }

    return thiz;
}

CIFontManager *CIFontManager::mpInstance = NULL;

CIFontManager *CIFontManager::GetInstance()
{
    gfGlobalMutexLock();
    if (!mpInstance) {
        mpInstance = new CIFontManager();
    }
    gfGlobalMutexUnLock();

    return mpInstance;
}

CIFontManager::CIFontManager()
{
    TU32 i = 0;
    for (i = 0; i < EFontCategory_TotalNumber; i ++) {
        mpFontEntry[i] = NULL;
    }
}

CIFontManager::~CIFontManager()
{
}

EECode CIFontManager::RegisterFont(IFont *font, EFontCategory cat)
{
    if (EFontCategory_TotalNumber > cat) {
        if (mpFontEntry[cat]) {
            LOG_WARN("already registered? cat %d\n", cat);
        }
        mpFontEntry[cat] = font;
    } else {
        LOG_FATAL("bad cat %x\n", cat);
        return EECode_BadParam;
    }
    return EECode_OK;
}

EECode CIFontManager::UnRegisterFont(IFont *font)
{
    TU32 i = 0;
    for (i = 0; i < EFontCategory_TotalNumber; i ++) {
        if (font == mpFontEntry[i]) {
            mpFontEntry[i] = NULL;
            return EECode_OK;
        }
    }

    LOG_ERROR("not found matched font\n");
    return EECode_NotExist;
}

IFont *CIFontManager::GetFont(EFontCategory cat) const
{
    if (EFontCategory_TotalNumber > cat) {
        if (!mpFontEntry[cat]) {
            LOG_WARN("not registered? cat %d\n", cat);
        }
        return mpFontEntry[cat];
    }

    LOG_FATAL("bad cat %x\n", cat);
    return NULL;
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

