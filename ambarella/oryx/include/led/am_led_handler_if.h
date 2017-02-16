/*******************************************************************************
 * am_led_handler_if.h
 *
 * History:
 *   2015-1-27 - [longli] created file
 *
 * Copyright (C) 2008-2015, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/
/*! @file am_led_handler_if.h
 *  @brief This file defines AMILEDHandler
 */
#ifndef AM_LED_HANDLER_IF_H_
#define AM_LED_HANDLER_IF_H_

#include "am_pointer.h"

struct AMLEDConfig
{
    /* gpio index(devices.h) */
    int32_t gpio_id;
    /* Unit:100ms, 1 = 100ms */
    uint32_t on_time;
    uint32_t off_time;
    /* for internal use, do not set */
    uint32_t accum_time;
    bool blink_flag;
    bool led_on;
};

class AMILEDHandler;
typedef AMPointer<AMILEDHandler> AMILEDHandlerPtr;

class AMILEDHandler
{
    friend AMILEDHandlerPtr;
  public:
    static AMILEDHandlerPtr get_instance();
    virtual bool deinit_led(const int32_t gpio_id) = 0;
    virtual void deinit_all_led() = 0;
    virtual bool get_led_state(AMLEDConfig &led_ins) = 0;
    virtual bool set_led_state(const AMLEDConfig &led_ins) = 0;
    virtual void print_led_list() = 0;

  protected:
    virtual void release() = 0;
    virtual void inc_ref() = 0;
    virtual ~AMILEDHandler(){};
};

/*! @example test_led.cpp
 *  This file is the test program of AMILEDHandler.
 */

#endif /* AM_LED_HANDLER_IF_H_ */
