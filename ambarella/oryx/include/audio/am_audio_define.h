/*******************************************************************************
 * am_audio_define.h
 *
 * History:
 *   2014-12-5 - [ypchang] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/
#ifndef AM_AUDIO_DEFINE_H_
#define AM_AUDIO_DEFINE_H_

/*! @mainpage Oyrx Audio
 *  @section Introduction
 *  Oryx Audio module is used to handle audio device parameters and provides a
 *  unified interface to get audio data.
 */

/*! @file am_audio_define.h
 *  @brief defines audio types
 */

/*! @enum AM_AUDIO_SAMPLE_FORMAT
 *  @brief This enumeration defines audio format
 */
enum AM_AUDIO_SAMPLE_FORMAT
{
  AM_SAMPLE_U8,    //!< unsigned 8 bites
  AM_SAMPLE_ALAW,  //!< G.711 Alaw
  AM_SAMPLE_ULAW,  //!< G.711 ULaw
  AM_SAMPLE_S16LE, //!< Signed 16 bits Little Endian
  AM_SAMPLE_S16BE, //!< Signed 16 bits Big Endian
  AM_SAMPLE_S32LE, //!< Signed 32 bits Little Endian
  AM_SAMPLE_S32BE, //!< Signed 32 bits Bit Endian
  AM_SAMPLE_INVALID//!< Invalid type
};

/*! @enum AM_AUDIO_TYPE
 *  @brief This enumeration audio type
 */
enum AM_AUDIO_TYPE
{
  AM_AUDIO_NULL    = -2,//!<audio null
  AM_AUDIO_LPCM    = 3, //!<audio lpcm
  AM_AUDIO_BPCM    = 4, //!<audio bpcm
  AM_AUDIO_G711A   = 5, //!<audio g711a
  AM_AUDIO_G711U   = 6, //!<audio g711u
  AM_AUDIO_G726_40 = 7, //!<audio g726_40
  AM_AUDIO_G726_32 = 8, //!<audio g726_32
  AM_AUDIO_G726_24 = 9, //!<audio g726_24
  AM_AUDIO_G726_16 = 10,//!<audio g726_16
  AM_AUDIO_AAC     = 11,//!<audio aac
  AM_AUDIO_OPUS    = 12,//!<audio opus
  AM_AUDIO_SPEEX   = 13,//!<audio speex
};

/*! @struct AM_AUDIO_INFO
 *  @brief This structure contains audio info.
 */
struct AM_AUDIO_INFO
{
    uint32_t sample_rate;      //!<sample rate
    uint32_t channels;         //!<audio channels
    uint32_t pkt_pts_increment;//!<Stands for PTS duration of a audio packet
    uint32_t sample_size;      //!<How many bytes an audio sample has
    uint32_t chunk_size;       //!<How many bytes an audio packet has
    int32_t  sample_format;    //!<Audio sample format
    AM_AUDIO_TYPE type;        //!<Audio type @sa AM_AUDIO_TYPE
    void *codec_info;          //!<Audio codec specific information
};

#endif /* AM_AUDIO_DEFINE_H_ */
