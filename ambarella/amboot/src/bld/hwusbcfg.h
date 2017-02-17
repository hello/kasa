/**
 * bld/usb/hwusbcfg.h
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


#ifndef __HWUSBCFG_H__
#define __HWUSBCFG_H__
	  
//-------------------------------------
// USB endpoints definitions
//-------------------------------------
#define USB_LEP_CNT		1	// count of logical endpoints
#define USB_LEP_CTRL		0	// logical endpoint 0 - control
#define USB_LEP_BULKOUT		1	// logical endpoint 1 - bulk out
#define USB_LEP_BULKIN		1	// logical endpoint 1 - bulk in
#define USB_LEP_INTROUT		2	// logical endpoint 2 - interrupt out
#define USB_LEP_INTRIN		2	// logical endpoint 2 - interrupt in

#define USB_EP_CNT		4	// count of endpoints 
#define USB_EP_CTRLOUT		0	// EP 0 - control out
#define USB_EP_CTRLIN		1	// EP 1 - control in
#define USB_EP_BULKOUT		2	// EP 2 - bulk out
#define USB_EP_BULKIN		3	// EP 3 - bulk in
#define USB_EP_INTROUT		4	// EP 4 - interrupt out
#define USB_EP_INTRIN		5	// EP 5 - interrupt in

#define USB_EP_IN						0
#define USB_EP_OUT						1

//-------------------------------------
// USB register fields
//-------------------------------------
//-----------------
// for device specific registers
//-----------------

// for USB_DEV_EP_INTR_REG
#define USB_DEV_CTRLOUT		0x00010000		// 0 (R/WC)
#define USB_DEV_BULKOUT		0x00020000		// 0 (R/WC)
#define USB_DEV_INTROUT		0x00040000		// 0 (R/WC)
#define USB_DEV_CTRLIN		0x00000001		// 0 (R/WC)
#define USB_DEV_BULKIN		0x00000002		// 0 (R/WC)
#define USB_DEV_INTRIN		0x00000004		// 0 (R/WC)

#define USB_DEV_EP_OUT		(USB_DEV_CTRLOUT | \
				 USB_DEV_BULKOUT | \
				 USB_DEV_INTROUT)
#define USB_DEV_EP_IN		(USB_DEV_CTRLIN | \
				 USB_DEV_BULKIN | \
				 USB_DEV_INTRIN)


// for USB_DEV_EP_INTR_MSK_REG
#define USB_DEV_MSK_CTRLOUT	0x00010000		// 0 (R/WC)
#define USB_DEV_MSK_BULKOUT	0x00020000		// 0 (R/WC)
#define USB_DEV_MSK_CTRLIN	0x00000001		// 0 (R/WC)
#define USB_DEV_MSK_BULKIN	0x00000002		// 0 (R/WC)
//#define USB_DEV_MSK_INTRIN	0x00000004		// 0 (R/WC)

// for USB_UDC_EP_INFO 
#define USB_UDC_CFG_NUM		0x00000080
#define USB_UDC_INTF_NUM	0x00000000
#define USB_UDC_ALT_SET		0x00000000

#endif // __HWUSBCFG_H__
