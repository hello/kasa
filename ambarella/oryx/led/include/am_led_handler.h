/*******************************************************************************
 * am_led_handler.h
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
