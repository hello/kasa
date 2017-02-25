/*******************************************************************************
 * platform_al_interface.cpp
 *
 * History:
 *  2014/11/11 - [Zhi He] create file
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

