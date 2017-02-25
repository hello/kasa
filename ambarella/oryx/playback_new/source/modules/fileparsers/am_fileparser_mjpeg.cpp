/*******************************************************************************
 * fileparser_mjpeg.cpp
 *
 * History:
 *  2016/04/26 - [Zhi He] create file
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

#include "am_native.h"
#include "am_native_log.h"

#include "am_internal.h"

#include "am_osal.h"
#include "am_framework.h"
#include "am_modules_interface.h"
#include "am_codec_interface.h"

#include "am_fileparser_mjpeg.h"

IMediaFileParser *gfCreateBasicMJPEGParser()
{
  return CMJPEGFileParser::Create();
}

CMJPEGFileParser *CMJPEGFileParser::Create()
{
  CMJPEGFileParser *result = new CMJPEGFileParser();
  if ((result) && (EECode_OK != result->Construct())) {
    result->Delete();
    result = NULL;
  }
  return result;
}

void CMJPEGFileParser::Delete()
{
  if (mpIO) {
    mpIO->Delete();
    mpIO = NULL;
  }
  if (mpReadBuffer) {
    DDBG_FREE(mpReadBuffer, "FPJV");
    mpReadBuffer = NULL;
  }
  delete this;
}

CMJPEGFileParser::CMJPEGFileParser()
  : mpIO(NULL)
  , mpReadBuffer(NULL)
  , mReadBufferSize(0)
  , mReadBlockSize(0)
  , mReadNextThreashold(0)
  , mpCurrentPtr(NULL)
  , mBufRemaingSize(0)
  , mFileTotalSize(0)
  , mFileCurrentOffset(0)
  , mbFileOpend(0)
  , mbFileEOF(0)
  , mbParseMediaInfo(0)
  , mbStartReadFile(0)
{
}

CMJPEGFileParser::~CMJPEGFileParser()
{
}

EECode CMJPEGFileParser::Construct()
{
  mReadBlockSize = 2 * 1024 * 1024;
  mReadNextThreashold = 1024 * 1024;
  mReadBufferSize = mReadBlockSize + mReadNextThreashold;
  mpReadBuffer = (TU8 *) DDBG_MALLOC(mReadBufferSize, "FPJV");
  if (DUnlikely(!mpReadBuffer)) {
    LOG_FATAL("Construct(), no memory, request size %" DPrint64u "\n", mReadBufferSize);
    mReadBufferSize = 0;
    return EECode_NoMemory;
  }
  mpIO = gfCreateIO(EIOType_File);
  if (DUnlikely(!mpIO)) {
    LOG_FATAL("Construct(), gfCreateIO() fail\n");
    return EECode_NoMemory;
  }
  memset(&mJpegInfo, 0x0, sizeof(mJpegInfo));
  return EECode_OK;
}

EECode CMJPEGFileParser::SetProperty(EMediaFileParserProperty property)
{
  return EECode_OK;
}

EECode CMJPEGFileParser::OpenFile(TChar *name)
{
  if (DUnlikely(mbFileOpend)) {
    LOG_WARN("close previous file first\n");
    mpIO->Close();
    mbFileOpend = 0;
  }
  EECode err = mpIO->Open(name, EIOFlagBit_Read | EIOFlagBit_Binary);
  if (DUnlikely(EECode_OK != err)) {
    LOG_ERROR("OpenFile(%s) fail, ret %d, %s\n", name, err, gfGetErrorCodeString(err));
    return err;
  }
  mpIO->Query(mFileTotalSize, mFileCurrentOffset);
  mbFileOpend = 1;
  return EECode_OK;
}

EECode CMJPEGFileParser::CloseFile()
{
  if (DUnlikely(!mbFileOpend)) {
    LOG_WARN("file not opened, or already closed\n");
    return EECode_OK;
  }
  mpIO->Close();
  mbFileOpend = 0;
  return EECode_OK;
}

EECode CMJPEGFileParser::SeekTo(TIOSize offset)
{
  if (DUnlikely(!mbFileOpend)) {
    LOG_ERROR("file not opened, or already closed\n");
    return EECode_BadState;
  }
  if (DUnlikely(offset >= mFileTotalSize)) {
    LOG_ERROR("SeekTo, offset >= total size\n");
    return EECode_BadParam;
  }
  mpIO->Seek(offset, EIOPosition_Begin);
  mFileCurrentOffset = offset;
  return EECode_OK;
}

EECode CMJPEGFileParser::ReadPacket(SMediaPacket *packet)
{
  TU8 *p_cur = NULL, *p_limit = NULL;
  TU8 segment_type = 0, start_image = 0, end_image = 0;
  TU32 segment_length = 0;
  TIOSize processing_size = 0;
  if ((!mbFileEOF) && (mBufRemaingSize < mReadNextThreashold)) {
    readDataFromFile();
  }
  p_cur = mpCurrentPtr;
  p_limit = mpCurrentPtr + mBufRemaingSize;
  if (p_cur >= p_limit) {
    DASSERT(p_cur == p_limit);
    DASSERT(mbFileEOF);
    return EECode_OK_EOF;
  }
  if ((EJPEG_MarkerPrefix != p_cur[0]) || (EJPEG_SOI != p_cur[1]) || (EJPEG_MarkerPrefix != p_cur[2])) {
    LOG_ERROR("bad jpeg header: %02x %02x %02x %02x\n", p_cur[0], p_cur[1], p_cur[2], p_cur[3]);
    return EECode_DataCorruption;
  }
  while (p_cur < p_limit) {
    if (EJPEG_MarkerPrefix == p_cur[0]) {
      segment_type = p_cur[1];
      if (EJPEG_SOI == segment_type) {
        start_image = 1;
        p_cur += 2;
      } else if (EJPEG_EOI == segment_type) {
        end_image = 1;
        p_cur += 2;
        break;
      } else if ((EJPEG_SOF0 <= segment_type) && (EJPEG_COMMENT >= segment_type)) {
        p_cur += 2;
        DBER16(segment_length, p_cur);
        p_cur += segment_length;
      } else {
        p_cur ++;
      }
    } else {
      p_cur ++;
    }
  }
  if (!start_image) {
    LOG_ERROR("do not find soi\n");
    return EECode_DataMissing;
  }
  if (!end_image) {
    LOG_ERROR("do not find eoi\n");
    return EECode_DataMissing;
  }
  processing_size = (TIOSize)(p_cur - mpCurrentPtr);
  packet->sub_packet_number = 0;
  packet->frame_type = EPredefinedPictureType_IDR;
  packet->p_data = mpCurrentPtr;
  packet->data_len = processing_size;
  packet->have_pts = 0;
  mpCurrentPtr += processing_size;
  mBufRemaingSize -= processing_size;
  if (mbFileEOF && (!mBufRemaingSize)) {
    packet->is_last_frame = 1;
  } else {
    packet->is_last_frame = 0;
  }
  return EECode_OK;
}

EECode CMJPEGFileParser::GetMediaInfo(SMediaInfo *info)
{
  if (!mbStartReadFile) {
    readDataFromFile();
  }
  if (!mbParseMediaInfo) {
    LOG_ERROR("can not find extradata, this file maybe not h265 file, or file is corrupted\n");
    return EECode_DataCorruption;
  }
  info->video_width = mJpegInfo.width;
  info->video_height = mJpegInfo.height;
  info->format = StreamFormat_JPEG;
  return EECode_OK;
}

EECode CMJPEGFileParser::GetExtradata(TU8 *&p_extradata, TU32 &extradata_size)
{
  p_extradata = NULL;
  extradata_size = 0;
  return EECode_OK;
}

EECode CMJPEGFileParser::getMediaInfo()
{
  TU8 *p_cur = mpCurrentPtr, *p_limit = mpCurrentPtr + mBufRemaingSize;
  TU8 segment_type, start_image = 0, start_scan = 0, start_frame = 0;
  TU32 segment_length = 0;
  SJPEGQtTable *p_cur_table = NULL;
  if ((EJPEG_MarkerPrefix != p_cur[0]) || (EJPEG_SOI != p_cur[1]) || (EJPEG_MarkerPrefix != p_cur[2])) {
    LOG_ERROR("bad jpeg header: %02x %02x %02x %02x\n", p_cur[0], p_cur[1], p_cur[2], p_cur[3]);
    return EECode_DataCorruption;
  }
  while (p_cur < p_limit) {
    if (EJPEG_MarkerPrefix != p_cur[0]) {
      LOG_ERROR("no marker prefix: %02x %02x %02x %02x\n", p_cur[0], p_cur[1], p_cur[2], p_cur[3]);
      return EECode_DataCorruption;
    }
    segment_type = p_cur[1];
    if (EJPEG_SOI == segment_type) {
      start_image = 1;
      p_cur += 2;
    } else if (EJPEG_EOI == segment_type) {
      start_image = 1;
      p_cur += 2;
      break;
    } else if (EJPEG_SOS == segment_type) {
      start_scan = 1;
      p_cur += 2;
      DBER16(segment_length, p_cur);
      p_cur += segment_length;
      break;
    } else if (EJPEG_DQT == segment_type) {
      if ((mJpegInfo.number_qt_tables + 1) >= DMAX_JPEG_QT_TABLE_NUMBER) {
        LOG_ERROR("too many tables? %d\n", mJpegInfo.number_qt_tables);
        return EECode_DataCorruption;
      }
      p_cur_table = &mJpegInfo.qt_tables[mJpegInfo.number_qt_tables];
      p_cur += 2;
      DBER16(segment_length, p_cur);
      p_cur_table->p_table = p_cur + 3;
      if (0 == (p_cur[2] >> 4)) {
        p_cur_table->length = 64;
      } else if (1 == (p_cur[2] >> 4)) {
        mJpegInfo.precision |= (1 << mJpegInfo.number_qt_tables);
        p_cur_table->length = 128;
      } else {
        LOG_FATAL("bad precise\n");
        return EECode_DataCorruption;
      }
      mJpegInfo.total_tables_length += p_cur_table->length;
      p_cur += segment_length;
      mJpegInfo.number_qt_tables ++;
      //LOG_PRINTF("dq table %d, length %d\n", mJpegInfo.number_qt_tables, segment_length);
    } else if ((EJPEG_APP_MIN <= segment_type) && (EJPEG_APP_MAX >= segment_type)) {
      p_cur += 2;
      DBER16(segment_length, p_cur);
      p_cur += segment_length;
      //LOG_PRINTF("skip EJPEG_APP, %02x, %d\n", segment_type, segment_length);
    } else if ((EJPEG_JPEG_MIN <= segment_type) && (EJPEG_JPEG_MAX >= segment_type)) {
      p_cur += 2;
      DBER16(segment_length, p_cur);
      p_cur += segment_length;
      //LOG_PRINTF("skip EJPEG_JPEG, %02x, %d\n", segment_type, segment_length);
    } else if ((EJPEG_REV_MIN <= segment_type) && (EJPEG_REV_MAX >= segment_type)) {
      p_cur += 2;
      DBER16(segment_length, p_cur);
      p_cur += segment_length;
      //LOG_PRINTF("skip EJPEG_REV, %02x, %d\n", segment_type, segment_length);
    } else {
      switch (segment_type) {
        case EJPEG_SOF0:
        case EJPEG_SOF1:
        case EJPEG_SOF2:
        case EJPEG_SOF3:
        case EJPEG_SOF5:
        case EJPEG_SOF6:
        case EJPEG_SOF7:
        case EJPEG_SOF8:
        case EJPEG_SOF9:
        case EJPEG_SOF10:
        case EJPEG_SOF11:
        case EJPEG_SOF13:
        case EJPEG_SOF14:
        case EJPEG_SOF15: {
            DASSERT(!start_frame);
            start_frame = 1;
            p_cur += 2;
            DBER16(segment_length, p_cur);
            TU8 *ptmp = p_cur + 3;
            DBER16(mJpegInfo.height, ptmp);
            ptmp += 2;
            DBER16(mJpegInfo.width, ptmp);
            ptmp += 2;
            TU32 ncomponents = ptmp[0];
            ptmp ++;
            TU32 sampling_factor_h[4] = {1};
            TU32 sampling_factor_v[4] = {1};
            if (3 == ncomponents) {
              LOG_PRINTF("jpeg: 3 components %02x %02x %02x.\n", ptmp[1], ptmp[4], ptmp[7]);
              sampling_factor_h[0] = (ptmp[1] >> 4) & 0xf;
              sampling_factor_v[0] = (ptmp[1]) & 0xf;
              sampling_factor_h[1] = (ptmp[4] >> 4) & 0xf;
              sampling_factor_v[1] = (ptmp[4]) & 0xf;
              sampling_factor_h[2] = (ptmp[7] >> 4) & 0xf;
              sampling_factor_v[2] = (ptmp[7]) & 0xf;
              if ((sampling_factor_h[1] == sampling_factor_h[2]) && ((sampling_factor_h[1] << 1) == sampling_factor_h[0])) {
                if (sampling_factor_v[1] == sampling_factor_v[2]) {
                  if ((sampling_factor_v[1] << 1) == sampling_factor_v[0]) {
                    mJpegInfo.type = EJpegTypeInFrameHeader_YUV420;
                  } else if (sampling_factor_v[1] == sampling_factor_v[0]) {
                    mJpegInfo.type = EJpegTypeInFrameHeader_YUV422;
                  } else {
                    LOG_ERROR("only support 420 and 422. %02x %02x %02x\n", ptmp[1], ptmp[4], ptmp[7]);
                    gfPrintMemory(ptmp, 16);
                    return EECode_BadFormat;
                  }
                } else {
                  LOG_ERROR("v sampling not expected. %02x %02x %02x\n", ptmp[1], ptmp[4], ptmp[7]);
                  gfPrintMemory(ptmp, 16);
                  return EECode_BadFormat;
                }
              } else {
                LOG_ERROR("h sampling not expected. %02x %02x %02x\n", ptmp[1], ptmp[4], ptmp[7]);
                gfPrintMemory(ptmp, 16);
                return EECode_BadFormat;
              }
            } else if (1 == ncomponents) {
              LOG_PRINTF("jpeg: 1 component %02x\n", ptmp[1]);
              mJpegInfo.type = EJpegTypeInFrameHeader_GREY8;
            } else {
              LOG_WARN("not support components %d\n", ncomponents);
            }
            p_cur += segment_length;
            //LOG_PRINTF("jpeg frame header, %02x, length %d, %dx%d, type %d\n", segment_type, segment_length, info->width, info->height, info->type);
          }
          break;
        case EJPEG_DHT:
        case EJPEG_DAT:
        case EJPEG_DNL:
        case EJPEG_DRI:
        case EJPEG_DHP:
        case EJPEG_EXP:
        case EJPEG_COMMENT:
          p_cur += 2;
          DBER16(segment_length, p_cur);
          p_cur += segment_length;
          //LOG_PRINTF("jpeg segment, %02x, %d\n", segment_type, segment_length);
          break;
        default:
          p_cur += 2;
          DBER16(segment_length, p_cur);
          p_cur += segment_length;
          LOG_WARN("unknown jpeg segment, %02x, %d\n", segment_type, segment_length);
          break;
      }
    }
  }
  if (!start_image) {
    LOG_ERROR("do not find soi\n");
    return EECode_DataMissing;
  }
  if (!start_frame) {
    LOG_ERROR("do not find sof\n");
    return EECode_DataMissing;
  }
  if (!start_scan) {
    LOG_ERROR("do not find sos\n");
    return EECode_DataMissing;
  }
  return EECode_OK;
}

EECode CMJPEGFileParser::readDataFromFile()
{
  TIOSize read_buf_size = mReadBlockSize;
  memcpy(mpReadBuffer, mpCurrentPtr, mBufRemaingSize);
  mpCurrentPtr = mpReadBuffer;
  EECode err = mpIO->Read(mpReadBuffer + mBufRemaingSize, 1, read_buf_size);
  if (DUnlikely(EECode_OK_EOF == err)) {
    mbFileEOF = 1;
    return EECode_OK;
  } else if (DUnlikely(EECode_OK != err)) {
    LOG_NOTICE("ReadDataFromFile(), mpIO->Read() ret %x, %s\n", err, gfGetErrorCodeString(err));
    mbFileEOF = 1;
    return err;
  }
  mbStartReadFile = 1;
  mBufRemaingSize += read_buf_size;
  if (DUnlikely(!mbParseMediaInfo)) {
    mbParseMediaInfo = 1;
    err = getMediaInfo();
  }
  return EECode_OK;
}

