/*
 * Copyright (c) 2004-2012 Atheros Communications Inc.
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
#include "debug.h"

static u8 *htcoex_get_elem(int elem_id, u8 *start, u16 len)
{
	size_t left = len;
	u8 *pos = start;
	u8 *elem_start = NULL;

	if ((start == NULL) || (len == 0))
		return NULL;

	while (left >= 2) {
		u8 id, elen;

		id = *pos++;
		elen = *pos++;
		left -= 2;

		if (elen > left)
			break;

		if (id == elem_id) {
			elem_start = (void *)pos;
			break;
		}

		left -= elen;
		pos += elen;
	}

	return elem_start;
}

static void htcoex_flush_bss_info(struct htcoex *coex)
{
	struct htcoex_bss_info *coex_bss, *tmp;

	spin_lock(&coex->bss_info_lock);

	list_for_each_entry_safe(coex_bss, tmp, &coex->bss_info_list, list) {
		list_del(&coex_bss->list);
		kfree(coex_bss->raw_frame);
		kfree(coex_bss);
	}

	spin_unlock(&coex->bss_info_lock);

	return;
}

static void htcoex_get_coexinfo(struct htcoex_bss_info *coex_bss,
				struct htcoex_coex_info *coex_info)
{
	struct ieee80211_ht_cap *htcap;
	struct ieee80211_mgmt *frame;
	u16 cap_info;
	int chan_id = 0;

	frame = (struct ieee80211_mgmt *)(coex_bss->raw_frame);
	chan_id = ieee80211_frequency_to_channel(
			(int)(coex_bss->channel->center_freq));
	htcap = (struct ieee80211_ht_cap *)htcoex_get_elem(
			WLAN_EID_HT_CAPABILITY, frame->u.beacon.variable,
			coex_bss->frame_len - (24 + 12));

	if (htcap) {
		cap_info = le16_to_cpu(htcap->cap_info);

		ath6kl_dbg(ATH6KL_DBG_HTCOEX, "htcoex BSSID "
			"%02x:%02x:%02x:%02x:%02x:%02x htcap %04x\n",
			frame->bssid[0], frame->bssid[1], frame->bssid[2],
			frame->bssid[3], frame->bssid[4], frame->bssid[5],
			cap_info);

		if (cap_info & IEEE80211_HT_CAP_40MHZ_INTOLERANT)
			coex_info->intolerant40++;
	} else if (coex_bss->channel->band == IEEE80211_BAND_2GHZ) {
		int i;

		for (i = 0; i < coex_info->num_chans; i++) {
			if (chan_id == coex_info->chans[i])
				break;
		}

		if (i == coex_info->num_chans) {
			/* chans only get size 14 */
			BUG_ON(coex_info->num_chans >= 14);
			coex_info->chans[coex_info->num_chans++] = chan_id;
		}
	}

	return;
}

static void htcoex_scan_start(unsigned long arg)
{
	struct htcoex *coex = (struct htcoex *) arg;
	struct ath6kl_vif *vif = coex->vif;
	struct ath6kl *ar;
	int ret;

	BUG_ON(!vif);

	ar = vif->ar;
	if ((vif->nw_type != INFRA_NETWORK) ||
		!test_bit(CONNECTED, &vif->flags) ||
		vif->scan_req ||
		test_bit(EAPOL_HANDSHAKE_PROTECT, &ar->flag))
		goto resche;

	ath6kl_dbg(ATH6KL_DBG_HTCOEX,
		   "htcoex scan (vif %p) num_scan %d\n",
		   vif,
		   coex->num_scan);

	if (!vif->usr_bss_filter) {
		clear_bit(CLEAR_BSSFILTER_ON_BEACON, &vif->flags);
		ret = ath6kl_wmi_bssfilter_cmd(
						ar->wmi,
						vif->fw_vif_idx,
						ALL_BUT_BSS_FILTER,
						0);
		if (ret) {
			ath6kl_err("couldn't set bss filtering\n");
			goto resche;
		}
	}

	/* bypass this time if ROC is ongoing. */
	if (test_bit(ROC_PEND, &vif->flags))
		goto resche;

	ret = ath6kl_wmi_startscan_cmd(ar->wmi, vif->fw_vif_idx, WMI_LONG_SCAN,
				       0, false, 0, 0,
				       coex->num_scan_channels,
				       coex->scan_channels);
	if (ret)
		ath6kl_err("wmi_startscan_cmd failed\n");
	else {
		vif->scan_req = &coex->request;
		coex->num_scan++;
	}

resche:
	if ((coex->flags & ATH6KL_HTCOEX_FLAGS_START) &&
		(coex->scan_interval))
		mod_timer(&coex->scan_timer,
			  jiffies + msecs_to_jiffies(coex->scan_interval));

	return;
}

static void htcoex_send_action(struct ath6kl_vif *vif,
			       struct htcoex_coex_info *coex_info)
{
	struct ath6kl *ar = vif->ar;
	struct ieee80211_mgmt *action_frame;
	int len = 0;

	ath6kl_dbg(ATH6KL_DBG_HTCOEX,
		   "htcoex send action (vif %p) intolerant40 %d num_chans %d bss_ch %d\n",
		   vif,
		   coex_info->intolerant40,
		   coex_info->num_chans,
		   vif->bss_ch);

	len = 24 +
		sizeof(struct ieee80211_action_public) +
		sizeof(struct ieee80211_bss_coex_ie) +
		sizeof(struct ieee80211_intolerant_chan_report_ie) +
		coex_info->num_chans;
	action_frame = kzalloc(len, GFP_KERNEL);
	if (action_frame) {
		struct ieee80211_action_public *action_public;
		struct ieee80211_bss_coex_ie *bss_coex_ie;
		struct ieee80211_intolerant_chan_report_ie *into_chan_report_ie;
		int i;

		/* Build action frame. */
		action_frame->frame_control = cpu_to_le16(IEEE80211_FTYPE_MGMT |
							IEEE80211_STYPE_ACTION);
		memcpy(action_frame->da, vif->bssid, ETH_ALEN);
		memcpy(action_frame->sa, vif->ndev->dev_addr, ETH_ALEN);
		memcpy(action_frame->bssid, vif->bssid, ETH_ALEN);

		action_public = (struct ieee80211_action_public *)
			(&action_frame->u.action);
		action_public->category = WLAN_CATEGORY_PUBLIC;
		action_public->action_code = WLAN_COEX_ACTION_2040COEX_MGMT;

		bss_coex_ie = (struct ieee80211_bss_coex_ie *)
			(action_public->variable);
		bss_coex_ie->element_id = WLAN_EID_BSS_COEX_2040;
		bss_coex_ie->len = 1;
		if (coex_info->intolerant40)
			bss_coex_ie->value |= IEEE80211_COEX_IE_20_WIDTH_REQ;

		into_chan_report_ie =
			(struct ieee80211_intolerant_chan_report_ie *)
				(++bss_coex_ie);
		into_chan_report_ie->element_id =
			WLAN_EID_INTOLERANT_CHAN_REPORT;
		into_chan_report_ie->len = coex_info->num_chans + 1;
		into_chan_report_ie->reg_class = 0;
		for (i = 0; i < coex_info->num_chans; i++)
			into_chan_report_ie->chan_variable[i] =
				coex_info->chans[i];

		ath6kl_dbg_dump(ATH6KL_DBG_RAW_BYTES, __func__, "tx-htcoex ",
						(u8 *)action_frame, len);

		BUG_ON(vif->bss_ch == 0);
		ath6kl_wmi_send_action_cmd(ar->wmi, vif->fw_vif_idx,
					   vif->send_action_id++,
					   vif->bss_ch, 0,
					   (u8 *)action_frame, len);
		kfree(action_frame);
	} else {
		ath6kl_err("failed to alloc memory for action_frame\n");
	}

	return;

}

static void htcoex_ht40_rateset(struct ath6kl_vif *vif,
				struct htcoex *coex,
				bool enabled)
{
	struct ath6kl *ar = vif->ar;
	u64 ratemask;

	if (enabled)
		ratemask = ATH6KL_HTCOEX_RATEMASK_HT40;
	else
		ratemask = ATH6KL_HTCOEX_RATEMASK_HT20;

	ath6kl_dbg(ATH6KL_DBG_HTCOEX,
		   "htcoex rateset (vif %p) HT40 rates %s, %llx -> %llx\n",
		   vif,
		   (enabled ? "Enabled" : "Disabled"),
		   coex->current_ratemask,
		   ratemask);

	if (coex->current_ratemask != ratemask)
		ath6kl_wmi_set_fix_rates(ar->wmi, vif->fw_vif_idx,
					 ratemask);

	coex->current_ratemask = ratemask;

	return;
}

static int
htcoex_clear_scan_channels(struct ath6kl_vif *vif)
{
	struct htcoex *coex = vif->htcoex_ctx;

	if (!coex)
		return -EINVAL;

	kfree(coex->scan_channels);
	coex->scan_channels = NULL;
	coex->num_scan_channels = 0;
	return 0;
}

static int
htcoex_set_scan_channels(struct ath6kl_vif *vif)
{
	struct htcoex *coex = vif->htcoex_ctx;
	struct wiphy *wiphy = coex->request.wiphy;
	int i;

	if (!wiphy->bands[IEEE80211_BAND_2GHZ])
		return -EINVAL;

	/*If the scan channel is already set,
	 * clear it and apply new channel list.
	 */
	if (coex->num_scan_channels != 0 || coex->scan_channels)
		htcoex_clear_scan_channels(vif);

	/*scan_channels may be larger than what we actually need because some
	*  of them might be disabled so we will filter them later.
	*/
	coex->scan_channels =
		kzalloc(wiphy->bands[IEEE80211_BAND_2GHZ]->n_channels *
					sizeof(u16), GFP_KERNEL);

	if (!coex->scan_channels)
		return -ENOMEM;

	/*Copy all  enabled 2G channels.*/
	for (i = 0; i < wiphy->bands[IEEE80211_BAND_2GHZ]->n_channels; i++) {
		struct ieee80211_channel *chan;

		chan = &wiphy->bands[IEEE80211_BAND_2GHZ]->channels[i];

		if (chan->flags & IEEE80211_CHAN_DISABLED)
			continue;

		coex->scan_channels[coex->num_scan_channels++] =
					chan->center_freq;
	}

	return 0;
}

struct htcoex *ath6kl_htcoex_init(struct ath6kl_vif *vif)
{
	struct htcoex *coex;

	coex = kzalloc(sizeof(struct htcoex), GFP_KERNEL);
	if (!coex) {
		ath6kl_err("failed to alloc memory for htcoex\n");
		return NULL;
	}

	coex->vif = vif;

	/* Disable by default. */
	coex->flags &= ~ATH6KL_HTCOEX_FLAGS_ENABLED;
	coex->scan_interval = ATH6KL_HTCOEX_SCAN_PERIOD;
	coex->rate_rollback_interval = ATH6KL_HTCOEX_RATE_ROLLBACK;

	/* Init. periodic scan timer. */
	init_timer(&coex->scan_timer);
	coex->scan_timer.function = htcoex_scan_start;
	coex->scan_timer.data = (unsigned long) coex;

#ifdef CFG80211_NETDEV_REPLACED_BY_WDEV
	coex->request.wdev = &vif->wdev;
#else
	coex->request.dev = vif->ndev;
#endif
	coex->request.wiphy = vif->ar->wiphy;

	coex->num_scan_channels = 0;
	coex->scan_channels = NULL;

	spin_lock_init(&coex->bss_info_lock);
	INIT_LIST_HEAD(&coex->bss_info_list);

	ath6kl_dbg(ATH6KL_DBG_HTCOEX,
		   "htcoex init (vif %p interval %d)\n",
		   vif,
		   coex->scan_interval);

	return coex;
}

void ath6kl_htcoex_deinit(struct ath6kl_vif *vif)
{
	struct htcoex *coex = vif->htcoex_ctx;

	if (coex) {
		if (coex->flags & ATH6KL_HTCOEX_FLAGS_START) {
			del_timer_sync(&coex->scan_timer);
			htcoex_clear_scan_channels(vif);
		}
		htcoex_flush_bss_info(coex);

		kfree(coex);
	}

	vif->htcoex_ctx = NULL;

	ath6kl_dbg(ATH6KL_DBG_HTCOEX,
		   "htcoex deinit (vif %p)\n",
		   vif);

	return;
}

void ath6kl_htcoex_bss_info(struct ath6kl_vif *vif,
			    struct ieee80211_mgmt *mgmt,
			    int len,
			    struct ieee80211_channel *channel)
{
	struct htcoex *coex = vif->htcoex_ctx;
	struct htcoex_bss_info *coex_bss, *tmp;
	int match = 0;

	/* Add bss even scan not issue by htcoex. */
	if (!(coex->flags & ATH6KL_HTCOEX_FLAGS_START) ||
		(vif->nw_type != INFRA_NETWORK) ||
		(memcmp(mgmt->bssid, vif->bssid, ETH_ALEN) == 0))
		return;

	ath6kl_dbg(ATH6KL_DBG_HTCOEX,
		   "htcoex bssinfo (vif %p) BSSID "
		   "%02x:%02x:%02x:%02x:%02x:%02x\n",
		   vif,
		   mgmt->bssid[0], mgmt->bssid[1], mgmt->bssid[2],
		   mgmt->bssid[3], mgmt->bssid[4], mgmt->bssid[5]);

	if (!list_empty(&coex->bss_info_list)) {
		list_for_each_entry_safe(coex_bss, tmp,
			&coex->bss_info_list, list) {
			if (memcmp(coex_bss->bssid,
				mgmt->bssid, ETH_ALEN) == 0) {
				match = 1;
				break;
			}
		}
	}

	if (!match) {
		coex_bss = kzalloc(sizeof(struct htcoex_bss_info), GFP_KERNEL);
		if (!coex_bss) {
			ath6kl_err("failed to alloc memory for coex_bss\n");
			return;
		}

		coex_bss->raw_frame = kmalloc(len, GFP_KERNEL);
		if (!coex_bss->raw_frame) {
			kfree(coex_bss);
			ath6kl_err("failed to alloc memory for coex_bss->raw_frame\n");
			return;
		}

		/* Add BSS info. */
		coex_bss->frame_len = len;
		memcpy(coex_bss->raw_frame, mgmt, len);
		memcpy(coex_bss->bssid, mgmt->bssid, ETH_ALEN);
		coex_bss->channel = channel;

		spin_lock(&coex->bss_info_lock);
		list_add_tail(&coex_bss->list, &coex->bss_info_list);
		spin_unlock(&coex->bss_info_lock);
	}

	return;
}

int ath6kl_htcoex_scan_complete_event(struct ath6kl_vif *vif, bool aborted)
{
	struct htcoex *coex = vif->htcoex_ctx;
	struct htcoex_bss_info *coex_bss, *tmp;
	struct htcoex_coex_info coex_info;
	int ret = HTCOEX_PASS_SCAN_DONE;

	/* Send Action frame even scan issue by user. */
	if (!(coex->flags & ATH6KL_HTCOEX_FLAGS_START) ||
		(vif->nw_type != INFRA_NETWORK) ||
		(!vif->scan_req))
		goto done;

	ath6kl_dbg(ATH6KL_DBG_HTCOEX,
		   "htcoex scan done (vif %p aborted %d) flags %x\n",
		   vif,
		   aborted,
		   coex->flags);

	memset(&coex_info, 0, sizeof(struct htcoex_coex_info));

	/* Parse all Beacon/ProbeResp's IEs to find 20/40 coex. information. */
	list_for_each_entry_safe(coex_bss, tmp, &coex->bss_info_list, list) {
		htcoex_get_coexinfo(coex_bss, &coex_info);
	}

	/* Send action frame to AP. */
	if (coex_info.intolerant40 || coex_info.num_chans)
		htcoex_send_action(vif, &coex_info);

	/* Disable HT40 rates. */
	if (coex_info.intolerant40) {
		htcoex_ht40_rateset(vif, coex, false);
		coex->tolerant40_cnt = 0;
	} else {
		/* Roll-back to HT40 rate if acceptable. */
		if (coex->rate_rollback_interval &&
		    (++coex->tolerant40_cnt > coex->rate_rollback_interval))
			htcoex_ht40_rateset(vif, coex, true);
	}

	/* Issue by htcoex. */
	if (vif->scan_req == &coex->request) {
		vif->scan_req = NULL;
		ret = HTCOEX_PORC_SCAN_DONE;
	}

done:
	/* Always flush it. */
	htcoex_flush_bss_info(coex);

	return ret;
}

void ath6kl_htcoex_connect_event(struct ath6kl_vif *vif)
{
	struct htcoex *coex = vif->htcoex_ctx;
	struct cfg80211_bss *bss;
	u8 *ies = NULL;
	u16 ies_len = 0;

	if (vif->nw_type != INFRA_NETWORK)
		return;

	bss = ath6kl_bss_get(vif->ar,
				NULL,
				vif->bssid,
				vif->ssid,
				vif->ssid_len,
				WLAN_CAPABILITY_ESS,
				WLAN_CAPABILITY_ESS);
	if (!bss) {
		WARN_ON(1);
		return;
	}

#ifdef CFG80211_SAFE_BSS_INFO_ACCESS
	rcu_read_lock();
	if (bss->ies) {
		ies = (u8 *)(bss->ies->data);
		ies_len = bss->ies->len;
	}
#else
	if (bss->information_elements) {
		ies = bss->information_elements;
		ies_len = bss->len_information_elements;
	}
#endif

	/* Start if BSS is 11n and 2G channel. */
	if ((coex->flags & ATH6KL_HTCOEX_FLAGS_ENABLED) &&
		(bss->channel->band == IEEE80211_BAND_2GHZ) &&
		(htcoex_get_elem(WLAN_EID_HT_CAPABILITY,
				 ies,
				 ies_len) != NULL)){
		if (coex->flags & ATH6KL_HTCOEX_FLAGS_START)
			del_timer(&coex->scan_timer);

		if (coex->scan_interval < ATH6KL_HTCOEX_SCAN_PERIOD)
			coex->scan_interval = ATH6KL_HTCOEX_SCAN_PERIOD;

		/*htcoex set scan channels*/
		if (htcoex_set_scan_channels(vif) == 0) {
			mod_timer(&coex->scan_timer, jiffies +
				msecs_to_jiffies(coex->scan_interval));
			coex->flags |= ATH6KL_HTCOEX_FLAGS_START;
		} else {
			ath6kl_err("htcoex set scan channels failed\n");
			WARN_ON(1);
		}

	} else
		coex->flags &= ~ATH6KL_HTCOEX_FLAGS_START;

#ifdef CFG80211_SAFE_BSS_INFO_ACCESS
	rcu_read_unlock();
#endif

	coex->num_scan = 0;
	coex->tolerant40_cnt = 0;
	coex->current_ratemask = ATH6KL_HTCOEX_RATEMASK_FULL;
	htcoex_flush_bss_info(coex);

	ath6kl_bss_put(vif->ar, bss);

	ath6kl_dbg(ATH6KL_DBG_HTCOEX,
		   "htcoex connect (vif %p) flags %x interval %d cycle %d\n",
		   vif,
		   coex->flags,
		   coex->scan_interval,
		   coex->rate_rollback_interval);

	return;
}

void ath6kl_htcoex_disconnect_event(struct ath6kl_vif *vif)
{
	struct htcoex *coex = vif->htcoex_ctx;

	if (vif->nw_type != INFRA_NETWORK)
		return;

	ath6kl_dbg(ATH6KL_DBG_HTCOEX,
		   "htcoex disconnect (vif %p) flags %x\n",
		   vif,
		   coex->flags);

	if (coex->flags & ATH6KL_HTCOEX_FLAGS_START) {
		del_timer(&coex->scan_timer);
		coex->flags &= ~ATH6KL_HTCOEX_FLAGS_START;

		/* Back to full rates */
		ath6kl_wmi_set_fix_rates(vif->ar->wmi, vif->fw_vif_idx,
					 ATH6KL_HTCOEX_RATEMASK_FULL);
		/*clear the scan channels*/
		htcoex_clear_scan_channels(vif);
	}

	htcoex_flush_bss_info(coex);

	return;
}

int ath6kl_htcoex_config(struct ath6kl_vif *vif, u32 interval, u8 rate_rollback)
{
	struct htcoex *coex = vif->htcoex_ctx;
	int restart = 0;

	coex->scan_interval = interval;

	if (coex->flags & ATH6KL_HTCOEX_FLAGS_START) {
		del_timer(&coex->scan_timer);
		htcoex_clear_scan_channels(vif);
		coex->flags &= ~ATH6KL_HTCOEX_FLAGS_START;
		restart = 1;
	}

	if (coex->scan_interval == 0)
		coex->flags &= ~ATH6KL_HTCOEX_FLAGS_ENABLED;
	else {
		if (coex->scan_interval < ATH6KL_HTCOEX_SCAN_PERIOD)
			coex->scan_interval = ATH6KL_HTCOEX_SCAN_PERIOD;

		coex->flags |= ATH6KL_HTCOEX_FLAGS_ENABLED;

		if (restart) {
			if (htcoex_set_scan_channels(vif) == 0) {
				mod_timer(&coex->scan_timer, jiffies +
					msecs_to_jiffies(coex->scan_interval));
				coex->flags |= ATH6KL_HTCOEX_FLAGS_START;
			} else {
				ath6kl_err("htcoex set scan channels failed\n");
				WARN_ON(1);
			}
		}

		coex->rate_rollback_interval = rate_rollback;
	}

	htcoex_flush_bss_info(coex);

	ath6kl_dbg(ATH6KL_DBG_HTCOEX,
		   "htcoex config (vif %p interval %d %s rate_rollback %d restart %d)\n",
		   vif,
		   coex->scan_interval,
		   (coex->flags & ATH6KL_HTCOEX_FLAGS_ENABLED) ? "ON" : "OFF",
		   coex->rate_rollback_interval,
		   restart);


	return 0;
}
