/*******************************************************************************
 * am_mp_lens_shad_cal.cpp
 *
 * History:
 *   May 12, 2015 - [longli] created file
 *
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
 */


#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "am_mp_server.h"
#include "am_mp_lens_shad_cal.h"
#include "img_adv_struct_arch.h"
#include "img_api_adv_arch.h"
#include "mw_struct.h"
#include "mw_api.h"

#define CALI_SHAD_FILE "/ambarella/calibration/cali_lens_shad.bin"
#define TEST_ENCODE_PATH "/usr/local/bin/test_encode"
#define RTSP_SERVER_PATH "/usr/local/bin/rtsp_server"
#define CALI_LENS_SHADING_PATH "/usr/local/bin/cali_lens_shading"
/*
 * Client of mp tool should send a string like the
 * following macro.
 *
 * The 1th integer: sensor type
 * The 2th integer: vin mode
 * The 3th integer: anti flicker mode
 * The 4th integer: AE target value
 */
#define MP_LEN_SHAD_CAL_FMT "%d:%d:%d:%d"

typedef enum mp_lens_shad_cal_step
{
   MP_LENS_SHAD_CAL_HAND_SHAKE = 0,
   MP_LENS_SHAD_CAL_INIT_PARAM,
   MP_LENS_SHAD_CAL_INIT_VDEV,
   MP_LENS_SHAD_CAL_CHECK_LIGHT,
   MP_LENS_SHAD_CAL_LIGHT_READY,
   MP_LENS_SHAD_CAL_DETECT,
   MP_LENS_SHAD_CAL_VERIFY,
   MP_LENS_SHAD_CAL_ENSURE,
} mp_lens_shad_cal_step_t;

typedef struct
{
    int32_t ae_target_value;
    int32_t flick_mode;
    int32_t width;
    int32_t height;
    int32_t bwidth;
    int32_t bheight;
}AMMPVdevSettings;

extern int32_t fd_iav;
static AMMPVdevSettings v_set;

static int32_t init_test_parameters(const char *msg, AMMPVdevSettings *v_set)
{
  int32_t ret = 0;
  int32_t sensor_type, vin_mode;

  do {
    if (!msg) {
      ret = -1;
      break;
    }

    if (sscanf(msg,
             MP_LEN_SHAD_CAL_FMT,
             &sensor_type,
             &vin_mode,
             &v_set->flick_mode,
             &v_set->ae_target_value) != 4) {
       printf("Format of testing parameters is wrong!\n ");
       ret = -1;
       break;
    }

    if (v_set->ae_target_value < 1 || v_set->ae_target_value > 4) {
      printf("Ae target value is wrong!\n");
      ret = -1;
      break;
    } else {
      v_set->ae_target_value <<= 10; /* ae_target_value *= 1024 */
    }

    if (v_set->flick_mode == 1) {
      v_set->flick_mode = 50;
    } else if (v_set->flick_mode == 2) {
      v_set->flick_mode = 60;
    } else {
      printf("Wrong flick mode, should be 1 or 2!\n");
      ret = -1;
      break;
    }

    switch (vin_mode) {
      case 1:
        v_set->width = 1280;
        v_set->height = 720;
        v_set->bwidth = 1280;
        v_set->bheight = 720;
        break;
      case 2:
        v_set->width = 1536;
        v_set->height = 1024;
        v_set->bwidth = 1536;
        v_set->bheight = 1024;
        break;
      case 3:
        v_set->width = 1920;
        v_set->height = 1080;
        v_set->bwidth = 1920;
        v_set->bheight = 1080;
        break;
      case 4:
        v_set->width = 2048;
        v_set->height = 1536;
        v_set->bwidth = 1920;
        v_set->bheight = 1080;
        break;
      case 5:
        v_set->width = 2304;
        v_set->height = 1296;
        v_set->bwidth = 2304;
        v_set->bheight = 1296;
        break;
      case 6:
        v_set->width = 2304;
        v_set->height = 1536;
        v_set->bwidth = 2304;
        v_set->bheight = 1536;
        break;
      case 7:
        v_set->width = 2560;
        v_set->height = 2048;
        v_set->bwidth = 1920;
        v_set->bheight = 1080;
        break;
      case 8:
        v_set->width = 2592;
        v_set->height = 1944;
        v_set->bwidth = 1920;
        v_set->bheight = 1080;
        break;
      case 9:
        v_set->width = 2688;
        v_set->height = 1512;
        v_set->bwidth = 1920;
        v_set->bheight = 1080;
        break;
      case 10:
        v_set->width = 3072;
        v_set->height = 2048;
        v_set->bwidth = 3072;
        v_set->bheight = 2048;
        break;
      case 11:
        v_set->width = 3840;
        v_set->height = 2160;
        v_set->bwidth = 3840;
        v_set->bheight = 2160;
        break;
      case 12:
        v_set->width = 4000;
        v_set->height = 3000;
        v_set->bwidth = 3840;
        v_set->bheight = 2160;
        break;
      case 13:
        v_set->width = 4096;
        v_set->height = 2160;
        v_set->bwidth = 3840;
        v_set->bheight = 2160;
        break;
      default:
        printf("vin mode unrecognized!\n");
        ret = -1;
        break;
    }

  } while (0);

  return ret;
}


am_mp_err_t mptool_lens_shad_cal_handler(am_mp_msg_t *from, am_mp_msg_t *to)
{
  int32_t status;
  char cmd[BUF_LEN * 2] = {0};

  switch (from->stage) {
    case MP_LENS_SHAD_CAL_HAND_SHAKE:
      break;

    case MP_LENS_SHAD_CAL_INIT_PARAM:
      if (access(CALI_SHAD_FILE, F_OK) == 0) {
        /* Old file exists and needs to be deleted. */
        snprintf(cmd, sizeof(cmd), "rm %s", CALI_SHAD_FILE);
        status = system(cmd);
        if (WEXITSTATUS(status)) {
          printf("Failed to delete %s: %s",
                 CALI_SHAD_FILE,
                 strerror(errno));
          to->result.ret = MP_ERROR;
          break;
        }
      }

      if (access(CALI_LENS_SHADING_PATH, F_OK|X_OK) != 0) {
        printf("access %s error.\n", CALI_LENS_SHADING_PATH);
        to->result.ret = MP_ERROR;
        break;
      }

      status = system("mount -o remount,rw /");
      if (WEXITSTATUS(status)) {
        printf("Failed to mount: %s", strerror (errno));
        to->result.ret = MP_ERROR;
        break;
      }

      status = system("mkdir -p /ambarella/calibration");
      if (WEXITSTATUS(status)) {
        printf("Failed to mkdir: %s", strerror (errno));
        to->result.ret = MP_ERROR;
        break;
      }

      if (init_test_parameters(from->msg, &v_set) < 0) {
        to->result.ret = MP_ERROR;
        break;
      }
      break;

    case MP_LENS_SHAD_CAL_INIT_VDEV:
      snprintf(cmd, sizeof(cmd) - 1,
               "%s -i %dx%d --raw-capture 1"
               " -X --bsize %dx%d --bmaxsize %dx%d -J --btype off",
               TEST_ENCODE_PATH, v_set.width, v_set.height,
               v_set.bwidth, v_set.bheight, v_set.bwidth, v_set.bheight);

      status = system(cmd);
      if (WEXITSTATUS(status)) {
        printf("Failed to enter preview!\n");
        to->result.ret = MP_ERROR;
        break;
      }

      if (mw_start_aaa(fd_iav) < 0) {
        perror("mw_start_aaa");
        to->result.ret = MP_ERROR;
        break;
      }

      sleep(2);

      snprintf(cmd, sizeof(cmd),
               "%s &", RTSP_SERVER_PATH);
      printf("\n%s\n", cmd);
      status = system(cmd);
      if (WEXITSTATUS(status)) {
        strcpy (to->msg, "init vdev: rtsp_server failed!\n");
        to->result.ret = MP_ERROR;
        break;
      }

      snprintf(cmd, sizeof(cmd),
               "%s -A -h %dx%d -e",
               TEST_ENCODE_PATH,
               v_set.bwidth, v_set.bheight);
      status = system(cmd);
      if (WEXITSTATUS(status)) {
        strcpy(to->msg, "init vdev: test_encode failed!\n");
        to->result.ret = MP_ERROR;
        break;
      }
      break;
    case MP_LENS_SHAD_CAL_CHECK_LIGHT:
    {
      int32_t value[1] = {0};
      uint32_t sensor_gain = 0;

      if (mw_get_sensor_gain(fd_iav, value) < 0) {
        printf("get sensor gain error!");
        to->result.ret = MP_ERROR;
        break;
      }
      sensor_gain = (value[0] >> 24);

      memcpy(to->msg, &sensor_gain, sizeof(uint32_t));
    }
      break;
    case MP_LENS_SHAD_CAL_LIGHT_READY:
      if (mw_stop_aaa() < 0) {
        perror("mw_stop_aaa");
        to->result.ret = MP_ERROR;
        break;
      }
      break;
    case MP_LENS_SHAD_CAL_DETECT:
      snprintf(cmd, sizeof(cmd) - 1,
               "%s -d -a %d -t %d -f %s",
               CALI_LENS_SHADING_PATH,
               v_set.flick_mode, v_set.ae_target_value,
               CALI_SHAD_FILE);
      status = system(cmd);
      if (WEXITSTATUS(status)) {
        to->result.ret = MP_ERROR;
        break;
      }
      break;
    case MP_LENS_SHAD_CAL_VERIFY:
      snprintf(cmd, sizeof(cmd), "%s -r", CALI_LENS_SHADING_PATH);
      printf("\n%s\n", cmd);
      status = system(cmd);
      if (WEXITSTATUS(status)) {
        to->result.ret = MP_ERROR;
        break;
      }

      snprintf(cmd, sizeof(cmd),
               "%s -c -f %s", CALI_LENS_SHADING_PATH, CALI_SHAD_FILE);
      status = system(cmd);
      if (WEXITSTATUS(status)) {
        to->result.ret = MP_ERROR;
        break;
      }
      break;
    case MP_LENS_SHAD_CAL_ENSURE:
      snprintf(cmd, sizeof(cmd),
               "%s -A -s", TEST_ENCODE_PATH);

      status = system(cmd);
      if (WEXITSTATUS(status)) {
        strcpy(to->msg, "result verify: test_encode -A -s failed!\n");
        to->result.ret = MP_ERROR;
        break;
      }

      system("kill -9 `pidof rtsp_server`");

      if (mw_stop_aaa() < 0) {
        perror("mw_stop_aaa");
        to->result.ret = MP_ERROR;
        break;
      }

      if (mptool_save_data(from->result.type,
                           from->result.ret,
                           SYNC_FILE) != MP_OK) {
        printf("Failed to save data!\n");
        to->result.ret = MP_ERROR;
      }
      break;
    default:
      to->result.ret = MP_NOT_SUPPORT;
      break;
  }

  return MP_OK;
}
