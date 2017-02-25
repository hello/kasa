/*******************************************************************************
 * am_jpeg_exif.cpp
 *
 * History:
 *   2016-07-25 - [smdong] created file
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

#include <sys/stat.h>
#include "am_jpeg_exif.h"
#include "turbojpeg.h"
#include "libexif/exif-data.h"

static std::mutex m_mtx;
#define  DECLARE_MUTEX  std::lock_guard<std::mutex> lck (m_mtx);

AMJpegExif* AMJpegExif::m_instance = nullptr;

static const uint8_t exif_header[] = {
  0xff, 0xd8, 0xff, 0xe1
};

AMIJpegExifPtr AMIJpegExif::create()
{
  return AMJpegExif::get_instance();
}

AMIJpegExif* AMJpegExif::get_instance()
{
  DECLARE_MUTEX
  if (AM_LIKELY(!m_instance)) {
    m_instance = new AMJpegExif();
  }
  return m_instance;
}

bool AMJpegExif::combine_jpeg_exif(const char *jpeg_file,
                                   const char *exif_file,
                                   const char *output_file,
                                   uint32_t thumbnail_jpeg_qual)
{
  bool ret = true;
  uint32_t jpeg_file_size = 0;
  uint32_t exif_file_size = 0;
  uint8_t *jpeg_file_data = nullptr;
  uint8_t *exif_file_data = nullptr;
  uint8_t *comb_file_data = nullptr;
  ExifData *exif_data = nullptr;
  uint8_t *raw_exif_data = nullptr;
  uint32_t raw_exif_data_len = 0;
  do {
    if (AM_UNLIKELY(nullptr == jpeg_file)) {
      ERROR("Input jpeg file is NULL!");
      ret = false;
      break;
    }
    char tmp_file_name[MAX_FILE_NAME_LENGTH] = {0};
    if (nullptr == exif_file) {
      snprintf(tmp_file_name, MAX_FILE_NAME_LENGTH, "%s.exif", jpeg_file);
    } else {
      strncpy(tmp_file_name, exif_file, MAX_FILE_NAME_LENGTH);
    }
    if (AM_UNLIKELY(!read_file(jpeg_file, jpeg_file_data,
                               jpeg_file_size))) {
      ERROR("Failed to read jpeg file");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(!read_file(tmp_file_name, exif_file_data,
                               exif_file_size))) {
      ERROR("Failed to read exif file");
      ret = false;
      break;
    }

    exif_data = exif_data_new();
    exif_data_load_data(exif_data, exif_file_data, exif_file_size);

    /* if thumbnail_jpeg_qual is 0, then will not generate thumbnail */
    if (thumbnail_jpeg_qual) {
      do {
        int jpeg_width, jpeg_height, jpeg_sub_samp, color_space;
        int width_desire = 0;
        int height_desire = 0;
        //decompression
        tjhandle jpeg_decompressor = tjInitDecompress();
        if (AM_UNLIKELY(nullptr == jpeg_decompressor)) {
          ERROR("Create tjhandle failed!");
          break;
        }
        if (AM_UNLIKELY(0 != tjDecompressHeader3(jpeg_decompressor,
                                                 jpeg_file_data,
                                                 jpeg_file_size,
                                                 &jpeg_width,
                                                 &jpeg_height,
                                                 &jpeg_sub_samp,
                                                 &color_space))) {
          ERROR("tjDecompressHeader3: %s",tjGetErrorStr());
          tjDestroy(jpeg_decompressor);
          break;
        }
        int width_scale = jpeg_width / DEFAULT_THUMBNAIL_WIDTH;
        int height_scale = jpeg_height / DEFAULT_THUMBNAIL_HEIGHT;
        int scale = (width_scale > height_scale) ? width_scale : height_scale;
        if (AM_LIKELY(scale != 0)) {
          if (scale > MAX_SCALE_NUMBER) {
            scale = MAX_SCALE_NUMBER;
          } else if (scale > 4) {
            scale = MAX_SCALE_NUMBER;
          } else if (scale > 2) {
            scale = 4;
          }
          width_desire = jpeg_width / scale;
          height_desire = jpeg_height / scale;
        } else {
          width_desire = jpeg_width;
          height_desire = jpeg_height;
        }

        uint8_t buffer[width_desire * height_desire * COLOR_COMPONENTS ] = {0};

        if(AM_UNLIKELY(0 != tjDecompress2(jpeg_decompressor,
                                          jpeg_file_data,
                                          jpeg_file_size,
                                          buffer,
                                          width_desire ,
                                          0,
                                          height_desire,
                                          TJPF_RGB,
                                          TJFLAG_FASTDCT))) {
          ERROR("tjDecompress2: %s", tjGetErrorStr());
          tjDestroy(jpeg_decompressor);
          break;
        }
        tjDestroy(jpeg_decompressor);
        //compression
        tjhandle jpeg_compressor = tjInitCompress();
        if (AM_UNLIKELY(nullptr == jpeg_compressor)) {
          ERROR("Create tjhandle failed!");
          break;
        }
        uint8_t *compressed_thumbnail = nullptr;
        unsigned long thumbnail_size = 0;
        if (AM_UNLIKELY(0 != tjCompress2(jpeg_compressor,
                                         buffer,
                                         width_desire,
                                         0,
                                         height_desire,
                                         TJPF_RGB,
                                         &compressed_thumbnail,
                                         &thumbnail_size,
                                         jpeg_sub_samp,
                                         thumbnail_jpeg_qual,
                                         TJFLAG_FASTDCT))) {
          ERROR("tjCompress2: %s", tjGetErrorStr());
          tjDestroy(jpeg_compressor);
          break;
        }
        tjDestroy(jpeg_compressor);

        ExifEntry *entry = nullptr;
        if (AM_UNLIKELY(!((entry = exif_content_get_entry(
            exif_data->ifd[EXIF_IFD_1], EXIF_TAG_COMPRESSION))))) {
          entry = exif_entry_new();
          if(entry != nullptr) {
            entry->tag = EXIF_TAG_COMPRESSION;
            exif_content_add_entry (exif_data->ifd[EXIF_IFD_1], entry);
            exif_entry_initialize (entry, EXIF_TAG_COMPRESSION);
            exif_entry_unref(entry);
          }
        }
        if (AM_LIKELY(entry)) {
          exif_set_short(entry->data, EXIF_BYTE_ORDER_INTEL, 6);
        }

        /* thumbnail */
        exif_data->data = compressed_thumbnail;
        exif_data->size = thumbnail_size;

      } while(0);
    }

    exif_data_save_data(exif_data, &raw_exif_data, &raw_exif_data_len);
    if (AM_UNLIKELY(0 == raw_exif_data_len)) {
      ERROR("Failed to store raw EXIF data!");
      ret = false;
      break;
    }

    uint8_t exif_len[2] = {uint8_t((raw_exif_data_len + 2) >> 8),
                           uint8_t((raw_exif_data_len + 2) & 0xff)};

    uint32_t comb_file_len = sizeof(exif_header) + raw_exif_data_len +
                               jpeg_file_size;

    comb_file_data = new uint8_t[comb_file_len];

    if (AM_UNLIKELY(comb_file_data == nullptr)) {
      ERROR("Memory create error!");
      ret = false;
      break;
    } else {
      /* Write EXIF header */
      memcpy(comb_file_data, exif_header, sizeof(exif_header));
      /* Write EXIF block length in big-endian order */
      memcpy(comb_file_data + sizeof(exif_header), exif_len, 2);
      /* Write EXIF data block */
      memcpy(comb_file_data + sizeof(exif_header) + 2,
             raw_exif_data, raw_exif_data_len);
      /* Write JPEG image data, skipping the non-EXIF header */
      memcpy(comb_file_data + sizeof(exif_header) + 2 + raw_exif_data_len,
             (jpeg_file_data + 2), jpeg_file_size - 2);

    }
    memset(tmp_file_name , 0, MAX_FILE_NAME_LENGTH);

    if (nullptr == output_file) {
      snprintf(tmp_file_name, MAX_FILE_NAME_LENGTH, "exif_%s", jpeg_file);
    } else {
      strncpy(tmp_file_name, output_file, MAX_FILE_NAME_LENGTH);
    }
    if (AM_UNLIKELY(!write_file(tmp_file_name, comb_file_data,
                                comb_file_len))) {
      ERROR("Failed to write combined jpeg file");
      ret = false;
      break;
    }

  } while(0);

  delete [] comb_file_data;
  delete [] jpeg_file_data;
  delete [] exif_file_data;
  exif_data_unref(exif_data);
  if (AM_LIKELY(raw_exif_data)) {
    free(raw_exif_data);
  }

  return ret;
}

bool AMJpegExif::read_file(const char *file_name,
                           uint8_t *&file_data,
                           uint32_t &file_len)
{
  bool ret = true;
  if (AM_LIKELY(AMFile::exists(file_name))) {
    AMFile file(file_name);
    if (AM_LIKELY(file.open(AMFile::AM_FILE_READONLY))) {
      file_len = file.size();
      if (AM_LIKELY(file_len > 0)) {
        file_data = new uint8_t[file_len];
        if (AM_LIKELY((nullptr != file_data) &&
               ((int)file_len == file.read((char*)file_data, file_len)))) {
          INFO("Successfully read the file: %s", file_name);
        } else {
          ERROR("Read file %s failed!", file_name);
          ret = false;
        }
      }
      file.close();
    } else {
      ERROR("Failed to open %s for reading!", file_name);
      ret = false;
    }
  } else {
    ERROR("Can not find the file: %s!", file_name);
    ret = false;
  }
  return ret;
}

bool AMJpegExif::write_file(const char *file_name,
                            uint8_t *file_data,
                            uint32_t file_len)
{
  bool ret = true;
  do {
    if (AM_UNLIKELY((nullptr == file_name) || (nullptr == file_data) ||
                    (0 == file_len))) {
      ERROR("Invalid parameters!");
      ret = false;
      break;
    }
    if (AM_UNLIKELY(AMFile::exists(file_name))) {
      WARN("Same file already exists, remove it: %s", file_name);
      if(AM_UNLIKELY(unlink(file_name) < 0)) {
        PERROR("unlink");
        ret = false;
        break;
      }
    }
    AMFile file(file_name);
    if (AM_LIKELY(file.open(AMFile::AM_FILE_CREATE))) {
      if(AM_LIKELY((int)file_len == file.write_reliable(file_data,
                                                        file_len))) {
        INFO("Successfully write the file: %s", file_name);
      } else {
        ERROR("Write file %s failed!", file_name);
        ret = false;
      }
      file.close();
    } else {
      PERROR("Failed to open file");
      ret = false;
      break;
    }
  } while(0);

  return ret;
}

AMJpegExif::AMJpegExif() :
    m_ref_count(0)
{

}

AMJpegExif::~AMJpegExif()
{
  m_instance = nullptr;
}

void AMJpegExif::inc_ref()
{
  ++ m_ref_count;
}

void AMJpegExif::release()
{
  if ((m_ref_count >= 0) && (--m_ref_count <= 0)) {
    NOTICE("Last reference of AMJpegExif's object %p, release it!",
           m_instance);
    delete m_instance;
    m_instance = nullptr;
    m_ref_count = 0;
  }
}
