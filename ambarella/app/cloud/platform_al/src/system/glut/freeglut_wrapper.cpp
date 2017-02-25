/*******************************************************************************
 * freeglut_wrapper.cpp
 *
 * History:
 *  2014/11/17 - [Zhi He] create file
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

#ifdef BUILD_MODULE_FREE_GULT

#include "common_types.h"
#include "common_osal.h"
#include "common_utils.h"
#include "common_log.h"

#include "common_base.h"

#include "platform_al_if.h"

#include "freeglut_wrapper.h"

#include <GL/glew.h>
#include "freeglut.h"

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

IPlatformWindow *gfCreatePlatformWindowFreeGLUT(CIQueue *p_msg_queue)
{
    IPlatformWindow *thiz = CIPlatformWindowFreeGLUT::Create(p_msg_queue);
    return thiz;
}

//-----------------------------------------------------------------------
//
// CIPlatformWindowFreeGLUT
//
//-----------------------------------------------------------------------

CIPlatformWindowFreeGLUT *CIPlatformWindowFreeGLUT::Create(CIQueue *p_msg_queue)
{
    CIPlatformWindowFreeGLUT *result = new CIPlatformWindowFreeGLUT();
    if (DUnlikely((result) && (EECode_OK != result->Construct()))) {
        result->Destroy();
        result = NULL;
    }
    return result;
}

CIPlatformWindowFreeGLUT::CIPlatformWindowFreeGLUT()
    : mbCursorVisible(1)
    , mbDrawContextSetup(0)
    , mCursorShape(EMouseShape_Standard)
    , mWndState(EWindowState_Nomal)
    , mWidth(0)
    , mHeight(0)
    , mPosx(0)
    , mPosy(0)
{
}

EECode CIPlatformWindowFreeGLUT::Construct()
{
    return EECode_OK;
}

CIPlatformWindowFreeGLUT::~CIPlatformWindowFreeGLUT()
{
}

EECode CIPlatformWindowFreeGLUT::Initialize()
{
    TInt argc = 0;

    glutInit(&argc, NULL);

    //GLUT_DOUBLE
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA /*| GLUT_STENCIL | GLUT_DEPTH*/);

    return EECode_OK;
}

void CIPlatformWindowFreeGLUT::DeInitialize()
{
    return;
}

EECode CIPlatformWindowFreeGLUT::CreateMainWindow(const TChar *name, TDimension posx, TDimension posy, TDimension width, TDimension height, EWindowState state)
{
    mWidth = width;
    mHeight = height;
    mPosx = posx;
    mPosy = posy;

    glutInitWindowPosition(mPosx, mPosy);
    glutInitWindowSize(mWidth, mHeight);
    glutCreateWindow(name);

    GLenum ret = glewInit();
    if (DUnlikely(GLEW_OK != ret)) {
        LOG_FATAL("CIPlatformWindowFreeGLUT::CreateMainWindow: glewInit return fail, ret %d\n", ret);
        return EECode_OSError;
    }

    return EECode_OK;
}

EECode CIPlatformWindowFreeGLUT::MoveMainWindow(TDimension posx, TDimension posy)
{
    LOG_NOTICE("CIPlatformWindowFreeGLUT::MoveMainWindow: TO DO\n");
    return EECode_OK;
}

EECode CIPlatformWindowFreeGLUT::ReSizeMainWindow(TDimension width, TDimension height)
{
    LOG_NOTICE("CIPlatformWindowFreeGLUT::ReSizeMainWindow: TO DO\n");
    return EECode_OK;
}

EECode CIPlatformWindowFreeGLUT::ReSizeClientWindow(TDimension width, TDimension height)
{
    LOG_NOTICE("CIPlatformWindowFreeGLUT::ReSizeClientWindow: TO DO\n");
    return EECode_OK;
}

void CIPlatformWindowFreeGLUT::DestroyMainWindow()
{

}

EECode CIPlatformWindowFreeGLUT::GetWindowRect(SRect &rect)
{
    rect.width = mWidth;
    rect.height = mHeight;
    rect.pos_x = mPosx;
    rect.pos_y = mPosy;
    return EECode_OK;
}

EECode CIPlatformWindowFreeGLUT::GetClientRect(SRect &rect)
{
    rect.width = mWidth;
    rect.height = mHeight;
    rect.pos_x = mPosx;
    rect.pos_y = mPosy;
    return EECode_OK;
}

EECode CIPlatformWindowFreeGLUT::SetClientWidth(TS32 width)
{
    LOG_FATAL("CIPlatformWindowFreeGLUT::SetClientWidth: TODO\n");
    return EECode_OK;
}


EECode CIPlatformWindowFreeGLUT::SetClientHeight(TS32 height)
{
    LOG_FATAL("CIPlatformWindowFreeGLUT::SetClientHeight: TODO\n");
    return EECode_OK;
}

EECode CIPlatformWindowFreeGLUT::SetClientSize(TDimension width, TDimension height)
{
    LOG_FATAL("CIPlatformWindowFreeGLUT::SetClientSize: TODO\n");
    return EECode_OK;
}

EWindowState CIPlatformWindowFreeGLUT::GetWindowState() const
{
    LOG_FATAL("CIPlatformWindowFreeGLUT::GetWindowState: TODO\n");
    return EWindowState_Nomal;
}

EECode CIPlatformWindowFreeGLUT::SetWindowState(EWindowState state)
{
    LOG_FATAL("CIPlatformWindowFreeGLUT::SetWindowState: TODO\n");
    return EECode_OK;
}

EECode CIPlatformWindowFreeGLUT::PreDrawing()
{
    //GLenum glerror = glGetError();
    //LOG_PRINTF("[GL]: PreDrawing: before glClearColor, glerror %d\n", glerror);

    glClearColor(0.0, 255, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT);

    //glerror = glGetError();
    //LOG_PRINTF("[GL]: PreDrawing: after glClear, glerror %d\n", glerror);
    return EECode_OK;
}

EECode CIPlatformWindowFreeGLUT::InitOrthoDrawing(TDimension width, TDimension height)
{
    return EECode_OK;
}

EECode CIPlatformWindowFreeGLUT::SwapBuffers()
{
    glutSwapBuffers();

    return EECode_OK;
}

EECode CIPlatformWindowFreeGLUT::ActivateDrawingContext()
{
    return EECode_OK;
}

EECode CIPlatformWindowFreeGLUT::Invalidate()
{
    return EECode_OK;
}

EECode CIPlatformWindowFreeGLUT::LoadCursor(TU8 visible, EMouseShape cursor) const
{
    LOG_FATAL("CIPlatformWindowFreeGLUT::LoadCursor: TODO\n");
    return EECode_OK;
}

EECode CIPlatformWindowFreeGLUT::ProcessEvent()
{
    return EECode_OK;
}

TULong CIPlatformWindowFreeGLUT::GetWindowHandle() const
{
    return 0;
}

void CIPlatformWindowFreeGLUT::Destroy()
{
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END

#endif

