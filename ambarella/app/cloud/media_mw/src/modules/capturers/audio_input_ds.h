/**
 * audio_input_ds.h
 *
 * History:
 *    2014/11/03 - [Zhi He] create file
 *
 * Copyright (C) 2014 - 2024, the Ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the Ambarella Inc.
 */

#ifndef __AUDIO_INPUT_DS_H__
#define __AUDIO_INPUT_DS_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN
DCODE_DELIMITER;

#define DDS_READY_EVENTS_NUM 8
#define DDS_MAX_DEVICE_NUM 4

class CAudioInputDS: public CObject, public IAudioInput, public IActiveObject
{
    typedef CObject inherited;

protected:
    CAudioInputDS(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink, void *pDev);
    EECode Construct();
    virtual ~CAudioInputDS();

public:
    static IAudioInput *Create(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink, void *pDev);
    virtual CObject *GetObject() const;
    virtual void Destroy();

public:
    virtual EECode SetupContext(SAudioParams *param);
    virtual EECode DestroyContext();
    virtual EECode SetupOutput(COutputPin *p_output_pin, CBufferPool *p_bufferpool, IMsgSink *p_msg_sink);

    virtual TUint GetChunkSize();
    virtual TUint GetBitsPerFrame();

    virtual EECode Start(TUint index = 0);
    virtual EECode Stop(TUint index = 0);
    virtual EECode Flush(TUint index = 0);

    virtual EECode Read(TU8 *pData, TUint dataSize, TUint *pNumFrames, TU64 *pTimeStamp, TUint index = 0);

    virtual EECode Pause(TUint index = 0);
    virtual EECode Resume(TUint index = 0);

    virtual EECode Mute();
    virtual EECode UnMute();

public:
    virtual void OnRun();

public:
    EECode InsertSoundDevice(GUID *id, TCHAR const *p_desc);

private:
    EECode openDevice();
    void processCmd(SCMD &cmd);
    void addBufferFlags(CIBuffer *p_buffer);

private:
    const volatile SPersistMediaConfig *mpPersistMediaConfig;
    IMsgSink *mpMsgSink;
    CIWorkQueue *mpWorkQueue;

    COutputPin *mpOutputPin;
    CBufferPool *mpBufferPool;

    TU32 msState;
    TU8 mbRunning;
    TU8 mbSpecifiedDevice;
    TU8 mReserved1;
    TU8 mReserved2;

    TU64 mFrameCount;
    TTime mPTS;
    TTime mDuration;

private:
    IDirectSoundCapture   *mpCapture;
    IDirectSoundCaptureBuffer  *mpCaptureBuffer;
    GUID            *mpSpecifiedGUID;
    GUID            mCaptureGUID[DDS_MAX_DEVICE_NUM];
    const TChar *mCaptureDescription[DDS_MAX_DEVICE_NUM];
    TU32 mCaptureNumber;

    WAVEFORMATEX mWaveFormatEX;
    HANDLE      mReadyEvents[DDS_READY_EVENTS_NUM];
    DSBPOSITIONNOTIFY mPosNotify[DDS_READY_EVENTS_NUM];
    IDirectSoundNotify8 *mpNotify;

    DSCBUFFERDESC mBufferDescription;

private:
    DWORD mBlockSize;
    DWORD mBlockCount;

    DWORD   mOffset;

};
DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif

