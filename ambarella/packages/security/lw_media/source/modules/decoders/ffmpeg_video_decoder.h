
/*
 * ffmpeg_video_decoder.h
 *
 * History:
 *    2014/09/26 - [Zhi He] create file
 *
 * Copyright (C) 2014 - 2024, the Ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the Ambarella Inc.
 */

#ifdef BUILD_MODULE_FFMPEG

#ifndef __FFMPEG_VIDEO_DECODER_H__
#define __FFMPEG_VIDEO_DECODER_H__

//-----------------------------------------------------------------------
//
// CFFMpegVideoDecoder
//
//-----------------------------------------------------------------------
class CFFMpegVideoDecoder: public CObject, virtual public IVideoDecoder
{
    typedef CObject inherited;

protected:
    CFFMpegVideoDecoder(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink, TUint index);
    virtual ~CFFMpegVideoDecoder();
    EECode Construct();

public:
    static IVideoDecoder *Create(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink, TUint index);
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

    virtual EECode SetExtraData(TU8 *p, TMemSize size);

    virtual EECode SetPbRule(TU8 direction, TU8 feeding_rule, TU16 speed);

    virtual EECode SetFrameRate(TUint framerate_num, TUint frameate_den);

    virtual EDecoderMode GetDecoderMode() const;

    //direct mode
    virtual EECode DecodeDirect(CIBuffer *in_buffer);
    virtual EECode SetBufferPoolDirect(IOutputPin *p_output_pin, IBufferPool *p_bufferpool);

    //turbo mode
    virtual EECode QueryFreeZoom(TUint &free_zoom, TU32 &fullness);
    virtual EECode QueryLastDisplayedPTS(TTime &pts);
    virtual EECode PushData(CIBuffer *in_buffer, TInt flush);
    virtual EECode PullDecodedFrame(CIBuffer *out_buffer, TInt block_wait, TInt &remaining);

    virtual EECode TuningPB(TU8 fw, TU16 frame_tick);

private:
    EECode setupDecoder();
    void destroyDecoder();

private:
    const volatile SPersistMediaConfig *mpPersistMediaConfig;
    IMsgSink *mpEngineMsgSink;
    IBufferPool *mpBufferPool;

private:
    AVCodecContext *mpCodec;
    AVCodec *mpDecoder;
    AVFrame mFrame;
    AVPacket mPacket;
    enum AVCodecID mCodecID;

private:
    TU8 *mpExtraData;
    TMemSize mExtraDataBufferSize;
    TMemSize mExtraDataSize;

private:
    TU8 mbDecoderSetup;
    TU8 mbSkipSPSPPS;
    TU8 mReserved1;
    TU8 mReserved2;
};

#endif

#endif

