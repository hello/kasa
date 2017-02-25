/*******************************************************************************
 * win_opengl.cpp
 *
 * History:
 *  2014/11/09 - [Zhi He] create file
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

#include "win_opengl.h"

#ifdef BUILD_OS_WINDOWS
#include <GL/glew.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_BEGIN
DCODE_DELIMITER;

#ifndef GWL_USERDATA
#define GWL_USERDATA        (-21)
#endif

IPlatformWindow *gfCreatePlatformWindowWinOpenGL(CIQueue *p_msg_queue)
{
    IPlatformWindow *thiz = CIPlatformWindowWinOpenGL::Create(p_msg_queue);
    return thiz;
}

CIQueue *CIPlatformWindowWinOpenGL::mpEventQueue = NULL;
//TU8 CIPlatformWindowWinOpenGL::mbActive = 0;
//TU8 CIPlatformWindowWinOpenGL::mbKeys[DMaxKeyNum] = {0};
LPCSTR CIPlatformWindowWinOpenGL::mpWindowClassName = "test0";

/** Reverse the bits in a TU8 */
static TU8 uns8ReverseBits(TU8 ch)
{
    ch = ((ch >> 1) & 0x55) | ((ch << 1) & 0xAA);
    ch = ((ch >> 2) & 0x33) | ((ch << 2) & 0xCC);
    ch = ((ch >> 4) & 0x0F) | ((ch << 4) & 0xF0);
    return ch;
}

/** Reverse the bits in a TU16 */
static TU16 uns16ReverseBits(TU16 shrt)
{
    shrt = ((shrt >> 1) & 0x5555) | ((shrt << 1) & 0xAAAA);
    shrt = ((shrt >> 2) & 0x3333) | ((shrt << 2) & 0xCCCC);
    shrt = ((shrt >> 4) & 0x0F0F) | ((shrt << 4) & 0xF0F0);
    shrt = ((shrt >> 8) & 0x00FF) | ((shrt << 8) & 0xFF00);
    return shrt;
}

static PIXELFORMATDESCRIPTOR sPreferredFormat = {
    sizeof(PIXELFORMATDESCRIPTOR),  /* size */
    1,  /* version */
    PFD_SUPPORT_OPENGL |
    PFD_DRAW_TO_WINDOW |
    PFD_DOUBLEBUFFER,   /* support double-buffering */
    PFD_TYPE_RGBA,  /* color type */
    32, /* prefered color depth */
    0, 0, 0, 0, 0, 0,   /* color bits (ignored) */
    0,  /* no alpha buffer */
    0,  /* alpha bits (ignored) */
    0,  /* no accumulation buffer */
    0, 0, 0, 0, /* accum bits (ignored) */
    16, /* depth buffer */
    0,  /* no stencil buffer */
    0,  /* no auxiliary buffers */
    PFD_MAIN_PLANE, /* main layer */
    0,  /* reserved */
    0, 0, 0 /* no layer, visible, damage masks */
};

//-----------------------------------------------------------------------
//
// CIPlatformWindowWinOpenGL
//
//-----------------------------------------------------------------------

CIPlatformWindowWinOpenGL *CIPlatformWindowWinOpenGL::Create(CIQueue *p_msg_queue)
{
    mpEventQueue = p_msg_queue;
    CIPlatformWindowWinOpenGL *result = new CIPlatformWindowWinOpenGL();
    if (DUnlikely((result) && (EECode_OK != result->Construct()))) {
        result->Destroy();
        result = NULL;
    }
    return result;
}

CIPlatformWindowWinOpenGL::CIPlatformWindowWinOpenGL()
    : mbCursorVisible(1)
    , mbDrawContextSetup(0)
    , mCursorShape(EMouseShape_Standard)
    , mWndState(EWindowState_Nomal)
    , mWidth(0)
    , mHeight(0)
    , mPosx(0)
    , mPosy(0)
    , mhInstance(0)
    , mhWnd(0)
    , mhDC(0)
    , mhGlRc(0)
    , mbHasMouseCaptured(0)
    , mnPressedButtons(0)
    , mCustomCursor(0)
{
}

EECode CIPlatformWindowWinOpenGL::Construct()
{
    return EECode_OK;
}

CIPlatformWindowWinOpenGL::~CIPlatformWindowWinOpenGL()
{
}

EECode CIPlatformWindowWinOpenGL::Initialize()
{
    mhInstance = GetModuleHandle(NULL); // Grab An Instance For Our Window
    mWndClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;   // Redraw On Size, And Own DC For Window.
    mWndClass.lpfnWndProc   = (WNDPROC) cbWndProc; // WndProc Handles Messages
    mWndClass.cbClsExtra    = 0;    // No Extra Window Data
    mWndClass.cbWndExtra    = 0;    // No Extra Window Data
    mWndClass.hInstance = mhInstance;  // Set The Instance
    mWndClass.hIcon = LoadIcon(NULL, IDI_WINLOGO);  // Load The Default Icon
    mWndClass.hCursor   = ::LoadCursor(NULL, IDC_ARROW);  // Load The Arrow Pointer
    mWndClass.hbrBackground = NULL; // No Background Required For GL
    mWndClass.lpszMenuName  = NULL; // We Don't Want A Menu
    mWndClass.lpszClassName = mpWindowClassName; // Set The Class Name

    if (!RegisterClass(&mWndClass)) {
        MessageBox(NULL, "Failed To Register The Window Class.", "ERROR", MB_OK | MB_ICONEXCLAMATION);
        return EECode_Error;    // Return FALSE
    }

    return EECode_OK;
}

void CIPlatformWindowWinOpenGL::DeInitialize()
{
    if (mhInstance == NULL) {
        MessageBox(NULL, "Warning, hInstance==NULL in CIPlatformWindowWinOpenGL::DeInitialize() .", "SHUTDOWN ERROR", MB_OK | MB_ICONINFORMATION);
        return;
    }
    if (!UnregisterClass(mpWindowClassName, mhInstance)) {
        MessageBox(NULL, "Could Not Unregister Class.", "SHUTDOWN ERROR", MB_OK | MB_ICONINFORMATION);
        mhInstance = NULL;  // Set hInstance To NULL
        return;
    }
    return;
}

EECode CIPlatformWindowWinOpenGL::CreateMainWindow(const TChar *name, TDimension posx, TDimension posy, TDimension width, TDimension height, EWindowState state)
{
    if (state != EWindowState_Fullscreen) {
        width += GetSystemMetrics(SM_CXSIZEFRAME) * 2;
        height += GetSystemMetrics(SM_CYSIZEFRAME) * 2 + GetSystemMetrics(SM_CYCAPTION);
        mhWnd = ::CreateWindow(
                    mpWindowClassName,  // pointer to registered class name
                    name,       // pointer to window name
                    WS_OVERLAPPEDWINDOW, //WS_POPUP,   // window style
                    posx,       // horizontal position of window
                    posy,       // vertical position of window
                    width,      // window width
                    height,     // window height
                    0,          // handle to parent or owner window
                    0,          // handle to menu or child-window identifier
                    ::GetModuleHandle(0),   // handle to application instance
                    0);         // pointer to window-creation data
    } else {
        mhWnd = ::CreateWindow(
                    mpWindowClassName,  // pointer to registered class name
                    name,           // pointer to window name
                    WS_POPUP | WS_MAXIMIZE, // window style
                    posx,           // horizontal position of window
                    posy,           // vertical position of window
                    width,          // window width
                    height,         // window height
                    0,                  // handle to parent or owner window
                    0,                  // handle to menu or child-window identifier
                    ::GetModuleHandle(0),   // handle to application instance
                    0);         // pointer to window-creation data
    }

    mWidth = width;
    mHeight = height;
    mPosx = posx;
    mPosy = posy;

    if (mhWnd) {
        // Store a pointer to this class in the window structure
        LONG result = ::SetWindowLong(mhWnd, GWL_USERDATA, (LONG)this);
        // Store the device context
        mhDC = ::GetDC(mhWnd);
        // Show the window
        TS32 nCmdShow;
        switch (state) {
            case EWindowState_Maxmize:
                nCmdShow = SW_SHOWMAXIMIZED;
                break;
            case EWindowState_Minmize:
                nCmdShow = SW_SHOWMINIMIZED;
                break;
            case EWindowState_Nomal:
            default:
                nCmdShow = SW_SHOWNORMAL;
                break;
        }
        if (DUnlikely(mhGlRc)) {
            removeDrawingContext();
        }
        installDrawingContext();
        ::ShowWindow(mhWnd, nCmdShow);
        // Force an initial paint of the window
        ::UpdateWindow(mhWnd);
    }

    return EECode_OK;
}

EECode CIPlatformWindowWinOpenGL::MoveMainWindow(TDimension posx, TDimension posy)
{
    LOG_NOTICE("CIPlatformWindowWinOpenGL::MoveMainWindow: TO DO\n");
    return EECode_OK;
}

EECode CIPlatformWindowWinOpenGL::ReSizeMainWindow(TDimension width, TDimension height)
{
    LOG_NOTICE("ReSizeMainWindow(%d, %d)\n", width, height);
    return EECode_OK;
}

EECode CIPlatformWindowWinOpenGL::ReSizeClientWindow(TDimension width, TDimension height)
{
    LOG_NOTICE("ReSizeClientWindow(%d, %d)\n", width, height);
    GLenum glerror = 0;

    glShadeModel(GL_FLAT);
    glerror = glGetError();
    if (DUnlikely(glerror)) {
        LOG_ERROR("[GL error]: ReSizeMainWindow, after glShadeModel: glerror %d\n", glerror);
        return EECode_GLError;
    }

    glClearColor(0.0f, 0.0f, 0.0f, 0.5f);
    glerror = glGetError();
    if (DUnlikely(glerror)) {
        LOG_ERROR("[GL error]: ReSizeMainWindow, after glClearColor: glerror %d\n", glerror);
        return EECode_GLError;
    }

    glEnable(GL_BLEND);
    glerror = glGetError();
    if (DUnlikely(glerror)) {
        LOG_ERROR("[GL error]: ReSizeMainWindow, after glEnable(GL_BLEND): glerror %d\n", glerror);
        return EECode_GLError;
    }

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glerror = glGetError();
    if (DUnlikely(glerror)) {
        LOG_ERROR("[GL error]: ReSizeMainWindow, after glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA): glerror %d\n", glerror);
        return EECode_GLError;
    }

    glEnable(GL_TEXTURE_2D);
    glerror = glGetError();
    if (DUnlikely(glerror)) {
        LOG_ERROR("[GL error]: ReSizeMainWindow, after glEnable(GL_TEXTURE_2D): glerror %d\n", glerror);
        return EECode_GLError;
    }

    glViewport(0, 0, width, height);
    glerror = glGetError();
    if (DUnlikely(glerror)) {
        LOG_ERROR("[GL error]: ReSizeMainWindow, after glViewport(): glerror %d\n", glerror);
        return EECode_GLError;
    }

    gluOrtho2D(-1.0, 1.0, -1.0, 1.0);
    glerror = glGetError();
    if (DUnlikely(glerror)) {
        LOG_ERROR("[GL error]: ReSizeMainWindow, after gluOrtho2D(): glerror %d\n", glerror);
        return EECode_GLError;
    }

    return EECode_OK;
}

void CIPlatformWindowWinOpenGL::DestroyMainWindow()
{
    if (mCustomCursor) {
        DestroyCursor(mCustomCursor);
        mCustomCursor = NULL;
    }

    if (mhGlRc) {
        removeDrawingContext();
    }

    if (mhDC) {
        ::ReleaseDC(mhWnd, mhDC);
        mhDC = 0;
    }
    if (mhWnd) {
        ::DestroyWindow(mhWnd);
        mhWnd = 0;
    }
}

EECode CIPlatformWindowWinOpenGL::GetWindowRect(SRect &rect)
{
    RECT wrect;
    ::GetWindowRect(mhWnd, &wrect);
    rect.pos_x = wrect.left;
    rect.pos_y = wrect.top;
    rect.width = wrect.right - wrect.left;
    rect.height = wrect.bottom - wrect.top;
    DASSERT(wrect.bottom > wrect.top);
    return EECode_OK;
}

EECode CIPlatformWindowWinOpenGL::GetClientRect(SRect &rect)
{
    RECT crect;
    ::GetClientRect(mhWnd, &crect);
    rect.pos_x = crect.left;
    rect.pos_y = crect.top;
    rect.width = crect.right - crect.left;
    rect.height = crect.bottom - crect.top;
    DASSERT(crect.bottom > crect.top);
    return EECode_OK;
}

EECode CIPlatformWindowWinOpenGL::SetClientWidth(TS32 width)
{
    SRect cBnds, wBnds;
    TS32 cx, cy;

    GetClientRect(cBnds);
    if ((cBnds.width) != width) {
        GetWindowRect(wBnds);
        cx = wBnds.width + width - cBnds.width;
        cy = wBnds.height;
        if (TRUE == SetWindowPos(mhWnd, HWND_TOP, 0, 0, cx, cy, SWP_NOMOVE | SWP_NOZORDER | SWP_NOSENDCHANGING)) {
            return EECode_OK;
        } else {
            LOG_ERROR("::SetWindowPos() fail\n");
            return EECode_OSError ;
        }
    }

    return EECode_OK;;
}


EECode CIPlatformWindowWinOpenGL::SetClientHeight(TS32 height)
{
    SRect cBnds, wBnds;
    TS32 cx, cy;

    GetClientRect(cBnds);
    if ((cBnds.height) != height) {
        GetWindowRect(wBnds);
        cx = wBnds.width;
        cy = wBnds.height + height - (cBnds.height);
        if (TRUE == ::SetWindowPos(mhWnd, HWND_TOP, 0, 0, cx, cy, SWP_NOMOVE | SWP_NOZORDER | SWP_NOSENDCHANGING)) {
            return EECode_OK;
        } else {
            LOG_ERROR("::SetWindowPos() fail\n");
            return EECode_OSError ;
        }

    }

    return EECode_OK;
}

EECode CIPlatformWindowWinOpenGL::SetClientSize(TDimension width, TDimension height)
{
    SRect cBnds, wBnds;
    TS32 cx, cy;

    GetClientRect(cBnds);
    LOG_NOTICE("client rect: cBnds.width %d, cBnds.height %d, cBnds.pos_x %d, cBnds.pos_y %d\n", cBnds.width, cBnds.height, cBnds.pos_x, cBnds.pos_y);
    if (((cBnds.width) != width) || ((cBnds.height) != height)) {
        GetWindowRect(wBnds);
        LOG_NOTICE("win rect: wBnds.width %d, wBnds.height %d, wBnds.pos_x %d, wBnds.pos_y %d\n", wBnds.width, wBnds.height, wBnds.pos_x, wBnds.pos_y);
        cx = wBnds.width + width - (cBnds.width);
        cy = wBnds.height + height - (cBnds.height);
        LOG_NOTICE("before SetWindowPos(%d, %d), width %d, height %d\n", cx, cy, width, height);
        if (TRUE == ::SetWindowPos(mhWnd, HWND_TOP, 0, 0, cx, cy, SWP_NOMOVE | SWP_NOZORDER | SWP_NOSENDCHANGING)) {
            return EECode_OK;
        } else {
            LOG_ERROR("::SetWindowPos() fail\n");
            return EECode_OSError ;
        }
    }

    return EECode_OK;
}

EWindowState CIPlatformWindowWinOpenGL::GetWindowState() const
{
    EWindowState state;
    if (::IsIconic(mhWnd)) {
        state = EWindowState_Minmize;
    } else if (::IsZoomed(mhWnd)) {
        state = EWindowState_Maxmize;
    }  else {
        state = EWindowState_Nomal;
    }
    return state;
}

EECode CIPlatformWindowWinOpenGL::SetWindowState(EWindowState state)
{
    WINDOWPLACEMENT wp;
    wp.length = sizeof(WINDOWPLACEMENT);
    ::GetWindowPlacement(mhWnd, &wp);
    switch (state) {
        case EWindowState_Minmize:
            wp.showCmd = SW_SHOWMINIMIZED;
            break;
        case EWindowState_Maxmize:
            ShowWindow(mhWnd, SW_HIDE); //fe. HACK!
            //Solves redraw problems when switching from fullscreen to normal.
            wp.showCmd = SW_SHOWMAXIMIZED;
            SetWindowLong(mhWnd, GWL_STYLE, WS_OVERLAPPEDWINDOW);
            break;
        case EWindowState_Fullscreen:
            wp.showCmd = SW_SHOWMAXIMIZED;
            SetWindowLong(mhWnd, GWL_STYLE, WS_POPUP | WS_MAXIMIZE);
            break;
        case EWindowState_Nomal:
        default:
            wp.showCmd = SW_SHOWNORMAL;
            break;
    }
    return ::SetWindowPlacement(mhWnd, &wp) == TRUE ? EECode_OK : EECode_Error ;
}

EECode CIPlatformWindowWinOpenGL::PreDrawing()
{
    GLenum glerror = glGetError();
    if (DUnlikely(glerror)) {
        LOG_ERROR("[GL error]: PreDrawing start: glerror %d\n", glerror);
        return EECode_GLError;
    }

    glClear(GL_COLOR_BUFFER_BIT);
    glerror = glGetError();
    if (DUnlikely(glerror)) {
        LOG_ERROR("[GL error]: PreDrawing: after glClear: glerror %d\n", glerror);
        return EECode_GLError;
    }

    return EECode_OK;
}

EECode CIPlatformWindowWinOpenGL::InitOrthoDrawing(TDimension width, TDimension height)
{
    GLenum glerror = 0;

    glShadeModel(GL_FLAT);
    glerror = glGetError();
    if (DUnlikely(glerror)) {
        LOG_ERROR("[GL error]: InitOrthoDrawing, after glShadeModel: glerror %d\n", glerror);
        return EECode_GLError;
    }

    glClearColor(0.0f, 0.0f, 0.0f, 0.5f);
    glerror = glGetError();
    if (DUnlikely(glerror)) {
        LOG_ERROR("[GL error]: InitOrthoDrawing, after glClearColor: glerror %d\n", glerror);
        return EECode_GLError;
    }

    glEnable(GL_BLEND);
    glerror = glGetError();
    if (DUnlikely(glerror)) {
        LOG_ERROR("[GL error]: InitOrthoDrawing, after glEnable(GL_BLEND): glerror %d\n", glerror);
        return EECode_GLError;
    }

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glerror = glGetError();
    if (DUnlikely(glerror)) {
        LOG_ERROR("[GL error]: InitOrthoDrawing, after glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA): glerror %d\n", glerror);
        return EECode_GLError;
    }

    glEnable(GL_TEXTURE_2D);
    glerror = glGetError();
    if (DUnlikely(glerror)) {
        LOG_ERROR("[GL error]: InitOrthoDrawing, after glEnable(GL_TEXTURE_2D): glerror %d\n", glerror);
        return EECode_GLError;
    }

    glViewport(0, 0, width, height);
    glerror = glGetError();
    if (DUnlikely(glerror)) {
        LOG_ERROR("[GL error]: InitOrthoDrawing, after glViewport(): glerror %d\n", glerror);
        return EECode_GLError;
    }

    gluOrtho2D(-1.0, 1.0, -1.0, 1.0);
    glerror = glGetError();
    if (DUnlikely(glerror)) {
        LOG_ERROR("[GL error]: InitOrthoDrawing, after gluOrtho2D(): glerror %d\n", glerror);
        return EECode_GLError;
    }

    return EECode_OK;
}

EECode CIPlatformWindowWinOpenGL::SwapBuffers()
{
    glFinish();

    GLenum glerror = glGetError();
    if (DUnlikely(glerror)) {
        LOG_ERROR("[GL error]: SwapBuffers, after glFinish(): glerror %d\n", glerror);
        return EECode_GLError;
    }

    BOOL ret = ::SwapBuffers(mhDC);
    glerror = glGetError();

    if (DUnlikely(glerror)) {
        LOG_ERROR("[GL error]: SwapBuffers, ::SwapBuffers(): glerror %d\n", glerror);
        return EECode_GLError;
    }

    if (DLikely(TRUE == ret)) {
        return EECode_OK;
    }

    LOG_ERROR("::SwapBuffers(mhDC) fail\n");
    return EECode_Error ;
}

EECode CIPlatformWindowWinOpenGL::ActivateDrawingContext()
{
    if (DLikely(mhDC && mhGlRc)) {
        BOOL ret = ::wglMakeCurrent(mhDC, mhGlRc);
        GLenum glerror = glGetError();
        if (DUnlikely(glerror)) {
            LOG_ERROR("[GL error]: ActivateDrawingContext, after ::wglMakeCurrent(): glerror %d\n", glerror);
            return EECode_GLError;
        }

        if (DLikely(TRUE == ret)) {
            return EECode_OK;
        }
        LOG_ERROR("::wglMakeCurrent() fail\n");
        return EECode_OSError ;
    }

    LOG_ERROR("ActivateDrawingContext(), NULL pointer\n");
    return EECode_InternalLogicalBug;
}

EECode CIPlatformWindowWinOpenGL::Invalidate()
{
    if (DLikely(mhWnd)) {
        TInt ret = ::InvalidateRect(mhWnd, 0, FALSE);
        if (DLikely(0 != ret)) {
            return EECode_OK;
        }
        LOG_ERROR("::InvalidateRect() fail\n");
        return EECode_OSError ;
    }

    LOG_ERROR("Invalidate(), NULL pointer\n");
    return EECode_InternalLogicalBug;
}

EECode CIPlatformWindowWinOpenGL::LoadCursor(TU8 visible, EMouseShape cursor) const
{
    if (!visible) {
        while (::ShowCursor(FALSE) >= 0);
    } else {
        while (::ShowCursor(TRUE) < 0);
    }

    if ((cursor == EMouseShape_Custom) && mCustomCursor) {
        ::SetCursor(mCustomCursor);
    } else {
        LPCSTR id;
        switch (cursor) {
            case EMouseShape_Standard:  id = IDC_ARROW;     break;
            case EMouseShape_Wait:          id = IDC_WAIT;      break;  // Hourglass
            case EMouseShape_Circle:        id = IDC_CROSS;     break;  // Crosshair
            default:
                LOG_FATAL("LoadCursor, bad type %d\n", cursor);
                return EECode_InternalLogicalBug;
                break;
        }

        HCURSOR hCursor = ::SetCursor(::LoadCursor(0, id));
    }

    return EECode_OK;
}

void CIPlatformWindowWinOpenGL::Destroy()
{
}

EECode CIPlatformWindowWinOpenGL::installDrawingContext()
{
    TS32 iPixelFormat = ::ChoosePixelFormat(mhDC, &sPreferredFormat);
    if (0 == iPixelFormat) {
        LOG_ERROR("::ChoosePixelFormat() fail?\n");
        return EECode_Error;
    }

    if (::SetPixelFormat(mhDC, iPixelFormat, &sPreferredFormat) == FALSE) {
        LOG_ERROR("::SetPixelFormat() fail?\n");
        return EECode_Error;
    }

    PIXELFORMATDESCRIPTOR preferredFormat;
    ::DescribePixelFormat(mhDC, iPixelFormat, sizeof(PIXELFORMATDESCRIPTOR), &preferredFormat);

    mhGlRc = ::wglCreateContext(mhDC);
    if (mhGlRc) {
        if (TRUE != ::wglMakeCurrent(mhDC, mhGlRc)) {
            LOG_ERROR("::wglMakeCurrent() fail?\n");
            return EECode_Error;
        }

        LOG_NOTICE("[flow]: before glewInit()\n");
        GLenum ret = glewInit();
        if (GLEW_OK != ret) {
            LOG_ERROR("[error]: glewInit() fail, ret %d\n", ret);
            return EECode_OSError;
        }
        LOG_NOTICE("[flow]: glewInit() done\n");
    } else {
        LOG_ERROR("::wglCreateContext() return NULL?\n");
        return EECode_Error;
    }

    return EECode_OK;
}

EECode CIPlatformWindowWinOpenGL::removeDrawingContext()
{
    if (DLikely(mhGlRc)) {
        if (FALSE == wglDeleteContext(mhGlRc)) {
            LOG_WARN("remove_drawingContext: wglDeleteContext fail?\n");
            return EECode_OSError;
        }
        mhGlRc = 0;
    } else {
        LOG_WARN("remove_drawingContext: context not setup yet\n");
    }

    return EECode_OK;
}

EECode CIPlatformWindowWinOpenGL::lostMouseCapture()
{
    if (mbHasMouseCaptured) {
        mbHasMouseCaptured = 0;
        mnPressedButtons = 0;
    }
    return EECode_OK;
}

EECode CIPlatformWindowWinOpenGL::registerMouseClickEvent(TU8 press)
{
    if (press) {
        if (!mbHasMouseCaptured) {
            ::SetCapture(mhWnd);
            mbHasMouseCaptured = 1;
        }
        mnPressedButtons++;
    } else {
        if (mnPressedButtons) {
            mnPressedButtons--;
            if (!mnPressedButtons) {
                ::ReleaseCapture();
                mbHasMouseCaptured = 0;
            }
        }
    }

    return EECode_OK;
}

EECode CIPlatformWindowWinOpenGL::setWindowCursorVisibility(TU8 visible)
{
    if (::GetForegroundWindow() == mhWnd) {
        LoadCursor(visible, mCursorShape);
    }
    return EECode_OK;
}

EECode CIPlatformWindowWinOpenGL::setWindowCursorShape(EMouseShape cursorShape)
{
    if (mCustomCursor) {
        DestroyCursor(mCustomCursor);
        mCustomCursor = NULL;
    }

    if (::GetForegroundWindow() == mhWnd) {
        LoadCursor(mbCursorVisible, cursorShape);
    }

    return EECode_OK;
}

EECode CIPlatformWindowWinOpenGL::setWindowCustomCursorShape(TU8 *bitmap, TU8 *mask, TS32 hotX, TS32 hotY)
{
    return setWindowCustomCursorShape((TU8 *)bitmap, (TU8 *)mask, 16, 16, hotX, hotY, 0, 1);
}

EECode CIPlatformWindowWinOpenGL::setWindowCustomCursorShape(TU8 *bitmap, TU8 *mask, TS32 sizeX, TS32 sizeY, TS32 hotX, TS32 hotY, TS32 fg_color, TS32 bg_color)
{
    TU32 andData[32];
    TU32 xorData[32];
    TU32 fullBitRow, fullMaskRow;
    TS32 x, y, cols;

    cols = sizeX / 8; /* Num of whole bytes per row (width of bm/mask) */
    if (sizeX % 8) {
        cols++;
    }

    if (mCustomCursor) {
        DestroyCursor(mCustomCursor);
        mCustomCursor = NULL;
    }

    memset(&andData, 0xFF, sizeof(andData));
    memset(&xorData, 0, sizeof(xorData));

    for (y = 0; y < sizeY; y++) {
        fullBitRow = 0;
        fullMaskRow = 0;
        for (x = cols - 1; x >= 0; x--) {
            fullBitRow <<= 8;
            fullMaskRow <<= 8;
            fullBitRow  |= uns8ReverseBits(bitmap[cols * y + x]);
            fullMaskRow |= uns8ReverseBits(mask[cols * y + x]);
        }
        xorData[y] = fullBitRow & fullMaskRow;
        andData[y] = ~fullMaskRow;
    }

    mCustomCursor = ::CreateCursor(::GetModuleHandle(0), hotX, hotY, 32, 32, andData, xorData);
    if (!mCustomCursor) {
        return EECode_OK;
    }

    if (::GetForegroundWindow() == mhWnd) {
        LoadCursor(mbCursorVisible, EMouseShape_Custom);
    }

    return EECode_OK;
}

EECode CIPlatformWindowWinOpenGL::screenToClient(TS32 inX, TS32 inY, TS32 &outX, TS32 &outY) const
{
    POINT point = { inX, inY };
    ::ScreenToClient(mhWnd, &point);
    outX = point.x;
    outY = point.y;
    return EECode_OK;
}

EECode CIPlatformWindowWinOpenGL::clientToScreen(TS32 inX, TS32 inY, TS32 &outX, TS32 &outY) const
{
    POINT point = { inX, inY };
    ::ClientToScreen(mhWnd, &point);
    outX = point.x;
    outY = point.y;
    return EECode_OK;
}

LRESULT WINAPI CIPlatformWindowWinOpenGL::cbWndProc(HWND hwnd, TU32 msg, WPARAM wParam, LPARAM lParam)
{
    SMSG msg1;

    switch (msg) {
        case WM_ACTIVATE: {
                msg1.code = EMSGType_WindowActive;
                if ((LOWORD(wParam) != WA_INACTIVE) && !((BOOL)HIWORD(wParam))) {
                    msg1.flag = 1;
                } else {
                    msg1.flag = 0;
                }
                if (mpEventQueue) {
                    mpEventQueue->PostMsg(&msg1, sizeof(SMSG));
                    return 0;
                }
                break;
            }

        case WM_SYSCOMMAND: {
                switch (wParam)  {
                    case SC_SCREENSAVE:
                    case SC_MONITORPOWER:
                        return 0;
                }
                break;
            }

        case WM_CLOSE: {
                //PostQuitMessage(0);// Send A Quit Message
                msg1.code = EMSGType_WindowClose;
                if (mpEventQueue) {
                    mpEventQueue->PostMsg(&msg1, sizeof(SMSG));
                    return 0;
                }
                break;
            }

        case WM_KEYDOWN: {
                msg1.code = EMSGType_WindowKeyPress;
                msg1.p1 = wParam;
                if (mpEventQueue) {
                    mpEventQueue->PostMsg(&msg1, sizeof(SMSG));
                }
                return 0;
            }

        case WM_KEYUP: {
                msg1.code = EMSGType_WindowKeyRelease;
                msg1.p1 = wParam;
                if (mpEventQueue) {
                    mpEventQueue->PostMsg(&msg1, sizeof(SMSG));
                }
                return 0;
            }

        case WM_SIZE: {
                msg1.code = EMSGType_WindowSize;
                msg1.p1 = (LOWORD(lParam));
                msg1.p2 = (HIWORD(lParam));
                if (mpEventQueue) {
                    mpEventQueue->PostMsg(&msg1, sizeof(SMSG));
                }
                return 0;
            }
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

EECode CIPlatformWindowWinOpenGL::ProcessEvent()
{
    MSG msg;
    if (::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        ::TranslateMessage(&msg);
        ::DispatchMessage(&msg);
    }
    return EECode_OK;
}

TULong CIPlatformWindowWinOpenGL::GetWindowHandle() const
{
    return (TULong) mhWnd;
}

DCONFIG_COMPILE_OPTION_CPPFILE_IMPLEMENT_END


