/*
 * lib_vproc.h
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

#ifndef LIB_VPROC_H_
#define LIB_VPROC_H_

#ifdef __cplusplus
extern "C" {
#endif


#define OVERLAY_CLUT_BYTES       (1024)

#define DEFAULT_COLOR_Y          12
#define DEFAULT_COLOR_U          128
#define DEFAULT_COLOR_V          128

#define MAX_PM_NUM               32
#define MAX_PMC_NUM              16

typedef enum {
	VPROC_OP_INSERT = 0,
	VPROC_OP_REMOVE = 1,
	VPROC_OP_UPDATE = 2,
	VPROC_OP_DRAW = 3,
	VPROC_OP_CLEAR = 4,
	VPROC_OP_SHOW = 5,
} VPROC_OP;

typedef struct {
	int major;
	int minor;
	int patch;
	unsigned int mod_time;
	char description[64];
} version_t;

// Don't change the order of vuy
typedef struct yuv_color_s {
	u8 v;
	u8 u;
	u8 y;
	u8 alpha;        // Unused for privacy mask
} yuv_color_t;

typedef struct pm_cfg_s {
	int id;
	struct iav_rect rect;
} pm_cfg_t;

typedef struct pm_param_s {
	VPROC_OP op;
	int pm_num;
	pm_cfg_t pm_list[MAX_PM_NUM];
} pm_param_t;

typedef struct mctf_param_s {
	int strength;
	struct iav_rect rect;
} mctf_param_t;

typedef struct cawarp_param_s {
	int data_num;
	float* data;
	float red_factor;
	float blue_factor;
	int center_offset_x;  // lens offset to VIN center. Left shift is negative while right shift is positive.
	int center_offset_y;  // lens offset to VIN center. Up shift is negative while down shift is positive.
} cawarp_param_t;

typedef struct pmc_area_s {
	struct iav_offset center;
	int radius;
} pmc_area_t;

typedef enum {
	PMC_TYPE_OUTSIDE = 0,   // PM outside of the circle
	PMC_TYPE_INSIDE = 1,    // PM inside the circle
} PMC_TYPE;

typedef struct pmc_cfg_s {
	int id;
	int pmc_type;
	pmc_area_t area;
} pmc_cfg_t;

typedef struct pmc_param_s {
	VPROC_OP op;
	int pm_num;
	pmc_cfg_t pm_list[MAX_PM_NUM];
} pmc_param_t;


int vproc_pm(pm_param_t* masks_in_main);
int vproc_pm_color(yuv_color_t* mask_color);
int vproc_dptz(struct iav_digital_zoom* dptz);
int vproc_pmc(pmc_param_t* masks_in_vin);

int vproc_cawarp(cawarp_param_t* cawarp);

int vproc_exit(void);
int vproc_version(version_t* version);


#ifdef __cplusplus
}
#endif

#endif /* LIB_VPROC_H_ */

