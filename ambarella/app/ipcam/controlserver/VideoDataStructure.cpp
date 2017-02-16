/**********************************************************************
 * VideoDataStructure.cpp
 *
 * Histroy:
 *  2011年03月22日 - [Yupeng Chang] created file
 *
 * Copyright (C) 2008-2011, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 **********************************************************************/
#include "VideoDataStructure.h"
#include "basetypes.h"
#include "iav_drv.h"
#include "ambas_common.h"
#include "ambas_vin.h"
#include <sys/ioctl.h>
#include <stdio.h>

bool EncodeInfo::get_encode_info (int iav_fd, unsigned int stream_id)
{
#define h264_config (encode_type._h264)
#define jpeg_config (encode_type._jpeg)
  clean ();
  if ((stream_id < 0) || (stream_id >= MW_MAX_STREAM_NUM)) {
    return false;
  } else {
    iav_encode_format_ex_t format = {0};
    format.id = (1 << stream_id);

    if (ioctl(iav_fd, IAV_IOC_GET_ENCODE_FORMAT_EX, &format) < 0) {
      perror ("IAV_IOC_GET_ENCODE_FORMAT_EX");
      return false;
    } else {
      struct amba_video_info video_info = {0};

      if (ioctl(iav_fd, IAV_IOC_VIN_SRC_GET_VIDEO_INFO, &video_info) < 0) {
        perror ("IAV_IOC_VIN_SRC_GET_VIDEO_INFO");
        return false;
      }
      encode_format.stream        = stream_id;
      encode_format.encode_type   = format.encode_type;
      encode_format.source        = format.source;
      encode_format.encode_width  = format.encode_width;
      encode_format.encode_height = format.encode_height;
      encode_format.encode_x      = format.encode_x;
      encode_format.encode_y      = format.encode_y;
      encode_format.encode_fps    = AMBA_VIDEO_FPS(video_info.fps);
    }

    /* According to the encode type, get H.264 config or Mjpeg config */
    switch (encode_format.encode_type) {
      case MW_ENCODE_H264: {                      /* H.264 */
        iav_h264_config_ex_t h264 = {0};
        iav_bitrate_info_ex_t bps = {0};
        h264.id = (1 << stream_id);
        bps.id = (1 << stream_id);
        if (!(ioctl (iav_fd, IAV_IOC_GET_H264_CONFIG_EX, &h264) < 0) &&
            !(ioctl (iav_fd, IAV_IOC_GET_BITRATE_EX, &bps) < 0)) {
          h264_config.stream = stream_id;
          h264_config.M = h264.M;
          h264_config.N = h264.N;
          h264_config.idr_interval = h264.idr_interval;
          h264_config.gop_model = h264.gop_model;
          h264_config.profile = h264.entropy_codec;
          h264_config.brc_mode = bps.rate_control_mode;
          h264_config.cbr_avg_bps = bps.cbr_avg_bitrate;
          h264_config.vbr_min_bps = bps.vbr_min_bitrate;
          h264_config.vbr_max_bps = bps.vbr_max_bitrate;
          /* Fill the encode_format.flip_rotate */
          encode_format.flip_rotate = ((h264.rotate_clockwise << 2) |
                                       (h264.vflip << 1) |
                                       h264.hflip);
        } else {
          perror ("IAV_IOC_GET_H264_CONFIG_EX");
          return false;
        }
      }break;
      case MW_ENCODE_MJPEG: {                     /* MJPEG */
        iav_jpeg_config_ex_t jpeg = {0};
        jpeg.id = (1 << stream_id);
        if (!(ioctl(iav_fd, IAV_IOC_GET_JPEG_CONFIG_EX, &jpeg) < 0)) {
          jpeg_config.chroma_format = jpeg.chroma_format;
          jpeg_config.quality = jpeg.quality;
        } else {
          perror ("IAV_IOC_GET_JPEG_CONFIG_EX");
          return false;
        }
      }break;
      case MW_ENCODE_OFF:                         /* NONE */
      default:
        return false;
    }
  }

  return true;
}

bool EncodeSetting::get_stream_info (int iav_fd)
{
  clean ();
  /* Get stream status */
  for (int id = 0; id < MW_MAX_STREAM_NUM; ++ id) {
    iav_encode_stream_info_ex_t stream_info = {0};
    stream_info.id = (1 << id);
    if (ioctl(iav_fd, IAV_IOC_GET_ENCODE_STREAM_INFO_EX, &stream_info) < 0) {
      perror ("IAV_IOC_GET_ENCODE_STREAM_INFO_EX");
      return false;
    }
     stream_state[id] =
       (((stream_info.state == IAV_STREAM_STATE_READY_FOR_ENCODING) ||
         (stream_info.state == IAV_STREAM_STATE_ENCODING)) ? true :
                                                             false);
  }

  /* Get encode info */
  for (int id = 0; id < MW_MAX_STREAM_NUM; ++ id) {
    if (stream_state[id]) {
      /* Only get the active stream info */
      stream_state[id] = stream_info[id].get_encode_info (iav_fd, id);
    } else {
      /* Clear disabled stream information */
      stream_info[id].clean();
    }
#ifdef DEBUG
    printf ("Stream %d is %s\n", id, (stream_state[id] ? "enabled" :
                                                         "disabled"));
#endif
  }

  return true;
}

