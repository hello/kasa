/*
 * Copyright (c) 2011 Atheros Communications Inc.
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

#ifndef ATH6KL_CFG80211_H
#define ATH6KL_CFG80211_H

enum ath6kl_cfg_suspend_mode {
	ATH6KL_CFG_SUSPEND_DEEPSLEEP,
	ATH6KL_CFG_SUSPEND_CUTPOWER,
	ATH6KL_CFG_SUSPEND_WOW
};

extern unsigned int testmode;
extern unsigned int ath6kl_p2p;
extern unsigned int ath6kl_vap;
extern unsigned int ath6kl_wow_ext;
extern unsigned int ath6kl_scan_timeout;
extern unsigned int ath6kl_roam_mode;

#ifdef ATH6KL_HSIC_RECOVER
extern u8 cached_mac[ETH_ALEN];
extern bool cached_mac_valid;
#endif

struct ath6kl_beacon_parameters {
	/* Settings */
	int beacon_interval;
	int dtim_period;
	const u8 *ssid;
	size_t ssid_len;
	enum nl80211_hidden_ssid hidden_ssid;
	struct cfg80211_crypto_settings crypto;
	bool privacy;
	enum nl80211_auth_type auth_type;
	int inactivity_timeout;
	struct ieee80211_channel *channel;	/* After kernel 3.6 */
	enum nl80211_channel_type channel_type;	/* After kernel 3.6 */
	u8 p2p_ctwindow;			/* After kernel 3.8 */
	bool p2p_opp_ps;			/* After kernel 3.8 */
	const struct cfg80211_acl_data *acl;	/* After kernel 3.9 */
	bool radar_required;			/* After kernel 3.9 */

	/* IEs */
	const u8 *head, *tail;
	int head_len, tail_len;
	const u8 *beacon_ies;
	size_t beacon_ies_len;
	const u8 *proberesp_ies;
	size_t proberesp_ies_len;
	const u8 *assocresp_ies;
	size_t assocresp_ies_len;
	const u8 *probe_resp;
	int probe_resp_len;
};

struct net_device *ath6kl_interface_add(struct ath6kl *ar, char *name,
					enum nl80211_iftype type,
					u8 fw_vif_idx, u8 nw_type);
int ath6kl_register_ieee80211_hw(struct ath6kl *ar);
struct ath6kl *ath6kl_core_alloc(struct device *dev);
void ath6kl_deinit_ieee80211_hw(struct ath6kl *ar);

void ath6kl_cfg80211_scan_complete_event(struct ath6kl_vif *vif, bool aborted);

void ath6kl_cfg80211_connect_result(struct ath6kl_vif *vif,
				const u8 *bssid,
				const u8 *req_ie,
				size_t req_ie_len,
				const u8 *resp_ie,
				size_t resp_ie_len,
				u16 status,
				gfp_t gfp);

void ath6kl_cfg80211_disconnected(struct ath6kl_vif *vif,
				u16 reason,
				u8 *ie,
				size_t ie_len,
				gfp_t gfp);

void ath6kl_cfg80211_connect_event(struct ath6kl_vif *vif, u16 channel,
				   u8 *bssid, u16 listen_intvl,
				   u16 beacon_intvl,
				   enum network_type nw_type,
				   u8 beacon_ie_len, u8 assoc_req_len,
				   u8 assoc_resp_len, u8 *assoc_info);

void ath6kl_cfg80211_disconnect_event(struct ath6kl_vif *vif, u8 reason,
				      u8 *bssid, u8 assoc_resp_len,
				      u8 *assoc_info, u16 proto_reason);

void ath6kl_cfg80211_tkip_micerr_event(struct ath6kl_vif *vif, u8 keyid,
				     bool ismcast);

int ath6kl_cfg80211_suspend(struct ath6kl *ar,
			    enum ath6kl_cfg_suspend_mode mode,
			    struct cfg80211_wowlan *wow);

int ath6kl_cfg80211_resume(struct ath6kl *ar);

void ath6kl_cfg80211_stop(struct ath6kl_vif *vif);
void ath6kl_cfg80211_stop_all(struct ath6kl *ar);

#if defined(CONFIG_ANDROID) || defined(USB_AUTO_SUSPEND)
int ath6kl_set_wow_mode(struct wiphy *wiphy, struct cfg80211_wowlan *wow);
int ath6kl_clear_wow_mode(struct wiphy *wiphy);
#endif /* defined(CONFIG_ANDROID) || defined(USB_AUTO_SUSPEND) */

bool ath6kl_sched_scan_trigger(struct ath6kl_vif *vif);

void ath6kl_scan_timer_handler(unsigned long ptr);

void ath6kl_shprotect_timer_handler(unsigned long ptr);

void ath6kl_judge_roam_parameter(struct ath6kl_vif *vif,
				bool call_on_disconnect);

void ath6kl_switch_parameter_based_on_connection(
			struct ath6kl_vif *vif,
			bool call_on_disconnect);

#if defined(USB_AUTO_SUSPEND)
void ath6kl_auto_pm_wakeup_resume(struct ath6kl *wk);
#endif
#endif /* ATH6KL_CFG80211_H */
