/**
 * am_vout.cpp
 *
 *  History:
 *    Jul 23, 2015 - [Shupeng Ren] created file
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

#include "am_log.h"
#include "am_encode_config.h"
#include "am_vout.h"

AMVoutPort::AMVoutPort(AM_VOUT_ID id)  :
  m_sink_id(-1),
  m_id(id),
  m_platform(nullptr)
{
}

AMVoutPort::~AMVoutPort()
{
  m_platform = nullptr;
}

AM_RESULT AMVoutPort::init(const AMVoutParam &param)
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    //dummy vout directly return
    if (AM_VOUT_ID_INVALID == m_id) {
      DEBUG("dummy vout!!!");
      break;
    }

    if (!(m_platform = AMIPlatform::get_instance())) {
      result = AM_RESULT_ERR_MEM;
      ERROR("Failed to create AMIPlatform");
      break;
    }

    if ((m_sink_id = m_platform->vout_sink_id_get(m_id, param.type.second))
        < 0) {
      ERROR("AMVoutPort: get vout sink id failed \n");
      result = AM_RESULT_ERR_DSP;
      break;
    }

    if ((result = m_platform->vout_active_sink_set(m_sink_id))
        != AM_RESULT_OK) {
      ERROR("AMVoutPort: set sink%d to active failed \n",m_sink_id);
      break;
    }

    AMPlatformVoutSinkConfig config;
    config.sink_id = m_sink_id;
    config.sink_type = param.type.second;
    config.video_type = param.video_type.second;
    config.mode = param.mode.second;
    config.fps = param.fps.second;
    config.flip = param.flip.second;
    config.rotate = param.rotate.second;
    if ((result = m_platform->vout_sink_config_set(config))
        != AM_RESULT_OK) {
      break;
    }
  } while (0);

  return result;
}

AM_RESULT AMVoutPort::halt()
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    //dummy vout directly return
    if (AM_VOUT_ID_INVALID == m_id) {
      DEBUG("dummy vout!!!");
      break;
    }
    result = m_platform->vout_halt(m_id);
  } while(0);

  return result;
}

AMHDMIVoutPort::AMHDMIVoutPort(AM_VOUT_ID id) :
    AMVoutPort(id)
{
}

AMHDMIVoutPort::~AMHDMIVoutPort()
{
}

AM_RESULT AMHDMIVoutPort::init(const AMVoutParam &param)
{
  return AMVoutPort::init(param);
}

AM_RESULT AMHDMIVoutPort::halt()
{
  return AMVoutPort::halt();
}

AMCVBSVoutPort::AMCVBSVoutPort(AM_VOUT_ID id) :
    AMVoutPort(id)
{
}

AMCVBSVoutPort::~AMCVBSVoutPort()
{
}

AM_RESULT AMCVBSVoutPort::init(const AMVoutParam &param)
{
  return AMVoutPort::init(param);
}

AM_RESULT AMCVBSVoutPort::halt()
{
  return AMVoutPort::halt();
}

AMLCDVoutPort::AMLCDVoutPort(AM_VOUT_ID id) :
        AMVoutPort(id)
{
}

AMLCDVoutPort::~AMLCDVoutPort()
{
}

AM_RESULT AMLCDVoutPort::init(const AMVoutParam &param)
{
  return AMVoutPort::init(param);
}

AM_RESULT AMLCDVoutPort::halt()
{
  return AMVoutPort::halt();
}

AM_RESULT AMLCDVoutPort::lcd_reset()
{
  //TODO
  return AM_RESULT_ERR_PERM;
}

AM_RESULT AMLCDVoutPort::lcd_power_on()
{
  //TODO
  return AM_RESULT_ERR_PERM;
}

AM_RESULT AMLCDVoutPort::lcd_backlight_on()
{
  //TODO
  return AM_RESULT_ERR_PERM;
}

AM_RESULT AMLCDVoutPort::lcd_pwm_set_brightness(uint32_t brightness)
{
  //TODO
  return AM_RESULT_ERR_PERM;
}

AM_RESULT AMLCDVoutPort::lcd_pwm_get_max_brightness(int32_t &value)
{
  //TODO
  return AM_RESULT_ERR_PERM;
}

AM_RESULT AMLCDVoutPort::lcd_pwm_get_current_brightness(int32_t &value)
{
  //TODO
  return AM_RESULT_ERR_PERM;
}

const char* AMLCDVoutPort::lcd_spi_dev_node()
{
  //TODO
  return nullptr;
}

AMYPbPrVoutPort::AMYPbPrVoutPort(AM_VOUT_ID id) :
          AMVoutPort(id)
{
}

AMYPbPrVoutPort::~AMYPbPrVoutPort()
{
}

AM_RESULT AMYPbPrVoutPort::init(const AMVoutParam &param)
{
  return AMVoutPort::init(param);
}

AM_RESULT AMYPbPrVoutPort::halt()
{
  return AMVoutPort::halt();
}

AMVout *AMVout::m_instance = nullptr;
AMVout::AMVout():
    m_config(nullptr)
{
}

AMVout::~AMVout()
{
  stop();
  m_config = nullptr;
  m_param.clear();
}

AMVout* AMVout::create()
{
  if (!m_instance) {
    m_instance = new AMVout();
    if (m_instance && (m_instance->init() != AM_RESULT_OK)) {
      delete m_instance;
      m_instance = nullptr;
    }
  }

  return m_instance;
}

void AMVout::destroy()
{
  DEBUG("AMVout destroy!");
  delete m_instance;
  m_instance = nullptr;
}

AM_RESULT AMVout::init()
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (!(m_config = AMVoutConfig::get_instance())) {
      ERROR("Failed to create VOUT config!");
      result = AM_RESULT_ERR_MEM;
      break;
    }

    if ((result = m_config->get_config(m_param)) != AM_RESULT_OK) {
      ERROR("Failed to get VOUT config!");
      break;
    }

  } while (0);

  return result;
}

AM_RESULT AMVout::start()
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    for (auto &m : m_param) {
      AM_VOUT_ID id = m.first;
      const AMVoutParam &param = m.second;

      AMVoutPort *port = nullptr;
      if (!(port = create_vout_port(id, param.type.second))) {
        result = AM_RESULT_ERR_MEM;
        ERROR("create vout port failed!!!");
        break;
      }

      if ((result = port->init(param)) != AM_RESULT_OK) {
        break;
      }

      INFO("vout start!!!!");
      m_port[id] = port;
    }
  } while(0);

  return result;
}

AM_RESULT AMVout::stop()
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    for (auto &m: m_port) {
        delete m.second;
        m.second = nullptr;
    }
    m_port.clear();
  } while(0);

  return result;
}

AM_RESULT AMVout::shutdown(AM_VOUT_ID id)
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    map<AM_VOUT_ID, AMVoutPort*>::iterator it = m_port.find(id);
    if (it == m_port.end()) {
      WARN("vout%d is not created!");
      result = AM_RESULT_ERR_PERM;
      break;
    }
    result = m_port[id]->halt();
  } while(0);

  return result;
}

AM_RESULT AMVout::load_config()
{
  AM_RESULT result = AM_RESULT_OK;

  do {
    if (!m_config) {
      result = AM_RESULT_ERR_INVALID;
      ERROR("AMVoutConfigPtr is null!");
      break;
    }
    if ((result = m_config->get_config(m_param)) != AM_RESULT_OK) {
      ERROR("Failed to get vout config!");
      break;
    }
  } while (0);

  return result;
}

AM_RESULT AMVout::get_param(AMVoutParamMap &param)
{
  param = m_param;
  return AM_RESULT_OK;
}

AM_RESULT AMVout::set_param(const AMVoutParamMap &param)
{
  m_param = param;
  return AM_RESULT_OK;
}

AMVoutPort * AMVout::create_vout_port(AM_VOUT_ID id, AM_VOUT_TYPE type)
{
  AMVoutPort * port = nullptr;
  switch(type)
  {
    case AM_VOUT_TYPE_LCD:
      port = new AMLCDVoutPort(id);
      break;
    case AM_VOUT_TYPE_HDMI:
      port = new AMHDMIVoutPort(id);
      break;
    case AM_VOUT_TYPE_CVBS:
      port = new AMCVBSVoutPort(id);
      break;
    case AM_VOUT_TYPE_YPBPR:
      port = new AMYPbPrVoutPort(id);
      break;
    case AM_VOUT_TYPE_NONE:
    default:
      //create a dummy vout
      port = new AMVoutPort(AM_VOUT_ID_INVALID);
      break;
  }
  if (!port) {
    ERROR("AMVout: create vout port type %d failed \n",type);
  }

  return port;
}
