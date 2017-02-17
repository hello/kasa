/**
 * bld/usb/hwusbreg.h
 *
 * History:
 *    2005/09/07 - [Arthur Yang] created file
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


#ifndef __HWUSBREG_H__
#define __HWUSBREG_H__

/*----------------------------------------------
    macros
----------------------------------------------*/

//-------------------------------------
// USB global definitions
//-------------------------------------

//-------------------------------------
// USB RxFIFO and TxFIFO depth (single or multiple)
//-------------------------------------
#define USB_RXFIFO_DEPTH_CTRLOUT	(256 << 16)		// shared
#define USB_RXFIFO_DEPTH_BULKOUT	(256 << 16)		// shared
#define USB_RXFIFO_DEPTH_INTROUT	(256 << 16)		// shared
#define USB_TXFIFO_DEPTH_CTRLIN		(64 / 4)		// 16 32-bit
#define USB_TXFIFO_DEPTH_BULKIN		(512 / 4)		// 128 32-bit
#define USB_TXFIFO_DEPTH_INTRIN		(512 / 4)		// 128 32-bit

#define USB_TXFIFO_DEPTH		(64 / 4 + 4 * 512 / 4)	// 528 32-bit
#define USB_RXFIFO_DEPTH		(0x400)			// 256 32-bit

//-------------------------------------
// USB memory map
//-------------------------------------
#define USB_EP_IN_BASE		USBDC_BASE
#define USB_EP_OUT_BASE		(USBDC_BASE + 0x0200)
#define USB_DEV_BASE		(USBDC_BASE + 0x0400)
#define USB_UDC_BASE		(USBDC_BASE + 0x0504)
#define USB_RXFIFO_BASE		(USBDC_BASE + 0x0800)
#define USB_TXFIFO_BASE		(USB_RXFIFO_BASE + USB_RXFIFO_DEPTH)

//-------------------------------------
// USB register address
//-------------------------------------

#define	USB_EP_IN_CTRL_REG(n)	((int *)(USB_EP_IN_BASE + 0x0000 + 0x0020 * (n)))
#define	USB_EP_IN_STS_REG(n)	((int *)(USB_EP_IN_BASE + 0x0004 + 0x0020 * (n)))
#define	USB_EP_IN_BUF_SZ_REG(n)	((int *)(USB_EP_IN_BASE + 0x0008 + 0x0020 * (n)))
#define	USB_EP_IN_MAX_PKT_SZ_REG(n)	((int *)(USB_EP_IN_BASE + 0x000c + 0x0020 * (n)))
#define	USB_EP_IN_DAT_DESC_PTR_REG(n)	((int *)(USB_EP_IN_BASE + 0x0014 + 0x0020 * (n)))
#define USB_EP_IN_WR_CFM_REG		((int *)(USB_EP_IN_BASE + 0x001c + 0x0020 * (n)))

#define	USB_EP_OUT_CTRL_REG(n)	((int *)(USB_EP_OUT_BASE + 0x0000 + 0x0020 * (n)))
#define	USB_EP_OUT_STS_REG(n)	((int *)(USB_EP_OUT_BASE + 0x0004 + 0x0020 * (n)))
#define	USB_EP_OUT_PKT_FRM_NUM_REG(n)	((int *)(USB_EP_OUT_BASE + 0x0008 + 0x0020 * (n)))
#define	USB_EP_OUT_MAX_PKT_SZ_REG(n) 	((int *)(USB_EP_OUT_BASE + 0x000c + 0x0020 * (n)))
#define	USB_EP_OUT_SETUP_BUF_PTR_REG(n) ((int *)(USB_EP_OUT_BASE + 0x0010 + 0x0020 * (n)))
#define	USB_EP_OUT_DAT_DESC_PTR_REG(n)	((int *)(USB_EP_OUT_BASE + 0x0014 + 0x0020 * (n)))
#define USB_EP_OUT_RD_CFM_ZO_REG		((int *)(USB_EP_OUT_BASE + 0x001c + 0x0020 * (n)))	// for slave-only mode

#define USB_DEV_CFG_REG		((int *)(USB_DEV_BASE + 0x0000))
#define USB_DEV_CTRL_REG	((int *)(USB_DEV_BASE + 0x0004))
#define USB_DEV_STS_REG		((int *)(USB_DEV_BASE + 0x0008))
#define USB_DEV_INTR_REG	((int *)(USB_DEV_BASE + 0x000c))
#define USB_DEV_INTR_MSK_REG	((int *)(USB_DEV_BASE + 0x0010))
#define USB_DEV_EP_INTR_REG	((int *)(USB_DEV_BASE + 0x0014))
#define USB_DEV_EP_INTR_MSK_REG	((int *)(USB_DEV_BASE + 0x0018))
#define USB_DEV_TEST_MODE_REG	((int *)(USB_DEV_BASE + 0x001c))



#define USB_UDC_REG(n)		((int *)(USB_UDC_BASE + 0x0004 * (n)))

//-------------------------------------
// USB register fields
//-------------------------------------
//-----------------
// for endpoint specific registers
//-----------------
// for USB_EP_IN_CTRL_REG(n) and USB_EP_OUT_CTRL_REG(n)	// default
#define USB_EP_STALL		0x00000001		// 0 (RW)
#define USB_EP_FLUSH		0x00000002		// 0 (RW)
#define USB_EP_SNOOP		0x00000004		// 0 (RW) - for OUT EP only
#define USB_EP_POLL_DEMAND	0x00000008		// 0 (RW) - for IN EP only
#define USB_EP_TYPE_CTRL	0x00000000		// 00 (RW)
#define USB_EP_TYPE_ISO		0x00000010		// 00 (RW)
#define USB_EP_TYPE_BULK	0x00000020		// 00 (RW)
#define USB_EP_TYPE_INTR	0x00000030		// 00 (RW)
// NAK is set by UDC after SETUP and STALL
#define USB_EP_NAK_STS		0x00000040		// 0 (RO)
#define USB_EP_SET_NAK		0x00000080		// 0 (WO)
#define USB_EP_CLR_NAK		0x00000100		// 0 (WO)
// set by APP when APP is ready for DMA
// clear by UDC when end of each packet if USB_DEV_DESC_UPD is set
// clear by UDC when end of playload if USB_DEV_DESC_UPD is clear
#define USB_EP_RCV_RDY		0x00000200		// 0 (RW)(D)
// 01 for OUT and 10 for SETUP(OUT EP only)
#define USB_EP_OUT_PKT_MSK	0x00000030		// 00 (R/WC)
#define USB_EP_OUT_PKT		0x00000010
#define USB_EP_SETUP_PKT	0x00000020
#define USB_EP_IN_PKT		0x00000040		// 0 (R/WC)
// set by UDC when descriptor is not available
// clear by APP when interrupt is serviced
#define USB_EP_BUF_NOT_AVAIL	0x00000080		// 0 (R/WC)(D)
// set by UDC when errors happen during DMA
#define USB_EP_HOST_ERR		0x00000200		// 0 (R/WC)
// set by UDC when DMA is finished
// clear by APP when interrupt is serviced
#define USB_EP_TRN_DMA_CMPL	0x00000400		// 0 (R/WC)(D)
// MASK for Rx packet data
#define USB_EP_RX_PKT_SZ	0x007ff800		// bit mask (RW)
#define USB_EP_RCV_CLR_STALL	0x02000000		// 0 (R/WC) (DS)

// for USB_EP_IN_BUF_SZ_REG(n) and USB_EP_OUT_PKT_FRM_NUM_REG(n)
#define USB_EP_TXFIFO_DEPTH	0x0000ffff		// bit mask (RW)
#define USB_EP_FRM_NUM		0x0000ffff		// bit mask (RW)

// for USB_EP_IN_MAX_PKT_SZ_REG(n) and USB_EP_OUT_MAX_PKT_SZ_REG(n)
#define USB_EP_RXFIFO_DEPTH	0xffff0000		// bit mask (RW)
#define USB_EP_MAX_PKT_SZ	0x0000ffff		// bit mask (RW)

//-----------------
// for device specific registers
//-----------------
// for USB_DEV_CFG_REG
#define USB_DEV_SPD_HI		0x00000000		// 00 (RW) 30/60MHz
#define USB_DEV_SPD_FU		0x00000001		// 00 (RW)
#define USB_DEV_SPD_LO		0x00000002		// 00 (RW) 30MHz
#define USB_DEV_SPD_FU48	0x00000003		// 00 (RW) - PHY CLK = 48 MHz

#define USB_DEV_REMOTE_WAKEUP_EN	0x00000004	// 0 (RW)

#define USB_DEV_BUS_POWER	0x00000000		// 0 (RW)
#define USB_DEV_SELF_POWER	0x00000008		// 0 (RW)

#define USB_DEV_SYNC_FRM_EN	0x00000010		// 0 (RW)

#define USB_DEV_PHY_16BIT	0x00000000		// 0 (RW)
#define USB_DEV_PHY_8BIT	0x00000020		// 0 (RW)

#define USB_DEV_UTMI_DIR_UNI	0x00000000		// 0 (RW) - UDC20 reserved to 0
#define USB_DEV_UTMI_DIR_BI	0x00000040		// 0 (RW)

#define USB_DEV_STS_OUT_NONZERO	0x00000180		// bit mask (RW)

#define USB_DEV_PHY_ERR		0x00000200		// 0 (RW)

#define USB_DEV_SPD_FU_TIMEOUT	0x00001c00		// bit mask (RW)
#define USB_DEV_SPD_HI_TIMEOUT	0x0000e000		// bit mask (RW)
// AC clear_feature (ENDPOINT_HALT) for EP0
#define USB_DEV_HALT_ACK	0x00000000		// 0 (RW)
// STALL clear_feature (ENDPOINT_HALT) for EP0
#define USB_DEV_HALT_STALL	0x00010000		// 1 (RW)
// enable CSR_DONE of USB_DEV_CFG_REG
#define USB_DEV_CSR_PRG_EN	0x00020000		// 1 (RW)
// STALL set_descriptor
#define USB_DEV_SET_DESC_STALL	0x00000000		// 0 (RW)
// ACK Set_Descriptor
#define USB_DEV_SET_DESC_ACK	0x00040000		// 1 (RW)

#define USB_DEV_SDR		0x00000000		// 0 (RW)
#define USB_DEV_DDR		0x00080000		// 1 (RW)

// for USB_DEV_CTRL_REG
#define USB_DEV_REMOTE_WAKEUP	0x00000001		// 0 (RW)
#define USB_DEV_RCV_DMA_EN	0x00000004		// 0 (RW)(D)
#define USB_DEV_TRN_DMA_EN	0x00000008		// 0 (RW)(D)
#define USB_DEV_DESC_UPD_PYL	0x00000000		// 0 (RW)(D)
#define USB_DEV_DESC_UPD_PKT	0x00000010		// 0 (RW)(D)

#define USB_DEV_LITTLE_ENDN	0x00000000		// 0 (RW)(D)
#define USB_DEV_BIG_ENDN	0x00000020		// 0 (RW)(D)
// packet-per-buffer mode
#define USB_DEV_PKT_PER_BUF_MD	0x00000000		// 0 (RW)(D)
// buffer fill mode
#define USB_DEV_BUF_FIL_MD	0x00000040		// 0 (RW)(D)
//for OUT EP only
#define USB_DEV_THRESH_EN	0x00000080		// 0 (RW)(D)

#define USB_DEV_BURST_EN	0x00000100		// 0 (RW)(D)
#define USB_DEV_SLAVE_ONLY_MD	0x00000000		// 0 (RW)(D)
#define USB_DEV_DMA_MD		0x00000200		// 0 (RW)(D)
// signal UDC20 to disconnect
#define USB_DEV_SOFT_DISCON	0x00000400		// 0 (RW)
// for gate-level simulation only
#define USB_DEV_TIMER_SCALE_DOWN	0x00000800	// 0 (RW)
// device NAK (applied to all EPs)
#define USB_DEV_NAK		0x00001000		// 0 (RW)
//set to ACK Set_Configuration or Set_Interface if USB_DEV_CSR_PRG_EN
#define USB_DEV_CSR_DONE	0x00002000		// 0 (RW)
											//		    clear to NAK
#define USB_DEV_BURST_LEN	0x00070000		// bit mask (RW)
#define USB_DEV_THRESH_LEN	0x0f000000		// bit mask (RW)

// for USB_DEV_STS_REG
#define USB_DEV_CFG_NUM		0x0000000f		// bit mask (RO)
#define USB_DEV_INTF_NUM	0x000000f0		// bit mask (RO)
#define USB_DEV_ALT_SET		0x00000f00		// bit mask (RO)
#define USB_DEV_SUSP_STS	0x00001000		// bit mask (RO)
#define USB_DEV_ENUM_SPD	0x00006000		// bit mask (RO)
#define USB_DEV_ENUM_SPD_HI	0x00000000		// 00 (RO)
#define USB_DEV_ENUM_SPD_FU	0x00002000		// 00 (RO)
#define USB_DEV_ENUM_SPD_LO	0x00004000		// 00 (RO)
#define USB_DEV_ENUM_SPD_FU48	0x00006000		// 00 (RO)

#define USB_DEV_RXFIFO_EMPTY_STS	0x00008000	// bit mask (RO)
#define USB_DEV_PHY_ERR_STS	0x00010000		// bit mask (RO)
#define USB_DEV_FRM_NUM		0xfffc0000		// bit mask (RO)

// for USB_DEV_INTR_REG
#define USB_DEV_SET_CFG		0x00000001		// 0 (R/WC)
#define USB_DEV_SET_INTF	0x00000002		// 0 (R/WC)
#define USB_DEV_IDLE_3MS	0x00000004		// 0 (R/WC)
#define USB_DEV_RESET		0x00000008		// 0 (R/WC)
#define USB_DEV_SUSP		0x00000010		// 0 (R/WC)
#define USB_DEV_SOF		0x00000020		// 0 (R/WC)
#define USB_DEV_ENUM_CMPL	0x00000040		// 0 (R/WC)

// for USB_DEV_INTR_MSK_REG
#define USB_DEV_MSK_SET_CFG	0x00000001		// 0 (R/WC)
#define USB_DEV_MSK_SET_INTF	0x00000002		// 0 (R/WC)
#define USB_DEV_MSK_IDLE_3MS	0x00000004		// 0 (R/WC)
#define USB_DEV_MSK_RESET	0x00000008		// 0 (R/WC)
#define USB_DEV_MSK_SUSP	0x00000010		// 0 (R/WC)
#define USB_DEV_MSK_SOF		0x00000020		// 0 (R/WC)
#define USB_DEV_MSK_SPD_ENUM_CMPL	0x00000040	// 0 (R/WC)

// for USB_DEV_EP_INTR_REG
#define USB_DEV_EP0_IN		0x00000001		// 0 (R/WC)
#define USB_DEV_EP1_IN		0x00000002		// 0 (R/WC)
#define USB_DEV_EP2_IN		0x00000004		// 0 (R/WC)
#define USB_DEV_EP3_IN		0x00000008		// 0 (R/WC)
#define USB_DEV_EP4_IN		0x00000010		// 0 (R/WC)
#define USB_DEV_EP5_IN		0x00000020		// 0 (R/WC)

#define USB_DEV_EP0_OUT		0x00010000		// 0 (R/WC)
#define USB_DEV_EP1_OUT		0x00020000		// 0 (R/WC)
#define USB_DEV_EP2_OUT		0x00040000		// 0 (R/WC)
#define USB_DEV_EP3_OUT		0x00080000		// 0 (R/WC)
#define USB_DEV_EP4_OUT		0x00100000		// 0 (R/WC)
#define USB_DEV_EP5_OUT		0x00200000		// 0 (R/WC)

// for USB_DEV_EP_INTR_MSK_REG
#define USB_DEV_MSK_EP0_IN	0x00000001		// 0 (R/WC)
#define USB_DEV_MSK_EP1_IN	0x00000002		// 0 (R/WC)
#define USB_DEV_MSK_EP2_IN	0x00000004		// 0 (R/WC)
#define USB_DEV_MSK_EP3_IN	0x00000008		// 0 (R/WC)
#define USB_DEV_MSK_EP4_IN	0x00000010		// 0 (R/WC)
#define USB_DEV_MSK_EP5_IN	0x00000020		// 0 (R/WC)

#define USB_DEV_MSK_EP0_OUT	0x00010000		// 0 (R/WC)
#define USB_DEV_MSK_EP1_OUT	0x00020000		// 0 (R/WC)
#define USB_DEV_MSK_EP2_OUT	0x00040000		// 0 (R/WC)
#define USB_DEV_MSK_EP3_OUT	0x00080000		// 0 (R/WC)
#define USB_DEV_MSK_EP4_OUT	0x00100000		// 0 (R/WC)
#define USB_DEV_MSK_EP5_OUT	0x00200000		// 0 (R/WC)

// for USB_DEV_TEST_MODE_REG
#define USB_DEV_TEST_MD		0x00000001		// 0 (RW)

// for USB_UDC_REG
#define USB_UDC_EP0_NUM		0x00000000
#define USB_UDC_EP1_NUM		0x00000001
#define USB_UDC_EP2_NUM		0x00000002
#define USB_UDC_EP3_NUM		0x00000003
#define USB_UDC_EP4_NUM		0x00000004
#define USB_UDC_EP5_NUM		0x00000005

#define USB_UDC_OUT		0x00000000
#define USB_UDC_IN		0x00000010

#define USB_UDC_CTRL		0x00000000
#define USB_UDC_ISO		0x00000020
#define USB_UDC_BULK		0x00000040
#define USB_UDC_INTR		0x00000060

#define USB_EP_CTRL_MAX_PKT_SZ		64
#define USB_EP_BULK_MAX_PKT_SZ_HI	512
#define USB_EP_BULK_MAX_PKT_SZ_FU	64
#define USB_EP_INTR_MAX_PKT_SZ		512

#define USB_EP_CTRLIN_MAX_PKT_SZ	USB_EP_CTRL_MAX_PKT_SZ
#define USB_EP_CTRLOUT_MAX_PKT_SZ	USB_EP_CTRL_MAX_PKT_SZ
#define USB_EP_BULKIN_MAX_PKT_SZ_HI	USB_EP_BULK_MAX_PKT_SZ_HI
#define USB_EP_BULKOUT_MAX_PKT_SZ_HI	USB_EP_BULK_MAX_PKT_SZ_HI
#define USB_EP_BULKIN_MAX_PKT_SZ_FU	USB_EP_BULK_MAX_PKT_SZ_FU
#define USB_EP_BULKOUT_MAX_PKT_SZ_FU	USB_EP_BULK_MAX_PKT_SZ_FU
#define USB_EP_INTRIN_MAX_PKT_SZ	USB_EP_INTR_MAX_PKT_SZ
#define USB_EP_INTROUT_MAX_PKT_SZ	USB_EP_INTR_MAX_PKT_SZ

#define USB_UDC_MAX_PKT_SZ		(0x1fff << 19)
#define USB_UDC_CTRLIN_MAX_PKT_SZ	(USB_EP_CTRLIN_MAX_PKT_SZ << 19)
#define USB_UDC_CTRLOUT_MAX_PKT_SZ	(USB_EP_CTRLOUT_MAX_PKT_SZ << 19)
#define USB_UDC_BULKIN_MAX_PKT_SZ_HI	(USB_EP_BULKIN_MAX_PKT_SZ_HI << 19)
#define USB_UDC_BULKOUT_MAX_PKT_SZ_HI	(USB_EP_BULKOUT_MAX_PKT_SZ_HI << 19)
#define USB_UDC_BULKIN_MAX_PKT_SZ_FU	(USB_EP_BULKIN_MAX_PKT_SZ_FU << 19)
#define USB_UDC_BULKOUT_MAX_PKT_SZ_FU	(USB_EP_BULKOUT_MAX_PKT_SZ_FU << 19)
#define USB_UDC_INTRIN_MAX_PKT_SZ	(USB_EP_INTRIN_MAX_PKT_SZ << 19)
#define USB_UDC_INTROUT_MAX_PKT_SZ	(USB_EP_INTROUT_MAX_PKT_SZ << 19)

//-------------------------------------
// DMA status quadlet fields
//-------------------------------------
// IN / OUT descriptor specific
#define USB_DMA_RXTX_BYTES	0x0000ffff		// bit mask

// SETUP descriptor specific
#define USB_DMA_CFG_STS		0x0fff0000		// bit mask
#define USB_DMA_CFG_NUM		0x0f000000		// bit mask
#define USB_DMA_INTF_NUM	0x00f00000		// bit mask
#define USB_DMA_ALT_SET		0x000f0000		// bitmask
// ISO OUT descriptor only
#define USB_DMA_FRM_NUM		0x07ff0000		// bit mask
// IN / OUT descriptor specific
#define USB_DMA_LAST		0x08000000		// bit mask

#define USB_DMA_RXTX_STS	0x30000000		// bit mask
#define USB_DMA_RXTX_SUCC	0x00000000		// 00
#define USB_DMA_RXTX_DES_ERR	0x10000000		// 01
#define USB_DMA_RXTX_BUF_ERR	0x30000000		// 11

#define USB_DMA_BUF_STS 	0xc0000000		// bit mask
#define USB_DMA_BUF_HOST_RDY	0x00000000		// 00
#define USB_DMA_BUF_DMA_BUSY	0x40000000		// 01
#define USB_DMA_BUF_DMA_DONE	0x80000000		// 10
#define	USB_DMA_BUF_HOST_BUSY	0xc0000000		// 11




//-------------------------------------
// Define
//-------------------------------------
#define CTRL_IN		0
#define BULK_IN		1
#define CTRL_OUT	2
#define BULK_OUT	3
#define EP_NUM		(BULK_OUT + 1)

#define BIT(n)		(1 << n)


//-------------------------------------
// Structure definition
//-------------------------------------
// SETUP buffer
struct USB_SETUP_PKT {
	u32 status;
	u32 reserved;
	u32 data0;
	u32 data1;
};

// IN / OUT data descriptor
struct USB_DATA_DESC {
	u32 status;
	u32 reserved;
	u32 *data_ptr;
	u32 *next_desc_ptr;
};

struct USB_DEV_REG {
	u32 config;
	u32 control;
	u32 status;
	u32 intr;
	u32 intrmask;
	u32 ep_intr;
	u32 ep_intrmask;
	u32 testmode;
};

struct USB_EP_UDC_REG {
	u32 value;
};

struct USB_EP_IN_REG {
	u32 control;
	u32 status;
	u32 buf_size;
	u32 max_pkt_size;
	u32 reserved1;
	struct USB_DATA_DESC *descriptor_p;
	u32 reserved2;
	u32 wr_confirm;
};

struct USB_EP_OUT_REG {
	u32 control;
	u32 status;
	u32 pkt_frame_n;
	u32 max_pkt_size;
	u32 *setup_buffer_p;
	struct USB_DATA_DESC *descriptor_p;
	u32 rd_confirm;
};

// for usbd.tx_fifo_size/rx_fifo_size
#define TX_FIFO_SIZE 		(528*4)
#define RX_FIFO_SIZE		(256*4)
// for usbd.bulk_pkt_size/control_pkt_size
#define MAX_CTRL_PKT_SIZE64	64
#define MAX_BULK_PKT_SIZE64	64		// Full-Speed
#define MAX_BULK_PKT_SIZE512	512		// High-Speed


/* for usbd.link */
#define USB_DISCONNECT 		0
#define USB_CONNECT		1
/* for usbd.speed */
#define _SPEED_(x)		(x >> 13)
#define USB_HS			0	// Low-Speed
#define USB_FS			1	// Full-Speed
#define USB_LS			2   // High-Speed
#define USB_FS48		3
/* for usbd.mode */
#define MODE_SLAVE		0   // Slave-Mode
#define MODE_DMA		1   // DMA-Mode

/* TOKEN */
#define COMMAND_TOKEN	0x55434D44
#define STATUS_TOKEN	0x55525350

#define TX_DIR 0x10000
#define RX_DIR 0x80000
#define NO_DIR 0xC0000

#define USB_CMD_RDY_TO_RCV	0
#define USB_CMD_RCV_DATA	1
#define USB_CMD_RDY_TO_SND	2
#define USB_CMD_SND_DATA	3
#define USB_CMD_INQUIRY_STATUS	4

#define USB_RSP_SUCCESS		0
#define USB_RSP_FAILED		1
#define USB_RSP_IN_BLD		2

/* Ready-to-Recv commands */
#define USB_CMD_SET_MEMORY_READ		1
#define USB_CMD_SET_MEMORY_WRITE	2
/* add for communicating with the directUSB for writing the PTB */
#define USB_CMD_SET_FW_PTB		3

/* Ready to Send commands */
#define USB_CMD_GET_MEMORY	1
#define USB_CMD_GET_FIRMWARE	2
#define USB_CMD_GET_FW_BST	10
#define USB_CMD_GET_FW_BLD	11
#define USB_CMD_GET_FW_PRI	12
#define USB_CMD_GET_FW_RMD	13
#define USB_CMD_GET_FW_ROM	14
#define USB_CMD_GET_FW_DSP	15
#define USB_CMD_GET_FW_PTB	16

/* Inquiry types */
#define USB_BLD_INQUIRY_CHIP	0x00000001


typedef struct _COMMAND_QUEUE_ {
	u32 pendding;
	u32	pipe;
	u32	TX_RX;
	u8	*pBuf;
	u32	need_size;
	u32	done_size;
	u32	lock;
} COMMAND_QUEUE;


typedef struct _WAIT_QUEUE_ {
	u32 index;
	u32 TX_RX;	/* 1: TX_DIR, 2: RX_DIR */
	u32 flag;	/* 0: wait, 1: done */
	u32 lock;	/* 0: release, 1: locked */
} WAIT_QUEUE;

typedef struct _QUEUE_INDEX_ {
	u32 cmd_start;
	u32 cmd_end;
	u32 rsp_index;
	u32 lock;
	u32 status;
} QUEUE_INDEX;

typedef struct cmd_s {
	u32 signature;
	u32 command;
	u32 parameter0;
	u32 parameter1;
	u32 parameter2;
	u32 parameter3;
	u32 parameter4;
	u32 parameter5;
} HOST_CMD;

typedef struct rsp_s {
	u32 signature;
	u32 response;
	u32 parameter0;
	u32 parameter1;
} DEV_RSP;

enum RX_FSM {
	COMMAND_PHASE = 0,
	RCV_DATA_PHASE,
	SND_DATA_PHASE,
};

typedef struct
{
    	u16 usb_cable;
    	u16 usb_address;
    	u8  enumeration;
   	u16 device;
    	u16 interface;
    	u16 endpoint[EP_NUM];

} DEVICE_STATUS;

typedef struct
{
    	u8 	logical;  /* logical endpoint number  */
    	u8 	physical; /* physical endpoint number */
} EP_ID;

typedef struct
{
    	EP_ID  	id;            /* endpoint id       */
    	u8  	transfer_type; /* USB transfer type */
    	u32 	maxpkt_size;   /* maxpacket size    */
    	u32		fifo_size;
} EP_INFO;

struct USBDEV {
	u32	base;
	struct USB_DEV_REG	*dev_reg;	/* device base register */
	void	*ep[EP_NUM];			/* endpoint base register */
	u32	ep_descriptor[EP_NUM];	/* descriptor address */
	u32	ep_buffer[EP_NUM];		/* packet buffer address */

	u32	*setup_buffer;

	EP_INFO	ep_info[EP_NUM];

	u32	ctrl_tx_cnt;
	u32	ctrl_rx_cnt;
	u32	bulk_tx_cnt;
	u32	bulk_rx_cnt;
	u32	error_cnt;
	u32	bulk_pkt_size;
	u32	control_pkt_size;
	u32	mode;		/* DMA or slave */
	u32	link;		/* connected or disconnected */
	u32	tx_fifo_size;
	u32	rx_fifo_size;
	u32	speed;
	u32	intr_num;

	DEVICE_STATUS status; /* feature set by host */

	u32	address;	/* address set by host */
	u32	setup[2];
};





#endif // __HWUSBREG_H__
