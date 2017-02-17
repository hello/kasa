/**
 * bld/cmd_adc.c
 *
 * Author: Anthony Ginger <hfjiang@ambarella.com>
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


#include <bldfunc.h>
#include <ambhw/adc.h>
#include <ambhw/uart.h>

/*===========================================================================*/
#if (ADC_SUPPORT_SLOT == 1)
static void diag_adc_set_slot_ctrl(u8 slot_id, u32 slot_value)
{
	switch (slot_id) {
	case 0:
		writel(ADC_SLOT_CTRL_0_REG, slot_value);
		break;
	case 1:
		writel(ADC_SLOT_CTRL_1_REG, slot_value);
		break;
	case 2:
		writel(ADC_SLOT_CTRL_2_REG, slot_value);
		break;
	case 3:
		writel(ADC_SLOT_CTRL_3_REG, slot_value);
		break;
	case 4:
		writel(ADC_SLOT_CTRL_4_REG, slot_value);
		break;
	case 5:
		writel(ADC_SLOT_CTRL_5_REG, slot_value);
		break;
	case 6:
		writel(ADC_SLOT_CTRL_6_REG, slot_value);
		break;
	case 7:
		writel(ADC_SLOT_CTRL_7_REG, slot_value);
		break;
	}
}

static void diag_adc_set_config(void)
{
	int i = 0;
	int slot_num_reg = 0;

	writel(ADC_CONTROL_REG, (readl(ADC_CONTROL_REG) | ADC_CONTROL_ENABLE));
	rct_timer_dly_ms(200);

	writel(ADC_SLOT_NUM_REG, 0);//set slot number=1
	writel(ADC_SLOT_PERIOD_REG, 60);//set slot period 60
	for (i = 0; i <= slot_num_reg; i++) {
		diag_adc_set_slot_ctrl(i, 0xfff);//set slot 0 ctrl 0xfff
	}
}
#endif

static void diag_adc(void)
{
	u32 data[ADC_NUM_CHANNELS] = {0};
	u32 old[ADC_NUM_CHANNELS]  = {0};
	u32 i, equal;
	int c = 0;

	writel(ADC16_CTRL_REG, 0x0);

	putstr("running ADC diagnostics...\r\n");
	putstr("press space key to quit!\r\n");

	writel(SCALER_ADC_REG, ADC_SOFT_RESET | 0x2);
	writel(SCALER_ADC_REG, 0x2);

#if (ADC_SUPPORT_SLOT == 1)
	writel(ADC_CONTROL_REG, (readl(ADC_CONTROL_REG) | ADC_CONTROL_RESET));
	diag_adc_set_config();
#else
	writel(ADC_ENABLE_REG, 0x1);
#endif

#if (ADC_CONTROL_TYPE == 0)
	writel(ADC_CONTROL_REG, 0x0);
#endif

	writel(ADC_DATA0_REG, 0x0);
	writel(ADC_DATA1_REG, 0x0);
	writel(ADC_DATA2_REG, 0x0);
	writel(ADC_DATA3_REG, 0x0);
#if (ADC_NUM_CHANNELS > 4)
	writel(ADC_DATA4_REG, 0x0);
#endif
#if (ADC_NUM_CHANNELS > 5)
	writel(ADC_DATA5_REG, 0x0);
#endif
#if (ADC_NUM_CHANNELS >= 8)
	writel(ADC_DATA6_REG, 0x0);
	writel(ADC_DATA7_REG, 0x0);
#endif
#if (ADC_NUM_CHANNELS >= 10)
	writel(ADC_DATA8_REG, 0x0);
	writel(ADC_DATA9_REG, 0x0);
#endif
#if (ADC_NUM_CHANNELS >= 12)
	writel(ADC_DATA10_REG, 0x0);
	writel(ADC_DATA11_REG, 0x0);
#endif

	writel(ADC_CONTROL_REG, (readl(ADC_CONTROL_REG) & (~ADC_CONTROL_MODE)));
	for (;;) {
		if (uart_poll()) {
			c = uart_read();
			if (c == 0x20 || c == 0x0d || c == 0x1b) {
				break;
			}
		}
		/* ADC control mode, single, start conversion */
		writel(ADC_CONTROL_REG,
			(readl(ADC_CONTROL_REG) | ADC_CONTROL_START));
#if (ADC_CONTROL_TYPE == 1)
		while ((readl(ADC_STATUS_REG) & ADC_CONTROL_STATUS) == 0x0);
#else
		while ((readl(ADC_CONTROL_REG) & ADC_CONTROL_STATUS) == 0x0);
#endif

		rct_timer_dly_ms(200);
		for (i = 0; i < ADC_NUM_CHANNELS; i++) {
			old[i] = data[i];
		}

		/* ADC interface Read from Channel 0, 1, 2, 3 */
#if (ADC_NUM_CHANNELS == 8)
		data[0] = (readl(ADC_DATA0_REG) + 0x8000) & 0xffff;
		data[1] = (readl(ADC_DATA1_REG) + 0x8000) & 0xffff;
		data[2] = (readl(ADC_DATA2_REG) + 0x8000) & 0xffff;
		data[3] = (readl(ADC_DATA3_REG) + 0x8000) & 0xffff;
		data[4] = (readl(ADC_DATA4_REG) + 0x8000) & 0xffff;
		data[5] = (readl(ADC_DATA5_REG) + 0x8000) & 0xffff;
		data[6] = (readl(ADC_DATA6_REG) + 0x8000) & 0xffff;
		data[7] = (readl(ADC_DATA7_REG) + 0x8000) & 0xffff;
#else
		data[0] = readl(ADC_DATA0_REG);
		data[1] = readl(ADC_DATA1_REG);
		data[2] = readl(ADC_DATA2_REG);
		data[3] = readl(ADC_DATA3_REG);
#if (ADC_NUM_CHANNELS > 4)
		data[4] = readl(ADC_DATA4_REG);
#endif
#if (ADC_NUM_CHANNELS > 5)
		data[5] = readl(ADC_DATA5_REG);
#endif
#if (ADC_NUM_CHANNELS >= 10)
		data[6] = readl(ADC_DATA6_REG);
		data[7] = readl(ADC_DATA7_REG);
		data[8] = readl(ADC_DATA8_REG);
		data[9] = readl(ADC_DATA9_REG);
#endif
#if (ADC_NUM_CHANNELS >= 12)
		data[10] = readl(ADC_DATA10_REG);
		data[11] = readl(ADC_DATA11_REG);
#endif
#endif

		equal = 1;
		for (i = 0; i < ADC_NUM_CHANNELS; i++) {
			if (data[i] != old[i]) {
				equal = 0;
				break;
			}
		}

		if (equal) {
			continue;
		}

		putstr("[");
		for (i = 0; i < ADC_NUM_CHANNELS; i++) {
			putdec(data[i]);
			putstr("] [");
		}
		putdec(i);
		putstr("]          \r");
	}

	writel(ADC_CONTROL_REG, 0x0);
#if (ADC_CONTROL_TYPE == 1)
	writel(ADC_CONTROL_REG,
		(readl(ADC_CONTROL_REG) & (~ADC_CONTROL_ENABLE)));
#else
	writel(ADC_ENABLE_REG, 0x0);
#endif
	writel(ADC16_CTRL_REG, 0x2);

	putstr("\r\ndone!\r\n");
}

/*===========================================================================*/
static int cmd_adc(int argc, char *argv[])
{
	diag_adc();

	return 0;
}

/*===========================================================================*/
static char help_adc[] =
	"adc - Diag ADC\r\n"
	;
__CMDLIST(cmd_adc, "adc", help_adc);

