/*******************************************************************************
 * iso_base_media_file_defs.cpp
 *
 * History:
 *   2015-4-15 - [ccjing] created file
 *
 * Copyright (C) 2008-2015, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/

#include "am_base_include.h"
#include "am_define.h"
#include "am_log.h"

#include "am_utils.h"
#include "am_video_types.h"
#include "am_muxer_mp4_helper.h"
#include "iso_base_media_file_defs.h"

SISOMBaseBox::SISOMBaseBox() :
    m_size(0),
    m_type(DISOM_BOX_TAG_NULL)
{
}

AM_STATE SISOMBaseBox::write_base_box(AMMp4FileWriter* file_writer)
{
  AM_STATE ret = AM_STATE_OK;
  do {
    if (AM_UNLIKELY(file_writer->write_be_u32(m_size) != AM_STATE_OK)) {
      ERROR("Write base box error.");
      ret = AM_STATE_ERROR;
      break;
    }
    if (AM_UNLIKELY(file_writer->write_data((uint8_t*)&m_type, sizeof(m_type))
                    != AM_STATE_OK)) {
      ERROR("Write base box error.");
      ret = AM_STATE_ERROR;
      break;
    }
  } while (0);
  return ret;
}

uint32_t SISOMBaseBox::get_base_box_length()
{
  return ((uint32_t)(sizeof(uint32_t) + DISOM_TAG_SIZE));
}

SISOMFullBox::SISOMFullBox():
    m_flags(0),
    m_version(0)
{
}

AM_STATE SISOMFullBox::write_full_box(AMMp4FileWriter* file_writer)
{
  AM_STATE ret = AM_STATE_OK;
  do {
    if(AM_UNLIKELY(m_base_box.write_base_box(file_writer) != AM_STATE_OK)) {
      ERROR("Failed to write base box in the full box");
      ret = AM_STATE_ERROR;
      break;
    }
    if(AM_UNLIKELY(file_writer->write_be_u32(((uint32_t) m_version << 24)
                                | ((uint32_t)m_flags)) != AM_STATE_OK)) {
      ERROR("Failed to write full box.");
      ret = AM_STATE_ERROR;
      break;
    }
  } while (0);
  return ret;
}

uint32_t SISOMFullBox::get_full_box_length()
{
  //m_flags is 24 bits.
  return ((uint32_t)(m_base_box.get_base_box_length() + sizeof(uint32_t)));
}

AM_STATE SISOMFreeBox::write_free_box(AMMp4FileWriter* file_writer)
{
  return m_base_box.write_base_box(file_writer);
}

SISOMFileTypeBox::SISOMFileTypeBox():
    m_compatible_brands_number(0),
    m_minor_version(0),
    m_major_brand(DISOM_BRAND_NULL)
{
  for(int i = 0; i< DISOM_MAX_COMPATIBLE_BRANDS; i++) {
    m_compatible_brands[i] = DISOM_BRAND_NULL;
  }
}

AM_STATE SISOMFileTypeBox::write_file_type_box(AMMp4FileWriter* file_writer)
{
  AM_STATE ret = AM_STATE_OK;
  do {
    if (AM_UNLIKELY(m_base_box.write_base_box(file_writer) != AM_STATE_OK)) {
      ERROR("Failed to write base box in file type box");
      ret = AM_STATE_ERROR;
      break;
    }
    if (AM_UNLIKELY(file_writer->write_data((uint8_t*)&m_major_brand,
                                            DISOM_TAG_SIZE) != AM_STATE_OK)) {
      ERROR("Failed to write major brand in file type box");
      ret = AM_STATE_ERROR;
      break;
    }
    if (AM_UNLIKELY(file_writer->write_be_u32(m_minor_version)
                    != AM_STATE_OK)) {
      ERROR("Failed to write minor version in file type box");
      ret = AM_STATE_ERROR;
      break;
    }
    for (uint32_t i = 0; i < m_compatible_brands_number; ++ i) {
      if (AM_UNLIKELY(file_writer->write_data((uint8_t*)&m_compatible_brands[i],
                                              DISOM_TAG_SIZE) != AM_STATE_OK)) {
        ERROR("Failed to write file type box");
        ret = AM_STATE_ERROR;
        break;
      }
    }
  } while (0);
  return ret;
}

AM_STATE SISOMMediaDataBox::write_media_data_box(AMMp4FileWriter* file_writer)
{
  AM_STATE ret = AM_STATE_OK;
  if(AM_UNLIKELY(m_base_box.write_base_box(file_writer) != AM_STATE_OK)) {
    ERROR("Failed to write media data box");
    ret = AM_STATE_ERROR;
  }
  return ret;
}

SISOMMovieHeaderBox::SISOMMovieHeaderBox():
    m_creation_time(0),
    m_modification_time(0),
    m_duration(0),
    m_timescale(0),
    m_rate(0),
    m_reserved_1(0),
    m_reserved_2(0),
    m_next_track_ID(0),
    m_volume(0),
    m_reserved(0)
{
  memset(m_matrix, 0, sizeof(m_matrix));
  memset(m_pre_defined, 0, sizeof(m_pre_defined));
}
AM_STATE SISOMMovieHeaderBox::write_movie_header_box(AMMp4FileWriter* file_writer)
{
  AM_STATE ret = AM_STATE_OK;
  do {
    if (AM_UNLIKELY(m_base_full_box.write_full_box(file_writer) !=
        AM_STATE_OK)) {
      ERROR("Failed to write base full box in movie header box");
      ret = AM_STATE_ERROR;
      break;
    }

    if (AM_LIKELY(!m_base_full_box.m_version)) {
      if (AM_UNLIKELY(file_writer->write_be_u32((uint32_t )m_creation_time)
                      != AM_STATE_OK
                      || file_writer->write_be_u32((uint32_t )m_modification_time)
                      != AM_STATE_OK
                      || file_writer->write_be_u32(m_timescale) != AM_STATE_OK
                      || file_writer->write_be_u32((uint32_t )m_duration)
                      != AM_STATE_OK)) {
        ERROR("Failed to write movie header box");
        ret = AM_STATE_ERROR;
        break;
      }
    } else {
      if (AM_UNLIKELY(file_writer->write_be_u64(m_creation_time) != AM_STATE_OK
                      || file_writer->write_be_u64(m_modification_time)
                      != AM_STATE_OK
                      || file_writer->write_be_u32(m_timescale) != AM_STATE_OK
                      || file_writer->write_be_u64(m_duration))) {
        ERROR("Failed to write movie header box");
        ret = AM_STATE_ERROR;
        break;
      }
    }

    if (AM_UNLIKELY(file_writer->write_be_u32(m_rate) != AM_STATE_OK
                    || file_writer->write_be_u16(m_volume) != AM_STATE_OK
                    || file_writer->write_be_u16(m_reserved) != AM_STATE_OK
                    || file_writer->write_be_u32(m_reserved_1) != AM_STATE_OK
                    || file_writer->write_be_u32(m_reserved_2) != AM_STATE_OK)) {
      ERROR("Failed to write movie header box");
      ret = AM_STATE_ERROR;
      break;
    }

    for (uint32_t i = 0; i < 9; i ++) {
      if(AM_UNLIKELY(file_writer->write_be_u32(m_matrix[i]) != AM_STATE_OK)) {
        ERROR("Failed to write movie header box");
        ret = AM_STATE_ERROR;
        return ret;
      }
    }

    for (uint32_t i = 0; i < 6; i ++) {
      if(AM_UNLIKELY(file_writer->write_be_u32(m_pre_defined[i]) != AM_STATE_OK)) {
        ERROR("Failed to write movie header box");
        ret = AM_STATE_ERROR;
        return ret;
      }
    }

    if(AM_UNLIKELY(file_writer->write_be_u32(m_next_track_ID) != AM_STATE_OK)) {
      ERROR("Failed to write movie header box");
      ret = AM_STATE_ERROR;
      break;
    }
  } while (0);
  return ret;
}

SISOMTrackHeaderBox::SISOMTrackHeaderBox():
    m_creation_time(0),
    m_modification_time(0),
    m_duration(0),
    m_track_ID(0),
    m_reserved(0),
    m_width(0),
    m_height(0),
    m_layer(0),
    m_alternate_group(0),
    m_volume(0),
    m_reserved_2(0)
{
  memset(m_reserved_1, 0, sizeof(m_reserved_1));
  memset(m_matrix, 0, sizeof(m_matrix));
}
AM_STATE SISOMTrackHeaderBox::write_movie_track_header_box(
                                             AMMp4FileWriter* file_writer)
{
  AM_STATE ret = AM_STATE_OK;
    do {
      if (AM_UNLIKELY(m_base_full_box.write_full_box(file_writer)
                      != AM_STATE_OK)) {
        ERROR("Failed to write movie track header box.");
        ret = AM_STATE_ERROR;
        break;
      }
      if (AM_LIKELY(!m_base_full_box.m_version)) {
        if (AM_UNLIKELY(file_writer->write_be_u32((uint32_t )m_creation_time)
            != AM_STATE_OK ||
            file_writer->write_be_u32((uint32_t )m_modification_time)
            != AM_STATE_OK ||
            file_writer->write_be_u32(m_track_ID) != AM_STATE_OK ||
            file_writer->write_be_u32(m_reserved) != AM_STATE_OK ||
            file_writer->write_be_u32((uint32_t )m_duration) != AM_STATE_OK)) {
          ERROR("Failed to write movie track header box.");
          ret = AM_STATE_ERROR;
          break;
        }
      } else {
        if (AM_UNLIKELY(file_writer->write_be_u64(m_creation_time)
                        != AM_STATE_OK ||
                        file_writer->write_be_u64(m_modification_time)
                        != AM_STATE_OK ||
                        file_writer->write_be_u32(m_track_ID)
                        != AM_STATE_OK ||
                        file_writer->write_be_u32(m_reserved)
                        != AM_STATE_OK ||
                        file_writer->write_be_u64(m_duration)
                        != AM_STATE_OK)) {
          ERROR("Failed to write movie track header box.");
          ret = AM_STATE_ERROR;
          break;
        }
      }
      for (uint32_t i = 0; i < 2; i ++) {
        if (AM_UNLIKELY(file_writer->write_be_u32(m_reserved_1[i])
                        != AM_STATE_OK)) {
          ERROR("Failed to write movie track header box.");
          ret = AM_STATE_ERROR;
          return ret;
        }
      }
      if (AM_UNLIKELY(file_writer->write_be_u16(m_layer) != AM_STATE_OK ||
                      file_writer->write_be_u16(m_alternate_group)
                      != AM_STATE_OK ||
                      file_writer->write_be_u16(m_volume) != AM_STATE_OK ||
                      file_writer->write_be_u16(m_reserved_2)
                      != AM_STATE_OK)) {
        ERROR("Failed to write movie track header box");
        ret = AM_STATE_ERROR;
        break;
      }
      for (uint32_t i = 0; i < 9; i ++) {
        if (AM_UNLIKELY(file_writer->write_be_u32(m_matrix[i])
                        != AM_STATE_OK)) {
          ERROR("Failed to write movie track header box");
          ret = AM_STATE_ERROR;
          return ret;
        }
      }

      if (AM_UNLIKELY(file_writer->write_be_u32(m_width) != AM_STATE_OK
          || file_writer->write_be_u32(m_height) != AM_STATE_OK)) {
        ERROR("Failed to write movie track header box");
        ret = AM_STATE_ERROR;
        break;
      }
    } while (0);
    return ret;
}

SISOMMediaHeaderBox::SISOMMediaHeaderBox():
    m_creation_time(0),
    m_modification_time(0),
    m_duration(0),
    m_timescale(0),
    m_pre_defined(0),
    m_pad(0)
{
  memset(m_language, 0, sizeof(m_language));
}

AM_STATE SISOMMediaHeaderBox::write_media_header_box(
                                      AMMp4FileWriter* file_writer)
{
  AM_STATE ret = AM_STATE_OK;
  do {
    if (AM_UNLIKELY(m_base_full_box.write_full_box(file_writer)
                    != AM_STATE_OK)) {
      ERROR("Failed to write media header box.");
      ret = AM_STATE_ERROR;
      break;
    }
    if (AM_LIKELY(!m_base_full_box.m_version)) {
      if (AM_UNLIKELY(file_writer->write_be_u32((uint32_t )m_creation_time)
                      != AM_STATE_OK ||
                      file_writer->write_be_u32((uint32_t )m_modification_time)
                      != AM_STATE_OK ||
                      file_writer->write_be_u32(m_timescale)
                      != AM_STATE_OK ||
                      file_writer->write_be_u32((uint32_t )m_duration)
                      != AM_STATE_OK)) {
        ERROR("Failed to write media header box.");
        ret = AM_STATE_OK;
        break;
      }
    } else {
      if (AM_UNLIKELY(file_writer->write_be_u64(m_creation_time)
                      != AM_STATE_OK ||
                      file_writer->write_be_u64(m_modification_time)
                      != AM_STATE_OK ||
                      file_writer->write_be_u32(m_timescale)
                      != AM_STATE_OK ||
                      file_writer->write_be_u64(m_duration)
                      != AM_STATE_OK)) {
        ERROR("Failed to write media header box.");
        ret = AM_STATE_ERROR;
        break;
      }
    }
    if (AM_UNLIKELY(file_writer->write_be_u32(((uint32_t )m_pad << 31)
                                              | ((uint32_t )m_language[0] << 26)
                                              | ((uint32_t )m_language[1] << 21)
                                              | ((uint32_t )m_language[2] << 16)
                                              | ((uint32_t )m_pre_defined))
                    != AM_STATE_OK)) {
      ERROR("Failed to write media header box");
      ret = AM_STATE_ERROR;
      break;
    }
  } while (0);
  return ret;
}

SISOMHandlerReferenceBox::SISOMHandlerReferenceBox():
    m_handler_type(DISOM_BOX_TAG_NULL),
    m_pre_defined(0),
    m_name(NULL)
{
  memset(m_reserved, 0, sizeof(m_reserved));
}

AM_STATE SISOMHandlerReferenceBox::write_hander_reference_box(
    AMMp4FileWriter* file_writer)
{
  AM_STATE ret = AM_STATE_OK;
  do {
    if (AM_UNLIKELY(m_base_full_box.write_full_box(file_writer)
                    != AM_STATE_OK ||
                    file_writer->write_be_u32(m_pre_defined)
                    != AM_STATE_OK ||
                    file_writer->write_data((uint8_t*)&m_handler_type,
                        DISOM_TAG_SIZE)!= AM_STATE_OK)) {
      ERROR("Failed to write hander reference box");
      ret = AM_STATE_ERROR;
      break;
    }
    for (uint32_t i = 0; i < 3; i ++) {
      if (AM_UNLIKELY(file_writer->write_be_u32(m_reserved[i]) != AM_STATE_OK)) {
        ERROR("Failed to write handler reference box.");
        ret = AM_STATE_ERROR;
        return ret;
      }
    }
    if (m_name) {
      if (AM_UNLIKELY(file_writer->write_data((uint8_t* )(m_name),
            (uint32_t )strlen(m_name)) != AM_STATE_OK)) {
        ERROR("Failed to write handler reference box.");
        ret = AM_STATE_ERROR;
        break;
      }
    }
    if (AM_UNLIKELY(file_writer->write_u8(0) != AM_STATE_OK)) {
      ERROR("Failed to write handler reference box.");
      ret = AM_STATE_ERROR;
      break;
    }
  } while (0);
  return ret;
}

SISOMVideoMediaInformationHeaderBox::SISOMVideoMediaInformationHeaderBox():
    m_graphicsmode(0)
{
  memset(m_opcolor, 0, sizeof(m_opcolor));
}

AM_STATE SISOMVideoMediaInformationHeaderBox::
    write_video_media_information_header_box(AMMp4FileWriter* file_writer)
{
  AM_STATE ret = AM_STATE_OK;
  do {
    if (AM_UNLIKELY(m_base_full_box.write_full_box(file_writer)
                    != AM_STATE_OK)) {
      ERROR("Failed to write video media header box.");
      ret = AM_STATE_ERROR;
      break;
    }
    if (AM_UNLIKELY(file_writer->write_be_u16(m_graphicsmode) != AM_STATE_OK ||
                    file_writer->write_be_u16(m_opcolor[0]) != AM_STATE_OK ||
                    file_writer->write_be_u16(m_opcolor[1]) != AM_STATE_OK ||
                    file_writer->write_be_u16(m_opcolor[2]) != AM_STATE_OK)) {
      ERROR("Failed to write video media header box.");
      ret = AM_STATE_ERROR;
      break;
    }
  } while (0);
  return ret;
}

SISOMSoundMediaInformationHeaderBox::SISOMSoundMediaInformationHeaderBox():
    m_balanse(0),
    m_reserved(0)
{
}

AM_STATE SISOMSoundMediaInformationHeaderBox::
     write_sound_media_information_header_box(AMMp4FileWriter* file_writer)
{
  AM_STATE ret = AM_STATE_OK;
  do {
    if (AM_UNLIKELY(m_base_full_box.write_full_box(file_writer)
                    != AM_STATE_OK)) {
      ret = AM_STATE_ERROR;
      break;
    }
    if (AM_UNLIKELY(file_writer->write_be_u16(m_balanse) != AM_STATE_OK ||
                    file_writer->write_be_u16(m_reserved) != AM_STATE_OK)) {
      ERROR("Failed to write sound media header box.");
      ret = AM_STATE_ERROR;
      break;
    }
  } while (0);
  return ret;
}

SISOMDecodingTimeToSampleBox::SISOMDecodingTimeToSampleBox():
    m_entry_buf_count(0),
    m_entry_count(0),
    m_p_entry(NULL)
{
}

SISOMDecodingTimeToSampleBox::~SISOMDecodingTimeToSampleBox()
{
  delete[] m_p_entry;
}

AM_STATE SISOMDecodingTimeToSampleBox::write_decoding_time_to_sample_box(
    AMMp4FileWriter* file_writer)
{
  AM_STATE ret = AM_STATE_OK;
  do {
    if (AM_UNLIKELY(m_base_full_box.write_full_box(file_writer)
                    != AM_STATE_OK)) {
      ERROR("Failed to write decoding time to sample box.");
      ret = AM_STATE_ERROR;
      break;
    }
    if (AM_UNLIKELY(file_writer->write_be_u32(m_entry_count) != AM_STATE_OK)) {
      ERROR("Failed to write decoding time to sample box.");
      ret = AM_STATE_ERROR;
      break;
    }
    for (uint32_t i = 0; i < m_entry_count; ++ i) {
      if (AM_UNLIKELY(file_writer->write_be_u32(m_p_entry[i].sample_count)
                      != AM_STATE_OK ||
                      file_writer->write_be_u32(m_p_entry[i].sample_delta)
                      != AM_STATE_OK)) {
        ERROR("Failed to write decoding time to sample box.");
        ret = AM_STATE_ERROR;
        return ret;
      }
    }
    m_entry_count = 0;
  } while (0);
  return ret;
}

void SISOMDecodingTimeToSampleBox::update_decoding_time_to_sample_box_size()
{
  m_base_full_box.m_base_box.m_size = DISOM_FULL_BOX_SIZE + sizeof(uint32_t) +
        m_entry_count * sizeof(_STimeEntry);
}

SISOMCompositionTimeToSampleBox::SISOMCompositionTimeToSampleBox():
    m_entry_count(0),
    m_entry_buf_count(0),
    m_p_entry(NULL)
{
}

SISOMCompositionTimeToSampleBox::~SISOMCompositionTimeToSampleBox()
{
  delete[] m_p_entry;
}

AM_STATE SISOMCompositionTimeToSampleBox::write_composition_time_to_sample_box(
                    AMMp4FileWriter* file_writer)
{
  AM_STATE ret = AM_STATE_OK;
    do{
      if(AM_UNLIKELY(m_base_full_box.write_full_box(file_writer)
                     != AM_STATE_OK)) {
        ERROR("Failed to write composition time to sample box.");
        ret = AM_STATE_ERROR;
        break;
      }
      if(AM_UNLIKELY(file_writer->write_be_u32(m_entry_count) != AM_STATE_OK)) {
        ERROR("Failed to write composition time to sample box.");
        ret = AM_STATE_ERROR;
        break;
      }
      for(uint32_t i = 0; i< m_entry_count; ++ i) {
        if(AM_UNLIKELY(file_writer->write_be_u32(m_p_entry[i].sample_count)
                       != AM_STATE_OK)) {
          ERROR("Failed to write sample count of composition time to sample box");
          ret = AM_STATE_ERROR;
          break;
        }
        if(AM_UNLIKELY(file_writer->write_be_u32(m_p_entry[i].sample_delta)
                       != AM_STATE_OK)) {
          ERROR("Failed to write sample delta fo composition time to sample box.");
          ret = AM_STATE_ERROR;
          break;
        }
      }
    }while(0);
    m_entry_count = 0;
    return ret;
}

void SISOMCompositionTimeToSampleBox::
                        update_composition_time_to_sample_box_size()
{
  m_base_full_box.m_base_box.m_size = DISOM_FULL_BOX_SIZE + sizeof(uint32_t) +
        m_entry_count * sizeof(_STimeEntry);
}

SISOMAVCDecoderConfigurationRecord::SISOMAVCDecoderConfigurationRecord():
    m_p_pps(NULL),
    m_p_sps(NULL),
    m_pictureParametersSetLength(0),
    m_sequenceParametersSetLength(0),
    m_configurationVersion(0),
    m_AVCProfileIndication(0),
    m_profile_compatibility(0),
    m_AVCLevelIndication(0),
    m_reserved(0),
    m_lengthSizeMinusOne(0),
    m_reserved_1(0),
    m_numOfSequenceParameterSets(0),
    m_numOfPictureParameterSets(0)
{
}

AM_STATE SISOMAVCDecoderConfigurationRecord::
         write_avc_decoder_configuration_record_box(
             AMMp4FileWriter* file_writer)
{
  AM_STATE ret = AM_STATE_OK;
  do {
    if (AM_UNLIKELY(m_base_box.write_base_box(file_writer) != AM_STATE_OK
                    || file_writer->write_u8(m_configurationVersion)
                    != AM_STATE_OK)) {
      ERROR("Failed to write avc decoder configuration record box.");
      ret = AM_STATE_ERROR;
      break;
    }
    if(AM_UNLIKELY(file_writer->write_u8(m_AVCProfileIndication)
                   != AM_STATE_OK
        || file_writer->write_u8(m_profile_compatibility) != AM_STATE_OK
        || file_writer->write_u8(m_AVCLevelIndication) != AM_STATE_OK)) {
      ERROR("Failed to write avc decoder configuration record box.");
      ret = AM_STATE_ERROR;
      break;
    }
    if(AM_UNLIKELY(file_writer->write_be_u16(((uint16_t )m_reserved << 10)
                                   | ((uint16_t )m_lengthSizeMinusOne << 8)
                                   | ((uint16_t )m_reserved_1 << 5)
                                   | ((uint16_t )m_numOfSequenceParameterSets))
                   != AM_STATE_OK
                   || file_writer->write_be_u16(m_sequenceParametersSetLength)
                   != AM_STATE_OK
                   || file_writer->write_data(m_p_sps,
                      m_sequenceParametersSetLength) != AM_STATE_OK
                   || file_writer->write_data(&m_numOfPictureParameterSets, 1)
                   != AM_STATE_OK
                   || file_writer->write_be_u16(m_pictureParametersSetLength)
                   != AM_STATE_OK
                   || file_writer->write_data(m_p_pps,
                           m_pictureParametersSetLength) != AM_STATE_OK)) {
      ERROR("Failed to write avc decoder configuration record box.");
      ret = AM_STATE_ERROR;
      break;
    }
  } while (0);
  return ret;
}

_SVisualSampleEntry::_SVisualSampleEntry():
    m_horizresolution(0),
    m_vertresolution(0),
    m_reserved_1(0),
    m_data_reference_index(0),
    m_pre_defined(0),
    m_reserved(0),
    m_width(0),
    m_height(0),
    m_frame_count(0),
    m_depth(0),
    m_pre_defined_2(0)
{
  memset(m_pre_defined_1, 0, sizeof(m_pre_defined_1));
  memset(m_reserved_0, 0, sizeof(m_reserved_0));
  memset(m_compressorname, 0, sizeof(m_compressorname));
}

AM_STATE _SVisualSampleEntry::write_visual_sample_entry(
    AMMp4FileWriter* file_writer)
{
  AM_STATE ret = AM_STATE_OK;
  do {
    if (AM_UNLIKELY(m_base_box.write_base_box(file_writer) != AM_STATE_OK ||
                    file_writer->write_data(m_reserved_0, 6) != AM_STATE_OK ||
                    file_writer->write_be_u16(m_data_reference_index)
                    != AM_STATE_OK ||
                    file_writer->write_be_u16(m_pre_defined) != AM_STATE_OK ||
                    file_writer->write_be_u16(m_reserved) != AM_STATE_OK)) {
      ERROR("Failed to write visual sample description box.");
      ret = AM_STATE_ERROR;
      break;
    }
    for (uint32_t i = 0; i < 3; i ++) {
      if (AM_UNLIKELY(file_writer->write_be_u32(m_pre_defined_1[i])
                      != AM_STATE_OK)) {
        ERROR("Failed to write visual sample description box.");
        ret = AM_STATE_ERROR;
        return ret;
      }
    }
    if (AM_UNLIKELY(file_writer->write_be_u16(m_width) != AM_STATE_OK
                 || file_writer->write_be_u16(m_height) != AM_STATE_OK
                 || file_writer->write_be_u32(m_horizresolution) != AM_STATE_OK
                 || file_writer->write_be_u32(m_vertresolution) != AM_STATE_OK
                 || file_writer->write_be_u32(m_reserved_1) != AM_STATE_OK
                 || file_writer->write_be_u16(m_frame_count) != AM_STATE_OK
                 || file_writer->write_data((uint8_t* )m_compressorname, 32)
                    != AM_STATE_OK
                 || file_writer->write_be_u16(m_depth) != AM_STATE_OK
                 || file_writer->write_be_s16(m_pre_defined_2)
                 != AM_STATE_OK)) {
      ERROR("Failed to write visual sample description box.");
      ret = AM_STATE_ERROR;
      break;
    }
    if (AM_UNLIKELY(m_avc_config.write_avc_decoder_configuration_record_box
                    (file_writer) != AM_STATE_OK)) {
      ERROR("Failed to write visual sample description box.");
      ret = AM_STATE_ERROR;
      break;
    }
  } while (0);
  return ret;
}

SISOMAACElementaryStreamDescriptorBox::SISOMAACElementaryStreamDescriptorBox():
            m_es_id(0),
            m_es_descriptor_type_tag(0),
            m_es_descriptor_type_length(0),
            m_stream_priority(0),
            m_buffer_size(0),
            m_max_bitrate(0),
            m_avg_bitrate(0),
            m_decoder_config_descriptor_type_tag(0),
            m_decoder_descriptor_type_length(0),
            m_object_type(0),
            m_stream_flag(0),
            m_audio_spec_config(0),
            m_decoder_specific_descriptor_type_tag(0),
            m_decoder_specific_descriptor_type_length(0),
            m_SL_descriptor_type_tag(0),
            m_SL_descriptor_type_length(0),
            m_SL_value(0)
{
}

AM_STATE SISOMAACElementaryStreamDescriptorBox::
                          write_aac_elementary_stream_description_box(
        AMMp4FileWriter* file_writer)
{
  AM_STATE ret = AM_STATE_OK;
  do {
    if (AM_UNLIKELY(m_base_full_box.write_full_box(file_writer)
                    != AM_STATE_OK)) {
      ERROR("Failed to write full box.");
      ret = AM_STATE_ERROR;
      break;
    }
    /*ES descriptor takes 38 bytes*/
    if (AM_UNLIKELY(file_writer->write_u8(m_es_descriptor_type_tag)
                    != AM_STATE_OK ||
                    file_writer->write_be_u16(0x8080) != AM_STATE_OK ||
                    file_writer->write_u8(m_es_descriptor_type_length)
                    != AM_STATE_OK ||
                    file_writer->write_be_u16(m_es_id) != AM_STATE_OK ||
                    file_writer->write_u8(m_stream_priority)
                    != AM_STATE_OK)) {
      ERROR("Failed to write aac elementary stream description box.");
      ret = AM_STATE_ERROR;
      break;
    }
    /*decoder descriptor takes 26 bytes*/
    if (AM_UNLIKELY(file_writer->write_u8(m_decoder_config_descriptor_type_tag)
                    != AM_STATE_OK ||
                    file_writer->write_be_u16(0x8080) != AM_STATE_OK ||
                    file_writer->write_u8(m_decoder_descriptor_type_length)
                    != AM_STATE_OK ||
                    file_writer->write_u8(m_object_type) != AM_STATE_OK ||
                    // stream type:6 upstream flag:1 reserved flag:1
                    file_writer->write_u8(0x15) != AM_STATE_OK ||
                    file_writer->write_be_u24(m_buffer_size)
                    != AM_STATE_OK ||
                    file_writer->write_be_u32(m_max_bitrate) != AM_STATE_OK ||
                    file_writer->write_be_u32(m_avg_bitrate) != AM_STATE_OK)) {
      ERROR("Failed to decoder descriptor.");
      ret = AM_STATE_ERROR;
      break;
    }
    /*decoder specific info descriptor takes 9 bytes*/
    if (AM_UNLIKELY(file_writer->write_u8(m_decoder_specific_descriptor_type_tag)
                    != AM_STATE_OK ||
                    file_writer->write_be_u16(0x8080) != AM_STATE_OK ||
                    file_writer->write_u8(m_decoder_specific_descriptor_type_length)
                    != AM_STATE_OK ||
                    file_writer->write_be_u16(m_audio_spec_config) != AM_STATE_OK ||
                    file_writer->write_be_u16(0) != AM_STATE_OK ||
                    file_writer->write_u8(0) != AM_STATE_OK)) {
      ERROR("Failed to write decoder specific info descriptor");
      ret = AM_STATE_ERROR;
      break;
    }
    /*SL descriptor takes 5 bytes*/
    if(AM_UNLIKELY(file_writer->write_u8(m_SL_descriptor_type_tag)
                   != AM_STATE_OK ||
                   file_writer->write_be_u16(0x8080) != AM_STATE_OK ||
                   file_writer->write_u8(m_SL_descriptor_type_length)
                   != AM_STATE_OK ||
                   file_writer->write_u8(m_SL_value) != AM_STATE_OK)) {
      ERROR("Failed to write aac elementary stream description box.");
      ret = AM_STATE_ERROR;
      break;
    }
  } while (0);
  return ret;
}

_SAudioSampleEntry::_SAudioSampleEntry():
          m_samplerate(0),
          m_data_reference_index(0),
          m_channelcount(0),
          m_samplesize(0),
          m_pre_defined(0),
          m_reserved_1(0)
{
  memset(m_reserved, 0, sizeof(m_reserved));
  memset(m_reserved_0, 0, sizeof(m_reserved_0));
}

AM_STATE _SAudioSampleEntry::write_audio_sample_entry(AMMp4FileWriter* file_writer)
{
  AM_STATE ret = AM_STATE_OK;
  do {
    if (AM_UNLIKELY(m_base_box.write_base_box(file_writer) != AM_STATE_OK ||
                    file_writer->write_data(m_reserved_0, 6) != AM_STATE_OK ||
                    file_writer->write_be_u16(m_data_reference_index)
                    != AM_STATE_OK)) {
      ERROR("Failed to write audio sample description box.");
      ret = AM_STATE_ERROR;
      break;
    }
    for (uint32_t i = 0; i < 2; i ++) {
      if (AM_UNLIKELY(file_writer->write_be_u32(m_reserved[i])
                      != AM_STATE_OK)) {
        ERROR("Failed to write audio sample description box.");
        ret = AM_STATE_ERROR;
        return ret;
      }
    }
    if (AM_UNLIKELY(file_writer->write_be_u16(m_channelcount) != AM_STATE_OK ||
                    file_writer->write_be_u16(m_samplesize) != AM_STATE_OK ||
                    file_writer->write_be_u16(m_pre_defined) != AM_STATE_OK ||
                    file_writer->write_be_u16(m_reserved_1) != AM_STATE_OK ||
                    file_writer->write_be_u32(m_samplerate) != AM_STATE_OK)) {
      ERROR("Failed to write audio sample description box.");
      ret = AM_STATE_ERROR;
      break;
    }
    if (AM_UNLIKELY(m_esd.write_aac_elementary_stream_description_box(
        file_writer) != AM_STATE_OK)) {
      ERROR("Failed to write audio sample description box.");
      ret = AM_STATE_ERROR;
      break;
    }
  } while (0);
  return ret;
}

SISOMVisualSampleDescriptionBox::SISOMVisualSampleDescriptionBox():
    m_entry_count(0)
{
}

AM_STATE SISOMVisualSampleDescriptionBox::write_visual_sample_description_box(
    AMMp4FileWriter* file_writer)
{
  AM_STATE ret = AM_STATE_OK;
  do {
    if (AM_UNLIKELY(m_base_full_box.write_full_box(file_writer) != AM_STATE_OK
                    || file_writer->write_be_u32(m_entry_count) != AM_STATE_OK
                    || m_visual_entry.write_visual_sample_entry(file_writer)
                    != AM_STATE_OK)) {
      ERROR("Failed to write visual sample description box.");
      ret = AM_STATE_ERROR;
      break;
    }
  } while (0);
  return ret;
}

SISOMAudioSampleDescriptionBox::SISOMAudioSampleDescriptionBox():
    m_entry_count(0)
{
}

AM_STATE SISOMAudioSampleDescriptionBox::write_audio_sample_description_box(
    AMMp4FileWriter* file_writer)
{
  AM_STATE ret = AM_STATE_OK;
  do {
    if (AM_UNLIKELY(m_base_full_box.write_full_box(file_writer) != AM_STATE_OK
                    || file_writer->write_be_u32(m_entry_count) != AM_STATE_OK
    )) {
      ERROR("Failed to write audio sample description box.");
      ret = AM_STATE_ERROR;
      break;
    }
    if(AM_UNLIKELY(m_audio_entry.write_audio_sample_entry(file_writer)
                   != AM_STATE_OK)) {
      ERROR("Failed to write audio sample entry.");
      ret = AM_STATE_ERROR;
      break;
    }
  } while (0);
  return ret;
}

SISOMSampleSizeBox::SISOMSampleSizeBox():
    m_sample_size(0),
    m_sample_count(0),
    m_entry_size(NULL),
    m_entry_buf_count(0)
{
}

SISOMSampleSizeBox::~SISOMSampleSizeBox()
{
  delete[] m_entry_size;
}

AM_STATE SISOMSampleSizeBox::write_sample_size_box(AMMp4FileWriter* file_writer)
{
  AM_STATE ret = AM_STATE_OK;
  do {
    if(AM_UNLIKELY(m_base_full_box.write_full_box(file_writer)
                   != AM_STATE_OK)) {
      ERROR("Failed to write sample size box");
      ret = AM_STATE_ERROR;
      break;
    }
    if(AM_UNLIKELY(file_writer->write_be_u32(m_sample_size) != AM_STATE_OK ||
                   file_writer->write_be_u32(m_sample_count) != AM_STATE_OK)) {
      ERROR("Failed to write sample size box");
      ret = AM_STATE_ERROR;
      break;
    }
    for (uint32_t i = 0; i < m_sample_count; i ++) {
      if(AM_UNLIKELY(file_writer->write_be_u32(m_entry_size[i]) != AM_STATE_OK)) {
        ERROR("Failed to write sample size box");
        ret = AM_STATE_ERROR;
        break;
      }
    }
    if(ret != AM_STATE_OK) {
      break;
    }
  } while (0);
  m_sample_count = 0;
  return ret;
}

void SISOMSampleSizeBox::update_sample_size_box_size()
{
  m_base_full_box.m_base_box.m_size = DISOM_FULL_BOX_SIZE + sizeof(uint32_t) +
        sizeof(uint32_t) + m_sample_count * sizeof(uint32_t);
}

_SSampleToChunkEntry::_SSampleToChunkEntry():
    m_first_entry(0),
    m_sample_per_chunk(0),
    m_sample_description_index(0)
{
}

AM_STATE _SSampleToChunkEntry::write_sample_chunk_entry(
    AMMp4FileWriter* file_writer)
{
  AM_STATE ret = AM_STATE_OK;
  do{
    if (AM_UNLIKELY(file_writer->write_be_u32(m_first_entry) != AM_STATE_OK ||
                    file_writer->write_be_u32(m_sample_per_chunk)
                    != AM_STATE_OK ||
                    file_writer->write_be_u32(m_sample_description_index)
                    != AM_STATE_OK)) {
      ERROR("Failed to write sample to chunk box.");
      ret = AM_STATE_ERROR;
      break;
    }
  }while(0);
  return ret;
}

SISOMSampleToChunkBox::SISOMSampleToChunkBox():
    m_entrys(NULL),
    m_entry_count(0)
{
}

SISOMSampleToChunkBox::~SISOMSampleToChunkBox()
{
  delete[] m_entrys;
}

AM_STATE SISOMSampleToChunkBox::write_sample_to_chunk_box(
                                         AMMp4FileWriter* file_writer)
{
  AM_STATE ret = AM_STATE_OK;
  do {
    if (AM_UNLIKELY(m_base_full_box.write_full_box(file_writer) != AM_STATE_OK)) {
      ERROR("Failed to write sample to chunk box");
      ret = AM_STATE_ERROR;
      break;
    }
    if (AM_UNLIKELY(file_writer->write_be_u32(m_entry_count) != AM_STATE_OK)) {
      ERROR("Failed to write sample to chunk box");
      ret = AM_STATE_ERROR;
      break;
    }
    for (uint32_t i = 0; i < m_entry_count; ++ i) {
      if (AM_UNLIKELY(m_entrys[i].write_sample_chunk_entry(file_writer)
                      != AM_STATE_OK)) {
        ERROR("Failed to write sample to chunk box.");
        ret = AM_STATE_ERROR;
        break;
      }
    }
  } while (0);
  m_entry_count = 0;
  return ret;
}

void SISOMSampleToChunkBox::update_sample_to_chunk_box_size()
{
  m_base_full_box.m_base_box.m_size = DISOM_FULL_BOX_SIZE + sizeof(uint32_t) +
                              m_entry_count * sizeof(_SSampleToChunkEntry);
}

SISOMChunkOffsetBox::SISOMChunkOffsetBox() :
    m_entry_count(0),
    m_chunk_offset(NULL),
    m_entry_buf_count(0)
{
}

SISOMChunkOffsetBox::~SISOMChunkOffsetBox()
{
  delete[] m_chunk_offset;
}

AM_STATE SISOMChunkOffsetBox::write_chunk_offset_box(
    AMMp4FileWriter* file_writer)
{
  AM_STATE ret = AM_STATE_OK;
  do {
    if(AM_UNLIKELY(m_base_full_box.write_full_box(file_writer) != AM_STATE_OK)) {
      ERROR("Failed to write chunk offset box.");
      ret = AM_STATE_ERROR;
      break;
    }
    if(AM_UNLIKELY(file_writer->write_be_u32(m_entry_count) != AM_STATE_OK)) {
      ERROR("Failed to write chunk offset box");
      ret = AM_STATE_ERROR;
      break;
    }
    for (uint32_t i = 0; i < m_entry_count; i ++) {
      if(AM_UNLIKELY(file_writer->write_be_u32(m_chunk_offset[i]) != AM_STATE_OK)) {
        ERROR("Failed to write chunk offset box.");
        ret = AM_STATE_ERROR;
        break;
      }
    }
  } while (0);
  m_entry_count = 0;
  return ret;
}

void SISOMChunkOffsetBox::update_chunk_offset_box_size()
{
  m_base_full_box.m_base_box.m_size = DISOM_FULL_BOX_SIZE + sizeof(uint32_t) +
      m_entry_count * sizeof(uint32_t);
}

SISOMSyncSampleTableBox::SISOMSyncSampleTableBox():
    m_sync_sample_table(NULL),
    m_stss_count(0),
    m_stss_buf_count(0)
{
}

SISOMSyncSampleTableBox::~SISOMSyncSampleTableBox()
{
  delete[] m_sync_sample_table;
}

AM_STATE SISOMSyncSampleTableBox::
                 write_sync_sample_table_box(AMMp4FileWriter* file_writer)
{
  AM_STATE ret = AM_STATE_OK;
  do {
    if (AM_UNLIKELY(m_base_full_box.write_full_box(file_writer)
                    != AM_STATE_OK)) {
      ERROR("Failed to write full box.");
      ret = AM_STATE_ERROR;
      break;
    }
    if (AM_UNLIKELY(file_writer->write_be_u32(m_stss_count)
                    != AM_STATE_OK)) {
      ERROR("Failed to write stss count.");
      ret = AM_STATE_ERROR;
      break;
    }
    for(uint32_t i = 0; i< m_stss_count; ++ i) {
      if (AM_UNLIKELY(file_writer->write_be_u32(m_sync_sample_table[i])
                      != AM_STATE_OK)) {
        ERROR("Failed to write stss to file.");
        ret = AM_STATE_ERROR;
        break;
      }
    }
  } while (0);
  m_stss_count = 0;
  return ret;
}

void SISOMSyncSampleTableBox::update_sync_sample_box_size()
{
  m_base_full_box.m_base_box.m_size = DISOM_FULL_BOX_SIZE + sizeof(uint32_t) +
      m_stss_count * sizeof(uint32_t);
}


AM_STATE SISOMDataEntryBox::write_data_entry_box(AMMp4FileWriter* file_writer)
{
  AM_STATE ret = AM_STATE_OK;
  do {
    if(AM_UNLIKELY(m_base_full_box.write_full_box(file_writer) != AM_STATE_OK)) {
      ERROR("Failed to write full box when write data entry box.");
      ret = AM_STATE_ERROR;
      break;
    }
  } while(0);
  return ret;
}

SISOMDataReferenceBox::SISOMDataReferenceBox():
    m_entry_count(0)
{}

AM_STATE SISOMDataReferenceBox::write_data_reference_box(
                                 AMMp4FileWriter* file_writer)
{
  AM_STATE ret = AM_STATE_OK;
  do {
    if (AM_UNLIKELY(m_base_full_box.write_full_box(file_writer)
                    != AM_STATE_OK ||
                    file_writer->write_be_u32(m_entry_count) != AM_STATE_OK)) {
      ERROR("Failed to write data to reference box.");
      ret = AM_STATE_OK;
      break;
    }
    if (AM_UNLIKELY(m_url.write_data_entry_box(file_writer) != AM_STATE_OK)) {
      ERROR("Failed to write data reference box.");
      ret = AM_STATE_ERROR;
      return ret;
    }
  } while (0);
  return ret;
}

AM_STATE SISOMDataInformationBox::write_data_information_box(
    AMMp4FileWriter* file_writer)
{
  AM_STATE ret = AM_STATE_OK;
  do {
    if (AM_UNLIKELY(m_base_box.write_base_box(file_writer) != AM_STATE_OK ||
                    m_data_ref.write_data_reference_box(file_writer)
                    != AM_STATE_OK)) {
      ERROR("Failed to write data infomation box.");
      ret = AM_STATE_ERROR;
      break;
    }
  } while (0);
  return ret;
}

AM_STATE SISOMSampleTableBox::write_video_sample_table_box(
    AMMp4FileWriter* file_writer)
{
  AM_STATE ret = AM_STATE_OK;
  if (AM_UNLIKELY(m_base_box.write_base_box(file_writer) != AM_STATE_OK ||
                  m_visual_sample_description.\
                  write_visual_sample_description_box(file_writer)
                      != AM_STATE_OK ||
                  m_stts.write_decoding_time_to_sample_box(file_writer)
                      != AM_STATE_OK ||
                  m_ctts.write_composition_time_to_sample_box(file_writer)
                      != AM_STATE_OK ||
                  m_sample_size.write_sample_size_box(file_writer)
                      != AM_STATE_OK ||
                  m_sample_to_chunk.write_sample_to_chunk_box(file_writer)
                      != AM_STATE_OK ||
                  m_chunk_offset.write_chunk_offset_box(file_writer)
                      != AM_STATE_OK ||
                  m_sync_sample.write_sync_sample_table_box(file_writer)
                      != AM_STATE_OK)) {
    ERROR("Failed to write video sample table box.");
    ret = AM_STATE_ERROR;
  }
  return ret;
}

AM_STATE SISOMSampleTableBox::write_audio_sample_table_box(
    AMMp4FileWriter* file_writer)
{
  AM_STATE ret = AM_STATE_OK;
  do {
    if (AM_UNLIKELY(m_base_box.write_base_box(file_writer) != AM_STATE_OK ||
                    m_audio_sample_description.\
                    write_audio_sample_description_box(file_writer)
                        != AM_STATE_OK ||
                    m_stts.write_decoding_time_to_sample_box(file_writer)
                        != AM_STATE_OK ||
                    m_sample_size.write_sample_size_box(file_writer)
                        != AM_STATE_OK ||
                    m_sample_to_chunk.write_sample_to_chunk_box(file_writer)
                        != AM_STATE_OK ||
                    m_chunk_offset.write_chunk_offset_box(file_writer)
                        != AM_STATE_OK)) {
      ERROR("Failed to write sound sample table box.");
      ret = AM_STATE_ERROR;
      break;
    }
  } while (0);
  return ret;
}

void SISOMSampleTableBox::update_video_sample_table_box_size()
{
  m_stts.update_decoding_time_to_sample_box_size();
  m_ctts.update_composition_time_to_sample_box_size();
  m_sample_size.update_sample_size_box_size();
  m_sample_to_chunk.update_sample_to_chunk_box_size();
  m_chunk_offset.update_chunk_offset_box_size();
  m_sync_sample.update_sync_sample_box_size();
  m_base_box.m_size = DISOM_BOX_SIZE + \
      m_stts.m_base_full_box.m_base_box.m_size + \
      m_ctts.m_base_full_box.m_base_box.m_size + \
      m_visual_sample_description.m_base_full_box.m_base_box.m_size + \
      m_sample_size.m_base_full_box.m_base_box.m_size + \
      m_sample_to_chunk.m_base_full_box.m_base_box.m_size + \
      m_chunk_offset.m_base_full_box.m_base_box.m_size + \
      m_sync_sample.m_base_full_box.m_base_box.m_size;
}

void SISOMSampleTableBox::update_audio_sample_table_box_size()
{
  m_stts.update_decoding_time_to_sample_box_size();
  m_sample_size.update_sample_size_box_size();
  m_sample_to_chunk.update_sample_to_chunk_box_size();
  m_chunk_offset.update_chunk_offset_box_size();
  m_base_box.m_size = DISOM_BOX_SIZE + \
      m_stts.m_base_full_box.m_base_box.m_size + \
      m_audio_sample_description.m_base_full_box.m_base_box.m_size + \
      m_sample_size.m_base_full_box.m_base_box.m_size + \
      m_sample_to_chunk.m_base_full_box.m_base_box.m_size + \
      m_chunk_offset.m_base_full_box.m_base_box.m_size;
}

AM_STATE SISOMMediaInformationBox::write_video_media_information_box(
    AMMp4FileWriter* file_writer)
{
  AM_STATE ret = AM_STATE_OK;
  do {
    if (AM_UNLIKELY(m_base_box.write_base_box(file_writer) != AM_STATE_OK ||
                    m_video_information_header.\
                    write_video_media_information_header_box(file_writer)
                    != AM_STATE_OK ||
                    m_data_info.write_data_information_box(file_writer)
                    != AM_STATE_OK ||
                    m_sample_table.write_video_sample_table_box(file_writer)
                    != AM_STATE_OK)) {
      ERROR("Failed to write video media information box.");
      ret = AM_STATE_ERROR;
      break;
    }
  } while (0);
  return ret;
}

AM_STATE SISOMMediaInformationBox::write_audio_media_information_box(
    AMMp4FileWriter* file_writer)
{
  AM_STATE ret = AM_STATE_OK;
  do {
    if (AM_UNLIKELY(m_base_box.write_base_box(file_writer) != AM_STATE_OK ||
                    m_sound_information_header.\
                    write_sound_media_information_header_box(file_writer)
                    != AM_STATE_OK ||
                    m_data_info.write_data_information_box(file_writer)
                    != AM_STATE_OK ||
                    m_sample_table.write_audio_sample_table_box(file_writer)
                    != AM_STATE_OK)) {
      ERROR("Failed to write audio media information box.");
      ret = AM_STATE_ERROR;
      break;
    }
  } while (0);
  return ret;
}

void SISOMMediaInformationBox::update_video_media_information_box_size()
{
  m_sample_table.update_video_sample_table_box_size();
  m_base_box.m_size = DISOM_BOX_SIZE + m_sample_table.m_base_box.m_size
      + m_video_information_header.m_base_full_box.m_base_box.m_size
      + m_data_info.m_base_box.m_size;
}
void SISOMMediaInformationBox::update_audio_media_information_box_size()
{
  m_sample_table.update_audio_sample_table_box_size();
  m_base_box.m_size = DISOM_BOX_SIZE + m_sample_table.m_base_box.m_size +
      m_sound_information_header.m_base_full_box.m_base_box.m_size +
      m_data_info.m_base_box.m_size;
}
AM_STATE SISOMMediaBox::write_video_media_box(AMMp4FileWriter* file_writer)
{
  AM_STATE ret = AM_STATE_OK;
  do {
    if (AM_UNLIKELY(m_base_box.write_base_box(file_writer) != AM_STATE_OK ||
                    m_media_header.write_media_header_box(file_writer)
                    != AM_STATE_OK ||
                    m_media_handler.write_hander_reference_box(file_writer)
                    != AM_STATE_OK ||
                    m_media_info.write_video_media_information_box(file_writer)
                    != AM_STATE_OK)) {
      ERROR("Failed to write video media box.");
      ret = AM_STATE_ERROR;
      break;
    }
  } while (0);
  return ret;
}

AM_STATE SISOMMediaBox::write_audio_media_box(AMMp4FileWriter* file_writer)
{
  AM_STATE ret = AM_STATE_OK;
  do {
    if (AM_UNLIKELY(m_base_box.write_base_box(file_writer) != AM_STATE_OK ||
                    m_media_header.write_media_header_box(file_writer)
                    != AM_STATE_OK ||
                    m_media_handler.write_hander_reference_box(file_writer)
                    != AM_STATE_OK ||
                    m_media_info.write_audio_media_information_box(file_writer)
                    != AM_STATE_OK)) {
      ERROR("Failed to write audio media box.");
      ret = AM_STATE_ERROR;
      break;
    }
  } while (0);
  return ret;
}

void SISOMMediaBox::update_video_media_box_size()
{
  m_media_info.update_video_media_information_box_size();
  m_base_box.m_size = DISOM_BOX_SIZE + \
      m_media_header.m_base_full_box.m_base_box.m_size + \
      m_media_handler.m_base_full_box.m_base_box.m_size + \
      m_media_info.m_base_box.m_size;
}

void SISOMMediaBox::update_audio_media_box_size()
{
  m_media_info.update_audio_media_information_box_size();
  m_base_box.m_size = DISOM_BOX_SIZE + \
      m_media_header.m_base_full_box.m_base_box.m_size + \
      m_media_handler.m_base_full_box.m_base_box.m_size + \
      m_media_info.m_base_box.m_size;
}

AM_STATE SISOMTrackBox::write_video_movie_track_box(AMMp4FileWriter* file_writer)
{
  AM_STATE ret = AM_STATE_OK;
  do {
    if (AM_UNLIKELY(m_base_box.write_base_box(file_writer) != AM_STATE_OK ||
                    m_track_header.write_movie_track_header_box(file_writer)
                    != AM_STATE_OK ||
                    m_media.write_video_media_box(file_writer)
                    != AM_STATE_OK)) {
      ERROR("Failed to write movie video track box.");
      ret = AM_STATE_ERROR;
      break;
    }
  } while (0);
  return ret;
}

AM_STATE SISOMTrackBox::write_audio_movie_track_box(AMMp4FileWriter* file_writer)
{
  AM_STATE ret = AM_STATE_OK;
  do {
    if (AM_UNLIKELY(m_base_box.write_base_box(file_writer) != AM_STATE_OK ||
                    m_track_header.write_movie_track_header_box(file_writer)
                    != AM_STATE_OK ||
                    m_media.write_audio_media_box(file_writer)
                    != AM_STATE_OK)) {
      ERROR("Failed to write movie audio track box.");
      ret = AM_STATE_ERROR;
      break;
    }
  } while (0);
  return ret;
}

void SISOMTrackBox::update_video_movie_track_box_size()
{
  m_media.update_video_media_box_size();
  m_base_box.m_size = DISOM_BOX_SIZE + \
      m_track_header.m_base_full_box.m_base_box.m_size + \
      m_media.m_base_box.m_size;
}

void SISOMTrackBox::update_audio_movie_track_box_size()
{
  m_media.update_audio_media_box_size();
  m_base_box.m_size = DISOM_BOX_SIZE
      + m_track_header.m_base_full_box.m_base_box.m_size
      + m_media.m_base_box.m_size;
}

AM_STATE SISOMMovieBox::write_movie_box(AMMp4FileWriter* file_writer)
{
  AM_STATE ret = AM_STATE_OK;
  do {
    if (AM_UNLIKELY(m_base_box.write_base_box(file_writer) != AM_STATE_OK ||
                    m_movie_header_box.write_movie_header_box(file_writer)
                    != AM_STATE_OK)) {
      ERROR("Failed to write movie box.");
      ret = AM_STATE_ERROR;
      break;
    }
    if (AM_UNLIKELY(m_video_track.write_video_movie_track_box(file_writer)
                    != AM_STATE_OK)) {
      ERROR("Failed to write video movie track box.");
      ret = AM_STATE_ERROR;
      break;
    }
    if (AM_UNLIKELY(m_audio_track.write_audio_movie_track_box(file_writer)
                    != AM_STATE_OK)) {
      ERROR("Failed to write audio movie track box.");
      ret = AM_STATE_ERROR;
      break;
    }
  } while (0);
  return ret;
}

void SISOMMovieBox::update_movie_box_size()
{
  uint32_t size = DISOM_BOX_SIZE
        + m_movie_header_box.m_base_full_box.m_base_box.m_size;
    m_video_track.update_video_movie_track_box_size();
    size += m_video_track.m_base_box.m_size;
    m_audio_track.update_audio_movie_track_box_size();
    size += m_audio_track.m_base_box.m_size;
    m_base_box.m_size = size;
}
