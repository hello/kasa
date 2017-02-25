/*******************************************************************************
 * ffmpeg_audio_encoder.h
 *
 * History:
 *    2014/10/27 - [Zhi He] create file
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

#ifdef BUILD_MODULE_FFMPEG

#ifndef __FFMPEG_AUDIO_ENCODER_H__
#define __FFMPEG_AUDIO_ENCODER_H__

DCONFIG_COMPILE_OPTION_HEADERFILE_BEGIN

DCODE_DELIMITER;

class CFFMpegAudioEncoder: public CObject, virtual public IAudioEncoder
{
    typedef CObject inherited;

public:
    static IAudioEncoder *Create(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink);
    virtual void Destroy();
    virtual void Delete();

public:
    virtual void PrintStatus();

public:
    virtual EECode SetupContext(SAudioParams *param);
    virtual void DestroyContext();

    virtual EECode Encode(TU8 *p_in, TU8 *&p_out, TU32 &output_size);
    virtual EECode GetExtraData(TU8 *&p, TU32 &size);

private:
    CFFMpegAudioEncoder(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink);
    EECode Construct();
    virtual ~CFFMpegAudioEncoder();

private:
    EECode setupEncoder(enum AVCodecID codec_id);
    void destroyEncoder();

private:
    const volatile SPersistMediaConfig *mpPersistMediaConfig;
    AVCodecContext *mpCodec;
    AVCodec *mpEncoder;

    enum AVCodecID mCodecID;
    TU32 mSamplerate;
    TU32 mFrameSize;
    TU32 mBitrate;

private:
    TU8 *mpAudioExtraData;
    TU32 mAudioExtraDataSize;

private:
    TU8 mbEncoderSetup;
    TU8 mChannelNumber;
    TU8 mSampleFormat;
    TU8 mReserved2;

private:
    AVFrame mFrame;
    AVPacket mPacket;
};

DCONFIG_COMPILE_OPTION_HEADERFILE_END
#endif

#endif

