/*******************************************************************************
 * am_playback_info.h
 *
 * History:
 *   Apr 29, 2015 - [ccjing] created file
 *
 * Copyright (C) 2008-2015, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/
#ifndef AM_PLAYBACK_INFO_H_
#define AM_PLAYBACK_INFO_H_
#include <limits.h>
#include "am_audio_define.h"
/*! @enum AM_PLAYBACK_URI_TYPE
 *  @brief enumerate playback URI type
 */
enum AM_PLAYBACK_URI_TYPE
{
  AM_PLAYBACK_URI_FILE,//!< URI is a file path
  AM_PLAYBACK_URI_RTP, //!< URI is a RTP URL
};

/*! @enum AM_PLAYBACK_IP_DOMAIN
 *  @brief Defines IP domain
 */
enum AM_PLAYBACK_IP_DOMAIN
{
  AM_PLAYBACK_IPV4 = 4, //!< Network domain is IPv4
  AM_PLAYBACK_IPV6 = 6, //!< Network domain is IPv6
  AM_PLAYBACK_UNKNOWN,  //!< Network domain is unknown
};

/*! @struct AMPlaybackRtpUri
 *  @brief Defines RTP URI related information
 */
struct AMPlaybackRtpUri
{
    uint32_t                udp_port;    //!<UDP port number
    AM_AUDIO_TYPE           audio_format;//!<Audio type
    uint32_t                sample_rate; //!<Audio sample rate
    uint32_t                channel;     //!<Audio channel
    AM_PLAYBACK_IP_DOMAIN   ip_domain;   //!<IP address domain
};

/*! @union AMPlaybackMedia
 *  @brief The union of media related information,
 *         the media is either a file or a RTP URL
 */
union AMPlaybackMedia
{
    /*! file path */
    char                 file[PATH_MAX];
    /*! RTP, see @ref AMPlaybackRtpUri for more information */
    AMPlaybackRtpUri     rtp;
};

/*! @struct AMPlaybackUri
 *  @brief defines the URI information
 */
struct AMPlaybackUri
{
    /*! The URI type is structure contains.
     *  see @ref AM_PLAYBACK_URI_TYPE
     */
    AM_PLAYBACK_URI_TYPE type;
    /*! Media URI information */
    AMPlaybackMedia      media;
    /*! Constructor of this structure */
    AMPlaybackUri() :
      type(AM_PLAYBACK_URI_FILE)
    {
      memset(&media, 0, sizeof(AMPlaybackMedia));
    }
};

#endif /* AM_PLAYBACK_INFO_H_ */
