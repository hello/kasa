/*
 * am_audio_alert.h
 *
 * Histroy:
 *  2013-01-24 [Hanbo Xiao] created file
 *  2014-11-19 [Dongge Wu] Reconstruct and porting to oryx
 *
 * Copyright (C) 2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
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
    EVENT_MODULE_ID m_plugin_id;
    int32_t m_alert_sensitivity;
    int32_t m_alert_threshold;
    int32_t m_alert_direction;
    int64_t m_volume_sum;
    int32_t m_sample_num;
    int32_t m_bits_per_sample;
    int32_t m_maximum_peak;
    int32_t m_micro_history_head;
    int32_t m_macro_history_head;
    int32_t m_micro_history_tail;
    int32_t m_macro_history_tail;
    int32_t *m_micro_history;
    int64_t *m_macro_history;
    AM_EVENT_CALLBACK m_callback;
    snd_pcm_format_t m_snd_pcm_format;
    AM_AUDIO_SAMPLE_FORMAT m_audio_format;
    AMIAudioCapture *m_audio_capture;
    uint64_t m_seq_num;
    bool m_enable_alert_detect;
    int32_t m_reference_count;
    AMAudioAlertConfig *m_alert_config;
    std::string m_conf_path;
    bool m_running_state;
};

#endif
