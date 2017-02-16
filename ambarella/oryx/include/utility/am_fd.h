/*******************************************************************************
 * am_fd.h
 *
 * History:
 *   2015-1-4 - [ypchang] created file
 *
 * Copyright (C) 2008-2015, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/
#ifndef ORYX_INCLUDE_UTILITY_AM_FD_H_
#define ORYX_INCLUDE_UTILITY_AM_FD_H_

/*! @file am_fd.h
 *  @brief This file define the prototype of send_fd and recv_fd.
 */

/*! @defgroup fd Socket File Descriptor Sender and Receiver
 * These functions are used to pass file descriptor between processes through
 * Unix Domain Socket.
 * @{
 */
#ifdef __cplusplus
extern "C" {
#endif
/*! Send sendfd through Unix Domain Socket fd.
 * @param fd a valid Unix Domain Socket
 * @param sendfd the socket to be sent through fd.
 * @return true if success, otherwise return false.
 */
bool send_fd(int fd, int sendfd);

/*! Receive a socket from Unix Domain Socket fd
 *  and save it into recvfd pointed address.
 * @param fd a valid Unix Domain Socket
 * @param recvfd a pointer pointing to the address to store the socket number
 * @return true if success, otherwise return false.
 */
bool recv_fd(int fd, int *recvfd);
#ifdef __cplusplus
}
#endif

/*!
 * @}
 */
#endif /* ORYX_INCLUDE_UTILITY_AM_FD_H_ */
