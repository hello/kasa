/*******************************************************************************
 * am_motion_detect.cpp
 *
 * History:
 *   May 3, 2016 - [binwang] created file
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
#include "am_define.h"
#include "am_log.h"
#include "am_thread.h"

#include "am_video_reader_if.h"
#include "am_video_address_if.h"

#include "mdet_lib.h"
#include "am_motion_detect.h"

enum AM_MOTIONT_STATUS
{
  MD_NO_MOTION = 0,
  MD_IN_MOTION,
};

#define MOTION_DETECT_CONFIG ((const char*)"event-motion-detect.acs")
#define SKIP_CNT 1
#define MAX_MOTION_INDICATOR 1000000
#define ROI_POINTS 4 //4 points decide a roi, left, right, top. bottom
#define MAX_BUF_WIDTH 1280
#define MAX_BUF_HEIGHT 720
#define AUTO_LOCK_MD(mtx) std::lock_guard<std::recursive_mutex> lck (mtx)

static std::recursive_mutex motion_detect_mtx;

#ifdef __cplusplus
extern "C" {
#endif
AMIEncodePlugin* create_encode_plugin(void *data)
{
  return AMMotionDetect::create((AMVin*)data);
}
#ifdef __cplusplus
}
#endif

AMMotionDetect::AMMotionDetect(const char *name) :
  m_name(name),
  m_conf_path(""),
  m_main_loop_exit(true),
  m_started(false),
  m_enable(false),
  m_platform(nullptr),
  m_video_reader(nullptr),
  m_video_address(nullptr),
  m_callback(nullptr),
  m_md_thread(nullptr),
  m_inst(nullptr),
  m_config(nullptr),
  m_roi_info(nullptr),
  m_threshold(nullptr),
  m_level_change_delay(nullptr),
  m_callback_owner_obj(nullptr)
{
  memset(&m_frame_desc, 0, sizeof(m_frame_desc));
  memset(&mdet_session, 0, sizeof(mdet_session));
  memset(&mdet_config, 0, sizeof(mdet_config));
  m_msg =
  { AM_SOURCE_BUFFER_MAIN, AM_DATA_FRAME_TYPE_ME1,
    0, 0, 0, AM_MD_MOTION_NONE, AM_MOTION_L0, 0, nullptr};
}

AMMotionDetect::~AMMotionDetect()
{
  stop();
  delete[] m_roi_info;
  delete[] m_threshold;
  delete[] m_level_change_delay;
  AM_RELEASE(m_config);
  mdet_destroy_instance(m_inst);
  m_platform = nullptr;
  m_video_reader = nullptr;
  m_video_address = nullptr;
}

AM_RESULT AMMotionDetect::load_config()
{
  AM_RESULT result = AM_RESULT_OK;

  do {
    if (!m_config) {
      result = AM_RESULT_ERR_INVALID;
      ERROR("m_config is null!");
      break;
    }
    if ((result = m_config->load_config()) != AM_RESULT_OK) {
      ERROR("Failed to get motion detect config!");
      break;
    }
    if ((result = m_config->get_config(m_config_param)) != AM_RESULT_OK) {
      ERROR("Failed to get motion detect config!");
      break;
    }
  } while (0);
  return result;
}

AM_RESULT AMMotionDetect::save_config()
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (!m_config) {
      result = AM_RESULT_ERR_INVALID;
      ERROR("m_config is null!");
      break;
    }
    sync_config();
    if ((result = m_config->set_config(m_config_param)) != AM_RESULT_OK) {
      ERROR("Failed to get motion detect config!");
      break;
    }
    if ((result = m_config->save_config()) != AM_RESULT_OK) {
      ERROR("Failed to set motion detect config!");
      break;
    }
  } while (0);
  return result;
}

bool AMMotionDetect::sync_config()
{
  bool result = true;
  do {
    m_config_param.enable = m_enable;
    memcpy(&m_config_param.buf, &m_md_buffer,
            sizeof(m_config_param.buf));
    memcpy(m_config_param.roi_info,
           m_roi_info,
           sizeof(m_config_param.roi_info));
    memcpy(m_config_param.th, m_threshold, sizeof(m_config_param.th));
    memcpy(m_config_param.lc_delay,
           m_level_change_delay,
           sizeof(m_config_param.lc_delay));
  } while (0);

  return result;
}

bool AMMotionDetect::construct()
{
  bool result = true;
  do {
    m_roi_info = new AMMDRoi[MAX_ROI_NUM];
    m_threshold = new AMMDThreshold[MAX_ROI_NUM];
    m_level_change_delay = new AMMDLevelChangeDelay[MAX_ROI_NUM];

    if (AM_UNLIKELY(!(m_config = AMMotionDetectConfig::get_instance()))) {
      ERROR("Failed to create AMMotionDetectConfig!");
      break;
    }
    if (AM_UNLIKELY(AM_RESULT_OK != load_config())) {
      ERROR("Motion Detect load config error");
      break;
    }
    m_enable = m_config_param.enable;
    memcpy(&m_md_buffer, &m_config_param.buf, sizeof(m_config_param.buf));
    memcpy(m_roi_info, m_config_param.roi_info,
           sizeof(m_config_param.roi_info));
    memcpy(m_threshold, m_config_param.th, sizeof(m_config_param.th));
    memcpy(m_level_change_delay,
           m_config_param.lc_delay, sizeof(m_config_param.lc_delay));
    m_inst = mdet_create_instance(MDET_ALGO_DIFF);
    if (!m_inst) {
      ERROR("AMMotionDetect::mdet_create_instance error\n");
      break;
    }

    m_platform = AMIPlatform::get_instance();
    if (!m_platform) {
      ERROR("Failed to create AMIPlatform");
      result = false;
      break;
    }

    m_video_reader = AMIVideoReader::get_instance();
    if (!m_video_reader) {
      ERROR("AMMotionDetect::Unable to get AMVideoReader instance\n");
      result = false;
      break;
    }

    m_video_address = AMIVideoAddress::get_instance();
    if (!m_video_address) {
      ERROR("Failed to get instance of VideoAddress!");
      result = false;
      break;
    }
  } while (0);

  return result;
}

AMMotionDetect *AMMotionDetect::create(AMVin *vin)
{
  AMMotionDetect *result = new AMMotionDetect("motion-detect");
  if (result && !result->construct()) {
    ERROR("AMMotionDetect::Failed to create an instance of AMMotionDetect\n");
    delete result;
    result = NULL;
  } else {
    INFO("AMMotionDetect::create successfully!\n");
  }

  return result;
}

void AMMotionDetect::destroy()
{
  stop();
  delete this;
}

bool AMMotionDetect::start(AM_STREAM_ID id)
{
  AUTO_LOCK_MD(motion_detect_mtx);
  INFO("AMMotionDetect::start motion detect plugin\n");
  bool result = true;
  do {
    if (m_started) {
      NOTICE("motion detect has already started!\n");
      break;
    }
    //get one frame data from buffer and do mdet_start.
    switch (m_md_buffer.buf_type) {
      case AM_DATA_FRAME_TYPE_YUV:
        if (m_video_reader->query_yuv_frame(m_frame_desc,
                                            m_md_buffer.buf_id,
                                            false) != AM_RESULT_OK) {
          ERROR("AMMotionDetect::from buffer %d failed\n", m_md_buffer.buf_id);
          result = false;
        } else {
          mdet_config.fm_dim.height = m_frame_desc.yuv.height;
          mdet_config.fm_dim.width = m_frame_desc.yuv.width;
          mdet_config.fm_dim.pitch = m_frame_desc.yuv.pitch;
        }
        break;
      case AM_DATA_FRAME_TYPE_ME0:
        if (m_video_reader->query_me0_frame(m_frame_desc,
                                            m_md_buffer.buf_id,
                                            false) != AM_RESULT_OK) {
          ERROR("AMMotionDetect::from buffer %d failed\n", m_md_buffer.buf_id);
          result = false;
        } else {
          mdet_config.fm_dim.height = m_frame_desc.me.height;
          mdet_config.fm_dim.width = m_frame_desc.me.width;
          mdet_config.fm_dim.pitch = m_frame_desc.me.pitch;
        }
        break;
      case AM_DATA_FRAME_TYPE_ME1:
        if (m_video_reader->query_me1_frame(m_frame_desc,
                                            m_md_buffer.buf_id,
                                            false) != AM_RESULT_OK) {
          ERROR("AMMotionDetect::from buffer %d failed\n", m_md_buffer.buf_id);
          result = false;
        } else {
          mdet_config.fm_dim.height = m_frame_desc.yuv.height;
          mdet_config.fm_dim.width = m_frame_desc.yuv.width;
          mdet_config.fm_dim.pitch = m_frame_desc.yuv.pitch;
        }
        break;
      default:
        break;
    }
    if (!result) {
      break;
    }
    if (mdet_config.fm_dim.width > MAX_BUF_WIDTH ||
        mdet_config.fm_dim.height > MAX_BUF_HEIGHT) {
      result = false;
      ERROR("Buffer size %dx%d exceeds %dx%d",
            mdet_config.fm_dim.width, mdet_config.fm_dim.height,
            MAX_BUF_WIDTH, MAX_BUF_HEIGHT);
      break;
    }

    INFO("buf[%d]: type=%d, width=%d, height=%d, pitch=%d\n",
         m_md_buffer.buf_id,
         m_md_buffer.buf_type,
         mdet_config.fm_dim.width,
         mdet_config.fm_dim.height,
         mdet_config.fm_dim.pitch);
    mdet_config.roi_info.num_roi = 0;
    for (uint32_t i = 0; i < MDET_MAX_ROIS; i ++) {
      m_roi_info[i].roi_id = i;
      set_roi_info(&m_roi_info[i]);
      if (m_roi_info[i].left != 0 || m_roi_info[i].right != 0
          || m_roi_info[i].top != 0 || m_roi_info[i].bottom != 0) {
        mdet_config.roi_info.num_roi ++;
      }
    }
    mdet_cfg mdet_config_get;
    (*m_inst->md_get_config)(&mdet_config_get);
    mdet_config.threshold = mdet_config_get.threshold;
    //print_md_config();
    (*m_inst->md_set_config)(&mdet_config);
    if ((*m_inst->md_start)(&mdet_session) < 0) {
      ERROR("AMMotionDetect::mdet_start failed\n");
      result = false;
      break;
    }

    //front ground matrix size equals buffer size, 1 means motion, 0 means no motion
    m_msg.motion_matrix = mdet_session.fg;
    m_msg.buf.buf_id = m_md_buffer.buf_id;
    m_msg.buf.buf_type = m_md_buffer.buf_type;

    m_main_loop_exit = false;
    m_md_thread = AMThread::create("Video.MD", md_main, this);
    if (!m_md_thread) {
      ERROR("AMMotionDetect::create m_md_thread failed\n");
      result = false;
      break;
    }

    m_started = true;
  } while (0);

  return result;
}

bool AMMotionDetect::stop(AM_STREAM_ID id)
{
  AUTO_LOCK_MD(motion_detect_mtx);
  bool result = true;

  if (m_started) {
    m_main_loop_exit = true;
    AM_DESTROY(m_md_thread);
    INFO("AMMotionDetect::Destroy <m_md_thread> successfully\n");
    if ((*m_inst->md_stop)(&mdet_session) < 0) {
      ERROR("AMMotionDetect::mdet_stop error\n");
      result = false;
    }
    m_started = false;
  } else {
    NOTICE("motion detect has been stopped already!\n");
  }

  return result;
}

std::string& AMMotionDetect::name()
{
  return m_name;
}

void* AMMotionDetect::get_interface()
{
  return ((AMIMotionDetect*)this);
}

bool AMMotionDetect::set_config(AMMDConfig *pConfig)
{
  AUTO_LOCK_MD(motion_detect_mtx);
  bool result = true;
  if (!pConfig || !pConfig->value) {
    ERROR("AMMotionDetect::Invalid argument!\n");
    return false;
  }

  switch (pConfig->key) {
    case AM_MD_ENABLE:
      result = set_md_state(*(bool*) pConfig->value);
      break;
    case AM_MD_BUFFER_ID:
      result = set_md_buffer_id(*(AM_SOURCE_BUFFER_ID*)pConfig->value);
      break;
    case AM_MD_BUFFER_TYPE:
      result = set_md_buffer_type(*(AM_DATA_FRAME_TYPE*)pConfig->value);
      break;
    case AM_MD_ROI:
      result = set_roi_info((AMMDRoi*)pConfig->value);
      break;
    case AM_MD_THRESHOLD:
      result = set_threshold_info((AMMDThreshold*)pConfig->value);
      break;
    case AM_MD_LEVEL_CHANGE_DELAY:
      result = set_level_change_delay_info(
          (AMMDLevelChangeDelay*)pConfig->value);
      break;
    case AM_MD_SYNC_CONFIG:
      result = sync_config();
      break;
    default:
      ERROR("Unknown key\n");
      result = false;
      break;
  }

  return result;
}

bool AMMotionDetect::get_config(AMMDConfig *pConfig)
{
  AUTO_LOCK_MD(motion_detect_mtx);
  bool result = true;
  if (!pConfig || !pConfig->value) {
    ERROR("AMMotionDetect::Invalid argument!\n");
    return false;
  }

  switch (pConfig->key) {
    case AM_MD_ENABLE:
      result = get_md_state((bool *) pConfig->value);
      break;
    case AM_MD_BUFFER_ID:
      result = get_md_buffer_id((AM_SOURCE_BUFFER_ID*)pConfig->value);
      break;
    case AM_MD_ROI:
      result = get_roi_info((AMMDRoi*)pConfig->value);
      break;
    case AM_MD_BUFFER_TYPE:
      result = get_md_buffer_type((AM_DATA_FRAME_TYPE*)pConfig->value);
      break;
    case AM_MD_THRESHOLD:
      result = get_threshold_info((AMMDThreshold*)pConfig->value);
      break;
    case AM_MD_LEVEL_CHANGE_DELAY:
      result = get_level_change_delay_info(
          (AMMDLevelChangeDelay*)pConfig->value);
      break;
    default:
      ERROR("Unknown key\n");
      result = false;
      break;
  }

  return result;
}

bool AMMotionDetect::set_md_state(bool enable)
{
  m_enable = enable;
  return true;
}

bool AMMotionDetect::get_md_buffer_id(AM_SOURCE_BUFFER_ID *source_buf_id)
{
  bool result = true;
  do {
    if (!source_buf_id) {
      ERROR("AMMotionDetect::get_md_buffer_id NULL pointer\n");
      result = false;
      break;
    }
    *source_buf_id = m_md_buffer.buf_id;
  } while (0);

  return result;
}

bool AMMotionDetect::set_md_buffer_id(AM_SOURCE_BUFFER_ID source_buf_id)
{
  m_md_buffer.buf_id = source_buf_id;
  return true;
}

bool AMMotionDetect::get_md_buffer_type(AM_DATA_FRAME_TYPE *source_buf_type)
{
  bool result = true;
  do {
    if (!source_buf_type) {
      ERROR("AMMotionDetect::get_md_buffer_type NULL pointer\n");
      result = false;
      break;
    }
    *source_buf_type = m_md_buffer.buf_type;
  } while (0);

  return result;
}

bool AMMotionDetect::set_md_buffer_type(AM_DATA_FRAME_TYPE source_buf_type)
{
  bool ret = true;
  do {
    AMFeatureParam param;
    if (m_platform->feature_config_get(param) != AM_RESULT_OK) {
      ret = false;
      break;
    }
    if ((param.version.second != 3) &&
        (m_md_buffer.buf_id != AM_SOURCE_BUFFER_MAIN) &&
        (source_buf_type == AM_DATA_FRAME_TYPE_ME0)) {
      ERROR("Platform%d buffer%d not support ME0 data type!",
            param.version.second, m_md_buffer.buf_id);
      ret = false;
      break;
    }
    if ((source_buf_type != AM_DATA_FRAME_TYPE_YUV) &&
        (source_buf_type != AM_DATA_FRAME_TYPE_ME0) &&
        (source_buf_type != AM_DATA_FRAME_TYPE_ME1)) {
      ERROR("Wrong buffer type %d for motion detect!",source_buf_type);
      ret = false;
      break;
    }
    if (source_buf_type == AM_DATA_FRAME_TYPE_YUV) {
      AMQueryFrameDesc  desc;
      if (m_video_reader->query_yuv_frame(desc, m_md_buffer.buf_id,
                                          false) != AM_RESULT_OK) {
        ERROR("Get buf%d YUV frame description failed\n", m_md_buffer.buf_id);
        ret = false;
        break;
      }
      if (desc.yuv.width > MAX_BUF_WIDTH || desc.yuv.height > MAX_BUF_HEIGHT) {
        ERROR("Buffer%d size: %dx%d exceeds MD max support size: %dx%d",
              m_md_buffer.buf_id, desc.yuv.width, desc.yuv.height,
              MAX_BUF_WIDTH, MAX_BUF_HEIGHT);
        ret = false;
        break;
      }
    }

    m_md_buffer.buf_type = source_buf_type;
  } while(0);
  return ret;
}

bool AMMotionDetect::get_md_state(bool *enable)
{
  bool result = true;
  do {
    if (!enable) {
      ERROR("AMMotionDetect::get_md_state NULL pointer\n");
      result = false;
      break;
    }
    *enable = m_enable;
  } while (0);

  return result;
}

bool AMMotionDetect::check_md_roi_format(AMMDRoi *roi_info,
                                         uint32_t buf_width,
                                         uint32_t buf_height,
                                         uint32_t buf_pitch)
{
  bool result = true;
  do {
    if (!roi_info) {
      ERROR("AMMotionDetect::check_md_roi_format, NULL pointer\n");
      result = false;
      break;
    }
    if ((roi_info->right > buf_width - 1) ||
        (roi_info->bottom > buf_height - 1)) {
      ERROR("roi%d right:%d, bottom:%d exceeds buffer "
            "width-1:%d, height-1:%d. \nSet to be buffer's window\n",
            roi_info->roi_id, roi_info->right, roi_info->bottom,
            buf_width - 1, buf_height - 1);
      roi_info->right = buf_width - 1;
      roi_info->bottom = buf_height - 1;
    }
    if ((roi_info->right > MAX_BUF_WIDTH - 1) ||
        (roi_info->bottom > MAX_BUF_HEIGHT - 1)) {
      ERROR("roi%d right:%d, bottom:%d exceeds MD max support size "
            "width-1:%d, height-1:%d. \nSet to be max support size window\n",
            roi_info->roi_id, roi_info->right, roi_info->bottom,
            buf_width - 1, buf_height - 1);
      roi_info->right = MAX_BUF_WIDTH - 1;
      roi_info->bottom = MAX_BUF_HEIGHT - 1;
    }
    if ((roi_info->valid) &&
        ((roi_info->left > roi_info->right) ||
        (roi_info->top > roi_info->bottom) ||
        (roi_info->right - roi_info->left < 2) ||
        (roi_info->bottom - roi_info->top < 2))) {
      ERROR("AMMotionDetect::roi%d must follow rule: right - left >= 2, "
            "bottom - top >= 2. Set to be left=0, right=2, top=0, bottom = 2\n",
            roi_info->roi_id);
      roi_info->left = 0;
      roi_info->top = 0;
      roi_info->right = 2;
      roi_info->bottom = 2;
    }

  } while (0);
  return result;
}

bool AMMotionDetect::set_roi_info(AMMDRoi *roi_info)
{
  bool result = true;
  do {
    if (!roi_info) {
      ERROR("AMMotionDetect::set_roi_info NULL pointer\n");
      result = false;
      break;
    }

    //get buffer size
    switch (m_md_buffer.buf_type) {
      case AM_DATA_FRAME_TYPE_YUV:
        if (m_video_reader->query_yuv_frame(m_frame_desc,
                                            m_md_buffer.buf_id,
                                            false) != AM_RESULT_OK) {
          ERROR("AMMotionDetect::from buffer %d failed\n", m_md_buffer.buf_id);
          result = false;
        } else {
          mdet_config.fm_dim.height = m_frame_desc.yuv.height;
          mdet_config.fm_dim.width = m_frame_desc.yuv.width;
          mdet_config.fm_dim.pitch = m_frame_desc.yuv.pitch;
        }
        break;
      case AM_DATA_FRAME_TYPE_ME0:
        if (m_video_reader->query_me0_frame(m_frame_desc,
                                            m_md_buffer.buf_id,
                                            false) != AM_RESULT_OK) {
          ERROR("AMMotionDetect::from buffer %d failed\n", m_md_buffer.buf_id);
          result = false;
        } else {
          mdet_config.fm_dim.height = m_frame_desc.me.height;
          mdet_config.fm_dim.width = m_frame_desc.me.width;
          mdet_config.fm_dim.pitch = m_frame_desc.me.pitch;
        }
        break;
      case AM_DATA_FRAME_TYPE_ME1:
        if (m_video_reader->query_me1_frame(m_frame_desc,
                                            m_md_buffer.buf_id,
                                            false) != AM_RESULT_OK) {
          ERROR("AMMotionDetect::from buffer %d failed\n", m_md_buffer.buf_id);
          result = false;
        } else {
          mdet_config.fm_dim.height = m_frame_desc.yuv.height;
          mdet_config.fm_dim.width = m_frame_desc.yuv.width;
          mdet_config.fm_dim.pitch = m_frame_desc.yuv.pitch;
        }
        break;
      default:
        break;
    }
    if (!result) {
      break;
    }
    if (!check_md_roi_format(roi_info, mdet_config.fm_dim.width,
                             mdet_config.fm_dim.height,
                             mdet_config.fm_dim.pitch)) {
      ERROR("AMMotionDetect::set_roi_info roi[%d] format wrong\n",
            roi_info->roi_id);
      result = false;
      break;
    }
    m_roi_info[roi_info->roi_id].roi_id = roi_info->roi_id;
    m_roi_info[roi_info->roi_id].left = roi_info->left;
    m_roi_info[roi_info->roi_id].right = roi_info->right;
    m_roi_info[roi_info->roi_id].top = roi_info->top;
    m_roi_info[roi_info->roi_id].bottom = roi_info->bottom;
    m_roi_info[roi_info->roi_id].valid = roi_info->valid;
    mdet_config.roi_info.roi[roi_info->roi_id].type = MDET_REGION_POLYGON;
    mdet_config.roi_info.roi[roi_info->roi_id].num_points = ROI_POINTS;
    mdet_config.roi_info.roi[roi_info->roi_id].points[0].x = roi_info->left;
    mdet_config.roi_info.roi[roi_info->roi_id].points[0].y = roi_info->top;
    mdet_config.roi_info.roi[roi_info->roi_id].points[1].x = roi_info->right;
    mdet_config.roi_info.roi[roi_info->roi_id].points[1].y = roi_info->top;

    mdet_config.roi_info.roi[roi_info->roi_id].points[2].x = roi_info->left;
    mdet_config.roi_info.roi[roi_info->roi_id].points[2].y = roi_info->bottom;
    mdet_config.roi_info.roi[roi_info->roi_id].points[3].x = roi_info->right;
    mdet_config.roi_info.roi[roi_info->roi_id].points[3].y = roi_info->bottom;
  } while (0);

  return result;
}

bool AMMotionDetect::get_roi_info(AMMDRoi *roi_info)
{
  bool result = true;
  do {
    if (!roi_info) {
      ERROR("AMMotionDetect::get_roi_info NULL pointer\n");
      result = false;
      break;
    }
    roi_info->left = m_roi_info[roi_info->roi_id].left;
    roi_info->right = m_roi_info[roi_info->roi_id].right;
    roi_info->top = m_roi_info[roi_info->roi_id].top;
    roi_info->bottom = m_roi_info[roi_info->roi_id].bottom;
    roi_info->valid = m_roi_info[roi_info->roi_id].valid;
  } while (0);

  return result;
}

bool AMMotionDetect::set_threshold_info(AMMDThreshold *threshold)
{
  bool result = true;
  do {
    if (!threshold) {
      ERROR("AMMotionDetect::set_theshold_info NULL pointer\n");
      result = false;
      break;
    }
    for (uint32_t i = 0; i < AM_MOTION_L_NUM - 1; i ++) {
      m_threshold[threshold->roi_id].threshold[i] = threshold->threshold[i];
    }
  } while (0);

  return result;
}

bool AMMotionDetect::get_threshold_info(AMMDThreshold *threshold)
{
  bool result = true;
  do {
    if (!threshold) {
      ERROR("AMMotionDetect::get_theshold_info NULL pointer\n");
      result = false;
      break;
    }
    for (uint32_t i = 0; i < AM_MOTION_L_NUM - 1; i ++) {
      threshold->threshold[i] = m_threshold[threshold->roi_id].threshold[i];
    }
  } while (0);

  return result;
}

bool AMMotionDetect::set_level_change_delay_info(
    AMMDLevelChangeDelay *level_change_delay)
{
  bool result = true;
  do {
    if (!level_change_delay) {
      ERROR("AMMotionDetect::set_level_change_delay_info NULL pointer\n");
      result = false;
      break;
    }
    for (uint32_t i = 0; i < AM_MOTION_L_NUM - 1; i ++) {
      m_level_change_delay[level_change_delay->\
                           roi_id].mt_level_change_delay[i] =
                               level_change_delay->mt_level_change_delay[i];
    }
  } while (0);

  return result;
}

bool AMMotionDetect::get_level_change_delay_info(
    AMMDLevelChangeDelay *level_change_delay)
{
  bool result = true;
  do {
    if (!level_change_delay) {
      ERROR("AMMotionDetect::get_level_change_delay_info NULL pointer\n");
      result = false;
      break;
    }
    for (uint32_t i = 0; i < AM_MOTION_L_NUM - 1; i ++) {
      level_change_delay->mt_level_change_delay[i] =
          m_level_change_delay[level_change_delay->\
                               roi_id].mt_level_change_delay[i];
    }
  } while (0);

  return result;
}

bool AMMotionDetect::set_md_callback(void *this_obj,
                                     AMRecvMD recv_md,
                                     AMMDMessage *msg)
{
  bool result = true;
  do {
    if (!this_obj || !recv_md) {
      ERROR("AMMotionDetect::set_md_callback error\n");
      result = false;
      break;
    }
    m_callback = recv_md;
    msg = &m_msg;
    m_callback_owner_obj = this_obj;
  } while (0);

  return result;
}

bool AMMotionDetect::exec_callback_func()
{
  bool ret = true;
  if (AM_LIKELY(m_callback(m_callback_owner_obj, &m_msg) == 0)) {
    ret = true;;
  } else {
    ERROR("AMMotionDetect::exec_callback_func failed");
    ret = false;
  }
  return ret;
}

#ifdef PRINT_MD_DATA
static inline void print_md_data(mdet_session_t md_data)
{
  PRINTF("\n\nmd_data: fm_dim.pitch=%d, fm_dim.width=%d, fm_dim.height=%d \n",
      md_data.fm_dim.pitch,
      md_data.fm_dim.width,
      md_data.fm_dim.height);

  PRINTF("roi_info.num_roi=%d\n", md_data.roi_info.num_roi);
  for (uint32_t i = 0; i < MDET_MAX_ROIS; i ++) {
    PRINTF("\n========roi_info.roi[%d]========", i);
    PRINTF("type=%d, num_points=%d, ",
        md_data.roi_info.roi[i].type,
        md_data.roi_info.roi[i].num_points);
    for (uint32_t j = 0; j < md_data.roi_info.roi[i].num_points; j ++) {
      PRINTF("points[%d].x=%d, points[%d].y=%d, ",
          j,
          md_data.roi_info.roi[i].points[j].x,
          j,
          md_data.roi_info.roi[i].points[j].y);
    }
    PRINTF("motion=%f, ", md_data.motion[i]);
    PRINTF("fg_pxls=%d, pixels=%d\n", md_data.fg_pxls[i], md_data.pixels[i]);
  }
}
#endif

bool AMMotionDetect::check_motion(const AMQueryFrameDesc &buf_frame)
{
  bool result = true;
  static int skip_cnt = SKIP_CNT;

  static AM_MOTIONT_STATUS motion_status[MDET_MAX_ROIS] =
  { MD_IN_MOTION, MD_IN_MOTION, MD_IN_MOTION, MD_IN_MOTION };

  static AM_MOTION_LEVEL motion_level[MDET_MAX_ROIS] =
  { AM_MOTION_L2, AM_MOTION_L2, AM_MOTION_L2, AM_MOTION_L2 };

  static uint32_t motion_level_change_check[MDET_MAX_ROIS][AM_MOTION_L_NUM-1] =
  { 0 };

  static int ignore_first_diff[MDET_MAX_ROIS] = { 0, 0, 0, 0 };

  do {
    if (!m_enable) {
      //if disable motion detect this module reads ME1 data
      //but does not detect motion
      break;
    }

    if (skip_cnt -- >= 0) {
      //Have to add frame skip to avoid too much CPU consumption.
      result = true;
      break;
    } else {
      skip_cnt = SKIP_CNT;
    }
    AMAddress vaddr = {0};

    switch (m_md_buffer.buf_type) {
      case AM_DATA_FRAME_TYPE_YUV:
        if (m_video_address->yuv_y_addr_get(buf_frame, vaddr) != AM_RESULT_OK) {
          result = false;
          ERROR("Failed to get YUV data address!");
        }
        break;
      case AM_DATA_FRAME_TYPE_ME0:
        if (m_video_address->me0_addr_get(buf_frame, vaddr) != AM_RESULT_OK) {
          result = false;
          ERROR("Failed to get ME0 data address!");
        }
        break;
      case AM_DATA_FRAME_TYPE_ME1:
        if (m_video_address->me1_addr_get(buf_frame, vaddr) != AM_RESULT_OK) {
          result = false;
          ERROR("Failed to get ME1 data address!");
        }
        break;
      default:
        break;
    }
    if (!result) {
      break;
    }

    if (m_inst->md_update_frame(&mdet_session,
                                vaddr.data,
                                mdet_config.threshold) < 0) {
      ERROR("AMMotionDetect::mdet_update_frame error\n");
      result = false;
      break;
    }
#ifdef PRINT_MD_DATA
    print_md_data(mdet_session);
#endif

    for (uint32_t i = 0; i < MDET_MAX_ROIS; i ++) {
      if (!m_roi_info[i].valid) {
        continue;
        //ignore invalid roi
      }
      if (ignore_first_diff[i] == 0) {
        ignore_first_diff[i] = 1;
        continue;
      }
      /*
       * mdet_session.motion[i] * 100 = percentage of motion pixels. If one pixel
       * has motion among 1280x720 pixels the motion value should be
       * 1/(1280*720)*MAX_MOTION_INDICATOR=1
      */
      mdet_session.motion[i] *= MAX_MOTION_INDICATOR;
      //PRINTF("ROI%d: motion=%f\n", i, mdet_session.motion[i]);
      m_msg.mt_value = mdet_session.motion[i];
      m_msg.pts = buf_frame.pts;

      if (!exec_callback_func()) {
        ERROR("m_callback occur error!\n");
        result = false;
        break;
      }

      if (motion_status[i] == MD_NO_MOTION) {
        motion_level_change_check[i][AM_MOTION_L0] = 0;
        motion_level_change_check[i][AM_MOTION_L1] = 0;
        if (mdet_session.motion[i] >= m_threshold[i].threshold[AM_MOTION_L0]) {
          motion_status[i] = MD_IN_MOTION;
          if (mdet_session.motion[i] >=
              m_threshold[i].threshold[AM_MOTION_L1]) {
            motion_level[i] = AM_MOTION_L2;
          } else {
            motion_level[i] = AM_MOTION_L1;
          }
          m_msg.seq_num ++;
          m_msg.roi_id = i;
          m_msg.mt_type = AM_MD_MOTION_START;
          m_msg.mt_level = motion_level[i];
          if (!exec_callback_func()) {
            ERROR("m_callback occur error!\n");
            result = false;
            break;
          }
        }
      } else {
        if (mdet_session.motion[i] < m_threshold[i].threshold[AM_MOTION_L0]) {
          motion_level_change_check[i][AM_MOTION_L0] ++;
          if (motion_level_change_check[i][AM_MOTION_L0] >=
              m_level_change_delay[i].mt_level_change_delay[AM_MOTION_L0]) {
            motion_level_change_check[i][AM_MOTION_L0] = 0;
            motion_level_change_check[i][AM_MOTION_L1] = 0;
            //prevent motion level jump from motion level2 to motion level0
            if (motion_level[i] == AM_MOTION_L2) {
              motion_level[i] = AM_MOTION_L1;
              m_msg.mt_type = AM_MD_MOTION_LEVEL_CHANGED;
            } else {
              motion_status[i] = MD_NO_MOTION;
              motion_level[i] = AM_MOTION_L0;
              m_msg.mt_type = AM_MD_MOTION_END;
            }
            if (motion_level[i] != m_msg.mt_level) {
              m_msg.seq_num ++;
              m_msg.roi_id = i;
              m_msg.mt_level = motion_level[i];
              if (!exec_callback_func()) {
                ERROR("m_callback occur error!\n");
                result = false;
                break;
              }
            }
          }
        } else if (mdet_session.motion[i] <
            m_threshold[i].threshold[AM_MOTION_L1]) {
          motion_level_change_check[i][AM_MOTION_L1] ++;
          if (motion_level[i] == AM_MOTION_L2) {
            if (motion_level_change_check[i][AM_MOTION_L1] >=
                m_level_change_delay[i].mt_level_change_delay[AM_MOTION_L1]) {
              motion_level_change_check[i][AM_MOTION_L0] = 0;
              motion_level_change_check[i][AM_MOTION_L1] = 0;
              motion_level[i] = AM_MOTION_L1;
            }
          } else {
            motion_level_change_check[i][AM_MOTION_L0] = 0;
            motion_level_change_check[i][AM_MOTION_L1] = 0;
            motion_level[i] = AM_MOTION_L1;
          }
          if (motion_level[i] != m_msg.mt_level) {
            m_msg.seq_num ++;
            m_msg.roi_id = i;
            m_msg.mt_type = AM_MD_MOTION_LEVEL_CHANGED;
            m_msg.mt_level = motion_level[i];
            if (!exec_callback_func()) {
              ERROR("m_callback occur error!\n");
              result = false;
              break;
            }
          }
        } else {
          motion_level_change_check[i][AM_MOTION_L0] = 0;
          motion_level_change_check[i][AM_MOTION_L1] = 0;
          motion_level[i] = AM_MOTION_L2;
          if (motion_level[i] != m_msg.mt_level) {
            m_msg.seq_num ++;
            m_msg.roi_id = i;
            m_msg.mt_type = AM_MD_MOTION_LEVEL_CHANGED;
            m_msg.mt_level = motion_level[i];
            if (!exec_callback_func()) {
              ERROR("m_callback occur error!\n");
              result = false;
              break;
            }
          }
        }
      }
    }
  } while (0);

  return result;
}

void AMMotionDetect::md_main(void *data)
{
  AM_RESULT result = AM_RESULT_OK;;
  INFO("AMMotionDetect::md_main start to run\n");
  if (!data) {
    ERROR("AMMotionDetect::md_main NULL pointer\n");
    return;
  }

  AMMotionDetect *md = (AMMotionDetect*)data;

  while (!md->m_main_loop_exit) {
    switch (md->m_md_buffer.buf_type) {
      case AM_DATA_FRAME_TYPE_YUV:
        if ((result = md->m_video_reader->query_yuv_frame(md->m_frame_desc,
                                            md->m_md_buffer.buf_id,
                                            false)) != AM_RESULT_OK) {
          if (result != AM_RESULT_ERR_AGAIN) {
            ERROR("AMMotionDetect::from buffer %d failed\n",
                  md->m_md_buffer.buf_id);
          }
        }
        break;
      case AM_DATA_FRAME_TYPE_ME0:
        if ((result = md->m_video_reader->query_me0_frame(md->m_frame_desc,
                                            md->m_md_buffer.buf_id,
                                            false)) != AM_RESULT_OK) {
          if (result != AM_RESULT_ERR_AGAIN) {
            ERROR("AMMotionDetect::from buffer %d failed\n",
                  md->m_md_buffer.buf_id);
          }
        }
        break;
      case AM_DATA_FRAME_TYPE_ME1:
        if ((result = md->m_video_reader->query_me1_frame(md->m_frame_desc,
                                            md->m_md_buffer.buf_id,
                                            false)) != AM_RESULT_OK) {
          if (result != AM_RESULT_ERR_AGAIN) {
            ERROR("AMMotionDetect::from buffer %d failed\n",
                  md->m_md_buffer.buf_id);
          }
        }
        break;
      default:
        ERROR("Unknown frame type");
        break;
    }

    if (!md->check_motion(md->m_frame_desc)) {
      ERROR("AMMotionDetect::check_motion error\n");
      break;
    }
  }
}

void AMMotionDetect::print_md_config()
{
  PRINTF("source_buffer_id = %d\n", (uint32_t )m_md_buffer.buf_id);
  PRINTF("source_buffer_type = %d\n", (uint32_t )m_md_buffer.buf_type);
  PRINTF("enable = %s\n", m_enable ? "true" : "false");
  PRINTF("\n\n");
  for (uint32_t i = 0; i < MDET_MAX_ROIS; i ++) {
    PRINTF("threshold info, roi[%d]: %d, %d\n",
           m_threshold[i].roi_id,
           m_threshold[i].threshold[0],
           m_threshold[i].threshold[1]);
  }
  PRINTF("\n\n");
  for (uint32_t i = 0; i < MDET_MAX_ROIS; i ++) {
    PRINTF("level_change delay info, roi[%d]: %d, %d\n",
           m_level_change_delay[i].roi_id,
           m_level_change_delay[i].mt_level_change_delay[0],
           m_level_change_delay[i].mt_level_change_delay[1]);
  }
  PRINTF("\n\n");
  for (uint32_t i = 0; i < MDET_MAX_ROIS; i ++) {
    PRINTF("m_roi_info, roi[%d]: left=%d, right=%d, top=%d, "
           "bottom=%d, valid=%d\n",
           m_roi_info[i].roi_id,
           m_roi_info[i].left,
           m_roi_info[i].right,
           m_roi_info[i].top,
           m_roi_info[i].bottom,
           m_roi_info[i].valid);
  }
}
