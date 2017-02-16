/**
 * freeglut_wrapper.h
 *
 * History:
 *  2014/11/17 - [Zhi He] create file
 *
 * Copyright (C) 2014 - 2024, the Ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the Ambarella Inc.
 */

#ifndef __FREEGLUT_WRAPPER_H__
#define __FREEGLUT_WRAPPER_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN
DCODE_DELIMITER;

//-----------------------------------------------------------------------
//
// CIPlatformWindowFreeGLUT
//
//-----------------------------------------------------------------------

class CIPlatformWindowFreeGLUT: public IPlatformWindow
{
public:
    static CIPlatformWindowFreeGLUT *Create(CIQueue *p_msg_queue);

protected:
    CIPlatformWindowFreeGLUT();

    EECode Construct();
    virtual ~CIPlatformWindowFreeGLUT();

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

public:
    static CIQueue *mpEventQueue;
    static TU8 mbActive;
    static TU8 mbKeys[DMaxKeyNum];

private:
    TU8 mbCursorVisible;
    TU8 mbDrawContextSetup;
    EMouseShape mCursorShape;
    EWindowState mWndState;

    TDimension mWidth;
    TDimension mHeight;
    TDimension mPosx;
    TDimension mPosy;
};

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif

