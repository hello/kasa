/*******************************************************************************
 * am_vout.cpp
 *
 * History:
 *   2014-7-14 - [lysun] created file
 *
 * Copyright (C) 2008-2016, Ambarella Co,Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/
#include <sys/ioctl.h>
#include "am_base_include.h"
#include "am_video_dsp.h"
#include "am_vout.h"
#include "am_log.h"
#include "am_video_param.h"
#include "am_video_device.h"
#include "am_vout_trans.h"
#include "am_video_trans.h"
#include "iav_ioctl.h"

AMVout::AMVout() :
m_id(AM_VOUT_ID_INVALID),
m_port(NULL),
m_framebuffer_osd(NULL),
m_iav(-1),
m_init_done(false)
{
  m_param.type = AM_VOUT_TYPE_NONE;
  m_param.mode = AM_VOUT_MODE_AUTO;
  m_param.fps =  AM_VIDEO_FPS_AUTO;
  m_param.flip = AM_VIDEO_FLIP_NONE;
}

AMVout::~AMVout()
{
}

int AMVoutPort::vout_get_sink_id()
{
  //TODO:  convert AM_VOUT_TYPE into vout sink type
  int i;
  int num = 0;
  int sink_id = -1;
  int ret = -1;
  struct amba_vout_sink_info sink_info;

  do {
    if (ioctl(m_iav, IAV_IOC_VOUT_GET_SINK_NUM, &num) < 0) {
      PERROR("IAV_IOC_VOUT_GET_SINK_NUM");
      break;
    }
    if (num < 1) {
      ERROR("AMVoutPort: Please load vout driver!\n");
      break;
    }

    for (i = num - 1; i >= 0; i --) {
      sink_info.id = i;
      if (ioctl(m_iav, IAV_IOC_VOUT_GET_SINK_INFO, &sink_info) < 0) {
        PERROR("IAV_IOC_VOUT_GET_SINK_INFO");
        break;
      }

      INFO("sink %d is %s\n", sink_info.id, sink_info.name);

      //check vout id with VOUT sink id
      if ((sink_info.sink_type == get_vout_sink_type(m_type)) &&
          (sink_info.source_id == get_vout_source_id(m_id)))
        sink_id = sink_info.id;
    }
    ret = sink_id;

    INFO("%s: %d %d, return %d\n", __func__, m_id, get_vout_sink_type(m_type),
         sink_id);
  } while (0);

  return ret;
}

AM_RESULT AMVoutPort::select_dev()
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (m_iav < 0) {
      result = AM_RESULT_ERR_INVALID;
      ERROR("Invalid IAV fd");
      break;
    }
    //TODO:  ignore the VOUT(LCD) init
    if (m_id == AM_VOUT_A) {
      break;
    }

    if (ioctl(m_iav, IAV_IOC_VOUT_SELECT_DEV, m_sink_id) < 0) {
      PERROR("IAV_IOC_VOUT_SELECT_DEV");
      switch (m_type) {
        case AM_VOUT_TYPE_LCD:
        case AM_VOUT_TYPE_DIGITAL:
          ERROR("AMVoutPort:  Digital link not ready \n");
          break;
        case AM_VOUT_TYPE_HDMI:
          ERROR("AMVoutPort: HDMI link not ready\n ");
          break;

        case AM_VOUT_TYPE_CVBS:
          ERROR("AMVoutPort: CVBS link not ready\n");
          break;
        default:
          break;
      }
      result = AM_RESULT_ERR_DSP;
      break;
    }
  } while (0);

  return result;
}

AM_RESULT AMVoutPort::config_sink(AMVoutParam *param)
{
  AM_RESULT result = AM_RESULT_OK;
  amba_video_sink_mode sink_cfg;
  int video_mode, video_fps;
  do {
    //configure VOUT
    memset(&sink_cfg, 0, sizeof(sink_cfg));
    video_mode = get_amba_vout_mode(m_mode);
    video_fps = get_amba_vout_fps(m_mode);
    if (result != AM_RESULT_OK) {
      ERROR("AMHDMIVoutPort: get vout mode fps error\n");
      break;
    }
    INFO("AMHDMIVoutPort: vout mode %d, video fps %d \n",
         video_mode,
         video_fps);
    sink_cfg.mode = video_mode;
    sink_cfg.frame_rate = video_fps;
    sink_cfg.ratio = AMBA_VIDEO_RATIO_AUTO;
    sink_cfg.bits = AMBA_VIDEO_BITS_AUTO;
    sink_cfg.type = get_vout_sink_type(m_type);
    if (m_mode == AM_VOUT_MODE_480I ||
        m_mode == AM_VOUT_MODE_576I||
        m_mode == AM_VOUT_MODE_1080I60 ||
        m_mode == AM_VOUT_MODE_1080I50) {
      sink_cfg.format = AMBA_VIDEO_FORMAT_INTERLACE;
    } else {
      sink_cfg.format = AMBA_VIDEO_FORMAT_PROGRESSIVE;
    }

    sink_cfg.sink_type = AMBA_VOUT_SINK_TYPE_AUTO;
    sink_cfg.bg_color.y = 0x10;
    sink_cfg.bg_color.cb = 0x80;
    sink_cfg.bg_color.cr = 0x80;
    sink_cfg.lcd_cfg.mode = AMBA_VOUT_LCD_MODE_DISABLE;
    sink_cfg.id = m_sink_id;

    sink_cfg.csc_en = 1;
    sink_cfg.hdmi_color_space = AMBA_VOUT_HDMI_CS_AUTO;

    sink_cfg.hdmi_3d_structure = DDD_RESERVED; //TODO: hardcode
    sink_cfg.hdmi_overscan = AMBA_VOUT_HDMI_OVERSCAN_AUTO; //TODO: hardcode
    sink_cfg.video_en = 1; //TODO: enable it

    //flip config:
    switch (param->flip) {
      case AM_VIDEO_FLIP_NONE:
        sink_cfg.video_flip = AMBA_VOUT_FLIP_NORMAL;
        break;
      case AM_VIDEO_FLIP_VERTICAL:
        sink_cfg.video_flip = AMBA_VOUT_FLIP_VERTICAL;
        break;
      case AM_VIDEO_FLIP_HORIZONTAL:
        sink_cfg.video_flip = AMBA_VOUT_FLIP_HORIZONTAL;
        break;
      case AM_VIDEO_FLIP_VH_BOTH:
        sink_cfg.video_flip = AMBA_VOUT_FLIP_HV;
        break;
      default:
        sink_cfg.video_flip = AMBA_VOUT_FLIP_NORMAL;
        break;
    }

    switch (param->rotate) {
      case AM_VIDEO_ROTATE_NONE:
        sink_cfg.video_rotate = AMBA_VOUT_ROTATE_NORMAL;
        break;
      case AM_VIDEO_ROTATE_90:
        sink_cfg.video_rotate = AMBA_VOUT_ROTATE_90;
        break;
      default:
        sink_cfg.video_rotate = AMBA_VOUT_ROTATE_NORMAL;
        break;
    }

    //fill video size config to VOUT SINK
    AMResolution size;
    amba_vout_video_size video_size;

    size = vout_mode_to_resolution(m_mode);
    video_size.specified = 1;
    video_size.vout_width = size.width;
    video_size.vout_height = size.height;
    video_size.video_width = size.width;
    video_size.video_height = size.height;
    sink_cfg.video_size = video_size;

    //sink_cfg.video_offset = video_offset;
    //sink_cfg.fb_id = fb_id;
    //sink_cfg.osd_flip = AMBA_VOUT_FLIP_NORMAL;
    //sink_cfg.osd_rotate = AMBA_VOUT_ROTATE_NORMAL;
    //sink_cfg.osd_rescale = osd_rescale;
    //sink_cfg.osd_offset = osd_offset;
    //sink_cfg.display_input = display_input;
    sink_cfg.direct_to_dsp = 0;
    if (ioctl(m_iav, IAV_IOC_VOUT_CONFIGURE_SINK, &sink_cfg) < 0) {
      PERROR("IAV_IOC_VOUT_CONFIGURE_SINK");
      result = AM_RESULT_ERR_DSP;
      break;
    }

    INFO("AMHDMIVoutPort: init_vout%d done\n", m_id);
  } while (0);

  return result;
}

AM_RESULT AMVoutPort::init(int iav_hd, AMVoutParam *param)
{
  AM_RESULT result = AM_RESULT_OK;

  do {
    if ((m_iav = iav_hd) < 0) {
      result = AM_RESULT_ERR_INVALID;
      ERROR("Invalid IAV fd");
      break;
    }

    if ((m_sink_id = vout_get_sink_id()) < 0) {
      ERROR("AMVoutPort: get vout sink id failed \n");
      result = AM_RESULT_ERR_DSP;
      break;
    }

    if ((result = select_dev()) != AM_RESULT_OK) {
      break;
    }

    if ((result = config_sink(param)) != AM_RESULT_OK) {
      break;
    }
  } while (0);

  return result;
}

AM_RESULT AMVoutPort::halt()
{
  return AM_RESULT_OK;
}

AM_RESULT AMVoutPort::set_mode(AM_VOUT_MODE mode)
{
  PRINTF("AMVoutPort: set_mode \n");
  return AM_RESULT_OK;
}

AM_RESULT AMHDMIVoutPort::init(int iav_hd, AMVoutParam *param)
{
  return AMVoutPort::init(iav_hd, param);
}

AM_RESULT AMHDMIVoutPort::halt()
{

  return AM_RESULT_OK;
}

AM_RESULT AMCVBSVoutPort::init(int iav_hd, AMVoutParam *param)
{
  return AMVoutPort::init(iav_hd, param);
}

AM_RESULT AMCVBSVoutPort::halt()
{
  return AM_RESULT_OK;
}

AM_RESULT AMLCDVoutPort::init(int iav_hd, AMVoutParam *param)
{
  return AMVoutPort::init(iav_hd, param);
}

AM_RESULT AMLCDVoutPort::halt()
{
  return AM_RESULT_OK;
}

AM_RESULT AMDigitalVoutPort::init(int iav_hd, AMVoutParam *param)
{
  return AMVoutPort::init(iav_hd, param);
}

AM_RESULT AMDigitalVoutPort::halt()
{
  return AM_RESULT_OK;
}

AMVoutPort * AMVout::create_vout_port()
{
  AMVoutPort * voutport;
  switch(m_param.type)
  {
    case AM_VOUT_TYPE_LCD:
      voutport = new AMLCDVoutPort(m_id, m_param.mode);
      break;
    case AM_VOUT_TYPE_HDMI:
      voutport = new AMHDMIVoutPort(m_id, m_param.mode);
      break;
    case AM_VOUT_TYPE_CVBS:
      voutport = new AMCVBSVoutPort(m_id, m_param.mode);
      break;
    case AM_VOUT_TYPE_DIGITAL:
      voutport = new AMDigitalVoutPort(m_id, m_param.mode);
      break;
    case AM_VOUT_TYPE_NONE:
    default:
      //init dummy vout
      voutport = new AMVoutPort(m_id, m_param.mode);
      break;
  }
  if (!voutport) {
    ERROR("AMVout: create vout port type %d failed \n", m_param.type);
  }
  return voutport;
}

AM_RESULT AMVout::init(int iav_hd, int vout_id, AMVoutParam *vout_param)
{
  AM_RESULT result = AM_RESULT_OK;
  AM_VOUT_ID id;
  do {
    m_iav = iav_hd;
    //check vout_id
    id = AM_VOUT_ID(vout_id);
    if ((id != AM_VOUT_A) && (id != AM_VOUT_B)) {
      ERROR("AMVout: init: vout id error %d \n", vout_id);
      result = AM_RESULT_ERR_INVALID;
      break;
    }

    if (!vout_param) {
      result = AM_RESULT_ERR_INVALID;
      break;
    }

    //save vout id for this vout class
    m_id = AM_VOUT_ID(vout_id);
    m_param = *vout_param;
    m_port = create_vout_port();
    if (!m_port) {
      result = AM_RESULT_ERR_MEM;
      break;
    }
    //now init port
    if ((result = m_port->init(m_iav, &m_param)) != AM_RESULT_OK) {
      break;
    }
  } while (0);
  return result;
}

AM_RESULT AMVout::shutdown()
{

  return AM_RESULT_OK;
}

AM_RESULT AMVout::reset()
{
  return AM_RESULT_OK;
}

//TODO: fill AMVout::set_mode and set_fps

AM_RESULT AMVout::set_mode()
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    //TODO:  set VOUT video size by hardcode
#if 0
    //enable vout
    iav_vout_enable_video_t enable_video;
    enable_video.vout_id = 1;
    enable_video.video_en = 1;
    if (ioctl(m_iav, IAV_IOC_VOUT_ENABLE_VIDEO, &enable_video) < 0) {
      PERROR("IAV_IOC_VOUT_ENABLE_VIDEO of vout0");
      ret = false;
      break;
    }

    //vout size
    iav_vout_change_video_size_t cvs;
    cvs.vout_id = get_vout_source_id(AM_VOUT_B);//get vout iav id
    cvs.width = 720;//hard code for 480p
    cvs.height = 480;//hard code for 480p
    if (ioctl(m_iav, IAV_IOC_VOUT_CHANGE_VIDEO_SIZE, &cvs) < 0) {
      PERROR("IAV_IOC_VOUT_CHANGE_VIDEO_SIZE of vout0");
      ret = false;
      break;
    }

#endif

    //setup VOUT port for this mode
    result = m_port->set_mode(m_param.mode);
    if (result != AM_RESULT_OK) {
      ERROR("AMVout: set_mode error \n");
      break;
    }
  } while (0);
  return result;
}

AM_RESULT AMVout::get_mode(AM_VOUT_MODE *mode)
{

  return AM_RESULT_OK;
}

AM_RESULT AMVout::set_fps()
{
  PRINTF("AMVout: set_fps dummy\n");
  return AM_RESULT_OK;
}

AM_RESULT AMVout::get_fps(AM_VIDEO_FPS *fps)
{
  return AM_RESULT_OK;
}

AM_RESULT AMVout::get_state(AM_VOUT_STATE *state)
{

  return AM_RESULT_OK;
}

AM_RESULT AMVout::flip()
{

  return AM_RESULT_OK;
}

AM_RESULT AMVout::color_conversion(AM_VOUT_COLOR_CONVERSION conversion)
{
  return AM_RESULT_OK;
}

AM_RESULT AMVout::change(AMVoutParam *vout_param)
{
  return AM_RESULT_OK;
}

//update mode, fps and other properties from config
AM_RESULT AMVout::update()
{
  AM_RESULT result = AM_RESULT_OK;

  do {
    if (m_id == AM_VOUT_A) {
      INFO("AMVin: second VIN is ignored now\n");
      break;
    }
    //update DSP mode and FPS

    result = set_mode();
    if (result != AM_RESULT_OK) {
      break;
    }

    result = set_fps();
    if (result != AM_RESULT_OK) {
      break;
    }
  } while (0);
  return result;
}

AM_RESULT AMVout::get_vout_port_config(AM_VOUT_TYPE *type, AM_VOUT_MODE *mode)

{
  AM_RESULT result = AM_RESULT_OK;
  do {
    //NOW hardcode it
    if (!type || !mode) {
      result = AM_RESULT_ERR_INVALID;
      break;
    }

    *type = AM_VOUT_TYPE_HDMI;
    *mode = AM_VOUT_MODE_480P;
  } while (0);
  return result;
}

AM_RESULT AMVout::video_fps_to_q9_format(AM_VIDEO_FPS *fps, AMVideoFpsFormatQ9 *q9)
{
  *q9 = get_amba_video_fps(*fps);
  return AM_RESULT_OK;
}

AM_RESULT AMVout::q9_format_to_videofps(AMVideoFpsFormatQ9 *q9, AM_VIDEO_FPS *fps)
{

  return AM_RESULT_OK;
}
