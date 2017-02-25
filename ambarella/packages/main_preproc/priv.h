/*
 * priv.h
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

#ifndef PRIV_H_
#define PRIV_H_

#include <basetypes.h>
#include "iav_ioctl.h"
#include "utils.h"

#define PM_MB_BUF_NUM              (8)
#define PM_PIXEL_BUF_NUM           (2)
#define PM_MAX_BUF_NUM             (PM_MB_BUF_NUM)
#define PM_ALL                     (-1)

#define TRACE_RECT(str, rect) \
	do {	\
		TRACE("%s\t(w %d, h %d, x %d, y %d)\n", \
			str, rect.width, rect.height, rect.x, rect.y); \
	} while(0)

#define TRACE_RECTP(str, rect) \
	do {	\
		TRACE("%s\t(w %d, h %d, x %d, y %d)\n", \
			str, rect->width, rect->height, rect->x, rect->y); \
	} while(0);

typedef enum {
	PM_MB_MAIN = 0,
	PM_PIXEL,
} PM_TYPE;

typedef struct mmap_info_s {
	u8	*addr;
	u32	length;
} mmap_info_t;

typedef struct pm_buffer_s {
	struct iav_rect active_win;
	int domain_width;
	int domain_height;
	int id;
	int pitch;
	int width;
	int height;
	u32 bytes;
	u8* addr[PM_MAX_BUF_NUM];
} pm_buffer_t;

typedef struct pm_node_s {
	int id;
	struct iav_rect rect;
	u32 state;
	int redraw;
	struct pm_node_s* next;
} pm_node_t;

typedef struct pm_record_s {
	pm_node_t* node_in_vin;
	int node_num;
	pm_buffer_t buffer;
} pm_record_t;

typedef struct pmc_node_s {
	int id;
	pmc_area_t area;
	int pmc_type;
	int redraw;
	struct pmc_node_s* next;
} pmc_node_t;

typedef struct pmc_record_s {
	pmc_node_t* node_in_vin;
	int node_num;
	pm_buffer_t buffer;
} pmc_record_t;

typedef struct vproc_param_s {
	mmap_info_t pm_mem;
	struct iav_system_resource resource;
	struct iav_privacy_mask_info pm_info;
	struct vindev_video_info vin;
	struct iav_srcbuf_setup srcbuf_setup;
	struct iav_srcbuf_format srcbuf[IAV_SRCBUF_NUM];

	pm_record_t pm_record;
	pmc_record_t pmc_record;

	struct iav_privacy_mask* pm;
	struct iav_digital_zoom* dptz[IAV_SRCBUF_NUM];
	struct iav_ca_warp* cawarp;

	struct iav_apply_flag apply_flag[IAV_VIDEO_PROC_NUM];
	struct iav_video_proc ioctl_pm;
	struct iav_video_proc ioctl_dptz[IAV_SRCBUF_NUM];
	struct iav_video_proc ioctl_cawarp;
} vproc_param_t;

// priv.c
int map_pm_buffer(void);
int get_pm_type(void);

int is_id_in_map(const int i, const u32 map);

pm_node_t* create_pm_node(int mask_id, const struct iav_rect* mask_rect);

int is_rect_overlap(const struct iav_rect* a, const struct iav_rect* b);
void get_overlap_rect(struct iav_rect* overlap, const struct iav_rect* a,
    const struct iav_rect* b);

int rounddown_to_mb(const int pixels, int pair);
int roundup_to_mb(const int pixels, int pair);
void rect_to_rectmb(struct iav_rect* rect_mb, const struct iav_rect* rect);
int rect_vin_to_main(struct iav_rect* rect_in_main,
    const struct iav_rect* rect_in_vin);
int rect_main_to_srcbuf(struct iav_rect* rect_in_srcbuf,
    const struct iav_rect* rect_in_main, const int src_id);
void rect_main_to_vin(struct iav_rect* rect_in_vin,
    const struct iav_rect* rect_in_main);
void rect_vin_to_buf(struct iav_rect* rect_in_buf,
    const struct iav_rect* rect_in_vin);

int check_pm_params(pm_param_t* pm_params);
void display_pm_nodes(void);
int output_pm_params(pm_param_t* pm_params);

// pm_mb.c
int operate_mbmain_pm(pm_param_t* pm_params);
int operate_mbmain_color(yuv_color_t* color);
int init_mbmain(void);
int deinit_mbmain(void);

int disable_mbmain_pm(void);

// pm_pixel.c
int operate_pixel_pm(pm_param_t* pm_params);
int operate_pixel_pmc(pmc_param_t* pmc_params);
int init_pixel(void);
int deinit_pixel(void);
int disable_pixel_pm(void);

// dptz.c
int operate_dptz(struct iav_digital_zoom* dptz);
int init_dptz(void);
int deinit_dptz(void);

// cawarp.c
int operate_cawarp(cawarp_param_t* cawarp);
int init_cawarp(void);
int deinit_cawarp(void);
int disable_cawarp(void);

// do_ioctl.c
int ioctl_map_pm(u32 buf_id);
int ioctl_get_pm_info(void);
int ioctl_get_srcbuf_format(const int source_id);
int ioctl_get_srcbuf_setup(void);
int ioctl_get_vin_info(void);
int ioctl_gdma_copy(struct iav_gdma_copy* gdma);
int ioctl_get_system_resource(void);
int ioctl_get_pm(void);
int ioctl_get_dptz(const int source_id);

int ioctl_apply(void);

int ioctl_cfg_pm(void);
int ioctl_cfg_dptz(const int buf_id);
int ioctl_cfg_cawarp(void);

extern vproc_param_t vproc;

#endif /* PRIV_H_ */
