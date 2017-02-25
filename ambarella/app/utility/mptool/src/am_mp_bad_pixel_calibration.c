/*******************************************************************************
 * am_mp_bad_pixel_calibration.cpp
 *
 * History:
 *   Mar 18, 2015 - [longli] created file
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
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "am_mp_server.h"
#include "am_mp_bad_pixel_calibration.h"

#define TEST_IMAGE_PATH "/usr/local/bin/test_image"
#define TEST_ENCODE_PATH "/usr/local/bin/test_encode"
#define RTSP_SERVER_PATH "/usr/local/bin/rtsp_server"
#define CALI_BAD_PIXEL_PATH "/usr/local/bin/cali_bad_pixel"
#define BITMAP_MERGER_PATH "/usr/local/bin/bitmap_merger"
#define BRIGHT_BAD_PIXEL_FILE "/ambarella/calibration/cali_bright_bad_pixel.bin"
#define DARK_BAD_PIXEL_FILE "/ambarella/calibration/cali_dark_bad_pixel.bin"
#define CALI_BAD_PIXEL_FILE "/ambarella/calibration/cali_bad_pixel.bin"

enum {
  SYSTEM_INIT = 0,
  BPC_START_ENCODING,
  BRIGHT_BAD_PIXEL_DETECTION,
  RESTORE_BAD_PIXEL_EFFECT,
  CORRECT_BRIGHT_BAD_PIXEL,
  BPC_RESTART_ENCODING,
  DARK_BAD_PIXEL_DETECTION,
  CORRECT_DARK_BAD_PIXEL,
  FINISH_CALIBRATION,
  BAD_PIXEL_RESULT_SAVE
};

typedef enum
{
  OV4689_4M_sensor = 0,
  MT9T002_3M_sensor,
  IMX104_1M_sensor,
  IMX178_6M_sensor,
  IMX224_sensor,
  MN43220_sensor,
  AR0230_sensor
}sensor_type;

typedef enum
{
  VIN_1280x720 = 0,
  VIN_1536x1024,
  VIN_1920x1080,
  VIN_2048x1536,
  VIN_2304x1296,
  VIN_2304x1536,
  VIN_2560x2048,
  VIN_2592x1944,
  VIN_2688x1512,
  VIN_3072x2048,
  VIN_3840x2160,
  VIN_4000x3000,
  VIN_4096x2160
}vin_mode;

typedef struct
{
    char sensor_name[32];
    int32_t high_thresh;
    int32_t low_thresh;
    int32_t agc_index;
    int32_t width;
    int32_t height;
    int32_t bwidth;
    int32_t bheight;
}AMMPSensorParameter;

typedef struct{
    sensor_type  type;
    vin_mode     mode;
}mp_sensor_msg;

static AMMPSensorParameter sensor_para;

static int32_t check_sys_resources()
{
  int32_t ret = 0;

  do {
    if (access(TEST_IMAGE_PATH, F_OK|X_OK) != 0) {
      printf("access %s error\n", TEST_IMAGE_PATH);
      ret = -1;
      break;
    }

    if (access(TEST_ENCODE_PATH, F_OK|X_OK) != 0) {
      printf("access %s error\n", TEST_ENCODE_PATH);
      ret = -1;
      break;
    }

    if (access(RTSP_SERVER_PATH, F_OK|X_OK) != 0) {
      printf("access %s error\n", RTSP_SERVER_PATH);
      ret = -1;
      break;
    }

    if (access(CALI_BAD_PIXEL_PATH, F_OK|X_OK) != 0) {
      printf("access %s error\n", CALI_BAD_PIXEL_PATH);
      ret = -1;
      break;
    }

    if (access(BITMAP_MERGER_PATH, F_OK|X_OK) != 0) {
      printf("access %s error\n", BITMAP_MERGER_PATH);
      ret = -1;
      break;
    }

    if (access("/ambarella/calibration", F_OK)) {
      int32_t status;
      char command[BUF_LEN] = {0};
      snprintf(command, sizeof(command),
               "mkdir -p /ambarella/calibration");
      status = system(command);
      if (WEXITSTATUS(status)) {
        printf("Make dir(/ambarella/calibration) failed!\n");
        ret = -1;
        break;
      }
    }
  } while (0);

  return ret;
}

static int32_t init_sensor_parameter(const mp_sensor_msg *msg,
                                     AMMPSensorParameter *sensor_para)
{
  int32_t ret = 0;

  do {
    if (!msg || !sensor_para) {
      ret = -1;
      break;
    }

    switch (msg->type) {
      case OV4689_4M_sensor:
        sensor_para->high_thresh = 200;
        sensor_para->low_thresh = 200;
        sensor_para->agc_index = 256;
        break;
      case MT9T002_3M_sensor:
        sensor_para->high_thresh = 200;
        sensor_para->low_thresh = 200;
        sensor_para->agc_index = 640;
        break;
      case IMX104_1M_sensor:
        sensor_para->high_thresh = 200;
        sensor_para->low_thresh = 200;
        sensor_para->agc_index = 768;
        break;
      case IMX178_6M_sensor:
        sensor_para->high_thresh = 200;
        sensor_para->low_thresh = 200;
        sensor_para->agc_index = 768;
        break;
      case IMX224_sensor:
        sensor_para->high_thresh = 200;
        sensor_para->low_thresh = 200;
        sensor_para->agc_index = 256;
        break;
      case MN43220_sensor:
        sensor_para->high_thresh = 200;
        sensor_para->low_thresh = 200;
        sensor_para->agc_index = 100;
        break;
      case AR0230_sensor:
        sensor_para->high_thresh = 200;
        sensor_para->low_thresh = 200;
        sensor_para->agc_index = 256;
        break;
      default:
        printf("sensor type unrecognized!\n");
        ret = -1;
        break;
    }

    if (ret) {
      break;
    }

    switch (msg->mode) {
      case VIN_1280x720:
        sensor_para->width = 1280;
        sensor_para->height = 720;
        sensor_para->bwidth = 1280;
        sensor_para->bheight = 720;
        break;
      case VIN_1536x1024:
        sensor_para->width = 1536;
        sensor_para->height = 1024;
        sensor_para->bwidth = 1536;
        sensor_para->bheight = 1024;
        break;
      case VIN_1920x1080:
        sensor_para->width = 1920;
        sensor_para->height = 1080;
        sensor_para->bwidth = 1920;
        sensor_para->bheight = 1080;
        break;
      case VIN_2048x1536:
        sensor_para->width = 2048;
        sensor_para->height = 1536;
        sensor_para->bwidth = 1920;
        sensor_para->bheight = 1080;
        break;
      case VIN_2304x1296:
        sensor_para->width = 2304;
        sensor_para->height = 1296;
        sensor_para->bwidth = 2304;
        sensor_para->bheight = 1296;
        break;
      case VIN_2304x1536:
        sensor_para->width = 2304;
        sensor_para->height = 1536;
        sensor_para->bwidth = 2304;
        sensor_para->bheight = 1536;
        break;
      case VIN_2560x2048:
        sensor_para->width = 2560;
        sensor_para->height = 2048;
        sensor_para->bwidth = 1920;
        sensor_para->bheight = 1080;
        break;
      case VIN_2592x1944:
        sensor_para->width = 2592;
        sensor_para->height = 1944;
        sensor_para->bwidth = 1920;
        sensor_para->bheight = 1080;
        break;
      case VIN_2688x1512:
        sensor_para->width = 2688;
        sensor_para->height = 1512;
        sensor_para->bwidth = 1920;
        sensor_para->bheight = 1080;
        break;
      case VIN_3072x2048:
        sensor_para->width = 3072;
        sensor_para->height = 2048;
        sensor_para->bwidth = 3072;
        sensor_para->bheight = 2048;
        break;
      case VIN_3840x2160:
        sensor_para->width = 3840;
        sensor_para->height = 2160;
        sensor_para->bwidth = 3840;
        sensor_para->bheight = 2160;
        break;
      case VIN_4000x3000:
        sensor_para->width = 4000;
        sensor_para->height = 3000;
        sensor_para->bwidth = 3840;
        sensor_para->bheight = 2160;
        break;
      case VIN_4096x2160:
        sensor_para->width = 4096;
        sensor_para->height = 2160;
        sensor_para->bwidth = 3840;
        sensor_para->bheight = 2160;
        break;
      default:
        printf("vin mode unrecognized!\n");
        ret = -1;
        break;
    }
  } while (0);

  printf("\nsensor type=%d  width=%d,height=%d\n",
         msg->type, sensor_para->width, sensor_para->height);
  return ret;
}

int32_t bpc_enter_preview(const AMMPSensorParameter *sensor_para)
{
  int32_t ret = 0;
  int32_t status;
  char command[BUF_LEN * 2];

  do {
    if (!sensor_para) {
      ret = -1;
      break;
    }
    snprintf(command, sizeof(command), "%s -i0 &", TEST_IMAGE_PATH);
    status = system(command);
    if (WEXITSTATUS(status)) {
      printf("Failed to preview when execute %s!\n", TEST_IMAGE_PATH);
      ret = -1;
      break;
    }
    sleep(1);
    snprintf(command, sizeof(command),
             "%s -i%dx%d --raw-capture 1 --enc-mode 0 "
             "-X --bsize %dx%d --bmaxsize %dx%d -J --btype off",
             TEST_ENCODE_PATH, sensor_para->width, sensor_para->height,
             sensor_para->bwidth, sensor_para->bheight,
             sensor_para->bwidth, sensor_para->bheight);
    status = system(command);
    if (WEXITSTATUS(status)) {
      printf("Failed to preview when execute %s!\n", TEST_ENCODE_PATH);
      ret = -1;
      break;
    }
    sleep(1);
    snprintf(command, sizeof(command), "killall -9 test_image");
    status = system(command);
    if (WEXITSTATUS(status)) {
      printf("Failed to kill test_image!\n");
      ret = -1;
      break;
    }
  } while (0);

  return ret;
}

int32_t bpc_start_encode(const AMMPSensorParameter *sensor_para)
{
  int32_t ret = 0;
  int32_t status;
  char command[BUF_LEN] = {0};

  snprintf(command, sizeof(command),
           "%s -A -h %dx%d --smaxsize %dx%d -e",
           TEST_ENCODE_PATH, sensor_para->bwidth, sensor_para->bheight,
           sensor_para->bwidth, sensor_para->bheight);

  status = system(command);
  if (WEXITSTATUS(status)) {
    printf("Failed to start encode!\n");
    ret = -1;
  }

  return ret;
}

static int32_t restore_bad_pixel_effect(const AMMPSensorParameter *sensor_para)
{
  int32_t ret = 0;
  int32_t status;
  char command[BUF_LEN] = {0};

  do {
    if (!sensor_para || bpc_enter_preview(sensor_para)) {
      ret = -1;
      break;
    }

    snprintf(command, sizeof(command), "%s -A %d",
             CALI_BAD_PIXEL_PATH, sensor_para->agc_index);
    status = system(command);
    if (WEXITSTATUS(status)) {
      printf("Restore_bad_pixel_effect: set agc index error\n");
      ret = -1;
      break;
    }

    snprintf(command, sizeof(command), "%s -S 1012",
             CALI_BAD_PIXEL_PATH);
    status = system(command);
    if (WEXITSTATUS(status)) {
      printf("Restore_bad_pixel_effect: set agc index error!\n");
      ret = -1;
      break;
    }

    snprintf(command, sizeof(command), "%s -r %d,%d",
             CALI_BAD_PIXEL_PATH, sensor_para->width, sensor_para->height);
    status = system(command);
    if (WEXITSTATUS(status)) {
      printf("Restore_bad_pixel_effect: set agc index error!\n");
      ret = -1;
      break;
    }
  } while (0);

  return ret;
}

static int32_t correct_bad_pixel(const int32_t width,
                                 const int32_t height,
                                 const char *file_name)
{
  int32_t ret = 0;
  int32_t status;
  char command[BUF_LEN] = {0};

  do {
    if (!file_name) {
      printf("correct_bad_pixel:file_name is NULL\n");
      ret = -1;
      break;
    }
    snprintf(command, sizeof(command), "%s -c %d,%d -f %s",
             CALI_BAD_PIXEL_PATH, width, height, file_name);
    status = system(command);
    if (WEXITSTATUS(status)) {
      printf("correct_bad_pixel:Failed to correct bad pixel using file %s!\n",
             file_name);
      ret = -1;
      break;
    }
  } while (0);

  return ret;
}

am_mp_err_t mptool_bad_pixel_calibration(am_mp_msg_t *from_msg,
                                        am_mp_msg_t *to_msg)
{
  int32_t status;
  char cmd[BUF_LEN * 2] = {0};
  mp_sensor_msg sensor_msg;
  memcpy(&sensor_msg, from_msg->msg, sizeof(sensor_msg));
  to_msg->result.ret = MP_OK;

  switch (from_msg->stage) {
    case SYSTEM_INIT:
      if (check_sys_resources()) {
        to_msg->result.ret = MP_ERROR;
        break;
      }

      if (init_sensor_parameter(&sensor_msg, &sensor_para) != 0) {
        to_msg->result.ret = MP_ERROR;
      }
      break;

    case BPC_START_ENCODING:
      if (bpc_enter_preview(&sensor_para)) {
        to_msg->result.ret = MP_ERROR;
        break;
      }

      snprintf(cmd, sizeof(cmd), "%s &", RTSP_SERVER_PATH);
      status = system(cmd);
      if (WEXITSTATUS(status)) {
        printf("Failed to start %s!\n", RTSP_SERVER_PATH);
        to_msg->result.ret = MP_ERROR;
        break;
      }
      //sleep(2);
      if (bpc_start_encode(&sensor_para)) {
        to_msg->result.ret = MP_ERROR;
        break;
      }
      break;
    case BRIGHT_BAD_PIXEL_DETECTION:
      snprintf(cmd, sizeof(cmd),
               "%s -d %d,%d,160,160,0,%d,%d,3,%d,1012 -f %s",
               CALI_BAD_PIXEL_PATH, sensor_para.width, sensor_para.height,
               sensor_para.high_thresh, sensor_para.low_thresh,
               sensor_para.agc_index, BRIGHT_BAD_PIXEL_FILE);
      status = system(cmd);
      if (WEXITSTATUS(status)) {
        printf("bad pixel detection failed!\n");
        to_msg->result.ret = MP_ERROR;
        break;
      }

      printf("Bright bad pixel detection finished, result saved in %s\n",
             BRIGHT_BAD_PIXEL_FILE);
      break;
    case RESTORE_BAD_PIXEL_EFFECT:
      if (restore_bad_pixel_effect(&sensor_para)) {
        to_msg->result.ret = MP_ERROR;
        break;
      }

      if (bpc_start_encode(&sensor_para)) {
        to_msg->result.ret = MP_ERROR;
        break;
      }
      break;
    case CORRECT_BRIGHT_BAD_PIXEL:
      if (correct_bad_pixel(sensor_para.width,
                            sensor_para.height,
                            BRIGHT_BAD_PIXEL_FILE)) {
        to_msg->result.ret = MP_ERROR;
        break;
      }
      break;
    case BPC_RESTART_ENCODING:
      if (bpc_enter_preview(&sensor_para)) {
        to_msg->result.ret = MP_ERROR;
        break;
      }
      //sleep(2);
      if (bpc_start_encode(&sensor_para)) {
        to_msg->result.ret = MP_ERROR;
        break;
      }
      break;
    case DARK_BAD_PIXEL_DETECTION:
      snprintf(cmd, sizeof(cmd),
               "%s -d %d,%d,16,16,1,20,40,3,0,1012 -f %s",
               CALI_BAD_PIXEL_PATH, sensor_para.width, sensor_para.height,
               DARK_BAD_PIXEL_FILE);
      status = system(cmd);
      if (WEXITSTATUS(status)) {
        printf("bad pixel detection failed!\n");
        to_msg->result.ret = MP_ERROR;
        break;
      }

      printf("Dark bad pixel detection finished, result saved in %s\n",
             DARK_BAD_PIXEL_FILE);
      break;
    case CORRECT_DARK_BAD_PIXEL:
      if (correct_bad_pixel(sensor_para.width,
                            sensor_para.height,
                            DARK_BAD_PIXEL_FILE)) {
        to_msg->result.ret = MP_ERROR;
        break;
      }
      break;
    case FINISH_CALIBRATION:
      snprintf(cmd, sizeof(cmd), "%s -o %s,%s,%s",
               BITMAP_MERGER_PATH, BRIGHT_BAD_PIXEL_FILE,
               DARK_BAD_PIXEL_FILE, CALI_BAD_PIXEL_FILE);
      status = system(cmd);
      if (WEXITSTATUS(status)) {
        printf("Failed to merge bad pixel bitmap!\n");
        to_msg->result.ret = MP_ERROR;
        break;
      }

      snprintf(cmd, sizeof(cmd), "rm -rf %s %s",
               BRIGHT_BAD_PIXEL_FILE, DARK_BAD_PIXEL_FILE);
      status = system(cmd);
      if (WEXITSTATUS(status)) {
        printf("Failed to delete bright/dark_bad_pixel.bin!\n");
        to_msg->result.ret = MP_ERROR;
        break;
      }

      if (correct_bad_pixel(sensor_para.width,
                            sensor_para.height,
                            CALI_BAD_PIXEL_FILE)) {
        to_msg->result.ret = MP_ERROR;
        break;
      }
      break;

    case BAD_PIXEL_RESULT_SAVE:
      snprintf(cmd, sizeof(cmd),
               "%s -A -s", TEST_ENCODE_PATH);

      status = system(cmd);
      if (WEXITSTATUS(status)) {
        strcpy(to_msg->msg, "result verify: test_encode -A -s failed!\n");
        to_msg->result.ret = MP_ERROR;
        break;
      }

      system("kill -9 `pidof rtsp_server`");

      snprintf(cmd, sizeof(cmd), "rm -rf %s", CALI_BAD_PIXEL_FILE);
      status = system(cmd);
      if (WEXITSTATUS(status)) {
        printf("Failed to delete bright/dark_bad_pixel.bin!\n");
        to_msg->result.ret = MP_ERROR;
        break;
      }

      if (mptool_save_data(from_msg->result.type,
                           from_msg->result.ret, SYNC_FILE) != MP_OK) {
        printf("mptool save data failed!\n");
        to_msg->result.ret = MP_ERROR;
        break;
      }
      break;
    default:
      to_msg->result.ret = MP_NOT_SUPPORT;
      break;
  }
  return MP_OK;
}

