/**
 * am_aac_audio_decoder.cpp
 *
 * History:
 *    2013/8/29 - [Roy Su]  create file
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

#include "am_native.h"
#include "am_native_log.h"

#include "am_internal.h"

#include "am_osal.h"
#include "am_framework.h"
#include "am_modules_interface.h"
#include "am_codec_interface.h"

#ifdef BUILD_MODULE_LIBAAC

extern "C" {
#include "aac_audio_dec.h"
}
#include "am_aac_audio_decoder.h"

#if 0
static au_aacdec_config_t aacdec_config;
TU32 CAACAudioDecoder::mpDecMem[106000];
TU8 CAACAudioDecoder::mpDecBackUp[252];
TU8 CAACAudioDecoder::mpInputBuf[16384];
TU32 CAACAudioDecoder::mpOutBuf[8192];
#endif

IAudioDecoder *gfCreateAACAudioDecoderModule(const TChar *pName, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pEngineMsgSink)
{
  return CAACAudioDecoder::Create(pName, pPersistMediaConfig, pEngineMsgSink);
}

IAudioDecoder *CAACAudioDecoder::Create(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink)
{
  CAACAudioDecoder *result = new CAACAudioDecoder(pname, pPersistMediaConfig, pMsgSink);
  if (result && result->Construct() != EECode_OK) {
    delete result;
    result = NULL;
  }
  return result;
}

void CAACAudioDecoder::Destroy()
{
  Delete();
}

CAACAudioDecoder::CAACAudioDecoder(const TChar *pname, const volatile SPersistMediaConfig *pPersistMediaConfig, IMsgSink *pMsgSink)
  : inherited(pname)
  , mpPersistMediaConfig(pPersistMediaConfig)
{
  mSamplingFrequency = 0;
  mChannelNumber = 0;
}

EECode CAACAudioDecoder::Construct()
{
  DSET_MODULE_LOG_CONFIG(ELogModuleAACAudioDecoder);
  /*
      EECode err = constructAACdec();
      if (err != EECode_OK)
      {
          LOGM_ERROR("CAACAudioDecoder ,ConstructAACdec fail err = %d .\n", err);
          return err;
      }
  */
  memset(&aacdec_config, 0, sizeof(aacdec_config));
  //mConfigLogLevel = ELogLevel_Debug;
  //mConfigLogOption = DCAL_BITMASK(ELogOption_Flow) | DCAL_BITMASK(ELogOption_State);
  return EECode_OK;
}

void CAACAudioDecoder::Delete()
{
  aacdec_close();
  inherited::Delete();
}

CAACAudioDecoder::~CAACAudioDecoder()
{
}
//--------------------------------------------------------------------
//
//
//--------------------------------------------------------------------
EECode CAACAudioDecoder::SetupContext(SAudioParams *param)
{
  //    EECode err;
  switch (param->codec_format) {
    case StreamFormat_AAC:
      if (constructAACdec(param) != EECode_OK) {
        return EECode_Error;
      }
      break;
    default:
      LOGM_ERROR("codec_format=%u not supported.\n", param->codec_format);
      return EECode_Error;
  }
  return EECode_OK;
}

//One Packet may contains more than one sample. Q these.
EECode CAACAudioDecoder::Decode(CIBuffer *in_buffer, CIBuffer *out_buffer, TInt &consumed_bytes)
{
  SAACData pkt;
  TInt out_size;
  TS16 *out_ptr;
  EECode err = EECode_Error;
  DASSERT(in_buffer);
  DASSERT(out_buffer);
  mDebugHeartBeat ++;
  if (!in_buffer || !out_buffer) {
    LOGM_FATAL("NULL in_buffer %p or NULL out_buffer %p\n", in_buffer, out_buffer);
    return EECode_InternalLogicalBug;
  }
  out_size = out_buffer->GetDataMemorySize();
  out_ptr = (TS16 *)out_buffer->GetDataPtrBase();
  pkt.data = in_buffer->GetDataPtr();
  pkt.size = in_buffer->GetDataSize();
  if (in_buffer->GetBufferFlags() & CIBuffer::RTP_AAC) {
    err = DecodeAudioAACdecRTP(&pkt, out_ptr, &out_size);
  } else {
    err = DecodeAudioAACdec(&pkt, out_ptr, &out_size);
  }
  if (EECode_OK != err) {
    LOGM_ERROR("DecodeAudioAACdec() fail, err %d\n", err);
    return EECode_Error;
  }
  out_buffer->SetDataPtr((TU8 *)out_ptr);
  out_buffer->SetDataSize((TUint)out_size);
  out_buffer->mAudioChannelNumber =  aacdec_config.outNumCh;
  return EECode_OK;
}

void CAACAudioDecoder::DestroyContext()
{
  return;
}

EECode CAACAudioDecoder::Start()
{
  return EECode_OK;
}

EECode CAACAudioDecoder::Stop()
{
  return EECode_OK;
}

EECode CAACAudioDecoder::Flush()
{
  return EECode_OK;
}

EECode CAACAudioDecoder::Suspend()
{
  return EECode_OK;
}

EECode CAACAudioDecoder::SetExtraData(TU8 *p, TUint size)
{
  return EECode_OK;
}

void CAACAudioDecoder::PrintStatus()
{
  LOGM_PRINTF("heart beat %d\n", mDebugHeartBeat);
  mDebugHeartBeat = 0;
}

static void Audio32iTo16i(TS32 *bufin, TS16 *bufout, TS32 ch, TS32 proc_size)
{
  TS32 i, j;
  TS32 *bufin_ptr;
  TS16 *bufout_ptr;
  bufin_ptr = bufin;
  bufout_ptr = bufout;
  for (i = 0 ; i < proc_size ; i++) {
    for (j = 0 ; j < ch ; j++) {
      *bufout_ptr = (*bufin_ptr) >> 16;
      bufin_ptr++;
      bufout_ptr++;
    }
  }
}

EECode CAACAudioDecoder::constructAACdec(SAudioParams *param)
{
  initParameters(param);
  setupDecoder();
  return EECode_OK;
}

EECode CAACAudioDecoder::initParameters(SAudioParams *param)
{
  mSamplingFrequency = aacdec_config.externalSamplingRate = param->sample_rate;
  aacdec_config.bsFormat  = ADTS_BSFORMAT;
  mChannelNumber = aacdec_config.outNumCh = param->channel_number;
  aacdec_config.externalBitrate      = 0;
  aacdec_config.frameCounter         = 0;
  aacdec_config.errorCounter         = 0;
  aacdec_config.interBufSize         = 0;
  aacdec_config.codec_lib_mem_addr = (TU32 *)mpDecMem;
  aacdec_config.consumedByte = 0;
  return EECode_OK;
}

EECode CAACAudioDecoder::setupDecoder()
{
  aacdec_setup(&aacdec_config);
  aacdec_open(&aacdec_config);
  if (aacdec_config.ErrorStatus != 0) {
    LOGM_INFO("SetupHWDecoder Failed!!!\n\n");
  }
  aacdec_config.dec_rptr = (TU8 *)mpInputBuf;
  return EECode_OK;
}

//One Packet may contains more than one sample. Q these.
EECode CAACAudioDecoder::DecodeAudioAACdec(SAACData *pPacket, TS16 *out_ptr, TInt *out_size)
{
  //AM_INFO("DecodeAudioAACdec\n");
  TU8 *saveData;
  TInt saveSize;
  TUint inSize;
  DASSERT(pPacket != NULL);
  saveData = pPacket->data;
  saveSize = pPacket->size;
  aacdec_config.dec_wptr = (TS32 *)mpOutBuf;
  if (pPacket->data[0] != 0xff || (pPacket->data[1] | 0x0f) != 0xff) {
    AddAdtsHeader(pPacket);
    aacdec_config.interBufSize = inSize = pPacket->size + 7;
  } else {
    WriteBitBuf(pPacket, 0);
    aacdec_config.interBufSize = inSize = pPacket->size;
  }
  aacdec_set_bitstream_rp(&aacdec_config);
  aacdec_decode(&aacdec_config);
  if (aacdec_config.ErrorStatus != 0) {
    LOGM_ERROR("DecodeAudio Failed, %d\n", aacdec_config.ErrorStatus);
    pPacket->data = saveData;
    pPacket->size = saveSize;
    return EECode_Error;
  }
  //LOGM_DEBUG("consumedByte=%u, inSize=%u  ErrorStatus =%d ---->\n", aacdec_config.consumedByte, inSize, aacdec_config.ErrorStatus);
  DASSERT(aacdec_config.consumedByte == inSize);
  //DASSERT((aacdec_config.frameCounter - mFrameNum) == 1);
  //mFrameNum = aacdec_config.frameCounter;
  //length donot change, just 32 high 16bits to 16.
  Audio32iTo16i(aacdec_config.dec_wptr, out_ptr, aacdec_config.outNumCh, aacdec_config.frameSize);
  *out_size = aacdec_config.frameSize * aacdec_config.outNumCh * 2; //THIS size is bytes!!!!
  //LOGM_DEBUG("sampleSize %d, inSize %d, pPacket->size  %d\n", *out_size, inSize, pPacket->size);
  pPacket->data = saveData;
  pPacket->size = saveSize;
  return EECode_OK;
}

EECode CAACAudioDecoder::DecodeAudioAACdecRTP(SAACData *pPacket, TS16 *out_ptr, TInt *out_size)
{
  //AM_INFO("DecodeAudioAACdec\n");
  TU8 *saveData;
  TInt saveSize;
  DASSERT(pPacket != NULL);
  saveData = pPacket->data;
  saveSize = pPacket->size;
  TU8 *packet = pPacket->data;
  /* decode the first 2 bytes where the AUHeader sections are stored
    length in bits */
  int au_headers_length = (packet[0] << 8) | packet[1];
  /*AU HeaderSize, hard code now
  */
  int au_header_size = 13 + 3;//data->sizelength + data->indexlength;
  int au_headers_length_bytes = (au_headers_length + 7) / 8;
  pPacket->data += au_headers_length_bytes + 2;
  pPacket->size -= au_headers_length_bytes + 2;
  int output_size = 0;
  for (int i = 0; i < au_headers_length / au_header_size; i++) {
    TUint inSize = ((packet[2 + i * 2] << 8) | packet[3 + i * 2]) >> 3;
    if (inSize) {
      pPacket->size = inSize;
      WriteBitBuf(pPacket, 0);
      aacdec_config.interBufSize = inSize;
      aacdec_config.dec_wptr = (TS32 *)mpOutBuf;
      aacdec_config.consumedByte = 0;
      aacdec_set_bitstream_rp(&aacdec_config);
      aacdec_decode(&aacdec_config);
      if (aacdec_config.ErrorStatus != 0) {
        LOGM_ERROR("DecodeAudio Failed\n");
        pPacket->data = saveData;
        pPacket->size = saveSize;
        return EECode_Error;
      }
      DASSERT(aacdec_config.consumedByte == inSize);
      //DASSERT((aacdec_config.frameCounter - mFrameNum) == 1);
      //mFrameNum = aacdec_config.frameCounter;
      //length donot change, just 32 high 16bits to 16.
      Audio32iTo16i(aacdec_config.dec_wptr, out_ptr + output_size / 2, aacdec_config.outNumCh, aacdec_config.frameSize);
      output_size += aacdec_config.frameSize * aacdec_config.outNumCh * 2; //THIS size is bytes!!!!
      pPacket->data += inSize;
    }
  }
  *out_size = output_size;
  pPacket->data = saveData;
  pPacket->size = saveSize;
  return EECode_OK;
}
EECode CAACAudioDecoder::WriteBitBuf(SAACData *pPacket, int offset)
{
  memcpy(mpInputBuf + offset, pPacket->data, pPacket->size);
  return EECode_OK;
}

EECode CAACAudioDecoder::AddAdtsHeader(SAACData *pPacket)
{
  TUint size = pPacket->size + 7;
#if 0
  TU8 adtsHeader[7];
  adtsHeader[0] = 0xff;
  adtsHeader[1] = 0xf9;
  adtsHeader[2] = 0x4c;
  adtsHeader[3] = 0x80;          //fixed header end, the last two bits is the length .
  adtsHeader[4] = (size >> 3) & 0xff;
  adtsHeader[5] = (size & 0x07) << 5; //todo
  adtsHeader[6] = 0xfc;  //todo
  for (TUint i = 0; i < 7; i++) {
    mpInputBuf[i] = adtsHeader[i];
    //hBitBuf->cntBits += 8;
  }
#endif
  gfBuildADTSHeader((TU8 *)mpInputBuf, 1, 0, 1, 1, gfGetAACSamplingFrequencyIndex(mSamplingFrequency), 0, (TU8) mChannelNumber,
      0, 0, 0, 0, size, 0xff, 0);
  WriteBitBuf(pPacket, 7);
  return EECode_OK;
}

#endif

