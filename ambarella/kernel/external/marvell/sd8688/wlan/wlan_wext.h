/** @file wlan_wext.h
 * @brief This file contains definition for IOCTL call.
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
	10/11/05: Add Doxygen format comments
	12/19/05: Correct a typo in structure _wlan_ioctl_wmm_tspec
	01/11/06: Conditionalize new scan/join ioctls
	04/10/06: Add hostcmd generic API
	04/18/06: Remove old Subscribe Event and add new Subscribe Event
	          implementation through generic hostcmd API
	06/08/06: Add definitions of custom events
	08/29/06: Add ledgpio private command
	12/24/07: Move private ioctl IDs to wlan_priv.h
********************************************************/

#ifndef	_WLAN_WEXT_H_
#define	_WLAN_WEXT_H_

/** Offset for subcommand */
#define SUBCMD_OFFSET				4

/** Maximum EEPROM data */
#define MAX_EEPROM_DATA     			256

/** Get log buffer size */
#define GETLOG_BUFSIZE				512

/** Firmware wakeup method : Unchanged */
#define WAKEUP_FW_UNCHANGED			0
/** Firmware wakeup method : Through interface */
#define WAKEUP_FW_THRU_INTERFACE		1
/** Firmware wakeup method : Through GPIO*/
#define WAKEUP_FW_THRU_GPIO			2

/** Deep Sleep enable flag */
#define DEEP_SLEEP_ENABLE			1
/** Deep Sleep disable flag */
#define DEEP_SLEEP_DISABLE  			0

/** Command 53 buffer length */
#define CMD53BUFLEN				512

/** MAC register */
#define	REG_MAC					0x19
/** BBP register */
#define	REG_BBP					0x1a
/** RF register */
#define	REG_RF					0x1b
/** PMIC register */
#define	REG_PMIC				0xad
/** EEPROM register */
#define	REG_EEPROM				0x59

/** Command disabled */
#define	CMD_DISABLED				0
/** Command enabled */
#define	CMD_ENABLED				1
/** Command get */
#define	CMD_GET					2
/** Skip command number */
#define SKIP_CMDNUM				4
/** Skip type */
#define SKIP_TYPE				1
/** Skip size */
#define SKIP_SIZE				2
/** Skip action */
#define SKIP_ACTION				2
/** Skip type and size */
#define SKIP_TYPE_SIZE			(SKIP_TYPE + SKIP_SIZE)
/** Skip type and action */
#define SKIP_TYPE_ACTION		(SKIP_TYPE + SKIP_ACTION)

/** Maximum size of set/get configurations */
#define MAX_SETGET_CONF_SIZE		2000    /* less than
                                                   MRVDRV_SIZE_OF_CMD_BUFFER */
/** Maximum length of set/get configuration commands */
#define MAX_SETGET_CONF_CMD_LEN		(MAX_SETGET_CONF_SIZE - SKIP_CMDNUM)

/* define custom events */
/** Custom event : Host Sleep activated */
#define CUS_EVT_HS_ACTIVATED		"HS_ACTIVATED "
/** Custom event : Host Sleep deactivated */
#define CUS_EVT_HS_DEACTIVATED		"HS_DEACTIVATED "
/** Custom event : Host Sleep wakeup */
#define CUS_EVT_HS_WAKEUP		"HS_WAKEUP"
/** Custom event : Beacon RSSI low */
#define CUS_EVT_BEACON_RSSI_LOW		"EVENT=BEACON_RSSI_LOW"
/** Custom event : Beacon SNR low */
#define CUS_EVT_BEACON_SNR_LOW		"EVENT=BEACON_SNR_LOW"
/** Custom event : Beacon RSSI high */
#define CUS_EVT_BEACON_RSSI_HIGH	"EVENT=BEACON_RSSI_HIGH"
/** Custom event : Beacon SNR high */
#define CUS_EVT_BEACON_SNR_HIGH		"EVENT=BEACON_SNR_HIGH"
/** Custom event : Max fail */
#define CUS_EVT_MAX_FAIL		"EVENT=MAX_FAIL"
/** Custom event : MIC failure, unicast */
#define CUS_EVT_MLME_MIC_ERR_UNI	"MLME-MICHAELMICFAILURE.indication unicast "
/** Custom event : MIC failure, multicast */
#define CUS_EVT_MLME_MIC_ERR_MUL	"MLME-MICHAELMICFAILURE.indication multicast "

/** Custom event : Data RSSI low */
#define CUS_EVT_DATA_RSSI_LOW		"EVENT=DATA_RSSI_LOW"
/** Custom event : Data SNR low */
#define CUS_EVT_DATA_SNR_LOW		"EVENT=DATA_SNR_LOW"
/** Custom event : Data RSSI high */
#define CUS_EVT_DATA_RSSI_HIGH		"EVENT=DATA_RSSI_HIGH"
/** Custom event : Data SNR high */
#define CUS_EVT_DATA_SNR_HIGH		"EVENT=DATA_SNR_HIGH"
/** Custom event : Link Quality */
#define CUS_EVT_LINK_QUALITY		"EVENT=LINK_QUALITY"
/** Custom event : Pre-Beacon Lost */
#define CUS_EVT_PRE_BEACON_LOST		"EVENT=PRE_BEACON_LOST"

/** Custom event : Deep Sleep awake */
#define CUS_EVT_DEEP_SLEEP_AWAKE	"EVENT=DS_AWAKE"

/** Custom event : AdHoc link sensed */
#define CUS_EVT_ADHOC_LINK_SENSED	"EVENT=ADHOC_LINK_SENSED"
/** Custom event : AdHoc beacon lost */
#define CUS_EVT_ADHOC_BCN_LOST		"EVENT=ADHOC_BCN_LOST"

/** wlan_ioctl */
typedef struct _wlan_ioctl
{
        /** Command ID */
    u16 command;
        /** data length */
    u16 len;
        /** data pointer */
    u8 *data;
} wlan_ioctl;

/** wlan_ioctl_rfantenna */
typedef struct _wlan_ioctl_rfantenna
{
    /** Action */
    u16 Action;
    /** RF Antenna mode */
    u16 AntennaMode;
} wlan_ioctl_rfantenna;

/** wlan_ioctl_regrdwr */
typedef struct _wlan_ioctl_regrdwr
{
        /** Which register to access */
    u16 WhichReg;
        /** Read or Write */
    u16 Action;
    /** Register offset */
    u32 Offset;
    /** NOB */
    u16 NOB;
    /** Value */
    u32 Value;
} wlan_ioctl_regrdwr;

/** wlan_ioctl_cfregrdwr */
typedef struct _wlan_ioctl_cfregrdwr
{
        /** Read or Write */
    u8 Action;
        /** register address */
    u16 Offset;
        /** register value */
    u16 Value;
} wlan_ioctl_cfregrdwr;

/** wlan_ioctl_adhoc_key_info */
typedef struct _wlan_ioctl_adhoc_key_info
{
    /** Action */
    u16 action;
    /** AdHoc key */
    u8 key[16];
    /** TKIP Tx MIC */
    u8 tkiptxmickey[16];
    /** TKIP Rx MIC */
    u8 tkiprxmickey[16];
} wlan_ioctl_adhoc_key_info;

/** sleep_params */
typedef struct _wlan_ioctl_sleep_params_config
{
    /** Action */
    u16 Action;
    /** Error */
    u16 Error;
    /** Offset */
    u16 Offset;
    /** Stable time */
    u16 StableTime;
    /** Calibration control */
    u8 CalControl;
    /** External sleep clock */
    u8 ExtSleepClk;
    /** Reserved */
    u16 Reserved;
} __ATTRIB_PACK__ wlan_ioctl_sleep_params_config,
    *pwlan_ioctl_sleep_params_config;

/** BCA TIME SHARE */
typedef struct _wlan_ioctl_bca_timeshare_config
{
        /** ACT_GET/ACT_SET */
    u16 Action;
        /** Type: WLAN, BT */
    u16 TrafficType;
        /** Interval: 20msec - 60000msec */
    u32 TimeShareInterval;
        /** PTA arbiter time in msec */
    u32 BTTime;
} __ATTRIB_PACK__ wlan_ioctl_bca_timeshare_config,
    *pwlan_ioctl_bca_timeshare_config;

/** Maximum number of CFPs in the list */
#define MAX_CFP_LIST_NUM	64

/** wlan_ioctl_cfp_table */
typedef struct _wlan_ioctl_cfp_table
{
    /** Region */
    u32 region;
    /** CFP number */
    u32 cfp_no;
    struct
    {
        /** CFP channel */
        u16 Channel;
        /** CFP frequency */
        u32 Freq;
        /** Maximum Tx power */
        u16 MaxTxPower;
        /** Unsupported flag */
        u8 Unsupported;
    } cfp[MAX_CFP_LIST_NUM];
} __ATTRIB_PACK__ wlan_ioctl_cfp_table, *pwlan_ioctl_cfp_table;

#endif /* _WLAN_WEXT_H_ */
