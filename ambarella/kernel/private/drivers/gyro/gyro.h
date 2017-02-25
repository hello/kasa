/*
 * kernel/private/drivers/ambarella/gyro/mpu6000_interrupt/mpu6000.h
 *
 * History:
 *    2012/12/26 - [Zhenwu Xue] Create
 *
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


#ifndef __MPU6000_H__
#define __MPU6000_H__

#define MPU6000_SPI_BUS_ID		0
#define MPU6000_SPI_CS_ID		1
#define MPU6000_SPI_WRITE_CLK  	(1000 * 1000)
#define MPU6000_SPI_READ_CLK  		(10 * 1000 * 1000)

typedef enum {
        MPU6000_REG_SMPLRT_DIV		= 25,
        MPU6000_REG_CONFIG		= 26,

        MPU6000_REG_INTERRUPT_PIN	= 55,
        MPU6000_REG_INTERRUPT_ENABLE	= 56,
        MPU6000_REG_INTERRUPT_STATUS	= 58,

        MPU6000_REG_ACCEL_XOUT_H		= 59,

        MPU6000_REG_GYRO_XOUT_H		= 67,
        MPU6000_REG_GYRO_YOUT_H		= 69,

        MPU6000_REG_USER_CTRL		= 106,
        MPU6000_REG_PWR_MGMT_1		= 107,

        MPU6000_REG_WHO_AM_I		= 117,
} mpu6000_reg_t;

#define MPU6000_ID			0x68

#define MPU6000_INT_RD_CLEAR		(1 << 4)
#define MPU6000_LATCH_INT_EN		(1 << 5)
#define MPU6000_INT_OPEN_DRAIN	(1 << 6)
#define MPU6000_INT_LEVEL_LOW		(1 << 7)
#define MPU6000_DATA_READY		(1 << 0)

#define ACCEL_GYRO_TOTAL_SIZE 14
#define ACCEL_OFFSET 0
#define GYRO_OFFSET 8

typedef struct {
	struct task_struct		*daemon;
	u32				killing_daemon;
	wait_queue_head_t		waitqueue;
	u32				data_ready;

	gyro_data_t			data;
	spinlock_t			data_lock;

	void				*eis_arg;
	int             busy;
	struct device_node *of_node;
	int irq_gpio;
	int irq_num;
	GYRO_EIS_CALLBACK	eis_callback;
} mpu6000_info_t;

#endif

