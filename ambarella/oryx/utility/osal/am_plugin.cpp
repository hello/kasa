/*******************************************************************************
 * am_plugin.cpp
 *
 * History:
 *   2014-7-18 - [ypchang] created file
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
