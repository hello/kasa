/*
 *
 * mw_defines.h
 *
 * History:
 *	2012/12/10 - [Jingyang Qiu] Created this file
 *
 * Copyright (C) 2015 Ambarella, Inc.
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
 */

#ifndef _MW_DEFINES_H_
#define _MW_DEFINES_H_

extern int G_mw_log_level;

/*********************************
 *
 *	for debug
 *
 *********************************/


#include "stdio.h"

#define	MW_PRINT(mylog, LOG_LEVEL, format, args...)		do {		\
							if (mylog >= LOG_LEVEL) {			\
								printf(format, ##args);			\
							}									\
						} while (0)

#define MW_ERROR(format, args...)			MW_PRINT(G_mw_log_level, MW_ERROR_LEVEL, "!!! [%s: %d]" format "\n", __FILE__, __LINE__, ##args)
#define MW_MSG(format, args...)			MW_PRINT(G_mw_log_level, MW_MSG_LEVEL, ">>> " format, ##args)
#define MW_INFO(format, args...)			MW_PRINT(G_mw_log_level, MW_INFO_LEVEL, "::: " format, ##args)
#define MW_DEBUG(format, args...)		MW_PRINT(G_mw_log_level, MW_DEBUG_LEVEL, "=== " format, ##args)



/*********************************
 *
 *	Predefined expression
 *
 *********************************/

#define		Q9_BASE			(512000000)

#ifndef DIV_ROUND
#define DIV_ROUND(divident, divider)    ( ( (divident)+((divider)>>1)) / (divider) )
#endif

#ifndef ROUND_UP
#define ROUND_UP(x, n)	( ((x)+(n)-1u) & ~((n)-1u) )
#endif

#ifndef MIN
#define MIN(a, b) ((a) > (b) ? (b) : (a))
#endif

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

#ifndef ABS
#define ABS(x) ({				\
		int __x = (x);			\
		(__x < 0) ? -__x : __x;		\
            })
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(array)   (sizeof(array) / sizeof(array[0]))
#endif


#endif // _MW_DEFINES_H_

