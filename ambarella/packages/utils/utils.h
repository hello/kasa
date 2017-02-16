/*******************************************************************************
 * utils.h
 *
 * History:
 *  Jun 5, 2013 - [qianshen] created file
 *
 * Copyright (C) 2012-2016, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
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
  LOG_ERROR = 0,
  LOG_INFO = 1,
  LOG_DEBUG = 2,
  LOG_TRACE = 3,
  LOG_MAX
} LOG_LEVEL;

int set_log(int level, char *target);

int log_error(const char* func, const char *format, ...);
int log_info(const char* func, const char *format, ...);
int log_debug(const char* func, const char *format, ...);
int log_trace(const char* func, const char *format, ...);

void start_timing(void);
void stop_timing(void);
void perf_report(void);

#endif /* UTILS_H_ */
