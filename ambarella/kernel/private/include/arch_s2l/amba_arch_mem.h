/*
 * amba_arch_mem.h
 *
 * History:
 *	2014/08/21 - [Jian Tang] created file
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


#ifndef __AMBA_ARCH_MEM_H__
#define __AMBA_ARCH_MEM_H__

#include <config.h>

/**
 * IAV / DSP MEMORY Layout:
 *      It's same as the DSP buffer layout in "/amboot/include/dsp.h".
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

/* IAV_MARGIN_SIZE is the memory margin size for IAV driver excluding IMG/BSB.
 * It is used to decide the reserved memory for IAV driver.
 * Normally below condition should be satisfied.
 * IAV_MARGIN_SIZE + IAV_DRAM_IMG + IAV_DRAM_BSB >= IAV_DRAM_MAX
 */
#ifdef CONFIG_AMBARELLA_IAV_DRAM_DECODE_ONLY

#define IAV_MARGIN_SIZE		0

#elif defined CONFIG_AMBARELLA_IAV_DRAM_VOUT_ONLY

#define IAV_MARGIN_SIZE		((6 << 20) + IAV_MEM_USR_SIZE)

#else

#ifdef CONFIG_AMBARELLA_IAV_ROI_IPB
#define IAV_MARGIN_SIZE		((18 << 20) + IAV_MEM_USR_SIZE + IAV_MEM_MV_SIZE + IAV_MEM_VCA_SIZE)
#else
#define IAV_MARGIN_SIZE		((11 << 20) + IAV_MEM_USR_SIZE + IAV_MEM_MV_SIZE + IAV_MEM_VCA_SIZE)
#endif

#endif

#ifndef CONFIG_AMBARELLA_IAV_DRAM_DECODE_ONLY

#ifdef CONFIG_IMGPROC_MEM_LARGE

#define DSP_IMGRSVD_SIZE		(64 << 20)
#if (DSP_IAVRSVD_SIZE < (DSP_IMGRSVD_SIZE + IAV_MARGIN_SIZE))
#undef DSP_IAVRSVD_SIZE
#define DSP_IAVRSVD_SIZE	(DSP_IMGRSVD_SIZE + IAV_MARGIN_SIZE)
//#  error "Reserved IAV driver memory size must be larger than 69MB."
#endif

#else

#define DSP_IMGRSVD_SIZE		(5 << 20)
#if (DSP_IAVRSVD_SIZE < (DSP_IMGRSVD_SIZE + IAV_MARGIN_SIZE))
#undef DSP_IAVRSVD_SIZE
#define DSP_IAVRSVD_SIZE	(DSP_IMGRSVD_SIZE + IAV_MARGIN_SIZE)
//#  error "Reserved IAV driver memory size must be larger than 16MB."
#endif

#endif	// CONFIG_IMGPROC_MEM_LARGE

#else

#define DSP_IMGRSVD_SIZE		0
#undef DSP_IAVRSVD_SIZE
#define DSP_IAVRSVD_SIZE		0

#endif


#define DSP_UCODE_SIZE			(4 << 20)

#ifdef CONFIG_AMBARELLA_DSP_LOG_SIZE
#define DSP_LOG_SIZE 	CONFIG_AMBARELLA_DSP_LOG_SIZE
#else
#define DSP_LOG_SIZE			(128 << 10)
#endif

#define DSP_BSH_SIZE			(16 << 10)
#define DSP_MSG_BUF_SIZE		(4 << 10)	/* MSG size is 256 bytes, total is 16 MSG. */
#define DSP_CMD_BUF_SIZE		(4 << 10)	/* CMD size is 128 bytes, total is 31 CMD + header. */
#define DSP_DEF_CMD_BUF_SIZE	(4 << 10)	/* Default CMD size is 128 bytes, total is 31 CMD + header. */

#ifndef DSP_FASTDATA_SIZE
#define DSP_FASTDATA_SIZE	0
#endif

#ifndef DSP_FASTAUDIO_SIZE
#define DSP_FASTAUDIO_SIZE	0
#endif

#ifndef DSP_BSB_SIZE
#define DSP_BSB_SIZE	0
#endif

#define DSP_DRAM_SIZE		(get_ambarella_iavmem_size() - \
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

#define DSP_BSB_START			(get_ambarella_iavmem_phys())
#define DSP_IAVRSVD_START		(DSP_BSB_START + DSP_BSB_SIZE)
#define DSP_FASTDATA_START		(DSP_IAVRSVD_START + DSP_IAVRSVD_SIZE)
#define DSP_FASTAUDIO_START		(DSP_FASTDATA_START + DSP_FASTDATA_SIZE)
#define DSP_DRAM_START			(DSP_FASTAUDIO_START + DSP_FASTAUDIO_SIZE)
#define DSP_DEF_CMD_BUF_START		(DSP_DRAM_START + DSP_DRAM_SIZE)
#define DSP_CMD_BUF_START		(DSP_DEF_CMD_BUF_START + DSP_DEF_CMD_BUF_SIZE)
#define DSP_MSG_BUF_START		(DSP_CMD_BUF_START + DSP_CMD_BUF_SIZE)
#define DSP_BSH_START			(DSP_MSG_BUF_START + DSP_MSG_BUF_SIZE)
#define DSP_LOG_START			(DSP_BSH_START + DSP_BSH_SIZE)
#define DSP_UCODE_START			(DSP_LOG_START + DSP_LOG_SIZE)

#ifdef CONFIG_DSP_LOG_START_0X80000

#define DSP_LOG_ADDR_OFFSET		(0x80000)
#define DSP_LOG_AREA_PHYS		(get_ambarella_ppm_phys() + DSP_LOG_ADDR_OFFSET)

#else

#ifdef CONFIG_DSP_LOG_START_IAVMEM
#define DSP_LOG_AREA_PHYS		(DSP_LOG_START)
#endif	// CONFIG_DSP_LOG_START_IAVMEM

#endif	// CONFIG_DSP_LOG_START_0X80000

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
#define DSP_CODE_MEMORY_OFFSET		(0)
#define DSP_MEMD_MEMORY_OFFSET		(3 * MB)
#define DSP_BINARY_DATA_MEMORY_OFFSET	(DSP_MEMD_MEMORY_OFFSET + 640 * KB)
#define DSP_CHIP_ID_OFFSET			(DSP_BINARY_DATA_MEMORY_OFFSET + (320 + 16) * KB)
#define DSP_INIT_DATA_OFFSET		(DSP_CHIP_ID_OFFSET + 1 * KB + 4)
#define DSP_DRAM_CODE_START		(DSP_UCODE_START + DSP_CODE_MEMORY_OFFSET)
#define UCODE_DSP_INIT_DATA_PTR		(DSP_DRAM_CODE_START + 0x30)
#define DSP_DRAM_MEMD_START		(DSP_UCODE_START + DSP_MEMD_MEMORY_OFFSET)
#define DSP_BINARY_DATA_START		(DSP_UCODE_START + DSP_BINARY_DATA_MEMORY_OFFSET)
#define DSP_CHIP_ID_START			(DSP_UCODE_START + DSP_CHIP_ID_OFFSET)
#define DSP_INIT_DATA_START			(DSP_UCODE_START + DSP_INIT_DATA_OFFSET)

#define DSP_IDSP_BASE			(get_ambarella_apb_virt() + 0x11801C)
#define DSP_ORC_BASE			(get_ambarella_apb_virt() + 0x11FF00)

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

#endif	// __AMBA_ARCH_MEM_H__

