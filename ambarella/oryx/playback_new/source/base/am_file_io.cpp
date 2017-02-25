/**
 * am_file_io.cpp
 *
 * History:
 *  2013/09/09 - [Zhi He] create file
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

#include "am_file_io.h"
IIO *gfCreateFileIO()
{
  return CFileIO::Create();
}

IIO *CFileIO::Create()
{
  CFileIO *result = new CFileIO();
  if ((result) && (EECode_OK != result->Construct())) {
    result->Delete();
    result = NULL;
  }
  return result;
}

void CFileIO::Delete()
{
  if (mpFile) {
    fclose(mpFile);
    mpFile = NULL;
  }
}

CFileIO::CFileIO()
  : mpFile(NULL)
  , mpFileName(NULL)
  , mnFileNameSize(0)
  , mInitFlag(0x0)
  , mReadTotalFileSize(0)
  , mWirteCurrentFileSize(0)
{
}

CFileIO::~CFileIO()
{
  if (mpFile) {
    fclose(mpFile);
    mpFile = NULL;
  }
}

EECode CFileIO::Construct()
{
  return EECode_OK;
}

EECode CFileIO::Open(TChar *name, TU32 flags)
{
  const TChar *pmode = NULL;
  DASSERT(name);
  if (!name) {
    LOG_FATAL("NULL param\n");
    return EECode_BadParam;
  }
  DASSERT(!mpFile);
  if (mpFile) {
    fclose(mpFile);
    mpFile = NULL;
  }
  if (flags & EIOFlagBit_Read) {
    if (flags & EIOFlagBit_Binary) {
      pmode = "rb";
      mInitFlag = EIOFlagBit_Read | EIOFlagBit_Binary;
    } else if (flags & EIOFlagBit_Text) {
      pmode = "rt";
      mInitFlag = EIOFlagBit_Read | EIOFlagBit_Text;
    } else {
      LOG_ERROR("not specified read file's format, txt or binary\n");
      return EECode_BadParam;
    }
  } else if (flags & EIOFlagBit_Write) {
    if (flags & EIOFlagBit_Append) {
      if (flags & EIOFlagBit_Binary) {
        pmode = "wb+";
        mInitFlag = EIOFlagBit_Write | EIOFlagBit_Append | EIOFlagBit_Binary;
      } else if (flags & EIOFlagBit_Text) {
        pmode = "wt+";
        mInitFlag = EIOFlagBit_Write | EIOFlagBit_Append | EIOFlagBit_Text;
      } else {
        LOG_ERROR("not specified write append file's format, txt or binary\n");
        return EECode_BadParam;
      }
    } else {
      if (flags & EIOFlagBit_Binary) {
        pmode = "wb";
        mInitFlag = EIOFlagBit_Write | EIOFlagBit_Binary;
      } else if (flags & EIOFlagBit_Text) {
        pmode = "wt";
        mInitFlag = EIOFlagBit_Write | EIOFlagBit_Write;
      } else {
        LOG_ERROR("not specified write file's format, txt or binary\n");
        return EECode_BadParam;
      }
    }
  } else {
    LOG_ERROR("not specified file's operation, read or write\n");
    return EECode_BadParam;
  }
  mpFile = fopen((const TChar *)name, pmode);
  if (!mpFile) {
    perror("fopen");
    //LOG_NOTICE("open file fail, %s, pmode %s\n", name, pmode);
    return EECode_Error;
  }
  fseek(mpFile, 0, SEEK_END);
  mTotalFileSize = (TFileSize)ftell(mpFile);
  fseek(mpFile, 0, SEEK_SET);
  return EECode_OK;
}

EECode CFileIO::Close()
{
  if (mpFile) {
    fclose(mpFile);
    mpFile = NULL;
    mInitFlag = 0;
  }
  return EECode_OK;
}

EECode CFileIO::SetProperty(TIOSize write_block_size, TIOSize read_block_size)
{
  LOG_FATAL("TO DO\n");
  return EECode_OK;
}

EECode CFileIO::GetProperty(TIOSize &write_block_size, TIOSize &read_block_size) const
{
  LOG_FATAL("TO DO\n");
  return EECode_OK;
}

EECode CFileIO::Write(TU8 *pdata, TU32 size, TIOSize count, TU8 sync)
{
  DASSERT(count > 0);
  DASSERT(size);
  DASSERT(pdata);
  if (DUnlikely((!pdata) || (!size) || (0 >= count))) {
    LOG_FATAL("bad parameters pdata %p, size %d, count %" DPrint64d "\n", pdata, size, count);
    return EECode_BadParam;
  }
  DASSERT(mpFile);
  if (DUnlikely((!mpFile) || !(mInitFlag & EIOFlagBit_Write))) {
    LOG_FATAL("NULL mpFile %p, or mInitFlag (%08x) no write flag bit\n", mpFile, mInitFlag);
    return EECode_BadState;
  }
  //DASSERT(1 == sync);
  if (DUnlikely(0 == fwrite((void *)pdata, size, count, mpFile))) {
    LOG_ERROR("write data fail, size %d, count %" DPrint64d "\n", size, count);
    return EECode_IOError;
  }
  return EECode_OK;
}

EECode CFileIO::Read(TU8 *pdata, TU32 size, TIOSize &count)
{
  DASSERT(count > 0);
  DASSERT(size);
  DASSERT(pdata);
  if (DUnlikely((!pdata) || (!size) || (0 >= count))) {
    LOG_FATAL("bad parameters pdata %p, size %d, count %" DPrint64u "\n", pdata, size, count);
    return EECode_BadParam;
  }
  DASSERT(mpFile);
  if (DUnlikely((!mpFile) || !(mInitFlag & EIOFlagBit_Read))) {
    LOG_FATAL("NULL mpFile %p, or mInitFlag (%08x) no read flag bit\n", mpFile, mInitFlag);
    return EECode_BadState;
  }
  TIOSize ret = 0;
  ret = fread((void *)pdata, size, count, mpFile);
  if (DUnlikely(0 > ret)) {
    LOG_ERROR("read data fail, size %d, count %" DPrint64d "\n", size, count);
    return EECode_IOError;
  } else if (0 == ret) {
    count = 0;
    return EECode_OK_EOF;
  }
  count = ret;
  return EECode_OK;
}

EECode CFileIO::Seek(TIOSize offset, EIOPosition posision)
{
  DASSERT(mpFile);
  if (mpFile) {
    if (EIOPosition_Begin == posision) {
      fseek(mpFile, offset, SEEK_SET);
    } else if (EIOPosition_Current == posision) {
      fseek(mpFile, offset, SEEK_CUR);
    } else if (EIOPosition_End == posision) {
      fseek(mpFile, offset, SEEK_END);
    } else {
      LOG_FATAL("not specified posision %d\n", posision);
      return EECode_BadParam;
    }
  }
  return EECode_OK;
}

void CFileIO::Sync()
{
  DASSERT(mpFile);
  if (mpFile) {
    fflush(mpFile);
  }
}

EECode CFileIO::Query(TIOSize &total_size, TIOSize &current_posotion) const
{
  total_size = mTotalFileSize;
  current_posotion = (TIOSize)ftell(mpFile);
  return EECode_OK;
}

void *gfCreateFileDumper(TChar *filename)
{
  FILE *p = fopen((const TChar *)filename, "wb+");
  if (!p) {
    LOG_ERROR("open file (%s) fail\n", filename);
    return NULL;
  }
  return (void *) p;
}

void gfFileDump(void *context, TU8 *data, TU32 size)
{
  if (context) {
    fwrite(data, 1, size, (FILE *) context);
    fflush((FILE *) context);
  }
}

void gfDestroyFileDumper(void *context)
{
  if (context) {
    fclose((FILE *) context);
  }
}

