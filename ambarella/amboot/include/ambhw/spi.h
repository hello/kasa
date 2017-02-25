/*
 * ambhw/spi.h
 *
 * History:
 *	2006/12/27 - [Charles Chiou] created file
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


#ifndef __AMBHW__SPI_H__
#define __AMBHW__SPI_H__

/* ==========================================================================*/
#if (CHIP_REV == A5S)
#define SPI_INSTANCES				2
#define SPI_AHB_INSTANCES			0
#define SPI_SLAVE_INSTANCES			1
#define SPI_AHB_SLAVE_INSTANCES			0
#elif (CHIP_REV == S2)
#define SPI_INSTANCES				1
#define SPI_AHB_INSTANCES			0
#define SPI_SLAVE_INSTANCES			1
#define SPI_AHB_SLAVE_INSTANCES			0
#elif (CHIP_REV == S2E)
#define SPI_INSTANCES				2
#define SPI_AHB_INSTANCES			0
#define SPI_SLAVE_INSTANCES			1
#define SPI_AHB_SLAVE_INSTANCES			0
#elif (CHIP_REV == S2L) || (CHIP_REV == S3) || (CHIP_REV == S3L)
#define SPI_INSTANCES				0
#define SPI_AHB_INSTANCES			2
#define SPI_SLAVE_INSTANCES			0
#define SPI_AHB_SLAVE_INSTANCES			1
#elif (CHIP_REV == A7L)
#define SPI_INSTANCES				1
#define SPI_AHB_INSTANCES			0
#define SPI_SLAVE_INSTANCES			1
#define SPI_AHB_SLAVE_INSTANCES			0
#else
#define SPI_INSTANCES				1
#define SPI_AHB_INSTANCES			0
#define SPI_SLAVE_INSTANCES			0
#define SPI_AHB_SLAVE_INSTANCES			0
#endif

#if (CHIP_REV == A5S)
#define SPI_SUPPORT_MASTER_CHANGE_ENA_POLARITY	0
#define SPI_SUPPORT_MASTER_DELAY_START_TIME	0
#define SPI_SUPPORT_NSM_SHAKE_START_BIT_CHSANGE	0
#else
#define SPI_SUPPORT_MASTER_CHANGE_ENA_POLARITY	1
#define SPI_SUPPORT_MASTER_DELAY_START_TIME	1 // S2L only support ssi0, remember to fix it
#define SPI_SUPPORT_NSM_SHAKE_START_BIT_CHSANGE	1 // S2 and S2L is not explain this in PRM
#endif

#if (CHIP_REV == S2L) || (CHIP_REV == S3) || (CHIP_REV == S3L)
#define SPI_TARGET_FRAME	1
#else
#define SPI_TARGET_FRAME	0
#endif

/* ==========================================================================*/
#if (SPI_INSTANCES >= 1)
#define SPI_OFFSET			0x2000
#define SPI_BASE			(APB_BASE + SPI_OFFSET)
#define SPI_REG(x)			(SPI_BASE + (x))
#endif

#if (SPI_SLAVE_INSTANCES >= 1)
#if (CHIP_REV == A7L)
#define SPI_SLAVE_OFFSET		0x1000
#else
#define SPI_SLAVE_OFFSET		0x1E000
#endif
#define SPI_SLAVE_BASE			(APB_BASE + SPI_SLAVE_OFFSET)
#define SPI_SLAVE_REG(x)		(SPI_SLAVE_BASE + (x))
#endif

#if (SPI_INSTANCES >= 2)
#define SPI2_OFFSET			0xF000
#define SPI2_BASE			(APB_BASE + SPI2_OFFSET)
#define SPI2_REG(x)			(SPI2_BASE + (x))
#endif

#if (SPI_INSTANCES >= 3)
#define SPI3_OFFSET			0x15000
#define SPI3_BASE			(APB_BASE + SPI3_OFFSET)
#define SPI3_REG(x)			(SPI3_BASE + (x))
#endif

#if (SPI_INSTANCES >= 4)
#define SPI4_OFFSET			0x16000
#define SPI4_BASE			(APB_BASE + SPI4_OFFSET)
#define SPI4_REG(x)			(SPI4_BASE + (x))
#endif

#if (SPI_AHB_INSTANCES == 1)
#define SSI_DMA_OFFSET			0xD000
#define SSI_DMA_BASE			(AHB_BASE + SSI_DMA_OFFSET)
#define SSI_DMA_REG(x)			(SSI_DMA_BASE + (x))
#endif

#if (SPI_AHB_INSTANCES == 2)
#define SPI_OFFSET			0x20000
#define SPI_BASE			(AHB_BASE + SPI_OFFSET)
#define SPI_REG(x)			(SPI_BASE + (x))

#define SPI2_OFFSET			0x21000
#define SPI2_BASE			(AHB_BASE + SPI2_OFFSET)
#define SPI2_REG(x)			(SPI2_BASE + (x))
#endif

#if (SPI_AHB_SLAVE_INSTANCES >= 1)
#define SPI_AHB_SLAVE_OFFSET		0x26000
#define SPI_AHB_SLAVE_BASE		(AHB_BASE + SPI_AHB_SLAVE_OFFSET)
#define SPI_AHB_SLAVE_REG(x)		(SPI_AHB_SLAVE_BASE + (x))
#endif

#define SPI_MASTER_INSTANCES		(SPI_INSTANCES + SPI_AHB_INSTANCES)


/* ==========================================================================*/
/* SPI_FIFO_SIZE */
#define SPI_DATA_FIFO_SIZE_16		0x10
#define SPI_DATA_FIFO_SIZE_32		0x20
#define SPI_DATA_FIFO_SIZE_64		0x40
#define SPI_DATA_FIFO_SIZE_128		0x80

/****************************************************/
/* Controller registers definitions                 */
/****************************************************/

#define SPI_CTRLR0_OFFSET		0x00
#define SPI_CTRLR1_OFFSET		0x04
#define SPI_SSIENR_OFFSET		0x08
#define SPI_MWCR_OFFSET			0x0c // no PRM explain it and no code use it. should be commented after check
#define SPI_SER_OFFSET			0x10
#define SPI_BAUDR_OFFSET		0x14
#define SPI_TXFTLR_OFFSET		0x18
#define SPI_RXFTLR_OFFSET		0x1c
#define SPI_TXFLR_OFFSET		0x20
#define SPI_RXFLR_OFFSET		0x24
#define SPI_SR_OFFSET			0x28
#define SPI_IMR_OFFSET			0x2c
#define SPI_ISR_OFFSET			0x30
#define SPI_RISR_OFFSET			0x34
#define SPI_TXOICR_OFFSET		0x38
#define SPI_RXOICR_OFFSET		0x3c
#define SPI_RXUICR_OFFSET		0x40
#define SPI_MSTICR_OFFSET		0x44
#define SPI_ICR_OFFSET			0x48
#define SPI_DMAC_OFFSET			0x4c
#define SPI_IDR_OFFSET			0x58
#define SPI_VERSION_ID_OFFSET		0x5c
#define SPI_DR_OFFSET			0x60

#if (SPI_SUPPORT_MASTER_CHANGE_ENA_POLARITY == 1)
#define SPI_SSIENPOLR_OFFSET		0x260
#endif
#if (SPI_SUPPORT_MASTER_DELAY_START_TIME == 1)
#define SPI_SCLK_OUT_DLY_OFFSET		0x264
#endif
#if (SPI_SUPPORT_NSM_SHAKE_START_BIT_CHSANGE == 1)
#define SPI_START_BIT_OFFSET		0x268
#endif

#if (SPI_TARGET_FRAME == 1)
#define SPI_TARGET_FRAME_COUNT	0x27c
#define SPI_FRAME_COUNT			0x280
#define SPI_FCRICR				0x284
#define SPI_TX_FRAME_COUNT		0x288
#endif
/* ==========================================================================*/
/* SPI rw mode */
#define SPI_WRITE_READ		0
#define SPI_WRITE_ONLY		1
#define SPI_READ_ONLY		2

/* Tx FIFO empty interrupt mask */
#define SPI_TXEIS_MASK		0x00000001
#define SPI_TXOIS_MASK 		0x00000002
#define SPI_RXFIS_MASK 		0x00000010
#define SPI_FCRIS_MASK 		0x00000100

/* SPI Parameters */
#define SPI_DUMMY_DATA		0xffff
#define MAX_QUERY_TIMES		10

/* Default SPI settings */
#define SPI_MODE		SPI_MODE_0
#define SPI_SCPOL		0
#define SPI_SCPH		0
#define SPI_FRF			0
#define SPI_CFS			0x0
#define SPI_DFS			0xf
#define SPI_BAUD_RATE		200000

/* ==========================================================================*/
#ifndef __ASM__
/* ==========================================================================*/

/* ==========================================================================*/
#endif
/* ==========================================================================*/

#endif

