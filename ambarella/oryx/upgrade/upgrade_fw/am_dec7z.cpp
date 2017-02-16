/*******************************************************************************
 * am_dec7z.cpp
 *
 * History:
 *   2015-3-9 - [longli] created file
 *
 * Copyright (C) 2008-2015, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/

#include "am_base_include.h"
#include <sys/stat.h>
#include <sys/types.h>
#include "am_log.h"
#include "am_dec7z.h"
#include "7zAlloc.h"
#include "7zCrc.h"
#include "7zVersion.h"
#include "Utf2str.h"
#include "errno.h"

#define BUFFER_SIZE 256

using namespace std;

AMDec7z::AMDec7z():
    m_f_open(false),
    m_ex_init(false)
{

}

AMDec7z::~AMDec7z()
{

  if (m_ex_init) {
    SzArEx_Free(&m_db, &m_alloc_imp);
    m_ex_init = false;
  }

  if (m_f_open) {
    File_Close(&m_archive_stream.file);
    m_f_open = false;
  }
}

void AMDec7z::destroy()
{
  delete this;
}

bool AMDec7z::create_dir(const string &path)
{
  bool ret = true;
  int32_t res;
  res = mkdir(path.c_str(), 0777) == 0 ? 0 : errno;
  if (!res || res == EEXIST) {
    ret = true;
  } else {
    ret = false;
  }

  return ret;
}

AMDec7z *AMDec7z::create(const string &filename)
{
  AMDec7z *result = new AMDec7z();
  if (result && !result->init(filename)) {
    DEBUG("Failed to create AMDec7z instance.");
    delete result;
    result = NULL;
  }

  return result;
}

bool AMDec7z::init(const string &filename)
{
  bool ret = true;

  do {
    SRes res;
    if (filename.empty()) {
      DEBUG("AMDec7z::init: file name is empty.");
      ret = false;
      break;
    }

    if (InFile_Open(&m_archive_stream.file, filename.c_str())) {
      DEBUG("Can't open this compressed files: %s, 7z file is invalid!",
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
      DEBUG("SzArEx_Open return error.");
      ret = false;
      break;
    }

  } while (0);

  return ret;
}

bool AMDec7z::get_filename_in_7z(string &name_list_got)
{
  bool ret = false;
  SRes res = SZ_OK;
  UInt16 *temp = NULL;
  size_t tempSize = 0;
  UInt32 i;


  name_list_got.clear();
  printf("File(s) in 7z:\n");
  for (i = 0; i < m_db.db.NumFiles; i ++) {
    const CSzFileItem *f = m_db.db.Files + i;
    size_t len;
    //size
    len = SzArEx_GetFileNameUtf16(&m_db, i, NULL);
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

    if (!f->IsDir) {
      CBuf buf;
      SRes res;
      Buf_Init(&buf);
      res = Utf16_To_Char(&buf, temp, 0);
      if (res == SZ_OK) {
        name_list_got = name_list_got + (const char *)buf.data + ":";
        printf("%s\n", (const char *)buf.data);
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

  do {
    if (!path.empty()) {
      if (access(path.c_str(), F_OK)) {
        if (!create_dir(path)) {
          printf("Failed to create dir %s\n", path.c_str());
        }
      }
    }

    for (i = 0; i < m_db.db.NumFiles; i ++) {
      size_t offset = 0;
      size_t outSizeProcessed = 0;
      const CSzFileItem *f = m_db.db.Files + i;
      size_t len;
      len = SzArEx_GetFileNameUtf16(&m_db, i, NULL);
      if (len > tempSize) {
        SzFree(NULL, temp);
        tempSize = len;
        temp = (UInt16 *) SzAlloc(NULL, tempSize * sizeof(temp[0]));
        if (temp == 0) {
          res = SZ_ERROR_MEM;
          break;
        }
      }
      SzArEx_GetFileNameUtf16(&m_db, i, temp);

      if (f->IsDir)
        continue;

      if (!f->IsDir) {
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
      UInt16 *name = (UInt16 *) temp;
      for (j = 0; name[j] != 0; j ++)
        if (name[j] == '/') {
          name[j] = 0;
          char fullname[BUFFER_SIZE] = "";
          strcpy(fullname, path.c_str());
          apend_utf16_to_Cstring(fullname, name);
          if (0 != (res = create_dir(fullname))) {
            ERROR("can not mkdir %s, return %d.\n", fullname, res);
            res = SZ_ERROR_FAIL;
            break;
          }
          name[j] = CHAR_PATH_SEPARATOR;
        }

      if (res != SZ_OK)
        break;

      if (f->IsDir) {
        char fullpath[BUFFER_SIZE] = "";
        strcpy(fullpath, path.c_str());
        apend_utf16_to_Cstring(fullpath, name);
        if (0 != (res = create_dir(fullpath))) {
          ERROR("can not mkdir %s, return %d.\n", fullpath, res);
          res = SZ_ERROR_FAIL;
          break;
        }
        continue;
      } else {
        char fullname[BUFFER_SIZE] = "";
        strcpy(fullname, path.c_str());
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
      printf("\nExtract 7z file successfully!\n");
      ret = true;
      break;
    } else if (res == SZ_ERROR_UNSUPPORTED) {
      ERROR("decoder doesn't support this archive");
    } else if (res == SZ_ERROR_MEM) {
      ERROR("can not allocate memory");
    } else if (res == SZ_ERROR_CRC) {
      ERROR("CRC error");
    } else {
      ERROR("\nERROR #%d\n", res);
    }
  } while (0);

  return ret;
}
