/*
 * amba_video_decoder.h
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

#ifndef __AMBA_VIDEO_DECODER_H__
#define __AMBA_VIDEO_DECODER_H__

class CAmbaVideoDecoder
    : public CObject
    , public IVideoDecoder
{
    typedef CObject inherited;

protected:
    CAmbaVideoDecoder(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink);
    virtual ~CAmbaVideoDecoder();
    EECode Construct();

public:
    static IVideoDecoder *Create(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink);
    virtual CObject *GetObject0() const;

public:
    virtual void PrintStatus();

public:
    virtual EECode SetupContext(SVideoDecoderInputParam *param);
    virtual EECode DestroyContext();

    virtual EECode SetBufferPool(IBufferPool *buffer_pool);

    virtual EECode Start();
    virtual EECode Stop();

    virtual EECode Decode(CIBuffer *in_buffer, CIBuffer *&out_buffer);
    virtual EECode Flush();

    virtual EECode Suspend();

    void Delete();

    virtual EECode SetExtraData(TU8 *p, TMemSize size);

    virtual EECode SetPbRule(TU8 direction, TU8 feeding_rule, TU16 speed);

    virtual EECode SetFrameRate(TUint framerate_num, TUint frameate_den);

    virtual EDecoderMode GetDecoderMode() const;

    //direct mode
    virtual EECode DecodeDirect(CIBuffer *in_buffer);
    virtual EECode SetBufferPoolDirect(IOutputPin *p_output_pin, IBufferPool *p_bufferpool);

private:
    TU8 *copyDataToBSB(TU8 *ptr, TU8 *buffer, TUint size);
    EECode decodeH264(CIBuffer *in_buffer);

    EECode enterDecodeMode();
    EECode leaveDecodeMode();
    EECode createDecoder(TU8 decoder_id, TU8 decoder_type, TU32 width, TU32 height);
    EECode destroyDecoder();

private:
    const volatile SPersistMediaConfig *mpPersistMediaConfig;
    IMsgSink *mpMsgSink;

    SStreamCodecInfo mStreamCodecInfo;
    StreamFormat mCodecFormat;

    TInt mIavFd;
    SFAmbaDSPDecAL mfDSPAL;

    SAmbaDSPMapBSB mMapBSB;
    SAmbaDSPDecodeModeConfig mModeConfig;
    SAmbaDSPVoutInfo mVoutInfos[DAMBADSP_MAX_DECODE_VOUT_NUMBER];
    SAmbaDSPDecoderInfo mDecoderInfo;

private:
    TUint mCapMaxCodedWidth, mCapMaxCodedHeight;

private:
    TU8 mbAddAmbaGopHeader;
    TU8 mDecId;
    TU8 mbBWplayback;
    TU8 mVoutNumber;

    TU8 mbEnableVoutDigital;
    TU8 mbEnableVoutHDMI;
    TU8 mbEnableVoutCVBS;
    TU8 mFeedingRule;

private:
    TUint mSpecifiedTimeScale;
    TUint mSpecifiedFrameTick;

private:
    TU8 *mpBitSreamBufferStart;
    TU8 *mpBitSreamBufferEnd;
    TU8 *mpBitStreamBufferCurPtr;

private:
    TU32 mFrameRateNum;
    TU32 mFrameRateDen;
    TU32 mFrameRate;

private:
    TU8 *mpVideoExtraData;
    TMemSize mVideoExtraDataSize;

private:
    TU8 mpAmbaGopHeader[DAMBA_GOP_HEADER_LENGTH + 2];

};

#endif

