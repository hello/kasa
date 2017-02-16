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

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN
DCODE_DELIMITER;

class CAmbaVideoDecoder
    : public CObject
    , virtual public IVideoDecoder
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

    virtual EECode QueryFreeZoom(TUint &free_zoom, TU32 &fullness);
    virtual EECode QueryLastDisplayedPTS(TTime &pts);
    virtual EECode PushData(CIBuffer *in_buffer, TInt flush);
    virtual EECode PullDecodedFrame(CIBuffer *out_buffer, TInt block_wait, TInt &remaining);

    virtual EECode TuningPB(TU8 fw, TU16 frame_tick);

private:
    TU8 *copyDataToBSB(TU8 *ptr, TU8 *buffer, TUint size);

    void dumpEsData(TU8 *pStart, TU8 *pEnd);
    EECode decodeH264(CIBuffer *in_buffer, CIBuffer *out_buffer);

    EECode feedH264(CIBuffer *in_buffer);
    EECode sendRemainFrames();

private:
    EECode fillEOSToBSB(TU8 *p_bsb_start);

private:
    const volatile SPersistMediaConfig *mpPersistMediaConfig;
    IMsgSink *mpMsgSink;

    SStreamCodecInfo mStreamCodecInfo;

    IDSPDecAPI *mpDSPDecoder;

    EDSPOperationMode mDSPMode;
    EDSPPlatform mDSPPlatform;
    StreamFormat mCodecFormat;

private:
    TUint mCapMaxCodedWidth, mCapMaxCodedHeight;
    //TUint mCurrentFramerateNum, mCurrentFramerateDen;

private:
    TU8 mbAddUdecWarpper;
    TU8 mbFeedPts;
    TU8 mbSpecifiedFPS;
    TU8 mbUDECInstanceSetup;

    TU8 mbUDECStopped;
    TU8 mbUDECHasFatalError;
    TU8 mbUDECPaused;
    TU8 mbUDECStepMode;

    TU8 mbHasWakeVout;
    TU8 mH264DataFmt;
    TU8 mH264AVCCNaluLen;
    TU8 mbMP4S;

    TU8 mbAddDelimiter;
    TU8 mbBWplayback;
    TU8 mbFeedingMode; // 1: normal, 2: turbo
    TU8 mIdentifyerCount;

private:
    TUint mSpecifiedTimeScale;
    TUint mSpecifiedFrameTick;

private:
    TU8 *mpBitSreamBufferStart;
    TU8 *mpBitSreamBufferEnd;
    TU8 *mpBitStreamBufferCurPtr;

    TU8 *mpBitStreamBufferCurAccumulatedPtr;
    TU32 mAccumulatedFrameNumber;

private:
    TU32 mFrameRateNum;
    TU32 mFrameRateDen;
    TU32 mFrameRate;

private:
    TU8 *mpVideoExtraData;
    TMemSize mVideoExtraDataSize;
    TU8 *mpH264spspps;
    TMemSize mH264spsppsSize;

private:
    TU8 mUSEQHeader[DUDEC_SEQ_HEADER_EX_LENGTH];
    TU8 mUPESHeader[DUDEC_PES_HEADER_LENGTH];
    TU8 mPrivateGOPHeader[DPRIVATE_GOP_NAL_HEADER_LENGTH + 2];

private:
    TTime mFakePTS;

private:
    FILE *mpDumpFile;
    FILE *mpDumpFileSeparate;
    TUint mDumpIndex;
    TUint mDumpStartFrame, mDumpEndFrame;

};
DCONFIG_COMPILE_OPTION_HEADERFILE_END
#endif

