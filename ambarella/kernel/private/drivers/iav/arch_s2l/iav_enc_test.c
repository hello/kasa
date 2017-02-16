/*
 * iav_enc_test.c
 *
 * History:
 *	2014/06/28 - [Jian Tang] created file
 *
 * Copyright (C) 2014-2018, Ambarella, Inc.
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
#include <linux/fs.h>
#include <linux/clk.h>
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

static struct iav_debug_cfg G_iav_debug[IAV_DEBUGCFG_NUM] = {
	[IAV_DEBUGCFG_CHECK] = {
		.cid = IAV_DEBUGCFG_CHECK,
	},
	[IAV_DEBUGCFG_MODULE] = {
		.cid = IAV_DEBUGCFG_MODULE,
	},
	[IAV_DEBUGCFG_AUDIT] = {
		.cid = IAV_DEBUGCFG_AUDIT,
	},
};

int is_iav_no_check(void)
{
	return G_iav_debug[IAV_DEBUGCFG_CHECK].arg.enable;
}

void iav_init_debug(struct ambarella_iav * iav)
{
	iav->debug = &G_iav_debug[0];
	memset(&G_iav_debug[0], 0, sizeof(G_iav_debug));
}

int iav_ioc_s_debug_cfg(struct ambarella_iav * iav, void __user * arg)
{
	struct iav_debug_cfg cfg;
	struct iav_debug_module *module = NULL;
	struct iav_debug_audit *audit = NULL;

	if (copy_from_user(&cfg, arg, sizeof(cfg)))
		return -EFAULT;

	switch (cfg.cid) {
	case IAV_DEBUGCFG_CHECK:
		G_iav_debug[IAV_DEBUGCFG_CHECK].arg.enable = cfg.arg.enable;
		break;
	case IAV_DEBUGCFG_MODULE:
		module = &G_iav_debug[cfg.cid].arg.module;
		if (cfg.arg.module.enable) {
			module->flags |= cfg.arg.module.flags;
		} else {
			module->flags &= ~cfg.arg.module.flags;
		}
		module->enable = cfg.arg.module.enable;
		break;
	case IAV_DEBUGCFG_AUDIT:
		audit = &G_iav_debug[cfg.cid].arg.audit;
		audit->id = cfg.arg.audit.id;
		audit->enable = cfg.arg.audit.enable;
		if (iav->dsp->set_audit) {
			iav->dsp->set_audit(iav->dsp, cfg.arg.audit.id, (u32)audit);
		}
		break;
	default:
		break;
	}

	return 0;
}

int iav_ioc_g_debug_cfg(struct ambarella_iav * iav, void __user * arg)
{
	struct iav_debug_cfg cfg;
	struct iav_debug_module * module = NULL;
	struct iav_debug_audit * audit = NULL;

	if (copy_from_user(&cfg, arg, sizeof(cfg)))
		return -EFAULT;

	switch (cfg.cid) {
	case IAV_DEBUGCFG_MODULE:
		module = module;
		break;
	case IAV_DEBUGCFG_AUDIT:
		if (cfg.arg.audit.id >= DAI_NUM) {
			iav_error("Invalid debug audit id [%d] from [0~%d].\n",
				cfg.id, (DAI_NUM - 1));
			return -EINVAL;
		}
		if (iav->dsp->get_audit) {
			audit = &G_iav_debug[cfg.cid].arg.audit;
			iav->dsp->get_audit(iav->dsp, cfg.arg.audit.id, (u32)audit);
			cfg.arg.audit = *audit;
		}
		break;
	default:
		break;
	}

	return copy_to_user(arg, &cfg, sizeof(cfg)) ? -EFAULT : 0;
}

