/*******************************************************************************
 * am_led_handler_if.h
 *
 * History:
 *   2015-1-27 - [longli] created file
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
