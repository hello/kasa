/*
 * am_audio_alert.h
 *
 * Histroy:
 *  2013-01-24 [Hanbo Xiao] created file
 *  2014-11-19 [Dongge Wu] Reconstruct and porting to oryx
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

#ifndef AM_AUDIO_ALERT_H
#define AM_AUDIO_ALERT_H

#include <asm/byteorder.h>
#include <alsa/asoundlib.h>
#include "am_audio_capture_if.h"

#define MICRO_AVERAGE_SAMPLES 8
#define MACRO_AVERAGE_SAMPLES 48000

class AMAudioAlert: public AMIEventPlugin
{
  public:
    DECLARE_EVENT_PLUGIN_INTERFACE
    bool set_alert_sensitivity(int32_t sensitivity);
    int32_t get_alert_sensitivity();
    bool set_alert_threshold(int32_t threshold);
    int32_t get_alert_threshold();
    AM_AUDIO_SAMPLE_FORMAT get_audio_format();
    bool set_alert_direction(int32_t direction);
    int32_t get_alert_direction();
    bool set_alert_callback(AM_EVENT_CALLBACK callback);
    bool set_alert_state(bool enable);
    bool get_alert_state();
    bool is_silent(int32_t percent = 5);
    bool sync_config();

    static void static_audio_alert_detect(AudioCapture *p);

    //AMIEventPlugin
    virtual bool start_plugin();
    virtual bool stop_plugin();
    virtual bool set_plugin_config(EVENT_MODULE_CONFIG *pConfig);
    virtual bool get_plugin_config(EVENT_MODULE_CONFIG *pConfig);
    virtual EVENT_MODULE_ID get_plugin_ID();

  public:
    virtual ~AMAudioAlert();

  private:
    AMAudioAlert(EVENT_MODULE_ID mid,
                 uint32_t sensitivity = 50,
                 uint32_t threshold = 50);
    bool construct();
    int32_t trigger_event(int32_t micro_data);
    int32_t store_macro_data(int64_t macro_data);
    int32_t audio_alert_detect(AudioPacket *pkg);
    bool set_audio_format(AM_AUDIO_SAMPLE_FORMAT format);

  private:
    int64_t  m_volume_sum;
    uint64_t m_seq_num;
    int32_t *m_micro_history;
    int64_t *m_macro_history;
    AM_EVENT_CALLBACK   m_callback;
    AMIAudioCapture    *m_audio_capture;
    AMAudioAlertConfig *m_alert_config;
    EVENT_MODULE_ID m_plugin_id;
    int32_t m_alert_sensitivity;
    int32_t m_alert_threshold;
    int32_t m_alert_direction;
    int32_t m_sample_num;
    int32_t m_bits_per_sample;
    int32_t m_maximum_peak;
    int32_t m_micro_history_head;
    int32_t m_macro_history_head;
    int32_t m_micro_history_tail;
    int32_t m_macro_history_tail;
    snd_pcm_format_t m_snd_pcm_format;
    AM_AUDIO_SAMPLE_FORMAT m_audio_format;
    int32_t m_reference_count;
    bool m_enable_alert_detect;
    bool m_running_state;
    std::string m_conf_path;
};

#endif
