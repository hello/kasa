/*
 * pm_pixel.c
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


static int G_pixel_inited;

static u8 enable_table[8][8] = {
	{0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x7F, 0xFF},
	{0x03, 0x02, 0x06, 0x0E, 0x1E, 0x3E, 0x7E, 0xFE},
	{0x07, 0x06, 0x04, 0x0C, 0x1C, 0x3C, 0x7C, 0xFC},
	{0x0F, 0x0E, 0x0C, 0x08, 0x18, 0x38, 0x78, 0xF8},
	{0x1F, 0x1E, 0x1C, 0x18, 0x10, 0x30, 0x70, 0xF0},
	{0x3F, 0x3E, 0x3C, 0x38, 0x30, 0x20, 0x60, 0xE0},
	{0x7F, 0x7E, 0x7C, 0x78, 0x70, 0x60, 0x40, 0xC0},
	{0xFF, 0xFE, 0xFC, 0xF8, 0xF0, 0xE0, 0xC0, 0x80},
};

static u8 disable_table[8][8] = {
	{0xFE, 0xFC, 0xF8, 0xF0, 0xE0, 0xC0, 0x80, 0x00},
	{0xFC, 0xFD, 0xF9, 0xF1, 0xE1, 0xC1, 0x81, 0x01},
	{0xF8, 0xF9, 0xFB, 0xF3, 0xE3, 0xC3, 0x83, 0x03},
	{0xF0, 0xF1, 0xF3, 0xF7, 0xE7, 0xC7, 0x87, 0x07},
	{0xE0, 0xE1, 0xE3, 0xE7, 0xEF, 0xCF, 0x8F, 0x0F},
	{0xC0, 0xC1, 0xC3, 0xC7, 0xCF, 0xDF, 0x9F, 0x1F},
	{0x80, 0x81, 0x83, 0x87, 0x8F, 0x9F, 0xBF, 0x3F},
	{0x00, 0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x7F},
};

static inline u32 get_dist_square(u32 x1, u32 y1, u32 x2, u32 y2)
{
	return (x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2);
}

static int is_circle_overlap(pmc_node_t *a, pmc_node_t *b)
{
	u32 square, square_len;
	int overlap = 0;

	square = get_dist_square(a->area.center.x, a->area.center.y,
		b->area.center.x, b->area.center.y);
	square_len = (a->area.radius + b->area.radius) *
		(a->area.radius + b->area.radius);

	if ((a->pmc_type == PMC_TYPE_INSIDE && b->pmc_type == PMC_TYPE_INSIDE) &&
		(square > square_len)) {
		overlap = 0;
	} else {
		overlap = 1;
	}

	return overlap;
}

static int is_circle_rect_overlap(pmc_node_t *circle, pm_node_t *rect)
{
	u32 square, square_len;
	u32 rect_x, rect_y;
	u32 dist_x, dist_y;
	int overlap = 0;

	rect_x = rect->rect.x + rect->rect.width / 2;
	rect_y = rect->rect.y + rect->rect.height / 2;

	dist_x = abs(circle->area.center.x - rect_x);
	dist_y = abs(circle->area.center.y - rect_y);

	if (dist_x > (rect->rect.width / 2 + circle->area.radius) ||
		dist_y > (rect->rect.height / 2 + circle->area.radius)) {
		overlap = 0;
	} else if (dist_x <= rect->rect.width / 2 ||
		dist_y <= rect->rect.height / 2) {
		overlap = 1;
	} else {
		square_len =
			(dist_x - rect->rect.width / 2) * (dist_x - rect->rect.width / 2) +
			(dist_y - rect->rect.height / 2) * (dist_x - rect->rect.height / 2);
		square = circle->area.radius * circle->area.radius;
		overlap = (square_len <= square);
	}

	if (overlap == 0 && circle->pmc_type == PMC_TYPE_INSIDE) {
		overlap = 0;
	} else {
		overlap = 1;
	}

	return overlap;
}

pmc_node_t* create_pmc_node(const pmc_cfg_t *pmc_cfg)
{
	pmc_node_t* new_node = (pmc_node_t*) malloc(sizeof(pmc_node_t));
	if (likely(new_node)) {
		memset(new_node, 0, sizeof(pmc_node_t));
		new_node->id = pmc_cfg->id;
		new_node->pmc_type = pmc_cfg->pmc_type;
		new_node->area = pmc_cfg->area;
	} else {
		ERROR("cannot malloc node.\n");
	}
	return new_node;
}

static int check_pmc_add(const VPROC_OP op, const pmc_cfg_t *pmc_cfg)
{
	int vin_width = vproc.pmc_record.buffer.domain_width;
	int vin_height = vproc.pmc_record.buffer.domain_height;
	pmc_node_t* node = vproc.pmc_record.node_in_vin;
	const pmc_area_t *area = &pmc_cfg->area;

	if ((pmc_cfg->pmc_type != PMC_TYPE_INSIDE) &&
		(pmc_cfg->pmc_type != PMC_TYPE_OUTSIDE)) {
		ERROR("Incorrect Circle Privacy mask type %d.\n");
		return -1;
	}

	if (!area) {
		ERROR("Circle Privacy mask area is NULL.\n");
		return -1;
	}

	if (!area->radius) {
		ERROR("Circle Privacy mask radius %u cannot be zero.\n", area->radius);
		return -1;
	}

	if ((area->center.x < area->radius) ||
		(area->center.y < area->radius) ||
		(area->center.x + area->radius > vin_width) ||
		(area->center.y + area->radius > vin_height)) {
		ERROR("Circle Privacy mask center %ux%u radius %u is out of VIN buffer"
			" %ux%u.\n", area->center.x, area->center.y, area->radius,
			vin_width, vin_height);
		return -1;
	}

	if (op == VPROC_OP_INSERT) {
		while (node) {
			if (pmc_cfg->id == node->id) {
				ERROR("Circle privacy mask id [%d] existed.\n", pmc_cfg->id);
				return -1;
			}
			node = node->next;
		}
	}

	return 0;
}

static int check_pmc_remove(const int id)
{
	pmc_node_t* node = vproc.pmc_record.node_in_vin;
	while (node) {
		if (id == node->id) {
			return 0;
		}
		node = node->next;
	}

	ERROR("Circle privacy mask [%d] does not exist.\n", id);
	return -1;
}

static int check_pmc_params(pmc_param_t* pmc_params)
{
	int i;
	pmc_cfg_t* pmc_cfg = NULL;

	switch (pmc_params->op) {
		case VPROC_OP_INSERT:
		case VPROC_OP_DRAW:
			if (pmc_params->pm_num > MAX_PMC_NUM) {
				ERROR("Invalid circle pm num %d, should be <= %d.\n",
					pmc_params->pm_num, MAX_PMC_NUM);
				return -1;
			}
			for (i = 0; i < pmc_params->pm_num; i++) {
				pmc_cfg = &pmc_params->pm_list[i];
				if (check_pmc_add(pmc_params->op, pmc_cfg) < 0) {
					return -1;
				}
			}
			break;
		case VPROC_OP_REMOVE:
			if (pmc_params->pm_num > MAX_PMC_NUM) {
				ERROR("Invalid circle pm num %d, should be <= %d.\n",
					pmc_params->pm_num, MAX_PMC_NUM);
				return -1;
			}
			for (i = 0; i < pmc_params->pm_num; i++) {
				pmc_cfg = &pmc_params->pm_list[i];
				if (check_pmc_remove(pmc_cfg->id) < 0) {
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

static void display_pmc_nodes(void)
{
	pmc_node_t* node = vproc.pmc_record.node_in_vin;

	DEBUG("\tTotal circle mask number = %d\n", vproc.pmc_record.node_num);
	while (node) {
		DEBUG("\t  circle mask%d, center: (x %d, y %d), r: %d)\n", node->id,
			node->area.center.x, node->area.center.y, node->area.radius);
		node = node->next;
	}
}

static void clear_pm(void)
{
	pm_buffer_t* buf = &vproc.pm_record.buffer;
	pmc_node_t* pmc_cur = vproc.pmc_record.node_in_vin;
	u32 total_size;

	total_size = vproc.pm_mem.length;
	memset(buf->addr[0], 0, total_size);

	// Mark all circle pm to be redrawed.
	while (pmc_cur) {
		pmc_cur->redraw = 1;
		DEBUG("mark circle mask%d to be redraw\n", pmc_cur->id);
		pmc_cur = pmc_cur->next;
	}
	DEBUG("clear rect pm done\n");
}

static void clear_pmc(void)
{
	pm_buffer_t* buf = &vproc.pmc_record.buffer;
	pm_node_t* pm_cur = vproc.pm_record.node_in_vin;
	u32 total_size;

	total_size = vproc.pm_mem.length;
	memset(buf->addr[0], 0, total_size);

	// Mark all rect pm to be redrawed.
	while (pm_cur) {
		pm_cur->redraw = 1;
		DEBUG("mark rect mask%d to be redraw\n", pm_cur->id);
		pm_cur = pm_cur->next;
	}
	DEBUG("clear circle pm done\n");
}

static void fill_pm_line(u8* addr, u16 l_idx, u16 r_idx, u16 l_off, u16 r_off,
	u16 left_pixels, int is_mask)
{
	if (is_mask) {
		if (unlikely(l_idx == r_idx)) {
			*addr |= enable_table[l_off][r_off];
		} else {
			if (l_off) {
				*addr++ |= enable_table[l_off][7];
				left_pixels -= 8 - l_off;
			}
			while (left_pixels / 8) {
				*addr |= enable_table[0][7];
				addr++;
				left_pixels -= 8;
			}
			if (left_pixels > 0) {
				*addr++ |= enable_table[0][left_pixels - 1];
			}
		}
	} else {
		if (unlikely(l_idx == r_idx)) {
			*addr &= disable_table[l_off][r_off];
		} else {
			if (l_off) {
				*addr++ &= disable_table[l_off][7];
				left_pixels -= 8 - l_off;
			}
			while (left_pixels / 8) {
				*addr++ &= disable_table[0][7];
				left_pixels -= 8;
			}
			if (left_pixels > 0) {
				*addr++ &= disable_table[0][left_pixels - 1];
			}
		}
	}
}

static void fill_buffer_pm(const struct iav_rect* rect, const int is_mask)
{
	int i;
	pm_buffer_t* buf = &vproc.pm_record.buffer;
	u16 left_idx, right_idx;
	u16 left_off, right_off;
	u16 left_pixels;
	u8* addr = NULL;

	left_idx = rect->x >> 3;
	right_idx = (rect->x + rect->width - 1) >> 3;
	left_off = rect->x % 8;
	right_off = (rect->x + rect->width - 1) % 8;

	for (i = 0; i < rect->height; i++) {
		left_pixels = rect->width;
		addr = buf->addr[0] + rect->y * buf->pitch + left_idx +
			i * buf->pitch;
		fill_pm_line(addr, left_idx, right_idx, left_off, right_off,
			left_pixels, is_mask);
	}
}

static int get_pmc_mask_length(const int y, const pmc_area_t* area,
	u16 *x_start)
{
	u32 square, dist;
	int i;

	if (y < area->center.y - area->radius ||
		y > area->center.y + area->radius) {
		return 0;
	}
	square = area->radius * area->radius;
	*x_start = 0;
	for (i = area->center.x - area->radius; i <= area->center.x; i++) {
		dist = get_dist_square((u32)i, (u32)y, area->center.x,
			area->center.y);
		if (dist <= square) {
			*x_start = i;
			break;
		}
	}

	return  2 * (area->center.x - *x_start) + 1;
}

static void fill_buffer_pmc_in(const pmc_area_t* area, const int is_mask)
{
	int i;
	pm_buffer_t* buf = &vproc.pmc_record.buffer;
	const struct iav_offset* center = &area->center;
	u16 left_idx, right_idx;
	u16 left_off, right_off;
	u16 left_pixels, radius;
	u16 x_start = 0, length;
	u8* addr = NULL;

	radius = area->radius;
	for (i = center->y - radius; i < center->y + radius; i++) {
		length = get_pmc_mask_length(i, area, &x_start);
		left_idx = x_start >> 3;
		right_idx = (x_start + length - 1) >> 3;
		left_off = x_start % 8;
		right_off = (x_start + length - 1) % 8;
		left_pixels = length;
		addr = buf->addr[0] + i * buf->pitch + left_idx;
		fill_pm_line(addr, left_idx, right_idx, left_off, right_off,
			left_pixels, is_mask);
	}
}

static void fill_buffer_pmc_out(const pmc_area_t* area, const int is_mask)
{
	int i;
	pm_buffer_t* buf = &vproc.pmc_record.buffer;
	const struct iav_offset* center = &area->center;
	u16 left_idx, right_idx;
	u16 left_off, right_off;
	u16 left_pixels, radius;
	u16 x_start = 0, length;
	u8* addr = NULL;

	radius = area->radius;
	for (i = 0; i < buf->domain_height; i++) {
		if (i < center->y - radius || i > center->y + radius) {
			left_idx = left_off = 0;
			right_idx = (buf->domain_width - 1) >> 3;
			right_off = (buf->domain_width - 1) % 8;
			left_pixels = buf->domain_width;
			addr = buf->addr[0] + i * buf->pitch;
			fill_pm_line(addr, left_idx, right_idx, left_off, right_off,
				left_pixels, is_mask);
		}
		else {
			length = get_pmc_mask_length(i, area, &x_start);
			// left half
			left_idx = left_off = 0;
			right_idx = (x_start - 1) >> 3;
			right_off = (x_start - 1) % 8;
			left_pixels = x_start;
			addr = buf->addr[0] + i * buf->pitch;
			fill_pm_line(addr, left_idx, right_idx, left_off, right_off,
				left_pixels, is_mask);
			// right half
			left_idx = (x_start + length) >> 3;
			left_off = (x_start + length) % 8;
			right_idx = (buf->domain_width - 1) >> 3;
			right_off = (buf->domain_width - 1) % 8;
			left_pixels = buf->domain_width - x_start - length;
			addr = buf->addr[0] + i * buf->pitch + left_idx;
			fill_pm_line(addr, left_idx, right_idx, left_off, right_off,
				left_pixels, is_mask);
		}
	}
}

static void draw_pm(pm_node_t* node)
{
	struct iav_rect rect_buf;

	rect_vin_to_buf(&rect_buf, &node->rect);
	fill_buffer_pm(&rect_buf, 1);

	node->redraw = 0;
}

static void draw_pmc(pmc_node_t *node)
{
	// Already VIN domain, so no map
	if (node->pmc_type == PMC_TYPE_INSIDE) {
		fill_buffer_pmc_in(&node->area, 1);
	} else {
		fill_buffer_pmc_out(&node->area, 1);
	}

	node->redraw = 0;
}

static void erase_pm(pm_node_t* node)
{
	struct iav_rect rect_buf;
	pm_node_t* pm_cur = vproc.pm_record.node_in_vin;
	pmc_node_t* pmc_cur = vproc.pmc_record.node_in_vin;

	rect_vin_to_buf(&rect_buf, &node->rect);
	fill_buffer_pm(&rect_buf, 0);

	// Mark the masks overlapped with the removed one.
	while (pm_cur) {
		if ((pm_cur != node) && is_rect_overlap(&pm_cur->rect, &node->rect)) {
			pm_cur->redraw = 1;
			DEBUG("mark rect mask%d to be redraw\n", pm_cur->id);
		}
		pm_cur = pm_cur->next;
	}
	// Mark the circle masks overlapped with the removed one.
	while (pmc_cur) {
		if (is_circle_rect_overlap(pmc_cur, node)) {
			pmc_cur->redraw = 1;
			DEBUG("mark circle mask%d to be redraw\n", pmc_cur->id);
		}
		pmc_cur = pmc_cur->next;
	}
}

static void erase_pmc(pmc_node_t *node)
{
	pmc_node_t* pmc_cur = vproc.pmc_record.node_in_vin;
	pm_node_t* pm_cur = vproc.pm_record.node_in_vin;

	if (node->pmc_type == PMC_TYPE_INSIDE) {
		fill_buffer_pmc_in(&node->area, 0);
	} else {
		fill_buffer_pmc_out(&node->area, 0);
	}

	// Mark the circle masks overlapped with the removed one.
	while (pmc_cur) {
		if ((pmc_cur != node) && is_circle_overlap(pmc_cur, node)) {
			pmc_cur->redraw = 1;
			DEBUG("mark circle mask%d to be redraw\n", pmc_cur->id);
		}
		pmc_cur = pmc_cur->next;
	}
	// Mark the rect masks overlapped with the removed one.
	while (pm_cur) {
		if (is_circle_rect_overlap(node, pm_cur)) {
			pm_cur->redraw = 1;
			DEBUG("mark rect mask%d to be redraw\n", pm_cur->id);
		}
		pm_cur = pm_cur->next;
	}
}

static void add_pixel_pm(const pm_cfg_t *pm_cfg)
{
	struct iav_rect rect_in_vin;
	pm_node_t* cur, *prev;

	TRACE("try to add new rect mask%d\n", pm_cfg->id);
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
	draw_pm(cur);
	DEBUG("append the new rect mask%d\n", cur->id);
}

static void add_pixel_pmc(const pmc_cfg_t *pmc_cfg)
{
	pmc_node_t* cur, *prev;

	TRACE("try to add new circle mask%d\n", pmc_cfg->id);
	cur = vproc.pmc_record.node_in_vin;
	prev = NULL;
	while (cur) {
		prev = cur;
		cur = cur->next;
	}
	cur = create_pmc_node(pmc_cfg);
	if (prev == NULL) {
		vproc.pmc_record.node_in_vin = cur;
	} else {
		prev->next = cur;
	}
	++vproc.pmc_record.node_num;
	draw_pmc(cur);
	DEBUG("append the new circle mask%d\n", cur->id);
}

static void remove_pixel_pm(int pm_id)
{
	pm_node_t* cur, *prev;

	cur = vproc.pm_record.node_in_vin;
	prev = NULL;

	while (cur) {
		if (pm_id == PM_ALL || cur->id == pm_id ) {
			DEBUG("remove rect mask%d\n", cur->id);
			erase_pm(cur);
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

static void remove_pixel_pmc(int pmc_id)
{
	pmc_node_t* cur, *prev;

	cur = vproc.pmc_record.node_in_vin;
	prev = NULL;

	while (cur) {
		if (pmc_id == PM_ALL || cur->id == pmc_id ) {
			DEBUG("remove circle mask%d\n", cur->id);
			erase_pmc(cur);
			if (prev == NULL ) {
				// cur is head
				vproc.pmc_record.node_in_vin = cur->next;
				free(cur);
				cur = vproc.pmc_record.node_in_vin;
			} else {
				prev->next = cur->next;
				free(cur);
				cur = prev->next;
			}
			DEBUG("remove done\n");
			--vproc.pmc_record.node_num;
		} else {
			prev = cur;
			cur = cur->next;
		}
	}
}

static void update_pixel_pm(void)
{
	pm_node_t* cur, *prev;

	cur = vproc.pm_record.node_in_vin;
	prev = NULL;

	while (cur) {
		draw_pm(cur);
		prev = cur;
		cur = prev->next;
	}
}

static void update_pixel_pmc(void)
{
	pmc_node_t* cur, *prev;

	cur = vproc.pmc_record.node_in_vin;
	prev = NULL;

	while (cur) {
		draw_pmc(cur);
		prev = cur;
		cur = prev->next;
	}
}

static void redraw_pixel_pm_pmc(void)
{
	pm_node_t* pm_cur;
	pmc_node_t* pmc_cur;

	// redraw previous rect PM
	pm_cur = vproc.pm_record.node_in_vin;
	while (pm_cur) {
		if (pm_cur->redraw == 1) {
			draw_pm(pm_cur);
			DEBUG("redraw done: rect mask%d\n", pm_cur->id);
		}
		pm_cur = pm_cur->next;
	}
	// redraw previous circle PM
	pmc_cur = vproc.pmc_record.node_in_vin;
	while (pmc_cur) {
		if (pmc_cur->redraw == 1) {
			draw_pmc(pmc_cur);
			DEBUG("redraw done: circle mask%d\n", pmc_cur->id);
		}
		pmc_cur = pmc_cur->next;
	}
}

static int output_pmc_params(pmc_param_t* pmc_params)
{
	pmc_node_t* node = vproc.pmc_record.node_in_vin;
	pmc_cfg_t* pmc_cfg;
	int i;

	pmc_params->pm_num = vproc.pmc_record.node_num;
	i = 0;
	while (node) {
		pmc_cfg = &pmc_params->pm_list[i];
		pmc_cfg->id = node->id;
		pmc_cfg->area = node->area;
		pmc_cfg->pmc_type = node->pmc_type;
		i = i + 1;
		node = node->next;
	}
	if (i != pmc_params->pm_num) {
		ERROR("Circle PM num mistmatch, expected: %d, actual: %d.\n",
			i, pmc_params->pm_num);
	}

	return 0;
}

int init_pixel(void)
{
	pm_buffer_t* buf = &vproc.pm_record.buffer;

	if (unlikely(!G_pixel_inited)) {
		do {
			if (map_pm_buffer() < 0) {
				break;
			}
			if (vproc.pm_info.domain == IAV_PM_DOMAIN_VIN) {
				if (ioctl_get_vin_info() < 0) {
					break;
				}
			} else {
				ERROR("Only support pixel level PM on VIN domain!\n");
				break;
			}
			buf->domain_width = vproc.vin.info.width;
			buf->domain_height = vproc.vin.info.height;
			buf->active_win.width = buf->domain_width;
			buf->active_win.height = buf->domain_height;
			buf->active_win.x = 0;
			buf->active_win.y = 0;

			if (ioctl_get_srcbuf_format(IAV_SRCBUF_MN) < 0) {
				break;
			}
			buf->pitch = vproc.pm_info.buffer_pitch;
			buf->width = vproc.pm_info.pixel_width;
			buf->height = buf->domain_height;
			buf->id = 0;
			buf->bytes = buf->pitch * buf->height;

			DEBUG("domain %dx%d, buffer pitch %d, width %d, height %d, "
				"size 0x%x\n", buf->domain_width, buf->domain_height,
			    buf->pitch, buf->width, buf->height, buf->bytes);

			if (unlikely(buf->bytes > vproc.pm_mem.length)) {
				ERROR("Memory size [0x%x] for pixel privacy mask must "
					"be no less than [0x%x].\n", vproc.pm_mem.length,
				    buf->bytes);
				break;
			}
			buf->addr[0] = vproc.pm_mem.addr;
			vproc.pmc_record.buffer = vproc.pm_record.buffer;
			clear_pm();
			clear_pmc();
			vproc.pm->y = 0;
			vproc.pm->u = 0;
			vproc.pm->v = 0;
			G_pixel_inited = 1;
		} while (0);
	}

	return G_pixel_inited ? 0 : -1;
}

int deinit_pixel(void)
{
	pm_node_t* node, *next;
	pmc_node_t* pmc_node, *pmc_next;

	if (G_pixel_inited) {
		// free rect pm
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
		// free circle pm
		pmc_node = vproc.pmc_record.node_in_vin;
		DEBUG("free total %d circle masks\n", vproc.pmc_record.node_num);
		while (pmc_node) {
			DEBUG("free circle mask%d\n", pmc_node->id);
			pmc_next = pmc_node->next;
			free(pmc_node);
			pmc_node = pmc_next;
		}
		vproc.pmc_record.node_in_vin = NULL;
		vproc.pmc_record.node_num = 0;
		G_pixel_inited = 0;
		DEBUG("done\n");
	}

	return G_pixel_inited ? -1 : 0;
}

int operate_pixel_pm(pm_param_t* pm_params)
{
	pm_buffer_t* buf = &vproc.pm_record.buffer;
	VPROC_OP op;
	pm_cfg_t* pm_cfg;
	int i;

	if (init_dptz() < 0 || init_pixel() < 0)
		return -1;

	if (check_pm_params(pm_params) < 0)
		return -1;

	op = pm_params->op;

	// show previous rect PM
	if (pm_params->op == VPROC_OP_SHOW) {
		return output_pm_params(pm_params);
	}

	if (op == VPROC_OP_UPDATE) {
		clear_pm();
	}

	switch (op) {
	case VPROC_OP_DRAW:
	case VPROC_OP_CLEAR:
		remove_pixel_pm(PM_ALL);
		break;
	case VPROC_OP_REMOVE:
		for (i = 0; i < pm_params->pm_num; i++) {
			pm_cfg = &pm_params->pm_list[i];
			remove_pixel_pm(pm_cfg->id);
		}
		break;
	case VPROC_OP_UPDATE:
		update_pixel_pm();
		break;
	default:
		break;
	}

	// redraw previous PMs
	redraw_pixel_pm_pmc();
	display_pm_nodes();

	// draw new rect PM
	if (op == VPROC_OP_INSERT || op == VPROC_OP_DRAW) {
		for (i = 0; i < pm_params->pm_num; i++) {
			pm_cfg = &pm_params->pm_list[i];
			add_pixel_pm(pm_cfg);
		}
	}
	display_pm_nodes();

	vproc.pm->enable = 1;
	vproc.pm->data_addr_offset = (u32)(buf->addr[0] - vproc.pm_mem.addr);
	vproc.pm->buf_pitch = buf->pitch;
	vproc.pm->buf_height = buf->height;

	return ioctl_cfg_pm();
}

int operate_pixel_pmc(pmc_param_t* pmc_params)
{
	pm_buffer_t* buf = &vproc.pmc_record.buffer;
	VPROC_OP op;
	pmc_cfg_t* pmc_cfg;
	int i;

	if (init_dptz() < 0 || init_pixel() < 0)
		return -1;

	if (check_pmc_params(pmc_params) < 0)
		return -1;

	op = pmc_params->op;

	// show previous circle PM
	if (pmc_params->op == VPROC_OP_SHOW) {
		return output_pmc_params(pmc_params);
	}

	if (op == VPROC_OP_UPDATE) {
		clear_pmc();
	}

	switch (op) {
	case VPROC_OP_DRAW:
	case VPROC_OP_CLEAR:
		remove_pixel_pmc(PM_ALL);
		break;
	case VPROC_OP_REMOVE:
		for (i = 0; i < pmc_params->pm_num; i++) {
			pmc_cfg = &pmc_params->pm_list[i];
			remove_pixel_pmc(pmc_cfg->id);
		}
		break;
	case VPROC_OP_UPDATE:
		update_pixel_pmc();
		break;
	default:
		break;
	}

	// redraw previous PMs
	redraw_pixel_pm_pmc();
	display_pmc_nodes();

	// draw new circle PM
	if (op == VPROC_OP_INSERT || op == VPROC_OP_DRAW) {
		for (i = 0; i < pmc_params->pm_num; i++) {
			pmc_cfg = &pmc_params->pm_list[i];
			add_pixel_pmc(pmc_cfg);
		}
	}
	display_pmc_nodes();

	vproc.pm->enable = 1;
	vproc.pm->data_addr_offset = (u32)(buf->addr[0] - vproc.pm_mem.addr);
	vproc.pm->buf_pitch = buf->pitch;
	vproc.pm->buf_height = buf->height;

	return ioctl_cfg_pm();
}

int disable_pixel_pm(void)
{
	clear_pm();
	clear_pmc();
	vproc.pm->enable = 0;

	return ioctl_cfg_pm();
}

