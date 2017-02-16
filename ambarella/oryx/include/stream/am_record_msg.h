/*******************************************************************************
 * am_record_msg.h
 *
 * History:
 *   2014-12-2 - [ypchang] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/

/*! @file am_record_msg.h
 *  @brief defines record engine message types, data structures and callbacks
 */

#ifndef AM_RECORD_MSG_H_
#define AM_RECORD_MSG_H_

/*! @enum AM_RECORD_MSG
 *  @brief record engine message
 *         These message types indicate record engine status.
 */
enum AM_RECORD_MSG
{
  AM_RECORD_MSG_START_OK, //!< START is OK
  AM_RECORD_MSG_STOP_OK,  //!< STOP is OK
  AM_RECORD_MSG_ERROR,    //!< ERROR has occurred
  AM_RECORD_MSG_ABORT,    //!< Engine is aborted
  AM_RECORD_MSG_EOS,      //!< End of Stream
  AM_RECORD_MSG_OVER_FLOW,//!< IO Over flow
  AM_RECORD_MSG_TIMEOUT,  //!< Operation timeout
  AM_RECORD_MSG_NULL      //!< Invalid message
};

/*! @struct AMRecordMsg
 *  @brief This structure contains message that sent by record engine.
 *  It is usually used by applications to retrieve engine status.
 */
struct AMRecordMsg
{
    AM_RECORD_MSG msg;  //!< engine message
    void         *data; //!< user data
    AMRecordMsg() :
      msg(AM_RECORD_MSG_NULL),
      data(nullptr){}
};

/*! @typedef AMRecordCallback
 *  @brief record callback function type
 *         Use this function to get message sent by record engine, usually
 *         this message contains engine status.
 * @param msg AMRecordMsg reference
 * @sa AMRecordMsg
 */
typedef void (*AMRecordCallback)(AMRecordMsg &msg);

#endif /* AM_RECORD_MSG_H_ */
