/*******************************************************************************
 * am_amba_video_encoder.h
 *
 * History:
 *    2012/08/03 - [Zhi He] create file
 *
 * Copyright (C) 2015 Ambarella, Inc.
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

#ifndef __AM_AMBA_VIDEO_ENCODER_H__
#define __AM_AMBA_VIDEO_ENCODER_H__

class CAmbaVideoEncoder
  : public CObject
  , public IVideoEncoder
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

  virtual EECode Encode(CIBuffer *in_buffer, CIBuffer *out_buffer, TU32 &current_remaining, TU32 &all_cached_frames, TU32 &output_index);

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

  StreamFormat mCodecFormat[DMaxStreamNumber];

  TU32 mBitrate[DMaxStreamNumber];

private:
  TU8 *mpBitstreamBufferStart;
  TU32 mBitstreamBufferSize;

private:
  TU32 mVideoWidth[DMaxStreamNumber];
  TU32 mVideoHeight[DMaxStreamNumber];
  TU32 mFramerateNum[DMaxStreamNumber];
  TU32 mFramerateDen[DMaxStreamNumber];
  float mFramerate[DMaxStreamNumber];

  TU32 mVinFramerateNum;
  TU32 mVinFramerateDen;
  float mVinFramerate;

private:
  TU32 mStreamID;

  TU8 mRemainingDescNumber;
  TU8 mCurrentDescIndex;
  TU8 mbCopyOut;
  TU8 mReserved0;

  TU8 mbSkipDelimiter;
  TU8 mbSkipSEI;
  TU8 mbRemaingData;
  TU8 mRemainingDataOutputIndex;

  TU8 mbWaitFirstKeyframe[DMaxStreamNumber];
  TU8 mbAlreadySendSyncBuffer[DMaxStreamNumber];

private:
  TU8 *mpVideoExtraData[DMaxStreamNumber];
  TUint mVideoExtraDataSize[DMaxStreamNumber];

  TU8 *mpRemainingData;
  TUint mnRemainingDataSize;
  TTime mRemainBufferTimestamp;
};
#endif

