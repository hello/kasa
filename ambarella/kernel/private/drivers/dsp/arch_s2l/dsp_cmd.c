/*
 * dsp_api.c
 *
 * History:
 *	2010/09/07 - [Zhenwu Xue] Ported from a5s
 *	2011/07/25 - [Louis Sun] ported from iOne for A7 IPCAM
 *
 * Copyright (C) 2007-2016, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#include <linux/module.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <plat/ambcache.h>
#include <plat/rct.h>
#include "dsp.h"

#define MEMD_OFFSET			0x150000
#define CODE_OFFSET			0x160000
#define MEMD_BASE			(DBGBUS_BASE + MEMD_OFFSET)
#define CODE_BASE			(DBGBUS_BASE + CODE_OFFSET)

#define DSP_DRAM_MAIN_OFFSET		0x0008
#define DSP_DRAM_SUB0_OFFSET		0x0008
#define DSP_DRAM_SUB1_OFFSET		0x8008
#define DSP_CONFIG_MAIN_OFFSET		0x0000
#define DSP_CONFIG_SUB0_OFFSET		0x0000
#define DSP_CONFIG_SUB1_OFFSET		0x8000

#define DSP_DRAM_MAIN_REG		(CODE_BASE + DSP_DRAM_MAIN_OFFSET) /* CODE */
#define DSP_DRAM_SUB0_REG		(MEMD_BASE + DSP_DRAM_SUB0_OFFSET) /* ME */
#define DSP_DRAM_SUB1_REG		(MEMD_BASE + DSP_DRAM_SUB1_OFFSET) /* MD */
#define DSP_CONFIG_MAIN_REG		(CODE_BASE + DSP_CONFIG_MAIN_OFFSET)
#define DSP_CONFIG_SUB0_REG		(MEMD_BASE + DSP_CONFIG_SUB0_OFFSET)
#define DSP_CONFIG_SUB1_REG		(MEMD_BASE + DSP_CONFIG_SUB1_OFFSET)

static inline struct amb_dsp_cmd *__alloc_cmd(struct ambarella_dsp *dsp)
{
	struct amb_dsp_cmd *cmd = NULL;
	unsigned long flags;

	spin_lock_irqsave(&dsp->lock, flags);
	if (!list_empty(&dsp->free_list)) {
		cmd = list_first_entry(&dsp->free_list, struct amb_dsp_cmd, node);
		list_del_init(&cmd->node);
	}
	spin_unlock_irqrestore(&dsp->lock, flags);

	/* no more cmd available in free list: create one more */
	if (!cmd) {
		cmd = kmalloc(sizeof(struct amb_dsp_cmd), GFP_ATOMIC);
		if (!cmd) {
			iav_error("No memory for dsp cmd!\n");
			return NULL;
		}
	}

	INIT_LIST_HEAD(&cmd->head);
	INIT_LIST_HEAD(&cmd->node);
	memset(&cmd->dsp_cmd, 0, sizeof(DSP_CMD));

	return cmd;
}

#if USE_INSTANT_CMD
static inline void __put_cmd(struct ambarella_dsp * dsp,
	struct dsp_cmd_port *port, struct amb_dsp_cmd *first,
	u32 sub_num)
{
	u8 * ptr = NULL;
	DSP_HEADER_CMD *hdr;
	struct amb_dsp_cmd *cmd, *_cmd;

	switch (first->flag) {
	case DSP_CMD_FLAG_HIGH_PRIOR:
		/* Insert the cmd list in the beginning of prior list */
		cmd = NULL;
		list_for_each_entry(cmd, &port->cmd_list, node) {
			if (cmd->flag != DSP_CMD_FLAG_HIGH_PRIOR)
				break;
		}
		if (cmd) {
			/* insert "first" before "cmd" */
			list_add_tail(&first->node, &cmd->node);
		} else {
			list_add_tail(&first->node, &port->cmd_list);
		}
		break;
	default:
		/* Insert the instant cmd into cmd buffer directly */
		ptr = (u8*)port->cmd_queue + sizeof(DSP_HEADER_CMD) +
			port->num_cmds * DSP_CMD_SIZE;
		memcpy(ptr, &first->dsp_cmd, DSP_CMD_SIZE);
		if (sub_num > 1) {
			list_for_each_entry_safe(cmd, _cmd, &first->head, node) {
				ptr += DSP_CMD_SIZE;
				memcpy(ptr, &cmd->dsp_cmd, DSP_CMD_SIZE);
			}
		}
		ptr = (u8*)port->cmd_queue + sizeof(DSP_HEADER_CMD) +
			port->num_cmds * DSP_CMD_SIZE;
		hdr = (DSP_HEADER_CMD *)port->cmd_queue;
		if (port->update_cmd_seq_num) {
			hdr->cmd_seq_num++;
			port->update_cmd_seq_num = 0;
		}
		port->num_cmds += sub_num;
		port->num_enc_cmds += sub_enc_num;
		hdr->num_cmds = port->num_cmds;

		/* Return the cmd list to free list */
		list_move(&first->node, &dsp->free_list);
		list_splice(&first->head, &dsp->free_list);
		break;
	}
}
#else
static inline void __put_cmd(struct ambarella_dsp * dsp,
	struct dsp_cmd_port *port, struct amb_dsp_cmd *first,
	u32 sub_num, u32 delay)
{
	struct amb_dsp_cmd *cmd;
	struct dsp_cmd_array *cmd_array;
	u32 idx = (port->cmd_list_idx + delay) % DSP_CMD_LIST_NUM;

	cmd_array = &port->cmd_array[idx];
	switch (first->flag) {
	case DSP_CMD_FLAG_HIGH_PRIOR:
		/* Insert the cmd list in the beginning of prior list */
		cmd = NULL;
		list_for_each_entry(cmd, &cmd_array->cmd_list, node) {
			if (cmd->flag != DSP_CMD_FLAG_HIGH_PRIOR)
				break;
		}
		if (cmd) {
			/* Insert "first" before "cmd" */
			list_add_tail(&first->node, &cmd->node);
		} else {
			list_add_tail(&first->node, &cmd_array->cmd_list);
		}
		break;
	default:
		list_add_tail(&first->node, &cmd_array->cmd_list);
		break;
	}
}
#endif	// #if USE_INSTANT_CMD

static DSP_CMD *__get_default_cmd(struct ambarella_dsp *dsp)
{
	DSP_HEADER_CMD *hdr;
	DSP_CMD *cmd_buffer;

	spin_lock_irq(&dsp->lock);
	hdr = (DSP_HEADER_CMD *)dsp->default_cmd_virt;
	hdr->num_cmds++;
	BUG_ON(hdr->num_cmds > MAX_DEFAULT_CMD);
	cmd_buffer = (DSP_CMD *)((u8 *)dsp->default_cmd_virt +
				sizeof(DSP_HEADER_CMD)) + hdr->num_cmds - 1;
	spin_unlock_irq(&dsp->lock);

	return cmd_buffer;
}

static void __set_default_cmds(struct ambarella_dsp *dsp, struct amb_dsp_cmd *first)
{
	struct amb_dsp_cmd *cmd, *_cmd;
	DSP_CMD *dsp_cmd;

	if (first) {
		/* add myself first */
		dsp_cmd = __get_default_cmd(dsp);
		memcpy(dsp_cmd, &first->dsp_cmd, sizeof(DSP_CMD));
		dsp_print_cmd(dsp_cmd);
		/* now add children list */
		list_for_each_entry_safe(cmd, _cmd, &first->head, node) {
			dsp_cmd = __get_default_cmd(dsp);
			memcpy(dsp_cmd, &cmd->dsp_cmd, sizeof(DSP_CMD));
			dsp_print_cmd(dsp_cmd);
			list_del(&cmd->node);
			kfree(cmd);
		}
		kfree(first);
	}
}

static void __reset_idsp(struct ambarella_dsp *dsp)
{
	iav_debug("dsp reset idsp \n");
#if 0
	dsp->idsp_reset_flag = 1;
	amba_writel((get_ambarella_apb_virt() + 0x118000), 0x100);
	if (!(amba_readl(get_ambarella_apb_virt() + 0x110068) & 0x0010)) {
		amba_writel((get_ambarella_apb_virt() + 0x11801c), 0xFF);
	}
#endif
	/* reset analog/digital mipi phy */
	amba_writel(DBGBUS_BASE + 0x11801c, 0x30000);
	mdelay(10);
	amba_writel(DBGBUS_BASE + 0x11801c, 0x0);
#if 1
	/* Reset setion 1 */
	amba_writel(DBGBUS_BASE + 0x11801c, 0x2);
	mdelay(10);
	amba_writel(DBGBUS_BASE + 0x11801c, 0x0);
#else
	/* Reset setion 8 for PIP */
	amba_writel(DBGBUS_BASE + 0x11801c, 0x100);
	mdelay(10);
	amba_writel(DBGBUS_BASE + 0x11801c, 0x0);
#endif
}

static int __boot_dsp(struct ambarella_dsp *dsp, dsp_op_mode_t op_mode)
{
	if (op_mode == DSP_ENCODE_MODE)
		__reset_idsp(dsp);

	// set dsp code/memd address
	amba_writel(DSP_DRAM_MAIN_REG, PHYS_TO_DSP(DSP_DRAM_CODE_START));
	amba_writel(DSP_DRAM_SUB0_REG, PHYS_TO_DSP(DSP_DRAM_MEMD_START));
	// reset dsp
	amba_writel(DSP_CONFIG_SUB0_REG, 0xF);
	amba_writel(DSP_CONFIG_MAIN_REG, 0xF);

	return 0;
}

static int __enter_enc_timer_mode(struct ambarella_dsp *dsp)
{
	struct amb_dsp_cmd *cmd;
	vin_timer_mode_t *dsp_cmd;

	cmd = dsp_get_cmd(&dsp->dsp_dev, 1);
	if (!cmd)
		return -ENOMEM;

	dsp_cmd = (vin_timer_mode_t *)&cmd->dsp_cmd;
	dsp_cmd->cmd_code = H264_ENC_USE_TIMER;
	dsp_cmd->timer_scaler = 0;

	dsp_set_enc_sub_mode(&dsp->dsp_dev, TIMER_MODE, cmd, 0);

	return 0;
}

static inline void __prepare_mode_switch(struct ambarella_dsp *dsp,
	u32 op_mode, struct amb_dsp_cmd *first)
{
	__set_default_cmds(dsp, first);
	dsp->dsp_init_data->operation_mode = op_mode;
	clean_d_cache(dsp->dsp_init_data, sizeof(dsp_init_data_t));
}

static void __reset_operation_cmd(struct dsp_device * dsp)
{
	struct amb_dsp_cmd * cmd = dsp_get_cmd(dsp, 0);
	reset_operation_t *dsp_cmd = (reset_operation_t *) &cmd->dsp_cmd;
	dsp_cmd->cmd_code = RESET_OPERATION;
	dsp_put_cmd(dsp, cmd, 0);
}

struct amb_dsp_cmd *dsp_get_cmd(struct dsp_device *dsp_dev, u32 flag)
{
	struct ambarella_dsp *dsp = to_ambarella_dsp(dsp_dev);
	struct amb_dsp_cmd *cmd = NULL;

	cmd = __alloc_cmd(dsp);
	if (!cmd)
		return NULL;

	cmd->cmd_type = DSP_CMD_TYPE_NORMAL;
	cmd->flag = flag;

	return cmd;
}

struct amb_dsp_cmd *dsp_get_multi_cmds(struct dsp_device *dsp_dev,
		int num, u32 flag)
{
	struct amb_dsp_cmd *first, *cmd;
	int i;

	if (num == 0)
		return NULL;

	first = dsp_get_cmd(dsp_dev, flag);
	if (!first)
		return NULL;

	for (i = 1; i < num; i++) {
		cmd = dsp_get_cmd(dsp_dev, flag);
		if (!cmd)
			goto get_cmd_err;

		list_add_tail(&cmd->node, &first->head);
	}

	return first;

get_cmd_err:
	dsp_release_cmd(dsp_dev, first);
	return NULL;
}

void dsp_put_cmd(struct dsp_device *dsp_dev, struct amb_dsp_cmd *first,
	u32 delay)
{
	struct ambarella_dsp *dsp = to_ambarella_dsp(dsp_dev);
	struct dsp_cmd_port *port;
	struct amb_dsp_cmd *cmd;
	struct dsp_cmd_array *cmd_array;
	unsigned long flags;
	u32 sub_num = 0;
	u32 sub_enc_num = 0;
	u32 idx;

	if (!first || delay >= DSP_CMD_LIST_NUM)
		return;

	/* print commands if necessary */
	dsp_print_cmd(&first->dsp_cmd);
	sub_num = 1;
	if (first->cmd_type == DSP_CMD_TYPE_ENC) {
		++sub_enc_num;
	}
	list_for_each_entry(cmd, &first->head, node) {
		if (cmd->dsp_cmd.cmd_code) {
			dsp_print_cmd(&cmd->dsp_cmd);
			++sub_num;
			if (cmd->cmd_type == DSP_CMD_TYPE_ENC) {
				++sub_enc_num;
			}
		}
	}

	port = &dsp->gen_cmd_port;
	while (1) {
		spin_lock_irqsave(&dsp->lock, flags);
		idx = (port->cmd_list_idx + delay) % DSP_CMD_LIST_NUM;
		cmd_array = &port->cmd_array[idx];
		if (unlikely(cmd_array->num_cmds + sub_num > MAX_NUM_CMD)) {
			iav_printk("===== DSP CMD Q is full. Wait for next INT!\n");
		} else if (cmd_array->num_enc_cmds + sub_enc_num > MAX_NUM_ENC_CMD) {
			iav_printk("===== DSP ENC CMD Q is full. Wait for next INT!\n");
		} else {
			break;
		}
		/* Wait until cmd Q is ready */
		spin_unlock_irqrestore(&dsp->lock, flags);
		dsp_wait_vcap(dsp_dev, 1);
	}
	__put_cmd(dsp, port, first, sub_num, delay);
	//iav_printk("DSP CMD Q [%d -%d], DSP ENC CMD Q [%d -%d].\n",
	//	cmd_array->num_cmds, sub_num, cmd_array->num_enc_cmds, sub_enc_num);
	cmd_array->num_cmds += sub_num;
	cmd_array->num_enc_cmds += sub_enc_num;
	spin_unlock_irqrestore(&dsp->lock, flags);
}

void dsp_release_cmd(struct dsp_device *dsp_dev, struct amb_dsp_cmd *first)
{
	struct ambarella_dsp *dsp = to_ambarella_dsp(dsp_dev);
	struct amb_dsp_cmd *cmd, *_cmd;
	unsigned long flags;

	if (!first)
		return;

	spin_lock_irqsave(&dsp->lock, flags);
	list_for_each_entry_safe(cmd, _cmd, &first->head, node) {
		list_add_tail(&cmd->node, &dsp->free_list);
	}
	list_add_tail(&first->node, &dsp->free_list);
	spin_unlock_irqrestore(&dsp->lock, flags);
}

int dsp_set_enc_sub_mode(struct dsp_device *dsp_dev,
		u32 enc_mode, struct amb_dsp_cmd *first, u8 force)
{
	int rval = 0;
	struct ambarella_dsp *dsp = to_ambarella_dsp(dsp_dev);

	if ((!force) && (enc_mode == dsp->enc_mode)) {
		iav_debug("DSP already in ENC sub mode %d, do nothing \n", enc_mode);
		return 0;
	}

	dsp_put_cmd(dsp_dev, first, 0);

	iav_debug("Entering DSP ENC sub mode [%d]\n", enc_mode);
	rval = wait_event_interruptible_timeout(dsp->sub_wq,
		(dsp->enc_mode == enc_mode), TWO_JIFFIES);
	if (rval <= 0) {
		iav_debug("[TIMEOUT] Enter sub mode event.\n");
	} else {
		iav_debug("[Done] Entered sub mode.\n");
	}

	return 0;
}

int dsp_set_op_mode(struct dsp_device *dsp_dev,
		u32 op_mode, struct amb_dsp_cmd *first)
{
	int rval = 0;
	struct ambarella_dsp *dsp = to_ambarella_dsp(dsp_dev);
	iav_debug("[debug]: dsp_set_op_mode: dsp->op_mode(%d), op_mode(%d)\n",
		dsp->op_mode, op_mode);
	if (op_mode == dsp->op_mode) {
		iav_debug("DSP already in mode %d, do nothing \n", op_mode);
		return 0;
	}

	if (dsp->op_mode == DSP_UNKNOWN_MODE) {
		/* DSP not booted, prepare default command */
		__prepare_mode_switch(dsp, op_mode, first);
		__boot_dsp(dsp, op_mode);
	} else if (dsp->op_mode == DSP_ENCODE_MODE) {
		/* enter into timer mode first */
		__enter_enc_timer_mode(dsp);

		__prepare_mode_switch(dsp, op_mode, first);
		__reset_operation_cmd(dsp_dev);
	} else if (dsp->op_mode == DSP_DECODE_MODE) {
		__prepare_mode_switch(dsp, op_mode, first);
		__reset_operation_cmd(dsp_dev);
	}

	iav_debug("Entering DSP mode [%d]...\n", op_mode);
	rval = wait_event_interruptible_timeout(dsp->op_wq,
		(dsp->op_mode == op_mode), TWO_JIFFIES);
	if (rval <= 0) {
		iav_debug("[TIMEOUT] Enter mode event.\n");
		return (-1);
	} else {
		iav_debug("[Done] Entered mode [%d].\n", op_mode);
	}

	return 0;
}

int dsp_get_chip_id(struct dsp_device *dsp_dev, u32 *id, u32 *chip)
{
	if (id != NULL) {
		struct ambarella_dsp *dsp = to_ambarella_dsp(dsp_dev);
		*id = *(dsp->chip_id);
	}

	if (chip != NULL) {
		*chip = bopt_sync((void *)DSP_INIT_DATA_BASE, NULL, NULL, NULL);
	}

	return 0;
}

int dsp_wait_vcap(struct dsp_device * dsp_dev, u32 count)
{
	int rval = 0;
	unsigned long cnt;
	struct ambarella_dsp *dsp = to_ambarella_dsp(dsp_dev);

	if ((DSP_ENCODE_MODE == dsp->op_mode) && (VIDEO_MODE == dsp->enc_mode)) {
		cnt = dsp->vcap_counter + count;
		rval = wait_event_interruptible_timeout(dsp->vcap_wq,
			(dsp->vcap_counter >= cnt), TWO_JIFFIES);
		if (rval <= 0) {
			iav_debug("[TIMEOUT] Wait VCAP event.\n");
		}
	} else {
		iav_debug("DSP is not in Encode video mode.\n");
		rval = -1;
	}

	return rval;
}

dsp_init_data_t *dsp_get_init_data(struct dsp_device *dsp_dev)
{
	struct ambarella_dsp *dsp = to_ambarella_dsp(dsp_dev);

	return dsp->dsp_init_data;
}

int dsp_set_audit(struct dsp_device * dsp_dev, u32 id, u32 audit_addr)
{
	struct ambarella_dsp *dsp = to_ambarella_dsp(dsp_dev);
	struct dsp_audit_info *audit = (struct dsp_audit_info *)audit_addr;

	dsp->audit[id].enable = audit->enable;
	if (!audit->enable) {
		dsp->audit[id].cnt = 0;
		dsp->audit[id].sum = 0;
		dsp->audit[id].max = 0;
		dsp->audit[id].min = ~0;
	}

	return 0;
}

int dsp_get_audit(struct dsp_device * dsp_dev, u32 id, u32 audit_addr)
{
	struct ambarella_dsp *dsp = to_ambarella_dsp(dsp_dev);
	struct dsp_audit_info *audit = (struct dsp_audit_info *)audit_addr;

	*audit = dsp->audit[id];

	return 0;
}

