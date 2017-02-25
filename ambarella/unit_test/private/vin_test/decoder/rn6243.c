/*
 * rn6243.c
 *
 * History:
 *	2010/12/05 - [Louis Sun] create
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

#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>


static u8 rn6243_registers[][2] = {
	{0x80, 0x31},
		//delay 10ms in between
	{0xDE, 0xE4},
	{0x82, 0x81},
	{0x83, 0x01},
	{0x82, 0x81}, 
	{0xE3, 0x88},
	{0x90, 0x21},
	{0x91, 0x02},
	{0x92, 0x00},
	{0x90, 0x21}, 
	{0x93, 0x21},
	{0x94, 0x02},
	{0x95, 0x00},
	{0x93, 0x21}, 
	{0xFF, 0x00},
	{0x01, 0x00},
	{0x02, 0x78},
	{0x03, 0x80},
	{0xFF, 0x01},
	{0x01, 0x00},
	{0x02, 0x78},
	{0x03, 0x80},
	{0xFF, 0x02},
	{0x01, 0x00},
	{0x02, 0x78},
	{0x03, 0x80},
	{0xFF, 0x03},
	{0x01, 0x00},
	{0x02, 0x78},
	{0x03, 0x80},
	{0x81, 0x0F},
};


static u8 rn6243_registers_weave[][2] = {
	{0x80, 0x31},
		//delay 10ms in between
	{0xDE, 0xE4},
	{0x82, 0x81},
	{0x83, 0x01},
	{0x82, 0x81}, 
	{0xE3, 0x00},
	{0x93, 0x21},
	{0x94, 0x02},
	{0x95, 0x00},
	{0x93, 0x21}, 
	{0xFF, 0x00},
	{0x01, 0x00},
	{0x02, 0x78},
	{0x03, 0x80},
	{0x3A, 0x20},
	{0x3F, 0x10},
	{0xFF, 0x01},
	{0x01, 0x00},
	{0x02, 0x78},
	{0x03, 0x80},
	{0x3A, 0x20},
	{0x3F, 0x11},
	{0xFF, 0x02},
	{0x01, 0x00},
	{0x02, 0x78},
	{0x03, 0x80},
	{0x3A, 0x20},
	{0x3F, 0x12},
	{0xFF, 0x03},
	{0x01, 0x00},
	{0x02, 0x78},
	{0x03, 0x80},
	{0x3A, 0x20},
	{0x3F, 0x13},
	{0x81, 0x0F},
};


/* mode: 0: default   1: weave mode */
static int init_rn6243(int mode)
{
	int i2c_fd, ret;
	int i;
	int addr = 94;  //RN6243 I2C address
	u8 *w8;
	int wsize = 2;
	i2c_fd = open("/dev/i2c-0", O_RDWR, 0);
	
	
	if (i2c_fd < 0) {
		perror("i2c open error \n");		
		return -1;
	}

	ret = ioctl(i2c_fd, I2C_TENBIT, 0);
	if (ret < 0) {
		perror("I2c ioctl error \n");
		return -1;	
	}

	ret = ioctl(i2c_fd, I2C_SLAVE, addr >> 1);
	if (ret < 0) {
		perror("I2c slave setting error \n");
		return -1;	
	}

	if (mode == 0) { 
		printf("fill register for default mode 1080p30 or 1080i60 \n");
		//first line
		w8 = &rn6243_registers[0][0];
		ret = write(i2c_fd, w8, wsize);
		if (ret < 0) {
			perror("i2c write error \n");
			return -1;	
		}
		usleep(20* 1000);
		//rest lines

		for (i=0; i < sizeof (rn6243_registers)/2 -1;  i++)
		{
			w8 = &rn6243_registers[i+1][0];
			ret = write(i2c_fd, w8, wsize);
			if (ret < 0) {
				perror("i2c write error \n");
			}	
		}
	} else {
		printf("fill register for WEAVE mode 1080p30 deinterlacing\n");
		//first line
		w8 = &rn6243_registers_weave[0][0];
		ret = write(i2c_fd, w8, wsize);
		if (ret < 0) {
			perror("i2c write error \n");
			return -1;	
		}
		usleep(20* 1000);
		//rest lines

		for (i=0; i < sizeof (rn6243_registers_weave)/2 -1;  i++)
		{
			w8 = &rn6243_registers_weave[i+1][0];
			ret = write(i2c_fd, w8, wsize);
			if (ret < 0) {
				perror("i2c write error \n");
			}	
		}
	}
	
	printf("all registers set done \n");
	return 0;

}
