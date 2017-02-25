/*******************************************************************************
 * am_jpeg_snapshot.cpp
 *
 * History:
 *   2015-07-01 - [Zhifeng Gong] created file
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
#include "am_file.h"
#include "am_configure.h"
#include "am_video_reader_if.h"
#include "am_jpeg_snapshot.h"

#include <iostream>
#include <list>
#include <sys/time.h>

static std::recursive_mutex m_mtx;
#define  DECLARE_MUTEX  std::lock_guard<std::recursive_mutex> lck (m_mtx);

#define MAX_CACHE_BUF          10
#define USR_DRAM_Y_OFFSET      0
#define USR_DRAM_UV_OFFSET     (2*1024*1024)

AMJpegSnapshot *AMJpegSnapshot::m_instance = nullptr;

AMJpegSnapshot::AMJpegSnapshot()
{
}

AMJpegSnapshot::~AMJpegSnapshot()
{
  destroy();
  m_instance = nullptr;
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
  if ((m_ref_counter > 0) && (--m_ref_counter == 0)) {
    DEBUG("This is the last reference of AMVideoReader's object, "
          "delete object instance %p", m_instance);
    delete m_instance;
    m_instance = nullptr;
  }
}

AMJpegSnapshot *AMJpegSnapshot::create()
{
  DECLARE_MUTEX;
  if (!m_instance) {
    m_instance = new AMJpegSnapshot();
    if (m_instance && (AM_RESULT_OK != m_instance->init())) {
      delete m_instance;
      m_instance = nullptr;
    }
  }
  return m_instance;
}

void AMJpegSnapshot::destroy()
{
  if (m_instance) {
    if (m_run) {
      m_run = false;
      m_mutex->lock();
      m_cond->signal();
      m_mutex->unlock();
      AM_DESTROY(m_thread);
      AM_DESTROY(m_jpeg_encoder);
    }
    deinit_dir();
    delete m_param;
    m_param = nullptr;
  }
}

AM_RESULT AMJpegSnapshot::set_source_buffer(AM_SOURCE_BUFFER_ID id)
{
  AM_RESULT ret = AM_RESULT_OK;
  do {
    if (m_param == nullptr) {
      ERROR("invalid paraments");
      ret = AM_RESULT_ERR_INVALID;
      break;
    }
    m_param->source_buffer = id;
  } while (0);
  return ret;
}

AM_RESULT AMJpegSnapshot::set_file_path(string path)
{
  AM_RESULT ret = AM_RESULT_OK;
  do {
    if (m_param == nullptr || path.empty()) {
      ERROR("invalid paraments");
      ret = AM_RESULT_ERR_INVALID;
      break;
    }
    m_param->jpeg_path = path;
  } while (0);
  return ret;
}

AM_RESULT AMJpegSnapshot::set_fps(float fps)
{
  AM_RESULT ret = AM_RESULT_OK;
  do {
    if (m_param == nullptr || fps < 0.0) {
      ERROR("invalid paraments");
      ret = AM_RESULT_ERR_INVALID;
      break;
    }
    m_param->fps = fps;
  } while (0);
  return ret;
}
#define MAX_JPEG_FILE_NUM    500
AM_RESULT AMJpegSnapshot::set_file_max_num(int num)
{
  AM_RESULT ret = AM_RESULT_OK;
  do {
    if (m_param == nullptr || num > MAX_JPEG_FILE_NUM || num <= 0) {
      ERROR("invalid paraments");
      ret = AM_RESULT_ERR_INVALID;
      break;
    }
    m_param->max_files = num;
  } while (0);
  return ret;
}

AM_RESULT AMJpegSnapshot::set_data_cb(AMJpegSnapshotCb cb)
{
  m_data_cb = cb;
  return AM_RESULT_OK;
}

AM_RESULT AMJpegSnapshot::save_file_enable()
{
  AM_RESULT ret = AM_RESULT_OK;
  do {
    if (m_param == nullptr) {
      ERROR("invalid paraments");
      ret = AM_RESULT_ERR_INVALID;
      break;
    }
    m_param->need_save_file = true;
  } while (0);
  return ret;
}

AM_RESULT AMJpegSnapshot::save_file_disable()
{
  AM_RESULT ret = AM_RESULT_OK;
  do {
    if (m_param == nullptr) {
      ERROR("invalid paraments");
      ret = AM_RESULT_ERR_INVALID;
      break;
    }
    m_param->need_save_file = false;
  } while (0);
  return ret;
}

bool AMJpegSnapshot::init_dir()
{
  bool res = true;
  struct file_table *ft = nullptr;
  do {
    if (false == AMFile::create_path(m_param->jpeg_path.c_str())) {
      ERROR("AMFile::create_path failed!\n");
      res = false;
      break;
    }
    m_file_table = (struct file_table *)calloc(1,
                    m_param->max_files * sizeof(struct file_table));
    INFO("m_file_table = %p", m_file_table);
    if (!m_file_table) {
      ERROR("malloc file_table failed!\n");
      res = false;
      break;
    }
    for (int i = 0; i < m_param->max_files; ++i) {
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
    m_file_table = nullptr;
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
    if (str == nullptr || len <= 0) {
      ERROR("invalid paraments!\n");
      res = AM_RESULT_ERR_INVALID;
      break;
    }
    gettimeofday(&tv, nullptr);
    now_sec = tv.tv_sec;
    now_ms = tv.tv_usec/1000;
    localtime_r(&now_sec, &now_tm);
    strftime(date_fmt, 20, "%Y%m%d_%H%M%S", &now_tm);
    snprintf(date_ms, sizeof(date_ms), "%03d", now_ms);
    snprintf(str, len, "%s/%s_%s.jpeg",
             m_param->jpeg_path.c_str(), date_fmt, date_ms);
  } while (0);
  return res;
}

int AMJpegSnapshot::get_yuv_file_name(char *str, int len)
{
  char date_fmt[20];
  char date_ms[4];
  struct timeval tv;
  struct tm now_tm;
  int now_ms;
  time_t now_sec;
  int res = AM_RESULT_OK;
  do {
    if (str == nullptr || len <= 0) {
      ERROR("invalid paraments!\n");
      res = AM_RESULT_ERR_INVALID;
      break;
    }
    gettimeofday(&tv, nullptr);
    now_sec = tv.tv_sec;
    now_ms = tv.tv_usec/1000;
    localtime_r(&now_sec, &now_tm);
    strftime(date_fmt, 20, "%Y%m%d_%H%M%S", &now_tm);
    snprintf(date_ms, sizeof(date_ms), "%03d", now_ms);
    snprintf(str, len, "%s/%s_%s.yuv",
             m_param->jpeg_path.c_str(), date_fmt, date_ms);
  } while (0);
  return res;
}

int AMJpegSnapshot::trim_jpeg_files()
{
  struct file_table *ft = nullptr;
  int res = AM_RESULT_OK;
  do {
    ++m_file_index;
    if (m_file_index >= m_param->max_files) {
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

int AMJpegSnapshot::save_yuv_in_file(char *filename, AMYUVData *yuv)
{
  int res = AM_RESULT_OK;
  AMFile f(filename);
  do {
    if (!f.open(AMFile::AM_FILE_CREATE)) {
      PERROR("Failed to open file");
      res = AM_RESULT_ERR_INVALID;
      break;
    }
    if (f.write_reliable(yuv->y.iov_base, yuv->y.iov_len) < 0) {
      PERROR("Failed to sava y data into file\n");
      res = AM_RESULT_ERR_INVALID;
      break;
    }
    if (f.write_reliable(yuv->uv.iov_base, yuv->uv.iov_len) < 0) {
      PERROR("Failed to sava uv data into file\n");
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
    m_param = new JpegSnapshotParam();
    if (!m_param) {
      ERROR("AMJpegSnapshotConfig:: new m_param error\n");
      res = AM_RESULT_ERR_INVALID;
      break;
    }
    m_jpeg_encoder = AMIJpegEncoder::get_instance();
    if (!m_jpeg_encoder) {
      res = AM_RESULT_ERR_INVALID;
      ERROR("Failed to create AMJpegEncode!");
      break;
    }
    if (0 != m_jpeg_encoder->create()) {
      res = AM_RESULT_ERR_INVALID;
      ERROR("Failed to create AMJpegEncode!");
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

AM_RESULT AMJpegSnapshot::run()
{
  AM_RESULT res = AM_RESULT_OK;
  do {
    if (m_run) {
      INFO("AMJpegSnapshot:: is already started\n");
      break;
    }
    if (false == init_dir()) {
      res = AM_RESULT_ERR_INVALID;
      ERROR("init dir failed!\n");
      break;
    }
    m_video_reader = AMIVideoReader::get_instance();
    if (!m_video_reader) {
      res = AM_RESULT_ERR_INVALID;
      break;
    }
    m_video_address = AMIVideoAddress::get_instance();
    if (!m_video_address) {
      res = AM_RESULT_ERR_INVALID;
      break;
    }

    m_gdma_support = m_video_reader->is_gdmacpy_support();
    if (m_gdma_support == false) {
      NOTICE("IAV Usr Buffer Size didn't set, only memcpy can be used\n");
      NOTICE("please set IAV_MEM_USR_SIZE=0x00800000 on menuconfig");
    } else {
      if (m_video_address->usr_addr_get(m_usr_addr) != AM_RESULT_OK) {
        ERROR("get usr mem failed \n");
        res = AM_RESULT_ERR_INVALID;
        break;
      }
    }
    m_run = true;
    if (!(m_thread = AMThread::create("JpegEncodeThread",
                                      static_jpeg_encode_thread, this))) {
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

AMYUVData *AMJpegSnapshot::capture_yuv_buffer()
{
  AMQueryFrameDesc frame_desc;
  AMQueryFrameDesc frame_tmp;
  static std::list<AMQueryFrameDesc> frame_desc_queue;
  uint32_t cache_size = 0;
  AMYUVData *yuv = nullptr;
  AM_RESULT result = AM_RESULT_OK;
  uint8_t *y_offset, *uv_offset;
  int32_t uv_height;
  int buf_id = m_param->source_buffer;

  do {
    yuv = (AMYUVData *)calloc(1, sizeof(AMYUVData));
    if (!yuv) {
      ERROR("malloc yuv failed!\n");
      result = AM_RESULT_ERR_INVALID;
      break;
    }

    result = m_video_reader->query_yuv_frame(frame_desc,
                                AM_SOURCE_BUFFER_ID(buf_id), false);
    if (result != AM_RESULT_OK) {
      ERROR("query yuv frame failed \n");
      result = AM_RESULT_ERR_INVALID;
      break;
    }
    uint32_t resolution = frame_desc.yuv.width * frame_desc.yuv.height;
    if (resolution >= 1920*1080) {//Full HD
      cache_size = 1;
    } else if (resolution >= 1280*720) {//HD
      cache_size = 2;
    } else {//smaller than HD
      cache_size = 2;
    }

    frame_desc_queue.push_back(frame_desc);
    if (frame_desc_queue.size() < cache_size) {
      ERROR("cache first frame, size = %d", frame_desc_queue.size());
      break;
    }
    if (!frame_desc_queue.empty()) {
      frame_tmp = frame_desc_queue.front();
      frame_desc_queue.pop_front();
    } else {
      ERROR("frame_desc_queue is empty");
      result = AM_RESULT_ERR_INVALID;
      break;
    }

    if (m_gdma_support) {
      y_offset = (uint8_t *)frame_tmp.yuv.y_offset;
      uv_offset = (uint8_t *)frame_tmp.yuv.uv_offset;
    } else {
      AMAddress yaddr, uvaddr;
      result = m_video_address->yuv_y_addr_get(frame_tmp, yaddr);
      if (result != AM_RESULT_OK) {
        ERROR("yuv_y_addr_get failed");
        break;
      }
      y_offset = yaddr.data;
      result = m_video_address->yuv_uv_addr_get(frame_tmp, uvaddr);
      if (result != AM_RESULT_OK) {
        ERROR("yuv_uv_addr_get failed");
        break;
      }
      uv_offset = uvaddr.data;
    }

    yuv->width = frame_tmp.yuv.width;
    yuv->height = frame_tmp.yuv.height;
    yuv->pitch = frame_tmp.yuv.pitch;
    yuv->format = frame_tmp.yuv.format;
    yuv->y_addr = y_offset;
    yuv->uv_addr = uv_offset;
    if (frame_tmp.yuv.format == AM_CHROMA_FORMAT_YUV420) {
      uv_height = yuv->height / 2;
    } else if (frame_tmp.yuv.format == AM_CHROMA_FORMAT_YUV422) {
      uv_height = yuv->height;
    } else {
      ERROR("not supported chroma format in YUV dump\n");
      result = AM_RESULT_ERR_INVALID;
      break;
    }
    if (m_gdma_support) {
      //copy y buffer
      yuv->y.iov_len = yuv->width * yuv->height;
      yuv->y.iov_base = (uint8_t *)USR_DRAM_Y_OFFSET;
      result = m_video_reader->gdmacpy(yuv->y.iov_base, y_offset,
                                       yuv->width, yuv->height, yuv->pitch);
      if (result != AM_RESULT_OK) {
        ERROR("gdmacpy y buffer failed");
        break;
      }
      yuv->y.iov_base = (uint8_t *)(m_usr_addr.data);

      //copy uv buffer
      yuv->uv.iov_len = yuv->pitch * uv_height;//XXX: pitch
      yuv->uv.iov_base = (uint8_t *)yuv->y.iov_len;
      result = m_video_reader->gdmacpy(yuv->uv.iov_base, uv_offset,
                                       yuv->pitch, uv_height, yuv->pitch);
      if (result != AM_RESULT_OK) {
        ERROR("gdmacpy uv buffer failed");
        break;
      }
      yuv->uv.iov_base = (uint8_t *)m_usr_addr.data + yuv->y.iov_len;
      yuv->buffer = m_usr_addr.data;
    } else {
      //copy y buffer
      yuv->y.iov_len = yuv->width * yuv->height;
      yuv->y.iov_base = (uint8_t *)calloc(1, yuv->y.iov_len);
      uint8_t *y_tmp = (uint8_t *)yuv->y.iov_base;
      if (yuv->pitch == yuv->width) {
        memcpy(yuv->y.iov_base, y_offset, yuv->y.iov_len);
      } else {
        for (int i = 0; i < yuv->height ; i++) {
          memcpy(y_tmp, y_offset + i * yuv->pitch, yuv->width);
          y_tmp += yuv->width;
        }
      }

      //copy uv buffer
      yuv->uv.iov_len = yuv->pitch * uv_height;//XXX: pitch
      yuv->uv.iov_base = (uint8_t *)calloc(1, yuv->uv.iov_len);
      memcpy(yuv->uv.iov_base, uv_offset, yuv->uv.iov_len);
    }

  } while (0);

  if (result != AM_RESULT_OK) {
    free_yuv_buffer(yuv);
    yuv = nullptr;
  }

  return yuv;
}

AMYUVData *AMJpegSnapshot::query_yuv_buffer()
{
  AMQueryFrameDesc frame_desc;
  AMYUVData *yuv = nullptr;
  AM_RESULT result = AM_RESULT_OK;
  uint8_t *y_offset, *uv_offset;
  int buf_id = m_param->source_buffer;

  do {
    yuv = (AMYUVData *)calloc(1, sizeof(AMYUVData));
    if (!yuv) {
      ERROR("malloc yuv failed!\n");
      result = AM_RESULT_ERR_INVALID;
      break;
    }

    result = m_video_reader->query_yuv_frame(frame_desc,
                                AM_SOURCE_BUFFER_ID(buf_id), false);
    if (result != AM_RESULT_OK) {
      ERROR("query yuv frame failed \n");
      result = AM_RESULT_ERR_INVALID;
      break;
    }

    AMAddress yaddr, uvaddr;
    result = m_video_address->yuv_y_addr_get(frame_desc, yaddr);
    if (result != AM_RESULT_OK) {
      ERROR("yuv_y_addr_get failed");
      break;
    }
    y_offset = yaddr.data;
    result = m_video_address->yuv_uv_addr_get(frame_desc, uvaddr);
    if (result != AM_RESULT_OK) {
      ERROR("yuv_uv_addr_get failed");
      break;
    }
    uv_offset = uvaddr.data;

    yuv->width = frame_desc.yuv.width;
    yuv->height = frame_desc.yuv.height;
    yuv->pitch = frame_desc.yuv.pitch;
    yuv->format = frame_desc.yuv.format;
    yuv->y_addr = y_offset;
    yuv->uv_addr = uv_offset;
  } while (0);

  if (result != AM_RESULT_OK) {
    free_yuv_buffer(yuv);
    yuv = nullptr;
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
    if (jpeg_encoder->m_state == AM_SNAPSHOT_STOP) {
      jpeg_encoder->m_mutex->lock();
      INFO("wait event");
      jpeg_encoder->m_cond->wait(jpeg_encoder->m_mutex);
      jpeg_encoder->m_mutex->unlock();
      INFO("event occur");
      continue;
    } else {
      gettimeofday(&curr, nullptr);
      pre = curr;
      jpeg_encoder->encode_yuv_to_jpeg();
      gettimeofday(&curr, nullptr);
      ms_encode = ((1000000 * (curr.tv_sec - pre.tv_sec))
                  + (curr.tv_usec - pre.tv_usec)) / 1000;
      INFO("encode_yuv_to_jpeg take [%06ld ms]", ms_encode);
      ms_sleep = (1.0/jpeg_encoder->m_param->fps)*1000 - ms_encode;
      ms_sleep = (ms_sleep < 0) ? 0 : ms_sleep;
      delay.tv_sec = 0;
      delay.tv_usec = ms_sleep * 1000;
      select(0, nullptr, nullptr, nullptr, &delay);
    }
  }
}

AM_RESULT AMJpegSnapshot::capture_start()
{
  m_state = AM_SNAPSHOT_START;
  m_mutex->lock();
  m_cond->signal();
  m_mutex->unlock();
  return AM_RESULT_OK;
}

AM_RESULT AMJpegSnapshot::capture_stop()
{
  m_state = AM_SNAPSHOT_STOP;
  m_mutex->lock();
  m_cond->signal();
  m_mutex->unlock();
  return AM_RESULT_OK;
}

void AMJpegSnapshot::free_yuv_buffer(AMYUVData *yuv)
{
  if (yuv) {
    if (m_gdma_support == false) {
      if (yuv->y.iov_base) {
        free(yuv->y.iov_base);
      }
      if (yuv->uv.iov_base) {
        free(yuv->uv.iov_base);
      }
    }
    free(yuv);
  }
}

int AMJpegSnapshot::encode_yuv_to_jpeg()
{
  AM_RESULT res = AM_RESULT_OK;
  int ret = -1;
  char filename_jpeg[MAX_FILENAME_LEN];
  struct file_table *ft = nullptr;
  AMYUVData *yuv = nullptr;
  AMJpegData *jpeg = nullptr;

  do {
    yuv = query_yuv_buffer();
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
    if (m_data_cb) {
      m_data_cb(jpeg->data.iov_base, jpeg->data.iov_len);
    }
    if (m_param->need_save_file) {
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
    }
    m_jpeg_encoder->free_jpeg_data(jpeg);
  } while (0);

  free_yuv_buffer(yuv);

  return res;
}
