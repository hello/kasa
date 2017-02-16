/*
 * am_audio_alert.cpp
 *
 * Histroy:
 *  2013-01-24 [Hanbo Xiao] created file
 *  2014-11-19 [Dongge Wu] Reconstruct plugin and porting to oryx
 *
 * Copyright (C) 2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include "am_base_include.h"
#include "am_log.h"
#include "am_define.h"

#include "am_event_types.h"
#include "am_base_event_plugin.h"
#include "am_audio_capture_if.h"
#include "am_audio_alert_config.h"
#include "am_audio_alert.h"

#define AUDIO_ALERT_CONFIG ((const char*)"event-audio-alert.acs")

static std::mutex audio_alert_mtx;

DECLARE_EVENT_PLUGIN_INIT_FINIT(AMAudioAlert, EV_AUDIO_ALERT_DECT)

AMIEventPlugin* AMAudioAlert::create(EVENT_MODULE_ID mid)
{
  INFO("AMAudioAlert::Create \n");
  AMAudioAlert *result = new AMAudioAlert(mid);
  if (result && !result->construct()) {
    ERROR("Failed to create an instance of AMAudioAlert");
    delete result;
    result = NULL;
  }

  return result;
}

AMAudioAlert::AMAudioAlert(EVENT_MODULE_ID mid,
                           uint32_t sensitivity,
                           uint32_t threshold) :
    m_plugin_id(mid),
    m_alert_sensitivity(sensitivity),
    m_alert_threshold(threshold),
    m_alert_direction(0),
    m_volume_sum(0LL),
    m_sample_num(0),
    m_bits_per_sample(0),
    m_maximum_peak(0),
    m_micro_history_head(0),
    m_macro_history_head(0),
    m_micro_history_tail(0),
    m_macro_history_tail(0),
    m_micro_history(NULL),
    m_macro_history(NULL),
    m_callback(NULL),
    m_snd_pcm_format(SND_PCM_FORMAT_UNKNOWN),
    m_audio_format(AM_SAMPLE_INVALID),
    m_audio_capture(NULL),
    m_seq_num(0UL),
    m_enable_alert_detect(false),
    m_reference_count(AM_AUDIO_ALERT_MAX_SENSITIVITY - sensitivity),
    m_alert_config(NULL),
    m_conf_path(ORYX_EVENT_CONF_DIR),
    m_running_state(false)
{
}

AMAudioAlert::~AMAudioAlert()
{
  INFO("AMAudioAlert::~AMAudioAlert \n");
  /*save config file default*/
  if (!sync_config()) {
    ERROR("save config file failed!\n");
  }

  m_running_state = false;

  if (m_alert_config) {
    delete m_alert_config;
  }
  if (m_audio_capture) {
    delete m_audio_capture;
  }
  if (m_micro_history) {
    free(m_micro_history);
  }

  if (m_macro_history) {
    free(m_macro_history);
  }
}

bool AMAudioAlert::construct()
{
  m_audio_capture = create_audio_capture("pulse",
                                         "AudioAlert",
                                         (void*) this,
                                         static_audio_alert_detect);
  if (NULL == m_audio_capture) {
    ERROR("Failed to create AmPulseAudioCapture object!\n");
    return false;
  }

  m_alert_config = new AMAudioAlertConfig();
  if (!m_alert_config) {
    ERROR("Failed to new AMAudioAlertConfig object!\n");
    return false;
  }

  m_conf_path.append(ORYX_EVENT_CONF_SUB_DIR).append(AUDIO_ALERT_CONFIG);
  AudioAlertConfig * audio_alert_config =
      m_alert_config->get_config(m_conf_path);
  if (!audio_alert_config) {
    ERROR("audio alert get config failed !\n");
    return false;
  }

  if (audio_alert_config->sensitivity
      < 0|| audio_alert_config->sensitivity > AM_AUDIO_ALERT_MAX_SENSITIVITY) {
    ERROR("audio alert config sensitivity is invalid!\n");
    return false;
  }
  m_alert_sensitivity = audio_alert_config->sensitivity;
  m_reference_count = AM_AUDIO_ALERT_MAX_SENSITIVITY - m_alert_sensitivity;

  if (audio_alert_config->threshold
      < 0|| audio_alert_config->threshold > AM_AUDIO_ALERT_MAX_THRESHOLD) {
    ERROR("audio alert config threshold is invalid!\n");
    return false;
  }
  m_alert_threshold = audio_alert_config->threshold;

  m_alert_direction = audio_alert_config->direction;
  m_enable_alert_detect = audio_alert_config->enable_alert_detect;
  m_audio_format = audio_alert_config->sample_format;

  m_micro_history = (int32_t*) malloc((m_reference_count + 1)
      * sizeof(int32_t));
  if (m_micro_history == NULL) {
    ERROR("new failed for m_micro_history!\n");
    return false;
  }
  memset(m_micro_history, 0, sizeof(int32_t) * (m_reference_count + 1));

  m_macro_history = (int64_t*) malloc((m_reference_count + 1)
      * sizeof(int64_t));
  if (m_macro_history == NULL) {
    ERROR("new failed for m_macro_history!\n");
    return false;
  }
  memset(m_macro_history, 0, sizeof(int64_t) * (m_reference_count + 1));

  //default init
  m_audio_capture->set_channel(audio_alert_config->channel_num);
  m_audio_capture->set_sample_rate(audio_alert_config->sample_rate);
  m_audio_capture->set_chunk_bytes(audio_alert_config->chunk_bytes);
  return set_audio_format(m_audio_format);
}

bool AMAudioAlert::set_alert_sensitivity(int32_t sensitivity)
{
  if (sensitivity < 0 || sensitivity > AM_AUDIO_ALERT_MAX_SENSITIVITY) {
    ERROR("Set audio alert sensitivity vaule error!\n");
    return false;
  }

  if (m_alert_sensitivity != sensitivity) {
    AUTO_LOCK(audio_alert_mtx);
    m_alert_sensitivity = sensitivity;
    m_reference_count = AM_AUDIO_ALERT_MAX_SENSITIVITY - sensitivity;

    m_micro_history = (int32_t*) realloc(m_micro_history,
                                         (m_reference_count + 1)
                                             * sizeof(int32_t));
    if (m_micro_history == NULL) {
      ERROR("realloc failed for m_micro_history!\n");
      return false;
    }

    m_macro_history = (int64_t*) realloc(m_macro_history,
                                         (m_reference_count + 1)
                                             * sizeof(int64_t));
    if (m_macro_history == NULL) {
      ERROR("realloc failed for m_macro_history!\n");
      free(m_micro_history);
      m_micro_history = NULL;
      return false;
    }

    memset(m_micro_history, 0, sizeof(int32_t) * (m_reference_count + 1));
    memset(m_macro_history, 0, sizeof(int64_t) * (m_reference_count + 1));
    m_micro_history_head = 0;
    m_macro_history_head = 0;
    m_micro_history_tail = 0;
    m_macro_history_tail = 0;
  }

  return true;
}

int32_t AMAudioAlert::get_alert_sensitivity()
{
  AUTO_LOCK(audio_alert_mtx);
  return m_alert_sensitivity;
}

bool AMAudioAlert::set_alert_direction(int32_t alert_direction)
{
  AUTO_LOCK(audio_alert_mtx);
  m_alert_direction = alert_direction;
  return true;
}

int32_t AMAudioAlert::get_alert_direction()
{
  AUTO_LOCK(audio_alert_mtx);
  return m_alert_direction;
}

bool AMAudioAlert::set_alert_threshold(int32_t threshold)
{
  if (threshold < 0 || threshold > AM_AUDIO_ALERT_MAX_THRESHOLD) {
    ERROR("threshold is invalid!\n");
    return false;
  }

  AUTO_LOCK(audio_alert_mtx);
  m_alert_threshold = threshold;
  return true;
}
int32_t AMAudioAlert::get_alert_threshold()
{
  AUTO_LOCK(audio_alert_mtx);
  return m_alert_threshold;
}

AM_AUDIO_SAMPLE_FORMAT AMAudioAlert::get_audio_format()
{
  AUTO_LOCK(audio_alert_mtx);
  return m_audio_format;
}

bool AMAudioAlert::set_alert_callback(AM_EVENT_CALLBACK callback)
{
  if (callback == NULL) {
    ERROR("callback is NULL!\n");
    return false;
  }

  AUTO_LOCK(audio_alert_mtx);
  m_callback = callback;
  return true;
}

bool AMAudioAlert::set_alert_state(bool enable)
{
  AUTO_LOCK(audio_alert_mtx);
  m_enable_alert_detect = enable;
  return true;
}

bool AMAudioAlert::get_alert_state()
{
  AUTO_LOCK(audio_alert_mtx);
  return m_enable_alert_detect;
}

bool AMAudioAlert::is_silent(int32_t percent)
{
  bool ret = true;

  if (percent < 0 || percent >= 100) {
    ERROR("Invalid argument, percent [0 - 100]: %d", percent);
    return false;
  }

  AUTO_LOCK(audio_alert_mtx);
  if (m_macro_history == NULL) {
    ERROR("m_macro_history is NULL !\n");
    return false;
  }

  for (int32_t i = 0; i < m_reference_count + 1; i ++) {
    if (m_macro_history[i] * 100>=
    m_maximum_peak * percent * MACRO_AVERAGE_SAMPLES) {
      ret = false;
      break;
    }
  }

  return ret;
}

bool AMAudioAlert::sync_config()
{
  AUTO_LOCK(audio_alert_mtx);
  AudioAlertConfig * audio_alert_config =
      m_alert_config->get_config(m_conf_path);
  if (!audio_alert_config) {
    ERROR("audio alert get config failed !\n");
    return false;
  }

  audio_alert_config->channel_num = m_audio_capture->get_channel();
  audio_alert_config->chunk_bytes = m_audio_capture->get_chunk_bytes();
  audio_alert_config->sample_rate = m_audio_capture->get_sample_rate();
  audio_alert_config->sample_format = m_audio_format;
  audio_alert_config->enable_alert_detect = m_enable_alert_detect;
  audio_alert_config->sensitivity = m_alert_sensitivity;
  audio_alert_config->threshold = m_alert_threshold;
  audio_alert_config->direction = m_alert_direction;

  return m_alert_config->set_config(audio_alert_config, m_conf_path);
}

bool AMAudioAlert::start_plugin()
{
  bool ret = true;
  AUTO_LOCK(audio_alert_mtx);

  if (!m_running_state) {
    if (!m_audio_capture->start()) {
      ret = false;
      ERROR("failed to start audio alert!\n");
    } else {
      INFO("start audio alert!\n");
      m_running_state = true;
    }
  } else {
    NOTICE("audio alert is already running!\n");
  }

  return ret;
}

bool AMAudioAlert::stop_plugin()
{
  bool ret = true;
  AUTO_LOCK(audio_alert_mtx);
  if (m_running_state) {
    if (!m_audio_capture->stop()) {
      ret = false;
      ERROR("failed to stop audio alert !\n");
    } else {
      INFO("stop audio alert!\n");
      m_running_state = false;
    }
  } else {
    NOTICE("audio alert is already stopped!\n");
  }

  return ret;
}

bool AMAudioAlert::set_plugin_config(EVENT_MODULE_CONFIG *pConfig)
{
  bool ret = true;

  if (pConfig == NULL || pConfig->value == NULL) {
    ERROR("Invalid argument!\n");
    return false;
  }

  switch (pConfig->key) {
    case AM_CHANNEL_NUM:
      ret = m_audio_capture->set_channel(*((uint32_t*) pConfig->value));
      break;
    case AM_SAMPLE_RATE:
      ret = m_audio_capture->set_sample_rate(*((uint32_t*) pConfig->value));
      break;
    case AM_CHUNK_BYTES:
      ret = m_audio_capture->set_chunk_bytes(*((uint32_t*) pConfig->value));
      break;
    case AM_SAMPLE_FORMAT:
      ret = set_audio_format(*((AM_AUDIO_SAMPLE_FORMAT*) pConfig->value));
      break;
    case AM_ALERT_ENABLE:
      ret = set_alert_state(!!*((int32_t *) pConfig->value));
      break;
    case AM_ALERT_CALLBACK:
      ret = set_alert_callback((AM_EVENT_CALLBACK) pConfig->value);
      break;
    case AM_ALERT_SENSITIVITY:
      ret = set_alert_sensitivity(*(int32_t *) pConfig->value);
      break;
    case AM_ALERT_THRESHOLD:
      ret = set_alert_threshold(*(int32_t *) pConfig->value);
      break;
    case AM_ALERT_DIRECTION:
      ret = set_alert_direction(*((int32_t *) pConfig->value));
      break;
    case AM_ALERT_SYNC_CONFIG:
      ret = sync_config();
      break;
    default:
      ERROR("Unknown key\n");
      ret = false;
      break;
  }

  return ret;
}

bool AMAudioAlert::get_plugin_config(EVENT_MODULE_CONFIG *pConfig)
{
  bool ret = true;

  if (pConfig == NULL || pConfig->value == NULL) {
    ERROR("Invalid argument!\n");
    return false;
  }

  switch (pConfig->key) {
    case AM_CHANNEL_NUM:
      *((int32_t*) pConfig->value) = m_audio_capture->get_channel();
      break;
    case AM_SAMPLE_RATE:
      *((int32_t*) pConfig->value) = m_audio_capture->get_sample_rate();
      break;
    case AM_CHUNK_BYTES:
      *((int32_t*) pConfig->value) = m_audio_capture->get_chunk_bytes();
      break;
    case AM_SAMPLE_FORMAT:
      *((int32_t*) pConfig->value) = m_audio_capture->get_sample_format();
      break;
    case AM_ALERT_ENABLE:
      *((bool*) pConfig->value) = get_alert_state();
      break;
    case AM_ALERT_SENSITIVITY:
      *((int32_t*) pConfig->value) = get_alert_sensitivity();
      break;
    case AM_ALERT_THRESHOLD:
      *((int32_t*) pConfig->value) = get_alert_threshold();
      break;
    case AM_ALERT_DIRECTION:
      *((int32_t*) pConfig->value) = get_alert_direction();
      break;
    default:
      ERROR("Unknown key\n");
      ret = false;
      break;
  }

  return ret;
}

EVENT_MODULE_ID AMAudioAlert::get_plugin_ID()
{
  return m_plugin_id;
}

int32_t AMAudioAlert::store_macro_data(int64_t macro_data)
{
  if (m_macro_history == NULL) {
    ERROR("m_macro_history is NULL\n");
    return -1;
  }

  m_macro_history[m_macro_history_head] = macro_data;
  if (m_macro_history_head == m_macro_history_tail) {
    m_macro_history_tail = (m_macro_history_tail + 1) % (m_reference_count + 1);
  }

  m_macro_history_head = (m_macro_history_head + 1) % (m_reference_count + 1);

  return 0;
}

int32_t AMAudioAlert::trigger_event(int32_t micro_data)
{
  int32_t ret = 0;
  micro_data = micro_data * 100 / m_maximum_peak;

  if (m_micro_history == NULL) {
    ERROR("m_micro_history is NULL\n");
    return -1;
  }

  m_micro_history[m_micro_history_head] = micro_data;
  if (m_micro_history_head == m_micro_history_tail) {
    m_micro_history_tail = (m_micro_history_tail + 1) % (m_reference_count + 1);
  }

  m_micro_history_head = (m_micro_history_head + 1) % (m_reference_count + 1);
  if (m_alert_direction) {
    if (micro_data < m_alert_threshold
        || (m_micro_history[m_micro_history_tail] >= m_alert_threshold)) {
      ret = 1;
    } else {
      for (int32_t i = 1; i < m_reference_count; ++ i) {
        if (m_micro_history[(m_micro_history_tail + i) % (m_reference_count + 1)]
            < m_alert_threshold) {
          ret = 1;
          break;
        }
      }
    }
  } else {
    if (micro_data >= m_alert_threshold
        || (m_micro_history[m_micro_history_tail] < m_alert_threshold)) {
      ret = 1;
    } else {
      for (int32_t i = 1; i < m_reference_count; ++ i) {
        if (m_micro_history[(m_micro_history_tail + i) % (m_reference_count + 1)]
            >= m_alert_threshold) {
          ret = 1;
          break;
        }
      }
    }
  }

  if (ret == 0) {
    NOTICE("micro_data = %d", micro_data);
    for (int32_t i = 0; i < m_reference_count + 1; ++ i) {
      DEBUG("micro_data[%d] = %d",
            i,
            m_micro_history[(m_micro_history_tail + i) % (m_reference_count + 1)]);
    }
  }

  return ret;
}

int32_t AMAudioAlert::audio_alert_detect(AudioPacket *pkg)
{
  int32_t val = 0;
  int32_t interval_counter = 0;
  int32_t count = pkg->length;
  int32_t average_peak = 0;
  int32_t sum = 0;
  int32_t ret = 0;
  int32_t format = snd_pcm_format_little_endian(m_snd_pcm_format);
  AM_EVENT_MESSAGE msg;

  AUTO_LOCK(audio_alert_mtx);

  if (!m_enable_alert_detect)
    return 0;

  count = pkg->length;
  memset(&msg, 0, sizeof(AM_EVENT_MESSAGE));
  switch (m_bits_per_sample) {
    case 8: {
      int8_t *ptr = (int8_t*) pkg->data;
      int8_t mask = snd_pcm_format_silence(m_snd_pcm_format);

      /* Computer average volume in current pcm data. */
      interval_counter = 0;
      while (-- count > 0) {
        val = abs(*ptr ++ ^ mask);
        sum += val;
        m_volume_sum += val;
        m_sample_num ++;
        interval_counter ++;
        if (interval_counter % MICRO_AVERAGE_SAMPLES == 0) {
          average_peak = sum / interval_counter;
          ret = trigger_event(average_peak);
          if (ret == 0) {
            msg.event_type = m_plugin_id;
            msg.seq_num = m_seq_num ++;
            msg.pts = pkg->pts;
            if (m_callback && m_callback(&msg) < 0) {
              ERROR("m_callback occur error!\n");
              return -1;
            }
          } else if (ret < 0) {
            ERROR("trigger_event failed !\n");
            return -1;
          }

          interval_counter = 0;
          sum = 0;
        }

        if (m_sample_num == MACRO_AVERAGE_SAMPLES) {
          if (store_macro_data(m_volume_sum) < 0) {
            ERROR("store_macro_data failed!\n");
            return -1;
          }
          m_volume_sum = 0;
          m_sample_num = 0;
        }
      }
    }
      break;

    case 16: {
      int16_t *ptr = (int16_t*) pkg->data;
      int16_t mask = snd_pcm_format_silence_16(m_snd_pcm_format);
      DEBUG("case 16: m_snd_pcm_format = %d\n", m_snd_pcm_format);

      count >>= 1; /* One sample takes up two bytes */
      while (count -- > 0) {
        val = format ? __le16_to_cpu(*ptr ++) : __be16_to_cpu(*ptr ++);

        if (val & (1 << (m_bits_per_sample - 1))) {
          val |= 0xffff << 16;
        }

        val = abs(val) ^ mask;
        sum += val;
        m_volume_sum += val;
        m_sample_num ++;
        interval_counter ++;
        if (interval_counter % MICRO_AVERAGE_SAMPLES == 0) {
          average_peak = sum / interval_counter;
          ret = trigger_event(average_peak);
          if (ret == 0) {
            msg.event_type = m_plugin_id;
            msg.seq_num = m_seq_num ++;
            msg.pts = pkg->pts;
            if (m_callback && m_callback(&msg) < 0) {
              ERROR("m_callback occur error!\n");
              return -1;
            }
          } else if (ret < 0) {
            ERROR("trigger_event failed !\n");
            return -1;
          }

          interval_counter = 0;
          sum = 0;
        }

        if (m_sample_num == MACRO_AVERAGE_SAMPLES) {
          if (store_macro_data(m_volume_sum) < 0) {
            ERROR("store_macro_data failed!\n");
            return -1;
          }
          m_volume_sum = 0;
          m_sample_num = 0;
        }
      }
    }
      break;

    case 24: {
      int8_t *ptr = (int8_t*) pkg->data;
      int32_t mask = snd_pcm_format_silence_32(m_snd_pcm_format);

      count /= 3; /* One sample takes up three bytes. */
      while (count -- > 0) {
        val =
            format ? (ptr[0] | (ptr[1] << 8) | (ptr[2] << 16)) :
                     (ptr[2] | (ptr[1] << 8) | (ptr[0] << 16));

        /* Correct signed bit in 32-bit value */
        if (val & (1 << (m_bits_per_sample - 1))) {
          val |= 0xff << 24;
        }

        val = abs(val) ^ mask;
        sum += val;
        m_volume_sum += val;
        m_sample_num ++;
        interval_counter ++;
        if (interval_counter % MICRO_AVERAGE_SAMPLES == 0) {
          average_peak = sum / interval_counter;
          ret = trigger_event(average_peak);
          if (ret == 0) {
            msg.event_type = m_plugin_id;
            msg.seq_num = m_seq_num ++;
            msg.pts = pkg->pts;
            if (m_callback && m_callback(&msg) < 0) {
              ERROR("m_callback occur error!\n");
              return -1;
            }
          } else if (ret < 0) {
            ERROR("trigger_event failed !\n");
            return -1;
          }

          interval_counter = 0;
          sum = 0;
        }

        if (m_sample_num == MACRO_AVERAGE_SAMPLES) {
          if (store_macro_data(m_volume_sum) < 0) {
            ERROR("store_macro_data failed!\n");
            return -1;
          }
          m_volume_sum = 0;
          m_sample_num = 0;
        }

        ptr += 3;
      }
    }
      break;

    case 32: {
      int32_t *ptr = (int32_t*) pkg->data;
      int32_t mask = snd_pcm_format_silence_32(m_snd_pcm_format);

      count >>= 2; /* One sample takes up four bytes. */
      while (count -- > 0) {
        val = abs(format ? __le32_to_cpu(*ptr ++) : __be32_to_cpu(*ptr ++))
            ^ mask;
        sum += val;
        m_volume_sum += val;
        m_sample_num ++;
        interval_counter ++;
        if (interval_counter % MICRO_AVERAGE_SAMPLES == 0) {
          average_peak = sum / interval_counter;
          ret = trigger_event(average_peak);
          if (ret == 0) {
            msg.event_type = m_plugin_id;
            msg.seq_num = m_seq_num ++;
            msg.pts = pkg->pts;
            if (m_callback && m_callback(&msg) < 0) {
              ERROR("m_callback occur error!\n");
              return -1;
            }
          } else if (ret < 0) {
            ERROR("trigger_event failed !\n");
            return -1;
          }

          interval_counter = 0;
          sum = 0;
        }

        if (m_sample_num == MACRO_AVERAGE_SAMPLES) {
          if (store_macro_data(m_volume_sum) < 0) {
            ERROR("store_macro_data failed!\n");
            return -1;
          }
          m_volume_sum = 0;
          m_sample_num = 0;
        }
      }
    }
      break;

    default:
      ERROR("No such sample");
      break;
  }

  return 0;
}

bool AMAudioAlert::set_audio_format(AM_AUDIO_SAMPLE_FORMAT format)
{
  bool ret = true;
  if (AM_LIKELY(m_audio_capture)) {
    switch (format) {
      case AM_SAMPLE_U8: {
        m_snd_pcm_format = SND_PCM_FORMAT_U8;
      }
        break;
      case AM_SAMPLE_ALAW: {
        m_snd_pcm_format = SND_PCM_FORMAT_A_LAW;
      }
        break;
      case AM_SAMPLE_ULAW: {
        m_snd_pcm_format = SND_PCM_FORMAT_MU_LAW;
      }
        break;
      case AM_SAMPLE_S16LE: {
        m_snd_pcm_format = SND_PCM_FORMAT_S16_LE;
      }
        break;
      case AM_SAMPLE_S16BE: {
        m_snd_pcm_format = SND_PCM_FORMAT_S16_BE;
      }
        break;
      case AM_SAMPLE_S32LE: {
        m_snd_pcm_format = SND_PCM_FORMAT_S32_LE;
      }
        break;
      case AM_SAMPLE_S32BE: {
        m_snd_pcm_format = SND_PCM_FORMAT_S32_BE;
      }
        break;
      case AM_SAMPLE_INVALID:
      default:
        ERROR("AMAudioAlert::unknown audio sample format!\n");
        ret = false;
        break;
    }
    do {
      if (!ret) {
        break;
      }
      if (!m_audio_capture->set_sample_format(format)) {
        ERROR("AMAudioAlert::m_audio_capture->set_sample_format failed!\n");
        ret = false;
        break;
      }
      m_bits_per_sample = snd_pcm_format_physical_width(m_snd_pcm_format);
      if (AM_UNLIKELY(m_bits_per_sample < 0)) {
        ERROR("m_bits_per_sample is less than 0! \n");
        ret = false;
        break;
      }
      m_maximum_peak = (1 << (m_bits_per_sample - 1)) - 1;
      m_audio_format = format;
    } while (0);
  }

  return ret;
}

void AMAudioAlert::static_audio_alert_detect(AudioCapture *p)
{
  ((AMAudioAlert*) p->owner)->audio_alert_detect(&p->packet);
}
