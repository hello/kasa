/*******************************************************************************
 * iav_enc_utils.c
 *
 * History:
 *  Mar 4, 2014 - [qianshen] created file
 *
 * Copyright (C) 2012-2016, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/
#include <config.h>
#include <linux/random.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <linux/delay.h>   // Fixme: msleep
#include <iav_utils.h>
#include <iav_ioctl.h>
#include <dsp_api.h>
#include <dsp_format.h>
#include <vin_api.h>
#include <vout_api.h>
#include "iav.h"
#include "iav_vin.h"
#include "iav_vout.h"
#include "iav_enc_api.h"
#include "iav_enc_utils.h"


inline int is_enc_work_state(struct ambarella_iav * iav)
{
	return ((iav->state == IAV_STATE_PREVIEW) ||
		(iav->state == IAV_STATE_ENCODING));
}

inline int is_warp_mode(struct ambarella_iav *iav)
{
	return ((iav->encode_mode == DSP_MULTI_REGION_WARP_MODE) ||
		(iav->encode_mode == DSP_SINGLE_REGION_WARP_MODE));
}

inline int is_stitched_vin(struct ambarella_iav *iav, u16 enc_mode)
{
	struct iav_rect *vin = NULL;
	int stitched = 0;
	u32 i;

	for (i = 0; i < IAV_MAX_ENCODE_STREAMS_NUM; ++i) {
		if (iav->stream[i].max_GOP_M > 1) {
			stitched = 1;
			break;
		}
	}
	if (!stitched) {
		get_vin_win(iav, &vin, 1);
		stitched = ((vin->width >= MAX_WIDTH_IN_FULL_FPS) &&
			(enc_mode != DSP_SINGLE_REGION_WARP_MODE));
	}

	return stitched;
}

inline int get_hdr_type(struct ambarella_iav *iav)
{
	return ((iav->system_config[iav->encode_mode].expo_num < MAX_HDR_2X) ?
		HDR_TYPE_OFF : G_encode_limit[iav->encode_mode].hdr_2x_supported);
}

inline u32 get_pm_unit(struct ambarella_iav *iav)
{
	return iav->system_config[iav->encode_mode].mctf_pm ? IAV_PM_UNIT_MB :
		IAV_PM_UNIT_PIXEL;
}

inline u32 get_pm_domain(struct ambarella_iav *iav)
{
	u32 domain, unit;

	if (iav->encode_mode == DSP_MULTI_REGION_WARP_MODE) {
		domain = IAV_PM_DOMAIN_VIN;
	} else {
		unit = get_pm_unit(iav);
		if (unit == IAV_PM_UNIT_MB) {
			domain = IAV_PM_DOMAIN_MAIN;
		} else {
			domain = IAV_PM_DOMAIN_VIN;
		}
	}

	return domain;
}

inline int get_pm_vin_win(struct ambarella_iav *iav, struct iav_rect **vin)
{
	if (iav->system_config[iav->encode_mode].expo_num >= MAX_HDR_2X) {
		if (!iav->vinc[0]->vin_format.max_act_win.width ||
			!iav->vinc[0]->vin_format.max_act_win.height) {
			iav_error("Sensor should export max_act_win in HDR mode!\n");
			return -1;
		}
		*vin = (struct iav_rect *)&iav->vinc[0]->vin_format.max_act_win;
		return 0;
	} else {
		return get_vin_win(iav, vin, 0);
	}
}

int get_iso_type(struct ambarella_iav *iav, u32 debug_enable_map)
{
	u32 chip_id;
	int iso_type;
	u32 enc_mode = iav->encode_mode;

	if (debug_enable_map & DEBUG_TYPE_ISO_TYPE) {
		iso_type = iav->system_config[enc_mode].debug_iso_type;
	} else {
		if (enc_mode == DSP_ADVANCED_ISO_MODE) {
			iav->dsp->get_chip_id(iav->dsp, &chip_id, NULL);
			switch (chip_id) {
			case IAV_CHIP_ID_S2L_22M:
			case IAV_CHIP_ID_S2L_33M:
			case IAV_CHIP_ID_S2L_55M:
			case IAV_CHIP_ID_S2L_99M:
			case IAV_CHIP_ID_S2L_TEST:
				/* Run Middle ISO for S2LM */
				iso_type = ISO_TYPE_MIDDLE;
				break;
			case IAV_CHIP_ID_S2L_63:
			case IAV_CHIP_ID_S2L_66:
			case IAV_CHIP_ID_S2L_88:
			case IAV_CHIP_ID_S2L_99:
				/* Run Advanced ISO for S2L */
				iso_type = ISO_TYPE_ADVANCED;
				break;
			default:
				iav_error("Invalid chip ID [%d].\n", chip_id);
				iso_type = ISO_TYPE_MIDDLE;
				break;
			}
		} else {
			iso_type = G_encode_limit[enc_mode].iso_supported;
		}
	}

	return iso_type;
}

u32 get_chip_id(struct ambarella_iav *iav)
{
	u32 chip_id;
	struct iav_system_config *sys_cfg = &iav->system_config[iav->encode_mode];

	if (sys_cfg->debug_enable_map & DEBUG_TYPE_CHIP_ID) {
		chip_id = sys_cfg->debug_chip_id;
	} else {
		iav->dsp->get_chip_id(iav->dsp, &chip_id, NULL);
	}

	return chip_id;
}

int get_gcd(int a, int b)
{
	if ((a == 0) || (b == 0)) {
		iav_debug("wrong input for gcd \n");
		return 1;
	}
	while ((a != 0) && (b != 0)) {
		if (a > b) {
			a = a%b;
		} else {
			b = b%a;
		}
	}
	return (a == 0) ? b : a;
}

inline u64 get_monotonic_pts(void)
{
	struct timeval tv;
	do_gettimeofday(&tv);
	return ((u64)tv.tv_sec * 1000000L + tv.tv_usec);
}

inline int get_vin_win(struct ambarella_iav *iav, struct iav_rect **vin, u8 vin_win_flag)
{
	struct iav_vin_format * vin_format = &iav->vinc[0]->vin_format;

	if ((iav->system_config[iav->encode_mode].expo_num >= MAX_HDR_2X) &&
		vin_format->act_win.width && vin_format->act_win.height &&
		!vin_win_flag) {
		*vin = &vin_format->act_win;
	} else {
		*vin = &vin_format->vin_win;
	}

	return 0;
}

inline int get_vout_win(u32 buf_id, struct iav_window *vout)
{
	struct amba_video_info *video_info;

	if (buf_id == IAV_SRCBUF_PA) {
		video_info = &G_voutinfo[0].video_info;
	} else if (buf_id == IAV_SRCBUF_PB){
		video_info = &G_voutinfo[1].video_info;
	} else {
		return -1;
	}
	vout->width = video_info->width;
	vout->height = video_info->height;

	return 0;
}

/* Need to add "mutex_lock / mutex_unlock" in the upper layer before
 * calling this function. */
inline int wait_vcap_count(struct ambarella_iav *iav, u32 count)
{
	struct dsp_device *dsp = iav->dsp;
	int rval;

	mutex_unlock(&iav->iav_mutex);
	rval = dsp->wait_vcap(dsp, count);
	mutex_lock(&iav->iav_mutex);

	return rval;
}

#ifndef CONFIG_PLAT_AMBARELLA_SUPPORT_GDMA
int dma_pitch_memcpy(struct iav_gdma_param *param)
{
	int i;
	u8 * src = (u8*)param->src_virt_addr;
	u8 * dst = (u8*)param->dest_virt_addr;
	for (i = 0; i < param->height; ++i) {
		memcpy(dst, src, param->width);
		src += param->src_pitch;
		dst += param->dest_pitch;
	}
	return 0;
}
#endif

int iav_mem_copy(struct ambarella_iav *iav, u32 buff_id,
	u32 src_offset, u32 dst_offset, u32 size, u32 pitch)
{
	struct iav_gdma_param param;
	u32 src_virt, src_phys, dst_virt, dst_phys;
	int rval = 0;

	src_virt = iav->mmap[buff_id].virt + src_offset;
	src_phys = iav->mmap[buff_id].phys + src_offset;

	dst_virt = iav->mmap[buff_id].virt + dst_offset;
	dst_phys = iav->mmap[buff_id].phys + dst_offset;

	// Use GDMA copy
	memset(&param, 0, sizeof(param));
	param.dest_phys_addr = (u32)dst_phys;
	param.dest_virt_addr = (u32)dst_virt;
	param.src_phys_addr = (u32)src_phys;
	param.src_virt_addr = (u32)src_virt;
	param.src_pitch = pitch;
	param.dest_pitch = pitch;
	param.width = pitch;
	param.height = size / pitch;

	/* Fix me, the mmap type is non cached in iav_create_mmap_table */
	param.src_non_cached = 1;
	param.dest_non_cached = 1;

	if (dma_pitch_memcpy(&param) < 0) {
		iav_error("dma_pitch_memcpy error\n");
		rval = -EFAULT;
	}
	// End of GDMA copy

	return rval;
}

