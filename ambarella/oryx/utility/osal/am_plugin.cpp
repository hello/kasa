/*******************************************************************************
 * am_plugin.cpp
 *
 * History:
 *   2014-7-18 - [ypchang] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co,Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/

#include "am_plugin.h"
#include "am_define.h"
#include "am_log.h"
#include "am_base_include.h"

#include <dlfcn.h>

AMPlugin* AMPlugin::create(const char *plugin)
{
  AMPlugin *result = new AMPlugin();
  if (AM_UNLIKELY(result && !result->init(plugin))) {
    delete result;
    result = NULL;
  }

  return result;
}

void* AMPlugin::get_symbol(const char *symbol)
{
  void *func = NULL;
  if (AM_LIKELY(symbol && m_handle)) {
    char *err = NULL;
    dlerror(); /* clear any existing errors */
    func = dlsym(m_handle, symbol);
    if (AM_LIKELY((err = dlerror()) != NULL)) {
      ERROR("Failed to get symbol %s: %s", symbol, err);
      func = NULL;
    }
  } else {
    if (AM_LIKELY(!m_handle)) {
      ERROR("Plugin is not loaded!");
    }
    if (AM_LIKELY(!symbol)) {
      ERROR("Invalid symbol!");
    }
  }

  return func;
}

void AMPlugin::destroy()
{
  if (-- m_ref <= 0) {
    delete this;
  }
}

void AMPlugin::add_ref()
{
  ++ m_ref;
}

bool AMPlugin::init(const char *plugin)
{
  bool ret = true;
  if (AM_LIKELY(plugin)) {
    m_plugin = plugin;
    dlerror(); /* clear any existing errors */
    m_handle = dlopen(plugin, RTLD_LAZY);
    if (AM_UNLIKELY(!m_handle)) {
      ERROR("Failed to open \"%s\"", plugin);
      ERROR("%s", dlerror());
      ret = false;
    } else {
      INFO("%s is loaded!", plugin);
    }
  } else {
    ERROR("Invalid plugin path!");
    ret = false;
  }

  return ret;
}

AMPlugin::AMPlugin() :
    m_handle(NULL),
    m_ref(1)
{
  m_plugin.clear();
}

AMPlugin::~AMPlugin()
{
  if (AM_LIKELY(m_handle)) {
    dlclose(m_handle);
    INFO("%s is unloaded!", m_plugin.c_str());
  }
}
