/*******************************************************************************
 * am_thumbplay.h
 *
 * History:
 *  2016/03/10 - [Zhi He] create file
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

#ifndef __AM_THUMBPLAY_H__
#define __AM_THUMBPLAY_H__

//-----------------------------------------------------------------------
//
// CThumbPlay
//
//-----------------------------------------------------------------------

typedef struct {
  TDimension width, height;
  TDimension offset_x, offset_y;
} SThumbPlayDisplayDimension;

class CThumbPlay
  : public IThumbPlayControl
{

public:
  static CThumbPlay *Create();

private:
  CThumbPlay();
  EECode Construct();
  virtual ~CThumbPlay();

public:
  virtual EECode SetMediaPath(TChar *url);
  virtual EECode SetDisplayDevice(EDisplayDevice device);
  virtual TU32 GetTotalFileNumber();
  virtual EECode SetDisplayLayout(EThumbPlayDisplayLayout layout, void *params);
  virtual EECode ExitDeviceMode();
  virtual EECode SetDisplayBuffer(SThumbPlayBufferDesc *buffer, TDimension buffer_width, TDimension buffer_height);
  virtual EECode DisplayThumb2Device(TU32 begin_index);
  virtual EECode DisplayThumb2Buffer(TU32 begin_index);

public:
  virtual void Destroy();

private:
  CIDoubleLinkedList::SNode *findNodeFromIndex(CIDoubleLinkedList *list, TU32 index);
  void rectangleLayout(SThumbPlayDisplayDimension *display, TU32 index);
  EECode openDevice();
  void closeDevice();
  EECode deviceSetupContext();
  EECode deviceDestroyContext();
  EECode deviceEnterDecodeMode();
  EECode deviceLeaveDecodeMode();
  EECode deviceCreateDecoder(TU8 decoder_id, TU8 decoder_type, TU32 width, TU32 height);
  EECode deviceDestroyDecoder();
  TU8 *deviceCopyDataToBSB(TU8 *ptr, TU8 *buffer, TUint size);
  EECode deviceDecodeIntraframe(TU8 *p_data, TU32 size, TU8 *pextra, TU32 extrasize, SAmbaDSPIntraplayBuffer *buffer);
  EECode deviceYUV2YUV(SAmbaDSPIntraplayBuffer *src_buffer, SAmbaDSPIntraplayBuffer *dst_buffer);
  EECode deviceDisplay(SAmbaDSPIntraplayBuffer *src_buffer);

private:
  TU32 mnTotalFileNumber;
  TU32 mCurrentDisplayBeginIndex;
  TU32 mnTotalDisplayNumber;
  TU32 mnCurrentDisplayNumber;

  IDirANSI *mpDir;
  SDirNodeANSI *mpDirTree;
  IMediaProbe *mpMediaProbe;

  EMediaProbeType mMediaProbeType;
  EThumbPlayDisplayLayout mCurrentDisplayLayout;

private:
  SThumbPlayDisplayRectangleLayoutParams mRectangleParams;
  SThumbPlayBufferDesc mDisplayBuffer;

private:
  CIDoubleLinkedList *mpDisplayItemList;

private:
  TU8 mbDirectDisplay;
  TU8 mbExitDecodeMode;
  TU8 mbDeviceInDecodeMode;
  TU8 mReserved2;

  TDimension mDisplayDeviceWidth, mDisplayDeviceHeight;
  //TDimension mHorizontalBorder, mVerticalBorder;

private:
  TInt mDeviceHandle;
  SFAmbaDSPDecAL mfDSPAL;

  SAmbaDSPMapBSB mMapBSB;
  SAmbaDSPMapIntraPB mMapIntraPB;
  SAmbaDSPDecodeModeConfig mModeConfig;
  SAmbaDSPVoutInfo mVoutInfos[DAMBADSP_MAX_VOUT_NUMBER];
  SAmbaDSPDecoderInfo mDecoderInfo;

  TU8 mVoutNumber;
  TU8 mbEnableVoutDigital;
  TU8 mbEnableVoutHDMI;
  TU8 mbEnableVoutCVBS;

  TU32 mCapMaxCodedWidth;
  TU32 mCapMaxCodedHeight;

  TU8 *mpBitSreamBufferStart;
  TU8 *mpBitSreamBufferEnd;
  TU8 *mpBitStreamBufferCurPtr;
};

#endif

