/*
 * kernel/private/drivers/ambarella/vout/digital/amb_dbus/ambdbus.c
 *
 * History:
 *    2009/05/21 - [Anthony Ginger] Create
 *
 * Copyright (C) 2004-2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/ambbus.h>
#include <linux/fb.h>
#include <plat/clk.h>

#include "vout_pri.h"

/* ========================================================================== */
struct ambdbus_info {
	struct __amba_vout_video_sink   video_sink;
};

static const char ambtve_digital_name[] = "DBus-All";
/* ========================================================================== */
static int ambdbus_reset(struct __amba_vout_video_sink *psink, u32 *args)
{
        int     rval = 0;

        if (psink->state != AMBA_VOUT_SINK_STATE_IDLE) {
                psink->pstate = psink->state;
                psink->state = AMBA_VOUT_SINK_STATE_IDLE;
        }

        return rval;
}

static int ambdbus_suspend(struct __amba_vout_video_sink *psink, u32 *args)
{
        int     rval = 0;

        if (psink->state != AMBA_VOUT_SINK_STATE_SUSPENDED) {
                psink->pstate = psink->state;
                psink->state = AMBA_VOUT_SINK_STATE_SUSPENDED;
        } else {
                rval = 1;
        }

        return rval;
}

static int ambdbus_resume(struct __amba_vout_video_sink *psink, u32 *args)
{
        int     rval = 0;

        if (psink->state == AMBA_VOUT_SINK_STATE_SUSPENDED) {
                psink->state = psink->pstate;
        } else {
                rval = 1;
        }

        return rval;
}

/* ========================================================================== */
static int ambdbus_set_video_mode(struct __amba_vout_video_sink *psink,
                                  struct amba_video_sink_mode *sink_mode)
{
        int                     rval = 0;
        struct ambdbus_info     *pinfo;

        pinfo = (struct ambdbus_info *)psink->pinfo;

        if (sink_mode->sink_type == AMBA_VOUT_SINK_TYPE_AUTO)
                sink_mode->sink_type = psink->sink_type;
        if (sink_mode->sink_type != psink->sink_type) {
                vout_err("%s-%d sink_type is %d, not %d!\n",
                         psink->name, psink->id,
                         psink->sink_type, sink_mode->sink_type);
                return -ENOIOCTLCMD;
        }

        vout_info("%s-%d switch to mode %d!\n",
                  psink->name, psink->id, sink_mode->mode);

        rval = amba_vout_video_source_cmd(psink->source_id,
                                          AMBA_VIDEO_SOURCE_INIT_DIGITAL,
                                          sink_mode);
        if (!rval)
                psink->state = AMBA_VOUT_SINK_STATE_RUNNING;

        amba_osd_on_vout_change(psink->source_id, sink_mode);

        return rval;
}

static int ambdbus_docmd(struct __amba_vout_video_sink *psink,
                         enum amba_video_sink_cmd cmd, void *args)
{
        int     rval = 0;

        switch (cmd) {
        case AMBA_VIDEO_SINK_IDLE:
                break;

        case AMBA_VIDEO_SINK_RESET:
                rval = ambdbus_reset(psink, args);
                break;

        case AMBA_VIDEO_SINK_SUSPEND:
                rval = ambdbus_suspend(psink, args);
                break;

        case AMBA_VIDEO_SINK_RESUME:
                rval = ambdbus_resume(psink, args);
                break;

        case AMBA_VIDEO_SINK_GET_SOURCE_ID:
                *(int *)args = psink->source_id;
                break;

        case AMBA_VIDEO_SINK_GET_INFO: {
                struct amba_vout_sink_info	*psink_info;

                psink_info = (struct amba_vout_sink_info *)args;
                psink_info->source_id = psink->source_id;
                psink_info->sink_type = psink->sink_type;
                memcpy(psink_info->name,
                        psink->name, sizeof(psink_info->name));
                psink_info->state = psink->state;
                psink_info->hdmi_plug = psink->hdmi_plug;
                memcpy(psink_info->hdmi_modes,
                        psink->hdmi_modes, sizeof(psink->hdmi_modes));
                psink_info->hdmi_native_mode = psink->hdmi_native_mode;
                psink_info->hdmi_overscan = psink->hdmi_overscan;
        }
        break;

        case AMBA_VIDEO_SINK_GET_MODE:
                rval = -ENOIOCTLCMD;
                break;

        case AMBA_VIDEO_SINK_SET_MODE:
                rval = ambdbus_set_video_mode(psink,
                        (struct amba_video_sink_mode *)args);
                break;

        default:
                vout_err("%s-%d do not support cmd %d!\n",
                         psink->name, psink->id, cmd);
                rval = -ENOIOCTLCMD;

                break;
        }

        return rval;
}

static int ambdbus_probe(struct device *dev)
{
	int				rval = 0;
	struct ambdbus_info		*pdbus_info;
	struct __amba_vout_video_sink	*psink;

	pdbus_info = kzalloc(sizeof(struct ambdbus_info), GFP_KERNEL);
	if (!pdbus_info)
		return -ENOMEM;

	psink = &pdbus_info->video_sink;
	psink->source_id = AMBA_VOUT_SOURCE_STARTING_ID;
	psink->sink_type = AMBA_VOUT_SINK_TYPE_DIGITAL;
	strlcpy(psink->name, ambtve_digital_name, sizeof(psink->name));
	psink->state = AMBA_VOUT_SINK_STATE_IDLE;
	psink->hdmi_plug = AMBA_VOUT_SINK_REMOVED;
	psink->hdmi_native_mode = AMBA_VIDEO_MODE_AUTO;
	psink->owner = THIS_MODULE;
	psink->pinfo = pdbus_info;
	psink->docmd = ambdbus_docmd;
	rval = amba_vout_add_video_sink(psink);
	if (rval)
		goto ambdbus_add_s2ldbus_free_pdbus_info;

        dev_set_drvdata(dev, pdbus_info);

	vout_notice("%s:%d@%d probed!\n", psink->name,
		psink->id, psink->source_id);

	return 0;

ambdbus_add_s2ldbus_free_pdbus_info:
	kfree(pdbus_info);

	return rval;
}

static int ambdbus_remove(struct device *dev)
{
	int                     rval = 0;
	struct ambdbus_info     *pdbus_info;

        pdbus_info = dev_get_drvdata(dev);
	if (pdbus_info) {
		rval = amba_vout_del_video_sink(&pdbus_info->video_sink);
		vout_notice("%s removed!\n", pdbus_info->video_sink.name);
		kfree(pdbus_info);
	}

	return rval;
}

struct amb_driver ambdbus = {
	.probe = ambdbus_probe,
	.remove = ambdbus_remove,
	.driver = {
		.name = "ambdbus",
		.owner = THIS_MODULE,
	}
};

static int __init ambdbus_init(void)
{
	return amb_register_driver(&ambdbus);
}

static void __exit ambdbus_exit(void)
{
	amb_unregister_driver(&ambdbus);
}

module_init(ambdbus_init);
module_exit(ambdbus_exit);

MODULE_DESCRIPTION("Built-in LCD Controller");
MODULE_AUTHOR("Anthony Ginger, <hfjiang@ambarella.com>");
MODULE_LICENSE("Proprietary");

