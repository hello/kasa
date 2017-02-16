/*******************************************************************************
 * am_led_handler.h
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
#ifndef AM_LED_HANDLER_H_
#define AM_LED_HANDLER_H_

#include "am_led_handler_if.h"
#include <vector>
#include <atomic>

typedef std::vector<AMLEDConfig> AMLEDConfigList;

class AMLEDHandler:public AMILEDHandler
{
    friend class AMILEDHandler;

  public:
    virtual bool set_led_state(const AMLEDConfig &led_ins) override;
    virtual bool get_led_state(AMLEDConfig &led_ins) override;
    virtual bool deinit_led(const int32_t gpio_id)  override;
    virtual void deinit_all_led() override;
    virtual void print_led_list() override;

  protected:
    static AMLEDHandler *get_instance();
    virtual void release() override;
    virtual void inc_ref() override;

  private:
    AMLEDHandler();
    virtual ~AMLEDHandler();
    bool gpio_export(const int32_t gpio_id, const bool gpio_export);
    bool init_gpio(const AMLEDConfig &led_ins);
    bool set_gpio_direction_out(const int32_t gpio_id, const bool gpio_out);
    bool light_gpio_led(const int32_t gpio_id, const bool led_on);
    void del_all_led();
    void poll_to_light_led();
    static void blink_thread();
    AMLEDHandler(AMLEDHandler const &copy) = delete;
    AMLEDHandler& operator=(AMLEDHandler const &copy) = delete;

  private:
    int32_t m_blk_count;
    bool m_thread_run;
    AMLEDConfigList m_led_config;
    std::atomic_int m_ref_counter;
    static AMLEDHandler *m_instance;
};

#endif /* AM_LED_HANDLER_H_ */
