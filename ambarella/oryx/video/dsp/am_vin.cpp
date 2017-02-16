/*******************************************************************************
 * am_vin.cpp
 *
 * History:
 *   2014-7-14 - [lysun] created file
 *
 * Copyright (C) 2008-2016, Ambarella Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "iav_ioctl.h"
#include "am_base_include.h"
#include "am_video_dsp.h"
#include "am_vin.h"
#include "am_log.h"
#include "am_video_device.h"
#include "am_vin_trans.h"
#include "am_video_trans.h"

#define DIV_ROUND(divident, divider) \
    (((divident)+((divider)>>1)) / (divider))

#define VIN0_VSYNC "/proc/ambarella/vin0_idsp"

AMVin::AMVin() :
    m_id(AM_VIN_ID_INVALID),
    m_iav(-1),
    m_state(AM_VIN_STATE_NOT_INITIALIZED),
    m_video_fps_q9_format(0),
    m_hdr_type(AM_HDR_SINGLE_EXPOSURE)
{
  m_param.type = AM_VIN_TYPE_NONE;
  m_param.mode = AM_VIN_MODE_AUTO;
  m_param.flip = AM_VIDEO_FLIP_NONE;
  m_param.fps = AM_VIDEO_FPS_AUTO;
  memset(&m_resolution, 0, sizeof(m_resolution));
}

AMVin::~AMVin()
{
}

AM_RESULT AMVin::init(int iav_hd, int vin_id, AMVinParam *vin_param)
{
  AM_RESULT result = AM_RESULT_OK;
  m_iav = iav_hd;
  do {
    if ((vin_id < AM_VIN_0) || (vin_id > AM_VIN_1)) {
      ERROR("AMVin: init failed , vin_id error %d \n");
      result = AM_RESULT_ERR_INVALID;
      break;
    }

    if (!vin_param) {
      result = AM_RESULT_ERR_INVALID;
      break;
    }

    m_id = AM_VIN_ID(vin_id);
    m_param = *vin_param;
  } while (0);
  return result;
}

AM_RESULT AMVin::set_hdr_type(AM_HDR_EXPOSURE_TYPE type)
{
  m_hdr_type = type;
  return AM_RESULT_OK;
}

AM_RESULT AMVin::get_hdr_type(AM_HDR_EXPOSURE_TYPE *type)
{
  if (type) {
    *type = m_hdr_type;
  }
  return AM_RESULT_OK;
}

AM_RESULT AMVin::destroy()
{
  AM_RESULT result = AM_RESULT_OK;
  struct vindev_mode vsrc_mode;
  do {
    INFO("AMVin: destroy: power off VIN\n");
    memset(&vsrc_mode, 0, sizeof(vsrc_mode));
    vsrc_mode.vsrc_id = 0;
    vsrc_mode.video_mode = (unsigned int) AMBA_VIDEO_MODE_OFF;
    vsrc_mode.hdr_mode = AMBA_VIDEO_LINEAR_MODE;
    if (ioctl(m_iav, IAV_IOC_VIN_SET_MODE, &vsrc_mode) < 0) {
      PERROR("AMVin: power off");
      result = AM_RESULT_ERR_DSP;
      break;
    }
  } while (0);
  return result;
}

AM_RESULT AMVin::reset()
{
  return AM_RESULT_OK;
}

AM_RESULT AMVin::detect_vin_capability()
{
  return AM_RESULT_OK;
}




AM_RESULT AMVin::set_mode()
{
  struct vindev_mode vsrc_mode;
  struct vindev_devinfo vsrc_info;
  struct vindev_video_info video_info;
  AM_RESULT result = AM_RESULT_OK;
  memset(&vsrc_mode, 0, sizeof(vsrc_mode));
  memset(&vsrc_info, 0, sizeof(vsrc_info));
  memset(&video_info, 0, sizeof(video_info));

  do {
    m_resolution = vin_mode_to_resolution(m_param.mode);

    result = detect_vin_capability();
    if (result != AM_RESULT_OK) {
      ERROR("AMVin: detect capability failed \n");
      break;
    }

#if 0
    if (mirror_pattern != VINDEV_MIRROR_AUTO || bayer_pattern != VINDEV_BAYER_PATTERN_AUTO) {
      mirror_mode.vsrc_id = 0;
      if (ioctl(fd_iav, IAV_IOC_VIN_SET_MIRROR, &mirror_mode) < 0) {
        perror("IAV_IOC_VIN_SET_MIRROR");
        return -1;
      }
    }
#endif
    //detect VIN type
    vsrc_info.vsrc_id = m_id;
    if (ioctl(m_iav, IAV_IOC_VIN_GET_DEVINFO, &vsrc_info) < 0) {
      PERROR("IAV_IOC_VIN_GET_DEVINFO");
      result = AM_RESULT_ERR_DSP;
      break;
    }

    //handle the AUTO config by checking sensor ID
    switch(vsrc_info.sensor_id)
    {
      case SENSOR_OV4689:
        INFO("AMVin: OV4689 sensor is used \n");
        if (m_param.mode == AM_VIN_MODE_AUTO)  {
          if ((m_hdr_type == AM_HDR_2_EXPOSURE) ||
              (m_hdr_type == AM_HDR_3_EXPOSURE)) {
            m_param.mode = AM_VIN_MODE_QHD;
            INFO("AMVin: VIN mode chooses to be 2560x1440\n");
          } else {
            m_param.mode = AM_VIN_MODE_4M_16_9;
            INFO("AMVin: VIN mode chooses to be 2688x1512\n");
          }
        }

        /*if ((m_hdr_type == AM_HDR_3_EXPOSURE) ||
            (m_hdr_type == AM_HDR_4_EXPOSURE)) {
           ERROR("AMVin: 3X or 4X HDR not supported by Oryx yet\n");
           result = AM_RESULT_ERR_INVALID;
        }*/
        break;

      case SENSOR_AR0230:
        INFO("AMVin: AR0230 sensor is used \n");
        break;

      case SENSOR_MN34220PL:
              INFO("AMVin: MN34220 sensor is used \n");
        break;


      case SENSOR_OV9718:
        INFO("AMVin: OV9718 sensor is used \n");
        if (m_hdr_type != AM_HDR_SINGLE_EXPOSURE) {
          ERROR("AMVin: HDR not supported on this sensor\n");
          result = AM_RESULT_ERR_INVALID;
        }
        break;

      case SENSOR_OV9750:
        INFO("AMVin: OV9750 sensor is used \n");
        if (m_hdr_type != AM_HDR_SINGLE_EXPOSURE) {
          ERROR("AMVin: HDR not supported on this sensor\n");
          result = AM_RESULT_ERR_INVALID;
        }
        break;


      case SENSOR_AR0141:
        INFO("AMVin: AR0141 sensor is used \n");
        if (m_hdr_type != AM_HDR_SINGLE_EXPOSURE) {
          ERROR("AMVin: HDR not supported on this sensor\n");
          result = AM_RESULT_ERR_INVALID;
        }
        break;

      case SENSOR_IMX322:
        INFO("AMVin: IMX322 sensor is used \n");
        if (m_hdr_type != AM_HDR_SINGLE_EXPOSURE) {
          ERROR("AMVin: HDR not supported on this sensor\n");
          result = AM_RESULT_ERR_INVALID;
        }
        break;

      default:
        WARN("AMVin: sensor id is %d, not in support list\n", vsrc_info.sensor_id);
        break;
    }

    if (result!= AM_RESULT_OK) {
      break;
    }

    vsrc_mode.vsrc_id = m_id;
    vsrc_mode.video_mode = get_amba_video_mode(m_param.mode);
    vsrc_mode.hdr_mode = get_hdr_mode(m_hdr_type);
    if (ioctl(m_iav, IAV_IOC_VIN_SET_MODE, &vsrc_mode) < 0) {
      PERROR("AMVin: IAV_IOC_VIN_SET_MODE ");
      result = AM_RESULT_ERR_DSP;
      break;
    }


    // get current video info
    video_info.vsrc_id = m_id;
    video_info.info.mode = AMBA_VIDEO_MODE_CURRENT;
    if (ioctl(m_iav, IAV_IOC_VIN_GET_VIDEOINFO, &video_info) < 0) {
      PERROR("IAV_IOC_VIN_GET_VIDEOINFO");
      result = AM_RESULT_ERR_DSP;
      break;
    }

    m_resolution.width = video_info.info.width;
    m_resolution.height = video_info.info.height;
    m_video_fps_q9_format = video_info.info.fps;

    INFO("AMVin(%d): set_mode, width = %d, height = %d, fps = %d\n",
         m_id,
         m_resolution.width,
         m_resolution.height,
         m_video_fps_q9_format);
  } while (0);
  return result;
}

AM_RESULT AMVin::get_mode(AM_VIN_MODE *mode)
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (!mode) {
      result = AM_RESULT_ERR_INVALID;
      break;
    }

    *mode = m_param.mode;
  } while (0);
  return result;
}

AM_RESULT AMVin::show_vin_info()
{
  AM_RESULT result = AM_RESULT_OK;
  struct vindev_fps video_fps;
  struct vindev_video_info video_info;

  do {
    /* get current video fps */
    video_fps.vsrc_id = 0;
    if (ioctl(m_iav, IAV_IOC_VIN_GET_FPS, &video_fps) < 0) {
      PERROR("IAV_IOC_VIN_GET_FPS");
      result = AM_RESULT_ERR_DSP;
      break;
    }
    /* get current video info */
    video_info.vsrc_id = 0;
    video_info.info.mode = AMBA_VIDEO_MODE_CURRENT;
    if (ioctl(m_iav, IAV_IOC_VIN_GET_VIDEOINFO, &video_info) < 0) {
      PERROR("IAV_IOC_VIN_GET_VIDEOINFO");
      result = AM_RESULT_ERR_DSP;
      break;
    }

    print_vin_info(&video_info.info, video_fps.fps);
  } while (0);
  return result;
}

AM_RESULT AMVin::print_vin_info(const void *video_info, int fps_q9)
{
  char        format[32];
  char        hdrmode[32];
  char        fps[32];
  char        type[32];
  char        bits[32];
  char        ratio[32];
  char        system[32];

  amba_video_info *vinfo = (amba_video_info*)video_info;
  switch (vinfo->format) {
    case AMBA_VIDEO_FORMAT_PROGRESSIVE:
      snprintf(format, 32, "%s", "P");
      break;
    case AMBA_VIDEO_FORMAT_INTERLACE:
      snprintf(format, 32, "%s", "I");
      break;
    case AMBA_VIDEO_FORMAT_AUTO:
      snprintf(format, 32, "%s", "Auto");
      break;
    default:
      snprintf(format, 32, "format?%d", vinfo->format);
      break;
  }

  switch (vinfo->hdr_mode) {
    case AMBA_VIDEO_2X_HDR_MODE:
      snprintf(hdrmode, 32, "%s", "(HDR 2x)");
      break;
    case AMBA_VIDEO_3X_HDR_MODE:
      snprintf(hdrmode, 32, "%s", "(HDR 3x)");
      break;
    case AMBA_VIDEO_4X_HDR_MODE:
      snprintf(hdrmode, 32, "%s", "(HDR 4x)");
      break;
    case AMBA_VIDEO_LINEAR_MODE:
    default:
      snprintf(hdrmode, 32, "%s", "");
      break;
  }

  switch (fps_q9) {
    case AMBA_VIDEO_FPS_AUTO:
      snprintf(fps, 32, "%s", "AUTO");
      break;
    case AMBA_VIDEO_FPS_29_97:
      snprintf(fps, 32, "%s", "29.97");
      break;
    case AMBA_VIDEO_FPS_59_94:
      snprintf(fps, 32, "%s", "59.94");
      break;
    default:
      snprintf(fps, 32, "%d", DIV_ROUND(521000000, fps_q9));
      break;
  }

  switch (vinfo->type) {
    case AMBA_VIDEO_TYPE_RGB_RAW:
      snprintf(type, 32, "%s", "RGB");
      break;
    case AMBA_VIDEO_TYPE_YUV_601:
      snprintf(type, 32, "%s", "YUV BT601");
      break;
    case AMBA_VIDEO_TYPE_YUV_656:
      snprintf(type, 32, "%s", "YUV BT656");
      break;
    case AMBA_VIDEO_TYPE_YUV_BT1120:
      snprintf(type, 32, "%s", "YUV BT1120");
      break;
    case AMBA_VIDEO_TYPE_RGB_601:
      snprintf(type, 32, "%s", "RGB BT601");
      break;
    case AMBA_VIDEO_TYPE_RGB_656:
      snprintf(type, 32, "%s", "RGB BT656");
      break;
    case AMBA_VIDEO_TYPE_RGB_BT1120:
      snprintf(type, 32, "%s", "RGB BT1120");
      break;
    default:
      snprintf(type, 32, "type?%d", vinfo->type);
      break;
  }

  switch (vinfo->bits) {
    case AMBA_VIDEO_BITS_AUTO:
      snprintf(bits, 32, "%s", "Bits Not Availiable");
      break;
    default:
      snprintf(bits, 32, "%dbits", vinfo->bits);
      break;
  }

  switch (vinfo->ratio) {
    case AMBA_VIDEO_RATIO_AUTO:
      snprintf(ratio, 32, "%s", "AUTO");
      break;
    case AMBA_VIDEO_RATIO_4_3:
      snprintf(ratio, 32, "%s", "4:3");
      break;
    case AMBA_VIDEO_RATIO_16_9:
      snprintf(ratio, 32, "%s", "16:9");
      break;
    default:
      snprintf(ratio, 32, "ratio?%d", vinfo->ratio);
      break;
  }

  switch (vinfo->system) {
    case AMBA_VIDEO_SYSTEM_AUTO:
      snprintf(system, 32, "%s", "AUTO");
      break;
    case AMBA_VIDEO_SYSTEM_NTSC:
      snprintf(system, 32, "%s", "NTSC");
      break;
    case AMBA_VIDEO_SYSTEM_PAL:
      snprintf(system, 32, "%s", "PAL");
      break;
    case AMBA_VIDEO_SYSTEM_SECAM:
      snprintf(system, 32, "%s", "SECAM");
      break;
    case AMBA_VIDEO_SYSTEM_ALL:
      snprintf(system, 32, "%s", "ALL");
      break;
    default:
      snprintf(system, 32, "system?%d", vinfo->system);
      break;
  }

  PRINTF("VIN Info:\t%dx%d%s%s\t%s\t%s\t%s\t%s\t%s\trev[%d]\n",
         vinfo->width, vinfo->height,
         format, hdrmode, fps, type, bits, ratio, system, vinfo->rev);

  return AM_RESULT_OK;
}

AM_RESULT AMVin::set_fps()
{
  AM_RESULT result = AM_RESULT_OK;
  struct vindev_fps vsrc_fps;
  do {
    vsrc_fps.vsrc_id = 0;
    m_video_fps_q9_format = get_amba_video_fps(m_param.fps);
    vsrc_fps.fps = m_video_fps_q9_format;
    if (ioctl(m_iav, IAV_IOC_VIN_SET_FPS, &vsrc_fps) < 0) {
      PERROR("IAV_IOC_VIN_SET_FPS");
      result = AM_RESULT_ERR_DSP;
      break;
    }
  } while (0);
  return result;
}

AM_RESULT AMVin::get_fps(AM_VIDEO_FPS *fps)
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (!fps) {
      result = AM_RESULT_ERR_INVALID;
      break;
    }

    *fps = m_param.fps;
  } while (0);
  return result;
}

AM_RESULT AMVin::get_state(AM_VIN_STATE *state)
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (!state) {
      result = AM_RESULT_ERR_INVALID;
      break;
    }

    *state = m_state;
  } while (0);
  return result;
}

AM_RESULT AMVin::flip()
{
  struct vindev_mirror mirror_mode;
  AM_RESULT result = AM_RESULT_OK;
  do {
    //set flip
    if (m_param.flip != AM_VIDEO_FLIP_AUTO) {
      mirror_mode.vsrc_id = 0;
      mirror_mode.pattern = get_mirror_pattern(m_param.flip);
      mirror_mode.bayer_pattern = VINDEV_BAYER_PATTERN_AUTO;
      if (ioctl(m_iav, IAV_IOC_VIN_SET_MIRROR, &mirror_mode) < 0) {
        PERROR("IAV_IOC_VIN_SET_MIRROR");
        result = AM_RESULT_ERR_DSP;
        break;
      }
    }
  } while (0);

  INFO("AMVin: set flip type %d\n", m_param.flip);
  return result;
}

AM_RESULT AMVin::change(AMVinParam *vin_param)
{

  return AM_RESULT_OK;
}

//update VIN from current config,
//may include setup mode, fps, and other properties
AM_RESULT AMVin::update()
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (m_id == AM_VIN_1) {
      INFO("AMVin: second VIN is ignored now\n");
      break;
    }
    result = set_mode();
    if (result != AM_RESULT_OK) {
      ERROR("AMVin: update :set_mode error \n");
      break;
    }
    result = set_fps();
    if (result != AM_RESULT_OK) {
      ERROR("AMVin: update :set_fps error \n");
      break;
    }
  } while (0);
  show_vin_info();
  return result;
}

AM_RESULT AMVin::get_resolution(AMResolution *size)
{
  if (!size) {
    ERROR("NULL point");
    return AM_RESULT_ERR_INVALID;
  }
  *size = m_resolution;
  INFO("get resolution, width: %d, height: %d\n", size->width, size->height);
  return AM_RESULT_OK;
}

AM_RESULT AMVin::get_AGC(AMVinAGC *agc)
{
  if (agc) {
    memset(agc, 0, sizeof(*agc));
  }
  return AM_RESULT_OK;
}

AM_RESULT AMVin::wait_frame_sync()
{
  return AM_RESULT_OK;
}

/****** AMRGBSensorVin ***************/

AMRGBSensorVin::AMRGBSensorVin():
     m_vin_sync_waiters_num(0),
     m_vin_sync_thread(NULL),
     m_vin_sync_exit_flag(false)
 {
     m_param.type = AM_VIN_TYPE_RGB_SENSOR;
 }

AMRGBSensorVin::~AMRGBSensorVin()
{
  destroy();
}

AM_RESULT AMRGBSensorVin::destroy()
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    //set flag to let sync thread to close fd
    m_vin_sync_exit_flag = true;
    //join thread before delete the thread obj
    DEBUG("AMRGBSensorVin: try join sync thread\n");
    m_vin_sync_thread->join();

    //destroy sem after join thread
    if (sem_destroy(&m_vin_sync_sem) < 0) {
          ERROR("AMRGBSensorVin: unable to destroy sync sem\n");
          result = AM_RESULT_ERR_MEM;
          break;
    }

    //delete thread obj
    if (m_vin_sync_thread) {
      delete m_vin_sync_thread;
      m_vin_sync_thread = NULL;
    }
    DEBUG("AMRGBSensorVin: delete sync thread done\n");

    //call parent class 's destroy
    result = AMVin::destroy();
  } while (0);
  return result;
}

void AMRGBSensorVin::vin_sync_thread_func(AMRGBSensorVin *pThis)
{
  if (!pThis) {
    ERROR("AMRGBSensorVin: fail to pass this to thread arg\n");
    return;
  }
  while (!pThis->m_vin_sync_exit_flag) {
    if (ioctl(pThis->m_iav, IAV_IOC_WAIT_NEXT_FRAME, 0) < 0) {
      DEBUG("AMRGBSensorVin: wait next frame failed\n");
      //try to sleep 33ms when IAV is not ready,
      //and poll IAV next frame again
      usleep(33 * 1000);
    } else {
      //only post frame sync when IAV_IOC_WAIT_NEXT_FRAME is OK
      if (pThis->post_frame_sync() != AM_RESULT_OK) {
        ERROR("AMRGBSensorVin: sync thread post sync error\n");
        break;
      }
    }
  }
}

AM_RESULT AMRGBSensorVin::init(int iav_hd, int vin_id, AMVinParam *vin_param)
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    result = AMVin::init(iav_hd, vin_id, vin_param);
    if (result != AM_RESULT_OK)
      break;

    //init vin sem as unnamed sem,
    //we will use the sem to handle the fd sync , and let more readers
    //to be able to wait on sem, rather than fd
    //because fd can only have one waiter, sem can have multiple waiters
    if (sem_init(&m_vin_sync_sem, 0, 0) < 0) {
      ERROR("AMRGBSensorVin: unable to initialize the vin sync sem\n");
      result = AM_RESULT_ERR_MEM;
      break;
    }

    //reset the waiter to 0
    m_vin_sync_waiters_num = 0;
    //create a thread which is used to handle the vsync fd and sem
    m_vin_sync_thread = new std::thread(AMRGBSensorVin::vin_sync_thread_func, this);
    if (!m_vin_sync_thread) {
      ERROR("AMRGBSensorVin: fail to create sync thread \n");
      result = AM_RESULT_ERR_MEM;
      break;
    } else {
      INFO("AMRGBSensorVin: create sync thread done\n");
    }

  } while (0);

  return result;

}


AM_RESULT AMRGBSensorVin::set_mode()
{
  AM_RESULT result = AM_RESULT_OK;
  INFO("AMRGBSensorVin: set_mode \n");
  do {
    result = AMVin::set_mode();
    if (result != AM_RESULT_OK) {
      break;
    }

    //additional handling for sensor type VIN:
    result = set_sensor_initial_agc_shutter();
    if(result != AM_RESULT_OK) {
      break;
    }
  } while (0);
  return result;
}

AM_RESULT AMRGBSensorVin::set_fps()
{
  return AMVin::set_fps();
}

AM_RESULT AMRGBSensorVin::get_AGC(AMVinAGC *agc)
{
  AM_RESULT result = AM_RESULT_OK;
  struct vindev_agc vsrc_agc;
  struct vindev_video_info video_info;
  do {
    if (!agc) {
      result = AM_RESULT_ERR_INVALID;
      break;
    }

    memset(&video_info, 0, sizeof(video_info));
    video_info.vsrc_id = 0;
    video_info.info.mode = AMBA_VIDEO_MODE_CURRENT;
    if (ioctl(m_iav, IAV_IOC_VIN_GET_VIDEOINFO, &video_info) < 0) {
      INFO("AMRGBSensorVin: video info not ready\n");
      result = AM_RESULT_ERR_AGAIN;
      break;
    }

    vsrc_agc.vsrc_id = 0;
    memset(&vsrc_agc, 0, sizeof(vsrc_agc));
    if (ioctl(m_iav, IAV_IOC_VIN_GET_AGC, &vsrc_agc) < 0) {
      ERROR("AMRGBSensorVin::IAV_IOC_VIN_GET_AGC");
      result = AM_RESULT_ERR_DSP;
      break;
    }

    memset(agc, 0, sizeof(*agc));
    agc->agc = vsrc_agc.agc;
    agc->agc_max = vsrc_agc.agc_max;
    agc->agc_min = vsrc_agc.agc_min;
    agc->agc_step = vsrc_agc.agc_step;
    agc->wdr_again_idx_min = vsrc_agc.wdr_again_idx_min;
    agc->wdr_again_idx_max = vsrc_agc.wdr_again_idx_max;
    agc->wdr_dgain_idx_min = vsrc_agc.wdr_dgain_idx_min;
    agc->wdr_dgain_idx_max = vsrc_agc.wdr_dgain_idx_max;

  } while (0);
  return result;
}



AM_RESULT AMRGBSensorVin::wait_frame_sync()
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    //more waiters come
    m_vin_sync_waiters_num++;
    //try wait
    if (sem_wait(&m_vin_sync_sem) < 0) {
      result = AM_RESULT_ERR_MEM;
      //wait failed, restore the wait count
      m_vin_sync_waiters_num--;
      break;
    }
  } while(0);
  return result;
}

AM_RESULT AMRGBSensorVin::post_frame_sync()
{
  AM_RESULT result = AM_RESULT_OK;
  //if no one is waiting, just return.
  //if there are multiple waiters , then post to all of them
  while (m_vin_sync_waiters_num > 0) {
    if (sem_post(&m_vin_sync_sem) < 0) {
      result = AM_RESULT_ERR_MEM;
      //post failed, restore the wait count
      break;
    }
    m_vin_sync_waiters_num--;
  }
  return result;
}

AM_RESULT AMRGBSensorVin::set_sensor_initial_agc_shutter()
{
  AM_RESULT result = AM_RESULT_OK;
  struct vindev_shutter vsrc_shutter;
  struct vindev_agc vsrc_agc;
  uint32_t vin_eshutter_time = 60; // 1/60 sec
  uint32_t vin_agc_db = 6; // 0dB

  uint32_t shutter_time_q9;
  int32_t agc_db;

  shutter_time_q9 = 512000000L / vin_eshutter_time;
  agc_db = vin_agc_db << 24;

  do {
    vsrc_shutter.vsrc_id = 0;
    vsrc_shutter.shutter = shutter_time_q9;
    if (ioctl(m_iav, IAV_IOC_VIN_SET_SHUTTER, &vsrc_shutter) < 0) {
      PERROR("IAV_IOC_VIN_SET_SHUTTER");
      result = AM_RESULT_ERR_DSP;
      break;
    }

    vsrc_agc.vsrc_id = 0;
    vsrc_agc.agc = agc_db;
    if (ioctl(m_iav, IAV_IOC_VIN_SET_AGC, &vsrc_agc) < 0) {
      PERROR("IAV_IOC_VIN_SET_AGC");
      result = AM_RESULT_ERR_DSP;
      break;
    }
  } while (0);

  return result;
}

AM_RESULT AMYUVSensorVin::set_fps(AM_VIDEO_FPS fps)
{
  INFO("AMYUVSensorVin: set_fps not supported yet \n");
  return AM_RESULT_OK;
}
