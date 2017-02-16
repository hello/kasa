/*******************************************************************************
 * am_jpeg_snapshot.cpp
 *
 * History:
 *   2015-07-01 - [Zhifeng Gong] created file
 *
 * Copyright (C) 2014-2015, Ambarella Co,Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <iav_ioctl.h>
#include <sys/time.h>
#include <signal.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "am_base_include.h"
#include "am_log.h"
#include "am_define.h"
#include "am_file.h"
#include "am_configure.h"
#include "am_event_monitor_if.h"
#include "am_video_reader_if.h"
#include "am_jpeg_snapshot.h"

static std::recursive_mutex m_mtx;
#define  DECLARE_MUTEX  std::lock_guard<std::recursive_mutex> lck (m_mtx);

#define ORYX_VIDEO_CONF_DIR    ((const char *)"/etc/oryx/utility/")
#define JPEG_ENCODE_CONFIG     ((const char *)"jpeg_snapshot.acs")
#define MD_JPEG_PATH           ((const char *)"/tmp/md_jpeg")


AMJpegSnapshot *AMJpegSnapshot::m_instance = NULL;

AMJpegSnapshot::AMJpegSnapshot():
  m_run(false),
  m_file_table(NULL),
  m_file_index(0),
  m_motion_state(AM_MD_MOTION_END),
  m_jpeg_encode_param(NULL),
  m_jpeg_encoder(NULL),
  m_thread(NULL),
  m_mutex(NULL),
  m_cond(NULL),
  m_config(NULL),
  m_conf_path(ORYX_VIDEO_CONF_DIR)
{
}

AMJpegSnapshot::~AMJpegSnapshot()
{
  destroy();
  delete m_config;
  delete m_jpeg_encode_param;

  AM_DESTROY(m_mutex);
  AM_DESTROY(m_cond);
  AM_DESTROY(m_thread);
}

AMIJpegSnapshotPtr AMIJpegSnapshot::get_instance()
{
  return AMJpegSnapshot::get_instance();
}

AMJpegSnapshot *AMJpegSnapshot::get_instance()
{
  return create();
}

void AMJpegSnapshot::inc_ref()
{
  ++ m_ref_counter;
}

void AMJpegSnapshot::release()
{
  DECLARE_MUTEX;
  DEBUG("AMJpegSnapshot release is called!");
  if ((m_ref_counter >= 0) && (--m_ref_counter <= 0)) {
    DEBUG("This is the last reference of AMVideoReader's object, "
          "delete object instance %p", m_instance);
    delete m_instance;
    m_instance = NULL;
  }
}

AMJpegSnapshot *AMJpegSnapshot::create()
{
  DECLARE_MUTEX;
  if (m_instance) {
    return m_instance;
  }
  m_instance = new AMJpegSnapshot();
  if (m_instance && m_instance->init()) {
    delete m_instance;
    m_instance = NULL;
  }
  return m_instance;
}

void AMJpegSnapshot::destroy()
{
  if (m_instance) {
    stop();
    deinit_dir();
    delete m_instance;
    m_instance = NULL;
  }
}

bool AMJpegSnapshot::init_dir()
{
  bool res = true;
  struct file_table *ft = NULL;
  do {
    if (false == AMFile::create_path(m_config->get_jpeg_path())) {
      ERROR("AMFile::create_path failed!\n");
      res = false;
      break;
    }
    m_file_table = (struct file_table *)calloc(1, m_config->get_max_files() * sizeof(struct file_table));
    if (!m_file_table) {
      ERROR("malloc file_table failed!\n");
      res = false;
      break;
    }
    for (int i = 0; i < m_config->get_max_files(); ++i) {
      ft = m_file_table + i;
      ft->id = i;
    }
  } while (0);
  return res;
}

void AMJpegSnapshot::deinit_dir()
{
  if (m_file_table) {
    free(m_file_table);
  }
}

int AMJpegSnapshot::get_file_name(char *str, int len)
{
  char date_fmt[20];
  char date_ms[4];
  struct timeval tv;
  struct tm now_tm;
  int now_ms;
  time_t now_sec;
  int res = AM_RESULT_OK;
  do {
    if (str == NULL || len <= 0) {
      ERROR("invalid paraments!\n");
      res = AM_RESULT_ERR_INVALID;
      break;
    }
    gettimeofday(&tv, NULL);
    now_sec = tv.tv_sec;
    now_ms = tv.tv_usec/1000;
    localtime_r(&now_sec, &now_tm);
    strftime(date_fmt, 20, "%Y%m%d_%H%M%S", &now_tm);
    snprintf(date_ms, sizeof(date_ms), "%03d", now_ms);
    snprintf(str, len, "%s/%s_%s.jpeg", m_config->get_jpeg_path(), date_fmt, date_ms);
  } while (0);
  return res;
}

int AMJpegSnapshot::trim_jpeg_files()
{
  struct file_table *ft = NULL;
  int res = AM_RESULT_OK;
  do {
    ++m_file_index;
    if (m_file_index >= m_config->get_max_files()) {
      m_file_index = 0;
    }
    ft = m_file_table + m_file_index;
    if (ft->used) {
      if (-1 == remove(ft->name)) {
        ERROR("remove %s failed: %d\n", ft->name, errno);
        res = AM_RESULT_ERR_INVALID;
        break;
      }
    }
    ft->used = 1;
    get_file_name(ft->name, MAX_FILENAME_LEN);
  } while (0);
  return res;
}

int AMJpegSnapshot::save_jpeg_in_file(char *filename, void *data, size_t size)
{
  int res = AM_RESULT_OK;
  AMFile f(filename);
  do {
    if (!f.open(AMFile::AM_FILE_CREATE)) {
      PERROR("Failed to open file");
      res = AM_RESULT_ERR_INVALID;
      break;
    }
    if (f.write_reliable(data, size) < 0) {
      PERROR("Failed to sava data into file\n");
      res = AM_RESULT_ERR_INVALID;
      break;
    }
  } while (0);
  f.close();
  return res;
}

AM_RESULT AMJpegSnapshot::init()
{
  AM_RESULT res = AM_RESULT_OK;

  do {
    m_config = new AMJpegSnapshotConfig();
    if (!m_config) {
      ERROR("AMJpegSnapshotConfig:: new m_config error\n");
      res = AM_RESULT_ERR_INVALID;
      break;
    }
    m_conf_path.append("/").append(JPEG_ENCODE_CONFIG);
    m_jpeg_encode_param = m_config->get_config(m_conf_path);
    if (!m_jpeg_encode_param) {
      ERROR("AMJpegSnapshotConfig:: get config failed!\n");
      res = AM_RESULT_ERR_INVALID;
      break;
    }
    if (!is_enable()) {
      INFO("AMJpegSnapshotConfig:: md_enable is false!\n");
      res = AM_RESULT_OK;
      break;
    }
    PRINTF("motion detection enabled\n");
    m_jpeg_encoder = AMIJpegEncoder::get_instance();
    if (!m_jpeg_encoder) {
      res = AM_RESULT_ERR_INVALID;
      ERROR("Failed to create AMJpegEncode!");
      break;
    }
    m_jpeg_encoder->create();

    if (false == init_dir()) {
      res = AM_RESULT_ERR_INVALID;
      ERROR("init dir failed!\n");
      break;
    }

    if (!(m_cond = AMCondition::create())) {
      res = AM_RESULT_ERR_INVALID;
      ERROR("Failed to create AMCondition!");
      break;
    }

    if (!(m_mutex = AMMutex::create())) {
      res = AM_RESULT_ERR_INVALID;
      ERROR("Failed to create AMMutex!");
      break;
    }

  } while (0);

  return res;
}

AM_RESULT AMJpegSnapshot::start()
{
  AM_RESULT res = AM_RESULT_OK;

  do {
    if (!is_enable()) {
      INFO("AMJpegSnapshotConfig:: md_enable is false!\n");
      res = AM_RESULT_OK;
      break;
    }
    if (m_run) {
      INFO("AMJpegSnapshot:: is already started\n");
      break;
    }
    m_video_reader = AMIVideoReader::get_instance();
    if (!m_video_reader) {
      res = AM_RESULT_ERR_INVALID;
      break;
    }

    if (m_video_reader->init() != AM_RESULT_OK) {
      ERROR("unable to init AMVideoReader\n");
      res = AM_RESULT_ERR_INVALID;
      break;
    }

    m_run = true;
    if (!(m_thread = AMThread::create("JpegEncodeThread", static_jpeg_encode_thread, this))) {
      ERROR("Failed to create AMThread!");
      res = AM_RESULT_ERR_INVALID;
      break;
    }
  } while (0);
  if (res != AM_RESULT_OK) {
    ERROR("AMJpegSnapshot::start failed!");
  }
  return res;
}

AM_RESULT AMJpegSnapshot::stop()
{
  AM_RESULT res = AM_RESULT_OK;
  do {
    if (!is_enable()) {
      INFO("AMJpegSnapshotConfig:: md_enable is false!\n");
      res = AM_RESULT_OK;
      break;
    }
    if (!m_run) {
      INFO("AMJpegSnapshot:: is already stopped!\n");
      break;
    }
    m_run = false;
    m_mutex->lock();
    m_cond->signal();
    m_mutex->unlock();
    AM_DESTROY(m_thread);
    m_jpeg_encoder->destroy();
  } while (0);

  return res;
}

bool AMJpegSnapshot::is_enable()
{
  return m_jpeg_encode_param->md_enable;
}

AMYUVData *AMJpegSnapshot::capture_yuv_buffer()
{
  AMQueryDataFrameDesc frame_desc;
  AMMemMapInfo dsp_mem;
  AMYUVData *yuv = NULL;
  AM_RESULT result = AM_RESULT_OK;
  uint8_t *y_offset, *uv_offset, *y_tmp;
  int32_t uv_height;
  int32_t i;
  int buf_id = m_jpeg_encode_param->source_buffer;

  do {
    yuv = (AMYUVData *)calloc(1, sizeof(AMYUVData));
    if (!yuv) {
      ERROR("malloc yuv failed!\n");
      result = AM_RESULT_ERR_INVALID;
      break;
    }

    result = m_video_reader->get_dsp_mem(&dsp_mem);
    if (result != AM_RESULT_OK) {
      ERROR("get dsp mem failed \n");
      result = AM_RESULT_ERR_INVALID;
      break;
    }

    result = m_video_reader->query_yuv_frame(&frame_desc,
                                AM_ENCODE_SOURCE_BUFFER_ID(buf_id), false);
    if (result != AM_RESULT_OK) {
      ERROR("query yuv frame failed \n");
      result = AM_RESULT_ERR_INVALID;
      break;
    }

    y_offset = dsp_mem.addr + frame_desc.yuv.y_addr_offset;
    uv_offset = dsp_mem.addr + frame_desc.yuv.uv_addr_offset;
    yuv->width = frame_desc.yuv.width;
    yuv->height = frame_desc.yuv.height;
    yuv->pitch = frame_desc.yuv.pitch;
    yuv->format = frame_desc.yuv.format;

    if (frame_desc.yuv.format == AM_ENCODE_CHROMA_FORMAT_YUV420) {
      uv_height = yuv->height / 2;
    } else if (frame_desc.yuv.format == AM_ENCODE_CHROMA_FORMAT_YUV422) {
      uv_height = yuv->height;
    } else {
      ERROR("not supported chroma format in YUV dump\n");
      result = AM_RESULT_ERR_INVALID;
      break;
    }

    //copy y buffer
    yuv->y.iov_len = yuv->width * yuv->height;
    yuv->y.iov_base = (uint8_t *)calloc(1, yuv->y.iov_len);
    y_tmp = (uint8_t *)yuv->y.iov_base;
    if (yuv->pitch == yuv->width) {
      memcpy(yuv->y.iov_base, y_offset, yuv->y.iov_len);
    } else {
      for (i = 0; i < yuv->height ; i++) {
        memcpy(y_tmp, y_offset + i * yuv->pitch, yuv->width);
        y_tmp += yuv->width;
      }
    }

    //copy uv buffer
    yuv->uv.iov_len = yuv->pitch * uv_height;//XXX: pitch
    yuv->uv.iov_base = (uint8_t *)calloc(1, yuv->uv.iov_len);
    memcpy(yuv->uv.iov_base, uv_offset, yuv->uv.iov_len);

  } while (0);

  if (result != AM_RESULT_OK) {
    free_yuv_buffer(yuv);
    yuv = NULL;
  }

  return yuv;
}

void AMJpegSnapshot::static_jpeg_encode_thread(void *arg)
{
  AMJpegSnapshot *jpeg_encoder = (AMJpegSnapshot*)arg;
  int ms_encode = 0;
  int ms_sleep = 0;
  struct timeval delay;
  struct timeval pre = {0, 0}, curr = {0, 0};

  while (jpeg_encoder->m_run) {
    if (jpeg_encoder->m_motion_state == AM_MD_MOTION_END) {
      jpeg_encoder->m_mutex->lock();
      INFO("wait motion detect");
      jpeg_encoder->m_cond->wait(jpeg_encoder->m_mutex);
      jpeg_encoder->m_mutex->unlock();
      INFO("motion detect occur");
      continue;
    } else {
      gettimeofday(&curr, NULL);
      pre = curr;
      jpeg_encoder->encode_yuv_to_jpeg();
      gettimeofday(&curr, NULL);
      ms_encode = ((1000000 * (curr.tv_sec - pre.tv_sec)) + (curr.tv_usec - pre.tv_usec)) / 1000;
      INFO("encode_yuv_to_jpeg take [%06ld ms]", ms_encode);
      ms_sleep = (1.0/jpeg_encoder->m_jpeg_encode_param->fps)*1000 - ms_encode;
      ms_sleep = (ms_sleep < 0) ? 0 : ms_sleep;
      delay.tv_sec = 0;
      delay.tv_usec = ms_sleep * 1000;
      select(0, NULL, NULL, NULL, &delay);
    }
  }
}

void AMJpegSnapshot::update_motion_state(AM_EVENT_MESSAGE *msg)
{
  AM_MOTION_TYPE type = msg->md_msg.mt_type;
  m_motion_state = type;
  switch (type) {
  case AM_MD_MOTION_START:
    INFO("AM_MD_MOTION_START\n");
    m_mutex->lock();
    m_cond->signal();
    m_mutex->unlock();
    break;
  case AM_MD_MOTION_LEVEL_CHANGED:
    INFO("AM_MD_MOTION_LEVEL_CHANGED\n");
    m_mutex->lock();
    m_cond->signal();
    m_mutex->unlock();
    break;
  case AM_MD_MOTION_END:
    INFO("AM_MD_MOTION_END\n");
    break;
  default:
    INFO("AM_MD_MOTION_TYPE=%d\n", type);
    break;
  }
}

void AMJpegSnapshot::free_yuv_buffer(AMYUVData *yuv)
{
  if (!yuv) {
    return;
  }
  free(yuv->y.iov_base);
  free(yuv->uv.iov_base);
  free(yuv);
}

int AMJpegSnapshot::encode_yuv_to_jpeg()
{
  AM_RESULT res = AM_RESULT_OK;
  int ret = -1;
  char filename_jpeg[MAX_FILENAME_LEN];
  struct file_table *ft = NULL;
  AMYUVData *yuv = NULL;
  AMJpegData *jpeg = NULL;

  do {
    yuv = capture_yuv_buffer();
    if (!yuv) {
      ERROR("capture_yuv_buffer failed!\n");
      res = AM_RESULT_ERR_INVALID;
      break;
    }

    jpeg = m_jpeg_encoder->new_jpeg_data(yuv->width, yuv->height);
    if (!jpeg) {
      ERROR("new_jpeg_data failed!\n");
      res = AM_RESULT_ERR_INVALID;
      break;
    }
    ret = m_jpeg_encoder->encode_yuv(yuv, jpeg);
    if (ret != 0) {
      ERROR("jpeg encode failed!\n");
      res = AM_RESULT_ERR_INVALID;
      break;
    }
    if (AM_RESULT_OK != trim_jpeg_files()) {
      ERROR("trim_jpeg_files failed!\n");
      res = AM_RESULT_ERR_INVALID;
      break;
    }
    if (AM_RESULT_OK != get_file_name(filename_jpeg, MAX_FILENAME_LEN)) {
      ERROR("get_file_name failed!\n");
      res = AM_RESULT_ERR_INVALID;
      break;
    }
    ft = m_file_table + m_file_index;
    save_jpeg_in_file(ft->name, jpeg->data.iov_base, jpeg->data.iov_len);
    INFO("Save JPEG file [%s] OK.\n", filename_jpeg);

    m_jpeg_encoder->free_jpeg_data(jpeg);
  } while (0);

  free_yuv_buffer(yuv);

  return res;
}

AMJpegSnapshotConfig::AMJpegSnapshotConfig() :
    m_param(NULL)
{
}

AMJpegSnapshotConfig::~AMJpegSnapshotConfig()
{
  delete m_param;
}

JpegSnapshotParam *AMJpegSnapshotConfig::get_config(const std::string &cfg_file_path)
{
  AMConfig *pconf = NULL;
  do {
    pconf = AMConfig::create(cfg_file_path.c_str());
    if (!pconf) {
      ERROR("AMJpegSnapshotConfig:: Create AMConfig failed!\n");
      break;
    }

    if (!m_param) {
      m_param = new JpegSnapshotParam();
      if (!m_param ) {
        ERROR("AMJpegSnapshotConfig::new jpeg encode config failed!\n");
        break;
      }
    }

    std::string tmp;
    AMConfig &conf = *pconf;
    if (conf["md_enable"].exists()) {
      m_param->md_enable = conf["md_enable"].get<bool>(false);
    } else {
      m_param->md_enable = false;
    }

    if (conf["source_buffer"].exists()) {
      m_param->source_buffer = conf["source_buffer"].get<int>(0);
    } else {
      m_param->source_buffer = 0;
    }

    if (conf["fps"].exists()) {
      m_param->fps = conf["fps"].get<float>(1.0);
    } else {
      m_param->fps = 1.0;
    }

    if (conf["jpeg_path"].exists()) {
      tmp = conf["jpeg_path"].get<std::string>(MD_JPEG_PATH);
      strncpy(m_param->jpeg_path, tmp.c_str(), tmp.length());
    } else {
      strncpy(m_param->jpeg_path, MD_JPEG_PATH, MAX_FILENAME_LEN);
    }

    if (conf["max_files"].exists()) {
      m_param->max_files = conf["max_files"].get<int>(10);
    } else {
      m_param->max_files = 10;
    }
  } while (0);
  if (pconf) {
    pconf->destroy();
  }
  return m_param;
}

bool AMJpegSnapshotConfig::set_config(JpegSnapshotParam *param, const std::string &cfg_file_path)
{
  bool res = true;
  std::string tmp;
  AMConfig *pconf = NULL;

  do {
    pconf = AMConfig::create(cfg_file_path.c_str());
    if (!pconf) {
      ERROR("AMJpegSnapshotConfig::Create AMConfig failed!\n");
      break;
    }
    if (!m_param) {
      m_param = new JpegSnapshotParam();
      if (!m_param) {
        ERROR("AMJpegSnapshotConfig:: new jpeg encode config failed!\n");
        break;
      }
    }
    memcpy(m_param, param, sizeof(JpegSnapshotParam));
    AMConfig &conf = *pconf;
    conf["md_enable"] = m_param->md_enable;
    conf["source_buffer"] = m_param->source_buffer;
    conf["fps"] = m_param->fps;
    tmp = m_param->jpeg_path;
    conf["jpeg_path"] = tmp;
    conf["max_files"] = m_param->max_files;
    if (!conf.save()) {
      ERROR("AMJpegSnapshotConfig: failed to save_config\n");
      res = false;
      break;
    }
  } while (0);
  if (pconf) {
    pconf->destroy();
  }
  return res;
}

char *AMJpegSnapshotConfig::get_jpeg_path()
{
  return m_param->jpeg_path;
}

int AMJpegSnapshotConfig::get_max_files()
{
  return m_param->max_files;
}
