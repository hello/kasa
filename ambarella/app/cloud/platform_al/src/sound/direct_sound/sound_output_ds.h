/**
 * sound_output_ds.h
 *
 * History:
 *    2014/11/20 - [Zhi He] create file
 *
 * Copyright (C) 2014 - 2024, the Ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the Ambarella Inc.
 */

#ifndef __SOUND_OUTPUT_DS_H__
#define __SOUND_OUTPUT_DS_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN
DCODE_DELIMITER;

#define DDS_DATA_BLOCK_NUM 40

class CSoundOutputDS: public CObject, public ISoundOutput
{
    typedef CObject inherited;

protected:
    CSoundOutputDS(SPersistCommonConfig *pPersistCommonConfig);
    EECode Construct();
    virtual ~CSoundOutputDS();

public:
    static ISoundOutput *Create(SPersistCommonConfig *pPersistCommonConfig);
    virtual void Destroy();

public:
    virtual EECode SetupContext(SSoundConfigure *config, TULong handle);
    virtual void DestroyContext();

public:
    virtual EECode Start();
    virtual EECode Stop();

public:
    virtual EECode WriteSoundData(TU8 *p, TU32 size);
    virtual EECode QueryBufferSize(TULong &mem_size) const;
    virtual EECode QueryCurrentPosition(TULong &write_offset, TULong &play_offset);
    virtual EECode SetCurrentPlayPosition(TULong play_offset);

private:
    EECode setupContext(TULong handle);
    EECode writeSoundData(TU8 *p, TU32 size);

private:
    TU8 mbStarted;
    TU8 mbContextSetup;
    TU8 mReserved1;
    TU8 mReserved2;

private:
    DWORD mBlockSize;
    DWORD mBufferSize;
    DWORD mCurOffset;

private:
    SPersistCommonConfig *mpPersistCommonConfig;
    SSoundConfigure mSoundConfig;

private:
    IDirectSound8 *mpDS;
    IDirectSoundBuffer8 *mpDSBuffer8;
    IDirectSoundBuffer *mpDSBuffer;
    //DSBPOSITIONNOTIFY mPosNotify[DDS_READY_EVENTS_NUM];
    //IDirectSoundNotify8* mpNotify;
    //HANDLE      mReadyEvents[DDS_READY_EVENTS_NUM];

    DSBUFFERDESC mDSBufferDesc;
    WAVEFORMATEX mWaveFormatEX;
};

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif

