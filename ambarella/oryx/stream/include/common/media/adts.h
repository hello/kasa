/*******************************************************************************
 * adts.h
 *
 * History:
 *   2014-10-27 - [ypchang] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/
#ifndef ADTS_H_
#define ADTS_H_

struct AdtsHeader
{
#ifdef BIGENDIAN
    /* Byte 7 low bit to high bit*/
    AM_U8 number_of_aac_frame     :2;
    AM_U8 buffer_fullness_6to1    :6;

    /* Byte 6 low bit to high bit*/
    AM_U8 buffer_fullness_11to7   :5;
    AM_U8 framelength_3to1        :3;

    /* Byte 5 low bit to high bit*/
    AM_U8 framelength_11to4       :8;

    /* Byte 4 low bit to high bit*/
    AM_U8 framelength_13to12      :2;
    AM_U8 copyright_start         :1;
    AM_U8 copyrighted_stream      :1;
    AM_U8 home                    :1;
    AM_U8 originality             :1;
    AM_U8 channel_conf_2to1       :2;

    /* Byte 3 low bit to high bit*/
    AM_U8 channel_conf_3          :1;
    AM_U8 private_stream          :1;
    AM_U8 sample_freqency_index   :4;
    AM_U8 profile                 :2;

    /* Byte 2 low bit to high bit*/
    AM_U8 protection              :1;
    AM_U8 layer                   :2;
    AM_U8 mpeg_version            :1;
    AM_U8 syncword_4to1           :4;

    /* Byte 1 low bit to high bit*/
    AM_U8 syncword_12to5          :8;
#else
    /* Byte 1 low bit to high bit*/
    uint8_t syncword_12to5        :8;

    /* Byte 2 low bit to high bit*/
    uint8_t protection            :1;
    uint8_t layer                 :2;
    uint8_t mpeg_version          :1;
    uint8_t syncword_4to1         :4;

    /* Byte 3 low bit to high bit*/
    uint8_t channel_conf_3        :1;
    uint8_t private_stream        :1;
    uint8_t sample_freqency_index :4;
    uint8_t profile               :2;

    /* Byte 4 low bit to high bit*/
    uint8_t framelength_13to12    :2;
    uint8_t copyright_start       :1;
    uint8_t copyrighted_stream    :1;
    uint8_t home                  :1;
    uint8_t originality           :1;
    uint8_t channel_conf_2to1     :2;

    /* Byte 5 low bit to high bit*/
    uint8_t framelength_11to4     :8;

    /* Byte 6 low bit to high bit*/
    uint8_t buffer_fullness_11to7 :5;
    uint8_t framelength_3to1      :3;

    /* Byte 7 low bit to high bit*/
    uint8_t number_of_aac_frame   :2;
    uint8_t buffer_fullness_6to1  :6;
#endif
    bool is_sync_word_ok()
    {
      return (0x0FFF == (0x0000 | (syncword_12to5 << 4) | syncword_4to1));
    }

    uint16_t frame_length()
    {
      return (uint16_t) (0x0000 | (framelength_13to12 << 11)
          | (framelength_11to4 << 3) | (framelength_3to1));
    }

    uint16_t buffer_fullness()
    {
      return (uint16_t) (0x0000 | (buffer_fullness_11to7 << 6)
          | (buffer_fullness_6to1));
    }

    uint8_t protection_absent()
    {
      return (uint8_t) (0x00 | protection);
    }

    uint8_t aac_frame_number()
    {
      return (uint8_t) (0x00 | number_of_aac_frame);
    }

    uint8_t aac_audio_object_type()
    {
      return (uint8_t) ((0x00 | profile) + 1);
    }

    uint8_t aac_frequency_index()
    {
      return (uint8_t) (0x00 | sample_freqency_index);
    }

    uint8_t aac_channel_conf()
    {
      return (uint8_t) (0x00 | ((channel_conf_3 << 2) | channel_conf_2to1));
    }
};

struct ADTS
{
    uint32_t size;
    uint8_t *addr;
};

#endif /* ADTS_H_ */
