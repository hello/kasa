/*******************************************************************************
 * freeglut_wrapper.h
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

