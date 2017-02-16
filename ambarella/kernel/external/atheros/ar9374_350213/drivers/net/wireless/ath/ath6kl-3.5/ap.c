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

static inline enum ap_keepalive_adjust __ap_keepalive_adjust_txrx_time(
	struct ath6kl_sta *conn, u16 last_txrx_time, unsigned long now)
{
	u32 diff_ms;
	enum ap_keepalive_adjust adjust_result = AP_KA_ADJ_ERROR;

	if (conn->last_txrx_time_tgt) {
		if (last_txrx_time >= conn->last_txrx_time_tgt)
			diff_ms = (last_txrx_time -
				   conn->last_txrx_time_tgt) << 10;
		else	/* wrap */
			diff_ms = (0xffff -
				   (conn->last_txrx_time_tgt - last_txrx_time))
				   << 10;

		/* Update to last one. */
		conn->last_txrx_time_tgt = last_txrx_time;
		conn->last_txrx_time += msecs_to_jiffies(diff_ms);

		if (conn->last_txrx_time > now)
			conn->last_txrx_time = now;

		adjust_result = AP_KA_ADJ_ADJUST;
	} else {
		/* First updated, treat as base time. */
		conn->last_txrx_time_tgt = last_txrx_time;
		conn->last_txrx_time = now;

		adjust_result = AP_KA_ADJ_BASESET;
	}

	return adjust_result;
}

static int _ap_keepalive_update_check_txrx_time(struct ath6kl_vif *vif,
						struct ath6kl_sta *conn,
						u16 last_txrx_time)
{
	struct ap_keepalive_info *ap_keepalive = vif->ap_keepalive_ctx;
	unsigned long now = jiffies;
	enum ap_keepalive_adjust adjust_result;
	int action = AP_KA_ACTION_NONE;

	spin_lock_bh(&conn->lock);
	adjust_result = __ap_keepalive_adjust_txrx_time(conn,
							last_txrx_time,
							now);
	if (adjust_result == AP_KA_ADJ_ADJUST) {
		if ((now - conn->last_txrx_time) >=
		    msecs_to_jiffies(ap_keepalive->ap_ka_interval))
			action = AP_KA_ACTION_POLL;

		if ((now - conn->last_txrx_time) >=
		    msecs_to_jiffies(ap_keepalive->ap_ka_remove_time))
			action = AP_KA_ACTION_REMOVE;
	}
	spin_unlock_bh(&conn->lock);

	ath6kl_dbg(ATH6KL_DBG_KEEPALIVE,
		   "ap_keepalive check (aid %d tgt/hst/now %d %ld %ld %s\n",
		   conn->aid,
		   conn->last_txrx_time_tgt,
		   conn->last_txrx_time,
		   now,
		   (action == AP_KA_ACTION_NONE) ? "NONE" :
			((action == AP_KA_ACTION_POLL) ? "POLL" : "REMOVE"));

	return action;
}

static int ap_keepalive_preload_txrx_time(struct ath6kl_vif *vif)
{
	struct ath6kl *ar = vif->ar;
	int ret = 0;

	/* Get AP stats only at least one station associated. */
	if (vif->sta_list_index)
		ret = ath6kl_wmi_get_stats_cmd(ar->wmi, vif->fw_vif_idx);

	return ret;
}

static int ap_keepalive_update_check_txrx_time(struct ath6kl_vif *vif)
{
#define _KICKOUT_CAUSE	(WLAN_REASON_DISASSOC_DUE_TO_INACTIVITY)
	struct ath6kl *ar = vif->ar;
	struct wmi_ap_mode_stat *ap_stats = &vif->ap_stats;
	struct wmi_per_sta_stat *per_sta_stat;
	struct ath6kl_sta *conn;
	int i, action;

	if (test_bit(STATS_UPDATE_PEND, &vif->flags)) {
		ath6kl_info("somebody still query now and ignore it.\n");
		return -EBUSY;
	}

	/* Now, tranfer to host time and update to vif->sta_list[]. */
	for (i = 0; i < AP_MAX_NUM_STA; i++) {
		per_sta_stat = &ap_stats->sta[i];
		if (per_sta_stat->aid) {
			conn = ath6kl_find_sta_by_aid(vif, per_sta_stat->aid);
			if (conn) {
				action = _ap_keepalive_update_check_txrx_time(
						vif,
						conn,
						per_sta_stat->last_txrx_time);

				if (action == AP_KA_ACTION_POLL) {
					ath6kl_wmi_ap_poll_sta(ar->wmi,
							       vif->fw_vif_idx,
							       conn->aid);
				} else if (action == AP_KA_ACTION_REMOVE) {
					ath6kl_wmi_ap_set_mlme(ar->wmi,
							       vif->fw_vif_idx,
							       WMI_AP_DEAUTH,
							       conn->mac,
							       _KICKOUT_CAUSE);
				}
			} else
				ath6kl_err("can't find this AID %d\n",
						per_sta_stat->aid);
		}
	}

	return 0;
#undef _KICKOUT_CAUSE
}

static void ap_keepalive_start(unsigned long arg)
{
	struct ap_keepalive_info *ap_keepalive =
		(struct ap_keepalive_info *) arg;
	struct ath6kl_vif *vif = ap_keepalive->vif;
	int ret;

	BUG_ON(!vif);

	ath6kl_dbg(ATH6KL_DBG_KEEPALIVE,
		   "ap_keepalive timer (vif idx %d) sta_list_index %x %s\n",
		   vif->fw_vif_idx,
		   vif->sta_list_index,
		   (ap_keepalive->flags & ATH6KL_AP_KA_FLAGS_PRELOAD_STAT) ?
		   "preload" : "update check");

	if ((vif->nw_type == AP_NETWORK) &&
	    test_bit(CONNECTED, &vif->flags)) {
		if (ap_keepalive->flags & ATH6KL_AP_KA_FLAGS_PRELOAD_STAT) {
			ret = ap_keepalive_preload_txrx_time(vif);
			if (ret)
				ath6kl_err("preload last_txrx_time fail\n");
		} else {
			/* Update and check last TXRX time each stations. */
			ret = ap_keepalive_update_check_txrx_time(vif);
			if (ret)
				ath6kl_err("updatecheck last_txrx_time fail\n");
		}

		if ((ap_keepalive->flags & ATH6KL_AP_KA_FLAGS_START) &&
		    (ap_keepalive->ap_ka_interval)) {
			if (ap_keepalive->flags &
				ATH6KL_AP_KA_FLAGS_PRELOAD_STAT) {
				mod_timer(&ap_keepalive->ap_ka_timer,
					  jiffies +
					  msecs_to_jiffies(
						ATH6KL_AP_KA_PRELOAD_LEADTIME));

				ap_keepalive->flags &=
					~ATH6KL_AP_KA_FLAGS_PRELOAD_STAT;
			} else {
				mod_timer(&ap_keepalive->ap_ka_timer,
					  jiffies +
					  msecs_to_jiffies(
						ap_keepalive->ap_ka_interval) -
					  msecs_to_jiffies(
						ATH6KL_AP_KA_PRELOAD_LEADTIME));

				ap_keepalive->flags |=
					ATH6KL_AP_KA_FLAGS_PRELOAD_STAT;
			}
		}
	}

	return;
}

struct ap_keepalive_info *ath6kl_ap_keepalive_init(struct ath6kl_vif *vif,
						   enum ap_keepalive_mode mode)
{
	struct ap_keepalive_info *ap_keepalive;

	ap_keepalive = kzalloc(sizeof(struct ap_keepalive_info), GFP_KERNEL);
	if (!ap_keepalive) {
		ath6kl_err("failed to alloc memory for ap_keepalive\n");
		return NULL;
	}

	ap_keepalive->vif = vif;
	ap_keepalive->ap_ka_interval = 0;
	ap_keepalive->ap_ka_reclaim_cycle = 0;
	if (mode == AP_KA_MODE_ENABLE) {
		ap_keepalive->flags |= ATH6KL_AP_KA_FLAGS_ENABLED;
		ap_keepalive->ap_ka_interval = ATH6KL_AP_KA_INTERVAL_DEFAULT;
		ap_keepalive->ap_ka_reclaim_cycle =
			ATH6KL_AP_KA_RECLAIM_CYCLE_DEFAULT;
	} else if (mode == AP_KA_MODE_BYSUPP)
		ap_keepalive->flags |= ATH6KL_AP_KA_FLAGS_BY_SUPP;

	ap_keepalive->ap_ka_remove_time = ap_keepalive->ap_ka_interval *
					  ap_keepalive->ap_ka_reclaim_cycle;

	/* Init. periodic scan timer. */
	init_timer(&ap_keepalive->ap_ka_timer);
	ap_keepalive->ap_ka_timer.function = ap_keepalive_start;
	ap_keepalive->ap_ka_timer.data = (unsigned long) ap_keepalive;

	ath6kl_dbg(ATH6KL_DBG_KEEPALIVE,
		   "ap_keepalive init (vif idx %d) interval %d cycle %d %s\n",
		   vif->fw_vif_idx,
		   ap_keepalive->ap_ka_interval,
		   ap_keepalive->ap_ka_reclaim_cycle,
		   (ap_keepalive->flags & ATH6KL_AP_KA_FLAGS_ENABLED) ? "ON" :
		    ((ap_keepalive->flags & ATH6KL_AP_KA_FLAGS_BY_SUPP) ?
			"SUPP" : "OFF"));

	return ap_keepalive;
}

void ath6kl_ap_keepalive_deinit(struct ath6kl_vif *vif)
{
	struct ap_keepalive_info *ap_keepalive = vif->ap_keepalive_ctx;

	if (ap_keepalive) {
		if (ap_keepalive->flags & ATH6KL_AP_KA_FLAGS_START)
			del_timer(&ap_keepalive->ap_ka_timer);

		kfree(ap_keepalive);
	}

	vif->ap_keepalive_ctx = NULL;

	ath6kl_dbg(ATH6KL_DBG_KEEPALIVE,
		   "ap_keepalive deinit (vif idx %d)\n",
		   vif->fw_vif_idx);

	return;
}

int ath6kl_ap_keepalive_start(struct ath6kl_vif *vif)
{
	struct ap_keepalive_info *ap_keepalive = vif->ap_keepalive_ctx;

	BUG_ON(!ap_keepalive);
	BUG_ON(vif->nw_type != AP_NETWORK);

	ath6kl_dbg(ATH6KL_DBG_KEEPALIVE,
		   "ap_keepalive start (vif idx %d) flags %x\n",
		   vif->fw_vif_idx,
		   ap_keepalive->flags);

	if (ap_keepalive->flags & ATH6KL_AP_KA_FLAGS_ENABLED) {
		mod_timer(&ap_keepalive->ap_ka_timer,
			  jiffies +
			  msecs_to_jiffies(ap_keepalive->ap_ka_interval) -
			  msecs_to_jiffies(ATH6KL_AP_KA_PRELOAD_LEADTIME));
		ap_keepalive->flags |= (ATH6KL_AP_KA_FLAGS_START |
					ATH6KL_AP_KA_FLAGS_PRELOAD_STAT);
	}

	return 0;
}

void ath6kl_ap_keepalive_stop(struct ath6kl_vif *vif)
{
	struct ap_keepalive_info *ap_keepalive = vif->ap_keepalive_ctx;

	if (!ap_keepalive)
		return;

	ath6kl_dbg(ATH6KL_DBG_KEEPALIVE,
		   "ap_keepalive stop (vif idx %d) flags %x\n",
		   vif->fw_vif_idx,
		   ap_keepalive->flags);

	if (ap_keepalive->flags & ATH6KL_AP_KA_FLAGS_START) {
		del_timer(&ap_keepalive->ap_ka_timer);
		ap_keepalive->flags &= ~(ATH6KL_AP_KA_FLAGS_START |
					 ATH6KL_AP_KA_FLAGS_PRELOAD_STAT);
	}

	return;
}

int ath6kl_ap_keepalive_config(struct ath6kl_vif *vif,
			       u32 ap_ka_interval,
			       u32 ap_ka_reclaim_cycle)
{
	struct ap_keepalive_info *ap_keepalive = vif->ap_keepalive_ctx;
	int restart = 0;

	if (ap_keepalive->flags & ATH6KL_AP_KA_FLAGS_BY_SUPP) {
		ath6kl_dbg(ATH6KL_DBG_INFO,
			   "already offlad to supplicant/hostapd.\n");
		return 0;
	}

	if ((ap_ka_interval != 0) &&
	    (ap_ka_interval < ATH6KL_AP_KA_INTERVAL_MIN))
		ap_ka_interval = ATH6KL_AP_KA_INTERVAL_MIN;

	if (ap_ka_reclaim_cycle == 0)
		ap_ka_reclaim_cycle = 1;

	if (ap_keepalive->flags & ATH6KL_AP_KA_FLAGS_START) {
		del_timer(&ap_keepalive->ap_ka_timer);
		ap_keepalive->flags &= ~(ATH6KL_AP_KA_FLAGS_START |
					 ATH6KL_AP_KA_FLAGS_PRELOAD_STAT);
		restart = 1;
	}

	if (ap_ka_interval == 0) {
		ap_keepalive->ap_ka_interval = 0;
		ap_keepalive->ap_ka_reclaim_cycle = 0;
		ap_keepalive->flags &= ~ATH6KL_AP_KA_FLAGS_ENABLED;
	} else {
		if (ap_ka_interval * ap_ka_reclaim_cycle <
			ATH6KL_AP_KA_RECLAIM_TIME_MAX) {
			ap_keepalive->ap_ka_interval = ap_ka_interval;
			ap_keepalive->ap_ka_reclaim_cycle = ap_ka_reclaim_cycle;
		} else {
			ap_keepalive->ap_ka_interval =
				ATH6KL_AP_KA_INTERVAL_DEFAULT;
			ap_keepalive->ap_ka_reclaim_cycle =
				ATH6KL_AP_KA_RECLAIM_CYCLE_DEFAULT;
		}

		ap_keepalive->ap_ka_remove_time =
				ap_keepalive->ap_ka_interval *
				ap_keepalive->ap_ka_reclaim_cycle;
		ap_keepalive->flags |= ATH6KL_AP_KA_FLAGS_ENABLED;

		if (restart) {
			mod_timer(&ap_keepalive->ap_ka_timer,
				  jiffies +
				  msecs_to_jiffies(
					ap_keepalive->ap_ka_interval) -
				  msecs_to_jiffies(
					ATH6KL_AP_KA_PRELOAD_LEADTIME * 1000));

			ap_keepalive->flags |=
					(ATH6KL_AP_KA_FLAGS_START |
					 ATH6KL_AP_KA_FLAGS_PRELOAD_STAT);
		}
	}

	ath6kl_dbg(ATH6KL_DBG_KEEPALIVE,
		   "ap_keepalive config (%d intvl %d cycle %d %s restart %d)\n",
		   vif->fw_vif_idx,
		   ap_keepalive->ap_ka_interval,
		   ap_keepalive->ap_ka_reclaim_cycle,
		   (ap_keepalive->flags & ATH6KL_AP_KA_FLAGS_ENABLED) ?
			"ON" : "OFF",
		   restart);


	return 0;
}

/* Offload to supplicant/hostapd */
static u32 ap_keepalive_get_inactive_time(struct ath6kl_vif *vif,
					  struct ath6kl_sta *conn)
{
	struct wmi_ap_mode_stat *ap_stats = &vif->ap_stats;
	u32 inact_time = 0;
	int i;

	for (i = 0; i < AP_MAX_NUM_STA; i++) {
		struct wmi_per_sta_stat *per_sta_stat = &ap_stats->sta[i];

		if (per_sta_stat->aid == conn->aid) {
			unsigned long now = jiffies;

			spin_lock_bh(&conn->lock);
			__ap_keepalive_adjust_txrx_time(conn,
						per_sta_stat->last_txrx_time,
						now);

			/* get inactive time. */
			inact_time = jiffies_to_msecs(now -
						      conn->last_txrx_time);
			spin_unlock_bh(&conn->lock);

			ath6kl_dbg(ATH6KL_DBG_KEEPALIVE,
			    "ap_keepalive inact tgt/hst/now %d %ld %ld\n",
			    conn->last_txrx_time_tgt,
			    conn->last_txrx_time,
			    now);
		}
	}

	return inact_time;
}

u32 ath6kl_ap_keepalive_get_inactive_time(struct ath6kl_vif *vif, u8 *mac)
{
	struct ath6kl_sta *conn;
	u32 inact_time;

	conn = ath6kl_find_sta(vif, mac);

	if (conn)
		inact_time = ap_keepalive_get_inactive_time(vif, conn);
	else {
		inact_time = 0;		/* return -1 ? */

		ath6kl_err("can't find %02x:%02x:%02x:%02x:%02x:%02x vif %d\n",
			mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
			vif->fw_vif_idx);
	}

	ath6kl_dbg(ATH6KL_DBG_KEEPALIVE,
		   "ap_keepalive inact aid %d inact_time %d ms\n",
		   (conn ? (conn->aid) : 0),
		   inact_time);

	return inact_time;
}

bool ath6kl_ap_keepalive_by_supp(struct ath6kl_vif *vif)
{
	struct ap_keepalive_info *ap_keepalive = vif->ap_keepalive_ctx;

	if (!ap_keepalive)
		return false;

	if (ap_keepalive->flags & ATH6KL_AP_KA_FLAGS_BY_SUPP)
		return true;
	else
		return false;
}

struct ap_acl_info *ath6kl_ap_acl_init(struct ath6kl_vif *vif)
{
	struct ap_acl_info *ap_acl;

	ap_acl = kzalloc(sizeof(struct ap_acl_info), GFP_KERNEL);
	if (!ap_acl) {
		ath6kl_err("failed to alloc memory for ap_acl\n");
		return NULL;
	}

	ap_acl->vif = vif;

	/* Default is OFF and configure it from debugfs or others */
	ap_acl->acl_mode = AP_ACL_MODE_DISABLE;

	ath6kl_dbg(ATH6KL_DBG_ACL,
			"ap_acl init (vif idx %d)\n",
			vif->fw_vif_idx);

	return ap_acl;
}

void ath6kl_ap_acl_deinit(struct ath6kl_vif *vif)
{
	struct ap_acl_info *ap_acl = vif->ap_acl_ctx;

	if (ap_acl != NULL) {
		if (ap_acl->last_acl_config != NULL)
			kfree(ap_acl->last_acl_config);
		kfree(ap_acl);
	}

	vif->ap_acl_ctx = NULL;

	ath6kl_dbg(ATH6KL_DBG_ACL,
			"ap_acl deinit (vif idx %d)\n",
			vif->fw_vif_idx);

	return;
}

int ath6kl_ap_acl_start(struct ath6kl_vif *vif)
{
	struct ap_acl_info *ap_acl = vif->ap_acl_ctx;

	if (ap_acl) {
		if (ap_acl->last_acl_config != NULL)
			kfree(ap_acl->last_acl_config);

		ap_acl->last_acl_config = NULL;
	}

	ath6kl_dbg(ATH6KL_DBG_ACL,
			"ap_acl start (vif idx %d)\n",
			vif->fw_vif_idx);

	return 0;
}

int ath6kl_ap_acl_stop(struct ath6kl_vif *vif)
{
	struct ap_acl_info *ap_acl = vif->ap_acl_ctx;

	if (ap_acl) {
		ath6kl_ap_acl_config_mac_list_reset(vif);
		ath6kl_ap_acl_config_policy(vif, AP_ACL_MODE_DISABLE);

		if (ap_acl->last_acl_config != NULL)
			kfree(ap_acl->last_acl_config);

		ap_acl->last_acl_config = NULL;
	}

	ath6kl_dbg(ATH6KL_DBG_ACL,
			"ap_acl stop (vif idx %d)\n",
			vif->fw_vif_idx);

	return 0;
}

int ath6kl_ap_acl_config_policy(struct ath6kl_vif *vif,
				enum ap_acl_mode mode)
{
	struct ap_acl_info *ap_acl = vif->ap_acl_ctx;
	struct ap_acl_entry *ap_acl_entry;
	u8 i, policy;

	if (ap_acl == NULL)
		return 0;

	switch (mode) {
	case AP_ACL_MODE_DISABLE:
		policy = AP_ACL_DISABLE;
		break;
	case AP_ACL_MODE_ALLOW:
		policy = AP_ACL_ALLOW_MAC;
		break;
	case AP_ACL_MODE_DENY:
		policy = AP_ACL_DENY_MAC;
		break;
	default:
		BUG_ON(1);
	}

	ap_acl->acl_mode = mode;
	ath6kl_wmi_ap_acl_policy(vif->ar->wmi, vif->fw_vif_idx, policy);

	ath6kl_dbg(ATH6KL_DBG_ACL,
		"ap_acl config (vif idx %d mode %s)\n",
		vif->fw_vif_idx,
		(ap_acl->acl_mode ?
			(ap_acl->acl_mode == AP_ACL_MODE_ALLOW ?
				"ALLOW" : "DENY") :
			"DISABLED"));

	if (ap_acl->acl_mode != AP_ACL_MODE_DISABLE) {
		for (i = 0; i < ATH6KL_AP_ACL_MAX_NUM; i++) {
			ap_acl_entry = &ap_acl->acl_list[i];

			if (ap_acl_entry->flags & ATH6KL_AP_ACL_FLAGS_USED) {
				ath6kl_wmi_ap_acl_mac_list(vif->ar->wmi,
						vif->fw_vif_idx,
						i,
						ap_acl_entry->mac_addr,
						ADD_MAC_ADDR);
				ath6kl_dbg(ATH6KL_DBG_ACL,
					"ap_acl config ADD "
					"%02x:%02x:%02x:%02x:%02x:%02x\n",
					ap_acl_entry->mac_addr[0],
					ap_acl_entry->mac_addr[1],
					ap_acl_entry->mac_addr[2],
					ap_acl_entry->mac_addr[3],
					ap_acl_entry->mac_addr[4],
					ap_acl_entry->mac_addr[5]);
			}
		}
	}

	return 0;
}

int ath6kl_ap_acl_config_mac_list(struct ath6kl_vif *vif,
				u8 *mac_addr, bool removed)
{
	struct ap_acl_info *ap_acl = vif->ap_acl_ctx;
	struct ap_acl_entry *ap_acl_entry;
	int i, slot_idx, first_not_used = 0xff;
	u8 action;

	if (ap_acl == NULL)
		return 0;

	ath6kl_dbg(ATH6KL_DBG_ACL,
		"ap_acl config %02x:%02x:%02x:%02x:%02x:%02x ",
		mac_addr[0], mac_addr[1], mac_addr[2],
		mac_addr[3], mac_addr[4], mac_addr[5]);

	for (i = 0; i < ATH6KL_AP_ACL_MAX_NUM; i++) {
		ap_acl_entry = &ap_acl->acl_list[i];
		if (ap_acl_entry->flags & ATH6KL_AP_ACL_FLAGS_USED) {
			if (memcmp(ap_acl_entry->mac_addr,
					mac_addr,
					ETH_ALEN) == 0) {
				ath6kl_dbg(ATH6KL_DBG_ACL,
					"found in slot %d\n",
					i);
				break;
			}
		} else if (first_not_used == 0xff)
			first_not_used = i;
	}

	if (removed) {
		action = DEL_MAC_ADDR;
		slot_idx = i;

		if (i == ATH6KL_AP_ACL_MAX_NUM) {
			ath6kl_dbg(ATH6KL_DBG_ACL,
				"not found and ignore it.\n");

			return 0;
		}

		ap_acl_entry = &ap_acl->acl_list[slot_idx];
		ap_acl_entry->flags &= ~ATH6KL_AP_ACL_FLAGS_USED;
	} else {
		action = ADD_MAC_ADDR;
		slot_idx = first_not_used;

		if (i != ATH6KL_AP_ACL_MAX_NUM)
			return 0;

		if (first_not_used == 0xff) {
			ath6kl_dbg(ATH6KL_DBG_ACL,
				"no availabe ACL slot!\n");

			return 0;
		}

		ap_acl_entry = &ap_acl->acl_list[slot_idx];
		ap_acl_entry->flags |= ATH6KL_AP_ACL_FLAGS_USED;
		memcpy(ap_acl_entry->mac_addr, mac_addr, ETH_ALEN);
	}

	if (ap_acl->acl_mode != AP_ACL_MODE_DISABLE) {
		ath6kl_wmi_ap_acl_mac_list(vif->ar->wmi,
					vif->fw_vif_idx,
					slot_idx,
					ap_acl_entry->mac_addr,
					action);
	}

	ath6kl_dbg(ATH6KL_DBG_ACL,
		"ap_acl config %s %02x:%02x:%02x:%02x:%02x:%02x, slot %d\n",
		((removed) ? "DEL" : "ADD"),
		mac_addr[0], mac_addr[1], mac_addr[2],
		mac_addr[3], mac_addr[4], mac_addr[5],
		slot_idx);

	return 0;
}

int ath6kl_ap_acl_config_mac_list_reset(struct ath6kl_vif *vif)
{
	struct ap_acl_info *ap_acl = vif->ap_acl_ctx;
	struct ap_acl_entry *ap_acl_entry;
	int i;

	if (ap_acl == NULL)
		return 0;

	ath6kl_dbg(ATH6KL_DBG_ACL, "ap_acl reset\n");

	for (i = 0; i < ATH6KL_AP_ACL_MAX_NUM; i++) {
		ap_acl_entry = &ap_acl->acl_list[i];
		if (ap_acl_entry->flags & ATH6KL_AP_ACL_FLAGS_USED)
			ath6kl_ap_acl_config_mac_list(vif,
						ap_acl_entry->mac_addr,
						true);
	}

	return 0;
}

int ath6kl_ap_acl_dump(struct ath6kl *ar, u8 *buf, int buf_len)
{
	int i, len = 0;

	if (!buf)
		return 0;

	for (i = 0; i < ar->vif_max; i++) {
		struct ath6kl_vif *vif;

		vif = ath6kl_get_vif_by_index(ar, i);
		if ((vif) &&
		    (vif->nw_type == AP_NETWORK)) {
			struct ap_acl_info *ap_acl;

			len += snprintf(buf + len, buf_len - len,
					"\nVAP%d - ",
					vif->fw_vif_idx);

			ap_acl = vif->ap_acl_ctx;
			if (!ap_acl)
				return 0;

			len += snprintf(buf + len, buf_len - len, "mode %d\n",
					ap_acl->acl_mode);

			for (i = 0; i < ATH6KL_AP_ACL_MAX_NUM; i++) {
				struct ap_acl_entry *ap_acl_entry;

				ap_acl_entry = &ap_acl->acl_list[i];

				len += snprintf(buf + len, buf_len - len,
					" %02d/%08x - "
					"%02x:%02x:%02x:%02x:%02x:%02x\n",
					i,
					ap_acl_entry->flags,
					ap_acl_entry->mac_addr[0],
					ap_acl_entry->mac_addr[1],
					ap_acl_entry->mac_addr[2],
					ap_acl_entry->mac_addr[3],
					ap_acl_entry->mac_addr[4],
					ap_acl_entry->mac_addr[5]);
			}
		}
	}

	return len;
}

