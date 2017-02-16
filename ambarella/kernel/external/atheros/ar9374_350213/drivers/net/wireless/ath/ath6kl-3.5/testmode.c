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
};

#define ATH6KL_TM_DATA_MAX_LEN		5000

static const struct nla_policy ath6kl_tm_policy[ATH6KL_TM_ATTR_MAX + 1] = {
	[ATH6KL_TM_ATTR_CMD]		= { .type = NLA_U32 },
	[ATH6KL_TM_ATTR_DATA]		= { .type = NLA_BINARY,
					    .len = ATH6KL_TM_DATA_MAX_LEN },
};

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

nla_put_failure:
	kfree_skb(skb);
	printk(KERN_ERR "nla_put failed on testmode rx skb!\n");
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

nla_put_failure:
	kfree_skb(skb);
	printk(KERN_ERR "nla_put failed on testmode event skb!\n");
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

nla_put_failure:
	kfree_skb(skb);
	printk(KERN_ERR "nla_put failed on testmode event skb!\n");
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

nla_put_failure:
	ret = -ENOBUFS;
	goto out;
}
EXPORT_SYMBOL(ath6kl_tm_rx_report);

#ifdef ATH6KL_SUPPORT_WLAN_HB
enum nl80211_wlan_hb_cmd {
	NL80211_WLAN_HB_ENABLE		= 0,
	NL80211_WLAN_TCP_PARAMS		= 1,
	NL80211_WLAN_TCP_FILTER		= 2,
	NL80211_WLAN_UDP_PARAMS		= 3,
	NL80211_WLAN_UDP_FILTER		= 4,
	NL80211_WLAN_NET_INFO		= 5,
};

#define WLAN_HB_UDP_ENABLE		0x1
#define WLAN_HB_TCP_ENABLE		0x2

struct wlan_hb_params {
	u16 cmd;
	u16 enable;

	union {
		struct {
			u16 src_port;
			u16 dst_port;
			u16 timeout;
		} tcp_params;

		struct {
			u16 length;
			u8 filter[64];
		} tcp_filter;

		struct {
			u16 src_port;
			u16 dst_port;
			u16 interval;
			u16 timeout;
		} udp_params;

		struct {
			u16 length;
			u8 filter[64];
		} udp_filter;

		struct {
			u32 device_ip;
			u32 server_ip;
			u32 gateway_ip;
			u8 gateway_mac[ETH_ALEN];
		} net_info;
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
			if (hb_params->enable != 0) {
				if (hb_params->enable & WLAN_HB_TCP_ENABLE) {
					ar->wlan_hb_enable |=
						WLAN_HB_TCP_ENABLE;

					if (ath6kl_wmi_set_heart_beat_params(
						ar->wmi,
						vif->fw_vif_idx,
						WLAN_HB_TCP_ENABLE)) {
						printk(KERN_ERR
						"%s: set heart beat enable fail\n",
						__func__);
						return -EINVAL;
					}
				} else
				if (hb_params->enable & WLAN_HB_UDP_ENABLE) {
					ar->wlan_hb_enable |=
						WLAN_HB_UDP_ENABLE;
				}
			} else {
				ar->wlan_hb_enable = 0;
				if (ath6kl_wmi_set_heart_beat_params(ar->wmi,
					vif->fw_vif_idx, 0)) {
					printk(KERN_ERR
					"%s: set heart beat enable fail\n",
					__func__);
					return -EINVAL;
				}
			}

		} else if (hb_params->cmd == NL80211_WLAN_TCP_PARAMS) {
			if (ath6kl_wmi_heart_beat_set_tcp_params(ar->wmi,
				vif->fw_vif_idx,
				hb_params->params.tcp_params.src_port,
				hb_params->params.tcp_params.dst_port,
				hb_params->params.tcp_params.timeout)) {
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
				hb_params->params.tcp_filter.length)) {
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
				hb_params->params.udp_params.interval,
				hb_params->params.udp_params.timeout)) {
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
					hb_params->params.udp_filter.length)) {
				printk(KERN_ERR
				"%s: set heart beat udp filter fail\n",
				__func__);
				return -EINVAL;
			}
		} else if (hb_params->cmd == NL80211_WLAN_NET_INFO) {
			if (ath6kl_wmi_heart_beat_set_network_info(ar->wmi,
				vif->fw_vif_idx,
				hb_params->params.net_info.device_ip,
				hb_params->params.net_info.server_ip,
				hb_params->params.net_info.gateway_ip,
				hb_params->params.net_info.gateway_mac)) {
				printk(KERN_ERR
				"%s: set heart beat network information fail\n",
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
		} else if (disc_params->cmd == NL80211_WIFI_DISC_STOP) {
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

		if (!ar->ktk_enable) {
			printk(KERN_ERR "%s: KTK feature is not enabled\n",
					__func__);
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

			memcpy(ar->ktk_passphrase,
				ktk_params->params.start_params.passphrase,
				16);

			if (ath6kl_wmi_probedssid_cmd(ar->wmi, vif->fw_vif_idx,
				1, SPECIFIC_SSID_FLAG,
				ktk_params->params.start_params.ssid_len,
				ktk_params->params.start_params.ssid)) {
				printk(KERN_ERR
					"%s: wifi ktk set probedssid fail\n",
					__func__);
				return -EINVAL;
			}
		} else if (ktk_params->cmd == NL80211_WIFI_KTK_STOP) {
			ar->ktk_active = false;
		}
	}

	return 0;
	break;
#endif
	default:
		return -EOPNOTSUPP;
	}
}

