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
#include "hif-ops.h"

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
		if (time_after_eq(now, conn->last_txrx_time + msecs_to_jiffies(ap_keepalive->ap_ka_interval)))
			action = AP_KA_ACTION_POLL;

		if (time_after_eq(now, conn->last_txrx_time + msecs_to_jiffies(ap_keepalive->ap_ka_remove_time)))
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
	if ((mode == AP_KA_MODE_ENABLE) ||
	    (mode == AP_KA_MODE_CONFIG_BYSUPP)) {
		ap_keepalive->flags |= ATH6KL_AP_KA_FLAGS_ENABLED;
		ap_keepalive->ap_ka_interval = ATH6KL_AP_KA_INTERVAL_DEFAULT;
		ap_keepalive->ap_ka_reclaim_cycle = ATH6KL_AP_KA_RECLAIM_CYCLE;

		if (mode == AP_KA_MODE_CONFIG_BYSUPP)
			ap_keepalive->flags |=
					ATH6KL_AP_KA_FLAGS_CONFIG_BY_SUPP;
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
		   (ap_keepalive->flags & ATH6KL_AP_KA_FLAGS_ENABLED) ?
		    ((ap_keepalive->flags & ATH6KL_AP_KA_FLAGS_CONFIG_BY_SUPP) ?
			"ON-SUPP" : "ON") :
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
		ath6kl_wmi_set_inact_cmd(vif->ar->wmi,
			DISALBE_AP_INACTIVE_TIMEMER);
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
		ath6kl_dbg(ATH6KL_DBG_KEEPALIVE,
			   "ap_keepalive offlad to supplicant/hostapd.\n");
		return 0;
	} else if (!(ap_keepalive->flags & ATH6KL_AP_KA_FLAGS_ENABLED)) {
		return 0;
	} else if (ap_keepalive->flags & ATH6KL_AP_KA_FLAGS_CONFIG_BY_SUPP) {
		ath6kl_dbg(ATH6KL_DBG_KEEPALIVE,
		   "ap_keepalive config offlad to supplicant/hostapd.\n");
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
				ATH6KL_AP_KA_RECLAIM_CYCLE;
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

int ath6kl_ap_keepalive_config_by_supp(struct ath6kl_vif *vif,
			       u16 inactive_time)
{
	struct ap_keepalive_info *ap_keepalive = vif->ap_keepalive_ctx;
	u32 ap_ka_interval;
	u32 timeout = inactive_time * 1000;	/* to ms. */

	if (!(ap_keepalive->flags & ATH6KL_AP_KA_FLAGS_ENABLED) ||
	    !(ap_keepalive->flags & ATH6KL_AP_KA_FLAGS_CONFIG_BY_SUPP))
		return 0;

	if (timeout == 0) {
		ath6kl_dbg(ATH6KL_DBG_KEEPALIVE,
			   "ap_keepalive config wrong inactive_time!\n");
		return 0;
	}

	/* Min. at lease 1 cycle */
	if (timeout < (ATH6KL_AP_KA_INTERVAL_MIN * 1))
		timeout = (ATH6KL_AP_KA_INTERVAL_MIN * 1);
	else if (timeout > ATH6KL_AP_KA_RECLAIM_TIME_MAX)
		timeout = ATH6KL_AP_KA_RECLAIM_TIME_MAX;

	if (timeout < (ATH6KL_AP_KA_INTERVAL_DEFAULT *
				ATH6KL_AP_KA_RECLAIM_CYCLE)) {
		if (timeout <= ATH6KL_AP_KA_INTERVAL_DEFAULT)
			ap_ka_interval = ATH6KL_AP_KA_INTERVAL_MIN;
		else
			ap_ka_interval = ATH6KL_AP_KA_INTERVAL_DEFAULT;
	} else
		ap_ka_interval = ATH6KL_AP_KA_INTERVAL_DEFAULT;

	/* Update the config */
	ap_keepalive->ap_ka_interval = ap_ka_interval;
	ap_keepalive->ap_ka_reclaim_cycle = timeout / ap_ka_interval;
	ap_keepalive->ap_ka_remove_time =
			ap_ka_interval * ap_keepalive->ap_ka_reclaim_cycle;

	ath6kl_dbg(ATH6KL_DBG_KEEPALIVE,
		   "ap_keepalive config_by_supp (%d supp %d intvl %d cycle %d)\n",
		   vif->fw_vif_idx,
		   inactive_time,
		   ap_keepalive->ap_ka_interval,
		   ap_keepalive->ap_ka_reclaim_cycle);

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

static inline void _ap_amdc_assoc_req_free(struct ap_admc_info *ap_admc,
	struct ap_admc_assoc_req *admc_assoc_req)
{
	if (admc_assoc_req) {
		/*
		 * TODO: Check admc_assoc_req->action then do something.
		 */
		kfree(admc_assoc_req);
	} else {
		/*
		 * This STA reject by Admission-Control check rule
		 * or firmware's frame parser.
		 */
		;
	}

	return;
}

static void _ap_amdc_assoc_req_timeout(unsigned long arg)
{
	struct ap_admc_assoc_req *admc_assoc_req;
	struct ap_admc_info *ap_admc;

	admc_assoc_req = (struct ap_admc_assoc_req *)arg;
	ap_admc = admc_assoc_req->ap_admc;

	ath6kl_dbg(ATH6KL_DBG_ADMC,
			"ap_admc timeout assoResp %p len %d depth %d\n",
			admc_assoc_req,
			admc_assoc_req->frame_len,
			ap_admc->assoc_req_cnt);

	admc_assoc_req->action = AP_ADMC_ACT_TIMEOUT;
	list_del(&admc_assoc_req->list);
	ap_admc->assoc_req_cnt--;

	_ap_amdc_assoc_req_free(ap_admc, admc_assoc_req);

	return;
}

static inline void _ap_amdc_assoc_req_add(struct ap_admc_info *ap_admc,
	u8 *frm,
	u16 len)
{
	struct ieee80211_mgmt *assocReq;
	struct ap_admc_assoc_req *admc_assoc_req;

	if (len > ATH6KL_AP_ADMC_ASSOC_REQ_MAX_LEN) {
		/* Is it possible? */
		ath6kl_err("Not support so large assoc_req frame\n");
		return;
	}

	admc_assoc_req = kzalloc(sizeof(struct ap_admc_assoc_req), GFP_KERNEL);
	if (!admc_assoc_req) {
		ath6kl_err("failed to alloc memory for admc_assoc_req\n");
		return;
	}

	if (ap_admc->assoc_req_cnt)
		ath6kl_info("Last assoc_req not yet finish!");

	/* Add assocReq info. */
	admc_assoc_req->ap_admc = ap_admc;
	admc_assoc_req->frame_len = len;
	memcpy(admc_assoc_req->raw_frame, frm, len);
	assocReq = (struct ieee80211_mgmt *)(admc_assoc_req->raw_frame);
	admc_assoc_req->sta_mac = assocReq->sa;
	admc_assoc_req->action = AP_ADMC_ACT_ACCPET;

	/*
	 * It still possbile that the host accept the STA but something wrong
	 * when sending assocResp. In this case, the target will not send
	 * connection event and need a timer to reclaim this item.
	 */
	init_timer(&admc_assoc_req->reclaim_timer);
	admc_assoc_req->reclaim_timer.function = _ap_amdc_assoc_req_timeout;
	admc_assoc_req->reclaim_timer.data = (unsigned long) admc_assoc_req;
	mod_timer(&admc_assoc_req->reclaim_timer,
		jiffies + msecs_to_jiffies(ap_admc->assoc_req_timeout));

	spin_lock_bh(&ap_admc->assoc_req_lock);
	list_add_tail(&admc_assoc_req->list, &ap_admc->assoc_req_list);
	ap_admc->assoc_req_cnt++;
	spin_unlock_bh(&ap_admc->assoc_req_lock);

	ath6kl_dbg(ATH6KL_DBG_ADMC,
			"ap_admc add assoResp len %d depth %d\n",
			len,
			ap_admc->assoc_req_cnt);

	return;
}

static inline struct ap_admc_assoc_req *_ap_amdc_assoc_req_search(
	struct ap_admc_info *ap_admc,
	u8 *sta_mac)
{
	struct ap_admc_assoc_req *admc_assoc_req, *tmp;

	/*
	 * Here assume the first matched address is the STA we want
	 * to handle if more than one the same addresses in the list.
	 */
	admc_assoc_req = NULL;
	if (!list_empty(&ap_admc->assoc_req_list)) {
		list_for_each_entry_safe(admc_assoc_req,
					tmp,
					&ap_admc->assoc_req_list,
					list) {
			if (memcmp(admc_assoc_req->sta_mac,
					sta_mac,
					ETH_ALEN) == 0)
				break;
		}
	}

	if (admc_assoc_req) {
		spin_lock_bh(&ap_admc->assoc_req_lock);
		del_timer(&admc_assoc_req->reclaim_timer);
		list_del(&admc_assoc_req->list);
		ap_admc->assoc_req_cnt--;
		spin_unlock_bh(&ap_admc->assoc_req_lock);
	}

	return admc_assoc_req;
}

static inline void _ap_amdc_assoc_req_flush(struct ap_admc_info *ap_admc)
{
	struct ap_admc_assoc_req *admc_assoc_req, *tmp;
	int freed = 0;

	spin_lock_bh(&ap_admc->assoc_req_lock);
	list_for_each_entry_safe(admc_assoc_req,
				tmp,
				&ap_admc->assoc_req_list,
				list) {
		admc_assoc_req->action = AP_ADMC_ACT_FLUSH;
		del_timer(&admc_assoc_req->reclaim_timer);
		list_del(&admc_assoc_req->list);
		ap_admc->assoc_req_cnt--;

		_ap_amdc_assoc_req_free(ap_admc, admc_assoc_req);
		freed++;
	}
	spin_unlock_bh(&ap_admc->assoc_req_lock);

	WARN_ON(ap_admc->assoc_req_cnt);

	ath6kl_dbg(ATH6KL_DBG_ADMC,
			"ap_admc flush %d\n",
			freed);

	return;
}

static bool _ap_amdc_check(struct ap_admc_info *ap_admc,
	u8 *frm,
	u8 req_type,
	u8 **sta_mac,
	u8 *reason_code)
{
	struct ieee80211_mgmt *assocReq = (struct ieee80211_mgmt *)frm;
	bool accept = true;

	*reason_code = WLAN_STATUS_SUCCESS;

	if (ap_admc->admc_mode == AP_ADMC_MODE_ACCEPT_ALWAYS) {
		/* Assume the target already check DA & BSSID already. */
		*sta_mac = assocReq->sa;

		ath6kl_dbg(ATH6KL_DBG_ADMC,
			"ap_admc check %02x:%02x:%02x:%02x:%02x:%02x\n",
			assocReq->sa[0], assocReq->sa[1], assocReq->sa[2],
			assocReq->sa[3], assocReq->sa[4], assocReq->sa[5]);
	} else {
		; /* TODO */
	}

	return accept;
}

struct ap_admc_info *ath6kl_ap_admc_init(struct ath6kl_vif *vif,
	enum ap_admc_mode mode)
{
	struct ap_admc_info *ap_admc;

	ap_admc = kzalloc(sizeof(struct ap_admc_info), GFP_KERNEL);
	if (!ap_admc) {
		ath6kl_err("failed to alloc memory for ap_admc\n");
		return NULL;
	}

	ap_admc->vif = vif;
	ap_admc->admc_mode = mode;
	ap_admc->assoc_req_timeout = ATH6KL_AP_ADMC_ASSOC_REQ_TIMEOUT;
	spin_lock_init(&ap_admc->assoc_req_lock);
	INIT_LIST_HEAD(&ap_admc->assoc_req_list);

	ath6kl_dbg(ATH6KL_DBG_ADMC,
			"ap_admc init (vif%d) mode %d timeout %d\n",
			vif->fw_vif_idx,
			ap_admc->admc_mode,
			ap_admc->assoc_req_timeout);

	return ap_admc;
}

void ath6kl_ap_admc_deinit(struct ath6kl_vif *vif)
{
	struct ap_admc_info *ap_admc = vif->ap_admc_ctx;

	if (ap_admc != NULL) {
		_ap_amdc_assoc_req_flush(ap_admc);
		kfree(ap_admc);
	}

	vif->ap_admc_ctx = NULL;

	ath6kl_dbg(ATH6KL_DBG_ADMC,
			"ap_admc deinit (vif%d)\n",
			vif->fw_vif_idx);

	return;
}

int ath6kl_ap_admc_start(struct ath6kl_vif *vif)
{
	struct ap_admc_info *ap_admc = vif->ap_admc_ctx;
	bool enabled = false;
	int ret;

	if (!ap_admc)
		return -ENOENT;

	if (ap_admc->admc_mode != AP_ADMC_MODE_DISABLE)
		enabled = true;

	ret = ath6kl_wmi_set_assoc_req_relay_cmd(vif->ar->wmi,
					vif->fw_vif_idx,
					enabled);

	ath6kl_dbg(ATH6KL_DBG_ADMC,
			"ap_admc start %s (vif%d) ret %d\n",
			(enabled ? "enable" : "disable"),
			vif->fw_vif_idx,
			ret);

	return ret;
}

int ath6kl_ap_admc_stop(struct ath6kl_vif *vif)
{
	struct ap_admc_info *ap_admc = vif->ap_admc_ctx;
	int ret;

	if (!ap_admc)
		return -ENOENT;

	_ap_amdc_assoc_req_flush(ap_admc);

	ret = ath6kl_wmi_set_assoc_req_relay_cmd(vif->ar->wmi,
					vif->fw_vif_idx,
					false);
	ath6kl_dbg(ATH6KL_DBG_ADMC,
			"ap_admc stop (vif%d) ret %d\n",
			vif->fw_vif_idx,
			ret);

	return ret;
}

void ath6kl_ap_admc_assoc_req(struct ath6kl_vif *vif,
	u8 *assocReq,
	u16 assocReq_len,
	u8 req_type,
	u8 fw_status)
{
	struct ap_admc_info *ap_admc = vif->ap_admc_ctx;
	u8 reason_code, *sta_mac = NULL;
	bool accept;

	BUG_ON(!ap_admc);
	BUG_ON(ap_admc->admc_mode == AP_ADMC_MODE_DISABLE);

	if (vif->nw_type == AP_NETWORK) {
		accept = _ap_amdc_check(ap_admc,
					assocReq,
					req_type,
					&sta_mac,
					&reason_code);
		if (ath6kl_wmi_send_assoc_resp_cmd(vif->ar->wmi,
						vif->fw_vif_idx,
						accept,
						reason_code,
						fw_status,
						sta_mac,
						req_type))
			ath6kl_err("ap_admc assoResp fail (vif%d)\n",
					vif->fw_vif_idx);

		/* Only add into list for successful case. */
		if ((fw_status == WLAN_STATUS_SUCCESS) &&
		    accept)
			_ap_amdc_assoc_req_add(ap_admc,
						assocReq,
						assocReq_len);
		else
			_ap_amdc_assoc_req_free(ap_admc, NULL);
	} else
		WARN_ON(1);

	return;
}

void ath6kl_ap_admc_assoc_req_fetch(struct ath6kl_vif *vif,
	struct wmi_connect_event *ev,
	u8 **assocReq,
	u16 *assocReq_len)
{
	struct ap_admc_info *ap_admc = vif->ap_admc_ctx;
	struct ap_admc_assoc_req *admc_assoc_req = NULL;
	u8 *sta_mac = ev->u.ap_sta.mac_addr;

	BUG_ON(!ap_admc);

	*assocReq = NULL;
	*assocReq_len = 0;

	if (ap_admc->admc_mode == AP_ADMC_MODE_DISABLE) {
		*assocReq = ev->assoc_info + ev->beacon_ie_len;
		*assocReq_len = ev->assoc_req_len;

		return;
	}

	admc_assoc_req = _ap_amdc_assoc_req_search(ap_admc, sta_mac);
	if (admc_assoc_req) {
		*assocReq = admc_assoc_req->raw_frame;
		*assocReq_len = admc_assoc_req->frame_len;
	} else
		ath6kl_err("No asoc_req found!");

	ath6kl_dbg(ATH6KL_DBG_ADMC,
			"ap_admc assoResp fetch %p %p len %d\n",
			*assocReq,
			admc_assoc_req,
			*assocReq_len);

	return;
}

void ath6kl_ap_admc_assoc_req_release(struct ath6kl_vif *vif,
	u8 *assocReq)
{
	struct ap_admc_info *ap_admc = vif->ap_admc_ctx;
	struct ap_admc_assoc_req *admc_assoc_req;

	BUG_ON(!ap_admc);

	if (ap_admc->admc_mode == AP_ADMC_MODE_DISABLE)
		return;

	admc_assoc_req = container_of(assocReq,
					struct ap_admc_assoc_req,
					raw_frame[0]);

	ath6kl_dbg(ATH6KL_DBG_ADMC,
			"ap_admc assoResp release %p %p\n",
			assocReq,
			admc_assoc_req);

	_ap_amdc_assoc_req_free(ap_admc, admc_assoc_req);

	return;
}

int ath6kl_ap_admc_dump(struct ath6kl *ar, u8 *buf, int buf_len)
{
	int i, len = 0;

	if (!buf)
		return 0;

	for (i = 0; i < ar->vif_max; i++) {
		struct ath6kl_vif *vif;

		vif = ath6kl_get_vif_by_index(ar, i);
		if ((vif) &&
		    (vif->nw_type == AP_NETWORK)) {
			struct ap_admc_info *ap_admc;

			len += snprintf(buf + len, buf_len - len,
					"\nVAP%d - ",
					vif->fw_vif_idx);

			ap_admc = vif->ap_admc_ctx;
			if (!ap_admc)
				return 0;

			len += snprintf(buf + len, buf_len - len,
					"mode %d timeout %d depth %d\n",
					ap_admc->admc_mode,
					ap_admc->assoc_req_timeout,
					ap_admc->assoc_req_cnt);
		}
	}

	return len;
}

void ath6kl_ap_rc_init(struct ath6kl_vif *vif)
{
	struct ap_rc_info *ap_rc = &vif->ap_rc_info_ctx;

	ap_rc->mode = AP_RC_MODE_DISABLE;
	ap_rc->chan = 0;
	ap_rc->chan_fixed = 0;

	ath6kl_dbg(ATH6KL_DBG_AP_RC,
			"ap_rc init mode %d\n",
			ap_rc->mode);

	return;
}

u16 ath6kl_ap_rc_get(struct ath6kl_vif *vif, u16 chan_config)
{
	struct ap_rc_info *ap_rc = &vif->ap_rc_info_ctx;
	enum ap_rc_mode rc_mode = ap_rc->mode;
	u16 chan_rc = chan_config;

	/*
	 * Only support AP recommend channel when
	 * 1.ATH6KL_MODULE_ENABLE_P2P_CHANMODE not support.
	 * 2.in pure SoftAP mode.
	 */
	if ((vif->next_chan_type == ATH6KL_CHAN_TYPE_NONE) &&
	    (vif->wdev.iftype != NL80211_IFTYPE_P2P_GO)) {
		struct ath6kl_rc_report rc_report;
		int ret = -1;

		memset(&rc_report, 0, sizeof(struct ath6kl_rc_report));
		if (rc_mode != AP_RC_MODE_DISABLE)
			ret = ath6kl_p2p_rc_get(vif->ar, &rc_report);

		if (ret == 0) {
			if (rc_mode == AP_RC_MODE_FIXED)
				chan_rc = ap_rc->chan_fixed;
			else if (rc_mode == AP_RC_MODE_2GALL)
				chan_rc = rc_report.rc_2g;
			else if (rc_mode == AP_RC_MODE_2GPOP)
				chan_rc = rc_report.rc_p2p_so;
			else if (rc_mode == AP_RC_MODE_5GALL)
				chan_rc = rc_report.rc_5g;
			else if (rc_mode == AP_RC_MODE_OVERALL)
				chan_rc = rc_report.rc_all;
			else if (rc_mode == AP_RC_MODE_2GNOLTE)
				chan_rc = rc_report.rc_2g_nolte;
			else if (rc_mode == AP_RC_MODE_5GNODFS)
				chan_rc = rc_report.rc_5g_nodfs;
			else if (rc_mode == AP_RC_MODE_OVERALLNOLTE)
				chan_rc = rc_report.rc_all_nolte;
			else if (rc_mode == AP_RC_MODE_OVERALLNODFS)
				chan_rc = rc_report.rc_all_nodfs;
			else if (rc_mode == AP_RC_MODE_OVERALLNOLTEDFS)
				chan_rc = rc_report.rc_all_noltedfs;

			ap_rc->chan = chan_rc;
		} else {
			/*
			 * If disabled, fixed modes or query fails then not to
			 * overwrite it
			 */
			chan_rc = chan_config;
			ap_rc->chan = 0;
		}
	} else
		chan_rc = chan_config;

	ath6kl_dbg(ATH6KL_DBG_AP_RC,
			"ap_rc get mode %d chan %d fixed %d config %d use %d\n",
			ap_rc->mode,
			ap_rc->chan,
			ap_rc->chan_fixed,
			chan_config,
			chan_rc);

	return chan_rc;
}

static s8 _ap_rc_build_scan_chan(struct ath6kl *ar, u16 *chan_list)
{
	struct wiphy *wiphy = ar->wiphy;
	struct ieee80211_supported_band *sband;
	struct ieee80211_channel *chan;
	s8 chan_num = 0;
	int i;

	/* TBD : fine tune channel order or needed channels. */
	sband = wiphy->bands[NL80211_BAND_2GHZ];
	for (i = 0; i < sband->n_channels; i++) {
		chan = &sband->channels[i];
		if (!(chan->flags & IEEE80211_CHAN_DISABLED))
			chan_list[chan_num++] = chan->center_freq;
	}

	sband = wiphy->bands[NL80211_BAND_5GHZ];
	for (i = 0; i < sband->n_channels; i++) {
		chan = &sband->channels[i];
		if (!(chan->flags & IEEE80211_CHAN_DISABLED))
			chan_list[chan_num++] = chan->center_freq;
	}

	return chan_num;
}

void ath6kl_ap_rc_update(struct ath6kl_vif *vif)
{
#define _AP_RC_SCAN_TIMEOUT	(2 * HZ)
	struct ath6kl *ar = vif->ar;
	struct ath6kl_vif *vif_tmp;
	struct ap_rc_info *ap_rc = &vif->ap_rc_info_ctx;
	bool scan_on_going = false;
	int i, ret;

	if ((ap_rc->mode == AP_RC_MODE_DISABLE) ||
	    (ap_rc->mode == AP_RC_MODE_FIXED))
		return;

	if (test_bit(DISABLE_SCAN, &ar->flag)) {
		ath6kl_err("ap_rc scan is disabled temporarily\n");
		return;
	}

	if (down_interruptible(&ar->sem)) {
		ath6kl_err("busy, couldn't get access\n");
		return;
	}

	ath6kl_dbg(ATH6KL_DBG_AP_RC,
			"ap_rc udpate start\n");

	/*
	 * If any kind of scan on-going and no need scan now.
	 * Otherwise, start a scan and block until the scan finished.
	 */
	for (i = 0; i < ar->vif_max; i++) {
		vif_tmp = ath6kl_get_vif_by_index(ar, i);
		if (vif_tmp && test_bit(SCANNING, &vif_tmp->flags))
			scan_on_going = true;
	}

	if (scan_on_going == false) {
		u16 *channels;
		s8 num_chan = 0;
		bool left;

		channels = kzalloc(WMI_MAX_CHANNELS * sizeof(u16), GFP_KERNEL);
		if (channels) {
			set_bit(SCANNING, &vif->flags);

			if (!vif->usr_bss_filter) {
				clear_bit(CLEAR_BSSFILTER_ON_BEACON,
						&vif->flags);
				ath6kl_wmi_bssfilter_cmd(ar->wmi,
							vif->fw_vif_idx,
							ALL_BSS_FILTER,
							0);
			}

			num_chan = _ap_rc_build_scan_chan(ar, channels);
			ret = ath6kl_wmi_startscan_cmd(ar->wmi,
							vif->fw_vif_idx,
							WMI_LONG_SCAN,
							true, false, 0, 0,
							num_chan,
							channels);
			if (ret)
				ath6kl_err("wmi_startscan_cmd failed\n");
			else {
				set_bit(SCANNING_WAIT, &vif->flags);

				ath6kl_p2p_rc_scan_start(vif, true);
#ifdef USB_AUTO_SUSPEND
				ath6kl_hif_auto_pm_disable(ar);
#endif

				/* Block until scan finish */
				left = wait_event_interruptible_timeout(
					ar->event_wq,
					!test_bit(SCANNING_WAIT, &vif->flags),
					_AP_RC_SCAN_TIMEOUT);

				if (left == 0) {
					clear_bit(SCANNING_WAIT, &vif->flags);
					ath6kl_wmi_abort_scan_cmd(ar->wmi,
							vif->fw_vif_idx);
					ath6kl_err("ap_rc scan timeout\n");
				}

				if (signal_pending(current)) {
					clear_bit(SCANNING_WAIT, &vif->flags);
					ath6kl_err("ap_rc tgt not respond\n");
				}
			}

			/* Local scan need to clear SCANNING by caller. */
			clear_bit(SCANNING, &vif->flags);

			kfree(channels);
		} else
			ath6kl_err("ap_rc alloc channel_list memory fail!\n");
	}

	up(&ar->sem);

	ath6kl_dbg(ATH6KL_DBG_AP_RC,
			"ap_rc udpate down %s scan\n",
			(scan_on_going ? "without" : "with"));

	return;
#undef _AP_RC_SCAN_TIMEOUT
}

int ath6kl_ap_rc_config(struct ath6kl_vif *vif, int mode_or_freq)
{
	struct ap_rc_info *ap_rc = &vif->ap_rc_info_ctx;

	/* Not yet support DFS in AP mode. */
	if ((mode_or_freq == AP_RC_MODE_5GALL) ||
	    (mode_or_freq == AP_RC_MODE_OVERALL) ||
	    (mode_or_freq == AP_RC_MODE_OVERALLNOLTE)) {
		ath6kl_err("set ap_rc error! mode_or_freq %d not yet support\n",
				mode_or_freq);

		return -EINVAL;
	}

	if (mode_or_freq >= AP_RC_MODE_DISABLE) {
		if (mode_or_freq <= AP_RC_MODE_MAX) {
			ap_rc->mode = mode_or_freq;
			ap_rc->chan = 0;
			ap_rc->chan_fixed = 0;
		} else {
			struct ieee80211_channel *chan;

			chan = ieee80211_get_channel(vif->ar->wiphy,
							mode_or_freq);
			if ((chan) &&
			    !(chan->flags & IEEE80211_CHAN_DISABLED)) {
				ap_rc->mode = AP_RC_MODE_FIXED;
				ap_rc->chan = 0;
				ap_rc->chan_fixed = mode_or_freq;
			} else {
				ath6kl_err("set ap_rc set fixed freq %d fail\n",
						mode_or_freq);

				return -EINVAL;
			}
		}

		ath6kl_dbg(ATH6KL_DBG_AP_RC,
				"ap_rc change to mode %d chan_fixed %d\n",
				ap_rc->mode,
				ap_rc->chan_fixed);

		return 0;
	}
		ath6kl_err("set ap_rc error! mode_or_freq %d\n",
				mode_or_freq);

	return -EINVAL;
}

int ath6kl_ap_ht_update_ies(struct ath6kl_vif *vif)
{
	int ret = 0;

	if (vif->nw_type != AP_NETWORK)
		return 0;

	/* EV117149 WAR : Only overwrite it for 5G band. */
	if (vif->next_chan > 4000) {
		u8 opmode = HT_INFO_OPMODE_PROT_PURE;

		if (vif->sta_no_ht_num > 0)
			opmode = HT_INFO_OPMODE_PROT_MIXED;

		/* TODO : RIFS mode & OBSS-nonHT-STA-Present */

		ret = ath6kl_wmi_set_ht_op_cmd(vif->ar->wmi,
						vif->fw_vif_idx,
						0,
						opmode);
	}

	return ret;
}

static inline bool _ap_is_11b_rate(u8 rate)
{
	u8 rates[] = { 2, 4, 11, 22 };
	u8 i;

	for (i = 0; i < ARRAY_SIZE(rates); i++)
		if (rate == rates[i])
			return true;

	return false;
}

void ath6kl_ap_beacon_info(struct ath6kl_vif *vif, u8 *beacon, u8 beacon_len)
{
	u8 *pie, *peie;
	u8 *rates_ie = NULL;
	u8 *ext_rates_ie = NULL;
	struct ieee80211_ht_cap *ht_cap_ie = NULL;
	struct ieee80211_ht_oper *ht_oper_ie = NULL;

	/*
	 * Get host interesting AP configuration by parsing the beacon content
	 * from AP CONNECTED event.
	 */
	if (vif->nw_type != AP_NETWORK)
		return;

	/* bypass timestamp, beacon interval and capability fields */
	pie = beacon + 8 + 2 + 2;
	peie = beacon + beacon_len;

	while (pie < peie) {
		switch (*pie) {
		case WLAN_EID_SUPP_RATES:
			if (pie[1])
				rates_ie = pie;
			break;
		case WLAN_EID_EXT_SUPP_RATES:
			if (pie[1])
				ext_rates_ie = pie;
			break;
		case WLAN_EID_HT_CAPABILITY:
			if (pie[1] >= sizeof(struct ieee80211_ht_cap))
				ht_cap_ie =
				(struct ieee80211_ht_cap *)(pie + 2);
			break;
		case WLAN_EID_HT_OPER:
			if (pie[1] >= sizeof(struct ieee80211_ht_oper))
				ht_oper_ie =
				(struct ieee80211_ht_oper *)(pie + 2);
			break;
		}
		pie += pie[1] + 2;
	}

	if (ht_cap_ie && ht_oper_ie) {	/* 11N */
		u16 cap_info = le16_to_cpu(ht_cap_ie->cap_info);
		u8 second_chan = (ht_oper_ie->ht_param &
					IEEE80211_HT_PARAM_CHA_SEC_OFFSET);
		bool ext_chan = false;

		vif->chan_type = ATH6KL_CHAN_TYPE_HT20;

		if (second_chan &&
		    (cap_info & IEEE80211_HT_CAP_SUP_WIDTH_20_40)) {
			ext_chan = true;
			if (second_chan == IEEE80211_HT_PARAM_CHA_SEC_ABOVE)
				vif->chan_type = ATH6KL_CHAN_TYPE_HT40PLUS;
			else
				vif->chan_type = ATH6KL_CHAN_TYPE_HT40MINUS;
		}

		if (vif->bss_ch > 5000) {	/* 11NA */
			if (ext_chan)
				vif->phymode = ATH6KL_PHY_MODE_11NA_HT40;
			else
				vif->phymode = ATH6KL_PHY_MODE_11NA_HT20;
		} else {			/* 11NG */
			if (ext_chan)
				vif->phymode = ATH6KL_PHY_MODE_11NG_HT40;
			else
				vif->phymode = ATH6KL_PHY_MODE_11NG_HT20;
		}
	} else {		/* not 11N */
		vif->chan_type = ATH6KL_CHAN_TYPE_NONE;

		if (vif->bss_ch > 5000)		/* 11A */
			vif->phymode = ATH6KL_PHY_MODE_11A;
		else {				/* 11B/G/GONLY */
			u8 i, rate;
			bool b_rates, g_rates;

			b_rates = g_rates = false;
			if (rates_ie) {
				for (i = 0; i < rates_ie[1]; i++) {
					rate = rates_ie[2 + i] & 0x7f;
					if (_ap_is_11b_rate(rate))
						b_rates = true;
					else
						g_rates = true;
				}
			}
			if (ext_rates_ie) {
				for (i = 0; i < ext_rates_ie[1]; i++) {
					rate = ext_rates_ie[2 + i] & 0x7f;
					if (_ap_is_11b_rate(rate))
						b_rates = true;
					else
						g_rates = true;
				}
			}

			if (g_rates)
				if (b_rates)
					vif->phymode = ATH6KL_PHY_MODE_11G;
				else
					vif->phymode = ATH6KL_PHY_MODE_11GONLY;
			else
				if (b_rates)
					vif->phymode = ATH6KL_PHY_MODE_11B;
				else
					ath6kl_err("Unknown AP phymode\n");
		}
	}

	return;
}

void ath6kl_ap_ch_switch(struct ath6kl_vif *vif)
{
	if (vif->nw_type != AP_NETWORK)
		return;

#ifdef CE_SUPPORT
{
	u32 freq;
	enum nl80211_channel_type channel_type;
	freq = (u32)le16_to_cpu(vif->bss_ch);

	if (vif->chan_type == ATH6KL_CHAN_TYPE_NONE)
		channel_type = NL80211_CHAN_NO_HT;
	else if (vif->chan_type == ATH6KL_CHAN_TYPE_HT20)
		channel_type = NL80211_CHAN_HT20;
	else if (vif->chan_type == ATH6KL_CHAN_TYPE_HT40MINUS)
		channel_type = NL80211_CHAN_HT40MINUS;
	else if (vif->chan_type == ATH6KL_CHAN_TYPE_HT40PLUS)
		channel_type = NL80211_CHAN_HT40PLUS;
	cfg80211_csa_done_info(vif->ndev, freq, channel_type, 1, GFP_ATOMIC);
}
#endif
	/*
	 * If target use the different channel setting as the host and use this
	 * helper API to update the working channel information to the user.
	 *
	 * If target support WMI_CHANNEL_CHANGE_EVENTID (DFS?) and this helper
	 * API may also useful.
	 */

#ifdef NL80211_CMD_OPER_CH_SWITCH_NOTIFY
	/*
	 * The next_chan_type is only valid when
	 * ATH6KL_MODULE_ENABLE_P2P_CHANMODE is on.
	 */
	if ((vif->next_chan != vif->bss_ch) ||
	    (vif->next_chan_type != vif->chan_type)) {
		if (ath6kl_mod_debug_quirks(vif->ar,
				ATH6KL_MODULE_ENABLE_P2P_CHANMODE)) {
			enum nl80211_channel_type type = NL80211_CHAN_NO_HT;
#ifdef CFG80211_NEW_CHAN_DEFINITION
			struct ieee80211_channel *chan;
			struct cfg80211_chan_def chandef;
#endif

			ath6kl_info("AP Channel switch from %d/%d to %d/%d\n",
					vif->next_chan, vif->next_chan_type,
					vif->bss_ch, vif->chan_type);

			if (vif->chan_type == ATH6KL_CHAN_TYPE_NONE)
				type = NL80211_CHAN_NO_HT;
			else if (vif->chan_type == ATH6KL_CHAN_TYPE_HT40PLUS)
				type = NL80211_CHAN_HT40PLUS;
			else if (vif->chan_type == ATH6KL_CHAN_TYPE_HT40MINUS)
				type = NL80211_CHAN_HT40MINUS;
			else if (vif->chan_type == ATH6KL_CHAN_TYPE_HT20)
				type = NL80211_CHAN_HT20;
			else
				WARN_ON(1);

			/*
			 * TODO: Better to check channel information is valid
			 * or not for P2P-GO mode before report to the user.
			 */
#ifdef CFG80211_NEW_CHAN_DEFINITION
			chan = ieee80211_get_channel(vif->ar->wiphy,
							vif->bss_ch);
			if (chan == NULL) {
				WARN_ON(1);
				return;
			}

			cfg80211_chandef_create(&chandef, chan, type);
			cfg80211_ch_switch_notify(vif->ndev,
						&chandef);
#else
			cfg80211_ch_switch_notify(vif->ndev,
						vif->bss_ch,
						type);
#endif
		}
	}
#endif
	return;
}

