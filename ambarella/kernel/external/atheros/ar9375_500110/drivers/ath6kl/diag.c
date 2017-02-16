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

#include <linux/ip.h>
#include "core.h"
#include "wmi.h"
#include "diagnose.h"


#ifdef ATH6KL_DIAGNOSTIC

struct wifi_diag_callbacks diag_evt_callback;
struct sk_buff_head diag_events;
static DEFINE_MUTEX(diag_event_mutex);


enum WIFI_DIAG_PM_STATE {
	WIFI_DIAG_PM_STATE_SLEEP = 1,
	WIFI_DIAG_PM_STATE_AWAKE = 2,
	WIFI_DIAG_PM_STATE_FAKE_SLEEP = 3,
};


/* diag wmi command */
int
ath6kl_wmi_cmd_send_diag(struct wmi *wmi, struct sk_buff *skb,
				    enum wmid_command_id cmd_id,
				    enum wmi_sync_flag sync_flag)
{
	struct wmid_cmd_hdr *cmd_hdr;
	int ret;

	skb_push(skb, sizeof(struct wmid_cmd_hdr));

	cmd_hdr = (struct wmid_cmd_hdr *) skb->data;
	cmd_hdr->cmd_id = cpu_to_le32(cmd_id);

	ret = ath6kl_wmi_cmd_send(wmi, 0, skb, WMI_DIAGNOSTIC_CMDID, sync_flag);

	return ret;
}

int
ath6kl_wmi_macfilter_cmd(struct wmi *wmi, int type, u32 low, u32 high)
{
	struct sk_buff *skb;
	int ret;
	struct wmid_macfilter_cmd *macfilter;

	skb = ath6kl_wmi_get_new_buf(
		sizeof(struct wmid_cmd_hdr)+sizeof(*macfilter));
	if (!skb)
		return -ENOMEM;
	macfilter = (struct wmid_macfilter_cmd *)skb->data;
	macfilter->type = type;
	macfilter->low = low;
	macfilter->high = high;

	ret = ath6kl_wmi_cmd_send_diag(wmi, skb, WMID_MACFILTER_CMDID,
					NO_SYNC_WMIFLAG);
	return ret;
}

int
ath6kl_wmi_fsm_cmd(struct wmi *wmi, bool enable)
{
	struct sk_buff *skb;
	int ret;
	struct wmid_event_set_cmd *fsm;

	skb = ath6kl_wmi_get_new_buf(sizeof(struct wmid_cmd_hdr)+sizeof(*fsm));
	if (!skb)
		return -ENOMEM;
	fsm = (struct wmid_event_set_cmd *)skb->data;
	fsm->enable = enable;

	ret = ath6kl_wmi_cmd_send_diag(wmi, skb, WMID_FSM_EVENT_CMDID,
					NO_SYNC_WMIFLAG);
	return ret;
}

int
ath6kl_wmi_pwrsave_cmd(struct wmi *wmi, bool enable)
{
	struct sk_buff *skb;
	int ret;
	struct wmid_event_set_cmd *ps;

	skb = ath6kl_wmi_get_new_buf(sizeof(struct wmid_cmd_hdr)+sizeof(*ps));
	if (!skb)
		return -ENOMEM;
	ps = (struct wmid_event_set_cmd *)skb->data;
	ps->enable = enable;

	ret = ath6kl_wmi_cmd_send_diag(wmi, skb, WMID_PWR_SAVE_EVENT_CMDID,
					NO_SYNC_WMIFLAG);
	return ret;
}

int
ath6kl_wmi_interference_cmd(struct wmi *wmi)
{
	struct sk_buff *skb;
	int ret;

	skb = ath6kl_wmi_get_new_buf(sizeof(struct wmid_cmd_hdr));
	if (!skb)
		return -ENOMEM;

	ret = ath6kl_wmi_cmd_send_diag(wmi, skb, WMID_INTERFERENCE_CMDID,
					NO_SYNC_WMIFLAG);
	return ret;
}

int
ath6kl_wmi_rxtime_cmd(struct wmi *wmi)
{
	struct sk_buff *skb;
	int ret;

	skb = ath6kl_wmi_get_new_buf(sizeof(struct wmid_cmd_hdr));
	if (!skb)
		return -ENOMEM;

	ret = ath6kl_wmi_cmd_send_diag(wmi, skb, WMID_RXTIME_CMDID,
					NO_SYNC_WMIFLAG);
	return ret;
}

int
ath6kl_wmi_pktlog_enable_cmd(struct wmi *wmi,
		struct wmi_enable_pktlog_cmd *options)
{
	struct sk_buff *skb;
	struct wmi_enable_pktlog_cmd *cmd;
	int status = 0;

	skb = ath6kl_wmi_get_new_buf(sizeof(struct wmi_enable_pktlog_cmd));
	if (!skb)
		return -ENOMEM;

	cmd = (struct wmi_enable_pktlog_cmd *) skb->data;
	cmd->evlist = options->evlist;
	cmd->option = options->option;
	cmd->trigger_thresh = cpu_to_le32(options->trigger_thresh);
	cmd->trigger_interval = cpu_to_le32(options->trigger_interval);
	cmd->trigger_tail_count = cpu_to_le32(options->trigger_tail_count);
	cmd->buffer_size = cpu_to_le32(options->buffer_size);

	status = ath6kl_wmi_cmd_send(wmi, 0, skb, WMI_PKTLOG_ENABLE_CMDID,
					NO_SYNC_WMIFLAG);

	return status;
}

int
ath6kl_wmi_pktlog_disable_cmd(struct wmi *wmi)
{
	return ath6kl_wmi_simple_cmd(wmi, 0, WMI_PKTLOG_DISABLE_CMDID);
}


/* diag wmi event */
#if 0
static void
dump_hex(u8 *frm, u32 len)
{
	u32    i;
	printk(KERN_INFO "\n-----------------------------\n");
	for (i = 0; i < len; i++) {
		printk(KERN_INFO "0x%02x ", frm[i]);
		if ((i+1) % 16 == 0)
			printk(KERN_INFO "\n");
	}
	printk(KERN_INFO "\n");
	printk(KERN_INFO "\n=============================\n\n");
}
#endif

int
ath6kl_wmi_pktlog_event_rx(struct wmi *wmi, u8 *datap, u32 len)
{
	u8 *pdata = NULL;
	struct ath_pktlog_hdr *log_hdr = (struct ath_pktlog_hdr *)datap ;
	u32 offset = 0;
	struct ath6kl_vif *vif = ath6kl_get_vif_by_index(wmi->parent_dev, 0);

	if (vif == NULL)
		return -EINVAL;

	while (len - offset > 0) {
		switch (le16_to_cpu(log_hdr->log_type)) {
		case PKTLOG_TYPE_TXSTATUS:
			pdata = (u8 *)log_hdr + sizeof(struct ath_pktlog_hdr);
			wifi_diag_mac_tx_frame_event(vif,
					(struct ath_pktlog_txstatus *)pdata);
			break;
		case PKTLOG_TYPE_TXCTL:
			pdata = (u8 *)log_hdr + sizeof(struct ath_pktlog_hdr);
			wifi_diag_mac_txctrl_event(vif,
					(struct ath_pktlog_txctl *)pdata);
			break;
		case PKTLOG_TYPE_RX:
			pdata = (u8 *)log_hdr + sizeof(struct ath_pktlog_hdr);
			wifi_diag_mac_rx_frame_event(vif,
					(struct ath_pktlog_rx *)pdata);
			break;
		default:
			break;
		}
		offset += le16_to_cpu(log_hdr->size) +
			sizeof(struct ath_pktlog_hdr);
		log_hdr = (struct ath_pktlog_hdr *)
			((u8 *)log_hdr + le16_to_cpu(log_hdr->size) +
			sizeof(struct ath_pktlog_hdr));
	}

	return 0;
}

int
ath6kl_wmi_start_scan_event(struct ath6kl_vif *vif, u32 seq_num)
{
	enum wifi_diag_mac_fsm_t fsm_event = WIFI_DIAG_MAC_FSM_SCANNING;
	printk(KERN_INFO "ath6kl_wmi_start_scan_event\n");
	wifi_diag_mac_fsm_event(vif, fsm_event, seq_num);
	return 0;
}

int
ath6kl_wmi_fsm_auth_event(struct ath6kl_vif *vif, u32 seq_num)
{
	enum wifi_diag_mac_fsm_t fsm_event = WIFI_DIAG_MAC_FSM_AUTH;
	printk(KERN_INFO "ath6kl_wmi_fsm_auth_event\n");
	wifi_diag_mac_fsm_event(vif, fsm_event, seq_num);
	return 0;
}

int
ath6kl_wmi_fsm_assoc_event(struct ath6kl_vif *vif, u32 seq_num)
{
	enum wifi_diag_mac_fsm_t fsm_event = WIFI_DIAG_MAC_FSM_ASSOC;
	printk(KERN_INFO "ath6kl_wmi_fsm_assoc_event\n");
	wifi_diag_mac_fsm_event(vif, fsm_event, seq_num);
	return 0;
}

int
ath6kl_wmi_fsm_deauth_event(struct ath6kl_vif *vif, u32 seq_num)
{
	enum wifi_diag_mac_fsm_t fsm_event = WIFI_DIAG_MAC_FSM_DEAUTH;
	printk(KERN_INFO "ath6kl_wmi_fsm_deauth_event\n");
	wifi_diag_mac_fsm_event(vif, fsm_event, seq_num);
	return 0;
}

int
ath6kl_wmi_fsm_disassoc_event(struct ath6kl_vif *vif, u32 seq_num)
{
	enum wifi_diag_mac_fsm_t fsm_event = WIFI_DIAG_MAC_FSM_DISASSOC;
	printk(KERN_INFO "ath6kl_wmi_fsm_disassoc_event\n");
	wifi_diag_mac_fsm_event(vif, fsm_event, seq_num);
	return 0;
}

int
ath6kl_wmi_pwrsave_event(struct ath6kl_vif *vif,
		struct wmi *wmi, u8 *datap, int len, u32 seq_num)
{
	struct wmid_pwr_save_event *state =
				(struct wmid_pwr_save_event *)datap;
	enum wifi_diag_pwrsave_t  oldpwrsave, pmpwrsave;

	printk(KERN_INFO "ath6kl_wmi_pwrsave_event\n");

	switch (state->oldState) {
	case WIFI_DIAG_PM_STATE_SLEEP:
		oldpwrsave = WIFI_DIAG_DEEPSLEEP_STOP;
		break;
	case WIFI_DIAG_PM_STATE_AWAKE:
		oldpwrsave = WIFI_DIAG_MAXPERF_STOP;
		break;
	case WIFI_DIAG_PM_STATE_FAKE_SLEEP:
		oldpwrsave = WIFI_DIAG_FAKESLEEP_STOP;
		break;
	default:
		return 0;
		break;
	}
	wifi_diag_send_pwrsave_event(vif, oldpwrsave, seq_num);

	switch (state->pmState) {
	case WIFI_DIAG_PM_STATE_SLEEP:
		pmpwrsave = WIFI_DIAG_DEEPSLEEP_START;
		break;
	case WIFI_DIAG_PM_STATE_AWAKE:
		pmpwrsave = WIFI_DIAG_MAXPERF_START;
		break;
	case WIFI_DIAG_PM_STATE_FAKE_SLEEP:
		pmpwrsave = WIFI_DIAG_FAKESLEEP_START;
		break;
	default:
		return 0;
		break;
	}
	wifi_diag_send_pwrsave_event(vif, pmpwrsave, seq_num);

	return 0;
}

int
ath6kl_wmi_diag_event(struct ath6kl_vif *vif,
		struct wmi *wmi, struct sk_buff *skb)
{
	struct wmid_cmd_hdr *cmd;
	u32 len;
	u16 id;
	u32 seq_num;
	u8 *datap;
	int ret = 0;

	if (skb->len < sizeof(struct wmid_cmd_hdr)) {
		printk(KERN_INFO "bad packet 1\n");
		return -EINVAL;
	}

	cmd = (struct wmid_cmd_hdr *) skb->data;
	id = le32_to_cpu(cmd->cmd_id);
	seq_num = le32_to_cpu(cmd->seq_num);

	skb_pull(skb, sizeof(struct wmid_cmd_hdr));

	datap = skb->data;
	len = skb->len;

	switch (id) {
	case WMID_START_SCAN_EVENTID:
		ret = ath6kl_wmi_start_scan_event(vif, seq_num);
		break;
	case WMID_FSM_AUTH_EVENTID:
		ret = ath6kl_wmi_fsm_auth_event(vif, seq_num);
		break;
	case WMID_FSM_ASSOC_EVENTID:
		ret = ath6kl_wmi_fsm_assoc_event(vif, seq_num);
		break;
	case WMID_FSM_DEAUTH_EVENTID:
		ret = ath6kl_wmi_fsm_deauth_event(vif, seq_num);
		break;
	case WMID_FSM_DISASSOC_EVENTID:
		ret = ath6kl_wmi_fsm_disassoc_event(vif, seq_num);
		break;
	case WMID_STAT_RX_RATE_EVENTID:
		ret = ath6kl_wmi_stat_rx_rate_event(vif, wmi,
						datap, len, seq_num);
		break;
	case WMID_STAT_TX_RATE_EVENTID:
		ret = ath6kl_wmi_stat_tx_rate_event(vif, wmi,
						datap, len, seq_num);
		break;
	case WMID_INTERFERENCE_EVENTID:
		ath6kl_wmi_interference_event(vif, wmi, datap, len, seq_num);
		break;
	case WMID_RXTIME_EVENTID:
		ath6kl_wmi_rxtime_event(vif, wmi, datap, len, seq_num);
		break;
	case WMID_PWR_SAVE_EVENTID:
		ath6kl_wmi_pwrsave_event(vif, wmi, datap, len, seq_num);
		break;
	case WMID_FSM_CONNECT_EVENTID:
		vif->diag.connect_seq_num = seq_num;
		break;
	case WMID_FSM_DISCONNECT_EVENTID:
		vif->diag.disconnect_seq_num = seq_num;
		break;
	default:
		printk(KERN_INFO "unknown cmd id 0x%x\n", id);
		ret = -EINVAL;
		break;
	}

	return ret;
}

/* diag driver handler table */
static struct wifi_drv_hdl_list wifi_drv_hdl_table = {
	.wifi_register = wifi_diag_drv_register,
	.wifi_unregister = wifi_diag_drv_unregister,
	.wifi_diag_reg_event_callback = wifi_diag_register_event_callback,
	.wifi_diag_cmd = wifi_diag_cmd_send,
};


void *
wifi_diag_drv_register(void *diag_hdl)
{
	return (void *)&wifi_drv_hdl_table;
}
EXPORT_SYMBOL(wifi_diag_drv_register);

enum wifi_diag_status_t
wifi_diag_drv_unregister(void *drv_hdl)
{
	struct ath6kl_vif *vif = ath6kl_get_vif_by_index(
					globalwmi->parent_dev, 0);

	enum wifi_diag_status_t diag_status = WIFI_DIAG_EOK;
	ath6kl_wmi_macfilter_cmd(globalwmi,
				WMI_PKTLOG_EVENT_TX | WMI_PKTLOG_EVENT_RX,
				WIFI_DIAG_MACFILTER_DISABLEALL,
				WIFI_DIAG_MACFILTER_DISABLEALL);
	ath6kl_wmi_pktlog_disable_cmd(globalwmi);
	ath6kl_wmi_fsm_cmd(globalwmi, false);
	ath6kl_wmi_pwrsave_cmd(globalwmi, false);

	if (vif != NULL)
		wifi_diag_timer_destroy(vif);

	return diag_status;
}

enum wifi_diag_status_t
wifi_diag_register_event_callback(void *drv_hdl,
		struct wifi_diag_callbacks *evt_callback)
{
	enum wifi_diag_status_t diag_status = WIFI_DIAG_EOK;
	struct ath6kl_vif *vif = ath6kl_get_vif_by_index(
					globalwmi->parent_dev, 0);

	if (vif == NULL)
		return WIFI_DIAG_ENXIO;

	if (!vif->diag.diag_event_init) {
		skb_queue_head_init(&diag_events);
		vif->diag.diag_event_init = true;
	}
	diag_evt_callback.diag_event_callback =
		evt_callback->diag_event_callback;

	return diag_status;
}

enum wifi_diag_status_t
wifi_diag_cmd_send(void *drv_hdl, struct wifi_diag_cmd_t *cmd)
{
	enum wifi_diag_status_t diag_status = WIFI_DIAG_EOK;
	struct ath6kl_vif *vif = ath6kl_get_vif_by_index(
					globalwmi->parent_dev, 0);

	switch (cmd->cmd_id) {
	case  WIFI_DIAG_MAC_TX_FILTER_CMDID:
	{
	if (cmd->len == sizeof(struct wifi_diag_mac_tx_filter_cmd_t)) {
		struct wifi_diag_mac_tx_filter_cmd_t *ptxfilter =
			(struct wifi_diag_mac_tx_filter_cmd_t *)
			cmd->cmd_data;
		ath6kl_wmi_macfilter_cmd(globalwmi,
			WMI_PKTLOG_EVENT_TX,
			ptxfilter->filter_mask_low &
				WIFI_DIAG_MACFILTER_LOW_MASK,
			ptxfilter->filter_mask_high &
				WIFI_DIAG_MACFILTER_HIGH_MASK);
	}
	break;
	}
	case  WIFI_DIAG_MAC_RX_FILTER_CMDID:
	{
	if (cmd->len == sizeof(struct wifi_diag_mac_rx_filter_cmd_t)) {
		struct wifi_diag_mac_rx_filter_cmd_t *prxfilter =
			(struct wifi_diag_mac_rx_filter_cmd_t *)
			cmd->cmd_data;
		ath6kl_wmi_macfilter_cmd(globalwmi, WMI_PKTLOG_EVENT_RX,
			prxfilter->filter_mask_low &
				WIFI_DIAG_MACFILTER_LOW_MASK,
			prxfilter->filter_mask_high &
				WIFI_DIAG_MACFILTER_HIGH_MASK);
	}
	break;
	}
	case  WIFI_DIAG_CFG_CMDID:
	{
	if (cmd->len == sizeof(struct wifi_diag_cfg_cmd_t)) {
		struct wifi_diag_cfg_cmd_t *pcfg =
			(struct wifi_diag_cfg_cmd_t *)cmd->cmd_data;

		if (vif == NULL)
			return WIFI_DIAG_ENXIO;

		if (pcfg->cfg != vif->diag.cfg_mask) {
			if (pcfg->cfg &
				(WIFI_DIAG_MAC_TX_FRAME_EVENTENABLE |
				 WIFI_DIAG_MAC_RX_FRAME_EVENTENABLE)) {
				ath6kl_wmi_pktlog_disable_cmd(globalwmi);
				{
				struct wmi_enable_pktlog_cmd cmd;
				cmd.option = WMI_PKTLOG_OPTION_LOG_DIAGNOSTIC;
				cmd.evlist = 0;
				cmd.trigger_interval = 0;
				cmd.trigger_tail_count = 0;
				cmd.trigger_thresh = 0;
				cmd.buffer_size = 1500;

				if (pcfg->cfg &
					WIFI_DIAG_MAC_TX_FRAME_EVENTENABLE)
					cmd.evlist |= WMI_PKTLOG_EVENT_TX;

				if (pcfg->cfg &
					WIFI_DIAG_MAC_RX_FRAME_EVENTENABLE)
					cmd.evlist |=
						WMI_PKTLOG_EVENT_RX;

				ath6kl_wmi_pktlog_enable_cmd(
						globalwmi, &cmd);
				}
			} else {
				ath6kl_wmi_pktlog_disable_cmd(
							globalwmi);
			}

			if (pcfg->cfg & WIFI_DIAG_MAC_FSM_EVENTENABLE)
				ath6kl_wmi_fsm_cmd(globalwmi, true);
			else
				ath6kl_wmi_fsm_cmd(globalwmi, false);

			if (pcfg->cfg &
				WIFI_DIAG_INTERFERENCE_EVENTENABLE) {
				del_timer(&vif->diag.interference_timer);
				init_timer(&vif->diag.interference_timer);
				setup_timer(&vif->diag.interference_timer,
				wifi_diag_interference_timer_handler,
					(unsigned long) vif);
				mod_timer(
					&vif->diag.interference_timer,
					jiffies +
					msecs_to_jiffies(1000));
			} else {
				del_timer(&vif->diag.
					interference_timer);
			}

			if (pcfg->cfg & WIFI_DIAG_RX_TIME_EVENTENABLE) {
				del_timer(&vif->diag.rxtime_timer);
				init_timer(&vif->diag.rxtime_timer);
				setup_timer(&vif->diag.rxtime_timer,
					wifi_diag_rxtime_timer_handler,
					(unsigned long) vif);
				mod_timer(&vif->diag.rxtime_timer,
					jiffies +
					msecs_to_jiffies(1000));
			} else {
				del_timer(&vif->diag.rxtime_timer);
			}

			if (pcfg->cfg & WIFI_DIAG_PWR_SAVE_EVENTENABLE)
				ath6kl_wmi_pwrsave_cmd(globalwmi, true);
			else
				ath6kl_wmi_pwrsave_cmd(globalwmi,
							false);

			if (pcfg->cfg & WIFI_DIAG_TX_STAT_EVENTENABLE) {
				vif->diag.tx_timer_val = pcfg->value;

				del_timer(&vif->diag.tx_stat_timer);
				init_timer(&vif->diag.tx_stat_timer);
				setup_timer(&vif->diag.tx_stat_timer,
					wifi_diag_tx_stat_timer_handler,
					(unsigned long) vif);
				mod_timer(&vif->diag.tx_stat_timer,
					jiffies +
					msecs_to_jiffies(
					vif->diag.tx_timer_val));
			} else {
				del_timer(&vif->diag.tx_stat_timer);
			}

			if (pcfg->cfg & WIFI_DIAG_RX_STAT_EVENTENABLE) {
				vif->diag.rx_timer_val = pcfg->value;

				del_timer(&vif->diag.rx_stat_timer);
				init_timer(&vif->diag.rx_stat_timer);
				setup_timer(&vif->diag.rx_stat_timer,
					wifi_diag_rx_stat_timer_handler,
					(unsigned long) vif);
				mod_timer(&vif->diag.rx_stat_timer,
					jiffies +
					msecs_to_jiffies(
					vif->diag.rx_timer_val));
			} else {
				del_timer(&vif->diag.rx_stat_timer);
			}

			vif->diag.cfg_mask = pcfg->cfg;
		}
	}

	break;
	}
	default:
		break;
	}

	return diag_status;
}


/* wifi diag event */
static const s32 _rate_tbl_11[][2] = {
	/* {W/O SGI, with SGI} */
	{1000, 1000},
	{2000, 2000},
	{5500, 5500},
	{11000, 11000},
	{6000, 6000},
	{9000, 9000},
	{12000, 12000},
	{18000, 18000},
	{24000, 24000},
	{36000, 36000},
	{48000, 48000},
	{54000, 54000},
	{6500, 7200},		/* HT 20, MCS 0 */
	{13000, 14400},
	{19500, 21700},
	{26000, 28900},
	{39000, 43300},
	{52000, 57800},
	{58500, 65000},
	{65000, 72200},
	{13000, 14400},		/* HT 20, MCS 8 */
	{26000, 28900},
	{39000, 43300},
	{52000, 57800},
	{78000, 86700},
	{104000, 115600},
	{117000, 130000},
	{130000, 144400},	/* HT 20, MCS 15 */
	{13500, 15000},		/* HT 40, MCS 0 */
	{27000, 30000},
	{40500, 45000},
	{54000, 60000},
	{81000, 90000},
	{108000, 120000},
	{121500, 135000},
	{135000, 150000},
	{27000, 30000},		/*HT 40, MCS 8 */
	{54000, 60000},
	{81000, 90000},
	{108000, 120000},
	{162000, 180000},
	{216000, 240000},
	{243000, 270000},
	{270000, 300000},	/*HT 40, MCS 15 */
	{0, 0}
};


static void wifi_diag_event_process(struct work_struct *work)
{
	struct sk_buff *skb;
	struct wifi_diag_event_t *pwifi_diag_event;

	mutex_lock(&diag_event_mutex);

	while ((skb = skb_dequeue(&diag_events))) {
		pwifi_diag_event = (struct wifi_diag_event_t *) skb->data;

		if (!diag_local_test) {
			diag_evt_callback.diag_event_callback(
				(void *)&wifi_drv_hdl_table, skb);
		} else {
			printk(KERN_INFO "eventid=%d seq_num=%d\n",
				pwifi_diag_event->event_id,
				pwifi_diag_event->seq_num);
			switch (pwifi_diag_event->event_id) {
			case WIFI_DIAG_MAC_TX_FRAME_EVENTID:
			{
				struct wifi_diag_mac_tx_frame_event_t
							*ptx_frame_event_data;
				ptx_frame_event_data =
				(struct wifi_diag_mac_tx_frame_event_t *)
					pwifi_diag_event->event_data;

				if (ptx_frame_event_data->frame_data != 0) {
					printk(KERN_INFO "tx wh[0]=%x wh[%d]=%x\n",
					ptx_frame_event_data->frame_data[0],
					ptx_frame_event_data->frame_length-1,
					ptx_frame_event_data->
					frame_data[ptx_frame_event_data->
						frame_length-1]);
				}

				printk(KERN_INFO
					"frame_type 0x%x frame_subtype 0x%x flen 0x%x pwr %d mcs %d bitrate %d\n",
					ptx_frame_event_data->frame_type,
					ptx_frame_event_data->frame_sub_type,
					ptx_frame_event_data->frame_length,
					ptx_frame_event_data->tx_pwr,
					ptx_frame_event_data->tx_mcs,
					ptx_frame_event_data->tx_bitrate);
			}
			break;
			case WIFI_DIAG_MAC_RX_FRAME_EVENTID:
			{
				struct wifi_diag_mac_rx_frame_event_t
							*prx_frame_event_data;
				prx_frame_event_data =
				(struct wifi_diag_mac_rx_frame_event_t *)
					pwifi_diag_event->event_data;

				if (prx_frame_event_data->frame_data != 0) {
					printk(KERN_INFO "rx wh[0]=%x wh[%d]=%x\n",
					prx_frame_event_data->frame_data[0],
					prx_frame_event_data->frame_length-1,
					prx_frame_event_data->
					frame_data[prx_frame_event_data->
							frame_length-1]);
				}

				printk(KERN_INFO
					"rssi %d, frame_type 0x%x frame_subtype 0x%x flen 0x%x rateindex %d bitrate %d\n",
					prx_frame_event_data->rssi,
					prx_frame_event_data->frame_type,
					prx_frame_event_data->frame_sub_type,
					prx_frame_event_data->frame_length,
					prx_frame_event_data->rx_mcs,
					prx_frame_event_data->rx_bitrate);
			}
			break;
			case WIFI_DIAG_MAC_FSM_EVENTID:
			{
				struct wifi_diag_mac_fsm_event_t
							*pfsm_event_data;
				pfsm_event_data =
					(struct wifi_diag_mac_fsm_event_t *)
					pwifi_diag_event->event_data;
				printk(KERN_INFO
					"fsm event %d\n", pfsm_event_data->fsm);
			}
			break;
			case WIFI_DIAG_INTERFERENCE_EVENTID:
			{
				struct wifi_diag_interference_event_t
						*pinterference_event_data;
				pinterference_event_data =
				(struct wifi_diag_interference_event_t *)
					pwifi_diag_event->event_data;
				printk(KERN_INFO
					"MAC_PCU_RX_CLEAR_CNT = 0x%x\n",
					pinterference_event_data->rx_clear_cnt);
			}
			break;
			case WIFI_DIAG_RX_TIME_EVENTID:
			{
				struct wifi_diag_rxtime_event_t
							*prxtime_event_data;
				prxtime_event_data =
					(struct wifi_diag_rxtime_event_t *)
					pwifi_diag_event->event_data;
				printk(KERN_INFO
					"MAC_PCU_RX_FRAME_CNT = 0x%x\n",
					prxtime_event_data->rx_frame_cnt);
			}
			break;
			case WIFI_DIAG_PWR_SAVE_EVENTID:
			{
				struct wifi_diag_pwrsave_event_t
							*ppwrsave_event_data;
				ppwrsave_event_data =
					(struct wifi_diag_pwrsave_event_t *)
					pwifi_diag_event->event_data;
				printk(KERN_INFO
					"enum wifi_diag_pwrsave_t = %d\n",
					ppwrsave_event_data->pwrsave);
			}
			break;
			case WIFI_DIAG_TX_STAT_EVENTID:
			{
				u32 i;
				struct wifi_diag_tx_stat_event_t
							*ptx_stat_event_data;
				ptx_stat_event_data =
					(struct wifi_diag_tx_stat_event_t *)
						pwifi_diag_event->event_data;

				printk(KERN_INFO "tx_pkt=%lld, tx_ucast_pkt=%lld, tx_retry_cnt=%lld, tx_fail_cnt=%lld\n",
					ptx_stat_event_data->tx_pkt,
					ptx_stat_event_data->tx_ucast_pkt,
					ptx_stat_event_data->tx_retry_cnt,
					ptx_stat_event_data->tx_fail_cnt);
				for (i = 0; i < 44; i++)
					printk(KERN_INFO "tx_rate_pkt[%d]=%d\n",
						i,
						ptx_stat_event_data->
							tx_rate_pkt[i]
						);
			}
			break;
			case WIFI_DIAG_RX_STAT_EVENTID:
			{
				u32 i;
				struct wifi_diag_rx_stat_event_t
							*prx_stat_event_data;
				prx_stat_event_data =
				(struct wifi_diag_rx_stat_event_t *)
						pwifi_diag_event->event_data;

				printk(KERN_INFO "rx_pkt=%lld, rx_ucast_pkt=%lld, rx_dupl_frame=%lld\n",
					prx_stat_event_data->rx_pkt,
					prx_stat_event_data->rx_ucast_pkt,
					prx_stat_event_data->rx_dupl_frame);
				for (i = 0; i < 44; i++)
					printk(KERN_INFO
						"rx_rate_pkt[%d]=%d\n",
						i,
						prx_stat_event_data->
							rx_rate_pkt[i]
						);
			}
			break;
			default:
			break;
			}
			dev_kfree_skb(skb);
		}
	}
	mutex_unlock(&diag_event_mutex);
}

static DECLARE_WORK(wifi_diag_event_work, wifi_diag_event_process);



void
wifi_diag_mac_tx_frame_event(struct ath6kl_vif *vif,
		struct ath_pktlog_txstatus *txstatus_log)
{
	struct wifi_diag_event_t *pwifi_diag_txframe_event;
	struct wifi_diag_mac_tx_frame_event_t *ptx_frame_event_data;
	struct sk_buff *skb;
	u32 size;
	u32 tx_mcs;
	u8 sgi = 0;
	u32 tx_buf_len = 0;

	if (!diag_local_test) {
		if (!(vif->diag.cfg_mask & WIFI_DIAG_MAC_TX_FRAME_EVENTENABLE)
			|| diag_evt_callback.diag_event_callback == NULL)
			return;
	}

	size = sizeof(*pwifi_diag_txframe_event) +
		sizeof(*ptx_frame_event_data) + 512;
	skb = ath6kl_wmi_get_new_buf(size);
	if (!skb)
		return ;

	pwifi_diag_txframe_event = (struct wifi_diag_event_t *) skb->data;
	pwifi_diag_txframe_event->event_id = WIFI_DIAG_MAC_TX_FRAME_EVENTID;
	pwifi_diag_txframe_event->seq_num = le32_to_cpu(txstatus_log->misc[2]);
	ptx_frame_event_data = (struct wifi_diag_mac_tx_frame_event_t *)
				pwifi_diag_txframe_event->event_data;

	ptx_frame_event_data->tx_pwr = txstatus_log->misc[1];
	tx_mcs = le16_to_cpu(txstatus_log->misc[0]);
	if (tx_mcs > MAX11_RATE_INDEX)
		tx_mcs = MAX11_RATE_INDEX;
	ptx_frame_event_data->tx_mcs = tx_mcs;
	sgi = (txstatus_log->misc[0]>>8) & WHAL_RC_FLAG_SGI;
	ptx_frame_event_data->tx_bitrate = _rate_tbl_11[tx_mcs][sgi];
	ptx_frame_event_data->frame_type = vif->diag.tx_frame_type;
	ptx_frame_event_data->frame_sub_type = vif->diag.tx_frame_subtype;
	ptx_frame_event_data->frame_length = vif->diag.tx_frame_len;

	tx_buf_len = le32_to_cpu(txstatus_log->buf_len);
	pwifi_diag_txframe_event->len = tx_buf_len +
					sizeof(*ptx_frame_event_data) - 1;

	if (ptx_frame_event_data->frame_data != 0) {
		u16 framectrl = vif->diag.tx_frame_subtype << 4 |
				vif->diag.tx_frame_type;

		memset(ptx_frame_event_data->frame_data, 0, 512);
	/* skip 2 pad bytes after qos header, assume there are no addr4 */
		if (IEEE80211_QOS_HAS_SEQ(framectrl)) {
			tx_buf_len -= IEEE80211_QOS_PADLEN;
			pwifi_diag_txframe_event->len -= IEEE80211_QOS_PADLEN;
			ptx_frame_event_data->frame_length -=
						IEEE80211_QOS_PADLEN;
			memcpy(ptx_frame_event_data->frame_data,
				txstatus_log->buf, IEEE80211_QOS_HEADERLEN);
			memcpy(ptx_frame_event_data->frame_data+
					IEEE80211_QOS_HEADERLEN,
				txstatus_log->buf+
					IEEE80211_QOS_HEADERLEN+
					IEEE80211_QOS_PADLEN,
				tx_buf_len - IEEE80211_QOS_HEADERLEN);
		} else {
			memcpy(ptx_frame_event_data->frame_data,
				txstatus_log->buf, tx_buf_len);
		}
	}

	if (vif->diag.diag_event_init) {
		skb_queue_tail(&diag_events, skb);
		schedule_work(&wifi_diag_event_work);
	}
}

/* record last descriptor's type, subtype and len for tx log*/
void
wifi_diag_mac_txctrl_event(struct ath6kl_vif *vif,
		struct ath_pktlog_txctl *txctrl_log)
{
	struct tx_ctrl_desc *txctrl;

	if (!diag_local_test) {
		if (!(vif->diag.cfg_mask & WIFI_DIAG_MAC_TX_FRAME_EVENTENABLE)
			|| diag_evt_callback.diag_event_callback == NULL)
			return;
	}

	vif->diag.tx_frame_type = le16_to_cpu(txctrl_log->framectrl) &
					IEEE80211_FC0_TYPE_MASK;
	vif->diag.tx_frame_subtype = (le16_to_cpu(txctrl_log->framectrl) &
					IEEE80211_FC0_SUBTYPE_MASK) >> 4;
	txctrl = (struct tx_ctrl_desc *)txctrl_log->txdesc_ctl;
	vif->diag.tx_frame_len = le16_to_cpu(WHAL_TXDESC_GET_FRAME_LEN(txctrl));
}

void
wifi_diag_mac_rx_frame_event(struct ath6kl_vif *vif,
		struct ath_pktlog_rx *rx_log)
{
	struct wifi_diag_event_t *pwifi_diag_rxframe_event;
	struct wifi_diag_mac_rx_frame_event_t *prx_frame_event_data;
	struct sk_buff *skb;
	u32 size;
	struct rx_desc_status *rxstatus;
	u8 sgi = 0;
	u32 rx_mcs = 0, rx_buf_len = 0;

	if (!diag_local_test) {
		if (!(vif->diag.cfg_mask & WIFI_DIAG_MAC_RX_FRAME_EVENTENABLE)
			|| diag_evt_callback.diag_event_callback == NULL)
			return;
	}

	size = sizeof(*pwifi_diag_rxframe_event) +
		sizeof(*prx_frame_event_data) + 512;
	skb = ath6kl_wmi_get_new_buf(size);
	if (!skb)
		return ;

	pwifi_diag_rxframe_event = (struct wifi_diag_event_t *) skb->data;
	pwifi_diag_rxframe_event->event_id = WIFI_DIAG_MAC_RX_FRAME_EVENTID;
	pwifi_diag_rxframe_event->seq_num = rx_log->seq_num;
	prx_frame_event_data = (struct wifi_diag_mac_rx_frame_event_t *)
				pwifi_diag_rxframe_event->event_data;
	rxstatus = (struct rx_desc_status *)&rx_log->rxstatus[0];
	prx_frame_event_data->rssi = le16_to_cpu(rxstatus->rsRssi) +
					rx_log->calibratednf;
	/* this needs to be compensated withnosie floor later */
	prx_frame_event_data->snr = le16_to_cpu(rxstatus->rsRssi);
	rx_mcs = le32_to_cpu(rx_log->rxmcs);
	if (rx_mcs > MAX11_RATE_INDEX)
		rx_mcs = MAX11_RATE_INDEX;
	prx_frame_event_data->rx_mcs = rx_mcs;
	sgi = rxstatus->rsRate.flags & WHAL_RC_FLAG_SGI;
	prx_frame_event_data->rx_bitrate = _rate_tbl_11[rx_mcs][sgi];
	prx_frame_event_data->fcs = (rxstatus->rsStatus & WHAL_RXERR_CRC) ?
					1 : 0;
	prx_frame_event_data->frame_type =
		le16_to_cpu(rx_log->framectrl) & IEEE80211_FC0_TYPE_MASK;
	prx_frame_event_data->frame_sub_type =
		(le16_to_cpu(rx_log->framectrl) &
		IEEE80211_FC0_SUBTYPE_MASK) >> 4;
	prx_frame_event_data->frame_length = le16_to_cpu(rxstatus->rsDataLen);

	rx_buf_len = le32_to_cpu(rx_log->buf_len);
	pwifi_diag_rxframe_event->len = rx_buf_len +
					sizeof(*prx_frame_event_data) - 1;

	if (prx_frame_event_data->frame_data != 0) {
		memset(prx_frame_event_data->frame_data, 0, 512);
		/* skip 2 pad bytes after qos header,
		   assume there are no addr4 */
		if (IEEE80211_QOS_HAS_SEQ(le16_to_cpu(rx_log->framectrl))) {
			rx_buf_len -= IEEE80211_QOS_PADLEN;
			pwifi_diag_rxframe_event->len -= IEEE80211_QOS_PADLEN;
			prx_frame_event_data->frame_length -=
						IEEE80211_QOS_PADLEN;
			memcpy(prx_frame_event_data->frame_data,
				rx_log->buf, IEEE80211_QOS_HEADERLEN);
			memcpy(prx_frame_event_data->frame_data+
						IEEE80211_QOS_HEADERLEN,
				rx_log->buf+IEEE80211_QOS_HEADERLEN+
						IEEE80211_QOS_PADLEN,
				rx_buf_len - IEEE80211_QOS_HEADERLEN);
		} else {
			memcpy(prx_frame_event_data->frame_data,
				rx_log->buf, rx_buf_len);
		}
	}

	if (vif->diag.diag_event_init) {
		skb_queue_tail(&diag_events, skb);
		schedule_work(&wifi_diag_event_work);
	}
}

void
wifi_diag_mac_fsm_event(struct ath6kl_vif *vif,
		enum wifi_diag_mac_fsm_t eventtype, u32 seq_num)
{
	struct wifi_diag_event_t *pwifi_diag_event;
	struct wifi_diag_mac_fsm_event_t *pfsm_event_data;
	struct sk_buff *skb;
	u32 size;

	if (!diag_local_test) {
		if (!(vif->diag.cfg_mask & WIFI_DIAG_MAC_FSM_EVENTENABLE)
			|| diag_evt_callback.diag_event_callback == NULL)
			return;
	}

	size = sizeof(*pwifi_diag_event) + sizeof(*pfsm_event_data) ;
	skb = ath6kl_wmi_get_new_buf(size);
	if (!skb)
		return;

	pwifi_diag_event = (struct wifi_diag_event_t *) skb->data;
	pwifi_diag_event->event_id = WIFI_DIAG_MAC_FSM_EVENTID;
	pwifi_diag_event->seq_num = seq_num;
	pwifi_diag_event->len = sizeof(*pfsm_event_data);
	pfsm_event_data = (struct wifi_diag_mac_fsm_event_t *)
			pwifi_diag_event->event_data;
	pfsm_event_data->fsm = eventtype;

	if (vif->diag.diag_event_init) {
		skb_queue_tail(&diag_events, skb);
		schedule_work(&wifi_diag_event_work);
	}
}

void
wifi_diag_send_pwrsave_event(struct ath6kl_vif *vif,
		enum wifi_diag_pwrsave_t pwrsave, u32 seq_num)
{
	struct wifi_diag_event_t *pwifi_diag_pwrsave_event;
	struct wifi_diag_pwrsave_event_t *ppwrsave_event_data;
	struct sk_buff *skb;
	u32 size;

	if (!diag_local_test) {
		if (!(vif->diag.cfg_mask & WIFI_DIAG_PWR_SAVE_EVENTENABLE)
			|| diag_evt_callback.diag_event_callback == NULL)
			return;
	}

	size = sizeof(*pwifi_diag_pwrsave_event) + sizeof(*ppwrsave_event_data);
	skb = ath6kl_wmi_get_new_buf(size);
	if (!skb)
		return;

	pwifi_diag_pwrsave_event = (struct wifi_diag_event_t *) skb->data;
	pwifi_diag_pwrsave_event->event_id = WIFI_DIAG_PWR_SAVE_EVENTID;
	pwifi_diag_pwrsave_event->seq_num = seq_num;
	pwifi_diag_pwrsave_event->len = sizeof(*ppwrsave_event_data);
	ppwrsave_event_data = (struct wifi_diag_pwrsave_event_t *)
			pwifi_diag_pwrsave_event->event_data;
	ppwrsave_event_data->pwrsave = pwrsave;

	if (vif->diag.diag_event_init) {
		skb_queue_tail(&diag_events, skb);
		schedule_work(&wifi_diag_event_work);
	}
}

int
ath6kl_wmi_stat_rx_rate_event(struct ath6kl_vif *vif,
		struct wmi *wmi, u8 *datap, int len, u32 seq_num)
{
	struct wifi_diag_event_t *pwifi_diag_rxstat_event;
	struct wifi_diag_rx_stat_event_t *prx_stat_event_data;
	struct sk_buff *skb;
	u32 size, i;
	u32 rx_rate_pkt[44];

	memcpy(&rx_rate_pkt[0], datap, sizeof(u32)*44);

	if (!diag_local_test) {
		if (!(vif->diag.cfg_mask & WIFI_DIAG_RX_STAT_EVENTENABLE)
			|| diag_evt_callback.diag_event_callback == NULL)
			return -EINVAL;
	}

	printk(KERN_INFO "ath6kl_wmi_stat_rx_rate_event\n");

	size = sizeof(*pwifi_diag_rxstat_event) + sizeof(*prx_stat_event_data);
	skb = ath6kl_wmi_get_new_buf(size);
	if (!skb)
		return -ENOMEM;

	pwifi_diag_rxstat_event = (struct wifi_diag_event_t *) skb->data;
	pwifi_diag_rxstat_event->event_id = WIFI_DIAG_RX_STAT_EVENTID;
	pwifi_diag_rxstat_event->seq_num = seq_num;
	pwifi_diag_rxstat_event->len = sizeof(*prx_stat_event_data);
	prx_stat_event_data = (struct wifi_diag_rx_stat_event_t *)
			pwifi_diag_rxstat_event->event_data;
	prx_stat_event_data->rx_pkt = vif->target_stats.rx_pkt;
	prx_stat_event_data->rx_ucast_pkt = vif->target_stats.rx_ucast_pkt;
	prx_stat_event_data->rx_dupl_frame = vif->target_stats.rx_dupl_frame;
	for (i = 0; i < 44; i++)
		prx_stat_event_data->rx_rate_pkt[i] = rx_rate_pkt[i];

	if (vif->diag.diag_event_init) {
		skb_queue_tail(&diag_events, skb);
		schedule_work(&wifi_diag_event_work);
	}

	return 0;
}

int
ath6kl_wmi_stat_tx_rate_event(struct ath6kl_vif *vif,
		struct wmi *wmi, u8 *datap, int len, u32 seq_num)
{
	struct wifi_diag_event_t *pwifi_diag_txstat_event;
	struct wifi_diag_tx_stat_event_t *ptx_stat_event_data;
	struct sk_buff *skb;
	u32 size, i;
	u32 tx_rate_pkt[44];

	memcpy(&tx_rate_pkt[0], datap, sizeof(u32)*44);

	if (!diag_local_test) {
		if (!(vif->diag.cfg_mask & WIFI_DIAG_TX_STAT_EVENTENABLE)
			|| diag_evt_callback.diag_event_callback == NULL)
			return -EINVAL;
	}

	printk(KERN_INFO "ath6kl_wmi_stat_tx_rate_event\n");

	size = sizeof(*pwifi_diag_txstat_event) + sizeof(*ptx_stat_event_data);
	skb = ath6kl_wmi_get_new_buf(size);
	if (!skb)
		return -ENOMEM;

	pwifi_diag_txstat_event = (struct wifi_diag_event_t *) skb->data;
	pwifi_diag_txstat_event->event_id = WIFI_DIAG_TX_STAT_EVENTID;
	pwifi_diag_txstat_event->seq_num = seq_num;
	pwifi_diag_txstat_event->len = sizeof(*ptx_stat_event_data);
	ptx_stat_event_data = (struct wifi_diag_tx_stat_event_t *)
			pwifi_diag_txstat_event->event_data;
	ptx_stat_event_data->tx_pkt = vif->target_stats.tx_pkt;
	ptx_stat_event_data->tx_ucast_pkt = vif->target_stats.tx_ucast_pkt;
	ptx_stat_event_data->tx_retry_cnt = vif->target_stats.tx_retry_cnt;
	ptx_stat_event_data->tx_fail_cnt = vif->target_stats.tx_fail_cnt;
	for (i = 0; i < 44; i++)
		ptx_stat_event_data->tx_rate_pkt[i] = tx_rate_pkt[i];

	if (vif->diag.diag_event_init) {
		skb_queue_tail(&diag_events, skb);
		schedule_work(&wifi_diag_event_work);
	}

	return 0;
}

int
ath6kl_wmi_interference_event(struct ath6kl_vif *vif,
		struct wmi *wmi, u8 *datap, int len, u32 seq_num)
{
	struct wifi_diag_event_t *pwifi_diag_interference_event;
	struct wifi_diag_interference_event_t *pinterference_event_data;
	struct sk_buff *skb;
	u32 size, rx_clear_cnt;

	printk(KERN_INFO "ath6kl_wmi_interference_event\n");

	if (!diag_local_test) {
		if (!(vif->diag.cfg_mask & WIFI_DIAG_INTERFERENCE_EVENTENABLE)
			|| diag_evt_callback.diag_event_callback == NULL)
			return -EINVAL;
	}

	size = sizeof(*pwifi_diag_interference_event) +
		sizeof(*pinterference_event_data);
	skb = ath6kl_wmi_get_new_buf(size);
	if (!skb)
		return -ENOMEM;

	pwifi_diag_interference_event = (struct wifi_diag_event_t *) skb->data;
	pwifi_diag_interference_event->event_id =
				WIFI_DIAG_INTERFERENCE_EVENTID;
	pwifi_diag_interference_event->seq_num = seq_num;
	pwifi_diag_interference_event->len = sizeof(*pinterference_event_data);
	pinterference_event_data = (struct wifi_diag_interference_event_t *)
				pwifi_diag_interference_event->event_data;

	rx_clear_cnt = *((u32 *)datap);
	if (rx_clear_cnt >= vif->diag.pre_rx_clear_cnt)
		pinterference_event_data->rx_clear_cnt =
			rx_clear_cnt - vif->diag.pre_rx_clear_cnt;
	else
		pinterference_event_data->rx_clear_cnt =
		0xFFFFFFFF - (vif->diag.pre_rx_clear_cnt - rx_clear_cnt);
	vif->diag.pre_rx_clear_cnt = rx_clear_cnt;

	if (vif->diag.diag_event_init) {
		skb_queue_tail(&diag_events, skb);
		schedule_work(&wifi_diag_event_work);
	}
	return 0;
}

int
ath6kl_wmi_rxtime_event(struct ath6kl_vif *vif,
		struct wmi *wmi, u8 *datap, int len, u32 seq_num)
{
	struct wifi_diag_event_t *pwifi_diag_rxtime_event;
	struct wifi_diag_rxtime_event_t *prxtime_event_data;
	struct sk_buff *skb;
	u32 size, rx_frame_cnt;

	printk(KERN_INFO KERN_INFO KERN_INFO "ath6kl_wmi_rxtime_event\n");

	if (!diag_local_test) {
		if (!(vif->diag.cfg_mask & WIFI_DIAG_RX_TIME_EVENTENABLE)
			|| diag_evt_callback.diag_event_callback == NULL)
			return -EINVAL;
	}

	size = sizeof(*pwifi_diag_rxtime_event) + sizeof(*prxtime_event_data);
	skb = ath6kl_wmi_get_new_buf(size);
	if (!skb)
		return -ENOMEM;

	pwifi_diag_rxtime_event = (struct wifi_diag_event_t *) skb->data;
	pwifi_diag_rxtime_event->event_id = WIFI_DIAG_RX_TIME_EVENTID;
	pwifi_diag_rxtime_event->seq_num = seq_num;
	pwifi_diag_rxtime_event->len = sizeof(*prxtime_event_data);
	prxtime_event_data = (struct wifi_diag_rxtime_event_t *)
				pwifi_diag_rxtime_event->event_data;

	rx_frame_cnt = *((u32 *)datap);
	if (rx_frame_cnt >= vif->diag.pre_rx_frame_cnt)
		prxtime_event_data->rx_frame_cnt =
			rx_frame_cnt - vif->diag.pre_rx_frame_cnt;
	else
		prxtime_event_data->rx_frame_cnt =
		0xFFFFFFFF - (vif->diag.pre_rx_frame_cnt - rx_frame_cnt);
	vif->diag.pre_rx_frame_cnt = rx_frame_cnt;

	if (vif->diag.diag_event_init) {
		skb_queue_tail(&diag_events, skb);
		schedule_work(&wifi_diag_event_work);
	}
	return 0;
}

void
wifi_diag_tx_stat_timer_handler(unsigned long ptr)
{
	struct ath6kl_vif *vif = (struct ath6kl_vif *)ptr;
	struct sk_buff *skb;

	ath6kl_wmi_get_stats_cmd(globalwmi, 0);

	skb = ath6kl_wmi_get_new_buf(sizeof(struct wmid_cmd_hdr));
	if (skb)
		ath6kl_wmi_cmd_send_diag(globalwmi, skb,
					WMID_STAT_TX_RATE_CMDID,
					NO_SYNC_WMIFLAG);

	mod_timer(&vif->diag.tx_stat_timer,
		jiffies + msecs_to_jiffies(vif->diag.tx_timer_val));
}

void
wifi_diag_rx_stat_timer_handler(unsigned long ptr)
{
	struct ath6kl_vif *vif = (struct ath6kl_vif *)ptr;
	struct sk_buff *skb;

	ath6kl_wmi_get_stats_cmd(globalwmi, 0);

	skb = ath6kl_wmi_get_new_buf(sizeof(struct wmid_cmd_hdr));
	if (skb)
		ath6kl_wmi_cmd_send_diag(globalwmi, skb,
					WMID_STAT_RX_RATE_CMDID,
					NO_SYNC_WMIFLAG);

	mod_timer(&vif->diag.rx_stat_timer,
		jiffies + msecs_to_jiffies(vif->diag.rx_timer_val));
}

void
wifi_diag_interference_timer_handler(unsigned long ptr)
{
	struct ath6kl_vif *vif = (struct ath6kl_vif *)ptr;

	ath6kl_wmi_interference_cmd(globalwmi);
	mod_timer(&vif->diag.interference_timer,
			jiffies + msecs_to_jiffies(1000));

	return;
}

void
wifi_diag_rxtime_timer_handler(unsigned long ptr)
{
	struct ath6kl_vif *vif = (struct ath6kl_vif *)ptr;

	ath6kl_wmi_rxtime_cmd(globalwmi);
	mod_timer(&vif->diag.rxtime_timer, jiffies + msecs_to_jiffies(1000));

	return;
}

void
wifi_diag_timer_destroy(struct ath6kl_vif *vif)
{
	del_timer(&vif->diag.tx_stat_timer);
	del_timer(&vif->diag.rx_stat_timer);
	del_timer(&vif->diag.interference_timer);
	del_timer(&vif->diag.rxtime_timer);

	if (vif->diag.diag_event_init) {
		skb_queue_purge(&diag_events);
		vif->diag.diag_event_init = false;
	}
}

void
wifi_diag_init(void)
{
	struct ath6kl_vif *vif = ath6kl_get_vif_by_index(
					globalwmi->parent_dev, 0);

	if (vif == NULL)
		return;

	skb_queue_head_init(&diag_events);
	vif->diag.diag_event_init = true;

	if (diag_local_test & WIFI_DIAG_MAC_FSM_EVENTENABLE)
		ath6kl_wmi_fsm_cmd(globalwmi, true);

	if (diag_local_test & WIFI_DIAG_PWR_SAVE_EVENTENABLE)
		ath6kl_wmi_pwrsave_cmd(globalwmi, true);

	if (diag_local_test & WIFI_DIAG_INTERFERENCE_EVENTENABLE) {
		del_timer(&vif->diag.interference_timer);
		init_timer(&vif->diag.interference_timer);
		setup_timer(&vif->diag.interference_timer,
			wifi_diag_interference_timer_handler,
			(unsigned long) vif);
		mod_timer(&vif->diag.interference_timer,
			jiffies + msecs_to_jiffies(1000));
	}

	if (diag_local_test & WIFI_DIAG_RX_TIME_EVENTENABLE) {
		del_timer(&vif->diag.rxtime_timer);
		init_timer(&vif->diag.rxtime_timer);
		setup_timer(&vif->diag.rxtime_timer,
			wifi_diag_rxtime_timer_handler, (unsigned long) vif);
		mod_timer(&vif->diag.rxtime_timer,
			jiffies + msecs_to_jiffies(1000));
	}

	if (diag_local_test & WIFI_DIAG_TX_STAT_EVENTENABLE) {
		del_timer(&vif->diag.tx_stat_timer);
		init_timer(&vif->diag.tx_stat_timer);
		setup_timer(&vif->diag.tx_stat_timer,
			wifi_diag_tx_stat_timer_handler, (unsigned long) vif);
		vif->diag.tx_timer_val = 2000;
		mod_timer(&vif->diag.tx_stat_timer,
			jiffies + msecs_to_jiffies(vif->diag.tx_timer_val));
	}

	if (diag_local_test & WIFI_DIAG_RX_STAT_EVENTENABLE) {
		del_timer(&vif->diag.rx_stat_timer);
		init_timer(&vif->diag.rx_stat_timer);
		setup_timer(&vif->diag.rx_stat_timer,
			wifi_diag_rx_stat_timer_handler, (unsigned long) vif);
		vif->diag.rx_timer_val = 2000;
		mod_timer(&vif->diag.rx_stat_timer,
			jiffies + msecs_to_jiffies(vif->diag.rx_timer_val));
	}

	if ((diag_local_test & WIFI_DIAG_MAC_TX_FRAME_EVENTENABLE) ||
	    (diag_local_test & WIFI_DIAG_MAC_RX_FRAME_EVENTENABLE)) {
		if ((diag_local_test & 0x00000200) &&
		    (diag_local_test & 0xFFFF0000)) {
			ath6kl_wmi_macfilter_cmd(globalwmi,
				WMI_PKTLOG_EVENT_TX | WMI_PKTLOG_EVENT_RX,
				((diag_local_test & 0xFFFF0000)>>16) &
					WIFI_DIAG_MACFILTER_LOW_MASK,
				WIFI_DIAG_MACFILTER_DISABLEALL &
					WIFI_DIAG_MACFILTER_HIGH_MASK);
		} else if ((diag_local_test & 0x00000800) &&
			   (diag_local_test & 0xFFFF0000)) {
			ath6kl_wmi_macfilter_cmd(globalwmi,
				WMI_PKTLOG_EVENT_TX | WMI_PKTLOG_EVENT_RX,
				WIFI_DIAG_MACFILTER_DISABLEALL &
					WIFI_DIAG_MACFILTER_LOW_MASK,
				((diag_local_test & 0xFFFF0000)>>16) &
					WIFI_DIAG_MACFILTER_HIGH_MASK);
		} else {
			ath6kl_wmi_macfilter_cmd(globalwmi,
				WMI_PKTLOG_EVENT_TX | WMI_PKTLOG_EVENT_RX,
				WIFI_DIAG_MACFILTER_ENABLEALL &
					WIFI_DIAG_MACFILTER_LOW_MASK,
				WIFI_DIAG_MACFILTER_ENABLEALL &
					WIFI_DIAG_MACFILTER_HIGH_MASK);
		}
		{
		struct wmi_enable_pktlog_cmd cmd;
		cmd.option = WMI_PKTLOG_OPTION_LOG_DIAGNOSTIC;
		cmd.evlist = WMI_PKTLOG_EVENT_TX | WMI_PKTLOG_EVENT_RX;
		cmd.trigger_interval = 0;
		cmd.trigger_tail_count = 0;
		cmd.trigger_thresh = 0;
		cmd.buffer_size = 1500;
		ath6kl_wmi_pktlog_enable_cmd(globalwmi, &cmd);
		}
	}
}

#endif /* ATH6KL_DIAGNOSTIC */
