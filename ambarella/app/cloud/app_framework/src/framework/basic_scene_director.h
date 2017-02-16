/**
 * basic_scene_director.h
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

