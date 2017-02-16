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

#ifndef ACS_H
#define ACS_H

struct acs_bss_info {
	struct list_head list;
	u8 snr;
	u8 *raw_frame;
	u16 frame_len;
	u8 bssid[ETH_ALEN];
	struct ieee80211_channel *channel;
};

struct acs {
	struct ath6kl_vif *vif;
	struct cfg80211_scan_request request;

	spinlock_t bss_info_lock;
	struct list_head bss_info_list;
};

enum {
	ACS_PORC_SCAN_DONE = 0,
	ACS_PASS_SCAN_DONE,
};

struct acs *ath6kl_acs_init(struct ath6kl_vif *vif);
void ath6kl_acs_deinit(struct ath6kl_vif *vif);
void ath6kl_acs_bss_info(struct ath6kl_vif *vif,
					struct ieee80211_mgmt *mgmt,
					int len,
					struct ieee80211_channel *channel,
					u8 snr);
int ath6kl_acs_scan_complete_event(struct ath6kl_vif *vif, bool aborted);
int ath6kl_acs_find_info(struct ath6kl_vif *vif) ;
void ath6kl_acs_get_info(struct ath6kl_vif *vif, int *best_chan);
int ath6kl_acs_ioctl(struct net_device *netdev, struct ifreq *ifr, int cmd);
#endif
