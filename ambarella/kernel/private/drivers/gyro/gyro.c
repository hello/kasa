/*
 * kernel/private/drivers/ambarella/gyro/mpu6000/mpu6000.c
 * This version use MPU6000 to generate interrupt, but use SPI polling mode to read within tasklet
 * The purpose is to avoid using workqueue and avoid the wait for SPI interrupt mode, in order to
 * get best real time performance
 *
 * History:
 *    2012/12/26 - [Zhenwu Xue] Create
 *    2013/09/25 - [Louis Sun] Modify it to use tasklet to read SPI
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

#include <config.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <mach/hardware.h>
#include <linux/bitrev.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <plat/spi.h>
#include <plat/iav_helper.h>
#include <amba_eis.h>
#include "gyro.h"

mpu6000_info_t	*pinfo = NULL;
static int sample_id = 0;

enum {
	AXIS_X_NEG = 0,
	AXIS_X_POS,
	AXIS_Y_NEG,
	AXIS_Y_POS,
	AXIS_Z_NEG,
	AXIS_Z_POS,
	AXIS_NUM,
	AXIS_FIRST = AXIS_X_NEG,
	AXIS_LAST = AXIS_NUM,
};

/* On S2L hawthorn, Mapping is
 * X_map : +Y
 * Y_map : -X
 * Z_map : +Z
 */

static int x_axis = AXIS_Y_POS;
module_param(x_axis, int, 0644);
MODULE_PARM_DESC(x_axis, "X axis direction - 0:-X, 1:+X, 2:-Y, 3:+Y, 4:-Z, 5:+Z");

static int y_axis = AXIS_X_NEG;
module_param(y_axis, int, 0644);
MODULE_PARM_DESC(y_axis, "Y axis direction - 0:-X, 1:+X, 2:-Y, 3:+Y, 4:-Z, 5:+Z");

static int z_axis = AXIS_Z_POS;
module_param(z_axis, int, 0644);
MODULE_PARM_DESC(z_axis, "Z axis direction - 0:-X, 1:+X, 2:-Y, 3:+Y, 4:-Z, 5:+Z");

void gyro_register_eis_callback(GYRO_EIS_CALLBACK cb, void *arg)
{
	pinfo->eis_callback = cb;
	pinfo->eis_arg = arg;
}
EXPORT_SYMBOL(gyro_register_eis_callback);

void gyro_unregister_eis_callback(void)
{
	pinfo->eis_callback = NULL;
	pinfo->eis_arg = NULL;
}
EXPORT_SYMBOL(gyro_unregister_eis_callback);


static int mpu6000_write_reg(u8 subaddr, u8 data)
{
	static u8			tx[2];
	static amba_spi_cfg_t		config;
	static amba_spi_write_t	write;
	static int init_flag = 0;

	tx[0]			= subaddr & 0x7F;
	tx[1]			= data;

	if (!init_flag) {
		config.cfs_dfs		= 8;
		config.baud_rate	= MPU6000_SPI_WRITE_CLK;
		config.cs_change	= 1;
		config.spi_mode		= SPI_MODE_3;

		write.buffer		= tx;
		write.n_size		= sizeof(tx);
		write.bus_id		= MPU6000_SPI_BUS_ID;
		write.cs_id		= MPU6000_SPI_CS_ID;
		init_flag = 1;
	}

	return ambarella_spi_write(&config, &write);
}

static int mpu6000_read_reg(u8 subaddr, u8 *data, int len)
{
	static u8				tx[1];
	static amba_spi_cfg_t			config;
	static amba_spi_write_then_read_t	wr;
	static int init_flag = 0;

	tx[0] = subaddr | 0x80;
	wr.r_buffer		= data;
	wr.r_size		= len;

	if (!init_flag) {
		config.cfs_dfs		= 8;
		config.baud_rate	= MPU6000_SPI_READ_CLK;
		config.cs_change	= 1;
		config.spi_mode		= SPI_MODE_3;

		wr.w_buffer 	= tx;
		wr.w_size		= sizeof(tx);
		wr.bus_id		= MPU6000_SPI_BUS_ID;
		wr.cs_id		= MPU6000_SPI_CS_ID;
		init_flag = 1;
	}

	return ambarella_spi_write_then_read(&config, &wr);
}

static inline s16 map_gryro_value(s16 x, s16 y, s16 z, int axis_direction)
{
	switch (axis_direction) {
	case AXIS_X_NEG:
		return -x;
	case AXIS_X_POS:
		return x;
	case AXIS_Y_NEG:
		return -y;
	case AXIS_Y_POS:
		return y;
	case AXIS_Z_NEG:
		return -z;
	case AXIS_Z_POS:
		return z;
	default:
		return 0;
	}
}

static void mpu6000_read_data(gyro_data_t *pdata)
{
	u8 data[ACCEL_GYRO_TOTAL_SIZE];
	s16 x, y, z;

	mpu6000_read_reg(MPU6000_REG_ACCEL_XOUT_H, data, ACCEL_GYRO_TOTAL_SIZE);
	x = (s16)((data[ACCEL_OFFSET+0] << 8) | data[ACCEL_OFFSET+1]);
	y = (s16)((data[ACCEL_OFFSET+2] << 8) | data[ACCEL_OFFSET+3]);
	z = (s16)((data[ACCEL_OFFSET+4] << 8) | data[ACCEL_OFFSET+5]);
	pdata->xa = map_gryro_value(x, y, z, x_axis);
	pdata->ya = map_gryro_value(x, y, z, y_axis);
	pdata->za = map_gryro_value(x, y, z, z_axis);
	//printk(KERN_DEBUG"xa:%d, ya:%d, za:%d\n", x, y, z);
	x = (s16)((data[GYRO_OFFSET+0] << 8) | data[GYRO_OFFSET+1]);
	y = (s16)((data[GYRO_OFFSET+2] << 8) | data[GYRO_OFFSET+3]);
	z = (s16)((data[GYRO_OFFSET+4] << 8) | data[GYRO_OFFSET+5]);
	pdata->xg = map_gryro_value(x, y, z, x_axis);;
	pdata->yg = map_gryro_value(x, y, z, y_axis);;
	pdata->zg = map_gryro_value(x, y, z, z_axis);;
	pdata->sample_id = sample_id++;
	//printk(KERN_DEBUG"xg:%d, yg:%d, zg:%d\n", x, y, z);
}

static void  gyro_read(void)
{
	if (unlikely(pinfo->busy)) {
            printk(KERN_DEBUG "already busy\n");
            return;
       }

	pinfo->busy = 1;
	mpu6000_read_data(&(pinfo->data));
	if (pinfo->eis_callback) {
		pinfo->eis_callback(&(pinfo->data), pinfo->eis_arg);
	}
	pinfo->busy = 0;
}

static irqreturn_t mpu6000_isr(int data,void *dev_id)
{
	gyro_read();
	return IRQ_HANDLED;
}

static const struct of_device_id ambarella_gyro_dt_ids[] __initconst = {
	{ .compatible = "ambarella,gyro" },
	{ },
};

static int gyro_request_gpio_irq(int gpio_id)
{
	int	errCode = 0;
	struct ambsvc_gpio gpio_svc;

	gpio_svc.svc_id = AMBSVC_GPIO_REQUEST;
	gpio_svc.gpio = gpio_id;
	errCode = ambarella_request_service(AMBARELLA_SERVICE_GPIO,
					&gpio_svc, NULL);
	if (errCode < 0) {
		printk("can't request gpio[%d]!\n", gpio_id);
		return errCode;
	}

	gpio_svc.svc_id = AMBSVC_GPIO_INPUT;
	errCode = ambarella_request_service(AMBARELLA_SERVICE_GPIO,
				&gpio_svc, NULL);
	if (errCode < 0)
		return errCode;

	gpio_svc.svc_id = AMBSVC_GPIO_TO_IRQ;
	errCode = ambarella_request_service(AMBARELLA_SERVICE_GPIO,
				&gpio_svc, &pinfo->irq_num);
	if (errCode < 0) {
		printk("can't get irq from gpio[%d]!\n", gpio_id);
		return errCode;
	}

	return errCode;
}

static int gyro_free_gpio(int gpio_id)
{
	int	errCode = 0;
	struct ambsvc_gpio gpio_svc;

	gpio_svc.svc_id = AMBSVC_GPIO_FREE;
	gpio_svc.gpio = gpio_id;
	ambarella_request_service(AMBARELLA_SERVICE_GPIO, &gpio_svc, NULL);

	return errCode;
}

static int check_axis_direction(void)
{
	int rval = 0;

	if (x_axis < AXIS_FIRST || x_axis >= AXIS_LAST) {
		printk("X axis Direction [%d] should be within [%d~%d]\n",
			x_axis, AXIS_FIRST, AXIS_LAST - 1);
		rval = -1;
	}

	if (y_axis < AXIS_FIRST || y_axis >= AXIS_LAST) {
		printk("Y axis Direction [%d] should be within [%d~%d]\n",
			y_axis, AXIS_FIRST, AXIS_LAST - 1);
		rval = -1;
	}

	if (z_axis < AXIS_FIRST || z_axis >= AXIS_LAST) {
		printk("Z axis Direction [%d] should be within [%d~%d]\n",
			z_axis, AXIS_FIRST, AXIS_LAST - 1);
		rval = -1;
	}

	return rval;
}

static int __init mpu6000_init(void)
{
	int				errCode = 0;
	struct device_node *np;
	u8				data;

	if (check_axis_direction() < 0) {
		errCode = -EINVAL;
		goto mpu6000_init_exit;
	}

	mpu6000_read_reg(MPU6000_REG_WHO_AM_I, &data, 1);
	if(data != MPU6000_ID){
		printk("%s: SPI Bus Error!\n", __func__);
		errCode = -EIO;
		goto mpu6000_init_exit;
	}

	pinfo = kzalloc(sizeof(*pinfo), GFP_KERNEL);
	if (!pinfo) {
		printk("%s: Out of Memory!\n", __func__);
		errCode = -ENOMEM;
		goto mpu6000_init_exit;
	}

	np = of_find_matching_node(NULL, ambarella_gyro_dt_ids);
	if (np == NULL) {
		printk("OPB: cannot find node\n");
		errCode = -ENODEV;
		goto mpu6000_init_exit;
	}

	pinfo->irq_gpio = of_get_named_gpio(np, "irq-gpio", 0);
	if (!pinfo->irq_gpio) {
		printk("'irq-gpio' not found\n");
		errCode = -EINVAL;
		goto mpu6000_init_exit;
	}

	errCode = gyro_request_gpio_irq(pinfo->irq_gpio);
	if (errCode < 0)
		goto mpu6000_free_gpio;

	spin_lock_init(&pinfo->data_lock);
	pinfo->busy = 0;

	/* Disable I2C Interface */
	mpu6000_write_reg(MPU6000_REG_USER_CTRL, 0x10);

	/* Disable Temperature Sensor */
	mpu6000_write_reg(MPU6000_REG_PWR_MGMT_1, 0x9);

	/* 2KHz Output Data Rate */
	mpu6000_write_reg(MPU6000_REG_CONFIG, 0);
	mpu6000_write_reg(MPU6000_REG_SMPLRT_DIV, 3);

	/* Configure Interrupt */
	mpu6000_write_reg(MPU6000_REG_INTERRUPT_PIN,
		MPU6000_INT_RD_CLEAR | MPU6000_LATCH_INT_EN |
		MPU6000_INT_OPEN_DRAIN | MPU6000_INT_LEVEL_LOW);
	mpu6000_write_reg(MPU6000_REG_INTERRUPT_ENABLE, MPU6000_DATA_READY);

	errCode = request_threaded_irq(pinfo->irq_num,
		NULL, mpu6000_isr, IRQF_TRIGGER_FALLING | IRQF_ONESHOT, "gyro", NULL);

	mpu6000_read_data(&pinfo->data);

	return 0;

mpu6000_free_gpio:
	gyro_free_gpio(pinfo->irq_gpio);
mpu6000_init_exit:
	if (errCode && pinfo) {
		kfree(pinfo);
		pinfo = NULL;
	}
	return errCode;
}

static void __exit mpu6000_exit(void)
{
	if (pinfo) {
		free_irq(pinfo->irq_num, NULL);
		gyro_free_gpio(pinfo->irq_gpio);
		kfree(pinfo);
		pinfo = NULL;
	}
}

module_init(mpu6000_init);
module_exit(mpu6000_exit);
MODULE_DESCRIPTION("Ambarella Gyro Sensor MPU6000 Driver");
MODULE_AUTHOR("Zhenwu Xue, <zwxue@ambarella.com>");
MODULE_LICENSE("Proprietary");

