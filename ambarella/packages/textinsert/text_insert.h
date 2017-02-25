/*******************************************************************************
 * text_insert.h
 *
 * History:
 *  2011/02/25 - [Qian Shen] created file
 *
 * Copyright (c) 2016 Ambarella, Inc.
 *
 * This file and its contents ( "Software" ) are protected by intellectual
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
 ******************************************************************************/

#ifndef _TEXT2BITMAP_LIB_H
#define _TEXT2BITMAP_LIB_H


#ifdef __cplusplus
extern "C" {
#endif

#define MAX_BYTE_SIZE	128

#ifndef _BASE_TYPES_H_
typedef unsigned char		u8;	/**< UNSIGNED 8-bit data type */
typedef unsigned short		u16;/**< UNSIGNED 16-bit data type */
typedef unsigned int		u32;	/**< UNSIGNED 32-bit data type */
typedef unsigned long long	u64; /**< UNSIGNED 64-bit data type */
typedef signed char		s8;	/**< SIGNED 8-bit data type */
typedef signed short		s16;	/**< SIGNED 16-bit data type */
typedef signed int		s32;	/**< SIGNED 32-bit data type */
#endif

typedef struct {
	int major;
	int minor;
	int patch;
	unsigned int mod_time;
	char description[64];
} version_t;

typedef struct font_attribute_s {
	char type[MAX_BYTE_SIZE];
	u32 size;
	u32 outline_width;		// 0: no outline
	s32 hori_bold;			// 0: no bold at all; positive is bold, negetive is thin in horizontal
	s32 vert_bold;			// 0: no bold at all; positive is bold, negetive is thin in vertical
	u32 italic;				// 0 is normal (no italic), 100 is 100% italic
	u32 disable_anti_alias; // 0: anti-alias is enabled, 1: disable anti-alias. Default is 0.
} font_attribute_t;

typedef struct bitmap_info_s {
	int width;
	int height;
} bitmap_info_t;

typedef struct pixel_type_s {
	u8 pixel_background;
	u8 pixel_font;
	u8 pixel_outline;
	u8 reserved;
} pixel_type_t;

int text2bitmap_lib_init(pixel_type_t *type);
int text2bitmap_set_font_attribute(const font_attribute_t *font_attr);
int text2bitmap_get_font_attribute(font_attribute_t *font_attr);
int text2bitmap_convert_character(u32 ui_char_code, u8 *bmp_addr, u16 buf_height, u16 buf_pitch, int offset_x, bitmap_info_t *bmp_info);
int text2bitmap_lib_deinit(void);
int text2bitmap_get_version(version_t* version);

#ifdef __cplusplus
}
#endif

#endif //_TEXT2BITMAP_LIB_H

