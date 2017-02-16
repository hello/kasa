/*
 * iav_enc_pm.c
 *
 * History:
 *	2014/03/13 - [Zhikan Yang] created file
 * Copyright (C) 2007-2016, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */
#include <config.h>
#include <linux/random.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <iav_utils.h>
#include <iav_ioctl.h>
#include <dsp_api.h>
#include <dsp_format.h>
#include <vin_api.h>
#include <vout_api.h>
#include "iav.h"
#include "iav_dsp_cmd.h"
#include "iav_vin.h"
#include "iav_vout.h"
#include "iav_enc_api.h"
#include "iav_enc_utils.h"


static inline u32 get_pm_mctf_pitch(u32 width)
{
	u32 width_mb;
	struct iav_mctf_filter mcft;

	width_mb = ALIGN(width, PIXEL_IN_MB) / PIXEL_IN_MB;

	return ALIGN(width_mb * sizeof(mcft), 32);
}

static inline u32 get_pm_mctf_height(u32 height)
{
	return ALIGN(height, PIXEL_IN_MB) / PIXEL_IN_MB;
}

static inline u32 get_pm_bpc_pitch(u32 width)
{
	u32 width_bytes = ALIGN(width, 8) / 8;

	return ALIGN(width_bytes, 32);
}

static void iav_init_pm_bpc(struct ambarella_iav *iav)
{
	/* Clear BPC based PM and BPC memory for APP to use */
	memset((u8 *)iav->mmap[IAV_BUFFER_PM_BPC].virt, 0,
		PM_BPC_PARTITION_SIZE);
	memset((u8 *)iav->mmap[IAV_BUFFER_BPC].virt, 0,
		PAGE_SIZE + PM_BPC_PARTITION_SIZE);

	iav->pm_bpc_enable = 0;
	iav->bpc_enable = 0;
	iav->curr_pm_index = 0;
	iav->next_pm_index = 0;
}

static void iav_init_pm_mctf(struct ambarella_iav *iav)
{
	/* Clear MCTF based PM memory for APP to use */
	memset((u8 *)iav->mmap[IAV_BUFFER_PM_MCTF].virt, 0,
		PM_MCTF_TOTAL_SIZE);
}

static void reset_pm_bpc(struct ambarella_iav *iav)
{
	/* Clear current Privacy mask */
	memset((u8 *)(iav->mmap[IAV_BUFFER_PM_BPC].virt +
		PM_BPC_PARTITION_SIZE * iav->curr_pm_index),
		0, PM_BPC_PARTITION_SIZE);

	/* Clear User PM Partition */
	memset((u8 *)iav->mmap[IAV_BUFFER_PM_BPC].virt, 0, PM_BPC_PARTITION_SIZE);

	/* Clear BPC Partition */
	memset((u8 *)iav->mmap[IAV_BUFFER_BPC].virt, 0,
		PAGE_SIZE + PM_BPC_PARTITION_SIZE);

	iav->pm_bpc_enable = 0;
	iav->bpc_enable = 0;
}

static void reset_pm_mctf(struct ambarella_iav *iav)
{
	/* Clear Privacy mask memory */
	memset((u8 *)iav->mmap[IAV_BUFFER_PM_MCTF].virt, 0, PM_MCTF_TOTAL_SIZE);
}

void iav_init_pm(struct ambarella_iav *iav)
{
	iav_init_pm_bpc(iav);
	iav_init_pm_mctf(iav);

	memset(&iav->pm, 0, sizeof(iav->pm));
}

void iav_reset_pm(struct ambarella_iav *iav)
{
	u32 pm_unit;

	pm_unit = get_pm_unit(iav);
	if (pm_unit == IAV_PM_UNIT_PIXEL) {
		reset_pm_bpc(iav);
	} else {
		reset_pm_mctf(iav);
	}
}

int iav_ioc_g_pm_info(struct ambarella_iav *iav, void __user * arg)
{
	struct iav_privacy_mask_info pm_info;
	struct iav_window *main_win = NULL;
	struct iav_rect *vin_win = NULL;

	mutex_lock(&iav->iav_mutex);

	memset(&pm_info, 0, sizeof(pm_info));

	pm_info.unit = get_pm_unit(iav);
	pm_info.domain = get_pm_domain(iav);

	main_win = &iav->srcbuf[IAV_SRCBUF_MN].win;
	get_pm_vin_win(iav, &vin_win);
	switch (pm_info.unit) {
	case IAV_PM_UNIT_PIXEL:
		pm_info.buffer_pitch = get_pm_bpc_pitch(vin_win->width);
		pm_info.pixel_width = vin_win->width;
		break;
	case IAV_PM_UNIT_MB:
	default:
		pm_info.pixel_width = 0;
		pm_info.buffer_pitch = get_pm_mctf_pitch(main_win->width);
		break;
	}

	mutex_unlock(&iav->iav_mutex);

	return copy_to_user(arg, &pm_info, sizeof(pm_info)) ? -EFAULT : 0;
}

int iav_set_pm_bpc(struct ambarella_iav *iav,
	struct iav_privacy_mask *priv_mask)
{
	struct iav_rect *vin_win = NULL;
	int i, j, pitch;
	u32 *user_pm, *bpc, *dsp_pm;
	u32 width_in_u8, padding_in_u32;

	if (!is_enc_work_state(iav)) {
		iav_error("Cannot set privacymask while IAV is not in PREVIEW or ENCODE.\n");
		return -1;
	}

	if (get_pm_vin_win(iav, &vin_win) < 0) {
		return -EFAULT;
	}

	iav->pm_bpc_enable = 1;
	width_in_u8 = ALIGN(vin_win->width, 32) / 8;
	pitch = ALIGN(vin_win->width / 8, 32);
	user_pm = (u32 *)iav->mmap[IAV_BUFFER_PM_BPC].virt;
	bpc = (u32 *)(iav->mmap[IAV_BUFFER_BPC].virt + PAGE_SIZE);
	iav->curr_pm_index = iav->next_pm_index + UPMP_NUM;
	dsp_pm = (u32 *)(iav->mmap[IAV_BUFFER_PM_BPC].virt + iav->curr_pm_index *
		PM_BPC_PARTITION_SIZE);

	iav->pm.enable = priv_mask->enable;
	if (priv_mask->y || priv_mask->u || priv_mask->v){
		iav->pm.y = priv_mask->y;
		iav->pm.u = priv_mask->u;
		iav->pm.v = priv_mask->v;
		iav_debug("Color is not supported for BPC type Privacy Mask!\n");
	}

	if (priv_mask->enable) {
		//blend BPC and Privacy mask
		padding_in_u32 = (pitch - width_in_u8) >> 2;
		for (i = 0; i < vin_win->height; i++) {
			for (j = 0; j < width_in_u8; j += 4, dsp_pm++, user_pm++, bpc++) {
				*dsp_pm = (*user_pm) | (*bpc);
			}
			dsp_pm += padding_in_u32;
			user_pm += padding_in_u32;
			bpc += padding_in_u32;
		}
	} else {
		//use BPC only
		memcpy(dsp_pm, bpc, pitch * vin_win->height);
	}

	if (iav->bpc_enable) {
		cmd_bpc_setup(iav, NULL, NULL);
	}
	cmd_set_pm_bpc(iav, NULL);

	//update Privacy mask index
	iav->next_pm_index = (iav->next_pm_index + 1) % IAV_PARTITION_TOGGLE_NUM;

	return 0;
}

int iav_set_pm_mctf(struct ambarella_iav *iav,
	struct iav_privacy_mask *priv_mask, u32 cmd_delay)
{
	cmd_set_pm_mctf(iav, NULL, cmd_delay);
	return 0;
}

static int check_pm_mctf(struct ambarella_iav *iav,
	struct iav_privacy_mask *pm)
{
	u32 buf_size, buf_pitch, buf_height;

	iav_no_check();

	if (pm->enable) {
		buf_pitch = get_pm_mctf_pitch(iav->srcbuf[IAV_SRCBUF_MN].win.width);
		if (buf_pitch != pm->buf_pitch) {
			iav_error("Incorrect buffer pitch %u, should be %d!\n",
				pm->buf_pitch, buf_pitch);
			return -1;
		}
		buf_height = get_pm_mctf_height(iav->srcbuf[IAV_SRCBUF_MN].win.height);
		if (buf_height != pm->buf_height) {
			iav_error("Incorrect buffer height %u, should be %d!\n",
				pm->buf_height, buf_height);
			return -1;
		}
		buf_size = pm->buf_pitch * pm->buf_height;
		if (pm->data_addr_offset + buf_size > PM_MCTF_TOTAL_SIZE) {
			iav_error("PM MCTF data offset 0x%x with data size 0x%x is out of "
				"range 0x%x!", pm->data_addr_offset, buf_size,
				PM_MCTF_TOTAL_SIZE);
			return -1;
		}
	}

	return 0;
}

static int check_pm_bpc(struct ambarella_iav *iav,
	struct iav_privacy_mask *pm)
{
	iav_no_check();

	if (pm->enable) {
		if (pm->data_addr_offset > PM_BPC_PARTITION_SIZE) {
			iav_error("PM BPC data offset 0x%x is out of range 0x%x!",
				pm->data_addr_offset, PM_MCTF_TOTAL_SIZE);
			return -1;
		}
	}

	return 0;
}

int iav_cfg_vproc_pm(struct ambarella_iav *iav,
	struct iav_privacy_mask *pm)
{
	u32 pm_unit;

	pm_unit = get_pm_unit(iav);

	if (pm_unit == IAV_PM_UNIT_MB) {
		if (check_pm_mctf(iav, pm) < 0) {
			iav_error("Check MCTF Privacy Mask failed!\n");
			return -EINVAL;
		}
	} else {
		if (check_pm_bpc(iav, pm) < 0) {
			iav_error("Check BPC Privacy Mask failed!\n");
			return -EINVAL;
		}
	}

	iav->pm = *pm;
	return 0;
}

int iav_ioc_s_pm(struct ambarella_iav *iav, void __user * arg)
{
	u32 pm_unit;
	int rval = 0;
	struct iav_privacy_mask priv_mask;

	if (!is_enc_work_state(iav)) {
		iav_error("Privacy mask should be set in Preview/Encode state.\n");
		return -1;
	}

	if (copy_from_user(&priv_mask, arg, sizeof(priv_mask)))
		return -EFAULT;

	mutex_lock(&iav->iav_mutex);
	pm_unit = get_pm_unit(iav);

	if (pm_unit == IAV_PM_UNIT_PIXEL) {
		rval = iav_set_pm_bpc(iav, &priv_mask);
	} else {
		rval = iav_set_pm_mctf(iav, &priv_mask, 0);
	}
	mutex_unlock(&iav->iav_mutex);

	return rval;
}

int iav_ioc_g_pm(struct ambarella_iav *iav, void __user * arg)
{
	int ret = 0;

	mutex_lock(&iav->iav_mutex);
	ret = copy_to_user(arg, &iav->pm, sizeof(iav->pm));
	mutex_unlock(&iav->iav_mutex);

	return (ret ? -EFAULT : 0);
}

int iav_ioc_s_bpc_setup(struct ambarella_iav *iav, void __user * arg)
{
	u8 * addr = NULL;
	struct iav_bpc_setup bpc_setup;

	if (!is_enc_work_state(iav)) {
		iav_error("BPC should be setup in Preview/Encode state.\n");
		return -1;
	}

	if (copy_from_user(&bpc_setup, arg, sizeof(bpc_setup)))
		return -EFAULT;

	addr = (u8 *)iav->mmap[IAV_BUFFER_BPC].virt;
	if (copy_from_user((void *)addr,
		(void*)bpc_setup.hot_pixel_thresh_addr, BPC_TABLE_SIZE))
			return -EFAULT;
	if (copy_from_user((addr + BPC_TABLE_SIZE),
		(void*)bpc_setup.dark_pixel_thresh_addr, BPC_TABLE_SIZE))
			return -EFAULT;

	mutex_lock(&iav->iav_mutex);
	iav->bpc_enable = 1;
	cmd_bpc_setup(iav, NULL, &bpc_setup);
	mutex_unlock(&iav->iav_mutex);

	return 0;
}

int iav_ioc_s_static_bpc(struct ambarella_iav *iav, void __user * arg)
{
	struct iav_bpc_fpn bpc_fpn;
	struct iav_rect *vin_win = NULL;
	u8 * addr = NULL;
	int i, j, pitch;
	u32 *user_pm, *bpc, *dsp_pm;
	u32 width_in_u8, padding_in_u32;

	if (!is_enc_work_state(iav)) {
		iav_error("BPC should be set in Preview/Encode state.\n");
		return -1;
	}

	if (copy_from_user(&bpc_fpn, arg, sizeof(bpc_fpn)))
		return -EFAULT;

	addr = (u8 *)iav->mmap[IAV_BUFFER_BPC].virt;
	if (bpc_fpn.fpn_pixel_mode == 0) {
		memset(addr + PAGE_SIZE, 0, bpc_fpn.fpn_pixels_buf_size);
	} else {
		if (copy_from_user((addr + PAGE_SIZE), (void*)bpc_fpn.fpn_pixels_addr,
			bpc_fpn.fpn_pixels_buf_size))
				return -EFAULT;
	}
	if (copy_from_user((addr + BPC_TABLE_SIZE * 2),
		(void*)bpc_fpn.intercepts_and_slopes_addr, 1024))
				return -EFAULT;

	if (get_pm_vin_win(iav, &vin_win) < 0) {
		return -EFAULT;
	}

	mutex_lock(&iav->iav_mutex);
	iav->pm_bpc_enable = 1;
	width_in_u8 = ALIGN(vin_win->width, 32) / 8;
	pitch = ALIGN(vin_win->width / 8, 32);
	user_pm = (u32 *)iav->mmap[IAV_BUFFER_PM_BPC].virt;
	bpc = (u32 *)(iav->mmap[IAV_BUFFER_BPC].virt + PAGE_SIZE);
	iav->curr_pm_index = iav->next_pm_index + UPMP_NUM;
	dsp_pm = (u32 *)(iav->mmap[IAV_BUFFER_PM_BPC].virt +
		iav->curr_pm_index * PM_BPC_PARTITION_SIZE);

	if (bpc_fpn.fpn_pixel_mode) {
		/* blend BPC and Privacy mask */
		padding_in_u32 = (pitch - width_in_u8) >> 2;
		for (i = 0; i < vin_win->height; i++) {
			for (j = 0; j < width_in_u8; j += 4, dsp_pm++, user_pm++, bpc++) {
				*dsp_pm = (*user_pm) | (*bpc);
			}
			dsp_pm += padding_in_u32;
			user_pm += padding_in_u32;
			bpc += padding_in_u32;
		}
	} else {
		/* use Privacy Mask only */
		memcpy(dsp_pm, user_pm, pitch * vin_win->height);
	}

	if (iav->bpc_enable) {
		cmd_bpc_setup(iav, NULL, NULL);
	}
	cmd_static_bpc(iav, NULL, &bpc_fpn);

	//update Privacy mask index
	iav->next_pm_index = (iav->next_pm_index + 1) % IAV_PARTITION_TOGGLE_NUM;

	mutex_unlock(&iav->iav_mutex);

	return 0;
}

