/*
 * kernel/private/drivers/ambarella/vin/decoders/ambdd/ambdd.c
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
#include <linux/idr.h>
#include <plat/ambsyncproc.h>
#include <iav_utils.h>
#include <vin_api.h>
#include <plat/clk.h>

#define AMBDD_TBD_CAP_START_X				(0)
#define AMBDD_TBD_CAP_START_Y				(0)
#define AMBDD_TBD_CAP_SYNC_START			(0x8000)

/* ========================================================================== */
static int intf_id = 0;
module_param(intf_id, int, 0644);
MODULE_PARM_DESC(intf_id, "Vin Interface");

static int video_type = AMBA_VIDEO_TYPE_YUV_656;
module_param(video_type, int, 0644);
MODULE_PARM_DESC(video_type, "Video Input Type");

static int video_format = AMBA_VIDEO_FORMAT_PROGRESSIVE;
module_param(video_format, int, 0644);
MODULE_PARM_DESC(video_format, "Video Input Format");

static int video_fps = AMBA_VIDEO_FPS_60;
module_param(video_fps, int, 0644);
MODULE_PARM_DESC(video_fps, "Video Input Frame Rate");

static int video_bits = AMBA_VIDEO_BITS_16;
module_param(video_bits, int, 0644);
MODULE_PARM_DESC(video_bits, "Video Input Width");

static int video_phase = 0;
module_param(video_phase, int, 0644);
MODULE_PARM_DESC(video_phase, "Video Input Sync Phase");

static int video_mode = AMBA_VIDEO_MODE_1080P;
module_param(video_mode, int, 0644);
MODULE_PARM_DESC(video_mode, "Video Input Mode");

static int video_data_edge = SENSOR_DATA_RISING_EDGE;
module_param(video_data_edge, int, 0644);
MODULE_PARM_DESC(video_data_edge, "Video Input Data Edge");

static int video_yuv_mode = 0;
module_param(video_yuv_mode, int, 0644);
MODULE_PARM_DESC(video_yuv_mode, "0 ~ 3 (CrY0CbY1 ~ Y0CbY1Cr)");

static int video_clkout = PLL_CLK_27MHZ;
module_param(video_clkout, int, 0644);
MODULE_PARM_DESC(video_clkout, "Video Clock, Output");

static int video_pixclk = PLL_CLK_74_25MHZ;
module_param(video_pixclk, int, 0644);
MODULE_PARM_DESC(video_pixclk, "Video Pixel Clock");

static int cap_start_x = AMBDD_TBD_CAP_START_X;
static int cap_start_y = AMBDD_TBD_CAP_START_Y;
static int cap_cap_w = 1920;
static int cap_cap_h = 1080;
static int sync_start = AMBDD_TBD_CAP_SYNC_START;

module_param(cap_start_x, int, 0644);
module_param(cap_start_y, int, 0644);
module_param(cap_cap_w, int, 0644);
module_param(cap_cap_h, int, 0644);
module_param(sync_start, int, 0644);
/* ========================================================================== */
struct ambdd_priv {
	struct vin_video_format ambdd_formats;
	struct vin_video_pll  ambdd_plls;
};

static int ambdd_set_vin_mode(struct vin_device *vdev, struct vin_video_format *format)
{
	struct vin_device_config ambdd_config;
	u32 brgb = 0;

	memset(&ambdd_config, 0, sizeof (ambdd_config));

	ambdd_config.interface_type = SENSOR_PARALLEL_LVCMOS;
	ambdd_config.yuv_pixel_order = video_yuv_mode;

	ambdd_config.plvcmos_cfg.vs_hs_polarity = video_phase ^ 0x3;
	ambdd_config.plvcmos_cfg.data_edge = video_data_edge;

	switch(video_type) {
	case AMBA_VIDEO_TYPE_YUV_601:
		ambdd_config.plvcmos_cfg.paralle_sync_type = SENSOR_PARALLEL_SYNC_601;
		break;

	case AMBA_VIDEO_TYPE_YUV_656:
	case AMBA_VIDEO_TYPE_YUV_BT1120:
		ambdd_config.plvcmos_cfg.paralle_sync_type = SENSOR_PARALLEL_SYNC_656;
		ambdd_config.plvcmos_cfg.sync_code_style = SENSOR_SYNC_STYLE_ITU656;
		break;

	case AMBA_VIDEO_TYPE_RGB_656:
	case AMBA_VIDEO_TYPE_RGB_BT1120:
		brgb = 1;
		ambdd_config.plvcmos_cfg.paralle_sync_type = SENSOR_PARALLEL_SYNC_656;
		ambdd_config.plvcmos_cfg.sync_code_style = SENSOR_SYNC_STYLE_ITU656;
		break;
	case AMBA_VIDEO_TYPE_RGB_601:
	case AMBA_VIDEO_TYPE_RGB_RAW:
	default:
		brgb = 1;
		ambdd_config.plvcmos_cfg.paralle_sync_type = SENSOR_PARALLEL_SYNC_601;
		break;
	}

	switch(video_bits) {
	case AMBA_VIDEO_BITS_10:
		ambdd_config.bit_resolution = AMBA_VIDEO_BITS_10;
		break;
	case AMBA_VIDEO_BITS_12:
		ambdd_config.bit_resolution = AMBA_VIDEO_BITS_12;
		break;
	case AMBA_VIDEO_BITS_14:
		ambdd_config.bit_resolution = AMBA_VIDEO_BITS_14;
		break;
	case AMBA_VIDEO_BITS_8:
	case AMBA_VIDEO_BITS_16:
	default:
		ambdd_config.bit_resolution = AMBA_VIDEO_BITS_8;
		break;
	}

	if (!brgb) {
		if (video_bits == AMBA_VIDEO_BITS_8)
			ambdd_config.input_mode = SENSOR_YUV_1PIX;
		else
			ambdd_config.input_mode = SENSOR_YUV_2PIX;
	} else {
		ambdd_config.input_mode = SENSOR_RGB_1PIX;
	}

	ambdd_config.cap_win.x = format->def_start_x;
	ambdd_config.cap_win.y = format->def_start_y;
	ambdd_config.cap_win.width = format->def_width;
	if (video_format == AMBA_VIDEO_FORMAT_INTERLACE) {
		ambdd_config.cap_win.height = format->def_height >> 1;
		ambdd_config.input_format = AMBA_VIN_INPUT_FORMAT_YUV_422_INTLC;
	} else {
		ambdd_config.cap_win.height = format->def_height;
		ambdd_config.input_format = AMBA_VIN_INPUT_FORMAT_YUV_422_PROG;
	}

	ambdd_config.sensor_id = GENERIC_SENSOR;
	ambdd_config.video_format = video_format;

	return ambarella_set_vin_config(vdev, &ambdd_config);
}

static int ambdd_set_format(struct vin_device *vdev, struct vin_video_format *format)
{
	int rval;

	rval = ambdd_set_vin_mode(vdev, format);
	if (rval < 0)
		return rval;

	return 0;
}

static int ambdd_get_format(struct vin_device *vdev)
{
	struct ambdd_priv *pinfo = (struct ambdd_priv *)vdev->priv;

	memcpy(vdev->formats, &pinfo->ambdd_formats, sizeof(pinfo->ambdd_formats));
	return 0;
}

static int ambdd_set_fps(struct vin_device *vdev, int fps)
{
	return 0;
}

static struct vin_ops ambdd_ops = {
	.set_format = ambdd_set_format,
	.get_format = ambdd_get_format,
	.set_frame_rate = ambdd_set_fps,
};

static int ambdd_drv_probe(struct ambpriv_device *ambdev)
{
	int rval = 0;
	struct vin_device *vdev;
	struct ambdd_priv *pinfo;

	vdev = ambarella_vin_create_device(ambdev->name,
		DECODER_AMBDD, sizeof(struct ambdd_priv));
	if (!vdev)
		return -ENOMEM;

	vdev->intf_id = intf_id;
	vdev->dev_type = VINDEV_TYPE_DECODER;
	vdev->sub_type = VINDEV_SUBTYPE_CVBS;
	vdev->default_mode = video_mode;

	pinfo = (struct ambdd_priv *)vdev->priv;

	pinfo->ambdd_formats.video_mode = video_mode;
	pinfo->ambdd_formats.def_start_x = cap_start_x;
	pinfo->ambdd_formats.def_start_y = cap_start_y;
	pinfo->ambdd_formats.def_width = cap_cap_w;
	pinfo->ambdd_formats.def_height = cap_cap_h;
	/* device mode */
	pinfo->ambdd_formats.device_mode = 0;
	pinfo->ambdd_formats.pll_idx = 0;
	pinfo->ambdd_formats.width = cap_cap_w;
	pinfo->ambdd_formats.height = cap_cap_h;
	pinfo->ambdd_formats.format = video_format;
	pinfo->ambdd_formats.type = video_type;
	pinfo->ambdd_formats.bits = video_bits;
	pinfo->ambdd_formats.ratio = AMBA_VIDEO_RATIO_AUTO;
	pinfo->ambdd_formats.max_fps = video_fps;
	pinfo->ambdd_formats.default_fps = video_fps;

	pinfo->ambdd_plls.clk_si = video_clkout;
	pinfo->ambdd_plls.pixelclk = video_pixclk;

	rval = ambarella_vin_register_device(vdev, &ambdd_ops,
			&pinfo->ambdd_formats, 1, &pinfo->ambdd_plls, 1);
	if (rval < 0) {
		ambarella_vin_free_device(vdev);
		return rval;
	}

	ambpriv_set_drvdata(ambdev, vdev);

	vin_info("%s[%d@%d] probed!\n", ambdev->name, vdev->vsrc_id, vdev->intf_id);

	return 0;
}

static int ambdd_drv_remove(struct ambpriv_device *ambdev)
{
	struct vin_device *vdev = ambpriv_get_drvdata(ambdev);

	ambarella_vin_unregister_device(vdev);
	ambarella_vin_free_device(vdev);

	return 0;
}

static struct ambpriv_driver ambdd_driver = {
	.probe = ambdd_drv_probe,
	.remove = ambdd_drv_remove,
	.driver = {
		.name = "ambdd",
		.owner = THIS_MODULE,
	}
};

static struct ambpriv_device *ambdd_device;

static int __init ambdd_init(void)
{
	int rval = 0;

	ambdd_device = ambpriv_create_bundle(&ambdd_driver, NULL, -1, NULL, -1);
	if (IS_ERR(ambdd_device))
		rval = PTR_ERR(ambdd_device);

	return rval;
}

static void __exit ambdd_exit(void)
{
	ambpriv_device_unregister(ambdd_device);
	ambpriv_driver_unregister(&ambdd_driver);
}

module_init(ambdd_init);
module_exit(ambdd_exit);

MODULE_DESCRIPTION("Ambarella Dummy Decoder");
MODULE_AUTHOR("Anthony Ginger <hfjiang@ambarella.com>");
MODULE_LICENSE("Proprietary");

