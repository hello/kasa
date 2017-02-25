/*******************************************************************************
 * lcd_ili8961.c
 *
 * History:
 *    2010/09/28 - [Zhenwu Xue] created file
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
static void lcd_ili8961_config_960x240()
{
	int status = 0;
	int spi_fd,ret = 0,size;
	int mode = 0, bits = 16, speed = 1200 * 1000;

	status = system("/usr/local/bin/amba_debug -g 9 -d 0x1");// workaround to  open backlight

	if (WEXITSTATUS(status)) {
		printf("result verify: %s failed!\n", "amba_debug");
	}

#if 1   // config registers

	spi_fd = open("/dev/spidev0.0", O_RDWR, 0);

	if(spi_fd<0){
		perror("can't open spi node!\n");
		goto lcd_ili8961_config_960x240_exit;
	}


	ret = ioctl(spi_fd, SPI_IOC_WR_MODE, &mode);
	if (ret < 0) {
		perror("can't set spi mode!\n");
		goto lcd_ili8961_config_960x240_exit;
	}

	ret = ioctl(spi_fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
	if (ret < 0) {
		perror("can't set bits per word!\n");
		goto lcd_ili8961_config_960x240_exit;
	}

	ret = ioctl(spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
	if (ret < 0) {
		perror("can't set max speed hz!\n");
		goto lcd_ili8961_config_960x240_exit;
	}

	{
		/*
			wr_cmd(0x05,0x5F);
			Delayms(5);
			wr_cmd(0x05,0x1F);//reset
			Delayms(10);
			wr_cmd(0x05,0x5F);
			Delayms(50);
			wr_cmd(0x2B,0x01);//exit standby mode
			wr_cmd(0x00,0x09);//VCOMAC
			wr_cmd(0x01,0x9F);//VCOMDC
			wr_cmd(0x04,0x09);//8-bit RGB interface
			wr_cmd(0x16,0x04);//Default Gamma setting  2.2
		*/

		#define REG_NUM 8

		u8   dly[REG_NUM]={5,10,50,0,0,0,0,0},cnt;
		u16  reg[REG_NUM] ={0x55f,
					  0x051f,
					  0x055f,
					  0x2b01,
					  0x0009,
					  0x019f,
					  //0x2f71,
					  0x040b,  //0x409,
					  0x1604
						};
		u16	*w16 = NULL;
		size=1;

		for(cnt=0;cnt<REG_NUM;cnt++){

			w16=reg+cnt;
			ret = write(spi_fd, w16, size << 1);
			if (ret != (size << 1)) {
				perror("write error!\n");
				goto lcd_ili8961_config_960x240_exit;
			}

			if(dly[cnt]!=0)
			 	usleep(dly[cnt]*1000);
		}
	}

#endif

lcd_ili8961_config_960x240_exit:

	return;
}

/* ========================================================================== */


static int lcd_ili8961_960x240(struct amba_video_sink_mode *pvout_cfg,
	enum amba_vout_lcd_mode_info lcd_mode)
{
	pvout_cfg->mode		= AMBA_VIDEO_MODE_960_240;
	pvout_cfg->ratio	= AMBA_VIDEO_RATIO_16_9;
	pvout_cfg->bits		= AMBA_VIDEO_BITS_8;
	pvout_cfg->type		= AMBA_VIDEO_TYPE_RGB_601;
	pvout_cfg->format	= AMBA_VIDEO_FORMAT_PROGRESSIVE;
	pvout_cfg->sink_type	= AMBA_VOUT_SINK_TYPE_DIGITAL;

	pvout_cfg->bg_color.y	= 0x10;
	pvout_cfg->bg_color.cb	= 0x80;
	pvout_cfg->bg_color.cr	= 0x80;

	pvout_cfg->lcd_cfg.mode	= lcd_mode;
	pvout_cfg->lcd_cfg.seqt	= AMBA_VOUT_LCD_SEQ_R0_G1_B2;
	pvout_cfg->lcd_cfg.seqb	= AMBA_VOUT_LCD_SEQ_G0_B1_R2; // AMBA_VOUT_LCD_SEQ_R0_G1_B2
	pvout_cfg->lcd_cfg.dclk_edge = AMBA_VOUT_LCD_CLK_RISING_EDGE;
	pvout_cfg->lcd_cfg.dclk_freq_hz	= 27000000;
	pvout_cfg->lcd_cfg.hvld_pol = AMBA_VOUT_LCD_HVLD_POL_HIGH;

	lcd_ili8961_config_960x240();  // open backlight + config registers

	return 0;
}


int lcd_ili8961_setmode(int mode, struct amba_video_sink_mode *pcfg)
{
	int				errCode = 0;

	switch(mode) {
	case AMBA_VIDEO_MODE_960_240:
		//errCode = lcd_ili8961_960x240(pcfg, AMBA_VOUT_LCD_MODE_1COLOR_PER_DOT);
		errCode = lcd_ili8961_960x240(pcfg, AMBA_VOUT_LCD_MODE_1COLOR_PER_DOT);
		break;

	default:
		errCode = -1;
	}

	return errCode;
}

