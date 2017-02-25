/*******************************************************************************
 * lcd_st7789v.c
 *
 * History:
 *    2016/03/11 - [BingLiang Hu] created file
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
 *******************************************************************************/
/* ========================================================================== */
#include <linux/spi/spidev.h>


/* ========================================================================== */


#define delay(_ms)	usleep((_ms)*1000)

typedef enum {
	GPIO_UNEXPORT = 0,
	GPIO_EXPORT = 1
} gpio_ex;

typedef enum {
	GPIO_IN = 0,
	GPIO_OUT = 1
} gpio_direction;

typedef enum {
	GPIO_LOW = 0,
	GPIO_HIGH = 1
} gpio_state;

s32 csb_fd = -1;
s32 sdi_fd = -1;
s32 sclb_fd = -1;
s32 res_fd = -1;
s32 bl_fd = -1;

#define CSB_GPIO 49
#define SDI_GPIO 47
#define SCLB_GPIO 46
#define RES_GPIO 53
#define BL_GPIO 113

s32 set_gpio_state(s32 *fd, s32 gpio_id, u32 state);

#define csb_low() set_gpio_state(&csb_fd, CSB_GPIO, GPIO_LOW)
#define csb_high() set_gpio_state(&csb_fd, CSB_GPIO, GPIO_HIGH)

#define sdi_low() set_gpio_state(&sdi_fd, SDI_GPIO, GPIO_LOW)
#define sdi_high() set_gpio_state(&sdi_fd, SDI_GPIO, GPIO_HIGH)

#define sclb_low() set_gpio_state(&sclb_fd, SCLB_GPIO, GPIO_LOW)
#define sclb_high() set_gpio_state(&sclb_fd, SCLB_GPIO, GPIO_HIGH)

#define res_low() set_gpio_state(&res_fd, RES_GPIO, GPIO_LOW)
#define res_high() set_gpio_state(&res_fd, RES_GPIO, GPIO_HIGH)

#define bl_low() set_gpio_state(&bl_fd, BL_GPIO, GPIO_LOW)
#define bl_high() set_gpio_state(&bl_fd, BL_GPIO, GPIO_HIGH)

void write_command(unsigned char y)
{
	unsigned char i;
	csb_low();
	sclb_low();
	sdi_low();
	sclb_high();
	for (i = 0; i < 8; i++) {
		sclb_low();
		if (y & 0x80) {
			sdi_high();
		}
		else {
			sdi_low();
		}
		sclb_high();
		y = y << 1;
	}
	csb_high();
}

void write_data(unsigned char w)
{
	unsigned char i;
	csb_low();
	sclb_low();
	sdi_high();
	sclb_high();
	for (i = 0; i < 8; i++)
	{
		sclb_low();
		if (w & 0x80) {
			sdi_high();
		}
		else {
			sdi_low();
		}
		sclb_high();
		w = w << 1;
	}
	csb_high();
}

void initi(void)
{
	res_high();
	delay(1);
	res_low();
	delay(10);
	res_high();
	delay(120);
	bl_high();
	//--------------------------------ST7789S Frame rate setting----------------------------------//
	write_command(0x36);
	write_data(0x00);
	//--------------------------------ST7789S Frame rate setting----------------------------------//
	write_command(0xb2);
	write_data(0x05);
	write_data(0x05);
	write_data(0x00);
	write_data(0x33);
	write_data(0x33);

	write_command(0xb7);
	write_data(0x35);

	//---------------------------------ST7789S Power setting--------------------------------------//

	write_command(0xbb);
	write_data(0x3F);           //vcom

	write_command(0xc0);
	write_data(0x2C);           //17调深浅

	write_command(0xc2);
	write_data(0x01);           //17调深浅

	write_command(0xc3);
	write_data(0x0F);           //17调深浅

	write_command(0xc4);
	write_data(0x20);

	write_command(0xc6);
	write_data(0x11);

	write_command(0xd0);
	write_data(0xa4);
	write_data(0xa1);

	write_command(0xe8);
	write_data(0x03);

	write_command(0xe9);
	write_data(0x09);
	write_data(0x09);
	write_data(0x08);
	//--------------------------------ST7789S gamma setting---------------------------------------//
	write_command(0xe0);
	write_data(0xd0);
	write_data(0x05);
	write_data(0x09);
	write_data(0x09);
	write_data(0x08);
	write_data(0x14);
	write_data(0x28);
	write_data(0x33);
	write_data(0x3F);
	write_data(0x07);
	write_data(0x13);
	write_data(0x14);
	write_data(0x28);
	write_data(0x30);

	write_command(0xe1);
	write_data(0xd0);
	write_data(0x05);
	write_data(0x09);
	write_data(0x09);
	write_data(0x08);
	write_data(0x03);
	write_data(0x24);
	write_data(0x32);
	write_data(0x32);
	write_data(0x3B);
	write_data(0x38);
	write_data(0x14);
	write_data(0x13);
	write_data(0x28);
	write_data(0x2F);

	write_command(0x21);        //反显

	//*********SET RGB Interfae***************
	write_command(0xB0);
	write_data(0x11);           //set RGB interface and DE mode.
	write_data(0x00);
	write_data(0x00);

	write_command(0xB1);
	write_data(0xEC);           // set DE mode ; SET Hs,Vs,DE,DOTCLK signal polarity
	write_data(0x04);
	write_data(0x14);

	write_command(0x3a);
	write_data(0x55);           //18 RGB ,55-16BIT RGB

	//************************
	write_command(0x11);
	delay(120);                 //Delay 120ms

	write_command(0x29);        //display on
	write_command(0x2c);
}

s32 do_gpio_export(s32 gpio_id, gpio_ex ex)
{
	s32 ret = -1;
	s32 fd = -1;
	char buf[128] = {0};
	char vbuf[4] = {0};

	sprintf(buf, "/sys/class/gpio/%s", ex == GPIO_EXPORT ? "export" : "unexport");
	if ((fd = open(buf, O_WRONLY)) < 0) {
		perror("do_gpio_export open");
	} else {
		sprintf(vbuf, "%d", gpio_id);
		if (strlen(vbuf) != write(fd, vbuf, strlen(vbuf))) {
			perror("write");
		} else {
			ret = 0;
		}
		close(fd);
	}

	return ret;
}

s32 set_gpio_direction(s32 gpio_id, gpio_direction direction)
{
	s32 ret = -1;
	s32 fd = -1;
	char buf[128] = {0};
	char *vbuf = NULL;

	sprintf(buf, "/sys/class/gpio/gpio%d/direction", gpio_id);
	if ((fd = open(buf, O_WRONLY)) < 0) {
		perror("set_gpio_direction open");
	} else {
		switch(direction) {
			case GPIO_OUT:
				vbuf = "out";
				break;
			case GPIO_IN:
				vbuf = "in";
				break;
			default:
				fprintf(stderr, "Invalid direction!\n");
				break;
		}
		if (vbuf) {
			if (strlen(vbuf) != write(fd, vbuf, strlen(vbuf))) {
				perror("write");
			} else {
				ret = 0;
			}
		}
		close(fd);
	}

	return ret;
}

s32 set_gpio_state(s32 *fd, s32 gpio_id, u32 state)
{
	char buf[128] = {0};
	char *vbuf = NULL;

	sprintf(buf, "/sys/class/gpio/gpio%d/value", gpio_id);
	if (*fd < 0) {
		if ((*fd = open(buf, O_RDWR)) < 0) {
			perror("set_gpio_state open");
			return -1;
		}
	}
	if (state == GPIO_LOW) {
		vbuf = "0";
	} else if (state == GPIO_HIGH) {
		vbuf = "1";
	} else {
		fprintf(stderr, "Invalid gpio state: %d\n", state);
		return -1;
	}
	if (vbuf) {
		if (strlen(vbuf) != write(*fd, vbuf, strlen(vbuf))) {
			perror("write");
			return -1;
		} else {
			lseek(*fd, 0, SEEK_SET);
		}
	}

	return 0;
}

void gpio_init(void)
{
	do_gpio_export(CSB_GPIO, GPIO_EXPORT);
	do_gpio_export(SDI_GPIO, GPIO_EXPORT);
	do_gpio_export(SCLB_GPIO, GPIO_EXPORT);
	do_gpio_export(RES_GPIO, GPIO_EXPORT);
	do_gpio_export(BL_GPIO, GPIO_EXPORT);
	set_gpio_direction(CSB_GPIO, GPIO_OUT);
	set_gpio_direction(SDI_GPIO, GPIO_OUT);
	set_gpio_direction(SCLB_GPIO, GPIO_OUT);
	set_gpio_direction(RES_GPIO, GPIO_OUT);
	set_gpio_direction(BL_GPIO, GPIO_OUT);
}

void gpio_unexport(void)
{
	do_gpio_export(CSB_GPIO, GPIO_UNEXPORT);
	do_gpio_export(SDI_GPIO, GPIO_UNEXPORT);
	do_gpio_export(SCLB_GPIO, GPIO_UNEXPORT);
	do_gpio_export(RES_GPIO, GPIO_UNEXPORT);
	do_gpio_export(BL_GPIO, GPIO_UNEXPORT);
}

static int lcd_st7789v_config_240x320(void)
{
	gpio_init();
	delay(100);
	initi();

	gpio_unexport();
	if(csb_fd > 0)
		close(csb_fd);
	if(sdi_fd > 0)
		close(sdi_fd);
	if(sclb_fd > 0)
		close(sclb_fd);
	return 0;
}


static int lcd_st7789v_240x320(struct amba_video_sink_mode *pvout_cfg,
	enum amba_vout_lcd_mode_info lcd_mode)
{
	pvout_cfg->mode		= AMBA_VIDEO_MODE_240_320;
	pvout_cfg->ratio	= AMBA_VIDEO_RATIO_4_3;
	pvout_cfg->bits		= AMBA_VIDEO_BITS_16;
	pvout_cfg->type		= AMBA_VIDEO_TYPE_RGB_601;
	pvout_cfg->format	= AMBA_VIDEO_FORMAT_PROGRESSIVE;
	pvout_cfg->sink_type	= AMBA_VOUT_SINK_TYPE_DIGITAL;

	pvout_cfg->bg_color.y	= 0x10;
	pvout_cfg->bg_color.cb	= 0x80;
	pvout_cfg->bg_color.cr	= 0x80;

	pvout_cfg->lcd_cfg.mode	= lcd_mode;
	pvout_cfg->lcd_cfg.seqt	= AMBA_VOUT_LCD_SEQ_R0_G1_B2;
	pvout_cfg->lcd_cfg.seqb	= AMBA_VOUT_LCD_SEQ_R0_G1_B2;
	pvout_cfg->lcd_cfg.dclk_edge = AMBA_VOUT_LCD_CLK_RISING_EDGE;
	pvout_cfg->lcd_cfg.dclk_freq_hz	= 7000000;
	pvout_cfg->lcd_cfg.hvld_pol = AMBA_VOUT_LCD_HVLD_POL_HIGH;

	lcd_st7789v_config_240x320();
	return 0;
}

int lcd_st7789v_16_setmode(int mode, struct amba_video_sink_mode *pcfg)
{
	int				errCode = 0;

	switch(mode) {
	case AMBA_VIDEO_MODE_240_320:
		errCode = lcd_st7789v_240x320(pcfg, AMBA_VOUT_LCD_MODE_RGB565);
		break;

	default:
		errCode = -1;
	}

	return errCode;
}

