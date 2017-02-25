/*
 * dsp_drv.c
 *
 * History:
 *	2012/09/27 - [Cao Rongrong] Created
 *	2014/03/03 - [Jian Tang] Modified
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


#include <linux/module.h>
#include <linux/ambpriv_device.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <linux/err.h>
#include <plat/ambcache.h>
#include <iav_ioctl.h>
#include "dsp.h"

static struct ambpriv_device *ambpriv_dsp_device;

/* It needs to switch back to VOUTA INT, if there is plan for PIP. */
//#define	VENC_IRQ_NO		(VIN_IRQ)
#define	VENC_IRQ_NO		(VDSP_PIP_CODING_IRQ)

static void __issue_cmd(void *dsp_cmd, u32 size, u32 flag, u32 delay)
{
	struct ambarella_dsp *dsp;
	struct amb_dsp_cmd *cmd;

	if (!dsp_cmd || size == 0 || delay >= DSP_CMD_LIST_NUM)
		return;

	dsp = ambpriv_get_drvdata(ambpriv_dsp_device);

	cmd = dsp_get_cmd(&dsp->dsp_dev, flag);
	if (!cmd)
		return;

	memcpy(&cmd->dsp_cmd, dsp_cmd, size);

	dsp_put_cmd(&dsp->dsp_dev, cmd, delay);
}

void dsp_issue_cmd(void *dsp_cmd, u32 size)
{
	__issue_cmd(dsp_cmd, size, DSP_CMD_FLAG_NORMAL, 0);
}
EXPORT_SYMBOL(dsp_issue_cmd);

void dsp_issue_delay_cmd(void *dsp_cmd, u32 size, u32 delay)
{
	__issue_cmd(dsp_cmd, size, DSP_CMD_FLAG_NORMAL, delay);
}
EXPORT_SYMBOL(dsp_issue_delay_cmd);

void dsp_issue_img_cmd(void *dsp_cmd, u32 size)
{
#if USE_INSTANT_CMD
	__issue_cmd(dsp_cmd, size, DSP_CMD_FLAG_INSTANT);
#else
	__issue_cmd(dsp_cmd, size, DSP_CMD_FLAG_NORMAL, 0);
#endif
}
EXPORT_SYMBOL(dsp_issue_img_cmd);

struct dsp_device *ambarella_request_dsp(void)
{
	struct ambarella_dsp *dsp;

	dsp = ambpriv_get_drvdata(ambpriv_dsp_device);

	return &dsp->dsp_dev;
}
EXPORT_SYMBOL(ambarella_request_dsp);

#if USE_INSTANT_CMD
static inline void update_dsp_cmd_list(struct ambarella_dsp *dsp,
	struct dsp_cmd_port *port, u32 *cmd_num, u32 prev_num_cmds)
{
	u8 * ptr = NULL;
	u32 i, rest_cmds;
	/* Re-align the rest commands issued after DSP fetched */
	BUG_ON(prev_num_cmds > port->num_cmds);
	rest_cmds = port->num_cmds - prev_num_cmds;
	if ((rest_cmds > 0) && (prev_num_cmds > 0)) {
		ptr = (u8*)port->cmd_queue + sizeof(DSP_HEADER_CMD);
		for (i = 0; i < rest_cmds; ++i, ptr += DSP_CMD_SIZE) {
			memcpy(ptr, ptr + prev_num_cmds * DSP_CMD_SIZE, DSP_CMD_SIZE);
		}
	}
	*cmd_num = rest_cmds;
}

static inline void update_dsp_cmd_hdr(struct dsp_cmd_port *port,
	DSP_HEADER_CMD *hdr, u32 cmd_num, u32 prev_num_cmds, u32 prev_cmd_seq)
{
//	encode_msg_t *enc_msg = (encode_msg_t *)port->current_msg;

	if ((port->num_cmds > 0) || (prev_num_cmds > 0)) {
		hdr->num_cmds = port->num_cmds;
		if (port->num_cmds == 0) {
			port->update_cmd_seq_num = 1;
		} else {
			if (prev_cmd_seq == hdr->cmd_seq_num) {
				++hdr->cmd_seq_num;
				port->update_cmd_seq_num = 0;
			}
		}
//		iav_printk("enc_msg [%d : %d], hdr [%d: %d], port [%d], cmd [0x%x].\n",
//			enc_msg->prev_cmd_seq, enc_msg->prev_num_cmds,
//			hdr->cmd_seq_num, hdr->num_cmds, port->num_cmds, *(u32 *)(ptr));
	} else if (hdr->num_cmds != 0) {
		hdr->num_cmds = 0;
//		iav_printk("enc_msg [%d : %d], hdr [%d: %d], port [%d].\n",
//			enc_msg->prev_cmd_seq, enc_msg->prev_num_cmds,
//			hdr->cmd_seq_num, hdr->num_cmds, port->num_cmds);
	}
}
#else
static void update_dsp_cmd_list(struct ambarella_dsp *dsp,
	struct dsp_cmd_port *port, u32 *cmd_num, u32 prev_num_cmds)
{
	u32 num, sub_num, idx;
	struct amb_dsp_cmd *first, *_first;
	struct amb_dsp_cmd *cmd, *_cmd;
	struct dsp_cmd_array *cmd_array;
	LIST_HEAD(cmd_list);

	num = *cmd_num;
	idx = port->cmd_list_idx;
	cmd_array = &port->cmd_array[idx];
	list_for_each_entry_safe(first, _first, &cmd_array->cmd_list, node) {
		/* remove empty cmds */
		if (first->dsp_cmd.cmd_code == 0) {
			list_move(&first->node, &dsp->free_list);
			list_splice(&first->head, &dsp->free_list);
			continue;
		}

		/* count command number in the same list */
		sub_num = 1;
		list_for_each_entry_safe(cmd, _cmd, &first->head, node) {
			/* remove empty cmds in sub list */
			if (cmd->dsp_cmd.cmd_code == 0) {
				list_move(&cmd->node, &dsp->free_list);
				continue;
			}
			sub_num++;
			BUG_ON(sub_num >= MAX_NUM_CMD);
		}

		if (first->flag == DSP_CMD_FLAG_BLOCK) {
			/* if there is no space enough for block cmds,
			 * then send all the rest cmds in next irq */
			if (num + sub_num >= MAX_NUM_CMD)
				break;

			list_move_tail(&first->node, &cmd_list);
			list_splice_tail(&first->head, &cmd_list);
			num += sub_num;
		} else {
			/* here we move cmd to cmd_list one by one, because
			 * we must check num every time. */
			num++;
			list_move_tail(&first->node, &cmd_list);
			list_for_each_entry_safe(cmd, _cmd, &first->head, node) {
				if (num < MAX_NUM_CMD) {
					num++;
					list_move_tail(&cmd->node, &cmd_list);
				} else {
					/* the rest cmds will be issued in
					 * next irq */
					list_move_tail(cmd->node.next, &cmd->head);
					list_move(&cmd->node, &cmd_array->cmd_list);
					break;
				}
			}
		}

		if (num >= MAX_NUM_CMD)
			break;
	}

	*cmd_num = num;
	if (num) {
		list_for_each_entry_reverse(cmd, &cmd_list, node) {
			memcpy((u8 *)(port->cmd_queue + num - 1) + sizeof(DSP_HEADER_CMD),
				&cmd->dsp_cmd, DSP_CMD_SIZE);
			num--;
		}
		list_splice(&cmd_list, &dsp->free_list);
	}

}

static inline void update_dsp_cmd_hdr(struct dsp_cmd_port *port,
	DSP_HEADER_CMD *hdr, u32 cmd_num, u32 prev_num_cmds, u32 prev_cmd_seq)
{
//	encode_msg_t *enc_msg = (encode_msg_t *)port->current_msg;

	if (cmd_num == 0) {
		return;
	}
	hdr->num_cmds = port->cmd_array[port->cmd_list_idx].num_cmds;
	hdr->cmd_seq_num++;
//	iav_printk("enc_msg [%d : %d], hdr [%d: %d], port [%d].\n",
//		enc_msg->prev_cmd_seq, enc_msg->prev_num_cmds,
//		hdr->cmd_seq_num, hdr->num_cmds, port->num_cmds);
}
#endif

static void dsp_transfer_cmd(struct ambarella_dsp *dsp, struct dsp_cmd_port *port)
{
	DSP_HEADER_CMD *hdr = NULL;
	encode_msg_t *enc_msg = NULL;
	u32 num = 0;
	u32 prev_num_cmds, prev_cmd_seq;
	u32 idx;

	hdr = (DSP_HEADER_CMD *)port->cmd_queue;
	if (MPV_FIXED_BUFFER == dsp->dsp_init_data->cmdmsg_protocol_version) {
		enc_msg = (encode_msg_t *)port->msg_queue;
	} else {
		enc_msg = (encode_msg_t *)port->current_msg;
	}
	prev_cmd_seq  = enc_msg->prev_cmd_seq;
	prev_num_cmds = (prev_cmd_seq != port->prev_cmd_seq_num) ?
		enc_msg->prev_num_cmds : 0;
	port->prev_cmd_seq_num = prev_cmd_seq;

	spin_lock(&dsp->lock);
	update_dsp_cmd_list(dsp, port, &num, prev_num_cmds);
	idx = port->cmd_list_idx;
	port->cmd_array[idx].num_cmds = num;
	/* Update command header. */
	update_dsp_cmd_hdr(port, hdr, num, prev_num_cmds, prev_cmd_seq);
	spin_unlock(&dsp->lock);

	dsp->dsp_init_data->prev_cmd_seq_num = hdr->cmd_seq_num;
	clean_d_cache(dsp->dsp_init_data, sizeof(dsp_init_data_t));

#if !USE_INSTANT_CMD
	port->cmd_array[idx].num_cmds = 0;
	port->cmd_array[idx].num_enc_cmds = 0;
	port->cmd_array[idx].num_vout_cmds = 0;
	port->cmd_list_idx = (idx + 1) % DSP_CMD_LIST_NUM;
#endif
}

static void dsp_process_msg(struct ambarella_dsp *dsp, struct dsp_cmd_port *port)
{
	struct dsp_device *dsp_dev = &dsp->dsp_dev;
	DSP_MSG * msg = NULL;
	encode_msg_t *encode_msg = NULL, *next_msg = NULL;
	static u32 prev_enc_mode = ENC_UNKNOWN_MODE;
	static int init_done = 0;
	int i;

	if (MPV_FIXED_BUFFER == dsp->dsp_init_data->cmdmsg_protocol_version) {
		msg = (DSP_MSG *)port->msg_queue;
		/* Fixed buffer, no need to add catch up mechanism. */
		init_done = 1;
	} else {
		msg = (DSP_MSG *)port->current_msg;
	}
	for (i = 0; i < dsp->vdsp_info->num_dsp_msgs; i++) {
		if (msg[i].msg_code == DSP_STATUS_MSG_ENCODE) {
			encode_msg = (encode_msg_t *)&msg[i];
			break;
		} else if (msg[i].msg_code == DSP_STATUS_MSG_DECODE) {
			encode_msg = (encode_msg_t *)&msg[i];
			break;
		}
	}
	if (encode_msg == NULL) {
		iav_error("Fatal: Incorrect encode msg, use the first msg anyway!\n");
		encode_msg = (encode_msg_t *)msg;
	}

	port->current_msg++;
	if ((u8 *)port->current_msg >= ((u8 *)port->msg_queue + DSP_MSG_BUF_SIZE)) {
		port->current_msg = port->msg_queue;
	}

	if (!init_done) {
		/* Special handling for DSP booted to encode mode before Linux */
		if (DSP_ENCODE_MODE == dsp->probe_op_mode) {
			while (1) {
				next_msg = (encode_msg_t *)port->current_msg;
				if (next_msg->vsync_cnt <= encode_msg->vsync_cnt) {
					break;
				}
				encode_msg = next_msg;

				port->current_msg++;
				if ((u8 *)port->current_msg >= ((u8 *)port->msg_queue + DSP_MSG_BUF_SIZE)) {
					port->current_msg = port->msg_queue;
				}
			}
		}
		init_done = 1;
	}

	if (dsp->op_mode != encode_msg->dsp_operation_mode) {
		dsp->op_mode = encode_msg->dsp_operation_mode;
		wake_up_interruptible(&dsp->op_wq);
	}
	if (dsp->enc_mode != encode_msg->encode_operation_mode) {
		dsp->enc_mode = encode_msg->encode_operation_mode;
		wake_up_interruptible(&dsp->sub_wq);
	}

	if (DSP_ENCODE_MODE == dsp->op_mode) {
		if ((VIDEO_MODE == dsp->enc_mode) ||
			((VIDEO_MODE == prev_enc_mode) && (TIMER_MODE == dsp->enc_mode))) {
			dsp->vcap_counter++;
			wake_up_interruptible(&dsp->vcap_wq);
		}
		prev_enc_mode = dsp->enc_mode;
	}

	for (i = 0; i < dsp->vdsp_info->num_dsp_msgs; ++i, ++msg) {
		if (DSP_DECODE_MODE == dsp->op_mode) {
			if (dsp_dev->msg_callback[CAT_DEC]) {
				dsp_dev->msg_callback[CAT_DEC](dsp_dev->msg_data[CAT_DEC],
					msg);
			}
		} else {
			if (dsp_dev->msg_callback[CAT_VCAP]) {
				dsp_dev->msg_callback[CAT_VCAP](dsp_dev->msg_data[CAT_VCAP],
					msg);
			}
		}
	}
}

static void do_audit_update(struct dsp_audit_info *audit,
	struct timeval *start, struct timeval *stop)
{
	u64 delta = 0;

	++audit->cnt;
	delta = ((u64) (stop->tv_sec - start->tv_sec) * 1000000L +
		(stop->tv_usec - start->tv_usec));
	audit->sum += delta;
	if (delta > audit->max) {
		audit->max = delta;
	}
	if (delta < audit->min) {
		audit->min = delta;
	}
}

static irqreturn_t vdsp_irq(int irqno, void *dev_id)
{
	struct ambarella_dsp *dsp = dev_id;
	struct dsp_audit_info *audit = &dsp->audit[DAI_VDSP];
	struct timeval start, stop;

	if (audit->enable) {
		do_gettimeofday(&start);
	}

	dsp->irq_counter++;
	dsp_transfer_cmd(dsp, &dsp->gen_cmd_port);
	dsp_process_msg(dsp, &dsp->gen_cmd_port);

	if (audit->enable) {
		do_gettimeofday(&stop);
		do_audit_update(audit, &start, &stop);
	}

	return IRQ_HANDLED;
}

static irqreturn_t venc_irq(int irqno, void *dev_id)
{
	struct ambarella_dsp *dsp = dev_id;
	struct dsp_audit_info *audit = &dsp->audit[DAI_VENC];
	struct dsp_device *dsp_dev = &dsp->dsp_dev;
	struct timeval start, stop;

	if (audit->enable) {
		do_gettimeofday(&start);
	}

	spin_lock(&dsp->lock);
	if (dsp_dev->msg_callback[CAT_ENC]) {
		dsp_dev->msg_callback[CAT_ENC](dsp_dev->msg_data[CAT_ENC], NULL);
	}
	spin_unlock(&dsp->lock);

	if (audit->enable) {
		do_gettimeofday(&stop);
		do_audit_update(audit, &start, &stop);
	}

	return IRQ_HANDLED;
}

static int dsp_irq_init(struct ambarella_dsp *dsp)
{
	int rval;

	rval = request_irq(CODE_VDSP_0_IRQ, vdsp_irq,
			IRQF_TRIGGER_RISING,
			"vdsp", dsp);
	if (rval < 0) {
		iav_error("request_irq failed (vdsp)\n");
		return rval;
	}
	rval = request_irq(VENC_IRQ_NO, venc_irq,
			IRQF_TRIGGER_RISING,
			"venc", dsp);
	if (rval < 0) {
		iav_error("request_irq failed (venc)\n");
		return rval;
	}

	return 0;
}

static void dsp_irq_free(struct ambarella_dsp *dsp)
{
	free_irq(CODE_VDSP_0_IRQ, dsp);
	free_irq(VENC_IRQ_NO, dsp);
}

static void dsp_free_cmd_port(struct dsp_cmd_port *port)
{
	struct amb_dsp_cmd *first, *_first, *cmd, *_cmd;
	struct dsp_cmd_array *cmd_array = port->cmd_array;
	int i;

	for (i = 0; i < DSP_CMD_LIST_NUM; ++i) {
		list_for_each_entry_safe(first, _first, &cmd_array[i].cmd_list, node) {
			/* delete children list first */
			list_for_each_entry_safe(cmd, _cmd, &first->head, node) {
				list_del(&cmd->node);
				kfree(cmd);
			}
			/* now delete myself */
			list_del(&first->node);
			kfree(first);
		}
	}

	if (port->msg_queue) {
		iounmap((void __iomem *)port->msg_queue);
		port->msg_queue = NULL;
	}
	if (port->cmd_queue) {
		iounmap((void __iomem *)port->cmd_queue);
		port->cmd_queue = NULL;
	}

}

static int dsp_init_cmd_port(struct dsp_cmd_port *port, u32 probe_mode)
{
	int i;

	port->total_cmd_size = DSP_CMD_BUF_SIZE;
	port->cmd_queue = ioremap_nocache(DSP_CMD_BUF_START,
		port->total_cmd_size);
	if (!port->cmd_queue) {
		iav_error("Failed to call ioremap() for DSP CMD.\n");
		return -ENOMEM;
	}

	port->total_msg_size = DSP_MSG_BUF_SIZE;
	port->msg_queue = ioremap_nocache(DSP_MSG_BUF_START,
		port->total_msg_size);
	if (!port->msg_queue) {
		iav_error("Failed to call ioremap() for DSP MSG.\n");
		iounmap((void __iomem *)port->cmd_queue);
		return -ENOMEM;
	}

	if (probe_mode == DSP_UNKNOWN_MODE) {
		memset(port->cmd_queue, 0, DSP_CMD_BUF_SIZE);
		memset(port->msg_queue, 0, DSP_MSG_BUF_SIZE);
	}

	port->current_msg = port->msg_queue;
	port->prev_cmd_seq_num = 0;
	port->cmd_list_idx = 0;

	/* command management */
	for (i = 0; i < DSP_CMD_LIST_NUM; ++i) {
		INIT_LIST_HEAD(&port->cmd_array[i].cmd_list);
		port->cmd_array[i].num_cmds = 0;
		port->cmd_array[i].num_enc_cmds = 0;
		port->cmd_array[i].num_vout_cmds = 0;
		port->cmd_array[i].update_cmd_seq_num = 1;
	}

	return 0;
}

static int dsp_fill_init_data(struct ambarella_dsp *dsp)
{
	dsp_init_data_t *init_data = NULL;

	/* initialize dsp_init_data */
	init_data = dsp->dsp_init_data;

	if (dsp->probe_op_mode == DSP_UNKNOWN_MODE) {
		memset(init_data, 0, sizeof(dsp_init_data_t));
	}

	/* setup default binary data pointer */
	init_data->default_binary_data_ptr = (u32 *)PHYS_TO_DSP(DSP_BINARY_DATA_START);

	/* cmd / msg queue */
	init_data->cmd_data_ptr = (u32 *)PHYS_TO_DSP(DSP_CMD_BUF_START);
	init_data->cmd_data_size = DSP_CMD_BUF_SIZE;
	init_data->result_queue_ptr = (u32 *)PHYS_TO_DSP(DSP_MSG_BUF_START);
	init_data->result_queue_size = DSP_MSG_BUF_SIZE;

	/* setup default cmd queue address */
	init_data->default_config_ptr = (u32 *)PHYS_TO_DSP(DSP_DEF_CMD_BUF_START);
	init_data->default_config_size = DSP_DEF_CMD_BUF_SIZE;
	if (dsp->probe_op_mode == DSP_UNKNOWN_MODE) {
		memset(dsp->default_cmd_virt, 0, dsp->total_default_cmd_size);
	}

	/* setup dsp buffer */
	init_data->DSP_buf_ptr = (u32 *)PHYS_TO_DSP(dsp->dsp_dev.buffer_start);
	init_data->DSP_buf_size = dsp->dsp_dev.buffer_size;
	init_data->dsp_log_buf_ptr = PHYS_TO_DSP(DSP_LOG_AREA_PHYS);
	init_data->dsp_log_size = DSP_LOG_SIZE;

	/* chip ID */
	init_data->chip_id_ptr = (u32 *)PHYS_TO_DSP(DSP_CHIP_ID_START);
	init_data->chip_id_checksum = 0;
	dsp->chip_id = ioremap_nocache(DSP_CHIP_ID_START,
		sizeof(u32) + sizeof(vdsp_info_t));
	if (!dsp->chip_id) {
		iav_error("Failed to call ioremap() for CHIP ID.\n");
		return -ENOMEM;
	}

	/* vdsp info */
	init_data->vdsp_info_ptr = (u32 *)PHYS_TO_DSP(DSP_CHIP_ID_START + sizeof(u32));
	dsp->vdsp_info = (vdsp_info_t *)((u32)dsp->chip_id + sizeof(u32));
	if (dsp->probe_op_mode == DSP_UNKNOWN_MODE) {
		dsp->vdsp_info->dsp_cmd_rx = 1;
		dsp->vdsp_info->dsp_msg_tx = 1;
		dsp->vdsp_info->num_dsp_msgs = 0;

		/* Use "fixed" MSG buffer only for normal boot. */
		init_data->cmdmsg_protocol_version = MPV_FIXED_BUFFER;
	}

#ifdef CONFIG_AMBARELLA_IAV_DRAM_VOUT_ONLY
	init_data->vout_profile = DEFAULT_VOUT0_4M_ROTATE_PROFILE;
#else
	init_data->vout_profile = DEFAULT_VOUT_PROFILE;
#endif

	clean_d_cache(init_data, sizeof(dsp_init_data_t));

	return 0;
}

int dsp_init_data(struct ambarella_dsp *dsp)
{
	int rval = 0;

	dsp->dsp_init_data = ioremap_nocache(DSP_INIT_DATA_START, sizeof(dsp_init_data_t));
	if (!dsp->dsp_init_data) {
		iav_error("Failed to call ioremap() for INIT DATA.\n");
		rval = -ENOMEM;
		goto dsp_init_data_err1;
	}

	/* dsp_init_data_pm is used to store dsp_init_data when system suspend */
	dsp->dsp_init_data_pm = kzalloc(sizeof(dsp_init_data_t), GFP_KERNEL);
	if (!dsp->dsp_init_data_pm) {
		iav_error("Out of memory (%d)!\n", __LINE__);
		rval = -ENOMEM;
		goto dsp_init_data_err2;
	}

	/* prepare memory for default commands */
	dsp->total_default_cmd_size = DSP_DEF_CMD_BUF_SIZE;
	dsp->default_cmd_phys = DSP_DEF_CMD_BUF_START;
	dsp->default_cmd_virt = ioremap_nocache(dsp->default_cmd_phys,
		dsp->total_default_cmd_size);
	if (!dsp->default_cmd_virt) {
		iav_error("Failed to call ioremap() for default_cmd.\n");
		rval = -ENOMEM;
		goto dsp_init_data_err3;
	}

	/* Fill initialized data for DSP */
	rval = dsp_fill_init_data(dsp);
	if (rval < 0) {
		goto dsp_init_data_err4;
	}

	bopt_sync((void *)DSP_INIT_DATA_START, (void *)DSP_IDSP_BASE,
		(void *)DSP_UCODE_START, NULL);

	return 0;

dsp_init_data_err4:
	iounmap((void __iomem *)dsp->default_cmd_virt);
	dsp->default_cmd_virt = NULL;
	dsp->default_cmd_phys = 0;
dsp_init_data_err3:
	kfree(dsp->dsp_init_data_pm);
	dsp->dsp_init_data_pm = NULL;
dsp_init_data_err2:
	iounmap((void __iomem *)dsp->dsp_init_data);
dsp_init_data_err1:
	return rval;
}

static void dsp_free_data(struct ambarella_dsp *dsp)
{
	iounmap((void __iomem *)dsp->chip_id);
	dsp->chip_id = NULL;
	dsp->vdsp_info = NULL;
	iounmap((void __iomem *)dsp->dsp_init_data);

	kfree(dsp->dsp_init_data_pm);
	dsp->dsp_init_data_pm = NULL;

	iounmap((void __iomem *)dsp->default_cmd_virt);
	dsp->default_cmd_virt = NULL;
}

static int dsp_audit_init(struct ambarella_dsp * dsp)
{
	dsp->audit[DAI_VDSP].id = DAI_VDSP;
	dsp->audit[DAI_VDSP].enable = 0;
	dsp->audit[DAI_VDSP].min = ~0;
	dsp->audit[DAI_VENC].id = DAI_VENC;
	dsp->audit[DAI_VENC].enable = 0;
	dsp->audit[DAI_VENC].min = ~0;

	return 0;
}

static int dsp_drv_probe_status(struct ambarella_dsp *dsp)
{
	u32 *p_iav_state = NULL;
	u32 iav_state = 0;
	u32 dsp_op_mode = 0;
	u32 dsp_enc_mode = 0;

	if (DSP_FASTDATA_SIZE > 0) {
		p_iav_state = ioremap_nocache(DSP_STATUS_STORE_START, sizeof(u32));
		if (!p_iav_state) {
			iav_error("Failed to call ioremap() for DSP STATUS.\n");
			return -ENOMEM;
		}
		iav_state = *p_iav_state;
	} else {
		iav_state = IAV_STATE_INIT;
	}

	switch (iav_state) {
	case IAV_STATE_IDLE:
		dsp_op_mode = DSP_ENCODE_MODE;
		dsp_enc_mode = TIMER_MODE;
		break;
	case IAV_STATE_PREVIEW:
	case IAV_STATE_ENCODING:
		dsp_op_mode = DSP_ENCODE_MODE;
		dsp_enc_mode = VIDEO_MODE;
		break;
	case IAV_STATE_INIT:
	default:
		dsp_op_mode = DSP_UNKNOWN_MODE;
		dsp_enc_mode = ENC_UNKNOWN_MODE;
		break;
	}
	dsp->op_mode = dsp_op_mode;
	dsp->probe_op_mode = dsp_op_mode;
	dsp->enc_mode = dsp_enc_mode;

	if (p_iav_state) {
		iounmap((void __iomem *)p_iav_state);
		p_iav_state = NULL;
	}
	iav_debug("DSP probe op mode [%u], enc mode [%u].\n", dsp_op_mode, dsp_enc_mode);

	return 0;
}

static int dsp_drv_probe(struct ambpriv_device *ambdev)
{
	struct ambarella_dsp *dsp;
	struct dsp_device *dsp_dev;
	int rval;

	dsp = kzalloc(sizeof(struct ambarella_dsp), GFP_KERNEL);
	if (!dsp) {
		iav_error("Out of memory (%d)!\n", __LINE__);
		return -ENOMEM;
	}

	spin_lock_init(&dsp->lock);
	init_waitqueue_head(&dsp->op_wq);
	init_waitqueue_head(&dsp->sub_wq);
	init_waitqueue_head(&dsp->vcap_wq);
	INIT_LIST_HEAD(&dsp->free_list);

	dsp_dev = &dsp->dsp_dev;
	dsp_dev->dev = &ambdev->dev;
	strlcpy(dsp->dsp_dev.name, dev_name(dsp->dsp_dev.dev), 32);
	dsp->dsp_dev.bsb_start = DSP_BSB_START;
	dsp->dsp_dev.buffer_start = DSP_DRAM_START;
	dsp->dsp_dev.buffer_size = DSP_DRAM_SIZE;
	dsp_dev->get_cmd = dsp_get_cmd;
	dsp_dev->get_multi_cmds = dsp_get_multi_cmds;
	dsp_dev->put_cmd = dsp_put_cmd;
	dsp_dev->print_cmd = NULL;
	dsp_dev->release_cmd = dsp_release_cmd;
	dsp_dev->set_op_mode = dsp_set_op_mode;
	dsp_dev->set_enc_sub_mode = dsp_set_enc_sub_mode;
	dsp_dev->get_chip_id = dsp_get_chip_id;
	dsp_dev->wait_vcap = dsp_wait_vcap;
	dsp_dev->get_dsp_init_data = dsp_get_init_data;
	dsp_dev->set_audit = dsp_set_audit;
	dsp_dev->get_audit = dsp_get_audit;
#ifdef CONFIG_PM
	dsp_dev->suspend = dsp_suspend;
	dsp_dev->resume = dsp_resume;
#endif
	dsp_dev->set_clock_state = dsp_set_clock_state;

	rval = dsp_drv_probe_status(dsp);
	if (rval < 0)
		goto probe_err1;

	rval = dsp_init_cmd_port(&dsp->gen_cmd_port, dsp->probe_op_mode);
	if (rval < 0)
		goto probe_err1;

	rval = dsp_init_data(dsp);
	if (rval < 0)
		goto probe_err2;

	rval = dsp_irq_init(dsp);
	if (rval < 0)
		goto probe_err3;

	/*rval = dsp_log_init();
	if (rval < 0)
		goto probe_err4;
		REMOVED BY Louis
	*/

	rval = ucode_dev_init(dsp);
	if (rval < 0)
		goto probe_err5;

	rval = dsp_audit_init(dsp);
	if (rval < 0)
		goto probe_err5;

	ambpriv_set_drvdata(ambdev, dsp);

	return 0;

probe_err5:
//	dsp_log_exit();
//probe_err4:
	dsp_irq_free(dsp);
probe_err3:
	dsp_free_data(dsp);
probe_err2:
	dsp_free_cmd_port(&dsp->gen_cmd_port);
probe_err1:
	kfree(dsp);
	return rval;
}

static int dsp_drv_remove(struct ambpriv_device *ambdev)
{
	struct ambarella_dsp *dsp;
	struct amb_dsp_cmd *cmd, *_cmd;

	dsp = ambpriv_get_drvdata(ambdev);

	ucode_dev_exit();
//	dsp_log_exit();
	dsp_irq_free(dsp);
	dsp_free_data(dsp);
	dsp_free_cmd_port(&dsp->gen_cmd_port);

	list_for_each_entry_safe(cmd, _cmd, &dsp->free_list, node) {
		list_del(&cmd->node);
		kfree(cmd);
	}
	kfree(dsp);

	return 0;
}

#ifdef CONFIG_PM
int dsp_suspend(struct dsp_device *dsp_dev)
{
	int i = 0;
	struct ambarella_dsp *dsp = NULL;
	struct dsp_cmd_port *port = NULL;
	DSP_HEADER_CMD *hdr_cmd = NULL;
	struct amb_dsp_cmd *first, *_first;

	/* Avoid DSP to touch memory in suspend */
	dsp_halt();

	dsp = ambpriv_get_drvdata(ambpriv_dsp_device);

	/* The following code is not must have, it's better to re-init the dsp cmd/msg queue */
	port = &dsp->gen_cmd_port;
	memset((u8 *)port->cmd_queue, 0, port->total_cmd_size);
	memset((u8 *)port->msg_queue, 0, port->total_msg_size);

	/* Must clear prev_cmd_seq_num as well, otherwise DSP will be crashed */
	dsp->dsp_init_data->prev_cmd_seq_num = 0;
	port->current_msg = port->msg_queue;
	port->prev_cmd_seq_num = 0;
	port->cmd_list_idx = 0;

	hdr_cmd = (DSP_HEADER_CMD *)port->cmd_queue;
	hdr_cmd->cmd_seq_num = 0;
	hdr_cmd->num_cmds = 0;

	for (i = 0; i < DSP_CMD_LIST_NUM; ++i) {
		if (port->cmd_array[i].num_cmds) {
			list_for_each_entry_safe(first, _first, &port->cmd_array[i].cmd_list, node) {
				list_move(&first->node, &dsp->free_list);
				list_splice(&first->head, &dsp->free_list);
			}

			port->cmd_array[i].num_cmds = 0;
			port->cmd_array[i].num_enc_cmds = 0;
			port->cmd_array[i].num_vout_cmds = 0;
			port->cmd_array[i].update_cmd_seq_num = 1;
		}
	}

	return 0;
}

int dsp_resume(struct dsp_device *dsp_dev)
{
	struct ambarella_dsp *dsp = NULL;

	dsp = ambpriv_get_drvdata(ambpriv_dsp_device);

	if (dsp->enc_mode == VIDEO_MODE || dsp->enc_mode == TIMER_MODE) {
		dsp->enc_mode = ENC_UNKNOWN_MODE;
		dsp->op_mode = DSP_UNKNOWN_MODE;
		dsp->probe_op_mode = DSP_UNKNOWN_MODE;
	}

	return 0;
}
#endif

static struct ambpriv_driver dsp_driver = {
	.probe = dsp_drv_probe,
	.remove = dsp_drv_remove,
	.driver = {
		.name = "dsp_s2l",
		.owner = THIS_MODULE,
	}
};

static int __init dspdrv_init(void)
{
	int rval = 0;

	ambpriv_dsp_device = ambpriv_create_bundle(&dsp_driver, NULL, -1, NULL, -1);
	if (IS_ERR(ambpriv_dsp_device))
		rval = PTR_ERR(ambpriv_dsp_device);

	return rval;
}

static void __exit dspdrv_exit(void)
{
	ambpriv_device_unregister(ambpriv_dsp_device);
	ambpriv_driver_unregister(&dsp_driver);
}

module_init(dspdrv_init);
module_exit(dspdrv_exit);

MODULE_AUTHOR("Cao Rongrong <rrcao@ambarella.com>");
MODULE_DESCRIPTION("Ambarella DSP driver");
MODULE_LICENSE("Proprietary");

