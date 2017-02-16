/** @file wlan_priv.h
 * @brief This file contains tables for private IOCTL call.
 *  
 * Copyright (C) 2003-2008, Marvell International Ltd.  
 *
 * This software file (the "File") is distributed by Marvell International 
 * Ltd. under the terms of the GNU General Public License Version 2, June 1991 
 * (the "License").  You may use, redistribute and/or modify this File in 
 * accordance with the terms and conditions of the License, a copy of which 
 * is available along with the File in the gpl.txt file or by writing to 
 * the Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 
 * 02111-1307 or on the worldwide web at http://www.gnu.org/licenses/gpl.txt.
 *
 * THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE 
 * IMPLIED WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE 
 * ARE EXPRESSLY DISCLAIMED.  The License provides additional details about 
 * this warranty disclaimer.
 *
 */
/********************************************************
Change log:
	11/15/07: Created
	12/24/07: Move private ioctl ID from wlan_wext.h
********************************************************/

#ifndef	_WLAN_PRIV_H_
#define	_WLAN_PRIV_H_

/** PRIVATE CMD ID */
#define	WLANIOCTL			0x8BE0

/** Private command ID to set WPA IE */
#define WLANSETWPAIE			(WLANIOCTL + 0)

/** Private command ID to set CIS dump */
#define WLANCISDUMP 			(WLANIOCTL + 1)

#ifdef MFG_CMD_SUPPORT
/** Private command ID for MFG command support */
#define	WLANMANFCMD			(WLANIOCTL + 2)
#endif

/** Private command ID to set/get register value */
#define	WLANREGRDWR			(WLANIOCTL + 3)

/** Private command to get/set 256 chars */
#define WLAN_SET_GET_256_CHAR		(WLANIOCTL + 5)
/** Private command to read/write passphrase */
#define WLANPASSPHRASE			1

/** Private command ID to Host command */
#define	WLANHOSTCMD			(WLANIOCTL + 4)

/** Private command ID for ARP filter */
#define WLANARPFILTER			(WLANIOCTL + 6)

/** Private command ID to set/get int */
#define WLAN_SETINT_GETINT		(WLANIOCTL + 7)
/** Private command ID to get Noise Floor value */
#define WLANNF					1
/** Private command ID to get RSSI */
#define WLANRSSI				2
/** Private command ID to set/get BG scan */
#define WLANBGSCAN				4
/** Private command ID to enabled 11d support */
#define WLANENABLE11D				5
/** Private command ID to set/get AdHoc G rate */
#define WLANADHOCGRATE				6
/** Private command ID to set/get SDIO clock */
#define WLANSDIOCLOCK				7
/** Private command ID to enable WMM */
#define WLANWMM_ENABLE				8
/** Private command ID to set Null packet generation */
#define WLANNULLGEN				10
/** Private command ID to set AdHoc */
#define WLANADHOCCSET				11
/** Private command ID to set AdHoc G protection */
#define WLAN_ADHOC_G_PROT			12

/** Private command ID to set/get none */
#define WLAN_SETNONE_GETNONE	        (WLANIOCTL + 8)
/** Private command ID to turn on radio */
#define WLANRADIOON                 		1
/** Private command ID to turn off radio */
#define WLANRADIOOFF                		2
/** Private command ID to remove AdHoc AES key */
#define WLANREMOVEADHOCAES          		3
/** Private command ID to stop AdHoc mode */
#define WLANADHOCSTOP               		4
/** Private command ID to set WLAN crypto test */
#define WLANCRYPTOTEST				6
#ifdef REASSOCIATION
/** Private command ID to set reassociation to auto mode */
#define WLANREASSOCIATIONAUTO			7
/** Private command ID to set reassociation to user mode */
#define WLANREASSOCIATIONUSER			8
#endif /* REASSOCIATION */
/** Private command ID to turn on wlan idle */
#define WLANWLANIDLEON				9
/** Private command ID to turn off wlan idle */
#define WLANWLANIDLEOFF				10

/** Private command ID to get log */
#define WLANGETLOG                  	(WLANIOCTL + 9)

/** Private command ID to set/get configurations */
#define WLAN_SETCONF_GETCONF		(WLANIOCTL + 10)
/** BG scan configuration */
#define BG_SCAN_CONFIG				1
/** BG scan configuration */
#define BG_SCAN_CFG   				3

/** Private command ID to set scan type */
#define WLANSCAN_TYPE			(WLANIOCTL + 11)

/** Private command ID to set a wext address variable */
#define WLAN_SETADDR_GETNONE		(WLANIOCTL + 12)
/** Private command ID to send deauthentication */
#define WLANDEAUTH                  		1

/** Private command ID to set/get 2k */
#define WLAN_SET_GET_2K			(WLANIOCTL + 13)
/** Private command ID to start scan */
#define WLAN_SET_USER_SCAN 			1
/** Private command ID to get the scan table */
#define WLAN_GET_SCAN_TABLE			2
/** Private command ID to set Marvell TLV */
#define WLAN_SET_MRVL_TLV 			3
/** Private command ID to get association response */
#define WLAN_GET_ASSOC_RSP			4
/** Private command ID to request ADDTS */
#define WLAN_ADDTS_REQ				5
/** Private command ID to request DELTS */
#define WLAN_DELTS_REQ				6
/** Private command ID to queue configuration */
#define WLAN_QUEUE_CONFIG			7
/** Private command ID to queue stats */
#define WLAN_QUEUE_STATS			8
/** Private command ID to get CFP table */
#define WLAN_GET_CFP_TABLE			9
/** Private command ID to set/get MEF configuration */
#define WLAN_MEF_CFG				10
/** Private command ID to get memory */
#define WLAN_GET_MEM				11
/** Private command ID to get Tx packet stats */
#define WLAN_TX_PKT_STATS			12

/** Private command ID to set none/get one int */
#define WLAN_SETNONE_GETONEINT		(WLANIOCTL + 15)
/** Private command ID to get region */
#define WLANGETREGION				1
/** Private command ID to get listen interval */
#define WLAN_GET_LISTEN_INTERVAL		2
/** Private command ID to get multiple DTIM */
#define WLAN_GET_MULTIPLE_DTIM			3
/** Private command ID to get Tx rate */
#define WLAN_GET_TX_RATE			4
/** Private command ID to get beacon average */
#define	WLANGETBCNAVG				5
/** Private command ID to get data average */
#define WLANGETDATAAVG				6
/** Private command ID to get current auth type for current connection */
#define WLAN_GETAUTHTYPE			7
/** Private command ID to get current rsn mode for unicast traffic */
#define WLAN_GETRSNMODE                         8
/** Private command ID to get current encryption mode for unicast traffic */
#define WLAN_GETACTCIPHER                       9
/** Private command ID to get current encryption mode for multicast/broadcast traffic */
#define WLAN_GETACTGROUPCIPHER                  10
/** Private command ID to get DTIM */
#define WLANGETDTIM   				11

/** Private command ID to set ten characters and get none */
#define WLAN_SETTENCHAR_GETNONE		(WLANIOCTL + 16)
/** Private command ID to set band */
#define WLAN_SET_BAND               		1
/** Private command ID to set AdHoc channel */
#define WLAN_SET_ADHOC_CH			2
/** Private command ID to set/get SW ann for 11h */
#define WLAN_11H_CHANSWANN			3

/** Private command ID to set none and get ten characters */
#define WLAN_SETNONE_GETTENCHAR		(WLANIOCTL + 17)
/** Private command ID to get band */
#define WLAN_GET_BAND				1
/** Private command ID to get AdHoc channel */
#define WLAN_GET_ADHOC_CH			2

/** Private command ID to set none/get tewlve chars*/
#define WLAN_SETNONE_GETTWELVE_CHAR	(WLANIOCTL + 19)
/** Private command ID to get Rx antenna */
#define WLAN_SUBCMD_GETRXANTENNA		1
/** Private command ID to get Tx antenna */
#define WLAN_SUBCMD_GETTXANTENNA		2
/** Private command ID to get TSF value */
#define WLAN_GET_TSF				3
/** Private command ID for WPS session */
#define WLAN_WPS_SESSION			4

/** Private command ID to set word character and get none */
#define WLAN_SETWORDCHAR_GETNONE	(WLANIOCTL + 20)
/** Private command ID to set AdHoc AES */
#define WLANSETADHOCAES				1

/** Private command ID to set one int/get word char */
#define WLAN_SETONEINT_GETWORDCHAR	(WLANIOCTL + 21)
/** Private command ID to get AdHoc AES key */
#define WLANGETADHOCAES				1
/** Private command ID to get version */
#define WLANVERSION				2
/** Private command ID to get extended version */
#define WLANVEREXT				3

/** Private command ID to set one int/get one int */
#define WLAN_SETONEINT_GETONEINT	(WLANIOCTL + 23)
/** Private command ID to set local power for 11h */
#define WLAN_11H_SETLOCALPOWER			1
/** Private command ID to get WMM QoS information */
#define WLAN_WMM_QOSINFO			2
/** Private command ID to set/get listen interval */
#define	WLAN_LISTENINTRVL			3
/** Private command ID to set/get firmware wakeup method */
#define WLAN_FW_WAKEUP_METHOD			4
/** Private command ID to set/get NULL packet interval */
#define WLAN_NULLPKTINTERVAL			5
/** Private command ID to set/get beacon miss timeout */
#define WLAN_BCN_MISS_TIMEOUT			6
/** Private command ID to set/get AdHoc awake period */
#define WLAN_ADHOC_AWAKE_PERIOD			7
/** Private command ID to set/get LDO */
#define WLAN_LDO				8
/** Private command ID to set/get module type */
#define WLAN_MODULE_TYPE			11
/** Private command ID to enable/disable auto Deep Sleep */
#define WLAN_AUTODEEPSLEEP			12
/** Private command ID to set/get enhance DPS */
#define WLAN_ENHANCEDPS				13
/** Private command ID to wake up MT */
#define WLAN_WAKEUP_MT				14

/** Private command ID to set/get RTS/CTS or CTS to SELF */
#define WLAN_RTS_CTS_CTRL           15

/** Private command ID to set one int/get none */
#define WLAN_SETONEINT_GETNONE		(WLANIOCTL + 24)
/** Private command ID to set Rx antenna */
#define WLAN_SUBCMD_SETRXANTENNA		1
/** Private command ID to set Tx antenna */
#define WLAN_SUBCMD_SETTXANTENNA		2
/** Private command ID to set authentication algorithm */
#define WLANSETAUTHALG				4
/** Private command ID to set encryption mode */
#define WLANSETENCRYPTIONMODE			5
/** Private command ID to set region */
#define WLANSETREGION				6
/** Private command ID to set listen interval */
#define WLAN_SET_LISTEN_INTERVAL		7

/** Private command ID to set multiple DTIM */
#define WLAN_SET_MULTIPLE_DTIM			8

/** Private command ID to set beacon average */
#define WLANSETBCNAVG				9
/** Private command ID to set data average */
#define WLANSETDATAAVG				10
/** Private command ID to associate */
#define WLANASSOCIATE				11

/** Private command ID to set 64-bit char/get 64-bit char */
#define WLAN_SET64CHAR_GET64CHAR	(WLANIOCTL + 25)
/** Private command ID to set/get sleep parameters */
#define WLANSLEEPPARAMS 			2
/** Private command ID to set/get BCA timeshare */
#define	WLAN_BCA_TIMESHARE			3
/** Private command ID to request TPC for 11h */
#define WLAN_11H_REQUESTTPC         		4
/** Private command ID to set power capabilities */
#define WLAN_11H_SETPOWERCAP        		5
/** Private command ID to set/get scan mode */
#define WLANSCAN_MODE				6
/** Private command ID to get AdHoc status */
#define WLAN_GET_ADHOC_STATUS			9
/** Private command ID to set generic IE */
#define WLAN_SET_GEN_IE                 	10
/** Private command ID to get generic IE */
#define WLAN_GET_GEN_IE                 	11
/** Private command ID to request MEAS */
#define WLAN_MEASREQ                    	12
/** Private command ID to get WMM queue status */
#define WLAN_WMM_QUEUE_STATUS			13
/** Private command ID to get Traffic stream status */
#define WLAN_WMM_TS_STATUS			14

/** Private command to scan for a specific ESSID */
#define WLANEXTSCAN			(WLANIOCTL + 26)

/** Private command ID to set/get Deep Sleep mode */
#define WLANDEEPSLEEP			(WLANIOCTL + 27)

/** Private command ID to set/get sixteen int */
#define WLAN_SET_GET_SIXTEEN_INT	(WLANIOCTL + 29)
/** Private command ID to set/get TPC configurations */
#define WLAN_TPCCFG                             1

/** Private command ID to set/get LED GPIO control */
#define WLAN_LED_GPIO_CTRL			5
/** Private command ID to set the number of probe requests per channel */
#define WLAN_SCANPROBES 			6
/** Private command ID to set/get the sleep period */
#define WLAN_SLEEP_PERIOD			7
/** Private command ID to set/get the rate set */
#define	WLAN_ADAPT_RATESET			8
/** Private command ID to get the SNR */
#define WLANSNR					9
/** Private command ID to get the rate */
#define WLAN_GET_RATE				10
/** Private command ID to get Rx information */
#define	WLAN_GET_RXINFO				11
/** Private command ID to set the ATIM window */
#define	WLAN_SET_ATIM_WINDOW			12
/** Private command ID to set/get the beacon interval */
#define WLAN_BEACON_INTERVAL			13
/** Private command ID to set/get SDIO pull */
#define WLAN_SDIO_PULL_CTRL			14
/** Private command ID to set/get the scan time */
#define WLAN_SCAN_TIME				15
/** Private command ID to set/get ECL system clock */
#define WLAN_ECL_SYS_CLOCK			16
/** Private command ID to set/get the Tx control */
#define WLAN_TXCONTROL				18
/** Private command ID to set/get Host Sleep configuration */
#define WLANHSCFG				20
/** Private command ID to set Host Sleep parameters */
#define WLANHSSETPARA				21
/** Private command ID to set Inactivity timeout ext */
#define	WLAN_INACTIVITY_TIMEOUT_EXT	 	22
/** Private command ID for debug configuration */
#define WLANDBGSCFG				23
#ifdef DEBUG_LEVEL1
/** Private command ID to set/get driver debug */
#define WLAN_DRV_DBG				24
#endif
/** Private command ID to set/get the max packet delay passed from drv to fw */
#define WLAN_DRV_DELAY_MAX			26
/** Private command ID for interface control */
#define WLAN_FUNC_IF_CTRL			27
/** Private command ID to set quiet IE */
#define WLAN_11H_SET_QUIET_IE			28

/** Private command ID to read/write Command 52 */
#define WLANCMD52RDWR			(WLANIOCTL + 30)
/** Private command ID to read/write Command 53 */
#define WLANCMD53RDWR			(WLANIOCTL + 31)

/**
 * iwpriv ioctl handlers
 */
static const struct iw_priv_args wlan_private_args[] = {
    /* 
     * { cmd, set_args, get_args, name } 
     */
    {
     WLANEXTSCAN,
     IW_PRIV_TYPE_INT,
     IW_PRIV_TYPE_CHAR | 2,
     "extscan"},
    {
     WLANHOSTCMD,
     IW_PRIV_TYPE_BYTE | 2047,
     IW_PRIV_TYPE_BYTE | 2047,
     "hostcmd"},
    {
     WLANARPFILTER,
     IW_PRIV_TYPE_BYTE | 2047,
     IW_PRIV_TYPE_BYTE | 2047,
     "arpfilter"},
    {
     WLANREGRDWR,
     IW_PRIV_TYPE_CHAR | 256,
     IW_PRIV_TYPE_CHAR | 256,
     "regrdwr"},
    {
     WLAN_SET_GET_256_CHAR,
     IW_PRIV_TYPE_CHAR | 256,
     IW_PRIV_TYPE_CHAR | 256,
     ""},
    {
     WLANCMD52RDWR,
     IW_PRIV_TYPE_BYTE | 7,
     IW_PRIV_TYPE_BYTE | 7,
     "sdcmd52rw"},
    {
     WLANCMD53RDWR,
     IW_PRIV_TYPE_CHAR | CMD53BUFLEN,
     IW_PRIV_TYPE_CHAR | CMD53BUFLEN,
     "sdcmd53rw"},
    {
     WLAN_SETCONF_GETCONF,
     IW_PRIV_TYPE_BYTE | MAX_SETGET_CONF_SIZE,
     IW_PRIV_TYPE_BYTE | MAX_SETGET_CONF_SIZE,
     "setgetconf"},
    {
     WLANCISDUMP,
     IW_PRIV_TYPE_NONE,
     IW_PRIV_TYPE_BYTE | 512,
     "getcis"},
    {
     WLANSCAN_TYPE,
     IW_PRIV_TYPE_CHAR | 8,
     IW_PRIV_TYPE_CHAR | 8,
     "scantype"},
    {
     WLAN_SETADDR_GETNONE,
     IW_PRIV_TYPE_ADDR | 1,
     IW_PRIV_TYPE_NONE,
     ""},
    {
     WLANDEAUTH,
     IW_PRIV_TYPE_ADDR | 1,
     IW_PRIV_TYPE_NONE,
     "deauth"},
    {
     WLAN_SETINT_GETINT,
     IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1,
     IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1,
     ""},
    {
     WLANNF,
     IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1,
     IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1,
     "getNF"},
    {
     WLANRSSI,
     IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1,
     IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1,
     "getRSSI"},
    {
     WLANBGSCAN,
     IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1,
     IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1,
     "bgscan"},
    {
     WLANENABLE11D,
     IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1,
     IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1,
     "enable11d"},
    {
     WLANADHOCGRATE,
     IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1,
     IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1,
     "adhocgrate"},
    {
     WLANSDIOCLOCK,
     IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1,
     IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1,
     "sdioclock"},
    {
     WLANWMM_ENABLE,
     IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1,
     IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1,
     "wmm"},
    {
     WLANNULLGEN,
     IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1,
     IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1,
     "uapsdnullgen"},
    {
     WLANADHOCCSET,
     IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1,
     IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1,
     "setcoalescing"},
    {
     WLAN_ADHOC_G_PROT,
     IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1,
     IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1,
     "adhocgprot"},
    {
     WLAN_SETONEINT_GETONEINT,
     IW_PRIV_TYPE_INT | 1,
     IW_PRIV_TYPE_INT | 1,
     ""},
    {
     WLAN_11H_SETLOCALPOWER,
     IW_PRIV_TYPE_INT | 1,
     IW_PRIV_TYPE_INT | 1,
     "setpowercons"},
    {
     WLAN_WMM_QOSINFO,
     IW_PRIV_TYPE_INT | 1,
     IW_PRIV_TYPE_INT | 1,
     "wmm_qosinfo"},
    {
     WLAN_LISTENINTRVL,
     IW_PRIV_TYPE_INT | 1,
     IW_PRIV_TYPE_INT | 1,
     "lolisteninter"},
    {
     WLAN_FW_WAKEUP_METHOD,
     IW_PRIV_TYPE_INT | 1,
     IW_PRIV_TYPE_INT | 1,
     "fwwakeupmethod"},
    {
     WLAN_NULLPKTINTERVAL,
     IW_PRIV_TYPE_INT | 1,
     IW_PRIV_TYPE_INT | 1,
     "psnullinterval"},
    {
     WLAN_BCN_MISS_TIMEOUT,
     IW_PRIV_TYPE_INT | 1,
     IW_PRIV_TYPE_INT | 1,
     "bcnmisto"},
    {
     WLAN_ADHOC_AWAKE_PERIOD,
     IW_PRIV_TYPE_INT | 1,
     IW_PRIV_TYPE_INT | 1,
     "adhocawakepd"},
    {
     WLAN_LDO,
     IW_PRIV_TYPE_INT | 1,
     IW_PRIV_TYPE_INT | 1,
     "ldocfg"},
    {
     WLAN_RTS_CTS_CTRL,
     IW_PRIV_TYPE_INT | 1,
     IW_PRIV_TYPE_INT | 1,
     "rtsctsctrl"},
    {
     WLAN_MODULE_TYPE,
     IW_PRIV_TYPE_INT | 1,
     IW_PRIV_TYPE_INT | 1,
     "moduletype"},
    {
     WLAN_AUTODEEPSLEEP,
     IW_PRIV_TYPE_INT | 1,
     IW_PRIV_TYPE_INT | 1,
     "autodeepsleep"},
    {
     WLAN_ENHANCEDPS,
     IW_PRIV_TYPE_INT | 1,
     IW_PRIV_TYPE_INT | 1,
     "enhanceps"},
    {
     WLAN_WAKEUP_MT,
     IW_PRIV_TYPE_INT | 1,
     IW_PRIV_TYPE_INT | 1,
     "wakeupmt"},

    /* Using iwpriv sub-command feature */
    {
     WLAN_SETONEINT_GETNONE,
     IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1,
     IW_PRIV_TYPE_NONE,
     ""},
    {
     WLAN_SUBCMD_SETRXANTENNA,
     IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1,
     IW_PRIV_TYPE_NONE,
     "setrxant"},
    {
     WLAN_SUBCMD_SETTXANTENNA,
     IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1,
     IW_PRIV_TYPE_NONE,
     "settxant"},
    {
     WLANSETAUTHALG,
     IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1,
     IW_PRIV_TYPE_NONE,
     "authalgs",
     },
    {
     WLANSETENCRYPTIONMODE,
     IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1,
     IW_PRIV_TYPE_NONE,
     "encryptionmode",
     },
    {
     WLANSETREGION,
     IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1,
     IW_PRIV_TYPE_NONE,
     "setregioncode"},
    {
     WLAN_SET_LISTEN_INTERVAL,
     IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1,
     IW_PRIV_TYPE_NONE,
     "setlisteninter"},
    {
     WLAN_SET_MULTIPLE_DTIM,
     IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1,
     IW_PRIV_TYPE_NONE,
     "setmultipledtim"},
    {
     WLANSETBCNAVG,
     IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1,
     IW_PRIV_TYPE_NONE,
     "setbcnavg"},
    {
     WLANSETDATAAVG,
     IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1,
     IW_PRIV_TYPE_NONE,
     "setdataavg"},
    {
     WLANASSOCIATE,
     IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1,
     IW_PRIV_TYPE_NONE,
     "associate"},
    {
     WLAN_SETNONE_GETONEINT,
     IW_PRIV_TYPE_NONE,
     IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1,
     ""},
    {
     WLANGETREGION,
     IW_PRIV_TYPE_NONE,
     IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1,
     "getregioncode"},
    {
     WLAN_GET_LISTEN_INTERVAL,
     IW_PRIV_TYPE_NONE,
     IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1,
     "getlisteninter"},
    {
     WLAN_GET_MULTIPLE_DTIM,
     IW_PRIV_TYPE_NONE,
     IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1,
     "getmultipledtim"},
    {
     WLAN_GET_TX_RATE,
     IW_PRIV_TYPE_NONE,
     IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1,
     "gettxrate"},
    {
     WLANGETBCNAVG,
     IW_PRIV_TYPE_NONE,
     IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1,
     "getbcnavg"},
    {
     WLANGETDATAAVG,
     IW_PRIV_TYPE_NONE,
     IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1,
     "getdataavg"},
    {
     WLAN_GETAUTHTYPE,
     IW_PRIV_TYPE_NONE,
     IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1,
     "getauthtype"},
    {
     WLAN_GETRSNMODE,
     IW_PRIV_TYPE_NONE,
     IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1,
     "getrsnmode"},
    {
     WLAN_GETACTCIPHER,
     IW_PRIV_TYPE_NONE,
     IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1,
     "act_paircipher"},
    {
     WLAN_GETACTGROUPCIPHER,
     IW_PRIV_TYPE_NONE,
     IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1,
     "act_groupcipher"},
    {
     WLANGETDTIM,
     IW_PRIV_TYPE_NONE,
     IW_PRIV_TYPE_INT | IW_PRIV_SIZE_FIXED | 1,
     "getdtim"},
    {
     WLAN_SETNONE_GETTWELVE_CHAR,
     IW_PRIV_TYPE_NONE,
     IW_PRIV_TYPE_CHAR | 12,
     ""},
    {
     WLAN_SUBCMD_GETRXANTENNA,
     IW_PRIV_TYPE_NONE,
     IW_PRIV_TYPE_CHAR | 12,
     "getrxant"},
    {
     WLAN_SUBCMD_GETTXANTENNA,
     IW_PRIV_TYPE_NONE,
     IW_PRIV_TYPE_CHAR | 12,
     "gettxant"},
    {
     WLAN_GET_TSF,
     IW_PRIV_TYPE_NONE,
     IW_PRIV_TYPE_CHAR | 12,
     "gettsf"},
    {
     WLAN_WPS_SESSION,
     IW_PRIV_TYPE_NONE,
     IW_PRIV_TYPE_CHAR | 12,
     "wpssession"},
    {
     WLANDEEPSLEEP,
     IW_PRIV_TYPE_CHAR | 1,
     IW_PRIV_TYPE_CHAR | 6,
     "deepsleep"},
    {
     WLAN_SETNONE_GETNONE,
     IW_PRIV_TYPE_NONE,
     IW_PRIV_TYPE_NONE,
     ""},
    {
     WLANADHOCSTOP,
     IW_PRIV_TYPE_NONE,
     IW_PRIV_TYPE_NONE,
     "adhocstop"},
    {
     WLANRADIOON,
     IW_PRIV_TYPE_NONE,
     IW_PRIV_TYPE_NONE,
     "radioon"},
    {
     WLANRADIOOFF,
     IW_PRIV_TYPE_NONE,
     IW_PRIV_TYPE_NONE,
     "radiooff"},
    {
     WLANREMOVEADHOCAES,
     IW_PRIV_TYPE_NONE,
     IW_PRIV_TYPE_NONE,
     "rmaeskey"},
    {
     WLANCRYPTOTEST,
     IW_PRIV_TYPE_NONE,
     IW_PRIV_TYPE_NONE,
     "crypto_test"},
#ifdef REASSOCIATION
    {
     WLANREASSOCIATIONAUTO,
     IW_PRIV_TYPE_NONE,
     IW_PRIV_TYPE_NONE,
     "reasso-on"},
    {
     WLANREASSOCIATIONUSER,
     IW_PRIV_TYPE_NONE,
     IW_PRIV_TYPE_NONE,
     "reasso-off"},
#endif /* REASSOCIATION */
    {
     WLANWLANIDLEON,
     IW_PRIV_TYPE_NONE,
     IW_PRIV_TYPE_NONE,
     "wlanidle-on"},
    {
     WLANWLANIDLEOFF,
     IW_PRIV_TYPE_NONE,
     IW_PRIV_TYPE_NONE,
     "wlanidle-off"},
    {
     WLAN_SET64CHAR_GET64CHAR,
     IW_PRIV_TYPE_CHAR | 64,
     IW_PRIV_TYPE_CHAR | 64,
     ""},
    {
     WLANSLEEPPARAMS,
     IW_PRIV_TYPE_CHAR | 64,
     IW_PRIV_TYPE_CHAR | 64,
     "sleepparams"},

    {
     WLAN_11H_REQUESTTPC,
     IW_PRIV_TYPE_CHAR | 64,
     IW_PRIV_TYPE_CHAR | 64,
     "requesttpc"},
    {
     WLAN_11H_SETPOWERCAP,
     IW_PRIV_TYPE_CHAR | 64,
     IW_PRIV_TYPE_CHAR | 64,
     "powercap"},
    {
     WLAN_MEASREQ,
     IW_PRIV_TYPE_CHAR | 64,
     IW_PRIV_TYPE_CHAR | 64,
     "measreq"},
    {
     WLAN_BCA_TIMESHARE,
     IW_PRIV_TYPE_CHAR | 64,
     IW_PRIV_TYPE_CHAR | 64,
     "bca-ts"},
    {
     WLANSCAN_MODE,
     IW_PRIV_TYPE_CHAR | 64,
     IW_PRIV_TYPE_CHAR | 64,
     "scanmode"},
    {
     WLAN_GET_ADHOC_STATUS,
     IW_PRIV_TYPE_CHAR | 64,
     IW_PRIV_TYPE_CHAR | 64,
     "getadhocstatus"},
    {
     WLAN_SET_GEN_IE,
     IW_PRIV_TYPE_CHAR | 64,
     IW_PRIV_TYPE_CHAR | 64,
     "setgenie"},
    {
     WLAN_GET_GEN_IE,
     IW_PRIV_TYPE_CHAR | 64,
     IW_PRIV_TYPE_CHAR | 64,
     "getgenie"},
    {
     WLAN_WMM_QUEUE_STATUS,
     IW_PRIV_TYPE_CHAR | 64,
     IW_PRIV_TYPE_CHAR | 64,
     "qstatus"},
    {
     WLAN_WMM_TS_STATUS,
     IW_PRIV_TYPE_CHAR | 64,
     IW_PRIV_TYPE_CHAR | 64,
     "ts_status"},
    {
     WLAN_SETWORDCHAR_GETNONE,
     IW_PRIV_TYPE_CHAR | 32,
     IW_PRIV_TYPE_NONE,
     ""},
    {
     WLANSETADHOCAES,
     IW_PRIV_TYPE_CHAR | 32,
     IW_PRIV_TYPE_NONE,
     "setaeskey"},
    {
     WLAN_SETONEINT_GETWORDCHAR,
     IW_PRIV_TYPE_INT | 1,
     IW_PRIV_TYPE_CHAR | 128,
     ""},
    {
     WLANGETADHOCAES,
     IW_PRIV_TYPE_INT | 1,
     IW_PRIV_TYPE_CHAR | 128,
     "getaeskey"},
    {
     WLANVERSION,
     IW_PRIV_TYPE_INT | 1,
     IW_PRIV_TYPE_CHAR | 128,
     "version"},
    {
     WLANVEREXT,
     IW_PRIV_TYPE_INT | 1,
     IW_PRIV_TYPE_CHAR | 128,
     "verext"},
    {
     WLANSETWPAIE,
     IW_PRIV_TYPE_CHAR | IW_PRIV_SIZE_FIXED | 24,
     IW_PRIV_TYPE_NONE,
     "setwpaie"},
    {
     WLAN_SETTENCHAR_GETNONE,
     IW_PRIV_TYPE_CHAR | 10,
     IW_PRIV_TYPE_NONE,
     ""},
    {
     WLAN_SET_BAND,
     IW_PRIV_TYPE_CHAR | 10,
     IW_PRIV_TYPE_NONE,
     "setband"},
    {
     WLAN_SET_ADHOC_CH,
     IW_PRIV_TYPE_CHAR | 10,
     IW_PRIV_TYPE_NONE,
     "setadhocch"},
    {
     WLAN_11H_CHANSWANN,
     IW_PRIV_TYPE_CHAR | 10,
     IW_PRIV_TYPE_NONE,
     "chanswann"},
    {
     WLAN_SETNONE_GETTENCHAR,
     IW_PRIV_TYPE_NONE,
     IW_PRIV_TYPE_CHAR | 10,
     ""},
    {
     WLAN_GET_BAND,
     IW_PRIV_TYPE_NONE,
     IW_PRIV_TYPE_CHAR | 10,
     "getband"},
    {
     WLAN_GET_ADHOC_CH,
     IW_PRIV_TYPE_NONE,
     IW_PRIV_TYPE_CHAR | 10,
     "getadhocch"},
    {
     WLANGETLOG,
     IW_PRIV_TYPE_NONE,
     IW_PRIV_TYPE_CHAR | GETLOG_BUFSIZE,
     "getlog"},
    {
     WLAN_SET_GET_SIXTEEN_INT,
     IW_PRIV_TYPE_INT | 16,
     IW_PRIV_TYPE_INT | 16,
     ""},
    {
     WLAN_TPCCFG,
     IW_PRIV_TYPE_INT | 16,
     IW_PRIV_TYPE_INT | 16,
     "tpccfg"},
    {
     WLAN_SCANPROBES,
     IW_PRIV_TYPE_INT | 16,
     IW_PRIV_TYPE_INT | 16,
     "scanprobes"},
    {
     WLAN_LED_GPIO_CTRL,
     IW_PRIV_TYPE_INT | 16,
     IW_PRIV_TYPE_INT | 16,
     "ledgpio"},
    {
     WLAN_SLEEP_PERIOD,
     IW_PRIV_TYPE_INT | 16,
     IW_PRIV_TYPE_INT | 16,
     "sleeppd"},
    {
     WLAN_ADAPT_RATESET,
     IW_PRIV_TYPE_INT | 16,
     IW_PRIV_TYPE_INT | 16,
     "rateadapt"},
    {
     WLANSNR,
     IW_PRIV_TYPE_INT | 16,
     IW_PRIV_TYPE_INT | 16,
     "getSNR"},
    {
     WLAN_GET_RATE,
     IW_PRIV_TYPE_INT | 16,
     IW_PRIV_TYPE_INT | 16,
     "getrate"},
    {
     WLAN_GET_RXINFO,
     IW_PRIV_TYPE_INT | 16,
     IW_PRIV_TYPE_INT | 16,
     "getrxinfo"},
    {
     WLAN_SET_ATIM_WINDOW,
     IW_PRIV_TYPE_INT | 16,
     IW_PRIV_TYPE_INT | 16,
     "atimwindow"},
    {
     WLAN_BEACON_INTERVAL,
     IW_PRIV_TYPE_INT | 16,
     IW_PRIV_TYPE_INT | 16,
     "bcninterval"},
    {
     WLAN_SDIO_PULL_CTRL,
     IW_PRIV_TYPE_INT | 16,
     IW_PRIV_TYPE_INT | 16,
     "sdiopullctrl"},
    {
     WLAN_SCAN_TIME,
     IW_PRIV_TYPE_INT | 16,
     IW_PRIV_TYPE_INT | 16,
     "scantime"},
    {
     WLAN_ECL_SYS_CLOCK,
     IW_PRIV_TYPE_INT | 16,
     IW_PRIV_TYPE_INT | 16,
     "sysclock"},
    {
     WLAN_TXCONTROL,
     IW_PRIV_TYPE_INT | 16,
     IW_PRIV_TYPE_INT | 16,
     "txcontrol"},
    {
     WLANHSCFG,
     IW_PRIV_TYPE_INT | 16,
     IW_PRIV_TYPE_INT | 16,
     "hscfg"},
    {
     WLANHSSETPARA,
     IW_PRIV_TYPE_INT | 16,
     IW_PRIV_TYPE_INT | 16,
     "hssetpara"},
    {
     WLAN_INACTIVITY_TIMEOUT_EXT,
     IW_PRIV_TYPE_INT | 16,
     IW_PRIV_TYPE_INT | 16,
     "inactoext"},
    {
     WLANDBGSCFG,
     IW_PRIV_TYPE_INT | 16,
     IW_PRIV_TYPE_INT | 16,
     "dbgscfg"},
#ifdef DEBUG_LEVEL1
    {
     WLAN_DRV_DBG,
     IW_PRIV_TYPE_INT | 16,
     IW_PRIV_TYPE_INT | 16,
     "drvdbg"},
#endif
    {
     WLAN_DRV_DELAY_MAX,
     IW_PRIV_TYPE_INT | 16,
     IW_PRIV_TYPE_INT | 16,
     "drvdelaymax"},
    {
     WLAN_FUNC_IF_CTRL,
     IW_PRIV_TYPE_INT | 16,
     IW_PRIV_TYPE_INT | 16,
     "intfctrl"},
    {
     WLAN_11H_SET_QUIET_IE,
     IW_PRIV_TYPE_INT | 16,
     IW_PRIV_TYPE_INT | 16,
     "setquietie"},
    {
     WLAN_SET_GET_2K,
     IW_PRIV_TYPE_BYTE | 2000,
     IW_PRIV_TYPE_BYTE | 2000,
     ""},
    {
     WLAN_SET_USER_SCAN,
     IW_PRIV_TYPE_BYTE | 2000,
     IW_PRIV_TYPE_BYTE | 2000,
     "setuserscan"},
    {
     WLAN_GET_SCAN_TABLE,
     IW_PRIV_TYPE_BYTE | 2000,
     IW_PRIV_TYPE_BYTE | 2000,
     "getscantable"},
    {
     WLAN_SET_MRVL_TLV,
     IW_PRIV_TYPE_BYTE | 2000,
     IW_PRIV_TYPE_BYTE | 2000,
     "setmrvltlv"},
    {
     WLAN_GET_ASSOC_RSP,
     IW_PRIV_TYPE_BYTE | 2000,
     IW_PRIV_TYPE_BYTE | 2000,
     "getassocrsp"},
    {
     WLAN_ADDTS_REQ,
     IW_PRIV_TYPE_BYTE | 2000,
     IW_PRIV_TYPE_BYTE | 2000,
     "addts"},
    {
     WLAN_DELTS_REQ,
     IW_PRIV_TYPE_BYTE | 2000,
     IW_PRIV_TYPE_BYTE | 2000,
     "delts"},
    {
     WLAN_QUEUE_CONFIG,
     IW_PRIV_TYPE_BYTE | 2000,
     IW_PRIV_TYPE_BYTE | 2000,
     "qconfig"},
    {
     WLAN_QUEUE_STATS,
     IW_PRIV_TYPE_BYTE | 2000,
     IW_PRIV_TYPE_BYTE | 2000,
     "qstats"},
    {
     WLAN_TX_PKT_STATS,
     IW_PRIV_TYPE_BYTE | 2000,
     IW_PRIV_TYPE_BYTE | 2000,
     "txpktstats"},
    {
     WLAN_GET_CFP_TABLE,
     IW_PRIV_TYPE_BYTE | 2000,
     IW_PRIV_TYPE_BYTE | 2000,
     "getcfptable"},
    {
     WLAN_MEF_CFG,
     IW_PRIV_TYPE_BYTE | 2000,
     IW_PRIV_TYPE_BYTE | 2000,
     "mefcfg"},
    {
     WLAN_GET_MEM,
     IW_PRIV_TYPE_BYTE | 2000,
     IW_PRIV_TYPE_BYTE | 2000,
     "getmem"},
};

#endif /* _WLAN_PRIV_H_ */
