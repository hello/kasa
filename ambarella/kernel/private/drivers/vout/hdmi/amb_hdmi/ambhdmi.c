/*
 * kernel/private/drivers/ambarella/vout/hdmi/amb_hdmi/ambhdmi.c
 *
 * History:
 *    2009/06/02 - [Zhenwu Xue] Initial revision
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
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/vmalloc.h>
#include <linux/of_gpio.h>
#include <linux/kthread.h>
#include <plat/clk.h>
#include <plat/ambevent.h>
#include <plat/audio.h>
#include <plat/service.h>

#include "vout_pri.h"
#include "ambhdmi.h"
#include "ambhdmi_edid.h"

/* ========================================================================== */
#define AMBHDMI_EDID_PROC_NAME  "hdmi_edid"

struct ambhdmi_sink {
        struct device			*dev;
        u32		                regbase;
        u32				irq;
        wait_queue_head_t		irq_waitqueue;
        u32				irq_pending;
        struct task_struct		*kthread;
        u32				killing_kthread;
        struct i2c_adapter		*ddc_adapter;
        amba_hdmi_raw_edid_t		raw_edid;
        amba_hdmi_edid_t		edid;
        struct proc_dir_entry		*proc_entry;
        struct __amba_vout_video_sink	video_sink;
        struct notifier_block		audio_transition;
};

struct ambhdmi_instance_info {
        const char			name[AMBA_VOUT_NAME_LENGTH];
        const int			source_id;
        const u32			sink_type;
        const struct resource		io_mem;
        const struct resource		irq;
};

/* ========================================================================== */

static struct ambhdmi_instance_info hdmi_instance = {
        .name		= "HDMI",
        .source_id	= AMBA_VOUT_SOURCE_STARTING_ID + 1,
        .sink_type	= AMBA_VOUT_SINK_TYPE_HDMI,
        .io_mem		= {
                .start	= HDMI_BASE,
                .end	= HDMI_BASE + 0x1000 - 1,
                .flags	= IORESOURCE_MEM,
        },
        .irq		= {
                .start	= HDMI_IRQ,
                .end	= HDMI_IRQ,
                .flags	= IORESOURCE_IRQ,
        }
};
/* ========================================================================== */
#include "ambhdmi_edid.c"
#include "ambhdmi_hdmise.c"

/* ========================================================================== */
static void ambhdmi_unplug(struct __amba_vout_video_sink *pvideo_sink)
{
        int                     i;
        u32                     regbase;
        struct  ambhdmi_sink    *phdmi_sink;

        phdmi_sink = (struct ambhdmi_sink *)pvideo_sink->pinfo;
        regbase = phdmi_sink->regbase;
        pvideo_sink->hdmi_plug = AMBA_VOUT_SINK_REMOVED;

        for (i = 0; i < AMBA_HDMI_MODE_MAX; i++) {
                pvideo_sink->hdmi_modes[i] = AMBA_VIDEO_MODE_MAX;
        }
        pvideo_sink->hdmi_native_mode = AMBA_VIDEO_MODE_MAX;
        amba_writel(regbase + HDMI_CLOCK_GATED_OFFSET, 0);

        amba_vout_video_source_cmd(pvideo_sink->source_id,
                                   AMBA_VIDEO_SOURCE_REPORT_SINK_EVENT,
                                   (void *)AMB_EV_VOUT_HDMI_REMOVE);
        vout_notice("HDMI Device Removed!\n");
}

static void ambhdmi_plug(struct __amba_vout_video_sink *pvideo_sink)
{
        int                     i, j, rval;
        u32                     regbase, reg, val;
        struct ambhdmi_sink     *phdmi_sink;
        amba_hdmi_edid_t        *pedid;

        phdmi_sink      = (struct ambhdmi_sink *)pvideo_sink->pinfo;
        pedid           = &phdmi_sink->edid;
        regbase         = phdmi_sink->regbase;

        /* Enable CEC Clock */
        reg     = regbase + HDMI_CLOCK_GATED_OFFSET;
        val     = amba_readl(reg);
        val     |= (HDMI_CLOCK_GATED_CEC_CLOCK_EN |
                    HDMI_CLOCK_GATED_HDCP_CLOCK_EN);
        amba_writel(reg, val);

        /* Read and Parse EDID */
        rval = ambhdmi_edid_read_and_parse(phdmi_sink);
        if(rval) {
                vout_notice("No EDID found!\n");
                amba_writel(regbase + HDMI_CLOCK_GATED_OFFSET, 0);
                return;
        }

        vout_notice("HDMI Device Connected!\n");

        /* hdmi_modes[] */
        pvideo_sink->hdmi_plug = AMBA_VOUT_SINK_PLUGGED;
        for (i = 0, j = 0; i < pedid->native_timings.number; i++) {
                pvideo_sink->hdmi_modes[j++] =
                        pedid->native_timings.supported_native_timings[i].vmode;
        }
        for (i = 0; i < pedid->cea_timings.number; i++) {
                if (j >= AMBA_HDMI_MODE_MAX)
                        break;
                pvideo_sink->hdmi_modes[j++] =
                        CEA_Timings[pedid->cea_timings.supported_cea_timings[i]].vmode;
        }

        /* hdmi_native_mode */
        if (pedid->native_timings.number) {
                pedid->native_timings.supported_native_timings[0].vmode =
                        AMBA_VIDEO_MODE_HDMI_NATIVE;
        }
        pvideo_sink->hdmi_native_mode =
                pedid->native_timings.supported_native_timings[0].vmode;
        pvideo_sink->hdmi_native_width =
                pedid->native_timings.supported_native_timings[0].h_active;
        pvideo_sink->hdmi_native_height =
                pedid->native_timings.supported_native_timings[0].v_active;

        amba_vout_video_source_cmd(
                pvideo_sink->source_id,
                AMBA_VIDEO_SOURCE_REPORT_SINK_EVENT,
                (void *)AMB_EV_VOUT_HDMI_PLUG);

        ambhdmi_edid_print(pedid);

        /* Enable CEC Rx Interrupt */
        reg = regbase + HDMI_INT_ENABLE_OFFSET;
        val = amba_readl(reg);
        val |= HDMI_INT_ENABLE_CEC_RX_INT_EN;
        amba_writel(reg, val);

        /* If working previously, reenable hdmise clock */
        reg = regbase + HDMI_OP_MODE_OFFSET;
        val = amba_readl(reg);
        if (val & HDMI_OP_MODE_OP_EN) {
                reg = regbase + HDMI_CLOCK_GATED_OFFSET;
                val = amba_readl(reg);
                val |= HDMI_CLOCK_GATED_HDMISE_CLOCK_EN;
                amba_writel(reg, val);
        }

        /* Switch between HDMI and DVI Mode */
        reg = regbase + HDMI_OP_MODE_OFFSET;
        val = amba_readl(reg);
        if (HDMI_OP_MODE_OP_MODE(val) == HDMI_OP_MODE_OP_MODE_HDMI &&
            phdmi_sink->edid.interface == DVI) {
                val &= 0xfffffffe;
                val |= HDMI_OP_MODE_OP_MODE_DVI;
                amba_writel(reg, val);
        }
        if (HDMI_OP_MODE_OP_MODE(val) == HDMI_OP_MODE_OP_MODE_DVI &&
            phdmi_sink->edid.interface == HDMI) {
                val &= 0xfffffffe;
                val |= HDMI_OP_MODE_OP_MODE_HDMI;
                amba_writel(reg, val);
        }
}

static int ambhdmi_task(void *arg)
{
        u32				regbase, reg, val;
        u32				irq, status;
        amba_hdmi_edid_t 		*pedid;
        char *				envp[2];
        struct __amba_vout_video_sink	*pvideo_sink;
        struct ambhdmi_sink		*phdmi_sink;

        pvideo_sink     = (struct __amba_vout_video_sink *)arg;
        phdmi_sink      = (struct ambhdmi_sink *)pvideo_sink->pinfo;
        regbase         = phdmi_sink->regbase;
        irq             = phdmi_sink->irq;
        pedid           = &phdmi_sink->edid;

        while (1) {
                wait_event_interruptible(phdmi_sink->irq_waitqueue,
                                         (phdmi_sink->irq_pending ||
                                          phdmi_sink->killing_kthread));
                if (phdmi_sink->killing_kthread)
                        break;
                phdmi_sink->irq_pending = 0;

                msleep(200);
                status = amba_readl(regbase + HDMI_STS_OFFSET);

                /* Rx Sense Remove or HPD Low */
                if ((status & AMBHDMI_PLUG_IN) != AMBHDMI_PLUG_IN) {
                        if ((status & HDMI_STS_RX_SENSE) == 0 &&
                            pvideo_sink->hdmi_plug == AMBA_VOUT_SINK_PLUGGED) {
                                /* HDMI Device Removed */
                                ambhdmi_unplug(pvideo_sink);
                                /* event report */
                                envp[0] = "HOTPLUG=1";
                                envp[1] = NULL;
                                amb_event_report_uevent(&phdmi_sink->dev->kobj,
                                                        KOBJ_CHANGE,
                                                        envp);
                        }
                }

                /* Rx Sense & HPD High */
                if ((status & AMBHDMI_PLUG_IN) == AMBHDMI_PLUG_IN &&
                    pvideo_sink->hdmi_plug != AMBA_VOUT_SINK_PLUGGED) {
                        /* HDMI Device Connected */
                        ambhdmi_plug(pvideo_sink);
                        /* event report */
                        envp[0] = "HOTPLUG=0";
                        envp[1] = NULL;
                        amb_event_report_uevent(&phdmi_sink->dev->kobj,
                                                KOBJ_CHANGE, envp);
                };

                /* CEC Rx */
                if (status & HDMI_INT_STS_CEC_RX_INTERRUPT) {
                        reg = regbase + HDMI_INT_ENABLE_OFFSET;
                        val = amba_readl(reg);
                        if (val & HDMI_INT_ENABLE_CEC_RX_INT_EN) {
                                ambhdmi_cec_receive_message(phdmi_sink);
                        }
                }

                amba_writel(regbase + HDMI_INT_STS_OFFSET, 0);
                enable_irq(irq);
        }

        return 0;
}

#ifdef CONFIG_PROC_FS
static int ambarella_hdmi_edid_proc_show(struct seq_file *m, void *v)
{
        int                     i, j, rval = 0;
        struct ambhdmi_sink     *phdmi_sink = m->private;

        /* Check HDMI sink */
        if (phdmi_sink == NULL) {
                seq_printf(m, "No HDMI sink!\n");
                return -EINVAL;
        }

        if (phdmi_sink->video_sink.hdmi_plug == AMBA_VOUT_SINK_REMOVED) {
                rval = seq_printf(m, "No HDMI Plugged In!\n");
                return rval;
        }
        if (!phdmi_sink->raw_edid.buf) {
                rval = seq_printf(m, "No EDID Contained !\n");
                return rval;
        }

        /* Base EDID */
        seq_printf(m, "Basic EDID:\n");
        for (i = 0; i < EDID_PER_SEGMENT_SIZE; i++) {
                if ((i & 0xf) == 0) {
                        seq_printf(m, "0x%02x: ", i);
                }
                seq_printf(m, "%02x ", phdmi_sink->raw_edid.buf[i]);
                if ((i & 0xf) == 0xf) {
                        seq_printf(m, "\n");
                }
        }

        /* EDID Extensions */
        if (phdmi_sink->raw_edid.buf[0x7e]) {
                for (i = 0; i < phdmi_sink->raw_edid.buf[0x7e]; i++) {
                        seq_printf(m, "EDID Extensions #%d:\n", i + 1);
                        for (j = 0; j < EDID_PER_SEGMENT_SIZE; j++) {
                                if ((j & 0xf) == 0) {
                                        seq_printf(m, "0x%02x: ", j);
                                }
                                seq_printf(m, "%02x ",
                                           phdmi_sink->raw_edid.buf[EDID_PER_SEGMENT_SIZE * (i + 1) + j]);
                                if ((j & 0xf) == 0xf) {
                                        seq_printf(m, "\n");
                                }
                        }
                }
        }

        return rval;
}

static int ambarella_hdmi_edid_proc_open(struct inode *inode,
                                         struct file *file)
{
        return single_open(file,
                           ambarella_hdmi_edid_proc_show,
                           PDE_DATA(inode));
}

static const struct file_operations ambarella_hdmi_edid_fops = {
        .open = ambarella_hdmi_edid_proc_open,
        .read = seq_read,
        .llseek = seq_lseek,
};
#endif

static int ambhdmi_create_edid_proc(struct ambhdmi_sink *phdmi_sink)
{
        int rval = 0;

        phdmi_sink->proc_entry = proc_create_data(AMBHDMI_EDID_PROC_NAME,
                                                  S_IRUGO,
                                                  get_ambarella_proc_dir(),
                                                  &ambarella_hdmi_edid_fops,
                                                  phdmi_sink);
        if (phdmi_sink->proc_entry == NULL) {
                vout_err("%s, create edid file failed!\n", __func__);
                rval = -ENOMEM;
        }

        return rval;
}

static int ambhdmi_remove_edid_proc(struct ambhdmi_sink *phdmi_sink)
{
        remove_proc_entry(AMBHDMI_EDID_PROC_NAME, get_ambarella_proc_dir());
        phdmi_sink->proc_entry = NULL;

        return 0;
}

static irqreturn_t ambhdmi_isr(int irq, void *dev_id)
{
        struct ambhdmi_sink *phdmi_sink = (struct ambhdmi_sink *)dev_id;

        disable_irq_nosync(irq);
        if (phdmi_sink && !phdmi_sink->irq_pending) {
                phdmi_sink->irq_pending = 1;
                wake_up(&phdmi_sink->irq_waitqueue);
        }

        return IRQ_HANDLED;
}

static int ambhdmi_hw_init(struct ambhdmi_sink *phdmi_sink)
{
        int     rval = 0;
        u32     regbase;

        regbase = phdmi_sink->regbase;

        /* Power on HDMI 5V */
        /* Soft Reset HDMISE */
        amba_writel(regbase + HDMI_HDMISE_SOFT_RESET_OFFSET,
                    HDMI_HDMISE_SOFT_RESET);
        amba_writel(regbase + HDMI_HDMISE_SOFT_RESET_OFFSET,
                    ~HDMI_HDMISE_SOFT_RESET);

        /* Reset CEC */
        amba_writel(regbase + CEC_CTRL_OFFSET, 0x1 << 31);

        /* Clock Gating */
        amba_writel(regbase + HDMI_CLOCK_GATED_OFFSET, 0);

        /* Enable Hotplug detect and loss interrupt */
        amba_writel(regbase + HDMI_INT_ENABLE_OFFSET,
                    HDMI_INT_ENABLE_PHY_RX_SENSE_REMOVE_EN |
                    HDMI_INT_ENABLE_PHY_RX_SENSE_EN |
                    HDMI_INT_ENABLE_HOT_PLUG_LOSS_EN |
                    HDMI_INT_ENABLE_HOT_PLUG_DETECT_EN
                   );

        return rval;
}

static int ambhdmi_reset(struct __amba_vout_video_sink *psink, u32 *args)
{
        int                     rval = 0;
        struct ambhdmi_sink     *phdmi_sink;

        phdmi_sink = (struct ambhdmi_sink *)psink->pinfo;

        psink->pstate = psink->state;
        psink->state = AMBA_VOUT_SINK_STATE_IDLE;
        amba_writel(phdmi_sink->regbase + HDMI_CLOCK_GATED_OFFSET, 0);

        return rval;
}

static int ambhdmi_set_video_mode(struct __amba_vout_video_sink *psink,
                                  struct amba_video_sink_mode *sink_mode)
{
        int                             cs, rval = 0;
        struct ambhdmi_sink             *phdmi_sink;
        const amba_hdmi_edid_t          *pedid;
        const amba_hdmi_color_space_t   *pcolor_space;
        struct amba_video_sink_mode     vout_mode;
        const amba_hdmi_video_timing_t  *vt = NULL;

        phdmi_sink = (struct ambhdmi_sink *)psink->pinfo;

        if (sink_mode->sink_type == AMBA_VOUT_SINK_TYPE_AUTO)
                sink_mode->sink_type = psink->sink_type;
        if (sink_mode->sink_type != psink->sink_type) {
                vout_err("%s-%d sink_type is %d, not %d!\n",
                         psink->name, psink->id,
                         psink->sink_type, sink_mode->sink_type);
                return -ENOIOCTLCMD;
        }

        /* Find Video Timing */
        vt = ambhdmi_edid_find_video_mode(phdmi_sink, sink_mode->mode);
        if (!vt) {
                vout_err("%s-%d can not support mode %d!\n",
                         psink->name, psink->id, sink_mode->mode);
                return -ENOIOCTLCMD;
        }

        vout_info("%s-%d switch to mode %d!\n",
                  psink->name, psink->id, sink_mode->mode);

        memcpy(&vout_mode, sink_mode, sizeof(vout_mode));
        vout_mode.ratio = AMBA_VIDEO_RATIO_16_9;
        vout_mode.bits = AMBA_VIDEO_BITS_16;
        vout_mode.type = AMBA_VIDEO_TYPE_YUV_601;
        vout_mode.sink_type = AMBA_VOUT_SINK_TYPE_HDMI;
        if (vt->interlace)
                vout_mode.format = AMBA_VIDEO_FORMAT_INTERLACE;
        else
                vout_mode.format = AMBA_VIDEO_FORMAT_PROGRESSIVE;
        vout_mode.lcd_cfg.mode = AMBA_VOUT_LCD_MODE_DISABLE;

        /* Select HDMI output color space */
        pedid		= &phdmi_sink->edid;
        pcolor_space	= &pedid->color_space;
        cs		= AMBA_VOUT_HDMI_CS_RGB;
        switch (vout_mode.hdmi_color_space) {
        case AMBA_VOUT_HDMI_CS_AUTO:
                cs = AMBA_VOUT_HDMI_CS_RGB;
                break;

        case AMBA_VOUT_HDMI_CS_YCBCR_444:
                if (pcolor_space->support_ycbcr444) {
                        cs = AMBA_VOUT_HDMI_CS_YCBCR_444;
                } else {
                        DRV_PRINT("%s: HDMI Sink Device doesn't support "
                                  "YCBCR444 input! Use RGB444 instead.\n",
                                  __func__);
                }
                break;

        case AMBA_VOUT_HDMI_CS_YCBCR_422:
                if (pcolor_space->support_ycbcr422) {
                        cs = AMBA_VOUT_HDMI_CS_YCBCR_422;
                } else {
                        DRV_PRINT("%s: HDMI Sink Device doesn't support "
                                  "YCBCR422 input! Use RGB444 instead.\n",
                                  __func__);
                }
                break;

        default:
                break;
        }

        vout_mode.hdmi_color_space = cs;
        vout_mode.hdmi_displayer_timing = vt;
        rval = amba_vout_video_source_cmd(psink->source_id,
                                          AMBA_VIDEO_SOURCE_INIT_HDMI,
                                          &vout_mode);
        if (rval)
                return rval;

        amba_osd_on_vout_change(psink->source_id, &vout_mode);

        psink->hdmi_3d_structure = sink_mode->hdmi_3d_structure;
        psink->hdmi_overscan = sink_mode->hdmi_overscan;
        psink->cs = cs;
        psink->vt = vt;
        ambhdmi_hdmise_init(phdmi_sink, cs,
                            sink_mode->hdmi_3d_structure,
                            sink_mode->hdmi_overscan, vt);

        psink->state = AMBA_VOUT_SINK_STATE_RUNNING;

        return rval;
}

static int ambhdmi_suspend(struct __amba_vout_video_sink *psink, u32 *args)
{
        int                     rval = 0;
        struct ambhdmi_sink     *phdmi_sink;

        phdmi_sink = (struct ambhdmi_sink *)psink->pinfo;

        psink->pstate = psink->state;
        psink->state = AMBA_VOUT_SINK_STATE_SUSPENDED;
        psink->hdmi_plug = AMBA_VOUT_SINK_REMOVED;
        amba_writel(phdmi_sink->regbase + HDMI_CLOCK_GATED_OFFSET, 0);

        return rval;
}

static int ambhdmi_resume(struct __amba_vout_video_sink *psink, u32 *args)
{
        int     rval = 0;
        struct ambhdmi_sink *phdmi_sink = (struct ambhdmi_sink *)psink->pinfo;

        rval = ambhdmi_hw_init(phdmi_sink);
        if (rval) {
                vout_err("%s, %d\n", __func__, __LINE__);
        }

        if(psink->vt != NULL)
                ambhdmi_hdmise_init(phdmi_sink, psink->cs,
                                    psink->hdmi_3d_structure,
                                    psink->hdmi_overscan, psink->vt);

        return rval;
}

static int ambhdmi_docmd(struct __amba_vout_video_sink *psink,
                         enum amba_video_sink_cmd cmd, void *args)
{
        int     rval = 0;

        switch (cmd) {
        case AMBA_VIDEO_SINK_IDLE:
                break;

        case AMBA_VIDEO_SINK_RESET:
                rval = ambhdmi_reset(psink, args);
                break;

        case AMBA_VIDEO_SINK_SUSPEND:
                rval = ambhdmi_suspend(psink, args);
                break;

        case AMBA_VIDEO_SINK_RESUME:
                rval = ambhdmi_resume(psink, args);
                break;

        case AMBA_VIDEO_SINK_GET_SOURCE_ID:
                *(int *)args = psink->source_id;
                break;

        case AMBA_VIDEO_SINK_GET_INFO: {
                struct amba_vout_sink_info	*psink_info;

                psink_info = (struct amba_vout_sink_info *)args;
                psink_info->source_id = psink->source_id;
                psink_info->sink_type = psink->sink_type;
                memcpy(psink_info->name, psink->name,
                       sizeof(psink_info->name));
                psink_info->state = psink->state;
                psink_info->hdmi_plug = psink->hdmi_plug;
                memcpy(psink_info->hdmi_modes, psink->hdmi_modes,
                       sizeof(psink->hdmi_modes));
                psink_info->hdmi_native_mode = psink->hdmi_native_mode;
                psink_info->hdmi_native_width = psink->hdmi_native_width;
                psink_info->hdmi_native_height = psink->hdmi_native_height;
                psink_info->hdmi_overscan = psink->hdmi_overscan;
        }
        break;

        case AMBA_VIDEO_SINK_GET_MODE:
                rval = -ENOIOCTLCMD;
                break;

        case AMBA_VIDEO_SINK_SET_MODE:
                rval = ambhdmi_set_video_mode(psink,
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

static int ambhdmi_add_video_sink(struct i2c_adapter *adapter,
                                  struct device *pdev)
{
        int				i, rval = 0;
        struct ambhdmi_sink		*phdmi_sink;
        struct __amba_vout_video_sink	*pvideo_sink;
        amba_hdmi_edid_t 		*pedid;

        /* Malloc HDMI INFO */
        phdmi_sink = kzalloc(sizeof(struct ambhdmi_sink), GFP_KERNEL);
        if (!phdmi_sink) {
                vout_err("%s, %d\n", __func__, __LINE__);
                return -ENOMEM;
        }

        /* Fill HDMI INFO */
        phdmi_sink->dev = pdev;
        phdmi_sink->regbase = hdmi_instance.io_mem.start;
        phdmi_sink->irq = hdmi_instance.irq.start;
        phdmi_sink->irq_pending	= 0;
        phdmi_sink->killing_kthread = 0;
        phdmi_sink->ddc_adapter = adapter;
        phdmi_sink->audio_transition.notifier_call =
                ambhdmi_hdmise_audio_transition;
        init_waitqueue_head(&phdmi_sink->irq_waitqueue);

        pedid = &phdmi_sink->edid;
        pedid->interface = HDMI;
        pedid->color_space.support_ycbcr444 = 0;
        pedid->color_space.support_ycbcr422 = 0;
        pedid->cec.physical_address = 0xffff;
        pedid->cec.logical_address = CEC_DEV_UNREGISTERED;

        /* Add Sink and Register Audio Notifier */
        pvideo_sink = &phdmi_sink->video_sink;
        pvideo_sink->source_id = hdmi_instance.source_id;
        pvideo_sink->sink_type = hdmi_instance.sink_type;
        strlcpy(pvideo_sink->name, hdmi_instance.name,
                sizeof(pvideo_sink->name));
        pvideo_sink->state = AMBA_VOUT_SINK_STATE_IDLE;
        pvideo_sink->hdmi_plug = AMBA_VOUT_SINK_REMOVED;
        for (i = 0; i < AMBA_HDMI_MODE_MAX; i++) {
                pvideo_sink->hdmi_modes[i] = AMBA_VIDEO_MODE_MAX;
        }
        pvideo_sink->hdmi_native_mode = AMBA_VIDEO_MODE_MAX;
        pvideo_sink->owner = THIS_MODULE;
        pvideo_sink->pinfo = phdmi_sink;
        pvideo_sink->docmd = ambhdmi_docmd;

        rval = amba_vout_add_video_sink(pvideo_sink);
        if (rval) {
                if (phdmi_sink)
                        kfree(phdmi_sink);
                return rval;
        }
        ambarella_audio_register_notifier(&phdmi_sink->audio_transition);
        dev_set_drvdata(pdev, phdmi_sink);
        phdmi_sink->kthread = kthread_run(ambhdmi_task, pvideo_sink, "hdmid");
        vout_notice("%s:%d@%d probed!\n", pvideo_sink->name, pvideo_sink->id,
                    pvideo_sink->source_id);

        return 0;
}

static int ambhdmi_del_video_sink(struct ambhdmi_sink *phdmi_sink)
{
        int     rval = 0;

        if (phdmi_sink->kthread) {
                phdmi_sink->killing_kthread = 1;
                wake_up(&phdmi_sink->irq_waitqueue);
                kthread_stop(phdmi_sink->kthread);
        }
        if (phdmi_sink->raw_edid.buf)
                vfree(phdmi_sink->raw_edid.buf);
        ambarella_audio_unregister_notifier(&phdmi_sink->audio_transition);
        rval = amba_vout_del_video_sink(&phdmi_sink->video_sink);
        vout_notice("%s module removed!\n", phdmi_sink->video_sink.name);
        kfree(phdmi_sink);

        return rval;
}


static int ambhdmi_probe(struct device *dev)
{
        int                     rval = 0;
        struct i2c_adapter      *adapter;
        struct ambhdmi_sink     *phdmi_sink;

        /* Add video sink */
        adapter = i2c_get_adapter(1);
        if (!adapter) {
                vout_err("%s get i2c failed!\n", __func__);
                return -ENODEV;
        }

        rval = ambhdmi_add_video_sink(adapter, dev);
        if (rval) {
                vout_err("%s add video sink failed!\n", __func__);
                return rval;
        }

        phdmi_sink = dev_get_drvdata(dev);

        /* Create proc directory */
        rval = ambhdmi_create_edid_proc(phdmi_sink);
        if (rval) {
                vout_err("%s, %d\n", __func__, __LINE__);
                goto create_edid_proc_error;
        }

        /* Init hardware */

        rval = ambhdmi_hw_init(phdmi_sink);
        if (rval) {
                vout_err("%s, %d\n", __func__, __LINE__);
                goto hw_init_error;
        }

        rval = request_irq(phdmi_sink->irq, ambhdmi_isr,
                           IRQF_TRIGGER_HIGH, "HDMI", phdmi_sink);
        if(rval) {
                vout_err("%s, %d\n", __func__, __LINE__);
                goto hw_init_error;
        }

        return rval;

hw_init_error:
        ambhdmi_remove_edid_proc(phdmi_sink);

create_edid_proc_error:
        ambhdmi_del_video_sink(phdmi_sink);
        return rval;
}

static int ambhdmi_remove(struct device *dev)
{
        struct ambhdmi_sink *phdmi_sink;

        phdmi_sink = dev_get_drvdata(dev);
        ambhdmi_remove_edid_proc(phdmi_sink);
        ambhdmi_del_video_sink(phdmi_sink);
        free_irq(phdmi_sink->irq, phdmi_sink);

        return 0;
}

struct amb_driver ambhdmi = {
        .probe = ambhdmi_probe,
        .remove = ambhdmi_remove,
        .driver = {
                .name = "ambhdmi",
                .owner = THIS_MODULE,
        }
};

static int __init ambhdmi_init(void)
{
        return amb_register_driver(&ambhdmi);
}

static void __exit ambhdmi_exit(void)
{
        amb_unregister_driver(&ambhdmi);
}

module_init(ambhdmi_init);
module_exit(ambhdmi_exit);

MODULE_DESCRIPTION("Ambarella Built-in HDMI Controller");
MODULE_AUTHOR("Zhenwu Xue, <zwxue@ambarella.com>");
MODULE_LICENSE("Proprietary");

