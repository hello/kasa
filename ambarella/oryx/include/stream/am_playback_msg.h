/*******************************************************************************
 * am_playback_msg.h
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
/*! @file am_playback_msg.h
 *  @brief defines playback engine message types, data structures and callbacks
 */
#ifndef AM_PLAYBACK_MSG_H_
#define AM_PLAYBACK_MSG_H_

/*! @enum AM_PLAYBACK_MSG
 *  @brief playback engine message
 *         These message types indicate playback engine status.
 */
enum AM_PLAYBACK_MSG
{
  AM_PLAYBACK_MSG_START_OK, //!< START is OK
  AM_PLAYBACK_MSG_PAUSE_OK, //!< PAUSE is OK
  AM_PLAYBACK_MSG_STOP_OK,  //!< STOP is OK
  AM_PLAYBACK_MSG_ERROR,    //!< Operation Error
  AM_PLAYBACK_MSG_ABORT,    //!< Engine is aborted
  AM_PLAYBACK_MSG_EOS,      //!< End of stream
  AM_PLAYBACK_MSG_TIMEOUT,  //!< Operation timeout
  AM_PLAYBACK_MSG_NULL      //!< Invalid message
};

/*! @struct AMPlaybackMsg
 *  @brief This structure contains message that sent by playback engine.
 *  It is usually used by applications to retrieve engine status.
 */
struct AMPlaybackMsg
{
    AM_PLAYBACK_MSG msg;  //!< engine message
    void           *data; //!< user data
    AMPlaybackMsg() :
      msg(AM_PLAYBACK_MSG_NULL),
      data(nullptr){}
};

/*! @typedef AMPlaybackCallback
 *  @brief playback callback function type
 *         Use this function to get message sent by playback engine, usually
 *         this message contains engine status.
 * @param msg AMPlaybackMsg reference
 * @sa AMPlaybackMsg
 */
typedef void (*AMPlaybackCallback)(AMPlaybackMsg &msg);

#endif /* AM_PLAYBACK_MSG_H_ */
