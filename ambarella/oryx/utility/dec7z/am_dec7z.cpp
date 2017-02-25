/*******************************************************************************
 * am_dec7z.cpp
 *
 * History:
 *   2015-3-9 - [longli] created file
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

#include "am_base_include.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <mutex>
#include "am_log.h"
#include "am_dec7z_if.h"
#include "am_dec7z.h"
#include "7zAlloc.h"
#include "7zCrc.h"
#include "7zVersion.h"
#include "Utf2Str.h"
#include "errno.h"

#define BUFFER_SIZE 256

using namespace std;

static mutex m_mtx;
#define  DECLARE_MUTEX  lock_guard<mutex> lck(m_mtx);

AMDec7z::AMDec7z():
    m_ref_counter(0),
    m_f_open(false),
    m_ex_init(false)
{

}

AMDec7z::~AMDec7z()
{
  INFO("~AMDec7z");
  if (m_ex_init) {
    SzArEx_Free(&m_db, &m_alloc_imp);
    m_ex_init = false;
  }

  if (m_f_open) {
    File_Close(&m_archive_stream.file);
    m_f_open = false;
  }

}

void AMDec7z::release()
{
  DECLARE_MUTEX;
  INFO("AMDec7z release");
  if ((m_ref_counter >= 0) && (--m_ref_counter <= 0)) {
    NOTICE("This is the last reference of AMDec7z's object, "
        "delete object instance 0x%p", this);
    delete this;
  }

}

void AMDec7z::inc_ref()
{
  DECLARE_MUTEX;
  ++m_ref_counter;
}

int32_t AMDec7z::create_dir(const string &path)
{
  int32_t res = 0;
  uint32_t s_pos = 0;
  uint32_t e_pos = 0;
  string sub_str;

  do {

    if (path.empty()) break;

    while (s_pos != string::npos) {
      s_pos = path.find_first_not_of('/', s_pos);
      if (s_pos != string::npos) {
        e_pos = path.find_first_of('/', s_pos);
        sub_str = path.substr(0, e_pos);
        if (access(sub_str.c_str(), F_OK)) {
          res = mkdir(sub_str.c_str(), 0755) == 0 ? 0 : errno;
          if (!res || res == EEXIST) {
            res = 0;
          } else {
            break;
          }
        }
        s_pos = e_pos;
      }
    }
  } while (0);

  return res;
}

AMIDec7zPtr AMIDec7z::create(const string &filename)
{
  return ((AMIDec7z*)AMDec7z::create(filename));
}

AMDec7z *AMDec7z::create(const string &filename)
{
  AMDec7z *ins = new AMDec7z();

  if (ins && !ins->init(filename)) {
    INFO("Failed to create AMDec7z instance.");
    delete ins;
    ins = NULL;
  }

  return ins;
}

bool AMDec7z::init(const string &filename)
{
  bool ret = true;

  do {
    SRes res;
    if (filename.empty()) {
      WARN("AMDec7z::init: file name is empty.");
      ret = false;
      break;
    }

    if (InFile_Open(&m_archive_stream.file, filename.c_str())) {
      WARN("Can't open this compressed files: %s, 7z file is invalid!",
            filename.c_str());
      ret = false; //7z file invalid
      break;
    }

    m_f_open = true;
    m_alloc_imp.Alloc = SzAlloc;
    m_alloc_imp.Free = SzFree;

    m_alloc_temp_imp.Alloc = SzAllocTemp;
    m_alloc_temp_imp.Free = SzFreeTemp;

    FileInStream_CreateVTable(&m_archive_stream);
    LookToRead_CreateVTable(&m_look_stream, False);

    m_look_stream.realStream = &m_archive_stream.s;
    LookToRead_Init(&m_look_stream);

    CrcGenerateTable();

    SzArEx_Init(&m_db);
    m_ex_init = true;
    res = SzArEx_Open(&m_db, &m_look_stream.s, &m_alloc_imp, &m_alloc_temp_imp);
    if (res != 0) {
      WARN("SzArEx_Open return error.");
      ret = false;
      break;
    }

  } while (0);

  return ret;
}

bool AMDec7z::get_filename_in_7z(string &name_list_got)
{
  DECLARE_MUTEX;
  bool ret = false;
  SRes res = SZ_OK;
  UInt16 *temp = NULL;
  size_t tempSize = 0;
  UInt32 i;


  name_list_got.clear();
  INFO("File(s) in 7z:\n");
  for (i = 0; i < m_db.NumFiles; i ++) {
    UInt32 isDir = SzArEx_IsDir(&m_db, i);
    size_t len = SzArEx_GetFileNameUtf16(&m_db, i, NULL);

    if (len > tempSize) {
      SzFree(NULL, temp);
      tempSize = len;
      temp = (UInt16 *) SzAlloc(NULL, tempSize * sizeof(temp[0]));
      if (temp == 0) {
        res = SZ_ERROR_MEM;
        break;
      }
    }
    //file name
    SzArEx_GetFileNameUtf16(&m_db, i, temp);

    if (!isDir) {
      CBuf buf;
      SRes res;
      Buf_Init(&buf);
      res = Utf16_To_Char(&buf, temp);
      if (res == SZ_OK) {
        name_list_got = name_list_got + (const char *)buf.data + ",";
        INFO("%s\n", (const char *)buf.data);
      }
      Buf_Free(&buf, &g_Alloc);
    }
  }

  if (res == SZ_OK) {
    ret = true;
  }

  return ret;
}

bool AMDec7z::dec7z(const string &path)
{
  DECLARE_MUTEX;
  bool ret = false;
  SRes res = SZ_OK;
  UInt16 *temp = NULL;
  size_t tempSize = 0;
  UInt32 i;
  /* it can have any value before first call (if outBuffer = 0) */
  UInt32 blockIndex = 0xFFFFFFFF;
  /* it must be 0 before first call for each new archive. */
  Byte *outBuffer = 0;
  /* it can have any value before first call (if outBuffer = 0) */
  size_t outBufferSize = 0;
  int32_t str_len_base = 0;
  int32_t str_len = 0;
  string path_tmp(path);

  INFO("begin to decompress %s\n", path.c_str());

  do {
    if (!path_tmp.empty()) {
      str_len_base = path_tmp.length();
      if (path_tmp.at(str_len_base - 1) != '/') {
        path_tmp += "/";
        ++str_len_base;
      }

      if (access(path_tmp.c_str(), F_OK)) {
        if (create_dir(path_tmp)) {
          printf("Failed to create dir %s\n", path_tmp.c_str());
        }
      }
    }

    for (i = 0; i < m_db.NumFiles; i ++) {
      size_t offset = 0;
      size_t outSizeProcessed = 0;
      UInt32 isDir = SzArEx_IsDir(&m_db, i);
      size_t len = SzArEx_GetFileNameUtf16(&m_db, i, NULL);
      if (len > tempSize) {
        SzFree(NULL, temp);
        tempSize = len;
        temp = (UInt16*)SzAlloc(NULL, tempSize * sizeof(temp[0]));
        if (temp == 0) {
          res = SZ_ERROR_MEM;
          break;
        }
      }
      SzArEx_GetFileNameUtf16(&m_db, i, temp);

      if (isDir)
        continue;

      if (!isDir) {
        res = SzArEx_Extract(&m_db,
                             &m_look_stream.s,
                             i,
                             &blockIndex,
                             &outBuffer,
                             &outBufferSize,
                             &offset,
                             &outSizeProcessed,
                             &m_alloc_imp,
                             &m_alloc_temp_imp);
        if (res != SZ_OK)
          break;
      }

      //Extract
      CSzFile outFile;
      size_t processedSize;
      size_t j;
      UInt16 *name = (UInt16*)temp;

      for (j = 0; name[j] != 0; j++);
      str_len = str_len_base + j;
      if (str_len + 1 > BUFFER_SIZE) {
        ERROR("Directory path name is too long. Array overflow!\n");
        res = SZ_ERROR_FAIL;
        break;
      }

      for (j = 0; name[j] != 0; j ++)
        if (name[j] == '/') {
          name[j] = 0;
          char fullname[BUFFER_SIZE] = "";
          strcpy(fullname, path_tmp.c_str());
          apend_utf16_to_Cstring(fullname, name);
          if (access(fullname, F_OK)) {
            if (0 != (res = create_dir(fullname))) {
              ERROR("can not mkdir %s, return %d.\n", fullname, res);
              res = SZ_ERROR_FAIL;
              break;
            }
          }
          name[j] = CHAR_PATH_SEPARATOR;
        }

      if (res != SZ_OK)
        break;


      if (isDir) {
        char fullpath[BUFFER_SIZE] = "";
        strcpy(fullpath, path_tmp.c_str());
        apend_utf16_to_Cstring(fullpath, name);
        if (access(fullpath, F_OK)) {
          if (0 != (res = create_dir(fullpath))) {
            ERROR("can not mkdir %s, return %d.\n", fullpath, res);
            res = SZ_ERROR_FAIL;
            break;
          }
        }
        continue;
      } else {
        char fullname[BUFFER_SIZE] = "";
        strcpy(fullname, path_tmp.c_str());
        apend_utf16_to_Cstring(fullname, name);
        if (OutFile_Open(&outFile, fullname)) {
          ERROR("can not open output file %s\n", fullname);
          res = SZ_ERROR_FAIL;
          break;
        }
      }
      processedSize = outSizeProcessed;
      if (File_Write(&outFile, outBuffer + offset, &processedSize) != 0
          || processedSize != outSizeProcessed) {
        ERROR("can not write output file\n");
        res = SZ_ERROR_FAIL;
        break;
      }
      if (File_Close(&outFile)) {
        ERROR("can not close output file\n");
        res = SZ_ERROR_FAIL;
        break;
      }
    }

    IAlloc_Free(&m_alloc_imp, outBuffer);
    SzFree(NULL, temp);

    if (res == SZ_OK) {
      INFO("\nExtract 7z file successfully!\n");
      ret = true;
      break;
    } else if (res == SZ_ERROR_UNSUPPORTED) {
      ERROR("decoder doesn't support this archive");
    } else if (res == SZ_ERROR_MEM) {
      ERROR("can not allocate memory");
    } else if (res == SZ_ERROR_CRC) {
      ERROR("CRC error");
    } else {
      ERROR("ERROR #%d\n", res);
    }
  } while (0);

  return ret;
}
