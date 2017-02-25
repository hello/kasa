/*******************************************************************************
 * x264_video_encoder.h
 *
 * History:
 *    2014/10/15 - [Zhi He] create file
 *
 * Copyright (c) 2016 Ambarella, Inc.
 *
 * This file and its contents ("Software") are protected by intellectual
 * property rights including, without limitation, U.S. and/or foreign
 * copyrights. This Software is also the confidential and proprietary
 * information of Ambarella, Inc. and its licensors. You may not use, reproduce,
 * disclose, distribute, modify, or otherwise prepare derivative works of this
 * Software or any portion thereof except pursuant to a signed license agreement
 * or nondisclosure agreement with Ambarella, Inc. or its authorized affiliates.
 * In the absence of such an agreement, you agree to promptly notify and return
 * this Software to Ambarella, Inc.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF NON-INFRINGEMENT,
 * MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL AMBARELLA, INC. OR ITS AFFILIATES BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; COMPUTER FAILURE OR MALFUNCTION; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************/

#ifndef __X264_VIDEO_ENCODER_H__
#define __X264_VIDEO_ENCODER_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN

DCODE_DELIMITER;

//-----------------------------------------------------------------------
//
// CX264VideoEncoder
//
//-----------------------------------------------------------------------
class CX264VideoEncoder: public CObject, public IVideoEncoder
{
    typedef CObject inherited;

public:
    static CX264VideoEncoder *Create(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index);
    virtual void Destroy();

protected:
    CX264VideoEncoder(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink, TU32 index);

    EECode Construct();
    virtual ~CX264VideoEncoder();

public:
    virtual void Delete();

public:
    virtual EECode SetupContext(SVideoEncoderInputParam *param);
    virtual void DestroyContext();
    virtual EECode SetBufferPool(IBufferPool *p_buffer_pool);

    virtual EECode Start();
    virtual EECode Stop();

    virtual EECode Encode(CIBuffer *in_buffer, CIBuffer *out_buffer, TU32 &current_remaining, TU32 &all_cached_frames);

    virtual EECode VideoEncoderProperty(EFilterPropertyType property_type, TUint set_or_get, void *p_param);

private:
    EECode setupX264Encoder();
    void closeX264Encoder();
    EECode encodeReturnRemaining(CIBuffer *out_buffer, TU32 &current_remaining);
    void updateTimestamp(CIBuffer *out_buffer);

private:
    const volatile SPersistMediaConfig *mpPersistMediaConfig;
    IMsgSink *mpEngineMsgSink;
    IBufferPool *mpBufferPool;
    IMemPool *mpMemPool;

private:
    TU8 mbX264EncoderSetup;
    TU8 mReserved0, mReserved1, mReserved2;

private:
    TU32 mVideoWidth, mVideoHeight;
    TU32 mFramerateNum, mFramerateDen;
    TU32 mBitrate;

    TU32 mParamsMaxIDRInterval;
    TU32 mParamsMinIInterval;
    TU32 mParamsBframes;

    float mFramerate;

private:
    TTime mStartTime;
    TTime mTimeBasedDTS;
    TTime mFrameCountBasedDTS;
    TTime mStartDTS;
    TTime mOriDTSGap;
    TTime mCurDTSGap;
    TS64 mFrameCount;

private:
    x264_param_t mX264Params;
    x264_t *mpX264;

    x264_nal_t *mpNal;
    TS32 mNalIndex;
    TS32 mNalCount;

    x264_picture_t mInputPicture;
    x264_picture_t mOutputPicture;
    TS64 mX264FrameTick;

    TTime mCurrentFramePTS;
    TTime mCurrentFrameDTS;

    TU64 mTotalInputFrameCount;
    TU64 mTotalOutputFrameCount;
};

DCONFIG_COMPILE_OPTION_HEADERFILE_END

#endif

