/*******************************************************************************
 * air_api_handle.cpp
 *
 * History:
 *   Nov 19, 2015 - [Huaiqing Wang] created file
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
 ******************************************************************************/
#include "am_base_include.h"
#include "am_log.h"
#include "am_define.h"
#include "am_api_video.h"
#include "am_video_types.h"
#include "am_api_helper.h"
#include "am_video_reader_if.h"
#include "air_api_handle.h"

static AMAPIHelperPtr g_api_helper = nullptr;
static AMIVideoReaderPtr reader = nullptr;

int32_t api_helper_init()
{
  int32_t ret = 0;
  g_api_helper = AMAPIHelper::get_instance();
  if (!g_api_helper) {
    ERROR("unable to get AMAPIHelper instance\n");
    ret = -1;
  }

  return ret;
};


int32_t add_stream_overlay(const overlay_layout_desc *position,
                                  const char *bmp_file)
{
  int32_t ret = 0;
  do {
    if (!position || !bmp_file) {
      ERROR("NULL pointer!");
      ret = -1;
      break;
    }
    am_service_result_t service_result;
    am_overlay_s add_cfg = {0};

    add_cfg.stream_id = OVERLAY_STREAM_ID;
    SET_BIT(add_cfg.enable_bits, AM_TYPE_EN_BIT);
    add_cfg.area.type = 2;

    SET_BIT(add_cfg.enable_bits, AM_ADD_EN_BIT);
    add_cfg.area.enable = 1;

    SET_BIT(add_cfg.enable_bits, AM_LAYOUT_EN_BIT);
    if (4 == position->layout_id) {
      add_cfg.area.layout = 4;
      SET_BIT(add_cfg.enable_bits, AM_OFFSETX_EN_BIT);
      add_cfg.area.offset_x = position->offset.x;
      SET_BIT(add_cfg.enable_bits, AM_OFFSETY_EN_BIT);
      add_cfg.area.offset_y = position->offset.y;
    } else {
      add_cfg.area.layout = position->layout_id;
    }

    SET_BIT(add_cfg.enable_bits, AM_ROTATE_EN_BIT);
    add_cfg.area.rotate = 1;

    SET_BIT(add_cfg.enable_bits, AM_COLOR_KEY_EN_BIT);
    add_cfg.area.color_key = 255;

    SET_BIT(add_cfg.enable_bits, AM_COLOR_RANGE_EN_BIT);
    add_cfg.area.color_range = 0;

    uint32_t len = strlen(bmp_file);
    if (len >= OSD_MAX_FILENAME) {
      ERROR("BMP filename size is too long(%d bytes)! \n", len);
      ret = -1;
      break;
    }
    SET_BIT(add_cfg.enable_bits, AM_BMP_EN_BIT);
    strncpy(add_cfg.area.bmp, bmp_file, len);
    add_cfg.area.bmp[len] = '\0';

    g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_OVERLAY_ADD,
                              &add_cfg, sizeof(add_cfg),
                              &service_result, sizeof(service_result));
    if ((ret = service_result.ret) != 0) {
      ERROR("failed to set overlay config!\n");
      break;
    }
  } while(0);

  return ret;
}

int32_t delete_stream_overlay(uint32_t area_id)
{
  int32_t ret = 0;
  am_service_result_t service_result;
  am_overlay_set_t set_cfg = {0};

  set_cfg.osd_id.stream_id = OVERLAY_STREAM_ID;
  SET_BIT(set_cfg.enable_bits, AM_REMOVE_EN_BIT);
  set_cfg.osd_id.area_id = area_id;

  g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_OVERLAY_SET,
                            &set_cfg, sizeof(set_cfg),
                            &service_result, sizeof(service_result));
  if ((ret = service_result.ret) != 0) {
    ERROR("failed to set overlay config!\n");
  }

  return ret;
}

int32_t destroy_stream_overlay()
{
  int32_t ret = 0;
  am_service_result_t service_result;
  g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_OVERLAY_DESTROY, nullptr, 0,
                            &service_result, sizeof(service_result));
  if ((ret = service_result.ret) != 0) {
    ERROR("failed to destroy overlay!\n");
  }
  return ret;
}

int32_t init_bmp_overlay_area(const point_desc &offset, const size_desc &size)
{
  am_service_result_t service_result;
  am_bmp_overlay_area_t area = {0};
  int32_t *area_id = nullptr;

  SET_BIT(area.enable_bits, AM_BMP_AREA_INIT_EN_BIT);
  area.stream_id = OVERLAY_STREAM_ID;

  SET_BIT(area.enable_bits, AM_OFFSETX_EN_BIT);
  area.offset_x = offset.x;

  SET_BIT(area.enable_bits, AM_OFFSETY_EN_BIT);
  area.offset_y = offset.y;

  SET_BIT(area.enable_bits, AM_WIDTH_EN_BIT);
  area.width = size.width;

  SET_BIT(area.enable_bits, AM_HEIGHT_EN_BIT);
  area.height = size.height;

  g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_BMP_OVERLAY_INIT,
                            &area, sizeof(area),
                            &service_result, sizeof(service_result));
  if (service_result.ret != 0) {
    ERROR("failed to init bmp overlay area!\n");
  } else {
     area_id = (int32_t*)service_result.data;
    PRINTF("The bmp overlay area id = %d\n", *area_id);
  }

  return area_id ? (*area_id) : -1;
}

int32_t add_bmp_to_area(const bmp_overlay_manipulate_desc &bmp)
{
  int32_t ret = 0;
  am_service_result_t service_result;
  am_bmp_overlay_area_t area = {0};

  SET_BIT(area.enable_bits, AM_BMP_AREA_ADD_EN_BIT);
  area.stream_id = OVERLAY_STREAM_ID;
  area.area_id = bmp.area_id;

  SET_BIT(area.enable_bits, AM_OFFSETX_EN_BIT);
  area.offset_x = bmp.offset.x;

  SET_BIT(area.enable_bits, AM_OFFSETY_EN_BIT);
  area.offset_y = bmp.offset.y;

  SET_BIT(area.enable_bits, AM_COLOR_KEY_EN_BIT);
  area.color_key = bmp.color_key;

  SET_BIT(area.enable_bits, AM_COLOR_RANGE_EN_BIT);
  area.color_range = bmp.color_range;

  SET_BIT(area.enable_bits, AM_BMP_EN_BIT);
  uint32_t len = strlen(bmp.bmp_file);
  if (len >= FILE_NAME_NUM_MAX) {
    WARN("BMP filename size is too long(%d bytes)! \n", len);
    return -1;
  }
  strncpy(area.bmp, bmp.bmp_file, len);
  area.bmp[len] = '\0';

  g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_BMP_OVERLAY_ADD,
                            &area, sizeof(area),
                            &service_result, sizeof(service_result));
  if (service_result.ret != 0) {
    ERROR("failed to add bmp to area!\n");
    ret = -1;
  }

  return ret;
}

int32_t render_bmp_overlay_area(int32_t area_id)
{
  int32_t ret = 0;
  am_service_result_t service_result;
  am_overlay_id_t id = {0};

  SET_BIT(id.enable_bits, AM_BMP_AREA_RENDER_EN_BIT);
  id.stream_id = OVERLAY_STREAM_ID;
  id.area_id = area_id;

  g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_BMP_OVERLAY_RENDER,
                            &id, sizeof(id),
                            &service_result, sizeof(service_result));
  if (service_result.ret != 0) {
    ERROR("failed to render area to display!\n");
    ret = -1;
  }
  return ret;
}

int32_t update_bmp_overlay_area(const bmp_overlay_manipulate_desc &bmp)
{
  int32_t ret = 0;
  am_service_result_t service_result;
  am_bmp_overlay_area_t area = {0};

  SET_BIT(area.enable_bits, AM_BMP_AREA_UPDATE_EN_BIT);
  area.stream_id = OVERLAY_STREAM_ID;
  area.area_id = bmp.area_id;

  SET_BIT(area.enable_bits, AM_OFFSETX_EN_BIT);
  area.offset_x = bmp.offset.x;

  SET_BIT(area.enable_bits, AM_OFFSETY_EN_BIT);
  area.offset_y = bmp.offset.y;

  SET_BIT(area.enable_bits, AM_COLOR_KEY_EN_BIT);
  area.color_key = bmp.color_key;

  SET_BIT(area.enable_bits, AM_COLOR_RANGE_EN_BIT);
  area.color_range = bmp.color_range;

  SET_BIT(area.enable_bits, AM_BMP_EN_BIT);
  uint32_t len = strlen(bmp.bmp_file);
  if (len >= FILE_NAME_NUM_MAX) {
    WARN("BMP filename size is too long(%d bytes)! \n", len);
    return -1;
  }
  strncpy(area.bmp, bmp.bmp_file, len);
  area.bmp[len] = '\0';

  g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_BMP_OVERLAY_UPDATE,
                            &area, sizeof(area),
                            &service_result, sizeof(service_result));
  if (service_result.ret != 0) {
    ERROR("failed to update overlay area!\n");
    ret = -1;
  }

  return ret;
}

int32_t start_encode()
{
  int32_t ret = 0;
  am_service_result_t service_result;
  g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_ENCODE_START, nullptr, 0,
                            &service_result, sizeof(service_result));
  if ((ret = service_result.ret) != 0) {
    ERROR("failed to start encoding!\n");
  }
  return ret;
}

int32_t stop_encode()
{
  int32_t ret = 0;
  am_service_result_t service_result;
  g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_ENCODE_STOP, nullptr, 0,
                            &service_result, sizeof(service_result));
  if ((ret = service_result.ret) != 0) {
    ERROR("failed to stop encoding!\n");
  }
  return ret;
}
int32_t set_input_size_dynamic(int32_t buf_id, const buffer_crop_desc *rect)
{
  int32_t ret = 0;
  do {
    if (!rect) {
      ERROR("NULL pointer!");
      ret = -1;
      break;
    }
    am_service_result_t service_result;
    am_dptz_warp_t dptz_warp_cfg = { 0 };

    dptz_warp_cfg.buf_id = buf_id;

    SET_BIT(dptz_warp_cfg.enable_bits, AM_DPTZ_SIZE_X_EN_BIT);
    dptz_warp_cfg.x = rect->offset.x;

    SET_BIT(dptz_warp_cfg.enable_bits, AM_DPTZ_SIZE_Y_EN_BIT);
    dptz_warp_cfg.y = rect->offset.y;

    SET_BIT(dptz_warp_cfg.enable_bits, AM_DPTZ_SIZE_W_EN_BIT);
    dptz_warp_cfg.w = rect->size.width;

    SET_BIT(dptz_warp_cfg.enable_bits, AM_DPTZ_SIZE_H_EN_BIT);
    dptz_warp_cfg.h = rect->size.height;

    g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_DYN_DPTZ_SIZE_SET,
                              &dptz_warp_cfg,
                              sizeof(dptz_warp_cfg),
                              &service_result,
                              sizeof(service_result));
    if ((ret = service_result.ret) != 0) {
      ERROR("failed to set dptz_warp config!\n");
    }

  } while(0);

  return ret;
}

int32_t set_source_buffer_size(int32_t buf_id, const buffer_crop_desc *rect)
{
  int32_t ret = 0;
  do {
    if (!rect) {
      ERROR("NULL pointer!");
      ret = -1;
      break;
    }
    am_service_result_t service_result;
    am_buffer_fmt_t buffer_fmt = {0};

    if (buf_id >= AM_ENCODE_SOURCE_BUFFER_MAX_NUM) {
      ERROR("invalid buffer id: %d\n",buf_id);
      break;
    }

    buffer_fmt.buffer_id = buf_id;

    SET_BIT(buffer_fmt.enable_bits, AM_BUFFER_FMT_TYPE_EN_BIT);
    buffer_fmt.type = 2; //preview

    SET_BIT(buffer_fmt.enable_bits, AM_BUFFER_FMT_INPUT_CROP_EN_BIT);
    buffer_fmt.input_crop = 1;

    SET_BIT(buffer_fmt.enable_bits, AM_BUFFER_FMT_INPUT_WIDTH_EN_BIT);
    buffer_fmt.input_width = rect->size.width;

    SET_BIT(buffer_fmt.enable_bits, AM_BUFFER_FMT_INPUT_HEIGHT_EN_BIT);
    buffer_fmt.input_height = rect->size.height;

    SET_BIT(buffer_fmt.enable_bits, AM_BUFFER_FMT_INPUT_X_EN_BIT);
    buffer_fmt.input_offset_x = rect->offset.x;

    SET_BIT(buffer_fmt.enable_bits, AM_BUFFER_FMT_INPUT_Y_EN_BIT);
    buffer_fmt.input_offset_y = rect->offset.y;

    SET_BIT(buffer_fmt.enable_bits, AM_BUFFER_FMT_WIDTH_EN_BIT);
    buffer_fmt.width = rect->size.width;

    SET_BIT(buffer_fmt.enable_bits, AM_BUFFER_FMT_HEIGHT_EN_BIT);
    buffer_fmt.height = rect->size.height;

    g_api_helper->method_call(AM_IPC_MW_CMD_VIDEO_BUFFER_FMT_SET,
                              &buffer_fmt, sizeof(buffer_fmt),
                              &service_result, sizeof(service_result));
    if ((ret = service_result.ret) != 0) {
      ERROR("failed to set source buffer format!\n");
      break;
    }
  } while(0);

  return ret;
}

int32_t get_gray_frame(int32_t buf_id, gray_frame_desc *data)
{
  int32_t ret = 0;
  do {
    if (!data) {
      ERROR("NULL pointer!");
      ret = -1;
      break;
    }
    AM_RESULT result;
    AMMemMapInfo dsp_mem;
    AMQueryDataFrameDesc frame_desc;
    if (nullptr == reader) {
      reader = AMIVideoReader::get_instance();
      if (!reader) {
        ERROR("unable to get AMVideoReader instance \n");
        ret = -1;
        break;
      }
      result = reader->init();
      if (result != AM_RESULT_OK) {
        ERROR("unable to init AMVideoReader\n");
        ret = -2;
        break;
      }
      result = reader->get_dsp_mem(&dsp_mem);
      if (result != AM_RESULT_OK) {
        ERROR("get bsb mem failed \n");
        ret = -1;
        break;
      }
    }
    if ((result=reader->query_yuv_frame(&frame_desc,
                                AM_ENCODE_SOURCE_BUFFER_ID(buf_id),
                                false)) == AM_RESULT_ERR_AGAIN) {
      //DSP state not ready to query video, small pause and query state again
      printf("try get frame again!!!");
      usleep(100 * 1000);
      continue;
    } else if (result != AM_RESULT_OK) {
      ERROR("query yuv frame failed \n");
      ret = -3;
      break;
    }

    data->width = frame_desc.yuv.width;
    data->height = frame_desc.yuv.height;
    data->pitch = frame_desc.yuv.pitch;
    data->seq_num = frame_desc.yuv.seq_num;
    data->addr = dsp_mem.addr + frame_desc.yuv.y_addr_offset;
    data->mono_pts = frame_desc.mono_pts;
  } while(0);

  return ret;
}
