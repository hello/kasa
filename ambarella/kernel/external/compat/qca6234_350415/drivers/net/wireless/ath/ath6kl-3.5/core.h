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

#ifndef CORE_H
#define CORE_H

#ifdef CE_SUPPORT
#ifdef CONFIG_ANDROID
#undef CONFIG_ANDROID
#endif
#endif

#include <linux/etherdevice.h>
#include <linux/rtnetlink.h>
#include <linux/firmware.h>
#include <linux/sched.h>
#include <linux/circ_buf.h>
#include <linux/ip.h>
#include <net/cfg80211.h>
#ifdef CONFIG_ANDROID
#ifdef CONFIG_HAS_WAKELOCK
#include <linux/wakelock.h>
#include <asm/mach-types.h>
#endif
#endif
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif
#include "htc.h"
#include "wmi.h"
#include "bmi.h"
#include "target.h"
#include "wmi_btcoex.h"
#ifdef ACS_SUPPORT
#include "acs.h"
#endif
#include "htcoex.h"
#include "p2p.h"
#include "ap.h"
#include "reg.h"
#include <linux/wireless.h>
#include <linux/interrupt.h>

#define MAKE_STR(symbol) #symbol
#define TO_STR(symbol) MAKE_STR(symbol)

/* The script (used for release builds) modifies the following line. */
#define __BUILD_VERSION_ (3.5.0.415)

#define DRV_VERSION		TO_STR(__BUILD_VERSION_)

/* enable diagnostic by default in this version */
#define ATH6KL_DIAGNOSTIC 1
#ifdef ATH6KL_DIAGNOSTIC
#include "diagnose.h"
#endif

#ifndef CONFIG_ATH6KL_MCC
#define CONFIG_ATH6KL_MCC
#endif

#ifdef CONFIG_ATH6KL_UB134
#ifndef CONFIG_ATH6KL_UDP_TPUT_WAR
#define CONFIG_ATH6KL_UDP_TPUT_WAR
#endif
#ifndef ATH6KL_HSIC_RECOVER
#define ATH6KL_HSIC_RECOVER
#endif
#endif

#ifdef CONFIG_ATH6KL_MCC
#define ATH6KL_MODULEP2P_DEF_MODE			\
	(ATH6KL_MODULEP2P_P2P_ENABLE |			\
	 ATH6KL_MODULEP2P_CONCURRENT_ENABLE_DEDICATE |	\
	 ATH6KL_MODULEP2P_CONCURRENT_MULTICHAN |	\
	 ATH6KL_MODULEP2P_P2P_WISE_SCAN)
#else
#define ATH6KL_MODULEP2P_DEF_MODE			\
	(ATH6KL_MODULEP2P_P2P_ENABLE |			\
	 ATH6KL_MODULEP2P_CONCURRENT_ENABLE_DEDICATE |	\
	 ATH6KL_MODULEP2P_P2P_WISE_SCAN)
#endif

/* ce and not ce are seperate */
#ifndef CE_SUPPORT
#ifdef CONFIG_ANDROID
#define ATH6KL_MODULE_DEF_DEBUG_QUIRKS			\
	(ATH6KL_MODULE_DISABLE_WMI_SYC |		\
	ATH6KL_MODULE_DISABLE_RX_AGGR_DROP |		\
	ATH6KL_MODULES_ANI_ENABLE |			\
	ATH6KL_MODULE_ENABLE_FW_CRASH_NOTIFY |       \
	 0)
#else
#define ATH6KL_MODULE_DEF_DEBUG_QUIRKS			\
	(ATH6KL_MODULE_DISABLE_WMI_SYC |		\
	ATH6KL_MODULE_DISABLE_RX_AGGR_DROP |		\
	ATH6KL_MODULES_ANI_ENABLE |			\
	 0)
#endif
#else
#define ATH6KL_MODULE_DEF_DEBUG_QUIRKS			\
	(ATH6KL_MODULE_DISABLE_WMI_SYC |		\
	ATH6KL_MODULE_DISABLE_SKB_DUP |			\
	0)
#endif

#ifndef ATH6KL_MODULEP2P_DEF_MODE
#define ATH6KL_MODULEP2P_DEF_MODE	(0)
#endif

#ifndef ATH6KL_MODULEVAP_DEF_MODE
#define ATH6KL_MODULEVAP_DEF_MODE	(0)
#endif

#ifndef ATH6KL_MODULE_DEF_DEBUG_QUIRKS
#define ATH6KL_MODULE_DEF_DEBUG_QUIRKS	(0)
#endif

#ifndef ATH6KL_DEVNAME_DEF_P2P
#define ATH6KL_DEVNAME_DEF_P2P		"p2p%d"
#endif

#ifndef ATH6KL_DEVNAME_DEF_AP
#define ATH6KL_DEVNAME_DEF_AP		"ap%d"
#endif

#ifndef ATH6KL_DEVNAME_DEF_STA
#define ATH6KL_DEVNAME_DEF_STA		"sta%d"
#endif

#if (LINUX_VERSION_CODE > KERNEL_VERSION(3,7,0))
#ifndef ATH6KL_SUPPORT_NETLINK_KERNEL3_7
#define ATH6KL_SUPPORT_NETLINK_KERNEL3_7
#endif
#endif

/*
 * Native kernel cfg80211 support.
 * Current ath6kl code base is kernel 3.2
 *
 * For ATH6KL_SUPPORT_NETLINK_KERNEL3_6 & ATH6KL_SUPPORT_NETLINK_KERNEL3_7,
 * not really need if compat.ko used.
 *
 * PLEASE CHANGE THESE FLAGS TO MEET YOUR BSP IF THE BSP USE
 * SPECIFIC CFG80211 CODE.
 */
#ifdef ATH6KL_SUPPORT_NL80211_KERNEL3_9		/* Kernel 3.9 series */
#ifndef ATH6KL_SUPPORT_NL80211_KERNEL3_8
#define ATH6KL_SUPPORT_NL80211_KERNEL3_8
#endif
#endif

#ifdef ATH6KL_SUPPORT_NL80211_KERNEL3_8		/* Kernel 3.8 series */
#ifndef ATH6KL_SUPPORT_NL80211_KERNEL3_7
#define ATH6KL_SUPPORT_NL80211_KERNEL3_7
#endif
#endif

#ifdef ATH6KL_SUPPORT_NL80211_KERNEL3_7		/* Kernel 3.7 series */
#ifndef ATH6KL_SUPPORT_NETLINK_KERNEL3_7
#define ATH6KL_SUPPORT_NETLINK_KERNEL3_7
#endif

#ifndef ATH6KL_SUPPORT_NL80211_KERNEL3_6
#define ATH6KL_SUPPORT_NL80211_KERNEL3_6
#endif
#endif

#ifdef ATH6KL_SUPPORT_NL80211_KERNEL3_6		/* Kernel 3.6 series */
#ifndef ATH6KL_SUPPORT_NETLINK_KERNEL3_6
#define ATH6KL_SUPPORT_NETLINK_KERNEL3_6
#endif

#ifndef ATH6KL_SUPPORT_NL80211_KERNEL3_5
#define ATH6KL_SUPPORT_NL80211_KERNEL3_5
#endif
#endif

#ifdef ATH6KL_SUPPORT_NL80211_KERNEL3_5		/* Kernel 3.5 series */
#ifndef ATH6KL_SUPPORT_NL80211_KERNEL3_4
#define ATH6KL_SUPPORT_NL80211_KERNEL3_4
#endif
#endif

/*
 * NOT to touch origional branches's Makefile and therefore turn-on
 * ATH6KL_SUPPORT_NL80211_QCA by default for all Android releases.
 */
#ifdef CONFIG_ANDROID
#define ATH6KL_SUPPORT_NL80211_QCA

#if defined(ATH6KL_SUPPORT_NL80211_KERNEL3_4) ||	\
	defined(ATH6KL_SUPPORT_NL80211_KERNEL3_6)
/*
 * New Android (after JB_2.5/JB_MR1) use build-in cfg80211.ko and
 * need to remove QCA's cfg80211 implementations.
 */
#undef ATH6KL_SUPPORT_NL80211_QCA
#endif

#ifndef machine_is_apq8064_dma
#define machine_is_apq8064_dma() 0
#endif

#ifndef machine_is_apq8064_bueller
#define machine_is_apq8064_bueller() 0
#endif
#endif

/*
 * No good way to support every version of cfg80211.ko so far, especially
 * we sometimes change the default behavior of public cfg80211.ko.
 *
 * To let driver code could compiler between different cfg80211.ko and using
 * these flags now.
 * Newer NL80211.h may has the similiar definitions to used by application
 * or the driver but, unfortunately, all are not we want.
 *
 * Please add ATH6KL_SUPPORT_NL80211_QCA or ATH6KL_SUPPORT_NL80211_KERNEL3_x
 * in your BSP's Makefiles based on the cfg80211.ko you used.
 */
#ifdef ATH6KL_SUPPORT_NL80211_QCA
/*
 * Means the cfg80211.ko is specific version and had QCA's special
 * implementations.
 *
 * NL80211_CMD_GET_WOWLAN_QCA: for QCA WoW command.
 * NL80211_CMD_BTCOEX_QCA: for QCA BTCoext NL80211 command and event.
 *
 * TODO : remove these special implementations.
 */
#define NL80211_CMD_GET_WOWLAN_QCA
#define NL80211_CMD_BTCOEX_QCA
#endif
#ifdef ATH6KL_SUPPORT_NL80211_KERNEL3_4
/*
 * NL80211_CMD_START_STOP_AP: for new call-back and structures.
 * NL80211_ATTR_WIPHY_RX_SIGNAL_DBM: for new API's parameter.
 * CFG80211_WIPHY_FLAG_HAS_REMAIN_ON_CHANNEL: new flag consist w/ RoC oper.
 * NL80211_ATTR_AP_INACTIVITY_TIMEOUT: new attribute to config AP keep-alive.
 */
#define NL80211_CMD_START_STOP_AP
#define NL80211_ATTR_WIPHY_RX_SIGNAL_DBM
#define CFG80211_WIPHY_FLAG_HAS_REMAIN_ON_CHANNEL
#define NL80211_ATTR_AP_INACTIVITY_TIMEOUT
#endif
#ifdef ATH6KL_SUPPORT_NL80211_KERNEL3_5
/*
 * NL80211_CMD_OPER_CH_SWITCH_NOTIFY: report working frequency back to user.
 */
#define NL80211_CMD_OPER_CH_SWITCH_NOTIFY
#endif
#ifdef ATH6KL_SUPPORT_NL80211_KERNEL3_6
/*
 * CFG80211_NETDEV_REPLACED_BY_WDEV: using wireless_dev pointer instead
 *                                   of net_device.
 * CFG80211_NO_SET_CHAN_OPERATION: no support set_channel call-back.
 */
#define CFG80211_NETDEV_REPLACED_BY_WDEV
#define CFG80211_NO_SET_CHAN_OPERATION
#endif
#ifdef ATH6KL_SUPPORT_NL80211_KERNEL3_8
/*
 * NL80211_WIPHY_FEATURE_SCAN_FLUSH: flush scan cache before scan if need.
 * CFG80211_TX_POWER_PER_WDEV: support set/get TX power per wdev context.
 * CFG80211_REMOVE_ROC_CHAN_TYPE: remove channel type of RoC & MgmtTx related
 *                                call-back and APIs.
 * CFG80211_NEW_CHAN_DEFINITION: summary origional channel definition and
 *                               channel type into new channel definitaion.
 * CFG80211_SAFE_BSS_INFO_ACCESS: to fix BSS IE race access problem.
 */
#define NL80211_WIPHY_FEATURE_SCAN_FLUSH
#define CFG80211_TX_POWER_PER_WDEV
#define CFG80211_REMOVE_ROC_CHAN_TYPE
#define CFG80211_NEW_CHAN_DEFINITION
#define CFG80211_SAFE_BSS_INFO_ACCESS
#endif
#ifdef ATH6KL_SUPPORT_NL80211_KERNEL3_9
/*
 * CFG80211_VOID_REG_NOTIFIER: The void function as *reg_notifier callback.
 * CFG80211_NEW_REF_BSS_OPER: Preparation for using spinlock.
 * NL80211_CMD_SET_AP_MAC_ACL: Support AP ACL.
 */
#define CFG80211_VOID_REG_NOTIFIER
#define CFG80211_NEW_REF_BSS_OPER
#define NL80211_CMD_SET_AP_MAC_ACL
#endif

#define ATH6KL_SUPPORT_WIFI_DISC 1
#define ATH6KL_SUPPORT_WIFI_KTK  1
#define ATH6KL_SUPPORT_WLAN_HB   1

#define MAX_ATH6KL                        1
#define ATH6KL_MAX_RX_BUFFERS             16
#define ATH6KL_BUFFER_SIZE                1664
#define ATH6KL_MAX_AMSDU_RX_BUFFERS       4
#define ATH6KL_AMSDU_REFILL_THRESHOLD     3
#define ATH6KL_AMSDU_BUFFER_SIZE     (WMI_MAX_AMSDU_RX_DATA_FRAME_LENGTH + 128)
#define MAX_MSDU_SUBFRAME_PAYLOAD_LEN	1508
#define MIN_MSDU_SUBFRAME_PAYLOAD_LEN	36

#define USER_SAVEDKEYS_STAT_INIT     0
#define USER_SAVEDKEYS_STAT_RUN      1

#define ATH6KL_TX_TIMEOUT      10
#define ATH6KL_MAX_ENDPOINTS   4
#define MAX_NODE_NUM           15

/* Extra bytes for htc header alignment */
#define ATH6KL_HTC_ALIGN_BYTES 3

/*
 * MAX_HI_COOKIE_NUM are reserved for high priority traffic.
 * Need more cookies for WMM purpose.
 */
#define MAX_DEF_COOKIE_NUM                400
#define MAX_HI_COOKIE_NUM                 40	/* 10% of MAX_COOKIE_NUM */
#define MAX_VIF_COOKIE_NUM                200   /* 50% of MAX_COOKIE_NUM */

#define MAX_COOKIE_DATA_NUM	(MAX_DEF_COOKIE_NUM + MAX_HI_COOKIE_NUM)
#define MAX_COOKIE_CTRL_NUM	(64 + 2)

#define MAX_DEFAULT_SEND_QUEUE_DEPTH      (MAX_DEF_COOKIE_NUM / WMM_NUM_AC)
#define MAX_DEFAULT_SEND_QUEUE_DEPTH_CTRL MAX_COOKIE_CTRL_NUM

#define DISCON_TIMER_INTVAL               10000  /* in msec */
#define A_DEFAULT_LISTEN_INTERVAL         100
#define A_MAX_WOW_LISTEN_INTERVAL         1000

#define ATH6KL_DISCONNECT_TIMEOUT	  3
#define ATH6KL_SEAMLESS_ROAMING_DISCONNECT_TIMEOUT	10

/* Channel dwell time in fg scan */
#define ATH6KL_FG_SCAN_INTERVAL           100 /* in msec */

#define ATH6KL_SCAN_ACT_DEWELL_TIME	20 /* in ms. */
#define ATH6KL_SCAN_PAS_DEWELL_TIME	50 /* in ms. */
#define ATH6KL_SCAN_PROBE_PER_SSID	1
#define ATH6KL_SCAN_FG_MAX_PERIOD	(5)	/* in sec. */
#define ATH6KL_SCAN_PAS_DEWELL_TIME_WITHOUT_ROAM 100 /* in ms. */

/* Remain-on-channel */
#define ATH6KL_ROC_MAX_PERIOD		(5)	/* in sec. */

/* scan time out */
#define ATH6KL_SCAN_TIMEOUT_LONG (8 * HZ)  /* in sec. */
#define ATH6KL_SCAN_TIMEOUT_SHORT (5 * HZ) /* in sec. */
#define ATH6KL_SCAN_TIMEOUT_WITHOUT_ROAM (20 * HZ)  /* in sec. */

/* 4 way-handshake protect */
#define ATH6KL_HANDSHAKE_PROC_TIMEOUT (3 * HZ) /* in sec. */

/* includes also the null byte */
#define ATH6KL_FIRMWARE_MAGIC               "QCA-ATH6KL"

/* Default HT CAP Parameters */
#define ATH6KL_24GHZ_HT40_DEF_STA_IDX		(0)	/* only STA interface */
#define ATH6KL_24GHZ_HT40_DEF_WIDTH		(1)	/* HT40 enabled */
#define ATH6KL_24GHZ_HT40_DEF_SGI		(1)	/* SGI enabled */
#define ATH6KL_24GHZ_HT40_DEF_INTOLR40		(0)	/* disabled */
#define ATH6KL_5GHZ_HT40_DEF_WIDTH		(1)	/* HT40 enabled */
#define ATH6KL_5GHZ_HT40_DEF_SGI		(1)	/* SGI enabled */
#define ATH6KL_5GHZ_HT40_DEF_INTOLR40		(0)	/* disabled */

/* default parameters for GeenTX */
#define ATH6KL_GTX_NEXT_PROBE_COUNT 20
#define ATH6KL_GTX_MAX_BACK_OFF 6
#define ATH6KL_GTX_MIN_RSSI 35
#define ATH6KL_GTX_FORCE_BACKOFF 0

/* delay around 29ms on 1/4 msg in wpa/wpa2 to avoid racing with roam
* event in certain platform
*/
#define ATH6KL_EAPOL_DELAY_REPORT_IN_HANDSHAKE	(msecs_to_jiffies(30))

/*
 * 1250 ms. = RX EAPOL +
 *            ATH6KL_EAPOL_DELAY_REPORT_IN_HANDSHAKE +
 *            supplicant procressing time +
 *            TX EAPOL +
 *            wait next RX EAPOL
 * or,
 *          = TX EAPOL +
 *            peer's supplicant procressing time +
 *            RX EAPOL +
 *            wait next TX EAPOL
 */
#define ATH6KL_SCAN_PREEMPT_IN_HANDSHAKE	(msecs_to_jiffies(1250))

/* default roam mode for different situation */
#if (defined(CONFIG_ANDROID) && !defined(ATH6KL_USB_ANDROID_CE))
#define ATH6KL_SDIO_DEFAULT_ROAM_MODE \
	ATH6KL_MODULEROAM_NO_LRSSI_SCAN_AT_MULTI
#define ATH6KL_USB_DEFAULT_ROAM_MODE \
	ATH6KL_MODULEROAM_NO_LRSSI_SCAN_AT_MULTI
#else
#define ATH6KL_SDIO_DEFAULT_ROAM_MODE \
	ATH6KL_MODULEROAM_DISABLE_LRSSI_SCAN
#define ATH6KL_USB_DEFAULT_ROAM_MODE \
		ATH6KL_MODULEROAM_DISABLE
#endif

enum ath6kl_fw_ie_type {
	ATH6KL_FW_IE_FW_VERSION = 0,
	ATH6KL_FW_IE_TIMESTAMP = 1,
	ATH6KL_FW_IE_OTP_IMAGE = 2,
	ATH6KL_FW_IE_FW_IMAGE = 3,
	ATH6KL_FW_IE_PATCH_IMAGE = 4,
	ATH6KL_FW_IE_RESERVED_RAM_SIZE = 5,
	ATH6KL_FW_IE_CAPABILITIES = 6,
	ATH6KL_FW_IE_PATCH_ADDR = 7,
	ATH6KL_FW_IE_BOARD_ADDR = 8,
	ATH6KL_FW_IE_VIF_MAX = 9,
};

enum ath6kl_fw_capability {
	ATH6KL_FW_CAPABILITY_HOST_P2P = 0,

	/* this needs to be last */
	ATH6KL_FW_CAPABILITY_MAX,
};

#define ATH6KL_CAPABILITY_LEN (ALIGN(ATH6KL_FW_CAPABILITY_MAX, 32) / 32)

struct ath6kl_fw_ie {
	__le32 id;
	__le32 len;
	u8 data[0];
};

/* Android privacy command */
#define ATH6KL_IOCTL_STANDARD01		(SIOCDEVPRIVATE+1)

/* Standard do_ioctl() ioctl interface */
#define ATH6KL_IOCTL_STANDARD02		(SIOCDEVPRIVATE+2)

/* BTC command */
#define ATH6KL_IOCTL_STANDARD03		(SIOCDEVPRIVATE+3)

/* hole, please reserved */
#define ATH6KL_IOCTL_STANDARD12		(SIOCDEVPRIVATE+12)

/* TX99 */
#define ATH6KL_IOCTL_STANDARD13		(SIOCDEVPRIVATE+13)

/* hole, please reserved */
#define ATH6KL_IOCTL_STANDARD15		(SIOCDEVPRIVATE+15)

/* used by btfilter */
#define ATH6KL_IOCTL_WEXT_PRIV6		(SIOCIWFIRSTPRIV+6)

/* ATH6KL_IOCTL_EXTENDED - extended ioctl */
#define ATH6KL_IOCTL_WEXT_PRIV26	(SIOCIWFIRSTPRIV+26)

/* reserved for QCSAP (old) */
#define ATH6KL_IOCTL_WEXT_PRIV27	(SIOCIWFIRSTPRIV+27)

/* reserved for QCSAP */
#define ATH6KL_IOCTL_WEXT_PRIV31	(SIOCIWFIRSTPRIV+31)

#define ATH6KL_IOCTL_AP_APSD		(0)
#define ATH6KL_IOCTL_AP_INTRABSS	(1)

/* TBD: ioctl number is aligned to olca branch
 *      will refine one the loopback tool is ready for native ath6kl
 */
enum ath6kl_xioctl {
	ATH6KL_XIOCTL_TRAFFIC_ACTIVITY_CHANGE = 80,
};

struct ath6kl_traffic_activity_change {
	u32    stream_id;	/* stream ID to indicate activity change */
	u32    active;		/* active (1) or inactive (0) */
};

struct ath6kl_ioctl_cmd {
	u32 subcmd;
	u32 options;
};

/* Android-specific IOCTL */
#define ANDROID_SETBAND_ALL		0
#define ANDROID_SETBAND_5G		1
#define ANDROID_SETBAND_2G		2
#define ANDROID_SETBAND_NO_DFS		3
#define ANDROID_SETBAND_NO_CH		4

struct ath6kl_android_wifi_priv_cmd {
	char *buf;
	int used_len;
	int total_len;
};

struct btcoex_ioctl {
	char *cmd;
	unsigned int cmd_len;
};

enum ath6kl_recovery_mode {
	ATH6KL_RECOVERY_MODE_NONE = 0,
	ATH6KL_RECOVERY_MODE_WARM,
	ATH6KL_RECOVERY_MODE_COLD,
};

#ifdef CONFIG_ATH6KL_RECOVERY_MODE_USER
#define ATH6KL_RECOVERY_MODE_DEFAULT CONFIG_ATH6KL_RECOVERY_MODE_USER
#else
#define ATH6KL_RECOVERY_MODE_DEFAULT ATH6KL_RECOVERY_MODE_NONE
#endif

#define ATH6KL_FW_API2_FILE "fw-2.bin"

/* AR6003 1.0 definitions */
#define AR6003_HW_1_0_VERSION                 0x300002ba

/* AR6003 2.0 definitions */
#define AR6003_HW_2_0_VERSION                 0x30000384
#define AR6003_HW_2_0_PATCH_DOWNLOAD_ADDRESS  0x57e910
#define AR6003_HW_2_0_FW_DIR			"ath6k/AR6003/hw2.0"
#define AR6003_HW_2_0_OTP_FILE			"otp.bin.z77"
#define AR6003_HW_2_0_FIRMWARE_FILE		"athwlan.bin.z77"
#define AR6003_HW_2_0_TCMD_FIRMWARE_FILE	"athtcmd_ram.bin"
#define AR6003_HW_2_0_PATCH_FILE		"data.patch.bin"
#define AR6003_HW_2_0_BOARD_DATA_FILE "ath6k/AR6003/hw2.0/bdata.bin"
#define AR6003_HW_2_0_DEFAULT_BOARD_DATA_FILE \
			"ath6k/AR6003/hw2.0/bdata.SD31.bin"

/* AR6003 3.0 definitions */
#define AR6003_HW_2_1_1_VERSION                 0x30000582
#define AR6003_HW_2_1_1_FW_DIR			"ath6k/AR6003/hw2.1.1"
#define AR6003_HW_2_1_1_OTP_FILE		"otp.bin"
#define AR6003_HW_2_1_1_FIRMWARE_FILE		"athwlan.bin"
#define AR6003_HW_2_1_1_TCMD_FIRMWARE_FILE	"athtcmd_ram.bin"
#define AR6003_HW_2_1_1_UTF_FIRMWARE_FILE	"utf.bin"
#define AR6003_HW_2_1_1_TESTSCRIPT_FILE	"nullTestFlow.bin"
#define AR6003_HW_2_1_1_PATCH_FILE		"data.patch.bin"
#define AR6003_HW_2_1_1_BOARD_DATA_FILE "ath6k/AR6003/hw2.1.1/bdata.bin"
#define AR6003_HW_2_1_1_DEFAULT_BOARD_DATA_FILE	\
			"ath6k/AR6003/hw2.1.1/bdata.SD31.bin"

/* AR6004 1.0 definitions */
#define AR6004_HW_1_0_VERSION                 0x30000623
#define AR6004_HW_1_0_FW_DIR			"ath6k/AR6004/hw1.0"
#define AR6004_HW_1_0_OTP_FILE			"otp.bin"
#define AR6004_HW_1_0_FIRMWARE_FILE		"fw.ram.bin"
#define AR6004_HW_1_0_BOARD_DATA_FILE         "ath6k/AR6004/hw1.0/bdata.bin"
#define AR6004_HW_1_0_DEFAULT_BOARD_DATA_FILE \
	"ath6k/AR6004/hw1.0/bdata.DB132.bin"

/* AR6004 1.1 definitions */
#define AR6004_HW_1_1_VERSION                 0x30000001
#define AR6004_HW_1_1_FW_DIR			"ath6k/AR6004/hw1.1"
#define AR6004_HW_1_1_OTP_FILE			"otp.bin"
#define AR6004_HW_1_1_FIRMWARE_FILE		"fw.ram.bin"
#define AR6004_HW_1_1_TCMD_FIRMWARE_FILE           "utf.bin"
#define AR6004_HW_1_1_UTF_FIRMWARE_FILE	"utf.bin"
#define AR6004_HW_1_1_TESTSCRIPT_FILE	"nullTestFlow.bin"
#define AR6004_HW_1_1_BOARD_DATA_FILE         "ath6k/AR6004/hw1.1/bdata.bin"
#define AR6004_HW_1_1_DEFAULT_BOARD_DATA_FILE \
	"ath6k/AR6004/hw1.1/bdata.DB132.bin"
#define AR6004_HW_1_1_EPPING_FILE             "ath6k/AR6004/hw1.1/epping.bin"
#define AR6004_HW_1_1_SOFTMAC_FILE            "ath6k/AR6004/hw1.1/softmac.bin"
#define AR6004_HW_1_1_SOFTMAC_2_FILE            \
	"ath6k/AR6004/hw1.1/softmac_2.bin"


/* AR6004 1.2 definitions */
#define AR6004_HW_1_2_VERSION                 0x300007e8
#define AR6004_HW_1_2_FW_DIR			"ath6k/AR6004/hw1.2"
#define AR6004_HW_1_2_OTP_FILE			"otp.bin"
#define AR6004_HW_1_2_FIRMWARE_2_FILE         "fw-2.bin"
#define AR6004_HW_1_2_FIRMWARE_FILE           "fw.ram.bin"
#define AR6004_HW_1_2_TCMD_FIRMWARE_FILE      "utf.bin"
#define AR6004_HW_1_2_UTF_FIRMWARE_FILE	"utf.bin"
#define AR6004_HW_1_2_TESTSCRIPT_FILE	"nullTestFlow.bin"
#define AR6004_HW_1_2_BOARD_DATA_FILE         "ath6k/AR6004/hw1.2/bdata.bin"
#define AR6004_HW_1_2_DEFAULT_BOARD_DATA_FILE \
	"ath6k/AR6004/hw1.2/bdata.bin"
#define AR6004_HW_1_2_EPPING_FILE             "ath6k/AR6004/hw1.2/epping.bin"
#define AR6004_HW_1_2_SOFTMAC_FILE            "ath6k/AR6004/hw1.2/softmac.bin"
#define AR6004_HW_1_2_SOFTMAC_2_FILE            \
	"ath6k/AR6004/hw1.2/softmac_2.bin"

/* AR6004 1.3 definitions */
#define AR6004_HW_1_3_VERSION                 0x31c8088a
#define AR6004_HW_1_3_FW_DIR			"ath6k/AR6004/hw1.3"
#define AR6004_HW_1_3_OTP_FILE			"otp.bin"
#define AR6004_HW_1_3_FIRMWARE_2_FILE         "fw-2.bin"
#define AR6004_HW_1_3_FIRMWARE_FILE           "fw.ram.bin"
#define AR6004_HW_1_3_FIRMWARE_EXT_FILE       "fw_ext.ram.bin"
#define AR6004_HW_1_3_TCMD_FIRMWARE_FILE      "utf.bin"
#define AR6004_HW_1_3_UTF_FIRMWARE_FILE	"utf.bin"
#define AR6004_HW_1_3_TESTSCRIPT_FILE	"nullTestFlow.bin"
#define AR6004_HW_1_3_BOARD_DATA_FILE         "ath6k/AR6004/hw1.3/bdata.bin"
#define AR6004_HW_1_3_DEFAULT_BOARD_DATA_FILE \
	"ath6k/AR6004/hw1.3/bdata.bin"
#define AR6004_HW_1_3_EPPING_FILE             "ath6k/AR6004/hw1.3/epping.bin"
#define AR6004_HW_1_3_SOFTMAC_FILE            "ath6k/AR6004/hw1.3/softmac.bin"
#define AR6004_HW_1_3_SOFTMAC_2_FILE            \
	"ath6k/AR6004/hw1.3/softmac_2.bin"

/* AR6004 2.0 definitions */
#define AR6004_HW_2_0_VERSION                 0x31c80958
#define AR6004_HW_2_0_FW_DIR			"ath6k/AR6004/hw2.0"
#define AR6004_HW_2_0_OTP_FILE			"otp.bin"
#define AR6004_HW_2_0_FIRMWARE_2_FILE         "fw-2.bin"
#define AR6004_HW_2_0_FIRMWARE_FILE           "fw.ram.bin"
#define AR6004_HW_2_0_TCMD_FIRMWARE_FILE      "utf.bin"
#define AR6004_HW_2_0_UTF_FIRMWARE_FILE	"utf.bin"
#define AR6004_HW_2_0_TESTSCRIPT_FILE	"nullTestFlow.bin"
#define AR6004_HW_2_0_BOARD_DATA_FILE         "ath6k/AR6004/hw2.0/bdata.bin"
#define AR6004_HW_2_0_DEFAULT_BOARD_DATA_FILE \
	"ath6k/AR6004/hw2.0/bdata.bin"
#define AR6004_HW_2_0_EPPING_FILE             "ath6k/AR6004/hw2.0/epping.bin"
#define AR6004_HW_2_0_SOFTMAC_FILE            "ath6k/AR6004/hw2.0/softmac.bin"
#define AR6004_HW_2_0_SOFTMAC_2_FILE            \
	"ath6k/AR6004/hw2.0/softmac_2.bin"

/* AR6004 3.0 definitions */
#define AR6004_HW_3_0_VERSION			0x31C809F8
#define AR6004_HW_3_0_FW_DIR			"ath6k/AR6004/hw3.0"
#define AR6004_HW_3_0_OTP_FILE			"otp.bin"
#define AR6004_HW_3_0_FIRMWARE_2_FILE         "fw-2.bin"
#define AR6004_HW_3_0_FIRMWARE_FILE           "fw.ram.bin"
#define AR6004_HW_3_0_TCMD_FIRMWARE_FILE      "utf.bin"
#define AR6004_HW_3_0_UTF_FIRMWARE_FILE	"utf.bin"
#define AR6004_HW_3_0_TESTSCRIPT_FILE	"nullTestFlow.bin"
#define AR6004_HW_3_0_BOARD_DATA_FILE         "ath6k/AR6004/hw3.0/bdata.bin"
#define AR6004_HW_3_0_DEFAULT_BOARD_DATA_FILE \
	"ath6k/AR6004/hw3.0/bdata.bin"
#define AR6004_HW_3_0_EPPING_FILE             "ath6k/AR6004/hw3.0/epping.bin"
#define AR6004_HW_3_0_SOFTMAC_FILE            "ath6k/AR6004/hw3.0/softmac.bin"
#define AR6004_HW_3_0_SOFTMAC_2_FILE            \
	"ath6k/AR6004/hw3.0/softmac_2.bin"

/* AR6006 1.0 definitions */
#define AR6006_HW_1_0_VERSION                 0x31c80997
#define AR6006_HW_1_0_FW_DIR			"ath6k/AR6006/hw1.0"
#define AR6006_HW_1_0_FIRMWARE_2_FILE         "fw-2.bin"
#define AR6006_HW_1_0_FIRMWARE_FILE           "fw.ram.bin"
#define AR6006_HW_1_0_BOARD_DATA_FILE         "ath6k/AR6006/hw1.0/bdata.bin"
#define AR6006_HW_1_0_EPPING_FILE             "ath6k/AR6006/hw1.0/epping.bin"
#define AR6006_HW_1_0_DEFAULT_BOARD_DATA_FILE \
	"ath6k/AR6006/hw1.0/bdata.bin"
#define AR6006_HW_1_0_SOFTMAC_FILE            "ath6k/AR6006/hw1.0/softmac.bin"
#define AR6006_HW_1_0_SOFTMAC_2_FILE            \
	"ath6k/AR6006/hw1.0/softmac_2.bin"

/* AR6006 1.1 definitions */
#define AR6006_HW_1_1_VERSION                 0x31c80002
#define AR6006_HW_1_1_FW_DIR			"ath6k/AR6006/hw1.1"
#define AR6006_HW_1_1_FIRMWARE_2_FILE         "fw-2.bin"
#define AR6006_HW_1_1_FIRMWARE_FILE           "fw.ram.bin"
#define AR6006_HW_1_1_BOARD_DATA_FILE         "ath6k/AR6006/hw1.1/bdata.bin"
#define AR6006_HW_1_1_EPPING_FILE             "ath6k/AR6006/hw1.1/epping.bin"
#define AR6006_HW_1_1_DEFAULT_BOARD_DATA_FILE \
	"ath6k/AR6006/hw1.1/bdata.bin"
#define AR6006_HW_1_1_SOFTMAC_FILE            "ath6k/AR6006/hw1.1/softmac.bin"
#define AR6006_HW_1_1_SOFTMAC_2_FILE            \
	"ath6k/AR6006/hw1.1/softmac_2.bin"

#define AR6004_MAX_64K_FW_SIZE                65536

#define BDATA_CHECKSUM_OFFSET                 4
#define BDATA_MAC_ADDR_OFFSET                 8
#define BDATA_OPFLAGS_OFFSET                 24
#define BDATA_TXRXMASK_OFFSET                40

/* Per STA data, used in AP mode */
#define STA_PS_AWAKE		BIT(0)
#define	STA_PS_SLEEP		BIT(1)
#define	STA_PS_POLLED		BIT(2)
#define STA_PS_APSD_TRIGGER	BIT(3)
#define STA_PS_APSD_EOSP	BIT(4)
#define STA_HT_SUPPORT		BIT(5)

/* HTC TX packet tagging definitions */
#define ATH6KL_CONTROL_PKT_TAG    HTC_TX_PACKET_TAG_USER_DEFINED
#define ATH6KL_DATA_PKT_TAG       (ATH6KL_CONTROL_PKT_TAG + 1)
#define ATH6KL_PRI_DATA_PKT_TAG       (ATH6KL_CONTROL_PKT_TAG + 2)

#define AR6003_CUST_DATA_SIZE 16

#define AGGR_WIN_IDX(x, y)          ((x) % (y))
#define AGGR_INCR_IDX(x, y)         AGGR_WIN_IDX(((x) + 1), (y))
#define AGGR_DCRM_IDX(x, y)         AGGR_WIN_IDX(((x) - 1), (y))
#define ATH6KL_MAX_SEQ_NO		0xFFF
#define ATH6KL_NEXT_SEQ_NO(x)		(((x) + 1) & ATH6KL_MAX_SEQ_NO)

#define NUM_OF_TIDS         8
#define AGGR_SZ_DEFAULT     8

#define AGGR_WIN_SZ_MIN     2
#define AGGR_WIN_SZ_MAX     8

#define TID_WINDOW_SZ(_x)   ((_x) << 1)

#define AGGR_NUM_OF_FREE_NETBUFS    16

#define AGGR_RX_TIMEOUT          100	/* in ms */
#define AGGR_RX_TIMEOUT_VO       50 /* in ms */
#define MCC_AGGR_RX_TIMEOUT		300 /* in ms */
#define MCC_AGGR_RX_TIMEOUT_VO	150 /* in ms */

#define AGGR_GET_RXTID_STATS(_p, _x)     (&(_p->stat[(_x)]))
#define AGGR_GET_RXTID(_p, _x)           (&(_p->rx_tid[(_x)]))

#define AGGR_BA_EVT_GET_CONNID(_conn)    ((_conn) >> 4)
#define AGGR_BA_EVT_GET_TID(_tid)        ((_tid) & 0xF)

#define AGGR_TX_MAX_AGGR_SIZE   1600 /* Sync to max. PDU size of host. */
#define AGGR_TX_MAX_PDU_SIZE    120
#define AGGR_TX_MIN_PDU_SIZE    64   /* 802.3(14) + LLC(8) + IP/TCP(20) = 42 */
#define AGGR_TX_MAX_NUM		6
#define AGGR_TX_TIMEOUT         4	/* in ms */

#define AGGR_TX_PROG_HS_THRESH		1000
#define AGGR_TX_PROG_HS_FACTOR		2
#define AGGR_TX_PROG_NS_THRESH		250
#define AGGR_TX_PROG_NS_FACTOR		4
#define AGGR_TX_PROG_CHECK_INTVAL	(1 * HZ)
#define AGGR_TX_PROG_HS_MAX_NUM		22
#define AGGR_TX_PROG_HS_TIMEOUT		8	/* in ms */

#define AGGR_GET_TXTID(_p, _x)           (&(_p->tx_tid[(_x)]))

#define WMI_TIMEOUT (2 * HZ)

#define MBOX_YIELD_LIMIT 99

/* AP-PS */
#define ATH6KL_PS_QUEUE_CHECK_AGE	(1 * 1000)	/* 1 sec. */
#define ATH6KL_PS_QUEUE_MAX_AGE		(5)		/* 5 cycles */
#define ATH6KL_PS_QUEUE_NO_AGE		(0)		/* no aging */

#define ATH6KL_PS_QUEUE_MAX_DEPTH	(65535)
#define ATH6KL_PS_QUEUE_NO_DEPTH	(0)			/* unlimit */

enum ps_queue_type {
	PS_QUEUE_TYPE_NONE,
	PS_QUEUE_TYPE_STA_UNICAST,
	PS_QUEUE_TYPE_STA_MGMT,
	PS_QUEUE_TYPE_AP_MULTICAST,
};

/* Scanband */
enum scanband_type {
	SCANBAND_TYPE_ALL,		/* Scan all supported channel */
	SCANBAND_TYPE_2G,		/* Scan 2GHz channel only */
	SCANBAND_TYPE_5G,		/* Scan 5GHz channel only */
	SCANBAND_TYPE_CHAN_ONLY,	/* Scan single channel only */
	SCANBAND_TYPE_P2PCHAN,		/* Scan P2P channel only */
	SCANBAND_TYPE_2_P2PCHAN,	/* Scan P2P channel 2 times */
	SCANBAND_TYPE_IGNORE_DFS,	/* Scan all supported channel but DFS */
	SCANBAND_TYPE_IGNORE_CH,	/* Scan all supported channel but CHs */
};

#define ATH6KL_RSN_CAP_NULLCONF		(0xffff)

/* configuration lags */
/*
 * ATH6KL_CONF_IGNORE_ERP_BARKER: Ignore the barker premable in
 * ERP IE of beacon to determine the short premable support when
 * sending (Re)Assoc req.
 * ATH6KL_CONF_IGNORE_PS_FAIL_EVT_IN_SCAN: Don't send the power
 * module state transition failure events which happen during
 * scan, to the host.
 */
#define ATH6KL_CONF_IGNORE_ERP_BARKER		BIT(0)
#define ATH6KL_CONF_IGNORE_PS_FAIL_EVT_IN_SCAN  BIT(1)
#define ATH6KL_CONF_ENABLE_11N			BIT(2)
#define ATH6KL_CONF_ENABLE_TX_BURST		BIT(3)
#define ATH6KL_CONF_SUSPEND_CUTPOWER		BIT(4)
#define ATH6KL_CONF_ENABLE_FLOWCTRL		BIT(5)
#define ATH6KL_CONF_DISABLE_SKIP_FLOWCTRL	BIT(6)
#define ATH6KL_CONF_DISABLE_RX_AGGR_DROP	BIT(7)
#define ATH6KL_CONF_SKB_DUP					BIT(8)

enum wlan_low_pwr_state {
	WLAN_POWER_STATE_ON,
	WLAN_POWER_STATE_CUT_PWR,
	WLAN_POWER_STATE_DEEP_SLEEP,
	WLAN_POWER_STATE_WOW
};

enum sme_state {
	SME_DISCONNECTED,
	SME_CONNECTING,
	SME_CONNECTED
};

struct skb_hold_q {
	struct sk_buff *skb;
	bool is_amsdu;
	u16 seq_no;
};

/*
 * ATH6KL_MAX_WAIT_CONTINUS_PKT: the maximum number of rx
 * continuous packets to be waited, the first packet will
 * wait tid_timeout_setting time, the second will wait
 * tid_timeout_setting / 2 etc.
*/
#define ATH6KL_MAX_WAIT_CONTINUOUS_PKT		3

struct rxtid {
	bool aggr;
	u16 win_sz;
	u16 seq_next;
	u32 hold_q_sz;
	struct skb_hold_q *hold_q;
	struct sk_buff_head q;
	spinlock_t lock;
	u16 timerwait_seq_num;		/* current wait seq_no next */
	bool sync_next_seq;
	struct timer_list tid_timer;
	u8 tid_timer_scheduled;
	u8	tid;
	u16	issue_timer_seq;
	struct aggr_conn_info *aggr_conn;
	u8 continuous_count;
};

struct rxtid_stats {
	u32 num_into_aggr;
	u32 num_dups;
	u32 num_oow;
	u32 num_mpdu;
	u32 num_amsdu;
	u32 num_delivered;
	u32 num_timeouts;
	u32 num_hole;
	u32 num_bar;
};

enum {
	AGGR_TX_OK = 0,
	AGGR_TX_DONE,
	AGGR_TX_BYPASS,
	AGGR_TX_DROP,
	AGGR_TX_UNKNOW,
};

struct txtid {
	u8 tid;
	u16 aid;
	u16 max_aggr_sz;		/* 0 means disable */
	struct timer_list timer;
	struct sk_buff *amsdu_skb;
	u8 amsdu_cnt;			/* current aggr count */
	u8 *amsdu_start;		/* start pointer of amsdu frame */
	u16 amsdu_len;			/* current aggr length */
	u16 amsdu_lastpdu_len;		/* last PDU length */
	spinlock_t lock;
	struct ath6kl_vif *vif;

	/* TX progressive */
	unsigned long last_check_time;
	u32 last_num_amsdu;
	u32 last_num_timeout;

	/* Statistics */
	u32 num_pdu;
	u32 num_amsdu;
	u32 num_timeout;
	u32 num_flush;
	u32 num_tx_null;
	u32 num_overflow;
};

struct aggr_info {
	struct ath6kl_vif *vif;
	struct sk_buff_head free_q;

	/* RX */
	u16 rx_aggr_timeout;	/* in ms */

	/* TX A-MSDU */
	bool tx_amsdu_enable;	/* IOT : treat A-MPDU & A-MSDU are exclusive. */
	bool tx_amsdu_seq_pkt;
	bool tx_amsdu_progressive;
	bool tx_amsdu_progressive_hispeed;	/* in high speed or not */

	u8 tx_amsdu_max_aggr_num;
	u32 tx_amsdu_max_aggr_len;
	u16 tx_amsdu_max_pdu_len;
	u16 tx_amsdu_timeout;	/* in ms */
};

struct aggr_conn_info {
	u8 aggr_sz;
	struct aggr_info *aggr_cntxt;
	struct net_device *dev;
	struct rxtid rx_tid[NUM_OF_TIDS];
	struct rxtid_stats stat[NUM_OF_TIDS];
	u32 tid_timeout_setting[NUM_OF_TIDS];
	/* TX A-MSDU */
	struct txtid tx_tid[NUM_OF_TIDS];
};

struct ath6kl_wep_key {
	u8 key_index;
	u8 key_len;
	u8 key[64];
};

#define ATH6KL_KEY_SEQ_LEN 8

struct ath6kl_key {
	u8 key[WLAN_MAX_KEY_LEN];
	u8 key_len;
	u8 seq[ATH6KL_KEY_SEQ_LEN];
	u8 seq_len;
	u32 cipher;
};

struct ath6kl_node_mapping {
	u8 mac_addr[ETH_ALEN];
	u8 ep_id;
	u8 tx_pend;
};

enum cookie_type {
	COOKIE_TYPE_NONE,
	COOKIE_TYPE_DATA,
	COOKIE_TYPE_CTRL,
};

struct ath6kl_cookie {
	struct ath6kl_cookie_pool *cookie_pool;
	struct sk_buff *skb;
	u32 map_no;
	struct htc_packet *htc_pkt;
	struct ath6kl_cookie *arc_list_next;
};

struct ath6kl_cookie_pool {
	enum cookie_type cookie_type;
	u32 cookie_num;		/* total number */

	struct ath6kl_cookie *cookie_list;
	u32 cookie_count;	/* current available number */
	struct ath6kl_cookie *cookie_mem;

	/* stats */
	u32 cookie_alloc_cnt;
	u32 cookie_alloc_fail_cnt;
	u32 cookie_free_cnt;
	u32 cookie_peak_cnt;
};

struct ath6kl_ps_buf_desc {
	struct list_head list;

	u32 age;
	size_t len;

	/* For DATA */
	struct sk_buff *skb;

	/* For MGMT */
	u32 freq;
	u32 wait;
	u32 id;
	bool no_cck;
	bool dont_wait_for_ack;
	u8 buf[1];
};

struct ath6kl_ps_buf_head {
	struct list_head list;
	spinlock_t lock;

	enum ps_queue_type queue_type;
	int depth;

	u32 age_cycle;
	u32 max_depth;

	/* stat */
	u32 enqueued;
	u32 enqueued_err;
	u32 dequeued;
	u32 aged;
};

#ifdef ATHTST_SUPPORT
#define ATH_RSSI_LPF_LEN                     10
#define ATH_RSSI_DUMMY_MARKER                0x127
#define RSSI_LPF_THRESHOLD                   -20
#define ATH_RSSI_EP_MULTIPLIER               (1<<7)  /* pow2 to optimize out */
#define HAL_RSSI_EP_MULTIPLIER  (1<<7)  /* pow2 to optimize out */
#define ATH_EP_RND(x, mul)                   \
	((((x)%(mul)) >= ((mul)/2)) ? ((x) + ((mul) - 1)) / (mul) : (x)/(mul))

#define ATH_EP_MUL(x, mul)                   ((x) * (mul))

#define ATH_RSSI_IN(x)                       \
	(ATH_EP_MUL((x), ATH_RSSI_EP_MULTIPLIER))

#define ATH_LPF_RSSI(x, y, len) \
((x != ATH_RSSI_DUMMY_MARKER) ? (((x) * ((len) - 1) + (y)) / (len)) : (y))

#define ATH_RSSI_LPF(x, y) do {                     \
	if ((y) >= RSSI_LPF_THRESHOLD)                         \
		x = ATH_LPF_RSSI((x), ATH_RSSI_IN((y)), ATH_RSSI_LPF_LEN);  \
} while (0)
#endif

enum ath6kl_phy_mode {
	ATH6KL_PHY_MODE_11A = 0,	/* 11a Mode */
	ATH6KL_PHY_MODE_11G = 1,		/* 11b/g Mode */
	ATH6KL_PHY_MODE_11B = 2,		/* 11b Mode */
	ATH6KL_PHY_MODE_11GONLY = 3,	/* 11g only Mode */
	ATH6KL_PHY_MODE_11NA_HT20 = 4,	/* 11na HT20 Mode */
	ATH6KL_PHY_MODE_11NG_HT20 = 5,	/* 11ng HT20 Mode */
	ATH6KL_PHY_MODE_11NA_HT40 = 6,	/* 11na HT40 Mode */
	ATH6KL_PHY_MODE_11NG_HT40 = 7,	/* 11ng HT40 Mode */
	ATH6KL_PHY_MODE_UNKNOWN = 8,	/* Unknown */
	ATH6KL_PHY_MODE_MAX = 8,
};

struct ath6kl_sta {
	u16 sta_flags;
	u8 mac[ETH_ALEN];
	u8 aid;
	u8 keymgmt;
	u8 ucipher;
	u8 auth;
	u8 wpa_ie[ATH6KL_MAX_IE];
	enum ath6kl_phy_mode phymode;
	struct ath6kl_vif *vif;

	/* ath6kl_sta global lock, psq_data & psq_mgmt also use it. */
	spinlock_t lock;

	/* AP-PS */
	struct ath6kl_ps_buf_head psq_data;
	struct ath6kl_ps_buf_head psq_mgmt;
	struct timer_list psq_age_timer;
	u8 psq_age_active;
	u8 apsd_info;

	/* TX/RX-AMSDU */
	struct aggr_conn_info *aggr_conn_cntxt;

	/* AP-Keepalive */
	u16 last_txrx_time_tgt;		/* target time. */
	unsigned long last_txrx_time;	/* in jiffies., host time. */
#ifdef ATHTST_SUPPORT
	int avg_data_rssi;
#endif
};

struct ath6kl_version {
	u32 target_ver;
	u32 wlan_ver;
	u32 abi_ver;
};

struct ath6kl_bmi {
	u32 cmd_credits;
	bool done_sent;
	u8 *cmd_buf;
	u32 max_data_size;
	u32 max_cmd_size;
};

struct target_stats {
	u64 tx_pkt;
	u64 tx_byte;
	u64 tx_ucast_pkt;
	u64 tx_ucast_byte;
	u64 tx_mcast_pkt;
	u64 tx_mcast_byte;
	u64 tx_bcast_pkt;
	u64 tx_bcast_byte;
	u64 tx_rts_success_cnt;
	u64 tx_pkt_per_ac[4];

	u64 tx_err;
	u64 tx_fail_cnt;
	u64 tx_retry_cnt;
	u64 tx_mult_retry_cnt;
	u64 tx_rts_fail_cnt;

	u64 rx_pkt;
	u64 rx_byte;
	u64 rx_ucast_pkt;
	u64 rx_ucast_byte;
	u64 rx_mcast_pkt;
	u64 rx_mcast_byte;
	u64 rx_bcast_pkt;
	u64 rx_bcast_byte;
	u64 rx_frgment_pkt;

	u64 rx_err;
	u64 rx_crc_err;
	u64 rx_key_cache_miss;
	u64 rx_decrypt_err;
	u64 rx_dupl_frame;

	u64 tkip_local_mic_fail;
	u64 tkip_cnter_measures_invoked;
	u64 tkip_replays;
	u64 tkip_fmt_err;
	u64 ccmp_fmt_err;
	u64 ccmp_replays;

	u64 pwr_save_fail_cnt;

	u64 cs_bmiss_cnt;
	u64 cs_low_rssi_cnt;
	u64 cs_connect_cnt;
	u64 cs_discon_cnt;

	s32 tx_ucast_rate;
	s8 tx_rate_index;
	s32 rx_ucast_rate;

	u32 lq_val;

	u32 wow_pkt_dropped;
	u16 wow_evt_discarded;

	s16 noise_floor_calib;
	s16 cs_rssi;
	s16 cs_ave_beacon_rssi;
	u8 cs_ave_beacon_snr;
	u8 cs_last_roam_msec;
	u8 cs_snr;

	u8 wow_host_pkt_wakeups;
	u8 wow_host_evt_wakeups;

	u32 arp_received;
	u32 arp_matched;
	u32 arp_replied;

	struct timeval update_time;
};

struct ath6kl_mbox_info {
	u32 htc_addr;
	u32 htc_ext_addr;
	u32 htc_ext_sz;

	u32 block_size;

	u32 gmbox_addr;

	u32 gmbox_sz;
};

enum ath6kl_hw_flags {
	ATH6KL_HW_TGT_ALIGN_PADDING		= BIT(0),
	ATH6KL_HW_SINGLE_PIPE_SCHED			= BIT(1),
	ATH6KL_HW_FIRMWARE_EXT_SUPPORT	= BIT(2),
	ATH6KL_HW_USB_FLOWCTRL				= BIT(3),
	ATH6KL_HW_XTAL_40MHZ				= BIT(4),
};

/*
 * 802.11i defines an extended IV for use with non-WEP ciphers.
 * When the EXTIV bit is set in the key id byte an additional
 * 4 bytes immediately follow the IV for TKIP.  For CCMP the
 * EXTIV bit is likewise set but the 8 bytes represent the
 * CCMP header rather than IV+extended-IV.
 */

#define ATH6KL_KEYBUF_SIZE 16
#define ATH6KL_MICBUF_SIZE (8+8)	/* space for both tx and rx */

#define ATH6KL_KEY_XMIT  0x01
#define ATH6KL_KEY_RECV  0x02
#define ATH6KL_KEY_DEFAULT   0x80	/* default xmit key */

/* Initial group key for AP mode */
struct ath6kl_req_key {
	bool valid;
	u8 key_index;
	int key_type;
	u8 key[WLAN_MAX_KEY_LEN];
	u8 key_len;
};

/*
 * Bluetooth WiFi co-ex information.
 * This structure keeps track of the Bluetooth related status.
 * This involves the Bluetooth ACL link role, Bluetooth remote lmp version.
 */
struct ath6kl_btcoex {
	u32 acl_role; /* Master/slave role of Bluetooth ACL link */
	u32 remote_lmp_ver; /* LMP version of the remote device. */
	u32 bt_vendor; /* Keeps track of the Bluetooth chip vendor */
};
enum ath6kl_hif_type {
	ATH6KL_HIF_TYPE_SDIO,
	ATH6KL_HIF_TYPE_USB,
};

enum ath6kl_chan_type {
	ATH6KL_CHAN_TYPE_NONE,		/* by target or 11abg */
	ATH6KL_CHAN_TYPE_HT40PLUS,
	ATH6KL_CHAN_TYPE_HT40MINUS,
	ATH6KL_CHAN_TYPE_HT20,
};

enum scan_plan_type {
	ATH6KL_SCAN_PLAN_IN_ORDER,
	ATH6KL_SCAN_PLAN_REVERSE_ORDER,
	ATH6KL_SCAN_PLAN_HOST_ORDER,
};

struct ath6kl_scan_plan {
	enum scan_plan_type type;
	u8 numChan;
	u16 chanList[64];	/* WMI_MAX_CHANNELS */
};

/*
 * Driver's maximum limit, note that some firmwares support only one vif
 * and the runtime (current) limit must be checked from ar->vif_max.
 */
#define ATH6KL_VIF_MAX	8

/* vif flags info */
enum ath6kl_vif_state {
	CONNECTED,
	CONNECT_PEND,
	CONNECT_HANDSHAKE_PROTECT,
	FIRST_EAPOL_PENDSENT,
	WMM_ENABLED,
	NETQ_STOPPED,
	DTIM_EXPIRED,
	NETDEV_REGISTERED,
	CLEAR_BSSFILTER_ON_BEACON,
	DTIM_PERIOD_AVAIL,
	WLAN_ENABLED,
	STATS_UPDATE_PEND,
	AMSDU_ENABLED,
	HOST_SLEEP_MODE_CMD_PROCESSED,
	ROC_PEND,
	ROC_ONGOING,
	ROC_CANCEL_PEND,
	ROC_WAIT_EVENT,
	DISCONNECT_PEND,
	PMKLIST_GET_PEND,
	PORT_STATUS_PEND,
	WLAN_WOW_ENABLE,
	SCANNING,
	SCANNING_WAIT,
	DORMANT,
	PS_STICK,
#ifdef ATHTST_SUPPORT
	CE_WMI_UPDATE,
	CE_WMI_SCAN,
	CE_WMI_TESTMODE_RX,
	CE_WMI_TESTMODE_GET,
#endif
};
#ifdef ATHTST_SUPPORT
/* IOCTL structure to configure the wireless interface. */
struct athr_cmd {
	int     cmd;        /* CMD Type */
	int     data[4];    /* CMD Data */
	int     list[16];   /* CMD LIST Data */
};

#define ATHR_WLAN_SCAN_BAND             1
#define ATHR_WLAN_FIND_BEST_CHANNEL     2
#define ATHR_WLAN_ACS_LIST              3

#define ATHR_CMD_SCANBAND_ALL           0   /* Scan all supported channel */
#define ATHR_CMD_SCANBAND_2G            1   /* Scan 2GHz channel only */
#define ATHR_CMD_SCANBAND_5G            2   /* Scan 5GHz channel only */
#define ATHR_CMD_SCANBAND_CHAN_ONLY     3   /* Scan single channel only */

#define ATHR_CMD_ACS_2G_LIST            1   /* Scan 2GHz channel only */
#define ATHR_CMD_ACS_5G_LIST            2   /* Scan 5GHz channel only */
#endif

#ifdef ACL_SUPPORT
#define AP_ACL_SIZE 10
#define ATH_MAC_LEN 6

struct WMI_AP_ACL {
	u16	index;
	u8	acl_mac[AP_ACL_SIZE][ATH_MAC_LEN];
	u8	wildcard[AP_ACL_SIZE];
	u8	policy;
};
#endif

struct bss_info_entry {
	struct list_head list;

	s32 signal;
	struct ieee80211_channel *channel;
	struct ieee80211_mgmt *mgmt;
	size_t len;
};

#define ATH6KL_BSS_POST_PROC_SCAN_ONGOING	(1 << 0)

struct bss_post_proc {
	struct ath6kl_vif *vif;
	u32 flags;
	spinlock_t bss_info_lock;
	struct list_head bss_info_list;
};

struct ath6kl_vif {
	struct list_head list;
	struct wireless_dev wdev;
	struct net_device *ndev;
	struct ath6kl *ar;
	/* Lock to protect vif specific net_stats and flags */
	spinlock_t if_lock;
	u8 fw_vif_idx;
	unsigned long flags;
	int ssid_len;
	u8 ssid[IEEE80211_MAX_SSID_LEN];
	u8 dot11_auth_mode;
	u8 auth_mode;
	u8 prwise_crypto;
	u8 prwise_crypto_len;
	u8 grp_crypto;
	u8 grp_crypto_len;
	u8 def_txkey_index;
	u8 next_mode;
	u8 nw_type;
	u8 bssid[ETH_ALEN];
	u8 req_bssid[ETH_ALEN];
	u16 ch_hint;
	u16 bss_ch;
	enum ath6kl_phy_mode phymode;	/* Working PhyMode for AP&STA modes */
	enum ath6kl_chan_type chan_type;/* Working ChanType for AP mode */
	struct ath6kl_wep_key wep_key_list[WMI_MAX_KEY_INDEX + 1];
#ifdef PMF_SUPPORT
	struct ath6kl_key keys[WMI_MAX_IGTK_INDEX + 1];
#else
	struct ath6kl_key keys[WMI_MAX_KEY_INDEX + 1];
#endif
	struct aggr_info *aggr_cntxt;
	struct timer_list disconnect_timer;
	u32 connect_ctrl_flags;
	u8 usr_bss_filter;
	struct cfg80211_scan_request *scan_req;
	struct timer_list vifscan_timer;
	struct timer_list shprotect_timer;
	enum sme_state sme_state;
	u8 intra_bss;
	u8 ap_apsd;
	struct wmi_ap_mode_stat ap_stats;
	int last_dump_ap_stats_idx;	/* for dump_station call-back */
	u8 ap_country_code[3];
	int sta_no_ht_num;
	struct ath6kl_sta sta_list[AP_MAX_NUM_STA];
	u16 sta_list_index;	/* at least AP_MAX_NUM_STA bits */
	struct ath6kl_req_key ap_mode_bkey;
	struct ath6kl_ps_buf_head psq_mcast;
	spinlock_t psq_mcast_lock;
	int reconnect_flag;
	u32 last_roc_id;
	struct ieee80211_channel *last_roc_channel;
	unsigned int last_roc_duration;
	u32 last_cancel_roc_id;
	u32 send_action_id;
	bool probe_req_report;
#ifdef CE_SUPPORT
	bool probe_resp_report;
#endif
	u16 next_chan;				/* Setting Channel */
	enum ath6kl_chan_type next_chan_type;	/* Setting Channel-Type */
	u16 assoc_bss_beacon_int;
	u8 assoc_bss_dtim_period;
	struct net_device_stats net_stats;
	struct target_stats target_stats;
	struct htcoex *htcoex_ctx;
	struct wmi_scan_params_cmd sc_params;
	struct wmi_scan_params_cmd sc_params_default;
	u8 pmkid_list_buf[MAX_PMKID_LIST_SIZE];
	u16 last_rsn_cap;
#ifdef ATH6KL_DIAGNOSTIC
	struct wifi_diag diag;
#endif
	struct p2p_ps_info *p2p_ps_info_ctx;
	enum scanband_type scanband_type;
	u32 scanband_chan;
	u16 scanband_ignore_chan[64];		/* WMI_MAX_CHANNELS */
	struct ath6kl_scan_plan scan_plan;
	struct ap_keepalive_info *ap_keepalive_ctx;
	struct ap_acl_info *ap_acl_ctx;
	struct ap_admc_info *ap_admc_ctx;
	struct timer_list sche_scan_timer;
	int sche_scan_interval;			/* in ms. */

	u8 last_pwr_mode;
	u8 saved_pwr_mode;
	u8 arp_offload_ip_set;
	u32 arp_offload_ip;
	struct delayed_work work_eapol_send;
	struct sk_buff *pend_skb;
	spinlock_t pend_skb_lock;

	int needed_headroom;
#ifdef CE_SUPPORT
	u8 scan_band;
	u32 scan_chan;

	u8 p2p_acs_2g_list[16];
	u8 p2p_acs_5g_list[16];
#endif

#ifdef ACL_SUPPORT
	struct WMI_AP_ACL	acl_db;
#endif

#ifdef ACS_SUPPORT
	struct acs *acs_ctx;
	int best_chan[4];
#endif
	struct wmi_ant_div_stat ant_div_stat;
	struct wmi_ani_stat ani_stat;
	u8 ani_enable;
	u8 ani_pollcnt;

	struct delayed_work work_pending_connect;
	struct p2p_pending_connect_info *pending_connect_info;

	struct bss_post_proc *bss_post_proc_ctx;
	u32 data_cookie_count;

	struct ap_rc_info ap_rc_info_ctx;
};

#define WOW_LIST_ID		0
#define WOW_HOST_REQ_DELAY	5000 /* ms */

/* Flag info */
enum ath6kl_dev_state {
	WMI_ENABLED,
	WMI_READY,
	WMI_CTRL_EP_FULL,
	TESTMODE,
	TESTMODE_EPPING,
	DESTROY_IN_PROGRESS,
	SKIP_SCAN,
	ROAM_TBL_PEND,
	FIRST_BOOT,
	USB_REMOTE_WKUP,
	INIT_DEFER_PROGRESS,
	DOWNLOAD_FIRMWARE_EXT,
	MCC_ENABLED,
	SKIP_FLOWCTRL_EVENT,
	DISABLE_SCAN,
	INTERNAL_REGDB,
	EAPOL_HANDSHAKE_PROTECT,
	REG_COUNTRY_UPDATE,
	CFG80211_REGDB,
};

enum ath6kl_state {
	ATH6KL_STATE_OFF,
	ATH6KL_STATE_ON,
	ATH6KL_STATE_DEEPSLEEP,
	ATH6KL_STATE_CUTPOWER,
	ATH6KL_STATE_WOW,
	ATH6KL_STATE_PRE_SUSPEND,
	ATH6KL_STATE_PRE_SUSPEND_DEEPSLEEP,
};

#define ATH6KL_VAPMODE_MASK	(0xf)	/* each VAP use 4 bits */
#define ATH6KL_VAPMODE_OFFSET	(4)

enum ath6kl_vap_mode {
	ATH6KL_VAPMODE_DISABLED = 0x0,
	ATH6KL_VAPMODE_STA,
	ATH6KL_VAPMODE_AP,	/* w/o 4 address */

	/* NOT YET */
	ATH6KL_VAPMODE_ADHOC,
	ATH6KL_VAPMODE_WDS,	/* AP w/ 4 address */
	ATH6KL_VAPMODE_P2PDEV,	/* Dedicaded P2P-Device */
	ATH6KL_VAPMODE_P2P,	/* P2P-GO or P2P-Client */

	ATH6KL_VAPMODE_LAST = 0xf,
};


#ifdef USB_AUTO_SUSPEND

struct usb_pm_skb_queue_t {
	struct list_head list;
	struct sk_buff *skb;
	int pipeID;
	struct ath6kl *ar;
};

#endif

struct ath6kl {
	struct device *dev;
	struct wiphy *wiphy;

	spinlock_t state_lock;
	enum ath6kl_state state;
	unsigned int testmode;

	unsigned int starving_prevention;

	struct ath6kl_bmi bmi;
	const struct ath6kl_hif_ops *hif_ops;
	const struct ath6kl_htc_ops *htc_ops;
	struct wmi *wmi;
	int tx_pending[ENDPOINT_MAX];
	int total_tx_data_pend;
	struct htc_target *htc_target;
	enum ath6kl_hif_type hif_type;
	void *hif_priv;
	struct list_head vif_list;
	/* Lock to avoid race in vif_list entries among add/del/traverse */
	spinlock_t list_lock;
	u8 num_vif;
	unsigned int vif_max;
	u8 max_norm_iface;
	u8 avail_idx_map;
	enum ath6kl_vap_mode next_mode[ATH6KL_VIF_MAX];
	spinlock_t lock;
	struct semaphore sem;
	struct semaphore wmi_evt_sem;
	u16 listen_intvl_b;
	u16 listen_intvl_t;
	struct low_rssi_scan_params low_rssi_roam_params;
	struct ath6kl_version version;
	u32 target_type;
	u32 target_subtype;
	u8 tx_pwr;
	struct ath6kl_node_mapping node_map[MAX_NODE_NUM];
	u8 ibss_ps_enable;
	bool ibss_if_active;
	u8 node_num;
	u8 next_ep_id;
	struct ath6kl_cookie_pool cookie_data;
	struct ath6kl_cookie_pool cookie_ctrl;
	enum htc_endpoint_id ac2ep_map[WMM_NUM_AC];
	bool ac_stream_active[WMM_NUM_AC];
	u8 ac_stream_pri_map[WMM_NUM_AC];
	u8 hiac_stream_active_pri;
	u8 ac_stream_active_num;
	u8 ep2ac_map[ENDPOINT_MAX];
	enum htc_endpoint_id ctrl_ep;
	struct ath6kl_htc_credit_info credit_state_info;
	u32 user_key_ctrl;          /* FIXME : NEED? */
	struct list_head amsdu_rx_buffer_queue;
	u8 rx_meta_ver;
	enum wlan_low_pwr_state wlan_pwr_state;
	u8 mac_addr[ETH_ALEN];
#define AR_MCAST_FILTER_MAC_ADDR_SIZE  4
	struct {
		void *rx_report;
		size_t rx_report_len;
	} tm;

	struct ath6kl_hw {
		u32 id;
		const char *name;
		u32 dataset_patch_addr;
		u32 app_load_addr;
		u32 app_start_override_addr;
		u32 app_load_ext_addr;
		u32 board_ext_data_addr;
		u32 reserved_ram_size;
		u32 board_addr;
		u32 testscript_addr;
		u32 flags;

		struct ath6kl_hw_fw {
			const char *dir;
			const char *otp;
			const char *fw;
			const char *tcmd;
			const char *patch;
			const char *api2;
			const char *utf;
			const char *testscript;
			const char *fw_ext;
		} fw;

		const char *fw_board;
		const char *fw_default_board;
		const char *fw_epping;
		const char *fw_softmac;
		const char *fw_softmac_2;
	} hw;

	u16 conf_flags;
	wait_queue_head_t event_wq;
	struct ath6kl_mbox_info mbox_info;

	unsigned long flag;

	u8 *fw_board;
	size_t fw_board_len;

	u8 *fw_otp;
	size_t fw_otp_len;

	u8 *fw;
	size_t fw_len;

	u8 *fw_patch;
	size_t fw_patch_len;

	u8 *fw_testscript;
	size_t fw_testscript_len;

	u8 *fw_softmac;
	size_t fw_softmac_len;

	u8 *fw_softmac_2;
	size_t fw_softmac_2_len;

	u8 *fw_ext;
	size_t fw_ext_len;

	unsigned long fw_capabilities[ATH6KL_CAPABILITY_LEN];

	struct workqueue_struct *ath6kl_wq;

	struct dentry *debugfs_phy;

	/* Support P2P or not */
	bool p2p;

	/* Support P2P-Concurrent or not */
	bool p2p_concurrent;

	/* Support P2P-Multi-Channel-Concurrent or not */
	bool p2p_multichan_concurrent;

	/* Need Dedicated-P2P-Device interface or not */
	bool p2p_dedicate;

	/* Support ath6kl-3.2's P2P-Concurrent or not */
	bool p2p_compat;

	/*
	 * STA + AP is a special mode.
	 * Reuse P2P framwork but no P2P function.
	 * At least 4VAPs to support STA(1) + P2P(2) + AP(1) mode.
	 */
#define IS_STA_AP_ONLY(_ar)					\
	((_ar)->p2p_concurrent_ap && ((_ar)->vif_max < TARGET_VIF_MAX))

	/* Support P2P-Concurrent with softAP or not */
	bool p2p_concurrent_ap;

	/* Retry P2P Action frame or not */
	bool p2p_frame_retry;

	/* Not to report P2P Frame to user if not in RoC period */
	bool p2p_frame_not_report;

	/* Allow P2P operate in PASSIVE/IBSS channels */
	bool p2p_in_pasv_chan;

	/* Only scan P2P channels for all P2P interfaces */
	bool p2p_wise_scan;

	/* WAR EV119712 */
	bool p2p_war_bad_intel_go;

	/* WAR CR468120 */
	bool p2p_war_bad_broadcom_go;

	/* WAR CR479897 */
	bool p2p_war_p2p_client_awake;

	/* IOT : Not to append P2P IE in concurrent STA interface */
#define P2P_IE_IN_PROBE_REQ	(1 << 0)
#define P2P_IE_IN_ASSOC_REQ	(1 << 1)
	u8 p2p_ie_not_append;

	bool sche_scan;

#ifdef ATH6KL_SUPPORT_WIFI_KTK
	/* ktk feature is started ot not */
	bool ktk_active;

	/* ktk cipher key */
	u8 ktk_passphrase[16];
#endif

#ifdef ATH6KL_SUPPORT_WIFI_DISC
	/* discovery feature is started ot not */
	bool disc_active;
#endif

	struct ath6kl_btcoex btcoex_info;
	u32 mod_debug_quirks;

#ifdef CONFIG_ATH6KL_DEBUG
	struct {
		struct sk_buff_head fwlog_queue;
		struct completion fwlog_completion;
		bool fwlog_open;
		u32 fwlog_mask;
		unsigned int dbgfs_diag_reg;
		u32 diag_reg_addr_wr;
		u32 diag_reg_val_wr;
		u64 set_tx_series;

		struct {
			unsigned int invalid_rate;
		} war_stats;

		u8 *roam_tbl;
		unsigned int roam_tbl_len;

		u8 keepalive;
		u8 disc_timeout;
		u8 mimo_ps_enable;
		u8 force_passive;
		u16 bgscan_int;
		enum wmi_roam_mode roam_mode;
#define ROAM_NULL_5G_BIAS	(255)
		u8 roam_5g_bias;

		struct smps_param {
			u8 flags;
			u8 rssi_thresh;
			u8 data_thresh;
			u8 mode;
			u8 automatic;
		} smps_params;

		struct lpl_force_enable_param {
			u8 lpl_policy;
			u8 no_blocker_detect;
			u8 no_rfb_detect;
			u8 rsvd;
		} lpl_force_enable_params;

		struct power_param {
			u16 idle_period;
			u16 ps_poll_num;
			u16 dtim;
			u16 tx_wakeup;
			u16 num_tx;
		} power_params;

		struct ht_cap_param {
			u8 isConfig;
			u8 band;
			u8 chan_width_40M_supported;
			u8 short_GI;
			u8 intolerance_40MHz;
		} ht_cap_param[IEEE80211_NUM_BANDS];

		u8 anistat_enable;
	} debug;
#endif /* CONFIG_ATH6KL_DEBUG */

	int wow_irq;
#ifdef CONFIG_ANDROID
#ifdef CONFIG_HAS_WAKELOCK
	struct wake_lock wake_lock;
#endif /* CONFIG_HAS_WAKELOCK */
#endif
	struct ath6kl_p2p_flowctrl *p2p_flowctrl_ctx;
	struct ath6kl_p2p_rc_info *p2p_rc_info_ctx;
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend early_suspend;
#endif /* CONFIG_HAS_EARLYSUSPEND */

#define INIT_DEFER_WAIT_TIMEOUT		(5 * HZ)
	struct work_struct init_defer_wk;
	wait_queue_head_t init_defer_wait_wq;

	u32 tx_on_vif;

	struct reg_info *reg_ctx;

	void (*fw_crash_notify)(struct ath6kl *ar);

	u32 roam_mode;
	u32 bootstrap_mode;
#ifdef USB_AUTO_SUSPEND
	struct usb_pm_skb_queue_t usb_pm_skb_queue;
	spinlock_t   usb_pm_lock;
	unsigned long  usb_autopm_scan;
	int auto_pm_cnt;

#endif
	struct wmi_green_tx_params green_tx_params;

	struct timer_list eapol_shprotect_timer;
	u32 eapol_shprotect_vif;			/* vif mask */

	/* Force wakeup interval = DTIM * dtim_ext */
	u8 dtim_ext;

	/* set if wow pattern set by debug_fs */
	bool get_wow_pattern;
};

static inline void *ath6kl_priv(struct net_device *dev)
{
	return ((struct ath6kl_vif *) netdev_priv(dev))->ar;
}

static inline u32 ath6kl_get_hi_item_addr(struct ath6kl *ar,
					  u32 item_offset)
{
	u32 addr = 0;

	if (ar->target_type == TARGET_TYPE_AR6003)
		addr = ATH6KL_AR6003_HI_START_ADDR + item_offset;
	else if (ar->target_type == TARGET_TYPE_AR6004)
		addr = ATH6KL_AR6004_HI_START_ADDR + item_offset;
	else if (ar->target_type == TARGET_TYPE_AR6006)
		addr = ATH6KL_AR6006_HI_START_ADDR + item_offset;

	return addr;
}

static inline u32 ath6kl_ps_queue_get_age(struct ath6kl_ps_buf_desc *ps_buf)
{
	return ps_buf->age;
}

static inline void ath6kl_ps_queue_set_age(struct ath6kl_ps_buf_desc *ps_buf,
	u32 age)
{
	ps_buf->age = age;
}

static inline void ath6kl_fw_crash_trap(struct ath6kl *ar)
{
	/* Notify to usr */
	if (ar->fw_crash_notify)
		ar->fw_crash_notify(ar);
}

static inline bool ath6kl_is_p2p_ie(const u8 *pos)
{
	return pos[0] == WLAN_EID_VENDOR_SPECIFIC && pos[1] >= 4 &&
		pos[2] == 0x50 && pos[3] == 0x6f &&
		pos[4] == 0x9a && pos[5] == 0x09;
}

static inline bool ath6kl_is_wfd_ie(const u8 *pos)
{
	return pos[0] == WLAN_EID_VENDOR_SPECIFIC && pos[1] >= 4 &&
		pos[2] == 0x50 && pos[3] == 0x6f &&
		pos[4] == 0x9a && pos[5] == 0x0a;
}

static inline struct cfg80211_bss *ath6kl_bss_get(struct ath6kl *ar,
					    struct ieee80211_channel *channel,
					    const u8 *bssid,
					    const u8 *ssid, size_t ssid_len,
					    u16 capa_mask, u16 capa_val)
{
	return cfg80211_get_bss(ar->wiphy,
				channel,
				bssid,
				ssid,
				ssid_len,
				capa_mask,
				capa_val);
}

static inline void ath6kl_bss_put(struct ath6kl *ar, struct cfg80211_bss *pub)
{
#ifdef CFG80211_NEW_REF_BSS_OPER
	cfg80211_put_bss(ar->wiphy, pub);
#else
	cfg80211_put_bss(pub);
#endif
}

int ath6kl_configure_target(struct ath6kl *ar);
void ath6kl_detect_error(unsigned long ptr);
void disconnect_timer_handler(unsigned long ptr);
void init_netdev(struct net_device *dev);
int ath6kl_cookie_init(struct ath6kl *ar);
void ath6kl_cookie_cleanup(struct ath6kl *ar);
void ath6kl_rx(struct htc_target *target, struct htc_packet *packet);
void ath6kl_tx_complete(struct htc_target *context,
						struct list_head *packet_queue);
enum htc_send_full_action ath6kl_tx_queue_full(struct htc_target *target,
					       struct htc_packet *packet);
void ath6kl_stop_txrx(struct ath6kl *ar);
void ath6kl_cleanup_amsdu_rxbufs(struct ath6kl *ar);
int ath6kl_diag_write32(struct ath6kl *ar, u32 address, __le32 value);
int ath6kl_diag_write(struct ath6kl *ar, u32 address, void *data, u32 length);
int ath6kl_diag_read32(struct ath6kl *ar, u32 address, u32 *value);
int ath6kl_diag_read(struct ath6kl *ar, u32 address, void *data, u32 length);
int ath6kl_read_fwlogs(struct ath6kl *ar);
void ath6kl_init_profile_info(struct ath6kl_vif *vif);
void ath6kl_tx_data_cleanup(struct ath6kl *ar);
void ath6kl_tx_data_cleanup_by_if(struct ath6kl_vif *vif);

struct ath6kl_cookie *ath6kl_alloc_cookie(struct ath6kl *ar,
	enum cookie_type cookie_type);
void ath6kl_free_cookie(struct ath6kl *ar, struct ath6kl_cookie *cookie);
bool ath6kl_mgmt_powersave_ap(struct ath6kl_vif *vif, u32 id, u32 freq,
	u32 wait, const u8 *buf, size_t len, bool no_cck,
	bool dont_wait_for_ack, u32 *flags);
int ath6kl_data_tx(struct sk_buff *skb, struct net_device *dev,
	bool bypass_tx_aggr);
int ath6kl_start_tx(struct sk_buff *skb, struct net_device *dev);

void aggr_tx_config(struct ath6kl_vif *vif,
			bool tx_amsdu_seq_pkt,
			bool tx_amsdu_progressive,
			u8 tx_amsdu_max_aggr_num,
			u16 tx_amsdu_max_pdu_len,
			u16 tx_amsdu_timeout);
void aggr_config(struct ath6kl_vif *vif,
			u16 rx_aggr_timeout);
struct aggr_info *aggr_init(struct ath6kl_vif *vif);
struct aggr_conn_info *aggr_init_conn(struct ath6kl_vif *vif);

void ath6kl_rx_refill(struct htc_target *target,
		      enum htc_endpoint_id endpoint);
void ath6kl_refill_amsdu_rxbufs(struct ath6kl *ar, int count);
struct htc_packet *ath6kl_alloc_amsdu_rxbuf(struct htc_target *target,
					    enum htc_endpoint_id endpoint,
					    int len);

void aggr_module_destroy(struct aggr_info *aggr);
void aggr_module_destroy_conn(struct aggr_conn_info *aggr_conn);
void aggr_reset_state(struct aggr_conn_info *aggr_conn);

struct ath6kl_sta *ath6kl_find_sta(struct ath6kl_vif *vif, u8 * node_addr);
struct ath6kl_sta *ath6kl_find_sta_by_aid(struct ath6kl_vif *vif, u8 aid);

void ath6kl_ready_event(void *devt, u8 * datap, u32 sw_ver, u32 abi_ver);
int ath6kl_control_tx(void *devt, struct sk_buff *skb,
		      enum htc_endpoint_id eid);
void ath6kl_connect_event(struct ath6kl_vif *vif, u16 channel,
			  u8 *bssid, u16 listen_int,
			  u16 beacon_int, enum network_type net_type,
			  u8 beacon_ie_len, u8 assoc_req_len,
			  u8 assoc_resp_len, u8 *assoc_info);
void ath6kl_connect_ap_mode_bss(struct ath6kl_vif *vif, u16 channel,
				u8 *beacon, u8 beacon_len);
void ath6kl_connect_ap_mode_sta(struct ath6kl_vif *vif, u8 aid, u8 *mac_addr,
				u8 keymgmt, u8 ucipher, u8 auth,
				u16 assoc_req_len, u8 *assoc_info,
				u8 apsd_info, u8 phymode);
void ath6kl_disconnect_event(struct ath6kl_vif *vif, u8 reason,
			     u8 *bssid, u8 assoc_resp_len,
			     u8 *assoc_info, u16 prot_reason_status);
void ath6kl_tkip_micerr_event(struct ath6kl_vif *vif, u8 keyid, bool ismcast);
void ath6kl_txpwr_rx_evt(void *devt, u8 tx_pwr);
void ath6kl_scan_complete_evt(struct ath6kl_vif *vif, int status);
void ath6kl_tgt_stats_event(struct ath6kl_vif *vif, u8 *ptr, u32 len);
void ath6kl_indicate_tx_activity(void *devt, u8 traffic_class, bool active);
enum htc_endpoint_id ath6kl_ac2_endpoint_id(void *devt, u8 ac);

void ath6kl_pspoll_event(struct ath6kl_vif *vif, u8 aid);

void ath6kl_dtimexpiry_event(struct ath6kl_vif *vif);
int ath6kl_disconnect(struct ath6kl_vif *vif);
void aggr_recv_delba_req_evt(struct ath6kl_vif *vif, u8 tid, u8 initiator);
void aggr_recv_addba_req_evt(struct ath6kl_vif *vif, u8 tid, u16 seq_no,
			     u8 win_sz);
void aggr_recv_addba_resp_evt(struct ath6kl_vif *vif, u8 tid,
	u16 amsdu_sz, u8 status);
void ath6kl_wakeup_event(void *dev);

void ath6kl_reset_device(struct ath6kl *ar, u32 target_type,
			 bool wait_fot_compltn, bool cold_reset);
void ath6kl_init_control_info(struct ath6kl_vif *vif);
void ath6kl_deinit_if_data(struct ath6kl_vif *vif);
void ath6kl_core_free(struct ath6kl *ar);
struct ath6kl_vif *ath6kl_vif_first(struct ath6kl *ar);
void ath6kl_cleanup_vif(struct ath6kl_vif *vif, bool wmi_ready);
int ath6kl_init_hw_start(struct ath6kl *ar);
int ath6kl_init_hw_stop(struct ath6kl *ar);
void ath6kl_check_wow_status(struct ath6kl *ar);
void ath6kl_htc_pipe_attach(struct ath6kl *ar);
void ath6kl_htc_mbox_attach(struct ath6kl *ar);

void ath6kl_ps_queue_init(struct ath6kl_ps_buf_head *psq,
			enum ps_queue_type queue_type,
			u32 age_cycle,
			u32 max_depth);
void ath6kl_ps_queue_purge(struct ath6kl_ps_buf_head *psq);
int ath6kl_ps_queue_empty(struct ath6kl_ps_buf_head *psq);
int ath6kl_ps_queue_depth(struct ath6kl_ps_buf_head *psq);
void ath6kl_ps_queue_stat(struct ath6kl_ps_buf_head *psq, int *depth,
	u32 *enqueued, u32 *enqueued_err, u32 *dequeued, u32 *aged);
struct ath6kl_ps_buf_desc *ath6kl_ps_queue_dequeue(
	struct ath6kl_ps_buf_head *psq);
int ath6kl_ps_queue_enqueue_mgmt(struct ath6kl_ps_buf_head *psq, const u8 *buf,
	u16 len, u32 id, u32 freq, u32 wait, bool no_cck,
	bool dont_wait_for_ack);
int ath6kl_ps_queue_enqueue_data(struct ath6kl_ps_buf_head *psq,
	struct sk_buff *skb);
void ath6kl_ps_queue_age_handler(unsigned long ptr);
void ath6kl_ps_queue_age_start(struct ath6kl_sta *conn);
void ath6kl_ps_queue_age_stop(struct ath6kl_sta *conn);

struct bss_post_proc *ath6kl_bss_post_proc_init(struct ath6kl_vif *vif);
void ath6kl_bss_post_proc_deinit(struct ath6kl_vif *vif);
void ath6kl_bss_post_proc_bss_scan_start(struct ath6kl_vif *vif);
int ath6kl_bss_post_proc_bss_complete_event(struct ath6kl_vif *vif);
void ath6kl_bss_post_proc_bss_info(struct ath6kl_vif *vif,
				struct ieee80211_mgmt *mgmt,
				int len,
				s32 snr,
				struct ieee80211_channel *channel);

#ifdef CONFIG_ANDROID
void ath6kl_sdio_init_msm(void);
void ath6kl_sdio_exit_msm(void);
#endif

#ifdef ATH6KL_BUS_VOTE
int ath6kl_hsic_init_msm(u8 *has_vreg);
void ath6kl_hsic_exit_msm(void);
int ath6kl_hsic_bind(int bind);
#endif

#ifdef ATH6KL_HSIC_RECOVER
void ath6kl_hsic_rediscovery(void);
#endif

void ath6kl_fw_crash_notify(struct ath6kl *ar);
void ath6kl_indicate_wmm_schedule_change(void *devt, bool active);
int _string_to_mac(char *string, int len, u8 *macaddr);
void ath6kl_flush_pend_skb(struct ath6kl_vif *vif);

#if   defined(CONFIG_ANDROID) || defined(USB_AUTO_SUSPEND)
int ath6kl_android_enable_wow_default(struct ath6kl *ar);
bool ath6kl_android_need_wow_suspend(struct ath6kl *ar);
#endif

#ifdef ATH6KL_SUPPORT_WLAN_HB
int ath6kl_enable_wow_hb(struct ath6kl *ar);
#endif

int ath6kl_fw_watchdog_enable(struct ath6kl *ar);
int ath6kl_fw_crash_cold_reset_enable(struct ath6kl *ar);

extern unsigned int htc_bundle_recv;
extern unsigned int htc_bundle_send;
extern unsigned int htc_bundle_send_timer;
extern unsigned int htc_bundle_send_th;
#ifdef CE_SUPPORT
extern unsigned int ath6kl_ce_flags;
#endif

#ifdef CONFIG_ANDROID
extern unsigned int ath6kl_bt_on;
#endif
#endif /* CORE_H */
