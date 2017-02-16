/*
 * Copyright (c) 2004-2011 Atheros Communications Inc.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "core.h"

#include <linux/skbuff.h>
#include <linux/fs.h>
#include <linux/vmalloc.h>
#include <linux/export.h>

#include "debug.h"
#include "target.h"
#include "wlan_location_defs.h"
#include "htc-ops.h"
#include "hif-ops.h"
#include <net/iw_handler.h>


struct ath6kl_fwlog_slot {
	__le32 timestamp;
	__le32 length;

	/* max ATH6KL_FWLOG_PAYLOAD_SIZE bytes */
	u8 payload[0];
};

#define SKIP_SPACE                         \
	while (*p == ' ' && *p != '\0')        \
		p++;

#define SEEK_SPACE                         \
	while (*p != ' ' && *p != '\0')        \
		p++;

#define ATH6KL_FWLOG_MAX_ENTRIES 20
#define ATH6KL_FWLOG_VALID_MASK 0x1ffff

#define ATH6KL_FWLOG_NUM_ARGS_OFFSET    30
#define ATH6KL_FWLOG_NUM_ARGS_MASK      0xC0000000 /* Bit 30-31 */
#define AT6HKL_FWLOG_NUM_ARGS_MAX	2 /* Upper limit is width of mask */

#define ATH6KL_FWGLOG_GET_NUMARGS(arg) \
	((arg & ATH6KL_FWLOG_NUM_ARGS_MASK) >> ATH6KL_FWLOG_NUM_ARGS_OFFSET)

int ath6kl_printk(const char *level, const char *fmt, ...)
{
#if defined(CONFIG_KERNEL_3_8_2) || defined(CONFIG_KERNEL_3_5_7) || defined(CONFIG_KERNEL_3_10_9)
#define LINUX_VERSION_CODE 3000
#define KERNEL_VERSION(_a,_b,_c) _a*1000+_b*100+_c
#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36))
	struct va_format vaf;
#endif
	va_list args;
	int rtn;

	va_start(args, fmt);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36))
	vaf.fmt = fmt;
	vaf.va = &args;

	rtn = printk(KERN_INFO "%sath6kl: %pV", level, &vaf);
#else
	printk("%sath6kl: ", level);
	rtn = vprintk(fmt, args);
#endif

	va_end(args);

	return rtn;
}

#define EVENT_ID_LEN 2
#define INTF_ID_LEN 1

void ath6kl_send_genevent_to_app(struct net_device *dev,
					u16 event_id, u8 ifid,
					u8 *datap, int len)
{
	char *buf;
	u16 size;
	union iwreq_data wrqu;

	size = len + EVENT_ID_LEN + INTF_ID_LEN;

	if (size > IW_GENERIC_IE_MAX)
		return;

	buf = kmalloc(size, GFP_ATOMIC);
	if (buf == NULL)
		return;

	memset(buf, 0, size);
	memcpy(buf, &event_id, EVENT_ID_LEN);
	memcpy(buf + EVENT_ID_LEN, &ifid, INTF_ID_LEN);
	memcpy(buf + EVENT_ID_LEN + INTF_ID_LEN, datap, len);

	memset(&wrqu, 0, sizeof(wrqu));
	wrqu.data.length = size;
	wireless_send_event(dev, IWEVGENIE, &wrqu, buf);
	kfree(buf);

}

void ath6kl_send_event_to_app(struct net_device *dev,
					u16 event_id, u8 ifid,
					u8 *datap, int len)
{
	char *buf;
	u16 size;
	union iwreq_data wrqu;

	size = len + EVENT_ID_LEN + INTF_ID_LEN;

	if (size > IW_CUSTOM_MAX)
		return;

	buf = kmalloc(size, GFP_ATOMIC);
	if (buf == NULL)
		return;

	memset(buf, 0, size);
	memcpy(buf, &event_id, EVENT_ID_LEN);
	memcpy(buf + EVENT_ID_LEN, &ifid, INTF_ID_LEN);
	memcpy(buf + EVENT_ID_LEN + INTF_ID_LEN, datap, len);

	memset(&wrqu, 0, sizeof(wrqu));
	wrqu.data.length = size;
	wireless_send_event(dev, IWEVCUSTOM, &wrqu, buf);
	kfree(buf);
}


#ifdef CONFIG_ATH6KL_DEBUG

#define REG_OUTPUT_LEN_PER_LINE	25
#define REGTYPE_STR_LEN		100

struct ath6kl_diag_reg_info {
	u32 reg_start;
	u32 reg_end;
	const char *reg_info;
};

static const struct ath6kl_diag_reg_info diag_reg[] = {
	{ 0x20000, 0x200fc, "General DMA and Rx registers" },
	{ 0x28000, 0x28900, "MAC PCU register & keycache" },
	{ 0x20800, 0x20a40, "QCU" },
	{ 0x21000, 0x212f0, "DCU" },
	{ 0x4000,  0x42e4, "RTC" },
	{ 0x540000, 0x540000 + (256 * 1024), "RAM" },
	{ 0x29800, 0x2B210, "Base Band" },
	{ 0x1C000, 0x1C748, "Analog" },
	{ 0x5000, 0x5160, "WLAN RTC" },
	{ 0x14000, 0x14170, "GPIO" },
	{ 0x7000, 0x7090, "BTC" },
};

void ath6kl_dump_registers(struct ath6kl_device *dev,
			   struct ath6kl_irq_proc_registers *irq_proc_reg,
			   struct ath6kl_irq_enable_reg *irq_enable_reg)
{

	ath6kl_dbg(ATH6KL_DBG_ANY, ("<------- Register Table -------->\n"));

	if (irq_proc_reg != NULL) {
		ath6kl_dbg(ATH6KL_DBG_ANY,
			"Host Int status:           0x%x\n",
			irq_proc_reg->host_int_status);
		ath6kl_dbg(ATH6KL_DBG_ANY,
			   "CPU Int status:            0x%x\n",
			irq_proc_reg->cpu_int_status);
		ath6kl_dbg(ATH6KL_DBG_ANY,
			   "Error Int status:          0x%x\n",
			irq_proc_reg->error_int_status);
		ath6kl_dbg(ATH6KL_DBG_ANY,
			   "Counter Int status:        0x%x\n",
			irq_proc_reg->counter_int_status);
		ath6kl_dbg(ATH6KL_DBG_ANY,
			   "Mbox Frame:                0x%x\n",
			irq_proc_reg->mbox_frame);
		ath6kl_dbg(ATH6KL_DBG_ANY,
			   "Rx Lookahead Valid:        0x%x\n",
			irq_proc_reg->rx_lkahd_valid);
		ath6kl_dbg(ATH6KL_DBG_ANY,
			   "Rx Lookahead 0:            0x%x\n",
			le32_to_cpu(irq_proc_reg->rx_lkahd[0]));
		ath6kl_dbg(ATH6KL_DBG_ANY,
			   "Rx Lookahead 1:            0x%x\n",
			le32_to_cpu(irq_proc_reg->rx_lkahd[1]));

		if (dev->ar->mbox_info.gmbox_addr != 0) {
			/*
			 * If the target supports GMBOX hardware, dump some
			 * additional state.
			 */
			ath6kl_dbg(ATH6KL_DBG_ANY,
				"GMBOX Host Int status 2:   0x%x\n",
				irq_proc_reg->host_int_status2);
			ath6kl_dbg(ATH6KL_DBG_ANY,
				"GMBOX RX Avail:            0x%x\n",
				irq_proc_reg->gmbox_rx_avail);
			ath6kl_dbg(ATH6KL_DBG_ANY,
				"GMBOX lookahead alias 0:   0x%x\n",
				le32_to_cpu(
				irq_proc_reg->rx_gmbox_lkahd_alias[0]));
			ath6kl_dbg(ATH6KL_DBG_ANY,
				"GMBOX lookahead alias 1:   0x%x\n",
				le32_to_cpu(
				irq_proc_reg->rx_gmbox_lkahd_alias[1]));
		}

	}

	if (irq_enable_reg != NULL) {
		ath6kl_dbg(ATH6KL_DBG_ANY,
			"Int status Enable:         0x%x\n",
			irq_enable_reg->int_status_en);
		ath6kl_dbg(ATH6KL_DBG_ANY, "Counter Int status Enable: 0x%x\n",
			irq_enable_reg->cntr_int_status_en);
	}
	ath6kl_dbg(ATH6KL_DBG_ANY, "<------------------------------->\n");
}

static void dump_cred_dist(struct htc_endpoint_credit_dist *ep_dist)
{
	ath6kl_dbg(ATH6KL_DBG_CREDIT,
		   "--- endpoint: %d  svc_id: 0x%X ---\n",
		   ep_dist->endpoint, ep_dist->svc_id);
	ath6kl_dbg(ATH6KL_DBG_CREDIT, " dist_flags     : 0x%X\n",
		   ep_dist->dist_flags);
	ath6kl_dbg(ATH6KL_DBG_CREDIT, " cred_norm      : %d\n",
		   ep_dist->cred_norm);
	ath6kl_dbg(ATH6KL_DBG_CREDIT, " cred_min       : %d\n",
		   ep_dist->cred_min);
	ath6kl_dbg(ATH6KL_DBG_CREDIT, " credits        : %d\n",
		   ep_dist->credits);
	ath6kl_dbg(ATH6KL_DBG_CREDIT, " cred_assngd    : %d\n",
		   ep_dist->cred_assngd);
	ath6kl_dbg(ATH6KL_DBG_CREDIT, " seek_cred      : %d\n",
		   ep_dist->seek_cred);
	ath6kl_dbg(ATH6KL_DBG_CREDIT, " cred_sz        : %d\n",
		   ep_dist->cred_sz);
	ath6kl_dbg(ATH6KL_DBG_CREDIT, " cred_per_msg   : %d\n",
		   ep_dist->cred_per_msg);
	ath6kl_dbg(ATH6KL_DBG_CREDIT, " cred_to_dist   : %d\n",
		   ep_dist->cred_to_dist);
	ath6kl_dbg(ATH6KL_DBG_CREDIT, " cred_alloc_max : %d\n",
		   ep_dist->cred_alloc_max);
	ath6kl_dbg(ATH6KL_DBG_CREDIT, " txq_depth      : %d\n",
		   get_queue_depth(&ep_dist->htc_ep->txq));
	ath6kl_dbg(ATH6KL_DBG_CREDIT,
		   "----------------------------------\n");
}

/* FIXME: move to htc.c */
void dump_cred_dist_stats(struct htc_target *target)
{
	struct htc_endpoint_credit_dist *ep_list;

	if (!AR_DBG_LVL_CHECK(ATH6KL_DBG_CREDIT))
		return;

	list_for_each_entry(ep_list, &target->cred_dist_list, list)
		dump_cred_dist(ep_list);

	ath6kl_dbg(ATH6KL_DBG_CREDIT,
		   "credit distribution total %d free %d\n",
		   target->credit_info->total_avail_credits,
		   target->credit_info->cur_free_credits);
}

static int ath6kl_debugfs_open(struct inode *inode, struct file *file)
{
	file->private_data = inode->i_private;
	return 0;
}

void ath6kl_debug_war(struct ath6kl *ar, enum ath6kl_war war)
{
	switch (war) {
	case ATH6KL_WAR_INVALID_RATE:
		ar->debug.war_stats.invalid_rate++;
		break;
	}
}

static ssize_t read_file_war_stats(struct file *file, char __user *user_buf,
				   size_t count, loff_t *ppos)
{
	struct ath6kl *ar = file->private_data;
	char *buf;
	unsigned int len = 0, buf_len = 1500;
	ssize_t ret_cnt;

	buf = kzalloc(buf_len, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	len += scnprintf(buf + len, buf_len - len, "\n");
	len += scnprintf(buf + len, buf_len - len, "%25s\n",
			 "Workaround stats");
	len += scnprintf(buf + len, buf_len - len, "%25s\n\n",
			 "=================");
	len += scnprintf(buf + len, buf_len - len, "%20s %10u\n",
			 "Invalid rates", ar->debug.war_stats.invalid_rate);

	if (WARN_ON(len > buf_len))
		len = buf_len;

	ret_cnt = simple_read_from_buffer(user_buf, count, ppos, buf, len);

	kfree(buf);
	return ret_cnt;
}

static const struct file_operations fops_war_stats = {
	.read = read_file_war_stats,
	.open = ath6kl_debugfs_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};


static u32
ath6kl_fwlog_fragment(const u8 *datap, u32 len, u32 limit)
{
	u32 *buffer;
	u32 count;
	u32 numargs;
	u32 length;
	u32 fraglen;

	count = fraglen = 0;
	buffer = (u32 *)datap;
	length = (limit >> 2);

	if (len <= limit) {
		fraglen = len;
	} else {
		while (count < length) {
			numargs = ATH6KL_FWGLOG_GET_NUMARGS(buffer[count]);
			fraglen = (count << 2);
			count += numargs + 1;
		}
	}

	return fraglen;
}


static void
ath6kl_debug_fwlog_event_send(struct ath6kl *ar, const u8 *buffer, u32 length)
{
#define MAX_WIRELESS_EVENT_SIZE 252

	/*
	 * Break it up into chunks of MAX_WIRELESS_EVENT_SIZE bytes of messages.
	 * There seems to be a limitation on the length of message that could be
	 * transmitted to the user app via this mechanism.
	 */
	u32 send, sent;
	struct ath6kl_vif *vif;
	struct net_device *dev;

	vif = ath6kl_vif_first(ar);

	/* should always get value by ath6kl_vif_first */
	if (!vif)
		return;

	dev = vif->ndev;

	sent = 0;
	send = ath6kl_fwlog_fragment(&buffer[sent], length - sent,
			MAX_WIRELESS_EVENT_SIZE);
	while (send) {
		ath6kl_send_event_to_app(dev, WMIX_DBGLOG_EVENTID,
			vif->fw_vif_idx, (u8 *)&buffer[sent], send);
		sent += send;
		send = ath6kl_fwlog_fragment(&buffer[sent], length - sent,
				MAX_WIRELESS_EVENT_SIZE);
	}
}


void ath6kl_debug_fwlog_event(struct ath6kl *ar, const void *buf, size_t len)
{
	struct ath6kl_fwlog_slot *slot;
	struct sk_buff *skb;
	size_t slot_len;

	if (WARN_ON(len > ATH6KL_FWLOG_PAYLOAD_SIZE))
		return;

	if (ath6kl_mod_debug_quirks(ar, ATH6KL_MODULE_ENABLE_FWLOG_EXT))
		ath6kl_debug_fwlog_event_send(ar, buf, len);

	slot_len = sizeof(*slot) + ATH6KL_FWLOG_PAYLOAD_SIZE;

	skb = alloc_skb(slot_len, GFP_KERNEL);
	if (!skb)
		return;

	slot = (struct ath6kl_fwlog_slot *) skb_put(skb, slot_len);

	slot->timestamp = cpu_to_le32(jiffies);
	slot->length = cpu_to_le32(len);
	memcpy(slot->payload, buf, len);

	/* Need to pad each record to fixed length ATH6KL_FWLOG_PAYLOAD_SIZE */
	memset(slot->payload + len, 0, ATH6KL_FWLOG_PAYLOAD_SIZE - len);

	spin_lock(&ar->debug.fwlog_queue.lock);

	__skb_queue_tail(&ar->debug.fwlog_queue, skb);
	complete(&ar->debug.fwlog_completion);

       /* drop oldest entries */
	while (skb_queue_len(&ar->debug.fwlog_queue) >
		ATH6KL_FWLOG_MAX_ENTRIES) {
		skb = __skb_dequeue(&ar->debug.fwlog_queue);
		kfree_skb(skb);
	}

	spin_unlock(&ar->debug.fwlog_queue.lock);

}

static int ath6kl_fwlog_open(struct inode *inode, struct file *file)
{
	struct ath6kl *ar = inode->i_private;

	if (ar->debug.fwlog_open)
		return -EBUSY;

	ar->debug.fwlog_open = true;

	file->private_data = inode->i_private;
	return 0;
}

static int ath6kl_fwlog_release(struct inode *inode, struct file *file)
{
	struct ath6kl *ar = inode->i_private;

	ar->debug.fwlog_open = false;

	return 0;
}


static ssize_t ath6kl_fwlog_read(struct file *file, char __user *user_buf,
				 size_t count, loff_t *ppos)
{
	struct ath6kl *ar = file->private_data;
	struct sk_buff *skb;
	size_t len = 0;

	ssize_t ret_cnt;
	char *buf;

	buf = vmalloc(count);
	if (!buf)
		return -ENOMEM;

	/* read undelivered logs from firmware */
	ath6kl_read_fwlogs(ar);

	spin_lock(&ar->debug.fwlog_queue.lock);

	while ((skb = __skb_dequeue(&ar->debug.fwlog_queue))) {
		if (skb->len > count - len) {
			/* not enough space, put skb back and leave */
			__skb_queue_head(&ar->debug.fwlog_queue, skb);
			break;
		}

		memcpy(buf + len, skb->data, skb->len);
		len += skb->len;

		kfree_skb(skb);
	}

	spin_unlock(&ar->debug.fwlog_queue.lock);

	/* FIXME: what to do if len == 0? */
	ret_cnt = simple_read_from_buffer(user_buf, count, ppos, buf, len);

	vfree(buf);

	return ret_cnt;
}

static const struct file_operations fops_fwlog = {
	.open = ath6kl_fwlog_open,
	.read = ath6kl_fwlog_read,
	.release = ath6kl_fwlog_release,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

static ssize_t ath6kl_fwlog_block_read(struct file *file,
		char __user *user_buf,
		size_t count,
		loff_t *ppos)
{
	struct ath6kl *ar = file->private_data;
	struct sk_buff *skb;
	ssize_t ret_cnt;
	size_t len = 0, not_copied;
	char *buf;
	int ret;

	buf = vmalloc(count);
	if (!buf)
		return -ENOMEM;

	spin_lock(&ar->debug.fwlog_queue.lock);

	if (skb_queue_len(&ar->debug.fwlog_queue) == 0) {
		/* we must init under queue lock */
		init_completion(&ar->debug.fwlog_completion);

		spin_unlock(&ar->debug.fwlog_queue.lock);

		ret = wait_for_completion_interruptible(
				&ar->debug.fwlog_completion);
		if (ret == -ERESTARTSYS) {
			vfree(buf);
			return ret;
		}

		spin_lock(&ar->debug.fwlog_queue.lock);
	}

	while ((skb = __skb_dequeue(&ar->debug.fwlog_queue))) {
		if (skb->len > count - len) {
			/* not enough space, put skb back and leave */
			__skb_queue_head(&ar->debug.fwlog_queue, skb);
			break;
		}


		memcpy(buf + len, skb->data, skb->len);
		len += skb->len;

		kfree_skb(skb);
	}

	spin_unlock(&ar->debug.fwlog_queue.lock);

	/* FIXME: what to do if len == 0? */
	not_copied = copy_to_user(user_buf, buf, len);
	if (not_copied != 0) {
		ret_cnt = -EFAULT;
		goto out;
	}

	*ppos = *ppos + len;

	ret_cnt = len;

out:
	vfree(buf);

	return ret_cnt;
}

static const struct file_operations fops_fwlog_block = {
	.open = ath6kl_fwlog_open,
	.release = ath6kl_fwlog_release,
	.read = ath6kl_fwlog_block_read,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};


static ssize_t ath6kl_fwlog_mask_read(struct file *file, char __user *user_buf,
				      size_t count, loff_t *ppos)
{
	struct ath6kl *ar = file->private_data;
	char buf[16];
	int len;

	len = snprintf(buf, sizeof(buf), "0x%x\n", ar->debug.fwlog_mask);

	return simple_read_from_buffer(user_buf, count, ppos, buf, len);
}

static ssize_t ath6kl_fwlog_mask_write(struct file *file,
				       const char __user *user_buf,
				       size_t count, loff_t *ppos)
{
	struct ath6kl *ar = file->private_data;
	int ret;

	ret = kstrtou32_from_user(user_buf, count, 0, &ar->debug.fwlog_mask);
	if (ret)
		return ret;

	ret = ath6kl_wmi_config_debug_module_cmd(ar->wmi,
						 ATH6KL_FWLOG_VALID_MASK,
						 ar->debug.fwlog_mask);
	if (ret)
		return ret;

	return count;
}

static const struct file_operations fops_fwlog_mask = {
	.open = ath6kl_debugfs_open,
	.read = ath6kl_fwlog_mask_read,
	.write = ath6kl_fwlog_mask_write,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

static ssize_t read_file_tgt_stats(struct file *file, char __user *user_buf,
				   size_t count, loff_t *ppos)
{
	struct ath6kl *ar = file->private_data;
	struct ath6kl_vif *vif;
	struct target_stats *tgt_stats;
	char *buf;
	unsigned int len = 0, buf_len = 2000;
	int i;
	long left;
	ssize_t ret_cnt;

	vif = ath6kl_vif_first(ar);
	if (!vif)
		return -EIO;

	tgt_stats = &vif->target_stats;

	buf = kzalloc(buf_len, GFP_KERNEL);
	if (!buf) {
		kfree(buf);
		return -ENOMEM;
	}

	if (down_interruptible(&ar->sem)) {
		kfree(buf);
		return -EBUSY;
	}

	set_bit(STATS_UPDATE_PEND, &vif->flags);

	if (ath6kl_wmi_get_stats_cmd(ar->wmi, 0)) {
		up(&ar->sem);
		kfree(buf);
		return -EIO;
	}

	left = wait_event_interruptible_timeout(ar->event_wq,
						!test_bit(STATS_UPDATE_PEND,
						&vif->flags), WMI_TIMEOUT);

	up(&ar->sem);

	if (left <= 0) {
		kfree(buf);
		return -ETIMEDOUT;
	}

	len += scnprintf(buf + len, buf_len - len, "\n");
	len += scnprintf(buf + len, buf_len - len, "%25s\n",
			 "Target Tx stats");
	len += scnprintf(buf + len, buf_len - len, "%25s\n\n",
			 "=================");
	len += scnprintf(buf + len, buf_len - len, "%20s %10llu\n",
			 "Ucast packets", tgt_stats->tx_ucast_pkt);
	len += scnprintf(buf + len, buf_len - len, "%20s %10llu\n",
			 "Bcast packets", tgt_stats->tx_bcast_pkt);
	len += scnprintf(buf + len, buf_len - len, "%20s %10llu\n",
			 "Mcast packets", tgt_stats->tx_mcast_pkt);
	len += scnprintf(buf + len, buf_len - len, "%20s %10llu\n",
			 "Ucast byte", tgt_stats->tx_ucast_byte);
	len += scnprintf(buf + len, buf_len - len, "%20s %10llu\n",
			 "Bcast byte", tgt_stats->tx_bcast_byte);
	len += scnprintf(buf + len, buf_len - len, "%20s %10llu\n",
			 "Mcast byte", tgt_stats->tx_mcast_byte);
	len += scnprintf(buf + len, buf_len - len, "%20s %10llu\n",
			 "Rts success cnt", tgt_stats->tx_rts_success_cnt);
	for (i = 0; i < 4; i++)
		len += scnprintf(buf + len, buf_len - len,
				 "%18s %d %10llu\n", "PER on ac",
				 i, tgt_stats->tx_pkt_per_ac[i]);
	len += scnprintf(buf + len, buf_len - len, "%20s %10llu\n",
			 "Error", tgt_stats->tx_err);
	len += scnprintf(buf + len, buf_len - len, "%20s %10llu\n",
			 "Fail count", tgt_stats->tx_fail_cnt);
	len += scnprintf(buf + len, buf_len - len, "%20s %10llu\n",
			 "Retry count", tgt_stats->tx_retry_cnt);
	len += scnprintf(buf + len, buf_len - len, "%20s %10llu\n",
			 "Multi retry cnt", tgt_stats->tx_mult_retry_cnt);
	len += scnprintf(buf + len, buf_len - len, "%20s %10llu\n",
			 "Rts fail cnt", tgt_stats->tx_rts_fail_cnt);
	len += scnprintf(buf + len, buf_len - len, "%25s %10llu\n\n",
			 "TKIP counter measure used",
			 tgt_stats->tkip_cnter_measures_invoked);

	len += scnprintf(buf + len, buf_len - len, "%25s\n",
			 "Target Rx stats");
	len += scnprintf(buf + len, buf_len - len, "%25s\n",
			 "=================");

	len += scnprintf(buf + len, buf_len - len, "%20s %10llu\n",
			 "Ucast packets", tgt_stats->rx_ucast_pkt);
	len += scnprintf(buf + len, buf_len - len, "%20s %10d\n",
			 "Ucast Rate", tgt_stats->rx_ucast_rate);
	len += scnprintf(buf + len, buf_len - len, "%20s %10llu\n",
			 "Bcast packets", tgt_stats->rx_bcast_pkt);
	len += scnprintf(buf + len, buf_len - len, "%20s %10llu\n",
			 "Mcast packets", tgt_stats->rx_mcast_pkt);
	len += scnprintf(buf + len, buf_len - len, "%20s %10llu\n",
			 "Ucast byte", tgt_stats->rx_ucast_byte);
	len += scnprintf(buf + len, buf_len - len, "%20s %10llu\n",
			 "Bcast byte", tgt_stats->rx_bcast_byte);
	len += scnprintf(buf + len, buf_len - len, "%20s %10llu\n",
			 "Mcast byte", tgt_stats->rx_mcast_byte);
	len += scnprintf(buf + len, buf_len - len, "%20s %10llu\n",
			 "Fragmented pkt", tgt_stats->rx_frgment_pkt);
	len += scnprintf(buf + len, buf_len - len, "%20s %10llu\n",
			 "Error", tgt_stats->rx_err);
	len += scnprintf(buf + len, buf_len - len, "%20s %10llu\n",
			 "CRC Err", tgt_stats->rx_crc_err);
	len += scnprintf(buf + len, buf_len - len, "%20s %10llu\n",
			 "Key chache miss", tgt_stats->rx_key_cache_miss);
	len += scnprintf(buf + len, buf_len - len, "%20s %10llu\n",
			 "Decrypt Err", tgt_stats->rx_decrypt_err);
	len += scnprintf(buf + len, buf_len - len, "%20s %10llu\n",
			 "Duplicate frame", tgt_stats->rx_dupl_frame);
	len += scnprintf(buf + len, buf_len - len, "%20s %10llu\n",
			 "Tkip Mic failure", tgt_stats->tkip_local_mic_fail);
	len += scnprintf(buf + len, buf_len - len, "%20s %10llu\n",
			 "TKIP format err", tgt_stats->tkip_fmt_err);
	len += scnprintf(buf + len, buf_len - len, "%20s %10llu\n",
			 "CCMP format Err", tgt_stats->ccmp_fmt_err);
	len += scnprintf(buf + len, buf_len - len, "%20s %10llu\n\n",
			 "CCMP Replay Err", tgt_stats->ccmp_replays);

	len += scnprintf(buf + len, buf_len - len, "%25s\n",
			 "Misc Target stats");
	len += scnprintf(buf + len, buf_len - len, "%25s\n",
			 "=================");
	len += scnprintf(buf + len, buf_len - len, "%20s %10llu\n",
			 "Beacon Miss count", tgt_stats->cs_bmiss_cnt);
	len += scnprintf(buf + len, buf_len - len, "%20s %10llu\n",
			 "Num Connects", tgt_stats->cs_connect_cnt);
	len += scnprintf(buf + len, buf_len - len, "%20s %10llu\n",
			 "Num disconnects", tgt_stats->cs_discon_cnt);
	len += scnprintf(buf + len, buf_len - len, "%20s %10d\n",
			 "Beacon avg rssi", tgt_stats->cs_ave_beacon_rssi);

	len += scnprintf(buf + len, buf_len - len, "%25s\n",
			 "Wow stats");
	len += scnprintf(buf + len, buf_len - len, "%25s\n",
			 "=================");
	len += scnprintf(buf + len, buf_len - len, "%20s %10u\n",
			 "Wow pkt dropped", tgt_stats->wow_pkt_dropped);
	len += scnprintf(buf + len, buf_len - len, "%20s %10u\n",
			 "Wow evt discarded", tgt_stats->wow_evt_discarded);
	len += scnprintf(buf + len, buf_len - len, "%20s %10u\n",
		 "Wow host pkt wakeup", tgt_stats->wow_host_pkt_wakeups);
	len += scnprintf(buf + len, buf_len - len, "%20s %10u\n",
		 "Wow host evt wakeups", tgt_stats->wow_host_evt_wakeups);
#ifdef CONFIG_RSSI_STATS
	/* Per Chain RSSI */
	len += scnprintf(buf + len, buf_len - len, "%25s\n",
			 "Rssi stats");
	len += scnprintf(buf + len, buf_len - len, "%25s\n",
			 "=================");	
	len += scnprintf(buf + len, buf_len - len, "%20s %10d\n",
		 "Rx Rssi", tgt_stats->rx_rssi);
	len += scnprintf(buf + len, buf_len - len, "%20s %10d\n",
		 "Rx Rssi_Ctl0", tgt_stats->rx_rssi_ctl0);
	len += scnprintf(buf + len, buf_len - len, "%20s %10d\n",
		 "Rx Rssi_Ext0", tgt_stats->rx_rssi_ext0);
	len += scnprintf(buf + len, buf_len - len, "%20s %10d\n",
		 "Rx Rssi_Ctl1", tgt_stats->rx_rssi_ctl1);
	len += scnprintf(buf + len, buf_len - len, "%20s %10d\n",
		 "Rx Rssi_Ext1", tgt_stats->rx_rssi_ext1);

	len += scnprintf(buf + len, buf_len - len, "%20s %10d\n",
		 "Tx Rssi", tgt_stats->tx_rssi);
	len += scnprintf(buf + len, buf_len - len, "%20s %10d\n",
		 "Tx Rssi_Ctl0", tgt_stats->tx_rssi_ctl0);
	len += scnprintf(buf + len, buf_len - len, "%20s %10d\n",
		 "Tx Rssi_Ext0", tgt_stats->tx_rssi_ext0);
	len += scnprintf(buf + len, buf_len - len, "%20s %10d\n",
		 "Tx Rssi_Ctl1", tgt_stats->tx_rssi_ctl1);
	len += scnprintf(buf + len, buf_len - len, "%20s %10d\n",
		 "Tx Rssi_Ext1", tgt_stats->tx_rssi_ext1);
#endif
	if (len > buf_len)
		len = buf_len;

	ret_cnt = simple_read_from_buffer(user_buf, count, ppos, buf, len);

	kfree(buf);
	return ret_cnt;
}

static const struct file_operations fops_tgt_stats = {
	.read = read_file_tgt_stats,
	.open = ath6kl_debugfs_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

#define print_credit_info(fmt_str, ep_list_field)		\
	(len += scnprintf(buf + len, buf_len - len, fmt_str,	\
			 ep_list->ep_list_field))
#define CREDIT_INFO_DISPLAY_STRING_LEN	200
#define CREDIT_INFO_LEN	128

static ssize_t read_file_credit_dist_stats(struct file *file,
					   char __user *user_buf,
					   size_t count, loff_t *ppos)
{
	struct ath6kl *ar = file->private_data;
	struct htc_target *target = ar->htc_target;
	struct htc_endpoint_credit_dist *ep_list;
	char *buf;
	unsigned int buf_len, len = 0;
	ssize_t ret_cnt;
	struct list_head *list_head = &target->cred_dist_list;

	if (WARN_ON(list_head->next == NULL))
		return -EFAULT;

	buf_len = CREDIT_INFO_DISPLAY_STRING_LEN +
		  get_queue_depth(&target->cred_dist_list) * CREDIT_INFO_LEN;
	buf = kzalloc(buf_len, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	len += scnprintf(buf + len, buf_len - len, "%25s%5d\n",
			 "Total Avail Credits: ",
			 target->credit_info->total_avail_credits);
	len += scnprintf(buf + len, buf_len - len, "%25s%5d\n",
			 "Free credits :",
			 target->credit_info->cur_free_credits);

	len += scnprintf(buf + len, buf_len - len,
			 " Epid  Flags    Cred_norm  Cred_min  Credits  Cred_assngd"
			 "  Seek_cred  Cred_sz  Cred_per_msg  Cred_to_dist"
			 "  Cred_alloc_max qdepth\n");

	list_for_each_entry(ep_list, &target->cred_dist_list, list) {
		print_credit_info("  %2d", endpoint);
		print_credit_info("%10x", dist_flags);
		print_credit_info("%8d", cred_norm);
		print_credit_info("%9d", cred_min);
		print_credit_info("%9d", credits);
		print_credit_info("%10d", cred_assngd);
		print_credit_info("%13d", seek_cred);
		print_credit_info("%12d", cred_sz);
		print_credit_info("%9d", cred_per_msg);
		print_credit_info("%14d", cred_to_dist);
		print_credit_info("%17d", cred_alloc_max);
		len += scnprintf(buf + len, buf_len - len, "%12d\n",
				 get_queue_depth(&ep_list->htc_ep->txq));
	}

	if (len > buf_len)
		len = buf_len;

	ret_cnt = simple_read_from_buffer(user_buf, count, ppos, buf, len);
	kfree(buf);
	return ret_cnt;
}

static const struct file_operations fops_credit_dist_stats = {
	.read = read_file_credit_dist_stats,
	.open = ath6kl_debugfs_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

static unsigned int print_endpoint_stat(struct htc_target *target, char *buf,
					unsigned int buf_len, unsigned int len,
					int offset, const char *name)
{
	int i;
	struct htc_endpoint_stats *ep_st;
	u32 *counter;

	len += scnprintf(buf + len, buf_len - len, "%s:", name);
	for (i = 0; i < ENDPOINT_MAX; i++) {
		ep_st = &target->endpoint[i].ep_st;
		counter = ((u32 *) ep_st) + (offset / 4);
		len += scnprintf(buf + len, buf_len - len, " %u", *counter);
	}
	len += scnprintf(buf + len, buf_len - len, "\n");

	return len;
}

static ssize_t ath6kl_endpoint_stats_read(struct file *file,
					  char __user *user_buf,
					  size_t count, loff_t *ppos)
{
	struct ath6kl *ar = file->private_data;
	struct htc_target *target = ar->htc_target;
	char *buf;
	unsigned int buf_len, len = 0;
	ssize_t ret_cnt;

	buf_len = sizeof(struct htc_endpoint_stats) / sizeof(u32) *
		(25 + ENDPOINT_MAX * 11);
	buf = kmalloc(buf_len, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

#define EPSTAT(name)							\
	len = print_endpoint_stat(target, buf, buf_len, len,		\
				  offsetof(struct htc_endpoint_stats, name), \
				  #name)
	EPSTAT(cred_low_indicate);
	EPSTAT(tx_issued);
	EPSTAT(tx_pkt_bundled);
	EPSTAT(tx_bundles);
	EPSTAT(tx_dropped);
	EPSTAT(tx_cred_rpt);
	EPSTAT(cred_rpt_from_rx);
	EPSTAT(cred_rpt_from_other);
	EPSTAT(cred_rpt_ep0);
	EPSTAT(cred_from_rx);
	EPSTAT(cred_from_other);
	EPSTAT(cred_from_ep0);
	EPSTAT(cred_cosumd);
	EPSTAT(cred_retnd);
	EPSTAT(rx_pkts);
	EPSTAT(rx_lkahds);
	EPSTAT(rx_bundl);
	EPSTAT(rx_bundle_lkahd);
	EPSTAT(rx_bundle_from_hdr);
	EPSTAT(rx_alloc_thresh_hit);
	EPSTAT(rxalloc_thresh_byte);
#undef EPSTAT

	if (len > buf_len)
		len = buf_len;

	ret_cnt = simple_read_from_buffer(user_buf, count, ppos, buf, len);
	kfree(buf);
	return ret_cnt;
}

static ssize_t ath6kl_endpoint_stats_write(struct file *file,
					   const char __user *user_buf,
					   size_t count, loff_t *ppos)
{
	struct ath6kl *ar = file->private_data;
	struct htc_target *target = ar->htc_target;
	int ret, i;
	u32 val;
	struct htc_endpoint_stats *ep_st;

	ret = kstrtou32_from_user(user_buf, count, 0, &val);
	if (ret)
		return ret;
	if (val == 0) {
		for (i = 0; i < ENDPOINT_MAX; i++) {
			ep_st = &target->endpoint[i].ep_st;
			memset(ep_st, 0, sizeof(*ep_st));
		}
	}

	return count;
}

static const struct file_operations fops_endpoint_stats = {
	.open = ath6kl_debugfs_open,
	.read = ath6kl_endpoint_stats_read,
	.write = ath6kl_endpoint_stats_write,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

static unsigned long ath6kl_get_num_reg(void)
{
	int i;
	unsigned long n_reg = 0;

	for (i = 0; i < ARRAY_SIZE(diag_reg); i++)
		n_reg = n_reg +
		     (diag_reg[i].reg_end - diag_reg[i].reg_start) / 4 + 1;

	return n_reg;
}

static bool ath6kl_dbg_is_diag_reg_valid(u32 reg_addr)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(diag_reg); i++) {
		if (reg_addr >= diag_reg[i].reg_start &&
		    reg_addr <= diag_reg[i].reg_end)
			return true;
	}

	return true;
}

static ssize_t ath6kl_regread_read(struct file *file, char __user *user_buf,
				    size_t count, loff_t *ppos)
{
	struct ath6kl *ar = file->private_data;
	u8 buf[50];
	unsigned int len = 0;

	if (ar->debug.dbgfs_diag_reg)
		len += scnprintf(buf + len, sizeof(buf) - len, "0x%x\n",
				ar->debug.dbgfs_diag_reg);
	else
		len += scnprintf(buf + len, sizeof(buf) - len,
				 "All diag registers\n");

	return simple_read_from_buffer(user_buf, count, ppos, buf, len);
}

static ssize_t ath6kl_regread_write(struct file *file,
				    const char __user *user_buf,
				    size_t count, loff_t *ppos)
{
	struct ath6kl *ar = file->private_data;
	u8 buf[50];
	unsigned int len;
	unsigned long reg_addr;

	len = min(count, sizeof(buf) - 1);
	if (copy_from_user(buf, user_buf, len))
		return -EFAULT;

	buf[len] = '\0';

	if (kstrtoul(buf, 0, &reg_addr))
		return -EINVAL;

	if ((reg_addr % 4) != 0)
		return -EINVAL;

	if (reg_addr && !ath6kl_dbg_is_diag_reg_valid(reg_addr))
		return -EINVAL;

	ar->debug.dbgfs_diag_reg = reg_addr;

	return count;
}

static const struct file_operations fops_diag_reg_read = {
	.read = ath6kl_regread_read,
	.write = ath6kl_regread_write,
	.open = ath6kl_debugfs_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

static int ath6kl_regdump_open(struct inode *inode, struct file *file)
{
	struct ath6kl *ar = inode->i_private;
	u8 *buf;
	unsigned long int reg_len;
	unsigned int len = 0, n_reg;
	u32 addr;
	__le32 reg_val;
	int i, status;

	/* Dump all the registers if no register is specified */
	if (!ar->debug.dbgfs_diag_reg)
		n_reg = ath6kl_get_num_reg();
	else
		n_reg = 1;

	reg_len = n_reg * REG_OUTPUT_LEN_PER_LINE;
	if (n_reg > 1)
		reg_len += REGTYPE_STR_LEN;

	buf = vmalloc(reg_len);
	if (!buf)
		return -ENOMEM;

	if (n_reg == 1) {
		addr = ar->debug.dbgfs_diag_reg;

		status = ath6kl_diag_read32(ar,
				TARG_VTOP(ar->target_type, addr),
				(u32 *)&reg_val);
		if (status)
			goto fail_reg_read;

		len += scnprintf(buf + len, reg_len - len,
				 "0x%06x 0x%08x\n", addr, le32_to_cpu(reg_val));
		goto done;
	}

	for (i = 0; i < ARRAY_SIZE(diag_reg); i++) {
		len += scnprintf(buf + len, reg_len - len,
				"%s\n", diag_reg[i].reg_info);
		for (addr = diag_reg[i].reg_start;
		     addr <= diag_reg[i].reg_end; addr += 4) {
			status = ath6kl_diag_read32(ar,
					TARG_VTOP(ar->target_type, addr),
					(u32 *)&reg_val);
			if (status)
				goto fail_reg_read;

			len += scnprintf(buf + len, reg_len - len,
					"0x%06x 0x%08x\n",
					addr, le32_to_cpu(reg_val));
		}
	}

done:
	file->private_data = buf;
	return 0;

fail_reg_read:
	ath6kl_warn("Unable to read memory:%u\n", addr);
	vfree(buf);
	return -EIO;
}

static ssize_t ath6kl_regdump_read(struct file *file, char __user *user_buf,
				  size_t count, loff_t *ppos)
{
	u8 *buf = file->private_data;
	return simple_read_from_buffer(user_buf, count, ppos, buf, strlen(buf));
}

static int ath6kl_regdump_release(struct inode *inode, struct file *file)
{
	vfree(file->private_data);
	return 0;
}

static const struct file_operations fops_reg_dump = {
	.open = ath6kl_regdump_open,
	.read = ath6kl_regdump_read,
	.release = ath6kl_regdump_release,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

static int ath6kl_set_roam_params(const char __user *user_buf,
			size_t count,
			struct low_rssi_scan_params *low_rssi_roam_params)
{
	char buf[64];
	char *p;
	unsigned int len;
	int value;

	len = min(count, sizeof(buf) - 1);
	if (copy_from_user(buf, user_buf, len))
		return -EFAULT;

	p = buf;

	SKIP_SPACE;

	sscanf(p, "%d", &value);
	low_rssi_roam_params->lrssi_scan_period = (u16) value;

	SEEK_SPACE;
	SKIP_SPACE;

	sscanf(p, "%d", &value);
	low_rssi_roam_params->lrssi_scan_threshold = (u16) value;

	SEEK_SPACE;
	SKIP_SPACE;

	sscanf(p, "%d", &value);
	low_rssi_roam_params->lrssi_roam_threshold = (u16) value;

	SEEK_SPACE;
	SKIP_SPACE;

	sscanf(p, "%d", &value);
	low_rssi_roam_params->roam_rssi_floor = (u8) value;

	return 0;
}


static ssize_t ath6kl_lrssi_roam_write(struct file *file,
						const char __user *user_buf,
						size_t count,
						loff_t *ppos)
{
	struct ath6kl *ar = file->private_data;
	int ret;

	ret = ath6kl_set_roam_params(user_buf, count,
		&ar->low_rssi_roam_params);

	if (ret)
		return ret;

	ath6kl_wmi_set_roam_ctrl_cmd(ar->wmi,
			0,
			ar->low_rssi_roam_params.lrssi_scan_period,
			ar->low_rssi_roam_params.lrssi_scan_threshold,
			ar->low_rssi_roam_params.lrssi_roam_threshold,
			ar->low_rssi_roam_params.roam_rssi_floor);

	return count;
}

static ssize_t ath6kl_lrssi_roam_read(struct file *file,
				      char __user *user_buf,
				      size_t count, loff_t *ppos)
{
#define _BUF_SIZE	(128)
	struct ath6kl *ar = file->private_data;
	u8 *buf;
	unsigned int len = 0;
	ssize_t ret_cnt;

	buf = kmalloc(_BUF_SIZE, GFP_ATOMIC);
	if (!buf)
		return -ENOMEM;

	len += scnprintf(buf + len, _BUF_SIZE - len,
		"scan_period(%d ms, scan_th %d, roam_th %d, rssi_flo %d)\n ",
		ar->low_rssi_roam_params.lrssi_scan_period,
		ar->low_rssi_roam_params.lrssi_scan_threshold,
		ar->low_rssi_roam_params.lrssi_roam_threshold,
		ar->low_rssi_roam_params.roam_rssi_floor);

	ret_cnt = simple_read_from_buffer(user_buf, count, ppos, buf, len);

	kfree(buf);

	return ret_cnt;
#undef _BUF_SIZE
}

static const struct file_operations fops_lrssi_roam_param = {
	.read = ath6kl_lrssi_roam_read,
	.write = ath6kl_lrssi_roam_write,
	.open = ath6kl_debugfs_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

static ssize_t ath6kl_driver_version_read(struct file *file,
				      char __user *user_buf,
				      size_t count, loff_t *ppos)
{
	struct ath6kl *ar = file->private_data;
	char buf[64];
	unsigned int len;

	len = snprintf(buf, sizeof(buf), "%s\n", DRV_VERSION);
	len += snprintf(buf + len, sizeof(buf) - len, "%s %s\n",
				ar->hw.name,
				(ar->hif_type == ATH6KL_HIF_TYPE_SDIO) ?
				"SDIO" :
				 ((ar->hif_type == ATH6KL_HIF_TYPE_USB) ?
				 "USB" : "UNKNOW"));
	len += snprintf(buf + len, sizeof(buf) - len, "%s\n",
				ar->wiphy->fw_version);

	return simple_read_from_buffer(user_buf, count, ppos, buf, len);
}

static const struct file_operations fops_driver_version = {
	.read = ath6kl_driver_version_read,
	.open = ath6kl_debugfs_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

static ssize_t ath6kl_rx_drop_operation_read(struct file *file,
				      char __user *user_buf,
				      size_t count, loff_t *ppos)
{
	struct ath6kl *ar = file->private_data;
	char buf[64];
	unsigned int len;

	len = snprintf(buf, sizeof(buf), "RX aggregation drop packets %s\n",
			(ar->conf_flags & ATH6KL_CONF_DISABLE_RX_AGGR_DROP)
			? "disabled" : "enabled");

	return simple_read_from_buffer(user_buf, count, ppos, buf, len);
}

static ssize_t ath6kl_rx_drop_operation_write(struct file *file,
				      const char __user *user_buf,
				      size_t count, loff_t *ppos)
{
	struct ath6kl *ar = file->private_data;
	char buf[20];
	size_t len;

	len = min(count, sizeof(buf) - 1);
	if (copy_from_user(buf, user_buf, len))
		return -EFAULT;

	buf[len] = '\0';
	if (len > 0 && buf[len - 1] == '\n')
		buf[len - 1] = '\0';

	if (strcasecmp(buf, "enable") == 0)
		ar->conf_flags &=
			~ATH6KL_CONF_DISABLE_RX_AGGR_DROP;
	else if (strcasecmp(buf, "disable") == 0)
		ar->conf_flags |=
			ATH6KL_CONF_DISABLE_RX_AGGR_DROP;
	else
		return -EINVAL;

	return count;
}

static const struct file_operations fops_rx_drop_operation = {
	.read = ath6kl_rx_drop_operation_read,
	.write = ath6kl_rx_drop_operation_write,
	.open = ath6kl_debugfs_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

static ssize_t ath6kl_regwrite_read(struct file *file,
				    char __user *user_buf,
				    size_t count, loff_t *ppos)
{
	struct ath6kl *ar = file->private_data;
	u8 buf[32];
	unsigned int len = 0;

	len = scnprintf(buf, sizeof(buf), "Addr: 0x%x Val: 0x%x\n",
			ar->debug.diag_reg_addr_wr, ar->debug.diag_reg_val_wr);

	return simple_read_from_buffer(user_buf, count, ppos, buf, len);
}

static ssize_t ath6kl_regwrite_write(struct file *file,
				     const char __user *user_buf,
				     size_t count, loff_t *ppos)
{
	struct ath6kl *ar = file->private_data;
	char buf[32];
	char *sptr, *token;
	unsigned int len = 0;
	u32 reg_addr, reg_val;

	len = min(count, sizeof(buf) - 1);
	if (copy_from_user(buf, user_buf, len))
		return -EFAULT;

	buf[len] = '\0';
	sptr = buf;

	token = strsep(&sptr, "=");
	if (!token)
		return -EINVAL;

	if (kstrtou32(token, 0, &reg_addr))
		return -EINVAL;

	if (!ath6kl_dbg_is_diag_reg_valid(reg_addr))
		return -EINVAL;

	if (kstrtou32(sptr, 0, &reg_val))
		return -EINVAL;

	ar->debug.diag_reg_addr_wr = reg_addr;
	ar->debug.diag_reg_val_wr = reg_val;

	if (ath6kl_diag_write32(ar, ar->debug.diag_reg_addr_wr,
				cpu_to_le32(ar->debug.diag_reg_val_wr)))
		return -EIO;

	return count;
}

static const struct file_operations fops_diag_reg_write = {
	.read = ath6kl_regwrite_read,
	.write = ath6kl_regwrite_write,
	.open = ath6kl_debugfs_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

int ath6kl_debug_roam_tbl_event(struct ath6kl *ar, const void *buf,
				size_t len)
{
	const struct wmi_target_roam_tbl *tbl;
	u16 num_entries;

	if (len < sizeof(*tbl))
		return -EINVAL;

	tbl = (const struct wmi_target_roam_tbl *) buf;
	num_entries = le16_to_cpu(tbl->num_entries);
	if (sizeof(*tbl) + num_entries * sizeof(struct wmi_bss_roam_info) >
	    len)
		return -EINVAL;

	if (ar->debug.roam_tbl == NULL ||
	    ar->debug.roam_tbl_len < (unsigned int) len) {
		kfree(ar->debug.roam_tbl);
		ar->debug.roam_tbl = kmalloc(len, GFP_ATOMIC);
		if (ar->debug.roam_tbl == NULL)
			return -ENOMEM;
	}

	memcpy(ar->debug.roam_tbl, buf, len);
	ar->debug.roam_tbl_len = len;

	if (test_bit(ROAM_TBL_PEND, &ar->flag)) {
		clear_bit(ROAM_TBL_PEND, &ar->flag);
		wake_up(&ar->event_wq);
	}

	return 0;
}

static ssize_t ath6kl_roam_table_read(struct file *file, char __user *user_buf,
				      size_t count, loff_t *ppos)
{
	struct ath6kl *ar = file->private_data;
	int ret;
	long left;
	struct wmi_target_roam_tbl *tbl;
	u16 num_entries, i;
	char *buf;
	unsigned int len, buf_len;
	ssize_t ret_cnt;

	if (down_interruptible(&ar->sem))
		return -EBUSY;

	set_bit(ROAM_TBL_PEND, &ar->flag);

	ret = ath6kl_wmi_get_roam_tbl_cmd(ar->wmi);
	if (ret) {
		up(&ar->sem);
		return ret;
	}

	left = wait_event_interruptible_timeout(
		ar->event_wq, !test_bit(ROAM_TBL_PEND, &ar->flag), WMI_TIMEOUT);
	up(&ar->sem);

	if (left <= 0)
		return -ETIMEDOUT;

	if (ar->debug.roam_tbl == NULL)
		return -ENOMEM;

	tbl = (struct wmi_target_roam_tbl *) ar->debug.roam_tbl;
	num_entries = le16_to_cpu(tbl->num_entries);

	buf_len = 100 + num_entries * 100;
	buf = kzalloc(buf_len, GFP_KERNEL);
	if (buf == NULL)
		return -ENOMEM;
	len = 0;
	len += scnprintf(buf + len, buf_len - len,
			 "roam_mode=%u\n\n"
			 "# roam_util bssid rssi rssidt last_rssi util bias\n",
			 le16_to_cpu(tbl->roam_mode));

	for (i = 0; i < num_entries; i++) {
		struct wmi_bss_roam_info *info = &tbl->info[i];
		len += scnprintf(buf + len, buf_len - len,
				 "%d %pM %d %d %d %d %d\n",
				 a_sle32_to_cpu(info->roam_util), info->bssid,
				 info->rssi, info->rssidt, info->last_rssi,
				 info->util, info->bias);
	}

	if (len > buf_len)
		len = buf_len;

	ret_cnt = simple_read_from_buffer(user_buf, count, ppos, buf, len);

	kfree(buf);
	return ret_cnt;
}

static const struct file_operations fops_roam_table = {
	.read = ath6kl_roam_table_read,
	.open = ath6kl_debugfs_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

static ssize_t ath6kl_force_roam_write(struct file *file,
				       const char __user *user_buf,
				       size_t count, loff_t *ppos)
{
	struct ath6kl *ar = file->private_data;
	int ret;
	char buf[20];
	size_t len;
	u8 bssid[ETH_ALEN];
	int i;
	int addr[ETH_ALEN];

	len = min(count, sizeof(buf) - 1);
	if (copy_from_user(buf, user_buf, len))
		return -EFAULT;
	buf[len] = '\0';

	if (sscanf(buf, "%02x:%02x:%02x:%02x:%02x:%02x",
		   &addr[0], &addr[1], &addr[2], &addr[3], &addr[4], &addr[5])
	    != ETH_ALEN)
		return -EINVAL;
	for (i = 0; i < ETH_ALEN; i++)
		bssid[i] = addr[i];

	ret = ath6kl_wmi_force_roam_cmd(ar->wmi, bssid);
	if (ret)
		return ret;

	return count;
}

static const struct file_operations fops_force_roam = {
	.write = ath6kl_force_roam_write,
	.open = ath6kl_debugfs_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

static ssize_t ath6kl_roam_mode_read(struct file *file,
	char __user *user_buf, size_t count, loff_t *ppos)
{
	struct ath6kl *ar = file->private_data;
	u8 buf[32];
	unsigned int len = 0;
	char *str;

	if (ar->debug.roam_mode == WMI_DEFAULT_ROAM_MODE)
		str = "default";
	else if (ar->debug.roam_mode == WMI_HOST_BIAS_ROAM_MODE)
		str = "bssbias";
	else if (ar->debug.roam_mode == WMI_LOCK_BSS_MODE)
		str = "lock";
	else
		str = "NONE";

	len = scnprintf(buf, sizeof(buf), "roam mode: %s\n",
		str);

	return simple_read_from_buffer(user_buf, count, ppos, buf, len);
}

static ssize_t ath6kl_roam_mode_write(struct file *file,
				      const char __user *user_buf,
				      size_t count, loff_t *ppos)
{
	struct ath6kl *ar = file->private_data;
	int ret;
	char buf[20];
	size_t len;
	enum wmi_roam_mode mode;

	len = min(count, sizeof(buf) - 1);
	if (copy_from_user(buf, user_buf, len))
		return -EFAULT;
	buf[len] = '\0';
	if (len > 0 && buf[len - 1] == '\n')
		buf[len - 1] = '\0';

	if (strcasecmp(buf, "default") == 0)
		mode = WMI_DEFAULT_ROAM_MODE;
	else if (strcasecmp(buf, "bssbias") == 0)
		mode = WMI_HOST_BIAS_ROAM_MODE;
	else if (strcasecmp(buf, "lock") == 0)
		mode = WMI_LOCK_BSS_MODE;
	else
		return -EINVAL;

	ar->debug.roam_mode = mode;

	ret = ath6kl_wmi_set_roam_mode_cmd(ar->wmi, mode);
	if (ret)
		return ret;

	return count;
}

static const struct file_operations fops_roam_mode = {
	.read = ath6kl_roam_mode_read,
	.write = ath6kl_roam_mode_write,
	.open = ath6kl_debugfs_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

static ssize_t ath6kl_roam_5g_bias_read(struct file *file,
	char __user *user_buf, size_t count, loff_t *ppos)
{
	struct ath6kl *ar = file->private_data;
	u8 buf[32];
	unsigned int len = 0;

	len = scnprintf(buf, sizeof(buf), "roam 5g bias: %d\n",
		ar->debug.roam_5g_bias);

	return simple_read_from_buffer(user_buf, count, ppos, buf, len);
}

static ssize_t ath6kl_roam_5g_bias_write(struct file *file,
				      const char __user *user_buf,
				      size_t count, loff_t *ppos)
{
	struct ath6kl *ar = file->private_data;
	int ret;
	u8 bias_5g;

	ret = kstrtou8_from_user(user_buf, count, 0, &bias_5g);
	if (ret)
		return ret;

	ar->debug.roam_5g_bias = bias_5g;

	ret = ath6kl_wmi_set_roam_5g_bias_cmd(ar->wmi, bias_5g);
	if (ret)
		return ret;

	return count;
}

static const struct file_operations fops_roam_5g_bias = {
	.read = ath6kl_roam_5g_bias_read,
	.write = ath6kl_roam_5g_bias_write,
	.open = ath6kl_debugfs_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

void ath6kl_debug_set_keepalive(struct ath6kl *ar, u8 keepalive)
{
	ar->debug.keepalive = keepalive;
}

static ssize_t ath6kl_keepalive_read(struct file *file, char __user *user_buf,
				     size_t count, loff_t *ppos)
{
	struct ath6kl *ar = file->private_data;
	char buf[16];
	int len;

	len = snprintf(buf, sizeof(buf), "%u\n", ar->debug.keepalive);

	return simple_read_from_buffer(user_buf, count, ppos, buf, len);
}

static ssize_t ath6kl_keepalive_write(struct file *file,
				      const char __user *user_buf,
				      size_t count, loff_t *ppos)
{
	struct ath6kl *ar = file->private_data;
	int ret;
	u8 val;

	ret = kstrtou8_from_user(user_buf, count, 0, &val);
	if (ret)
		return ret;

	ret = ath6kl_wmi_set_keepalive_cmd(ar->wmi, 0, val);
	if (ret)
		return ret;

	return count;
}

static const struct file_operations fops_keepalive = {
	.open = ath6kl_debugfs_open,
	.read = ath6kl_keepalive_read,
	.write = ath6kl_keepalive_write,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

void ath6kl_debug_set_disconnect_timeout(struct ath6kl *ar, u8 timeout)
{
	ar->debug.disc_timeout = timeout;
}

static ssize_t ath6kl_disconnect_timeout_read(struct file *file,
					      char __user *user_buf,
					      size_t count, loff_t *ppos)
{
	struct ath6kl *ar = file->private_data;
	char buf[16];
	int len;

	len = snprintf(buf, sizeof(buf), "%u\n", ar->debug.disc_timeout);

	return simple_read_from_buffer(user_buf, count, ppos, buf, len);
}

static ssize_t ath6kl_disconnect_timeout_write(struct file *file,
					       const char __user *user_buf,
					       size_t count, loff_t *ppos)
{
	struct ath6kl *ar = file->private_data;
	int ret;
	u8 val;

	ret = kstrtou8_from_user(user_buf, count, 0, &val);
	if (ret)
		return ret;

	ret = ath6kl_wmi_disctimeout_cmd(ar->wmi, 0, val);
	if (ret)
		return ret;

	return count;
}

static const struct file_operations fops_disconnect_timeout = {
	.open = ath6kl_debugfs_open,
	.read = ath6kl_disconnect_timeout_read,
	.write = ath6kl_disconnect_timeout_write,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

static ssize_t ath6kl_create_qos_write(struct file *file,
						const char __user *user_buf,
						size_t count, loff_t *ppos)
{

	struct ath6kl *ar = file->private_data;
	struct ath6kl_vif *vif;
	char buf[200];
	ssize_t len;
	char *sptr, *token;
	struct wmi_create_pstream_cmd pstream;
	u32 val32;
	u16 val16;

	vif = ath6kl_vif_first(ar);
	if (!vif)
		return -EIO;

	memset(&pstream, 0x00, sizeof(struct wmi_create_pstream_cmd));
	len = min(count, sizeof(buf) - 1);
	if (copy_from_user(buf, user_buf, len))
		return -EFAULT;
	buf[len] = '\0';
	sptr = buf;

	token = strsep(&sptr, " ");
	if (!token)
		return -EINVAL;
	if (kstrtou8(token, 0, &pstream.user_pri))
		return -EINVAL;

	token = strsep(&sptr, " ");
	if (!token)
		return -EINVAL;
	if (kstrtou8(token, 0, &pstream.traffic_direc))
		return -EINVAL;

	token = strsep(&sptr, " ");
	if (!token)
		return -EINVAL;
	if (kstrtou8(token, 0, &pstream.traffic_class))
		return -EINVAL;

	token = strsep(&sptr, " ");
	if (!token)
		return -EINVAL;
	if (kstrtou8(token, 0, &pstream.traffic_type))
		return -EINVAL;

	token = strsep(&sptr, " ");
	if (!token)
		return -EINVAL;
	if (kstrtou8(token, 0, &pstream.voice_psc_cap))
		return -EINVAL;

	token = strsep(&sptr, " ");
	if (!token)
		return -EINVAL;
	if (kstrtou32(token, 0, &val32))
		return -EINVAL;
	pstream.min_service_int = cpu_to_le32(val32);

	token = strsep(&sptr, " ");
	if (!token)
		return -EINVAL;
	if (kstrtou32(token, 0, &val32))
		return -EINVAL;
	pstream.max_service_int = cpu_to_le32(val32);

	token = strsep(&sptr, " ");
	if (!token)
		return -EINVAL;
	if (kstrtou32(token, 0, &val32))
		return -EINVAL;
	pstream.inactivity_int = cpu_to_le32(val32);

	token = strsep(&sptr, " ");
	if (!token)
		return -EINVAL;
	if (kstrtou32(token, 0, &val32))
		return -EINVAL;
	pstream.suspension_int = cpu_to_le32(val32);

	token = strsep(&sptr, " ");
	if (!token)
		return -EINVAL;
	if (kstrtou32(token, 0, &val32))
		return -EINVAL;
	pstream.service_start_time = cpu_to_le32(val32);

	token = strsep(&sptr, " ");
	if (!token)
		return -EINVAL;
	if (kstrtou8(token, 0, &pstream.tsid))
		return -EINVAL;

	token = strsep(&sptr, " ");
	if (!token)
		return -EINVAL;
	if (kstrtou16(token, 0, &val16))
		return -EINVAL;
	pstream.nominal_msdu = cpu_to_le16(val16);

	token = strsep(&sptr, " ");
	if (!token)
		return -EINVAL;
	if (kstrtou16(token, 0, &val16))
		return -EINVAL;
	pstream.max_msdu = cpu_to_le16(val16);

	token = strsep(&sptr, " ");
	if (!token)
		return -EINVAL;
	if (kstrtou32(token, 0, &val32))
		return -EINVAL;
	pstream.min_data_rate = cpu_to_le32(val32);

	token = strsep(&sptr, " ");
	if (!token)
		return -EINVAL;
	if (kstrtou32(token, 0, &val32))
		return -EINVAL;
	pstream.mean_data_rate = cpu_to_le32(val32);

	token = strsep(&sptr, " ");
	if (!token)
		return -EINVAL;
	if (kstrtou32(token, 0, &val32))
		return -EINVAL;
	pstream.peak_data_rate = cpu_to_le32(val32);

	token = strsep(&sptr, " ");
	if (!token)
		return -EINVAL;
	if (kstrtou32(token, 0, &val32))
		return -EINVAL;
	pstream.max_burst_size = cpu_to_le32(val32);

	token = strsep(&sptr, " ");
	if (!token)
		return -EINVAL;
	if (kstrtou32(token, 0, &val32))
		return -EINVAL;
	pstream.delay_bound = cpu_to_le32(val32);

	token = strsep(&sptr, " ");
	if (!token)
		return -EINVAL;
	if (kstrtou32(token, 0, &val32))
		return -EINVAL;
	pstream.min_phy_rate = cpu_to_le32(val32);

	token = strsep(&sptr, " ");
	if (!token)
		return -EINVAL;
	if (kstrtou32(token, 0, &val32))
		return -EINVAL;
	pstream.sba = cpu_to_le32(val32);

	token = strsep(&sptr, " ");
	if (!token)
		return -EINVAL;
	if (kstrtou32(token, 0, &val32))
		return -EINVAL;
	pstream.medium_time = cpu_to_le32(val32);

	ath6kl_wmi_create_pstream_cmd(ar->wmi, vif->fw_vif_idx, &pstream);

	return count;
}

static const struct file_operations fops_create_qos = {
	.write = ath6kl_create_qos_write,
	.open = ath6kl_debugfs_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

static ssize_t ath6kl_delete_qos_write(struct file *file,
				const char __user *user_buf,
				size_t count, loff_t *ppos)
{

	struct ath6kl *ar = file->private_data;
	struct ath6kl_vif *vif;
	char buf[100];
	ssize_t len;
	char *sptr, *token;
	u8 traffic_class;
	u8 tsid;

	vif = ath6kl_vif_first(ar);
	if (!vif)
		return -EIO;

	len = min(count, sizeof(buf) - 1);
	if (copy_from_user(buf, user_buf, len))
		return -EFAULT;
	buf[len] = '\0';
	sptr = buf;

	token = strsep(&sptr, " ");
	if (!token)
		return -EINVAL;
	if (kstrtou8(token, 0, &traffic_class))
		return -EINVAL;

	token = strsep(&sptr, " ");
	if (!token)
		return -EINVAL;
	if (kstrtou8(token, 0, &tsid))
		return -EINVAL;

	ath6kl_wmi_delete_pstream_cmd(ar->wmi, vif->fw_vif_idx,
				      traffic_class, tsid);

	return count;
}

static const struct file_operations fops_delete_qos = {
	.write = ath6kl_delete_qos_write,
	.open = ath6kl_debugfs_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

static ssize_t ath6kl_bgscan_int_read(struct file *file,
	char __user *user_buf, size_t count, loff_t *ppos)
{
	struct ath6kl *ar = file->private_data;
	u8 buf[128];
	unsigned int len = 0;

	len = scnprintf(buf, sizeof(buf), "bgscan interval: %d\n",
		ar->debug.bgscan_int);

	return simple_read_from_buffer(user_buf, count, ppos, buf, len);
}

static ssize_t ath6kl_bgscan_int_write(struct file *file,
				const char __user *user_buf,
				size_t count, loff_t *ppos)
{
	struct ath6kl *ar = file->private_data;
	struct ath6kl_vif *vif;
	u16 bgscan_int;
	char buf[32];
	ssize_t len;

	vif = ath6kl_vif_first(ar);
	if (!vif)
		return -EIO;

	len = min(count, sizeof(buf) - 1);
	if (copy_from_user(buf, user_buf, len))
		return -EFAULT;

	buf[len] = '\0';
	if (kstrtou16(buf, 0, &bgscan_int))
		return -EINVAL;

	if (bgscan_int == 0)
		bgscan_int = 0xffff;

	ar->debug.bgscan_int = bgscan_int;

	ath6kl_wmi_scanparams_cmd(ar->wmi, 0,
				vif->sc_params.fg_start_period,
				vif->sc_params.fg_end_period,
				bgscan_int,
				vif->sc_params.minact_chdwell_time,
				vif->sc_params.maxact_chdwell_time,
				vif->sc_params.pas_chdwell_time,
				vif->sc_params.short_scan_ratio,
				vif->sc_params.scan_ctrl_flags,
				vif->sc_params.max_dfsch_act_time,
				vif->sc_params.maxact_scan_per_ssid);

	return count;
}

static const struct file_operations fops_bgscan_int = {
	.read = ath6kl_bgscan_int_read,
	.write = ath6kl_bgscan_int_write,
	.open = ath6kl_debugfs_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

static ssize_t ath6kl_listen_int_write(struct file *file,
						const char __user *user_buf,
						size_t count, loff_t *ppos)
{
	struct ath6kl *ar = file->private_data;
	u16 listen_int_t, listen_int_b;
	char buf[32];
	char *sptr, *token;
	ssize_t len;

	len = min(count, sizeof(buf) - 1);
	if (copy_from_user(buf, user_buf, len))
		return -EFAULT;

	buf[len] = '\0';
	sptr = buf;

	token = strsep(&sptr, " ");
	if (!token)
		return -EINVAL;

	if (kstrtou16(token, 0, &listen_int_t))
		return -EINVAL;

	if (kstrtou16(sptr, 0, &listen_int_b))
		return -EINVAL;

	if ((listen_int_t < 15) || (listen_int_t > 5000))
		return -EINVAL;

	if ((listen_int_b < 1) || (listen_int_b > 50))
		return -EINVAL;

	ar->listen_intvl_t = listen_int_t;
	ar->listen_intvl_b = listen_int_b;

	ath6kl_wmi_listeninterval_cmd(ar->wmi, 0, ar->listen_intvl_t,
				      ar->listen_intvl_b);

	return count;
}

static ssize_t ath6kl_listen_int_read(struct file *file,
						char __user *user_buf,
						size_t count, loff_t *ppos)
{
	struct ath6kl *ar = file->private_data;
	char buf[32];
	int len;

	len = scnprintf(buf, sizeof(buf), "%u %u\n", ar->listen_intvl_t,
					ar->listen_intvl_b);

	return simple_read_from_buffer(user_buf, count, ppos, buf, len);
}

static const struct file_operations fops_listen_int = {
	.read = ath6kl_listen_int_read,
	.write = ath6kl_listen_int_write,
	.open = ath6kl_debugfs_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

static ssize_t ath6kl_power_params_write(struct file *file,
						const char __user *user_buf,
						size_t count, loff_t *ppos)
{
	struct ath6kl *ar = file->private_data;
	u8 buf[100];
	unsigned int len = 0;
	char *sptr, *token;
	u16 idle_period, ps_poll_num, dtim,
		tx_wakeup, num_tx;

	len = min(count, sizeof(buf) - 1);
	if (copy_from_user(buf, user_buf, len))
		return -EFAULT;
	buf[len] = '\0';
	sptr = buf;

	token = strsep(&sptr, " ");
	if (!token)
		return -EINVAL;
	if (kstrtou16(token, 0, &idle_period))
		return -EINVAL;

	token = strsep(&sptr, " ");
	if (!token)
		return -EINVAL;
	if (kstrtou16(token, 0, &ps_poll_num))
		return -EINVAL;

	token = strsep(&sptr, " ");
	if (!token)
		return -EINVAL;
	if (kstrtou16(token, 0, &dtim))
		return -EINVAL;

	token = strsep(&sptr, " ");
	if (!token)
		return -EINVAL;
	if (kstrtou16(token, 0, &tx_wakeup))
		return -EINVAL;

	token = strsep(&sptr, " ");
	if (!token)
		return -EINVAL;
	if (kstrtou16(token, 0, &num_tx))
		return -EINVAL;

	ar->debug.power_params.idle_period = idle_period;
	ar->debug.power_params.ps_poll_num = ps_poll_num;
	ar->debug.power_params.dtim = dtim;
	ar->debug.power_params.tx_wakeup = tx_wakeup;
	ar->debug.power_params.num_tx = num_tx;

	ath6kl_wmi_pmparams_cmd(ar->wmi, 0, idle_period, ps_poll_num,
				dtim, tx_wakeup, num_tx, 0);

	return count;
}

static ssize_t ath6kl_power_params_read(struct file *file,
				char __user *user_buf,
				size_t count, loff_t *ppos)
{
	struct ath6kl *ar = file->private_data;
	u8 buf[256];
	unsigned int len = 0;

	len = scnprintf(buf, sizeof(buf),
			"idle_period: %d, ps_poll_num: %d, dtim: %d, tx_wakeup:%d , num_tx: %d\n",
			ar->debug.power_params.idle_period,
			ar->debug.power_params.ps_poll_num,
			ar->debug.power_params.dtim,
			ar->debug.power_params.tx_wakeup,
			ar->debug.power_params.num_tx);

	return simple_read_from_buffer(user_buf, count, ppos, buf, len);
}

static const struct file_operations fops_power_params = {
	.read = ath6kl_power_params_read,
	.write = ath6kl_power_params_write,
	.open = ath6kl_debugfs_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

static int ath6kl_parse_lpl_force_enable_params(const char __user *user_buf,
				size_t count,
				struct lpl_force_enable_param *param)
{
	char buf[64];
	char *p;
	unsigned int len;
	int value;

	len = min(count, sizeof(buf) - 1);
	if (copy_from_user(buf, user_buf, len))
		return -EFAULT;

	p = buf;

	SKIP_SPACE;

	sscanf(p, "%d", &value);
	param->lpl_policy = value;

	SEEK_SPACE;
	SKIP_SPACE;

	sscanf(p, "%d", &value);
	param->no_blocker_detect = value;

	SEEK_SPACE;
	SKIP_SPACE;

	sscanf(p, "%d", &value);
	param->no_rfb_detect = value;

	return 0;
}

/* File operation functions for low power listen */
static ssize_t ath6kl_lpl_force_enable_write(struct file *file,
				const char __user *user_buf,
				size_t count, loff_t *ppos)
{
	struct ath6kl *ar = file->private_data;
	int ret;

	ret = ath6kl_parse_lpl_force_enable_params(user_buf, count,
			&ar->debug.lpl_force_enable_params);
	if (ret)
		return ret;

	if (ath6kl_wmi_lpl_enable_cmd(ar->wmi,
			(struct wmi_lpl_force_enable_cmd *)
			&ar->debug.lpl_force_enable_params))
		return -EIO;

	return count;
}

static ssize_t ath6kl_lpl_force_enable_read(struct file *file,
				char __user *user_buf,
				size_t count, loff_t *ppos)
{
	struct ath6kl *ar = file->private_data;
	u8 buf[128];
	unsigned int len = 0;

	len = scnprintf(buf, sizeof(buf),
			"lplPolicy: %d, noBlockerDetect: %d, noRfbDetect: %d\n",
			ar->debug.lpl_force_enable_params.lpl_policy,
			ar->debug.lpl_force_enable_params.no_blocker_detect,
			ar->debug.lpl_force_enable_params.no_rfb_detect);

	return simple_read_from_buffer(user_buf, count, ppos, buf, len);
}

/* debug fs for low power listen */
static const struct file_operations fops_lpl_force_enable = {
	.read = ath6kl_lpl_force_enable_read,
	.write = ath6kl_lpl_force_enable_write,
	.open = ath6kl_debugfs_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

/* File operation functions for mimo power saving */
static ssize_t ath6kl_mimo_ps_enable_write(struct file *file,
				const char __user *user_buf,
				size_t count, loff_t *ppos)
{
	struct ath6kl *ar = file->private_data;
	int ret;

	ret = kstrtou8_from_user(user_buf, count, 0, &ar->debug.mimo_ps_enable);
	if (ret)
		return ret;

	if (ath6kl_wmi_smps_enable(ar->wmi,
		(struct wmi_config_enable_cmd *)&ar->debug.mimo_ps_enable))
		return -EIO;

	return count;
}

static ssize_t ath6kl_mimo_ps_enable_read(struct file *file,
				char __user *user_buf,
				size_t count, loff_t *ppos)
{
	struct ath6kl *ar = file->private_data;
	u8 buf[32];
	unsigned int len = 0;

	len = scnprintf(buf, sizeof(buf), "MIMO Power Saving: %d\n",
			ar->debug.mimo_ps_enable);

	return simple_read_from_buffer(user_buf, count, ppos, buf, len);
}

/* debug fs for mimo power saving */
static const struct file_operations fops_mimo_ps_enable = {
	.read = ath6kl_mimo_ps_enable_read,
	.write = ath6kl_mimo_ps_enable_write,
	.open = ath6kl_debugfs_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

static int ath6kl_parse_mimo_ps_params(const char __user *user_buf,
				size_t count,
				struct smps_param *param)
{
	char buf[64];
	char *p;
	unsigned int len;
	int value;

	len = min(count, sizeof(buf) - 1);
	if (copy_from_user(buf, user_buf, len))
		return -EFAULT;

	p = buf;

	SKIP_SPACE;

	sscanf(p, "%d", &value);

	if (value == 1)
		param->mode = WMI_SMPS_MODE_STATIC;
	else if (value == 2)
		param->mode = WMI_SMPS_MODE_DYNAMIC;
	else
		return -EINVAL;

	param->flags = WMI_SMPS_OPTION_MODE;

	SEEK_SPACE;
	SKIP_SPACE;

	sscanf(p, "%d", &value);
	param->automatic = value;

	if (value == 0 || value == 1)
		param->flags |= WMI_SMPS_OPTION_AUTO;

	SEEK_SPACE;
	SKIP_SPACE;

	sscanf(p, "%d", &value);

	if (value != -1) {
		param->data_thresh = value;
		param->flags |= WMI_SMPS_OPTION_DATATHRESH;
	}

	SEEK_SPACE;
	SKIP_SPACE;

	sscanf(p, "%d", &value);

	if (value != -1) {
		param->rssi_thresh = value;
		param->flags |= WMI_SMPS_OPTION_RSSITHRESH;
	}

	return 0;
}

/* File operation functions for mimo power saving */
static ssize_t ath6kl_mimo_ps_params_write(struct file *file,
				const char __user *user_buf,
				size_t count, loff_t *ppos)
{
	struct ath6kl *ar = file->private_data;
	int ret;

	ret = ath6kl_parse_mimo_ps_params(user_buf,
			count, &ar->debug.smps_params);
	if (ret)
		return ret;

	if (ath6kl_wmi_smps_config(ar->wmi,
			(struct wmi_config_smps_cmd *)&ar->debug.smps_params))
		return -EIO;

	return count;
}

static ssize_t ath6kl_mimo_ps_params_read(struct file *file,
				char __user *user_buf,
				size_t count, loff_t *ppos)
{
	struct ath6kl *ar = file->private_data;
	u8 buf[128];
	unsigned int len = 0;

	len = scnprintf(buf, sizeof(buf),
			"flags: %d, mode: %d, automatic: %d, dataThresh: %d, rssiThresh: %d\n",
			ar->debug.smps_params.flags,
			ar->debug.smps_params.mode,
			ar->debug.smps_params.automatic,
			ar->debug.smps_params.data_thresh,
			ar->debug.smps_params.rssi_thresh);

	return simple_read_from_buffer(user_buf, count, ppos, buf, len);
}

/* debug fs for mimo power saving */
static const struct file_operations fops_mimo_ps_params = {
	.read = ath6kl_mimo_ps_params_read,
	.write = ath6kl_mimo_ps_params_write,
	.open = ath6kl_debugfs_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};


/* File operation functions for Green Tx */
static ssize_t ath6kl_green_tx_write(struct file *file,
				const char __user *user_buf,
				size_t count, loff_t *ppos)
{
	struct ath6kl *ar = file->private_data;
	int ret;

	ret = kstrtou32_from_user(user_buf, count, 0,
			&ar->green_tx_params.enable);

	if (ret)
		return ret;

	if (ath6kl_wmi_set_green_tx_params(ar->wmi,
		(struct wmi_green_tx_params *) &ar->green_tx_params))
		return -EIO;

	return count;
}

static ssize_t ath6kl_green_tx_read(struct file *file,
				char __user *user_buf,
				size_t count, loff_t *ppos)
{
	struct ath6kl *ar = file->private_data;
	u8 buf[32];
	unsigned int len = 0;

	len = scnprintf(buf, sizeof(buf), "Green Tx: %d\n",
			ar->green_tx_params.enable);

	return simple_read_from_buffer(user_buf, count, ppos, buf, len);
}

/* debug fs for green tx */
static const struct file_operations fops_green_tx = {
	.read = ath6kl_green_tx_read,
	.write = ath6kl_green_tx_write,
	.open = ath6kl_debugfs_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

static int ath6kl_parse_green_tx_params(const char __user *user_buf,
				size_t count,
				struct wmi_green_tx_params *param)
{
	char buf[64];
	char *p;
	unsigned int len;
	int value;

	len = min(count, sizeof(buf) - 1);
	if (copy_from_user(buf, user_buf, len))
		return -EFAULT;

	p = buf;

	SKIP_SPACE;

	sscanf(p, "%d", &value);
	param->next_probe_count = value;

	SEEK_SPACE;
	SKIP_SPACE;

	sscanf(p, "%d", &value);
	param->max_back_off = value;

	SEEK_SPACE;
	SKIP_SPACE;

	sscanf(p, "%d", &value);
	param->min_gtx_rssi = value;

	SEEK_SPACE;
	SKIP_SPACE;

	sscanf(p, "%d", &value);
	param->force_back_off = value;

	return 0;
}

static ssize_t ath6kl_green_tx_params_write(struct file *file,
				const char __user *user_buf,
				size_t count, loff_t *ppos)
{
	struct ath6kl *ar = file->private_data;
	int ret;

	ret = ath6kl_parse_green_tx_params(user_buf,
			count, &ar->green_tx_params);

	if (ret)
		return ret;

	if (ath6kl_wmi_set_green_tx_params(ar->wmi,
		(struct wmi_green_tx_params *) &ar->green_tx_params))
		return -EIO;

	return count;
}

static ssize_t ath6kl_green_tx_params_read(struct file *file,
				char __user *user_buf,
				size_t count, loff_t *ppos)
{
	struct ath6kl *ar = file->private_data;
	u8 buf[128];
	unsigned int len = 0;

	len = scnprintf(buf, sizeof(buf),
			"next_probe_count: %d, max_back_off: %d, min_gtx_rssi: %d, force_back_off: %d\n",
			ar->green_tx_params.next_probe_count,
			ar->green_tx_params.max_back_off,
			ar->green_tx_params.min_gtx_rssi,
			ar->green_tx_params.force_back_off);

	return simple_read_from_buffer(user_buf, count, ppos, buf, len);
}

static const struct file_operations fops_green_tx_params = {
	.read = ath6kl_green_tx_params_read,
	.write = ath6kl_green_tx_params_write,
	.open = ath6kl_debugfs_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};


static int ath6kl_parse_ht_cap_params(const char __user *user_buf, size_t count,
				struct ht_cap_param *htCapParam)
{
	char buf[64];
	char *p;
	unsigned int len;
	int value;

	len = min(count, sizeof(buf) - 1);
	if (copy_from_user(buf, user_buf, len))
		return -EFAULT;

	p = buf;

	SKIP_SPACE;

	sscanf(p, "%d", &value);

	if (value >= IEEE80211_NUM_BANDS)
		return -EFAULT;

	htCapParam->band = (u8) value;

	SEEK_SPACE;
	SKIP_SPACE;

	sscanf(p, "%d", &value);
	htCapParam->chan_width_40M_supported = (u8) value;

	SEEK_SPACE;
	SKIP_SPACE;

	sscanf(p, "%d", &value);
	htCapParam->short_GI = (u8) value;

	SEEK_SPACE;
	SKIP_SPACE;

	sscanf(p, "%d", &value);
	htCapParam->intolerance_40MHz = (u8) value;

	return 0;
}

/* File operation functions for HT Cap */
static ssize_t ath6kl_ht_cap_params_write(struct file *file,
				const char __user *user_buf,
				size_t count, loff_t *ppos)
{
	struct ath6kl *ar = file->private_data;
	struct ht_cap_param tempHtCap;
	int i, ret;

	memset(&tempHtCap, 0, sizeof(struct ht_cap_param));
	ret = ath6kl_parse_ht_cap_params(user_buf, count, &tempHtCap);

	if (ret)
		return ret;

	for (i = 0; i < ar->vif_max; i++) {
		if (ath6kl_wmi_set_ht_cap_cmd(ar->wmi, i,
			tempHtCap.band,
			tempHtCap.chan_width_40M_supported,
			tempHtCap.short_GI,
			tempHtCap.intolerance_40MHz))
			return -EIO;
	}

	tempHtCap.isConfig = 1;
	memcpy(&ar->debug.ht_cap_param[tempHtCap.band],
		&tempHtCap, sizeof(struct ht_cap_param));

	return count;
}

static ssize_t ath6kl_ht_cap_params_read(struct file *file,
				char __user *user_buf,
				size_t count, loff_t *ppos)
{
#define _BUF_SIZE	(128)
	struct ath6kl *ar = file->private_data;
	u8 *buf;
	unsigned int len = 0;
	int band;
	ssize_t ret_cnt;

	buf = kmalloc(_BUF_SIZE, GFP_ATOMIC);
	if (!buf)
		return -ENOMEM;

	for (band = 0; band < A_NUM_BANDS; band++) {
		if (ar->debug.ht_cap_param[band].isConfig)
			len += scnprintf(buf + len, _BUF_SIZE - len,
			"%s chan_width_40M_supported: %d, short_GI: %d, intolerance_40MHz: %d\n",
			(ar->debug.ht_cap_param[band].band == A_BAND_24GHZ ?
			"24GHZ" : "5GHZ"),
			ar->debug.ht_cap_param[band].chan_width_40M_supported,
			ar->debug.ht_cap_param[band].short_GI,
			ar->debug.ht_cap_param[band].intolerance_40MHz);
	}

	ret_cnt = simple_read_from_buffer(user_buf, count, ppos, buf, len);

	kfree(buf);

	return ret_cnt;
#undef _BUF_SIZE
}

/* debug fs for HT Cap */
static const struct file_operations fops_ht_cap_params = {
	.read = ath6kl_ht_cap_params_read,
	.write = ath6kl_ht_cap_params_write,
	.open = ath6kl_debugfs_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

static ssize_t ath6kl_debug_mask_write(struct file *file,
				const char __user *user_buf,
				size_t count, loff_t *ppos)
{
	int ret;

	ret = kstrtou32_from_user(user_buf, count, 0, &debug_mask);

	if (ret)
		return ret;

	return count;
}

static ssize_t ath6kl_debug_mask_read(struct file *file,
				char __user *user_buf,
				size_t count, loff_t *ppos)
{
	char buf[32];
	int len;

	len = snprintf(buf, sizeof(buf), "debug_mask: 0x%x\n", debug_mask);

	return simple_read_from_buffer(user_buf, count, ppos, buf, len);
}

static ssize_t ath6kl_debug_mask_ext_write(struct file *file,
				const char __user *user_buf,
				size_t count, loff_t *ppos)
{
	int ret;

	ret = kstrtou32_from_user(user_buf, count, 0, &debug_mask_ext);

	if (ret)
		return ret;

	return count;
}

static ssize_t ath6kl_debug_mask_ext_read(struct file *file,
				char __user *user_buf,
				size_t count, loff_t *ppos)
{
	char buf[32];
	int len;

	len = snprintf(buf, sizeof(buf), "debug_mask_ext: 0x%x\n",
				debug_mask_ext);

	return simple_read_from_buffer(user_buf, count, ppos, buf, len);
}

static const struct file_operations fops_debug_mask = {
	.read = ath6kl_debug_mask_read,
	.write = ath6kl_debug_mask_write,
	.open = ath6kl_debugfs_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

static const struct file_operations fops_debug_mask_ext = {
	.read = ath6kl_debug_mask_ext_read,
	.write = ath6kl_debug_mask_ext_write,
	.open = ath6kl_debugfs_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

/* File operation functions for HT Coex */
static ssize_t ath6kl_ht_coex_params_write(struct file *file,
				const char __user *user_buf,
				size_t count, loff_t *ppos)
{
	struct ath6kl *ar = file->private_data;
	struct ath6kl_vif *vif;
	char *p;
	int scan_interval, rate_interval;
	char buf[32];
	ssize_t len;
	int i;

	len = min(count, sizeof(buf) - 1);
	if (copy_from_user(buf, user_buf, len))
		return -EFAULT;

	p = buf;

	SKIP_SPACE;

	sscanf(p, "%d", &scan_interval);

	SEEK_SPACE;
	SKIP_SPACE;

	sscanf(p, "%d", &rate_interval);

	for (i = 0; i < ar->vif_max; i++) {
		vif = ath6kl_get_vif_by_index(ar, i);
		if (vif)
			ath6kl_htcoex_config(vif,
					     (u32)scan_interval,
					     (u8)rate_interval);
	}

	return count;
}

static ssize_t ath6kl_ht_coex_params_read(struct file *file,
				char __user *user_buf,
				size_t count, loff_t *ppos)
{
#define _BUF_SIZE	(128)
	struct ath6kl *ar = file->private_data;
	struct ath6kl_vif *vif;
	u8 *buf;
	unsigned int len = 0;
	int i;
	ssize_t ret_cnt;

	buf = kmalloc(_BUF_SIZE, GFP_ATOMIC);
	if (!buf)
		return -ENOMEM;

	for (i = 0; i < ar->vif_max; i++) {
		vif = ath6kl_get_vif_by_index(ar, i);
		if ((vif) &&
			(vif->htcoex_ctx))
			len += scnprintf(buf + len, _BUF_SIZE - len,
					"int-%d flags %x, scan %d ms, num_scan %d, rate %d, t_cnt %d\n",
					i,
					vif->htcoex_ctx->flags,
					vif->htcoex_ctx->scan_interval,
					vif->htcoex_ctx->num_scan,
					vif->htcoex_ctx->rate_rollback_interval,
					vif->htcoex_ctx->tolerant40_cnt);
	}

	ret_cnt = simple_read_from_buffer(user_buf, count, ppos, buf, len);

	kfree(buf);

	return ret_cnt;
#undef _BUF_SIZE
}

/* debug fs for HT Coex */
static const struct file_operations fops_ht_coex_params = {
	.read = ath6kl_ht_coex_params_read,
	.write = ath6kl_ht_coex_params_write,
	.open = ath6kl_debugfs_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

static int ath6kl_parse_tx_amsdu_params(const char __user *user_buf,
				size_t count,
				struct ath6kl_vif *vif)
{
	char buf[64];
	char *p;
	unsigned int len;
	int value;
	u8 tx_amsdu_max_aggr_num;
	u16 tx_amsdu_max_pdu_len, tx_amsdu_timeout;
	bool tx_amsdu_seq_pkt;
	bool tx_amsdu_progressive;

	len = min(count, sizeof(buf) - 1);
	if (copy_from_user(buf, user_buf, len))
		return -EFAULT;

	p = buf;

	SKIP_SPACE;

	sscanf(p, "%d", &value);
	tx_amsdu_max_aggr_num = value;

	SEEK_SPACE;
	SKIP_SPACE;

	sscanf(p, "%d", &value);
	tx_amsdu_max_pdu_len = value;

	SEEK_SPACE;
	SKIP_SPACE;

	sscanf(p, "%d", &value);
	tx_amsdu_timeout = value;

	SEEK_SPACE;
	SKIP_SPACE;

	sscanf(p, "%d", &value);
	if (value)
		tx_amsdu_seq_pkt = true;
	else
		tx_amsdu_seq_pkt = false;

	SEEK_SPACE;
	SKIP_SPACE;

	sscanf(p, "%d", &value);
	if (value)
		tx_amsdu_progressive = true;
	else
		tx_amsdu_progressive = false;

	aggr_tx_config(vif,
			tx_amsdu_seq_pkt,
			tx_amsdu_progressive,
			tx_amsdu_max_aggr_num,
			tx_amsdu_max_pdu_len,
			tx_amsdu_timeout);

	return 0;
}

/* File operation functions for TX A-MSDU Params */
static ssize_t ath6kl_tx_amsdu_params_write(struct file *file,
				const char __user *user_buf,
				size_t count, loff_t *ppos)
{
	struct ath6kl *ar = file->private_data;
	struct ath6kl_vif *vif;
	int i, ret;

	for (i = 0; i < ar->vif_max; i++) {
		vif = ath6kl_get_vif_by_index(ar, i);
		if ((vif) &&
			(vif->aggr_cntxt)) {
			ret = ath6kl_parse_tx_amsdu_params(user_buf,
							count, vif);

			if (ret)
				return ret;
		}
	}

	return count;
}

static ssize_t ath6kl_tx_amsdu_params_read(struct file *file,
				char __user *user_buf,
				size_t count, loff_t *ppos)
{
#define _BUF_SIZE	(8192)
	struct ath6kl *ar = file->private_data;
	struct ath6kl_vif *vif;
	struct ath6kl_sta *conn;
	struct txtid *txtid;
	u8 *buf;
	unsigned int len = 0;
	int i, j, k, num_sta;
	ssize_t ret_cnt;

	buf = kmalloc(_BUF_SIZE, GFP_ATOMIC);
	if (!buf)
		return -ENOMEM;

	for (i = 0; i < ar->vif_max; i++) {
		vif = ath6kl_get_vif_by_index(ar, i);
		if ((vif) &&
			(vif->aggr_cntxt)) {
			len += scnprintf(buf + len, _BUF_SIZE - len,
				"DEV-%d %s max_aggr_len: %d max_aggr_num: %d, max_pdu_len: %d, timeout = %d ms, seq_pkt %s, tx_prog %s\n",
				i,
				(vif->aggr_cntxt->tx_amsdu_enable == true ?
							"Enable" : "Disable"),
				vif->aggr_cntxt->tx_amsdu_max_aggr_len,
				vif->aggr_cntxt->tx_amsdu_max_aggr_num,
				vif->aggr_cntxt->tx_amsdu_max_pdu_len,
				vif->aggr_cntxt->tx_amsdu_timeout,
				(vif->aggr_cntxt->tx_amsdu_seq_pkt == true ?
							"Yes" : "No"),
				(vif->aggr_cntxt->tx_amsdu_progressive == true ?
							"Yes" : "No"));

			num_sta = (vif->nw_type == INFRA_NETWORK) ?
					1 : AP_MAX_NUM_STA;
			for (j = 0; j < num_sta; j++) {
				conn = &vif->sta_list[j];
				len += scnprintf(buf + len, _BUF_SIZE - len,
						" [%02x:%02x:%02x:%02x:%02x:%02x]\n",
						conn->mac[0], conn->mac[1],
						conn->mac[2], conn->mac[3],
						conn->mac[4], conn->mac[5]);
				for (k = 0; k < NUM_OF_TIDS; k++) {
					txtid = AGGR_GET_TXTID(
						conn->aggr_conn_cntxt, k);
					len += scnprintf(buf + len,
							_BUF_SIZE - len,
							"   %d-%d-%s AMSDU: %d PDU: %d PROG: %d/%d TIMEOUT/FLUSH/NULL/OVERFLOW: %d/%d/%d/%d\n",
							txtid->tid,
							txtid->aid,
							(txtid->max_aggr_sz ?
							"ON " : "OFF"),
							txtid->num_amsdu,
							txtid->num_pdu,
							txtid->last_num_amsdu,
							txtid->last_num_timeout,
							txtid->num_timeout,
							txtid->num_flush,
							txtid->num_tx_null,
							txtid->num_overflow);
				}
			}
		}
	}

	ret_cnt = simple_read_from_buffer(user_buf, count, ppos, buf, len);

	kfree(buf);

	return ret_cnt;
#undef _BUF_SIZE
}

/* debug fs for TX A-MSDU Params */
static const struct file_operations fops_tx_amsdu_params = {
	.read = ath6kl_tx_amsdu_params_read,
	.write = ath6kl_tx_amsdu_params_write,
	.open = ath6kl_debugfs_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

static int ath6kl_parse_tx_amsdu(const char __user *user_buf, size_t count,
				struct ath6kl_vif *vif)
{
	char buf[64];
	char *p;
	unsigned int len;
	int value;

	len = min(count, sizeof(buf) - 1);
	if (copy_from_user(buf, user_buf, len))
		return -EFAULT;

	p = buf;

	SKIP_SPACE;

	sscanf(p, "%d", &value);
	if (value)
		set_bit(AMSDU_ENABLED, &vif->flags);
	else
		clear_bit(AMSDU_ENABLED, &vif->flags);

	return 0;
}

/* File operation functions for TX A-MSDU */
static ssize_t ath6kl_tx_amsdu_write(struct file *file,
				const char __user *user_buf,
				size_t count, loff_t *ppos)
{
	struct ath6kl *ar = file->private_data;
	struct ath6kl_vif *vif;
	int i, ret;

	for (i = 0; i < ar->vif_max; i++) {
		vif = ath6kl_get_vif_by_index(ar, i);
		if ((vif) &&
			(vif->aggr_cntxt)) {
			ret = ath6kl_parse_tx_amsdu(user_buf, count, vif);

			if (ret)
				return ret;
		}
	}

	return count;
}

static ssize_t ath6kl_tx_amsdu_read(struct file *file,
				char __user *user_buf,
				size_t count, loff_t *ppos)
{
#define _BUF_SIZE	(128)
	struct ath6kl *ar = file->private_data;
	struct ath6kl_vif *vif;
	u8 *buf;
	unsigned int len = 0;
	int i;
	ssize_t ret_cnt;

	buf = kmalloc(_BUF_SIZE, GFP_ATOMIC);
	if (!buf)
		return -ENOMEM;

	for (i = 0; i < ar->vif_max; i++) {
		vif = ath6kl_get_vif_by_index(ar, i);
		if ((vif) &&
			(vif->aggr_cntxt))
			len += scnprintf(buf + len, _BUF_SIZE - len,
					"DEV-%d %s\n",
					i,
					(test_bit(AMSDU_ENABLED, &vif->flags)
					? "ON" : "OFF"));
	}

	ret_cnt = simple_read_from_buffer(user_buf, count, ppos, buf, len);

	kfree(buf);

	return ret_cnt;
#undef _BUF_SIZE
}

/* debug fs for TX A-MSDU */
static const struct file_operations fops_tx_amsdu = {
	.read = ath6kl_tx_amsdu_read,
	.write = ath6kl_tx_amsdu_write,
	.open = ath6kl_debugfs_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

static ssize_t ath6kl_htc_stat_read(struct file *file,
				char __user *user_buf,
				size_t count, loff_t *ppos)
{
#define _BUF_SIZE	(8192)
	struct ath6kl *ar = file->private_data;
	struct ath6kl_cookie_pool *cookie_pool;
	u8 *buf;
	unsigned int len = 0;
	ssize_t ret_cnt;

	buf = kmalloc(_BUF_SIZE, GFP_ATOMIC);
	if (!buf)
		return -ENOMEM;

	/* HTC cookie stats */
	cookie_pool = &ar->cookie_data;
	len += scnprintf(buf + len, _BUF_SIZE - len,
			"DATA Cookie : num %d avail %d, alloc %d fail %d free %d peak %d\n",
			cookie_pool->cookie_num,
			cookie_pool->cookie_count,
			cookie_pool->cookie_alloc_cnt,
			cookie_pool->cookie_alloc_fail_cnt,
			cookie_pool->cookie_free_cnt,
			cookie_pool->cookie_peak_cnt);

	cookie_pool = &ar->cookie_ctrl;
	len += scnprintf(buf + len, _BUF_SIZE - len,
			"CTRL Cookie : num %d avail %d, alloc %d fail %d free %d peak %d\n",
			cookie_pool->cookie_num,
			cookie_pool->cookie_count,
			cookie_pool->cookie_alloc_cnt,
			cookie_pool->cookie_alloc_fail_cnt,
			cookie_pool->cookie_free_cnt,
			cookie_pool->cookie_peak_cnt);


	len += ath6kl_htc_stat(ar->htc_target, buf + len, _BUF_SIZE - len);

	ret_cnt = simple_read_from_buffer(user_buf, count, ppos, buf, len);

	kfree(buf);

	return ret_cnt;
#undef _BUF_SIZE
}


/* debug fs for HTC EP Stats. */
static const struct file_operations fops_htc_stat = {
	.read = ath6kl_htc_stat_read,
	.open = ath6kl_debugfs_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

static ssize_t ath6kl_hif_stat_read(struct file *file,
				char __user *user_buf,
				size_t count, loff_t *ppos)
{
#define _BUF_SIZE	(4096)
	struct ath6kl *ar = file->private_data;
	u8 *buf;
	unsigned int len;
	ssize_t ret_cnt;

	buf = kmalloc(_BUF_SIZE, GFP_ATOMIC);
	if (!buf)
		return -ENOMEM;

	len = ath6kl_hif_stat(ar, buf, _BUF_SIZE);

	ret_cnt = simple_read_from_buffer(user_buf, count, ppos, buf, len);

	kfree(buf);

	return ret_cnt;
#undef _BUF_SIZE
}


/* debug fs for HIF Stats. */
static const struct file_operations fops_hif_stat = {
	.read = ath6kl_hif_stat_read,
	.open = ath6kl_debugfs_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

/* File operation functions for HIF-PIPE Max. Schedule packages */
static ssize_t ath6kl_hif_pipe_max_sche_write(struct file *file,
				const char __user *user_buf,
				size_t count, loff_t *ppos)
{
	struct ath6kl *ar = file->private_data;
	char buf[64];
	char *p;
	unsigned int len;
	u32 max_sche_tx, max_sche_rx;

	if (ar->hif_type != ATH6KL_HIF_TYPE_USB)
		return -EIO;

	len = min(count, sizeof(buf) - 1);
	if (copy_from_user(buf, user_buf, len))
		return -EFAULT;

	p = buf;

	SKIP_SPACE;

	sscanf(p, "%d", &max_sche_tx);

	SEEK_SPACE;
	SKIP_SPACE;

	sscanf(p, "%d", &max_sche_rx);

	ath6kl_hif_pipe_set_max_sche(ar, max_sche_tx, max_sche_rx);

	return count;
}

/* debug fs for HIF-PIPE Max. Schedule packages */
static const struct file_operations fops_hif_pipe_max_sche = {
	.write = ath6kl_hif_pipe_max_sche_write,
	.open = ath6kl_debugfs_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

/* File operation functions for AP-APSD */
static ssize_t ath6kl_ap_apsd_read(struct file *file,
				char __user *user_buf,
				size_t count, loff_t *ppos)
{
#define _BUF_SIZE	(128)
	struct ath6kl *ar = file->private_data;
	struct ath6kl_vif *vif;
	u8 *buf;
	unsigned int len = 0;
	int i;
	ssize_t ret_cnt;

	buf = kmalloc(_BUF_SIZE, GFP_ATOMIC);
	if (!buf)
		return -ENOMEM;

	for (i = 0; i < ar->vif_max; i++) {
		vif = ath6kl_get_vif_by_index(ar, i);
		if ((vif) && (vif->nw_type == AP_NETWORK)) {
			len += scnprintf(buf, _BUF_SIZE,
				"VAP(%d), apsd: %d\n", i,
				vif->ap_apsd);
		}
	}

	ret_cnt = simple_read_from_buffer(user_buf, count, ppos, buf, len);

	kfree(buf);

	return ret_cnt;
#undef _BUF_SIZE
}

static ssize_t ath6kl_ap_apsd_write(struct file *file,
				const char __user *user_buf,
				size_t count, loff_t *ppos)
{
	struct ath6kl *ar = file->private_data;
	struct ath6kl_vif *vif;
	u32 ap_apsd;
	char buf[32];
	ssize_t len;
	int i;

	len = min(count, sizeof(buf) - 1);
	if (copy_from_user(buf, user_buf, len))
		return -EFAULT;

	buf[len] = '\0';
	if (kstrtou32(buf, 0, &ap_apsd))
		return -EINVAL;

	for (i = 0; i < ar->vif_max; i++) {
		vif = ath6kl_get_vif_by_index(ar, i);
		if ((vif) &&
			(vif->nw_type == AP_NETWORK)) {
			ath6kl_wmi_ap_set_apsd(vif->ar->wmi, vif->fw_vif_idx,
				ap_apsd ?
				WMI_AP_APSD_ENABLED : WMI_AP_APSD_DISABLED);
			vif->ap_apsd = ap_apsd;
		}
	}

	return count;
}

/* debug fs for AP-APSD */
static const struct file_operations fops_ap_apsd = {
	.read = ath6kl_ap_apsd_read,
	.write = ath6kl_ap_apsd_write,
	.open = ath6kl_debugfs_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

/* File operation functions for AP-IntraBss */
static ssize_t ath6kl_ap_intrabss_read(struct file *file,
				char __user *user_buf,
				size_t count, loff_t *ppos)
{
#define _BUF_SIZE	(256)
	struct ath6kl *ar = file->private_data;
	struct ath6kl_vif *vif;
	u8 *buf;
	unsigned int len = 0;
	int i;
	ssize_t ret_cnt;

	buf = kmalloc(_BUF_SIZE, GFP_ATOMIC);
	if (!buf)
		return -ENOMEM;

	for (i = 0; i < ar->vif_max; i++) {
		vif = ath6kl_get_vif_by_index(ar, i);
		if ((vif) && (vif->nw_type == AP_NETWORK)) {
			len += scnprintf(buf, _BUF_SIZE,
				"VAP(%d), ap_intrabss: %d\n", i,
				vif->intra_bss);
		}
	}

	ret_cnt = simple_read_from_buffer(user_buf, count, ppos, buf, len);

	kfree(buf);

	return ret_cnt;
#undef _BUF_SIZE
}

static ssize_t ath6kl_ap_intrabss_write(struct file *file,
				const char __user *user_buf,
				size_t count, loff_t *ppos)
{
	struct ath6kl *ar = file->private_data;
	struct ath6kl_vif *vif;
	u32 ap_intrabss;
	char buf[32];
	ssize_t len;
	int i;

	len = min(count, sizeof(buf) - 1);
	if (copy_from_user(buf, user_buf, len))
		return -EFAULT;

	buf[len] = '\0';
	if (kstrtou32(buf, 0, &ap_intrabss))
		return -EINVAL;

	for (i = 0; i < ar->vif_max; i++) {
		vif = ath6kl_get_vif_by_index(ar, i);
		if ((vif) &&
			(vif->nw_type == AP_NETWORK))
			vif->intra_bss = (ap_intrabss ? 1 : 0);
	}

	return count;
}

/* debug fs for AP-IntraBss */
static const struct file_operations fops_ap_intrabss = {
	.read = ath6kl_ap_intrabss_read,
	.write = ath6kl_ap_intrabss_write,
	.open = ath6kl_debugfs_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

static ssize_t ath6kl_get_force_passcan(struct file *file,
				char __user *user_buf,
				size_t count, loff_t *ppos)
{
	struct ath6kl *ar = file->private_data;
	u8 buf[128];
	unsigned int len = 0;

	len = scnprintf(buf, sizeof(buf),
		"force passive: %d\n", ar->debug.force_passive);

	return simple_read_from_buffer(user_buf, count, ppos, buf, len);
}

static ssize_t ath6kl_set_force_passcan(struct file *file,
				const char __user *user_buf,
				size_t count, loff_t *ppos)
{
	struct ath6kl *ar = file->private_data;
	struct ath6kl_vif *vif;
	unsigned int len = 0;
	u8 buf[16];
	u32 force_passcan;

	vif = ath6kl_vif_first(ar);
	if (!vif)
		return -EIO;

	len = min(count, sizeof(buf) - 1);
	if (copy_from_user(buf, user_buf, len))
		return -EFAULT;

	buf[len] = '\0';
	if (kstrtou32(buf, 0, &force_passcan))
		return -EINVAL;

	if (force_passcan == 1)
		vif->sc_params.scan_ctrl_flags &= ~ACTIVE_SCAN_CTRL_FLAGS;
	else
		vif->sc_params.scan_ctrl_flags |= ACTIVE_SCAN_CTRL_FLAGS;

	ar->debug.force_passive = force_passcan;

	ath6kl_wmi_scanparams_cmd(ar->wmi, vif->fw_vif_idx,
				vif->sc_params.fg_start_period,
				vif->sc_params.fg_end_period,
				vif->sc_params.bg_period,
				vif->sc_params.minact_chdwell_time,
				vif->sc_params.maxact_chdwell_time,
				vif->sc_params.pas_chdwell_time,
				vif->sc_params.short_scan_ratio,
				vif->sc_params.scan_ctrl_flags,
				vif->sc_params.max_dfsch_act_time,
				vif->sc_params.maxact_scan_per_ssid);

	return count;
}

static const struct file_operations fops_force_passcan = {
	.read = ath6kl_get_force_passcan,
	.write = ath6kl_set_force_passcan,
	.open = ath6kl_debugfs_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

static ssize_t ath6kl_get_pmkid_list(struct file *file,
				char __user *user_buf,
				size_t count, loff_t *ppos)
{
#define PMKID_LIST_NUM_SIZE 4

	struct ath6kl *ar = file->private_data;
	struct ath6kl_vif *vif;
	unsigned int len = 0, buf_len;
	u8 *p;
	u8 buf[512];
	struct wmi_pmkid_list_reply *reply;
	long left;

	vif = ath6kl_vif_first(ar);
	if (!vif)
		return -EIO;

	if (down_interruptible(&ar->sem))
		return -EBUSY;

	set_bit(PMKLIST_GET_PEND, &vif->flags);

	if (ath6kl_wmi_get_pmkid_list(ar->wmi, 0)) {
		up(&ar->sem);
		return -EIO;
	}

	left = wait_event_interruptible_timeout(ar->event_wq,
						!test_bit(PMKLIST_GET_PEND,
						&vif->flags), WMI_TIMEOUT);

	up(&ar->sem);

	if (left <= 0)
		return -ETIMEDOUT;

	reply = (struct wmi_pmkid_list_reply *) vif->pmkid_list_buf;
	p = buf;
	buf_len = sizeof(buf);

	len += scnprintf(p + len, buf_len - len, "PMKID List\n");

	if (reply->num_pmkid != 0 && reply->num_pmkid <= WMI_MAX_PMKID_CACHE) {
		int i;
		char *bssid = (char *)reply + PMKID_LIST_NUM_SIZE;
		char *pmkid = (char *)reply + PMKID_LIST_NUM_SIZE + ETH_ALEN;

		for (i = 0; i < reply->num_pmkid; i++) {
			len += scnprintf(p + len, buf_len - len,
					"bssid: %02x:%02x:%02x:%02x:%02x:%02x\n",
					(bssid[0] & 0xff), (bssid[1] & 0xff),
					(bssid[2] & 0xff), (bssid[3] & 0xff),
					(bssid[4] & 0xff), (bssid[5] & 0xff));
			len += scnprintf(p + len, buf_len - len,
				"pmkid: %02x:%02x:%02x:%02x:%02x:%02x%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\n",
				(pmkid[0] & 0xff), (pmkid[1] & 0xff),
				(pmkid[2] & 0xff), (pmkid[3] & 0xff),
				(pmkid[4] & 0xff), (pmkid[5] & 0xff),
				(pmkid[6] & 0xff), (pmkid[7] & 0xff),
				(pmkid[8] & 0xff), (pmkid[9] & 0xff),
				(pmkid[10] & 0xff), (pmkid[11] & 0xff),
				(pmkid[12] & 0xff), (pmkid[13] & 0xff),
				(pmkid[14] & 0xff), (pmkid[15] & 0xff));

			bssid += (sizeof(struct wmi_pmkid) + ETH_ALEN);
			pmkid += (sizeof(struct wmi_pmkid) + ETH_ALEN);
		}
	}

	return simple_read_from_buffer(user_buf, count, ppos, buf, len);
}

static const struct file_operations fops_pmkid_list = {
	.read = ath6kl_get_pmkid_list,
	.open = ath6kl_debugfs_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

static ssize_t ath6kl_txseries_write(struct file *file,
				     const char __user *user_buf,
				     size_t count, loff_t *ppos)
{
	struct ath6kl *ar = file->private_data;
	struct ath6kl_vif *vif;
	u64 tx_series;
	int ret, i;

	for (i = 0; i < ar->vif_max; i++) {
		vif = ath6kl_get_vif_by_index(ar, i);

		if (vif) {

			ret = kstrtou64_from_user(user_buf, count, 0,
						&tx_series);
			if (ret)
				return ret;

			ar->debug.set_tx_series = tx_series;

			if (ath6kl_wmi_set_tx_select_rates_on_all_mode(
							ar->wmi,
							vif->fw_vif_idx,
							tx_series))
				return -EIO;
		}
	}

	return count;
}

static ssize_t ath6kl_txseries_read(struct file *file,
			char __user *user_buf,
			size_t count, loff_t *ppos)
{
	struct ath6kl *ar = file->private_data;
	u8 buf[32];
	unsigned int len = 0;

	len = scnprintf(buf, sizeof(buf), "Txratemask is 0x%llx\n",
		ar->debug.set_tx_series);

	return simple_read_from_buffer(user_buf, count, ppos, buf, len);
}

static const struct file_operations fops_txseries_write = {
	.read = ath6kl_txseries_read,
	.write = ath6kl_txseries_write,
	.open = ath6kl_debugfs_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

static ssize_t ath6kl_antdivcfg_write(struct file *file,
				     const char __user *user_buf,
				     size_t count, loff_t *ppos)
{
	struct ath6kl *ar = file->private_data;
	struct ath6kl_vif *vif;
	u8 div_control;
	int ret;

	vif = ath6kl_vif_first(ar);
	if (!vif)
		return -EIO;

	ret = kstrtou8_from_user(user_buf, count, 0, &div_control);
	if (ret)
		return ret;

	if (ath6kl_wmi_set_antdivcfg(ar->wmi, vif->fw_vif_idx, div_control))
		return -EIO;

	return count;
}

static const struct file_operations fops_antdiv_write = {
	.write = ath6kl_antdivcfg_write,
	.open = ath6kl_debugfs_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

int ath6kl_antdiv_stat(struct ath6kl_vif *vif,
			     u8 *buf, int buf_len)
{
	int len = 0;
	len += snprintf(buf + len, buf_len - len, "\n Scan start time : %d",
		vif->ant_div_stat.scan_start_time);
	len += snprintf(buf + len, buf_len - len, "\n Total packet count : %d",
		vif->ant_div_stat.total_pkt_count);
	len += snprintf(buf + len, buf_len - len, "\n Main Recv count : %d",
		vif->ant_div_stat.main_recv_cnt);
	len += snprintf(buf + len, buf_len - len, "\n Alt Recv count : %d",
		vif->ant_div_stat.alt_recv_cnt);
	len += snprintf(buf + len, buf_len - len, "\n Main Rssi Avg : %d",
		vif->ant_div_stat.main_rssi_avg);
	len += snprintf(buf + len, buf_len - len, "\n Alt Rssi Avg : %d",
		vif->ant_div_stat.alt_rssi_avg);
	len += snprintf(buf + len, buf_len - len, "\n Current Main Set : %d",
		vif->ant_div_stat.curr_main_set);
	len += snprintf(buf + len, buf_len - len, "\n Current Alt Set : %d",
		vif->ant_div_stat.curr_alt_set);
	len += snprintf(buf + len, buf_len - len, "\n Current Bias : %d",
		vif->ant_div_stat.curr_bias);
	len += snprintf(buf + len, buf_len - len, "\n Main LNA conf : %d",
		vif->ant_div_stat.main_lna_conf);
	len += snprintf(buf + len, buf_len - len, "\n Alt LNA conf : %d",
		vif->ant_div_stat.alt_lna_conf);
	len += snprintf(buf + len, buf_len - len, "\n Fast Div Bias: %d",
		vif->ant_div_stat.fast_div_bias);
	len += snprintf(buf + len, buf_len - len, "\n End state : %d",
		vif->ant_div_stat.end_st);
	len += snprintf(buf + len, buf_len - len, "\n Scan: %d",
		vif->ant_div_stat.scan);
	len += snprintf(buf + len, buf_len - len, "\n Scan not start : %d\n",
		vif->ant_div_stat.scan_not_start);
	return len;
}

static ssize_t ath6kl_antdivstat_read(struct file *file,
				     char __user *user_buf,
				     size_t count, loff_t *ppos)
{
#define _BUF_SIZE	(4096)
	struct ath6kl *ar = file->private_data;
	u8 *buf;
	unsigned int len;
	ssize_t ret_cnt;
	struct ath6kl_vif *vif;
	vif = ath6kl_vif_first(ar);

	/* should always get value by ath6kl_vif_first */
	if (!vif)
		return -EIO;

	buf = kmalloc(_BUF_SIZE, GFP_ATOMIC);
	if (!buf)
		return -ENOMEM;
	len = ath6kl_antdiv_stat(vif, buf, _BUF_SIZE);
	ret_cnt = simple_read_from_buffer(user_buf, count, ppos, buf, len);
	kfree(buf);
	return ret_cnt;
#undef _BUF_SIZE
}

static const struct file_operations fops_antdiv_state_read = {
	.read = ath6kl_antdivstat_read,
	.open = ath6kl_debugfs_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

int ath6kl_ani_stat(struct ath6kl_vif *vif,
			     u8 *buf, int buf_len)
{
	int len = 0;

	if (!vif->ar->debug.anistat_enable) {
			len += snprintf(buf + len, buf_len - len,
				" ANI : ANISTAT OFF\n");
	} else {
		if ((vif->ani_enable == vif->ani_stat.enable) &&
			(vif->ani_pollcnt == vif->ani_stat.pollcnt)) {
			len += snprintf(buf + len, buf_len - len,
				"\n ANI : No Change... (en = %d, pollcnt = %d)\n",
				vif->ani_enable,
				vif->ani_stat.pollcnt);
		} else {
			len += snprintf(buf + len, buf_len - len,
				"\n ANI : %s",
				vif->ani_stat.enable ? "ON" : "OFF");
			len += snprintf(buf + len, buf_len - len,
				"\n RSSI : %d",
				vif->ani_stat.rssi);
			len += snprintf(buf + len, buf_len - len,
				"\n Poll Count : %d",
				vif->ani_stat.pollcnt);
			len += snprintf(buf + len, buf_len - len,
				"\n noiseImmunityLevel : %d",
				vif->ani_stat.noiseImmunityLevel);
			len += snprintf(buf + len, buf_len - len,
				"\n spurImmunityLevel : %d",
				vif->ani_stat.spurImmunityLevel);
			len += snprintf(buf + len, buf_len - len,
				"\n firstepLevel : %d",
				vif->ani_stat.firstepLevel);
			len += snprintf(buf + len, buf_len - len,
				"\n ofdmWeakSigDetectOff : %s",
				vif->ani_stat.ofdmWeakSigDetectOff ?
				"ON" : "OFF");
			len += snprintf(buf + len, buf_len - len,
				"\n cckWeakSigThreshold : %s",
				vif->ani_stat.cckWeakSigThreshold ?
				"HIGH" : "LOW");

			len += snprintf(buf + len, buf_len - len,
				"\n listenTime : %d",
				vif->ani_stat.listenTime);
			len += snprintf(buf + len, buf_len - len,
				"\n ofdmTrigHigh : %d",
				vif->ani_stat.ofdmTrigHigh);
			len += snprintf(buf + len, buf_len - len,
				"\n ofdmTrigLow : %d",
				vif->ani_stat.ofdmTrigLow);
			len += snprintf(buf + len, buf_len - len,
				"\n cckTrigHigh : %d",
				vif->ani_stat.cckTrigHigh);
			len += snprintf(buf + len, buf_len - len,
				"\n cckTrigLow : %d",
				vif->ani_stat.cckTrigLow);
			len += snprintf(buf + len, buf_len - len,
				"\n rssiThrLow : %d",
				vif->ani_stat.rssiThrLow);
			len += snprintf(buf + len, buf_len - len,
				"\n rssiThrHigh : %d",
				vif->ani_stat.rssiThrHigh);
			len += snprintf(buf + len, buf_len - len,
				"\n cycleCount : 0x%x",
				vif->ani_stat.cycleCount);
			len += snprintf(buf + len, buf_len - len,
				"\n ofdmPhyErrCount : %d",
				vif->ani_stat.ofdmPhyErrCount);
			len += snprintf(buf + len, buf_len - len,
				"\n cckPhyErrCount : %d",
				vif->ani_stat.cckPhyErrCount);
			len += snprintf(buf + len, buf_len - len,
				"\n ofdmPhyErrBase : %d",
				vif->ani_stat.ofdmPhyErrBase);
			len += snprintf(buf + len, buf_len - len,
				"\n cckPhyErrBase : %d",
				vif->ani_stat.cckPhyErrBase);
			len += snprintf(buf + len, buf_len - len,
				"\n rxFrameCount : %d",
				vif->ani_stat.rxFrameCount);
			len += snprintf(buf + len, buf_len - len,
				"\n txFrameCount : %d\n",
				vif->ani_stat.txFrameCount);

			vif->ani_enable = vif->ani_stat.enable;
			vif->ani_pollcnt = vif->ani_stat.pollcnt;
		}
	}

	return len;
}

static ssize_t ath6kl_anistat_read(struct file *file,
				     char __user *user_buf,
				     size_t count, loff_t *ppos)
{
#define _BUF_SIZE	(4096)
	struct ath6kl *ar = file->private_data;
	u8 *buf;
	unsigned int len;
	ssize_t ret_cnt;
	struct ath6kl_vif *vif;
	vif = ath6kl_vif_first(ar);

	/* should always get value by ath6kl_vif_first */
	if (!vif)
		return -EIO;

	buf = kmalloc(_BUF_SIZE, GFP_ATOMIC);
	if (!buf)
		return -ENOMEM;
	len = ath6kl_ani_stat(vif, buf, _BUF_SIZE);
	ret_cnt = simple_read_from_buffer(user_buf, count, ppos, buf, len);
	kfree(buf);
	return ret_cnt;
#undef _BUF_SIZE
}

/* File operation functions for ANI statistic info */
static ssize_t ath6kl_anistat_write(struct file *file,
				const char __user *user_buf,
				size_t count, loff_t *ppos)
{
	struct ath6kl *ar = file->private_data;
	int ret;

	ret = kstrtou8_from_user(user_buf, count, 0, &ar->debug.anistat_enable);
	if (ret)
		return ret;

	if (ath6kl_wmi_anistate_enable(ar->wmi,
		(struct wmi_config_enable_cmd *)&ar->debug.anistat_enable))
		return -EIO;

	return count;
}

static const struct file_operations fops_ani_state_read = {
	.read = ath6kl_anistat_read,
	.write = ath6kl_anistat_write,
	.open = ath6kl_debugfs_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

struct pattern_hdr {
	u8	preamble[4];
	u16	duration;
	u8	seqno;
	u8	schedule;
	u16	pktlen;
	u16	chksum;
} __packed;

int readpatternfile(char *path, u8* buf, int len)
{
	struct file *fp = NULL;
	mm_segment_t pre_fd;
	int filelen = 0;

	path[strlen(path)-1] = '\0';

	fp = filp_open((const char *)path, O_RDONLY, 0);

	pre_fd = get_fs();
	set_fs(KERNEL_DS);
	if (fp && !IS_ERR(fp))	{
		if (fp->f_op && fp->f_op->read)	{
			fp->f_op->read(fp, buf, len, &fp->f_pos);
			filelen = fp->f_dentry->d_inode->i_size;
		}
		filp_close(fp, 0);
	}
	set_fs(pre_fd);
	return filelen;
}

static ssize_t ath6kl_patterngen_write(struct file *file,
				     const char __user *user_buf,
				     size_t count, loff_t *ppos)
{
	struct ath6kl *ar = file->private_data;
	struct ath6kl_vif *vif;
	struct ath6kl_cookie *cookie = NULL;
	enum htc_endpoint_id eid = ENDPOINT_UNUSED;
	int ret;

	u8 pattern_idx = 0;
	u16 pattern_duration = 0;
	u8 *pattern_buf = NULL;
	u16 pattern_len;
	struct sk_buff *skb = NULL;

	u32 map_no = 0;
	u16 htc_tag = ATH6KL_DATA_PKT_TAG;
	u8 ac = 99 ; /* initialize to unmapped ac */
	bool chk_adhoc_ps_mapping = false;
	u32 wmi_data_flags = 0;

	/* save user command */
	u8 buf[512];
	struct pattern_hdr PHdr;
	unsigned int len = 0;
	char *sptr, *token;
	len = min(count, sizeof(buf) - 1);
	if (copy_from_user(buf, user_buf, len))
		return -EFAULT;
	buf[len] = '\0';
	sptr = buf;

	/* get pattern Idx */
	token = strsep(&sptr, " ");
	if (!token)
		return -EINVAL;
	if (kstrtou8(token, 0, &pattern_idx))
		return -EINVAL;
	if (pattern_idx > 6)
		return -EINVAL;
	/* get pattern duration */
	token = strsep(&sptr, " ");
	if (!token)
		return -EINVAL;
	if (kstrtou16(token, 0, &pattern_duration))
		return -EINVAL;

	if (pattern_duration == 0) {
		pattern_buf = kmalloc(64, GFP_ATOMIC);
		if (!pattern_buf)
			return -ENOMEM;
		memset(pattern_buf, 0, 64);
		pattern_len = 64;
	} else{
		/* get pattern path */
		token = strsep(&sptr, " ");
		if (!token)
			return -EINVAL;

		pattern_buf = kmalloc(1024, GFP_ATOMIC);
		if (!pattern_buf)
			return -ENOMEM;

		pattern_len = readpatternfile(token, pattern_buf, 1024);

		if (!pattern_len || pattern_len > 1024)
			return -EINVAL;
	}

	vif = ath6kl_vif_first(ar);
	if (!vif) {
		kfree(pattern_buf);
		return -EIO;
	}

	/* If target is not associated */
	if (!test_bit(CONNECTED, &vif->flags)) {
		kfree(pattern_buf);
		return -EINVAL;
	}

	if (!test_bit(WMI_READY, &ar->flag) &&
	    !test_bit(TESTMODE_EPPING, &ar->flag)) {
		goto fail_tx;
	}

	/*form SKB*/
	skb = alloc_skb(pattern_len, GFP_KERNEL);
	if (!skb)
		goto fail_tx;
	memcpy(skb->data, pattern_buf, pattern_len);
	skb->len = pattern_len;

	if (test_bit(WMI_ENABLED, &ar->flag)) {
		if (skb_headroom(skb) < vif->needed_headroom) {
			struct sk_buff *tmp_skb = ath6kl_buf_alloc(skb->len);

			if (tmp_skb == NULL) {
				vif->net_stats.tx_dropped++;
				goto fail_tx;
			}

			skb_put(tmp_skb, skb->len);
			memcpy(tmp_skb->data, skb->data, skb->len);
			kfree_skb(skb);
			skb = tmp_skb;
		}

		PHdr.preamble[0] = 0xDE;
		PHdr.preamble[1] = 0xAD;
		PHdr.preamble[2] = 0xBE;
		PHdr.preamble[3] = 0xEF;
		PHdr.seqno = pattern_idx;
		PHdr.pktlen = pattern_len;
		PHdr.duration = (pattern_duration)*500;
		PHdr.schedule = 0;
		PHdr.chksum = 0;

		if (ath6kl_wmi_data_hdr_add(ar->wmi, skb, DATA_MSGTYPE,
			wmi_data_flags, 0,
			WMI_META_VERSION_2, (void *)&PHdr,
			vif->fw_vif_idx)) {
			ath6kl_err("wmi_data_hdr_add failed\n");
			goto fail_tx;
		}

		memcpy(skb->data+6, &PHdr, sizeof(PHdr));

		if ((vif->nw_type == ADHOC_NETWORK) &&
		     ar->ibss_ps_enable && test_bit(CONNECTED, &vif->flags)) {
			chk_adhoc_ps_mapping = true;
			ac = 0; /*ac is default 99 would be problematic, assign 0(BE) here*/
		} else {
			/* get the stream mapping */
			ret = ath6kl_wmi_implicit_create_pstream(ar->wmi,
					vif->fw_vif_idx, skb,
					0, test_bit(WMM_ENABLED, &vif->flags),
					&ac, &htc_tag);
			if (ret)
				goto fail_tx;
		}
	} else
		goto fail_tx;


	spin_lock_bh(&ar->lock);

	eid = ar->ac2ep_map[ac];

	if (eid == 0 || eid == ENDPOINT_UNUSED) {
		if ((ac == WMM_NUM_AC)) {
			/* for epping testing, the last AC maps to the control
			 * endpoint
			 */
			eid = ar->ctrl_ep;
		} else {
			ath6kl_err("eid %d is not mapped!\n", eid);
			spin_unlock_bh(&ar->lock);
			goto fail_tx;
		}
	}

	/* allocate resource for this packet */
	if (htc_tag == ATH6KL_DATA_PKT_TAG)
		cookie = ath6kl_alloc_cookie(ar, COOKIE_TYPE_DATA);
	else
		cookie = ath6kl_alloc_cookie(ar, COOKIE_TYPE_CTRL);

	if (!cookie) {
		spin_unlock_bh(&ar->lock);
		goto fail_tx;
	}

	/* update counts while the lock is held */
	ar->tx_pending[eid]++;
	ar->total_tx_data_pend++;

	spin_unlock_bh(&ar->lock);

	if (!IS_ALIGNED((unsigned long) skb->data - HTC_HDR_LENGTH, 4) &&
	    skb_cloned(skb)) {
		/*
		 * We will touch (move the buffer data to align it. Since the
		 * skb buffer is cloned and not only the header is changed, we
		 * have to copy it to allow the changes. Since we are copying
		 * the data here, we may as well align it by reserving suitable
		 * headroom to avoid the memmove in ath6kl_htc_tx_buf_align().
		 */
		struct sk_buff *nskb;

		nskb = skb_copy_expand(skb, HTC_HDR_LENGTH, 0, GFP_ATOMIC);
		if (nskb == NULL)
			goto fail_tx;
		kfree_skb(skb);
		skb = nskb;
	}

	cookie->skb = skb;
	cookie->map_no = map_no;
	set_htc_pkt_info(cookie->htc_pkt, cookie, skb->data, skb->len,
			 eid, htc_tag);
	cookie->htc_pkt->skb = skb;

	ar->tx_on_vif |= (1 << vif->fw_vif_idx);

	/*
	 * HTC interface is asynchronous, if this fails, cleanup will
	 * happen in the ath6kl_tx_complete callback.
	 */
	ath6kl_htc_tx(ar->htc_target, cookie->htc_pkt);

	kfree(pattern_buf);

	return count;

fail_tx:
	if (skb)
		dev_kfree_skb(skb);

	kfree(pattern_buf);

	vif->net_stats.tx_dropped++;
	vif->net_stats.tx_aborted_errors++;

	return -EINVAL;
}


static const struct file_operations fops_pattern_gen = {
	.write = ath6kl_patterngen_write,
	.open = ath6kl_debugfs_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};


static ssize_t ath6kl_wowpattern_write(struct file *file,
				     const char __user *user_buf,
				     size_t count, loff_t *ppos)
{
	struct ath6kl *ar = file->private_data;
	struct ath6kl_vif *vif;
	int ret;

	u8 pattern_idx = 0;
	u8 pattern_offset;
	u8 pattern_buf[128];
	u8 pattern_mask[16];
	u8 pattern_len;
	u8 mask_idx = 0;
	u32 host_req_delay = WOW_HOST_REQ_DELAY;

	/* save user command */
	u8 buf[512];
	unsigned int len = 0;
	char *sptr, *token;
	len = min(count, sizeof(buf) - 1);
	if (copy_from_user(buf, user_buf, len))
		return -EFAULT;
	buf[len] = '\0';
	sptr = buf;

	vif = ath6kl_vif_first(ar);
	if (!vif)
		return -EIO;


	/* get pattern Idx */
	token = strsep(&sptr, " ");
	if (!token)
		return -EINVAL;
	if (kstrtou8(token, 0, &pattern_idx))
		return -EINVAL;
	if (pattern_idx > 16)
		return -EINVAL;

	/* get pattern offset */
	token = strsep(&sptr, " ");

	/* delete wow pattern if no futher argument */
	if (!token) {
		ath6kl_wmi_del_wow_pattern_cmd(ar->wmi,
								vif->fw_vif_idx,
								WOW_LIST_ID,
								pattern_idx);
		return count;
	}
	if (kstrtou8(token, 0, &pattern_offset))
		return -EINVAL;
	/* get pattern path */
	token = strsep(&sptr, " ");
	if (!token)
		return -EINVAL;

	pattern_len = readpatternfile(token, pattern_buf, sizeof(pattern_buf));

	if (!pattern_len || pattern_len > 128)
		return -EINVAL;

	/* Reset wakeup delay time to 2 secs for sdio, keep 5 secs for
	 * usb now
	 */
	if (ar->hif_type == ATH6KL_HIF_TYPE_SDIO)
		host_req_delay = 2000;

	/* for hsic mode, reduce the wakeup time to 2 secs */
	if (BOOTSTRAP_IS_HSIC(ar->bootstrap_mode))
		host_req_delay = 2000;

	/* generate bytemask by pattern length */
	for (mask_idx = 0; mask_idx < pattern_len/8; mask_idx++)
		pattern_mask[mask_idx] = 0xff;

	if (pattern_len % 8)
		pattern_mask[mask_idx] = (1 << (pattern_len % 8)) - 1;

	ath6kl_wmi_del_wow_pattern_cmd(ar->wmi,
							vif->fw_vif_idx,
							WOW_LIST_ID,
							pattern_idx);

	ret = ath6kl_wmi_add_wow_ext_pattern_cmd(ar->wmi,
							vif->fw_vif_idx,
							WOW_LIST_ID,
							pattern_len,
							pattern_idx,
							pattern_offset,
							pattern_buf,
							pattern_mask);

	if (ret)
		return -EINVAL;
#ifndef CONFIG_ANDROID
	ret = ath6kl_wmi_set_wow_mode_cmd(ar->wmi, vif->fw_vif_idx,
					ATH6KL_WOW_MODE_ENABLE,
					WOW_FILTER_OPTION_PATTERNS,
					host_req_delay);

	if (ret)
		return -EINVAL;

	set_bit(WLAN_WOW_ENABLE, &vif->flags);
#endif
	ar->get_wow_pattern = true;

	return count;
}


static const struct file_operations fops_wowpattern_gen = {
	.write = ath6kl_wowpattern_write,
	.open = ath6kl_debugfs_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};


static ssize_t ath6kl_p2p_flowctrl_stat_read(struct file *file,
				char __user *user_buf,
				size_t count, loff_t *ppos)
{
#define _BUF_SIZE	(4096)
	struct ath6kl *ar = file->private_data;
	u8 *buf;
	unsigned int len;
	ssize_t ret_cnt;

	buf = kmalloc(_BUF_SIZE, GFP_ATOMIC);
	if (!buf)
		return -ENOMEM;

	len = ath6kl_p2p_flowctrl_stat(ar, buf, _BUF_SIZE);

	ret_cnt = simple_read_from_buffer(user_buf, count, ppos, buf, len);

	kfree(buf);

	return ret_cnt;
#undef _BUF_SIZE
}

/* debug fs for P2P Flowctrl Stats. */
static const struct file_operations fops_p2p_flowctrl_stat = {
	.read = ath6kl_p2p_flowctrl_stat_read,
	.open = ath6kl_debugfs_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

static ssize_t ath6kl_ap_ps_stat_read(struct file *file,
				char __user *user_buf,
				size_t count, loff_t *ppos)
{
#define _BUF_SIZE	(2048)
	struct ath6kl *ar = file->private_data;
	struct ath6kl_vif *vif;
	struct ath6kl_sta *conn;
	u8 *buf, *p;
	unsigned int len = 0;
	int i, j, buf_len, depth;
	u32 enq, enq_err, deq, age;
	ssize_t ret_cnt;

	buf = kmalloc(_BUF_SIZE, GFP_ATOMIC);
	if (!buf)
		return -ENOMEM;

	p = buf;
	buf_len = _BUF_SIZE;
	for (i = 0; i < ar->vif_max; i++) {
		vif = ath6kl_get_vif_by_index(ar, i);
		if ((vif) &&
			(vif->nw_type == AP_NETWORK)) {

			ath6kl_ps_queue_stat(&vif->psq_mcast,
					&depth, &enq, &enq_err, &deq, &age);
			len += scnprintf(p + len, buf_len - len,
					"VIF[%d] = psq_mcast %d/%d/%d/%d/%d\n",
					vif->fw_vif_idx,
					depth, enq, enq_err, deq, age);

			for (j = 0; j < AP_MAX_NUM_STA; j++) {
				conn = &vif->sta_list[j];

				len += scnprintf(p + len, buf_len - len,
					" STA - %02x:%02x:%02x:%02x:%02x:%02x aid %02d apsd %d state %02x phy %d",
					conn->mac[0], conn->mac[1],
					conn->mac[2], conn->mac[3],
					conn->mac[4], conn->mac[5],
					conn->aid,
					conn->apsd_info,
					conn->sta_flags,
					conn->phymode);

				ath6kl_ps_queue_stat(&conn->psq_data,
						&depth, &enq, &enq_err,
						&deq, &age);
				len += scnprintf(p + len, buf_len - len,
					" psq_data %d/%d/%d/%d/%d",
					depth, enq, enq_err, deq, age);

				ath6kl_ps_queue_stat(&conn->psq_mgmt,
						&depth, &enq, &enq_err,
						&deq, &age);
				len += scnprintf(p + len, buf_len - len,
					" psq_mgmt %d/%d/%d/%d/%d\n",
					depth, enq, enq_err, deq, age);
			}
		}
	}

	ret_cnt = simple_read_from_buffer(user_buf, count, ppos, buf, len);

	kfree(buf);

	return ret_cnt;
#undef _BUF_SIZE
}

/* debug fs for AP-PS Stats. */
static const struct file_operations fops_ap_ps_stat = {
	.read = ath6kl_ap_ps_stat_read,
	.open = ath6kl_debugfs_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

static int ath6kl_parse_rx_aggr_params(const char __user *user_buf,
				size_t count,
				struct ath6kl_vif *vif)
{
	char buf[64];
	char *p;
	unsigned int len;
	int value;
	u16 rx_aggr_timeout;


	len = min(count, sizeof(buf) - 1);
	if (copy_from_user(buf, user_buf, len))
		return -EFAULT;

	p = buf;

	SKIP_SPACE;

	sscanf(p, "%d", &value);
	rx_aggr_timeout = value;

	aggr_config(vif, rx_aggr_timeout);

	return 0;
}

/* File operation functions for RX Aggr. Params */
static ssize_t ath6kl_rx_aggr_params_write(struct file *file,
				const char __user *user_buf,
				size_t count, loff_t *ppos)
{
	struct ath6kl *ar = file->private_data;
	struct ath6kl_vif *vif;
	int i, ret;

	for (i = 0; i < ar->vif_max; i++) {
		vif = ath6kl_get_vif_by_index(ar, i);
		if ((vif) &&
			(vif->aggr_cntxt)) {
			ret = ath6kl_parse_rx_aggr_params(user_buf, count, vif);

			if (ret)
				return ret;
		}
	}

	return count;
}

static ssize_t ath6kl_rx_aggr_params_read(struct file *file,
				char __user *user_buf,
				size_t count, loff_t *ppos)
{
#define _BUF_SIZE	(512)
	struct ath6kl *ar = file->private_data;
	struct ath6kl_vif *vif;
	u8 *buf;
	unsigned int len = 0;
	int i;
	ssize_t ret_cnt;

	buf = kmalloc(_BUF_SIZE, GFP_ATOMIC);
	if (!buf)
		return -ENOMEM;

	for (i = 0; i < ar->vif_max; i++) {
		vif = ath6kl_get_vif_by_index(ar, i);
		if ((vif) &&
			(vif->aggr_cntxt)) {
			len += scnprintf(buf + len, _BUF_SIZE - len,
					"DEV-%d rx_aggr_timeout = %d ms\n",
					i,
					vif->aggr_cntxt->rx_aggr_timeout);
		}
	}

	ret_cnt = simple_read_from_buffer(user_buf, count, ppos, buf, len);

	kfree(buf);

	return ret_cnt;
#undef _BUF_SIZE
}

/* debug fs for RX Aggr. Params */
static const struct file_operations fops_rx_aggr_params = {
	.read = ath6kl_rx_aggr_params_read,
	.write = ath6kl_rx_aggr_params_write,
	.open = ath6kl_debugfs_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

/* File operation functions for RX bundle. Params */
static ssize_t ath6kl_rx_bundle_params_write(struct file *file,
				const char __user *user_buf,
				size_t count, loff_t *ppos)
{
	struct ath6kl *ar = file->private_data;
	char buf[16];
	char *p;
	int len, value;
	unsigned char min_rx_bundle_frame, rx_bundle_timeout;

	len = min(count, sizeof(buf) - 1);
	if (copy_from_user(buf, user_buf, len))
		return -EFAULT;

	p = buf;
	SKIP_SPACE;

	sscanf(p, "%d", &value);
	if (value > 20) {
		printk("min_rx_bundle_frame: < 20\n\r");
		value = 20;
	}
	if (value < 0) {
		printk("min_rx_bundle_frame: >= 0\n\r");
		value = 0;
	}
	min_rx_bundle_frame = value;

	SEEK_SPACE;
	SKIP_SPACE;

	sscanf(p, "%d", &value);
	if (value > 255) {
		printk("rx_bundle_timeout: <= 255\n\r");
		value = 255;
	}
	if (value < 0) {
		printk("rx_bundle_timeout: >= 0\n\r");
		value = 0;
	}
	rx_bundle_timeout = value;
	ath6kl_wmi_set_rx_bundle_param(ar->wmi, 0,
		min_rx_bundle_frame,rx_bundle_timeout);

	return count;
}

/* debug fs for RX bundle. Params */
static const struct file_operations fops_rx_bundle_params = {
	.write = ath6kl_rx_bundle_params_write,
	.open = ath6kl_debugfs_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

/* File operation functions for SCAN Params */
static int ath6kl_parse_scan_params(const char __user *user_buf, size_t count,
					int *maxact_chdwell_time,
					int *pas_chdwell_time,
					int *maxact_scan_per_ssid)
{
	char buf[16];
	char *p;
	int value;
	int len;

	len = min(count, sizeof(buf) - 1);
	if (copy_from_user(buf, user_buf, len))
		return -EFAULT;

	p = buf;

	SKIP_SPACE;

	sscanf(p, "%d", &value);
	if (value < ATH6KL_SCAN_ACT_DEWELL_TIME)
		value = ATH6KL_SCAN_ACT_DEWELL_TIME;
	*maxact_chdwell_time = value;

	SEEK_SPACE;
	SKIP_SPACE;

	sscanf(p, "%d", &value);
	if (value < ATH6KL_SCAN_PAS_DEWELL_TIME)
		value = ATH6KL_SCAN_PAS_DEWELL_TIME;
	*pas_chdwell_time = value;

	SEEK_SPACE;
	SKIP_SPACE;

	sscanf(p, "%d", &value);
	if (value <= ATH6KL_SCAN_PROBE_PER_SSID)
		value = 1;
	*maxact_scan_per_ssid = value;

	return 0;
}

static ssize_t ath6kl_scan_params_write(struct file *file,
				const char __user *user_buf,
				size_t count, loff_t *ppos)
{
	struct ath6kl *ar = file->private_data;
	struct ath6kl_vif *vif;
	int i, ret;
	int maxact_chdwell_time, pas_chdwell_time, maxact_scan_per_ssid;

	for (i = 0; i < ar->vif_max; i++) {
		vif = ath6kl_get_vif_by_index(ar, i);
		if (vif) {
			ret = ath6kl_parse_scan_params(user_buf, count,
							&maxact_chdwell_time,
							&pas_chdwell_time,
							&maxact_scan_per_ssid);
			if (!ret) {
				vif->sc_params.maxact_chdwell_time =
							maxact_chdwell_time;
				vif->sc_params.pas_chdwell_time =
							pas_chdwell_time;
				vif->sc_params.maxact_scan_per_ssid =
							maxact_scan_per_ssid;
				memcpy(&vif->sc_params_default, &vif->sc_params,
					sizeof(struct wmi_scan_params_cmd));
			} else
				return ret;
		}
	}

	return count;
}

static ssize_t ath6kl_scan_params_read(struct file *file,
				char __user *user_buf,
				size_t count, loff_t *ppos)
{
#define _BUF_SIZE	(512)
	struct ath6kl *ar = file->private_data;
	struct ath6kl_vif *vif;
	u8 *buf;
	unsigned int len = 0;
	int i;
	ssize_t ret_cnt;

	buf = kmalloc(_BUF_SIZE, GFP_ATOMIC);
	if (!buf)
		return -ENOMEM;

	for (i = 0; i < ar->vif_max; i++) {
		vif = ath6kl_get_vif_by_index(ar, i);
		if (vif) {
			len += scnprintf(buf + len, _BUF_SIZE - len,
				"DEV-%d Flags = 0x%x ActDwellTime = %d PasDwellTime = %d ScanPerSsid = %d\n",
				i,
				vif->sc_params.scan_ctrl_flags,
				(vif->sc_params.maxact_chdwell_time == 0 ?
					ATH6KL_SCAN_ACT_DEWELL_TIME :
					vif->sc_params.maxact_chdwell_time),
				(vif->sc_params.pas_chdwell_time == 0 ?
					ATH6KL_SCAN_PAS_DEWELL_TIME :
					vif->sc_params.pas_chdwell_time),
				(vif->sc_params.maxact_scan_per_ssid == 0 ?
					ATH6KL_SCAN_PROBE_PER_SSID :
					vif->sc_params.maxact_scan_per_ssid));
		}
	}

	ret_cnt = simple_read_from_buffer(user_buf, count, ppos, buf, len);

	kfree(buf);

	return ret_cnt;
#undef _BUF_SIZE
}

/* debug fs for SCAN Params */
static const struct file_operations fops_scan_params = {
	.read = ath6kl_scan_params_read,
	.write = ath6kl_scan_params_write,
	.open = ath6kl_debugfs_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

/* File operation functions for AP Keep-alive */
static ssize_t ath6kl_ap_keepalive_params_write(struct file *file,
				const char __user *user_buf,
				size_t count, loff_t *ppos)
{
	struct ath6kl *ar = file->private_data;
	struct ath6kl_vif *vif;
	char *p;
	int internal, cycle;
	char buf[32];
	ssize_t len;
	int i;

	len = min(count, sizeof(buf) - 1);
	if (copy_from_user(buf, user_buf, len))
		return -EFAULT;

	p = buf;

	SKIP_SPACE;

	sscanf(p, "%d", &internal);

	SEEK_SPACE;
	SKIP_SPACE;

	sscanf(p, "%d", &cycle);

	for (i = 0; i < ar->vif_max; i++) {
		vif = ath6kl_get_vif_by_index(ar, i);
		if (vif)
			ath6kl_ap_keepalive_config(vif,
						   (u32)internal,
						   (u32)cycle);
	}

	return count;
}

static ssize_t ath6kl_ap_keepalive_params_read(struct file *file,
				char __user *user_buf,
				size_t count, loff_t *ppos)
{
#define _BUF_SIZE	(256)
	struct ath6kl *ar = file->private_data;
	struct ath6kl_vif *vif;
	u8 *buf;
	unsigned int len = 0;
	int i;
	ssize_t ret_cnt;

	buf = kmalloc(_BUF_SIZE, GFP_ATOMIC);
	if (!buf)
		return -ENOMEM;

	for (i = 0; i < ar->vif_max; i++) {
		vif = ath6kl_get_vif_by_index(ar, i);
		if ((vif) &&
		    (vif->ap_keepalive_ctx)) {
			if (vif->ap_keepalive_ctx->flags &
				ATH6KL_AP_KA_FLAGS_BY_SUPP)
				len += scnprintf(buf + len, _BUF_SIZE - len,
					"int-%d offload to supplicant/hostapd\n",
					i);
			else
				len += scnprintf(buf + len, _BUF_SIZE - len,
				"int-%d flags %x, interval %d ms., cycle %d\n",
				i,
				vif->ap_keepalive_ctx->flags,
				vif->ap_keepalive_ctx->ap_ka_interval,
				vif->ap_keepalive_ctx->ap_ka_reclaim_cycle);
		}
	}

	ret_cnt = simple_read_from_buffer(user_buf, count, ppos, buf, len);

	kfree(buf);

	return ret_cnt;
#undef _BUF_SIZE
}

/* debug fs for AP Keep-alive */
static const struct file_operations fops_ap_keepalive_params = {
	.read = ath6kl_ap_keepalive_params_read,
	.write = ath6kl_ap_keepalive_params_write,
	.open = ath6kl_debugfs_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

static ssize_t ath6kl_tgt_ap_stat_read(struct file *file,
				char __user *user_buf,
				size_t count, loff_t *ppos)
{
#define _BUF_SIZE	(2048)
	struct ath6kl *ar = file->private_data;
	struct ath6kl_vif *vif;
	u8 *buf;
	u8 *p;
	unsigned int len = 0;
	int i, j, buf_len;
	long left;
	ssize_t ret_cnt;

	buf = kmalloc(_BUF_SIZE, GFP_ATOMIC);
	if (!buf)
		return -ENOMEM;

	p = buf;
	buf_len = _BUF_SIZE;
	for (i = 0; i < ar->vif_max; i++) {
		vif = ath6kl_get_vif_by_index(ar, i);

		if ((vif) &&
		    (vif->nw_type == AP_NETWORK)) {
			struct wmi_ap_mode_stat *ap_stats;

			if (down_interruptible(&ar->sem)) {
				ret_cnt = -EBUSY;
				goto FAIL;
			}

			set_bit(STATS_UPDATE_PEND, &vif->flags);

			if (ath6kl_wmi_get_stats_cmd(ar->wmi,
						vif->fw_vif_idx)) {
				up(&ar->sem);
				ret_cnt = -EIO;
				goto FAIL;
			}

			left = wait_event_interruptible_timeout(ar->event_wq,
						!test_bit(STATS_UPDATE_PEND,
						&vif->flags), WMI_TIMEOUT);
			up(&ar->sem);

			if (left <= 0) {
				ret_cnt = -ETIMEDOUT;
				goto FAIL;
			}

			len += scnprintf(p + len, buf_len - len, "VIF[%d]\n",
					vif->fw_vif_idx);

			ap_stats = &vif->ap_stats;
			for (j = 0; j < AP_MAX_NUM_STA; j++) {
				struct wmi_per_sta_stat *per_sta_stat =
							&ap_stats->sta[j];

				if (per_sta_stat->aid) {
					len += scnprintf(p + len, buf_len - len,
						" STA - mac=%02x:%02x:%02x:%02x:%02x:%02x\n"
						" AID %02d tx_bytes/pkts/error %d/%d/%d rx_bytes/pkts/error %d/%d/%d tx_ucast_rate %d last_txrx_time %d rssi=%d\n",
						per_sta_stat->mac[0],
						per_sta_stat->mac[1],
						per_sta_stat->mac[2],
						per_sta_stat->mac[3],
						per_sta_stat->mac[4],
						per_sta_stat->mac[5],
						per_sta_stat->aid,
						per_sta_stat->tx_bytes,
						per_sta_stat->tx_pkts,
						per_sta_stat->tx_error,
						per_sta_stat->rx_bytes,
						per_sta_stat->rx_pkts,
						per_sta_stat->rx_error,
						per_sta_stat->tx_ucast_rate,
						per_sta_stat->last_txrx_time,
						per_sta_stat->rsRssi);
				}
			}
		}
	}

	ret_cnt = simple_read_from_buffer(user_buf, count, ppos, buf, len);

FAIL:
	kfree(buf);

	return ret_cnt;
#undef _BUF_SIZE
}

/* debug fs for AP Stats. */
static const struct file_operations fops_tgt_ap_stats = {
	.read = ath6kl_tgt_ap_stat_read,
	.open = ath6kl_debugfs_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

static ssize_t ath6kl_txrx_error_stats_read(struct file *file,
				char __user *user_buf,
				size_t count, loff_t *ppos)
{
#define _BUF_SIZE	(2048)
	struct ath6kl *ar = file->private_data;
	struct ath6kl_vif *vif;
	u8 *buf;
	u8 *p;
	unsigned int len = 0;
	int i, j, buf_len;
	long left;
	ssize_t ret_cnt;

	buf = kmalloc(_BUF_SIZE, GFP_ATOMIC);
	if (!buf)
		return -ENOMEM;

	p = buf;
	buf_len = _BUF_SIZE;
	for (i = 0; i < ar->vif_max; i++) {
		vif = ath6kl_get_vif_by_index(ar, i);

		if (vif) {
			if (down_interruptible(&ar->sem)) {
				ret_cnt = -EBUSY;
				goto FAIL;
			}

			set_bit(STATS_UPDATE_PEND, &vif->flags);

			if (ath6kl_wmi_get_stats_cmd(ar->wmi,
						vif->fw_vif_idx)) {
				up(&ar->sem);
				ret_cnt = -EIO;
				goto FAIL;
			}

			left = wait_event_interruptible_timeout(ar->event_wq,
						!test_bit(STATS_UPDATE_PEND,
						&vif->flags), WMI_TIMEOUT);
			up(&ar->sem);

			if (left <= 0) {
				ret_cnt = -ETIMEDOUT;
				goto FAIL;
			}

			len += scnprintf(p + len, buf_len - len, "VIF[%d]\n",
					vif->fw_vif_idx);

			if(vif->nw_type == AP_NETWORK){
				struct wmi_ap_mode_stat *ap_stats;

				len += scnprintf(p + len, buf_len - len, "AP Mode\n");

				ap_stats = &vif->ap_stats;
				for (j = 0; j < AP_MAX_NUM_STA; j++) {
					struct wmi_per_sta_stat *per_sta_stat =
								&ap_stats->sta[j];

					if (per_sta_stat->aid) {
						len += scnprintf(p + len, buf_len - len,
							" STA - mac=%02x:%02x:%02x:%02x:%02x:%02x\n"
							" tx_pkts/error %d/%d rx_pkts/error %d/%d \n",
							per_sta_stat->mac[0],
							per_sta_stat->mac[1],
							per_sta_stat->mac[2],
							per_sta_stat->mac[3],
							per_sta_stat->mac[4],
							per_sta_stat->mac[5],
							per_sta_stat->tx_pkts,
							per_sta_stat->tx_error,
							per_sta_stat->rx_pkts,
							per_sta_stat->rx_error);
					}
				}
			}else{
				struct target_stats *tgt_stats;

				len += scnprintf(p + len, buf_len - len, "STA Mode\n");
				tgt_stats = &vif->target_stats;
				
				len += scnprintf(buf + len, buf_len - len, "%20s %10llu\n",
						 "Tx packets", tgt_stats->tx_ucast_pkt);
				len += scnprintf(buf + len, buf_len - len, "%20s %10llu\n",
						 "Tx Errors", tgt_stats->tx_err);
				len += scnprintf(buf + len, buf_len - len, "%20s %10llu\n",
						 "Rx packets", tgt_stats->rx_ucast_pkt);
				len += scnprintf(buf + len, buf_len - len, "%20s %10llu\n",
						 "Rx Errors", tgt_stats->rx_err);
				
				if (len > buf_len)
					len = buf_len;
			}			
		}
	}

	ret_cnt = simple_read_from_buffer(user_buf, count, ppos, buf, len);

FAIL:
	kfree(buf);

	return ret_cnt;
#undef _BUF_SIZE
}

/* debug fs for AP Stats. */
static const struct file_operations fops_txrx_error_stats = {
	.read = ath6kl_txrx_error_stats_read,
	.open = ath6kl_debugfs_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

void __chan_flag_to_string(u32 flags, u8 *string, int str_len)
{
	u8 *p = string;
	int len = 0;

	string[0] = '\0';

	if (flags & IEEE80211_CHAN_DISABLED)
		len += scnprintf(p + len, str_len - len,
						"%s",
						"[DISABLE]");

	if (flags & IEEE80211_CHAN_PASSIVE_SCAN)
		len += scnprintf(p + len, str_len - len,
						"%s",
						"[PASSIVE_SCAN]");

	if (flags & IEEE80211_CHAN_NO_IBSS)
		len += scnprintf(p + len, str_len - len,
						"%s",
						"[NO_IBSS]");

	if (flags & IEEE80211_CHAN_RADAR)
		len += scnprintf(p + len, str_len - len,
						"%s",
						"[RADAR]");

	len += scnprintf(p + len, str_len - len,
					"%s",
					"[HT20]");

	if ((flags & IEEE80211_CHAN_NO_HT40PLUS) &&
	    (flags & IEEE80211_CHAN_NO_HT40MINUS)) {
		;
	} else {
		if (flags & IEEE80211_CHAN_NO_HT40PLUS)
			len += scnprintf(p + len, str_len - len,
							"%s",
							"[HT40-]");
		else if (flags & IEEE80211_CHAN_NO_HT40MINUS)
			len += scnprintf(p + len, str_len - len,
							"%s",
							"[HT40+]");
		else
			len += scnprintf(p + len, str_len - len,
							"%s",
							"[HT40+][HT40-]");
	}

	return;
}

static ssize_t ath6kl_chan_list_read(struct file *file,
				char __user *user_buf,
				size_t count, loff_t *ppos)
{
#define _BUF_SIZE	(2048)
	struct ath6kl *ar = file->private_data;
	struct wiphy *wiphy = ar->wiphy;
	struct reg_info *reg = ar->reg_ctx;
	u8 *buf, *p;
	u8 flag_string[96];
	unsigned int len = 0;
	int i, buf_len;
	enum ieee80211_band band;
	ssize_t ret_cnt;

	if (!reg)
		return 0;

	buf = kmalloc(_BUF_SIZE, GFP_ATOMIC);
	if (!buf)
		return -ENOMEM;

	p = buf;
	buf_len = _BUF_SIZE;

	if (reg->current_regd) {
		len += scnprintf(p + len, buf_len - len,
				"\nCurrent Regulatory - %08x %c%c %d rules\n",
				reg->current_reg_code,
				reg->current_regd->alpha2[0],
				reg->current_regd->alpha2[1],
				reg->current_regd->n_reg_rules);

		for (i = 0; i < reg->current_regd->n_reg_rules; i++) {
			struct ieee80211_reg_rule *regRule;

			regRule = &reg->current_regd->reg_rules[i];
			len += scnprintf(p + len, buf_len - len,
						"\t[%d - %d, HT%d]\n",
				regRule->freq_range.start_freq_khz / 1000,
				regRule->freq_range.end_freq_khz / 1000,
				regRule->freq_range.max_bandwidth_khz / 1000);
		}
	}

	for (band = 0; band < IEEE80211_NUM_BANDS; band++) {
		if (!wiphy->bands[band])
			continue;

		len += scnprintf(p + len, buf_len - len, "\n%s\n",
				(band == IEEE80211_BAND_2GHZ ?
				"2.4GHz" : "5GHz"));

		for (i = 0; i < wiphy->bands[band]->n_channels; i++) {
			struct ieee80211_channel *chan;

			chan = &wiphy->bands[band]->channels[i];

			if (chan->flags & IEEE80211_CHAN_DISABLED)
				continue;

			__chan_flag_to_string(chan->flags, flag_string, 96);
			len += scnprintf(p + len, buf_len - len,
					" CH%4d - %4d %s\n",
					chan->hw_value,
					chan->center_freq,
					flag_string);
		}
	}

	len += scnprintf(p + len, buf_len - len,
			"\nCurrent operation channel\n");

	for (i = 0; i < ar->vif_max; i++) {
		struct ath6kl_vif *vif = ath6kl_get_vif_by_index(ar, i);

		if (vif)
			len += scnprintf(p + len, buf_len - len,
					" VIF%d [%s] - ch %d phy %d type %d\n",
					i,
					(test_bit(CONNECTED, &vif->flags) ?
						"CONN" : "IDLE"),
					vif->bss_ch,
					vif->phymode,
					vif->chan_type);
	}

	ret_cnt = simple_read_from_buffer(user_buf, count, ppos, buf, len);

	kfree(buf);

	return ret_cnt;
#undef _BUF_SIZE
}

/* debug fs for Channel List. */
static const struct file_operations fops_chan_list = {
	.read = ath6kl_chan_list_read,
	.open = ath6kl_debugfs_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

/* File operation functions for AP ACL */
static ssize_t ath6kl_ap_acl_policy_write(struct file *file,
				const char __user *user_buf,
				size_t count, loff_t *ppos)
{
	struct ath6kl *ar = file->private_data;
	struct ath6kl_vif *vif;
	char *p;
	int policy;
	char buf[32];
	ssize_t len;
	int i;

#ifdef NL80211_CMD_SET_AP_MAC_ACL
	/* Configurate from NL80211. */
	if (ar->wiphy->max_acl_mac_addrs)
		return -EINVAL;
#endif

	len = min(count, sizeof(buf) - 1);
	if (copy_from_user(buf, user_buf, len))
		return -EFAULT;

	p = buf;

	SKIP_SPACE;

	sscanf(p, "%d", &policy);

	for (i = 0; i < ar->vif_max; i++) {
		vif = ath6kl_get_vif_by_index(ar, i);

		/* Now, only for softap mode, not P2P-GO mode */
		if ((vif) &&
		    (vif->wdev.iftype == NL80211_IFTYPE_AP))
			ath6kl_ap_acl_config_policy(vif, (u8)policy);
	}

	return count;
}

static ssize_t ath6kl_ap_acl_policy_read(struct file *file,
				char __user *user_buf,
				size_t count, loff_t *ppos)
{
#define _BUF_SIZE	(256)
	struct ath6kl *ar = file->private_data;
	struct ath6kl_vif *vif;
	u8 *buf;
	unsigned int len = 0;
	int i;
	ssize_t ret_cnt;

	buf = kmalloc(_BUF_SIZE, GFP_ATOMIC);
	if (!buf)
		return -ENOMEM;

	for (i = 0; i < ar->vif_max; i++) {
		vif = ath6kl_get_vif_by_index(ar, i);
		if ((vif) &&
		    (vif->ap_acl_ctx)) {
			struct ap_acl_info *ap_acl = vif->ap_acl_ctx;

			len += scnprintf(buf + len, _BUF_SIZE - len,
				"int-%d acl_policy %s\n",
				vif->fw_vif_idx,
				(ap_acl->acl_mode ?
					(ap_acl->acl_mode == AP_ACL_MODE_DENY ?
						"DENY" : "ALLOW") :
					"DISABLED"));
		}
	}

	ret_cnt = simple_read_from_buffer(user_buf, count, ppos, buf, len);

	kfree(buf);

	return ret_cnt;
#undef _BUF_SIZE
}

/* debug fs for AP ACL */
static const struct file_operations fops_ap_acl_policy = {
	.read = ath6kl_ap_acl_policy_read,
	.write = ath6kl_ap_acl_policy_write,
	.open = ath6kl_debugfs_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

/* File operation functions for AP ACL MAC-LIST */
static ssize_t ath6kl_ap_acl_mac_list_write(struct file *file,
				const char __user *user_buf,
				size_t count, loff_t *ppos)
{
	struct ath6kl *ar = file->private_data;
	struct ath6kl_vif *vif;
	char *p;
	bool removed;
	char buf[32];
	u8 mac_addr[ETH_ALEN];
	int addr[ETH_ALEN];
	ssize_t len;
	int i;

#ifdef NL80211_CMD_SET_AP_MAC_ACL
	/* Configurate from NL80211. */
	if (ar->wiphy->max_acl_mac_addrs)
		return -EINVAL;
#endif

	len = min(count, sizeof(buf) - 1);
	if (copy_from_user(buf, user_buf, len))
		return -EFAULT;

	p = buf;

	SKIP_SPACE;

	if (sscanf(p, "%02x:%02x:%02x:%02x:%02x:%02x",
		   &addr[0], &addr[1], &addr[2],
		   &addr[3], &addr[4], &addr[5]) != ETH_ALEN)
		return -EINVAL;

	SEEK_SPACE;
	SKIP_SPACE;

	if (strstr(p, "r"))
		removed = true;
	else
		removed = false;

	for (i = 0; i < ETH_ALEN; i++)
		mac_addr[i] = (u8)addr[i];

	for (i = 0; i < ar->vif_max; i++) {
		vif = ath6kl_get_vif_by_index(ar, i);

		/* Now, only for softap mode, not P2P-GO mode */
		if ((vif) &&
		    (vif->wdev.iftype == NL80211_IFTYPE_AP))
			ath6kl_ap_acl_config_mac_list(vif,
						mac_addr,
						removed);
	}

	return count;
}

static ssize_t ath6kl_ap_acl_mac_list_read(struct file *file,
				char __user *user_buf,
				size_t count, loff_t *ppos)
{
#define _BUF_SIZE	(2048)
	struct ath6kl *ar = file->private_data;
	u8 *buf;
	unsigned int len = 0;
	ssize_t ret_cnt;

	buf = kmalloc(_BUF_SIZE, GFP_ATOMIC);
	if (!buf)
		return -ENOMEM;

	len = ath6kl_ap_acl_dump(ar, buf, _BUF_SIZE);

	ret_cnt = simple_read_from_buffer(user_buf, count, ppos, buf, len);

	kfree(buf);

	return ret_cnt;
#undef _BUF_SIZE
}

/* debug fs for AP ACL MAC-LIST */
static const struct file_operations fops_ap_acl_mac_list = {
	.read = ath6kl_ap_acl_mac_list_read,
	.write = ath6kl_ap_acl_mac_list_write,
	.open = ath6kl_debugfs_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

/* File operation functions for AMPDU */
static ssize_t ath6kl_ampdu_write(struct file *file,
				const char __user *user_buf,
				size_t count, loff_t *ppos)
{
	struct ath6kl *ar = file->private_data;
	struct ath6kl_vif *vif;
	u32 ampdu;
	char buf[32];
	ssize_t len;
	int i;

	len = min(count, sizeof(buf) - 1);
	if (copy_from_user(buf, user_buf, len))
		return -EFAULT;

	buf[len] = '\0';
	if (kstrtou32(buf, 0, &ampdu))
		return -EINVAL;

	for (i = 0; i < ar->vif_max; i++) {
		vif = ath6kl_get_vif_by_index(ar, i);
		/* only support 8 TIDs now. */
		if (vif)
			ath6kl_wmi_allow_aggr_cmd(vif->ar->wmi,
						vif->fw_vif_idx,
						(ampdu ? 0xff : 0),
						(ampdu ? 0xff : 0));
	}

	return count;
}

/* debug fs for AMPDU */
static const struct file_operations fops_ampdu = {
	.write = ath6kl_ampdu_write,
	.open = ath6kl_debugfs_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

static int ath6kl_parse_arp_offload_ip_addrs(const char __user *user_buf,
				size_t count,
				u8 *ip_addrs)
{
	char buf[64];
	char *p;
	unsigned int len;
	int value, i;

	len = min(count, sizeof(buf) - 1);
	if (copy_from_user(buf, user_buf, len))
		return -EFAULT;

	p = buf;

	SKIP_SPACE;

	for (i = 0 ; i < 4; i++) {
		sscanf(p, "%d", &value);
		if (value > 0xff) {
			/*Wrong value should be less than 255*/
			return -EFAULT;
		}
		*(ip_addrs+i) = value;
		if (i < 3) {
			SEEK_SPACE;
			SKIP_SPACE;
		}
	}

	return 0;
}

static ssize_t ath6kl_arp_offload_ipaddrs_write(struct file *file,
				const char __user *user_buf,
				size_t count, loff_t *ppos)
{
	struct ath6kl *ar = file->private_data;
	struct ath6kl_vif *vif;
	int ret;
	u8 ip_addrs[4];

	vif = ath6kl_vif_first(ar);
	if (!vif)
		return -EIO;

	vif->arp_offload_ip_set = 0;
	ret = ath6kl_parse_arp_offload_ip_addrs(user_buf, count, ip_addrs);

	if (ret)
		return ret;

	if (ath6kl_wmi_set_arp_offload_ip_cmd(ar->wmi,
			vif->fw_vif_idx, ip_addrs))
		return -EIO;
	vif->arp_offload_ip_set = 1;

	return count;
}

/* debug fs for ARP OFFLOAD */
static const struct file_operations fops_arp_offload_ip_addrs = {
	.write = ath6kl_arp_offload_ipaddrs_write,
	.open = ath6kl_debugfs_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

#define TBTT_MCC_STA_LOWER_BOUND      20
#define TBTT_MCC_STA_UPPER_BOUND      80
/* File operation for mcc profile */
static ssize_t ath6kl_mcc_profile(struct file *file,
				const char __user *user_buf,
				size_t count, loff_t *ppos)
{
	struct ath6kl *ar = file->private_data;
	char buf[10];
	ssize_t len;
	u32 mcc_profile;
	u32 sta_time;
	len = min(count, sizeof(buf) - 1);
	if (copy_from_user(buf, user_buf, len))
		return -EFAULT;

	buf[len] = '\0';
	if (kstrtou32(buf, 0, &mcc_profile))
		return -EINVAL;
	sta_time = (mcc_profile & 0x0F)*10;
	if (mcc_profile & WMI_MCC_DUAL_TIME_MASK) {
		if (sta_time > TBTT_MCC_STA_UPPER_BOUND ||
			sta_time < TBTT_MCC_STA_LOWER_BOUND) {
			return -EINVAL;
		}
	}
	if (ath6kl_wmi_set_mcc_profile_cmd(ar->wmi, mcc_profile))
		return -EIO;
	return count;
}

/* debug fs for mcc profile */
static const struct file_operations fops_mcc_profile = {
	.write = ath6kl_mcc_profile,
	.open = ath6kl_debugfs_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

/* File operation for seamless mcc/scc switch */
static ssize_t ath6kl_seamless_mcc_scc_switch(struct file *file,
				const char __user *user_buf,
				size_t count, loff_t *ppos)
{
	struct ath6kl *ar = file->private_data;
	char buf[10];
	ssize_t len;
	u32 freq;

	len = min(count, sizeof(buf) - 1);
	if (copy_from_user(buf, user_buf, len))
		return -EFAULT;

	buf[len] = '\0';
	if (kstrtou32(buf, 0, &freq))
		return -EINVAL;

	if (ath6kl_wmi_set_seamless_mcc_scc_switch_freq_cmd(ar->wmi, freq))
		return -EIO;

	return count;
}

/* debug fs for seamless mcc/scc switch */
static const struct file_operations fops_seamless_mcc_scc_switch = {
	.write = ath6kl_seamless_mcc_scc_switch,
	.open = ath6kl_debugfs_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

/* File operation for P2P Frame Retry */
static ssize_t ath6kl_p2p_frame_retry_write(struct file *file,
				const char __user *user_buf,
				size_t count, loff_t *ppos)
{
	struct ath6kl *ar = file->private_data;
	u32 p2p_frame_retry;
	char buf[32];
	ssize_t len;

	len = min(count, sizeof(buf) - 1);
	if (copy_from_user(buf, user_buf, len))
		return -EFAULT;

	buf[len] = '\0';
	if (kstrtou32(buf, 0, &p2p_frame_retry))
		return -EINVAL;

	if (p2p_frame_retry)
		ar->p2p_frame_retry = true;
	else
		ar->p2p_frame_retry = false;

	return count;
}

/* debug fs for P2P Frame Retry */
static const struct file_operations fops_p2p_frame_retry = {
	.write = ath6kl_p2p_frame_retry_write,
	.open = ath6kl_debugfs_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

/* File operation for P2P Frame Conditional Reject */
static ssize_t ath6kl_p2p_frame_cond_reject_write(struct file *file,
				const char __user *user_buf,
				size_t count, loff_t *ppos)
{
	struct ath6kl *ar = file->private_data;
	u32 p2p_frame_cond_reject;
	char buf[32];
	ssize_t len;

	len = min(count, sizeof(buf) - 1);
	if (copy_from_user(buf, user_buf, len))
		return -EFAULT;

	buf[len] = '\0';
	if (kstrtou32(buf, 0, &p2p_frame_cond_reject))
		return -EINVAL;

	if (p2p_frame_cond_reject)
		ar->p2p_frame_not_report = true;
	else
		ar->p2p_frame_not_report = false;

	return count;
}

/* debug fs for P2P Frame Conditional Reject */
static const struct file_operations p2p_frame_cond_reject = {
	.write = ath6kl_p2p_frame_cond_reject_write,
	.open = ath6kl_debugfs_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

static ssize_t ath6kl_debug_quirks_write(struct file *file,
					const char __user *user_buf,
					size_t count, loff_t *ppos)
{
	struct ath6kl *ar = file->private_data;
	char buf[32];
	ssize_t len;
	char *p;
	u32 quirks, reset;

	len = min(count, sizeof(buf) - 1);
	if (copy_from_user(buf, user_buf, len))
		return -EFAULT;

	p = buf;

	SKIP_SPACE;

	sscanf(p, "0x%x", &quirks);

	SEEK_SPACE;
	SKIP_SPACE;

	sscanf(p, "%d", &reset);

	debug_quirks = quirks;
	ar->mod_debug_quirks = debug_quirks;

	if (reset)
		ath6kl_reset_device(ar, ar->target_type, true, true);

	return count;
}

static ssize_t ath6kl_debug_quirks_read(struct file *file,
					char __user *user_buf,
					size_t count, loff_t *ppos)
{
	char buf[32];
	int len;

	len = snprintf(buf, sizeof(buf), "debug_quirks: 0x%x\n", debug_quirks);

	return simple_read_from_buffer(user_buf, count, ppos, buf, len);
}

static const struct file_operations fops_debug_quirks = {
	.read = ath6kl_debug_quirks_read,
	.write = ath6kl_debug_quirks_write,
	.open = ath6kl_debugfs_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

static ssize_t ath6kl_debug_disable_scan(struct file *file,
					const char __user *user_buf,
					size_t count, loff_t *ppos)
{
	struct ath6kl *ar = file->private_data;
	char buf[32];
	ssize_t len;
	u32 value;

	len = min(count, sizeof(buf) - 1);
	if (copy_from_user(buf, user_buf, len))
		return -EFAULT;

	buf[len] = '\0';
	if (kstrtou32(buf, 0, &value))
		return -EINVAL;

	if (value)
		set_bit(DISABLE_SCAN, &ar->flag);
	else
		clear_bit(DISABLE_SCAN, &ar->flag);

	return count;
}

static const struct file_operations fops_disable_scan = {
	.write = ath6kl_debug_disable_scan,
	.open = ath6kl_debugfs_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

/* File operation for P2P Frame Conditional Reject */
static ssize_t ath6kl_disable_runtime_flowctrl_write(struct file *file,
				const char __user *user_buf,
				size_t count, loff_t *ppos)
{
	struct ath6kl *ar = file->private_data;
	u32 value;
	char buf[32];
	ssize_t len;

	len = min(count, sizeof(buf) - 1);
	if (copy_from_user(buf, user_buf, len))
		return -EFAULT;

	buf[len] = '\0';
	if (kstrtou32(buf, 0, &value))
		return -EINVAL;

	if (value)
		ar->conf_flags |= ATH6KL_CONF_DISABLE_SKIP_FLOWCTRL;
	else
		ar->conf_flags &= ~ATH6KL_CONF_DISABLE_SKIP_FLOWCTRL;

	if (ar->conf_flags & ATH6KL_CONF_ENABLE_FLOWCTRL) {
		if (ar->conf_flags & ATH6KL_CONF_DISABLE_SKIP_FLOWCTRL)
			clear_bit(SKIP_FLOWCTRL_EVENT, &ar->flag);
		else {
			if (test_bit(MCC_ENABLED, &ar->flag))
				clear_bit(SKIP_FLOWCTRL_EVENT, &ar->flag);
			else
				set_bit(SKIP_FLOWCTRL_EVENT, &ar->flag);
		}
	} else
		clear_bit(SKIP_FLOWCTRL_EVENT, &ar->flag);

	return count;
}

static ssize_t ath6kl_disable_runtime_flowctrl_read(struct file *file,
					char __user *user_buf,
					size_t count, loff_t *ppos)
{
	struct ath6kl *ar = file->private_data;
	char buf[16];
	int len;

	len = snprintf(buf, sizeof(buf), "%d %d\n",
			(ar->conf_flags &
			ATH6KL_CONF_DISABLE_SKIP_FLOWCTRL) ? 1 : 0,
			test_bit(SKIP_FLOWCTRL_EVENT, &ar->flag));

	return simple_read_from_buffer(user_buf, count, ppos, buf, len);
}


static const struct file_operations fops_disable_runtime_flowctrl = {
	.read = ath6kl_disable_runtime_flowctrl_read,
	.write = ath6kl_disable_runtime_flowctrl_write,
	.open = ath6kl_debugfs_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};
#ifdef USB_AUTO_SUSPEND

int debugfs_get_pm_state(struct ath6kl *usbpm_ar)
{
	return usbpm_ar->state;
}

static ssize_t ath6kl_usb_autopm_usagecnt_write(struct file *file,
				const char __user *user_buf,
				size_t count, loff_t *ppos)
{
	int ret;
	int value;
	struct ath6kl *ar = file->private_data;

	ret = kstrtou32_from_user(user_buf, count, 0, &value);

	if (ret)
		return ret;

	if (value == 0) {
		ath6kl_hif_auto_pm_enable(ar);
		ath6kl_dbg(ATH6KL_DBG_ANY, ("auto pm -1\n"));
	} else if (value == 1) {
		ath6kl_hif_auto_pm_disable(ar);
		ath6kl_dbg(ATH6KL_DBG_ANY, ("auto pm +1\n"));
	}

	return count;

}
static ssize_t ath6kl_usb_autopm_usagecnt_read(struct file *file,
				char __user *user_buf,
				size_t count, loff_t *ppos)
{
	char buf[32];
	int len;
	int usb_auto_usagecnt;
	struct ath6kl *ar = file->private_data;

	usb_auto_usagecnt = ath6kl_hif_auto_pm_get_usage_cnt(ar);

	len = snprintf(buf, sizeof(buf),
		"usbautopm: 0x%x\n", usb_auto_usagecnt);

	return simple_read_from_buffer(user_buf, count, ppos, buf, len);
}

static const struct file_operations fops_usb_autopm_usagecnt = {
	.read = ath6kl_usb_autopm_usagecnt_read,
	.write = ath6kl_usb_autopm_usagecnt_write,
	.open = ath6kl_debugfs_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

static char *ar_state[]  = {
	"OFF",
	"ON",
	"DEEPSLEEP",
	"CUTPOWER",
	"WOW",
	"SUSPEND"
};

static ssize_t ath6kl_usb_pm_status_read(struct file *file,
				char __user *user_buf,
				size_t count, loff_t *ppos)
{
	char buf[256];
	int len;
	int state;
	int buf_len;
	struct usb_pm_skb_queue_t *p_usb_pm_skb_queue;
	struct ath6kl *ar = file->private_data;

	buf_len = sizeof(buf);

	state = debugfs_get_pm_state(ar);

	len = snprintf(buf, sizeof(buf), "state: %s\n", ar_state[state]);

	p_usb_pm_skb_queue =  &ar->usb_pm_skb_queue;

	len += snprintf(buf + len, buf_len - len, "usb_pm_q depth: %d\n",
		get_queue_depth(&(p_usb_pm_skb_queue->list)));

	return simple_read_from_buffer(user_buf, count, ppos, buf, len);
}

static const struct file_operations fops_usb_pm_status = {
	.read = ath6kl_usb_pm_status_read,
	.open = ath6kl_debugfs_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

#endif /* USB_AUTO_SUSPEND */

/* File operation for P2P IE not append */
static ssize_t ath6kl_p2p_ie_not_append_write(struct file *file,
				const char __user *user_buf,
				size_t count, loff_t *ppos)
{
	struct ath6kl *ar = file->private_data;
	u32 p2p_ie_not_append;
	char buf[32];
	ssize_t len;

	len = min(count, sizeof(buf) - 1);
	if (copy_from_user(buf, user_buf, len))
		return -EFAULT;

	buf[len] = '\0';
	if (kstrtou32(buf, 0, &p2p_ie_not_append))
		return -EINVAL;

	ar->p2p_ie_not_append = 0;
	if (p2p_ie_not_append & P2P_IE_IN_PROBE_REQ)
		ar->p2p_ie_not_append |= P2P_IE_IN_PROBE_REQ;

	if (p2p_ie_not_append & P2P_IE_IN_ASSOC_REQ)
		ar->p2p_ie_not_append |= P2P_IE_IN_ASSOC_REQ;

	return count;
}

static ssize_t ath6kl_p2p_ie_not_append_read(struct file *file,
					char __user *user_buf,
					size_t count, loff_t *ppos)
{
	struct ath6kl *ar = file->private_data;
	struct ath6kl_vif *vif;
	char buf[64];
	int len = 0;

	vif = ath6kl_vif_first(ar);
	if (!vif)
		return -EIO;

	if ((ar->p2p_concurrent) &&
	    (vif->nw_type == INFRA_NETWORK)) {
		len = snprintf(buf, sizeof(buf),
				"ProbeReq %s append, AssocReq %s append\n",
				((ar->p2p_ie_not_append & P2P_IE_IN_PROBE_REQ) ?
					"NOT" : ""),
				((ar->p2p_ie_not_append & P2P_IE_IN_ASSOC_REQ) ?
					"NOT" : ""));
	}

	return simple_read_from_buffer(user_buf, count, ppos, buf, len);
}

/* debug fs for P2P IE not append */
static const struct file_operations fops_p2p_ie_not_append = {
	.read = ath6kl_p2p_ie_not_append_read,
	.write = ath6kl_p2p_ie_not_append_write,
	.open = ath6kl_debugfs_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

/* File operation for AP Admission-Control */
static ssize_t ath6kl_ap_admc_read(struct file *file,
				char __user *user_buf,
				size_t count, loff_t *ppos)
{
#define _BUF_SIZE	(256)
	struct ath6kl *ar = file->private_data;
	u8 *buf;
	unsigned int len = 0;
	ssize_t ret_cnt;

	buf = kmalloc(_BUF_SIZE, GFP_ATOMIC);
	if (!buf)
		return -ENOMEM;

	len = ath6kl_ap_admc_dump(ar, buf, _BUF_SIZE);

	ret_cnt = simple_read_from_buffer(user_buf, count, ppos, buf, len);

	kfree(buf);

	return ret_cnt;
#undef _BUF_SIZE
}

/* debug fs for AP Admission-Control. */
static const struct file_operations fops_ap_admc = {
	.read = ath6kl_ap_admc_read,
	.open = ath6kl_debugfs_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

/* File operation for P2P RecommendChannel */
static ssize_t ath6kl_p2p_rc_write(struct file *file,
				const char __user *user_buf,
				size_t count, loff_t *ppos)
{
	struct ath6kl *ar = file->private_data;
	u32 tmp;
	int type = 0;
	u16 freq = 0;
	char buf[8];
	ssize_t len;

	len = min(count, sizeof(buf) - 1);
	if (copy_from_user(buf, user_buf, len))
		return -EFAULT;

	buf[len] = '\0';
	if (kstrtou32(buf, 0, &tmp))
		return -EINVAL;

	if (tmp == P2P_RC_USER_BLACK_CHAN)
		type = P2P_RC_USER_BLACK_CHAN;
	else if (tmp == P2P_RC_USER_WHITE_CHAN)
		type = P2P_RC_USER_WHITE_CHAN;
	else if ((tmp >= 2412) && (tmp <= 5825))
		freq = (u16)tmp;
	else
		return -EINVAL;

	ath6kl_p2p_rc_config(ar, freq, (freq ? -1 : type));

	return count;
}

static ssize_t ath6kl_p2p_rc_read(struct file *file,
				char __user *user_buf,
				size_t count, loff_t *ppos)
{
#define _BUF_SIZE	(2048)
	struct ath6kl *ar = file->private_data;
	u8 *buf;
	unsigned int len = 0;
	ssize_t ret_cnt;

	buf = kmalloc(_BUF_SIZE, GFP_ATOMIC);
	if (!buf)
		return -ENOMEM;

	len = ath6kl_p2p_rc_dump(ar, buf, _BUF_SIZE);

	ret_cnt = simple_read_from_buffer(user_buf, count, ppos, buf, len);

	kfree(buf);

	return ret_cnt;
#undef _BUF_SIZE
}

/* debug fs for P2P RecommendChannel. */
static const struct file_operations fops_p2p_rc = {
	.read = ath6kl_p2p_rc_read,
	.write = ath6kl_p2p_rc_write,
	.open = ath6kl_debugfs_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

/* File operation functions for HIF-PIPE RX Queue threshold */
static ssize_t ath6kl_hif_pipe_rxq_threshold_write(struct file *file,
		const char __user *user_buf,
		size_t count, loff_t *ppos)
{
	struct ath6kl *ar = file->private_data;
	char buf[64];
	char *sptr, *token;
	unsigned int len;
	u32 rxq_threshold = 0;

	if (ar->hif_type != ATH6KL_HIF_TYPE_USB)
		return -EIO;

	len = min(count, sizeof(buf) - 1);
	if (copy_from_user(buf, user_buf, len))
		return -EFAULT;

	buf[len] = '\0';

	sptr = buf;

	token = strsep(&sptr, " ");
	if (!token)
		return -EINVAL;
	if (kstrtou32(token, 0, &rxq_threshold))
		return -EINVAL;

	ath6kl_hif_pipe_set_rxq_threshold(ar, rxq_threshold);

	return count;
}

/* debug fs for HIF-PIPE Max. Schedule packages */
static const struct file_operations fops_hif_pipe_rxq_threshold = {
	.write = ath6kl_hif_pipe_rxq_threshold_write,
	.open = ath6kl_debugfs_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

static ssize_t ath6kl_skb_dup_read(struct file *file,
				      char __user *user_buf,
				      size_t count, loff_t *ppos)
{
	struct ath6kl *ar = file->private_data;
	char buf[64];
	unsigned int len;

	len = snprintf(buf, sizeof(buf), "skb_duplicate %s\n",
			(ar->conf_flags & ATH6KL_CONF_SKB_DUP)
			? "enabled" : "disabled");

	return simple_read_from_buffer(user_buf, count, ppos, buf, len);
}

static ssize_t ath6kl_skb_dup_write(struct file *file,
				      const char __user *user_buf,
				      size_t count, loff_t *ppos)
{
	struct ath6kl *ar = file->private_data;
	char buf[20];
	size_t len;

	len = min(count, sizeof(buf) - 1);
	if (copy_from_user(buf, user_buf, len))
		return -EFAULT;

	buf[len] = '\0';
	if (len > 0 && buf[len - 1] == '\n')
		buf[len - 1] = '\0';

	if (strcasecmp(buf, "disable") == 0)
		ar->conf_flags &=
			~ATH6KL_CONF_SKB_DUP;
	else if (strcasecmp(buf, "enable") == 0)
		ar->conf_flags |=
			ATH6KL_CONF_SKB_DUP;
	else
		return -EINVAL;

	return count;
}

static const struct file_operations fops_skb_dup_operation = {
	.read = ath6kl_skb_dup_read,
	.write = ath6kl_skb_dup_write,
	.open = ath6kl_debugfs_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

/* File operation functions for DTIM extend */
static ssize_t ath6kl_dtim_ext_write(struct file *file,
				const char __user *user_buf,
				size_t count, loff_t *ppos)
{
	struct ath6kl *ar = file->private_data;
	int ret;

	ret = kstrtou8_from_user(user_buf, count, 0,
			&ar->dtim_ext);

	if (ret)
		return ret;

	if (ath6kl_wmi_set_dtim_ext(ar->wmi, ar->dtim_ext))
		return -EIO;

	return count;
}

static ssize_t ath6kl_dtim_ext_read(struct file *file,
				char __user *user_buf,
				size_t count, loff_t *ppos)
{
	struct ath6kl *ar = file->private_data;
	u8 buf[32];
	unsigned int len = 0;

	len = scnprintf(buf, sizeof(buf), "DTIM Ext: %d\n",
			ar->dtim_ext);

	return simple_read_from_buffer(user_buf, count, ppos, buf, len);
}

/* debug fs for DTIM extend */
static const struct file_operations fops_dtim_ext = {
	.read = ath6kl_dtim_ext_read,
	.write = ath6kl_dtim_ext_write,
	.open = ath6kl_debugfs_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

/* File operation functions for Regulatory-Country */
static ssize_t ath6kl_reg_country_write(struct file *file,
				const char __user *user_buf,
				size_t count, loff_t *ppos)
{
	struct ath6kl *ar = file->private_data;
	char buf[8], alpha2[2];
	unsigned int len;

	len = min(count, sizeof(buf) - 1);
	if (copy_from_user(buf, user_buf, len))
		return -EFAULT;

	alpha2[0] = buf[0];
	alpha2[1] = buf[1];

	if (ath6kl_reg_set_country(ar, alpha2))
		return -EIO;

	return count;
}

/* debug fs for Regulatory-Country */
static const struct file_operations fops_reg_country_write = {
	.write = ath6kl_reg_country_write,
	.open = ath6kl_debugfs_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

/* File operation functions for P2P GO sync to AP */
static ssize_t ath6kl_p2p_go_sync_write(struct file *file,
				const char __user *user_buf,
				size_t count, loff_t *ppos)
{
	struct ath6kl *ar = file->private_data;
	char *buf, *p;
	int value, i, addr[ETH_ALEN];
	u16 ch_list[1];
	struct wmi_set_go_sync_cmd gsync;
	struct ath6kl_vif *vif;

	buf = kzalloc(count+1, GFP_ATOMIC);
	if (buf == NULL)
		return -EFAULT;
	if (copy_from_user(buf, user_buf, (unsigned int)count)) {
		kfree(buf);
		return -EFAULT;
	}

	p = buf;
	SKIP_SPACE;
	sscanf(p, "%d", &value);
	ch_list[0] = gsync.freq = (u16)value;
	SEEK_SPACE;
	SKIP_SPACE;
	if (sscanf(p, "%02x:%02x:%02x:%02x:%02x:%02x",
		   &addr[0], &addr[1], &addr[2], &addr[3], &addr[4], &addr[5])
		!= ETH_ALEN) {
		kfree(buf);
		return -EINVAL;
	}
	for (i = 0; i < ETH_ALEN; i++)
		gsync.addr[i] = (u8)addr[i];

	SEEK_SPACE;
	SKIP_SPACE;
	sscanf(p, "%d", &value);
	gsync.repeat = (u8)value;
	SEEK_SPACE;
	SKIP_SPACE;
	sscanf(p, "%d", &value);
	gsync.sta_dwell_time = (u8)value;

	vif = ath6kl_vif_first(ar);
	if (vif) {
		if (!vif->usr_bss_filter) {
			clear_bit(CLEAR_BSSFILTER_ON_BEACON, &vif->flags);
			ath6kl_wmi_bssfilter_cmd(
				ar->wmi,
				vif->fw_vif_idx,
				ALL_BSS_FILTER,
				0);
		}
		ath6kl_wmi_scanparams_cmd(ar->wmi, vif->fw_vif_idx,
				vif->sc_params.fg_start_period,
				vif->sc_params.fg_end_period,
				vif->sc_params.bg_period,
				120,
				120,
				vif->sc_params.pas_chdwell_time,
				vif->sc_params.short_scan_ratio,
				vif->sc_params.scan_ctrl_flags,
				vif->sc_params.max_dfsch_act_time,
				vif->sc_params.maxact_scan_per_ssid);
		ath6kl_wmi_go_sync_cmd(ar->wmi, vif->fw_vif_idx, &gsync);
		ath6kl_wmi_startscan_cmd(ar->wmi, vif->fw_vif_idx, WMI_LONG_SCAN,
				1, false, 0,
				120,
				1, ch_list);
	}
	kfree(buf);
	return count;
}

/* debug fs for P2P GO sync to AP Params */
static const struct file_operations fops_p2p_go_sync = {
	.write = ath6kl_p2p_go_sync_write,
	.open = ath6kl_debugfs_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

/* File operation for AP RecommendChannel */
static ssize_t ath6kl_ap_rc_write(struct file *file,
				const char __user *user_buf,
				size_t count, loff_t *ppos)
{
	struct ath6kl *ar = file->private_data;
	struct ath6kl_vif *vif;
	u32 i, tmp;
	int mode_or_freq = 0;
	char buf[8];
	ssize_t len;

	len = min(count, sizeof(buf) - 1);
	if (copy_from_user(buf, user_buf, len))
		return -EFAULT;

	buf[len] = '\0';
	if (kstrtou32(buf, 0, &tmp))
		return -EINVAL;

	mode_or_freq = (int)tmp;

	for (i = 0; i < ar->vif_max; i++) {
		vif = ath6kl_get_vif_by_index(ar, i);
		if (vif) {
			if (ath6kl_ap_rc_config(vif, mode_or_freq))
				return -EINVAL;
		}
	}

	/*
	 * Expect the user will start AP after configurate and start
	 * recommend channel update immediately.
	 */
	vif = ath6kl_vif_first(ar);
	if (vif)
		ath6kl_ap_rc_update(vif);

	return count;
}

/* debug fs for AP RecommendChannel. */
static const struct file_operations fops_ap_rc = {
	.write = ath6kl_ap_rc_write,
	.open = ath6kl_debugfs_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

/* File operation for BSS Post-Proc */
static ssize_t ath6kl_bss_proc_write(struct file *file,
				const char __user *user_buf,
				size_t count, loff_t *ppos)
{
	struct ath6kl *ar = file->private_data;
	struct ath6kl_vif *vif;
	char *p;
	int bss_cache, aging_time;
	char buf[32];
	ssize_t len;
	int i;

	len = min(count, sizeof(buf) - 1);
	if (copy_from_user(buf, user_buf, len))
		return -EFAULT;

	p = buf;

	SKIP_SPACE;

	sscanf(p, "%d", &bss_cache);

	SEEK_SPACE;
	SKIP_SPACE;

	sscanf(p, "%d", &aging_time);

	for (i = 0; i < ar->vif_max; i++) {
		vif = ath6kl_get_vif_by_index(ar, i);
		if (vif)
			ath6kl_bss_post_proc_bss_config(vif,
						(bss_cache ? true : false),
						(int)aging_time);
	}

	return count;
}

/* debug fs for BSS Post-Proc. */
static const struct file_operations fops_bss_proc = {
	.write = ath6kl_bss_proc_write,
	.open = ath6kl_debugfs_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

static ssize_t ath6kl_sync_tsf_write(struct file *file,
				const char __user *user_buf,
				size_t count, loff_t *ppos)
{
    struct ath6kl *ar = file->private_data;
	
    ath6kl_wmi_set_sync_tsf_cmd(ar->wmi, 1);
    return count;
}

static const struct file_operations fops_sync_tsf = {
	.write = ath6kl_sync_tsf_write,
	.open = ath6kl_debugfs_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

static ssize_t ath6kl_get_tsf(struct file *file,
				char __user *user_buf,
				size_t count, loff_t *ppos)
{
	struct ath6kl *ar = file->private_data;
	u8 buf[32];
	unsigned int len = 0;
	u32 datah = 0, datal = 0;
#define TSF_TO_TU(_h,_l)    ((((u64)(_h)) << 32) | ((u64)(_l)))
#define REG_TSF_L 0x5054
#define REG_TSF_H 0x5058

	ath6kl_diag_read32(ar, REG_TSF_L, &datal);
	ath6kl_diag_read32(ar, REG_TSF_H, &datah);

	len = scnprintf(buf, sizeof(buf), "%llu\n", TSF_TO_TU(datah, datal));

	return simple_read_from_buffer(user_buf, count, ppos, buf, len);
}

static const struct file_operations fops_get_tsf = {
	.read = ath6kl_get_tsf,
	.open = ath6kl_debugfs_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};


#ifdef DIRECT_AUDIO_SUPPORT
extern int Direct_Audio_TX_debug(void);
extern int Direct_Audio_RX_debug(void);

static ssize_t ath6kl_aow_write(struct file *file,
					const char __user *user_buf,
					size_t count, loff_t *ppos)
{
	u32 tmp,len;
	char buf[32];

	len = min(count, sizeof(buf) - 1);
	if (copy_from_user(buf, user_buf, len))
		return -EFAULT;
	buf[len] = '\0';
	if (kstrtou32(buf, 0, &tmp))
		return -EINVAL;	
	if (tmp == 0) {//simulate aow tx
		Direct_Audio_TX_debug();
	} else if (tmp == 1) {//simulate aow rx
		Direct_Audio_RX_debug();
	}
	return count;
}

extern void Direct_Audio_debug_dump(void);

static ssize_t ath6kl_aow_read(struct file *file,
					char __user *user_buf,
					size_t count, loff_t *ppos)
{
	char buf[32];
	int len;
	
	Direct_Audio_debug_dump();
	len = snprintf(buf, sizeof(buf), "aow debug dump\n");
	return simple_read_from_buffer(user_buf, count, ppos, buf, len);
}

static int ath6kl_aow_open(struct inode *inode, struct file *file)
{
	file->private_data = inode->i_private;
	return 0;
}

static const struct file_operations fops_aow = {
	.read = ath6kl_aow_read,
	.write = ath6kl_aow_write,
	.open = ath6kl_aow_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};
#endif

int ath6kl_debug_init(struct ath6kl *ar)
{
	skb_queue_head_init(&ar->debug.fwlog_queue);
	init_completion(&ar->debug.fwlog_completion);

	/*
	 * Actually we are lying here but don't know how to read the mask
	 * value from the firmware.
	 */
	ar->debug.fwlog_mask = 0;

	ar->debugfs_phy = debugfs_create_dir("ath6kl",
					     ar->wiphy->debugfsdir);
	if (!ar->debugfs_phy)
		return -ENOMEM;

	debugfs_create_file("tgt_stats", S_IRUSR, ar->debugfs_phy, ar,
			    &fops_tgt_stats);

	debugfs_create_file("credit_dist_stats", S_IRUSR, ar->debugfs_phy, ar,
			    &fops_credit_dist_stats);

	debugfs_create_file("endpoint_stats", S_IRUSR | S_IWUSR,
			    ar->debugfs_phy, ar, &fops_endpoint_stats);

	debugfs_create_file("fwlog", S_IRUSR, ar->debugfs_phy, ar,
			    &fops_fwlog);

	debugfs_create_file("fwlog_block", S_IRUSR, ar->debugfs_phy, ar,
			    &fops_fwlog_block);

	debugfs_create_file("fwlog_mask", S_IRUSR | S_IWUSR, ar->debugfs_phy,
			    ar, &fops_fwlog_mask);

	debugfs_create_file("reg_addr", S_IRUSR | S_IWUSR, ar->debugfs_phy, ar,
			    &fops_diag_reg_read);

	debugfs_create_file("reg_dump", S_IRUSR, ar->debugfs_phy, ar,
			    &fops_reg_dump);

	debugfs_create_file("lrssi_roam_param", S_IRUSR | S_IWUSR,
			    ar->debugfs_phy, ar, &fops_lrssi_roam_param);

	debugfs_create_file("driver_version", S_IRUSR | S_IWUSR,
			    ar->debugfs_phy, ar, &fops_driver_version);

	debugfs_create_file("rx_drop_operation", S_IRUSR | S_IWUSR,
			    ar->debugfs_phy, ar, &fops_rx_drop_operation);

	debugfs_create_file("reg_write", S_IRUSR | S_IWUSR,
			    ar->debugfs_phy, ar, &fops_diag_reg_write);

	debugfs_create_file("war_stats", S_IRUSR, ar->debugfs_phy, ar,
			    &fops_war_stats);

	debugfs_create_file("roam_table", S_IRUSR, ar->debugfs_phy, ar,
			    &fops_roam_table);

	debugfs_create_file("force_roam", S_IWUSR, ar->debugfs_phy, ar,
			    &fops_force_roam);

	debugfs_create_file("roam_mode", S_IWUSR, ar->debugfs_phy, ar,
			    &fops_roam_mode);

	debugfs_create_file("roam_5g_bias", S_IWUSR, ar->debugfs_phy, ar,
			    &fops_roam_5g_bias);

	debugfs_create_file("keepalive", S_IRUSR | S_IWUSR, ar->debugfs_phy, ar,
			    &fops_keepalive);

	debugfs_create_file("disconnect_timeout", S_IRUSR | S_IWUSR,
			    ar->debugfs_phy, ar, &fops_disconnect_timeout);

	debugfs_create_file("create_qos", S_IWUSR, ar->debugfs_phy, ar,
				&fops_create_qos);

	debugfs_create_file("delete_qos", S_IWUSR, ar->debugfs_phy, ar,
				&fops_delete_qos);

	debugfs_create_file("bgscan_interval", S_IWUSR,
				ar->debugfs_phy, ar, &fops_bgscan_int);

	debugfs_create_file("power_params", S_IWUSR, ar->debugfs_phy, ar,
				&fops_power_params);

	debugfs_create_file("lpl_force_enable", S_IRUSR | S_IWUSR,
				ar->debugfs_phy, ar, &fops_lpl_force_enable);

	debugfs_create_file("mimo_ps_enable", S_IRUSR | S_IWUSR,
				ar->debugfs_phy, ar, &fops_mimo_ps_enable);

	debugfs_create_file("mimo_ps_params", S_IRUSR | S_IWUSR,
				ar->debugfs_phy, ar, &fops_mimo_ps_params);

	debugfs_create_file("green_tx_enable", S_IRUSR | S_IWUSR,
				ar->debugfs_phy, ar, &fops_green_tx);

	debugfs_create_file("green_tx_params", S_IRUSR | S_IWUSR,
				ar->debugfs_phy, ar, &fops_green_tx_params);

	debugfs_create_file("ht_cap_params", S_IRUSR | S_IWUSR,
			    ar->debugfs_phy, ar, &fops_ht_cap_params);

	debugfs_create_file("ht_coex_params", S_IRUSR | S_IWUSR,
			    ar->debugfs_phy, ar, &fops_ht_coex_params);

	debugfs_create_file("debug_mask", S_IRUSR | S_IWUSR,
			    ar->debugfs_phy, ar, &fops_debug_mask);

	debugfs_create_file("debug_mask_ext", S_IRUSR | S_IWUSR,
			    ar->debugfs_phy, ar, &fops_debug_mask_ext);

	debugfs_create_file("tx_amsdu", S_IRUSR | S_IWUSR,
			    ar->debugfs_phy, ar, &fops_tx_amsdu);

	debugfs_create_file("tx_amsdu_params", S_IRUSR | S_IWUSR,
			    ar->debugfs_phy, ar, &fops_tx_amsdu_params);

	debugfs_create_file("htc_stat", S_IRUSR,
			    ar->debugfs_phy, ar, &fops_htc_stat);

	debugfs_create_file("hif_stat", S_IRUSR,
			    ar->debugfs_phy, ar, &fops_hif_stat);

	debugfs_create_file("hif_max_sche", S_IWUSR,
			    ar->debugfs_phy, ar, &fops_hif_pipe_max_sche);

	debugfs_create_file("ap_apsd", S_IWUSR,
			    ar->debugfs_phy, ar, &fops_ap_apsd);

	debugfs_create_file("ap_intrabss", S_IWUSR,
			    ar->debugfs_phy, ar, &fops_ap_intrabss);

	debugfs_create_file("force_passcan", S_IRUSR | S_IWUSR,
			    ar->debugfs_phy, ar, &fops_force_passcan);

	debugfs_create_file("pmkid_list", S_IRUSR,
			    ar->debugfs_phy, ar, &fops_pmkid_list);

	debugfs_create_file("txrate_sereies", S_IWUSR,
			    ar->debugfs_phy, ar, &fops_txseries_write);

	debugfs_create_file("p2p_flowctrl_stat", S_IRUSR,
			    ar->debugfs_phy, ar, &fops_p2p_flowctrl_stat);

	debugfs_create_file("ap_ps_stat", S_IRUSR,
			    ar->debugfs_phy, ar, &fops_ap_ps_stat);

	debugfs_create_file("rx_aggr_params", S_IRUSR | S_IWUSR,
			    ar->debugfs_phy, ar, &fops_rx_aggr_params);

	debugfs_create_file("rx_bundle_params", S_IRUSR | S_IWUSR,
			    ar->debugfs_phy, ar, &fops_rx_bundle_params);

	debugfs_create_file("scan_params", S_IRUSR | S_IWUSR,
			    ar->debugfs_phy, ar, &fops_scan_params);

	debugfs_create_file("ap_keepalive_params", S_IRUSR,
			    ar->debugfs_phy, ar, &fops_ap_keepalive_params);

	debugfs_create_file("tgt_ap_stats", S_IRUSR | S_IWUSR,
			    ar->debugfs_phy, ar, &fops_tgt_ap_stats);
        
	debugfs_create_file("txrx_error_stats", S_IRUSR | S_IWUSR,
			    ar->debugfs_phy, ar, &fops_txrx_error_stats);

	debugfs_create_file("chan_list", S_IRUSR,
			    ar->debugfs_phy, ar, &fops_chan_list);

	debugfs_create_file("ap_acl_policy", S_IRUSR | S_IWUSR,
			    ar->debugfs_phy, ar, &fops_ap_acl_policy);

	debugfs_create_file("ap_acl_mac_list", S_IRUSR | S_IWUSR,
			    ar->debugfs_phy, ar, &fops_ap_acl_mac_list);

	debugfs_create_file("ampdu", S_IWUSR,
			    ar->debugfs_phy, ar, &fops_ampdu);

	debugfs_create_file("arp_ip_addrs", S_IWUSR,
			    ar->debugfs_phy, ar, &fops_arp_offload_ip_addrs);

	debugfs_create_file("mcc_profile", S_IWUSR,
			    ar->debugfs_phy, ar, &fops_mcc_profile);

	debugfs_create_file("seamless_mcc_scc_switch", S_IWUSR,
			    ar->debugfs_phy, ar, &fops_seamless_mcc_scc_switch);

	debugfs_create_file("p2p_frame_retry", S_IWUSR,
			    ar->debugfs_phy, ar, &fops_p2p_frame_retry);

	debugfs_create_file("p2p_frame_cond_reject", S_IWUSR,
			    ar->debugfs_phy, ar, &p2p_frame_cond_reject);

	debugfs_create_file("debug_quirks", S_IRUSR | S_IWUSR,
			    ar->debugfs_phy, ar, &fops_debug_quirks);

	debugfs_create_file("disable_scan", S_IRUSR | S_IWUSR,
			    ar->debugfs_phy, ar, &fops_disable_scan);

	debugfs_create_file("disable_runtime_flowctrl", S_IWUSR,
			ar->debugfs_phy, ar, &fops_disable_runtime_flowctrl);
#ifdef USB_AUTO_SUSPEND
	debugfs_create_file("usb_autopm_usagecnt", S_IRUSR | S_IWUSR,
			    ar->debugfs_phy, ar, &fops_usb_autopm_usagecnt);

	debugfs_create_file("usb_pm_status", S_IRUSR | S_IWUSR,
			    ar->debugfs_phy, ar, &fops_usb_pm_status);
#endif

	debugfs_create_file("p2p_ie_not_append", S_IWUSR,
			    ar->debugfs_phy, ar, &fops_p2p_ie_not_append);

	debugfs_create_file("ap_admc", S_IRUSR,
			    ar->debugfs_phy, ar, &fops_ap_admc);

	debugfs_create_file("antdivcfg", S_IWUSR,
				ar->debugfs_phy, ar, &fops_antdiv_write);

	debugfs_create_file("antdivstat", S_IRUSR,
				ar->debugfs_phy, ar, &fops_antdiv_state_read);

	debugfs_create_file("anistat", S_IRUSR,
				ar->debugfs_phy, ar, &fops_ani_state_read);

	debugfs_create_file("pattern_gen", S_IWUSR | S_IRUSR,
				ar->debugfs_phy, ar, &fops_pattern_gen);

	debugfs_create_file("p2p_rc", S_IRUSR | S_IWUSR,
				ar->debugfs_phy, ar, &fops_p2p_rc);

	debugfs_create_file("hif_rxq_threshold", S_IWUSR,
			ar->debugfs_phy, ar, &fops_hif_pipe_rxq_threshold);

	debugfs_create_file("skb_dup_enable", S_IRUSR | S_IWUSR,
				ar->debugfs_phy, ar, &fops_skb_dup_operation);

	debugfs_create_file("dtim_ext", S_IRUSR | S_IWUSR,
			    ar->debugfs_phy, ar, &fops_dtim_ext);

	debugfs_create_file("wow_pattern", S_IRUSR | S_IWUSR,
			    ar->debugfs_phy, ar, &fops_wowpattern_gen);

	debugfs_create_file("reg_country", S_IWUSR,
				ar->debugfs_phy, ar, &fops_reg_country_write);

	debugfs_create_file("p2p_go_sync", S_IWUSR,
				ar->debugfs_phy, ar, &fops_p2p_go_sync);

	debugfs_create_file("ap_rc", S_IWUSR,
				ar->debugfs_phy, ar, &fops_ap_rc);

	debugfs_create_file("bss_proc", S_IWUSR,
				ar->debugfs_phy, ar, &fops_bss_proc);
#ifdef DIRECT_AUDIO_SUPPORT
	debugfs_create_file("aow", S_IRUSR | S_IWUSR,
			    ar->debugfs_phy, ar, &fops_aow);
#endif
				
	debugfs_create_file("sync_tsf", S_IWUSR | S_IWGRP | S_IWOTH,
			    ar->debugfs_phy, ar, &fops_sync_tsf);

	debugfs_create_file("get_tsf", S_IRUSR | S_IRGRP | S_IROTH,
			    ar->debugfs_phy, ar, &fops_get_tsf);

	return 0;
}


void ath6kl_debug_cleanup(struct ath6kl *ar)
{
	skb_queue_purge(&ar->debug.fwlog_queue);
	complete(&ar->debug.fwlog_completion);
	kfree(ar->debug.roam_tbl);
}

#endif
