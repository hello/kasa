/*
 * dsp.h
 *
 * History:
 *	2010/06/21 - [Zhenwu Xue] created file
 *
 * Copyright (C) 2007-2016, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#ifndef __DSP_H__
#define __DSP_H__

#include <linux/semaphore.h>
#include <linux/sched.h>
#include <mach/init.h>
#include <iav_utils.h>
#include <iav_config.h>
#include "amba_arch_mem.h"
#include "dsp_cmd_msg.h"
#include "dsp_api.h"


#define MAX_NUM_CMD			31
#define MAX_DEFAULT_CMD			31
#define MAX_NUM_ENC_CMD		16
#define DSP_CMD_LIST_NUM	4

#define MAX_CMD_SIZE			(MAX_NUM_CMD * DSP_CMD_SIZE)
#define MAX_DEFAULT_CMD_SIZE		(MAX_DEFAULT_CMD * DSP_CMD_SIZE)

/* There is no message header in S2L, so max num message is 16. */
#define MAX_NUM_MSG			(DSP_MSG_BUF_SIZE / DSP_MSG_SIZE)

struct dsp_cmd_array {
	struct list_head	cmd_list;
	u8			update_cmd_seq_num;
	u8			num_cmds;
	u8			num_enc_cmds;
	u8			reserved;
};

struct dsp_cmd_port {
	DSP_CMD			*cmd_queue;
	DSP_MSG			*msg_queue;
	DSP_MSG			*current_msg;
	u32			total_cmd_size;
	u32			total_msg_size;
	u32			prev_cmd_seq_num;
	u32			cmd_list_idx;
	struct dsp_cmd_array cmd_array[DSP_CMD_LIST_NUM];
};

struct dsp_audit_info {
	u32		id;
	u32		enable : 1;
	u32		reserved : 31;
	u64		cnt;
	u64		sum;
	u32		max;
	u32		min;
};

struct ambarella_dsp {
	struct dsp_device	dsp_dev;

	/* Init data related */
	dsp_init_data_t		*dsp_init_data;
	dsp_init_data_t		*dsp_init_data_pm;
	void			*default_cmd_virt;
	dma_addr_t		default_cmd_phys;
	u32			total_default_cmd_size;

	/* DSP command related */
	struct dsp_cmd_port	gen_cmd_port;

	u64			vcap_counter;
	u64			irq_counter;

	/* Internal usage bit flags */
	u32			idsp_reset_flag : 1;
	u32			reserved : 31;

	/* DSP mode related */
	dsp_op_mode_t		op_mode;
	u32			probe_op_mode;
	u32			enc_mode;
//	u32			encode_state;	/* Not used yet */
	u32			*chip_id;
	vdsp_info_t		*vdsp_info;

	/* Audit related */
	struct dsp_audit_info	audit[DAI_NUM];

	spinlock_t		lock;
	wait_queue_head_t	op_wq;
	wait_queue_head_t	sub_wq;
	wait_queue_head_t	vcap_wq;

	struct list_head	free_list;
};

static inline struct ambarella_dsp *to_ambarella_dsp(struct dsp_device *dsp_dev)
{
	return container_of(dsp_dev, struct ambarella_dsp, dsp_dev);
}

int dsp_init_data(struct ambarella_dsp *dsp);

extern int ucode_dev_init(struct ambarella_dsp *dsp);
extern void ucode_dev_exit(void);
extern void dsp_print_cmd(void * cmd);
extern int dsp_update_log(void);
extern int dsp_set_op_mode(struct dsp_device *dsp_dev,
		u32 op_mode, struct amb_dsp_cmd *dsp_cmd);
extern int dsp_set_enc_sub_mode(struct dsp_device *dsp_dev,
		u32 enc_mode, struct amb_dsp_cmd *first, u8 force);
extern void dsp_release_cmd(struct dsp_device *dsp_dev, struct amb_dsp_cmd *first);
extern struct amb_dsp_cmd *dsp_get_cmd(struct dsp_device *dsp_dev, u32 flag);
extern struct amb_dsp_cmd *dsp_get_multi_cmds(struct dsp_device *dsp_dev,
		int num, u32 flag);
extern void dsp_put_cmd(struct dsp_device *dsp_dev, struct amb_dsp_cmd *dsp_cmd, u32 delay);
extern int dsp_get_chip_id(struct dsp_device *dsp_dev, u32 *id, u32 *chip);
extern int dsp_wait_vcap(struct dsp_device *dsp_dev, u32 count);
extern dsp_init_data_t *dsp_get_init_data(struct dsp_device *dsp_dev);
extern int dsp_set_audit(struct dsp_device *dsp_dev, u32 id, u32 audit_addr);
extern int dsp_get_audit(struct dsp_device *dsp_dev, u32 id, u32 audit_addr);
extern int bopt_sync(void *arg0, void *arg1, void *arg2, void *arg3);
extern int dsp_suspend(struct dsp_device *dsp_dev);
extern int dsp_resume(struct dsp_device *dsp_dev);

#endif

