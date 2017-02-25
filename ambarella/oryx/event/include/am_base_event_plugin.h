/*******************************************************************************
 * am_base_event_plugin.h
 *
 * Histroy:
 *  2014-11-19  [Dongge Wu] created file
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
