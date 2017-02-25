/*******************************************************************************
 * am_configure.cpp
 *
 * History:
 *   2014-7-28 - [ypchang] created file
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
#include "am_define.h"
#include "am_log.h"
#include "am_configure.h"

#include "am_file.h"

AMConfig* AMConfig::create(const char *config)
{
  AMConfig *result = new AMConfig();
  if (AM_UNLIKELY(result && !result->init(config))) {
    delete result;
    result = NULL;
  }
  return result;
}

void AMConfig::destroy()
{
  delete this;
}

bool AMConfig::save()
{
  bool ret = false;
  std::string config_string = serialize();
  if (AM_LIKELY(config_string.length() > 0)) {
    AMFile file(filename.c_str());
    if (AM_LIKELY(file.open(AMFile::AM_FILE_CLEARWRITE))) {
      size_t written = 0;
      size_t total = config_string.length();
      const char *str = config_string.c_str();
      while (written < total) {
        ssize_t retval = file.write((void*)(str + written), (total - written));
        if (AM_LIKELY(retval > 0)) {
          written += retval;
        } else if (retval <= 0) {
          ERROR("Failed to write file %s: %s", file.name(), strerror(errno));
          break;
        }
      }
      ret = (written == total);
      file.close(true); /* Make sure file is written to storage */
    } else {
      ERROR("Failed to open %s for writing!", file.name());
    }
  }
  return ret;
}

bool AMConfig::init(const char *config)
{
  *this = fromFile(config);
  return (luaRef != -1);
}

AMConfig& AMConfig::operator=(const LuaTable &table)
{
  *((LuaTable*)this) = table;
  return *this;
}
