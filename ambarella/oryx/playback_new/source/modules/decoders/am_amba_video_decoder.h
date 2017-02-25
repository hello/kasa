/**
 * am_amba_video_decoder.h
 *
 * History:
 *    2012/08/03 - [Zhi He] create file
 *
 * Copyright (c) 2015 Ambarella, Inc.
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
 */

#ifndef __AM_AMBA_VIDEO_DECODER_H__
#define __AM_AMBA_VIDEO_DECODER_H__

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
  EECode decodeH265(CIBuffer *in_buffer);

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
  SAmbaDSPVoutInfo mVoutInfos[DAMBADSP_MAX_VOUT_NUMBER];
  SAmbaDSPDecoderInfo mDecoderInfo;

private:
  TU32 mCapMaxCodedWidth, mCapMaxCodedHeight;

private:
  TU8 mbAddAmbaGopHeader;
  TU8 mDecId;
  TU8 mbBWplayback;
  TU8 mVoutNumber;

  TU8 mbEnableVoutDigital;
  TU8 mbEnableVoutHDMI;
  TU8 mbEnableVoutCVBS;
  TU8 mFeedingRule;

    TU8 mbStopCmdSent;
    TU8 mbDiscardCurrentGOP;
    TU8 mbGopBasedFeed;
    TU8 mbAutoMapBSB;

    TU8 mbSendDecodeReadyMsg;
    TU8 mReserved0;
    TU8 mReserved1;
    TU8 mReserved2;

  TU8 mbExitDecodeMode;
  TU8 mbSupportAllframeBackwardPlayback;
  TU16 mMaxGopSize;

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
  TU8 mpAmbaGopHeader[DAMBA_MAX_GOP_HEADER_LENGTH];

private:
  TU8 mbDumpBitstream;
  TU8 mbDumpOnly;
  TU16 mDumpIndex;
  void *mpDumper;
};

#endif

