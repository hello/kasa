/**
 * am_amba_dsp.cpp
 *
 * History:
 *    2015/07/31 - [Zhi He] create file
 *
 * Copyright (c) 2015 Ambarella, Inc.
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

#include "am_native.h"
#include "am_native_log.h"

#include "am_internal.h"

#include "am_osal.h"

#include "am_amba_dsp.h"

#ifdef BUILD_MODULE_AMBA_DSP

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>


#ifdef BUILD_DSP_AMBA_S2L

#include "errno.h"
#include "basetypes.h"
#include "iav_ioctl.h"

#elif defined (BUILD_DSP_AMBA_S2) || defined (BUILD_DSP_AMBA_S2E)

#include "errno.h"
#include "basetypes.h"
#include "ambas_common.h"
#include "ambas_vout.h"
#include "ambas_vin.h"
#include "iav_drv.h"

#elif defined (BUILD_DSP_AMBA_S3L)

#include "errno.h"
#include "basetypes.h"
#include "iav_ioctl.h"

#elif defined (BUILD_DSP_AMBA_S5)

#include "errno.h"
#include "basetypes.h"
#include "iav_ioctl.h"

#elif defined (BUILD_DSP_AMBA_S5L)

#include "errno.h"
#include "basetypes.h"
#include "iav_ioctl.h"

#endif

#include "am_linux_device_lcd.h"

#if 0
static void __parse_fps(TU32 fps_q9, SAmbaDSPVinInfo *vininfo)
{
  switch (fps_q9) {
    case AMBA_VIDEO_FPS_AUTO:
      vininfo->fps = 30;
      vininfo->fr_num = 90000;
      vininfo->fr_den = 3003;
      LOG_CONFIGURATION("auto fps, set 29.97 as default\n");
      break;
    case AMBA_VIDEO_FPS_29_97:
      vininfo->fps = 30;
      vininfo->fr_num = 90000;
      vininfo->fr_den = 3003;
      break;
    case AMBA_VIDEO_FPS_30:
      vininfo->fps = 30;
      vininfo->fr_num = 90000;
      vininfo->fr_den = 3000;
      break;
    case AMBA_VIDEO_FPS_59_94:
      vininfo->fps = 60;
      vininfo->fr_num = 90000;
      vininfo->fr_den = 1501;
      break;
    case AMBA_VIDEO_FPS_60:
      vininfo->fps = 60;
      vininfo->fr_num = 90000;
      vininfo->fr_den = 1500;
      break;
    case AMBA_VIDEO_FPS_120:
      vininfo->fps = 120;
      vininfo->fr_num = 90000;
      vininfo->fr_den = 750;
      break;
    case AMBA_VIDEO_FPS_23_976:
      vininfo->fps = 24;
      vininfo->fr_num = 90000;
      vininfo->fr_den = 3754;
      break;
    case AMBA_VIDEO_FPS_24:
      vininfo->fps = 24;
      vininfo->fr_num = 90000;
      vininfo->fr_den = 3750;
      break;
    case AMBA_VIDEO_FPS_25:
      vininfo->fps = 25;
      vininfo->fr_num = 90000;
      vininfo->fr_den = 3600;
      break;
    case AMBA_VIDEO_FPS_20:
      vininfo->fps = 20;
      vininfo->fr_num = 90000;
      vininfo->fr_den = 4500;
      break;
    case AMBA_VIDEO_FPS_15:
      vininfo->fps = 15;
      vininfo->fr_num = 90000;
      vininfo->fr_den = 6000;
      break;
    case AMBA_VIDEO_FPS_12:
      vininfo->fps = 12;
      vininfo->fr_num = 90000;
      vininfo->fr_den = 7500;
      break;
    case AMBA_VIDEO_FPS_10:
      vininfo->fps = 10;
      vininfo->fr_num = 90000;
      vininfo->fr_den = 9000;
      break;
    case AMBA_VIDEO_FPS_6:
      vininfo->fps = 6;
      vininfo->fr_num = 90000;
      vininfo->fr_den = 15000;
      break;
    case AMBA_VIDEO_FPS_5:
      vininfo->fps = 5;
      vininfo->fr_num = 90000;
      vininfo->fr_den = 18000;
      break;
    case AMBA_VIDEO_FPS_4:
      vininfo->fps = 4;
      vininfo->fr_num = 90000;
      vininfo->fr_den = 22500;
      break;
    case AMBA_VIDEO_FPS_3:
      vininfo->fps = 3;
      vininfo->fr_num = 90000;
      vininfo->fr_den = 30000;
      break;
    case AMBA_VIDEO_FPS_2:
      vininfo->fps = 2;
      vininfo->fr_num = 90000;
      vininfo->fr_den = 45000;
      break;
    case AMBA_VIDEO_FPS_1:
      vininfo->fps = 1;
      vininfo->fr_num = 90000;
      vininfo->fr_den = 90000;
      break;
    default:
      vininfo->fps = (TU64) ((TU64) 512000000 + (fps_q9 >> 1)) / (TU64) (fps_q9);
      vininfo->fr_num = 90000;
      vininfo->fr_den = (float) 90000 * (float) (fps_q9) / ((float) ((TU64) 512000000 + (TU64) (fps_q9 >> 1)));
      break;
  }
  return;
}

#else

static void __parse_fps(TU32 fps_q9, SAmbaDSPVinInfo *vininfo)
{
  switch(fps_q9) {
    case AMBA_VIDEO_FPS_AUTO:
      vininfo->fps = 30;
      vininfo->fr_num = 90000;
      vininfo->fr_den = 3003;
      break;
    case AMBA_VIDEO_FPS_29_97:
      vininfo->fps = 30;
      vininfo->fr_num = 90000;
      vininfo->fr_den = 3003;
      break;
    case AMBA_VIDEO_FPS_59_94:
      vininfo->fps = 60;
      vininfo->fr_num = 90000;
      vininfo->fr_den = 1501;
      break;
    default:
      vininfo->fps = (TU64) ((TU64) 512000000 + (fps_q9 >> 1)) / (TU64) (fps_q9);
      vininfo->fr_num = 90000;
      vininfo->fr_den = (TU64) 90000 * (TU64) (fps_q9) / (512000000);
      break;
  }
}

#endif

static int __get_single_vout_info(int iav_fd, int chan, int type, SAmbaDSPVoutInfo *voutinfo)
{
  int num;
  int sink_type = 0;
  int i;
  struct amba_vout_sink_info  sink_info;
  if (EAMDSP_VOUT_TYPE_DIGITAL == type) {
    sink_type = AMBA_VOUT_SINK_TYPE_DIGITAL;
  } else if (EAMDSP_VOUT_TYPE_HDMI == type) {
    sink_type = AMBA_VOUT_SINK_TYPE_HDMI;
  } else if (EAMDSP_VOUT_TYPE_CVBS == type) {
    sink_type = AMBA_VOUT_SINK_TYPE_CVBS;
  } else {
    LOG_ERROR("not valid type %d\n", type);
    return (-1);
  }
  num = 0;
  if (ioctl(iav_fd, IAV_IOC_VOUT_GET_SINK_NUM, &num) < 0) {
    perror("IAV_IOC_VOUT_GET_SINK_NUM");
    LOG_ERROR("IAV_IOC_VOUT_GET_SINK_NUM fail, errno %d\n", errno);
    return (-2);
  }
  if (num < 1) {
    LOG_PRINTF("Please load vout driver!\n");
    return (-3);
  }
  for (i = 0; i < num; i++) {
    sink_info.id = i;
    if (ioctl(iav_fd, IAV_IOC_VOUT_GET_SINK_INFO, &sink_info) < 0) {
      perror("IAV_IOC_VOUT_GET_SINK_INFO");
      LOG_ERROR("IAV_IOC_VOUT_GET_SINK_NUM fail, errno %d\n", errno);
      return (-4);
    }
    if ((sink_info.state == AMBA_VOUT_SINK_STATE_RUNNING) && (sink_info.sink_type == sink_type) && (sink_info.source_id == chan)) {
      if (voutinfo) {
        voutinfo->sink_id = sink_info.id;
        voutinfo->source_id = sink_info.source_id;
        voutinfo->sink_type = sink_info.sink_type;
        voutinfo->width = sink_info.sink_mode.video_size.vout_width;
        voutinfo->height = sink_info.sink_mode.video_size.vout_height;
        voutinfo->offset_x = sink_info.sink_mode.video_offset.offset_x;
        voutinfo->offset_y = sink_info.sink_mode.video_offset.offset_y;
        voutinfo->rotate = sink_info.sink_mode.video_rotate;
        voutinfo->flip = sink_info.sink_mode.video_flip;
        voutinfo->mode =  sink_info.sink_mode.mode;
        return 0;
      }
    }
  }
  //LOG_NOTICE("no vout with type %d, id %d\n", type, chan);
  return (-5);
}

static int __vout_get_sink_id(int fd, int chan, int sink_type)
{
  int         num;
  int         i;
  struct amba_vout_sink_info    sink_info;
  u32         sink_id = -1;
  num = 0;
  if (ioctl(fd, IAV_IOC_VOUT_GET_SINK_NUM, &num) < 0) {
    perror("IAV_IOC_VOUT_GET_SINK_NUM");
    return -1;
  }
  if (num < 1) {
    printf("Please load vout driver!\n");
    return -1;
  }
  for (i = num - 1; i >= 0; i--) {
    sink_info.id = i;
    if (ioctl(fd, IAV_IOC_VOUT_GET_SINK_INFO, &sink_info) < 0) {
      perror("IAV_IOC_VOUT_GET_SINK_INFO");
      return -1;
    }
    //printf("sink %d is %s\n", sink_info.id, sink_info.name);
    if ((sink_info.sink_type == sink_type) &&
        (sink_info.source_id == chan)) {
      sink_id = sink_info.id;
    }
  }
  //printf("%s: %d %d, return %d\n", __func__, chan, sink_type, sink_id);
  return sink_id;
}

typedef struct {
  const char  *name;
  int mode;
  int width;
  int height;
} SVoutResolution;

SVoutResolution gsVoutResolutionList[] = {
  //Typically for Analog and HDMI
  {"480i", AMBA_VIDEO_MODE_480I, 720, 480},
  {"576i", AMBA_VIDEO_MODE_576I, 720, 576},
  {"480p", AMBA_VIDEO_MODE_D1_NTSC, 720, 480},
  {"576p", AMBA_VIDEO_MODE_D1_PAL, 720, 576},
  {"720p", AMBA_VIDEO_MODE_720P, 1280, 720},
#ifndef BUILD_DSP_AMBA_S5L
  {"720p50", AMBA_VIDEO_MODE_720P50, 1280, 720},
  {"720p30", AMBA_VIDEO_MODE_720P30, 1280, 720},
  {"720p25", AMBA_VIDEO_MODE_720P25, 1280, 720},
  {"720p24", AMBA_VIDEO_MODE_720P24, 1280, 720},
#endif
  {"1080i", AMBA_VIDEO_MODE_1080I, 1920, 1080},
#ifndef BUILD_DSP_AMBA_S5L
  {"1080i50", AMBA_VIDEO_MODE_1080I50, 1920, 1080},
  {"1080p24", AMBA_VIDEO_MODE_1080P24, 1920, 1080},
  {"1080p25", AMBA_VIDEO_MODE_1080P25, 1920, 1080},
  {"1080p30", AMBA_VIDEO_MODE_1080P30, 1920, 1080},
#endif
  {"1080p", AMBA_VIDEO_MODE_1080P, 1920, 1080},
#ifndef BUILD_DSP_AMBA_S5L
  {"1080p50", AMBA_VIDEO_MODE_1080P50, 1920, 1080},
  {"native", AMBA_VIDEO_MODE_HDMI_NATIVE, 0, 0},
  {"2160p30", AMBA_VIDEO_MODE_2160P30, 3840, 2160},
  {"2160p25", AMBA_VIDEO_MODE_2160P25, 3840, 2160},
  {"2160p24", AMBA_VIDEO_MODE_2160P24, 3840, 2160},
  {"2160p24se", AMBA_VIDEO_MODE_2160P24_SE, 4096, 2160},
#endif
  // Typically for LCD
  {"D480I", AMBA_VIDEO_MODE_480I, 720, 480},
  {"D576I", AMBA_VIDEO_MODE_576I, 720, 576},
  {"D480P", AMBA_VIDEO_MODE_D1_NTSC, 720, 480},
  {"D576P", AMBA_VIDEO_MODE_D1_PAL, 720, 576},
  {"D720P", AMBA_VIDEO_MODE_720P, 1280, 720},
#ifndef BUILD_DSP_AMBA_S5L
  {"D720P50", AMBA_VIDEO_MODE_720P50, 1280, 720},
#endif
  {"D1080I", AMBA_VIDEO_MODE_1080I, 1920, 1080},
#ifndef BUILD_DSP_AMBA_S5L
  {"D1080I50", AMBA_VIDEO_MODE_1080I50, 1920, 1080},
  {"D1080P24", AMBA_VIDEO_MODE_1080P24, 1920, 1080},
  {"D1080P25", AMBA_VIDEO_MODE_1080P25, 1920, 1080},
  {"D1080P30", AMBA_VIDEO_MODE_1080P30, 1920, 1080},
#endif
  {"D1080P", AMBA_VIDEO_MODE_1080P, 1920, 1080},
#ifndef BUILD_DSP_AMBA_S5L
  {"D1080P50", AMBA_VIDEO_MODE_1080P50, 1920, 1080},
#endif
  {"D960x240", AMBA_VIDEO_MODE_960_240, 960, 240},    //AUO27
  {"D320x240", AMBA_VIDEO_MODE_320_240, 320, 240},    //AUO27
  {"D320x288", AMBA_VIDEO_MODE_320_288, 320, 288},    //AUO27
  {"D360x240", AMBA_VIDEO_MODE_360_240, 360, 240},    //AUO27
  {"D360x288", AMBA_VIDEO_MODE_360_288, 360, 288},    //AUO27
  {"D480x640", AMBA_VIDEO_MODE_480_640, 480, 640},    //P28K
  {"D480x800", AMBA_VIDEO_MODE_480_800, 480, 800},    //TPO648
  {"hvga", AMBA_VIDEO_MODE_HVGA, 320, 480},   //TPO489
  {"vga",  AMBA_VIDEO_MODE_VGA, 640, 480},
  {"wvga", AMBA_VIDEO_MODE_WVGA,   800, 480}, //TD043
#ifndef BUILD_DSP_AMBA_S5L
  {"D240x320", AMBA_VIDEO_MODE_240_320, 240, 320},    //ST7789V
#endif
  {"D240x400", AMBA_VIDEO_MODE_240_400, 240, 400},    //WDF2440
  {"xga",  AMBA_VIDEO_MODE_XGA, 1024, 768},   //EJ080NA
  {"wsvga", AMBA_VIDEO_MODE_WSVGA, 1024, 600},    //AT070TNA2
  {"D960x540", AMBA_VIDEO_MODE_960_540, 960, 540},    //E330QHD
};

SVoutResolution *__find_vout_mode_res(const char *name)
{
  TU32 i;
  for (i = 0; i < sizeof(gsVoutResolutionList) / sizeof(gsVoutResolutionList[0]); i++) {
    if (strcmp(gsVoutResolutionList[i].name, name) == 0) {
      return &gsVoutResolutionList[i];
    }
  }
  LOG_ERROR("vout resolution '%s' not found\n", name);
  return NULL;
}

void gfPrintAvailableVideoOutputMode()
{
  TU32 i;
  printf("Available video output mode:\n");
  for (i = 0; i < sizeof(gsVoutResolutionList) / sizeof(gsVoutResolutionList[0]); i++) {
    printf("\t%s:\t\t%dx%d\n", gsVoutResolutionList[i].name, gsVoutResolutionList[i].width, gsVoutResolutionList[i].height);
  }
}

enum amba_vout_sink_type __sink_type_from_string(const char *string)
{
  if (string) {
    if (!strcmp("hdmi", string)) {
      return AMBA_VOUT_SINK_TYPE_HDMI;
    } else if (!strcmp("digital", string)) {
      return AMBA_VOUT_SINK_TYPE_DIGITAL;
    } else if (!strcmp("cvbs", string)) {
      return AMBA_VOUT_SINK_TYPE_CVBS;
    } else if (!strcmp("svideo", string)) {
      return AMBA_VOUT_SINK_TYPE_SVIDEO;
    } else if (!strcmp("ypbpr", string)) {
      return AMBA_VOUT_SINK_TYPE_YPBPR;
    } else if (!strcmp("mipi", string)) {
      return AMBA_VOUT_SINK_TYPE_MIPI;
    } else {
      LOG_ERROR("not known vout sink type: %s\n", string);
    }
  } else {
    LOG_ERROR("NULL string\n");
  }
  return AMBA_VOUT_SINK_TYPE_AUTO;
}

static int __configure_vout(int iav_fd, SVideoOutputConfigure *vout_config)
{
  //external devices
  enum amba_vout_sink_type sink_type = AMBA_VOUT_SINK_TYPE_AUTO;
  SLCDModel *lcd = NULL;
  SVoutResolution *v_mode_res = NULL;
  //Options related to the whole vout
  int csc_en = 1, fps = AMBA_VIDEO_FPS_AUTO, qt = 0;
  enum amba_vout_hdmi_color_space hdmi_cs = AMBA_VOUT_HDMI_CS_AUTO;
#if (!defined (BUILD_DSP_AMBA_S2)) && (!defined (BUILD_DSP_AMBA_S2E))
  enum ddd_structure hdmi_3d_structure = DDD_RESERVED;
#else
  ddd_structure_t hdmi_3d_structure = DDD_RESERVED;
#endif
  enum amba_vout_hdmi_overscan hdmi_overscan = AMBA_VOUT_HDMI_OVERSCAN_AUTO;
  enum amba_vout_display_input display_input = AMBA_VOUT_INPUT_FROM_MIXER;
  enum amba_vout_mixer_csc mixer_csc = AMBA_VOUT_MIXER_ENABLE_FOR_OSD;
  //Options related to Video only
  int video_en = 1;
  enum amba_vout_flip_info video_flip = AMBA_VOUT_FLIP_NORMAL;
  enum amba_vout_rotate_info video_rotate = AMBA_VOUT_ROTATE_NORMAL;
  struct amba_vout_video_size video_size;
  struct amba_vout_video_offset video_offset;
  //Options related to OSD only
  int fb_id = -2;
  enum amba_vout_flip_info osd_flip = AMBA_VOUT_FLIP_NORMAL;
  struct amba_vout_osd_rescale osd_rescale;
  struct amba_vout_osd_offset osd_offset;
  //For sink config
  int sink_id = -1;
  struct amba_video_sink_mode sink_cfg;
  memset(&video_size, 0x0, sizeof(video_size));
  memset(&video_offset, 0x0, sizeof(video_offset));
  memset(&osd_rescale, 0x0, sizeof(osd_rescale));
  memset(&osd_offset, 0x0, sizeof(osd_offset));
  if (vout_config->mode_string) {
    v_mode_res = __find_vout_mode_res(vout_config->mode_string);
    if (!v_mode_res) {
      LOG_ERROR("not reconized video mode: %s\n", vout_config->mode_string);
      return (-1);
    }
    video_size.specified = 1;
    video_size.video_width = v_mode_res->width;
    video_size.video_height = v_mode_res->height;
    video_size.vout_width = v_mode_res->width;
    video_size.vout_height = v_mode_res->height;
    video_offset.specified = 1;
    video_offset.offset_x = 0;
    video_offset.offset_y = 0;
  } else {
    LOG_FATAL("NULL mode string\n");
    return (-2);
  }
  if (vout_config->sink_type_string) {
    sink_type = __sink_type_from_string(vout_config->sink_type_string);
    if (AMBA_VOUT_SINK_TYPE_AUTO == sink_type) {
      LOG_ERROR("not reconized sink type: %s\n", vout_config->sink_type_string);
      return (-1);
    }
  } else {
    LOG_FATAL("NULL sink type string\n");
    return (-2);
  }
  sink_id = __vout_get_sink_id(iav_fd, vout_config->vout_id, sink_type);
  if (ioctl(iav_fd, IAV_IOC_VOUT_SELECT_DEV, sink_id) < 0) {
    perror("IAV_IOC_VOUT_SELECT_DEV");
    switch (sink_type) {
      case AMBA_VOUT_SINK_TYPE_CVBS:
        LOG_ERROR("No CVBS sink, or driver not loaded\n");
        break;
      case AMBA_VOUT_SINK_TYPE_YPBPR:
        LOG_ERROR("No YPBPR sink, or driver not loaded\n");
        break;
      case AMBA_VOUT_SINK_TYPE_DIGITAL:
        LOG_ERROR("No DIGITAL sink, or driver not loaded\n");
        break;
      case AMBA_VOUT_SINK_TYPE_HDMI:
        LOG_ERROR("No HDMI sink, or driver not loaded\n");
        break;
      default:
        LOG_ERROR("not known sink type %d\n", sink_type);
        break;
    }
    return -1;
  }
  // update display input
  if (!vout_config->b_config_mixer) {
#if (!defined (BUILD_DSP_AMBA_S2)) && (!defined (BUILD_DSP_AMBA_S2E))
    struct iav_system_resource resource;
    memset(&resource, 0, sizeof(resource));
    resource.encode_mode = DSP_CURRENT_MODE;
    if (ioctl(iav_fd, IAV_IOC_GET_SYSTEM_RESOURCE, &resource) < 0) {
      perror("IAV_IOC_GET_SYSTEM_RESOURCE");
      return -1;
    }
    if (vout_config->vout_id == 0) {
      display_input = resource.mixer_a_enable ?
                      AMBA_VOUT_INPUT_FROM_MIXER : AMBA_VOUT_INPUT_FROM_SMEM;
    } else if (vout_config->vout_id == 1) {
      display_input = resource.mixer_b_enable ?
                      AMBA_VOUT_INPUT_FROM_MIXER : AMBA_VOUT_INPUT_FROM_SMEM;
    }
#else
    if (vout_config->mixer_flag) {
      display_input = AMBA_VOUT_INPUT_FROM_MIXER;
    } else {
      display_input = AMBA_VOUT_INPUT_FROM_SMEM;
    }
#endif
  } else {
    if (vout_config->mixer_flag) {
      display_input = AMBA_VOUT_INPUT_FROM_MIXER;
    } else {
      display_input = AMBA_VOUT_INPUT_FROM_SMEM;
    }
  }
  //Configure vout
  memset(&sink_cfg, 0, sizeof(sink_cfg));
  if (sink_type == AMBA_VOUT_SINK_TYPE_DIGITAL) {
    if (vout_config->device_string) {
      lcd = gfFindLCDFromName(vout_config->device_string);
      if (!lcd) {
        LOG_FATAL("do not find LCD device from name, %s\n", vout_config->device_string);
        return -1;
      }
    } else {
      LOG_FATAL("must specify LCD model name\n");
      return -1;
    }
    if (lcd->lcd_setmode(v_mode_res->mode, &sink_cfg) < 0) {
      perror("lcd_setmode fail.");
      return -1;
    }
  } else {
    sink_cfg.mode = v_mode_res->mode;
    sink_cfg.ratio = AMBA_VIDEO_RATIO_AUTO;
    sink_cfg.bits = AMBA_VIDEO_BITS_AUTO;
    sink_cfg.type = sink_type;
    if (v_mode_res->mode == AMBA_VIDEO_MODE_480I || v_mode_res->mode == AMBA_VIDEO_MODE_576I
        || v_mode_res->mode == AMBA_VIDEO_MODE_1080I
#ifndef BUILD_DSP_AMBA_S5L
        || v_mode_res->mode == AMBA_VIDEO_MODE_1080I50
#endif
       ) {
      sink_cfg.format = AMBA_VIDEO_FORMAT_INTERLACE;
    } else {
      sink_cfg.format = AMBA_VIDEO_FORMAT_PROGRESSIVE;
    }
    sink_cfg.sink_type = AMBA_VOUT_SINK_TYPE_AUTO;
    sink_cfg.bg_color.y = 0x10;
    sink_cfg.bg_color.cb = 0x80;
    sink_cfg.bg_color.cr = 0x80;
    sink_cfg.lcd_cfg.mode = AMBA_VOUT_LCD_MODE_DISABLE;
  }
  if (!csc_en) {
    sink_cfg.bg_color.y = 0;
    sink_cfg.bg_color.cb  = 0;
    sink_cfg.bg_color.cr  = 0;
  }
  if (qt) {
    sink_cfg.osd_tailor = (enum amba_vout_tailored_info) ((int) AMBA_VOUT_OSD_NO_CSC | (int)AMBA_VOUT_OSD_AUTO_COPY);
  }
  sink_cfg.id = sink_id;
  sink_cfg.frame_rate = fps;
  sink_cfg.csc_en = csc_en;
  sink_cfg.hdmi_color_space = hdmi_cs;
  sink_cfg.hdmi_3d_structure = hdmi_3d_structure;
  sink_cfg.hdmi_overscan = hdmi_overscan;
  sink_cfg.video_en = video_en;
  sink_cfg.mixer_csc = mixer_csc;
  sink_cfg.video_flip = video_flip;
  sink_cfg.video_rotate = video_rotate;
  sink_cfg.video_size = video_size;
  sink_cfg.video_offset = video_offset;
  sink_cfg.fb_id = fb_id;
  sink_cfg.osd_flip = osd_flip;
  sink_cfg.osd_rotate = AMBA_VOUT_ROTATE_NORMAL;
  sink_cfg.osd_rescale = osd_rescale;
  sink_cfg.osd_offset = osd_offset;
  sink_cfg.display_input = display_input;
  sink_cfg.direct_to_dsp = vout_config->b_direct_2_dsp;
  if (ioctl(iav_fd, IAV_IOC_VOUT_CONFIGURE_SINK, &sink_cfg) < 0) {
    perror("IAV_IOC_VOUT_CONFIGURE_SINK");
    return -1;
  }
  return 0;
}

static int __is_vout_alive(int iav_fd, int chan)
{
  int num = 0;
  int i;
  struct amba_vout_sink_info  sink_info;
  if (ioctl(iav_fd, IAV_IOC_VOUT_GET_SINK_NUM, &num) < 0) {
    perror("IAV_IOC_VOUT_GET_SINK_NUM");
    LOG_ERROR("IAV_IOC_VOUT_GET_SINK_NUM fail, errno %d\n", errno);
    return 0;
  }
  if (num < 1) {
    LOG_PRINTF("Please load vout driver!\n");
    return 0;
  }
  for (i = 0; i < num; i++) {
    sink_info.id = i;
    if (ioctl(iav_fd, IAV_IOC_VOUT_GET_SINK_INFO, &sink_info) < 0) {
      perror("IAV_IOC_VOUT_GET_SINK_INFO");
      LOG_ERROR("IAV_IOC_VOUT_GET_SINK_NUM fail, errno %d\n", errno);
      return 0;
    }
    if ((sink_info.state == AMBA_VOUT_SINK_STATE_RUNNING) && (sink_info.source_id == chan)) {
      return 1;
    }
  }
  return 0;
}

static int __halt_vout(int iav_fd, int vout_id)
{
  if (ioctl(iav_fd, IAV_IOC_VOUT_HALT, vout_id) < 0) {
    perror("IAV_IOC_VOUT_HALT");
    return -1;
  }
  return 0;
}

#ifdef BUILD_DSP_AMBA_S2L

static TInt __s2l_get_dsp_mode(TInt iav_fd, SAmbaDSPMode *mode)
{
  int state = 0;
  int ret = 0;
  ret = ioctl(iav_fd, IAV_IOC_GET_IAV_STATE, &state);
  if (0 > ret) {
    perror("IAV_IOC_GET_IAV_STATE");
    LOG_FATAL("IAV_IOC_GET_IAV_STATE fail, errno %d\n", errno);
    return ret;
  }
  switch (state) {
    case IAV_STATE_INIT:
      mode->dsp_mode = EAMDSP_MODE_INIT;
      break;
    case IAV_STATE_IDLE:
      mode->dsp_mode = EAMDSP_MODE_IDLE;
      break;
    case IAV_STATE_PREVIEW:
      mode->dsp_mode = EAMDSP_MODE_PREVIEW;
      break;
    case IAV_STATE_ENCODING:
      mode->dsp_mode = EAMDSP_MODE_ENCODE;
      break;
    case IAV_STATE_DECODING:
      mode->dsp_mode = EAMDSP_MODE_DECODE;
      break;
    default:
      LOG_FATAL("un expected dsp mode %d\n", state);
      mode->dsp_mode = EAMDSP_MODE_INVALID;
      break;
  }
  return 0;
}

static TInt __s2l_enter_decode_mode(TInt iav_fd, SAmbaDSPDecodeModeConfig *mode_config)
{
  struct iav_decode_mode_config decode_mode;
  TInt i = 0;
  memset(&decode_mode, 0x0, sizeof(decode_mode));
  if (mode_config->num_decoder > DAMBADSP_MAX_DECODER_NUMBER) {
    LOG_FATAL("BAD num_decoder %d\n", mode_config->num_decoder);
    return (-100);
  }
  decode_mode.num_decoder = mode_config->num_decoder;
  decode_mode.max_frm_width = mode_config->max_frm_width;
  decode_mode.max_frm_height = mode_config->max_frm_height;
  decode_mode.max_vout0_width = mode_config->max_vout0_width;
  decode_mode.max_vout0_height = mode_config->max_vout0_height;
  decode_mode.max_vout1_width = mode_config->max_vout1_width;
  decode_mode.max_vout1_height = mode_config->max_vout1_height;
  decode_mode.b_support_ff_fb_bw = mode_config->b_support_ff_fb_bw;
  decode_mode.debug_max_frame_per_interrupt = mode_config->debug_max_frame_per_interrupt;
  decode_mode.debug_use_dproc = mode_config->debug_use_dproc;
  i = ioctl(iav_fd, IAV_IOC_ENTER_DECODE_MODE, &decode_mode);
  if (0 > i) {
    perror("IAV_IOC_ENTER_DECODE_MODE");
    LOG_FATAL("enter decode mode fail, errno %d\n", errno);
    return i;
  }
  return 0;
}

static TInt __s2l_leave_decode_mode(TInt iav_fd)
{
  TInt ret = ioctl(iav_fd, IAV_IOC_LEAVE_DECODE_MODE);
  if (0 > ret) {
    perror("IAV_IOC_LEAVE_DECODE_MODE");
    LOG_FATAL("leave decode mode fail, errno %d\n", errno);
  }
  return ret;
}

static TInt __s2l_create_decoder(TInt iav_fd, SAmbaDSPDecoderInfo *p_decoder_info)
{
  TInt i = 0;
  struct iav_decoder_info decoder_info;
  memset(&decoder_info, 0x0, sizeof(decoder_info));
  decoder_info.decoder_id = p_decoder_info->decoder_id;
  if (EAMDSP_VIDEO_CODEC_TYPE_H264 == p_decoder_info->decoder_type) {
    decoder_info.decoder_type = IAV_DECODER_TYPE_H264;
  } else if (EAMDSP_VIDEO_CODEC_TYPE_H265 == p_decoder_info->decoder_type) {
    decoder_info.decoder_type = IAV_DECODER_TYPE_H265;
  } else {
    LOG_FATAL("bad video codec type %d\n", p_decoder_info->decoder_type);
    return (-101);
  }
  decoder_info.num_vout = p_decoder_info->num_vout;
  decoder_info.width = p_decoder_info->width;
  decoder_info.height = p_decoder_info->height;
  if (decoder_info.num_vout > DAMBADSP_MAX_VOUT_NUMBER) {
    LOG_FATAL("BAD num_vout %d\n", p_decoder_info->num_vout);
    return (-100);
  }
  for (i = 0; i < decoder_info.num_vout; i ++) {
    decoder_info.vout_configs[i].vout_id = p_decoder_info->vout_configs[i].vout_id;
    decoder_info.vout_configs[i].enable = p_decoder_info->vout_configs[i].enable;
    decoder_info.vout_configs[i].flip = p_decoder_info->vout_configs[i].flip;
    decoder_info.vout_configs[i].rotate = p_decoder_info->vout_configs[i].rotate;
    decoder_info.vout_configs[i].target_win_offset_x = p_decoder_info->vout_configs[i].target_win_offset_x;
    decoder_info.vout_configs[i].target_win_offset_y = p_decoder_info->vout_configs[i].target_win_offset_y;
    decoder_info.vout_configs[i].target_win_width = p_decoder_info->vout_configs[i].target_win_width;
    decoder_info.vout_configs[i].target_win_height = p_decoder_info->vout_configs[i].target_win_height;
    decoder_info.vout_configs[i].zoom_factor_x = p_decoder_info->vout_configs[i].zoom_factor_x;
    decoder_info.vout_configs[i].zoom_factor_y = p_decoder_info->vout_configs[i].zoom_factor_y;
    decoder_info.vout_configs[i].vout_mode = p_decoder_info->vout_configs[i].vout_mode;
    LOG_PRINTF("vout(%d), w %d, h %d, zoom x %08x, y %08x\n", i, decoder_info.vout_configs[i].target_win_width, decoder_info.vout_configs[i].target_win_height, decoder_info.vout_configs[i].zoom_factor_x, decoder_info.vout_configs[i].zoom_factor_y);
  }
  decoder_info.bsb_start_offset = p_decoder_info->bsb_start_offset;
  decoder_info.bsb_size = p_decoder_info->bsb_size;
  i = ioctl(iav_fd, IAV_IOC_CREATE_DECODER, &decoder_info);
  if (0 > i) {
    perror("IAV_IOC_CREATE_DECODER");
    LOG_FATAL("create decoder fail, errno %d\n", errno);
    return i;
  }
  p_decoder_info->bsb_start_offset = decoder_info.bsb_start_offset;
  p_decoder_info->bsb_size = decoder_info.bsb_size;
  return 0;
}

static TInt __s2l_destroy_decoder(TInt iav_fd, TU8 decoder_id)
{
  TInt ret = ioctl(iav_fd, IAV_IOC_DESTROY_DECODER, decoder_id);
  if (0 > ret) {
    perror("IAV_IOC_DESTROY_DECODER");
    LOG_FATAL("destroy decoder fail, errno %d\n", errno);
  }
  return ret;
}

static TInt __s2l_query_decode_config(int iav_fd, SAmbaDSPQueryDecodeConfig *config)
{
  if (config) {
    config->auto_map_bsb = 0;
    config->rendering_monitor_mode = 0;
  } else {
    LOG_FATAL("NULL\n");
    return (-1);
  }
  return 0;
}

static int __s2l_decode_trick_play(int iav_fd, TU8 decoder_id, TU8 trick_play)
{
  int ret;
  struct iav_decode_trick_play trickplay;
  trickplay.decoder_id = decoder_id;
  trickplay.trick_play = trick_play;
  ret = ioctl(iav_fd, IAV_IOC_DECODE_TRICK_PLAY, &trickplay);
  if (0 > ret) {
    if (EACCES == errno) {
      LOG_NOTICE("stopped\n");
      return DDECODER_STOPPED;
    }
    perror("IAV_IOC_DECODE_TRICK_PLAY");
    LOG_ERROR("trickplay error, errno %d\n", errno);
    return ret;
  }
  return 0;
}

static int __s2l_decode_start(int iav_fd, TU8 decoder_id)
{
  int ret = ioctl(iav_fd, IAV_IOC_DECODE_START, decoder_id);
  if (ret < 0) {
    perror("IAV_IOC_DECODE_START");
    LOG_ERROR("decode start error, errno %d\n", errno);
    return ret;
  }
  return 0;
}

static int __s2l_decode_stop(int iav_fd, TU8 decoder_id, TU8 stop_flag)
{
  int ret;
  struct iav_decode_stop stop;
  stop.decoder_id = decoder_id;
  stop.stop_flag = stop_flag;
  ret = ioctl(iav_fd, IAV_IOC_DECODE_STOP, &stop);
  if (0 > ret) {
    perror("IAV_IOC_DECODE_STOP");
    LOG_ERROR("decode stop error, errno %d\n", errno);
    return ret;
  }
  return 0;
}

static int __s2l_decode_speed(int iav_fd, TU8 decoder_id, TU16 speed, TU8 scan_mode, TU8 direction)
{
  int ret;
  struct iav_decode_speed spd;
  spd.decoder_id = decoder_id;
  spd.direction = direction;
  spd.speed = speed;
  spd.scan_mode = scan_mode;
  //LOG_PRINTF("speed, direction %d, speed %x, scan mode %d\n", spd.direction, spd.speed, spd.scan_mode);
  ret = ioctl(iav_fd, IAV_IOC_DECODE_SPEED, &spd);
  if (0 > ret) {
    if (EACCES == errno) {
      LOG_NOTICE("stopped\n");
      return DDECODER_STOPPED;
    }
    perror("IAV_IOC_DECODE_SPEED");
    LOG_ERROR("decode speed error, errno %d\n", errno);
    return ret;
  }
  return 0;
}

static int __s2l_decode_request_bits_fifo(int iav_fd, int decoder_id, TU32 size, void *cur_pos_offset)
{
  struct iav_decode_bsb wait;
  int ret;
  wait.decoder_id = decoder_id;
  wait.room = size;
  wait.start_offset = (TU32) cur_pos_offset;
  ret = ioctl(iav_fd, IAV_IOC_WAIT_DECODE_BSB, &wait);
  if (0 > ret) {
    if (EACCES == errno) {
      LOG_NOTICE("stopped\n");
      return DDECODER_STOPPED;
    }
    LOG_ERROR("IAV_IOC_WAIT_DECODE_BSB fail, errno %d.\n", errno);
    perror("IAV_IOC_WAIT_DECODE_BSB");
    return ret;
  }
  return 0;
}

static int __s2l_decode(int iav_fd, SAmbaDSPDecode *dec)
{
  TInt ret = 0;
  struct iav_decode_video decode_video;
  memset(&decode_video, 0, sizeof(decode_video));
  decode_video.decoder_id = dec->decoder_id;
  decode_video.num_frames = dec->num_frames;
  decode_video.start_ptr_offset = dec->start_ptr_offset;
  decode_video.end_ptr_offset = dec->end_ptr_offset;
  decode_video.first_frame_display = dec->first_frame_display;
  ret = ioctl(iav_fd, IAV_IOC_DECODE_VIDEO, &decode_video);
  if (0 > ret) {
    if (EACCES == errno) {
      LOG_NOTICE("stopped\n");
      return DDECODER_STOPPED;
    }
    perror("IAV_IOC_DECODE_VIDEO");
    LOG_ERROR("IAV_IOC_DECODE_VIDEO fail, errno %d.\n", errno);
    return ret;
  }
  return 0;
}

static int __s2l_decode_query_bsb_status_and_print(int iav_fd, TU8 decoder_id)
{
  int ret;
  struct iav_decode_bsb bsb;
  memset(&bsb, 0x0, sizeof(bsb));
  bsb.decoder_id = decoder_id;
  ret = ioctl(iav_fd, IAV_IOC_QUERY_DECODE_BSB, &bsb);
  if (0 > ret) {
    if (EACCES == errno) {
      LOG_NOTICE("stopped\n");
      return DDECODER_STOPPED;
    }
    LOG_ERROR("IAV_IOC_QUERY_DECODE_BSB fail, errno %d.\n", errno);
    perror("IAV_IOC_QUERY_DECODE_BSB");
    return ret;
  }
  LOG_PRINTF("[bsb]: current write offset (arm) 0x%08x, current read offset (dsp) 0x%08x, safe room (minus 256 bytes) %d, free room %d\n", bsb.start_offset, bsb.dsp_read_offset, bsb.room, bsb.free_room);
  return 0;
}

static int __s2l_decode_query_status_and_print(int iav_fd, TU8 decoder_id)
{
  int ret;
  struct iav_decode_status status;
  memset(&status, 0x0, sizeof(status));
  status.decoder_id = decoder_id;
  ret = ioctl(iav_fd, IAV_IOC_QUERY_DECODE_STATUS, &status);
  if (0 > ret) {
    if (EACCES == errno) {
      LOG_NOTICE("stopped\n");
      return DDECODER_STOPPED;
    }
    perror("IAV_IOC_QUERY_DECODE_STATUS");
    LOG_ERROR("IAV_IOC_QUERY_DECODE_STATUS fail, errno %d.\n", errno);
    return ret;
  }
  LOG_PRINTF("[decode status]: decode_state %d, decoded_pic_number %d, error_status %d, total_error_count %d, irq_count %d\n", status.decode_state, status.decoded_pic_number, status.error_status, status.total_error_count, status.irq_count);
  LOG_PRINTF("[decode status, bsb]: current write offset (arm) 0x%08x, current read offset (dsp) 0x%08x, safe room (minus 256 bytes) %d, free room %d\n", status.write_offset, status.dsp_read_offset, status.room, status.free_room);
  LOG_PRINTF("[decode status, last pts]: %d, is_started %d, is_send_stop_cmd %d\n", status.last_pts, status.is_started, status.is_send_stop_cmd);
  LOG_PRINTF("[decode status, yuv addr]: yuv422_y 0x%08x, yuv422_uv 0x%08x\n", status.yuv422_y_addr, status.yuv422_uv_addr);
  return 0;
}

static int __s2l_decode_query_bsb_status(int iav_fd, SAmbaDSPBSBStatus *status)
{
  int ret;
  struct iav_decode_bsb bsb;
  memset(&bsb, 0x0, sizeof(bsb));
  bsb.decoder_id = status->decoder_id;
  ret = ioctl(iav_fd, IAV_IOC_QUERY_DECODE_BSB, &bsb);
  if (0 > ret) {
    if (EACCES == errno) {
      LOG_NOTICE("stopped\n");
      return DDECODER_STOPPED;
    }
    LOG_ERROR("IAV_IOC_QUERY_DECODE_BSB fail, errno %d.\n", errno);
    perror("IAV_IOC_QUERY_DECODE_BSB");
    return ret;
  }
  status->start_offset = bsb.start_offset;
  status->room = bsb.room;
  status->dsp_read_offset = bsb.dsp_read_offset;
  status->free_room = bsb.free_room;
  return 0;
}

static int __s2l_decode_query_status(int iav_fd, SAmbaDSPDecodeStatus *status)
{
  int ret;
  struct iav_decode_status dec_status;
  memset(&dec_status, 0x0, sizeof(dec_status));
  dec_status.decoder_id = status->decoder_id;
  ret = ioctl(iav_fd, IAV_IOC_QUERY_DECODE_STATUS, &dec_status);
  if (0 > ret) {
    if (EACCES == errno) {
      LOG_NOTICE("stopped\n");
      return DDECODER_STOPPED;
    }
    perror("IAV_IOC_QUERY_DECODE_STATUS");
    LOG_ERROR("IAV_IOC_QUERY_DECODE_STATUS fail, errno %d.\n", errno);
    return ret;
  }
  status->is_started = dec_status.is_started;
  status->is_send_stop_cmd = dec_status.is_send_stop_cmd;
  status->last_pts = dec_status.last_pts;
  status->decode_state = dec_status.decode_state;
  status->error_status = dec_status.error_status;
  status->total_error_count = dec_status.total_error_count;
  status->decoded_pic_number = dec_status.decoded_pic_number;
  status->write_offset = dec_status.write_offset;
  status->room = dec_status.room;
  status->dsp_read_offset = dec_status.dsp_read_offset;
  status->free_room = dec_status.free_room;
  status->irq_count = dec_status.irq_count;
  status->yuv422_y_addr = dec_status.yuv422_y_addr;
  status->yuv422_uv_addr = dec_status.yuv422_uv_addr;
  return 0;
}

static int __s2l_get_vin_info(int iav_fd, SAmbaDSPVinInfo *vininfo)
{
  struct vindev_video_info vin_info;
  struct vindev_fps active_fps;
  TU32 fps_q9 = 1;
  int ret = 0;
  memset(&vin_info, 0x0, sizeof(vin_info));
  vin_info.vsrc_id = 0;
  vin_info.info.mode = AMBA_VIDEO_MODE_CURRENT;
  ret = ioctl(iav_fd, IAV_IOC_VIN_GET_VIDEOINFO, &vin_info);
  if (0 > ret) {
    perror("IAV_IOC_VIN_GET_VIDEOINFO");
    LOG_WARN("IAV_IOC_VIN_GET_VIDEOINFO fail, errno %d\n", errno);
    return ret;
  }
  memset(&active_fps, 0, sizeof(active_fps));
  active_fps.vsrc_id = 0;
  ret = ioctl(iav_fd, IAV_IOC_VIN_GET_FPS, &active_fps);
  if (0 > ret) {
    perror("IAV_IOC_VIN_GET_FPS");
    LOG_WARN("IAV_IOC_VIN_GET_FPS fail, errno %d\n", errno);
    return ret;
  }
  fps_q9 = active_fps.fps;
  __parse_fps(fps_q9, vininfo);
  vininfo->width = vin_info.info.width;
  vininfo->height = vin_info.info.height;
  vininfo->format = vin_info.info.format;
  vininfo->type = vin_info.info.type;
  vininfo->bits = vin_info.info.bits;
  vininfo->ratio = vin_info.info.ratio;
  vininfo->system = vin_info.info.system;
  vininfo->flip = vin_info.info.flip;
  vininfo->rotate = vin_info.info.rotate;
  return 0;
}

static int __s2l_get_stream_framefactor(int iav_fd, int index, SAmbaDSPStreamFramefactor *framefactor)
{
  struct iav_stream_cfg streamcfg;
  int ret = 0;
  if (DMaxStreamNumber <= index) {
    LOG_FATAL("index(%d) not as expected\n", index);
    return (-10);
  }
  memset(&streamcfg, 0, sizeof(streamcfg));
  streamcfg.id = index;
  streamcfg.cid = IAV_STMCFG_FPS;
  ret = ioctl(iav_fd, IAV_IOC_GET_STREAM_CONFIG, &streamcfg);
  if (0 > ret) {
    perror("IAV_IOC_GET_STREAM_CONFIG");
    LOG_ERROR("IAV_IOC_GET_STREAM_CONFIG fail, errno %d\n", errno);
    return ret;
  }
  framefactor->framefactor_num = streamcfg.arg.fps.fps_multi;
  framefactor->framefactor_den = streamcfg.arg.fps.fps_div;
  return 0;
}

static int __s2l_map_bsb(int iav_fd, SAmbaDSPMapBSB *map_bsb)
{
  int ret = 0;
  unsigned int map_size = 0;
  struct iav_querybuf querybuf;
  querybuf.buf = IAV_BUFFER_BSB;
  ret = ioctl(iav_fd, IAV_IOC_QUERY_BUF, &querybuf);
  if (0 > ret) {
    perror("IAV_IOC_QUERY_BUF");
    LOG_ERROR("IAV_IOC_QUERY_BUF fail\n");
    return ret;
  }
  map_bsb->size = querybuf.length;
  if (map_bsb->b_two_times) {
    map_size = querybuf.length * 2;
  } else {
    map_size = querybuf.length;
  }
  if (map_bsb->b_enable_read && map_bsb->b_enable_write) {
    map_bsb->base = mmap(NULL, map_size, PROT_WRITE | PROT_READ, MAP_SHARED, iav_fd, querybuf.offset);
  } else if (map_bsb->b_enable_read && !map_bsb->b_enable_write) {
    map_bsb->base = mmap(NULL, map_size, PROT_READ, MAP_SHARED, iav_fd, querybuf.offset);
  } else if (!map_bsb->b_enable_read && map_bsb->b_enable_write) {
    map_bsb->base = mmap(NULL, map_size, PROT_WRITE, MAP_SHARED, iav_fd, querybuf.offset);
  } else {
    LOG_ERROR("not read or write\n");
    return (-1);
  }
  if (map_bsb->base == MAP_FAILED) {
    perror("mmap");
    LOG_ERROR("mmap fail, errno %d\n", errno);
    return -1;
  }
  LOG_PRINTF("[mmap]: bsb_mem = %p, size = 0x%x\n", map_bsb->base, map_bsb->size);
  return 0;
}

static int __s2l_map_dsp(int iav_fd, SAmbaDSPMapDSP *map_dsp)
{
  TInt ret = 0;
  struct iav_querybuf querybuf;
  querybuf.buf = IAV_BUFFER_DSP;
  ret = ioctl(iav_fd, IAV_IOC_QUERY_BUF, &querybuf);
  if (DUnlikely(0 > ret)) {
    perror("IAV_IOC_QUERY_BUF");
    LOG_ERROR("IAV_IOC_QUERY_BUF fail, errno %d\n", errno);
    return ret;
  }
  map_dsp->size = querybuf.length;
  map_dsp->base = mmap(NULL, map_dsp->size, PROT_READ | PROT_WRITE, MAP_SHARED, iav_fd, querybuf.offset);
  if (DUnlikely(map_dsp->base == MAP_FAILED)) {
    perror("mmap");
    LOG_ERROR("mmap fail, errno %d\n", errno);
    return -1;
  }
  LOG_PRINTF("[mmap]: dsp_mem = %p, size = 0x%x\n", map_dsp->base, map_dsp->size);
  return 0;
}

static int __s2l_unmap_bsb(int iav_fd, SAmbaDSPMapBSB *map_bsb)
{
  if (map_bsb->base && map_bsb->size) {
    int ret = 0;
    unsigned int map_size = 0;
    if (map_bsb->b_two_times) {
      map_size = map_bsb->size * 2;
    } else {
      map_size = map_bsb->size;
    }
    ret = munmap(map_bsb->base, map_size);
    LOG_PRINTF("[munmap]: bsb_mem = %p, size = 0x%x\n", map_bsb->base, map_bsb->size);
    map_bsb->base = NULL;
    map_bsb->size = 0;
    if (DUnlikely(0 > ret)) {
      perror("munmap");
      LOG_ERROR("munmap fail, errno %d\n", errno);
      return ret;
    }
  } else {
    LOG_ERROR("bad params, %p, %d\n", map_bsb->base, map_bsb->size);
    return (-1);
  }
  return 0;
}

static int __s2l_unmap_dsp(int iav_fd, SAmbaDSPMapDSP *map_dsp)
{
  if (map_dsp->base && map_dsp->size) {
    int ret = 0;
    ret = munmap(map_dsp->base, map_dsp->size);
    LOG_PRINTF("[munmap]: dsp_mem = %p, size = 0x%x\n", map_dsp->base, map_dsp->size);
    map_dsp->base = NULL;
    map_dsp->size = 0;
    if (DUnlikely(0 > ret)) {
      perror("munmap");
      LOG_ERROR("munmap fail, errno %d\n", errno);
      return ret;
    }
  } else {
    LOG_ERROR("bad params, %p, %d\n", map_dsp->base, map_dsp->size);
    return (-1);
  }
  return 0;
}

static int __s2l_efm_get_bufferpool_info(int iav_fd, SAmbaEFMPoolInfo *buffer_pool_info)
{
  TInt ret = 0;
  struct iav_efm_get_pool_info efm_pool_info;
  memset(&efm_pool_info, 0, sizeof(efm_pool_info));
  ret = ioctl(iav_fd, IAV_IOC_EFM_GET_POOL_INFO, &efm_pool_info);
  if (0 > ret) {
    perror("IAV_IOC_EFM_GET_POOL_INFO");
    LOG_ERROR("IAV_IOC_EFM_GET_POOL_INFO fail, errno %d\n", errno);
    return ret;
  }
  if (buffer_pool_info) {
    buffer_pool_info->yuv_buf_num = efm_pool_info.yuv_buf_num;
    buffer_pool_info->yuv_pitch = efm_pool_info.yuv_pitch;
    buffer_pool_info->me1_buf_num = efm_pool_info.me1_buf_num;
    buffer_pool_info->me1_pitch = efm_pool_info.me1_pitch;
    buffer_pool_info->yuv_buf_width = efm_pool_info.yuv_size.width;
    buffer_pool_info->yuv_buf_height = efm_pool_info.yuv_size.height;
    buffer_pool_info->me1_buf_width = efm_pool_info.me1_size.width;
    buffer_pool_info->me1_buf_height = efm_pool_info.me1_size.height;
    buffer_pool_info->b_use_addr = 0;
  } else {
    LOG_FATAL("NULL param\n");
    return (-2);
  }
  return 0;
}

static int __s2l_efm_request_frame(int iav_fd, SAmbaEFMFrame *frame)
{
  TInt ret = 0;
  struct iav_efm_request_frame request;
  memset(&request, 0, sizeof(request));
  ret = ioctl(iav_fd, IAV_IOC_EFM_REQUEST_FRAME, &request);
  if (0 > ret) {
    perror("IAV_IOC_EFM_REQUEST_FRAME");
    LOG_ERROR("IAV_IOC_EFM_REQUEST_FRAME fail, errno %d\n", errno);
    return (-1);
  }
  if (frame) {
    frame->frame_idx = request.frame_idx;
    frame->yuv_luma_offset = (TPointer) request.yuv_luma_offset;
    frame->yuv_chroma_offset = (TPointer) request.yuv_chroma_offset;
    frame->me1_offset = (TPointer) request.me1_offset;
  } else {
    LOG_FATAL("NULL param\n");
    return (-2);
  }
  return 0;
}

static int __s2l_efm_finish_frame(int iav_fd, SAmbaEFMFinishFrame *finish_frame)
{
  if (finish_frame) {
    TInt ret = 0;
    if (!finish_frame->b_not_wait_next_interrupt) {
      ret = ioctl(iav_fd, IAV_IOC_WAIT_NEXT_FRAME, 0);
      if (0 > ret) {
        perror("IAV_IOC_WAIT_NEXT_FRAME");
        LOG_ERROR("IAV_IOC_WAIT_NEXT_FRAME fail, errno %d\n", errno);
        return (-1);
      }
    }
    struct iav_efm_handshake_frame handshake;
    memset(&handshake, 0, sizeof(handshake));
    handshake.frame_idx = finish_frame->frame_idx;
    handshake.frame_pts = finish_frame->frame_pts;
    handshake.is_last_frame = finish_frame->is_last_frame;
    handshake.use_hw_pts = finish_frame->use_hw_pts;
    ret = ioctl(iav_fd, IAV_IOC_EFM_HANDSHAKE_FRAME, &handshake);
    if (0 > ret) {
      perror("IAV_IOC_EFM_HANDSHAKE_FRAME");
      LOG_ERROR("IAV_IOC_EFM_HANDSHAKE_FRAME fail, errno %d\n", errno);
      return (-1);
    }
  } else {
    LOG_FATAL("NULL param\n");
    return (-2);
  }
  return 0;
}

static int __s2l_read_bitstream(int iav_fd, SAmbaDSPReadBitstream *bitstream)
{
  struct iav_querydesc query_desc;
  struct iav_framedesc *frame_desc;
  int ret = 0;
  memset(&query_desc, 0, sizeof(query_desc));
  frame_desc = &query_desc.arg.frame;
  query_desc.qid = IAV_DESC_FRAME;
  //frame_desc->id = bitstream->stream_id;
  frame_desc->id = 0xffffffff;
  frame_desc->time_ms = bitstream->timeout_ms;
  ret = ioctl(iav_fd, IAV_IOC_QUERY_DESC, &query_desc);
  if (ret) {
    if (EAGAIN == errno) {
      return 1;
    }
    perror("IAV_IOC_QUERY_DESC");
    LOG_ERROR("IAV_IOC_QUERY_DESC error, errno %d\n", errno);
    return (-1);
  }
  bitstream->stream_id = frame_desc->id;
  bitstream->offset = frame_desc->data_addr_offset;
  bitstream->size = frame_desc->size;
  bitstream->pts = frame_desc->arm_pts;
  bitstream->video_width = frame_desc->reso.width;
  bitstream->video_height = frame_desc->reso.height;
  //bitstream->slice_id = frame_desc->slice_id;
  //bitstream->slice_num = frame_desc->slice_num;
  if (frame_desc->stream_end) {
    DASSERT(!bitstream->size);
    bitstream->size = 0;
    bitstream->offset = 0;
  }
  if (IAV_STREAM_TYPE_H264 == frame_desc->stream_type) {
    bitstream->stream_type = StreamFormat_H264;
  } /*else if (IAV_STREAM_TYPE_H265 == frame_desc->stream_type) {
        bitstream->stream_type = StreamFormat_H265;
    }*/ else if (IAV_STREAM_TYPE_MJPEG == frame_desc->stream_type) {
    bitstream->stream_type = StreamFormat_JPEG;
  } else {
    bitstream->stream_type = StreamFormat_Invalid;
    LOG_FATAL("bad stream type %d\n", bitstream->stream_type);
    return (-2);
  }
  return 0;
}

static int __s2l_is_ready_for_read_bitstream(int iav_fd)
{
  return 1;
}

static int __s2l_encode_start(int iav_fd, TU32 mask)
{
  int ret = ioctl(iav_fd, IAV_IOC_START_ENCODE, mask);
  if (ret) {
    perror("IAV_IOC_START_ENCODE");
    LOG_ERROR("IAV_IOC_START_ENCODE fail, errno %d, mask 0x%08x\n", errno, mask);
    return ret;
  }
  return 0;
}

static int __s2l_encode_stop(int iav_fd, TU32 mask)
{
  int ret = ioctl(iav_fd, IAV_IOC_STOP_ENCODE, mask);
  if (ret) {
    perror("IAV_IOC_STOP_ENCODE");
    LOG_ERROR("IAV_IOC_STOP_ENCODE fail, errno %d, mask 0x%08x\n", errno, mask);
    return ret;
  }
  return 0;
}

static int __s2l_query_encode_stream_info(int iav_fd, SAmbaDSPEncStreamInfo *info)
{
  struct iav_stream_info stream_info;
  int ret = 0;
  stream_info.id = info->id;
  ret = ioctl(iav_fd, IAV_IOC_GET_STREAM_INFO, &stream_info);
  if (ret) {
    perror("IAV_IOC_GET_STREAM_INFO");
    LOG_ERROR("IAV_IOC_GET_STREAM_INFO fail, errno %d\n", errno);
    return ret;
  }
  switch (stream_info.state) {
    case IAV_STREAM_STATE_IDLE:
      info->state = EAMDSP_ENC_STREAM_STATE_IDLE;
      break;
    case IAV_STREAM_STATE_STARTING:
      info->state = EAMDSP_ENC_STREAM_STATE_STARTING;
      break;
    case IAV_STREAM_STATE_ENCODING:
      info->state = EAMDSP_ENC_STREAM_STATE_ENCODING;
      break;
    case IAV_STREAM_STATE_STOPPING:
      info->state = EAMDSP_ENC_STREAM_STATE_STOPPING;
      break;
    case IAV_STREAM_STATE_UNKNOWN:
      LOG_ERROR("unknown state\n");
      info->state = EAMDSP_ENC_STREAM_STATE_UNKNOWN;
      return (-1);
      break;
    default:
      LOG_ERROR("unexpected state %d\n", stream_info.state);
      info->state = EAMDSP_ENC_STREAM_STATE_ERROR;
      return (-2);
      break;
  }
  return 0;
}

static int __s2l_query_encode_stream_format(int iav_fd, SAmbaDSPEncStreamFormat *fmt)
{
  struct iav_stream_format format;
  int ret = 0;
  memset(&format, 0, sizeof(format));
  format.id = fmt->id;
  ret = ioctl(iav_fd, IAV_IOC_GET_STREAM_FORMAT, &format);
  if (ret) {
    perror("IAV_IOC_GET_STREAM_FORMAT");
    LOG_ERROR("IAV_IOC_GET_STREAM_FORMAT fail, errno %d\n", errno);
    return ret;
  }
  switch (format.type) {
    case IAV_STREAM_TYPE_H264:
      fmt->codec = EAMDSP_VIDEO_CODEC_TYPE_H264;
      break;
    case IAV_STREAM_TYPE_MJPEG:
      fmt->codec = EAMDSP_VIDEO_CODEC_TYPE_MJPEG;
      break;
    case IAV_STREAM_TYPE_NONE:
      LOG_WARN("codec none?\n");
      fmt->codec = EAMDSP_VIDEO_CODEC_TYPE_INVALID;
      break;
    default:
      LOG_ERROR("unexpected codec %d\n", format.type);
      fmt->codec = EAMDSP_VIDEO_CODEC_TYPE_INVALID;
      return (-1);
      break;
  }
  switch (format.buf_id) {
    case IAV_SRCBUF_MN:
      fmt->source_buffer = EAMDSP_ENC_STREAM_SOURCE_BUFFER_MAIN;
      break;
    case IAV_SRCBUF_PC:
      fmt->source_buffer = EAMDSP_ENC_STREAM_SOURCE_BUFFER_SECOND;
      break;
    case IAV_SRCBUF_PB:
      fmt->source_buffer = EAMDSP_ENC_STREAM_SOURCE_BUFFER_THIRD;
      break;
    case IAV_SRCBUF_PA:
      fmt->source_buffer = EAMDSP_ENC_STREAM_SOURCE_BUFFER_FOURTH;
      break;
    case IAV_SRCBUF_EFM:
      fmt->source_buffer = EAMDSP_ENC_STREAM_SOURCE_BUFFER_EFM;
      break;
    default:
      LOG_ERROR("unexpected source buffer %d\n", format.buf_id);
      fmt->source_buffer = EAMDSP_ENC_STREAM_SOURCE_BUFFER_INVALID;
      return (-2);
      break;
  }
  return 0;
}

static int __s2l_query_source_buffer_info(int iav_fd, SAmbaDSPSourceBufferInfo *info)
{
  struct iav_srcbuf_format buf_format;
  memset(&buf_format, 0, sizeof(buf_format));
  buf_format.buf_id = info->buf_id;
  if (ioctl(iav_fd, IAV_IOC_GET_SOURCE_BUFFER_FORMAT, &buf_format) < 0) {
    perror("IAV_IOC_GET_SOURCE_BUFFER_FORMAT");
    LOG_ERROR("IAV_IOC_QUERY_DESC fail, errno %d\n", errno);
    return (-1);
  }
  info->size_width = buf_format.size.width;
  info->size_height = buf_format.size.height;
  info->crop_size_x = buf_format.input.width;
  info->crop_size_y = buf_format.input.height;
  info->crop_pos_x = buf_format.input.x;
  info->crop_pos_y = buf_format.input.y;
  return 0;
}

static int __s2l_query_yuv_buffer(int iav_fd, SAmbaDSPQueryYUVBuffer *yuv_buffer)
{
  struct iav_yuvbufdesc *yuv_desc;
  struct iav_querydesc query_desc;
  int ret = 0;
  memset(&query_desc, 0, sizeof(query_desc));
  query_desc.qid = IAV_DESC_YUV;
  query_desc.arg.yuv.buf_id = (enum iav_srcbuf_id) yuv_buffer->buf_id;
  ret = ioctl(iav_fd, IAV_IOC_QUERY_DESC, &query_desc);
  if (ret < 0) {
    perror("IAV_IOC_QUERY_DESC");
    LOG_ERROR("IAV_IOC_QUERY_DESC fail, errno %d\n", errno);
    return ret;
  }
  yuv_desc = &query_desc.arg.yuv;
  yuv_buffer->y_addr_offset = yuv_desc->y_addr_offset;
  yuv_buffer->uv_addr_offset = yuv_desc->uv_addr_offset;
  yuv_buffer->width = yuv_desc->width;
  yuv_buffer->height = yuv_desc->height;
  yuv_buffer->pitch = yuv_desc->pitch;
  yuv_buffer->seq_num = yuv_desc->seq_num;
  yuv_buffer->format = yuv_desc->format;
  yuv_buffer->dsp_pts = yuv_desc->dsp_pts;
  yuv_buffer->mono_pts = yuv_desc->mono_pts;
  return 0;
}

static u32 __translate_buffer_type(u32 type)
{
  switch (type) {
    case EAmbaBufferType_DSP:
      return IAV_BUFFER_DSP;
      break;
    case EAmbaBufferType_BSB:
      return IAV_BUFFER_BSB;
      break;
    case EAmbaBufferType_USR:
      return IAV_BUFFER_USR;
      break;
    case EAmbaBufferType_MV:
      return IAV_BUFFER_MV;
      break;
    case EAmbaBufferType_OVERLAY:
      return IAV_BUFFER_OVERLAY;
      break;
    case EAmbaBufferType_QPMATRIX:
      return IAV_BUFFER_QPMATRIX;
      break;
    case EAmbaBufferType_WARP:
      return IAV_BUFFER_WARP;
      break;
    case EAmbaBufferType_QUANT:
      return IAV_BUFFER_QUANT;
      break;
    case EAmbaBufferType_IMG:
      return IAV_BUFFER_IMG;
      break;
    case EAmbaBufferType_CMD_SYNC:
      return IAV_BUFFER_CMD_SYNC;
      break;
    case EAmbaBufferType_FB_DATA:
      return IAV_BUFFER_FB_DATA;
      break;
    case EAmbaBufferType_FB_AUDIO:
      return IAV_BUFFER_FB_AUDIO;
      break;
    default:
      LOG_FATAL("unexpected type %d\n", type);
  }
  return type;
}

static int __s2l_gdma_copy(int iav_fd, SAmbaGDMACopy *copy)
{
  TInt ret = 0;
  struct iav_gdma_copy param;
  param.src_offset = copy->src_offset;
  param.dst_offset = copy->dst_offset;
  param.src_mmap_type = __translate_buffer_type(copy->src_mmap_type);
  param.dst_mmap_type = __translate_buffer_type(copy->dst_mmap_type);
  param.src_pitch = copy->src_pitch;
  param.dst_pitch = copy->dst_pitch;
  param.width = copy->width;
  param.height = copy->height;
  ret = ioctl(iav_fd, IAV_IOC_GDMA_COPY, &param);
  if (ret < 0) {
    perror("IAV_IOC_GDMA_COPY");
    LOG_ERROR("IAV_IOC_GDMA_COPY fail, errno %d\n", errno);
    return ret;
  }
  return 0;
}

static TInt __s2l_dsp_enter_idle_mode(TInt iav_fd)
{
  TInt ret = ioctl(iav_fd, IAV_IOC_ENTER_IDLE);
  if (0 > ret) {
    perror("IAV_IOC_ENTER_IDLE");
    LOG_FATAL("enter idle mode fail, errno %d\n", errno);
  }
  return ret;
}

static void __setup_s2l_al_context(SFAmbaDSPDecAL *al)
{
  al->f_get_dsp_mode = __s2l_get_dsp_mode;
  al->f_enter_mode = __s2l_enter_decode_mode;
  al->f_leave_mode = __s2l_leave_decode_mode;
  al->f_create_decoder = __s2l_create_decoder;
  al->f_destroy_decoder = __s2l_destroy_decoder;
  al->f_query_decode_config = __s2l_query_decode_config;
  al->f_trickplay = __s2l_decode_trick_play;
  al->f_start = __s2l_decode_start;
  al->f_stop = __s2l_decode_stop;
  al->f_speed = __s2l_decode_speed;
  al->f_request_bsb = __s2l_decode_request_bits_fifo;
  al->f_decode = __s2l_decode;
  al->f_query_print_decode_bsb_status = __s2l_decode_query_bsb_status_and_print;
  al->f_query_print_decode_status = __s2l_decode_query_status_and_print;
  al->f_query_decode_bsb_status = __s2l_decode_query_bsb_status;
  al->f_query_decode_status = __s2l_decode_query_status;
  al->f_decode_wait_vout_dormant = NULL;
  al->f_decode_wake_vout = NULL;
  al->f_decode_wait_eos_flag = NULL;
  al->f_decode_wait_eos = NULL;
  al->f_configure_vout = __configure_vout;
  al->f_get_vout_info = __get_single_vout_info;
  al->f_get_vin_info = __s2l_get_vin_info;
  al->f_get_stream_framefactor = __s2l_get_stream_framefactor;
  al->f_map_bsb = __s2l_map_bsb;
  al->f_map_dsp = __s2l_map_dsp;
  al->f_map_intrapb = NULL;
  al->f_unmap_bsb = __s2l_unmap_bsb;
  al->f_unmap_dsp = __s2l_unmap_dsp;
  al->f_unmap_intrapb = NULL;
  al->f_efm_get_buffer_pool_info = __s2l_efm_get_bufferpool_info;
  al->f_efm_request_frame = __s2l_efm_request_frame;
  al->f_efm_finish_frame = __s2l_efm_finish_frame;
  al->f_read_bitstream = __s2l_read_bitstream;
  al->f_is_ready_for_read_bitstream = __s2l_is_ready_for_read_bitstream;
  al->f_intraplay_reset_buffers = NULL;
  al->f_intraplay_request_buffer = NULL;
  al->f_intraplay_decode = NULL;
  al->f_intraplay_yuv2yuv = NULL;
  al->f_intraplay_display = NULL;
  al->f_encode_start = __s2l_encode_start;
  al->f_encode_stop = __s2l_encode_stop;
  al->f_query_encode_stream_info = __s2l_query_encode_stream_info;
  al->f_query_encode_stream_fmt = __s2l_query_encode_stream_format;
  al->f_query_source_buffer_info = __s2l_query_source_buffer_info;
  al->f_query_yuv_buffer = __s2l_query_yuv_buffer;
  al->f_gdma_copy = __s2l_gdma_copy;
  al->f_enter_idle_mode = __s2l_dsp_enter_idle_mode;
}

#elif defined (BUILD_DSP_AMBA_S2) || defined (BUILD_DSP_AMBA_S2E)

static TInt __s2e_get_dsp_mode(TInt iav_fd, SAmbaDSPMode *mode)
{
  int ret = 0;
  iav_state_info_t info;
  ret = ioctl(iav_fd, IAV_IOC_GET_STATE_INFO, &info);
  if (0 > ret) {
    perror("IAV_IOC_GET_IAV_STATE");
    LOG_FATAL("IAV_IOC_GET_IAV_STATE fail, errno %d\n", errno);
    return ret;
  }
  switch (info.state) {
    case IAV_STATE_INIT:
      mode->dsp_mode = EAMDSP_MODE_INIT;
      break;
    case IAV_STATE_IDLE:
      mode->dsp_mode = EAMDSP_MODE_IDLE;
      break;
    case IAV_STATE_PREVIEW:
      mode->dsp_mode = EAMDSP_MODE_PREVIEW;
      break;
    case IAV_STATE_ENCODING:
      mode->dsp_mode = EAMDSP_MODE_ENCODE;
      break;
    case IAV_STATE_DECODING:
      mode->dsp_mode = EAMDSP_MODE_DECODE;
      break;
    default:
      LOG_FATAL("un expected dsp mode %d\n", info.state);
      mode->dsp_mode = EAMDSP_MODE_INVALID;
      break;
  }
  return 0;
}

static TInt __s2e_enter_decode_mode(TInt iav_fd, SAmbaDSPDecodeModeConfig *mode_config)
{
  TInt ret = 0;
  iav_udec_mode_config_t modeconfig;
  iav_udec_config_t udec_config;
  memset(&modeconfig, 0, sizeof(modeconfig));
  modeconfig.postp_mode = 2;
  modeconfig.enable_deint = 0;
  modeconfig.pp_chroma_fmt_max = 2;
  if (mode_config->max_vout0_width > mode_config->max_vout1_width) {
    modeconfig.pp_max_frm_width = mode_config->max_vout0_width;
  } else {
    modeconfig.pp_max_frm_width = mode_config->max_vout1_width;
  }
  if (mode_config->max_vout0_height > mode_config->max_vout1_height) {
    modeconfig.pp_max_frm_height = mode_config->max_vout0_height;
  } else {
    modeconfig.pp_max_frm_height = mode_config->max_vout1_height;
  }
  modeconfig.pp_max_frm_num = 5;
  modeconfig.vout_mask = mode_config->vout_mask;
  modeconfig.num_udecs = 1;
  modeconfig.udec_config = &udec_config;
  memset(&udec_config, 0, sizeof(udec_config));
  udec_config.tiled_mode = 5;
  udec_config.frm_chroma_fmt_max = 1;
  udec_config.dec_types = 0x17;
  if ((mode_config->max_frm_width * mode_config->max_frm_height) >= (720 * 480)) {
    udec_config.max_frm_num = 12;
  } else {
    udec_config.max_frm_num = 19;
  }
  udec_config.max_frm_width = mode_config->max_frm_width;
  udec_config.max_frm_height = mode_config->max_frm_height;
  udec_config.max_fifo_size = 16 * 1024 * 1024;
  ret = ioctl(iav_fd, IAV_IOC_ENTER_UDEC_MODE, &modeconfig);
  if (0 > ret) {
    perror("IAV_IOC_ENTER_UDEC_MODE");
    LOG_FATAL("enter udec mode fail\n");
  }
  return ret;
}

static TInt __s2e_leave_decode_mode(TInt iav_fd)
{
  TInt ret = ioctl(iav_fd, IAV_IOC_ENTER_IDLE);
  if (0 > ret) {
    perror("IAV_IOC_ENTER_IDLE");
    LOG_FATAL("enter idle mode fail, errno %d\n", errno);
  }
  return ret;
}

static TInt __s2e_create_decoder(TInt iav_fd, SAmbaDSPDecoderInfo *p_decoder_info)
{
  iav_udec_info_ex_t info;
  iav_udec_vout_config_t vout_config;
  memset(&info, 0x0, sizeof(info));
  memset(&vout_config, 0x0, sizeof(vout_config));
  info.udec_id = p_decoder_info->decoder_id;
  info.udec_type = UDEC_H264;
  info.noncachable_buffer = 1;
  info.enable_pp = 1;
  info.enable_deint = 0;
  info.enable_err_handle = 0;
  info.interlaced_out = 0;
  info.packed_out = 0;
  info.vout_configs.num_vout = 1;
  info.vout_configs.vout_config = &vout_config;
  info.vout_configs.first_pts_low = 0;
  info.vout_configs.first_pts_high = 0;
  info.vout_configs.input_center_x = p_decoder_info->width / 2;
  info.vout_configs.input_center_y = p_decoder_info->height / 2;
  info.bits_fifo_size = 16 * 1024 * 1024;
  info.ref_cache_size = 720 * 1024;
  info.u.h264.pjpeg_buf_size = 16 * 1024 * 1024;
  vout_config.vout_id = p_decoder_info->vout_configs[0].vout_id;
  vout_config.udec_id = p_decoder_info->decoder_id;
  vout_config.disable = 0;
  vout_config.flip = p_decoder_info->vout_configs[0].flip;
  vout_config.rotate = p_decoder_info->vout_configs[0].rotate;
  vout_config.target_win_offset_x = 0;
  vout_config.target_win_offset_y = 0;
  vout_config.target_win_width = p_decoder_info->vout_configs[0].target_win_width;
  vout_config.target_win_height = p_decoder_info->vout_configs[0].target_win_height;
  vout_config.win_offset_x = 0;
  vout_config.win_offset_y = 0;
  vout_config.win_width = p_decoder_info->vout_configs[0].target_win_width;
  vout_config.win_height = p_decoder_info->vout_configs[0].target_win_height;
  if (ioctl(iav_fd, IAV_IOC_INIT_UDEC, &info) < 0) {
    perror("IAV_IOC_INIT_UDEC");
    LOG_FATAL("[error]: init udec fail, errno %d\n", errno);
    return -1;
  }
  p_decoder_info->b_use_addr = 1;
  p_decoder_info->bsb_start_offset = (TU32) info.bits_fifo_start;
  p_decoder_info->bsb_size = info.bits_fifo_size;
  return 0;
}

static TInt __s2e_destroy_decoder(TInt iav_fd, TU8 decoder_id)
{
  TInt ret = ioctl(iav_fd, IAV_IOC_RELEASE_UDEC, decoder_id);
  if (ret < 0) {
    perror("IAV_IOC_RELEASE_UDEC");
    LOG_FATAL("[error]: release udec fail, errno %d\n", errno);
    return (-1);
  }
  return ret;
}

static TInt __s2e_query_decode_config(int iav_fd, SAmbaDSPQueryDecodeConfig *config)
{
  if (config) {
    config->auto_map_bsb = 1;
    config->rendering_monitor_mode = 1;
  } else {
    LOG_FATAL("NULL\n");
    return (-1);
  }
  return 0;
}

static int __s2e_decode_trick_play(int iav_fd, TU8 decoder_id, TU8 trick_play)
{
  iav_udec_trickplay_t trickplay;
  int ret;
  trickplay.decoder_id = decoder_id;
  trickplay.mode = trick_play;
  ret = ioctl(iav_fd, IAV_IOC_UDEC_TRICKPLAY, &trickplay);
  if (ret < 0) {
    if (EACCES == errno) {
      LOG_NOTICE("stopped\n");
      return DDECODER_STOPPED;
    }
    LOG_FATAL("[error]: trickplay fail, errno %d.\n", errno);
    perror("IAV_IOC_UDEC_TRICKPLAY");
    return (-1);
  }
  return ret;
}

static int __s2e_decode_start(int iav_fd, TU8 decoder_id)
{
  return 0;
}

static int __s2e_decode_stop(int iav_fd, TU8 decoder_id, TU8 stop_flag)
{
  int ret;
  unsigned int stop_code = (stop_flag << 24) | decoder_id;
  ret = ioctl(iav_fd, IAV_IOC_UDEC_STOP, stop_code);
  if (ret < 0) {
    LOG_FATAL("[error]: stop fail, errno %d.\n", errno);
    perror("IAV_IOC_UDEC_STOP");
    return (-1);
  }
  return 0;
}

static int __s2e_decode_speed(int iav_fd, TU8 decoder_id, TU16 speed, TU8 scan_mode, TU8 direction)
{
  int ret;
  iav_udec_pb_speed_t speed_t;
  memset(&speed_t, 0x0, sizeof(speed_t));
  speed_t.speed = speed;
  speed_t.decoder_id = decoder_id;
  speed_t.scan_mode = scan_mode;
  speed_t.direction = direction;
  ret = ioctl(iav_fd, IAV_IOC_UDEC_PB_SPEED, &speed_t);
  if (ret < 0) {
    if (EACCES == errno) {
      LOG_NOTICE("stopped\n");
      return DDECODER_STOPPED;
    }
    LOG_FATAL("[error]: speed fail, errno %d.\n", errno);
    perror("IAV_IOC_UDEC_PB_SPEED");
    return -1;
  }
  return 0;
}

static int __s2e_decode_request_bits_fifo(int iav_fd, int decoder_id, TU32 size, void *cur_pos_offset)
{
  iav_wait_decoder_t wait;
  int ret;
  wait.emptiness.room = size + 256;
  wait.emptiness.start_addr = (TU8 *) cur_pos_offset;
  wait.flags = IAV_WAIT_BITS_FIFO;
  wait.decoder_id = decoder_id;
  ret = ioctl(iav_fd, IAV_IOC_WAIT_DECODER, &wait);
  if (ret < 0) {
    if (EACCES == errno) {
      LOG_NOTICE("stopped\n");
      return DDECODER_STOPPED;
    }
    LOG_FATAL("[error]: wait decoder fail, errno %d.\n", errno);
    perror("IAV_IOC_WAIT_DECODER");
    return (-1);
  }
  return 0;
}

static int __s2e_decode(int iav_fd, SAmbaDSPDecode *dec)
{
  TInt ret = 0;
  iav_udec_decode_t decode_video;
  memset(&decode_video, 0, sizeof(decode_video));
  decode_video.udec_type = UDEC_H264;
  decode_video.decoder_id = dec->decoder_id;
  decode_video.num_pics = dec->num_frames;
  decode_video.u.fifo.start_addr = (TU8 *) dec->start_ptr_offset;
  decode_video.u.fifo.end_addr = (TU8 *) dec->end_ptr_offset;
  ret = ioctl(iav_fd, IAV_IOC_UDEC_DECODE, &decode_video);
  if (0 > ret) {
    if (EACCES == errno) {
      LOG_NOTICE("stopped\n");
      return DDECODER_STOPPED;
    }
    perror("IAV_IOC_UDEC_DECODE");
    LOG_FATAL("[error]: decode error, errno %d.\n", errno);
    return (-1);
  }
  return 0;
}

static int __s2e_decode_query_bsb_status_and_print(int iav_fd, TU8 decoder_id)
{
  return 0;
}

static int __s2e_decode_query_status_and_print(int iav_fd, TU8 decoder_id)
{
  return 0;
}

static int __s2e_decode_query_bsb_status(int iav_fd, SAmbaDSPBSBStatus *status)
{
  return 0;
}

static int __s2e_decode_query_status(int iav_fd, SAmbaDSPDecodeStatus *status)
{
  return 0;
}

static int __s2e_decode_wait_vout_dormant(int iav_fd, TU8 decoder_id)
{
  int ret;
  iav_udec_state_t state;
  memset(&state, 0, sizeof(state));
  state.decoder_id = decoder_id;
  state.flags = 0;
  ret = ioctl(iav_fd, IAV_IOC_GET_UDEC_STATE, &state);
  if (ret) {
    perror("IAV_IOC_GET_UDEC_STATE");
    LOG_ERROR("IAV_IOC_GET_UDEC_STATE fail, errno %d.\n", errno);
    return (-7);
  }
  if ((IAV_UDEC_STATE_RUN != state.udec_state) && (IAV_UDEC_STATE_READY != state.udec_state)) {
    LOG_ERROR("udec state is (%u), not in run or ready state\n", state.udec_state);
    return (-1);
  }
  if (IAV_VOUT_STATE_PRE_RUN != state.vout_state) {
    if (IAV_VOUT_STATE_DORMANT == state.vout_state) {
      LOG_NOTICE("udec state %u, vout state %d, vout in dormant now\n", state.udec_state, state.vout_state);
      return 0;
    } else if (IAV_VOUT_STATE_RUN == state.vout_state) {
      LOG_ERROR("already in run?\n");
      return (-1);
    }
    LOG_WARN("vout state is (%u), not pre_run or dormant state\n", state.vout_state);
  }
  return 2;
}

static int __s2e_decode_wakevout(int iav_fd, TU8 decoder_id)
{
  int ret;
  iav_wake_vout_t wake_vout;
  memset(&wake_vout, 0, sizeof(wake_vout));
  wake_vout.decoder_id = decoder_id;
  ret = ioctl(iav_fd, IAV_IOC_WAKE_VOUT, &wake_vout);
  if (0 > ret) {
    perror("IAV_IOC_WAKE_VOUT");
    LOG_ERROR("IAV_IOC_WAKE_VOUT fail, errno %d\n", errno);
    return -1;
  }
  return 0;
}

static int __s2e_decode_wait_eos_flag(int iav_fd, SAmbaDSPDecodeEOSTimestamp *eos_timestamp)
{
  int ret = 0;
  iav_udec_status_t status;
  memset(&status, 0, sizeof(status));
  status.decoder_id = eos_timestamp->decoder_id;
  status.nowait = 0;
  ret = ioctl(iav_fd, IAV_IOC_WAIT_UDEC_STATUS, &status);
  if (0 > ret) {
    if (EACCES == errno) {
      LOG_NOTICE("stopped\n");
      return DDECODER_STOPPED;
    }
    perror("IAV_IOC_WAIT_UDEC_STATUS");
    LOG_ERROR("IAV_IOC_WAIT_UDEC_STATUS fail, errno %d.\n", errno);
    return ret;
  }
  if (IAV_VOUT_STATE_RUN != status.vout_state) {
    return 1;
  }
  if (status.eos_flag) {
    eos_timestamp->last_pts_high = status.pts_high;
    eos_timestamp->last_pts_low = status.pts_low;
    LOG_NOTICE("last pts flag comes, high %u, low %u.\n", status.pts_high, status.pts_low);
    return 0;
  }
  return 2;
}

static int __s2e_decode_wait_eos(int iav_fd, SAmbaDSPDecodeEOSTimestamp *eos_timestamp)
{
  int ret = 0;
  iav_udec_status_t status;
  memset(&status, 0, sizeof(status));
  status.decoder_id = eos_timestamp->decoder_id;
  status.nowait = 0;
  ret = ioctl(iav_fd, IAV_IOC_WAIT_UDEC_STATUS, &status);
  if (0 > ret) {
    if (EACCES == errno) {
      LOG_NOTICE("stopped\n");
      return DDECODER_STOPPED;
    }
    perror("IAV_IOC_WAIT_UDEC_STATUS");
    LOG_ERROR("IAV_IOC_WAIT_UDEC_STATUS fail, errno %d.\n", errno);
    return (-1);
  }
  if (IAV_VOUT_STATE_RUN != status.vout_state) {
    return 1;
  }
  if ((status.pts_high == eos_timestamp->last_pts_high) && (status.pts_low == eos_timestamp->last_pts_low)) {
    LOG_NOTICE("wait last pts done!, high %u, low %u.\n", status.pts_high, status.pts_low);
    return 0;
  }
  return 2;
}

static int __s2e_get_vin_info(int iav_fd, SAmbaDSPVinInfo *vininfo)
{
  struct amba_video_info vin_info;
  TU32 fps_q9 = 1;
  int ret = 0;
  ret = ioctl(iav_fd, IAV_IOC_VIN_SRC_GET_VIDEO_INFO, &vin_info);
  if (0 > ret) {
    perror("IAV_IOC_VIN_SRC_GET_VIDEO_INFO");
    LOG_ERROR("IAV_IOC_VIN_SRC_GET_VIDEO_INFO fail, errno %d\n", errno);
    return ret;
  }
  ret = ioctl(iav_fd, IAV_IOC_VIN_SRC_GET_FRAME_RATE, &fps_q9);
  if (0 > ret) {
    perror("IAV_IOC_VIN_SRC_GET_FRAME_RATE");
    LOG_WARN("IAV_IOC_VIN_SRC_GET_FRAME_RATE fail, errno %d\n", errno);
    return ret;
  }
  __parse_fps(fps_q9, vininfo);
  vininfo->width = vin_info.width;
  vininfo->height = vin_info.height;
  vininfo->format = vin_info.format;
  vininfo->type = vin_info.type;
  vininfo->bits = vin_info.bits;
  vininfo->ratio = vin_info.ratio;
  vininfo->system = vin_info.system;
  vininfo->flip = vin_info.flip;
  vininfo->rotate = vin_info.rotate;
  vininfo->pattern = vin_info.pattern;
  return 0;
}

static int __s2e_get_stream_framefactor(int iav_fd, int index, SAmbaDSPStreamFramefactor *framefactor)
{
  iav_change_framerate_factor_ex_t frame_factor;
  int ret = 0;
  if (DMaxStreamNumber <= index) {
    LOG_FATAL("index(%d) not as expected\n", index);
    return (-10);
  }
  memset(&frame_factor, 0, sizeof(frame_factor));
  frame_factor.id = 1 << index;
  ret = ioctl(iav_fd, IAV_IOC_GET_FRAMERATE_FACTOR_EX, &frame_factor);
  if (0 > ret) {
    perror("IAV_IOC_GET_FRAMERATE_FACTOR_EX");
    LOG_ERROR("IAV_IOC_GET_FRAMERATE_FACTOR_EX fail, errno %d\n", errno);
    return ret;
  }
  framefactor->framefactor_num = frame_factor.ratio_numerator;
  framefactor->framefactor_den = frame_factor.ratio_denominator;
  return 0;
}

static int __s2e_map_bsb(int iav_fd, SAmbaDSPMapBSB *map_bsb)
{
  int ret = 0;
  iav_mmap_info_t info;
  ret = ioctl(iav_fd, IAV_IOC_MAP_BSB2, &info);
  if (0 > ret) {
    perror("IAV_IOC_MAP_BSB2");
    LOG_ERROR("IAV_IOC_MAP_BSB2 fail, errno %d\n", errno);
    return (-1);
  }
  map_bsb->base = info.addr;
  map_bsb->size = info.length;
  LOG_PRINTF("[mmap]: bsb_mem = %p, size = 0x%x\n", map_bsb->base, map_bsb->size);
  return 0;
}

static int __s2e_map_dsp(int iav_fd, SAmbaDSPMapDSP *map_dsp)
{
  TInt ret = 0;
  iav_mmap_info_t info;
  ret = ioctl(iav_fd, IAV_IOC_MAP_DSP, &info);
  if (0 > ret) {
    perror("IAV_IOC_MAP_DSP");
    LOG_ERROR("IAV_IOC_MAP_DSP fail, errno %d\n", errno);
    return ret;
  }
  map_dsp->size = info.length;
  map_dsp->base = (void *) info.addr;
  LOG_PRINTF("[mmap]: dsp_mem = %p, size = 0x%x\n", map_dsp->base, map_dsp->size);
  return 0;
}

static int __s2e_unmap_bsb(int iav_fd, SAmbaDSPMapBSB *map_bsb)
{
  if (map_bsb->base && map_bsb->size) {
    int ret = 0;
    ret = ioctl(iav_fd, IAV_IOC_UNMAP_BSB);
    if (0 > ret) {
      perror("IAV_IOC_UNMAP_BSB");
      LOG_ERROR("IAV_IOC_UNMAP_BSB fail, errno %d\n", errno);
      return (-1);
    }
    LOG_PRINTF("[unmap]: bsb_mem = %p, size = 0x%x\n", map_bsb->base, map_bsb->size);
    map_bsb->base = NULL;
    map_bsb->size = 0;
  } else {
    LOG_ERROR("bad params, %p, %d\n", map_bsb->base, map_bsb->size);
    return (-1);
  }
  return 0;
}

static int __s2e_unmap_dsp(int iav_fd, SAmbaDSPMapDSP *map_dsp)
{
  if (map_dsp->base && map_dsp->size) {
    TInt ret = 0;
    ret = ioctl(iav_fd, IAV_IOC_UNMAP_DSP);
    if (0 > ret) {
      perror("IAV_IOC_UNMAP_DSP");
      LOG_ERROR("IAV_IOC_UNMAP_DSP fail, errno %d\n", errno);
      return ret;
    }
    LOG_PRINTF("[unmap]: dsp_mem = %p, size = 0x%x\n", map_dsp->base, map_dsp->size);
    map_dsp->base = NULL;
    map_dsp->size = 0;
  } else {
    LOG_ERROR("bad params, %p, %d\n", map_dsp->base, map_dsp->size);
    return (-1);
  }
  return 0;
}

static int __s2e_efm_get_bufferpool_info(int iav_fd, SAmbaEFMPoolInfo *buffer_pool_info)
{
  TInt ret = 0;
  iav_source_buffer_setup_ex_t  buffer_setup;
  memset(&buffer_setup, 0x0, sizeof(buffer_setup));
  ret = ioctl(iav_fd, IAV_IOC_GET_SOURCE_BUFFER_SETUP_EX, &buffer_setup);
  if (0 > ret) {
    perror("IAV_IOC_GET_SOURCE_BUFFER_SETUP_EX");
    LOG_ERROR("IAV_IOC_GET_SOURCE_BUFFER_SETUP_EX fail, errno %d\n", errno);
    return ret;
  }
  if (buffer_pool_info) {
    buffer_pool_info->yuv_buf_num = 6;
    buffer_pool_info->yuv_pitch = buffer_setup.size[IAV_ENCODE_SOURCE_MAIN_DRAM].width;
    buffer_pool_info->me1_buf_num = 6;
    buffer_pool_info->me1_pitch = buffer_setup.size[IAV_ENCODE_SOURCE_MAIN_DRAM].width / 4;
    buffer_pool_info->yuv_buf_width = buffer_setup.size[IAV_ENCODE_SOURCE_MAIN_DRAM].width;
    buffer_pool_info->yuv_buf_height = buffer_setup.size[IAV_ENCODE_SOURCE_MAIN_DRAM].height;
    buffer_pool_info->me1_buf_width = buffer_setup.size[IAV_ENCODE_SOURCE_MAIN_DRAM].width / 4;
    buffer_pool_info->me1_buf_height = buffer_setup.size[IAV_ENCODE_SOURCE_MAIN_DRAM].height / 4;
    buffer_pool_info->b_use_addr = 1;
  } else {
    LOG_FATAL("NULL param\n");
    return (-2);
  }
  return 0;
}

static int __s2e_efm_request_frame(int iav_fd, SAmbaEFMFrame *frame)
{
  TInt ret = 0;
  iav_enc_dram_buf_frame_ex_t buf_frame;
  memset(&buf_frame, 0, sizeof(buf_frame));
  buf_frame.buf_id = IAV_ENCODE_SOURCE_MAIN_DRAM;
  ret = ioctl(iav_fd, IAV_IOC_ENC_DRAM_REQUEST_FRAME_EX, &buf_frame);
  if (0 > ret) {
    perror("IAV_IOC_ENC_DRAM_REQUEST_FRAME_EX");
    LOG_ERROR("IAV_IOC_ENC_DRAM_REQUEST_FRAME_EX fail, errno %d\n", errno);
    return ret;
  }
  if (frame) {
    frame->frame_idx = buf_frame.frame_id;
    frame->yuv_luma_offset = (TPointer) buf_frame.y_addr;
    frame->yuv_chroma_offset = (TPointer) buf_frame.uv_addr;
    frame->me1_offset = (TPointer) buf_frame.me1_addr;
  } else {
    LOG_FATAL("NULL param\n");
    return (-2);
  }
  return 0;
}

static int __s2e_efm_finish_frame(int iav_fd, SAmbaEFMFinishFrame *finish_frame)
{
  if (finish_frame) {
    TInt ret = 0;
    iav_enc_dram_buf_update_ex_t buf_update;
    memset(&buf_update, 0, sizeof(buf_update));
    buf_update.buf_id = IAV_ENCODE_SOURCE_MAIN_DRAM;
    buf_update.frame_id = finish_frame->frame_idx;
    buf_update.frame_pts = finish_frame->frame_pts;
    buf_update.is_last_frame = finish_frame->is_last_frame;
    buf_update.use_hw_pts = finish_frame->use_hw_pts;
    buf_update.size.width = finish_frame->buffer_width;
    buf_update.size.height = finish_frame->buffer_height;
    ret = ioctl(iav_fd, IAV_IOC_ENC_DRAM_UPDATE_FRAME_EX, &buf_update);
    if (0 > ret) {
      perror("IAV_IOC_ENC_DRAM_UPDATE_FRAME_EX");
      LOG_ERROR("IAV_IOC_ENC_DRAM_UPDATE_FRAME_EX fail, errno %d\n", errno);
      return ret;
    }
  } else {
    LOG_FATAL("NULL param\n");
    return (-2);
  }
  return 0;
}

static int __s2e_read_bitstream(int iav_fd, SAmbaDSPReadBitstream *bitstream)
{
  bits_info_ex_t  bits_info;
  int ret = 0;
  memset(&bits_info, 0, sizeof(bits_info_ex_t));
  bits_info.time_ms = bitstream->timeout_ms;
  ret = ioctl(iav_fd, IAV_IOC_READ_BITSTREAM_EX, &bits_info);
  if (EAGAIN == errno) {
    return 1;
  } else if (EIO == errno) {
    LOG_NOTICE("IAV_IOC_READ_BITSTREAM_EX ret EIO\n");
    return 2;
  }
  if (0 > ret) {
    perror("IAV_IOC_READ_BITSTREAM_EX");
    LOG_ERROR("IAV_IOC_READ_BITSTREAM_EX fail, errno %d\n", errno);
    return ret;
  }
  bitstream->stream_id = bits_info.stream_id;
  bitstream->offset = bits_info.data_addr_offset;
  bitstream->size = bits_info.pic_size;
  bitstream->pts = bits_info.PTS;
  bitstream->video_width = bits_info.pic_width;
  bitstream->video_height = bits_info.pic_height;
  if (bits_info.stream_end) {
    bitstream->size = 0;
    bitstream->offset = 0;
    return 0;
  }
  if (P_FRAME == bits_info.pic_type) {
    bitstream->stream_type = StreamFormat_H264;
    bitstream->hint_frame_type = EPredefinedPictureType_P;
    bitstream->hint_is_keyframe = 0;
  } else if (IDR_FRAME == bits_info.pic_type) {
    bitstream->stream_type = StreamFormat_H264;
    bitstream->hint_frame_type = EPredefinedPictureType_IDR;
    bitstream->hint_is_keyframe = 1;
  } else if (I_FRAME == bits_info.pic_type) {
    bitstream->stream_type = StreamFormat_H264;
    bitstream->hint_frame_type = EPredefinedPictureType_I;
    bitstream->hint_is_keyframe = 0;
  } else if (B_FRAME == bits_info.pic_type) {
    bitstream->stream_type = StreamFormat_H264;
    bitstream->hint_frame_type = EPredefinedPictureType_B;
    bitstream->hint_is_keyframe = 0;
  } else if (JPEG_STREAM == bits_info.pic_type) {
    bitstream->stream_type = StreamFormat_JPEG;
    bitstream->hint_frame_type = EPredefinedPictureType_IDR;
    bitstream->hint_is_keyframe = 1;
  } else if ((IAV_PIC_TYPE_LAST > bits_info.pic_type) && (IAV_PIC_TYPE_FIRST <= bits_info.pic_type)) {
    bitstream->stream_type = StreamFormat_Invalid;
    LOG_NOTICE("not supported pic_type %d\n", bits_info.pic_type);
  } else {
    bitstream->stream_type = StreamFormat_Invalid;
    LOG_FATAL("bad pic type %d\n", bits_info.pic_type);
    return (-2);
  }
  return 0;
}

static int __s2e_is_ready_for_read_bitstream(int iav_fd)
{
  unsigned int i = 0;
  iav_encode_stream_info_ex_t info;
  for (i = 0; i < IAV_STREAM_MAX_NUM_IMPL; ++ i) {
    info.id = (1 << i);
    if (ioctl(iav_fd, IAV_IOC_GET_ENCODE_STREAM_INFO_EX, &info) < 0) {
      perror("IAV_IOC_GET_ENCODE_STREAM_INFO_EX");
      LOG_ERROR("IAV_IOC_GET_ENCODE_STREAM_INFO_EX error\n");
      return 0;
    }
    if (info.state == IAV_STREAM_STATE_ENCODING) {
      return 1;
    }
  }
  return 0;
}

static int __s2e_encode_start(int iav_fd, TU32 mask)
{
  int ret = ioctl(iav_fd, IAV_IOC_START_ENCODE_EX, mask);
  if (ret) {
    perror("IAV_IOC_START_ENCODE_EX");
    LOG_ERROR("IAV_IOC_START_ENCODE_EX fail, errno %d, mask 0x%08x\n", errno, mask);
    return ret;
  }
  return 0;
}

static int __s2e_encode_stop(int iav_fd, TU32 mask)
{
  int ret = ioctl(iav_fd, IAV_IOC_STOP_ENCODE_EX, mask);
  if (ret) {
    perror("IAV_IOC_STOP_ENCODE_EX");
    LOG_ERROR("IAV_IOC_STOP_ENCODE_EX fail, errno %d, mask 0x%08x\n", errno, mask);
    return ret;
  }
  return 0;
}

static int __s2e_query_encode_stream_info(int iav_fd, SAmbaDSPEncStreamInfo *info)
{
  iav_encode_stream_info_ex_t stream_info;
  int ret = 0;
  memset(&stream_info, 0, sizeof(stream_info));
  stream_info.id = (1 << info->id);
  ret = ioctl(iav_fd, IAV_IOC_GET_ENCODE_STREAM_INFO_EX, &stream_info);
  if (ret) {
    perror("IAV_IOC_GET_ENCODE_STREAM_INFO_EX");
    LOG_ERROR("IAV_IOC_GET_ENCODE_STREAM_INFO_EX fail, errno %d\n", errno);
    return ret;
  }
  switch (stream_info.state) {
    case IAV_STREAM_STATE_READY_FOR_ENCODING:
      info->state = EAMDSP_ENC_STREAM_STATE_IDLE;
      break;
    case IAV_STREAM_STATE_STARTING:
      info->state = EAMDSP_ENC_STREAM_STATE_STARTING;
      break;
    case IAV_STREAM_STATE_ENCODING:
      info->state = EAMDSP_ENC_STREAM_STATE_ENCODING;
      break;
    case IAV_STREAM_STATE_STOPPING:
      info->state = EAMDSP_ENC_STREAM_STATE_STOPPING;
      break;
    case IAV_STREAM_STATE_UNKNOWN:
      LOG_ERROR("unknown state\n");
      info->state = EAMDSP_ENC_STREAM_STATE_UNKNOWN;
      return (-1);
      break;
    case IAV_STREAM_STATE_ERROR:
      LOG_ERROR("state error\n");
      info->state = EAMDSP_ENC_STREAM_STATE_ERROR;
      return (-2);
      break;
    default:
      LOG_ERROR("unexpected state %d\n", stream_info.state);
      info->state = EAMDSP_ENC_STREAM_STATE_ERROR;
      return (-3);
      break;
  }
  return 0;
}

static int __s2e_query_encode_stream_format(int iav_fd, SAmbaDSPEncStreamFormat *fmt)
{
  iav_encode_format_ex_t stream_encode_format;
  int ret = 0;
  memset(&stream_encode_format, 0, sizeof(stream_encode_format));
  stream_encode_format.id = (1 << fmt->id);
  ret = ioctl(iav_fd, IAV_IOC_GET_ENCODE_FORMAT_EX, &stream_encode_format);
  if (ret) {
    perror("IAV_IOC_GET_ENCODE_FORMAT_EX");
    LOG_ERROR("IAV_IOC_GET_ENCODE_FORMAT_EX fail, errno %d\n", errno);
    return ret;
  }
  switch (stream_encode_format.encode_type) {
    case IAV_ENCODE_H264:
      fmt->codec = EAMDSP_VIDEO_CODEC_TYPE_H264;
      break;
    case IAV_ENCODE_MJPEG:
      fmt->codec = EAMDSP_VIDEO_CODEC_TYPE_MJPEG;
      break;
    case IAV_ENCODE_NONE:
      LOG_WARN("codec none?\n");
      fmt->codec = EAMDSP_VIDEO_CODEC_TYPE_INVALID;
      break;
    default:
      LOG_ERROR("unexpected codec %d\n", stream_encode_format.encode_type);
      fmt->codec = EAMDSP_VIDEO_CODEC_TYPE_INVALID;
      return (-1);
      break;
  }
  switch (stream_encode_format.source) {
    case IAV_ENCODE_SOURCE_MAIN_BUFFER:
      fmt->source_buffer = EAMDSP_ENC_STREAM_SOURCE_BUFFER_MAIN;
      break;
    case IAV_ENCODE_SOURCE_SECOND_BUFFER:
      fmt->source_buffer = EAMDSP_ENC_STREAM_SOURCE_BUFFER_SECOND;
      break;
    case IAV_ENCODE_SOURCE_THIRD_BUFFER:
      fmt->source_buffer = EAMDSP_ENC_STREAM_SOURCE_BUFFER_THIRD;
      break;
    case IAV_ENCODE_SOURCE_FOURTH_BUFFER:
      fmt->source_buffer = EAMDSP_ENC_STREAM_SOURCE_BUFFER_FOURTH;
      break;
    case IAV_ENCODE_SOURCE_MAIN_DRAM:
      fmt->source_buffer = EAMDSP_ENC_STREAM_SOURCE_BUFFER_EFM;
      break;
    default:
      LOG_ERROR("unexpected source buffer %d\n", stream_encode_format.encode_type);
      fmt->source_buffer = EAMDSP_ENC_STREAM_SOURCE_BUFFER_INVALID;
      return (-2);
      break;
  }
  return 0;
}

static TInt __s2e_dsp_enter_idle_mode(TInt iav_fd)
{
  TInt ret = ioctl(iav_fd, IAV_IOC_ENTER_IDLE);
  if (0 > ret) {
    perror("IAV_IOC_ENTER_IDLE");
    LOG_FATAL("enter idle mode fail, errno %d\n", errno);
  }
  return ret;
}

static void __setup_s2_s2e_al_context(SFAmbaDSPDecAL *al)
{
  al->f_get_dsp_mode = __s2e_get_dsp_mode;
  al->f_enter_mode = __s2e_enter_decode_mode;
  al->f_leave_mode = __s2e_leave_decode_mode;
  al->f_create_decoder = __s2e_create_decoder;
  al->f_destroy_decoder = __s2e_destroy_decoder;
  al->f_query_decode_config = __s2e_query_decode_config;
  al->f_trickplay = __s2e_decode_trick_play;
  al->f_start = __s2e_decode_start;
  al->f_stop = __s2e_decode_stop;
  al->f_speed = __s2e_decode_speed;
  al->f_request_bsb = __s2e_decode_request_bits_fifo;
  al->f_decode = __s2e_decode;
  al->f_query_print_decode_bsb_status = __s2e_decode_query_bsb_status_and_print;
  al->f_query_print_decode_status = __s2e_decode_query_status_and_print;
  al->f_query_decode_bsb_status = __s2e_decode_query_bsb_status;
  al->f_query_decode_status = __s2e_decode_query_status;
  al->f_decode_wait_vout_dormant = __s2e_decode_wait_vout_dormant;
  al->f_decode_wake_vout = __s2e_decode_wakevout;
  al->f_decode_wait_eos_flag = __s2e_decode_wait_eos_flag;
  al->f_decode_wait_eos = __s2e_decode_wait_eos;
  al->f_configure_vout = __configure_vout;
  al->f_get_vout_info = __get_single_vout_info;
  al->f_get_vin_info = __s2e_get_vin_info;
  al->f_get_stream_framefactor = __s2e_get_stream_framefactor;
  al->f_map_bsb = __s2e_map_bsb;
  al->f_map_dsp = __s2e_map_dsp;
  al->f_map_intrapb = NULL;
  al->f_unmap_bsb = __s2e_unmap_bsb;
  al->f_unmap_dsp = __s2e_unmap_dsp;
  al->f_unmap_intrapb = NULL;
  al->f_efm_get_buffer_pool_info = __s2e_efm_get_bufferpool_info;
  al->f_efm_request_frame = __s2e_efm_request_frame;
  al->f_efm_finish_frame = __s2e_efm_finish_frame;
  al->f_read_bitstream = __s2e_read_bitstream;
  al->f_is_ready_for_read_bitstream = __s2e_is_ready_for_read_bitstream;
  al->f_intraplay_reset_buffers = NULL;
  al->f_intraplay_request_buffer = NULL;
  al->f_intraplay_decode = NULL;
  al->f_intraplay_yuv2yuv = NULL;
  al->f_intraplay_display = NULL;
  al->f_encode_start = __s2e_encode_start;
  al->f_encode_stop = __s2e_encode_stop;
  al->f_query_encode_stream_info = __s2e_query_encode_stream_info;
  al->f_query_encode_stream_fmt = __s2e_query_encode_stream_format;
  al->f_query_source_buffer_info = NULL;
  al->f_query_yuv_buffer = NULL;
  al->f_gdma_copy = NULL;
  al->f_enter_idle_mode = __s2e_dsp_enter_idle_mode;
}

#elif defined (BUILD_DSP_AMBA_S3L)

static TInt __s3l_get_dsp_mode(TInt iav_fd, SAmbaDSPMode *mode)
{
  int state = 0;
  int ret = 0;
  ret = ioctl(iav_fd, IAV_IOC_GET_IAV_STATE, &state);
  if (0 > ret) {
    perror("IAV_IOC_GET_IAV_STATE");
    LOG_FATAL("IAV_IOC_GET_IAV_STATE fail, errno %d\n", errno);
    return ret;
  }
  switch (state) {
    case IAV_STATE_INIT:
      mode->dsp_mode = EAMDSP_MODE_INIT;
      break;
    case IAV_STATE_IDLE:
      mode->dsp_mode = EAMDSP_MODE_IDLE;
      break;
    case IAV_STATE_PREVIEW:
      mode->dsp_mode = EAMDSP_MODE_PREVIEW;
      break;
    case IAV_STATE_ENCODING:
      mode->dsp_mode = EAMDSP_MODE_ENCODE;
      break;
    case IAV_STATE_DECODING:
      mode->dsp_mode = EAMDSP_MODE_DECODE;
      break;
    default:
      LOG_FATAL("un expected dsp mode %d\n", state);
      mode->dsp_mode = EAMDSP_MODE_INVALID;
      break;
  }
  return 0;
}

static TInt __s3l_enter_decode_mode(TInt iav_fd, SAmbaDSPDecodeModeConfig *mode_config)
{
  struct iav_decode_mode_config decode_mode;
  TInt i = 0;
  memset(&decode_mode, 0x0, sizeof(decode_mode));
  if (mode_config->num_decoder > DAMBADSP_MAX_DECODER_NUMBER) {
    LOG_FATAL("BAD num_decoder %d\n", mode_config->num_decoder);
    return (-100);
  }
  decode_mode.num_decoder = mode_config->num_decoder;
  decode_mode.max_frm_width = mode_config->max_frm_width;
  decode_mode.max_frm_height = mode_config->max_frm_height;
  decode_mode.max_vout0_width = mode_config->max_vout0_width;
  decode_mode.max_vout0_height = mode_config->max_vout0_height;
  decode_mode.max_vout1_width = mode_config->max_vout1_width;
  decode_mode.max_vout1_height = mode_config->max_vout1_height;
  decode_mode.b_support_ff_fb_bw = mode_config->b_support_ff_fb_bw;
  decode_mode.max_n_to_m_ratio = mode_config->max_gop_size;
  decode_mode.debug_max_frame_per_interrupt = mode_config->debug_max_frame_per_interrupt;
  decode_mode.debug_use_dproc = mode_config->debug_use_dproc;
  i = ioctl(iav_fd, IAV_IOC_ENTER_DECODE_MODE, &decode_mode);
  if (0 > i) {
    perror("IAV_IOC_ENTER_DECODE_MODE");
    LOG_FATAL("enter decode mode fail, errno %d\n", errno);
    return i;
  }
  return 0;
}

static TInt __s3l_leave_decode_mode(TInt iav_fd)
{
  TInt ret = ioctl(iav_fd, IAV_IOC_LEAVE_DECODE_MODE);
  if (0 > ret) {
    perror("IAV_IOC_LEAVE_DECODE_MODE");
    LOG_FATAL("leave decode mode fail, errno %d\n", errno);
  }
  return ret;
}

static TInt __s3l_create_decoder(TInt iav_fd, SAmbaDSPDecoderInfo *p_decoder_info)
{
  TInt i = 0;
  struct iav_decoder_info decoder_info;
  memset(&decoder_info, 0x0, sizeof(decoder_info));
  decoder_info.decoder_id = p_decoder_info->decoder_id;
  if (EAMDSP_VIDEO_CODEC_TYPE_H264 == p_decoder_info->decoder_type) {
    decoder_info.decoder_type = IAV_DECODER_TYPE_H264;
  } else if (EAMDSP_VIDEO_CODEC_TYPE_H265 == p_decoder_info->decoder_type) {
    decoder_info.decoder_type = IAV_DECODER_TYPE_H265;
  } else {
    LOG_FATAL("bad video codec type %d\n", p_decoder_info->decoder_type);
    return (-101);
  }
  decoder_info.num_vout = p_decoder_info->num_vout;
  decoder_info.width = p_decoder_info->width;
  decoder_info.height = p_decoder_info->height;
  if (decoder_info.num_vout > DAMBADSP_MAX_VOUT_NUMBER) {
    LOG_FATAL("BAD num_vout %d\n", p_decoder_info->num_vout);
    return (-100);
  }
  for (i = 0; i < decoder_info.num_vout; i ++) {
    decoder_info.vout_configs[i].vout_id = p_decoder_info->vout_configs[i].vout_id;
    decoder_info.vout_configs[i].enable = p_decoder_info->vout_configs[i].enable;
    decoder_info.vout_configs[i].flip = p_decoder_info->vout_configs[i].flip;
    decoder_info.vout_configs[i].rotate = p_decoder_info->vout_configs[i].rotate;
    decoder_info.vout_configs[i].target_win_offset_x = p_decoder_info->vout_configs[i].target_win_offset_x;
    decoder_info.vout_configs[i].target_win_offset_y = p_decoder_info->vout_configs[i].target_win_offset_y;
    decoder_info.vout_configs[i].target_win_width = p_decoder_info->vout_configs[i].target_win_width;
    decoder_info.vout_configs[i].target_win_height = p_decoder_info->vout_configs[i].target_win_height;
    decoder_info.vout_configs[i].zoom_factor_x = p_decoder_info->vout_configs[i].zoom_factor_x;
    decoder_info.vout_configs[i].zoom_factor_y = p_decoder_info->vout_configs[i].zoom_factor_y;
    decoder_info.vout_configs[i].vout_mode = p_decoder_info->vout_configs[i].vout_mode;
    LOG_PRINTF("vout(%d), w %d, h %d, zoom x %08x, y %08x\n", i, decoder_info.vout_configs[i].target_win_width, decoder_info.vout_configs[i].target_win_height, decoder_info.vout_configs[i].zoom_factor_x, decoder_info.vout_configs[i].zoom_factor_y);
  }
  decoder_info.bsb_start_offset = p_decoder_info->bsb_start_offset;
  decoder_info.bsb_size = p_decoder_info->bsb_size;
  i = ioctl(iav_fd, IAV_IOC_CREATE_DECODER, &decoder_info);
  if (0 > i) {
    perror("IAV_IOC_CREATE_DECODER");
    LOG_FATAL("create decoder fail, errno %d\n", errno);
    return i;
  }
  p_decoder_info->bsb_start_offset = decoder_info.bsb_start_offset;
  p_decoder_info->bsb_size = decoder_info.bsb_size;
  return 0;
}

static TInt __s3l_destroy_decoder(TInt iav_fd, TU8 decoder_id)
{
  TInt ret = ioctl(iav_fd, IAV_IOC_DESTROY_DECODER, decoder_id);
  if (0 > ret) {
    perror("IAV_IOC_DESTROY_DECODER");
    LOG_FATAL("destroy decoder fail, errno %d\n", errno);
  }
  return ret;
}

static TInt __s3l_query_decode_config(int iav_fd, SAmbaDSPQueryDecodeConfig *config)
{
  if (config) {
    config->auto_map_bsb = 0;
    config->rendering_monitor_mode = 0;
  } else {
    LOG_FATAL("NULL\n");
    return (-1);
  }
  return 0;
}

static int __s3l_decode_trick_play(int iav_fd, TU8 decoder_id, TU8 trick_play)
{
  int ret;
  struct iav_decode_trick_play trickplay;
  trickplay.decoder_id = decoder_id;
  trickplay.trick_play = trick_play;
  ret = ioctl(iav_fd, IAV_IOC_DECODE_TRICK_PLAY, &trickplay);
  if (0 > ret) {
    if (EACCES == errno) {
      LOG_NOTICE("stopped\n");
      return DDECODER_STOPPED;
    }
    perror("IAV_IOC_DECODE_TRICK_PLAY");
    LOG_ERROR("trickplay error, errno %d\n", errno);
    return ret;
  }
  return 0;
}

static int __s3l_decode_start(int iav_fd, TU8 decoder_id)
{
  int ret = ioctl(iav_fd, IAV_IOC_DECODE_START, decoder_id);
  if (ret < 0) {
    perror("IAV_IOC_DECODE_START");
    LOG_ERROR("decode start error, errno %d\n", errno);
    return ret;
  }
  return 0;
}

static int __s3l_decode_stop(int iav_fd, TU8 decoder_id, TU8 stop_flag)
{
  int ret;
  struct iav_decode_stop stop;
  stop.decoder_id = decoder_id;
  stop.stop_flag = stop_flag;
  ret = ioctl(iav_fd, IAV_IOC_DECODE_STOP, &stop);
  if (0 > ret) {
    perror("IAV_IOC_DECODE_STOP");
    LOG_ERROR("decode stop error, errno %d\n", errno);
    return ret;
  }
  return 0;
}

static int __s3l_decode_speed(int iav_fd, TU8 decoder_id, TU16 speed, TU8 scan_mode, TU8 direction)
{
  int ret;
  struct iav_decode_speed spd;
  spd.decoder_id = decoder_id;
  spd.direction = direction;
  spd.speed = speed;
  spd.scan_mode = scan_mode;
  //LOG_PRINTF("speed, direction %d, speed %x, scan mode %d\n", spd.direction, spd.speed, spd.scan_mode);
  ret = ioctl(iav_fd, IAV_IOC_DECODE_SPEED, &spd);
  if (0 > ret) {
    if (EACCES == errno) {
      LOG_NOTICE("stopped\n");
      return DDECODER_STOPPED;
    }
    perror("IAV_IOC_DECODE_SPEED");
    LOG_ERROR("decode speed error, errno %d\n", errno);
    return ret;
  }
  return 0;
}

static int __s3l_decode_request_bits_fifo(int iav_fd, int decoder_id, TU32 size, void *cur_pos_offset)
{
  struct iav_decode_bsb wait;
  int ret;
  wait.decoder_id = decoder_id;
  wait.room = size;
  wait.start_offset = (TU32) cur_pos_offset;
  ret = ioctl(iav_fd, IAV_IOC_WAIT_DECODE_BSB, &wait);
  if (0 > ret) {
    if (EACCES == errno) {
      LOG_NOTICE("stopped\n");
      return DDECODER_STOPPED;
    }
    LOG_ERROR("IAV_IOC_WAIT_DECODE_BSB fail, errno %d.\n", errno);
    perror("IAV_IOC_WAIT_DECODE_BSB");
    return ret;
  }
  return 0;
}

static int __s3l_decode(int iav_fd, SAmbaDSPDecode *dec)
{
  TInt ret = 0;
  struct iav_decode_video decode_video;
  memset(&decode_video, 0, sizeof(decode_video));
  decode_video.decoder_id = dec->decoder_id;
  decode_video.num_frames = dec->num_frames;
  decode_video.start_ptr_offset = dec->start_ptr_offset;
  decode_video.end_ptr_offset = dec->end_ptr_offset;
  decode_video.first_frame_display = dec->first_frame_display;
  ret = ioctl(iav_fd, IAV_IOC_DECODE_VIDEO, &decode_video);
  if (0 > ret) {
    if (EACCES == errno) {
      LOG_NOTICE("stopped\n");
      return DDECODER_STOPPED;
    }
    perror("IAV_IOC_DECODE_VIDEO");
    LOG_ERROR("IAV_IOC_DECODE_VIDEO fail, errno %d.\n", errno);
    return ret;
  }
  return 0;
}

static int __s3l_decode_query_bsb_status_and_print(int iav_fd, TU8 decoder_id)
{
  int ret;
  struct iav_decode_bsb bsb;
  memset(&bsb, 0x0, sizeof(bsb));
  bsb.decoder_id = decoder_id;
  ret = ioctl(iav_fd, IAV_IOC_QUERY_DECODE_BSB, &bsb);
  if (0 > ret) {
    if (EACCES == errno) {
      LOG_NOTICE("stopped\n");
      return DDECODER_STOPPED;
    }
    LOG_ERROR("IAV_IOC_QUERY_DECODE_BSB fail, errno %d.\n", errno);
    perror("IAV_IOC_QUERY_DECODE_BSB");
    return ret;
  }
  LOG_PRINTF("[bsb]: current write offset (arm) 0x%08x, current read offset (dsp) 0x%08x, safe room (minus 256 bytes) %d, free room %d\n", bsb.start_offset, bsb.dsp_read_offset, bsb.room, bsb.free_room);
  return 0;
}

static int __s3l_decode_query_status_and_print(int iav_fd, TU8 decoder_id)
{
  int ret;
  struct iav_decode_status status;
  memset(&status, 0x0, sizeof(status));
  status.decoder_id = decoder_id;
  ret = ioctl(iav_fd, IAV_IOC_QUERY_DECODE_STATUS, &status);
  if (0 > ret) {
    if (EACCES == errno) {
      LOG_NOTICE("stopped\n");
      return DDECODER_STOPPED;
    }
    perror("IAV_IOC_QUERY_DECODE_STATUS");
    LOG_ERROR("IAV_IOC_QUERY_DECODE_STATUS fail, errno %d.\n", errno);
    return ret;
  }
  LOG_PRINTF("[decode status]: decode_state %d, decoded_pic_number %d, error_status %d, total_error_count %d, irq_count %d\n", status.decode_state, status.decoded_pic_number, status.error_status, status.total_error_count, status.irq_count);
  LOG_PRINTF("[decode status, bsb]: current write offset (arm) 0x%08x, current read offset (dsp) 0x%08x, safe room (minus 256 bytes) %d, free room %d\n", status.write_offset, status.dsp_read_offset, status.room, status.free_room);
  LOG_PRINTF("[decode status, last pts]: %d, is_started %d, is_send_stop_cmd %d\n", status.last_pts, status.is_started, status.is_send_stop_cmd);
  LOG_PRINTF("[decode status, yuv addr]: yuv422_y 0x%08x, yuv422_uv 0x%08x\n", status.yuv422_y_addr, status.yuv422_uv_addr);
  return 0;
}

static int __s3l_decode_query_bsb_status(int iav_fd, SAmbaDSPBSBStatus *status)
{
  int ret;
  struct iav_decode_bsb bsb;
  memset(&bsb, 0x0, sizeof(bsb));
  bsb.decoder_id = status->decoder_id;
  ret = ioctl(iav_fd, IAV_IOC_QUERY_DECODE_BSB, &bsb);
  if (0 > ret) {
    if (EACCES == errno) {
      LOG_NOTICE("stopped\n");
      return DDECODER_STOPPED;
    }
    LOG_ERROR("IAV_IOC_QUERY_DECODE_BSB fail, errno %d.\n", errno);
    perror("IAV_IOC_QUERY_DECODE_BSB");
    return ret;
  }
  status->start_offset = bsb.start_offset;
  status->room = bsb.room;
  status->dsp_read_offset = bsb.dsp_read_offset;
  status->free_room = bsb.free_room;
  return 0;
}

static int __s3l_decode_query_status(int iav_fd, SAmbaDSPDecodeStatus *status)
{
  int ret;
  struct iav_decode_status dec_status;
  memset(&dec_status, 0x0, sizeof(dec_status));
  dec_status.decoder_id = status->decoder_id;
  ret = ioctl(iav_fd, IAV_IOC_QUERY_DECODE_STATUS, &dec_status);
  if (0 > ret) {
    if (EACCES == errno) {
      LOG_NOTICE("stopped\n");
      return DDECODER_STOPPED;
    }
    perror("IAV_IOC_QUERY_DECODE_STATUS");
    LOG_ERROR("IAV_IOC_QUERY_DECODE_STATUS fail, errno %d.\n", errno);
    return ret;
  }
  status->is_started = dec_status.is_started;
  status->is_send_stop_cmd = dec_status.is_send_stop_cmd;
  status->last_pts = dec_status.last_pts;
  status->decode_state = dec_status.decode_state;
  status->error_status = dec_status.error_status;
  status->total_error_count = dec_status.total_error_count;
  status->decoded_pic_number = dec_status.decoded_pic_number;
  status->write_offset = dec_status.write_offset;
  status->room = dec_status.room;
  status->dsp_read_offset = dec_status.dsp_read_offset;
  status->free_room = dec_status.free_room;
  status->irq_count = dec_status.irq_count;
  status->yuv422_y_addr = dec_status.yuv422_y_addr;
  status->yuv422_uv_addr = dec_status.yuv422_uv_addr;
  return 0;
}

static int __s3l_get_vin_info(int iav_fd, SAmbaDSPVinInfo *vininfo)
{
  struct vindev_video_info vin_info;
  struct vindev_fps active_fps;
  TU32 fps_q9 = 1;
  int ret = 0;
  memset(&vin_info, 0x0, sizeof(vin_info));
  vin_info.vsrc_id = 0;
  vin_info.info.mode = AMBA_VIDEO_MODE_CURRENT;
  ret = ioctl(iav_fd, IAV_IOC_VIN_GET_VIDEOINFO, &vin_info);
  if (0 > ret) {
    perror("IAV_IOC_VIN_GET_VIDEOINFO");
    LOG_WARN("IAV_IOC_VIN_GET_VIDEOINFO fail, errno %d\n", errno);
    return ret;
  }
  memset(&active_fps, 0, sizeof(active_fps));
  active_fps.vsrc_id = 0;
  ret = ioctl(iav_fd, IAV_IOC_VIN_GET_FPS, &active_fps);
  if (0 > ret) {
    perror("IAV_IOC_VIN_GET_FPS");
    LOG_WARN("IAV_IOC_VIN_GET_FPS fail, errno %d\n", errno);
    return ret;
  }
  fps_q9 = active_fps.fps;
  __parse_fps(fps_q9, vininfo);
  vininfo->width = vin_info.info.width;
  vininfo->height = vin_info.info.height;
  vininfo->format = vin_info.info.format;
  vininfo->type = vin_info.info.type;
  vininfo->bits = vin_info.info.bits;
  vininfo->ratio = vin_info.info.ratio;
  vininfo->system = vin_info.info.system;
  vininfo->flip = vin_info.info.flip;
  vininfo->rotate = vin_info.info.rotate;
  return 0;
}

static int __s3l_get_stream_framefactor(int iav_fd, int index, SAmbaDSPStreamFramefactor *framefactor)
{
  struct iav_stream_cfg streamcfg;
  int ret = 0;
  if (DMaxStreamNumber <= index) {
    LOG_FATAL("index(%d) not as expected\n", index);
    return (-10);
  }
  memset(&streamcfg, 0, sizeof(streamcfg));
  streamcfg.id = index;
  streamcfg.cid = IAV_STMCFG_FPS;
  ret = ioctl(iav_fd, IAV_IOC_GET_STREAM_CONFIG, &streamcfg);
  if (0 > ret) {
    perror("IAV_IOC_GET_STREAM_CONFIG");
    LOG_ERROR("IAV_IOC_GET_STREAM_CONFIG fail, errno %d\n", errno);
    return ret;
  }
  framefactor->framefactor_num = streamcfg.arg.fps.fps_multi;
  framefactor->framefactor_den = streamcfg.arg.fps.fps_div;
  return 0;
}

static int __s3l_map_bsb(int iav_fd, SAmbaDSPMapBSB *map_bsb)
{
  int ret = 0;
  unsigned int map_size = 0;
  struct iav_querybuf querybuf;
  querybuf.buf = IAV_BUFFER_BSB;
  ret = ioctl(iav_fd, IAV_IOC_QUERY_BUF, &querybuf);
  if (0 > ret) {
    perror("IAV_IOC_QUERY_BUF");
    LOG_ERROR("IAV_IOC_QUERY_BUF fail, errno %d\n", errno);
    return ret;
  }
  map_bsb->size = querybuf.length;
  if (map_bsb->b_two_times) {
    map_size = querybuf.length * 2;
  } else {
    map_size = querybuf.length;
  }
  if (map_bsb->b_enable_read && map_bsb->b_enable_write) {
    map_bsb->base = mmap(NULL, map_size, PROT_WRITE | PROT_READ, MAP_SHARED, iav_fd, querybuf.offset);
  } else if (map_bsb->b_enable_read && !map_bsb->b_enable_write) {
    map_bsb->base = mmap(NULL, map_size, PROT_READ, MAP_SHARED, iav_fd, querybuf.offset);
  } else if (!map_bsb->b_enable_read && map_bsb->b_enable_write) {
    map_bsb->base = mmap(NULL, map_size, PROT_WRITE, MAP_SHARED, iav_fd, querybuf.offset);
  } else {
    LOG_ERROR("not read or write\n");
    return (-1);
  }
  if (map_bsb->base == MAP_FAILED) {
    perror("mmap");
    LOG_ERROR("mmap fail\n");
    return -1;
  }
  LOG_PRINTF("[mmap]: bsb_mem = %p, size = 0x%x\n", map_bsb->base, map_bsb->size);
  return 0;
}

static int __s3l_map_dsp(int iav_fd, SAmbaDSPMapDSP *map_dsp)
{
  TInt ret = 0;
  struct iav_querybuf querybuf;
  querybuf.buf = IAV_BUFFER_DSP;
  ret = ioctl(iav_fd, IAV_IOC_QUERY_BUF, &querybuf);
  if (DUnlikely(0 > ret)) {
    perror("IAV_IOC_QUERY_BUF");
    LOG_ERROR("IAV_IOC_QUERY_BUF fail, errno %d\n", errno);
    return ret;
  }
  map_dsp->size = querybuf.length;
  map_dsp->base = mmap(NULL, map_dsp->size, PROT_READ | PROT_WRITE, MAP_SHARED, iav_fd, querybuf.offset);
  if (DUnlikely(map_dsp->base == MAP_FAILED)) {
    perror("mmap");
    LOG_ERROR("mmap fail, errno %d\n", errno);
    return -1;
  }
  LOG_PRINTF("[mmap]: dsp_mem = %p, size = 0x%x\n", map_dsp->base, map_dsp->size);
  return 0;
}

static int __s3l_map_intrapb(int iav_fd, SAmbaDSPMapIntraPB *map_intrapb)
{
  TInt ret = 0;
  struct iav_querybuf querybuf;
  querybuf.buf = IAV_BUFFER_INTRA_PB;
  ret = ioctl(iav_fd, IAV_IOC_QUERY_BUF, &querybuf);
  if (DUnlikely(0 > ret)) {
    perror("IAV_IOC_QUERY_BUF");
    LOG_ERROR("IAV_IOC_QUERY_BUF fail, errno %d\n", errno);
    return ret;
  }
  map_intrapb->size = querybuf.length;
  map_intrapb->base = mmap(NULL, map_intrapb->size, PROT_READ | PROT_WRITE, MAP_SHARED, iav_fd, querybuf.offset);
  if (DUnlikely(map_intrapb->base == MAP_FAILED)) {
    perror("mmap");
    LOG_ERROR("mmap fail, errno %d\n", errno);
    return -1;
  }
  LOG_PRINTF("[mmap]: intrapb_mem = %p, size = 0x%x\n", map_intrapb->base, map_intrapb->size);
  return 0;
}

static int __s3l_unmap_bsb(int iav_fd, SAmbaDSPMapBSB *map_bsb)
{
  if (map_bsb->base && map_bsb->size) {
    int ret = 0;
    unsigned int map_size = 0;
    if (map_bsb->b_two_times) {
      map_size = map_bsb->size * 2;
    } else {
      map_size = map_bsb->size;
    }
    ret = munmap(map_bsb->base, map_size);
    LOG_PRINTF("[munmap]: bsb_mem = %p, size = 0x%x\n", map_bsb->base, map_bsb->size);
    map_bsb->base = NULL;
    map_bsb->size = 0;
    if (DUnlikely(0 > ret)) {
      perror("munmap");
      LOG_ERROR("munmap fail, errno %d\n", errno);
      return ret;
    }
  } else {
    LOG_ERROR("bad params, %p, %d\n", map_bsb->base, map_bsb->size);
    return (-1);
  }
  return 0;
}

static int __s3l_unmap_dsp(int iav_fd, SAmbaDSPMapDSP *map_dsp)
{
  if (map_dsp->base && map_dsp->size) {
    int ret = 0;
    ret = munmap(map_dsp->base, map_dsp->size);
    LOG_PRINTF("[munmap]: dsp_mem = %p, size = 0x%x\n", map_dsp->base, map_dsp->size);
    map_dsp->base = NULL;
    map_dsp->size = 0;
    if (DUnlikely(0 > ret)) {
      perror("munmap");
      LOG_ERROR("munmap fail, errno %d\n", errno);
      return ret;
    }
  } else {
    LOG_ERROR("bad params, %p, %d\n", map_dsp->base, map_dsp->size);
    return (-1);
  }
  return 0;
}

static int __s3l_unmap_intrapb(int iav_fd, SAmbaDSPMapIntraPB *map_intrapb)
{
  if (map_intrapb->base && map_intrapb->size) {
    int ret = 0;
    ret = munmap(map_intrapb->base, map_intrapb->size);
    LOG_PRINTF("[munmap]: intrapb_mem = %p, size = 0x%x\n", map_intrapb->base, map_intrapb->size);
    map_intrapb->base = NULL;
    map_intrapb->size = 0;
    if (DUnlikely(0 > ret)) {
      perror("munmap");
      LOG_ERROR("munmap fail, errno %d\n", errno);
      return ret;
    }
  } else {
    LOG_ERROR("bad params, %p, %d\n", map_intrapb->base, map_intrapb->size);
    return (-1);
  }
  return 0;
}

static int __s3l_efm_get_bufferpool_info(int iav_fd, SAmbaEFMPoolInfo *buffer_pool_info)
{
  TInt ret = 0;
  struct iav_efm_get_pool_info efm_pool_info;
  memset(&efm_pool_info, 0, sizeof(efm_pool_info));
  ret = ioctl(iav_fd, IAV_IOC_EFM_GET_POOL_INFO, &efm_pool_info);
  if (0 > ret) {
    perror("IAV_IOC_EFM_GET_POOL_INFO");
    LOG_ERROR("IAV_IOC_EFM_GET_POOL_INFO fail, errno %d\n", errno);
    return ret;
  }
  if (buffer_pool_info) {
    buffer_pool_info->yuv_buf_num = efm_pool_info.yuv_buf_num;
    buffer_pool_info->yuv_pitch = efm_pool_info.yuv_pitch;
    buffer_pool_info->me1_buf_num = efm_pool_info.me_buf_num;
    buffer_pool_info->me1_pitch = efm_pool_info.me1_pitch;
    buffer_pool_info->yuv_buf_width = efm_pool_info.yuv_size.width;
    buffer_pool_info->yuv_buf_height = efm_pool_info.yuv_size.height;
    buffer_pool_info->me1_buf_width = efm_pool_info.me1_size.width;
    buffer_pool_info->me1_buf_height = efm_pool_info.me1_size.height;
    buffer_pool_info->b_use_addr = 0;
  } else {
    LOG_FATAL("NULL param\n");
    return (-2);
  }
  return 0;
}

static int __s3l_efm_request_frame(int iav_fd, SAmbaEFMFrame *frame)
{
  TInt ret = 0;
  //TInt max_retry = 10;
  struct iav_efm_request_frame request;
  memset(&request, 0, sizeof(request));
  while (1) {
    ret = ioctl(iav_fd, IAV_IOC_EFM_REQUEST_FRAME, &request);
    if (0 > ret) {
      if (EAGAIN == errno) {
        //max_retry --;
        gfOSmsleep(10);
        continue;
      } else {
        perror("IAV_IOC_EFM_REQUEST_FRAME");
        LOG_ERROR("IAV_IOC_EFM_REQUEST_FRAME fail, errno %d\n", errno);
      }
      return (-1);
    } else {
      break;
    }
  }
  if (frame) {
    frame->frame_idx = request.frame_idx;
    frame->yuv_luma_offset = (TPointer) request.yuv_luma_offset;
    frame->yuv_chroma_offset = (TPointer) request.yuv_chroma_offset;
    frame->me1_offset = (TPointer) request.me1_offset;
  } else {
    LOG_FATAL("NULL param\n");
    return (-2);
  }
  return 0;
}

static int __s3l_efm_finish_frame(int iav_fd, SAmbaEFMFinishFrame *finish_frame)
{
  if (finish_frame) {
    TInt ret = 0;
    if (!finish_frame->b_not_wait_next_interrupt) {
      ret = ioctl(iav_fd, IAV_IOC_WAIT_NEXT_FRAME, 0);
      if (0 > ret) {
        perror("IAV_IOC_WAIT_NEXT_FRAME");
        LOG_ERROR("IAV_IOC_WAIT_NEXT_FRAME fail, errno %d\n", errno);
        return (-1);
      }
    }
    struct iav_efm_handshake_frame handshake;
    memset(&handshake, 0, sizeof(handshake));
    handshake.frame_idx = finish_frame->frame_idx;
    handshake.frame_pts = finish_frame->frame_pts;
    handshake.stream_id = finish_frame->stream_id;
    handshake.is_last_frame = finish_frame->is_last_frame;
    handshake.use_hw_pts = finish_frame->use_hw_pts;
    ret = ioctl(iav_fd, IAV_IOC_EFM_HANDSHAKE_FRAME, &handshake);
    if (0 > ret) {
      perror("IAV_IOC_EFM_REQUEST_FRAME");
      LOG_ERROR("IAV_IOC_EFM_REQUEST_FRAME fail, errno %d\n", errno);
      return (-1);
    }
  } else {
    LOG_FATAL("NULL param\n");
    return (-2);
  }
  return 0;
}

static int __s3l_read_bitstream(int iav_fd, SAmbaDSPReadBitstream *bitstream)
{
  struct iav_querydesc query_desc;
  struct iav_framedesc *frame_desc;
  int ret = 0;
  memset(&query_desc, 0, sizeof(query_desc));
  frame_desc = &query_desc.arg.frame;
  query_desc.qid = IAV_DESC_FRAME;
  //frame_desc->id = bitstream->stream_id;
  frame_desc->id = 0xffffffff;
  frame_desc->time_ms = bitstream->timeout_ms;
  ret = ioctl(iav_fd, IAV_IOC_QUERY_DESC, &query_desc);
  if (ret) {
    if (EAGAIN == errno) {
      return 1;
    }
    perror("IAV_IOC_QUERY_DESC");
    LOG_ERROR("IAV_IOC_QUERY_DESC fail, errno %d\n", errno);
    return (-1);
  }
  bitstream->stream_id = frame_desc->id;
  bitstream->offset = frame_desc->data_addr_offset;
  bitstream->size = frame_desc->size;
  bitstream->pts = frame_desc->arm_pts;
  bitstream->video_width = frame_desc->reso.width;
  bitstream->video_height = frame_desc->reso.height;
  bitstream->slice_id = frame_desc->slice_id;
  bitstream->slice_num = frame_desc->slice_num;
  if (frame_desc->stream_end) {
    DASSERT(!bitstream->size);
    bitstream->size = 0;
    bitstream->offset = 0;
  }
  if (IAV_STREAM_TYPE_H264 == frame_desc->stream_type) {
    bitstream->stream_type = StreamFormat_H264;
  } else if (IAV_STREAM_TYPE_H265 == frame_desc->stream_type) {
    bitstream->stream_type = StreamFormat_H265;
  } else if (IAV_STREAM_TYPE_MJPEG == frame_desc->stream_type) {
    bitstream->stream_type = StreamFormat_JPEG;
  } else {
    bitstream->stream_type = StreamFormat_Invalid;
    LOG_FATAL("bad stream type %d\n", bitstream->stream_type);
    return (-2);
  }
  return 0;
}

static int __s3l_is_ready_for_read_bitstream(int iav_fd)
{
  return 1;
}

static int __s3l_intraplay_reset_buffers(int iav_fd, SAmbaDSPIntraplayResetBuffers *reset_buffers)
{
  TInt ret = 0;
  struct iav_intraplay_reset_buffers reset;
  memset(&reset, 0, sizeof(reset));
  reset.decoder_id = reset_buffers->decoder_id;
  reset.max_num = reset_buffers->max_num;
  ret = ioctl(iav_fd, IAV_IOC_INTRAPLAY_RESET_BUFFERS, &reset);
  if (ret) {
    perror("IAV_IOC_INTRAPLAY_RESET_BUFFERS");
    LOG_ERROR("IAV_IOC_INTRAPLAY_RESET_BUFFERS fail, errno %d\n", errno);
    return ret;
  }
  return 0;
}

static int __s3l_intraplay_request_buffer(int iav_fd, SAmbaDSPIntraplayBuffer *buffer)
{
  TInt ret = 0;
  struct iav_intraplay_frame_buffer buf;
  memset(&buf, 0, sizeof(buf));
  buf.ch_fmt = buffer->ch_fmt;
  buf.buf_width = buffer->buf_width;
  buf.buf_height = buffer->buf_height;
  ret = ioctl(iav_fd, IAV_IOC_INTRAPLAY_REQUEST_BUFFER, &buf);
  if (ret) {
    perror("IOC_INTRAPLAY_REQUEST_BUFFER");
    LOG_ERROR("IOC_INTRAPLAY_REQUEST_BUFFER fail, errno %d\n", errno);
    return ret;
  }
  buffer->buffer_id = buf.buffer_id;
  buffer->ch_fmt = buf.ch_fmt;
  buffer->buf_pitch = buf.buf_pitch;
  buffer->buf_width = buf.buf_width;
  buffer->buf_height = buf.buf_height;
  buffer->lu_buf_offset = buf.lu_buf_offset;
  buffer->ch_buf_offset = buf.ch_buf_offset;
  buffer->img_width = buf.img_width;
  buffer->img_height = buf.img_height;
  buffer->img_offset_x = buf.img_offset_x;
  buffer->img_offset_y = buf.img_offset_y;
  buffer->buffer_size = buf.buffer_size;
  return 0;
}

static int __s3l_intraplay_decode(int iav_fd, SAmbaDSPIntraplayDecode *decode)
{
  TInt ret = 0;
  TU32 i = 0;
  struct iav_intraplay_decode dec;
  memset(&dec, 0, sizeof(dec));
  dec.decoder_id = decode->decoder_id;
  dec.num = decode->num;
  dec.decode_type = decode->decode_type;
  for (i = 0; i < dec.num; i ++) {
    dec.bitstreams[i].bits_fifo_start = decode->bitstreams[i].bits_fifo_start;
    dec.bitstreams[i].bits_fifo_end = decode->bitstreams[i].bits_fifo_end;
    dec.buffers[i].buffer_id = decode->buffers[i].buffer_id;
    dec.buffers[i].ch_fmt = decode->buffers[i].ch_fmt;
    dec.buffers[i].buf_pitch = decode->buffers[i].buf_pitch;
    dec.buffers[i].buf_width = decode->buffers[i].buf_width;
    dec.buffers[i].buf_height = decode->buffers[i].buf_height;
    dec.buffers[i].lu_buf_offset = decode->buffers[i].lu_buf_offset;
    dec.buffers[i].ch_buf_offset = decode->buffers[i].ch_buf_offset;
    dec.buffers[i].img_width = decode->buffers[i].img_width;
    dec.buffers[i].img_height = decode->buffers[i].img_height;
    dec.buffers[i].img_offset_x = decode->buffers[i].img_offset_x;
    dec.buffers[i].img_offset_y = decode->buffers[i].img_offset_y;
    dec.buffers[i].buffer_size = decode->buffers[i].buffer_size;
  }
  ret = ioctl(iav_fd, IAV_IOC_INTRAPLAY_DECODE, &dec);
  if (ret) {
    perror("IAV_IOC_INTRAPLAY_DECODE");
    LOG_ERROR("IAV_IOC_INTRAPLAY_DECODE fail, errno %d\n", errno);
    return ret;
  }
  return 0;
}

static int __s3l_intraplay_yuv2yuv(int iav_fd, SAmbaDSPIntraplayYUV2YUV *yuv2yuv)
{
  TInt ret = 0;
  TU32 i = 0;
  struct iav_intraplay_yuv2yuv yy;
  memset(&yy, 0, sizeof(yy));
  yy.decoder_id = yuv2yuv->decoder_id;
  yy.num = yuv2yuv->num;
  yy.rotate = yuv2yuv->rotate;
  yy.flip = yuv2yuv->flip;
  yy.luma_gain = yuv2yuv->luma_gain;
  yy.src_buf.buffer_id = yuv2yuv->src_buf.buffer_id;
  yy.src_buf.ch_fmt = yuv2yuv->src_buf.ch_fmt;
  yy.src_buf.buf_pitch = yuv2yuv->src_buf.buf_pitch;
  yy.src_buf.buf_width = yuv2yuv->src_buf.buf_width;
  yy.src_buf.buf_height = yuv2yuv->src_buf.buf_height;
  yy.src_buf.lu_buf_offset = yuv2yuv->src_buf.lu_buf_offset;
  yy.src_buf.ch_buf_offset = yuv2yuv->src_buf.ch_buf_offset;
  yy.src_buf.img_width = yuv2yuv->src_buf.img_width;
  yy.src_buf.img_height = yuv2yuv->src_buf.img_height;
  yy.src_buf.img_offset_x = yuv2yuv->src_buf.img_offset_x;
  yy.src_buf.img_offset_y = yuv2yuv->src_buf.img_offset_y;
  yy.src_buf.buffer_size = yuv2yuv->src_buf.buffer_size;
  for (i = 0; i < yy.num; i ++) {
    yy.dst_buf[i].buffer_id = yuv2yuv->dst_buf[i].buffer_id;
    yy.dst_buf[i].ch_fmt = yuv2yuv->dst_buf[i].ch_fmt;
    yy.dst_buf[i].buf_pitch = yuv2yuv->dst_buf[i].buf_pitch;
    yy.dst_buf[i].buf_width = yuv2yuv->dst_buf[i].buf_width;
    yy.dst_buf[i].buf_height = yuv2yuv->dst_buf[i].buf_height;
    yy.dst_buf[i].lu_buf_offset = yuv2yuv->dst_buf[i].lu_buf_offset;
    yy.dst_buf[i].ch_buf_offset = yuv2yuv->dst_buf[i].ch_buf_offset;
    yy.dst_buf[i].img_width = yuv2yuv->dst_buf[i].img_width;
    yy.dst_buf[i].img_height = yuv2yuv->dst_buf[i].img_height;
    yy.dst_buf[i].img_offset_x = yuv2yuv->dst_buf[i].img_offset_x;
    yy.dst_buf[i].img_offset_y = yuv2yuv->dst_buf[i].img_offset_y;
    yy.dst_buf[i].buffer_size = yuv2yuv->dst_buf[i].buffer_size;
  }
  ret = ioctl(iav_fd, IAV_IOC_INTRAPLAY_YUV2YUV, &yy);
  if (ret) {
    perror("IAV_IOC_INTRAPLAY_YUV2YUV");
    LOG_ERROR("IAV_IOC_INTRAPLAY_YUV2YUV fail, errno %d\n", errno);
    return ret;
  }
  return 0;
}

static int __s3l_intraplay_display(int iav_fd, SAmbaDSPIntraplayDisplay *display)
{
  TInt ret = 0;
  TU32 i = 0;
  struct iav_intraplay_display d;
  memset(&d, 0, sizeof(d));
  d.decoder_id = display->decoder_id;
  d.num = display->num;
  for (i = 0; i < d.num; i ++) {
    d.buffers[i].buffer_id = display->buffers[i].buffer_id;
    d.buffers[i].ch_fmt = display->buffers[i].ch_fmt;
    d.buffers[i].buf_pitch = display->buffers[i].buf_pitch;
    d.buffers[i].buf_width = display->buffers[i].buf_width;
    d.buffers[i].buf_height = display->buffers[i].buf_height;
    d.buffers[i].lu_buf_offset = display->buffers[i].lu_buf_offset;
    d.buffers[i].ch_buf_offset = display->buffers[i].ch_buf_offset;
    d.buffers[i].img_width = display->buffers[i].img_width;
    d.buffers[i].img_height = display->buffers[i].img_height;
    d.buffers[i].img_offset_x = display->buffers[i].img_offset_x;
    d.buffers[i].img_offset_y = display->buffers[i].img_offset_y;
    d.buffers[i].buffer_size = display->buffers[i].buffer_size;
    d.desc[i].vout_id = display->desc[i].vout_id;
    d.desc[i].vid_win_update = display->desc[i].vid_win_update;
    d.desc[i].vid_win_rotate = display->desc[i].vid_win_rotate;
    d.desc[i].vid_flip = display->desc[i].vid_flip;
    d.desc[i].vid_win_width = display->desc[i].vid_win_width;
    d.desc[i].vid_win_height = display->desc[i].vid_win_height;
    d.desc[i].vid_win_offset_x = display->desc[i].vid_win_offset_x;
    d.desc[i].vid_win_offset_y = display->desc[i].vid_win_offset_y;
  }
  ret = ioctl(iav_fd, IAV_IOC_INTRAPLAY_DISPLAY, &d);
  if (ret) {
    perror("IAV_IOC_INTRAPLAY_DISPLAY");
    LOG_ERROR("IAV_IOC_INTRAPLAY_DISPLAY fail, errno %d\n", errno);
    return ret;
  }
  return 0;
}

static int __s3l_encode_start(int iav_fd, TU32 mask)
{
  int ret = ioctl(iav_fd, IAV_IOC_START_ENCODE, mask);
  if (ret) {
    perror("IAV_IOC_START_ENCODE");
    LOG_ERROR("IAV_IOC_START_ENCODE fail, errno %d, mask 0x%08x\n", errno, mask);
    return ret;
  }
  return 0;
}

static int __s3l_encode_stop(int iav_fd, TU32 mask)
{
  int ret = ioctl(iav_fd, IAV_IOC_STOP_ENCODE, mask);
  if (ret) {
    perror("IAV_IOC_STOP_ENCODE");
    LOG_ERROR("IAV_IOC_STOP_ENCODE fail, errno %d, mask 0x%08x\n", errno, mask);
    return ret;
  }
  return 0;
}

static int __s3l_query_encode_stream_info(int iav_fd, SAmbaDSPEncStreamInfo *info)
{
  struct iav_stream_info stream_info;
  int ret = 0;
  stream_info.id = info->id;
  ret = ioctl(iav_fd, IAV_IOC_GET_STREAM_INFO, &stream_info);
  if (ret) {
    perror("IAV_IOC_GET_STREAM_INFO");
    LOG_ERROR("IAV_IOC_GET_STREAM_INFO fail, errno %d\n", errno);
    return ret;
  }
  switch (stream_info.state) {
    case IAV_STREAM_STATE_IDLE:
      info->state = EAMDSP_ENC_STREAM_STATE_IDLE;
      break;
    case IAV_STREAM_STATE_STARTING:
      info->state = EAMDSP_ENC_STREAM_STATE_STARTING;
      break;
    case IAV_STREAM_STATE_ENCODING:
      info->state = EAMDSP_ENC_STREAM_STATE_ENCODING;
      break;
    case IAV_STREAM_STATE_STOPPING:
      info->state = EAMDSP_ENC_STREAM_STATE_STOPPING;
      break;
    case IAV_STREAM_STATE_UNKNOWN:
      LOG_ERROR("unknown state\n");
      info->state = EAMDSP_ENC_STREAM_STATE_UNKNOWN;
      return (-1);
      break;
    default:
      LOG_ERROR("unexpected state %d\n", stream_info.state);
      info->state = EAMDSP_ENC_STREAM_STATE_ERROR;
      return (-2);
      break;
  }
  return 0;
}

static int __s3l_query_encode_stream_format(int iav_fd, SAmbaDSPEncStreamFormat *fmt)
{
  struct iav_stream_format format;
  int ret = 0;
  memset(&format, 0, sizeof(format));
  format.id = fmt->id;
  ret = ioctl(iav_fd, IAV_IOC_GET_STREAM_FORMAT, &format);
  if (ret) {
    perror("IAV_IOC_GET_STREAM_FORMAT");
    LOG_ERROR("IAV_IOC_GET_STREAM_FORMAT fail, errno %d\n", errno);
    return ret;
  }
  switch (format.type) {
    case IAV_STREAM_TYPE_H264:
      fmt->codec = EAMDSP_VIDEO_CODEC_TYPE_H264;
      break;
    case IAV_STREAM_TYPE_H265:
      fmt->codec = EAMDSP_VIDEO_CODEC_TYPE_H265;
      break;
    case IAV_STREAM_TYPE_MJPEG:
      fmt->codec = EAMDSP_VIDEO_CODEC_TYPE_MJPEG;
      break;
    case IAV_STREAM_TYPE_NONE:
      LOG_WARN("codec none?\n");
      fmt->codec = EAMDSP_VIDEO_CODEC_TYPE_INVALID;
      break;
    default:
      LOG_ERROR("unexpected codec %d\n", format.type);
      fmt->codec = EAMDSP_VIDEO_CODEC_TYPE_INVALID;
      return (-1);
      break;
  }
  switch (format.buf_id) {
    case IAV_SRCBUF_MN:
      fmt->source_buffer = EAMDSP_ENC_STREAM_SOURCE_BUFFER_MAIN;
      break;
    case IAV_SRCBUF_PC:
      fmt->source_buffer = EAMDSP_ENC_STREAM_SOURCE_BUFFER_SECOND;
      break;
    case IAV_SRCBUF_PB:
      fmt->source_buffer = EAMDSP_ENC_STREAM_SOURCE_BUFFER_THIRD;
      break;
    case IAV_SRCBUF_PA:
      fmt->source_buffer = EAMDSP_ENC_STREAM_SOURCE_BUFFER_FOURTH;
      break;
    case IAV_SRCBUF_PD:
      fmt->source_buffer = EAMDSP_ENC_STREAM_SOURCE_BUFFER_FIFTH;
      break;
    case IAV_SRCBUF_EFM:
      fmt->source_buffer = EAMDSP_ENC_STREAM_SOURCE_BUFFER_EFM;
      break;
    default:
      LOG_ERROR("unexpected source buffer %d\n", format.buf_id);
      fmt->source_buffer = EAMDSP_ENC_STREAM_SOURCE_BUFFER_INVALID;
      return (-2);
      break;
  }
  return 0;
}

static int __s3l_query_source_buffer_info(int iav_fd, SAmbaDSPSourceBufferInfo *info)
{
  struct iav_srcbuf_format buf_format;
  memset(&buf_format, 0, sizeof(buf_format));
  buf_format.buf_id = info->buf_id;
  if (ioctl(iav_fd, IAV_IOC_GET_SOURCE_BUFFER_FORMAT, &buf_format) < 0) {
    perror("IAV_IOC_GET_SOURCE_BUFFER_FORMAT");
    LOG_ERROR("IAV_IOC_QUERY_DESC fail, errno %d\n", errno);
    return (-1);
  }
  info->size_width = buf_format.size.width;
  info->size_height = buf_format.size.height;
  info->crop_size_x = buf_format.input.width;
  info->crop_size_y = buf_format.input.height;
  info->crop_pos_x = buf_format.input.x;
  info->crop_pos_y = buf_format.input.y;
  return 0;
}

static int __s3l_query_yuv_buffer(int iav_fd, SAmbaDSPQueryYUVBuffer *yuv_buffer)
{
  struct iav_yuvbufdesc *yuv_desc;
  struct iav_querydesc query_desc;
  int ret = 0;
  memset(&query_desc, 0, sizeof(query_desc));
  query_desc.qid = IAV_DESC_YUV;
  query_desc.arg.yuv.buf_id = (enum iav_srcbuf_id) yuv_buffer->buf_id;
  ret = ioctl(iav_fd, IAV_IOC_QUERY_DESC, &query_desc);
  if (ret < 0) {
    perror("IAV_IOC_QUERY_DESC");
    LOG_ERROR("IAV_IOC_QUERY_DESC fail, errno %d\n", errno);
    return ret;
  }
  yuv_desc = &query_desc.arg.yuv;
  yuv_buffer->y_addr_offset = yuv_desc->y_addr_offset;
  yuv_buffer->uv_addr_offset = yuv_desc->uv_addr_offset;
  yuv_buffer->width = yuv_desc->width;
  yuv_buffer->height = yuv_desc->height;
  yuv_buffer->pitch = yuv_desc->pitch;
  yuv_buffer->seq_num = yuv_desc->seq_num;
  yuv_buffer->format = yuv_desc->format;
  yuv_buffer->dsp_pts = yuv_desc->dsp_pts;
  yuv_buffer->mono_pts = yuv_desc->mono_pts;
  return 0;
}

static u32 __translate_buffer_type(u32 type)
{
  switch (type) {
    case EAmbaBufferType_DSP:
      return IAV_BUFFER_DSP;
      break;
    case EAmbaBufferType_BSB:
      return IAV_BUFFER_BSB;
      break;
    case EAmbaBufferType_USR:
      return IAV_BUFFER_USR;
      break;
    case EAmbaBufferType_MV:
      return IAV_BUFFER_MV;
      break;
    case EAmbaBufferType_OVERLAY:
      return IAV_BUFFER_OVERLAY;
      break;
    case EAmbaBufferType_QPMATRIX:
      return IAV_BUFFER_QPMATRIX;
      break;
    case EAmbaBufferType_WARP:
      return IAV_BUFFER_WARP;
      break;
    case EAmbaBufferType_QUANT:
      return IAV_BUFFER_QUANT;
      break;
    case EAmbaBufferType_IMG:
      return IAV_BUFFER_IMG;
      break;
    case EAmbaBufferType_PM_IDSP:
      return IAV_BUFFER_PM_IDSP;
      break;
    case EAmbaBufferType_CMD_SYNC:
      return IAV_BUFFER_CMD_SYNC;
      break;
    case EAmbaBufferType_FB_DATA:
      return IAV_BUFFER_FB_DATA;
      break;
    case EAmbaBufferType_FB_AUDIO:
      return IAV_BUFFER_FB_AUDIO;
      break;
    case EAmbaBufferType_QPMATRIX_RAW:
      return IAV_BUFFER_QPMATRIX_RAW;
      break;
    case EAmbaBufferType_INTRA_PB:
      return IAV_BUFFER_INTRA_PB;
      break;
    case EAmbaBufferType_SBP:
      return IAV_BUFFER_SBP;
      break;
    case EAmbaBufferType_MULTI_CHAN:
      return IAV_BUFFER_MULTI_CHAN;
      break;
    default:
      LOG_FATAL("unexpected type %d\n", type);
  }
  return type;
}

static int __s3l_gdma_copy(int iav_fd, SAmbaGDMACopy *copy)
{
  TInt ret = 0;
  struct iav_gdma_copy param;
  param.src_offset = copy->src_offset;
  param.dst_offset = copy->dst_offset;
  param.src_mmap_type = __translate_buffer_type(copy->src_mmap_type);
  param.dst_mmap_type = __translate_buffer_type(copy->dst_mmap_type);
  param.src_pitch = copy->src_pitch;
  param.dst_pitch = copy->dst_pitch;
  param.width = copy->width;
  param.height = copy->height;
  ret = ioctl(iav_fd, IAV_IOC_GDMA_COPY, &param);
  if (ret < 0) {
    perror("IAV_IOC_GDMA_COPY");
    LOG_ERROR("IAV_IOC_GDMA_COPY fail, errno %d\n", errno);
    return ret;
  }
  return 0;
}

static TInt __s3l_dsp_enter_idle_mode(TInt iav_fd)
{
  TInt ret = ioctl(iav_fd, IAV_IOC_ENTER_IDLE);
  if (0 > ret) {
    perror("IAV_IOC_ENTER_IDLE");
    LOG_FATAL("enter idle mode fail, errno %d\n", errno);
  }
  return ret;
}

static void __setup_s3l_al_context(SFAmbaDSPDecAL *al)
{
  al->f_get_dsp_mode = __s3l_get_dsp_mode;
  al->f_enter_mode = __s3l_enter_decode_mode;
  al->f_leave_mode = __s3l_leave_decode_mode;
  al->f_create_decoder = __s3l_create_decoder;
  al->f_destroy_decoder = __s3l_destroy_decoder;
  al->f_query_decode_config = __s3l_query_decode_config;
  al->f_trickplay = __s3l_decode_trick_play;
  al->f_start = __s3l_decode_start;
  al->f_stop = __s3l_decode_stop;
  al->f_speed = __s3l_decode_speed;
  al->f_request_bsb = __s3l_decode_request_bits_fifo;
  al->f_decode = __s3l_decode;
  al->f_query_print_decode_bsb_status = __s3l_decode_query_bsb_status_and_print;
  al->f_query_print_decode_status = __s3l_decode_query_status_and_print;
  al->f_query_decode_bsb_status = __s3l_decode_query_bsb_status;
  al->f_query_decode_status = __s3l_decode_query_status;
  al->f_decode_wait_vout_dormant = NULL;
  al->f_decode_wake_vout = NULL;
  al->f_decode_wait_eos_flag = NULL;
  al->f_decode_wait_eos = NULL;
  al->f_configure_vout = __configure_vout;
  al->f_get_vout_info = __get_single_vout_info;
  al->f_get_vin_info = __s3l_get_vin_info;
  al->f_get_stream_framefactor = __s3l_get_stream_framefactor;
  al->f_map_bsb = __s3l_map_bsb;
  al->f_map_dsp = __s3l_map_dsp;
  al->f_map_intrapb = __s3l_map_intrapb;
  al->f_unmap_bsb = __s3l_unmap_bsb;
  al->f_unmap_dsp = __s3l_unmap_dsp;
  al->f_unmap_intrapb = __s3l_unmap_intrapb;
  al->f_efm_get_buffer_pool_info = __s3l_efm_get_bufferpool_info;
  al->f_efm_request_frame = __s3l_efm_request_frame;
  al->f_efm_finish_frame = __s3l_efm_finish_frame;
  al->f_read_bitstream = __s3l_read_bitstream;
  al->f_is_ready_for_read_bitstream = __s3l_is_ready_for_read_bitstream;
  al->f_intraplay_reset_buffers = __s3l_intraplay_reset_buffers;
  al->f_intraplay_request_buffer = __s3l_intraplay_request_buffer;
  al->f_intraplay_decode = __s3l_intraplay_decode;
  al->f_intraplay_yuv2yuv = __s3l_intraplay_yuv2yuv;
  al->f_intraplay_display = __s3l_intraplay_display;
  al->f_encode_start = __s3l_encode_start;
  al->f_encode_stop = __s3l_encode_stop;
  al->f_query_encode_stream_info = __s3l_query_encode_stream_info;
  al->f_query_encode_stream_fmt = __s3l_query_encode_stream_format;
  al->f_query_source_buffer_info = __s3l_query_source_buffer_info;
  al->f_query_yuv_buffer = __s3l_query_yuv_buffer;
  al->f_gdma_copy = __s3l_gdma_copy;
  al->f_enter_idle_mode = __s3l_dsp_enter_idle_mode;
}

#elif defined (BUILD_DSP_AMBA_S5)

static TInt __s5_get_dsp_mode(TInt iav_fd, SAmbaDSPMode *mode)
{
  int state = 0;
  int ret = 0;
  ret = ioctl(iav_fd, IAV_IOC_GET_IAV_STATE, &state);
  if (0 > ret) {
    perror("IAV_IOC_GET_IAV_STATE");
    LOG_FATAL("IAV_IOC_GET_IAV_STATE fail, errno %d\n", errno);
    return ret;
  }
  switch (state) {
    case IAV_STATE_INIT:
      mode->dsp_mode = EAMDSP_MODE_INIT;
      break;
    case IAV_STATE_IDLE:
      mode->dsp_mode = EAMDSP_MODE_IDLE;
      break;
    case IAV_STATE_PREVIEW:
      mode->dsp_mode = EAMDSP_MODE_PREVIEW;
      break;
    case IAV_STATE_ENCODING:
      mode->dsp_mode = EAMDSP_MODE_ENCODE;
      break;
    case IAV_STATE_DECODING:
      mode->dsp_mode = EAMDSP_MODE_DECODE;
      break;
    default:
      LOG_FATAL("un expected dsp mode %d\n", state);
      mode->dsp_mode = EAMDSP_MODE_INVALID;
      break;
  }
  return 0;
}

static TInt __s5_enter_decode_mode(TInt iav_fd, SAmbaDSPDecodeModeConfig *mode_config)
{
  struct iav_decode_mode_config decode_mode;
  TInt i = 0;
  memset(&decode_mode, 0x0, sizeof(decode_mode));
  if (mode_config->num_decoder > DAMBADSP_MAX_DECODER_NUMBER) {
    LOG_FATAL("BAD num_decoder %d\n", mode_config->num_decoder);
    return (-100);
  }
  decode_mode.num_decoder = mode_config->num_decoder;
  decode_mode.max_frm_width = mode_config->max_frm_width;
  decode_mode.max_frm_height = mode_config->max_frm_height;
  decode_mode.max_vout0_width = mode_config->max_vout0_width;
  decode_mode.max_vout0_height = mode_config->max_vout0_height;
  decode_mode.max_vout1_width = mode_config->max_vout1_width;
  decode_mode.max_vout1_height = mode_config->max_vout1_height;
  decode_mode.b_support_ff_fb_bw = mode_config->b_support_ff_fb_bw;
  decode_mode.debug_max_frame_per_interrupt = mode_config->debug_max_frame_per_interrupt;
  decode_mode.debug_use_dproc = mode_config->debug_use_dproc;
  i = ioctl(iav_fd, IAV_IOC_ENTER_DECODE_MODE, &decode_mode);
  if (0 > i) {
    perror("IAV_IOC_ENTER_DECODE_MODE");
    LOG_FATAL("enter decode mode fail, errno %d\n", errno);
    return i;
  }
  return 0;
}

static TInt __s5_leave_decode_mode(TInt iav_fd)
{
  TInt ret = ioctl(iav_fd, IAV_IOC_LEAVE_DECODE_MODE);
  if (0 > ret) {
    perror("IAV_IOC_LEAVE_DECODE_MODE");
    LOG_FATAL("leave decode mode fail, errno %d\n", errno);
  }
  return ret;
}

static TInt __s5_create_decoder(TInt iav_fd, SAmbaDSPDecoderInfo *p_decoder_info)
{
  TInt i = 0;
  struct iav_decoder_info decoder_info;
  memset(&decoder_info, 0x0, sizeof(decoder_info));
  decoder_info.decoder_id = p_decoder_info->decoder_id;
  if (EAMDSP_VIDEO_CODEC_TYPE_H264 == p_decoder_info->decoder_type) {
    decoder_info.decoder_type = IAV_DECODER_TYPE_H264;
  } else if (EAMDSP_VIDEO_CODEC_TYPE_H265 == p_decoder_info->decoder_type) {
    decoder_info.decoder_type = IAV_DECODER_TYPE_H265;
  } else {
    LOG_FATAL("bad video codec type %d\n", p_decoder_info->decoder_type);
    return (-101);
  }
  decoder_info.num_vout = p_decoder_info->num_vout;
  decoder_info.width = p_decoder_info->width;
  decoder_info.height = p_decoder_info->height;
  if (decoder_info.num_vout > DAMBADSP_MAX_VOUT_NUMBER) {
    LOG_FATAL("BAD num_vout %d\n", p_decoder_info->num_vout);
    return (-100);
  }
  for (i = 0; i < decoder_info.num_vout; i ++) {
    decoder_info.vout_configs[i].vout_id = p_decoder_info->vout_configs[i].vout_id;
    decoder_info.vout_configs[i].enable = p_decoder_info->vout_configs[i].enable;
    decoder_info.vout_configs[i].flip = p_decoder_info->vout_configs[i].flip;
    decoder_info.vout_configs[i].rotate = p_decoder_info->vout_configs[i].rotate;
    decoder_info.vout_configs[i].target_win_offset_x = p_decoder_info->vout_configs[i].target_win_offset_x;
    decoder_info.vout_configs[i].target_win_offset_y = p_decoder_info->vout_configs[i].target_win_offset_y;
    decoder_info.vout_configs[i].target_win_width = p_decoder_info->vout_configs[i].target_win_width;
    decoder_info.vout_configs[i].target_win_height = p_decoder_info->vout_configs[i].target_win_height;
    decoder_info.vout_configs[i].zoom_factor_x = p_decoder_info->vout_configs[i].zoom_factor_x;
    decoder_info.vout_configs[i].zoom_factor_y = p_decoder_info->vout_configs[i].zoom_factor_y;
    decoder_info.vout_configs[i].vout_mode = p_decoder_info->vout_configs[i].vout_mode;
    LOG_PRINTF("vout(%d), w %d, h %d, zoom x %08x, y %08x\n", i, decoder_info.vout_configs[i].target_win_width, decoder_info.vout_configs[i].target_win_height, decoder_info.vout_configs[i].zoom_factor_x, decoder_info.vout_configs[i].zoom_factor_y);
  }
  decoder_info.bsb_start_offset = p_decoder_info->bsb_start_offset;
  decoder_info.bsb_size = p_decoder_info->bsb_size;
  i = ioctl(iav_fd, IAV_IOC_CREATE_DECODER, &decoder_info);
  if (0 > i) {
    perror("IAV_IOC_CREATE_DECODER");
    LOG_FATAL("create decoder fail, errno %d\n", errno);
    return i;
  }
  p_decoder_info->bsb_start_offset = decoder_info.bsb_start_offset;
  p_decoder_info->bsb_size = decoder_info.bsb_size;
  return 0;
}

static TInt __s5_destroy_decoder(TInt iav_fd, TU8 decoder_id)
{
  TInt ret = ioctl(iav_fd, IAV_IOC_DESTROY_DECODER, decoder_id);
  if (0 > ret) {
    perror("IAV_IOC_DESTROY_DECODER");
    LOG_FATAL("destroy decoder fail, errno %d\n", errno);
  }
  return ret;
}

static TInt __s5_query_decode_config(int iav_fd, SAmbaDSPQueryDecodeConfig *config)
{
  if (config) {
    config->auto_map_bsb = 0;
    config->rendering_monitor_mode = 0;
  } else {
    LOG_FATAL("NULL\n");
    return (-1);
  }
  return 0;
}

static int __s5_decode_trick_play(int iav_fd, TU8 decoder_id, TU8 trick_play)
{
  int ret;
  struct iav_decode_trick_play trickplay;
  trickplay.decoder_id = decoder_id;
  trickplay.trick_play = trick_play;
  ret = ioctl(iav_fd, IAV_IOC_DECODE_TRICK_PLAY, &trickplay);
  if (0 > ret) {
    if (EACCES == errno) {
      LOG_NOTICE("stopped\n");
      return DDECODER_STOPPED;
    }
    perror("IAV_IOC_DECODE_TRICK_PLAY");
    LOG_ERROR("trickplay error, errno %d\n", errno);
    return ret;
  }
  return 0;
}

static int __s5_decode_start(int iav_fd, TU8 decoder_id)
{
  int ret = ioctl(iav_fd, IAV_IOC_DECODE_START, decoder_id);
  if (ret < 0) {
    perror("IAV_IOC_DECODE_START");
    LOG_ERROR("decode start error, errno %d\n", errno);
    return ret;
  }
  return 0;
}

static int __s5_decode_stop(int iav_fd, TU8 decoder_id, TU8 stop_flag)
{
  int ret;
  struct iav_decode_stop stop;
  stop.decoder_id = decoder_id;
  stop.stop_flag = stop_flag;
  ret = ioctl(iav_fd, IAV_IOC_DECODE_STOP, &stop);
  if (0 > ret) {
    perror("IAV_IOC_DECODE_STOP");
    LOG_ERROR("decode stop error, errno %d\n", errno);
    return ret;
  }
  return 0;
}

static int __s5_decode_speed(int iav_fd, TU8 decoder_id, TU16 speed, TU8 scan_mode, TU8 direction)
{
  int ret;
  struct iav_decode_speed spd;
  spd.decoder_id = decoder_id;
  spd.direction = direction;
  spd.speed = speed;
  spd.scan_mode = scan_mode;
  //LOG_PRINTF("speed, direction %d, speed %x, scan mode %d\n", spd.direction, spd.speed, spd.scan_mode);
  ret = ioctl(iav_fd, IAV_IOC_DECODE_SPEED, &spd);
  if (0 > ret) {
    if (EACCES == errno) {
      LOG_NOTICE("stopped\n");
      return DDECODER_STOPPED;
    }
    perror("IAV_IOC_DECODE_SPEED");
    LOG_ERROR("decode speed error, errno %d\n", errno);
    return ret;
  }
  return 0;
}

static int __s5_decode_request_bits_fifo(int iav_fd, int decoder_id, TU32 size, void *cur_pos_offset)
{
  struct iav_decode_bsb wait;
  int ret;
  unsigned long tmp = (unsigned long) cur_pos_offset;
  wait.decoder_id = decoder_id;
  wait.room = size;
  wait.start_offset = (TU32) tmp;
  ret = ioctl(iav_fd, IAV_IOC_WAIT_DECODE_BSB, &wait);
  if (0 > ret) {
    if (EACCES == errno) {
      LOG_NOTICE("stopped\n");
      return DDECODER_STOPPED;
    }
    LOG_ERROR("IAV_IOC_WAIT_DECODE_BSB fail, errno %d.\n", errno);
    perror("IAV_IOC_WAIT_DECODE_BSB");
    return ret;
  }
  return 0;
}

static int __s5_decode(int iav_fd, SAmbaDSPDecode *dec)
{
  TInt ret = 0;
  struct iav_decode_video decode_video;
  memset(&decode_video, 0, sizeof(decode_video));
  decode_video.decoder_id = dec->decoder_id;
  decode_video.num_frames = dec->num_frames;
  decode_video.start_ptr_offset = dec->start_ptr_offset;
  decode_video.end_ptr_offset = dec->end_ptr_offset;
  decode_video.first_frame_display = dec->first_frame_display;
  ret = ioctl(iav_fd, IAV_IOC_DECODE_VIDEO, &decode_video);
  if (0 > ret) {
    if (EACCES == errno) {
      LOG_NOTICE("stopped\n");
      return DDECODER_STOPPED;
    }
    perror("IAV_IOC_DECODE_VIDEO");
    LOG_ERROR("IAV_IOC_DECODE_VIDEO fail, errno %d.\n", errno);
    return ret;
  }
  return 0;
}

static int __s5_decode_query_bsb_status_and_print(int iav_fd, TU8 decoder_id)
{
  int ret;
  struct iav_decode_bsb bsb;
  memset(&bsb, 0x0, sizeof(bsb));
  bsb.decoder_id = decoder_id;
  ret = ioctl(iav_fd, IAV_IOC_QUERY_DECODE_BSB, &bsb);
  if (0 > ret) {
    if (EACCES == errno) {
      LOG_NOTICE("stopped\n");
      return DDECODER_STOPPED;
    }
    LOG_ERROR("IAV_IOC_QUERY_DECODE_BSB fail, errno %d.\n", errno);
    perror("IAV_IOC_QUERY_DECODE_BSB");
    return ret;
  }
  LOG_PRINTF("[bsb]: current write offset (arm) 0x%08x, current read offset (dsp) 0x%08x, safe room (minus 256 bytes) %d, free room %d\n", bsb.start_offset, bsb.dsp_read_offset, bsb.room, bsb.free_room);
  return 0;
}

static int __s5_decode_query_status_and_print(int iav_fd, TU8 decoder_id)
{
  int ret;
  struct iav_decode_status status;
  memset(&status, 0x0, sizeof(status));
  status.decoder_id = decoder_id;
  ret = ioctl(iav_fd, IAV_IOC_QUERY_DECODE_STATUS, &status);
  if (0 > ret) {
    if (EACCES == errno) {
      LOG_NOTICE("stopped\n");
      return DDECODER_STOPPED;
    }
    perror("IAV_IOC_QUERY_DECODE_STATUS");
    LOG_ERROR("IAV_IOC_QUERY_DECODE_STATUS fail, errno %d.\n", errno);
    return ret;
  }
  LOG_PRINTF("[decode status]: decode_state %d, decoded_pic_number %d, error_status %d, total_error_count %d, irq_count %d\n", status.decode_state, status.decoded_pic_number, status.error_status, status.total_error_count, status.irq_count);
  LOG_PRINTF("[decode status, bsb]: current write offset (arm) 0x%08x, current read offset (dsp) 0x%08x, safe room (minus 256 bytes) %d, free room %d\n", status.write_offset, status.dsp_read_offset, status.room, status.free_room);
  LOG_PRINTF("[decode status, last pts]: %d, is_started %d, is_send_stop_cmd %d\n", status.last_pts, status.is_started, status.is_send_stop_cmd);
  LOG_PRINTF("[decode status, yuv addr]: yuv422_y 0x%08x, yuv422_uv 0x%08x\n", status.yuv422_y_addr, status.yuv422_uv_addr);
  return 0;
}

static int __s5_decode_query_bsb_status(int iav_fd, SAmbaDSPBSBStatus *status)
{
  int ret;
  struct iav_decode_bsb bsb;
  memset(&bsb, 0x0, sizeof(bsb));
  bsb.decoder_id = status->decoder_id;
  ret = ioctl(iav_fd, IAV_IOC_QUERY_DECODE_BSB, &bsb);
  if (0 > ret) {
    if (EACCES == errno) {
      LOG_NOTICE("stopped\n");
      return DDECODER_STOPPED;
    }
    LOG_ERROR("IAV_IOC_QUERY_DECODE_BSB fail, errno %d.\n", errno);
    perror("IAV_IOC_QUERY_DECODE_BSB");
    return ret;
  }
  status->start_offset = bsb.start_offset;
  status->room = bsb.room;
  status->dsp_read_offset = bsb.dsp_read_offset;
  status->free_room = bsb.free_room;
  return 0;
}

static int __s5_decode_query_status(int iav_fd, SAmbaDSPDecodeStatus *status)
{
  int ret;
  struct iav_decode_status dec_status;
  memset(&dec_status, 0x0, sizeof(dec_status));
  dec_status.decoder_id = status->decoder_id;
  ret = ioctl(iav_fd, IAV_IOC_QUERY_DECODE_STATUS, &dec_status);
  if (0 > ret) {
    if (EACCES == errno) {
      LOG_NOTICE("stopped\n");
      return DDECODER_STOPPED;
    }
    perror("IAV_IOC_QUERY_DECODE_STATUS");
    LOG_ERROR("IAV_IOC_QUERY_DECODE_STATUS fail, errno %d.\n", errno);
    return ret;
  }
  status->is_started = dec_status.is_started;
  status->is_send_stop_cmd = dec_status.is_send_stop_cmd;
  status->last_pts = dec_status.last_pts;
  status->decode_state = dec_status.decode_state;
  status->error_status = dec_status.error_status;
  status->total_error_count = dec_status.total_error_count;
  status->decoded_pic_number = dec_status.decoded_pic_number;
  status->write_offset = dec_status.write_offset;
  status->room = dec_status.room;
  status->dsp_read_offset = dec_status.dsp_read_offset;
  status->free_room = dec_status.free_room;
  status->irq_count = dec_status.irq_count;
  status->yuv422_y_addr = dec_status.yuv422_y_addr;
  status->yuv422_uv_addr = dec_status.yuv422_uv_addr;
  return 0;
}

static int __s5_get_vin_info(int iav_fd, SAmbaDSPVinInfo *vininfo)
{
  struct vindev_video_info vin_info;
  struct vindev_fps active_fps;
  TU32 fps_q9 = 1;
  int ret = 0;
  memset(&vin_info, 0x0, sizeof(vin_info));
  vin_info.vsrc_id = 0;
  vin_info.info.mode = AMBA_VIDEO_MODE_CURRENT;
  ret = ioctl(iav_fd, IAV_IOC_VIN_GET_VIDEOINFO, &vin_info);
  if (0 > ret) {
    perror("IAV_IOC_VIN_GET_VIDEOINFO");
    LOG_WARN("IAV_IOC_VIN_GET_VIDEOINFO fail, errno %d\n", errno);
    return ret;
  }
  memset(&active_fps, 0, sizeof(active_fps));
  active_fps.vsrc_id = 0;
  ret = ioctl(iav_fd, IAV_IOC_VIN_GET_FPS, &active_fps);
  if (0 > ret) {
    perror("IAV_IOC_VIN_GET_FPS");
    LOG_WARN("IAV_IOC_VIN_GET_FPS fail, errno %d\n", errno);
    return ret;
  }
  fps_q9 = active_fps.fps;
  __parse_fps(fps_q9, vininfo);
  vininfo->width = vin_info.info.width;
  vininfo->height = vin_info.info.height;
  vininfo->format = vin_info.info.format;
  vininfo->type = vin_info.info.type;
  vininfo->bits = vin_info.info.bits;
  vininfo->ratio = vin_info.info.ratio;
  vininfo->system = vin_info.info.system;
  vininfo->flip = vin_info.info.flip;
  vininfo->rotate = vin_info.info.rotate;
  return 0;
}

static int __s5_get_stream_framefactor(int iav_fd, int index, SAmbaDSPStreamFramefactor *framefactor)
{
  struct iav_stream_cfg streamcfg;
  int ret = 0;
  if (DMaxStreamNumber <= index) {
    LOG_FATAL("index(%d) not as expected\n", index);
    return (-10);
  }
  memset(&streamcfg, 0, sizeof(streamcfg));
  streamcfg.id = index;
  streamcfg.cid = IAV_STMCFG_FPS;
  ret = ioctl(iav_fd, IAV_IOC_GET_STREAM_CONFIG, &streamcfg);
  if (0 > ret) {
    perror("IAV_IOC_GET_STREAM_CONFIG");
    LOG_ERROR("IAV_IOC_GET_STREAM_CONFIG fail, errno %d\n", errno);
    return ret;
  }
  framefactor->framefactor_num = streamcfg.arg.fps.fps_multi;
  framefactor->framefactor_den = streamcfg.arg.fps.fps_div;
  return 0;
}

static int __s5_map_bsb(int iav_fd, SAmbaDSPMapBSB *map_bsb)
{
  int ret = 0;
  unsigned int map_size = 0;
  struct iav_querybuf querybuf;
  querybuf.buf = IAV_BUFFER_BSB;
  ret = ioctl(iav_fd, IAV_IOC_QUERY_BUF, &querybuf);
  if (0 > ret) {
    perror("IAV_IOC_QUERY_BUF");
    LOG_ERROR("IAV_IOC_QUERY_BUF fail, errno %d\n", errno);
    return ret;
  }
  map_bsb->size = querybuf.length;
  if (map_bsb->b_two_times) {
    map_size = querybuf.length * 2;
  } else {
    map_size = querybuf.length;
  }
  if (map_bsb->b_enable_read && map_bsb->b_enable_write) {
    map_bsb->base = mmap(NULL, map_size, PROT_WRITE | PROT_READ, MAP_SHARED, iav_fd, querybuf.offset);
  } else if (map_bsb->b_enable_read && !map_bsb->b_enable_write) {
    map_bsb->base = mmap(NULL, map_size, PROT_READ, MAP_SHARED, iav_fd, querybuf.offset);
  } else if (!map_bsb->b_enable_read && map_bsb->b_enable_write) {
    map_bsb->base = mmap(NULL, map_size, PROT_WRITE, MAP_SHARED, iav_fd, querybuf.offset);
  } else {
    LOG_ERROR("not read or write\n");
    return (-1);
  }
  if (map_bsb->base == MAP_FAILED) {
    perror("mmap");
    LOG_ERROR("mmap fail\n");
    return -1;
  }
  LOG_PRINTF("[mmap]: bsb_mem = %p, size = 0x%x\n", map_bsb->base, map_bsb->size);
  return 0;
}

static int __s5_map_dsp(int iav_fd, SAmbaDSPMapDSP *map_dsp)
{
  TInt ret = 0;
  struct iav_querybuf querybuf;
  querybuf.buf = IAV_BUFFER_DSP;
  ret = ioctl(iav_fd, IAV_IOC_QUERY_BUF, &querybuf);
  if (DUnlikely(0 > ret)) {
    perror("IAV_IOC_QUERY_BUF");
    LOG_ERROR("IAV_IOC_QUERY_BUF fail, errno %d\n", errno);
    return ret;
  }
  map_dsp->size = querybuf.length;
  map_dsp->base = mmap(NULL, map_dsp->size, PROT_READ | PROT_WRITE, MAP_SHARED, iav_fd, querybuf.offset);
  if (DUnlikely(map_dsp->base == MAP_FAILED)) {
    perror("mmap");
    LOG_ERROR("mmap fail, errno %d\n", errno);
    return -1;
  }
  LOG_PRINTF("[mmap]: dsp_mem = %p, size = 0x%x\n", map_dsp->base, map_dsp->size);
  return 0;
}

static int __s5_map_intrapb(int iav_fd, SAmbaDSPMapIntraPB *map_intrapb)
{
  TInt ret = 0;
  struct iav_querybuf querybuf;
  querybuf.buf = IAV_BUFFER_INTRA_PB;
  ret = ioctl(iav_fd, IAV_IOC_QUERY_BUF, &querybuf);
  if (DUnlikely(0 > ret)) {
    perror("IAV_IOC_QUERY_BUF");
    LOG_ERROR("IAV_IOC_QUERY_BUF fail, errno %d\n", errno);
    return ret;
  }
  map_intrapb->size = querybuf.length;
  map_intrapb->base = mmap(NULL, map_intrapb->size, PROT_READ | PROT_WRITE, MAP_SHARED, iav_fd, querybuf.offset);
  if (DUnlikely(map_intrapb->base == MAP_FAILED)) {
    perror("mmap");
    LOG_ERROR("mmap fail, errno %d\n", errno);
    return -1;
  }
  LOG_PRINTF("[mmap]: intrapb_mem = %p, size = 0x%x\n", map_intrapb->base, map_intrapb->size);
  return 0;
}

static int __s5_unmap_bsb(int iav_fd, SAmbaDSPMapBSB *map_bsb)
{
  if (map_bsb->base && map_bsb->size) {
    int ret = 0;
    unsigned int map_size = 0;
    if (map_bsb->b_two_times) {
      map_size = map_bsb->size * 2;
    } else {
      map_size = map_bsb->size;
    }
    ret = munmap(map_bsb->base, map_size);
    LOG_PRINTF("[munmap]: bsb_mem = %p, size = 0x%x\n", map_bsb->base, map_bsb->size);
    map_bsb->base = NULL;
    map_bsb->size = 0;
    if (DUnlikely(0 > ret)) {
      perror("munmap");
      LOG_ERROR("munmap fail, errno %d\n", errno);
      return ret;
    }
  } else {
    LOG_ERROR("bad params, %p, %d\n", map_bsb->base, map_bsb->size);
    return (-1);
  }
  return 0;
}

static int __s5_unmap_dsp(int iav_fd, SAmbaDSPMapDSP *map_dsp)
{
  if (map_dsp->base && map_dsp->size) {
    int ret = 0;
    ret = munmap(map_dsp->base, map_dsp->size);
    LOG_PRINTF("[munmap]: dsp_mem = %p, size = 0x%x\n", map_dsp->base, map_dsp->size);
    map_dsp->base = NULL;
    map_dsp->size = 0;
    if (DUnlikely(0 > ret)) {
      perror("munmap");
      LOG_ERROR("munmap fail, errno %d\n", errno);
      return ret;
    }
  } else {
    LOG_ERROR("bad params, %p, %d\n", map_dsp->base, map_dsp->size);
    return (-1);
  }
  return 0;
}

static int __s5_unmap_intrapb(int iav_fd, SAmbaDSPMapIntraPB *map_intrapb)
{
  if (map_intrapb->base && map_intrapb->size) {
    int ret = 0;
    ret = munmap(map_intrapb->base, map_intrapb->size);
    LOG_PRINTF("[munmap]: intrapb_mem = %p, size = 0x%x\n", map_intrapb->base, map_intrapb->size);
    map_intrapb->base = NULL;
    map_intrapb->size = 0;
    if (DUnlikely(0 > ret)) {
      perror("munmap");
      LOG_ERROR("munmap fail, errno %d\n", errno);
      return ret;
    }
  } else {
    LOG_ERROR("bad params, %p, %d\n", map_intrapb->base, map_intrapb->size);
    return (-1);
  }
  return 0;
}

static int __s5_efm_get_bufferpool_info(int iav_fd, SAmbaEFMPoolInfo *buffer_pool_info)
{
  TInt ret = 0;
  struct iav_efm_get_pool_info efm_pool_info;
  memset(&efm_pool_info, 0, sizeof(efm_pool_info));
  ret = ioctl(iav_fd, IAV_IOC_EFM_GET_POOL_INFO, &efm_pool_info);
  if (0 > ret) {
    perror("IAV_IOC_EFM_GET_POOL_INFO");
    LOG_ERROR("IAV_IOC_EFM_GET_POOL_INFO fail, errno %d\n", errno);
    return ret;
  }
  if (buffer_pool_info) {
    buffer_pool_info->yuv_buf_num = efm_pool_info.yuv_buf_num;
    buffer_pool_info->yuv_pitch = efm_pool_info.yuv_pitch;
    buffer_pool_info->me1_buf_num = efm_pool_info.me_buf_num;
    buffer_pool_info->me1_pitch = efm_pool_info.me1_pitch;
    buffer_pool_info->yuv_buf_width = efm_pool_info.yuv_size.width;
    buffer_pool_info->yuv_buf_height = efm_pool_info.yuv_size.height;
    buffer_pool_info->me1_buf_width = efm_pool_info.me1_size.width;
    buffer_pool_info->me1_buf_height = efm_pool_info.me1_size.height;
    buffer_pool_info->b_use_addr = 0;
  } else {
    LOG_FATAL("NULL param\n");
    return (-2);
  }
  return 0;
}

static int __s5_efm_request_frame(int iav_fd, SAmbaEFMFrame *frame)
{
  TInt ret = 0;
  struct iav_efm_request_frame request;
  memset(&request, 0, sizeof(request));
  ret = ioctl(iav_fd, IAV_IOC_EFM_REQUEST_FRAME, &request);
  if (0 > ret) {
    perror("IAV_IOC_EFM_REQUEST_FRAME");
    LOG_ERROR("IAV_IOC_EFM_REQUEST_FRAME fail, errno %d\n", errno);
    return (-1);
  }
  if (frame) {
    frame->frame_idx = request.frame_idx;
    frame->yuv_luma_offset = (TPointer) request.yuv_luma_offset;
    frame->yuv_chroma_offset = (TPointer) request.yuv_chroma_offset;
    frame->me1_offset = (TPointer) request.me1_offset;
  } else {
    LOG_FATAL("NULL param\n");
    return (-2);
  }
  return 0;
}

static int __s5_efm_finish_frame(int iav_fd, SAmbaEFMFinishFrame *finish_frame)
{
  if (finish_frame) {
    TInt ret = 0;
    if (!finish_frame->b_not_wait_next_interrupt) {
      ret = ioctl(iav_fd, IAV_IOC_WAIT_NEXT_FRAME, 0);
      if (0 > ret) {
        perror("IAV_IOC_WAIT_NEXT_FRAME");
        LOG_ERROR("IAV_IOC_WAIT_NEXT_FRAME fail, errno %d\n", errno);
        return (-1);
      }
    }
    struct iav_efm_handshake_frame handshake;
    memset(&handshake, 0, sizeof(handshake));
    handshake.frame_idx = finish_frame->frame_idx;
    handshake.frame_pts = finish_frame->frame_pts;
    handshake.stream_id = finish_frame->stream_id;
    ret = ioctl(iav_fd, IAV_IOC_EFM_HANDSHAKE_FRAME, &handshake);
    if (0 > ret) {
      perror("IAV_IOC_EFM_REQUEST_FRAME");
      LOG_ERROR("IAV_IOC_EFM_REQUEST_FRAME fail, errno %d\n", errno);
      return (-1);
    }
  } else {
    LOG_FATAL("NULL param\n");
    return (-2);
  }
  return 0;
}

static int __s5_read_bitstream(int iav_fd, SAmbaDSPReadBitstream *bitstream)
{
  struct iav_querydesc query_desc;
  struct iav_framedesc *frame_desc;
  int ret = 0;
  memset(&query_desc, 0, sizeof(query_desc));
  frame_desc = &query_desc.arg.frame;
  query_desc.qid = IAV_DESC_FRAME;
  //frame_desc->id = bitstream->stream_id;
  frame_desc->id = 0xffffffff;
  frame_desc->time_ms = bitstream->timeout_ms;
  ret = ioctl(iav_fd, IAV_IOC_QUERY_DESC, &query_desc);
  if (ret) {
    if (EAGAIN == errno) {
      return 1;
    }
    perror("IAV_IOC_QUERY_DESC");
    LOG_ERROR("IAV_IOC_QUERY_DESC fail, errno %d\n", errno);
    return (-1);
  }
  bitstream->stream_id = frame_desc->id;
  bitstream->offset = frame_desc->data_addr_offset;
  bitstream->size = frame_desc->size;
  bitstream->pts = frame_desc->arm_pts;
  bitstream->video_width = frame_desc->reso.width;
  bitstream->video_height = frame_desc->reso.height;
  bitstream->slice_id = frame_desc->slice_id;
  bitstream->slice_num = frame_desc->slice_num;
  if (frame_desc->stream_end) {
    DASSERT(!bitstream->size);
    bitstream->size = 0;
    bitstream->offset = 0;
  }
  if (IAV_STREAM_TYPE_H264 == frame_desc->stream_type) {
    bitstream->stream_type = StreamFormat_H264;
  } else if (IAV_STREAM_TYPE_H265 == frame_desc->stream_type) {
    bitstream->stream_type = StreamFormat_H265;
  } else if (IAV_STREAM_TYPE_MJPEG == frame_desc->stream_type) {
    bitstream->stream_type = StreamFormat_JPEG;
  } else {
    bitstream->stream_type = StreamFormat_Invalid;
    LOG_FATAL("bad stream type %d\n", bitstream->stream_type);
    return (-2);
  }
  return 0;
}

static int __s5_is_ready_for_read_bitstream(int iav_fd)
{
  return 1;
}

static int __s5_intraplay_reset_buffers(int iav_fd, SAmbaDSPIntraplayResetBuffers *reset_buffers)
{
  TInt ret = 0;
  struct iav_intraplay_reset_buffers reset;
  memset(&reset, 0, sizeof(reset));
  reset.decoder_id = reset_buffers->decoder_id;
  reset.max_num = reset_buffers->max_num;
  ret = ioctl(iav_fd, IAV_IOC_INTRAPLAY_RESET_BUFFERS, &reset);
  if (ret) {
    perror("IAV_IOC_INTRAPLAY_RESET_BUFFERS");
    LOG_ERROR("IAV_IOC_INTRAPLAY_RESET_BUFFERS fail, errno %d\n", errno);
    return ret;
  }
  return 0;
}

static int __s5_intraplay_request_buffer(int iav_fd, SAmbaDSPIntraplayBuffer *buffer)
{
  TInt ret = 0;
  struct iav_intraplay_frame_buffer buf;
  memset(&buf, 0, sizeof(buf));
  buf.ch_fmt = buffer->ch_fmt;
  buf.buf_width = buffer->buf_width;
  buf.buf_height = buffer->buf_height;
  ret = ioctl(iav_fd, IAV_IOC_INTRAPLAY_REQUEST_BUFFER, &buf);
  if (ret) {
    perror("IOC_INTRAPLAY_REQUEST_BUFFER");
    LOG_ERROR("IOC_INTRAPLAY_REQUEST_BUFFER fail, errno %d\n", errno);
    return ret;
  }
  buffer->buffer_id = buf.buffer_id;
  buffer->ch_fmt = buf.ch_fmt;
  buffer->buf_pitch = buf.buf_pitch;
  buffer->buf_width = buf.buf_width;
  buffer->buf_height = buf.buf_height;
  buffer->lu_buf_offset = buf.lu_buf_offset;
  buffer->ch_buf_offset = buf.ch_buf_offset;
  buffer->img_width = buf.img_width;
  buffer->img_height = buf.img_height;
  buffer->img_offset_x = buf.img_offset_x;
  buffer->img_offset_y = buf.img_offset_y;
  buffer->buffer_size = buf.buffer_size;
  return 0;
}

static int __s5_intraplay_decode(int iav_fd, SAmbaDSPIntraplayDecode *decode)
{
  TInt ret = 0;
  TU32 i = 0;
  struct iav_intraplay_decode dec;
  memset(&dec, 0, sizeof(dec));
  dec.decoder_id = decode->decoder_id;
  dec.num = decode->num;
  dec.decode_type = decode->decode_type;
  for (i = 0; i < dec.num; i ++) {
    dec.bitstreams[i].bits_fifo_start = decode->bitstreams[i].bits_fifo_start;
    dec.bitstreams[i].bits_fifo_end = decode->bitstreams[i].bits_fifo_end;
    dec.buffers[i].buffer_id = decode->buffers[i].buffer_id;
    dec.buffers[i].ch_fmt = decode->buffers[i].ch_fmt;
    dec.buffers[i].buf_pitch = decode->buffers[i].buf_pitch;
    dec.buffers[i].buf_width = decode->buffers[i].buf_width;
    dec.buffers[i].buf_height = decode->buffers[i].buf_height;
    dec.buffers[i].lu_buf_offset = decode->buffers[i].lu_buf_offset;
    dec.buffers[i].ch_buf_offset = decode->buffers[i].ch_buf_offset;
    dec.buffers[i].img_width = decode->buffers[i].img_width;
    dec.buffers[i].img_height = decode->buffers[i].img_height;
    dec.buffers[i].img_offset_x = decode->buffers[i].img_offset_x;
    dec.buffers[i].img_offset_y = decode->buffers[i].img_offset_y;
    dec.buffers[i].buffer_size = decode->buffers[i].buffer_size;
  }
  ret = ioctl(iav_fd, IAV_IOC_INTRAPLAY_DECODE, &dec);
  if (ret) {
    perror("IAV_IOC_INTRAPLAY_DECODE");
    LOG_ERROR("IAV_IOC_INTRAPLAY_DECODE fail, errno %d\n", errno);
    return ret;
  }
  return 0;
}

static int __s5_intraplay_yuv2yuv(int iav_fd, SAmbaDSPIntraplayYUV2YUV *yuv2yuv)
{
  TInt ret = 0;
  TU32 i = 0;
  struct iav_intraplay_yuv2yuv yy;
  memset(&yy, 0, sizeof(yy));
  yy.decoder_id = yuv2yuv->decoder_id;
  yy.num = yuv2yuv->num;
  yy.rotate = yuv2yuv->rotate;
  yy.flip = yuv2yuv->flip;
  yy.luma_gain = yuv2yuv->luma_gain;
  yy.src_buf.buffer_id = yuv2yuv->src_buf.buffer_id;
  yy.src_buf.ch_fmt = yuv2yuv->src_buf.ch_fmt;
  yy.src_buf.buf_pitch = yuv2yuv->src_buf.buf_pitch;
  yy.src_buf.buf_width = yuv2yuv->src_buf.buf_width;
  yy.src_buf.buf_height = yuv2yuv->src_buf.buf_height;
  yy.src_buf.lu_buf_offset = yuv2yuv->src_buf.lu_buf_offset;
  yy.src_buf.ch_buf_offset = yuv2yuv->src_buf.ch_buf_offset;
  yy.src_buf.img_width = yuv2yuv->src_buf.img_width;
  yy.src_buf.img_height = yuv2yuv->src_buf.img_height;
  yy.src_buf.img_offset_x = yuv2yuv->src_buf.img_offset_x;
  yy.src_buf.img_offset_y = yuv2yuv->src_buf.img_offset_y;
  yy.src_buf.buffer_size = yuv2yuv->src_buf.buffer_size;
  for (i = 0; i < yy.num; i ++) {
    yy.dst_buf[i].buffer_id = yuv2yuv->dst_buf[i].buffer_id;
    yy.dst_buf[i].ch_fmt = yuv2yuv->dst_buf[i].ch_fmt;
    yy.dst_buf[i].buf_pitch = yuv2yuv->dst_buf[i].buf_pitch;
    yy.dst_buf[i].buf_width = yuv2yuv->dst_buf[i].buf_width;
    yy.dst_buf[i].buf_height = yuv2yuv->dst_buf[i].buf_height;
    yy.dst_buf[i].lu_buf_offset = yuv2yuv->dst_buf[i].lu_buf_offset;
    yy.dst_buf[i].ch_buf_offset = yuv2yuv->dst_buf[i].ch_buf_offset;
    yy.dst_buf[i].img_width = yuv2yuv->dst_buf[i].img_width;
    yy.dst_buf[i].img_height = yuv2yuv->dst_buf[i].img_height;
    yy.dst_buf[i].img_offset_x = yuv2yuv->dst_buf[i].img_offset_x;
    yy.dst_buf[i].img_offset_y = yuv2yuv->dst_buf[i].img_offset_y;
    yy.dst_buf[i].buffer_size = yuv2yuv->dst_buf[i].buffer_size;
  }
  ret = ioctl(iav_fd, IAV_IOC_INTRAPLAY_YUV2YUV, &yy);
  if (ret) {
    perror("IAV_IOC_INTRAPLAY_YUV2YUV");
    LOG_ERROR("IAV_IOC_INTRAPLAY_YUV2YUV fail, errno %d\n", errno);
    return ret;
  }
  return 0;
}

static int __s5_intraplay_display(int iav_fd, SAmbaDSPIntraplayDisplay *display)
{
  TInt ret = 0;
  TU32 i = 0;
  struct iav_intraplay_display d;
  memset(&d, 0, sizeof(d));
  d.decoder_id = display->decoder_id;
  d.num = display->num;
  for (i = 0; i < d.num; i ++) {
    d.buffers[i].buffer_id = display->buffers[i].buffer_id;
    d.buffers[i].ch_fmt = display->buffers[i].ch_fmt;
    d.buffers[i].buf_pitch = display->buffers[i].buf_pitch;
    d.buffers[i].buf_width = display->buffers[i].buf_width;
    d.buffers[i].buf_height = display->buffers[i].buf_height;
    d.buffers[i].lu_buf_offset = display->buffers[i].lu_buf_offset;
    d.buffers[i].ch_buf_offset = display->buffers[i].ch_buf_offset;
    d.buffers[i].img_width = display->buffers[i].img_width;
    d.buffers[i].img_height = display->buffers[i].img_height;
    d.buffers[i].img_offset_x = display->buffers[i].img_offset_x;
    d.buffers[i].img_offset_y = display->buffers[i].img_offset_y;
    d.buffers[i].buffer_size = display->buffers[i].buffer_size;
    d.desc[i].vout_id = display->desc[i].vout_id;
    d.desc[i].vid_win_update = display->desc[i].vid_win_update;
    d.desc[i].vid_win_rotate = display->desc[i].vid_win_rotate;
    d.desc[i].vid_flip = display->desc[i].vid_flip;
    d.desc[i].vid_win_width = display->desc[i].vid_win_width;
    d.desc[i].vid_win_height = display->desc[i].vid_win_height;
    d.desc[i].vid_win_offset_x = display->desc[i].vid_win_offset_x;
    d.desc[i].vid_win_offset_y = display->desc[i].vid_win_offset_y;
  }
  ret = ioctl(iav_fd, IAV_IOC_INTRAPLAY_DISPLAY, &d);
  if (ret) {
    perror("IAV_IOC_INTRAPLAY_DISPLAY");
    LOG_ERROR("IAV_IOC_INTRAPLAY_DISPLAY fail, errno %d\n", errno);
    return ret;
  }
  return 0;
}

static int __s5_encode_start(int iav_fd, TU32 mask)
{
  int ret = ioctl(iav_fd, IAV_IOC_START_ENCODE, mask);
  if (ret) {
    perror("IAV_IOC_START_ENCODE");
    LOG_ERROR("IAV_IOC_START_ENCODE fail, errno %d, mask 0x%08x\n", errno, mask);
    return ret;
  }
  return 0;
}

static int __s5_encode_stop(int iav_fd, TU32 mask)
{
  int ret = ioctl(iav_fd, IAV_IOC_STOP_ENCODE, mask);
  if (ret) {
    perror("IAV_IOC_STOP_ENCODE");
    LOG_ERROR("IAV_IOC_STOP_ENCODE fail, errno %d, mask 0x%08x\n", errno, mask);
    return ret;
  }
  return 0;
}

static int __s5_query_encode_stream_info(int iav_fd, SAmbaDSPEncStreamInfo *info)
{
  struct iav_stream_info stream_info;
  int ret = 0;
  stream_info.id = info->id;
  ret = ioctl(iav_fd, IAV_IOC_GET_STREAM_INFO, &stream_info);
  if (ret) {
    perror("IAV_IOC_GET_STREAM_INFO");
    LOG_ERROR("IAV_IOC_GET_STREAM_INFO fail, errno %d\n", errno);
    return ret;
  }
  switch (stream_info.state) {
    case IAV_STREAM_STATE_IDLE:
      info->state = EAMDSP_ENC_STREAM_STATE_IDLE;
      break;
    case IAV_STREAM_STATE_STARTING:
      info->state = EAMDSP_ENC_STREAM_STATE_STARTING;
      break;
    case IAV_STREAM_STATE_ENCODING:
      info->state = EAMDSP_ENC_STREAM_STATE_ENCODING;
      break;
    case IAV_STREAM_STATE_STOPPING:
      info->state = EAMDSP_ENC_STREAM_STATE_STOPPING;
      break;
    case IAV_STREAM_STATE_UNKNOWN:
      LOG_ERROR("unknown state\n");
      info->state = EAMDSP_ENC_STREAM_STATE_UNKNOWN;
      return (-1);
      break;
    default:
      LOG_ERROR("unexpected state %d\n", stream_info.state);
      info->state = EAMDSP_ENC_STREAM_STATE_ERROR;
      return (-2);
      break;
  }
  return 0;
}

static int __s5_query_encode_stream_format(int iav_fd, SAmbaDSPEncStreamFormat *fmt)
{
  struct iav_stream_format format;
  int ret = 0;
  memset(&format, 0, sizeof(format));
  format.id = fmt->id;
  ret = ioctl(iav_fd, IAV_IOC_GET_STREAM_FORMAT, &format);
  if (ret) {
    perror("IAV_IOC_GET_STREAM_FORMAT");
    LOG_ERROR("IAV_IOC_GET_STREAM_FORMAT fail, errno %d\n", errno);
    return ret;
  }
  switch (format.type) {
    case IAV_STREAM_TYPE_H264:
      fmt->codec = EAMDSP_VIDEO_CODEC_TYPE_H264;
      break;
    case IAV_STREAM_TYPE_H265:
      fmt->codec = EAMDSP_VIDEO_CODEC_TYPE_H265;
      break;
    case IAV_STREAM_TYPE_MJPEG:
      fmt->codec = EAMDSP_VIDEO_CODEC_TYPE_MJPEG;
      break;
    case IAV_STREAM_TYPE_NONE:
      LOG_WARN("codec none?\n");
      fmt->codec = EAMDSP_VIDEO_CODEC_TYPE_INVALID;
      break;
    default:
      LOG_ERROR("unexpected codec %d\n", format.type);
      fmt->codec = EAMDSP_VIDEO_CODEC_TYPE_INVALID;
      return (-1);
      break;
  }
  switch (format.buf_id) {
    case IAV_SRCBUF_MN:
      fmt->source_buffer = EAMDSP_ENC_STREAM_SOURCE_BUFFER_MAIN;
      break;
    case IAV_SRCBUF_PC:
      fmt->source_buffer = EAMDSP_ENC_STREAM_SOURCE_BUFFER_SECOND;
      break;
    case IAV_SRCBUF_PB:
      fmt->source_buffer = EAMDSP_ENC_STREAM_SOURCE_BUFFER_THIRD;
      break;
    case IAV_SRCBUF_PA:
      fmt->source_buffer = EAMDSP_ENC_STREAM_SOURCE_BUFFER_FOURTH;
      break;
    case IAV_SRCBUF_PD:
      fmt->source_buffer = EAMDSP_ENC_STREAM_SOURCE_BUFFER_FIFTH;
      break;
    case IAV_SRCBUF_EFM:
      fmt->source_buffer = EAMDSP_ENC_STREAM_SOURCE_BUFFER_EFM;
      break;
    default:
      LOG_ERROR("unexpected source buffer %d\n", format.buf_id);
      fmt->source_buffer = EAMDSP_ENC_STREAM_SOURCE_BUFFER_INVALID;
      return (-2);
      break;
  }
  return 0;
}

static int __s5_query_source_buffer_info(int iav_fd, SAmbaDSPSourceBufferInfo *info)
{
  struct iav_srcbuf_format buf_format;
  memset(&buf_format, 0, sizeof(buf_format));
  buf_format.buf_id = info->buf_id;
  if (ioctl(iav_fd, IAV_IOC_GET_SOURCE_BUFFER_FORMAT, &buf_format) < 0) {
    perror("IAV_IOC_GET_SOURCE_BUFFER_FORMAT");
    LOG_ERROR("IAV_IOC_QUERY_DESC fail, errno %d\n", errno);
    return (-1);
  }
  info->size_width = buf_format.size.width;
  info->size_height = buf_format.size.height;
  info->crop_size_x = buf_format.input.width;
  info->crop_size_y = buf_format.input.height;
  info->crop_pos_x = buf_format.input.x;
  info->crop_pos_y = buf_format.input.y;
  return 0;
}

static int __s5_query_yuv_buffer(int iav_fd, SAmbaDSPQueryYUVBuffer *yuv_buffer)
{
  struct iav_yuvbufdesc *yuv_desc;
  struct iav_querydesc query_desc;
  int ret = 0;
  memset(&query_desc, 0, sizeof(query_desc));
  query_desc.qid = IAV_DESC_YUV;
  query_desc.arg.yuv.buf_id = (enum iav_srcbuf_id) yuv_buffer->buf_id;
  ret = ioctl(iav_fd, IAV_IOC_QUERY_DESC, &query_desc);
  if (ret < 0) {
    perror("IAV_IOC_QUERY_DESC");
    LOG_ERROR("IAV_IOC_QUERY_DESC fail, errno %d\n", errno);
    return ret;
  }
  yuv_desc = &query_desc.arg.yuv;
  yuv_buffer->y_addr_offset = yuv_desc->y_addr_offset;
  yuv_buffer->uv_addr_offset = yuv_desc->uv_addr_offset;
  yuv_buffer->width = yuv_desc->width;
  yuv_buffer->height = yuv_desc->height;
  yuv_buffer->pitch = yuv_desc->pitch;
  yuv_buffer->seq_num = yuv_desc->seq_num;
  yuv_buffer->format = yuv_desc->format;
  yuv_buffer->dsp_pts = yuv_desc->dsp_pts;
  yuv_buffer->mono_pts = yuv_desc->mono_pts;
  return 0;
}

static u32 __translate_buffer_type(u32 type)
{
  switch (type) {
    case EAmbaBufferType_DSP:
      return IAV_BUFFER_DSP;
      break;
    case EAmbaBufferType_BSB:
      return IAV_BUFFER_BSB;
      break;
    case EAmbaBufferType_USR:
      return IAV_BUFFER_USR;
      break;
    case EAmbaBufferType_MV:
      return IAV_BUFFER_MV;
      break;
    case EAmbaBufferType_OVERLAY:
      return IAV_BUFFER_OVERLAY;
      break;
    case EAmbaBufferType_QPMATRIX:
      return IAV_BUFFER_QPMATRIX;
      break;
    case EAmbaBufferType_WARP:
      return IAV_BUFFER_WARP;
      break;
    case EAmbaBufferType_QUANT:
      return IAV_BUFFER_QUANT;
      break;
    case EAmbaBufferType_IMG:
      return IAV_BUFFER_IMG;
      break;
    case EAmbaBufferType_PM_IDSP:
      return IAV_BUFFER_PM_IDSP;
      break;
    case EAmbaBufferType_FB_DATA:
      return IAV_BUFFER_FB_DATA;
      break;
    case EAmbaBufferType_FB_AUDIO:
      return IAV_BUFFER_FB_AUDIO;
      break;
    case EAmbaBufferType_QPMATRIX_RAW:
      return IAV_BUFFER_QPMATRIX_RAW;
      break;
    case EAmbaBufferType_INTRA_PB:
      return IAV_BUFFER_INTRA_PB;
      break;
    default:
      LOG_FATAL("unexpected type %d\n", type);
  }
  return type;
}

static int __s5_gdma_copy(int iav_fd, SAmbaGDMACopy *copy)
{
  TInt ret = 0;
  struct iav_gdma_copy param;
  param.src_offset = copy->src_offset;
  param.dst_offset = copy->dst_offset;
  param.src_mmap_type = __translate_buffer_type(copy->src_mmap_type);
  param.dst_mmap_type = __translate_buffer_type(copy->dst_mmap_type);
  param.src_pitch = copy->src_pitch;
  param.dst_pitch = copy->dst_pitch;
  param.width = copy->width;
  param.height = copy->height;
  ret = ioctl(iav_fd, IAV_IOC_GDMA_COPY, &param);
  if (ret < 0) {
    perror("IAV_IOC_GDMA_COPY");
    LOG_ERROR("IAV_IOC_GDMA_COPY fail, errno %d\n", errno);
    return ret;
  }
  return 0;
}

static TInt __s5_dsp_enter_idle_mode(TInt iav_fd)
{
  TInt ret = ioctl(iav_fd, IAV_IOC_ENTER_IDLE);
  if (0 > ret) {
    perror("IAV_IOC_ENTER_IDLE");
    LOG_FATAL("enter idle mode fail, errno %d\n", errno);
  }
  return ret;
}

static void __setup_s5_al_context(SFAmbaDSPDecAL *al)
{
  al->f_get_dsp_mode = __s5_get_dsp_mode;
  al->f_enter_mode = __s5_enter_decode_mode;
  al->f_leave_mode = __s5_leave_decode_mode;
  al->f_create_decoder = __s5_create_decoder;
  al->f_destroy_decoder = __s5_destroy_decoder;
  al->f_query_decode_config = __s5_query_decode_config;
  al->f_trickplay = __s5_decode_trick_play;
  al->f_start = __s5_decode_start;
  al->f_stop = __s5_decode_stop;
  al->f_speed = __s5_decode_speed;
  al->f_request_bsb = __s5_decode_request_bits_fifo;
  al->f_decode = __s5_decode;
  al->f_query_print_decode_bsb_status = __s5_decode_query_bsb_status_and_print;
  al->f_query_print_decode_status = __s5_decode_query_status_and_print;
  al->f_query_decode_bsb_status = __s5_decode_query_bsb_status;
  al->f_query_decode_status = __s5_decode_query_status;
  al->f_decode_wait_vout_dormant = NULL;
  al->f_decode_wake_vout = NULL;
  al->f_decode_wait_eos_flag = NULL;
  al->f_decode_wait_eos = NULL;
  al->f_configure_vout = __configure_vout;
  al->f_get_vout_info = __get_single_vout_info;
  al->f_get_vin_info = __s5_get_vin_info;
  al->f_get_stream_framefactor = __s5_get_stream_framefactor;
  al->f_map_bsb = __s5_map_bsb;
  al->f_map_dsp = __s5_map_dsp;
  al->f_map_intrapb = __s5_map_intrapb;
  al->f_unmap_bsb = __s5_unmap_bsb;
  al->f_unmap_dsp = __s5_unmap_dsp;
  al->f_unmap_intrapb = __s5_unmap_intrapb;
  al->f_efm_get_buffer_pool_info = __s5_efm_get_bufferpool_info;
  al->f_efm_request_frame = __s5_efm_request_frame;
  al->f_efm_finish_frame = __s5_efm_finish_frame;
  al->f_read_bitstream = __s5_read_bitstream;
  al->f_is_ready_for_read_bitstream = __s5_is_ready_for_read_bitstream;
  al->f_intraplay_reset_buffers = __s5_intraplay_reset_buffers;
  al->f_intraplay_request_buffer = __s5_intraplay_request_buffer;
  al->f_intraplay_decode = __s5_intraplay_decode;
  al->f_intraplay_yuv2yuv = __s5_intraplay_yuv2yuv;
  al->f_intraplay_display = __s5_intraplay_display;
  al->f_encode_start = __s5_encode_start;
  al->f_encode_stop = __s5_encode_stop;
  al->f_query_encode_stream_info = __s5_query_encode_stream_info;
  al->f_query_encode_stream_fmt = __s5_query_encode_stream_format;
  al->f_query_source_buffer_info = __s5_query_source_buffer_info;
  al->f_query_yuv_buffer = __s5_query_yuv_buffer;
  al->f_gdma_copy = __s5_gdma_copy;
  al->f_enter_idle_mode = __s5_dsp_enter_idle_mode;
}

#elif defined (BUILD_DSP_AMBA_S5L)

static TInt __s5l_get_dsp_mode(TInt iav_fd, SAmbaDSPMode *mode)
{
  int state = 0;
  int ret = 0;
  ret = ioctl(iav_fd, IAV_IOC_GET_IAV_STATE, &state);
  if (0 > ret) {
    perror("IAV_IOC_GET_IAV_STATE");
    LOG_FATAL("IAV_IOC_GET_IAV_STATE fail, errno %d\n", errno);
    return ret;
  }
  switch (state) {
  case IAV_STATE_INIT:
    mode->dsp_mode = EAMDSP_MODE_INIT;
    break;
  case IAV_STATE_IDLE:
    mode->dsp_mode = EAMDSP_MODE_IDLE;
    break;
  case IAV_STATE_PREVIEW:
    mode->dsp_mode = EAMDSP_MODE_PREVIEW;
    break;
  case IAV_STATE_ENCODING:
    mode->dsp_mode = EAMDSP_MODE_ENCODE;
    break;
  case IAV_STATE_DECODING:
    mode->dsp_mode = EAMDSP_MODE_DECODE;
    break;
  default:
    LOG_FATAL("un expected dsp mode %d\n", state);
    mode->dsp_mode = EAMDSP_MODE_INVALID;
    break;
  }
  return 0;
}

static TInt __s5l_enter_decode_mode(TInt iav_fd, SAmbaDSPDecodeModeConfig *mode_config)
{
  struct iav_decode_mode_config decode_mode;
  TInt i = 0;
  memset(&decode_mode, 0x0, sizeof(decode_mode));
  if (mode_config->num_decoder > DAMBADSP_MAX_DECODER_NUMBER) {
    LOG_FATAL("BAD num_decoder %d\n", mode_config->num_decoder);
    return (-100);
  }
  decode_mode.num_decoder = mode_config->num_decoder;
  decode_mode.max_frm_width = mode_config->max_frm_width;
  decode_mode.max_frm_height = mode_config->max_frm_height;
  decode_mode.max_vout0_width = mode_config->max_vout0_width;
  decode_mode.max_vout0_height = mode_config->max_vout0_height;
  decode_mode.max_vout1_width = mode_config->max_vout1_width;
  decode_mode.max_vout1_height = mode_config->max_vout1_height;
  decode_mode.b_support_ff_fb_bw = mode_config->b_support_ff_fb_bw;
  decode_mode.max_n_to_m_ratio = mode_config->max_gop_size;
  decode_mode.debug_max_frame_per_interrupt = mode_config->debug_max_frame_per_interrupt;
  decode_mode.debug_use_dproc = mode_config->debug_use_dproc;
  i = ioctl(iav_fd, IAV_IOC_ENTER_DECODE_MODE, &decode_mode);
  if (0 > i) {
    perror("IAV_IOC_ENTER_DECODE_MODE");
    LOG_FATAL("enter decode mode fail, errno %d\n", errno);
    return i;
  }
  return 0;
}

static TInt __s5l_leave_decode_mode(TInt iav_fd)
{
  TInt ret = ioctl(iav_fd, IAV_IOC_LEAVE_DECODE_MODE);
  if (0 > ret) {
    perror("IAV_IOC_LEAVE_DECODE_MODE");
    LOG_FATAL("leave decode mode fail, errno %d\n", errno);
  }
  return ret;
}

static TInt __s5l_create_decoder(TInt iav_fd, SAmbaDSPDecoderInfo *p_decoder_info)
{
  TInt i = 0;
  struct iav_decoder_info decoder_info;
  memset(&decoder_info, 0x0, sizeof(decoder_info));
  decoder_info.decoder_id = p_decoder_info->decoder_id;
  if (EAMDSP_VIDEO_CODEC_TYPE_H264 == p_decoder_info->decoder_type) {
    decoder_info.decoder_type = IAV_DECODER_TYPE_H264;
  } else if (EAMDSP_VIDEO_CODEC_TYPE_H265 == p_decoder_info->decoder_type) {
    decoder_info.decoder_type = IAV_DECODER_TYPE_H265;
  } else {
    LOG_FATAL("bad video codec type %d\n", p_decoder_info->decoder_type);
    return (-101);
  }
  decoder_info.num_vout = p_decoder_info->num_vout;
  decoder_info.width = p_decoder_info->width;
  decoder_info.height = p_decoder_info->height;
  if (decoder_info.num_vout > DAMBADSP_MAX_VOUT_NUMBER) {
    LOG_FATAL("BAD num_vout %d\n", p_decoder_info->num_vout);
    return (-100);
  }
  for (i = 0; i < decoder_info.num_vout; i ++) {
    decoder_info.vout_configs[i].vout_id = p_decoder_info->vout_configs[i].vout_id;
    decoder_info.vout_configs[i].enable = p_decoder_info->vout_configs[i].enable;
    decoder_info.vout_configs[i].flip = p_decoder_info->vout_configs[i].flip;
    decoder_info.vout_configs[i].rotate = p_decoder_info->vout_configs[i].rotate;
    decoder_info.vout_configs[i].target_win_offset_x = p_decoder_info->vout_configs[i].target_win_offset_x;
    decoder_info.vout_configs[i].target_win_offset_y = p_decoder_info->vout_configs[i].target_win_offset_y;
    decoder_info.vout_configs[i].target_win_width = p_decoder_info->vout_configs[i].target_win_width;
    decoder_info.vout_configs[i].target_win_height = p_decoder_info->vout_configs[i].target_win_height;
    decoder_info.vout_configs[i].zoom_factor_x = p_decoder_info->vout_configs[i].zoom_factor_x;
    decoder_info.vout_configs[i].zoom_factor_y = p_decoder_info->vout_configs[i].zoom_factor_y;
    decoder_info.vout_configs[i].vout_mode = p_decoder_info->vout_configs[i].vout_mode;
    LOG_PRINTF("vout(%d), w %d, h %d, zoom x %08x, y %08x\n", i, decoder_info.vout_configs[i].target_win_width, decoder_info.vout_configs[i].target_win_height, decoder_info.vout_configs[i].zoom_factor_x, decoder_info.vout_configs[i].zoom_factor_y);
  }
  decoder_info.bsb_start_offset = p_decoder_info->bsb_start_offset;
  decoder_info.bsb_size = p_decoder_info->bsb_size;
  i = ioctl(iav_fd, IAV_IOC_CREATE_DECODER, &decoder_info);
  if (0 > i) {
    perror("IAV_IOC_CREATE_DECODER");
    LOG_FATAL("create decoder fail, errno %d\n", errno);
    return i;
  }
  p_decoder_info->bsb_start_offset = decoder_info.bsb_start_offset;
  p_decoder_info->bsb_size = decoder_info.bsb_size;
  return 0;
}

static TInt __s5l_destroy_decoder(TInt iav_fd, TU8 decoder_id)
{
  TInt ret = ioctl(iav_fd, IAV_IOC_DESTROY_DECODER, decoder_id);
  if (0 > ret) {
    perror("IAV_IOC_DESTROY_DECODER");
    LOG_FATAL("destroy decoder fail, errno %d\n", errno);
  }
  return ret;
}

static TInt __s5l_query_decode_config(int iav_fd, SAmbaDSPQueryDecodeConfig *config)
{
  if (config) {
    config->auto_map_bsb = 0;
    config->rendering_monitor_mode = 0;
  } else {
    LOG_FATAL("NULL\n");
    return (-1);
  }
  return 0;
}

static int __s5l_decode_trick_play(int iav_fd, TU8 decoder_id, TU8 trick_play)
{
  int ret;
  struct iav_decode_trick_play trickplay;
  trickplay.decoder_id = decoder_id;
  trickplay.trick_play = trick_play;
  ret = ioctl(iav_fd, IAV_IOC_DECODE_TRICK_PLAY, &trickplay);
  if (0 > ret) {
    if (EACCES == errno) {
      LOG_NOTICE("stopped\n");
      return DDECODER_STOPPED;
    }
    perror("IAV_IOC_DECODE_TRICK_PLAY");
    LOG_ERROR("trickplay error, errno %d\n", errno);
    return ret;
  }
  return 0;
}

static int __s5l_decode_start(int iav_fd, TU8 decoder_id)
{
  int ret = ioctl(iav_fd, IAV_IOC_DECODE_START, decoder_id);
  if (ret < 0) {
    perror("IAV_IOC_DECODE_START");
    LOG_ERROR("decode start error, errno %d\n", errno);
    return ret;
  }
  return 0;
}

static int __s5l_decode_stop(int iav_fd, TU8 decoder_id, TU8 stop_flag)
{
  int ret;
  struct iav_decode_stop stop;
  stop.decoder_id = decoder_id;
  stop.stop_flag = stop_flag;
  ret = ioctl(iav_fd, IAV_IOC_DECODE_STOP, &stop);
  if (0 > ret) {
    perror("IAV_IOC_DECODE_STOP");
    LOG_ERROR("decode stop error, errno %d\n", errno);
    return ret;
  }
  return 0;
}

static int __s5l_decode_speed(int iav_fd, TU8 decoder_id, TU16 speed, TU8 scan_mode, TU8 direction)
{
  int ret;
  struct iav_decode_speed spd;
  spd.decoder_id = decoder_id;
  spd.direction = direction;
  spd.speed = speed;
  spd.scan_mode = scan_mode;
  //LOG_PRINTF("speed, direction %d, speed %x, scan mode %d\n", spd.direction, spd.speed, spd.scan_mode);
  ret = ioctl(iav_fd, IAV_IOC_DECODE_SPEED, &spd);
  if (0 > ret) {
    if (EACCES == errno) {
      LOG_NOTICE("stopped\n");
      return DDECODER_STOPPED;
    }
    perror("IAV_IOC_DECODE_SPEED");
    LOG_ERROR("decode speed error, errno %d\n", errno);
    return ret;
  }
  return 0;
}

static int __s5l_decode_request_bits_fifo(int iav_fd, int decoder_id, TU32 size, void *cur_pos_offset)
{
  struct iav_decode_bsb wait;
  int ret;
  wait.decoder_id = decoder_id;
  wait.room = size;
  wait.start_offset = (TU32) (TPointer) cur_pos_offset;
  ret = ioctl(iav_fd, IAV_IOC_WAIT_DECODE_BSB, &wait);
  if (0 > ret) {
    if (EACCES == errno) {
      LOG_NOTICE("stopped\n");
      return DDECODER_STOPPED;
    }
    LOG_ERROR("IAV_IOC_WAIT_DECODE_BSB fail, errno %d.\n", errno);
    perror("IAV_IOC_WAIT_DECODE_BSB");
    return ret;
  }
  return 0;
}

static int __s5l_decode(int iav_fd, SAmbaDSPDecode *dec)
{
  TInt ret = 0;
  struct iav_decode_video decode_video;
  memset(&decode_video, 0, sizeof(decode_video));
  decode_video.decoder_id = dec->decoder_id;
  decode_video.num_frames = dec->num_frames;
  decode_video.start_ptr_offset = dec->start_ptr_offset;
  decode_video.end_ptr_offset = dec->end_ptr_offset;
  decode_video.first_frame_display = dec->first_frame_display;
  ret = ioctl(iav_fd, IAV_IOC_DECODE_VIDEO, &decode_video);
  if (0 > ret) {
    if (EACCES == errno) {
      LOG_NOTICE("stopped\n");
      return DDECODER_STOPPED;
    }
    perror("IAV_IOC_DECODE_VIDEO");
    LOG_ERROR("IAV_IOC_DECODE_VIDEO fail, errno %d.\n", errno);
    return ret;
  }
  return 0;
}

static int __s5l_decode_query_bsb_status_and_print(int iav_fd, TU8 decoder_id)
{
  int ret;
  struct iav_decode_bsb bsb;
  memset(&bsb, 0x0, sizeof(bsb));
  bsb.decoder_id = decoder_id;
  ret = ioctl(iav_fd, IAV_IOC_QUERY_DECODE_BSB, &bsb);
  if (0 > ret) {
    if (EACCES == errno) {
      LOG_NOTICE("stopped\n");
      return DDECODER_STOPPED;
    }
    LOG_ERROR("IAV_IOC_QUERY_DECODE_BSB fail, errno %d.\n", errno);
    perror("IAV_IOC_QUERY_DECODE_BSB");
    return ret;
  }
  LOG_PRINTF("[bsb]: current write offset (arm) 0x%08x, current read offset (dsp) 0x%08x, safe room (minus 256 bytes) %d, free room %d\n", bsb.start_offset, bsb.dsp_read_offset, bsb.room, bsb.free_room);
  return 0;
}

static int __s5l_decode_query_status_and_print(int iav_fd, TU8 decoder_id)
{
  int ret;
  struct iav_decode_status status;
  memset(&status, 0x0, sizeof(status));
  status.decoder_id = decoder_id;
  ret = ioctl(iav_fd, IAV_IOC_QUERY_DECODE_STATUS, &status);
  if (0 > ret) {
    if (EACCES == errno) {
      LOG_NOTICE("stopped\n");
      return DDECODER_STOPPED;
    }
    perror("IAV_IOC_QUERY_DECODE_STATUS");
    LOG_ERROR("IAV_IOC_QUERY_DECODE_STATUS fail, errno %d.\n", errno);
    return ret;
  }
  LOG_PRINTF("[decode status]: decode_state %d, decoded_pic_number %d, error_status %d, total_error_count %d, irq_count %d\n", status.decode_state, status.decoded_pic_number, status.error_status, status.total_error_count, status.irq_count);
  LOG_PRINTF("[decode status, bsb]: current write offset (arm) 0x%08x, current read offset (dsp) 0x%08x, safe room (minus 256 bytes) %d, free room %d\n", status.write_offset, status.dsp_read_offset, status.room, status.free_room);
  LOG_PRINTF("[decode status, last pts]: %d, is_started %d, is_send_stop_cmd %d\n", status.last_pts, status.is_started, status.is_send_stop_cmd);
  LOG_PRINTF("[decode status, yuv addr]: yuv422_y 0x%08x, yuv422_uv 0x%08x\n", status.yuv422_y_addr, status.yuv422_uv_addr);
  return 0;
}

static int __s5l_decode_query_bsb_status(int iav_fd, SAmbaDSPBSBStatus *status)
{
  int ret;
  struct iav_decode_bsb bsb;
  memset(&bsb, 0x0, sizeof(bsb));
  bsb.decoder_id = status->decoder_id;
  ret = ioctl(iav_fd, IAV_IOC_QUERY_DECODE_BSB, &bsb);
  if (0 > ret) {
    if (EACCES == errno) {
      LOG_NOTICE("stopped\n");
      return DDECODER_STOPPED;
    }
    LOG_ERROR("IAV_IOC_QUERY_DECODE_BSB fail, errno %d.\n", errno);
    perror("IAV_IOC_QUERY_DECODE_BSB");
    return ret;
  }
  status->start_offset = bsb.start_offset;
  status->room = bsb.room;
  status->dsp_read_offset = bsb.dsp_read_offset;
  status->free_room = bsb.free_room;
  return 0;
}

static int __s5l_decode_query_status(int iav_fd, SAmbaDSPDecodeStatus *status)
{
  int ret;
  struct iav_decode_status dec_status;
  memset(&dec_status, 0x0, sizeof(dec_status));
  dec_status.decoder_id = status->decoder_id;
  ret = ioctl(iav_fd, IAV_IOC_QUERY_DECODE_STATUS, &dec_status);
  if (0 > ret) {
    if (EACCES == errno) {
      LOG_NOTICE("stopped\n");
      return DDECODER_STOPPED;
    }
    perror("IAV_IOC_QUERY_DECODE_STATUS");
    LOG_ERROR("IAV_IOC_QUERY_DECODE_STATUS fail, errno %d.\n", errno);
    return ret;
  }
  status->is_started = dec_status.is_started;
  status->is_send_stop_cmd = dec_status.is_send_stop_cmd;
  status->last_pts = dec_status.last_pts;
  status->decode_state = dec_status.decode_state;
  status->error_status = dec_status.error_status;
  status->total_error_count = dec_status.total_error_count;
  status->decoded_pic_number = dec_status.decoded_pic_number;
  status->write_offset = dec_status.write_offset;
  status->room = dec_status.room;
  status->dsp_read_offset = dec_status.dsp_read_offset;
  status->free_room = dec_status.free_room;
  status->irq_count = dec_status.irq_count;
  status->yuv422_y_addr = dec_status.yuv422_y_addr;
  status->yuv422_uv_addr = dec_status.yuv422_uv_addr;
  return 0;
}

static int __s5l_get_vin_info(int iav_fd, SAmbaDSPVinInfo *vininfo)
{
  struct vindev_video_info vin_info;
  struct vindev_fps active_fps;
  TU32 fps_q9 = 1;
  int ret = 0;
  memset(&vin_info, 0x0, sizeof(vin_info));
  vin_info.vsrc_id = 0;
  vin_info.info.mode = AMBA_VIDEO_MODE_CURRENT;
  ret = ioctl(iav_fd, IAV_IOC_VIN_GET_VIDEOINFO, &vin_info);
  if (0 > ret) {
    perror("IAV_IOC_VIN_GET_VIDEOINFO");
    LOG_WARN("IAV_IOC_VIN_GET_VIDEOINFO fail, errno %d\n", errno);
    return ret;
  }
  memset(&active_fps, 0, sizeof(active_fps));
  active_fps.vsrc_id = 0;
  ret = ioctl(iav_fd, IAV_IOC_VIN_GET_FPS, &active_fps);
  if (0 > ret) {
    perror("IAV_IOC_VIN_GET_FPS");
    LOG_WARN("IAV_IOC_VIN_GET_FPS fail, errno %d\n", errno);
    return ret;
  }
  fps_q9 = active_fps.fps;
  __parse_fps(fps_q9, vininfo);
  vininfo->width = vin_info.info.width;
  vininfo->height = vin_info.info.height;
  vininfo->format = vin_info.info.format;
  vininfo->type = vin_info.info.type;
  vininfo->bits = vin_info.info.bits;
  vininfo->ratio = vin_info.info.ratio;
  vininfo->system = vin_info.info.system;
  vininfo->flip = vin_info.info.flip;
  vininfo->rotate = vin_info.info.rotate;
  return 0;
}

static int __s5l_get_stream_framefactor(int iav_fd, int index, SAmbaDSPStreamFramefactor *framefactor)
{
  struct iav_stream_cfg streamcfg;
  int ret = 0;
  if (DMaxStreamNumber <= index) {
    LOG_FATAL("index(%d) not as expected\n", index);
    return (-10);
  }
  memset(&streamcfg, 0, sizeof(streamcfg));
  streamcfg.id = index;
  streamcfg.cid = IAV_STMCFG_FPS;
  ret = ioctl(iav_fd, IAV_IOC_GET_STREAM_CONFIG, &streamcfg);
  if (0 > ret) {
    perror("IAV_IOC_GET_STREAM_CONFIG");
    LOG_ERROR("IAV_IOC_GET_STREAM_CONFIG fail, errno %d\n", errno);
    return ret;
  }
  framefactor->framefactor_num = streamcfg.arg.fps.fps_multi;
  framefactor->framefactor_den = streamcfg.arg.fps.fps_div;
  return 0;
}

static int __s5l_map_bsb(int iav_fd, SAmbaDSPMapBSB *map_bsb)
{
  int ret = 0;
  unsigned int map_size = 0;
  struct iav_querybuf querybuf;
  querybuf.buf = IAV_BUFFER_BSB;
  ret = ioctl(iav_fd, IAV_IOC_QUERY_BUF, &querybuf);
  if (0 > ret) {
    perror("IAV_IOC_QUERY_BUF");
    LOG_ERROR("IAV_IOC_QUERY_BUF fail, errno %d\n", errno);
    return ret;
  }
  map_bsb->size = querybuf.length;
  if (map_bsb->b_two_times) {
    map_size = querybuf.length * 2;
  } else {
    map_size = querybuf.length;
  }
  if (map_bsb->b_enable_read && map_bsb->b_enable_write) {
    map_bsb->base = mmap(NULL, map_size, PROT_WRITE | PROT_READ, MAP_SHARED, iav_fd, querybuf.offset);
  } else if (map_bsb->b_enable_read && !map_bsb->b_enable_write) {
    map_bsb->base = mmap(NULL, map_size, PROT_READ, MAP_SHARED, iav_fd, querybuf.offset);
  } else if (!map_bsb->b_enable_read && map_bsb->b_enable_write) {
    map_bsb->base = mmap(NULL, map_size, PROT_WRITE, MAP_SHARED, iav_fd, querybuf.offset);
  } else {
    LOG_ERROR("not read or write\n");
    return (-1);
  }
  if (map_bsb->base == MAP_FAILED) {
    perror("mmap");
    LOG_ERROR("mmap fail\n");
    return -1;
  }
  LOG_PRINTF("[mmap]: bsb_mem = %p, size = 0x%x\n", map_bsb->base, map_bsb->size);
  return 0;
}

static int __s5l_map_dsp(int iav_fd, SAmbaDSPMapDSP *map_dsp)
{
  TInt ret = 0;
  struct iav_querybuf querybuf;
  querybuf.buf = IAV_BUFFER_DSP;
  ret = ioctl(iav_fd, IAV_IOC_QUERY_BUF, &querybuf);
  if (DUnlikely(0 > ret)) {
    perror("IAV_IOC_QUERY_BUF");
    LOG_ERROR("IAV_IOC_QUERY_BUF fail, errno %d\n", errno);
    return ret;
  }
  map_dsp->size = querybuf.length;
  map_dsp->base = mmap(NULL, map_dsp->size, PROT_READ | PROT_WRITE, MAP_SHARED, iav_fd, querybuf.offset);
  if (DUnlikely(map_dsp->base == MAP_FAILED)) {
    perror("mmap");
    LOG_ERROR("mmap fail, errno %d\n", errno);
    return -1;
  }
  LOG_PRINTF("[mmap]: dsp_mem = %p, size = 0x%x\n", map_dsp->base, map_dsp->size);
  return 0;
}

static int __s5l_map_intrapb(int iav_fd, SAmbaDSPMapIntraPB *map_intrapb)
{
  TInt ret = 0;
  struct iav_querybuf querybuf;
  querybuf.buf = IAV_BUFFER_INTRA_PB;
  ret = ioctl(iav_fd, IAV_IOC_QUERY_BUF, &querybuf);
  if (DUnlikely(0 > ret)) {
    perror("IAV_IOC_QUERY_BUF");
    LOG_ERROR("IAV_IOC_QUERY_BUF fail, errno %d\n", errno);
    return ret;
  }
  map_intrapb->size = querybuf.length;
  map_intrapb->base = mmap(NULL, map_intrapb->size, PROT_READ | PROT_WRITE, MAP_SHARED, iav_fd, querybuf.offset);
  if (DUnlikely(map_intrapb->base == MAP_FAILED)) {
    perror("mmap");
    LOG_ERROR("mmap fail, errno %d\n", errno);
    return -1;
  }
  LOG_PRINTF("[mmap]: intrapb_mem = %p, size = 0x%x\n", map_intrapb->base, map_intrapb->size);
  return 0;
}

static int __s5l_unmap_bsb(int iav_fd, SAmbaDSPMapBSB *map_bsb)
{
  if (map_bsb->base && map_bsb->size) {
    int ret = 0;
    unsigned int map_size = 0;
    if (map_bsb->b_two_times) {
      map_size = map_bsb->size * 2;
    } else {
      map_size = map_bsb->size;
    }
    ret = munmap(map_bsb->base, map_size);
    LOG_PRINTF("[munmap]: bsb_mem = %p, size = 0x%x\n", map_bsb->base, map_bsb->size);
    map_bsb->base = NULL;
    map_bsb->size = 0;
    if (DUnlikely(0 > ret)) {
      perror("munmap");
      LOG_ERROR("munmap fail, errno %d\n", errno);
      return ret;
    }
  } else {
    LOG_ERROR("bad params, %p, %d\n", map_bsb->base, map_bsb->size);
    return (-1);
  }
  return 0;
}

static int __s5l_unmap_dsp(int iav_fd, SAmbaDSPMapDSP *map_dsp)
{
  if (map_dsp->base && map_dsp->size) {
    int ret = 0;
    ret = munmap(map_dsp->base, map_dsp->size);
    LOG_PRINTF("[munmap]: dsp_mem = %p, size = 0x%x\n", map_dsp->base, map_dsp->size);
    map_dsp->base = NULL;
    map_dsp->size = 0;
    if (DUnlikely(0 > ret)) {
      perror("munmap");
      LOG_ERROR("munmap fail, errno %d\n", errno);
      return ret;
    }
  } else {
    LOG_ERROR("bad params, %p, %d\n", map_dsp->base, map_dsp->size);
    return (-1);
  }
  return 0;
}

static int __s5l_unmap_intrapb(int iav_fd, SAmbaDSPMapIntraPB *map_intrapb)
{
  if (map_intrapb->base && map_intrapb->size) {
    int ret = 0;
    ret = munmap(map_intrapb->base, map_intrapb->size);
    LOG_PRINTF("[munmap]: intrapb_mem = %p, size = 0x%x\n", map_intrapb->base, map_intrapb->size);
    map_intrapb->base = NULL;
    map_intrapb->size = 0;
    if (DUnlikely(0 > ret)) {
      perror("munmap");
      LOG_ERROR("munmap fail, errno %d\n", errno);
      return ret;
    }
  } else {
    LOG_ERROR("bad params, %p, %d\n", map_intrapb->base, map_intrapb->size);
    return (-1);
  }
  return 0;
}

static int __s5l_efm_get_bufferpool_info(int iav_fd, SAmbaEFMPoolInfo *buffer_pool_info)
{
  TInt ret = 0;
  struct iav_efm_get_pool_info efm_pool_info;
  memset(&efm_pool_info, 0, sizeof(efm_pool_info));
  ret = ioctl(iav_fd, IAV_IOC_EFM_GET_POOL_INFO, &efm_pool_info);
  if (0 > ret) {
    perror("IAV_IOC_EFM_GET_POOL_INFO");
    LOG_ERROR("IAV_IOC_EFM_GET_POOL_INFO fail, errno %d\n", errno);
    return ret;
  }
  if (buffer_pool_info) {
    buffer_pool_info->yuv_buf_num = efm_pool_info.yuv_buf_num;
    buffer_pool_info->yuv_pitch = efm_pool_info.yuv_pitch;
    buffer_pool_info->me1_buf_num = efm_pool_info.me_buf_num;
    buffer_pool_info->me1_pitch = efm_pool_info.me1_pitch;
    buffer_pool_info->yuv_buf_width = efm_pool_info.yuv_size.width;
    buffer_pool_info->yuv_buf_height = efm_pool_info.yuv_size.height;
    buffer_pool_info->me1_buf_width = efm_pool_info.me1_size.width;
    buffer_pool_info->me1_buf_height = efm_pool_info.me1_size.height;
    buffer_pool_info->b_use_addr = 0;
  } else {
    LOG_FATAL("NULL param\n");
    return (-2);
  }
  return 0;
}

static int __s5l_efm_request_frame(int iav_fd, SAmbaEFMFrame *frame)
{
  TInt ret = 0;
  //TInt max_retry = 10;
  struct iav_efm_request_frame request;
  memset(&request, 0, sizeof(request));
  while (1) {
    ret = ioctl(iav_fd, IAV_IOC_EFM_REQUEST_FRAME, &request);
    if (0 > ret) {
      if (EAGAIN == errno) {
        //max_retry --;
        gfOSmsleep(10);
        continue;
      } else {
        perror("IAV_IOC_EFM_REQUEST_FRAME");
        LOG_ERROR("IAV_IOC_EFM_REQUEST_FRAME fail, errno %d\n", errno);
      }
      return (-1);
    } else {
      break;
    }
  }
  if (frame) {
    frame->frame_idx = request.frame_idx;
    frame->yuv_luma_offset = (TPointer) request.yuv_luma_offset;
    frame->yuv_chroma_offset = (TPointer) request.yuv_chroma_offset;
    frame->me1_offset = (TPointer) request.me1_offset;
  } else {
    LOG_FATAL("NULL param\n");
    return (-2);
  }
  return 0;
}

static int __s5l_efm_finish_frame(int iav_fd, SAmbaEFMFinishFrame *finish_frame)
{
  if (finish_frame) {
    TInt ret = 0;
    if (!finish_frame->b_not_wait_next_interrupt) {
      ret = ioctl(iav_fd, IAV_IOC_WAIT_NEXT_FRAME, 0);
      if (0 > ret) {
        perror("IAV_IOC_WAIT_NEXT_FRAME");
        LOG_ERROR("IAV_IOC_WAIT_NEXT_FRAME fail, errno %d\n", errno);
        return (-1);
      }
    }
    struct iav_efm_handshake_frame handshake;
    memset(&handshake, 0, sizeof(handshake));
    handshake.frame_idx = finish_frame->frame_idx;
    handshake.frame_pts = finish_frame->frame_pts;
    handshake.stream_id = finish_frame->stream_id;
    handshake.is_last_frame = finish_frame->is_last_frame;
    handshake.use_hw_pts = finish_frame->use_hw_pts;
    ret = ioctl(iav_fd, IAV_IOC_EFM_HANDSHAKE_FRAME, &handshake);
    if (0 > ret) {
      perror("IAV_IOC_EFM_REQUEST_FRAME");
      LOG_ERROR("IAV_IOC_EFM_REQUEST_FRAME fail, errno %d\n", errno);
      return (-1);
    }
  } else {
    LOG_FATAL("NULL param\n");
    return (-2);
  }
  return 0;
}

static int __s5l_read_bitstream(int iav_fd, SAmbaDSPReadBitstream *bitstream)
{
  struct iav_querydesc query_desc;
  struct iav_framedesc *frame_desc;
  int ret = 0;
  memset(&query_desc, 0, sizeof(query_desc));
  frame_desc = &query_desc.arg.frame;
  query_desc.qid = IAV_DESC_FRAME;
  //frame_desc->id = bitstream->stream_id;
  frame_desc->id = 0xffffffff;
  frame_desc->time_ms = bitstream->timeout_ms;
  ret = ioctl(iav_fd, IAV_IOC_QUERY_DESC, &query_desc);
  if (ret) {
    if (EAGAIN == errno) {
      return 1;
    }
    perror("IAV_IOC_QUERY_DESC");
    LOG_ERROR("IAV_IOC_QUERY_DESC fail, errno %d\n", errno);
    return (-1);
  }
  bitstream->stream_id = frame_desc->id;
  bitstream->offset = frame_desc->data_addr_offset;
  bitstream->size = frame_desc->size;
  bitstream->pts = frame_desc->arm_pts;
  bitstream->video_width = frame_desc->reso.width;
  bitstream->video_height = frame_desc->reso.height;
  bitstream->slice_id = frame_desc->slice_id;
  bitstream->slice_num = frame_desc->slice_num;
  if (frame_desc->stream_end) {
    DASSERT(!bitstream->size);
    bitstream->size = 0;
    bitstream->offset = 0;
  }
  if (IAV_STREAM_TYPE_H264 == frame_desc->stream_type) {
    bitstream->stream_type = StreamFormat_H264;
  } else if (IAV_STREAM_TYPE_H265 == frame_desc->stream_type) {
    bitstream->stream_type = StreamFormat_H265;
  } else if (IAV_STREAM_TYPE_MJPEG == frame_desc->stream_type) {
    bitstream->stream_type = StreamFormat_JPEG;
  } else {
    bitstream->stream_type = StreamFormat_Invalid;
    LOG_FATAL("bad stream type %d\n", bitstream->stream_type);
    return (-2);
  }
  return 0;
}

static int __s5l_is_ready_for_read_bitstream(int iav_fd)
{
  return 1;
}

static int __s5l_intraplay_reset_buffers(int iav_fd, SAmbaDSPIntraplayResetBuffers *reset_buffers)
{
  TInt ret = 0;
  struct iav_intraplay_reset_buffers reset;
  memset(&reset, 0, sizeof(reset));
  reset.decoder_id = reset_buffers->decoder_id;
  reset.max_num = reset_buffers->max_num;
  ret = ioctl(iav_fd, IAV_IOC_INTRAPLAY_RESET_BUFFERS, &reset);
  if (ret) {
    perror("IAV_IOC_INTRAPLAY_RESET_BUFFERS");
    LOG_ERROR("IAV_IOC_INTRAPLAY_RESET_BUFFERS fail, errno %d\n", errno);
    return ret;
  }
  return 0;
}

static int __s5l_intraplay_request_buffer(int iav_fd, SAmbaDSPIntraplayBuffer *buffer)
{
  TInt ret = 0;
  struct iav_intraplay_frame_buffer buf;
  memset(&buf, 0, sizeof(buf));
  buf.ch_fmt = buffer->ch_fmt;
  buf.buf_width = buffer->buf_width;
  buf.buf_height = buffer->buf_height;
  ret = ioctl(iav_fd, IAV_IOC_INTRAPLAY_REQUEST_BUFFER, &buf);
  if (ret) {
    perror("IOC_INTRAPLAY_REQUEST_BUFFER");
    LOG_ERROR("IOC_INTRAPLAY_REQUEST_BUFFER fail, errno %d\n", errno);
    return ret;
  }
  buffer->buffer_id = buf.buffer_id;
  buffer->ch_fmt = buf.ch_fmt;
  buffer->buf_pitch = buf.buf_pitch;
  buffer->buf_width = buf.buf_width;
  buffer->buf_height = buf.buf_height;
  buffer->lu_buf_offset = buf.lu_buf_offset;
  buffer->ch_buf_offset = buf.ch_buf_offset;
  buffer->img_width = buf.img_width;
  buffer->img_height = buf.img_height;
  buffer->img_offset_x = buf.img_offset_x;
  buffer->img_offset_y = buf.img_offset_y;
  buffer->buffer_size = buf.buffer_size;
  return 0;
}

static int __s5l_intraplay_decode(int iav_fd, SAmbaDSPIntraplayDecode *decode)
{
  TInt ret = 0;
  TU32 i = 0;
  struct iav_intraplay_decode dec;
  memset(&dec, 0, sizeof(dec));
  dec.decoder_id = decode->decoder_id;
  dec.num = decode->num;
  dec.decode_type = decode->decode_type;
  for (i = 0; i < dec.num; i ++) {
    dec.bitstreams[i].bits_fifo_start = decode->bitstreams[i].bits_fifo_start;
    dec.bitstreams[i].bits_fifo_end = decode->bitstreams[i].bits_fifo_end;
    dec.buffers[i].buffer_id = decode->buffers[i].buffer_id;
    dec.buffers[i].ch_fmt = decode->buffers[i].ch_fmt;
    dec.buffers[i].buf_pitch = decode->buffers[i].buf_pitch;
    dec.buffers[i].buf_width = decode->buffers[i].buf_width;
    dec.buffers[i].buf_height = decode->buffers[i].buf_height;
    dec.buffers[i].lu_buf_offset = decode->buffers[i].lu_buf_offset;
    dec.buffers[i].ch_buf_offset = decode->buffers[i].ch_buf_offset;
    dec.buffers[i].img_width = decode->buffers[i].img_width;
    dec.buffers[i].img_height = decode->buffers[i].img_height;
    dec.buffers[i].img_offset_x = decode->buffers[i].img_offset_x;
    dec.buffers[i].img_offset_y = decode->buffers[i].img_offset_y;
    dec.buffers[i].buffer_size = decode->buffers[i].buffer_size;
  }
  ret = ioctl(iav_fd, IAV_IOC_INTRAPLAY_DECODE, &dec);
  if (ret) {
    perror("IAV_IOC_INTRAPLAY_DECODE");
    LOG_ERROR("IAV_IOC_INTRAPLAY_DECODE fail, errno %d\n", errno);
    return ret;
  }
  return 0;
}

static int __s5l_intraplay_yuv2yuv(int iav_fd, SAmbaDSPIntraplayYUV2YUV *yuv2yuv)
{
  TInt ret = 0;
  TU32 i = 0;
  struct iav_intraplay_yuv2yuv yy;
  memset(&yy, 0, sizeof(yy));
  yy.decoder_id = yuv2yuv->decoder_id;
  yy.num = yuv2yuv->num;
  yy.rotate = yuv2yuv->rotate;
  yy.flip = yuv2yuv->flip;
  yy.luma_gain = yuv2yuv->luma_gain;
  yy.src_buf.buffer_id = yuv2yuv->src_buf.buffer_id;
  yy.src_buf.ch_fmt = yuv2yuv->src_buf.ch_fmt;
  yy.src_buf.buf_pitch = yuv2yuv->src_buf.buf_pitch;
  yy.src_buf.buf_width = yuv2yuv->src_buf.buf_width;
  yy.src_buf.buf_height = yuv2yuv->src_buf.buf_height;
  yy.src_buf.lu_buf_offset = yuv2yuv->src_buf.lu_buf_offset;
  yy.src_buf.ch_buf_offset = yuv2yuv->src_buf.ch_buf_offset;
  yy.src_buf.img_width = yuv2yuv->src_buf.img_width;
  yy.src_buf.img_height = yuv2yuv->src_buf.img_height;
  yy.src_buf.img_offset_x = yuv2yuv->src_buf.img_offset_x;
  yy.src_buf.img_offset_y = yuv2yuv->src_buf.img_offset_y;
  yy.src_buf.buffer_size = yuv2yuv->src_buf.buffer_size;
  for (i = 0; i < yy.num; i ++) {
    yy.dst_buf[i].buffer_id = yuv2yuv->dst_buf[i].buffer_id;
    yy.dst_buf[i].ch_fmt = yuv2yuv->dst_buf[i].ch_fmt;
    yy.dst_buf[i].buf_pitch = yuv2yuv->dst_buf[i].buf_pitch;
    yy.dst_buf[i].buf_width = yuv2yuv->dst_buf[i].buf_width;
    yy.dst_buf[i].buf_height = yuv2yuv->dst_buf[i].buf_height;
    yy.dst_buf[i].lu_buf_offset = yuv2yuv->dst_buf[i].lu_buf_offset;
    yy.dst_buf[i].ch_buf_offset = yuv2yuv->dst_buf[i].ch_buf_offset;
    yy.dst_buf[i].img_width = yuv2yuv->dst_buf[i].img_width;
    yy.dst_buf[i].img_height = yuv2yuv->dst_buf[i].img_height;
    yy.dst_buf[i].img_offset_x = yuv2yuv->dst_buf[i].img_offset_x;
    yy.dst_buf[i].img_offset_y = yuv2yuv->dst_buf[i].img_offset_y;
    yy.dst_buf[i].buffer_size = yuv2yuv->dst_buf[i].buffer_size;
  }
  ret = ioctl(iav_fd, IAV_IOC_INTRAPLAY_YUV2YUV, &yy);
  if (ret) {
    perror("IAV_IOC_INTRAPLAY_YUV2YUV");
    LOG_ERROR("IAV_IOC_INTRAPLAY_YUV2YUV fail, errno %d\n", errno);
    return ret;
  }
  return 0;
}

static int __s5l_intraplay_display(int iav_fd, SAmbaDSPIntraplayDisplay *display)
{
  TInt ret = 0;
  TU32 i = 0;
  struct iav_intraplay_display d;
  memset(&d, 0, sizeof(d));
  d.decoder_id = display->decoder_id;
  d.num = display->num;
  for (i = 0; i < d.num; i ++) {
    d.buffers[i].buffer_id = display->buffers[i].buffer_id;
    d.buffers[i].ch_fmt = display->buffers[i].ch_fmt;
    d.buffers[i].buf_pitch = display->buffers[i].buf_pitch;
    d.buffers[i].buf_width = display->buffers[i].buf_width;
    d.buffers[i].buf_height = display->buffers[i].buf_height;
    d.buffers[i].lu_buf_offset = display->buffers[i].lu_buf_offset;
    d.buffers[i].ch_buf_offset = display->buffers[i].ch_buf_offset;
    d.buffers[i].img_width = display->buffers[i].img_width;
    d.buffers[i].img_height = display->buffers[i].img_height;
    d.buffers[i].img_offset_x = display->buffers[i].img_offset_x;
    d.buffers[i].img_offset_y = display->buffers[i].img_offset_y;
    d.buffers[i].buffer_size = display->buffers[i].buffer_size;
    d.desc[i].vout_id = display->desc[i].vout_id;
    d.desc[i].vid_win_update = display->desc[i].vid_win_update;
    d.desc[i].vid_win_rotate = display->desc[i].vid_win_rotate;
    d.desc[i].vid_flip = display->desc[i].vid_flip;
    d.desc[i].vid_win_width = display->desc[i].vid_win_width;
    d.desc[i].vid_win_height = display->desc[i].vid_win_height;
    d.desc[i].vid_win_offset_x = display->desc[i].vid_win_offset_x;
    d.desc[i].vid_win_offset_y = display->desc[i].vid_win_offset_y;
  }
  ret = ioctl(iav_fd, IAV_IOC_INTRAPLAY_DISPLAY, &d);
  if (ret) {
    perror("IAV_IOC_INTRAPLAY_DISPLAY");
    LOG_ERROR("IAV_IOC_INTRAPLAY_DISPLAY fail, errno %d\n", errno);
    return ret;
  }
  return 0;
}

static int __s5l_encode_start(int iav_fd, TU32 mask)
{
  int ret = ioctl(iav_fd, IAV_IOC_START_ENCODE, mask);
  if (ret) {
    perror("IAV_IOC_START_ENCODE");
    LOG_ERROR("IAV_IOC_START_ENCODE fail, errno %d, mask 0x%08x\n", errno, mask);
    return ret;
  }
  return 0;
}

static int __s5l_encode_stop(int iav_fd, TU32 mask)
{
  int ret = ioctl(iav_fd, IAV_IOC_STOP_ENCODE, mask);
  if (ret) {
    perror("IAV_IOC_STOP_ENCODE");
    LOG_ERROR("IAV_IOC_STOP_ENCODE fail, errno %d, mask 0x%08x\n", errno, mask);
    return ret;
  }
  return 0;
}

static int __s5l_query_encode_stream_info(int iav_fd, SAmbaDSPEncStreamInfo *info)
{
  struct iav_stream_info stream_info;
  int ret = 0;
  stream_info.id = info->id;
  ret = ioctl(iav_fd, IAV_IOC_GET_STREAM_INFO, &stream_info);
  if (ret) {
    perror("IAV_IOC_GET_STREAM_INFO");
    LOG_ERROR("IAV_IOC_GET_STREAM_INFO fail, errno %d\n", errno);
    return ret;
  }
  switch (stream_info.state) {
  case IAV_STREAM_STATE_IDLE:
    info->state = EAMDSP_ENC_STREAM_STATE_IDLE;
    break;
  case IAV_STREAM_STATE_STARTING:
    info->state = EAMDSP_ENC_STREAM_STATE_STARTING;
    break;
  case IAV_STREAM_STATE_ENCODING:
    info->state = EAMDSP_ENC_STREAM_STATE_ENCODING;
    break;
  case IAV_STREAM_STATE_STOPPING:
    info->state = EAMDSP_ENC_STREAM_STATE_STOPPING;
    break;
  case IAV_STREAM_STATE_UNKNOWN:
    LOG_ERROR("unknown state\n");
    info->state = EAMDSP_ENC_STREAM_STATE_UNKNOWN;
    return (-1);
    break;
  default:
    LOG_ERROR("unexpected state %d\n", stream_info.state);
    info->state = EAMDSP_ENC_STREAM_STATE_ERROR;
    return (-2);
    break;
  }
  return 0;
}

static int __s5l_query_encode_stream_format(int iav_fd, SAmbaDSPEncStreamFormat *fmt)
{
  struct iav_stream_format format;
  int ret = 0;
  memset(&format, 0, sizeof(format));
  format.id = fmt->id;
  ret = ioctl(iav_fd, IAV_IOC_GET_STREAM_FORMAT, &format);
  if (ret) {
    perror("IAV_IOC_GET_STREAM_FORMAT");
    LOG_ERROR("IAV_IOC_GET_STREAM_FORMAT fail, errno %d\n", errno);
    return ret;
  }
  switch (format.type) {
  case IAV_STREAM_TYPE_H264:
    fmt->codec = EAMDSP_VIDEO_CODEC_TYPE_H264;
    break;
  case IAV_STREAM_TYPE_H265:
    fmt->codec = EAMDSP_VIDEO_CODEC_TYPE_H265;
    break;
  case IAV_STREAM_TYPE_MJPEG:
    fmt->codec = EAMDSP_VIDEO_CODEC_TYPE_MJPEG;
    break;
  case IAV_STREAM_TYPE_NONE:
    LOG_WARN("codec none?\n");
    fmt->codec = EAMDSP_VIDEO_CODEC_TYPE_INVALID;
    break;
  default:
    LOG_ERROR("unexpected codec %d\n", format.type);
    fmt->codec = EAMDSP_VIDEO_CODEC_TYPE_INVALID;
    return (-1);
    break;
  }
  switch (format.buf_id) {
  case IAV_SRCBUF_MN:
    fmt->source_buffer = EAMDSP_ENC_STREAM_SOURCE_BUFFER_MAIN;
    break;
  case IAV_SRCBUF_PC:
    fmt->source_buffer = EAMDSP_ENC_STREAM_SOURCE_BUFFER_SECOND;
    break;
  case IAV_SRCBUF_PB:
    fmt->source_buffer = EAMDSP_ENC_STREAM_SOURCE_BUFFER_THIRD;
    break;
  case IAV_SRCBUF_PA:
    fmt->source_buffer = EAMDSP_ENC_STREAM_SOURCE_BUFFER_FOURTH;
    break;
  case IAV_SRCBUF_PD:
    fmt->source_buffer = EAMDSP_ENC_STREAM_SOURCE_BUFFER_FIFTH;
    break;
  case IAV_SRCBUF_EFM:
    fmt->source_buffer = EAMDSP_ENC_STREAM_SOURCE_BUFFER_EFM;
    break;
  default:
    LOG_ERROR("unexpected source buffer %d\n", format.buf_id);
    fmt->source_buffer = EAMDSP_ENC_STREAM_SOURCE_BUFFER_INVALID;
    return (-2);
    break;
  }
  return 0;
}

static int __s5l_query_source_buffer_info(int iav_fd, SAmbaDSPSourceBufferInfo *info)
{
  struct iav_srcbuf_format buf_format;
  memset(&buf_format, 0, sizeof(buf_format));
  buf_format.buf_id = info->buf_id;
  if (ioctl(iav_fd, IAV_IOC_GET_SOURCE_BUFFER_FORMAT, &buf_format) < 0) {
    perror("IAV_IOC_GET_SOURCE_BUFFER_FORMAT");
    LOG_ERROR("IAV_IOC_QUERY_DESC fail, errno %d\n", errno);
    return (-1);
  }
  info->size_width = buf_format.size.width;
  info->size_height = buf_format.size.height;
  info->crop_size_x = buf_format.input.width;
  info->crop_size_y = buf_format.input.height;
  info->crop_pos_x = buf_format.input.x;
  info->crop_pos_y = buf_format.input.y;
  return 0;
}

static int __s5l_query_yuv_buffer(int iav_fd, SAmbaDSPQueryYUVBuffer *yuv_buffer)
{
  struct iav_yuvbufdesc *yuv_desc;
  struct iav_querydesc query_desc;
  int ret = 0;
  memset(&query_desc, 0, sizeof(query_desc));
  query_desc.qid = IAV_DESC_YUV;
  query_desc.arg.yuv.buf_id = (enum iav_srcbuf_id) yuv_buffer->buf_id;
  ret = ioctl(iav_fd, IAV_IOC_QUERY_DESC, &query_desc);
  if (ret < 0) {
    perror("IAV_IOC_QUERY_DESC");
    LOG_ERROR("IAV_IOC_QUERY_DESC fail, errno %d\n", errno);
    return ret;
  }
  yuv_desc = &query_desc.arg.yuv;
  yuv_buffer->y_addr_offset = yuv_desc->y_addr_offset;
  yuv_buffer->uv_addr_offset = yuv_desc->uv_addr_offset;
  yuv_buffer->width = yuv_desc->width;
  yuv_buffer->height = yuv_desc->height;
  yuv_buffer->pitch = yuv_desc->pitch;
  yuv_buffer->seq_num = yuv_desc->seq_num;
  yuv_buffer->format = yuv_desc->format;
  yuv_buffer->dsp_pts = yuv_desc->dsp_pts;
  yuv_buffer->mono_pts = yuv_desc->mono_pts;
  return 0;
}

static u32 __translate_buffer_type(u32 type)
{
  switch (type) {
  case EAmbaBufferType_DSP:
    return IAV_BUFFER_DSP;
    break;
  case EAmbaBufferType_BSB:
    return IAV_BUFFER_BSB;
    break;
  case EAmbaBufferType_USR:
    return IAV_BUFFER_USR;
    break;
  case EAmbaBufferType_MV:
    return IAV_BUFFER_MV;
    break;
  case EAmbaBufferType_OVERLAY:
    return IAV_BUFFER_OVERLAY;
    break;
  case EAmbaBufferType_QPMATRIX:
    return IAV_BUFFER_QPMATRIX;
    break;
  case EAmbaBufferType_WARP:
    return IAV_BUFFER_WARP;
    break;
  case EAmbaBufferType_QUANT:
    return IAV_BUFFER_QUANT;
    break;
  case EAmbaBufferType_IMG:
    return IAV_BUFFER_IMG;
    break;
  case EAmbaBufferType_PM_IDSP:
    return IAV_BUFFER_PM_IDSP;
    break;
  case EAmbaBufferType_CMD_SYNC:
    return IAV_BUFFER_CMD_SYNC;
    break;
  case EAmbaBufferType_FB_DATA:
    return IAV_BUFFER_FB_DATA;
    break;
  case EAmbaBufferType_FB_AUDIO:
    return IAV_BUFFER_FB_AUDIO;
    break;
  case EAmbaBufferType_QPMATRIX_RAW:
    return IAV_BUFFER_QPMATRIX_RAW;
    break;
  case EAmbaBufferType_INTRA_PB:
    return IAV_BUFFER_INTRA_PB;
    break;
  case EAmbaBufferType_SBP:
    return IAV_BUFFER_SBP;
    break;
  case EAmbaBufferType_MULTI_CHAN:
    return IAV_BUFFER_MULTI_CHAN;
    break;
  default:
    LOG_FATAL("unexpected type %d\n", type);
  }
  return type;
}

static int __s5l_gdma_copy(int iav_fd, SAmbaGDMACopy *copy)
{
  TInt ret = 0;
  struct iav_gdma_copy param;
  param.src_offset = copy->src_offset;
  param.dst_offset = copy->dst_offset;
  param.src_mmap_type = __translate_buffer_type(copy->src_mmap_type);
  param.dst_mmap_type = __translate_buffer_type(copy->dst_mmap_type);
  param.src_pitch = copy->src_pitch;
  param.dst_pitch = copy->dst_pitch;
  param.width = copy->width;
  param.height = copy->height;
  ret = ioctl(iav_fd, IAV_IOC_GDMA_COPY, &param);
  if (ret < 0) {
    perror("IAV_IOC_GDMA_COPY");
    LOG_ERROR("IAV_IOC_GDMA_COPY fail, errno %d\n", errno);
    return ret;
  }
  return 0;
}

static TInt __s5l_dsp_enter_idle_mode(TInt iav_fd)
{
  TInt ret = ioctl(iav_fd, IAV_IOC_ENTER_IDLE);
  if (0 > ret) {
    perror("IAV_IOC_ENTER_IDLE");
    LOG_FATAL("enter idle mode fail, errno %d\n", errno);
  }
  return ret;
}

static void __setup_s5l_al_context(SFAmbaDSPDecAL *al)
{
  al->f_get_dsp_mode = __s5l_get_dsp_mode;
  al->f_enter_mode = __s5l_enter_decode_mode;
  al->f_leave_mode = __s5l_leave_decode_mode;
  al->f_create_decoder = __s5l_create_decoder;
  al->f_destroy_decoder = __s5l_destroy_decoder;
  al->f_query_decode_config = __s5l_query_decode_config;
  al->f_trickplay = __s5l_decode_trick_play;
  al->f_start = __s5l_decode_start;
  al->f_stop = __s5l_decode_stop;
  al->f_speed = __s5l_decode_speed;
  al->f_request_bsb = __s5l_decode_request_bits_fifo;
  al->f_decode = __s5l_decode;
  al->f_query_print_decode_bsb_status = __s5l_decode_query_bsb_status_and_print;
  al->f_query_print_decode_status = __s5l_decode_query_status_and_print;
  al->f_query_decode_bsb_status = __s5l_decode_query_bsb_status;
  al->f_query_decode_status = __s5l_decode_query_status;
  al->f_decode_wait_vout_dormant = NULL;
  al->f_decode_wake_vout = NULL;
  al->f_decode_wait_eos_flag = NULL;
  al->f_decode_wait_eos = NULL;
  al->f_configure_vout = __configure_vout;
  al->f_get_vout_info = __get_single_vout_info;
  al->f_get_vin_info = __s5l_get_vin_info;
  al->f_get_stream_framefactor = __s5l_get_stream_framefactor;
  al->f_map_bsb = __s5l_map_bsb;
  al->f_map_dsp = __s5l_map_dsp;
  al->f_map_intrapb = __s5l_map_intrapb;
  al->f_unmap_bsb = __s5l_unmap_bsb;
  al->f_unmap_dsp = __s5l_unmap_dsp;
  al->f_unmap_intrapb = __s5l_unmap_intrapb;
  al->f_efm_get_buffer_pool_info = __s5l_efm_get_bufferpool_info;
  al->f_efm_request_frame = __s5l_efm_request_frame;
  al->f_efm_finish_frame = __s5l_efm_finish_frame;
  al->f_read_bitstream = __s5l_read_bitstream;
  al->f_is_ready_for_read_bitstream = __s5l_is_ready_for_read_bitstream;
  al->f_intraplay_reset_buffers = __s5l_intraplay_reset_buffers;
  al->f_intraplay_request_buffer = __s5l_intraplay_request_buffer;
  al->f_intraplay_decode = __s5l_intraplay_decode;
  al->f_intraplay_yuv2yuv = __s5l_intraplay_yuv2yuv;
  al->f_intraplay_display = __s5l_intraplay_display;
  al->f_encode_start = __s5l_encode_start;
  al->f_encode_stop = __s5l_encode_stop;
  al->f_query_encode_stream_info = __s5l_query_encode_stream_info;
  al->f_query_encode_stream_fmt = __s5l_query_encode_stream_format;
  al->f_query_source_buffer_info = __s5l_query_source_buffer_info;
  al->f_query_yuv_buffer = __s5l_query_yuv_buffer;
  al->f_gdma_copy = __s5l_gdma_copy;
  al->f_enter_idle_mode = __s5l_dsp_enter_idle_mode;
}

#endif

#endif

int gfOpenIAVHandle()
{
#ifdef BUILD_MODULE_AMBA_DSP
  int fd = open("/dev/iav", O_RDWR, 0);
  if (0 > fd) {
    LOG_FATAL("open iav fail, %d.\n", fd);
  }
  return fd;
#endif
  LOG_FATAL("dsp related is not compiled\n");
  return (-4);
}

void gfCloseIAVHandle(int fd)
{
#ifdef BUILD_MODULE_AMBA_DSP
  if (0 > fd) {
    LOG_FATAL("bad fd %d\n", fd);
    return;
  }
  close(fd);
  return;
#endif
  LOG_FATAL("dsp related is not compiled\n");
  return;
}

void gfSetupDSPAlContext(SFAmbaDSPDecAL *al)
{
#ifdef BUILD_MODULE_AMBA_DSP
#ifdef BUILD_DSP_AMBA_S2L
  __setup_s2l_al_context(al);
#elif defined (BUILD_DSP_AMBA_S2) || defined (BUILD_DSP_AMBA_S2E)
  __setup_s2_s2e_al_context(al);
#elif defined (BUILD_DSP_AMBA_S3L)
  __setup_s3l_al_context(al);
#elif defined (BUILD_DSP_AMBA_S5)
  __setup_s5_al_context(al);
#elif defined (BUILD_DSP_AMBA_S5L)
  __setup_s5l_al_context(al);
#else
  LOG_FATAL("add support here\n");
#endif
  return;
#endif
  LOG_FATAL("dsp related is not compiled\n");
  return;
}

const TChar *gfGetDSPPlatformName()
{
#ifdef BUILD_MODULE_AMBA_DSP
#ifdef BUILD_DSP_AMBA_S2L
  return "S2L";
#elif defined (BUILD_DSP_AMBA_S2) || defined (BUILD_DSP_AMBA_S2E)
  return "S2E";
#elif defined (BUILD_DSP_AMBA_S3L)
  return "S3L";
#elif defined (BUILD_DSP_AMBA_S5)
  return "S5";
#elif defined (BUILD_DSP_AMBA_S5L)
  return "S5L";
#else
  return "Unknown";
#endif
#endif
  return "NoDSP";
}

EECode gfAmbaDSPConfigureVout(SVideoOutputConfigure *config)
{
#ifdef BUILD_MODULE_AMBA_DSP
  SFAmbaDSPDecAL al;
  int ret = 0;
  int fd = -1;
  if (!config) {
    LOG_FATAL("NULL config.\n");
    return EECode_OSError;
  }
  gfSetupDSPAlContext(&al);
  if (!al.f_configure_vout) {
    LOG_FATAL("configure vout function is NULL.\n");
    return EECode_OSError;
  }
  fd = gfOpenIAVHandle();
  if (fd >= 0) {
    ret = al.f_configure_vout(fd, config);
  } else {
    LOG_FATAL("gfOpenIAVHandle fail.\n");
    return EECode_OSError;
  }
  gfCloseIAVHandle(fd);
  fd = -1;
  if (!ret) {
    return EECode_OK;
  }
  LOG_ERROR("configure vout fail\n");
  return EECode_OSError;
#endif
  LOG_FATAL("dsp related is not compiled\n");
  return EECode_NotSupported;
}

EECode gfAmbaDSPHaltCurrentVout()
{
#ifdef BUILD_MODULE_AMBA_DSP
  int fd = 0;
  int i = 0;
  fd = gfOpenIAVHandle();
  LOG_PRINTF("halt current vout, begin\n");
  if (fd >= 0) {
    for (i = 0; i < 2; i ++) {
      if (__is_vout_alive(fd, i)) {
        LOG_PRINTF("halt %d\n", i);
        __halt_vout(fd, i);
      }
    }
    LOG_PRINTF("halt current vout, end\n");
    gfCloseIAVHandle(fd);
    return EECode_OK;
  }
  LOG_FATAL("gfOpenIAVHandle fail.\n");
  return EECode_OSError;
#endif
  LOG_FATAL("dsp related is not compiled\n");
  return EECode_NotSupported;
}

EECode gfAmbaDSPGetMode(TU32 *mode)
{
#ifdef BUILD_MODULE_AMBA_DSP
  SFAmbaDSPDecAL al;
  SAmbaDSPMode dsp_mode;
  int ret = 0;
  int fd = -1;
  gfSetupDSPAlContext(&al);
  if (!al.f_get_dsp_mode) {
    LOG_FATAL("f_get_dsp_mode function is NULL.\n");
    return EECode_OSError;
  }
  fd = gfOpenIAVHandle();
  if (fd >= 0) {
    ret = al.f_get_dsp_mode(fd, &dsp_mode);
  } else {
    LOG_FATAL("gfOpenIAVHandle fail.\n");
    return EECode_OSError;
  }
  gfCloseIAVHandle(fd);
  fd = -1;
  if (!ret) {
    *mode = dsp_mode.dsp_mode;
    return EECode_OK;
  }
  LOG_ERROR("get dsp mode fail\n");
  return EECode_OSError;
#endif
  LOG_FATAL("dsp related is not compiled\n");
  return EECode_NotSupported;
}

EECode gfAmbaDSPEnterIdle()
{
#ifdef BUILD_MODULE_AMBA_DSP
  SFAmbaDSPDecAL al;
  int ret = 0;
  int fd = -1;
  gfSetupDSPAlContext(&al);
  if (!al.f_enter_idle_mode) {
    LOG_FATAL("f_enter_idle_mode function is NULL.\n");
    return EECode_OSError;
  }
  fd = gfOpenIAVHandle();
  if (fd >= 0) {
    ret = al.f_enter_idle_mode(fd);
    gfCloseIAVHandle(fd);
    if (!ret) {
      return EECode_OK;
    }
  } else {
    LOG_FATAL("gfOpenIAVHandle fail.\n");
    return EECode_OSError;
  }
  LOG_ERROR("get dsp mode fail\n");
  return EECode_OSError;
#endif
  LOG_FATAL("dsp related is not compiled\n");
  return EECode_NotSupported;
}

void gfFillAmbaH264GopHeader(TU8 *p_gop_header, TU32 frame_tick, TU32 time_scale, TU32 pts, TU8 gopsize, TU8 m)
{
  TU32 tick_high = frame_tick;
  TU32 tick_low = tick_high & 0x0000ffff;
  TU32 scale_high = time_scale;
  TU32 scale_low = scale_high & 0x0000ffff;
  TU32 pts_high = 0;
  TU32 pts_low = 0;
  tick_high >>= 16;
  scale_high >>= 16;
  p_gop_header[0] = 0; // start code prefix
  p_gop_header[1] = 0;
  p_gop_header[2] = 0;
  p_gop_header[3] = 1;
  p_gop_header[4] = 0x7a; // nal type = 0x1a
  p_gop_header[5] = 0x01; // version main
  p_gop_header[6] = 0x01; // version sub
  p_gop_header[7] = tick_high >> 10;
  p_gop_header[8] = tick_high >> 2;
  p_gop_header[9] = (tick_high << 6) | (1 << 5) | (tick_low >> 11);
  p_gop_header[10] = tick_low >> 3;
  p_gop_header[11] = (tick_low << 5) | (1 << 4) | (scale_high >> 12);
  p_gop_header[12] = scale_high >> 4;
  p_gop_header[13] = (scale_high << 4) | (1 << 3) | (scale_low >> 13);
  p_gop_header[14] = scale_low >> 5;
  p_gop_header[15] = (scale_low << 3) | (1 << 2) | (pts_high >> 14);
  p_gop_header[16] = pts_high >> 6;
  p_gop_header[17] = (pts_high << 2) | (1 << 1) | (pts_low >> 15);
  p_gop_header[18] = pts_low >> 7;
  p_gop_header[19] = (pts_low << 1) | 1;
  p_gop_header[20] = gopsize;
  p_gop_header[21] = (m & 0xf) << 4;
}

void gfUpdateAmbaH264GopHeader(TU8 *p_gop_header, TU32 pts, TU8 gopsize)
{
  TU32 pts_high = (pts >> 16) & 0x0000ffff;
  TU32 pts_low = pts & 0x0000ffff;
  p_gop_header[15] = (p_gop_header[15]  & 0xFC) | (pts_high >> 14);
  p_gop_header[16] = pts_high >> 6;
  p_gop_header[17] = (pts_high << 2) | (1 << 1) | (pts_low >> 15);
  p_gop_header[18] = pts_low >> 7;
  p_gop_header[19] = (pts_low << 1) | 1;
  p_gop_header[20] = gopsize;
}

void gfFillAmbaH265GopHeader(TU8 *p_gop_header, TU32 frame_tick, TU32 time_scale, TU32 pts, TU8 gopsize, TU8 m)
{
  TU32 tick_high = frame_tick;
  TU32 tick_low = tick_high & 0x0000ffff;
  TU32 scale_high = time_scale;
  TU32 scale_low = scale_high & 0x0000ffff;
  TU32 pts_high = 0;
  TU32 pts_low = 0;
  tick_high >>= 16;
  scale_high >>= 16;
  p_gop_header[0] = 0; // start code prefix
  p_gop_header[1] = 0;
  p_gop_header[2] = 0;
  p_gop_header[3] = 1;
  p_gop_header[4] = 0x34; // nal type = 0x1a
  p_gop_header[5] = 0x01;
  p_gop_header[6] = 0x01; // version main
  p_gop_header[7] = 0x01; // version sub
  p_gop_header[8] = tick_high >> 10;
  p_gop_header[9] = tick_high >> 2;
  p_gop_header[10] = (tick_high << 6) | (1 << 5) | (tick_low >> 11);
  p_gop_header[11] = tick_low >> 3;
  p_gop_header[12] = (tick_low << 5) | (1 << 4) | (scale_high >> 12);
  p_gop_header[13] = scale_high >> 4;
  p_gop_header[14] = (scale_high << 4) | (1 << 3) | (scale_low >> 13);
  p_gop_header[15] = scale_low >> 5;
  p_gop_header[16] = (scale_low << 3) | (1 << 2) | (pts_high >> 14);
  p_gop_header[17] = pts_high >> 6;
  p_gop_header[18] = (pts_high << 2) | (1 << 1) | (pts_low >> 15);
  p_gop_header[19] = pts_low >> 7;
  p_gop_header[20] = (pts_low << 1) | 1;
  p_gop_header[21] = gopsize;
  p_gop_header[22] = (m & 0xf) << 4;
}

void gfUpdateAmbaH265GopHeader(TU8 *p_gop_header, TU32 pts, TU8 gopsize)
{
  TU32 pts_high = (pts >> 16) & 0x0000ffff;
  TU32 pts_low = pts & 0x0000ffff;
  p_gop_header[16] = (p_gop_header[16]  & 0xFC) | (pts_high >> 14);
  p_gop_header[17] = pts_high >> 6;
  p_gop_header[18] = (pts_high << 2) | (1 << 1) | (pts_low >> 15);
  p_gop_header[19] = pts_low >> 7;
  p_gop_header[20] = (pts_low << 1) | 1;
  p_gop_header[21] = gopsize;
}

unsigned int gfGetMaxEncodingStreamNumber()
{
#ifdef BUILD_MODULE_AMBA_DSP
  return IAV_STREAM_MAX_NUM_ALL;
#endif
  LOG_FATAL("dsp related is not compiled\n");
  return 0;
}

