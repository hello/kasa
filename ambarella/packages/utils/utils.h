/*******************************************************************************
 * utils.h
 *
 * History:
 *  Jun 5, 2013 - [qianshen] created file
 *
 * Copyright (c) 2016 Ambarella, Inc.
 *
 * This file and its contents ( "Software" ) are protected by intellectual
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

#ifndef UTILS_H_
#define UTILS_H_

#ifndef ERROR
#define ERROR(format, arg...)       log_error(__PRETTY_FUNCTION__, format, ##arg)
#endif

#ifndef INFO
#define INFO(format, arg...)        log_info(__PRETTY_FUNCTION__, format, ##arg)
#endif

#ifndef DEBUG
#define DEBUG(format, arg...)       log_debug(__PRETTY_FUNCTION__, format, ##arg)
#endif

#ifndef TRACE
#define TRACE(format, arg...)       log_trace(__PRETTY_FUNCTION__, format, ##arg)
#endif

#define likely(x)   (__builtin_expect(!!(x), 1))
#define unlikely(x) (__builtin_expect(!!(x), 0))

#ifndef ROUND_UP
#define ROUND_UP(x, n)	( ((x)+(n)-1u) & ~((n)-1u) )
#endif

#ifndef ROUND_DOWN
#define ROUND_DOWN(size, align)	((size) & ~((align) - 1))
#endif

#ifndef MAX
#define MAX(x, y)       ((x) > (y) ? (x) : (y))
#endif

#ifndef MIN
#define MIN(x, y)       ((x) < (y) ? (x) : (y))
#endif

typedef enum {
  AMBA_LOG_ERROR = 0,
  AMBA_LOG_INFO = 1,
  AMBA_LOG_DEBUG = 2,
  AMBA_LOG_TRACE = 3,
  AMBA_LOG_MAX
} AMBA_LOG_LEVEL;

int set_log(int level, char *target);

int log_error(const char* func, const char *format, ...);
int log_info(const char* func, const char *format, ...);
int log_debug(const char* func, const char *format, ...);
int log_trace(const char* func, const char *format, ...);

void start_timing(void);
void stop_timing(void);
void perf_report(void);

#endif /* UTILS_H_ */
