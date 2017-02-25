/*
 * dsp_api.h
 *
 * History:
 *	2015/07/12 - [Jian Tang] created file
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

enum {
	DSP_CLK_TYPE_IDSP = 0,
	DSP_CLK_TYPE_HEVC = 1,

	DSP_CLK_TYPE_NUM,
	DSP_CLK_TYPE_FIRST = 0,
	DSP_CLK_TYPE_LAST = DSP_CLK_TYPE_NUM,
};

struct amb_dsp_cmd {
	DSP_CMD dsp_cmd;
	struct list_head head;
	struct list_head node;
	u32 cmd_type;
	u32 flag;
	u32 keep_latest :1;
	u32 reserved :31;
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
	u32 reserved;

	void (*msg_callback[NUM_MSG_CAT])(void *data, DSP_MSG *msg);
	void *msg_data[NUM_MSG_CAT];
	void (*enc_callback)(void *data, DSP_MSG *msg);
	void *enc_data;

	int (*set_op_mode)(struct dsp_device *dsp_dev, u32 op_mode,
		struct amb_dsp_cmd *cmd, u32 no_wait);
	int (*set_enc_sub_mode)(struct dsp_device *dsp_dev, u32 enc_mode,
		struct amb_dsp_cmd *cmd, u32 no_wait, u8 force);

	struct amb_dsp_cmd *(*get_cmd)(struct dsp_device *dsp_dev, u32 flag);
	struct amb_dsp_cmd *(*get_multi_cmds)(struct dsp_device *dsp_dev,
			int num, u32 flag);
	void (*put_cmd)(struct dsp_device *dsp_dev, struct amb_dsp_cmd *cmd, u32 delay);
	void (*print_cmd)(void *cmd);
	void (*release_cmd)(struct dsp_device *dsp_dev, struct amb_dsp_cmd *cmd);
	void (*set_vcap_port)(struct dsp_device *dsp_dev, u8 enable);
	int (*get_chip_id)(struct dsp_device *dsp_dev, u32 *dsp_chip_id, u32 *chip);
	int (*wait_vcap)(struct dsp_device *dsp_dev, u32 count);
	dsp_init_data_t *(*get_dsp_init_data)(struct dsp_device *dsp_dev);
	int (*set_audit)(struct dsp_device *dsp_dev, u32 cmd, unsigned long audit_addr);
	int (*get_audit)(struct dsp_device *dsp_dev, u32 cmd, unsigned long audit_addr);
	int (*suspend)(struct dsp_device *dsp_dev);
	int (*resume)(struct dsp_device *dsp_dev);
	int (*set_clock_state)(u32 clk_type, u32 enable);
 };

#endif	// _DSP_API_H

