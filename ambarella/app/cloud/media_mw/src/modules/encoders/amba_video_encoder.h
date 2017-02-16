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

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN
DCODE_DELIMITER;

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

    IDSPEncAPI *mpDSPEncoder;
    TUint mEncoderIndex;

    EDSPOperationMode mDSPMode;
    EDSPPlatform mDSPPlatform;
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
    TU8 mbEncoderStarted;
    TU8 mRemainingDescNumber;
    TU8 mCurrentDescIndex;
    TU8 mbCopyOut;

    TU8 mbSendSyncBuffer;
    TU8 mReserved0;
    TU8 mReserved1;
    TU8 mReserved2;

private:
    TU8 *mpVideoExtraData;
    TUint mVideoExtraDataSize;
    TU8 *mpH264spspps;
    TUint mH264spsppsSize;

private:
    FILE *mpDumpFile;
    FILE *mpDumpFileSeparate;
    TUint mDumpIndex;
    TUint mDumpStartFrame, mDumpEndFrame;

private:
    SBitDescs mCurrentDesc;
};
DCONFIG_COMPILE_OPTION_HEADERFILE_END
#endif

