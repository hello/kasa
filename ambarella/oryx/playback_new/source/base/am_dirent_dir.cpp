/*******************************************************************************
 * am_dirent_dir.cpp
 *
 * History:
 *  2016/03/08 - [Zhi He] create file
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

#include "am_dirent_dir.h"

#include "dirent.h"
#include <sys/stat.h>

static TInt __get_ext_n(TChar *filename, TChar *ext, TInt n)
{
  char *p0 = strrchr(filename, '.');
  if (!p0)
  { return 0; }
  strncpy(ext, p0 + 1, n);
  return 1;
}

IDirANSI *gfCreateDirentDirANSI()
{
  return CDirentDirANSI::Create();
}

IDirANSI *CDirentDirANSI::Create()
{
  CDirentDirANSI *result = new CDirentDirANSI();
  if ((result) && (EECode_OK != result->Construct())) {
    result->Delete();
    result = NULL;
  }
  return result;
}

void CDirentDirANSI::Delete()
{
}

CDirentDirANSI::CDirentDirANSI()
  : IDirANSI()
{
}

CDirentDirANSI::~CDirentDirANSI()
{
}

EECode CDirentDirANSI::Construct()
{
  return EECode_OK;
}

EECode CDirentDirANSI::ReadDir(TChar *name, TU32 max_depth, SDirNodeANSI  *&p_dir)
{
  EECode err = EECode_OK;
  mMaxDepth = max_depth;
  p_dir = allocNode();
  if (DLikely(p_dir)) {
    err = readDir(name, 1, p_dir);
    if (DUnlikely(EECode_OK != err)) {
      LOG_ERROR("readDir fail, ret 0x%08x, %s\n", err, gfGetErrorCodeString(err));
      cleanNode(p_dir);
      p_dir = NULL;
      return err;
    }
  } else {
    LOG_FATAL("no memory\n");
    releaseNode(p_dir);
    p_dir = NULL;
    return EECode_NoMemory;
  }
  return EECode_OK;
}

EECode CDirentDirANSI::readDir(TChar *name, TU32 depth, SDirNodeANSI *p_dir)
{
  DIR *dir;
  EECode err = EECode_OK;
  TInt len = 0;
  TChar fullpath[DMAX_PATHFILE_NANE_LENGTH], currfile[DMAX_PATHFILE_NANE_LENGTH];
  struct dirent *s_dir;
  struct stat file_stat;
  len = strlen(name);
  if ((len + 1) > DMAX_PATHFILE_NANE_LENGTH) {
    LOG_ERROR("path too long %d\n", len);
    return EECode_BadParam;
  }
  strcpy(fullpath, name);
  dir = opendir(fullpath);
  if (!dir) {
    LOG_ERROR("open dir(%s) fail, depth %d\n", fullpath, depth);
    return EECode_BadParam;
  }
  while (NULL != (s_dir = readdir(dir))) {
    if (!(strcmp(s_dir->d_name, ".")) || !(strcmp(s_dir->d_name, ".."))) {
      continue;
    }
    snprintf(currfile, DMAX_PATHFILE_NANE_LENGTH, "%s/%s", fullpath, s_dir->d_name);
    LOG_NOTICE("process file: %s\n", currfile);
    stat(currfile, &file_stat);
    len = strlen(currfile);
    if (S_ISDIR(file_stat.st_mode)) {
      if ((!mMaxDepth) || (mMaxDepth >= depth)) {
        SDirNodeANSI *p_new = allocNode();
        if (p_new) {
          p_new->is_dir = 1;
          appendChild(p_dir, p_new);
          err = readDir(currfile, depth + 1, p_new);
          if (EECode_OK != err) {
            LOG_ERROR("readDir fail, ret 0x%08x, %s, depth %d\n", err, gfGetErrorCodeString(err), depth);
            closedir(dir);
            return err;
          }
        } else {
          LOG_FATAL("no memory\n");
          closedir(dir);
          return EECode_NoMemory;
        }
      }
    } else {
      SDirNodeANSI *p_new = allocNode();
      if (p_new) {
        p_new->is_dir = 0;
        p_new->name = (TChar *) DDBG_MALLOC(len + 4, "DANM");
        if (p_new->name) {
          snprintf(p_new->name, DMAX_FILE_EXTERTION_NANE_LENGTH, "%s", currfile);
          __get_ext_n(currfile, p_new->file_ext, DMAX_FILE_EXTERTION_NANE_LENGTH - 1);
          LOG_NOTICE("append file: %s, ext %s\n", p_new->name, p_new->file_ext);
          appendChild(p_dir, p_new);
        } else {
          releaseNode(p_new);
          LOG_FATAL("no memory\n");
          closedir(dir);
          return EECode_NoMemory;
        }
      } else {
        LOG_FATAL("no memory\n");
        closedir(dir);
        return EECode_NoMemory;
      }
    }
  }
  closedir(dir);
  return EECode_OK;
}

