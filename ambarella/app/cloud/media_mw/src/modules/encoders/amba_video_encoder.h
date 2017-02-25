/*******************************************************************************
 * amba_video_encoder.h
 *
 * History:
 *    2012/08/03 - [Zhi He] create file
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

