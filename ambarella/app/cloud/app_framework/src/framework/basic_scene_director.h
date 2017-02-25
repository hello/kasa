/*******************************************************************************
 * basic_scene_director.h
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

#ifndef __BASIC_SCENE_DIRECTOR_H__
#define __BASIC_SCENE_DIRECTOR_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN
DCODE_DELIMITER;

//-----------------------------------------------------------------------
//
// CBasicSceneDirector
//
//-----------------------------------------------------------------------

class CBasicSceneDirector: public CObject, public ISceneDirector, public IActiveObject
{
    typedef CObject inherited;
public:
    static CBasicSceneDirector *Create(EVisualSettingPrefer visual_platform_prefer, SPersistCommonConfig *p_common_config, SSharedComponent *p_shared_component);
    virtual CObject *GetObject() const;
    virtual void Delete();

protected:
    CBasicSceneDirector(EVisualSettingPrefer visual_platform_prefer, SPersistCommonConfig *p_common_config, SSharedComponent *p_shared_component);

    EECode Construct();
    virtual ~CBasicSceneDirector();

public:
    virtual EECode AddScene(CIScene *scene);
    virtual void RemoveScene(CIScene *scene);

public:
    virtual EECode SwitchScene(CIScene *scene);

public:
    virtual EECode StartRunning();
    virtual EECode ExitRunning();

public:
    virtual void OnRun();

public:
    virtual EECode PreferMainWindowSetting();
    virtual EECode PreferVisualSetting(EVisualSetting setting = EVisualSetting_opengl);
    virtual EECode PreferAudioSetting();

public:
    virtual EECode CreateMainWindow(const TChar *name, TDimension posx, TDimension posy, TDimension width, TDimension height, EWindowState state);
    virtual EECode MoveMainWindow(TDimension posx, TDimension posy);
    virtual EECode ReSizeMainWindow(TDimension width, TDimension height);
    virtual EECode ReSizeClientWindow(TDimension width, TDimension height);
    virtual TULong GetWindowHandle() const;

public:
    virtual void Destroy();

private:
    EECode renderScenes();
    TU32 needReDraw();
    void processCmd(SCMD &cmd);
    EECode setupComponentContext();
    EECode prepareRendering();
    EECode createMainWindow();
    EECode moveMainWindow(TDimension posx, TDimension posy);
    EECode resizeMainWindow(TDimension width, TDimension height);
    EECode resizeClientWindow(TDimension width, TDimension height);

private:
    CIWorkQueue *mpWorkQueue;
    SPersistCommonConfig *mpCommonConfig;
    CIDoubleLinkedList *mpSceneList;

    CIClockManager *mpClockManager;
    CIClockReference *mpClockReference;

    CIQueue *mpMsgQueue;
    IPlatformWindow *mpPlatformWindow;

    //input
    const TChar *mpMainWindowName;
    TDimension mRequestMainWindowPosX;
    TDimension mRequestMainWindowPosY;
    TDimension mRequestMainWindowWdith;
    TDimension mRequestMainWindowHeight;
    EWindowState mRequestMainWindowState;

    TU8 mbLoop;
    TU8 mbDrawFirstFrame;
    TU8 mReserved1, mReserved2;

    EVisualSetting mVisualSetting;
    EVisualSettingPrefer mVisualSettingPrefer;
    ESceneDirectorState msState;

private:
    TTime mGlobalBeginTime;

    TTime mCurBeginTime;
    TTime mCurDuration;
    TTime mCurTime;

    TU64 mGlobalTicks;
    TU64 mCurTicks;
};

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif

