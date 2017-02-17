/**
 * adc.h
 *
 * Author: Sky Chen <chuchen@ambarella.com>
 *
 * Copyright (c) 2015 Ambarella, Inc.
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

#ifndef __AMBOOT_ADC_H__
#define __AMBOOT_ADC_H__

/* add for smart 3a params in adc partition */
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


struct params_info { /* (4 * 1) + (4 * 1) + 4 + 4 + 4 + 4 + 4 + 4 + 16  + 4 + 4 + 4 + 4= 64 */
	unsigned char enable_params;
	unsigned char enable_dual_stream;
	unsigned char enable_fastosd;
	unsigned char enable_ldc;

	unsigned char enable_stream0_recording_smartavc;
	unsigned char enable_stream1_recording_smartavc;
	unsigned char enable_stream0_streaming_smartavc;
	unsigned char enable_stream0_streaming_svct;

	unsigned int stream0_recording_mode;
	unsigned int stream0_recording_bitrate;

	unsigned int stream1_recording_mode;
	unsigned int stream1_recording_bitrate;

	unsigned int stream0_streaming_resolution;
	unsigned int stream0_streaming_bitrate;
	char fastosd_string[16];

	int timezone;//unit: hours.  regions East of UTC have positive numbers
	int rotation_mode;
	int smart3a_strategy;
	unsigned int light_value;

	int vca_mode;
	int vca_frame_num;

}__attribute__((packed));

struct adcfw_header { /* 4 + 4 + 2 + 2 + 20 + (32 * 5) + 64 + 32 = 288 */
	unsigned int	magic;
	unsigned int	fw_size; /* totally fw size including the header */
	unsigned short	smart3a_num;
	unsigned short	smart3a_size;
	unsigned char head_rev[20];
	struct smart3a_file_info smart3a[5];
	struct params_info params_in_amboot;
	unsigned char rev[32];
}__attribute__((packed));

#endif
