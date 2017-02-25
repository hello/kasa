/*******************************************************************************
 * text_insert.c
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

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ft2build.h>
#include "text_insert.h"

#include FT_FREETYPE_H
#include FT_TRIGONOMETRY_H
#include FT_BITMAP_H
#include FT_STROKER_H

#define FT_INTEGER_BIT		(6)

#ifndef FT_PIX_FLOOR
#define FT_PIX_FLOOR(x)		((x) & ~((1 << FT_INTEGER_BIT) - 1))
#endif

#ifndef FT_PIX_INT
#define FT_PIX_INT(x)		((x) >> FT_INTEGER_BIT)
#endif

#ifndef MIN
#define MIN(x,y)		({	\
		typeof(x) _x = (x); \
		typeof(y) _y = (y); \
		(void) (&_x == &_y);\
		_x < _y ? _x : _y; })
#endif

#ifndef MAX
#define MAX(x,y)		({	\
		typeof(x) _x = (x); \
		typeof(y) _y = (y); \
		(void) (&_x == &_y);\
		_x > _y ? _x : _y; })
#endif

static font_attribute_t cfg_font;
static FT_Library library;
static FT_Face face;

static int flag_init = 0;
static int flag_face = 0;
static pixel_type_t default_pixel_type = {
		.pixel_background = 0,
		.pixel_outline = 1,
		.pixel_font = 255
};
pixel_type_t pixel_type;

typedef struct span_s {
	int x;
	int y;
	int width;
	int coverage;
} span_t;

typedef struct spans_s {
	span_t span;
	struct spans_s* next;
} spans_t;

typedef struct rect_s {
	int xmin;
	int xmax;
	int ymin;
	int ymax;
} rect_t;

static version_t G_text2bitmap_version = {
	.major = 1,
	.minor = 0,
	.patch = 0,
	.mod_time = 0x20130924,
	.description = "Ambarella S2 text-to-bitmap Library",
};

/*
 * ********************************************************
 *		  Set itatlic.
 * ********************************************************
 */

static inline void set_italic(FT_GlyphSlot slot, const int italic)
{
	// Matrix fields are all in 16.16 format.
	FT_Matrix matrix;
	matrix.xx = 0x10000L;
	matrix.xy = italic * 0x10000L / 100;
	matrix.yx = 0;
	matrix.yy = 0x10000L;
	FT_Set_Transform(slot->face, &matrix, 0);
}

/*
 **********************************************************************************
 *	  Set bold in horizontal and vertical direction.
 *
 *
 * FT_Error Hori_FT_Outline_Embolden(FT_Outline *outline, FT_Pos strength)
 * FT_Error Vert_FT_Outline_Embolden(FT_Outline *outline, FT_Pos strength)
 * FT_Error New_FT_Outline_Embolden(FT_Outline *outline, FT_Pos str_h, FT_Pos str_v)
 * int set_bold(FT_GlyphSlot slot, int v_str, int h_str)
 *
 **********************************************************************************
 */
// Embold in horizon
static FT_Error Hori_FT_Outline_Embolden(FT_Outline *outline, FT_Pos strength)
{
	FT_Vector *points;
	FT_Vector v_prev, v_first, v_next, v_cur;
	FT_Angle rotate, angle_in, angle_out;
	FT_Int c, n, first;
	FT_Int orientation;

	int last = 0;
	FT_Vector in, out;
	FT_Angle angle_diff;
	FT_Pos d;
	FT_Fixed scale;

	if (!outline)
		return FT_Err_Invalid_Argument;

	strength /= 2;
	if (strength == 0)
		return FT_Err_Ok;

	orientation = FT_Outline_Get_Orientation(outline);
	if (orientation == FT_ORIENTATION_NONE) {
		if (outline->n_contours)
			return FT_Err_Invalid_Argument;
		else
			return FT_Err_Ok;
	}

	if (orientation == FT_ORIENTATION_TRUETYPE)
		rotate = -FT_ANGLE_PI2;
	else
		rotate = FT_ANGLE_PI2;

	points = outline->points;

	first = 0;
	for (c = 0; c < outline->n_contours; c++) {
		last = outline->contours[c];

		v_first = points[first];
		v_prev = points[last];
		v_cur = v_first;

		for (n = first; n <= last; n++) {
			if (n < last)
				v_next = points[n+1];
			else
				v_next = v_first;

			// compute the in and out vectors
			in.x = v_cur.x - v_prev.x;
			in.y = v_cur.y - v_prev.y;

			out.x = v_next.x - v_cur.x;
			out.y = v_next.y - v_cur.y;

			angle_in = FT_Atan2(in.x, in.y);
			angle_out = FT_Atan2(out.x, out.y);
			angle_diff = FT_Angle_Diff(angle_in, angle_out);
			scale = FT_Cos(angle_diff / 2);

			if (scale < 0x4000L && scale > -0x4000L) {
				in.x = in.y = 0;
			} else {
				d = FT_DivFix(strength, scale);
				FT_Vector_From_Polar(&in, d, angle_in + angle_diff/2 - rotate);
			}

			outline->points[n].x = v_cur.x + strength + in.x;

			v_prev = v_cur;
			v_cur = v_next;
		}

		first = last + 1;
	}

	return FT_Err_Ok;
}

// Embold in vertical
static FT_Error Vert_FT_Outline_Embolden(FT_Outline *outline, FT_Pos strength)
{
	FT_Vector *points;
	FT_Vector v_prev, v_first, v_next, v_cur;
	FT_Angle rotate, angle_in, angle_out;
	FT_Int c, n, first;
	FT_Int orientation;

	int last = 0;
	FT_Vector in, out;
	FT_Angle angle_diff;
	FT_Pos d;
	FT_Fixed scale;

	if (!outline)
		return FT_Err_Invalid_Argument;

	strength /= 2;
	if (strength == 0)
		return FT_Err_Ok;

	orientation = FT_Outline_Get_Orientation(outline);
	if (orientation == FT_ORIENTATION_NONE) {
		if (outline->n_contours)
			return FT_Err_Invalid_Argument;
		else
			return FT_Err_Ok;
	}

	if (orientation == FT_ORIENTATION_TRUETYPE)
		rotate = -FT_ANGLE_PI2;
	else
		rotate = FT_ANGLE_PI2;

	points = outline->points;

	first = 0;
	for (c = 0; c < outline->n_contours; c++) {
		last = outline->contours[c];

		v_first = points[first];
		v_prev = points[last];
		v_cur = v_first;

		for (n = first; n <= last; n++) {

			if (n < last)
				v_next = points[n+1];
			else
				v_next = v_first;

			// compute the in and out vectors
			in.x = v_cur.x - v_prev.x;
			in.y = v_cur.y - v_prev.y;

			out.x = v_next.x - v_cur.x;
			out.y = v_next.y - v_cur.y;

			angle_in = FT_Atan2(in.x, in.y);
			angle_out = FT_Atan2(out.x, out.y);
			angle_diff = FT_Angle_Diff(angle_in, angle_out);
			scale = FT_Cos(angle_diff / 2);

			if (scale < 0x4000L && scale > -0x4000L) {
				in.x = in.y = 0;
			} else {
				d = FT_DivFix(strength, scale);
				FT_Vector_From_Polar(&in, d, angle_in + angle_diff/2 - rotate);
			}

			outline->points[n].y = v_cur.y + strength + in.y;

			v_prev = v_cur;
			v_cur = v_next;
		}

		first = last + 1;
	}

	return FT_Err_Ok;
}

static FT_Error New_FT_Outline_Embolden(FT_Outline *outline, FT_Pos str_h, FT_Pos str_v)
{
	if (!outline)
		return FT_Err_Invalid_Argument;
	int orientation = FT_Outline_Get_Orientation(outline);
	if ((orientation == FT_ORIENTATION_NONE) &&
		(outline->n_contours))
		return FT_Err_Invalid_Argument;
	Vert_FT_Outline_Embolden(outline, str_v);
	Hori_FT_Outline_Embolden(outline, str_h);
	return FT_Err_Ok;
}


/*
 * Return 0 if bold is set successfully, FT_Error otherwise.
 */
static int set_bold(FT_GlyphSlot slot, int v_str, int h_str)
{
	if (v_str == 0 && h_str == 0)
		return 0;
	FT_Library ft_library = slot->library;
	FT_Error ft_error;
	FT_Pos xstr = v_str, ystr = h_str;

	if (slot->format != FT_GLYPH_FORMAT_OUTLINE &&
		slot->format != FT_GLYPH_FORMAT_BITMAP)
		return -1;
	if (slot->format == FT_GLYPH_FORMAT_OUTLINE) {
		FT_BBox old_box;
		FT_Outline_Get_CBox(&slot->outline, &old_box);
		ft_error = New_FT_Outline_Embolden(&slot->outline, xstr, ystr);
		if (ft_error)
			return ft_error;

		FT_BBox new_box;
		FT_Outline_Get_CBox(&slot->outline, &new_box);
		xstr = (new_box.xMax - new_box.xMin) - (old_box.xMax - old_box.xMin);
		ystr = (new_box.yMax - new_box.yMin) - (old_box.yMax - old_box.yMin);
	} else if (slot->format == FT_GLYPH_FORMAT_BITMAP) {
		xstr = FT_PIX_FLOOR(xstr);
		if (xstr == 0)
			xstr = 1 << 6;
		ystr = FT_PIX_FLOOR(ystr);

		ft_error = FT_Bitmap_Embolden(ft_library, &slot->bitmap, xstr, ystr);
		if (ft_error)
			return ft_error;
	}

	if (slot->advance.x)
		slot->advance.x += xstr;

	if (slot->advance.y)
		slot->advance.y += ystr;

	slot->metrics.width += xstr;
	slot->metrics.height += ystr;
	slot->metrics.horiBearingY += ystr;
	slot->metrics.horiAdvance += xstr;
	slot->metrics.vertBearingX -= xstr/2;
	slot->metrics.vertBearingY += ystr;
	slot->metrics.vertAdvance += ystr;

	if (slot->format == FT_GLYPH_FORMAT_BITMAP)
		slot->bitmap_top += FT_PIX_INT(ystr);

	return 0;
}

/*
 ********************************************************************************************************
 * Set outline.
 *
 * The method is to generate a bigger bitmap as background, and put the original bitmap on the top.
 *
 * void rect_include(rect_t *rect, int a, int b)
 * void raster_callback(const int y, const int count, const FT_Span *spans, void* const user)
 * void render_spans(FT_Library *library, FT_Outline *outline, spans_t *spans)
 * void free_spans(spans_t *spans)
 * u8* set_outline(const u32 outline_width, int* width, int* height)
 *
 ********************************************************************************************************
 */
static inline void rect_include(rect_t *rect, int a, int b)
{
	rect->xmin = MIN(rect->xmin, a);
	rect->ymin = MIN(rect->ymin, b);
	rect->xmax = MAX(rect->xmax, a);
	rect->ymax = MAX(rect->ymax, b);
}

void raster_callback(const int y, const int count, const FT_Span *spans, void* const user)
{
	int i;
	spans_t *sptr = (spans_t *)user, *sp = NULL;
	while (sptr->next != NULL) {
		sptr = sptr->next;
	}
	for (i = 0; i < count; i++) {
		sp = (spans_t *)malloc(sizeof(spans_t));
		sp->span.x = spans[i].x;
		sp->span.y = y;
		sp->span.width = spans[i].len;
		sp->span.coverage = spans[i].coverage;
		sp->next = NULL;

		sptr->next = sp;
		sptr = sptr->next;
	}
}

static void render_spans(FT_Library *library, FT_Outline *outline, spans_t *spans)
{
	FT_Raster_Params params;
	memset(&params, 0, sizeof(params));
	params.flags = FT_RASTER_FLAG_AA | FT_RASTER_FLAG_DIRECT;
	params.gray_spans = raster_callback;
	params.user = spans;

	FT_Outline_Render(*library, outline, &params);
}

static void free_spans(spans_t *spans)
{
	spans_t *iter = spans, *tmp = NULL;
	while (iter != NULL) {
		tmp = iter;
		iter = iter->next;
		free(tmp);
	}
}

/*
 * Return the font bitmap buffer address. NULL if failed.
 */
static u8* set_outline(const u32 outline_width, int* width, int* height)
{
	u32 linesize = outline_width * cfg_font.size;
	FT_GlyphSlot slot = face->glyph;
	u8 *pixel = NULL;
	spans_t *spans = NULL, *outlinespans = NULL;

	// Render the basic glyph to a span list.
	spans = (spans_t *)malloc(sizeof(spans_t));
	if (spans == NULL) {
		printf("spans malloc error\n");
		return NULL;
	}
	spans->span.x = 0;
	spans->span.y = 0;
	spans->span.width = 0;
	spans->span.coverage = 0;
	spans->next = NULL;
	render_spans(&library, &(slot->outline), spans);

	// spans for the outline
	outlinespans = (spans_t *)malloc(sizeof(spans_t));
	if (outlinespans == NULL) {
		printf("outlinespans malloc error\n");
		return NULL;
	}
	outlinespans->span.x = 0;
	outlinespans->span.y = 0;
	outlinespans->span.width = 0;
	outlinespans->span.coverage = 0;
	outlinespans->next = NULL;

	// Set up a stroker
	FT_Stroker stroker;
	FT_Stroker_New(library, &stroker);
	FT_Stroker_Set(stroker,
			linesize,
			FT_STROKER_LINECAP_ROUND,
			FT_STROKER_LINEJOIN_ROUND,
			0);

	FT_Glyph glyph;
	if (FT_Get_Glyph(slot, &glyph) == 0) {
		FT_Glyph_StrokeBorder(&glyph, stroker, 0, 1);   // outside border. Destroyed on success.
		if (glyph->format == FT_GLYPH_FORMAT_OUTLINE) {
			// Render the outline spans to the span list
			FT_Outline *ol = &(((FT_OutlineGlyph)glyph)->outline);
			render_spans(&library, ol, outlinespans);
		}

		// Clean up afterwards.
		FT_Stroker_Done(stroker);
		FT_Done_Glyph(glyph);

		// Put them together
		if (outlinespans->next != NULL) {
			rect_t rect;
			rect.xmin = spans->span.x;
			rect.xmax = spans->span.x;
			rect.ymin = spans->span.y;
			rect.ymax = spans->span.y;

			spans_t *iter = NULL;
			for (iter = spans; iter != NULL; iter = iter->next) {
				rect_include(&rect, iter->span.x, iter->span.y);
				rect_include(&rect, iter->span.x+iter->span.width-1, iter->span.y);
			}

			for (iter = outlinespans; iter != NULL; iter = iter->next) {
				rect_include(&rect, iter->span.x, iter->span.y);
				rect_include(&rect, iter->span.x+iter->span.width-1, iter->span.y);
			}
			int pwidth = rect.xmax - rect.xmin + 1;
			int pheight = rect.ymax - rect.ymin + 1;
			int w = 0;
			pixel = (u8 *)malloc(pwidth * pheight * sizeof(u8));
			if (pixel == NULL) {
				printf("Mem alloc error. Strings may be too long.\n");
				return NULL;
			}
			memset(pixel, pixel_type.pixel_background, pwidth * pheight *sizeof(u8));

			for (iter = outlinespans; iter != NULL; iter = iter->next) {
				for (w = 0; w < (iter->span.width); w++)
					pixel[(int)((pheight-1-(iter->span.y-rect.ymin)) * pwidth + iter->span.x-rect.xmin+w)] = pixel_type.pixel_outline;
			}
			for (iter = spans; iter != NULL; iter = iter->next) {
				for (w = 0; w < (iter->span.width); w++)
					pixel[(int)((pheight-1-(iter->span.y-rect.ymin)) * pwidth + iter->span.x-rect.xmin+w)] = MAX(iter->span.coverage, pixel_type.pixel_outline+1) ;
			}

			*width = pwidth;
			*height = pheight;
		}
	}
	free_spans(outlinespans);
	free_spans(spans);

	return pixel;
}

/*
 * Return 0 if initializing the freetype library successfully, -1 otherwise.
 * The handle library is set to a new instatnce of FreeType library, which loads
 * each module that FreeType knows about in the library.
 *
 */
int text2bitmap_lib_init(pixel_type_t *type)
{
	if (flag_init == 1) {
		printf("text2bitmap_lib_init has been called.\n");
		return 0;
	}

	FT_Error ft_error = FT_Init_FreeType(&library);
	if (ft_error) {
		printf("An error occurred during library initilization!\n");
		return -1;
	}
	if (type == NULL) {
		memcpy(&pixel_type, &default_pixel_type, sizeof(pixel_type_t));
	} else {
		memcpy(&pixel_type, type, sizeof(pixel_type_t));
	}

	flag_init = 1;
	return 0;
}

/*
 * Return 0 if font attribute is set successfully, -1 otherwise.
 */
int text2bitmap_set_font_attribute(const font_attribute_t *font_attr)
{
	FT_Error ft_error;

	if (font_attr == NULL) {
		printf("[text2bitmap_set_font_attribute] NULL pointer !\n");
		return -1;
	}
	if (flag_face == 1) {
		if ((ft_error = FT_Done_Face(face)) != 0) {
			printf("Error! Destroy old attribute failed.\n");
			return -1;
		}
		flag_face = 0;
	}

	ft_error = FT_New_Face(library, font_attr->type, 0, &face);

	if (ft_error == FT_Err_Unknown_File_Format) {
		printf("Error! The font format in %s is unsupported.", font_attr->type);
		return -1;
	} else if (ft_error) {
		printf("Error %d! The font file %s could not be opened or read, or it is broken.\n", ft_error, font_attr->type);
		return -1;
	}
	strncpy(cfg_font.type, font_attr->type, sizeof(font_attr->type));
	cfg_font.size = font_attr->size;
	cfg_font.outline_width = font_attr->outline_width;
	cfg_font.hori_bold = font_attr->hori_bold;
	cfg_font.vert_bold = font_attr->vert_bold;
	cfg_font.italic = font_attr->italic;
	cfg_font.disable_anti_alias = font_attr->disable_anti_alias;
	ft_error = FT_Set_Pixel_Sizes(face, cfg_font.size, 0);
	if (ft_error) {
		printf("Error! Set font size to pixel %d*%d failed.\n", cfg_font.size, cfg_font.size);
		return -1;
	}
	flag_face = 1;
  //  printf("Set font size to pixel %d*%d.\n", cfg_font.size, cfg_font.size);

	return 0;
}

int text2bitmap_get_font_attribute(font_attribute_t * font_attr)
{
	if (font_attr == NULL) {
		printf("[text2bitmap_get_font_attribute] NULL pointer !\n");
		return -1;
	}
	if (flag_face == 0) {
		printf("None font attribute is set.\n");
		return 0;
	}
	strncpy(font_attr->type, cfg_font.type, sizeof(font_attr->type));
	font_attr->size = cfg_font.size;
	font_attr->outline_width = cfg_font.outline_width;
	font_attr->hori_bold = cfg_font.hori_bold;
	font_attr->vert_bold = cfg_font.vert_bold;
	font_attr->italic = cfg_font.italic;
	font_attr->disable_anti_alias = cfg_font.disable_anti_alias;
	return 0;
}

/*
 * Return 0 if font bitmap buffer is filled successfully, -1 otherwise.
 *
 * bmp_addr is the buffer bitmap data is to write.
 * buf_height and buf_pitch for bitmap alignment with the buffer.
 * offset_x is to set  horizontal offset of the character bitmap to the buffer.
 * The bitmap info including height and width will be given out in
 * bitmap_info_t *bmp_info.
 */
int text2bitmap_convert_character(u32 ui_char_code,
								  u8 *bmp_addr,
								  u16 buf_height,
								  u16 buf_pitch,
								  int offset_x,
								  bitmap_info_t *bmp_info)
{
	int ft_error;
	int width, height, left, top, advance_x,advance_y, bmp_pitch = 0;
	u8 *mapped_pixel = NULL;
	FT_GlyphSlot slot = face->glyph;

	if (buf_height < cfg_font.size) {
		printf("The height of the buffer to be filled with bitmap data is smaller than the font size.\n");
		return -1;
	}
	if (bmp_info == NULL) {
		printf("[text2bitmap_convert_character] NULL pointer!\n");
		return -1;
	}

	FT_UInt ui_glyph_index = FT_Get_Char_Index(face, ui_char_code);
	if (cfg_font.italic) {
		set_italic(slot, cfg_font.italic);
	}

	ft_error = FT_Load_Glyph(face, ui_glyph_index, FT_LOAD_NO_BITMAP);
	if (ft_error != 0) {
		printf("Load Glyph error!\n");
		return -1;
	}

/*
	printf("width=%d, height=%d, horiX=%d, horiY=%d, horiA=%d,vertX=%d,vertY=%d,vertA=%d\n",
		width, height, FT_PIX_INT(slot->metrics.horiBearingX), FT_PIX_INT(slot->metrics.vertBearingY),
		FT_PIX_INT(slot->metrics.horiAdvance), FT_PIX_INT(slot->metrics.vertBearingX),
		FT_PIX_INT(slot->metrics.vertBearingY), FT_PIX_INT(slot->metrics.vertAdvance));

	printf("bitmap: width = %d, rows = %d, pitch = %d, top=%d,left=%d, advance.x=%d, advance.y=%d\n",
		slot->bitmap.width, slot->bitmap.rows, slot->bitmap.pitch,
		slot->bitmap_top, slot->bitmap_left,
		FT_PIX_INT(slot->advance.x), FT_PIX_INT(slot->advance.y));
*/

	if (cfg_font.hori_bold || cfg_font.vert_bold) {
		set_bold(slot, cfg_font.vert_bold * cfg_font.size, cfg_font.hori_bold * cfg_font.size);
	}

	width = FT_PIX_INT(slot->metrics.width);
	height = FT_PIX_INT(slot->metrics.height);
	left = FT_PIX_INT(slot->metrics.horiBearingX);
	top = FT_PIX_INT(slot->metrics.horiBearingY);
	advance_x = FT_PIX_INT(slot->metrics.horiAdvance);
	advance_y = FT_PIX_INT(slot->metrics.vertAdvance);
	if (!cfg_font.disable_anti_alias
			&&(cfg_font.outline_width)
			&& (face->glyph->format == FT_GLYPH_FORMAT_OUTLINE)) {
		mapped_pixel = set_outline(cfg_font.outline_width, &bmp_pitch, &height);
	} else {
		FT_Render_Glyph(slot, FT_RENDER_MODE_NORMAL);
		mapped_pixel = slot->bitmap.buffer;
		bmp_pitch = slot->bitmap.pitch;
	}

	if (height > buf_height) {
		printf("Error: converted bitmap height [%d] exceeds the buffer height [%d]\n",
			height, buf_height);
		goto ERROR_QUIT;
	}

	width = MAX(width, bmp_pitch);
	width = MIN(width, buf_pitch - offset_x - left);

	if ((bmp_pitch > 0) && (bmp_addr != NULL)) {
		int row = 0, col = 0, dst_index = 0, src_index = 0;
		u8 * ptr = NULL;
		u32 font_gap = cfg_font.size - top;
	//	printf("\n\n");
		if ((font_gap + height) > buf_height)
			font_gap = buf_height - height;
		for (row = 0; row < height; row++) {
			dst_index = buf_pitch * (row + font_gap) + offset_x;
			src_index = row * bmp_pitch;
			for (col = 0; col < width; col++, dst_index++, src_index++) {
				if (dst_index < 0)
					continue;
				if ((mapped_pixel[src_index]) == 0) {
					//	printf("-\t");
					continue;
				}
				ptr = bmp_addr + dst_index;
				if (cfg_font.disable_anti_alias)
					*ptr = pixel_type.pixel_font;
				else
					*ptr = mapped_pixel[src_index];
			//	printf("%d\t", *ptr);
			}
		//	 printf("\n");
		}
	}

	bmp_info->width = MAX(width + left, advance_x);
	bmp_info->height = MAX(height + top, advance_y);

ERROR_QUIT:
	if (cfg_font.outline_width && (mapped_pixel != NULL)) {
		free(mapped_pixel);
		mapped_pixel = NULL;
	}
	return 0;
}

/*
 * Return 0 if the library is closed.
 */
int text2bitmap_lib_deinit()
{
	if (flag_init == 1) {
		FT_Done_Face(face);
		FT_Done_FreeType(library);
		flag_init = 0;
		flag_face = 0;
	}
	return 0;
}

int text2bitmap_get_version(version_t* version)
{
	memcpy(version, &G_text2bitmap_version, sizeof(version_t));
	return 0;
}

