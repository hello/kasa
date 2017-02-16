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

#ifndef HTCOEX_H
#define HTCOEX_H

#define ATH6KL_HTCOEX_SCAN_PERIOD			(60 * 1000) /* in ms */

/* in scan cycle */
#define ATH6KL_HTCOEX_RATE_ROLLBACK			(0)

/* htcoex enable/disable */
#define ATH6KL_HTCOEX_FLAGS_ENABLED			BIT(0)

/* htcoex start/stop */
#define ATH6KL_HTCOEX_FLAGS_START			BIT(1)

/* 1x1/2x2 solution only */
#define ATH6KL_HTCOEX_RATEMASK_FULL		(0x00000fffffffffffULL)
#define ATH6KL_HTCOEX_RATEMASK_HT20		(0x000000000fffffffULL)
#define ATH6KL_HTCOEX_RATEMASK_HT40		(0x00000fffffffffffULL)

struct htcoex_coex_info {
	u32 intolerant40;
	int num_chans;
	u8 chans[14];
};

struct htcoex_bss_info {
	struct list_head list;

	u8 *raw_frame;
	u16 frame_len;
	u8 bssid[ETH_ALEN];
	struct ieee80211_channel *channel;
};

struct htcoex {
	struct ath6kl_vif *vif;
	u32 flags;
	u32 scan_interval;		/* in ms, 0 means htcoex disable. */
	u32 num_scan;

	u8 rate_rollback_interval; /* in scan cycle, 0 means no roll-back. */
	u64 current_ratemask;
	u32 tolerant40_cnt;

	struct timer_list scan_timer;
	struct cfg80211_scan_request request;

	spinlock_t bss_info_lock;
	struct list_head bss_info_list;

	s8 num_scan_channels;
	u16 *scan_channels;
};

enum {
	HTCOEX_PORC_SCAN_DONE = 0,
	HTCOEX_PASS_SCAN_DONE,
};

/* COEX action codes */
enum ieee80211_coex_actioncode {
	WLAN_COEX_ACTION_2040COEX_MGMT = 0,
};

/* Public action frame format */
struct ieee80211_action_public {
	u8 category;
	u8 action_code;
	u8 variable[0];
} __packed;

/* 20/40 BSS Coexistence IE */
#define IEEE80211_COEX_IE_INFO_REQ		(1 << 0)
#define IEEE80211_COEX_IE_40_INTOLERANT		(1 << 1)
#define IEEE80211_COEX_IE_20_WIDTH_REQ		(1 << 2)
#define IEEE80211_COEX_IE_OBSS_SCAN_REQ		(1 << 3)
#define IEEE80211_COEX_IE_OBSS_SCAN_GRANT	(1 << 4)

struct ieee80211_bss_coex_ie {
	u8 element_id;
	u8 len;
	u8 value;
} __packed;

/* 20/40 BSS Intolerant Channel Report IE */
#define	WLAN_EID_INTOLERANT_CHAN_REPORT		73

struct ieee80211_intolerant_chan_report_ie {
	u8 element_id;
	u8 len;
	u8 reg_class;
	u8 chan_variable[0];
} __packed;

struct htcoex *ath6kl_htcoex_init(struct ath6kl_vif *vif);
void ath6kl_htcoex_deinit(struct ath6kl_vif *vif);
void ath6kl_htcoex_bss_info(struct ath6kl_vif *vif,
			    struct ieee80211_mgmt *mgmt,
			    int len,
			    struct ieee80211_channel *channel);
int ath6kl_htcoex_scan_complete_event(struct ath6kl_vif *vif, bool aborted);
void ath6kl_htcoex_connect_event(struct ath6kl_vif *vif);
void ath6kl_htcoex_disconnect_event(struct ath6kl_vif *vif);
int ath6kl_htcoex_config(struct ath6kl_vif *vif,
	u32 interval, u8 rate_rollback);
#endif
