/*
 * amba_video_encoder.h
 *
 * History:
 *    2012/08/03 - [Zhi He] create file
 *
 * Copyright (C) 2014 - 2024, the Ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the Ambarella Inc.
 */

#ifndef __AMBA_VIDEO_ENCODER_H__
#define __AMBA_VIDEO_ENCODER_H__

class CAmbaVideoEncoder
    : public CObject
    , virtual public IVideoEncoder
{
    typedef CObject inherited;

protected:
    CAmbaVideoEncoder(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink);
    virtual ~CAmbaVideoEncoder();

public:
    static IVideoEncoder *Create(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink);
    virtual void Destroy();

public:
    virtual void PrintStatus();

public:
    virtual EECode SetupContext(SVideoEncoderInputParam *param);
    virtual void DestroyContext();
    virtual EECode SetBufferPool(IBufferPool *p_buffer_pool);
    virtual EECode Start();
    virtual EECode Stop();

    virtual EECode Encode(CIBuffer *in_buffer, CIBuffer *out_buffer, TU32 &current_remaining, TU32 &all_cached_frames);

    virtual EECode VideoEncoderProperty(EFilterPropertyType property_type, TUint set_or_get, void *p_param);

    void Delete();

private:
    EECode Construct();
    void dumpEsData(TU8 *pStart, TU8 *pEnd);

private:
    const volatile SPersistMediaConfig *mpPersistMediaConfig;
    IMsgSink *mpMsgSink;

    TInt mIavFd;
    SAmbaDSPMapBSB mMapBSB;
    SFAmbaDSPDecAL mfDSPAL;

    StreamFormat mCodecFormat;
    float mFrameRate;

    TUint mBitrate;
    TUint mFrameRateReduceFactor;

private:
    TU8 *mpBitstreamBufferStart;
    TU32 mBitstreamBufferSize;

private:
    TUint mCapMaxCodedWidth, mCapMaxCodedHeight;
    TUint mCurrentFramerateNum, mCurrentFramerateDen;

private:
    TU8 mStreamID;
    TU8 mRemainingDescNumber;
    TU8 mCurrentDescIndex;
    TU8 mbCopyOut;

    TU8 mbSendSyncBuffer;
    TU8 mbSendExtrabuffer;
    TU8 mbWaitFirstKeyframe;
    TU8 mbRemaingData;

private:
    TU8 *mpVideoExtraData;
    TUint mVideoExtraDataSize;

    TU8 *mpRemainingData;
    TUint mnRemainingDataSize;
    TTime mRemainBufferTimestamp;

private:
    TTime mPreviousTimestamp;
};
#endif

