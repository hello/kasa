/*******************************************************************************
 * am_video_service_msg_action.cpp
 *
 * History:
 *   2014-9-16 - [lysun] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co,Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#include "am_base_include.h"
#include "am_define.h"
#include "am_log.h"

#include "am_api_video.h"
#include "am_video_dsp.h"
#include "am_video_param.h"
#include "am_encode_device.h"

//Implementation Interface
#include "am_vin_config_if.h"
#include "am_video_camera_if.h"
#include "am_encode_device_if.h"
#include "am_service_frame_if.h"
#include "am_encode_device_config_if.h"

#include <signal.h>

extern AMIVideoCameraPtr g_video_camera;
extern AMIServiceFrame *g_service_frame;
extern AM_SERVICE_STATE g_service_state;

void ON_SERVICE_INIT(void *msg_data,
                     int msg_data_size,
                     void *result_addr,
                     int result_max_size)
{
  am_service_result_t *service_result = (am_service_result_t*) result_addr;
  service_result->ret = 0;
  service_result->state = g_service_state;
  if (service_result->state == AM_SERVICE_STATE_INIT_DONE) {
    INFO("Video Service Init Done \n");
  }
}

void ON_SERVICE_DESTROY(void *msg_data,
                        int msg_data_size,
                        void *result_addr,
                        int result_max_size)
{
  INFO("video service destroy, cleanup\n");
  g_video_camera->stop();
  g_service_frame->quit();
}

void ON_SERVICE_START(void *msg_data,
                      int msg_data_size,
                      void *result_addr,
                      int result_max_size)
{
  int32_t ret = 0;
  if (!g_video_camera) {
    ERROR("Video Service: fail to get AMVideoCamera instance\n");
    ret = -1;
  } else {
    if (g_service_state == AM_SERVICE_STATE_STARTED) {
      ret = 0;
    } else {
      if (g_video_camera->start() != AM_RESULT_OK) {
        ERROR("Video Service: start failed\n");
        g_service_state = AM_SERVICE_STATE_ERROR;
        ret = -2;
      } else {
        g_service_state = AM_SERVICE_STATE_STARTED;
      }
    }
  }
  am_service_result_t *service_result = (am_service_result_t *) result_addr;
  service_result->ret = ret;
}

void ON_SERVICE_STOP(void *msg_data,
                     int msg_data_size,
                     void *result_addr,
                     int result_max_size)
{
  int32_t ret = 0;
  if (!g_video_camera) {
    ERROR("Video Service: fail to get AMVideoCamera instance\n");
    ret = -1;
  } else {
    if (g_video_camera->stop() != AM_RESULT_OK) {
      ERROR("Video Service: stop failed\n");
      ret = -2;
      g_service_state = AM_SERVICE_STATE_ERROR;
    } else {
      g_service_state = AM_SERVICE_STATE_STOPPED;
    }
  }
  am_service_result_t *service_result = (am_service_result_t *) result_addr;
  service_result->ret = ret;
}

void ON_SERVICE_RESTART(void *msg_data,
                        int msg_data_size,
                        void *result_addr,
                        int result_max_size)
{
  INFO("video service restart\n");
}
void ON_SERVICE_STATUS(void *msg_data,
                       int msg_data_size,
                       void *result_addr,
                       int result_max_size)
{
  INFO("video service get status\n");
}

void ON_VIN_GET(void *msg_data,
                int msg_data_size,
                void *result_addr,
                int result_max_size)
{
  INFO("video service VIN GET\n");
}

void ON_VIN_SET(void *msg_data,
                int msg_data_size,
                void *result_addr,
                int result_max_size)
{
  INFO("video service VIN SET\n");
  int32_t ret = 0;
  am_service_result_t *service_result = (am_service_result_t*) result_addr;
  memset(service_result, 0, sizeof(am_service_result_t));

  if (!msg_data) {
    service_result->ret = -1;
    return;
  }

  am_vin_config_t *config = (am_vin_config_t*) msg_data;
  AMIVinConfig *vin_config =
      g_video_camera->get_encode_device()->get_vin_config();

  if (TEST_BIT(config->enable_bits, AM_VIN_CONFIG_WIDTH_HEIGHT_EN_BIT)) {

  }

  if (TEST_BIT(config->enable_bits, AM_VIN_CONFIG_FLIP_EN_BIT)) {
    AM_VIDEO_FLIP flip = AM_VIDEO_FLIP(config->flip);
    vin_config->set_flip_config(AM_VIN_ID(config->vin_id), flip);
  }

  if (TEST_BIT(config->enable_bits, AM_VIN_CONFIG_FPS_EN_BIT)) {
    AM_VIDEO_FPS fps = AM_VIDEO_FPS(config->fps);
    vin_config->set_fps_config(AM_VIN_ID(config->vin_id), fps);
  }

  if (TEST_BIT(config->enable_bits, AM_VIN_CONFIG_BAYER_PATTERN_EN_BIT)) {
    AM_VIN_BAYER_PATTERN bayer = AM_VIN_BAYER_PATTERN(config->bayer_pattern);
    vin_config->set_bayer_config(AM_VIN_ID(config->vin_id), bayer);
  }
  ret = vin_config->save_config();

  service_result->ret = ret;
}

void ON_STREAM_FMT_GET(void *msg_data,
                       int msg_data_size,
                       void *result_addr,
                       int result_max_size)
{
  INFO("video service STREAM FMT GET\n");
  int32_t ret = 0;
  am_service_result_t *service_result = (am_service_result_t*) result_addr;
  memset(service_result, 0, sizeof(am_service_result_t));

  if (!msg_data) {
    service_result->ret = -1;
    return;
  }

  uint32_t stream_id = *((uint32_t*) msg_data);
  am_stream_fmt_t *fmt = (am_stream_fmt_t*) service_result->data;

  do {
    AMIEncodeDevice *dev = g_video_camera->get_encode_device();
    if (!dev) {
      ret = -1;
      ERROR("Can't get encode device!");
      break;
    }
    AMIEncodeDeviceConfig *dev_config = dev->get_encode_config();
    if (!dev_config) {
      ret = -1;
      ERROR("Can't get encode device config!");
      break;
    }
    if (dev_config->load_config() != AM_RESULT_OK) {
      ret = -1;
      ERROR("Can't load config!");
      break;
    }
    AMEncodeParamAll *param = dev_config->get_encode_param();
    if (!param) {
      ret = -1;
      ERROR("Can't get encode param!");
      break;
    }
    AMEncodeStreamFormat *stream_fmt =
        &param->stream_format.encode_format[stream_id];

    fmt->enable = stream_fmt->enable;
    switch (stream_fmt->type) {
      case AM_ENCODE_STREAM_TYPE_NONE:
        fmt->type = 0;
        break;
      case AM_ENCODE_STREAM_TYPE_H264:
        fmt->type = 1;
        break;
      case AM_ENCODE_STREAM_TYPE_MJPEG:
        fmt->type = 2;
        break;
      default:
        ERROR("Wrong type format: %d!", stream_fmt->type);
        break;
    }
    fmt->source = stream_fmt->source;
    fmt->frame_fact_num = stream_fmt->fps.fps_multi;
    fmt->frame_fact_den = stream_fmt->fps.fps_div;
    fmt->width = stream_fmt->width;
    fmt->height = stream_fmt->height;
    fmt->offset_x = stream_fmt->offset_x;
    fmt->offset_y = stream_fmt->offset_y;
    fmt->hflip = stream_fmt->hflip;
    fmt->vflip = stream_fmt->vflip;
    fmt->rotate = stream_fmt->rotate_90_ccw;
  } while (0);
  service_result->ret = ret;
}

void ON_STREAM_FMT_SET(void *msg_data,
                       int msg_data_size,
                       void *result_addr,
                       int result_max_size)
{
  INFO("video service STREAM FMT SET\n");
  int32_t ret = 0;
  am_service_result_t *service_result = (am_service_result_t*) result_addr;

  if (!msg_data) {
    service_result->ret = -1;
    return;
  }

  am_stream_fmt_t *fmt = (am_stream_fmt_t*) msg_data;
  uint32_t stream_id = fmt->stream_id;

  do {
    AMIEncodeDevice *dev = g_video_camera->get_encode_device();
    if (!dev) {
      ret = -1;
      ERROR("Can't get encode device!");
      break;
    }
    AMIEncodeDeviceConfig *dev_config = dev->get_encode_config();
    if (!dev_config) {
      ret = -1;
      ERROR("Can't get encode device config!");
      break;
    }
    if (dev_config->load_config() != AM_RESULT_OK) {
      ret = -1;
      ERROR("Can't load config!");
      break;
    }
    AMEncodeParamAll *param = dev_config->get_encode_param();
    if (!param) {
      ret = -1;
      ERROR("Can't get encode param!");
      break;
    }
    AMEncodeStreamFormat *stream_fmt =
        &param->stream_format.encode_format[stream_id];

    if (TEST_BIT(fmt->enable_bits, AM_STREAM_FMT_ENABLE_EN_BIT)) {
      stream_fmt->enable = fmt->enable;
    }

    if (TEST_BIT(fmt->enable_bits, AM_STREAM_FMT_TYPE_EN_BIT)) {
      INFO("type");
      stream_fmt->type = AM_ENCODE_STREAM_TYPE(fmt->type);
    }

    if (TEST_BIT(fmt->enable_bits, AM_STREAM_FMT_SOURCE_EN_BIT)) {
      stream_fmt->source = AM_ENCODE_SOURCE_BUFFER_ID(fmt->source);
    }

    if (TEST_BIT(fmt->enable_bits, AM_STREAM_FMT_FRAME_NUM_EN_BIT)) {
      INFO("fact num");
      stream_fmt->fps.fps_multi = fmt->frame_fact_num;
    }

    if (TEST_BIT(fmt->enable_bits, AM_STREAM_FMT_FRAME_DEN_EN_BIT)) {
      INFO("fact den");
      stream_fmt->fps.fps_div = fmt->frame_fact_den;
    }

    if (TEST_BIT(fmt->enable_bits, AM_STREAM_FMT_WIDTH_EN_BIT)) {
      stream_fmt->width = fmt->width;
    }

    if (TEST_BIT(fmt->enable_bits, AM_STREAM_FMT_HEIGHT_EN_BIT)) {
      stream_fmt->height = fmt->height;
    }

    if (TEST_BIT(fmt->enable_bits, AM_STREAM_FMT_OFFSET_X_EN_BIT)) {
      stream_fmt->offset_x = fmt->offset_x;
    }

    if (TEST_BIT(fmt->enable_bits, AM_STREAM_FMT_OFFSET_Y_EN_BIT)) {
      stream_fmt->offset_y = fmt->offset_y;
    }

    if (TEST_BIT(fmt->enable_bits, AM_STREAM_FMT_HFLIP_EN_BIT)) {
      stream_fmt->hflip = !!fmt->hflip;
    }

    if (TEST_BIT(fmt->enable_bits, AM_STREAM_FMT_VFLIP_EN_BIT)) {
      stream_fmt->vflip = !!fmt->vflip;
    }

    if (TEST_BIT(fmt->enable_bits, AM_STREAM_FMT_ROTATE_EN_BIT)) {
      stream_fmt->rotate_90_ccw = !!fmt->rotate;
    }

    if (dev_config->sync() != AM_RESULT_OK) {
      ret = -1;
      ERROR("Can't sync device config!");
      break;
    }
    if (dev_config->save_config() != AM_RESULT_OK) {
      ret = -1;
      ERROR("Can't save device config!");
      break;
    }
  } while (0);
  service_result->ret = ret;
}

void ON_STREAM_CFG_GET(void *msg_data,
                       int msg_data_size,
                       void *result_addr,
                       int result_max_size)
{
  INFO("video service STREAM CFG GET\n");
  int32_t ret = 0;
  am_service_result_t *service_result = (am_service_result_t*) result_addr;
  memset(service_result, 0, sizeof(am_service_result_t));
  if (!msg_data) {
    service_result->ret = -1;
    return;
  }
  am_stream_cfg_t *cfg = (am_stream_cfg_t*) service_result->data;
  uint32_t stream_id = *((uint32_t*) msg_data);

  do {
    AMIEncodeDevice *dev = g_video_camera->get_encode_device();
    if (!dev) {
      ret = -1;
      ERROR("Can't get encode device!");
      break;
    }
    AMIEncodeDeviceConfig *dev_config = dev->get_encode_config();
    if (!dev_config) {
      ret = -1;
      ERROR("Can't get encode device config!");
      break;
    }
    if (dev_config->load_config() != AM_RESULT_OK) {
      ret = -1;
      ERROR("Can't load config!");
      break;
    }
    AMEncodeParamAll *param = dev_config->get_encode_param();
    if (!param) {
      ret = -1;
      ERROR("Can't get encode param!");
      break;
    }
    AMEncodeStreamConfig *stream_cfg =
        &param->stream_config.encode_config[stream_id];

    cfg->h264.bitrate_ctrl = stream_cfg->h264_config.bitrate_control;
    cfg->h264.profile = stream_cfg->h264_config.profile_level;
    cfg->h264.au_type = stream_cfg->h264_config.au_type;
    cfg->h264.chroma = stream_cfg->h264_config.chroma_format;
    cfg->h264.M = stream_cfg->h264_config.M;
    cfg->h264.N = stream_cfg->h264_config.N;
    cfg->h264.idr_interval = stream_cfg->h264_config.idr_interval;
    cfg->h264.target_bitrate = stream_cfg->h264_config.target_bitrate;
    cfg->h264.mv_threshold = stream_cfg->h264_config.mv_threshold;
    cfg->h264.encode_improve = stream_cfg->h264_config.encode_improve;
    cfg->h264.multi_ref_p = stream_cfg->h264_config.multi_ref_p;
    cfg->h264.long_term_intvl = stream_cfg->h264_config.long_term_intvl;

    cfg->mjpeg.quality = stream_cfg->mjpeg_config.quality;
    cfg->mjpeg.chroma = stream_cfg->mjpeg_config.chroma_format;
  } while (0);
  service_result->ret = ret;
}
void ON_STREAM_CFG_SET(void *msg_data,
                       int msg_data_size,
                       void *result_addr,
                       int result_max_size)
{
  INFO("video service STREAM CFG SET\n");
  int32_t ret = 0;
  am_service_result_t *service_result = (am_service_result_t*) result_addr;
  memset(service_result, 0, sizeof(am_service_result_t));

  if (!msg_data) {
    service_result->ret = -1;
    return;
  }
  am_stream_cfg_t *cfg = (am_stream_cfg_t*) msg_data;
  uint32_t stream_id = cfg->stream_id;

  do {
    AMIEncodeDevice *dev = g_video_camera->get_encode_device();
    if (!dev) {
      ret = -1;
      ERROR("Can't get encode device!");
      break;
    }
    AMIEncodeDeviceConfig *dev_config = dev->get_encode_config();
    if (!dev_config) {
      ret = -1;
      ERROR("Can't get encode device config!");
      break;
    }
    if (dev_config->load_config() != AM_RESULT_OK) {
      ret = -1;
      ERROR("Can't load config!");
      break;
    }
    AMEncodeParamAll *param = dev_config->get_encode_param();
    if (!param) {
      ret = -1;
      ERROR("Can't get encode param!");
      break;
    }
    AMEncodeStreamConfig *stream_cfg =
        &param->stream_config.encode_config[stream_id];

    if (TEST_BIT(cfg->enable_bits, AM_STREAM_CFG_H264_EN_BIT)) {
      if (TEST_BIT(cfg->h264.enable_bits, AM_H264_CFG_BITRATE_CTRL_EN_BIT)) {
        stream_cfg->h264_config.bitrate_control =
            AM_ENCODE_H264_RATE_CONTROL(cfg->h264.bitrate_ctrl);
      }

      if (TEST_BIT(cfg->h264.enable_bits, AM_H264_CFG_PROFILE_EN_BIT)) {
        stream_cfg->h264_config.profile_level =
            AM_ENCODE_H264_PROFILE(cfg->h264.profile);
      }

      if (TEST_BIT(cfg->h264.enable_bits, AM_H264_CFG_AU_TYPE_EN_BIT)) {
        stream_cfg->h264_config.au_type =
            AM_ENCODE_H264_AU_TYPE(cfg->h264.au_type);
      }

      if (TEST_BIT(cfg->h264.enable_bits, AM_H264_CFG_CHROMA_EN_BIT)) {
        stream_cfg->h264_config.chroma_format =
            AM_ENCODE_CHROMA_FORMAT(cfg->h264.chroma);
      }

      if (TEST_BIT(cfg->h264.enable_bits, AM_H264_CFG_M_EN_BIT)) {
        stream_cfg->h264_config.M = cfg->h264.M;
      }

      if (TEST_BIT(cfg->h264.enable_bits, AM_H264_CFG_N_EN_BIT)) {
        stream_cfg->h264_config.N = cfg->h264.N;
      }

      if (TEST_BIT(cfg->h264.enable_bits, AM_H264_CFG_IDR_EN_BIT)) {
        stream_cfg->h264_config.idr_interval = cfg->h264.idr_interval;
      }

      if (TEST_BIT(cfg->h264.enable_bits, AM_H264_CFG_BITRATE_EN_BIT)) {
        stream_cfg->h264_config.target_bitrate = cfg->h264.target_bitrate;
      }

      if (TEST_BIT(cfg->h264.enable_bits, AM_H264_CFG_MV_THRESHOLD_EN_BIT)) {
        stream_cfg->h264_config.mv_threshold = cfg->h264.mv_threshold;
      }

      if (TEST_BIT(cfg->h264.enable_bits, AM_H264_CFG_ENC_IMPROVE_EN_BIT)) {
        stream_cfg->h264_config.encode_improve = cfg->h264.encode_improve;
      }

      if (TEST_BIT(cfg->h264.enable_bits, AM_H264_CFG_MULTI_REF_P_EN_BIT)) {
        stream_cfg->h264_config.multi_ref_p = cfg->h264.multi_ref_p;
      }

      if (TEST_BIT(cfg->h264.enable_bits, AM_H264_CFG_LONG_TERM_INTVL_EN_BIT)) {
        stream_cfg->h264_config.long_term_intvl = cfg->h264.long_term_intvl;
      }
    }

    if (TEST_BIT(cfg->enable_bits, AM_STREAM_CFG_MJPEG_EN_BIT)) {
      if (TEST_BIT(cfg->mjpeg.enable_bits, AM_MJPEG_CFG_QUALITY_EN_BIT)) {
        stream_cfg->mjpeg_config.quality = cfg->mjpeg.quality;
      }

      if (TEST_BIT(cfg->mjpeg.enable_bits, AM_MJPEG_CFG_CHROMA_EN_BIT)) {
        stream_cfg->mjpeg_config.chroma_format =
            AM_ENCODE_CHROMA_FORMAT(cfg->mjpeg.chroma);
      }
    }

    if (dev_config->sync() != AM_RESULT_OK) {
      ret = -1;
      ERROR("Can't sync device config!");
      break;
    }
    if (dev_config->save_config() != AM_RESULT_OK) {
      ret = -1;
      ERROR("Can't save device config!");
      break;
    }
  } while (0);
  service_result->ret = ret;
}

void ON_BUFFER_FMT_GET(void *msg_data,
                       int msg_data_size,
                       void *result_addr,
                       int result_max_size)
{
  INFO("video service ON_BUFFER_FMT_GET\n");
  int32_t ret = 0;
  am_service_result_t *service_result = (am_service_result_t*) result_addr;
  memset(service_result, 0, sizeof(am_service_result_t));
  if (!msg_data) {
    service_result->ret = -1;
    return;
  }

  am_buffer_fmt_t *fmt = (am_buffer_fmt_t*) service_result->data;
  uint32_t buffer_id = *((uint32_t*) msg_data);

  do {
    AMIEncodeDevice *dev = g_video_camera->get_encode_device();
    if (!dev) {
      ret = -1;
      ERROR("Can't get encode device!");
      break;
    }
    AMIEncodeDeviceConfig *dev_config = dev->get_encode_config();
    if (!dev_config) {
      ret = -1;
      ERROR("Can't get encode device config!");
      break;
    }
    if (dev_config->load_config() != AM_RESULT_OK) {
      ret = -1;
      ERROR("Can't load config!");
      break;
    }
    AMEncodeParamAll *param = dev_config->get_encode_param();
    if (!param) {
      ret = -1;
      ERROR("Can't get encode param!");
      break;
    }
    AMEncodeSourceBufferFormat *buffer_fmt =
        &param->buffer_format.source_buffer_format[buffer_id];

    fmt->buffer_id = buffer_id;
    fmt->type = buffer_fmt->type;
    fmt->input_crop = buffer_fmt->input_crop;
    fmt->input_width = buffer_fmt->input.width;
    fmt->input_height = buffer_fmt->input.height;
    fmt->input_offset_x = buffer_fmt->input.x;
    fmt->input_offset_y = buffer_fmt->input.y;
    fmt->width = buffer_fmt->size.width;
    fmt->height = buffer_fmt->size.height;
    fmt->prewarp = buffer_fmt->prewarp;
  } while (0);
  service_result->ret = ret;
}

void ON_BUFFER_FMT_SET(void *msg_data,
                       int msg_data_size,
                       void *result_addr,
                       int result_max_size)
{
  INFO("video service ON_BUFFER_FMT_SET\n");
  int32_t ret = 0;
  am_service_result_t *service_result = (am_service_result_t*) result_addr;
  memset(service_result, 0, sizeof(am_service_result_t));
  if (!msg_data) {
    service_result->ret = -1;
    return;
  }

  am_buffer_fmt_t *fmt = (am_buffer_fmt_t*) msg_data;
  uint32_t buffer_id = fmt->buffer_id;
  do {
    AMIEncodeDevice *dev = g_video_camera->get_encode_device();
    if (!dev) {
      ret = -1;
      ERROR("Can't get encode device!");
      break;
    }
    AMIEncodeDeviceConfig *dev_config = dev->get_encode_config();
    if (!dev_config) {
      ret = -1;
      ERROR("Can't get encode device config!");
      break;
    }
    if (dev_config->load_config() != AM_RESULT_OK) {
      ret = -1;
      ERROR("Can't load config!");
      break;
    }
    AMEncodeParamAll *param = dev_config->get_encode_param();
    if (!param) {
      ret = -1;
      ERROR("Can't get encode param!");
      break;
    }
    AMEncodeSourceBufferFormat *buffer_fmt =
        &param->buffer_format.source_buffer_format[buffer_id];

    if (TEST_BIT(fmt->enable_bits, AM_BUFFER_FMT_TYPE_EN_BIT)) {
      buffer_fmt->type = AM_ENCODE_SOURCE_BUFFER_TYPE(fmt->type);
    }
    if (TEST_BIT(fmt->enable_bits, AM_BUFFER_FMT_INPUT_CROP_EN_BIT)) {
      buffer_fmt->input_crop = fmt->input_crop;
    }
    if (TEST_BIT(fmt->enable_bits, AM_BUFFER_FMT_INPUT_WIDTH_EN_BIT)) {
      buffer_fmt->input.width = fmt->input_width;
    }
    if (TEST_BIT(fmt->enable_bits, AM_BUFFER_FMT_INPUT_HEIGHT_EN_BIT)) {
      buffer_fmt->input.height = fmt->input_height;
    }
    if (TEST_BIT(fmt->enable_bits, AM_BUFFER_FMT_INPUT_X_EN_BIT)) {
      buffer_fmt->input.x = fmt->input_offset_x;
    }
    if (TEST_BIT(fmt->enable_bits, AM_BUFFER_FMT_INPUT_Y_EN_BIT)) {
      buffer_fmt->input.y = fmt->input_offset_y;
    }
    if (TEST_BIT(fmt->enable_bits, AM_BUFFER_FMT_WIDTH_EN_BIT)) {
      buffer_fmt->size.width = fmt->width;
    }
    if (TEST_BIT(fmt->enable_bits, AM_BUFFER_FMT_HEIGHT_EN_BIT)) {
      buffer_fmt->size.height = fmt->height;
    }
    if (TEST_BIT(fmt->enable_bits, AM_BUFFER_FMT_PREWARP_EN_BIT)) {
      buffer_fmt->prewarp = fmt->prewarp;
    }

    if (dev_config->sync() != AM_RESULT_OK) {
      ret = -1;
      ERROR("Can't sync device config!");
      break;
    }
    if (dev_config->save_config() != AM_RESULT_OK) {
      ret = -1;
      ERROR("Can't save device config!");
      break;
    }
  } while (0);
  service_result->ret = ret;
}

void ON_BUFFER_ALLOC_STYLE_GET(void *msg_data,
                               int msg_data_size,
                               void *result_addr,
                               int result_max_size)
{
  INFO("video service ON_BUFFER_ALLOC_STYLE_GET\n");
  int32_t ret = 0;
  am_service_result_t *service_result = (am_service_result_t*) result_addr;
  memset(service_result, 0, sizeof(am_service_result_t));
  if (!msg_data) {
    service_result->ret = -1;
    return;
  }

  am_buffer_alloc_style_t *style =
      (am_buffer_alloc_style_t*) service_result->data;
  do {
    AMIEncodeDevice *dev = g_video_camera->get_encode_device();
    if (!dev) {
      ret = -1;
      ERROR("Can't get encode device!");
      break;
    }
    AMIEncodeDeviceConfig *dev_config = dev->get_encode_config();
    if (!dev_config) {
      ret = -1;
      ERROR("Can't get encode device config!");
      break;
    }
    if (dev_config->load_config() != AM_RESULT_OK) {
      ret = -1;
      ERROR("Can't load config!");
      break;
    }
    AM_ENCODE_SOURCE_BUFFER_ALLOCATE_STYLE alloc_style =
        dev_config->get_buffer_alloc_style();
    style->alloc_style = alloc_style;
  } while (0);
  service_result->ret = ret;
}

void ON_BUFFER_ALLOC_STYLE_SET(void *msg_data,
                               int msg_data_size,
                               void *result_addr,
                               int result_max_size)
{
  INFO("video service ON_BUFFER_ALLOC_STYLE_SET\n");
  int32_t ret = 0;
  am_service_result_t *service_result = (am_service_result_t*) result_addr;
  memset(service_result, 0, sizeof(am_service_result_t));
  if (!msg_data) {
    service_result->ret = -1;
    return;
  }
  am_buffer_alloc_style_t *style = (am_buffer_alloc_style_t*) msg_data;
  do {
    AMIEncodeDevice *dev = g_video_camera->get_encode_device();
    if (!dev) {
      ret = -1;
      ERROR("Can't get encode device!");
      break;
    }
    AMIEncodeDeviceConfig *dev_config = dev->get_encode_config();
    if (!dev_config) {
      ret = -1;
      ERROR("Can't get encode device config!");
      break;
    }
    if (dev_config->load_config() != AM_RESULT_OK) {
      ret = -1;
      ERROR("Can't load config!");
      break;
    }
    if (dev_config->set_buffer_alloc_style(AM_ENCODE_SOURCE_BUFFER_ALLOCATE_STYLE(style->alloc_style))
        != AM_RESULT_OK) {
      ret = -1;
      ERROR("Can't set buffer alloctate style!");
      break;
    }
    if (dev_config->sync() != AM_RESULT_OK) {
      ret = -1;
      ERROR("Can't sync device config!");
      break;
    }
    if (dev_config->save_config() != AM_RESULT_OK) {
      ret = -1;
      ERROR("Can't save device config!");
      break;
    }
  } while (0);
  service_result->ret = ret;
}

void ON_STREAM_STATUS_GET(void *msg_data,
                          int msg_data_size,
                          void *result_addr,
                          int result_max_size)
{
  INFO("video service STREAM STATUS GET\n");
  int32_t ret = 0;
  am_service_result_t *service_result = (am_service_result_t*) result_addr;
  memset(service_result, 0, sizeof(am_service_result_t));
  if (!msg_data) {
    service_result->ret = -1;
    return;
  }
  uint32_t stream_id;
  am_stream_status_t *status = (am_stream_status_t*) service_result->data;
  do {
    AMIEncodeDevice *dev = g_video_camera->get_encode_device();
    if (!dev) {
      ERROR("Can't get encode device!");
      ret = -1;
      break;
    }
    AMEncodeStream *stream = dev->get_encode_stream_list();
    if (!stream) {
      ERROR("Can't get encode stream list!");
      ret = -1;
      break;
    }

    for (uint32_t i = 0; i < AM_STREAM_MAX_NUM; ++ i) {
      stream_id = i;
      if (stream + stream_id) {
        if ((stream + stream_id)->is_encoding()) {
          status->status |= 1 << stream_id;
        }
      } else {
        break;
      }
    }
  } while (0);

  service_result->ret = ret;
}

void ON_DPTZ_WARP_SET(void *msg_data,
                      int msg_data_size,
                      void *result_addr,
                      int result_max_size)
{
  INFO("video service ON_DPTZ_WARP_SET!\n");
  am_service_result_t *service_result = (am_service_result_t*) result_addr;
  memset(service_result, 0, sizeof(am_service_result_t));
  if (!msg_data) {
    ERROR("NULL pointer!\n");
    service_result->ret = -1;
    return;
  }
  am_dptz_warp_t *dptz_warp_param = (am_dptz_warp_t*) msg_data;
  uint32_t buffer_id = dptz_warp_param->buf_id;
  do {
    AMIEncodeDevice *en_dev = g_video_camera->get_encode_device();
    if (!en_dev) {
      ERROR("get encode device failed\n");
      service_result->ret = -1;
      break;
    }
    AMDPTZWarpConfig dptz_warp_config;
    dptz_warp_config.buf_cfg = buffer_id;
    if (en_dev->get_dptz_warp_config(&dptz_warp_config) != AM_RESULT_OK) {
      ERROR("get dptz warp config failed!\n");
      service_result->ret = -1;
      break;
    }

    if (TEST_BIT(dptz_warp_param->enable_bits,
                 AM_DPTZ_WARP_LDC_STRENGTH_EN_BIT)) {
      dptz_warp_config.warp_cfg.ldc_strength =
          dptz_warp_param->ldc_strenght;
    }

    if (TEST_BIT(dptz_warp_param->enable_bits,
                 AM_DPTZ_WARP_PANO_HFOV_DEGREE_EN_BIT)) {
      dptz_warp_config.warp_cfg.pano_hfov_degree =
          dptz_warp_param->pano_hfov_degree;
    }

    if (TEST_BIT(dptz_warp_param->enable_bits,
                 AM_DPTZ_WARP_PAN_RATIO_EN_BIT)) {
      dptz_warp_config.dptz_cfg[buffer_id].pan_ratio =
          dptz_warp_param->pan_ratio;
    }

    if (TEST_BIT(dptz_warp_param->enable_bits,
                 AM_DPTZ_WARP_TILT_RATIO_EN_BIT)) {
      dptz_warp_config.dptz_cfg[buffer_id].tilt_ratio =
          dptz_warp_param->tilt_ratio;
    }

    if (TEST_BIT(dptz_warp_param->enable_bits,
                 AM_DPTZ_WARP_ZOOM_RATIO_EN_BIT)) {
      dptz_warp_config.dptz_cfg[buffer_id].zoom_ratio =
          dptz_warp_param->zoom_ratio;
    }

    service_result->ret = en_dev->set_dptz_warp_config(&dptz_warp_config);
    if (service_result->ret != AM_RESULT_OK) {
      ERROR("set dptz warp config failed!\n");
      break;
    }
  } while (0);
}

void ON_DPTZ_WARP_GET(void *msg_data,
                      int msg_data_size,
                      void *result_addr,
                      int result_max_size)
{
  INFO("video service ON_DPTZ_WARP_GET!\n");
  am_service_result_t *service_result = (am_service_result_t*) result_addr;
  memset(service_result, 0, sizeof(am_service_result_t));
  am_dptz_warp_t *dptz_warp_param =
      (am_dptz_warp_t*) service_result->data;
  uint32_t buffer_id = *((uint32_t*) msg_data);

  do {
    AMIEncodeDevice *en_dev = g_video_camera->get_encode_device();
    if (!en_dev) {
      ERROR("get encode device failed\n");
      service_result->ret = -1;
      break;
    }

    AMDPTZWarpConfig dptz_warp_config;
    memset(&dptz_warp_config, 0, sizeof(dptz_warp_config));
    dptz_warp_config.buf_cfg = buffer_id;
    if (en_dev->get_dptz_warp_config(&dptz_warp_config) != AM_RESULT_OK) {
      ERROR("get dptz warp config failed!\n");
      service_result->ret = -1;
      break;
    }

    dptz_warp_param->ldc_strenght = dptz_warp_config.warp_cfg.ldc_strength;
    dptz_warp_param->pano_hfov_degree = dptz_warp_config.warp_cfg.pano_hfov_degree;
    dptz_warp_param->buf_id = buffer_id;
    dptz_warp_param->pan_ratio = dptz_warp_config.dptz_cfg[buffer_id].pan_ratio;
    dptz_warp_param->tilt_ratio = dptz_warp_config.dptz_cfg[buffer_id].tilt_ratio;
    dptz_warp_param->zoom_ratio = dptz_warp_config.dptz_cfg[buffer_id].zoom_ratio;

  } while (0);
}

void ON_ENCODE_H264_LBR_SET(void *msg_data,
                            int msg_data_size,
                            void *result_addr,
                            int result_max_size)
{
  INFO("video service ON_ENCODE_H264_LBR_SET!\n");
  am_service_result_t *service_result = (am_service_result_t*) result_addr;
  memset(service_result, 0, sizeof(am_service_result_t));
  if (!msg_data) {
    ERROR("NULL pointer!\n");
    service_result->ret = -1;
    return;
  }
  am_encode_h264_lbr_ctrl_t *lbr_param = (am_encode_h264_lbr_ctrl_t*) msg_data;
  do {
    AMIEncodeDevice *en_dev = g_video_camera->get_encode_device();
    if (!en_dev) {
      ERROR("get encode device failed\n");
      service_result->ret = -1;
      break;
    }
    AMEncodeLBRConfig lbr_config;
    if (en_dev->get_lbr_config(&lbr_config) != AM_RESULT_OK) {
      ERROR("get lbr config failed!\n");
      service_result->ret = -1;
      break;
    }

    if (TEST_BIT(lbr_param->enable_bits,
                 AM_ENCODE_H264_LBR_ENABLE_LBR_EN_BIT)) {
      lbr_config.stream_params[lbr_param->stream_id].enable_lbr =
          lbr_param->enable_lbr;
    }
    if (TEST_BIT(lbr_param->enable_bits,
                 AM_ENCODE_H264_LBR_AUTO_BITRATE_CEILING_EN_BIT)) {
      lbr_config.stream_params[lbr_param->stream_id].auto_target =
          lbr_param->auto_bitrate_ceiling;
    }
    if (TEST_BIT(lbr_param->enable_bits,
                 AM_ENCODE_H264_LBR_BITRATE_CEILING_EN_BIT)) {
      lbr_config.stream_params[lbr_param->stream_id].bitrate_ceiling =
          lbr_param->bitrate_ceiling;
    }
    if (TEST_BIT(lbr_param->enable_bits,
                 AM_ENCODE_H264_LBR_DROP_FRAME_EN_BIT)) {
      lbr_config.stream_params[lbr_param->stream_id].frame_drop =
          lbr_param->drop_frame;
    }

    if (en_dev->set_lbr_config(&lbr_config) != AM_RESULT_OK) {
      ERROR("set lbr config failed!\n");
      service_result->ret = -1;
      break;
    }
  } while (0);
}

void ON_ENCODE_H264_LBR_GET(void *msg_data,
                            int msg_data_size,
                            void *result_addr,
                            int result_max_size)
{
  INFO("video service ON_ENCODE_H264_LBR_GET!\n");
  am_service_result_t *service_result = (am_service_result_t*) result_addr;
  memset(service_result, 0, sizeof(am_service_result_t));
  if (!msg_data) {
    ERROR("NULL pointer!\n");
    service_result->ret = -1;
    return;
  }
  am_encode_h264_lbr_ctrl_t *lbr_param =
      (am_encode_h264_lbr_ctrl_t*) service_result->data;
  uint32_t stream_id = *((uint32_t*) msg_data);
  do {
    AMIEncodeDevice *en_dev = g_video_camera->get_encode_device();
    if (!en_dev) {
      ERROR("get encode device failed\n");
      service_result->ret = -1;
      break;
    }

    AMEncodeLBRConfig lbr_config;
    if (en_dev->get_lbr_config(&lbr_config) != AM_RESULT_OK) {
      ERROR("get lbr control config failed!\n");
      service_result->ret = -1;
      break;
    }

    lbr_param->stream_id = stream_id;
    lbr_param->enable_lbr = lbr_config.stream_params[stream_id].enable_lbr;
    lbr_param->auto_bitrate_ceiling =
        lbr_config.stream_params[stream_id].auto_target;
    lbr_param->bitrate_ceiling =
        lbr_config.stream_params[stream_id].bitrate_ceiling;
    lbr_param->drop_frame = lbr_config.stream_params[stream_id].frame_drop;
  } while (0);
}

void ON_VIDEO_ENCODE_START(void *msg_data,
                           int msg_data_size,
                           void *result_addr,
                           int result_max_size)
{
  INFO("video service VIDEO ENCODE START\n");
  int32_t ret = 0;
  am_service_result_t *service_result = (am_service_result_t*)result_addr;
  if (g_video_camera->start() != AM_RESULT_OK) {
    ret = -1;
    ERROR("Video Service: failed to start encoding\n");
  }
  service_result->ret = ret;
}

void ON_VIDEO_ENCODE_STOP(void *msg_data,
                          int msg_data_size,
                          void *result_addr,
                          int result_max_size)
{
  INFO("video service VIDEO ENCODE STOP\n");
  int32_t ret = 0;
  am_service_result_t *service_result = (am_service_result_t*) result_addr;
  if (g_video_camera->stop() != AM_RESULT_OK) {
    ret = -1;
    ERROR("Video Service: failed to stop encoding\n");
  }

  service_result->ret = ret;
}

void ON_VIDEO_FORCE_IDR(void *msg_data,
                        int msg_data_size,
                        void *result_addr,
                        int result_max_size)
{
  INFO("video service ON_VIDEO_FORCE_IDR\n");
  int32_t ret = 0;
  am_service_result_t *service_result = (am_service_result_t*) result_addr;

  do {
    AMIEncodeDevice *en_dev = g_video_camera->get_encode_device();
    if (!en_dev) {
      ERROR("Get encode device failed\n");
      ret = -1;
      break;
    }

    AM_STREAM_ID id = *(AM_STREAM_ID*)msg_data;
    ret = en_dev->set_force_idr(id);
  } while (0);
  service_result->ret = ret;
}

void ON_COMMON_GET_EVENT(void *msg_data,
                        int msg_data_size,
                        void *result_addr,
                        int result_max_size)
{
  INFO("video service ON_COMMON_GET_EVENT!\n");
  do {
    AMIEncodeDevice *en_dev = g_video_camera->get_encode_device();
    if (!en_dev) {
      ERROR("get encode device failed\n");
      break;
    }
    AMLBRControl *lbr_ctrl = en_dev->get_lbr_ctrl_list();
    if (!lbr_ctrl) {
      ERROR("get lbr control list failed!\n");
      break;
    }
    lbr_ctrl->receive_motion_info_from_ipc_lbr(msg_data);

    if (en_dev->is_motion_enable()) {
      en_dev->update_motion_state((AM_EVENT_MESSAGE*)msg_data);
    }
  } while (0);
}

void ON_VIDEO_OVERLAY_GET_MAX_NUM(void *msg_data,
                                  int msg_data_size,
                                  void *result_addr,
                                  int result_max_size)
{
  INFO("video service ON_VIDEO_OVERLAY_GET_MAX_NUM\n");
  am_service_result_t *service_result = (am_service_result_t*) result_addr;
  memset(service_result, 0, sizeof(am_service_result_t));

  uint32_t *value = (uint32_t*)service_result->data;
  *value = OSD_AREA_MAX_NUM;

  service_result->ret = 0;
}

void ON_VIDEO_OVERLAY_DESTROY(void *msg_data,
                              int msg_data_size,
                              void *result_addr,
                              int result_max_size)
{
  INFO("video service ON_VIDEO_OVERLAY_DESTROY\n");
  int32_t ret = 0;
  am_service_result_t *service_result = (am_service_result_t*) result_addr;

  do {
    AMIEncodeDevice *en_dev = g_video_camera->get_encode_device();
    if (!en_dev) {
      ERROR("Get encode device failed\n");
      ret = -1;
      break;
    }

    AMIOSDOverlay *overlay = en_dev->get_osd_overlay();
    if (!overlay) {
      ERROR("Get overlay instance failed!\n");
      ret = -1;
      break;
    }

    AMIEncodeDeviceConfig *dev_config = en_dev->get_encode_config();
    if (!dev_config) {
      ERROR("Can't get encode device config!");
      ret = -1;
      break;
    }
    AMEncodeParamAll *param = dev_config->get_encode_param();
    if (!param) {
      ERROR("Can't get encode param!");
      ret = -1;
      break;
    }

    if (!(overlay->destroy())) {
      ret = -1;
      ERROR("Video Service: failed to destroy overlay\n");
    }

    memset(&param->osd_overlay_config, 0, sizeof(AMOSDOverlayConfig));
    dev_config->sync();
  } while (0);
  service_result->ret = ret;
}

void ON_VIDEO_OVERLAY_SAVE(void *msg_data,
                           int msg_data_size,
                           void *result_addr,
                           int result_max_size)
{
  INFO("video service ON_VIDEO_OVERLAY_SAVE\n");
  int32_t ret = 0;
  am_service_result_t *service_result = (am_service_result_t*) result_addr;

  do {
    AMIEncodeDevice *en_dev = g_video_camera->get_encode_device();
    if (!en_dev) {
      ERROR("Get encode device failed\n");
      ret = -1;
      break;
    }

    AMIEncodeDeviceConfig *dev_config = en_dev->get_encode_config();
    if (!dev_config) {
      ERROR("Can't get encode device config!");
      ret = -1;
      break;
    }

    AMEncodeParamAll *param = dev_config->get_encode_param();
    if (!param) {
      ERROR("Can't get encode param!");
      service_result->ret = -1;
      break;
    }
    //set save flag
    param->osd_overlay_config.save_flag = true;

    dev_config->sync();
    dev_config->save_config();

  } while (0);
  service_result->ret = ret;
}

void ON_VIDEO_OVERLAY_ADD(void *msg_data,
                          int msg_data_size,
                          void *result_addr,
                          int result_max_size)
{
  INFO("video service ON_VIDEO_OVERLAY_ADD\n");
  am_service_result_t *service_result = (am_service_result_t*) result_addr;
  memset(service_result, 0, sizeof(am_service_result_t));

  if (!msg_data) {
    service_result->ret = -1;
    return;
  }

  am_overlay_t *cfg = (am_overlay_t*)msg_data;
  uint32_t stream_id = cfg->stream_id;
  if (stream_id<0 || stream_id >=AM_STREAM_MAX_NUM) {
    ERROR("Wrong stream ID: %d!\n",stream_id);
    service_result->ret = -1;
    return;
  }

  do {
    AMIEncodeDevice *en_dev = g_video_camera->get_encode_device();
    if (!en_dev) {
      ERROR("Get encode device failed\n");
      service_result->ret = -1;
      break;
    }
    AMIOSDOverlay *overlay = en_dev->get_osd_overlay();
    if (!overlay) {
      ERROR("Get overlay instance failed!\n");
      service_result->ret = -1;
      break;
    }
    AMIEncodeDeviceConfig *dev_config = en_dev->get_encode_config();
    if (!dev_config) {
      ERROR("Can't get encode device config!");
      service_result->ret = -1;
      break;
    }
    AMEncodeParamAll *param = dev_config->get_encode_param();
    if (!param) {
      ERROR("Can't get encode param!");
      service_result->ret = -1;
      break;
    }
    AMOSDInfo *osd_cfg = &param->osd_overlay_config.osd_cfg[stream_id];
    if (osd_cfg->area_num >= OSD_AREA_MAX_NUM) {
      ERROR("Stream%d current already had %d osd area!\n",stream_id, osd_cfg->area_num);
      service_result->ret = -1;
      break;
    }

    if (TEST_BIT(cfg->enable_bits, AM_ADD_EN_BIT)) {
      AMOSDInfo overlay_info = {0};
      overlay_info.area_num = 1;

      AMOSDArea *area = &(overlay_info.overlay_area[0]);
      area->enable = cfg->area.enable;

      if (TEST_BIT(cfg->enable_bits, AM_WIDTH_EN_BIT)) {
        area->width = cfg->area.width;
      }
      if (TEST_BIT(cfg->enable_bits, AM_HEIGHT_EN_BIT)) {
        area->height = cfg->area.height;
      }
      if (TEST_BIT(cfg->enable_bits, AM_LAYOUT_EN_BIT)) {
        area->area_position.style = (AMOSDLayoutStyle)cfg->area.layout;
      }
      if (TEST_BIT(cfg->enable_bits, AM_OFFSETX_EN_BIT)) {
        area->area_position.offset_x = cfg->area.offset_x;
      }
      if (TEST_BIT(cfg->enable_bits, AM_OFFSETY_EN_BIT)) {
        area->area_position.offset_y = cfg->area.offset_y;
      }

      AMOSDAttribute *attr = &(overlay_info.attribute[0]);
      if (TEST_BIT(cfg->enable_bits, AM_TYPE_EN_BIT)) {
        attr->type = (AMOSDType)cfg->area.type;
        INFO("osd type = %d \n",attr->type);
      } else {
        WARN("No overlay type, use test pattern type!\n");
        attr->type = AM_OSD_TYPE_TEST_PATTERN;
      }
      if (TEST_BIT(cfg->enable_bits, AM_ROTATE_EN_BIT)) {
        attr->enable_rotate = cfg->area.rotate;
      } else {
        attr->enable_rotate = 1;
      }

      if ((AM_OSD_TYPE_GENERIC_STRING == attr->type) ||
          (AM_OSD_TYPE_TIME_STRING == attr->type)) {
        AMOSDFont *font = &(attr->osd_text_box.font);
        if (TEST_BIT(cfg->enable_bits, AM_FONT_TYPE_EN_BIT)) {
          strcpy(font->ttf_name, cfg->area.font_type);
          font->ttf_name[OSD_FILENAME_MAX_NUM-1] = '\0';
        }
        if (TEST_BIT(cfg->enable_bits, AM_FONT_SIZE_EN_BIT)) {
          font->size = cfg->area.font_size;
        }
        if (TEST_BIT(cfg->enable_bits, AM_FONT_OUTLINE_W_EN_BIT)) {
          font->outline_width = cfg->area.font_outline_w;
        }
        if (TEST_BIT(cfg->enable_bits, AM_FONT_HOR_BOLD_EN_BIT)) {
          font->hor_bold = cfg->area.font_hor_bold;
        }
        if (TEST_BIT(cfg->enable_bits, AM_FONT_VER_BOLD_EN_BIT)) {
          font->ver_bold = cfg->area.font_ver_bold;
        }
        if (TEST_BIT(cfg->enable_bits, AM_FONT_ITALIC_EN_BIT)) {
          font->italic = cfg->area.font_italic;
        }
        if (TEST_BIT(cfg->enable_bits, AM_STRING_EN_BIT)) {
          strcpy(attr->osd_text_box.osd_string, cfg->area.str);
          attr->osd_text_box.osd_string[OSD_STRING_MAX_NUM-1] = '\0';
        }
        if (TEST_BIT(cfg->enable_bits, AM_FONT_COLOR_EN_BIT)) {
          if (cfg->area.font_color < 8) {
            attr->osd_text_box.font_color.id = cfg->area.font_color;
          } else {
            attr->osd_text_box.font_color.id = 8;
            attr->osd_text_box.font_color.color.v =
                (uint8_t)((cfg->area.font_color >> 24) & 0xff);
            attr->osd_text_box.font_color.color.u =
                (uint8_t)((cfg->area.font_color >> 16) & 0xff);
            attr->osd_text_box.font_color.color.y =
                (uint8_t)((cfg->area.font_color >> 8) & 0xff);
            attr->osd_text_box.font_color.color.a =
                (uint8_t)(cfg->area.font_color & 0xff);
          }
        }
      } else if (AM_OSD_TYPE_PICTURE == attr->type) {
        if (TEST_BIT(cfg->enable_bits, AM_COLOR_KEY_EN_BIT))
        {
          attr->osd_pic.colorkey[0].color.v =
              (uint8_t)((cfg->area.color_key >> 24) & 0xff);
          attr->osd_pic.colorkey[0].color.u =
              (uint8_t)((cfg->area.color_key >> 16) & 0xff);
          attr->osd_pic.colorkey[0].color.y =
              (uint8_t)((cfg->area.color_key >> 8) & 0xff);
          attr->osd_pic.colorkey[0].color.a =
              (uint8_t)(cfg->area.color_key & 0xff);
        }
        if (TEST_BIT(cfg->enable_bits, AM_COLOR_RANGE_EN_BIT))
        {
          attr->osd_pic.colorkey[0].range = cfg->area.color_range;
        }
        if (TEST_BIT(cfg->enable_bits, AM_BMP_EN_BIT)) {
          strcpy(attr->osd_pic.filename, cfg->area.bmp);
          attr->osd_pic.filename[OSD_FILENAME_MAX_NUM-1] = '\0';
        }
      }

      overlay->add((AM_STREAM_ID)stream_id, &overlay_info);
      //save configure if it is success added
      int32_t area_id = -1;
      while(!((overlay_info.active_area >> ++area_id) & 0x01)
          && (area_id < OSD_AREA_MAX_NUM));

      //area_id=OSD_AREA_MAX_NUM means this add action is failed
      if (area_id < OSD_AREA_MAX_NUM) {
        osd_cfg->active_area |= overlay_info.active_area;
        memcpy(&osd_cfg->attribute[area_id], attr, sizeof(AMOSDAttribute));
        memcpy(&osd_cfg->overlay_area[area_id], area, sizeof(AMOSDArea));
        ++osd_cfg->area_num;
      }
    }

    dev_config->sync();
  } while (0);
}

void ON_VIDEO_OVERLAY_SET(void *msg_data,
                          int msg_data_size,
                          void *result_addr,
                          int result_max_size)
{
  INFO("video service ON_VIDEO_OVERLAY_SET\n");
  am_service_result_t *service_result = (am_service_result_t*) result_addr;
  memset(service_result, 0, sizeof(am_service_result_t));

  if (!msg_data) {
    service_result->ret = -1;
    return;
  }

  am_overlay_set_t *cfg = (am_overlay_set_t*) msg_data;
  uint32_t stream_id = cfg->osd_id.stream_id;
  if (stream_id<0 || stream_id >=AM_STREAM_MAX_NUM) {
    ERROR("Wrong stream ID: %d!\n",stream_id);
    service_result->ret = -1;
    return;
  }

  do {
    AMIEncodeDevice *en_dev = g_video_camera->get_encode_device();
    if (!en_dev) {
      ERROR("Get encode device failed\n");
      service_result->ret = -1;
      break;
    }
    AMIOSDOverlay *overlay = en_dev->get_osd_overlay();
    if (!overlay) {
      ERROR("Get overlay instance failed!\n");
      service_result->ret = -1;
      break;
    }
    AMIEncodeDeviceConfig *dev_config = en_dev->get_encode_config();
    if (!dev_config) {
      ERROR("Can't get encode device config!");
      service_result->ret = -1;
      break;
    }
    AMEncodeParamAll *param = dev_config->get_encode_param();
    if (!param) {
      ERROR("Can't get encode param!");
      service_result->ret = -1;
      break;
    }
    AMOSDInfo *osd_cfg = &param->osd_overlay_config.osd_cfg[stream_id];
    //remove
    if (TEST_BIT(cfg->enable_bits, AM_REMOVE_EN_BIT)) {
      int32_t area_id = cfg->osd_id.area_id;
      if (!(overlay->remove((AM_STREAM_ID)stream_id, area_id))) {
        ERROR("Video Service: failed to remove overlay\n");
        service_result->ret = -1;
        break;
      }
      //update state information
      if (area_id != OSD_AREA_MAX_NUM) {
        osd_cfg->active_area &= ~(0x01<<area_id);
        memset(&osd_cfg->attribute[area_id], 0,sizeof(AMOSDAttribute));
        memset(&osd_cfg->overlay_area[area_id], 0, sizeof(AMOSDArea));
        --osd_cfg->area_num;
      } else {
        memset(osd_cfg, 0, sizeof(AMOSDInfo));
      }
    }
    //enable
    if (TEST_BIT(cfg->enable_bits, AM_ENABLE_EN_BIT)) {
      int32_t area_id = cfg->osd_id.area_id;
      if (!(overlay->enable((AM_STREAM_ID)stream_id, area_id))) {
        ERROR("Video Service: failed to enable overlay\n");
        service_result->ret = -1;
        break;
      }
      //update state information
      if (area_id != OSD_AREA_MAX_NUM) {
        osd_cfg->overlay_area[area_id].enable = 1;
      } else {
        for (uint32_t i=0; i<OSD_AREA_MAX_NUM; ++i) {
          if ((osd_cfg->active_area >> i)&0x01) {
            osd_cfg->overlay_area[i].enable = 1;
          }
        }
      }
    }
    //disable
    if (TEST_BIT(cfg->enable_bits, AM_DISABLE_EN_BIT)) {
      int32_t area_id = cfg->osd_id.area_id;
      if (!(overlay->disable((AM_STREAM_ID)stream_id, area_id))) {
        ERROR("Video Service: failed to disable overlay\n");
        service_result->ret = -1;
        break;
      }
      //update state information
      if (area_id != OSD_AREA_MAX_NUM) {
        osd_cfg->overlay_area[area_id].enable = 0;
      } else {
        for (uint32_t i=0; i<OSD_AREA_MAX_NUM; ++i) {
          osd_cfg->overlay_area[i].enable = 0;
        }
      }
    }

    dev_config->sync();
  } while (0);
  service_result->ret = 0;
}

void ON_VIDEO_OVERLAY_GET(void *msg_data,
                          int msg_data_size,
                          void *result_addr,
                          int result_max_size)
{
  INFO("video service ON_VIDEO_OVERLAY_GET!\n");
  am_service_result_t *service_result = (am_service_result_t*) result_addr;
  memset(service_result, 0, sizeof(am_service_result_t));
  if (!msg_data) {
    ERROR("NULL pointer!\n");
    service_result->ret = -1;
    return;
  }
  uint32_t stream_id = ((am_overlay_id_t*)msg_data)->stream_id;
  uint32_t area_id = ((am_overlay_id_t*)msg_data)->area_id;

  do {
    AMIEncodeDevice *en_dev = g_video_camera->get_encode_device();
    if (!en_dev) {
      ERROR("Get encode device failed\n");
      service_result->ret = -1;
      break;
    }
    AMIOSDOverlay *overlay = en_dev->get_osd_overlay();
    if (!overlay) {
      ERROR("Get overlay instance failed!\n");
      service_result->ret = -1;
      break;
    }
    AMIEncodeDeviceConfig *dev_config = en_dev->get_encode_config();
    if (!dev_config) {
      ERROR("Can't get encode device config!");
      service_result->ret = -1;
      break;
    }
    AMEncodeParamAll *param = dev_config->get_encode_param();
    if (!param) {
      ERROR("Can't get encode param!");
      service_result->ret = -1;
      break;
    }
    AMOSDInfo *osd_cfg = &param->osd_overlay_config.osd_cfg[stream_id];
    am_overlay_area_t *cfg = (am_overlay_area_t*)service_result->data;

    if ((osd_cfg->active_area >> area_id)&0x01) {
      cfg->enable = osd_cfg->overlay_area[area_id].enable;
    } else {
      cfg->enable = 2; //not created
    }
    cfg->width = osd_cfg->overlay_area[area_id].width;
    cfg->height = osd_cfg->overlay_area[area_id].height;
    cfg->layout = osd_cfg->overlay_area[area_id].area_position.style;
    cfg->offset_x = osd_cfg->overlay_area[area_id].area_position.offset_x;
    cfg->offset_y = osd_cfg->overlay_area[area_id].area_position.offset_y;
    cfg->type = osd_cfg->attribute[area_id].type;
    cfg->rotate = osd_cfg->attribute[area_id].enable_rotate;

    if ((AM_OSD_TYPE_GENERIC_STRING == cfg->type) ||
        (AM_OSD_TYPE_TIME_STRING == cfg->type)) {
      cfg->font_size = osd_cfg->attribute[area_id].osd_text_box.font.size;
      cfg->font_outline_w = osd_cfg->attribute[area_id].osd_text_box.font.outline_width;
      cfg->font_hor_bold = osd_cfg->attribute[area_id].osd_text_box.font.hor_bold;
      cfg->font_ver_bold = osd_cfg->attribute[area_id].osd_text_box.font.ver_bold;
      cfg->font_italic = osd_cfg->attribute[area_id].osd_text_box.font.italic;
      uint32_t tmp = osd_cfg->attribute[area_id].osd_text_box.font_color.id;;
      if (tmp < 8) {
        cfg->font_color = tmp;
      } else {
        AMOSDCLUT color = osd_cfg->attribute[area_id].osd_text_box.font_color.color;
        cfg->font_color = (color.v << 24) | (color.u << 16) | (color.y << 8) | color.y;
      }
    } else if (AM_OSD_TYPE_PICTURE == cfg->type) {
      AMOSDCLUT color = osd_cfg->attribute[area_id].osd_pic.colorkey[0].color;
      cfg->color_key = (color.v << 24) | (color.u << 16) | (color.y << 8) | color.a;
      cfg->color_range = osd_cfg->attribute[area_id].osd_pic.colorkey[0].range;
    }

  } while (0);
}
