/*
 * imgproc_drv.c
 *
 * History:
 *	2012/10/10 - [Cao Rongrong] created file
 *	2013/12/12 - [Jian Tang] modified file
 *
 * Copyright (C) 2012-2016, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#include <config.h>
#include <linux/kernel.h>

#include <linux/wait.h>
#include <linux/completion.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <linux/mm.h>
#include <plat/ambcache.h>

#include <mach/hardware.h>
#include <mach/init.h>
#include <asm/uaccess.h>
#include <linux/delay.h>

#include "basetypes.h"
#include "dsp_cmd_msg.h"
#include "amba_imgproc.h"
#include "ambas_imgproc_arch.h"
#include "ambas_imgproc_ioctl_arch.h"

#include "dsp_api.h"
#include "iav_common.h"
#include "iav_ioctl.h"
#include "iav_utils.h"
#include "imgproc_drv.h"
#include "imgproc_pri.h"


static struct imgproc_private_info G_img;

static DSP_CMD G_img_cmds[IMG_CMD_NUM];

static DECLARE_COMPLETION(g_raw2yuv_comp);

static inline int is_in_3a_work_state(struct iav_imgproc_info * context)
{
	if ((context->iav_state == IAV_STATE_PREVIEW) ||
		(context->iav_state == IAV_STATE_ENCODING)) {
		return 1;
	} else {
		iav_error("Incorrect IAV state [%d].\n", context->iav_state);
		return 0;
	}
}

static void img_exit(void)
{
	if (G_img.wr_virt) {
		kfree(G_img.wr_virt);
	}

	if (G_img.rd_virt) {
		dma_free_writecombine(NULL, IMS_READ_TOTAL_SIZE,
			G_img.rd_virt, G_img.rd_phys);
	}
	memset(&G_img, 0, sizeof(G_img));
}

static int img_init(void)
{
	u8 i, *base = NULL;

	memset(&G_img, 0, sizeof(G_img));

	/* 3A statistics data is for read only. Using DMA write combine to map
	 * as non-cached & write buffered memory. No need to call
	 * "invalidate_d_cache" function. */
	base = (u8 *)dma_alloc_writecombine(NULL, IMS_READ_TOTAL_SIZE,
		&G_img.rd_phys, GFP_KERNEL);
	if (!base) {
		iav_error("Failed to malloc memory [%d] KB for IMGPROC RD.\n",
			(IMS_READ_TOTAL_SIZE >> 10));
		goto IMG_INIT_EXIT;
	}
	G_img.rd_virt = base;

	/* 3A configuration data is for write only. Using kzalloc to allocate
	 * cacheable memory. Need to call "clean_d_cache" before issue DSP cmd. */
	base = kzalloc(IMS_WRITE_TOTAL_SIZE, GFP_KERNEL);
	if (!base) {
		iav_error("Failed to malloc memory [%d] KB for IMGPROC.\n",
			(IMS_WRITE_TOTAL_SIZE >> 10));
		return -ENOMEM;
	}
	G_img.wr_virt = base;
	img_printk("IMG KERN VIRT: WR [0x%x, %d KB], RD [0x%x, %d KB].\n",
		(u32)G_img.rd_virt, (IMS_READ_TOTAL_SIZE >> 10),
		(u32)G_img.wr_virt, (IMS_WRITE_TOTAL_SIZE >> 10));

	G_img.addr[IMB_RGB_FIFO_BASE] = G_img.rd_virt + IMO_RGB_FIFO_BASE;
	G_img.addr[IMB_CFA_FIFO_BASE] = G_img.rd_virt + IMO_CFA_FIFO_BASE;
	G_img.addr[IMB_HDR_BLEND_FIFO_BASE] = G_img.rd_virt + IMO_HDR_BLEND_FIFO_BASE;
	G_img.addr[IMB_HDR_SHORT_FIFO_BASE] = G_img.rd_virt + IMO_HDR_SHORT_FIFO_BASE;

	G_img.addr[IMB_INPUT_LUT] = G_img.wr_virt + IMO_INPUT_LUT;
	G_img.addr[IMB_MATRIX_DRAM] = G_img.wr_virt + IMO_MATRIX_DRAM;
	G_img.addr[IMB_OUTPUT_LUT] = G_img.wr_virt + IMO_OUTPUT_LUT;
	G_img.addr[IMB_CHROMA_GAIN_CURVE] = G_img.wr_virt + IMO_CHROMA_GAIN_CURVE;
	G_img.addr[IMB_HOT_PIXEL_THD_TABLE] = G_img.wr_virt + IMO_HOT_PIXEL_THD_TABLE;
	G_img.addr[IMB_DARK_PIXEL_THD_TABLE] = G_img.wr_virt + IMO_DARK_PIXEL_THD_TABLE;
	G_img.addr[IMB_MCTF_CFG] = G_img.wr_virt + IMO_MCTF_CFG;
	G_img.addr[IMB_K0123_TABLE] = G_img.wr_virt + IMO_K0123_TABLE;
	G_img.addr[IMB_EXPOSURE_GAIN_CURVE] = G_img.wr_virt + IMO_EXPOSURE_GAIN_CURVE;
	G_img.addr[IMB_LUMA_SHARPEN_ALPHA_TABLE] = G_img.wr_virt + IMO_LUMA_SHARPEN_ALPHA_TABLE;
	G_img.addr[IMB_COEFF_FIR_0] = G_img.wr_virt + IMO_COEFF_FIR_0;
	G_img.addr[IMB_COEFF_FIR_1] = G_img.wr_virt + IMO_COEFF_FIR_1;
	G_img.addr[IMB_CORING_TABLE] = G_img.wr_virt + IMO_CORING_TABLE;
	G_img.addr[IMB_VIGNETTE_R_GAIN] = G_img.wr_virt + IMO_VIGNETTE_R_GAIN;
	G_img.addr[IMB_VIGNETTE_GO_GAIN] = G_img.wr_virt + IMO_VIGNETTE_GO_GAIN;
	G_img.addr[IMB_VIGNETTE_GE_GAIN] = G_img.wr_virt + IMO_VIGNETTE_GE_GAIN;
	G_img.addr[IMB_VIGNETTE_B_GAIN] = G_img.wr_virt + IMO_VIGNETTE_B_GAIN;
	G_img.addr[IMB_PIXEL_MAP] = G_img.wr_virt + IMO_PIXEL_MAP;
	G_img.addr[IMB_FPN_REG] = G_img.wr_virt + IMO_FPN_REG;
	G_img.addr[IMB_LNL_TONE_CURVE] = G_img.wr_virt + IMO_LNL_TONE_CURVE;
	G_img.addr[IMB_FLOAT_TILE_CONFIG] = G_img.wr_virt + IMO_FLOAT_TILE_CONFIG;
	G_img.addr[IMB_LUMA_3D_TABLE] = G_img.wr_virt + IMO_LUMA_3D_TABLE;

	G_img.cfa_next = 0;
	G_img.rgb_next = 0;
	G_img.hdr_next = 0;
	for (i = 0; i < MAX_SLICE_FOR_FRAME; ++i) {
		G_img.rgb_entry[i] = INVALID_AAA_DATA_ENTRY;
		G_img.cfa_entry[i] = INVALID_AAA_DATA_ENTRY;
	}
	for (i = 0; i < MAX_HDR_SLICE_FOR_FRAME; ++i) {
		G_img.hdr_entry[i] = INVALID_AAA_DATA_ENTRY;
	}

	G_img.dsp_enc_mode = ENC_UNKNOWN_MODE;
	spin_lock_init(&G_img.lock);
	init_waitqueue_head(&G_img.statis_wq);
	init_waitqueue_head(&G_img.dsp_wq);

#ifdef CONFIG_PM
	G_img.save_cmd = 1;
#else
	G_img.save_cmd = 0;
#endif

	return 0;

IMG_INIT_EXIT:
	img_exit();
	return -1;
}


#if 0
static u32 img_virt_to_dsp(struct iav_imgproc_info * context, void * addr)
{
	return PHYS_TO_DSP((u32)addr - context->img_virt + context->img_phys);
}
#endif

static int irq_save_rgb_cfa_ptr(void *rgb_fifo_ptr, void *cfa_fifo_ptr)
{
	u8 i, slice, ready, entry, next;
	struct rgb_aaa_stat * rgb = NULL;
	struct cfa_aaa_stat * cfa = NULL;

	if (!rgb_fifo_ptr) {
		G_img.rgb_entry[0] = INVALID_AAA_DATA_ENTRY;
		G_img.rgb_next = 0;
	} else {
		rgb = (struct rgb_aaa_stat *)(DSP_TO_PHYS(rgb_fifo_ptr) -
			G_img.rd_phys + G_img.rd_virt);
		slice = rgb->aaa_tile_info.total_slices_x;
		entry = ((u8*)rgb - G_img.addr[IMB_RGB_FIFO_BASE]) / RGB_AAA_DATA_BLOCK;
		next = G_img.rgb_next;
		if (entry < next) {
			entry += MAX_AAA_DATA_NUM;
		}
		ready = 0;
		if (next + (slice -1) <= entry) {
			ready = 1;
			while (next + (2 * slice - 1) <= entry) {
				next += slice;
			}
			if (next >= MAX_AAA_DATA_NUM) {
				next -= MAX_AAA_DATA_NUM;
			}
		}
		if (ready) {
			for (i = 0; i < slice; ++i) {
				G_img.rgb_entry[i] = next;
				if (++next == MAX_AAA_DATA_NUM) {
					next = 0;
				}
			}
		} else {
			G_img.rgb_entry[0] = INVALID_AAA_DATA_ENTRY;
		}
		G_img.rgb_next = next;

#if 0
		iav_debug("++ RGB [0x%x -> 0x%x] slice %d. Base [0x%x], curr [%d], next [%d].\n",
			(u32)rgb_fifo_ptr, (u32)rgb, slice, (u32)G_img.addr[IMB_RGB_FIFO_BASE],
			G_img.rgb_entry[0], G_img.rgb_next);
#endif
	}

	if (!cfa_fifo_ptr) {
		G_img.cfa_entry[0] = INVALID_AAA_DATA_ENTRY;
		G_img.cfa_next = 0;
	} else {
		cfa = (struct cfa_aaa_stat *)(DSP_TO_PHYS(cfa_fifo_ptr) -
			G_img.rd_phys + G_img.rd_virt);
		slice = cfa->aaa_tile_info.total_slices_x;
		entry = ((u8*)cfa - G_img.addr[IMB_CFA_FIFO_BASE]) / CFA_AAA_DATA_BLOCK;
		next = G_img.cfa_next;
		if (entry < next) {
			entry += MAX_AAA_DATA_NUM;
		}
		ready = 0;
		if (next + (slice - 1) <= entry) {
			ready = 1;
			while (next + (2 * slice - 1) <= entry) {
				next += slice;
			}
			if (next >= MAX_AAA_DATA_NUM) {
				next -= MAX_AAA_DATA_NUM;
			}
		}
		if (ready) {
			for (i = 0; i < slice; ++i) {
				G_img.cfa_entry[i] = next;
				if (++next == MAX_AAA_DATA_NUM) {
					next = 0;
				}
			}
		} else {
			G_img.cfa_entry[0] = INVALID_AAA_DATA_ENTRY;
		}
		G_img.cfa_next = next;

#if 0
		iav_debug("++ CFA [0x%x -> 0x%x] slice %d. Base [0x%x], curr [%d], next [%d].\n",
			(u32)cfa_fifo_ptr, (u32)cfa, slice, (u32)G_img.addr[IMB_CFA_FIFO_BASE],
			G_img.cfa_entry[0], G_img.cfa_next);
#endif
	}

	return 0;
}

static int irq_save_hdr_ptr(void *hdr_blend_fifo, void *hdr_short_fifo)
{
	u8 i, slice, ready, entry, next;
	struct cfa_pre_hdr_stat * hdr = NULL;

	if (!hdr_blend_fifo || !hdr_short_fifo) {
		G_img.hdr_entry[0] = INVALID_AAA_DATA_ENTRY;
		G_img.hdr_next = 0;
		return -1;
	}

	hdr = (struct cfa_pre_hdr_stat *)(DSP_TO_PHYS(hdr_blend_fifo) -
		G_img.rd_phys + G_img.rd_virt);
	slice = hdr->cfg_info.total_slice_in_x * (hdr->cfg_info.total_exposures - 1);
	entry = ((u8*)hdr - G_img.addr[IMB_HDR_BLEND_FIFO_BASE]) / CFA_PRE_HDR_BLOCK;
	next = G_img.hdr_next;
	if (entry < next) {
		entry += MAX_HDR_DATA_NUM;
	}
	ready = 0;
	if (next + (slice - 1) <= entry) {
		ready = 1;
		while (next + (2 * slice - 1) <= entry) {
			next += slice;
		}
		if (next >= MAX_HDR_DATA_NUM) {
			next -= MAX_HDR_DATA_NUM;
		}
	}
	if (ready) {
		for (i = 0; i < slice; ++i) {
			G_img.hdr_entry[i] = next;
			if (++next == MAX_HDR_DATA_NUM) {
				next = 0;
			}
		}
	} else {
		G_img.hdr_entry[0] = INVALID_AAA_DATA_ENTRY;
	}
	G_img.hdr_next = next;

#if 0
	iav_debug("HDR 0 Base [0x%x], curr [%d], [%d, %d, %d, %d, %d].\n",
		(u32)G_img.addr[IMB_HDR_BLEND_FIFO_BASE], G_img.hdr_entry[0],
		hdr->cfg_info.vin_stats_type, hdr->cfg_info.total_exposures,
		hdr->cfg_info.blend_index, hdr->cfg_info.total_slice_in_x,
		hdr->cfg_info.slice_index_x);
#endif

	return 0;
}

static int irq_save_hist_ptr(u32 hist_phys_addr, u16 pitch_size)
{
	if(hist_phys_addr == 0 || pitch_size == 0){
		G_img.raw_hist_addr = 0;
		G_img.raw_pitch = 0;
		return -1;
	}

	G_img.raw_hist_addr = hist_phys_addr;
	G_img.raw_pitch = pitch_size;

	return 0;
}

static int img_get_statistics(struct iav_imgproc_info *context, struct img_statistics  __user *arg)
{
	u8 *dst = NULL;
	u8 i, slice_x, slice, total;
	u64 cnt;
	struct img_statistics mw_cmd;
	struct cfa_aaa_stat *cfa = NULL;
	struct rgb_aaa_stat *rgb = NULL;
	struct cfa_pre_hdr_stat *hdr_main = NULL, *hdr_short = NULL;
	u8* hist_virt = NULL;

	if (copy_from_user(&mw_cmd, arg, sizeof(mw_cmd)))
		return -EFAULT;

	if (!mw_cmd.rgb_statis || !mw_cmd.rgb_data_valid ||
		!mw_cmd.cfa_statis || !mw_cmd.cfa_data_valid) {
		return -EFAULT;
	}

	/* Wait for data ready */
	cnt = G_img.statis_3a_cnt + 1;
	mutex_unlock(context->iav_mutex);
	wait_event_interruptible(G_img.statis_wq, (G_img.statis_3a_cnt >= cnt));
	mutex_lock(context->iav_mutex);

	put_user(0, mw_cmd.rgb_data_valid);
	if (G_img.rgb_entry[0] != INVALID_AAA_DATA_ENTRY) {
		/* Copy RGB AAA data */
		rgb = (struct rgb_aaa_stat *)(G_img.addr[IMB_RGB_FIFO_BASE] +
			G_img.rgb_entry[0] * RGB_AAA_DATA_BLOCK);
		slice = rgb->aaa_tile_info.total_slices_x;
		dst = (u8*)mw_cmd.rgb_statis;
		for (i = 0; i < slice; ++i, dst += RGB_AAA_DATA_BLOCK) {
			rgb = (struct rgb_aaa_stat *)(G_img.addr[IMB_RGB_FIFO_BASE] +
				G_img.rgb_entry[i] * RGB_AAA_DATA_BLOCK);
			slice_x = rgb->aaa_tile_info.slice_index_x;
			if (slice_x >= slice) {
				iav_error("Incorrect slice [%d] of RGB data.\n", slice_x);
				return -EFAULT;
			}
			if (copy_to_user(dst, rgb, RGB_AAA_DATA_BLOCK)) {
				iav_error("Failed to copy RGB data of slice [%d].\n", i);
				return -EFAULT;
			}
		}
		put_user(1, mw_cmd.rgb_data_valid);
	}

	put_user(0, mw_cmd.cfa_data_valid);
	if (G_img.cfa_entry[0] != INVALID_AAA_DATA_ENTRY) {
		/* Copy CFA AAA data */
		cfa = (struct cfa_aaa_stat *)(G_img.addr[IMB_CFA_FIFO_BASE] +
			G_img.cfa_entry[0] * CFA_AAA_DATA_BLOCK);
		slice = cfa->aaa_tile_info.total_slices_x;
		dst = (u8*)mw_cmd.cfa_statis;
		for (i = 0; i < slice; ++i, dst += CFA_AAA_DATA_BLOCK) {
			cfa = (struct cfa_aaa_stat *)(G_img.addr[IMB_CFA_FIFO_BASE] +
				G_img.cfa_entry[i] * CFA_AAA_DATA_BLOCK);
			slice_x = cfa->aaa_tile_info.slice_index_x;
			if (slice_x >= slice) {
				iav_error("Incorrect slice [%d] of CFA data.\n", slice_x);
				return -EFAULT;
			}
			if (copy_to_user(dst, cfa, CFA_AAA_DATA_BLOCK)) {
				iav_error("Failed to copy CFA data of slice [%d].\n", i);
				return -EFAULT;
			}
		}
		put_user(1, mw_cmd.cfa_data_valid);
	}

	/* Copy pre-HDR histogram data*/
	if (context->hdr_mode && mw_cmd.cfa_hdr_statis && mw_cmd.cfa_hdr_valid) {
		put_user(0, mw_cmd.cfa_hdr_valid);
		if (G_img.hdr_entry[0] != INVALID_AAA_DATA_ENTRY) {
			hdr_main = (struct cfa_pre_hdr_stat *)(G_img.addr[IMB_HDR_BLEND_FIFO_BASE] +
				G_img.hdr_entry[0] * CFA_PRE_HDR_BLOCK);
			slice = hdr_main->cfg_info.total_slice_in_x;
			total = slice * (hdr_main->cfg_info.total_exposures - 1);

			dst = (u8*)mw_cmd.cfa_hdr_statis;
			for (i = 0; i < total; ++i, dst += CFA_PRE_HDR_BLOCK) {
				/* Copy Main data */
				hdr_main = (struct cfa_pre_hdr_stat *)(G_img.addr[IMB_HDR_BLEND_FIFO_BASE] +
					G_img.hdr_entry[i] * CFA_PRE_HDR_BLOCK);
				slice_x = hdr_main->cfg_info.total_exposures;
				if (slice_x > (G_img.expo_num_minus_1 + 1)) {
					iav_error("Incorrect total expo [%d] of Main data.\n", slice_x);
					return -EFAULT;
				}
				slice_x = hdr_main->cfg_info.slice_index_x;
				if (slice_x >= slice) {
					iav_error("Incorrect slice [%d] of Main data.\n", slice_x);
					return -EFAULT;
				}
				if (copy_to_user(dst, hdr_main, CFA_PRE_HDR_BLOCK)) {
					iav_error("Failed to copy Main data of slice [%d].\n", slice_x);
					return -EFAULT;
				}

				/* Copy HDR data */
				hdr_short = (struct cfa_pre_hdr_stat *)(G_img.addr[IMB_HDR_SHORT_FIFO_BASE] +
					G_img.hdr_entry[i] * CFA_PRE_HDR_BLOCK);
				slice_x = hdr_short->cfg_info.total_exposures;
				if (slice_x > (G_img.expo_num_minus_1 + 1)) {
					iav_error("Incorrect total expo [%d] of Short data.\n", slice_x);
					return -EFAULT;
				}
				slice_x = hdr_short->cfg_info.slice_index_x;
				if (slice_x >= slice) {
					iav_error("Incorrect slice [%d] of Short data.\n", slice_x);
					return -EFAULT;
				}
				if (copy_to_user(dst + (MAX_EXPOSURE_NUM - 1) * 2 * CFA_PRE_HDR_BLOCK,
					hdr_short, CFA_PRE_HDR_BLOCK)) {
					iav_error("Failed to copy Short data of slice [%d].\n", slice_x);
					return -EFAULT;
				}
			}

			put_user(1, mw_cmd.cfa_hdr_valid);
		}
	}

	/* Copy sensor embedded histogram data */
	if (mw_cmd.hist_data_valid && mw_cmd.hist_statis && G_img.raw_hist_addr) {
		put_user(0, mw_cmd.hist_data_valid);
		hist_virt = (u8*)(G_img.raw_hist_addr - context->dsp_phys + context->dsp_virt);

		/* pitch size and raw width, 4 bytes */
		if(put_user(G_img.raw_pitch, (u16*)mw_cmd.hist_statis)){
			return -EFAULT;
		}
		if(put_user(context->cap_width, (u16*)(mw_cmd.hist_statis + sizeof(u16)))){
			return -EFAULT;
		}

		/* bottom 2 line which contain histogram information */
		if(copy_to_user(mw_cmd.hist_statis + sizeof(u16) * 2,
			hist_virt + (context->cap_height - SENSOR_HIST_ROW_NUM ) * G_img.raw_pitch ,
			SENSOR_HIST_ROW_NUM * G_img.raw_pitch)){
			return -EFAULT;
		}
		put_user(1, mw_cmd.hist_data_valid);
	}

	put_user(G_img.dsp_pts, mw_cmd.dsp_pts_data_addr);
	put_user(G_img.mono_pts, mw_cmd.mono_pts_data_addr);
	return 0;
}

static int dsp_aaa_statistics_af_setup(struct iav_imgproc_info *context,
	struct aaa_statistics_ex *param)
{
	aaa_statistics_setup1_t dsp_cmd1;
	aaa_statistics_setup2_t dsp_cmd2;
	struct aaa_statistics_ex mw_cmd;
	u32 i;

	if (copy_from_user(&mw_cmd, param, sizeof(mw_cmd)))
		return -EFAULT;

	memset(&dsp_cmd1, 0, sizeof(dsp_cmd1));
	memset(&dsp_cmd2, 0, sizeof(dsp_cmd2));

	dsp_cmd1.cmd_code = AAA_STATISTICS_SETUP1;
	dsp_cmd1.af_horizontal_filter1_mode = mw_cmd.af_horizontal_filter1_mode;
	dsp_cmd1.af_horizontal_filter1_stage1_enb = mw_cmd.af_horizontal_filter1_stage1_enb;
	dsp_cmd1.af_horizontal_filter1_stage2_enb = mw_cmd.af_horizontal_filter1_stage2_enb;
	dsp_cmd1.af_horizontal_filter1_stage3_enb = mw_cmd.af_horizontal_filter1_stage3_enb;

	for (i = 0; i < 7; i++)
		dsp_cmd1.af_horizontal_filter1_gain[i] = mw_cmd.af_horizontal_filter1_gain[i];
	for (i = 0; i < 4; i++)
		dsp_cmd1.af_horizontal_filter1_shift[i] = mw_cmd.af_horizontal_filter1_shift[i];

	dsp_cmd1.af_horizontal_filter1_bias_off = mw_cmd.af_horizontal_filter1_bias_off;
	dsp_cmd1.af_horizontal_filter1_thresh = mw_cmd.af_horizontal_filter1_thresh;
	dsp_cmd1.af_vertical_filter1_thresh = mw_cmd.af_vertical_filter1_thresh;
	dsp_cmd1.af_tile_fv1_horizontal_shift = mw_cmd.af_tile_fv1_horizontal_shift;
	dsp_cmd1.af_tile_fv1_vertical_shift = mw_cmd.af_tile_fv1_vertical_shift;
	dsp_cmd1.af_tile_fv1_horizontal_weight = mw_cmd.af_tile_fv1_horizontal_weight;
	dsp_cmd1.af_tile_fv1_vertical_weight = mw_cmd.af_tile_fv1_vertical_weight;

	dsp_issue_img_cmd(&dsp_cmd1, sizeof(dsp_cmd1));
	if (G_img.save_cmd) {
		memcpy(&G_img_cmds[IMG_CMD_AAA_STATISTICS_SETUP1], &dsp_cmd1,
			sizeof(DSP_CMD));
	}

	dsp_cmd2.cmd_code = AAA_STATISTICS_SETUP2;
	dsp_cmd2.af_horizontal_filter2_mode = mw_cmd.af_horizontal_filter2_mode;
	dsp_cmd2.af_horizontal_filter2_stage1_enb = mw_cmd.af_horizontal_filter2_stage1_enb;
	dsp_cmd2.af_horizontal_filter2_stage2_enb = mw_cmd.af_horizontal_filter2_stage2_enb;
	dsp_cmd2.af_horizontal_filter2_stage3_enb = mw_cmd.af_horizontal_filter2_stage3_enb;

	for (i = 0; i < 7; i ++) {
		dsp_cmd2.af_horizontal_filter2_gain[i] =
			mw_cmd.af_horizontal_filter2_gain[i];
	}
	for (i = 0; i < 4; i++) {
		dsp_cmd2.af_horizontal_filter2_shift[i] =
			mw_cmd.af_horizontal_filter2_shift[i];
	}

	dsp_cmd2.af_horizontal_filter2_bias_off = mw_cmd.af_horizontal_filter2_bias_off;
	dsp_cmd2.af_horizontal_filter2_thresh = mw_cmd.af_horizontal_filter2_thresh;
	dsp_cmd2.af_vertical_filter2_thresh = mw_cmd.af_vertical_filter2_thresh;
	dsp_cmd2.af_tile_fv2_horizontal_shift = mw_cmd.af_tile_fv2_horizontal_shift;
	dsp_cmd2.af_tile_fv2_vertical_shift = mw_cmd.af_tile_fv2_vertical_shift;
	dsp_cmd2.af_tile_fv2_horizontal_weight = mw_cmd.af_tile_fv2_horizontal_weight;
	dsp_cmd2.af_tile_fv2_vertical_weight = mw_cmd.af_tile_fv2_vertical_weight;

	dsp_issue_img_cmd(&dsp_cmd2, sizeof(dsp_cmd2));
	if (G_img.save_cmd) {
		memcpy(&G_img_cmds[IMG_CMD_AAA_STATISTICS_SETUP2], &dsp_cmd2,
			sizeof(DSP_CMD));
	}

	return 0;
}

int dsp_noise_filter_setup (struct iav_imgproc_info *context, u32 strength)
{
	noise_filter_setup_t dsp_cmd;

	if (!is_in_3a_work_state(context))
		return -EPERM;

	memset(&dsp_cmd, 0, sizeof(dsp_cmd));
	dsp_cmd.cmd_code = NOISE_FILTER_SETUP;

	dsp_cmd.strength = strength;

	dsp_issue_img_cmd(&dsp_cmd, sizeof(dsp_cmd));

	return 0;
}

static int dsp_aaa_statistics_setup(struct iav_imgproc_info *context,
	aaa_statistics_setup_t __user * param)
{
	aaa_statistics_setup_t dsp_cmd;

	if (!is_in_3a_work_state(context))
		return -EPERM;

	if (copy_from_user(&dsp_cmd, param, sizeof(dsp_cmd))) {
		return -EFAULT;
	}

	if (!G_img.addr[IMB_RGB_FIFO_BASE] || !G_img.addr[IMB_CFA_FIFO_BASE]) {
		iav_error("AAA statistics data memory is NOT allocated yet.\n");
		return -ENOMEM;
	}

	dsp_cmd.cmd_code = AAA_STATISTICS_SETUP;
	dsp_cmd.data_fifo_base = PHYS_TO_DSP(G_img.addr[IMB_RGB_FIFO_BASE] -
		G_img.rd_virt + G_img.rd_phys);
	dsp_cmd.data_fifo_limit = dsp_cmd.data_fifo_base + IMS_RGB_FIFO_BASE;
	dsp_cmd.data_fifo2_base = PHYS_TO_DSP(G_img.addr[IMB_CFA_FIFO_BASE] -
		G_img.rd_virt + G_img.rd_phys);
	dsp_cmd.data_fifo2_limit = dsp_cmd.data_fifo2_base + IMS_CFA_FIFO_BASE;

	/* Todo: add AWB & AE & AF tile config */

	dsp_issue_img_cmd(&dsp_cmd, sizeof(dsp_cmd));
	if (G_img.save_cmd) {
		memcpy(&G_img_cmds[IMG_CMD_AAA_STATISTICS_SETUP], &dsp_cmd,
			sizeof(DSP_CMD));
	}

	return 0;
}

static int dsp_aaa_histogram_setup(struct iav_imgproc_info *context,
	aaa_histogram_t __user * param)
{
	aaa_histogram_t dsp_cmd;

	if (!is_in_3a_work_state(context))
		return -EPERM;

	if (copy_from_user(&dsp_cmd, param, sizeof(dsp_cmd))) {
		return -EFAULT;
	}

	dsp_cmd.cmd_code = AAA_HISTORGRAM_SETUP;

	dsp_issue_img_cmd(&dsp_cmd, sizeof(dsp_cmd));
	if (G_img.save_cmd) {
		memcpy(&G_img_cmds[IMG_CMD_AAA_HISTORGRAM_SETUP], &dsp_cmd,
			sizeof(DSP_CMD));
	}

	return 0;
}

static int dsp_cfa_pre_hdr_statistics_setup(struct iav_imgproc_info *context,
	vin_stats_setup_t __user * param)
{
	vin_stats_setup_t dsp_cmd;

	if (!is_in_3a_work_state(context))
		return -EPERM;

	if (copy_from_user(&dsp_cmd, param, sizeof(dsp_cmd)))
		return -EFAULT;

	if (!G_img.addr[IMB_HDR_BLEND_FIFO_BASE] ||
		!G_img.addr[IMB_HDR_SHORT_FIFO_BASE]) {
		iav_error("CFA HDR statistics data memory is NOT allocated yet.\n");
		return -ENOMEM;
	}

	dsp_cmd.cmd_code = VIN_STATISTICS_SETUP;
	dsp_cmd.main_data_fifo_base= PHYS_TO_DSP(G_img.addr[IMB_HDR_BLEND_FIFO_BASE] -
		G_img.rd_virt + G_img.rd_phys);
	dsp_cmd.main_data_fifo_limit= dsp_cmd.main_data_fifo_base + IMS_HDR_BLEND_FIFO_BASE;
	dsp_cmd.hdr_data_fifo_base= PHYS_TO_DSP(G_img.addr[IMB_HDR_SHORT_FIFO_BASE] -
		G_img.rd_virt + G_img.rd_phys);
	dsp_cmd.hdr_data_fifo_limit= dsp_cmd.hdr_data_fifo_base + IMS_HDR_SHORT_FIFO_BASE;

	dsp_issue_img_cmd(&dsp_cmd, sizeof(dsp_cmd));
	if (G_img.save_cmd) {
		memcpy(&G_img_cmds[IMG_CMD_VIN_STATISTICS_SETUP], &dsp_cmd,
			sizeof(DSP_CMD));
	}

	return 0;
}

static int dsp_mctf_mv_stabilizer_setup (struct iav_imgproc_info *context,
	mctf_mv_stab_setup_t __user *param)
{
	u8 * addr = G_img.addr[IMB_MCTF_CFG];
	mctf_mv_stab_setup_t dsp_cmd;

	if (!is_in_3a_work_state(context))
		return -EPERM;

	if (copy_from_user(&dsp_cmd, param, sizeof(dsp_cmd)))
		return -EFAULT;

	if (copy_from_user(addr, (void*)dsp_cmd.mctf_cfg_dram_addr, IMS_MCTF_CFG))
		return -EFAULT;

	dsp_cmd.cmd_code = MCTF_MV_STAB_SETUP;
	dsp_cmd.mctf_cfg_dram_addr = VIRT_TO_DSP(addr);
	clean_d_cache(addr, IMS_MCTF_CFG);

	dsp_issue_img_cmd(&dsp_cmd, sizeof(dsp_cmd));

	return 0;
}

static int dsp_black_level_global_offset(struct iav_imgproc_info *context,
	black_level_global_offset_t __user *param)
{
	black_level_global_offset_t	dsp_cmd;

	if (!is_in_3a_work_state(context))
		return -EPERM;

	if (copy_from_user(&dsp_cmd, param, sizeof(dsp_cmd)))
		return -EFAULT;

	dsp_cmd.cmd_code = BLACK_LEVEL_GLOBAL_OFFSET;

	dsp_issue_img_cmd(&dsp_cmd, sizeof(dsp_cmd));
	if (G_img.save_cmd) {
		memcpy(&G_img_cmds[IMG_CMD_BLACK_LEVEL_GLOBAL_OFFSET], &dsp_cmd,
			sizeof(DSP_CMD));
	}

	return 0;
}

static int dsp_cfa_domain_leakage_filter_setup(struct iav_imgproc_info *context,
	cfa_domain_leakage_filter_t __user *param)
{
	cfa_domain_leakage_filter_t dsp_cmd;

	if (!is_in_3a_work_state(context))
		return -EPERM;

	if (copy_from_user(&dsp_cmd, param, sizeof(dsp_cmd)))
		return -EFAULT;

	dsp_cmd.cmd_code = CFA_DOMAIN_LEAKAGE_FILTER;

	dsp_issue_img_cmd(&dsp_cmd, sizeof(dsp_cmd));

	return 0;
}

static int dsp_cfa_noise_filter (struct iav_imgproc_info *context,
	cfa_noise_filter_t __user *param)
{
	cfa_noise_filter_t dsp_cmd;

	if (!is_in_3a_work_state(context))
		return -EPERM;

	if(copy_from_user(&dsp_cmd, param, sizeof(dsp_cmd)))
		return -EFAULT;

	dsp_cmd.cmd_code = CFA_NOISE_FILTER;

	dsp_issue_img_cmd(&dsp_cmd, sizeof(dsp_cmd));

	return 0;
}

static int dsp_vignette_compensation(struct iav_imgproc_info *context,
	vignette_compensation_t __user *param)
{
	vignette_compensation_t dsp_cmd;

	if (!is_in_3a_work_state(context))
		return -EPERM;

	if (copy_from_user(&dsp_cmd, param, sizeof(dsp_cmd)))
		return -EFAULT;
	if (copy_from_user(G_img.addr[IMB_VIGNETTE_R_GAIN],
		(void*)param->tile_gain_addr, IMS_VIGNETTE_R_GAIN))
		return -EFAULT;
	if (copy_from_user(G_img.addr[IMB_VIGNETTE_GE_GAIN],
		(void*)param->tile_gain_addr_green_even, IMS_VIGNETTE_GE_GAIN))
		return -EFAULT;
	if (copy_from_user(G_img.addr[IMB_VIGNETTE_GO_GAIN],
		(void*)param->tile_gain_addr_green_odd, IMS_VIGNETTE_GO_GAIN))
		return -EFAULT;
	if (copy_from_user(G_img.addr[IMB_VIGNETTE_B_GAIN],
		(void*)param->tile_gain_addr_blue, IMS_VIGNETTE_B_GAIN))
		return -EFAULT;

	dsp_cmd.cmd_code = VIGNETTE_COMPENSATION;
	dsp_cmd.tile_gain_addr = VIRT_TO_DSP(G_img.addr[IMB_VIGNETTE_R_GAIN]);
	dsp_cmd.tile_gain_addr_green_even = VIRT_TO_DSP(G_img.addr[IMB_VIGNETTE_GE_GAIN]);
	dsp_cmd.tile_gain_addr_green_odd = VIRT_TO_DSP(G_img.addr[IMB_VIGNETTE_GO_GAIN]);
	dsp_cmd.tile_gain_addr_blue = VIRT_TO_DSP(G_img.addr[IMB_VIGNETTE_B_GAIN]);

	clean_d_cache(G_img.addr[IMB_VIGNETTE_R_GAIN], IMS_VIGNETTE_R_GAIN);
	clean_d_cache(G_img.addr[IMB_VIGNETTE_GE_GAIN], IMS_VIGNETTE_GE_GAIN);
	clean_d_cache(G_img.addr[IMB_VIGNETTE_GO_GAIN], IMS_VIGNETTE_GO_GAIN);
	clean_d_cache(G_img.addr[IMB_VIGNETTE_B_GAIN], IMS_VIGNETTE_B_GAIN);

	dsp_issue_img_cmd(&dsp_cmd, sizeof(dsp_cmd));
	if (G_img.save_cmd) {
		memcpy(&G_img_cmds[IMG_CMD_VIGNETTE_COMPENSATION], &dsp_cmd,
			sizeof(DSP_CMD));
	}

	return 0;
}

static int dsp_local_exposure (struct iav_imgproc_info *context,
	local_exposure_t __user *param)
{
	u8 * addr = G_img.addr[IMB_EXPOSURE_GAIN_CURVE];
	local_exposure_t dsp_cmd;

	if (!is_in_3a_work_state(context))
		return -EPERM;

	if (copy_from_user(&dsp_cmd, param, sizeof(dsp_cmd)))
		return -EFAULT;

	if (copy_from_user(addr, (void*)(param->gain_curve_table_addr),
		IMS_EXPOSURE_GAIN_CURVE))
		return -EFAULT;

	dsp_cmd.cmd_code = LOCAL_EXPOSURE;
	dsp_cmd.gain_curve_table_addr = VIRT_TO_DSP(addr);
	clean_d_cache(addr, IMS_EXPOSURE_GAIN_CURVE);

	dsp_issue_img_cmd(&dsp_cmd, sizeof(dsp_cmd));
	if (G_img.save_cmd) {
		memcpy(&G_img_cmds[IMG_CMD_LOCAL_EXPOSURE], &dsp_cmd, sizeof(DSP_CMD));
	}

	return 0;
}

static int dsp_color_correction (struct iav_imgproc_info *context,
	color_correction_t __user *param)
{
	color_correction_t dsp_cmd;

	if (!is_in_3a_work_state(context))
		return -EPERM;

	if (copy_from_user(&dsp_cmd, param, sizeof(dsp_cmd)))
		return -EFAULT;

	dsp_cmd.cmd_code = COLOR_CORRECTION;
	dsp_cmd.in_lookup_table_addr = VIRT_TO_DSP(G_img.addr[IMB_INPUT_LUT]);
	dsp_cmd.matrix_addr = VIRT_TO_DSP(G_img.addr[IMB_MATRIX_DRAM]);

	if (dsp_cmd.output_lookup_bypass == 1) {
		dsp_cmd.out_lookup_table_addr = 0;
	} else {
		if (param->out_lookup_table_addr) {
			if (copy_from_user(G_img.addr[IMB_OUTPUT_LUT],
				(void*)param->out_lookup_table_addr, IMS_OUTPUT_LUT))
				return -EFAULT;
		}
		dsp_cmd.out_lookup_table_addr = VIRT_TO_DSP(G_img.addr[IMB_OUTPUT_LUT]);
	}
	if (param->in_lookup_table_addr) {
		if (copy_from_user(G_img.addr[IMB_INPUT_LUT],
			(void*)param->in_lookup_table_addr, IMS_INPUT_LUT))
			return -EFAULT;
	}
	if (param->matrix_addr) {
		if (copy_from_user(G_img.addr[IMB_MATRIX_DRAM],
			(void*)param->matrix_addr, IMS_MATRIX_DRAM))
			return -EFAULT;
	}

	clean_d_cache(G_img.addr[IMB_INPUT_LUT], IMS_INPUT_LUT);
	clean_d_cache(G_img.addr[IMB_MATRIX_DRAM], IMS_MATRIX_DRAM);
	clean_d_cache(G_img.addr[IMB_OUTPUT_LUT], IMS_OUTPUT_LUT);

	dsp_issue_img_cmd(&dsp_cmd, sizeof(dsp_cmd));

	return 0;
}

static int dsp_rgb_to_yuv_setup (struct iav_imgproc_info *context,
	rgb_to_yuv_setup_t __user *param)
{
	rgb_to_yuv_setup_t dsp_cmd;

	if (!is_in_3a_work_state(context))
		return -EPERM;

	if (copy_from_user(&dsp_cmd, param, sizeof(dsp_cmd)))
		return -EFAULT;

	dsp_cmd.cmd_code = RGB_TO_YUV_SETUP;

	dsp_issue_img_cmd(&dsp_cmd, sizeof(dsp_cmd));

	return 0;
}

static int dsp_chroma_scale (struct iav_imgproc_info *context,
	chroma_scale_t __user *param)
{
	u8 * addr = G_img.addr[IMB_CHROMA_GAIN_CURVE];
	chroma_scale_t dsp_cmd;

	if (!is_in_3a_work_state(context))
		return -EPERM;

	if (copy_from_user(&dsp_cmd, param, sizeof(dsp_cmd)))
		return -EFAULT;
	if (copy_from_user(addr, (void*)(param->gain_curver_addr),
		IMS_CHROMA_GAIN_CURVE))
		return -EFAULT;

	dsp_cmd.cmd_code = CHROMA_SCALE;
	dsp_cmd.gain_curver_addr = VIRT_TO_DSP(addr);
	clean_d_cache(addr, IMS_CHROMA_GAIN_CURVE);

	dsp_issue_img_cmd(&dsp_cmd, sizeof(dsp_cmd));

	return 0;
}

static int dsp_chroma_noise_filter (struct iav_imgproc_info *context,
	chroma_noise_filter_t __user *param)
{
	chroma_noise_filter_t dsp_cmd;

	if (!is_in_3a_work_state(context))
		return -EPERM;

	if (copy_from_user(&dsp_cmd, param, sizeof(dsp_cmd)))
		return -EFAULT;

	dsp_cmd.cmd_code = CHROMA_NOISE_FILTER;

	if(context->hdr_mode != 2){
		dsp_issue_img_cmd(&dsp_cmd, sizeof(dsp_cmd));
	}

	return 0;
}

static int dsp_chroma_median_filter (struct iav_imgproc_info *context,
	chroma_median_filter_info_t __user *param)
{
	u8 * addr = G_img.addr[IMB_K0123_TABLE];
	chroma_median_filter_info_t dsp_cmd;

	if (!is_in_3a_work_state(context))
		return -EPERM;

	if (copy_from_user(&dsp_cmd, param, sizeof(dsp_cmd)))
		return -EFAULT;
	if (copy_from_user(addr, (void*)param->k0123_table_addr, IMS_K0123_TABLE))
		return -EFAULT;

	dsp_cmd.cmd_code = CHROMA_MEDIAN_FILTER;
	dsp_cmd.k0123_table_addr = VIRT_TO_DSP(addr);

	clean_d_cache(addr, IMS_K0123_TABLE);

	dsp_issue_img_cmd(&dsp_cmd, sizeof(dsp_cmd));

	return 0;
}

static int dsp_luma_sharpening (struct iav_imgproc_info *context,
	luma_sharpening_t __user *param)
{
	u8 * addr = G_img.addr[IMB_LUMA_SHARPEN_ALPHA_TABLE];
	luma_sharpening_t dsp_cmd;

	if (!is_in_3a_work_state(context))
		return -EPERM;

	if (copy_from_user(&dsp_cmd, param, sizeof(dsp_cmd)))
		return -EFAULT;
	if (copy_from_user(addr, (void*)param->alpha_table_addr,
		IMS_LUMA_SHARPEN_ALPHA_TABLE))
		return -EFAULT;

	dsp_cmd.cmd_code = LUMA_SHARPENING;
	dsp_cmd.alpha_table_addr = VIRT_TO_DSP(addr);

	clean_d_cache(addr, IMS_LUMA_SHARPEN_ALPHA_TABLE);

	dsp_issue_img_cmd(&dsp_cmd, sizeof(dsp_cmd));

	return 0;
}

static int dsp_luma_sharp_fir (struct iav_imgproc_info *context,
	luma_sharpening_FIR_config_t __user *param)
{
	luma_sharpening_FIR_config_t dsp_cmd;

	if (!is_in_3a_work_state(context))
		return -EPERM;

	if (copy_from_user(&dsp_cmd, param, sizeof(dsp_cmd)))
		return -EFAULT;
	if (copy_from_user(G_img.addr[IMB_COEFF_FIR_0],
		(void*)param->coeff_FIR1_addr, IMS_COEFF_FIR_0))
		return -EFAULT;
	if (copy_from_user(G_img.addr[IMB_COEFF_FIR_1],
		(void*)param->coeff_FIR2_addr, IMS_COEFF_FIR_1))
		return -EFAULT;
	if (copy_from_user(G_img.addr[IMB_CORING_TABLE],
		(void*)param->coring_table_addr, IMS_CORING_TABLE))
		return -EFAULT;

	dsp_cmd.cmd_code = LUMA_SHARPENING_FIR_CONFIG;
	dsp_cmd.coeff_FIR1_addr = VIRT_TO_DSP(G_img.addr[IMB_COEFF_FIR_0]);
	dsp_cmd.coeff_FIR2_addr =  VIRT_TO_DSP(G_img.addr[IMB_COEFF_FIR_1]);
	dsp_cmd.coring_table_addr = VIRT_TO_DSP(G_img.addr[IMB_CORING_TABLE]);

	clean_d_cache(G_img.addr[IMB_COEFF_FIR_0], IMS_COEFF_FIR_0);
	clean_d_cache(G_img.addr[IMB_COEFF_FIR_1], IMS_COEFF_FIR_1);
	clean_d_cache(G_img.addr[IMB_CORING_TABLE], IMS_CORING_TABLE);

	dsp_issue_img_cmd(&dsp_cmd, sizeof(dsp_cmd));

	return 0;
}

static int dsp_luma_sharpen_blend_ctrl (struct iav_imgproc_info *context,
	luma_sharpening_blend_control_t __user *param)
{
	luma_sharpening_blend_control_t	dsp_cmd;

	if (!is_in_3a_work_state(context))
		return -EPERM;

	if (copy_from_user(&dsp_cmd, param, sizeof(dsp_cmd)))
		return -EFAULT;

	dsp_cmd.cmd_code = LUMA_SHARPENING_BLEND_CONTROL;

	dsp_issue_img_cmd(&dsp_cmd, sizeof(dsp_cmd));

	return 0;
}

static int dsp_luma_sharpen_level_ctrl (struct iav_imgproc_info *context,
	luma_sharpening_level_control_t __user *param)
{
	luma_sharpening_level_control_t	dsp_cmd;

	if (!is_in_3a_work_state(context))
		return -EPERM;

	if (copy_from_user(&dsp_cmd, param, sizeof(dsp_cmd)))
		return -EFAULT;

	dsp_cmd.cmd_code = LUMA_SHARPENING_LEVEL_CONTROL;

	dsp_issue_img_cmd(&dsp_cmd, sizeof(dsp_cmd));

	return 0;
}

static int dsp_luma_sharpen_tone(struct iav_imgproc_info *context,
	luma_sharpening_tone_control_t __user *param)
{
	u8 * addr = G_img.addr[IMB_LUMA_3D_TABLE];
	luma_sharpening_tone_control_t	dsp_cmd;

	if (!is_in_3a_work_state(context))
		return -EPERM;

	if (copy_from_user(&dsp_cmd, param, sizeof(dsp_cmd)))
		return -EFAULT;
	if (copy_from_user(addr, (void*)param->tone_based_3d_level_table_addr,
		IMS_LUMA_3D_TABLE))
		return -EFAULT;

	dsp_cmd.cmd_code = LUMA_SHARPENING_TONE;
	dsp_cmd.tone_based_3d_level_table_addr = VIRT_TO_DSP(addr);

	clean_d_cache(addr, IMS_LUMA_3D_TABLE);

	dsp_issue_img_cmd(&dsp_cmd, sizeof(dsp_cmd));

	return 0;
}

static int dsp_rgb_gain_adjustment (struct iav_imgproc_info *context,
	rgb_gain_adjust_t __user *param)
{
	rgb_gain_adjust_t dsp_cmd;

	if (!is_in_3a_work_state(context))
		return -EPERM;

	if (copy_from_user(&dsp_cmd, param, sizeof(dsp_cmd)))
		return -EFAULT;

 	dsp_cmd.cmd_code = RGB_GAIN_ADJUSTMENT;

	dsp_issue_img_cmd(&dsp_cmd, sizeof(dsp_cmd));
	if(G_img.save_cmd) {
		memcpy(&G_img_cmds[IMG_CMD_RGB_GAIN_ADJUSTMENT], &dsp_cmd,
			sizeof(DSP_CMD));
	}

	return 0;
}

static int dsp_anti_aliasing_filter(struct iav_imgproc_info *context,
	anti_aliasing_filter_t __user* param)
{
	anti_aliasing_filter_t dsp_cmd;

	if (!is_in_3a_work_state(context))
		return -EPERM;

	if (copy_from_user(&dsp_cmd, param, sizeof(dsp_cmd)))
		return -EFAULT;

	dsp_cmd.cmd_code = ANTI_ALIASING;

	dsp_issue_img_cmd(&dsp_cmd, sizeof(dsp_cmd));

	return 0;
}

static int dsp_strong_grgb_mismatch_filter(struct iav_imgproc_info *context,
	strong_grgb_mismatch_filter_t __user* param)
{
	strong_grgb_mismatch_filter_t dsp_cmd;

	if (!is_in_3a_work_state(context))
		return -EPERM;

	if (copy_from_user(&dsp_cmd, param, sizeof(dsp_cmd)))
		return -EFAULT;

	dsp_cmd.cmd_code = STRONG_GRGB_MISMATCH_FILTER;

	dsp_issue_img_cmd(&dsp_cmd, sizeof(dsp_cmd));

	return 0;
}

static int dsp_digital_gain_sat_level(struct iav_imgproc_info *context,
	digital_gain_level_t __user* param)
{
	digital_gain_level_t dsp_cmd;

	if (!is_in_3a_work_state(context))
		return -EPERM;

	if (copy_from_user(&dsp_cmd, param, sizeof(dsp_cmd)))
		return -EFAULT;

	dsp_cmd.cmd_code = DIGITAL_GAIN_SATURATION_LEVEL;

	dsp_issue_img_cmd(&dsp_cmd, sizeof(dsp_cmd));
	if (G_img.save_cmd) {
		memcpy(&G_img_cmds[IMG_CMD_DIGITAL_GAIN_SATURATION_LEVEL], &dsp_cmd,
			sizeof(DSP_CMD));
	}

	return 0;
}

static int dsp_bad_pixel_correct_setup (struct iav_imgproc_info *context,
	bad_pixel_correct_setup_t __user* param)
{
	u8 *hot = NULL, *dark = NULL;
	bad_pixel_correct_setup_t dsp_cmd;

	// Suggest to use another IOCTL
	iav_error("3A Should call IAV_IOC_CFG_BPC_SETUP instead!\n");
	return -EPERM;

	if (!is_in_3a_work_state(context))
		return -EPERM;

	if (copy_from_user(&dsp_cmd, param, sizeof(dsp_cmd)))
		return -EFAULT;

	hot = G_img.addr[IMB_HOT_PIXEL_THD_TABLE];
	if (copy_from_user(hot, (void*)param->hot_pixel_thresh_addr,
		IMS_HOT_PIXEL_THD_TABLE))
		return -EFAULT;
	dark = G_img.addr[IMB_DARK_PIXEL_THD_TABLE];
	if (copy_from_user(dark, (void*)param->dark_pixel_thresh_addr,
		IMS_DARK_PIXEL_THD_TABLE))
		return -EFAULT;

	dsp_cmd.cmd_code = BAD_PIXEL_CORRECT_SETUP;
	dsp_cmd.hot_pixel_thresh_addr = VIRT_TO_DSP(hot);
	dsp_cmd.dark_pixel_thresh_addr = VIRT_TO_DSP(dark);

	clean_d_cache(hot, IMS_HOT_PIXEL_THD_TABLE);
	clean_d_cache(dark, IMS_DARK_PIXEL_THD_TABLE);

	dsp_issue_img_cmd(&dsp_cmd, sizeof(dsp_cmd));

	return 0;
}

static int dsp_demoasic_filter (struct iav_imgproc_info *context,
	demoasic_filter_t __user* param)
{
	demoasic_filter_t dsp_cmd;

	if (!is_in_3a_work_state(context))
		return -EPERM;

	if (copy_from_user(&dsp_cmd, param, sizeof(dsp_cmd)))
		return -EFAULT;

	dsp_cmd.cmd_code = DEMOASIC_FILTER;

	dsp_issue_img_cmd(&dsp_cmd, sizeof(dsp_cmd));

	return 0;
}

static int dsp_sensor_input_setup (struct iav_imgproc_info *context,
	sensor_input_setup_t *param)
{
	sensor_input_setup_t dsp_cmd;

	if (copy_from_user(&dsp_cmd, param, sizeof(dsp_cmd)))
		return -EFAULT;

 	dsp_cmd.cmd_code = SENSOR_INPUT_SETUP;

//	dsp_issue_img_cmd(&dsp_cmd, sizeof(dsp_cmd));
	return 0;
}

static int dsp_fixed_pattern_noise_correct (struct iav_imgproc_info *context,
	fixed_pattern_noise_correct_t __user *arg)
{
	u8 * bpc, * fpn;
	fixed_pattern_noise_correct_t dsp_cmd;

	// Suggest to use another IOCTL
	iav_error("3A Should call IAV_IOC_APPLY_STATIC_BPC instead !\n");
	return -EPERM;

	if (copy_from_user(&dsp_cmd, arg, sizeof(dsp_cmd)))
		return -EFAULT;

	/* BPC memory is moved to PM buffer. */
	if (dsp_cmd.fpn_pixels_buf_size > IMS_PIXEL_MAP)
		return -EINVAL;

	bpc = G_img.addr[IMB_PIXEL_MAP];
	fpn = G_img.addr[IMB_FPN_REG];
       if (dsp_cmd.fpn_pixel_mode == 0) {
		memset(bpc, 0, dsp_cmd.fpn_pixels_buf_size);
	} else {
		if (copy_from_user(bpc, (void*)dsp_cmd.fpn_pixels_addr, dsp_cmd.fpn_pixels_buf_size))
			return -EFAULT;
	}
	if (copy_from_user(fpn, (void*)dsp_cmd.intercepts_and_slopes_addr,
		IMS_FPN_REG))
		return -EFAULT;

	dsp_cmd.cmd_code = FIXED_PATTERN_NOISE_CORRECTION;
	dsp_cmd.fpn_pixels_addr = VIRT_TO_DSP(bpc);
	dsp_cmd.intercepts_and_slopes_addr = VIRT_TO_DSP(fpn);
	dsp_cmd.intercept_shift = 3;
	dsp_cmd.row_gain_enable = 0;
 	dsp_cmd.row_gain_addr = 0;
	dsp_cmd.column_gain_enable = 0;//mw_cmd.column_gain_enable;
 	dsp_cmd.column_gain_addr = 0;//VIRT_TO_DSP(column_offset);

	clean_d_cache(bpc, dsp_cmd.fpn_pixels_buf_size);
	clean_d_cache(fpn, IMS_FPN_REG);

	dsp_issue_img_cmd(&dsp_cmd, sizeof(dsp_cmd));

	return 0;
}

static int dsp_luma_sharp_lnl (struct iav_imgproc_info *context,
	luma_sharpening_LNL_t *param)
{
	u8 * addr = G_img.addr[IMB_LNL_TONE_CURVE];
	luma_sharpening_LNL_t dsp_cmd;

	if (!is_in_3a_work_state(context))
		return -EPERM;

	if (copy_from_user(&dsp_cmd, param, sizeof(dsp_cmd)))
		return -EFAULT;
	if (copy_from_user(addr, (void*)param->tone_curve_addr,
		IMS_LNL_TONE_CURVE))
		return -EFAULT;

	dsp_cmd.cmd_code = LUMA_SHARPENING_LNL;
	dsp_cmd.tone_curve_addr = VIRT_TO_DSP(addr);

	clean_d_cache(addr, IMS_LNL_TONE_CURVE);

	dsp_issue_img_cmd(&dsp_cmd, sizeof(dsp_cmd));

	return 0;
}

static int dsp_aaa_floating_statistics_setup (struct iav_imgproc_info *context,
	aaa_floating_tile_config_t __user * param)
{
	u8 * addr = G_img.addr[IMB_FLOAT_TILE_CONFIG];
	aaa_floating_tile_config_t dsp_cmd;

	if (!is_in_3a_work_state(context))
		return -EPERM;

	if (copy_from_user(&dsp_cmd, param, sizeof(dsp_cmd)))
		return -EFAULT;
	if (copy_from_user(addr,
		(void*)param->floating_tile_config_addr, IMS_FLOAT_TILE_CONFIG))
		return -EFAULT;

	dsp_cmd.cmd_code = AAA_FLOATING_TILE_CONFIG;
	dsp_cmd.floating_tile_config_addr = VIRT_TO_DSP(addr);

	clean_d_cache(addr, IMS_FLOAT_TILE_CONFIG);

	dsp_issue_img_cmd(&dsp_cmd, sizeof(dsp_cmd));

	return 0;
}

static int dsp_hdr_video_proc(struct iav_imgproc_info* context,
	ipcam_set_hdr_proc_control_t __user* param)
{
	ipcam_set_hdr_proc_control_t dsp_cmd;

	if (!is_in_3a_work_state(context))
		return -EPERM;

	if (copy_from_user(&dsp_cmd, param, sizeof(dsp_cmd)))
		return -EFAULT;

	dsp_cmd.cmd_code = IPCAM_SET_HDR_PROC_CONTROL;

	dsp_issue_img_cmd(&dsp_cmd, sizeof(dsp_cmd));

	return 0;
}

static int dsp_hdr_set_buf_id(struct iav_imgproc_info* context, u8 __user* param)
{
	if (!is_in_3a_work_state(context))
		return -EPERM;

	if(get_user(G_img.hdr_buf_id, param)){
		return -EFAULT;
	}

	return 0;
}

static int dsp_hdr_get_buf_id(struct iav_imgproc_info* context, u8 __user* param)
{
	if (!is_in_3a_work_state(context))
		return -EPERM;

	if(put_user(G_img.hdr_buf_id, param)){
		return -EFAULT;
	}

	return 0;
}

static int img_check_dsp_mode(struct iav_imgproc_info *context)
{
	int rval = 0;
	mutex_unlock(context->iav_mutex);
	rval = wait_event_interruptible_timeout(G_img.dsp_wq,
		(G_img.dsp_enc_mode == VIDEO_MODE),
		TWO_JIFFIES);
	mutex_lock(context->iav_mutex);
	return (rval <= 0) ? -EINTR : 0;
}

static int img_dump_idsp(struct iav_imgproc_info* context,
	idsp_config_info_t __user * param)
{
	idsp_config_info_t mw_cmd;
	dump_idsp_config_t dsp_cmd;
	int output_id = 0;
	int rval = -1;

	u32 *p_sec_size;
	u8* dump_buffer = kzalloc(MAX_DUMP_BUFFER_SIZE, GFP_KERNEL);

	if (dump_buffer == NULL) {
		printk("get null dump_buffer\n");
		return -1;
	}

	if (copy_from_user(&mw_cmd, param, sizeof(idsp_config_info_t)))
		return -EFAULT;

	memset(&dsp_cmd, 0, sizeof(dump_idsp_config_t));
	output_id = mw_cmd.id_section;
	*((int *)dump_buffer) = 0xdeadbeef;
	clean_d_cache(dump_buffer, MAX_DUMP_BUFFER_SIZE);

	dsp_cmd.cmd_code = DUMP_IDSP_CONFIG;
	dsp_cmd.mode = mw_cmd.id_section;
	dsp_cmd.dram_addr = VIRT_TO_DSP(dump_buffer);
	dsp_cmd.dram_size = MAX_DUMP_BUFFER_SIZE;

	switch(output_id)
	{
		case DUMP_IDSP_0:
		case DUMP_IDSP_1:
		case DUMP_IDSP_2:
		case DUMP_IDSP_3:
		case DUMP_IDSP_4:
		case DUMP_IDSP_5:
		case DUMP_IDSP_6:
		case DUMP_IDSP_7:
		case DUMP_IDSP_100:

			dsp_issue_img_cmd(&dsp_cmd, sizeof(dsp_cmd));

			msleep(3000);
			invalidate_d_cache(dump_buffer, MAX_DUMP_BUFFER_SIZE);
			if (*((int *)dump_buffer) == 0xdeadbeef){
				printk("DSP is halted! Dumping post mortem section 0 debug data\n");
				kfree(dump_buffer);
				return -EFAULT;
			}else{
				p_sec_size = (u32*)(dump_buffer + 8);
				dsp_cmd.dram_size = ((u32)*p_sec_size) + 64; //64 for header,
				printk("sec_size %d %d\n", *p_sec_size, dsp_cmd.dram_size);
			}
			param->addr_long = dsp_cmd.dram_size;

			if(copy_to_user(param->addr,dump_buffer,dsp_cmd.dram_size)){
				printk("cpy to usr err\n");
				kfree(dump_buffer);
				return -EFAULT;
			}
			rval=0;
			break;
		default:
			printk("Unknown output id %d\n", output_id);
			rval = -1;
			break;
	}

	kfree(dump_buffer);
	return rval;
}


static int dsp_set_liso_cfg_ni(struct iav_imgproc_info * context, u8 __user* arg)
{
	u8* cfg_addr = (u8*)context->img_virt + context->img_config_offset;

	if (copy_from_user(cfg_addr, arg, LISO_CFG_DATA_SIZE))
		return -EFAULT;

	return 0;
}

static int dsp_update_liso_cfg_ni(struct iav_imgproc_info * context,
	struct idsp_hiso_config_update_s __user * arg)
{
	static u8 buf_id = 0;
	u32 offset;
	idsp_hiso_config_update_t param;
	video_hiso_config_update_t dsp_cmd;

	if (copy_from_user(&param, arg, sizeof(param)))
		return -EFAULT;

	offset = context->img_config_offset + (buf_id + 1) * LISO_CFG_DATA_SIZE;
	if (copy_from_user((u8 *)context->img_virt + offset,
		(void *)param.hiso_param_daddr, LISO_CFG_DATA_SIZE))
		return -EFAULT;

	memset(&dsp_cmd, 0, sizeof(video_hiso_config_update_t));
	dsp_cmd.cmd_code = VIDEO_HISO_CONFIG_UPDATE;
	dsp_cmd.loadcfg_type.word = param.loadcfg_type.word;
	dsp_cmd.hiso_param_daddr = PHYS_TO_DSP(context->img_phys + offset);
	buf_id ^= 1;

	if (is_in_3a_work_state(context)) {
		dsp_issue_img_cmd(&dsp_cmd, sizeof(dsp_cmd));
		if (G_img.save_cmd) {
			memcpy(&G_img_cmds[IMG_CMD_VIDEO_HISO_CONFIG_UPDATE], &dsp_cmd,
				sizeof(DSP_CMD));
		}
	}

	return 0;
}

static int img_reset_raw2enc(struct iav_imgproc_info *context)
{
#ifdef CONFIG_IMGPROC_MEM_LARGE
	G_img.prev_addr = 0;
	G_img.proc_count = 0;
	G_img.raw_batch_num = 0;
	INIT_COMPLETION(g_raw2yuv_comp);
#endif
	return 0;
}

static int img_wait_raw2enc(struct iav_imgproc_info *context)
{
#ifdef CONFIG_IMGPROC_MEM_LARGE
	mutex_unlock(context->iav_mutex);
	if (wait_for_completion_interruptible(&g_raw2yuv_comp)) {
		mutex_lock(context->iav_mutex);
		iav_error("Failed to wait for raw2yuv completion!\n");
		return -EINTR;
	}
	mutex_lock(context->iav_mutex);
#endif
	return 0;
}

static int img_feed_raw(struct iav_imgproc_info *context,
	struct raw2enc_raw_feed_info __user *param)
{
#ifdef CONFIG_IMGPROC_MEM_LARGE
	raw_encode_video_setup_cmd_t dsp_cmd;
	struct raw2enc_raw_feed_info mw_cmd;

	if (copy_from_user(&mw_cmd, param, sizeof(mw_cmd)))
		return -EFAULT;

	memset(&dsp_cmd, 0, sizeof(dsp_cmd));
	dsp_cmd.cmd_code = RAW_ENCODE_VIDEO_SETUP;
	dsp_cmd.sensor_raw_start_daddr = PHYS_TO_DSP(context->img_phys +
		mw_cmd.daddr_offset);
	dsp_cmd.daddr_offset = mw_cmd.raw_size;
	dsp_cmd.dpitch = mw_cmd.dpitch;
	dsp_cmd.num_frames = mw_cmd.num_frames;

	G_img.raw_batch_num = mw_cmd.num_frames;

	if (is_in_3a_work_state(context)) {
		dsp_issue_img_cmd(&dsp_cmd, sizeof(dsp_cmd));
	}
#endif
	return 0;
}

int amba_imgproc_cmd(struct iav_imgproc_info *info, unsigned int cmd, unsigned long arg)
{
	int rval;

	switch (cmd) {
	case IAV_IOC_IMG_GET_STATISTICS:
		rval = img_get_statistics(info, (struct img_statistics __user*)arg);
		break;
	case IAV_IOC_IMG_CONFIG_STATISTICS:
		rval = dsp_aaa_statistics_setup(info, (aaa_statistics_setup_t __user *)arg);
		break;
	case IAV_IOC_IMG_CONFIG_HISTOGRAM:
		rval = dsp_aaa_histogram_setup(info, (aaa_histogram_t __user *)arg);
		break;
	case IAV_IOC_IMG_CONFIG_HDR_STATISTICS:
		rval = dsp_cfa_pre_hdr_statistics_setup(info, (vin_stats_setup_t __user *)arg);
		break;
	case IAV_IOC_IMG_AAA_SETUP_EX:
		rval = dsp_aaa_statistics_af_setup(info, (struct aaa_statistics_ex __user*)arg);
		break;
	case IAV_IOC_IMG_NOISE_FILTER_SETUP:
		rval = dsp_noise_filter_setup(info, (u32)arg);
		break;
	case IAV_IOC_IMG_BLACK_LEVEL_GLOBAL_OFFSET:
		rval = dsp_black_level_global_offset(info, (black_level_global_offset_t __user*)arg);
		break;
	case IAV_IOC_IMG_BAD_PIXEL_CORRECTION:
		rval = dsp_bad_pixel_correct_setup(info, (bad_pixel_correct_setup_t __user*)arg);
		break;
	case IAV_IOC_IMG_CFA_LEAKAGE_FILTER_SETUP:
		rval = dsp_cfa_domain_leakage_filter_setup(info, (cfa_domain_leakage_filter_t __user*)arg);
		break;
	case IAV_IOC_IMG_CFA_NOISE_FILTER_SETUP:
		rval = dsp_cfa_noise_filter(info, (cfa_noise_filter_t __user*)arg);
		break;
	case IAV_IOC_IMG_VIGNETTE_COMPENSATION:
		rval = dsp_vignette_compensation(info, (vignette_compensation_t __user*)arg);
		break;
	case IAV_IOC_IMG_LOCAL_EXPOSURE:
		rval = dsp_local_exposure(info, (local_exposure_t __user*)arg);
		break;
	case IAV_IOC_IMG_COLOR_CORRECTION:
		rval = dsp_color_correction(info, (color_correction_t __user*)arg);
		break;
	case IAV_IOC_IMG_RGB_TO_YUV_SETUP:
		rval = dsp_rgb_to_yuv_setup(info, (rgb_to_yuv_setup_t __user*)arg);
		break;
	case IAV_IOC_IMG_CHROMA_SCALE:
		rval = dsp_chroma_scale(info, (chroma_scale_t __user*)arg);
		break;
	case IAV_IOC_IMG_CHROMA_MEDIAN_FILTER_SETUP:
		rval = dsp_chroma_median_filter(info, (chroma_median_filter_info_t __user*)arg);
		break;
	case IAV_IOC_IMG_LUMA_SHARPENING:
		rval = dsp_luma_sharpening(info, (luma_sharpening_t __user*)arg);
		break;
	case IAV_IOC_IMG_LUMA_SHARPENING_FIR_CONFIG:
		rval = dsp_luma_sharp_fir(info, (luma_sharpening_FIR_config_t __user *)arg);
		break;
	case IAV_IOC_IMG_LUMA_SHARPENING_BLEND_CONFIG:
		rval = dsp_luma_sharpen_blend_ctrl(info, (luma_sharpening_blend_control_t __user*)arg);
		break;
	case IAV_IOC_IMG_LUMA_SHARPENING_LEVEL_CONTROL:
		rval = dsp_luma_sharpen_level_ctrl(info, (luma_sharpening_level_control_t __user*)arg);
		break;
	case IAV_IOC_IMG_MCTF_MV_STABILIZER_SETUP:
		rval = dsp_mctf_mv_stabilizer_setup(info, (mctf_mv_stab_setup_t __user*)arg);
		break;
	case IAV_IOC_IMG_RGB_GAIN_ADJUST:
		rval = dsp_rgb_gain_adjustment(info, (rgb_gain_adjust_t __user*)arg);
		break;
	case IAV_IOC_IMG_ANTI_ALIASING_CONFIG:
		rval = dsp_anti_aliasing_filter(info, (anti_aliasing_filter_t __user*)arg);
		break;
	case IAV_IOC_IMG_DIGITAL_SATURATION_LEVEL:
		rval = dsp_digital_gain_sat_level(info, (digital_gain_level_t __user *)arg);
		break;
	case IAV_IOC_IMG_DEMOSAIC_CONIFG:
		rval = dsp_demoasic_filter(info, (demoasic_filter_t __user *)arg);
		break;
	case IAV_IOC_IMG_SENSOR_CONFIG:
		rval = dsp_sensor_input_setup(info, (sensor_input_setup_t __user *)arg);
		break;
	case IAV_IOC_IMG_STATIC_BAD_PIXEL_CORRECTION:
		rval = dsp_fixed_pattern_noise_correct(info, (fixed_pattern_noise_correct_t __user *)arg);
		break;
	case IAV_IOC_IMG_CONFIG_FLOAT_STATISTICS:
		rval = dsp_aaa_floating_statistics_setup(info, (aaa_floating_tile_config_t __user *) arg);
		break;
	case IAV_IOC_IMG_CHROMA_NOISE_FILTER_SETUP:
		rval = dsp_chroma_noise_filter(info, (chroma_noise_filter_t __user*) arg);
		break;
	case IAV_IOC_IMG_LUMA_SHARPENING_LNL:
		rval = dsp_luma_sharp_lnl(info, (luma_sharpening_LNL_t __user *)arg);
		break;
	case IAV_IOC_IMG_STRONG_GRGB_MISMATCH:
		rval = dsp_strong_grgb_mismatch_filter(info,(strong_grgb_mismatch_filter_t __user *)arg);
		break;
	case IAV_IOC_IMG_WAIT_ENTER_PREVIEW:
		rval = img_check_dsp_mode(info);
		break;
	case IAV_IOC_IMG_LUMA_SHARPENING_TONE:
		rval = dsp_luma_sharpen_tone(info, (luma_sharpening_tone_control_t __user*)arg);
		break;
	case IAV_IOC_IMG_DUMP_IDSP_SEC:
		rval = img_dump_idsp(info,(idsp_config_info_t __user*)arg);
		break;
	case IAV_IOC_IMG_HDR_SET_VIDEO_PROC:
		rval = dsp_hdr_video_proc(info, (ipcam_set_hdr_proc_control_t __user*)arg);
		break;
	case IAV_IOC_IMG_HDR_SET_BUF_ID:
		rval = dsp_hdr_set_buf_id(info, (u8 __user*)arg);
		break;
	case IAV_IOC_IMG_HDR_GET_BUF_ID:
		rval = dsp_hdr_get_buf_id(info, (u8 __user*)arg);
		break;
	case IAV_IOC_IMG_SET_LISO_CFG_NI:
		rval = dsp_set_liso_cfg_ni(info, (u8 __user *)arg);
		break;
	case IAV_IOC_IMG_UPDATE_LISO_CFG_NI:
		rval = dsp_update_liso_cfg_ni(info,
			(struct idsp_hiso_config_update_s __user *)arg);
		break;
	case IAV_IOC_IMG_RESET_RAW2ENC_NI:
		rval = img_reset_raw2enc(info);
		break;
	case IAV_IOC_IMG_WAIT_RAW2ENC_NI:
		rval = img_wait_raw2enc(info);
		break;
	case IAV_IOC_IMG_FEED_RAW_NI:
		rval = img_feed_raw(info,(struct raw2enc_raw_feed_info __user *)arg);
		break;
	default:
		rval = -ENOIOCTLCMD;
		break;
	}

	if (rval == 0) {
		if (info->hdr_expo_num >= MAX_EXPOSURE_NUM) {
			iav_error("Too many exposure number [%d]. Need to increase the "
				"internal PTR number.\n", info->hdr_expo_num);
		}
		G_img.expo_num_minus_1 = info->hdr_expo_num - 1;
	}

	return rval;
}
EXPORT_SYMBOL(amba_imgproc_cmd);

//for raw2enc mode
#ifdef CONFIG_PM
int amba_imgproc_suspend(void)
{
	return 0;
}
EXPORT_SYMBOL(amba_imgproc_suspend);
int amba_imgproc_resume(int enc_mode, int expo_num)
{
	int i;
	video_hiso_config_update_t *update_cmd;

	/* Clear imgproc driver status */
	G_img.cfa_next = 0;
	G_img.rgb_next = 0;
	G_img.hdr_next = 0;
	for (i = 0; i < MAX_SLICE_FOR_FRAME; ++i) {
		G_img.rgb_entry[i] = INVALID_AAA_DATA_ENTRY;
		G_img.cfa_entry[i] = INVALID_AAA_DATA_ENTRY;
	}
	for (i = 0; i < MAX_HDR_SLICE_FOR_FRAME; ++i) {
		G_img.hdr_entry[i] = INVALID_AAA_DATA_ENTRY;
	}
	G_img.statis_3a_cnt = 0;

	G_img.raw_hist_addr = 0;
	G_img.raw_pitch = 0;
	G_img.dsp_pts = 0;
	G_img.mono_pts = 0;

	/* Issue necessary IMG dsp commands*/
	dsp_issue_img_cmd(&G_img_cmds[IMG_CMD_AAA_STATISTICS_SETUP],
		sizeof(DSP_CMD));
	dsp_issue_img_cmd(&G_img_cmds[IMG_CMD_AAA_HISTORGRAM_SETUP],
		sizeof(DSP_CMD));
	if (expo_num != 1) {
		dsp_issue_cmd(&G_img_cmds[IMG_CMD_VIN_STATISTICS_SETUP],
			sizeof(DSP_CMD));
	}
	switch(enc_mode) {
	case DSP_NORMAL_ISO_MODE:
		dsp_issue_img_cmd(&G_img_cmds[IMG_CMD_VIGNETTE_COMPENSATION],
			sizeof(DSP_CMD));
		dsp_issue_img_cmd(&G_img_cmds[IMG_CMD_RGB_GAIN_ADJUSTMENT],
			sizeof(DSP_CMD));
		dsp_issue_img_cmd(&G_img_cmds[IMG_CMD_DIGITAL_GAIN_SATURATION_LEVEL],
			sizeof(DSP_CMD));
		dsp_issue_img_cmd(&G_img_cmds[IMG_CMD_LOCAL_EXPOSURE],
			sizeof(DSP_CMD));
		dsp_issue_img_cmd(&G_img_cmds[IMG_CMD_BLACK_LEVEL_GLOBAL_OFFSET],
			sizeof(DSP_CMD));
		break;
	case DSP_ADVANCED_ISO_MODE:
		update_cmd = (video_hiso_config_update_t*)
			&G_img_cmds[IMG_CMD_VIDEO_HISO_CONFIG_UPDATE];
		update_cmd->loadcfg_type.flag.hiso_config_aaa_update = 1;
		dsp_issue_img_cmd(&G_img_cmds[IMG_CMD_VIDEO_HISO_CONFIG_UPDATE],
			sizeof(DSP_CMD));
		dsp_issue_img_cmd(&G_img_cmds[IMG_CMD_AAA_STATISTICS_SETUP1],
			sizeof(DSP_CMD));
		dsp_issue_img_cmd(&G_img_cmds[IMG_CMD_AAA_STATISTICS_SETUP2],
			sizeof(DSP_CMD));
		dsp_issue_img_cmd(&G_img_cmds[IMG_CMD_VIDEO_HISO_CONFIG_UPDATE],
			sizeof(DSP_CMD));
		break;
	case DSP_HDR_LINE_INTERLEAVED_MODE:
		update_cmd = (video_hiso_config_update_t*)
			&G_img_cmds[IMG_CMD_VIDEO_HISO_CONFIG_UPDATE];
		update_cmd->loadcfg_type.flag.hiso_config_aaa_update = 1;
		dsp_issue_img_cmd(&G_img_cmds[IMG_CMD_VIDEO_HISO_CONFIG_UPDATE],
			sizeof(DSP_CMD));
		break;
	case DSP_MULTI_REGION_WARP_MODE:
	case DSP_BLEND_ISO_MODE:
	default:
		break;
	}

	return 0;
}
EXPORT_SYMBOL(amba_imgproc_resume);
#endif

static int img_count_proc_num(u32 addr)
{
#ifdef CONFIG_IMGPROC_MEM_LARGE
//	printk("addr 0x%x 0x%x cnt %d\n", addr, prev_addr, G_img.raw_batch_num);
	if (addr != G_img.prev_addr) {
		G_img.proc_count++;
		G_img.prev_addr = addr;
	}
	if (G_img.proc_count >= G_img.raw_batch_num && G_img.raw_batch_num) {
		G_img.proc_count = 0;
		img_printk("process done\n");
		complete(&g_raw2yuv_comp);
	}
#endif
	return 0;
}

int amba_imgproc_msg(encode_msg_t *msg, u64 mono_pts)
{
//#ifdef BUILD_AMBARELLA_IMGPROC_DRV
//	img_printk("stat %p %p\n", (void*)msg->aaa_data_fifo_next, (void*)msg->yuv_aaa_data_fifo_next);
	G_img.dsp_enc_mode = msg->encode_operation_mode;//dsp_operation_mode
	wake_up_interruptible(&G_img.dsp_wq);
	irq_save_rgb_cfa_ptr((void *)msg->yuv_aaa_data_fifo_next,
		(void *)msg->aaa_data_fifo_next);
	irq_save_hdr_ptr((void *)msg->vin_main_data_fifo_next,
		(void *)msg->vin_hdr_data_fifo_next);
	irq_save_hist_ptr(msg->raw_pic_addr, msg->raw_pic_pitch);

	img_count_proc_num(msg->raw_pic_addr);
//#endif

	/* Add dsp_pts and mono_pts in 3A statistics */
	G_img.dsp_pts = msg->sw_pts;
	G_img.mono_pts = mono_pts;

	/* For RGB input, S2L output CFA and RGB statistics together.
	 * For YUV input, S2L output RGB statictics only */
	spin_lock(&G_img.lock);
	++G_img.statis_3a_cnt;
	spin_unlock(&G_img.lock);
	wake_up_interruptible_all(&G_img.statis_wq);

	return 0;
}
EXPORT_SYMBOL(amba_imgproc_msg);

module_init(img_init);
module_exit(img_exit);

