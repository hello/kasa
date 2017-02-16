/*******************************************************************************
 * am_media_service_msg.h
 *
 * History:
 *   2015-3-24 - [ccjing] created file
 *
 * Copyright (C) 2008-2015, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
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
  AM_MEDIA_SERVICE_MSG_ACK,       //!< The type of ack message.
  AM_MEDIA_SERVICE_MSG_AUDIO_INFO,//!< The type of audio info message.
  AM_MEDIA_SERVICE_MSG_STOP,      //!< The type of stop message.
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

/*! @union AMMediaServiceMsgBlock
 *  @brief The union of all kinds of msg.
 */
union AMMediaServiceMsgBlock
{
    struct AMMediaServiceMsgACK        msg_ack; //!<ack message.
    struct AMMediaServiceMsgSTOP       msg_stop;//!<stop message.
    struct AMMediaServiceMsgAudioINFO  msg_info;//!<audio info message.
};

/*!
 * @}
 */
#endif /* AM_MEDIA_SERVICE_MSG_H_ */
