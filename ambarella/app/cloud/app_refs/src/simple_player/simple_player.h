/**
 * simple_player.h
 *
 * History:
 *    2015/01/09 - [Zhi He] create file
 *
 * Copyright (C) 2014 - 2024, the Ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the Ambarella Inc.
 */

#ifndef __SIMPLE_PLAYER_H__
#define __SIMPLE_PLAYER_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN
DCODE_DELIMITER;

class CISimplePlayer: public ISimplePlayer
{
protected:
    CISimplePlayer();
    virtual ~CISimplePlayer();

public:
    static CISimplePlayer *Create();

public:
    virtual void Destroy();
    virtual int Initialize(void *owner = NULL, void *p_external_msg_queue = NULL);

public:
    virtual int Play(char *url, char *title_name = NULL);

public:
    void MainLoop();

private:
    EECode setupContext();
    EECode startContext();
    EECode stopContext();
    void destroyContext();

private:
    EECode processMsgLoop();
    EECode waitPlay();

private:
    int loadCurrentConfig(const char *configfile);
    int storeCurrentConfig(const char *configfile);
    int checkWithSystemSettings();

private:
    SMediaSimpleAPIContext mMediaAPIContxt;
    IThread *mpMainThread;
    CIQueue *mpMsgQueue;
    CIQueue *mpExternalMsgQueue;
    CMediaSimpleAPI *mpMediaAPI;
    IGenericEngineControlV4 *mpMediaEngineAPI;
    IClockSource *mpClockSource;
    CIClockManager *mpClockManager;
    CIClockReference *mpClockReference;
    ISceneDirector *mpDirector;
    CIScene *mpScene;
    CIGUILayer *mpLayer;
    IVisualDirectRendering *mpVisualDirectRendering;
    CGUIArea *mpTexture;

    ISoundComposer *mpSoundComposer;
    CISoundTrack *mpSoundTrack;
    ISoundDirectRendering *mpSoundDirectRendering;

    int mbSetupContext;
    int mbStartContext;
    int mbThreadStarted;
    int mbReconnecting;
    int mbExit;

private:
    TDimension mPbMainWindowWidth;
    TDimension mPbMainWindowHeight;

    char mPreferDisplay[DMaxPreferStringLen];
    char mPreferPlatform[DMaxPreferStringLen];

private:
    char mWindowTitleName[256];
    void *mpOwner;
};

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif

