/*******************************************************************************
 * am_engine_config.h
 *
 * History:
 *   2014-12-30 - [ypchang] created file
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
#ifndef ORYX_STREAM_INCLUDE_COMMON_AM_ENGINE_CONFIG_H_
#define ORYX_STREAM_INCLUDE_COMMON_AM_ENGINE_CONFIG_H_

#include "am_amf_types.h"
#include "am_amf_interface.h"
#include "am_plugin.h"
#include "am_define.h"
#include "am_log.h"

struct ConnectionConfig
{
    ConnectionConfig **input;       /* Input filter list    */
    ConnectionConfig **output;      /* Output filter list   */
    uint32_t           input_number;  /* Input filter number  */
    uint32_t           output_number; /* Output filter number */
    std::string        filter;        /* Current Filter       */
    ConnectionConfig() :
      input(nullptr),
      output(nullptr),
      input_number(0),
      output_number(0)
    {}
    ~ConnectionConfig()
    {
      delete[] input;
      delete[] output;
    }
};

struct EngineConfig
{
    std::string      *filters;
    ConnectionConfig *connections;
    uint32_t          op_timeout;
    uint32_t          filter_num;
    EngineConfig() :
      filters(nullptr),
      connections(nullptr),
      op_timeout(0),
      filter_num(0)
     {}
    ~EngineConfig()
    {
      delete[] filters;
      delete[] connections;
    }
};

class AMPlugin;
struct EngineFilter
{
    AMPlugin     *so;
    AMIInterface *filter_obj;
    std::string   filter;
    EngineFilter() :
      so(NULL),
      filter_obj(NULL)
    {
      filter.clear();
    }
    ~EngineFilter()
    {
      AM_DESTROY(filter_obj);
      AM_DESTROY(so);
      DEBUG("~EngineFilter");
    }
};

#ifdef BUILD_AMBARELLA_ORYX_FILTER_DIR
#define ORYX_FILTER_DIR ((const char*)BUILD_AMBARELLA_ORYX_FILTER_DIR)
#else
#define ORYX_FILTER_DIR ((const char*)"/usr/lib/oryx/filter")
#endif

#ifdef BUILD_AMBARELLA_ORYX_CONF_DIR
#define ORYX_CONF_DIR ((const char*)BUILD_AMBARELLA_ORYX_CONF_DIR)
#else
#define ORYX_CONF_DIR ((const char*)"/etc/oryx")
#endif

#endif /* ORYX_STREAM_INCLUDE_COMMON_AM_ENGINE_CONFIG_H_ */
