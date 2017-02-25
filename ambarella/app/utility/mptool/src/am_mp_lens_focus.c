/*******************************************************************************
 * am_mp_lens_focus.cpp
 *
 * History:
 *   May 11, 2015 - [longli] created file
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
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "am_mp_server.h"
#include "am_mp_lens_focus.h"

#include "img_adv_struct_arch.h"
#include "mw_struct.h"
#include "mw_api.h"
#include "AmbaDSP_Img3aStatistics.h"
#include "AmbaDSP_ImgUtility.h"
#include "img_customer_interface_arch.h"

typedef enum {
  LENS_FOCUS_HAND_SHAKE = 0,
  LENS_FOCUS_START_RTSP,
  LENS_FOCUS_STOP_RTSP,
  LENS_FOCUS_GET_AF_DATA,
} mp_lens_focus_stage_t;

typedef struct {
  uint8_t *cfa_aaa;
  uint8_t *rgb_aaa;
  uint8_t *cfa_pre;
  uint8_t *cfa_data_valid;
  uint8_t *rgb_data_valid;
  uint8_t *cfa_pre_hdr_data_valid;
}statistic_data_t;

extern int32_t fd_iav;

static int32_t init_for_get_statistics(statistic_data_t *stat)
{
  int ret = 0;
  stat->cfa_aaa = (uint8_t *)malloc(sizeof(struct cfa_aaa_stat) *
                                    MAX_SLICE_NUM);
  stat->rgb_aaa = (uint8_t *)malloc(sizeof(struct rgb_aaa_stat) *
                                    MAX_SLICE_NUM);
  stat->cfa_pre = (uint8_t *)malloc(sizeof(struct cfa_pre_hdr_stat) *
                                    MAX_PRE_HDR_STAT_BLK_NUM);
  stat->cfa_data_valid = (uint8_t *)malloc(sizeof(u8));
  stat->rgb_data_valid = (uint8_t *)malloc(sizeof(u8));
  stat->cfa_pre_hdr_data_valid = (uint8_t *)malloc(sizeof(u8));

  if (!(stat->cfa_aaa && stat->rgb_aaa && stat->cfa_pre
      && stat->cfa_data_valid && stat->rgb_data_valid
      && stat->cfa_pre_hdr_data_valid)) {
    ret = -1;
  }

  return ret;
}

static int32_t deinit_for_get_statistics(statistic_data_t *stat)
{
  if (stat->cfa_aaa) {
    free(stat->cfa_aaa);
    stat->cfa_aaa = NULL;
  }
  if (stat->rgb_aaa) {
    free(stat->rgb_aaa);
    stat->rgb_aaa = NULL;
  }
  if (stat->cfa_pre) {
    free(stat->cfa_pre);
    stat->cfa_pre = NULL;
  }
  if (stat->cfa_data_valid) {
    free(stat->cfa_data_valid);
    stat->cfa_data_valid = NULL;
  }
  if (stat->rgb_data_valid) {
    free(stat->rgb_data_valid);
    stat->rgb_data_valid = NULL;
  }
  if (stat->cfa_pre_hdr_data_valid) {
    free(stat->cfa_pre_hdr_data_valid);
    stat->cfa_pre_hdr_data_valid = NULL;
  }
  return 0;
}

static int32_t get_statistics_data_af(uint32_t *paf_sum)
{
  mw_sys_info G_mw_info;
  int32_t ret = 0;
  amba_dsp_aaa_statistic_data_t g_stat;
  amba_img_dsp_mode_cfg_t ik_mode;
  aaa_tile_report_t act_tile;
  statistic_data_t stat;
  img_aaa_stat_t hdr_statis_gp[MAX_HDR_EXPOSURE_NUM];

  do {
    if (!paf_sum) {
      printf("paf_sum is NULL\n");
      ret = -1;
      break;
    }

    memset(&stat, 0, sizeof(stat));
    memset(&ik_mode, 0, sizeof(ik_mode));
    memset(&act_tile, 0, sizeof(act_tile));
    if (init_for_get_statistics(&stat) < 0) {
      perror("init_for_get_statistics");
      ret = -1;
      break;
    }

    memset(&g_stat, 0, sizeof(amba_dsp_aaa_statistic_data_t));
    g_stat.CfaAaaDataAddr = (u32)(stat.cfa_aaa);
    g_stat.RgbAaaDataAddr = (u32)(stat.rgb_aaa);
    g_stat.CfaPreHdrDataAddr = (u32)(stat.cfa_pre);
    g_stat.RgbDataValid = (u32)(stat.rgb_data_valid);
    g_stat.CfaDataValid = (u32)(stat.cfa_data_valid);
    g_stat.CfaPreHdrDataValid = (u32)(stat.cfa_pre_hdr_data_valid);

    if (mw_get_sys_info(&G_mw_info) < 0) {
      printf("mw_get_sensor_hdr_expo error\n");
      ret = -1;
      break;
    }

    if(amba_img_dsp_3a_get_aaa_stat(fd_iav, &ik_mode, &g_stat)<0) {
      perror("amba_img_dsp_3a_get_aaa_stat");
      ret = -1;
      break;
    }

    if(parse_aaa_data(&g_stat, (hdr_pipeline_t)G_mw_info.res.isp_pipeline,
                      hdr_statis_gp, &act_tile) < 0) {
      perror("parse_aaa_data");
      ret = -1;
      break;
    }

    af_stat_t *pAf_data = hdr_statis_gp[0].af_info;
    uint32_t sum_fv2, old_sum_fv2,total_size;
    uint32_t i = 0;

    // if hdr expo num > 1, get the first one
    total_size = hdr_statis_gp[0].tile_info.af_tile_num_col *
        hdr_statis_gp[0].tile_info.af_tile_num_row;
    old_sum_fv2 = sum_fv2 = 0;
    for (i = 0; i < total_size; ++i) {
      old_sum_fv2 = sum_fv2;
      sum_fv2 += pAf_data[i].sum_fv2;
      if (sum_fv2 < old_sum_fv2) {
        printf("af data is overflow\n");
        ret = -1;
        break;
      }
    }

    if (i == total_size) {
      memcpy(paf_sum, &sum_fv2, sizeof(uint32_t));
    }
  } while (0);

  deinit_for_get_statistics(&stat);

  return ret;
}


am_mp_err_t mptool_lens_focus_handler(am_mp_msg_t *from_msg,
                                      am_mp_msg_t *to_msg)
{
  int32_t status = 0;
  uint32_t af_data = 0;
  to_msg->result.ret = MP_OK;

  switch(from_msg->stage) {
    case LENS_FOCUS_HAND_SHAKE:
      break;
    case LENS_FOCUS_START_RTSP:
      status = system("/usr/local/bin/test_encode -i0 -X --bmaxsize 720p --bsize 720p -A -h720p -J --btype off");
      if (WEXITSTATUS(status)) {
        printf("result verify: %s failed!\n", "test_encode");
        to_msg->result.ret = MP_ERROR;
        break;
      }

      if (mw_start_aaa(fd_iav) < 0) {
        perror("mw_start_aaa");
        to_msg->result.ret = MP_ERROR;
        break;
      }

      /* Start rtsp server*/
      status = system("/usr/local/bin/rtsp_server &");
      if (WEXITSTATUS(status)) {
        printf("result verify: %s failed!\n", "rtsp_server");
        to_msg->result.ret = MP_ERROR;
        break;
      }

      sleep(1);

      status = system("/usr/local/bin/test_encode -A -e");
      if (WEXITSTATUS(status)) {
        printf("result verify: %s failed!\n", "test_encode -e");
        to_msg->result.ret = MP_ERROR;
        break;
      }

      break;
    case LENS_FOCUS_STOP_RTSP:
      /*stop rtsp server*/
      status = system("/usr/local/bin/test_encode -A -s");
      if (WEXITSTATUS(status)) {
        printf("result verify: test_encode -A -s failed!\n");
        to_msg->result.ret = MP_ERROR;
        break;
      }

      if (mw_stop_aaa() < 0) {
        perror("mw_stop_aaa");
        to_msg->result.ret = MP_ERROR;
        break;
      }

      system("kill -9 `pidof rtsp_server`");

      if (mptool_save_data(from_msg->result.type, from_msg->result.ret, SYNC_FILE) != MP_OK) {
        printf("save failed! \n");
        to_msg->result.ret = MP_ERROR;
      }
      break;
    case LENS_FOCUS_GET_AF_DATA:
      if (get_statistics_data_af(&af_data)<0)
        to_msg->result.ret = MP_ERROR;
      memcpy(to_msg->msg, &af_data ,sizeof (u32));
      break;
    default:
      to_msg->result.ret = MP_NOT_SUPPORT;
      break;
  }

  return MP_OK;
}
