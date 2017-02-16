/**
 * boards/s2lm_ironman/bsp/iav/smart3a.c
 *
 * History:
 *    2015/02/11 - [Tao Wu] created file
 *
 * Copyright (C) 2012-2016, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include "rtc.h"
#include <adc.h>

//#define SMART_3A_DEBUG
#define DEFAULT_BINARY_IDSPCFG_OFFSET	(8208)
#define IDSPCFG_BINARY_HEAD_SIZE			(64)

enum idsp_cfg_select_policy {
	IDSP_CFG_SELECT_ONLY_ONE=0,
	IDSP_CFG_SELECT_VIA_UTC_HOUR,
	IDSP_CFG_SELECT_VIA_ENV_BRIGHTNESS,
};

static int select_idsp_cfg_via_rtc_hour()
{
	u32 val = 0;
	struct rtc_time* now = 0;

	val = readl(RTC_REG_ADDR);
	rtc_time_to_tm(val, now);

#ifdef SMART_3A_DEBUG
	putstr("RTC time is: 0x");
	puthex((now->tm_year + YEAR_BASE));
	putstr("-0x");
	puthex((now->tm_mon + MONTH_BASE));
	putstr("-0x");
	puthex(now->tm_mday);
	putstr("-0x");
	puthex(now->tm_hour);
	putstr("-0x");
	puthex(now->tm_min);
	putstr("-0x");
	puthex(now->tm_sec);
	putstr("\r\n");
#endif

	if (now->tm_hour < 0 || now->tm_hour >= 24) {
		putstr("RTC hour is invalid\r\n");
		return -1;
	} else {

#ifdef SMART_3A_DEBUG
		putstr("Select idspcfg section");
		putdec(now->tm_hour);
		putstr("\r\n");
#endif
		return now->tm_hour;
	}
}

static int select_idsp_cfg(enum idsp_cfg_select_policy policy)
{
	switch (policy) {
	case IDSP_CFG_SELECT_ONLY_ONE:
		return 0;
		break;

	case IDSP_CFG_SELECT_VIA_UTC_HOUR:
		return select_idsp_cfg_via_rtc_hour();
		break;

	case IDSP_CFG_SELECT_VIA_ENV_BRIGHTNESS:
		putstr("Policy BRIGHTNESS not supported yet\r\n");
		return -1;
		break;

	default:
		putstr("Invalid policy\r\n");
		return -1;
		break;
	}
}

int find_idsp_cfg(flpart_table_t *pptb, struct adcfw_header **p_hdr,
	int *p_idsp_cfg_index)
{
	int rval = -1;
	int idsp_cfg_index = -1;
	enum idsp_cfg_select_policy policy = IDSP_CFG_SELECT_ONLY_ONE;
	struct adcfw_header *hdr;

	if (!p_hdr || !p_idsp_cfg_index) {
		putstr("Find idspcfg failed, NULL input\r\n");
		return -1;
	}

#ifdef SMART_3A_DEBUG
	u32 time1_MS=0, time2_MS = 0, time3_MS = 0, delta1_MS = 0, delta2_MS = 0;
	time1_MS = rct_timer_get_count();
#endif

	rval = bld_loader_load_partition(PART_ADC, pptb, 0);
	if (rval < 0) {
		putstr("Load PART_ADC  failed\r\n");
		return -1;
	}

#ifdef SMART_3A_DEBUG
	time2_MS = rct_timer_get_count();
#endif

	hdr = (struct adcfw_header *)pptb->part[PART_ADC].mem_addr;
	if (hdr->magic != ADCFW_IMG_MAGIC
		|| hdr->fw_size != pptb->part[PART_ADC].img_len) {
		putstr("Invalid PART_ADC size\r\n");
		return -1;
	}
	idsp_cfg_index = select_idsp_cfg(policy);
	if (idsp_cfg_index < 0 || idsp_cfg_index >= hdr->smart3a_num) {
		putstr("Find idspcfg section failed via policy ");
		putdec(policy);
		putstr(", idspcfg index=");
		putdec(idsp_cfg_index);
		putstr(", should in range as [0, ");
		puthex(hdr->smart3a_num);
		putstr(")\r\n");
		return -1;
	}
	*p_hdr = hdr;
	*p_idsp_cfg_index = idsp_cfg_index;

#ifdef SMART_3A_DEBUG
	time3_MS = rct_timer_get_count();
	delta1_MS = time2_MS - time1_MS;
	delta2_MS = time3_MS - time2_MS;
	putstr("[LOAD3A]: <Load ADC partition to image> cost 0x");
	puthex(delta1_MS);
	putstr(" MS!\r\n");
	putstr("[LOAD3A]: <Find selected section of ADC image> cost 0x");
	puthex(delta2_MS);
	putstr(" MS!\r\n");
#endif

	return 0;
}

int load_3A_param(struct adcfw_header *hdr, int idsp_cfg_index)
{
	u32 addr=0, size=0;

#ifdef SMART_3A_DEBUG
	u32 time3_MS = 0, time4_MS = 0, delta3_MS = 0;
	time3_MS = rct_timer_get_count();
#endif

	addr = (u32)hdr + hdr->smart3a[idsp_cfg_index].offset;
	size = hdr->smart3a_size;
	if (addr && size > IDSPCFG_BINARY_HEAD_SIZE) {
		memcpy((void *)(UCODE_DEFAULT_BINARY_START + DEFAULT_BINARY_IDSPCFG_OFFSET),
			(void *)(((u8*)addr) + IDSPCFG_BINARY_HEAD_SIZE), (size - IDSPCFG_BINARY_HEAD_SIZE));
	} else {
		putstr("Invalid idspcfg partition size\r\n");
	}

#ifdef SMART_3A_DEBUG
	time4_MS = rct_timer_get_count();
	delta3_MS = time4_MS - time3_MS;
	putstr("[LOAD3A]: <Copy 3A params to DSP config mem> cost 0x");
	puthex(delta3_MS);
	putstr(" MS!\r\n");
#endif

	return 0;
}

int load_shutter_gain_param(struct adcfw_header *hdr, int idsp_cfg_index)
{

#ifdef SMART_3A_DEBUG
	u32 time3_MS = 0, time4_MS = 0, delta3_MS = 0;
	time3_MS = rct_timer_get_count();
#endif

	idc_bld_write_16_8(IDC_MASTER1, 0x6c, 0x3500, hdr->smart3a[idsp_cfg_index].para0);
	idc_bld_write_16_8(IDC_MASTER1, 0x6c, 0x3501, hdr->smart3a[idsp_cfg_index].para1);
	idc_bld_write_16_8(IDC_MASTER1, 0x6c, 0x3502, hdr->smart3a[idsp_cfg_index].para2);
	idc_bld_write_16_8(IDC_MASTER1, 0x6c, 0x3508, hdr->smart3a[idsp_cfg_index].para3);
	idc_bld_write_16_8(IDC_MASTER1, 0x6c, 0x3509, hdr->smart3a[idsp_cfg_index].para4);

#ifdef SMART_3A_DEBUG
	time4_MS = rct_timer_get_count();
	delta3_MS = time4_MS - time3_MS;
	putstr("[LOAD3A]: <r_gain=");
	putdec(hdr->smart3a[idsp_cfg_index].r_gain);

	putstr(", b_gain=");
	putdec(hdr->smart3a[idsp_cfg_index].b_gain);

	putstr(", d_gain=");
	putdec(hdr->smart3a[idsp_cfg_index].d_gain);

	putstr(", shutter=");
	putdec(hdr->smart3a[idsp_cfg_index].shutter);

	putstr(", agc=");
	putdec(hdr->smart3a[idsp_cfg_index].agc);

	putstr(", shutter3500=0x");
	puthex(hdr->smart3a[idsp_cfg_index].para0);

	putstr(", shutter3501=0x");
	puthex(hdr->smart3a[idsp_cfg_index].para1);

	putstr(", shutter3502=0x");
	puthex(hdr->smart3a[idsp_cfg_index].para2);

	putstr(", gain3508=0x");
	puthex(hdr->smart3a[idsp_cfg_index].para3);

	putstr(", gain3509=0x");
	puthex(hdr->smart3a[idsp_cfg_index].para4);
	putstr(">");
	putstr(" cost 0x");
	puthex(delta3_MS);
	putstr(" MS!\r\n");
#endif

	return 0;
}
