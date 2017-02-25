/*
 * pm_mb.c
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <basetypes.h>
#include "iav_ioctl.h"

#include "lib_vproc.h"
#include "priv.h"

static int G_mbmain_inited = 0;

static inline int get_next_buffer_id(const pm_buffer_t* buf)
{
	return (buf->id >= PM_MB_BUF_NUM - 1) ? 0 : (buf->id + 1);
}

static inline int get_cur_buffer_id(pm_buffer_t* buf)
{
	if (unlikely(buf->id < 0)) {
		buf->id = 0;
	}
	return buf->id;
}

static int get_mb_bytes(const int pitch, const int mb_height)
{
	return pitch * mb_height;
}

static void clear_pm(const int buf_id)
{
	pm_buffer_t* buf = &vproc.pm_record.buffer;
	struct iav_mctf_filter* pm =
	    (struct iav_mctf_filter *) buf->addr[buf_id];
	int pm_num = buf->bytes / sizeof(struct iav_mctf_filter);
	int i;
	for (i = 0; i < pm_num; ++i) {
		pm[i].mask_privacy = 0;
	}
	DEBUG("clear pm_mctf buffer %d\n", buf_id);
}

static void copy_pm(const int src_buf_id, const int dst_buf_id)
{
	pm_buffer_t* buf = &vproc.pm_record.buffer;
	if (src_buf_id < 0) {
		return;
	}
	memcpy(buf->addr[dst_buf_id], buf->addr[src_buf_id], buf->bytes);
	DEBUG("copy buffer %d to buffer %d\n", src_buf_id, dst_buf_id);
}

static void fill_buffer_pm(const pm_buffer_t* buf, const int buf_id,
    const struct iav_rect* rect_mb, const int is_mask)
{
	int i, j;
	struct iav_mctf_filter* p;
	u8* line_addr = buf->addr[buf_id] + rect_mb->y * buf->pitch
	    + rect_mb->x * sizeof(struct iav_mctf_filter);

	for (i = 0; i < rect_mb->height; i++) {
		p = (struct iav_mctf_filter *) line_addr;
		for (j = 0; j < rect_mb->width; j++, p++) {
			p->mask_privacy = (is_mask ? 0x1 : 0x0);
		}
		line_addr += buf->pitch;
	}
}

static void draw_pm(const int buf_id, pm_node_t* node)
{
	struct iav_rect pm_mb, pm_in_main;

	if (rect_vin_to_main(&pm_in_main, &node->rect) == 0) {
		rect_to_rectmb(&pm_mb, &pm_in_main);
		fill_buffer_pm(&vproc.pm_record.buffer, buf_id, &pm_mb, 1);
	}
	node->redraw = 0;
}

static void erase_pm(const int buf_id, pm_node_t* node)
{
	pm_node_t* cur = vproc.pm_record.node_in_vin;
	struct iav_rect pm_mb, pm_in_main;

	if (rect_vin_to_main(&pm_in_main, &node->rect) == 0) {
		rect_to_rectmb(&pm_mb, &pm_in_main);
		fill_buffer_pm(&vproc.pm_record.buffer, buf_id, &pm_mb, 0);
	}

	// Mark the masks overlapped with the removed one.
	while (cur) {
		if ((cur != node) && is_rect_overlap(&cur->rect, &node->rect)) {
			cur->redraw = 1;
			DEBUG("mark mask%d to be redraw\n", cur->id);
		}
		cur = cur->next;
	}
}

static void add_mbmain_pm(const int buf_id, const pm_cfg_t *pm_cfg)
{
	struct iav_rect rect_in_vin;
	pm_node_t* cur, *prev;

	TRACE("try to add new mask%d\n", pm_cfg->id);
	rect_main_to_vin(&rect_in_vin, &pm_cfg->rect);
	cur = vproc.pm_record.node_in_vin;
	prev = NULL;
	while (cur) {
		prev = cur;
		cur = cur->next;
	}
	cur = create_pm_node(pm_cfg->id, &rect_in_vin);
	if (prev == NULL) {
		vproc.pm_record.node_in_vin = cur;
	} else {
		prev->next = cur;
	}
	++vproc.pm_record.node_num;
	draw_pm(buf_id, cur);
	DEBUG("append the new mask%d\n", cur->id);
}

static void remove_mbmain_pm(const int buf_id, int pm_id)
{
	pm_node_t* cur, *prev;

	cur = vproc.pm_record.node_in_vin;
	prev = NULL;

	while (cur) {
		if (pm_id == PM_ALL || cur->id == pm_id ) {
			DEBUG("remove mask%d\n", cur->id);
			erase_pm(buf_id, cur);
			if (prev == NULL ) {
				// cur is head
				vproc.pm_record.node_in_vin = cur->next;
				free(cur);
				cur = vproc.pm_record.node_in_vin;
			} else {
				prev->next = cur->next;
				free(cur);
				cur = prev->next;
			}
			DEBUG("remove done\n");
			--vproc.pm_record.node_num;
		} else {
			prev = cur;
			cur = cur->next;
		}
	}
}

static void update_mbmain_pm(const int buf_id)
{
	pm_node_t* cur, *prev;

	cur = vproc.pm_record.node_in_vin;
	prev = NULL;

	while (cur) {
		draw_pm(buf_id, cur);
		prev = cur;
		cur = prev->next;
	}
}

int init_mbmain(void)
{
	int i;
	u32 total_size, single_size;
	struct iav_window* mainbuf;
	pm_buffer_t* buf;

	if (unlikely(!G_mbmain_inited)) {
		buf = &vproc.pm_record.buffer;
		do {
			if (map_pm_buffer() < 0) {
				break;
			}
			if (ioctl_get_srcbuf_format(IAV_SRCBUF_MN) < 0) {
				break;
			}
			if (ioctl_get_system_resource() < 0) {
				break;
			}
			mainbuf = &vproc.srcbuf[IAV_SRCBUF_MN].size;
			buf->domain_width = mainbuf->width;
			buf->domain_height = mainbuf->height;
			buf->pitch = vproc.pm_info.buffer_pitch;
			buf->width = roundup_to_mb(buf->domain_width, 0);
			buf->height = roundup_to_mb(buf->domain_height, 1);
			buf->id = -1;
			buf->bytes = get_mb_bytes(buf->pitch, buf->height);
			total_size = buf->bytes * PM_MB_BUF_NUM;

			DEBUG("domain %dx%d, buffer pitch %d, width %d, height %d, "
				"size 0x%x\n", buf->domain_width, buf->domain_height,
			    buf->pitch, buf->width, buf->height, buf->bytes);

			if (unlikely(total_size > vproc.pm_mem.length)) {
				ERROR("Memory size [0x%x] for mb main privacy mask "
					"must be no less than [0x%x].\n", vproc.pm_mem.length,
				    total_size);
				break;
			}

			single_size = vproc.pm_mem.length / PM_MB_BUF_NUM;
			for (i = 0; i < PM_MB_BUF_NUM; ++i) {
				buf->addr[i] = vproc.pm_mem.addr + i * single_size;
				clear_pm(i);
			}
			vproc.pm->y = DEFAULT_COLOR_Y;
			vproc.pm->u = DEFAULT_COLOR_U;
			vproc.pm->v = DEFAULT_COLOR_V;
			G_mbmain_inited = 1;

		} while (0);
	}
	return G_mbmain_inited ? 0 : -1;
}

int deinit_mbmain(void)
{
	pm_node_t* node, *next;

	if (G_mbmain_inited) {
		node = vproc.pm_record.node_in_vin;
		DEBUG("free total %d masks\n", vproc.pm_record.node_num);
		while (node) {
			DEBUG("free mask%d\n", node->id);
			next = node->next;
			free(node);
			node = next;
		}
		vproc.pm_record.node_in_vin = NULL;
		vproc.pm_record.node_num = 0;
		G_mbmain_inited = 0;
		DEBUG("done\n");
	}
	return G_mbmain_inited ? -1 : 0;
}

int operate_mbmain_pm(pm_param_t* pm_params)
{
	pm_node_t* cur;
	int cur_buf_id, buf_id, i;
	pm_buffer_t* buf = &vproc.pm_record.buffer;
	VPROC_OP op;
	pm_cfg_t *pm_cfg;

	if (init_dptz() < 0 || init_mbmain() < 0)
		return -1;

	if (check_pm_params(pm_params) < 0)
		return -1;

	cur_buf_id = get_cur_buffer_id(buf);
	buf_id = get_next_buffer_id(buf);
	op = pm_params->op;

	// show previous PM
	if (pm_params->op == VPROC_OP_SHOW) {
		return output_pm_params(pm_params);
	}

	if (pm_params->op == VPROC_OP_UPDATE) {
		clear_pm(buf_id);
	} else if (cur_buf_id >= 0) {
		copy_pm(cur_buf_id, buf_id);
	}
	switch (op) {
	case VPROC_OP_DRAW:
	case VPROC_OP_CLEAR:
		remove_mbmain_pm(buf_id, PM_ALL);
		break;
	case VPROC_OP_REMOVE:
		for (i = 0; i < pm_params->pm_num; i++) {
			pm_cfg = &pm_params->pm_list[i];
			remove_mbmain_pm(buf_id, pm_cfg->id);
		}
		break;
	case VPROC_OP_UPDATE:
		update_mbmain_pm(buf_id);
		break;
	default:
		break;
	}

	// redraw previous PM
	cur = vproc.pm_record.node_in_vin;
	while (cur) {
		if (cur->redraw == 1) {
			draw_pm(buf_id, cur);
			DEBUG("redraw done: mask%d\n", cur->id);
		}
		cur = cur->next;
	}
	display_pm_nodes();
	// append new PM
	if (op == VPROC_OP_INSERT || op == VPROC_OP_DRAW) {
		for (i = 0; i < pm_params->pm_num; i++) {
			pm_cfg = &pm_params->pm_list[i];
			add_mbmain_pm(buf_id, pm_cfg);
		}
	}
	display_pm_nodes();

	buf->id = buf_id;

	vproc.pm->enable = 1;
	vproc.pm->data_addr_offset = buf->addr[buf_id] - vproc.pm_mem.addr;
	vproc.pm->buf_pitch = buf->pitch;
	vproc.pm->buf_height = buf->height;

	return ioctl_cfg_pm();
}

int operate_mbmain_color(yuv_color_t* color)
{
	if (init_mbmain() < 0)
		return -1;

	if (ioctl_get_pm() < 0)
		return -1;

	vproc.pm->y = color->y;
	vproc.pm->u = color->u;
	vproc.pm->v = color->v;

	return ioctl_cfg_pm();
}

int disable_mbmain_pm(void)
{
	int buf_id = get_cur_buffer_id(&vproc.pm_record.buffer);
	clear_pm(buf_id);
	vproc.pm->enable = 0;
	return ioctl_cfg_pm();
}

