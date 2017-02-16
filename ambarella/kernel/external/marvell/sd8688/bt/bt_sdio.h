/** @file bt_sdio.h
 *  @brief This file contains SDIO (interface) module
 *  related macros, enum, and structure.
 *       
 *  Copyright (C) 2007-2008, Marvell International Ltd.
 *
 *  This software file (the "File") is distributed by Marvell International 
 *  Ltd. under the terms of the GNU General Public License Version 2, June 1991 
 *  (the "License").  You may use, redistribute and/or modify this File in 
 *  accordance with the terms and conditions of the License, a copy of which 
 *  is available along with the File in the gpl.txt file or by writing to 
 *  the Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 
 *  02111-1307 or on the worldwide web at http://www.gnu.org/licenses/gpl.txt.
 *
 *  THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE 
 *  IMPLIED WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE 
 *  ARE EXPRESSLY DISCLAIMED.  The License provides additional details about 
 *  this warranty disclaimer.
 *
 */

#ifndef _BT_SDIO_H_
#define _BT_SDIO_H_

/** IRQ return type */
typedef irqreturn_t IRQ_RET_TYPE;
/** IRQ return */
#define IRQ_RET		return IRQ_HANDLED
/** ISR notifier function */
typedef IRQ_RET_TYPE(*isr_notifier_fn_t) (s32 irq, void *dev_id,
                                          struct pt_regs * reg);

/** SDIO header length */
#define SDIO_HEADER_LEN			4

/* SD block size can not bigger than 64 due to buf size limit in firmware */
/** define SD block size for data Tx/Rx */
#define SD_BLOCK_SIZE			64
/** define SD block size for firmware download */
#define SD_BLOCK_SIZE_FW_DL		256

/** Number of blocks for firmware transfer */
#define FIRMWARE_TRANSFER_NBLOCK	2

#ifndef MAX
/** Return maximum of two */
#define MAX(a,b)		((a) > (b) ? (a) : (b))
#endif

/** This is for firmware specific length */
#define EXTRA_LEN	36

/** Command buffer size for Marvell driver */
#define MRVDRV_SIZE_OF_CMD_BUFFER       (2 * 1024)

/** Bluetooth Rx packet buffer size for Marvell driver */
#define MRVDRV_BT_RX_PACKET_BUFFER_SIZE \
	(HCI_MAX_FRAME_SIZE + EXTRA_LEN)

/** Buffer size to allocate */
#define ALLOC_BUF_SIZE		(((MAX(MRVDRV_BT_RX_PACKET_BUFFER_SIZE, \
					MRVDRV_SIZE_OF_CMD_BUFFER) + SDIO_HEADER_LEN \
					+ SD_BLOCK_SIZE - 1) / SD_BLOCK_SIZE) * SD_BLOCK_SIZE)

/** The number of times to try when polling for status bits */
#define MAX_POLL_TRIES			100

/** The number of times to try when waiting for downloaded firmware to 
     become active. (polling the scratch register). */

#define MAX_FIRMWARE_POLL_TRIES		100

/** HIM disable */
#define	HIM_DISABLE			0xff
/** HIM enable */
#define HIM_ENABLE			0x03
/** Firmware ready */
#define FIRMWARE_READY			0xfedc

/* Bus Interface Control Reg 0x07 */
/** SD BUS width 1 */
#define SD_BUS_WIDTH_1			0x00
/** SD BUS width 4 */
#define SD_BUS_WIDTH_4			0x02
/** SD BUS width mask */
#define SD_BUS_WIDTH_MASK		0x03
/** Asynchronous interrupt mode */
#define ASYNC_INT_MODE			0x20

/* Host Control Registers */
/** I/O port 0 register */
#define IO_PORT_0_REG			0x00
/** I/O port 1 register */
#define IO_PORT_1_REG			0x01
/** I/O port 2 register */
#define IO_PORT_2_REG			0x02
/** Configuration register */
#define CONFIGURATION_REG		0x03
/** Host without Command 53 finish host */
#define HOST_WO_CMD53_FINISH_HOST	(0x1U << 2)
/** Host power up */
#define HOST_POWER_UP			(0x1U << 1)
/** Host power down */
#define HOST_POWER_DOWN			(0x1U << 0)
/** Host interrupt mask register */
#define HOST_INT_MASK_REG		0x04

/* Card Control Registers */
/** SQ read base address of A0 register */
#define SQ_READ_BASE_ADDRESS_A0_REG  	0x10
/** SQ read base address of A1 register */
#define SQ_READ_BASE_ADDRESS_A1_REG  	0x11

/** Host interrupt status register */
#define HOST_INTSTATUS_REG		0x05
/** Upload host interrupt status */
#define UP_LD_HOST_INT_STATUS		(0x1U)
/** Download host interrupt status */
#define DN_LD_HOST_INT_STATUS		(0x2U)

/** Card status register */
#define CARD_STATUS_REG              	0x20
/** Card I/O ready */
#define CARD_IO_READY              	(0x1U << 3)
/** CIS card ready */
#define CIS_CARD_RDY                 	(0x1U << 2)
/** Upload card ready */
#define UP_LD_CARD_RDY               	(0x1U << 1)
/** Download card ready */
#define DN_LD_CARD_RDY               	(0x1U << 0)

/** Card OCR 0 register */
#define CARD_OCR_0_REG               	0x34
/** Card OCR 1 register */
#define CARD_OCR_1_REG               	0x35

/** Card Control Registers */
#define CARD_REVISION_REG            	0x3c

/** Firmware status 0 register */
#define CARD_FW_STATUS0_REG		0x40
/** Firmware status 1 register */
#define CARD_FW_STATUS1_REG		0x41
/** Rx length register */
#define CARD_RX_LEN_REG			0x42
/** Rx unit register */
#define CARD_RX_UNIT_REG		0x43
struct sdio_mmc_card
{
        /** sdio_func structure pointer */
    struct sdio_func *func;
        /** bt_private structure pointer */
    bt_private *priv;
};
#endif /* _BT_SDIO_H_ */
