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

#ifndef P2P_H
#define P2P_H

#define ATH6KL_P2P_BMISS_TIME				(50)

#define ATH6KL_P2P_PS_MAX_NOA_DESCRIPTORS		4

#define ATH6KL_P2P_PS_FLAGS_NOA_ENABLED			BIT(0)
#define ATH6KL_P2P_PS_FLAGS_OPPPS_ENABLED		BIT(1)

#define NL80211_IFTYPE_P2P_DEVICE_QCA			(0xff)

enum {
	IEEE80211_P2P_ATTR_NOTICE_OF_ABSENCE = 12,
};

enum {
	WLAN_PUB_ACTION_VENDER_SPECIFIC = 9,
};

enum {
	WLAN_P2P_GO_NEG_REG = 0,
	WLAN_P2P_GO_NEG_RESP = 1,
	WLAN_P2P_GO_NEG_CONF = 2,
	WLAN_P2P_INVITATION_REQ = 3,
	WLAN_P2P_INVITATION_RESP = 4,
	WLAN_P2P_DEV_DISC_REQ = 5,
	WLAN_P2P_DEV_DISC_RESP = 6,
	WLAN_P2P_PROV_DISC_REQ = 7,
	WLAN_P2P_PROV_DISC_RESP = 8
};

struct ieee80211_p2p_action_public {
	u8 category;
	u8 action_code;
	u32 oui;
	u8 action_subtype;
	u8 variable[0];
} __packed;

struct ieee80211_p2p_action_vendor {
	u8 category;
	u32 oui;
	u8 action_subtype;
	u8 variable[0];
} __packed;

struct ieee80211_p2p_noa_descriptor {
	u8 count_or_type;	/* 255: continuous schedule, 0: reserved */
	__le32 duration;
	__le32 interval;
	__le32 start_or_offset;
} __packed;

struct ieee80211_p2p_noa_info {
	u8 index;
	u8 ctwindow_opps_param;
	struct ieee80211_p2p_noa_descriptor
		noas[ATH6KL_P2P_PS_MAX_NOA_DESCRIPTORS];
} __packed;

struct ieee80211_p2p_noa_ie {
	u8 element_id;
	u8 len;
	u32 oui;
	u8 attr;
	u16 attr_len;
	struct ieee80211_p2p_noa_info noa_info;
} __packed;

struct p2p_ps_info {
	struct ath6kl_vif *vif;
	spinlock_t p2p_ps_lock;

	/* P2P-GO */
	u32 go_flags;
	u8 go_noa_enable_idx;
	struct ieee80211_p2p_noa_info go_noa;

	/* Cached information */
	u8 *go_last_beacon_app_ie;
	u16 go_last_beacon_app_ie_len;
	u8 *go_last_noa_ie;
	u16 go_last_noa_ie_len;
	u8 *go_working_buffer;

	/* counter */
	u32 go_noa_notif_cnt;
};

/* P2P Flowctrl */
#define ATH6KL_P2P_FLOWCTRL_NULL_CONNID			(0xff)
#define ATH6KL_P2P_FLOWCTRL_RECYCLE_LIMIT		(10)

#define ATH6KL_P2P_FLOWCTRL_NETIF_STOP			(0x00)
#define ATH6KL_P2P_FLOWCTRL_NETIF_WAKE			(0x01)

#define ATH6KL_P2P_FLOWCTRL_REQ_STEP			(30)
#define ATH6KL_P2P_FLOWCTRL_TXQ_LIMIT			(50)

struct ath6kl_fw_conn_list {
	struct list_head conn_queue;
	struct list_head re_queue;

	u8 connId;		/* ID sync between host and target. */
	u8 parent_connId;	/* For P2P_FLOWCTRL_SCHE_TYPE_INTERFACE */
	u8 mac_addr[ETH_ALEN];
	struct ath6kl_vif *vif;

	union {
		struct {
			u8 bk_uapsd:1;
			u8 be_uapsd:1;
			u8 vi_uapsd:1;
			u8 vo_uapsd:1;
			u8 ps:1;	/* 1 means power saved */
			u8 ocs:1;	/* 1 means off channel */
			u8 res:2;
		};
		u8 connect_status;
	};

	bool previous_can_send;

	/* stat */
	int sche_tx_queued;
	u32 sche_tx;
	u32 sche_re_tx;
	u32 sche_re_tx_aging;
};

enum p2p_flowctrl_sche_type {
	P2P_FLOWCTRL_SCHE_TYPE_CONNECTION,
	P2P_FLOWCTRL_SCHE_TYPE_INTERFACE,
};

struct ath6kl_p2p_flowctrl {
	struct ath6kl *ar;
	spinlock_t p2p_flowctrl_lock;

	enum p2p_flowctrl_sche_type sche_type;
	struct ath6kl_fw_conn_list fw_conn_list[NUM_CONN];

	u32 p2p_flowctrl_event_cnt;
};

/* P2P RecommendChannel */
#define ATH6KL_RC_FLAGS_NEED_UPDATED		(1 << 0)
#define ATH6KL_RC_FLAGS_DONE			(1 << 1)
#define ATH6KL_RC_FLAGS_HIGH_CHAN		(1 << 2)
#define ATH6KL_RC_FLAGS_ALWAYS_FRESH		(1 << 3)
#define ATH6KL_RC_FLAGS_IGNORE_DFS_CHAN		(1 << 4)
#define ATH6KL_RC_FLAGS_CHAN_RECORD_FETCHED	(1 << 5)

#define ATH6KL_RC_MAX_2G_CHAN_RECORD	(14)
#define ATH6KL_RC_MAX_5G_CHAN_RECORD	(66)
#define ATH6KL_RC_MAX_CHAN_RECORD	(ATH6KL_RC_MAX_2G_CHAN_RECORD + \
					ATH6KL_RC_MAX_5G_CHAN_RECORD)

#define ATH6KL_RC_AVERAGE_CHAN_CNT	(5)	/* 20MHz width */
#define ATH6KL_RC_AVERAGE_CHAN_OFFSET	(2)	/* 10MHz plus or minus */
#define ATH6KL_RC_AVERAGE_CHAN_START	(2412)	/* Channel 1 */
#define ATH6KL_RC_AVERAGE_CHAN_END	(2472)	/* Channel 13 */

#define ATH6KL_RC_FRESH_TIME		msecs_to_jiffies(5 * 60 * 1000)

enum p2p_rc_type {
	P2P_RC_TYPE_2GALL,	/* the best in all 2G channels */
	P2P_RC_TYPE_5GALL,	/* the best in all 5G channels */
	P2P_RC_TYPE_OVERALL,	/* the best in all channels */
	P2P_RC_TYPE_SOCAIL,	/* the best in 1, 6 abd 11 channels */
	P2P_RC_TYPE_2GP2P,	/* the best in P2P 2G channels */
	P2P_RC_TYPE_5GP2P,	/* the best in P2P 5G channels */
	P2P_RC_TYPE_ALLP2P,	/* the best in P2P channels */

	P2P_RC_TYPE_MAX,	/* keep last */
};

struct p2p_rc_bss_info {
	struct list_head list;
	struct ieee80211_channel *channel;
	u8 rssi;
};

struct p2p_rc_chan_record {
	struct ieee80211_channel *channel;

#define P2P_RC_MAX_SNR		(96)
#define P2P_RC_NULL_SNR		(P2P_RC_MAX_SNR + 1)
	u8 best_snr;
	u8 aver_snr;	/* for 2G */
};

struct ath6kl_p2p_rc_info {
	struct ath6kl *ar;
	u32 flags;
	spinlock_t p2p_rc_lock;
	int chan_record_cnt;
	struct p2p_rc_chan_record chan_record[ATH6KL_RC_MAX_CHAN_RECORD];
	unsigned long last_update;		/* in jiffies */

#define P2P_RC_DEF_SNR_COMP		(0)
	int snr_compensation;

	/* Keep the latest result. */
	struct p2p_rc_chan_record *last_p2p_rc[P2P_RC_TYPE_MAX];
};

struct p2p_oper_chan {
	u16 oper_class;
	u32 min_chan_freq;
	u32 max_chan_freq;
	u32 inc_freq;

#define P2P_OPER_CHAN_BW_NULL		0
#define P2P_OPER_CHAN_BW_HT20		1
#define P2P_OPER_CHAN_BW_HT40_PLUS	2
#define P2P_OPER_CHAN_BW_HT40_MINUS	3
	u8 bw;
};

struct p2p_pending_connect_info {
#define ATH6KL_P2P_MAX_PENDING_INFO_IELEN	512
	u8 bssid[ETH_ALEN];
	u8 req_ie[ATH6KL_P2P_MAX_PENDING_INFO_IELEN];
	size_t req_ie_len;
	u8 resp_ie[ATH6KL_P2P_MAX_PENDING_INFO_IELEN];
	size_t resp_ie_len;
	u16 status;
	gfp_t gfp;
};

struct p2p_ps_info *ath6kl_p2p_ps_init(struct ath6kl_vif *vif);
void ath6kl_p2p_ps_deinit(struct ath6kl_vif *vif);

int ath6kl_p2p_ps_reset_noa(struct p2p_ps_info *p2p_ps);
int ath6kl_p2p_ps_setup_noa(struct p2p_ps_info *p2p_ps,
			    int noa_id,
			    u8 count_type,
			    u32 interval,
			    u32 start_offset,
			    u32 duration);

int ath6kl_p2p_ps_reset_opps(struct p2p_ps_info *p2p_ps);
int ath6kl_p2p_ps_setup_opps(struct p2p_ps_info *p2p_ps,
			     u8 enabled,
			     u8 ctwindows);

int ath6kl_p2p_ps_update_notif(struct p2p_ps_info *p2p_ps);
void ath6kl_p2p_ps_user_app_ie(struct p2p_ps_info *p2p_ps, u8 mgmt_frm_type,
			       u8 **ie, int *len);

int ath6kl_p2p_utils_trans_porttype(enum nl80211_iftype type,
				    u8 *opmode,
				    u8 *subopmode);
int ath6kl_p2p_utils_init_port(struct ath6kl_vif *vif,
			       enum nl80211_iftype type);
int ath6kl_p2p_utils_check_port(struct ath6kl_vif *vif,
				u8 port_id);

struct ath6kl_p2p_flowctrl *ath6kl_p2p_flowctrl_conn_list_init(
	struct ath6kl *ar);
void ath6kl_p2p_flowctrl_conn_collect_by_conn(
	struct ath6kl_fw_conn_list *fw_conn, struct list_head *pcontainer,
	int *preclaim);
void ath6kl_p2p_flowctrl_conn_list_deinit(struct ath6kl *ar);
void ath6kl_p2p_flowctrl_conn_list_cleanup(struct ath6kl *ar);
void ath6kl_p2p_flowctrl_conn_list_cleanup_by_if(struct ath6kl_vif *vif);
void ath6kl_p2p_flowctrl_netif_transition(struct ath6kl *ar,
			u8 new_state);
void ath6kl_p2p_flowctrl_tx_schedule(struct ath6kl *ar);
int ath6kl_p2p_flowctrl_tx_schedule_pkt(struct ath6kl *ar, void *pkt);
void ath6kl_p2p_flowctrl_state_change(struct ath6kl *ar);
void ath6kl_p2p_flowctrl_state_update(struct ath6kl *ar,
				      u8 numConn,
				      u8 ac_map[],
				      u8 ac_queue_depth[]);
void ath6kl_p2p_flowctrl_set_conn_id(struct ath6kl_vif *vif,
				     u8 mac_addr[],
				     u8 connId);
u8 ath6kl_p2p_flowctrl_get_conn_id(struct ath6kl_vif *vif,
				   struct sk_buff *skb);
int ath6kl_p2p_flowctrl_stat(struct ath6kl *ar,
			     u8 *buf, int buf_len);


struct ath6kl_p2p_rc_info *ath6kl_p2p_rc_init(struct ath6kl *ar);
void ath6kl_p2p_rc_deinit(struct ath6kl *ar);
void ath6kl_p2p_rc_fetch_chan(struct ath6kl *ar);
void ath6kl_p2p_rc_bss_info(struct ath6kl_vif *vif,
			    u8 snr,
			    struct ieee80211_channel *channel);
void ath6kl_p2p_rc_scan_start(struct ath6kl_vif *vif);
int ath6kl_p2p_rc_scan_complete_event(struct ath6kl_vif *vif, bool aborted);
int ath6kl_p2p_rc_get(struct ath6kl *ar,
			u16 *rc_2g,
			u16 *rc_5g,
			u16 *rc_all,
			u16 *rc_p2p_social,
			u16 *rc_p2p_2g,
			u16 *rc_p2p_5g,
			u16 *rc_p2p_all);
int ath6kl_p2p_rc_dump(struct ath6kl *ar, u8 *buf, int buf_len);

bool ath6kl_p2p_frame_retry(struct ath6kl *ar, u8 *frm, int len);
bool ath6kl_p2p_is_p2p_frame(struct ath6kl *ar, const u8 *frm, size_t len);
void ath6kl_p2p_connect_event(struct ath6kl_vif *vif,
				u8 beacon_ie_len,
				u8 assoc_req_len,
				u8 assoc_resp_len,
				u8 *assoc_info);
void ath6kl_p2p_reconfig_ps(struct ath6kl *ar,
			bool mcc,
			bool call_on_disconnect);
bool ath6kl_p2p_pending_connect_event(struct ath6kl_vif *vif,
					const u8 *bssid,
					const u8 *req_ie,
					size_t req_ie_len,
					const u8 *resp_ie,
					size_t resp_ie_len,
					u16 status,
					gfp_t gfp);
void ath6kl_p2p_pending_disconnect_event(struct ath6kl_vif *vif,
					u16 reason,
					u8 *ie,
					size_t ie_len,
					gfp_t gfp);
bool ath6kl_p2p_ie_append(struct ath6kl_vif *vif, u8 mgmt_frame_type);
bool ath6kl_p2p_is_p2p_channel(u32 freq);
bool ath6kl_p2p_is_social_channel(u32 freq);
#endif

