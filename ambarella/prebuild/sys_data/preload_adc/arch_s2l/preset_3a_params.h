/**
 * prebuild/sys_data/preload_adc/arch_s2l/preset_3a_params.h
 *
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

#ifndef __PRESET_3A_PARAMS__
#define __PRESET_3A_PARAMS__

#include "adc.h"
/* layout of struct smart3a_file_info is as follows:
**    offset: unsigned int
**    r_gain, b_gain, d_gain, shutter, agc:  unsigned int
**    para0, para1, para2, para3, para4:    unsigned char
**    rev[3]: unsigned char
**
*/
static struct smart3a_file_info preset_3a_params[]={
#if defined(CONFIG_BOARD_VERSION_S2LMELEKTRA_OV4689_S2L22M) || defined (CONFIG_BOARD_VERSION_S2LMELEKTRA_OV4689_S2L55M)
	[0] = {
		.r_gain = 6400,
		.b_gain = 10000,
		.d_gain = 4096,
		.shutter = 1500,
		.agc = 60,
		.para0 = 0x00,
		.para1 = 0x60,
		.para2 = 0x00,
		.para3 = 0x00,
		.para4 = 0x80
	},
	[1] = {
		.r_gain = 6400,
		.b_gain = 10000,
		.d_gain = 4096,
		.shutter = 1500,
		.agc = 60,
		.para0 = 0x00,
		.para1 = 0x60,
		.para2 = 0x00,
		.para3 = 0x00,
		.para4 = 0x80
	},
	[2] = {
		.r_gain = 6400,
		.b_gain = 10000,
		.d_gain = 4096,
		.shutter = 1500,
		.agc = 60,
		.para0 = 0x00,
		.para1 = 0x60,
		.para2 = 0x00,
		.para3 = 0x00,
		.para4 = 0x80
	},
	[3] = {
		.r_gain = 6400,
		.b_gain = 10000,
		.d_gain = 4096,
		.shutter = 1500,
		.agc = 60,
		.para0 = 0x00,
		.para1 = 0x60,
		.para2 = 0x00,
		.para3 = 0x00,
		.para4 = 0x80
	},
	[4] = {
		.r_gain = 6400,
		.b_gain = 10000,
		.d_gain = 4096,
		.shutter = 1500,
		.agc = 60,
		.para0 = 0x00,
		.para1 = 0x60,
		.para2 = 0x00,
		.para3 = 0x00,
		.para4 = 0x80
	},
#elif defined (CONFIG_BOARD_VERSION_S2LMELEKTRA_IMX322_S2L22M) || defined (CONFIG_BOARD_VERSION_S2LMELEKTRA_IMX322_S2L55M)
       [0] = {
		.r_gain = 7600,
		.b_gain = 6096,
		.d_gain = 3978,
		.shutter = 562,
		.agc = 55,
		.para0 = 0x02,
		.para1 = 0x34,
		.para2 = 0x37,
	},
	[1] = {
		.r_gain = 6012,
		.b_gain = 9992,
		.d_gain = 4051,
		.shutter = 562,
		.agc = 53,
		.para0 = 0x02,
		.para1 = 0x34,
		.para2 = 0x35,
	},
	[2] = {
		.r_gain = 5956,
		.b_gain = 9732,
		.d_gain = 4061,
		.shutter = 562,
		.agc = 1,
		.para0 = 0x02,
		.para1 = 0x34,
		.para2 = 0x01,
	},
	[3] = {
		.r_gain = 6068,
		.b_gain = 9404,
		.d_gain = 4046,
		.shutter = 281,
		.agc = 7,
		.para0 = 0x03,
		.para1 = 0x4d,
		.para2 = 0x07,
	},
	[4] = {
		.r_gain = 5348,
		.b_gain = 10740,
		.d_gain = 4129,
		.shutter = 562,
		.agc = 10,
		.para0 = 0x03,
		.para1 = 0x14,
		.para2 = 0x1d,
	},
#else
	[0] = {
		.r_gain = 6400,
		.b_gain = 10000,
		.d_gain = 4096,
		.shutter = 1500,
		.agc = 60,
		.para0 = 0x00,
		.para1 = 0x60,
		.para2 = 0x04,
		.para3 = 0x00,
		.para4 = 0x80
	},
	[1] = {
		.r_gain = 6400,
		.b_gain = 10000,
		.d_gain = 4096,
		.shutter = 1500,
		.agc = 60,
		.para0 = 0x00,
		.para1 = 0x60,
		.para2 = 0x04,
		.para3 = 0x00,
		.para4 = 0x80
	},
	[2] = {
		.r_gain = 6400,
		.b_gain = 10000,
		.d_gain = 4096,
		.shutter = 1500,
		.agc = 60,
		.para0 = 0x00,
		.para1 = 0x60,
		.para2 = 0x04,
		.para3 = 0x00,
		.para4 = 0x80
	},
	[3] = {
		.r_gain = 6400,
		.b_gain = 10000,
		.d_gain = 4096,
		.shutter = 1500,
		.agc = 60,
		.para0 = 0x00,
		.para1 = 0x60,
		.para2 = 0x04,
		.para3 = 0x00,
		.para4 = 0x80
	},
	[4] = {
		.r_gain = 6400,
		.b_gain = 10000,
		.d_gain = 4096,
		.shutter = 1500,
		.agc = 60,
		.para0 = 0x00,
		.para1 = 0x60,
		.para2 = 0x04,
		.para3 = 0x00,
		.para4 = 0x80
	},
#endif
};
#endif
