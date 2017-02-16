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

#include <linux/moduleparam.h>
#ifndef CE_OLD_KERNEL_SUPPORT_2_6_23
#include <linux/gpio.h>
#endif
#include <linux/interrupt.h>
#include <linux/time.h>
#include <linux/inetdevice.h>
#include "core.h"
#include "cfg80211.h"
#include "debug.h"
#include "hif-ops.h"
#include "testmode.h"
#include "cfg80211_btcoex.h"
#ifdef ATH6KL_DIAGNOSTIC
#include "diagnose.h"
#endif
#include "pm.h"
#include "rttm.h"

unsigned int debug_quirks = ATH6KL_MODULE_DEF_DEBUG_QUIRKS;


module_param(debug_quirks, uint, 0644);

#define RATETAB_ENT(_rate, _rateid, _flags) {   \
	.bitrate    = (_rate),                  \
	.flags      = (_flags),                 \
	.hw_value   = (_rateid),                \
}

#define CHAN2G(_channel, _freq, _flags) {   \
	.band           = IEEE80211_BAND_2GHZ,  \
	.hw_value       = (_channel),           \
	.center_freq    = (_freq),              \
	.flags          = (_flags),             \
	.max_antenna_gain   = 0,                \
	.max_power      = 30,                   \
}

#define CHAN5G(_channel, _flags) {		    \
	.band           = IEEE80211_BAND_5GHZ,      \
	.hw_value       = (_channel),               \
	.center_freq    = 5000 + (5 * (_channel)),  \
	.flags          = (_flags),                 \
	.max_antenna_gain   = 0,                    \
	.max_power      = 30,                       \
}

static struct ieee80211_rate ath6kl_rates[] = {
	RATETAB_ENT(10, 0x1, 0),
	RATETAB_ENT(20, 0x2, 0),
	RATETAB_ENT(55, 0x4, 0),
	RATETAB_ENT(110, 0x8, 0),
	RATETAB_ENT(60, 0x10, 0),
	RATETAB_ENT(90, 0x20, 0),
	RATETAB_ENT(120, 0x40, 0),
	RATETAB_ENT(180, 0x80, 0),
	RATETAB_ENT(240, 0x100, 0),
	RATETAB_ENT(360, 0x200, 0),
	RATETAB_ENT(480, 0x400, 0),
	RATETAB_ENT(540, 0x800, 0),
};

#define ath6kl_a_rates     (ath6kl_rates + 4)
#define ath6kl_a_rates_size    8
#define ath6kl_g_rates     (ath6kl_rates + 0)
#define ath6kl_g_rates_size    12
#define ath6kl_b_rates_size    4

/* 802.1d to AC mapping. Refer pg 57 of WMM-test-plan-v1.2 */
static const u8 up_to_ac[] = {
	WMM_AC_BE,
	WMM_AC_BK,
	WMM_AC_BK,
	WMM_AC_BE,
	WMM_AC_VI,
	WMM_AC_VI,
	WMM_AC_VO,
	WMM_AC_VO,
};

static struct ieee80211_channel ath6kl_2ghz_channels[] = {
	CHAN2G(1, 2412, 0),
	CHAN2G(2, 2417, 0),
	CHAN2G(3, 2422, 0),
	CHAN2G(4, 2427, 0),
	CHAN2G(5, 2432, 0),
	CHAN2G(6, 2437, 0),
	CHAN2G(7, 2442, 0),
	CHAN2G(8, 2447, 0),
	CHAN2G(9, 2452, 0),
	CHAN2G(10, 2457, 0),
	CHAN2G(11, 2462, 0),
	CHAN2G(12, 2467, 0),
	CHAN2G(13, 2472, 0),
	CHAN2G(14, 2484, 0),
};

static struct ieee80211_channel ath6kl_5ghz_a_channels[] = {
	CHAN5G(34, 0), CHAN5G(36, 0),
	CHAN5G(38, 0), CHAN5G(40, 0),
	CHAN5G(42, 0), CHAN5G(44, 0),
	CHAN5G(46, 0), CHAN5G(48, 0),
	CHAN5G(52, 0), CHAN5G(56, 0),
	CHAN5G(60, 0), CHAN5G(64, 0),
	CHAN5G(100, 0), CHAN5G(104, 0),
	CHAN5G(108, 0), CHAN5G(112, 0),
	CHAN5G(116, 0), CHAN5G(120, 0),
	CHAN5G(124, 0), CHAN5G(128, 0),
	CHAN5G(132, 0), CHAN5G(136, 0),
	CHAN5G(140, 0), CHAN5G(144, 0),
	CHAN5G(149, 0),	CHAN5G(153, 0),
	CHAN5G(157, 0),	CHAN5G(161, 0),
	CHAN5G(165, 0),
};

static struct ieee80211_channel ath6kl_dsrc_channels[] = {
	CHAN5G(172,0),
	CHAN5G(174,0),
	CHAN5G(176,0),
	CHAN5G(178,0),
	CHAN5G(180,0),
	CHAN5G(182,0),
	CHAN5G(184,0),
};

static struct ieee80211_supported_band ath6kl_band_2ghz = {
	.n_channels = ARRAY_SIZE(ath6kl_2ghz_channels),
	.channels = ath6kl_2ghz_channels,
	.n_bitrates = ath6kl_g_rates_size,
	.bitrates = ath6kl_g_rates,
	.ht_cap = {
		.ht_supported   = true,
		.cap            = IEEE80211_HT_CAP_SUP_WIDTH_20_40 |
				  IEEE80211_HT_CAP_SGI_20 |
				  IEEE80211_HT_CAP_SGI_40 |
				  IEEE80211_HT_CAP_TX_STBC |
				  0x100 | /* FIXME : One chain RX STBC */
				  IEEE80211_HT_CAP_SM_PS,
		.ampdu_factor   = IEEE80211_HT_MAX_AMPDU_64K,
		.ampdu_density  = IEEE80211_HT_MPDU_DENSITY_8,
		.mcs = {
			.rx_mask = { 0xff, 0xff, 0, 0, 0, 0, 0, 0, 0, 0},
		},
	},
};

static struct ieee80211_supported_band ath6kl_band_5ghz = {
	.n_channels = ARRAY_SIZE(ath6kl_5ghz_a_channels),
	.channels = ath6kl_5ghz_a_channels,
	.n_bitrates = ath6kl_a_rates_size,
	.bitrates = ath6kl_a_rates,
	.ht_cap = {
		.ht_supported   = true,
		.cap            = IEEE80211_HT_CAP_SUP_WIDTH_20_40 |
				  IEEE80211_HT_CAP_SGI_20 |
				  IEEE80211_HT_CAP_SGI_40 |
				  IEEE80211_HT_CAP_TX_STBC |
				  0x100 | /* FIXME : One chain RX STBC */
				  IEEE80211_HT_CAP_SM_PS,
		.ampdu_factor   = IEEE80211_HT_MAX_AMPDU_64K,
		.ampdu_density  = IEEE80211_HT_MPDU_DENSITY_8,
		.mcs = {
			.rx_mask = { 0xff, 0xff, 0, 0, 0, 0, 0, 0, 0, 0},
		},
	},
};

static struct ieee80211_supported_band ath6kl_band_dsrc = {
	.n_channels = ARRAY_SIZE(ath6kl_dsrc_channels),
	.channels = ath6kl_dsrc_channels,
	.n_bitrates = ath6kl_a_rates_size,
	.bitrates = ath6kl_a_rates,
	.ht_cap = {
		.ht_supported   = true,
		.cap            = IEEE80211_HT_CAP_SUP_WIDTH_20_40 |
				  IEEE80211_HT_CAP_SGI_20 |
				  IEEE80211_HT_CAP_SGI_40 |
				  IEEE80211_HT_CAP_TX_STBC |
				  0x100 | /* FIXME : One chain RX STBC */
				  IEEE80211_HT_CAP_SM_PS,
		.ampdu_factor   = IEEE80211_HT_MAX_AMPDU_64K,
		.ampdu_density  = IEEE80211_HT_MPDU_DENSITY_8,
		.mcs = {
			.rx_mask = { 0xff, 0xff, 0, 0, 0, 0, 0, 0, 0, 0},
		},
	},
};

/* Max. 2 devices = 1STA + 1SOFTAP */
static const struct ieee80211_iface_limit ath6kl_limits_sta_ap[] = {
	{
		.max = 1,
		.types = BIT(NL80211_IFTYPE_STATION),
	},
	{
		.max = 1,
		.types = BIT(NL80211_IFTYPE_AP),
	},
};

static struct ieee80211_iface_combination
	ath6kl_iface_combinations_sta_ap[] = {
	{
		.num_different_channels = 1,
		.max_interfaces = 2,
		.limits = ath6kl_limits_sta_ap,
		.n_limits = ARRAY_SIZE(ath6kl_limits_sta_ap),
	},
};

/* Max. 2 devices = 1STA&P2P-DEVICE + 1P2P-GO|P2P-CLIENT */
static const struct ieee80211_iface_limit ath6kl_limits_p2p_concurrent2[] = {
	{
		.max = 2,
		.types = BIT(NL80211_IFTYPE_STATION),
	},
	{
		.max = 1,
		.types = BIT(NL80211_IFTYPE_P2P_CLIENT) |
				BIT(NL80211_IFTYPE_P2P_GO),
	},
};

static struct ieee80211_iface_combination
	ath6kl_iface_combinations_p2p_concurrent2[] = {
	{
		.num_different_channels = 1,
		.max_interfaces = 2,
		.limits = ath6kl_limits_p2p_concurrent2,
		.n_limits = ARRAY_SIZE(ath6kl_limits_p2p_concurrent2),
	},
};

/* Max. 3 devices = 1STA + 1P2P-DEVICE + 1P2P-GO|P2P-CLIENT */
static const struct ieee80211_iface_limit ath6kl_limits_p2p_concurrent3[] = {
	{
		.max = 3,
		.types = BIT(NL80211_IFTYPE_STATION),
	},
	{
		.max = 1,
		.types = BIT(NL80211_IFTYPE_P2P_CLIENT) |
				BIT(NL80211_IFTYPE_P2P_GO),
	},
};

static struct ieee80211_iface_combination
	ath6kl_iface_combinations_p2p_concurrent3[] = {
	{
		.num_different_channels = 1,
		.max_interfaces = 3,
		.limits = ath6kl_limits_p2p_concurrent3,
		.n_limits = ARRAY_SIZE(ath6kl_limits_p2p_concurrent3),
	},
};

/* Max. 4 devices = 1STA + 1P2P-DEVICE + 2P2P-GO|P2P-CLIENT */
static const struct ieee80211_iface_limit ath6kl_limits_p2p_concurrent4[] = {
	{
		.max = 4,
		.types = BIT(NL80211_IFTYPE_STATION),
	},
	{
		.max = 2,
		.types = BIT(NL80211_IFTYPE_P2P_CLIENT) |
				BIT(NL80211_IFTYPE_P2P_GO),
	},
};

static struct ieee80211_iface_combination
	ath6kl_iface_combinations_p2p_concurrent4[] = {
	{
		.num_different_channels = 1,
		.max_interfaces = 4,
		.limits = ath6kl_limits_p2p_concurrent4,
		.n_limits = ARRAY_SIZE(ath6kl_limits_p2p_concurrent4),
	},
};

/* Max. 4 devices = 1STA + 1P2P-DEVICE + 1P2P-GO|P2P-CLIENT + 1SOFTAP */
static const struct ieee80211_iface_limit ath6kl_limits_p2p_concurrent4_1[] = {
	{
		.max = 3,
		.types = BIT(NL80211_IFTYPE_STATION),
	},
	{
		.max = 1,
		.types = BIT(NL80211_IFTYPE_AP),
	},
	{
		.max = 1,
		.types = BIT(NL80211_IFTYPE_P2P_CLIENT) |
				BIT(NL80211_IFTYPE_P2P_GO),
	},
};

static struct ieee80211_iface_combination
	ath6kl_iface_combinations_p2p_concurrent4_1[] = {
	{
		.num_different_channels = 1,
		.max_interfaces = 4,
		.limits = ath6kl_limits_p2p_concurrent4_1,
		.n_limits = ARRAY_SIZE(ath6kl_limits_p2p_concurrent4_1),
	},
};

#define CCKM_KRK_CIPHER_SUITE 0x004096ff /* use for KRK */

#ifdef PMF_SUPPORT
#define WLAN_AKM_SUITE_8021X_SHA256 0x000FAC05
#define WLAN_AKM_SUITE_PSK_SHA256   0x000FAC06

static int ath6kl_set_akm_suites(struct wiphy *wiphy, struct net_device *dev,
				struct cfg80211_connect_params *sme);
static int ath6kl_set_rsn_cap(struct wiphy *wiphy, struct net_device *dev,
				const u8 *ies, int ies_len);
#endif

static void ath6kl_eapol_shprotect_timer_handler(unsigned long ptr);

static int ath6kl_set_wpa_version(struct ath6kl_vif *vif,
				  enum nl80211_wpa_versions wpa_version)
{
	ath6kl_dbg(ATH6KL_DBG_WLAN_CFG, "%s: %u\n", __func__, wpa_version);

	if (!wpa_version) {
		vif->auth_mode = NONE_AUTH;
	} else if (wpa_version & NL80211_WPA_VERSION_2) {
		vif->auth_mode = WPA2_AUTH;
	} else if (wpa_version & NL80211_WPA_VERSION_1) {
		vif->auth_mode = WPA_AUTH;
	} else {
		ath6kl_err("%s: %u not supported\n", __func__, wpa_version);
		return -ENOTSUPP;
	}

	return 0;
}

static int ath6kl_set_auth_type(struct ath6kl_vif *vif,
				enum nl80211_auth_type auth_type)
{
	ath6kl_dbg(ATH6KL_DBG_WLAN_CFG, "%s: 0x%x\n", __func__, auth_type);

	switch (auth_type) {
	case NL80211_AUTHTYPE_OPEN_SYSTEM:
		vif->dot11_auth_mode = OPEN_AUTH;
		break;
	case NL80211_AUTHTYPE_SHARED_KEY:
		vif->dot11_auth_mode = SHARED_AUTH;
		break;
	case NL80211_AUTHTYPE_NETWORK_EAP:
		vif->dot11_auth_mode = LEAP_AUTH;
		break;

	case NL80211_AUTHTYPE_AUTOMATIC:
		vif->dot11_auth_mode = OPEN_AUTH | SHARED_AUTH;
		break;

	default:
		ath6kl_err("%s: 0x%x not spported\n", __func__, auth_type);
		return -ENOTSUPP;
	}

	return 0;
}

static int ath6kl_set_cipher(struct ath6kl_vif *vif, u32 cipher, bool ucast)
{
	u8 *ar_cipher = ucast ? &vif->prwise_crypto : &vif->grp_crypto;
	u8 *ar_cipher_len = ucast ? &vif->prwise_crypto_len :
		&vif->grp_crypto_len;

	ath6kl_dbg(ATH6KL_DBG_WLAN_CFG, "%s: cipher 0x%x, ucast %u\n",
		   __func__, cipher, ucast);

	switch (cipher) {
	case 0:
		/* our own hack to use value 0 as no crypto used */
		*ar_cipher = NONE_CRYPT;
		*ar_cipher_len = 0;
		break;
	case WLAN_CIPHER_SUITE_WEP40:
		*ar_cipher = WEP_CRYPT;
		*ar_cipher_len = 5;
		break;
	case WLAN_CIPHER_SUITE_WEP104:
		*ar_cipher = WEP_CRYPT;
		*ar_cipher_len = 13;
		break;
	case WLAN_CIPHER_SUITE_TKIP:
		*ar_cipher = TKIP_CRYPT;
		*ar_cipher_len = 0;
		break;
	case WLAN_CIPHER_SUITE_CCMP:
		*ar_cipher = AES_CRYPT;
		*ar_cipher_len = 0;
		break;
	case WLAN_CIPHER_SUITE_SMS4:
		*ar_cipher = WAPI_CRYPT;
		*ar_cipher_len = 0;
		break;
	default:
		ath6kl_err("cipher 0x%x not supported\n", cipher);
		return -ENOTSUPP;
	}

	return 0;
}

static void ath6kl_set_key_mgmt(struct ath6kl_vif *vif, u32 key_mgmt)
{
	ath6kl_dbg(ATH6KL_DBG_WLAN_CFG, "%s: 0x%x\n", __func__, key_mgmt);

	if (key_mgmt == WLAN_AKM_SUITE_PSK) {
		if (vif->auth_mode == WPA_AUTH)
			vif->auth_mode = WPA_PSK_AUTH;
		else if (vif->auth_mode == WPA2_AUTH)
			vif->auth_mode = WPA2_PSK_AUTH;
	} else if (key_mgmt == 0x00409600) {
		if (vif->auth_mode == WPA_AUTH)
			vif->auth_mode = WPA_AUTH_CCKM;
		else if (vif->auth_mode == WPA2_AUTH)
			vif->auth_mode = WPA2_AUTH_CCKM;
#ifdef PMF_SUPPORT
	} else if (key_mgmt == WLAN_AKM_SUITE_PSK_SHA256) {
		vif->auth_mode = WPA2_PSK_SHA256_AUTH;
	} else if ((key_mgmt != WLAN_AKM_SUITE_8021X) &&
		(key_mgmt != WLAN_AKM_SUITE_8021X_SHA256)) {
#else
	} else if (key_mgmt != WLAN_AKM_SUITE_8021X) {
#endif
		vif->auth_mode = NONE_AUTH;
	}
}

#ifdef ATH6KL_SUPPORT_WIFI_KTK
static void ath6kl_install_ktk_ptk(struct ath6kl_vif *vif)
{
	struct ath6kl *ar = vif->ar;
	struct ath6kl_key *ptk = NULL;
	enum wmi_sync_flag sync_flag = SYNC_BOTH_WMIFLAG;
	int status;
	char buff[32];

	ath6kl_dbg(ATH6KL_DBG_WLAN_CFG, "%s\n", __func__);

	memset(buff, 0, 32);
	memcpy(buff, ar->ktk_passphrase, 16);

	/* set ptk at index 0 */
	ptk = &vif->keys[0];
	memset(ptk, 0, sizeof(struct ath6kl_key));

	ptk->key_len = 16;
	memcpy(ptk->key, ar->ktk_passphrase, ptk->key_len);
	ptk->seq_len = 6;
	memset(ptk->seq, 0, ptk->seq_len);

	if (ath6kl_mod_debug_quirks(ar, ATH6KL_MODULE_DISABLE_WMI_SYC))
		sync_flag = NO_SYNC_WMIFLAG;

	status = ath6kl_wmi_addkey_cmd(ar->wmi, vif->fw_vif_idx, 0,
					KTK_CRYPT, GROUP_USAGE | TX_USAGE,
					ptk->key_len,
					ptk->seq,
					ptk->seq_len,
					ptk->key,
					KEY_OP_INIT_VAL,
					NULL,
					sync_flag);

	if (status)
		ath6kl_err("%s: set ptk at index 0 failed\n", __func__);
}
#endif

static bool __ath6kl_cfg80211_ready(struct ath6kl *ar)
{
	if (!test_bit(WMI_READY, &ar->flag)) {
		ath6kl_err("wmi is not ready\n");
		return false;
	}

	return true;
}

static bool ath6kl_cfg80211_ready(struct ath6kl_vif *vif)
{
	if (!__ath6kl_cfg80211_ready(vif->ar))
		return false;

#if defined(USB_AUTO_SUSPEND)
	if ((vif->ar->state == ATH6KL_STATE_WOW) ||
		(vif->ar->state == ATH6KL_STATE_DEEPSLEEP) ||
		(vif->ar->state == ATH6KL_STATE_PRE_SUSPEND)) {
		ath6kl_dbg(ATH6KL_DBG_WLAN_CFG,
		"ignore wlan disabled in AUTO suspend mode!\n");
	} else {
		if (!test_bit(WLAN_ENABLED, &vif->flags))
			return false;
	}
#else
	if (!test_bit(WLAN_ENABLED, &vif->flags))
		return false;
#endif

	return true;
}

static bool ath6kl_is_wpa_ie(const u8 *pos)
{
	return pos[0] == WLAN_EID_VENDOR_SPECIFIC && pos[1] >= 4 &&
		pos[2] == 0x00 && pos[3] == 0x50 &&
		pos[4] == 0xf2 && pos[5] == 0x01;
}

static bool ath6kl_is_rsn_ie(const u8 *pos)
{
	return pos[0] == WLAN_EID_RSN;
}

static bool ath6kl_is_wps_ie(const u8 *pos)
{
	return (pos[0] == WLAN_EID_VENDOR_SPECIFIC &&
		pos[1] >= 4 &&
		pos[2] == 0x00 && pos[3] == 0x50 && pos[4] == 0xf2 &&
		pos[5] == 0x04);
}

static bool ath6kl_is_wmm_ie(const u8 *pos)
{
	return (pos[0] == WLAN_EID_VENDOR_SPECIFIC &&
		pos[1] >= 4 &&
		pos[2] == 0x00 && pos[3] == 0x50 && pos[4] == 0xf2 &&
		pos[5] == 0x02);
}

static int ath6kl_set_assoc_req_ies(struct ath6kl_vif *vif, const u8 *ies,
				    size_t ies_len)
{
	struct ath6kl *ar = vif->ar;
	const u8 *pos;
	u8 *buf = NULL;
	size_t len = 0;
	int ret;

	/*
	 * Clear previously set flag
	 */

	vif->connect_ctrl_flags &= ~CONNECT_WPS_FLAG;

	/*
	 * Filter out RSN/WPA or P2P/WFD IE(s)
	 */
	if (ies && ies_len) {
		buf = kmalloc(ies_len, GFP_KERNEL);
		if (buf == NULL)
			return -ENOMEM;
		pos = ies;

		while (pos + 1 < ies + ies_len) {
			if (pos + 2 + pos[1] > ies + ies_len)
				break;
			if (!(ath6kl_is_wpa_ie(pos) ||
			      ath6kl_is_rsn_ie(pos))) {
				if ((ath6kl_is_p2p_ie(pos) ||
				     ath6kl_is_wfd_ie(pos)) &&
				    !ath6kl_p2p_ie_append(vif,
							P2P_IE_IN_ASSOC_REQ))
					ath6kl_dbg(ATH6KL_DBG_WLAN_CFG,
						"Remove Asso's P2P IE\n");
				else {
					memcpy(buf + len, pos, 2 + pos[1]);
					len += 2 + pos[1];
				}
			}

			if (ath6kl_is_wps_ie(pos))
				vif->connect_ctrl_flags |= CONNECT_WPS_FLAG;

			pos += 2 + pos[1];
		}
	}

	ret = ath6kl_wmi_set_appie_cmd(ar->wmi, vif->fw_vif_idx,
				       WMI_FRAME_ASSOC_REQ, buf, len);
	kfree(buf);
	return ret;
}

static int ath6kl_nliftype_to_drv_iftype(enum nl80211_iftype type, u8 *nw_type)
{
	switch (type) {
	case NL80211_IFTYPE_STATION:
		*nw_type = INFRA_NETWORK;
		break;
	case NL80211_IFTYPE_ADHOC:
		*nw_type = ADHOC_NETWORK;
		break;
	case NL80211_IFTYPE_AP:
		*nw_type = AP_NETWORK;
		break;
	case NL80211_IFTYPE_P2P_CLIENT:
		*nw_type = INFRA_NETWORK;
		break;
	case NL80211_IFTYPE_P2P_GO:
		*nw_type = AP_NETWORK;
		break;
	default:
		ath6kl_err("invalid interface type %u\n", type);
		return -ENOTSUPP;
	}

	return 0;
}

static bool ath6kl_is_valid_iftype(struct ath6kl *ar, enum nl80211_iftype type,
				   u8 *if_idx, u8 *nw_type)
{
	int i;

	if (ath6kl_nliftype_to_drv_iftype(type, nw_type))
		return false;

	if (ar->ibss_if_active || ((type == NL80211_IFTYPE_ADHOC) &&
	    ar->num_vif))
		return false;

	/* we use firmware index in 1, 4, 3, 2 order to align with
	 * firmware design in the dedicate interface case
	 */
	if (ar->p2p_concurrent && ar->p2p_dedicate) {
		if ((ar->p2p_concurrent_ap) &&
			(type == NL80211_IFTYPE_AP)) {
			for (i = 0; i < ar->max_norm_iface; i++) {
				if ((ar->avail_idx_map >> i) & BIT(0)) {
					*if_idx = i;
					return true;
				}
			}
			return false;
		}

		if (type == NL80211_IFTYPE_STATION ||
			type == NL80211_IFTYPE_AP ||
			type == NL80211_IFTYPE_ADHOC) {
			for (i = (ar->vif_max - 1); i > 0; i--) {
				if ((ar->avail_idx_map >> i) & BIT(0)) {
					*if_idx = i;
					return true;
				}
			}
		}

		if (type == NL80211_IFTYPE_P2P_CLIENT ||
			type == NL80211_IFTYPE_P2P_GO) {
			for (i = (ar->vif_max - 1); i >
				(ar->max_norm_iface - 1); i--) {
				if ((ar->avail_idx_map >> i) & BIT(0)) {
					*if_idx = i;
					return true;
				}
			}
		}
	} else {
		if (type == NL80211_IFTYPE_STATION ||
		type == NL80211_IFTYPE_AP || type == NL80211_IFTYPE_ADHOC) {
			for (i = 0; i < ar->vif_max; i++) {
				if ((ar->avail_idx_map >> i) & BIT(0)) {
					*if_idx = i;
					return true;
				}
			}
		}

		if (type == NL80211_IFTYPE_P2P_CLIENT ||
		type == NL80211_IFTYPE_P2P_GO) {
			for (i = ar->max_norm_iface; i < ar->vif_max; i++) {
				if ((ar->avail_idx_map >> i) & BIT(0)) {
					*if_idx = i;
					return true;
				}
			}
		}

	}
	return false;
}

/* avoid using roam in p2p, ap, adhoc mode, and  current USB devices  */
/* use in connect_cmd, disconnect event, ap_start */
/* only support lrssi roam at this time*/
void ath6kl_judge_roam_parameter(
			struct ath6kl_vif *vif,
			bool call_on_disconnect)
{
	struct ath6kl *ar = vif->ar;
	u8 connected_count = 0;
	bool lrssi_scan_enable = true;
	struct ath6kl_vif *vif_temp;

	if (ar->roam_mode == ATH6KL_MODULEROAM_DISABLE ||
		ar->roam_mode == ATH6KL_MODULEROAM_DISABLE_LRSSI_SCAN) {
		lrssi_scan_enable = false;
		goto DONE;
	} else if (ar->roam_mode == ATH6KL_MODULEROAM_NO_LRSSI_SCAN_AT_MULTI) {

		lrssi_scan_enable = true;
		list_for_each_entry(vif_temp, &ar->vif_list, list) {
			if (vif->fw_vif_idx == vif_temp->fw_vif_idx)
				continue;
			if (test_bit(CONNECTED, &vif_temp->flags))
				connected_count++;

			if (call_on_disconnect) {
				if ((connected_count == 1) &&
					(vif_temp->wdev.iftype ==
						NL80211_IFTYPE_STATION)) {
					lrssi_scan_enable = true;
				} else {
					lrssi_scan_enable = false;
				}
			} else if (connected_count) {
				lrssi_scan_enable = false;
				goto DONE;
			}
		}
	} else { /* ATH6KL_MODULEROAM_ENABLE_ALL */
		lrssi_scan_enable = true;
	}

	/* disable roam when it is in any other mode */
	if (!call_on_disconnect &&
		vif->wdev.iftype != NL80211_IFTYPE_STATION)
		lrssi_scan_enable = false;

DONE:
	if (lrssi_scan_enable) {
		ath6kl_wmi_set_roam_ctrl_cmd(ar->wmi,
				0,
				ar->low_rssi_roam_params.lrssi_scan_period,
				ar->low_rssi_roam_params.lrssi_scan_threshold,
				ar->low_rssi_roam_params.lrssi_roam_threshold,
				ar->low_rssi_roam_params.roam_rssi_floor);
	} else {
		ath6kl_wmi_set_roam_ctrl_cmd(ar->wmi,
			0, 0xFFFF, 0, 0, 100);
	}

}

static void switch_tid_rx_timeout(
		struct ath6kl_vif *vif,
		bool mcc)
{
	u8 i, j = 0;
	struct aggr_conn_info *aggr_conn;
	struct rxtid *rxtid;

	for (i = 0; i < AP_MAX_NUM_STA; i++) {
		aggr_conn = vif->sta_list[i].aggr_conn_cntxt;
		for (j = 0; j < NUM_OF_TIDS; j++) {
			rxtid = AGGR_GET_RXTID(aggr_conn, j);
			spin_lock_bh(&rxtid->lock);
			switch (up_to_ac[j]) {
			case WMM_AC_BK:
			case WMM_AC_BE:
			case WMM_AC_VI:
				if (mcc)
					aggr_conn->tid_timeout_setting[j] =
						MCC_AGGR_RX_TIMEOUT;
				else
					aggr_conn->tid_timeout_setting[j] =
						AGGR_RX_TIMEOUT;
				break;
			case WMM_AC_VO:
				if (mcc)
					aggr_conn->tid_timeout_setting[j] =
						MCC_AGGR_RX_TIMEOUT_VO;
				else
					aggr_conn->tid_timeout_setting[j] =
						AGGR_RX_TIMEOUT_VO;
				break;
			}
			spin_unlock_bh(&rxtid->lock);
		}
	}
}

#ifdef USB_AUTO_SUSPEND
void ath6kl_check_autopm_onoff(struct ath6kl *ar)
{
	struct ath6kl_vif *vif_temp;
	/*
	 * Switch Auto PM On/Off
	 * In folloing case, we will turn off AUTOPM
	 * 1. p2p0-P2P-xxx is connected
	 * 2. any nw_type is AP network
	 */
	int vif_cnt = 0;
	int autopm_turn_on = 1;
	list_for_each_entry(vif_temp, &ar->vif_list, list) {
		vif_cnt++;
		if (test_bit(CONNECTED, &vif_temp->flags)) {
			if (vif_cnt > 1) {
				autopm_turn_on = 0;
				break;
			}
			if (vif_temp->nw_type == AP_NETWORK) {
				autopm_turn_on = 0;
				break;
			}
		}
	}
	if (autopm_turn_on)
		ath6kl_hif_auto_pm_turnon(ar);
	else
		ath6kl_hif_auto_pm_turnoff(ar);
}
#endif

/* This function is used by sta/p2pclient/go interface to switch paramters
  * needed for MCC/connection specific parameters
  */
void ath6kl_switch_parameter_based_on_connection(
			struct ath6kl_vif *vif,
			bool call_on_disconnect)
{
	struct ath6kl *ar = vif->ar;
	u8 connected_count = 0;
	struct ath6kl_vif *vif_temp;
	bool mcc = false;
	u16 pre_vifch = 0;

	list_for_each_entry(vif_temp, &ar->vif_list, list) {
		if (test_bit(CONNECTED, &vif_temp->flags)) {
			connected_count++;
			if (pre_vifch == 0) {
				pre_vifch = vif_temp->bss_ch;
			} else if (vif_temp->bss_ch != 0 &&
						pre_vifch != vif_temp->bss_ch) {
				mcc = true;
			}
		}
	}

	if (connected_count) {
		list_for_each_entry(vif_temp, &ar->vif_list, list) {
			if (test_bit(CONNECTED, &vif_temp->flags)) {
				if (call_on_disconnect &&
					vif->fw_vif_idx == vif_temp->fw_vif_idx)
					continue;
				switch_tid_rx_timeout(vif_temp, mcc);
			}
		}
	}


	/*
	 * Delete rekey protection timer if
	 * 1. all vifs disconnected
	 * 2. the timer was only fireed by this vif.
	 */
	if ((call_on_disconnect) &&
	    test_bit(EAPOL_HANDSHAKE_PROTECT, &ar->flag)) {
		if ((connected_count == 0) ||
		    (ar->eapol_shprotect_vif == (1 << vif->fw_vif_idx))) {
			clear_bit(EAPOL_HANDSHAKE_PROTECT, &ar->flag);
			del_timer(&ar->eapol_shprotect_timer);
		}
	}

	/*
	 * Switch scan parameter
	 * Currently, when there are more than one vif connected,
	 * revise the passive dwell time as the default
	 * ATH6KL_SCAN_PAS_DEWELL_TIME
	 */

	if (connected_count) {
		list_for_each_entry(vif_temp, &ar->vif_list, list) {
			vif_temp->sc_params.pas_chdwell_time =
				ATH6KL_SCAN_PAS_DEWELL_TIME;
		}
	} else {
		list_for_each_entry(vif_temp, &ar->vif_list, list) {
			memcpy(&vif->sc_params,
				&vif->sc_params_default,
				sizeof(struct wmi_scan_params_cmd));
		}
	}

	if (mcc)
		set_bit(MCC_ENABLED, &ar->flag);
	else
		clear_bit(MCC_ENABLED, &ar->flag);

	if (ar->conf_flags & ATH6KL_CONF_ENABLE_FLOWCTRL) {
		if (ar->conf_flags & ATH6KL_CONF_DISABLE_SKIP_FLOWCTRL) {
			clear_bit(SKIP_FLOWCTRL_EVENT, &ar->flag);
		} else {
			if (test_bit(MCC_ENABLED, &ar->flag))
				clear_bit(SKIP_FLOWCTRL_EVENT, &ar->flag);
			else
				set_bit(SKIP_FLOWCTRL_EVENT, &ar->flag);
		}
	} else {
		clear_bit(SKIP_FLOWCTRL_EVENT, &ar->flag);
	}

	if (mcc) {
		list_for_each_entry(vif_temp, &ar->vif_list, list) {
			if (test_bit(CONNECTED, &vif_temp->flags)) {
				if (call_on_disconnect &&
					vif->fw_vif_idx == vif_temp->fw_vif_idx)
					continue;
				if (vif_temp->nw_type == AP_NETWORK) {
					ath6kl_ap_keepalive_config(vif_temp,
						ATH6KL_AP_KA_INTERVAL_DEFAULT,
						ATH6KL_AP_KA_RECLAIM_CYCLE_MCC);
				}
			}
		}
	} else {
		list_for_each_entry(vif_temp, &ar->vif_list, list) {
			if (test_bit(CONNECTED, &vif_temp->flags)) {
				if (call_on_disconnect &&
					vif->fw_vif_idx == vif_temp->fw_vif_idx)
					continue;
				if (vif_temp->nw_type == AP_NETWORK) {
					ath6kl_ap_keepalive_config(vif_temp,
						ATH6KL_AP_KA_INTERVAL_DEFAULT,
						ATH6KL_AP_KA_RECLAIM_CYCLE_SCC);
				}
			}
		}
	}
	
	/* Update HIF queue policy */
	ath6kl_hif_pipe_set_max_queue_number(ar, mcc);

	/* Reconfigurate the PS mode case by case. */
	ath6kl_p2p_reconfig_ps(ar, mcc, call_on_disconnect);

#ifdef USB_AUTO_SUSPEND
	ath6kl_check_autopm_onoff(ar);
#endif /* USB_AUTO_SUSPEND */
}

static inline void _change_sta_scan_plan(struct ath6kl_vif *vif)
{
	if (vif->ssid_len) {
		int i, chan_num;
		u16 chan_list[64];	/* WMI_MAX_CHANNELS */

		memset(chan_list, 0, sizeof(u16) * 64);
		chan_num = ath6kl_bss_post_proc_candidate_bss(vif,
								vif->ssid,
								vif->ssid_len,
								chan_list);

		if (chan_num) {
			vif->scan_plan.type = ATH6KL_SCAN_PLAN_HOST_ORDER;

			/* For STA, prefer to use 5G APs. */
			if (chan_num <= 21) {	/* (64 / 3) */
				vif->scan_plan.numChan = chan_num * 3;

				for (i = 0; i < chan_num; i++)
					vif->scan_plan.chanList[i * 3] =
					vif->scan_plan.chanList[i * 3 + 1] =
					vif->scan_plan.chanList[i * 3 + 2] =
						chan_list[chan_num - 1 - i];
			} else {
				vif->scan_plan.numChan = chan_num;

				for (i = 0; i < chan_num; i++)
					vif->scan_plan.chanList[i] =
						chan_list[chan_num - 1 - i];
			}

			/*
			 * If only one channel need to try and 1s' disconnection
			 * timeout is enough.
			 */
			if (chan_num == 1)
				ath6kl_wmi_disctimeout_cmd(vif->ar->wmi,
							vif->fw_vif_idx,
							1);
		} else
			vif->scan_plan.type = ATH6KL_SCAN_PLAN_REVERSE_ORDER;
	} else
		vif->scan_plan.type = ATH6KL_SCAN_PLAN_REVERSE_ORDER;

	return;
}

static inline void _change_p2p_scan_plan(struct ath6kl_vif *vif)
{
#define _MAX_STAY_TIME		(200)	/* 2 Beacon Time */
	struct ath6kl *ar = vif->ar;
	struct ath6kl_vif *tmp;
	bool change_scan_plan = true;
	int i;

	/*
	 * Only fine tune for SCC case to stay the target channel
	 * at least 2 beacon time for connection try rather than report
	 * disconnect immediately.
	 */

	list_for_each_entry(tmp, &ar->vif_list, list) {
		if ((tmp != vif) &&
		    test_bit(CONNECTED, &tmp->flags) &&
		    (tmp->bss_ch != vif->ch_hint)) {
			change_scan_plan = false;
			break;
		}
	}

	if (change_scan_plan) {
		vif->scan_plan.type = ATH6KL_SCAN_PLAN_HOST_ORDER;
		vif->scan_plan.numChan = _MAX_STAY_TIME /
					 vif->sc_params.maxact_chdwell_time;

		for (i = 0; i < vif->scan_plan.numChan; i++)
			vif->scan_plan.chanList[i] = vif->ch_hint;
	}

	return;
#undef _MAX_STAY_TIME
}

void ath6kl_change_scan_plan(struct ath6kl_vif *vif, bool reset)
{
	struct ath6kl *ar = vif->ar;

	/* Default */
	vif->scan_plan.type = ATH6KL_SCAN_PLAN_REVERSE_ORDER;
	vif->scan_plan.numChan = 0;

	if (!reset) {
		/*
		 * Currently, only fine tune scan-plan when turn-on target
		 * roaming.
		 */
		if (!(ar->wiphy->flags & WIPHY_FLAG_SUPPORTS_FW_ROAM))
			goto done;

		if (vif->wdev.iftype == NL80211_IFTYPE_STATION)
			_change_sta_scan_plan(vif);
		else if ((vif->wdev.iftype == NL80211_IFTYPE_P2P_CLIENT) &&
			   (vif->ssid_len)) {
			struct cfg80211_bss *bss;

			/*
			 * Firmware roaming is global setting in cfg80211 but
			 * not in target and P2P-Client actually no need
			 * roaming and therefore pass the channel & bssid hint
			 * to target.
			 */
			bss = ath6kl_bss_get(vif->ar,
						NULL,
						NULL,
						vif->ssid,
						vif->ssid_len,
						WLAN_CAPABILITY_ESS,
						WLAN_CAPABILITY_ESS);

			if (bss && bss->channel) {
#ifdef CFG80211_SAFE_BSS_INFO_ACCESS
				rcu_read_lock();
#endif
				vif->ch_hint = bss->channel->center_freq;
				memcpy(vif->req_bssid, bss->bssid, ETH_ALEN);
#ifdef CFG80211_SAFE_BSS_INFO_ACCESS
				rcu_read_unlock();
#endif
			}

			if (bss)
				ath6kl_bss_put(vif->ar, bss);

			/* Change scan-plan to host order if need. */
			_change_p2p_scan_plan(vif);
		}
	}

done:
	ath6kl_dbg(ATH6KL_DBG_WLAN_CFG | ATH6KL_DBG_EXT_SCAN,
		   "change scan plan, vif %d reset %d type %d numChan %d\n",
		   vif->fw_vif_idx,
		   reset,
		   vif->scan_plan.type,
		   vif->scan_plan.numChan);

	/* Update to the target */
	ath6kl_wmi_set_scan_chan_plan(ar->wmi,
					vif->fw_vif_idx,
					vif->scan_plan.type,
					vif->scan_plan.numChan,
					vif->scan_plan.chanList);

	return;
}

static int ath6kl_cfg80211_connect(struct wiphy *wiphy, struct net_device *dev,
				   struct cfg80211_connect_params *sme)
{
	struct ath6kl *ar = ath6kl_priv(dev);
	struct ath6kl_vif *vif = netdev_priv(dev);
	int status, left;

	if (!ath6kl_cfg80211_ready(vif))
		return -EIO;

	if (test_bit(DESTROY_IN_PROGRESS, &ar->flag)) {
		ath6kl_err("destroy in progress\n");
		return -EBUSY;
	}

	if (test_bit(SKIP_SCAN, &ar->flag) &&
	    ((sme->channel && sme->channel->center_freq == 0) ||
	     (sme->bssid && is_zero_ether_addr(sme->bssid)))) {
		ath6kl_err("SkipScan: channel or bssid invalid\n");
		return -EINVAL;
	}

	if (down_interruptible(&ar->sem)) {
		ath6kl_err("busy, couldn't get access\n");
		return -ERESTARTSYS;
	}

	vif->sme_state = SME_CONNECTING;

	if (ar->tx_pending[ath6kl_wmi_get_control_ep(ar->wmi)]) {
		/*
		 * sleep until the command queue drains
		 */
		left = wait_event_interruptible_timeout(ar->event_wq,
			ar->tx_pending[ath6kl_wmi_get_control_ep(ar->wmi)] == 0,
			WMI_TIMEOUT);
		if (left == 0) {
			ath6kl_warn("clear wmi ctrl data timeout connect\n");
			up(&ar->sem);
			return -ETIMEDOUT;
		} else if (signal_pending(current)) {
			ath6kl_err("cmd queue drain timeout\n");
			up(&ar->sem);
			return -EINTR;
		}
	}

	/* Diable background scan */
	vif->sc_params.bg_period = 0xFFFF;
	ath6kl_wmi_scanparams_cmd(ar->wmi, vif->fw_vif_idx,
				vif->sc_params.fg_start_period,
				vif->sc_params.fg_end_period,
				vif->sc_params.bg_period,
				vif->sc_params.minact_chdwell_time,
				vif->sc_params.maxact_chdwell_time,
				vif->sc_params.pas_chdwell_time,
				vif->sc_params.short_scan_ratio,
				vif->sc_params.scan_ctrl_flags,
				vif->sc_params.max_dfsch_act_time,
				vif->sc_params.maxact_scan_per_ssid);

	ath6kl_judge_roam_parameter(vif, false);

	if (vif->wdev.iftype == NL80211_IFTYPE_STATION)
		ath6kl_wmi_set_green_tx_params(ar->wmi, &ar->green_tx_params);

	if (ath6kl_wmi_set_rate_ctrl_cmd(ar->wmi,
				vif->fw_vif_idx, RATECTRL_MODE_PERONLY))
		ath6kl_err("set rate_ctrl failed\n");

	if (sme->ie && (sme->ie_len > 0)) {
		status = ath6kl_set_assoc_req_ies(vif, sme->ie, sme->ie_len);
		if (status) {
			up(&ar->sem);
			return status;
		}
	} else
		vif->connect_ctrl_flags &= ~CONNECT_WPS_FLAG;

#ifdef PMF_SUPPORT
	/* Update AKM suites. */
	if (ath6kl_set_akm_suites(wiphy, dev, sme)) {
		up(&ar->sem);
		return -EIO;
	}

	/* Update RSN Capabilities. */
	if (ath6kl_set_rsn_cap(wiphy, dev, sme->ie, sme->ie_len)) {
		up(&ar->sem);
		return -EIO;
	}
#endif

	vif->connect_ctrl_flags |= CONNECT_IGNORE_WPAx_GROUP_CIPHER;

	if (test_bit(CONNECTED, &vif->flags) &&
	    vif->ssid_len == sme->ssid_len &&
	    !memcmp(vif->ssid, sme->ssid, vif->ssid_len)) {
		vif->reconnect_flag = true;
		status = ath6kl_wmi_reconnect_cmd(ar->wmi, vif->fw_vif_idx,
						  vif->req_bssid,
						  vif->ch_hint);

		up(&ar->sem);
		if (status) {
			ath6kl_err("wmi_reconnect_cmd failed\n");
			return -EIO;
		}
		return 0;
	} else if (vif->ssid_len == sme->ssid_len &&
		   !memcmp(vif->ssid, sme->ssid, vif->ssid_len)) {
		ath6kl_disconnect(vif);
	}

	memset(vif->ssid, 0, sizeof(vif->ssid));
	vif->ssid_len = sme->ssid_len;
	memcpy(vif->ssid, sme->ssid, sme->ssid_len);

	if (sme->channel)
		vif->ch_hint = sme->channel->center_freq;

	memset(vif->req_bssid, 0, sizeof(vif->req_bssid));
	if (sme->bssid && !is_broadcast_ether_addr(sme->bssid))
		memcpy(vif->req_bssid, sme->bssid, sizeof(vif->req_bssid));

	ath6kl_set_wpa_version(vif, sme->crypto.wpa_versions);

	status = ath6kl_set_auth_type(vif, sme->auth_type);
	if (status) {
		up(&ar->sem);
		return status;
	}

	if (sme->crypto.n_ciphers_pairwise)
		ath6kl_set_cipher(vif, sme->crypto.ciphers_pairwise[0], true);
	else
		ath6kl_set_cipher(vif, 0, true);

	ath6kl_set_cipher(vif, sme->crypto.cipher_group, false);

	if (sme->crypto.n_akm_suites)
		ath6kl_set_key_mgmt(vif, sme->crypto.akm_suites[0]);

	if ((sme->key_len) &&
	    (vif->auth_mode == NONE_AUTH) &&
	    (vif->prwise_crypto == WEP_CRYPT)) {
		struct ath6kl_key *key = NULL;

		if (sme->key_idx > WMI_MAX_KEY_INDEX) {
			ath6kl_err("key index %d out of bounds\n",
				   sme->key_idx);
			up(&ar->sem);
			return -ENOENT;
		}

		key = &vif->keys[sme->key_idx];
		key->key_len = sme->key_len;
		memcpy(key->key, sme->key, key->key_len);
		key->cipher = vif->prwise_crypto;
		vif->def_txkey_index = sme->key_idx;

		ath6kl_wmi_addkey_cmd(ar->wmi, vif->fw_vif_idx, sme->key_idx,
				      vif->prwise_crypto,
				      GROUP_USAGE | TX_USAGE,
				      key->key_len,
				      NULL, 0,
				      key->key, KEY_OP_INIT_VAL, NULL,
				      NO_SYNC_WMIFLAG);
	}

	if (!vif->usr_bss_filter) {
		clear_bit(CLEAR_BSSFILTER_ON_BEACON, &vif->flags);
		if (ath6kl_wmi_bssfilter_cmd(ar->wmi, vif->fw_vif_idx,
		    ALL_BSS_FILTER, 0) != 0) {
			ath6kl_err("couldn't set bss filtering\n");
			up(&ar->sem);
			return -EIO;
		}
	}

	vif->nw_type = vif->next_mode;

	ath6kl_dbg(ATH6KL_DBG_WLAN_CFG,
		   "%s: connect called with authmode %d dot11 auth %d"
		   " PW crypto %d PW crypto len %d GRP crypto %d"
		   " GRP crypto len %d channel hint %u\n",
		   __func__,
		   vif->auth_mode, vif->dot11_auth_mode, vif->prwise_crypto,
		   vif->prwise_crypto_len, vif->grp_crypto,
		   vif->grp_crypto_len, vif->ch_hint);

	vif->reconnect_flag = 0;

	/*
	 * Set disconnection timeout to 0 to cause firmware report
	 * disconnection event immediately rather than waiting defualt timer
	 * timeout (10sec) or supplicant trigger connection timeout (10sec).
	 */
	ath6kl_wmi_disctimeout_cmd(ar->wmi, vif->fw_vif_idx, 0);

	ath6kl_change_scan_plan(vif, false);

	status = ath6kl_wmi_connect_cmd(ar->wmi, vif->fw_vif_idx, vif->nw_type,
					vif->dot11_auth_mode, vif->auth_mode,
					vif->prwise_crypto,
					vif->prwise_crypto_len,
					vif->grp_crypto, vif->grp_crypto_len,
					vif->ssid_len, vif->ssid,
					vif->req_bssid, vif->ch_hint,
					vif->connect_ctrl_flags);

	up(&ar->sem);

	if (status == -EINVAL) {
		memset(vif->ssid, 0, sizeof(vif->ssid));
		vif->ssid_len = 0;
		ath6kl_err("invalid request\n");
		return -ENOENT;
	} else if (status) {
		ath6kl_err("ath6kl_wmi_connect_cmd failed\n");
		return -EIO;
	}

	if ((!(vif->connect_ctrl_flags & CONNECT_DO_WPA_OFFLOAD)) &&
	    ((vif->auth_mode == WPA_PSK_AUTH)
	     || (vif->auth_mode == WPA2_PSK_AUTH))) {
		mod_timer(&vif->disconnect_timer,
			  jiffies + msecs_to_jiffies(DISCON_TIMER_INTVAL));
	}

	vif->connect_ctrl_flags &= ~CONNECT_DO_WPA_OFFLOAD;
	set_bit(CONNECT_PEND, &vif->flags);

	return 0;
}

static struct cfg80211_bss *ath6kl_add_bss_if_needed(struct ath6kl_vif *vif,
				    enum network_type nw_type,
				    const u8 *bssid,
				    struct ieee80211_channel *chan,
				    const u8 *beacon_ie, size_t beacon_ie_len)
{
	struct ath6kl *ar = vif->ar;
	struct cfg80211_bss *bss;
	u16 cap_mask, cap_val;
	u8 *ie;

	if (nw_type & ADHOC_NETWORK) {
		cap_mask = WLAN_CAPABILITY_IBSS;
		cap_val = WLAN_CAPABILITY_IBSS;
	} else {
		cap_mask = WLAN_CAPABILITY_ESS;
		cap_val = WLAN_CAPABILITY_ESS;
	}

	bss = ath6kl_bss_get(ar, chan, bssid,
			       vif->ssid, vif->ssid_len,
			       cap_mask, cap_val);
	if (bss == NULL) {
		/*
		 * Since cfg80211 may not yet know about the BSS,
		 * generate a partial entry until the first BSS info
		 * event becomes available.
		 *
		 * Prepend SSID element since it is not included in the Beacon
		 * IEs from the target.
		 */
		ie = kmalloc(2 + vif->ssid_len + beacon_ie_len, GFP_KERNEL);
		if (ie == NULL)
			return NULL;
		ie[0] = WLAN_EID_SSID;
		ie[1] = vif->ssid_len;
		memcpy(ie + 2, vif->ssid, vif->ssid_len);
		memcpy(ie + 2 + vif->ssid_len, beacon_ie, beacon_ie_len);
		bss = cfg80211_inform_bss(ar->wiphy, chan,
					  bssid, 0, cap_val, 100,
					  ie, 2 + vif->ssid_len + beacon_ie_len,
					  0, GFP_KERNEL);
		if (bss)
			ath6kl_dbg(ATH6KL_DBG_WLAN_CFG, "added bss %pM to "
				   "cfg80211\n", bssid);
		kfree(ie);
	} else
		ath6kl_dbg(ATH6KL_DBG_WLAN_CFG, "cfg80211 already has a bss\n");

	return bss;
}

static bool ath6kl_roamed_indicate(struct ath6kl_vif *vif,
				u8 *bssid, bool reset_hk_prot)
{
	struct ath6kl *ar = vif->ar;
	if ((vif->sme_state == SME_CONNECTED) &&
		(ar->wiphy->flags & WIPHY_FLAG_SUPPORTS_FW_ROAM) &&
		((!test_bit(CONNECT_HANDSHAKE_PROTECT, &vif->flags)) ||
			(reset_hk_prot == true))) {

		ath6kl_dbg(ATH6KL_DBG_WLAN_CFG, "%s: %pM roam, vif->flags 0x%lu,"
			"reset_hk_prot %x\n",
			__func__, bssid, vif->flags, reset_hk_prot);
		return true;
	}
	ath6kl_dbg(ATH6KL_DBG_WLAN_CFG, "%s: %pM notroam\n", __func__, bssid);
	return false;
}

static bool ath6kl_handshake_protect(struct ath6kl_vif *vif, u8 *bssid)
{
	bool reset_handshake_pro = false;

	if ((vif->auth_mode > NONE_AUTH) &&
		(vif->prwise_crypto > WEP_CRYPT) &&
		(vif->nw_type == INFRA_NETWORK)) {

		if ((memcmp(bssid, vif->bssid, ETH_ALEN) != 0) ||
			(!test_bit(CONNECT_HANDSHAKE_PROTECT, &vif->flags))) {
			spin_lock_bh(&vif->if_lock);
			set_bit(CONNECT_HANDSHAKE_PROTECT, &vif->flags);
			if (vif->pend_skb)
				ath6kl_flush_pend_skb(vif);
			set_bit(FIRST_EAPOL_PENDSENT, &vif->flags);
			mod_timer(&vif->shprotect_timer,
				jiffies + ATH6KL_HANDSHAKE_PROC_TIMEOUT);
			spin_unlock_bh(&vif->if_lock);
			reset_handshake_pro = true;
		} else {
			reset_handshake_pro = false;
		}

	} else {
		spin_lock_bh(&vif->if_lock);
		clear_bit(CONNECT_HANDSHAKE_PROTECT, &vif->flags);
		if (vif->pend_skb)
			ath6kl_flush_pend_skb(vif);
		del_timer(&vif->shprotect_timer);
		spin_unlock_bh(&vif->if_lock);
		reset_handshake_pro = false;
	}

	return reset_handshake_pro;
}

void ath6kl_cfg80211_connect_result(struct ath6kl_vif *vif,
				const u8 *bssid,
				const u8 *req_ie,
				size_t req_ie_len,
				const u8 *resp_ie,
				size_t resp_ie_len,
				u16 status,
				gfp_t gfp)
{
	bool need_pending;

	ath6kl_change_scan_plan(vif, true);

	need_pending = ath6kl_p2p_pending_connect_event(vif,
							bssid,
							req_ie,
							req_ie_len,
							resp_ie,
							resp_ie_len,
							status,
							gfp);
	if (!need_pending)
		cfg80211_connect_result(vif->ndev,
					bssid,
					req_ie,
					req_ie_len,
					resp_ie,
					resp_ie_len,
					status,
					gfp);

	return;
}

void ath6kl_cfg80211_disconnected(struct ath6kl_vif *vif,
				u16 reason,
				u8 *ie,
				size_t ie_len,
				gfp_t gfp)
{
	ath6kl_change_scan_plan(vif, true);

	ath6kl_p2p_pending_disconnect_event(vif, reason, ie, ie_len, gfp);
	cfg80211_disconnected(vif->ndev,
				reason,
				ie,
				ie_len,
				gfp);

	return;
}

void ath6kl_cfg80211_connect_event(struct ath6kl_vif *vif, u16 channel,
				   u8 *bssid, u16 listen_intvl,
				   u16 beacon_intvl,
				   enum network_type nw_type,
				   u8 beacon_ie_len, u8 assoc_req_len,
				   u8 assoc_resp_len, u8 *assoc_info)
{
	struct ieee80211_channel *chan;
	struct ath6kl *ar = vif->ar;
	struct cfg80211_bss *bss;
	bool reset_hk_prot = false;

	/* capinfo + listen interval */
	u8 assoc_req_ie_offset = sizeof(u16) + sizeof(u16);

	/* capinfo + status code +  associd */
	u8 assoc_resp_ie_offset = sizeof(u16) + sizeof(u16) + sizeof(u16);

	u8 *assoc_req_ie = assoc_info + beacon_ie_len + assoc_req_ie_offset;
	u8 *assoc_resp_ie = assoc_info + beacon_ie_len + assoc_req_len +
	    assoc_resp_ie_offset;

	ath6kl_dbg(ATH6KL_DBG_WLAN_CFG, "%s: bssid %pM\n", __func__, bssid);

	assoc_req_len -= assoc_req_ie_offset;
	assoc_resp_len -= assoc_resp_ie_offset;

	/*
	 * Store Beacon interval here; DTIM period will be available only once
	 * a Beacon frame from the AP is seen.
	 */
	vif->assoc_bss_beacon_int = beacon_intvl;
	clear_bit(DTIM_PERIOD_AVAIL, &vif->flags);

	if (nw_type & ADHOC_NETWORK) {
		if (vif->wdev.iftype != NL80211_IFTYPE_ADHOC) {
			ath6kl_dbg(ATH6KL_DBG_WLAN_CFG,
				   "%s: ath6k not in ibss mode\n", __func__);
			return;
		}
#ifdef ATH6KL_SUPPORT_WIFI_KTK
		if (ar->ktk_active)
			ath6kl_install_ktk_ptk(vif);
#endif
	}

	if (nw_type & INFRA_NETWORK) {
		if (vif->wdev.iftype != NL80211_IFTYPE_STATION &&
		    vif->wdev.iftype != NL80211_IFTYPE_P2P_CLIENT) {
			ath6kl_dbg(ATH6KL_DBG_WLAN_CFG,
				   "%s: ath6k not in station mode\n", __func__);
			return;
		}
	}

	chan = ieee80211_get_channel(ar->wiphy, (int) channel);

	if (chan == NULL) {
		ath6kl_dbg(ATH6KL_DBG_WLAN_CFG,
			"%s: ath6k could not get channel information\n",
			__func__);
		return;
	}

	bss = ath6kl_add_bss_if_needed(vif, nw_type, bssid, chan,
				assoc_info, beacon_ie_len);
	if (!bss) {
		ath6kl_err("could not add cfg80211 bss entry\n");
		return;
	}

	if (nw_type & ADHOC_NETWORK) {
#ifdef ATH6KL_DIAGNOSTIC
		wifi_diag_mac_fsm_event(vif,
		(enum wifi_diag_mac_fsm_t)WIFI_DIAG_MAC_FSM_CONNECTED,
		vif->diag.connect_seq_num);
#endif
		ath6kl_dbg(ATH6KL_DBG_WLAN_CFG, "ad-hoc %s selected\n",
				nw_type & ADHOC_CREATOR ? "creator" : "joiner");
		cfg80211_ibss_joined(vif->ndev, bssid, GFP_KERNEL);
		ath6kl_bss_put(vif->ar, bss);
		return;
	}

	if (nw_type & INFRA_NETWORK)
		vif->phymode = ((u16)nw_type >> 8) & 0xff;

	reset_hk_prot = ath6kl_handshake_protect(vif, bssid);

	if (vif->sme_state == SME_CONNECTING) {
		/* inform connect result to cfg80211 */
		vif->sme_state = SME_CONNECTED;
#ifdef ATH6KL_DIAGNOSTIC
	wifi_diag_mac_fsm_event(vif,
		(enum wifi_diag_mac_fsm_t)WIFI_DIAG_MAC_FSM_CONNECTED,
		vif->diag.connect_seq_num);
#endif
		ath6kl_bss_put(vif->ar, bss);
		ath6kl_cfg80211_connect_result(vif, bssid,
					assoc_req_ie, assoc_req_len,
					assoc_resp_ie, assoc_resp_len,
					WLAN_STATUS_SUCCESS, GFP_KERNEL);
	} else if (ath6kl_roamed_indicate(vif, bssid, reset_hk_prot) == true) {
		cfg80211_roamed_bss(vif->ndev, bss, assoc_req_ie, assoc_req_len,
				assoc_resp_ie, assoc_resp_len, GFP_KERNEL);
	}
}

static int ath6kl_cfg80211_disconnect(struct wiphy *wiphy,
				      struct net_device *dev, u16 reason_code)
{
	struct ath6kl *ar = (struct ath6kl *)ath6kl_priv(dev);
	struct ath6kl_vif *vif = netdev_priv(dev);
	int ret;

	ath6kl_dbg(ATH6KL_DBG_WLAN_CFG | ATH6KL_DBG_EXT_INFO1, "%s: reason=%u\n",
			__func__, reason_code);

	if (!ath6kl_cfg80211_ready(vif))
		return -EIO;

	if (test_bit(DESTROY_IN_PROGRESS, &ar->flag)) {
		ath6kl_err("busy, destroy in progress\n");
		return -EBUSY;
	}

	if (down_interruptible(&ar->sem)) {
		ath6kl_err("busy, couldn't get access\n");
		return -ERESTARTSYS;
	}
	vif->reconnect_flag = 0;
	ret = ath6kl_disconnect(vif);
	memset(vif->ssid, 0, sizeof(vif->ssid));
	vif->ssid_len = 0;

	if (!test_bit(SKIP_SCAN, &ar->flag))
		memset(vif->req_bssid, 0, sizeof(vif->req_bssid));

	up(&ar->sem);

	vif->sme_state = SME_DISCONNECTED;

	/*
	 * To avoid race condition between driver and supplicant, waiting
	 * until received disconnect event.
	 */
	if ((!ret) &&
	    (vif->nw_type == INFRA_NETWORK) &&
	    (test_bit(CONNECTED, &vif->flags))) {
		if (test_bit(DISCONNECT_PEND, &vif->flags)) {
			/*
			 * Already be called by other commands
			 * (ex, interface down), so ignore.
			 */
			return 0;
		}
		set_bit(DISCONNECT_PEND, &vif->flags);
		wait_event_interruptible_timeout(ar->event_wq,
						 !test_bit(DISCONNECT_PEND,
						 &vif->flags),
						 (HZ/2));

		if (signal_pending(current)) {
			ath6kl_err("wait DISCONNECT timeout!\n");
			return -EINTR;
		}
	}

	return 0;
}

void ath6kl_cfg80211_disconnect_event(struct ath6kl_vif *vif, u8 reason,
				      u8 *bssid, u8 assoc_resp_len,
				      u8 *assoc_info, u16 proto_reason)
{
	struct ath6kl *ar = vif->ar;

	ath6kl_dbg(ATH6KL_DBG_WLAN_CFG | ATH6KL_DBG_EXT_INFO1,
		"%s: reason=%u, proto_reason %u\n",
		__func__, reason, proto_reason);

	if (vif->scan_req) {
		del_timer(&vif->vifscan_timer);
		ath6kl_wmi_abort_scan_cmd(ar->wmi, vif->fw_vif_idx);
		cfg80211_scan_done(vif->scan_req, true);
		vif->scan_req = NULL;
		clear_bit(SCANNING, &vif->flags);
#if defined(USB_AUTO_SUSPEND)
		/*
		In Disconnected state, the driver will enter deep sleep,
		such that the scan response will be lost, thus here
		prevent driver goes into deep sleep mode
		*/

		if (test_and_clear_bit(SCANNING, &ar->usb_autopm_scan)) {
			if (ath6kl_hif_auto_pm_get_usage_cnt(ar) == 0) {
				ath6kl_dbg(ATH6KL_DBG_WLAN_CFG,
				"%s: warnning pm_usage_cnt =0\n", __func__);
			} else {
				ath6kl_hif_auto_pm_enable(ar);
			}
		}

#endif
	}

	if (vif->nw_type & ADHOC_NETWORK) {
		if (vif->wdev.iftype != NL80211_IFTYPE_ADHOC) {
			ath6kl_dbg(ATH6KL_DBG_WLAN_CFG,
				   "%s: ath6k not in ibss mode\n", __func__);
		}
		return;
	}

	if (vif->nw_type & INFRA_NETWORK) {
		if (vif->wdev.iftype != NL80211_IFTYPE_STATION &&
		    vif->wdev.iftype != NL80211_IFTYPE_P2P_CLIENT) {
			ath6kl_dbg(ATH6KL_DBG_WLAN_CFG,
				   "%s: ath6k not in station mode\n", __func__);
			return;
		}
	}

	if (vif->pend_skb)
		ath6kl_flush_pend_skb(vif);

	/*
	 * Send a disconnect command to target when a disconnect event is
	 * received with reason code other than 3 (DISCONNECT_CMD - disconnect
	 * request from host) to make the firmware stop trying to connect even
	 * after giving disconnect event. There will be one more disconnect
	 * event for this disconnect command with reason code DISCONNECT_CMD
	 * which will be notified to cfg80211.
	 */

	if (reason != DISCONNECT_CMD) {
		if ((reason == AUTH_FAILED) &&
		    (vif->dot11_auth_mode & SHARED_AUTH))
			vif->next_conn_status = WLAN_STATUS_CHALLENGE_FAIL;
		else
			vif->next_conn_status = WLAN_STATUS_UNSPECIFIED_FAILURE;
		ath6kl_wmi_disconnect_cmd(ar->wmi, vif->fw_vif_idx);
		return;
	}

	clear_bit(CONNECT_PEND, &vif->flags);

	if (vif->sme_state == SME_CONNECTING) {
		u16 status = WLAN_STATUS_UNSPECIFIED_FAILURE;

		if (vif->next_conn_status)
			status = vif->next_conn_status;

		ath6kl_cfg80211_connect_result(vif,
				bssid, NULL, 0,
				NULL, 0,
				status,
				GFP_KERNEL);
	} else if (vif->sme_state == SME_CONNECTED) {
		ath6kl_cfg80211_disconnected(vif, proto_reason,
				NULL, 0, GFP_KERNEL);
	}
#ifdef ATH6KL_DIAGNOSTIC
	wifi_diag_mac_fsm_event(vif,
		(enum wifi_diag_mac_fsm_t)WIFI_DIAG_MAC_FSM_DISCONNECTED,
		vif->diag.disconnect_seq_num);
#endif

	vif->sme_state = SME_DISCONNECTED;
	vif->next_conn_status = WLAN_STATUS_SUCCESS;

	if (reason == DISCONNECT_CMD) {
		if (test_bit(DISCONNECT_PEND, &vif->flags) &&
		    (vif->nw_type == INFRA_NETWORK)) {
			clear_bit(DISCONNECT_PEND, &vif->flags);
			wake_up(&ar->event_wq);
		}
	}
}

static int ath6kl_cfg80211_change_bss(struct wiphy *wiphy,
	struct net_device *ndev, struct bss_parameters *params)
{
	struct ath6kl *ar = (struct ath6kl *)ath6kl_priv(ndev);
	struct ath6kl_vif *vif = netdev_priv(ndev);

	if (!ath6kl_cfg80211_ready(vif))
		return -EIO;

	if (down_interruptible(&ar->sem)) {
		ath6kl_err("busy, couldn't get access\n");
		return -ERESTARTSYS;
	}

	if (params->ap_isolate >= 0)
		vif->intra_bss = !params->ap_isolate;

	/* FIXME : support others. */

	up(&ar->sem);

	return 0;
}

void ath6kl_scan_timer_handler(unsigned long ptr)
{
	struct ath6kl_vif *vif = (struct ath6kl_vif *)ptr;
	struct ath6kl *ar = vif->ar;

	ath6kl_dbg(ATH6KL_DBG_WLAN_CFG, "%s scan timer hit\n", __func__);
	if (vif->scan_req) {
		ath6kl_wmi_abort_scan_cmd(ar->wmi, vif->fw_vif_idx);
		cfg80211_scan_done(vif->scan_req, true);
		vif->scan_req = NULL;
		clear_bit(SCANNING, &vif->flags);
#if defined(USB_AUTO_SUSPEND)
		/*
		In Disconnected state, the driver will enter deep sleep,
		such that the scan response will be lost, thus here
		prevent driver goes into deep sleep mode
		*/

		if (test_and_clear_bit(SCANNING, &ar->usb_autopm_scan)) {
			if (ath6kl_hif_auto_pm_get_usage_cnt(ar) == 0) {
				ath6kl_dbg(ATH6KL_DBG_WLAN_CFG,
				"%s: warnning pm_usage_cnt =0\n", __func__);
			} else {
				ath6kl_hif_auto_pm_enable(ar);
			}
		}

#endif
	}
#if defined(CE_2_SUPPORT)
	/* Electric Shock testing: Trigger USB module reset */
	cfg80211_disconnected(vif->ndev, 99, NULL, 0, GFP_KERNEL);
#endif
}

/* assume we support not more than two differnet channels */
static int ath6kl_scan_timeout_cal(struct ath6kl *ar)
{
	struct ath6kl_vif *vif;
	u16 connected_count = 0;

	if (!(ar->wiphy->flags & WIPHY_FLAG_SUPPORTS_FW_ROAM))
		return ATH6KL_SCAN_TIMEOUT_WITHOUT_ROAM;

	if (ath6kl_scan_timeout &&
		(ath6kl_scan_timeout < ATH6KL_SCAN_TIMEOUT_SHORT))
		return ATH6KL_SCAN_TIMEOUT_SHORT;

	list_for_each_entry(vif, &ar->vif_list, list) {
		if (test_bit(CONNECTED, &vif->flags)) {
			connected_count++;
			if (connected_count == 1)
				return ATH6KL_SCAN_TIMEOUT_ONE_CON;
			else if (connected_count > 1)
				return ATH6KL_SCAN_TIMEOUT_LONG;
		}
	}

	return ATH6KL_SCAN_TIMEOUT_SHORT;
}

static int ath6kl_set_probe_req_ies(struct ath6kl_vif *vif, const u8 *ies,
				    size_t ies_len)
{
	struct ath6kl *ar = vif->ar;
	const u8 *pos;
	u8 *buf = NULL;
	size_t len = 0;
	int ret;

	/*
	 * Filter out P2P/WFD IE(s)
	 */
	if (ies && ies_len) {
		buf = kmalloc(ies_len, GFP_KERNEL);
		if (buf == NULL)
			return -ENOMEM;
		pos = ies;

		while (pos + 1 < ies + ies_len) {
			if (pos + 2 + pos[1] > ies + ies_len)
				break;
			if ((ath6kl_is_p2p_ie(pos) ||
			     ath6kl_is_wfd_ie(pos)) &&
			     !ath6kl_p2p_ie_append(vif,
						P2P_IE_IN_PROBE_REQ))
				ath6kl_dbg(ATH6KL_DBG_WLAN_CFG,
						"Remove Probe's P2P IE\n");
			else {
				memcpy(buf + len, pos, 2 + pos[1]);
				len += 2 + pos[1];
			}
			pos += 2 + pos[1];
		}
	}

	ret = ath6kl_wmi_set_appie_cmd(ar->wmi, vif->fw_vif_idx,
				       WMI_FRAME_PROBE_REQ, buf, len);
	kfree(buf);

	return ret;
}

static bool _ath6kl_scanband_ignore_ch(struct ath6kl_vif *vif, u16 freq)
{
	int i;

	for (i = 0; i < 64; i++) {
		if (freq == vif->scanband_ignore_chan[i])
			return true;
		if (vif->scanband_ignore_chan[i] == 0)
			return false;
	}

	return false;
}

static s8 ath6kl_scanband(struct ath6kl_vif *vif,
					u16 *channels,
					s8 n_channels,
					struct cfg80211_scan_request *request)
{
	int i;
	u8 skip_chan_num = 0;
	u8 num_chan = n_channels;

	switch (vif->scanband_type) {
	case SCANBAND_TYPE_CHAN_ONLY:
		channels[0] = vif->scanband_chan;
		ath6kl_dbg(ATH6KL_DBG_INFO | ATH6KL_DBG_EXT_SCAN,
			"Only signal channel scan, channel %d\n",
			channels[0]);
		num_chan = 1;
		break;
	case SCANBAND_TYPE_5G:
		ath6kl_dbg(ATH6KL_DBG_INFO | ATH6KL_DBG_EXT_SCAN,
			"Only 5G channels scan, channel list - ");
		for (i = 0; i < n_channels; i++) {
			if (request->channels[i]->center_freq <= 2484) {
				skip_chan_num++;
				continue;
			}
			channels[i - skip_chan_num] =
					request->channels[i]->center_freq;
			ath6kl_dbg(ATH6KL_DBG_INFO | ATH6KL_DBG_EXT_SCAN,
				"%d ", channels[i - skip_chan_num]);
		}
		num_chan -= skip_chan_num;
		break;
	case SCANBAND_TYPE_2G:
		ath6kl_dbg(ATH6KL_DBG_INFO | ATH6KL_DBG_EXT_SCAN,
			"Only 2G channels scan, channel list - ");
		for (i = 0; i < n_channels; i++) {
			if (request->channels[i]->center_freq > 2484) {
				skip_chan_num++;
				continue;
			}
			channels[i - skip_chan_num] =
					request->channels[i]->center_freq;
			ath6kl_dbg(ATH6KL_DBG_INFO | ATH6KL_DBG_EXT_SCAN,
				"%d ", channels[i - skip_chan_num]);
		}
		num_chan -= skip_chan_num;
		break;
	case SCANBAND_TYPE_P2PCHAN:
		num_chan = ath6kl_p2p_build_scan_chan(vif,
							n_channels,
							channels);
		if (num_chan) {
			ath6kl_dbg(ATH6KL_DBG_INFO | ATH6KL_DBG_EXT_SCAN,
				"Only P2P channels scan, full scan instead\n");

			break;
		} else
			num_chan = n_channels;

		ath6kl_dbg(ATH6KL_DBG_INFO | ATH6KL_DBG_EXT_SCAN,
			"Only P2P channels scan, channel list - ");
		for (i = 0; i < n_channels; i++) {
			if (!ath6kl_reg_is_p2p_channel(vif->ar,
					request->channels[i]->center_freq)) {
				skip_chan_num++;
				continue;
			}
			channels[i - skip_chan_num] =
					request->channels[i]->center_freq;
			ath6kl_dbg(ATH6KL_DBG_INFO | ATH6KL_DBG_EXT_SCAN,
				"%d ", channels[i - skip_chan_num]);
		}
		num_chan -= skip_chan_num;
		break;
	case SCANBAND_TYPE_2_P2PCHAN:
		num_chan = ath6kl_p2p_build_scan_chan(vif,
							n_channels,
							channels);
		if (num_chan) {
			ath6kl_dbg(ATH6KL_DBG_INFO | ATH6KL_DBG_EXT_SCAN,
				"x2 P2P channels scan, full scan instead\n");

			break;
		} else
			num_chan = n_channels;

		ath6kl_dbg(ATH6KL_DBG_INFO | ATH6KL_DBG_EXT_SCAN,
			"x2 P2P channels scan, channel list - ");
		for (i = 0; i < n_channels; i++) {
			if (!ath6kl_reg_is_p2p_channel(vif->ar,
					request->channels[i]->center_freq)) {
				skip_chan_num++;
				continue;
			}
			channels[i - skip_chan_num] =
					request->channels[i]->center_freq;
			ath6kl_dbg(ATH6KL_DBG_INFO | ATH6KL_DBG_EXT_SCAN,
				"%d ", channels[i - skip_chan_num]);
		}
		num_chan -= skip_chan_num;
		/* Avoid to scan lots of channels. */
		if (num_chan <= (WMI_MAX_CHANNELS >> 1)) {
			memcpy(&channels[num_chan],
				&channels[0],
				num_chan * sizeof(u16));
			num_chan *= 2;
		}
		break;
	case SCANBAND_TYPE_IGNORE_DFS:
		ath6kl_dbg(ATH6KL_DBG_INFO | ATH6KL_DBG_EXT_SCAN,
			"No DFS channels scan, channel list - ");
		for (i = 0; i < n_channels; i++) {
			if (ath6kl_reg_is_dfs_channel(vif->ar,
					request->channels[i]->center_freq)) {
				skip_chan_num++;
				continue;
			}
			channels[i - skip_chan_num] =
					request->channels[i]->center_freq;
			ath6kl_dbg(ATH6KL_DBG_INFO | ATH6KL_DBG_EXT_SCAN,
				"%d ", channels[i - skip_chan_num]);
		}
		num_chan -= skip_chan_num;
		break;
	case SCANBAND_TYPE_IGNORE_CH:
		ath6kl_dbg(ATH6KL_DBG_INFO | ATH6KL_DBG_EXT_SCAN,
			"Ignore channels scan, channel list - ");
		for (i = 0; i < n_channels; i++) {
			if (_ath6kl_scanband_ignore_ch(vif,
					request->channels[i]->center_freq)) {
				skip_chan_num++;
				continue;
			}
			channels[i - skip_chan_num] =
					request->channels[i]->center_freq;
			ath6kl_dbg(ATH6KL_DBG_INFO | ATH6KL_DBG_EXT_SCAN,
				"%d ", channels[i - skip_chan_num]);
		}
		num_chan -= skip_chan_num;
		break;
	default:
		for (i = 0; i < n_channels; i++)
			channels[i] = request->channels[i]->center_freq;
		break;
	}

	return num_chan;
}

static int _ath6kl_cfg80211_scan(struct wiphy *wiphy, struct net_device *ndev,
				struct cfg80211_scan_request *request)
{
	struct ath6kl *ar = (struct ath6kl *)ath6kl_priv(ndev);
	struct ath6kl_vif *vif = netdev_priv(ndev);
	s8 n_channels = 0;
	u16 *channels = NULL;
	int ret = 0;
	u32 force_fg_scan = 0;
	bool sche_scan_trig, left;

	if (test_bit(DISABLE_SCAN, &ar->flag)) {
		ath6kl_dbg(ATH6KL_DBG_EXT_SCAN,
			"scan is disabled temporarily\n");
		return -EIO;
	}

	if (vif->sme_state == SME_CONNECTING) {
		ath6kl_dbg(ATH6KL_DBG_EXT_SCAN,
			"Connection on-going, reject scan\n");
		return -EBUSY;
	}

	if (!ath6kl_cfg80211_ready(vif))
		return -EIO;

	if (down_interruptible(&ar->sem)) {
		ath6kl_err("busy, couldn't get access\n");
		return -ERESTARTSYS;
	}

	ath6kl_dbg(ATH6KL_DBG_WLAN_CFG | ATH6KL_DBG_EXT_SCAN,
		"%s\n", __func__);

	/*
	 * Last Cancel-RoC not yet finished. To update vif->last_cancel_roc_id
	 * first to avoid wrong cookie report to supplicant.
	 */
	if (test_bit(ROC_CANCEL_PEND, &vif->flags)) {
		ath6kl_dbg(ATH6KL_DBG_EXT_ROC,
			"RoC : Scan but Cancel-RoC not yet back, wait it finish %x\n",
			vif->last_cancel_roc_id);

		wait_event_interruptible_timeout(ar->event_wq,
					!test_bit(ROC_ONGOING, &vif->flags),
					WMI_TIMEOUT);
		if (signal_pending(current)) {
			ath6kl_dbg(ATH6KL_DBG_EXT_ROC,
				"RoC : target did not respond\n");
			up(&ar->sem);
			return -EINTR;
		}
	}

	/* RoC is ongoing and stop it first. */
	if (test_bit(ROC_ONGOING, &vif->flags)) {
		ath6kl_dbg(ATH6KL_DBG_EXT_ROC,
			"RoC : Scan but On-going-RoC, cancel it first %x\n",
			vif->last_roc_id);

		set_bit(ROC_CANCEL_PEND, &vif->flags);
		if (ath6kl_wmi_cancel_remain_on_chnl_cmd(ar->wmi,
				vif->fw_vif_idx) != 0) {
			ath6kl_dbg(ATH6KL_DBG_EXT_ROC,
				"RoC : cancel ROC failed\n");
			clear_bit(ROC_CANCEL_PEND, &vif->flags);
			up(&ar->sem);
			return -EIO;
		}

		left = wait_event_interruptible_timeout(ar->event_wq,
					!test_bit(ROC_ONGOING, &vif->flags),
					WMI_TIMEOUT);

		if (left == 0) {
			ath6kl_dbg(ATH6KL_DBG_EXT_ROC,
				"RoC : wait cancel RoC timeout\n");
			clear_bit(ROC_CANCEL_PEND, &vif->flags);
			clear_bit(ROC_ONGOING, &vif->flags);
			up(&ar->sem);
			return -EINTR;
		}

		if (signal_pending(current)) {
			ath6kl_dbg(ATH6KL_DBG_EXT_ROC,
				"RoC : target did not respond\n");
			up(&ar->sem);
			return -EINTR;
		}
	}

	sche_scan_trig = ath6kl_sched_scan_trigger(vif);

	if (!vif->usr_bss_filter) {
		clear_bit(CLEAR_BSSFILTER_ON_BEACON, &vif->flags);

		/*
		 *  EV: 109005
		 *  Fix bug that Probe response from the connected AP
		 *  will be filtered by setting this filter ,
		 *  thus the AP's information won't be updated.
		 */
		ret = ath6kl_wmi_bssfilter_cmd(
			ar->wmi, vif->fw_vif_idx, ALL_BSS_FILTER , 0);


		if (ret) {
			ath6kl_err("couldn't set bss filtering\n");
			up(&ar->sem);
			return ret;
		}
	}

	if ((request->n_ssids && request->ssids[0].ssid_len) &&
	    (!sche_scan_trig)) {
		u8 i;

		if (request->n_ssids > (MAX_PROBED_SSID_INDEX - 1))
			request->n_ssids = MAX_PROBED_SSID_INDEX - 1;

		for (i = 0; i < request->n_ssids; i++)
			ath6kl_wmi_probedssid_cmd(ar->wmi, vif->fw_vif_idx,
						  i + 1, SPECIFIC_SSID_FLAG,
						  request->ssids[i].ssid_len,
						  request->ssids[i].ssid);
	} else if (ar->p2p)
		ath6kl_wmi_probedssid_cmd(ar->wmi, vif->fw_vif_idx,
					  MAX_PROBED_SSID_INDEX, ANY_SSID_FLAG,
					  0,
					  NULL);

	if ((request->ie) &&
	    (!sche_scan_trig)) {
		ret = ath6kl_set_probe_req_ies(vif,
						request->ie,
						request->ie_len);
		if (ret) {
			ath6kl_err("failed to set Probe Request appie for "
				   "scan");
			up(&ar->sem);
			return ret;
		}
	}

	/*
	 * Scan only the requested channels if the request specifies a set of
	 * channels. If the list is longer than the target supports, do not
	 * configure the list and instead, scan all available channels.
	 */
	if ((request->n_channels > 0 &&
	     request->n_channels <= WMI_MAX_CHANNELS) &&
	    (!sche_scan_trig)) {

		n_channels = request->n_channels;

		channels = kzalloc(n_channels * sizeof(u16) * 2, GFP_KERNEL);
		if (channels == NULL) {
			ath6kl_dbg(ATH6KL_DBG_EXT_SCAN,
				"failed to set scan chan, scan all channel\n");
			n_channels = 0;
		}

		if (n_channels) {
			/* Rearrange according scanband */
			n_channels = ath6kl_scanband(vif,
						     channels,
						     n_channels,
						     request);
		}
	}

	if (test_bit(CONNECTED, &vif->flags))
		force_fg_scan = 1;

	if (test_and_set_bit(SCANNING, &vif->flags)) {
		kfree(channels);
		up(&ar->sem);
		return -EBUSY;
	}

	if (test_bit(CONNECT_HANDSHAKE_PROTECT, &vif->flags) ||
	    test_bit(EAPOL_HANDSHAKE_PROTECT, &ar->flag)) {
		ath6kl_dbg(ATH6KL_DBG_EXT_SCAN,
			"EAPOL %s_handshake_protect reject scan\n",
			(test_bit(EAPOL_HANDSHAKE_PROTECT, &ar->flag) ?
			 "rekey" : "connect"));
		clear_bit(SCANNING, &vif->flags);
		kfree(channels);
		up(&ar->sem);
		return -EBUSY;
	}

	ret = ath6kl_wmi_scanparams_cmd(ar->wmi, vif->fw_vif_idx,
				vif->sc_params.fg_start_period,
				vif->sc_params.fg_end_period,
				vif->sc_params.bg_period,
				vif->sc_params.minact_chdwell_time,
				vif->sc_params.maxact_chdwell_time,
				vif->sc_params.pas_chdwell_time,
				vif->sc_params.short_scan_ratio,
				vif->sc_params.scan_ctrl_flags,
				vif->sc_params.max_dfsch_act_time,
				vif->sc_params.maxact_scan_per_ssid);

	if (ret) {
		ath6kl_err("ath6kl_cfg80211_scan: set scan parameter failed\n");
		clear_bit(SCANNING, &vif->flags);
		kfree(channels);
		up(&ar->sem);
		return ret;
	}

	ret = ath6kl_wmi_startscan_cmd(ar->wmi, vif->fw_vif_idx, WMI_LONG_SCAN,
					force_fg_scan, false, 0,
					ATH6KL_FG_SCAN_INTERVAL,
					n_channels, channels);
	if (ret) {
		ath6kl_err("wmi_startscan_cmd failed\n");
		clear_bit(SCANNING, &vif->flags);
	} else {
		vif->scan_req = request;
		mod_timer(&vif->vifscan_timer,
			jiffies + ath6kl_scan_timeout_cal(ar));
#ifdef USB_AUTO_SUSPEND
		if (test_bit(SCANNING, &ar->usb_autopm_scan) == 0) {
			set_bit(SCANNING, &ar->usb_autopm_scan);
			ath6kl_hif_auto_pm_disable(ar);
		}
#endif
	}

	kfree(channels);

	ath6kl_bss_post_proc_bss_scan_start(vif);
	ath6kl_p2p_rc_scan_start(vif, false);

	up(&ar->sem);

	return ret;
}

#ifdef CFG80211_NETDEV_REPLACED_BY_WDEV
static int ath6kl_cfg80211_scan(struct wiphy *wiphy,
				struct cfg80211_scan_request *request)
{
	BUG_ON(!request->wdev);
	BUG_ON(!request->wdev->netdev);

	return _ath6kl_cfg80211_scan(wiphy,
					request->wdev->netdev,
					request);
}
#else
static int ath6kl_cfg80211_scan(struct wiphy *wiphy, struct net_device *ndev,
				struct cfg80211_scan_request *request)
{
	return _ath6kl_cfg80211_scan(wiphy, ndev, request);
}
#endif

void ath6kl_cfg80211_scan_complete_event(struct ath6kl_vif *vif, bool aborted)
{
	struct ath6kl *ar = vif->ar;
	int i;

	ath6kl_dbg(ATH6KL_DBG_WLAN_CFG | ATH6KL_DBG_EXT_SCAN,
		"%s: status%s\n", __func__,
		aborted ? " aborted" : "complete");

#if defined(USB_AUTO_SUSPEND)
	/*
	In Disconnected state, the driver will enter deep sleep,
	such that the scan response will be lost, thus here
	prevent driver goes into deep sleep mode
	*/

	if (test_and_clear_bit(SCANNING, &ar->usb_autopm_scan)) {
		if (ath6kl_hif_auto_pm_get_usage_cnt(ar) == 0) {
			ath6kl_dbg(ATH6KL_DBG_WLAN_CFG | ATH6KL_DBG_EXT_SCAN,
			"%s: warnning pm_usage_cnt =0\n", __func__);
		} else {
			ath6kl_hif_auto_pm_enable(ar);
		}
	}

#endif
	del_timer(&vif->vifscan_timer);

	if (test_bit(SCANNING_WAIT, &vif->flags)) {
		clear_bit(SCANNING_WAIT, &vif->flags);
		wake_up(&ar->event_wq);
	}

	if (!vif->scan_req)
		return;

	if (aborted)
		goto out;

	if (vif->scan_req->n_ssids && vif->scan_req->ssids[0].ssid_len) {
		for (i = 0; i < vif->scan_req->n_ssids; i++) {
			ath6kl_wmi_probedssid_cmd(ar->wmi, vif->fw_vif_idx,
						  i + 1, DISABLE_SSID_FLAG,
						  0, NULL);
		}
	} else if (ar->p2p)
		ath6kl_wmi_probedssid_cmd(ar->wmi, vif->fw_vif_idx,
						  MAX_PROBED_SSID_INDEX,
						  DISABLE_SSID_FLAG,
						  0, NULL);

out:
	cfg80211_scan_done(vif->scan_req, aborted);
	vif->scan_req = NULL;
	clear_bit(SCANNING, &vif->flags);
}

#ifdef PMF_SUPPORT
static int ath6kl_cfg80211_add_mgmt_key(struct wiphy *wiphy,
					struct net_device *ndev,
					u8 key_index, bool pairwise,
					const u8 *mac_addr,
					struct key_params *params)
{
	struct ath6kl *ar = (struct ath6kl *)ath6kl_priv(ndev);
	struct ath6kl_vif *vif = netdev_priv(ndev);
	struct ath6kl_key *key = NULL;
	enum wmi_sync_flag sync_flag = NO_SYNC_WMIFLAG;
	int ret;

	if (!ath6kl_cfg80211_ready(vif))
		return -EIO;

	if (!params)
		return -EINVAL;

	if (down_interruptible(&ar->sem)) {
		ath6kl_err("busy, couldn't get access\n");
		return -ERESTARTSYS;
	}

	if (key_index < WMI_MIN_IGTK_INDEX || key_index > WMI_MAX_IGTK_INDEX) {
		ath6kl_dbg(ATH6KL_DBG_WLAN_CFG,
			   "%s: key index %d out of bounds\n", __func__,
			   key_index);
		up(&ar->sem);
		return -ENOENT;
	}

	key = &vif->keys[key_index];
	memset(key, 0, sizeof(struct ath6kl_key));

	if (params) {
		int seq_len = params->seq_len;
		if (params->key_len > WMI_IGTK_KEY_LEN ||
			seq_len > sizeof(key->seq)) {
			up(&ar->sem);
			return -EINVAL;
		}

		key->key_len = params->key_len;
		memcpy(key->key, params->key, key->key_len);
		key->seq_len = seq_len;
		memcpy(key->seq, params->seq, key->seq_len);
		key->cipher = params->cipher;
	}

	if (vif->nw_type == AP_NETWORK && !pairwise && params) {
		/* Do something here for AP/GO mode. */
		;
	}

	if (ath6kl_mod_debug_quirks(ar, ATH6KL_MODULE_DISABLE_WMI_SYC))
		sync_flag = NO_SYNC_WMIFLAG;

	ret = ath6kl_wmi_addkey_igtk_cmd(ar->wmi, vif->fw_vif_idx, key_index,
				     key->key_len, key->seq,
				     key->key, sync_flag);

	up(&ar->sem);
	return ret;
}
#endif

static int ath6kl_cfg80211_add_key(struct wiphy *wiphy,
				   struct net_device *ndev,
				   u8 key_index, bool pairwise,
				   const u8 *mac_addr,
				   struct key_params *params)
{
	struct ath6kl *ar = (struct ath6kl *)ath6kl_priv(ndev);
	struct ath6kl_vif *vif = netdev_priv(ndev);
	struct ath6kl_key *key = NULL;
	enum wmi_sync_flag sync_flag = SYNC_BOTH_WMIFLAG;
	u8 key_usage;
	u8 key_type;
	int ret;

	if (!ath6kl_cfg80211_ready(vif))
		return -EIO;

	if (!params)
		return -EINVAL;

#ifdef PMF_SUPPORT
	if (params->cipher == WLAN_CIPHER_SUITE_AES_CMAC)
		return ath6kl_cfg80211_add_mgmt_key(wiphy, ndev, key_index,
						pairwise, mac_addr, params);
#endif

	if (down_interruptible(&ar->sem)) {
		ath6kl_err("busy, couldn't get access\n");
		return -ERESTARTSYS;
	}

	if (params->cipher == CCKM_KRK_CIPHER_SUITE) {
		if (params->key_len != WMI_KRK_LEN) {
			up(&ar->sem);
			return -EINVAL;
		}

		ret = ath6kl_wmi_add_krk_cmd(ar->wmi, vif->fw_vif_idx,
					      params->key);
		up(&ar->sem);
		return ret;
	}

	if (key_index > WMI_MAX_KEY_INDEX) {
		ath6kl_dbg(ATH6KL_DBG_WLAN_CFG,
			   "%s: key index %d out of bounds\n", __func__,
			   key_index);
		up(&ar->sem);
		return -ENOENT;
	}

	key = &vif->keys[key_index];
	memset(key, 0, sizeof(struct ath6kl_key));

	if (pairwise)
		key_usage = PAIRWISE_USAGE;
	else
		key_usage = GROUP_USAGE;

	if (params) {
		int seq_len = params->seq_len;
		if (params->cipher == WLAN_CIPHER_SUITE_SMS4 &&
		    seq_len > ATH6KL_KEY_SEQ_LEN) {
			/* Only first half of the WPI PN is configured */
			seq_len = ATH6KL_KEY_SEQ_LEN;
		}
		if (params->key_len > WLAN_MAX_KEY_LEN ||
			seq_len > sizeof(key->seq)) {
			up(&ar->sem);
			return -EINVAL;
		}

		key->key_len = params->key_len;
		memcpy(key->key, params->key, key->key_len);
		key->seq_len = seq_len;
		memcpy(key->seq, params->seq, key->seq_len);
		key->cipher = params->cipher;
	}

	switch (key->cipher) {
	case WLAN_CIPHER_SUITE_WEP40:
	case WLAN_CIPHER_SUITE_WEP104:
		key_type = WEP_CRYPT;
		break;

	case WLAN_CIPHER_SUITE_TKIP:
		key_type = TKIP_CRYPT;
		break;

	case WLAN_CIPHER_SUITE_CCMP:
		key_type = AES_CRYPT;
		break;
	case WLAN_CIPHER_SUITE_SMS4:
		key_type = WAPI_CRYPT;
		break;

	default:
		up(&ar->sem);
		return -ENOTSUPP;
	}

	if (((vif->auth_mode == WPA_PSK_AUTH)
	     || (vif->auth_mode == WPA2_PSK_AUTH))
	    && (key_usage & GROUP_USAGE))
		del_timer(&vif->disconnect_timer);

	if (key_usage & GROUP_USAGE) {
		if (vif->pend_skb) {
			ath6kl_err("eapol protect shall be off already\n");
			ath6kl_flush_pend_skb(vif);
		}

		spin_lock_bh(&vif->if_lock);
		clear_bit(CONNECT_HANDSHAKE_PROTECT, &vif->flags);
		del_timer(&vif->shprotect_timer);
		spin_unlock_bh(&vif->if_lock);
	}

	ath6kl_dbg(ATH6KL_DBG_WLAN_CFG,
		   "%s: index %d, key_len %d, key_type 0x%x, key_usage 0x%x, seq_len %d\n",
		   __func__, key_index, key->key_len, key_type,
		   key_usage, key->seq_len);

	if (vif->nw_type == AP_NETWORK && !pairwise &&
	    (key_type == TKIP_CRYPT || key_type == AES_CRYPT) && params) {
		vif->ap_mode_bkey.valid = true;
		vif->ap_mode_bkey.key_index = key_index;
		vif->ap_mode_bkey.key_type = key_type;
		vif->ap_mode_bkey.key_len = key->key_len;
		memcpy(vif->ap_mode_bkey.key, key->key, key->key_len);
		if (!test_bit(CONNECTED, &vif->flags)) {
			ath6kl_dbg(ATH6KL_DBG_WLAN_CFG, "Delay initial group "
				   "key configuration until AP mode has been "
				   "started\n");
			/*
			 * The key will be set in ath6kl_connect_ap_mode() once
			 * the connected event is received from the target.
			 */
			up(&ar->sem);
			return 0;
		}
	}

	if (vif->next_mode == AP_NETWORK && key_type == WEP_CRYPT &&
	    !test_bit(CONNECTED, &vif->flags)) {
		/*
		 * Store the key locally so that it can be re-configured after
		 * the AP mode has properly started
		 * (ath6kl_install_statioc_wep_keys).
		 */
		ath6kl_dbg(ATH6KL_DBG_WLAN_CFG, "Delay WEP key configuration "
			   "until AP mode has been started\n");
		vif->wep_key_list[key_index].key_len = key->key_len;
		memcpy(vif->wep_key_list[key_index].key, key->key,
		       key->key_len);
		up(&ar->sem);
		return 0;
	}

	if (ath6kl_mod_debug_quirks(ar, ATH6KL_MODULE_DISABLE_WMI_SYC))
		sync_flag = NO_SYNC_WMIFLAG;

	ret = ath6kl_wmi_addkey_cmd(ar->wmi, vif->fw_vif_idx, key_index,
				     key_type, key_usage, key->key_len,
				     key->seq, key->seq_len, key->key,
				     KEY_OP_INIT_VAL,
				     (u8 *) mac_addr, sync_flag);

	up(&ar->sem);

	return ret;
}

static int ath6kl_cfg80211_del_key(struct wiphy *wiphy, struct net_device *ndev,
				   u8 key_index, bool pairwise,
				   const u8 *mac_addr)
{
	struct ath6kl *ar = (struct ath6kl *)ath6kl_priv(ndev);
	struct ath6kl_vif *vif = netdev_priv(ndev);
	int ret;

	ath6kl_dbg(ATH6KL_DBG_WLAN_CFG, "%s: index %d\n", __func__, key_index);

	if (!ath6kl_cfg80211_ready(vif))
		return -EIO;

	if (down_interruptible(&ar->sem)) {
		ath6kl_err("busy, couldn't get access\n");
		return -ERESTARTSYS;
	}

	if (key_index > WMI_MAX_KEY_INDEX) {
		ath6kl_dbg(ATH6KL_DBG_WLAN_CFG,
			   "%s: key index %d out of bounds\n", __func__,
			   key_index);
		up(&ar->sem);
		return -ENOENT;
	}

	if (!vif->keys[key_index].key_len) {
		ath6kl_dbg(ATH6KL_DBG_WLAN_CFG,
			   "%s: index %d is empty\n", __func__, key_index);
		up(&ar->sem);
		return 0;
	}

	vif->keys[key_index].key_len = 0;

	ret = ath6kl_wmi_deletekey_cmd(ar->wmi, vif->fw_vif_idx, key_index);

	up(&ar->sem);

	return ret;
}

static int ath6kl_cfg80211_get_key(struct wiphy *wiphy, struct net_device *ndev,
				   u8 key_index, bool pairwise,
				   const u8 *mac_addr, void *cookie,
				   void (*callback) (void *cookie,
						     struct key_params *))
{
	struct ath6kl_vif *vif = netdev_priv(ndev);
	struct ath6kl_key *key = NULL;
	struct key_params params;

	ath6kl_dbg(ATH6KL_DBG_WLAN_CFG, "%s: index %d\n", __func__, key_index);

	if (!ath6kl_cfg80211_ready(vif))
		return -EIO;

	if (down_interruptible(&vif->ar->sem)) {
		ath6kl_err("busy, couldn't get access\n");
		return -ERESTARTSYS;
	}

	if (key_index > WMI_MAX_KEY_INDEX) {
		ath6kl_dbg(ATH6KL_DBG_WLAN_CFG,
			   "%s: key index %d out of bounds\n", __func__,
			   key_index);
		up(&vif->ar->sem);
		return -ENOENT;
	}

	key = &vif->keys[key_index];
	memset(&params, 0, sizeof(params));
	params.cipher = key->cipher;
	params.key_len = key->key_len;
	params.seq_len = key->seq_len;
	params.seq = key->seq;
	params.key = key->key;

	callback(cookie, &params);

	up(&vif->ar->sem);

	return key->key_len ? 0 : -ENOENT;
}

static int ath6kl_cfg80211_set_default_key(struct wiphy *wiphy,
					   struct net_device *ndev,
					   u8 key_index, bool unicast,
					   bool multicast)
{
	struct ath6kl *ar = (struct ath6kl *)ath6kl_priv(ndev);
	struct ath6kl_vif *vif = netdev_priv(ndev);
	struct ath6kl_key *key = NULL;
	u8 key_usage;
	enum crypto_type key_type = NONE_CRYPT;
	enum wmi_sync_flag sync_flag = SYNC_BOTH_WMIFLAG;
	int ret;

	ath6kl_dbg(ATH6KL_DBG_WLAN_CFG, "%s: index %d\n", __func__, key_index);

	if (!ath6kl_cfg80211_ready(vif))
		return -EIO;

	if (down_interruptible(&ar->sem)) {
		ath6kl_err("busy, couldn't get access\n");
		return -ERESTARTSYS;
	}

	if (key_index > WMI_MAX_KEY_INDEX) {
		ath6kl_dbg(ATH6KL_DBG_WLAN_CFG,
			   "%s: key index %d out of bounds\n",
			   __func__, key_index);
		up(&ar->sem);
		return -ENOENT;
	}

	if (!vif->keys[key_index].key_len) {
		ath6kl_dbg(ATH6KL_DBG_WLAN_CFG, "%s: invalid key index %d\n",
			   __func__, key_index);
		up(&ar->sem);
		return -EINVAL;
	}

	vif->def_txkey_index = key_index;
	key = &vif->keys[vif->def_txkey_index];
	key_usage = GROUP_USAGE;
	if (vif->prwise_crypto == WEP_CRYPT)
		key_usage |= TX_USAGE;
	if (unicast)
		key_type = vif->prwise_crypto;
	if (multicast)
		key_type = vif->grp_crypto;

	if (vif->next_mode ==
	AP_NETWORK && !test_bit(CONNECTED, &vif->flags)) {
		up(&ar->sem);
		return 0; /* Delay until AP mode has been started */
	}

	if (ath6kl_mod_debug_quirks(ar, ATH6KL_MODULE_DISABLE_WMI_SYC))
		sync_flag = NO_SYNC_WMIFLAG;

	ret = ath6kl_wmi_addkey_cmd(ar->wmi, vif->fw_vif_idx,
				     vif->def_txkey_index,
				     key_type, key_usage,
				     key->key_len, key->seq, key->seq_len,
				     key->key,
				     KEY_OP_INIT_VAL, NULL,
				     sync_flag);

	up(&ar->sem);

	return ret;
}

void ath6kl_cfg80211_tkip_micerr_event(struct ath6kl_vif *vif, u8 keyid,
				       bool ismcast)
{
	ath6kl_dbg(ATH6KL_DBG_WLAN_CFG,
		   "%s: keyid %d, ismcast %d\n", __func__, keyid, ismcast);

	cfg80211_michael_mic_failure(vif->ndev, vif->bssid,
				     (ismcast ? NL80211_KEYTYPE_GROUP :
				      NL80211_KEYTYPE_PAIRWISE), keyid, NULL,
				     GFP_KERNEL);
}

static int ath6kl_cfg80211_set_wiphy_params(struct wiphy *wiphy, u32 changed)
{
	struct ath6kl *ar = (struct ath6kl *)wiphy_priv(wiphy);
	struct ath6kl_vif *vif;
	int ret;

	ath6kl_dbg(ATH6KL_DBG_WLAN_CFG, "%s: changed 0x%x\n", __func__,
		   changed);

	vif = ath6kl_vif_first(ar);
	if (!vif)
		return -EIO;

	if (!ath6kl_cfg80211_ready(vif))
		return -EIO;

	if (down_interruptible(&ar->sem)) {
		ath6kl_err("busy, couldn't get access\n");
		return -ERESTARTSYS;
	}

	if (changed & WIPHY_PARAM_RTS_THRESHOLD) {
		ret = ath6kl_wmi_set_rts_cmd(ar->wmi, 0, wiphy->rts_threshold);
		if (ret != 0) {
			ath6kl_err("ath6kl_wmi_set_rts_cmd failed\n");
			up(&ar->sem);
			return -EIO;
		}
	}

        if ( changed & WIPHY_PARAM_COVERAGE_CLASS ) {
                ret = ath6kl_wmi_set_coverage_class(ar->wmi, wiphy->coverage_class);
                if (ret != 0) {
                        ath6kl_err("ath6kl_wmi_set_coverage_class failed\n");
                        up(&ar->sem);
                        return -EIO;
                }
        }

	up(&ar->sem);

	return 0;
}

/*
 * The type nl80211_tx_power_setting replaces the following
 * data type from 2.6.36 onwards
*/
static int _ath6kl_cfg80211_set_txpower(struct wiphy *wiphy,
					struct net_device *ndev,
					enum nl80211_tx_power_setting type,
					int mbm)
{
	struct ath6kl *ar = (struct ath6kl *)wiphy_priv(wiphy);
	struct ath6kl_vif *vif = netdev_priv(ndev);
	u8 ath6kl_dbm;
	int dbm = MBM_TO_DBM(mbm);

	ath6kl_dbg(ATH6KL_DBG_WLAN_CFG, "%s: type 0x%x, dbm %d\n", __func__,
		   type, dbm);

	if (!ath6kl_cfg80211_ready(vif))
		return -EIO;

	if (down_interruptible(&ar->sem)) {
		ath6kl_err("busy, couldn't get access\n");
		return -ERESTARTSYS;
	}

	switch (type) {
	case NL80211_TX_POWER_AUTOMATIC:
		up(&ar->sem);
		return 0;
	case NL80211_TX_POWER_LIMITED:
		ar->tx_pwr = ath6kl_dbm = dbm;
		break;
	default:
		ath6kl_dbg(ATH6KL_DBG_WLAN_CFG, "%s: type 0x%x not supported\n",
			   __func__, type);
		up(&ar->sem);
		return -EOPNOTSUPP;
	}

	ath6kl_wmi_set_tx_pwr_cmd(ar->wmi, vif->fw_vif_idx, ath6kl_dbm);

	up(&ar->sem);

	return 0;
}

static int _ath6kl_cfg80211_get_txpower(struct wiphy *wiphy,
					struct net_device *ndev,
					int *dbm)
{
	struct ath6kl *ar = (struct ath6kl *)wiphy_priv(wiphy);
	struct ath6kl_vif *vif = netdev_priv(ndev);

	if (!ath6kl_cfg80211_ready(vif))
		return -EIO;

	if (down_interruptible(&ar->sem)) {
		ath6kl_err("busy, couldn't get access\n");
		return -ERESTARTSYS;
	}

	if (test_bit(CONNECTED, &vif->flags)) {
		ar->tx_pwr = 0;

		if (ath6kl_wmi_get_tx_pwr_cmd(ar->wmi, vif->fw_vif_idx) != 0) {
			ath6kl_err("ath6kl_wmi_get_tx_pwr_cmd failed\n");
			up(&ar->sem);
			return -EIO;
		}

		wait_event_interruptible_timeout(ar->event_wq, ar->tx_pwr != 0,
						 5 * HZ);

		if (signal_pending(current)) {
			ath6kl_err("target did not respond\n");
			up(&ar->sem);
			return -EINTR;
		}
	}

	*dbm = ar->tx_pwr;

	up(&ar->sem);

	return 0;
}

#ifdef CFG80211_TX_POWER_PER_WDEV
static int ath6kl_cfg80211_set_txpower(struct wiphy *wiphy,
				       struct wireless_dev *wdev,
				       enum nl80211_tx_power_setting type,
				       int mbm)
{
	struct ath6kl *ar = (struct ath6kl *)wiphy_priv(wiphy);
	struct net_device *ndev;

	if (wdev == NULL) {
		struct ath6kl_vif *vif = ath6kl_vif_first(ar);

		if (!vif)
			return -EIO;
		ndev = vif->ndev;
	} else
		ndev = wdev->netdev;

	return _ath6kl_cfg80211_set_txpower(wiphy, ndev, type, mbm);
}

static int ath6kl_cfg80211_get_txpower(struct wiphy *wiphy,
				       struct wireless_dev *wdev,
				       int *dbm)
{
	struct ath6kl *ar = (struct ath6kl *)wiphy_priv(wiphy);
	struct net_device *ndev;

	if (wdev == NULL) {
		struct ath6kl_vif *vif = ath6kl_vif_first(ar);

		if (!vif)
			return -EIO;
		ndev = vif->ndev;
	} else
		ndev = wdev->netdev;

	return _ath6kl_cfg80211_get_txpower(wiphy, ndev , dbm);
}
#else
static int ath6kl_cfg80211_set_txpower(struct wiphy *wiphy,
				       enum nl80211_tx_power_setting type,
				       int mbm)
{
	struct ath6kl *ar = (struct ath6kl *)wiphy_priv(wiphy);
	struct ath6kl_vif *vif = ath6kl_vif_first(ar);

	if (!vif)
		return -EIO;

	return _ath6kl_cfg80211_set_txpower(wiphy, vif->ndev, type, mbm);
}

static int ath6kl_cfg80211_get_txpower(struct wiphy *wiphy, int *dbm)
{
	struct ath6kl *ar = (struct ath6kl *)wiphy_priv(wiphy);
	struct ath6kl_vif *vif = ath6kl_vif_first(ar);

	if (!vif)
		return -EIO;

	return _ath6kl_cfg80211_get_txpower(wiphy, vif->ndev, dbm);
}
#endif

static int ath6kl_cfg80211_set_power_mgmt(struct wiphy *wiphy,
					  struct net_device *dev,
					  bool pmgmt, int timeout)
{
	struct ath6kl *ar = ath6kl_priv(dev);
	struct wmi_power_mode_cmd mode;
	struct ath6kl_vif *vif = netdev_priv(dev);

	ath6kl_dbg(ATH6KL_DBG_WLAN_CFG, "%s: pmgmt %d, timeout %d\n",
		   __func__, pmgmt, timeout);

	if (test_bit(PS_STICK, &vif->flags)) {
		ath6kl_dbg(ATH6KL_DBG_WLAN_CFG, "PS mode already stick (%d).\n",
				vif->last_pwr_mode);
		return 0;
	}

	if (!ath6kl_cfg80211_ready(vif))
		return -EIO;

	if (down_interruptible(&ar->sem)) {
		ath6kl_err("busy, couldn't get access\n");
		return -ERESTARTSYS;
	}

	if (pmgmt) {
		ath6kl_dbg(ATH6KL_DBG_WLAN_CFG, "%s: rec power\n", __func__);
		mode.pwr_mode = REC_POWER;
	} else {
		ath6kl_dbg(ATH6KL_DBG_WLAN_CFG, "%s: max perf\n", __func__);
		mode.pwr_mode = MAX_PERF_POWER;
	}

	if (ath6kl_wmi_powermode_cmd(ar->wmi, vif->fw_vif_idx,
	     mode.pwr_mode) != 0) {
		ath6kl_err("wmi_powermode_cmd failed\n");
		up(&ar->sem);
		return -EIO;
	}

	up(&ar->sem);

	return 0;
}

static struct net_device *_ath6kl_cfg80211_add_iface(struct wiphy *wiphy,
						    char *name,
						    enum nl80211_iftype type,
						    u32 *flags,
						    struct vif_params *params)
{
	struct ath6kl *ar = wiphy_priv(wiphy);
	struct net_device *ndev;
	u8 if_idx, nw_type;

	if (!__ath6kl_cfg80211_ready(ar))
		return NULL;

	if (down_interruptible(&ar->sem)) {
		ath6kl_err("busy, couldn't get access\n");
		return NULL;
	}

	if (ar->num_vif == ar->vif_max) {
		ath6kl_err("Reached maximum number of supported vif\n");
		up(&ar->sem);
		return NULL;
	}

	if (!ath6kl_is_valid_iftype(ar, type, &if_idx, &nw_type)) {
		ath6kl_err("Not a supported interface type\n");
		up(&ar->sem);
		return NULL;
	}

	ndev = ath6kl_interface_add(ar, name, type, if_idx, nw_type);
	if (!ndev) {
		up(&ar->sem);
		return NULL;
	}

	ar->num_vif++;

	up(&ar->sem);

	return ndev;
}

static int _ath6kl_cfg80211_del_iface(struct wiphy *wiphy,
				     struct net_device *ndev)
{
	struct ath6kl *ar = wiphy_priv(wiphy);
	struct ath6kl_vif *vif = netdev_priv(ndev);

	if (!__ath6kl_cfg80211_ready(ar))
		return -EIO;

	if (down_interruptible(&ar->sem)) {
		ath6kl_err("busy, couldn't get access\n");
		return -ERESTARTSYS;
	}

	/* fix EV110820 */
	clear_bit(CLEAR_BSSFILTER_ON_BEACON, &vif->flags);
	ath6kl_wmi_bssfilter_cmd(ar->wmi, vif->fw_vif_idx,
				NONE_BSS_FILTER, 0);

	ath6kl_judge_roam_parameter(vif, true);
	ath6kl_switch_parameter_based_on_connection(vif, true);

	spin_lock_bh(&ar->list_lock);
	list_del(&vif->list);
	spin_unlock_bh(&ar->list_lock);

	ath6kl_cleanup_vif(vif, test_bit(WMI_READY, &ar->flag));

	ath6kl_deinit_if_data(vif);

	up(&ar->sem);

	return 0;
}

#ifdef CFG80211_NETDEV_REPLACED_BY_WDEV
static struct wireless_dev *ath6kl_cfg80211_add_iface(struct wiphy *wiphy,
						    const char *name,
						    enum nl80211_iftype type,
						    u32 *flags,
						    struct vif_params *params)
{
	struct net_device *dev;

	dev = _ath6kl_cfg80211_add_iface(wiphy,
					(char *)name,
					type,
					flags,
					params);

	if (dev)
		return dev->ieee80211_ptr;
	else
		return ERR_PTR(-EINVAL);
}

static int ath6kl_cfg80211_del_iface(struct wiphy *wiphy,
				     struct wireless_dev *wdev)
{
	BUG_ON(!wdev->netdev);

	return _ath6kl_cfg80211_del_iface(wiphy, wdev->netdev);
}
#else
static struct net_device *ath6kl_cfg80211_add_iface(struct wiphy *wiphy,
						    char *name,
						    enum nl80211_iftype type,
						    u32 *flags,
						    struct vif_params *params)
{
	struct net_device *dev;

	dev = _ath6kl_cfg80211_add_iface(wiphy,
					name,
					type,
					flags,
					params);

	if (dev)
		return dev;
	else
		return ERR_PTR(-EINVAL);
}

static int ath6kl_cfg80211_del_iface(struct wiphy *wiphy,
				     struct net_device *ndev)
{
	return _ath6kl_cfg80211_del_iface(wiphy, ndev);
}
#endif

static int ath6kl_cfg80211_change_iface(struct wiphy *wiphy,
					struct net_device *ndev,
					enum nl80211_iftype type, u32 *flags,
					struct vif_params *params)
{
	struct ath6kl_vif *vif = netdev_priv(ndev);

	ath6kl_dbg(ATH6KL_DBG_WLAN_CFG, "%s: type %u\n", __func__, type);

	if (down_interruptible(&vif->ar->sem)) {
		ath6kl_err("busy, couldn't get access\n");
		return -ERESTARTSYS;
	}

	switch (type) {
	case NL80211_IFTYPE_STATION:
		vif->next_mode = INFRA_NETWORK;
		break;
	case NL80211_IFTYPE_ADHOC:
		vif->next_mode = ADHOC_NETWORK;
		break;
	case NL80211_IFTYPE_AP:
		vif->next_mode = AP_NETWORK;
		break;
	case NL80211_IFTYPE_P2P_CLIENT:
		vif->next_mode = INFRA_NETWORK;
		break;
	case NL80211_IFTYPE_P2P_GO:
		vif->next_mode = AP_NETWORK;
		break;
	default:
		ath6kl_err("invalid interface type %u\n", type);
		up(&vif->ar->sem);
		return -EOPNOTSUPP;
	}

	vif->wdev.iftype = type;

	up(&vif->ar->sem);

	return 0;
}

static int ath6kl_cfg80211_join_ibss(struct wiphy *wiphy,
				     struct net_device *dev,
				     struct cfg80211_ibss_params *ibss_param)
{
	struct ath6kl *ar = ath6kl_priv(dev);
	struct ath6kl_vif *vif = netdev_priv(dev);
	struct ieee80211_channel *chan;
	int status;

	if (!ath6kl_cfg80211_ready(vif))
		return -EIO;

	if (down_interruptible(&ar->sem)) {
		ath6kl_err("busy, couldn't get access\n");
		return -ERESTARTSYS;
	}

	vif->ssid_len = ibss_param->ssid_len;
	memcpy(vif->ssid, ibss_param->ssid, vif->ssid_len);

#ifdef CFG80211_NEW_CHAN_DEFINITION
	chan = ibss_param->chandef.chan;
#else
	chan = ibss_param->channel;
#endif

	if (chan)
		vif->ch_hint = chan->center_freq;

	if (ibss_param->channel_fixed) {
		/*
		 * TODO: channel_fixed: The channel should be fixed, do not
		 * search for IBSSs to join on other channels. Target
		 * firmware does not support this feature, needs to be
		 * updated.
		 */
		up(&ar->sem);
		return -EOPNOTSUPP;
	}
	/* Diable background scan */
	vif->sc_params.bg_period = 0xFFFF;
#ifdef ATH6KL_SUPPORT_WIFI_KTK
	vif->sc_params.minact_chdwell_time = 0;
	vif->sc_params.maxact_chdwell_time = 105;
#endif
	ath6kl_wmi_scanparams_cmd(ar->wmi, vif->fw_vif_idx,
				vif->sc_params.fg_start_period,
				vif->sc_params.fg_end_period,
				vif->sc_params.bg_period,
				vif->sc_params.minact_chdwell_time,
				vif->sc_params.maxact_chdwell_time,
				vif->sc_params.pas_chdwell_time,
				vif->sc_params.short_scan_ratio,
				vif->sc_params.scan_ctrl_flags,
				vif->sc_params.max_dfsch_act_time,
				vif->sc_params.maxact_scan_per_ssid);

	memset(vif->req_bssid, 0, sizeof(vif->req_bssid));
	if (ibss_param->bssid && !is_broadcast_ether_addr(ibss_param->bssid))
		memcpy(vif->req_bssid, ibss_param->bssid,
		       sizeof(vif->req_bssid));

	ath6kl_set_wpa_version(vif, 0);

	status = ath6kl_set_auth_type(vif, NL80211_AUTHTYPE_OPEN_SYSTEM);
	if (status) {
		up(&ar->sem);
		return status;
	}

#ifdef ATH6KL_SUPPORT_WIFI_KTK
	if (!ar->ktk_active) {
#endif
		if (ibss_param->privacy) {
			ath6kl_set_cipher(vif, WLAN_CIPHER_SUITE_WEP40, true);
			ath6kl_set_cipher(vif, WLAN_CIPHER_SUITE_WEP40, false);
		} else {
			ath6kl_set_cipher(vif, 0, true);
			ath6kl_set_cipher(vif, 0, false);
		}
#ifdef ATH6KL_SUPPORT_WIFI_KTK
	} else {
		ath6kl_set_cipher(vif, WLAN_CIPHER_SUITE_CCMP, true);
		ath6kl_set_cipher(vif, WLAN_CIPHER_SUITE_CCMP, false);
	}
#endif

	vif->nw_type = vif->next_mode;

	ath6kl_dbg(ATH6KL_DBG_WLAN_CFG,
		   "%s: connect called with authmode %d dot11 auth %d"
		   " PW crypto %d PW crypto len %d GRP crypto %d"
		   " GRP crypto len %d channel hint %u\n",
		   __func__,
		   vif->auth_mode, vif->dot11_auth_mode, vif->prwise_crypto,
		   vif->prwise_crypto_len, vif->grp_crypto,
		   vif->grp_crypto_len, vif->ch_hint);

	status = ath6kl_wmi_connect_cmd(ar->wmi, vif->fw_vif_idx, vif->nw_type,
					vif->dot11_auth_mode, vif->auth_mode,
					vif->prwise_crypto,
					vif->prwise_crypto_len,
					vif->grp_crypto, vif->grp_crypto_len,
					vif->ssid_len, vif->ssid,
					vif->req_bssid, vif->ch_hint,
					vif->connect_ctrl_flags);
	set_bit(CONNECT_PEND, &vif->flags);

	up(&ar->sem);

	return 0;
}

static int ath6kl_cfg80211_leave_ibss(struct wiphy *wiphy,
				      struct net_device *dev)
{
	struct ath6kl_vif *vif = netdev_priv(dev);

	if (!ath6kl_cfg80211_ready(vif))
		return -EIO;

	if (down_interruptible(&vif->ar->sem)) {
		ath6kl_err("busy, couldn't get access\n");
		return -ERESTARTSYS;
	}

	ath6kl_disconnect(vif);
	memset(vif->ssid, 0, sizeof(vif->ssid));
	vif->ssid_len = 0;

	up(&vif->ar->sem);

	return 0;
}

static const u32 cipher_suites[] = {
	WLAN_CIPHER_SUITE_WEP40,
	WLAN_CIPHER_SUITE_WEP104,
	WLAN_CIPHER_SUITE_TKIP,
	WLAN_CIPHER_SUITE_CCMP,
	CCKM_KRK_CIPHER_SUITE,
	WLAN_CIPHER_SUITE_SMS4,
#ifdef PMF_SUPPORT
	WLAN_CIPHER_SUITE_AES_CMAC,
#endif
};

static bool is_rate_legacy(s32 rate)
{
	static const s32 legacy[] = { 1000, 2000, 5500, 11000,
		6000, 9000, 12000, 18000, 24000,
		36000, 48000, 54000
	};
	u8 i;

	for (i = 0; i < ARRAY_SIZE(legacy); i++)
		if (rate == legacy[i])
			return true;

	return false;
}

static bool is_rate_ht20(s32 rate, u8 *mcs, bool *sgi)
{
	static const s32 ht20[] = { 6500, 13000, 19500, 26000, 39000,
		52000, 58500, 65000
	};
	static const s32 ht20_sgi[] = { 7200, 14400, 21700, 28900, 43300,
		57800, 65000, 72200
	};
	u8 i;

	for (i = 0; i < ARRAY_SIZE(ht20); i++) {
		if (rate == ht20[i]) {
			*sgi = false;
			*mcs = i;
			return true;
		}
	}

	for (i = 0; i < ARRAY_SIZE(ht20_sgi); i++) {
		if (rate == ht20_sgi[i]) {
			*sgi = true;
			*mcs = i;
			return true;
		}
	}
	return false;
}

static bool is_rate_ht40(s32 rate, u8 *mcs, bool *sgi)
{
	static const s32 ht40[] = { 13500, 27000, 40500, 54000,
		81000, 108000, 121500, 135000
	};
	static const s32 ht40_sgi[] = { 15000, 30000, 45000, 60000,
		90000, 120000, 135000, 150000
	};
	u8 i;

	for (i = 0; i < ARRAY_SIZE(ht40); i++) {
		if (rate == ht40[i]) {
			*sgi = false;
			*mcs = i;
			return true;
		}
	}

	for (i = 0; i < ARRAY_SIZE(ht40_sgi); i++) {
		if (rate == ht40_sgi[i]) {
			*sgi = true;
			*mcs = i;
			return true;
		}
	}

	return false;
}

static int __get_rate_info(struct rate_info *txrate, s32 rate, s8 rate_idx)
{
	s8 rate_id = rate_idx & 0x7f;

	txrate->flags = 0;
	if (rate_id <= 11) {
		txrate->legacy = rate / 100;
	} else if (rate_id <= 43) {
		txrate->flags |= RATE_INFO_FLAGS_MCS;
		if (rate_idx >> 7)
			txrate->flags |= RATE_INFO_FLAGS_SHORT_GI;

		if (rate_id <= 27)
			txrate->mcs = rate_id - 12;
		else {
			txrate->mcs = rate_id - 28;
			txrate->flags |= RATE_INFO_FLAGS_40_MHZ_WIDTH;
		}
	} else {
		ath6kl_dbg(ATH6KL_DBG_WLAN_CFG,
			"invalid rate from ar6004 stats: rate %d idx %d\n",
			rate, rate_idx);

		return -1;
	}

	return 0;
}

static void _get_sta_info(struct ath6kl_vif *vif,
			struct station_info *sinfo)
{
	struct target_stats *stats = &vif->target_stats;
	s32 rate;
	s8 rateid;
	u8 mcs;
	bool sgi;

	/* Not only STA but also IBSS mode. */

	if (stats->rx_byte) {
		sinfo->rx_bytes = stats->rx_byte;
		sinfo->filled |= STATION_INFO_RX_BYTES;
		sinfo->rx_packets = stats->rx_pkt;
		sinfo->filled |= STATION_INFO_RX_PACKETS;
	}

	if (stats->tx_byte) {
		sinfo->tx_bytes = stats->tx_byte;
		sinfo->filled |= STATION_INFO_TX_BYTES;
		sinfo->tx_packets = stats->tx_pkt;
		sinfo->filled |= STATION_INFO_TX_PACKETS;
	}

	sinfo->signal = stats->cs_rssi;
	sinfo->filled |= STATION_INFO_SIGNAL;

	rate = stats->tx_ucast_rate;

	if (vif->ar->target_type == TARGET_TYPE_AR6004) {
		rateid = stats->tx_rate_index;
		if (__get_rate_info(&sinfo->txrate, rate, rateid))
			ath6kl_debug_war(vif->ar, ATH6KL_WAR_INVALID_RATE);
		else
			sinfo->filled |= STATION_INFO_TX_BITRATE;
	} else {
		sinfo->txrate.flags = 0;
		if (is_rate_legacy(rate)) {
			sinfo->txrate.legacy = rate / 100;
			sinfo->filled |= STATION_INFO_TX_BITRATE;
		} else if (is_rate_ht20(rate, &mcs, &sgi)) {
			if (sgi)
				sinfo->txrate.flags |= RATE_INFO_FLAGS_SHORT_GI;
			sinfo->txrate.mcs = mcs;
			sinfo->txrate.flags |= RATE_INFO_FLAGS_MCS;
			sinfo->filled |= STATION_INFO_TX_BITRATE;
		} else if (is_rate_ht40(rate, &mcs, &sgi)) {
			if (sgi)
				sinfo->txrate.flags |= RATE_INFO_FLAGS_SHORT_GI;
			sinfo->txrate.mcs = mcs;
			sinfo->txrate.flags |= RATE_INFO_FLAGS_40_MHZ_WIDTH;
			sinfo->txrate.flags |= RATE_INFO_FLAGS_MCS;
			sinfo->filled |= STATION_INFO_TX_BITRATE;
		} else {
			ath6kl_dbg(ATH6KL_DBG_WLAN_CFG,
				"invalid rate from stats: %d\n", rate);
			ath6kl_debug_war(vif->ar, ATH6KL_WAR_INVALID_RATE);
		}
	}

	if ((vif->nw_type == INFRA_NETWORK) &&
	    test_bit(CONNECTED, &vif->flags) &&
	    test_bit(DTIM_PERIOD_AVAIL, &vif->flags) &&
	    vif->nw_type == INFRA_NETWORK) {
		sinfo->filled |= STATION_INFO_BSS_PARAM;
		sinfo->bss_param.flags = 0;
		sinfo->bss_param.dtim_period = vif->assoc_bss_dtim_period;
		sinfo->bss_param.beacon_interval = vif->assoc_bss_beacon_int;
	}

	return;
}

static void _get_ap_sta_info(struct ath6kl_vif *vif,
			struct wmi_per_sta_stat *sta,
			u8 *mac_addr,
			struct station_info *sinfo)
{
	s32 rate;
	s8 rateid;

	if (ath6kl_ap_keepalive_by_supp(vif))
		sinfo->inactive_time =
			ath6kl_ap_keepalive_get_inactive_time(vif, mac_addr);
	else {
		/*
		 * Always report 1 sec. to let supplicant bypass
		 * its keep-alive mechanims.
		 */
		sinfo->inactive_time = 1 * 1000;
	}
	sinfo->filled |= STATION_INFO_INACTIVE_TIME;

	sinfo->rx_bytes = sta->rx_bytes;
	sinfo->filled |= STATION_INFO_RX_BYTES;
	sinfo->rx_packets = sta->rx_pkts;
	sinfo->filled |= STATION_INFO_RX_PACKETS;

	sinfo->tx_bytes = sta->tx_bytes;
	sinfo->filled |= STATION_INFO_TX_BYTES;
	sinfo->tx_packets = sta->tx_pkts;
	sinfo->filled |= STATION_INFO_TX_PACKETS;

	if (vif->ar->target_type == TARGET_TYPE_AR6004) {
		rate = ath6kl_wmi_get_rate_ar6004(sta->tx_ucast_rate);
		rateid = sta->tx_ucast_rate;
		if (__get_rate_info(&sinfo->txrate, rate, rateid))
			ath6kl_debug_war(vif->ar, ATH6KL_WAR_INVALID_RATE);
		else
			sinfo->filled |= STATION_INFO_TX_BITRATE;
	}

	return;
}

static int ath6kl_get_station(struct wiphy *wiphy, struct net_device *dev,
			      u8 *mac, struct station_info *sinfo)
{
	struct ath6kl *ar = ath6kl_priv(dev);
	struct ath6kl_vif *vif = netdev_priv(dev);
	struct ath6kl_sta *conn = NULL;
	long left;

	if (!ath6kl_cfg80211_ready(vif))
		return -EIO;

	if (vif->nw_type == AP_NETWORK) {
		conn = ath6kl_find_sta(vif, mac);
		if (conn == NULL) {
			ath6kl_err("Can't find %02x%02x%02x%02x%02x%02x\n",
					mac[0], mac[1], mac[2],
					mac[3], mac[4], mac[5]);

			return -EINVAL;
		}
	} else {
		if (memcmp(mac, vif->bssid, ETH_ALEN) != 0)
			return -ENOENT;
	}

#ifdef USB_AUTO_SUSPEND
	/* skip to update FW statistics in 1min when wow suspend */
	if (ar->state == ATH6KL_STATE_WOW) {
		struct timeval cur_time;
		long diff_time;

		do_gettimeofday(&cur_time);
		if (cur_time.tv_sec >= vif->target_stats.update_time.tv_sec) {
			diff_time = cur_time.tv_sec
				- vif->target_stats.update_time.tv_sec;

			if (diff_time <= 60)
				goto skip_fw_stats;
		}
	}
#endif

	if (down_interruptible(&ar->sem))
		return -EBUSY;

	set_bit(STATS_UPDATE_PEND, &vif->flags);

	if (ath6kl_wmi_get_stats_cmd(ar->wmi, vif->fw_vif_idx)) {
		up(&ar->sem);
		return -EIO;
	}

	left = wait_event_interruptible_timeout(ar->event_wq,
						!test_bit(STATS_UPDATE_PEND,
							  &vif->flags),
						WMI_TIMEOUT);

	clear_bit(STATS_UPDATE_PEND, &vif->flags);
	up(&ar->sem);

	if (left == 0)
		return -ETIMEDOUT;
	else if (left < 0)
		return left;

#ifdef USB_AUTO_SUSPEND
skip_fw_stats:
#endif

	if (vif->nw_type == AP_NETWORK) {
		struct wmi_ap_mode_stat *ap = &vif->ap_stats;
		struct wmi_per_sta_stat *sta = NULL;

		for (left = 0; left < AP_MAX_NUM_STA; left++) {
			if (conn->aid == ap->sta[left].aid) {
				sta = &ap->sta[left];
				break;
			}
		}

		if (!sta)
			return -EINVAL;

		_get_ap_sta_info(vif, sta, mac, sinfo);

		return 0;
	}

	_get_sta_info(vif, sinfo);

	return 0;
}

static int ath6kl_dump_station(struct wiphy *wiphy, struct net_device *dev,
			       int idx, u8 *mac, struct station_info *sinfo)
{
	struct ath6kl *ar = ath6kl_priv(dev);
	struct ath6kl_vif *vif = netdev_priv(dev);
	struct ath6kl_sta *conn;
	struct wmi_ap_mode_stat *ap;
	struct wmi_per_sta_stat *sta;
	long left;
	int next;

	if (!ath6kl_cfg80211_ready(vif))
		return -EIO;

	if (vif->nw_type == AP_NETWORK) {
		/* Get AP stats only at least one station associated. */
		if ((vif->sta_list_index == 0) ||
		    (idx >= AP_MAX_NUM_STA))
			return -ENOENT;

		/* Only need to update it when 1st STA dump. */
		if (idx == 0)
			vif->last_dump_ap_stats_idx = 0;
		else
			goto update_done;
	} else {
		if ((idx != 0) ||
		    !test_bit(CONNECTED, &vif->flags))
			return -ENOENT;
	}

	if (down_interruptible(&ar->sem))
		return -EBUSY;

	set_bit(STATS_UPDATE_PEND, &vif->flags);

	if (ath6kl_wmi_get_stats_cmd(ar->wmi, vif->fw_vif_idx)) {
		up(&ar->sem);
		return -EIO;
	}

	left = wait_event_interruptible_timeout(ar->event_wq,
						!test_bit(STATS_UPDATE_PEND,
							  &vif->flags),
						WMI_TIMEOUT);

	clear_bit(STATS_UPDATE_PEND, &vif->flags);
	up(&ar->sem);

	if (left == 0)
		return -ETIMEDOUT;
	else if (left < 0)
		return left;

update_done:
	if (vif->nw_type == AP_NETWORK) {
		if (vif->last_dump_ap_stats_idx >= AP_MAX_NUM_STA)
			return -ENOENT;

		/* Find next STA */
		ap = &vif->ap_stats;
		for (next = vif->last_dump_ap_stats_idx;
		     next < AP_MAX_NUM_STA;
		     next++) {
			sta = &(ap->sta[next]);
			if (sta->aid) {
				conn = ath6kl_find_sta_by_aid(vif, sta->aid);
				if (conn)
					break;
			}
		}

		/* All STAs reported */
		if (next == AP_MAX_NUM_STA)
			return -ENOENT;

		/* Set to the next one. */
		vif->last_dump_ap_stats_idx = next + 1;

		memcpy(mac, conn->mac, ETH_ALEN);
		_get_ap_sta_info(vif, sta, conn->mac, sinfo);

		return 0;
	}

	memcpy(mac, vif->ndev->dev_addr, ETH_ALEN);
	_get_sta_info(vif, sinfo);

	return 0;
}

static int ath6kl_cfg80211_set_bitrate_mask(struct wiphy *wiphy,
				    struct net_device *dev,
				    const u8 *peer,
				    const struct cfg80211_bitrate_mask *mask)
{
	struct ath6kl *ar = ath6kl_priv(dev);
	struct ath6kl_vif *vif = netdev_priv(dev);
	int ret, disabled;

	WARN_ON(peer);
	if ((peer) ||
		((mask->control[NL80211_BAND_2GHZ].legacy != 0xfff) &&
		 (mask->control[NL80211_BAND_2GHZ].legacy != 0xff0))) {
		/* FIXME : not support yet */
		return -ENOTSUPP;
	}

	if (!ath6kl_cfg80211_ready(vif))
		return -EIO;

	if (down_interruptible(&ar->sem)) {
		ath6kl_err("busy, couldn't get access\n");
		return -ERESTARTSYS;
	}

	if (mask->control[NL80211_BAND_2GHZ].legacy &
		((1 << ath6kl_b_rates_size) - 1))
		disabled = 0;
	else
		disabled = 1;

	ret = ath6kl_wmi_disable_11b_rates_cmd(ar->wmi, vif->fw_vif_idx,
						disabled);

	up(&ar->sem);

	return ret;
}

static int ath6kl_set_pmksa(struct wiphy *wiphy, struct net_device *netdev,
			    struct cfg80211_pmksa *pmksa)
{
	struct ath6kl *ar = ath6kl_priv(netdev);
	struct ath6kl_vif *vif = netdev_priv(netdev);
	int ret;

	if (!ath6kl_cfg80211_ready(vif))
		return -EIO;

	if (down_interruptible(&ar->sem)) {
		ath6kl_err("busy, couldn't get access\n");
		return -ERESTARTSYS;
	}

	ret = ath6kl_wmi_setpmkid_cmd(ar->wmi, vif->fw_vif_idx, pmksa->bssid,
				       pmksa->pmkid, true);

	up(&ar->sem);

	return ret;
}

static int ath6kl_del_pmksa(struct wiphy *wiphy, struct net_device *netdev,
			    struct cfg80211_pmksa *pmksa)
{
	struct ath6kl *ar = ath6kl_priv(netdev);
	struct ath6kl_vif *vif = netdev_priv(netdev);
	int ret;

	if (!ath6kl_cfg80211_ready(vif))
		return -EIO;

	if (down_interruptible(&ar->sem)) {
		ath6kl_err("busy, couldn't get access\n");
		return -ERESTARTSYS;
	}

	ret = ath6kl_wmi_setpmkid_cmd(ar->wmi, vif->fw_vif_idx, pmksa->bssid,
				       pmksa->pmkid, false);

	up(&ar->sem);

	return ret;
}

static int ath6kl_flush_pmksa(struct wiphy *wiphy, struct net_device *netdev)
{
	struct ath6kl *ar = ath6kl_priv(netdev);
	struct ath6kl_vif *vif = netdev_priv(netdev);
	int ret;

	if (!ath6kl_cfg80211_ready(vif))
		return -EIO;

	if (down_interruptible(&ar->sem)) {
		ath6kl_err("busy, couldn't get access\n");
		return -ERESTARTSYS;
	}

	if (test_bit(CONNECTED, &vif->flags)) {
		ret = ath6kl_wmi_setpmkid_cmd(ar->wmi, vif->fw_vif_idx,
					       vif->bssid, NULL, false);
		up(&ar->sem);
		return ret;
	}

	up(&ar->sem);

	return 0;
}

static int ath6kl_wow_suspend(struct ath6kl *ar, struct cfg80211_wowlan *wow)
{
	struct ath6kl_vif *vif;
	int ret, left;
#ifndef CONFIG_ANDROID
	int pos;
	u32 filter = 0;
	u8 mask[WOW_MASK_SIZE];
#endif
	u16 i;
	struct in_device *in_dev;
	struct in_ifaddr *ifa;
	unsigned char src_ip[4];

	/*if already in wow state just return without error*/
	if (ar->state == ATH6KL_STATE_WOW)
		return 0;

	vif = ath6kl_vif_first(ar);
	if (!vif)
		return -EIO;

	if (!ath6kl_cfg80211_ready(vif))
		return -EIO;

#if (!defined(CONFIG_ANDROID) && !defined(USB_AUTO_SUSPEND))
	if (!ar->get_wow_pattern) {
		/* Clear existing WOW patterns */
		for (i = 0; i < WOW_MAX_FILTERS_PER_LIST; i++)
			ath6kl_wmi_del_wow_pattern_cmd(ar->wmi, vif->fw_vif_idx,
							   WOW_LIST_ID, i);
		/* Configure new WOW patterns */
		for (i = 0; i < wow->n_patterns; i++) {
			if (ath6kl_wow_ext) {
				ret = ath6kl_wmi_add_wow_ext_pattern_cmd(
					ar->wmi,
					vif->fw_vif_idx, WOW_LIST_ID,
					wow->patterns[i].pattern_len,
					i,
					0,
					wow->patterns[i].pattern,
					wow->patterns[i].mask);
				/* filter for wow ext pattern */
				filter |= WOW_FILTER_OPTION_PATTERNS;
			} else {
				/*
				 * Convert given nl80211 specific mask value to
				 * equivalent driver specific mask value
				 * and send it to the chip along with patterns.
				 * For example, if the mask value defined
				 * in struct cfg80211_wowlan is 0xA
				 * (equivalent binary is 1010), then equivalent
				 * driver specific mask value is
				 * "0xFF 0x00 0xFF 0x00".
				 */
				memset(&mask, 0, sizeof(mask));
				for (pos = 0;
					pos < wow->patterns[i].pattern_len;
					pos++) {
					if (wow->patterns[i].mask[pos / 8] &
					(0x1 << (pos % 8)))
						mask[pos] = 0xFF;
				}
				/*
				 * Note: Pattern's offset is not passed
				 * as part of wowlan parameter from CFG layer.
				 * So it's always passed as ZERO to
				 * the firmware.
				 * It means, given WOW patterns are always
				 * matched from the first byte
				 * of received pkt in the firmware.
				 */
				ret = ath6kl_wmi_add_wow_pattern_cmd(ar->wmi,
						vif->fw_vif_idx, WOW_LIST_ID,
						wow->patterns[i].pattern_len,
						0 /* pattern offset */,
						wow->patterns[i].pattern, mask);
			}
			if (ret)
				return ret;
		}
	} else
		filter |= WOW_FILTER_OPTION_PATTERNS;

	if (wow) {
		if (wow->disconnect || wow->any)
			filter |= WOW_FILTER_OPTION_NWK_DISASSOC;

		if (wow->magic_pkt || wow->any)
			filter |= WOW_FILTER_OPTION_MAGIC_PACKET;

		if (wow->gtk_rekey_failure || wow->any) {
			filter |= (WOW_FILTER_OPTION_EAP_REQ |
					WOW_FILTER_OPTION_8021X_4WAYHS |
					WOW_FILTER_OPTION_GTK_ERROR |
					WOW_FILTER_OPTION_OFFLOAD_GTK);
		}

		if (wow->eap_identity_req || wow->any)
			filter |= WOW_FILTER_OPTION_EAP_REQ;

		if (wow->four_way_handshake || wow->any)
			filter |= WOW_FILTER_OPTION_8021X_4WAYHS;

		if (vif->arp_offload_ip_set || wow->any)
			filter |= WOW_FILTER_OPTION_OFFLOAD_ARP;
	}

	/*Do GTK offload in WPA/WPA2 auth mode connection.*/
	if (vif->auth_mode == WPA2_AUTH_CCKM || vif->auth_mode == WPA2_PSK_AUTH
	  || vif->auth_mode == WPA_AUTH_CCKM || vif->auth_mode == WPA_PSK_AUTH){
		filter |= WOW_FILTER_OPTION_OFFLOAD_GTK;
	}

	if (filter || (wow && wow->n_patterns)) {
		ret = ath6kl_wmi_set_wow_mode_cmd(ar->wmi, vif->fw_vif_idx,
						  ATH6KL_WOW_MODE_ENABLE,
						  filter,
						  WOW_HOST_REQ_DELAY);
		if (ret)
			return ret;
	}
#endif /*!CONFIG_ANDROID*/

	/* Setup own IP addr for ARP agent. */
	for (i = 0; i < ar->vif_max; i++) {
		struct ath6kl_vif *mvif = ath6kl_get_vif_by_index(ar, i);
		if (!mvif)
			continue;

		memset(src_ip, 0x0, 4);
		in_dev = __in_dev_get_rtnl(mvif->ndev);
		if (in_dev) {
			ifa = in_dev->ifa_list;
			if (ifa && ifa->ifa_local) {
				if (mvif->arp_offload_ip != ifa->ifa_local) {
					memcpy(src_ip, &ifa->ifa_local, 4);
					if (!ath6kl_wmi_set_arp_offload_ip_cmd(
						ar->wmi, mvif->fw_vif_idx,
						src_ip)) {
						mvif->arp_offload_ip =
							ifa->ifa_local;
						ath6kl_dbg(ATH6KL_DBG_WOWLAN,
							"%s: enable %s arp offload %d.%d.%d.%d\n",
							__func__,
							mvif->ndev->name,
							src_ip[0], src_ip[1],
							src_ip[2], src_ip[3]);
					}
				}
			} else if (mvif->arp_offload_ip != 0) {
				if (!ath6kl_wmi_set_arp_offload_ip_cmd(
						ar->wmi, mvif->fw_vif_idx,
						src_ip)) {
						mvif->arp_offload_ip = 0;
						ath6kl_dbg(ATH6KL_DBG_WOWLAN,
							"%s: disable %s arp offload\n",
							__func__,
							mvif->ndev->name);
				}
			}
		}
	}

	clear_bit(HOST_SLEEP_MODE_CMD_PROCESSED, &vif->flags);

	ret = ath6kl_wmi_set_host_sleep_mode_cmd(ar->wmi, vif->fw_vif_idx,
						 ATH6KL_HOST_MODE_ASLEEP);

	if (ret)
		return ret;

	left = wait_event_interruptible_timeout(ar->event_wq,
			test_bit(HOST_SLEEP_MODE_CMD_PROCESSED, &vif->flags),
			WMI_TIMEOUT);

	if (left == 0) {
		ath6kl_warn("timeout, didn't get host sleep cmd "
				"processed event\n");
		ret = -ETIMEDOUT;
	} else if (left < 0) {
		ath6kl_warn("error while waiting for host sleep cmd "
				"processed event %d\n", left);
		ret = left;
	}

	if (ar->tx_pending[ar->ctrl_ep]) {
		left = wait_event_interruptible_timeout(ar->event_wq,
				ar->tx_pending[ar->ctrl_ep] == 0, WMI_TIMEOUT);
		if (left == 0) {
			ath6kl_warn("clear wmi ctrl data timeout\n");
			ret = -ETIMEDOUT;
		} else if (left < 0) {
			ath6kl_warn("clear wmi ctrl data failed: %d\n", left);
			ret = left;
		}
	}

#ifdef CONFIG_ANDROID
	if (ret)
		ath6kl_wmi_set_host_sleep_mode_cmd(ar->wmi, vif->fw_vif_idx,
						 ATH6KL_HOST_MODE_AWAKE);
#endif
	return ret;
}

static int ath6kl_wow_resume(struct ath6kl *ar)
{
	struct ath6kl_vif *vif;
	int ret;

	vif = ath6kl_vif_first(ar);
	if (!vif)
		return -EIO;

	ret = ath6kl_wmi_set_host_sleep_mode_cmd(ar->wmi, vif->fw_vif_idx,
						 ATH6KL_HOST_MODE_AWAKE);
	return ret;
}

#ifdef CONFIG_ANDROID
static irqreturn_t ath6kl_wow_irq(int irq, void *dev_id)
{
	struct rttm_context *prttm = NULL;

	prttm = DEV_GETRTT_HDL();

	if ((prttm) && (prttm->rttdhclkcal_active)) {
		struct timespec ts;

		getnstimeofday(&ts);
		prttm->rttd2h2_clk.tabs_h2[prttm->dhclkcal_index].sec =
			ts.tv_sec;
		prttm->rttd2h2_clk.tabs_h2[prttm->dhclkcal_index].nsec =
			ts.tv_nsec;
		prttm->dhclkcal_index++;
	}

	return IRQ_HANDLED;
}
#endif

static bool ath6kl_cfg80211_need_suspend(struct ath6kl *ar, u32 *suspend_vif)
{
	struct ath6kl_vif *vif;
	int i;

	vif = ath6kl_vif_first(ar);
	if (!vif)
		return false;

	if (!test_bit(WMI_READY, &ar->flag)) {
		ath6kl_err("deepsleep failed as wmi is not ready\n");
		return false;
	}

	/*
	 * If one of all virtual interfaces is AP mode then
	 * force to awake.
	 */
	*suspend_vif = 0;
	for (i = 0; i < ar->vif_max; i++) {
		vif = ath6kl_get_vif_by_index(ar, i);
		if (vif) {
			#ifdef CE_SUPPORT
			if (vif->nw_type != AP_NETWORK)
				*suspend_vif |= (1 << i);
			#else
			if (vif->nw_type == AP_NETWORK) {
				*suspend_vif = 0;
				return false;
			}
			*suspend_vif |= (1 << i);
			#endif
		}
	}

	return true;
}

static int ath6kl_cfg80211_deepsleep_suspend(struct ath6kl *ar)
{
	struct ath6kl_vif *vif;
	int i, ret = 0, left;
	u32 need_suspend_vif = 0;

	if (!ath6kl_cfg80211_need_suspend(ar, &need_suspend_vif))
		return -EOPNOTSUPP;


	ath6kl_dbg(ATH6KL_DBG_SUSPEND, "deep sleep suspend %x\n",
					need_suspend_vif);

#ifdef CONFIG_ANDROID
	if (ar->wow_irq) {
		if (disable_irq_wake(ar->wow_irq))
			ath6kl_err("Couldn't disable hostwake IRQ wakeup mode\n");

		free_irq(ar->wow_irq, ar);
	}
#endif

	for (i = 0; i < ar->vif_max; i++) {
		if (need_suspend_vif & (1 << i)) {
			vif = ath6kl_get_vif_by_index(ar, i);
			if (vif) {
				clear_bit(WLAN_ENABLED, &vif->flags);
				netif_dormant_on(vif->ndev);
				set_bit(DORMANT, &vif->flags);
			}
		}
	}

#ifdef USB_AUTO_SUSPEND
	/* in mck3.0, we won't have credit full problem as before,
	   so we could move ATH6KL_STATE_DEEPSLEEP after stop_all()
	*/
	ath6kl_cfg80211_stop_all(ar);

	spin_lock_bh(&ar->state_lock);
	ar->state = ATH6KL_STATE_DEEPSLEEP;
	spin_unlock_bh(&ar->state_lock);
#else
	spin_lock_bh(&ar->state_lock);
	ar->state = ATH6KL_STATE_DEEPSLEEP;
	spin_unlock_bh(&ar->state_lock);

	ath6kl_cfg80211_stop_all(ar);
#endif

	/*
	 * Flush data packets and wait for all control packets
	 * to be cleared in TX path before deep sleep suspend.
	 */
	ath6kl_tx_data_cleanup(ar);

	if (ar->tx_pending[ar->ctrl_ep]) {
		left = wait_event_interruptible_timeout(ar->event_wq,
				ar->tx_pending[ar->ctrl_ep] == 0, WMI_TIMEOUT);
		if (left == 0) {
			ath6kl_warn("clear wmi ctrl data timeout txpend %d\n",
			ar->tx_pending[ar->ctrl_ep]);
			ret = -ETIMEDOUT;
		} else if (left < 0) {
			ath6kl_warn("clear wmi ctrl data failed:%d tx_pend=%d\n",
			left, ar->tx_pending[ar->ctrl_ep]);
			ret = left;
		}
	}

	return ret;
}

int ath6kl_cfg80211_suspend(struct ath6kl *ar,
			    enum ath6kl_cfg_suspend_mode mode,
			    struct cfg80211_wowlan *wow)
{
	int ret;
	u32 need_suspend_vif = 0;
	/* make sure no AP mode at any vif*/
	if (ath6kl_cfg80211_need_suspend(ar, &need_suspend_vif)
		&& ar->get_wow_pattern == true)
		mode = ATH6KL_CFG_SUSPEND_WOW;

	switch (mode) {
	case ATH6KL_CFG_SUSPEND_WOW:

		ath6kl_dbg(ATH6KL_DBG_SUSPEND, "wow mode suspend\n");

		/* Flush all non control pkts in TX path */
		ath6kl_tx_data_cleanup(ar);
#ifdef USB_AUTO_SUSPEND
		ar->state = ATH6KL_STATE_PRE_SUSPEND;
#endif
		ret = ath6kl_wow_suspend(ar, wow);
		if (ret) {
			ath6kl_err("wow suspend failed: %d\n", ret);
#ifdef USB_AUTO_SUSPEND
			ar->state = ATH6KL_STATE_WOW;
#endif
			return ret;
		}
		spin_lock_bh(&ar->state_lock);
		ar->state = ATH6KL_STATE_WOW;
		spin_unlock_bh(&ar->state_lock);
		break;

	case ATH6KL_CFG_SUSPEND_DEEPSLEEP:
		if (ar->state == ATH6KL_STATE_DEEPSLEEP)
			break;

		ath6kl_dbg(ATH6KL_DBG_SUSPEND, "deep sleep suspend\n");

#ifdef USB_AUTO_SUSPEND
		ar->state = ATH6KL_STATE_PRE_SUSPEND_DEEPSLEEP;
#endif
		ret = ath6kl_cfg80211_deepsleep_suspend(ar);
		if (ret) {
			ath6kl_err("deepsleep suspend failed: %d\n", ret);
			return ret;
		}

		break;

	case ATH6KL_CFG_SUSPEND_CUTPOWER:

#ifdef CONFIG_ANDROID
		if (ar->wow_irq) {
			if (disable_irq_wake(ar->wow_irq))
				ath6kl_err("Couldn't disable hostwake IRQ wakeup mode\n");

			free_irq(ar->wow_irq, ar);
		}
#endif

		ath6kl_cfg80211_stop_all(ar);

		if (ar->state == ATH6KL_STATE_OFF) {
			ath6kl_dbg(ATH6KL_DBG_SUSPEND,
				   "suspend hw off, no action for cutpower\n");
			break;
		}

		ath6kl_dbg(ATH6KL_DBG_SUSPEND, "suspend cutting power\n");

		ret = ath6kl_init_hw_stop(ar);
		if (ret) {
			ath6kl_warn("failed to stop hw during suspend: %d\n",
				    ret);
		}

		ar->state = ATH6KL_STATE_CUTPOWER;

		break;

	default:
		break;
	}

	return 0;
}

int ath6kl_cfg80211_resume(struct ath6kl *ar)
{
	int i, ret;
	struct ath6kl_vif *vif;

	switch (ar->state) {
	case  ATH6KL_STATE_WOW:
		ath6kl_dbg(ATH6KL_DBG_SUSPEND, "wow mode resume\n");

#ifdef CONFIG_HAS_WAKELOCK
		wake_lock_timeout(&ar->wake_lock, 3*HZ);
#else
		/* TODO: What should I do if there is no wake lock?? */
#endif



		spin_lock_bh(&ar->state_lock);
		ar->state = ATH6KL_STATE_ON;
		spin_unlock_bh(&ar->state_lock);

		ret = ath6kl_wow_resume(ar);
		if (ret) {
			ath6kl_warn("wow mode resume failed: %d\n", ret);
			return ret;
		}

#ifdef USB_AUTO_SUSPEND
		spin_lock_bh(&ar->usb_pm_lock);
		ath6kl_auto_pm_wakeup_resume(ar);
		spin_unlock_bh(&ar->usb_pm_lock);
#endif
		break;

	case ATH6KL_STATE_DEEPSLEEP:
		ath6kl_dbg(ATH6KL_DBG_SUSPEND, "deep sleep resume\n");

		spin_lock_bh(&ar->state_lock);
		ar->state = ATH6KL_STATE_ON;
		spin_unlock_bh(&ar->state_lock);
#ifdef USB_AUTO_SUSPEND
		spin_lock_bh(&ar->usb_pm_lock);
		ath6kl_auto_pm_wakeup_resume(ar);
		spin_unlock_bh(&ar->usb_pm_lock);
#endif

#ifdef CONFIG_ANDROID
		if (ar->wow_irq) {
			int ret;
			ret = request_irq(ar->wow_irq, ath6kl_wow_irq,
				IRQF_SHARED | IRQF_TRIGGER_RISING,
				"ar6000" "sdiowakeup", ar);
			if (!ret) {
				ret = enable_irq_wake(ar->wow_irq);
				if (ret < 0) {
					ath6kl_err("Couldn't enable WoW IRQ as wakeup interrupt");
					return ret;
				}
				ath6kl_info("ath6kl: WoW IRQ %d\n",
					ar->wow_irq);
			}
		}
#endif
		for (i = 0; i < ar->vif_max; i++) {
			vif = ath6kl_get_vif_by_index(ar, i);
			if (vif) {
				if (test_bit(DORMANT, &vif->flags)) {
					set_bit(WLAN_ENABLED, &vif->flags);
					netif_dormant_off(vif->ndev);
					clear_bit(DORMANT, &vif->flags);
				}

				/* restore previous power mode */
				if (vif->last_pwr_mode != vif->saved_pwr_mode) {
					if (ath6kl_wmi_powermode_cmd(ar->wmi, i,
						vif->saved_pwr_mode) != 0) {
						ath6kl_err("wmi powermode command failed during resume\n");
					}
				}
			}
		}

		break;

	case ATH6KL_STATE_CUTPOWER:
		ath6kl_dbg(ATH6KL_DBG_SUSPEND, "resume restoring power\n");

#ifdef CONFIG_ANDROID
		if (ar->wow_irq) {
			int ret;
			ret = request_irq(ar->wow_irq, ath6kl_wow_irq,
				IRQF_SHARED | IRQF_TRIGGER_RISING,
				"ar6000" "sdiowakeup", ar);
			if (!ret) {
				ret = enable_irq_wake(ar->wow_irq);
				if (ret < 0) {
					ath6kl_err("Couldn't enable WoW IRQ as wakeup interrupt");
					return ret;
				}
				ath6kl_info("ath6kl: WoW IRQ %d\n",
					ar->wow_irq);
			}
		}
#endif

		ret = ath6kl_init_hw_start(ar);
		if (ret) {
			ath6kl_warn("Failed to boot hw in resume: %d\n", ret);
			return ret;
		}
		break;

	default:
		break;
	}

	return 0;
}

#ifdef CONFIG_PM

/* hif layer decides what suspend mode to use */
static int __ath6kl_cfg80211_suspend(struct wiphy *wiphy,
				 struct cfg80211_wowlan *wow)
{
	struct ath6kl *ar = wiphy_priv(wiphy);

#if defined(USB_AUTO_SUSPEND)
	if (BOOTSTRAP_IS_HSIC(ar->bootstrap_mode))
		return 0;
#endif

	return ath6kl_hif_suspend(ar, wow);
}

static int __ath6kl_cfg80211_resume(struct wiphy *wiphy)
{
	struct ath6kl *ar = wiphy_priv(wiphy);

	if (BOOTSTRAP_IS_HSIC(ar->bootstrap_mode))
		return 0;

	return ath6kl_hif_resume(ar);
}

/*
 * FIXME: WOW suspend mode is selected if the host sdio controller supports
 * both sdio irq wake up and keep power. The target pulls sdio data line to
 * wake up the host when WOW pattern matches. This causes sdio irq handler
 * is being called in the host side which internally hits ath6kl's RX path.
 *
 * Since sdio interrupt is not disabled, RX path executes even before
 * the host executes the actual resume operation from PM module.
 *
 * In the current scenario, WOW resume should happen before start processing
 * any data from the target. So It's required to perform WOW resume in RX path.
 * Ideally we should perform WOW resume only in the actual platform
 * resume path. This area needs bit rework to avoid WOW resume in RX path.
 *
 * ath6kl_check_wow_status() is called from ath6kl_rx().
 */
void ath6kl_check_wow_status(struct ath6kl *ar)
{
	if (ar->state == ATH6KL_STATE_WOW)
		ath6kl_cfg80211_resume(ar);
}

#else

void ath6kl_check_wow_status(struct ath6kl *ar)
{
}
#endif

#ifndef CFG80211_NO_SET_CHAN_OPERATION
static int ath6kl_set_channel(struct wiphy *wiphy, struct net_device *dev,
			      struct ieee80211_channel *chan,
			      enum nl80211_channel_type channel_type)
{
	struct ath6kl_vif *vif;

	if (dev == NULL)
		return -EBUSY;

	vif = netdev_priv(dev);

	if (!ath6kl_cfg80211_ready(vif))
		return -EIO;

	if (down_interruptible(&vif->ar->sem)) {
		ath6kl_err("busy, couldn't get access\n");
		return -ERESTARTSYS;
	}

	ath6kl_dbg(ATH6KL_DBG_WLAN_CFG, "%s: center_freq=%u hw_value=%u channel_type=%d\n",
		   __func__, chan->center_freq, chan->hw_value, channel_type);
	vif->next_chan = chan->center_freq;

	if (ath6kl_mod_debug_quirks(vif->ar,
		ATH6KL_MODULE_ENABLE_P2P_CHANMODE)) {
		if (channel_type == NL80211_CHAN_HT40PLUS)
			vif->next_chan_type = ATH6KL_CHAN_TYPE_HT40PLUS;
		else if (channel_type == NL80211_CHAN_HT40MINUS)
			vif->next_chan_type = ATH6KL_CHAN_TYPE_HT40MINUS;
		else if (channel_type == NL80211_CHAN_HT20)
			vif->next_chan_type = ATH6KL_CHAN_TYPE_HT20;
		else
			vif->next_chan_type = ATH6KL_CHAN_TYPE_NONE;
	} else
		vif->next_chan_type = ATH6KL_CHAN_TYPE_NONE;

	up(&vif->ar->sem);

	return 0;
}
#endif

static int ath6kl_set_ap_probe_resp_ies(struct ath6kl_vif *vif,
					const u8 *ies, size_t ies_len)
{
	struct ath6kl *ar = vif->ar;
	const u8 *pos;
	u8 *buf = NULL;
	size_t len = 0;
	int ret;

	/*
	 * Filter out P2P/WFD IE(s) since they will be included depending on
	 * the Probe Request frame in ath6kl_wmi_send_go_probe_response_cmd().
	 */

	if (ies && ies_len) {
		buf = kmalloc(ies_len, GFP_KERNEL);
		if (buf == NULL)
			return -ENOMEM;
		pos = ies;
		while (pos + 1 < ies + ies_len) {
			if (pos + 2 + pos[1] > ies + ies_len)
				break;
			if ((!ath6kl_is_p2p_ie(pos)) &&
			    (!ath6kl_is_wfd_ie(pos))) {
				memcpy(buf + len, pos, 2 + pos[1]);
				len += 2 + pos[1];
			}
			pos += 2 + pos[1];
		}
	}

	ret = ath6kl_wmi_set_appie_cmd(ar->wmi, vif->fw_vif_idx,
				       WMI_FRAME_PROBE_RESP, buf, len);
	kfree(buf);
	return ret;
}

static int ath6kl_set_uapsd(struct wiphy *wiphy, struct net_device *dev,
				const u8 *ies, int ies_len)
{
	struct ath6kl *ar = ath6kl_priv(dev);
	struct ath6kl_vif *vif = netdev_priv(dev);
	const u8 *pos;
	int uapsd = 0; /* default is OFF */
	bool found = false;

	if (ies && ies_len) {
		pos = ies;

		while (pos + 1 < ies + ies_len) {
			if (pos + 2 + pos[1] > ies + ies_len)
				break;

			if (ath6kl_is_wmm_ie(pos)) {
				found = true;
				if (pos[8] & 0x80) /* QOS-INFO, BIT(7) */
					uapsd = 1;
				break;
			}

			pos += 2 + pos[1];
		}
	}

	if (!found)
		clear_bit(WMM_ENABLED, &vif->flags);

	return ath6kl_wmi_ap_set_apsd(ar->wmi, vif->fw_vif_idx, uapsd);
}

#ifdef PMF_SUPPORT
static int ath6kl_set_akm_suites(struct wiphy *wiphy, struct net_device *dev,
				struct cfg80211_connect_params *sme)
{
	const u8 *pos, *ies = sme->ie;
	int ies_len = sme->ie_len;
	int ret = 0;

	if (ies && ies_len) {
		pos = ies;

		while (pos + 1 < ies + ies_len) {
			if (pos + 2 + pos[1] > ies + ies_len)
				break;

			if (ath6kl_is_rsn_ie(pos)) {
				int offset, i;

				/*
				 * VER                  [2] +
				 * 1 GROUP Cipher       [4] +
				 * n Pairwise Ciphers   [2 + 4*n] +
				 * n AKM suites         [2 + 4*n] +
				 * RSN-Capabilities     [2] +
				 * n PMKID Count/List   [2 + 16*n] +
				 * 1 Group Mgmt Cipher  [4]
				 */
				if (pos[1] <= 18)
					break;

				/* Get/Set AKM value */
				offset = 1 + 1 + 2 + 4 + 2 + (pos[8] * 4);
				if (pos[offset]) {
					sme->crypto.n_akm_suites = pos[offset];
					sme->crypto.akm_suites[0]
						 = pos[offset + 2];
					for (i = 1; i < 4; i++) {
						sme->crypto.akm_suites[0]
						 = sme->crypto.akm_suites[0]
						   << 8;
						sme->crypto.akm_suites[0]
						 += pos[offset + 2 + i];
					}
					break;
				}
			}

			pos += 2 + pos[1];
		}
	}

	return ret;
}
#endif

static int ath6kl_set_rsn_cap(struct wiphy *wiphy, struct net_device *dev,
				const u8 *ies, int ies_len)
{
	struct ath6kl *ar = ath6kl_priv(dev);
	struct ath6kl_vif *vif = netdev_priv(dev);
	const u8 *pos;
	u16 rsn_cap = ATH6KL_RSN_CAP_NULLCONF;
	int ret = 0;

	if (ies && ies_len) {
		pos = ies;

		while (pos + 1 < ies + ies_len) {
			if (pos + 2 + pos[1] > ies + ies_len)
				break;

			if (ath6kl_is_rsn_ie(pos)) {
				int offset;

				/*
				 *  VER[2] +
				 *  1 GROUP Cipher[4] +
				 *  1 Pairwise Cipher[2+4] +
				 *  1 Auth suit[2+4] = 18
				 */
				if (pos[1] <= 18)
					break;

				/* Get RSN-CAP offset */
				offset = 1 + 1 + 2 + 4 + 2 + (pos[8] * 4);
				offset += 2 + (pos[offset] * 4);
				if (offset > (pos[1] + 2))
					break;

				rsn_cap = (pos[offset] |
					   (pos[offset + 1] << 8));
				break;
			}

			pos += 2 + pos[1];
		}
	}

	vif->last_rsn_cap = ATH6KL_RSN_CAP_NULLCONF;
	if (rsn_cap != ATH6KL_RSN_CAP_NULLCONF) {
		ret = ath6kl_wmi_get_rsn_cap(ar->wmi, vif->fw_vif_idx);
		if (!ret)
			ret = ath6kl_wmi_set_rsn_cap(ar->wmi,
				vif->fw_vif_idx, rsn_cap);
	}

	return ret;
}

static u16 _ath6kl_ap_get_channel(struct ath6kl_vif *vif,
				struct ath6kl_beacon_parameters *info)
{
	u16 chan = 0;

	/* The wpa_supplicant will set-channel first then start AP. */

#ifdef CFG80211_NO_SET_CHAN_OPERATION
	/*
	 * New cfg80211 implementation will cache channel information and
	 * setup it when start AP operation.
	 * Here get the channel information from the parameter of start AP
	 * operation then set to driver.
	 */
	if (ath6kl_mod_debug_quirks(vif->ar,
		ATH6KL_MODULE_ENABLE_P2P_CHANMODE)) {
		if (info->channel_type == NL80211_CHAN_HT40PLUS)
			vif->next_chan_type = ATH6KL_CHAN_TYPE_HT40PLUS;
		else if (info->channel_type == NL80211_CHAN_HT40MINUS)
			vif->next_chan_type = ATH6KL_CHAN_TYPE_HT40MINUS;
		else if (info->channel_type == NL80211_CHAN_HT20)
			vif->next_chan_type = ATH6KL_CHAN_TYPE_HT20;
		else
			vif->next_chan_type = ATH6KL_CHAN_TYPE_NONE;
	} else
		vif->next_chan_type = ATH6KL_CHAN_TYPE_NONE;

	WARN_ON(!info->channel);
	if (info->channel)
		vif->next_chan = info->channel->center_freq;
	else {
		vif->next_chan = 0;
		vif->next_chan_type = ATH6KL_CHAN_TYPE_NONE;
	}
#endif

	/*
	 * The driver need to cache channel information in the set-channel
	 * operation for old cfg80211 implemenation.
	 * Here just report the cached channel information.
	 */
	chan = vif->next_chan;
	if (vif->next_chan_type != ATH6KL_CHAN_TYPE_NONE) {
		enum wmi_connect_ap_channel_type chan_type =
			AP_CHANNEL_TYPE_NONE;

		if (vif->next_chan_type == ATH6KL_CHAN_TYPE_HT40PLUS)
			chan_type = AP_CHANNEL_TYPE_HT40PLUS;
		else if (vif->next_chan_type == ATH6KL_CHAN_TYPE_HT40MINUS)
			chan_type = AP_CHANNEL_TYPE_HT40MINUS;
		else if (vif->next_chan_type == ATH6KL_CHAN_TYPE_HT20)
			chan_type = AP_CHANNEL_TYPE_HT20;

		chan |= (chan_type << WMI_CONNECT_AP_CHAN_SELECT_OFFSET);
	}

	/* Overwrite the channel if AP recommand channel turns on */
	chan = ath6kl_ap_rc_get(vif, chan);

	return chan;
}

#ifdef NL80211_CMD_SET_AP_MAC_ACL
static void _ath6kl_acl_config(struct ath6kl_vif *vif,
				const struct cfg80211_acl_data *acl)
{
	enum ap_acl_mode mode;
	u8 mac_addr[ETH_ALEN];
	int i;

	/* Remove all ACL address first. */
	ath6kl_ap_acl_config_mac_list_reset(vif);

	/* Disable ACL. */
	ath6kl_ap_acl_config_policy(vif, AP_ACL_MODE_DISABLE);

	/* Turn-on if need. */
	if (acl->n_acl_entries == 0)
		mode = AP_ACL_MODE_DISABLE;
	else if (acl->acl_policy == NL80211_ACL_POLICY_ACCEPT_UNLESS_LISTED)
		mode = AP_ACL_MODE_ALLOW;
	else if (acl->acl_policy == NL80211_ACL_POLICY_DENY_UNLESS_LISTED)
		mode = AP_ACL_MODE_DENY;
	else {
		WARN_ON(1);
		mode = AP_ACL_MODE_DISABLE;
	}
	ath6kl_ap_acl_config_policy(vif, mode);

	/* Add new ACL address. */
	for (i = 0; i < acl->n_acl_entries; i++) {
		if (i < AP_ACL_SIZE) {
			memcpy(mac_addr, acl->mac_addrs[i].addr, ETH_ALEN);
			ath6kl_ap_acl_config_mac_list(vif, mac_addr, false);
		} else {
			ath6kl_err("Wrong ACL number %d\n", acl->n_acl_entries);
			break;
		}
	}

	return;
}
#endif

static int ath6kl_ap_beacon(struct wiphy *wiphy, struct net_device *dev,
			    struct ath6kl_beacon_parameters *info, bool add)
{
	struct ath6kl *ar = ath6kl_priv(dev);
	struct ath6kl_vif *vif = netdev_priv(dev);
	struct ieee80211_mgmt *mgmt;
	u8 *ies;
	int ies_len;
	struct wmi_connect_cmd p;
	int res;
	int i;
	u16 chan;

	ath6kl_dbg(ATH6KL_DBG_WLAN_CFG, "%s: add=%d\n", __func__, add);

	if (!ath6kl_cfg80211_ready(vif))
		return -EIO;

	if (down_interruptible(&ar->sem)) {
		ath6kl_err("busy, couldn't get access\n");
		return -ERESTARTSYS;
	}

	if (vif->next_mode != AP_NETWORK) {
		up(&ar->sem);
		return -EOPNOTSUPP;
	}

	if (info->beacon_ies) {
		u8 *beacon_ies = (u8 *)info->beacon_ies;
		size_t beacon_ies_len = info->beacon_ies_len;

		ath6kl_p2p_ps_user_app_ie(vif->p2p_ps_info_ctx,
					  WMI_FRAME_BEACON,
					  &beacon_ies,
					  &beacon_ies_len);

		res = ath6kl_wmi_set_appie_cmd(ar->wmi, vif->fw_vif_idx,
					       WMI_FRAME_BEACON,
					       beacon_ies,
					       beacon_ies_len);
		if (res) {
			up(&ar->sem);
			return res;
		}
	}

	/*
	 * IOT : Set default DTIM period to 1 .
	 * wpa_supplicant default is 2 and target default is 5.
	 */
	ath6kl_dbg(ATH6KL_DBG_WLAN_CFG, "%s: set DTIM from %d to 1.\n",
		__func__, info->dtim_period);
	ath6kl_wmi_set_dtim_cmd(ar->wmi, vif->fw_vif_idx, 1);

	if (info->proberesp_ies) {
		res = ath6kl_set_ap_probe_resp_ies(vif, info->proberesp_ies,
						   info->proberesp_ies_len);
		if (res) {
			up(&ar->sem);
			return res;
		}
	}
	if (info->assocresp_ies) {
		res = ath6kl_wmi_set_appie_cmd(ar->wmi, vif->fw_vif_idx,
					       WMI_FRAME_ASSOC_RESP,
					       info->assocresp_ies,
					       info->assocresp_ies_len);
		if (res) {
			up(&ar->sem);
			return res;
		}
	}

	if (!add) {
		up(&ar->sem);
		return 0;
	}

#ifdef NL80211_CMD_SET_AP_MAC_ACL
	/* Config ACL */
	if (info->acl)
		_ath6kl_acl_config(vif, info->acl);
#endif

	/* Config keep-alive */
	if (info->inactivity_timeout)
		ath6kl_ap_keepalive_config_by_supp(vif,
						info->inactivity_timeout);

	/* Turn-on/off uAPS. */
	if (ath6kl_set_uapsd(wiphy, dev, info->tail, info->tail_len)) {
		up(&ar->sem);
		return -EIO;
	}

	/* Update RSN Capabilities. */
	if (ath6kl_set_rsn_cap(wiphy, dev, info->tail, info->tail_len)) {
		up(&ar->sem);
		return -EIO;
	}

	/* Turn off power saving mode, if the first interface is AP mode */
	if (vif == ath6kl_vif_first(ar)) {
		if (ath6kl_wmi_powermode_cmd(ar->wmi, vif->fw_vif_idx,
					MAX_PERF_POWER)) {
			up(&ar->sem);
			return -EIO;
		}
	}

	vif->ap_mode_bkey.valid = false;

	if (info->beacon_interval) {
		res = ath6kl_wmi_set_beacon_interval_cmd(ar->wmi,
			vif->fw_vif_idx, info->beacon_interval);
		if (res) {
			up(&ar->sem);
			return res;
		}
	}

	if (info->head == NULL) {
		up(&ar->sem);
		return -EINVAL;
	}
	mgmt = (struct ieee80211_mgmt *) info->head;
	ies = mgmt->u.beacon.variable;
	if (ies > info->head + info->head_len) {
		up(&ar->sem);
		return -EINVAL;
	}
	ies_len = info->head + info->head_len - ies;

	if (info->ssid == NULL) {
		up(&ar->sem);
		return -EINVAL;
	}
	memcpy(vif->ssid, info->ssid, info->ssid_len);
	vif->ssid_len = info->ssid_len;

	if (info->hidden_ssid == NL80211_HIDDEN_SSID_ZERO_LEN) {
		up(&ar->sem);
		return -ENOTSUPP;
	} else if (info->hidden_ssid == NL80211_HIDDEN_SSID_ZERO_CONTENTS) {
		res = ath6kl_wmi_set_hidden_ssid_cmd(
			ar->wmi, vif->fw_vif_idx, 1);
	} else {
		res = ath6kl_wmi_set_hidden_ssid_cmd(
			ar->wmi, vif->fw_vif_idx, 0);
	}
	if (res) {
		up(&ar->sem);
		return res;
	}

	res = ath6kl_set_auth_type(vif, info->auth_type);
	if (res) {
		up(&ar->sem);
		return res;
	}

	memset(&p, 0, sizeof(p));

	for (i = 0; i < info->crypto.n_akm_suites; i++) {
		switch (info->crypto.akm_suites[i]) {
		case WLAN_AKM_SUITE_8021X:
			if (info->crypto.wpa_versions & NL80211_WPA_VERSION_1)
				p.auth_mode |= WPA_AUTH;
			if (info->crypto.wpa_versions & NL80211_WPA_VERSION_2)
				p.auth_mode |= WPA2_AUTH;
			break;
		case WLAN_AKM_SUITE_PSK:
			if (info->crypto.wpa_versions & NL80211_WPA_VERSION_1)
				p.auth_mode |= WPA_PSK_AUTH;
			if (info->crypto.wpa_versions & NL80211_WPA_VERSION_2)
				p.auth_mode |= WPA2_PSK_AUTH;
			break;
		}
	}
	if (p.auth_mode == 0)
		p.auth_mode = NONE_AUTH;
	vif->auth_mode = p.auth_mode;

	for (i = 0; i < info->crypto.n_ciphers_pairwise; i++) {
		switch (info->crypto.ciphers_pairwise[i]) {
		case WLAN_CIPHER_SUITE_WEP40:
		case WLAN_CIPHER_SUITE_WEP104:
			p.prwise_crypto_type |= WEP_CRYPT;
			break;
		case WLAN_CIPHER_SUITE_TKIP:
			p.prwise_crypto_type |= TKIP_CRYPT;
			break;
		case WLAN_CIPHER_SUITE_CCMP:
			p.prwise_crypto_type |= AES_CRYPT;
			break;
		case WLAN_CIPHER_SUITE_SMS4:
			p.prwise_crypto_type |= WAPI_CRYPT;
			break;
		}
	}
	if (p.prwise_crypto_type == 0) {
		p.prwise_crypto_type = NONE_CRYPT;
		ath6kl_set_cipher(vif, 0, true);
	} else if (info->crypto.n_ciphers_pairwise == 1)
		ath6kl_set_cipher(vif, info->crypto.ciphers_pairwise[0], true);

	switch (info->crypto.cipher_group) {
	case WLAN_CIPHER_SUITE_WEP40:
	case WLAN_CIPHER_SUITE_WEP104:
		p.grp_crypto_type = WEP_CRYPT;
		break;
	case WLAN_CIPHER_SUITE_TKIP:
		p.grp_crypto_type = TKIP_CRYPT;
		break;
	case WLAN_CIPHER_SUITE_CCMP:
		p.grp_crypto_type = AES_CRYPT;
		break;
	case WLAN_CIPHER_SUITE_SMS4:
		p.grp_crypto_type = WAPI_CRYPT;
		break;
	default:
		p.grp_crypto_type = NONE_CRYPT;
		break;
	}
	ath6kl_set_cipher(vif, info->crypto.cipher_group, false);

	p.nw_type = AP_NETWORK;
	vif->nw_type = vif->next_mode;

	p.ssid_len = vif->ssid_len;
	memcpy(p.ssid, vif->ssid, vif->ssid_len);
	p.dot11_auth_mode = vif->dot11_auth_mode;

	chan = _ath6kl_ap_get_channel(vif, info);
	p.ch = cpu_to_le16(chan);

	res = ath6kl_wmi_ap_profile_commit(ar->wmi, vif->fw_vif_idx, &p);
	if (res < 0) {
		up(&ar->sem);
		return res;
	}

	up(&ar->sem);

	return 0;
}

#ifdef NL80211_CMD_START_STOP_AP
static int ath6kl_start_ap(struct wiphy *wiphy, struct net_device *dev,
			     struct cfg80211_ap_settings *settings)
{
	struct ath6kl_beacon_parameters *info;
	int ret = 0;

	info = kzalloc(sizeof(struct ath6kl_beacon_parameters), GFP_ATOMIC);
	if (info == NULL)
		return -ENOMEM;

	/* Fetch to local setting */
	info->beacon_interval    = settings->beacon_interval;
	info->dtim_period        = settings->dtim_period;
	info->ssid               = settings->ssid;
	info->ssid_len           = settings->ssid_len;
	info->hidden_ssid        = settings->hidden_ssid;
	memcpy(&info->crypto,
		&settings->crypto,
		sizeof(struct cfg80211_crypto_settings));
	info->privacy            = settings->privacy;
	info->auth_type          = settings->auth_type;
	info->inactivity_timeout = settings->inactivity_timeout;
#ifdef CFG80211_NO_SET_CHAN_OPERATION
#ifdef CFG80211_NEW_CHAN_DEFINITION
	info->channel		= settings->chandef.chan;
	info->channel_type	= cfg80211_get_chandef_type(&settings->chandef);
#else
	info->channel		= settings->channel;
	info->channel_type	= settings->channel_type;
#endif
#endif
	info->p2p_ctwindow	= 0;
	info->p2p_opp_ps	= false;
#ifdef NL80211_CMD_SET_AP_MAC_ACL
	info->acl		= settings->acl;
#endif

	/*
	 * The target will take care DFS behavior and the host just need to
	 * report events back to the user.
	 */
	info->radar_required	= false;

	info->head              = settings->beacon.head;
	info->tail              = settings->beacon.tail;
	info->head_len          = settings->beacon.head_len;
	info->tail_len          = settings->beacon.tail_len;
	info->beacon_ies        = settings->beacon.beacon_ies;
	info->beacon_ies_len    = settings->beacon.beacon_ies_len;
	info->proberesp_ies     = settings->beacon.proberesp_ies;
	info->proberesp_ies_len = settings->beacon.proberesp_ies_len;
	info->assocresp_ies     = settings->beacon.assocresp_ies;
	info->assocresp_ies_len = settings->beacon.assocresp_ies_len;
	info->probe_resp        = settings->beacon.probe_resp;
	info->probe_resp_len    = settings->beacon.probe_resp_len;

	ret = ath6kl_ap_beacon(wiphy, dev, info, true);
	kfree(info);

	return ret;
}

static int ath6kl_change_beacon(struct wiphy *wiphy, struct net_device *dev,
			     struct cfg80211_beacon_data *settings)
{
	struct ath6kl_beacon_parameters *info;
	int ret = 0;

	info = kzalloc(sizeof(struct ath6kl_beacon_parameters), GFP_ATOMIC);
	if (info == NULL)
		return -ENOMEM;

	/* Fetch to local setting */
	info->head              = settings->head;
	info->tail              = settings->tail;
	info->head_len          = settings->head_len;
	info->tail_len          = settings->tail_len;
	info->beacon_ies        = settings->beacon_ies;
	info->beacon_ies_len    = settings->beacon_ies_len;
	info->proberesp_ies     = settings->proberesp_ies;
	info->proberesp_ies_len = settings->proberesp_ies_len;
	info->assocresp_ies     = settings->assocresp_ies;
	info->assocresp_ies_len = settings->assocresp_ies_len;
	info->probe_resp        = settings->probe_resp;
	info->probe_resp_len    = settings->probe_resp_len;

	ret = ath6kl_ap_beacon(wiphy, dev, info, false);
	kfree(info);

	return ret;
}
#else
static int ath6kl_add_beacon(struct wiphy *wiphy, struct net_device *dev,
			     struct beacon_parameters *settings)
{
	struct ath6kl_beacon_parameters *info;
	int ret = 0;

	info = kzalloc(sizeof(struct ath6kl_beacon_parameters), GFP_ATOMIC);
	if (info == NULL)
		return -ENOMEM;

	/* Fetch to local setting */
	info->beacon_interval    = settings->interval;
	info->dtim_period        = settings->dtim_period;
	info->ssid               = settings->ssid;
	info->ssid_len           = settings->ssid_len;
	info->hidden_ssid        = settings->hidden_ssid;
	memcpy(&info->crypto,
		&settings->crypto,
		sizeof(struct cfg80211_crypto_settings));
	info->privacy            = settings->privacy;
	info->auth_type          = settings->auth_type;

	info->head              = settings->head;
	info->tail              = settings->tail;
	info->head_len          = settings->head_len;
	info->tail_len          = settings->tail_len;
	info->beacon_ies        = settings->beacon_ies;
	info->beacon_ies_len    = settings->beacon_ies_len;
	info->proberesp_ies     = settings->proberesp_ies;
	info->proberesp_ies_len = settings->proberesp_ies_len;
	info->assocresp_ies     = settings->assocresp_ies;
	info->assocresp_ies_len = settings->assocresp_ies_len;
	info->probe_resp        = settings->probe_resp;
	info->probe_resp_len    = settings->probe_resp_len;

	ret = ath6kl_ap_beacon(wiphy, dev, info, true);
	kfree(info);

	return ret;
}

static int ath6kl_set_beacon(struct wiphy *wiphy, struct net_device *dev,
			     struct beacon_parameters *settings)
{
	struct ath6kl_beacon_parameters *info;
	int ret = 0;

	info = kzalloc(sizeof(struct ath6kl_beacon_parameters), GFP_ATOMIC);
	if (info == NULL)
		return -ENOMEM;

	/* Fetch to local setting */
	info->head              = settings->head;
	info->tail              = settings->tail;
	info->head_len          = settings->head_len;
	info->tail_len          = settings->tail_len;
	info->beacon_ies        = settings->beacon_ies;
	info->beacon_ies_len    = settings->beacon_ies_len;
	info->proberesp_ies     = settings->proberesp_ies;
	info->proberesp_ies_len = settings->proberesp_ies_len;
	info->assocresp_ies     = settings->assocresp_ies;
	info->assocresp_ies_len = settings->assocresp_ies_len;
	info->probe_resp        = settings->probe_resp;
	info->probe_resp_len    = settings->probe_resp_len;

	ret = ath6kl_ap_beacon(wiphy, dev, info, false);
	kfree(info);

	return ret;
}
#endif
static int ath6kl_del_beacon(struct wiphy *wiphy, struct net_device *dev)
{
	struct ath6kl *ar = ath6kl_priv(dev);
	struct ath6kl_vif *vif = netdev_priv(dev);

	if (!ath6kl_cfg80211_ready(vif))
		return -EIO;

	if (down_interruptible(&ar->sem)) {
		ath6kl_err("busy, couldn't get access\n");
		return -ERESTARTSYS;
	}

	if (vif->nw_type != AP_NETWORK) {
		up(&ar->sem);
		return -EOPNOTSUPP;
	}
	if (!test_bit(CONNECTED, &vif->flags)) {
		up(&ar->sem);
		return -ENOTCONN;
	}

	/* Back to origional value. */
	if (vif->last_rsn_cap != ATH6KL_RSN_CAP_NULLCONF)
		ath6kl_wmi_set_rsn_cap(ar->wmi, vif->fw_vif_idx,
			vif->last_rsn_cap);

	/* Stop keep-alive. */
	ath6kl_ap_keepalive_stop(vif);

	/* Stop ACL. */
	ath6kl_ap_acl_stop(vif);

	/* Stop Admission-Control */
	ath6kl_ap_admc_stop(vif);

	ath6kl_wmi_disconnect_cmd(ar->wmi, vif->fw_vif_idx);
	clear_bit(CONNECTED, &vif->flags);
	ath6kl_judge_roam_parameter(vif, true);
	ath6kl_switch_parameter_based_on_connection(vif, true);

	up(&ar->sem);

	return 0;
}

static const u8 bcast_addr[ETH_ALEN] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

static int ath6kl_del_station(struct wiphy *wiphy, struct net_device *dev,
			      u8 *mac)
{
	struct ath6kl *ar = ath6kl_priv(dev);
	struct ath6kl_vif *vif = netdev_priv(dev);
	const u8 *addr = mac ? mac : bcast_addr;
	int ret;
	struct ath6kl_sta *sta_conn = NULL;

	if (!ath6kl_cfg80211_ready(vif))
		return -EIO;

	if (down_interruptible(&ar->sem)) {
		ath6kl_err("busy, couldn't get access\n");
		return -ERESTARTSYS;
	}

	if (!is_broadcast_ether_addr(addr)) {
		sta_conn = ath6kl_find_sta(vif, (u8 *)addr);
		if (sta_conn == NULL) {
			up(&ar->sem);
			ret = 0;
			return ret;
		}
	}

	ret = ath6kl_wmi_ap_set_mlme(ar->wmi, vif->fw_vif_idx, WMI_AP_DEAUTH,
				      addr, WLAN_REASON_PREV_AUTH_NOT_VALID);

	up(&ar->sem);

	return ret;
}

static int ath6kl_change_station(struct wiphy *wiphy, struct net_device *dev,
				 u8 *mac, struct station_parameters *params)
{
	struct ath6kl *ar = ath6kl_priv(dev);
	struct ath6kl_vif *vif = netdev_priv(dev);
	int ret = 0;
	struct ath6kl_sta *sta_conn = NULL;

	if (!ath6kl_cfg80211_ready(vif))
		return -EIO;

	if (down_interruptible(&ar->sem)) {
		ath6kl_err("busy, couldn't get access\n");
		return -ERESTARTSYS;
	}

	if (vif->nw_type != AP_NETWORK) {
		up(&ar->sem);
		return -EOPNOTSUPP;
	}

	/* Use this only for authorizing/unauthorizing a station */
	if (!(params->sta_flags_mask & BIT(NL80211_STA_FLAG_AUTHORIZED))) {
		up(&ar->sem);
		return -EOPNOTSUPP;
	}

	if (params->sta_flags_set & BIT(NL80211_STA_FLAG_AUTHORIZED)) {
		ret = ath6kl_wmi_ap_set_mlme(ar->wmi, vif->fw_vif_idx,
					      WMI_AP_MLME_AUTHORIZE, mac, 0);

		up(&ar->sem);

		return ret;
	}

	if (!is_broadcast_ether_addr(mac)) {
		sta_conn = ath6kl_find_sta(vif, mac);
		if (sta_conn == NULL) {
			up(&ar->sem);
			return ret;
		}
	}

	ret = ath6kl_wmi_ap_set_mlme(ar->wmi, vif->fw_vif_idx,
				      WMI_AP_MLME_UNAUTHORIZE, mac, 0);

	up(&ar->sem);

	return ret;
}

static int _ath6kl_remain_on_channel(struct wiphy *wiphy,
				    struct net_device *dev,
				    struct ieee80211_channel *chan,
				    enum nl80211_channel_type channel_type,
				    unsigned int duration,
				    u64 *cookie)
{
#define MAX_ROC_PERIOD \
	(ATH6KL_ROC_MAX_PERIOD * HZ + HZ / ATH6KL_ROC_MAX_PERIOD)
#define MAX_SCAN_PERIOD (ATH6KL_SCAN_FG_MAX_PERIOD * HZ)
	struct ath6kl *ar = ath6kl_priv(dev);
	struct ath6kl_vif *vif = netdev_priv(dev);
	u32 id;
	int ret;
	long left;

	if (!ath6kl_cfg80211_ready(vif))
		return -EIO;

	if (down_interruptible(&ar->sem)) {
		ath6kl_err("busy, couldn't get access\n");
		return -ERESTARTSYS;
	}

	/* If already ongoing scan then wait it finish. */
	if (vif->scan_req) {
		ath6kl_dbg(ATH6KL_DBG_EXT_ROC,
			"RoC : Schedule a RoC but on-going Scan %x\n",
				vif->last_roc_id);

		wait_event_interruptible_timeout(ar->event_wq,
						 !vif->scan_req,
						 MAX_SCAN_PERIOD);

		if (signal_pending(current)) {
			ath6kl_dbg(ATH6KL_DBG_EXT_ROC,
				"RoC : last scan not yet finish?\n");
			up(&ar->sem);
			return -EBUSY;
		}
	}

	/* If already pending remain-on-channel then reject request. */
	if (test_bit(ROC_PEND, &vif->flags)) {
		ath6kl_dbg(ATH6KL_DBG_EXT_ROC,
			"RoC : Receive duplicate ROC.\n");
		up(&ar->sem);
		return -EBUSY;
	}

	/* If ongoing remain-on-channel and wait it finish. */
	if (test_bit(ROC_ONGOING, &vif->flags)) {
		ath6kl_dbg(ATH6KL_DBG_EXT_ROC,
			"RoC : Last RoC not yet finish, %x %d %d",
				vif->last_roc_id,
				((test_bit(ROC_PEND, &vif->flags)) ? 1 : 0),
				((test_bit(ROC_ONGOING, &vif->flags)) ? 1 : 0));

		set_bit(ROC_WAIT_EVENT, &vif->flags);
		left = wait_event_interruptible_timeout(ar->event_wq,
			!test_bit(ROC_ONGOING, &vif->flags),
			 MAX_ROC_PERIOD);

		if (left == 0) {
			ath6kl_dbg(ATH6KL_DBG_EXT_ROC,
				"RoC : wait ROC_WAIT_EVENT timeout\n");
			clear_bit(ROC_WAIT_EVENT, &vif->flags);
			clear_bit(ROC_ONGOING, &vif->flags);
		}

		if (signal_pending(current)) {
			ath6kl_dbg(ATH6KL_DBG_EXT_ROC,
				"RoC : last RoC not yet finish?\n");
			up(&ar->sem);
			return -EBUSY;
		}
	}

	spin_lock_bh(&vif->if_lock);
	id = ++vif->last_roc_id;
	if (id == 0) {
		/* Do not use 0 as the cookie value */
		id = ++vif->last_roc_id;
	}
	*cookie = id;
	spin_unlock_bh(&vif->if_lock);

	/* Cache request channel and report to cfg80211 when target reject. */
	vif->last_roc_channel = chan;
	vif->last_roc_duration = duration;

	set_bit(ROC_PEND, &vif->flags);
	ret = ath6kl_wmi_remain_on_chnl_cmd(ar->wmi, vif->fw_vif_idx,
					     chan->center_freq, duration);
	up(&ar->sem);

	return ret;
#undef MAX_ROC_PERIOD
#undef MAX_SCAN_PERIOD
}

static int _ath6kl_cancel_remain_on_channel(struct wiphy *wiphy,
					   struct net_device *dev,
					   u64 cookie)
{
	struct ath6kl *ar = ath6kl_priv(dev);
	struct ath6kl_vif *vif = netdev_priv(dev);
	int ret;
	long left;

	if (!ath6kl_cfg80211_ready(vif))
		return -EIO;

	if (down_interruptible(&ar->sem)) {
		ath6kl_err("busy, couldn't get access\n");
		return -ERESTARTSYS;
	}

	/*
	 * RoC not yet start but be cancelled. Wait it started then cancel
	 * it by order to avoid wrong cookie report to supplicant.
	 */
	if (test_bit(ROC_PEND, &vif->flags)) {
		ath6kl_dbg(ATH6KL_DBG_EXT_ROC,
			"RoC : RoC not start but canceled %x\n",
				vif->last_roc_id);

		set_bit(ROC_WAIT_EVENT, &vif->flags);
		left = wait_event_interruptible_timeout(ar->event_wq,
			test_bit(ROC_ONGOING, &vif->flags),
			WMI_TIMEOUT);

		/*
		 * Another corner case is that last RoC not yet start & also
		 * be rejected by target, but be canceled. In this case,
		 * treat WMI_TIMEOUT as target reject then return.
		 */
		if (left == 0) {
			ath6kl_dbg(ATH6KL_DBG_EXT_ROC,
				"RoC : wait ROC_WAIT_EVENT timeout, %lld %d\n",
					cookie,
					vif->last_roc_id);
			clear_bit(ROC_WAIT_EVENT, &vif->flags);
			clear_bit(ROC_PEND, &vif->flags);
			up(&ar->sem);
			return 0;
		}

		if (signal_pending(current)) {
			ath6kl_dbg(ATH6KL_DBG_EXT_ROC,
				"RoC : target did not respond\n");
			up(&ar->sem);
			return -EINTR;
		}
	}

	spin_lock_bh(&vif->if_lock);
	if (cookie != vif->last_roc_id) {
		ath6kl_dbg(ATH6KL_DBG_EXT_ROC,
			"RoC : Cancel-RoC %llx but current is %x\n", cookie,
			vif->last_roc_id);

		spin_unlock_bh(&vif->if_lock);
		up(&ar->sem);
		return -ENOENT;
	}

	vif->last_cancel_roc_id = cookie;
	set_bit(ROC_CANCEL_PEND, &vif->flags);
	spin_unlock_bh(&vif->if_lock);

	ret = ath6kl_wmi_cancel_remain_on_chnl_cmd(ar->wmi, vif->fw_vif_idx);

	up(&ar->sem);

	return ret;
}

static int _ath6kl_mgmt_tx(struct wiphy *wiphy, struct net_device *dev,
			  struct ieee80211_channel *chan, bool offchan,
			  enum nl80211_channel_type channel_type,
			  bool channel_type_valid, unsigned int wait,
			  const u8 *buf, size_t len, bool no_cck,
			  bool dont_wait_for_ack, u64 *cookie)
{
	struct ath6kl *ar = ath6kl_priv(dev);
	struct ath6kl_vif *vif = netdev_priv(dev);
	u32 id;
	u32 wmi_data_flags = 0;
	const struct ieee80211_mgmt *mgmt;
	int ret = 0;

	if (!ath6kl_cfg80211_ready(vif))
		return -EIO;

	if (down_interruptible(&ar->sem)) {
		ath6kl_err("busy, couldn't get access\n");
		return -ERESTARTSYS;
	}

	if ((ar->p2p) && !ar->p2p_compat &&
		(ar->p2p_concurrent) &&
		(vif->fw_vif_idx == (ar->vif_max - 1)) &&
		!test_bit(ROC_ONGOING, &vif->flags)) {

		if (ath6kl_p2p_is_p2p_frame(ar, buf, len)) {
			ath6kl_dbg(ATH6KL_DBG_EXT_ROC,
				"RoC : Channel closed, ignore action frame\n");
			up(&ar->sem);
			return -EINVAL;
		}

	}

	id = vif->send_action_id++;
	if (id == 0) {
		/*
		 * 0 is a reserved value in the WMI command and shall not be
		 * used for the command.
		 */
		id = vif->send_action_id++;
	}

	*cookie = id;

	/* AP mode Power saving processing */
	if (vif->nw_type == AP_NETWORK) {
		if (ath6kl_mgmt_powersave_ap(vif,
						id,
						chan->center_freq,
						wait,
						buf,
						len,
						no_cck,
						dont_wait_for_ack,
						&wmi_data_flags)) {
			up(&ar->sem);
			return ret;
		}
	}

	mgmt = (const struct ieee80211_mgmt *) buf;
	if (buf + len >= mgmt->u.probe_resp.variable &&
	    vif->nw_type == AP_NETWORK && test_bit(CONNECTED, &vif->flags) &&
	    ieee80211_is_probe_resp(mgmt->frame_control)) {
		/*
		 * Send Probe Response frame in AP mode using a separate WMI
		 * command to allow the target to fill in the generic IEs.
		 */
		ret = ath6kl_wmi_send_go_probe_response_cmd(ar->wmi, vif,
							buf, len,
							chan->center_freq);
		up(&ar->sem);
		return ret;
	}

	ret = ath6kl_wmi_send_action_cmd(ar->wmi, vif->fw_vif_idx, id,
					  chan->center_freq, wait,
					  buf, len);
	up(&ar->sem);
	return ret;
}

static void _ath6kl_mgmt_frame_register(struct wiphy *wiphy,
				       struct net_device *dev,
				       u16 frame_type, bool reg)
{
	struct ath6kl_vif *vif = netdev_priv(dev);

	if (!ath6kl_cfg80211_ready(vif))
		return;

	ath6kl_dbg(ATH6KL_DBG_WLAN_CFG, "%s: frame_type=0x%x reg=%d\n",
		   __func__, frame_type, reg);
	if (frame_type == IEEE80211_STYPE_PROBE_REQ) {
		/*
		 * Note: This notification callback is not allowed to sleep, so
		 * we cannot send WMI_PROBE_REQ_REPORT_CMD here. Instead, we
		 * hardcode target to report Probe Request frames all the time.
		 */
		vif->probe_req_report = reg;
	}
#ifdef CE_SUPPORT
	if (frame_type == IEEE80211_STYPE_PROBE_RESP)
		vif->probe_resp_report = reg;
#endif
}

#ifdef CFG80211_NETDEV_REPLACED_BY_WDEV
#ifdef CFG80211_REMOVE_ROC_CHAN_TYPE
static int ath6kl_remain_on_channel(struct wiphy *wiphy,
				    struct wireless_dev *wdev,
				    struct ieee80211_channel *chan,
				    unsigned int duration,
				    u64 *cookie)
{
	BUG_ON(!wdev->netdev);

	return _ath6kl_remain_on_channel(wiphy,
					wdev->netdev,
					chan,
					NL80211_CHAN_NO_HT,
					duration,
					cookie);
}
#else
static int ath6kl_remain_on_channel(struct wiphy *wiphy,
				    struct wireless_dev *wdev,
				    struct ieee80211_channel *chan,
				    enum nl80211_channel_type channel_type,
				    unsigned int duration,
				    u64 *cookie)
{
	BUG_ON(!wdev->netdev);

	return _ath6kl_remain_on_channel(wiphy,
					wdev->netdev,
					chan,
					channel_type,
					duration,
					cookie);
}
#endif

static int ath6kl_cancel_remain_on_channel(struct wiphy *wiphy,
					   struct wireless_dev *wdev,
					   u64 cookie)
{
	BUG_ON(!wdev->netdev);

	return _ath6kl_cancel_remain_on_channel(wiphy,
						wdev->netdev,
						cookie);
}

#ifdef CFG80211_REMOVE_ROC_CHAN_TYPE
static int ath6kl_mgmt_tx(struct wiphy *wiphy, struct wireless_dev *wdev,
			  struct ieee80211_channel *chan, bool offchan,
			  unsigned int wait,
			  const u8 *buf, size_t len, bool no_cck,
			  bool dont_wait_for_ack, u64 *cookie)
{
	BUG_ON(!wdev->netdev);

	return _ath6kl_mgmt_tx(wiphy,
				wdev->netdev,
				chan,
				offchan,
				NL80211_CHAN_NO_HT,
				true,
				wait,
				buf,
				len,
				no_cck,
				dont_wait_for_ack,
				cookie);
}
#else
static int ath6kl_mgmt_tx(struct wiphy *wiphy, struct wireless_dev *wdev,
			  struct ieee80211_channel *chan, bool offchan,
			  enum nl80211_channel_type channel_type,
			  bool channel_type_valid, unsigned int wait,
			  const u8 *buf, size_t len, bool no_cck,
			  bool dont_wait_for_ack, u64 *cookie)
{
	BUG_ON(!wdev->netdev);

	return _ath6kl_mgmt_tx(wiphy,
				wdev->netdev,
				chan,
				offchan,
				channel_type,
				channel_type_valid,
				wait,
				buf,
				len,
				no_cck,
				dont_wait_for_ack,
				cookie);
}
#endif

static void ath6kl_mgmt_frame_register(struct wiphy *wiphy,
				       struct wireless_dev *wdev,
				       u16 frame_type, bool reg)
{
	BUG_ON(!wdev->netdev);

	return _ath6kl_mgmt_frame_register(wiphy,
					wdev->netdev,
					frame_type,
					reg);
}
#else
static int ath6kl_remain_on_channel(struct wiphy *wiphy,
				    struct net_device *dev,
				    struct ieee80211_channel *chan,
				    enum nl80211_channel_type channel_type,
				    unsigned int duration,
				    u64 *cookie)
{
	return _ath6kl_remain_on_channel(wiphy,
					dev,
					chan,
					channel_type,
					duration,
					cookie);
}

static int ath6kl_cancel_remain_on_channel(struct wiphy *wiphy,
					   struct net_device *dev,
					   u64 cookie)
{
	return _ath6kl_cancel_remain_on_channel(wiphy,
						dev,
						cookie);
}

static int ath6kl_mgmt_tx(struct wiphy *wiphy, struct net_device *dev,
			  struct ieee80211_channel *chan, bool offchan,
			  enum nl80211_channel_type channel_type,
			  bool channel_type_valid, unsigned int wait,
			  const u8 *buf, size_t len, bool no_cck,
			  bool dont_wait_for_ack, u64 *cookie)
{
	return _ath6kl_mgmt_tx(wiphy,
				dev,
				chan,
				offchan,
				channel_type,
				channel_type_valid,
				wait,
				buf,
				len,
				no_cck,
				dont_wait_for_ack,
				cookie);
}

static void ath6kl_mgmt_frame_register(struct wiphy *wiphy,
				       struct net_device *dev,
				       u16 frame_type, bool reg)
{
	return _ath6kl_mgmt_frame_register(wiphy,
					dev,
					frame_type,
					reg);
}
#endif

int	ath6kl_set_gtk_rekey_offload(struct wiphy *wiphy,
		struct net_device *dev, struct cfg80211_gtk_rekey_data *data)
{
	int ret = 0;
	struct ath6kl *ar = (struct ath6kl *)wiphy_priv(wiphy);
	struct wmi_gtk_offload_op cmd;
	struct ath6kl_vif *vif = ath6kl_vif_first(ar);

	ath6kl_dbg(ATH6KL_DBG_TRC, "+++\n");
	if (!vif)
		return -EIO;

	if (!ath6kl_cfg80211_ready(vif))
		return -EIO;

	if (!data)
		return ret;


	memset(&cmd, 0, sizeof(struct wmi_gtk_offload_op));

	memcpy(cmd.kek, data->kek, GTK_OFFLOAD_KEK_BYTES);
	memcpy(cmd.kck, data->kck, GTK_OFFLOAD_KCK_BYTES);
	memcpy(cmd.replay_counter, data->replay_ctr, GTK_REPLAY_COUNTER_BYTES);

	ret = ath6kl_wm_set_gtk_offload(ar->wmi, vif->fw_vif_idx, cmd.kek,
		cmd.kck, cmd.replay_counter);

	return ret;
}

static int ath6kl_ap_probe_client(struct wiphy *wiphy, struct net_device *dev,
				const u8 *peer, u64 *cookie)
{
	struct ath6kl *ar = (struct ath6kl *)wiphy_priv(wiphy);
	struct ath6kl_vif *vif = netdev_priv(dev);
	struct ath6kl_sta *conn;
	int ret;

	BUG_ON(vif->nw_type != AP_NETWORK);

	conn = ath6kl_find_sta(vif, (u8 *)peer);
	if (conn)
		ret = ath6kl_wmi_ap_poll_sta(ar->wmi,
				       vif->fw_vif_idx,
				       conn->aid);
	else {
		ret = -EINTR;

		ath6kl_dbg(ATH6KL_DBG_INFO,
			"can't find sta %02x:%02x:%02x:%02x:%02x:%02x, vif-idx %d\n",
			peer[0], peer[1], peer[2], peer[3], peer[4], peer[5],
			vif->fw_vif_idx);
	}

	return ret;
}

bool ath6kl_sched_scan_trigger(struct ath6kl_vif *vif)
{
	struct ath6kl *ar = vif->ar;
	int i;

	/* NOT YET */

	if (!ar->sche_scan)
		return false;

	for (i = 0; i < ar->vif_max; i++) {
		struct ath6kl_vif *vif_trig;

		vif_trig = ath6kl_get_vif_by_index(ar, i);
		if ((vif_trig) &&
		    (vif_trig != vif) &&
		    (!test_bit(CONNECTED, &vif_trig->flags)) &&
		    (vif_trig->sche_scan_interval)) {
			ath6kl_dbg(ATH6KL_DBG_INFO, "sche scan triggered\n");

			return true;
		}
	}

	return false;
}

static void ath6kl_sched_scan_timer(unsigned long ptr)
{
	struct ath6kl_vif *vif = (struct ath6kl_vif *)ptr;

	/* NOT YET */

	ath6kl_dbg(ATH6KL_DBG_INFO, "report sche scan\n");

	cfg80211_sched_scan_results(vif->ar->wiphy);

	mod_timer(&vif->sche_scan_timer,
			jiffies + msecs_to_jiffies(vif->sche_scan_interval));

	return;
}

static int ath6kl_sched_scan_start(struct wiphy *wiphy,
				struct net_device *dev,
				struct cfg80211_sched_scan_request *request)
{
	struct ath6kl_vif *vif = netdev_priv(dev);
	int ret = 0;

	/* NOT YET */

	ath6kl_dbg(ATH6KL_DBG_INFO,
		"start sche scan, interval %d, n_ssids %d, n_channels %d\n",
		request->interval,
		request->n_ssids,
		request->n_channels);

	if ((vif->ar->sche_scan) &&
	    (vif->nw_type == INFRA_NETWORK)) {
		vif->sche_scan_interval = request->interval;

		mod_timer(&vif->sche_scan_timer,
				jiffies +
				msecs_to_jiffies(vif->sche_scan_interval));
	} else
		ret = -EOPNOTSUPP;

	return ret;
}

static int ath6kl_sched_scan_stop(struct wiphy *wiphy,
				struct net_device *dev)
{
	struct ath6kl_vif *vif = netdev_priv(dev);

	/* NOT YET */

	ath6kl_dbg(ATH6KL_DBG_INFO, "stop sche scan\n");

	if (vif->sche_scan_interval) {
		del_timer(&vif->sche_scan_timer);
		vif->sche_scan_interval = 0;
	}

	return 0;
}

static int ath6kl_sched_scan_init(struct ath6kl_vif *vif)
{
	struct ath6kl *ar = vif->ar;

	/* NOT YET */

	if (ar->sche_scan) {
		vif->sche_scan_interval = 0;

		init_timer(&vif->sche_scan_timer);
		vif->sche_scan_timer.function = ath6kl_sched_scan_timer;
		vif->sche_scan_timer.data = (unsigned long)(vif);
	}

	return 0;
}

static int ath6kl_sched_scan_deinit(struct ath6kl_vif *vif)
{
	struct ath6kl *ar = vif->ar;

	ath6kl_sched_scan_stop(ar->wiphy, vif->ndev);

	return 0;
}

#if defined(CONFIG_ANDROID) || defined(USB_AUTO_SUSPEND)
int ath6kl_set_wow_mode(struct wiphy *wiphy, struct cfg80211_wowlan *wow)
{
	struct ath6kl_vif *vif;
	int ret = 0, pos, i;
	struct ath6kl *ar = (struct ath6kl *)wiphy_priv(wiphy);
	u32 filter = 0;
	u8 mask[WOW_MASK_SIZE];
	u32 host_req_delay = WOW_HOST_REQ_DELAY;

	ath6kl_dbg(ATH6KL_DBG_WOWLAN, "%s: +++\n", __func__);

	vif = ath6kl_vif_first(ar);
	if (!vif) {
		ret = -EIO;
		goto FAIL;
	}

	if (!ath6kl_cfg80211_ready(vif)) {
		ret = -EIO;
		goto FAIL;
	}

	if (!ar->get_wow_pattern && WARN_ON(!wow))
		goto FAIL;

	/* Reset wakeup delay time to 2 secs for sdio, keep 5 secs for
	 * usb now
	 */
	if (ar->hif_type == ATH6KL_HIF_TYPE_SDIO)
		host_req_delay = 2000;

	/* for hsic mode, reduce the wakeup time to 2 secs */
	if (BOOTSTRAP_IS_HSIC(ar->bootstrap_mode))
		host_req_delay = 2000;

	/* Clear existing WOW patterns */
	if (!ar->get_wow_pattern) {
		for (i = 0; i < WOW_MAX_FILTERS_PER_LIST; i++)
			ath6kl_wmi_del_wow_pattern_cmd(ar->wmi, vif->fw_vif_idx,
							   WOW_LIST_ID, i);

		/* Configure new WOW patterns */
		for (i = 0; i < wow->n_patterns; i++) {

			if (ath6kl_wow_ext) {
				ret = ath6kl_wmi_add_wow_ext_pattern_cmd(
						ar->wmi,
						vif->fw_vif_idx, WOW_LIST_ID,
						wow->patterns[i].pattern_len,
						i,
						0,
						wow->patterns[i].pattern,
						wow->patterns[i].mask);
				/* filter for wow ext pattern */
				filter |= WOW_FILTER_OPTION_PATTERNS;
			} else {
				/*
				 * Convert given nl80211 specific mask value to
				 * equivalent driver specific mask value
				 * and send it to the chip along with patterns.
				 * For example, if the mask value defined
				 * in struct cfg80211_wowlan is 0xA
				 * (equivalent binary is 1010), then equivalent
				 * driver specific mask value is
				 * "0xFF 0x00 0xFF 0x00".
				 */
				memset(&mask, 0, sizeof(mask));
				for (pos = 0;
					pos < wow->patterns[i].pattern_len;
					pos++) {
					if (wow->patterns[i].mask[pos / 8] &
						(0x1 << (pos % 8)))
						mask[pos] = 0xFF;
				}
				/*
				 * Note: Pattern's offset is not passed
				 * as part of wowlan parameter from CFG layer.
				 * So it's always passed as ZERO to
				 * the firmware.
				 * It means, given WOW patterns are always
				 * matched from the first byte
				 * of received pkt in the firmware.
				 */
				ret = ath6kl_wmi_add_wow_pattern_cmd(ar->wmi,
						vif->fw_vif_idx, WOW_LIST_ID,
						wow->patterns[i].pattern_len,
						0 /* pattern offset */,
						wow->patterns[i].pattern, mask);
			}
			if (ret)
				return ret;
		}
	} else
		filter |= WOW_FILTER_OPTION_PATTERNS;

	if (wow) {
		if (wow->disconnect || wow->any) {
			ath6kl_dbg(ATH6KL_DBG_WOWLAN, "filter: WOW_FILTER_OPTION_NWK_DISASSOC\n");
			filter |= WOW_FILTER_OPTION_NWK_DISASSOC;
		}

		if (wow->magic_pkt || wow->any) {
			ath6kl_dbg(ATH6KL_DBG_WOWLAN, "filter: WOW_FILTER_OPTION_MAGIC_PACKET\n");
			filter |= WOW_FILTER_OPTION_MAGIC_PACKET;
		}
		if (wow->gtk_rekey_failure || wow->any) {
			ath6kl_dbg(ATH6KL_DBG_WOWLAN,
				"filter: WOW_FILTER_OPTION_OFFLOAD_GTK/GTK_ERROR\n");
			filter |= (WOW_FILTER_OPTION_EAP_REQ |
						WOW_FILTER_OPTION_8021X_4WAYHS |
						WOW_FILTER_OPTION_GTK_ERROR |
						WOW_FILTER_OPTION_OFFLOAD_GTK);
		}
		if (wow->eap_identity_req || wow->any) {
			ath6kl_dbg(ATH6KL_DBG_WOWLAN, "filter: WOW_FILTER_OPTION_EAP_REQ\n");
			filter |= WOW_FILTER_OPTION_EAP_REQ;

		}

		if (wow->four_way_handshake || wow->any) {
			ath6kl_dbg(ATH6KL_DBG_WOWLAN, "filter: WOW_FILTER_OPTION_8021X_4WAYHS\n");
			filter |= WOW_FILTER_OPTION_8021X_4WAYHS;
		}

		if (vif->arp_offload_ip_set || wow->any) {
			ath6kl_dbg(ATH6KL_DBG_WOWLAN, "filter: WOW_FILTER_OPTION_OFFLOAD_ARP\n");
			filter |= WOW_FILTER_OPTION_OFFLOAD_ARP;
		}
	}

	/*Do GTK offload in WPA/WPA2 auth mode connection.*/
	if (vif->auth_mode == WPA2_AUTH_CCKM || vif->auth_mode == WPA2_PSK_AUTH
	  || vif->auth_mode == WPA_AUTH_CCKM || vif->auth_mode == WPA_PSK_AUTH){
		ath6kl_dbg(ATH6KL_DBG_WOWLAN, "filter: WOW_FILTER_OPTION_OFFLOAD_GTK\n");
		filter |= WOW_FILTER_OPTION_OFFLOAD_GTK;
	}

	if (filter || (wow && wow->n_patterns)) {
		ath6kl_dbg(ATH6KL_DBG_WOWLAN, "Set filter: 0x%x ", filter);
		ret = ath6kl_wmi_set_wow_mode_cmd(ar->wmi, vif->fw_vif_idx,
						ATH6KL_WOW_MODE_ENABLE,
						filter,
						host_req_delay);
	}

	set_bit(WLAN_WOW_ENABLE, &vif->flags);

FAIL:
	ath6kl_dbg(ATH6KL_DBG_WOWLAN, "%s: --- return %d\n", __func__, ret);
	return ret;
}

int ath6kl_clear_wow_mode(struct wiphy *wiphy)
{
	struct ath6kl_vif *vif;
	struct ath6kl *ar = wiphy_priv(wiphy);
	int ret = 0;

	ath6kl_dbg(ATH6KL_DBG_WOWLAN, "%s: +++\n", __func__);

	vif = ath6kl_vif_first(ar);
	if (!vif)
		return -EIO;
	ret = ath6kl_wmi_set_wow_mode_cmd(ar->wmi, vif->fw_vif_idx,
						  ATH6KL_WOW_MODE_DISABLE,
						  0,
						  0);

	ret = ath6kl_wmi_set_host_sleep_mode_cmd(ar->wmi, vif->fw_vif_idx,
						 ATH6KL_HOST_MODE_AWAKE);

	clear_bit(WLAN_WOW_ENABLE, &vif->flags);
	ath6kl_dbg(ATH6KL_DBG_WOWLAN, "%s: ---\n", __func__);

	return ret;
}
#endif /*CONFIG_ANDROID*/

static const struct ieee80211_txrx_stypes
ath6kl_mgmt_stypes[NUM_NL80211_IFTYPES] = {
	[NL80211_IFTYPE_STATION] = {
		.tx = BIT(IEEE80211_STYPE_ACTION >> 4) |
		BIT(IEEE80211_STYPE_PROBE_RESP >> 4),
		.rx = BIT(IEEE80211_STYPE_ACTION >> 4) |
		BIT(IEEE80211_STYPE_PROBE_REQ >> 4)
#ifdef CE_SUPPORT
		| BIT(IEEE80211_STYPE_PROBE_RESP >> 4)
#endif
	},
	[NL80211_IFTYPE_AP] = {
		.tx = BIT(IEEE80211_STYPE_ACTION >> 4) |
		BIT(IEEE80211_STYPE_PROBE_RESP >> 4),
		.rx = BIT(IEEE80211_STYPE_ACTION >> 4) |
		BIT(IEEE80211_STYPE_PROBE_REQ >> 4)
#ifdef CE_SUPPORT
		| BIT(IEEE80211_STYPE_PROBE_RESP >> 4)
#endif
	},
	[NL80211_IFTYPE_P2P_CLIENT] = {
		.tx = BIT(IEEE80211_STYPE_ACTION >> 4) |
		BIT(IEEE80211_STYPE_PROBE_RESP >> 4),
		.rx = BIT(IEEE80211_STYPE_ACTION >> 4) |
		BIT(IEEE80211_STYPE_PROBE_REQ >> 4)
#ifdef CE_SUPPORT
		| BIT(IEEE80211_STYPE_PROBE_RESP >> 4)
#endif
	},
	[NL80211_IFTYPE_P2P_GO] = {
		.tx = BIT(IEEE80211_STYPE_ACTION >> 4) |
		BIT(IEEE80211_STYPE_PROBE_RESP >> 4),
		.rx = BIT(IEEE80211_STYPE_ACTION >> 4) |
		BIT(IEEE80211_STYPE_PROBE_REQ >> 4)
#ifdef CE_SUPPORT
		|BIT(IEEE80211_STYPE_PROBE_RESP >> 4)
#endif
	},
};

#ifdef NL80211_CMD_SET_AP_MAC_ACL
static int ath6kl_cfg80211_ap_acl(struct wiphy *wiphy, struct net_device *dev,
					const struct cfg80211_acl_data *params)
{
	struct ath6kl_vif *vif = netdev_priv(dev);

	if (params)
		_ath6kl_acl_config(vif, params);

	return 0;
}
#endif

/* NOTE : this table may be over-wrote by ath6kl_change_cfg80211_ops() call. */
static struct cfg80211_ops ath6kl_cfg80211_ops = {
	.add_virtual_intf = ath6kl_cfg80211_add_iface,
	.del_virtual_intf = ath6kl_cfg80211_del_iface,
	.change_virtual_intf = ath6kl_cfg80211_change_iface,
	.change_bss = ath6kl_cfg80211_change_bss,
	.scan = ath6kl_cfg80211_scan,
	.connect = ath6kl_cfg80211_connect,
	.disconnect = ath6kl_cfg80211_disconnect,
	.add_key = ath6kl_cfg80211_add_key,
	.get_key = ath6kl_cfg80211_get_key,
	.del_key = ath6kl_cfg80211_del_key,
	.set_default_key = ath6kl_cfg80211_set_default_key,
	.set_wiphy_params = ath6kl_cfg80211_set_wiphy_params,
	.set_tx_power = ath6kl_cfg80211_set_txpower,
	.get_tx_power = ath6kl_cfg80211_get_txpower,
	.set_power_mgmt = ath6kl_cfg80211_set_power_mgmt,
	.join_ibss = ath6kl_cfg80211_join_ibss,
	.leave_ibss = ath6kl_cfg80211_leave_ibss,
	.get_station = ath6kl_get_station,
	.dump_station = ath6kl_dump_station,
	.set_bitrate_mask = ath6kl_cfg80211_set_bitrate_mask,
	.set_pmksa = ath6kl_set_pmksa,
	.del_pmksa = ath6kl_del_pmksa,
	.flush_pmksa = ath6kl_flush_pmksa,
	CFG80211_TESTMODE_CMD(ath6kl_tm_cmd)
#ifdef CONFIG_PM
	.suspend = __ath6kl_cfg80211_suspend,
	.resume = __ath6kl_cfg80211_resume,
#ifdef NL80211_CMD_GET_WOWLAN_QCA
	.set_wow_mode = ath6kl_set_wow_mode,
	.clr_wow_mode = ath6kl_clear_wow_mode,
#endif
#endif
	.set_rekey_data = ath6kl_set_gtk_rekey_offload,
#ifndef CFG80211_NO_SET_CHAN_OPERATION
	.set_channel = ath6kl_set_channel,
#endif
#ifdef NL80211_CMD_START_STOP_AP
	.start_ap = ath6kl_start_ap,
	.change_beacon = ath6kl_change_beacon,
	.stop_ap = ath6kl_del_beacon,
#else
	.add_beacon = ath6kl_add_beacon,
	.set_beacon = ath6kl_set_beacon,
	.del_beacon = ath6kl_del_beacon,
#endif
	.del_station = ath6kl_del_station,
	.change_station = ath6kl_change_station,
	.remain_on_channel = ath6kl_remain_on_channel,
	.cancel_remain_on_channel = ath6kl_cancel_remain_on_channel,
	.mgmt_tx = ath6kl_mgmt_tx,
	.mgmt_frame_register = ath6kl_mgmt_frame_register,
#ifdef NL80211_CMD_BTCOEX_QCA
	.notify_btcoex = ath6kl_notify_btcoex,
#endif
};

void ath6kl_cfg80211_stop(struct ath6kl_vif *vif)
{
	vif->saved_pwr_mode = vif->last_pwr_mode;

	/* put all interfaces to power save */
	if (ath6kl_wmi_powermode_cmd(vif->ar->wmi,
		vif->fw_vif_idx, REC_POWER) != 0) {
		ath6kl_err("wmi powermode command failed during suspend\n");
	}

	switch (vif->sme_state) {
	case SME_CONNECTING:
		ath6kl_cfg80211_connect_result(vif, vif->bssid, NULL, 0,
					NULL, 0,
					WLAN_STATUS_UNSPECIFIED_FAILURE,
					GFP_KERNEL);
		break;
	case SME_CONNECTED:
	default:
		/*
		 * FIXME: oddly enough smeState is in DISCONNECTED during
		 * suspend, why? Need to send disconnected event in that
		 * state.
		 */
		ath6kl_cfg80211_disconnected(vif, 0, NULL, 0, GFP_KERNEL);
		break;
	}

	if (test_bit(CONNECTED, &vif->flags) ||
	    test_bit(CONNECT_PEND, &vif->flags))
		ath6kl_wmi_disconnect_cmd(vif->ar->wmi, vif->fw_vif_idx);

	vif->sme_state = SME_DISCONNECTED;
	clear_bit(CONNECTED, &vif->flags);
	clear_bit(CONNECT_PEND, &vif->flags);
	clear_bit(CONNECT_HANDSHAKE_PROTECT, &vif->flags);
	clear_bit(PS_STICK, &vif->flags);
	del_timer(&vif->shprotect_timer);

	/* disable scanning */
	if (ath6kl_wmi_scanparams_cmd(vif->ar->wmi, vif->fw_vif_idx,
		0xFFFF, 0, 0, 0, 0, 0, 0, 0, 0, 0) != 0)
		printk(KERN_WARNING "ath6kl: failed to disable scan "
		       "during suspend\n");

	if (test_bit(SCANNING, &vif->flags))
		ath6kl_cfg80211_scan_complete_event(vif, true);

	return;
}

void ath6kl_cfg80211_stop_all(struct ath6kl *ar)
{
	struct ath6kl_vif *vif, *tmp_vif;
#ifdef CE_SUPPORT
	list_for_each_entry_safe(vif, tmp_vif, &ar->vif_list, list) {
		if (ar->state == ATH6KL_STATE_DEEPSLEEP) {
			if (vif->nw_type != AP_NETWORK)
				ath6kl_cfg80211_stop(vif);
		}
	}
#else
	list_for_each_entry_safe(vif, tmp_vif, &ar->vif_list, list)
		ath6kl_cfg80211_stop(vif);
#endif
}

static void ath6kl_change_cfg80211_ops(struct cfg80211_ops *cfg80211_ops)
{
	/* Offload AP keep-alive function to user. */
	if (debug_quirks & ATH6KL_MODULE_KEEPALIVE_BY_SUPP) {
		WARN_ON(cfg80211_ops->probe_client);

		cfg80211_ops->probe_client = ath6kl_ap_probe_client;

		ath6kl_info("Offload AP Keep-alive to supplicant/hostapd\n");
	}

	/* Support Scheduled-Scan (a.k.a. PNO) */
	if (debug_quirks & ATH6KL_MODULE_ENABLE_SCHE_SCAN) {
		cfg80211_ops->sched_scan_start = ath6kl_sched_scan_start;
		cfg80211_ops->sched_scan_stop = ath6kl_sched_scan_stop;
	}

#ifdef NL80211_CMD_SET_AP_MAC_ACL
	if (debug_quirks & ATH6KL_MODULE_AP_ACL_BY_NL80211) {
		cfg80211_ops->set_mac_acl = ath6kl_cfg80211_ap_acl;

		ath6kl_info("Configurate AP-ACL from NL80211.\n");
	}
#endif

	return;
}

static void ath6kl_reset_cfg80211_ops(struct cfg80211_ops *cfg80211_ops)
{
	cfg80211_ops->probe_client = NULL;

	cfg80211_ops->sched_scan_start = NULL;
	cfg80211_ops->sched_scan_stop = NULL;

	return;
}

static void _judge_p2p_framework(struct ath6kl *ar, unsigned int p2p_config)
{
	ar->p2p = !!p2p_config;

	ar->p2p_concurrent =
		!!(p2p_config & ATH6KL_MODULEP2P_CONCURRENT_ENABLE_DEDICATE) ||
		!!(p2p_config & ATH6KL_MODULEP2P_CONCURRENT_NO_DEDICATE) ||
		!!(p2p_config & ATH6KL_MODULEP2P_CONCURRENT_MULTICHAN) ||
		!!(p2p_config & ATH6KL_MODULEP2P_CONCURRENT_COMPAT);

	ar->p2p_dedicate =
		!!(p2p_config & ATH6KL_MODULEP2P_CONCURRENT_ENABLE_DEDICATE) ||
		!!(p2p_config & ATH6KL_MODULEP2P_CONCURRENT_COMPAT);

	ar->p2p_multichan_concurrent =
		!!(p2p_config & ATH6KL_MODULEP2P_CONCURRENT_MULTICHAN);

	ar->p2p_compat =
		!!(p2p_config & ATH6KL_MODULEP2P_CONCURRENT_COMPAT);

	ar->p2p_concurrent_ap =
		!!(p2p_config & ATH6KL_MODULEP2P_CONCURRENT_AP);

	ar->p2p_in_pasv_chan =
		!!(p2p_config & ATH6KL_MODULEP2P_P2P_IN_PASSIVE_CHAN);

	ar->p2p_wise_scan =
		!!(p2p_config & ATH6KL_MODULEP2P_P2P_WISE_SCAN);

	WARN_ON((!ar->p2p_concurrent) && (ar->p2p_multichan_concurrent));
	WARN_ON((ar->p2p_concurrent_ap) &&
		((!ar->p2p_concurrent) || (!ar->p2p_dedicate)));
	WARN_ON((!ar->p2p) && (ar->p2p_wise_scan));

	if (ar->p2p_concurrent) {
		if (ath6kl_mod_debug_quirks(ar, ATH6KL_MODULE_P2P_MAX_FW_VIF))
			ar->vif_max = TARGET_VIF_MAX;	/* TODO */
		else {
			if (ar->p2p_dedicate)
				ar->vif_max = 3;
			else
				ar->vif_max = 2; /*currently we only support 2*/

		}
	} else
		ar->vif_max = 1;

	/*
	 * P2P-Concurrent w/ softAP:
	 *  STA + AP       : vif_max == 3 && ar->p2p_concurrent_ap
	 *  STA + P2P + AP : vif_max >= 4 && ar->p2p_concurrent_ap (not yet)
	 *  w/ MCC         : (not yet)
	 */
	WARN_ON((ar->p2p_concurrent_ap) && (ar->vif_max >= TARGET_VIF_MAX));
	WARN_ON((ar->p2p_concurrent_ap) && (ar->p2p_multichan_concurrent));

	ar->max_norm_iface = 1;
	if (ar->p2p_concurrent_ap)
		ar->max_norm_iface++;

	ar->p2p_frame_retry = false;

	if (ar->p2p_compat)
		ar->p2p_frame_not_report = true;
	else
		ar->p2p_frame_not_report = false;

	ar->p2p_war_bad_intel_go = true;
	ar->p2p_war_bad_broadcom_go = true;
	ar->p2p_war_p2p_client_awake = true;

	ar->p2p_ie_not_append = P2P_IE_IN_PROBE_REQ;

	ath6kl_info("%dVAP/%d, P2P %s, concurrent %s %s,"
		" %s dedicate p2p-device,"
		" multi-channel-concurrent %s, p2p-compat %s%s%s%s\n",
		ar->vif_max,
		ar->max_norm_iface,
		(ar->p2p ? "enable" : "disable"),
		(ar->p2p_concurrent ? "on" : "off"),
		(ar->p2p_concurrent_ap ? "with softAP" : ""),
		(ar->p2p_dedicate ? "with" : "without"),
		(ar->p2p_multichan_concurrent ? "enable" : "disable"),
		(ar->p2p_compat ? "enable (ignore p2p_dedicate)" : "disable"),
		(ar->p2p_ie_not_append ? ", sta-p2p-ie removed" : ""),
		(ar->p2p_in_pasv_chan ? ", p2p_in_pasv_chan enable" : ""),
		(ar->p2p_wise_scan ? ", p2p_wise_scan enable" : ""));

	return;
}

static void _judge_vap_framework(struct ath6kl *ar, unsigned int vap_config)
{
	int i;
	int ap_num = 0, sta_num = 0;

	/*
	 * TODO : To replace older ath6kl_p2p, in case of vap_config have
	 *        P2P related VAP mode and transfer vap_config to XXX here
	 *        then feed to _judge_p2p_framework() to configure P2P's
	 *        framework.
	 */

	for (i = 0; i < ATH6KL_VIF_MAX; i++) {
		ar->next_mode[i] = (vap_config >>
					(i * ATH6KL_VAPMODE_OFFSET)) &
				   ATH6KL_VAPMODE_MASK;

		if (ar->next_mode[i] == ATH6KL_VAPMODE_DISABLED)
			break;

		if (ar->next_mode[i] == ATH6KL_VAPMODE_STA)
			sta_num++;
		else if (ar->next_mode[i] == ATH6KL_VAPMODE_AP)
			ap_num++;
	}

	/* Now, only support 1STA+1AP or 2AP */
	WARN_ON((sta_num > 1) ||
		(ap_num > 2) ||
		(sta_num + ap_num > 2));

	ar->vif_max = i;
	ar->max_norm_iface = i;

	ath6kl_info("%dVAP/%d, NO P2P, vapmode %d, %d, %d, %d\n",
		ar->vif_max,
		ar->max_norm_iface,
		ar->next_mode[0], ar->next_mode[1],
		ar->next_mode[2], ar->next_mode[3]);

}

struct ath6kl *ath6kl_core_alloc(struct device *dev)
{
	struct ath6kl *ar;
	struct wiphy *wiphy;

	/*
	 * Overwrite cfg80211_ops function table if we need to
	 * enable/disable some features.
	 */
	ath6kl_change_cfg80211_ops(&ath6kl_cfg80211_ops);

	/* create a new wiphy for use with cfg80211 */
	wiphy = wiphy_new(&ath6kl_cfg80211_ops, sizeof(struct ath6kl));

	if (!wiphy) {
		ath6kl_err("couldn't allocate wiphy device\n");
		return NULL;
	}

	ar = wiphy_priv(wiphy);
	ar->mod_debug_quirks = (debug_quirks 
#if defined(CE_2_SUPPORT)
		| ATH6KL_MODULE_DISABLE_WAIT_DEFER 
#endif
		| ATH6KL_MODULE_DISABLE_WMI_SYC);

	ar->wiphy = wiphy;
	ar->dev = dev;

	BUG_ON((ath6kl_p2p && ath6kl_vap));

	/*
	 * For backward compatible, keep ATH6KL_MODULE_TESTMODE_ENABLE as
	 * testmode = 1. Note that if want to configure as testmode=2,
	 * Must not configure ATH6KL_MODULE_TESTMODE_ENABLE,
	 * And configure testmode = 2 as module parameter directly.
	 */
	if (ath6kl_mod_debug_quirks(ar, ATH6KL_MODULE_TESTMODE_ENABLE))
		testmode = 1;

	if (testmode ||
		ath6kl_mod_debug_quirks(ar, ATH6KL_MODULE_ENABLE_EPPING)) {
		ath6kl_p2p = 0;
		ath6kl_vap = 0;
		ath6kl_roam_mode = ATH6KL_MODULEROAM_DISABLE;
	}

	if (ath6kl_p2p)
		_judge_p2p_framework(ar, ath6kl_p2p);
	else if (ath6kl_vap)
		_judge_vap_framework(ar, ath6kl_vap);
	else if (ath6kl_dsrc) {
		ar->vif_max = 2;
		ar->max_norm_iface = 2;
	} else {
		ar->vif_max = 1;
		ar->max_norm_iface = 1;
	}

	spin_lock_init(&ar->lock);
	spin_lock_init(&ar->list_lock);
	spin_lock_init(&ar->state_lock);

	init_waitqueue_head(&ar->event_wq);
	sema_init(&ar->sem, 1);
	sema_init(&ar->wmi_evt_sem, 1);

	INIT_LIST_HEAD(&ar->amsdu_rx_buffer_queue);
	INIT_LIST_HEAD(&ar->vif_list);

	setup_timer(&ar->eapol_shprotect_timer,
			ath6kl_eapol_shprotect_timer_handler,
			(unsigned long) ar);

	clear_bit(WMI_ENABLED, &ar->flag);
	clear_bit(SKIP_SCAN, &ar->flag);
	clear_bit(DESTROY_IN_PROGRESS, &ar->flag);
	clear_bit(EAPOL_HANDSHAKE_PROTECT, &ar->flag);

	ar->listen_intvl_t = A_DEFAULT_LISTEN_INTERVAL;
	ar->listen_intvl_b = 0;
	ar->tx_pwr = 0;

	ar->low_rssi_roam_params.lrssi_scan_period = WMI_ROAM_LRSSI_SCAN_PERIOD;
	ar->low_rssi_roam_params.lrssi_scan_threshold = \
		WMI_ROAM_LRSSI_SCAN_THRESHOLD;
	ar->low_rssi_roam_params.lrssi_roam_threshold = \
		WMI_ROAM_LRSSI_ROAM_THRESHOLD;
	ar->low_rssi_roam_params.roam_rssi_floor = WMI_ROAM_LRSSI_ROAM_FLOOR;

	ar->state = ATH6KL_STATE_OFF;

	if ((ar->p2p_concurrent) &&
	    (ar->p2p_dedicate))
		ar->sche_scan = ath6kl_mod_debug_quirks(ar,
			ATH6KL_MODULE_ENABLE_SCHE_SCAN);

#ifdef CONFIG_ANDROID
	if (machine_is_apq8064_dma() || machine_is_apq8064_bueller())
		ath6kl_roam_mode = ATH6KL_MODULEROAM_DISABLE;
#endif

	ar->roam_mode = ath6kl_roam_mode;

	if (ar->sche_scan &&
		 ar->roam_mode != ATH6KL_MODULEROAM_DISABLE) {
		ath6kl_err("Roam shall be exclusive with schedule scan. Disabled\n");
		ar->roam_mode = ATH6KL_MODULEROAM_DISABLE;
	}

	return ar;
}

int ath6kl_register_ieee80211_hw(struct ath6kl *ar)
{
	struct wiphy *wiphy = ar->wiphy;
	int ret;

	wiphy->mgmt_stypes = ath6kl_mgmt_stypes;

	wiphy->max_remain_on_channel_duration = ATH6KL_ROC_MAX_PERIOD * 1000;

	/* set device pointer for wiphy */
	set_wiphy_dev(wiphy, ar->dev);

	wiphy->interface_modes = BIT(NL80211_IFTYPE_STATION) |
				 BIT(NL80211_IFTYPE_ADHOC) |
				 BIT(NL80211_IFTYPE_AP);
	if (ar->p2p) {
		if (!IS_STA_AP_ONLY(ar))
			wiphy->interface_modes |=
				BIT(NL80211_IFTYPE_P2P_GO) |
				BIT(NL80211_IFTYPE_P2P_CLIENT);
	}

	if (ar->p2p_concurrent) {
		struct ieee80211_iface_combination
			*ieee80211_iface_combination = NULL;

		switch (ar->vif_max) {
		case 4:
			ieee80211_iface_combination =
				ath6kl_iface_combinations_p2p_concurrent4;
			wiphy->n_iface_combinations = ARRAY_SIZE(
				ath6kl_iface_combinations_p2p_concurrent4);
			break;
		case 3:
			ieee80211_iface_combination =
				ath6kl_iface_combinations_p2p_concurrent3;
			wiphy->n_iface_combinations = ARRAY_SIZE(
				ath6kl_iface_combinations_p2p_concurrent3);
			break;
		case 2:
			ieee80211_iface_combination =
				ath6kl_iface_combinations_p2p_concurrent2;
			wiphy->n_iface_combinations = ARRAY_SIZE(
				ath6kl_iface_combinations_p2p_concurrent2);
			break;
		default:
			WARN_ON(1);
		}

		/* Overwrite it if P2P-Concurrent w/ softAP mode. */
		if (ar->p2p_concurrent_ap) {
			if (IS_STA_AP_ONLY(ar)) {
				ieee80211_iface_combination =
				ath6kl_iface_combinations_sta_ap;
				wiphy->n_iface_combinations = ARRAY_SIZE(
				ath6kl_iface_combinations_sta_ap);
			} else {
				ieee80211_iface_combination =
				ath6kl_iface_combinations_p2p_concurrent4_1;
				wiphy->n_iface_combinations = ARRAY_SIZE(
				ath6kl_iface_combinations_p2p_concurrent4_1);
			}
		}

		/* Update max. channel support */
		if (ar->p2p_multichan_concurrent &&
				ieee80211_iface_combination != NULL) {
			ieee80211_iface_combination[0].num_different_channels =
			ieee80211_iface_combination[0].max_interfaces;
		}

		wiphy->iface_combinations = ieee80211_iface_combination;

		if (ar->p2p_compat) {
			wiphy->n_iface_combinations = 0;
			wiphy->iface_combinations = NULL;
		}
	}

	/* update MCS rate mask & TX STBC. */
	if (!(ar->target_subtype & TARGET_SUBTYPE_2SS)) {
		ath6kl_band_2ghz.ht_cap.mcs.rx_mask[1] = 0;
		ath6kl_band_5ghz.ht_cap.mcs.rx_mask[1] = 0;
                ath6kl_band_dsrc.ht_cap.mcs.rx_mask[1] = 0;

		ath6kl_band_2ghz.ht_cap.cap &= ~IEEE80211_HT_CAP_TX_STBC;
		ath6kl_band_5ghz.ht_cap.cap &= ~IEEE80211_HT_CAP_TX_STBC;
                ath6kl_band_dsrc.ht_cap.cap &= ~IEEE80211_HT_CAP_TX_STBC;
	}

	/* update 2G-HT40 capability. */
	if (!(ar->target_subtype & TARGET_SUBTYPE_HT40)) {
		ath6kl_band_2ghz.ht_cap.cap &=
			~IEEE80211_HT_CAP_SUP_WIDTH_20_40;
		ath6kl_band_2ghz.ht_cap.cap &= ~IEEE80211_HT_CAP_SGI_40;
		ath6kl_band_2ghz.ht_cap.cap &= ~IEEE80211_HT_CAP_DSSSCCK40;
	}

	/* max num of ssids that can be probed during scanning */
	wiphy->max_scan_ssids = MAX_PROBED_SSID_INDEX;
	wiphy->max_scan_ie_len = MAX_APP_IE_LEN;
	if (ar->sche_scan) {
		wiphy->flags |= WIPHY_FLAG_SUPPORTS_SCHED_SCAN;
		wiphy->max_sched_scan_ssids = MAX_PROBED_SSID_INDEX;
		wiphy->max_sched_scan_ie_len = MAX_APP_IE_LEN;
	}

	wiphy->bands[IEEE80211_BAND_2GHZ] = &ath6kl_band_2ghz;
	if ((!ath6kl_mod_debug_quirks(ar, ATH6KL_MODULE_DISABLE_5G)) &&
	    (ar->target_subtype & TARGET_SUBTYPE_DUAL))
		wiphy->bands[IEEE80211_BAND_5GHZ] = &ath6kl_band_5ghz;
	else {
		ath6kl_info("Disable 5G support by %s!\n",
			(ath6kl_mod_debug_quirks(ar, ATH6KL_MODULE_DISABLE_5G) ?
			"driver" : "board-data"));
	}
	wiphy->signal_type = CFG80211_SIGNAL_TYPE_MBM;

	wiphy->cipher_suites = cipher_suites;
	wiphy->n_cipher_suites = ARRAY_SIZE(cipher_suites);

	wiphy->flags |= WIPHY_FLAG_AP_UAPSD;
#ifdef CFG80211_WIPHY_FLAG_HAS_REMAIN_ON_CHANNEL
	if ((ath6kl_cfg80211_ops.remain_on_channel) &&
	    (ath6kl_cfg80211_ops.cancel_remain_on_channel))
		wiphy->flags |= WIPHY_FLAG_HAS_REMAIN_ON_CHANNEL;
#endif

	if (ar->roam_mode != ATH6KL_MODULEROAM_DISABLE)
		wiphy->flags |= WIPHY_FLAG_SUPPORTS_FW_ROAM;

#ifdef NL80211_ATTR_AP_INACTIVITY_TIMEOUT
	if (ath6kl_mod_debug_quirks(ar,	ATH6KL_MODULE_KEEPALIVE_CONFIG_BY_SUPP))
		wiphy->features |= NL80211_FEATURE_INACTIVITY_TIMER;
#endif

#ifdef NL80211_WIPHY_FEATURE_SCAN_FLUSH
	/*
	 * The cfg80211 support it by default but we disabled here.
	 * To speed up the scan time and the channel dewell time is not really
	 * longer enough. Cache BSS information may be helpful to ath6kl.
	 */
	wiphy->features &= ~NL80211_FEATURE_SCAN_FLUSH;
#endif

#ifdef CFG80211_TX_POWER_PER_WDEV
	wiphy->features |= NL80211_FEATURE_VIF_TXPOWER;
#endif

#ifdef CONFIG_PM
	wiphy->wowlan.flags = WIPHY_WOWLAN_ANY |
				WIPHY_WOWLAN_MAGIC_PKT |
				WIPHY_WOWLAN_DISCONNECT |
				WIPHY_WOWLAN_SUPPORTS_GTK_REKEY |
				WIPHY_WOWLAN_GTK_REKEY_FAILURE |
				WIPHY_WOWLAN_EAP_IDENTITY_REQ	|
				WIPHY_WOWLAN_4WAY_HANDSHAKE	|
				WIPHY_WOWLAN_RFKILL_RELEASE;

	wiphy->wowlan.n_patterns =
		(WOW_MAX_FILTER_LISTS*WOW_MAX_FILTERS_PER_LIST);
	wiphy->wowlan.pattern_min_len = WOW_MIN_PATTERN_SIZE;
	wiphy->wowlan.pattern_max_len = WOW_MAX_PATTERN_SIZE;
	ath6kl_dbg(ATH6KL_DBG_WOWLAN, "wow attr: flags: %x, n_patterns: %d, ",
			wiphy->wowlan.flags,
			wiphy->wowlan.n_patterns);
	ath6kl_dbg(ATH6KL_DBG_WOWLAN, "min_len: %d, max_len: %d\n",
			wiphy->wowlan.pattern_min_len,
			wiphy->wowlan.pattern_max_len);
#endif

#ifdef CONFIG_ANDROID
#ifdef CONFIG_HAS_WAKELOCK
	wake_lock_init(&ar->wake_lock, WAKE_LOCK_SUSPEND, "ath6kl_suspend_wl");
#endif

	if (ar->wow_irq) {
		int ret;
		ret = request_irq(ar->wow_irq, ath6kl_wow_irq,
				IRQF_SHARED | IRQF_TRIGGER_RISING,
				"ar6000" "sdiowakeup", ar);
		if (!ret) {
			ret = enable_irq_wake(ar->wow_irq);
			if (ret < 0) {
				ath6kl_err("Couldn't enable WoW IRQ as wakeup interrupt");
				return ret;
			}
			printk("ath6kl: WoW IRQ %d\n", ar->wow_irq);
		}
	}

	ath6kl_setup_android_resource(ar);
#endif

	if (test_bit(INTERNAL_REGDB, &ar->flag) ||
	    test_bit(CFG80211_REGDB, &ar->flag)) {
#ifdef CFG80211_VOID_REG_NOTIFIER
		wiphy->reg_notifier = ath6kl_reg_notifier2;
#else
		wiphy->reg_notifier = ath6kl_reg_notifier;
#endif
		wiphy->flags |= WIPHY_FLAG_CUSTOM_REGULATORY;
		wiphy->flags |= WIPHY_FLAG_STRICT_REGULATORY;
		wiphy->flags |= WIPHY_FLAG_DISABLE_BEACON_HINTS;
	}

#ifdef NL80211_CMD_SET_AP_MAC_ACL
	if (debug_quirks & ATH6KL_MODULE_AP_ACL_BY_NL80211)
		wiphy->max_acl_mac_addrs = ATH6KL_AP_ACL_MAX_NUM;
#endif

	ret = wiphy_register(wiphy);
	if (ret < 0) {
		ath6kl_err("couldn't register wiphy device\n");
		return ret;
	}

	return 0;
}

static int ath6kl_init_if_data(struct ath6kl_vif *vif)
{
	int i;
	struct ath6kl *ar = vif->ar;

	vif->aggr_cntxt = aggr_init(vif);
	if (!vif->aggr_cntxt) {
		ath6kl_err("failed to initialize aggr\n");
		return -ENOMEM;
	}

	for (i = 0; i < AP_MAX_NUM_STA; i++) {
		vif->sta_list[i].aggr_conn_cntxt = aggr_init_conn(vif);
		if (!vif->sta_list[i].aggr_conn_cntxt) {
			ath6kl_err("failed to initialize aggr_node\n");
			return -ENOMEM;
		}
	}
#ifdef ACS_SUPPORT
	vif->acs_ctx = ath6kl_acs_init(vif);
	if (!vif->acs_ctx) {
		ath6kl_err("failed to initialize acs");
		return -ENOMEM;
	}
#endif
	vif->htcoex_ctx = ath6kl_htcoex_init(vif);
	if (!vif->htcoex_ctx) {
		ath6kl_err("failed to initialize htcoex\n");
		return -ENOMEM;
	}

#ifdef CONFIG_ANDROID
	/* Enable htcoex for wlan0 in Android. Scan period is 60s */
	if ((vif->fw_vif_idx == 0) &&
	    (vif->ar->target_subtype & TARGET_SUBTYPE_HT40))
		ath6kl_htcoex_config(vif, ATH6KL_HTCOEX_SCAN_PERIOD, 0);

	/* WAR: CR480066 */
	vif->bss_post_proc_ctx = ath6kl_bss_post_proc_init(vif);
	if (!vif->bss_post_proc_ctx) {
		ath6kl_err("failed to initialize bss_post_proc\n");
		return -ENOMEM;
	}
#endif

	vif->p2p_ps_info_ctx = ath6kl_p2p_ps_init(vif);
	if (!vif->p2p_ps_info_ctx) {
		ath6kl_err("failed to initialize p2p_ps\n");
		return -ENOMEM;
	}

	if (!test_bit(TESTMODE_EPPING, &ar->flag)) {
		if (ath6kl_mod_debug_quirks(vif->ar,
			ATH6KL_MODULE_KEEPALIVE_BY_SUPP)) {
			vif->ap_keepalive_ctx =
				ath6kl_ap_keepalive_init(vif,
						AP_KA_MODE_BYSUPP);
		} else if (ath6kl_mod_debug_quirks(vif->ar,
			ATH6KL_MODULE_KEEPALIVE_CONFIG_BY_SUPP)) {
			vif->ap_keepalive_ctx =
				ath6kl_ap_keepalive_init(vif,
						AP_KA_MODE_CONFIG_BYSUPP);
		} else {
			vif->ap_keepalive_ctx =
				ath6kl_ap_keepalive_init(vif,
						AP_KA_MODE_ENABLE);
		}

		if (!vif->ap_keepalive_ctx) {
			ath6kl_err("failed to initialize ap_keepalive\n");
			return -ENOMEM;
		}
	}

	vif->ap_acl_ctx = ath6kl_ap_acl_init(vif);
	if (!vif->ap_acl_ctx) {
		ath6kl_err("failed to initialize ap_acl\n");
		return -ENOMEM;
	}

	vif->ap_admc_ctx = ath6kl_ap_admc_init(vif, AP_ADMC_MODE_ACCEPT_ALWAYS);
	if (!vif->ap_admc_ctx) {
		ath6kl_err("failed to initialize ap_admc\n");
		return -ENOMEM;
	}

	ath6kl_ap_rc_init(vif);

	setup_timer(&vif->disconnect_timer, disconnect_timer_handler,
		    (unsigned long) vif->ndev);
	set_bit(WMM_ENABLED, &vif->flags);
	spin_lock_init(&vif->if_lock);

	setup_timer(&vif->vifscan_timer, ath6kl_scan_timer_handler,
			(unsigned long) vif);

	setup_timer(&vif->shprotect_timer, ath6kl_shprotect_timer_handler,
			(unsigned long) vif);
	spin_lock_init(&vif->pend_skb_lock);

	vif->scan_req = NULL;
	vif->pend_skb = NULL;

	vif->scanband_chan = 0;
	if (ar->p2p_wise_scan &&
	    (vif->fw_vif_idx != 0) &&
	    (vif->fw_vif_idx != (ar->vif_max - 1))) {
		/* Only apply to P2P interfaces. */
		vif->scanband_type = SCANBAND_TYPE_2_P2PCHAN;
	} else
		vif->scanband_type = SCANBAND_TYPE_ALL;

	vif->saved_pwr_mode =
	vif->last_pwr_mode = REC_POWER;
#ifdef CONFIG_SWOW_SUPPORT
	vif->last_suspend_wow = 0;
#endif

	return 0;
}

void ath6kl_deinit_if_data(struct ath6kl_vif *vif)
{
	struct ath6kl *ar = vif->ar;
	int ctr;

#ifdef ATH6KL_DIAGNOSTIC
	wifi_diag_timer_destroy(vif);
#endif

	for (ctr = 0; ctr < AP_MAX_NUM_STA ; ctr++) {
		if (vif->sta_list[ctr].psq_age_active) {
			del_timer_sync(&vif->sta_list[ctr].psq_age_timer);
			vif->sta_list[ctr].psq_age_active = 0;
		}
		if (!ath6kl_ps_queue_empty(&vif->sta_list[ctr].psq_data))
			ath6kl_ps_queue_purge(&vif->sta_list[ctr].psq_data);

		if (!ath6kl_ps_queue_empty(&vif->sta_list[ctr].psq_mgmt))
			ath6kl_ps_queue_purge(&vif->sta_list[ctr].psq_mgmt);

		aggr_module_destroy_conn(vif->sta_list[ctr].aggr_conn_cntxt);
	}

	netif_carrier_off(vif->ndev);
	netif_stop_queue(vif->ndev);
	ath6kl_tx_data_cleanup_by_if(vif);

	aggr_module_destroy(vif->aggr_cntxt);

#ifdef ACS_SUPPORT
	ath6kl_acs_deinit(vif);
#endif

	ath6kl_htcoex_deinit(vif);

	ath6kl_bss_post_proc_deinit(vif);

	ath6kl_p2p_ps_deinit(vif);

	ath6kl_ap_keepalive_deinit(vif);

	ath6kl_ap_acl_deinit(vif);

	ath6kl_ap_admc_deinit(vif);

	ath6kl_sched_scan_deinit(vif);

	ar->avail_idx_map |= BIT(vif->fw_vif_idx);

	if (vif->nw_type == ADHOC_NETWORK)
		ar->ibss_if_active = false;

	del_timer(&vif->vifscan_timer);

	unregister_netdevice(vif->ndev);

	ar->num_vif--;
}

struct net_device *ath6kl_interface_add(struct ath6kl *ar, char *name,
					enum nl80211_iftype type, u8 fw_vif_idx,
					u8 nw_type)
{
	struct net_device *ndev;
	struct ath6kl_vif *vif;
	int i;
#if defined(ATH6KL_HSIC_RECOVER) || defined(CE_2_SUPPORT)
	u8 zero_mac[ETH_ALEN] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
#endif

	ndev = alloc_netdev(sizeof(*vif), name, ether_setup);
	if (!ndev)
		return NULL;

	vif = netdev_priv(ndev);
	ndev->ieee80211_ptr = &vif->wdev;
	vif->wdev.wiphy = ar->wiphy;
	vif->ar = ar;
	vif->ndev = ndev;
	SET_NETDEV_DEV(ndev, wiphy_dev(vif->wdev.wiphy));
	vif->wdev.netdev = ndev;
	vif->wdev.iftype = type;
	vif->fw_vif_idx = fw_vif_idx;
	vif->nw_type = vif->next_mode = nw_type;

	memcpy(ndev->dev_addr, ar->mac_addr, ETH_ALEN);

#if defined(ATH6KL_HSIC_RECOVER) || defined(CE_2_SUPPORT)
	/* If we don't get the mac address just in time */
	if (memcmp(ar->mac_addr, zero_mac, ETH_ALEN)  == 0 &&
		cached_mac_valid == true) {
		memcpy(ndev->dev_addr, cached_mac, ETH_ALEN);
	}
#endif

	if (fw_vif_idx != 0)
		ndev->dev_addr[0] = (ndev->dev_addr[0] ^ (1 << fw_vif_idx)) |
				     0x2;

	init_netdev(ndev);

	ath6kl_init_control_info(vif);

	/* ATH6KL_MODULE_ENABLE_EPPING is enable will skip
	 * the following procedure */
	if (!ath6kl_mod_debug_quirks(vif->ar, ATH6KL_MODULE_ENABLE_EPPING)) {
		/* TODO: Pass interface specific pointer instead of ar */
		if (ath6kl_init_if_data(vif))
			goto err;
	}

	if (register_netdevice(ndev))
		goto err;

	ar->avail_idx_map &= ~BIT(fw_vif_idx);
	vif->sme_state = SME_DISCONNECTED;
	set_bit(WLAN_ENABLED, &vif->flags);
	ar->wlan_pwr_state = WLAN_POWER_STATE_ON;
	set_bit(NETDEV_REGISTERED, &vif->flags);

	if (type == NL80211_IFTYPE_ADHOC)
		ar->ibss_if_active = true;

	spin_lock_bh(&ar->list_lock);
	list_add_tail(&vif->list, &ar->vif_list);
	spin_unlock_bh(&ar->list_lock);

	ath6kl_p2p_utils_init_port(vif, type);

	ath6kl_sched_scan_init(vif);

	return ndev;

err:
	for (i = 0; i < AP_MAX_NUM_STA ; i++)
		aggr_module_destroy_conn(vif->sta_list[i].aggr_conn_cntxt);
	aggr_module_destroy(vif->aggr_cntxt);
	ath6kl_htcoex_deinit(vif);
	ath6kl_bss_post_proc_deinit(vif);
	ath6kl_p2p_ps_deinit(vif);
	ath6kl_ap_keepalive_deinit(vif);
	ath6kl_ap_acl_deinit(vif);
	ath6kl_ap_admc_deinit(vif);
	free_netdev(ndev);
	return NULL;
}

void ath6kl_deinit_ieee80211_hw(struct ath6kl *ar)
{
#ifdef CONFIG_ANDROID
	if (ar->wow_irq) {
		if (disable_irq_wake(ar->wow_irq))
			ath6kl_err("Couldn't disable hostwake IRQ wakeup mode\n");

		free_irq(ar->wow_irq, ar);
		ar->wow_irq = 0;
	}

#ifdef CONFIG_HAS_WAKELOCK
	wake_lock_destroy(&ar->wake_lock);
#endif

	ath6kl_cleanup_android_resource(ar);
#endif
	wiphy_unregister(ar->wiphy);
	wiphy_free(ar->wiphy);

	ath6kl_reset_cfg80211_ops(&ath6kl_cfg80211_ops);
}

void ath6kl_core_init_defer(struct work_struct *wk)
{
#define MAX_RD_WAIT_CNT	(20)	/* 20 * 100 = 2 sec. */
	struct ath6kl *ar;
	struct vif_params params;
	int i;

	ar = container_of(wk, struct ath6kl,
			    init_defer_wk);

	/* Automatically create virtual interface. */
	if (!ath6kl_mod_debug_quirks(ar,
		ATH6KL_MODULE_DISABLE_AUTO_ADD_INF) &&
	    (ar->p2p_concurrent) &&
	    (ar->p2p_dedicate)) {
		if (!IS_STA_AP_ONLY(ar)) {
			ath6kl_info("Create dedicated p2p interface\n");

			rtnl_lock();
			params.use_4addr = 0;
			if (ath6kl_cfg80211_add_iface(ar->wiphy,
							ATH6KL_DEVNAME_DEF_P2P,
							NL80211_IFTYPE_STATION,
							NULL,
							&params) == NULL) {
				ath6kl_err("Create dedicated p2p interface fail!\n");
			}
			rtnl_unlock();
		}

		if (ar->p2p_concurrent_ap) {
			ath6kl_info("Create concurrent ap interface\n");

			rtnl_lock();
			params.use_4addr = 0;
			if (ath6kl_cfg80211_add_iface(ar->wiphy,
							ATH6KL_DEVNAME_DEF_AP,
							NL80211_IFTYPE_AP,
							NULL,
							&params) == NULL) {
				ath6kl_err("Create concurrent ap interface fail!\n");
			}
			rtnl_unlock();
		}
	}

	/* Automatically create virtual interface. */
	if (!ath6kl_mod_debug_quirks(ar,
		ATH6KL_MODULE_DISABLE_AUTO_ADD_INF) &&
	    (!ar->p2p)) {
		for (i = 1; i < ATH6KL_VIF_MAX; i++) {
			rtnl_lock();
			params.use_4addr = 0;
			if (ar->next_mode[i] == ATH6KL_VAPMODE_STA) {
				if (ath6kl_cfg80211_add_iface(
							ar->wiphy,
							ATH6KL_DEVNAME_DEF_STA,
							NL80211_IFTYPE_STATION,
							NULL,
							&params) == NULL) {
					ath6kl_err("Create sta interface fail!\n");
				}
			} else if (ar->next_mode[i] == ATH6KL_VAPMODE_AP) {
				if (ath6kl_cfg80211_add_iface(
							ar->wiphy,
							ATH6KL_DEVNAME_DEF_AP,
							NL80211_IFTYPE_AP,
							NULL,
							&params) == NULL) {
					ath6kl_err("Create ap interface fail!\n");
				}
			}
			rtnl_unlock();
		}
	}

	/* Wait target report WMI_REGDOMAIN_EVENTID done */
	for (i = 0; i < MAX_RD_WAIT_CNT; i++) {
		if (ath6kl_reg_is_init_done(ar)) {
			/* wait more 2 jiffies for regdb updated */
			schedule_timeout_interruptible(2);
			break;
		}

		schedule_timeout_interruptible(msecs_to_jiffies(100));
	}

	clear_bit(INIT_DEFER_PROGRESS, &ar->flag);
	if (!ath6kl_mod_debug_quirks(ar, ATH6KL_MODULE_DISABLE_WAIT_DEFER))
		wake_up(&ar->init_defer_wait_wq);

	return;
#undef MAX_RD_WAIT_CNT
}

/* we use this flag to protect 4 way handshake */
void ath6kl_shprotect_timer_handler(unsigned long ptr)
{
	struct ath6kl_vif *vif = (struct ath6kl_vif *)ptr;
	struct ath6kl *ar = vif->ar;
	u8 bssid[ETH_ALEN];

	ath6kl_dbg(ATH6KL_DBG_WLAN_CFG, "%s\n", __func__);
	clear_bit(CONNECT_HANDSHAKE_PROTECT, &vif->flags);

	if (vif->pend_skb) {
		ath6kl_err("%s, shall not have pend skb\n", __func__);
		ath6kl_flush_pend_skb(vif);
	}

	if (ar->wiphy->flags & WIPHY_FLAG_SUPPORTS_FW_ROAM) {
		/*trigger roam, only work if firmware support*/
		memset(bssid, 0, ETH_ALEN);
		bssid[0] = 0xFF;
		ath6kl_wmi_force_roam_cmd(ar->wmi, (const u8 *)bssid);
	}
}

static void ath6kl_eapol_shprotect_timer_handler(unsigned long ptr)
{
	struct ath6kl *ar = (struct ath6kl *)ptr;

	ath6kl_dbg(ATH6KL_DBG_WLAN_CFG, "%s\n", __func__);
	clear_bit(EAPOL_HANDSHAKE_PROTECT, &ar->flag);
	ar->eapol_shprotect_vif = 0;

	return;
}

#if defined(CONFIG_ANDROID) || defined(USB_AUTO_SUSPEND)
int ath6kl_android_enable_wow_default(struct ath6kl *ar)
{
	unsigned char mask = 0x3F;
	int mask_len;
	int rv = 0, i;
	struct cfg80211_wowlan wow;

	if (test_bit(TESTMODE_EPPING, &ar->flag))
		return 0;

	memset(&wow, 0, sizeof(wow));

	/*default filters : ANY + all self MACs*/
	wow.any = true;
	wow.patterns = kcalloc(ar->vif_max,
			sizeof(wow.patterns[0]), GFP_KERNEL);
	if (!wow.patterns)
		return -ENOMEM;

	mask_len = DIV_ROUND_UP(ETH_ALEN, 8);
	for (i = 0; i < ar->vif_max; i++) {

		wow.patterns[i].mask = kmalloc(mask_len + ETH_ALEN, GFP_KERNEL);
		if (!wow.patterns[i].mask) {
			rv = -ENOMEM;
			goto failed;
		}
		wow.patterns[i].pattern = wow.patterns[i].mask + mask_len;
		memcpy(wow.patterns[i].mask, &mask, mask_len);
		wow.patterns[i].pattern_len = ETH_ALEN;
		memcpy(wow.patterns[i].pattern, ar->mac_addr, ETH_ALEN);
		if (i != 0)
			wow.patterns[i].pattern[0] =
			(wow.patterns[i].pattern[0] ^ (1 << i)) | 0x2;

		wow.n_patterns++;

		ath6kl_dbg(ATH6KL_DBG_WOWLAN,
			"Add wow pattern:%x:%x:%x:%x:%x:%x\n",
			wow.patterns[i].pattern[0], wow.patterns[i].pattern[1],
			wow.patterns[i].pattern[2], wow.patterns[i].pattern[3],
			wow.patterns[i].pattern[4], wow.patterns[i].pattern[5]);

	}

	/*Set wow mode to target firmware*/
	rv = ath6kl_set_wow_mode(ar->wiphy, &wow);
failed:
	for (i = 0; i < ar->vif_max; i++)
		kfree(wow.patterns[i].mask);

	kfree(wow.patterns);

	return rv;
}

bool ath6kl_android_need_wow_suspend(struct ath6kl *ar)
{
	struct ath6kl_vif *vif = NULL;
	int i;
	bool isConnected = false;

	vif = ath6kl_vif_first(ar);
	if (!vif)
		return false;

	if (!test_bit(WLAN_WOW_ENABLE, &vif->flags))
		return false;

#ifdef ATH6KL_SUPPORT_WIFI_DISC
	/* Always allow WoW suspend in discovery mode */
	if (ar->disc_active)
		return true;
#endif

	/*If p2p-GO or softAP interface, we don't do Wow suspend.
	  *Otherwise, if one of the interfaces is connected, we do WoW
	  *to save power.
	  */
	for (i = 0; i < ar->vif_max; i++) {
		vif = ath6kl_get_vif_by_index(ar, i);
		if (vif) {
			/*if p2p-GO or softAP interface, don't do wow*/
			if (vif->nw_type == AP_NETWORK)
				return false;
			else if (test_bit(CONNECTED, &vif->flags))
				isConnected = true;
		}
	}

	return isConnected;
}
#endif

#ifdef ATH6KL_SUPPORT_WLAN_HB
int ath6kl_enable_wow_hb(struct ath6kl *ar)
{
	u32 filter = 0;
	int i, ret;
	struct ath6kl_vif *vif;

	vif = ath6kl_vif_first(ar);
	if (!vif)
		return -EIO;

	if (!ath6kl_cfg80211_ready(vif))
		return -EIO;

	/* Clear existing WOW patterns */
	for (i = 0; i < WOW_MAX_FILTERS_PER_LIST; i++)
		ath6kl_wmi_del_wow_pattern_cmd(ar->wmi, vif->fw_vif_idx,
				WOW_LIST_ID, i);

	filter |= WOW_FILTER_OPTION_MAGIC_PACKET |
		WOW_FILTER_OPTION_NWK_DISASSOC;

	/*Do GTK offload in WPA/WPA2 auth mode connection.*/
	if (vif->auth_mode == WPA2_AUTH_CCKM ||
			vif->auth_mode == WPA2_PSK_AUTH ||
			vif->auth_mode == WPA_AUTH_CCKM ||
			vif->auth_mode == WPA_PSK_AUTH) {
		filter |= WOW_FILTER_OPTION_OFFLOAD_GTK;
	}

	ret = ath6kl_wmi_set_wow_mode_cmd(ar->wmi, vif->fw_vif_idx,
					ATH6KL_WOW_MODE_ENABLE,
					filter,
					WOW_HOST_REQ_DELAY);

	return ret;
}
#endif
