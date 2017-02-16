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

#ifndef AP_H
#define AP_H

/* Time defines */
#define ATH6KL_AP_KA_INTERVAL_DEFAULT		(15 * 1000)	/* in ms. */
#define ATH6KL_AP_KA_INTERVAL_MIN		(5 * 1000)	/* in ms. */
#define ATH6KL_AP_KA_RECLAIM_CYCLE_DEFAULT	(4)		/* 1 min. */
#define ATH6KL_AP_KA_RECLAIM_TIME_MAX		((15 * 60) * 1000)

/* At least WMI_TIMEOUT */
#define ATH6KL_AP_KA_PRELOAD_LEADTIME		(2 * 1000)

/* For P2P case, use the lower values for Android platform. */
#define ATH6KL_AP_KA_INTERVAL_DEFAULT_P2P	(10 * 1000)	/* in ms. */
#define ATH6KL_AP_KA_RECLAIM_CYCLE_DEFAULT_P2P	(2)		/* 20 sec. */

/* flags */
#define ATH6KL_AP_KA_FLAGS_ENABLED		BIT(0)
#define ATH6KL_AP_KA_FLAGS_BY_SUPP		BIT(1)	/* offload to user */
#define ATH6KL_AP_KA_FLAGS_START		BIT(2)
#define ATH6KL_AP_KA_FLAGS_PRELOAD_STAT		BIT(3)	/* for preload state */

/* Next action */
#define AP_KA_ACTION_NONE			(0)
#define AP_KA_ACTION_POLL			(1)
#define AP_KA_ACTION_REMOVE			(2)

/* Operation mode */
enum ap_keepalive_mode {
	AP_KA_MODE_DISABLE = 0,
	AP_KA_MODE_ENABLE,
	AP_KA_MODE_BYSUPP,
};

enum ap_keepalive_adjust {
	AP_KA_ADJ_ERROR = -1,
	AP_KA_ADJ_BASESET = 0,
	AP_KA_ADJ_ADJUST,
};

struct ap_keepalive_info {
	struct ath6kl_vif *vif;
	u32 flags;

	struct timer_list ap_ka_timer;
	/* In ms., run checking per ap_ka_interval */
	u32 ap_ka_interval;

	/* Reclaim STA after N checking fails */
	u32 ap_ka_reclaim_cycle;

	/* Remove this station after (ap_ka_interval * ap_ka_reclaim_cycle) */
	u32 ap_ka_remove_time;
};

/* ACL defines */
#define ATH6KL_AP_ACL_MAX_NUM		(10)	/* limited by AP_ACL_SIZE */

#define ATH6KL_AP_ACL_FLAGS_USED	(1 << 0)

/* Operation mode */
enum ap_acl_mode {
	AP_ACL_MODE_DISABLE = 0,
	AP_ACL_MODE_ALLOW,
	AP_ACL_MODE_DENY,
};

struct ap_acl_entry {
	u32 flags;
	u8 mac_addr[ETH_ALEN];
};

struct ap_acl_info {
	struct ath6kl_vif *vif;

	enum ap_acl_mode acl_mode;
	struct ap_acl_entry acl_list[ATH6KL_AP_ACL_MAX_NUM];

	/*
	 * Cache last ACL setting from hostapd.
	 * Need special hostapd to support this.
	 */
	u8 *last_acl_config;
};

struct ap_keepalive_info *ath6kl_ap_keepalive_init(struct ath6kl_vif *vif,
						   enum ap_keepalive_mode mode);
void ath6kl_ap_keepalive_deinit(struct ath6kl_vif *vif);
int ath6kl_ap_keepalive_start(struct ath6kl_vif *vif);
void ath6kl_ap_keepalive_stop(struct ath6kl_vif *vif);
int ath6kl_ap_keepalive_config(struct ath6kl_vif *vif,
			       u32 ap_ka_interval,
			       u32 ap_ka_reclaim_cycle);
u32 ath6kl_ap_keepalive_get_inactive_time(struct ath6kl_vif *vif, u8 *mac);
bool ath6kl_ap_keepalive_by_supp(struct ath6kl_vif *vif);
struct ap_acl_info *ath6kl_ap_acl_init(struct ath6kl_vif *vif);
void ath6kl_ap_acl_deinit(struct ath6kl_vif *vif);
int ath6kl_ap_acl_start(struct ath6kl_vif *vif);
int ath6kl_ap_acl_stop(struct ath6kl_vif *vif);
int ath6kl_ap_acl_config_policy(struct ath6kl_vif *vif,
				enum ap_acl_mode mode);
int ath6kl_ap_acl_config_mac_list(struct ath6kl_vif *vif,
				u8 *mac_addr, bool removed);
int ath6kl_ap_acl_config_mac_list_reset(struct ath6kl_vif *vif);
int ath6kl_ap_acl_dump(struct ath6kl *ar, u8 *buf, int buf_len);
#endif

