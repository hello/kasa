/*
 * Copyright (c) 2010-2011 Atheros Communications Inc.
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

#include "testmode.h"

#include <net/netlink.h>

#include "core.h"

/*
 * netlink.h remove these macros from kernel 3.5.
 * TODO : Error handle for nla_put_XXX calls.
 */
#ifndef NLA_PUT
#define NLA_PUT_U32	nla_put_u32
#define NLA_PUT		nla_put
#else
#define _NLA_PUT_ERR_RTN
#endif

enum ath6kl_tm_attr {
	__ATH6KL_TM_ATTR_INVALID	= 0,
	ATH6KL_TM_ATTR_CMD		= 1,
	ATH6KL_TM_ATTR_DATA		= 2,
	ATH6KL_TM_ATTR_TYPE		= 3,

	/* keep last */
	__ATH6KL_TM_ATTR_AFTER_LAST,
	ATH6KL_TM_ATTR_MAX		= __ATH6KL_TM_ATTR_AFTER_LAST - 1,
};

enum ath6kl_tm_cmd {
	ATH6KL_TM_CMD_TCMD         = 0,
	ATH6KL_TM_CMD_WLAN_HB      = 1,
	ATH6KL_TM_CMD_WIFI_DISC    = 2,
	ATH6KL_TM_CMD_WIFI_KTK     = 3,
	ATH6KL_TM_CMD_DFS_SKIP     = 4,
	ATH6KL_TM_CMD_WLAN_SWOW    = 5,
    ATH6KL_TM_CMD_WIFI_DEBUGFS = 6, 
};

#define ATH6KL_TM_DATA_MAX_LEN		5000

static const struct nla_policy ath6kl_tm_policy[ATH6KL_TM_ATTR_MAX + 1] = {
	[ATH6KL_TM_ATTR_CMD]		= { .type = NLA_U32 },
	[ATH6KL_TM_ATTR_DATA]		= { .type = NLA_BINARY,
					    .len = ATH6KL_TM_DATA_MAX_LEN },
};

#ifdef CONFIG_NL80211_TESTMODE

void ath6kl_tm_rx_report_event(struct ath6kl *ar, void *buf, size_t buf_len)
{
	if (down_interruptible(&ar->sem))
		return;

	kfree(ar->tm.rx_report);

	ar->tm.rx_report = kmemdup(buf, buf_len, GFP_KERNEL);
	ar->tm.rx_report_len = buf_len;

	up(&ar->sem);

	wake_up(&ar->event_wq);
}

void ath6kl_tm_rx_event(struct ath6kl *ar, void *buf, size_t buf_len)
{
	struct sk_buff *skb;

	if (!buf || buf_len == 0) {
		printk(KERN_ERR "buf buflen is empty\n");
		return;
	}

	skb = cfg80211_testmode_alloc_event_skb(ar->wiphy, buf_len, GFP_ATOMIC);

	if (!skb) {
		printk(KERN_ERR "failed to allocate testmode rx skb!\n");
		return;
	}

	NLA_PUT_U32(skb, ATH6KL_TM_ATTR_CMD, ATH6KL_TM_CMD_TCMD);
	NLA_PUT(skb, ATH6KL_TM_ATTR_DATA, buf_len, buf);
	cfg80211_testmode_event(skb, GFP_ATOMIC);
	return;

#ifdef _NLA_PUT_ERR_RTN
nla_put_failure:
	kfree_skb(skb);
	printk(KERN_ERR "nla_put failed on testmode rx skb!\n");
#endif

}

#ifdef ATH6KL_SUPPORT_WLAN_HB
void ath6kl_wlan_hb_event(struct ath6kl *ar, u8 value,
			void *buf, size_t buf_len)
{
	struct sk_buff *skb;

	if (!buf || buf_len == 0) {
		printk(KERN_ERR "buf buflen is empty\n");
		return;
	}

	skb = cfg80211_testmode_alloc_event_skb(ar->wiphy, buf_len, GFP_ATOMIC);

	if (!skb) {
		printk(KERN_ERR "failed to allocate testmode event skb!\n");
		return;
	}

	NLA_PUT_U32(skb, ATH6KL_TM_ATTR_CMD, ATH6KL_TM_CMD_WLAN_HB);
	NLA_PUT_U32(skb, ATH6KL_TM_ATTR_TYPE, value);
	NLA_PUT(skb, ATH6KL_TM_ATTR_DATA, buf_len, buf);
	cfg80211_testmode_event(skb, GFP_ATOMIC);
	return;

#ifdef _NLA_PUT_ERR_RTN
nla_put_failure:
	kfree_skb(skb);
	printk(KERN_ERR "nla_put failed on testmode event skb!\n");
#endif
}
#endif

#ifdef ATH6KL_SUPPORT_WIFI_DISC
void ath6kl_tm_disc_event(struct ath6kl *ar, void *buf, size_t buf_len)
{
	struct sk_buff *skb;

	if (!buf || buf_len == 0) {
		printk(KERN_ERR "buf buflen is empty\n");
		return;
	}

	skb = cfg80211_testmode_alloc_event_skb(ar->wiphy, buf_len, GFP_ATOMIC);

	if (!skb) {
		printk(KERN_ERR "failed to allocate testmode event skb!\n");
		return;
	}

	NLA_PUT_U32(skb, ATH6KL_TM_ATTR_CMD, ATH6KL_TM_CMD_WIFI_DISC);
	NLA_PUT(skb, ATH6KL_TM_ATTR_DATA, buf_len, buf);
	cfg80211_testmode_event(skb, GFP_ATOMIC);
	return;

#ifdef _NLA_PUT_ERR_RTN
nla_put_failure:
	kfree_skb(skb);
	printk(KERN_ERR "nla_put failed on testmode event skb!\n");
#endif
}
#endif

static int ath6kl_tm_rx_report(struct ath6kl *ar, void *buf, size_t buf_len,
			       struct sk_buff *skb)
{
	int ret = 0;
	long left;

	if (!test_bit(WMI_READY, &ar->flag)) {
		ret = -EIO;
		goto out;
	}

	if (test_bit(DESTROY_IN_PROGRESS, &ar->flag)) {
		ret = -EBUSY;
		goto out;
	}
	if (down_interruptible(&ar->sem))
		return -EIO;

	if (ath6kl_wmi_test_cmd(ar->wmi, buf, buf_len) < 0) {
		up(&ar->sem);
		return -EIO;
	}

	left = wait_event_interruptible_timeout(ar->event_wq,
						ar->tm.rx_report != NULL,
						WMI_TIMEOUT);



	if (left == 0) {
		ret = -ETIMEDOUT;
		goto out;
	} else if (left < 0) {
		ret = left;
		goto out;
	}

	if (ar->tm.rx_report == NULL || ar->tm.rx_report_len == 0) {
		ret = -EINVAL;
		goto out;
	}

	NLA_PUT(skb, ATH6KL_TM_ATTR_DATA, ar->tm.rx_report_len,
		ar->tm.rx_report);

	kfree(ar->tm.rx_report);
	ar->tm.rx_report = NULL;

out:
	up(&ar->sem);

	return ret;

#ifdef _NLA_PUT_ERR_RTN
nla_put_failure:
	ret = -ENOBUFS;
	goto out;
#endif
}
EXPORT_SYMBOL(ath6kl_tm_rx_report);

#ifdef ATH6KL_SUPPORT_WLAN_HB
enum nl80211_wlan_hb_cmd {
	NL80211_WLAN_HB_ENABLE		= 0,
	NL80211_WLAN_TCP_PARAMS		= 1,
	NL80211_WLAN_TCP_FILTER		= 2,
	NL80211_WLAN_UDP_PARAMS		= 3,
	NL80211_WLAN_UDP_FILTER		= 4,
};

#define WLAN_HB_UDP		0x1
#define WLAN_HB_TCP		0x2

struct wlan_hb_params {
	u16 cmd;
	u16 dummy;

	union {
		struct {
			u8 enable;
			u8 item;
			u8 session;
		} hb_params;

		struct {
			u32 srv_ip;
			u32 dev_ip;
			u16 src_port;
			u16 dst_port;
			u16 timeout;
			u8 session;
			u8 gateway_mac[ETH_ALEN];
		} tcp_params;

		struct {
			u16 length;
			u8 offset;
			u8 session;
			u8 filter[64];
		} tcp_filter;

		struct {
			u32 srv_ip;
			u32 dev_ip;
			u16 src_port;
			u16 dst_port;
			u16 interval;
			u16 timeout;
			u8 session;
			u8 gateway_mac[ETH_ALEN];
		} udp_params;

		struct {
			u16 length;
			u8 offset;
			u8 session;
			u8 filter[64];
		} udp_filter;

	} params;
};
#endif

#ifdef ATH6KL_SUPPORT_WIFI_DISC
#define WLAN_WIFI_DISC_MAX_IE_SIZE	200

enum nl80211_wifi_disc_cmd {
	NL80211_WIFI_DISC_IE		= 0,
	NL80211_WIFI_DISC_IE_FILTER	= 1,
	NL80211_WIFI_DISC_START		= 2,
	NL80211_WIFI_DISC_STOP		= 3,
};

struct wifi_disc_params {
	u16 cmd;

	union {
		struct {
			u16 length;
			u8 ie[WLAN_WIFI_DISC_MAX_IE_SIZE];
		} ie_params;

		struct {
			u16 enable;
			u16 startPos;
			u16 length;
			u8 filter[WLAN_WIFI_DISC_MAX_IE_SIZE];
		} ie_filter_params;

		struct {
			u16 channel;
			u16 dwellTime;
			u16 sleepTime;
			u16 random;
			u16 numPeers;
			u16 peerTimeout;
			u16 txPower;
		} start_params;
	} params;
};
#endif

#ifdef ATH6KL_SUPPORT_WIFI_KTK
#define WLAN_WIFI_KTK_MAX_IE_SIZE	200

enum nl80211_wifi_ktk_cmd {
	NL80211_WIFI_KTK_IE         = 0,
	NL80211_WIFI_KTK_IE_FILTER  = 1,
	NL80211_WIFI_KTK_START      = 2,
	NL80211_WIFI_KTK_STOP       = 3,
};

struct wifi_ktk_params {
	u16 cmd;
	union {
		struct {
			u16 length;
			u8 ie[WLAN_WIFI_KTK_MAX_IE_SIZE];
		} ie_params;

		struct {
			u16 enable;
			u16 startPos;
			u16 length;
			u8 filter[WLAN_WIFI_KTK_MAX_IE_SIZE];
		} ie_filter_params;

		struct {
			u16 ssid_len;
			u8 ssid[32];
			u8 passphrase[16];
		} start_params;
	} params;
};
#endif

struct wifi_dfs_skip_params {
	u16 enable;
};

#ifdef CONFIG_SWOW_SUPPORT
enum nl80211_wlan_swow_cmd {
	NL80211_WLAN_SWOW_SETMODE       = 0,
	NL80211_WLAN_SWOW_STOP          = 1,
	NL80211_WLAN_SWOW_SET_PWD       = 2,
	NL80211_WLAN_SWOW_MODE_GET      = 3,
	NL80211_WLAN_SWOW_PWD_GET       = 4,	
	
};
enum swow_mode {
	SWOW_INDOOR_MODE  = 0,  
	SWOW_OUTDOOR_MODE = 1,
};

struct wlan_swow_params {
	u16 cmd;
	u16 mode;

	union {
		struct {
			u16 length;
			u8 password[32];
		} pwd_info;
	} params;
};
extern int ath6kl_usb_suspend(struct ath6kl *ar, struct cfg80211_wowlan *wow);
extern int ath6kl_usb_resume(struct ath6kl *ar);

void swow_set_suspend(struct ath6kl_vif *vif, int enable)
{
	if (vif) {
		if (down_interruptible(&vif->ar->sem))
			return;
		printk("%s[%d] enable=%d, current=%d\n",
			__func__, __LINE__, enable, vif->last_suspend_wow);
		if (enable == 1 && vif->last_suspend_wow == 0) { 
		  /* enetr suspend mode */
		  struct cfg80211_wowlan wow_setting;
		  memset(&wow_setting, 0, sizeof(struct cfg80211_wowlan));
		  wow_setting.disconnect = 1;
			ath6kl_usb_suspend(vif->ar, &wow_setting);
			vif->last_suspend_wow = 1;
		}
		if (enable == 0 && vif->last_suspend_wow == 1) { 
		  /* leave suspend mode */
			ath6kl_usb_resume(vif->ar);
			vif->last_suspend_wow = 0;
		}
		up(&vif->ar->sem);
	}
}
#endif //swow

#if defined(CE_2_SUPPORT)
extern unsigned int debug_mask;
extern unsigned int debug_mask_ext;

struct wlan_debugfs_params {
	u16 cmd;
	union {
		struct {
			u8 ratio;
		} tx_power_params;
        
        struct {
            u8 skip;
        } passive_ch_skip_parames;
        
        struct {
            u16 act_dwell_t;
            u16 pas_dwell_t;
            u16 per_ssid;
        } scan_set_params;

        struct {
            u32 mask;
            u32 mask_ext;
        } debug_mask_params;

        struct {
            u32 internal;
            u32 cycle;
        } ap_alive_params;

        struct {
            u8 band;
            u8 ht40_supported;
            u8 short_GI;
            u8 intolerance_ht40;
        } ht_cap_params;

        struct {
            u32 enable_ampdu;
        } ampdu_parames;

        struct {
            u32 reg_addr;
            u32 reg_value;
        } reg_params;

        struct {
            u32 retry_limit;
        } direct_audio_parames;
	} params;
};

struct wlan_debugfs_read {
	u16 cmd;
    union {
        struct {
            u64 tsf_value;
        } tsf_read_params;

        struct {
            u32 mask;
            u32 mask_ext;
        } debug_mask_params;

        struct {
            u32 reg_addr;
            u32 reg_value;
        } reg_params;
	} params;
};

enum nl80211_wlan_debugfs_cmds {
	DEBUGFS_TX_POWER             = 0,
    DEBUGFS_PASSIVE_CHANNEL_SKIP = 1,
    DEBUGFS_GET_TSF              = 2,
    DEBUGFS_SYNC_TSF             = 3,
    DEBUGFS_GET_SCAN_PARAMS      = 4,
    DEBUGFS_SET_SCAN_PARAMS      = 5,
    DEBUGFS_GET_DEBUG_MASK       = 6,
    DEBUGFS_SET_DEBUG_MASK       = 7,
    DEBUGFS_GET_AP_ALIVE_PARAMS  = 8,
    DEBUGFS_SET_AP_ALIVE_PARAMS  = 9,
    DEBUGFS_GET_HT_CAP_PARAMS    = 10,
    DEBUGFS_SET_HT_CAP_PARAMS    = 11,
    DEBUGFS_CHAN_LIST            = 12,
    DEBUGFS_AMPDU                = 13,
    DEBUGFS_REG_READ             = 14,
    DEBUGFS_REG_WRITE            = 15,
    DEBUGFS_DA_RETRY_LIMIT       = 16,
};

int debugfs_tx_power(struct ath6kl_vif *vif, u8 ratio)
{
    struct ath6kl *ar;
    
    if (vif)
        ar = vif->ar;
    else
        return -EIO;

	if (test_bit(CONNECTED, &vif->flags)) {
		ar->tx_pwr = 0;

		if (ath6kl_wmi_get_tx_pwr_cmd(ar->wmi, vif->fw_vif_idx) != 0) {
			printk(KERN_ERR "ath6kl_wmi_get_tx_pwr_cmd failed\n");
			return -EIO;
		}

		wait_event_interruptible_timeout(ar->event_wq, ar->tx_pwr != 0,
						 5 * HZ);

		if (signal_pending(current)) {
			printk(KERN_ERR "target did not respond\n");
			return -EINTR;
		}

		if(ar->tx_pwr > ar->max_tx_pwr)
			ar->max_tx_pwr = ar->tx_pwr;

        ath6kl_wmi_set_tx_pwr_cmd(ar->wmi, vif->fw_vif_idx, ratio*ar->max_tx_pwr/100);
	}
    return 0;
}

void debugfs_passive_channel_skip(struct ath6kl_vif *vif, u8 skip)
{
    int i;
    struct ath6kl *ar;

    if (vif)
        ar = vif->ar;
    else
        return;

    for (i = 0; i < ar->vif_max; i++) {
		vif = ath6kl_get_vif_by_index(ar, i);
        if (vif) {
		    if (skip == 1)
        	    vif->sc_params.pas_chdwell_time = 3;
            else
        	    vif->sc_params.pas_chdwell_time = 105;
        }
	}
}

void debugfs_show_scan_params(struct ath6kl *ar)
{
    int i;
    struct ath6kl_vif *vif;

     for (i = 0; i < ar->vif_max; i++) {
		vif = ath6kl_get_vif_by_index(ar, i);
		if (vif) {
			printk("DEV-%d Flags = 0x%x ActDwellTime = %d PasDwellTime = %d ScanPerSsid = %d\n",
				i,
				vif->sc_params.scan_ctrl_flags,
				(vif->sc_params.maxact_chdwell_time == 0 ?
					ATH6KL_SCAN_ACT_DEWELL_TIME :
					vif->sc_params.maxact_chdwell_time),
				(vif->sc_params.pas_chdwell_time == 0 ?
					ATH6KL_SCAN_PAS_DEWELL_TIME :
					vif->sc_params.pas_chdwell_time),
				(vif->sc_params.maxact_scan_per_ssid == 0 ?
					ATH6KL_SCAN_PROBE_PER_SSID :
					vif->sc_params.maxact_scan_per_ssid));
		}
	}
}

void debugfs_set_scan_params(struct ath6kl *ar, u16 act_dwell_t,
                             u16 pas_dwell_t, u16 per_ssid)
{
	int i;
    struct ath6kl_vif *vif;
    
    if (act_dwell_t < ATH6KL_SCAN_ACT_DEWELL_TIME)
		act_dwell_t = ATH6KL_SCAN_ACT_DEWELL_TIME;

	if (pas_dwell_t < ATH6KL_SCAN_PAS_DEWELL_TIME)
		pas_dwell_t = ATH6KL_SCAN_PAS_DEWELL_TIME;

	if (per_ssid <= ATH6KL_SCAN_PROBE_PER_SSID)
		per_ssid = ATH6KL_SCAN_PROBE_PER_SSID;

    for (i = 0; i < ar->vif_max; i++) {
		vif = ath6kl_get_vif_by_index(ar, i);
        if (vif) {
	        vif->sc_params.maxact_chdwell_time = act_dwell_t;
	        vif->sc_params.pas_chdwell_time = pas_dwell_t;
	        vif->sc_params.maxact_scan_per_ssid = per_ssid;
	        memcpy(&vif->sc_params_default, &vif->sc_params,
		    	 sizeof(struct wmi_scan_params_cmd));
        }
    }
             
}

void debugfs_show_ap_alive_params(struct ath6kl *ar)
{
	int i;
	struct ath6kl_vif *vif;
    
	for (i = 0; i < ar->vif_max; i++) {
		vif = ath6kl_get_vif_by_index(ar, i);
		if ((vif) &&
		    (vif->ap_keepalive_ctx)) {
			if (vif->ap_keepalive_ctx->flags &
				ATH6KL_AP_KA_FLAGS_BY_SUPP)
				printk("int-%d offload to supplicant/hostapd\n", i);
			else
				printk("int-%d flags %x, interval %d ms., cycle %d\n",
				i,
				vif->ap_keepalive_ctx->flags,
				vif->ap_keepalive_ctx->ap_ka_interval,
				vif->ap_keepalive_ctx->ap_ka_reclaim_cycle);
		}
	}
}

void debugfs_set_ap_alive_params(struct ath6kl *ar, u32 internal, u32 cycle)
{
	int i;
	struct ath6kl_vif *vif;

	for (i = 0; i < ar->vif_max; i++) {
		vif = ath6kl_get_vif_by_index(ar, i);
		if (vif)
			ath6kl_ap_keepalive_config(vif,
						   (u32)internal,
						   (u32)cycle);
	}
}

void debugfs_show_ht_cap_params(struct ath6kl *ar)
{
    int band;
    
	for (band = 0; band < A_NUM_BANDS; band++) {
		if (ar->debug.ht_cap_param[band].isConfig)
			printk("%s chan_width_40M_supported: %d, short_GI: %d, intolerance_40MHz: %d\n",
			(ar->debug.ht_cap_param[band].band == A_BAND_24GHZ ?
			"24GHZ" : "5GHZ"),
			ar->debug.ht_cap_param[band].chan_width_40M_supported,
			ar->debug.ht_cap_param[band].short_GI,
			ar->debug.ht_cap_param[band].intolerance_40MHz);
	}

}

void debugfs_set_ht_cap_params(struct ath6kl *ar,
                               u8 band, u8 ht40_supported,
                               u8 short_GI, u8 intolerance_ht40)
{
    struct ht_cap_param tempHtCap;
	int i;

	memset(&tempHtCap, 0, sizeof(struct ht_cap_param));
    tempHtCap.band = band;
    tempHtCap.chan_width_40M_supported = ht40_supported;
    tempHtCap.short_GI = short_GI;
    tempHtCap.intolerance_40MHz = intolerance_ht40;

	for (i = 0; i < ar->vif_max; i++) {
		if (ath6kl_wmi_set_ht_cap_cmd(ar->wmi, i,
			tempHtCap.band,
			tempHtCap.chan_width_40M_supported,
			tempHtCap.short_GI,
			tempHtCap.intolerance_40MHz))
			return;
	}

	tempHtCap.isConfig = 1;
	memcpy(&ar->debug.ht_cap_param[tempHtCap.band],
		&tempHtCap, sizeof(struct ht_cap_param));

}

void debugfs_show_chan_list(struct ath6kl *ar)
{
    struct wiphy *wiphy = ar->wiphy;
	struct reg_info *reg = ar->reg_ctx;
	enum ieee80211_band band;
	int i;
	u8 flag_string[96];

	if (reg->current_regd) {
		printk("\nCurrent Regulatory - %08x %c%c %d rules\n",
				reg->current_reg_code,
				reg->current_regd->alpha2[0],
				reg->current_regd->alpha2[1],
				reg->current_regd->n_reg_rules);

		for (i = 0; i < reg->current_regd->n_reg_rules; i++) {
			struct ieee80211_reg_rule *regRule;

			regRule = &reg->current_regd->reg_rules[i];
			printk("\t[%d - %d, HT%d]\n",
				regRule->freq_range.start_freq_khz / 1000,
				regRule->freq_range.end_freq_khz / 1000,
				regRule->freq_range.max_bandwidth_khz / 1000);
		}
	}

	for (band = 0; band < IEEE80211_NUM_BANDS; band++) {
		if (!wiphy->bands[band])
			continue;

		printk("\n%s\n", (band == IEEE80211_BAND_2GHZ ?
				"2.4GHz" : "5GHz"));

		for (i = 0; i < wiphy->bands[band]->n_channels; i++) {
			struct ieee80211_channel *chan;

			chan = &wiphy->bands[band]->channels[i];

			if (chan->flags & IEEE80211_CHAN_DISABLED)
				continue;

            __chan_flag_to_string(chan->flags, flag_string, 96);
			printk(" CH%4d - %4d %s\n",
					chan->hw_value,
					chan->center_freq,
					flag_string);
		}
	}

	printk("\nCurrent operation channel\n");

	for (i = 0; i < ar->vif_max; i++) {
		struct ath6kl_vif *vif = ath6kl_get_vif_by_index(ar, i);

		if (vif)
			printk(" VIF%d [%s] - ch %d phy %d type %d\n",
					i,
					(test_bit(CONNECTED, &vif->flags) ?
						"CONN" : "IDLE"),
					vif->bss_ch,
					vif->phymode,
					vif->chan_type);
	}
}

void debugfs_ampdu_enable(struct ath6kl *ar, u32 enable_ampdu)
{
	int i;
	struct ath6kl_vif *vif;

    for (i = 0; i < ar->vif_max; i++) {
		vif = ath6kl_get_vif_by_index(ar, i);
		/* only support 8 TIDs now. */
		if (vif)
			ath6kl_wmi_allow_aggr_cmd(vif->ar->wmi,
						vif->fw_vif_idx,
						(enable_ampdu ? 0xff : 0),
						(enable_ampdu ? 0xff : 0));
	}

}

#endif //CE_2_SUPPORT

int ath6kl_tm_cmd(struct wiphy *wiphy, void *data, int len)
{
	struct ath6kl *ar = wiphy_priv(wiphy);
	struct nlattr *tb[ATH6KL_TM_ATTR_MAX + 1];
	int err, buf_len;
	void *buf;

	err = nla_parse(tb, ATH6KL_TM_ATTR_MAX, data, len,
			ath6kl_tm_policy);

	if (err)
		return err;

	if (!tb[ATH6KL_TM_ATTR_CMD])
		return -EINVAL;

	switch (nla_get_u32(tb[ATH6KL_TM_ATTR_CMD])) {
	case ATH6KL_TM_CMD_TCMD:
		if (!tb[ATH6KL_TM_ATTR_DATA])
			return -EINVAL;

		buf = nla_data(tb[ATH6KL_TM_ATTR_DATA]);
		buf_len = nla_len(tb[ATH6KL_TM_ATTR_DATA]);

		ath6kl_wmi_test_cmd(ar->wmi, buf, buf_len);

		return 0;

		break;
#ifdef CONFIG_SWOW_SUPPORT
case ATH6KL_TM_CMD_WLAN_SWOW:
	{
		struct wlan_swow_params *swow_params;
		struct ath6kl_vif *vif;

		vif = ath6kl_vif_first(ar);

		if (!vif)
			return -EINVAL;
			
		if (!tb[ATH6KL_TM_ATTR_DATA]) {
			printk(KERN_ERR "%s: NO DATA\n", __func__);
			return -EINVAL;
		}

		buf = nla_data(tb[ATH6KL_TM_ATTR_DATA]);
		buf_len = nla_len(tb[ATH6KL_TM_ATTR_DATA]);
		
		swow_params = (struct wlan_swow_params *)buf;
		
		if (swow_params->cmd == NL80211_WLAN_SWOW_SETMODE) {
			printk("SWOW MODE %x \n",swow_params->mode);
			printk("SWOW Password Length %d\n",swow_params->params.pwd_info.length);

			if (swow_params->params.pwd_info.length){
				int ii=0;
				for (ii=0; ii < swow_params->params.pwd_info.length; ii++)
				    printk("PWD[%02d]: %02x\n",ii,swow_params->params.pwd_info.password[ii]);		    
			}
			if (swow_params->mode == SWOW_INDOOR_MODE) {
				/*Indoor*/
				ath6kl_wmi_set_swol_indoor_info_cmd(
		             ar->wmi, 1, 
		             swow_params->params.pwd_info.length, 
		             swow_params->params.pwd_info.password);
				usleep_range(10000, 10000);
				swow_set_suspend(vif, 1);/* enetr suspend mode */
			} else if (swow_params->mode == SWOW_OUTDOOR_MODE) {
				/*Outdoor*/
				//ath6kl_wmi_set_swol_outdoor_info_cmd(ar->wmi, 1,...);
			}
		} else if (swow_params->cmd == NL80211_WLAN_SWOW_STOP) {
			/*-1 : Fail (WoWLAN mode is not running)*/
			printk("STOP SWOW \n");
			/*Due to wmi would be disabled, need to wake up first*/
			swow_set_suspend(vif, 0);
			usleep_range(10000, 10000);
			ath6kl_wmi_set_swol_indoor_info_cmd(ar->wmi, 0, 0, NULL);		  
			//ath6kl_wmi_set_swol_outdoor_info_cmd(ar->wmi, 0, 0,...);
		} else if (swow_params->cmd == NL80211_WLAN_SWOW_SET_PWD){
			/*Password (0~16 B)*/
			printk("SWOW Password Length %d\n",swow_params->params.pwd_info.length);
		}
	}

	return 0;
	break;
#endif //swow
#ifdef ATH6KL_SUPPORT_WLAN_HB
	case ATH6KL_TM_CMD_WLAN_HB:
	{
		struct wlan_hb_params *hb_params;
		struct ath6kl_vif *vif;

		vif = ath6kl_vif_first(ar);

		if (!vif)
			return -EINVAL;

		if (!tb[ATH6KL_TM_ATTR_DATA]) {
			printk(KERN_ERR "%s: NO DATA\n", __func__);
			return -EINVAL;
		}

		buf = nla_data(tb[ATH6KL_TM_ATTR_DATA]);
		buf_len = nla_len(tb[ATH6KL_TM_ATTR_DATA]);

		hb_params = (struct wlan_hb_params *)buf;

		if (hb_params->cmd == NL80211_WLAN_HB_ENABLE) {
			if (hb_params->params.hb_params.enable != 0) {

				if (ath6kl_enable_wow_hb(ar)) {
					printk(KERN_ERR
					"%s: enable hb wow fail\n",
					__func__);
					return -EINVAL;
				}

				if (hb_params->params.hb_params.item
							 == WLAN_HB_TCP) {
					if (ath6kl_wmi_set_heart_beat_params(
						ar->wmi,
						vif->fw_vif_idx,
						1,
						WLAN_HB_TCP,
					hb_params->params.hb_params.session)) {
						printk(KERN_ERR
						"%s: set heart beat enable fail\n",
						__func__);
						return -EINVAL;
					}
				} else if (hb_params->params.hb_params.item
							 ==  WLAN_HB_UDP) {
					if (ath6kl_wmi_set_heart_beat_params(
						ar->wmi,
						vif->fw_vif_idx,
						1,
						WLAN_HB_UDP,
					hb_params->params.hb_params.session)) {
						printk(KERN_ERR
						"%s: set heart beat enable fail\n",
						__func__);
						return -EINVAL;
					}
				}
			} else {
#ifdef CONFIG_ANDROID
				if (ath6kl_android_enable_wow_default(ar)) {
					printk(KERN_ERR
					"%s: enable android defualt wow fail\n",
					__func__);
				}
#endif

				hb_params->params.hb_params.item = WLAN_HB_TCP;
				if (hb_params->params.hb_params.item
							 == WLAN_HB_TCP) {
					if (ath6kl_wmi_set_heart_beat_params(
						ar->wmi,
						vif->fw_vif_idx,
						0,
						WLAN_HB_TCP,
					hb_params->params.hb_params.session)) {
						printk(KERN_ERR
						"%s: set heart beat disable fail\n",
						__func__);
						return -EINVAL;
					}
				} else if (hb_params->params.hb_params.item
							 ==  WLAN_HB_UDP) {
					if (ath6kl_wmi_set_heart_beat_params(
						ar->wmi,
						vif->fw_vif_idx,
						0,
						WLAN_HB_UDP,
					hb_params->params.hb_params.session)) {
						printk(KERN_ERR
						"%s: set heart beat disable fail\n",
						__func__);
						return -EINVAL;
					}
				}
			}
		} else if (hb_params->cmd == NL80211_WLAN_TCP_PARAMS) {
			if (ath6kl_wmi_heart_beat_set_tcp_params(ar->wmi,
				vif->fw_vif_idx,
				hb_params->params.tcp_params.src_port,
				hb_params->params.tcp_params.dst_port,
				hb_params->params.tcp_params.srv_ip,
				hb_params->params.tcp_params.dev_ip,
				hb_params->params.tcp_params.timeout,
				hb_params->params.tcp_params.session,
				hb_params->params.tcp_params.gateway_mac)) {
				printk(KERN_ERR
				"%s: set heart beat tcp params fail\n",
				__func__);
				return -EINVAL;
			}

		} else if (hb_params->cmd == NL80211_WLAN_TCP_FILTER) {
			if (hb_params->params.tcp_filter.length
				> WMI_MAX_TCP_FILTER_SIZE) {
				printk(KERN_ERR "%s: size of tcp filter is too large: %d\n",
					__func__,
					hb_params->params.tcp_filter.length);
				return -E2BIG;
			}

			if (ath6kl_wmi_heart_beat_set_tcp_filter(ar->wmi,
				vif->fw_vif_idx,
				hb_params->params.tcp_filter.filter,
				hb_params->params.tcp_filter.length,
				hb_params->params.tcp_filter.offset,
				hb_params->params.tcp_filter.session)) {
				printk(KERN_ERR
				"%s: set heart beat tcp filter fail\n",
				__func__);
				return -EINVAL;
			}
		} else if (hb_params->cmd == NL80211_WLAN_UDP_PARAMS) {
			if (ath6kl_wmi_heart_beat_set_udp_params(ar->wmi,
				vif->fw_vif_idx,
				hb_params->params.udp_params.src_port,
				hb_params->params.udp_params.dst_port,
				hb_params->params.udp_params.srv_ip,
				hb_params->params.udp_params.dev_ip,
				hb_params->params.udp_params.interval,
				hb_params->params.udp_params.timeout,
				hb_params->params.udp_params.session,
				hb_params->params.udp_params.gateway_mac)) {
				printk(KERN_ERR
				"%s: set heart beat udp params fail\n",
				__func__);
				return -EINVAL;
			}
		} else if (hb_params->cmd == NL80211_WLAN_UDP_FILTER) {
			if (hb_params->params.udp_filter.length
					> WMI_MAX_UDP_FILTER_SIZE) {
				printk(KERN_ERR
				"%s: size of udp filter is too large: %d\n",
				__func__,
				hb_params->params.udp_filter.length);
				return -E2BIG;
			}

			if (ath6kl_wmi_heart_beat_set_udp_filter(ar->wmi,
					vif->fw_vif_idx,
					hb_params->params.udp_filter.filter,
					hb_params->params.udp_filter.length,
					hb_params->params.udp_filter.offset,
					hb_params->params.udp_filter.session)) {
				printk(KERN_ERR
				"%s: set heart beat udp filter fail\n",
				__func__);
				return -EINVAL;
			}
		}
	}

	return 0;
	break;
#endif
#ifdef ATH6KL_SUPPORT_WIFI_DISC
	case ATH6KL_TM_CMD_WIFI_DISC:
	{
		struct wifi_disc_params *disc_params;
		struct ath6kl_vif *vif;

		vif = ath6kl_vif_first(ar);

		if (!vif)
			return -EINVAL;

		if (!tb[ATH6KL_TM_ATTR_DATA]) {
			printk(KERN_ERR "%s: NO DATA\n", __func__);
			return -EINVAL;
		}

		buf = nla_data(tb[ATH6KL_TM_ATTR_DATA]);
		buf_len = nla_len(tb[ATH6KL_TM_ATTR_DATA]);

		disc_params = (struct wifi_disc_params *)buf;

		if (disc_params->cmd == NL80211_WIFI_DISC_IE) {
			u8 ie_hdr[6] = {0xDD, 0x00, 0x00, 0x03, 0x7f, 0x00};
			u8 *ie = NULL;
			u16 ie_length = disc_params->params.ie_params.length;

			ie = kmalloc(ie_length+6, GFP_KERNEL);
			if (ie == NULL)
				return -ENOMEM;

			memcpy(ie, ie_hdr, 6);
			ie[1] = ie_length+4;
			memcpy(ie+6,
				disc_params->params.ie_params.ie,
				ie_length);

			if (ath6kl_wmi_set_appie_cmd(ar->wmi, vif->fw_vif_idx,
				WMI_FRAME_PROBE_REQ, ie, ie_length+6)) {
				kfree(ie);
				printk(KERN_ERR
				"%s: wifi discovery set probe request ie fail\n",
				__func__);
				return -EINVAL;
			}

			if (ath6kl_wmi_set_appie_cmd(ar->wmi, vif->fw_vif_idx,
				WMI_FRAME_PROBE_RESP, ie, ie_length+6)) {
				kfree(ie);
				printk(KERN_ERR
				"%s: wifi discovery set probe response ie fail\n",
				__func__);
				return -EINVAL;
			}

			kfree(ie);
		} else if (disc_params->cmd == NL80211_WIFI_DISC_IE_FILTER) {
			if (ath6kl_wmi_disc_ie_cmd(ar->wmi, vif->fw_vif_idx,
				disc_params->params.ie_filter_params.enable,
				disc_params->params.ie_filter_params.startPos,
				disc_params->params.ie_filter_params.filter,
				disc_params->params.ie_filter_params.length)) {
				printk(KERN_ERR
				"%s: wifi discovery set ie filter fail\n",
				__func__);
				return -EINVAL;
			}
		} else if (disc_params->cmd == NL80211_WIFI_DISC_START) {
			int band, freq, numPeers, random;

			if (disc_params->params.start_params.channel <= 14)
				band = IEEE80211_BAND_2GHZ;
			else
				band = IEEE80211_BAND_5GHZ;

			freq = ieee80211_channel_to_frequency(
				disc_params->params.start_params.channel, band);
			if (!freq) {
				printk(KERN_ERR "%s: wifi discovery start channel %d error\n",
				__func__,
				disc_params->params.start_params.channel);
				return -EINVAL;
			}

			if (disc_params->params.start_params.numPeers == 0)
				numPeers = 1;
			else if (disc_params->params.start_params.numPeers > 4)
				numPeers = 4;
			else
				numPeers =
				disc_params->params.start_params.numPeers;

			random = (disc_params->params.start_params.random == 0)
				? 100 : disc_params->params.start_params.random;

			if (disc_params->params.start_params.txPower)
				ath6kl_wmi_set_tx_pwr_cmd(ar->wmi,
				vif->fw_vif_idx,
				disc_params->params.start_params.txPower);

			/* disable scanning */
			ath6kl_wmi_scanparams_cmd(ar->wmi, vif->fw_vif_idx,
						0xFFFF, 0, 0,
						0, 0, 0, 0, 0, 0, 0);

			if (ath6kl_wmi_disc_mode_cmd(ar->wmi,
				vif->fw_vif_idx, 1, freq,
				disc_params->params.start_params.dwellTime,
				disc_params->params.start_params.sleepTime,
				random, numPeers,
				disc_params->params.start_params.peerTimeout
				)) {
				printk(KERN_ERR "%s: wifi discovery start fail\n",
						__func__);
				return -EINVAL;
			}

			/* change disc state to active */
			ar->disc_active = true;
		} else if (disc_params->cmd == NL80211_WIFI_DISC_STOP) {
			/* change disc state to inactive */
			ar->disc_active = false;
			if (ath6kl_wmi_disc_mode_cmd(ar->wmi, vif->fw_vif_idx,
							0, 0, 0, 0, 0, 0, 0)) {
				printk(KERN_ERR "%s: wifi discovery stop fail\n",
						__func__);
				return -EINVAL;
			}
		}
	}

	return 0;
	break;
#endif

#ifdef ATH6KL_SUPPORT_WIFI_KTK
	case ATH6KL_TM_CMD_WIFI_KTK:
	{
		struct wifi_ktk_params *ktk_params;
		struct ath6kl_vif *vif;

		vif = ath6kl_vif_first(ar);
		if (!vif)
			return -EINVAL;

		if (!tb[ATH6KL_TM_ATTR_DATA]) {
			printk(KERN_ERR "%s: NO DATA\n", __func__);
			return -EINVAL;
		}

		buf = nla_data(tb[ATH6KL_TM_ATTR_DATA]);
		buf_len = nla_len(tb[ATH6KL_TM_ATTR_DATA]);

		ktk_params = (struct wifi_ktk_params *)buf;

		if (ktk_params->cmd == NL80211_WIFI_KTK_IE) {
			u8 ie_hdr[6] = {0xDD, 0x00, 0x00, 0x03, 0x7f, 0x00};
			u8 *ie = NULL;
			u16 ie_length = ktk_params->params.ie_params.length;

			ie = kmalloc(ie_length+6, GFP_KERNEL);
			if (ie == NULL)
				return -ENOMEM;

			memcpy(ie, ie_hdr, 6);
			ie[1] = ie_length+4;
			memcpy(ie+6, ktk_params->params.ie_params.ie,
				ie_length);

			if (ath6kl_wmi_set_appie_cmd(ar->wmi, vif->fw_vif_idx,
							WMI_FRAME_PROBE_RESP,
							ie,
							ie_length+6)) {
				kfree(ie);
				printk(KERN_ERR
					"%s: wifi ktk set probe	response ie fail\n",
					__func__);
				return -EINVAL;
			}

			if (ath6kl_wmi_set_appie_cmd(ar->wmi, vif->fw_vif_idx,
					WMI_FRAME_BEACON, ie, ie_length+6)) {
				kfree(ie);
				printk(KERN_ERR
					"%s: wifi ktk set beacon ie fail\n",
					__func__);
				return -EINVAL;
			}

			kfree(ie);
		} else if (ktk_params->cmd == NL80211_WIFI_KTK_IE_FILTER) {
			if (ath6kl_wmi_disc_ie_cmd(ar->wmi, vif->fw_vif_idx,
				ktk_params->params.ie_filter_params.enable,
				ktk_params->params.ie_filter_params.startPos,
				ktk_params->params.ie_filter_params.filter,
				ktk_params->params.ie_filter_params.length)) {
				printk(KERN_ERR
					"%s: wifi ktk set ie filter fail\n",
					__func__);
				return -EINVAL;
			}
		} else if (ktk_params->cmd == NL80211_WIFI_KTK_START) {
			ar->ktk_active = true;

			/* Clear the legacy ie pattern and filter */
			if (ath6kl_wmi_disc_ie_cmd(ar->wmi, vif->fw_vif_idx,
					0,
					0,
					NULL,
					0)) {
				printk(KERN_ERR "%s: wifi ktk clear ie filter fail\n",
					__func__);
				return -EINVAL;
			}

			memcpy(ar->ktk_passphrase,
				ktk_params->params.start_params.passphrase,
				16);

			if (ath6kl_wmi_probedssid_cmd(ar->wmi, vif->fw_vif_idx,
				1, SPECIFIC_SSID_FLAG,
				ktk_params->params.start_params.ssid_len,
				ktk_params->params.start_params.ssid)) {
				printk(KERN_ERR "%s: wifi ktk set probedssid fail\n",
					__func__);
				return -EINVAL;
			}

			if (ath6kl_wmi_ibss_pm_caps_cmd(ar->wmi,
					vif->fw_vif_idx,
					ADHOC_PS_KTK,
					5,
					10,
					10)) {
				printk(KERN_ERR "%s: wifi ktk set power save mode on fail\n",
					__func__);
				return -EINVAL;
			}
		} else if (ktk_params->cmd == NL80211_WIFI_KTK_STOP) {
			ar->ktk_active = false;

			if (ath6kl_wmi_ibss_pm_caps_cmd(ar->wmi,
					vif->fw_vif_idx,
					ADHOC_PS_DISABLE,
					0,
					0,
					0)) {
				printk(KERN_ERR "%s: wifi ktk set power save mode off fail\n",
					__func__);
				return -EINVAL;
			}
		}
	}

	return 0;
	break;
#endif

	case ATH6KL_TM_CMD_DFS_SKIP:
	{
		struct wifi_dfs_skip_params *dfs_skip_params;
		struct ath6kl_vif *vif;

		vif = ath6kl_vif_first(ar);

		if (!vif)
			return -EINVAL;

		if (!tb[ATH6KL_TM_ATTR_DATA]) {
			printk(KERN_ERR "%s: NO DATA\n", __func__);
			return -EINVAL;
		}

		buf = nla_data(tb[ATH6KL_TM_ATTR_DATA]);
		buf_len = nla_len(tb[ATH6KL_TM_ATTR_DATA]);

		dfs_skip_params = (struct wifi_dfs_skip_params *)buf;

		if (dfs_skip_params->enable)
			vif->sc_params.scan_ctrl_flags
				 |= ENABLE_DFS_SKIP_CTRL_FLAGS;
		else
			vif->sc_params.scan_ctrl_flags
				 &= ~ENABLE_DFS_SKIP_CTRL_FLAGS;

		if (ath6kl_wmi_scanparams_cmd(ar->wmi, vif->fw_vif_idx,
				vif->sc_params.fg_start_period,
				vif->sc_params.fg_end_period,
				vif->sc_params.bg_period,
				vif->sc_params.minact_chdwell_time,
				vif->sc_params.maxact_chdwell_time,
				vif->sc_params.pas_chdwell_time,
				vif->sc_params.short_scan_ratio,
				vif->sc_params.scan_ctrl_flags,
				vif->sc_params.max_dfsch_act_time,
				vif->sc_params.maxact_scan_per_ssid)) {
						printk(KERN_ERR "%s: wifi dfs skip enable fail\n",
					__func__);
				return -EINVAL;
			}
	}

	return 0;
	break;

#if defined(CE_2_SUPPORT)
	case ATH6KL_TM_CMD_WIFI_DEBUGFS:
        {
            struct wlan_debugfs_params *debugfs_params;
		    struct ath6kl_vif *vif;
            vif = ath6kl_vif_first(ar);

		    if (!vif)
			    return -EINVAL;

            if (!tb[ATH6KL_TM_ATTR_DATA]) {
			    printk(KERN_ERR "%s: NO DATA\n", __func__);
			    return -EINVAL;
            }

            buf = nla_data(tb[ATH6KL_TM_ATTR_DATA]);
		    buf_len = nla_len(tb[ATH6KL_TM_ATTR_DATA]);

            debugfs_params = (struct wlan_debugfs_params *)buf;
            if (debugfs_params->cmd == DEBUGFS_TX_POWER) {
                return debugfs_tx_power(vif, debugfs_params->params.tx_power_params.ratio);
            } else if (debugfs_params->cmd == DEBUGFS_PASSIVE_CHANNEL_SKIP) {
                debugfs_passive_channel_skip(vif, debugfs_params->params.passive_ch_skip_parames.skip);
            } else if (debugfs_params->cmd == DEBUGFS_GET_TSF) {
                struct sk_buff *skb;
                struct wlan_debugfs_read back_data;

                skb = cfg80211_testmode_alloc_reply_skb(wiphy,
                               sizeof(struct wlan_debugfs_read));
                if (!skb)
                    return -ENOMEM;
                back_data.cmd = DEBUGFS_GET_TSF;
                {
	                u32 datah = 0, datal = 0;
#define TSF_TO_TU(_h,_l)    ((((u64)(_h)) << 32) | ((u64)(_l)))
#define REG_TSF_L 0x5054
#define REG_TSF_H 0x5058
	                ath6kl_diag_read32(ar, REG_TSF_L, &datal);
	                ath6kl_diag_read32(ar, REG_TSF_H, &datah);
                    back_data.params.tsf_read_params.tsf_value = TSF_TO_TU(datah, datal);
                }
                nla_put(skb, NL80211_ATTR_TESTDATA, sizeof(struct wlan_debugfs_read), &back_data);
                return cfg80211_testmode_reply(skb);            
            } else if (debugfs_params->cmd == DEBUGFS_SYNC_TSF) {
                ath6kl_wmi_set_sync_tsf_cmd(ar->wmi, 1);
            } else if (debugfs_params->cmd == DEBUGFS_GET_SCAN_PARAMS) {
                debugfs_show_scan_params(ar);
            } else if (debugfs_params->cmd == DEBUGFS_SET_SCAN_PARAMS) {
                debugfs_set_scan_params(ar,
                    debugfs_params->params.scan_set_params.act_dwell_t,
                    debugfs_params->params.scan_set_params.pas_dwell_t,
                    debugfs_params->params.scan_set_params.per_ssid);
            } else if (debugfs_params->cmd == DEBUGFS_GET_DEBUG_MASK) {
                struct sk_buff *skb;
                struct wlan_debugfs_read back_data;

                skb = cfg80211_testmode_alloc_reply_skb(wiphy,
                               sizeof(struct wlan_debugfs_read));
                if (!skb)
                    return -ENOMEM;
                back_data.cmd = DEBUGFS_GET_DEBUG_MASK;
                back_data.params.debug_mask_params.mask = debug_mask;
                back_data.params.debug_mask_params.mask_ext = debug_mask_ext;
                nla_put(skb, NL80211_ATTR_TESTDATA, sizeof(struct wlan_debugfs_read), &back_data);
                return cfg80211_testmode_reply(skb);
            } else if (debugfs_params->cmd == DEBUGFS_SET_DEBUG_MASK) {
                debug_mask = debugfs_params->params.debug_mask_params.mask;
                debug_mask_ext = debugfs_params->params.debug_mask_params.mask_ext;
            } else if (debugfs_params->cmd == DEBUGFS_GET_AP_ALIVE_PARAMS) {
                debugfs_show_ap_alive_params(ar);
            } else if (debugfs_params->cmd == DEBUGFS_SET_AP_ALIVE_PARAMS) {
                debugfs_set_ap_alive_params(ar,
                    debugfs_params->params.ap_alive_params.internal,
                    debugfs_params->params.ap_alive_params.cycle);
            } else if (debugfs_params->cmd == DEBUGFS_GET_HT_CAP_PARAMS) {
                debugfs_show_ht_cap_params(ar);
            } else if (debugfs_params->cmd == DEBUGFS_SET_HT_CAP_PARAMS) {
                debugfs_set_ht_cap_params(ar,
                    debugfs_params->params.ht_cap_params.band,
                    debugfs_params->params.ht_cap_params.ht40_supported,
                    debugfs_params->params.ht_cap_params.short_GI,
                    debugfs_params->params.ht_cap_params.intolerance_ht40);
            } else if (debugfs_params->cmd == DEBUGFS_CHAN_LIST) {
                debugfs_show_chan_list(ar);
            } else if (debugfs_params->cmd == DEBUGFS_AMPDU) {
                debugfs_ampdu_enable(ar,
                    debugfs_params->params.ampdu_parames.enable_ampdu);
            } else if (debugfs_params->cmd == DEBUGFS_REG_READ) {
                u32 reg_addr, reg_value;
                struct sk_buff *skb;
                struct wlan_debugfs_read back_data;

                skb = cfg80211_testmode_alloc_reply_skb(wiphy,
                               sizeof(struct wlan_debugfs_read));
                if (!skb)
                    return -ENOMEM;
                back_data.cmd = DEBUGFS_REG_READ;

                reg_addr = debugfs_params->params.reg_params.reg_addr;
                back_data.params.reg_params.reg_addr = reg_addr;
                if (ath6kl_diag_read32(ar, reg_addr, &reg_value)) {
                    /*Read Fail*/
#define READ_FAIL_VALUE 0xbe000bad
                    back_data.params.reg_params.reg_value = READ_FAIL_VALUE;
                } else {
                    back_data.params.reg_params.reg_value = reg_value;
                }
                nla_put(skb, NL80211_ATTR_TESTDATA, sizeof(struct wlan_debugfs_read), &back_data);
                return cfg80211_testmode_reply(skb);
            } else if (debugfs_params->cmd == DEBUGFS_REG_WRITE) {
                u32 reg_addr, reg_value;
                reg_addr = debugfs_params->params.reg_params.reg_addr;
                reg_value = debugfs_params->params.reg_params.reg_value;

                ath6kl_diag_write32(ar, reg_addr, cpu_to_le32(reg_value));
            } else if (debugfs_params->cmd == DEBUGFS_DA_RETRY_LIMIT) {
                u32 retry_limit;
                retry_limit = debugfs_params->params.direct_audio_parames.retry_limit;
                ath6kl_wmi_set_da_retry_limit_cmd(ar->wmi, retry_limit);
            }
		}
        return 0;
#endif //CE_2_SUPPORT
	default:
		return -EOPNOTSUPP;
	}
}

#endif /* CONFIG_NL80211_TESTMODE */
