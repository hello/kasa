
/***********************************************************
 * fb_image.c
 *
 * History:
 *	2013/08/02 - [Jy Qiu] created file
 *
 * Copyright (C) 2015 Ambarella, Inc.
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

#include <wchar.h>
#include <linux/fb.h>

#include <ctype.h>
#include <time.h>
#include <sys/time.h>
#include <sys/mman.h>

#include "fb_image.h"

fb_vin_vout_size_t	fb_size;

char G_fb_osd[128];
static int fb_x = 0;
static int fb_y = 0;

int G_fb_text_flag;


#define	FB_BACKGROUD_W	512
#define	FB_BACKGROUD_H	256

#define	RECTANGLE_DISTANCE	1

/*	center	*/
//#define	FB_BACKGROUD_X	((fb_size.vout_width -FB_BACKGROUD_W) / 2)
//#define	FB_BACKGROUD_Y	(fb_size.vout_height - (var.yres -FB_BACKGROUD_H) / 2)

/*	right bottum	*/
#define	FB_BACKGROUD_X	((fb_size.vout_width - FB_BACKGROUD_W) * 7 / 8)
#define	FB_BACKGROUD_Y	(fb_size.vout_height - (var.yres - FB_BACKGROUD_H) / 8)

#define	FB_HISTOGRAM_COLOR		100
#define	FB_HISTOGRAM_ALPHA		3
#define	FB_BACKGROUD_COLOR		200
#define	FB_BACKGROUD_ALPHA		3

static int fb = -1;
u8 *fb_mem = NULL;

typedef struct clut_s {
	u8 v;
	u8 u;
	u8 y;
	u8 alpha;
} clut_t;

struct fb_cmap_user {
	u32 start; /* First entry	*/
	u32 len; /* Number of entries */
	u16 *y;
	u16 *u;
	u16 *v;
	u16 *transparent;		/* transparency, can be NULL */
};

typedef struct fb_rectangle_s {
	int	offset_x;
	int	offset_y;
	int	rec_width;
	int	rec_height;
	int	color;
	int	alpha;
} fb_rectangle_t;

typedef struct fb_color_s {
	u16	backgroud_color;
	u16	backgroud_alpha;
	u16	rec_color;
	u16	rec_alpha;
} fb_color_t;

static fb_color_t fb_color = {
	.backgroud_color = FB_BACKGROUD_COLOR,
	.backgroud_alpha = FB_BACKGROUD_ALPHA,
	.rec_color = FB_HISTOGRAM_COLOR,
	.rec_alpha = FB_HISTOGRAM_ALPHA,
};

struct fb_var_screeninfo var;
struct fb_fix_screeninfo fix;

static unsigned short y_table[256] =
{
	  5,
	191,
	  0,
	191,
	  0,
	191,
	  0,
	192,
	128,
	255,
	  0,
	255,
	  0,
	255,
	  0,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 51,
	102,
	153,
	204,
	255,
	  0,
	 17,
	 34,
	 51,
	 68,
	 85,
	102,
	119,
	136,
	153,
	170,
	187,
	204,
	221,
	238,
	255,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	204,
	242,
};


static unsigned short u_table[256] =
{
	  4,
	  0,
	191,
	191,
	  0,
	  0,
	191,
	192,
	128,
	  0,
	255,
	255,
	  0,
	  0,
	255,
	255,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	102,
	102,
	102,
	102,
	102,
	102,
	153,
	153,
	153,
	153,
	153,
	153,
	204,
	204,
	204,
	204,
	204,
	204,
	255,
	255,
	255,
	255,
	255,
	255,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	102,
	102,
	102,
	102,
	102,
	102,
	153,
	153,
	153,
	153,
	153,
	153,
	204,
	204,
	204,
	204,
	204,
	204,
	255,
	255,
	255,
	255,
	255,
	255,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	102,
	102,
	102,
	102,
	102,
	102,
	153,
	153,
	153,
	153,
	153,
	153,
	204,
	204,
	204,
	204,
	204,
	204,
	255,
	255,
	255,
	255,
	255,
	255,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	102,
	102,
	102,
	102,
	102,
	102,
	153,
	153,
	153,
	153,
	153,
	153,
	204,
	204,
	204,
	204,
	204,
	204,
	255,
	255,
	255,
	255,
	255,
	255,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	102,
	102,
	102,
	102,
	102,
	102,
	153,
	153,
	153,
	153,
	153,
	153,
	204,
	204,
	204,
	204,
	204,
	204,
	255,
	255,
	255,
	255,
	255,
	255,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	102,
	102,
	102,
	102,
	102,
	102,
	153,
	153,
	153,
	153,
	153,
	153,
	204,
	204,
	204,
	204,
	204,
	204,
	255,
	255,
	255,
	255,
	255,
	255,
	  0,
	 17,
	 34,
	 51,
	 68,
	 85,
	102,
	119,
	136,
	153,
	170,
	187,
	204,
	221,
	238,
	255,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	102,
};


static unsigned short v_table[256] =
{
	  3,
	  0,
	  0,
	  0,
	191,
	191,
	191,
	192,
	128,
	  0,
	  0,
	  0,
	255,
	255,
	255,
	255,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	 51,
	102,
	102,
	102,
	102,
	102,
	102,
	102,
	102,
	102,
	102,
	102,
	102,
	102,
	102,
	102,
	102,
	102,
	102,
	102,
	102,
	102,
	102,
	102,
	102,
	102,
	102,
	102,
	102,
	102,
	102,
	102,
	102,
	102,
	102,
	102,
	102,
	153,
	153,
	153,
	153,
	153,
	153,
	153,
	153,
	153,
	153,
	153,
	153,
	153,
	153,
	153,
	153,
	153,
	153,
	153,
	153,
	153,
	153,
	153,
	153,
	153,
	153,
	153,
	153,
	153,
	153,
	153,
	153,
	153,
	153,
	153,
	153,
	204,
	204,
	204,
	204,
	204,
	204,
	204,
	204,
	204,
	204,
	204,
	204,
	204,
	204,
	204,
	204,
	204,
	204,
	204,
	204,
	204,
	204,
	204,
	204,
	204,
	204,
	204,
	204,
	204,
	204,
	204,
	204,
	204,
	204,
	204,
	204,
	255,
	255,
	255,
	255,
	255,
	255,
	255,
	255,
	255,
	255,
	255,
	255,
	255,
	255,
	255,
	255,
	255,
	255,
	255,
	255,
	255,
	255,
	255,
	255,
	255,
	255,
	255,
	255,
	255,
	255,
	255,
	255,
	255,
	255,
	255,
	255,
	  0,
	 17,
	 34,
	 51,
	 68,
	 85,
	102,
	119,
	136,
	153,
	170,
	187,
	204,
	221,
	238,
	255,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	  0,
	 34,
};

static unsigned short blend_table[256] =
{
		0,  12, 12, 12, 12, 12, 12, 12,
		12,  12, 12, 12, 12, 12, 12, 12,
		12,  12, 12, 12, 12, 12, 12, 12,
		12,  12, 12, 12, 12, 12, 12, 12,
		12,  12, 12, 12, 12, 12, 12, 12,
		12,  12, 12, 12, 12, 12, 12, 12,
		12,  12, 12, 12, 12, 12, 12, 12,
		12,  12, 12, 12, 12, 12, 12, 12,
		12,  12, 12, 12, 12, 12, 12, 12,
		12,  12, 12, 12, 12, 12, 12, 12,
		12,  12, 12, 12, 12, 12, 12, 12,
		12,  12, 12, 12, 12, 12, 12, 12,
		12,  12, 12, 12, 12, 12, 12, 12,
		12,  12, 12, 12, 12, 12, 12, 12,
		12,  12, 12, 12, 12, 12, 12, 12,
		12,  12, 12, 12, 12, 12, 12, 12,
		12,  12, 12, 12, 12, 12, 12, 12,
		12,  12, 12, 12, 12, 12, 12, 12,
		12,  12, 12, 12, 12, 12, 12, 12,
		12,  12, 12, 12, 12, 12, 12, 12,
		12,  12, 12, 12, 12, 12, 12, 12,
		12,  12, 12, 12, 12, 12, 12, 12,
		12,  12, 12, 12, 12, 12, 12, 12,
		12,  12, 12, 12, 12, 12, 12, 12,
		12,  12, 12, 12, 12, 12, 12, 12,
		12,  12, 12, 12, 12, 12, 12, 12,
		12,  12, 12, 12, 12, 12, 12, 12,
		12,  12, 12, 12, 12, 12, 12, 12,
		12,  12, 12, 12, 12, 12, 12, 12,
		12,  12, 12, 12, 12, 12, 12, 12,
		12,  12, 12, 12, 12, 12, 12, 12,
};

static int fb_put_cmap(fb_color_t *color)
{
	u16 i;
	struct fb_cmap_user cmap;
	cmap.start = 0;
	cmap.len = 256;

	for (i = 0; i < 256; ++i) {
		blend_table[i] = i;
	}

	cmap.start = 0;
	cmap.len = 256;
	cmap.y = y_table;
	cmap.u = u_table;
	cmap.v = v_table;
	blend_table[color->backgroud_color] = color->backgroud_alpha;
	blend_table[color->rec_color] = color->rec_alpha;

	cmap.transparent = blend_table;

	if (ioctl(fb, FBIOPUTCMAP, &cmap) < 0) {
		printf("Unable to put cmap!\n");
		return -1;
	}
	return 0;
}

static int fb_init(void)
{

	if (ioctl(fb, FBIOGET_VSCREENINFO, &var) >= 0) {
		printf("xres = %d, yres = %d, xres_virtual = %d, yres_virtual = %d\n",
			var.xres, var.yres, var.xres_virtual, var.yres_virtual);
		printf("fb_size:  vout_width = %d, vout_height = %d \n",
			fb_size.vout_width, fb_size.vout_height);

		if (fb_size.vout_width < var.xres) {
			var.xres = var.xres_virtual = fb_size.vout_width;
		}
		if (fb_size.vout_height < var.yres) {
			var.yres = fb_size.vout_height;
			var.yres_virtual = fb_size.vout_height * 2;
		}
		printf("xres = %d, yres = %d, xres_virtual = %d, yres_virtual = %d\n",
			var.xres, var.yres, var.xres_virtual, var.yres_virtual);
		if ( ioctl(fb, FBIOPUT_VSCREENINFO, &var) >= 0 )
			printf("\tFB: %d x %d @ %dbpp\n", var.xres, var.yres, var.bits_per_pixel);
	} else {
		printf("\tUnable to get var and fix info!\n");
		return -1;
	}

	if (ioctl(fb, FBIOGET_FSCREENINFO, &fix) >= 0)
		fb_mem = (u8 *) mmap(NULL, fix.smem_len, PROT_WRITE, MAP_SHARED, fb, 0);
	else {
		printf("Unable to get screen_fix_info\n");
		return -1;
	}

	fb_x = fb_x < var.xres ? fb_x : 0;
	fb_y = fb_y < var.yres ? fb_y : 0;
	printf("xres = %d, yres = %d, line = %d\n", var.xres, var.yres, fix.line_length);
	printf("fb_x = %d, fb_y = %d, line = %d\n", fb_x, fb_y, fix.line_length);
	return 0;
}

#ifdef BUILD_AMBARELLA_TEXTINSERT_PACKAGE

#include "text_insert.h"

static wchar_t wstring[MAX_BYTE_SIZE];

pixel_type_t pixel_type = {
		.pixel_background = 0,
		.pixel_outline = 1,
		.pixel_font = 255,
};

char font_type[4][MAX_BYTE_SIZE] = {
		"/usr/share/fonts/Vera.ttf",
		"/usr/share/fonts/Lucida.ttf",
		"/usr/share/fonts/gbsn00lp.ttf",
		"/usr/share/fonts/UnPen.ttf", };

wchar_t default_wtext[4][MAX_BYTE_SIZE] = {
	L"AmbarellaA5sSDK",
	L"True Type Support",
	L"安霸中国(Chinese)",
	L"여보세요(Korean)" };

static font_attribute_t font =
{
	.size = 24,
	.type = "/usr/share/fonts/gbsn00lp.ttf",
	.disable_anti_alias = 1,
	.hori_bold = 1,
	.italic = 0,
};

#define TEXT_DISPALY_TIME	500	//1=20ms, 500=10s

#ifndef LINE_SPACING
#define LINE_SPACING(x)		((x) * 3 / 2)
#endif

#define TIME_DISPLAY_LINE	0
#define FIXED_DIGIT_WIDTH	(2)
#define TIME_OFFSET_X_SHIFT	LINE_SPACING(font.size) *12
char last_time[MAX_BYTE_SIZE];
u8 *digit_buf = NULL;
static int digit_width = 0;
static int time_offset[32];

int char_to_wchar(const char* orig_str, wchar_t *wtext)
{
	int str_length = strlen(orig_str);
	if (str_length != mbstowcs(wtext, orig_str, str_length + 1))
		return -1;

	if (wcscat(wtext, L"") == NULL)
		return -1;

	return 0;
}

static void get_time_string(char *str)
{
	time_t t;
	struct tm *p;
	time(&t);
	p = gmtime(&t);
	sprintf(str, "%04d-%02d-%02d %02d:%02d:%02d", (1900 + p->tm_year), (1
	        + p->tm_mon), p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec);
}

static int fb_time_digital()
{
	int i, offset_x;
	int line_height = LINE_SPACING(font.size);

	u8 *line_addr = fb_mem + fix.line_length* line_height *TIME_DISPLAY_LINE;
	u16 total_width = 10 * FIXED_DIGIT_WIDTH * font.size;
	bitmap_info_t bmp = { 0, 0 };
	wchar_t digits[32] = L"0123456789";
	wchar_t *p;
	char current_time[255];
	wchar_t wstring_time[MAX_BYTE_SIZE];

	digit_buf = (u8 *) malloc(total_width * line_height);
	if (digit_buf == NULL) {
		printf("malloc error!\n");
		return -1;
	}
	/* Store 10 digits' bitmap data in (u8 *)digit_buf for later reuse. */
	p = &digits[0];
	offset_x = 0;
	memset(digit_buf, pixel_type.pixel_background, total_width * line_height);
	for (i = 0; i < wcslen(&digits[0]); i++) {
		if (text2bitmap_convert_character(*p, digit_buf, line_height,
				total_width, offset_x, &bmp) < 0)
			return -1;

		offset_x += FIXED_DIGIT_WIDTH * font.size;
		if (bmp.width > digit_width)
			digit_width = bmp.width;
		p++;
	}

	/* Write the time in the frame buffer */
	memset(line_addr, pixel_type.pixel_background, fix.line_length * line_height);
	offset_x = fb_x + TIME_OFFSET_X_SHIFT;

	get_time_string(&current_time[0]);
	if(char_to_wchar(current_time, &wstring_time[0]) < 0) {
		memset(&wstring_time[0], 0, MAX_BYTE_SIZE);
	}
	p = &wstring_time[0];
	for (i = 0; i < wcslen(&wstring_time[0]); i++) {
		time_offset[i] = offset_x;
		if (text2bitmap_convert_character(*p, line_addr, line_height,
				fix.line_length, offset_x, &bmp) < 0)
			return -1;
		if (isdigit(*p))
			offset_x += digit_width;
		else
			offset_x += bmp.width;
		p++;
	}
	strcpy(last_time, current_time);

	return 0;
}
#endif

int clear_all_fb(void)
{
	if (fb < 0) {
		return -1;
	}

	printf("Blanking ...\n");
	ioctl(fb, FBIOBLANK, FB_BLANK_NORMAL);
	memset(fb_mem, 0, var.yres * fix.line_length *2);
	ioctl(fb, FBIOPAN_DISPLAY, &var);
	return 0;
}

int show_all_fb(void)
{
	ioctl(fb, FBIOPAN_DISPLAY, &var);
	var.yoffset = var.yoffset ? 0 : var.yres;
	return 0;
}

int init_fb(fb_vin_vout_size_t *pFb_size)
{
	fb = open("/dev/fb0", O_RDWR);
	if (fb < 0) {
		perror("Open fb error");
		return -1;
	}

	printf("Set color map ...\n");
	if (fb_put_cmap(&fb_color) < 0)
		return -1;

	printf("Initialize frame buffer ...\n");
	memcpy(&fb_size, pFb_size, sizeof(fb_vin_vout_size_t));
	if (fb_init() < 0)
		return -1;

/*
	if (text2bitmap_lib_init(&pixel_type) < 0)
		return -1;

	if (text2bitmap_set_font_attribute(&font) < 0)
		return -1;

	sprintf(G_fb_osd, "Enable Fb display \n");
	G_fb_text_flag = 1;
	fb_time_digital();
*/
	clear_all_fb();
	return 0;
}

int deinit_fb(void)
{
	if (fb > 0) {
		clear_all_fb();
		close(fb);
		fb = -1;
	}
	return 0;
}

static int draw_rectangle(fb_rectangle_t *pRectangle)
{
	if (pRectangle == NULL) {
		printf("Error:pRectangle is null\n");
		return -1;
	}

	int height = pRectangle->rec_height;
	u8 *fb_mem_offset = NULL;
	fb_mem_offset = fb_mem + FB_BACKGROUD_X + pRectangle->offset_x +
		fix.line_length * (FB_BACKGROUD_Y + var.yoffset - pRectangle->offset_y);

	while (height--) {
		memset(fb_mem_offset - fix.line_length * height,
			pRectangle->color, pRectangle->rec_width);
	}

	return 0;
}

int draw_histogram_plane_figure(fb_show_histogram_t *pHdata)
{
	fb_rectangle_t rec_data;
	int i, j;
	int rec_width = FB_BACKGROUD_W / pHdata->col_num;
	int rec_height = FB_BACKGROUD_H / pHdata->row_num;
	int rec_unit = 1;
	if (pHdata == NULL) {
		printf("ERROR: pHdata is NULL\n");
		return -1;
	}
	/*	draw backgroud	*/
	rec_data.rec_width =FB_BACKGROUD_W;
	rec_data.offset_x =  0;
	rec_data.offset_y =  0;
	rec_data.rec_height =  FB_BACKGROUD_H;
	rec_data.color = FB_BACKGROUD_COLOR;
	draw_rectangle(&rec_data);

	/*	draw histograms	*/
	rec_data.rec_width = rec_width - RECTANGLE_DISTANCE;
	rec_data.color = FB_HISTOGRAM_COLOR;

	/*	for limit the height	*/
	rec_unit = (pHdata->the_max -1) / rec_height + 1;

	for (i = 0; i < pHdata->row_num; ++i) {
		rec_data.offset_y = (pHdata->row_num -1 - i) * rec_height;
		for (j = 0; j < pHdata->col_num; ++j) {
			rec_data.offset_x = j * rec_width;
			rec_data.rec_height = pHdata->histogram_value[i * pHdata->col_num + j]
				/ rec_unit;
			draw_rectangle(&rec_data);
		}
	}
	show_all_fb();
	return 0;
}

int draw_histogram_figure(fb_show_histogram_t *pHdata)
{
	fb_rectangle_t rec_data;
	int i;
	int rec_width = FB_BACKGROUD_W / pHdata->total_num;
	int rec_height = FB_BACKGROUD_H;

	int rec_unit = 1;

	if (pHdata == NULL) {
		printf("ERROR: pHdata is NULL\n");
		return -1;
	}
	/*	draw backgroud	*/
	rec_data.rec_width =FB_BACKGROUD_W;
	rec_data.offset_x =  0;
	rec_data.offset_y =  0;
	rec_data.rec_height =  FB_BACKGROUD_H;
	rec_data.color = FB_BACKGROUD_COLOR;
	draw_rectangle(&rec_data);

	/*	draw histograms	*/
	rec_data.rec_width = rec_width - RECTANGLE_DISTANCE;
	rec_data.color = FB_HISTOGRAM_COLOR;
	rec_data.offset_y =  0;

	/*	for limit the height	*/
	rec_unit = (pHdata->the_max -1) / rec_height + 1;

	for (i = 0; i < pHdata->total_num; ++i) {
		rec_data.offset_x = i * rec_width;
		rec_data.rec_height = pHdata ->histogram_value[i] / rec_unit;
		draw_rectangle(&rec_data);
	}
	show_all_fb();
	return 0;
}

