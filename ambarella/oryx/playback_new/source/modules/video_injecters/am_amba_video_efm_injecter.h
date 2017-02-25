/*******************************************************************************
 * am_amba_video_efm_injecter.h
 *
 * History:
 *    2015/12/25 - [Zhi He] create file
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

#ifndef __AM_AMBA_VIDEO_EFM_INJECTER_H__
#define __AM_AMBA_VIDEO_EFM_INJECTER_H__

class CAmbaVideoEFMInjecter: public CObject, public IVideoInjecter
{
  typedef CObject inherited;

protected:
  CAmbaVideoEFMInjecter(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink);
  EECode Construct();
  virtual ~CAmbaVideoEFMInjecter();

public:
  static IVideoInjecter *Create(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink);
  virtual void Destroy();

public:
  virtual EECode SetupContext(SVideoInjectParams *params);
  virtual void DestroyContext();
  virtual EECode Prepare(TU8 stream_index);
  virtual EECode Inject(CIBuffer *buffer);
  virtual EECode InjectVoid(TU32 last_flag);
  virtual EECode BatchInject(TU8 src_buf, TU8 num, TU8 last_flag,
      void *callback_context, TFInjecterCallback callback);

private:
  void feedFrameYUV420pYUV422p(CIBuffer *buffer, SAmbaEFMFrame *frame);
  void feedFrameYUYV(CIBuffer *buffer, SAmbaEFMFrame *frame);
  void feedFrameNV12(CIBuffer *buffer, SAmbaEFMFrame *frame);

  void fillBlackFrameNV12(SAmbaEFMFrame *frame);

private:
  const volatile SPersistMediaConfig *mpPersistMediaConfig;
  IMsgSink *mpMsgSink;

private:
  TInt mIavFd;
  SFAmbaDSPDecAL mfDSPAL;

  SAmbaDSPMapDSP mMapDSP;
  SAmbaEFMPoolInfo mPoolInfo;

private:
  TU8 mbSetup;
  TU8 mStreamIndex;
  TU8 mReserved0;
  TU8 mReserved1;
};

#endif

