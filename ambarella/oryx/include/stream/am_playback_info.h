/*******************************************************************************
 * am_playback_info.h
 *
 * History:
 *   Apr 29, 2015 - [ccjing] created file
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
  AM_PLAYBACK_URI_UNIX_DOMAIN, //!<URI is is unix domain url>
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
    AM_AUDIO_TYPE           audio_type;  //!<Audio type
    uint32_t                sample_rate; //!<Audio sample rate
    uint32_t                channel;     //!<Audio channel
    AM_PLAYBACK_IP_DOMAIN   ip_domain;   //!<IP address domain
};

struct AMPlaybackUnixDomainUri
{
    AM_AUDIO_TYPE           audio_type;  //!<Audio type>
    uint32_t                sample_rate; //!<Audio sample rate>
    uint32_t                channel;     //!<Audio channel>
    int32_t                 playback_id; //!<playback id>
    char                    name[64];    //!<unix domain name>
};

/*! @union AMPlaybackMedia
 *  @brief The union of media related information,
 *         the media is either a file or a RTP URL
 */
union AMPlaybackMedia
{
    /*! file path */
    char                    file[PATH_MAX];
    /*! RTP, see @ref AMPlaybackRtpUri for more information */
    AMPlaybackRtpUri        rtp;
    /*! UNIX DOMAIN, see @ref AMPlaybackRtpUri for more information */
    AMPlaybackUnixDomainUri unix_domain;
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
