/**
 * win_opengl.h
 *
 * History:
 *  2014/11/09 - [Zhi He] create file
 *
 * Copyright (C) 2014 - 2024, the Ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the Ambarella Inc.
 */

#ifndef __WIN_OPENGL_H__
#define __WIN_OPENGL_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN
DCODE_DELIMITER;

//-----------------------------------------------------------------------
//
// CIPlatformWindowWinOpenGL
//
//-----------------------------------------------------------------------

class CIPlatformWindowWinOpenGL: public IPlatformWindow
{
public:
    static CIPlatformWindowWinOpenGL *Create(CIQueue *p_msg_queue);

protected:
    CIPlatformWindowWinOpenGL();

    EECode Construct();
    virtual ~CIPlatformWindowWinOpenGL();

public:
    virtual EECode Initialize();
    virtual void DeInitialize();
    virtual EECode CreateMainWindow(const TChar *name, TDimension posx, TDimension posy, TDimension width, TDimension height, EWindowState state);
    virtual EECode MoveMainWindow(TDimension posx, TDimension posy);
    virtual EECode ReSizeMainWindow(TDimension width, TDimension height);
    virtual EECode ReSizeClientWindow(TDimension width, TDimension height);
    virtual void DestroyMainWindow();

public:
    virtual EECode GetWindowRect(SRect &rect);
    virtual EECode GetClientRect(SRect &rect);

    virtual EECode SetClientWidth(TS32 width);
    virtual EECode SetClientHeight(TS32 height);
    virtual EECode SetClientSize(TDimension width, TDimension height);

    virtual EWindowState GetWindowState() const;
    virtual EECode SetWindowState(EWindowState state);

    virtual EECode PreDrawing();
    virtual EECode InitOrthoDrawing(TDimension width, TDimension height);
    virtual EECode SwapBuffers();
    virtual EECode ActivateDrawingContext();
    virtual EECode Invalidate();
    virtual EECode LoadCursor(TU8 visible, EMouseShape cursor) const;

    virtual EECode ProcessEvent();
    virtual TULong GetWindowHandle() const;

public:
    virtual void Destroy();

private:
    static LRESULT WINAPI CIPlatformWindowWinOpenGL::cbWndProc(HWND hwnd, TU32 msg, WPARAM wParam, LPARAM lParam);

private:
    EECode installDrawingContext();
    EECode removeDrawingContext();
    EECode lostMouseCapture();
    EECode registerMouseClickEvent(TU8 press);
    EECode setWindowCursorVisibility(TU8 visible);
    EECode setWindowCursorShape(EMouseShape cursorShape);
    EECode setWindowCustomCursorShape(TU8 *bitmap, TU8 *mask, TS32 hotX, TS32 hotY);
    EECode setWindowCustomCursorShape(TU8 *bitmap, TU8 *mask, TS32 sizeX, TS32 sizeY, TS32 hotX, TS32 hotY, TS32 fg_color, TS32 bg_color);
    EECode screenToClient(TS32 inX, TS32 inY, TS32 &outX, TS32 &outY) const;
    EECode clientToScreen(TS32 inX, TS32 inY, TS32 &outX, TS32 &outY) const;
    static LRESULT WINAPI mfWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

public:
    //LPCSTR  mpClassName;
    static LPCSTR mpWindowClassName;
    static CIQueue *mpEventQueue;
    //static TU8 mbActive;
    //static TU8 mbKeys[DMaxKeyNum];

private:
    TU8 mbCursorVisible;
    TU8 mbDrawContextSetup;
    EMouseShape mCursorShape;
    EWindowState mWndState;

    TDimension mWidth;
    TDimension mHeight;
    TDimension mPosx;
    TDimension mPosy;

private:
    WNDCLASS    mWndClass; // Windows Class Structure
    HINSTANCE   mhInstance;    //Grab An Instance For Our Window

    /** Window handle. */
    HWND mhWnd;
    /** Device context handle. */
    HDC mhDC;
    /** OpenGL rendering context. */
    HGLRC mhGlRc;
    /** Flag for if window has captured the mouse */
    TU8 mbHasMouseCaptured;

    /** Count of number of pressed buttons */
    TInt mnPressedButtons;
    /** HCURSOR structure of the custom cursor */
    HCURSOR mCustomCursor;

    //static LPCSTR msWindowClassName;
    //static const TU32 mMaxTitleLength;
};

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif

