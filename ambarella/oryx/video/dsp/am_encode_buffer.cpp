/*******************************************************************************
 * am_encode_buffer.cpp
 *
 * History:
 *   2014-7-15 - [lysun] created file
 *
 * Copyright (C) 2008-2016, Ambarella Co,Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#include <sys/ioctl.h>
#include "am_base_include.h"
#include "am_encode_buffer.h"
#include "am_encode_device.h"
#include "am_log.h"

#include "iav_ioctl.h"

AMEncodeSourceBuffer::AMEncodeSourceBuffer():
    m_state(AM_ENCODE_SOURCE_BUFFER_STATE_OFF),
    m_ref_counter(0),
    m_encode_device(NULL),
    m_id(AM_ENCODE_SOURCE_BUFFER_INVALID),
    m_iav(-1)
{
  m_max_source_buffer_num = AM_ENCODE_SOURCE_BUFFER_MAX_NUM;
  memset(&m_format, 0, sizeof(m_format));
}

AMEncodeSourceBuffer::~AMEncodeSourceBuffer()
{

}

AM_RESULT AMEncodeSourceBuffer::init(AMEncodeDevice *encode_device,
                                     AM_ENCODE_SOURCE_BUFFER_ID id, int iav_hd)
{
  m_encode_device = encode_device;
  m_iav = iav_hd;
  m_id = id;

  return AM_RESULT_OK;
}

static iav_srcbuf_type get_source_buffer_type(AM_ENCODE_SOURCE_BUFFER_TYPE type)
{
  iav_srcbuf_type src_buf_type;
  switch (type)
  {
    case AM_ENCODE_SOURCE_BUFFER_TYPE_ENCODE:
      src_buf_type = IAV_SRCBUF_TYPE_ENCODE;
      break;
    case AM_ENCODE_SOURCE_BUFFER_TYPE_PREVIEW:
      src_buf_type = IAV_SRCBUF_TYPE_PREVIEW;
      break;
    case AM_ENCODE_SOURCE_BUFFER_TYPE_OFF:
    default:
      src_buf_type = IAV_SRCBUF_TYPE_OFF;
      break;
  }
  return src_buf_type;
}

static AM_DSP_STATE get_dsp_state(int fd)
{
  int32_t state;
  AM_DSP_STATE dsp_state = AM_DSP_STATE_ERROR;

  do {
    if (ioctl(fd, IAV_IOC_GET_IAV_STATE, &state) < 0) {
      PERROR("IAV_IOC_GET_IAV_STATE");
      break;
    }
    switch (state) {
      case IAV_STATE_IDLE:
        dsp_state = AM_DSP_STATE_IDLE;
        break;
      case IAV_STATE_PREVIEW:
        dsp_state = AM_DSP_STATE_READY_FOR_ENCODE;
        break;
      case IAV_STATE_ENCODING:
        dsp_state = AM_DSP_STATE_ENCODING;
        break;
      case IAV_STATE_DECODING:
        dsp_state = AM_DSP_STATE_PLAYING;
        break;
      case IAV_STATE_INIT:
        dsp_state = AM_DSP_STATE_NOT_INIT;
        break;
      case IAV_STATE_EXITING_PREVIEW:
        dsp_state = AM_DSP_STATE_EXITING_ENCODING;
        break;
      default:
        dsp_state = AM_DSP_STATE_ERROR;
        break;
    }
  } while (0);

  return dsp_state;
}

AM_RESULT AMEncodeSourceBuffer::setup_source_buffer_all(
                AMEncodeSourceBufferFormatAll *source_buffer_format_all)
{
  int i;
  AM_RESULT result = AM_RESULT_OK;
  struct iav_srcbuf_setup buf_setup;
  do {

    AM_DSP_STATE dsp_state = get_dsp_state(m_iav);
    //always check dsp state, this operation must be done in idle
    if (dsp_state != AM_DSP_STATE_IDLE) {
      ERROR("AMEncodeDevice: cannot setup_source_buffer_all when dsp not idle\n");
      result = AM_RESULT_ERR_PERM;
      break;
    }

    if (!source_buffer_format_all) {
      ERROR("AMEncodeDevice: cannot accept null format \n ");
      result = AM_RESULT_ERR_INVALID;
      break;
    }

    //try to use source buffer setup to setup all source buffers

    memset(&buf_setup, 0, sizeof(buf_setup));
    for (i = 0; i < m_max_source_buffer_num; i ++) {
      //copy input
      buf_setup.input[i].x =
          source_buffer_format_all->source_buffer_format[i].input.x;
      buf_setup.input[i].y =
          source_buffer_format_all->source_buffer_format[i].input.y;

      buf_setup.input[i].width =
          source_buffer_format_all->source_buffer_format[i].input.width;
      buf_setup.input[i].height =
          source_buffer_format_all->source_buffer_format[i].input.height;

      //copy size
      buf_setup.size[i].width =
          source_buffer_format_all->source_buffer_format[i].size.width;
      buf_setup.size[i].height =
          source_buffer_format_all->source_buffer_format[i].size.height;
      buf_setup.type[i] = get_source_buffer_type(source_buffer_format_all->\
                                                 source_buffer_format[i].type);

      buf_setup.unwarp[i] = 0;

      INFO("buf[%d], input x=%d, y=%d, w=%d, y=%d \n",
           i,
           buf_setup.input[i].x,
           buf_setup.input[i].y,
           buf_setup.input[i].width,
           buf_setup.input[i].height);
      INFO("buf[%d], size width = %d, height = %d, type =%d\n",
           i,
           buf_setup.size[i].width,
           buf_setup.size[i].height,
           buf_setup.type[i]);

      m_format.type         = (AM_ENCODE_SOURCE_BUFFER_TYPE)buf_setup.type[i];
      m_format.input.x      = buf_setup.input[i].x;
      m_format.input.y      = buf_setup.input[i].y;
      m_format.input.width  = buf_setup.input[i].width;
      m_format.input.height = buf_setup.input[i].height;
      m_format.size.width   = buf_setup.size[i].width;
      m_format.size.height  = buf_setup.size[i].height;
    }

    PRINTF("AMEncodeDevice: now setup all source buffers \n");

    if (ioctl(m_iav, IAV_IOC_SET_SOURCE_BUFFER_SETUP, &buf_setup) < 0) {
      PERROR("IAV_IOC_SET_SOURCE_BUFFER_SETUP");
      result = AM_RESULT_ERR_DSP;
      break;
    }
  } while (0);

  return result;
}

AM_RESULT AMEncodeSourceBuffer::set_source_buffer_format(AMEncodeSourceBufferFormat
                                                         *buffer_format)
{
  struct iav_srcbuf_format format;
  AM_RESULT result = AM_RESULT_OK;
  do {
    if (!buffer_format) {
      result = AM_RESULT_ERR_INVALID;
      break;
    }

    memset(&format, 0, sizeof(format));
    format.buf_id = m_id;

    format.input.width = buffer_format->input.width;
    format.input.height = buffer_format->input.height;
    format.input.x = buffer_format->input.x;
    format.input.y = buffer_format->input.y;
    format.size.width = buffer_format->size.width;
    format.size.height = buffer_format->size.height;

    if (ioctl(m_iav, IAV_IOC_SET_SOURCE_BUFFER_FORMAT, &format) < 0) {
      PERROR("IAV_IOC_SET_SOURCE_BUFFER_FORMAT");
      result = AM_RESULT_ERR_DSP;
      break;
    }

    //back up buffer format
    memcpy(&m_format, buffer_format, sizeof(m_format));
  } while (0);

  return result;
}

AM_RESULT AMEncodeSourceBuffer::get_source_buffer_format(AMEncodeSourceBufferFormat
                                                         *buffer_format)
{
  AM_RESULT result = AM_RESULT_OK;

  do {
    struct iav_srcbuf_format format;
    if (!buffer_format) {
      result = AM_RESULT_ERR_INVALID;
      break;
    }
    memset(&format, 0, sizeof(format));
    format.buf_id = m_id;
    if (ioctl(m_iav, IAV_IOC_GET_SOURCE_BUFFER_FORMAT, &format) < 0) {
      PERROR("IAV_IOC_GET_SOURCE_BUFFER_FORMAT");
      result = AM_RESULT_ERR_DSP;
      break;
    }

    buffer_format->input.width = format.input.width;
    buffer_format->input.height = format.input.height;
    buffer_format->input.x = format.input.x;
    buffer_format->input.y = format.input.y;
    buffer_format->size.width = format.size.width;
    buffer_format->size.height = format.size.height;

    //back up buffer format
    memcpy(&m_format, buffer_format, sizeof(m_format));
  } while (0);

  return result;
}

AM_RESULT AMEncodeSourceBuffer::IncRef()
{
  return AM_RESULT_OK;
}

AM_RESULT AMEncodeSourceBuffer::DecRef()
{
  return AM_RESULT_OK;
}
