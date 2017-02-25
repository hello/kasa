/**
 * am_aac_audio_decoder.h
 *
 * History:
 *    2013/8/29 - [Zhi He] create file
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

#ifndef __AM_AAC_AUDIO_DECODER_H__
#define __AM_AAC_AUDIO_DECODER_H__

typedef struct _SAACData {
  TU8 *data;
  TInt size;
} SAACData;

class CAACAudioDecoder: public CObject, public IAudioDecoder
{
  typedef CObject inherited;

public:
  static IAudioDecoder *Create(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink);
  virtual void Destroy();

public:
  virtual void PrintStatus();

public:
  virtual EECode SetupContext(SAudioParams *param);
  virtual void DestroyContext();

  virtual EECode Start();
  virtual EECode Stop();

  virtual EECode Decode(CIBuffer *in_buffer, CIBuffer *out_buffer, TInt &consumed_bytes);
  virtual EECode Flush();

  virtual EECode Suspend();

  virtual EECode SetExtraData(TU8 *p, TUint size);

  virtual void Delete();

private:
  CAACAudioDecoder(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink);
  EECode Construct();
  virtual ~CAACAudioDecoder();

private:
  const volatile SPersistMediaConfig *mpPersistMediaConfig;

private:
  EECode constructAACdec(SAudioParams *param);
  EECode initParameters(SAudioParams *param);
  EECode setupDecoder();
  EECode DecodeAudioAACdec(SAACData *pPacket, TS16 *out_ptr, TInt *out_size);
  EECode DecodeAudioAACdecRTP(SAACData *pPacket, TS16 *out_ptr, TInt *out_size);
  EECode WriteBitBuf(SAACData *pPacket, TInt offset);
  EECode AddAdtsHeader(SAACData *pPacket);
private:
  TU32 mSamplingFrequency;
  TU32 mChannelNumber;
  au_aacdec_config_t aacdec_config;
  TU32 mpDecMem[106000];
  TU8 mpDecBackUp[252];
  TU8 mpInputBuf[16384];
  TU32 mpOutBuf[8192];
};
#endif

