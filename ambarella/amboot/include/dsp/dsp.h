/**
 * dsp.h
 *
 * Author: Cao Rongrong <rrcao@ambarella.com>
 *
 * Copyright (C) 2014-2016, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __IAV_DSP_H__
#define __IAV_DSP_H__

#if (CHIP_REV == S2L)
#include "s2l_cmd_msg.h"
#endif

/**
 *
 * DSP Buffer Layout:
 *
 *       +--------------------+ <--- IDSP_RAM_START
 *       | DSP_BSB_SIZE       |
 *       +--------------------+
 *       | DSP_IAVRSVD_SIZE   |
 *       +--------------------+
 *       | DSP_FASTDATA_SIZE  |
 *       +--------------------+
 *       | DSP_FASTAUDIO_SIZE |
 *       +--------------------+
 *       | DSP_BUFFER_SIZE    |
 *       +--------------------+
 *       | DSP_CMD_BUF_SIZE   |
 *       +--------------------+
 *       | DSP_MSG_BUF_SIZE   |
 *       +--------------------+
 *       | DSP_BSH_SIZE       |
 *       +--------------------+
 *       | DSP_LOG_SIZE       |
 *       +--------------------+
 *       | DSP_UCODE_SIZE     |
 *       +--------------------+
 *
 */

/*
 * Note:
 *   1. "start" means the first byte of physical address.
 *   2. "base" means the first byte of virtual address.
 *   3. DSP_BSB_SIZE ,DSP_IAVRSVD_SIZE, DSP_FASTAUDIO_SIZE are specified by menuconfig.
 *   4. DSP_IAVRSVD_SIZE is the size of memory reserved for IAV drivers.
 *   5. DSP_FASTDATA_SIZE is the size of memory reserved for Fastboot data.
 *      If FastData is disabled, DSP_FASTDATA_SIZE should be set zero, or not defined.
 *      It's used for store dsp_status/vin_video_format in amboot.
 *   6. DSP_FASTAUDIO_SIZE is the size of memory reserved for FastAudio.
 *      If FastAudio is disabled, DSP_FASTAUDIO_SIZE should be set zero, or not defined.
 *   7. DSP_BSH_SIZE is the size of memory for storing BIT_STREAM_HDR.
 */

/* Interface type sync from kernel/private/include/vin_sensors.h */
typedef enum {
	SENSOR_PARALLEL_LVCMOS = 0,
	SENSOR_SERIAL_LVDS = 1,
	SENSOR_PARALLEL_LVDS = 2,
	SENSOR_MIPI = 3,
} SENSOR_IF;

/* DSP status sync from include/iav_ioctl.h */
enum iav_state {
	IAV_STATE_IDLE = 0,
	IAV_STATE_PREVIEW = 1,
	IAV_STATE_ENCODING = 2,
	IAV_STATE_STILL_CAPTURE = 3,
	IAV_STATE_DECODING = 4,
	IAV_STATE_TRANSCODING = 5,
	IAV_STATE_DUPLEX = 6,
	IAV_STATE_EXITING_PREVIEW = 7,
	IAV_STATE_INIT = 0xFF,
};

#define DSP_UCODE_SIZE			(4 << 20)
#define DSP_LOG_SIZE			(128 << 10)
#define DSP_BSH_SIZE			(16 << 10)
#define DSP_MSG_BUF_SIZE		(4 << 10)	/* MSG size is 256 bytes */
#define DSP_CMD_BUF_SIZE		(4 << 10)	/* CMD size is 128 bytes */

#define DSP_BUFFER_SIZE			(DRAM_SIZE - (IDSP_RAM_START - DRAM_START_ADDR) - \
						DSP_UCODE_SIZE - \
						DSP_LOG_SIZE - \
						DSP_BSH_SIZE - \
						DSP_MSG_BUF_SIZE - \
						DSP_CMD_BUF_SIZE - \
						DSP_FASTDATA_SIZE - \
						DSP_FASTAUDIO_SIZE - \
						DSP_IAVRSVD_SIZE - \
						DSP_BSB_SIZE)

#define DSP_BSB_START			(IDSP_RAM_START)
#define DSP_IAVRSVD_START		(DSP_BSB_START + DSP_BSB_SIZE)
#define DSP_FASTDATA_START		(DSP_IAVRSVD_START + DSP_IAVRSVD_SIZE)
#define DSP_FASTAUDIO_START		(DSP_FASTDATA_START + DSP_FASTDATA_SIZE)
#define DSP_BUFFER_START		(DSP_FASTAUDIO_START + DSP_FASTAUDIO_SIZE)
#define DSP_CMD_BUF_START		(DSP_BUFFER_START + DSP_BUFFER_SIZE)
#define DSP_MSG_BUF_START		(DSP_CMD_BUF_START + DSP_CMD_BUF_SIZE)
#define DSP_BSH_START			(DSP_MSG_BUF_START + DSP_MSG_BUF_SIZE)
#define DSP_LOG_START			(DSP_BSH_START + DSP_BSH_SIZE)
#define DSP_UCODE_START			(DSP_LOG_START + DSP_LOG_SIZE)

/*
 * layout for ucode in memory:
 *
 *       +----------------------+ <--- DSP_UCODE_START
 *       | ORCCODE       (3 MB) |
 *       +----------------------+
 *       | ORCME       (640 KB) |
 *       +----------------------+
 *       | DEFAULT BIN (320 KB) |
 *       +----------------------+ <--- Optional for fast boot
 *       | DEFAULT MCTF (16 KB) |
 *       +----------------------+ <--- Chip ID
 *       | CHIP ID        (4 B) |
 *       +----------------------+ <--- vdsp_info structure
 *       | VDSP_INFO BIN (1 KB) |
 *       +----------------------+
 */

#define UCODE_ORCCODE_START		(DSP_UCODE_START)
#define UCODE_ORCME_START		(UCODE_ORCCODE_START + (3 << 20))
#define UCODE_DEFAULT_BINARY_START	(UCODE_ORCME_START + (640 << 10))
#define UCODE_DEFAULT_MCTF_START	(UCODE_DEFAULT_BINARY_START + (320 << 10))
#define UCODE_CHIP_ID_START		(UCODE_DEFAULT_MCTF_START + (16 << 10))

#define DSP_INIT_DATA_START		(DRAM_START_ADDR + 0x000F0000)

/* layout for fastosd(overlay) in memory */
#define DSP_OVERLAY_START 		(DSP_BSB_START + DSP_BSB_SIZE)
/* 0x1000 = 4 KB */
#define DSP_OVERLAY_FONT_INDEX_START	(DSP_OVERLAY_START + 0)
#define DSP_OVERLAY_FONT_MAP_START	(DSP_OVERLAY_FONT_INDEX_START + 0x1000)
#define DSP_OVERLAY_CLUT_START		(DSP_OVERLAY_FONT_MAP_START + 0x4000)
#define DSP_OVERLAY_STRING_START	(DSP_OVERLAY_CLUT_START + 0x1000)
#define DSP_OVERLAY_STRING_OUT_START	(DSP_OVERLAY_STRING_START + 0x1000)

/*
 * layout for DSP_FASTDATA in memory:
 *
 *       +-------------------------+ <--- DSP_FASTDATA_START
 *       | DSP_STATUS       (4 B)  |
 *       +-------------------------+ <--- vin_video_format structure
 *       | VIN_VIDEO_FORMAT (128 B)|
 *       +-------------------------+ <--- vin_dsp_config structure
 *       | VIN_DSP_CONFIG  (128 B) |
 *       +-------------------------+
 */

/* store DSP_FASTDATA in amboot, restore it after enter Linux IAV */
#define DSP_STATUS_STORE_SIZE	(4)
#define DSP_VIN_VIDEO_FORMAT_STORE_SIZE	(128)
#define DSP_STATUS_STORE_START		(DSP_FASTDATA_START)
#define DSP_VIN_VIDEO_FORMAT_STORE_START	(DSP_STATUS_STORE_START + DSP_STATUS_STORE_SIZE)
#define DSP_VIN_CONFIG_STORE_START	(DSP_VIN_VIDEO_FORMAT_STORE_START + DSP_VIN_VIDEO_FORMAT_STORE_SIZE)
#define DSP_FASTDATA_INVALID		(0xFF)

#define AUDIO_PLAY_MAX_SIZE			(0x80000)

/* ==========================================================================*/

/* Used for dump dsp cmd with Hex format */
#define AMBOOT_IAV_DEBUG_DSP_CMD
#undef AMBOOT_IAV_DEBUG_DSP_CMD

#ifdef AMBOOT_IAV_DEBUG_DSP_CMD
#define dsp_print_cmd(cmd, cmd_size) 	\
	do { 					\
		int _i_; 				\
		int _j_;				\
		u32 *pcmd_raw = (u32 *)cmd; \
		putstr(#cmd);		\
		putstr("[");			\
		putdec(cmd_size);	\
		putstr("] = {\r\n");	\
		for (_i_ = 0, _j_= 1 ; _i_ < ((cmd_size + 3) >> 2); _i_++, _j_++) {	\
			putstr("0x");			\
			puthex(pcmd_raw[_i_]);\
			putstr(", ");			\
			if (_j_== 4) {			\
				putstr("\r\n");	\
				_j_ = 0;		\
			}				\
		} 					\
		putstr("\r\n}\r\n");	\
	} while (0)
#else
#define dsp_print_cmd(cmd, cmd_size)
#endif

/* ==========================================================================*/

#define UCODE_FILE_NAME_SIZE		(24)
#define DSPFW_IMG_MAGIC			(0x43525231)
#define MAX_UCODE_FILE_NUM		(8)
#define MAX_LOGO_FILE_NUM		(4)
#define SPLASH_CLUT_SIZE		(256)

struct ucode_file_info { /* 4 + 4 + 24 = 32 */
	unsigned int			offset;
	unsigned int			size;
	char				name[UCODE_FILE_NAME_SIZE];
}__attribute__((packed));

struct splash_file_info { /* 32 */
	unsigned int			offset;
	unsigned int			size;
	short				width;
	short				height;
	char				rev[32- (4 * 2) - (2 * 2)];
}__attribute__((packed));

struct dspfw_header { /* 4 + 4 + 4 + 4 + (32 * 8) + (32 * 4) + 112 = 512 */
	unsigned int			magic;
	unsigned int			size; /* totally size including the header */
	unsigned short			total_dsp;
	unsigned short			total_logo;
	struct ucode_file_info		ucode[MAX_UCODE_FILE_NUM];
	struct splash_file_info		logo[MAX_LOGO_FILE_NUM];
	unsigned char			rev[112];
}__attribute__((packed));

/* ==========================================================================*/
extern void vin_phy_init(int interface_type);
extern int dsp_init(void);
extern int dsp_boot(void);
extern int add_dsp_cmd(void *cmd, unsigned int size);

extern void audio_set_play_size(unsigned int addr, unsigned int size);
extern void audio_init(void);
extern void audio_start(void);

extern int dsp_get_ucode_by_name(struct dspfw_header *hdr, const char *name,
		unsigned int *addr, unsigned int *size);

#endif

