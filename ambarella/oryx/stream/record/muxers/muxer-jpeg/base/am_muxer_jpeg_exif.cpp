/*******************************************************************************
 * am_muxer_jpeg_exif.cpp
 *
 * History:
 *   2016-6-27 - [smdong] created file
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
#include "am_amf_types.h"
#include "am_log.h"
#include "am_define.h"
#include <assert.h>

#include "am_muxer_jpeg_exif.h"

AMMuxerJpegExif* AMMuxerJpegExif::create() {
  AMMuxerJpegExif *result = new AMMuxerJpegExif();
  if (AM_UNLIKELY(result && (result->init() != AM_STATE_OK))) {
    delete result;
    result = nullptr;
  }
  return result;
}

ExifEntry* AMMuxerJpegExif::init_tag(ExifIfd ifd, ExifTag tag)
{
  ExifEntry *entry;
  /* Return an existing tag if one exists */
  if (AM_UNLIKELY(!((entry = exif_content_get_entry(m_exif_data->ifd[ifd],
                                                    tag))))) {
    /* Allocate a new entry */
    entry = exif_entry_new ();
    assert(entry != nullptr); /* catch an out of memory condition */
    entry->tag = tag; /* tag must be set before calling
                         exif_content_add_entry */

    /* Attach the ExifEntry to an IFD */
    exif_content_add_entry (m_exif_data->ifd[ifd], entry);

    /* Allocate memory for the entry and fill with default data */
    exif_entry_initialize (entry, tag);

    /* Ownership of the ExifEntry has now been passed to the IFD.
     * One must be very careful in accessing a structure after
     * unref'ing it; in this case, we know "entry" won't be freed
     * because the reference count was bumped when it was added to
     * the IFD.
     */
    exif_entry_unref(entry);
  }
  return entry;
}

ExifEntry* AMMuxerJpegExif::create_tag(ExifIfd ifd, ExifTag tag, size_t len)
{
  void *buf;
  ExifEntry *entry;

  /* Create a memory allocator to manage this ExifEntry */
  ExifMem *mem = exif_mem_new_default();
  assert(mem != nullptr); /* catch an out of memory condition */

  /* Create a new ExifEntry using our allocator */
  entry = exif_entry_new_mem (mem);
  assert(entry != nullptr);

  /* Allocate memory to use for holding the tag data */
  buf = exif_mem_alloc(mem, len);
  assert(buf != nullptr);

  /* Fill in the entry */
  entry->data = (uint8_t*)buf;
  entry->size = len;
  entry->tag = tag;
  entry->components = len;
  entry->format = EXIF_FORMAT_UNDEFINED;

  /* Attach the ExifEntry to an IFD */
  exif_content_add_entry (m_exif_data->ifd[ifd], entry);

  /* The ExifMem and ExifEntry are now owned elsewhere */
  exif_mem_unref(mem);
  exif_entry_unref(entry);

  return entry;
}

ExifEntry* AMMuxerJpegExif::is_exif_content_has_entry(ExifIfd ifd, ExifTag tag)
{
  ExifEntry *entry = nullptr;
  entry = exif_content_get_entry(m_exif_data->ifd[ifd], tag);

  return entry;
}


void AMMuxerJpegExif::destroy()
{
  delete this;
}

ExifData* AMMuxerJpegExif::get_exif_data()
{
  return m_exif_data;
}

AMMuxerJpegExif::AMMuxerJpegExif()
{

}

AMMuxerJpegExif::~AMMuxerJpegExif()
{
  exif_data_unref(m_exif_data);
}

AM_STATE AMMuxerJpegExif::init()
{
  AM_STATE state = AM_STATE_OK;
  do {
    if (AM_LIKELY(m_exif_data == nullptr)) {
      if (AM_UNLIKELY(!(m_exif_data = exif_data_new()))) {
        ERROR("create exif entry failed!");
        state = AM_STATE_ERROR;
        break;
      }
    }

    /* Set the image options */
    exif_data_set_option(m_exif_data, EXIF_DATA_OPTION_FOLLOW_SPECIFICATION);
    exif_data_set_data_type(m_exif_data, EXIF_DATA_TYPE_COMPRESSED);
    exif_data_set_byte_order(m_exif_data, FILE_BYTE_ORDER);

    /* Create the mandatory EXIF fields with default data */
    exif_data_fix(m_exif_data);

  } while(0);

  return state;
}
