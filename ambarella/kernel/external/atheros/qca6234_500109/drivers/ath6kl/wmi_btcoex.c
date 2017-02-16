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
#include "wmi_btcoex.h"
#include "debug.h"
#ifdef CONFIG_ANDROID
#include <mach/socinfo.h>
#endif

#define BDATA_ANTCONF_OFFSET	4069
#define BDATA_BTDEV_OFFSET	4070

#define BT_DEVICE_TYPE_QCOM	2

#define WMI_A2DP_CONFIG_FLAG_ALLOW_OPTIMIZATION    (1 << 0)
#define WMI_A2DP_CONFIG_FLAG_IS_EDR_CAPABLE        (1 << 1)
#define WMI_A2DP_CONFIG_FLAG_IS_BT_ROLE_MASTER     (1 << 2)
#define WMI_A2DP_CONFIG_FLAG_IS_A2DP_HIGH_PRI      (1 << 3)
#define WMI_A2DP_CONFIG_FLAG_FIND_BT_ROLE          (1 << 4)
#define WMI_A2DP_CONFIG_FLAG_DIS_SCANCONN_STOMP    (1 << 5)

#define BTCOEX_A2DP_MAX_BLUETOOTH_TIME_LSB 24
#define BTCOEX_A2DP_EDR_MAX_BLUETOOTH_TIME     \
	(40 << BTCOEX_A2DP_MAX_BLUETOOTH_TIME_LSB)
#define BTCOEX_A2DP_BDR_MAX_BLUETOOTH_TIME     \
	(70 << BTCOEX_A2DP_MAX_BLUETOOTH_TIME_LSB)
#define BTCOEX_APMODE_A2DP_MAX_BLUETOOTH_TIME     \
	(30 << BTCOEX_A2DP_MAX_BLUETOOTH_TIME_LSB)
#define BTCOEX_APMODE_A2DP_BDR_MAX_BLUETOOTH_TIME     \
	(60 << BTCOEX_A2DP_MAX_BLUETOOTH_TIME_LSB)
#define BTCOEX_A2DP_WLAN_MAX_DUR			   25
#define BTCOEX_A2DP_BDR_WLAN_MAX_DUR           20
#define BTCOEX_APMODE_A2DP_BDR_WLAN_MAX_DUR    40
#define BTCOEX_APMODE_A2DP_WLAN_MAX_DUR	       70
#define BTCOEX_APMODE_A2DP_MIN_BURST_CNT       10

#define WMI_SCO_CONFIG_FLAG_ALLOW_OPTIMIZATION   (1 << 0)
#define WMI_SCO_CONFIG_FLAG_IS_EDR_CAPABLE       (1 << 1)
#define WMI_SCO_CONFIG_FLAG_IS_BT_MASTER         (1 << 2)
#define WMI_SCO_CONFIG_FLAG_FW_DETECT_OF_PER     (1 << 3)
#define WMI_SCO_CONFIG_FLAG_DIS_SCANCONN_STOMP   (1 << 4)

static inline struct sk_buff *ath6kl_wmi_btcoex_get_new_buf(u32 size)
{
	struct sk_buff *skb;

	skb = ath6kl_buf_alloc(size);
	if (!skb)
		return NULL;

	skb_put(skb, size);
	if (size)
		memset(skb->data, 0, size);

	return skb;
}

/*  This is a temporary WAR since BTC related NL80211 commands are
 *  defined in Linux nl80211.h.
 */
#define NL80211_WMI_SET_BT_STATUS		0
#define NL80211_WMI_SET_BT_PARAMS		1
#define NL80211_WMI_SET_BT_FT_ANT		2
#define NL80211_WMI_SET_COLOCATED_BT_DEV	3
#define NL80211_WMI_SET_BT_INQUIRY_PAGE_CONFIG	4
#define NL80211_WMI_SET_BT_SCO_CONFIG		5
#define NL80211_WMI_SET_BT_A2DP_CONFIG		6
#define NL80211_WMI_SET_BT_ACLCOEX_CONFIG	7
#define NL80211_WMI_SET_BT_DEBUG		8
#define NL80211_WMI_SET_BT_OPSTATUS		9
#define NL80211_WMI_GET_BT_CONFIG		10
#define NL80211_WMI_GET_BT_STATS		11
#define NL80211_WMI_SET_BT_HID_CONFIG		12

static int ath6kl_get_wmi_cmd(int nl_cmd)
{
	int wmi_cmd = 0;
	switch (nl_cmd) {
	case NL80211_WMI_SET_BT_STATUS:
		ath6kl_dbg(ATH6KL_DBG_WMI, "Set BT status\n");
		wmi_cmd = WMI_SET_BT_STATUS_CMDID;
		break;

	case NL80211_WMI_SET_BT_PARAMS:
		ath6kl_dbg(ATH6KL_DBG_WMI, "Set BT params\n");
		wmi_cmd = WMI_SET_BT_PARAMS_CMDID;
		break;

	case NL80211_WMI_SET_BT_FT_ANT:
		ath6kl_dbg(ATH6KL_DBG_WMI, "Set BT FT antenna\n");
		wmi_cmd = WMI_SET_BTCOEX_FE_ANT_CMDID;
		break;

	case NL80211_WMI_SET_COLOCATED_BT_DEV:
		ath6kl_dbg(ATH6KL_DBG_WMI, "Set BT collocated dev\n");
		wmi_cmd = WMI_SET_BTCOEX_COLOCATED_BT_DEV_CMDID;
		break;

	case NL80211_WMI_SET_BT_INQUIRY_PAGE_CONFIG:
		ath6kl_dbg(ATH6KL_DBG_WMI, "Set BT inquiry page\n");
		wmi_cmd = WMI_SET_BTCOEX_BTINQUIRY_PAGE_CONFIG_CMDID;
		break;

	case NL80211_WMI_SET_BT_SCO_CONFIG:
		ath6kl_dbg(ATH6KL_DBG_WMI, "Set BT sco config\n");
		wmi_cmd = WMI_SET_BTCOEX_SCO_CONFIG_CMDID;
		break;

	case NL80211_WMI_SET_BT_A2DP_CONFIG:
		ath6kl_dbg(ATH6KL_DBG_WMI, "Set BT a2dp config\n");
		wmi_cmd = WMI_SET_BTCOEX_A2DP_CONFIG_CMDID;
		break;

	case NL80211_WMI_SET_BT_ACLCOEX_CONFIG:
		ath6kl_dbg(ATH6KL_DBG_WMI, "Set BT acl config\n");
		wmi_cmd = WMI_SET_BTCOEX_ACLCOEX_CONFIG_CMDID;
		break;

	case NL80211_WMI_SET_BT_DEBUG:
		ath6kl_dbg(ATH6KL_DBG_WMI, "Set BT bt debug\n");
		wmi_cmd = WMI_SET_BTCOEX_DEBUG_CMDID;
		break;

	case NL80211_WMI_SET_BT_OPSTATUS:
		ath6kl_dbg(ATH6KL_DBG_WMI, "Set BT op status\n");
		wmi_cmd = WMI_SET_BTCOEX_BT_OPERATING_STATUS_CMDID;
		break;

	case NL80211_WMI_GET_BT_CONFIG:
		ath6kl_dbg(ATH6KL_DBG_WMI, "Get BT config\n");
		wmi_cmd = WMI_GET_BTCOEX_CONFIG_CMDID;
		break;

	case NL80211_WMI_GET_BT_STATS:
		ath6kl_dbg(ATH6KL_DBG_WMI, "Get BT status\n");
		wmi_cmd = WMI_GET_BTCOEX_STATS_CMDID;
		break;

	case NL80211_WMI_SET_BT_HID_CONFIG:
		ath6kl_dbg(ATH6KL_DBG_WMI, "Set BT hid config\n");
		wmi_cmd = WMI_SET_BTCOEX_HID_CONFIG_CMDID;
	}
	return wmi_cmd;
}

u8 fe_antenna_type(struct ath6kl *ar)
{
	/* default setting for McK */
	u8 fe_antenna = WMI_BTCOEX_FE_ANT_DUAL_SH_BT_LOW_ISO;

	if (ar->version.target_ver == AR6004_HW_3_0_VERSION) {
#ifdef CONFIG_ANDROID
		if (machine_is_apq8064_dma() ||
			machine_is_apq8064_bueller())
			fe_antenna =
				WMI_BTCOEX_FE_ANT_DUAL_SH_BT_LOW_ISO;
		else
			fe_antenna =
				WMI_BTCOEX_FE_ANT_DUAL_SH_BT_HIGH_ISO;
#endif
	} else {
		/* fill in correct antenna configuration from
		   board data if valid */
		if (ar->fw_board[BDATA_ANTCONF_OFFSET])
			fe_antenna =
				ar->fw_board[BDATA_ANTCONF_OFFSET];
	}
	switch (fe_antenna) {
	case WMI_BTCOEX_FE_ANT_SINGLE:
		ath6kl_info("antenna config = single antenna\n");
		break;
	case WMI_BTCOEX_FE_ANT_DUAL_SH_BT_LOW_ISO:
		ath6kl_info("antenna config = daul ant. low isolation\n");
		break;
	case WMI_BTCOEX_FE_ANT_DUAL_SH_BT_HIGH_ISO:
		ath6kl_info("antenna config = dual ant. high isolation\n");
		break;
	case WMI_BTCOEX_FE_ANT_TRIPLE:
		ath6kl_info("antenna config = triple antenna\n");
		break;
	default:
		ath6kl_info("antenna config = invalid config!\n");
		break;
	}

	return fe_antenna;
}

void ath6kl_btcoex_adjust_params(struct ath6kl *ar,
			int wmi_cmd, u8 *buf)
{
	switch (wmi_cmd) {
	case WMI_SET_BTCOEX_FE_ANT_CMDID:
	{
		struct wmi_set_btcoex_fe_antenna_cmd *cmd =
			(struct wmi_set_btcoex_fe_antenna_cmd *)buf;

		cmd->fe_antenna_type = fe_antenna_type(ar);

		/* disable green tx if it's enabled & BT is on */
		ar->green_tx_params.enable = false;
		ath6kl_wmi_set_green_tx_params(ar->wmi,
			&ar->green_tx_params);
	}
	break;
	case WMI_SET_BTCOEX_COLOCATED_BT_DEV_CMDID:
	{
		struct wmi_set_btcoex_colocated_bt_dev_cmd *cmd =
			(struct wmi_set_btcoex_colocated_bt_dev_cmd *)buf;
		/* Regardless of setting, force to use
		 * qcom-colocated BT. It's the current working mode.
		 */
		ar->btcoex_info.bt_vendor = BT_DEVICE_TYPE_QCOM;
		cmd->colocated_bt_dev = BT_DEVICE_TYPE_QCOM;
	}
	break;
	case WMI_SET_BTCOEX_A2DP_CONFIG_CMDID:
	{
		struct wmi_set_btcoex_a2dp_config_cmd *cmd =
			(struct wmi_set_btcoex_a2dp_config_cmd *)buf;
		struct btcoex_a2dp_config *a2dp_config = &cmd->a2dp_config;
		struct btcoex_pspoll_a2dp_config *pspoll_config =
							&cmd->pspoll_config;
		struct ath6kl_vif *vif;

		if (ar->btcoex_info.bt_vendor == BT_DEVICE_TYPE_QCOM) {
			a2dp_config->a2dp_flags |= cpu_to_le32(
				WMI_A2DP_CONFIG_FLAG_ALLOW_OPTIMIZATION);
		} else {
			a2dp_config->a2dp_flags |= cpu_to_le32(
				WMI_A2DP_CONFIG_FLAG_IS_A2DP_HIGH_PRI);
		}

		if (a2dp_config->a2dp_flags &
			WMI_A2DP_CONFIG_FLAG_IS_EDR_CAPABLE) {
			/* A2DP EDR config overwrites */

			if (a2dp_config->a2dp_flags &
				WMI_A2DP_CONFIG_FLAG_IS_BT_ROLE_MASTER) {

				a2dp_config->a2dp_flags |= cpu_to_le32(
					BTCOEX_A2DP_EDR_MAX_BLUETOOTH_TIME);

				/* disable stomping BT during WLAN scan
				 * or connection
				 */
				a2dp_config->a2dp_flags |= cpu_to_le32(
					WMI_A2DP_CONFIG_FLAG_DIS_SCANCONN_STOMP
					);
			} else {
				/* use BDR parameter for A2DP EDR slave case */
				a2dp_config->a2dp_flags |= cpu_to_le32(
					BTCOEX_A2DP_BDR_MAX_BLUETOOTH_TIME);
			}
			pspoll_config->a2dp_wlan_max_dur =
				BTCOEX_A2DP_WLAN_MAX_DUR;
		} else {
			/* A2DP BDR config overwrites */
			a2dp_config->a2dp_flags |= cpu_to_le32(
				BTCOEX_A2DP_BDR_MAX_BLUETOOTH_TIME);
			pspoll_config->a2dp_wlan_max_dur =
				BTCOEX_A2DP_BDR_WLAN_MAX_DUR;
		}

		/* change A2DP parameters for AP mode*/
		vif = ath6kl_vif_first(ar);
		if (!vif)
			return;

		if (vif->nw_type == AP_NETWORK) {
			if (a2dp_config->a2dp_flags &
				WMI_A2DP_CONFIG_FLAG_IS_EDR_CAPABLE) {
				pspoll_config->a2dp_wlan_max_dur =
					BTCOEX_APMODE_A2DP_WLAN_MAX_DUR;
				a2dp_config->a2dp_flags |= cpu_to_le32(
					BTCOEX_APMODE_A2DP_MAX_BLUETOOTH_TIME);
			} else {
				pspoll_config->a2dp_wlan_max_dur =
					BTCOEX_APMODE_A2DP_BDR_WLAN_MAX_DUR;
				a2dp_config->a2dp_flags |= cpu_to_le32(
				BTCOEX_APMODE_A2DP_BDR_MAX_BLUETOOTH_TIME);
			}

			if (ar->version.target_ver == AR6004_HW_1_3_VERSION) {
				pspoll_config->a2dp_min_bus_cnt = cpu_to_le32(
					BTCOEX_APMODE_A2DP_MIN_BURST_CNT);
			}
		}
	}
	break;
	case WMI_SET_BTCOEX_SCO_CONFIG_CMDID:
	{
		struct wmi_set_btcoex_sco_config_cmd *cmd =
			(struct wmi_set_btcoex_sco_config_cmd *)buf;
		struct btcoex_sco_config *sco_config = &cmd->sco_config;
		if (sco_config->sco_flags &
			WMI_SCO_CONFIG_FLAG_IS_EDR_CAPABLE) {
			/* disable stomping BT during WLAN scan/connection */
			sco_config->sco_flags |= cpu_to_le32(
				WMI_SCO_CONFIG_FLAG_DIS_SCANCONN_STOMP);
		}
	}
	break;

	}
}

int ath6kl_wmi_send_btcoex_cmd(struct ath6kl *ar,
			u8 *buf, int len)
{
	struct sk_buff *skb;
	u32 nl_cmd;
	int wmi_cmd;
	struct wmi *wmi = ar->wmi;

	nl_cmd = *(u32 *)buf;
	buf += sizeof(u32);
	len -= sizeof(u32);
	wmi_cmd = ath6kl_get_wmi_cmd(nl_cmd);
	if (wmi_cmd == 0)
		return -ENOMEM;

	skb = ath6kl_wmi_btcoex_get_new_buf(len);
	if (!skb)
		return -ENOMEM;

	ath6kl_btcoex_adjust_params(ar, wmi_cmd, buf);

	memcpy(skb->data, buf, len);

	return ath6kl_wmi_cmd_send(wmi, 0, skb,
			(enum wmi_cmd_id)wmi_cmd,
			NO_SYNC_WMIFLAG);
}
