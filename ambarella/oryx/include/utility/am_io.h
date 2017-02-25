/*******************************************************************************
 * am_io.h
 *
 * History:
 *   2016年9月26日 - [ypchang] created file
 *
 * Copyright (c) 2015 Ambarella, Inc.
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
#ifndef AM_IO_H_
#define AM_IO_H_
#include <stdint.h>
/*! @file am_io.h
 *  @brief This file define the prototype of am_read and am_write
 */

/*! @defgroup io Standard IO Operation Functions
 * These functions implements standard IO Operation.
 * @{
 */
#ifdef __cplusplus
extern "C" {
#endif
/*! Wraps ::read function and handles errors
 * @param fd file descriptor
 * @param buf pointer to data buffer
 * @param data_len data length to read
 * @param retry_count retry count when fail to read
 * @return -1 or actual bytes read from fd
 */
ssize_t am_read(int fd,
                void *buf,
                size_t read_len,
                uint32_t retry_count = 1);

/*! Wraps ::write function and handles errors
 * @param fd file descriptor
 * @param data pointer to data buffer
 * @param data_len data length to write
 * @param retry_count retry count when fail to write
 * @return -1 or actual bytes write to fd
 */
ssize_t am_write(int fd,
                 const void *data,
                 size_t data_len,
                 uint32_t retry_count = 1);
#ifdef __cplusplus
}
#endif
/*!
 * @}
 */
#endif /* AM_IO_H_ */
