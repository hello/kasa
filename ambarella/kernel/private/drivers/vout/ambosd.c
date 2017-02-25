/*
 * kernel/private/drivers/ambarella/vout/osd/amb_osd/ambosd.c
 *
 * History:
 *    2009/11/03 - [Zhenwu Xue] Initial revision
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


#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fb.h>
#include <plat/fb.h>
#include <plat/ambcache.h>
#include <iav_utils.h>
#include "vout_pri.h"

#define	NUM_FB		2

typedef struct {
	u16					*pLUT;
	u8					*pOSDBuffer;
	u32					buffer_id;
} ambosd_rgb2yuv_t;

typedef struct {
	u8					*pLUT;
	u8					*pOSDBuffer;
	u16					buffer_id;
} ambosd_rgb2clut_t;

typedef struct {
	/* vout related */
	int					vout_id[2];
	int					interlace[2];
	int					cvbs_resolution[2];

	/* fb related */
	struct mutex				mtx;
	int					mtx_init;
	enum ambarella_fb_color_format		color_format;
	struct ambarella_fb_cvs_buf		conversion_buf;
	ambosd_rgb2clut_t			rgb2clut_info;
	ambosd_rgb2yuv_t			rgb565_2_uyv565_info;
	ambosd_rgb2yuv_t			argb4444_2_ayuv4444_info;
	ambosd_rgb2yuv_t			rgba4444_2_ayuv4444_info;
	ambosd_rgb2yuv_t			rgba8888_2_ayuv4444_info;
	ambosd_rgb2yuv_t			abgr8888_2_ayuv4444_info;
	int					xres, yres;
	int					cmap_changed;
	int					support_direct_mode;
	int					support_mixer_csc;
	int					csc_en;
	int					auto_copy;
} ambosd_info_t;

static ambosd_info_t ambosd_info[NUM_FB] = {
	{
		.vout_id = {-1, -1},
		.mtx_init = 0,
		.color_format = AMBFB_COLOR_UNSUPPORTED,
		.conversion_buf		= {
			.available	= -1,
			.ping_buf	= NULL,
			.ping_buf_size	= 0,
			.pong_buf	= NULL,
			.pong_buf_size	= 0,
		},
		.xres = -1,
		.yres = -1,
		.cmap_changed = 0,
		.csc_en = -1,
		.auto_copy = 0,
	},
	{
		.vout_id = {-1, -1},
		.mtx_init = 0,
		.color_format = AMBFB_COLOR_UNSUPPORTED,
		.conversion_buf		= {
			.available	= -1,
			.ping_buf	= NULL,
			.ping_buf_size	= 0,
			.pong_buf	= NULL,
			.pong_buf_size	= 0,
		},
		.xres = -1,
		.yres = -1,
		.cmap_changed = 0,
		.csc_en = -1,
		.auto_copy = 0,
	},
};

typedef struct {
	struct mutex				mtx;
	int					mtx_init;
	struct amba_video_sink_mode		sink_mode;
	int					data_valid;
} ambosd_sink_cfg_t;

static ambosd_sink_cfg_t ambosd_sink_cfg[2] = {
	{
		.mtx_init = 0,
		.data_valid = 0,
	},
	{
		.mtx_init = 0,
		.data_valid = 0,
	},
};

#include "arch/ambosd_arch.c"

/*==================Use buffer mapped to user space directly==================*/
static int amba_direct_pan_display(struct fb_var_screeninfo *var,
	struct fb_info *info)
{
	ambosd_info_t			*posd_info;
	u8				*pSrcOSDBuffer, *pDstOSDBuffer;
	int				errorCode;

	posd_info = &ambosd_info[info->node];

	mutex_lock(&posd_info->mtx);

	pSrcOSDBuffer = info->screen_base +
		info->fix.line_length * var->yoffset;

	pDstOSDBuffer = pSrcOSDBuffer;
       if(!posd_info->conversion_buf.available)
	       clean_d_cache(pDstOSDBuffer, info->fix.line_length * var->yres);

	errorCode = amba_direct_pan_display_arch(info, posd_info, pDstOSDBuffer);

	if (posd_info->auto_copy) {
		pSrcOSDBuffer = info->screen_base + info->fix.line_length * var->yoffset;
		if (var->yoffset) {
			pDstOSDBuffer = info->screen_base;
		} else {
			pDstOSDBuffer = info->screen_base + info->fix.line_length * var->yres;
		}
		fb_memcpy(pDstOSDBuffer, pSrcOSDBuffer, info->fix.line_length * var->yres);
	}

	mutex_unlock(&posd_info->mtx);

	return errorCode;
}

/*===================================rgb2clut=================================*/
static int amba_rgb2clut_init(ambosd_info_t *posd_info,
	u32 pitch, u32 height)
{
	ambosd_rgb2clut_t		*prgb2clut;
	u32				index, size, color_depth;
	u32				r, g, b;
	u8				**ppLUT, **ppBuffer;
	int				errorCode = 0;

	prgb2clut = &posd_info->rgb2clut_info;
	ppLUT = (u8 **)&prgb2clut->pLUT;
	ppBuffer = (u8 **)&prgb2clut->pOSDBuffer;

	if (*ppLUT) {
		kfree(*ppLUT);
		*ppLUT = NULL;
	}

	if (*ppBuffer) {
		kfree(*ppBuffer);
		*ppBuffer = NULL;
	}

	/* Malloc OSD Buffer */
	if (!posd_info->conversion_buf.available) {
		size = 2 * pitch * height;
		if (size == 0) {
			errorCode = -EINVAL;
			goto amba_rgb2clut_init_exit;
		}

		*ppBuffer = kzalloc(size, GFP_KERNEL);
		if (!(*ppBuffer)) {
			vout_err("%s: pOSDBuffer out of memory!\n", __func__);
			errorCode = -ENOMEM;
			goto amba_rgb2clut_init_exit;
		}
	}

	/* Malloc LUT Buffer */
	color_depth = (0x1 << 16);
	*ppLUT = kzalloc(color_depth, GFP_KERNEL);
	if (!(*ppLUT)) {
		vout_err("%s: pLUT out of memory!\n", __func__);
		kfree(*ppBuffer);
		*ppBuffer = NULL;
		errorCode = -ENOMEM;
		goto amba_rgb2clut_init_exit;
	}

	/* Fill LUT */
	for (index = 0; index < color_depth; index++) {
		r = (index & 0xF800) >> 8;
		g = (index & 0x07E0) >> 3;
		b = (index & 0x001F) << 3;

		(*ppLUT)[index] = (300 * r + 600 * g + 110 * b) >> 10;
		if (!(*ppLUT)[index])
			(*ppLUT)[index] = 1;
	}

amba_rgb2clut_init_exit:
	return errorCode;
}

static void amba_convert_rgb2clut(ambosd_info_t *posd_info,
	u16 *pSrc, u8 *pDest, u32 pitch, u32 xres, u32 yres)
{
	u32				tmp_Src, tmp_Dest;
	u32				*ptmp_Src, *ptmp_Dest;
	u32				src_pitch, dest_pitch;
	u32				i, j, k;
	ambosd_rgb2clut_t		*prgb2clut;

	prgb2clut = &posd_info->rgb2clut_info;
	ptmp_Src = (u32 *)pSrc;
	ptmp_Dest = (u32 *)pDest;
	src_pitch = pitch >> 2;
	dest_pitch = pitch >> 3;
	xres >>= 2;

	for (j = 0; j < yres; j++) {
		for (i = 0; i < xres; i++) {
			k = (i << 1) + 1;
			tmp_Src = ptmp_Src[k--];
			tmp_Dest = prgb2clut->pLUT[tmp_Src >> 16];
			tmp_Dest <<= 8;
			tmp_Dest |= prgb2clut->pLUT[tmp_Src & 0xFFFF];
			tmp_Dest <<= 8;
			tmp_Src = ptmp_Src[k];
			tmp_Dest |= prgb2clut->pLUT[tmp_Src >> 16];
			tmp_Dest <<= 8;
			tmp_Dest |= prgb2clut->pLUT[tmp_Src & 0xFFFF];

			ptmp_Dest[i] = tmp_Dest;
		}
		ptmp_Src += src_pitch;
		ptmp_Dest += dest_pitch;
	}
}

static int amba_rgb2clut_pan_display(struct fb_var_screeninfo *var,
	struct fb_info *info)
{
	int				errorCode = 0;
	u16				*pSrcOSDBuffer;
	u8				*pDestOSDBuffer;
	ambosd_info_t			*posd_info;
	ambosd_rgb2clut_t		*prgb2clut;

	posd_info = &ambosd_info[info->node];
	prgb2clut = &posd_info->rgb2clut_info;

	mutex_lock(&posd_info->mtx);

	pSrcOSDBuffer = (u16 *)(info->screen_base +
		info->fix.line_length * var->yoffset);


	prgb2clut->buffer_id++;
	prgb2clut->buffer_id &= 0x1;
	if (!posd_info->conversion_buf.available) {
		pDestOSDBuffer = prgb2clut->pOSDBuffer +
			var->yres * prgb2clut->buffer_id * (info->fix.line_length >> 1);
	} else {
		if (!prgb2clut->buffer_id) {
			pDestOSDBuffer = posd_info->conversion_buf.ping_buf;
		} else {
			pDestOSDBuffer = posd_info->conversion_buf.pong_buf;
		}
	}

	amba_convert_rgb2clut(posd_info, pSrcOSDBuffer,
		pDestOSDBuffer, info->fix.line_length,
		info->var.xres, info->var.yres);

	clean_d_cache(pDestOSDBuffer, info->fix.line_length * var->yres);
	amba_rgb2clut_pan_display_arch(info, posd_info, pDestOSDBuffer);

	mutex_unlock(&posd_info->mtx);
	return errorCode;
}

/*=============================rgb565_2_uyv565================================*/
static int amba_rgb565_2_uyv565_init(ambosd_info_t *posd_info,
	u32 pitch, u32 yres)
{
	ambosd_rgb2yuv_t		*prgb2yuv;
	u16				**ppLUT;
	u8				**ppOSDBuffer;
	u32				index;
	u8				r, g, b;
	u8				y, u, v;
	int				errorCode = 0;

	prgb2yuv = &posd_info->rgb565_2_uyv565_info;
	ppLUT = &prgb2yuv->pLUT;
	ppOSDBuffer = (u8 **)&prgb2yuv->pOSDBuffer;

	if (*ppLUT) {
		kfree(*ppLUT);
		*ppLUT = NULL;
	}

	*ppLUT = kmalloc((0x1 << 16) * sizeof(u16), GFP_KERNEL);
	if (!(*ppLUT)) {
		vout_err("%s: pLUT out of memory!\n", __func__);
		errorCode = -ENOMEM;
		goto amba_rgb565_2_uyv565_init_exit;
	}

	if (*ppOSDBuffer) {
		kfree(*ppOSDBuffer);
		*ppOSDBuffer = NULL;
	}

	if (!posd_info->conversion_buf.available) {
		*ppOSDBuffer = kzalloc(pitch * yres * 2, GFP_KERNEL);
		if (!(*ppOSDBuffer)) {
			vout_err("%s: pOSDBuffer out of memory(pitch = %d, "
				"yres = %d)!\n", __func__, pitch, yres);
			errorCode = -ENOMEM;
			goto amba_rgb565_2_uyv565_init_exit;
		}
	}

	for (index = 0; index < (0x1 << 16); index++) {
		r = (index & 0xF800) >> 8;
		g = (index & 0x07E0) >> 3;
		b = (index & 0x001F) << 3;

		y = (( 66 * r + 129 * g +  25 * b + 128) >> 8) +  16;
		u = ((-38 * r -  74 * g + 112 * b + 128) >> 8) + 128;
		v = ((112 * r -  94 * g -  18 * b + 128) >> 8) + 128;

		y >>= 2;
		u >>= 3;
		v >>= 3;

		(*ppLUT)[index] = ((u << 11) | (y << 5) | (v));
	}

amba_rgb565_2_uyv565_init_exit:
	return errorCode;
}

static void amba_convert_rgb565_2_uyv565(ambosd_info_t *posd_info,
	u16 *pSrc, u16 *pDst,
	u32 pitch, u32 xres, u32 yres)
{
	u32				i, size;
	u32				*prgb, *puyv;
	u32				rgb32, uyv32;
	u16				*pLUT;

	prgb = (u32 *)pSrc;
	puyv = (u32 *)pDst;
	pLUT = posd_info->rgb565_2_uyv565_info.pLUT;
	size = (pitch * yres) >> 4;

	for (i = 0; i < size; i++) {
		rgb32 = *prgb++;
		uyv32 = pLUT[rgb32 >> 16] << 16;
		uyv32 |= pLUT[(rgb32 << 16) >> 16];
		*puyv++ = uyv32;

		rgb32 = *prgb++;
		uyv32 = pLUT[rgb32 >> 16] << 16;
		uyv32 |= pLUT[(rgb32 << 16) >> 16];
		*puyv++ = uyv32;

		rgb32 = *prgb++;
		uyv32 = pLUT[rgb32 >> 16] << 16;
		uyv32 |= pLUT[(rgb32 << 16) >> 16];
		*puyv++ = uyv32;

		rgb32 = *prgb++;
		uyv32 = pLUT[rgb32 >> 16] << 16;
		uyv32 |= pLUT[(rgb32 << 16) >> 16];
		*puyv++ = uyv32;
	}

}

static int amba_rgb565_2_uyv565_pan_display(struct fb_var_screeninfo *var,
	struct fb_info *info)
{
	ambosd_info_t			*posd_info;
	ambosd_rgb2yuv_t		*prgb2yuv;
	int				errorCode = 0;
	u16				*pSrcOSDBuffer, *pDstOSDBuffer = NULL;

	posd_info = &ambosd_info[info->node];
	prgb2yuv = &posd_info->rgb565_2_uyv565_info;

	mutex_lock(&posd_info->mtx);

	prgb2yuv->buffer_id++;
	prgb2yuv->buffer_id &= 0x1;
	pSrcOSDBuffer = (u16 *)(info->screen_base +
		info->fix.line_length * var->yoffset);

	if (!posd_info->conversion_buf.available) {
		pDstOSDBuffer = (u16 *)(prgb2yuv->pOSDBuffer +
			var->yres * prgb2yuv->buffer_id * info->fix.line_length);
	} else {
		if (!prgb2yuv->buffer_id) {
			pDstOSDBuffer =
				(u16 *)(posd_info->conversion_buf.ping_buf);
		} else {
			pDstOSDBuffer =
				(u16 *)(posd_info->conversion_buf.pong_buf);
		}
	}

	amba_convert_rgb565_2_uyv565(posd_info, pSrcOSDBuffer, pDstOSDBuffer,
		info->fix.line_length, info->var.xres, info->var.yres);

	if (!pDstOSDBuffer) {
		errorCode = -1;
		goto amba_rgb565_2_uyv565_pan_display_exit;
	}

       if(!posd_info->conversion_buf.available)
	       clean_d_cache(pDstOSDBuffer, info->fix.line_length * var->yres);

	errorCode = amba_rgb2yuv_pan_display_arch(info, posd_info, pDstOSDBuffer);

amba_rgb565_2_uyv565_pan_display_exit:
	mutex_unlock(&posd_info->mtx);
	return errorCode;
}

/*===========================argb4444_2_ayuv4444==============================*/
static int amba_argb4444_2_ayuv4444_init(ambosd_info_t *posd_info,
	u32 pitch, u32 yres)
{
	ambosd_rgb2yuv_t		*prgb2yuv;
	u16				**ppLUT;
	u8				**ppOSDBuffer;
	u32				index;
	u8				a, r, g, b;
	u8				y, u, v;
	int				errorCode = 0;

	prgb2yuv = &posd_info->argb4444_2_ayuv4444_info;
	ppLUT = &prgb2yuv->pLUT;
	ppOSDBuffer = (u8 **)&prgb2yuv->pOSDBuffer;

	if (*ppLUT) {
		kfree(*ppLUT);
		*ppLUT = NULL;
	}

	*ppLUT = kmalloc((0x1 << 16) * sizeof(u16), GFP_KERNEL);
	if (!(*ppLUT)) {
		vout_err("%s: pLUT out of memory!\n", __func__);
		errorCode = -ENOMEM;
		goto amba_argb4444_2_ayuv4444_init_exit;
	}

	if (*ppOSDBuffer) {
		kfree(*ppOSDBuffer);
		*ppOSDBuffer = NULL;
	}

	if (!posd_info->conversion_buf.available) {
		*ppOSDBuffer = kzalloc(pitch * yres * 2, GFP_KERNEL);
		if (!(*ppOSDBuffer)) {
			vout_err("%s: pOSDBuffer out of memory(pitch = %d, "
				"yres = %d)!\n", __func__, pitch, yres);
			errorCode = -ENOMEM;
			goto amba_argb4444_2_ayuv4444_init_exit;
		}
	}

	for (index = 0; index < (0x1 << 16); index++) {
		a = (index & 0xF000) >> 12;
		r = (index & 0x0F00) >> 4;
		g = (index & 0x00F0) >> 0;
		b = (index & 0x000F) << 4;

		y = (( 66 * r + 129 * g +  25 * b + 128) >> 8) +  16;
		u = ((-38 * r -  74 * g + 112 * b + 128) >> 8) + 128;
		v = ((112 * r -  94 * g -  18 * b + 128) >> 8) + 128;

		y >>= 4;
		u >>= 4;
		v >>= 4;

		(*ppLUT)[index] = ((a << 12) | (y << 8) | (u << 4) | (v << 0));
	}

amba_argb4444_2_ayuv4444_init_exit:
	return errorCode;
}

static void amba_convert_argb4444_2_ayuv4444(ambosd_info_t *posd_info,
	u16 *pSrc, u16 *pDst,
	u32 pitch, u32 xres, u32 yres)
{
	u32				i, size;
	u32				*prgb, *puyv;
	u32				rgb32, uyv32;
	u16				*pLUT;

	prgb = (u32 *)pSrc;
	puyv = (u32 *)pDst;
	pLUT = posd_info->argb4444_2_ayuv4444_info.pLUT;
	size = (pitch * yres) >> 4;

	for (i = 0; i < size; i++) {
		rgb32 = *prgb++;
		uyv32 = pLUT[rgb32 >> 16] << 16;
		uyv32 |= pLUT[(rgb32 << 16) >> 16];
		*puyv++ = uyv32;

		rgb32 = *prgb++;
		uyv32 = pLUT[rgb32 >> 16] << 16;
		uyv32 |= pLUT[(rgb32 << 16) >> 16];
		*puyv++ = uyv32;

		rgb32 = *prgb++;
		uyv32 = pLUT[rgb32 >> 16] << 16;
		uyv32 |= pLUT[(rgb32 << 16) >> 16];
		*puyv++ = uyv32;

		rgb32 = *prgb++;
		uyv32 = pLUT[rgb32 >> 16] << 16;
		uyv32 |= pLUT[(rgb32 << 16) >> 16];
		*puyv++ = uyv32;
	}

}

static int amba_argb4444_2_ayuv4444_pan_display(struct fb_var_screeninfo *var,
	struct fb_info *info)
{
	ambosd_info_t			*posd_info;
	ambosd_rgb2yuv_t		*prgb2yuv;
	int				errorCode = 0;
	u16				*pSrcOSDBuffer, *pDstOSDBuffer = NULL;

	posd_info = &ambosd_info[info->node];
	prgb2yuv = &posd_info->argb4444_2_ayuv4444_info;

	mutex_lock(&posd_info->mtx);

	prgb2yuv->buffer_id++;
	prgb2yuv->buffer_id &= 0x1;
	pSrcOSDBuffer = (u16 *)(info->screen_base +
		info->fix.line_length * var->yoffset);
	if (!posd_info->conversion_buf.available) {
		pDstOSDBuffer = (u16 *)(prgb2yuv->pOSDBuffer +
			var->yres * prgb2yuv->buffer_id * info->fix.line_length);
	} else {
		if (!prgb2yuv->buffer_id) {
			pDstOSDBuffer =
				(u16 *)(posd_info->conversion_buf.ping_buf);
		} else {
			pDstOSDBuffer =
				(u16 *)(posd_info->conversion_buf.pong_buf);
		}
	}

	amba_convert_argb4444_2_ayuv4444(posd_info, pSrcOSDBuffer, pDstOSDBuffer,
		info->fix.line_length, info->var.xres, info->var.yres);

	if (!pDstOSDBuffer) {
		errorCode = -1;
		goto amba_argb4444_2_ayuv4444_pan_display_exit;
	}

       if(!posd_info->conversion_buf.available)
	       clean_d_cache(pDstOSDBuffer, info->fix.line_length * var->yres);
	errorCode = amba_rgb2yuv_pan_display_arch(info, posd_info, pDstOSDBuffer);

amba_argb4444_2_ayuv4444_pan_display_exit:
	mutex_unlock(&posd_info->mtx);
	return errorCode;
}


/*===========================rgba4444_2_ayuv4444==============================*/
static int amba_rgba4444_2_ayuv4444_init(ambosd_info_t *posd_info,
	u32 pitch, u32 yres)
{
	ambosd_rgb2yuv_t		*prgb2yuv;
	u16				**ppLUT;
	u8				**ppOSDBuffer;
	u32				index;
	u8				a, r, g, b;
	u8				y, u, v;
	int				errorCode = 0;

	prgb2yuv = &posd_info->rgba4444_2_ayuv4444_info;
	ppLUT = &prgb2yuv->pLUT;
	ppOSDBuffer = (u8 **)&prgb2yuv->pOSDBuffer;

	if (*ppLUT) {
		kfree(*ppLUT);
		*ppLUT = NULL;
	}

	*ppLUT = kmalloc((0x1 << 16) * sizeof(u16), GFP_KERNEL);
	if (!(*ppLUT)) {
		vout_err("%s: pLUT out of memory!\n", __func__);
		errorCode = -ENOMEM;
		goto amba_rgba4444_2_ayuv4444_init_exit;
	}

	if (*ppOSDBuffer) {
		kfree(*ppOSDBuffer);
		*ppOSDBuffer = NULL;
	}

	if (!posd_info->conversion_buf.available) {
		*ppOSDBuffer = kzalloc(pitch * yres * 2, GFP_KERNEL);
		if (!(*ppOSDBuffer)) {
			vout_err("%s: pOSDBuffer out of memory(pitch = %d, "
				"yres = %d)!\n", __func__, pitch, yres);
			errorCode = -ENOMEM;
			goto amba_rgba4444_2_ayuv4444_init_exit;
		}
	}

	for (index = 0; index < (0x1 << 16); index++) {
		r = (index & 0xF000) >> 8;
		g = (index & 0x0F00) >> 4;
		b = (index & 0x00F0) >> 0;
		a = (index & 0x000F) >> 0;

		y = (( 66 * r + 129 * g +  25 * b + 128) >> 8) +  16;
		u = ((-38 * r -  74 * g + 112 * b + 128) >> 8) + 128;
		v = ((112 * r -  94 * g -  18 * b + 128) >> 8) + 128;

		y >>= 4;
		u >>= 4;
		v >>= 4;

		(*ppLUT)[index] = ((a << 12) | (y << 8) | (u << 4) | (v << 0));
	}

amba_rgba4444_2_ayuv4444_init_exit:
	return errorCode;
}

static void amba_convert_rgba4444_2_ayuv4444(ambosd_info_t *posd_info,
	u16 *pSrc, u16 *pDst,
	u32 pitch, u32 xres, u32 yres)
{
	u32				i, size;
	u32				*prgb, *puyv;
	u32				rgb32, uyv32;
	u16				*pLUT;

	prgb = (u32 *)pSrc;
	puyv = (u32 *)pDst;
	pLUT = posd_info->rgba4444_2_ayuv4444_info.pLUT;
	size = (pitch * yres) >> 4;

	for (i = 0; i < size; i++) {
		rgb32 = *prgb++;
		uyv32 = pLUT[rgb32 >> 16] << 16;
		uyv32 |= pLUT[(rgb32 << 16) >> 16];
		*puyv++ = uyv32;

		rgb32 = *prgb++;
		uyv32 = pLUT[rgb32 >> 16] << 16;
		uyv32 |= pLUT[(rgb32 << 16) >> 16];
		*puyv++ = uyv32;

		rgb32 = *prgb++;
		uyv32 = pLUT[rgb32 >> 16] << 16;
		uyv32 |= pLUT[(rgb32 << 16) >> 16];
		*puyv++ = uyv32;

		rgb32 = *prgb++;
		uyv32 = pLUT[rgb32 >> 16] << 16;
		uyv32 |= pLUT[(rgb32 << 16) >> 16];
		*puyv++ = uyv32;
	}

}

static int amba_rgba4444_2_ayuv4444_pan_display(struct fb_var_screeninfo *var,
	struct fb_info *info)
{
	ambosd_info_t			*posd_info;
	ambosd_rgb2yuv_t		*prgb2yuv;
	int				errorCode = 0;
	u16				*pSrcOSDBuffer, *pDstOSDBuffer = NULL;

	posd_info = &ambosd_info[info->node];
	prgb2yuv = &posd_info->rgba4444_2_ayuv4444_info;

	mutex_lock(&posd_info->mtx);

	prgb2yuv->buffer_id++;
	prgb2yuv->buffer_id &= 0x1;
	pSrcOSDBuffer = (u16 *)(info->screen_base +
		info->fix.line_length * var->yoffset);
	if (!posd_info->conversion_buf.available) {
		pDstOSDBuffer = (u16 *)(prgb2yuv->pOSDBuffer +
			var->yres * prgb2yuv->buffer_id * info->fix.line_length);
	} else {
		if (!prgb2yuv->buffer_id) {
			pDstOSDBuffer =
				(u16 *)(posd_info->conversion_buf.ping_buf);
		} else {
			pDstOSDBuffer =
				(u16 *)(posd_info->conversion_buf.pong_buf);
		}
	}

	amba_convert_rgba4444_2_ayuv4444(posd_info, pSrcOSDBuffer, pDstOSDBuffer,
		info->fix.line_length, info->var.xres, info->var.yres);

	if (!pDstOSDBuffer) {
		errorCode = -1;
		goto amba_rgba4444_2_ayuv4444_pan_display_exit;
	}

       if(!posd_info->conversion_buf.available)
	       clean_d_cache(pDstOSDBuffer, info->fix.line_length * var->yres);
	errorCode = amba_rgb2yuv_pan_display_arch(info, posd_info, pDstOSDBuffer);

amba_rgba4444_2_ayuv4444_pan_display_exit:
	mutex_unlock(&posd_info->mtx);
	return errorCode;
}

/*===========================rgba8888_2_ayuv4444==============================*/
static int amba_rgba8888_2_ayuv4444_init(ambosd_info_t *posd_info,
	u32 pitch, u32 yres)
{
	ambosd_rgb2yuv_t		*prgb2yuv;
	u16				**ppLUT;
	u8				**ppOSDBuffer;
	u32				index;
	u8				a, r, g, b;
	u8				y, u, v;
	int				errorCode = 0;

	prgb2yuv = &posd_info->rgba8888_2_ayuv4444_info;
	ppLUT = &prgb2yuv->pLUT;
	ppOSDBuffer = (u8 **)&prgb2yuv->pOSDBuffer;

	if (*ppLUT) {
		kfree(*ppLUT);
		*ppLUT = NULL;
	}

	*ppLUT = kmalloc((0x1 << 16) * sizeof(u16), GFP_KERNEL);
	if (!(*ppLUT)) {
		vout_err("%s: pLUT out of memory!\n", __func__);
		errorCode = -ENOMEM;
		goto amba_rgba8888_2_ayuv4444_init_exit;
	}

	if (*ppOSDBuffer) {
		kfree(*ppOSDBuffer);
		*ppOSDBuffer = NULL;
	}

	if (!posd_info->conversion_buf.available) {
		*ppOSDBuffer = kzalloc(pitch * yres * 2, GFP_KERNEL);
		if (!(*ppOSDBuffer)) {
			vout_err("%s: pOSDBuffer out of memory(pitch = %d, "
				"yres = %d)!\n", __func__, pitch, yres);
			errorCode = -ENOMEM;
			goto amba_rgba8888_2_ayuv4444_init_exit;
		}
	}

	for (index = 0; index < (0x1 << 16); index++) {
		r = (index & 0xF000) >> 8;
		g = (index & 0x0F00) >> 4;
		b = (index & 0x00F0) >> 0;
		a = (index & 0x000F) >> 0;

		y = (( 66 * r + 129 * g +  25 * b + 128) >> 8) +  16;
		u = ((-38 * r -  74 * g + 112 * b + 128) >> 8) + 128;
		v = ((112 * r -  94 * g -  18 * b + 128) >> 8) + 128;

		y >>= 4;
		u >>= 4;
		v >>= 4;

		(*ppLUT)[index] = ((a << 12) | (y << 8) | (u << 4) | (v << 0));
	}

amba_rgba8888_2_ayuv4444_init_exit:
	return errorCode;
}

static void amba_convert_rgba8888_2_ayuv4444(ambosd_info_t *posd_info,
	u16 *pSrc, u16 *pDst,
	u32 pitch, u32 xres, u32 yres)
{
	u32				i, size;
	u32				*prgb, *puyv;
	u32				rgb32, uyv32;
	u16				rgb16, *pLUT;

	prgb = (u32 *)pSrc;
	puyv = (u32 *)pDst;
	pLUT = posd_info->rgba8888_2_ayuv4444_info.pLUT;
	size = (pitch * yres) >> 5;

	for (i = 0; i < size; i++) {
		rgb32 = *prgb++;
		rgb16 = ((rgb32 & 0xf0000000) >> 16) |
			((rgb32 & 0x00f00000) >> 12) |
			((rgb32 & 0x0000f000) >> 8) |
			((rgb32 & 0x000000f0) >> 4);
		uyv32 = pLUT[rgb16] << 16;

		rgb32 = *prgb++;
		rgb16 = ((rgb32 & 0xf0000000) >> 16) |
			((rgb32 & 0x00f00000) >> 12) |
			((rgb32 & 0x0000f000) >> 8) |
			((rgb32 & 0x000000f0) >> 4);
		uyv32 |= pLUT[rgb16];
		*puyv++ = uyv32;

		rgb32 = *prgb++;
		rgb16 = ((rgb32 & 0xf0000000) >> 16) |
			((rgb32 & 0x00f00000) >> 12) |
			((rgb32 & 0x0000f000) >> 8) |
			((rgb32 & 0x000000f0) >> 4);
		uyv32 = pLUT[rgb16] << 16;

		rgb32 = *prgb++;
		rgb16 = ((rgb32 & 0xf0000000) >> 16) |
			((rgb32 & 0x00f00000) >> 12) |
			((rgb32 & 0x0000f000) >> 8) |
			((rgb32 & 0x000000f0) >> 4);
		uyv32 |= pLUT[rgb16];
		*puyv++ = uyv32;

		rgb32 = *prgb++;
		rgb16 = ((rgb32 & 0xf0000000) >> 16) |
			((rgb32 & 0x00f00000) >> 12) |
			((rgb32 & 0x0000f000) >> 8) |
			((rgb32 & 0x000000f0) >> 4);
		uyv32 = pLUT[rgb16] << 16;

		rgb32 = *prgb++;
		rgb16 = ((rgb32 & 0xf0000000) >> 16) |
			((rgb32 & 0x00f00000) >> 12) |
			((rgb32 & 0x0000f000) >> 8) |
			((rgb32 & 0x000000f0) >> 4);
		uyv32 |= pLUT[rgb16];
		*puyv++ = uyv32;

		rgb32 = *prgb++;
		rgb16 = ((rgb32 & 0xf0000000) >> 16) |
			((rgb32 & 0x00f00000) >> 12) |
			((rgb32 & 0x0000f000) >> 8) |
			((rgb32 & 0x000000f0) >> 4);
		uyv32 = pLUT[rgb16] << 16;

		rgb32 = *prgb++;
		rgb16 = ((rgb32 & 0xf0000000) >> 16) |
			((rgb32 & 0x00f00000) >> 12) |
			((rgb32 & 0x0000f000) >> 8) |
			((rgb32 & 0x000000f0) >> 4);
		uyv32 |= pLUT[rgb16];
		*puyv++ = uyv32;
	}

}

static int amba_rgba8888_2_ayuv4444_pan_display(struct fb_var_screeninfo *var,
	struct fb_info *info)
{
	ambosd_info_t			*posd_info;
	ambosd_rgb2yuv_t		*prgb2yuv;
	int				errorCode = 0;
	u16				*pSrcOSDBuffer, *pDstOSDBuffer = NULL;

	posd_info = &ambosd_info[info->node];
	prgb2yuv = &posd_info->rgba8888_2_ayuv4444_info;

	mutex_lock(&posd_info->mtx);

	prgb2yuv->buffer_id++;
	prgb2yuv->buffer_id &= 0x1;
	pSrcOSDBuffer = (u16 *)(info->screen_base +
		info->fix.line_length * var->yoffset);
	if (!posd_info->conversion_buf.available) {
		pDstOSDBuffer = (u16 *)(prgb2yuv->pOSDBuffer +
			var->yres * prgb2yuv->buffer_id * info->fix.line_length);
	} else {
		if (!prgb2yuv->buffer_id) {
			pDstOSDBuffer =
				(u16 *)(posd_info->conversion_buf.ping_buf);
		} else {
			pDstOSDBuffer =
				(u16 *)(posd_info->conversion_buf.pong_buf);
		}
	}

	amba_convert_rgba8888_2_ayuv4444(posd_info, pSrcOSDBuffer, pDstOSDBuffer,
		info->fix.line_length, info->var.xres, info->var.yres);

	if (!pDstOSDBuffer) {
		errorCode = -1;
		goto amba_rgba8888_2_ayuv4444_pan_display_exit;
	}

       if(!posd_info->conversion_buf.available)
	       clean_d_cache(pDstOSDBuffer, info->fix.line_length * var->yres);
	errorCode = amba_rgb2yuv_pan_display_arch(info, posd_info, pDstOSDBuffer);

amba_rgba8888_2_ayuv4444_pan_display_exit:
	mutex_unlock(&posd_info->mtx);
	return errorCode;
}

/*===========================abgr8888_2_ayuv4444==============================*/
static int amba_abgr8888_2_ayuv4444_init(ambosd_info_t *posd_info,
	u32 pitch, u32 yres)
{
	ambosd_rgb2yuv_t		*prgb2yuv;
	u16				**ppLUT;
	u8				**ppOSDBuffer;
	u32				index;
	u8				a, r, g, b;
	u8				y, u, v;
	int				errorCode = 0;

	prgb2yuv = &posd_info->abgr8888_2_ayuv4444_info;
	ppLUT = &prgb2yuv->pLUT;
	ppOSDBuffer = (u8 **)&prgb2yuv->pOSDBuffer;

	if (*ppLUT) {
		kfree(*ppLUT);
		*ppLUT = NULL;
	}

	*ppLUT = kmalloc((0x1 << 16) * sizeof(u16), GFP_KERNEL);
	if (!(*ppLUT)) {
		vout_err("%s: pLUT out of memory!\n", __func__);
		errorCode = -ENOMEM;
		goto amba_abgr8888_2_ayuv4444_init_exit;
	}

	if (*ppOSDBuffer) {
		kfree(*ppOSDBuffer);
		*ppOSDBuffer = NULL;
	}

	if (!posd_info->conversion_buf.available) {
		*ppOSDBuffer = kzalloc(pitch * yres * 2, GFP_KERNEL);
		if (!(*ppOSDBuffer)) {
			vout_err("%s: pOSDBuffer out of memory(pitch = %d, "
				"yres = %d)!\n", __func__, pitch, yres);
			errorCode = -ENOMEM;
			goto amba_abgr8888_2_ayuv4444_init_exit;
		}
	}

	for (index = 0; index < (0x1 << 16); index++) {
		a = (index & 0xF000) >> 12;
		b = (index & 0x0F00) >> 4;
		g = (index & 0x00F0) >> 0;
		r = (index & 0x000F) << 4;

		y = (( 66 * r + 129 * g +  25 * b + 128) >> 8) +  16;
		u = ((-38 * r -  74 * g + 112 * b + 128) >> 8) + 128;
		v = ((112 * r -  94 * g -  18 * b + 128) >> 8) + 128;

		y >>= 4;
		u >>= 4;
		v >>= 4;

		(*ppLUT)[index] = ((a << 12) | (y << 8) | (u << 4) | (v << 0));
	}

amba_abgr8888_2_ayuv4444_init_exit:
	return errorCode;
}

static void amba_convert_abgr8888_2_ayuv4444(ambosd_info_t *posd_info,
	u16 *pSrc, u16 *pDst,
	u32 pitch, u32 xres, u32 yres)
{
	u32				i, size;
	u32				*prgb, *puyv;
	u32				rgb32, uyv32;
	u16				rgb16, *pLUT;

	prgb = (u32 *)pSrc;
	puyv = (u32 *)pDst;
	pLUT = posd_info->abgr8888_2_ayuv4444_info.pLUT;
	size = (pitch * yres) >> 5;

	for (i = 0; i < size; i++) {
		rgb32 = *prgb++;
		rgb16 = ((rgb32 & 0xf0000000) >> 16) |
			((rgb32 & 0x00f00000) >> 12) |
			((rgb32 & 0x0000f000) >> 8) |
			((rgb32 & 0x000000f0) >> 4);
		uyv32 = pLUT[rgb16];

		rgb32 = *prgb++;
		rgb16 = ((rgb32 & 0xf0000000) >> 16) |
			((rgb32 & 0x00f00000) >> 12) |
			((rgb32 & 0x0000f000) >> 8) |
			((rgb32 & 0x000000f0) >> 4);
		uyv32 |= (pLUT[rgb16] << 16);
		*puyv++ = uyv32;

		rgb32 = *prgb++;
		rgb16 = ((rgb32 & 0xf0000000) >> 16) |
			((rgb32 & 0x00f00000) >> 12) |
			((rgb32 & 0x0000f000) >> 8) |
			((rgb32 & 0x000000f0) >> 4);
		uyv32 = pLUT[rgb16];

		rgb32 = *prgb++;
		rgb16 = ((rgb32 & 0xf0000000) >> 16) |
			((rgb32 & 0x00f00000) >> 12) |
			((rgb32 & 0x0000f000) >> 8) |
			((rgb32 & 0x000000f0) >> 4);
		uyv32 |= (pLUT[rgb16] << 16);
		*puyv++ = uyv32;

		rgb32 = *prgb++;
		rgb16 = ((rgb32 & 0xf0000000) >> 16) |
			((rgb32 & 0x00f00000) >> 12) |
			((rgb32 & 0x0000f000) >> 8) |
			((rgb32 & 0x000000f0) >> 4);
		uyv32 = pLUT[rgb16];

		rgb32 = *prgb++;
		rgb16 = ((rgb32 & 0xf0000000) >> 16) |
			((rgb32 & 0x00f00000) >> 12) |
			((rgb32 & 0x0000f000) >> 8) |
			((rgb32 & 0x000000f0) >> 4);
		uyv32 |= (pLUT[rgb16] << 16);
		*puyv++ = uyv32;

		rgb32 = *prgb++;
		rgb16 = ((rgb32 & 0xf0000000) >> 16) |
			((rgb32 & 0x00f00000) >> 12) |
			((rgb32 & 0x0000f000) >> 8) |
			((rgb32 & 0x000000f0) >> 4);
		uyv32 = pLUT[rgb16];

		rgb32 = *prgb++;
		rgb16 = ((rgb32 & 0xf0000000) >> 16) |
			((rgb32 & 0x00f00000) >> 12) |
			((rgb32 & 0x0000f000) >> 8) |
			((rgb32 & 0x000000f0) >> 4);
		uyv32 |= (pLUT[rgb16] << 16);
		*puyv++ = uyv32;
	}

}

static int amba_abgr8888_2_ayuv4444_pan_display(struct fb_var_screeninfo *var,
	struct fb_info *info)
{
	ambosd_info_t			*posd_info;
	ambosd_rgb2yuv_t		*prgb2yuv;
	int				errorCode = 0;
	u16				*pSrcOSDBuffer, *pDstOSDBuffer = NULL;

	posd_info = &ambosd_info[info->node];
	prgb2yuv = &posd_info->abgr8888_2_ayuv4444_info;

	mutex_lock(&posd_info->mtx);

	prgb2yuv->buffer_id++;
	prgb2yuv->buffer_id &= 0x1;
	pSrcOSDBuffer = (u16 *)(info->screen_base +
		info->fix.line_length * var->yoffset);
	if (!posd_info->conversion_buf.available) {
		pDstOSDBuffer = (u16 *)(prgb2yuv->pOSDBuffer +
			var->yres * prgb2yuv->buffer_id * info->fix.line_length);
	} else {
		if (!prgb2yuv->buffer_id) {
			pDstOSDBuffer =
				(u16 *)(posd_info->conversion_buf.ping_buf);
		} else {
			pDstOSDBuffer =
				(u16 *)(posd_info->conversion_buf.pong_buf);
		}
	}

	amba_convert_abgr8888_2_ayuv4444(posd_info, pSrcOSDBuffer, pDstOSDBuffer,
		info->fix.line_length, info->var.xres, info->var.yres);

	if (!pDstOSDBuffer) {
		errorCode = -1;
		goto amba_abgr8888_2_ayuv4444_pan_display_exit;
	}

       if(!posd_info->conversion_buf.available)
	       clean_d_cache(pDstOSDBuffer, info->fix.line_length * var->yres);
	errorCode = amba_rgb2yuv_pan_display_arch(info, posd_info, pDstOSDBuffer);

amba_abgr8888_2_ayuv4444_pan_display_exit:
	mutex_unlock(&posd_info->mtx);
	return errorCode;
}

static int amba_fb_set_par(struct fb_info *info)
{
	int				errorCode = 0, i;
	ambosd_info_t			*posd_info;
	struct amba_video_sink_mode	sink_mode;
	ambosd_sink_cfg_t		*psink_cfg;
	u32				flag;

	posd_info = &ambosd_info[info->node];
	for (i = 0; i < 2; i++) {
		if (posd_info->vout_id[i] == -1)
			continue;

		psink_cfg = &ambosd_sink_cfg[i];
		if (!psink_cfg->mtx_init) {
			mutex_init(&psink_cfg->mtx);
			psink_cfg->mtx_init = 1;
		}

		mutex_lock(&psink_cfg->mtx);
		if (!psink_cfg->data_valid) {
			vout_warn("%s: Please init vout first!\n", __func__);
			errorCode = -EACCES;
			mutex_unlock(&psink_cfg->mtx);
			goto amba_fb_set_par_exit;
		} else {
			memcpy(&sink_mode, &psink_cfg->sink_mode,
				sizeof(struct amba_video_sink_mode));
		}
		mutex_unlock(&psink_cfg->mtx);

		errorCode = amba_osd_on_vout_change(i, &sink_mode);

		flag = AMBA_VIDEO_SOURCE_UPDATE_OSD_SETUP;
		errorCode = amba_vout_video_source_cmd(i,
				AMBA_VIDEO_SOURCE_UPDATE_VOUT_SETUP, &flag);
	}

amba_fb_set_par_exit:
	return errorCode;
}

static int amba_fb_set_blank(int blank_mode, struct fb_info *info)
{
	int				errorCode = 0;

	switch (blank_mode) {
	case FB_BLANK_NORMAL:
	case FB_BLANK_VSYNC_SUSPEND:
	case FB_BLANK_HSYNC_SUSPEND:
	case FB_BLANK_POWERDOWN:
		memset((void *)info->screen_base, 0, info->fix.smem_len);
		break;

	default:
		break;
	}

	return errorCode;
}

static int amba_fb_set_cmap(struct fb_cmap *cmap, struct fb_info *info)
{
	int				errorCode = 0, i;
	ambosd_info_t			*posd_info;
	struct amba_video_sink_mode	sink_mode;
	ambosd_sink_cfg_t		*psink_cfg;
	u32				flag;

	posd_info = &ambosd_info[info->node];
	if (posd_info->color_format != AMBFB_COLOR_CLUT_8BPP)
		goto amba_fb_set_cmap_exit;

	for (i = 0; i < 2; i++) {
		if (posd_info->vout_id[i] == -1)
			continue;

		psink_cfg = &ambosd_sink_cfg[i];
		if (!psink_cfg->mtx_init) {
			mutex_init(&psink_cfg->mtx);
			psink_cfg->mtx_init = 1;
		}

		mutex_lock(&psink_cfg->mtx);
		if (!psink_cfg->data_valid) {
			vout_warn("%s: Please init vout first!\n", __func__);
			errorCode = -EACCES;
			mutex_unlock(&psink_cfg->mtx);
			goto amba_fb_set_cmap_exit;
		} else {
			memcpy(&sink_mode, &psink_cfg->sink_mode,
				sizeof(struct amba_video_sink_mode));
		}
		mutex_unlock(&psink_cfg->mtx);

		posd_info->cmap_changed = 1;
		errorCode = amba_osd_on_vout_change(i, &sink_mode);

		flag = AMBA_VIDEO_SOURCE_UPDATE_OSD_SETUP;
		errorCode = amba_vout_video_source_cmd(i,
				AMBA_VIDEO_SOURCE_UPDATE_VOUT_SETUP, &flag);
	}

amba_fb_set_cmap_exit:
	return errorCode;
}

/* Notice: Ideally, when one fb displays on two vouts, both vout should enable
	   csc or disable csc; but if not, fb will always use the one that was
	   first initialized
*/
int amba_osd_on_vout_change(int vout_id, struct amba_video_sink_mode *sink_mode)
{
	int				errorCode = 0, fb_id, skip_fb = 0;
	ambosd_info_t			*posd_info;
	struct ambarella_platform_fb	*pplatform_info;
	struct ambarella_fb_iav_info	*piav_info;
	ambosd_sink_cfg_t		*psink_cfg;
	int				fb_reinit = 0;

	/* Malloc Local Memory */
	pplatform_info =
		kzalloc(sizeof(struct ambarella_platform_fb), GFP_KERNEL);
	piav_info =
		kzalloc(sizeof(struct ambarella_fb_iav_info), GFP_KERNEL);
	if (!pplatform_info || !piav_info) {
		DRV_PRINT("%s: Out of memory!\n", __func__);
		goto amba_osd_on_vout_change_exit;
	}

	/* Update sink config */
	psink_cfg = &ambosd_sink_cfg[vout_id];
	if (!psink_cfg->mtx_init) {
		mutex_init(&psink_cfg->mtx);
		psink_cfg->mtx_init = 1;
	}
	mutex_lock(&psink_cfg->mtx);
	memcpy(&psink_cfg->sink_mode, sink_mode, sizeof(*sink_mode));
	psink_cfg->data_valid = 1;
	mutex_unlock(&psink_cfg->mtx);

	/* Judge whether vout uses valid fb */
	fb_id = sink_mode->fb_id;
	if (fb_id < 0 || fb_id >= NUM_FB)
		skip_fb = 1;
	if (!skip_fb && ambarella_fb_get_platform_info(fb_id, pplatform_info))
		skip_fb = 1;
	if (!skip_fb && (pplatform_info->fb_status != AMBFB_ACTIVE_MODE))
		skip_fb = 1;
	if (skip_fb) {
		for (fb_id = 0; fb_id < NUM_FB; fb_id++) {
			posd_info = &ambosd_info[fb_id];
			if (!posd_info->mtx_init) {
				mutex_init(&posd_info->mtx);
				posd_info->mtx_init = 1;
			}
			mutex_lock(&posd_info->mtx);
			posd_info->vout_id[vout_id] = -1;
			mutex_unlock(&posd_info->mtx);
		}
		amba_osd_on_vout_change_arch(vout_id, sink_mode,
			NULL, NULL);
		goto amba_osd_on_vout_change_exit;
	}

	/* Update fb info */
	posd_info = &ambosd_info[fb_id];
	if (!posd_info->mtx_init) {
		mutex_init(&posd_info->mtx);
		posd_info->mtx_init = 1;
	}
	mutex_lock(&posd_info->mtx);
	posd_info->vout_id[vout_id] = vout_id;
	amba_osd_on_vout_change_pre_arch(posd_info);
	if (pplatform_info->color_format != posd_info->color_format) {
		posd_info->color_format = pplatform_info->color_format;
		fb_reinit = 1;
	}
	if (pplatform_info->conversion_buf.available != posd_info->conversion_buf.available
		|| pplatform_info->conversion_buf.ping_buf != posd_info->conversion_buf.ping_buf
		|| pplatform_info->conversion_buf.ping_buf_size != posd_info->conversion_buf.ping_buf_size
		|| pplatform_info->conversion_buf.pong_buf != posd_info->conversion_buf.pong_buf
		|| pplatform_info->conversion_buf.pong_buf_size != posd_info->conversion_buf.pong_buf_size) {
		posd_info->conversion_buf = pplatform_info->conversion_buf;
		fb_reinit = 1;
	}
	if (posd_info->cmap_changed &&
		posd_info->color_format == AMBFB_COLOR_CLUT_8BPP) {
		fb_reinit = 1;
		posd_info->cmap_changed = 0;
	}
	if (pplatform_info->screen_var.xres != posd_info->xres) {
		posd_info->xres = pplatform_info->screen_var.xres;
		fb_reinit = 1;
	}
	if (pplatform_info->screen_var.yres != posd_info->yres) {
		posd_info->yres = pplatform_info->screen_var.yres;
		fb_reinit = 1;
	}
	if (sink_mode->csc_en != posd_info->csc_en) {
		posd_info->csc_en = sink_mode->csc_en;
		fb_reinit = 1;
	}
	if (sink_mode->format == AMBA_VIDEO_FORMAT_INTERLACE)
		posd_info->interlace[vout_id] = 1;
	else
		posd_info->interlace[vout_id] = 0;
	if (sink_mode->mode == AMBA_VIDEO_MODE_480I ||
		sink_mode->mode == AMBA_VIDEO_MODE_576I) {
		posd_info->cvbs_resolution[vout_id] = 1;
	} else {
		posd_info->cvbs_resolution[vout_id] = 0;
	}
	if (sink_mode->osd_tailor & AMBA_VOUT_OSD_AUTO_COPY) {
		posd_info->auto_copy = 1;
	}

	/* Change Vout OSD Setting */
	sink_mode->osd_size.width = pplatform_info->screen_var.xres;
	sink_mode->osd_size.height = pplatform_info->screen_var.yres;
	amba_osd_on_vout_change_arch(vout_id, sink_mode,
		posd_info, pplatform_info);

	/* Re-init fb */
	if (fb_reinit) {
		piav_info->screen_var = pplatform_info->screen_var;
		piav_info->screen_fix = pplatform_info->screen_fix;
		piav_info->dsp_status = AMBA_DSP_ENCODE_MODE;
		switch (pplatform_info->color_format) {
		case AMBFB_COLOR_CLUT_8BPP:
			piav_info->pan_display = amba_direct_pan_display;
			break;

		case AMBFB_COLOR_AUTO:
		case AMBFB_COLOR_RGB565:
			if (posd_info->support_direct_mode) {
				if (posd_info->csc_en && (!posd_info->support_mixer_csc ||
                                   (sink_mode->mixer_csc == AMBA_VOUT_MIXER_DISABLE)) &&
                                   !(sink_mode->osd_tailor & AMBA_VOUT_OSD_NO_CSC)) {
					if (!amba_rgb565_2_uyv565_init(posd_info,
						piav_info->screen_fix.line_length,
						piav_info->screen_var.yres)) {

						piav_info->pan_display = amba_rgb565_2_uyv565_pan_display;
					}
				} else {
					piav_info->pan_display = amba_direct_pan_display;
				}
			} else {
				if (!amba_rgb2clut_init(posd_info,
					piav_info->screen_fix.line_length >> 1,
					piav_info->screen_var.yres_virtual)){

					piav_info->pan_display = amba_rgb2clut_pan_display;
				}
			}
			break;

		case AMBFB_COLOR_ARGB4444:
			if (posd_info->support_direct_mode) {
				if (posd_info->csc_en && (!posd_info->support_mixer_csc ||
                                   (sink_mode->mixer_csc == AMBA_VOUT_MIXER_DISABLE))) {
					if (!amba_argb4444_2_ayuv4444_init(posd_info,
						piav_info->screen_fix.line_length,
						piav_info->screen_var.yres)) {

						piav_info->pan_display = amba_argb4444_2_ayuv4444_pan_display;
					}
				} else {
					piav_info->pan_display = amba_direct_pan_display;
				}
			} else {
				if (!amba_rgb2clut_init(posd_info,
					piav_info->screen_fix.line_length >> 1,
					piav_info->screen_var.yres_virtual)){

					piav_info->pan_display = amba_rgb2clut_pan_display;
				}
			}
			break;

		case AMBFB_COLOR_RGBA4444:
			if (posd_info->support_direct_mode) {
				if (posd_info->csc_en && (!posd_info->support_mixer_csc ||
                                   (sink_mode->mixer_csc == AMBA_VOUT_MIXER_DISABLE))) {
					if (!amba_rgba4444_2_ayuv4444_init(posd_info,
						piav_info->screen_fix.line_length,
						piav_info->screen_var.yres)) {

						piav_info->pan_display = amba_rgba4444_2_ayuv4444_pan_display;
					}
				} else {
					piav_info->pan_display = amba_direct_pan_display;
				}
			} else {
				if (!amba_rgb2clut_init(posd_info,
					piav_info->screen_fix.line_length >> 1,
					piav_info->screen_var.yres_virtual)){

					piav_info->pan_display = amba_rgb2clut_pan_display;
				}
			}
			break;

		case AMBFB_COLOR_RGBA8888:
			if (posd_info->support_direct_mode) {
				if (posd_info->csc_en && (!posd_info->support_mixer_csc ||
                                   (sink_mode->mixer_csc == AMBA_VOUT_MIXER_DISABLE))) {
					if (!amba_rgba8888_2_ayuv4444_init(posd_info,
						piav_info->screen_fix.line_length,
						piav_info->screen_var.yres)) {

						piav_info->pan_display = amba_rgba8888_2_ayuv4444_pan_display;
					}
				} else {
					piav_info->pan_display = amba_direct_pan_display;
				}
			} else {
				if (!amba_rgb2clut_init(posd_info,
					piav_info->screen_fix.line_length >> 1,
					piav_info->screen_var.yres_virtual)){

					piav_info->pan_display = amba_rgb2clut_pan_display;
				}
			}
			break;

		case AMBFB_COLOR_ABGR8888:
			if (posd_info->support_direct_mode) {
				if (posd_info->csc_en && (!posd_info->support_mixer_csc ||
                                   (sink_mode->mixer_csc == AMBA_VOUT_MIXER_DISABLE))) {
					if (!amba_abgr8888_2_ayuv4444_init(posd_info,
						piav_info->screen_fix.line_length,
						piav_info->screen_var.yres)) {

						piav_info->pan_display = amba_abgr8888_2_ayuv4444_pan_display;
					}
				} else {
					piav_info->pan_display = amba_direct_pan_display;
				}
			} else {
				if (!amba_rgb2clut_init(posd_info,
					piav_info->screen_fix.line_length >> 1,
					piav_info->screen_var.yres_virtual)){

					piav_info->pan_display = amba_rgb2clut_pan_display;
				}
			}
			break;

		case AMBFB_COLOR_ARGB8888:
			piav_info->pan_display = amba_direct_pan_display;
			break;

		case AMBFB_COLOR_AYUV8888:
			piav_info->pan_display = amba_direct_pan_display;
			break;

		case AMBFB_COLOR_VYU565:
			piav_info->pan_display = amba_direct_pan_display;
			break;

		default:
			break;
		}
		piav_info->set_par = amba_fb_set_par;
		piav_info->setcmap = amba_fb_set_cmap;
		piav_info->set_blank = amba_fb_set_blank;
		ambarella_fb_set_iav_info(fb_id, piav_info);
	}
	mutex_unlock(&posd_info->mtx);

	posd_info = &ambosd_info[1 - fb_id];
	if (!posd_info->mtx_init) {
		mutex_init(&posd_info->mtx);
		posd_info->mtx_init = 1;
	}
	mutex_lock(&posd_info->mtx);
	posd_info->vout_id[vout_id] = -1;
	mutex_unlock(&posd_info->mtx);

amba_osd_on_vout_change_exit:
	if (pplatform_info) kfree(pplatform_info);
	if (piav_info) kfree(piav_info);

	return errorCode;
};
EXPORT_SYMBOL(amba_osd_on_vout_change);

int amba_osd_on_fb_switch(int vout_id, int fb_id)
{
	int				errorCode = 0;
	struct amba_video_sink_mode	sink_mode;
	ambosd_sink_cfg_t		*psink_cfg;

	psink_cfg = &ambosd_sink_cfg[vout_id];
	if (!psink_cfg->mtx_init) {
		mutex_init(&psink_cfg->mtx);
		psink_cfg->mtx_init = 1;
	}

	mutex_lock(&psink_cfg->mtx);
	if (!psink_cfg->data_valid) {
		vout_warn("%s: Please init vout first!\n", __func__);
		errorCode = -EACCES;
		mutex_unlock(&psink_cfg->mtx);
		goto amba_osd_on_fb_switch_exit;
	} else {
		memcpy(&sink_mode, &psink_cfg->sink_mode,
			sizeof(struct amba_video_sink_mode));
		sink_mode.fb_id = fb_id;
	}
	mutex_unlock(&psink_cfg->mtx);

	errorCode = amba_osd_on_vout_change(vout_id, &sink_mode);

amba_osd_on_fb_switch_exit:
	return errorCode;
}
EXPORT_SYMBOL(amba_osd_on_fb_switch);

int amba_osd_on_csc_change(int vout_id, int csc_en)
{
	return amba_osd_on_csc_change_arch(vout_id, csc_en);
}
EXPORT_SYMBOL(amba_osd_on_csc_change);

int amba_osd_on_rescaler_change(int vout_id, int enable, int width, int height)
{
	int				errorCode = 0;
	struct amba_video_sink_mode	sink_mode;
	ambosd_sink_cfg_t		*psink_cfg;

	psink_cfg = &ambosd_sink_cfg[vout_id];
	if (!psink_cfg->mtx_init) {
		mutex_init(&psink_cfg->mtx);
		psink_cfg->mtx_init = 1;
	}

	mutex_lock(&psink_cfg->mtx);
	if (!psink_cfg->data_valid) {
		vout_warn("%s: Please init vout first!\n", __func__);
		errorCode = -EACCES;
		mutex_unlock(&psink_cfg->mtx);
		goto amba_osd_on_rescaler_change_exit;
	} else {
		memcpy(&sink_mode, &psink_cfg->sink_mode,
			sizeof(struct amba_video_sink_mode));
		if (sink_mode.fb_id < 0) {
			vout_warn("%s: No fb available for vout%d!\n",
				__func__, vout_id);
			errorCode = -EACCES;
			mutex_unlock(&psink_cfg->mtx);
			goto amba_osd_on_rescaler_change_exit;
		}
		sink_mode.osd_rescale.enable = enable;
		sink_mode.osd_rescale.width = width;
		sink_mode.osd_rescale.height = height;
	}
	mutex_unlock(&psink_cfg->mtx);

	errorCode = amba_osd_on_vout_change(vout_id, &sink_mode);

amba_osd_on_rescaler_change_exit:
	return errorCode;
}
EXPORT_SYMBOL(amba_osd_on_rescaler_change);

int amba_osd_on_offset_change(int vout_id, int change, int x, int y)
{
	int				errorCode = 0;
	struct amba_video_sink_mode	sink_mode;
	ambosd_sink_cfg_t		*psink_cfg;

	psink_cfg = &ambosd_sink_cfg[vout_id];
	if (!psink_cfg->mtx_init) {
		mutex_init(&psink_cfg->mtx);
		psink_cfg->mtx_init = 1;
	}

	mutex_lock(&psink_cfg->mtx);
	if (!psink_cfg->data_valid) {
		vout_warn("%s: Please init vout first!\n", __func__);
		errorCode = -EACCES;
		mutex_unlock(&psink_cfg->mtx);
		goto amba_osd_on_offset_change_exit;
	} else {
		memcpy(&sink_mode, &psink_cfg->sink_mode,
			sizeof(struct amba_video_sink_mode));
		if (sink_mode.fb_id < 0) {
			vout_warn("%s: No fb available for vout%d!\n",
				__func__, vout_id);
			errorCode = -EACCES;
			mutex_unlock(&psink_cfg->mtx);
			goto amba_osd_on_offset_change_exit;
		}
		sink_mode.osd_offset.specified = change;
		sink_mode.osd_offset.offset_x = x;
		sink_mode.osd_offset.offset_y = y;
	}
	mutex_unlock(&psink_cfg->mtx);

	errorCode = amba_osd_on_vout_change(vout_id, &sink_mode);

amba_osd_on_offset_change_exit:
	return errorCode;
}
EXPORT_SYMBOL(amba_osd_on_offset_change);

