/**
 * boards/s2lm_kiwi/bsp/iav/iav_fastosd.c
 *
 * History:
 *    2014/12/03 - [Tao Wu] created file
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

static char osd_dram[] = "2014/12/08-12:01:01";
static int osd_string_index = 0;

static void int_to_u8(int in_num, char offset, u8 end)
{
	char tmp[8] = {'0', '0'};
	char *str = osd_dram;
	int n = 0;
	int num = in_num;

	while (num) {
		tmp[n++] = num % 10 + offset;
		num = num / 10;
	}

	/* Fill 00 when num is equal 0 or samller than 10 */
	if (in_num == 0 || in_num < 10) {
		n = 2;
	}
	n--;

	while (n >= 0) {
		str[osd_string_index++] = tmp[n--];
	}
	if (end >= 0) {
		str[osd_string_index++] = end;
	}
}

static void tm_to_osd(struct rtc_time *tm_value)
{
	/* 10 is equal : in index map */
	osd_string_index = 0;
	int_to_u8((tm_value->tm_year + YEAR_BASE), '0', '/'); //Year start from 1970
	int_to_u8((tm_value->tm_mon + MONTH_BASE), '0', '/'); // Month start from 0 to 11
	int_to_u8(tm_value->tm_mday, '0', '-');
	int_to_u8(tm_value->tm_hour, '0', ':');
	int_to_u8(tm_value->tm_min, '0', ':');
	int_to_u8(tm_value->tm_sec, '0', -1);
}

/* ==========================================================================*/

static ipcam_fast_osd_insert_t ipcam_fast_osd_insert = {
	.cmd_code = IPCAM_FAST_OSD_INSERT,
#if defined(CONFIG_S2LMKIWI_DSP_VOUT_TD043)
	.vout_id = 1,
#else
	.vout_id = 0,
#endif
	.fast_osd_enable = 1,
	.string_num_region = 1,
	.osd_num_region = 0,
	.font_index_addr = DSP_OVERLAY_FONT_INDEX_START,
	.font_map_addr = DSP_OVERLAY_FONT_MAP_START,
	.font_map_pitch = 512,
	.font_map_h = 32,
	.string_addr[0] = DSP_OVERLAY_STRING_START,
	.string_addr[1] = 0x00000000,
	.string_len[0] = (sizeof(osd_dram) - 1),
	.string_len[1] = 0,
	.string_output_addr[0] = DSP_OVERLAY_STRING_OUT_START,
	.string_output_addr[1] = 0x00000000,
	.string_output_pitch[0] = 1024,
	.string_output_pitch[1] = 0,
	.clut_addr[0] = DSP_OVERLAY_CLUT_START,
	.clut_addr[1] = 0x00000000,
	.string_win_offset_x[0] = 16,
	.string_win_offset_x[1] = 0,
	.string_win_offset_y[0] = 16,
	.string_win_offset_y[1] = 0,
};

/* ==========================================================================*/

static int load_fastosd_resources(flpart_table_t *pptb) {
	u32 addr = 0;
	u32 size = 0;
	int rval = 0;

	struct dspfw_header *hdr = (struct dspfw_header *)pptb->part[PART_DSP].mem_addr;
	if ( !hdr) {
		return -1;
	}
	rval = dsp_get_ucode_by_name(hdr, "font_index.bin", &addr, &size);
	if (rval >= 0) {
		memcpy((void *)DSP_OVERLAY_FONT_INDEX_START, (void *)addr, size);
	}
	rval = dsp_get_ucode_by_name(hdr, "font_map.bin", &addr, &size);
	if (rval >= 0) {
		memcpy((void *)DSP_OVERLAY_FONT_MAP_START, (void *)addr, size);
	}
	rval = dsp_get_ucode_by_name(hdr, "clut.bin", &addr, &size);
	if (rval >= 0) {
		memcpy((void *)DSP_OVERLAY_CLUT_START, (void *)addr, size);
	}

	return rval;
}

static int iav_fastosd(flpart_table_t *pptb)
{
	u32	val = 0;
	struct rtc_time now;

	val = load_fastosd_resources(pptb);

	if (val < 0) {
		return -1;
	}

	val = readl(RTC_REG_ADDR);
	rtc_time_to_tm(val, &now);
	tm_to_osd(&now);

	memcpy((void *)(DSP_OVERLAY_STRING_START), (void *)osd_dram, sizeof(osd_dram));

	/* fast osd setup */
	add_dsp_cmd(&ipcam_fast_osd_insert, sizeof(ipcam_fast_osd_insert));

	putstr_debug("iav_fastosd");
	return 0;
}

