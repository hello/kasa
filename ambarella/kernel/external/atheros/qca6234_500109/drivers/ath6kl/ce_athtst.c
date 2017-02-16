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

#include "core.h"
#include "hif-ops.h"
#include "cfg80211.h"
#include "target.h"
#include "debug.h"
#include "ioctl_vendor_ce.h"
#include "ce_athtst.h"
#include "hif-ops.h"
#include "tx99_wcmd.h"
#include "acs.h"
#include "acl_wcmd.h"
#include "ieee80211_ioctl.h"
#include <linux/vmalloc.h>
#include <linux/if_arp.h>       /* XXX for ARPHRD_ETHER */
#include <linux/wireless.h>

#ifdef ATHTST_SUPPORT
u32 addr_private;
u32 val_private;
static struct athcfg_wcmd_sta_t stainfo_private;
static struct athcfg_wcmd_txpower_t txpow_private;
static struct athcfg_wcmd_version_info_t version_info_private;
struct athcfg_wcmd_testmode_t testmode_private;
#if defined(CE_CUSTOM_1)
u8 widi_enable_private;
#endif

struct wmi_ctl_cmd	ctl_private;

void ath6kl_tgt_ce_get_reg_event(struct ath6kl_vif *vif, u8 *ptr, u32 len)
{
	struct wmi_reg_cmd *p = (struct wmi_reg_cmd *) ptr;
	u32 addr;
	u32 val;

	addr_private = addr = le32_to_cpu(p->addr);
	val_private = val = le32_to_cpu(p->val);

	if (test_bit(CE_WMI_UPDATE, &vif->flags)) {
		clear_bit(CE_WMI_UPDATE, &vif->flags);
		wake_up(&vif->ar->event_wq);
	}
}


void ath6kl_tgt_ce_get_version_info_event(struct ath6kl_vif *vif,
				u8 *ptr, u32 len)
{
	struct WMI_CUSTOMER_VERSION_INFO_STRUCT *p =
		(struct WMI_CUSTOMER_VERSION_INFO_STRUCT *) ptr;

	memcpy(version_info_private.driver, p->driver,
			sizeof(version_info_private.driver));
	p->sw_version = le32_to_cpu(p->sw_version);
	sprintf(version_info_private.version, "%s", TO_STR(__BUILD_VERSION_));
	sprintf(version_info_private.fw_version, "%d.%d.%d.%d",
		(p->sw_version&0xf0000000)>>28,
		(p->sw_version&0x0f000000)>>24,
		(p->sw_version&0x00ff0000)>>16,
		(p->sw_version&0x0000ffff));
	if (test_bit(CE_WMI_UPDATE, &vif->flags)) {
		clear_bit(CE_WMI_UPDATE, &vif->flags);
		wake_up(&vif->ar->event_wq);
	}
}

#if defined(CE_CUSTOM_1)
void ath6kl_tgt_ce_get_widimode_event(struct ath6kl_vif *vif, u8 *ptr, u32 len)
{
	widi_enable_private = *ptr;

	if (test_bit(CE_WMI_UPDATE, &vif->flags)) {
		clear_bit(CE_WMI_UPDATE, &vif->flags);
		wake_up(&vif->ar->event_wq);
	}
}
#endif

void ath6kl_tgt_ce_get_testmode_event(struct ath6kl_vif *vif, u8 *ptr, u32 len)
{
	struct WMI_CUSTOMER_TESTMODE_STRUCT *p =
		(struct WMI_CUSTOMER_TESTMODE_STRUCT *) ptr;

	switch (p->operation) {
	case ATHCFG_WCMD_TESTMODE_CHAN:
		testmode_private.chan = le16_to_cpu(p->chan);
		break;
	case ATHCFG_WCMD_TESTMODE_BSSID:
		memcpy(testmode_private.bssid,
				p->bssid,
				sizeof(testmode_private.bssid));
		break;
	case ATHCFG_WCMD_TESTMODE_RX:
		testmode_private.rx = p->rx;
		break;
	case ATHCFG_WCMD_TESTMODE_RESULT:
		testmode_private.rssi_combined = le32_to_cpu(p->rssi_combined);
		testmode_private.rssi0 = le32_to_cpu(p->rssi0);
		testmode_private.rssi1 = le32_to_cpu(p->rssi1);
		testmode_private.rssi2 = le32_to_cpu(p->rssi2);
		break;
	case ATHCFG_WCMD_TESTMODE_ANT:
		testmode_private.antenna = p->antenna;
		break;
	}

	if (test_bit(CE_WMI_TESTMODE_GET, &vif->flags)) {
		clear_bit(CE_WMI_TESTMODE_GET, &vif->flags);
		wake_up(&vif->ar->event_wq);
	}
}


void ath6kl_tgt_ce_get_txpow_event(struct ath6kl_vif *vif, u8 *ptr, u32 len)
{
	struct WMI_CUSTOMER_TXPOW_STRUCT *p =
		(struct WMI_CUSTOMER_TXPOW_STRUCT *)ptr;

	memcpy(&txpow_private.txpowertable[0],
			&p->txpowertable[0],
			sizeof(struct WMI_CUSTOMER_TXPOW_STRUCT));

	if (test_bit(CE_WMI_UPDATE, &vif->flags)) {
		clear_bit(CE_WMI_UPDATE, &vif->flags);
		wake_up(&vif->ar->event_wq);
	}
}


void ath6kl_tgt_ce_get_stainfo_event(struct ath6kl_vif *vif, u8 *ptr, u32 len)
{
	struct WMI_CUSTOMER_STAINFO_STRUCT *p =
		(struct WMI_CUSTOMER_STAINFO_STRUCT *)ptr;

	stainfo_private.flags = p->flags;
	stainfo_private.phymode = p->phymode;
	memcpy(&stainfo_private.bssid, p->bssid, sizeof(stainfo_private.bssid));
	stainfo_private.assoc_time = le32_to_cpu(p->assoc_time);
	stainfo_private.wpa_4way_handshake_time =
			le32_to_cpu(p->wpa_4way_handshake_time);
	stainfo_private.wpa_2way_handshake_time =
			le32_to_cpu(p->wpa_2way_handshake_time);
	stainfo_private.rx_rate_kbps = le32_to_cpu(p->rx_rate_kbps);
	stainfo_private.rx_rate_mcs = p->rx_rate_mcs;
	stainfo_private.tx_rate_kbps = le32_to_cpu(p->tx_rate_kbps);
	stainfo_private.rx_rssi = p->rx_rssi;
	stainfo_private.rx_rssi_beacon = p->rx_rssi_beacon;

	if (test_bit(CE_WMI_UPDATE, &vif->flags)) {
		clear_bit(CE_WMI_UPDATE, &vif->flags);
		wake_up(&vif->ar->event_wq);
	}
}

static int total_bss_info;
static struct athcfg_wcmd_scan_result_t scaninfor_db[100];
struct ieee80211_ie_header {
	u_int8_t    element_id;     /* Element Id */
	u_int8_t    length;         /* IE Length */
} __packed;
enum {
	IEEE80211_ELEMID_SSID             = 0,
	IEEE80211_ELEMID_RATES            = 1,
	IEEE80211_ELEMID_FHPARMS          = 2,
	IEEE80211_ELEMID_DSPARMS          = 3,
	IEEE80211_ELEMID_CFPARMS          = 4,
	IEEE80211_ELEMID_TIM              = 5,
	IEEE80211_ELEMID_IBSSPARMS        = 6,
	IEEE80211_ELEMID_COUNTRY          = 7,
	IEEE80211_ELEMID_REQINFO          = 10,
	IEEE80211_ELEMID_QBSS_LOAD        = 11,
	IEEE80211_ELEMID_CHALLENGE        = 16,
	/* 17-31 reserved for challenge text extension */
	IEEE80211_ELEMID_PWRCNSTR         = 32,
	IEEE80211_ELEMID_PWRCAP           = 33,
	IEEE80211_ELEMID_TPCREQ           = 34,
	IEEE80211_ELEMID_TPCREP           = 35,
	IEEE80211_ELEMID_SUPPCHAN         = 36,
	IEEE80211_ELEMID_CHANSWITCHANN    = 37,
	IEEE80211_ELEMID_MEASREQ          = 38,
	IEEE80211_ELEMID_MEASREP          = 39,
	IEEE80211_ELEMID_QUIET            = 40,
	IEEE80211_ELEMID_IBSSDFS          = 41,
	IEEE80211_ELEMID_ERP              = 42,
	IEEE80211_ELEMID_HTCAP_ANA        = 45,
	IEEE80211_ELEMID_RESERVED_47      = 47,
	IEEE80211_ELEMID_RSN              = 48,
	IEEE80211_ELEMID_XRATES           = 50,
	IEEE80211_ELEMID_HTCAP            = 51,
	IEEE80211_ELEMID_HTINFO           = 52,
	IEEE80211_ELEMID_MOBILITY_DOMAIN  = 54,
	IEEE80211_ELEMID_FT               = 55,
	IEEE80211_ELEMID_TIMEOUT_INTERVAL = 56,
	IEEE80211_ELEMID_EXTCHANSWITCHANN = 60,
	IEEE80211_ELEMID_HTINFO_ANA       = 61,
	IEEE80211_ELEMID_WAPI             = 68,   /* IE for WAPI */
	IEEE80211_ELEMID_2040_COEXT       = 72,
	IEEE80211_ELEMID_2040_INTOL       = 73,
	IEEE80211_ELEMID_OBSS_SCAN        = 74,
	IEEE80211_ELEMID_XCAPS            = 127,
	IEEE80211_ELEMID_RESERVED_133     = 133,
	IEEE80211_ELEMID_TPC              = 150,
	IEEE80211_ELEMID_CCKM             = 156,
	IEEE80211_ELEMID_VENDOR           = 221,  /* vendor private */
};
#define	WPA_OUI_TYPE        0x01
#define	WPA_OUI             0xf25000
#define LE_READ_4(p)                              \
		((a_uint32_t)                             \
		((((const a_uint8_t *)(p))[0]) |          \
		(((const a_uint8_t *)(p))[1] <<  8) |     \
		(((const a_uint8_t *)(p))[2] << 16) |     \
		(((const a_uint8_t *)(p))[3] << 24)))
#define BE_READ_4(p)                                \
		((a_uint32_t)                               \
		((((const a_uint8_t *)(p))[0] << 24) |     \
		(((const a_uint8_t *)(p))[1] << 16) |     \
		(((const a_uint8_t *)(p))[2] <<  8) |     \
		(((const a_uint8_t *)(p))[3])))
static inline a_int8_t
iswpaoui(const a_uint8_t *frm)
{
	return frm[1] > 3 && LE_READ_4(frm + 2) ==
			((WPA_OUI_TYPE << 24) | WPA_OUI);
}
#define WPS_OUI                     0xf25000
#define WPS_OUI_TYPE                0x04
#define WSC_OUI                     0x0050f204
static inline a_int8_t
iswpsoui(const a_uint8_t *frm)
{
	return frm[1] > 3 && BE_READ_4(frm + 2) == WSC_OUI;
}
#define WME_OUI                     0xf25000
#define WME_OUI_TYPE                0x02
#define WME_INFO_OUI_SUBTYPE        0x00
#define WME_PARAM_OUI_SUBTYPE       0x01
#define WME_TSPEC_OUI_SUBTYPE       0x02
static inline int
iswmeparam(const u_int8_t *frm)
{
	return ((frm[1] > 5) &&
	(LE_READ_4(frm + 2) == ((WME_OUI_TYPE << 24) | WME_OUI)) &&
	(frm[6] == WME_PARAM_OUI_SUBTYPE));
}
#define ATH_OUI                     0x7f0300    /* Atheros OUI */
#define ATH_OUI_TYPE                0x01
static inline int
isatherosoui(const u_int8_t *frm)
{
	return (frm[1] > 3) &&
	(LE_READ_4(frm + 2) == ((ATH_OUI_TYPE << 24) | ATH_OUI));
}
/* Temporary vendor specific IE for 11n pre-standard interoperability */
#define VENDOR_HT_OUI               0x00904c
#define VENDOR_HT_CAP_ID            51
#define VENDOR_HT_INFO_ID           52
static inline int
ishtcap(const u_int8_t *frm)
{
	return ((frm[1] > 3) &&
	(BE_READ_4(frm + 2) == ((VENDOR_HT_OUI << 8) | VENDOR_HT_CAP_ID)));
}
static inline int
ishtinfo(const u_int8_t *frm)
{
	return ((frm[1] > 3) &&
	(BE_READ_4(frm + 2) == ((VENDOR_HT_OUI << 8) | VENDOR_HT_INFO_ID)));
}
struct ieee80211_ie_htcap_cmn {
	u_int16_t   hc_cap;         /* HT capabilities */
	u_int8_t    hc_maxampdu:2,    /* B0-1 maximum rx A-MPDU factor */
				hc_mpdudensity:3,    /* B2-4 MPDU density
				(aka Minimum MPDU Start Spacing) */
				hc_reserved:3;    /* B5-7 reserved */
	u_int8_t    hc_mcsset[16];          /* supported MCS set */
	u_int16_t   hc_extcap;              /* extended HT capabilities */
	u_int32_t   hc_txbf;                /* txbf capabilities */
	u_int8_t    hc_antenna;             /* antenna capabilities */
} __packed;

void ath6kl_tgt_ce_get_scaninfo_event(struct ath6kl_vif *vif,
			u8 *datap, u32 len)
{
	struct wmi_bss_info_hdr2 *bih;
	u8 *buf;
	struct ieee80211_channel *channel;
	struct ieee80211_mgmt *mgmt;
	struct athcfg_wcmd_scan_result_t *scan_entry;
	/* parse ie information */
	struct ieee80211_ie_header *ie_element;
	unsigned char *temp_ptr;
	int remained_len;
	/* find the exist entry or add new one */
	int i, entry_match_item;
	bool entry_match = 0;

	if (len <= sizeof(struct wmi_bss_info_hdr2))
		return;

	bih = (struct wmi_bss_info_hdr2 *) datap;
	buf = datap + sizeof(struct wmi_bss_info_hdr2);
	len -= sizeof(struct wmi_bss_info_hdr2);

	printk(KERN_DEBUG "bss info evt - ch %u, snr %d, rssi %d, bssid \"%pM\" "
		"frame_type=%d\n",
		bih->ch, bih->snr, bih->snr - 95, bih->bssid,
		bih->frame_type);

	if (bih->frame_type != BEACON_FTYPE &&
		bih->frame_type != PROBERESP_FTYPE)
		return; /* Only update BSS table for now */

	channel = ieee80211_get_channel(vif->wdev.wiphy, le16_to_cpu(bih->ch));
	if (channel == NULL)
		return;

	if (len < 8 + 2 + 2)
		return;
	/*
	* In theory, use of cfg80211_inform_bss() would be more natural here
	* since we do not have the full frame. However, at least for now,
	* cfg80211 can only distinguish Beacon and Probe Response frames from
	* each other when using cfg80211_inform_bss_frame(), so let's build a
	* fake IEEE 802.11 header to be able to take benefit of this.
	*/
	mgmt = kmalloc(24 + len, GFP_ATOMIC);
	if (mgmt == NULL)
		return;

	if (bih->frame_type == BEACON_FTYPE) {
		mgmt->frame_control = cpu_to_le16(IEEE80211_FTYPE_MGMT |
					IEEE80211_STYPE_BEACON);
		memset(mgmt->da, 0xff, ETH_ALEN);
	} else {
		struct net_device *dev = vif->ndev;

		mgmt->frame_control = cpu_to_le16(IEEE80211_FTYPE_MGMT |
					IEEE80211_STYPE_PROBE_RESP);
		memcpy(mgmt->da, dev->dev_addr, ETH_ALEN);
	}

	mgmt->duration = cpu_to_le16(0);
	memcpy(mgmt->sa, bih->bssid, ETH_ALEN);
	memcpy(mgmt->bssid, bih->bssid, ETH_ALEN);
	mgmt->seq_ctrl = cpu_to_le16(0);

	memcpy(&mgmt->u.beacon, buf, len);

	/* save to scan information database */
	scan_entry = kmalloc(sizeof(struct athcfg_wcmd_scan_result_t),
		GFP_ATOMIC);
	if (scan_entry == NULL)
		goto fail;
	memset(scan_entry, 0x00, sizeof(struct athcfg_wcmd_scan_result_t));
	scan_entry->isr_freq = bih->ch;
	scan_entry->isr_ieee =
		(unsigned char)ieee80211_frequency_to_channel(bih->ch);
	scan_entry->isr_rssi = bih->snr;
	memcpy(&scan_entry->isr_bssid[0],
		&bih->bssid[0],
		ATHCFG_WCMD_ADDR_LEN);
	scan_entry->isr_capinfo =
		le16_to_cpu(mgmt->u.beacon.capab_info);
	/* parse ie information */
	ie_element = (struct ieee80211_ie_header *)
	&(mgmt->u.beacon.variable);
	remained_len = len - 5 * sizeof(u8);
	while (remained_len >= 0) {
		remained_len = remained_len -
			sizeof(struct ieee80211_ie_header) -
			ie_element->length;
		if (ie_element->length == 0) {
			ie_element += 1;    /* next IE */
			continue;
		}
		if (remained_len < ie_element->length) {
			/* Incomplete/bad info element */
			break;
		}

		temp_ptr = (unsigned char *)ie_element;
		/* point to data area */
		temp_ptr = temp_ptr +
		sizeof(struct ieee80211_ie_header);

		switch (ie_element->element_id) {
		case IEEE80211_ELEMID_SSID:
			memcpy(
			&scan_entry->isr_ssid,
			temp_ptr,
			ie_element->length);
			break;
		case IEEE80211_ELEMID_RATES:
			memcpy(
			&scan_entry->isr_rates[0],
			temp_ptr,
			ie_element->length);
			scan_entry->isr_nrates = ie_element->length;
			break;
		case IEEE80211_ELEMID_XRATES:
			memcpy(
			&scan_entry->isr_rates[scan_entry->isr_nrates],
			temp_ptr,
			ie_element->length);
			scan_entry->isr_nrates += ie_element->length;
			break;
		case IEEE80211_ELEMID_ERP:
			memcpy(&scan_entry->isr_erp,
			temp_ptr,
			ie_element->length);
			break;
		case IEEE80211_ELEMID_RSN:
			scan_entry->isr_rsn_ie.len = ie_element->length;
			memcpy(&scan_entry->isr_rsn_ie.data,
			temp_ptr,
			ie_element->length);
			break;
		case IEEE80211_ELEMID_HTCAP_ANA:
			if (scan_entry->isr_htcap_ie.len == 0) {
				scan_entry->isr_htcap_ie.len =
				ie_element->length;
				memcpy(
				&scan_entry->isr_htcap_ie.data[0],
				temp_ptr,
				ie_element->length);
			}
			break;
		case IEEE80211_ELEMID_HTINFO_ANA:
		/* we only care if there isn't already an HT IE (ANA) */
			if (scan_entry->isr_htinfo_ie.len == 0) {
				scan_entry->isr_htinfo_ie.len =
				ie_element->length;
				memcpy(
				&scan_entry->isr_htinfo_ie.data[0],
				temp_ptr,
				ie_element->length);
			}
			break;
		case IEEE80211_ELEMID_HTCAP:
		/* we only care if there isn't already an HT IE (ANA) */
			if (scan_entry->isr_htcap_ie.len == 0) {
				scan_entry->isr_htcap_ie.len =
				ie_element->length;
				memcpy(
				&scan_entry->isr_htcap_ie.data[0],
				temp_ptr,
				ie_element->length);
			}
			break;
		case IEEE80211_ELEMID_HTINFO:
		/* we only care if there isn't already an HT IE (ANA) */
			if (scan_entry->isr_htinfo_ie.len == 0) {
				scan_entry->isr_htinfo_ie.len =
				ie_element->length;
				memcpy(
				&scan_entry->isr_htinfo_ie.data[0],
				temp_ptr,
				ie_element->length);
			}
			break;
		case IEEE80211_ELEMID_VENDOR:
			if (iswpaoui((u_int8_t *) ie_element)) {
				scan_entry->isr_wpa_ie.len =
				ie_element->length;
				memcpy(
				&scan_entry->isr_wpa_ie.data[0],
				temp_ptr,
				ie_element->length);
			} else if (iswpsoui((u_int8_t *) ie_element)) {
				scan_entry->isr_wps_ie.len =
				ie_element->length;
				memcpy(
				&scan_entry->isr_wps_ie.data[0],
				temp_ptr,
				ie_element->length);
			} else if (iswmeparam(
					(u_int8_t *) ie_element)) {
				scan_entry->isr_wme_ie.len =
				ie_element->length;
				memcpy(
				&scan_entry->isr_wme_ie.data[0],
				temp_ptr,
				ie_element->length);
			} else if (isatherosoui(
					(u_int8_t *) ie_element)) {
				scan_entry->isr_ath_ie.len =
				ie_element->length;
				memcpy(
				&scan_entry->isr_ath_ie.data[0],
				temp_ptr,
				ie_element->length);
			} else if (ishtcap(
					(u_int8_t *) ie_element)) {
				if (scan_entry->isr_htcap_ie.len == 0) {
					scan_entry->isr_htcap_ie.len =
					ie_element->length;
					memcpy(
					&scan_entry->isr_htcap_ie.data[0],
					temp_ptr,
					ie_element->length);
				}
			} else if (ishtinfo(
					(u_int8_t *) ie_element)) {
				if (scan_entry->isr_htinfo_ie.len ==  0) {
					scan_entry->isr_htinfo_ie.len =
					ie_element->length;
					memcpy(
					&scan_entry->isr_htinfo_ie.data[0],
					temp_ptr,
					ie_element->length);
				}
			}
			break;
		default:
			break;
		}
		ie_element = (struct ieee80211_ie_header *)
		(temp_ptr + ie_element->length);
	}

	/* find the exist entry or add new one */
	if (total_bss_info >= 100) {
		printk(KERN_DEBUG "Excess max entry 100\n");
		goto fail_free_all;
	}

	for (i = 0; i < total_bss_info; i++) {
		if (scaninfor_db[i].isr_freq == scan_entry->isr_freq) {
			if (memcmp(scaninfor_db[i].isr_bssid ,
				scan_entry->isr_bssid,
				ATHCFG_WCMD_ADDR_LEN) == 0) {
				/* find it */
				if (strcmp(scaninfor_db[i].isr_ssid ,
					scan_entry->isr_ssid) == 0) {
					/* update it */
					entry_match = 1;
					entry_match_item = i;
					memcpy(&scaninfor_db[i], scan_entry,
				sizeof(struct athcfg_wcmd_scan_result_t));
					break;
				}
			}
		}
	}

	if (entry_match == 0) {
		memcpy(&scaninfor_db[total_bss_info],
			scan_entry,
			sizeof(struct athcfg_wcmd_scan_result_t));
		total_bss_info++;
	}

fail_free_all:
	kfree(scan_entry);
fail:
	kfree(mgmt);

	return;
}


void ath6kl_tgt_ce_set_scan_done_event(struct ath6kl_vif *vif,
				u8 *datap, u32 len)
{
	if (test_bit(CE_WMI_SCAN, &vif->flags)) {
		clear_bit(CE_WMI_SCAN, &vif->flags);
		wake_up(&vif->ar->event_wq);
	}
	return;
}

void ath6kl_tgt_ce_csa_event_rx(struct ath6kl_vif *vif, u8 *datap,int len)
{
	struct wmi_csa_info_strut *p = (struct wmi_csa_info_strut *) datap;
	struct net_device *dev = vif->ndev;
	struct ieee80211_channel *channel;
	struct ath6kl *ar = vif->ar;
	struct cfg80211_bss *bss;
	int freq;
	u16	networkType;
	enum nl80211_channel_type channel_type;

	freq = le32_to_cpu(p->freq);
	networkType = le16_to_cpu(p->networkType);
	if (networkType & INFRA_NETWORK)
		vif->phymode = ((u16)networkType >> 8) & 0xff;

	if( vif->phymode <= ATH6KL_PHY_MODE_11GONLY)
		channel_type = NL80211_CHAN_NO_HT;
	else if( vif->phymode <= ATH6KL_PHY_MODE_11NG_HT40) {
		channel_type = NL80211_CHAN_HT20;
	} else
		channel_type = NL80211_CHAN_HT40MINUS;
	cfg80211_csa_done_info(dev, freq, channel_type, 0, GFP_ATOMIC);
	channel = ieee80211_get_channel(vif->ar->wiphy, freq);
	bss = cfg80211_get_bss_ce(ar->wiphy, vif->bssid,
					   vif->ssid, vif->ssid_len);
	if (bss != NULL)
		bss->channel = channel;

	return;
}

void ath6kl_tgt_ce_get_ctl_event(struct ath6kl_vif *vif, u8 *ptr, u32 len)
{
	struct wmi_ctl_cmd *p = (struct wmi_ctl_cmd *) ptr;
	int i;
	ctl_private.channel = le32_to_cpu(p->channel);
	for(i=0;i<TARGET_POWER_MODE_MAX;i++) {
		ctl_private.txpower[i] = le32_to_cpu(p->txpower[i]);
		ctl_private.ctlmode[i] = p->ctlmode[i];
		//printk("[%d]channel=%d 0x%x 0x%x\n\r",i,ctl_private.channel ,ctl_private.txpower[i],ctl_private.ctlmode[i]);
	}
	
	for(i=0;i<CE_RATE_MAX;i++) {
		ctl_private.cal_txpower[i] = le16_to_cpu(p->cal_txpower[i]);
	}
	if (test_bit(CE_WMI_UPDATE, &vif->flags)) {
		clear_bit(CE_WMI_UPDATE, &vif->flags);
		wake_up(&vif->ar->event_wq);
	}
}

int ath6kl_wmi_get_customer_product_info_cmd(struct ath6kl_vif *vif,
			struct athcfg_wcmd_product_info_t *val)
{
	int ret = 0;

	ath6kl_usb_get_usbinfo((void *)val);
	return ret;
}


int ath6kl_wmi_set_customer_reg_cmd(struct ath6kl_vif *vif, u32 addr,
			u32 val)
{
	struct sk_buff *skb;
	struct wmi_reg_cmd *cmd;
	int ret;

	skb = ath6kl_wmi_get_new_buf(sizeof(*cmd));
	if (!skb)
		return -ENOMEM;

	cmd = (struct wmi_reg_cmd *) skb->data;
	cmd->addr = addr;
	cmd->val = val;
	ret = ath6kl_wmi_cmd_send(vif->ar->wmi, vif->fw_vif_idx, skb,
				WMI_SET_CUSTOM_REG, NO_SYNC_WMIFLAG);
	return ret;
}


int ath6kl_wmi_get_customer_testmode_cmd(struct ath6kl_vif *vif,
		struct athcfg_wcmd_testmode_t *val)
{
	struct sk_buff *skb;
	struct WMI_CUSTOMER_TESTMODE_STRUCT *cmd;
	int ret;
	int left;

	skb = ath6kl_wmi_get_new_buf(sizeof(*cmd));
	if (!skb)
		return -ENOMEM;
	cmd = (struct WMI_CUSTOMER_TESTMODE_STRUCT *)skb->data;

	/* transfer to little endian */
	memcpy(cmd->bssid, val->bssid,  sizeof(cmd->bssid));
	cmd->chan = cpu_to_le32(val->chan);
	cmd->operation = cpu_to_le16(val->operation);
	cmd->antenna = val->antenna;
	cmd->rx = val->rx;

	cmd->rssi_combined = cpu_to_le32(val->rssi_combined);
	cmd->rssi0 = cpu_to_le32(val->rssi0);
	cmd->rssi1 = cpu_to_le32(val->rssi1);
	cmd->rssi2 = cpu_to_le32(val->rssi2);

	set_bit(CE_WMI_TESTMODE_GET, &vif->flags);

	ret = ath6kl_wmi_cmd_send(vif->ar->wmi, vif->fw_vif_idx, skb,
			WMI_GET_CUSTOM_TESTMODE, NO_SYNC_WMIFLAG);

	left = wait_event_interruptible_timeout(vif->ar->event_wq,
		!test_bit(CE_WMI_TESTMODE_GET, &vif->flags), WMI_TIMEOUT);

	if (left == 0)
		return -ETIMEDOUT;
	switch (val->operation) {
	case ATHCFG_WCMD_TESTMODE_CHAN:
		val->chan = testmode_private.chan;
		break;
	case ATHCFG_WCMD_TESTMODE_BSSID:
		memcpy(val->bssid, testmode_private.bssid, sizeof(val->bssid));
		break;
	case ATHCFG_WCMD_TESTMODE_RX:
		val->rx = testmode_private.rx;
		break;
	case ATHCFG_WCMD_TESTMODE_RESULT:
		val->rssi_combined = testmode_private.rssi_combined;
		val->rssi0 = testmode_private.rssi0;
		val->rssi1 = testmode_private.rssi1;
		val->rssi2 = testmode_private.rssi2;
		break;
	case ATHCFG_WCMD_TESTMODE_ANT:
		val->antenna = testmode_private.antenna;
		break;
	default:
		printk(KERN_DEBUG "%s[%d]Not support\n", __func__, __LINE__);
		break;
	}
	return ret;
}


int ath6kl_wmi_set_customer_testmode_cmd(struct ath6kl_vif *vif,
		struct athcfg_wcmd_testmode_t *val)
{
	struct sk_buff *skb;
	struct WMI_CUSTOMER_TESTMODE_STRUCT *cmd;
	int ret = 0;
	bool send_wmi_flag = false;

	switch (val->operation) {
	case ATHCFG_WCMD_TESTMODE_BSSID:
	{
		if (memcmp(testmode_private.bssid,
			val->bssid,
			sizeof(testmode_private.bssid)) != 0) {
			memcpy(testmode_private.bssid,
				val->bssid,
				sizeof(testmode_private.bssid));
			send_wmi_flag = true;
		}
	}
	break;
	case ATHCFG_WCMD_TESTMODE_CHAN:
	{
		if (testmode_private.chan != val->chan) {
			testmode_private.chan = val->chan;
			send_wmi_flag = true;
		}
	}
	break;
	case ATHCFG_WCMD_TESTMODE_RX:
	{
		if (testmode_private.rx != val->rx) {
			testmode_private.rx = val->rx;
			send_wmi_flag = true;
		}
	}
	break;
	case ATHCFG_WCMD_TESTMODE_ANT:
	default:
		printk(KERN_DEBUG "%s[%d]Not support\n", __func__, __LINE__);
		return -1;
	}

	/* send WMI to target */
	if (send_wmi_flag) {
		testmode_private.rx = val->rx;
		skb = ath6kl_wmi_get_new_buf(sizeof(*cmd));
		if (!skb)
			return -ENOMEM;

		cmd = (struct WMI_CUSTOMER_TESTMODE_STRUCT *)skb->data;
		/* transfer to little endian */
		memcpy(cmd->bssid, val->bssid,  sizeof(cmd->bssid));
		cmd->chan = cpu_to_le32(val->chan);
		cmd->operation = cpu_to_le16(val->operation);
		cmd->antenna = val->antenna;
		cmd->rx = val->rx;

		cmd->rssi_combined = cpu_to_le32(val->rssi_combined);
		cmd->rssi0 = cpu_to_le32(val->rssi0);
		cmd->rssi1 = cpu_to_le32(val->rssi1);
		cmd->rssi2 = cpu_to_le32(val->rssi2);

		ret = ath6kl_wmi_cmd_send(vif->ar->wmi, vif->fw_vif_idx, skb,
			WMI_SET_CUSTOM_TESTMODE, NO_SYNC_WMIFLAG);
	}

	if (val->operation == ATHCFG_WCMD_TESTMODE_RX) {
		if (val->rx == 1) {
			s8 n_channels = 1;
			u16 *channels = NULL;
			int i;
			set_bit(CE_WMI_TESTMODE_RX, &vif->flags);

			/* set scan param */
			ath6kl_wmi_scanparams_cmd(vif->ar->wmi,
				vif->fw_vif_idx,
				0,
				0xffff,
				0,
				vif->sc_params.minact_chdwell_time,
				vif->sc_params.maxact_chdwell_time, /* 0xffff*/
				1000, /*vif->sc_params.pas_chdwell_time, msec */
				vif->sc_params.short_scan_ratio,
				(vif->sc_params.scan_ctrl_flags &
						 ~ACTIVE_SCAN_CTRL_FLAGS),
				vif->sc_params.max_dfsch_act_time,
				vif->sc_params.maxact_scan_per_ssid);

			/* assign request channel */
			channels = kzalloc(n_channels * sizeof(u16),
								GFP_KERNEL);
			if (channels == NULL)
				n_channels = 0;

			for (i = 0; i < n_channels; i++)
				channels[i] = ieee80211_channel_to_frequency(
				(testmode_private.chan == 0 ?
					 1 : testmode_private.chan),
				IEEE80211_NUM_BANDS);

			ret = ath6kl_wmi_bssfilter_cmd(
							vif->ar->wmi,
							vif->fw_vif_idx,
							ALL_BSS_FILTER, 0);
			if (ret) {
				printk(KERN_DEBUG "%s[%d] Fail\n",
					__func__, __LINE__);
				goto rx_fail;
			}
			ret = ath6kl_wmi_startscan_cmd(
					vif->ar->wmi,
					vif->fw_vif_idx,
					WMI_LONG_SCAN, 1, false, 0, 0,
					n_channels, channels);
			if (ret) {
				printk(KERN_DEBUG "%s[%d] Fail\n",
				__func__, __LINE__);
				goto rx_fail;
			}

rx_fail:
			kfree(channels);
		} else {
			/* cancel test mode scan */
			clear_bit(CE_WMI_TESTMODE_RX, &vif->flags);
			ath6kl_wmi_scanparams_cmd(vif->ar->wmi,
				vif->fw_vif_idx,
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
		}
	}
	return ret;
}


int ath6kl_wmi_get_customer_reg_cmd(struct ath6kl_vif *vif,
		u32 addr, u32 *val)
{
	struct sk_buff *skb;
	struct wmi_reg_cmd *cmd;
	int ret;
	int left;


	if (down_interruptible(&vif->ar->sem))
		return -EBUSY;

	skb = ath6kl_wmi_get_new_buf(sizeof(*cmd));
	if (!skb) {
		up(&vif->ar->sem);
		return -ENOMEM;
	}

	cmd = (struct wmi_reg_cmd *) skb->data;
	cmd->addr = addr;
	set_bit(CE_WMI_UPDATE, &vif->flags);
	ret = ath6kl_wmi_cmd_send(vif->ar->wmi, vif->fw_vif_idx, skb,
			WMI_GET_CUSTOM_REG, NO_SYNC_WMIFLAG);

	left = wait_event_interruptible_timeout(vif->ar->event_wq,
		!test_bit(CE_WMI_UPDATE, &vif->flags), WMI_TIMEOUT);

	up(&vif->ar->sem);
	if (left == 0)
		return -ETIMEDOUT;

	*val = val_private;
	return ret;
}


int ath6kl_wmi_set_customer_scan_cmd(struct ath6kl_vif *vif)
{
	int ret;
	int left;

	if (down_interruptible(&vif->ar->sem))
		return -EBUSY;

	set_bit(CE_WMI_SCAN, &vif->flags);

	ret = ath6kl_wmi_bssfilter_cmd(vif->ar->wmi, vif->fw_vif_idx,
				ALL_BSS_FILTER, 0);
	if (ret) {
		up(&vif->ar->sem);
		return ret;
	}

	total_bss_info = 0;
	memset(&scaninfor_db[0], 0x00, sizeof(scaninfor_db));
	ret = ath6kl_wmi_startscan_cmd(vif->ar->wmi, vif->fw_vif_idx,
			WMI_LONG_SCAN, 0, false, 0, 0, 0, NULL);

	left = wait_event_interruptible_timeout(vif->ar->event_wq,
			!test_bit(CE_WMI_SCAN, &vif->flags), WMI_TIMEOUT * 10);

	ath6kl_wmi_bssfilter_cmd(vif->ar->wmi, vif->fw_vif_idx,
				 NONE_BSS_FILTER, 0);
	up(&vif->ar->sem);

	return ret;
}


int ath6kl_wmi_get_customer_scan_cmd(struct ath6kl_vif *vif,
			struct athcfg_wcmd_scan_t *val)
{
	a_uint8_t scan_offset;
	int ret = 0, i;
	struct athcfg_wcmd_scan_t *custom_scan_temp;

	if (down_interruptible(&vif->ar->sem))
		return -EBUSY;

	scan_offset = val->offset;
	custom_scan_temp = val;
	if ((scan_offset + ATHCFG_WCMD_MAX_AP) < total_bss_info) {
		custom_scan_temp->cnt = 8;
		custom_scan_temp->more = 1;
	} else {
		custom_scan_temp->cnt = total_bss_info - scan_offset;
		custom_scan_temp->more = 0;
	}

	for (i = 0; i < custom_scan_temp->cnt; i++) {
		if (scaninfor_db[scan_offset+i].isr_htcap_ie.len != 0) {
			memcpy(
			&scaninfor_db[scan_offset+i].isr_htcap_mcsset[0],
((struct ieee80211_ie_htcap_cmn *)
scaninfor_db[scan_offset+i].isr_htcap_ie.data)->hc_mcsset,
			ATHCFG_WCMD_MAX_HT_MCSSET);
		}
		memcpy(
		&custom_scan_temp->result[i],
		&scaninfor_db[scan_offset + i],
		sizeof(struct athcfg_wcmd_scan_result_t));
	}
	up(&vif->ar->sem);

	return ret;
}


int ath6kl_wmi_get_customer_scan_time_cmd(struct ath6kl_vif *vif,
			struct athcfg_wcmd_scantime_t *val)
{
	struct sk_buff *skb;
	struct athcfg_wcmd_sta_t *cmd;
	int ret;
	int left;
	unsigned long start_scan_timestamp;
	unsigned long end_scan_timestamp;
	unsigned long scan_time_msec;

	if (down_interruptible(&vif->ar->sem))
		return -EBUSY;

	skb = ath6kl_wmi_get_new_buf(sizeof(*cmd));
	if (!skb) {
		up(&vif->ar->sem);
		return -ENOMEM;
	}

	cmd = (struct athcfg_wcmd_sta_t *)skb->data;

	set_bit(CE_WMI_SCAN, &vif->flags);

	ret = ath6kl_wmi_bssfilter_cmd(vif->ar->wmi, vif->fw_vif_idx,
					ALL_BSS_FILTER, 0);
	if (ret) {
		up(&vif->ar->sem);
		return ret;
	}
	start_scan_timestamp = jiffies;
	ret = ath6kl_wmi_startscan_cmd(vif->ar->wmi, vif->fw_vif_idx,
			WMI_LONG_SCAN, 0, false, 0, 0, 0, NULL);

	left = wait_event_interruptible_timeout(vif->ar->event_wq,
			!test_bit(CE_WMI_SCAN, &vif->flags), WMI_TIMEOUT * 10);

	end_scan_timestamp = jiffies;
	scan_time_msec =
	jiffies_to_msecs(end_scan_timestamp-start_scan_timestamp);
	if (val)
		val->scan_time = scan_time_msec;

	ath6kl_wmi_bssfilter_cmd(vif->ar->wmi, vif->fw_vif_idx,
		NONE_BSS_FILTER, 0);
	up(&vif->ar->sem);

	return ret;
}


int ath6kl_wmi_get_customer_version_info_cmd(struct ath6kl_vif *vif,
			struct athcfg_wcmd_version_info_t *val)
{
	struct sk_buff *skb;
	struct athcfg_wcmd_version_info_t *cmd;
	int ret;
	int left;
	if (down_interruptible(&vif->ar->sem))
		return -EBUSY;

	skb = ath6kl_wmi_get_new_buf(sizeof(*cmd));
	if (!skb) {
		up(&vif->ar->sem);
		return -ENOMEM;
	}

	cmd = (struct athcfg_wcmd_version_info_t *)skb->data;

	set_bit(CE_WMI_UPDATE, &vif->flags);

	ret = ath6kl_wmi_cmd_send(vif->ar->wmi, vif->fw_vif_idx, skb,
			WMI_GET_CUSTOM_VERSION_INFO, NO_SYNC_WMIFLAG);

	left = wait_event_interruptible_timeout(vif->ar->event_wq,
			!test_bit(CE_WMI_UPDATE, &vif->flags), WMI_TIMEOUT);
	up(&vif->ar->sem);

	if (left == 0)
		return -ETIMEDOUT;

	/* get tx result */
	memcpy(val , &version_info_private,
	sizeof(struct athcfg_wcmd_version_info_t));
	return ret;
}
#define PHY_BB_POWERTX_RATE1_ADDRESS		0x0000a3c0
#define PHY_BB_POWERTX_RATE2_ADDRESS		0x0000a3c4
#define PHY_BB_POWERTX_RATE3_ADDRESS		0x0000a3c8
#define PHY_BB_POWERTX_RATE4_ADDRESS		0x0000a3cc
#define PHY_BB_POWERTX_RATE5_ADDRESS		0x0000a3d0
#define PHY_BB_POWERTX_RATE6_ADDRESS		0x0000a3d4
#define PHY_BB_POWERTX_RATE7_ADDRESS		0x0000a3d8
#define PHY_BB_POWERTX_RATE8_ADDRESS		0x0000a3dc
#define PHY_BB_POWERTX_RATE9_ADDRESS		0x0000a3e0
#define PHY_BB_POWERTX_RATE10_ADDRESS		0x0000a3e4
#define PHY_BB_POWERTX_RATE11_ADDRESS		0x0000a3e8
#define PHY_BB_POWERTX_RATE12_ADDRESS		0x0000a3ec
/* 6 (LSB), 9, 12, 18 (MSB) */
#define AR_PHY_POWER_TX_RATE1   PHY_BB_POWERTX_RATE1_ADDRESS
/* 24 (LSB), 36, 48, 54 (MSB) */
#define AR_PHY_POWER_TX_RATE2   PHY_BB_POWERTX_RATE2_ADDRESS
/* 1L (LSB), reserved, 2L, 2S (MSB) */
#define AR_PHY_POWER_TX_RATE3   PHY_BB_POWERTX_RATE3_ADDRESS
/* 5.5L (LSB), 5.5S, 11L, 11S (MSB) */
#define AR_PHY_POWER_TX_RATE4   PHY_BB_POWERTX_RATE4_ADDRESS
/*
 * Write the HT20 power per rate set
 * Use MCS 0 target power for MCS 0/8
 * Use MCS 1 target power for MCS 1/2/3/9/10/11
 */
/* 0 (LSB),1,4,5 (MSB) */
#define AR_PHY_POWER_TX_RATE5   PHY_BB_POWERTX_RATE5_ADDRESS
/* 6 (LSB), 7, 12, 13 (MSB) */
#define AR_PHY_POWER_TX_RATE6   PHY_BB_POWERTX_RATE6_ADDRESS
/*
 * Write the HT40 power per rate set
 * Use MCS 0 target power for MCS 0/8
 * Use MCS 1 target power for MCS 1/2/3/9/10/11
 */
/* 0 (LSB),1,4,5 (MSB) */
#define AR_PHY_POWER_TX_RATE7   PHY_BB_POWERTX_RATE7_ADDRESS
/* 6 (LSB), 7, 12, 13 (MSB) */
#define AR_PHY_POWER_TX_RATE8   PHY_BB_POWERTX_RATE8_ADDRESS
/* Write the Dup/Ext 40 power per rate set */
#define AR_PHY_POWER_TX_RATE9   PHY_BB_POWERTX_RATE9_ADDRESS
/* 14 (LSB), 15 */
#define AR_PHY_POWER_TX_RATE10   PHY_BB_POWERTX_RATE10_ADDRESS
/* 14 (LSB), 15 */
#define AR_PHY_POWER_TX_RATE12   PHY_BB_POWERTX_RATE12_ADDRESS


static int _OS_REG_READ(struct ath6kl_vif *vif, int reg_addr)
{
	struct sk_buff *skb;
	struct wmi_reg_cmd *cmd;
	int left;
	int reg_val;
	int ret;

	if (down_interruptible(&vif->ar->sem))
		return -EBUSY;
	skb = ath6kl_wmi_get_new_buf(sizeof(*cmd));
	if (!skb) {
		up(&vif->ar->sem);
		return -ENOMEM;
	}

	cmd = (struct wmi_reg_cmd *) skb->data;

	cmd->addr = cpu_to_le32(reg_addr);
	set_bit(CE_WMI_UPDATE, &vif->flags);
	ret = ath6kl_wmi_cmd_send(vif->ar->wmi, vif->fw_vif_idx, skb,
			WMI_GET_CUSTOM_REG, NO_SYNC_WMIFLAG);

	left = wait_event_interruptible_timeout(vif->ar->event_wq,
			!test_bit(CE_WMI_UPDATE, &vif->flags), WMI_TIMEOUT);
	reg_val = val_private;
	up(&vif->ar->sem);
	if (left == 0)
		return -ETIMEDOUT;
	return reg_val;
}

#define OS_REG_READ(reg) _OS_REG_READ(vif, (reg | 0x30000000))
int ath6kl_wmi_get_customer_txpow_cmd(struct ath6kl_vif *vif,
			struct athcfg_wcmd_txpower_t *val)
{
	/* use reg read to get txpower data */
	int i;
	char *pRatesPower = (char *)val;
	for (i = 0; i < 4; i++)
		pRatesPower[i] = (u_int8_t)
		(OS_REG_READ(AR_PHY_POWER_TX_RATE1) >> (8*(i-0)));
	for (i = 4; i < 8; i++)
		pRatesPower[i] = (u_int8_t)
		(OS_REG_READ(AR_PHY_POWER_TX_RATE2) >> (8*(i-4)));
	for (i = 8; i < 12; i++) {
		if (i == 8)
			pRatesPower[i] = (u_int8_t)
			(OS_REG_READ(AR_PHY_POWER_TX_RATE3) >> (8*(i-8)));
		if (i == 9)
			pRatesPower[i] = (u_int8_t)
			(OS_REG_READ(AR_PHY_POWER_TX_RATE3) >> (8*(10-8)));
		if (i == 10)
			pRatesPower[i] = (u_int8_t)
			(OS_REG_READ(AR_PHY_POWER_TX_RATE3) >> (8*(11-8)));
		if (i == 11)
			pRatesPower[i] = (u_int8_t)
			(OS_REG_READ(AR_PHY_POWER_TX_RATE4) >> (8*(12-12)));
	}
	for (i = 12; i < 16; i++) {
		if (i == 12)
			pRatesPower[i] = (u_int8_t)
			(OS_REG_READ(AR_PHY_POWER_TX_RATE4) >> (8*(13-12)));
		if (i == 13)
			pRatesPower[i] = (u_int8_t)
			(OS_REG_READ(AR_PHY_POWER_TX_RATE4) >> (8*(14-12)));
		if (i == 14)
			pRatesPower[i] = (u_int8_t)
			(OS_REG_READ(AR_PHY_POWER_TX_RATE4) >> (8*(15-12)));
	}
	/*HT20 MCS0 ~ MCS7*/
	for (i = 16; i < 20; i++) {
		if (i == 16)/* MCS0 */
			pRatesPower[i] = (u_int8_t)
			(OS_REG_READ(AR_PHY_POWER_TX_RATE5) >> (8*(16-16)));
		if (i == 17)/* MCS1 */
			pRatesPower[i] = (u_int8_t)
			(OS_REG_READ(AR_PHY_POWER_TX_RATE5) >> (8*(17-16)));
		if (i == 18)/* MCS2 */
			pRatesPower[i] = (u_int8_t)
			(OS_REG_READ(AR_PHY_POWER_TX_RATE5) >> (8*(17-16)));
		if (i == 19)/* MCS3 */
			pRatesPower[i] = (u_int8_t)
			(OS_REG_READ(AR_PHY_POWER_TX_RATE5) >> (8*(17-16)));
	}

	for (i = 20; i < 24; i++) {
		if (i == 20)/* MCS4 */
			pRatesPower[i] = (u_int8_t)
			(OS_REG_READ(AR_PHY_POWER_TX_RATE5) >> (8*(18-16)));
		if (i == 21)/* MCS5 */
			pRatesPower[i] = (u_int8_t)
			(OS_REG_READ(AR_PHY_POWER_TX_RATE5) >> (8*(19-16)));
		if (i == 22)/* MCS6 */
			pRatesPower[i] = (u_int8_t)
			(OS_REG_READ(AR_PHY_POWER_TX_RATE6) >> (8*(20-20)));
		if (i == 23)/* MCS7 */
			pRatesPower[i] = (u_int8_t)
			(OS_REG_READ(AR_PHY_POWER_TX_RATE6) >> (8*(21-20)));
	}

	/*HT40 MCS0 ~ MCS7*/
	for (i = 24; i < 28; i++) {
		if (i == 24)/* MCS0 8 */
			pRatesPower[i] = (u_int8_t)
			(OS_REG_READ(AR_PHY_POWER_TX_RATE7) >> (8*(24-24)));
		if (i == 25)/* MCS1 */
			pRatesPower[i] = (u_int8_t)
			(OS_REG_READ(AR_PHY_POWER_TX_RATE7) >> (8*(25-24)));
		if (i == 26)/* MCS2 */
			pRatesPower[i] = (u_int8_t)
			(OS_REG_READ(AR_PHY_POWER_TX_RATE7) >> (8*(25-24)));
		if (i == 27)/* MCS3 */
			pRatesPower[i] = (u_int8_t)
			(OS_REG_READ(AR_PHY_POWER_TX_RATE7) >> (8*(25-24)));
	}
	for (i = 28; i < 32; i++) {
		if (i == 28)/* MCS4 */
			pRatesPower[i] = (u_int8_t)
			(OS_REG_READ(AR_PHY_POWER_TX_RATE7) >> (8*(26-24)));
		if (i == 29)/* MCS5 */
			pRatesPower[i] = (u_int8_t)
			(OS_REG_READ(AR_PHY_POWER_TX_RATE7) >> (8*(27-24)));
		if (i == 30)/* MCS6 */
			pRatesPower[i] = (u_int8_t)
			(OS_REG_READ(AR_PHY_POWER_TX_RATE8) >> (8*(28-28)));
		if (i == 31)/* MCS7 */
			pRatesPower[i] = (u_int8_t)
			(OS_REG_READ(AR_PHY_POWER_TX_RATE8) >> (8*(29-28)));
	}

	/*HT20 MCS8 ~ MCS15*/
	/* 0/8/16 (LSB), 1-3/9-11/17-19, 4, 5 (MSB) */
	for (i = 32; i < 36; i++) {
		if (i == 32)/* MCS8 */
			pRatesPower[i] = (u_int8_t)
			(OS_REG_READ(AR_PHY_POWER_TX_RATE5) >> (8*(32-32)));
		if (i == 33)/* MCS9 */
			pRatesPower[i] = (u_int8_t)
			(OS_REG_READ(AR_PHY_POWER_TX_RATE5) >> (8*(33-32)));
		if (i == 34)/* MCS10 */
			pRatesPower[i] = (u_int8_t)
			(OS_REG_READ(AR_PHY_POWER_TX_RATE5) >> (8*(33-32)));
		if (i == 35)/* MCS11 */
			pRatesPower[i] = (u_int8_t)
			(OS_REG_READ(AR_PHY_POWER_TX_RATE5) >> (8*(33-32)));
	}
	/* 6 (LSB), 7, 12, 13 (MSB) */
	/* 14 (LSB), 15, 20, 21 */
	for (i = 36; i < 40; i++) {
		if (i == 36)/* MCS12 */
			pRatesPower[i] = (u_int8_t)
			(OS_REG_READ(AR_PHY_POWER_TX_RATE6) >> (8*(38-36)));
		if (i == 37)/* MCS13 */
			pRatesPower[i] = (u_int8_t)
			(OS_REG_READ(AR_PHY_POWER_TX_RATE6) >> (8*(39-36)));
		if (i == 38)/* MCS14 */
			pRatesPower[i] = (u_int8_t)
			(OS_REG_READ(AR_PHY_POWER_TX_RATE10) >> (8*(36-36)));
		if (i == 39)/* MCS15 */
			pRatesPower[i] = (u_int8_t)
			(OS_REG_READ(AR_PHY_POWER_TX_RATE10) >> (8*(37-36)));
	}
	/*HT40 MCS8 ~ MCS15*/
	for (i = 40; i < 44; i++) {
		if (i == 40)/* MCS8 */
			pRatesPower[i] = (u_int8_t)
			(OS_REG_READ(AR_PHY_POWER_TX_RATE7) >> (8*(40-40)));
		if (i == 41)/* MCS9 */
			pRatesPower[i] = (u_int8_t)
			(OS_REG_READ(AR_PHY_POWER_TX_RATE7) >> (8*(41-40)));
		if (i == 42)/* MCS10 */
			pRatesPower[i] = (u_int8_t)
			(OS_REG_READ(AR_PHY_POWER_TX_RATE7) >> (8*(41-40)));
		if (i == 43)/* MCS11 */
			pRatesPower[i] = (u_int8_t)
			(OS_REG_READ(AR_PHY_POWER_TX_RATE7) >> (8*(41-40)));
	}
	for (i = 44; i < 48; i++) {
		if (i == 44)/* MCS12 */
			pRatesPower[i] = (u_int8_t)
			(OS_REG_READ(AR_PHY_POWER_TX_RATE8) >> (8*(46-44)));
		if (i == 45)/* MCS13 */
			pRatesPower[i] = (u_int8_t)
			(OS_REG_READ(AR_PHY_POWER_TX_RATE8) >> (8*(47-44)));
		if (i == 46)/* MCS14 */
			pRatesPower[i] = (u_int8_t)
			(OS_REG_READ(AR_PHY_POWER_TX_RATE12) >> (8*(44-44)));
		if (i == 47)/* MCS15 */
			pRatesPower[i] = (u_int8_t)
			(OS_REG_READ(AR_PHY_POWER_TX_RATE12) >> (8*(45-44)));
	}
	return 0;
}


int ath6kl_wmi_get_customer_stainfo_cmd(struct ath6kl_vif *vif,
			struct athcfg_wcmd_sta_t *val)
{
	struct sk_buff *skb;
	struct athcfg_wcmd_sta_t *cmd;
	int ret;
	int left;

	if (down_interruptible(&vif->ar->sem))
		return -EBUSY;
	skb = ath6kl_wmi_get_new_buf(sizeof(*cmd));
	if (!skb) {
		up(&vif->ar->sem);
		return -ENOMEM;
	}
	cmd = (struct athcfg_wcmd_sta_t *) skb->data;
	set_bit(CE_WMI_UPDATE, &vif->flags);
	ret = ath6kl_wmi_cmd_send(vif->ar->wmi, vif->fw_vif_idx, skb,
			WMI_GET_CUSTOM_STAINFO, NO_SYNC_WMIFLAG);

	left = wait_event_interruptible_timeout(vif->ar->event_wq,
				!test_bit(CE_WMI_UPDATE, &vif->flags),
				WMI_TIMEOUT);

	if (left == 0) {
		up(&vif->ar->sem);
		return -ETIMEDOUT;
	}
	if ((stainfo_private.flags & ATHCFG_WCMD_STAINFO_LINKUP) == ATHCFG_WCMD_STAINFO_LINKUP) {
		/* Get beacon RSSI */
		if (vif->nw_type != AP_NETWORK) {
			ret = ath6kl_wmi_get_stats_cmd(vif->ar->wmi,
							vif->fw_vif_idx);

			if (ret != 0) {
				up(&vif->ar->sem);
				return -EIO;
			}

			left = wait_event_interruptible_timeout(
				vif->ar->event_wq,
				!test_bit(STATS_UPDATE_PEND,
				&vif->flags), WMI_TIMEOUT);

			if (left == 0) {
				up(&vif->ar->sem);
				return -ETIMEDOUT;
			} else if (left < 0) {
				up(&vif->ar->sem);
				return left;
			}
			if (vif->target_stats.cs_rssi < -96)
				stainfo_private.rx_rssi_beacon = 0;
			else
				stainfo_private.rx_rssi_beacon =
				(a_uint8_t)vif->target_stats.cs_rssi+96;
		} else
			stainfo_private.rx_rssi_beacon = 0;

		/* data rx rssi */
		if (vif->nw_type != AP_NETWORK) {
			struct ath6kl_sta *conn = NULL;
			conn = &vif->sta_list[0];
			stainfo_private.rx_rssi =
			(a_uint8_t)ATH_EP_RND(conn->avg_data_rssi,
						HAL_RSSI_EP_MULTIPLIER);
			if (stainfo_private.rx_rssi == 0)
				stainfo_private.rx_rssi =
				stainfo_private.rx_rssi_beacon;
		} else if (vif->sta_list_index != 0) {
			/* search first connected sta */
			struct ath6kl_sta *conn = NULL;
			int ctr;
			int avg_data_rssi[AP_MAX_NUM_STA];
			int avg_data_rssi_for_ap = 0, avg_rssi_cnt = 0;
			for (ctr = 0; ctr < AP_MAX_NUM_STA; ctr++) {
				if ((vif->sta_list_index & (0x1 << ctr)) != 0) {
					conn = &vif->sta_list[ctr];
					avg_data_rssi[ctr] = ATH_EP_RND(
						conn->avg_data_rssi,
						HAL_RSSI_EP_MULTIPLIER);
					if (avg_data_rssi[ctr] != 0) {
						avg_rssi_cnt++;
						avg_data_rssi_for_ap +=
							avg_data_rssi[ctr];
					}
					/* only get the first one,
					for customer request */
					break;
				}
			}
			if (avg_rssi_cnt != 0)
				stainfo_private.rx_rssi =
				avg_data_rssi_for_ap/avg_rssi_cnt;
			else
				stainfo_private.rx_rssi = 0;
		} else {
			stainfo_private.rx_rssi = 0;
		}
	} else {
		stainfo_private.rx_rssi_beacon = 0;
		stainfo_private.rx_rssi = 0;
	}
	up(&vif->ar->sem);

	memcpy(val, &stainfo_private, sizeof(struct athcfg_wcmd_sta_t));
	return ret;
}


int ath6kl_wmi_get_customer_mode_cmd(struct ath6kl_vif *vif,
			enum athcfg_wcmd_phymode_t *val)
{
	struct sk_buff *skb;
	struct athcfg_wcmd_sta_t *cmd;
	int ret;
	int left;

	if (down_interruptible(&vif->ar->sem))
		return -EBUSY;
	skb = ath6kl_wmi_get_new_buf(sizeof(*cmd));
	if (!skb) {
		up(&vif->ar->sem);
		return -ENOMEM;
	}

	cmd = (struct athcfg_wcmd_sta_t *)skb->data;

	set_bit(CE_WMI_UPDATE, &vif->flags);

	ret = ath6kl_wmi_cmd_send(vif->ar->wmi, vif->fw_vif_idx, skb,
			WMI_GET_CUSTOM_STAINFO, NO_SYNC_WMIFLAG);

	left = wait_event_interruptible_timeout(vif->ar->event_wq,
			!test_bit(CE_WMI_UPDATE, &vif->flags), WMI_TIMEOUT);
	up(&vif->ar->sem);
	if (left == 0)
		return -ETIMEDOUT;

	if (test_bit(CONNECTED, &vif->flags))
		memcpy(val, &stainfo_private.phymode,
			sizeof(enum athcfg_wcmd_phymode_t));
	else
		memset(val, 0x0, sizeof(enum athcfg_wcmd_phymode_t));

	return ret;
}


int ath6kl_wmi_get_customer_stats_cmd(struct ath6kl_vif *vif,
			struct athcfg_wcmd_stats_t *val)
{
	int ret;
	int left;

	if (down_interruptible(&vif->ar->sem))
		return -EBUSY;
	set_bit(STATS_UPDATE_PEND, &vif->flags);

	ret = ath6kl_wmi_get_stats_cmd(vif->ar->wmi, vif->fw_vif_idx);

	if (ret != 0) {
		up(&vif->ar->sem);
		return -EIO;
	}

	left = wait_event_interruptible_timeout(vif->ar->event_wq,
			!test_bit(STATS_UPDATE_PEND, &vif->flags), WMI_TIMEOUT);
	up(&vif->ar->sem);

	if (left == 0)
		return -ETIMEDOUT;
	else if (left < 0)
		return left;

	val->txPackets = vif->target_stats.tx_pkt;
	/* retry count,not include first tx */
	val->txRetry = vif->target_stats.tx_fail_cnt;
	val->txAggrRetry = 0;
	val->txAggrSubRetry = 0;
	val->txAggrExcessiveRetry = 0;
	val->txSubRetry = 0;

	return ret;
}

int ath6kl_wmi_set_suspend_cmd(struct ath6kl_vif *vif,
			struct athcfg_wcmd_suspend_t *val)
{
	int ret = 0;

	if (down_interruptible(&vif->ar->sem))
		return -EBUSY;
#ifdef CONFIG_PM
	printk(KERN_DEBUG "%s[%d]val->val=%d\n\r",
		__func__, __LINE__, val->val);
	if (val->val == 1) /* enetr suspend mode */
		cfg80211_suspend_issue(vif->ndev, 1);
	else
		cfg80211_suspend_issue(vif->ndev, 0);
#endif
	up(&vif->ar->sem);
	return ret;
}

#if defined(CE_CUSTOM_1)
int ath6kl_wmi_set_customer_widi_cmd(struct ath6kl_vif *vif, u8 val)
{
	struct sk_buff *skb;
	u8 *widi_enable;
	int ret;

	skb = ath6kl_wmi_get_new_buf(sizeof(u8));
	if (!skb)
		return -ENOMEM;

	widi_enable = (u8 *)skb->data;
	*widi_enable = val;

	if (htcoex) {
		if (val) {
			/* Disable htcoex periodic scan in WiDi mode */
			ath6kl_htcoex_config(vif, 0,
					ATH6KL_HTCOEX_RATE_ROLLBACK);
		} else {
			/* Enable htcoex periodic scan after leaving WiDi
			mode */
			ath6kl_htcoex_config(vif, ATH6KL_HTCOEX_SCAN_PERIOD,
					ATH6KL_HTCOEX_RATE_ROLLBACK);
		}
	}

	ret = ath6kl_wmi_cmd_send(vif->ar->wmi, vif->fw_vif_idx, skb,
				WMI_SET_CUSTOM_WIDI, NO_SYNC_WMIFLAG);

	return ret;
}

int ath6kl_wmi_get_customer_widi_cmd(struct ath6kl_vif *vif, u8 *val)
{
	struct sk_buff *skb;
	int ret;
	int left;

	if (down_interruptible(&vif->ar->sem))
		return -EBUSY;
	skb = ath6kl_wmi_get_new_buf(sizeof(u8));
	if (!skb) {
		up(&vif->ar->sem);
		return -ENOMEM;
	}

	set_bit(CE_WMI_UPDATE, &vif->flags);

	ret = ath6kl_wmi_cmd_send(vif->ar->wmi, vif->fw_vif_idx, skb,
			WMI_GET_CUSTOM_WIDI, NO_SYNC_WMIFLAG);

	left = wait_event_interruptible_timeout(vif->ar->event_wq,
			!test_bit(CE_WMI_UPDATE, &vif->flags), WMI_TIMEOUT);
	up(&vif->ar->sem);

	if (left == 0)
		return -ETIMEDOUT;

    /* get result */
	*val = widi_enable_private;
	return ret;
}
#endif

int ath6kl_wmi_get_chan_list(struct ath6kl_vif *vif,
			struct athcfg_wcmd_chan_list_t *val)
{
	int ret = 0;
	struct athcfg_wcmd_chan_list_t *val_temp = val;
	struct ath6kl *ar = vif->ar;
	struct wiphy *wiphy = ar->wiphy;
	enum ieee80211_band band;
	int i;

	if (down_interruptible(&vif->ar->sem))
		return -EBUSY;

	for (band = 0; band < IEEE80211_NUM_BANDS; band++) {
		if (!wiphy->bands[band])
			continue;
		if(band == IEEE80211_BAND_2GHZ) {			
			val_temp->n_2ghz_channels = wiphy->bands[band]->n_channels;
		}
		if(band == IEEE80211_BAND_5GHZ) {			
			val_temp->n_5ghz_channels = wiphy->bands[band]->n_channels;
		}
		for (i = 0; i < wiphy->bands[band]->n_channels; i++) {
			if(band == IEEE80211_BAND_2GHZ) {
				val_temp->ce_ath6kl_2ghz_a_channels[i] = wiphy->bands[band]->channels[i];
			}
			if(band == IEEE80211_BAND_5GHZ) {
				val_temp->ce_ath6kl_5ghz_a_channels[i] = wiphy->bands[band]->channels[i];
			}
		}
	}
	
	up(&vif->ar->sem);
	return ret;
}

int ath6kl_wmi_set_csa(struct ath6kl_vif *vif,
			struct athcfg_wcmd_csa_t *val)
{
	int ret;
	if (down_interruptible(&vif->ar->sem))
		return -EBUSY;
	ret = ath6kl_wmi_set_seamless_mcc_scc_switch_freq_cmd(vif->ar->wmi, val->freq);
	up(&vif->ar->sem);
	return ret;
}

/* Match rate index with rate table so that it can be used for TPC */
typedef enum {
    CE_RATE_1           = 0,
#define CE_RATE_1L     CE_RATE_1
    CE_RATE_2           = 1,
#define CE_RATE_2S     CE_RATE_2
#define CE_RATE_2L     CE_RATE_2
    CE_RATE_5_5        = 2,
#define CE_RATE_5_5S   CE_RATE_5_5
#define CE_RATE_5_5L   CE_RATE_5_5
    CE_RATE_11         = 3,
#define CE_RATE_11S    CE_RATE_11
#define CE_RATE_11L    CE_RATE_11
    CE_RATE_6           = 4,
    CE_RATE_9           = 5,
    CE_RATE_12          = 6,
    CE_RATE_18          = 7,
    CE_RATE_24          = 8,
    CE_RATE_36          = 9,
    CE_RATE_48          = 10,
    CE_RATE_54          = 11,
    CE_RATE_HT20_0      = 12,
    CE_RATE_HT20_1      = 13, 
    CE_RATE_HT20_2      = 14,
    CE_RATE_HT20_3      = 15,
    CE_RATE_HT20_4      = 16,
    CE_RATE_HT20_5      = 17,
    CE_RATE_HT20_6      = 18,
    CE_RATE_HT20_7      = 19,
    CE_RATE_HT20_8      = 20,
    CE_RATE_HT20_9      = 21,
    CE_RATE_HT20_10     = 22,
    CE_RATE_HT20_11     = 23,
    CE_RATE_HT20_12     = 24,
    CE_RATE_HT20_13     = 25,
    CE_RATE_HT20_14     = 26,
    CE_RATE_HT20_15     = 27,
    CE_RATE_HT40_0      = 28,
    CE_RATE_HT40_1      = 29, 
    CE_RATE_HT40_2      = 30,
    CE_RATE_HT40_3      = 31,
    CE_RATE_HT40_4      = 32,
    CE_RATE_HT40_5      = 33,
    CE_RATE_HT40_6      = 34,
    CE_RATE_HT40_7      = 35,
    CE_RATE_HT40_8      = 36,
    CE_RATE_HT40_9      = 37,
    CE_RATE_HT40_10     = 38,
    CE_RATE_HT40_11     = 39,
    CE_RATE_HT40_12     = 40,
    CE_RATE_HT40_13     = 41,
    CE_RATE_HT40_14     = 42,
    CE_RATE_HT40_15     = 43,
} CE_RATES;
int ath6kl_wmi_get_ctl_list(struct ath6kl_vif *vif,
			struct athcfg_wcmd_ctl_list_t *val)
{
	struct sk_buff *skb;
	struct wmi_ctl_cmd *cmd;
	int ret = 0;
	int left;
	struct ath6kl *ar = vif->ar;
	struct wiphy *wiphy = ar->wiphy;
	struct athcfg_wcmd_ctl_list_t *val_temp = val;
	enum ieee80211_band band;
	int i,j;
	for (band = 0; band < IEEE80211_NUM_BANDS; band++) {
		if (!wiphy->bands[band])
			continue;
		if(band == IEEE80211_BAND_2GHZ) {			
			val_temp->n_2ghz_channels = wiphy->bands[band]->n_channels;
		}
		if(band == IEEE80211_BAND_5GHZ) {			
			val_temp->n_5ghz_channels = wiphy->bands[band]->n_channels;
		}
		for (i = 0; i < wiphy->bands[band]->n_channels; i++) {
			if(band == IEEE80211_BAND_2GHZ) {
				val_temp->ce_ath6kl_2ghz_a_ctls[i].center_freq = wiphy->bands[band]->channels[i].center_freq;
				val_temp->ce_ath6kl_2ghz_a_ctls[i].flags = wiphy->bands[band]->channels[i].flags;
				val_temp->ce_ath6kl_2ghz_a_ctls[i].ch_idx = wiphy->bands[band]->channels[i].hw_value;
			}
			if(band == IEEE80211_BAND_5GHZ) {
				val_temp->ce_ath6kl_5ghz_a_ctls[i].center_freq = wiphy->bands[band]->channels[i].center_freq;			
				val_temp->ce_ath6kl_5ghz_a_ctls[i].flags = wiphy->bands[band]->channels[i].flags;	
				val_temp->ce_ath6kl_5ghz_a_ctls[i].ch_idx = wiphy->bands[band]->channels[i].hw_value;
			}
			if (wiphy->bands[band]->channels[i].flags & IEEE80211_CHAN_DISABLED)
				continue;
			if (down_interruptible(&vif->ar->sem))
				return -EBUSY;

			skb = ath6kl_wmi_get_new_buf(sizeof(*cmd));
			if (!skb) {
				up(&vif->ar->sem);
				return -ENOMEM;
			}
			
			cmd = (struct wmi_ctl_cmd *) skb->data;
			cmd->channel = wiphy->bands[band]->channels[i].center_freq;
			//printk("freq=%d,flags = 0x%x\n\r",wiphy->bands[band]->channels[i].center_freq,wiphy->bands[band]->channels[i].flags);
			if (IEEE80211_CHAN_NO_HT40 == (wiphy->bands[band]->channels[i].flags & (IEEE80211_CHAN_NO_HT40)))
				cmd->flags = 0;//not support HT40
			else if ((wiphy->bands[band]->channels[i].flags & IEEE80211_CHAN_NO_HT40MINUS)) {
				cmd->flags = 1;//HT40+
			} else if ((wiphy->bands[band]->channels[i].flags & IEEE80211_CHAN_NO_HT40PLUS)) {
				cmd->flags = 2;//HT40-
			}
			set_bit(CE_WMI_UPDATE, &vif->flags);
			ret = ath6kl_wmi_cmd_send(vif->ar->wmi, vif->fw_vif_idx, skb,
					WMI_GET_CTL, NO_SYNC_WMIFLAG);//this command will update txpower
			left = wait_event_interruptible_timeout(vif->ar->event_wq,
				!test_bit(CE_WMI_UPDATE, &vif->flags), WMI_TIMEOUT);
			up(&vif->ar->sem);
			if (left == 0)
				return -ETIMEDOUT;

			if(band == IEEE80211_BAND_2GHZ) {
				struct athcfg_wcmd_txpower_t txpower_val;
				memcpy(val_temp->ce_ath6kl_2ghz_a_ctls[i].txpower,ctl_private.txpower,
					sizeof(ctl_private.txpower));
				memcpy(val_temp->ce_ath6kl_2ghz_a_ctls[i].ctlmode,ctl_private.ctlmode,
					sizeof(ctl_private.ctlmode));
				ath6kl_wmi_get_customer_txpow_cmd(vif, &txpower_val);
				for(j=0;j< CE_RATE_MAX;j++) {
					if (j >= CE_RATE_HT40_8) {
						val_temp->ce_ath6kl_2ghz_a_ctls[i].cal_txpower[j] = (u16)txpower_val.txpowertable[40+(j-CE_RATE_HT40_8)];
					} else if (j >= CE_RATE_HT40_0) {
						val_temp->ce_ath6kl_2ghz_a_ctls[i].cal_txpower[j] = (u16)txpower_val.txpowertable[24+(j-CE_RATE_HT40_0)];
					} else if (j >= CE_RATE_HT20_8) {
						val_temp->ce_ath6kl_2ghz_a_ctls[i].cal_txpower[j] = (u16)txpower_val.txpowertable[32+(j-CE_RATE_HT20_8)];
					} else if (j >= CE_RATE_HT20_0) {
						val_temp->ce_ath6kl_2ghz_a_ctls[i].cal_txpower[j] = (u16)txpower_val.txpowertable[16+(j-CE_RATE_HT20_0)];
					} else if (j >= CE_RATE_6) {
						val_temp->ce_ath6kl_2ghz_a_ctls[i].cal_txpower[j] = (u16)txpower_val.txpowertable[0+(j-CE_RATE_6)];
					} else {
						switch(j) {
						case CE_RATE_1:
							val_temp->ce_ath6kl_2ghz_a_ctls[i].cal_txpower[j] = (u16)txpower_val.txpowertable[8];
							break;
						case CE_RATE_2:
							val_temp->ce_ath6kl_2ghz_a_ctls[i].cal_txpower[j] = (u16)txpower_val.txpowertable[10];
							break;
						case CE_RATE_5_5:
							val_temp->ce_ath6kl_2ghz_a_ctls[i].cal_txpower[j] = (u16)txpower_val.txpowertable[12];
							break;
						case CE_RATE_11:
							val_temp->ce_ath6kl_2ghz_a_ctls[i].cal_txpower[j] = (u16)txpower_val.txpowertable[14];
							break;
						}
					}
				}
			}
			if(band == IEEE80211_BAND_5GHZ) {
				struct athcfg_wcmd_txpower_t txpower_val;
				memcpy(val_temp->ce_ath6kl_5ghz_a_ctls[i].txpower,ctl_private.txpower,
					sizeof(ctl_private.txpower));
				memcpy(val_temp->ce_ath6kl_5ghz_a_ctls[i].ctlmode,ctl_private.ctlmode,
					sizeof(ctl_private.ctlmode));
				ath6kl_wmi_get_customer_txpow_cmd(vif, &txpower_val);
/*
8 dBm ~ 13dBm : target power = target power - 1dBm		-->7~12			(14~24)
13 dBm ~ 16 dBm : target power = target + 0.5 dBm		-->13.5~16.5  (27~33)
16dBm~ : target power = target + 1 dBm.					-->17		(34)
*/
				for(j=0;j< CE_RATE_MAX;j++) {
					if (j >= CE_RATE_HT40_8) {
						val_temp->ce_ath6kl_5ghz_a_ctls[i].cal_txpower[j] = (u16)txpower_val.txpowertable[40+(j-CE_RATE_HT40_8)];
					} else if (j >= CE_RATE_HT40_0) {
						val_temp->ce_ath6kl_5ghz_a_ctls[i].cal_txpower[j] = (u16)txpower_val.txpowertable[24+(j-CE_RATE_HT40_0)];						
					} else if (j >= CE_RATE_HT20_8) {
						val_temp->ce_ath6kl_5ghz_a_ctls[i].cal_txpower[j] = (u16)txpower_val.txpowertable[32+(j-CE_RATE_HT20_8)];					
					} else if (j >= CE_RATE_HT20_0) {
						val_temp->ce_ath6kl_5ghz_a_ctls[i].cal_txpower[j] = (u16)txpower_val.txpowertable[16+(j-CE_RATE_HT20_0)];					
					} else if (j >= CE_RATE_6) {
						val_temp->ce_ath6kl_5ghz_a_ctls[i].cal_txpower[j] = (u16)txpower_val.txpowertable[0+(j-CE_RATE_6)];						
					} else {
						switch(j) {
						case CE_RATE_1:
							val_temp->ce_ath6kl_5ghz_a_ctls[i].cal_txpower[j] = (u16)txpower_val.txpowertable[8];
							break;
						case CE_RATE_2:
							val_temp->ce_ath6kl_5ghz_a_ctls[i].cal_txpower[j] = (u16)txpower_val.txpowertable[10];
							break;
						case CE_RATE_5_5:
							val_temp->ce_ath6kl_5ghz_a_ctls[i].cal_txpower[j] = (u16)txpower_val.txpowertable[12];
							break;
						case CE_RATE_11:
							val_temp->ce_ath6kl_5ghz_a_ctls[i].cal_txpower[j] = (u16)txpower_val.txpowertable[14];
							break;
						}
					}
					if (j >= CE_RATE_6) {
						if (( val_temp->ce_ath6kl_5ghz_a_ctls[i].cal_txpower[j] < 24) && ( val_temp->ce_ath6kl_5ghz_a_ctls[i].cal_txpower[j] >= 14)) {
							val_temp->ce_ath6kl_5ghz_a_ctls[i].cal_txpower[j] = val_temp->ce_ath6kl_5ghz_a_ctls[i].cal_txpower[j] + 2;
						} else if (( val_temp->ce_ath6kl_5ghz_a_ctls[i].cal_txpower[j] < 33) && ( val_temp->ce_ath6kl_5ghz_a_ctls[i].cal_txpower[j] >= 27)) {
							val_temp->ce_ath6kl_5ghz_a_ctls[i].cal_txpower[j] = val_temp->ce_ath6kl_5ghz_a_ctls[i].cal_txpower[j] - 1;
						} else if(val_temp->ce_ath6kl_5ghz_a_ctls[i].cal_txpower[j] >= 34) {
							val_temp->ce_ath6kl_5ghz_a_ctls[i].cal_txpower[j] = val_temp->ce_ath6kl_5ghz_a_ctls[i].cal_txpower[j] -2;
						}		
					}
				}
			}
		}
	}	
	return ret;
}

static int
_ath6kl_athtst_wioctl(struct net_device *netdev, void *data)
{
	struct ath6kl_vif *vif = netdev_priv(netdev);
	struct athcfg_wcmd *req = NULL;
	unsigned long req_len = sizeof(struct athcfg_wcmd);
	unsigned long status = EIO;

	req = vmalloc(req_len);

	if (!req)
		return -ENOMEM;

	memset(req, 0, req_len);

	/* XXX: Optimize only copy the amount thats valid */
	if (copy_from_user(req, data, req_len))
		goto done;

	/* try to use exist WMI command to implement ATHTST command behavior */
	switch (req->iic_cmd) {
	case ATHCFG_WCMD_GET_DEV_PRODUCT_INFO:
		ath6kl_wmi_get_customer_product_info_cmd(vif,
					&req->iic_data.product_info);
		break;
	case ATHCFG_WCMD_GET_REG:
		ath6kl_wmi_get_customer_reg_cmd(vif, req->iic_data.reg.addr,
						&req->iic_data.reg.val);
		break;
	case ATHCFG_WCMD_SET_REG:
		ath6kl_wmi_set_customer_reg_cmd(vif, req->iic_data.reg.addr,
						req->iic_data.reg.val);
		break;
	case ATHCFG_WCMD_GET_TESTMODE:
		ath6kl_wmi_get_customer_testmode_cmd(vif,
						     &req->iic_data.testmode);
		break;
	case ATHCFG_WCMD_SET_TESTMODE:
		ath6kl_wmi_set_customer_testmode_cmd(vif,
						     &req->iic_data.testmode);
		break;
	case ATHCFG_WCMD_GET_SCANTIME:
		ath6kl_wmi_get_customer_scan_time_cmd(vif,
						      &req->iic_data.scantime);
		break;
	case ATHCFG_WCMD_GET_MODE: /* get it from stainfo command */
		ath6kl_wmi_get_customer_mode_cmd(vif, &req->iic_data.phymode);
		break;
	case ATHCFG_WCMD_GET_SCAN:
		ath6kl_wmi_get_customer_scan_cmd(vif, req->iic_data.scan);
		break;
	case ATHCFG_WCMD_SET_SCAN:
		ath6kl_wmi_set_customer_scan_cmd(vif);
		break;
	case ATHCFG_WCMD_GET_DEV_VERSION_INFO:
		ath6kl_wmi_get_customer_version_info_cmd(vif,
						&req->iic_data.version_info);
		break;
	case ATHCFG_WCMD_GET_TX_POWER:
		ath6kl_wmi_get_customer_txpow_cmd(vif, &req->iic_data.txpower);
		break;
	case ATHCFG_WCMD_GET_STAINFO:
		ath6kl_wmi_get_customer_stainfo_cmd(vif,
						  &req->iic_data.station_info);
		break;
	case ATHCFG_WCMD_GET_STATS:
		ath6kl_wmi_get_customer_stats_cmd(vif, &req->iic_data.stats);
		break;
	case ATHCFG_WCMD_SET_SUSPEND:
		ath6kl_wmi_set_suspend_cmd(vif, &req->iic_data.suspend);
		break;
#if defined(CE_CUSTOM_1)
	case ATHCFG_WCMD_SET_WIDI:
		ath6kl_wmi_set_customer_widi_cmd(vif,
						 req->iic_data.widi_enable);
		break;
	case ATHCFG_WCMD_GET_WIDI:
		ath6kl_wmi_get_customer_widi_cmd(vif,
						 &req->iic_data.widi_enable);
		break;
#endif
	case ATHCFG_WCMD_GET_CHAN_LIST:
		ath6kl_wmi_get_chan_list(vif, &req->iic_data.chans);
		break;
	case ATHCFG_WCMD_SET_CSA:
		ath6kl_wmi_set_csa(vif, &req->iic_data.csa);
		break;
	case ATHCFG_WCMD_GET_CTL_LIST:
		ath6kl_wmi_get_ctl_list(vif, &req->iic_data.ctls);
		break;
	default:
		goto done;
	}
	status = 0;

	/* XXX: Optimize only copy the amount thats valid */
	if (copy_to_user(data, req, req_len))
		status = -EIO;

done:
	vfree(req);

	return status;
}


static int
_ath6kl_app_ioctl(struct net_device *netdev, void *data)
{
	struct ath6kl_vif *vif = netdev_priv(netdev);
	struct athr_cmd *req = NULL;
	unsigned int req_len = sizeof(struct athr_cmd);
	unsigned long status = 0;

	req = vmalloc(req_len);
	if (!req)
		return -ENOMEM;

	memset(req, 0, req_len);
	if (copy_from_user(req, data, req_len)) {
		status = -EFAULT;
		goto done;
	}

	switch (req->cmd) {
	case ATHR_WLAN_SCAN_BAND:
		if (req->data[0] > ATHR_CMD_SCANBAND_5G) {
			vif->scan_band = ATHR_CMD_SCANBAND_CHAN_ONLY;
			vif->scan_chan = req->data[0];
			vif->scanband_type = SCANBAND_TYPE_CHAN_ONLY;
			vif->scanband_chan = vif->scan_chan;
		} else {
			vif->scan_band = req->data[0];
			vif->scan_chan = 0;
			vif->scanband_type = vif->scan_band;
		}
		printk(KERN_DEBUG "## req->cmd = %d, req->data[0] = %d\n",
			req->cmd, req->data[0]);
		printk(KERN_DEBUG "## vif->scan_band = %d, vif->scan_chan = %d\n",
			vif->scan_band, vif->scan_chan);
		break;
	case ATHR_WLAN_FIND_BEST_CHANNEL: {
		/* don't update it if single channel scan. */
		if (vif->scan_band == ATHR_CMD_SCANBAND_CHAN_ONLY) {
			status = -EFAULT;
			break;
		}
		ath6kl_acs_get_info(vif, req->data);
		if (copy_to_user(data, req, req_len))
			status = -EFAULT;
		break;
	}
	case ATHR_WLAN_ACS_LIST: {
		if (req->data[0] == ATHR_CMD_ACS_5G_LIST) {
			memcpy(vif->p2p_acs_5g_list, req->list, sizeof(vif->p2p_acs_5g_list));
		} else { /* ATHR_CMD_ACS_2G_LIST */
			memcpy(vif->p2p_acs_2g_list, req->list, sizeof(vif->p2p_acs_2g_list));
		}
		break;
	}
	default:
		goto done;
	}

done:
	vfree(req);
	return status;
}

int ath6kl_ce_ioctl(struct net_device *netdev, struct ifreq *ifr, int cmd)
{
	int error = EINVAL;

	switch (cmd) {
#ifdef ATHTST_SUPPORT
	case ATHCFG_WCMD_IOCTL:
		error = _ath6kl_athtst_wioctl(netdev, ifr->ifr_data);
		break;
#endif
#ifdef TX99_SUPPORT
	case SIOCIOCTLTX99:
		error = ath6kl_wmi_tx99_cmd(netdev, ifr->ifr_data);
		break;
#endif
#ifdef ATHTST_SUPPORT
	case SIOCIOCTLAPP:
		error = _ath6kl_app_ioctl(netdev, ifr->ifr_data);
		break;
#endif
#ifdef ACL_SUPPORT
	case IEEE80211_IOCTL_SETPARAM:
	{
		struct iwreq *iwr;
		iwr = (struct iwreq *)ifr;
		error =  ath6kl_wmi_set_param_cmd(netdev, (void *)iwr);
		break;
	}
	case IEEE80211_IOCTL_SETMLME:
	{
		struct iwreq *iwr;
		iwr = (struct iwreq *)ifr;
		error =  ath6kl_wmi_set_mlme_cmd(netdev, (void *)iwr);
		break;
	}
	case IEEE80211_IOCTL_GETPARAM:
	{
		struct iwreq *iwr;
		iwr = (struct iwreq *)ifr;
		error =  ath6kl_wmi_get_param_cmd(netdev, (void *)iwr);
		break;
	}
	case IEEE80211_IOCTL_ADDMAC:
	{
		struct iwreq *iwr;
		iwr = (struct iwreq *)ifr;
		error =  ath6kl_wmi_addmac_cmd(netdev, (void *)iwr);
		break;
	}
	case IEEE80211_IOCTL_DELMAC:
	{
		struct iwreq *iwr;
		iwr = (struct iwreq *)ifr;
		error =  ath6kl_wmi_delmac_cmd(netdev, (void *)iwr);
		break;
	}
	case IEEE80211_IOCTL_GET_MACADDR:
	{
		struct iwreq *iwr;
		iwr = (struct iwreq *)ifr;
		error =  ath6kl_wmi_getmac_cmd(netdev, (void *)iwr);
		break;
	}
#endif
	case IEEE80211_IOCTL_KICKMAC:
	{
		struct iwreq *iwr = (struct iwreq *)ifr;
		struct ath6kl_vif *vif = netdev_priv(netdev);
		struct sockaddr *sa;
		char     req_content[16];
		char     *req = &req_content[0];

		memcpy(req, iwr->u.name, 16);
		sa = (struct sockaddr *)req;
		if (sa->sa_family != ARPHRD_ETHER)
			break;
		error =  ath6kl_wmi_ap_set_mlme(vif->ar->wmi, vif->fw_vif_idx,
			WMI_AP_DISASSOC, sa->sa_data, WLAN_REASON_UNSPECIFIED);
		break;
	}
	default:
#if !defined(CE_CUSTOM_0) && !defined(CE_CUSTOM_1)
		printk(KERN_DEBUG "%s[%d]Not support\n\r", __func__, __LINE__);
#endif
		break ;
	}

	return error;
}
#endif
