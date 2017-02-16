/**
 * am_event_monitor_if.h
 *
 *  History:
 *		Dec 19, 2014 - [Shupeng Ren] created file
 *
 * Copyright (C) 2007-2014, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

/*! @file am_event_monitor_if.h
 *  This file defines AMIEventMonitor
 */
#ifndef _AM_EVENT_MONITOR_IF_H_
#define _AM_EVENT_MONITOR_IF_H_

#include "am_pointer.h"
#include "am_event_types.h"

class AMIEventMonitor;

/*! @typedef AMIEventMonitorPtr
 *  Smart pointer type of AMIEventMonitor
 */
typedef AMPointer<AMIEventMonitor> AMIEventMonitorPtr;

/*! @class AMIEventMonitor
 *  @brief AMEventMonitor interface class
 */
class AMIEventMonitor
{
    friend AMIEventMonitorPtr;

  public:
    /*!
     * Constructor
     * @return AMIEventMonitorPtr object1`
     */
    static AMIEventMonitorPtr get_instance();

    /*!
     * Check if an event plug-in module is registered
     * @param module_id @ref EVENT_MODULE_ID "event module ID"
     * @return true if registered, otherwise false
     */
    virtual bool is_module_registered(EVENT_MODULE_ID module_id) = 0;

    /*!
     * Register an event plug-in module
     * @param module_id @ref EVENT_MODULE_ID "event module ID"
     * @param module_path C style string containing module's absolute path
     * @return true if success, otherwise false
     */
    virtual bool register_module(EVENT_MODULE_ID module_id,
                                 const char *module_path) = 0;

    /*!
     * Start an event plug-in specified by its event ID
     * @param module_id @ref EVENT_MODULE_ID "event module ID"
     * @return true if success, otherwise false
     */
    virtual bool start_event_monitor(EVENT_MODULE_ID module_id) = 0;

    /*!
     * Stop an event plug-in specified by its event ID
     * @param module_id @ref EVENT_MODULE_ID "event module ID"
     * @return true if success, otherwise false
     */
    virtual bool stop_event_monitor(EVENT_MODULE_ID module_id) = 0;

    /*!
     * Destroy the object of an event plug-in specified by its event ID
     * @param module_id @ref EVENT_MODULE_ID "event module ID"
     * @return true if success, otherwise false
     */
    virtual bool destroy_event_monitor(EVENT_MODULE_ID module_id) = 0;

    /*!
     * Start all event plug-in module registered into event monitor
     * @return true if success, otherwise false
     * @sa stop_all_event_monitor
     * @sa register_module
     * @sa is_module_registered
     */
    virtual bool start_all_event_monitor() = 0;

    /*!
     * Stop all event plug-in module registered into event monitor
     * @return true if success, otherwise false
     * @sa start_all_event_monitor
     */
    virtual bool stop_all_event_monitor() = 0;

    /*!
     * Destroy all the object of event plug-in registered into event monitor
     * @return true if success, otherwise false
     * @sa destroy_event_monitor
     */
    virtual bool destroy_all_event_monitor() = 0;

    /*!
     * Set the configuration of an event plug-in module
     * @param module_id @ref EVENT_MODULE_ID "event module ID"
     * @param config @ref EVENT_MODULE_CONFIG "event module config pointer"
     * @return true if success, otherwise false
     */
    virtual bool set_monitor_config(EVENT_MODULE_ID module_id,
                                    EVENT_MODULE_CONFIG *config) = 0;

    /*!
     * Get the configuration of an event plug-in module
     * @param module_id @ref EVENT_MODULE_ID "event module ID"
     * @param config @ref EVENT_MODULE_CONFIG "event module config pointer"
     * @return true if success, otherwise false
     */
    virtual bool get_monitor_config(EVENT_MODULE_ID module_id,
                                    EVENT_MODULE_CONFIG *config) = 0;

  protected:
    virtual void release() = 0;
    virtual void inc_ref() = 0;
    virtual ~AMIEventMonitor() {}
};

#endif /* _AM_EVENT_MONITOR_IF_H_ */
