/*******************************************************************************
 * am_ipc_types.h
 *
 * History:
 *   2014-9-4 - [lysun] created file
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

/*! @file am_ipc_types.h
 *  @brief This file defines Oryx Service IPC data structures.
 */

#ifndef AM_IPC_TYPES_H_
#define AM_IPC_TYPES_H_

/*! Max module name length */
#define AM_MAX_MODULE_NAME_SIZE     128

/*! Max IPC message size
 *
 * please think again if you want to increase AM_MAX_IPC_MESSAGE_SIZE
 * this is because 1024 is big enough for most APIs.
 * if it's not sufficient, then maybe the API definition has some problem,
 * you may need to output the long message to /tmp/1.txt and
 * send file name instead
 */
#define AM_MAX_IPC_MESSAGE_SIZE     1024

/*! Max IPC message queue used */
#define AM_MAX_IPC_MESSAGE_QUE_USED 128

/*! @typedef am_ipc_message_header_t
 *  @brief am_ipc_message_header_s type
 */
/*! @struct am_ipc_message_header_s
 *  @brief IPC Message Header
 */
typedef struct am_ipc_message_header_s
{
    uint64_t time_stamp;  //!<time stamp got by gettimeofday, in micro seconds
    uint32_t msg_id;      //!< Got from macro BUILD_CMD
    uint16_t header_size; //!< Size of message header
    uint16_t payload_size;//!< Size of message payload
} am_ipc_message_header_t;

/*! @typedef am_ipc_message_payload_t
 *  @brief am_ipc_message_payload_s type
 */
/*! @struct am_ipc_message_payload_s
 *  put a few uint32_t as pseudo payload for CMD.
 *  payload is defined by different cmds
 */
typedef struct am_ipc_message_payload_s
{
    uint8_t data[0]; //!< point32_t to start address of payload memory
} am_ipc_message_payload_t;


/*! @typedef am_ipc_message_t
 *  @brief am_ipc_message_s type
 */

/*! @struct am_ipc_message_s
 *  @brief IPC message structure
 *
 * IPC message use simple data structure,
 * data is transferred by POSIX message queue
 */
typedef struct am_ipc_message_s
{
    am_ipc_message_header_t header;   //!< IPC Message Header object
    am_ipc_message_payload_t payload; //!< IPC Message Payload object
} am_ipc_message_t;

/*! @enum AM_IPC_CMD_START_NUM
 *  @brief Defines IPC command start number
 */
enum AM_IPC_CMD_START_NUM
{
  /*! CMD start for internal use by this IPC */
  AM_IPC_CMD_INTERNAL_START = 100,

  /*! CMD start for ORYX services */
  AM_IPC_CMD_FOR_USER_START = 256,

  /*! CMD start for MW APIs */
  AM_IPC_CMD_FOR_MW_API_START = 512,
};

/*! @enum AM_SYS_IPC_INTERNAL_CMD
 *  @brief IPC internal command
 */
enum AM_SYS_IPC_INTERNAL_CMD
{
  /*!
   * IPC connection is done
   */
  _AM_IPC_CMD_ID_CONNECTION_DONE =  AM_IPC_CMD_INTERNAL_START,

  /*!
   * IPC group ID
   */
  _AM_IPC_CMD_ID_GROUP,

  /*!
   * DOWN call, need return
   */
  _AM_IPC_CMD_ID_TEST_ONLY_NEED_RETURN,

  /*!
   * DOWN call, no return
   */
  _AM_IPC_CMD_ID_TEST_ONLY_NO_NEED_RETURN,

  /*!
   * UP call, no return
   */
  _AM_IPC_CMD_ID_TEST_NOTIF,
};

/*! @enum AM_SYS_IPC_MSG_MAP_BITS
 *
 *  - 31 .................................. 0
 *  - Bit    31: call direction
 *    - 0: DOWN (app call mw's function)
 *    .
 *    - 1: UP   (mw notify app or return cmd value)
 *    .
 *
 *  - Bit    30: return indicator
 *    - 0: no need return (simple notification or return value msg)
 *    .
 *    - 1: need return    (like app call mw's function)
 *    .
 *
 *  - Bit    29: group CMD indicator
 *    - 0: not group CMD
 *    .
 *    - 1: group CMD
 *    .
 *
 *  - Bit     0: generic group
 *  - Bit  1~15: group id defined
 *  - Bit 25~28: group CMD id
 *  - Bit 24~16: type
 *  - Bit 15~ 0: CMD id
 */

enum AM_SYS_IPC_MSG_MAP_BITS
{
  AM_SYS_IPC_MSG_DIRECTION_BIT      = 31, //!< Bit 31
  AM_SYS_IPC_MSG_NEED_RETURN_BIT    = 30, //!< Bit 30
  AM_SYS_IPC_MSG_GROUP_BIT          = 29, //!< Bit 29
  AM_SYS_IPC_MSG_GROUP_ID_START_BIT = 25, //!< Bit 25
  AM_SYS_IPC_MSG_TYPE_BIT           = 16, //!< Bit 16
};

/*! @enum AM_SYS_IPC_MSG_DIRECTION
 *  @brief Define IPC message direction
 */
enum AM_SYS_IPC_MSG_DIRECTION
{
  AM_IPC_DIRECTION_DOWN = 0, //!< App calls middlware's function
  AM_IPC_DIRECTION_UP   = 1, //!< Middleware notifies App or returns value
};

/*! @enum AM_SYS_IPC_MSG_NEED_RETURN
 *  @brief Define IPC message return type
 */
enum AM_SYS_IPC_MSG_NEED_RETURN
{
  AM_IPC_NO_NEED_RETURN = 0, //!< NO RETURN
  AM_IPC_NEED_RETURN    = 1, //!< HAS RETURN
};

/*! @enum AM_SYS_IPC_MSG_IS_GROUP_CMD
 *  @brief Define IPC message grouping type
 */
enum AM_SYS_IPC_MSG_IS_GROUP_CMD
{
  AM_IPC_NOT_GROUP_CMD = 0, //!< NOT GROUP CMD
  AM_IPC_GROUP_CMD = 1,     //!< GROUP CMD
};

/* use IPC MODULE ID to define GROUP ID
 * please note GROUP ID by current definition is 0~15 (only 16 values)
 */

/*! @enum AM_SYS_IPC_MSG_MASK
 *  @brief  IPC message bits mask
 */
enum AM_SYS_IPC_MSG_MASK
{
  AM_SYS_IPC_MSG_CMD_ID_MASK = 0xFFFF,   //!< CMD ID bits mask
  AM_SYS_IPC_MSG_DIRECTION_MASK = 0x1,   //!< Message direction bits mask
  AM_SYS_IPC_MSG_NEED_RETURN_MASK = 0x1, //!< Return type bits mask
  AM_SYS_IPC_MSG_IS_GROUP_CMD_MASK = 0x1,//!< Is CMD group bits mask
  AM_SYS_IPC_MSG_GROUP_CMD_ID_MASK = 0xF,//!< Group CMD ID bits mask
  AM_SYS_IPC_MSG_TYPE_MASK = 0xFF,       //!< Message type bits mask
};

/*! Helper macro used to build IPC message ID */
#define BUILD_IPC_MSG_ID(cmd_id, direction, need_return)                       \
((((uint32_t)cmd_id) & AM_SYS_IPC_MSG_CMD_ID_MASK)                           | \
    ((((uint32_t)direction) & AM_SYS_IPC_MSG_DIRECTION_MASK)                   \
        << AM_SYS_IPC_MSG_DIRECTION_BIT)                                     | \
        ((((uint32_t)need_return) & AM_SYS_IPC_MSG_NEED_RETURN_MASK)           \
            << AM_SYS_IPC_MSG_NEED_RETURN_BIT))

/*! Helper macro used to build IPC message ID with message type */
#define BUILD_IPC_MSG_ID_WITH_TYPE(cmd_id, direction, need_return, type)       \
((((uint32_t)cmd_id) & AM_SYS_IPC_MSG_CMD_ID_MASK)                           | \
    ((((uint32_t)direction) & AM_SYS_IPC_MSG_DIRECTION_MASK)                   \
        << AM_SYS_IPC_MSG_DIRECTION_BIT)                                     | \
        ((((uint32_t)need_return) & AM_SYS_IPC_MSG_NEED_RETURN_MASK)           \
            << AM_SYS_IPC_MSG_NEED_RETURN_BIT)                               | \
            ((((uint32_t)type) & AM_SYS_IPC_MSG_TYPE_MASK)                     \
                  << AM_SYS_IPC_MSG_TYPE_BIT))


/*! Helper macro used to build CMD group
 *
 * Build CMD group, a group is a cmd group which can transfer more than one CMDs
 * by single IPC, the purpose is to reduce IPC latency when app uses a series of
 * CMDs frequently
 */
#define BUILD_IPC_MSG_ID_GROUP(cmd_id, direction, need_return)                 \
((((uint32_t)cmd_id) & AM_SYS_IPC_MSG_CMD_ID_MASK)                           | \
    ((((uint32_t)direction) & AM_SYS_IPC_MSG_DIRECTION_MASK)                   \
        << AM_SYS_IPC_MSG_DIRECTION_BIT)                                     | \
        ((((uint32_t)need_return) & AM_SYS_IPC_MSG_NEED_RETURN_MASK)           \
            << AM_SYS_IPC_MSG_NEED_RETURN_BIT)                               | \
            (1 <<AM_SYS_IPC_MSG_GROUP_BIT))


/*! Helper macro to get IPC message direction */
#define GET_IPC_MSG_DIRECTION(msg_id)                                          \
(((msg_id)>>AM_SYS_IPC_MSG_DIRECTION_BIT) & AM_SYS_IPC_MSG_DIRECTION_MASK)

/*! Helper macro to get IPC message return type */
#define GET_IPC_MSG_NEED_RETURN(msg_id)                                        \
(((msg_id)>>AM_SYS_IPC_MSG_NEED_RETURN_BIT) & AM_SYS_IPC_MSG_DIRECTION_MASK)

/*! Helper macro to get IPC message CMD ID */
#define GET_IPC_MSG_CMD_ID(msg_id)                                             \
((msg_id) & AM_SYS_IPC_MSG_CMD_ID_MASK)

/*! Helper macro to get IPC message group type */
#define GET_IPC_MSG_IS_GROUP(msg_id)                                           \
(((msg_id) >> AM_SYS_IPC_MSG_GROUP_BIT ) & AM_SYS_IPC_MSG_IS_GROUP_CMD_MASK)

/*! Helper macro to get IPC message CMD group ID */
#define GET_IPC_MSG_GROUP_ID(msg_id)                                           \
(((msg_id) >> AM_SYS_IPC_MSG_GROUP_ID_START_BIT) &                             \
    AM_SYS_IPC_MSG_GROUP_CMD_ID_MASK)

/*! Helper macro to set IPC message to UP direction */
#define SET_IPC_MSG_UP_DIRECTION(msg_id)                                       \
((msg_id)  |  ( 1 << AM_SYS_IPC_MSG_DIRECTION_BIT) )

/*! Helper macro to set IPC message to DOWN direction */
#define SET_IPC_MSG_DOWN_DIRECTION(msg_id)                                     \
((msg_id) & (~( 1 << AM_SYS_IPC_MSG_DIRECTION_BIT) ))

/*! Helper macro to get IPC message type */
#define GET_IPC_MSG_TYPE(msg_id)                                               \
(((msg_id)>>AM_SYS_IPC_MSG_TYPE_BIT) & AM_SYS_IPC_MSG_TYPE_MASK)

/*! Pre-defined IPC message ID
 *
 * This message is sent by sender to receiver , to update receiver's status
 */
#define AM_IPC_CMD_ID_CONNECTION_DONE                                          \
BUILD_IPC_MSG_ID(_AM_IPC_CMD_ID_CONNECTION_DONE,                               \
                 AM_IPC_DIRECTION_DOWN,                                        \
                 AM_IPC_NO_NEED_RETURN)

/*!
 *  Pre-defined IPC CMD group type
 */
#define AM_IPC_CMD_ID_GROUP_CMD_GENERIC                                        \
BUILD_IPC_MSG_ID_GROUP(_AM_IPC_CMD_ID_GROUP,                                   \
                       AM_IPC_DIRECTION_DOWN,                                  \
                       AM_IPC_NEED_RETURN)

#define AM_IPC_CMD_ID_TEST_ONLY                                                \
BUILD_IPC_MSG_ID(_AM_IPC_CMD_ID_TEST_ONLY_NEED_RETURN,                         \
                 AM_IPC_DIRECTION_DOWN,                                        \
                 AM_IPC_NEED_RETURN)

#define AM_IPC_CMD_ID_TEST_ONLY_NO_RETURN                                      \
BUILD_IPC_MSG_ID(_AM_IPC_CMD_ID_TEST_ONLY_NO_NEED_RETURN,                      \
                 AM_IPC_DIRECTION_DOWN,                                        \
                 AM_IPC_NO_NEED_RETURN)

#define AM_IPC_CMD_ID_TEST_NOTIF                                               \
BUILD_IPC_MSG_ID(_AM_IPC_CMD_ID_TEST_NOTIF,                                    \
                 AM_IPC_DIRECTION_UP,                                          \
                 AM_IPC_NO_NEED_RETURN)

/*! @enum AM_IPC_CMD_ERROR_CODE
 *  @brief IPC Command Error Code Definition
 */
enum AM_IPC_CMD_ERROR_CODE
{
  /*!
   * OK, Success!
   */
  AM_IPC_CMD_SUCCESS          =  0,

  /*!
   * Unsupported IPC method
   */
  AM_IPC_CMD_ERR_UNSUPPORTED  = -1,

  /*!
   * Invalid (Input) CMD argument
   */
  AM_IPC_CMD_ERR_INVALID_ARG  = -2,

  /*!
   * Invalid IPC
   */
  AM_IPC_CMD_ERR_INVALID_IPC  = -3,

  /*!
   * The CMD can be ignored and it is harmless
   */
  AM_IPC_CMD_ERR_IGNORE       = -4,

  /*!
   * System is busy or not ready and cannot handle this CMD,
   * user may try it again
   */
  AM_IPC_CMD_ERR_AGAIN        = -5,

  /*!
   * No access right to call this CMD
   */
  AM_IPC_CMD_ERR_ACCESS       = -6,

  /*!
   * Memory is not sufficient enough during the IPC call
   */
  AM_IPC_CMD_ERR_NOMEM        = -7,

  /*!
   * This method is obsolete now, and should NEVER be used
   */
  AM_IPC_CMD_ERR_OBSOLETE     = -8,

  /*!
   * An internal error of system has been found
   */
  AM_IPC_CMD_ERR_INTERNAL     = -9,

  /*!
   * An internal error of timeout
   */
  AM_IPC_CMD_ERR_TIMEOUT      = -10,

  /*!
   * Unknown error
   */
  AM_IPC_CMD_ERR_UNKNOWN      = -11,
};

/*! @example test_ipc_base.cpp
 */

/*! @example test_ipc_cmd.cpp
 */

/*! @example test_ipc_notif.cpp
 */

/*! @example test_ipc_sync_cmd.cpp
 */

/*! @example test_ipc_up_notif.cpp
 */
#endif /* AM_IPC_TYPES_H_ */
