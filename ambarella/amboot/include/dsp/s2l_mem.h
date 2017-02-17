/**
 * amboot/include/dsp/s2l_mem.h
 * should be consistent with kernel/private/include/arch_s2l/amba_arch_mem.h
 *
 * Author: Cao Rongrong <rrcao@ambarella.com>
 *
 * Copyright (c) 2016 Ambarella, Inc.
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

#ifndef __AMBA_S2L_MEM_ARCH_H__
#define __AMBA_S2L_MEM_ARCH_H__
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

#include "config.h"

#define PAGE_SHIFT	12
#define PAGE_SIZE	(1UL << PAGE_SHIFT)
#define PAGE_MASK	(~(PAGE_SIZE - 1))
#define PAGE_ALIGN(size)	(((size) + PAGE_SIZE - 1) & PAGE_MASK)
#define ALIGN(size, align) ((size + align - 1) & (~(align - 1)))

#define	IAV_MAX_ENCODE_STREAMS_NUM	4

typedef enum {
	DSP_ENC_CMD_SIZE = (128),
	ENC_CMD_TOGGLED_NUM = (8),
	CMD_SYNC_SIZE = (DSP_ENC_CMD_SIZE * IAV_MAX_ENCODE_STREAMS_NUM),
	CMD_SYNC_TOTAL_SIZE = (CMD_SYNC_SIZE * ENC_CMD_TOGGLED_NUM),
} IAV_CMD_SYNC_PARAMS;

/*
 * QP matrix memory configuration
 */
typedef enum {
#ifdef IAV_MEM_QPM_SIZE
	SINGLE_QP_MATRIX_SIZE = PAGE_ALIGN(IAV_MEM_QPM_SIZE),
#else
	SINGLE_QP_MATRIX_SIZE = 0,
#endif
#ifndef CONFIG_AMBARELLA_IAV_ROI_IPB
	STREAM_QP_MATRIX_NUM = (1),
#else
	STREAM_QP_MATRIX_NUM = (3),
#endif
	STREAM_QP_MATRIX_SIZE = (SINGLE_QP_MATRIX_SIZE * STREAM_QP_MATRIX_NUM),
	QP_MATRIX_SIZE = (STREAM_QP_MATRIX_SIZE * IAV_MAX_ENCODE_STREAMS_NUM),
	QP_MATRIX_TOGGLED_NUM = (4),
	/* 96KB * 4 * (1 + 4) = 1920 KB for 1 qp matrix per stream*/
	/* 96KB * 3 * 4 * (1 + 4) = 5760 KB for 3 qp matrixes per stream*/
	QP_MATRIX_TOTAL_SIZE = (QP_MATRIX_SIZE * (1 + QP_MATRIX_TOGGLED_NUM)),
}IAV_QPMATRIX_PARAMS;

/*
 * WARP configuration
 */
typedef enum {
	VWARP_BLOCK_HEIGHT_MAX = 64,
	VWARP_BLOCK_HEIGHT_MAX_LDC = 32,
	VWARP_BLOCK_HEIGHT_MIN = 28,
	LDC_PADDING_WIDTH_MAX = 256,

	WARP_TABLE_AREA_MAX_NUM = 8,
	WARP_TABLE_AREA_MAX_WIDTH = 32,
	WARP_TABLE_AREA_MAX_HEIGHT = 48,
	WARP_TABLE_AREA_MAX_SIZE = (WARP_TABLE_AREA_MAX_WIDTH
	    * WARP_TABLE_AREA_MAX_HEIGHT),
	/* User Warp Partition */
	UWARP_NUM = 1,
	WARP_CMD_SIZE = 128,
	WARP_VECT_PART_SIZE = (WARP_TABLE_AREA_MAX_NUM * \
		WARP_TABLE_AREA_MAX_SIZE * sizeof(s16) * 3),  // 3 tables per one area(H-warp, V-warp, ME1-Vwarp)
	IAV_PARTITION_TOGGLE_NUM = 4,
	WARP_VECT_TOTAL_SIZE = WARP_VECT_PART_SIZE * (IAV_PARTITION_TOGGLE_NUM + UWARP_NUM),
	WARP_BATCH_PART_SIZE = WARP_TABLE_AREA_MAX_NUM * WARP_CMD_SIZE,
	WARP_BATCH_CMDS_OFFSET = WARP_VECT_TOTAL_SIZE,
	WARP_BATCH_CMDS_SIZE = WARP_BATCH_PART_SIZE * IAV_PARTITION_TOGGLE_NUM,

	/* 8 * 3KB * 3 * (4 + 1) + 4 = 364 KB */
	WARP_BUFFER_TOTAL_SIZE = WARP_VECT_TOTAL_SIZE + WARP_BATCH_CMDS_SIZE,
} IAV_WARP_PARAMS;

typedef enum {
	JPEG_QT_SIZE = 128,
	JPEG_QT_BUFFER_NUM = 4,
	JPEG_QT_TOTAL_SIZE = (IAV_MAX_ENCODE_STREAMS_NUM * JPEG_QT_SIZE * JPEG_QT_BUFFER_NUM),
} IAV_JPEG_QT_PARAMS;

/*
 * Privacymask memory configuration
 */
typedef enum {
	/*
	 * BPC memory configuration
	 */
	PM_BPC_WIDTH_MAX = 3072,
	PM_BPC_HEIGHT_MAX = 2048,
	PM_BPC_PARTITION_SIZE = ALIGN(PM_BPC_WIDTH_MAX / 8, 32) * PM_BPC_HEIGHT_MAX,
} IAV_PM_PARAMS;

#define DSP_USR_SIZE		PAGE_ALIGN(IAV_MEM_USR_SIZE)
#define DSP_MV_SIZE			PAGE_ALIGN(IAV_MEM_MV_SIZE)
#define DSP_OVERLAY_SIZE	PAGE_ALIGN(IAV_MEM_OVERLAY_SIZE)
#define DSP_QPM_SIZE		PAGE_ALIGN(QP_MATRIX_TOTAL_SIZE)
#ifndef CONFIG_AMBARELLA_IAV_DRAM_WARP_MEM
	#define DSP_WARP_SIZE	0
#else
	#define DSP_WARP_SIZE	PAGE_ALIGN(WARP_BUFFER_TOTAL_SIZE)
#endif
#define DSP_QUANT_SIZE		PAGE_ALIGN(JPEG_QT_TOTAL_SIZE)
#define DSP_PM_SIZE			PAGE_ALIGN(IAV_MEM_PM_SIZE_S2L)
#define DSP_BPC_SIZE		(PAGE_SIZE + PAGE_ALIGN(PM_BPC_PARTITION_SIZE))
#define DSP_CMD_SYNC_SIZE	PAGE_ALIGN(CMD_SYNC_TOTAL_SIZE)
#define DSP_VCA_SIZE		PAGE_ALIGN(IAV_MEM_VCA_SIZE)

#ifdef CONFIG_AMBARELLA_IAV_ROI_IPB
	#define IAV_MARGIN_SIZE		((18 << 20) + DSP_USR_SIZE + DSP_MV_SIZE + DSP_VCA_SIZE)
#else
	#define IAV_MARGIN_SIZE		((11 << 20) + DSP_USR_SIZE + DSP_MV_SIZE + DSP_VCA_SIZE)
#endif

#define DSP_IMGRSVD_SIZE		(5 << 20)
#if (DSP_IAVRSVD_SIZE < (DSP_IMGRSVD_SIZE + IAV_MARGIN_SIZE))
	#undef DSP_IAVRSVD_SIZE
	#define DSP_IAVRSVD_SIZE	(DSP_IMGRSVD_SIZE + IAV_MARGIN_SIZE)
#endif

/*
 * Note:
 * DSP_IMGPROC_OFFSET include the following parts, which in total are 2508 KB.
 * [USR]	0 KB
 * [MV]	0 KB
 * [OVERLAY]	2048 KB
 * [QPMATRIX]	384 KB
 * [WARP]	72 KB
 * [QUANT]	4 KB
 */
#define DSP_UCODE_SIZE			(4 << 20)

#ifdef CONFIG_AMBARELLA_DSP_LOG_SIZE
#define DSP_LOG_SIZE 	CONFIG_AMBARELLA_DSP_LOG_SIZE
#else
#define DSP_LOG_SIZE			(128 << 10)
#endif

#define DSP_BSH_SIZE			(16 << 10)
#define DSP_MSG_BUF_SIZE		(4 << 10)	/* MSG size is 256 bytes */
#define DSP_CMD_BUF_SIZE		(4 << 10)	/* CMD size is 128 bytes */
#define DSP_DEF_CMD_BUF_SIZE	(4 << 10)	/* Default CMD size is 128 bytes */
#define DSP_IMAGE_CONFIG_SIZE		(4 << 20)
#define DSP_ISOCFG_RSVED_SIZE		(512 << 10)

#define DSP_BUFFER_SIZE			(DRAM_SIZE - (IDSP_RAM_START - DRAM_START_ADDR) - \
						DSP_UCODE_SIZE - \
						DSP_LOG_SIZE - \
						DSP_BSH_SIZE - \
						DSP_MSG_BUF_SIZE - \
						DSP_CMD_BUF_SIZE - \
						DSP_DEF_CMD_BUF_SIZE - \
						DSP_FASTDATA_SIZE - \
						DSP_FASTAUDIO_SIZE - \
						DSP_IAVRSVD_SIZE - \
						DSP_BSB_SIZE)

#define DSP_BSB_START			(IDSP_RAM_START)
#define DSP_IAVRSVD_START		(DSP_BSB_START + DSP_BSB_SIZE)
#define DSP_OVERLAY_START 		(DSP_IAVRSVD_START + DSP_USR_SIZE + DSP_MV_SIZE)
#define DSP_IMGPROC_START		(DSP_OVERLAY_START + DSP_OVERLAY_SIZE + DSP_QPM_SIZE + \
						DSP_WARP_SIZE + DSP_QUANT_SIZE)
#define DSP_IMAGE_CONFIG_START		(DSP_IMGPROC_START)
#define DSP_STILL_ISO_CONFIG_START	(DSP_IMAGE_CONFIG_START + DSP_IMAGE_CONFIG_SIZE)
#define DSP_FASTBOOT_IDSPCFG_START	(DSP_STILL_ISO_CONFIG_START + DSP_ISOCFG_RSVED_SIZE)
#define DSP_VCA_START			(DSP_IMGPROC_START + DSP_IMGRSVD_SIZE + DSP_PM_SIZE + \
						DSP_BPC_SIZE + DSP_CMD_SYNC_SIZE)
#define DSP_FASTDATA_START		(DSP_IAVRSVD_START + DSP_IAVRSVD_SIZE)
#define DSP_FASTAUDIO_START		(DSP_FASTDATA_START + DSP_FASTDATA_SIZE)
#define DSP_BUFFER_START		(DSP_FASTAUDIO_START + DSP_FASTAUDIO_SIZE)
#define DSP_DEF_CMD_BUF_START		(DSP_BUFFER_START + DSP_BUFFER_SIZE)
#define DSP_CMD_BUF_START		(DSP_DEF_CMD_BUF_START + DSP_DEF_CMD_BUF_SIZE)
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
 *       +----------------------+ <--- dsp init data
 *       | DSP INIT DATA (128 B)|
 *       +----------------------+
 *       | RESERVED      (4 B)  |
 *       +----------------------+
 */

#define UCODE_ORCCODE_START		(DSP_UCODE_START)
#define UCODE_DSP_INIT_DATA_PTR		(UCODE_ORCCODE_START + 0x30)
#define UCODE_ORCME_START		(UCODE_ORCCODE_START + (3 << 20))
#define UCODE_DEFAULT_BINARY_START	(UCODE_ORCME_START + (640 << 10))
#define UCODE_DEFAULT_MCTF_START	(UCODE_DEFAULT_BINARY_START + (320 << 10))
#define UCODE_CHIP_ID_START		(UCODE_DEFAULT_MCTF_START + (16 << 10))
#define DSP_INIT_DATA_START		(UCODE_CHIP_ID_START + 4 + (1 << 10))

/* layout for fastosd(overlay) in memory */
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
 *       | RESERVED  (1 KB - 260 B)|
 *       +-------------------------+ <--- FASTBOOT_USER_DATA_OFFSET
 *       | USER DATA        (3 KB) |
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

#endif
