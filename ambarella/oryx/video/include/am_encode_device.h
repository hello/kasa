/**
 * am_encode_device.h
 *
 *  History:
 *    Jul 10, 2015 - [Shupeng Ren] created file
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
#ifndef ORYX_VIDEO_INCLUDE_AM_ENCODE_DEVICE_H_
#define ORYX_VIDEO_INCLUDE_AM_ENCODE_DEVICE_H_

#include "am_platform_if.h"
#include "am_motion_detect_types.h"

using std::vector;
using std::map;

class AMVin;
class AMVout;
class AMMutex;
class AMPlugin;
class AMEncodeBuffer;
class AMEncodeStream;
class AMEncodeOverlay;
class AMIEncodeWarp;
class AMIEncodePlugin;

struct AMVideoPlugin
{
    AMPlugin        *so;
    AMIEncodePlugin *plugin;
    AMVideoPlugin();
    AMVideoPlugin(const AMVideoPlugin &vplugin);
    ~AMVideoPlugin();
};
typedef std::map<std::string, AMVideoPlugin*> AMVideoPluginMap;

class AMEncodeDevice
{
  public:
    static AMEncodeDevice *create();
    virtual void destroy();

    virtual AM_RESULT start();
    virtual AM_RESULT start_stream(AM_STREAM_ID id);
    virtual AM_RESULT stop();
    virtual AM_RESULT stop_stream(AM_STREAM_ID id);

    virtual AM_RESULT goto_low_power_mode();

    virtual AM_RESULT idle();
    virtual AM_RESULT preview(AM_STREAM_ID id = AM_STREAM_ID_MAX,
                              AM_POWER_MODE mode = AM_POWER_MODE_HIGH);
    virtual AM_RESULT encode(AM_STREAM_ID id = AM_STREAM_ID_MAX);
    virtual AM_RESULT decode();

    virtual AM_RESULT get_stream_status(AM_STREAM_ID id,
                                        AM_STREAM_STATE &state);
    virtual AM_RESULT set_stream_param(AM_STREAM_ID id,
                                       AMStreamConfigParam &param);
    virtual AM_RESULT save_stream_config();

    virtual AM_RESULT get_buffer_state(AM_SOURCE_BUFFER_ID id,
                                       AM_SRCBUF_STATE &state);

    virtual AM_RESULT get_buffer_format(AMBufferConfigParam &param);
    virtual AM_RESULT set_buffer_format(const AMBufferConfigParam &param);
    virtual AM_RESULT save_buffer_config();

    virtual AM_RESULT halt_vout(AM_VOUT_ID id);

    virtual AM_RESULT force_idr(AM_STREAM_ID stream_id);

    /* For video Plugin */
    virtual void* get_video_plugin_interface(const std::string &plugin_name);

    virtual AM_RESULT load_config();
    virtual AM_RESULT load_config_all();

  private:
    virtual AM_RESULT goto_idle();
    virtual AM_RESULT enter_preview();
    virtual AM_RESULT start_encode(AM_STREAM_ID id);
    virtual AM_RESULT stop_encode(AM_STREAM_ID id);
    virtual AM_RESULT start_decode();
    virtual AM_RESULT stop_decode();

    virtual AM_RESULT change_iav_state(AM_IAV_STATE state,
                                       AM_STREAM_ID id = AM_STREAM_ID_MAX);
    virtual AM_IAV_STATE get_iav_state(AM_IAV_STATE target, AM_STREAM_ID id);

    virtual AM_RESULT enter_preview_normal();
    virtual AM_RESULT enter_preview_low();
    virtual AM_RESULT goto_idle_normal();
    virtual AM_RESULT goto_idle_low();

    virtual AM_RESULT enter_preview_normal_pre_routine();
    virtual AM_RESULT enter_preview_normal_post_routine();
    virtual AM_RESULT enter_preview_low_pre_routine();
    virtual AM_RESULT enter_preview_low_post_routine();
    virtual AM_RESULT enter_encode_pre_routine(AM_STREAM_ID id);
    virtual AM_RESULT enter_encode_post_routine(AM_STREAM_ID id);
    virtual AM_RESULT leave_encode_pre_routine(AM_STREAM_ID id);
    virtual AM_RESULT leave_encode_post_routine(AM_STREAM_ID id);
    virtual AM_RESULT leave_preview_normal_pre_routine();
    virtual AM_RESULT leave_preview_normal_post_routine();
    virtual AM_RESULT leave_preview_low_pre_routine();
    virtual AM_RESULT leave_preview_low_post_routine();

    virtual AM_RESULT enter_decode_routine();
    virtual AM_RESULT leave_decode_routine();
    virtual AM_RESULT idle_routine_from_decode();

    virtual AM_RESULT set_system_resource();
    virtual AM_RESULT set_system_resource_low();

  private:
    AM_RESULT vin_create();
    AM_RESULT vin_start();
    AM_RESULT vin_stop();
    AM_RESULT vin_delete();
    void load_all_plugins();
    AMVideoPlugin* load_video_plugin(const char *name, void *data);
    static int32_t static_receive_data_from_md(void *obj_owner,
                                               AMMDMessage *info);
    void receive_data_from_motion_detect(AMMDMessage *info);

  private:
    AMEncodeDevice();
    virtual ~AMEncodeDevice();
    AM_RESULT init();

  private:
    AMEncodeBuffer       *m_buffer;
    AMEncodeStream       *m_stream;
    AMVout               *m_vout;
    AMMutex              *m_iav_lock;
    AMIPlatformPtr        m_platform;
    AM_IAV_STATE          m_state;
    vector<AMVin*>        m_vin;
    AMVideoPluginMap      m_plugin_map_encode;
    AMVideoPluginMap      m_plugin_map_preview;
};

#endif /* ORYX_VIDEO_INCLUDE_AM_ENCODE_DEVICE_H_ */
