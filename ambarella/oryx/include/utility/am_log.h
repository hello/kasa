/*******************************************************************************
 * am_log.h
 *
 * Histroy:
 *  2012-2-27 2012 - [ypchang] created file
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

#ifndef AMLOG_H_
#define AMLOG_H_
#include <errno.h>

/*! @file am_log.h
 *  @brief This file defines functions and macros used to do log.
 */
enum AM_LOG_LEVEL {
  AM_LOG_LEVEL_PRINT  = 0,//!< AM_LOG_LEVEL_PRINT
  AM_LOG_LEVEL_ERROR  = 1,//!< AM_LOG_LEVEL_ERROR
  AM_LOG_LEVEL_WARN   = 2,//!< AM_LOG_LEVEL_WARN
  AM_LOG_LEVEL_STAT   = 3,//!< AM_LOG_LEVEL_STAT
  AM_LOG_LEVEL_NOTICE = 4,//!< AM_LOG_LEVEL_NOTICE
  AM_LOG_LEVEL_INFO   = 5,//!< AM_LOG_LEVEL_INFO
  AM_LOG_LEVEL_DEBUG  = 6,//!< AM_LOG_LEVEL_DEBUG
  AM_LOG_LEVEL_MAX        //!< AM_LOG_LEVEL_MAX
};

enum AM_LOG_TARGET {
  AM_LOG_TARGET_STDERR = 0,
  AM_LOG_TARGET_SYSLOG = 1,
  AM_LOG_TARGET_FILE   = 2,
  AM_LOG_TARGET_NULL   = 3,
  AM_LOG_TARGET_MAX
};

#define AM_LEVEL_ENV_VAR     "AM_ORYX_LOG_LEVEL"
#define AM_CLASS_ENV_VAR     "AM_ORYX_LOG_CLASS"
#define AM_TARGET_ENV_VAR    "AM_ORYX_LOG_TARGET"
#define AM_TIMESTAMP_ENV_VAR "AM_ORYX_LOG_TIMESTAMP"

#ifdef __cplusplus
bool set_log_level(const char *level, bool change = false);
bool set_log_target(const char *target, bool change = false);
#else
bool set_log_level(const char *level, bool change);
bool set_log_target(const char *target, bool change);
#endif

#ifdef __cplusplus
extern "C" {
#endif
bool set_timestamp_enabled(bool enable);
void close_log_file();
void am_debug  (const char *pretty_func, const char *file,
                int                line, const char *format, ...);
void am_info   (const char *pretty_func, const char *file,
                int                line, const char *format, ...);
void am_notice (const char *pretty_func, const char *file,
                int                line, const char *format, ...);
void am_stat   (const char *pretty_func, const char *file,
                int                line, const char *format, ...);
void am_warn   (const char *pretty_func, const char *file,
                int                line, const char *format, ...);
void am_error  (const char *pretty_func, const char *file,
                int                line, const char *format, ...);
void am_print  (const char *format, ...);
#ifdef __cplusplus
}
#endif

/*!
 * @addtogroup define
 * @{
 */

/*!
 * print message in debug level
 */
#define DEBUG(format, arg...) \
  am_debug  (__PRETTY_FUNCTION__, __FILE__, __LINE__, format, ##arg)

/*!
 * print message in info level
 */
#define INFO(format, arg...) \
  am_info   (__PRETTY_FUNCTION__, __FILE__, __LINE__, format, ##arg)

/*!
 * print message in notice level
 */
#define NOTICE(format, arg...) \
  am_notice (__PRETTY_FUNCTION__, __FILE__, __LINE__, format, ##arg)

/*!
 * print message in statistics level
 */
#define STAT(format, arg...) \
  am_stat (__PRETTY_FUNCTION__, __FILE__, __LINE__, format, ##arg)

/*!
 * print message in warn level
 */
#define WARN(format, arg...) \
  am_warn   (__PRETTY_FUNCTION__, __FILE__, __LINE__, format, ##arg)

/*!
 * print message in error level
 */
#define ERROR(format, arg...) \
  am_error  (__PRETTY_FUNCTION__, __FILE__, __LINE__, format, ##arg)

/*!
 * print message into stdout
 */
#define PRINTF(format, arg...) am_print  (format, ##arg)

/*!
 * print message in error level with additional error message
 */
#define PERROR(msg) ERROR("%s: %s", msg, strerror(errno))
/*!
 * @}
 */
#endif /* AMLOG_H_ */
