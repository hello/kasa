/**
 * dsp.h
 *
 * Author: Cao Rongrong <rrcao@ambarella.com>
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

#ifndef __IAV_DSP_H__
#define __IAV_DSP_H__

#include <dsp/dspfw.h>

#if (CHIP_REV == S2L)
#include "s2l_cmd_msg.h"
#include "s2l_mem.h"
#endif

/* Interface type sync from kernel/private/include/vin_sensors.h */
typedef enum {
	SENSOR_PARALLEL_LVCMOS = 0,
	SENSOR_SERIAL_LVDS = 1,
	SENSOR_PARALLEL_LVDS = 2,
	SENSOR_MIPI = 3,
} SENSOR_IF;

/* DSP status sync from include/iav_ioctl.h */
enum iav_state {
	IAV_STATE_IDLE = 0,
	IAV_STATE_PREVIEW = 1,
	IAV_STATE_ENCODING = 2,
	IAV_STATE_STILL_CAPTURE = 3,
	IAV_STATE_DECODING = 4,
	IAV_STATE_TRANSCODING = 5,
	IAV_STATE_DUPLEX = 6,
	IAV_STATE_EXITING_PREVIEW = 7,
	IAV_STATE_INIT = 0xFF,
};

/* ==========================================================================*/
/* Used for dump dsp cmd with Hex format */
#define AMBOOT_IAV_DEBUG_DSP_CMD
#undef AMBOOT_IAV_DEBUG_DSP_CMD

#ifdef AMBOOT_IAV_DEBUG_DSP_CMD
#define dsp_print_cmd(cmd, cmd_size) 	\
	do { 					\
		int _i_; 				\
		int _j_;				\
		u32 *pcmd_raw = (u32 *)cmd; \
		putstr(#cmd);		\
		putstr("[");			\
		putdec(cmd_size);	\
		putstr("] = {\r\n");	\
		for (_i_ = 0, _j_= 1 ; _i_ < ((cmd_size + 3) >> 2); _i_++, _j_++) {	\
			putstr("0x");			\
			puthex(pcmd_raw[_i_]);\
			putstr(", ");			\
			if (_j_== 4) {			\
				putstr("\r\n");	\
				_j_ = 0;		\
			}				\
		} 					\
		putstr("\r\n}\r\n");	\
	} while (0)
#else
#define dsp_print_cmd(cmd, cmd_size)
#endif

/* ==========================================================================*/
extern void vin_phy_init(int interface_type);
extern int dsp_init(void);
extern int dsp_boot(void);
extern int add_dsp_cmd(void *cmd, unsigned int size);

extern void audio_set_play_size(unsigned int addr, unsigned int size);
extern void audio_init(void);
extern void audio_start(void);

extern int dsp_get_ucode_by_name(struct dspfw_header *hdr, const char *name,
		unsigned int *addr, unsigned int *size);

#endif

