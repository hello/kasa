/*******************************************************************************
 * am_media_service_msg.h
 *
 * History:
 *   2015-3-24 - [ccjing] created file
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
/*! @file am_media_service_msg.h
 *  @brief This header file contains enumerations and structs which is used
 *         in the media service.
 */
#ifndef AM_MEDIA_SERVICE_MSG_H_
#define AM_MEDIA_SERVICE_MSG_H_
/*! @addtogroup airapi-datastructure-media
 *  @{
 */
#include "am_playback_info.h"
#define MEDIA_SERVICE_UNIX_DOMAIN_NAME  "/run/oryx/media-server.socket"
/*! @enum AM_MEDIA_NET_STATE
 *  @brief media service net state.
 *         enumerate the network state of the media service.
 */
enum AM_MEDIA_NET_STATE
{
  AM_MEDIA_NET_OK,           //!< Indicate the network state is OK.
  AM_MEDIA_NET_ERROR,        //!< Indicate the network state is Error.
  AM_MEDIA_NET_SLOW,         //!< Indicate the network is too slow.
  AM_MEDIA_NET_PEER_SHUTDOWN,//!< Indicate the network is shutdown by peer
  AM_MEDIA_NET_BADFD,        //!< Indicate the file descriptor is invalid.
};

/*! @enum AM_MEDIA_SERVICE_MSG_TYPE
 *  @brief enumerate the msg type used in the media service
 */
enum AM_MEDIA_SERVICE_MSG_TYPE
{
  AM_MEDIA_SERVICE_MSG_ACK,        //!< The type of ack message.
  AM_MEDIA_SERVICE_MSG_AUDIO_INFO, //!< The type of audio info message.
  AM_MEDIA_SERVICE_MSG_UDS_URI,    //!< The type of Unix Domain URI message.
  AM_MEDIA_SERVICE_MSG_PLAYBACK_ID,//!< The type of playback id.
  AM_MEDIA_SERVICE_MSG_STOP,       //!< The type of stop message.
};

/*! @enum AM_MEDIA_SERVICE_CLIENT_PROTO
 *  @brief enumerate the protocol of service which is connected with media
 *         service.
 */
enum AM_MEDIA_SERVICE_CLIENT_PROTO
{
  AM_MEDIA_SERVICE_CLIENT_PROTO_UNKNOWN = -1,//!<Unknown prototol
  AM_MEDIA_SERVICE_CLIENT_PROTO_SIP = 0,     //!<Sip protocol
  AM_MEDIA_SERVICE_CLIENT_PROTO_NUM,         //!<The number of protocols
};

/*! @struct AMMediaServiceMsgBase
 *  @brief This struct is used as a message base when communicate with media
 *         service.
 */
struct AMMediaServiceMsgBase
{
    uint32_t                   length;//!<The size of the whole message.
    AM_MEDIA_SERVICE_MSG_TYPE  type;  //!<The type of the message.
};

/*! @struct AMMediaServiceMsgACK
 *  @brief This struct is used as an ack message when communicate with media
 *         service.
 */
struct AMMediaServiceMsgACK
{
    AMMediaServiceMsgBase         base; //!<AMMediaServiceMsgBase struct.
    /*! The protocol of the client who sends the message.
     */
    AM_MEDIA_SERVICE_CLIENT_PROTO proto;//!<The protocol of the client.
};

/*! @struct AMMediaServiceMsgSTOP
 *  @brief This struct is used as a stop message when communicate with media
 *         service. The media service will stop audio playback when receive
 *         this message.
 */
struct AMMediaServiceMsgSTOP
{
    AMMediaServiceMsgBase base;//!<AMMediaServiceMsgBase struct.
    int32_t               playback_id; //!<playback id>
};

/*! @struct AMMediaServiceMsgAudioINFO
 *  @brief This struct is used as an audio info message when sip service
 *         communicate with media service. The data in the message can be
 *         used to initialize playback moudle to playback audio data which
 *         is sent by rtp protocol in the media service.
 */
struct AMMediaServiceMsgAudioINFO
{
    AMMediaServiceMsgBase   base;        //!<AMMediaServiceMsgBase struct.
    uint32_t                udp_port;    //!<The udp port used by rtp protocol.
    AM_AUDIO_TYPE           audio_format;//!<The audio format.
    uint32_t                sample_rate; //!<The audio sample rate..
    uint32_t                channel;     //!<The audio channel.
    AM_PLAYBACK_IP_DOMAIN   ip_domain;   //!<AM_PLAYBACK_IPV4 or AM_PLAYBACK_IPV6
};

/*! @struct AMMediaServiceMsgUdsURI
 *  @brief This struct is used as an Unix Domain Socket URI message when sip
 *         service communicate with media service. The data in the message can
 *         be used to initialize playback moudle to playback audio data which
 *         is sent by sip through Unix Domain Socket.
 */
struct AMMediaServiceMsgUdsURI
{
  AMMediaServiceMsgBase   base;        //!<AMMediaServiceMsgBase struct.
  AM_AUDIO_TYPE           audio_type;  //!<Audio type>
  uint32_t                sample_rate; //!<Audio sample rate>
  uint32_t                channel;     //!<Audio channel>
  int32_t                 playback_id; //!<playback id>
  char                    name[64];    //!<unix domain server name>
};

/*! @struct AMMediaServiceMsgPlaybackID
 *  @brief This struct is used to identify which playback will play the media
 *         and tell the sip client should stop which playback.
 */
struct AMMediaServiceMsgPlaybackID
{
  AMMediaServiceMsgBase   base;        //!<AMMediaServiceMsgBase struct.
  int32_t                 playback_id; //!<playback id>
};

/*! @union AMMediaServiceMsgBlock
 *  @brief The union of all kinds of msg.
 */
union AMMediaServiceMsgBlock
{
    struct AMMediaServiceMsgACK        msg_ack;   //!<ack message.
    struct AMMediaServiceMsgSTOP       msg_stop;  //!<stop message.
    struct AMMediaServiceMsgAudioINFO  msg_info;  //!<audio info message.
    struct AMMediaServiceMsgUdsURI     msg_uds;   //!<unix domain URI message
    struct AMMediaServiceMsgPlaybackID msg_playid;//!<playback id>
};

/*!
 * @}
 */
#endif /* AM_MEDIA_SERVICE_MSG_H_ */
