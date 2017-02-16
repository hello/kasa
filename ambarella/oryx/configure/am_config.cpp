/*******************************************************************************
 * am_configure.cpp
 *
 * History:
 *   2014-7-28 - [ypchang] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
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
