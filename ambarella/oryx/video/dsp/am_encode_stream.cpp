/*******************************************************************************
 * am_encode_stream.cpp
 *
 * History:
 *   2014-7-15 - [lysun] created file
 *
 * Copyright (C) 2008-2016, Ambarella Co,Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/
#include <sys/time.h>
#include <sys/ioctl.h>
#include "am_base_include.h"
#include "iav_ioctl.h"
#include "am_encode_buffer.h"
#include "am_encode_device.h"
#include "am_log.h"

AMEncodeStream::AMEncodeStream() :
    m_state(AM_ENCODE_STREAM_STATE_IDLE),
    m_osd_overlay(NULL),
    m_encode_device(NULL),
    m_id(AM_STREAM_ID_INVALID),
    m_iav(-1)
{
  memset(&m_stream_format, 0, sizeof(m_stream_format));
  memset(&m_stream_config, 0, sizeof(m_stream_config));
}

AMEncodeStream::~AMEncodeStream()
{

}


/***************** public methods start ***************************/

AM_RESULT AMEncodeStream::init(AMEncodeDevice *encode_device,
                               AM_STREAM_ID id, int iav_hd)
{
  m_encode_device = encode_device;
  m_id = id;
  m_iav = iav_hd;
  return AM_RESULT_OK;
}

AM_RESULT AMEncodeStream::set_encode_format(AMEncodeStreamFormat *encode_format)
{
  struct iav_stream_format format;
  struct iav_stream_cfg streamcfg;
  AM_RESULT result = AM_RESULT_OK;

  do {
    if (!encode_format) {
      result = AM_RESULT_ERR_INVALID;
      break;
    }

    //update frame fps to a lower fps, to avoid
    //the issue if we want to change to a lower fps but higher resolution
    //the check in IAV won't let it pass check
    memset(&streamcfg, 0, sizeof(streamcfg));
    streamcfg.id = m_id;
    streamcfg.cid = IAV_STMCFG_FPS;
    streamcfg.arg.fps.fps_multi = 1;
    streamcfg.arg.fps.fps_div = 30;

    //set fps before stream format, otherwise, sometimes unable to
    //setup high resolution but low fps
    if (ioctl(m_iav, IAV_IOC_SET_STREAM_CONFIG, &streamcfg) < 0) {
      result = AM_RESULT_ERR_DSP;
      break;
    }

    memset(&format, 0, sizeof(format));
    format.id = m_id;
    //determine type, and do data type conversion
    if (encode_format->type == AM_ENCODE_STREAM_TYPE_H264) {
      format.type = IAV_STREAM_TYPE_H264;
    } else if (encode_format->type == AM_ENCODE_STREAM_TYPE_MJPEG) {
      format.type = IAV_STREAM_TYPE_MJPEG;
    } else {
      format.type = IAV_STREAM_TYPE_NONE;
    }

    //encode window
    format.enc_win.width = encode_format->width;
    format.enc_win.height = encode_format->height;
    format.enc_win.x = encode_format->offset_x;
    format.enc_win.y = encode_format->offset_y;

    //buf id
    format.buf_id = iav_srcbuf_id((uint32_t) (encode_format->source));

    //hflip and vflip
    format.hflip = encode_format->hflip;
    format.vflip = encode_format->vflip;
    if (encode_format->rotate_90_ccw) {
      format.rotate_cw = 1;
      format.hflip = !encode_format->hflip;
      format.vflip = !encode_format->vflip;
    } else {
      format.rotate_cw = 0;
      format.hflip = encode_format->hflip;
      format.vflip = encode_format->vflip;
    }

    if (ioctl(m_iav, IAV_IOC_SET_STREAM_FORMAT, &format) < 0) {
      PERROR("IAV_IOC_SET_STREAM_FORMAT");
      result = AM_RESULT_ERR_DSP;
      break;
    }


    //update frame fps to the expected FPS
    //so if the resolution is big and this frame factor is high
    //then the check will fail here
    memset(&streamcfg, 0, sizeof(streamcfg));
    streamcfg.id = m_id;
    streamcfg.cid = IAV_STMCFG_FPS;
    streamcfg.arg.fps.fps_multi = encode_format->fps.fps_multi;
    streamcfg.arg.fps.fps_div = encode_format->fps.fps_div;

    //set fps before stream format, otherwise, sometimes unable to
    //setup high resolution but low fps
    if (ioctl(m_iav, IAV_IOC_SET_STREAM_CONFIG, &streamcfg) < 0) {
      result = AM_RESULT_ERR_DSP;
      break;
    }


    //back up the format
    m_stream_format = *encode_format;
  } while (0);
  return result;
}

//still get format from IAV, because other app may have changed it
AM_RESULT AMEncodeStream::get_encode_format(AMEncodeStreamFormat *encode_format)
{
  AM_RESULT result = AM_RESULT_OK;
  struct iav_stream_format format;

  do {
    if (!encode_format) {
      result = AM_RESULT_ERR_INVALID;
      break;
    }

    memset(&format, 0, sizeof(format));
    format.id = m_id;
    if (ioctl(m_iav, IAV_IOC_GET_STREAM_FORMAT, &format) < 0) {
      PERROR("IAV_IOC_GET_STREAM_FORMAT");
      result = AM_RESULT_ERR_DSP;
      break;
    }

    if (format.type == IAV_STREAM_TYPE_H264) {
      encode_format->type = AM_ENCODE_STREAM_TYPE_H264;
    } else if (format.type == IAV_STREAM_TYPE_MJPEG) {
      encode_format->type = AM_ENCODE_STREAM_TYPE_MJPEG;
    } else {
      encode_format->type = AM_ENCODE_STREAM_TYPE_NONE;
    }

    //encode window
    encode_format->width = format.enc_win.width;
    encode_format->height = format.enc_win.height;
    encode_format->offset_x = format.enc_win.x;
    encode_format->offset_y = format.enc_win.y;

    //buf id
    encode_format->source =
        AM_ENCODE_SOURCE_BUFFER_ID((uint32_t) format.buf_id);

    //hflip and vflip
    encode_format->hflip = format.hflip;
    encode_format->vflip = format.vflip;

    if (format.rotate_cw) {
      encode_format->rotate_90_ccw = 1;
      encode_format->hflip = !format.hflip;
      encode_format->vflip = !format.vflip;
    }else {
      encode_format->rotate_90_ccw = 0;
      encode_format->hflip = format.hflip;
      encode_format->vflip = format.vflip;
    }

    //TODO: Compare encode_format with cached m_stream_format,
    m_stream_format = *encode_format;
  } while (0);

  return result;
}

//simple interfacce, will not allow arbitrary change for QP limit
//for the frames. otherwise, customer may change it to broken
static AM_RESULT fill_bitrate(iav_bitrate *bitrate,
                              AM_ENCODE_H264_RATE_CONTROL bitrate_control,
                              uint32_t target_bitrate)
{
  AM_RESULT result = AM_RESULT_OK;

  do {
    if (!bitrate) {
      result = AM_RESULT_ERR_INVALID;
      break;
    }

    memset(bitrate, 0, sizeof(*bitrate));

    switch (bitrate_control) {
      case AM_ENCODE_H264_RC_CBR:
        bitrate->vbr_setting = IAV_BRC_SCBR;
        bitrate->average_bitrate = target_bitrate;
        bitrate->qp_min_on_I = 10;
        bitrate->qp_max_on_I = 51;
        bitrate->qp_min_on_P = 10;
        bitrate->qp_max_on_P = 51;
        bitrate->qp_min_on_B = 10;
        bitrate->qp_max_on_B = 51;
        bitrate->i_qp_reduce = 6; //hard code i_qp_reduce
        bitrate->p_qp_reduce = 3; //hard code p_qp_reduce
        bitrate->adapt_qp = 0;    //disable aqp by default
        bitrate->skip_flag = 0;
        break;
      case AM_ENCODE_H264_RC_LBR:
        bitrate->vbr_setting = IAV_BRC_SCBR;
        bitrate->average_bitrate = target_bitrate;
        bitrate->qp_min_on_I = 17;
        bitrate->qp_max_on_I = 51;
        bitrate->qp_min_on_P = 17;
        bitrate->qp_max_on_P = 51;
        bitrate->qp_min_on_B = 17;
        bitrate->qp_max_on_B = 51;
        bitrate->i_qp_reduce = 6; //hard code i_qp_reduce
        bitrate->p_qp_reduce = 3; //hard code p_qp_reduce
        bitrate->adapt_qp = 0;    //disable aqp by default
        bitrate->skip_flag = 0;
        break;
      default:
        ERROR("AMEncodestream: rate control %d not supported now \n",
              bitrate_control);
        result = AM_RESULT_ERR_INVALID;
        break;
    }
  } while (0);

  return result;
}

//this is static config, if the stream is encoding, then the new setting
//is still set to IAV and remembered by IAV, but will only take effect for
//the next encoding session.
AM_RESULT AMEncodeStream::set_h264_encode_config(AMEncodeH264Config * h264_config)
{
  struct iav_h264_cfg h264cfg;
  struct iav_stream_cfg stmcfg;
  struct iav_bitrate bitrate;
  //struct iav_h264_gop gop;
  AM_RESULT result = AM_RESULT_OK;

  do {
    if (!h264_config) {
      result = AM_RESULT_ERR_INVALID;
      break;
    }

    memset(&h264cfg, 0, sizeof(h264cfg));
    h264cfg.id = m_id;
    h264cfg.gop_structure = h264_config->gop_model;
    h264cfg.M = h264_config->M;
    h264cfg.N = h264_config->N;
    h264cfg.idr_interval = h264_config->idr_interval;
    h264cfg.profile = h264_config->profile_level;

    //chroma is very special
    if (h264_config->chroma_format == AM_ENCODE_CHROMA_FORMAT_YUV420) {
      h264cfg.chroma_format = H264_CHROMA_YUV420;
    } else if (h264_config->chroma_format == AM_ENCODE_CHROMA_FORMAT_Y8) {
      h264cfg.chroma_format = H264_CHROMA_MONO;
    } else {
      result = AM_RESULT_ERR_INVALID;
      break;
    }

    h264cfg.mv_threshold = h264_config->mv_threshold;
    h264cfg.au_type = h264_config->au_type;
    h264cfg.enc_improve = h264_config->encode_improve;
    h264cfg.multi_ref_p = h264_config->multi_ref_p;
    h264cfg.long_term_intvl = h264_config->long_term_intvl;

    //no settings on panic mode so far
    if (ioctl(m_iav, IAV_IOC_SET_H264_CONFIG, &h264cfg) < 0) {
      result = AM_RESULT_ERR_DSP;
      break;
    }

    //rate control configs
    //prepare bitrate

    result = fill_bitrate(&bitrate,
                          h264_config->bitrate_control,
                          h264_config->target_bitrate);

    memset(&stmcfg, 0, sizeof(stmcfg));
    stmcfg.id = m_id;
    stmcfg.cid = IAV_H264_CFG_BITRATE;
    stmcfg.arg.h264_rc = bitrate;
    if (ioctl(m_iav, IAV_IOC_SET_STREAM_CONFIG, &stmcfg) < 0) {
      PERROR("IAV_IOC_SET_STREAM_CONFIG");
      result = AM_RESULT_ERR_DSP;
      break;
    }
  } while (0);

  return result;
}

AMEncodeSourceBuffer *AMEncodeStream::get_source_buffer()
{
  AMEncodeSourceBuffer *source_buffer = NULL;
  int stream_id = (int)m_id;
  int buffer_id = (int)m_stream_format.source;
  if (buffer_id < 0)
    return NULL;

  INFO("AMEncodeStream %d 's buffer id is %d \n",  stream_id, buffer_id);
  //if it's valid buffer id, then get the source buffer object
  source_buffer  = m_encode_device->get_source_buffer_list() + buffer_id;
  return source_buffer;
}

static AM_ENCODE_H264_GOP_MODEL get_gop_model(uint32_t iav_gop_structure)
{
  AM_ENCODE_H264_GOP_MODEL model;
  switch (iav_gop_structure) {
    case 0:
      model = AM_ENCODE_H264_GOP_MODEL_SIMPLE;
      break;
    case 1:
      model = AM_ENCODE_H264_GOP_MODEL_ADVANCED;
      break;
    case 2:
      model = AM_ENCODE_H264_GOP_MODEL_SVCT_FIRST;
      break;
    case 5:
      model = AM_ENCODE_H264_GOP_MODEL_SVCT_LAST;
      break;
    default:
      ERROR("AMEncodeStream: get_gop_model failed, %d \n", iav_gop_structure);
      model = AM_ENCODE_H264_GOP_MODEL_SIMPLE;
      break;
  }
  return model;
}

static AM_ENCODE_H264_PROFILE get_profile_level(uint32_t iav_profile_level)
{
  AM_ENCODE_H264_PROFILE h264_profile =  AM_ENCODE_H264_PROFILE_MAIN;
  switch (iav_profile_level){
    case 0:
      h264_profile = AM_ENCODE_H264_PROFILE_BASELINE;
      break;
    case 1:
      h264_profile = AM_ENCODE_H264_PROFILE_MAIN;
      break;
    case 2:
      h264_profile = AM_ENCODE_H264_PROFILE_HIGH;
      break;
    default:
      ERROR("h264 profile %d not supported, use default\n", iav_profile_level);
      h264_profile = AM_ENCODE_H264_PROFILE_MAIN;
      break;
  }

  return h264_profile;
}


static AM_ENCODE_H264_AU_TYPE get_au_type(uint32_t iav_au_type)
{
  AM_ENCODE_H264_AU_TYPE au_type;
  switch(iav_au_type) {
    case 0:
      au_type = AM_ENCODE_H264_AU_TYPE_NO_AUD_NO_SEI;
      break;
    case 1:
      au_type = AM_ENCODE_H264_AU_TYPE_AUD_BEFORE_SPS_WITH_SEI;
      break;
    case 2:
      au_type = AM_ENCODE_H264_AU_TYPE_AUD_AFTER_SPS_WITH_SEI;
      break;
    case 3:
      au_type =  AM_ENCODE_H264_AU_TYPE_NO_AUD_WITH_SEI;
      break;
    default:
      ERROR("h264 au type %d not supported \n", iav_au_type);
      au_type = AM_ENCODE_H264_AU_TYPE_NO_AUD_NO_SEI;
      break;
  };
  return au_type;
}

AM_RESULT AMEncodeStream::get_h264_encode_config(AMEncodeH264Config *h264_config)
{
  struct iav_h264_cfg h264cfg;
  struct iav_stream_cfg stmcfg;
  struct iav_bitrate bitrate;
  AM_RESULT result = AM_RESULT_OK;
  memset(&h264cfg, 0, sizeof(h264cfg));

  do {
    if (!h264_config) {
      result = AM_RESULT_ERR_INVALID;
      break;
    }

    if (ioctl(m_iav, IAV_IOC_GET_H264_CONFIG, &h264cfg) < 0) {
      result = AM_RESULT_ERR_DSP;
      break;
    }

    h264_config->gop_model = get_gop_model(h264cfg.gop_structure);
    h264_config->M = h264cfg.M;
    h264_config->N = h264cfg.N;
    h264_config->idr_interval = h264cfg.idr_interval;
    h264_config->profile_level = get_profile_level(h264cfg.profile);

    if (h264cfg.chroma_format == H264_CHROMA_YUV420) {
      h264_config->chroma_format = AM_ENCODE_CHROMA_FORMAT_YUV420;
    } else if (h264cfg.chroma_format == H264_CHROMA_MONO) {
      h264_config->chroma_format = AM_ENCODE_CHROMA_FORMAT_Y8;
    } else {
      result = AM_RESULT_ERR_INVALID;
      break;
    }

    h264_config->mv_threshold = h264cfg.mv_threshold;
    h264_config->au_type = get_au_type(h264cfg.au_type);
    h264_config->encode_improve = h264cfg.enc_improve;
    h264_config->multi_ref_p = h264cfg.multi_ref_p;
    h264_config->long_term_intvl = h264cfg.long_term_intvl;

    //get bitrate:

    memset(&stmcfg, 0, sizeof(stmcfg));
    stmcfg.id = m_id;
    stmcfg.cid = IAV_H264_CFG_BITRATE;
    if (ioctl(m_iav, IAV_IOC_GET_STREAM_CONFIG, &stmcfg) < 0) {
      PERROR("IAV_IOC_GET_STREAM_CONFIG");
      result = AM_RESULT_ERR_DSP;
      break;
    }
    bitrate = stmcfg.arg.h264_rc;
    h264_config->target_bitrate = bitrate.average_bitrate;

  } while (0);

  return result;
}

//this is static config, if the stream is encoding, then the new setting
//is still set to IAV and remembered by IAV, but will only take effect for
//the next encoding session.
AM_RESULT AMEncodeStream::set_mjpeg_encode_config(AMEncodeMJPEGConfig * mjpeg_config)
{
  struct iav_mjpeg_cfg mjpeg_cfg;
  AM_RESULT result = AM_RESULT_OK;

  do {
    if (!mjpeg_config) {
      result = AM_RESULT_ERR_INVALID;
      break;
    }

    memset(&mjpeg_cfg, 0, sizeof(mjpeg_cfg));
    mjpeg_cfg.id = m_id;
    mjpeg_cfg.quality = mjpeg_config->quality;
    if (mjpeg_config->chroma_format == AM_ENCODE_CHROMA_FORMAT_YUV422) {
      mjpeg_cfg.chroma_format = JPEG_CHROMA_YUV422;
    } else if (mjpeg_config->chroma_format == AM_ENCODE_CHROMA_FORMAT_YUV420) {
      mjpeg_cfg.chroma_format = JPEG_CHROMA_YUV420;
    } else {
      mjpeg_cfg.chroma_format = JPEG_CHROMA_MONO;
    }

    if (ioctl(m_iav, IAV_IOC_SET_MJPEG_CONFIG, &mjpeg_cfg) < 0) {
      result = AM_RESULT_ERR_DSP;
      break;
    }
  } while (0);
  return result;
}

AM_RESULT AMEncodeStream::get_mjpeg_encode_config(AMEncodeMJPEGConfig * mjpeg_config)
{
  struct iav_mjpeg_cfg mjpeg_cfg;

  AM_RESULT result = AM_RESULT_OK;

  do {
    if (!mjpeg_config) {
      result = AM_RESULT_ERR_INVALID;
      break;
    }
    memset(&mjpeg_cfg, 0, sizeof(mjpeg_cfg));
    mjpeg_cfg.id = m_id;
    if (ioctl(m_iav, IAV_IOC_GET_MJPEG_CONFIG, &mjpeg_cfg) < 0) {
      result = AM_RESULT_ERR_DSP;
      break;
    }
    mjpeg_config->quality = mjpeg_cfg.quality;
    if (mjpeg_cfg.chroma_format == JPEG_CHROMA_YUV422) {
      mjpeg_config->chroma_format = AM_ENCODE_CHROMA_FORMAT_YUV422;
    } else if (mjpeg_cfg.chroma_format == JPEG_CHROMA_YUV420) {
      mjpeg_config->chroma_format = AM_ENCODE_CHROMA_FORMAT_YUV420;
    } else {
      mjpeg_config->chroma_format = AM_ENCODE_CHROMA_FORMAT_Y8;
    }

  } while (0);
  return result;
}

AM_RESULT AMEncodeStream::set_encode_config(AMEncodeStreamConfig *encode_config)
{
  AM_RESULT result = AM_RESULT_OK;
  AM_ENCODE_STREAM_TYPE type = m_stream_format.type;
  do {
    if (type == AM_ENCODE_STREAM_TYPE_NONE) {
      INFO("Ignore stream, because this is not enabled\n");
      break;
    } else if (type == AM_ENCODE_STREAM_TYPE_H264) {
      set_h264_encode_config(&encode_config->h264_config);
    } else if (type == AM_ENCODE_STREAM_TYPE_MJPEG) {
      set_mjpeg_encode_config(&encode_config->mjpeg_config);
    }
    //backup the new config
    m_stream_config = *encode_config;
  } while (0);
  return result;
}

AM_RESULT AMEncodeStream::get_encode_config(AMEncodeStreamConfig *encode_config)
{
  AM_ENCODE_STREAM_TYPE type = m_stream_format.type;
  AM_RESULT result = AM_RESULT_OK;
  //get format, currently, no optimized version still get everything from
  //IAV driver, in the future, possible to optimize to directly return from
  //m_stream_config
  do {
    if (!encode_config) {
      result = AM_RESULT_ERR_INVALID;
      break;
    }
    memset(encode_config, 0, sizeof(*encode_config));
    if (type == AM_ENCODE_STREAM_TYPE_H264) {
      get_h264_encode_config(&encode_config->h264_config);
    } else if (type == AM_ENCODE_STREAM_TYPE_MJPEG) {
      get_mjpeg_encode_config(&encode_config->mjpeg_config);
    } else {
      ERROR("AMEncodeStream: stream type %d, not support to get config\n",
            type);
      result = AM_RESULT_ERR_INVALID;
      break;
    }
    //backup the new config
    m_stream_config = *encode_config;

  } while (0);

  return result;
}

AM_RESULT AMEncodeStream::set_encode_dynamic_config(AMEncodeStreamDynamicConfig *encode_dyna_config)
{
  //struct iav_stream_info info;
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (!encode_dyna_config) {
      result = AM_RESULT_ERR_INVALID;
      break;
    }

    if (m_stream_format.type == AM_ENCODE_STREAM_TYPE_NONE) {
      ERROR("cannot set dynamic config when stream type not configured\n");
      result = AM_RESULT_ERR_INVALID;
      break;
    }

    //refresh state again
    result = update_stream_state();
    if (result != AM_RESULT_OK) {
      ERROR("AMEncodeStream: update_stream_state failed \n");
      break;
    }

    //check state, state must be in encoding for dynamic config
    if (m_state != AM_ENCODE_STREAM_STATE_ENCODING) {
      ERROR("AMEncodestream: current state %d, busy for dynamic config\n");
      result = AM_RESULT_ERR_PERM;
      break;
    }

    //change frame fps on the fly is by "frame factor change"
    if (encode_dyna_config->frame_fps_change) {
      struct iav_stream_cfg streamcfg;
      memset(&streamcfg, 0, sizeof(streamcfg));
      streamcfg.id = m_id;
      streamcfg.cid = IAV_STMCFG_FPS;
      streamcfg.arg.fps.fps_multi = encode_dyna_config->frame_fps.fps_multi;
      streamcfg.arg.fps.fps_div = encode_dyna_config->frame_fps.fps_div;
      INFO("AMEncodeStream: Stream [%d] change frame interval %d/%d \n",
           m_id,
           streamcfg.arg.fps.fps_multi,
           streamcfg.arg.fps.fps_div);
      if (ioctl(m_iav, IAV_IOC_SET_STREAM_CONFIG, &streamcfg) < 0) {
        result = AM_RESULT_ERR_DSP;
        break;
      }
    }

    //change encode_offset on the fly is kind of DPTZ Type II
    if (encode_dyna_config->encode_offset_change) {
      struct iav_stream_format format;
      memset(&format, 0, sizeof(format));
      format.id = m_id;
      if (ioctl(m_iav, IAV_IOC_GET_STREAM_FORMAT, &format) < 0) {
        result = AM_RESULT_ERR_DSP;
        break;
      }
      //just update the encoding offset should be on the fly change
      format.enc_win.x = encode_dyna_config->encode_offset.x;
      format.enc_win.y = encode_dyna_config->encode_offset.y;
      if (ioctl(m_iav, IAV_IOC_SET_STREAM_FORMAT, &format) < 0) {
        result = AM_RESULT_ERR_DSP;
        break;
      }

      //should update m_stream_format to keep sync
      m_stream_format.offset_x = encode_dyna_config->encode_offset.x;
      m_stream_format.offset_y = encode_dyna_config->encode_offset.y;
    }

    if (encode_dyna_config->chroma_format_change) {
      struct iav_stream_cfg cfg;
      memset(&cfg, 0, sizeof(cfg));
      cfg.id = m_id;
      cfg.cid = IAV_STMCFG_CHROMA;

      if (m_stream_format.type == AM_ENCODE_STREAM_TYPE_H264) {
        if (encode_dyna_config->chroma_format == AM_ENCODE_CHROMA_FORMAT_Y8) {
          cfg.arg.chroma = H264_CHROMA_MONO;
        } else {
          cfg.arg.chroma = H264_CHROMA_YUV420;
        }
      } else {
        if (encode_dyna_config->chroma_format == AM_ENCODE_CHROMA_FORMAT_Y8) {
          cfg.arg.chroma = JPEG_CHROMA_MONO;
        } else {
          cfg.arg.chroma = JPEG_CHROMA_YUV420;
        }
      }
      if (ioctl(m_iav, IAV_IOC_SET_STREAM_CONFIG, &cfg) < 0) {
        result = AM_RESULT_ERR_DSP;
        break;
      }
      //update chroma format for both
      m_stream_config.h264_config.chroma_format =
          encode_dyna_config->chroma_format;
      m_stream_config.mjpeg_config.chroma_format =
          encode_dyna_config->chroma_format;
    }

    if (encode_dyna_config->h264_gop_change) {
      struct iav_stream_cfg cfg;
      struct iav_h264_gop gop;
      memset(&cfg, 0, sizeof(cfg));
      cfg.id = m_id;
      cfg.cid = IAV_H264_CFG_GOP;
      gop.id = m_id;
      gop.N = encode_dyna_config->h264_gop.N;
      gop.idr_interval = encode_dyna_config->h264_gop.idr_interval;
      cfg.arg.h264_gop = gop;
      if (ioctl(m_iav, IAV_IOC_SET_STREAM_CONFIG, &cfg) < 0) {
        result = AM_RESULT_ERR_DSP;
        break;
      }

      //update m_stream_config for backup
      m_stream_config.h264_config.N = gop.N;
      m_stream_config.h264_config.idr_interval = gop.idr_interval;
    }

    if (encode_dyna_config->h264_target_bitrate_change) {
      struct iav_stream_cfg cfg;
      struct iav_bitrate bitrate;
      memset(&cfg, 0, sizeof(cfg));
      cfg.id = m_id;
      cfg.cid = IAV_H264_CFG_BITRATE;

      result = fill_bitrate(&bitrate,
                            m_stream_config.h264_config.bitrate_control,
                            encode_dyna_config->h264_target_bitrate);

      if (result != AM_RESULT_OK) {
        break;
      }

      cfg.arg.h264_rc = bitrate;
      // Following configurations can be changed during encoding
      if (ioctl(m_iav, IAV_IOC_SET_STREAM_CONFIG, &cfg) < 0) {
        result = AM_RESULT_ERR_DSP;
        break;
      }

      m_stream_config.h264_config.target_bitrate =
          encode_dyna_config->h264_target_bitrate;

    }

    if (encode_dyna_config->h264_force_idr_immediately) {
      struct iav_stream_cfg stream_cfg;
      memset(&stream_cfg, 0, sizeof(stream_cfg));
      stream_cfg.id = m_id;
      stream_cfg.cid = IAV_H264_CFG_FORCE_IDR;
      if (ioctl(m_iav, IAV_IOC_SET_STREAM_CONFIG, &stream_cfg) < 0) {
        result = AM_RESULT_ERR_DSP;
        break;
      }
    }

    if (encode_dyna_config->jpeg_quality_change) {
      struct iav_stream_cfg stream_cfg;
      memset(&stream_cfg, 0, sizeof(stream_cfg));
      stream_cfg.id = m_id;
      stream_cfg.cid = IAV_MJPEG_CFG_QUALITY;
      stream_cfg.arg.mjpeg_quality = encode_dyna_config->jpeg_quality;
      if (ioctl(m_iav, IAV_IOC_SET_STREAM_CONFIG, &stream_cfg) < 0) {
        result = AM_RESULT_ERR_DSP;
        break;
      }
      m_stream_config.mjpeg_config.quality = encode_dyna_config->jpeg_quality;
    }
  } while (0);

  //TODO:
  //future optimization may be checking the m_stream_config to compare with the
  //new settings, if same, then don't apply
  return result;
}

AM_RESULT AMEncodeStream::start_encode()
{
  AM_RESULT result = AM_RESULT_OK;

  do {
    uint32_t streamid;
    if (m_stream_format.type == AM_ENCODE_STREAM_TYPE_NONE) {
      INFO("AMEncodeStream: ignore encoding when stream%d type is none", m_id);
      break;
    }

    streamid = 1 << m_id;
    if (ioctl(m_iav, IAV_IOC_START_ENCODE, streamid) < 0) {
      ERROR("IAV_IOC_START_ENCODE, stream%d: %s", m_id, strerror(errno));
      result = AM_RESULT_ERR_DSP;
      break;
    }
  } while (0);
  update_stream_state();
  return result;
}

AM_RESULT AMEncodeStream::stop_encode()
{
  uint32_t streamid;
  AM_RESULT result = AM_RESULT_OK;
  struct timeval tv1, tv2;
  do {
    if (m_stream_format.type == AM_ENCODE_STREAM_TYPE_NONE) {
      INFO("AMEncodeStream: ignore encoding when type is none \n");
      break;
    }

    gettimeofday(&tv1, NULL);
    streamid = 1 << m_id;
    if (ioctl(m_iav, IAV_IOC_STOP_ENCODE, streamid) < 0) {
      PERROR("IAV_IOC_STOP_ENCODE");
      result = AM_RESULT_ERR_DSP;
      break;
    }

    gettimeofday(&tv2, NULL);
    INFO("Stop Encode costs %ld micro seconds \n",
         (tv2.tv_usec + tv2.tv_sec * 1000000L)
         - (tv1.tv_usec + tv1.tv_sec * 1000000L));
  } while (0);
  update_stream_state();
  return result;
}

bool AMEncodeStream::is_encoding()
{
  update_stream_state();
  return ((m_state ==  AM_ENCODE_STREAM_STATE_STARTING) ||
      (m_state ==  AM_ENCODE_STREAM_STATE_ENCODING));
}

/***************** public methods end ***************************/


AM_RESULT AMEncodeStream::update_stream_state()
{
  struct iav_stream_info info;
  AM_RESULT result = AM_RESULT_OK;

  do {
    memset(&info, 0, sizeof(info));
    info.id = m_id;
    if (ioctl(m_iav, IAV_IOC_GET_STREAM_INFO, &info) < 0) {
      PERROR("IAV_IOC_GET_STREAM_INFO");
      result = AM_RESULT_ERR_DSP;
      break;
    } else {

      switch (info.state) {
        case IAV_STREAM_STATE_IDLE:
          m_state = AM_ENCODE_STREAM_STATE_IDLE;
          break;
        case IAV_STREAM_STATE_STARTING:
          m_state = AM_ENCODE_STREAM_STATE_STARTING;
          break;
        case IAV_STREAM_STATE_ENCODING:
          m_state = AM_ENCODE_STREAM_STATE_ENCODING;
          break;
        case IAV_STREAM_STATE_STOPPING:
          m_state = AM_ENCODE_STREAM_STATE_STOPPING;
          break;
        case IAV_STREAM_STATE_UNKNOWN:
          m_state = AM_ENCODE_STREAM_STATE_ERROR;
          break;
        default:

          result = AM_RESULT_ERR_INVALID;
          break;
      }
      PRINTF("AMEncodeStream: stream %d state %d \n", (int )(m_id), info.state);
    }

  } while (0);
  return result;
}
