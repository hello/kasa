/*
 * dsp_api.h
 *
 * History:
 *	2015/07/12 - [Jian Tang] created file
 *
 * Copyright (C) 2015-2018, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#ifndef _DSP_API_H
#define _DSP_API_H

#include <linux/list.h>
#include <dsp_cmd_msg.h>

#define USE_INSTANT_CMD	(0)

enum {
	DSP_CMD_FLAG_NORMAL = 0,
	DSP_CMD_FLAG_HIGH_PRIOR = 1,
	DSP_CMD_FLAG_BLOCK = 2,
	DSP_CMD_FLAG_INSTANT = 3,
	DSP_CMD_FLAG_BATCH = 4,

	DSP_CMD_FLAG_NUM,
	DSP_CMD_FLAG_FIRST = 0,
	DSP_CMD_FLAG_LAST = DSP_CMD_FLAG_NUM,
};

enum {
	DSP_CMD_TYPE_NORMAL = 0,
	DSP_CMD_TYPE_VIN = 1,
	DSP_CMD_TYPE_VOUT = 2,
	DSP_CMD_TYPE_PREV = 3,
	DSP_CMD_TYPE_ENC = 4,
	DSP_CMD_TYPE_IMG = 5,
	DSP_CMD_TYPE_DEC = 6,

	DSP_CMD_TYPE_NUM,
	DSP_CMD_TYPE_FIRST = 0,
	DSP_CMD_TYPE_LAST = DSP_CMD_TYPE_NUM,
};

struct amb_dsp_cmd {
	DSP_CMD dsp_cmd;
	struct list_head head;
	struct list_head node;
	u32 cmd_type;
	u32 flag;
};

struct dsp_device *ambarella_request_dsp(void);

void dsp_issue_cmd(void *cmd, u32 size);
void dsp_issue_delay_cmd(void *cmd, u32 size, u32 delay);
void dsp_issue_img_cmd(void *cmd, u32 size);


struct dsp_device {
	char name[32];
	struct device *dev;

	u32 bsb_start;
	u32 buffer_start;
	u32 buffer_size;

	void (*msg_callback[NUM_MSG_CAT])(void *data, DSP_MSG *msg);
	void *msg_data[NUM_MSG_CAT];
	void (*enc_callback)(void *data, DSP_MSG *msg);
	void *enc_data;

	int (*set_op_mode)(struct dsp_device *dsp_dev,
			u32 op_mode, struct amb_dsp_cmd *cmd);
	int (*set_enc_sub_mode)(struct dsp_device *dsp_dev,
			u32 enc_mode, struct amb_dsp_cmd *cmd, u8 force);

	struct amb_dsp_cmd *(*get_cmd)(struct dsp_device *dsp_dev, u32 flag);
	struct amb_dsp_cmd *(*get_multi_cmds)(struct dsp_device *dsp_dev,
			int num, u32 flag);
	void (*put_cmd)(struct dsp_device *dsp_dev, struct amb_dsp_cmd *cmd, u32 delay);
	void (*release_cmd)(struct dsp_device *dsp_dev, struct amb_dsp_cmd *cmd);
	void (*set_vcap_port)(struct dsp_device *dsp_dev, u8 enable);
	int (*get_chip_id)(struct dsp_device *dsp_dev, u32 *dsp_chip_id, u32 *chip);
	int (*wait_vcap)(struct dsp_device *dsp_dev, u32 count);
	dsp_init_data_t *(*get_dsp_init_data)(struct dsp_device *dsp_dev);
	int (*set_audit)(struct dsp_device *dsp_dev, u32 cmd, u32 audit_addr);
	int (*get_audit)(struct dsp_device *dsp_dev, u32 cmd, u32 audit_addr);
	int (*suspend)(struct dsp_device *dsp_dev);
	int (*resume)(struct dsp_device *dsp_dev);
 };

#endif	// _DSP_API_H

