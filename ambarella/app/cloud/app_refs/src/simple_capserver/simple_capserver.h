/**
 * simple_capserver.h
 *
 * History:
 *    2015/01/05 - [Zhi He] create file
 *
 * Copyright (C) 2014 - 2024, the Ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the Ambarella Inc.
 */

#ifndef __SIMPLE_CAPSERVER_H__
#define __SIMPLE_CAPSERVER_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN
DCODE_DELIMITER;

class CISimpleCapServer : public ISimpleCapServer
{
protected:
    CISimpleCapServer();
    virtual ~CISimpleCapServer();

public:
    static CISimpleCapServer *Create();

public:
    virtual void Destroy();
    virtual int Initialize();

public:
    virtual int Start();
    virtual int Stop();

public:
    virtual void Mute();
    virtual void UnMute();

private:
    void loadCurrentConfig(const char *configfile);
    void storeCurrentConfig(const char *configfile);

    EECode checkWithSystemSettings();
    EECode loadAudioDevices();

    EECode setupMediaAPI();
    void destroyMediaAPI();

private:
    SMediaSimpleAPIContext mMediaAPIContxt;
    SSystemSoundInputDevices mSystemSoundInputDevices;

    CMediaSimpleAPI *mpMediaSimpleAPI;

    TU8 mCurrentSoundInputDeviceIndex;
    TU8 mbSetupMediaContext;
    TU8 mbRunning;
    TU8 mbStarted;
};

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif

