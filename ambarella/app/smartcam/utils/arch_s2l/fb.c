/*******************************************************************************
 * fb.c
 *
 * History:
 *   Jun 24, 2015 - [binwang] created file
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/fb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include "fb.h"
#include <basetypes.h>

static int        fd;
static struct fb_fix_screeninfo   finfo;
static struct fb_var_screeninfo   vinfo;
struct fb_cmap        cmap;
static unsigned char      *mem = NULL;

int open_fb(unsigned int w, unsigned int h)
{
  int       i, ret;
  unsigned short      *buf, *r, *g, *b, *a;

  fd = open("/dev/fb0", O_RDWR);
  if (fd < 0) {
    ret = -1;
    goto open_fb_exit;
  }

  ret = ioctl(fd, FBIOGET_FSCREENINFO, &finfo);
  if (ret < 0) {
    ret = -1;
    goto open_fb_exit;
  }

  ret = ioctl(fd, FBIOGET_VSCREENINFO, &vinfo);
  if (ret < 0) {
    ret = -1;
    goto open_fb_exit;
  }

  if (w > vinfo.xres_virtual || h > vinfo.yres_virtual / 2) {
    ret = -1;
    goto open_fb_exit;
  }

  vinfo.xres    = w;
  vinfo.yres    = h;
  vinfo.bits_per_pixel  = 8;
  memset(&vinfo.red, 0, sizeof(vinfo.red));
  memset(&vinfo.green, 0, sizeof(vinfo.green));
  memset(&vinfo.blue, 0, sizeof(vinfo.blue));
  memset(&vinfo.transp, 0, sizeof(vinfo.transp));
  ret = ioctl(fd, FBIOPUT_VSCREENINFO, &vinfo);
  if (ret < 0) {
    ret = -1;
    goto open_fb_exit;
  }

  buf = (unsigned short *)malloc(256 * 4 * sizeof(unsigned short));
  if (!buf) {
    ret = -1;
    goto open_fb_exit;
  }
  r         = buf;
  g         = buf + 256;
  b         = buf + 512;
  a         = buf + 768;

  for (i = 0; i < 256; i++) {
    r[i] = i;
  }
  for (i = 0; i < 256; i++) {
    g[i] = 128;
  }
  for (i = 0; i < 256; i++) {
    b[i] = 128;
  }
  for (i = 0; i < 256; i++) {
    a[i] = 255;
  }

  r[FB_COLOR_TRANSPARENT] = 0;
  g[FB_COLOR_TRANSPARENT] = 0;
  b[FB_COLOR_TRANSPARENT] = 0;
  a[FB_COLOR_TRANSPARENT] = 0;

  r[FB_COLOR_SEMI_TRANSPARENT]  = 0;
  g[FB_COLOR_SEMI_TRANSPARENT]  = 0;
  b[FB_COLOR_SEMI_TRANSPARENT]  = 0;
  a[FB_COLOR_SEMI_TRANSPARENT]  = 128;

  r[FB_COLOR_RED]   = 255;
  g[FB_COLOR_RED]   = 0;
  b[FB_COLOR_RED]   = 0;
  a[FB_COLOR_RED]   = 255;

  r[FB_COLOR_GREEN] = 0;
  g[FB_COLOR_GREEN] = 255;
  b[FB_COLOR_GREEN] = 0;
  a[FB_COLOR_GREEN] = 255;

  r[FB_COLOR_BLUE]  = 0;
  g[FB_COLOR_BLUE]  = 0;
  b[FB_COLOR_BLUE]  = 255;
  a[FB_COLOR_BLUE]  = 255;

  r[FB_COLOR_WHITE] = 255;
  g[FB_COLOR_WHITE] = 255;
  b[FB_COLOR_WHITE] = 255;
  a[FB_COLOR_WHITE] = 255;

  r[FB_COLOR_BLACK] = 0;
  g[FB_COLOR_BLACK] = 0;
  b[FB_COLOR_BLACK] = 0;
  a[FB_COLOR_BLACK] = 255;

  cmap.start  = 0;
  cmap.len  = 256;
  cmap.red  = r;
  cmap.green  = g;
  cmap.blue = b;
  cmap.transp = a;

  ret = ioctl(fd, FBIOPUTCMAP, &cmap);
  if (ret < 0) {
    ret = -1;
    goto open_fb_exit;
  }
  free(buf);

  mem = (unsigned char *)mmap(0, finfo.smem_len, PROT_WRITE, MAP_SHARED, fd, 0);
  if (!mem) {
    ret = -1;
    goto open_fb_exit;
  }

  ret = ioctl(fd, FBIOBLANK, FB_BLANK_NORMAL);
  if (ret < 0) {
    ret = -1;
    munmap(mem, finfo.smem_len);
    goto open_fb_exit;
  }

open_fb_exit:
  if (ret && fd > 0) {
    close(fd);
  }
  return ret;
}

int blank_fb(void)
{
  if (fd < 0) {
    return -1;
  }

  ioctl(fd, FBIOBLANK, FB_BLANK_NORMAL);
  return ioctl(fd, FBIOPAN_DISPLAY, &vinfo);
}

int render_frame(char *d)
{
  int         ret;
  int         i;
  unsigned char   *p;

  if (fd < 0 || !mem) {
    return -1;
  }

  if (vinfo.yoffset) {
    vinfo.yoffset = 0;
    p       = mem;
  } else {
    vinfo.yoffset     = vinfo.yres;
    p       = mem + vinfo.yoffset * finfo.line_length;
  }

  for (i = 0; i < (int)vinfo.yres; i++, d += vinfo.xres, p += finfo.line_length) {
    memcpy(p, d, vinfo.xres);
    asm ("PLD [%0, #128]"::"r" (d));
  }

  ret = ioctl(fd, FBIOPAN_DISPLAY, &vinfo);
  if (ret < 0) {
    return -1;
  }

  return 0;
}

int close_fb(void)
{
  if (fd > 0 && mem) {
    munmap(mem, finfo.smem_len);
    close(fd);
    return 0;
  } else {
    return -1;
  }
}
