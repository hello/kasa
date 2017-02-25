/*******************************************************************************
 * am_encode_device.cpp
 *
 * History:
 *   May 7, 2015 - [ypchang] created file
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

#include "am_base_include.h"
#include "am_log.h"
#include "am_define.h"

#include "am_video_types.h"
#include "am_encode_types.h"
#include "am_video_utility.h"
#include "am_encode_config.h"
#include "am_encode_device.h"

#include "am_vin.h"
#include "am_vout.h"
#include "am_mutex.h"
#include "am_encode_buffer.h"
#include "am_encode_stream.h"

#include "am_plugin.h"
#include "am_encode_plugin_if.h"
#include "am_encode_warp_if.h"
#include "am_low_bitrate_control_if.h"
#include "am_smartrc_if.h"
#include "am_encode_eis_if.h"
#include "am_encode_overlay_if.h"
#include "am_dptz_if.h"
#include "am_motion_detect_if.h"

AMVideoPlugin::AMVideoPlugin() :
  so(nullptr),
  plugin(nullptr)
{}

AMVideoPlugin::AMVideoPlugin(const AMVideoPlugin &vplugin) :
  so(vplugin.so),
  plugin(vplugin.plugin)
{}

AMVideoPlugin::~AMVideoPlugin()
{
  AM_DESTROY(plugin);
  AM_DESTROY(so);
}

AMEncodeDevice* AMEncodeDevice::create()
{
  AMEncodeDevice *result = new AMEncodeDevice();
  if (result && (AM_RESULT_OK != result->init())) {
    delete result;
    result = nullptr;
  }
  return result;
}

void AMEncodeDevice::destroy()
{
  delete this;
}

AM_RESULT AMEncodeDevice::init()
{
  AM_RESULT result = AM_RESULT_OK;

  do {
    if (!(m_platform = AMIPlatform::get_instance())) {
      result = AM_RESULT_ERR_MEM;
      ERROR("Failed to create AMIPlatform!");
      break;
    }

    if ((result = vin_create()) != AM_RESULT_OK) {
      ERROR("Failed to create VIN!");
      break;
    }

    if (!(m_vout = AMVout::create())) {
      ERROR("Failed to create VOUT!");
      result = AM_RESULT_ERR_MEM;
      break;
    }

    if (!(m_iav_lock = AMMutex::create())) {
      ERROR("Failed to create mutex for IAV!");
      result = AM_RESULT_ERR_MEM;
      break;
    }

    if (!(m_buffer = AMEncodeBuffer::create())) {
      result = AM_RESULT_ERR_MEM;
      ERROR("Failed to create AMEncodeBuffer!");
      break;
    }

    if (!(m_stream = AMEncodeStream::create())) {
      result = AM_RESULT_ERR_MEM;
      ERROR("Failed to create AMEncodeStream!");
      break;
    }

    if ((result = load_config_all()) != AM_RESULT_OK) {
      break;
    }

  } while (0);

  return result;
}

AMEncodeDevice::AMEncodeDevice() :
  m_buffer(nullptr),
  m_stream(nullptr),
  m_vout(nullptr),
  m_iav_lock(nullptr),
  m_platform(nullptr),
  m_state(AM_IAV_STATE_INIT)
{
  m_vin.clear();
  m_plugin_map_encode.clear();
  m_plugin_map_preview.clear();
  DEBUG("AMEncodeDevice is created!");
}

AMEncodeDevice::~AMEncodeDevice()
{
  for (auto &iter : m_plugin_map_preview) {
    delete iter.second;
  }
  m_plugin_map_preview.clear();
  for (auto &iter : m_plugin_map_encode) {
    delete iter.second;
  }
  m_plugin_map_encode.clear();
  vin_delete();
  AM_DESTROY(m_vout);
  AM_DESTROY(m_iav_lock);
  AM_DESTROY(m_buffer);
  AM_DESTROY(m_stream);
  m_platform = nullptr;
  DEBUG("AMEncodeDevice is destroyed!");
}

AM_RESULT AMEncodeDevice::vin_create()
{
  AM_RESULT result = AM_RESULT_ERR_INVALID;

  AMVin *vin;
  AMVinConfigPtr vin_config = nullptr;
  AMVinParamMap vin_param;
  if (!(vin_config = AMVinConfig::get_instance())) {
    ERROR("Failed to create VIN config!");
    return result;
  }

  if ((result = vin_config->get_config(vin_param)) != AM_RESULT_OK) {
    ERROR("Failed to get VIN config!");
    return result;
  }

  for (auto &m : vin_param) {
    vin = nullptr;
    switch (m.second.type.second) {
      case AM_SENSOR_TYPE_RGB:
        vin = AMRGBSensorVin::create(m.first);
        break;
      case AM_SENSOR_TYPE_YUV:
        vin = AMYUVSensorVin::create(m.first);
        break;
      case AM_SENSOR_TYPE_YUV_GENERIC:
        vin = AMYUVGenericVin::create(m.first);
        break;
      default:
        break;
    }
    if (vin) {
      m_vin.push_back(vin);
    }
  }

  if (m_vin.size()) {
    result = AM_RESULT_OK;
  }

  return result;
}

AM_RESULT AMEncodeDevice::vin_start()
{
  AM_RESULT result = AM_RESULT_OK;
  for (auto &v : m_vin) {
    if ((result = v->start()) != AM_RESULT_OK) {
      break;
    }
  }
  return result;
}

AM_RESULT AMEncodeDevice::vin_stop()
{
  AM_RESULT result = AM_RESULT_OK;
  for (auto &v : m_vin) {
    if (v->is_close_vin_needed() &&
        (result = v->stop()) != AM_RESULT_OK) {
      break;
    }
  }
  return result;
}

AM_RESULT AMEncodeDevice::vin_delete()
{
  AM_RESULT result = AM_RESULT_OK;

  for (auto &v : m_vin) {
    AM_DESTROY(v);
  }

  return result;
}

void AMEncodeDevice::load_all_plugins()
{
  do {
    if (!m_platform) {
      ERROR("AMPlatform object is not created!");
      break;
    }

    AMVideoPlugin *vplugin = nullptr;
    /* WARP Plugin */
    AMVideoPluginMap::iterator warp_iter =
        m_plugin_map_encode.find(VIDEO_PLUGIN_WARP);
    AMPluginData warp_data(m_vin[AM_VIN_0], m_stream);
    std::string warp_so(VIDEO_PLUGIN_DIR);
    warp_so.append("/").append(VIDEO_PLUGIN_WARP_SO);

    if (warp_iter != m_plugin_map_encode.end()) {
      NOTICE("Encode Plugin \"%s\" is already loaded! Erase it first!",
             warp_iter->second->plugin->name().c_str());
      delete warp_iter->second;
      m_plugin_map_encode.erase(warp_iter);
    }

    AMVideoPluginMap::iterator eis_iter =
        m_plugin_map_encode.find(VIDEO_PLUGIN_EIS);
    AMPluginData eis_data(m_vin[AM_VIN_0], m_stream);
    std::string eis_so(VIDEO_PLUGIN_DIR);
    eis_so.append("/").append(VIDEO_PLUGIN_EIS_SO);

    if (eis_iter != m_plugin_map_encode.end()) {
      NOTICE("Encode Plugin \"%s\" is already loaded! Erase it first!",
             eis_iter->second->plugin->name().c_str());
      delete eis_iter->second;
      m_plugin_map_encode.erase(eis_iter);
    }
    if (m_platform->has_warp()) {
      switch(m_platform->get_dewarp_func()) {
        case AM_DEWARP_LDC:
        case AM_DEWARP_FULL: {
          vplugin = load_video_plugin(warp_so.c_str(), &warp_data);
          if (vplugin) {
            m_plugin_map_encode[VIDEO_PLUGIN_WARP] = vplugin;
            NOTICE("Encode Plugin \"%s\" is loaded!",
                   vplugin->plugin->name().c_str());
          }
        }break;
        case AM_DEWARP_EIS: {
          vplugin = load_video_plugin(eis_so.c_str(), &eis_data);
          if (vplugin) {
            m_plugin_map_encode[VIDEO_PLUGIN_EIS] = vplugin;
            NOTICE("Encode Plugin \"%s\" is loaded!",
                   vplugin->plugin->name().c_str());
          }

        }break;
        default : {
          ERROR("Invalid dewarp function %d", m_platform->get_dewarp_func());
        }break;
      }

    } else {
      NOTICE("WARP is not enabled!");
    }

    /* DPTZ Plugin */
    AMVideoPluginMap::iterator dptz_iter =
        m_plugin_map_encode.find(VIDEO_PLUGIN_DPTZ);
    std::string dptz_so(VIDEO_PLUGIN_DIR);
    dptz_so.append("/").append(VIDEO_PLUGIN_DPTZ_SO);

    if (dptz_iter != m_plugin_map_encode.end()) {
      NOTICE("Encode Plugin \"%s\" is already loaded! Erase it first!",
             dptz_iter->second->plugin->name().c_str());
      delete dptz_iter->second;
      m_plugin_map_encode.erase(dptz_iter);
    }
    switch(m_platform->get_dptz()) {
      case AM_DPTZ_ENABLE: {
        vplugin = load_video_plugin(dptz_so.c_str(), m_vin[AM_VIN_0]);
        if (vplugin) {
          m_plugin_map_encode[VIDEO_PLUGIN_DPTZ] = vplugin;
          NOTICE("Encode Plugin \"%s\" is loaded!",
                 vplugin->plugin->name().c_str());
        }
      } break;
      case AM_DPTZ_DISABLE: {
        NOTICE("Disable DPTZ control!");
      } break;
      default: {
        WARN("Unknown set of DPTZ");
        break;
      }
    }

    /* Bitrate Control Plugin */
    AMVideoPluginMap::iterator lbr_iter =
        m_plugin_map_encode.find(VIDEO_PLUGIN_LBR);
    std::string lbr_so(VIDEO_PLUGIN_DIR);
    lbr_so.append("/").append(VIDEO_PLUGIN_LBR_SO);
    if (lbr_iter != m_plugin_map_encode.end()) {
      NOTICE("Encode Plugin \"%s\" is already loaded! Erase it first!",
             lbr_iter->second->plugin->name().c_str());
      delete lbr_iter->second;
      m_plugin_map_encode.erase(lbr_iter);
    }

    AMVideoPluginMap::iterator smartrc_iter =
        m_plugin_map_encode.find(VIDEO_PLUGIN_SMARTRC);
    std::string smartrc_so(VIDEO_PLUGIN_DIR);
    smartrc_so.append("/").append(VIDEO_PLUGIN_SMARTRC_SO);
    if (smartrc_iter != m_plugin_map_encode.end()) {
      NOTICE("Encode Plugin \"%s\" is already loaded! Erase it first!",
             smartrc_iter->second->plugin->name().c_str());
      delete smartrc_iter->second;
      m_plugin_map_encode.erase(smartrc_iter);
    }
    switch(m_platform->get_bitrate_ctrl_method()) {
      case AM_BITRATE_CTRL_LBR: { /* LBR */
        AMPluginData lbr_data(m_vin[AM_VIN_0], m_stream);
        vplugin = load_video_plugin(lbr_so.c_str(), &lbr_data);
        if (vplugin) {
          m_plugin_map_encode[VIDEO_PLUGIN_LBR] = vplugin;
          NOTICE("Encode Plugin \"%s\" is loaded!",
                 vplugin->plugin->name().c_str());
        }
      }break;
      case AM_BITRATE_CTRL_SMARTRC: { /* SmartRC */
        AMPluginData smartrc_data(m_vin[AM_VIN_0], m_stream);
        vplugin = load_video_plugin(smartrc_so.c_str(), &smartrc_data);
        if (vplugin) {
          m_plugin_map_encode[VIDEO_PLUGIN_SMARTRC] = vplugin;
          NOTICE("Encode Plugin \"%s\" is loaded!",
                 vplugin->plugin->name().c_str());
        }
      }break;
      case AM_BITRATE_CTRL_NONE: {
        NOTICE("No special bitrate control algorithm is enabled!");
      }break;
      default: {
        WARN("Unknown bitrate control algorithm!");
      }break;
    }

    /* Overlay Plugin */
    AMVideoPluginMap::iterator ol_iter =
        m_plugin_map_encode.find(VIDEO_PLUGIN_OVERLAY);
    std::string ol_so(VIDEO_PLUGIN_DIR);
    ol_so.append("/").append(VIDEO_PLUGIN_OVERLAY_SO);

    if (ol_iter != m_plugin_map_encode.end()) {
      NOTICE("Encode Plugin \"%s\" is already loaded! Erase it first!",
             ol_iter->second->plugin->name().c_str());
      delete ol_iter->second;
      m_plugin_map_encode.erase(ol_iter);
    }
    switch(m_platform->get_overlay()) {
      case AM_OVERLAY_PLUGIN_ENABLE: {
        vplugin = load_video_plugin(ol_so.c_str(), m_stream);
        if (vplugin) {
          m_plugin_map_encode[VIDEO_PLUGIN_OVERLAY] = vplugin;
          NOTICE("Encode Plugin \"%s\" is loaded!",
                 vplugin->plugin->name().c_str());
        }
      } break;
      case AM_OVERLAY_PLUGIN_DISABLE: {
        NOTICE("Disable Overlay plugin!");
      } break;
      default: {
        WARN("Unknown set of Overlay");
        break;
      }
    }

    /* Video Motion Detect Plugin */
    AMVideoPluginMap::iterator md_iter =
        m_plugin_map_encode.find(VIDEO_PLUGIN_MD);
    AMVideoPlugin *mdplugin = nullptr;
    std::string md_so(VIDEO_PLUGIN_DIR);
    md_so.append("/").append(VIDEO_PLUGIN_MD_SO);

    if (md_iter != m_plugin_map_encode.end()) {
      NOTICE("Encode Plugin \"%s\" is already loaded! Re-loading it!",
             md_iter->second->plugin->name().c_str());
      delete md_iter->second;
      m_plugin_map_encode.erase(md_iter);
    }
    switch(m_platform->get_video_md()) {
      case AM_MD_PLUGIN_ENABLE: {
        mdplugin = load_video_plugin(md_so.c_str(), m_stream);
        if (mdplugin) {
          m_plugin_map_encode[VIDEO_PLUGIN_MD] = mdplugin;
          NOTICE("Encode Plugin \"%s\" is loaded!",
                 mdplugin->plugin->name().c_str());
        }
      } break;
      case AM_MD_PLUGIN_DISABLE: {
        NOTICE("Disable video_md plugin!");
      } break;
      default: {
        WARN("Unknown set of video_md plugin");
        break;
      }
    }

  }while(0);

}

AMVideoPlugin* AMEncodeDevice::load_video_plugin(const char *name,
                                                 void *data)
{
  AMVideoPlugin *vplugin = nullptr;
  do {
    if (!name) {
      ERROR("Invalid plugin (null)!");
      break;
    }

    vplugin = new AMVideoPlugin();
    if (!vplugin) {
      ERROR("Failed to allocate memory for AMVideoPlugin object!");
      break;
    }

    vplugin->so = AMPlugin::create(name);
    if (!vplugin->so) {
      ERROR("Failed to load plugin: %s", name);
    } else {
      CreateEncodePlugin create_encode_plugin =
          (CreateEncodePlugin)vplugin->so->get_symbol(CREATE_ENCODE_PLUGIN);
      if (!create_encode_plugin) {
        ERROR("Invalid warp plugin: %s", name);
        break;
      }
      vplugin->plugin = create_encode_plugin(data);
      if (!vplugin->plugin) {
        ERROR("Failed to create object of video plugin: %s!", name);
        break;
      }
    }
  }while(0);

  if (vplugin && (!vplugin->so || !vplugin->plugin)) {
    delete vplugin;
    vplugin = nullptr;
  }

  return vplugin;
}

AM_RESULT AMEncodeDevice::start()
{
  return encode();
}

AM_RESULT AMEncodeDevice::start_stream(AM_STREAM_ID id)
{
  return encode(id);
}

AM_RESULT AMEncodeDevice::stop()
{
  return idle();
}

AM_RESULT AMEncodeDevice::stop_stream(AM_STREAM_ID id)
{
  return preview(id);
}

AM_RESULT AMEncodeDevice::goto_low_power_mode()
{
  return change_iav_state(AM_IAV_STATE_PREVIEW_LOW);
}

AM_RESULT AMEncodeDevice::idle()
{
  return change_iav_state(AM_IAV_STATE_IDLE);
}

AM_RESULT AMEncodeDevice::preview(AM_STREAM_ID id, AM_POWER_MODE mode)
{
  AM_RESULT result = AM_RESULT_OK;

  if (mode == AM_POWER_MODE_LOW) {
    result = change_iav_state(AM_IAV_STATE_PREVIEW_LOW);
  } else {
    result = change_iav_state(AM_IAV_STATE_PREVIEW, id);
  }

  return result;
}

AM_RESULT AMEncodeDevice::encode(AM_STREAM_ID id)
{
  return change_iav_state(AM_IAV_STATE_ENCODING, id);
}

AM_RESULT AMEncodeDevice::decode()
{
  return change_iav_state(AM_IAV_STATE_DECODING);
}

AM_RESULT AMEncodeDevice::get_stream_status(AM_STREAM_ID id,
                                            AM_STREAM_STATE &state)
{
  return m_platform->stream_state_get(id, state);
}

AM_RESULT AMEncodeDevice::get_buffer_state(AM_SOURCE_BUFFER_ID id,
                                          AM_SRCBUF_STATE &state)
{
  return m_buffer->get_buffer_state(id, state);
}

AM_RESULT AMEncodeDevice::get_buffer_format(AMBufferConfigParam &param)
{
  return m_buffer->get_buffer_format(param);
}

AM_RESULT AMEncodeDevice::set_buffer_format(const AMBufferConfigParam &param)
{
  return m_buffer->set_buffer_format(param);
}

AM_RESULT AMEncodeDevice::save_buffer_config()
{
  return m_buffer->save_config();
}

AM_RESULT AMEncodeDevice::set_stream_param(AM_STREAM_ID id,
                                           AMStreamConfigParam &param)
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    AMStreamParamMap stream_param;
    if ((result = m_stream->get_param(stream_param)) != AM_RESULT_OK) {
      ERROR("Failed to get stream param!");
      break;
    }
    AMStreamParamMap::iterator itr = stream_param.find(id);
    if (itr == stream_param.end()) {
      result = AM_RESULT_ERR_INVALID;
      break;
    }
    if (!itr->second.stream_format.second.enable.second) {
      INFO("stream %d is not enable in configure file!\n",id);
      break;
    }

    if (param.stream_format.first) {
      const AMStreamFormatConfig &format_param = param.stream_format.second;
      AMStreamFormatConfig &format = stream_param[id].stream_format.second;
      stream_param[id].stream_format.first = true;
      if (format_param.type.first) {
        format.type.second = format_param.type.second;
        format.type.first = true;
      }
      if (format_param.enc_win.first) {
        if ((format_param.enc_win.second.size.width != -1) &&
            (format_param.enc_win.second.size.height != -1)) {
          format.enc_win.second.size = format_param.enc_win.second.size;
        }
        if ((format_param.enc_win.second.offset.x != -1) &&
            (format_param.enc_win.second.offset.y != -1)) {
          format.enc_win.second.offset = format_param.enc_win.second.offset;
        }
        format.enc_win.first = true;
      }
      if (format_param.source.first) {
        format.source.second = format_param.source.second;
        format.source.first = true;
      }
      if (format_param.flip.first) {
        format.flip.second = format_param.flip.second;
        format.flip.first = true;
      }
      if (format_param.rotate_90_cw.first) {
        format.rotate_90_cw.second = format_param.rotate_90_cw.second;
        format.rotate_90_cw.first = true;
      }
      if (format_param.fps.first) {
        format.fps.second = format_param.fps.second;
        format.fps.first = true;
      }
    }

    if (param.h26x_config.first) {
      const AMStreamH26xConfig &h26x_param = param.h26x_config.second;
      AMStreamH26xConfig &h26x = stream_param[id].h26x_config.second;
      stream_param[id].h26x_config.first = true;
      if (h26x_param.profile_level.first) {
        h26x.profile_level.second = h26x_param.profile_level.second;
        h26x.profile_level.first = true;
      }
      if (h26x_param.bitrate_control.first) {
        h26x.bitrate_control.second = h26x_param.bitrate_control.second;
        h26x.bitrate_control.first = true;
      }
      if (h26x_param.target_bitrate.first) {
        h26x.target_bitrate.second = h26x_param.target_bitrate.second;
        h26x.target_bitrate.first = true;
      }
      if (h26x_param.N.first) {
        h26x.N.second = h26x_param.N.second;
        h26x.N.first = true;
      }
      if (h26x_param.idr_interval.first) {
        h26x.idr_interval.second = h26x_param.idr_interval.second;
        h26x.idr_interval.first = true;
      }
    }

    if (param.mjpeg_config.first) {
      const AMStreamMJPEGConfig &mjpeg_param = param.mjpeg_config.second;
      AMStreamMJPEGConfig &mjpeg = stream_param[id].mjpeg_config.second;
      stream_param[id].mjpeg_config.first = true;
      if (mjpeg_param.quality.first) {
        mjpeg.quality.second = mjpeg_param.quality.second;
        mjpeg.quality.first = true;
      }
    }

    if ((result = m_stream->set_param(stream_param)) != AM_RESULT_OK) {
      ERROR("Failed to set stream type param!");
      break;
    }
  } while (0);

  return result;
}

AM_RESULT AMEncodeDevice::save_stream_config()
{
  return m_stream->save_config();
}

AM_RESULT AMEncodeDevice::halt_vout(AM_VOUT_ID id)
{
  return m_vout->shutdown(id);
}

AM_RESULT AMEncodeDevice::force_idr(AM_STREAM_ID stream_id)
{
  return m_platform->force_idr(stream_id);
}

AM_RESULT AMEncodeDevice::load_config()
{
  AM_RESULT result = AM_RESULT_OK;
  do {

  } while (0);
  return result;
}

AM_RESULT AMEncodeDevice::load_config_all()
{
  AM_RESULT result = AM_RESULT_OK;

  do {
    if ((result = m_platform->load_config()) != AM_RESULT_OK) {
      break;
    }

    for (auto &v : m_vin) {
      if ((result = v->load_config()) != AM_RESULT_OK) {
        ERROR("Failed to load Vin[%d] config!", v->id_get());
        break;
      }
    }
    if (result != AM_RESULT_OK) {
      break;
    }

    if ((result = m_vout->load_config()) != AM_RESULT_OK) {
      break;
    }

    if ((result = m_buffer->load_config()) != AM_RESULT_OK) {
      break;
    }

    if ((result = m_stream->load_config()) != AM_RESULT_OK) {
      break;
    }

    if ((result = load_config()) != AM_RESULT_OK) {
      break;
    }

    if (result != AM_RESULT_OK) {
      break;
    }

    load_all_plugins();
  } while (0);
  return result;
}

AM_RESULT AMEncodeDevice::enter_preview_normal()
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    if ((result = enter_preview_normal_pre_routine()) != AM_RESULT_OK) {
      break;
    }
    if ((result = enter_preview()) != AM_RESULT_OK) {
      ERROR("Failed to enter preview!");
      break;
    }
    if ((result = enter_preview_normal_post_routine()) != AM_RESULT_OK) {
      break;
    }
  } while (0);
  return result;
}

AM_RESULT AMEncodeDevice::enter_preview_low()
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    if ((result = enter_preview_low_pre_routine()) != AM_RESULT_OK) {
      break;
    }
    if ((result = enter_preview()) != AM_RESULT_OK) {
      ERROR("Failed to enter preview!");
      break;
    }
    if ((result = enter_preview_low_post_routine()) != AM_RESULT_OK) {
      break;
    }
  } while (0);
  return result;
}

AM_RESULT AMEncodeDevice::goto_idle_normal()
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    if ((result = leave_preview_normal_pre_routine()) != AM_RESULT_OK) {
      break;
    }
    if ((result = goto_idle()) != AM_RESULT_OK) {
      ERROR("Failed to goto idle!");
      break;
    }
    if ((result = leave_preview_normal_post_routine()) != AM_RESULT_OK) {
      break;
    }
  } while (0);
  return result;
}

AM_RESULT AMEncodeDevice::goto_idle_low()
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    if ((result = leave_preview_low_pre_routine()) != AM_RESULT_OK) {
      break;
    }
    if ((result = goto_idle()) != AM_RESULT_OK) {
      ERROR("Failed to goto idle!");
      break;
    }
    if ((result = leave_preview_low_post_routine()) != AM_RESULT_OK) {
      break;
    }
  } while (0);
  return result;
}

AM_RESULT AMEncodeDevice::enter_preview_normal_pre_routine()
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    AM_POWER_MODE cur_mode = AM_POWER_MODE_HIGH;
    if ((result = m_platform->get_power_mode(cur_mode)) != AM_RESULT_OK) {
      break;
    }
    if ((cur_mode != AM_POWER_MODE_HIGH) &&
        ((result = m_platform->set_power_mode(AM_POWER_MODE_HIGH))
        != AM_RESULT_OK)) {
      ERROR("failed set encode to high power mode");
      break;
    }

    if ((result = m_platform->load_config()) != AM_RESULT_OK) {
      break;
    }

    if ((result = vin_start()) != AM_RESULT_OK) {
      result = AM_RESULT_ERR_DSP;
      ERROR("Failed to start VIN!");
      break;
    }

    if ((result = m_vout->start()) != AM_RESULT_OK) {
      result = AM_RESULT_ERR_DSP;
      ERROR("Failed to start VOUT!");
      break;
    }

    if (set_system_resource() != AM_RESULT_OK) {
      result = AM_RESULT_ERR_DSP;
      ERROR("Failed to set system resource!");
      break;
    }

    if ((result = m_buffer->setup()) != AM_RESULT_OK) {
      result = AM_RESULT_ERR_DSP;
      ERROR("Failed to setup Buffers!");
      break;
    }
  } while (0);
  return result;
}

AM_RESULT AMEncodeDevice::enter_preview_normal_post_routine()
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    for (AMVideoPluginMap::iterator iter = m_plugin_map_preview.begin();
        iter != m_plugin_map_preview.end();
        ++ iter) {
      AMIEncodePlugin *plugin = iter->second->plugin;
      if (AM_UNLIKELY(!plugin->start())) {
        ERROR("Failed to start encode plugin %s", plugin->name().c_str());
      } else {
        INFO("Encode plugin %s started successfully!", plugin->name().c_str());
      }
    }
  } while (0);
  return result;
}

AM_RESULT AMEncodeDevice::leave_preview_normal_pre_routine()
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    if ((result = m_vout->stop()) != AM_RESULT_OK) {
      result = AM_RESULT_ERR_DSP;
      ERROR("Failed to stop VOUT!");
      break;
    }
    for (AMVideoPluginMap::iterator iter = m_plugin_map_preview.begin();
        iter != m_plugin_map_preview.end();
        ++ iter) {
      AMIEncodePlugin *plugin = iter->second->plugin;
      if (AM_UNLIKELY(!plugin->stop())) {
        ERROR("Failed to stop encode plugin %s", plugin->name().c_str());
      } else {
        INFO("Encode plugin %s stopped successfully!", plugin->name().c_str());
      }
    }
  } while (0);

  return result;
}

AM_RESULT AMEncodeDevice::leave_preview_normal_post_routine()
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    if ((result = vin_stop()) != AM_RESULT_OK) {
      result = AM_RESULT_ERR_DSP;
      ERROR("Failed to stop VIN!");
      break;
    }
  } while (0);
  return result;
}

AM_RESULT AMEncodeDevice::enter_preview_low_pre_routine()
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    AM_POWER_MODE cur_mode = AM_POWER_MODE_HIGH;
    if ((result = m_platform->get_power_mode(cur_mode)) != AM_RESULT_OK) {
      break;
    }
    if ((cur_mode != AM_POWER_MODE_LOW) &&
        ((result = m_platform->set_power_mode(AM_POWER_MODE_LOW))
        != AM_RESULT_OK)) {
      ERROR("failed set encode to low power mode");
      break;
    }

    if ((result = m_platform->load_config()) != AM_RESULT_OK) {
      break;
    }

    if ((result = vin_start()) != AM_RESULT_OK) {
      result = AM_RESULT_ERR_DSP;
      ERROR("Failed to start VIN!");
      break;
    }

    if (set_system_resource_low() != AM_RESULT_OK) {
      result = AM_RESULT_ERR_DSP;
      ERROR("Failed to set system resource!");
      break;
    }

    if ((result = m_buffer->setup()) != AM_RESULT_OK) {
      result = AM_RESULT_ERR_DSP;
      ERROR("Failed to setup Buffers!");
      break;
    }
  } while (0);
  return result;
}

AM_RESULT AMEncodeDevice::enter_preview_low_post_routine()
{
  AM_RESULT result = AM_RESULT_OK;
  do {

  } while (0);
  return result;
}

AM_RESULT AMEncodeDevice::leave_preview_low_pre_routine()
{
  AM_RESULT result = AM_RESULT_OK;
  do {

  } while (0);

  return result;
}

AM_RESULT AMEncodeDevice::leave_preview_low_post_routine()
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    if ((result = vin_stop()) != AM_RESULT_OK) {
      result = AM_RESULT_ERR_DSP;
      ERROR("Failed to stop VIN!");
      break;
    }
  } while (0);
  return result;
}

AM_RESULT AMEncodeDevice::enter_encode_pre_routine(AM_STREAM_ID id)
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    if ((result = m_stream->setup(id)) != AM_RESULT_OK) {
      break;
    }
  } while (0);
  return result;
}

int32_t AMEncodeDevice::static_receive_data_from_md(void *obj_owner,
                                                    AMMDMessage *info)
{
  AMEncodeDevice *self = static_cast<AMEncodeDevice*>(obj_owner);
  self->receive_data_from_motion_detect(info);
  return 0;
}

void AMEncodeDevice::receive_data_from_motion_detect(AMMDMessage *info)
{
/*
  const char *motion_type[AM_MD_MOTION_TYPE_NUM] =
  { "MOTION_NONE", "MOTION_START", "MOTION_LEVEL_CHANGED", "MOTION_END" };
  const char *motion_level[AM_MOTION_L_NUM] =
  { "MOTION_LEVEL_0", "MOTION_LEVEL_1", "MOTION_LEVEL_2" };
  INFO("motion value: %d, motion event number: %llu, %s, %s, ROI#%d",
       info->mt_value,
       info->seq_num,
       motion_type[info->mt_type],
       motion_level[info->mt_level],
       info->roi_id);
*/

  AMILBRControl *lbr =
      (AMILBRControl*) get_video_plugin_interface(VIDEO_PLUGIN_LBR);
  if (AM_UNLIKELY(!lbr)) {
    DEBUG("Video Plugin %s is not loaded\n", VIDEO_PLUGIN_LBR_SO);
  } else {
    //LBR will receive motion info from this interface
    lbr->process_motion_info((void *)info);
  }

  AMISmartRC *smartrc =
      (AMISmartRC*) get_video_plugin_interface(VIDEO_PLUGIN_SMARTRC);
  if (AM_UNLIKELY(!smartrc)) {
    DEBUG("Video Plugin %s is not loaded\n", VIDEO_PLUGIN_SMARTRC_SO);
  } else {
    //SmartRC will receive motion info from this interface
    smartrc->process_motion_info((void *)info);
  }
}

AM_RESULT AMEncodeDevice::enter_encode_post_routine(AM_STREAM_ID id)
{
  AM_RESULT result = AM_RESULT_OK;
  AMMDMessage *m_msg = nullptr;

  do {
    for (AMVideoPluginMap::iterator iter = m_plugin_map_encode.begin();
        iter != m_plugin_map_encode.end();
        ++ iter) {
      AMIEncodePlugin *plugin = iter->second->plugin;
      if (AM_UNLIKELY(!plugin->start(id))) {
        ERROR("Failed to start encode plugin %s", plugin->name().c_str());
      } else {
        INFO("Encode plugin %s started successfully!", plugin->name().c_str());
        if (0 == strcmp(iter->first.c_str(), VIDEO_PLUGIN_MD)) {
          /*Set callback function after motion detect plugin starting.
           *callback function receive motion info from motion detect.
           */
          AMIMotionDetect *video_md =
              (AMIMotionDetect *)iter->second->plugin->get_interface();
          if (AM_UNLIKELY(!video_md->set_md_callback(this,
                                                    static_receive_data_from_md,
                                                    m_msg))) {
            ERROR("Set motion detect callback function failed");
          }
        }
      }
    }
  } while (0);
  return result;
}

AM_RESULT AMEncodeDevice::leave_encode_pre_routine(AM_STREAM_ID id)
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    for (AMVideoPluginMap::iterator iter = m_plugin_map_encode.begin();
        iter != m_plugin_map_encode.end();
        ++ iter) {
      AMIEncodePlugin *plugin = iter->second->plugin;
      if (AM_UNLIKELY(!plugin->stop(id))) {
        ERROR("Failed to stop encode plugin %s", plugin->name().c_str());
      } else {
        INFO("Encode plugin %s stopped successfully!", plugin->name().c_str());
      }
    }
  } while (0);
  return result;
}

AM_RESULT AMEncodeDevice::leave_encode_post_routine(AM_STREAM_ID id)
{
  AM_RESULT result = AM_RESULT_OK;
  do {

  } while (0);
  return result;
}

AM_RESULT AMEncodeDevice::enter_decode_routine()
{
  AM_RESULT result = AM_RESULT_OK;
  do {

  } while (0);
  return result;
}

AM_RESULT AMEncodeDevice::leave_decode_routine()
{
  AM_RESULT result = AM_RESULT_OK;
  do {

  } while (0);
  return result;
}

AM_RESULT AMEncodeDevice::idle_routine_from_decode()
{
  AM_RESULT result = AM_RESULT_OK;
  do {

  } while (0);
  return result;
}

AM_RESULT AMEncodeDevice::goto_idle()
{
  return m_platform->goto_idle();
}

AM_RESULT AMEncodeDevice::enter_preview()
{
  return m_platform->goto_preview();
}

AM_RESULT AMEncodeDevice::start_encode(AM_STREAM_ID id)
{
  return m_stream->start_encode(id);
}

AM_RESULT AMEncodeDevice::stop_encode(AM_STREAM_ID id)
{
  return m_stream->stop_encode(id);
}

AM_RESULT AMEncodeDevice::start_decode()
{
 return m_platform->decode_start();
}

AM_RESULT AMEncodeDevice::stop_decode()
{
  return m_platform->decode_stop();
}

AM_IAV_STATE AMEncodeDevice::get_iav_state(AM_IAV_STATE target,
                                           AM_STREAM_ID id)
{
  AM_IAV_STATE iav_state = AM_IAV_STATE_ERROR;
  if (m_platform->iav_state_get(iav_state) != AM_RESULT_OK) {
    iav_state = AM_IAV_STATE_ERROR;
  } else {
    const AMStreamStateMap &stream_state = m_stream->stream_state();
    switch(iav_state) {
      case AM_IAV_STATE_PREVIEW: {
        bool is_encoding = false;
        for (auto &m : stream_state) {
          const AMStreamFormatConfig& stream_cfg =
              m_stream->stream_config(m.first);
          if ((id != AM_STREAM_ID_MAX) && (m.first != id)) {
            continue;
          }
          if (!stream_cfg.enable.second) {
            continue;
          }
          if ((m.second == AM_STREAM_STATE_STARTING) ||
              (m.second == AM_STREAM_STATE_ENCODING)) {
            if (m.second == AM_STREAM_STATE_STARTING) {
              NOTICE("Stream[%d] is still starting, consider it is encoding!",
                     m.first);
            }
            is_encoding = true;
          }
        }
        iav_state = is_encoding ? AM_IAV_STATE_ENCODING : AM_IAV_STATE_PREVIEW;
      }break;
      case AM_IAV_STATE_ENCODING: {
        bool is_encoding = true;
        for (auto &m : stream_state) {
          const AMStreamFormatConfig& stream_cfg =
              m_stream->stream_config(m.first);
          if ((id != AM_STREAM_ID_MAX) && (m.first != id)) {
            continue;
          }
          if (!stream_cfg.enable.second) {
            continue;
          }
          switch(target) {
            case AM_IAV_STATE_ENCODING: {
              /* When target is encoding, if there's any stream that is enabled
               * but still in IDLE state, we should set IAV state to PREVIEW
               * to make the IAV state machine go on start encoding procedure
               */
              if (m.second == AM_STREAM_STATE_IDLE) {
                is_encoding = false;
              }
            }break;
            default: {
              /* If target is not encoding, if a single stream is in IDLE state
               * but other streams are still encoding, we should set IAV state
               * to PREVIEW to make the IAV state machine think that this single
               * stream is already stopped
               */
              if ((m.second == AM_STREAM_STATE_IDLE) &&
                  (id != AM_STREAM_ID_MAX)) {
                is_encoding = false;
              }
            }break;
          }
        }
        iav_state = is_encoding ? AM_IAV_STATE_ENCODING : AM_IAV_STATE_PREVIEW;
      }break;
      default: break;
    }
  }

  return iav_state;
}

AM_RESULT AMEncodeDevice::change_iav_state(AM_IAV_STATE target,
                                           AM_STREAM_ID id)
{
  AUTO_MTX_LOCK(m_iav_lock);
  const uint32_t MAX_TRY_TIMES = 5;
  AM_RESULT result = AM_RESULT_OK;
  std::pair<AM_IAV_STATE, uint32_t> last_state = {AM_IAV_STATE_INIT, 0};

  if (target == AM_IAV_STATE_INIT ||
      target == AM_IAV_STATE_EXITING_PREVIEW ||
      target == AM_IAV_STATE_ERROR) {
    ERROR("Invalid IAV state: %d!", target);
    return result;
  }

  while (true) {
    m_state = get_iav_state(target, id);
    if (AM_STREAM_ID_MAX == id) {
      PRINTF("IAV state: %s %s %s!",
             AMVideoTrans::iav_state_to_str(m_state).c_str(),
             (m_state == target) ? "==" : "-->",
                 AMVideoTrans::iav_state_to_str(target).c_str());
    } else {
      AM_STREAM_STATE s_state = AM_STREAM_STATE_IDLE;
      m_platform->stream_state_get(id, s_state);
      PRINTF("STREAM[%d] state: %s %s %s!",
             id,
             AMVideoTrans::stream_state_to_str(s_state).c_str(),
             (m_state == target) ? "==" : "-->",
             ((target == AM_IAV_STATE_ENCODING) ?
                 AMVideoTrans::stream_state_to_str(
                     AM_STREAM_STATE_ENCODING).c_str() :
                 ((target == AM_IAV_STATE_PREVIEW) ?
                 AMVideoTrans::stream_state_to_str(
                     AM_STREAM_STATE_IDLE).c_str() :
                 AMVideoTrans::stream_state_to_str(
                     AM_STREAM_STATE_IDLE).c_str())));
    }

    if (m_state == target) {
      break;
    }

    if ((m_state == last_state.first) &&
        (++last_state.second >= MAX_TRY_TIMES)) {
      result = AM_RESULT_ERR_DSP;
      ERROR("Failed to change IAV state!");
      break;
    }

    switch (last_state.first = m_state) {
      case AM_IAV_STATE_INIT: {
        result = goto_idle();
      } break;

      case AM_IAV_STATE_IDLE: {
        switch (target) {
          case AM_IAV_STATE_PREVIEW:
          case AM_IAV_STATE_ENCODING:
            if ((result = enter_preview_normal()) != AM_RESULT_OK) {
              ERROR("Failed to enter normal mode preview!");
              break;
            }
            break;
          case AM_IAV_STATE_PREVIEW_LOW:
            if ((result = enter_preview_low()) != AM_RESULT_OK) {
              ERROR("Failed to enter low mode preview!");
              break;
            }
            break;
          case AM_IAV_STATE_DECODING:
            if ((result = enter_decode_routine()) != AM_RESULT_OK) {
              break;
            }
            if ((result = start_decode()) != AM_RESULT_OK) {
              ERROR("Failed to start decode!");
              break;
            }
            break;
          default:
            break;
        }
      } break;

      case AM_IAV_STATE_PREVIEW: {
        switch (target) {
          case AM_IAV_STATE_ENCODING:
            if ((result = enter_encode_pre_routine(id)) != AM_RESULT_OK) {
              break;
            }
            if ((result = start_encode(id)) != AM_RESULT_OK) {
              if (result == AM_RESULT_ERR_PERM) {
                WARN("Enter preview state!");
                target = AM_IAV_STATE_PREVIEW;
                result = AM_RESULT_OK;
              } else {
                ERROR("Failed to start encode!");
              }
              break;
            }
            if ((result = enter_encode_post_routine(id)) != AM_RESULT_OK) {
              break;
            }
            break;
          case AM_IAV_STATE_IDLE:
          case AM_IAV_STATE_PREVIEW_LOW:
            if ((result = goto_idle_normal()) != AM_RESULT_OK) {
              ERROR("Failed goto idle from normal preview mode!");
              break;
            }
            break;
          case AM_IAV_STATE_DECODING:
            if ((result = goto_idle()) != AM_RESULT_OK) {
              ERROR("Failed to goto idle!");
              break;
            }
            if ((result = idle_routine_from_decode()) != AM_RESULT_OK) {
              break;
            }
            break;
          default:
            break;
        }
      } break;

      case AM_IAV_STATE_PREVIEW_LOW: {
        switch (target) {
          case AM_IAV_STATE_IDLE:
          case AM_IAV_STATE_PREVIEW:
          case AM_IAV_STATE_ENCODING:
            if ((result = goto_idle_low()) != AM_RESULT_OK) {
              ERROR("Failed goto idle from low preview mode!");
              break;
            }
            break;
          default:
            break;
        }
      } break;

      case AM_IAV_STATE_ENCODING: {
        if ((result = leave_encode_pre_routine(id)) != AM_RESULT_OK) {
          break;
        }
        if ((result = stop_encode(id)) != AM_RESULT_OK) {
          break;
        }
        if ((result = leave_encode_post_routine(id)) != AM_RESULT_OK) {
          break;
        }
      } break;

      case AM_IAV_STATE_EXITING_PREVIEW: {
        usleep(100000);
      } break;

      case AM_IAV_STATE_DECODING: {
        if ((result = leave_decode_routine()) == AM_RESULT_OK) {
          result = stop_decode();
        }
      } break;

      case AM_IAV_STATE_ERROR: {
      } break;

      default: {
      } break;
    }
    if (result != AM_RESULT_OK) {
      break;
    }
  }

  return result;
}

AM_RESULT AMEncodeDevice::set_system_resource()
{
  AM_RESULT result = AM_RESULT_OK;

  do {
    AMBufferParamMap buffer_param;
     if ((result = m_buffer->get_param(buffer_param)) != AM_RESULT_OK) {
       ERROR("Failed to get buffer param!");
       break;
     }

    AMStreamParamMap stream_param;
     if ((result = m_stream->get_param(stream_param)) != AM_RESULT_OK) {
       ERROR("Failed to get stream param!");
       break;
     }

     if ((result = m_platform->system_resource_set(buffer_param, stream_param))
         != AM_RESULT_OK) {
       ERROR("Failed to set system resource limit!");
     }
  } while (0);

  return result;
}

AM_RESULT AMEncodeDevice::set_system_resource_low()
{
  AM_RESULT result = AM_RESULT_OK;

  do {
    AMBufferParamMap buffer_param;
    AMStreamParamMap stream_param;
     if ((result = m_platform->system_resource_set(buffer_param, stream_param))
         != AM_RESULT_OK) {
       ERROR("Failed to set system resource limit!");
     }
  } while (0);

  return result;
}

void* AMEncodeDevice::get_video_plugin_interface(const std::string &plugin_name)
{
  AMVideoPluginMap::iterator iter_prev = m_plugin_map_preview.find(plugin_name);
  AMVideoPluginMap::iterator iter_enc = m_plugin_map_encode.find(plugin_name);
  return ((iter_enc != m_plugin_map_encode.end()) ?
      (iter_enc->second->plugin->get_interface()) :
      ((iter_prev != m_plugin_map_preview.end()) ?
          iter_prev->second->plugin->get_interface() :
          nullptr));
}
