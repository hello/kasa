/**
 * adc.h
 *
 * Author: Roy Su <qiangsu@ambarella.com>
 *
 * Copyright (C) 2014-2016, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __AMBOOT_ADC_H__
#define __AMBOOT_ADC_H__

//add for smart 3a params in adc partition
#define ADCFW_IMG_MAGIC			(0x43525243)

struct smart3a_file_info { /* 4 + (4 * 5) + 5 + 3 = 32 */
	unsigned int	offset;
	/* AWB */
	unsigned int	r_gain;
	unsigned int	b_gain;
	/* AE */
	unsigned int	d_gain;
	unsigned int	shutter;
	unsigned int	agc;
	/* Sensor 3 Shutter Register */
	unsigned char para0;
	unsigned char para1;
	unsigned char para2;
	/* Sensor 2 AGC Register */
	unsigned char para3;
	unsigned char para4;

	unsigned char rev[3];
}__attribute__((packed));


struct video_param_info { /* 4 + 4 + 4  + 4 = 16 */
      unsigned int enable_video_param;
      unsigned int video_mode;
      unsigned int bitrate_quality;
      unsigned int res;
}__attribute__((packed));

struct adcfw_header { /* 4 + 4 + 2 + 2 + 20 + (32 * 4) + 96 = 256 */
	unsigned int	magic;
	unsigned int	fw_size; /* totally fw size including the header */
	unsigned short	smart3a_num;
	unsigned short	smart3a_size;
	unsigned char head_rev[20];
	struct smart3a_file_info smart3a[4];
       struct video_param_info video_param;
	unsigned char rev[80];
}__attribute__((packed));

#endif
