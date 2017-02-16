/*
 * linuxfb_video_renderer.h
 *
 * History:
 *    2015/09/14 - [Zhi He] create file
 *
 * Copyright (C) 2014 - 2024, the Ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the Ambarella Inc.
 */

#ifndef __LINUXFB_VIDEO_RENDERER_H__
#define __LINUXFB_VIDEO_RENDERER_H__

class CLinuxFBVideoRenderer
    : public CObject
    , public IVideoRenderer
{
    typedef CObject inherited;

protected:
    CLinuxFBVideoRenderer(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink);
    virtual ~CLinuxFBVideoRenderer();

public:
    static IVideoRenderer *Create(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink);
    virtual void Destroy();

public:
    virtual EECode SetupContext(SVideoParams *param = NULL);
    virtual EECode DestroyContext();

    virtual EECode Start(TUint index = 0);
    virtual EECode Stop(TUint index = 0);
    virtual EECode Flush(TUint index = 0);

    virtual EECode Render(CIBuffer *p_buffer, TUint index = 0);

    virtual EECode Pause(TUint index = 0);
    virtual EECode Resume(TUint index = 0);
    virtual EECode StepPlay(TUint index = 0);

    virtual EECode SyncTo(TTime pts, TUint index = 0);
    virtual EECode QueryLastShownTimeStamp(TTime &pts, TUint index = 0);

private:
    EECode Construct();
private:
    const volatile SPersistMediaConfig *mpPersistMediaConfig;
    IMsgSink *mpMsgSink;

private:
    TInt mFd;
    TU32 mWidth;
    TU32 mHeight;
    TU32 mLinesize;
    TU32 mBufferSize;
    TU32 mBitsPerPixel;
    TU8 *mpBuffers;
    TU8 *mpBuffer[4];

    TU32 mSrcOffsetX;
    TU32 mSrcOffsetY;
    TU32 mSrcWidth;
    TU32 mSrcHeight;
    TTime mLastDisplayPTS;

    TU32 mTotalDisplayFrameCount;
    TTime mFirstFrameTime;
    TTime mCurFrameTime;

    TU32 *mpScaleIndexX;
    TU32 *mpScaleIndexY;

    TU8 mbScaled;
    TU8 mFrameCount;
    TU8 mCurFrameIndex;
    TU8 mbYUV16BitMode;

private:
    struct  fb_var_screeninfo    mVinfo;
    struct  fb_fix_screeninfo   mFinfo;
};

#endif

