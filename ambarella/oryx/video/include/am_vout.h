/**
 * am_vout.h
 *
 *  History:
 *    Jul 10, 2015 - [Shupeng Ren] created file
 *    Dec 8, 2015 - [Huaiqing Wang] reconstruct
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
 */
#ifndef ORYX_VIDEO_INCLUDE_AM_VOUT_H_
#define ORYX_VIDEO_INCLUDE_AM_VOUT_H_

#include "am_platform_if.h"

/* AMVout is for "VOUT mode" control,
 * which is a DSP working mode and DSP control.
 * AMVoutPort is the physical Port used in VOUT mode,
 * like HDMI, CVBS, LCD, digital vout,
 */

class AMVoutPort
{
  public:
    AMVoutPort(AM_VOUT_ID id);
    virtual ~AMVoutPort();

    virtual AM_RESULT init(const AMVoutParam &param);
    virtual AM_RESULT halt();

  protected:
    int32_t         m_sink_id;
    AM_VOUT_ID      m_id;
    AMIPlatformPtr  m_platform;
};

class AMHDMIVoutPort: public AMVoutPort
{
  public:
    AMHDMIVoutPort(AM_VOUT_ID id);
    virtual ~AMHDMIVoutPort();

    virtual AM_RESULT init(const AMVoutParam &param);
    virtual AM_RESULT halt();
};

class AMLCDVoutPort: public AMVoutPort
{
  public:
    AMLCDVoutPort(AM_VOUT_ID id);
    virtual ~AMLCDVoutPort();

    virtual AM_RESULT init(const AMVoutParam &param);
    virtual AM_RESULT halt();

    /* LCD Specific Methods */
    virtual AM_RESULT lcd_reset();
    virtual AM_RESULT lcd_power_on();
    virtual AM_RESULT lcd_backlight_on();
    virtual AM_RESULT lcd_pwm_set_brightness(uint32_t brightness);
    virtual AM_RESULT lcd_pwm_get_max_brightness(int32_t &value);
    virtual AM_RESULT lcd_pwm_get_current_brightness(int32_t &value);
    virtual const char* lcd_spi_dev_node();
};

class AMCVBSVoutPort: public AMVoutPort
{
  public:
    AMCVBSVoutPort(AM_VOUT_ID id);
    virtual ~AMCVBSVoutPort();

    virtual AM_RESULT init(const AMVoutParam &param);
    virtual AM_RESULT halt();
};

class AMYPbPrVoutPort: public AMVoutPort
{
  public:
    AMYPbPrVoutPort(AM_VOUT_ID id);
    virtual ~AMYPbPrVoutPort();

    virtual AM_RESULT init(const AMVoutParam &param);
    virtual AM_RESULT halt();
};

class AMVout
{
  public:
    static AMVout* create();
    void destroy();

    AM_RESULT load_config();
    AM_RESULT get_param(AMVoutParamMap &param);
    AM_RESULT set_param(const AMVoutParamMap &param);
    AM_RESULT shutdown(AM_VOUT_ID id);

    AM_RESULT start();
    AM_RESULT stop();

  private:
    AMVout();
    virtual ~AMVout();
    AM_RESULT init();
    AMVoutPort *create_vout_port(AM_VOUT_ID id, AM_VOUT_TYPE type);

  private:
    static  AMVout              *m_instance;
    AMVoutConfigPtr              m_config;
    AMVoutParamMap               m_param;
    map<AM_VOUT_ID, AMVoutPort*> m_port;
};

#endif /* ORYX_VIDEO_INCLUDE_AM_VOUT_H_ */
