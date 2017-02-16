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

/*
 * This file contains the definitions of the WMI protocol specified in the
 * Wireless Module Interface (WMI).  It includes definitions of all the
 * commands and events. Commands are messages from the host to the WM.
 * Events and Replies are messages from the WM to the host.
 */

#ifndef CE_ATHTST_H
#define CE_ATHTST_H

#include <linux/ieee80211.h>

#include "htc.h"
#include "wmi.h"
#include "ioctl_vendor_ce.h"

struct wmi_reg_cmd {
	u32 addr;
	u32 val;
} __packed;

#define ATHCFG_WCMD_ADDR_LEN             6
struct WMI_CUSTOMER_STAINFO_STRUCT {
/* #define ATHCFG_WCMD_STAINFO_LINKUP        0x1 */
/* #define ATHCFG_WCMD_STAINFO_SHORTGI       0x2 */
	u32	flags;  /* Flags */
	enum	athcfg_wcmd_phymode_t  phymode;  /* STA Connection Type */
	u8	bssid[ATHCFG_WCMD_ADDR_LEN];/* STA Current BSSID */
	u32	assoc_time;  /* STA association time */
	u32	wpa_4way_handshake_time; /* STA 4-way time */
	u32	wpa_2way_handshake_time; /* STA 2-way time */
	u32	rx_rate_kbps; /* STA latest RX Rate (Kbps) */
/* #define ATHCFG_WCMD_STAINFO_MCS_NULL      0xff */
	u8	rx_rate_mcs; /* STA latest RX Rate MCS (11n) */
	u32	tx_rate_kbps; /* STA latest TX Rate (Kbps) */
	u8	rx_rssi; /* RSSI of all received frames */
	u8	rx_rssi_beacon; /* RSSI of Beacon */
    /* TBD : others information */
} __packed;

/**
 * @brief get txpower-info
 */
struct WMI_CUSTOMER_TXPOW_STRUCT {
/* #define ATHCFG_WCMD_TX_POWER_TABLE_SIZE 32 */
#define ATHCFG_WCMD_TX_POWER_TABLE_SIZE 48/* for ar6002 */
	u8	txpowertable[ATHCFG_WCMD_TX_POWER_TABLE_SIZE];
} __packed;

/**
 * @brief get version-info
 */
struct WMI_CUSTOMER_VERSION_INFO_STRUCT {
	u32	sw_version; /*SW version*/
	u8	driver[32]; /* Driver Short Name */
} __packed;

/**
 * @brief  set/get testmode-info
 */
struct WMI_CUSTOMER_TESTMODE_STRUCT {
	u8	bssid[ATHCFG_WCMD_ADDR_LEN];
	u32	chan;         /* ChanID */

#define ATHCFG_WCMD_TESTMODE_CHAN     0x1
#define ATHCFG_WCMD_TESTMODE_BSSID    0x2
#define ATHCFG_WCMD_TESTMODE_RX       0x3
#define ATHCFG_WCMD_TESTMODE_RESULT   0x4
#define ATHCFG_WCMD_TESTMODE_ANT      0x5
	u16     operation;    /* Operation */
	u8      antenna;      /* RX-ANT */
	u8      rx;           /* RX START/STOP */
	u32     rssi_combined;/* RSSI */
	u32     rssi0;        /* RSSI */
	u32     rssi1;        /* RSSI */
	u32     rssi2;        /* RSSI */
} __packed;

/**
 * @brief csa info
 */
struct wmi_csa_info_strut {
    u32    freq;
	u16		networkType;
} __packed;

#define WMI_CE_RATE_MAX (48)
typedef enum {
    TARGET_POWER_MODE_OFDM     = 0,
    TARGET_POWER_MODE_CCK      = 1,
    TARGET_POWER_MODE_HT20     = 2,
    TARGET_POWER_MODE_OFDM_EXT = 3,
    TARGET_POWER_MODE_CCK_EXT  = 4,
    TARGET_POWER_MODE_HT40     = 5,
    TARGET_POWER_MODE_MAX      = 6,
} TARGET_POWER_MODE_INDEX;
#define WMI_CE_TPWR_MODE_MAX (6)
struct wmi_ctl_cmd {
	u32 channel;//input
	u32 flags;//input,1:HT40+,2:HT40-,0:not support
	u32 txpower[WMI_CE_TPWR_MODE_MAX];//output
	u8  ctlmode[WMI_CE_TPWR_MODE_MAX];//output
	u16 cal_txpower[WMI_CE_RATE_MAX];
} __packed;

extern struct athcfg_wcmd_testmode_t testmode_private;

void ath6kl_tgt_ce_get_reg_event(struct ath6kl_vif *vif, u8 *ptr, u32 len);
void ath6kl_tgt_ce_get_version_info_event(struct ath6kl_vif *vif, u8 *ptr,
						u32 len);
void ath6kl_tgt_ce_get_testmode_event(struct ath6kl_vif *vif, u8 *ptr,
						u32 len);
void ath6kl_tgt_ce_get_txpow_event(struct ath6kl_vif *vif, u8 *ptr, u32 len);
void ath6kl_tgt_ce_get_stainfo_event(struct ath6kl_vif *vif, u8 *ptr, u32 len);
void ath6kl_tgt_ce_get_scaninfo_event(struct ath6kl_vif *vif, u8 *datap,
						u32 len);
void ath6kl_tgt_ce_set_scan_done_event(struct ath6kl_vif *vif, u8 *ptr,
						u32 len);
#if defined(CE_CUSTOM_1)
void ath6kl_tgt_ce_get_widimode_event(struct ath6kl_vif *vif, u8 *ptr,
						u32 len);
#endif
void ath6kl_tgt_ce_csa_event_rx(struct ath6kl_vif *vif, u8 *datap,int len);
void ath6kl_tgt_ce_get_ctl_event(struct ath6kl_vif *vif, u8 *ptr, u32 len);
int ath6kl_wmi_get_customer_product_info_cmd(struct ath6kl_vif *vif,
					struct athcfg_wcmd_product_info_t *val);

int ath6kl_wmi_set_customer_reg_cmd(struct ath6kl_vif *vif, u32 addr, u32 val);

int ath6kl_wmi_get_customer_reg_cmd(struct ath6kl_vif *vif, u32 addr, u32 *val);

int ath6kl_wmi_set_customer_scan_cmd(struct ath6kl_vif *vif);

int ath6kl_wmi_get_customer_scan_cmd(struct ath6kl_vif *vif,
					struct athcfg_wcmd_scan_t *val);

int ath6kl_wmi_get_customer_testmode_cmd(struct ath6kl_vif *vif,
					struct athcfg_wcmd_testmode_t *val);

int ath6kl_wmi_set_customer_testmode_cmd(struct ath6kl_vif *vif,
					struct athcfg_wcmd_testmode_t *val);

int ath6kl_wmi_get_customer_scan_time_cmd(struct ath6kl_vif *vif,
					struct athcfg_wcmd_scantime_t *val);

int ath6kl_wmi_get_customer_version_info_cmd(struct ath6kl_vif *vif,
					struct athcfg_wcmd_version_info_t *val);

int ath6kl_wmi_get_customer_txpow_cmd(struct ath6kl_vif *vif,
					struct athcfg_wcmd_txpower_t *val);

int ath6kl_wmi_get_customer_stainfo_cmd(struct ath6kl_vif *vif,
					struct athcfg_wcmd_sta_t *val);

int ath6kl_wmi_get_customer_mode_cmd(struct ath6kl_vif *vif,
					enum athcfg_wcmd_phymode_t *val);

int ath6kl_wmi_get_customer_stats_cmd(struct ath6kl_vif *vif,
					struct athcfg_wcmd_stats_t *val);

int ath6kl_wmi_set_suspend_cmd(struct ath6kl_vif *vif,
					struct athcfg_wcmd_suspend_t *val);
#if defined(CE_CUSTOM_1)
int ath6kl_wmi_set_customer_widi_cmd(struct ath6kl_vif *vif, u8 val);
int ath6kl_wmi_get_customer_widi_cmd(struct ath6kl_vif *vif, u8 *val);
#endif
void ath6kl_usb_get_usbinfo(void *product_info);
int ath6kl_ce_ioctl(struct net_device *netdev, struct ifreq *ifr, int cmd);
#endif /* CE_ATHTST_H */
