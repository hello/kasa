/*
 * test_osd.c
 *
 * History:
 *	2009/11/20 - [Zhenwu Xue] Initial Revision
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

    #include <unistd.h>
    #include <stdio.h>
    #include <fcntl.h>
    #include <linux/fb.h>
    #include <sys/mman.h>
    #include <sys/ioctl.h>
    #include <stdlib.h>

    int main(int argc, char **argv)
    {
        int				fbfd = 0;
        struct fb_var_screeninfo	vinfo;
        struct fb_fix_screeninfo	finfo;
        long int			screensize = 0;
        char				*fbp = 0;
        int				x = 0, y = 0;
        int				x_start = 0, y_start = 0;
	int				width = 0, height = 0;
	int				color = 0;
        long int			location = 0;
	char				buf[32];

	if (argc != 7) {
		printf("Usage: %s fb_id x y width height color\n", argv[0]);
		return 0;
	}

        // Open the file for reading and writing
        sprintf(buf, "/dev/fb%s", argv[1]);
        fbfd = open(buf, O_RDWR);
        if (!fbfd) {
		printf("Error: cannot open %s!\n", buf);
		return -1;
        } else {
	        printf("The framebuffer device was opened successfully.\n");
        }

        // Get fixed screen information
        if (ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo)) {
		printf("Error reading fixed information.\n");
		return -2;
        }

        // Get variable screen information
        if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo)) {
		printf("Error reading variable information.\n");
		return -3;
        } else {
        	printf("%dx%d, %dbpp\n", vinfo.xres, vinfo.yres, vinfo.bits_per_pixel);
        }

        // Figure out the size of the screen in bytes
        screensize = vinfo.yres * finfo.line_length;

        // Map the device to memory
        fbp = (char *)mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED,
                           fbfd, 0);
        if (!fbp) {
		printf("Error: failed to map framebuffer device to memory.\n");
		return -4;
        } else {
        	printf("The framebuffer device was mapped to memory successfully.\n");
        }

	x_start	= atoi(argv[2]);
	y_start	= atoi(argv[3]);
	width	= atoi(argv[4]);
	height	= atoi(argv[5]);
	sscanf(argv[6], "%x", &color);

        // Figure out where in memory to put the pixel
        for (y = y_start; y < (y_start + height); y++) {
		for (x = x_start; x < (x_start + width); x++) {
			location = (x + vinfo.xoffset) * (vinfo.bits_per_pixel / 8) +
                           (y + vinfo.yoffset) * finfo.line_length;
			if (vinfo.bits_per_pixel == 8) {
				*((unsigned char *)(fbp + location)) = color;
			}
			if (vinfo.bits_per_pixel == 16) {
				*((unsigned short *)(fbp + location)) = color;
			}
			if (vinfo.bits_per_pixel == 32) {
				*((unsigned int *)(fbp + location)) = color;
			}
		}
	}

        munmap(fbp, screensize);
	ioctl(fbfd, FBIOPAN_DISPLAY, &vinfo);
        close(fbfd);

        return 0;
    }
