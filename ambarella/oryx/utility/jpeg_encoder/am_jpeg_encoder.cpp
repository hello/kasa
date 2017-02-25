/*******************************************************************************
 * am_jpeg_encoder.cpp
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
#include "am_jpeg_encoder.h"

#include <jpeglib.h>
#include "turbojpeg.h"
#include <mutex>          // std::mutex
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

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

AMJpegEncoder *AMJpegEncoder::m_instance = nullptr;

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

  if (cinfo->dest == nullptr) {
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
  m_instance = nullptr;
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
    m_instance = nullptr;
  }
}

AMJpegData *AMJpegEncoder::new_jpeg_data(int width, int height)
{
  if (width == 0 || height == 0) {
    ERROR("width or height can't be zero\n");
    return nullptr;
  }
  AMJpegData *jpeg = (AMJpegData *)calloc(1, sizeof(AMJpegData));
  if (!jpeg) {
    ERROR("malloc jpeg failed!\n");
    return nullptr;
  }
  jpeg->data.iov_len = width * height * COLOR_COMPONENTS;
  jpeg->data.iov_base = (uint8_t *)calloc(1, jpeg->data.iov_len);
  if (!jpeg->data.iov_base) {
    ERROR("Not enough memory for jpeg buffer.\n");
    free(jpeg);
    return nullptr;
  }
  jpeg->color_componenets = COLOR_COMPONENTS;
  jpeg->width = width;
  jpeg->height = height;
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
  int ret = 0;
  uint32_t size = input->width * input->height;
  uint32_t quarter_size = size / 4;
  uint8_t *yuv = nullptr;
  int32_t uv_height;
  chroma_neon_arg chroma;

  JSAMPROW y[16];
  JSAMPROW cb[16];
  JSAMPROW cr[16];
  JSAMPARRAY planes[3];

  planes[0] = y;
  planes[1] = cb;
  planes[2] = cr;

  do {
    if (input->format == AM_ENCODE_CHROMA_FORMAT_YUV420) {
      uv_height = input->height/2;
    } else {
      uv_height = input->height;
    }
    int y_len = input->width * input->height;
    int uv_len = input->pitch * uv_height;
    yuv = (uint8_t *)calloc(1, y_len + uv_len);
    if (!yuv) {
      ERROR("malloc yuv buffer failed!\n");
      ret = -1;
      break;
    }
    uint8_t *y_offset = yuv;
    uint8_t *uv_offset = yuv + y_len;

    //copy y buffer
    uint8_t *y_tmp = (uint8_t *)y_offset;
    if (input->pitch == input->width) {
      memcpy(y_offset, input->y_addr, y_len);
    } else {
      for (int i = 0; i < input->height ; i++) {
        memcpy(y_tmp, input->y_addr + i * input->pitch, input->width);
        y_tmp += input->width;
      }
    }

    //no need copy, just convert uv data
    if (input->format == AM_ENCODE_CHROMA_FORMAT_YUV420) {
      chroma.in = input->uv_addr;
      chroma.row = input->height/2;
      chroma.col = input->width;
      chroma.pitch = input->pitch;
      chroma.u = uv_offset;
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
  } while (0);

  return ret;
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

int AMJpegEncoder::gen_thumbnail(const char *outpath, const char *inpath,
                                 int scale)
{
  int width_org, height_org, sub_samp, color_space;
  int width = 0;
  int height = 0;
  uint32_t inlen;
  int ret = 0;
  uint8_t *indata = nullptr;
  uint8_t *buffer = nullptr;
  uint8_t *thumbnail = nullptr;
  unsigned long thumbnail_size = 0;
  do {
    if (!outpath || !inpath) {
      ERROR("invalid file path");
      ret = -1;
      break;
    }
    if (scale != 1 && scale != 2 && scale != 4 && scale != 8) {
      ERROR("scale only can be 1/2/4/8");
      ret = -1;
      break;
    }
    AMFile infile(inpath);
    infile.open(AMFile::AM_FILE_READONLY);
    inlen = infile.size();

    indata = (uint8_t *)calloc(1, inlen);
    if (!indata) {
      ERROR("calloc %d failed", inlen);
      ret = -1;
      break;
    }
    infile.read((char *)indata, inlen);

    tjhandle decompressor = tjInitDecompress();
    if (!decompressor) {
      ERROR("tjInitDecompress failed");
      ret = -1;
      break;
    }
    if (0 != tjDecompressHeader3(decompressor, indata, inlen,
                        &width_org, &height_org,
                        &sub_samp, &color_space)) {
      ERROR("tjDecompressHeader3 failed");
      ret = -1;
      break;
    }
    width = width_org / scale;
    height = height_org / scale;

    buffer = (uint8_t *)calloc(1, width * height * COLOR_COMPONENTS);
    if (!buffer) {
      ERROR("calloc failed");
      ret = -1;
      break;
    }
    if (0 != tjDecompress2(decompressor, indata,
                  inlen, buffer, width , 0,
                  height, TJPF_RGB, TJFLAG_FASTDCT)) {
      ERROR("tjDecompress2 failed");
      ret = -1;
      break;
    }
    if (0 != tjDestroy(decompressor)) {
      ERROR("tjDestroy failed");
      ret = -1;
      break;
    }
    tjhandle jpeg_compressor = tjInitCompress();
    tjCompress2(jpeg_compressor, buffer, width, 0, height,
                TJPF_RGB, &thumbnail, &thumbnail_size,
                sub_samp, m_quality, TJFLAG_FASTDCT);
    tjDestroy(jpeg_compressor);

    AMFile outfile(outpath);
    outfile.open(AMFile::AM_FILE_CREATE);
    outfile.write(thumbnail, thumbnail_size);
  } while (0);

  if (indata) free(indata);
  if (buffer) free(buffer);
  if (thumbnail) free(thumbnail);

  return ret;
}

