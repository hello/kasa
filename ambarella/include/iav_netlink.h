/*
 * iav_netlink.h
 *
 * History:
 *	2014/07/25 - [Zhaoyang Chen] created file
 *	2015/07/24 - [Jian Tang] modified file
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

#ifndef ___IAV_NETLINK_H__
#define ___IAV_NETLINK_H__


#define MAX_NETLINK_REQUESTS	4

#define NL_PORT_IMAGE			20
#define NL_PORT_VSYNC			21

#define MAX_NL_MSG_LEN			1024

enum NL_OBJ_INDEX {
	NL_OBJ_IMAGE = 0,
	NL_OBJ_VSYNC,
	NL_OBJ_MAX_NUM,
};

enum NL_REQUEST_IMAGE {
	NL_REQ_IMG_START_AAA = 0,
	NL_REQ_IMG_STOP_AAA,
	NL_REQ_IMG_PREPARE_AAA,
	NL_REQ_IMG_FIRST = NL_REQ_IMG_START_AAA,
	NL_REQ_IMG_LAST = NL_REQ_IMG_PREPARE_AAA + 1,
	NL_REQ_IMG_NUM = NL_REQ_IMG_LAST - NL_REQ_IMG_FIRST,

};

enum NL_REQUEST_VSYNC {
	NL_REQ_VSYNC_RESTORE = 0,
	NL_REQ_VSYNC_FIRST = NL_REQ_VSYNC_RESTORE,
	NL_REQ_VSYNC_LAST = NL_REQ_VSYNC_RESTORE + 1,
	NL_REQ_VSYNC_NUM = NL_REQ_VSYNC_LAST - NL_REQ_VSYNC_FIRST,
};

enum NL_SESSION_CMD {
	NL_SESS_CMD_CONNECT = 0,
	NL_SESS_CMD_DISCONNECT,
	NL_SESS_CMD_FIRST = NL_SESS_CMD_CONNECT,
	NL_SESS_CMD_LAST = NL_SESS_CMD_DISCONNECT + 1,
	NL_SESS_CMD_NUM = NL_SESS_CMD_LAST - NL_SESS_CMD_FIRST,
};

enum NL_CMD_STATUS {
	NL_CMD_STATUS_SUCCESS = 0,
	NL_CMD_STATUS_FAIL,
};

enum NL_MSG_TYPE {
	NL_MSG_TYPE_SESSION = 0,
	NL_MSG_TYPE_REQUEST,
};

enum NL_MSG_DIR {
	NL_MSG_DIR_CMD = 0,
	NL_MSG_DIR_STATUS,
};

struct nl_msg_data {
	u32 pid;
	u32 port;
	u32 type;
	u32 dir;
	u32 cmd;
	u32 status;
};

#endif

