/*
 * kernel/private/drivers/ambarella/vout/dve/amb_tve/ambtve.c
 *
 * History:
 *    2009/05/14 - [Anthony Ginger] Create
 *
 * Copyright (C) 2004-2008, Ambarella, Inc.
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
/* ========================================================================== */
struct ambtve_info {
        struct __amba_vout_video_sink			video_sink;
        struct amba_video_source_scale_analog_info	ntsc_analog_info;
        struct amba_video_source_scale_analog_info	pal_analog_info;
};

/* ========================================================================== */
static const char ambtve_cvbs_name[] = "CVBS";

/* ========================================================================== */
static int ambtve_reset(struct __amba_vout_video_sink *psink, u32 *args)
{
        int     rval = 0;

        if (psink->state != AMBA_VOUT_SINK_STATE_IDLE) {
                psink->pstate = psink->state;
                psink->state = AMBA_VOUT_SINK_STATE_IDLE;
        }

        return rval;
}

static int ambtve_suspend(struct __amba_vout_video_sink *psink, u32 *args)
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

static int ambtve_resume(struct __amba_vout_video_sink *psink, u32 *args)
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
static int ambtve_set_video_mode(struct __amba_vout_video_sink *psink,
                                 struct amba_video_sink_mode *sink_mode)
{
        int                     rval = 0;
        struct ambtve_info      *pinfo;

        pinfo = (struct ambtve_info *)psink->pinfo;

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
                                          AMBA_VIDEO_SOURCE_INIT_ANALOG,
                                          sink_mode);
        if (!rval)
                psink->state = AMBA_VOUT_SINK_STATE_RUNNING;

        amba_osd_on_vout_change(psink->source_id, sink_mode);

        return rval;
}

static int ambtve_docmd(struct __amba_vout_video_sink *psink,
                        enum amba_video_sink_cmd cmd, void *args)
{
        int     rval = 0;

        switch (cmd) {
        case AMBA_VIDEO_SINK_IDLE:
                break;

        case AMBA_VIDEO_SINK_RESET:
                rval = ambtve_reset(psink, args);
                break;

        case AMBA_VIDEO_SINK_SUSPEND:
                rval = ambtve_suspend(psink, args);
                break;

        case AMBA_VIDEO_SINK_RESUME:
                rval = ambtve_resume(psink, args);
                break;

        case AMBA_VIDEO_SINK_GET_SOURCE_ID:
                *(int *)args = psink->source_id;
                break;

        case AMBA_VIDEO_SINK_GET_INFO: {
                struct amba_vout_sink_info	*psink_info;

                psink_info = (struct amba_vout_sink_info *)args;
                psink_info->source_id = psink->source_id;
                psink_info->sink_type = psink->sink_type;
                memcpy(psink_info->name, psink->name, sizeof(psink_info->name));
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
                rval = ambtve_set_video_mode(psink,
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

static int ambtve_probe(struct device *dev)
{
        int				rval = 0;
        struct __amba_vout_video_sink	*psink;
        struct ambtve_info              *pambtve_cvbs_info;

        pambtve_cvbs_info = kzalloc(sizeof(struct ambtve_info), GFP_KERNEL);
        if (!pambtve_cvbs_info)
                return -ENOMEM;

        pambtve_cvbs_info->ntsc_analog_info.y_coeff = 0x000000b8;
        pambtve_cvbs_info->ntsc_analog_info.pb_coeff = 0x000004e0;
        pambtve_cvbs_info->ntsc_analog_info.pr_coeff = 0x00000438;
        pambtve_cvbs_info->pal_analog_info.y_coeff = 0x000000b4;
        pambtve_cvbs_info->pal_analog_info.pb_coeff = 0x000004d8;
        pambtve_cvbs_info->pal_analog_info.pr_coeff = 0x00000428;

        psink = &pambtve_cvbs_info->video_sink;
        psink->source_id = 1;
        psink->sink_type = AMBA_VOUT_SINK_TYPE_CVBS;
        strlcpy(psink->name, ambtve_cvbs_name, sizeof(psink->name));
        psink->state = AMBA_VOUT_SINK_STATE_IDLE;
        psink->hdmi_plug = AMBA_VOUT_SINK_REMOVED;
        psink->hdmi_native_mode = AMBA_VIDEO_MODE_AUTO;
        psink->owner = THIS_MODULE;
        psink->pinfo = pambtve_cvbs_info;
        psink->docmd = ambtve_docmd;
        rval = amba_vout_add_video_sink(psink);
        if (rval)
                goto ambtve_free_cvbs;

        dev_set_drvdata(dev, pambtve_cvbs_info);
        vout_notice("%s:%d@%d probed!\n", psink->name,
                    psink->id, psink->source_id);

        return 0;;

ambtve_free_cvbs:
        kfree(pambtve_cvbs_info);

        return rval;
}

static int ambtve_remove(struct device *dev)
{
        int                     rval = 0;
        struct ambtve_info      *pambtve_cvbs_info;

        pambtve_cvbs_info = dev_get_drvdata(dev);

        if (pambtve_cvbs_info) {
                rval = amba_vout_del_video_sink(
                               &pambtve_cvbs_info->video_sink);

                vout_notice("%s removed!\n",
                            pambtve_cvbs_info->video_sink.name);
                kfree(pambtve_cvbs_info);
        }

        return rval;
}

struct amb_driver ambtve = {
        .probe = ambtve_probe,
        .remove = ambtve_remove,
        .driver = {
                .name = "ambtve",
                .owner = THIS_MODULE,
        }
};

static int __init ambtve_init(void)
{
        return amb_register_driver(&ambtve);
}

static void __exit ambtve_exit(void)
{
        amb_unregister_driver(&ambtve);
}

module_init(ambtve_init);
module_exit(ambtve_exit);

MODULE_DESCRIPTION("Built-in NTSC/PAL Video Encoder");
MODULE_AUTHOR("Anthony Ginger, <hfjiang@ambarella.com>");
MODULE_LICENSE("Proprietary");

