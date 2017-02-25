/*#############################################################################
 * Copyright 2014-2015 Sony Corporation.
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Sony Corporation.
 * No part of this file may be copied, modified, sold, and distributed in any
 * form or by any means without prior explicit permission in writing from
 * Sony Corporation.
 * @file sonysd_local.h
 */
/*###########################################################################*/

#define SIGNATURE "SDZ FOR SONY V02"
#define SECTORSZ 512


typedef struct _sonysd_local_info {
	unsigned long error_log_sector_cnt;
} sonysd_local_info;


#define PRINTF(...) sonysd_printf(__VA_ARGS__)
int sonysd_printf(const char* format, ...);


int sonysd_card_lock(int fd);
int sonysd_card_unlock(int fd);
int sonysd_readsector(int fd, unsigned char* buf, unsigned long addr);
int sonysd_vscmd(int fd);


