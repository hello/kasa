/*******************************************************************************
 * am_engine_config.h
 *
 * History:
 *   2014-12-30 - [ypchang] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
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
    uint32_t         input_number;  /* Input filter number  */
    uint32_t         output_number; /* Output filter number */
    std::string      filter;        /* Current Filter       */
    ConnectionConfig **input;       /* Input filter list    */
    ConnectionConfig **output;      /* Output filter list   */
    ConnectionConfig() :
      input_number(0),
      output_number(0),
      input(NULL),
      output(NULL)
    {}
    ~ConnectionConfig()
    {
      delete[] input;
      delete[] output;
    }
};

struct EngineConfig
{
    uint32_t          op_timeout;
    uint32_t          filter_num;
    std::string      *filters;
    ConnectionConfig *connections;
    EngineConfig() :
      op_timeout(0),
      filter_num(0),
      filters(NULL),
      connections(NULL){}
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
