/*******************************************************************************
 * lib_vproc.c
 *
 * History:
 *  2015/04/01 - [Zhaoyang Chen] created file
 *
 * Copyright (C) 2015-2019, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#include <basetypes.h>
#include "iav_ioctl.h"

#include "lib_vproc.h"
#include "priv.h"

extern version_t G_mainpp_version;
vproc_param_t vproc;

static void init_vp_param(void)
{
	static int inited = 0;
	int i;
	if (unlikely(!inited)) {
		memset(&vproc, 0, sizeof(vproc));

		vproc.pm = &vproc.ioctl_pm.arg.pm;
		vproc.cawarp = &vproc.ioctl_cawarp.arg.cawarp;

		for (i = IAV_SRCBUF_FIRST; i < IAV_SRCBUF_LAST; i++) {
			vproc.dptz[i] = &vproc.ioctl_dptz[i].arg.dptz;
		}

		inited = 1;
		DEBUG("done\n");
	}
}

int vproc_pm(pm_param_t* pm_params)
{

	init_vp_param();

	int ret = -1;

	switch (get_pm_type()) {
		case PM_MB_MAIN:
			ret = operate_mbmain_pm(pm_params);
			break;
		case PM_PIXEL:
			ret = operate_pixel_pm(pm_params);
			break;
		default:
			ERROR("Privacy Mask is not supported.\n");
			break;
	}

	if ((pm_params->op != VPROC_OP_SHOW) && (ret == 0)) {
		ret = ioctl_apply();
	}

	return ret;
}

int vproc_pm_color(yuv_color_t* mask_color)
{
	init_vp_param();

	int ret = -1;

	switch (get_pm_type()) {
		case PM_MB_MAIN:
			ret = operate_mbmain_color(mask_color);
			break;
		default:
			ERROR("Privacy Mask color is not supported in current encode "
				"mode.\n");
			break;
	}

	return ret ? -1 : ioctl_apply();
}

int vproc_dptz(struct iav_digital_zoom* dptz)
{
	init_vp_param();

	pm_param_t pm_params;
	int ret = -1;

	switch(get_pm_type()) {
		case PM_MB_MAIN:
			if (dptz->buf_id == IAV_SRCBUF_MN) {
				pm_params.op = VPROC_OP_UPDATE;
				ret = (operate_dptz(dptz)
					|| operate_mbmain_pm(&pm_params)
					|| operate_cawarp(NULL));
			} else {
				ret = operate_dptz(dptz);
			}
			break;
		case PM_PIXEL:
			ret = (operate_dptz(dptz)
			    || (dptz->buf_id == IAV_SRCBUF_MN
			        && operate_cawarp(NULL )));
			break;
		default:
			ERROR("DPTZ is not supported.\n");
			break;
	}
	return ret ? -1 : ioctl_apply();
}

int vproc_cawarp(cawarp_param_t* cawarp)
{
	init_vp_param();

	int ret = operate_cawarp(cawarp);
	return ret ? -1 : ioctl_apply();
}

int vproc_pmc(pmc_param_t* pmc_params)
{
	init_vp_param();

	int ret = -1;

	switch (get_pm_type()) {
		case PM_PIXEL:
			ret = operate_pixel_pmc(pmc_params);
			break;
		case PM_MB_MAIN:
		/* fall through */
		default:
			ERROR("Circle PM is only supported for pixel type case.\n");
			break;
	}

	if ((pmc_params->op != VPROC_OP_SHOW) && (ret == 0)) {
		ret = ioctl_apply();
	}

	return ret;
}

int vproc_exit(void)
{
	init_vp_param();

	int ret = -1;

	switch (get_pm_type()) {
		case PM_MB_MAIN:
			ret = disable_mbmain_pm();
			break;
		case PM_PIXEL:
			ret = disable_pixel_pm();
			break;
		default:
			ERROR("Unknown type\n");
			break;
	}
	if (!ret) {
		ret = (ioctl_apply() || deinit_mbmain() || deinit_pixel()
			|| deinit_cawarp() || deinit_dptz());
	}
	return ret ? -1 : 0;
}

int vproc_version(version_t* version)
{
	init_vp_param();

	memcpy(version, &G_mainpp_version, sizeof(version_t));
	return 0;
}
