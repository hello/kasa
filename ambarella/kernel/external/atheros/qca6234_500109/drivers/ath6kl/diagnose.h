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

#ifndef DIAGNOSE_H
#define DIAGNOSE_H


#ifdef ATH6KL_DIAGNOSTIC
#include <linux/export.h>


/* *********************/
/* Types of packet log events */
#define PKTLOG_TYPE_TXCTL    0
#define PKTLOG_TYPE_TXSTATUS 1
#define PKTLOG_TYPE_RX       2

#define PKTLOG_MAX_TXCTL_WORDS 13
#define PKTLOG_MAX_TXSTATUS_WORDS 10

#define IEEE80211_FC0_TYPE_MASK         0x0c
#define IEEE80211_FC0_TYPE_MGT          0x00
#define IEEE80211_FC0_TYPE_CTL          0x04
#define IEEE80211_FC0_TYPE_DATA         0x08

#define IEEE80211_FC0_SUBTYPE_QOS       0x80
#define IEEE80211_FC0_SUBTYPE_MASK      0xf0

/* does frame have QoS sequence control data */
#define IEEE80211_QOS_HAS_SEQ(framectrl) \
	((framectrl & \
	 (IEEE80211_FC0_TYPE_MASK | IEEE80211_FC0_SUBTYPE_QOS)) == \
	 (IEEE80211_FC0_TYPE_DATA | IEEE80211_FC0_SUBTYPE_QOS))

#define IEEE80211_QOS_HEADERLEN		26
#define IEEE80211_QOS_PADLEN		2

/* Diagnostic command ID */

#define WIFI_DIAG_MAC_TX_FILTER_CMDID	0x0001
#define WIFI_DIAG_MAC_RX_FILTER_CMDID	0x0002
#define WIFI_DIAG_CFG_CMDID	            0x0003

/* MAC TX/RX filter type mask */
#define WIFI_DIAG_MACFILTER_MGT		0x00003F3F
#define WIFI_DIAG_MACFILTER_CTL		0xFF000000
#define WIFI_DIAG_MACFILTER_DATA	0x0000DFFF
#define WIFI_DIAG_MACFILTER_LOW_MASK	0xFF003F3F
#define WIFI_DIAG_MACFILTER_HIGH_MASK	0x0000DFFF
#define WIFI_DIAG_MACFILTER_ENABLEALL	0xFFFFFFFF
#define WIFI_DIAG_MACFILTER_DISABLEALL	0x00000000

/* command enable/disable mask */
#define WIFI_DIAG_MAC_TX_FRAME_EVENTENABLE  0x00000001
#define WIFI_DIAG_MAC_RX_FRAME_EVENTENABLE  0x00000002
#define WIFI_DIAG_MAC_FSM_EVENTENABLE       0x00000004
#define WIFI_DIAG_INTERFERENCE_EVENTENABLE  0x00000008
#define WIFI_DIAG_RX_TIME_EVENTENABLE       0x00000010
#define WIFI_DIAG_PWR_SAVE_EVENTENABLE      0x00000020
#define WIFI_DIAG_TX_STAT_EVENTENABLE       0x00000040
#define WIFI_DIAG_RX_STAT_EVENTENABLE       0x00000080
#define WIFI_DIAG_BT_TIME_EVENTENABLE       0x00000100

/* for rx status */
#define WHAL_RC_FLAG_SGI        0x08 /* use HT SGI if set */
#define MAX11_RATE_INDEX        43
#define WHAL_RXERR_CRC          0x01

extern unsigned int diag_local_test;
extern struct wmi *globalwmi;

struct ath_pktlog_txctl {
	u16 framectrl;       /* frame control field from header */
	u16 seqctrl;         /* frame control field from header */
	u16 bssid_tail;      /* last two octets of bssid */
	u16 sa_tail;         /* last two octets of TA */
	u16 da_tail;         /* last two octets of RA */
	u16 resvd;
	u32 txdesc_ctl[PKTLOG_MAX_TXCTL_WORDS];     /* Tx descriptor words */
	u32 *proto_hdr;   /* Protocol header words (variable length!) */
	u32 *misc;         /* Can be used for HT specific or other misc info */
} __packed;

struct ath_pktlog_txstatus {
	/* Tx descriptor status words */
	u32 txdesc_status[PKTLOG_MAX_TXSTATUS_WORDS];

	/* Can be used for HT specific or other misc info */
	/* 0 is rsrate, 1 is txpower */
	u32 misc[3];

	u32 buf_len;
	u8  buf[1];
} __packed;

#define PKTLOG_MAX_RXSTATUS_WORDS 12

struct ath_pktlog_rx {
	u16 framectrl;       /* frame control field from header */
	u16 seqctrl;         /* sequence control field */
	u16 bssid_tail;      /* last two octets of bssid */
	u16 sa_tail;         /* last two octets of TA */
	u16 da_tail;         /* last two octets of RA */
	u16 resvd;
	u32 rxdesc_status[PKTLOG_MAX_RXSTATUS_WORDS];  /* Rx descriptor words */
	u32 *proto_hdr;  /* Protocol header words (variable length!) */
	u32 *misc;         /* Can be used for HT specific or other misc info */
	u32 rxmcs;
	u32 calibratednf;
	u32 rxstatus[5];
	u32 seq_num;
	u32 buf_len;
	u8  buf[1];
} __packed;

/* Each packet log entry consists of the following fixed length header
   followed by variable length log information determined by log_type */
struct ath_pktlog_hdr {
	u32 flags;    /* See flags defined below */
	u16 log_type; /* Type of log information foll this header */
	u16 size;       /* Size of variable length log information in bytes */
	u32 timestamp;
} __packed;

struct whal_rate_code {
	u8 rateCode;
	u8 flags;
} __packed;


/* Rx status is in first ds, this shall be only used by AR6004_REV6 for now */

struct rx_desc_status {
	u16      rsDataLen; /* rx frame length */
	u16      reserved0;  /* for alignment */
	u8       rsStatus;  /* rx status, 0 => recv ok */
	u8       reserved1;  /* for alignment */
	u8       rsRssi;    /* rx frame RSSI */
	u8       reserved2;   /* for alignment */

	/* h/w receive rate code + flags WHAL_RATE_CODE */
	struct whal_rate_code rsRate;
} __packed;

/* tx ctrl descriptor */
struct tx_ctrl_desc {
	u32    dsCtl0;    /* opaque DMA control 0 */
} __packed;

#define WHAL_TXDESC_GET_FRAME_LEN(x)       ((x)->dsCtl0 & 0xFFF)



/* DIANOSTIC API */

/* event ID */
enum wifi_diag_event_id {
	WIFI_DIAG_MAC_TX_FRAME_EVENTID = 0x0001,
	WIFI_DIAG_MAC_RX_FRAME_EVENTID,
	WIFI_DIAG_MAC_FSM_EVENTID,
	WIFI_DIAG_INTERFERENCE_EVENTID,
	WIFI_DIAG_RX_TIME_EVENTID,
	WIFI_DIAG_PWR_SAVE_EVENTID,
	WIFI_DIAG_TX_STAT_EVENTID,
	WIFI_DIAG_RX_STAT_EVENTID,
	WIFI_DIAG_BT_TIME_EVENTID,
};


enum wifi_diag_status_t {
	WIFI_DIAG_EOK = 0x0,
	WIFI_DIAG_ENXIO,
	WIFI_DIAG_ENOMEM,
	WIFI_DIAG_EINVAL,
	WIFI_DIAG_ENOTSUPP,
	WIFI_DIAG_EINPROGRESS,
	WIFI_DIAG_EBUSY,
	WIFI_DIAG_EEXIST,
	WIFI_DIAG_ENOSPC,
	WIFI_DIAG_ASYNC,
};


struct wifi_diag_event_t {
	u16 event_id;
	u16 len;
	u32 seq_num;
	u8 event_data[1];
} __packed;

enum wifi_diag_mac_fsm_t {
	WIFI_DIAG_MAC_FSM_SCANNING = 0x0,
	WIFI_DIAG_MAC_FSM_AUTH,
	WIFI_DIAG_MAC_FSM_ASSOC,
	WIFI_DIAG_MAC_FSM_CONNECTED,
	WIFI_DIAG_MAC_FSM_DEAUTH,
	WIFI_DIAG_MAC_FSM_DISASSOC,
	WIFI_DIAG_MAC_FSM_DISCONNECTED,
};

struct wifi_diag_mac_fsm_event_t {
	enum wifi_diag_mac_fsm_t  fsm;
} __packed;


/* txrx frame event*/
struct wifi_diag_mac_rx_frame_event_t {
	u32 rssi;
	u8  snr;
	u32 rx_mcs;
	u32 rx_bitrate;
	bool fcs;
	u8  frame_type;
	u8  frame_sub_type;
	u16 frame_length;
	u8  frame_data[1];
} __packed;

struct wifi_diag_mac_tx_frame_event_t {
	u32 tx_pwr;
	u8  tx_mcs;
	u32 tx_bitrate;
	u8  frame_type;
	u8  frame_sub_type;
	u16 frame_length;
	u8  frame_data[1];
} __packed;

/* txrx statistics event */
struct wifi_diag_tx_stat_event_t {
	u64 tx_pkt;
	u64 tx_ucast_pkt;
	u64 tx_retry_cnt;
	u64 tx_fail_cnt;
	u32 tx_rate_pkt[44];
} __packed;

struct wifi_diag_rx_stat_event_t {
	u64 rx_pkt;
	u64 rx_ucast_pkt;
	u64 rx_dupl_frame;
	u32 rx_rate_pkt[44];
} __packed;

/* interference time period event */
struct wifi_diag_interference_event_t {
	u32 rx_clear_cnt;
} __packed;

/* RX time period event */
struct wifi_diag_rxtime_event_t {
	u32 rx_frame_cnt;
} __packed;

/* power save event */
enum wifi_diag_pwrsave_t {
	WIFI_DIAG_DEEPSLEEP_START = 0x0,
	WIFI_DIAG_DEEPSLEEP_STOP,
	WIFI_DIAG_FAKESLEEP_START,
	WIFI_DIAG_FAKESLEEP_STOP,
	WIFI_DIAG_MAXPERF_START,
	WIFI_DIAG_MAXPERF_STOP,
};

struct wifi_diag_pwrsave_event_t {
	enum wifi_diag_pwrsave_t  pwrsave;
} __packed;


struct wifi_diag_cmd_t {
	u16 cmd_id;
	u16 len;
	u8 cmd_data[1];
} __packed;

struct wifi_diag_mac_tx_filter_cmd_t {
	u32 filter_mask_low;
	u32 filter_mask_high;
} __packed;

struct wifi_diag_mac_rx_filter_cmd_t {
	u32 filter_mask_low;
	u32 filter_mask_high;
} __packed;

struct wifi_diag_cfg_cmd_t {
	u32 cfg;
	u16 value;
} __packed;

struct wifi_diag_callbacks {
	enum wifi_diag_status_t (*diag_event_callback)
		(void *diag_hdl, struct sk_buff *skb);
};

struct wifi_drv_hdl_list {
	void *             (*wifi_register)(void *diag_hdl);
	enum wifi_diag_status_t (*wifi_unregister)(void *drv_hdl);
	enum wifi_diag_status_t (*wifi_diag_reg_event_callback)
		(void *drv_hdl, struct wifi_diag_callbacks *evt_callback);
	enum wifi_diag_status_t (*wifi_diag_cmd)
		(void *drv_hdl, struct wifi_diag_cmd_t *cmd);
};


struct wifi_diag {
	u32	cfg_mask;
	bool	diag_event_init;
	u8	tx_frame_type;
	u8	tx_frame_subtype;
	u16	tx_frame_len;
	u32	pre_rx_clear_cnt;
	u32	pre_rx_frame_cnt;
	u32	connect_seq_num;
	u32	disconnect_seq_num;
	struct timer_list	tx_stat_timer;
	struct timer_list	rx_stat_timer;
	struct timer_list	interference_timer;
	struct timer_list	rxtime_timer;
	u32	tx_timer_val;
	u32	rx_timer_val;
};


void *
wifi_diag_drv_register(void *diag_hdl);
enum wifi_diag_status_t
wifi_diag_drv_unregister(void *drv_hdl);
enum wifi_diag_status_t wifi_diag_register_event_callback(void *drv_hdl,
	struct wifi_diag_callbacks *evt_callback);
enum wifi_diag_status_t wifi_diag_cmd_send(void *drv_hdl,
	struct wifi_diag_cmd_t *cmd);
void wifi_diag_mac_tx_frame_event(struct ath6kl_vif *vif,
	struct ath_pktlog_txstatus *txstatus_log);
void wifi_diag_mac_txctrl_event(struct ath6kl_vif *vif,
	struct ath_pktlog_txctl *txctrl_log);
void wifi_diag_mac_rx_frame_event(struct ath6kl_vif *vif,
	struct ath_pktlog_rx *rx_log);
void wifi_diag_mac_fsm_event(struct ath6kl_vif *vif,
	enum wifi_diag_mac_fsm_t eventtype, u32 seq_num);
void wifi_diag_send_pwrsave_event(struct ath6kl_vif *vif,
	enum wifi_diag_pwrsave_t pwrsave, u32 seq_num);
void wifi_diag_tx_stat_timer_handler(unsigned long ptr);
void wifi_diag_rx_stat_timer_handler(unsigned long ptr);
void wifi_diag_interference_timer_handler(unsigned long ptr);
void wifi_diag_rxtime_timer_handler(unsigned long ptr);
void wifi_diag_timer_destroy(struct ath6kl_vif *vif);
void wifi_diag_init(void);

int ath6kl_wmi_stat_rx_rate_event(struct ath6kl_vif *vif,
	struct wmi *wmi, u8 *datap, int len, u32 seq_num);
int ath6kl_wmi_stat_tx_rate_event(struct ath6kl_vif *vif,
	struct wmi *wmi, u8 *datap, int len, u32 seq_num);
int ath6kl_wmi_interference_event(struct ath6kl_vif *vif,
	struct wmi *wmi, u8 *datap, int len, u32 seq_num);
int ath6kl_wmi_rxtime_event(struct ath6kl_vif *vif,
	struct wmi *wmi, u8 *datap, int len, u32 seq_num);
int ath6kl_wmi_diag_event(struct ath6kl_vif *vif,
	struct wmi *wmi, struct sk_buff *skb);

#endif /* ATH6KL_DIAGNOSTIC */
#endif /* DIAGNOSE_H */
