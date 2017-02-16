/*******************************************************************************
 * am_video_reader.cpp
 *
 * History:
 *   2014-8-25 - [lysun] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co,Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "am_base_include.h"
#include "am_video_reader.h"
#include "am_log.h"
#include "iav_ioctl.h"

#include <mutex>  //using C++11 mutex
//C++ 11 mutex (using non-recursive type)

static std::mutex m_mtx;
#define  DECLARE_MUTEX  std::lock_guard<std::mutex> lck (m_mtx);

AMVideoReader *AMVideoReader::m_instance = NULL;

AMVideoReader::AMVideoReader() :
    m_iav(-1), //iav file handle
    m_ref_counter(0),
    m_init_ready(false),
    m_is_dsp_mapped(false),
    m_is_bsb_mapped(false)
{
  memset(&m_dsp_mem, 0, sizeof(m_dsp_mem));
  memset(&m_bsb_mem, 0, sizeof(m_bsb_mem));

  for (uint32_t i = 0; i < AM_STREAM_MAX_NUM; ++ i) {
    m_stream_session_changed[i] = false;
    m_stream_session_id[i]      = 0;
  }
}

AMVideoReader::~AMVideoReader()
{
  destroy();
  m_instance = NULL;
  DEBUG("~AMVideoReader");
}

AMIVideoReaderPtr AMIVideoReader::get_instance()
{
  return AMVideoReader::get_instance();
}

AMVideoReader* AMVideoReader::get_instance()
{
  DECLARE_MUTEX;
  if (!m_instance) {
    m_instance = new AMVideoReader();
  }
  return m_instance;
}

void AMVideoReader::inc_ref()
{
  ++ m_ref_counter;
}

void AMVideoReader::release()
{
  DECLARE_MUTEX;
  DEBUG("AMVideoReader release is called!");
  if ((m_ref_counter >= 0) && (--m_ref_counter <= 0)) {
    DEBUG("This is the last reference of AMVideoReader's object, "
          "delete object instance %p", m_instance);
    delete m_instance;
    m_instance = NULL;
  }
}

AM_RESULT AMVideoReader::init()
{
  return init(true, true);
}

AM_RESULT AMVideoReader::init(bool dump_bsb, bool dump_dsp)
{
  DECLARE_MUTEX;
  AM_RESULT result = AM_RESULT_OK;
  do {

    INFO("AMVideoReader: init \n");
    if (m_init_ready) {
      NOTICE("AMVideoReader: already initialized!\n");
      break;
    }

    if (m_iav >= 0) {
      ERROR("AMVideoReader: iav is already open, strange \n");
      result = AM_RESULT_ERR_PERM;
      break;
    }

    m_iav = open("/dev/iav", O_RDWR, 0);
    if (m_iav < 0) {
      PERROR("/dev/iav");
      result = AM_RESULT_ERR_IO;
      break;
    }

    if (dump_bsb) {
      result = map_bsb();
      if (result != AM_RESULT_OK) {
        ERROR("AMVideoReader: map bsb error \n");
        break;
      }
    }

    if (dump_dsp) {
      result = map_dsp();
      if (result != AM_RESULT_OK) {
        ERROR("AMVideoReader: map dsp error \n");
        break;
      }
    }

    m_init_ready = true;
  } while (0);

  return result;
}

//use local version of destroy, but do not use AMVideoDevice's destory
//because AMVideoReader's destroy will not change DSP state, it only
//close iav handle and unmap
AM_RESULT AMVideoReader::destroy()
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (!m_init_ready)
      break;

    INFO("unmap mem\n");
    //unmap mems
    unmap_dsp();
    unmap_bsb();
    //close iav handle
    if (m_iav > 0) {
      close(m_iav);
      m_iav = -1;
    }
    m_init_ready = false;
  } while (0);
  return result;
}

static AM_VIDEO_FRAME_TYPE iav_pic_type_to_video_frame_type(uint32_t iav_pic_type)
{
  AM_VIDEO_FRAME_TYPE  frame_type;
  switch (iav_pic_type)
  {
    case IAV_PIC_TYPE_MJPEG_FRAME:
      frame_type = AM_VIDEO_FRAME_TYPE_MJPEG;
      break;
    case IAV_PIC_TYPE_IDR_FRAME:
      frame_type = AM_VIDEO_FRAME_TYPE_H264_IDR;
      break;
    case IAV_PIC_TYPE_I_FRAME:
      frame_type = AM_VIDEO_FRAME_TYPE_H264_I;
      break;
    case IAV_PIC_TYPE_P_FRAME:
    case IAV_PIC_TYPE_P_FAST_SEEK_FRAME:
      frame_type = AM_VIDEO_FRAME_TYPE_H264_P;
      break;
    case IAV_PIC_TYPE_B_FRAME:
      frame_type = AM_VIDEO_FRAME_TYPE_H264_B;
      break;
    default:
      ERROR("VIDEO FRAME TYPE %d not supported\n ", iav_pic_type);
      frame_type = AM_VIDEO_FRAME_TYPE_NONE;
      break;
  }
  return frame_type;
}


AM_RESULT AMVideoReader::query_video_frame(AMQueryDataFrameDesc *frame_desc,
                                           uint32_t timeout)
{
  DECLARE_MUTEX;
  struct iav_querydesc query_desc;
  struct iav_framedesc *iav_frame;
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (!m_init_ready) {
      result = AM_RESULT_ERR_PERM;
      break;
    }

    if (!frame_desc) {
      result = AM_RESULT_ERR_INVALID;
      break;
    }

    //IAV API
    memset(&query_desc, 0, sizeof(query_desc));
    query_desc.qid = IAV_DESC_FRAME;
    query_desc.arg.frame.id = ((uint32_t)-1); /* Query All Streams */
    query_desc.arg.frame.time_ms = timeout;
    if (ioctl(m_iav, IAV_IOC_QUERY_DESC, &query_desc) < 0) {
      if (errno == EINTR) {
        INFO("AMVideoReader interrupted \n");
        result = AM_RESULT_ERR_IO;
      } else if (errno == EAGAIN){
        result = AM_RESULT_ERR_AGAIN;
      }else {
        PERROR("IAV_IOC_QUERY_DESC");
        result = AM_RESULT_ERR_DSP;
      }
      break;
    }
    iav_frame = &query_desc.arg.frame;

    memset(frame_desc, 0, sizeof(*frame_desc));
    frame_desc->frame_type = AM_DATA_FRAME_TYPE_VIDEO;
    frame_desc->video.video_type = iav_pic_type_to_video_frame_type(iav_frame->pic_type);
    frame_desc->video.data_addr_offset = iav_frame->data_addr_offset;
    frame_desc->video.data_size = iav_frame->size;
    frame_desc->video.frame_num = iav_frame->frame_num;
    frame_desc->video.stream_id = iav_frame->id;
    frame_desc->video.width = iav_frame->reso.width;
    frame_desc->video.height = iav_frame->reso.height;
    frame_desc->video.jpeg_quality = iav_frame->jpeg_quality;
    frame_desc->video.session_id = iav_frame->session_id;
    frame_desc->video.stream_end_flag = iav_frame->stream_end;
    frame_desc->mono_pts = iav_frame->arm_pts;

    //check result of stream_id to prevent array exceed range
    if (frame_desc->video.stream_id >= AM_STREAM_MAX_NUM) {
      ERROR("frame desc reported stream id incorrect! \n");
      result = AM_RESULT_ERR_DSP;
      break;
    }

    //update session management, session id can be used and realized by
    //reader to create/close files
    if (m_stream_session_id[frame_desc->video.stream_id]!=
        frame_desc->video.session_id) {
      m_stream_session_changed[frame_desc->video.stream_id] = true;
      //update session id
      m_stream_session_id[frame_desc->video.stream_id] = frame_desc->video.session_id;
    } else {
      m_stream_session_changed[frame_desc->video.stream_id] = false;
    }
  } while (0);
  return result;
}

static AM_ENCODE_CHROMA_FORMAT iav_yuv_format_to_chroma_format(uint32_t iav_yuv_format)
{
  AM_ENCODE_CHROMA_FORMAT chroma = AM_ENCODE_CHROMA_FORMAT_YUV420;
  switch (iav_yuv_format)
  {
    case IAV_YUV_FORMAT_YUV422:
      chroma = AM_ENCODE_CHROMA_FORMAT_YUV422;
      break;
    case IAV_YUV_FORMAT_YUV420:
      chroma = AM_ENCODE_CHROMA_FORMAT_YUV420;
      break;
    case IAV_YUV_FORMAT_YUV400:
      chroma = AM_ENCODE_CHROMA_FORMAT_Y8;
      break;
    default:
      ERROR("iav yuv format %d unrecognized \n", iav_yuv_format);
      break;
  }
  return chroma;
}

AM_RESULT AMVideoReader::query_yuv_frame(AMQueryDataFrameDesc *frame_desc,
                                         AM_ENCODE_SOURCE_BUFFER_ID buffer_id,
                                         bool latest)
{
  DECLARE_MUTEX;
  struct iav_querydesc query_desc;
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (!m_init_ready) {
      result = AM_RESULT_ERR_PERM;
      break;
    }

    if (!frame_desc) {
      result = AM_RESULT_ERR_INVALID;
      break;
    }

    memset(&query_desc, 0, sizeof(query_desc));

    query_desc.qid = IAV_DESC_YUV;
    query_desc.arg.yuv.buf_id = iav_srcbuf_id( (int) buffer_id);
    if (latest) {
      query_desc.arg.yuv.flag |= IAV_BUFCAP_NONBLOCK;
    } else {
      query_desc.arg.yuv.flag &= ~IAV_BUFCAP_NONBLOCK;  //be block read
    }

    if (ioctl(m_iav, IAV_IOC_QUERY_DESC, &query_desc) < 0) {
      if (errno == EINTR) {
        INFO("AMVideoReader interrupted \n");
        result = AM_RESULT_ERR_IO;
      } else {
        result = AM_RESULT_ERR_DSP;
        PERROR("IAV_IOC_QUERY_DESC");
      }
      break;
    }

    memset(frame_desc, 0, sizeof(*frame_desc));

    frame_desc->frame_type = AM_DATA_FRAME_TYPE_YUV;
    frame_desc->mono_pts = query_desc.arg.yuv.mono_pts;
    frame_desc->yuv.non_block_flag = 0;
    frame_desc->yuv.buffer_id = AM_ENCODE_SOURCE_BUFFER_ID((int)query_desc.arg.yuv.buf_id);
    frame_desc->yuv.format = iav_yuv_format_to_chroma_format(query_desc.arg.yuv.format);
    frame_desc->yuv.height = query_desc.arg.yuv.height;
    frame_desc->yuv.width = query_desc.arg.yuv.width;
    frame_desc->yuv.pitch = query_desc.arg.yuv.pitch;
    frame_desc->yuv.seq_num = query_desc.arg.yuv.seq_num;
    frame_desc->yuv.uv_addr_offset = query_desc.arg.yuv.uv_addr_offset;
    frame_desc->yuv.y_addr_offset = query_desc.arg.yuv.y_addr_offset;

    //TODO: add ioctl to query
    break;
  } while (0);
  return result;
}

AM_RESULT AMVideoReader::query_luma_frame(AMQueryDataFrameDesc *frame_desc,
                                          AM_ENCODE_SOURCE_BUFFER_ID buffer_id, bool latest)
{
  DECLARE_MUTEX;
  struct iav_querydesc query_desc;
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (!m_init_ready) {
      result = AM_RESULT_ERR_PERM;
      break;
    }

    if (!frame_desc) {
      result = AM_RESULT_ERR_INVALID;
      break;
    }

    memset(&query_desc, 0, sizeof(query_desc));
    query_desc.qid = IAV_DESC_ME1;
    query_desc.arg.me1.buf_id = iav_srcbuf_id((int)buffer_id);
    if (latest) {
      query_desc.arg.me1.flag |= IAV_BUFCAP_NONBLOCK;
    } else {
      query_desc.arg.me1.flag &= ~IAV_BUFCAP_NONBLOCK;  //be block read
    }

    if (ioctl(m_iav, IAV_IOC_QUERY_DESC, &query_desc) < 0) {
      if (errno == EINTR) {
        INFO("AMVideoReader interrupted \n");
        result = AM_RESULT_ERR_IO;
      } else {
        result = AM_RESULT_ERR_DSP;
        PERROR("IAV_IOC_QUERY_DESC");
      }
      break;
    }

    memset(frame_desc, 0, sizeof(*frame_desc));

    frame_desc->frame_type = AM_DATA_FRAME_TYPE_LUMA;
    frame_desc->mono_pts = query_desc.arg.me1.mono_pts;
    frame_desc->luma.non_block_flag = 0;
    frame_desc->luma.buffer_id =
        AM_ENCODE_SOURCE_BUFFER_ID((int) query_desc.arg.me1.buf_id);
    frame_desc->luma.data_addr_offset = query_desc.arg.me1.data_addr_offset;
    frame_desc->luma.height = query_desc.arg.me1.height;
    frame_desc->luma.width = query_desc.arg.me1.width;
    frame_desc->luma.pitch = query_desc.arg.me1.pitch;
    frame_desc->luma.seq_num = query_desc.arg.me1.seq_num;

    //TODO: add ioctl to query
    break;
  } while (0);
  return result;
}

AM_RESULT AMVideoReader::query_bayer_raw_frame(AMQueryDataFrameDesc *frame_desc)
{
  DECLARE_MUTEX;
  struct iav_querydesc query_desc;
  struct iav_rawbufdesc *raw_desc;
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (!m_init_ready) {
      result = AM_RESULT_ERR_PERM;
      break;
    }

    if (!frame_desc) {
      result = AM_RESULT_ERR_INVALID;
      break;
    }

    memset(&query_desc, 0, sizeof(query_desc));
    query_desc.qid = IAV_DESC_RAW;
    query_desc.arg.raw.flag &= ~IAV_BUFCAP_NONBLOCK;  //be block read

    if (ioctl(m_iav, IAV_IOC_QUERY_DESC, &query_desc) < 0) {
      if (errno == EINTR) {
        INFO("AMVideoReader interrupted \n");
        result = AM_RESULT_ERR_IO;
        break;
      } else {
        PERROR("IAV_IOC_QUERY_DESC");
        result = AM_RESULT_ERR_DSP;
        break;
      }
    }

    raw_desc = &query_desc.arg.raw;
    memset(frame_desc, 0, sizeof(*frame_desc));
    frame_desc->frame_type = AM_DATA_FRAME_TYPE_BAYER_PATTERN_RAW;
    frame_desc->bayer.width = raw_desc->width;
    frame_desc->bayer.height = raw_desc->height;
    frame_desc->bayer.pitch  = raw_desc->pitch;
    frame_desc->bayer.data_addr_offset = raw_desc->raw_addr_offset;
    frame_desc->mono_pts = raw_desc->mono_pts;

    //TODO: add ioctl to query
    break;
  } while (0);
  return result;
}

AM_RESULT AMVideoReader::query_stream_info(AMStreamInfo *stream_info)
{
  AM_RESULT result = AM_RESULT_OK;
  struct iav_stream_format stream_fmt = {0};
  struct iav_h264_cfg h264_cfg = {0};
  struct iav_stream_cfg stream_cfg = {0};

  do {
    if (!m_init_ready) {
      result = AM_RESULT_ERR_PERM;
      break;
    }

    if (!stream_info) {
      result = AM_RESULT_ERR_INVALID;
      break;
    }

    stream_fmt.id = stream_info->stream_id;
    if (ioctl(m_iav, IAV_IOC_GET_STREAM_FORMAT, &stream_fmt) < 0) {
      PERROR("IAV_IOC_GET_STREAM_FORMAT");
      result = AM_RESULT_ERR_DSP;
      break;
    }

    if (stream_fmt.type == IAV_STREAM_TYPE_NONE) {
      memset(stream_info, 0, sizeof(AMStreamInfo));
      stream_info->stream_id = AM_STREAM_ID(stream_fmt.id);
      break;
    }

    stream_cfg.id = stream_info->stream_id;
    stream_cfg.cid = IAV_STMCFG_FPS;
    if (ioctl(m_iav, IAV_IOC_GET_STREAM_CONFIG, &stream_cfg) < 0) {
      PERROR("IAV_IOC_GET_STREAM_CONFIG");
      result = AM_RESULT_ERR_DSP;
      break;
    }
    stream_info->mul = stream_cfg.arg.fps.fps_multi;
    stream_info->div = stream_cfg.arg.fps.fps_div;

    if (stream_fmt.type == IAV_STREAM_TYPE_H264) {
      h264_cfg.id = stream_info->stream_id;
      if (ioctl(m_iav, IAV_IOC_GET_H264_CONFIG, &h264_cfg) < 0) {
        PERROR("IAV_IOC_GET_H264_CONFIG");
        result = AM_RESULT_ERR_DSP;
        break;
      }
      stream_info->m = h264_cfg.M;
      stream_info->n = h264_cfg.N;
      stream_info->rate = h264_cfg.pic_info.rate;
      stream_info->scale = h264_cfg.pic_info.scale;
    } else {
      stream_info->m = 0;
      stream_info->n = 0;
      stream_info->rate = 0;
      stream_info->scale = 0;
    }
  } while (0);
  return result;
}

AM_RESULT AMVideoReader::map_dsp()
{
  AM_RESULT result = AM_RESULT_OK;
  struct iav_querybuf querybuf;
  uint32_t dsp_size;
  void *dsp_mem;

  do {
    if (m_init_ready) {
      ERROR("AMVideoReader: should not call map_dsp again after init done\n");
      result = AM_RESULT_ERR_PERM;
      break;
    }

    querybuf.buf = IAV_BUFFER_DSP;
    if (ioctl(m_iav, IAV_IOC_QUERY_BUF, &querybuf) < 0) {
      PERROR("IAV_IOC_QUERY_BUF");
      result = AM_RESULT_ERR_DSP;
      break;
    }
    dsp_size = querybuf.length;
    dsp_mem = mmap(NULL, dsp_size,
                   PROT_READ,
                   MAP_SHARED, m_iav, querybuf.offset);
    if (dsp_mem == MAP_FAILED) {
      PERROR("mmap (%d) failed: %s\n");
      result = AM_RESULT_ERR_IO;
      break;
    }
    m_dsp_mem.addr = (uint8_t *) dsp_mem;
    m_dsp_mem.length = dsp_size;
    m_is_dsp_mapped = true;

    INFO("dsp_mem = 0x%x, size = 0x%x\n", (uint32_t ) dsp_mem, dsp_size);
  } while (0);

  return result;
}

AM_RESULT AMVideoReader::unmap_dsp()
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (!m_init_ready) {
      result = AM_RESULT_ERR_PERM;
      break;
    }

    if (m_dsp_mem.addr) {
      if (munmap(m_dsp_mem.addr, m_dsp_mem.length) < 0) {
        PERROR("AMVideoDevice: unmap_dsp \n");
        result = AM_RESULT_ERR_MEM;
        break;
      }
      memset(&m_dsp_mem, 0, sizeof(m_dsp_mem));
      m_is_bsb_mapped = false;
    }
  } while (0);
  return result;
}

//map_dsp is needed when reading out encoded video data
AM_RESULT AMVideoReader::map_bsb()
{
  struct iav_querybuf querybuf;
  AM_RESULT result = AM_RESULT_OK;
  uint32_t bsb_size;
  void *bsb_mem = NULL;

  do {
    if (m_init_ready) {
      ERROR("AMVideoReader: should not call map_bsb again after init done\n");
      result = AM_RESULT_ERR_PERM;
      break;
    }

    querybuf.buf = IAV_BUFFER_BSB;
    if (ioctl(m_iav, IAV_IOC_QUERY_BUF, &querybuf) < 0) {
      PERROR("IAV_IOC_QUERY_BUF");
      result = AM_RESULT_ERR_DSP;
      break;
    }
    bsb_size = querybuf.length;
    bsb_mem = mmap(NULL, bsb_size * 2,
                   PROT_READ,
                   MAP_SHARED, m_iav, querybuf.offset);
    if (bsb_mem == MAP_FAILED) {
      PERROR("mmap (%d) failed: %s\n");
      result = AM_RESULT_ERR_IO;
      break;

    }
    m_bsb_mem.addr = (uint8_t *)bsb_mem;
    m_bsb_mem.length = bsb_size;
    m_is_bsb_mapped = true;

    INFO("bsb_mem = 0x%x, size = 0x%x\n", (uint32_t ) bsb_mem, bsb_size);

  } while (0);

  return result;
}

AM_RESULT AMVideoReader::unmap_bsb()
{
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (!m_init_ready) {
      result = AM_RESULT_ERR_PERM;
      break;
    }

    if (m_bsb_mem.addr) {
      if (munmap(m_bsb_mem.addr, m_bsb_mem.length) < 0) {
        PERROR("AMVideoDevice: unmap_bsb \n");
        result = AM_RESULT_ERR_MEM;
        break;
      }
      memset(&m_bsb_mem, 0, sizeof(m_bsb_mem));
      m_is_bsb_mapped = false;
    }
  } while (0);
  return result;
}

AM_RESULT AMVideoReader::get_dsp_mem(AMMemMapInfo *dsp_mem)
{
  DECLARE_MUTEX;
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (!dsp_mem) {
      result = AM_RESULT_ERR_INVALID;
      break;
    }

    if (!m_is_dsp_mapped) {
      result = AM_RESULT_ERR_PERM;
      ERROR("AMVideoReader: dsp mem not mapped yet\n");
      break;
    }

    *dsp_mem = m_dsp_mem;
  } while (0);

  return result;
}

AM_RESULT AMVideoReader::get_bsb_mem(AMMemMapInfo *bsb_mem)
{
  DECLARE_MUTEX;
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (!bsb_mem) {
      result = AM_RESULT_ERR_INVALID;
      break;
    }

    if (!m_is_bsb_mapped) {
      result = AM_RESULT_ERR_PERM;
      ERROR("AMVideoReader: bsb mem not mapped yet\n");
      break;
    }

    *bsb_mem = m_bsb_mem;
  } while (0);

  return result;
}
