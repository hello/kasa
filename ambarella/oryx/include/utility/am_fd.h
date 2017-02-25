/*******************************************************************************
 * am_fd.h
 *
 * History:
 *   2015-1-4 - [ypchang] created file
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
