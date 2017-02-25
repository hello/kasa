/*
 * kernel/private/include/vin_sensors.h
 *
 * History:
 *    2012/05/13 - [Rongrong Cao] Create
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

#ifndef __VIN_SENSORS_H__
#define __VIN_SENSORS_H__

/* SENSOR IDs */
#define GENERIC_SENSOR			(127)	//changed to 127 in new DSP firmware

/* RGB */
#define SENSOR_RGB_1PIX			(0)
#define SENSOR_RGB_2PIX			(1)
/* YUV */
#define SENSOR_YUV_1PIX			(2)
#define SENSOR_YUV_2PIX			(3)

/* YUV pixel order */
#define SENSOR_CR_Y0_CB_Y1		(0)
#define SENSOR_CB_Y0_CR_Y1		(1)
#define SENSOR_Y0_CR_Y1_CB		(2)
#define SENSOR_Y0_CB_Y1_CR		(3)

/* Interface type */
#define SENSOR_PARALLEL_LVCMOS		(0)
#define SENSOR_SERIAL_LVDS		(1)
#define SENSOR_PARALLEL_LVDS		(2)
#define SENSOR_MIPI			(3)

/* Lane number */
#define SENSOR_1_LANE			(1)
#define SENSOR_2_LANE			(2)
#define SENSOR_3_LANE			(3)
#define SENSOR_4_LANE			(4)
#define SENSOR_6_LANE			(6)
#define SENSOR_8_LANE			(8)
#define SENSOR_10_LANE			(10)
#define SENSOR_12_LANE			(12)

/* Sync code style */
#define SENSOR_SYNC_STYLE_SONY		(0)
#define SENSOR_SYNC_STYLE_HISPI		(1)
#define SENSOR_SYNC_STYLE_ITU656		(2)
#define SENSOR_SYNC_STYLE_PANASONIC	(3)
#define SENSOR_SYNC_STYLE_SONY_DOL	(4)
#define SENSOR_SYNC_STYLE_HISPI_PSP	(5)
#define SENSOR_SYNC_STYLE_INTERLACE	(6)

/* vsync/hsync polarity */
#define SENSOR_VS_HIGH			(0x1 << 1)
#define SENSOR_VS_LOW			(0x0 << 1)
#define SENSOR_HS_HIGH			(0x1 << 0)
#define SENSOR_HS_LOW			(0x0 << 0)

/* data dege */
#define SENSOR_DATA_RISING_EDGE	(0)
#define SENSOR_DATA_FALLING_EDGE	(1)

/* sync mode */
#define SENSOR_SYNC_MODE_MASTER	(0)
#define SENSOR_SYNC_MODE_SLAVE	(1)

/* paralle_sync_type */
#define SENSOR_PARALLEL_SYNC_656	(0)
#define SENSOR_PARALLEL_SYNC_601	(1)

/* mipi clock bit rate */
#define SENSOR_MIPI_BIT_RATE_L	(0)
#define SENSOR_MIPI_BIT_RATE_H	(1)

#endif

