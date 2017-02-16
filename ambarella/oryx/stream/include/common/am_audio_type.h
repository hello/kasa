/*******************************************************************************
 * am_audio_type.h
 *
 * History:
 *   2014-11-3 - [ypchang] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/
#ifndef AM_AUDIO_TYPE_H_
#define AM_AUDIO_TYPE_H_

/*! @enum AM_AUDIO_CODEC_TYPE
 *  Defines audio codec type, this enumeration is used to indicate which audio
 *  codec should be used when decoding.
 */
enum AM_AUDIO_CODEC_TYPE
{
  /*! Invalid type */
  AM_AUDIO_CODEC_NONE  = -1,

  /*! No need to decode, just pass through */
  AM_AUDIO_CODEC_PASS  = 0,

  /*! AAC codec */
  AM_AUDIO_CODEC_AAC   = 1,

  /*! OPUS codec */
  AM_AUDIO_CODEC_OPUS  = 2,

  /*! S16LE PCM codec */
  AM_AUDIO_CODEC_LPCM  = 3,

  /*! S16BE PCM codec */
  AM_AUDIO_CODEC_BPCM  = 4,

  /*! G.726 codec */
  AM_AUDIO_CODEC_G726  = 5,

  /*! G.711 codec */
  AM_AUDIO_CODEC_G711  = 6,

  /*! Speex codec */
  AM_AUDIO_CODEC_SPEEX = 7,
};

#endif /* AM_AUDIO_TYPE_H_ */
