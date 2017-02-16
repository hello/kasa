/*******************************************************************************
 * am_osd_overlay.cpp
 *
 * History:
 *   2014-12-3 - [lysun] created file
 *   2015-6-24 - [Huaiqing Wang] reconstruct
 * Copyright (C) 2008-2014, Ambarella Co,Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/
#include <string.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include "iav_ioctl.h"
#include "am_encode_device.h"
#include "am_log.h"
#include "am_osd_overlay.h"
#include "am_define.h"
#include "text_insert.h"
#include <wchar.h>
#include <locale.h>

static std::recursive_mutex m_mtx;
#define  DECLARE_MUTEX  std::lock_guard<std::recursive_mutex> lck (m_mtx);

////////////// AMOSDOverlay class, the class to draw OSD overlay /////////
AMOSDOverlay* AMOSDOverlay::m_osd_instance = NULL;
AMOSDOverlay::AMOSDOverlay():
    m_encode_device(NULL),
    m_iav(-1),
    m_area_buffer_size(0),
    m_stream_buffer_size(0),
    m_time_num(0),
    m_is_mapped(false),
    m_thread_hand(NULL)
{
  for (int i = 0; i < OSD_OBJ_MAX_NUM; i ++)
    m_osd[i] = NULL;
  for (int i = 0; i < AM_STREAM_MAX_NUM; i ++)
    m_time_osd[i] = NULL;
  memset(m_osd_area, 0, sizeof(m_osd_area));
  memset(&m_osd_mem, 0, sizeof(AMMemMapInfo));
  memset(m_layout, 0, sizeof(m_layout));
}

AMIOSDOverlay* AMIOSDOverlay::create()
{
  return AMOSDOverlay::create();
}

AMOSDOverlay* AMOSDOverlay::create()
{
  DECLARE_MUTEX;
  if (!m_osd_instance) {
    m_osd_instance = new AMOSDOverlay();
  }
  return m_osd_instance;
}

AM_RESULT AMOSDOverlay::init(int32_t fd_iav, AMEncodeDevice *encode_device)
{
  if (!encode_device) {
    ERROR("AMOSDOverlay:: init error \n");
    return AM_RESULT_ERR_INVALID;
  }

  if (fd_iav < 0) {
    ERROR("AMOSDOverlay: wrong iav handle to init \n");
    return AM_RESULT_ERR_INVALID;
  }

  m_iav = fd_iav;
  m_encode_device = encode_device;

  return AM_RESULT_OK;
}

bool AMOSDOverlay::destroy()
{
  DECLARE_MUTEX;
  for (int32_t i = 0; i < AM_STREAM_MAX_NUM; ++ i) {
    remove((AM_STREAM_ID) i);
  }
  m_thread_hand->destroy();
  m_thread_hand = NULL;

  return  unmap_overlay();;
}

bool AMOSDOverlay::add(AM_STREAM_ID stream_id, AMOSDInfo *overlay_info)
{
  int32_t begin_id = 0, end_id = 0, osd_obj_id = 0, area_id = 0;
  bool changed = false;

  if (!check_stream_id(stream_id) || !overlay_info) {
    ERROR("Invalid parameter!");
    return false;
  }
  if (get_iva_hanle() < 0 || !map_overlay()) {
    return false;
  }

  //find out stream_id's all area osd range in m_osd[OSD_OBJ_MAX_NUM]
  if (!get_osd_object_id_range(stream_id, OSD_AREA_MAX_NUM, begin_id, end_id)) {
    return false;
  }

  INFO("area number = %d\n",overlay_info->area_num);
  for (uint32_t i = 0; i < overlay_info->area_num; ++i) {
    for (osd_obj_id = begin_id; osd_obj_id < end_id; ++osd_obj_id) {
      if (m_osd[osd_obj_id]) {
        continue;
      }
      //success find a empty area， add a osd object to it
      area_id = osd_obj_id - begin_id; //empty area id for the stream
      if (!(m_osd_instance->add_osd_area(stream_id, area_id,
                                         &overlay_info->attribute[i],
                                         &overlay_info->overlay_area[i]))) {
        ERROR("Add user osd:%d to stream:%d area:%d failed!",
              i, stream_id, area_id);
      } else {
        changed = true;
        overlay_info->active_area |= (1<<area_id);
        DEBUG("Add user osd:%d to stream:%d area:%d success!",
              i, stream_id, area_id);
        break; //break this loop, otherwise it will create
        //more than one area for the same user osd
      }
    }
  }
  if (changed) {
    apply(stream_id);
  }

  return true;
}

bool AMOSDOverlay::remove(AM_STREAM_ID stream_id, int32_t area_id)
{
  int32_t begin_id = 0, end_id = 0;
  bool changed = false;

  //find out area_id's osd location of stream_id in m_osd[OSD_OBJ_MAX_NUM]
  if (!get_osd_object_id_range(stream_id, area_id, begin_id, end_id)) {
    return false;
  }

  for (int32_t i = begin_id; i < end_id; i ++) {
    if (m_osd[i]) {
      //if the object is a time osd, must delete the backup pointer first
      if (m_time_osd[stream_id] == m_osd[i]) {
        m_time_osd[stream_id] = NULL;
        --m_time_num;
        //if no stream have time osd, destroy the time update thread
        if (0 == m_time_num) {
          m_thread_hand->destroy();
          m_thread_hand = NULL;
        }
      }
      delete m_osd[i]; //destroy the osd object
      m_osd[i] = NULL;
      memset(&m_osd_area[i], 0, sizeof(AMOSDArea));
      changed = true;
    }
  }
  if (true == changed) {
    apply(stream_id);
  } else {
    INFO("No osd object can be remove! \n");
  }

  return true;
}

bool AMOSDOverlay::enable(AM_STREAM_ID stream_id, int32_t area_id)
{
  int32_t begin_id = 0, end_id = 0;
  bool changed = false;

  //find out area_id's osd location of stream_id in m_osd[OSD_OBJ_MAX_NUM]
  if (!get_osd_object_id_range(stream_id, area_id, begin_id, end_id)) {
    return false;
  }

  for (int32_t i = begin_id; i < end_id; ++ i) {
    if (m_osd[i]) {
      if (m_osd_area[i].enable) {
        INFO("Osd object: %d is enabled, need not enable it! \n", i);
        continue;
      }
      m_osd_area[i].enable = 1;
      changed = true;
    }
  }
  if (true == changed) {
    apply(stream_id);
  } else {
    INFO("No osd object can be enable! \n");
  }

  return true;
}

bool AMOSDOverlay::disable(AM_STREAM_ID stream_id, int32_t area_id)
{
  int32_t begin_id = 0, end_id = 0;
  bool changed = false;

  //find out area_id's osd location of stream_id in m_osd[OSD_OBJ_MAX_NUM]
  if (!get_osd_object_id_range(stream_id, area_id, begin_id, end_id)) {
    return false;
  }

  for (int32_t i = begin_id; i < end_id; ++ i) {
    if (m_osd[i]) {
      if (!m_osd_area[i].enable) {
        INFO("Osd object: %d is disabled, need not disable it! \n", i);
        continue;
      }
      m_osd_area[i].enable = 0;
      changed = true;
    }
  }
  if (true == changed) {
    apply(stream_id);
  } else {
    INFO("No osd object can be disable! \n");
  }

  return true;
}

void AMOSDOverlay::print_osd_infomation(AM_STREAM_ID stream_id)
{
  struct iav_overlay_insert overlay_insert;
  struct iav_overlay_area *area;
  int begin = 0, end = 0;

  //if user just create osd instance but not add any object,
  //the m_iav will < 0, so we must open it first
  if (get_iva_hanle() < 0 || !check_encode_state()) {
    return;
  }

  if (AM_STREAM_MAX_NUM == stream_id) {
    begin = 0;
    end = AM_STREAM_MAX_NUM;
  } else {
    begin = stream_id;
    end = stream_id + 1;
  }

  for (int i = begin; i < end; ++ i) {
    memset(&overlay_insert, 0, sizeof(overlay_insert));
    overlay_insert.id = i;
    if (ioctl(m_iav, IAV_IOC_GET_OVERLAY_INSERT, &overlay_insert) < 0) {
      perror("IAV_IOC_GET_OVERLAY_INSERT");
      return;
    }
    for (int j = 0; j < OSD_AREA_MAX_NUM; ++ j) {
      area = &overlay_insert.area[j];
      INFO("stream:%d area:%d is %s, offset(%d x %d), W x H = %d x %d, pitch = %d,"
          " total_size = %d, clut_offset = %d, data_offset = %d\n\n",
          overlay_insert.id, j, (area->enable == 1) ? "enable" : "disable",
          area->start_x, area->start_y, area->width, area->height, area->pitch,
          area->total_size, area->clut_addr_offset, area->data_addr_offset);
    }
  }
}

int32_t AMOSDOverlay::get_iva_hanle()
{
  if (m_iav < 0) {
    m_iav = open("/dev/iav", O_RDWR, 0);
    if (m_iav < 0) {
      PERROR("/dev/iav");
    }
  }

  return m_iav;
}

AMOSDArea* AMOSDOverlay::get_object_area_info(int32_t id)
{
  if (id < 0 || id >= OSD_OBJ_MAX_NUM) {
    ERROR("Invalid object id:%d \n", id);
    return NULL;
  }

  return &m_osd_area[id];
}

bool AMOSDOverlay::add_osd_area(AM_STREAM_ID stream_id, int32_t area_id,
                        AMOSDAttribute *osd_attribute,
                        AMOSDArea *osd_area)
{
  if (!check_stream_id(stream_id) ||
      !check_area_id(area_id) ||
      !osd_attribute || !osd_area ) {
    return false;
  }

  int32_t osd_obj_id = stream_id * OSD_AREA_MAX_NUM + area_id;
  if (!(m_osd[osd_obj_id] = create_osd_object(stream_id,
                                              osd_attribute->type))) {
    ERROR("Create osd object:%d failed!!!", osd_obj_id);
    return false;
  }

  m_osd_area[osd_obj_id].clut_addr_offset = osd_obj_id * OSD_CLUT_SIZE;
  m_osd_area[osd_obj_id].data_addr_offset = OSD_YUV_OFFSET +
      stream_id * m_stream_buffer_size + area_id * m_area_buffer_size;
  m_osd_area[osd_obj_id].enable = osd_area->enable;

  if (!(m_osd[osd_obj_id]->save_attribute(osd_attribute)) ||
      !(m_osd[osd_obj_id]->create(stream_id, area_id, osd_area)) ||
      !(m_osd[osd_obj_id]->render(stream_id, area_id))) {
    //if the object is a time osd, must delete the backup pointer first
    if (m_time_osd[stream_id] == m_osd[osd_obj_id]) {
      m_time_osd[stream_id] = NULL;
      --m_time_num;
      //if no stream have time osd, destroy the time update thread
      if (0 == m_time_num) {
        m_thread_hand->destroy();
        m_thread_hand = NULL;
      }
    }
    delete m_osd[osd_obj_id];
    m_osd[osd_obj_id] = NULL;
    memset(&m_osd_area[osd_obj_id], 0, sizeof(AMOSDArea));

    ERROR("AMOSDOverlay: stream:%d area:%d osd object create error\n",
          stream_id, area_id);
    return false;
  }

  return true;
}

void AMOSDOverlay::apply(AM_STREAM_ID stream_id)
{
  struct iav_overlay_insert overlay_insert;
  struct iav_overlay_area *area;

  if (!check_encode_state()) {
    return;
  }

  memset(&overlay_insert, 0, sizeof(overlay_insert));
  overlay_insert.id = stream_id;
  overlay_insert.enable = 1;
  for (int32_t i = 0; i < OSD_AREA_MAX_NUM; ++ i) {
    area = &overlay_insert.area[i];
    int32_t obj = stream_id * OSD_AREA_MAX_NUM + i;
    area->enable = m_osd_area[obj].enable;
    area->start_x = m_osd_area[obj].area_position.offset_x;
    area->start_y = m_osd_area[obj].area_position.offset_y;
    area->width = m_osd_area[obj].width;
    area->height = m_osd_area[obj].height;
    area->pitch = m_osd_area[obj].pitch;
    area->total_size = m_osd_area[obj].total_size;
    area->clut_addr_offset = m_osd_area[obj].clut_addr_offset;
    area->data_addr_offset = m_osd_area[obj].data_addr_offset;
  }

  if (ioctl(m_iav, IAV_IOC_SET_OVERLAY_INSERT, &overlay_insert) < 0) {
    perror("IAV_IOC_SET_OVERLAY_INSERT");
  }
}

bool AMOSDOverlay::map_overlay()
{
  if (m_is_mapped) {
    return true;
  }

  struct iav_querybuf querybuf;
  querybuf.buf = IAV_BUFFER_OVERLAY;
  if (ioctl(m_iav, IAV_IOC_QUERY_BUF, &querybuf) < 0) {
    PERROR("IAV_IOC_QUERY_BUF");
    return false;
  }

  uint32_t overlay_size = querybuf.length;
  void * overlay_addr = mmap(NULL, overlay_size, PROT_WRITE,
                             MAP_SHARED, m_iav, querybuf.offset);
  INFO("overlay: map_overlay, start = %p, total size = 0x%x\n",
       overlay_addr, overlay_size);
  if (MAP_FAILED == overlay_addr) {
    PERROR("mmap (%d) failed: %s\n");
    return false;
  }
  m_is_mapped = true;
  m_osd_mem.addr = (uint8_t *) overlay_addr;
  m_osd_mem.length = overlay_size;
  m_stream_buffer_size = (overlay_size - OSD_YUV_OFFSET) / AM_STREAM_MAX_NUM;
  m_area_buffer_size = m_stream_buffer_size / OSD_AREA_MAX_NUM;
  INFO("overlay: overlay max size per stream is 0x%x, max size per area "
      "is 0x%x \n", m_stream_buffer_size, m_area_buffer_size);

  return true;
}

bool AMOSDOverlay::unmap_overlay()
{
  if (m_osd_mem.addr && m_is_mapped) {
    if (munmap(m_osd_mem.addr, m_osd_mem.length) < 0) {
      PERROR("AMVideoDevice: unmap_overlay \n");
      return false;
    }
    memset(&m_osd_mem, 0, sizeof(m_osd_mem));
    m_is_mapped = false;
  }
  return true;
}

bool AMOSDOverlay::get_osd_object_id_range(AM_STREAM_ID stream_id,
                      int32_t area_id, int32_t &begin, int32_t &end)
{
  if (!check_stream_id(stream_id)
      || !(OSD_AREA_MAX_NUM == area_id || check_area_id(area_id))) {
    INFO("Invalid parameter!!! \n");
    return false;
  }

  if (OSD_AREA_MAX_NUM == area_id) {
    begin = stream_id * OSD_AREA_MAX_NUM;
    end = stream_id * OSD_AREA_MAX_NUM + OSD_AREA_MAX_NUM;
  } else {
    begin = stream_id * OSD_AREA_MAX_NUM + area_id;
    end = stream_id * OSD_AREA_MAX_NUM + area_id + 1;
  }
  return true;
}

bool AMOSDOverlay::check_stream_id(AM_STREAM_ID stream_id)
{
  if (stream_id <= AM_STREAM_ID_INVALID || stream_id >= AM_STREAM_MAX_NUM) {
    ERROR("Invalid stream id: %d \n", stream_id);
    return false;
  }
  return true;
}

bool AMOSDOverlay::check_area_id(int32_t area_id)
{
  if (area_id < 0 || area_id >= OSD_AREA_MAX_NUM) {
    INFO("Invalid area id: %d!!! \n", area_id);
    return false;
  }
  return true;
}

bool AMOSDOverlay::check_encode_state()
{
  int iav_state;
  if (ioctl(m_iav, IAV_IOC_GET_IAV_STATE, &iav_state) < 0) {
    perror("IAV_IOC_GET_IAV_STATE");
    return false;
  }

  if ((iav_state != IAV_STATE_PREVIEW) && (iav_state != IAV_STATE_ENCODING)) {
    ERROR("IAV must be in PREVIEW or ENCODE for OSD.\n");
    return false;
  }
  return true;
}

AMOSD *AMOSDOverlay::create_osd_object(AM_STREAM_ID stream_id, AMOSDType type)
{
  AMOSD *osd_object = NULL;
  switch (type) {
    case AM_OSD_TYPE_TEST_PATTERN:
      osd_object = new AMTestPatternOSD();
      break;
    case AM_OSD_TYPE_PICTURE:
      osd_object = new AMPictureOSD();
      break;
    case AM_OSD_TYPE_GENERIC_STRING:
      osd_object = new AMTextOSD();
      break;
    case AM_OSD_TYPE_TIME_STRING:
      if (!m_thread_hand) {
        if ((m_thread_hand = AMThread::create("time_osd_thread",
                                              m_thread_func, (void*)this)))
          INFO("Overlay Time osd task created!!!");
        else {
          ERROR("Create time osd thread failed!!!");
          break;
        }
      }
      if (!m_time_osd[stream_id]) {
        osd_object = new AMTimeTextOSD();
        m_time_osd[stream_id] = osd_object;
        ++m_time_num;
      } else {
        DEBUG("Stream:%d can't create more than one time osd!", stream_id);
      }
      break;
    default:
      break;
  }

  return osd_object;
}

void AMOSDOverlay::m_thread_func(void* arg)
{
  AMOSDOverlay *osd_overlay = (AMOSDOverlay*) arg;
  timeval timeout;
  int32_t err;

  while (osd_overlay->m_time_num) {
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    do {
      err = select(0, NULL, NULL, NULL, &timeout);
    } while (err < 0 && errno == EINTR);

    for (int32_t i = 0; i < AM_STREAM_MAX_NUM; i ++) {
      if (osd_overlay->m_time_osd[i]) {
        osd_overlay->m_time_osd[i]->update();
        osd_overlay->apply((AM_STREAM_ID)i);
      }
    }
  }
}

//////////// AMOSD class, use for abstract OSD object, parent class////////////
AMOSD::AMOSD() :
    m_streamid(AM_STREAM_ID_INVALID),
    m_alpha(255),
    m_clut(NULL),
    m_data(NULL),
    m_startx(0),
    m_starty(0),
    m_width(0),
    m_height(0),
    m_pitch(0),
    m_color_num(0),
    m_flip_rotate(AM_NO_ROTATE_FLIP),
    m_area_w(0),
    m_area_h(0),
    m_buffer(NULL),
    m_stream_w(0),
    m_stream_h(0)
{
}

bool AMOSD::create(AM_STREAM_ID stream_id, int32_t area_id,
                   AMOSDArea *osd_area)
{
  if (!osd_area) {
    ERROR("Invalid pointer!");
    return false;
  }

  AMMemMapInfo *mem_info = NULL;
  AMOSDArea *area_info = NULL;

  AMOSDOverlay *osd_handle = AMOSDOverlay::m_osd_instance;
  int32_t osd_obj_id = stream_id * OSD_AREA_MAX_NUM + area_id;

  if (!(mem_info = osd_handle->get_mem_info()) ||
      !(area_info = osd_handle->get_object_area_info(osd_obj_id))) {
    return false;
  }

  DEBUG("osd_obj_id:%d user set OSD WxH = %dx%d \n", osd_obj_id,
        osd_area->width, osd_area->height);
  m_streamid = stream_id;
  m_alpha = 255;
  m_area_w = osd_area->width;
  m_area_h = osd_area->height;
  m_clut = (AMOSDCLUT *) (mem_info->addr + area_info->clut_addr_offset);
  m_data = mem_info->addr + area_info->data_addr_offset;
  memset(m_clut, 0, OSD_CLUT_SIZE);

  set_positon(&(osd_area->area_position));

  return get_encode_format(stream_id);
}

bool AMOSD::save_attribute(AMOSDAttribute *attr)
{
  if (!attr) {
    ERROR("Invalid pointer!");
    return false;
  }
  m_attribute = *attr;

  return true;
}

void AMOSD::destroy()
{
  delete[] m_buffer;
  m_buffer = NULL;
}

void AMOSD::update_area_info(AMOSDArea *area)
{
  if (!area) {
    ERROR("Invalid pointer");
    return;
  }
  area->width = m_width;
  area->height = m_height;
  area->pitch = m_pitch;
  area->area_position.offset_x = m_startx;
  area->area_position.offset_y = m_starty;
  area->total_size = area->pitch * area->height;
}

void AMOSD::set_positon(AMOSDLayout *position)
{
  if (!position) {
    ERROR("Invalid pointer");
    return;
  }
  //in auto mode, we just gave the limit value for start x and y,
  //and it will auto adjust to actual value in set_overlay_size
  switch (position->style) {
    case AM_OSD_LAYOUT_AUTO_LEFT_TOP:
      m_startx = 0;
      m_starty = 0;
      break;
    case AM_OSD_LAYOUT_AUTO_RIGHT_TOP:
      m_startx = 10000;
      m_starty = 0;
      break;
    case AM_OSD_LAYOUT_AUTO_LEFT_BOTTOM:
      m_startx = 0;
      m_starty = 10000;
      break;
    case AM_OSD_LAYOUT_AUTO_RIGHT_BOTTOM:
      m_startx = 10000;
      m_starty = 10000;
      break;
    case AM_OSD_LAYOUT_MANUAL:
      m_startx = position->offset_x;
      m_starty = position->offset_y;
      break;
    default:
      DEBUG("Invalid layout parameter:%d, set position to "
          "default value(0,0)! \n", position->style);
      m_startx = 0;
      m_starty = 0;
      break;
  }
}

bool AMOSD::get_encode_format(AM_STREAM_ID stream_id)
{
  uint32_t r_flag = 0;
  struct iav_stream_format format;
  int32_t fd = AMOSDOverlay::m_osd_instance->get_iva_hanle();

  format.id = stream_id;
  if (ioctl(fd, IAV_IOC_GET_STREAM_FORMAT, &format) < 0) {
    perror("IAV_IOC_GET_STREAM_FORMAT");
    return false;
  }

  m_stream_w = format.enc_win.width;
  m_stream_h = format.enc_win.height;
  r_flag |= (format.rotate_cw ? AM_ROTATE_90 : AM_NO_ROTATE_FLIP);
  r_flag |= (format.hflip ? AM_HORIZONTAL_FLIP : AM_NO_ROTATE_FLIP);
  r_flag |= (format.vflip ? AM_VERTICAL_FLIP : AM_NO_ROTATE_FLIP);

  uint16_t user_r_flag = m_attribute.enable_rotate;
  if (user_r_flag) {
    m_flip_rotate = r_flag;
  }
  INFO("Encode stream:%d window width = %d, height = %d, flip_rotate = %d.\n"
      "User rotate flag = %d \n",stream_id, m_stream_w, m_stream_h,
      r_flag, user_r_flag);

  return true;
}

bool AMOSD::rotate_fill(uint8_t *dst, uint32_t dst_pitch, uint32_t dst_width,
                        uint32_t dst_height, uint32_t dst_x, uint32_t dst_y,
                        uint8_t *src, uint32_t src_pitch, uint32_t src_height,
                        uint32_t src_x, uint32_t src_y, uint32_t data_width,
                        uint32_t data_height)
{
  if (!src || !dst) {
    ERROR("src or dst is NULL.");
    return false;
  }

  if ((data_width + src_x > src_pitch) ||
      (data_height + src_y > src_height)) {
    ERROR("Failed to fill overlay data. Rotate flag[%u]. Src:[%ux%u] >= "
        "[%ux%u] + offset[%ux%u],", m_flip_rotate, src_pitch, src_height,
        data_width, data_height, src_x, src_y);
    return false;
  }

  uint8_t *sp = src + src_y * src_pitch + src_x;
  uint8_t *dp = NULL;
  uint32_t row = 0, col = 0;

  switch (m_flip_rotate) {
    case AM_NO_ROTATE_FLIP:
      dp = dst + dst_y * dst_pitch + dst_x;
      for (row = 0; row < data_height; ++ row) {
        memcpy(dp, sp, data_width);
        sp += src_pitch;
        dp += dst_pitch;
      }
      break;
    case AM_CW_ROTATE_90:
      dp = dst + (dst_height - dst_x - 1) * dst_pitch + dst_y;
      for (row = 0; row < data_height; ++ row) {
        for (col = 0; col < data_width; ++ col) {
          *(dp - col * dst_pitch) = *(sp + col);
        }
        sp += src_pitch;
        dp ++;
      }
      break;
    case AM_HORIZONTAL_FLIP:
      dp = dst + dst_y * dst_pitch + dst_width - dst_x - 1;
      for (row = 0; row < data_height; ++ row) {
        for (col = 0; col < data_width; ++ col) {
          *(dp - col) = *(sp + col);
        }
        sp += src_pitch;
        dp += dst_pitch;
      }
      break;
    case AM_VERTICAL_FLIP:
      dp = dst + (dst_height - dst_y - 1) * dst_pitch + dst_x;
      for (row = 0; row < data_height; ++ row) {
        memcpy(dp, sp, data_width);
        sp += src_pitch;
        dp -= dst_pitch;
      }
      break;
    case AM_CW_ROTATE_180:
      dp = dst + (dst_height - dst_y - 1) * dst_pitch + dst_width - dst_x - 1;
      for (row = 0; row < data_height; ++ row) {
        for (col = 0; col < data_width; ++ col) {
          *(dp - col) = *(sp + col);
        }
        sp += src_pitch;
        dp -= dst_pitch;
      }
      break;
    case AM_CW_ROTATE_270:
      dp = dst + dst_x * dst_pitch + dst_width - dst_y - 1;
      for (row = 0; row < data_height; ++ row) {
        for (col = 0; col < data_width; ++ col) {
          *(dp + col * dst_pitch) = *(sp + col);
        }
        sp += src_pitch;
        dp --;
      }
      break;
    case (AM_HORIZONTAL_FLIP | AM_ROTATE_90):
              dp = dst + dst_x * dst_pitch + dst_y;
    for (row = 0; row < data_height; ++ row) {
      for (col = 0; col < data_width; ++ col) {
        *(dp + col * dst_pitch) = *(sp + col);
      }
      sp += src_pitch;
      dp ++;
    }
    break;
    case (AM_VERTICAL_FLIP | AM_ROTATE_90):
              dp = dst + (dst_height - dst_x - 1) * dst_pitch + dst_width - dst_y - 1;
    for (row = 0; row < data_height; ++ row) {
      for (col = 0; col < data_width; ++ col) {
        *(dp - col * dst_pitch) = *(sp + col);
      }
      sp += src_pitch;
      dp --;
    }
    break;
    default:
      break;
  }

  return true;
}

bool AMOSD::fill_overlay_data()
{
  return rotate_fill(m_data, m_pitch, m_width, m_height, 0, 0,
                     m_buffer, m_area_w, m_area_h, 0, 0, m_area_w, m_area_h);
}

void AMOSD::set_overlay_size()
{
  uint32_t source_buf_w, source_buf_h, tmp;
  uint16_t tmp_startx = m_startx, tmp_starty = m_starty;
  uint32_t win_width = m_stream_w, win_height = m_stream_h;

  if (m_flip_rotate & AM_ROTATE_90) {
    source_buf_w = win_height;
    source_buf_h = win_width;
  } else {
    source_buf_w = win_width;
    source_buf_h = win_height;
  }

  if (m_area_w > source_buf_w) {
    WARN("Overlay width[%u] > source buffer width[%u]. Reset width to[%u],"
        "offset x to 0.", m_area_w, source_buf_w, source_buf_w);
    m_area_w = source_buf_w;
    tmp_startx = 0;
  }

  if (m_area_h > source_buf_h) {
    WARN("Overlay height[%u] > source buffer height[%u]. Reset height to[%u],"
        "offset y to 0.", m_area_h, source_buf_h, source_buf_h);
    m_area_h = source_buf_h;
    tmp_starty = 0;
  }

  if (tmp_startx + m_area_w > source_buf_w) {
    tmp = source_buf_w - m_area_w;
    INFO("Overlay width[%u] + offset x[%u] > source buffer width[%u]. "
        "Reset offset x to[%u]", m_area_w, tmp_startx, source_buf_w, tmp);
    tmp_startx = (uint16_t) tmp;
  }

  if (tmp_starty + m_area_h > source_buf_h) {
    tmp = source_buf_h - m_area_h;
    INFO("Overlay height[%u] + offset y[%u] > source buffer height[%u]. "
        "Reset offset x to[%u]", m_area_h, tmp_starty, source_buf_h, tmp);
    tmp_starty = (uint16_t) tmp;
  }

  //m_startx, m_starty, m_width and m_height is relative to mmap buffer,
  //not encode stream that user set.so we must transform it here
  if (m_flip_rotate & AM_ROTATE_90) {
    m_startx = tmp_starty;
    m_starty = (uint16_t)(win_height - m_area_w - tmp_startx);
    m_width = m_area_h;
    m_height = m_area_w;
  } else {
    m_startx = tmp_startx;
    m_starty = tmp_starty;
    m_width = m_area_w;
    m_height = m_area_h;
  }

  if (m_flip_rotate & AM_HORIZONTAL_FLIP) {
    m_startx = win_width - m_width - m_startx;
  }

  if (m_flip_rotate & AM_VERTICAL_FLIP) {
    m_starty = win_height - m_height - m_starty;
  }

  DEBUG("Area state in local memory:[%ux%u]",m_area_w, m_area_h);
  DEBUG("Area state in mmap buffer: [%ux%u] + offset [%ux%u].",
        m_width, m_height, m_startx, m_starty);
}

bool AMOSD::check_overlay()
{
  uint16_t width = 0, height = 0, offset_x = 0, offset_y = 0;

  if (m_width < OSD_WIDTH_ALIGN ||  m_height < OSD_HEIGHT_ALIGN) {
    ERROR("Area width[%u] x height[%u] cannot be smaller than [%ux%u]",
          m_width, m_height, OSD_WIDTH_ALIGN, OSD_HEIGHT_ALIGN);
    return false;
  }

  if (m_width & (OSD_WIDTH_ALIGN - 1)) {
    width = ROUND_DOWN(m_width, OSD_WIDTH_ALIGN);
    INFO("Area width[%u] is not aligned to %u. Round down to [%u].",
         m_width, OSD_WIDTH_ALIGN, width);
    m_width = width;
  }

  if (m_startx & (OSD_OFFSET_X_ALIGN - 1)) {
    offset_x = ROUND_DOWN(m_startx, OSD_OFFSET_X_ALIGN);
    INFO("Area offset x[%u] is not aligned to %u. Round down to [%u].",
         m_startx, OSD_OFFSET_X_ALIGN, offset_x);
    m_startx = offset_x;
  }

  if (m_height & (OSD_HEIGHT_ALIGN - 1)) {
    height = ROUND_DOWN(m_height, OSD_HEIGHT_ALIGN);
    INFO("Area height[%u] is not aligned to %u. Round down to [%u].",
         m_height, OSD_HEIGHT_ALIGN, height);
    m_height = height;
  }

  if (m_starty & (OSD_OFFSET_Y_ALIGN - 1)) {
    offset_y = ROUND_DOWN(m_starty, OSD_OFFSET_Y_ALIGN);
    INFO("Area offset_y [%u] is not aligned to %u. Round down to [%u].",
         m_starty, OSD_OFFSET_Y_ALIGN, offset_y);
    m_starty = offset_y;
  }

  m_pitch = ROUND_UP(m_width, OSD_PITCH_ALIGN);

  uint32_t area_size = m_pitch * m_height;
  uint32_t area_max_size = AMOSDOverlay::m_osd_instance->get_area_buffer_maxsize();
  if (area_size > area_max_size) {
    ERROR("Area size[%u] = [%u x %u] exceeds the max size[%u].",
          area_size, m_pitch, m_height, area_max_size);
    return false;
  }

  if (m_flip_rotate & AM_ROTATE_90) {
    m_area_w = m_height;
    m_area_h = m_width;
  } else {
    m_area_w = m_width;
    m_area_h = m_height;
  }
  DEBUG("After aligned: local memory[%ux%u], mmap buffer[%ux%u]"
      "(pitch[%u]) + offset[%ux%u].", m_area_w, m_area_h,
      m_width, m_height, m_pitch, m_startx, m_starty);

  return true;
}

bool AMOSD::alloc_display_mem()
{
  if (m_area_w && m_area_h) {
    delete[] m_buffer;
    uint32_t display_size = m_area_w * m_area_h;
    m_buffer = (uint8_t *) new uint8_t[display_size];

    if (!m_buffer) {
      ERROR("Can't allot memory[%u] for local buffer.", display_size);
      return false;
    }
    memset(m_buffer, 0, display_size);
  } else {
    ERROR("Display size [%ux%u] cannot be zero.", m_area_w, m_area_h);
    return false;
  }

  return true;
}

//////////// AMTestPatternOSD class, use for Picture type OSD  ////////////////////
static uint16_t y_table[256] = { 5, 191, 0, 191, 0, 191, 0, 192, 128, 255, 0,
                                 255, 0, 255, 0, 255, 0, 51, 102, 153, 204, 255,
                                 0, 51, 102, 153, 204, 255, 0, 51, 102, 153,
                                 204, 255, 0, 51, 102, 153, 204, 255, 0, 51,
                                 102, 153, 204, 255, 0, 51, 102, 153, 204, 255,
                                 0, 51, 102, 153, 204, 255, 0, 51, 102, 153,
                                 204, 255, 0, 51, 102, 153, 204, 255, 0, 51,
                                 102, 153, 204, 255, 0, 51, 102, 153, 204, 255,
                                 0, 51, 102, 153, 204, 255, 0, 51, 102, 153,
                                 204, 255, 0, 51, 102, 153, 204, 255, 0, 51,
                                 102, 153, 204, 255, 0, 51, 102, 153, 204, 255,
                                 0, 51, 102, 153, 204, 255, 0, 51, 102, 153,
                                 204, 255, 0, 51, 102, 153, 204, 255, 0, 51,
                                 102, 153, 204, 255, 0, 51, 102, 153, 204, 255,
                                 0, 51, 102, 153, 204, 255, 0, 51, 102, 153,
                                 204, 255, 0, 51, 102, 153, 204, 255, 0, 51,
                                 102, 153, 204, 255, 0, 51, 102, 153, 204, 255,
                                 0, 51, 102, 153, 204, 255, 0, 51, 102, 153,
                                 204, 255, 0, 51, 102, 153, 204, 255, 0, 51,
                                 102, 153, 204, 255, 0, 51, 102, 153, 204, 255,
                                 0, 51, 102, 153, 204, 255, 0, 51, 102, 153,
                                 204, 255, 0, 51, 102, 153, 204, 255, 0, 51,
                                 102, 153, 204, 255, 0, 51, 102, 153, 204, 255,
                                 0, 17, 34, 51, 68, 85, 102, 119, 136, 153, 170,
                                 187, 204, 221, 238, 255, 0, 0, 0, 0, 0, 0, 204,
                                 242, };
static uint16_t u_table[256] = { 4, 0, 191, 191, 0, 0, 191, 192, 128, 0, 255,
                                 255, 0, 0, 255, 255, 0, 0, 0, 0, 0, 0, 51, 51,
                                 51, 51, 51, 51, 102, 102, 102, 102, 102, 102,
                                 153, 153, 153, 153, 153, 153, 204, 204, 204,
                                 204, 204, 204, 255, 255, 255, 255, 255, 255, 0,
                                 0, 0, 0, 0, 0, 51, 51, 51, 51, 51, 51, 102,
                                 102, 102, 102, 102, 102, 153, 153, 153, 153,
                                 153, 153, 204, 204, 204, 204, 204, 204, 255,
                                 255, 255, 255, 255, 255, 0, 0, 0, 0, 0, 0, 51,
                                 51, 51, 51, 51, 51, 102, 102, 102, 102, 102,
                                 102, 153, 153, 153, 153, 153, 153, 204, 204,
                                 204, 204, 204, 204, 255, 255, 255, 255, 255,
                                 255, 0, 0, 0, 0, 0, 0, 51, 51, 51, 51, 51, 51,
                                 102, 102, 102, 102, 102, 102, 153, 153, 153,
                                 153, 153, 153, 204, 204, 204, 204, 204, 204,
                                 255, 255, 255, 255, 255, 255, 0, 0, 0, 0, 0, 0,
                                 51, 51, 51, 51, 51, 51, 102, 102, 102, 102,
                                 102, 102, 153, 153, 153, 153, 153, 153, 204,
                                 204, 204, 204, 204, 204, 255, 255, 255, 255,
                                 255, 255, 0, 0, 0, 0, 0, 0, 51, 51, 51, 51, 51,
                                 51, 102, 102, 102, 102, 102, 102, 153, 153,
                                 153, 153, 153, 153, 204, 204, 204, 204, 204,
                                 204, 255, 255, 255, 255, 255, 255, 0, 17, 34,
                                 51, 68, 85, 102, 119, 136, 153, 170, 187, 204,
                                 221, 238, 255, 0, 0, 0, 0, 0, 0, 0, 102, };
static uint16_t v_table[256] = { 3, 0, 0, 0, 191, 191, 191, 192, 128, 0, 0, 0,
                                 255, 255, 255, 255, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 51, 51, 51,
                                 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51,
                                 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51, 51,
                                 51, 51, 51, 51, 51, 51, 51, 51, 51, 102, 102,
                                 102, 102, 102, 102, 102, 102, 102, 102, 102,
                                 102, 102, 102, 102, 102, 102, 102, 102, 102,
                                 102, 102, 102, 102, 102, 102, 102, 102, 102,
                                 102, 102, 102, 102, 102, 102, 102, 153, 153,
                                 153, 153, 153, 153, 153, 153, 153, 153, 153,
                                 153, 153, 153, 153, 153, 153, 153, 153, 153,
                                 153, 153, 153, 153, 153, 153, 153, 153, 153,
                                 153, 153, 153, 153, 153, 153, 153, 204, 204,
                                 204, 204, 204, 204, 204, 204, 204, 204, 204,
                                 204, 204, 204, 204, 204, 204, 204, 204, 204,
                                 204, 204, 204, 204, 204, 204, 204, 204, 204,
                                 204, 204, 204, 204, 204, 204, 204, 255, 255,
                                 255, 255, 255, 255, 255, 255, 255, 255, 255,
                                 255, 255, 255, 255, 255, 255, 255, 255, 255,
                                 255, 255, 255, 255, 255, 255, 255, 255, 255,
                                 255, 255, 255, 255, 255, 255, 255, 0, 17, 34,
                                 51, 68, 85, 102, 119, 136, 153, 170, 187, 204,
                                 221, 238, 255, 0, 0, 0, 0, 0, 0, 0, 34, };

bool AMTestPatternOSD::create(AM_STREAM_ID stream_id, int32_t area_id,
                              AMOSDArea *osd_area)
{
  if (!osd_area) {
    ERROR("Invalid pointer!");
    return false;
  }
  m_color_num = OSD_CLUT_MAX_NUM;

  if (!(AMOSD::create(stream_id, area_id, osd_area))) {
    return false;
  }
  set_overlay_size();

  return check_overlay();
}

bool AMTestPatternOSD::render(AM_STREAM_ID stream_id, int32_t area_id)
{
  AMOSDArea *osd_area = NULL;
  int32_t osd_obj_id = stream_id * OSD_AREA_MAX_NUM + area_id;
  AMOSDOverlay *osd_handle = AMOSDOverlay::m_osd_instance;

  if (!(osd_area = osd_handle->get_object_area_info(osd_obj_id))) {
    return false;
  }
  fill_pattern_clut(osd_area->clut_addr_offset);

  if (create_pattern_data(osd_area->total_size) && fill_overlay_data()) {
    update_area_info(osd_area);
  } else {
    return false;
  }

  return true;
}

void AMTestPatternOSD::fill_pattern_clut(uint32_t clut_offset)
{
  AMOSDCLUT *clut = m_clut;

  int clut_id = clut_offset / OSD_CLUT_SIZE;
  for (int i = 0; i < OSD_CLUT_MAX_NUM; i ++) {
    clut[i].y = y_table[clut_id * OSD_CLUT_MAX_NUM + i];
    clut[i].u = u_table[clut_id * OSD_CLUT_MAX_NUM + i];
    clut[i].v = v_table[clut_id * OSD_CLUT_MAX_NUM + i];
    clut[i].a = 255;
  }
}

bool AMTestPatternOSD::create_pattern_data(uint32_t area_size)
{
  if (alloc_display_mem()) {
    int n = 8;
    int slice_size = area_size / n;
    for (int i = 0; i < n; i ++)
      memset(m_buffer + i * slice_size, i, slice_size);
    return true;
  }
  return false;
}

//////////// AMPictureOSD class, use for Picture type OSD  ////////////////////
bool AMPictureOSD::create(AM_STREAM_ID stream_id, int32_t area_id,
                          AMOSDArea *osd_area)
{
  if (!osd_area) {
    ERROR("Invalid pointer!");
    return false;
  }

  if (!(AMOSD::create(stream_id, area_id, osd_area))) {
    return false;
  }

  m_pic_filename = m_attribute.osd_pic.filename;
  if (!m_pic_filename.size()) {
    ERROR("No bitmap file!");
    return false;
  }

  if (!(m_fp = fopen(m_pic_filename.c_str(), "r"))) {
    ERROR("failed to open bitmap file [%s].", m_pic_filename.c_str());
    return false;
  }

  if (!init_bitmap_info(m_fp)) {
    fclose(m_fp);
    return false;
  }
  set_overlay_size();

  return check_overlay();
}

bool AMPictureOSD::render(AM_STREAM_ID stream_id, int32_t area_id)
{
  AMOSDArea *osd_area = NULL;
  int32_t osd_obj_id = stream_id * OSD_AREA_MAX_NUM + area_id;
  AMOSDOverlay *osd_handle = AMOSDOverlay::m_osd_instance;

  if (!(osd_area = osd_handle->get_object_area_info(osd_obj_id))) {
    return false;
  }
  fill_bitmap_clut(m_fp);

  if (create_bitmap_data(m_fp) && fill_overlay_data()) {
    update_area_info(osd_area);
  } else {
    fclose(m_fp);
    return false;
  }
  fclose(m_fp);

  return true;
}

bool AMPictureOSD::init_bitmap_info(FILE *fp)
{
  AMOSDBmpFileHeader fileHeader = { 0 };
  AMOSDBmpInfoHeader infoHeader = { 0 };

  fread(&fileHeader.bfType, sizeof(fileHeader.bfType), 1, fp);
  fread(&fileHeader.bfSize, sizeof(fileHeader.bfSize), 1, fp);
  fread(&fileHeader.bfReserved1, sizeof(fileHeader.bfReserved1), 1, fp);
  fread(&fileHeader.bfReserved2, sizeof(fileHeader.bfReserved2), 1, fp);
  fread(&fileHeader.bfOffBits, sizeof(fileHeader.bfOffBits), 1, fp);
  if (fileHeader.bfType != OSD_BMP_MAGIC) {
    ERROR("Invalid type [%d]. Not a bitmap.", fileHeader.bfType);
    return false;
  }

  fread(&infoHeader, sizeof(AMOSDBmpInfoHeader), 1, fp);
  if (infoHeader.biBitCount != OSD_BMP_BIT) {
    ERROR("Invalid [%u]bit. Only support %u bit bitmap.",
          infoHeader.biBitCount,
          OSD_BMP_BIT);
    return false;
  }

  if (infoHeader.biSizeImage != (infoHeader.biWidth * infoHeader.biHeight)) {
    ERROR("Invalid image size [%u]. Not equal to width[%u] x height[%u].",
          infoHeader.biSizeImage,
          infoHeader.biWidth,
          infoHeader.biHeight);
    return false;
  }

  if ((infoHeader.biWidth & 0x1F) || (infoHeader.biHeight & 0x3)) {
    printf("the image size %dx%d, width must be multiple of 32,"
        " height must be multiple of 4.\n",
        infoHeader.biWidth, infoHeader.biHeight);
    return false;
  }

  m_color_num = (fileHeader.bfOffBits - sizeof(AMOSDBmpFileHeader)
      - sizeof(AMOSDBmpInfoHeader)) / sizeof(AMOSDRGB);
  if (m_color_num > OSD_CLUT_SIZE / sizeof(AMOSDCLUT)) {
    WARN("OffsetBits [%u], color number = %u (>[%u]). Reset to max.",
         fileHeader.bfOffBits,
         m_color_num,
         OSD_CLUT_SIZE/sizeof(AMOSDCLUT));
    m_color_num = OSD_CLUT_SIZE / sizeof(AMOSDCLUT);
  }

  m_area_w = infoHeader.biWidth;
  m_area_h = infoHeader.biHeight;
  INFO("Bmp file width = %d, height = %d \n", m_area_w, m_area_h);

  return true;
}

//because two float values do subtraction may occur
//indivisible problem, so we will do a round for it
static uint32_t get_proper_value(float value)
{
  uint32_t tmp = 0;
  if (value - (int32_t)value > 0.5f || (value < 0
      && value - (int32_t)value > -0.5f)) {
    tmp = (uint32_t)value+1;
  } else {
    tmp = (uint32_t)value;
  }

  return tmp;
}

void AMPictureOSD::fill_bitmap_clut(FILE *fp)
{
  if (!fp) {
    ERROR("Invalid file pointer!");
    return;
  }
  AMOSDCLUT *clut = m_clut;
  float tmp;

  AMOSDRGB rgb = { 0 };
  for (uint32_t i = 0; i < m_color_num; ++ i) {
    fread(&rgb, sizeof(AMOSDRGB), 1, fp);
    clut[i].y =
        (uint8_t) (0.257f * rgb.r + 0.504f * rgb.g + 0.098f * rgb.b + 16);

    tmp = 0.439f * rgb.b - 0.291f * rgb.g - 0.148f * rgb.r + 128;
    clut[i].u = (uint8_t)get_proper_value(tmp);

    tmp = 0.439f * rgb.r - 0.368f * rgb.g - 0.071f * rgb.b + 128;
    clut[i].v = (uint8_t)get_proper_value(tmp);

    for (int32_t j=0; j<OSD_COLOR_KEY_MAX_NUM; ++j) {
      AMOSDCLUT ckey = m_attribute.osd_pic.colorkey[j].color;
      uint8_t   crange = m_attribute.osd_pic.colorkey[j].range;
      if ((clut[i].y <= ckey.y + crange && clut[i].y + crange >= ckey.y)&&
          (clut[i].u <= ckey.u + crange && clut[i].u + crange >= ckey.u)&&
          (clut[i].v <= ckey.v + crange && clut[i].v + crange >= ckey.v)) {
        DEBUG("User transparent color(%d,%d,%d)!!!",clut[i].y,clut[i].u,clut[i].v);
        clut[i].a = ckey.a;
        break;
      } else {
        clut[i].a = 255;
      }
    }
  }
}

bool AMPictureOSD::create_bitmap_data(FILE *fp)
{
  if (!fp) {
    ERROR("Invalid file pointer!");
    return false;
  }
  if (alloc_display_mem()) {
    for (uint32_t i = 0; i < m_area_h; ++ i) {
      fread(m_buffer + (m_area_h - 1 - i) * m_area_w,
            m_area_w, 1, fp);
    }
  } else {
    return false;
  }
  return true;
}

//////////// AMTextOSD class, use for generic String type OSD /////////////////
#define DEFAULT_OVERLAY_FONT_SIZE             (24)
#define DEFAULT_OVERLAY_FONT_OUTLINE_WIDTH    (2)

#define OVERLAY_FULL_TRANSPARENT              (0x0)
#define OVERLAY_NONE_TRANSPARENT              (0xff)

#define DEFAULT_OVERLAY_BACKGROUND_Y          (235)
#define DEFAULT_OVERLAY_BACKGROUND_U          (128)
#define DEFAULT_OVERLAY_BACKGROUND_V          (128)
#define DEFAULT_OVERLAY_BACKGROUND_ALPHA      (OVERLAY_FULL_TRANSPARENT)

#define DEFAULT_OVERLAY_OUTLINE_Y             (12)
#define DEFAULT_OVERLAY_OUTLINE_U             (128)
#define DEFAULT_OVERLAY_OUTLINE_V             (128)
#define DEFAULT_OVERLAY_OUTLINE_ALPHA         (OVERLAY_NONE_TRANSPARENT)

#define TEXT_CLUT_ENTRY_BACKGOURND            (0)
#define TEXT_CLUT_ENTRY_OUTLINE               (1)

static AMOSDCLUT color[] = {
  /* alpha 0 is full transparent */
  { 128, 128, 235, 255 }, /* white */
  { 128, 128, 12,  255 }, /* black */
  { 240, 90,  82,  255 }, /* red */
  { 110, 240, 41,  255 }, /* blue */
  { 34,  54,  145, 255 }, /* green */
  { 146, 16,  210, 255 }, /* yellow */
  { 16,  166, 170, 255 }, /* cyan */
  { 222, 202, 107, 255 }, /* magenta */
};

AMTextOSD::AMTextOSD() :
    m_font_width(0),
    m_font_height(0),
    m_font_pitch(0)
{
  memset(&m_font, 0 , sizeof(m_font));
  memset(&m_font_color, 0, sizeof(m_font_color));
  memset(&m_outline_color, 0, sizeof(m_outline_color));
  memset(&m_background_color, 0, sizeof(m_background_color));
}

bool AMTextOSD::create(AM_STREAM_ID stream_id, int32_t area_id,
                       AMOSDArea *osd_area)
{
  if (!osd_area) {
    ERROR("Invalid pointer!");
    return false;
  }
  if (!(AMOSD::create(stream_id, area_id, osd_area)) ||
      !init_text_info(&m_attribute.osd_text_box) ||
      !open_textinsert_lib()) {
    return false;
  }
  set_overlay_size();

  return check_overlay();
}

bool AMTextOSD::render(AM_STREAM_ID stream_id, int32_t area_id)
{
  AMOSDArea *osd_area = NULL;
  int32_t osd_obj_id = stream_id * OSD_AREA_MAX_NUM + area_id;
  AMOSDOverlay *osd_handle = AMOSDOverlay::m_osd_instance;

  if (!(osd_area = osd_handle->get_object_area_info(osd_obj_id))) {
    return false;
  }
  fill_text_clut();

  if ( create_text_data(m_attribute.osd_text_box.osd_string) && fill_overlay_data()) {
    update_area_info(osd_area);
    close_textinsert_lib();
  }

  return true;
}

bool AMTextOSD::find_available_font(char *font)
{
  bool ret = false;
  DIR *dir;
  struct dirent *ptr;
  char fonts_dir[OSD_FILENAME_MAX_NUM] = "/usr/share/fonts/";
  char *font_extension = NULL;
  do {
    if (!font) {
      break;
    }
    if ((dir=opendir(fonts_dir)) == NULL)
    {
      perror("Open fonts dir error...");
      break;
    }
    while ((ptr = readdir(dir)) != NULL)
    {
      if(0 == strcmp(ptr->d_name, ".") || 0 == strcmp(ptr->d_name,".."))
        continue;
      else if((DT_REG == ptr->d_type) &&
          (font_extension = strchr(ptr->d_name, '.')) != NULL)
      {
        if (0 == strcmp(font_extension, ".ttf") || 0 == strcmp(font_extension, ".ttc")) {
          snprintf(font,OSD_FILENAME_MAX_NUM,"%s%s", fonts_dir, ptr->d_name);
          INFO("Using a available font: \"%s\"\n",font);
          ret = true;
          break;
        }
      }
    }
    closedir(dir);
  } while(0);
  if (false == ret) {
    ERROR("No available fonts in fonts dir: %s\n",fonts_dir);
  }
  return ret;
}

bool AMTextOSD::init_text_info(AMOSDTextBox *textbox)
{
  if (textbox) {
    AMOSDFont *font = &(textbox->font);

    m_font.size = (font->size > 0) ? font->size : DEFAULT_OVERLAY_FONT_SIZE;
    m_font.outline_width = (font->outline_width > 0) ? font->outline_width :
        DEFAULT_OVERLAY_FONT_OUTLINE_WIDTH;

    if (strlen(font->ttf_name)) {
      if (access(font->ttf_name, F_OK) != -1) {
        memcpy(m_font.ttf_name, font->ttf_name, strlen(font->ttf_name));
        font->ttf_name[strlen(font->ttf_name)] = '\0';
      } else {
        WARN("Font \"%s\" is not exist!\n", font->ttf_name);
        if (!find_available_font(m_font.ttf_name)) {
          return false;
        }
      }
    } else {
      WARN("No font parameter!\n");
      if (!find_available_font(m_font.ttf_name)) {
        return false;
      }
    }

    m_font.hor_bold = (font->hor_bold > 0) ? font->hor_bold : 0;
    m_font.ver_bold = (font->ver_bold > 0) ? font->ver_bold : 0;
    m_font.italic = (font->italic > 0) ? font->italic : 0;
    m_font.disable_anti_alias = 0;

    m_font_pitch = m_font_width = m_font.size;
    m_font_height = m_font.size * 3/2;
    INFO("font width = %d, height = %d \n", m_font_width, m_font_height);

    uint32_t color_id = textbox->font_color.id ;
    if (color_id >= 0 && color_id < 8 ) {
      memcpy(&m_font_color, &color[color_id], sizeof(AMOSDCLUT));
    } else {
      memcpy(&m_font_color, &textbox->font_color.color, sizeof(AMOSDCLUT));
    }

    if (textbox->background_color) {
      memcpy(&m_background_color, textbox->background_color, sizeof(AMOSDCLUT));
    } else {
      m_background_color.y = DEFAULT_OVERLAY_BACKGROUND_Y;
      m_background_color.u = DEFAULT_OVERLAY_BACKGROUND_U;
      m_background_color.v = DEFAULT_OVERLAY_BACKGROUND_V;
      m_background_color.a = DEFAULT_OVERLAY_BACKGROUND_ALPHA;
      WARN("Use default backgroud color!!! \n");
    }

    if (m_font.outline_width) {
      if (textbox->outline_color) {
        memcpy(&m_outline_color, textbox->outline_color, sizeof(AMOSDCLUT));
      } else {
        m_outline_color.y = DEFAULT_OVERLAY_OUTLINE_Y;
        m_outline_color.u = DEFAULT_OVERLAY_OUTLINE_U;
        m_outline_color.v = DEFAULT_OVERLAY_OUTLINE_V;
        m_outline_color.a = DEFAULT_OVERLAY_OUTLINE_ALPHA;
        WARN("Use default outline color!!! \n");
      }
    }
    m_color_num = OSD_CLUT_MAX_NUM;
  } else {
    ERROR("Text Box is NULL.");
    return false;
  }

  return true;
}

bool AMTextOSD::close_textinsert_lib()
{
  return (text2bitmap_lib_deinit() >= 0);
}

bool AMTextOSD::open_textinsert_lib()
{
  if (text2bitmap_lib_init(NULL) < 0) {
    ERROR("Failed to init text insert library.");
    return false;
  }

  font_attribute_t font_attr;
  memset(&font_attr, 0, sizeof(font_attribute_t));
  font_attr.size = m_font.size;
  font_attr.outline_width = m_font.outline_width;
  font_attr.hori_bold = m_font.hor_bold;
  font_attr.vert_bold = m_font.ver_bold;
  font_attr.italic = m_font.italic * 50;
  snprintf(font_attr.type, sizeof(font_attr.type),
           "%s", m_font.ttf_name);
  INFO("font_attr.type = %s \n", font_attr.type);

  if (text2bitmap_set_font_attribute(&font_attr) < 0) {
    ERROR("Failed to set up font attribute in text insert library.");
    close_textinsert_lib();
    return false;
  }

  return true;
}

void AMTextOSD::fill_text_clut()
{
  AMOSDCLUT *clut = m_clut;

  for (uint32_t i = 0; i < (OSD_CLUT_SIZE / sizeof(AMOSDCLUT)); ++ i) {
    clut[i].y = m_font_color.y;
    clut[i].u = m_font_color.u;
    clut[i].v = m_font_color.v;
    clut[i].a = ((i < m_font_color.a) ? i : m_font_color.a);
  }
  // Fill Background color
  memcpy(&clut[TEXT_CLUT_ENTRY_BACKGOURND],
         &m_background_color,
         sizeof(AMOSDCLUT));
  // Fill Outline color
  if (m_font.outline_width) {
    memcpy(&clut[TEXT_CLUT_ENTRY_OUTLINE], &m_outline_color, sizeof(AMOSDCLUT));
  }
  DEBUG("Fill text clut is done.");
}

bool AMTextOSD::char_to_wchar(wchar_t *wide_str, const char *str,
                              uint32_t max_len)
{
  setlocale(LC_ALL, "");
  if (mbstowcs(wide_str, str, max_len) < 0) {
    ERROR("Failed to convert char to wchar");
    return false;
  }
  return true;
}

bool AMTextOSD::create_text_data(char *str)
{
  if (!strlen(str)) {
    snprintf(str, OSD_FILENAME_MAX_NUM, "Hello, Ambarella!");
  }

  if (alloc_display_mem()) {
    uint32_t len = strlen(str);
    INFO("string is [%s] \n", str);
    INFO("len = %d \n",len);

    wchar_t *wide_str = (wchar_t *) new wchar_t[len];

    if (char_to_wchar(wide_str, str, len)) {
      uint32_t offset_x = 0, offset_y = 0;
      bitmap_info_t bmp_info = { 0 };
      uint8_t *line_head = m_buffer;
      for (uint32_t i = 0; i < len; ++ i) {
        if ((offset_x + m_font_pitch) > m_area_w) {
          // Add a new line
          INFO("%d:m_font_pitch = %d, m_area_w = %d \n",i, m_font_pitch, m_area_w);
          DEBUG("Add a new line.");
          offset_y += m_font_height;
          offset_x = 0;
          line_head += m_area_w * m_font_height;
        }
        if ((m_font_height + offset_y) > m_area_h) {
          // No more new line
          DEBUG("No more space for a new line. %u + %u > %u",
                m_font_height, offset_y, m_area_h);
          break;
        }
        // Neither a digit nor a converted letter in time string
        if (text2bitmap_convert_character(wide_str[i], line_head,
                                          m_font_height, m_area_w, offset_x, &bmp_info) < 0) {
          ERROR("text2bitmap library： Failed to convert the charactor "
              "[%c].",
              str[i]);
          return false;
        }
        offset_x += bmp_info.width;
      }
    }
    delete[] wide_str;
  } else {
    return false;
  }

  return true;
}

//////////// AMTimeTextOSD class, use for time String type OSD ////////////////
static const char weekStr[][4] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri",
                                   "Sat" };
AMTimeTextOSD::AMTimeTextOSD() :
    m_time_length(0)
{
  memset(m_offsetx, 0, sizeof(m_offsetx));
  memset(m_offsety, 0, sizeof(m_offsety));
  memset(m_time_string, 0, sizeof(m_time_string));
  for (int i = 0; i < OSD_DIGIT_NUM; ++ i) {
    m_digit[i] = NULL;
  }
  for (int i = 0; i < OSD_LETTER_NUM; ++ i) {
    m_upper_letter[i] = NULL;
    m_lower_letter[i] = NULL;
  }
}

AMTimeTextOSD::~AMTimeTextOSD()
{
  for (int i = 0; i < OSD_DIGIT_NUM; ++ i) {
    delete[] m_digit[i];
    m_digit[i] = NULL;
  }
  for (int i = 0; i < OSD_LETTER_NUM; ++ i) {
    delete[] m_upper_letter[i];
    delete[] m_lower_letter[i];
    m_upper_letter[i] = NULL;
    m_lower_letter[i] = NULL;
  }
}

bool AMTimeTextOSD::create(AM_STREAM_ID stream_id, int32_t area_id,
                           AMOSDArea *osd_area)
{
  if (!osd_area) {
    ERROR("Invalid pointer!");
    return false;
  }
  return AMTextOSD::create(stream_id, area_id, osd_area);
}

bool AMTimeTextOSD::render(AM_STREAM_ID stream_id, int32_t area_id)
{
  AMOSDArea *osd_area = NULL;
  int32_t osd_obj_id = stream_id * OSD_AREA_MAX_NUM + area_id;
  AMOSDOverlay *osd_handle = AMOSDOverlay::m_osd_instance;

  if (!(osd_area = osd_handle->get_object_area_info(osd_obj_id))) {
    return false;
  }
  fill_text_clut();

  char time_str[OSD_FILENAME_MAX_NUM];
  get_time_string(time_str);
  if (prepare_time_data() && create_time_data(time_str)
      && fill_overlay_data()) {
    update_area_info(osd_area);
    close_textinsert_lib();
  } else {
    return false;
  }

  return true;
}

void AMTimeTextOSD::update() //create and allocate memory
{
  char time_str[OSD_FILENAME_MAX_NUM];
  uint32_t n = 0;
  uint8_t *src = NULL;

  get_time_string(time_str);
  for (uint32_t i = 0; i < m_time_length; ++ i) {
    if (time_str[i] != m_time_string[i]) {
      if (is_digit(time_str[i])) {
        n = time_str[i] - '0';
        src = (m_digit[n] ? m_digit[n] : NULL);
      } else if (is_lower_letter(time_str[i])) {
        n = time_str[i] - 'a';
        src = (m_lower_letter[n] ? m_lower_letter[n] : NULL);
      } else if (is_upper_letter(time_str[i])) {
        n = time_str[i] - 'A';
        src = (m_upper_letter[n] ? m_upper_letter[n] : NULL);
      } else {
        src = NULL;
      }
      if (src) {
        rotate_fill(m_data, m_pitch, m_width, m_height, m_offsetx[i], m_offsety[i],
                    src, m_font_pitch, m_font_height, 0, 0, m_font_width, m_font_height);
      } else {
        ERROR("Unknown character [%c] in time string [%s].",
              time_str[i], time_str);
      }
    }
  }
  memcpy(m_time_string, time_str, strlen(time_str));
}

void AMTimeTextOSD::get_time_string(char *time_str)
{
  tm *p = NULL;
  time_t t = time(NULL);
  p = gmtime(&t);

  snprintf(time_str, OSD_FILENAME_MAX_NUM, "%04d-%02d-%02d %s %02d:%02d:%02d",
           (1900 + p->tm_year), (1 + p->tm_mon), p->tm_mday,
           weekStr[p->tm_wday], p->tm_hour, p->tm_min, p->tm_sec);

  DEBUG("current time is %s \n", time_str);
}

bool AMTimeTextOSD::prepare_time_data()
{
  if (!m_font_pitch || !m_font_height) {
    ERROR("Font pitch [%u] or height [%u] is zero.",
          m_font_pitch, m_font_height);
    return false;
  }

  m_font_width = 0;
  bitmap_info_t bmp_info = { 0 };
  uint32_t digit_size = m_font_pitch * m_font_height;
  for (uint32_t i = 0; i < OSD_DIGIT_NUM; ++ i) {
    delete[] m_digit[i];
    m_digit[i] = (uint8_t *)new uint8_t[digit_size];
    if (!m_digit[i]) {
      ERROR("Cannot allot memory for digit %u (size %ux%u).",
            i, m_font_pitch, m_font_height);
      return false;
    }
    memset(m_digit[i], 0, digit_size);
    if (text2bitmap_convert_character(i + L'0', m_digit[i],
                                      m_font_height, m_font_pitch, 0, &bmp_info) < 0) {
      ERROR("Failed to convert digit %u in text insert library.", i);
      return false;
    }
    if (bmp_info.width > (int32_t) m_font_width) {
      m_font_width = (uint32_t) bmp_info.width;
    }
  }

  uint32_t week_size = sizeof(weekStr[0]);
  uint32_t week_num = sizeof(weekStr) / week_size;
  uint32_t n = 0, letter_size = m_font_pitch * m_font_height;
  uint32_t is_lower = false;
  wchar_t wc = 0;
  for (uint32_t i = 0; i < week_num; ++ i) {
    for (uint32_t j = 0; j < week_size - 1; ++ j) {
      if (is_lower_letter(weekStr[i][j])) {
        n = weekStr[i][j] - 'a';
        if (m_lower_letter[n])
          continue;
        wc = n + L'a';
        is_lower = true;
      } else if (is_upper_letter(weekStr[i][j])) {
        n = weekStr[i][j] - 'A';
        if (m_upper_letter[n])
          continue;
        wc = n + L'A';
        is_lower = false;
      } else {
        ERROR("[%c] in weekStr [%s] is not a letter",
              weekStr[i][j], weekStr[i]);
        return false;
      }

      uint8_t *letter_addr = (uint8_t *)new uint8_t[letter_size];
      if (!letter_addr) {
        ERROR("Cannot allot memory for letter %c (size %ux%u).",
              weekStr[i][j], m_font_pitch, m_font_height);
        return false;
      }
      memset(letter_addr, 0, letter_size);
      if (is_lower) {
        delete[] m_lower_letter[n];
        m_lower_letter[n] = letter_addr;
      } else {
        delete[] m_upper_letter[n];
        m_upper_letter[n] = letter_addr;
      }
      if (text2bitmap_convert_character(wc, letter_addr,
                 m_font_height, m_font_pitch, 0, &bmp_info) < 0) {
        ERROR("Failed to convert letter %c in text insert library.",
              weekStr[i][j]);
        return false;
      }
      if (bmp_info.width > (int32_t) m_font_width) {
        m_font_width = (uint32_t) bmp_info.width;
      }
    }
  }
  DEBUG("Font pitch %u, height %u, width %u.",
        m_font_pitch, m_font_height, m_font_width);

  return true;
}

bool AMTimeTextOSD::create_time_data(char *str)
{
  uint32_t offset_x = 0, offset_y = 0, display_len = 0;
  bitmap_info_t bmp_info = { 0 };

  if (alloc_display_mem()) {
    uint32_t len = strlen(str);
    if (len > OSD_FILENAME_MAX_NUM) {
      INFO("The length [%d] of time string exceeds the max length [%d]."
          " Display %d at most.", len, OSD_FILENAME_MAX_NUM, OSD_FILENAME_MAX_NUM);
      len = OSD_FILENAME_MAX_NUM;
    }
    DEBUG("string is [%s] \n", str);

    wchar_t *wide_str = new wchar_t[len];
    if (char_to_wchar(wide_str, str, len)) {
      uint8_t *line_head = m_buffer;
      for (uint32_t i = 0; i < len; ++ i) {
        if ((offset_x + m_font_pitch) > (uint32_t)m_area_w) {
          // Add a new line
          DEBUG("Add a new line.");
          offset_y += m_font_height;
          offset_x = 0;
          line_head += m_area_w * m_font_height;
        }
        if ((m_font_height + offset_y) > (uint32_t) m_area_h) {
          // No more new line
          DEBUG("No more space for a new line. %u + %u > %u",
                m_font_height, offset_y, m_area_h);
          break;
        }
        // Remember the charactor's offset in the overlay data memory
        m_offsetx[i] = offset_x;
        m_offsety[i] = offset_y;

        // Digit and letter in time string
        if ((is_digit(str[i]) || is_lower_letter(str[i])
            || is_upper_letter(str[i]))) {
          uint8_t *dst = line_head + offset_x;
          uint8_t *src = NULL;
          if (is_digit(str[i])) {
            src = m_digit[str[i] - '0'];
          } else if (is_lower_letter(str[i])) {
            src = m_lower_letter[str[i] - 'a'];
          } else if (is_upper_letter(str[i])) {
            src = m_upper_letter[str[i] - 'A'];
          }
          if (src) {
            for (uint32_t row = 0; row < m_font_height; ++ row) {
              memcpy(dst, src, m_font_width);
              dst += m_area_w;
              src += m_font_pitch;
            }
            offset_x += m_font_width;
          }
        } else {
          // Neither a digit nor a converted letter in time string
          if (text2bitmap_convert_character(wide_str[i], line_head,
                                            m_font_height, m_area_w, offset_x, &bmp_info) < 0) {
            ERROR("text2bitmap library： Failed to convert the charactor "
                "[%c].", str[i]);
            return false;
          }
          offset_x += bmp_info.width;
        }
        display_len ++;
      }
      m_time_length = display_len;
      memcpy(m_time_string, str, strlen(str));
      DEBUG("Time display length %u, time string: %s \n",
            m_time_length, m_time_string);
    }
    delete[] wide_str;
  } else {
    return false;
  }

  return true;
}
