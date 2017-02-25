/**
 * boards/s2lm_kiwi/bsp/iav/smart3a.c
 *
 * History:
 *    2015/02/11 - [Tao Wu] created file
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


#include "rtc.h"
#include "adc.h"
#include "iav_fastboot.h"

//#define SMART_3A_DEBUG

#ifdef CONFIG_S2LMKIWI_ENABLE_ADVANCED_ISO_MODE
#if defined(CONFIG_ISO_TYPE_MIDDLE)
struct image_binary_info image_info[MAX_IMAGE_BINARY_NUM] =
{
	// name					//size	//src 	//dest	//cfg
	{"01_cc_reg.bin",			2304,	0,	0,	300},
	{"02_cc_3d_A.bin",			16384,	2304,	2304,	304},
	{"03_cc_3d_C.bin",			0,	18688,	18688,	0},
	{"04_cc_out.bin",			1024,	18688,	18688,	308},
	{"05_thresh_dark.bin",			768,	19712,	19712,	352},
	{"06_thresh_hot.bin",			768,	20480,	20480,	356},
	{"07_local_exposure_gain.bin",		512,	21248,	21248,	264},
	{"08_cmf_table.bin",			48,	21760,	21760,	360},
	{"09_chroma_scale_gain.bin",		256,	21808,	21808,	296},
	{"10_fir1_A.bin",			256,	22064,	22064,	412},
	{"11_fir2_A.bin",			256,	22320,	22320,	416},
	{"12_coring_A.bin",			256,	22576,	22576,	420},
	{"13_fir1_B.bin",			0,	22832,	22832,	0},
	{"14_fir2_B.bin",			0,	22832,	22832,	0},
	{"15_coring_B.bin",			0,	22832,	22832,	0},
	{"16_wide_chroma_mctf.bin",		0,	22832,	22832,	0},
	{"17_fir1_motion_detection.bin",	256,	22832,	22832,	620},
	{"18_fir1_motion_detection_map.bin",	256,	23088,	23088,	624},
	{"19_coring_motion_detection_map.bin",	256,	23344,	23344,	628},
	{"20_video_mctf.bin",			528,	23600,	23600,	616},
	{"21_md_fir1.bin",			256,	24128,	24128,	620},
	{"22_md_map_fir1.bin",			256,	24384,	24384,	624},
	{"23_md_map_coring.bin",		256,	24640,	24640,	628},
	{"24_cc_reg_combine.bin",		0,	24896,	24896,	792},
	{"25_cc_3d_combine.bin",		0,	24896,	24896,	796},
	{"26_thresh_dark_mo.bin",		0,	24896,	24896,	648},
	{"27_thresh_hot_mo.bin",		0,	24896,	24896,	652},
	{"28_cmf_table_mo.bin",			0,	24896,	24896,	656},
	{"29_fir1_mo.bin",			0,	24896,	24896,	708},
	{"30_fir2_mo.bin",			0,	24896,	24896,	712},
	{"31_coring_mo.bin",			0,	24896,	24896,	716},
};
#elif defined(CONFIG_ISO_TYPE_ADVANCED)
struct image_binary_info image_info[MAX_IMAGE_BINARY_NUM] =
{
	// name					//size	//src 	//dest	//cfg
	{"01_cc_reg.bin",			2304,	0,	0,	300},
	{"02_cc_3d_A.bin",			16384,	2304,	2304,	304},
	{"03_cc_3d_C.bin",			0,	18688,	18688,	0},
	{"04_cc_out.bin",			1024,	18688,	18688,	308},
	{"05_thresh_dark.bin",			768,	19712,	19712,	352},
	{"06_thresh_hot.bin",			768,	20480,	20480,	356},
	{"07_local_exposure_gain.bin",		512,	21248,	21248,	264},
	{"08_cmf_table.bin",			48,	21760,	21760,	360},
	{"09_chroma_scale_gain.bin",		256,	21808,	21808,	296},
	{"10_fir1_A.bin",			256,	22064,	22064,	412},
	{"11_fir2_A.bin",			256,	22320,	22320,	416},
	{"12_coring_A.bin",			256,	22576,	22576,	420},
	{"13_fir1_B.bin",			256,	22832,	22832,	496},
	{"14_fir2_B.bin",			256,	23088,	23088,	500},
	{"15_coring_B.bin",			256,	23344,	23344,	504},
	{"16_wide_chroma_mctf.bin",		528,	23600,	23600,	560},
	{"17_fir1_motion_detection.bin",	256,	24128,	24128,	620},
	{"18_fir1_motion_detection_map.bin",	256,	24384,	24384,	624},
	{"19_coring_motion_detection_map.bin",	256,	24640,	24640,	628},
	{"20_video_mctf.bin",			528,	24896,	24896,	616},
	{"21_md_fir1.bin",			256,	25424,	25424,	620},
	{"22_md_map_fir1.bin",			256,	25680,	25680,	624},
	{"23_md_map_coring.bin",		256,	25936,	25936,	628},
	{"24_cc_reg_combine.bin",		0,	26192,	26192,	792},
	{"25_cc_3d_combine.bin",		0,	26192,	26192,	796},
	{"26_thresh_dark_mo.bin",		0,	26192,	26192,	648},
	{"27_thresh_hot_mo.bin",		0,	26192,	26192,	652},
	{"28_cmf_table_mo.bin",			0,	26192,	26192,	656},
	{"29_fir1_mo.bin",			0,	26192,	26192,	708},
	{"30_fir2_mo.bin",			0,	26192,	26192,	712},
	{"31_coring_mo.bin",			0,	26192,	26192,	716},
};
#endif

int dsp_update_aaa_binary_addr(void)
{
	u32 params_base = (u32)DSP_FASTBOOT_IDSPCFG_START;
	u32 cfg_base = (u32)DSP_STILL_ISO_CONFIG_START;
	u32 *addr;
	int i;

	for (i = 0; i < MAX_IMAGE_BINARY_NUM; i++) {
		if (image_info[i].size != 0) {
			addr = (u32*)(cfg_base + image_info[i].cfg_offset);
			*addr = params_base + image_info[i].dest_offset;
		}
	}

	return 0;
}

static void load_3A_param_adv_iso(struct adcfw_header *hdr, int idsp_cfg_index)
{
	u32 addr = 0, size = 0;

	addr = (u32)hdr + hdr->smart3a[idsp_cfg_index].offset;
	size = SIZE_IDSP_CFG_ADV_ISO;
	memcpy((void *)DSP_FASTBOOT_IDSPCFG_START, (void *)addr, size);

	/* iso table */
	addr = (u32)hdr + hdr->smart3a[idsp_cfg_index].offset + size;
	size = SIZE_LISO_CFG_ADV_ISO;
	u32* color_update_flag = (u32 *)((u8*)(addr) + COLOR_UPDATE_FLAG_OFFSET);
	*color_update_flag = 0;
	memcpy((void *)DSP_STILL_ISO_CONFIG_START, (void *)addr, size);
	dsp_update_aaa_binary_addr();
}
#endif

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

int load_3A_param_liso(struct adcfw_header *hdr, int idsp_cfg_index)
{
	u32 addr = 0, size = 0;

	addr = (u32)hdr + hdr->smart3a[idsp_cfg_index].offset;
	size = hdr->smart3a_size;
	if (addr && size > IDSPCFG_BINARY_HEAD_SIZE) {
		memcpy((void *)(UCODE_DEFAULT_BINARY_START + DEFAULT_BINARY_IDSPCFG_OFFSET),
			(void *)(((u8*)addr) + IDSPCFG_BINARY_HEAD_SIZE), (size - IDSPCFG_BINARY_HEAD_SIZE));
	} else {
		putstr("Invalid idspcfg partition size\r\n");
	}

	return 0;
}

int load_3A_param(struct adcfw_header *hdr, int idsp_cfg_index)
{
#ifdef SMART_3A_DEBUG
	u32 time3_MS = 0, time4_MS = 0, delta3_MS = 0;
	time3_MS = rct_timer_get_count();
#endif

#ifdef CONFIG_S2LMKIWI_ENABLE_ADVANCED_ISO_MODE
	load_3A_param_adv_iso(hdr, idsp_cfg_index);
#else
	load_3A_param_liso(hdr, idsp_cfg_index);
#endif

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
