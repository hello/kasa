/*
 * iav_netlink.h
 *
 * History:
 *	2014/07/25 - [Zhaoyang Chen] created file
 *	2015/07/24 - [Jian Tang] modified file
 *
 * Copyright (C) 2014-2018, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
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

