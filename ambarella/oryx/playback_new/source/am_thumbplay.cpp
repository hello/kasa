/*******************************************************************************
 * am_thumbplay.cpp
 *
 * History:
 *    2016/03/10 - [Zhi He] create file
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

#include "stdlib.h"
#include "stdio.h"
#include "string.h"

#include "am_native.h"
#include "am_native_log.h"

#include "am_internal.h"

#include "am_network_al.h"

#include "am_internal_streaming.h"

#include "am_osal.h"
#include "am_framework.h"
#include "am_modules_interface.h"
#include "am_filters_interface.h"

#include "am_amba_dsp.h"

#include "am_thumbplay.h"

IThumbPlayControl *CreateThumbPlay()
{
  return (IThumbPlayControl *) CThumbPlay::Create();
}

//-----------------------------------------------------------------------
//
// CThumbPlay
//
//-----------------------------------------------------------------------
CThumbPlay *CThumbPlay::Create()
{
  CThumbPlay *result = new CThumbPlay();
  if (result && result->Construct() != EECode_OK) {
    delete result;
    result = NULL;
  }
  return result;
}

CThumbPlay::CThumbPlay()
  : mnTotalFileNumber(0)
  , mCurrentDisplayBeginIndex(0)
  , mnTotalDisplayNumber(0)
  , mnCurrentDisplayNumber(0)
  , mpDir(NULL)
  , mpDirTree(NULL)
  , mpMediaProbe(NULL)
  , mMediaProbeType(EMediaProbeType_Invalid)
  , mCurrentDisplayLayout(EThumbPlayDisplayLayout_Invalid)
  , mpDisplayItemList(NULL)
{
  memset(&mRectangleParams, 0x0, sizeof(mRectangleParams));
  mbExitDecodeMode = 0;
  mbDeviceInDecodeMode = 0;
  mDeviceHandle = -1;
  memset(&mMapBSB, 0x0, sizeof(mMapBSB));
  memset(&mMapIntraPB, 0x0, sizeof(mMapIntraPB));
  memset(&mModeConfig, 0x0, sizeof(mModeConfig));
  mVoutNumber = 1;
  mbEnableVoutDigital = 0;
  mbEnableVoutHDMI = 0;
  mbEnableVoutCVBS = 0;
  mCapMaxCodedWidth = 1920;
  mCapMaxCodedHeight = 1088;
  mpBitSreamBufferStart = NULL;
  mpBitSreamBufferEnd = NULL;
  mpBitStreamBufferCurPtr = NULL;
}

EECode CThumbPlay::Construct()
{
  mpDir = gfCreateIDirANSI(EDirType_Dirent);
  if (DUnlikely(!mpDir)) {
    LOG_ERROR("gfCreateIDirANSI fail\n");
    return EECode_NoMemory;
  }
  return EECode_OK;
}

CThumbPlay::~CThumbPlay()
{
}

EECode CThumbPlay::SetMediaPath(TChar *url)
{
  EECode err = EECode_OK;
  if (DLikely(url && mpDir)) {
    if (mpDirTree) {
      mpDir->CleanTree(mpDirTree);
      mpDirTree = NULL;
    }
    if (mpDisplayItemList) {
      mpDir->CleanDisplayList(mpDisplayItemList);
      mpDisplayItemList = NULL;
    }
    LOG_NOTICE("before ReadDir\n");
    err = mpDir->ReadDir(url, 0, mpDirTree);
    if (EECode_OK != err) {
      LOG_ERROR("ReadDir fail, ret 0x%08x, %s\n", err, gfGetErrorCodeString(err));
      return err;
    }
    LOG_NOTICE("before BuildDisplayList\n");
    err = mpDir->BuildDisplayList(mpDirTree, mpDisplayItemList);
    if (EECode_OK != err) {
      LOG_ERROR("BuildDisplayList fail, ret 0x%08x, %s\n", err, gfGetErrorCodeString(err));
      return err;
    }
    mnTotalFileNumber = mpDisplayItemList->NumberOfNodes();
    LOG_NOTICE("after BuildDisplayList, mnTotalFileNumber %d\n", mnTotalFileNumber);
  } else {
    LOG_FATAL("NULL url %p, or mpDir %p\n", url, mpDir);
    return EECode_BadParam;
  }
  return EECode_OK;
}

EECode CThumbPlay::SetDisplayDevice(EDisplayDevice device)
{
  mbEnableVoutDigital = 0;
  mbEnableVoutHDMI = 0;
  mbEnableVoutCVBS = 0;
  LOG_NOTICE("device, %x\n", device);
  switch (device) {
    case EDisplayDevice_DIGITAL:
      mbEnableVoutDigital = 1;
      break;
    case EDisplayDevice_HDMI:
      mbEnableVoutHDMI = 1;
      break;
    case EDisplayDevice_CVBS:
      mbEnableVoutCVBS = 1;
      break;
    default:
      LOG_FATAL("bad device type %d\n", device);
      return EECode_BadParam;
  }
  return EECode_OK;
}

TU32 CThumbPlay::GetTotalFileNumber()
{
  return mnTotalFileNumber;
}

EECode CThumbPlay::SetDisplayLayout(EThumbPlayDisplayLayout layout, void *params)
{
  switch (layout) {
    case EThumbPlayDisplayLayout_Rectangle: {
        SThumbPlayDisplayRectangleLayoutParams *p = (SThumbPlayDisplayRectangleLayoutParams *) params;
        if (!p) {
          LOG_ERROR("NULL params\n");
          return EECode_BadParam;
        }
        if ((p->rows < 1) || (p->columns < 1)) {
          LOG_ERROR("bad rows columns %d %d\n", p->rows, p->columns);
          return EECode_BadParam;
        }
        if ((p->border_x < 0) || (p->border_y < 0)) {
          LOG_ERROR("bad border_x border_y %d %d\n", p->border_x, p->border_y);
          return EECode_BadParam;
        }
        if (p->tot_width && p->tot_height) {
          if ((p->tot_width < p->columns) || (p->tot_height < p->rows)) {
            LOG_ERROR("bad width height %d %d\n", p->tot_width, p->tot_height);
            return EECode_BadParam;
          }
          if ((p->tot_width < (p->columns * (1 + 2 * p->border_x))) || (p->tot_height < (p->rows * (1 + 2 * p->border_y)))) {
            LOG_ERROR("too small width height %d %d, columns row %d %d, border x %d, y %d\n", p->tot_width, p->tot_height, p->columns, p->rows, p->border_x, p->border_y);
            return EECode_BadParam;
          }
          mRectangleParams.rows = p->rows;
          mRectangleParams.columns = p->columns;
          mRectangleParams.tot_width = p->tot_width;
          mRectangleParams.tot_height = p->tot_height;
          mRectangleParams.border_x = p->border_x;
          mRectangleParams.border_y = p->border_y;
          mRectangleParams.rect_width = p->tot_width / mRectangleParams.columns;
          mRectangleParams.rect_height = p->tot_height / mRectangleParams.rows;
          mnTotalDisplayNumber = mRectangleParams.rows * mRectangleParams.columns;
        } else if (mDisplayDeviceWidth && mDisplayDeviceHeight) {
          p->tot_width = mDisplayDeviceWidth;
          p->tot_height = mDisplayDeviceHeight;
          if ((p->tot_width < (p->columns * (1 + 2 * p->border_x))) || (p->tot_height < (p->rows * (1 + 2 * p->border_y)))) {
            LOG_ERROR("too small width height %d %d, columns row %d %d, border x %d, y %d\n", p->tot_width, p->tot_height, p->columns, p->rows, p->border_x, p->border_y);
            return EECode_BadParam;
          }
          mRectangleParams.rows = p->rows;
          mRectangleParams.columns = p->columns;
          mRectangleParams.tot_width = p->tot_width;
          mRectangleParams.tot_height = p->tot_height;
          mRectangleParams.border_x = p->border_x;
          mRectangleParams.border_y = p->border_y;
          mRectangleParams.rect_width = p->tot_width / mRectangleParams.columns;
          mRectangleParams.rect_height = p->tot_height / mRectangleParams.rows;
          mnTotalDisplayNumber = mRectangleParams.rows * mRectangleParams.columns;
        } else {
          mRectangleParams.rows = p->rows;
          mRectangleParams.columns = p->columns;
          mRectangleParams.border_x = p->border_x;
          mRectangleParams.border_y = p->border_y;
          mnTotalDisplayNumber = mRectangleParams.rows * mRectangleParams.columns;
        }
      }
      break;
    default:
      LOG_FATAL("not support layout %d\n", layout);
      break;
  }
  mCurrentDisplayLayout = layout;
  return EECode_OK;
}

EECode CThumbPlay::ExitDeviceMode()
{
  if (mbDeviceInDecodeMode) {
    deviceDestroyContext();
    mbDeviceInDecodeMode = 0;
  }
  if (0 <= mDeviceHandle) {
    closeDevice();
  }
  return EECode_OK;
}

EECode CThumbPlay::SetDisplayBuffer(SThumbPlayBufferDesc *buffer, TDimension buffer_width, TDimension buffer_height)
{
  TU32 i = 0;
  mDisplayBuffer.fmt = buffer->fmt;
  for (i = 0; i < 4; i ++) {
    mDisplayBuffer.p[i] = buffer->p[i];
    mDisplayBuffer.linesize[i] = buffer->linesize[i];
  }
  return EECode_OK;
}

EECode CThumbPlay::DisplayThumb2Device(TU32 begin_index)
{
  TU32 i = 0;
  TInt ret = 0;
  EECode err = EECode_OK;
  CIDoubleLinkedList::SNode *node = NULL;
  SDirNodeANSI *context = NULL;
  SMediaProbedInfo mediainfo;
  SGenericDataBuffer *data_buffer = NULL;
  SThumbPlayDisplayDimension display_dimension;
  SAmbaDSPIntraplayBuffer intraplay_yuv420_buffer;
  SAmbaDSPIntraplayBuffer intraplay_yuv422_buffer;
  SAmbaDSPIntraplayResetBuffers intraplay_reset_buffers;
  if (begin_index >= mnTotalFileNumber) {
    LOG_ERROR("begin index (%d) exceed max %d\n", begin_index, mnTotalFileNumber);
    return EECode_BadParam;
  }
  if (0 > mDeviceHandle) {
    err = openDevice();
    if (EECode_OK != err) {
      LOG_ERROR("openDevice() fail, ret 0x%08x, %s\n", err, gfGetErrorCodeString(err));
      return err;
    }
  }
  if (!mbDeviceInDecodeMode) {
    err = deviceSetupContext();
    if (EECode_OK != err) {
      LOG_ERROR("deviceSetupContext() fail, ret 0x%08x, %s\n", err, gfGetErrorCodeString(err));
      return err;
    }
    mbDeviceInDecodeMode = 1;
  }
  if (!mpMediaProbe) {
    mpMediaProbe = gfCreateMediaProbe(EMediaProbeType_MP4);
    if (!mpMediaProbe) {
      LOG_ERROR("gfCreateMediaProbe fail\n");
      return EECode_NotSupported;
    }
  }
  mnCurrentDisplayNumber = mnTotalFileNumber - begin_index;
  if (mnCurrentDisplayNumber > mnTotalDisplayNumber) {
    mnCurrentDisplayNumber = mnTotalDisplayNumber;
  }
  node = findNodeFromIndex(mpDisplayItemList, begin_index);
  if (node) {
    switch (mCurrentDisplayLayout) {
      case EThumbPlayDisplayLayout_Rectangle: {
          TU32 show_thumb = 0;
          //FILE *p_dump = NULL;
          intraplay_reset_buffers.decoder_id = 0;
          intraplay_reset_buffers.max_num = 2;
          ret = mfDSPAL.f_intraplay_reset_buffers(mDeviceHandle, &intraplay_reset_buffers);
          if (ret) {
            LOG_ERROR("reset buffers fail, ret %d\n", ret);
            mpMediaProbe->ReleaseFrame(data_buffer);
            mpMediaProbe->Close();
            return EECode_OSError;
          }
          intraplay_yuv422_buffer.buf_width = mDisplayDeviceWidth;
          intraplay_yuv422_buffer.buf_height = mDisplayDeviceHeight;
          intraplay_yuv422_buffer.ch_fmt = EAMDSP_BUFFER_PIX_FMT_422;
          ret = mfDSPAL.f_intraplay_request_buffer(mDeviceHandle, &intraplay_yuv422_buffer);
          if (ret) {
            LOG_ERROR("reset buffers fail, ret %d\n", ret);
            mpMediaProbe->ReleaseFrame(data_buffer);
            mpMediaProbe->Close();
            return EECode_OSError;
          }
#if 0
          printf("memset: start 0x%08x, lu offset 0x%08x, ch offset 0x%08x, pitch %d, width %d, height %d\n", (TU32) mMapIntraPB.base,
                 intraplay_yuv422_buffer.lu_buf_offset, intraplay_yuv422_buffer.ch_buf_offset,
                 intraplay_yuv422_buffer.buf_pitch, intraplay_yuv422_buffer.buf_width, intraplay_yuv422_buffer.buf_height);
          memset(((TU8 *) mMapIntraPB.base) + intraplay_yuv422_buffer.lu_buf_offset, 0x01, intraplay_yuv422_buffer.buf_pitch * intraplay_yuv422_buffer.buf_height);
          memset(((TU8 *) mMapIntraPB.base) + intraplay_yuv422_buffer.ch_buf_offset, 0x02, intraplay_yuv422_buffer.buf_pitch * intraplay_yuv422_buffer.buf_height / 2);
          p_dump = fopen("luma_ori.dump", "wb+");
          if (p_dump) {
            printf("lu, start 0x%08x, lu offset 0x%08x, ch offset 0x%08x, pitch %d, width %d, height %d\n", (TU32) mMapIntraPB.base,
                   intraplay_yuv422_buffer.lu_buf_offset, intraplay_yuv422_buffer.ch_buf_offset,
                   intraplay_yuv422_buffer.buf_pitch, intraplay_yuv422_buffer.buf_width, intraplay_yuv422_buffer.buf_height);
            fwrite(((TU8 *) mMapIntraPB.base) + intraplay_yuv422_buffer.lu_buf_offset, 1, intraplay_yuv422_buffer.buf_pitch * intraplay_yuv422_buffer.buf_height, p_dump);
            fflush(p_dump);
            fclose(p_dump);
          }
          p_dump = fopen("chroma_ori.dump", "wb+");
          if (p_dump) {
            printf("ch, start 0x%08x, lu offset 0x%08x, ch offset 0x%08x, pitch %d, width %d, height %d\n", (TU32) mMapIntraPB.base,
                   intraplay_yuv422_buffer.lu_buf_offset, intraplay_yuv422_buffer.ch_buf_offset,
                   intraplay_yuv422_buffer.buf_pitch, intraplay_yuv422_buffer.buf_width, intraplay_yuv422_buffer.buf_height);
            fwrite(((TU8 *) mMapIntraPB.base) + intraplay_yuv422_buffer.ch_buf_offset, 1, intraplay_yuv422_buffer.buf_pitch * intraplay_yuv422_buffer.buf_height / 2, p_dump);
            fflush(p_dump);
            fclose(p_dump);
          }
          p_dump = NULL;
#endif
          intraplay_yuv420_buffer.buf_width = mCapMaxCodedWidth;
          intraplay_yuv420_buffer.buf_height = mCapMaxCodedHeight;
          intraplay_yuv420_buffer.ch_fmt = EAMDSP_BUFFER_PIX_FMT_420;
          ret = mfDSPAL.f_intraplay_request_buffer(mDeviceHandle, &intraplay_yuv420_buffer);
          if (ret) {
            LOG_ERROR("reset buffers fail, ret %d\n", ret);
            mpMediaProbe->ReleaseFrame(data_buffer);
            mpMediaProbe->Close();
            return EECode_OSError;
          }
          for (i = 0; i < mnCurrentDisplayNumber; i ++) {
            if (node) {
              context = (SDirNodeANSI *) node->p_context;
              if (context) {
                err = mpMediaProbe->Open(context->name, &mediainfo);
                if (EECode_OK == err) {
                  if ((mediainfo.info.video_width > mCapMaxCodedWidth) || (mediainfo.info.video_height > mCapMaxCodedHeight)) {
                    LOG_WARN("skip media file(%s), due to exceed resolution %dx%d, max %dx%d\n",
                             context->name, mediainfo.info.video_width, mediainfo.info.video_height,
                             mCapMaxCodedWidth, mCapMaxCodedHeight);
                    mpMediaProbe->Close();
                    continue;
                  }
                  data_buffer = mpMediaProbe->GetKeyFrame(0);
                  if (data_buffer) {
                    intraplay_yuv420_buffer.img_width = mediainfo.info.video_width;
                    intraplay_yuv420_buffer.img_height = mediainfo.info.video_height;
                    intraplay_yuv420_buffer.buf_width = mediainfo.info.video_width;
                    intraplay_yuv420_buffer.buf_height = mediainfo.info.video_height;
                    intraplay_yuv420_buffer.buf_pitch = (mediainfo.info.video_width + 31) & (~31);
                    intraplay_yuv420_buffer.ch_buf_offset = intraplay_yuv420_buffer.lu_buf_offset + intraplay_yuv420_buffer.buf_pitch * intraplay_yuv420_buffer.buf_height;
                    rectangleLayout(&display_dimension, i);
                    intraplay_yuv422_buffer.buf_width = display_dimension.width;
                    intraplay_yuv422_buffer.buf_height = display_dimension.height;
                    intraplay_yuv422_buffer.img_width = display_dimension.width;
                    intraplay_yuv422_buffer.img_height = display_dimension.height;
                    intraplay_yuv422_buffer.img_offset_x = display_dimension.offset_x;
                    intraplay_yuv422_buffer.img_offset_y = display_dimension.offset_y;
                    //LOG_NOTICE("extradata %d:\n", mediainfo.video_extradata_len);
                    //gfPrintMemory(mediainfo.p_video_extradata, mediainfo.video_extradata_len);
                    //LOG_NOTICE("frame data (first 16 bytes):\n");
                    //gfPrintMemory(data_buffer->p_buffer, 16);
                    err = deviceDecodeIntraframe(data_buffer->p_buffer, data_buffer->data_size, mediainfo.p_video_extradata, mediainfo.video_extradata_len, &intraplay_yuv420_buffer);
                    if (EECode_OK != err) {
                      LOG_ERROR("decode intra fail, ret 0x%08x, %s\n", err, gfGetErrorCodeString(err));
                      mpMediaProbe->ReleaseFrame(data_buffer);
                      mpMediaProbe->Close();
                      return err;
                    }
                    err = deviceYUV2YUV(&intraplay_yuv420_buffer, &intraplay_yuv422_buffer);
                    if (EECode_OK != err) {
                      LOG_ERROR("yuv2yuv fail, ret 0x%08x, %s\n", err, gfGetErrorCodeString(err));
                      mpMediaProbe->ReleaseFrame(data_buffer);
                      mpMediaProbe->Close();
                      return err;
                    }
                    mpMediaProbe->ReleaseFrame(data_buffer);
                    mpMediaProbe->Close();
                    show_thumb = 1;
                  } else {
                    LOG_FATAL("no memory\n");
                  }
                } else {
                  LOG_ERROR("open file(%s) fail, ret 0x%08x, %s\n", context->name, err, gfGetErrorCodeString(err));
                }
              } else {
                LOG_FATAL("NULL context\n");
              }
              node = mpDisplayItemList->NextNode(node);
            } else {
              LOG_FATAL("NULL node\n");
              break;
            }
          }
          if (show_thumb) {
            intraplay_yuv422_buffer.buf_width = mDisplayDeviceWidth;
            intraplay_yuv422_buffer.buf_height = mDisplayDeviceHeight;
            intraplay_yuv422_buffer.img_width = mDisplayDeviceWidth;
            intraplay_yuv422_buffer.img_height = mDisplayDeviceHeight;
            intraplay_yuv422_buffer.img_offset_x = 0;
            intraplay_yuv422_buffer.img_offset_y = 0;
            err = deviceDisplay(&intraplay_yuv422_buffer);
            if (EECode_OK != err) {
              LOG_ERROR("display fail, ret 0x%08x, %s\n", err, gfGetErrorCodeString(err));
              mpMediaProbe->ReleaseFrame(data_buffer);
              mpMediaProbe->Close();
              return err;
            }
#if 0
            p_dump = fopen("luma1.dump", "wb+");
            if (p_dump) {
              printf("lu, start 0x%08x, lu offset 0x%08x, ch offset 0x%08x, pitch %d, width %d, height %d\n", (TU32) mMapIntraPB.base,
                     intraplay_yuv422_buffer.lu_buf_offset, intraplay_yuv422_buffer.ch_buf_offset,
                     intraplay_yuv422_buffer.buf_pitch, intraplay_yuv422_buffer.buf_width, intraplay_yuv422_buffer.buf_height);
              fwrite((TU8 *) mMapIntraPB.base + intraplay_yuv422_buffer.lu_buf_offset, 1, intraplay_yuv422_buffer.buf_pitch * intraplay_yuv422_buffer.buf_height, p_dump);
              fflush(p_dump);
              fclose(p_dump);
            }
            p_dump = fopen("chroma1.dump", "wb+");
            if (p_dump) {
              printf("ch, start 0x%08x, lu offset 0x%08x, ch offset 0x%08x, pitch %d, width %d, height %d\n", (TU32) mMapIntraPB.base,
                     intraplay_yuv422_buffer.lu_buf_offset, intraplay_yuv422_buffer.ch_buf_offset,
                     intraplay_yuv422_buffer.buf_pitch, intraplay_yuv422_buffer.buf_width, intraplay_yuv422_buffer.buf_height);
              fwrite((TU8 *) mMapIntraPB.base + intraplay_yuv422_buffer.ch_buf_offset, 1, intraplay_yuv422_buffer.buf_pitch * intraplay_yuv422_buffer.buf_height / 2, p_dump);
              fflush(p_dump);
              fclose(p_dump);
            }
            p_dump = NULL;
#endif
          }
        }
        break;
      default:
        LOG_FATAL("bad display layout type %d\n", mCurrentDisplayLayout);
        break;
    }
  }
  return EECode_OK;
}

EECode CThumbPlay::DisplayThumb2Buffer(TU32 begin_index)
{
  LOG_FATAL("to do\n");
  return EECode_NoImplement;
}

void CThumbPlay::Destroy()
{
  if (mpDir) {
    if (mpDirTree) {
      mpDir->CleanTree(mpDirTree);
      mpDirTree = NULL;
    }
    mpDir->Delete();
    mpDir = NULL;
  }
  if (mbDeviceInDecodeMode) {
    deviceDestroyContext();
    mbDeviceInDecodeMode = 0;
  }
  if (0 <= mDeviceHandle) {
    closeDevice();
  }
}

CIDoubleLinkedList::SNode *CThumbPlay::findNodeFromIndex(CIDoubleLinkedList *list, TU32 index)
{
  TU32 i = 0;
  CIDoubleLinkedList::SNode *node = list->FirstNode();
  while (node) {
    if (index == i) {
      return node;
    }
    i ++;
    node = list->NextNode(node);
  }
  return NULL;
}

void CThumbPlay::rectangleLayout(SThumbPlayDisplayDimension *display, TU32 index)
{
  TU32 x = 0, y = 0;
  if (display && mRectangleParams.rows && mRectangleParams.columns) {
    if (index < (TU32)(mRectangleParams.rows * mRectangleParams.columns)) {
      y = index / mRectangleParams.columns;
      x = index - (y * mRectangleParams.columns);
      display->width = mRectangleParams.rect_width - (mRectangleParams.border_x * 2);
      display->height = mRectangleParams.rect_height - (mRectangleParams.border_y * 2);
      display->offset_x = mRectangleParams.rect_width * x + mRectangleParams.border_x;
      display->offset_y = mRectangleParams.rect_height * y + mRectangleParams.border_y;
      return;
    } else {
      LOG_FATAL("index %d exceed max %dx%d\n", index, mRectangleParams.rows, mRectangleParams.columns);
      return;
    }
  } else {
    LOG_FATAL("bad params: display %p, rect %dx%d\n", display, mRectangleParams.rows, mRectangleParams.columns);
    return;
  }
}

EECode CThumbPlay::openDevice()
{
  mDeviceHandle = gfOpenIAVHandle();
  if (0 > mDeviceHandle) {
    LOG_ERROR("open iav handle fail, ret %d\n", mDeviceHandle);
    return EECode_OSError;
  }
  gfSetupDSPAlContext(&mfDSPAL);
  return EECode_OK;
}

void CThumbPlay::closeDevice()
{
  gfCloseIAVHandle(mDeviceHandle);
  mDeviceHandle = -1;
}

EECode CThumbPlay::deviceSetupContext()
{
  EECode err = EECode_OK;
  err = deviceEnterDecodeMode();
  if (EECode_OK != err) {
    LOG_FATAL("enter decode mode fail\n");
    return err;
  }
  err = deviceCreateDecoder(0, EAMDSP_VIDEO_CODEC_TYPE_H264, mCapMaxCodedWidth, mCapMaxCodedHeight);
  if (EECode_OK != err) {
    LOG_FATAL("create decoder fail\n");
    return err;
  }
  DASSERT(mfDSPAL.f_speed);
  DASSERT(mfDSPAL.f_start);
  mfDSPAL.f_start(mDeviceHandle, 0);
  mfDSPAL.f_speed(mDeviceHandle, 0, 0x100, EAMDSP_PB_SCAN_MODE_ALL_FRAMES, EAMDSP_PB_DIRECTION_FW);
  return EECode_OK;
}

EECode CThumbPlay::deviceDestroyContext()
{
  EECode err;
  mfDSPAL.f_stop(mDeviceHandle, 0, 1);
  err = deviceDestroyDecoder();
  if (EECode_OK != err) {
    LOG_FATAL("destroy decoder fail\n");
  }
  if (mbExitDecodeMode) {
    err = deviceLeaveDecodeMode();
    if (EECode_OK != err) {
      LOG_FATAL("leave decode mode fail\n");
    }
  }
  TInt ret = mfDSPAL.f_unmap_bsb(mDeviceHandle, &mMapBSB);
  if (0 > ret) {
    LOG_FATAL("unmap bsb fail\n");
  }
  return err;
}

EECode CThumbPlay::deviceEnterDecodeMode()
{
  TInt ret = 0;
  TInt vout_num = 0;
  TInt has_digital = 0, has_hdmi = 0, has_cvbs = 0;
  DASSERT(mfDSPAL.f_map_bsb);
  DASSERT(mfDSPAL.f_map_intrapb);
  DASSERT(mfDSPAL.f_get_vout_info);
  DASSERT(mfDSPAL.f_enter_mode);
  DASSERT(0 < mDeviceHandle);
  mMapBSB.b_two_times = 0;
  mMapBSB.b_enable_write = 1;
  mMapBSB.b_enable_read = 0;
  ret = mfDSPAL.f_map_bsb(mDeviceHandle, &mMapBSB);
  if (0 > ret) {
    LOG_FATAL("map bsb fail\n");
    return EECode_Error;
  }
  //disable here
#if 0
  ret = mfDSPAL.f_map_intrapb(mDeviceHandle, &mMapIntraPB);
  if (0 > ret) {
    LOG_FATAL("map intrapb fail\n");
    return EECode_Error;
  }
#endif
  ret = mfDSPAL.f_get_vout_info(mDeviceHandle, 0, EAMDSP_VOUT_TYPE_DIGITAL, &mVoutInfos[0]);
  if ((0 > ret) || (!mVoutInfos[0].width) || (!mVoutInfos[0].height)) {
    LOG_CONFIGURATION("digital vout not enabled\n");
    mbEnableVoutDigital = 0;
  } else {
    has_digital = 1;
    vout_num ++;
  }
  ret = mfDSPAL.f_get_vout_info(mDeviceHandle, 1, EAMDSP_VOUT_TYPE_HDMI, &mVoutInfos[1]);
  if ((0 > ret) || (!mVoutInfos[1].width) || (!mVoutInfos[1].height)) {
    LOG_CONFIGURATION("hdmi vout not enabled\n");
    mbEnableVoutHDMI = 0;
  } else {
    has_hdmi = 1;
    vout_num ++;
  }
  ret = mfDSPAL.f_get_vout_info(mDeviceHandle, 1, EAMDSP_VOUT_TYPE_CVBS, &mVoutInfos[2]);
  if ((0 > ret) || (!mVoutInfos[2].width) || (!mVoutInfos[2].height)) {
    LOG_CONFIGURATION("cvbs vout not enabled\n");
    mbEnableVoutCVBS = 0;
  } else {
    has_cvbs = 1;
    vout_num ++;
  }
  if (!vout_num) {
    LOG_FATAL("no vout found\n");
    return EECode_Error;
  }
  if ((!mbEnableVoutDigital) && (!mbEnableVoutHDMI) && (!mbEnableVoutCVBS)) {
    if (has_digital) {
      mbEnableVoutDigital = 1;
    } else if (has_hdmi) {
      mbEnableVoutHDMI = 1;
    } else if (has_cvbs) {
      mbEnableVoutCVBS = 1;
    }
    LOG_WARN("usr do not specify vout, guess default: cvbs %d, digital %d, hdmi %d\n", mbEnableVoutCVBS, mbEnableVoutDigital, mbEnableVoutHDMI);
  }
  if (mbEnableVoutDigital) {
    mDisplayDeviceWidth = mVoutInfos[0].width;
    mDisplayDeviceHeight = mVoutInfos[0].height;
  } else if (mbEnableVoutHDMI) {
    mDisplayDeviceWidth = mVoutInfos[1].width;
    mDisplayDeviceHeight = mVoutInfos[1].height;
  } else if (mbEnableVoutCVBS) {
    mDisplayDeviceWidth = mVoutInfos[2].width;
    mDisplayDeviceHeight = mVoutInfos[2].height;
  }
  LOG_CONFIGURATION("display device: %dx%d\n", mDisplayDeviceWidth, mDisplayDeviceHeight);
  if ((!mRectangleParams.tot_width)  || (!mRectangleParams.tot_height)) {
    mRectangleParams.tot_width = mDisplayDeviceWidth;
    mRectangleParams.tot_height = mDisplayDeviceHeight;
    if ((mRectangleParams.rows) && (mRectangleParams.rows)) {
      if ((mRectangleParams.tot_width < mRectangleParams.columns) || (mRectangleParams.tot_height < mRectangleParams.rows)) {
        LOG_ERROR("bad width height %d %d\n", mRectangleParams.tot_width, mRectangleParams.tot_height);
        return EECode_BadParam;
      }
      if ((mRectangleParams.tot_width < (mRectangleParams.columns * (1 + 2 * mRectangleParams.border_x))) || (mRectangleParams.tot_height < (mRectangleParams.rows * (1 + 2 * mRectangleParams.border_y)))) {
        LOG_ERROR("too small width height %d %d, columns row %d %d, border x %d, y %d\n", mRectangleParams.tot_width, mRectangleParams.tot_height, mRectangleParams.columns, mRectangleParams.rows, mRectangleParams.border_x, mRectangleParams.border_y);
        return EECode_BadParam;
      }
      mRectangleParams.rect_width = mRectangleParams.tot_width / mRectangleParams.columns;
      mRectangleParams.rect_height = mRectangleParams.tot_height / mRectangleParams.rows;
      mnTotalDisplayNumber = mRectangleParams.rows * mRectangleParams.columns;
    }
  }
  mVoutNumber = 1;
  mModeConfig.num_decoder = 1;
  mModeConfig.max_frm_width = mCapMaxCodedWidth;
  mModeConfig.max_frm_height = mCapMaxCodedHeight;
  LOG_NOTICE("enter decode mode...\n");
  ret = mfDSPAL.f_enter_mode(mDeviceHandle, &mModeConfig);
  if (0 > ret) {
    LOG_ERROR("enter decode mode fail, ret %d\n", ret);
    return EECode_Error;
  }
  LOG_NOTICE("enter decode mode done\n");
  return EECode_OK;
}

EECode CThumbPlay::deviceLeaveDecodeMode()
{
  TInt ret = 0;
  DASSERT(mfDSPAL.f_leave_mode);
  DASSERT(0 < mDeviceHandle);
  LOG_NOTICE("leave decode mode...\n");
  ret = mfDSPAL.f_leave_mode(mDeviceHandle);
  if (0 > ret) {
    LOG_ERROR("leave decode mode fail, ret %d\n", ret);
    return EECode_Error;
  }
  LOG_NOTICE("leave decode mode done\n");
  return EECode_OK;
}

EECode CThumbPlay::deviceCreateDecoder(TU8 decoder_id, TU8 decoder_type, TU32 width, TU32 height)
{
  TInt ret = 0;
  DASSERT(mfDSPAL.f_create_decoder);
  DASSERT(0 < mDeviceHandle);
  mDecoderInfo.decoder_id = decoder_id;
  mDecoderInfo.decoder_type = decoder_type;
  mDecoderInfo.width = width;
  mDecoderInfo.height = height;
  if (mbEnableVoutDigital) {
    mDecoderInfo.vout_configs[0].enable = 1;
    mDecoderInfo.vout_configs[0].vout_id = 0;
    mDecoderInfo.vout_configs[0].flip = mVoutInfos[0].flip;
    mDecoderInfo.vout_configs[0].rotate = mVoutInfos[0].rotate;
    mDecoderInfo.vout_configs[0].target_win_offset_x = mVoutInfos[0].offset_x;
    mDecoderInfo.vout_configs[0].target_win_offset_y = mVoutInfos[0].offset_y;
    mDecoderInfo.vout_configs[0].target_win_width = mVoutInfos[0].width;
    mDecoderInfo.vout_configs[0].target_win_height = mVoutInfos[0].height;
    mDecoderInfo.vout_configs[0].zoom_factor_x = (mVoutInfos[0].width * 0x10000) / width;
    mDecoderInfo.vout_configs[0].zoom_factor_y = (mVoutInfos[0].height * 0x10000) / height;
    mDecoderInfo.vout_configs[0].vout_mode = mVoutInfos[0].mode;
  } else if (mbEnableVoutHDMI) {
    mDecoderInfo.vout_configs[0].enable = 1;
    mDecoderInfo.vout_configs[0].vout_id = 1;
    mDecoderInfo.vout_configs[0].flip = mVoutInfos[1].flip;
    mDecoderInfo.vout_configs[0].rotate = mVoutInfos[1].rotate;
    mDecoderInfo.vout_configs[0].target_win_offset_x = mVoutInfos[1].offset_x;
    mDecoderInfo.vout_configs[0].target_win_offset_y = mVoutInfos[1].offset_y;
    mDecoderInfo.vout_configs[0].target_win_width = mVoutInfos[1].width;
    mDecoderInfo.vout_configs[0].target_win_height = mVoutInfos[1].height;
    mDecoderInfo.vout_configs[0].zoom_factor_x = (mVoutInfos[1].width * 0x10000) / width;
    mDecoderInfo.vout_configs[0].zoom_factor_y = (mVoutInfos[1].height * 0x10000) / height;
    mDecoderInfo.vout_configs[0].vout_mode = mVoutInfos[1].mode;
  } else if (mbEnableVoutCVBS) {
    mDecoderInfo.vout_configs[0].enable = 1;
    mDecoderInfo.vout_configs[0].vout_id = 1;
    mDecoderInfo.vout_configs[0].flip = mVoutInfos[2].flip;
    mDecoderInfo.vout_configs[0].rotate = mVoutInfos[2].rotate;
    mDecoderInfo.vout_configs[0].target_win_offset_x = mVoutInfos[2].offset_x;
    mDecoderInfo.vout_configs[0].target_win_offset_y = mVoutInfos[2].offset_y;
    mDecoderInfo.vout_configs[0].target_win_width = mVoutInfos[2].width;
    mDecoderInfo.vout_configs[0].target_win_height = mVoutInfos[2].height;
    mDecoderInfo.vout_configs[0].zoom_factor_x = (mVoutInfos[2].width * 0x10000) / width;
    mDecoderInfo.vout_configs[0].zoom_factor_y = (mVoutInfos[2].height * 0x10000) / height;
    mDecoderInfo.vout_configs[0].vout_mode = mVoutInfos[2].mode;
  }  else {
    LOG_FATAL("no vout\n");
    return EECode_Error;
  }
  mDecoderInfo.num_vout = mVoutNumber;
  ret = mfDSPAL.f_create_decoder(mDeviceHandle, &mDecoderInfo);
  if (0 > ret) {
    LOG_ERROR("create decoder fail, ret %d\n", ret);
    return EECode_Error;
  }
  mpBitStreamBufferCurPtr = mpBitSreamBufferStart = (TU8 *)mMapBSB.base + mDecoderInfo.bsb_start_offset;
  mpBitSreamBufferEnd = mpBitSreamBufferStart + mMapBSB.size;
  return EECode_OK;
}

EECode CThumbPlay::deviceDestroyDecoder()
{
  TInt ret = 0;
  DASSERT(mfDSPAL.f_destroy_decoder);
  DASSERT(0 < mDeviceHandle);
  LOG_NOTICE("destroy decoder...\n");
  ret = mfDSPAL.f_destroy_decoder(mDeviceHandle, 0);
  if (0 > ret) {
    LOG_ERROR("destroy decoder fail, ret %d\n", ret);
    return EECode_Error;
  }
  LOG_NOTICE("destroy decoder done\n");
  return EECode_OK;
}

TU8 *CThumbPlay::deviceCopyDataToBSB(TU8 *ptr, TU8 *buffer, TUint size)
{
  if (ptr + size <= mpBitSreamBufferEnd) {
    memcpy((void *)ptr, (const void *)buffer, size);
    return ptr + size;
  } else {
    TInt room = mpBitSreamBufferEnd - ptr;
    TU8 *ptr2;
    memcpy((void *)ptr, (const void *)buffer, room);
    ptr2 = buffer + room;
    size -= room;
    memcpy((void *)mpBitSreamBufferStart, (const void *)ptr2, size);
    return mpBitSreamBufferStart + size;
  }
}

EECode CThumbPlay::deviceDecodeIntraframe(TU8 *p_data, TU32 size, TU8 *pextra, TU32 extrasize, SAmbaDSPIntraplayBuffer *buffer)
{
  TInt ret = 0;
  SAmbaDSPIntraplayDecode intradec;
  DASSERT(mfDSPAL.f_request_bsb);
  DASSERT(mfDSPAL.f_intraplay_decode);
  if (mpBitStreamBufferCurPtr == mpBitSreamBufferEnd) {
    mpBitStreamBufferCurPtr = mpBitSreamBufferStart;
  }
  memset(&intradec, 0x0, sizeof(intradec));
  intradec.decoder_id = 0;
  intradec.num = 1;
  intradec.decode_type = EAMDSP_VIDEO_CODEC_TYPE_H264;
  if (0) {
    intradec.bitstreams[0].bits_fifo_start = (TU32)(mpBitStreamBufferCurPtr - mpBitSreamBufferStart);
    ret = mfDSPAL.f_request_bsb(mDeviceHandle, 0, size + extrasize + 1024, mpBitStreamBufferCurPtr);
    if (DUnlikely(0 > ret)) {
      LOG_ERROR("request bsb fail, return %d\n", ret);
      return EECode_Error;
    }
  } else {
    mpBitStreamBufferCurPtr = mpBitSreamBufferStart;
    intradec.bitstreams[0].bits_fifo_start = 0;
  }
  mpBitStreamBufferCurPtr = deviceCopyDataToBSB(mpBitStreamBufferCurPtr, pextra, extrasize);
  mpBitStreamBufferCurPtr = deviceCopyDataToBSB(mpBitStreamBufferCurPtr, p_data, size);
  intradec.bitstreams[0].bits_fifo_end = (TU32)(mpBitStreamBufferCurPtr - mpBitSreamBufferStart);
  intradec.buffers[0].buffer_id = buffer->buffer_id;
  intradec.buffers[0].ch_fmt = buffer->ch_fmt;
  intradec.buffers[0].buf_pitch = buffer->buf_pitch;
  intradec.buffers[0].buf_width = buffer->buf_width;
  intradec.buffers[0].buf_height = buffer->buf_height;
  intradec.buffers[0].lu_buf_offset = buffer->lu_buf_offset;
  intradec.buffers[0].ch_buf_offset = buffer->ch_buf_offset;
  intradec.buffers[0].img_width = buffer->img_width;
  intradec.buffers[0].img_height = buffer->img_height;
  intradec.buffers[0].img_offset_x = buffer->img_offset_x;
  intradec.buffers[0].img_offset_y = buffer->img_offset_y;
  intradec.buffers[0].buffer_size = buffer->buffer_size;
  ret = mfDSPAL.f_intraplay_decode(mDeviceHandle, &intradec);
  if (DUnlikely(0 > ret)) {
    LOG_ERROR("decode intra fail, return %d\n", ret);
    return EECode_Error;
  }
  return EECode_OK;
}

EECode CThumbPlay::deviceYUV2YUV(SAmbaDSPIntraplayBuffer *src_buffer, SAmbaDSPIntraplayBuffer *dst_buffer)
{
  TInt ret = 0;
  SAmbaDSPIntraplayYUV2YUV yy;
  DASSERT(mfDSPAL.f_intraplay_yuv2yuv);
  memset(&yy, 0x0, sizeof(yy));
  yy.decoder_id = 0;
  yy.num = 1;
  yy.rotate = 0;
  yy.flip = 0;
  yy.luma_gain = 50;
  yy.src_buf.buffer_id = src_buffer->buffer_id;
  yy.src_buf.ch_fmt = src_buffer->ch_fmt;
  yy.src_buf.buf_pitch = src_buffer->buf_pitch;
  yy.src_buf.buf_width = src_buffer->buf_width;
  yy.src_buf.buf_height = src_buffer->buf_height;
  yy.src_buf.lu_buf_offset = src_buffer->lu_buf_offset;
  yy.src_buf.ch_buf_offset = src_buffer->ch_buf_offset;
  yy.src_buf.img_width = src_buffer->img_width;
  yy.src_buf.img_height = src_buffer->img_height;
  yy.src_buf.img_offset_x = src_buffer->img_offset_x;
  yy.src_buf.img_offset_y = src_buffer->img_offset_y;
  yy.src_buf.buffer_size = src_buffer->buffer_size;
  yy.dst_buf[0].buffer_id = dst_buffer->buffer_id;
  yy.dst_buf[0].ch_fmt = dst_buffer->ch_fmt;
  yy.dst_buf[0].buf_pitch = dst_buffer->buf_pitch;
  yy.dst_buf[0].buf_width = dst_buffer->buf_width;
  yy.dst_buf[0].buf_height = dst_buffer->buf_height;
  yy.dst_buf[0].lu_buf_offset = dst_buffer->lu_buf_offset;
  yy.dst_buf[0].ch_buf_offset = dst_buffer->ch_buf_offset;
  yy.dst_buf[0].img_width = dst_buffer->img_width;
  yy.dst_buf[0].img_height = dst_buffer->img_height;
  yy.dst_buf[0].img_offset_x = dst_buffer->img_offset_x;
  yy.dst_buf[0].img_offset_y = dst_buffer->img_offset_y;
  yy.dst_buf[0].buffer_size = dst_buffer->buffer_size;
  ret = mfDSPAL.f_intraplay_yuv2yuv(mDeviceHandle, &yy);
  if (DUnlikely(0 > ret)) {
    LOG_ERROR("yuv2yuv fail, return %d\n", ret);
    return EECode_Error;
  }
  return EECode_OK;
}

EECode CThumbPlay::deviceDisplay(SAmbaDSPIntraplayBuffer *src_buffer)
{
  TInt ret = 0;
  SAmbaDSPIntraplayDisplay display;
  DASSERT(mfDSPAL.f_intraplay_display);
  memset(&display, 0x0, sizeof(display));
  display.decoder_id = 0;
  display.num = 1;
  DASSERT(EAMDSP_BUFFER_PIX_FMT_422 == src_buffer->ch_fmt);
  display.buffers[0].buffer_id = src_buffer->buffer_id;
  display.buffers[0].ch_fmt = src_buffer->ch_fmt;
  display.buffers[0].buf_pitch = src_buffer->buf_pitch;
  display.buffers[0].buf_width = src_buffer->buf_width;
  display.buffers[0].buf_height = src_buffer->buf_height;
  display.buffers[0].lu_buf_offset = src_buffer->lu_buf_offset;
  display.buffers[0].ch_buf_offset = src_buffer->ch_buf_offset;
  display.buffers[0].img_width = src_buffer->img_width;
  display.buffers[0].img_height = src_buffer->img_height;
  display.buffers[0].img_offset_x = src_buffer->img_offset_x;
  display.buffers[0].img_offset_y = src_buffer->img_offset_y;
  display.buffers[0].buffer_size = src_buffer->buffer_size;
  if (mbEnableVoutDigital) {
    display.desc[0].vout_id = 0;
  } else {
    display.desc[0].vout_id = 1;
  }
  display.desc[0].vid_win_update = 1;
  display.desc[0].vid_win_rotate = 0;
  display.desc[0].vid_flip = 0;
  display.desc[0].vid_win_width = mDisplayDeviceWidth;
  display.desc[0].vid_win_height = mDisplayDeviceHeight;
  display.desc[0].vid_win_offset_x = 0;
  display.desc[0].vid_win_offset_y = 0;
  ret = mfDSPAL.f_intraplay_display(mDeviceHandle, &display);
  if (DUnlikely(0 > ret)) {
    LOG_ERROR("display fail, return %d\n", ret);
    return EECode_Error;
  }
  return EECode_OK;
}

