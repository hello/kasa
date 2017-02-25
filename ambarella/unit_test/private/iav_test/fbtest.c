/*
 * fbtest.c
 *
 * History:
 *   Dec 30, 2010 - [Oliver Li] created file
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

//#include <string.h>
//#include <stdio.h>
//#include <stdlib.h>
//#include <unistd.h>
//#include <fcntl.h>
//#include <sys/mman.h>
//#include <sys/ioctl.h>
#include <linux/fb.h>
//#include "types.h"

const char *fb_dev = "/dev/fb0";
struct fb_var_screeninfo var;
int fb_dev_pitch;
char *fb_dev_base;
int fb_dev_size;

typedef struct bmp_s {
	FILE *fp;
	int width;
	int height;
	int pitch;
} bmp_t;

bmp_t bmp;

u32 read_dword_le(u8 *buffer)
{
	return (buffer[3] << 24) | (buffer[2] << 16) | (buffer[1] << 8) | buffer[0];
}

u16 read_word_le(u8 *buffer)
{
	return (buffer[1] << 8) | buffer[0];
}

#define ALIGN_CENTER			0
#define ALIGN_TOP_OR_LEFT		1
#define ALIGN_BOTTOM_OR_RIGHT		2

int x_align = ALIGN_BOTTOM_OR_RIGHT;
int y_align = ALIGN_BOTTOM_OR_RIGHT;

int do_show_bitmap(const char *filename)
{
	u8 buffer[54];
	u8 *line_buffer;
	u8 *fb_ptr;
	int i, j;
	int offset;
	int screen_xoff;
	int screen_yoff;
	int pic_width;
	int pic_height;

	if (fread(buffer, sizeof(buffer), 1, bmp.fp) != 1) {
		perror("read1");
		return -1;
	}

	if (buffer[0] != 'B' || buffer[1] != 'M') {
		printf("%s is not a bitmap file\n", filename);
		return -1;
	}

	bmp.width = read_dword_le(buffer + 18);
	bmp.height = read_dword_le(buffer + 22);
	printf("bmp size: %d * %d\n", bmp.width, bmp.height);

	if (read_word_le(buffer + 28) != 24) {
		printf("Please use 24-bit bitmap!\n");
		return -1;
	}

	bmp.pitch = (bmp.width * 3 + 3) & ~3;
	printf("pitch: %d\n", bmp.pitch);

	offset = read_dword_le(buffer + 10);
	fseek(bmp.fp, offset, SEEK_SET);
	printf("seek to %d\n", offset);

	line_buffer = malloc(bmp.pitch);
	if (line_buffer == NULL) {
		printf("Cannot alloc %d bytes\n", bmp.pitch);
		return -1;
	}

	if (var.xres > bmp.width) {
		pic_width = bmp.width;

		switch (x_align) {
		case ALIGN_CENTER: screen_xoff = (var.xres - bmp.width) / 2; break;
		case ALIGN_TOP_OR_LEFT: screen_xoff = 0; break;
		case ALIGN_BOTTOM_OR_RIGHT:
		default: screen_xoff = var.xres - bmp.width; break;
		}
	} else {
		screen_xoff = 0;
		pic_width = var.xres;
	}

	if (var.yres > bmp.height) {
		pic_height = bmp.height;

		switch (y_align) {
		case ALIGN_CENTER: screen_yoff = (var.yres - bmp.height) / 2; break;
		case ALIGN_TOP_OR_LEFT: screen_yoff = 0; break;
		case ALIGN_BOTTOM_OR_RIGHT:
		default: screen_yoff = var.yres - bmp.height; break;
		}
	} else {
		screen_yoff = 0;
		pic_height = var.yres;
	}

	printf("draw %d %d %d %d\n", screen_xoff, screen_yoff, pic_width, pic_height);

	fb_ptr = (u8*)fb_dev_base + (screen_yoff + pic_height) * fb_dev_pitch + screen_xoff * (var.bits_per_pixel >> 3);

	if (var.bits_per_pixel == 16) {
		for (i = 0; i < pic_height; i++) {
			u8 *ptr = fb_ptr;
			u8 *pline;

			fb_ptr -= fb_dev_pitch;
			ptr = fb_ptr;

			if (fread(line_buffer, bmp.pitch, 1, bmp.fp) != 1) {
				perror("read2");
				goto Error;
			}

			pline = line_buffer;
			for (j = 0; j < pic_width; j++) {
				unsigned b = *pline++;
				unsigned g = *pline++;
				unsigned r = *pline++;
				b >>= 3; g >>= 2; r >>= 3;
				*(u16*)ptr = (r << 11) | (g << 5) | b;
				//*(u16*)ptr = (r << 11) | ((g & 0x3F) << 5) | (b & 0x1F);
				ptr += 2;
			}
		}
	} else if (var.bits_per_pixel == 32) {
		for (i = 0; i < pic_height; i++) {
			u8 *ptr = fb_ptr;
			u8 *pline;

			fb_ptr -= fb_dev_pitch;
			ptr = fb_ptr;

			if (fread(line_buffer, bmp.pitch, 1, bmp.fp) != 1) {
				perror("read2");
				goto Error;
			}

			pline = line_buffer;
			for (j = 0; j < pic_width; j++) {
				unsigned b = *pline++;
				unsigned g = *pline++;
				unsigned r = *pline++;
				*(u32*)ptr = (255 << 24) | (r << 16) | (g << 8) | b;
				ptr += 4;
			}
		}
	}

	free(line_buffer);
	printf("render file done\n");

	return 0;

Error:
	free(line_buffer);
	return -1;
}

int show_bitmap(const char *filename, int clear)
{
	int fd;
	int rval = 0;

	fd = open(fb_dev, O_RDWR);
	if (fd > 0) printf("open %s\n", fb_dev);
	else {
		perror(fb_dev);
		return -1;
	}

	if (ioctl(fd, FBIOGET_VSCREENINFO, &var) < 0) {
		perror("FBIOGET_VSCREENINFO");
		rval = -1;
		goto Error1;
	}

	printf("xres: %d, yres: %d, bpp: %d\n", var.xres, var.yres, var.bits_per_pixel);

	fb_dev_pitch = var.xres * (var.bits_per_pixel >> 3);
	fb_dev_size = fb_dev_pitch * var.yres;
	fb_dev_base = mmap(NULL, fb_dev_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (fb_dev_base == MAP_FAILED) {
		perror("mmap");
		rval = -1;
		goto Error1;
	}

	if (clear) {
		printf("clear osd\n");
		memset(fb_dev_base, 0, fb_dev_size);
	}

	if (filename && filename[0]) {
		bmp.fp  = fopen(filename, "r");
		if (bmp.fp == NULL) {
			perror(filename);
			rval = -1;
			goto Error2;
		}

		do_show_bitmap(filename);

		fclose(bmp.fp);
	}

	if (ioctl(fd, FBIOPAN_DISPLAY, &var) < 0) {
		perror("FBIOPAN_DISPLAY");
		rval = -1;
		goto Error2;
	}

	rval = 0;

Error2:
	if (munmap(fb_dev_base, fb_dev_size) < 0) {
		perror("munmap");
	}

Error1:
	close(fd);
	return rval;
}

