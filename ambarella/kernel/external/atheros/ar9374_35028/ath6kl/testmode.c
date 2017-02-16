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

	default:
		return -EOPNOTSUPP;
	}
}

#endif /* CONFIG_NL80211_TESTMODE */
