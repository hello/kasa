/*
 * Copyright (c) 2010, Atheros Communications Inc.
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
#ifndef _NET80211_IEEE80211_IOCTL_H_
#define _NET80211_IEEE80211_IOCTL_H_
enum {
	IEEE80211_PARAM_TURBO			= 1,	/*
	turbo mode */
	IEEE80211_PARAM_MODE                    = 2,	/*
	phy mode (11a, 11b, etc.) */
	IEEE80211_PARAM_AUTHMODE                = 3,	/*
	authentication mode */
	IEEE80211_PARAM_PROTMODE                = 4,	/*
	802.11g protection */
	IEEE80211_PARAM_MCASTCIPHER             = 5,	/*
	multicast/default cipher */
	IEEE80211_PARAM_MCASTKEYLEN             = 6,	/*
	multicast key length */
	IEEE80211_PARAM_UCASTCIPHERS            = 7,	/*
	unicast cipher suites */
	IEEE80211_PARAM_UCASTCIPHER             = 8,	/*
	unicast cipher */
	IEEE80211_PARAM_UCASTKEYLEN             = 9,	/*
	unicast key length */
	IEEE80211_PARAM_WPA                     = 10,	/*
	WPA mode (0,1,2) */
	IEEE80211_PARAM_ROAMING                 = 12,	/*
	roaming mode */
	IEEE80211_PARAM_PRIVACY                 = 13,	/*
	privacy invoked */
	IEEE80211_PARAM_COUNTERMEASURES         = 14,	/*
	WPA/TKIP countermeasures */
	IEEE80211_PARAM_DROPUNENCRYPTED         = 15,	/*
	discard unencrypted frames */
	IEEE80211_PARAM_DRIVER_CAPS             = 16,	/*
	driver capabilities */
	IEEE80211_PARAM_MACCMD                  = 17,	/*
	MAC ACL operation */
	IEEE80211_PARAM_WMM                     = 18,	/*
	WMM mode (on, off) */
	IEEE80211_PARAM_HIDESSID                = 19,	/*
	hide SSID mode (on, off) */
	IEEE80211_PARAM_APBRIDGE                = 20,	/*
	AP inter-sta bridging */
	IEEE80211_PARAM_KEYMGTALGS              = 21,	/*
	key management algorithms */
	IEEE80211_PARAM_RSNCAPS                 = 22,	/*
	RSN capabilities */
	IEEE80211_PARAM_INACT                   = 23,	/*
	station inactivity timeout */
	IEEE80211_PARAM_INACT_AUTH              = 24,	/*
	station auth inact timeout */
	IEEE80211_PARAM_INACT_INIT              = 25,	/*
	station init inact timeout */
	IEEE80211_PARAM_ABOLT                   = 26,	/*
	Atheros Adv. Capabilities */
	IEEE80211_PARAM_DTIM_PERIOD             = 28,	/*
	DTIM period (beacons) */
	IEEE80211_PARAM_BEACON_INTERVAL         = 29,	/*
	beacon interval (ms) */
	IEEE80211_PARAM_DOTH                    = 30,	/*
	11.h is on/off */
	IEEE80211_PARAM_PWRTARGET               = 31,	/*
	Current Channel Pwr Constraint */
	IEEE80211_PARAM_GENREASSOC              = 32,	/*
	Generate a reassociation request */
	IEEE80211_PARAM_COMPRESSION             = 33,	/*
	compression */
	IEEE80211_PARAM_FF                      = 34,	/*
	fast frames support */
	IEEE80211_PARAM_XR                      = 35,	/*
	XR support */
	IEEE80211_PARAM_BURST                   = 36,	/*
	burst mode */
	IEEE80211_PARAM_PUREG                   = 37,	/*
	pure 11g (no 11b stations) */
	IEEE80211_PARAM_AR                      = 38,	/*
	AR support */
	IEEE80211_PARAM_WDS                     = 39,	/*
	Enable 4 address processing */
	IEEE80211_PARAM_BGSCAN                  = 40,	/*
	bg scanning (on, off) */
	IEEE80211_PARAM_BGSCAN_IDLE             = 41,	/*
	bg scan idle threshold */
	IEEE80211_PARAM_BGSCAN_INTERVAL         = 42,	/*
	bg scan interval */
	IEEE80211_PARAM_MCAST_RATE              = 43,	/*
	Multicast Tx Rate */
	IEEE80211_PARAM_COVERAGE_CLASS          = 44,	/*
	coverage class */
	IEEE80211_PARAM_COUNTRY_IE              = 45,	/*
	enable country IE */
	IEEE80211_PARAM_SCANVALID               = 46,	/*
	scan cache valid threshold */
	IEEE80211_PARAM_ROAM_RSSI_11A           = 47,	/*
	rssi threshold in 11a */
	IEEE80211_PARAM_ROAM_RSSI_11B           = 48,	/*
	rssi threshold in 11b */
	IEEE80211_PARAM_ROAM_RSSI_11G           = 49,	/*
	rssi threshold in 11g */
	IEEE80211_PARAM_ROAM_RATE_11A           = 50,	/*
	tx rate threshold in 11a */
	IEEE80211_PARAM_ROAM_RATE_11B           = 51,	/*
	tx rate threshold in 11b */
	IEEE80211_PARAM_ROAM_RATE_11G           = 52,	/*
	tx rate threshold in 11g */
	IEEE80211_PARAM_UAPSDINFO               = 53,	/*
	value for qos info field */
	IEEE80211_PARAM_SLEEP                   = 54,	/*
	force sleep/wake */
	IEEE80211_PARAM_QOSNULL                 = 55,	/*
	force sleep/wake */
	IEEE80211_PARAM_PSPOLL                  = 56,	/*
	force ps-poll generation (sta only) */
	IEEE80211_PARAM_EOSPDROP                = 57,	/*
	force uapsd EOSP drop (ap only) */
	IEEE80211_PARAM_MARKDFS                 = 58,	/*
	mark a dfs interference channel when found */
	IEEE80211_PARAM_REGCLASS                = 59,	/*
	enable regclass ids in country IE */
	IEEE80211_PARAM_CHANBW                  = 60,	/*
	set chan bandwidth preference */
	IEEE80211_PARAM_WMM_AGGRMODE            = 61,	/*
	set WMM Aggressive Mode */
	IEEE80211_PARAM_SHORTPREAMBLE		= 62,	/*
	enable/disable short Preamble */
	IEEE80211_PARAM_BLOCKDFSCHAN		= 63,	/*
	enable/disable use of DFS channels */
	IEEE80211_PARAM_CWM_MODE                = 64,	/*
	CWM mode */
	IEEE80211_PARAM_CWM_EXTOFFSET           = 65,	/*
	CWM extension channel offset */
	IEEE80211_PARAM_CWM_EXTPROTMODE         = 66,	/*
	CWM extension channel protection mode */
	IEEE80211_PARAM_CWM_EXTPROTSPACING      = 67,	/*
	CWM extension channel protection spacing */
	IEEE80211_PARAM_CWM_ENABLE              = 68,	/*
	CWM state machine enabled */
	IEEE80211_PARAM_CWM_EXTBUSYTHRESHOLD    = 69,	/*
	CWM extension channel busy threshold */
	IEEE80211_PARAM_CWM_CHWIDTH             = 70,	/*
	CWM STATE: current channel width */
	IEEE80211_PARAM_SHORT_GI                = 71,	/*
	half GI */
	IEEE80211_PARAM_FAST_CC                 = 72,	/*
	fast channel change */

	/*
	 * 11n A-MPDU, A-MSDU support
	 */
	IEEE80211_PARAM_AMPDU                   = 73,	/*
	11n a-mpdu support */
	IEEE80211_PARAM_AMPDU_LIMIT             = 74,	/*
	a-mpdu length limit */
	IEEE80211_PARAM_AMPDU_DENSITY           = 75,	/*
	a-mpdu density */
	IEEE80211_PARAM_AMPDU_SUBFRAMES         = 76,	/*
	a-mpdu subframe limit */
	IEEE80211_PARAM_AMSDU                   = 77,	/*
	a-msdu support */
	IEEE80211_PARAM_AMSDU_LIMIT             = 78,	/*
	a-msdu length limit */

	IEEE80211_PARAM_COUNTRYCODE             = 79,	/*
	Get country code */
	IEEE80211_PARAM_TX_CHAINMASK            = 80,	/*
	Tx chain mask */
	IEEE80211_PARAM_RX_CHAINMASK            = 81,	/*
	Rx chain mask */
	IEEE80211_PARAM_RTSCTS_RATECODE         = 82,	/*
	RTS Rate code */
	IEEE80211_PARAM_HT_PROTECTION           = 83,	/*
	Protect traffic in HT mode */
	IEEE80211_PARAM_RESET_ONCE              = 84,	/*
	Force a reset */
	IEEE80211_PARAM_SETADDBAOPER            = 85,	/*
	Set ADDBA mode */
	IEEE80211_PARAM_TX_CHAINMASK_LEGACY     = 86,	/*
	Tx chain mask for legacy clients */
	IEEE80211_PARAM_11N_RATE                = 87,	/*
	Set ADDBA mode */
	IEEE80211_PARAM_11N_RETRIES             = 88,	/*
	Tx chain mask for legacy clients */
	IEEE80211_PARAM_DBG_LVL                 = 89,	/*
	Debug Level for specific VAP */
	IEEE80211_PARAM_WDS_AUTODETECT          = 90,	/*
	Configurable Auto Detect/Delba for WDS mode */
	IEEE80211_PARAM_ATH_RADIO               = 91,	/*
	returns the name of the radio being used */
	IEEE80211_PARAM_IGNORE_11DBEACON        = 92,	/*
	Don't process 11d beacon (on, off) */
	IEEE80211_PARAM_STA_FORWARD             = 93,	/*
	Enable client 3 addr forwarding */

	/*
	 * Mcast Enhancement support
	 */
	IEEE80211_PARAM_ME                      = 94,	/*
	Set Mcast enhancement option: 0 disable, 1 tunneling, 2 translate */
	IEEE80211_PARAM_MEDUMP                  = 95,	/*
	Dump the snoop table for mcast enhancement */
	IEEE80211_PARAM_MEDEBUG                 = 96,	/*
	mcast enhancement debug level */
	IEEE80211_PARAM_ME_SNOOPLENGTH          = 97,	/*
	mcast snoop list length */
	IEEE80211_PARAM_ME_TIMER                = 98,	/*
	Set Mcast enhancement timer to update the snoop list, in msec */
	IEEE80211_PARAM_ME_TIMEOUT              = 99,	/*
	Set Mcast enhancement timeout for STA's without traffic, in msec */
	IEEE80211_PARAM_PUREN                   = 100,	/*
	pure 11n (no 11bg/11a stations) */
	IEEE80211_PARAM_BASICRATES              = 101,	/*
	Change Basic Rates */
	IEEE80211_PARAM_NO_EDGE_CH              = 102,	/*
	Avoid band edge channels */
	IEEE80211_PARAM_WEP_TKIP_HT             = 103,	/*
	Enable HT rates with WEP/TKIP encryption */
	IEEE80211_PARAM_RADIO                   = 104,	/*
	radio on/off */
	IEEE80211_PARAM_NETWORK_SLEEP           = 105,	/*
	set network sleep enable/disable */
	IEEE80211_PARAM_DROPUNENC_EAPOL         = 106,

	/*
	 * Headline block removal
	 */
	IEEE80211_PARAM_HBR_TIMER               = 107,
	IEEE80211_PARAM_HBR_STATE               = 108,

	/*
	 * Unassociated power consumpion improve
	 */
	IEEE80211_PARAM_SLEEP_PRE_SCAN          = 109,
	IEEE80211_PARAM_SCAN_PRE_SLEEP          = 110,
	IEEE80211_PARAM_VAP_IND                 = 111,	/*
	Independent VAP mode for Repeater and AP-STA config */

	/* support for wapi: set auth mode and key */
	IEEE80211_PARAM_SETWAPI                 = 112,
	IEEE80211_IOCTL_GREEN_AP_PS_ENABLE      = 113,
	IEEE80211_IOCTL_GREEN_AP_PS_TIMEOUT     = 114,
	IEEE80211_IOCTL_GREEN_AP_PS_ON_TIME     = 115,
	IEEE80211_PARAM_WPS                     = 116,
	IEEE80211_PARAM_RX_RATE                 = 117,
	IEEE80211_PARAM_CHEXTOFFSET             = 118,
	IEEE80211_PARAM_CHSCANINIT              = 119,
	IEEE80211_PARAM_MPDU_SPACING            = 120,
	IEEE80211_PARAM_HT40_INTOLERANT         = 121,
	IEEE80211_PARAM_CHWIDTH                 = 122,
	IEEE80211_PARAM_EXTAP                   = 123,	/*
	Enable client 3 addr forwarding */
	IEEE80211_PARAM_COEXT_DISABLE           = 124,
	IEEE80211_PARAM_ME_DROPMCAST            = 125,	/*
	drop mcast if empty entry */
	IEEE80211_PARAM_ME_SHOWDENY             = 126,	/*
	show deny table for mcast enhancement */
	IEEE80211_PARAM_ME_CLEARDENY            = 127,	/*
	clear deny table for mcast enhancement */
	IEEE80211_PARAM_ME_ADDDENY              = 128,	/*
	add deny entry for mcast enhancement */
	IEEE80211_PARAM_GETIQUECONFIG           = 129,	/*
	print out the iQUE config */
	IEEE80211_PARAM_CCMPSW_ENCDEC           = 130,	/*
	support for ccmp s/w encrypt decrypt */

	/* Support for repeater placement */
	IEEE80211_PARAM_CUSTPROTO_ENABLE        = 131,
	IEEE80211_PARAM_GPUTCALC_ENABLE         = 132,
	IEEE80211_PARAM_DEVUP                   = 133,
	IEEE80211_PARAM_MACDEV                  = 134,
	IEEE80211_PARAM_MACADDR1                = 135,
	IEEE80211_PARAM_MACADDR2                = 136,
	IEEE80211_PARAM_GPUTMODE                = 137,
	IEEE80211_PARAM_TXPROTOMSG              = 138,
	IEEE80211_PARAM_RXPROTOMSG              = 139,
	IEEE80211_PARAM_STATUS                  = 140,
	IEEE80211_PARAM_ASSOC                   = 141,
	IEEE80211_PARAM_NUMSTAS                 = 142,
	IEEE80211_PARAM_STA1ROUTE               = 143,
	IEEE80211_PARAM_STA2ROUTE               = 144,
	IEEE80211_PARAM_STA3ROUTE               = 145,
	IEEE80211_PARAM_STA4ROUTE               = 146,
	IEEE80211_PARAM_TDLS_ENABLE             = 147,  /* TDLS support */
	IEEE80211_PARAM_SET_TDLS_RMAC           = 148,  /* Set TDLS link */
	IEEE80211_PARAM_CLR_TDLS_RMAC           = 149,  /* Clear TDLS link */
	IEEE80211_PARAM_TDLS_MACADDR1           = 150,
	IEEE80211_PARAM_TDLS_MACADDR2           = 151,
	IEEE80211_PARAM_TDLS_ACTION             = 152,
#ifdef ATH_SUPPORT_AOW
	IEEE80211_PARAM_SWRETRIES               = 153,
	IEEE80211_PARAM_RTSRETRIES              = 154,
	IEEE80211_PARAM_AOW_LATENCY             = 155,
	IEEE80211_PARAM_AOW_STATS               = 156,
	IEEE80211_PARAM_AOW_LIST_AUDIO_CHANNELS = 157,
	IEEE80211_PARAM_AOW_PLAY_LOCAL          = 158,
	IEEE80211_PARAM_AOW_CLEAR_AUDIO_CHANNELS = 159,
	IEEE80211_PARAM_AOW_INTERLEAVE          = 160,
	IEEE80211_PARAM_AOW_ER                  = 161,
	IEEE80211_PARAM_AOW_PRINT_CAPTURE       = 162,
	IEEE80211_PARAM_AOW_ENABLE_CAPTURE      = 163,
	IEEE80211_PARAM_AOW_FORCE_INPUT         = 164,
	IEEE80211_PARAM_AOW_EC                  = 165,
	IEEE80211_PARAM_AOW_EC_FMAP             = 166,
	IEEE80211_PARAM_AOW_ES                  = 167,
	IEEE80211_PARAM_AOW_ESS                 = 168,
	IEEE80211_PARAM_AOW_ESS_COUNT           = 169,
	IEEE80211_PARAM_AOW_ESTATS              = 170,
	IEEE80211_PARAM_AOW_AS                  = 171,
	IEEE80211_PARAM_AOW_PLAY_RX_CHANNEL     = 172,
	IEEE80211_PARAM_AOW_SIM_CTRL_CMD        = 173,
	IEEE80211_PARAM_AOW_FRAME_SIZE          = 174,
	IEEE80211_PARAM_AOW_ALT_SETTING         = 175,
	IEEE80211_PARAM_AOW_ASSOC_ONLY          = 176,
	IEEE80211_PARAM_AOW_EC_RAMP             = 177,
	IEEE80211_PARAM_AOW_DISCONNECT_DEVICE   = 178,
#endif  /* ATH_SUPPORT_AOW */
	IEEE80211_PARAM_PERIODIC_SCAN           = 179,
#ifdef ATH_SUPPORT_AP_WDS_COMBO
	IEEE80211_PARAM_NO_BEACON               = 180,	/*
	No beacon xmit on VAP */
	IEEE80211_PARAM_NO_BEACON               = 181,	/*
	No beacon xmit on VAP */
#endif
	IEEE80211_PARAM_VAP_COUNTRY_IE          = 182,	/*
	802.11d country ie per vap */
	IEEE80211_PARAM_VAP_DOTH                = 183,	/*
	802.11h per vap */
	IEEE80211_PARAM_STA_QUICKKICKOUT        = 184,	/*
	station quick kick out */
	IEEE80211_PARAM_AUTO_ASSOC              = 185,
#ifdef ATH_SUPPORT_IQUE
	IEEE80211_PARAM_RC_VIVO                 = 186,	/*
	Use separate rate control algorithm for VI/VO queues */
#endif
	IEEE80211_PARAM_CLR_APPOPT_IE           = 187,	/*
	Clear Cached App/OptIE */
	IEEE80211_PARAM_SW_WOW                  = 188,	/* wow by sw */
#ifdef UMAC_SUPPORT_WDS
	IEEE80211_PARAM_ADD_WDS_ADDR            = 190,	/* add wds addr */
#endif
	IEEE80211_PARAM_WAPIREKEY_USK           = 191,
	IEEE80211_PARAM_WAPIREKEY_MSK           = 192,
	IEEE80211_PARAM_WAPIREKEY_UPDATE        = 193,
	IEEE80211_PARAM_RXBUF_LIFETIME		= 194,	/*
	lifetime of reycled rx buffers */
#ifdef __CARRIER_PLATFORM__
	IEEE80211_PARAM_PLTFRM_PRIVATE          = 195, /*
	platfrom's private ioctl*/
#endif
#ifdef CONFIG_RCPI
	IEEE80211_PARAM_TDLS_RCPI_HI            = 196,	/*
	RCPI params: hi,lo threshold and margin */
	IEEE80211_PARAM_TDLS_RCPI_LOW           = 197,	/*
	RCPI params: hi,lo threshold and margin */
	IEEE80211_PARAM_TDLS_RCPI_MARGIN        = 198,	/*
	RCPI params: hi,lo threshold and margin */
	IEEE80211_PARAM_TDLS_SET_RCPI           = 199,	/*
	RCPI params: set hi,lo threshold and margin */
	IEEE80211_PARAM_TDLS_GET_RCPI           = 200,	/*
	RCPI params: get hi,lo threshold and margin */
#endif
	IEEE80211_PARAM_TDLS_PEER_UAPSD_ENABLE  = 201,	/*
	Enable TDLS Peer U-APSD Power Save feature */
	IEEE80211_PARAM_TDLS_DIALOG_TOKEN       = 202,	/*
	Dialog Token of TDLS Discovery Request */
	IEEE80211_PARAM_TDLS_DISCOVERY_REQ      = 203,	/*
	Do TDLS Discovery Request */
	IEEE80211_PARAM_TDLS_AUTO_ENABLE        = 204,	/*
	Enable TDLS auto setup */
	IEEE80211_PARAM_TDLS_QOSNULL            = 205,	/*
	Send QOSNULL frame to remote peer */
	IEEE80211_PARAM_TDLS_OFF_TIMEOUT        = 206,	/*
	Seconds of Timeout for off table : TDLS_OFF_TABLE_TIMEOUT */
	IEEE80211_PARAM_TDLS_TDB_TIMEOUT        = 207,	/*
	Seconds of Timeout for teardown block : TD_BLOCK_TIMEOUT */
	IEEE80211_PARAM_TDLS_WEAK_TIMEOUT       = 208,	/*
	Seconds of Timeout for weak peer : WEAK_PEER_TIMEOUT */
	IEEE80211_PARAM_TDLS_RSSI_MARGIN        = 209,	/*
	RSSI margin between AP path and Direct link one */
	IEEE80211_PARAM_TDLS_RSSI_UPPER_BOUNDARY = 210,	/*
	RSSI upper boundary of Direct link path */
	IEEE80211_PARAM_TDLS_RSSI_LOWER_BOUNDARY = 211,	/*
	RSSI lower boundary of Direct link path */
	IEEE80211_PARAM_TDLS_PATH_SELECT        = 212,	/*
	Enable TDLS Path Select bewteen AP path and Direct link one */
	IEEE80211_PARAM_TDLS_RSSI_OFFSET        = 213,	/*
	RSSI offset of TDLS Path Select */
	IEEE80211_PARAM_TDLS_PATH_SEL_PERIOD    = 214,	/*
	Period time of Path Select */
	IEEE80211_PARAM_STA_PWR_SET_PSPOLL      = 215,	/*
	Set ips_use_pspoll flag for STA */
	IEEE80211_PARAM_MP_DETECT               = 216,	/*
	Magic packet detect */
	IEEE80211_PARAM_TDLS_TABLE_QUERY        = 217,	/*
	Print Table info. of AUTO-TDLS */
	IEEE80211_PARAM_NO_STOP_DISASSOC        = 218,	/*
	Do not send disassociation frame on stopping vap */
	IEEE80211_PARAM_SCAN_BAND               = 219,	/*
	only scan channels of requested band */
	IEEE80211_PARAM_MAX_CLIENTS             = 220,
	IEEE80211_PARAM_SUSPEND_SET		= 221,  /* suspend mode set*/
};

#if 1
/* These 16 ioctl are wireless device private.
 * Each driver is free to use them for whatever purpose it chooses,
 * however the driver *must* export the description of those ioctls
 * with SIOCGIWPRIV and *must* use arguments as defined below.
 * If you don't follow those rules, DaveM is going to hate you (reason :
 * it make mixed 32/64bit operation impossible).
 */
#define SIOCIWFIRSTPRIV                 0x8BE0
#define SIOCIWLASTPRIV                  0x8BFF
#endif

#define LINUX_PVT_WIOCTL                (SIOCDEVPRIVATE +  1)
/* #define ATHCFG_WCMD_IOCTL               (SIOCDEVPRIVATE + 15) */   /*
	IEEE80211_IOCTL_VENDOR*/
#define SIOCIOCTLTX99                   (SIOCDEVPRIVATE + 13)
#define SIOCIOCTLAPP                    (SIOCDEVPRIVATE + 12)

#define	IEEE80211_IOCTL_SETPARAM        (SIOCIWFIRSTPRIV +  0)
#define	IEEE80211_IOCTL_GETPARAM        (SIOCIWFIRSTPRIV +  1)
#define IEEE80211_IOCTL_SETMLME			(SIOCIWFIRSTPRIV +  6)
#define	IEEE80211_IOCTL_ADDMAC          (SIOCIWFIRSTPRIV + 10)  /*
	Add ACL MAC Address */
#define	IEEE80211_IOCTL_DELMAC          (SIOCIWFIRSTPRIV + 12)  /*
	Del ACL MAC Address */
#define IEEE80211_IOCTL_KICKMAC         (SIOCIWFIRSTPRIV + 15)
#define IEEE80211_IOCTL_SET_APPIEBUF    (SIOCIWFIRSTPRIV + 20)
#define IEEE80211_IOCTL_GET_MACADDR     (SIOCIWFIRSTPRIV + 29)  /*
	Get ACL List */

#define IEEE80211_ADDR_LEN  6       /* size of 802.11 address */
#define IEEE80211_NWID_LEN                  32
/*
 * MLME state manipulation request.  IEEE80211_MLME_ASSOC
 * only makes sense when operating as a station.  The other
 * requests can be used when operating as a station or an
 * ap (to effect a station).
 */
struct ieee80211req_mlme {
	u_int8_t	im_op;		/* operation to perform */
#define	IEEE80211_MLME_ASSOC		1	/* associate station */
#define	IEEE80211_MLME_DISASSOC		2	/* disassociate station */
#define	IEEE80211_MLME_DEAUTH		3	/* deauthenticate station */
#define	IEEE80211_MLME_AUTHORIZE	4	/* authorize station */
#define	IEEE80211_MLME_UNAUTHORIZE	5	/* unauthorize station */
#define	IEEE80211_MLME_STOP_BSS		6	/* stop bss */
#define IEEE80211_MLME_CLEAR_STATS	7	/* clear station statistic */
	u_int8_t	im_ssid_len;	/* length of optional ssid */
	u_int16_t	im_reason;	/* 802.11 reason code */
	u_int8_t	im_macaddr[IEEE80211_ADDR_LEN];
	u_int8_t	im_ssid[IEEE80211_NWID_LEN];
};

#endif /* _NET80211_IEEE80211_IOCTL_H_ */
