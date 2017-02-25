/*
 * kernel/private/drivers/ambarella/vin/decoders/adv7441a/adv7441a.c
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


#include <linux/module.h>
#include <linux/ambpriv_device.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/firmware.h>
#include <iav_utils.h>
#include <vin_api.h>
#include <plat/clk.h>
#include "adv7441a.h"

static int bus_addr = (0 << 16) | (CONFIG_I2C_AMBARELLA_ADV7441A_USERMAP_ADDR);
module_param(bus_addr, int, 0644);
MODULE_PARM_DESC(bus_addr, " bus and addr: bit16~bit31: bus, bit0~bit15: addr");

static int swap_cbcr = 0;
module_param(swap_cbcr, int, 0644);
MODULE_PARM_DESC(swap_cbcr, "Set true will try swap cbcr");

static int check_hdmi_infoframe = 0;
module_param(check_hdmi_infoframe, int, 0644);
MODULE_PARM_DESC(check_hdmi_infoframe, "Set true will check HDMI infoframe.");

static char *edid_data = NULL;
MODULE_PARM_DESC(edid_data, "EDID Bin file for ADV7441A");
module_param(edid_data, charp, 0444);

static int blackmagic = 0;
module_param(blackmagic, int, 0644);
MODULE_PARM_DESC(blackmagic, "Compatible for blackmagic box");

static int bt601mode = 0;
module_param(bt601mode, int, 0644);
MODULE_PARM_DESC(bt601mode, "Set true to use bt601 mode");
/* ========================================================================== */
struct adv7441a_priv {
	struct i2c_client idc_usrmap;
	struct i2c_client idc_usrmap1;
	struct i2c_client idc_usrmap2;
	struct i2c_client idc_vdpmap;
	struct i2c_client idc_hdmimap;
	struct i2c_client idc_rksvmap;
	struct i2c_client idc_edidmap;
	struct i2c_client idc_revmap;

	enum amba_video_mode cap_vin_mode;
	u16 cap_start_x;
	u16 cap_start_y;
	u16 cap_cap_w;
	u16 cap_cap_h;
	u16 sync_start;
	u8 video_system;
	u32 frame_rate;
	u8 aspect_ratio;
	u8 input_type;
	u8 video_format;
	u8 bit_resolution;
	u8 input_format;

	u16 line_width;
	u16 f0_height;
	u16 f1_height;
	u16 hs_front_porch;
	u16 hs_plus_width;
	u16 hs_back_porch;
	u16 f0_vs_front_porch;
	u16 f0_vs_plus_width;
	u16 f0_vs_back_porch;
	u16 f1_vs_front_porch;
	u16 f1_vs_plus_width;
	u16 f1_vs_back_porch;

	const struct adv7441a_reg_table *fix_reg_table;
	u32 so_freq_hz;
	u32 so_pclk_freq_hz;

	u32 valid_edid;
	u8 edid[ADV7441A_EDID_TABLE_SIZE];
	u16 cap_src_id;
};

#include "adv7441a_table.c"

static const char adv7441a_name[] = "adv7441a";

static int adv7441a_write_reg(struct vin_device *vdev, u8 regmap, u8 subaddr, u8 data)
{
	int errCode = 0;
	struct adv7441a_priv *adv7441a;
	struct i2c_client *client;
	struct i2c_msg msgs[1];
	u8 pbuf[2];

	adv7441a = (struct adv7441a_priv *)vdev->priv;

	switch (regmap) {
	case USER_MAP:
		client = &adv7441a->idc_usrmap;
		break;

	case USER_MAP_1:
		client = &adv7441a->idc_usrmap1;
		break;

	case USER_MAP_2:
		client = &adv7441a->idc_usrmap2;
		break;

	case VDP_MAP:
		client = &adv7441a->idc_vdpmap;
		break;

	case HDMI_MAP:
		client = &adv7441a->idc_hdmimap;
		break;

	case KSV_MAP:
		client = &adv7441a->idc_rksvmap;
		break;

	case EDID_MAP:
		client = &adv7441a->idc_edidmap;
		break;

	case RESERVED_MAP:
		client = &adv7441a->idc_revmap;
		break;

	default:
		errCode = -EIO;
		goto adv7441a_write_reg_exit;
		break;
	}

	//vin_debug("regmap:0x%x, regmap_addr:0x%x, addr:0x%x, data:0x%x\n",
	//	regmap, client->addr, subaddr, data);

	pbuf[0] = subaddr;
	pbuf[1] = data;

	msgs[0].addr = client->addr;
	msgs[0].flags = client->flags;
	msgs[0].len = 2;
	msgs[0].buf = pbuf;

	errCode = i2c_transfer(client->adapter, msgs, 1);
	if (errCode != 1) {
		vin_error("adv7441a_write_reg(error) %d [0x%x:0x%x]\n", errCode, subaddr, data);
		errCode = -EIO;
	} else {
		errCode = 0;
	}

adv7441a_write_reg_exit:
	return errCode;
}

static int adv7441a_read_reg(struct vin_device *vdev, u8 regmap, u8 subaddr, u8 * pdata)
{
	int errCode = 0;
	struct adv7441a_priv *adv7441a;
	struct i2c_client *client;
	struct i2c_msg msgs[2];
	u8 pbuf[2];

	adv7441a = (struct adv7441a_priv *)vdev->priv;

	switch (regmap) {
	case USER_MAP:
		client = &adv7441a->idc_usrmap;
		break;

	case USER_MAP_1:
		client = &adv7441a->idc_usrmap1;
		break;

	case USER_MAP_2:
		client = &adv7441a->idc_usrmap2;
		break;

	case VDP_MAP:
		client = &adv7441a->idc_vdpmap;
		break;

	case HDMI_MAP:
		client = &adv7441a->idc_hdmimap;
		break;

	case KSV_MAP:
		client = &adv7441a->idc_rksvmap;
		break;

	case EDID_MAP:
		client = &adv7441a->idc_edidmap;
		break;

	case RESERVED_MAP:
		client = &adv7441a->idc_revmap;
		break;

	default:
		errCode = -EIO;
		goto adv7441a_read_reg_exit;
		break;
	}

	msgs[0].addr = client->addr;
	msgs[0].flags = client->flags;
	msgs[0].len = 1;
	msgs[0].buf = &subaddr;
	msgs[1].addr = client->addr;
	msgs[1].flags = client->flags | I2C_M_RD;
	msgs[1].len = 2;
	msgs[1].buf = pbuf;

	errCode = i2c_transfer(client->adapter, msgs, 2);
	if (errCode != 2) {
		vin_error("adv7441a_read_reg(error) %d [0x%x]\n", errCode, subaddr);
		errCode = -EIO;
	} else {
		*pdata = pbuf[0];
		errCode = 0;
	}

adv7441a_read_reg_exit:
	return errCode;
}

static int adv7441a_write_reset(struct vin_device *vdev)
{
	int errCode = 0;
	struct adv7441a_priv *adv7441a;
	struct i2c_client *client;
	struct i2c_msg msgs[1];
	u8 pbuf[2];

	adv7441a = (struct adv7441a_priv *)vdev->priv;
	client = &adv7441a->idc_usrmap;

	pbuf[0] = ADV7441A_REG_PWR;
	pbuf[1] = 0x80;

	msgs[0].addr = client->addr;
	msgs[0].flags = client->flags | I2C_M_IGNORE_NAK;
	msgs[0].len = 2;
	msgs[0].buf = pbuf;

	errCode = i2c_transfer(client->adapter, msgs, 1);
	if (errCode != 1) {
		vin_error("adv7441a_write_reset(error) %d [0x%x:0x%x]\n", errCode, pbuf[0], pbuf[1]);
		errCode = -EIO;
	} else {
		errCode = 0;
	}

	return errCode;
}

static int adv7441a_init_interrupts(struct vin_device *vdev)
{
	int errCode = 0;
	u8 regdata;

	/*Initiate int mask1 */
	regdata = 0xff;
	errCode = adv7441a_write_reg(vdev, USER_MAP_1, ADV7441A_REG_INT1MSK1, regdata);
	/*Reset int clear1 */
	regdata = 0xff;
	errCode = adv7441a_write_reg(vdev, USER_MAP_1, ADV7441A_REG_INT1CLR1, regdata);
	regdata = 0x00;
	errCode = adv7441a_write_reg(vdev, USER_MAP_1, ADV7441A_REG_INT1CLR1, regdata);

	/*Initiate int mask2 */
	regdata = 0xff;
	errCode = adv7441a_write_reg(vdev, USER_MAP_1, ADV7441A_REG_INT1MSK2, regdata);
	/*Reset int clear2 */
	regdata = 0xff;
	errCode = adv7441a_write_reg(vdev, USER_MAP_1, ADV7441A_REG_INT1CLR2, regdata);
	regdata = 0x00;
	errCode = adv7441a_write_reg(vdev, USER_MAP_1, ADV7441A_REG_INT1CLR2, regdata);

	/*Initiate int mask3 */
	regdata = 0xff;
	errCode = adv7441a_write_reg(vdev, USER_MAP_1, ADV7441A_REG_INT1MSK3, regdata);
	/*Reset int clear3 */
	regdata = 0xff;
	errCode = adv7441a_write_reg(vdev, USER_MAP_1, ADV7441A_REG_INT1CLR3, regdata);
	regdata = 0x00;
	errCode = adv7441a_write_reg(vdev, USER_MAP_1, ADV7441A_REG_INT1CLR3, regdata);

	/*HDMI interrupt */
	regdata = 0x10;
	errCode = adv7441a_write_reg(vdev, USER_MAP_1, ADV7441A_REG_HDMIINT2MSK2, regdata);
	errCode = adv7441a_write_reg(vdev, USER_MAP_1, ADV7441A_REG_HDMIINT2MSK1, regdata);
	regdata = 0xff;
	errCode = adv7441a_write_reg(vdev, USER_MAP_1, ADV7441A_REG_HDMIINTSTUS2, regdata);

	return errCode;
}

static void adv7441a_sw_reset(struct vin_device *vdev, int force_init)
{
	u32 i;
	struct adv7441a_priv *pinfo;
	u8 reg;

	pinfo = (struct adv7441a_priv *)vdev->priv;

	if (force_init) {
		//Deassert HPD...
		msleep(ADV7441A_RETRY_SLEEP_MS);
	}

	adv7441a_write_reg(vdev, KSV_MAP, ADV7441A_REG_CTRL_BITS, 0);

	adv7441a_write_reset(vdev);
	do {
		msleep(5);
		adv7441a_read_reg(vdev, USER_MAP, ADV7441A_REG_PWR, &reg);
	} while ((reg & 0x80) != 0);

	adv7441a_init_interrupts(vdev);

	adv7441a_write_reg(vdev, KSV_MAP, ADV7441A_REG_BCAPS,
		ADV7441A_BCAPS_DEFAULT | ADV7441A_BCAPS_HDMI_ENABLE);

	if (pinfo->valid_edid) {
		adv7441a_write_reg(vdev, KSV_MAP, ADV7441A_REG_CTRL_BITS, 0);
		for (i = 0; i < ADV7441A_EDID_TABLE_SIZE; i++)
			adv7441a_write_reg(vdev, EDID_MAP, i, pinfo->edid[i]);
		adv7441a_write_reg(vdev, KSV_MAP, ADV7441A_REG_CTRL_BITS,
			ADV7441A_EDID_A_ENABLE | ADV7441A_EDID_B_ENABLE);
		msleep(5);
		for (i = 0; i < ADV7441A_EDID_TABLE_SIZE; i++) {
			adv7441a_read_reg(vdev, EDID_MAP, i, &reg);
			vin_debug("[%02d]: 0x%02x\n", i, reg);
		}
	}

	if (force_init) {
		//Assert HPD...
	}

	msleep(ADV7441A_RETRY_SLEEP_MS);
}

static int adv7441a_set_vin_mode(struct vin_device *vdev, struct vin_video_format *format)
{
	struct vin_device_config adv7441a_config;
	struct adv7441a_priv *pinfo;
	pinfo = (struct adv7441a_priv *)vdev->priv;

	memset(&adv7441a_config, 0, sizeof (adv7441a_config));

	adv7441a_config.interface_type = SENSOR_PARALLEL_LVCMOS;

	if (pinfo->bit_resolution == AMBA_VIDEO_BITS_8)
		adv7441a_config.input_mode = SENSOR_YUV_1PIX;
	else
		adv7441a_config.input_mode = SENSOR_YUV_2PIX;

	if (swap_cbcr)
		adv7441a_config.yuv_pixel_order = SENSOR_CR_Y0_CB_Y1;
	else
		adv7441a_config.yuv_pixel_order = SENSOR_CB_Y0_CR_Y1;

	adv7441a_config.plvcmos_cfg.vs_hs_polarity = SENSOR_VS_HIGH | SENSOR_HS_HIGH;
	adv7441a_config.plvcmos_cfg.data_edge = SENSOR_DATA_RISING_EDGE;

	if (pinfo->input_type == AMBA_VIDEO_TYPE_YUV_656)
		adv7441a_config.plvcmos_cfg.paralle_sync_type = SENSOR_PARALLEL_SYNC_656;
	else
		adv7441a_config.plvcmos_cfg.paralle_sync_type = SENSOR_PARALLEL_SYNC_601;

	adv7441a_config.cap_win.x = pinfo->cap_start_x;
	adv7441a_config.cap_win.y = pinfo->cap_start_y;
	adv7441a_config.cap_win.width = pinfo->cap_cap_w;

	if (pinfo->video_format == AMBA_VIDEO_FORMAT_INTERLACE) {
		adv7441a_config.cap_win.height = pinfo->cap_cap_h>> 1;
		adv7441a_config.input_format = AMBA_VIN_INPUT_FORMAT_YUV_422_INTLC;
	} else {
		adv7441a_config.cap_win.height = pinfo->cap_cap_h;
		adv7441a_config.input_format = AMBA_VIN_INPUT_FORMAT_YUV_422_PROG;
	}

	adv7441a_config.plvcmos_cfg.sync_code_style = SENSOR_SYNC_STYLE_ITU656;

	adv7441a_config.sensor_id = GENERIC_SENSOR;
	adv7441a_config.video_format = pinfo->video_format ;
	/* yuv input, always set VIN to 8btis */
	adv7441a_config.bit_resolution = AMBA_VIDEO_BITS_8;

	return ambarella_set_vin_config(vdev, &adv7441a_config);
}

static int adv7441a_query_decoder_idrev(struct vin_device *vdev, u8 * id)
{
	int errCode = 0;
	u8 idrev = 0;

	errCode = adv7441a_read_reg(vdev, USER_MAP, ADV7441A_REG_IDENT, &idrev);
	switch (idrev) {
	case 0x01:
		vin_info("ADV7441A-ES1 is detected\n");
		break;
	case 0x02:
		vin_info("ADV7441A-ES2 is detected\n");
		break;
	case 0x04:
		vin_info("ADV7441A-ES3 is detected\n");
		break;
	default:
		vin_info("Can't detect ADV7441A, idrev is 0x%x\n", idrev);
		break;
	}
	*id = idrev;

	return errCode;
}

static void adv7441a_fill_share_regs(struct vin_device *vdev, const struct adv7441a_reg_table regtable[])
{
	u32 i;

	for (i = 0;; i++) {
		if ((regtable[i].data == 0xff) && (regtable[i].reg == 0xff))
			break;
		adv7441a_write_reg(vdev, regtable[i].regmap, regtable[i].reg, regtable[i].data);
	}
}

static int adv7441a_select_source(struct vin_device *vdev)
{
	int errCode = 0;
	u8 regdata;
	u32 source_id;
	struct adv7441a_priv *pinfo;

	pinfo = (struct adv7441a_priv *)vdev->priv;
	source_id = adv7441a_source_table[pinfo->cap_src_id].source_id;

	adv7441a_sw_reset(vdev, 0);

	if (source_id < ADV7441A_HDMI_MIN_ID) {
		errCode = adv7441a_read_reg(vdev, USER_MAP, ADV7441A_REG_INPUTCTRL, &regdata);
		if (errCode)
			goto adv7441a_select_channel_exit;

		regdata &= ~ADV7441A_INPUTCTRL_INSEL_MASK;
		regdata |= source_id;
		adv7441a_write_reg(vdev, USER_MAP, ADV7441A_REG_INPUTCTRL, regdata);
	} else {
		errCode = adv7441a_read_reg(vdev, HDMI_MAP, ADV7441A_REG_HDMIPORTSEL, &regdata);
		if (errCode)
			goto adv7441a_select_channel_exit;

		regdata &= ~ADV7441A_HDMIPORTSEL_PORT_MASK;
		regdata |= (source_id - ADV7441A_HDMI_MIN_ID);
		adv7441a_write_reg(vdev, HDMI_MAP, ADV7441A_REG_HDMIPORTSEL, regdata);
	}

	if (bt601mode)
		adv7441a_fill_share_regs(vdev, adv7441a_source_table[pinfo->cap_src_id].pregtable_601);
	else
		adv7441a_fill_share_regs(vdev, adv7441a_source_table[pinfo->cap_src_id].pregtable_656);

adv7441a_select_channel_exit:
	return errCode;
}

static int adv7441a_init_device(struct vin_device *vdev)
{
	u8 id;

	adv7441a_sw_reset(vdev, 1);
	adv7441a_query_decoder_idrev(vdev, &id);
	adv7441a_select_source(vdev);
	vin_info("%s:%d probed %d!\n", adv7441a_name, id, ADV7441A_SOURCE_NUM);

	return 0;
}

static void adv7441a_print_info(struct vin_device *vdev)
{
	struct adv7441a_priv *pinfo;

	pinfo = (struct adv7441a_priv *)vdev->priv;

	vin_debug("cap_src_id = %d\n", pinfo->cap_src_id);
	vin_debug("cap_vin_mode = %d\n", pinfo->cap_vin_mode);
	vin_debug("cap_start_x = %d\n", pinfo->cap_start_x);
	vin_debug("cap_start_y = %d\n", pinfo->cap_start_y);
	vin_debug("cap_cap_w = %d\n", pinfo->cap_cap_w);
	vin_debug("cap_cap_h = %d\n", pinfo->cap_cap_h);
	vin_debug("sync_start = %d\n", pinfo->sync_start);
	vin_debug("video_system = %d\n", pinfo->video_system);
	vin_debug("frame_rate = %d\n", pinfo->frame_rate);
	vin_debug("aspect_ratio = %d\n", pinfo->aspect_ratio);
	vin_debug("input_type = %d\n", pinfo->input_type);
	vin_debug("video_format = %d\n", pinfo->video_format);
	vin_debug("bit_resolution = %d\n", pinfo->bit_resolution);
	vin_debug("input_format = %d\n", pinfo->input_format);

	vin_debug("line_width = %d\n", pinfo->line_width);
	vin_debug("f0_height = %d\n", pinfo->f0_height);
	vin_debug("f1_height = %d\n", pinfo->f1_height);
	vin_debug("hs_front_porch = %d\n", pinfo->hs_front_porch);
	vin_debug("hs_plus_width = %d\n", pinfo->hs_plus_width);
	vin_debug("hs_back_porch = %d\n", pinfo->hs_back_porch);
	vin_debug("f0_vs_front_porch = %d\n", pinfo->f0_vs_front_porch);
	vin_debug("f0_vs_plus_width = %d\n", pinfo->f0_vs_plus_width);
	vin_debug("f0_vs_back_porch = %d\n", pinfo->f0_vs_back_porch);
	vin_debug("f1_vs_front_porch = %d\n", pinfo->f1_vs_front_porch);
	vin_debug("f1_vs_plus_width = %d\n", pinfo->f1_vs_plus_width);
	vin_debug("f1_vs_back_porch = %d\n", pinfo->f1_vs_back_porch);

	vin_debug("fix_reg_table = 0x%x\n", (u32) (pinfo->fix_reg_table));
	vin_debug("so_freq_hz = %d\n", pinfo->so_freq_hz);
	vin_debug("so_pclk_freq_hz = %d\n", pinfo->so_pclk_freq_hz);
}

static int adv7441a_check_status_hdmi_infoframe(struct vin_device *vdev)
{
	int errCode = 0;
	u8 regdata;
	u8 infoframever;
	struct adv7441a_priv *pinfo = (struct adv7441a_priv *)vdev->priv;
	int count = ADV7441A_RETRY_COUNT;

	errCode = adv7441a_read_reg(vdev, USER_MAP_1, ADV7441A_REG_HDMIRAWSTUS1, &regdata);
	if (errCode)
		goto adv7441a_check_status_hdmi_infoframe_exit;
	if ((regdata & ADV7441A_HDMIRAWSTUTS1_AVIINFO_MASK) == ADV7441A_HDMIRAWSTUTS1_AVIINFO_DET) {
		errCode = adv7441a_read_reg(vdev, USER_MAP_1, ADV7441A_REG_HDMIINTSTUS4, &regdata);
		if (errCode)
			goto adv7441a_check_status_hdmi_infoframe_exit;
	} else {
		vin_warn("ADV7441A_HDMIRAWSTUTS1_AVIINFO_NODET\n");
	}

	while (count) {
		errCode = adv7441a_read_reg(vdev, HDMI_MAP, ADV7441A_REG_AVIINFOVER, &infoframever);
		if (errCode)
			goto adv7441a_check_status_hdmi_infoframe_exit;

		if (infoframever >= 2)
			break;

		count--;
		msleep(ADV7441A_RETRY_SLEEP_MS);
	}
	if (!count) {
		vin_error("HDMI infoframever = %d.\n", infoframever);
		errCode = -EINVAL;
		goto adv7441a_check_status_hdmi_infoframe_exit;
	}

	errCode = adv7441a_read_reg(vdev, HDMI_MAP, ADV7441A_REG_AVIINFODATA4, &regdata);
	if (errCode)
		goto adv7441a_check_status_hdmi_infoframe_exit;
	vin_info("video identification code is 0x%x\n", regdata & ADV7441A_AVIINFODATA4_VIC_MASK);

	pinfo->cap_start_x = pinfo->hs_back_porch;
	pinfo->cap_start_y = pinfo->f0_vs_front_porch + pinfo->f1_vs_front_porch;
	pinfo->so_freq_hz = PLL_CLK_27MHZ;
	pinfo->so_pclk_freq_hz = PLL_CLK_27MHZ;

	switch (regdata & ADV7441A_AVIINFODATA4_VIC_MASK) {
	/*-----------------60 Hz Systems ----------------*/
	case 1:		/*640x480P60 */
		pinfo->cap_vin_mode = AMBA_VIDEO_MODE_VGA;
		pinfo->cap_cap_w = AMBA_VIDEO_VGA_W;
		pinfo->cap_cap_h = AMBA_VIDEO_VGA_H;
		pinfo->video_system = AMBA_VIDEO_SYSTEM_PAL;
		pinfo->aspect_ratio = AMBA_VIDEO_RATIO_4_3;
		pinfo->frame_rate = AMBA_VIDEO_FPS_60;
		pinfo->video_format = AMBA_VIDEO_FORMAT_PROGRESSIVE;
		pinfo->sync_start = ADV7441A_TBD_CAP_SYNC_START;
		break;

	case 2:		/*720x480P60 */
		pinfo->cap_vin_mode = AMBA_VIDEO_MODE_D1_NTSC;
		pinfo->cap_cap_w = AMBA_VIDEO_NTSC_W;
		pinfo->cap_cap_h = AMBA_VIDEO_NTSC_H;
		pinfo->video_system = AMBA_VIDEO_SYSTEM_NTSC;
		pinfo->aspect_ratio = AMBA_VIDEO_RATIO_4_3;
		pinfo->frame_rate = AMBA_VIDEO_FPS_59_94;//AMBA_VIDEO_FPS_60;
		pinfo->video_format = AMBA_VIDEO_FORMAT_PROGRESSIVE;
		pinfo->sync_start = ADV7441A_480P_601_CAP_SYNC_START;
		break;

	case 3:		/*720x480P60 */
		pinfo->cap_vin_mode = AMBA_VIDEO_MODE_D1_NTSC;
		pinfo->cap_cap_w = AMBA_VIDEO_NTSC_W;
		pinfo->cap_cap_h = AMBA_VIDEO_NTSC_H;
		pinfo->video_system = AMBA_VIDEO_SYSTEM_NTSC;
		pinfo->aspect_ratio = AMBA_VIDEO_RATIO_16_9;
		pinfo->frame_rate = AMBA_VIDEO_FPS_60;
		pinfo->video_format = AMBA_VIDEO_FORMAT_PROGRESSIVE;
		pinfo->sync_start = ADV7441A_480P_601_CAP_SYNC_START;
		break;

	case 4:		/*1280x720P60 */
		pinfo->cap_vin_mode = AMBA_VIDEO_MODE_720P;
		pinfo->cap_cap_w = AMBA_VIDEO_720P_W;
		pinfo->cap_cap_h = AMBA_VIDEO_720P_H;
		pinfo->video_system = AMBA_VIDEO_SYSTEM_NTSC;
		pinfo->aspect_ratio = AMBA_VIDEO_RATIO_16_9;
		pinfo->frame_rate = AMBA_VIDEO_FPS_59_94;//AMBA_VIDEO_FPS_60;
		pinfo->video_format = AMBA_VIDEO_FORMAT_PROGRESSIVE;
		pinfo->so_freq_hz = PLL_CLK_74_25MHZ;
		pinfo->so_pclk_freq_hz = PLL_CLK_74_25MHZ;
		pinfo->sync_start = ADV7441A_720P60_601_CAP_SYNC_START;
		break;

	case 5:		/*1920x1080i60 */
		pinfo->cap_vin_mode = AMBA_VIDEO_MODE_1080I;
		pinfo->cap_cap_w = AMBA_VIDEO_1080P_W;
		pinfo->cap_cap_h = AMBA_VIDEO_1080P_H;
		pinfo->video_system = AMBA_VIDEO_SYSTEM_NTSC;
		pinfo->aspect_ratio = AMBA_VIDEO_RATIO_16_9;
		pinfo->video_format = AMBA_VIDEO_FORMAT_INTERLACE;
		pinfo->frame_rate = AMBA_VIDEO_FPS_59_94;
		pinfo->so_freq_hz = PLL_CLK_74_25D1001MHZ;
		pinfo->so_pclk_freq_hz = PLL_CLK_74_25D1001MHZ;
		pinfo->sync_start = ADV7441A_1080I60_601_CAP_SYNC_START;
		break;

	case 6:		/*720x480i60 */
		pinfo->cap_vin_mode = AMBA_VIDEO_MODE_480I;
		pinfo->cap_cap_w = AMBA_VIDEO_NTSC_W;
		pinfo->cap_cap_h = AMBA_VIDEO_NTSC_H;
		pinfo->video_system = AMBA_VIDEO_SYSTEM_NTSC;
		pinfo->aspect_ratio = AMBA_VIDEO_RATIO_4_3;
		pinfo->frame_rate = AMBA_VIDEO_FPS_59_94;
		pinfo->video_format = AMBA_VIDEO_FORMAT_INTERLACE;
		pinfo->so_freq_hz = PLL_CLK_13_5MHZ;
		pinfo->so_pclk_freq_hz = PLL_CLK_13_5MHZ;
		pinfo->sync_start = ADV7441A_480I_601_CAP_SYNC_START;
		if (pinfo->line_width == pinfo->cap_cap_w) {
			pinfo->cap_start_x = ADV7441A_480I_601_CAP_START_X;
			pinfo->cap_start_y = ADV7441A_480I_601_CAP_START_Y;
		}
		break;

	case 7:		/*720x480i60 */
		pinfo->cap_vin_mode = AMBA_VIDEO_MODE_480I;
		pinfo->cap_cap_w = AMBA_VIDEO_NTSC_W;
		pinfo->cap_cap_h = AMBA_VIDEO_NTSC_H;
		pinfo->video_system = AMBA_VIDEO_SYSTEM_NTSC;
		pinfo->aspect_ratio = AMBA_VIDEO_RATIO_16_9;
		pinfo->frame_rate = AMBA_VIDEO_FPS_59_94;
		pinfo->video_format = AMBA_VIDEO_FORMAT_INTERLACE;
		pinfo->so_freq_hz = PLL_CLK_13_5MHZ;
		pinfo->so_pclk_freq_hz = PLL_CLK_13_5MHZ;
		pinfo->sync_start = ADV7441A_480I_601_CAP_SYNC_START;
		if (pinfo->line_width == pinfo->cap_cap_w) {
			pinfo->cap_start_x = ADV7441A_480I_601_CAP_START_X;
			pinfo->cap_start_y = ADV7441A_480I_601_CAP_START_Y;
		}
		break;

	case 16:		/*1920x1080P60 */
		pinfo->cap_vin_mode = AMBA_VIDEO_MODE_1080P;
		pinfo->cap_cap_w = AMBA_VIDEO_1080P_W;
		pinfo->cap_cap_h = AMBA_VIDEO_1080P_H;
		pinfo->video_system = AMBA_VIDEO_SYSTEM_NTSC;
		pinfo->aspect_ratio = AMBA_VIDEO_RATIO_16_9;
		pinfo->frame_rate = AMBA_VIDEO_FPS_60;
		pinfo->video_format = AMBA_VIDEO_FORMAT_PROGRESSIVE;
		pinfo->so_freq_hz = PLL_CLK_148_5MHZ;
		pinfo->so_pclk_freq_hz = PLL_CLK_148_5MHZ;
		pinfo->sync_start = ADV7441A_1080P60_601_CAP_SYNC_START;
		break;

	/*-----------------50 Hz Systems ----------------*/
	case 17:		/*720x576p@50 */
		pinfo->cap_vin_mode = AMBA_VIDEO_MODE_D1_PAL;
		pinfo->cap_cap_w = AMBA_VIDEO_PAL_W;
		pinfo->cap_cap_h = AMBA_VIDEO_PAL_H;
		pinfo->video_system = AMBA_VIDEO_SYSTEM_PAL;
		pinfo->aspect_ratio = AMBA_VIDEO_RATIO_4_3;
		pinfo->frame_rate = AMBA_VIDEO_FPS_50;
		pinfo->video_format = AMBA_VIDEO_FORMAT_PROGRESSIVE;
		pinfo->sync_start = ADV7441A_576P_601_CAP_SYNC_START;
		break;

	case 18:		/*720x576p@50 */
		pinfo->cap_vin_mode = AMBA_VIDEO_MODE_D1_PAL;
		pinfo->cap_cap_w = AMBA_VIDEO_PAL_W;
		pinfo->cap_cap_h = AMBA_VIDEO_PAL_H;
		pinfo->video_system = AMBA_VIDEO_SYSTEM_PAL;
		pinfo->aspect_ratio = AMBA_VIDEO_RATIO_16_9;
		pinfo->frame_rate = AMBA_VIDEO_FPS_50;
		pinfo->video_format = AMBA_VIDEO_FORMAT_PROGRESSIVE;
		pinfo->sync_start = ADV7441A_576P_601_CAP_SYNC_START;
		break;

	case 19:		/*1280x720P50 */
		pinfo->cap_vin_mode = AMBA_VIDEO_MODE_720P;
		pinfo->cap_cap_w = AMBA_VIDEO_720P_W;
		pinfo->cap_cap_h = AMBA_VIDEO_720P_H;
		pinfo->video_system = AMBA_VIDEO_SYSTEM_PAL;
		pinfo->aspect_ratio = AMBA_VIDEO_RATIO_16_9;
		pinfo->frame_rate = AMBA_VIDEO_FPS_50;
		pinfo->video_format = AMBA_VIDEO_FORMAT_PROGRESSIVE;
		pinfo->fix_reg_table = adv7441a_hdmi_720p50_fix_regs;
		pinfo->so_freq_hz = PLL_CLK_74_25MHZ;
		pinfo->so_pclk_freq_hz = PLL_CLK_74_25MHZ;
		pinfo->sync_start = ADV7441A_720P50_601_CAP_SYNC_START;
		break;

	case 20:		/*1920x1080i50 */
		pinfo->cap_vin_mode = AMBA_VIDEO_MODE_1080I;
		pinfo->cap_cap_w = AMBA_VIDEO_1080P_W;
		pinfo->cap_cap_h = AMBA_VIDEO_1080P_H;
		pinfo->video_system = AMBA_VIDEO_SYSTEM_PAL;
		pinfo->aspect_ratio = AMBA_VIDEO_RATIO_16_9;
		pinfo->frame_rate = AMBA_VIDEO_FPS_50;
		pinfo->video_format = AMBA_VIDEO_FORMAT_INTERLACE;
		pinfo->fix_reg_table = adv7441a_hdmi_1080i50_fix_regs;
		pinfo->so_freq_hz = PLL_CLK_74_25MHZ;
		pinfo->so_pclk_freq_hz = PLL_CLK_74_25MHZ;
		pinfo->sync_start = ADV7441A_1080I50_601_CAP_SYNC_START;
		break;

	case 21:		/*720x576i@50 */
		pinfo->cap_vin_mode = AMBA_VIDEO_MODE_576I;
		pinfo->cap_cap_w = AMBA_VIDEO_PAL_W;
		pinfo->cap_cap_h = AMBA_VIDEO_PAL_H;
		pinfo->video_system = AMBA_VIDEO_SYSTEM_PAL;
		pinfo->aspect_ratio = AMBA_VIDEO_RATIO_4_3;
		pinfo->frame_rate = AMBA_VIDEO_FPS_50;
		pinfo->video_format = AMBA_VIDEO_FORMAT_INTERLACE;
		pinfo->so_freq_hz = PLL_CLK_13_5MHZ;
		pinfo->so_pclk_freq_hz = PLL_CLK_13_5MHZ;
		pinfo->sync_start = ADV7441A_576I_601_CAP_SYNC_START;
		if (pinfo->line_width == pinfo->cap_cap_w) {
			pinfo->cap_start_x = ADV7441A_480I_601_CAP_START_X;
			pinfo->cap_start_y = ADV7441A_480I_601_CAP_START_Y;
		}
		break;

	case 22:		/*720x576i@50 */
		pinfo->cap_vin_mode = AMBA_VIDEO_MODE_576I;
		pinfo->cap_cap_w = AMBA_VIDEO_PAL_W;
		pinfo->cap_cap_h = AMBA_VIDEO_PAL_H;
		pinfo->video_system = AMBA_VIDEO_SYSTEM_PAL;
		pinfo->aspect_ratio = AMBA_VIDEO_RATIO_16_9;
		pinfo->frame_rate = AMBA_VIDEO_FPS_50;
		pinfo->video_format = AMBA_VIDEO_FORMAT_INTERLACE;
		pinfo->so_freq_hz = PLL_CLK_13_5MHZ;
		pinfo->so_pclk_freq_hz = PLL_CLK_13_5MHZ;
		pinfo->sync_start = ADV7441A_576I_601_CAP_SYNC_START;
		if (pinfo->line_width == pinfo->cap_cap_w) {
			pinfo->cap_start_x = ADV7441A_480I_601_CAP_START_X;
			pinfo->cap_start_y = ADV7441A_480I_601_CAP_START_Y;
		}
		break;

	case 31:		/*1920x1080P50 */
		pinfo->cap_vin_mode = AMBA_VIDEO_MODE_1080P;
		pinfo->cap_cap_w = AMBA_VIDEO_1080P_W;
		pinfo->cap_cap_h = AMBA_VIDEO_1080P_H;
		pinfo->video_system = AMBA_VIDEO_SYSTEM_PAL;
		pinfo->aspect_ratio = AMBA_VIDEO_RATIO_16_9;
		pinfo->frame_rate = AMBA_VIDEO_FPS_50;
		pinfo->video_format = AMBA_VIDEO_FORMAT_PROGRESSIVE;
		pinfo->fix_reg_table = adv7441a_hdmi_1080p50_fix_regs;
		pinfo->so_freq_hz = PLL_CLK_148_5MHZ;
		pinfo->so_pclk_freq_hz = PLL_CLK_148_5MHZ;
		pinfo->sync_start = ADV7441A_1080P50_601_CAP_SYNC_START;
		break;

	case 32:		/*1920x1080P24 */
		pinfo->cap_vin_mode = AMBA_VIDEO_MODE_1080P;
		pinfo->cap_cap_w = AMBA_VIDEO_1080P_W;
		pinfo->cap_cap_h = AMBA_VIDEO_1080P_H;
		pinfo->video_system = AMBA_VIDEO_SYSTEM_PAL;
		pinfo->aspect_ratio = AMBA_VIDEO_RATIO_16_9;
		pinfo->frame_rate = AMBA_VIDEO_FPS(24);
		pinfo->video_format = AMBA_VIDEO_FORMAT_PROGRESSIVE;
		pinfo->fix_reg_table = adv7441a_hdmi_1080p24_fix_regs;
		pinfo->so_freq_hz = PLL_CLK_74_25MHZ;
		pinfo->so_pclk_freq_hz = PLL_CLK_74_25MHZ;
		pinfo->sync_start = ADV7441A_TBD_CAP_SYNC_START;
		break;

	case 33:		/*1920x1080P25 */
		pinfo->cap_vin_mode = AMBA_VIDEO_MODE_1080P;
		pinfo->cap_cap_w = AMBA_VIDEO_1080P_W;
		pinfo->cap_cap_h = AMBA_VIDEO_1080P_H;
		pinfo->video_system = AMBA_VIDEO_SYSTEM_PAL;
		pinfo->aspect_ratio = AMBA_VIDEO_RATIO_16_9;
		pinfo->frame_rate = AMBA_VIDEO_FPS_25;
		pinfo->video_format = AMBA_VIDEO_FORMAT_PROGRESSIVE;
		pinfo->fix_reg_table = adv7441a_hdmi_1080p25_fix_regs;
		pinfo->so_freq_hz = PLL_CLK_74_25MHZ;
		pinfo->so_pclk_freq_hz = PLL_CLK_74_25MHZ;
		pinfo->sync_start = ADV7441A_TBD_CAP_SYNC_START;
		break;

	case 34:		/*1920x1080P30 */
		pinfo->cap_vin_mode = AMBA_VIDEO_MODE_1080P;
		pinfo->cap_cap_w = AMBA_VIDEO_1080P_W;
		pinfo->cap_cap_h = AMBA_VIDEO_1080P_H;
		pinfo->video_system = AMBA_VIDEO_SYSTEM_PAL;
		pinfo->aspect_ratio = AMBA_VIDEO_RATIO_16_9;
		pinfo->frame_rate = AMBA_VIDEO_FPS_30;
		pinfo->video_format = AMBA_VIDEO_FORMAT_PROGRESSIVE;
		if (!blackmagic) {
			/* remove fix regs to be compatible with blackmagic box */
			pinfo->fix_reg_table = adv7441a_hdmi_1080p30_fix_regs;
		}
		pinfo->so_freq_hz = PLL_CLK_74_25MHZ;
		pinfo->so_pclk_freq_hz = PLL_CLK_74_25MHZ;
		pinfo->sync_start = ADV7441A_TBD_CAP_SYNC_START;
		break;

	default:
		vin_warn("Unknown video identification code is 0x%x\n",
			regdata & ADV7441A_AVIINFODATA4_VIC_MASK);
		errCode = -1;
		goto adv7441a_check_status_hdmi_infoframe_exit;
	}

	if (pinfo->video_format == AMBA_VIDEO_FORMAT_PROGRESSIVE) {
		if (pinfo->line_width != pinfo->cap_cap_w) {
			vin_warn("Progressive Mismatch: line_width[%d] and cap_cap_w[%d].\n",
				pinfo->line_width, pinfo->cap_cap_w);
			errCode = -1;
			goto adv7441a_check_status_hdmi_infoframe_exit;
		}

		if ((pinfo->f0_height != pinfo->cap_cap_h) ||
			(pinfo->f1_height != pinfo->cap_cap_h)){
			vin_warn("Progressive Mismatch: f0_height[%d], f1_height[%d] and cap_cap_h[%d].\n",
				pinfo->f0_height, pinfo->f0_height, pinfo->cap_cap_h);
			errCode = -1;
			goto adv7441a_check_status_hdmi_infoframe_exit;
		}
	} else {
		if ((pinfo->line_width != pinfo->cap_cap_w) &&
			(pinfo->line_width != (pinfo->cap_cap_w * 2))){
			vin_warn("Interlace Mismatch: line_width[%d] and cap_cap_w[%d].\n",
				pinfo->line_width, pinfo->cap_cap_w);
			errCode = -1;
			goto adv7441a_check_status_hdmi_infoframe_exit;
		}

		if ((pinfo->f0_height + pinfo->f1_height) != pinfo->cap_cap_h) {
			vin_warn("Interlace Mismatch: f0_height[%d], f1_height[%d] and cap_cap_h[%d].\n",
				pinfo->f0_height, pinfo->f0_height, pinfo->cap_cap_h);
			errCode = -1;
			goto adv7441a_check_status_hdmi_infoframe_exit;
		}
	}

adv7441a_check_status_hdmi_infoframe_exit:
	return errCode;
}

static int adv7441a_check_status_hdmi_resolution(struct vin_device *vdev)
{
	int errCode = 0;
	u8 regdata1;
	u8 regdata2;
	int count = ADV7441A_RETRY_COUNT;
	struct adv7441a_priv *pinfo;

	pinfo = (struct adv7441a_priv *)vdev->priv;

	/*Increase fix value for different source if necessary... */
	count *= ADV7441A_RETRY_FIX;
	/*PLL locked */
	while (count) {
		errCode = adv7441a_read_reg(vdev, HDMI_MAP, ADV7441A_REG_REG04, &regdata1);
		if (errCode)
			goto adv7441a_check_status_hdmi_resolution_exit;
		if ((regdata1 & ADV7441A_REG04_LOCK_MASK) == ADV7441A_REG04_LOCK_LOCKED)
			break;
		else
			count--;
		msleep(ADV7441A_RETRY_SLEEP_MS);
	}
	if (!count) {
		vin_error("Count time out, HDMI PLL not locked 0x%x!\n", regdata1);
		errCode = -EINVAL;
		goto adv7441a_check_status_hdmi_resolution_exit;
	}
	/*DE regeneration locked */
	while (count) {
		errCode = adv7441a_read_reg(vdev, HDMI_MAP, ADV7441A_REG_REG07, &regdata1);
		if (errCode)
			goto adv7441a_check_status_hdmi_resolution_exit;
		if ((regdata1 & ADV7441A_REG07_LOCK_MASK) == ADV7441A_REG07_LOCK_LOCKED)
			break;
		else
			count--;
		msleep(ADV7441A_RETRY_SLEEP_MS);
	}
	if (!count) {
		vin_error("Count time out, HDMI DE not locked 0x%x!\n", regdata1);
		errCode = -EINVAL;
		goto adv7441a_check_status_hdmi_resolution_exit;
	}

	/*Line width */
	adv7441a_read_reg(vdev, HDMI_MAP, ADV7441A_REG_LW, &regdata2);
	pinfo->line_width = (regdata1 & ADV7441A_REG07_LW11TO8_MASK) << 8;
	pinfo->line_width |= regdata2;

	/*field 0 */
	adv7441a_read_reg(vdev, HDMI_MAP, ADV7441A_REG_F0HMSB, &regdata1);
	adv7441a_read_reg(vdev, HDMI_MAP, ADV7441A_REG_F0HLSB, &regdata2);
	pinfo->f0_height = (regdata1 & ADV7441A_F0HMSB_MSB_MASK) << 8;
	pinfo->f0_height |= regdata2;

	adv7441a_read_reg(vdev, HDMI_MAP, ADV7441A_REG_F0VSFPMSB, &regdata1);
	adv7441a_read_reg(vdev, HDMI_MAP, ADV7441A_REG_F0VSFPLSB, &regdata2);
	pinfo->f0_vs_front_porch = (regdata1 & ADV7441A_F0VSFPMSB_MSB_MASK) << 8;
	pinfo->f0_vs_front_porch |= regdata2;

	adv7441a_read_reg(vdev, HDMI_MAP, ADV7441A_REG_F0VSPWMSB, &regdata1);
	adv7441a_read_reg(vdev, HDMI_MAP, ADV7441A_REG_F0VSPWMSB, &regdata2);
	pinfo->f0_vs_plus_width = (regdata1 & ADV7441A_F0VSFPMSB_MSB_MASK) << 8;
	pinfo->f0_vs_plus_width |= regdata2;

	adv7441a_read_reg(vdev, HDMI_MAP, ADV7441A_REG_F0VSBPMSB, &regdata1);
	adv7441a_read_reg(vdev, HDMI_MAP, ADV7441A_REG_F0VSBPLSB, &regdata2);
	pinfo->f0_vs_back_porch = (regdata1 & ADV7441A_F0VSBPMSB_MSB_MASK) << 8;
	pinfo->f0_vs_back_porch |= regdata2;

	/*field 1 */
	adv7441a_read_reg(vdev, HDMI_MAP, ADV7441A_REG_F1HMSB, &regdata1);
	adv7441a_read_reg(vdev, HDMI_MAP, ADV7441A_REG_F1HLSB, &regdata2);
	pinfo->f1_height = (regdata1 & ADV7441A_F1HMSB_MSB_MASK) << 8;
	pinfo->f1_height |= regdata2;

	adv7441a_read_reg(vdev, HDMI_MAP, ADV7441A_REG_F1VSFPMSB, &regdata1);
	adv7441a_read_reg(vdev, HDMI_MAP, ADV7441A_REG_F1VSFPLSB, &regdata2);
	pinfo->f1_vs_front_porch = (regdata1 & ADV7441A_F1VSFPMSB_MSB_MASK) << 8;
	pinfo->f1_vs_front_porch |= regdata2;

	adv7441a_read_reg(vdev, HDMI_MAP, ADV7441A_REG_F1VSPWMSB, &regdata1);
	adv7441a_read_reg(vdev, HDMI_MAP, ADV7441A_REG_F1VSPWLSB, &regdata2);
	pinfo->f1_vs_plus_width = (regdata1 & ADV7441A_F1VSFPMSB_MSB_MASK) << 8;
	pinfo->f1_vs_plus_width |= regdata2;

	adv7441a_read_reg(vdev, HDMI_MAP, ADV7441A_REG_F1VSBPMSB, &regdata1);
	adv7441a_read_reg(vdev, HDMI_MAP, ADV7441A_REG_F1VSBPLSB, &regdata2);
	pinfo->f1_vs_back_porch = (regdata1 & ADV7441A_F1VSBPMSB_MSB_MASK) << 8;
	pinfo->f1_vs_back_porch |= regdata2;

	/*HS*/
	adv7441a_read_reg(vdev, HDMI_MAP, ADV7441A_REG_HSFPMSB, &regdata1);
	adv7441a_read_reg(vdev, HDMI_MAP, ADV7441A_REG_HSFPLSB, &regdata2);
	pinfo->hs_front_porch = (regdata1 & ADV7441A_HSFPMSB_MSB_MASK) << 8;
	pinfo->hs_front_porch |= regdata2;

	adv7441a_read_reg(vdev, HDMI_MAP, ADV7441A_REG_HSPWMSB, &regdata1);
	adv7441a_read_reg(vdev, HDMI_MAP, ADV7441A_REG_HSPWLSB, &regdata2);
	pinfo->hs_plus_width = (regdata1 & ADV7441A_HSPWMSB_MSB_MASK) << 8;
	pinfo->hs_plus_width |= regdata2;

	adv7441a_read_reg(vdev, HDMI_MAP, ADV7441A_REG_HSBPMSB, &regdata1);
	adv7441a_read_reg(vdev, HDMI_MAP, ADV7441A_REG_HSBPLSB, &regdata2);
	pinfo->hs_back_porch = (regdata1 & ADV7441A_HSBPMSB_MSB_MASK) << 8;
	pinfo->hs_back_porch |= regdata2;

adv7441a_check_status_hdmi_resolution_exit:
	return errCode;
}

static int adv7441a_check_status_hdmi(struct vin_device *vdev)
{
	int errCode = 0;
	u8 regdata;
	u8 mask;
	int count = ADV7441A_RETRY_COUNT;
	struct adv7441a_priv *pinfo = (struct adv7441a_priv *)vdev->priv;
	u32 source_id = adv7441a_source_table[pinfo->cap_src_id].source_id;

	if (source_id == ADV7441A_HDMI_MIN_ID)
		mask = ADV7441A_HDMIRAWSTUTS3_TMDSCLKA_MASK;
	else
		mask = ADV7441A_HDMIRAWSTUTS3_TMDSCLKB_MASK;
	mask |= ADV7441A_HDMIRAWSTUTS3_CLKLOCK_MASK;
	while (count) {
		errCode = adv7441a_read_reg(vdev, USER_MAP_1, ADV7441A_REG_HDMIRAWSTUS3, &regdata);
		if (errCode)
			goto adv7441a_check_status_hdmi_exit;
		if ((regdata & mask) == mask)
			break;
		else
			count--;
		msleep(ADV7441A_RETRY_SLEEP_MS);
	}
	if (!count) {
		vin_error("Count time out, video not detected 0x%x!\n", regdata);
		errCode = -EINVAL;
		goto adv7441a_check_status_hdmi_exit;
	}

	errCode = adv7441a_check_status_hdmi_resolution(vdev);
	if (errCode)
		goto adv7441a_check_status_hdmi_exit;

	/*check hdmi mode */
	errCode = adv7441a_read_reg(vdev, HDMI_MAP, ADV7441A_REG_REG05, &regdata);
	if (errCode)
		goto adv7441a_check_status_hdmi_exit;
	if ((regdata & ADV7441A_REG05_HDCP_MASK) == ADV7441A_REG05_HDCP_INUSE)
		vin_info("HDCP protected!\n");
	if ((regdata & ADV7441A_REG05_HDMIMODE_MASK) != ADV7441A_REG05_HDMIMODE_HDMI)
		vin_info("Input mode is DVI!\n");

	if (((regdata & ADV7441A_REG05_HDMIMODE_MASK)== ADV7441A_REG05_HDMIMODE_HDMI) || check_hdmi_infoframe) {
		errCode = adv7441a_check_status_hdmi_infoframe(vdev);
		if (!errCode)
			goto adv7441a_check_status_check_bitwidth;
		else
			errCode = 0;
	}

	vin_info("Checking HDMI width and height...\n");
	pinfo->so_freq_hz = PLL_CLK_27MHZ;
	pinfo->so_pclk_freq_hz = PLL_CLK_27MHZ;
	pinfo->cap_start_x = pinfo->hs_back_porch;
	pinfo->cap_start_y = pinfo->f0_vs_front_porch +
		pinfo->f1_vs_front_porch;
	if ((pinfo->line_width == AMBA_VIDEO_1080P_W) &&
	    (pinfo->f0_height == AMBA_VIDEO_1080P_H) &&
	    (pinfo->f1_height == AMBA_VIDEO_1080P_H)) {
		//1920x1080P60
		pinfo->cap_vin_mode = AMBA_VIDEO_MODE_1080P;
		pinfo->video_format = AMBA_VIDEO_FORMAT_PROGRESSIVE;
		pinfo->frame_rate = AMBA_VIDEO_FPS_60;
		pinfo->cap_cap_w = AMBA_VIDEO_1080P_W;
		pinfo->cap_cap_h = AMBA_VIDEO_1080P_H;
		pinfo->video_system = AMBA_VIDEO_SYSTEM_NTSC;
		pinfo->aspect_ratio = AMBA_VIDEO_RATIO_16_9;
		pinfo->so_freq_hz = PLL_CLK_148_5MHZ;
		pinfo->so_pclk_freq_hz = PLL_CLK_148_5MHZ;
		pinfo->sync_start = ADV7441A_1080P60_601_CAP_SYNC_START;
	} else if ((pinfo->line_width == AMBA_VIDEO_1080P_W) &&
		   (pinfo->f0_height == (AMBA_VIDEO_1080P_H / 2)) &&
		   (pinfo->f1_height == (AMBA_VIDEO_1080P_H / 2))) {
		//1920x1080I60
		pinfo->cap_vin_mode = AMBA_VIDEO_MODE_1080I;
		pinfo->video_format = AMBA_VIDEO_FORMAT_INTERLACE;
		pinfo->frame_rate = AMBA_VIDEO_FPS_59_94;
		pinfo->cap_cap_w = AMBA_VIDEO_1080P_W;
		pinfo->cap_cap_h = AMBA_VIDEO_1080P_H;
		pinfo->video_system = AMBA_VIDEO_SYSTEM_NTSC;
		pinfo->aspect_ratio = AMBA_VIDEO_RATIO_16_9;
		pinfo->so_freq_hz = PLL_CLK_74_25D1001MHZ;
		pinfo->so_pclk_freq_hz = PLL_CLK_74_25D1001MHZ;
		pinfo->sync_start = ADV7441A_1080I60_601_CAP_SYNC_START;
	} else if ((pinfo->line_width == AMBA_VIDEO_720P_W) &&
		   (pinfo->f0_height == AMBA_VIDEO_720P_H) &&
		   (pinfo->f1_height == AMBA_VIDEO_720P_H)) {
		//1280x720P60
		pinfo->cap_vin_mode = AMBA_VIDEO_MODE_720P;
		pinfo->video_format = AMBA_VIDEO_FORMAT_PROGRESSIVE;
		pinfo->frame_rate = AMBA_VIDEO_FPS_59_94;//AMBA_VIDEO_FPS_60;
		pinfo->cap_cap_w = AMBA_VIDEO_720P_W;
		pinfo->cap_cap_h = AMBA_VIDEO_720P_H;
		pinfo->video_system = AMBA_VIDEO_SYSTEM_NTSC;
		pinfo->aspect_ratio = AMBA_VIDEO_RATIO_16_9;
		pinfo->so_freq_hz = PLL_CLK_74_25MHZ;
		pinfo->so_pclk_freq_hz = PLL_CLK_74_25MHZ;
		pinfo->sync_start = ADV7441A_720P60_601_CAP_SYNC_START;
	} else if ((pinfo->line_width == AMBA_VIDEO_PAL_W) &&
		   (pinfo->f0_height == AMBA_VIDEO_PAL_H) &&
		   (pinfo->f1_height == AMBA_VIDEO_PAL_H)) {
		//576P
		pinfo->cap_vin_mode = AMBA_VIDEO_MODE_D1_PAL;
		pinfo->video_format = AMBA_VIDEO_FORMAT_PROGRESSIVE;
		pinfo->frame_rate = AMBA_VIDEO_FPS_50;
		pinfo->cap_cap_w = AMBA_VIDEO_PAL_W;
		pinfo->cap_cap_h = AMBA_VIDEO_PAL_H;
		pinfo->video_system = AMBA_VIDEO_SYSTEM_PAL;
		pinfo->aspect_ratio = AMBA_VIDEO_RATIO_4_3;
		pinfo->sync_start = ADV7441A_576P_601_CAP_SYNC_START;
	} else if ((pinfo->line_width == (AMBA_VIDEO_PAL_W * 2)) &&
		   (pinfo->f0_height == (AMBA_VIDEO_PAL_H / 2)) &&
		   (pinfo->f1_height == (AMBA_VIDEO_PAL_H / 2))) {
		//576I
		pinfo->cap_vin_mode = AMBA_VIDEO_MODE_576I;
		pinfo->video_format = AMBA_VIDEO_FORMAT_INTERLACE;
		pinfo->frame_rate = AMBA_VIDEO_FPS_50;
		pinfo->cap_cap_w = AMBA_VIDEO_PAL_W * 2;
		pinfo->cap_cap_h = AMBA_VIDEO_PAL_H;
		pinfo->video_system = AMBA_VIDEO_SYSTEM_PAL;
		pinfo->aspect_ratio = AMBA_VIDEO_RATIO_4_3;
		pinfo->so_freq_hz = PLL_CLK_13_5MHZ;
		pinfo->so_pclk_freq_hz = PLL_CLK_13_5MHZ;
		pinfo->sync_start = ADV7441A_576I_601_CAP_SYNC_START;
	} else if ((pinfo->line_width == AMBA_VIDEO_PAL_W) &&
		   (pinfo->f0_height == (AMBA_VIDEO_PAL_H / 2)) &&
		   (pinfo->f1_height == (AMBA_VIDEO_PAL_H / 2))) {
		//576I
		pinfo->cap_vin_mode = AMBA_VIDEO_MODE_576I;
		pinfo->video_format = AMBA_VIDEO_FORMAT_INTERLACE;
		pinfo->frame_rate = AMBA_VIDEO_FPS_50;
		pinfo->cap_cap_w = AMBA_VIDEO_PAL_W;
		pinfo->cap_cap_h = AMBA_VIDEO_PAL_H;
		pinfo->video_system = AMBA_VIDEO_SYSTEM_PAL;
		pinfo->aspect_ratio = AMBA_VIDEO_RATIO_4_3;
		pinfo->so_freq_hz = PLL_CLK_13_5MHZ;
		pinfo->so_pclk_freq_hz = PLL_CLK_13_5MHZ;
		pinfo->sync_start = ADV7441A_576I_601_CAP_SYNC_START;
	} else if ((pinfo->line_width == AMBA_VIDEO_NTSC_W) &&
		   (pinfo->f0_height == AMBA_VIDEO_NTSC_H) &&
		   (pinfo->f1_height == AMBA_VIDEO_NTSC_H)) {
		//480P
		pinfo->cap_vin_mode = AMBA_VIDEO_MODE_D1_NTSC;
		pinfo->video_format = AMBA_VIDEO_FORMAT_PROGRESSIVE;
		pinfo->frame_rate = AMBA_VIDEO_FPS_59_94;//AMBA_VIDEO_FPS_60;
		pinfo->cap_cap_w = AMBA_VIDEO_NTSC_W;
		pinfo->cap_cap_h = AMBA_VIDEO_NTSC_H;
		pinfo->video_system = AMBA_VIDEO_SYSTEM_NTSC;
		pinfo->aspect_ratio = AMBA_VIDEO_RATIO_4_3;
		pinfo->sync_start = ADV7441A_480P_601_CAP_SYNC_START;
	} else if ((pinfo->line_width == (AMBA_VIDEO_NTSC_W * 2)) &&
		   (pinfo->f0_height == (AMBA_VIDEO_NTSC_H / 2)) &&
		   (pinfo->f1_height == (AMBA_VIDEO_NTSC_H / 2))) {
		//480I
		pinfo->cap_vin_mode = AMBA_VIDEO_MODE_480I;
		pinfo->video_format = AMBA_VIDEO_FORMAT_INTERLACE;
		pinfo->frame_rate = AMBA_VIDEO_FPS_59_94;
		pinfo->cap_cap_w = AMBA_VIDEO_NTSC_W * 2;
		pinfo->cap_cap_h = AMBA_VIDEO_NTSC_H;
		pinfo->video_system = AMBA_VIDEO_SYSTEM_NTSC;
		pinfo->aspect_ratio = AMBA_VIDEO_RATIO_4_3;
		pinfo->so_freq_hz = PLL_CLK_13_5MHZ;
		pinfo->so_pclk_freq_hz = PLL_CLK_13_5MHZ;
		pinfo->sync_start = ADV7441A_480I_601_CAP_SYNC_START;
		pinfo->cap_start_x = pinfo->hs_back_porch +
			pinfo->hs_front_porch + pinfo->hs_plus_width;
	} else if ((pinfo->line_width == AMBA_VIDEO_NTSC_W) &&
		   (pinfo->f0_height == (AMBA_VIDEO_NTSC_H / 2)) &&
		   (pinfo->f1_height == (AMBA_VIDEO_NTSC_H / 2))) {
		//480I
		pinfo->cap_vin_mode = AMBA_VIDEO_MODE_480I;
		pinfo->video_format = AMBA_VIDEO_FORMAT_INTERLACE;
		pinfo->frame_rate = AMBA_VIDEO_FPS_59_94;
		pinfo->cap_cap_w = AMBA_VIDEO_NTSC_W;
		pinfo->cap_cap_h = AMBA_VIDEO_NTSC_H;
		pinfo->video_system = AMBA_VIDEO_SYSTEM_NTSC;
		pinfo->aspect_ratio = AMBA_VIDEO_RATIO_4_3;
		pinfo->so_freq_hz = PLL_CLK_13_5MHZ;
		pinfo->so_pclk_freq_hz = PLL_CLK_13_5MHZ;
		pinfo->sync_start = ADV7441A_480I_601_CAP_SYNC_START;
	} else if ((pinfo->line_width == AMBA_VIDEO_VGA_W) &&
		   (pinfo->f0_height == AMBA_VIDEO_VGA_H) &&
		   (pinfo->f1_height == AMBA_VIDEO_VGA_H)) {
		//VGA
		pinfo->cap_vin_mode = AMBA_VIDEO_MODE_VGA;
		pinfo->video_format = AMBA_VIDEO_FORMAT_PROGRESSIVE;
		pinfo->frame_rate = AMBA_VIDEO_FPS_60;
		pinfo->cap_cap_w = AMBA_VIDEO_VGA_W;
		pinfo->cap_cap_h = AMBA_VIDEO_VGA_H;
		pinfo->video_system = AMBA_VIDEO_SYSTEM_AUTO;
		pinfo->aspect_ratio = AMBA_VIDEO_RATIO_AUTO;
		pinfo->sync_start = ADV7441A_TBD_CAP_SYNC_START;
	} else if ((pinfo->line_width == AMBA_VIDEO_SVGA_W) &&
		   (pinfo->f0_height == AMBA_VIDEO_SVGA_H) &&
		   (pinfo->f1_height == AMBA_VIDEO_SVGA_H)) {
		//SVGA
		pinfo->cap_vin_mode = AMBA_VIDEO_MODE_SVGA;
		pinfo->video_format = AMBA_VIDEO_FORMAT_PROGRESSIVE;
		pinfo->frame_rate = AMBA_VIDEO_FPS_60;
		pinfo->cap_cap_w = AMBA_VIDEO_SVGA_W;
		pinfo->cap_cap_h = AMBA_VIDEO_SVGA_H;
		pinfo->video_system = AMBA_VIDEO_SYSTEM_AUTO;
		pinfo->aspect_ratio = AMBA_VIDEO_RATIO_AUTO;
		pinfo->so_freq_hz = PLL_CLK_74_25D1001MHZ;
		pinfo->so_pclk_freq_hz = PLL_CLK_74_25D1001MHZ;
		pinfo->sync_start = ADV7441A_TBD_CAP_SYNC_START;
	} else if ((pinfo->line_width == AMBA_VIDEO_XGA_W) &&
		   (pinfo->f0_height == AMBA_VIDEO_XGA_H) &&
		   (pinfo->f1_height == AMBA_VIDEO_XGA_H)) {
		//XGA
		pinfo->cap_vin_mode = AMBA_VIDEO_MODE_XGA;
		pinfo->video_format = AMBA_VIDEO_FORMAT_PROGRESSIVE;
		pinfo->frame_rate = AMBA_VIDEO_FPS_60;
		pinfo->cap_cap_w = AMBA_VIDEO_XGA_W;
		pinfo->cap_cap_h = AMBA_VIDEO_XGA_H;
		pinfo->video_system = AMBA_VIDEO_SYSTEM_AUTO;
		pinfo->aspect_ratio = AMBA_VIDEO_RATIO_AUTO;
		pinfo->so_freq_hz = PLL_CLK_74_25D1001MHZ;
		pinfo->so_pclk_freq_hz = PLL_CLK_74_25D1001MHZ;
		pinfo->sync_start = ADV7441A_TBD_CAP_SYNC_START;
	} else if ((pinfo->line_width == AMBA_VIDEO_SXGA_W) &&
		   (pinfo->f0_height == AMBA_VIDEO_SXGA_H) &&
		   (pinfo->f1_height == AMBA_VIDEO_SXGA_H)) {
		//SXGA
		pinfo->cap_vin_mode = AMBA_VIDEO_MODE_SXGA;
		pinfo->video_format = AMBA_VIDEO_FORMAT_PROGRESSIVE;
		pinfo->frame_rate = AMBA_VIDEO_FPS_60;
		pinfo->cap_cap_w = AMBA_VIDEO_SXGA_W;
		pinfo->cap_cap_h = AMBA_VIDEO_SXGA_H;
		pinfo->video_system = AMBA_VIDEO_SYSTEM_AUTO;
		pinfo->aspect_ratio = AMBA_VIDEO_RATIO_AUTO;
		pinfo->so_freq_hz = PLL_CLK_74_25D1001MHZ;
		pinfo->so_pclk_freq_hz = PLL_CLK_74_25D1001MHZ;
		pinfo->sync_start = ADV7441A_TBD_CAP_SYNC_START;
	} else if ((pinfo->line_width == AMBA_VIDEO_UXGA_W) &&
		   (pinfo->f0_height == AMBA_VIDEO_UXGA_H) &&
		   (pinfo->f1_height == AMBA_VIDEO_UXGA_H)) {
		//UXGA
		pinfo->cap_vin_mode = AMBA_VIDEO_MODE_UXGA;
		pinfo->video_format = AMBA_VIDEO_FORMAT_PROGRESSIVE;
		pinfo->frame_rate = AMBA_VIDEO_FPS_60;
		pinfo->cap_cap_w = AMBA_VIDEO_UXGA_W;
		pinfo->cap_cap_h = AMBA_VIDEO_UXGA_H;
		pinfo->video_system = AMBA_VIDEO_SYSTEM_AUTO;
		pinfo->aspect_ratio = AMBA_VIDEO_RATIO_AUTO;
		pinfo->so_freq_hz = PLL_CLK_74_25D1001MHZ;
		pinfo->so_pclk_freq_hz = PLL_CLK_74_25D1001MHZ;
		pinfo->sync_start = ADV7441A_TBD_CAP_SYNC_START;
	} else if ((pinfo->line_width == AMBA_VIDEO_WXGA_W) &&
		   (pinfo->f0_height == AMBA_VIDEO_WXGA_H) &&
		   (pinfo->f1_height == AMBA_VIDEO_WXGA_H)) {
		//WXGA
		pinfo->cap_vin_mode = AMBA_VIDEO_MODE_WXGA;
		pinfo->video_format = AMBA_VIDEO_FORMAT_PROGRESSIVE;
		pinfo->frame_rate = AMBA_VIDEO_FPS_60;
		pinfo->cap_cap_w = AMBA_VIDEO_WXGA_W;
		pinfo->cap_cap_h = AMBA_VIDEO_WXGA_H;
		pinfo->video_system = AMBA_VIDEO_SYSTEM_AUTO;
		pinfo->aspect_ratio = AMBA_VIDEO_RATIO_AUTO;
		pinfo->so_freq_hz = PLL_CLK_74_25D1001MHZ;
		pinfo->so_pclk_freq_hz = PLL_CLK_74_25D1001MHZ;
		pinfo->sync_start = ADV7441A_TBD_CAP_SYNC_START;
	} else if ((pinfo->line_width == AMBA_VIDEO_WSXGA_W) &&
		   (pinfo->f0_height == AMBA_VIDEO_WSXGA_H) &&
		   (pinfo->f1_height == AMBA_VIDEO_WSXGA_H)) {
		//WSXGA
		pinfo->cap_vin_mode = AMBA_VIDEO_MODE_WSXGA;
		pinfo->video_format = AMBA_VIDEO_FORMAT_PROGRESSIVE;
		pinfo->frame_rate = AMBA_VIDEO_FPS_60;
		pinfo->cap_cap_w = AMBA_VIDEO_WSXGA_W;
		pinfo->cap_cap_h = AMBA_VIDEO_WSXGA_H;
		pinfo->video_system = AMBA_VIDEO_SYSTEM_AUTO;
		pinfo->aspect_ratio = AMBA_VIDEO_RATIO_AUTO;
		pinfo->so_freq_hz = PLL_CLK_74_25D1001MHZ;
		pinfo->so_pclk_freq_hz = PLL_CLK_74_25D1001MHZ;
		pinfo->sync_start = ADV7441A_TBD_CAP_SYNC_START;
	} else if ((pinfo->line_width == AMBA_VIDEO_WSXGAP_W) &&
		   (pinfo->f0_height == AMBA_VIDEO_WSXGAP_H) &&
		   (pinfo->f1_height == AMBA_VIDEO_WSXGAP_H)) {
		//WSXGAP
		pinfo->cap_vin_mode = AMBA_VIDEO_MODE_WSXGAP;
		pinfo->video_format = AMBA_VIDEO_FORMAT_PROGRESSIVE;
		pinfo->frame_rate = AMBA_VIDEO_FPS_60;
		pinfo->cap_cap_w = AMBA_VIDEO_WSXGAP_W;
		pinfo->cap_cap_h = AMBA_VIDEO_WSXGAP_H;
		pinfo->video_system = AMBA_VIDEO_SYSTEM_AUTO;
		pinfo->aspect_ratio = AMBA_VIDEO_RATIO_AUTO;
		pinfo->so_freq_hz = PLL_CLK_148_5D1001MHZ;
		pinfo->so_pclk_freq_hz = PLL_CLK_148_5D1001MHZ;
		pinfo->sync_start = ADV7441A_TBD_CAP_SYNC_START;
	} else if ((pinfo->line_width == AMBA_VIDEO_WUXGA_W) &&
		   (pinfo->f0_height == AMBA_VIDEO_WUXGA_H) &&
		   (pinfo->f1_height == AMBA_VIDEO_WUXGA_H)) {
		//WUXGA
		pinfo->cap_vin_mode = AMBA_VIDEO_MODE_WUXGA;
		pinfo->video_format = AMBA_VIDEO_FORMAT_PROGRESSIVE;
		pinfo->frame_rate = AMBA_VIDEO_FPS_60;
		pinfo->cap_cap_w = AMBA_VIDEO_WUXGA_W;
		pinfo->cap_cap_h = AMBA_VIDEO_WUXGA_H;
		pinfo->video_system = AMBA_VIDEO_SYSTEM_AUTO;
		pinfo->aspect_ratio = AMBA_VIDEO_RATIO_AUTO;
		pinfo->so_freq_hz = PLL_CLK_148_5D1001MHZ;
		pinfo->so_pclk_freq_hz = PLL_CLK_148_5D1001MHZ;
		pinfo->sync_start = ADV7441A_TBD_CAP_SYNC_START;
	} else {
		vin_error("Not support video format %dx[%d/%d]\n",
			pinfo->line_width, pinfo->f0_height,
			pinfo->f1_height);
		adv7441a_print_info(vdev);
		errCode = -1;
		goto adv7441a_check_status_hdmi_exit;
	}

adv7441a_check_status_check_bitwidth:
	if (pinfo->video_format == AMBA_VIDEO_FORMAT_PROGRESSIVE)
		pinfo->input_format = AMBA_VIN_INPUT_FORMAT_YUV_422_PROG;
	else
		pinfo->input_format = AMBA_VIN_INPUT_FORMAT_YUV_422_INTLC;

	errCode = adv7441a_read_reg(vdev, USER_MAP, ADV7441A_REG_OUTCTRL, &regdata);

	switch (regdata & ADV7441A_OUTCTRL_OFSEL_MASK) {
	case ADV7441A_OUTCTRL_OFSEL_8BIT656:
		pinfo->bit_resolution = AMBA_VIDEO_BITS_8;
		pinfo->input_type = AMBA_VIDEO_TYPE_YUV_656;
		break;

	case ADV7441A_OUTCTRL_OFSEL_16BIT:
		if (bt601mode) {
			pinfo->cap_start_x = ADV7441A_1080P60_601_CAP_START_X,
			pinfo->cap_start_y = ADV7441A_1080P60_601_CAP_START_Y,
			pinfo->sync_start = ADV7441A_1080P60_601_CAP_SYNC_START,
			pinfo->bit_resolution = AMBA_VIDEO_BITS_16;
			pinfo->input_type = AMBA_VIDEO_TYPE_YUV_601;
		} else {
			pinfo->cap_start_x = ADV7441A_EMBMODE_CAP_START_X,
			pinfo->cap_start_y = ADV7441A_EMBMODE_CAP_START_Y,
			pinfo->sync_start = ADV7441A_EMBMODE_CAP_SYNC_START,
			pinfo->bit_resolution = AMBA_VIDEO_BITS_16;
			pinfo->input_type = AMBA_VIDEO_TYPE_YUV_656;
		}
		break;

	default:
		vin_error("not support ouput format\n");
		break;
	}

adv7441a_check_status_hdmi_exit:
	return errCode;
}

static int adv7441a_check_status(struct vin_device *vdev)
{
	int errCode = 0;
	struct adv7441a_priv *pinfo;

	pinfo = (struct adv7441a_priv *) vdev->priv;

	pinfo->line_width = 0;
	pinfo->f0_height = 0;
	pinfo->f1_height = 0;
	pinfo->f0_vs_back_porch = 0;
	pinfo->f1_vs_back_porch = 1;

	switch (adv7441a_source_table[pinfo->cap_src_id].source_processor) {
	case ADV7441A_PROCESSOR_SDP:
		//errCode = adv7441a_check_status_analog(vdev);
		break;

	case ADV7441A_PROCESSOR_CP:
		//errCode = adv7441a_check_status_cp(vdev);
		break;

	case ADV7441A_PROCESSOR_HDMI:
		errCode = adv7441a_check_status_hdmi(vdev);
		break;

	default:
		vin_error("Unknown processor type\n");
		errCode = -EINVAL;
		break;
	}

	return errCode;
}

static int adv7441a_set_format(struct vin_device *vdev, struct vin_video_format *format)
{
	int rval;
	struct adv7441a_priv *pinfo;

	pinfo = (struct adv7441a_priv *) vdev->priv;

	rval = adv7441a_select_source(vdev);
	if (rval)
		return rval;

	rval = adv7441a_check_status(vdev);
	if (rval)
		return rval;

	if (pinfo->fix_reg_table)
		adv7441a_fill_share_regs(vdev, pinfo->fix_reg_table);

	adv7441a_print_info(vdev);

	rval = adv7441a_set_vin_mode(vdev, format);
	if (rval < 0)
		return rval;

	msleep(100);

	return 0;
}

static int adv7441a_get_format(struct vin_device *vdev)
{
	int rval;
	struct adv7441a_priv *pinfo;

	pinfo = (struct adv7441a_priv *) vdev->priv;

	rval = adv7441a_select_source(vdev);
	if (rval)
		return rval;

	rval = adv7441a_check_status(vdev);
	if (rval)
		return rval;

	vdev->formats->video_mode = AMBA_VIDEO_MODE_AUTO;
	vdev->formats->def_start_x = pinfo->cap_start_x;
	vdev->formats->def_start_y = pinfo->cap_start_y;
	vdev->formats->def_width = pinfo->cap_cap_w;
	vdev->formats->def_height = pinfo->cap_cap_h;
	vdev->formats->default_fps = pinfo->frame_rate;
	vdev->formats->max_fps = pinfo->frame_rate;
	vdev->formats->ratio = pinfo->aspect_ratio;
	vdev->formats->format = pinfo->video_format;
	vdev->formats->bits = pinfo->bit_resolution;

	return 0;
}

static int adv7441a_set_fps(struct vin_device *vdev, int fps)
{
	return 0;
}

static struct vin_ops adv7441a_ops = {
	.init_device = adv7441a_init_device,
	.set_format = adv7441a_set_format,
	.set_frame_rate = adv7441a_set_fps,
	.get_format = adv7441a_get_format,
};

static int adv7441a_set_i2c_client(struct i2c_client *client,
	struct i2c_client *_client, struct adv7441a_priv *pinfo)
{
	u32				addr;

	i2c_set_clientdata(_client, pinfo);
	addr = client->addr;
	memcpy(client, _client, sizeof(*client));
	client->addr = addr;
	strlcpy(client->name, adv7441a_name, sizeof (client->name));

	return 0;
}

static int adv7441a_update_idc_usrmap2(struct vin_device *vdev)
{
	int errCode = 0;
	u8 regdata;
	struct adv7441a_priv *pinfo;

	pinfo = (struct adv7441a_priv *) vdev->priv;

	errCode = adv7441a_read_reg(vdev, USER_MAP, ADV7441A_REG_USS2, &regdata);
	pinfo->idc_usrmap2.addr = (regdata + CONFIG_I2C_AMBARELLA_ADV7441A_ALSB_OFFSET) >> 1;
	vin_debug("User map2 addr is 0x%x\n", (pinfo->idc_usrmap2.addr << 1));

	return errCode;
}

static int adv7441a_update_idc_maps(struct vin_device *vdev)
{
	int errCode = 0;
	u8 regdata;
	struct adv7441a_priv *pinfo;

	pinfo = (struct adv7441a_priv *) vdev->priv;

	errCode = adv7441a_read_reg(vdev, USER_MAP_2, ADV7441A_REG_HID, &regdata);
	pinfo->idc_revmap.addr = (regdata + CONFIG_I2C_AMBARELLA_ADV7441A_ALSB_OFFSET) >> 1;
	vin_debug("Hidden Map addr is 0x%x\n", (pinfo->idc_revmap.addr << 1));

	errCode = adv7441a_read_reg(vdev, USER_MAP_2, ADV7441A_REG_USS1, &regdata);
	pinfo->idc_usrmap1.addr = (regdata + CONFIG_I2C_AMBARELLA_ADV7441A_ALSB_OFFSET) >> 1;
	vin_debug("User Sub Map 1 addr is 0x%x\n", (pinfo->idc_usrmap1.addr << 1));

	errCode = adv7441a_read_reg(vdev, USER_MAP_2, ADV7441A_REG_VDP, &regdata);
	pinfo->idc_vdpmap.addr = (regdata + CONFIG_I2C_AMBARELLA_ADV7441A_ALSB_OFFSET) >> 1;
	vin_debug("VDP Map addr is 0x%x\n", (pinfo->idc_vdpmap.addr << 1));

	errCode = adv7441a_read_reg(vdev, USER_MAP_2, ADV7441A_REG_KSV, &regdata);
	pinfo->idc_rksvmap.addr = (regdata + CONFIG_I2C_AMBARELLA_ADV7441A_ALSB_OFFSET) >> 1;
	vin_debug("KSV Map addr is 0x%x\n", (pinfo->idc_rksvmap.addr << 1));

	errCode = adv7441a_read_reg(vdev, USER_MAP_2, ADV7441A_REG_EDID, &regdata);
	pinfo->idc_edidmap.addr = (regdata + CONFIG_I2C_AMBARELLA_ADV7441A_ALSB_OFFSET) >> 1;
	vin_debug("EDID Map addr is 0x%x\n", (pinfo->idc_edidmap.addr << 1));

	errCode = adv7441a_read_reg(vdev, USER_MAP_2, ADV7441A_REG_HDMI, &regdata);
	pinfo->idc_hdmimap.addr = (regdata + CONFIG_I2C_AMBARELLA_ADV7441A_ALSB_OFFSET) >> 1;
	vin_debug("HDMI Map addr is 0x%x\n", (pinfo->idc_hdmimap.addr << 1));

	return errCode;
}

static int adv7441a_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int rval = 0;
	struct vin_device *vdev;
	struct adv7441a_priv *pinfo;
	u32	edid_size;
	const struct firmware *pedid;

	vdev = ambarella_vin_create_device(client->name, DECODER_ADV7441A, sizeof(struct adv7441a_priv));
	if (!vdev)
		return -ENOMEM;

	vdev->intf_id = 0;
	vdev->dev_type = VINDEV_TYPE_DECODER;
	vdev->sub_type = VINDEV_SUBTYPE_HDMI;
	vdev->default_mode = AMBA_VIDEO_MODE_AUTO;

	pinfo = (struct adv7441a_priv *)vdev->priv;

	/* EDID Data */
	if (edid_data != NULL) {
		rval = request_firmware(&pedid, edid_data,
							&client->adapter->dev);
		if (rval) {
			vin_error("Can't load EDID from %s!\n", edid_data);
			goto adv7441a_probe_err;
		}
		vin_info("Load %s, size = %d\n", edid_data, pedid->size);
		if (pedid->size > ADV7441A_EDID_TABLE_SIZE)
			edid_size = ADV7441A_EDID_TABLE_SIZE;
		else
			edid_size = pedid->size;
		memcpy(pinfo->edid, pedid->data, edid_size);
		pinfo->valid_edid = 1;
	}

	i2c_set_clientdata(client, vdev);

	pinfo->idc_usrmap.addr = client->addr;
	rval = adv7441a_set_i2c_client(&pinfo->idc_usrmap, client, pinfo);
	if (rval)
		goto adv7441a_probe_err;

	rval = adv7441a_update_idc_usrmap2(vdev);
	if (rval)
		goto adv7441a_probe_err;
	rval = adv7441a_set_i2c_client(&pinfo->idc_usrmap2, client, pinfo);
	if (rval)
		goto adv7441a_probe_err;

	rval = adv7441a_update_idc_maps(vdev);
	if (rval)
		goto adv7441a_probe_err;
	rval = adv7441a_set_i2c_client(&pinfo->idc_usrmap1, client, pinfo);
	if (rval)
		goto adv7441a_probe_err;
	rval = adv7441a_set_i2c_client(&pinfo->idc_vdpmap, client, pinfo);
	if (rval)
		goto adv7441a_probe_err;
	rval = adv7441a_set_i2c_client(&pinfo->idc_hdmimap, client, pinfo);
	if (rval)
		goto adv7441a_probe_err;
	rval = adv7441a_set_i2c_client(&pinfo->idc_rksvmap, client, pinfo);
	if (rval)
		goto adv7441a_probe_err;
	rval = adv7441a_set_i2c_client(&pinfo->idc_edidmap, client, pinfo);
	if (rval)
		goto adv7441a_probe_err;
	rval = adv7441a_set_i2c_client(&pinfo->idc_revmap, client, pinfo);
	if (rval)
		goto adv7441a_probe_err;

	rval = ambarella_vin_register_device(vdev, &adv7441a_ops,
		adv7441a_formats, sizeof(adv7441a_formats), adv7441a_plls, ARRAY_SIZE(adv7441a_plls));
	if (rval < 0)
		goto adv7441a_probe_err;

	pinfo->cap_src_id = 0;

	return 0;

adv7441a_probe_err:
	ambarella_vin_free_device(vdev);
	return rval;
}

static int adv7441a_remove(struct i2c_client *client)
{
	struct vin_device *vdev;

	vdev = (struct vin_device *)i2c_get_clientdata(client);
	ambarella_vin_unregister_device(vdev);
	ambarella_vin_free_device(vdev);

	return 0;
}

static const struct i2c_device_id adv7441a_idtable[] = {
	{ "adv7441a", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, adv7441a_idtable);

static struct i2c_driver i2c_driver_adv7441a = {
	.driver = {
		.name	= "adv7441a",
	},

	.id_table	= adv7441a_idtable,
	.probe		= adv7441a_probe,
	.remove		= adv7441a_remove,

};

static int __init adv7441a_init(void)
{
	int bus, addr, rval;

	bus = bus_addr >> 16;
	addr = bus_addr & 0xffff;

	rval = ambpriv_i2c_update_addr("adv7441a", bus, addr);
	if (rval < 0)
		return rval;

	rval = i2c_add_driver(&i2c_driver_adv7441a);
	if (rval < 0)
		return rval;

	return 0;
}

static void __exit adv7441a_exit(void)
{
	i2c_del_driver(&i2c_driver_adv7441a);
}

module_init(adv7441a_init);
module_exit(adv7441a_exit);

MODULE_DESCRIPTION("Ambarella Dummy Decoder");
MODULE_AUTHOR("Anthony Ginger <hfjiang@ambarella.com>");
MODULE_LICENSE("Proprietary");

