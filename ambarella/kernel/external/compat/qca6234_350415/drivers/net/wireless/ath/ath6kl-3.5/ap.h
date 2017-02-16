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
#define ATH6KL_AP_KA_RECLAIM_CYCLE		(4)		/* 1 min. */
#define ATH6KL_AP_KA_RECLAIM_TIME_MAX		((15 * 60) * 1000)

/* Do some fine tune to overwrite the config in P2P cases. */
#define ATH6KL_AP_KA_RECLAIM_CYCLE_SCC		(16)		/* 4 min. */
#define ATH6KL_AP_KA_RECLAIM_CYCLE_MCC		(19)		/* 4.75 min. */

/* At least WMI_TIMEOUT */
#define ATH6KL_AP_KA_PRELOAD_LEADTIME		(2 * 1000)

/* flags */
#define ATH6KL_AP_KA_FLAGS_ENABLED		BIT(0)
#define ATH6KL_AP_KA_FLAGS_BY_SUPP		BIT(1)	/* offload to user */
#define ATH6KL_AP_KA_FLAGS_START		BIT(2)
#define ATH6KL_AP_KA_FLAGS_PRELOAD_STAT		BIT(3)	/* for preload state */
#define ATH6KL_AP_KA_FLAGS_CONFIG_BY_SUPP	BIT(4)

/* Next action */
#define AP_KA_ACTION_NONE			(0)
#define AP_KA_ACTION_POLL			(1)
#define AP_KA_ACTION_REMOVE			(2)

/* Operation mode */
enum ap_keepalive_mode {
	/* Disabled and using the target's mechanism. */
	AP_KA_MODE_DISABLE = 0,

	/* Using driver's mechanism and setting from the driver or debugfs. */
	AP_KA_MODE_ENABLE,

	/* Using supplicant/hostapd's mechanism. */
	AP_KA_MODE_BYSUPP,

	/* Using driver's mechanism but config from the supplicant/hostapd. */
	AP_KA_MODE_CONFIG_BYSUPP,
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

/* Admission-Control mode */
#define ATH6KL_AP_ADMC_ASSOC_REQ_MAX_LEN	(1200)
#define ATH6KL_AP_ADMC_ASSOC_REQ_TIMEOUT	(500)	/* in ms. */

enum ap_admc_mode {
	AP_ADMC_MODE_DISABLE = 0,	/* by firmware */
	AP_ADMC_MODE_ACCEPT_ALWAYS = 1,	/* always accept */
};

enum ap_admc_action {
	AP_ADMC_ACT_ACCPET = 0,
	AP_ADMC_ACT_TIMEOUT = 1,
	AP_ADMC_ACT_FLUSH = 2,
};

struct ap_admc_assoc_req {
	struct list_head list;

	struct ap_admc_info *ap_admc;
	struct timer_list reclaim_timer;
	u8 raw_frame[ATH6KL_AP_ADMC_ASSOC_REQ_MAX_LEN];
	u16 frame_len;
	u8 *sta_mac;
	enum ap_admc_action action;
};

struct ap_admc_info {
	struct ath6kl_vif *vif;

	enum ap_admc_mode admc_mode;
	spinlock_t assoc_req_lock;
	struct list_head assoc_req_list;
	int assoc_req_timeout;

	u32 assoc_req_cnt;
};

/*
 * WLAN_EID_HT_INFORMATION & struct ieee80211_ht_info in ieee80211.h
 * is changed to WLAN_EID_HT_OPERATION & struct ieee80211_ht_operation
 * from kernel3.5. Using local defines instead of dirty compiler flag.
 */
#define WLAN_EID_HT_OPER	(61)

struct ieee80211_ht_oper {
	u8 primary_chan;
	u8 ht_param;
	__le16 operation_mode;
	__le16 stbc_param;
	u8 basic_set[16];
} __packed;

/* AP Recommend Channel */
enum ap_rc_mode {
	AP_RC_MODE_DISABLE = 0,
	AP_RC_MODE_FIXED = 1,
	AP_RC_MODE_2GALL = 2,
	AP_RC_MODE_2GPOP = 3,		/* Channel 1,6 or 11 */
	AP_RC_MODE_5GALL = 4,
	AP_RC_MODE_OVERALL = 5,
	AP_RC_MODE_2GNOLTE = 6,
	AP_RC_MODE_5GNODFS = 7,
	AP_RC_MODE_OVERALLNOLTE = 8,
	AP_RC_MODE_OVERALLNODFS = 9,
	AP_RC_MODE_OVERALLNOLTEDFS = 10,

	/* keep last */
	AP_RC_MODE_MAX = AP_RC_MODE_OVERALLNOLTEDFS,
};

struct ap_rc_info {
	enum ap_rc_mode mode;
	u16 chan;
	u16 chan_fixed;		/* AP_RC_MODE_FIXED */
};

struct ap_keepalive_info *ath6kl_ap_keepalive_init(struct ath6kl_vif *vif,
						   enum ap_keepalive_mode mode);
void ath6kl_ap_keepalive_deinit(struct ath6kl_vif *vif);
int ath6kl_ap_keepalive_start(struct ath6kl_vif *vif);
void ath6kl_ap_keepalive_stop(struct ath6kl_vif *vif);
int ath6kl_ap_keepalive_config(struct ath6kl_vif *vif,
			       u32 ap_ka_interval,
			       u32 ap_ka_reclaim_cycle);
int ath6kl_ap_keepalive_config_by_supp(struct ath6kl_vif *vif,
			       u16 inactive_time);
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
struct ap_admc_info *ath6kl_ap_admc_init(struct ath6kl_vif *vif,
	enum ap_admc_mode mode);
void ath6kl_ap_admc_deinit(struct ath6kl_vif *vif);
int ath6kl_ap_admc_start(struct ath6kl_vif *vif);
int ath6kl_ap_admc_stop(struct ath6kl_vif *vif);
void ath6kl_ap_admc_assoc_req(struct ath6kl_vif *vif,
	u8 *assocReq,
	u16 assocReq_len,
	u8 req_type,
	u8 fw_status);
void ath6kl_ap_admc_assoc_req_fetch(struct ath6kl_vif *vif,
	struct wmi_connect_event *ev,
	u8 **assocReq,
	u16 *assocReq_len);
void ath6kl_ap_admc_assoc_req_release(struct ath6kl_vif *vif,
	u8 *assocReq);
int ath6kl_ap_admc_dump(struct ath6kl *ar, u8 *buf, int buf_len);
void ath6kl_ap_rc_init(struct ath6kl_vif *vif);
u16 ath6kl_ap_rc_get(struct ath6kl_vif *vif, u16 chan_config);
void ath6kl_ap_rc_update(struct ath6kl_vif *vif);
int ath6kl_ap_rc_config(struct ath6kl_vif *vif, int mode_or_freq);
int ath6kl_ap_ht_update_ies(struct ath6kl_vif *vif);
void ath6kl_ap_beacon_info(struct ath6kl_vif *vif, u8 *beacon, u8 beacon_len);
void ath6kl_ap_ch_switch(struct ath6kl_vif *vif);
#endif

