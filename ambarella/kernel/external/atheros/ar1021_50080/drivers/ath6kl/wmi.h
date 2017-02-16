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

#ifndef WMI_H
#define WMI_H

#include <linux/ieee80211.h>

#include "htc.h"
#include "wlan_location_defs.h"
#define HTC_PROTOCOL_VERSION		0x0002
#define WMI_PROTOCOL_VERSION		0x0002
#define WMI_CONTROL_MSG_MAX_LEN		256
#define is_ethertype(type_or_len)	((type_or_len) >= 0x0600)

#define IP_ETHERTYPE		0x0800

#define WMI_IMPLICIT_PSTREAM	0xFF
#define WMI_MAX_THINSTREAM	15

#define SSID_IE_LEN_INDEX	13

/* Host side link management data structures */
#define SIG_QUALITY_THRESH_LVLS		6
#define SIG_QUALITY_UPPER_THRESH_LVLS	SIG_QUALITY_THRESH_LVLS
#define SIG_QUALITY_LOWER_THRESH_LVLS	SIG_QUALITY_THRESH_LVLS

#define A_BAND_24GHZ           0
#define A_BAND_5GHZ            1
#define A_NUM_BANDS            2

/* in ms */
#define WMI_IMPLICIT_PSTREAM_INACTIVITY_INT 5000

/*
 * There are no signed versions of __le16 and __le32, so for a temporary
 * solution come up with our own version. The idea is from fs/ntfs/types.h.
 *
 * Use a_ prefix so that it doesn't conflict if we get proper support to
 * linux/types.h.
 */
typedef __s16 __bitwise a_sle16;
typedef __s32 __bitwise a_sle32;

static inline a_sle32 a_cpu_to_sle32(s32 val)
{
	return (__force a_sle32) cpu_to_le32(val);
}

static inline s32 a_sle32_to_cpu(a_sle32 val)
{
	return le32_to_cpu((__force __le32) val);
}

static inline a_sle16 a_cpu_to_sle16(s16 val)
{
	return (__force a_sle16) cpu_to_le16(val);
}

static inline s16 a_sle16_to_cpu(a_sle16 val)
{
	return le16_to_cpu((__force __le16) val);
}

struct sq_threshold_params {
	s16 upper_threshold[SIG_QUALITY_UPPER_THRESH_LVLS];
	s16 lower_threshold[SIG_QUALITY_LOWER_THRESH_LVLS];
	u32 upper_threshold_valid_count;
	u32 lower_threshold_valid_count;
	u32 polling_interval;
	u8 weight;
	u8 last_rssi;
	u8 last_rssi_poll_event;
};

struct wmi_data_sync_bufs {
	u8 traffic_class;
	struct sk_buff *skb;
};

struct wmi_mgmt_tx_frame {
	struct list_head list;

	struct ath6kl_vif *vif;
	u8 *mgmt_tx_frame;
	size_t mgmt_tx_frame_len;
	int mgmt_tx_frame_idx;
	u32 mgmt_tx_frame_freq;
#define WMI_TX_MGMT_RETRY_MAX	(3)
	int mgmt_tx_frame_retry;
};

/* WMM stream classes */
#define WMM_NUM_AC  4
#define WMM_AC_BE   0		/* best effort */
#define WMM_AC_BK   1		/* background */
#define WMM_AC_VI   2		/* video */
#define WMM_AC_VO   3		/* voice */

#define WMI_VOICE_USER_PRIORITY    0x7

struct wmi {
	u16 stream_exist_for_ac[WMM_NUM_AC];
	u8 fat_pipe_exist;
	struct ath6kl *parent_dev;
	u8 pwr_mode;
	spinlock_t lock;
	enum htc_endpoint_id ep_id;
	struct sq_threshold_params
	    sq_threshld[SIGNAL_QUALITY_METRICS_NUM_MAX];
	bool is_wmm_enabled;
	u8 traffic_class;
	bool is_probe_ssid;

	struct list_head mgmt_tx_frame_list;
};

struct host_app_area {
	__le32 wmi_protocol_ver;
} __packed;

enum wmi_msg_type {
	DATA_MSGTYPE = 0x0,
	CNTL_MSGTYPE,
	SYNC_MSGTYPE,
	OPT_MSGTYPE,
};

/*
 * Macros for operating on WMI_DATA_HDR (info) field
 */

#define WMI_DATA_HDR_MSG_TYPE_MASK  0x03
#define WMI_DATA_HDR_MSG_TYPE_SHIFT 0
#define WMI_DATA_HDR_UP_MASK        0x07
#define WMI_DATA_HDR_UP_SHIFT       2

/* In AP mode, the same bit (b5) is used to indicate Power save state in
 * the Rx dir and More data bit state in the tx direction.
 */
#define WMI_DATA_HDR_PS_MASK        0x1
#define WMI_DATA_HDR_PS_SHIFT       5

#define WMI_DATA_HDR_MORE_MASK      0x1
#define WMI_DATA_HDR_MORE_SHIFT     5
#define WMI_DATA_HDR_SET_MORE_BIT(h)	\
	((h)->info |= (WMI_DATA_HDR_MORE_MASK << WMI_DATA_HDR_MORE_SHIFT))
#define WMI_DATA_HDR_HAS_MORE_BIT(h)	\
	((h)->info & (WMI_DATA_HDR_MORE_MASK << WMI_DATA_HDR_MORE_SHIFT))

enum wmi_data_hdr_data_type {
	WMI_DATA_HDR_DATA_TYPE_802_3 = 0,
	WMI_DATA_HDR_DATA_TYPE_802_11,

	/* used to be used for the PAL */
	WMI_DATA_HDR_DATA_TYPE_ACL,
};

/* Bitmap of data header flags */
enum WMI_DATA_HDR_FLAGS {
	WMI_DATA_HDR_FLAGS_MORE = 0x1,
	WMI_DATA_HDR_FLAGS_EOSP = 0x2,
	WMI_DATA_HDR_FLAGS_TRIGGERED = 0x4,
	WMI_DATA_HDR_FLAGS_PSPOLLED = 0x8,
};

#define WMI_DATA_HDR_DATA_TYPE_MASK     0x3
#define WMI_DATA_HDR_DATA_TYPE_SHIFT    6

/* Macros for operating on WMI_DATA_HDR (info2) field */
#define WMI_DATA_HDR_SEQNO_MASK     0xFFF
#define WMI_DATA_HDR_SEQNO_SHIFT    0

#define WMI_DATA_HDR_AMSDU_MASK     0x1
#define WMI_DATA_HDR_AMSDU_SHIFT    12

#define WMI_DATA_HDR_META_MASK      0x7
#define WMI_DATA_HDR_META_SHIFT     13

#define WMI_DATA_HDR_PAD_BEFORE_DATA_MASK               0xFF
#define WMI_DATA_HDR_PAD_BEFORE_DATA_SHIFT              0x8

#define WMI_DATA_HDR_IF_IDX_MASK    0xF

/* FIXME : endian? */
#define WMI_DATA_HDR_TRIGGER_MASK	0x1
#define WMI_DATA_HDR_TRIGGER_SHIFT	4
#define WMI_DATA_HDR_SET_TRIGGER(h, _v)					\
	((h)->info3 = ((h)->info3 & ~(WMI_DATA_HDR_TRIGGER_MASK <<	\
				      WMI_DATA_HDR_TRIGGER_SHIFT))	\
	| ((_v) << WMI_DATA_HDR_TRIGGER_SHIFT))
#define WMI_DATA_HDR_IS_TRIGGER(h)					\
	(((le16_to_cpu((h)->info3) >> WMI_DATA_HDR_TRIGGER_SHIFT) &	\
	WMI_DATA_HDR_TRIGGER_MASK) == WMI_DATA_HDR_TRIGGER_MASK)

#define WMI_DATA_HDR_EOSP_MASK			WMI_DATA_HDR_TRIGGER_MASK
#define WMI_DATA_HDR_EOSP_SHIFT			WMI_DATA_HDR_TRIGGER_SHIFT
#define WMI_DATA_HDR_SET_EOSP_BIT(h)	\
	((h)->info3 |= (WMI_DATA_HDR_EOSP_MASK << WMI_DATA_HDR_EOSP_SHIFT))
#define WMI_DATA_HDR_HAS_EOSP_BIT(h)	\
	((h)->info3 & (WMI_DATA_HDR_EOSP_MASK << WMI_DATA_HDR_EOSP_SHIFT))

#define WMI_DATA_HDR_TRIGGERED_MASK			0x1
#define WMI_DATA_HDR_TRIGGERED_SHIFT		6
#define WMI_DATA_HDR_SET_TRIGGERED_BIT(h)	((h)->info3 |=	\
	(WMI_DATA_HDR_TRIGGERED_MASK << WMI_DATA_HDR_TRIGGERED_SHIFT))
#define WMI_DATA_HDR_HAS_TRIGGERED_BIT(h)	((h)->info3 &	\
	(WMI_DATA_HDR_TRIGGERED_MASK << WMI_DATA_HDR_TRIGGERED_SHIFT))

#define WMI_DATA_HDR_PSPOLLED_MASK			0x1
#define WMI_DATA_HDR_PSPOLLED_SHIFT			7
#define WMI_DATA_HDR_SET_PSPOLLED_BIT(h)	((h)->info3 |= \
	(WMI_DATA_HDR_PSPOLLED_MASK << WMI_DATA_HDR_PSPOLLED_SHIFT))
#define WMI_DATA_HDR_HAS_PSPOLLED_BIT(h)	((h)->info3 & \
	(WMI_DATA_HDR_PSPOLLED_MASK << WMI_DATA_HDR_PSPOLLED_SHIFT))

struct wmi_data_hdr {
	s8 rssi;

	/*
	 * usage of 'info' field(8-bit):
	 *
	 *  b1:b0       - WMI_MSG_TYPE
	 *  b4:b3:b2    - UP(tid)
	 *  b5          - Used in AP mode.
	 *  More-data in tx dir, PS in rx.
	 *  b7:b6       - Dot3 header(0),
	 *                Dot11 Header(1),
	 *                ACL data(2)
	 */
	u8 info;

	/*
	 * usage of 'info2' field(16-bit):
	 *
	 * b11:b0       - seq_no
	 * b12          - A-MSDU?
	 * b15:b13      - META_DATA_VERSION 0 - 7
	 */
	__le16 info2;

	/*
	 * usage of info3, 16-bit:
	 * b3:b0	- Interface index
	 * b15:b4	- Reserved
	 */
	__le16 info3;
} __packed;

static inline u8 wmi_data_hdr_get_up(struct wmi_data_hdr *dhdr)
{
	return (dhdr->info >> WMI_DATA_HDR_UP_SHIFT) & WMI_DATA_HDR_UP_MASK;
}

static inline void wmi_data_hdr_set_up(struct wmi_data_hdr *dhdr,
				       u8 usr_pri)
{
	dhdr->info &= ~(WMI_DATA_HDR_UP_MASK << WMI_DATA_HDR_UP_SHIFT);
	dhdr->info |= usr_pri << WMI_DATA_HDR_UP_SHIFT;
}

static inline u8 wmi_data_hdr_get_dot11(struct wmi_data_hdr *dhdr)
{
	u8 data_type;

	data_type = (dhdr->info >> WMI_DATA_HDR_DATA_TYPE_SHIFT) &
				   WMI_DATA_HDR_DATA_TYPE_MASK;
	return (data_type == WMI_DATA_HDR_DATA_TYPE_802_11);
}

static inline u16 wmi_data_hdr_get_seqno(struct wmi_data_hdr *dhdr)
{
	return (le16_to_cpu(dhdr->info2) >> WMI_DATA_HDR_SEQNO_SHIFT) &
				WMI_DATA_HDR_SEQNO_MASK;
}

static inline u8 wmi_data_hdr_is_amsdu(struct wmi_data_hdr *dhdr)
{
	return (le16_to_cpu(dhdr->info2) >> WMI_DATA_HDR_AMSDU_SHIFT) &
			       WMI_DATA_HDR_AMSDU_MASK;
}

static inline u8 wmi_data_hdr_get_meta(struct wmi_data_hdr *dhdr)
{
	return (le16_to_cpu(dhdr->info2) >> WMI_DATA_HDR_META_SHIFT) &
			       WMI_DATA_HDR_META_MASK;
}

static inline u8 wmi_data_hdr_get_if_idx(struct wmi_data_hdr *dhdr)
{
	return le16_to_cpu(dhdr->info3) & WMI_DATA_HDR_IF_IDX_MASK;
}

/* Tx meta version definitions */
#define WMI_MAX_TX_META_SZ	12
#define WMI_META_VERSION_1	0x01
#define WMI_META_VERSION_2	0x02

struct wmi_tx_meta_v1 {
	/* packet ID to identify the tx request */
	u8 pkt_id;

	/* rate policy to be used for the tx of this frame */
	u8 rate_plcy_id;
} __packed;

struct wmi_tx_meta_v2 {
	/*
	 * Offset from start of the WMI header for csum calculation to
	 * begin.
	 */
	u8 csum_start;

	/* offset from start of WMI header where final csum goes */
	u8 csum_dest;

	/* no of bytes over which csum is calculated */
	u8 csum_flags;
} __packed;

struct wmi_rx_meta_v1 {
	u8 status;

	/* rate index mapped to rate at which this packet was received. */
	u8 rix;

	/* rssi of packet */
	u8 rssi;

	/* rf channel during packet reception */
	u8 channel;

	__le16 flags;
} __packed;

struct wmi_rx_meta_v2 {
	__le16 csum;

	/* bit 0 set -partial csum valid bit 1 set -test mode */
	u8 csum_flags;
} __packed;

#define WMI_CMD_HDR_IF_ID_MASK 0xF

/* Control Path */
struct wmi_cmd_hdr {
	__le16 cmd_id;

	/* info1 - 16 bits
	 * b03:b00 - id
	 * b15:b04 - unused */
	__le16 info1;

	/* for alignment */
	__le16 reserved;
} __packed;

static inline u8 wmi_cmd_hdr_get_if_idx(struct wmi_cmd_hdr *chdr)
{
	return le16_to_cpu(chdr->info1) & WMI_CMD_HDR_IF_ID_MASK;
}

/* List of WMI commands */
enum wmi_cmd_id {
	WMI_CONNECT_CMDID = 0x0001,
	WMI_RECONNECT_CMDID,
	WMI_DISCONNECT_CMDID,
	WMI_SYNCHRONIZE_CMDID,
	WMI_CREATE_PSTREAM_CMDID,
	WMI_DELETE_PSTREAM_CMDID,
	WMI_START_SCAN_CMDID,
	WMI_SET_SCAN_PARAMS_CMDID,
	WMI_SET_BSS_FILTER_CMDID,
	WMI_SET_PROBED_SSID_CMDID,	/* 10 */
	WMI_SET_LISTEN_INT_CMDID,
	WMI_SET_BMISS_TIME_CMDID,
	WMI_SET_DISC_TIMEOUT_CMDID,
	WMI_GET_CHANNEL_LIST_CMDID,
	WMI_SET_BEACON_INT_CMDID,
	WMI_GET_STATISTICS_CMDID,
	WMI_SET_CHANNEL_PARAMS_CMDID,
	WMI_SET_POWER_MODE_CMDID,
	WMI_SET_IBSS_PM_CAPS_CMDID,
	WMI_SET_POWER_PARAMS_CMDID,	/* 20 */
	WMI_SET_POWERSAVE_TIMERS_POLICY_CMDID,
	WMI_ADD_CIPHER_KEY_CMDID,
	WMI_DELETE_CIPHER_KEY_CMDID,
	WMI_ADD_KRK_CMDID,
	WMI_DELETE_KRK_CMDID,
	WMI_SET_PMKID_CMDID,
	WMI_SET_TX_PWR_CMDID,
	WMI_GET_TX_PWR_CMDID,
	WMI_SET_ASSOC_INFO_CMDID,
	WMI_ADD_BAD_AP_CMDID,		/* 30 */
	WMI_DELETE_BAD_AP_CMDID,
	WMI_SET_TKIP_COUNTERMEASURES_CMDID,
	WMI_RSSI_THRESHOLD_PARAMS_CMDID,
	WMI_TARGET_ERROR_REPORT_BITMASK_CMDID,
	WMI_SET_ACCESS_PARAMS_CMDID,
	WMI_SET_RETRY_LIMITS_CMDID,
	WMI_SET_OPT_MODE_CMDID,
	WMI_OPT_TX_FRAME_CMDID,
	WMI_SET_VOICE_PKT_SIZE_CMDID,
	WMI_SET_MAX_SP_LEN_CMDID,	/* 40 */
	WMI_SET_ROAM_CTRL_CMDID,
	WMI_GET_ROAM_TBL_CMDID,
	WMI_GET_ROAM_DATA_CMDID,
	WMI_ENABLE_RM_CMDID,
	WMI_SET_MAX_OFFHOME_DURATION_CMDID,
	WMI_EXTENSION_CMDID,	/* Non-wireless extensions */
	WMI_SNR_THRESHOLD_PARAMS_CMDID,
	WMI_LQ_THRESHOLD_PARAMS_CMDID,
	WMI_SET_LPREAMBLE_CMDID,
	WMI_SET_RTS_CMDID,		/* 50 */
	WMI_CLR_RSSI_SNR_CMDID,
	WMI_SET_FIXRATES_CMDID,
	WMI_GET_FIXRATES_CMDID,
	WMI_SET_AUTH_MODE_CMDID,
	WMI_SET_REASSOC_MODE_CMDID,
	WMI_SET_WMM_CMDID,
	WMI_SET_WMM_TXOP_CMDID,
	WMI_TEST_CMDID,
	/* COEX AR6002 only */
	WMI_SET_BT_STATUS_CMDID,
	WMI_SET_BT_PARAMS_CMDID,	/* 60 */

	WMI_SET_KEEPALIVE_CMDID,
	WMI_GET_KEEPALIVE_CMDID,
	WMI_SET_APPIE_CMDID,
	WMI_GET_APPIE_CMDID,
	WMI_SET_WSC_STATUS_CMDID,

	/* Wake on Wireless */
	WMI_SET_HOST_SLEEP_MODE_CMDID,
	WMI_SET_WOW_MODE_CMDID,
	WMI_GET_WOW_LIST_CMDID,
	WMI_ADD_WOW_PATTERN_CMDID,
	WMI_DEL_WOW_PATTERN_CMDID,	/* 70 */

	WMI_SET_FRAMERATES_CMDID,
	WMI_SET_AP_PS_CMDID,
	WMI_SET_QOS_SUPP_CMDID,
	/* WMI_THIN_RESERVED_... mark the start and end
	 * values for WMI_THIN_RESERVED command IDs. These
	 * command IDs can be found in wmi_thin.h */
	WMI_THIN_RESERVED_START = 0x8000,
	WMI_THIN_RESERVED_END = 0x8fff,
	/*
	 * Developer commands starts at 0xF000
	 */
	WMI_SET_BITRATE_CMDID = 0xF000,
	WMI_GET_BITRATE_CMDID,
	WMI_SET_WHALPARAM_CMDID,


	/*Should add the new command to the tail for compatible with
	 * etna.
	 */
	WMI_SET_MAC_ADDRESS_CMDID,
	WMI_SET_AKMP_PARAMS_CMDID,
	WMI_SET_PMKID_LIST_CMDID,
	WMI_GET_PMKID_LIST_CMDID,
	WMI_ABORT_SCAN_CMDID,
	WMI_SET_TARGET_EVENT_REPORT_CMDID,

	/* Unused */
	WMI_UNUSED1,
	WMI_UNUSED2,

	/*
	 * AP mode commands
	 */
	WMI_AP_HIDDEN_SSID_CMDID,	/* F00B */
	WMI_AP_SET_NUM_STA_CMDID,
	WMI_AP_ACL_POLICY_CMDID,
	WMI_AP_ACL_MAC_LIST_CMDID,
	WMI_AP_CONFIG_COMMIT_CMDID,
	WMI_AP_SET_MLME_CMDID,	/* F010 */
	WMI_AP_SET_PVB_CMDID,
	WMI_AP_CONN_INACT_CMDID,
	WMI_AP_PROT_SCAN_TIME_CMDID,
	WMI_AP_SET_COUNTRY_CMDID,
	WMI_AP_SET_DTIM_CMDID,
	WMI_AP_MODE_STAT_CMDID,

	WMI_SET_IP_CMDID,	/* F017 */
	WMI_SET_PARAMS_CMDID,
	WMI_SET_MCAST_FILTER_CMDID,
	WMI_DEL_MCAST_FILTER_CMDID,

	WMI_ALLOW_AGGR_CMDID,	/* F01B */
	WMI_ADDBA_REQ_CMDID,
	WMI_DELBA_REQ_CMDID,
	WMI_SET_HT_CAP_CMDID,
	WMI_SET_HT_OP_CMDID,
	WMI_SET_TX_SELECT_RATES_CMDID,
	WMI_SET_TX_SGI_PARAM_CMDID,
	WMI_SET_RATE_POLICY_CMDID,

	WMI_HCI_CMD_CMDID,	/* F023 */
	WMI_RX_FRAME_FORMAT_CMDID,
	WMI_SET_THIN_MODE_CMDID,
	WMI_SET_BT_WLAN_CONN_PRECEDENCE_CMDID,

	WMI_AP_SET_11BG_RATESET_CMDID,	/* F027 */
	WMI_SET_PMK_CMDID,
	WMI_MCAST_FILTER_CMDID,
	/* COEX CMDID AR6003 */
	WMI_SET_BTCOEX_FE_ANT_CMDID, /* F02A */
	WMI_SET_BTCOEX_COLOCATED_BT_DEV_CMDID,
	WMI_SET_BTCOEX_SCO_CONFIG_CMDID,
	WMI_SET_BTCOEX_A2DP_CONFIG_CMDID,
	WMI_SET_BTCOEX_ACLCOEX_CONFIG_CMDID,
	WMI_SET_BTCOEX_BTINQUIRY_PAGE_CONFIG_CMDID,
	WMI_SET_BTCOEX_DEBUG_CMDID,
	WMI_SET_BTCOEX_BT_OPERATING_STATUS_CMDID,
	WMI_GET_BTCOEX_STATS_CMDID,
	WMI_GET_BTCOEX_CONFIG_CMDID,

	WMI_DFS_RESERVED,	/* F034 */
	WMI_SET_DFS_MINRSSITHRESH_CMDID,
	WMI_SET_DFS_MAXPULSEDUR_CMDID,
	WMI_DFS_RADAR_DETECTED_CMDID,

	/* P2P CMDS */
	WMI_P2P_SET_CONFIG_CMDID,	/* F037 */
	WMI_WPS_SET_CONFIG_CMDID,
	WMI_SET_REQ_DEV_ATTR_CMDID,
	WMI_P2P_FIND_CMDID,
	WMI_P2P_STOP_FIND_CMDID,
	WMI_P2P_GO_NEG_START_CMDID,
	WMI_P2P_LISTEN_CMDID,


	WMI_CONFIG_TX_MAC_RULES_CMDID,	/* F044 */
	WMI_SET_PROMISCUOUS_MODE_CMDID,
	WMI_RX_FRAME_FILTER_CMDID,
	WMI_SET_CHANNEL_CMDID,

	/* WAC commands */
	WMI_ENABLE_WAC_CMDID,
	WMI_WAC_SCAN_REPLY_CMDID,
	WMI_WAC_CTRL_REQ_CMDID,

	WMI_SET_DIV_PARAMS_CMDID,
	WMI_GET_PMK_CMDID,
	WMI_SET_PASSPHRASE_CMDID,
	WMI_SEND_ASSOC_RES_CMDID,
	WMI_SET_ASSOC_REQ_RELAY_CMDID,

	/* ACS command, consists of sub-commands */
	WMI_ACS_CTRL_CMDID,
	WMI_SET_EXCESS_TX_RETRY_THRES_CMDID,
	WMI_SET_TBD_TIME_CMDID, /*added for wmiconfig command for TBD */

	/* Pktlog cmds */
	WMI_PKTLOG_ENABLE_CMDID,
	WMI_PKTLOG_DISABLE_CMDID,

	/* More P2P Cmds */
	WMI_P2P_GO_NEG_REQ_RSP_CMDID,
	WMI_P2P_GRP_INIT_CMDID,
	WMI_P2P_GRP_FORMATION_DONE_CMDID,
	WMI_P2P_INVITE_CMDID,
	WMI_P2P_INVITE_REQ_RSP_CMDID,
	WMI_P2P_PROV_DISC_REQ_CMDID,
	WMI_P2P_SET_CMDID,

	WMI_GET_RFKILL_MODE_CMDID,
	WMI_SET_RFKILL_MODE_CMDID,
	WMI_AP_SET_APSD_CMDID,
	WMI_AP_APSD_BUFFERED_TRAFFIC_CMDID,

	WMI_P2P_SDPD_TX_CMDID, /* F05C */
	WMI_P2P_STOP_SDPD_CMDID,
	WMI_P2P_CANCEL_CMDID,
	/* Ultra low power store / recall commands */
	WMI_STORERECALL_CONFIGURE_CMDID,
	WMI_STORERECALL_RECALL_CMDID,
	WMI_STORERECALL_HOST_READY_CMDID,
	WMI_FORCE_TARGET_ASSERT_CMDID,

	WMI_SET_PROBED_SSID_EX_CMDID,
	WMI_SET_NETWORK_LIST_OFFLOAD_CMDID,
	WMI_SET_ARP_NS_OFFLOAD_CMDID,
	WMI_ADD_WOW_EXT_PATTERN_CMDID,
	WMI_GTK_OFFLOAD_OP_CMDID,

	WMI_REMAIN_ON_CHNL_CMDID,
	WMI_CANCEL_REMAIN_ON_CHNL_CMDID,
	WMI_SEND_ACTION_CMDID,
	WMI_PROBE_REQ_REPORT_CMDID,
	WMI_DISABLE_11B_RATES_CMDID,
	WMI_SEND_PROBE_RESPONSE_CMDID,
	WMI_GET_P2P_INFO_CMDID,
	WMI_AP_JOIN_BSS_CMDID,

	WMI_SMPS_ENABLE_CMDID,
	WMI_SMPS_CONFIG_CMDID,
	WMI_SET_RATECTRL_PARM_CMDID,
	/*  LPL specific commands*/
	WMI_LPL_FORCE_ENABLE_CMDID,
	WMI_LPL_SET_POLICY_CMDID,
	WMI_LPL_GET_POLICY_CMDID,
	WMI_LPL_GET_HWSTATE_CMDID,
	WMI_LPL_SET_PARAMS_CMDID,
	WMI_LPL_GET_PARAMS_CMDID,

	WMI_SET_BUNDLE_PARAM_CMDID,

	/*GreenTx specific commands*/
	WMI_GREENTX_PARAMS_CMDID,

	/* WPS Commands */
	WMI_WPS_START_CMDID,
	WMI_GET_WPS_STATUS_CMDID,

	/* More P2P commands */
	WMI_SET_NOA_CMDID,
	WMI_GET_NOA_CMDID,
	WMI_SET_OPPPS_CMDID,
	WMI_GET_OPPPS_CMDID,
	WMI_ADD_PORT_CMDID,
	WMI_DEL_PORT_CMDID,

	/* 802.11w cmd */
	WMI_SET_RSN_CAP_CMDID,
	WMI_GET_RSN_CAP_CMDID,
	WMI_SET_IGTK_CMDID,

	WMI_RX_FILTER_COALESCE_FILTER_OP_CMDID,
	WMI_RX_FILTER_SET_FRAME_TEST_LIST_CMDID,

	WMI_RTT_MEASREQ_CMDID,
	WMI_RTT_CAPREQ_CMDID,
	WMI_RTT_STATUSREQ_CMDID,

	/*led cotrol*/
	WMI_ENABLE_LED_CMDID,
	WMI_CONFIG_LED_CMDID,
	WMI_SET_LED_CMDID,
	/*Socket translation commands*/
	WMI_SOCKET_CMDID,
	WMI_P2P_PSIE_CONFIG_CMDID,
	WMI_LOG_FRAME_CMDID,
	WMI_QUERY_PHY_INFO_CMDID,
	/* P2P FW Offload support */
	WMI_P2P_CONNECT_CMDID,
	WMI_P2P_GET_NODE_LIST_CMDID,
	WMI_P2P_AUTH_GO_NEG_CMDID,
	WMI_P2P_FW_PROV_DISC_REQ_CMDID,

	WMI_SET_DFS_ENABLE_CMDID,

	WMI_CCX_FRAME_REPORT_CMDID, /* CCXv4 */
	WMI_CCX_HOST_FEATURE_CONFIG_CMDID,
	WMI_DIAGNOSTIC_CMDID,	/* diagnostic */
	WMI_SET_FILTERED_PROMISCUOUS_MODE_CMDID,

	/*added btcoex command*/
	WMI_SET_BTCOEX_HID_CONFIG_CMDID,
	WMI_RTT_CONFIG_CMDID,
	WMI_STA_BMISS_ENHANCE_CMDID,
	WMI_P2P_PERSISTENT_PROFILE_CMDID,
	WMI_P2P_SET_JOIN_PROFILE_CMDID,

	/* wifi heart beat */
	WMI_HEART_PARAMS_CMDID,
	WMI_HEART_SET_TCP_PARAMS_CMDID,
	WMI_HEART_SET_TCP_PKT_FILTER_CMDID,
	WMI_HEART_SET_UDP_PARAMS_CMDID,
	WMI_HEART_SET_UDP_PKT_FILTER_CMDID,
	WMI_HEART_SET_NETWORK_INFO_CMDID,

	/* wifi discovery */
	WMI_DISC_SET_IE_FILTER_CMDID,
	WMI_DISC_SET_MODE_CMDID,

	WMI_RTT_CLKCALINFO_CMDID,

	WMI_P2P_SET_PROFILE_CMDID,

	/* P2P FW GO PS Command */
	WMI_P2P_FW_SET_NOA_CMDID,
	WMI_P2P_FW_GET_NOA_CMDID,                /* F0AA */
	WMI_P2P_FW_SET_OPPPS_CMDID,
	WMI_P2P_FW_GET_OPPPS_CMDID,

	/*led cotrol*/
	WMI_ENABLE_BLINKING_LED_CMDID,

	WMI_AP_POLL_STA_CMDID,
	WMI_AP_PSBUF_OFFLOAD_CMDID,
	WMI_SET_REGDOMAIN_CMDID,
/* merge from olca mainline for align command id - start */
	WMI_ARGOS_CMDID,
	WMI_SEND_MGMT_CMDID,
	WMI_BEGIN_SCAN_CMDID,
	WMI_SET_IE_CMDID,
	WMI_SET_RSSI_FILTER_CMDID,
	WMI_SET_CREDIT_REVERSE_CMDID   = 0xF0B6,
	WMI_SET_RCV_DATA_CLASSIFIER_CMDID,
	WMI_AP_SET_IDLE_CLOSE_TIME_CMDID,
	WMI_SET_LTE_COEX_STATE_CMDID,
	WMI_SET_MCC_PROFILE_CMDID,
	WMI_SET_MEDIA_STREAM_CMDID = 0xF0BB,

	/* More SB private commands */
	WMI_SET_CUSTOM_REG,	/* F0BC */
	WMI_GET_CUSTOM_REG,
	WMI_GET_CUSTOM_PRODUCT_INFO,
	WMI_SET_CUSTOM_TESTMODE,
	WMI_GET_CUSTOM_TESTMODE,	/* F0C0 */
	WMI_GET_CUSTOM_STAINFO,
	WMI_GET_CUSTOM_SCANTIME,
	WMI_SET_CUSTOM_SCAN,
	WMI_GET_CUSTOM_SCAN,
	WMI_GET_CUSTOM_VERSION_INFO,
	WMI_GET_CUSTOM_WIFI_TXPOW,
	WMI_GET_CUSTOM_ATHSTATS,

	WMI_TX99TOOL_CMDID,/* F0C8 */
	WMI_SET_CUSTOM_PROBE_RESP_REPORT_CMDID,
	WMI_SET_CUSTOM_WIDI,
	WMI_GET_CUSTOM_WIDI,

	/*Diversity control*/
	WMI_SET_ANTDIVCFG_CMDID, /* F0CC */
	WMI_GET_ANTDIVSTAT_CMDID,

	WMI_GET_ANISTAT_CMDID,

	WMI_SET_SEAMLESS_MCC_SCC_SWITCH_FREQ_CMDID,

	WMI_SET_CHAIN_MASK_CMDID,

	WMI_SET_SCAN_CHAN_PLAN_CMDID,
	WMI_SET_MCC_EVENT_MODE_CMDID,
	WMI_GET_CTL,

	WMI_SET_OCB_CHANNEL_OP_CMDID,

	WMI_SET_SWOL_INDOOR_INFO,
	WMI_SET_SWOL_OUTDOOR_INFO,

	WMI_SET_SYNC_TSF_CMDID,

/* merge from olca mainline for align command id - end
 * private commands shall grow back from 0xFFFE
 */
	WMI_SET_GO_SYNC_CMDID = 0xFFFC,
	WMI_SET_DTIM_EXT_CMDID = 0xFFFD,
	WMI_SET_CREDIT_BYPASS_CMDID = 0xFFFE,
};

enum wmi_mgmt_frame_type {
	WMI_FRAME_BEACON = 0,
	WMI_FRAME_PROBE_REQ,
	WMI_FRAME_PROBE_RESP,
	WMI_FRAME_ASSOC_REQ,
	WMI_FRAME_ASSOC_RESP,
	WMI_NUM_MGMT_FRAME
};

/* WMI_CONNECT_CMDID  */
enum network_type {
	INFRA_NETWORK = 0x01,
	ADHOC_NETWORK = 0x02,
	ADHOC_CREATOR = 0x04,
	AP_NETWORK = 0x10,
};

enum dot11_auth_mode {
	OPEN_AUTH = 0x01,
	SHARED_AUTH = 0x02,

	/* different from IEEE_AUTH_MODE definitions */
	LEAP_AUTH = 0x04,
};

enum auth_mode {
	NONE_AUTH = 0x01,
	WPA_AUTH = 0x02,
	WPA2_AUTH = 0x04,
	WPA_PSK_AUTH = 0x08,
	WPA2_PSK_AUTH = 0x10,
	WPA_AUTH_CCKM = 0x20,
	WPA2_AUTH_CCKM = 0x40,
#ifdef PMF_SUPPORT
	WPA2_PSK_SHA256_AUTH = 0x80,
#endif
};

#define WMI_MIN_KEY_INDEX   0
#define WMI_MAX_KEY_INDEX   3

#define WMI_MAX_KEY_LEN     32

#ifdef PMF_SUPPORT
#define WMI_MIN_IGTK_INDEX  4
#define WMI_MAX_IGTK_INDEX  5
#define WMI_IGTK_KEY_LEN    16
#endif

/*
 * NB: these values are ordered carefully; there are lots of
 * of implications in any reordering.  In particular beware
 * that 4 is not used to avoid conflicting with IEEE80211_F_PRIVACY.
 */
#define ATH6KL_CIPHER_WEP            0
#define ATH6KL_CIPHER_TKIP           1
#define ATH6KL_CIPHER_AES_OCB        2
#define ATH6KL_CIPHER_AES_CCM        3
#define ATH6KL_CIPHER_CKIP           5
#define ATH6KL_CIPHER_CCKM_KRK       6
#define ATH6KL_CIPHER_NONE           7 /* pseudo value */

/*
 * 802.11 rate set.
 */
#define ATH6KL_RATE_MAXSIZE  15	/* max rates we'll handle */

#define ATH_OUI_TYPE            0x01
#define WPA_OUI_TYPE            0x01
#define WMM_PARAM_OUI_SUBTYPE   0x01
#define WMM_OUI_TYPE            0x02
#define WSC_OUT_TYPE            0x04

enum wmi_connect_ctrl_flags_bits {
	CONNECT_ASSOC_POLICY_USER = 0x0001,
	CONNECT_SEND_REASSOC = 0x0002,
	CONNECT_IGNORE_WPAx_GROUP_CIPHER = 0x0004,
	CONNECT_PROFILE_MATCH_DONE = 0x0008,
	CONNECT_IGNORE_AAC_BEACON = 0x0010,
	CONNECT_CSA_FOLLOW_BSS = 0x0020,
	CONNECT_DO_WPA_OFFLOAD = 0x0040,
	CONNECT_DO_NOT_DEAUTH = 0x0080,
	CONNECT_WPS_FLAG = 0x0100,
	/* AP configuration flags */
	AP_NO_DISASSOC_UPON_DEAUTH = 0x10000,
	AP_HOSTPAL_SUPPORT = 0x20000,
};

#define WMI_CONNECT_AP_CHAN_SELECT_OFFSET  (14)
#define WMI_CONNECT_AP_CHAN_SELECT_MASK    (0xc000)

enum wmi_connect_ap_channel_type {
	AP_CHANNEL_TYPE_NONE = 0,
	AP_CHANNEL_TYPE_HT40PLUS,
	AP_CHANNEL_TYPE_HT40MINUS,
	AP_CHANNEL_TYPE_HT20
};

struct wmi_connect_cmd {
	u8 nw_type;
	u8 dot11_auth_mode;
	u8 auth_mode;
	u8 prwise_crypto_type;
	u8 prwise_crypto_len;
	u8 grp_crypto_type;
	u8 grp_crypto_len;
	u8 ssid_len;
	u8 ssid[IEEE80211_MAX_SSID_LEN];
	__le16 ch;
	u8 bssid[ETH_ALEN];
	__le32 ctrl_flags;
} __packed;

/* WMI_RECONNECT_CMDID */
struct wmi_reconnect_cmd {
	/* channel hint */
	__le16 channel;

	/* mandatory if set */
	u8 bssid[ETH_ALEN];
} __packed;

/* WMI_ADD_CIPHER_KEY_CMDID */
enum key_usage {
	PAIRWISE_USAGE = 0x00,
	GROUP_USAGE = 0x01,

	/* default Tx Key - static WEP only */
	TX_USAGE = 0x02,
};

/*
 * Bit Flag
 * Bit 0 - Initialise TSC - default is Initialize
 */
#define KEY_OP_INIT_TSC     0x01
#define KEY_OP_INIT_RSC     0x02

/* default initialise the TSC & RSC */
#define KEY_OP_INIT_VAL     0x03
#define KEY_OP_VALID_MASK   0x03

struct wmi_add_cipher_key_cmd {
	u8 key_index;
	u8 key_type;

	/* enum key_usage */
	u8 key_usage;

	u8 key_len;

	/* key replay sequence counter */
	u8 key_rsc[8];

	u8 key[WLAN_MAX_KEY_LEN];

	/* additional key control info */
	u8 key_op_ctrl;

	u8 key_mac_addr[ETH_ALEN];
} __packed;

/* WMI_DELETE_CIPHER_KEY_CMDID */
struct wmi_delete_cipher_key_cmd {
	u8 key_index;
} __packed;

#define WMI_KRK_LEN     16

/* WMI_ADD_KRK_CMDID */
struct wmi_add_krk_cmd {
	u8 krk[WMI_KRK_LEN];
} __packed;

#ifdef PMF_SUPPORT
struct wmi_add_igtk_cmd {
	u8 key_index;
	u8 key_len;
	u8 key_rsc[6];
	u8 key[WMI_IGTK_KEY_LEN];
} __packed;
#endif

/* WMI_SETPMKID_CMDID */

#define WMI_PMKID_LEN 16

enum pmkid_enable_flg {
	PMKID_DISABLE = 0,
	PMKID_ENABLE = 1,
};

struct wmi_setpmkid_cmd {
	u8 bssid[ETH_ALEN];

	/* enum pmkid_enable_flg */
	u8 enable;

	u8 pmkid[WMI_PMKID_LEN];
} __packed;

/* WMI_START_SCAN_CMD */
enum wmi_scan_type {
	WMI_LONG_SCAN = 0,
	WMI_SHORT_SCAN = 1,
};

struct wmi_start_scan_cmd {
	__le32 force_fg_scan;

	/* for legacy cisco AP compatibility */
	__le32 is_legacy;

	/* max duration in the home channel(msec) */
	__le32 home_dwell_time;

	/* time interval between scans (msec) */
	__le32 force_scan_intvl;

	/* enum wmi_scan_type */
	u8 scan_type;

	/* how many channels follow */
	u8 num_ch;

	/* channels in Mhz */
	__le16 ch_list[1];
} __packed;

/*
 *  Warning: scan control flag value of 0xFF is used to disable
 *  all flags in WMI_SCAN_PARAMS_CMD. Do not add any more
 *  flags here
 */
enum wmi_scan_ctrl_flags_bits {

	/* set if can scan in the connect cmd */
	CONNECT_SCAN_CTRL_FLAGS = 0x01,

	/* set if scan for the SSID it is already connected to */
	SCAN_CONNECTED_CTRL_FLAGS = 0x02,

	/* set if enable active scan */
	ACTIVE_SCAN_CTRL_FLAGS = 0x04,

	/* set if enable roam scan when bmiss and lowrssi */
	ROAM_SCAN_CTRL_FLAGS = 0x08,

	/* set if follows customer BSSINFO reporting rule */
	REPORT_BSSINFO_CTRL_FLAGS = 0x10,

	/* if disabled, target doesn't scan after a disconnect event  */
	ENABLE_AUTO_CTRL_FLAGS = 0x20,

	/*
	 * Scan complete event with canceled status will be generated when
	 * a scan is prempted before it gets completed.
	 */
	ENABLE_SCAN_ABORT_EVENT = 0x40,

	/* set if skip scanning dfs channel */
	ENABLE_DFS_SKIP_CTRL_FLAGS = 0x80,
};

struct wmi_scan_params_cmd {
	  /* sec */
	__le16 fg_start_period;

	/* sec */
	__le16 fg_end_period;

	/* sec */
	__le16 bg_period;

	/* msec */
	__le16 maxact_chdwell_time;

	/* msec */
	__le16 pas_chdwell_time;

	  /* how many shorts scan for one long */
	u8 short_scan_ratio;

	u8 scan_ctrl_flags;

	/* msec */
	__le16 minact_chdwell_time;

	/* max active scans per ssid */
	__le16 maxact_scan_per_ssid;

	/* msecs */
	__le32 max_dfsch_act_time;
} __packed;

/* WMI_SET_BSS_FILTER_CMDID */
enum wmi_bss_filter {
	/* no beacons forwarded */
	NONE_BSS_FILTER = 0x0,

	/* all beacons forwarded */
	ALL_BSS_FILTER,

	/* only beacons matching profile */
	PROFILE_FILTER,

	/* all but beacons matching profile */
	ALL_BUT_PROFILE_FILTER,

	/* only beacons matching current BSS */
	CURRENT_BSS_FILTER,

	/* all but beacons matching BSS */
	ALL_BUT_BSS_FILTER,

	/* beacons matching probed ssid */
	PROBED_SSID_FILTER,

	/* marker only */
	LAST_BSS_FILTER,
};

struct wmi_bss_filter_cmd {
	/* see, enum wmi_bss_filter */
	u8 bss_filter;

	/* for alignment */
	u8 reserved1;

	/* for alignment */
	__le16 reserved2;

	__le32 ie_mask;
} __packed;

/* WMI_SET_PROBED_SSID_CMDID */
#define MAX_PROBED_SSID_INDEX   9

enum wmi_ssid_flag {
	/* disables entry */
	DISABLE_SSID_FLAG = 0,

	/* probes specified ssid */
	SPECIFIC_SSID_FLAG = 0x01,

	/* probes for any ssid */
	ANY_SSID_FLAG = 0x02,
};

struct wmi_probed_ssid_cmd {
	/* 0 to MAX_PROBED_SSID_INDEX */
	u8 entry_index;

	/* see, enum wmi_ssid_flg */
	u8 flag;

	u8 ssid_len;
	u8 ssid[IEEE80211_MAX_SSID_LEN];
} __packed;

/*
 * WMI_SET_LISTEN_INT_CMDID
 * The Listen interval is between 15 and 3000 TUs
 */
struct wmi_listen_int_cmd {
	__le16 listen_intvl;
	__le16 num_beacons;
} __packed;

struct wmi_set_regdomain_cmd {
	u8 length;
	u8 iso_name[2];
} __packed;

/* WMI_SET_POWER_MODE_CMDID */
enum wmi_power_mode {
	REC_POWER = 0x01,
	MAX_PERF_POWER,
};

struct wmi_power_mode_cmd {
	/* see, enum wmi_power_mode */
	u8 pwr_mode;
} __packed;

/*
 * Policy to determnine whether power save failure event should be sent to
 * host during scanning
 */
enum power_save_fail_event_policy {
	SEND_POWER_SAVE_FAIL_EVENT_ALWAYS = 1,
	IGNORE_POWER_SAVE_FAIL_EVENT_DURING_SCAN = 2,
};

struct wmi_power_params_cmd {
	/* msec */
	__le16 idle_period;

	__le16 pspoll_number;
	__le16 dtim_policy;
	__le16 tx_wakeup_policy;
	__le16 num_tx_to_wakeup;
	__le16 ps_fail_event_policy;
} __packed;

/* Adhoc power save types */
enum wmi_adhoc_ps_type {
	ADHOC_PS_DISABLE = 1,
	ADHOC_PS_ATH = 2,
	ADHOC_PS_IEEE = 3,
	ADHOC_PS_OTHER = 4,
	ADHOC_PS_KTK = 5,
};

struct wmi_ibss_pm_caps_cmd {
	/* see, enum wmi_adhoc_ps_type */
	u8 power_saving;

	/* number of beacon periods */
	u8 ttl;

	/* msec */
	__le16 atim_windows;

	/* msec */
	__le16 timeout_value;
} __packed;

/* WMI_SET_DISC_TIMEOUT_CMDID */
struct wmi_disc_timeout_cmd {
	/* seconds */
	u8 discon_timeout;
} __packed;

enum dir_type {
	UPLINK_TRAFFIC = 0,
	DNLINK_TRAFFIC = 1,
	BIDIR_TRAFFIC = 2,
};

enum voiceps_cap_type {
	DISABLE_FOR_THIS_AC = 0,
	ENABLE_FOR_THIS_AC = 1,
	ENABLE_FOR_ALL_AC = 2,
};

enum traffic_type {
	TRAFFIC_TYPE_APERIODIC = 0,
	TRAFFIC_TYPE_PERIODIC = 1,
};

/* WMI_SYNCHRONIZE_CMDID */
struct wmi_sync_cmd {
	u8 data_sync_map;
} __packed;

/* WMI_CREATE_PSTREAM_CMDID */
struct wmi_create_pstream_cmd {
	/* msec */
	__le32 min_service_int;

	/* msec */
	__le32 max_service_int;

	/* msec */
	__le32 inactivity_int;

	/* msec */
	__le32 suspension_int;

	__le32 service_start_time;

	/* in bps */
	__le32 min_data_rate;

	/* in bps */
	__le32 mean_data_rate;

	/* in bps */
	__le32 peak_data_rate;

	__le32 max_burst_size;
	__le32 delay_bound;

	/* in bps */
	__le32 min_phy_rate;

	__le32 sba;
	__le32 medium_time;

	/* in octects */
	__le16 nominal_msdu;

	/* in octects */
	__le16 max_msdu;

	u8 traffic_class;

	/* see, enum dir_type */
	u8 traffic_direc;

	u8 rx_queue_num;

	/* see, enum traffic_type */
	u8 traffic_type;

	/* see, enum voiceps_cap_type */
	u8 voice_psc_cap;
	u8 tsid;

	/* 802.1D user priority */
	u8 user_pri;

	/* nominal phy rate */
	u8 nominal_phy;
} __packed;

/* WMI_DELETE_PSTREAM_CMDID */
struct wmi_delete_pstream_cmd {
	u8 tx_queue_num;
	u8 rx_queue_num;
	u8 traffic_direc;
	u8 traffic_class;
	u8 tsid;
} __packed;

/* WMI_SET_CHANNEL_PARAMS_CMDID */
enum wmi_phy_mode {
	WMI_11A_MODE = 0x1,
	WMI_11G_MODE = 0x2,
	WMI_11AG_MODE = 0x3,
	WMI_11B_MODE = 0x4,
	WMI_11GONLY_MODE = 0x5,
};

#define WMI_MAX_CHANNELS        64

/*
 *  WMI_RSSI_THRESHOLD_PARAMS_CMDID
 *  Setting the polltime to 0 would disable polling. Threshold values are
 *  in the ascending order, and should agree to:
 *  (lowThreshold_lowerVal < lowThreshold_upperVal < highThreshold_lowerVal
 *   < highThreshold_upperVal)
 */

struct wmi_rssi_threshold_params_cmd {
	/* polling time as a factor of LI */
	__le32 poll_time;

	/* lowest of upper */
	a_sle16 thresh_above1_val;

	a_sle16 thresh_above2_val;
	a_sle16 thresh_above3_val;
	a_sle16 thresh_above4_val;
	a_sle16 thresh_above5_val;

	/* highest of upper */
	a_sle16 thresh_above6_val;

	/* lowest of bellow */
	a_sle16 thresh_below1_val;

	a_sle16 thresh_below2_val;
	a_sle16 thresh_below3_val;
	a_sle16 thresh_below4_val;
	a_sle16 thresh_below5_val;

	/* highest of bellow */
	a_sle16 thresh_below6_val;

	/* "alpha" */
	u8 weight;

	u8 reserved[3];
} __packed;

/*
 *  WMI_SNR_THRESHOLD_PARAMS_CMDID
 *  Setting the polltime to 0 would disable polling.
 */

struct wmi_snr_threshold_params_cmd {
	/* polling time as a factor of LI */
	__le32 poll_time;

	/* "alpha" */
	u8 weight;

	/* lowest of uppper */
	u8 thresh_above1_val;

	u8 thresh_above2_val;
	u8 thresh_above3_val;

	/* highest of upper */
	u8 thresh_above4_val;

	/* lowest of bellow */
	u8 thresh_below1_val;

	u8 thresh_below2_val;
	u8 thresh_below3_val;

	/* highest of bellow */
	u8 thresh_below4_val;

	u8 reserved[3];
} __packed;

enum wmi_preamble_policy {
	WMI_IGNORE_BARKER_IN_ERP = 0,
	WMI_DONOT_IGNORE_BARKER_IN_ERP
};

struct wmi_set_lpreamble_cmd {
	u8 status;
	u8 preamble_policy;
} __packed;

struct wmi_set_rts_cmd {
	__le16 threshold;
} __packed;

/* WMI_SET_TX_PWR_CMDID */
struct wmi_set_tx_pwr_cmd {
	/* in dbM units */
	u8 dbM;
} __packed;

struct wmi_tx_pwr_reply {
	/* in dbM units */
	u8 dbM;
} __packed;

struct wmi_report_sleep_state_event {
	__le32 sleep_state;
};

enum wmi_report_sleep_status {
	WMI_REPORT_SLEEP_STATUS_IS_DEEP_SLEEP = 0,
	WMI_REPORT_SLEEP_STATUS_IS_AWAKE
};
enum target_event_report_config {
	/* default */
	DISCONN_EVT_IN_RECONN = 0,

	NO_DISCONN_EVT_IN_RECONN
};

/*
 * Used with WMI_AP_SET_NUM_STA_CMDID
 */
struct WMI_AP_NUM_STA_CMD {
	u8     num_sta;
};

/* Command Replies */

/* WMI_GET_CHANNEL_LIST_CMDID reply */
struct wmi_channel_list_reply {
	u8 reserved;

	/* number of channels in reply */
	u8 num_ch;

	/* channel in Mhz */
	__le16 ch_list[1];
} __packed;

/* List of Events (target to host) */
enum wmi_event_id {
	WMI_READY_EVENTID = 0x1001,
	WMI_CONNECT_EVENTID,
	WMI_DISCONNECT_EVENTID,
	WMI_BSSINFO_EVENTID,
	WMI_CMDERROR_EVENTID,
	WMI_REGDOMAIN_EVENTID,
	WMI_PSTREAM_TIMEOUT_EVENTID,
	WMI_NEIGHBOR_REPORT_EVENTID,
	WMI_TKIP_MICERR_EVENTID,
	WMI_SCAN_COMPLETE_EVENTID,	/* 0x100a */
	WMI_REPORT_STATISTICS_EVENTID,
	WMI_RSSI_THRESHOLD_EVENTID,
	WMI_ERROR_REPORT_EVENTID,
	WMI_OPT_RX_FRAME_EVENTID,
	WMI_REPORT_ROAM_TBL_EVENTID,
	WMI_EXTENSION_EVENTID,
	WMI_CAC_EVENTID,
	WMI_SNR_THRESHOLD_EVENTID,
	WMI_LQ_THRESHOLD_EVENTID,
	WMI_TX_RETRY_ERR_EVENTID,	/* 0x1014 */
	WMI_REPORT_ROAM_DATA_EVENTID,
	WMI_TEST_EVENTID,
	WMI_APLIST_EVENTID,
	WMI_GET_WOW_LIST_EVENTID,
	WMI_GET_PMKID_LIST_EVENTID,
	WMI_CHANNEL_CHANGE_EVENTID,
	WMI_PEER_NODE_EVENTID,
	WMI_PSPOLL_EVENTID,
	WMI_DTIMEXPIRY_EVENTID,
	WMI_WLAN_VERSION_EVENTID,
	WMI_SET_PARAMS_REPLY_EVENTID,
	WMI_ADDBA_REQ_EVENTID,		/*0x1020 */
	WMI_ADDBA_RESP_EVENTID,
	WMI_DELBA_REQ_EVENTID,
	WMI_TX_COMPLETE_EVENTID,
	WMI_HCI_EVENT_EVENTID,
	WMI_ACL_DATA_EVENTID,
	WMI_REPORT_SLEEP_STATE_EVENTID,
	WMI_WAPI_REKEY_EVENTID,
	WMI_REPORT_BTCOEX_STATS_EVENTID,
	WMI_REPORT_BTCOEX_CONFIG_EVENTID,
	WMI_GET_PMK_EVENTID,

	/* DFS Events */
	WMI_DFS_HOST_ATTACH_EVENTID,	/* 102B */
	WMI_DFS_HOST_INIT_EVENTID,
	WMI_DFS_RESET_DELAYLINES_EVENTID,
	WMI_DFS_RESET_RADARQ_EVENTID,
	WMI_DFS_RESET_AR_EVENTID,
	WMI_DFS_RESET_ARQ_EVENTID,
	WMI_DFS_SET_DUR_MULTIPLIER_EVENTID,
	WMI_DFS_SET_BANGRADAR_EVENTID,
	WMI_DFS_SET_DEBUGLEVEL_EVENTID,
	WMI_DFS_PHYERR_EVENTID,
	/* CCX Evants */
	WMI_CCX_RM_STATUS_EVENTID,	/* 1035 */

	/* P2P Events */
	WMI_P2P_GO_NEG_RESULT_EVENTID,	/* 1036 */

	WMI_WAC_SCAN_DONE_EVENTID,
	WMI_WAC_REPORT_BSS_EVENTID,
	WMI_WAC_START_WPS_EVENTID,
	WMI_WAC_CTRL_REQ_REPLY_EVENTID,
	WMI_REPORT_WMM_PARAMS_EVENTID,
	WMI_WAC_REJECT_WPS_EVENTID,

	/* More P2P Events */
	WMI_P2P_GO_NEG_REQ_EVENTID,
	WMI_P2P_INVITE_REQ_EVENTID,
	WMI_P2P_INVITE_RCVD_RESULT_EVENTID,
	WMI_P2P_INVITE_SENT_RESULT_EVENTID,
	WMI_P2P_PROV_DISC_RESP_EVENTID,
	WMI_P2P_PROV_DISC_REQ_EVENTID,

	/* RFKILL Events */
	WMI_RFKILL_STATE_CHANGE_EVENTID,
	WMI_RFKILL_GET_MODE_CMD_EVENTID,

	WMI_P2P_START_SDPD_EVENTID,
	WMI_P2P_SDPD_RX_EVENTID,

	/* Special event used to notify host that AR6003
	 * has processed sleep command (needed to prevent
	 * a late incoming credit report from crashing
	 * the system)
	 */
	WMI_SET_HOST_SLEEP_MODE_CMD_PROCESSED_EVENTID,

	WMI_THIN_RESERVED_START_EVENTID = 0x8000,
	/* Events in this range are reserved for thinmode
	 * See wmi_thin.h for actual definitions */
	WMI_THIN_RESERVED_END_EVENTID = 0x8fff,

	WMI_SET_CHANNEL_EVENTID,
	WMI_ASSOC_REQ_EVENTID,

	/* generic ACS event */
	WMI_ACS_EVENTID,
	WMI_STORERECALL_STORE_EVENTID,
	WMI_WOW_EXT_WAKE_EVENTID,
	WMI_GTK_OFFLOAD_STATUS_EVENTID,
	WMI_NETWORK_LIST_OFFLOAD_EVENTID,
	WMI_REMAIN_ON_CHNL_EVENTID,
	WMI_CANCEL_REMAIN_ON_CHNL_EVENTID,
	WMI_TX_STATUS_EVENTID,
	WMI_RX_PROBE_REQ_EVENTID,
	WMI_P2P_CAPABILITIES_EVENTID,
	WMI_RX_ACTION_EVENTID,
	WMI_P2P_INFO_EVENTID,
	/* WPS Events */
	WMI_WPS_GET_STATUS_EVENTID,
	WMI_WPS_PROFILE_EVENTID,

	/* more P2P events */
	WMI_NOA_INFO_EVENTID,
	WMI_OPPPS_INFO_EVENTID,
	WMI_PORT_STATUS_EVENTID,

	/* 802.11w */
	WMI_GET_RSN_CAP_EVENTID,

	WMI_FLOWCTRL_IND_EVENTID,
	WMI_FLOWCTRL_UAPSD_FRAME_DILIVERED_EVENTID,

	/*Socket Translation Events*/
	WMI_SOCKET_RESPONSE_EVENTID,

	WMI_LOG_FRAME_EVENTID,
	WMI_QUERY_PHY_INFO_EVENTID,
	WMI_CCX_ROAMING_EVENTID,

	WMI_P2P_NODE_LIST_EVENTID,
	WMI_P2P_REQ_TO_AUTH_EVENTID,
	WMI_DIAGNOSTIC_EVENTID,	/* diagnostic */
	WMI_DISC_PEER_EVENTID,	/* wifi discovery */
	WMI_BSS_RSSI_INFO_EVENTID,
	WMI_ARGOS_EVENTID,
	WMI_AP_IDLE_CLOSE_TIMEOUT_EVENTID = 0x9020,
	WMI_SEND_DUMMY_DATA_EVENTID,
	WMI_FLUSH_BUFFERED_DATA_EVENTID,
	WMI_WLAN_INFO_LTE_EVENTID,
	WMI_CLIENT_POWER_SAVE_EVENTID,

	/* SB private events */
	WMI_GET_REG_EVENTID,	/* 0x9025 */
	WMI_GET_STAINFO_EVENTID,
	WMI_GET_TXPOW_EVENTID,
	WMI_GET_VERSION_INFO_EVENTID,
	WMI_GET_TESTMODE_EVENTID,
	WMI_RX_PROBE_RESP_EVENTID,
	WMI_ACL_REJECT_EVENTID,
	WMI_GET_WIDIMODE_EVENTID,/* 0x902C */
	WMI_CSA_EVENTID,
	WMI_GET_CTL_EVENTID,

/* merge from olca mainline for align command id - end
 * private commands shall grow back from 0xFFFE
 */
	WMI_EVENTID_LAST = 0xFFFE,
};

struct wmi_ready_event_2 {
	__le32 sw_version;
	__le32 abi_version;
	u8 mac_addr[ETH_ALEN];
	u8 phy_cap;
} __packed;

/* Connect Event */
struct wmi_connect_event {
	union {
		struct {
			__le16 ch;
			u8 bssid[ETH_ALEN];
			__le16 listen_intvl;
			__le16 beacon_intvl;
			__le16 nw_type;
			u8     tx_scheduler_enabled; /* 0x5A means enabled */
			u8     aid;
		} sta;
		struct {
			u8 phymode;
			u8 aid;
			u8 mac_addr[ETH_ALEN];
			u8 auth;
			u8 keymgmt;
			__le16 cipher;
			u8 apsd_info;
			u8 unused[3];
		} ap_sta;
		struct {
			__le16 ch;
			u8 bssid[ETH_ALEN];
			u8 tx_scheduler_enabled; /* 0x5A means enabled */
			u8 aid;
			u8 unused[6];
		} ap_bss;
	} u;
	u8 beacon_ie_len;
	u8 assoc_req_len;
	u8 assoc_resp_len;
	u8 assoc_info[1];
} __packed;

/* Disconnect Event */
enum wmi_disconnect_reason {
	NO_NETWORK_AVAIL = 0x01,

	/* bmiss */
	LOST_LINK = 0x02,

	DISCONNECT_CMD = 0x03,
	BSS_DISCONNECTED = 0x04,
	AUTH_FAILED = 0x05,
	ASSOC_FAILED = 0x06,
	NO_RESOURCES_AVAIL = 0x07,
	CSERV_DISCONNECT = 0x08,
	INVALID_PROFILE = 0x0a,
	DOT11H_CHANNEL_SWITCH = 0x0b,
	PROFILE_MISMATCH = 0x0c,
	CONNECTION_EVICTED = 0x0d,
	IBSS_MERGE = 0xe,
};

#define ATH6KL_COUNTRY_RD_SHIFT        16

struct ath6kl_wmi_regdomain {
	__le32 reg_code;
};

struct wmi_disconnect_event {
	/* reason code, see 802.11 spec. */
	__le16 proto_reason_status;

	/* set if known */
	u8 bssid[ETH_ALEN];

	/* see WMI_DISCONNECT_REASON */
	u8 disconn_reason;

	u8 assoc_resp_len;
	u8 assoc_info[1];
} __packed;

/*
 * BSS Info Event.
 * Mechanism used to inform host of the presence and characteristic of
 * wireless networks present.  Consists of bss info header followed by
 * the beacon or probe-response frame body.  The 802.11 header is no included.
 */
enum wmi_bi_ftype {
	BEACON_FTYPE = 0x1,
	PROBERESP_FTYPE,
	ACTION_MGMT_FTYPE,
	PROBEREQ_FTYPE,
};

#define DEF_SCAN_FOR_ROAM_INTVL			2
#define WMI_ROAM_LRSSI_SCAN_PERIOD		(15 * 1000)	/* secs */
#define WMI_ROAM_LRSSI_ROAM_THRESHOLD	29	/* rssi */
#define WMI_ROAM_LRSSI_SCAN_THRESHOLD (WMI_ROAM_LRSSI_ROAM_THRESHOLD + \
	DEF_SCAN_FOR_ROAM_INTVL)	/* rssi */
#define WMI_ROAM_LRSSI_ROAM_FLOOR		60	/* rssi */

enum wmi_roam_ctrl {
	WMI_FORCE_ROAM = 1,
	WMI_SET_ROAM_MODE,
	WMI_SET_HOST_BIAS,
	WMI_SET_LRSSI_SCAN_PARAMS,
	WMI_SET_HOST_5G_BIAS,
};

enum wmi_roam_mode {
	WMI_DEFAULT_ROAM_MODE = 1, /* RSSI based roam */
	WMI_HOST_BIAS_ROAM_MODE = 2, /* Host bias based roam */
	WMI_LOCK_BSS_MODE = 3, /* Lock to the current BSS */
};

struct bss_bias {
	u8 bssid[ETH_ALEN];
	s8 bias;
} __packed;

struct bss_bias_info {
	u8 num_bss;
	struct bss_bias bss_bias[0];
} __packed;

struct low_rssi_scan_params {
	__le16 lrssi_scan_period;
	a_sle16 lrssi_scan_threshold;
	a_sle16 lrssi_roam_threshold;
	u8 roam_rssi_floor;
	u8 reserved[1];
} __packed;

struct roam_ctrl_cmd {
	union {
		u8 bssid[ETH_ALEN]; /* WMI_FORCE_ROAM */
		u8 roam_mode; /* WMI_SET_ROAM_MODE */
		struct bss_bias_info bss; /* WMI_SET_HOST_BIAS */
		u8 bias5G; /* WMI_SET_HOST_5G_BIAS */
		struct low_rssi_scan_params params; /* WMI_SET_LRSSI_SCAN_PARAMS
						     */
	} __packed info;
	u8 roam_ctrl;
} __packed;

/* BSS INFO HDR version 2.0 */
struct wmi_bss_info_hdr2 {
	__le16 ch; /* frequency in MHz */

	/* see, enum wmi_bi_ftype */
	u8 frame_type;

	u8 snr; /* note: rssi = snr - 95 dBm */
	u8 bssid[ETH_ALEN];
	__le16 ie_mask;
} __packed;

/* Command Error Event */
enum wmi_error_code {
	INVALID_PARAM = 0x01,
	ILLEGAL_STATE = 0x02,
	INTERNAL_ERROR = 0x03,
};

struct wmi_cmd_error_event {
	__le16 cmd_id;
	u8 err_code;
} __packed;

struct wmi_pstream_timeout_event {
	u8 tx_queue_num;
	u8 rx_queue_num;
	u8 traffic_direc;
	u8 traffic_class;
} __packed;

/*
 * The WMI_NEIGHBOR_REPORT Event is generated by the target to inform
 * the host of BSS's it has found that matches the current profile.
 * It can be used by the host to cache PMKs and/to initiate pre-authentication
 * if the BSS supports it.  The first bssid is always the current associated
 * BSS.
 * The bssid and bssFlags information repeats according to the number
 * or APs reported.
 */
enum wmi_bss_flags {
	WMI_DEFAULT_BSS_FLAGS = 0x00,
	WMI_PREAUTH_CAPABLE_BSS = 0x01,
	WMI_PMKID_VALID_BSS = 0x02,
};

struct wmi_neighbor_info {
	u8 bssid[ETH_ALEN];
	u8 bss_flags; /* enum wmi_bss_flags */
} __packed;

struct wmi_neighbor_report_event {
	u8 num_neighbors;
	struct wmi_neighbor_info neighbor[0];
} __packed;

/* TKIP MIC Error Event */
struct wmi_tkip_micerr_event {
	u8 key_id;
	u8 is_mcast;
} __packed;

enum wmi_scan_status {
	WMI_SCAN_STATUS_SUCCESS = 0,
};

/* WMI_SCAN_COMPLETE_EVENTID */
struct wmi_scan_complete_event {
	a_sle32 status;
} __packed;

#define MAX_OPT_DATA_LEN 1400

/*
 * Special frame receive Event.
 * Mechanism used to inform host of the receiption of the special frames.
 * Consists of special frame info header followed by special frame body.
 * The 802.11 header is not included.
 */
struct wmi_opt_rx_info_hdr {
	__le16 ch;
	u8 frame_type;
	s8 snr;
	u8 src_addr[ETH_ALEN];
	u8 bssid[ETH_ALEN];
} __packed;

/* Reporting statistic */
struct tx_stats {
	__le32 pkt;
	__le32 byte;
	__le32 ucast_pkt;
	__le32 ucast_byte;
	__le32 mcast_pkt;
	__le32 mcast_byte;
	__le32 bcast_pkt;
	__le32 bcast_byte;
	__le32 rts_success_cnt;
	__le32 pkt_per_ac[4];
	__le32 err_per_ac[4];

	__le32 err;
	__le32 fail_cnt;
	__le32 retry_cnt;
	__le32 mult_retry_cnt;
	__le32 rts_fail_cnt;
	a_sle32 ucast_rate;
} __packed;

struct rx_stats {
	__le32 pkt;
	__le32 byte;
	__le32 ucast_pkt;
	__le32 ucast_byte;
	__le32 mcast_pkt;
	__le32 mcast_byte;
	__le32 bcast_pkt;
	__le32 bcast_byte;
	__le32 frgment_pkt;

	__le32 err;
	__le32 crc_err;
	__le32 key_cache_miss;
	__le32 decrypt_err;
	__le32 dupl_frame;
	a_sle32 ucast_rate;
} __packed;

struct tkip_ccmp_stats {
	__le32 tkip_local_mic_fail;
	__le32 tkip_cnter_measures_invoked;
	__le32 tkip_replays;
	__le32 tkip_fmt_err;
	__le32 ccmp_fmt_err;
	__le32 ccmp_replays;
} __packed;

struct pm_stats {
	__le32 pwr_save_failure_cnt;
	__le16 stop_tx_failure_cnt;
	__le16 atim_tx_failure_cnt;
	__le16 atim_rx_failure_cnt;
	__le16 bcn_rx_failure_cnt;
} __packed;

struct cserv_stats {
	__le32 cs_bmiss_cnt;
	__le32 cs_low_rssi_cnt;
	__le16 cs_connect_cnt;
	__le16 cs_discon_cnt;
	a_sle16 cs_ave_beacon_rssi;
	__le16 cs_roam_count;
	a_sle16 cs_rssi;
	u8 cs_snr;
	u8 cs_ave_beacon_snr;
	u8 cs_last_roam_msec;
} __packed;

struct wlan_net_stats {
	struct tx_stats tx;
	struct rx_stats rx;
	struct tkip_ccmp_stats tkip_ccmp_stats;
} __packed;

struct arp_stats {
	__le32 arp_received;
	__le32 arp_matched;
	__le32 arp_replied;
} __packed;

struct wlan_wow_stats {
	__le32 wow_pkt_dropped;
	__le16 wow_evt_discarded;
	u8 wow_host_pkt_wakeups;
	u8 wow_host_evt_wakeups;
} __packed;

struct wmi_target_stats {
	__le32 lq_val;
	a_sle32 noise_floor_calib;
	struct pm_stats pm_stats;
	struct wlan_net_stats stats;
	struct wlan_wow_stats wow_stats;
	struct arp_stats arp_stats;
	struct cserv_stats cserv_stats;
} __packed;

/*
 * WMI_RSSI_THRESHOLD_EVENTID.
 * Indicate the RSSI events to host. Events are indicated when we breach a
 * thresold value.
 */
enum wmi_rssi_threshold_val {
	WMI_RSSI_THRESHOLD1_ABOVE = 0,
	WMI_RSSI_THRESHOLD2_ABOVE,
	WMI_RSSI_THRESHOLD3_ABOVE,
	WMI_RSSI_THRESHOLD4_ABOVE,
	WMI_RSSI_THRESHOLD5_ABOVE,
	WMI_RSSI_THRESHOLD6_ABOVE,
	WMI_RSSI_THRESHOLD1_BELOW,
	WMI_RSSI_THRESHOLD2_BELOW,
	WMI_RSSI_THRESHOLD3_BELOW,
	WMI_RSSI_THRESHOLD4_BELOW,
	WMI_RSSI_THRESHOLD5_BELOW,
	WMI_RSSI_THRESHOLD6_BELOW
};

struct wmi_rssi_threshold_event {
	a_sle16 rssi;
	u8 range;
} __packed;

enum wmi_snr_threshold_val {
	WMI_SNR_THRESHOLD1_ABOVE = 1,
	WMI_SNR_THRESHOLD1_BELOW,
	WMI_SNR_THRESHOLD2_ABOVE,
	WMI_SNR_THRESHOLD2_BELOW,
	WMI_SNR_THRESHOLD3_ABOVE,
	WMI_SNR_THRESHOLD3_BELOW,
	WMI_SNR_THRESHOLD4_ABOVE,
	WMI_SNR_THRESHOLD4_BELOW
};

struct wmi_snr_threshold_event {
	/* see, enum wmi_snr_threshold_val */
	u8 range;

	u8 snr;
} __packed;

/* WMI_REPORT_ROAM_TBL_EVENTID */
#define MAX_ROAM_TBL_CAND   5

struct wmi_bss_roam_info {
	a_sle32 roam_util;
	u8 bssid[ETH_ALEN];
	s8 rssi;
	s8 rssidt;
	s8 last_rssi;
	s8 util;
	s8 bias;

	/* for alignment */
	u8 reserved;
} __packed;

struct wmi_target_roam_tbl {
	__le16 roam_mode;
	__le16 num_entries;
	struct wmi_bss_roam_info info[];
} __packed;

/* WMI_CAC_EVENTID */
enum cac_indication {
	CAC_INDICATION_ADMISSION = 0x00,
	CAC_INDICATION_ADMISSION_RESP = 0x01,
	CAC_INDICATION_DELETE = 0x02,
	CAC_INDICATION_NO_RESP = 0x03,
};

#define WMM_TSPEC_IE_LEN   63

struct wmi_cac_event {
	u8 ac;
	u8 cac_indication;
	u8 status_code;
	u8 tspec_suggestion[WMM_TSPEC_IE_LEN];
} __packed;

/* WMI_APLIST_EVENTID */

enum aplist_ver {
	APLIST_VER1 = 1,
};

struct wmi_ap_info_v1 {
	u8 bssid[ETH_ALEN];
	__le16 channel;
} __packed;

union wmi_ap_info {
	struct wmi_ap_info_v1 ap_info_v1;
} __packed;

struct wmi_aplist_event {
	u8 ap_list_ver;
	u8 num_ap;
	union wmi_ap_info ap_list[1];
} __packed;

/* Developer Commands */

/*
 * WMI_SET_BITRATE_CMDID
 *
 * Get bit rate cmd uses same definition as set bit rate cmd
 */
enum wmi_bit_rate {
	RATE_AUTO = -1,
	RATE_1Mb = 0,
	RATE_2Mb = 1,
	RATE_5_5Mb = 2,
	RATE_11Mb = 3,
	RATE_6Mb = 4,
	RATE_9Mb = 5,
	RATE_12Mb = 6,
	RATE_18Mb = 7,
	RATE_24Mb = 8,
	RATE_36Mb = 9,
	RATE_48Mb = 10,
	RATE_54Mb = 11,
	RATE_MCS_0_20 = 12,
	RATE_MCS_1_20 = 13,
	RATE_MCS_2_20 = 14,
	RATE_MCS_3_20 = 15,
	RATE_MCS_4_20 = 16,
	RATE_MCS_5_20 = 17,
	RATE_MCS_6_20 = 18,
	RATE_MCS_7_20 = 19,
	RATE_MCS_0_40 = 20,
	RATE_MCS_1_40 = 21,
	RATE_MCS_2_40 = 22,
	RATE_MCS_3_40 = 23,
	RATE_MCS_4_40 = 24,
	RATE_MCS_5_40 = 25,
	RATE_MCS_6_40 = 26,
	RATE_MCS_7_40 = 27,
};

struct wmi_bit_rate_reply {
	/* see, enum wmi_bit_rate */
	s8 rate_index;
} __packed;

/*
 * WMI_SET_FIXRATES_CMDID
 *
 * Get fix rates cmd uses same definition as set fix rates cmd
 */
struct wmi_fix_rates_reply {
	/* see wmi_bit_rate */
	__le32 fix_rate_mask;
} __packed;

enum roam_data_type {
	/* get the roam time data */
	ROAM_DATA_TIME = 1,
};

struct wmi_target_roam_time {
	__le32 disassoc_time;
	__le32 no_txrx_time;
	__le32 assoc_time;
	__le32 allow_txrx_time;
	u8 disassoc_bssid[ETH_ALEN];
	s8 disassoc_bss_rssi;
	u8 assoc_bssid[ETH_ALEN];
	s8 assoc_bss_rssi;
} __packed;

enum wmi_txop_cfg {
	WMI_TXOP_DISABLED = 0,
	WMI_TXOP_ENABLED
};

struct wmi_set_wmm_txop_cmd {
	u8 txop_enable;
} __packed;

struct wmi_set_keepalive_cmd {
	u8 keep_alive_intvl;
} __packed;

struct wmi_get_keepalive_cmd {
	__le32 configured;
	u8 keep_alive_intvl;
} __packed;

#define MAX_APP_IE_LEN		(255)

struct wmi_set_appie_cmd {
	u8 mgmt_frm_type; /* enum wmi_mgmt_frame_type */
	u8 ie_len;
	u8 ie_info[0];
} __packed;

/* Notify the WSC registration status to the target */
#define WSC_REG_ACTIVE     1
#define WSC_REG_INACTIVE   0

/* use this ID in WMI_DEL_WOW_PATTERN, to clear all patterns */
#define WOW_EXT_FILTER_ID_CLEAR_ALL     0xFFFF

#define WOW_MAX_FILTER_LISTS	 1
#define WOW_MAX_FILTERS_PER_LIST 4
#define WOW_MIN_PATTERN_SIZE	 6
#define WOW_MAX_PATTERN_SIZE	 64
#define WOW_MASK_SIZE		 64

#define MAC_MAX_FILTERS_PER_LIST 4

struct wow_filter {
	u8 wow_valid_filter;
	u8 wow_filter_id;
	u8 wow_filter_size;
	u8 wow_filter_offset;
	u8 wow_filter_mask[WOW_MASK_SIZE];
	u8 wow_filter_pattern[WOW_MAX_PATTERN_SIZE];
} __packed;

#define MAX_IP_ADDRS  2

struct wmi_set_ip_cmd {
	/* IP in network byte order */
	__le32 ips[MAX_IP_ADDRS];
} __packed;

enum ath6kl_wow_filters {
	WOW_FILTER_SSID			= BIT(0),
	WOW_FILTER_OPTION_MAGIC_PACKET  = BIT(2),
	WOW_FILTER_OPTION_EAP_REQ	= BIT(3),
	WOW_FILTER_OPTION_PATTERNS	= BIT(4),
	WOW_FILTER_OPTION_OFFLOAD_ARP	= BIT(5),
	WOW_FILTER_OPTION_OFFLOAD_NS	= BIT(6),
	WOW_FILTER_OPTION_OFFLOAD_GTK	= BIT(7),
	WOW_FILTER_OPTION_8021X_4WAYHS	= BIT(8),
	WOW_FILTER_OPTION_NLO_DISCVRY	= BIT(9),
	WOW_FILTER_OPTION_NWK_DISASSOC	= BIT(10),
	WOW_FILTER_OPTION_GTK_ERROR	= BIT(11),
	WOW_FILTER_OPTION_TEST_MODE	= BIT(15),
};

enum ath6kl_host_mode {
	ATH6KL_HOST_MODE_AWAKE,
	ATH6KL_HOST_MODE_ASLEEP,
};

/* WMI_SET_HOST_SLEEP_MODE_CMDID */
struct wmi_set_host_sleep_mode_cmd {
	__le32 awake;
	__le32 asleep;
} __packed;

enum ath6kl_wow_mode {
	ATH6KL_WOW_MODE_DISABLE,
	ATH6KL_WOW_MODE_ENABLE,
};

/* WMI_SET_WOW_MODE_CMDID */
struct wmi_set_wow_mode_cmd {
	__le32 enable_wow;
	__le32 filter;
	__le16 host_req_delay;
} __packed;

/* WMI_ADD_WOW_PATTERN_CMDID */
struct wmi_add_wow_pattern_cmd {
	u8 filter_list_id;
	u8 filter_size;
	u8 filter_offset;
	u8 filter[1];
} __packed;

/* WMI_ADD_WOW_EXT_PATTERN_CMDID */
struct wmi_add_wow_ext_pattern_cmd {
	u8 filter_list_id;
	u8 filter_id;
	__le16 filter_size;
	u8 filter_offset;
	u8 filter[1];
} __packed;

/* WMI_DEL_WOW_PATTERN_CMDID */
struct wmi_del_wow_pattern_cmd {
	__le16 filter_list_id;
	__le16 filter_id;
} __packed;


/* WMI_SET_AKMP_PARAMS_CMD */
struct wmi_pmkid {
	u8 pmkid[WMI_PMKID_LEN];
} __packed;

/* WMI_GET_PMKID_LIST_CMD  Reply */
struct wmi_pmkid_list_reply {
	__le32 num_pmkid;
	u8 bssid_list[ETH_ALEN][1];
	struct wmi_pmkid pmkid_list[1];
} __packed;

/* WMI_ANT_DIV_CMD */
struct wmi_ant_div_cmd {
	u8	diversity_control;
	u8	main_lna_conf;
	u8	alt_lna_conf;
	u8	fast_div_bias;
	u8	main_gaintb;
	u8	alt_gaintb;
} __packed;

/* WMI_ANT_DIV_STAT */
struct wmi_ant_div_stat {
	u32	scan_start_time;
	u16	total_pkt_count;
	u16	count;
	int	alt_recv_cnt;
	int	main_recv_cnt;
	int	alt_ratio;
	int	main_rssi_avg;
	int	alt_rssi_avg;
	int	curr_main_set;
	int	curr_alt_set;
	int	end_st;
	int	scan;
	int	scan_not_start;
	int	curr_bias;
	u8	main_lna_conf;
	u8	alt_lna_conf;
	u8	fast_div_bias;
} __packed;

/* WMI_ANI_STAT */
struct wmi_ani_stat {
	u8	enable;
	u8	rssi;
	u8	pollcnt;
	u8	noiseImmunityLevel;
	u8	spurImmunityLevel;
	u8	firstepLevel;
	u8	ofdmWeakSigDetectOff;
	u8	cckWeakSigThreshold;
	u32	listenTime;
	u32	ofdmTrigHigh;
	u32	ofdmTrigLow;
	u32	cckTrigHigh;
	u32	cckTrigLow;
	u32	rssiThrLow;
	u32	rssiThrHigh;
	u32	cycleCount;
	u32	ofdmPhyErrCount;
	u32	cckPhyErrCount;
	u32	ofdmPhyErrBase;
	u32	cckPhyErrBase;
	u32	rxFrameCount;
	u32	txFrameCount;
} __packed;

#define WMI_MAX_PMKID_CACHE   8
#define MAX_PMKID_LIST_SIZE   (sizeof(__le32) + WMI_MAX_PMKID_CACHE * \
				(sizeof(struct wmi_pmkid) + ETH_ALEN))

/* WMI_ADDBA_REQ_EVENTID */
struct wmi_addba_req_event {
	u8 tid;
	u8 win_sz;
	__le16 st_seq_no;

	/* f/w response for ADDBA Req; OK (0) or failure (!=0) */
	u8 status;
} __packed;

/* WMI_ADDBA_RESP_EVENTID */
struct wmi_addba_resp_event {
	u8 tid;

	/* OK (0), failure (!=0) */
	u8 status;

	/* three values: not supported(0), 3839, 8k */
	__le16 amsdu_sz;
} __packed;

/* WMI_DELBA_EVENTID
 * f/w received a DELBA for peer and processed it.
 * Host is notified of this
 */
struct wmi_delba_event {
	u8 tid;
	u8 is_peer_initiator;
	__le16 reason_code;
} __packed;

#define PEER_NODE_JOIN_EVENT		0x00
#define PEER_NODE_LEAVE_EVENT		0x01
#define PEER_FIRST_NODE_JOIN_EVENT	0x10
#define PEER_LAST_NODE_LEAVE_EVENT	0x11

struct wmi_peer_node_event {
	u8 event_code;
	u8 peer_mac_addr[ETH_ALEN];
} __packed;

/* Transmit complete event data structure(s) */

/* version 1 of tx complete msg */
struct tx_complete_msg_v1 {
#define TX_COMPLETE_STATUS_SUCCESS 0
#define TX_COMPLETE_STATUS_RETRIES 1
#define TX_COMPLETE_STATUS_NOLINK  2
#define TX_COMPLETE_STATUS_TIMEOUT 3
#define TX_COMPLETE_STATUS_OTHER   4

	u8 status;

	/* packet ID to identify parent packet */
	u8 pkt_id;

	/* rate index on successful transmission */
	u8 rate_idx;

	/* number of ACK failures in tx attempt */
	u8 ack_failures;
} __packed;

struct wmi_tx_complete_event {
	/* no of tx comp msgs following this struct */
	u8 num_msg;

	/* length in bytes for each individual msg following this struct */
	u8 msg_len;

	/* version of tx complete msg data following this struct */
	u8 msg_type;

	/* individual messages follow this header */
	u8 reserved;
} __packed;

/*
 * ------- AP Mode definitions --------------
 */

/*
 * !!! Warning !!!
 * -Changing the following values needs compilation of both driver and firmware
 */
#define AP_MAX_NUM_STA          10

/* Only for chips after McK1.2. */
#define NUM_DEV                 4

/* Sync with target's */
#define NUM_CONN                (AP_MAX_NUM_STA + NUM_DEV)

/* As P2P device port won't enter CONN state, so we omit 1 CONN buffer */
#define WMI_NUM_CONN            (AP_MAX_NUM_STA + NUM_DEV - 1)

/* Spl. AID used to set DTIM flag in the beacons */
#define MCAST_AID               0xFF

#define DEF_AP_COUNTRY_CODE     "US "

/* Used with WMI_AP_SET_NUM_STA_CMDID */

/*
 * Used with WMI_AP_SET_MLME_CMDID
 */

/* MLME Commands */
#define WMI_AP_MLME_ASSOC       1   /* associate station */
#define WMI_AP_DISASSOC         2   /* disassociate station */
#define WMI_AP_DEAUTH           3   /* deauthenticate station */
#define WMI_AP_MLME_AUTHORIZE   4   /* authorize station */
#define WMI_AP_MLME_UNAUTHORIZE 5   /* unauthorize station */

struct wmi_ap_set_mlme_cmd {
	u8 mac[ETH_ALEN];
	__le16 reason;		/* 802.11 reason code */
	u8 cmd;			/* operation to perform (WMI_AP_*) */
} __packed;

struct wmi_ap_set_pvb_cmd {
	__le32 flag;
	__le16 rsvd;
	__le16 aid;
} __packed;

struct wmi_rx_frame_format_cmd {
	/* version of meta data for rx packets <0 = default> (0-7 = valid) */
	u8 meta_ver;

	/*
	 * 1 == leave .11 header intact,
	 * 0 == replace .11 header with .3 <default>
	 */
	u8 dot11_hdr;

	/*
	 * 1 == defragmentation is performed by host,
	 * 0 == performed by target <default>
	 */
	u8 defrag_on_host;

	/* for alignment */
	u8 reserved[1];
} __packed;

/* AP mode events */

/* WMI_PS_POLL_EVENT */
struct wmi_pspoll_event {
	__le16 aid;
} __packed;

struct wmi_per_sta_stat {
	__le32 tx_bytes;
	__le32 tx_pkts;
	__le32 tx_error;
	__le32 tx_discard;
	__le32 rx_bytes;
	__le32 rx_pkts;
	__le32 rx_error;
	__le32 rx_discard;
	u8 aid;
	u8 tx_ucast_rate;

	/* unit is (ms./1024). Target time, not host time. */
	__le16 last_txrx_time;
} __packed;

struct wmi_ap_mode_stat {
	__le32 action;
	struct wmi_per_sta_stat sta[AP_MAX_NUM_STA + 1];
} __packed;

/* End of AP mode definitions */

struct wmi_remain_on_chnl_cmd {
	__le32 freq;
	__le32 duration;
} __packed;

struct wmi_send_action_cmd {
	__le32 id;
	__le32 freq;
	__le32 wait;
	__le16 len;
	u8 data[0];
} __packed;

struct wmi_tx_status_event {
	__le32 id;
	u8 ack_status;
} __packed;

struct wmi_probe_req_report_cmd {
	u8 enable;
} __packed;

struct wmi_probe_resp_report_cmd {
	u8 enable;
} __packed;

struct wmi_disable_11b_rates_cmd {
	u8 disable;
} __packed;

struct wmi_set_appie_extended_cmd {
	u8 role_id;
	u8 mgmt_frm_type;
	u8 ie_len;
	u8 ie_info[0];
} __packed;

struct wmi_remain_on_chnl_event {
	__le32 freq;
	__le32 duration;
} __packed;

struct wmi_cancel_remain_on_chnl_event {
	__le32 freq;
	__le32 duration;
	u8 status;
} __packed;

struct wmi_rx_action_event {
	__le32 freq;
	__le16 len;
	u8 data[0];
} __packed;

struct wmi_p2p_capabilities_event {
	__le16 len;
	u8 data[0];
} __packed;

struct wmi_p2p_rx_probe_req_event {
	__le32 freq;
	__le16 len;
	u8 data[0];
} __packed;

struct wmi_p2p_rx_probe_resp_event {
	__le32 freq;
	__le16 len;
	u8 data[0];
} __packed;
struct wmi_acl_reject_event {
	__le16 len;
	u8 data[0];
} __packed;
#define P2P_FLAG_CAPABILITIES_REQ   (0x00000001)
#define P2P_FLAG_MACADDR_REQ        (0x00000002)
#define P2P_FLAG_HMODEL_REQ         (0x00000004)

struct wmi_get_p2p_info {
	__le32 info_req_flags;
} __packed;

struct wmi_p2p_info_event {
	__le32 info_req_flags;
	__le16 len;
	u8 data[0];
} __packed;

struct wmi_p2p_capabilities {
	u8 go_power_save;
} __packed;

struct wmi_p2p_macaddr {
	u8 mac_addr[ETH_ALEN];
} __packed;

struct wmi_p2p_hmodel {
	u8 p2p_model;
} __packed;

struct wmi_p2p_probe_response_cmd {
	__le32 freq;
	u8 destination_addr[ETH_ALEN];
	__le16 len;
	u8 data[0];
} __packed;

#define RATECTRL_MODE_DEFAULT 0
#define RATECTRL_MODE_PERONLY 1

struct wmi_set_ratectrl_parm_cmd {
	u32 mode;
} __packed;

enum wmi_pkt_cmd {
	WMI_PKTLOG_CMD_DISABLE,
	WMI_PKTLOG_CMD_SETSIZE,
} __packed;

#define PKTLOG_MAX_BYTES 4096

struct wmi_get_pktlog_cmd {
	__le32 nbytes;
	u8 buffer[PKTLOG_MAX_BYTES];
} __packed;


enum wmi_pktlog_option {
	WMI_PKTLOG_OPTION_LOG_TCP_HEADERS = 0x1,
	WMI_PKTLOG_OPTION_TRIGGER_THRUPUT = 0x2,
	WMI_PKTLOG_OPTION_TRIGGER_SACK    = 0x4,
	WMI_PKTLOG_OPTION_TRIGGER_PER     = 0x8,
	WMI_PKTLOG_OPTION_LOG_DIAGNOSTIC  = 0x20
};

enum wmi_pktlog_event {
	WMI_PKTLOG_EVENT_RX  = 0x1,
	WMI_PKTLOG_EVENT_TX  = 0x2,
	WMI_PKTLOG_EVENT_RCF = 0x4, /* Rate Control Find */
	WMI_PKTLOG_EVENT_RCU = 0x8, /* Rate Control Update */
};

struct wmi_enable_pktlog_cmd {
	enum wmi_pktlog_event evlist;
	enum wmi_pktlog_option option;
	__le32 trigger_thresh;
	__le32 trigger_interval;
	__le32 trigger_tail_count;
	__le32 buffer_size;
} __packed;



/* Extended WMI (WMIX)
 *
 * Extended WMIX commands are encapsulated in a WMI message with
 * cmd=WMI_EXTENSION_CMD.
 *
 * Extended WMI commands are those that are needed during wireless
 * operation, but which are not really wireless commands.  This allows,
 * for instance, platform-specific commands.  Extended WMI commands are
 * embedded in a WMI command message with WMI_COMMAND_ID=WMI_EXTENSION_CMDID.
 * Extended WMI events are similarly embedded in a WMI event message with
 * WMI_EVENT_ID=WMI_EXTENSION_EVENTID.
 */
struct wmix_cmd_hdr {
	__le32 cmd_id;
} __packed;

enum wmix_command_id {
	WMIX_DSETOPEN_REPLY_CMDID = 0x2001,
	WMIX_DSETDATA_REPLY_CMDID,
	WMIX_GPIO_OUTPUT_SET_CMDID,
	WMIX_GPIO_INPUT_GET_CMDID,
	WMIX_GPIO_REGISTER_SET_CMDID,
	WMIX_GPIO_REGISTER_GET_CMDID,
	WMIX_GPIO_INTR_ACK_CMDID,
	WMIX_HB_CHALLENGE_RESP_CMDID,
	WMIX_DBGLOG_CFG_MODULE_CMDID,
	WMIX_PROF_CFG_CMDID,	/* 0x200a */
	WMIX_PROF_ADDR_SET_CMDID,
	WMIX_PROF_START_CMDID,
	WMIX_PROF_STOP_CMDID,
	WMIX_PROF_COUNT_GET_CMDID,
};

enum wmix_event_id {
	WMIX_DSETOPENREQ_EVENTID = 0x3001,
	WMIX_DSETCLOSE_EVENTID,
	WMIX_DSETDATAREQ_EVENTID,
	WMIX_GPIO_INTR_EVENTID,
	WMIX_GPIO_DATA_EVENTID,
	WMIX_GPIO_ACK_EVENTID,
	WMIX_HB_CHALLENGE_RESP_EVENTID,
	WMIX_DBGLOG_EVENTID,
	WMIX_PROF_COUNT_EVENTID,
	WMIX_PKTLOG_EVENTID,
	WMIX_RTT_RESP_EVENTID,
};

/*
 * ------Error Detection support-------
 */

/*
 * WMIX_HB_CHALLENGE_RESP_CMDID
 * Heartbeat Challenge Response command
 */
struct wmix_hb_challenge_resp_cmd {
	__le32 cookie;
	__le32 source;
} __packed;

struct ath6kl_wmix_dbglog_cfg_module_cmd {
	__le32 valid;
	__le32 config;
} __packed;

/* End of Extended WMI (WMIX) */


/* Diagnostic WMI (WMID)
 *
 * Diagnostic WMI commands are only for diagnostic tool.
 * Diagnostic WMI commands are encapsulated in a WMI message with
 * WMI_COMMAND_ID=WMI_DIAGNOSTIC_CMDID.
 * Diagnostic WMI events are similarly embedded in a WMI event message with
 * WMI_EVENT_ID=WMI_DIAGNOSTIC_EVENTID.
 */
struct wmid_cmd_hdr {
	__le32 cmd_id;
	__le32 seq_num;
} __packed;

struct wmid_event_set_cmd {
	bool  enable;
} __packed;

struct wmid_macfilter_cmd {
	__le32 type;
	__le32 low;
	__le32 high;
} __packed;

enum wmid_command_id {
	WMID_INTERFERENCE_CMDID = 0x2001,
	WMID_RXTIME_CMDID,
	WMID_FSM_EVENT_CMDID,
	WMID_PWR_SAVE_EVENT_CMDID,
	WMID_MACFILTER_CMDID,
	WMID_STAT_RX_RATE_CMDID,
	WMID_STAT_TX_RATE_CMDID,
};

enum wmid_event_id {
	WMID_START_SCAN_EVENTID = 0x3001,
	WMID_FSM_AUTH_EVENTID,
	WMID_FSM_ASSOC_EVENTID,
	WMID_FSM_DEAUTH_EVENTID,
	WMID_FSM_DISASSOC_EVENTID,
	WMID_STAT_RX_RATE_EVENTID,
	WMID_STAT_TX_RATE_EVENTID,
	WMID_INTERFERENCE_EVENTID,
	WMID_RXTIME_EVENTID,
	WMID_PWR_SAVE_EVENTID,
	WMID_FSM_CONNECT_EVENTID,
	WMID_FSM_DISCONNECT_EVENTID,
};

struct wmid_pwr_save_event {
	__le16  oldState;
	__le16  pmState;
} __packed;
/* End of Diagnostic WMI (WMID) */

enum wmi_sync_flag {
	NO_SYNC_WMIFLAG = 0,

	/* transmit all queued data before cmd */
	SYNC_BEFORE_WMIFLAG,

	/* any new data waits until cmd execs */
	SYNC_AFTER_WMIFLAG,

	SYNC_BOTH_WMIFLAG,

	/* end marker */
	END_WMIFLAG
};

/* Green TX parameters */
struct wmi_green_tx_params {
	u32    enable;
	u8     next_probe_count;
	u8     max_back_off;
	u8     min_gtx_rssi;
	u8     force_back_off;
} __packed;

/* flow control indication parameters */
struct wmi_flowctrl_ind_event {
	u8     num_of_conn;
	u8     ac_map[WMI_NUM_CONN];
	u8     ac_queue_depth[WMI_NUM_CONN * 2];
} __packed;

/* SMPS parameters */
enum WMI_SMPS_OPTION {
	WMI_SMPS_OPTION_MODE          = 0x1,
	WMI_SMPS_OPTION_AUTO          = 0x2,
	WMI_SMPS_OPTION_DATATHRESH    = 0x4,
	WMI_SMPS_OPTION_RSSITHRESH    = 0x8,
};

enum WMI_SMPS_MODE {
	WMI_SMPS_MODE_STATIC    = 0x1,
	WMI_SMPS_MODE_DYNAMIC   = 0x2,
};

struct wmi_config_smps_cmd {
	u8    flags;            /* To indicate which options have changed */
	u8    rssiThresh;
	u8    dataThresh;
	u8    mode;             /* static/dynamic */
	u8    automatic;
} __packed;

struct wmi_config_enable_cmd {
	u8    enable;          /* Enable/disable */
} __packed;

struct wmi_lpl_force_enable_cmd {
	u8    lplPolicy;
	u8    noBlockerDetect;
	u8    noRfbDetect;
	u8    rsvd;
} __packed;

/*command structure for all policy related commands*/
struct wmi_lpl_policy_cmd {
	u64   index;
	u32   value;
} __packed;

struct wmi_lpl_params_cmd {
	u32   rfbPeriod;
	u32   rfbObsDuration;
	u32   blockerBeaconRssi;
	u8    chanOff;
	u32   rfbDiffThold;
	u32   bRssiThold;
	u32   maxBlockerRssi;
} __packed;

/* HT Cap parameters */
struct wmi_set_ht_cap {
	u8 band;
	u8 enable;
	u8 chan_width_40M_supported;
	u8 short_GI_20MHz;
	u8 short_GI_40MHz;
	u8 intolerance_40MHz;
	u8 max_ampdu_len_exp;
} __packed;

#define HT_INFO_OPMODE_PROT_PURE	(0)	/* No protection mode */
#define HT_INFO_OPMODE_PROT_MAY_LEGACY	(1)	/* Nonmember protection mode */
#define HT_INFO_OPMODE_PROT_HT20_ASSOC	(2)	/* HT20 protection mode */
#define HT_INFO_OPMODE_PROT_MIXED	(3)	/* NonHT mixed mode */

/* HT Info parameters */
struct wmi_set_ht_op {
	u8 sta_chan_width;
	u8 ap_ht_info;	/* BO-B1 : ht_opmode */
} __packed;

/* beacon interval */
struct wmi_set_beacon_intvl {
	u16 beacon_interval;
} __packed;

/* hidden ssid */
struct wmi_set_hidden_ssid {
	u8 hidden_ssid;
} __packed;

/* DTIM */
struct wmi_set_dtim_cmd {
	u8 dtim;
} __packed;

/* uAPSD */
enum {
	WMI_AP_APSD_DISABLED = 0,
	WMI_AP_APSD_ENABLED
};

struct wmi_ap_set_apsd_cmd {
	u8 enable;
} __packed;

enum {
	WMI_AP_APSD_NO_DELIVERY_FRAMES_FOR_THIS_TRIGGER =  0x1,
};

struct wmi_ap_apsd_buffered_traffic_cmd {
	u16 aid;
	u16 bitmap;
	u32 flags;
} __packed;

#define GTK_OFFLOAD_KEK_BYTES       16
#define GTK_OFFLOAD_KCK_BYTES       16
#define GTK_REPLAY_COUNTER_BYTES    8

/* set offload parameters, KEK,KCK and replay counter values are valid */
#define WMI_GTK_OFFLOAD_OPCODE_SET     1

/* clear offload parameters */
#define WMI_GTK_OFFLOAD_OPCODE_CLEAR   2

/* get status, generates WMI_GTK_OFFLOAD_STATUS_EVENT  */
#define WMI_GTK_OFFLOAD_OPCODE_STATUS  3

 /* structure to issue GTK offload opcode to set/clear or fetch status
  * NOTE: offload is enabled when WOW options are enabled.
  */
/* WOW_FILTER_OPTION_OFFLOAD_GTK */
struct wmi_gtk_offload_op {
	__le32    flags;                          /* control flags */
	u8     opcode;                         /* opcode */
	u8     kek[GTK_OFFLOAD_KEK_BYTES];     /* key encryption key */
	u8     kck[GTK_OFFLOAD_KCK_BYTES];     /* key confirmation key */

	/* replay counter for re-key */
	u8     replay_counter[GTK_REPLAY_COUNTER_BYTES];
} __packed;

struct  wmi_gtk_offload_status_event {
	__le32    flags;          /* status flags */

	/* number of successful GTK refresh exchanges since last SET operation*/
	__le32    refresh_cnt;

	/* current replay counter */
	u8     replay_counter[GTK_REPLAY_COUNTER_BYTES];
} __packed;

/* TX rate series */
#define WMI_MODE_MAX              8
#define WMI_MAX_RATE_MASK         2

struct wmi_set_tx_select_rate_cmd {
	__le32 rateMasks[WMI_MODE_MAX * WMI_MAX_RATE_MASK];
} __packed;

struct wmi_rsn_cap_cmd {
	__le16   rsn_cap;
} __packed;

#define WMI_MAX_RATE_MASK         2

struct wmi_set_fix_rates_cmd {
	__le32 fixRateMask[WMI_MAX_RATE_MASK];
} __packed;

/* NOA Info. */
struct wmi_noa_descriptor {
	__le32 duration;
	__le32 interval;
	__le32 start_or_offset;
	u8 count_or_type;
} __packed;

struct wmi_noa_info {
	u8 enable;
	u8 count;
	u8 noas[0];		/* struct wmi_noa_descriptor */
} __packed;

/* OppPS Info. */
struct wmi_oppps_info {
	u8 enable;
	u8 ctwin;
} __packed;

/* Port operation */
#define ADD_PORT_FAIL		0x0
#define ADD_PORT_SUCCESS	0x1
#define DEL_PORT_FAIL		0x2
#define DEL_PORT_SUCCESS	0x3

struct wmi_add_port_cmd {
	u8 port_id;
	u8 port_opmode;
	u8 port_subopmode;
	u8 mac_addr[6];
} __packed;

struct wmi_del_port_cmd {
	u8 port_id;
} __packed;

struct wmi_port_status {
	u8 status;
	u8 port_id;
	u8 mac_addr[6];
} __packed;


/* WMI_WOW_EXT_WAKE_EVENTID */
enum WOW_EXT_WAKE_TYPE {
	WOW_EXT_WAKE_TYPE_UNDEF = 0,
	WOW_EXT_WAKE_TYPE_MAGIC,
	WOW_EXT_WAKE_TYPE_PATTERN,
	WOW_EXT_WAKE_TYPE_EAPREQ,
	WOW_EXT_WAKE_TYPE_4WAYHS,
	WOW_EXT_WAKE_TYPE_NETWORK_NLO,
	WOW_EXT_WAKE_TYPE_NETWORK_DISASSOC,
	WOW_EXT_WAKE_TYPE_NETWORK_GTK_OFFL_ERROR,
	WOW_EXT_WAKE_TYPE_IPV4_TCP_SYN,
	WOW_EXT_WAKE_TYPE_IPV6_TCP_SYN,
	WOW_EXT_WAKE_TYPE_WLAN_HB,
	WOW_EXT_WAKE_TYPE_MAXs
};

struct wmi_wow_event_wake_event {
	u16	flags;
	u8	type;
	u8	value;
	u16	packet_length;
	u16	wake_data_length;
	u8	wake_data[1];
} __packed;

#define WMI_MAX_TCP_FILTER_SIZE		32
#define WMI_MAX_UDP_FILTER_SIZE		32

struct wmi_heart_beat_params_cmd {
	u8 enable;
	u8 item;
	u8 session;
} __packed;

struct wmi_heart_beat_tcp_params_cmd {
	__le32 srv_ip;
	__le32 dev_ip;
	__le16 src_port;
	__le16 dst_port;
	__le16 timeout;
	u8 session;
	u8 gateway_mac[ETH_ALEN];
} __packed;

struct wmi_heart_beat_tcp_filter_cmd {
	u8 length;
	u8 offset;
	u8 session;
	u8 filter[WMI_MAX_TCP_FILTER_SIZE];
} __packed;

struct wmi_heart_beat_udp_params_cmd {
	__le32 srv_ip;
	__le32 dev_ip;
	__le16 src_port;
	__le16 dst_port;
	__le16 interval;
	__le16 timeout;
	u8 session;
	u8 gateway_mac[ETH_ALEN];
} __packed;

struct wmi_heart_beat_udp_filter_cmd {
	u8 length;
	u8 offset;
	u8 session;
	u8 filter[WMI_MAX_UDP_FILTER_SIZE];
} __packed;

struct wmi_heart_beat_network_info_cmd {
	u32 device_ip;
	u32 server_ip;
	u32 gateway_ip;
	u8 gateway_mac[ETH_ALEN];
} __packed;

/* wifi discovery */
struct wmi_disc_ie_filter_cmd {
	u8 enable;
	u8 startPos;
	u8 length;
	u8 pattern[1];
} __packed;

struct wmi_disc_mode_cmd {
	__le16 enable;
	__le16 channel; /* channels in Mhz */
	__le32 home_dwell_time;	/* max duration in the home channel(msec) */
	__le32 sleepTime;
	__le32 random;
	__le32 numPeers;
	__le32 peerTimeout;
} __packed;

struct wmi_disc_peer {
	__le32 rssi;
	u8 addr[ETH_ALEN];
} __packed;

struct wmi_disc_peer_event {
	u8 peer_num;
	u8 peer_data[1];
} __packed;

struct wmi_ap_poll_sta_cmd {
	u8 aid;
	u8 reserved[7];
} __packed;

struct wmm_params {
	u8 acm;
	u8 aifsn;
	u8 logcwmin;
	u8 logcwmax;
	u16 txopLimit;
};

struct wmi_report_wmm_params {
	struct wmm_params wmm_params[WMM_NUM_AC];
} __packed;

struct wmi_set_credit_bypass_cmd {
	u8 eid;
	u8 restore;
	u16 threshold;
} __packed;

/* AP ACL */
#define AP_ACL_SIZE             10

#define AP_ACL_DISABLE		0x00
#define AP_ACL_ALLOW_MAC	0x01
#define AP_ACL_DENY_MAC		0x02
#define AP_ACL_RETAIN_LIST_MASK	0x80

struct wmi_ap_acl_policy_cmd {
	u8 policy;
} __packed;

#define ADD_MAC_ADDR		1
#define DEL_MAC_ADDR		2

struct wmi_ap_acl_mac_list_cmd {
	u8 action;
	u8 index;
	u8 mac[ETH_ALEN];
	u8 wildcard;
} __packed;

struct wmi_allow_aggr_cmd {
	u16 tx_allow_aggr;	/* bitmask indicate TID */
	u16 rx_allow_aggr;	/* bitmask indicate TID */
} __packed;

/* AP Admission-Control */
struct wmi_assoc_req_event {
	u8 status;
	u8 rspType;
	u8 assocReq[0];
} __packed;

struct wmi_assoc_resp_send_cmd {
	u8 host_accept;
	u8 host_reasonCode;
	u8 target_status;
	u8 sta_mac_addr[ETH_ALEN];
	u8 rspType;
} __packed;

struct wmi_assoc_req_relay_cmd {
	u8 enable;
} __packed;


/* ARP OFFLOAD */
struct wmi_ipv6_addr {
	u8 address[16];    /* IPV6 in Network Byte Order */
} __packed;

#define WMI_MAX_NS_OFFLOADS           2
#define WMI_MAX_ARP_OFFLOADS          2

/* the tuple entry is valid */
#define WMI_ARPOFF_FLAGS_VALID              (1 << 0)
/* the target mac address is valid */
#define WMI_ARPOFF_FLAGS_MAC_VALID          (1 << 1)
/* remote IP field is valid */
#define WMI_ARPOFF_FLAGS_REMOTE_IP_VALID    (1 << 2)

/* flags
 * IPV4 addresses of the local node
 * source address of the remote node requesting the ARP (qualifier)
 * mac address for this tuple, if not valid, the local MAC is used
 */
struct wmi_arp_offload_tuple {
	u8 flags;
	u8 target_ipaddr[4];
	u8 remote_ipaddr[4];
	u8 target_mac[ETH_ALEN];
} __packed;

/* the tuple entry is valid */
#define WMI_NSOFF_FLAGS_VALID           (1 << 0)
/* the target mac address is valid */
#define WMI_NSOFF_FLAGS_MAC_VALID       (1 << 1)
/* remote IP field is valid */
#define WMI_NSOFF_FLAGS_REMOTE_IP_VALID (1 << 2)

#define WMI_NSOFF_MAX_TARGET_IPS    2

/* flags
 * IPV6 target addresses of the local node
 * multi-cast source IP addresses for receiving solicitations
 * address of remote node requesting the solicitation (qualifier)
 * mac address for this tuple, if not valid, the local MAC is used
 */
struct wmi_ns_offload_tuple {
	u8 flags;
	struct wmi_ipv6_addr target_ipaddr[WMI_NSOFF_MAX_TARGET_IPS];
	struct wmi_ipv6_addr solicitation_ipaddr;
	struct wmi_ipv6_addr remote_ipaddr;
	u8 target_mac[ETH_ALEN];
} __packed;

struct wmi_set_arp_ns_offload_cmd {
	u32 flags;
	struct wmi_ns_offload_tuple ns_tuples[WMI_MAX_NS_OFFLOADS];
	struct wmi_arp_offload_tuple arp_tuples[WMI_MAX_ARP_OFFLOADS];
} __packed;

/* MCC profile setting
 *
 */
enum WMI_MCC_PROFILE {
	WMI_MCC_PROFILE_STA50  = BIT(0),
	WMI_MCC_PROFILE_STA20  = BIT(1),
	WMI_MCC_PROFILE_STA80  = BIT(2),
	WMI_MCC_CTS_ENABLE     = BIT(4),
	WMI_MCC_PSPOLL_ENABLE  = BIT(5),
	WMI_MCC_DUAL_TIME_MASK = BIT(8),
};

struct wmi_set_mcc_profile_cmd {
	u32 mcc_profile;
} __packed;

struct wmi_set_go_sync_cmd {
	u16 freq;
	u8 addr[ETH_ALEN];
	u8 repeat;
	u8 sta_dwell_time;
} __packed;

#define DISALBE_AP_INACTIVE_TIMEMER 0
struct wmi_ap_conn_inact_cmd {
	u32	period;
} __packed;


struct wmi_set_sync_tsf_cmd {
	u32 sync_tsf_p;
} __packed;

#define MIN_BMISS_TIME		1000
#define MAX_BMISS_TIME		5000
#define MIN_BMISS_BEACONS	1
#define MAX_BMISS_BEACONS	50

struct wmi_bmiss_time_cmd {
	u16 bmissTime;
	u16 numBeacons;
} __packed;

enum WMI_SCAN_PLAN_TYPE {
	WMI_SCAN_PLAN_IN_ORDER  = 0,
	WMI_SCAN_PLAN_REVERSE_ORDER = 1,
	WMI_SCAN_PLAN_HOST_ORDER = 2,
};

struct wmi_scan_chan_plan_cmd {
	u32 flag;
	u8 type;
	u8 numChannels;
	__le16 channellist[1];
} __packed;

struct wmi_set_dtim_ext_cmd {
	u8 dtim_ext;
} __packed;

/* 802.11p */
enum WMI_OCB_CHANNEL_OP_TYPE {
	WMI_OCB_CHANNEL_OP_TX = 0,
	WMI_OCB_CHANNEL_OP_RX = 1,
	MAX_WMI_OCB_CHANNEL_OP,
};

struct wmi_set_ocb_channel_op_cmd {
	u32 type;
	u32 channel;
	u32 tx_power;
	u32 tx_rate;
	u32 duration; /* Not yet supported */
	u32 periodicity; /* Not yet supported */
} __packed;

#ifdef CONFIG_SWOW_SUPPORT
struct wmi_set_swol_indoor_info_cmd {
	u8 wlan_swol_indoor;
	u8 swol_indoor_key[16];
	u8 swol_indoor_key_len;
} __packed;

struct wmi_set_swol_outdoor_info_cmd {
	u8 wlan_swol_tcp_enable;

	u16 tcp_src_port;//NC port
	u16 tcp_dst_port;//Push server port
	u32 tcp_seq;	
	u32 ack_seq;
	
	u16 ip_id;//NC ip
	
	u32 device_ip;//NC ip
	u32 server_ip;//Push server ip
	u32 gateway_ip;//gateway ip
	u8 gateway_mac[ETH_ALEN];
    
	u32	asyncId;
	u8	swol_rc4_key[16];
	u8	swol_rc4_key_len;
} __packed;

#endif
enum htc_endpoint_id ath6kl_wmi_get_control_ep(struct wmi *wmi);
void ath6kl_wmi_set_control_ep(struct wmi *wmi, enum htc_endpoint_id ep_id);
int ath6kl_wmi_dix_2_dot3(struct wmi *wmi, struct sk_buff *skb);
int ath6kl_wmi_data_hdr_add(struct wmi *wmi, struct sk_buff *skb,
			    u8 msg_type, u32 flags,
			    enum wmi_data_hdr_data_type data_type,
			    u8 meta_ver, void *tx_meta_info, u8 if_idx);
u8 ath6kl_wmi_determine_user_priority(u8 *pkt, u32 layer2_pri);

int ath6kl_wmi_dot11_hdr_remove(struct wmi *wmi, struct sk_buff *skb);
int ath6kl_wmi_dot3_2_dix(struct sk_buff *skb);
int ath6kl_wmi_implicit_create_pstream(struct wmi *wmi, u8 if_idx,
			struct sk_buff *skb, u32 layer2_priority,
			bool wmm_enabled, u8 *ac,
			u16 *phtc_tag);

int ath6kl_wmi_control_rx(struct wmi *wmi, struct sk_buff *skb);

int ath6kl_wmi_cmd_send(struct wmi *wmi, u8 if_idx, struct sk_buff *skb,
			enum wmi_cmd_id cmd_id, enum wmi_sync_flag sync_flag);

int ath6kl_wmi_connect_cmd(struct wmi *wmi, u8 if_idx,
			   enum network_type nw_type,
			   enum dot11_auth_mode dot11_auth_mode,
			   enum auth_mode auth_mode,
			   enum crypto_type pairwise_crypto,
			   u8 pairwise_crypto_len,
			   enum crypto_type group_crypto,
			   u8 group_crypto_len, int ssid_len, u8 *ssid,
			   u8 *bssid, u16 channel, u32 ctrl_flags);

int ath6kl_wmi_reconnect_cmd(struct wmi *wmi, u8 if_idx, u8 *bssid,
			     u16 channel);
int ath6kl_wmi_disconnect_cmd(struct wmi *wmi, u8 if_idx);
int ath6kl_wmi_startscan_cmd(struct wmi *wmi, u8 if_idx,
			     enum wmi_scan_type scan_type,
			     u32 force_fgscan, u32 is_legacy,
			     u32 home_dwell_time, u32 force_scan_interval,
			     s8 num_chan, u16 *ch_list);
int ath6kl_wmi_scanparams_cmd(struct wmi *wmi, u8 if_idx, u16 fg_start_sec,
			      u16 fg_end_sec, u16 bg_sec,
			      u16 minact_chdw_msec, u16 maxact_chdw_msec,
			      u16 pas_chdw_msec, u8 short_scan_ratio,
			      u8 scan_ctrl_flag, u32 max_dfsch_act_time,
			      u16 maxact_scan_per_ssid);
int ath6kl_wmi_bssfilter_cmd(struct wmi *wmi, u8 if_idx, u8 filter,
			     u32 ie_mask);
int ath6kl_wmi_go_sync_cmd(struct wmi *wmi, u8 if_idx,
				struct wmi_set_go_sync_cmd *gsync);
int ath6kl_wmi_probedssid_cmd(struct wmi *wmi, u8 if_idx, u8 index, u8 flag,
			      u8 ssid_len, u8 *ssid);
int ath6kl_wmi_listeninterval_cmd(struct wmi *wmi, u8 if_idx,
				  u16 listen_interval,
				  u16 listen_beacons);
int ath6kl_wmi_powermode_cmd(struct wmi *wmi, u8 if_idx, u8 pwr_mode);
int ath6kl_wmi_pmparams_cmd(struct wmi *wmi, u8 if_idx, u16 idle_period,
			    u16 ps_poll_num, u16 dtim_policy,
			    u16 tx_wakup_policy, u16 num_tx_to_wakeup,
			    u16 ps_fail_event_policy);
int ath6kl_wmi_ibss_pm_caps_cmd(struct wmi *wmi, u8 if_idx, u8 adhoc_ps_type,
			    u8 ttl,
			    u16 atim_windows,
			    u16 timeout_value);
int ath6kl_wmi_create_pstream_cmd(struct wmi *wmi, u8 if_idx,
				  struct wmi_create_pstream_cmd *pstream);
int ath6kl_wmi_delete_pstream_cmd(struct wmi *wmi, u8 if_idx, u8 traffic_class,
				  u8 tsid);
int ath6kl_wmi_disctimeout_cmd(struct wmi *wmi, u8 if_idx, u8 timeout);

int ath6kl_wmi_set_rts_cmd(struct wmi *wmi, u8 if_idx, u16 threshold);
int ath6kl_wmi_set_lpreamble_cmd(struct wmi *wmi, u8 if_idx, u8 status,
				 u8 preamble_policy);

int ath6kl_wmi_get_challenge_resp_cmd(struct wmi *wmi, u32 cookie, u32 source);
int ath6kl_wmi_config_debug_module_cmd(struct wmi *wmi, u32 valid, u32 config);

int ath6kl_wmi_get_stats_cmd(struct wmi *wmi, u8 if_idx);
int ath6kl_wmi_addkey_cmd(struct wmi *wmi, u8 if_idx, u8 key_index,
			  enum crypto_type key_type,
			  u8 key_usage, u8 key_len,
			  u8 *key_rsc, unsigned int key_rsc_len,
			  u8 *key_material,
			  u8 key_op_ctrl, u8 *mac_addr,
			  enum wmi_sync_flag sync_flag);
#ifdef PMF_SUPPORT
int ath6kl_wmi_addkey_igtk_cmd(struct wmi *wmi, u8 if_idx, u8 key_index,
				u8 key_len, u8 *key_rsc, u8 *key_material,
				enum wmi_sync_flag sync_flag);
#endif
int ath6kl_wmi_add_krk_cmd(struct wmi *wmi, u8 if_idx, u8 *krk);
int ath6kl_wmi_deletekey_cmd(struct wmi *wmi, u8 if_idx, u8 key_index);
int ath6kl_wmi_setpmkid_cmd(struct wmi *wmi, u8 if_idx, const u8 *bssid,
			    const u8 *pmkid, bool set);
int ath6kl_wmi_set_tx_pwr_cmd(struct wmi *wmi, u8 if_idx, u8 dbM);
int ath6kl_wmi_get_tx_pwr_cmd(struct wmi *wmi, u8 if_idx);
int ath6kl_wmi_get_roam_tbl_cmd(struct wmi *wmi);

int ath6kl_wmi_set_wmm_txop(struct wmi *wmi, u8 if_idx, enum wmi_txop_cfg cfg);
int ath6kl_wmi_set_keepalive_cmd(struct wmi *wmi, u8 if_idx,
				 u8 keep_alive_intvl);
int ath6kl_wmi_test_cmd(struct wmi *wmi, void *buf, size_t len);

s32 ath6kl_wmi_get_rate(s8 rate_index);
s32 ath6kl_wmi_get_rate_ar6004(s8 rate_index);

int ath6kl_wmi_set_ip_cmd(struct wmi *wmi, struct wmi_set_ip_cmd *ip_cmd);

/*Wake on Wireless WMI commands*/
int ath6kl_wmi_set_host_sleep_mode_cmd(struct wmi *wmi, u8 if_idx,
				       enum ath6kl_host_mode host_mode);
int ath6kl_wmi_set_wow_mode_cmd(struct wmi *wmi, u8 if_idx,
				enum ath6kl_wow_mode wow_mode,
				u32 filter, u16 host_req_delay);
int ath6kl_wmi_add_wow_pattern_cmd(struct wmi *wmi, u8 if_idx,
				   u8 list_id, u8 filter_size,
				   u8 filter_offset, u8 *filter, u8 *mask);
int ath6kl_wmi_del_wow_pattern_cmd(struct wmi *wmi, u8 if_idx,
				   u16 list_id, u16 filter_id);
int ath6kl_wmi_add_wow_ext_pattern_cmd(struct wmi *wmi, u8 if_idx,
				   u8 list_id, u8 filter_size,
				   u8 filter_id, u8 filter_offset,
				   u8 *filter, u8 *mask);
int ath6kl_wmi_del_all_wow_ext_patterns_cmd(struct wmi *wmi, u8 if_idx,
				__le16 filter_list_id);
int ath6kl_wm_set_gtk_offload(struct wmi *wmi, u8 if_idx,
				u8 *kek, u8 *kck, u8 *replay_ctr);

int ath6kl_wmi_set_roam_ctrl_cmd(struct wmi *wmi,
	u8 fw_vif_idx,	u16  lowrssi_scan_period, u16  lowrssi_scan_threshold,
	u16  lowrssi_roam_threshold,
	u8   roam_rssi_floor);

int ath6kl_wmi_force_roam_cmd(struct wmi *wmi, const u8 *bssid);
int ath6kl_wmi_set_roam_mode_cmd(struct wmi *wmi, enum wmi_roam_mode mode);
int ath6kl_wmi_set_roam_5g_bias_cmd(struct wmi *wmi, u8 bias_5g);

/* AP mode */
int ath6kl_wmi_ap_profile_commit(struct wmi *wmip, u8 if_idx,
				 struct wmi_connect_cmd *p);

int ath6kl_wmi_ap_set_mlme(struct wmi *wmip, u8 if_idx, u8 cmd,
			   const u8 *mac, u16 reason);

int ath6kl_wmi_set_pvb_cmd(struct wmi *wmi, u8 if_idx, u16 aid, bool flag);

int ath6kl_wmi_set_rx_frame_format_cmd(struct wmi *wmi, u8 if_idx,
				       u8 rx_meta_version,
				       bool rx_dot11_hdr, bool defrag_on_host);

int ath6kl_wmi_set_appie_cmd(struct wmi *wmi, u8 if_idx, u8 mgmt_frm_type,
			     const u8 *ie, u8 ie_len);

int ath6kl_wmi_set_rate_ctrl_cmd(struct wmi *wmi,
				u8 if_idx, u32 ratemode);

/* P2P */
int ath6kl_wmi_disable_11b_rates_cmd(struct wmi *wmi, u8 if_idx, bool disable);

int ath6kl_wmi_remain_on_chnl_cmd(struct wmi *wmi, u8 if_idx, u32 freq,
				  u32 dur);

int ath6kl_wmi_send_action_cmd(struct wmi *wmi, u8 if_idx, u32 id, u32 freq,
			       u32 wait, const u8 *data, u16 data_len);

int ath6kl_wmi_send_probe_response_cmd(struct wmi *wmi, u8 if_idx, u32 freq,
				       const u8 *dst, const u8 *data,
				       u16 data_len);

int ath6kl_wmi_send_go_probe_response_cmd(struct wmi *wmi,
	struct ath6kl_vif *vif, const u8 *buf, size_t len, unsigned int freq);

int ath6kl_wmi_probe_report_req_cmd(struct wmi *wmi, u8 if_idx, bool enable);

int ath6kl_wmi_probe_resp_report_req_cmd(struct wmi *wmi, u8 if_idx,
						bool enable);

int ath6kl_wmi_info_req_cmd(struct wmi *wmi, u8 if_idx, u32 info_req_flags);

int ath6kl_wmi_cancel_remain_on_chnl_cmd(struct wmi *wmi, u8 if_idx);

int ath6kl_wmi_set_appie_cmd(struct wmi *wmi, u8 if_idx, u8 mgmt_frm_type,
			     const u8 *ie, u8 ie_len);

struct ath6kl_vif *ath6kl_get_vif_by_index(struct ath6kl *ar, u8 if_idx);
void *ath6kl_wmi_init(struct ath6kl *devt);
void ath6kl_wmi_shutdown(struct wmi *wmi);
void ath6kl_wmi_reset(struct wmi *wmi);

int wmi_rtt_req_meas(struct wmi *wmip, struct nsp_mrqst *pstmrqst, u32 len);
int wmi_rtt_config(struct wmi *wmip, struct nsp_rtt_config *);
int wmi_rtt_req(struct wmi *wmip, enum wmi_cmd_id cmd_id, void *data, u32 len);

int ath6kl_wmi_set_green_tx_params(struct wmi *wmi,
				struct wmi_green_tx_params *params);

int ath6kl_wmi_smps_config(struct wmi *wmi,
				struct wmi_config_smps_cmd *options);

int ath6kl_wmi_smps_enable(struct wmi *wmi,
				struct wmi_config_enable_cmd *options);

int ath6kl_wmi_lpl_enable_cmd(struct wmi *wmi,
	struct wmi_lpl_force_enable_cmd *force_enable_cmd);

int ath6kl_wmi_pktlog_enable_cmd(struct wmi *wmi,
	struct wmi_enable_pktlog_cmd *options);

int ath6kl_wmi_pktlog_disable_cmd(struct wmi *wmi);

int ath6kl_wmi_pktlog_event_rx(struct wmi *wmi, u8 *datap, u32 len);

int ath6kl_wmi_simple_cmd(struct wmi *wmi, u8 if_idx, enum wmi_cmd_id cmd_id);

struct sk_buff *ath6kl_wmi_get_new_buf(u32 size);

int ath6kl_wmi_abort_scan_cmd(struct wmi *wmi, u8 if_idx);

int ath6kl_wmi_set_ht_cap_cmd(struct wmi *wmi, u8 if_idx,
	u8 band, u8 chan_width_40M_supported,
	u8 short_GI, u8 intolerance_40MHz);

int ath6kl_wmi_set_ht_op_cmd(struct wmi *wmi, u8 if_idx,
	u8 sta_chan_width, u8 opmode);

int ath6kl_wmi_set_dtim_cmd(struct wmi *wmi, u8 if_idx, u32 dtim);

int ath6kl_wmi_ap_set_apsd(struct wmi *wmi, u8 if_idx, u8 enable);

int ath6kl_wmi_set_apsd_buffered_traffic_cmd(struct wmi *wmi, u8 if_idx,
						u16 aid, u16 bitmap, u32 flags);

int ath6kl_wmi_set_tx_select_rates_on_all_mode(struct wmi *wmi,
	u8 if_idx, u64 mask);

int ath6kl_wmi_set_hidden_ssid_cmd(struct wmi *wmi, u8 if_idx, u8 hidden_ssid);

int ath6kl_wmi_set_beacon_interval_cmd(struct wmi *wmi, u8 if_idx,
	u32 beacon_interval);

int ath6kl_wmi_set_rsn_cap(struct wmi *wmi, u8 if_idx, u16 rsn_cap);
int ath6kl_wmi_get_rsn_cap(struct wmi *wmi, u8 if_idx);
int ath6kl_wmi_get_pmkid_list(struct wmi *wmi, u8 if_idx);

int ath6kl_wmi_set_fix_rates(struct wmi *wmi, u8 if_idx, u64 mask);

int ath6kl_wmi_add_port_cmd(struct wmi *wmi, struct ath6kl_vif *vif,
	u8 opmode, u8 subopmode);
int ath6kl_wmi_del_port_cmd(struct wmi *wmi, u8 if_idx, u8 port_id);

int ath6kl_wmi_set_noa_cmd(struct wmi *wmi, u8 if_idx,
	u8 count, u32 start, u32 duration, u32 interval);
int ath6kl_wmi_set_oppps_cmd(struct wmi *wmi, u8 if_idx,
				u8 enable, u8 ctwin);

int ath6kl_wmi_set_heart_beat_params(struct wmi *wmi, u8 if_idx,
	u8 enable, u8 item, u8 session);
int ath6kl_wmi_heart_beat_set_tcp_params(struct wmi *wmi, u8 if_idx,
	u16 src_port, u16 dst_port, u32 srv_ip, u32 dev_ip, u16 timeout,
	u8 session, u8 *gateway_mac);
int ath6kl_wmi_heart_beat_set_tcp_filter(struct wmi *wmi, u8 if_idx,
	u8 *filter, u8 length, u8 offset, u8 session);
int ath6kl_wmi_heart_beat_set_udp_params(struct wmi *wmi, u8 if_idx,
	u16 src_port, u16 dst_port, u32 srv_ip, u32 dev_ip, u16 interval,
	u16 timeout, u8 session, u8 *gateway_mac);
int ath6kl_wmi_heart_beat_set_udp_filter(struct wmi *wmi, u8 if_idx,
	u8 *filter, u8 length, u8 offset, u8 session);

int ath6kl_wmi_disc_ie_cmd(struct wmi *wmi, u8 if_idx, u8 enable,
	u8 startPos, u8 *pattern, u8 length);
int ath6kl_wmi_disc_mode_cmd(struct wmi *wmi, u8 if_idx, u16 enable,
	u16 channel, u32 home_dwell_time, u32 sleepTime, u32 random,
	u32 numPeers, u32 peerTimeout);

int ath6kl_wmi_ap_poll_sta(struct wmi *wmi, u8 if_idx, u8 aid);
int ath6kl_wmi_ap_acl_policy(struct wmi *wmi, u8 if_idx, u8 policy);
int ath6kl_wmi_ap_acl_mac_list(struct wmi *wmi, u8 if_idx,
	u8 idx, u8 *mac_addr, u8 action);
int ath6kl_wmi_allow_aggr_cmd(struct wmi *wmi, u8 if_idx,
	u16 tx_tid_mask, u16 rx_tid_mask);
int ath6kl_wmi_set_credit_bypass(struct wmi *wmi, u8 if_idx, u8 eid,
	u8 restore, u16 threshold);
int ath6kl_wmi_set_arp_offload_ip_cmd(struct wmi *wmi, u8 if_idx, u8 *ip_addrs);
int ath6kl_wmi_set_mcc_profile_cmd(struct wmi *wmi, u32 mcc_profile);
int ath6kl_wmi_set_seamless_mcc_scc_switch_freq_cmd(struct wmi *wmi, u32 freq);

int ath6kl_wmi_set_regdomain_cmd(struct wmi *wmi, const char *alpha2);
int ath6kl_wmi_set_inact_cmd(struct wmi *wmi, u32 inacperiod);
int ath6kl_wmi_send_assoc_resp_cmd(struct wmi *wmi, u8 if_idx,
	bool accept, u8 reason_code, u8 fw_status, u8 *sta_mac, u8 req_type);
int ath6kl_wmi_set_assoc_req_relay_cmd(struct wmi *wmi, u8 if_idx,
	bool enabled);

int ath6kl_wmi_set_ap_num_sta_cmd(struct wmi *wmi, u8 if_idx, u8 sta_nums);
int ath6kl_wmi_set_antdivcfg(struct wmi *wmi, u8 if_idx, u8 diversity_control);
int ath6kl_wmi_antdivstate_event_rx(struct ath6kl_vif *vif, u8 *datap, int len);

int ath6kl_wmi_antdivstate_debug_event_rx(struct ath6kl_vif *vif,
	u8 *datap, int len);
int ath6kl_antdiv_stat_debug(struct ath6kl *ar, u8 *buf, int buf_len);

int ath6kl_wmi_anistate_event_rx(struct ath6kl_vif *vif, u8 *datap, int len);

int ath6kl_wmi_anistate_debug_event_rx(struct ath6kl_vif *vif,
	u8 *datap, int len);
int ath6kl_wmi_anistate_enable(struct wmi *wmi,
	struct wmi_config_enable_cmd *options);
int ath6kl_ani_stat_debug(struct ath6kl *ar, u8 *buf, int buf_len);
int ath6kl_wmi_set_bmiss_time(struct wmi *wmi, u8 if_idx, u16 numBeacon);

int ath6kl_wmi_set_scan_chan_plan(struct wmi *wmi, u8 if_idx,
					u8 type, u8 numChan, u16 *chanList);
int ath6kl_wmi_set_dtim_ext(struct wmi *wmi, u8 dtim_ext);

/* 802.11p */
int ath6kl_wmi_set_ocb_channel_op(struct wmi *wmi, u8 if_idx, u8 type,
	u16 channel, u16 tx_power, u16 tx_rate, u32 duration, u32 periodicity);
#ifdef CONFIG_SWOW_SUPPORT
int ath6kl_wmi_set_swol_indoor_info_cmd(struct wmi *wmi, u8 enable, 
	u16 length, u8 *password);
int ath6kl_wmi_set_swol_outdoor_info_cmd(struct wmi *wmi, u8 enable, 
	u16 length, u8 *password);
#endif//swow

int ath6kl_wmi_set_sync_tsf_cmd(struct wmi *wmi, u32 sync_tsf_p);
#endif /* WMI_H */
