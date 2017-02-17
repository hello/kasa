/*
 * crypto_chip.h
 *
 * History:
 *	2015/06/19 - [Zhi He] Created file
 *
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
 */


#ifndef __CRYPTO_CHIP_H__
#define __CRYPTO_CHIP_H__

enum {
	EStatusCode_OK = 0x0,

	EStatusCode_BadParameters = 0x01,
	EStatusCode_CmdFail = 0x02,

	EStatusCode_NotPermitted = 0x10,
	EStatusCode_NoMemory = 0x11,
	EStatusCode_IOError = 0x12,
	EStatusCode_DeviceBusy = 0x13,
	EStatusCode_CommunicationFail = 0x14,

};

#define DLOG_PUT_STRING putstr
#define DLOG_PUT_BYTE putbyte
#define DLOG_PUT_DEC putdec

//#define DENABLE_LOG
#ifndef DENABLE_LOG
#define DLOG_DEBUG(format, args...) (void)0
#define DLOG_NOTICE(format, args...) (void)0
#define DLOG_ERROR(format, args...) (void)0
#else
#define DLOG_DEBUG(format, args...) 	do { \
		printf(format,##args); \
	} while (0)

#define DLOG_NOTICE(format, args...) do { \
		printf(format, ##args); \
		printf("\t\tat file %s, function %s, line %d:\n", __FILE__, __FUNCTION__, __LINE__); \
	} while (0)

#define DLOG_ERROR(format, args...) do { \
		printf("!!Error at file %s, function %s, line %d:\n", __FILE__, __FUNCTION__, __LINE__); \
		printf(format, ##args); \
	} while (0)
#endif


#endif

