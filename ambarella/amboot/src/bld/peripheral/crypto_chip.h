/*
 * crypto_chip.h
 *
 * History:
 *	2015/06/19 - [Zhi He] Created file
 *
 * Copyright (C) 2015 - 2025, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
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

