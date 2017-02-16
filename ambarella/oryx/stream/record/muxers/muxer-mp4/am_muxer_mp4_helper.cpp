/*******************************************************************************
 * am_muxer_mp4_helper.h
 *
 * History:
 *   2014-12-02 [Zhi He] created file
 *
 * Copyright (C) 2014, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include "am_base_include.h"
#include "am_define.h"
#include "am_log.h"

#include "iso_base_media_file_defs.h"
#include "adts.h"
#include "am_muxer_mp4_helper.h"

/*uint32_t _get_adts_frame_length(uint8_t *p)
{
  return (((uint32_t) (p[3] & 0x3)) << 11) | (((uint32_t) p[4]) << 3)
      | (((uint32_t) p[5] >> 5) & 0x7);
}

AM_STATE _parse_adts_header(uint8_t *p, SADTSHeader *header)
{
  if (AM_UNLIKELY((!p) || (!header))) {
    ERROR("NULL parameters in gfParseADTSHeader\n");
    return AM_STATE_BAD_PARAM;
  }

  if (AM_UNLIKELY((0xff != p[0]) || (0xf0 != (p[1] & 0xf0)))) {
    ERROR("not find sync byte in gfParseADTSHeader\n");
    return AM_STATE_BAD_PARAM;
  }

  header->ID = (p[1] >> 3) & 0x01;
  header->layer = (p[1] >> 1) & 0x03;
  header->protection_absent = p[1] & 0x01;

  header->profile = (p[2] >> 6) & 0x03;
  header->sampling_frequency_index = (p[2] >> 2) & 0x0f;
  header->private_bit = (p[2] >> 1) & 0x01;
  header->channel_configuration = ((p[2] & 0x01) << 2) | ((p[3] >> 6) & 0x3);
  header->original_copy = (p[3] >> 5) & 0x01;
  header->home = (p[3] >> 4) & 0x01;

  header->copyright_identification_bit = (p[3] >> 3) & 0x01;
  header->copyright_identification_start = (p[3] >> 2) & 0x01;

  header->aac_frame_length = (((uint16_t) p[3] & 0x03) << 11)
      | ((uint16_t) p[4] << 3) | (((uint16_t) p[5] >> 5) & 0x7);

  header->adts_buffer_fullness = (((uint16_t) p[5] & 0x1f) << 6)
      | (((uint16_t) p[6] >> 2) & 0x3f);
  header->number_of_raw_data_blocks_in_frame = p[6] & 0x03;

  return AM_STATE_OK;
}

uint32_t _get_adts_sampling_frequency(uint8_t sampling_frequency_index)
{
  switch (sampling_frequency_index) {

    case EADTSSamplingFrequency_96000:
      return 96000;
      break;

    case EADTSSamplingFrequency_88200:
      return 88200;
      break;

    case EADTSSamplingFrequency_64000:
      return 64000;
      break;

    case EADTSSamplingFrequency_48000:
      return 48000;
      break;

    case EADTSSamplingFrequency_44100:
      return 44100;
      break;

    case EADTSSamplingFrequency_32000:
      return 32000;
      break;

    case EADTSSamplingFrequency_24000:
      return 24000;
      break;

    case EADTSSamplingFrequency_22050:
      return 22050;
      break;

    case EADTSSamplingFrequency_16000:
      return 16000;
      break;

    case EADTSSamplingFrequency_12000:
      return 12000;
      break;

    case EADTSSamplingFrequency_11025:
      return 11025;
      break;

    case EADTSSamplingFrequency_8000:
      return 8000;
      break;

    case EADTSSamplingFrequency_7350:
      return 7350;
      break;

    default:
      break;
  }

  return 0;
}
*/
AM_STATE _get_avc_sps_pps(uint8_t *data_base,
                          uint32_t data_size,
                          uint8_t *&p_sps,
                          uint32_t &sps_size,
                          uint8_t *&p_pps,
                          uint32_t &pps_size)
{
  uint8_t has_sps = 0, has_pps = 0;

  uint8_t* ptr = data_base, *ptr_end = data_base + data_size;
  uint32_t nal_type = 0;

  if (AM_UNLIKELY((NULL == ptr) || (!data_size))) {
    ERROR("NULL pointer(%p) or zero size(%d)\n", data_base, data_size);
    return AM_STATE_BAD_PARAM;
  }

  while (ptr < ptr_end) {
    if (*ptr == 0x00) {
      if (*(ptr + 1) == 0x00) {
        if (*(ptr + 2) == 0x00) {
          if (*(ptr + 3) == 0x01) {
            nal_type = ptr[4] & 0x1F;
            if (ENalType_SPS == nal_type) {
              has_sps = 1;
              p_sps = ptr;
              ptr += 3;
            } else if (ENalType_PPS == nal_type) {
              p_pps = ptr;
              has_pps = 1;
              ptr += 3;
            } else if (has_sps && has_pps) {
              sps_size = (uint32_t) (p_pps - p_sps);
              pps_size = (uint32_t) (ptr - p_pps);
              return AM_STATE_OK;
            }
          }
        } else if (*(ptr + 2) == 0x01) {
          nal_type = ptr[3] & 0x1F;
          if (ENalType_SPS == nal_type) {
            has_sps = 1;
            p_sps = ptr;
            ptr += 2;
          } else if (ENalType_PPS == nal_type) {
            has_pps = 1;
            p_pps = ptr;
            ptr += 2;
          } else if (has_sps && has_pps) {
            sps_size = (uint32_t) (p_pps - p_sps);
            pps_size = (uint32_t) (ptr - p_pps);
            return AM_STATE_OK;
          }
        }
      }
    }
    ++ ptr;
  }

  if (has_sps && has_pps) {
    sps_size = (uint32_t) (p_pps - p_sps);
    pps_size = (uint32_t) (ptr - p_pps);
    return AM_STATE_OK;
  }

  return AM_STATE_NOT_EXIST;
}

struct GetbitsContext
{
    const uint8_t *buffer, *buffer_end;
    uint32_t index;
    uint32_t size_in_bits;
};

#define READ_LE_32(x)                 \
  ((((const uint8_t*)(x))[3] << 24) | \
  (((const uint8_t*)(x))[2] << 16)  | \
  (((const uint8_t*)(x))[1] <<  8)  | \
  ((const uint8_t*)(x))[0])

#define READ_BE_32(x)                 \
  ((((const uint8_t*)(x))[0] << 24) | \
  (((const uint8_t*)(x))[1] << 16)  | \
  (((const uint8_t*)(x))[2] <<  8)  | \
  ((const uint8_t*)(x))[3])

#define BIT_OPEN_READER(name, gb)      \
  uint32_t name##_index = (gb)->index; \
  uint32_t name##_cache  =   0

#define BITS_CLOSE_READER(name, gb) (gb)->index = name##_index
#define BITS_UPDATE_CACHE_BE(name, gb) \
  name##_cache = READ_BE_32(((const uint8_t *)(gb)->buffer)+(name##_index>>3)) \
  << (name##_index&0x07)
#define BITS_UPDATE_CACHE_LE(name, gb) \
  name##_cache = READ_LE_32(((const uint8_t *)(gb)->buffer)+(name##_index>>3)) \
  >> (name##_index&0x07)

#define BITS_SKIP_CACHE(name, gb, num) name##_cache >>= (num)
#define BITS_SKIP_COUNTER(name, gb, num) name##_index += (num)

#define BITS_SKIP_BITS(name, gb, num) do { \
    BITS_SKIP_CACHE(name, gb, num);        \
    BITS_SKIP_COUNTER(name, gb, num);      \
} while (0)

#define BITS_LAST_SKIP_BITS(name, gb, num) BITS_SKIP_COUNTER(name, gb, num)
#define BITS_LAST_SKIP_CACHE(name, gb, num)

#define NEG_USR32(a,s) (((uint32_t)(a))>>(32-(s)))
#define BITS_SHOW_UBITS(name, gb, num) NEG_USR32(name ## _cache, num)

#define BITS_GET_CACHE(name, gb) ((uint32_t)name##_cache)

static const uint8_t simple_log2_table[256] =
{ 0, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
  4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
  6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
  6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 7, 7,
  7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
  7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
  7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
  7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
  7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7
};

static const uint8_t simple_golomb_vlc_len[512] =
{ 19, 17, 15, 15, 13, 13, 13, 13, 11, 11, 11, 11, 11, 11, 11, 11, 9, 9, 9, 9, 9,
  9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
  7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
  3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
  3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
  3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
  3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
  3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
};

static const uint8_t simple_ue_golomb_vlc_code[512] =
{ 32, 32, 32, 32, 32, 32, 32, 32, 31, 32, 32, 32, 32, 32, 32, 32, 15, 16, 17,
  18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 7, 7, 7, 7, 8, 8, 8, 8, 9,
  9, 9, 9, 10, 10, 10, 10, 11, 11, 11, 11, 12, 12, 12, 12, 13, 13, 13, 13, 14,
  14, 14, 14, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4,
  4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static const int8_t simple_se_golomb_vlc_code[512] =
{ 17, 17, 17, 17, 17, 17, 17, 17, 16, 17, 17, 17, 17, 17, 17, 17, 8, -8, 9, -9,
  10, -10, 11, -11, 12, -12, 13, -13, 14, -14, 15, -15, 4, 4, 4, 4, -4, -4, -4,
  -4, 5, 5, 5, 5, -5, -5, -5, -5, 6, 6, 6, 6, -6, -6, -6, -6, 7, 7, 7, 7, -7,
  -7, -7, -7, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, -2, -2, -2, -2,
  -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
  3, 3, 3, 3, 3, 3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3, -3,
  -3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static const uint8_t default_scaling4[2][16] =
{
  { 6, 13, 20, 28, 13, 20, 28, 32, 20, 28, 32, 37, 28, 32, 37, 42 },
  { 10, 14, 20, 24, 14, 20, 24, 27, 20, 24, 27, 30, 24, 27, 30, 34 }
};

static const uint8_t default_scaling8[2][64] =
{
  { 6, 10, 13, 16, 18, 23, 25, 27, 10, 11, 16, 18, 23, 25, 27, 29, 13, 16, 18,
    23, 25, 27, 29, 31, 16, 18, 23, 25, 27, 29, 31, 33, 18, 23, 25, 27, 29, 31,
    33, 36, 23, 25, 27, 29, 31, 33, 36, 38, 25, 27, 29, 31, 33, 36, 38, 40, 27,
    29, 31, 33, 36, 38, 40, 42
  },

  { 9, 13, 15, 17, 19, 21, 22, 24, 13, 13, 17, 19, 21, 22, 24, 25, 15, 17, 19,
    21, 22, 24, 25, 27, 17, 19, 21, 22, 24, 25, 27, 28, 19, 21, 22, 24, 25, 27,
    28, 30, 21, 22, 24, 25, 27, 28, 30, 32, 22, 24, 25, 27, 28, 30, 32, 33, 24,
    25, 27, 28, 30, 32, 33, 35
  }
};

static const uint8_t simple_zigzag_scan[16] =
{ 0 + 0 * 4, 1 + 0 * 4, 0 + 1 * 4, 0 + 2 * 4,
  1 + 1 * 4, 2 + 0 * 4, 3 + 0 * 4, 2 + 1 * 4,
  1 + 2 * 4, 0 + 3 * 4, 1 + 3 * 4, 2 + 2 * 4,
  3 + 1 * 4, 3 + 2 * 4, 2 + 3 * 4, 3 + 3 * 4,
};

const uint8_t simple_zigzag_direct[64] =
{ 0, 1, 8, 16, 9, 2, 3, 10, 17, 24, 32, 25, 18, 11, 4, 5, 12, 19, 26, 33, 40,
  48, 41, 34, 27, 20, 13, 6, 7, 14, 21, 28, 35, 42, 49, 56, 57, 50, 43, 36, 29,
  22, 15, 23, 30, 37, 44, 51, 58, 59, 52, 45, 38, 31, 39, 46, 53, 60, 61, 54,
  47, 55, 62, 63
};

static int _simple_log2_c(uint32_t v)
{
  int n = 0;
  if (v & 0xffff0000) {
    v >>= 16;
    n += 16;
  }
  if (v & 0xff00) {
    v >>= 8;
    n += 8;
  }
  n += simple_log2_table[v];

  return n;
}

static inline int _get_se_golomb(GetbitsContext *gb)
{
  uint32_t buf;
  int log;

  BIT_OPEN_READER(re, gb);
  BITS_UPDATE_CACHE_BE(re, gb);
  buf = BITS_GET_CACHE(re, gb);

  if (buf >= (1 << 27)) {
    buf >>= 32 - 9;
    BITS_LAST_SKIP_BITS(re, gb, simple_golomb_vlc_len[buf]);
    BITS_CLOSE_READER(re, gb);

    return simple_se_golomb_vlc_code[buf];
  } else {
    log = _simple_log2_c(buf);
    BITS_LAST_SKIP_BITS(re, gb, 31 - log);
    BITS_UPDATE_CACHE_BE(re, gb);
    buf = BITS_GET_CACHE(re, gb);
    buf >>= log;

    BITS_LAST_SKIP_BITS(re, gb, 32 - log);
    BITS_CLOSE_READER(re, gb);

    if (buf & 1) {
      buf = -(buf >> 1);
    } else {
      buf = (buf >> 1);
    }

    return buf;
  }
}

static inline int _get_ue_golomb(GetbitsContext *gb)
{
  uint32_t buf;
  int log;

  BIT_OPEN_READER(re, gb);
  BITS_UPDATE_CACHE_BE(re, gb);
  buf = BITS_GET_CACHE(re, gb);

  if (buf >= (1 << 27)) {
    buf >>= 32 - 9;
    BITS_LAST_SKIP_BITS(re, gb, simple_golomb_vlc_len[buf]);
    BITS_CLOSE_READER(re, gb);
    return simple_ue_golomb_vlc_code[buf];
  } else {
    log = 2 * _simple_log2_c(buf) - 31;
    buf >>= log;
    buf --;
    BITS_LAST_SKIP_BITS(re, gb, 32 - log);
    BITS_CLOSE_READER(re, gb);
    return buf;
  }
}

static inline int _get_ue_golomb_31(GetbitsContext *gb)
{
  uint32_t buf;

  BIT_OPEN_READER(re, gb);

  BITS_UPDATE_CACHE_BE(re, gb);
  buf = BITS_GET_CACHE(re, gb);

  buf >>= 32 - 9;

  BITS_LAST_SKIP_BITS(re, gb, simple_golomb_vlc_len[buf]);
  BITS_CLOSE_READER(re, gb);

  return simple_ue_golomb_vlc_code[buf];
}

#if 0
/* Comment out unused static functions */
static inline uint32_t _show_bits(GetbitsContext *s, int n)
{
  register int tmp;
  BIT_OPEN_READER(re, s);
  BITS_UPDATE_CACHE_BE(re, s);
  tmp = BITS_SHOW_UBITS(re, s, n);
  return tmp;
}

static inline uint32_t _show_bits1(GetbitsContext *s)
{
  return _show_bits(s, 1);
}
#endif

static inline uint32_t _get_bits(GetbitsContext *s, int n)
{
  int tmp;
  BIT_OPEN_READER(re, s);
  BITS_UPDATE_CACHE_BE(re, s);
  tmp = BITS_SHOW_UBITS(re, s, n);
  BITS_LAST_SKIP_BITS(re, s, n);
  BITS_CLOSE_READER(re, s);
  return tmp;
}

static inline uint32_t _get_bits1(GetbitsContext *s)
{
  uint32_t index = s->index;
  uint8_t result = s->buffer[index >> 3];

  result <<= index & 7;
  result >>= 8 - 1;

  index ++;
  s->index = index;

  return result;
}

static void _decode_scaling_list(GetbitsContext *gb,
                                 uint8_t *factors,
                                 int size,
                                 const uint8_t *jvt_list,
                                 const uint8_t *fallback_list)
{
  int i, last = 8, next = 8;
  const uint8_t *scan =
      (size == 16) ? simple_zigzag_scan : simple_zigzag_direct;
  if (!_get_bits1(gb)) {
    memcpy(factors, fallback_list, size * sizeof(uint8_t));
  } else {
    for (i = 0; i < size; i ++) {
      if (next) {
        next = (last + _get_se_golomb(gb)) & 0xff;
      }
      if (!i && !next) {
        memcpy(factors, jvt_list, size * sizeof(uint8_t));
        break;
      }
      last = factors[scan[i]] = next ? next : last;
    }
  }
}

static void _decode_scaling_matrices(GetbitsContext *gb,
                                     SCodecAVCSPS *sps,
                                     SCodecAVCPPS *pps,
                                     int is_sps,
                                     uint8_t (*scaling_matrix4)[16],
                                     uint8_t (*scaling_matrix8)[64])
{
  int fallback_sps = !is_sps && sps->scaling_matrix_present;
  const uint8_t *fallback[4] =
  {
    fallback_sps ? sps->scaling_matrix4[0] : default_scaling4[0],
    fallback_sps ? sps->scaling_matrix4[3] : default_scaling4[1],
    fallback_sps ? sps->scaling_matrix8[0] : default_scaling8[0],
    fallback_sps ? sps->scaling_matrix8[3] : default_scaling8[1]
  };

  if (_get_bits1(gb)) {
    sps->scaling_matrix_present |= is_sps;
    _decode_scaling_list(gb,
                         scaling_matrix4[0],
                         16,
                         default_scaling4[0],
                         fallback[0]); // Intra, Y
    _decode_scaling_list(gb,
                         scaling_matrix4[1],
                         16,
                         default_scaling4[0],
                         scaling_matrix4[0]); // Intra, Cr
    _decode_scaling_list(gb,
                         scaling_matrix4[2],
                         16,
                         default_scaling4[0],
                         scaling_matrix4[1]); // Intra, Cb
    _decode_scaling_list(gb,
                         scaling_matrix4[3],
                         16,
                         default_scaling4[1],
                         fallback[1]); // Inter, Y
    _decode_scaling_list(gb,
                         scaling_matrix4[4],
                         16,
                         default_scaling4[1],
                         scaling_matrix4[3]); // Inter, Cr
    _decode_scaling_list(gb,
                         scaling_matrix4[5],
                         16,
                         default_scaling4[1],
                         scaling_matrix4[4]); // Inter, Cb
    if (is_sps || pps->transform_8x8_mode) {
      _decode_scaling_list(gb,
                           scaling_matrix8[0],
                           64,
                           default_scaling8[0],
                           fallback[2]);  // Intra, Y
      _decode_scaling_list(gb,
                           scaling_matrix8[3],
                           64,
                           default_scaling8[1],
                           fallback[3]);  // Inter, Y
      if (sps->chroma_format_idc == 3) {
        _decode_scaling_list(gb,
                             scaling_matrix8[1],
                             64,
                             default_scaling8[0],
                             scaling_matrix8[0]);  // Intra, Cr
        _decode_scaling_list(gb,
                             scaling_matrix8[4],
                             64,
                             default_scaling8[1],
                             scaling_matrix8[3]);  // Inter, Cr
        _decode_scaling_list(gb,
                             scaling_matrix8[2],
                             64,
                             default_scaling8[0],
                             scaling_matrix8[1]);  // Intra, Cb
        _decode_scaling_list(gb,
                             scaling_matrix8[5],
                             64,
                             default_scaling8[1],
                             scaling_matrix8[4]);  // Inter, Cb
      }
    }
  }
}

AM_STATE _try_parse_avc_sps_pps(uint8_t *p_data,
                                uint32_t data_size,
                                SCodecAVCExtraData *p_header)
{
  GetbitsContext gb;
  gb.buffer = p_data;
  gb.buffer_end = p_data + data_size;
  gb.index = 0;
  gb.size_in_bits = 8 * data_size;

  memset(p_header, 0x0, sizeof(SCodecAVCExtraData));

  uint32_t sps_id;
  int i, log2_max_frame_num_minus4;

  p_header->sps.profile_idc = _get_bits(&gb, 8);
  p_header->sps.constraint_set_flags |= _get_bits1(&gb) << 0;
  p_header->sps.constraint_set_flags |= _get_bits1(&gb) << 1;
  p_header->sps.constraint_set_flags |= _get_bits1(&gb) << 2;
  p_header->sps.constraint_set_flags |= _get_bits1(&gb) << 3;
  p_header->sps.constraint_set_flags |= _get_bits1(&gb) << 4;
  p_header->sps.constraint_set_flags |= _get_bits1(&gb) << 5;
  _get_bits(&gb, 2);
  p_header->sps.level_idc = _get_bits(&gb, 8);
  sps_id = _get_ue_golomb_31(&gb);

  if (sps_id >= DMAX_SPS_COUNT) {
    ERROR("sps_id (%d) out of range\n", sps_id);
    return AM_STATE_BAD_PARAM;
  }

  p_header->sps.time_offset_length = 24;
  p_header->sps.full_range = -1;

  memset(p_header->sps.scaling_matrix4,
         16,
         sizeof(p_header->sps.scaling_matrix4));
  memset(p_header->sps.scaling_matrix8,
         16,
         sizeof(p_header->sps.scaling_matrix8));
  p_header->sps.scaling_matrix_present = 0;
  p_header->sps.colorspace = EColorSpace_UNSPECIFIED;

  if (p_header->sps.profile_idc == 100 || p_header->sps.profile_idc == 110 ||
      p_header->sps.profile_idc == 122 || p_header->sps.profile_idc == 244 ||
      p_header->sps.profile_idc == 44  || p_header->sps.profile_idc == 83  ||
      p_header->sps.profile_idc == 86  || p_header->sps.profile_idc == 118 ||
      p_header->sps.profile_idc == 128 || p_header->sps.profile_idc == 144) {
    p_header->sps.chroma_format_idc = _get_ue_golomb_31(&gb);

    if ((uint32_t) p_header->sps.chroma_format_idc > 3U) {
      ERROR("chroma_format_idc %d is illegal\n",
            p_header->sps.chroma_format_idc);
      return AM_STATE_BAD_PARAM;
    } else if (p_header->sps.chroma_format_idc == 3) {
      p_header->sps.residual_color_transform_flag = _get_bits1(&gb);
      if (p_header->sps.residual_color_transform_flag) {
        ERROR("separate color planes are not supported\n");
        return AM_STATE_BAD_PARAM;
      }
    }

    p_header->sps.bit_depth_luma = _get_ue_golomb(&gb) + 8;
    p_header->sps.bit_depth_chroma = _get_ue_golomb(&gb) + 8;
    if (((uint32_t) p_header->sps.bit_depth_luma > 14U) ||
        ((uint32_t) p_header->sps.bit_depth_chroma > 14U) ||
        (p_header->sps.bit_depth_luma != p_header->sps.bit_depth_chroma)) {
      ERROR("illegal bit depth value (%d, %d)\n",
            p_header->sps.bit_depth_luma,
            p_header->sps.bit_depth_chroma);
      return AM_STATE_BAD_PARAM;
    }
    p_header->sps.transform_bypass = _get_bits1(&gb);
    _decode_scaling_matrices(&gb,
                             &p_header->sps,
                             NULL,
                             1,
                             p_header->sps.scaling_matrix4,
                             p_header->sps.scaling_matrix8);

  } else {
    p_header->sps.chroma_format_idc = 1;
    p_header->sps.bit_depth_luma = 8;
    p_header->sps.bit_depth_chroma = 8;
  }

  log2_max_frame_num_minus4 = _get_ue_golomb(&gb);
  if (log2_max_frame_num_minus4 < (DMIN_LOG2_MAX_FRAME_NUM - 4)
      || log2_max_frame_num_minus4 > (DMAX_LOG2_MAX_FRAME_NUM - 4)) {
    ERROR("log2_max_frame_num_minus4 out of range (0-12): %d\n",
          log2_max_frame_num_minus4);
    return AM_STATE_BAD_PARAM;
  }
  p_header->sps.log2_max_frame_num = log2_max_frame_num_minus4 + 4;
  p_header->sps.poc_type = _get_ue_golomb_31(&gb);

  if (p_header->sps.poc_type == 0) {
    uint32_t t = _get_ue_golomb(&gb);
    if (t > 12) {
      ERROR("log2_max_poc_lsb (%d) is out of range\n", t);
      return AM_STATE_BAD_PARAM;
    }
    p_header->sps.log2_max_poc_lsb = t + 4;
  } else if (p_header->sps.poc_type == 1) {
    p_header->sps.delta_pic_order_always_zero_flag = _get_bits1(&gb);
    p_header->sps.offset_for_non_ref_pic = _get_se_golomb(&gb);
    p_header->sps.offset_for_top_to_bottom_field = _get_se_golomb(&gb);
    p_header->sps.poc_cycle_length = _get_ue_golomb(&gb);

    if ((uint32_t) p_header->sps.poc_cycle_length
        >= DARRAY_ELEMS(p_header->sps.offset_for_ref_frame)) {
      ERROR("poc_cycle_length overflow %u\n", p_header->sps.poc_cycle_length);
      return AM_STATE_BAD_PARAM;
    }

    for (i = 0; i < p_header->sps.poc_cycle_length; i ++) {
      p_header->sps.offset_for_ref_frame[i] = _get_se_golomb(&gb);
    }
  } else if (p_header->sps.poc_type != 2) {
    ERROR("illegal POC type %d\n", p_header->sps.poc_type);
    return AM_STATE_BAD_PARAM;
  }

  p_header->sps.ref_frame_count = _get_ue_golomb_31(&gb);

  if ((p_header->sps.ref_frame_count > (DMAX_PICTURE_COUNT - 2)) ||
      ((uint32_t) p_header->sps.ref_frame_count > 16U)) {
    ERROR("too many reference frames, %d\n", p_header->sps.ref_frame_count);
    return AM_STATE_BAD_PARAM;
  }

  p_header->sps.gaps_in_frame_num_allowed_flag = _get_bits1(&gb);
  p_header->sps.mb_width = _get_ue_golomb(&gb) + 1;
  p_header->sps.mb_height = _get_ue_golomb(&gb) + 1;

  return AM_STATE_OK;
}

/*SimpleCFileWriter::SimpleCFileWriter() :
    m_file(NULL)
{
}

SimpleCFileWriter::~SimpleCFileWriter()
{
  if (m_file) {
    fclose(m_file);
    m_file = NULL;
  }
}

AM_STATE SimpleCFileWriter::open_file(char *name)
{
  if (AM_UNLIKELY(!name)) {
    ERROR("NULL param\n");
    return AM_STATE_BAD_PARAM;
  }

  if (AM_UNLIKELY(m_file)) {
    fclose(m_file);
    m_file = NULL;
  }

  m_file = fopen((const char*) name, "wb");
  if (!m_file) {
    perror("fopen");
    ERROR("open file(%s) fail\n", name);
    return AM_STATE_ERROR;
  }

  return AM_STATE_OK;
}

void SimpleCFileWriter::close_file()
{
  if (m_file) {
    fclose(m_file);
    m_file = NULL;
  }
}

AM_STATE SimpleCFileWriter::write_data(uint8_t *pdata,
                                       TIOSize block_size,
                                       TIOSize block_count)
{
  if (AM_UNLIKELY((!pdata) || (!block_size) || (0 >= block_count))) {
    ERROR("bad parameters pdata %p, block_size %lld, block_count %lld\n",
          pdata,
          block_size,
          block_count);
    return AM_STATE_BAD_PARAM;
  }

  if (AM_UNLIKELY(!m_file)) {
    ERROR("NULL m_file %p\n", m_file);
    return AM_STATE_BAD_STATE;
  }

  if (AM_UNLIKELY(0
      == fwrite((void* )pdata, block_size, block_count, m_file))) {
    ERROR("write data fail, block_size %lld, block_count %lld\n",
          block_size,
          block_count);
    return AM_STATE_IO_ERROR;
  }

  return AM_STATE_OK;
}

AM_STATE SimpleCFileWriter::seekto(TIOSize offset)
{
  if (AM_UNLIKELY(m_file)) {
    fseek(m_file, offset, SEEK_SET);
  } else {
    ERROR("NULL m_file %p\n", m_file);
    return AM_STATE_BAD_STATE;
  }

  return AM_STATE_OK;
}

SimplePosixWriter::SimplePosixWriter() :
    m_fd(-1)
{
}

SimplePosixWriter::~SimplePosixWriter()
{
  if (0 <= m_fd) {
    close(m_fd);
    m_fd = -1;
  }
}

AM_STATE SimplePosixWriter::open_file(char* name)
{
  if (AM_UNLIKELY(!name)) {
    ERROR("NULL param\n");
    return AM_STATE_BAD_PARAM;
  }

  if (AM_UNLIKELY(0 <= m_fd)) {
    close(m_fd);
    m_fd = -1;
  }

  m_fd = open(name, O_WRONLY | O_CREAT | O_TRUNC,
              S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

  if (0 > m_fd) {
    perror("open");
    ERROR("open file(%s) fail\n", name);
    return AM_STATE_IO_ERROR;
  }

  return AM_STATE_OK;
}

void SimplePosixWriter::close_file()
{
  if (0 <= m_fd) {
    close(m_fd);
    m_fd = -1;
  }
}

AM_STATE SimplePosixWriter::write_data(uint8_t* pdata,
                                       TIOSize block_size,
                                       TIOSize block_count)
{
  if (AM_UNLIKELY((!pdata) || (!block_size) || (0 >= block_count))) {
    ERROR("bad parameters pdata %p, block_size %lld, block_count %lld\n",
          pdata,
          block_size,
          block_count);
    return AM_STATE_BAD_PARAM;
  }

  if (AM_UNLIKELY(0 > m_fd)) {
    ERROR("file not opened\n");
    return AM_STATE_BAD_STATE;
  }

  int ret = 0;
  TIOSize remaining_size = block_size * block_count;

  do {
    ret = write(m_fd, pdata, remaining_size);
    if (AM_LIKELY(0 < ret)) {
      remaining_size -= ret;
      pdata += ret;
    } else if (AM_UNLIKELY((errno != EAGAIN) && (errno != EINTR))) {
      PERROR("write");
      ERROR("write fail\n");
      return AM_STATE_IO_ERROR;
    }
  } while (remaining_size);

  return AM_STATE_OK;
}

AM_STATE SimplePosixWriter::seekto(TIOSize offset)
{
  if (AM_UNLIKELY(0 > m_fd)) {
    ERROR("file not opened\n");
    return AM_STATE_BAD_STATE;
  }

  lseek(m_fd, offset, SEEK_SET);

  return AM_STATE_OK;
}
*/
