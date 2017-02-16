/*******************************************************************************
 * am_jpeg_encoder.cpp
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
#include <signal.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <jpeglib.h>
#include <mutex>          // std::mutex

#include "am_base_include.h"
#include "am_log.h"
#include "am_define.h"
#include "am_jpeg_encoder.h"

#define COLOR_COMPONENTS       (3)
#define OUTPUT_BUF_SIZE        (4096)
#define DEFAULT_JPEG_QUALITY   (60)

static std::mutex m_mtx;
#define  DECLARE_MUTEX  std::lock_guard<std::mutex> lck (m_mtx);

typedef struct chroma_neon_arg {
  uint8_t *in;
  uint8_t *u;
  uint8_t *v;
  int row;
  int col;
  int pitch;
} chroma_neon_arg;


extern "C" void chrome_convert(struct chroma_neon_arg *);

AMJpegEncoder *AMJpegEncoder::m_instance = NULL;

enum AM_ENCODE_CHROMA_FORMAT
{
  AM_ENCODE_CHROMA_FORMAT_YUV420 = 0,
  AM_ENCODE_CHROMA_FORMAT_YUV422 = 1,
  AM_ENCODE_CHROMA_FORMAT_Y8 = 2, //gray scale (or monochrome)
};

typedef struct {
  struct jpeg_destination_mgr pub; /* public fields */
  JOCTET * buffer;    /* start of buffer */
  unsigned char *outbuffer;
  int outbuffer_size;
  unsigned char *outbuffer_cursor;
  int *written;
} my_destmgr;

static void init_destination(j_compress_ptr cinfo)
{
  my_destmgr * dest = (my_destmgr*) cinfo->dest;

  /* Allocate the output buffer --- it will be released when done with
   * image */
  dest->buffer = (JOCTET *)(*cinfo->mem->alloc_small) ((j_common_ptr) cinfo,
                 JPOOL_IMAGE, OUTPUT_BUF_SIZE * sizeof(JOCTET));

  *(dest->written) = 0;

  dest->pub.next_output_byte = dest->buffer;
  dest->pub.free_in_buffer = OUTPUT_BUF_SIZE;
}

static boolean empty_output_buffer(j_compress_ptr cinfo)
{
  my_destmgr *dest = (my_destmgr *) cinfo->dest;

  memcpy(dest->outbuffer_cursor, dest->buffer, OUTPUT_BUF_SIZE);
  dest->outbuffer_cursor += OUTPUT_BUF_SIZE;
  *(dest->written) += OUTPUT_BUF_SIZE;

  dest->pub.next_output_byte = dest->buffer;
  dest->pub.free_in_buffer = OUTPUT_BUF_SIZE;

  return TRUE;
}

static void term_destination(j_compress_ptr cinfo)
{
  my_destmgr * dest = (my_destmgr *) cinfo->dest;
  size_t datacount = OUTPUT_BUF_SIZE - dest->pub.free_in_buffer;

  memcpy(dest->outbuffer_cursor, dest->buffer, datacount);
  dest->outbuffer_cursor += datacount;
  *(dest->written) += datacount;
}

static void dest_buffer(j_compress_ptr cinfo, unsigned char *buffer, int size, int *written)
{
  my_destmgr * dest;

  if (cinfo->dest == NULL) {
    cinfo->dest = (struct jpeg_destination_mgr *)(*cinfo->mem->alloc_small)
                   ((j_common_ptr) cinfo, JPOOL_PERMANENT, sizeof(my_destmgr));
  }

  dest = (my_destmgr*) cinfo->dest;
  dest->pub.init_destination = init_destination;
  dest->pub.empty_output_buffer = empty_output_buffer;
  dest->pub.term_destination = term_destination;
  dest->outbuffer = buffer;
  dest->outbuffer_size = size;
  dest->outbuffer_cursor = buffer;
  dest->written = written;
}

AMJpegEncoder::AMJpegEncoder():
  m_ref_counter(0),
  m_quality(DEFAULT_JPEG_QUALITY)
{
  memset(&m_info, 0, sizeof(m_info));
  memset(&m_err, 0, sizeof(m_err));
}

AMJpegEncoder::~AMJpegEncoder()
{
  m_instance = NULL;
}

int AMJpegEncoder::create()
{
  m_info.err = jpeg_std_error(&m_err);
  jpeg_create_compress(&m_info);
  jpeg_set_quality(&m_info, m_quality, TRUE);

  return 0;
}

void AMJpegEncoder::destroy()
{
  jpeg_destroy_compress(&m_info);
}

AMIJpegEncoderPtr AMIJpegEncoder::get_instance()
{
  return AMJpegEncoder::get_instance();
}

AMJpegEncoder *AMJpegEncoder::get_instance()
{
  DECLARE_MUTEX;
  if (!m_instance) {
    m_instance = new AMJpegEncoder();
  }
  return m_instance;
}

void AMJpegEncoder::inc_ref()
{
  ++ m_ref_counter;
}

void AMJpegEncoder::release()
{
  DECLARE_MUTEX;
  DEBUG("AMJpegEncode release is called!");
  if ((m_ref_counter >= 0) && (--m_ref_counter <= 0)) {
    DEBUG("This is the last reference of AMJpegEncode's object, "
          "delete object instance %p", m_instance);
    delete m_instance;
    m_instance = NULL;
  }
}

AMJpegData *AMJpegEncoder::new_jpeg_data(int width, int height)
{
  if (width == 0 || height == 0) {
    ERROR("width or height can't be zeor\n");
    return NULL;
  }
  AMJpegData *jpeg = (AMJpegData *)calloc(1, sizeof(AMJpegData));
  if (!jpeg) {
    ERROR("malloc jpeg failed!\n");
    return NULL;
  }
  jpeg->data.iov_len = width * height * COLOR_COMPONENTS;
  jpeg->data.iov_base = (uint8_t *)calloc(1, jpeg->data.iov_len);
  if (!jpeg->data.iov_base) {
    ERROR("Not enough memory for jpeg buffer.\n");
    return NULL;
  }
  jpeg->color_componenets = COLOR_COMPONENTS;
  return jpeg;
}

void AMJpegEncoder::free_jpeg_data(AMJpegData *jpeg)
{
  if (!jpeg) {
    return;
  }
  free(jpeg->data.iov_base);
  free(jpeg);
}

int AMJpegEncoder::encode_yuv(AMYUVData *input, AMJpegData *output)
{
  uint32_t size = input->width * input->height;
  uint32_t quarter_size = size / 4;

  JSAMPROW y[16];
  JSAMPROW cb[16];
  JSAMPROW cr[16];
  JSAMPARRAY planes[3];

  planes[0] = y;
  planes[1] = cb;
  planes[2] = cr;
  uint8_t *yuv = (uint8_t *)calloc(1, input->y.iov_len + input->uv.iov_len);
  if (!yuv) {
    ERROR("malloc yuv buffer failed!\n");
    return -1;
  }
  uint8_t *y_offset = yuv;
  uint8_t *uv_offset = yuv + input->y.iov_len;
  chroma_neon_arg chroma;
  memcpy(y_offset, input->y.iov_base, input->y.iov_len);
  memcpy(uv_offset, input->uv.iov_base, input->uv.iov_len);

  if (input->format == AM_ENCODE_CHROMA_FORMAT_YUV420) {
    chroma.in = (uint8_t *)input->uv.iov_base;
    chroma.row = input->height/2;
    chroma.col = input->width;
    chroma.pitch = input->pitch;
    chroma.u = yuv + input->y.iov_len;
    chroma.v = chroma.u + (input->width/2) * (input->height/2);
    chrome_convert(&chroma);
  } else {
    ERROR("not support chrome_convert\n");
  }

  dest_buffer(&m_info, (uint8_t *)output->data.iov_base, size, (int *)&output->data.iov_len);

  m_info.image_width = input->width;/* image width and height, in pixels */
  m_info.image_height = input->height;
  m_info.input_components = COLOR_COMPONENTS;/* # of color components per pixel */
  m_info.in_color_space = JCS_YCbCr;       /* colorspace of input image */

  jpeg_set_defaults(&m_info);
  m_info.raw_data_in = TRUE;// supply downsampled data
  m_info.dct_method = JDCT_IFAST;

  m_info.comp_info[0].h_samp_factor = 2;
  m_info.comp_info[0].v_samp_factor = 2;
  m_info.comp_info[1].h_samp_factor = 1;
  m_info.comp_info[1].v_samp_factor = 1;
  m_info.comp_info[2].h_samp_factor = 1;
  m_info.comp_info[2].v_samp_factor = 1;

  jpeg_start_compress(&m_info, TRUE);

  for (int j = 0; j < input->height; j += 16) {
    for (int i = 0; i < 16; i++) {
      int row = input->width * (i + j);
      y[i] = yuv + row;
      if (i % 2 == 0) {
        cb[i/2] = yuv + size + row/4;
        cr[i/2] = yuv + size + quarter_size + row/4;
      }
    }
    jpeg_write_raw_data(&m_info, planes, 16);
  }
  jpeg_finish_compress(&m_info);
  free(yuv);
  return 0;
}


int AMJpegEncoder::set_quality(int quality)
{
  m_quality = quality;
  return 0;
}

int AMJpegEncoder::get_quality()
{
  return m_quality;
}
