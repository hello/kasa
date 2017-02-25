/*
 * dsp.h
 *
 * History:
 *	2010/06/21 - [Zhenwu Xue] created file
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
#define MAX_NUM_VOUT_CMD	12

#define MAX_CMD_SIZE			(MAX_NUM_CMD * DSP_CMD_SIZE)
#define MAX_DEFAULT_CMD_SIZE		(MAX_DEFAULT_CMD * DSP_CMD_SIZE)

/* There is no message header in S2L, so max num message is 16. */
#define MAX_NUM_MSG			(DSP_MSG_BUF_SIZE / DSP_MSG_SIZE)

struct dsp_cmd_array {
	struct list_head	cmd_list;
	u8			update_cmd_seq_num;
	u8			num_cmds;
	u8			num_enc_cmds;
	u8			num_vout_cmds;
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
extern int dsp_set_op_mode(struct dsp_device *dsp_dev, u32 op_mode,
	struct amb_dsp_cmd *dsp_cmd, u32 no_wait);
extern int dsp_set_enc_sub_mode(struct dsp_device *dsp_dev, u32 enc_mode,
	struct amb_dsp_cmd *first, u32 no_wait, u8 force);
extern void dsp_release_cmd(struct dsp_device *dsp_dev, struct amb_dsp_cmd *first);
extern struct amb_dsp_cmd *dsp_get_cmd(struct dsp_device *dsp_dev, u32 flag);
extern struct amb_dsp_cmd *dsp_get_multi_cmds(struct dsp_device *dsp_dev,
		int num, u32 flag);
extern void dsp_put_cmd(struct dsp_device *dsp_dev, struct amb_dsp_cmd *dsp_cmd, u32 delay);
extern int dsp_get_chip_id(struct dsp_device *dsp_dev, u32 *id, u32 *chip);
extern int dsp_wait_vcap(struct dsp_device *dsp_dev, u32 count);
extern dsp_init_data_t *dsp_get_init_data(struct dsp_device *dsp_dev);
extern int dsp_set_audit(struct dsp_device *dsp_dev, u32 id, unsigned long audit_addr);
extern int dsp_get_audit(struct dsp_device *dsp_dev, u32 id, unsigned long audit_addr);
extern int bopt_sync(void *arg0, void *arg1, void *arg2, void *arg3);
extern int dsp_suspend(struct dsp_device *dsp_dev);
extern int dsp_resume(struct dsp_device *dsp_dev);
extern int dsp_set_clock_state(u32 clk_type, u32 enable);

#ifdef CONFIG_PM
void dsp_halt(void);
#endif

#endif

