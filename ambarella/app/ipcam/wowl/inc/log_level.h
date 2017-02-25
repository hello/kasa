/*******************************************************************************
 * log_level.h
 *
 * History:
 *    2015/8/8 - [Tao Wu] Create
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

#ifndef __LOG_LEVEL_H_
#define __LOG_LEVEL_H_

typedef enum {
	LOG_ERROR = 0,
	LOG_WARN,
	LOG_INFO,
	LOG_DEBUG,
	LOG_VERBOSE,

	LOG_LEVEL_NUM,
	LOG_LEVEL_FIRST = LOG_ERROR,
	LOG_LEVEL_LAST =  LOG_VERBOSE,
} log_level_t;

static log_level_t G_current_level = LOG_INFO;

#define log_print(_level_, _str_, _arg_...) do {\
	if (_level_ <= G_current_level ) {\
		printf(_str_, ##_arg_);	\
	}							\
}while(0)

#define loge(_str_, _arg_...) do {		\
	log_print(LOG_ERROR, _str_, ##_arg_);\
}while(0)

#define logw(_str_, _arg_...) do {		\
	log_print(LOG_WARN, _str_, ##_arg_);\
}while(0)

#define logi(_str_, _arg_...) do {		\
	log_print(LOG_INFO, _str_, ##_arg_); \
}while(0)

#define logd(_str_, _arg_...) do {		\
	log_print(LOG_DEBUG, _str_, ##_arg_);\
}while(0)

#define logv(_str_, _arg_...) do {		\
	log_print(LOG_VERBOSE, _str_, ##_arg_);\
}while(0)

void set_log_level(int level)
{
	G_current_level = level;
	printf("Log Level Set: %d\n", G_current_level);
}

int get_log_level(void)
{
	printf("Log Level Get: %d\n", G_current_level);
	return G_current_level;
}

#endif
