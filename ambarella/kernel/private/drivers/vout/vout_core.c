/*
 * kernel/private/drivers/ambarella/vout/vout_core.c
 *
 * History:
 *    2008/01/18 - [Anthony Ginger] Create
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


#include <linux/idr.h>
#include <linux/i2c.h>
#include <plat/event.h>

#include "vout_pri.h"

static LIST_HEAD(vout_source);
static LIST_HEAD(vout_sink);
static DEFINE_MUTEX(vout_source_mutex);
static DEFINE_MUTEX(vout_sink_mutex);
static DEFINE_IDR(amba_vout_source_idr);
static DEFINE_IDR(amba_vout_sink_idr);

/* ========================================================================== */
int amba_vout_add_video_source(struct __amba_vout_video_source *psrc)
{
	int					retval = 0;
	int					id;

	vout_dbg("amba_vout_add_video_source\n");

	if (!psrc) {
		retval = -EINVAL;
		goto amba_vout_add_video_source_exit;
	}

	mutex_init(&psrc->cmd_lock);

	mutex_lock(&vout_source_mutex);
	id = idr_alloc_cyclic(&amba_vout_source_idr, psrc,
		psrc->id, 0, GFP_KERNEL);
	if (id < 0) {
		retval = -ENOMEM;
	} else {
		retval = 0;
	}

	if (retval == 0) {
		psrc->id = id;
		list_add_tail(&psrc->list, &vout_source);
	} else {
		vout_err("amba_vout_add_video_source %s-%d fail with %d!\n",
			psrc->name, psrc->id, retval);
	}
	mutex_unlock(&vout_source_mutex);

amba_vout_add_video_source_exit:
	return retval;
}
EXPORT_SYMBOL(amba_vout_add_video_source);

int amba_vout_del_video_source(struct __amba_vout_video_source *psrc)
{
	int					retval = 0;
	struct list_head			*item;
	struct __amba_vout_video_sink		*psink;

	vout_dbg("amba_vout_del_video_source\n");

	if (!psrc) {
		retval = -EINVAL;
		goto amba_vout_del_video_source_exit;
	}

	mutex_lock(&vout_sink_mutex);
	list_for_each(item, &vout_sink) {
		psink = list_entry(item, struct __amba_vout_video_sink, list);
		if (psink->source_id == psrc->id) {
			retval = -EBUSY;
			break;
		}
	}
	mutex_unlock(&vout_sink_mutex);
	if (retval)
		goto amba_vout_del_video_source_exit;

	mutex_lock(&vout_source_mutex);
	list_del(&psrc->list);
	idr_remove(&amba_vout_source_idr, psrc->id);
	mutex_unlock(&vout_source_mutex);

amba_vout_del_video_source_exit:
	return retval;
}
EXPORT_SYMBOL(amba_vout_del_video_source);

struct __amba_vout_video_source *__amba_vout_get_video_source(int id)
{
	struct __amba_vout_video_source		*psrc = NULL;

	vout_dbg("__amba_vout_get_video_source %d\n", id);

	mutex_lock(&vout_source_mutex);
	psrc = (struct __amba_vout_video_source *)
		idr_find(&amba_vout_source_idr, id);
	mutex_unlock(&vout_source_mutex);

	return psrc;
}

int amba_vout_add_video_sink(struct __amba_vout_video_sink *psink)
{
	int					retval = 0;
	int					id;
	struct __amba_vout_video_source		*psrc;

	vout_dbg("amba_vout_add_sink\n");

	if (!psink) {
		retval = -EINVAL;
		goto amba_vout_add_video_sink_exit;
	}

	psrc = __amba_vout_get_video_source(psink->source_id);
	if (!psrc) {
		vout_warn("Can't find psrc[%d] for %s, try default psrc!\n",
				psink->source_id, psink->name);

		psink->source_id = AMBA_VOUT_SOURCE_STARTING_ID;
		psrc = __amba_vout_get_video_source(psink->source_id);
		if (!psrc) {
			vout_warn("Can't find default psrc for %s!\n",
				psink->name);
			retval = -ENODEV;
			goto amba_vout_add_video_sink_exit;
		}
	}
	mutex_init(&psink->cmd_lock);

	retval = amba_vout_video_source_cmd(psink->source_id,
		AMBA_VIDEO_SOURCE_REGISTER_SINK, psink);
	if (retval != 0) {
		retval = -EIO;
		goto amba_vout_add_video_sink_exit;
	}

	mutex_lock(&vout_sink_mutex);
	id = idr_alloc_cyclic(&amba_vout_sink_idr, psink,
		AMBA_VOUT_SINK_STARTING_ID, 0, GFP_KERNEL);
	if (id < 0) {
		retval = -ENOMEM;
	} else {
		retval = 0;
	}

	if (retval == 0) {
		psink->id = id;
		list_add_tail(&psink->list, &vout_sink);
	} else {
		vout_err("amba_vout_add_sink %d:%s-%d fail with %d!\n",
			psink->source_id, psink->name,
			psink->id, retval);
		retval = amba_vout_video_source_cmd(psink->source_id,
			AMBA_VIDEO_SOURCE_UNREGISTER_SINK, psink);
	}
	mutex_unlock(&vout_sink_mutex);

amba_vout_add_video_sink_exit:
	return retval;
}
EXPORT_SYMBOL(amba_vout_add_video_sink);

struct __amba_vout_video_sink *__amba_vout_get_video_sink(int id)
{
	struct __amba_vout_video_sink		*psink = NULL;

	vout_dbg("__amba_vout_get_video_sink %d\n", id);

	mutex_lock(&vout_sink_mutex);
	psink = (struct __amba_vout_video_sink *)
		idr_find(&amba_vout_sink_idr, id);
	mutex_unlock(&vout_sink_mutex);

	return psink;
}

int amba_vout_del_video_sink(struct __amba_vout_video_sink *psink)
{
	int					retval = 0;

	vout_dbg("amba_vout_del_sink\n");

	if (!psink) {
		retval = -EINVAL;
		goto amba_vout_del_video_sink_exit;
	}

	if (__amba_vout_get_video_sink(psink->id) == NULL) {
		retval = -ENXIO;
		goto amba_vout_del_video_sink_exit;
	}

	retval = amba_vout_video_source_cmd(psink->source_id,
		AMBA_VIDEO_SOURCE_UNREGISTER_SINK, psink);
	if (retval) {
		retval = -EBUSY;
		goto amba_vout_del_video_sink_exit;
	}

	mutex_lock(&vout_sink_mutex);
	list_del(&psink->list);
	idr_remove(&amba_vout_sink_idr, psink->id);
	mutex_unlock(&vout_sink_mutex);

amba_vout_del_video_sink_exit:
	return retval;
}
EXPORT_SYMBOL(amba_vout_del_video_sink);

int amba_vout_video_source_cmd(int id,
	enum amba_video_source_cmd cmd, void *args)
{
	int					retval = 0;
	struct __amba_vout_video_source		*psrc;

	vout_dbg("amba_vout_video_source_cmd %d %d 0x%x\n",
		id, cmd, (u32)args);

	psrc = __amba_vout_get_video_source(id);
	if (!psrc) {
		retval = -EINVAL;
		goto amba_vout_video_source_cmd_exit;
	}

	mutex_lock(&psrc->cmd_lock);
	if (likely(psrc->docmd)) {
		retval = psrc->docmd(psrc, cmd, args);
		if (retval) {
			vout_err("%s-%d docmd[%d] return with %d!\n",
				psrc->name, psrc->id,
				cmd, retval);
			goto amba_vout_video_source_cmd_exit_cmdlock;
		}
	} else {
		vout_err("%s-%d should support docmd!\n",
			psrc->name, psrc->id);
		retval = -EINVAL;
	}

amba_vout_video_source_cmd_exit_cmdlock:
	mutex_unlock(&psrc->cmd_lock);

amba_vout_video_source_cmd_exit:
	return retval;
}
EXPORT_SYMBOL(amba_vout_video_source_cmd);

int amba_vout_video_sink_cmd(int id,
	enum amba_video_sink_cmd cmd, void *args)
{
	int					retval = 0;
	struct __amba_vout_video_sink		*psink;

	vout_dbg("amba_vout_video_sink_cmd %d 0x%x\n", cmd, (int)args);

	psink = __amba_vout_get_video_sink(id);
	if (!psink) {
		retval = -EINVAL;
		goto amba_vout_video_sink_cmd_exit;
	}

	mutex_lock(&psink->cmd_lock);
	if (likely(psink->docmd)) {
		retval = psink->docmd(psink, cmd, args);
		if (retval) {
			vout_err("%s-%d docmd[%d] return with %d!\n",
				psink->name, psink->id,
				cmd, retval);
			goto amba_vout_video_sink_cmd_exit_cmdlock;
		}
	} else {
		vout_err("%s-%d should support docmd!\n",
			psink->name, psink->id);
		retval = -EINVAL;
	}

amba_vout_video_sink_cmd_exit_cmdlock:
	mutex_unlock(&psink->cmd_lock);

amba_vout_video_sink_cmd_exit:
	return retval;
}
EXPORT_SYMBOL(amba_vout_video_sink_cmd);

int amba_vout_pm(u32 pmval)
{
	int					retval = 0;
	struct list_head			*item;
	struct __amba_vout_video_sink		*psink;
	struct __amba_vout_video_source		*psource;
	u32					sink_cmd;
	u32					source_cmd;

	switch (pmval) {
	case AMBA_EVENT_PRE_CPUFREQ:
	case AMBA_EVENT_PRE_PM:
	case AMBA_EVENT_PRE_TOSS:
		sink_cmd = AMBA_VIDEO_SINK_SUSPEND;
		source_cmd = AMBA_VIDEO_SOURCE_SUSPEND;
		break;

	case AMBA_EVENT_POST_CPUFREQ:
	case AMBA_EVENT_POST_PM:
	case AMBA_EVENT_POST_TOSS:
		sink_cmd = AMBA_VIDEO_SINK_RESUME;
		source_cmd = AMBA_VIDEO_SOURCE_RESUME;
		break;

	case AMBA_EVENT_CHECK_CPUFREQ:
	case AMBA_EVENT_CHECK_PM:
	case AMBA_EVENT_CHECK_TOSS:

	case AMBA_EVENT_TAKEOVER_DSP:
	case AMBA_EVENT_PRE_TAKEOVER_DSP:
	case AMBA_EVENT_POST_TAKEOVER_DSP:
	case AMBA_EVENT_GIVEUP_DSP:
	case AMBA_EVENT_PRE_GIVEUP_DSP:
	case AMBA_EVENT_POST_GIVEUP_DSP:
	case AMBA_EVENT_POST_VIN_LOSS:
		goto amba_vout_pm_exit;
		break;

	default:
		vout_err("%s: unknown event 0x%x\n", __func__, pmval);
		retval = -EINVAL;
		goto amba_vout_pm_exit;
		break;
	}

	mutex_lock(&vout_sink_mutex);
	list_for_each(item, &vout_sink) {
		psink = list_entry(item, struct __amba_vout_video_sink, list);
		mutex_lock(&psink->cmd_lock);
		retval = psink->docmd(psink, sink_cmd, NULL);
		if (retval) {
			vout_err("%s-%d docmd[%d] return %d!\n",
				psink->name, psink->id,
				sink_cmd, retval);
		}
		mutex_unlock(&psink->cmd_lock);
	}
	mutex_unlock(&vout_sink_mutex);
	mutex_lock(&vout_source_mutex);
	list_for_each(item, &vout_source) {
		psource = list_entry(item,
			struct __amba_vout_video_source, list);
		mutex_lock(&psource->cmd_lock);
		retval = psource->docmd(psource, source_cmd, NULL);
		if (retval) {
			vout_err("%s-%d docmd[%d] return %d!\n",
				psource->name, psource->id,
				source_cmd, retval);
		}
		mutex_unlock(&psource->cmd_lock);
	}
	mutex_unlock(&vout_source_mutex);

amba_vout_pm_exit:
	return retval;
}
EXPORT_SYMBOL(amba_vout_pm);

