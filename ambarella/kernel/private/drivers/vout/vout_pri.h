/*
 * kernel/private/drivers/ambarella/vout/vout_pri.h
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


#ifndef __VOUT_PRI_H
#define __VOUT_PRI_H

#include <iav_ioctl.h>
#include <vout_api.h>
#include <msg_print.h>
#include <iav_config.h>

/* ========================================================================== */
/* Timinig of standard video signals */
#define YUV525I_Y_PELS_PER_LINE			(858)
#define YUV525I_UV_PELS_PER_LINE		(YUV525I_Y_PELS_PER_LINE >> 1)
#define YUV625I_Y_PELS_PER_LINE			(864)
#define YUV625I_UV_PELS_PER_LINE		(YUV625I_Y_PELS_PER_LINE >> 1)

#define YUV720P_Y_PELS_PER_LINE			(1650)
#define YUV720P_UV_PELS_PER_LINE		(YUV720P_Y_PELS_PER_LINE >> 1)
#define YUV1080I_Y_PELS_PER_LINE		(2200)
#define YUV1080I_UV_PELS_PER_LINE		(YUV1080I_Y_PELS_PER_LINE >> 1)
#define YUV1080P_Y_PELS_PER_LINE		(2200)

#define YUV720P50_Y_PELS_PER_LINE		(1980)
#define YUV720P50_UV_PELS_PER_LINE		(YUV720P50_Y_PELS_PER_LINE >> 1)
#define YUV1080I50_Y_PELS_PER_LINE		(2640)
#define YUV1080I50_UV_PELS_PER_LINE		(YUV1080I50_Y_PELS_PER_LINE >> 1)
#define YUV1080P50_Y_PELS_PER_LINE		(2640)

/* Timinig of active video signals */
/* SD */
#define YUV525I_ACT_LINES_PER_FRM		(480)
#define YUV525I_ACT_LINES_PER_FLD		(YUV525I_ACT_LINES_PER_FRM >> 1)
#define YUV525I_DEFAULT_FRAME_RATE		AMBA_VIDEO_FPS_29_97
#define YUV480P_DEFAULT_FRAME_RATE		AMBA_VIDEO_FPS_59_94

#define YUV625I_ACT_LINES_PER_FRM		(576)
#define YUV625I_ACT_LINES_PER_FLD		(YUV625I_ACT_LINES_PER_FRM >> 1)
#define YUV625I_DEFAULT_FRAME_RATE		AMBA_VIDEO_FPS_25
#define YUV576P_DEFAULT_FRAME_RATE		AMBA_VIDEO_FPS_50

#define SD_Y_ACT_PELS_PER_LINE			(720)
#define SD_UV_ACT_PELS_PER_LINE			(SD_Y_ACT_PELS_PER_LINE >> 1)

/* 720p */
#define YUV720P_ACT_LINES_PER_FRM		(720)
#define YUV720P_DEFAULT_FRAME_RATE		AMBA_VIDEO_FPS_59_94
#define YUV720P50_DEFAULT_FRAME_RATE		AMBA_VIDEO_FPS_50

#define HD720P_Y_ACT_PELS_PER_LINE		(1280)
#define HD720P_UV_ACT_PELS_PER_LINE		(HD720P_Y_ACT_PELS_PER_LINE >> 1)

/* 1080I */
#define YUV1080I_ACT_LINES_PER_FRM		1080
#define YUV1080I_ACT_LINES_PER_FLD		(YUV1080I_ACT_LINES_PER_FRM >> 1)
#define YUV1080I_DEFAULT_FRAME_RATE		AMBA_VIDEO_FPS_29_97
#define YUV1080I50_DEFAULT_FRAME_RATE		AMBA_VIDEO_FPS_50

#define HD1080I_Y_ACT_PELS_PER_LINE		(1920)
#define HD1080I_UV_ACT_PELS_PER_LINE		(HD1080I_Y_ACT_PELS_PER_LINE >> 1)

/* 1080P */
#define YUV1080P_ACT_LINES_PER_FRM		(1080)
#define YUV1080P_DEFAULT_FRAME_RATE		AMBA_VIDEO_FPS_59_94
#define YUV1080P50_DEFAULT_FRAME_RATE		AMBA_VIDEO_FPS_50

#define HD1080P_Y_ACT_PELS_PER_LINE		(1920)
#define HD1080P_UV_ACT_PELS_PER_LINE		(HD1080P_Y_ACT_PELS_PER_LINE >> 1)


/* ========================================================================== */
#define AMBA_VOUT_TIMEOUT		(HZ / 6 + 1)

#define AMBA_VOUT_IRQ_NA		(0x00)
#define AMBA_VOUT_IRQ_WAIT		(1 << 0)
#define AMBA_VOUT_IRQ_READY		(1 << 1)

#define AMBA_VOUT_SETUP_NA		(0x00)
#define AMBA_VOUT_SETUP_VALID		(1 << 0)
#define AMBA_VOUT_SETUP_CHANGED		(1 << 1)
#define AMBA_VOUT_SETUP_NEW		(AMBA_VOUT_SETUP_VALID | AMBA_VOUT_SETUP_CHANGED)

#define VO_CLK_ONCHIP_PLL_27MHZ   	0x0    	/* Default setting */

#ifndef DRV_PRINT
#ifdef BUILD_AMBARELLA_PRIVATE_DRV_MSG
#define DRV_PRINT	print_drv
#else
#define DRV_PRINT	printk
#endif
#endif

#define vout_printk(level, format, arg...)	\
	DRV_PRINT(level format, ## arg)

#define vout_err(format, arg...)		\
	vout_printk(KERN_ERR , "Vout error: " format , ## arg)
#define vout_info(format, arg...)		\
	vout_printk(KERN_INFO , "Vout info: " format , ## arg)
#define vout_warn(format, arg...)		\
	vout_printk(KERN_WARNING , "Vout warn: " format , ## arg)
#define vout_notice(format, arg...)		\
	vout_printk(KERN_NOTICE , "Vout notice: " format , ## arg)

#ifndef CONFIG_AMBARELLA_VOUT_DEBUG
#define vout_dbg(format, arg...)
#else
#define vout_dbg(format, arg...)		\
                vout_printk(KERN_DEBUG , "Vout debug: " format , ## arg)
#endif

#if defined(CONFIG_AMBARELLA_VOUT_DEBUG)
#define vout_dump_buffer(buffer_ptr, buffer_size)	\
	do { \
		int i; \
		u32 *praw = (u32 *)buffer_ptr; \
		DRV_PRINT(KERN_DEBUG "[%d] = {\n", buffer_size); \
		for (i = 0; i < ((buffer_size + 3) >> 2); i++) {\
			DRV_PRINT(KERN_DEBUG "0x%08X,\n", praw[i]); \
		} \
		DRV_PRINT(KERN_DEBUG "}\n"); \
	} while (0)
#else
#define vout_dump_buffer(buffer_ptr, buffer_size)
#endif

#define vout_errorcode()			\
	vout_err("%s:%d = %d!\n", __func__, __LINE__, errorCode)

static inline int amba_vout_hdmi_check_i2c(struct i2c_adapter *adap)
{
	int				errorCode = 0;

	if (strcmp(adap->name, "ambarella-i2c") != 0) {
		errorCode = -ENODEV;
		goto amba_vin_check_i2c_exit;
	}

	if ((adap->class & I2C_CLASS_DDC) == 0) {
		errorCode = -EACCES;
		goto amba_vin_check_i2c_exit;
	}

amba_vin_check_i2c_exit:
	return errorCode;
}

/* ========================================================================== */
struct __amba_vout_video_source {
	int id;
	u32 source_type;
	char name[AMBA_VOUT_NAME_LENGTH];

	struct module *owner;
	void *pinfo;
	struct list_head list;

	struct mutex cmd_lock;
	int (*docmd)(struct __amba_vout_video_source *adap,
		enum amba_video_source_cmd cmd, void *args);

	int total_sink_num;
	int active_sink_id;
        enum amba_vout_sink_type active_sink_type;
        u32 freq_hz;
};

#define AMBA_HDMI_MODE_MAX   32
struct __amba_vout_video_sink {
	int id;
	int source_id;
	enum amba_vout_sink_type sink_type;
	char name[AMBA_VOUT_NAME_LENGTH];
	enum amba_vout_sink_state pstate;	//state before suspension
	enum amba_vout_sink_state state;
	enum amba_vout_sink_plug hdmi_plug;
	enum amba_video_mode hdmi_modes[AMBA_HDMI_MODE_MAX];
	enum amba_video_mode hdmi_native_mode;
	u16 hdmi_native_width;
	u16 hdmi_native_height;
	enum amba_vout_hdmi_overscan hdmi_overscan;
        enum ddd_structure hdmi_3d_structure;
        int cs;
        const amba_hdmi_video_timing_t  *vt;

	struct module *owner;
	void *pinfo;
	struct list_head list;

	struct mutex cmd_lock;
	int (*docmd)(struct __amba_vout_video_sink *src,
		enum amba_video_sink_cmd cmd, void *args);
};

/* ========================================================================== */
int amba_vout_add_video_source(struct __amba_vout_video_source *psrc);
int amba_vout_del_video_source(struct __amba_vout_video_source *psrc);

int amba_vout_add_video_sink(struct __amba_vout_video_sink *psink);
int amba_vout_del_video_sink(struct __amba_vout_video_sink *psink);

/* ========================================================================== */
void rct_set_vout_clk_src(u32 mode);
void rct_set_vout_freq_hz(u32 freq_hz);
u32 rct_get_vout_freq_hz(void);

void rct_set_vout2_clk_src(u32 mode);
void rct_set_vout2_freq_hz(u32 freq_hz);
u32 rct_get_vout2_freq_hz(void);

/* ========================================================================== */

#endif //__VOUT_PRI_H

