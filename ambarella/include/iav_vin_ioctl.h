/*
 * iav_vin_ioctl.h
 *
 * History:
 *	2013/09/04 - [Cao Rongrong] Created file
 *
 * Copyright (C) 2015 Ambarella, Inc.
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

#ifndef __IAV_VIN_IOCTL_H__
#define __IAV_VIN_IOCTL_H__

/* ==========================================================================*/
#include <linux/ioctl.h>

/* ==========================================================================*/
#define AMBA_VINDEV_MAX_NUM			32

/* ==========================================================================*/
enum {
	VINDEV_TYPE_SENSOR			= 0,
	VINDEV_TYPE_DECODER			= 1,
};

enum {
	VINDEV_SUBTYPE_CMOS			= 0x000,
	VINDEV_SUBTYPE_CCD			= 0x001,
	VINDEV_SUBTYPE_CVBS			= 0x100,
	VINDEV_SUBTYPE_SVIDEO			= 0x101,
	VINDEV_SUBTYPE_YPBPR			= 0x102,
	VINDEV_SUBTYPE_HDMI			= 0x103,
	VINDEV_SUBTYPE_VGA			= 0x104,
};

enum amba_vindev_mirror_pattern_e {
	VINDEV_MIRROR_NONE			= 0,
	VINDEV_MIRROR_VERTICALLY		= 1,
	VINDEV_MIRROR_HORRIZONTALLY		= 2,
	VINDEV_MIRROR_HORRIZONTALLY_VERTICALLY	= 3,
	VINDEV_MIRROR_AUTO			= 255,
};

enum amba_vindev_bayer_pattern_e {
	VINDEV_BAYER_PATTERN_RG			= 0,
	VINDEV_BAYER_PATTERN_BG			= 1,
	VINDEV_BAYER_PATTERN_GR			= 2,
	VINDEV_BAYER_PATTERN_GB			= 3,
	/* RGB/IR CFA, IR instead of B */
	VINDEV_BAYER_PATTERN_RG_GI		= 4,
	VINDEV_BAYER_PATTERN_IG_GR		= 5,
	VINDEV_BAYER_PATTERN_GR_IG		= 6,
	VINDEV_BAYER_PATTERN_GI_RG		= 7,
	/* RGB/IR CFA, IR instead of R */
	VINDEV_BAYER_PATTERN_BG_GI		= 8,
	VINDEV_BAYER_PATTERN_IG_GB		= 9,
	VINDEV_BAYER_PATTERN_GB_IG		= 10,
	VINDEV_BAYER_PATTERN_GI_BG		= 11,

	VINDEV_BAYER_PATTERN_AUTO		= 255,
};

enum {
	/* Onsemi Sensor */
	SENSOR_MT9J001				= 0x00000000,
	SENSOR_MT9M033				= 0x00000001,
	SENSOR_MT9P001				= 0x00000002,
	SENSOR_MT9V136				= 0x00000003,
	SENSOR_MT9T002				= 0x00000004,
	SENSOR_AR0331				= 0x00000005,
	SENSOR_AR0130				= 0x00000006,
	SENSOR_AR0141				= 0x00000007,
	SENSOR_AR0230				= 0x00000008,
	SENSOR_AR0237				= 0x00000009,

	/* OV Sensor */
	SENSOR_OV10620				= 0x00001000,
	SENSOR_OV14810				= 0x00001001,
	SENSOR_OV2710				= 0x00001002,
	SENSOR_OV5653				= 0x00001003,
	SENSOR_OV7720				= 0x00001004,
	SENSOR_OV7725				= 0x00001005,
	SENSOR_OV7740				= 0x00001006,
	SENSOR_OV9710				= 0x00001007,
	SENSOR_OV10630				= 0x00001008,
	SENSOR_OV9726				= 0x00001009,
	SENSOR_OV4689				= 0x0000100A,
	SENSOR_OV9718				= 0x0000100B,
	SENSOR_OV5658				= 0x0000100C,
	SENSOR_OV9750				= 0x0000100D,
	SENSOR_OV9732				= 0x0000100E,
	SENSOR_OV2718				= 0x0000100F,

	/* Samsung Sensor */
	SENSOR_S5K3E2FX				= 0x00002000,
	SENSOR_S5K4AWFX				= 0x00002001,
	SENSOR_S5K5B3GX				= 0x00002002,

	/* Sony Sensor */
	SENSOR_IMX035				= 0x00003000,
	SENSOR_IMX036				= 0x00003001,
	SENSOR_IMX072				= 0x00003002,
	SENSOR_IMX122				= 0x00003003,
	SENSOR_IMX121				= 0x00003004,
	SENSOR_IMX104				= 0x00003005,
	SENSOR_IMX136				= 0x00003006,
	SENSOR_IMX105				= 0x00003007,
	SENSOR_IMX172				= 0x00003008,
	SENSOR_IMX178				= 0x00003009,
	SENSOR_IMX123				= 0x0000300A,
	SENSOR_IMX123_DCG			= 0x0000300B,
	SENSOR_IMX322				= 0x0000300C,
	SENSOR_IMX124				= 0x0000300D,
	SENSOR_IMX224				= 0x0000300E,
	SENSOR_IMX174				= 0x0000300F,
	SENSOR_IMX291				= 0x00003010,
	SENSOR_IMX185				= 0x00003011,
	SENSOR_IMX226				= 0x00003012,
	SENSOR_IMX290				= 0x00003013,
	SENSOR_IMX274				= 0x00003014,
	SENSOR_IMX326				= 0x00003015,

	/* Panasonic Sensor */
	SENSOR_MN34041PL			= 0x00004000,
	SENSOR_MN34031PL			= 0x00004001,
	SENSOR_MN34220PL			= 0x00004002,
	SENSOR_MN34210PL			= 0x00004003,
	SENSOR_MN34227PL			= 0x00004004,
	SENSOR_MN34420PL			= 0x00004005,

	/*altera fpga vin in*/
	SENSOR_ALTERA_FPGA			= 0x00005000,

	/* Dummy Sensor */
	SENSOR_AMBDS				= 0x00006000,

	/* Customized Sensor */
	SENSOR_CUSTOM_FIRST			= 0x00010000,

	/* ADI Decoder */
	DECODER_ADV7403				= 0x80000000,
	DECODER_ADV7441A			= 0x80000001,
	DECODER_ADV7443				= 0x80000002,

	/* RICHNEX Decoder */
	DECODER_RN6240				= 0x80001000,

	/* TI Decoder */
	DECODER_TVP5150				= 0x80002000,

	/* Techwell Decoder */
	DECODER_TW2864				= 0x80003000,
	DECODER_TW9910				= 0x80003001,

	DECODER_GS2970				= 0x80004000,

	/* Dummy Decoder */
	DECODER_AMBDD				= 0x8FFFFFFF,
};

enum {
	VINDEV_SET_AGC_ONLY		= 0,
	VINDEV_SET_SHT_ONLY		= 1,
	VINDEV_SET_AGC_SHT		= 2,
};

/* ==========================================================================*/
struct vindev_devinfo {
	unsigned int vsrc_id;
	unsigned int intf_id;
	char name[32];
	unsigned int dev_type;
	unsigned int sub_type;
	unsigned int sensor_id;
};

struct vindev_mode {
	unsigned int vsrc_id;
	unsigned int video_mode;
	unsigned int hdr_mode;
	unsigned int bits;
};

struct vindev_fps {
	unsigned int vsrc_id;
	unsigned int fps;
};

struct vindev_video_info {
	unsigned int vsrc_id;
	struct amba_video_info info;
};

struct vindev_shutter {
	unsigned int vsrc_id;
	unsigned int shutter;
};

struct vindev_shutter_row {
	unsigned int vsrc_id;
	unsigned int shutter_row;
};

struct vindev_agc_shutter {
	unsigned int vsrc_id;
	unsigned int shutter_row;
	unsigned int agc_idx;
	unsigned int mode;
};

struct vindev_agc {
	unsigned int vsrc_id;
	unsigned int agc;
	unsigned int agc_max;
	unsigned int agc_min;
	unsigned int agc_step;
	unsigned int wdr_again_idx_min;
	unsigned int wdr_again_idx_max;
	unsigned int wdr_dgain_idx_min;
	unsigned int wdr_dgain_idx_max;
};

struct vindev_agc_idx {
	unsigned int vsrc_id;
	unsigned int agc_idx;
};

struct vindev_mirror {
	unsigned int vsrc_id;
	unsigned int pattern;			//enum amba_vindev_mirror_pattern_e;
	unsigned int bayer_pattern;		//enum amba_vindev_bayer_pattern_e;
};

struct vindev_reg {
	unsigned int vsrc_id;
	unsigned int addr;
	unsigned int data;
};

struct vindev_eisinfo {
	unsigned int vsrc_id;
	unsigned int vb_time; // ns
	unsigned short sensor_cell_width; // um * 100
	unsigned short sensor_cell_height; // um * 100
	unsigned char column_bin; // binning pixels
	unsigned char row_bin; // binning pixels
};

struct vindev_wdr_gain_info {
	unsigned int vsrc_id;
	unsigned int gain_idx;
};

struct vindev_aaa_info {
	unsigned int vsrc_id;
	unsigned int sensor_id;
	unsigned int bayer_pattern;
	unsigned int agc_step;
	unsigned int hdr_mode;
	unsigned int hdr_long_offset;
	unsigned int hdr_short1_offset;
	unsigned int hdr_short2_offset;
	unsigned int hdr_short3_offset;
	unsigned int pixel_size;
	unsigned int dual_gain_mode;
	unsigned int line_time;
	unsigned int vb_time;
	unsigned int sht0_max;
	unsigned int sht1_max;
	unsigned int sht2_max;
};

struct vindev_dgain_ratio {
	unsigned int vsrc_id;
	unsigned int r_ratio;
	unsigned int gr_ratio;
	unsigned int gb_ratio;
	unsigned int b_ratio;
};

struct vindev_chip_status {
	unsigned int vsrc_id;
	int temperature;
};

struct vindev_wdr_gp_s {
	unsigned int vsrc_id;
	unsigned int l;
	unsigned int s1;
	unsigned int s2;
	unsigned int s3;
};

struct vindev_wdr_gp_info {
	unsigned int vsrc_id;
	struct vindev_wdr_gp_s shutter_gp;
	struct vindev_wdr_gp_s again_gp;
	struct vindev_wdr_gp_s dgain_gp;
};


/* ==========================================================================*/
#define VINIOC_MAGIC				's'
#define VIN_IO(nr)				_IO(VINIOC_MAGIC, nr)
#define VIN_IOR(nr, size)			_IOR(VINIOC_MAGIC, nr, size)
#define VIN_IOW(nr, size)			_IOW(VINIOC_MAGIC, nr, size)
#define VIN_IOWR(nr, size)			_IOWR(VINIOC_MAGIC, nr, size)

enum {
	IOC_VIN_DEVINFO					= 0x01,
	IOC_VIN_MODE						= 0x03,
	IOC_VIN_FPS						= 0x04,
	IOC_VIN_VIDEOINFO					= 0x05,
	IOC_VIN_SHUTTER					= 0x07,
	IOC_VIN_AGC						= 0x08,
	IOC_VIN_AGC_SHUTTER				= 0x09,
	IOC_VIN_DGAIN_RATIO				= 0x0A,
	IOC_VIN_MIRROR					= 0x0B,
	IOC_VIN_EISINFO					= 0x10,
	IOC_VIN_REG						= 0x11,
	IOC_VIN_AGC_INDEX					= 0x12,
	IOC_VIN_SHUTTER2ROW				= 0x13,
	IOC_VIN_SHUTTER_ROW				= 0x14,
	IOC_VIN_WDR_AGAIN_INDEX			= 0x15,
	IOC_VIN_WDR_AGAIN_INDEX_GP		= 0x16,
	IOC_VIN_WDR_DGAIN_INDEX_GP		= 0x17,
	IOC_VIN_WDR_SHUTTER_ROW_GP		= 0x18,
	IOC_VIN_WDR_SHUTTER2ROW		= 0x19,
	IOC_VIN_AAAINFO					= 0x1A,
	IOC_VIN_CHIP_STATUS				= 0x1B,
	IOC_VIN_WDR_ROW_GP				= 0x1C
};

/* ==========================================================================*/
#define IAV_IOC_VIN_GET_DEVINFO				VIN_IOR(IOC_VIN_DEVINFO, struct vindev_devinfo)
#define IAV_IOC_VIN_SET_MODE					VIN_IOW(IOC_VIN_MODE, struct vindev_mode)
#define IAV_IOC_VIN_GET_MODE					VIN_IOR(IOC_VIN_MODE, struct vindev_mode)
#define IAV_IOC_VIN_SET_FPS						VIN_IOW(IOC_VIN_FPS, struct vindev_fps)
#define IAV_IOC_VIN_GET_FPS					VIN_IOR(IOC_VIN_FPS, struct vindev_fps)
#define IAV_IOC_VIN_GET_VIDEOINFO				VIN_IOR(IOC_VIN_VIDEOINFO, struct vindev_video_info)
#define IAV_IOC_VIN_SET_SHUTTER				VIN_IOW(IOC_VIN_SHUTTER, struct vindev_shutter)
#define IAV_IOC_VIN_GET_SHUTTER				VIN_IOR(IOC_VIN_SHUTTER, struct vindev_shutter)
#define IAV_IOC_VIN_SET_AGC					VIN_IOW(IOC_VIN_AGC, struct vindev_agc)
#define IAV_IOC_VIN_GET_AGC					VIN_IOR(IOC_VIN_AGC, struct vindev_agc)
#define IAV_IOC_VIN_SET_MIRROR				VIN_IOW(IOC_VIN_MIRROR, struct vindev_mirror)
#define IAV_IOC_VIN_GET_MIRROR				VIN_IOR(IOC_VIN_MIRROR, struct vindev_mirror)
#define IAV_IOC_VIN_GET_AAAINFO				VIN_IOR(IOC_VIN_AAAINFO, struct vindev_aaa_info)
#define IAV_IOC_VIN_SET_AGC_SHUTTER			VIN_IOW(IOC_VIN_AGC_SHUTTER, struct vindev_agc_shutter)
#define IAV_IOC_VIN_SET_DGAIN_RATIO			VIN_IOW(IOC_VIN_DGAIN_RATIO, struct vindev_dgain_ratio)
#define IAV_IOC_VIN_GET_DGAIN_RATIO			VIN_IOR(IOC_VIN_DGAIN_RATIO, struct vindev_dgain_ratio)

#define IAV_IOC_VIN_GET_EISINFO				VIN_IOR(IOC_VIN_EISINFO, struct vindev_eisinfo)
#define IAV_IOC_VIN_SET_REG					VIN_IOW(IOC_VIN_REG, struct vindev_reg)
#define IAV_IOC_VIN_GET_REG					VIN_IOR(IOC_VIN_REG, struct vindev_reg)
#define IAV_IOC_VIN_GET_CHIP_STATUS			VIN_IOR(IOC_VIN_CHIP_STATUS, struct vindev_chip_status)

#define IAV_IOC_VIN_SET_AGC_INDEX				VIN_IOW(IOC_VIN_AGC_INDEX, struct vindev_agc_idx)
#define IAV_IOC_VIN_SHUTTER2ROW				VIN_IOWR(IOC_VIN_SHUTTER2ROW, struct vindev_shutter_row)
#define IAV_IOC_VIN_SET_SHUTTER_ROW			VIN_IOW(IOC_VIN_SHUTTER_ROW, struct vindev_shutter_row)
#define IAV_IOC_VIN_GET_WDR_SHUTTER_ROW_GP	VIN_IOR(IOC_VIN_WDR_SHUTTER_ROW_GP, struct vindev_wdr_gp_s)
#define IAV_IOC_VIN_GET_WDR_AGAIN_INDEX_GP	VIN_IOR(IOC_VIN_WDR_AGAIN_INDEX_GP, struct vindev_wdr_gp_s)
#define IAV_IOC_VIN_GET_WDR_DGAIN_INDEX_GP	VIN_IOR(IOC_VIN_WDR_DGAIN_INDEX_GP, struct vindev_wdr_gp_s)
#define IAV_IOC_VIN_WDR_SHUTTER2ROW			VIN_IOWR(IOC_VIN_WDR_SHUTTER2ROW, struct vindev_wdr_gp_s)
#define IAV_IOC_VIN_SET_WDR_GP				VIN_IOW(IOC_VIN_WDR_ROW_GP, struct vindev_wdr_gp_info)

/* ==========================================================================*/

#endif	//__IAV_VIN_IOCTL_H__

