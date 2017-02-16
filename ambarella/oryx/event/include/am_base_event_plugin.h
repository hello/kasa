/*******************************************************************************
 * am_base_event_plugin.h
 *
 * Histroy:
 *  2014-11-19  [Dongge Wu] created file
 *
 * Copyright (C) 2008-2016, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#ifndef AM_BASE_EVENT_PLUGIN_H_
#define AM_BASE_EVENT_PLUGIN_H_
/* All event plugins' interface */
class AMIEventPlugin
{
  public:
    virtual bool start_plugin()                                  = 0;
    virtual bool stop_plugin()                                   = 0;
    virtual bool set_plugin_config(EVENT_MODULE_CONFIG *pConfig) = 0;
    virtual bool get_plugin_config(EVENT_MODULE_CONFIG *pConfig) = 0;
    virtual EVENT_MODULE_ID get_plugin_ID()                      = 0;
    virtual ~AMIEventPlugin(){}
};

/* All event plugins must use this macro define under class define body */
#define DECLARE_EVENT_PLUGIN_INTERFACE \
  static AMIEventPlugin* create(EVENT_MODULE_ID moudle_id);

#define DECLARE_EVENT_PLUGIN_INIT(plName, mid) \
    AMIEventPlugin *instance; \
    void __attribute__((constructor)) create_instance() \
    { \
      instance = plName::create(mid); \
    }

#define DECLARE_EVENT_PLUGIN_FINIT \
    void __attribute__((destructor)) destroy_instance() \
    { \
      if (instance) delete instance; instance = NULL; \
    }

/* All event plugins must use this macro define at outside of class body*/
#define DECLARE_EVENT_PLUGIN_INIT_FINIT(pName, mid) \
    DECLARE_EVENT_PLUGIN_INIT(pName, mid) \
    DECLARE_EVENT_PLUGIN_FINIT

#endif

#ifdef BUILD_AMBARELLA_ORYX_CONF_DIR
#define ORYX_EVENT_CONF_DIR ((const char*)BUILD_AMBARELLA_ORYX_CONF_DIR)
#else
#define ORYX_EVENT_CONF_DIR ((const char*)"/etc/oryx")
#endif

#define ORYX_EVENT_CONF_SUB_DIR ((const char*)"/event/")
