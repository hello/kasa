/*******************************************************************************
 * am_vout.h
 *
 * Histroy:
 *  2012-3-6 2012 - [ypchang] created file
 *  2014-6-24 2014 - [Louis] modify this for Oryx MW
 *
 * Copyright (C) 2008-2011, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#ifndef AMVOUT_H_
#define AMVOUT_H_

/* AMVout is for "VOUT mode" control,
 * which is a DSP working mode and DSP control.
 * AMVoutPort is the physical Port used in VOUT mode,
 * like HDMI, CVBS, LCD, digital vout,
 */
#include "am_video_types.h"
#include "am_video_param.h"

class AMVoutPort
{
  public:
    AMVoutPort(AM_VOUT_ID id, AM_VOUT_MODE mode) :
      m_type(AM_VOUT_TYPE_NONE),
      m_iav(-1),
      m_sink_id(-1),
      m_id(id),
      m_mode(mode)
  {
  }
    virtual ~AMVoutPort()
    {
    }
    virtual AM_RESULT init(int iav_hd, AMVoutParam *param); //default init is to turn off vout
    virtual AM_RESULT halt();
    virtual AM_RESULT set_mode(AM_VOUT_MODE mode);
    AM_VOUT_TYPE m_type;

  protected:
    int vout_get_sink_id();
    AM_RESULT select_dev();
    AM_RESULT config_sink(AMVoutParam *param);

    int m_iav;
    int m_sink_id;
    AM_VOUT_ID m_id;
    AM_VOUT_MODE m_mode;
};

class AMHDMIVoutPort: public AMVoutPort
{
  public:
    AMHDMIVoutPort(AM_VOUT_ID id, AM_VOUT_MODE mode) :
      AMVoutPort(id, mode)
  {
      m_type = AM_VOUT_TYPE_HDMI;
  }
    virtual ~AMHDMIVoutPort()
    {
    }
    virtual AM_RESULT init(int iav_hd, AMVoutParam *param);
    virtual AM_RESULT halt();
};

class AMLCDVoutPort: public AMVoutPort
{
  public:
    AMLCDVoutPort(AM_VOUT_ID id, AM_VOUT_MODE mode) :
      AMVoutPort(id, mode)
  {
      m_type = AM_VOUT_TYPE_LCD;
  }
    virtual ~AMLCDVoutPort()
    {
    }
    virtual AM_RESULT init(int iav_hd, AMVoutParam *param);
    virtual AM_RESULT halt();
    /* LCD Specific Methods */
    virtual AM_RESULT lcd_reset()
    {
      return AM_RESULT_ERR_PERM;
    }
    virtual AM_RESULT lcd_power_on()
    {
      return AM_RESULT_ERR_PERM;
    }
    virtual AM_RESULT lcd_backlight_on()
    {
      return AM_RESULT_ERR_PERM;
    }
    virtual AM_RESULT lcd_pwm_set_brightness(uint32_t brightness)
    {
      return AM_RESULT_ERR_PERM;
    }
    virtual AM_RESULT lcd_pwm_get_max_brightness(int32_t &value)
    {
      return AM_RESULT_ERR_PERM;
    }
    virtual AM_RESULT lcd_pwm_get_current_brightness(int32_t &value)
    {
      return AM_RESULT_ERR_PERM;
    }
    virtual const char* lcd_spi_dev_node()
    {
      return NULL;
    }
};

class AMCVBSVoutPort: public AMVoutPort
{
  public:
    AMCVBSVoutPort(AM_VOUT_ID id, AM_VOUT_MODE mode) :
      AMVoutPort(id, mode)
  {
      m_type = AM_VOUT_TYPE_CVBS;
  }
    virtual ~AMCVBSVoutPort()
    {
    }
    virtual AM_RESULT init(int iav_hd, AMVoutParam *param);
    virtual AM_RESULT halt();
};

class AMDigitalVoutPort: public AMVoutPort
{
  public:
    AMDigitalVoutPort(AM_VOUT_ID id, AM_VOUT_MODE mode) :
      AMVoutPort(id, mode)
  {
      m_type = AM_VOUT_TYPE_DIGITAL;
  }
    virtual ~AMDigitalVoutPort()
    {
    }
    virtual AM_RESULT init(int iav_hd, AMVoutParam *param);
    virtual AM_RESULT halt();
};

class AMVoutFrameBufferOSD
{
  public:
    AMVoutFrameBufferOSD(){}
    virtual ~AMVoutFrameBufferOSD(){}

    AM_RESULT init(int iav_hd);
    AM_RESULT color_conversion_switch(bool onoff);
    AM_RESULT video_layer_switch(bool onoff);
    AM_RESULT set_framebuffer_id(int32_t fb);
    AM_RESULT osd_flip(int32_t flipInfo);
    AM_RESULT change_osd_offset(uint32_t offx, uint32_t offy, bool center);
    AM_RESULT set_framebuffer_bg_color(uint8_t r, uint8_t g, uint8_t b);
    AM_RESULT clear_framebuffer();
};

class AMVout
{
  public:
    AMVout();
    virtual ~AMVout();
    virtual AM_RESULT init(int iav_hd, int vout_id, AMVoutParam *vout_param);
    virtual AM_RESULT shutdown(); //only run once
    virtual AM_RESULT reset();

    virtual AM_RESULT set_mode();
    virtual AM_RESULT get_mode(AM_VOUT_MODE *mode);

    virtual AM_RESULT set_fps();
    virtual AM_RESULT get_fps(AM_VIDEO_FPS *fps); //VOUT FPS

    /*VOUT state , means not working, or working.*/
    virtual AM_RESULT get_state(AM_VOUT_STATE  *state);

    virtual AM_RESULT flip();
    virtual AM_RESULT color_conversion(AM_VOUT_COLOR_CONVERSION conversion);

    /*
    virtual bool load_config(); //load Vout from config
    virtual bool save_config(); //save Vout to config
     */
    virtual AM_RESULT change(AMVoutParam *vout_param);
    virtual AM_RESULT update();  //refresh from config

  protected:
    /* below are some helper functions which are stateless
     * and work all the time
     */
    AM_RESULT video_fps_to_q9_format(AM_VIDEO_FPS *fps, AMVideoFpsFormatQ9 *q9);
    AM_RESULT q9_format_to_videofps(AMVideoFpsFormatQ9 *q9, AM_VIDEO_FPS *fps);

    AMVoutPort * create_vout_port();
    AM_RESULT get_vout_port_config(AM_VOUT_TYPE *type,
                                   AM_VOUT_MODE *mode);

  public:
    AM_VOUT_ID   m_id;
    //One VOUT has 1 VoutPort, it's 1:1 map
    AMVoutPort  *m_port;
    // One VOUT has 1 FrameBuffer OSD, it's 1:1 map
    AMVoutFrameBufferOSD *m_framebuffer_osd;

  protected:
    int          m_iav;
    bool         m_init_done;

    //VOUT config attributes
    AMVoutParam  m_param;
};

#endif /* AMVOUT_H_ */
