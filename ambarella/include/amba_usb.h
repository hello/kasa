/*
 * amba_usb.h
 *
 * History:
 *	2008/7/10 - [Cao Rongrong] created file
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


#ifndef   TEST_USB_STREAM_H
#define   TEST_USB_STREAM_H

#define READ_SIZE		32768
#define WRITE_SIZE		32768
#define SIMPLE_CMD_SIZE	32

/* TOKEN */
#define AMB_COMMAND_TOKEN		0x55434D44
#define AMB_STATUS_TOKEN		0x55525350

#define USB_CMD_TRY_TO_CONN	0
#define USB_CMD_RDY_TO_RCV		1
#define USB_CMD_RCV_DATA		2
#define USB_CMD_RDY_TO_SND		3
#define USB_CMD_SND_DATA		4
#define USB_CMD_SET_MODE		5
#define USB_CMD_RECV_REQUEST	6


#define AMB_RSP_SUCCESS		0
#define AMB_RSP_FAILED		1

#define AMB_CMD_PARA_UP		0
#define AMB_CMD_PARA_DOWN		1
#define AMB_CMD_PARA_MEASURE_SPD		2

#define	AMB_RSP_NO_CONN		0
#define AMB_RSP_CONNECT		1

#define NR_PORT			32
#define ALL_PORT			0xffff

#define PORT_STATUS_CHANGE		0x55
#define PORT_NO_CONNECT		0
#define PORT_CONNECT			1
#define PORT_FREE_ALL			2

#define REQUEST_HOST_CONNECT	0xaa
#define HOST_NO_CONNECT		0
#define HOST_CONNECT			1

#define FLAG_LAST_TRANS		0x01
#define FLAG_FORCE_FINISH		0x10

#define AMB_DATA_STREAM_MAGIC	'u'
#define AMB_DATA_STREAM_WR_RSP	_IOW(AMB_DATA_STREAM_MAGIC, 1, struct amb_usb_rsp *)
#define AMB_DATA_STREAM_RD_CMD	_IOR(AMB_DATA_STREAM_MAGIC, 1, struct amb_usb_cmd *)
#define AMB_DATA_STREAM_STATUS_CHANGE	_IOW(AMB_DATA_STREAM_MAGIC, 2, struct amb_usb_notify *)

struct amb_usb_cmd {
	u32 signature;
	u32 command;
	u32 parameter[(SIMPLE_CMD_SIZE / sizeof(u32)) - 2];
};

struct amb_usb_rsp {
	u32 signature;
	u32 response;
	u32 parameter0;
	u32 parameter1;
};

struct amb_usb_notify {
	u16	bNotifyType;
	u16	port_id;
	u16	value;
	u16	status;
};

struct amb_usb_head {
	u32 port_id;
	u32 size;
	u32 flag1;
	u32 flag2;
};

#define USB_PORT_IDLE	0x0
#define USB_PORT_OPEN	0x1
#define USB_PORT_CLOSED	0x2

#define USB_HEAD_SIZE		sizeof(struct amb_usb_head)
struct amb_usb_buf {
	struct amb_usb_head head;
	char buffer[0];
};

enum RX_FSM {
	COMMAND_PHASE = 0,
	RCV_FILENAME_PHASE,
	RCV_DATA_PHASE,
	SND_FILENAME_PHASE,
	SND_DATA_PHASE,
	MEASURE_SPEED_PHASE,
};


#endif /* end   */

