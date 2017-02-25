/*#############################################################################
 * Copyright 2014-2015 Sony Corporation.
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Sony Corporation.
 * No part of this file may be copied, modified, sold, and distributed in any
 * form or by any means without prior explicit permission in writing from
 * Sony Corporation.
 * @file sonysd_lib.c
 */
/*###########################################################################*/

#include <stdio.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include "sonysd.h"
#include "sonysd_local.h"


static const char* sonysd_devpath = "/dev/mmcblk0";


int sonysd_printf(const char* format, ...){
	va_list arg;
	va_start(arg, format);
	vfprintf(stderr, format, arg);
	va_end(arg);
	return 0;
}



#if 0 // Big Endien
__inline unsigned char GetUInt8(unsigned char *ptr) {
	return ptr[0];
}
__inline unsigned long GetUInt16(unsigned char *ptr) {
	return ((unsigned long) ptr[0] << 8) + ptr[1];
}
__inline unsigned long GetUInt32(unsigned char *ptr) {
	return ((unsigned long) ptr[0] << 24) + ((unsigned long) ptr[1] << 16)
		+ ((unsigned long) ptr[2] << 8) + ptr[3];
}
__inline unsigned long long GetUInt64(unsigned char *ptr) {
	return ((unsigned long long) ptr[0] << 56) + ((unsigned long long) ptr[1] << 48)
		+ ((unsigned long long) ptr[2] << 40) + ((unsigned long long) ptr[3] << 32)
		+ ((unsigned long long) ptr[4] << 24) + ((unsigned long long) ptr[5] << 16)
		+ ((unsigned long long) ptr[6] << 8) + ptr[7];
}
#else // Little Endien
__inline unsigned char GetUInt8(unsigned char *ptr) {
	return ptr[0];
}
__inline unsigned short GetUInt16(unsigned char *ptr) {
	return ((unsigned short) ptr[1] << 8) + ptr[0];
}
__inline unsigned long GetUInt32(unsigned char *ptr) {
	return ((unsigned long) ptr[3] << 24) + ((unsigned long) ptr[2] << 16)
		+ ((unsigned long) ptr[1] << 8) + ptr[0];
}
__inline unsigned long long GetUInt64(unsigned char *ptr) {
	return ((unsigned long long) ptr[7] << 56) + ((unsigned long long) ptr[6] << 48)
		+ ((unsigned long long) ptr[5] << 40) + ((unsigned long long) ptr[4] << 32)
		+ ((unsigned long long) ptr[3] << 24) + ((unsigned long long) ptr[2] << 16)
		+ ((unsigned long long) ptr[1] << 8) + ptr[0];
}
#endif


void sonysd_set_devpath(const char* path){
	sonysd_devpath = path;
}

void sonysd_debug_print_info(sonysd_info *info){
	PRINTF("life_information_num: %llu\n",       info->life_information_num);
	PRINTF("life_information_den: %llu\n",       info->life_information_den);
	PRINTF("data_size_per_unit: %lu\n",          info->data_size_per_unit);
	PRINTF("spare_block_rate: %lu\n",            info->spare_block_rate);
	PRINTF("num_of_sudden_power_failure: %lu\n", info->num_of_sudden_power_failure);
	PRINTF("operation_mode: %lu\n",              info->operation_mode);
}

void sonysd_debug_print_errlog(sonysd_errlog *log){
	int i;
	unsigned int j;

	PRINTF("total error count: %d\n", log->count);
	for(i=0; i<log->count; i++){
		PRINTF("---[%d]---------------------------------------\n", i+1);
		PRINTF("error_no: %u\n",                   log->items[i].error_no);
		PRINTF("error_type: 0x%02X\n",             log->items[i].error_type);
		PRINTF("block_type: 0x%02X\n",             log->items[i].block_type);
		PRINTF("logical_CE: 0x%02X\n",             log->items[i].logical_CE);
		PRINTF("physical_plane: 0x%02X\n",         log->items[i].physical_plane);
		PRINTF("physical_block: 0x%04X\n",         log->items[i].physical_block);
		PRINTF("physical_page:  0x%04X\n",         log->items[i].physical_page);
		PRINTF("error_type_of_timeout: 0x%04X\n",  log->items[i].error_type_of_timeout);
		PRINTF("erase_block_count: %lu\n",         log->items[i].erase_block_count);
		PRINTF("write_page_error_count: %u\n",     log->items[i].write_page_error_count);
		PRINTF("read_page_error_count: %u\n",      log->items[i].read_page_error_count);
		PRINTF("erase_block_error_count: %u\n",    log->items[i].erase_block_error_count);
		PRINTF("timeout_count: %u\n",              log->items[i].timeout_count);
		PRINTF("crc_error_count: %u\n",            log->items[i].crc_error_count);
		PRINTF("latest cmd: 0x%04X  sector_addr: 0x%08lX  resp: 0x%08lX\n",
					log->items[i].command[0].cmd,
					log->items[i].command[0].addr,
					log->items[i].command[0].resp
		);
		for(j=1; j<sizeof(log->items[0].command)/sizeof(log->items[0].command[0]); j++){
			PRINTF("history %d : 0x%04X  sector_addr: 0x%08lX  resp: 0x%08lX\n",
						j,
						log->items[i].command[j].cmd,
						log->items[i].command[j].addr,
						log->items[i].command[j].resp
			);
		}
	}
}


#define READ(fd, buf, addr) \
if(0 != sonysd_readsector(fd, buf, addr)){ \
	PRINTF("read error (%d).\n", errno);     \
	goto ReadError;                          \
}                                          \


static int sonysd_get_info_core(int fd, sonysd_info *info, sonysd_local_info *local){
	int ret = SONYSD_ERR_OTHER;
	unsigned char buf[SECTORSZ];

	READ(fd, buf, 19);
	READ(fd, buf, 4);
	READ(fd, buf, 26);
	READ(fd, buf, 2);

	if(strncmp(SIGNATURE, (char*)buf, 0x10) != 0){
		ret = SONYSD_ERR_UNSUPPORT;
		goto Error1;
	}

	if(info != NULL){
		memset(info, 0, sizeof(sonysd_info));
		info->life_information_num =        GetUInt64(buf + 0x10);
		info->life_information_den =        GetUInt64(buf + 0x18);
		info->data_size_per_unit =          GetUInt32(buf + 0x50);
		info->spare_block_rate =            GetUInt32(buf + 0x54);
		info->num_of_sudden_power_failure = GetUInt32(buf + 0x58);
		info->operation_mode =              GetUInt32(buf + 0x5C);
	}

	if(local != NULL){
		memset(local, 0, sizeof(sonysd_local_info));
		local->error_log_sector_cnt =       GetUInt32(buf + 0x48);
	}

	ret = SONYSD_SUCCESS;

Error1:
ReadError:
	return ret;
}


int sonysd_get_info(sonysd_info *info){
	int err, ret = SONYSD_ERR_OTHER;
	int fd = -1;

	if(0 > (fd = open(sonysd_devpath, O_RDONLY))){
		PRINTF("Can't open %s\n", sonysd_devpath);
		if(errno == ENOENT){
			ret = SONYSD_ERR_NOCARD;
		}
		goto Error1;
	}

	if(0 != sonysd_card_lock(fd)){
		goto Error2;
	}

	if(0 != (err = sonysd_get_info_core(fd, info, NULL))){
		ret = err;
		goto Error3;
	}

	ret = SONYSD_SUCCESS;

Error3:
	sonysd_card_unlock(fd);
Error2:
	close(fd);
Error1:
	return ret;
}


static void sonysd_read_err(sonysd_err* err, unsigned char* buf){
	int i;

	err->error_no =                     GetUInt16(buf);
	err->error_type =                   GetUInt8 (buf + 0x02);
	err->block_type =                   GetUInt8 (buf + 0x03);
	err->logical_CE =                   GetUInt8 (buf + 0x04);
	err->physical_plane =               GetUInt8 (buf + 0x05);
	err->physical_block =               GetUInt16(buf + 0x06);
	err->physical_page =                GetUInt16(buf + 0x08);
	err->error_type_of_timeout =        GetUInt16(buf + 0x0A);
	err->erase_block_count =            GetUInt32(buf + 0x0C);
	err->write_page_error_count =       GetUInt16(buf + 0x10);
	err->read_page_error_count =        GetUInt16(buf + 0x12);
	err->erase_block_error_count =      GetUInt16(buf + 0x14);
	err->timeout_count =                GetUInt16(buf + 0x16);
	err->crc_error_count =              GetUInt16(buf + 0x18);

	buf += 0x20;

	for(i=0; i<6; i++){
		err->command[i].cmd =             GetUInt16(buf);
		err->command[i].addr =            GetUInt32(buf + 0x02);
		err->command[i].resp =            GetUInt32(buf + 0x06);
		buf += 0x10;
	}
}


int sonysd_get_errlog(sonysd_errlog *log){
	int err, ret = SONYSD_ERR_OTHER;
	int fd = -1;
	sonysd_local_info local;
	unsigned int i, cnt = 0;
	unsigned char buf[SECTORSZ];

	memset(log, 0xFF, sizeof(sonysd_errlog));
	log->count = 0;

	if(0 > (fd = open(sonysd_devpath, O_RDONLY))){
		PRINTF("Can't open %s\n", sonysd_devpath);
		if(errno == ENOENT){
			ret = SONYSD_ERR_NOCARD;
		}
		goto Error1;
	}

	if(0 != sonysd_card_lock(fd)){
		goto Error2;
	}

	if(0 != (err = sonysd_get_info_core(fd, NULL, &local))){
		ret = err;
		goto Error3;
	}

	for(i=0; i<local.error_log_sector_cnt &&
			cnt + 3 < sizeof(log->items) / sizeof(sonysd_err); i++){
		READ(fd, buf, 2);

		sonysd_read_err(&log->items[cnt++], &buf[0]);
		sonysd_read_err(&log->items[cnt++], &buf[0x80]);
		sonysd_read_err(&log->items[cnt++], &buf[0x100]);
		sonysd_read_err(&log->items[cnt++], &buf[0x180]);
	}

	for(i=0; i<64; i++){
		if(log->items[i].error_type == 0xFF){
			break;
		}
		log->count++;
	}

	ret = SONYSD_SUCCESS;

ReadError:
Error3:
	sonysd_card_unlock(fd);
Error2:
	close(fd);
Error1:
	return ret;
}


int sonysd_set_operation_mode(unsigned long on_off){
	int err, ret = SONYSD_ERR_OTHER;
	int fd = -1;
	sonysd_info info;

	if(0 > (fd = open(sonysd_devpath, O_RDONLY))){
		PRINTF("Can't open %s.\n", sonysd_devpath);
		if(errno == ENOENT){
			ret = SONYSD_ERR_NOCARD;
		}
		goto Error1;
	}

	if(0 != sonysd_card_lock(fd)){
		goto Error2;
	}

	if(0 != (err = sonysd_get_info_core(fd, &info, NULL))){
		ret = err;
		goto Error3;
	}

	if(info.operation_mode != on_off){
		if(0 != sonysd_vscmd(fd)){
			PRINTF("vscmd error (%d).\n", errno);
			goto Error3;
		}

		if(0 != (err = sonysd_get_info_core(fd, &info, NULL))){
			ret = err;
			goto Error3;
		}
	
		if(info.operation_mode != on_off){
			PRINTF("Failed to set operation mode.\n");
			goto Error3;
		}
	}

	ret = SONYSD_SUCCESS;

Error3:
	sonysd_card_unlock(fd);
Error2:
	close(fd);
Error1:
	return ret;
}


