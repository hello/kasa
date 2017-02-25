/*
 * priv.c
 *
 * History:
 *  2015/04/01 - [Zhaoyang Chen] created file
 *
 * Copyright (c) 2016 Ambarella, Inc.
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

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <basetypes.h>
#include "iav_ioctl.h"

#include "lib_vproc.h"
#include "priv.h"

const char* PM_TYPE_STR[] = { "MB Main", "Pixel" };


int map_pm_buffer(void)
{
	u16 pm_unit = vproc.pm_info.unit;

	if (pm_unit == IAV_PM_UNIT_MB) {
		return ioctl_map_pm(IAV_BUFFER_PM_MCTF);
	} else {
		return ioctl_map_pm(IAV_BUFFER_PM_BPC);
	}
}

int get_pm_type(void)
{
	int type = -1;

	if (ioctl_get_pm_info() < 0) {
		return -1;
	}
	switch (vproc.pm_info.unit) {
		case IAV_PM_UNIT_MB:
			type = PM_MB_MAIN;
			break;
		case IAV_PM_UNIT_PIXEL:
			type = PM_PIXEL;
			break;
		default:
			break;
	}
	if (type < 0) {
		ERROR("Cannot handle privacy mask unit %d, domain %d.\n",
		    vproc.pm_info.unit, vproc.pm_info.domain);
	} else {
		DEBUG("==== %s ======\n", PM_TYPE_STR[type]);
	}
	return type;
}

pm_node_t* create_pm_node(const int mask_id, const struct iav_rect* mask_rect)
{
	pm_node_t* new_node = (pm_node_t*) malloc(sizeof(pm_node_t));
	if (likely(new_node)) {
		memset(new_node, 0, sizeof(pm_node_t));
		new_node->id = mask_id;
		if (likely(mask_rect)) {
			new_node->rect = *mask_rect;
		}
	} else {
		ERROR("cannot malloc node.\n");
	}
	return new_node;
}

inline int is_rect_overlap(const struct iav_rect* a, const struct iav_rect* b)
{
	return ((MAX(a->x + a->width, b->x + b->width) - MIN(a->x, b->x))
	< (a->width + b->width))&& ((MAX(a->y + a->height, b->y + b->height)
			- MIN(a->y, b->y)) < (a->height + b->height));
}

inline void get_overlap_rect(struct iav_rect* overlap,
    const struct iav_rect* a,
    const struct iav_rect* b)
{
	overlap->x = MAX(a->x, b->x);
	overlap->y = MAX(a->y, b->y);
	overlap->width = MIN(a->x + a->width, b->x + b->width) - overlap->x;
	overlap->height = MIN(a->y + a->height, b->y + b->height) - overlap->y;
}

inline int roundup_to_mb(const int pixels, const int pair)
{
	if (pair) {
		return ROUND_UP(pixels, 32) / 16;
	} else {
		return ROUND_UP(pixels, 16) / 16;
	}
}

inline int rounddown_to_mb(const int pixels, const int pair)
{
	if (pair) {
		return ROUND_DOWN(pixels, 32) / 16;
	} else {
		return ROUND_DOWN(pixels, 16) / 16;
	}
}

inline void rect_to_rectmb(struct iav_rect* rect_mb, const struct iav_rect* rect)
{
	int pair = (vproc.resource.encode_mode == DSP_BLEND_ISO_MODE);
	rect_mb->x = rounddown_to_mb(rect->x, 0);
	rect_mb->y = rounddown_to_mb(rect->y, pair);
	rect_mb->width = roundup_to_mb(rect->x + rect->width, 0) - rect_mb->x;
	rect_mb->height = roundup_to_mb(rect->y + rect->height, pair) - rect_mb->y;
}

int rect_vin_to_main(struct iav_rect* rect_main, const struct iav_rect* rect_vin)
{
	struct iav_rect* in = &vproc.dptz[IAV_SRCBUF_MN]->input;
	struct iav_window* out = &vproc.srcbuf[IAV_SRCBUF_MN].size;

	TRACE_RECTP("rect in vin", rect_vin);
	if (!is_rect_overlap(rect_vin, in)) {
		TRACE("not in main buffer.\n");
		return -1;
	}
	get_overlap_rect(rect_main, rect_vin, in);
	// Convert offset to dptz I input
	rect_main->x = ((rect_main->x - in->x) * out->width + (in->width >> 1))
		/ in->width;
	rect_main->y = ((rect_main->y - in->y) * out->height + (in->height >> 1))
	    / in->height;
	rect_main->width = (rect_main->width * out->width + (in->width >> 1))
		/ in->width;
	rect_main->height = (rect_main->height * out->height + (in->height >> 1))
	    / in->height;

	TRACE_RECTP("\t\t=> rect in main buffer", rect_main);
	return 0;

}

void rect_vin_to_buf(struct iav_rect* rect_in_buf,
    const struct iav_rect* rect_in_vin)
{
	pm_buffer_t *buf = &vproc.pm_record.buffer;

	TRACE_RECTP("rect in vin", rect_in_vin);

	rect_in_buf->x = rect_in_vin->x + buf->active_win.x;
	rect_in_buf->y = rect_in_vin->y + buf->active_win.y;
	rect_in_buf->width = rect_in_vin->width;
	rect_in_buf->height = rect_in_vin->height;

	TRACE_RECTP("\t\t=> rect in the buffer", rect_in_buf);
}

void rect_main_to_vin(struct iav_rect* rect_vin,
	const struct iav_rect* rect_main)
{
	struct iav_rect* in = &vproc.dptz[IAV_SRCBUF_MN]->input;
	struct iav_window* out = &vproc.srcbuf[IAV_SRCBUF_MN].size;

	rect_vin->x = in->x + (in->width * rect_main->x + (out->width >> 1))
		/ out->width;
	rect_vin->width = (in->width * rect_main->width + (out->width >> 1))
		/ out->width;
	rect_vin->y = in->y + (in->height * rect_main->y + (out->height >> 1))
		/ out->height;
	rect_vin->height = (in->height * rect_main->height + (out->height >> 1))
		/ out->height;

	TRACE_RECTP("rect in main", rect_main);
	TRACE_RECTP("\t\t=> rect in vin", rect_vin);
}

int rect_main_to_srcbuf(struct iav_rect* rect_in_srcbuf,
    const struct iav_rect* rect_in_main, const int src_id)
{
	struct iav_rect* input = &vproc.dptz[src_id]->input;
	struct iav_window* output = &vproc.srcbuf[src_id].size;

	if (src_id == IAV_SRCBUF_MN) {
		*rect_in_srcbuf = *rect_in_main;
		return 0;
	}

	TRACE_RECTP("rect in main buffer", rect_in_main);
	if (!is_rect_overlap(rect_in_main, input)) {
		TRACE("not in source buffer %d.\n", src_id);
		return -1;
	}
	get_overlap_rect(rect_in_srcbuf, rect_in_main, input);
	// Convert offset to srcbuf input
	rect_in_srcbuf->x = (rect_in_srcbuf->x - input->x) * output->width
	    / input->width;
	rect_in_srcbuf->y = (rect_in_srcbuf->y - input->y) * output->height
	    / input->height;
	rect_in_srcbuf->width = rect_in_srcbuf->width * output->width
	    / input->width;
	rect_in_srcbuf->height = rect_in_srcbuf->height * output->height
	    / input->height;

	TRACE_RECTP("\t\t=> rect in source buffer", rect_in_srcbuf);
	return 0;
}

inline int is_id_in_map(const int i, const u32 map)
{
	return map & (1 << i);
}

static int check_pm_add(const VPROC_OP op, const int id, const struct iav_rect* rect)
{
	struct iav_window* mainbuf = &vproc.srcbuf[IAV_SRCBUF_MN].size;
	pm_node_t* node = vproc.pm_record.node_in_vin;

	if (!rect) {
		ERROR("Rect Privacy mask is NULL.\n");
		return -1;
	}

	if (!rect->width || !rect->height) {
		ERROR("Privacy mask rect %ux%u cannot be zero.\n", rect->width,
		    rect->height);
		return -1;
	}

	if ((rect->x + rect->width > mainbuf->width)
	    || (rect->y + rect->height > mainbuf->height)) {
		ERROR("Privacy mask rect %ux%u offset %ux%u is out of main buffer"
			" %ux%u.\n", rect->width, rect->height, rect->x, rect->y,
		    mainbuf->width, mainbuf->height);
		return -1;
	}

	if (op == VPROC_OP_INSERT) {
		while (node) {
			if (id == node->id) {
				ERROR("Rect privacy mask id [%d] existed.\n", id);
				return -1;
			}
			node = node->next;
		}
	}

	return 0;
}

static int check_pm_remove(const int id)
{
	pm_node_t* node = vproc.pm_record.node_in_vin;
	while (node) {
		if (id == node->id) {
			return 0;
		}
		node = node->next;
	}

	ERROR("Rect privacy mask [%d] does not exist.\n", id);
	return -1;
}

int check_pm_params(pm_param_t* pm_params)
{
	int i;
	pm_cfg_t* pm_cfg = NULL;

	switch (pm_params->op) {
		case VPROC_OP_INSERT:
		case VPROC_OP_DRAW:
			if (pm_params->pm_num > MAX_PM_NUM) {
				ERROR("Invalid rect pm num %d, should be <= %d.\n",
					pm_params->pm_num, MAX_PM_NUM);
				return -1;
			}
			for (i = 0; i < pm_params->pm_num; i++) {
				pm_cfg = &pm_params->pm_list[i];
				if (check_pm_add(pm_params->op, pm_cfg->id,
					&pm_cfg->rect) < 0) {
					return -1;
				}
			}
			break;
		case VPROC_OP_REMOVE:
			if (pm_params->pm_num > MAX_PM_NUM) {
				ERROR("Invalid rect pm num %d, should be <= %d.\n",
					pm_params->pm_num, MAX_PM_NUM);
				return -1;
			}
			for (i = 0; i < pm_params->pm_num; i++) {
				pm_cfg = &pm_params->pm_list[i];
				if (check_pm_remove(pm_cfg->id) < 0) {
					return -1;
				}
			}
			break;
		/* No need to check */
		case VPROC_OP_CLEAR:
		case VPROC_OP_SHOW:
		default:
			break;
	}

	return 0;
}

void display_pm_nodes(void)
{
	pm_node_t* node = vproc.pm_record.node_in_vin;

	DEBUG("\tTotal rect mask number = %d\n", vproc.pm_record.node_num);
	while (node) {
		DEBUG("\t  rect mask%d (x %d, y %d, w %d, h %d)\n", node->id,
			node->rect.x, node->rect.y, node->rect.width, node->rect.height);
		node = node->next;
	}
}

int output_pm_params(pm_param_t* pm_params)
{
	pm_node_t* node = vproc.pm_record.node_in_vin;
	pm_cfg_t* pm_cfg;
	int i;

	pm_params->pm_num = vproc.pm_record.node_num;
	i = 0;
	while (node) {
		pm_cfg = &pm_params->pm_list[i];
		pm_cfg->id = node->id;
		rect_vin_to_main(&pm_cfg->rect, &node->rect);
		i = i + 1;
		node = node->next;
	}
	if (i != pm_params->pm_num) {
		ERROR("Rect PM num mistmatch, expected: %d, actual: %d.\n",
			i, pm_params->pm_num);
	}

	return 0;
}
