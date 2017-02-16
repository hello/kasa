/** @file wlan_fw.h
 * 
 * @brief This file contains firmware specific defines. 
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
	05/30/07: Initial creation
********************************************************/

#ifndef _WLAN_FW_H
#define _WLAN_FW_H

/** Macros for Data Alignment : size */
#define ALIGN_SZ(p, a)	\
	(((p) + ((a) - 1)) & ~((a) - 1))

/** Macros for Data Alignment : address */
#define ALIGN_ADDR(p, a)	\
	((((u32)(p)) + (((u32)(a)) - 1)) & ~(((u32)(a)) - 1))

/** The first valid channel for use */
#define FIRST_VALID_CHANNEL	0xff
/** PUBLIC DEFINITIONS */
/** Default Ad-Hoc channel */
#define DEFAULT_AD_HOC_CHANNEL       6
/** Default Ad-Hoc channel A */
#define DEFAULT_AD_HOC_CHANNEL_A    36

/** Length of SNAP header */
#define MRVDRV_SNAP_HEADER_LEN          8

/** This is for firmware specific length */
#define EXTRA_LEN	36

#ifdef PXA3XX_DMA_ALIGN
/** DMA alignment value for PXA3XX platforms */
#define PXA3XX_DMA_ALIGNMENT	8
#endif /* PXA3XX_DMA_ALIGN */

/** Buffer size for ethernet Tx packets */
#define MRVDRV_ETH_TX_PACKET_BUFFER_SIZE \
	(ETH_FRAME_LEN + sizeof(TxPD) + EXTRA_LEN)

/** Buffer size for ethernet Rx packets */
#define MRVDRV_ETH_RX_PACKET_BUFFER_SIZE \
	(ETH_FRAME_LEN + sizeof(RxPD) \
	 + MRVDRV_SNAP_HEADER_LEN + EXTRA_LEN)

/** Host Command option for wait for RSP */
#define HostCmd_OPTION_WAITFORRSP             0x0002
/** Host Command option for wait for RSP Timeout */
#define HostCmd_OPTION_TIMEOUT	              0x0004

/** Host Command ID : Get hardware specifications */
#define HostCmd_CMD_GET_HW_SPEC               0x0003
/** Host Command ID : 802.11 scan */
#define HostCmd_CMD_802_11_SCAN               0x0006
/** Host Command ID : 802.11 get log */
#define HostCmd_CMD_802_11_GET_LOG            0x000b
/** Host Command ID : MAC multicast address */
#define HostCmd_CMD_MAC_MULTICAST_ADR         0x0010
/** Host Command ID : 802.11 EEPROM access */
#define HostCmd_CMD_802_11_EEPROM_ACCESS      0x0059
/** Host Command ID : 802.11 associate */
#define HostCmd_CMD_802_11_ASSOCIATE          0x0012
/** Host Command ID : 802.11 set WEP */
#define HostCmd_CMD_802_11_SET_WEP            0x0013
/** Host Command ID : 802.11 SNMP MIB */
#define HostCmd_CMD_802_11_SNMP_MIB           0x0016
/** Host Command ID : MAC register access */
#define HostCmd_CMD_MAC_REG_ACCESS            0x0019
/** Host Command ID : BBP register access */
#define HostCmd_CMD_BBP_REG_ACCESS            0x001a
/** Host Command ID : RF register access */
#define HostCmd_CMD_RF_REG_ACCESS             0x001b
/** Host Command ID : PMIC register access */
#define HostCmd_CMD_PMIC_REG_ACCESS      	  0x00ad
/** Host Command ID : 802.11 radio control */
#define HostCmd_CMD_802_11_RADIO_CONTROL      0x001c
/** Host Command ID : 802.11 RF channel */
#define HostCmd_CMD_802_11_RF_CHANNEL         0x001d
/** Host Command ID : 802.11 RF Tx power */
#define HostCmd_CMD_802_11_RF_TX_POWER        0x001e
/** Host Command ID : 802.11 RF antenna */
#define HostCmd_CMD_802_11_RF_ANTENNA         0x0020

/** Host Command ID : 802.11 Power Save mode */
#define HostCmd_CMD_802_11_PS_MODE	      0x0021

/** Host Command ID : 802.11 deauthenticate */
#define HostCmd_CMD_802_11_DEAUTHENTICATE     0x0024
/** Host Command ID : MAC control */
#define HostCmd_CMD_MAC_CONTROL               0x0028
/** Host Command ID : 802.11 Ad-Hoc start */
#define HostCmd_CMD_802_11_AD_HOC_START       0x002b
/** Host Command ID : 802.11 Ad-Hoc join */
#define HostCmd_CMD_802_11_AD_HOC_JOIN        0x002c

/** Host Command ID : 802.11 key material */
#define HostCmd_CMD_802_11_KEY_MATERIAL       0x005e

/** Host Command ID : 802.11 Deep Sleep */
#define HostCmd_CMD_802_11_DEEP_SLEEP         0x003e

/** Host Command ID : 802.11 Ad-Hoc stop */
#define HostCmd_CMD_802_11_AD_HOC_STOP        0x0040

/** Host Command ID : 802.11 Host Sleep configuration */
#define HostCmd_CMD_802_11_HOST_SLEEP_CFG     0x0043
/** Host Command ID : 802.11 Wakeup confirm */
#define HostCmd_CMD_802_11_WAKEUP_CONFIRM     0x0044
/** Host Command ID : 802.11 Host Sleep activated */
#define HostCmd_CMD_802_11_HOST_SLEEP_ACTIVATE	0x0045

/** Host Command ID : 802.22 MAC address */
#define HostCmd_CMD_802_11_MAC_ADDRESS        0x004D
/** Host Command ID : 802.11 EEPROM access */
#define HostCmd_CMD_802_11_EEPROM_ACCESS      0x0059

/** Host Command ID : 802.11 D domain information */
#define HostCmd_CMD_802_11D_DOMAIN_INFO       0x005b

/** Host Command ID : WMM Traffic Stream Status */
#define HostCmd_CMD_WMM_TS_STATUS             0x005d

/** Host Command ID : 802.11 TPC information */
#define HostCmd_CMD_802_11_TPC_INFO           0x005f
/** Host Command ID : 802.11 TPC adapt req */
#define HostCmd_CMD_802_11_TPC_ADAPT_REQ      0x0060
/** Host Command ID : 802.11 channel SW ann */
#define HostCmd_CMD_802_11_CHAN_SW_ANN        0x0061
/** Host Command ID : Measurement request */
#define HostCmd_CMD_MEASUREMENT_REQUEST       0x0062
/** Host Command ID : Measurement report */
#define HostCmd_CMD_MEASUREMENT_REPORT        0x0063

/** Host Command ID : 802.11 sleep parameters */
#define HostCmd_CMD_802_11_SLEEP_PARAMS          0x0066

/** Host Command ID : 802.11 sleep period */
#define HostCmd_CMD_802_11_SLEEP_PERIOD          0x0068
/** Host Command ID : 802.11 BCA configuration timeshare */
#define HostCmd_CMD_802_11_BCA_CONFIG_TIMESHARE  0x0069

/** Host Command ID : 802.11 BG scan configuration */
#define HostCmd_CMD_802_11_BG_SCAN_CONFIG        0x006b
/** Host Command ID : 802.11 BG scan query */
#define HostCmd_CMD_802_11_BG_SCAN_QUERY         0x006c

/** Host Command ID : WMM ADDTS req  */
#define HostCmd_CMD_WMM_ADDTS_REQ                0x006E
/** Host Command ID : WMM DELTS req */
#define HostCmd_CMD_WMM_DELTS_REQ                0x006F
/** Host Command ID : WMM queue configuration */
#define HostCmd_CMD_WMM_QUEUE_CONFIG             0x0070
/** Host Command ID : 802.11 get status */
#define HostCmd_CMD_WMM_GET_STATUS               0x0071

/** Host Command ID : 802.11 TPC configuration */
#define HostCmd_CMD_802_11_TPC_CFG               0x0072

/** Host Command ID : 802.11 firmware wakeup method */
#define HostCmd_CMD_802_11_FW_WAKE_METHOD        0x0074

/** Host Command ID : 802.11 LED control */
#define HostCmd_CMD_802_11_LED_CONTROL           0x004e

/** Host Command ID : 802.11 subscribe event */
#define HostCmd_CMD_802_11_SUBSCRIBE_EVENT       0x0075

/** Host Command ID : 802.11 rate adapt rateset */
#define HostCmd_CMD_802_11_RATE_ADAPT_RATESET    0x0076

/** Host Command ID : 802.11 crypto */
#define HostCmd_CMD_802_11_CRYPTO                0x0078

/** Host Command ID : 802.11 Tx rate query */
#define HostCmd_CMD_802_11_TX_RATE_QUERY	0x007f

/** Host Command ID : 802.11 power adapt configuration ext */
#define HostCmd_CMD_802_11_POWER_ADAPT_CFG_EXT	0x007e

/** Host Command ID : Get TSF */
#define HostCmd_CMD_GET_TSF                      0x0080

/** Host Command ID : WMM queue stats */
#define HostCmd_CMD_WMM_QUEUE_STATS              0x0081

/** Host Command ID : 802.11 auto Tx */
#define HostCmd_CMD_802_11_AUTO_TX		0x0082
/** Host Command ID : 802.11 IBSS coalescing status */
#define HostCmd_CMD_802_11_IBSS_COALESCING_STATUS 0x0083

/** Host Command ID : Memory access */
#define HostCmd_CMD_MEM_ACCESS			0x0086

#ifdef MFG_CMD_SUPPORT
/** Host Command ID : Mfg command */
#define HostCmd_CMD_MFG_COMMAND               0x0089
#endif
/** Host Command ID : Inactivity timeout ext */
#define HostCmd_CMD_INACTIVITY_TIMEOUT_EXT 	0x008a

/** Host Command ID : DBGS configuration */
#define HostCmd_CMD_DBGS_CFG		      0x008b
/** Host Command ID : Get memory */
#define HostCmd_CMD_GET_MEM		      0x008c

/** Host Command ID : Tx packets stats */
#define HostCmd_CMD_TX_PKT_STATS              0x008d

/** Host Command ID : Configuration data */
#define HostCmd_CMD_CFG_DATA                  0x008f

/** Host Command ID : SDIO pull control */
#define HostCmd_CMD_SDIO_PULL_CTRL		      0x0093

/** Host Command ID : ECL system clock configuration */
#define HostCmd_CMD_ECL_SYSTEM_CLOCK_CONFIG   0x0094

/** Host Command ID : 802.11 LDO configuration */
#define HostCmd_CMD_802_11_LDO_CONFIG         0x0096

/** Host Command ID : Extended version */
#define HostCmd_CMD_VERSION_EXT               0x0097

/** Host Command ID : Module type configuration */
#define HostCmd_CMD_MODULE_TYPE_CONFIG        0x0099

/** Host Command ID : MEF configuration */
#define HostCmd_CMD_MEF_CFG		      0x009a

/** Host Command ID : 802.11 RSSI INFO*/
#define HostCmd_CMD_RSSI_INFO		      0x00a4

/** Host Command ID : Function initialization */
#define HostCmd_CMD_FUNC_INIT                 0x00a9
/** Host Command ID : Function shutdown */
#define HostCmd_CMD_FUNC_SHUTDOWN             0x00aa

/** Host Command ID : Function interface control */
#define HostCmd_CMD_FUNC_IF_CTRL              0x00ab

/* For the IEEE Power Save */
/** Host Subcommand ID : Enter power save */
#define HostCmd_SubCmd_Enter_PS               0x0030
/** Host Subcommand ID : Exit power save */
#define HostCmd_SubCmd_Exit_PS                0x0031
/** Host Subcommand ID : Sleep confirmed */
#define HostCmd_SubCmd_Sleep_Confirmed        0x0034
/** Host Subcommand ID : Full power down */
#define HostCmd_SubCmd_Full_PowerDown         0x0035
/** Host Subcommand ID : Full power up */
#define HostCmd_SubCmd_Full_PowerUp           0x0036

/** Command RET code, MSB is set to 1 */
#define HostCmd_RET_BIT                       0x8000

/** General Result Code*/
/** General result code OK */
#define HostCmd_RESULT_OK                    0x0000
/** Genenral error */
#define HostCmd_RESULT_ERROR                 0x0001
/** Command is not valid */
#define HostCmd_RESULT_NOT_SUPPORT           0x0002
/** Command is pending */
#define HostCmd_RESULT_PENDING               0x0003
/** System is busy (command ignored) */
#define HostCmd_RESULT_BUSY                  0x0004
/** Data buffer is not big enough */
#define HostCmd_RESULT_PARTIAL_DATA          0x0005

/* Definition of action or option for each command */

/* Define general purpose action */
/** General purpose action : Read */
#define HostCmd_ACT_GEN_READ                    0x0000
/** General purpose action : Write */
#define HostCmd_ACT_GEN_WRITE                   0x0001
/** General purpose action : Get */
#define HostCmd_ACT_GEN_GET                     0x0000
/** General purpose action : Set */
#define HostCmd_ACT_GEN_SET                     0x0001
/** General purpose action : Remove */
#define HostCmd_ACT_GEN_REMOVE                  0x0002
/** General purpose action : Get_Current */
#define HostCmd_ACT_GEN_GET_CURRENT             0x0003
/** General purpose action : Off */
#define HostCmd_ACT_GEN_OFF                     0x0000
/** General purpose action : On */
#define HostCmd_ACT_GEN_ON                      0x0001

/* Define action or option for HostCmd_CMD_802_11_SET_WEP */
/** WEP action : Add */
#define HostCmd_ACT_ADD                         0x0002
/** WEP action : Remove */
#define HostCmd_ACT_REMOVE                      0x0004

/** WEP type : 40 bit */
#define HostCmd_TYPE_WEP_40_BIT                 0x0001
/** WEP type : 104 bit */
#define HostCmd_TYPE_WEP_104_BIT                0x0002

/** WEP Key index mask */
#define HostCmd_WEP_KEY_INDEX_MASK              0x3fff

/* Define action or option for HostCmd_CMD_802_11_SCAN */
/** Scan type : BSS */
#define HostCmd_BSS_TYPE_BSS                    0x0001
/** Scan type : IBSS */
#define HostCmd_BSS_TYPE_IBSS                   0x0002
/** Scan type : Any */
#define HostCmd_BSS_TYPE_ANY                    0x0003

/* Define action or option for HostCmd_CMD_802_11_SCAN */
/** Scan type : Active */
#define HostCmd_SCAN_TYPE_ACTIVE                0x0000
/** Scan type : Passive */
#define HostCmd_SCAN_TYPE_PASSIVE               0x0001

/* Radio type definitions for the channel TLV */
/** Radio type BG */
#define HostCmd_SCAN_RADIO_TYPE_BG		0
/** Radio type A */
#define HostCmd_SCAN_RADIO_TYPE_A		1

/* Define action or option for HostCmd_CMD_MAC_CONTROL */
/** MAC action : Rx on */
#define HostCmd_ACT_MAC_RX_ON                   0x0001
/** MAC action : Tx on */
#define HostCmd_ACT_MAC_TX_ON                   0x0002
/** MAC action : Loopback on */
#define HostCmd_ACT_MAC_LOOPBACK_ON             0x0004
/** MAC action : WEP enable */
#define HostCmd_ACT_MAC_WEP_ENABLE              0x0008
/** MAC action : EthernetII enable */
#define HostCmd_ACT_MAC_ETHERNETII_ENABLE       0x0010
/** MAC action : Promiscous mode enable */
#define HostCmd_ACT_MAC_PROMISCUOUS_ENABLE      0x0080
/** MAC action : All multicast enable */
#define HostCmd_ACT_MAC_ALL_MULTICAST_ENABLE    0x0100
/** MAC action : RTS/CTS  Enable*/
#define HostCmd_ACT_MAC_RTS_CTS_ENABLE			0x0200
/** MAC action : Strict protection enable */
#define HostCmd_ACT_MAC_STRICT_PROTECTION_ENABLE  0x0400
/** MAC action : Ad-Hoc G protection on */
#define HostCmd_ACT_MAC_ADHOC_G_PROTECTION_ON	  0x2000

/* Define action or option or constant for HostCmd_CMD_MAC_MULTICAST_ADR */
/** MAC address size */
#define HostCmd_SIZE_MAC_ADR                    6
/** Maximum number of MAC addresses */
#define HostCmd_MAX_MCAST_ADRS                  32

/** Radio on */
#define RADIO_ON                                0x01
/** Radio off */
#define RADIO_OFF                               0x00

/* Define action or option for CMD_802_11_RF_CHANNEL */
/** Get RF channel */
#define HostCmd_OPT_802_11_RF_CHANNEL_GET       0x00
/** Set RF channel */
#define HostCmd_OPT_802_11_RF_CHANNEL_SET       0x01

/** Host command action : Set Rx */
#define HostCmd_ACT_SET_RX                      0x0001
/** Host command action : Set Tx */
#define HostCmd_ACT_SET_TX                      0x0002
/** Host command action : Set both Rx and Tx */
#define HostCmd_ACT_SET_BOTH                    0x0003
/** Host command action : Get Rx */
#define HostCmd_ACT_GET_RX                      0x0004
/** Host command action : Get Tx */
#define HostCmd_ACT_GET_TX                      0x0008
/** Host command action : Get both Rx and Tx */
#define HostCmd_ACT_GET_BOTH                    0x000c

/** Card Event definition : Dummy host wakeup signal */
#define EVENT_DUMMY_HOST_WAKEUP_SIGNAL  0x00000001
/** Card Event definition : Link lost with scan */
#define EVENT_LINK_LOST_WITH_SCAN       0x00000002
/** Card Event definition : Link lost */
#define EVENT_LINK_LOST                 0x00000003
/** Card Event definition : Link sensed */
#define EVENT_LINK_SENSED               0x00000004
/** Card Event definition : MIB changed */
#define EVENT_MIB_CHANGED               0x00000006
/** Card Event definition : Init done */
#define EVENT_INIT_DONE                 0x00000007
/** Card Event definition : Deauthenticated */
#define EVENT_DEAUTHENTICATED           0x00000008
/** Card Event definition : Disassociated */
#define EVENT_DISASSOCIATED             0x00000009
/** Card Event definition : Power save awake */
#define EVENT_PS_AWAKE                  0x0000000a
/** Card Event definition : Power save sleep */
#define EVENT_PS_SLEEP                  0x0000000b
/** Card Event definition : MIC error multicast */
#define EVENT_MIC_ERR_MULTICAST         0x0000000d
/** Card Event definition : MIC error unicast */
#define EVENT_MIC_ERR_UNICAST           0x0000000e
/** Card Event definition : WM awake */
#define EVENT_WM_AWAKE                  0x0000000f
/** Card Event definition : Deep Sleep awake */
#define EVENT_DEEP_SLEEP_AWAKE          0x00000010
/** Card Event definition : Ad-Hoc BCN lost */
#define EVENT_ADHOC_BCN_LOST            0x00000011
/** Card Event definition : Host Sleep awake */
#define EVENT_HOST_SLEEP_AWAKE          0x00000012
/** Card Event definition : Stop Tx */
#define EVENT_STOP_TX                   0x00000013
/** Card Event definition : Start Tx */
#define EVENT_START_TX                  0x00000014
/** Card Event definition : Channel switch */
#define EVENT_CHANNEL_SWITCH            0x00000015
/** Card Event definition : MEAS report ready */
#define EVENT_MEAS_REPORT_RDY           0x00000016
/** Card Event definition : WMM status change */
#define EVENT_WMM_STATUS_CHANGE         0x00000017
/** Card Event definition : BG scan report */
#define EVENT_BG_SCAN_REPORT            0x00000018
/** Card Event definition : Beacon RSSI low */
#define EVENT_RSSI_LOW                  0x00000019
/** Card Event definition : Beacon SNR low */
#define EVENT_SNR_LOW                   0x0000001a
/** Card Event definition : Maximum fail */
#define EVENT_MAX_FAIL                  0x0000001b
/** Card Event definition : Beacon RSSI high */
#define EVENT_RSSI_HIGH                 0x0000001c
/** Card Event definition : Beacon SNR high */
#define EVENT_SNR_HIGH                  0x0000001d
/** Card Event definition : IBSS coalsced */
#define EVENT_IBSS_COALESCED            0x0000001e
/** Card Event definition : Data RSSI low */
#define EVENT_DATA_RSSI_LOW             0x00000024
/** Card Event definition : Data SNR low */
#define EVENT_DATA_SNR_LOW              0x00000025
/** Card Event definition : Data RSSI high */
#define EVENT_DATA_RSSI_HIGH            0x00000026
/** Card Event definition : Data SNR high */
#define EVENT_DATA_SNR_HIGH             0x00000027
/** Card Event definition : Link Quality */
#define EVENT_LINK_QUALITY              0x00000028
/** Card Event definition : Pre-Beacon Lost */
#define EVENT_PRE_BEACON_LOST           0x00000031

/** Define bitmap conditions for HOST_SLEEP_CFG : Cancel */
#define HOST_SLEEP_CFG_CANCEL			0xffffffff
/** Define bitmap conditions for HOST_SLEEP_CFG : GPIO FF */
#define HOST_SLEEP_CFG_GPIO_FF			0xff
/** Define bitmap conditions for HOST_SLEEP_CFG : GAP FF */
#define HOST_SLEEP_CFG_GAP_FF			0xff

/** Maximum size of multicast list */
#define MRVDRV_MAX_MULTICAST_LIST_SIZE	32
/** Maximum size of channel */
#define MRVDRV_MAX_CHANNEL_SIZE		14
/** Maximum length of SSID */
#define MRVDRV_MAX_SSID_LENGTH			32
/** Maximum number of BSS descriptors */
#define MRVDRV_MAX_BSS_DESCRIPTS		16
/** WEP list macros & data structures */
/** Size of key buffer in bytes */
#define MRVL_KEY_BUFFER_SIZE_IN_BYTE  16
/** Maximum length of WPA key */
#define MRVL_MAX_KEY_WPA_KEY_LENGTH     32

/** 802.11 b */
#define BS_802_11B	0
/** 802.11 g */
#define BS_802_11G	1
/** 802.11 a */
#define BS_802_11A	2
/** Setup the number of rates passed in the driver/firmware API.*/
#define A_SUPPORTED_RATES		14

/** Firmware multiple bands support */
#define FW_MULTI_BANDS_SUPPORT	(BIT(8) | BIT(9) | BIT(10))
/** Band B */
#define	BAND_B			(0x01)
/** Band G */
#define	BAND_G			(0x02)
/** Band A */
#define BAND_A			(0x04)
/** All bands (B, G, A) */
#define ALL_802_11_BANDS	(BAND_B | BAND_G | BAND_A)

/** Check if multiple bands support is enabled in firmware */
#define	IS_SUPPORT_MULTI_BANDS(_adapter) \
				(_adapter->fwCapInfo & FW_MULTI_BANDS_SUPPORT)
/** Get default bands of the firmware */
#define GET_FW_DEFAULT_BANDS(_adapter)	\
				((_adapter->fwCapInfo >> 8) & ALL_802_11_BANDS)

/** Setup the number of rates passed in the driver/firmware API.*/
#define HOSTCMD_SUPPORTED_RATES A_SUPPORTED_RATES

/** Rates supported in band B */
#define B_SUPPORTED_RATES		8
/** Rates supported in band G */
#define G_SUPPORTED_RATES		14

/** WLAN supported rates */
#define	WLAN_SUPPORTED_RATES		14

/** Maximum power adapt group size */
#define	MAX_POWER_ADAPT_GROUP		5

/** 802.11 supported rates */
typedef u8 WLAN_802_11_RATES[WLAN_SUPPORTED_RATES];
/** 802.11 MAC address */
typedef u8 WLAN_802_11_MAC_ADDRESS[ETH_ALEN];

/** WLAN_802_11_NETWORK_TYPE */
typedef enum _WLAN_802_11_NETWORK_TYPE
{
    Wlan802_11FH,
    Wlan802_11DS,
    /* defined as upper bound */
    Wlan802_11NetworkTypeMax
} WLAN_802_11_NETWORK_TYPE, *PWLAN_802_11_NETWORK_TYPE;

/** WLAN_802_11_NETWORK_INFRASTRUCTURE */
typedef enum _WLAN_802_11_NETWORK_INFRASTRUCTURE
{
    Wlan802_11IBSS,
    Wlan802_11Infrastructure,
    Wlan802_11AutoUnknown,
    /* defined as upper bound */
    Wlan802_11InfrastructureMax
} WLAN_802_11_NETWORK_INFRASTRUCTURE, *PWLAN_802_11_NETWORK_INFRASTRUCTURE;

/** Maximum size of IEEE Information Elements */
#define IEEE_MAX_IE_SIZE  256

/** IEEE Type definitions  */
typedef enum _IEEEtypes_ElementId_e
{
    SSID = 0,
    SUPPORTED_RATES = 1,
    FH_PARAM_SET = 2,
    DS_PARAM_SET = 3,
    CF_PARAM_SET = 4,

    IBSS_PARAM_SET = 6,

    COUNTRY_INFO = 7,

    POWER_CONSTRAINT = 32,
    POWER_CAPABILITY = 33,
    TPC_REQUEST = 34,
    TPC_REPORT = 35,
    SUPPORTED_CHANNELS = 36,
    CHANNEL_SWITCH_ANN = 37,
    QUIET = 40,
    IBSS_DFS = 41,
    ERP_INFO = 42,
    EXTENDED_SUPPORTED_RATES = 50,

    VENDOR_SPECIFIC_221 = 221,
    WMM_IE = VENDOR_SPECIFIC_221,

    WPS_IE = VENDOR_SPECIFIC_221,

    WPA_IE = VENDOR_SPECIFIC_221,
    RSN_IE = 48,

} __ATTRIB_PACK__ IEEEtypes_ElementId_e;

/** Capability information mask */
#define CAPINFO_MASK    (~( BIT(15) | BIT(14) |               \
                            BIT(12) | BIT(11) | BIT(9)) )

#ifdef BIG_ENDIAN
/** Capability Bit Map*/
typedef struct _IEEEtypes_CapInfo_t
{
    u8 Rsrvd1:2;
    u8 DSSSOFDM:1;
    u8 Rsvrd2:2;
    u8 ShortSlotTime:1;
    u8 Rsrvd3:1;
    u8 SpectrumMgmt:1;
    u8 ChanAgility:1;
    u8 Pbcc:1;
    u8 ShortPreamble:1;
    u8 Privacy:1;
    u8 CfPollRqst:1;
    u8 CfPollable:1;
    u8 Ibss:1;
    u8 Ess:1;
} __ATTRIB_PACK__ IEEEtypes_CapInfo_t;
#else
typedef struct _IEEEtypes_CapInfo_t
{
    /** Capability Bit Map : ESS */
    u8 Ess:1;
    /** Capability Bit Map : IBSS */
    u8 Ibss:1;
    /** Capability Bit Map : CF pollable */
    u8 CfPollable:1;
    /** Capability Bit Map : CF poll request */
    u8 CfPollRqst:1;
    /** Capability Bit Map : Privacy */
    u8 Privacy:1;
    /** Capability Bit Map : Short preamble */
    u8 ShortPreamble:1;
    /** Capability Bit Map : PBCC */
    u8 Pbcc:1;
    /** Capability Bit Map : Channel agility */
    u8 ChanAgility:1;
    /** Capability Bit Map : Spectrum management */
    u8 SpectrumMgmt:1;
    /** Capability Bit Map : Reserved */
    u8 Rsrvd3:1;
    /** Capability Bit Map : Short slot time */
    u8 ShortSlotTime:1;
    /** Capability Bit Map : APSD */
    u8 Apsd:1;
    /** Capability Bit Map : Reserved */
    u8 Rsvrd2:1;
    /** Capability Bit Map : DSS OFDM */
    u8 DSSSOFDM:1;
    /** Capability Bit Map : Reserved */
    u8 Rsrvd1:2;
} __ATTRIB_PACK__ IEEEtypes_CapInfo_t;

#endif /* BIG_ENDIAN */

typedef struct
{
    /** Element ID */
    u8 ElementId;
    /** Length */
    u8 Len;
} __ATTRIB_PACK__ IEEEtypes_Header_t;

/** IEEEtypes_CfParamSet_t */
typedef struct _IEEEtypes_CfParamSet_t
{
    /** CF peremeter : Element ID */
    u8 ElementId;
    /** CF peremeter : Length */
    u8 Len;
    /** CF peremeter : Count */
    u8 CfpCnt;
    /** CF peremeter : Period */
    u8 CfpPeriod;
    /** CF peremeter : Maximum duration */
    u16 CfpMaxDuration;
    /** CF peremeter : Remaining duration */
    u16 CfpDurationRemaining;
} __ATTRIB_PACK__ IEEEtypes_CfParamSet_t;

typedef struct IEEEtypes_IbssParamSet_t
{
    /** Element ID */
    u8 ElementId;
    /** Length */
    u8 Len;
    /** ATIM window value */
    u16 AtimWindow;
} __ATTRIB_PACK__ IEEEtypes_IbssParamSet_t;

/** IEEEtypes_SsParamSet_t */
typedef union _IEEEtypes_SsParamSet_t
{
    /** SS parameter : CF parameter set */
    IEEEtypes_CfParamSet_t CfParamSet;
    /** SS parameter : IBSS parameter set */
    IEEEtypes_IbssParamSet_t IbssParamSet;
} __ATTRIB_PACK__ IEEEtypes_SsParamSet_t;

/** IEEEtypes_FhParamSet_t */
typedef struct _IEEEtypes_FhParamSet_t
{
    /** FH parameter : Element ID */
    u8 ElementId;
    /** FH parameter : Length */
    u8 Len;
    /** FH parameter : Dwell time */
    u16 DwellTime;
    /** FH parameter : Hop set */
    u8 HopSet;
    /** FH parameter : Hop pattern */
    u8 HopPattern;
    /** FH parameter : Hop index */
    u8 HopIndex;
} __ATTRIB_PACK__ IEEEtypes_FhParamSet_t;

/** IEEEtypes_DsParamSet_t */
typedef struct _IEEEtypes_DsParamSet_t
{
    /** DS parameter : Element ID */
    u8 ElementId;
    /** DS parameter : Length */
    u8 Len;
    /** DS parameter : Current channel */
    u8 CurrentChan;
} __ATTRIB_PACK__ IEEEtypes_DsParamSet_t;

/** IEEEtypes_PhyParamSet_t */
typedef union IEEEtypes_PhyParamSet_t
{
    /** FH parameter set */
    IEEEtypes_FhParamSet_t FhParamSet;
    /** DS parameter set */
    IEEEtypes_DsParamSet_t DsParamSet;
} __ATTRIB_PACK__ IEEEtypes_PhyParamSet_t;

typedef struct _IEEEtypes_ERPInfo_t
{
    /** Element ID */
    u8 ElementId;
    /** Length */
    u8 Len;
    /** ERP flags */
    u8 ERPFlags;
} __ATTRIB_PACK__ IEEEtypes_ERPInfo_t;

/** 16 bit unsigned integer */
typedef u16 IEEEtypes_AId_t;
/** 16 bit unsigned integer */
typedef u16 IEEEtypes_StatusCode_t;

typedef struct
{
    /** Capability information */
    IEEEtypes_CapInfo_t Capability;
    /** Association response status code */
    IEEEtypes_StatusCode_t StatusCode;
    /** Association ID */
    IEEEtypes_AId_t AId;
    /** IE data buffer */
    u8 IEBuffer[1];
} __ATTRIB_PACK__ IEEEtypes_AssocRsp_t;

typedef struct
{
    /** Element ID */
    u8 ElementId;
    /** Length */
    u8 Len;
    /** OUI */
    u8 Oui[3];
    /** OUI type */
    u8 OuiType;
    /** OUI subtype */
    u8 OuiSubtype;
    /** Version */
    u8 Version;
} __ATTRIB_PACK__ IEEEtypes_VendorHeader_t;

typedef struct
{
    /** Vendor specific IE header */
    IEEEtypes_VendorHeader_t VendHdr;

    /** IE Max - size of previous fields */
    u8 Data[IEEE_MAX_IE_SIZE - sizeof(IEEEtypes_VendorHeader_t)];

}
__ATTRIB_PACK__ IEEEtypes_VendorSpecific_t;

typedef struct
{
    /** Generic IE header */
    IEEEtypes_Header_t IeeeHdr;

    /** IE Max - size of previous fields */
    u8 Data[IEEE_MAX_IE_SIZE - sizeof(IEEEtypes_Header_t)];

}
__ATTRIB_PACK__ IEEEtypes_Generic_t;

/** TLV  type ID definition */
#define PROPRIETARY_TLV_BASE_ID		0x0100

/** Terminating TLV Type */
#define MRVL_TERMINATE_TLV_ID		0xffff

/** TLV type : SSID */
#define TLV_TYPE_SSID				0x0000
/** TLV type : Rates */
#define TLV_TYPE_RATES				0x0001
/** TLV type : PHY FH */
#define TLV_TYPE_PHY_FH				0x0002
/** TLV type : PHY DS */
#define TLV_TYPE_PHY_DS				0x0003
/** TLV type : CF */
#define TLV_TYPE_CF				    0x0004
/** TLV type : IBSS */
#define TLV_TYPE_IBSS				0x0006

/** TLV type : Domain */
#define TLV_TYPE_DOMAIN				0x0007

/** TLV type : Power constraint */
#define TLV_TYPE_POWER_CONSTRAINT   0x0020
/** TLV type : Power capability */
#define TLV_TYPE_POWER_CAPABILITY   0x0021

/** TLV type : Key material */
#define TLV_TYPE_KEY_MATERIAL       (PROPRIETARY_TLV_BASE_ID + 0)
/** TLV type : Channel list */
#define TLV_TYPE_CHANLIST           (PROPRIETARY_TLV_BASE_ID + 1)
/** TLV type : Number of probes */
#define TLV_TYPE_NUMPROBES          (PROPRIETARY_TLV_BASE_ID + 2)
/** TLV type : Beacon RSSI low */
#define TLV_TYPE_RSSI_LOW           (PROPRIETARY_TLV_BASE_ID + 4)
/** TLV type : Beacon SNR low */
#define TLV_TYPE_SNR_LOW            (PROPRIETARY_TLV_BASE_ID + 5)
/** TLV type : Fail count */
#define TLV_TYPE_FAILCOUNT          (PROPRIETARY_TLV_BASE_ID + 6)
/** TLV type : BCN miss */
#define TLV_TYPE_BCNMISS            (PROPRIETARY_TLV_BASE_ID + 7)
/** TLV type : LED GPIO */
#define TLV_TYPE_LED_GPIO           (PROPRIETARY_TLV_BASE_ID + 8)
/** TLV type : LED behavior */
#define TLV_TYPE_LEDBEHAVIOR        (PROPRIETARY_TLV_BASE_ID + 9)
/** TLV type : Passthrough */
#define TLV_TYPE_PASSTHROUGH        (PROPRIETARY_TLV_BASE_ID + 10)
/** TLV type : Power TBL 2.4 Ghz */
#define TLV_TYPE_POWER_TBL_2_4GHZ   (PROPRIETARY_TLV_BASE_ID + 12)
/** TLV type : Power TBL 5 GHz */
#define TLV_TYPE_POWER_TBL_5GHZ     (PROPRIETARY_TLV_BASE_ID + 13)
/** TLV type : WMM queue status */
#define TLV_TYPE_WMMQSTATUS         (PROPRIETARY_TLV_BASE_ID + 16)
/** TLV type : Crypto data */
#define TLV_TYPE_CRYPTO_DATA        (PROPRIETARY_TLV_BASE_ID + 17)
/** TLV type : Wildcard SSID */
#define TLV_TYPE_WILDCARDSSID       (PROPRIETARY_TLV_BASE_ID + 18)
/** TLV type : TSF timestamp */
#define TLV_TYPE_TSFTIMESTAMP       (PROPRIETARY_TLV_BASE_ID + 19)
/** TLV type : Power adapter configuration ext */
#define TLV_TYPE_POWERADAPTCFGEXT   (PROPRIETARY_TLV_BASE_ID + 20)
/** TLV type : Beacon RSSI high */
#define TLV_TYPE_RSSI_HIGH          (PROPRIETARY_TLV_BASE_ID + 22)
/** TLV type : Beacon SNR high */
#define TLV_TYPE_SNR_HIGH           (PROPRIETARY_TLV_BASE_ID + 23)
/** TLV type : Auto Tx */
#define TLV_TYPE_AUTO_TX            (PROPRIETARY_TLV_BASE_ID + 24)

/** TLV type : Start BG scan later */
#define TLV_TYPE_STARTBGSCANLATER   (PROPRIETARY_TLV_BASE_ID + 30)
/** TLV type : Authentication type */
#define TLV_TYPE_AUTH_TYPE          (PROPRIETARY_TLV_BASE_ID + 31)
/** TLV type :Link Quality */
#define TLV_TYPE_LINK_QUALITY       (PROPRIETARY_TLV_BASE_ID + 36)
/** TLV type : Data RSSI low */
#define TLV_TYPE_RSSI_LOW_DATA      (PROPRIETARY_TLV_BASE_ID + 38)
/** TLV type : Data SNR low */
#define TLV_TYPE_SNR_LOW_DATA       (PROPRIETARY_TLV_BASE_ID + 39)
/** TLV type : Data RSSI high */
#define TLV_TYPE_RSSI_HIGH_DATA     (PROPRIETARY_TLV_BASE_ID + 40)
/** TLV type : Data SNR high */
#define TLV_TYPE_SNR_HIGH_DATA      (PROPRIETARY_TLV_BASE_ID + 41)

/** TLV type : Channel band list */
#define TLV_TYPE_CHANNELBANDLIST    (PROPRIETARY_TLV_BASE_ID + 42)

/** TLV type: Pre-Beacon Lost */
#define TLV_TYPE_PRE_BEACON_LOST    (PROPRIETARY_TLV_BASE_ID + 73)

/** TLV related data structures*/
/** MrvlIEtypesHeader_t */
typedef struct _MrvlIEtypesHeader
{
    /** Header type */
    u16 Type;
    /** Header length */
    u16 Len;
} __ATTRIB_PACK__ MrvlIEtypesHeader_t;

/** MrvlIEtypes_Data_t */
typedef struct _MrvlIEtypes_Data_t
{
    /** Header */
    MrvlIEtypesHeader_t Header;
    /** Data */
    u8 Data[1];
} __ATTRIB_PACK__ MrvlIEtypes_Data_t;

/** MrvlIEtypes_RatesParamSet_t */
typedef struct _MrvlIEtypes_RatesParamSet_t
{
    /** Header */
    MrvlIEtypesHeader_t Header;
    /** Rates */
    u8 Rates[1];
} __ATTRIB_PACK__ MrvlIEtypes_RatesParamSet_t;

/** MrvlIEtypes_SsIdParamSet_t */
typedef struct _MrvlIEtypes_SsIdParamSet_t
{
    /** Header */
    MrvlIEtypesHeader_t Header;
    /** SSID */
    u8 SsId[1];
} __ATTRIB_PACK__ MrvlIEtypes_SsIdParamSet_t;

/** MrvlIEtypes_WildCardSsIdParamSet_t */
typedef struct _MrvlIEtypes_WildCardSsIdParamSet_t
{
    /** Header */
    MrvlIEtypesHeader_t Header;
    /** Maximum SSID length */
    u8 MaxSsidLength;
    /** SSID */
    u8 SsId[1];
} __ATTRIB_PACK__ MrvlIEtypes_WildCardSsIdParamSet_t;

/** ChanScanMode_t */
typedef struct
{
#ifdef BIG_ENDIAN
    u8 Reserved_2_7:6;
    u8 DisableChanFilt:1;
    u8 PassiveScan:1;
#else
    /** Channel scan mode passive flag */
    u8 PassiveScan:1;
    /** Disble channel filtering flag */
    u8 DisableChanFilt:1;
    /** Reserved */
    u8 Reserved_2_7:6;
#endif
} __ATTRIB_PACK__ ChanScanMode_t;

/** ChanScanParamSet_t */
typedef struct _ChanScanParamSet_t
{
    /** Channel scan parameter : radio type */
    u8 RadioType;
    /** Channel scan parameter : channel number */
    u8 ChanNumber;
    /** Channel scan parameter : channel scan mode */
    ChanScanMode_t ChanScanMode;
    /** Channel scan parameter : minimum scan time */
    u16 MinScanTime;
    /** Channel scan parameter : maximum scan time */
    u16 MaxScanTime;
} __ATTRIB_PACK__ ChanScanParamSet_t;

/** MrvlIEtypes_ChanListParamSet_t */
typedef struct _MrvlIEtypes_ChanListParamSet_t
{
    /** Header */
    MrvlIEtypesHeader_t Header;
    /** Channel scan parameters */
    ChanScanParamSet_t ChanScanParam[1];
} __ATTRIB_PACK__ MrvlIEtypes_ChanListParamSet_t;

/** ChanScanParamSet_t */
typedef struct _ChanBandParamSet_t
{
   /** Channel scan parameter : radio type */
    u8 RadioType;
    /** channel number */
    u8 ChanNumber;
} __ATTRIB_PACK__ ChanBandParamSet_t;

/** MrvlIEtypes_ChanBandListParamSet_t */
typedef struct _MrvlIEtypes_ChanBandListParamSet_t
{
    /** Header */
    MrvlIEtypesHeader_t Header;
    /** Channel Band parameters */
    ChanBandParamSet_t ChanBandParam[1];
} __ATTRIB_PACK__ MrvlIEtypes_ChanBandListParamSet_t;

/** CfParamSet_t */
typedef struct _CfParamSet_t
{
    /** CF parameter : count */
    u8 CfpCnt;
    /** CF parameter : period */
    u8 CfpPeriod;
    /** CF parameter : duration */
    u16 CfpMaxDuration;
    /** CF parameter : duration remaining */
    u16 CfpDurationRemaining;
} __ATTRIB_PACK__ CfParamSet_t;

/** IbssParamSet_t */
typedef struct _IbssParamSet_t
{
    /** ATIM window value */
    u16 AtimWindow;
} __ATTRIB_PACK__ IbssParamSet_t;

/** MrvlIEtypes_SsParamSet_t */
typedef struct _MrvlIEtypes_SsParamSet_t
{
    /** Header */
    MrvlIEtypesHeader_t Header;
    union
    {
        /** CF parameter set */
        CfParamSet_t CfParamSet[1];
        /** IBSS parameter set */
        IbssParamSet_t IbssParamSet[1];
    } cf_ibss;
} __ATTRIB_PACK__ MrvlIEtypes_SsParamSet_t;

/** FhParamSet_t */
typedef struct _FhParamSet_t
{
    /** FH parameter : Dwell time */
    u16 DwellTime;
    /** FH parameter : Hop set */
    u8 HopSet;
    /** FH parameter : Hop pattern */
    u8 HopPattern;
    /** FH parameter : Hop index */
    u8 HopIndex;
} __ATTRIB_PACK__ FhParamSet_t;

/** DsParamSet_t */
typedef struct _DsParamSet_t
{
    /** Current channel number */
    u8 CurrentChan;
} __ATTRIB_PACK__ DsParamSet_t;

/** MrvlIEtypes_PhyParamSet_t */
typedef struct _MrvlIEtypes_PhyParamSet_t
{
    /** Header */
    MrvlIEtypesHeader_t Header;
    union
    {
        /** FH parameter set */
        FhParamSet_t FhParamSet[1];
        /** DS parameter set */
        DsParamSet_t DsParamSet[1];
    } fh_ds;
} __ATTRIB_PACK__ MrvlIEtypes_PhyParamSet_t;

/** MrvlIEtypes_RsnParamSet_t */
typedef struct _MrvlIEtypes_RsnParamSet_t
{
    /** Header */
    MrvlIEtypesHeader_t Header;
    /** RSN IE */
    u8 RsnIE[1];
} __ATTRIB_PACK__ MrvlIEtypes_RsnParamSet_t;

/** MrvlIEtypes_WmmParamSet_t */
typedef struct _MrvlIEtypes_WmmParamSet_t
{
    /** Header */
    MrvlIEtypesHeader_t Header;
    /** WMM IE */
    u8 WmmIE[1];
} __ATTRIB_PACK__ MrvlIEtypes_WmmParamSet_t;

typedef struct
{
    /** Header */
    MrvlIEtypesHeader_t Header;
    /** Queue index */
    u8 QueueIndex;
    /** Disabled flag */
    u8 Disabled;
    /** Medium time allocation in 32us units*/
    u16 MediumTime;
    /** Flow required flag */
    u8 FlowRequired;
    /** Flow created flag */
    u8 FlowCreated;
    /** Reserved */
    u32 Reserved;
} __ATTRIB_PACK__ MrvlIEtypes_WmmQueueStatus_t;

/** Table of TSF values returned in the scan result */
typedef struct
{
    /** Header */
    MrvlIEtypesHeader_t Header;
    /** TSF table */
    u64 tsfTable[1];
} __ATTRIB_PACK__ MrvlIEtypes_TsfTimestamp_t;

/**  Local Power Capability */
typedef struct _MrvlIEtypes_PowerCapability_t
{
    /** Header */
    MrvlIEtypesHeader_t Header;
    /** Minmum power */
    s8 MinPower;
    /** Maximum power */
    s8 MaxPower;
} __ATTRIB_PACK__ MrvlIEtypes_PowerCapability_t;

/** MrvlIEtypes_RssiParamSet_t */
typedef struct _MrvlIEtypes_RssiThreshold_t
{
    /** Header */
    MrvlIEtypesHeader_t Header;
    /** RSSI value */
    u8 RSSIValue;
    /** RSSI frequency */
    u8 RSSIFreq;
} __ATTRIB_PACK__ MrvlIEtypes_RssiThreshold_t;

/** MrvlIEtypes_SnrThreshold_t */
typedef struct _MrvlIEtypes_SnrThreshold_t
{
    /** Header */
    MrvlIEtypesHeader_t Header;
    /** SNR value */
    u8 SNRValue;
    /** SNR frequency */
    u8 SNRFreq;
} __ATTRIB_PACK__ MrvlIEtypes_SnrThreshold_t;

/** MrvlIEtypes_LinkQuality_t */
typedef struct _MrvlIEtypes_LinkQuality_t
{
    /** Header */
    MrvlIEtypesHeader_t Header;
    /** Link SNR threshold */
    u16 LinkSNRThrs;
    /** Link SNR frequency */
    u16 LinkSNRFreq;
    /** Minimum rate value */
    u16 MinRateVal;
    /** Minimum rate frequency */
    u16 MinRateFreq;
    /** Tx latency value */
    u32 TxLatencyVal;
    /** Tx latency threshold */
    u32 TxLatencyThrs;
} __ATTRIB_PACK__ MrvlIEtypes_LinkQuality_t;

/** MrvlIEtypes_PreBeaconLost_t */
typedef struct _MrvlIEtypes_PreBeaconLost_t
{
    /** Header */
    MrvlIEtypesHeader_t Header;
    /** Pre-Beacon Lost */
    u8 PreBeaconLost;
    /** Reserved */
    u8 Reserved;
} __ATTRIB_PACK__ MrvlIEtypes_PreBeaconLost_t;

/** MrvlIEtypes_FailureCount_t */
typedef struct _MrvlIEtypes_FailureCount_t
{
    /** Header */
    MrvlIEtypesHeader_t Header;
    /** Failure value */
    u8 FailValue;
    /** Failure frequency */
    u8 FailFreq;
} __ATTRIB_PACK__ MrvlIEtypes_FailureCount_t;

/** MrvlIEtypes_BeaconsMissed_t */
typedef struct _MrvlIEtypes_BeaconsMissed_t
{
    /** Header */
    MrvlIEtypesHeader_t Header;
    /** Number of beacons missed */
    u8 BeaconMissed;
    /** Reserved */
    u8 Reserved;
} __ATTRIB_PACK__ MrvlIEtypes_BeaconsMissed_t;

/** MrvlIEtypes_NumProbes_t */
typedef struct _MrvlIEtypes_NumProbes_t
{
    /** Header */
    MrvlIEtypesHeader_t Header;
    /** Number of probes */
    u16 NumProbes;
} __ATTRIB_PACK__ MrvlIEtypes_NumProbes_t;

/** MrvlIEtypes_StartBGScanLater_t */
typedef struct _MrvlIEtypes_StartBGScanLater_t
{
    /** Header */
    MrvlIEtypesHeader_t Header;
    /** Start later flag */
    u16 StartLater;
} __ATTRIB_PACK__ MrvlIEtypes_StartBGScanLater_t;

typedef struct _LedGpio_t
{
    u8 LedNum;                                  /**< LED # mapped to GPIO pin # below */
    u8 GpioNum;                                 /**< GPIO pin # used to control LED # above */
} __ATTRIB_PACK__ LedGpio_t;

/** MrvlIEtypes_LedGpio_t */
typedef struct _MrvlIEtypes_LedGpio_t
{
    /** Header */
    MrvlIEtypesHeader_t Header;
    /** LED GPIO */
    LedGpio_t LedGpio[1];
} __ATTRIB_PACK__ MrvlIEtypes_LedGpio_t;

/** MrvlIEtypes_LedBehavior_t */
typedef struct _MrvlIEtypes_LedBehavior_t
{
    MrvlIEtypesHeader_t Header;         /**< Header */
    u8 FirmwareState;                           /**< Firmware State */
    u8 LedNum;                                  /**< LED # */
    u8 LedState;                                /**< LED State corresponding to Firmware State */
    u8 LedArgs;                                 /**< Arguments for LED State */
} __ATTRIB_PACK__ MrvlIEtypes_LedBehavior_t;

typedef struct _PA_Group_t
{
    u16 PowerAdaptLevel;                            /**< Power adapt level */
    u16 RateBitmap;                                 /**< Rate bitmap */
    u32 Reserved;                                   /**< Reserved */
} __ATTRIB_PACK__ PA_Group_t;

/** MrvlIEtypes_PA_Group_t */
typedef struct _MrvlIEtypes_PowerAdapt_Group_t
{
    MrvlIEtypesHeader_t Header;                               /**< Header */
    PA_Group_t PA_Group[MAX_POWER_ADAPT_GROUP];                   /**< Power adapt group */
} __ATTRIB_PACK__ MrvlIEtypes_PowerAdapt_Group_t;

typedef struct _AutoTx_MacFrame_t
{
    u16 Interval;                               /**< in seconds */
    u8 Priority;                                /**< User Priority: 0~7, ignored if non-WMM */
    u8 Reserved;                                /**< set to 0 */
    u16 FrameLen;                               /**< Length of MAC frame payload */
    u8 DestMacAddr[ETH_ALEN];                           /**< Destination MAC address */
    u8 SrcMacAddr[ETH_ALEN];                            /**< Source MAC address */
    u8 Payload[];                                       /**< Payload */
} __ATTRIB_PACK__ AutoTx_MacFrame_t;

/** MrvlIEtypes_AutoTx_t */
typedef struct _MrvlIEtypes_AutoTx_t
{
    MrvlIEtypesHeader_t Header;                     /**< Header */
    AutoTx_MacFrame_t AutoTx_MacFrame;              /**< Auto Tx MAC frame */
} __ATTRIB_PACK__ MrvlIEtypes_AutoTx_t;

/** Auth type to be used in the Authentication portion of an Assoc seq */
typedef struct
{
    /** Header */
    MrvlIEtypesHeader_t Header;
    /** Authentication type */
    u16 AuthType;
} __ATTRIB_PACK__ MrvlIEtypes_AuthType_t;

/** Maximum subbands for 11d */
#define MRVDRV_MAX_SUBBAND_802_11D		83
/** Country code length */
#define COUNTRY_CODE_LEN			3

/** Data structure for Country IE*/
typedef struct _IEEEtypes_SubbandSet
{
    u8 FirstChan;               /**< First channel */
    u8 NoOfChan;                /**< Number of channels */
    u8 MaxTxPwr;                    /**< Maximum Tx power */
} __ATTRIB_PACK__ IEEEtypes_SubbandSet_t;

typedef struct _IEEEtypes_CountryInfoSet
{
    u8 ElementId;                               /**< Element ID */
    u8 Len;                                     /**< Length */
    u8 CountryCode[COUNTRY_CODE_LEN];           /**< Country code */
    IEEEtypes_SubbandSet_t Subband[1];      /**< Set of subbands */
} __ATTRIB_PACK__ IEEEtypes_CountryInfoSet_t;

typedef struct _IEEEtypes_CountryInfoFullSet
{
    u8 ElementId;                               /**< Element ID */
    u8 Len;                                     /**< Length */
    u8 CountryCode[COUNTRY_CODE_LEN];           /**< Country code */
    /** Set of subbands */
    IEEEtypes_SubbandSet_t Subband[MRVDRV_MAX_SUBBAND_802_11D];
} __ATTRIB_PACK__ IEEEtypes_CountryInfoFullSet_t;

typedef struct _MrvlIEtypes_DomainParamSet
{
    MrvlIEtypesHeader_t Header;                         /**< Header */
    u8 CountryCode[COUNTRY_CODE_LEN];                           /**< Country code */
    IEEEtypes_SubbandSet_t Subband[1];              /**< Set of subbands */
} __ATTRIB_PACK__ MrvlIEtypes_DomainParamSet_t;

/** Size of a TSPEC.  Used to allocate necessary buffer space in commands */
#define WMM_TSPEC_SIZE              63

/** Extra IE bytes allocated in messages for appended IEs after a TSPEC */
#define WMM_ADDTS_EXTRA_IE_BYTES    256

/** Extra TLV bytes allocated in messages for configuring WMM Queues */
#define WMM_QUEUE_CONFIG_EXTRA_TLV_BYTES 64

/** Maximum number of AC QOS queues available in the driver/firmware */
#define MAX_AC_QUEUES 4

/** enum of WMM AC_QUEUES */
typedef enum
{
    WMM_AC_BK,
    WMM_AC_BE,
    WMM_AC_VI,
    WMM_AC_VO,
} __ATTRIB_PACK__ wlan_wmm_ac_e;

/** data structure of WMM QoS information */
typedef struct
{
#ifdef BIG_ENDIAN
    /** QoS UAPSD */
    u8 QosUAPSD:1;
    /** Reserved */
    u8 Reserved:3;
    /** Parameter set count */
    u8 ParaSetCount:4;
#else
    /** Parameter set count */
    u8 ParaSetCount:4;
    /** Reserved */
    u8 Reserved:3;
    /** QoS UAPSD */
    u8 QosUAPSD:1;
#endif
} __ATTRIB_PACK__ IEEEtypes_WmmQosInfo_t;

typedef struct
{
#ifdef BIG_ENDIAN
    /** Reserved */
    u8 Reserved:1;
    /** Aci */
    u8 Aci:2;
    /** Acm */
    u8 Acm:1;
    /** Aifsn */
    u8 Aifsn:4;
#else
    /** Aifsn */
    u8 Aifsn:4;
    /** Acm */
    u8 Acm:1;
    /** Aci */
    u8 Aci:2;
    /** Reserved */
    u8 Reserved:1;
#endif
} __ATTRIB_PACK__ IEEEtypes_WmmAciAifsn_t;

/**  data structure of WMM ECW */
typedef struct
{
#ifdef BIG_ENDIAN
    /** Maximum Ecw */
    u8 EcwMax:4;
    /** Minimum Ecw */
    u8 EcwMin:4;
#else
    /** Minimum Ecw */
    u8 EcwMin:4;
    /** Maximum Ecw */
    u8 EcwMax:4;
#endif
} __ATTRIB_PACK__ IEEEtypes_WmmEcw_t;

/** data structure of WMM AC parameters  */
typedef struct
{
    IEEEtypes_WmmAciAifsn_t AciAifsn;       /**< AciAifSn */
    IEEEtypes_WmmEcw_t Ecw;                 /**< Ecw */
    u16 TxopLimit;                          /**< Tx op limit */
} __ATTRIB_PACK__ IEEEtypes_WmmAcParameters_t;

/** data structure of WMM Info IE  */
typedef struct
{

    /**
     * WMM Info IE - Vendor Specific Header:
     *   ElementId   [221/0xdd]
     *   Len         [7] 
     *   Oui         [00:50:f2]
     *   OuiType     [2]
     *   OuiSubType  [0]
     *   Version     [1]
     */
    IEEEtypes_VendorHeader_t VendHdr;

    /** QoS information */
    IEEEtypes_WmmQosInfo_t QoSInfo;

} __ATTRIB_PACK__ IEEEtypes_WmmInfo_t;

/** data structure of WMM parameter IE  */
typedef struct
{
    /**
     * WMM Parameter IE - Vendor Specific Header:
     *   ElementId   [221/0xdd]
     *   Len         [24] 
     *   Oui         [00:50:f2]
     *   OuiType     [2]
     *   OuiSubType  [1]
     *   Version     [1]
     */
    IEEEtypes_VendorHeader_t VendHdr;

    /** QoS information */
    IEEEtypes_WmmQosInfo_t QoSInfo;
    /** Reserved */
    u8 Reserved;

    /** AC Parameters Record WMM_AC_BE, WMM_AC_BK, WMM_AC_VI, WMM_AC_VO */
    IEEEtypes_WmmAcParameters_t AcParams[MAX_AC_QUEUES];

} __ATTRIB_PACK__ IEEEtypes_WmmParameter_t;

/**
 *  @brief Firmware command structure to retrieve the firmware WMM status.
 *
 *  Used to retrieve the status of each WMM AC Queue in TLV 
 *    format (MrvlIEtypes_WmmQueueStatus_t) as well as the current WMM
 *    parameter IE advertised by the AP.  
 *  
 *  Used in response to a EVENT_WMM_STATUS_CHANGE event signaling
 *    a QOS change on one of the ACs or a change in the WMM Parameter in
 *    the Beacon.
 *
 *  TLV based command, byte arrays used for max sizing purpose. There are no 
 *    arguments sent in the command, the TLVs are returned by the firmware.
 */
typedef struct
{
    /** Queue status TLV */
    u8 queueStatusTlv[sizeof(MrvlIEtypes_WmmQueueStatus_t) * MAX_AC_QUEUES];
    /** WMM parameter TLV */
    u8 wmmParamTlv[sizeof(IEEEtypes_WmmParameter_t) + 2];

}
__ATTRIB_PACK__ HostCmd_DS_WMM_GET_STATUS;

/**
 *  @brief Enumeration for the command result from an ADDTS or DELTS command 
 */
typedef enum
{
    TSPEC_RESULT_SUCCESS = 0,
    TSPEC_RESULT_EXEC_FAILURE = 1,
    TSPEC_RESULT_TIMEOUT = 2,
    TSPEC_RESULT_DATA_INVALID = 3,

} __ATTRIB_PACK__ wlan_wmm_tspec_result_e;

/**
 *  @brief IOCTL structure to send an ADDTS request and retrieve the response.
 *
 *  IOCTL structure from the application layer relayed to firmware to 
 *    instigate an ADDTS management frame with an appropriate TSPEC IE as well
 *    as any additional IEs appended in the ADDTS Action frame.
 *
 *  @sa wlan_wmm_addts_req_ioctl
 */
typedef struct
{
    wlan_wmm_tspec_result_e commandResult;  /**< Firmware execution result */
    u32 timeout_ms;                         /**< Timeout value in milliseconds */

    u8 ieeeStatusCode;                      /**< IEEE status code */

    u8 tspecData[WMM_TSPEC_SIZE];           /**< TSPEC to send in the ADDTS */

    u8 addtsExtraIEBuf[WMM_ADDTS_EXTRA_IE_BYTES];   /**< ADDTS extra IE buffer */

} __ATTRIB_PACK__ wlan_ioctl_wmm_addts_req_t;

/**
 *  @brief IOCTL structure to send a DELTS request.
 *
 *  IOCTL structure from the application layer relayed to firmware to 
 *    instigate an DELTS management frame with an appropriate TSPEC IE.
 *
 *  @sa wlan_wmm_delts_req_ioctl
 */
typedef struct
{
    wlan_wmm_tspec_result_e commandResult;  /**< Firmware execution result */

    u8 ieeeReasonCode;             /**< IEEE reason code sent, unused for WMM */

    u8 tspecData[WMM_TSPEC_SIZE];  /**< TSPEC to send in the DELTS */

} __ATTRIB_PACK__ wlan_ioctl_wmm_delts_req_t;

/**
 *  @brief Internal command structure used in executing an ADDTS command.
 *
 *  Relay information between the IOCTL layer and the firmware command and 
 *    command response procedures.
 *
 *  @sa wlan_wmm_addts_req_ioctl
 *  @sa wlan_cmd_wmm_addts_req
 *  @sa wlan_cmdresp_wmm_addts_req
 */
typedef struct
{
    wlan_wmm_tspec_result_e commandResult;  /**< Firmware execution result */
    u32 timeout_ms;                         /**< Timeout value in milliseconds */

    u8 dialogToken;                         /**< Dialog token */
    u8 ieeeStatusCode;                      /**< IEEE status code */

    int tspecDataLen;                       /**< TSPEC data length */
    u8 tspecData[WMM_TSPEC_SIZE];           /**< TSPEC to send in the ADDTS */
    u8 addtsExtraIEBuf[WMM_ADDTS_EXTRA_IE_BYTES];   /**< ADDTS extra IE buffer */

} wlan_cmd_wmm_addts_req_t;

/**
 *  @brief Internal command structure used in executing an DELTS command.
 *
 *  Relay information between the IOCTL layer and the firmware command and 
 *    command response procedures.
 *
 *  @sa wlan_wmm_delts_req_ioctl
 *  @sa wlan_cmd_wmm_delts_req
 *  @sa wlan_cmdresp_wmm_delts_req
 */
typedef struct
{
    wlan_wmm_tspec_result_e commandResult;  /**< Firmware execution result */

    u8 dialogToken;                         /**< Dialog token */

    u8 ieeeReasonCode;                      /**< IEEE reason code sent */

    int tspecDataLen;                       /**< TSPEC data length */
    u8 tspecData[WMM_TSPEC_SIZE];           /**< TSPEC data */

} wlan_cmd_wmm_delts_req_t;

/**
 *  @brief Command structure for the HostCmd_CMD_WMM_ADDTS_REQ firmware command
 *
 */
typedef struct
{
    wlan_wmm_tspec_result_e commandResult;   /**< Command result */
    u32 timeout_ms;                          /**< Timeout value in milliseconds */

    u8 dialogToken;                                 /**< Dialog token */
    u8 ieeeStatusCode;                              /**< IEEE status code */
    u8 tspecData[WMM_TSPEC_SIZE];                   /**< TSPEC data */
    u8 addtsExtraIEBuf[WMM_ADDTS_EXTRA_IE_BYTES];   /**< ADDTS extra IE buffer */

} __ATTRIB_PACK__ HostCmd_DS_WMM_ADDTS_REQ;

/**
 *  @brief Command structure for the HostCmd_CMD_WMM_DELTS_REQ firmware command
 */
typedef struct
{
    wlan_wmm_tspec_result_e commandResult;  /**< Command result */
    u8 dialogToken;                         /**< Dialog token */
    u8 ieeeReasonCode;                      /**< IEEE reason code */
    u8 tspecData[WMM_TSPEC_SIZE];           /**< TSPEC data */

} __ATTRIB_PACK__ HostCmd_DS_WMM_DELTS_REQ;

/**
 *  @brief Enumeration for the action field in the Queue configure command
 */
typedef enum
{
    WMM_QUEUE_CONFIG_ACTION_GET = 0,
    WMM_QUEUE_CONFIG_ACTION_SET = 1,
    WMM_QUEUE_CONFIG_ACTION_DEFAULT = 2,

    WMM_QUEUE_CONFIG_ACTION_MAX
} __ATTRIB_PACK__ wlan_wmm_queue_config_action_e;

/**
 *  @brief Command structure for the HostCmd_CMD_WMM_QUEUE_CONFIG firmware cmd
 *
 *  Set/Get/Default the Queue parameters for a specific AC in the firmware.
 *
 */
typedef struct
{
    wlan_wmm_queue_config_action_e action;  /**< Set, Get, or Default */
    wlan_wmm_ac_e accessCategory;           /**< WMM_AC_BK(0) to WMM_AC_VO(3) */

    /** @brief MSDU lifetime expiry per 802.11e
     *
     *   - Ignored if 0 on a set command 
     *   - Set to the 802.11e specified 500 TUs when defaulted
     */
    u16 msduLifetimeExpiry;

    u8 tlvBuffer[WMM_QUEUE_CONFIG_EXTRA_TLV_BYTES];  /**< Not supported yet */

} __ATTRIB_PACK__ HostCmd_DS_WMM_QUEUE_CONFIG;

/**
 *  @brief Internal command structure used in executing a queue config command.
 *
 *  Relay information between the IOCTL layer and the firmware command and 
 *    command response procedures.
 *
 *  @sa wlan_wmm_queue_config_ioctl
 *  @sa wlan_cmd_wmm_queue_config
 *  @sa wlan_cmdresp_wmm_queue_config
 */
typedef struct
{
    wlan_wmm_queue_config_action_e action; /**< Set, Get, or Default */
    wlan_wmm_ac_e accessCategory;          /**< WMM_AC_BK(0) to WMM_AC_VO(3) */
    u16 msduLifetimeExpiry;                /**< lifetime expiry in TUs */

    int tlvBufLen;                                   /**< Not supported yet */
    u8 tlvBuffer[WMM_QUEUE_CONFIG_EXTRA_TLV_BYTES];  /**< Not supported yet */

} wlan_cmd_wmm_queue_config_t;

/**
 *  @brief IOCTL structure to configure a specific AC Queue's parameters
 *
 *  IOCTL structure from the application layer relayed to firmware to 
 *    get, set, or default the WMM AC queue parameters.
 *
 *  - msduLifetimeExpiry is ignored if set to 0 on a set command
 *
 *  @sa wlan_wmm_queue_config_ioctl
 */
typedef struct
{
    wlan_wmm_queue_config_action_e action;  /**< Set, Get, or Default */
    wlan_wmm_ac_e accessCategory;           /**< WMM_AC_BK(0) to WMM_AC_VO(3) */
    u16 msduLifetimeExpiry;                 /**< lifetime expiry in TUs */

    u8 supportedRates[10];                  /**< Not supported yet */

} __ATTRIB_PACK__ wlan_ioctl_wmm_queue_config_t;

/**
 *   @brief Enumeration for the action field in the queue stats command
 */
typedef enum
{
    WMM_STATS_ACTION_START = 0,
    WMM_STATS_ACTION_STOP = 1,
    WMM_STATS_ACTION_GET_CLR = 2,
    WMM_STATS_ACTION_SET_CFG = 3,  /**< Not currently used */
    WMM_STATS_ACTION_GET_CFG = 4,  /**< Not currently used */

    WMM_STATS_ACTION_MAX
} __ATTRIB_PACK__ wlan_wmm_stats_action_e;

/** Number of bins in the histogram for the HostCmd_DS_WMM_QUEUE_STATS */
#define WMM_STATS_PKTS_HIST_BINS  7

/**
 *  @brief Command structure for the HostCmd_CMD_WMM_QUEUE_STATS firmware cmd
 *
 *  Turn statistical collection on/off for a given AC or retrieve the 
 *    accumulated stats for an AC and clear them in the firmware.
 */
typedef struct
{
    wlan_wmm_stats_action_e action;  /**< Start, Stop, or Get  */
    wlan_wmm_ac_e accessCategory;    /**< WMM_AC_BK(0) to WMM_AC_VO(3) */

    u16 pktCount;      /**< Number of successful packets transmitted */
    u16 pktLoss;       /**< Packets lost; not included in pktCount */
    u32 avgQueueDelay; /**< Average Queue delay in microseconds */
    u32 avgTxDelay;    /**< Average Transmission delay in microseconds */
    u16 usedTime;      /**< Calculated used time - units of 32 microseconds */
    u16 policedTime;   /**< Calculated policed time - units of 32 microseconds */

    /** @brief Queue Delay Histogram; number of packets per queue delay range
     * 
     *  [0] -  0ms <= delay < 5ms
     *  [1] -  5ms <= delay < 10ms
     *  [2] - 10ms <= delay < 20ms
     *  [3] - 20ms <= delay < 30ms
     *  [4] - 30ms <= delay < 40ms
     *  [5] - 40ms <= delay < 50ms
     *  [6] - 50ms <= delay < msduLifetime (TUs)
     */
    u16 delayHistogram[WMM_STATS_PKTS_HIST_BINS];

    /** Reserved */
    u16 reserved_u16_1;

} __ATTRIB_PACK__ HostCmd_DS_WMM_QUEUE_STATS;

/**
 *  @brief IOCTL structure to start, stop, and get statistics for a WMM AC
 *
 *  IOCTL structure from the application layer relayed to firmware to 
 *    start or stop statistical collection for a given AC.  Also used to 
 *    retrieve and clear the collected stats on a given AC.
 *
 *  @sa wlan_wmm_queue_stats_ioctl
 */
typedef struct
{
    wlan_wmm_stats_action_e action;  /**< Start, Stop, or Get */
    wlan_wmm_ac_e accessCategory;    /**< WMM_AC_BK(0) to WMM_AC_VO(3) */
    u16 pktCount;      /**< Number of successful packets transmitted   */
    u16 pktLoss;       /**< Packets lost; not included in pktCount    */
    u32 avgQueueDelay; /**< Average Queue delay in microseconds */
    u32 avgTxDelay;    /**< Average Transmission delay in microseconds */
    u16 usedTime;      /**< Calculated used time - units of 32 microseconds */
    u16 policedTime;   /**< Calculated policed time - units of 32 microseconds */

    /** @brief Queue Delay Histogram; number of packets per queue delay range
     * 
     *  [0] -  0ms <= delay < 5ms
     *  [1] -  5ms <= delay < 10ms
     *  [2] - 10ms <= delay < 20ms
     *  [3] - 20ms <= delay < 30ms
     *  [4] - 30ms <= delay < 40ms
     *  [5] - 40ms <= delay < 50ms
     *  [6] - 50ms <= delay < msduLifetime (TUs)
     */
    u16 delayHistogram[WMM_STATS_PKTS_HIST_BINS];
} __ATTRIB_PACK__ wlan_ioctl_wmm_queue_stats_t;

/** 
 *  @brief IOCTL and command sub structure for a Traffic stream status.
 */
typedef struct
{
    u8 tid;               /**< TSID: Range: 0->7 */
    u8 valid;             /**< TSID specified is valid */
    u8 accessCategory;    /**< AC TSID is active on */
    u8 userPriority;      /**< UP specified for the TSID */

    u8 psb;               /**< Power save mode for TSID: 0 (legacy), 1 (UAPSD) */
    u8 flowDir;           /**< Upstream (0), Downlink(1), Bidirectional(3) */
    u16 mediumTime;       /**< Medium time granted for the TSID */

} __ATTRIB_PACK__ HostCmd_DS_WMM_TS_STATUS,
    wlan_ioctl_wmm_ts_status_t, wlan_cmd_wmm_ts_status_t;

/** 
 *  @brief IOCTL sub structure for a specific WMM AC Status
 */
typedef struct
{
    /** WMM Acm */
    u8 wmmAcm;
    /** Flow required flag */
    u8 flowRequired;
    /** Flow created flag */
    u8 flowCreated;
    /** Disabled flag */
    u8 disabled;
} __ATTRIB_PACK__ wlan_ioctl_wmm_queue_status_ac_t;

/**
 *  @brief IOCTL structure to retrieve the WMM AC Queue status
 *
 *  IOCTL structure from the application layer to retrieve:
 *     - ACM bit setting for the AC
 *     - Firmware status (flow required, flow created, flow disabled)
 *
 *  @sa wlan_wmm_queue_status_ioctl
 */
typedef struct
{
    /** WMM AC queue status */
    wlan_ioctl_wmm_queue_status_ac_t acStatus[MAX_AC_QUEUES];
} __ATTRIB_PACK__ wlan_ioctl_wmm_queue_status_t;

/** Firmware status for a specific AC */
typedef struct
{
    /** Disabled flag */
    u8 Disabled;
    /** Flow required flag */
    u8 FlowRequired;
    /** Flow created flag */
    u8 FlowCreated;
} WmmAcStatus_t;

/* hostcmd.h */
/*  802.11-related definitions */

/** TxPD descriptor */
typedef struct _TxPD
{
    /** Reserved */
    u32 Reserved3;
    /** Tx Control */
    u32 TxControl;
    /** Tx packet offset */
    u32 TxPktOffset;
    /** Tx packet length */
    u16 TxPktLength;
    /**Destination MAC address */
    u8 TxDestAddr[ETH_ALEN];
    /** Pkt Priority */
    u8 Priority;
    /** Trasnit Pkt Flags*/
    u8 Flags;
    /** Amount of time the packet has been queued in the driver (units = 2ms)*/
    u8 PktDelay_2ms;
    /** Reserved */
    u8 Reserved1;

} __ATTRIB_PACK__ TxPD, *PTxPD;

/** RxPD Descriptor */
typedef struct _RxPD
{
        /** Reserved */
    u16 Reserved2;

        /** SNR */
    u8 SNR;

        /** Rx Control */
    u8 RxControl;

        /** Rx Packet Length */
    u16 RxPktLength;

        /** Noise Floor */
    u8 NF;

        /** Rx Packet Rate */
    u8 RxRate;

        /** Rx Pkt offset */
    u32 RxPktOffset;
        /** Received packet type */
    u8 RxPktType;
        /** Reserved */
    u8 Reserved_1[3];
        /** Pkt Priority */
    u8 Priority;
        /** Reserved */
    u8 Reserved[3];

} __ATTRIB_PACK__ RxPD, *PRxPD;

#if defined(__KERNEL__)

/** CmdCtrlNode */
typedef struct _CmdCtrlNode
{
        /** CMD link list*/
    struct list_head list;

    u32 Status;

        /** CMD ID*/
    WLAN_OID cmd_oid;

        /** CMD wait option: wait for finish or no wait*/
    u16 wait_option;

        /** command parameter*/
    void *pdata_buf;

        /** command data*/
    u8 *BufVirtualAddr;

    u16 CmdFlags;

        /** wait queue*/
    u16 CmdWaitQWoken;
    wait_queue_head_t cmdwait_q __ATTRIB_ALIGN__;
} __ATTRIB_PACK__ CmdCtrlNode, *PCmdCtrlNode;

#endif

/** MRVL_WEP_KEY */
typedef struct _MRVL_WEP_KEY
{
    /** Length */
    u32 Length;
    /** WEP key index */
    u32 KeyIndex;
    /** WEP key length */
    u32 KeyLength;
    /** WEP keys */
    u8 KeyMaterial[MRVL_KEY_BUFFER_SIZE_IN_BYTE];
} __ATTRIB_PACK__ MRVL_WEP_KEY, *PMRVL_WEP_KEY;

/** Unsigned long long */
typedef ULONGLONG WLAN_802_11_KEY_RSC;

/** WLAN_802_11_KEY */
typedef struct _WLAN_802_11_KEY
{
    /** Length */
    u32 Length;
    /** Key index */
    u32 KeyIndex;
    /** Key length */
    u32 KeyLength;
    /** BSSID */
    WLAN_802_11_MAC_ADDRESS BSSID;
    /** Key RSC */
    WLAN_802_11_KEY_RSC KeyRSC;
    /** Key */
    u8 KeyMaterial[MRVL_MAX_KEY_WPA_KEY_LENGTH];
} __ATTRIB_PACK__ WLAN_802_11_KEY;

/** MRVL_WPA_KEY */
typedef struct _MRVL_WPA_KEY
{
    /** Key index */
    u32 KeyIndex;
    /** Key length */
    u32 KeyLength;
    /** Key RSC */
    u32 KeyRSC;
    /** Key material */
    u8 KeyMaterial[MRVL_MAX_KEY_WPA_KEY_LENGTH];
} MRVL_WPA_KEY, *PMRVL_WPA_KEY;

/** MRVL_WLAN_WPA_KEY */
typedef struct _MRVL_WLAN_WPA_KEY
{
    /** Encryption key */
    u8 EncryptionKey[16];
    /** MIC key 1 */
    u8 MICKey1[8];
    /** MIC key 2 */
    u8 MICKey2[8];
} MRVL_WLAN_WPA_KEY, *PMRVL_WLAN_WPA_KEY;

/** Received Signal Strength Indication  in dBm*/
typedef LONG WLAN_802_11_RSSI;

/** WLAN_802_11_WEP */
typedef struct _WLAN_802_11_WEP
{
        /** Length of this structure */
    u32 Length;

        /** 0 is the per-client key, 1-N are the global keys */
    u32 KeyIndex;

        /** length of key in bytes */
    u32 KeyLength;

        /** variable length depending on above field */
    u8 KeyMaterial[1];
} __ATTRIB_PACK__ WLAN_802_11_WEP;

/** WLAN_802_11_SSID */
typedef struct _WLAN_802_11_SSID
{
        /** SSID Length*/
    u32 SsidLength;

        /** SSID information field */
    u8 Ssid[MRVDRV_MAX_SSID_LENGTH];
} __ATTRIB_PACK__ WLAN_802_11_SSID;

/** Framentation threshold */
typedef u32 WLAN_802_11_FRAGMENTATION_THRESHOLD;
/** RTS threshold */
typedef u32 WLAN_802_11_RTS_THRESHOLD;
/** 802.11 antenna */
typedef u32 WLAN_802_11_ANTENNA;

/** wlan_offset_value */
typedef struct _wlan_offset_value
{
    /** Offset */
    u32 offset;
    /** Value */
    u32 value;
} wlan_offset_value;

/** WLAN_802_11_FIXED_IEs */
typedef struct _WLAN_802_11_FIXED_IEs
{
    /** Timestamp */
    u8 Timestamp[8];
    /** Beacon interval */
    u16 BeaconInterval;
    /** Capabilities*/
    u16 Capabilities;
} WLAN_802_11_FIXED_IEs;

/** WLAN_802_11_VARIABLE_IEs */
typedef struct _WLAN_802_11_VARIABLE_IEs
{
    /** Element ID */
    u8 ElementID;
    /** Length */
    u8 Length;
    /** IE data */
    u8 data[1];
} WLAN_802_11_VARIABLE_IEs;

/** WLAN_802_11_AI_RESFI */
typedef struct _WLAN_802_11_AI_RESFI
{
    /** Capabilities */
    u16 Capabilities;
    /** Status code */
    u16 StatusCode;
    /** Association ID */
    u16 AssociationId;
} WLAN_802_11_AI_RESFI;

/** WLAN_802_11_AI_REQFI */
typedef struct _WLAN_802_11_AI_REQFI
{
    /** Capabilities */
    u16 Capabilities;
    /** Listen interval */
    u16 ListenInterval;
    /** Current AP address */
    WLAN_802_11_MAC_ADDRESS CurrentAPAddress;
} WLAN_802_11_AI_REQFI;

/* Define general data structure */
/** HostCmd_DS_GEN */
typedef struct _HostCmd_DS_GEN
{
    /** Command */
    u16 Command;
    /** Size */
    u16 Size;
    /** Sequence number */
    u16 SeqNum;
    /** Result */
    u16 Result;
} __ATTRIB_PACK__ HostCmd_DS_GEN, HostCmd_DS_802_11_DEEP_SLEEP;

/** Size of HostCmd_DS_GEN */
#define S_DS_GEN    sizeof(HostCmd_DS_GEN)
/**
 * Define data structure for HostCmd_CMD_GET_HW_SPEC
 * This structure defines the response for the GET_HW_SPEC command
 */
/** HostCmd_DS_GET_HW_SPEC */
typedef struct _HostCmd_DS_GET_HW_SPEC
{
        /** HW Interface version number */
    u16 HWIfVersion;

        /** HW version number */
    u16 Version;

        /** Max number of TxPD FW can handle*/
    u16 NumOfTxPD;

        /** Max no of Multicast address  */
    u16 NumOfMCastAdr;

        /** MAC address */
    u8 PermanentAddr[ETH_ALEN];

        /** Region Code */
    u16 RegionCode;

        /** Number of antenna used */
    u16 NumberOfAntenna;

        /** FW release number, example 0x1234=1.2.3.4 */
    u32 FWReleaseNumber;

    /** Reserved field */
    u32 Reserved_1;

    /** Reserved field */
    u32 Reserved_2;

    /** Reserved field */
    u32 Reserved_3;

        /** FW/HW Capability */
    u32 fwCapInfo;
} __ATTRIB_PACK__ HostCmd_DS_GET_HW_SPEC;

typedef struct _HostCmd_DS_802_11_SUBSCRIBE_EVENT
{
    /** Action */
    u16 Action;
    /** Events */
    u16 Events;
} __ATTRIB_PACK__ HostCmd_DS_802_11_SUBSCRIBE_EVENT;

/** 
 * This scan handle Country Information IE(802.11d compliant) 
 * Define data structure for HostCmd_CMD_802_11_SCAN 
 */
/** HostCmd_DS_802_11_SCAN */
typedef struct _HostCmd_DS_802_11_SCAN
{
    /** BSS type */
    u8 BSSType;
    /** BSSID */
    u8 BSSID[ETH_ALEN];
    /** TLV buffer */
    u8 TlvBuffer[1];
        /** MrvlIEtypes_SsIdParamSet_t 	SsIdParamSet; 
	 * MrvlIEtypes_ChanListParamSet_t	ChanListParamSet;
	 * MrvlIEtypes_RatesParamSet_t 	OpRateSet; 
	 */
} __ATTRIB_PACK__ HostCmd_DS_802_11_SCAN;

typedef struct _HostCmd_DS_802_11_SCAN_RSP
{
    /** Size of BSS descriptor */
    u16 BSSDescriptSize;
    /** Numner of sets */
    u8 NumberOfSets;
    /** BSS descriptor and TLV buffer */
    u8 BssDescAndTlvBuffer[1];

} __ATTRIB_PACK__ HostCmd_DS_802_11_SCAN_RSP;

/** HostCmd_CMD_802_11_GET_LOG */
typedef struct _HostCmd_DS_802_11_GET_LOG
{
    /** Number of multicast transmitted frames */
    u32 mcasttxframe;
    /** Number of failures */
    u32 failed;
    /** Number of retries */
    u32 retry;
    /** Number of multiretries */
    u32 multiretry;
    /** Number of duplicate frames */
    u32 framedup;
    /** Number of RTS success */
    u32 rtssuccess;
    /** Number of RTS failure */
    u32 rtsfailure;
    /** Number of acknowledgement failure */
    u32 ackfailure;
    /** Number of fragmented packets received */
    u32 rxfrag;
    /** Number of multicast frames received */
    u32 mcastrxframe;
    /** FCS error */
    u32 fcserror;
    /** Number of transmitted frames */
    u32 txframe;
    /** Reserved field */
    u32 reserved;
} __ATTRIB_PACK__ HostCmd_DS_802_11_GET_LOG;

/**  HostCmd_CMD_MAC_CONTROL */
typedef struct _HostCmd_DS_MAC_CONTROL
{
    /** Action */
    u16 Action;
    /** Reserved field */
    u16 Reserved;
} __ATTRIB_PACK__ HostCmd_DS_MAC_CONTROL;

/**  HostCmd_CMD_MAC_MULTICAST_ADR */
typedef struct _HostCmd_DS_MAC_MULTICAST_ADR
{
    /** Action */
    u16 Action;
    /** Number of addresses */
    u16 NumOfAdrs;
    /** List of MAC */
    u8 MACList[ETH_ALEN * MRVDRV_MAX_MULTICAST_LIST_SIZE];
} __ATTRIB_PACK__ HostCmd_DS_MAC_MULTICAST_ADR;

/**  HostCmd_CMD_802_11_DEAUTHENTICATE */
typedef struct _HostCmd_DS_802_11_DEAUTHENTICATE
{
    /** MAC address */
    u8 MacAddr[ETH_ALEN];
    /** Deauthentication resaon code */
    u16 ReasonCode;
} __ATTRIB_PACK__ HostCmd_DS_802_11_DEAUTHENTICATE;

/** HostCmd_DS_802_11_ASSOCIATE */
typedef struct _HostCmd_DS_802_11_ASSOCIATE
{
    /** Peer STA address */
    u8 PeerStaAddr[ETH_ALEN];
    /** Capability information */
    IEEEtypes_CapInfo_t CapInfo;
    /** Listen interval */
    u16 ListenInterval;
    /** Reserved field */
    u8 Reserved1[3];

    /**
     *  MrvlIEtypes_SsIdParamSet_t  SsIdParamSet;
     *  MrvlIEtypes_PhyParamSet_t   PhyParamSet;
     *  MrvlIEtypes_SsParamSet_t    SsParamSet;
     *  MrvlIEtypes_RatesParamSet_t RatesParamSet;
     */
} __ATTRIB_PACK__ HostCmd_DS_802_11_ASSOCIATE;

/** HostCmd_CMD_802_11_ASSOCIATE response */
typedef struct
{
    /** Association response structure */
    IEEEtypes_AssocRsp_t assocRsp;
} __ATTRIB_PACK__ HostCmd_DS_802_11_ASSOCIATE_RSP;

/**  HostCmd_CMD_802_11_AD_HOC_START response */
typedef struct _HostCmd_DS_802_11_AD_HOC_RESULT
{
    /** Padding */
    u8 PAD[3];
    /** AdHoc BSSID */
    u8 BSSID[ETH_ALEN];
} __ATTRIB_PACK__ HostCmd_DS_802_11_AD_HOC_RESULT;

/**  HostCmd_CMD_802_11_SET_WEP */
typedef struct _HostCmd_DS_802_11_SET_WEP
{
        /** ACT_ADD, ACT_REMOVE or ACT_ENABLE  */
    u16 Action;

        /** Key Index selected for Tx */
    u16 KeyIndex;

    /* 40, 128bit or TXWEP */
    /** Type of WEP for key 1 */
    u8 WEPTypeForKey1;

    /** Type of WEP for key 2 */
    u8 WEPTypeForKey2;
    /** Type of WEP for key 3 */
    u8 WEPTypeForKey3;
    /** Type of WEP for key 4 */
    u8 WEPTypeForKey4;
    /** WEP key 1 */
    u8 WEP1[16];
    /** WEP key 2 */
    u8 WEP2[16];
    /** WEP key 3 */
    u8 WEP3[16];
    /** WEP key 4 */
    u8 WEP4[16];
} __ATTRIB_PACK__ HostCmd_DS_802_11_SET_WEP;

/** HostCmd_DS_802_11_AD_HOC_STOP */
typedef struct _HostCmd_DS_802_11_AD_HOC_STOP
{

} __ATTRIB_PACK__ HostCmd_DS_802_11_AD_HOC_STOP;

/**  HostCmd_CMD_802_11_SNMP_MIB */
typedef struct _HostCmd_DS_802_11_SNMP_MIB
{
    /** SNMP query type */
    u16 QueryType;
    /** SNMP object ID */
    u16 OID;
    /** SNMP buffer size */
    u16 BufSize;
    /** Value */
    u8 Value[128];
} __ATTRIB_PACK__ HostCmd_DS_802_11_SNMP_MIB;

/** HostCmd_CMD_MAC_REG_ACCESS */
typedef struct _HostCmd_DS_MAC_REG_ACCESS
{
    /** Action */
    u16 Action;
    /** MAC register offset */
    u16 Offset;
    /** MAC register value */
    u32 Value;
} __ATTRIB_PACK__ HostCmd_DS_MAC_REG_ACCESS;

/** HostCmd_CMD_BBP_REG_ACCESS */
typedef struct _HostCmd_DS_BBP_REG_ACCESS
{
    /** Acion */
    u16 Action;
    /** BBP register offset */
    u16 Offset;
    /** BBP register value */
    u8 Value;
    /** Reserved field */
    u8 Reserved[3];
} __ATTRIB_PACK__ HostCmd_DS_BBP_REG_ACCESS;

/**  HostCmd_CMD_RF_REG_ACCESS */
typedef struct _HostCmd_DS_RF_REG_ACCESS
{
    /** Action */
    u16 Action;
    /** RF register offset */
    u16 Offset;
    /** RF register value */
    u8 Value;
    /** Reserved field */
    u8 Reserved[3];
} __ATTRIB_PACK__ HostCmd_DS_RF_REG_ACCESS;

/**  HostCmd_CMD_PMIC_REG_ACCESS */
typedef struct _HostCmd_DS_PMIC_REG_ACCESS
{
    /** Action */
    u16 Action;
    /** RF register offset */
    u16 Offset;
    /** RF register value */
    u8 Value;
    /** Reserved field */
    u8 Reserved[3];
} __ATTRIB_PACK__ HostCmd_DS_PMIC_REG_ACCESS;

/** HostCmd_CMD_802_11_RADIO_CONTROL */
typedef struct _HostCmd_DS_802_11_RADIO_CONTROL
{
    /** Action */
    u16 Action;
    /** Control */
    u16 Control;
} __ATTRIB_PACK__ HostCmd_DS_802_11_RADIO_CONTROL;

/* HostCmd_DS_802_11_SLEEP_PARAMS */
typedef struct _HostCmd_DS_802_11_SLEEP_PARAMS
{
        /** ACT_GET/ACT_SET */
    u16 Action;

        /** Sleep clock error in ppm */
    u16 Error;

        /** Wakeup offset in usec */
    u16 Offset;

        /** Clock stabilization time in usec */
    u16 StableTime;

        /** Control periodic calibration */
    u8 CalControl;

        /** Control the use of external sleep clock */
    u8 ExternalSleepClk;

        /** Reserved field, should be set to zero */
    u16 Reserved;
} __ATTRIB_PACK__ HostCmd_DS_802_11_SLEEP_PARAMS;

/* HostCmd_DS_802_11_SLEEP_PERIOD */
typedef struct _HostCmd_DS_802_11_SLEEP_PERIOD
{
        /** ACT_GET/ACT_SET */
    u16 Action;

        /** Sleep Period in msec */
    u16 Period;
} __ATTRIB_PACK__ HostCmd_DS_802_11_SLEEP_PERIOD;

/* HostCmd_DS_802_11_BCA_TIMESHARE */
typedef struct _HostCmd_DS_802_11_BCA_TIMESHARE
{
    /** ACT_GET/ACT_SET */
    u16 Action;

    /** Type: WLAN, BT */
    u16 TrafficType;

    /** 20msec - 60000msec */
    u32 TimeShareInterval;

    /** PTA arbiter time in msec */
    u32 BTTime;
} __ATTRIB_PACK__ HostCmd_DS_802_11_BCA_TIMESHARE;

/** HostCmd_CMD_802_11_RF_CHANNEL */
typedef struct _HostCmd_DS_802_11_RF_CHANNEL
{
    /** Action */
    u16 Action;
    /** Current channel */
    u16 CurrentChannel;
    /** RF type */
    u16 RFType;
    /** Reserved field */
    u16 Reserved;
    /** List of channels */
    u8 ChannelList[32];
} __ATTRIB_PACK__ HostCmd_DS_802_11_RF_CHANNEL;

/**  HostCmd_CMD_802_11_RSSI */
typedef struct _HostCmd_DS_802_11_RSSI_INFO
{
        /** Action */
    u16 Action;
    /** Parameter used for exponential averaging for Data */
    u16 Ndata;
    /** Parameter used for exponential averaging for Data */
    u16 Nbcn;
    /** Reserved field 0 */
    u16 Reserved[9];
    /** Reserved field 1 */
    u64 Reserved_1;
} __ATTRIB_PACK__ HostCmd_DS_802_11_RSSI_INFO;

/** HostCmd_DS_802_11_RSSI_INFO_RSP */
typedef struct _HostCmd_DS_802_11_RSSI_INFO_RSP
{
    /** Action */
    u16 Action;
    /** Parameter used for exponential averaging for Data */
    u16 Ndata;
    /** Parameter used for exponential averaging for Data */
    u16 Nbcn;
    /** Last Data RSSI in dBm */
    s16 DataRSSIlast;
    /** Last Data NF in dBm */
    s16 DataNFlast;
    /** AVG DATA RSSI in dBm */
    s16 DataRSSIAvg;
    /** AVG DATA NF in dBm */
    s16 DataNFAvg;
    /**	Last BEACON RSSI in dBm */
    s16 BcnRSSIlast;
    /**	Last BEACON NF in dBm */
    s16 BcnNFlast;
    /** AVG BEACON RSSI in dBm */
    s16 BcnRSSIAvg;
    /** AVG BEACON NF in dBm */
    s16 BcnNFAvg;
    /** Last RSSI Beacon TSF */
    u64 TSFbcn;
} __ATTRIB_PACK__ HostCmd_DS_802_11_RSSI_INFO_RSP;

/** HostCmd_DS_802_11_MAC_ADDRESS */
typedef struct _HostCmd_DS_802_11_MAC_ADDRESS
{
    /** Action */
    u16 Action;
    /** MAC address */
    u8 MacAdd[ETH_ALEN];
} __ATTRIB_PACK__ HostCmd_DS_802_11_MAC_ADDRESS;

/** HostCmd_CMD_802_11_RF_TX_POWER */
typedef struct _HostCmd_DS_802_11_RF_TX_POWER
{
    /** Action */
    u16 Action;
    /** Current power level */
    u16 CurrentLevel;
    /** Maximum power */
    u8 MaxPower;
    /** Minimum power */
    u8 MinPower;
} __ATTRIB_PACK__ HostCmd_DS_802_11_RF_TX_POWER;

/** HostCmd_CMD_802_11_RF_ANTENNA */
typedef struct _HostCmd_DS_802_11_RF_ANTENNA
{
    /** Action */
    u16 Action;

    /**  Number of antennas or 0xffff(diversity) */
    u16 AntennaMode;

} __ATTRIB_PACK__ HostCmd_DS_802_11_RF_ANTENNA;

/** HostCmd_CMD_802_11_PS_MODE */
typedef struct _HostCmd_DS_802_11_PS_MODE
{
    /** Action */
    u16 Action;
    /** NULL packet interval */
    u16 NullPktInterval;
    /** Multiple DTIM */
    u16 MultipleDtim;
    /** Beacon miss timeout */
    u16 BCNMissTimeOut;
    /** Local listen interval */
    u16 LocalListenInterval;
    /** AdHoc awake period */
    u16 AdhocAwakePeriod;
} __ATTRIB_PACK__ HostCmd_DS_802_11_PS_MODE;

/** PS_CMD_ConfirmSleep */
typedef struct _PS_CMD_ConfirmSleep
{
    /** Command */
    u16 Command;
    /** Size */
    u16 Size;
    /** Sequence number */
    u16 SeqNum;
    /** Result */
    u16 Result;

    /** Action */
    u16 Action;
    /** Reserved */
    u16 Reserved1;
    /** Multiple DTIM */
    u16 MultipleDtim;
    /** Reserved */
    u16 Reserved;
    /** Local listen interval */
    u16 LocalListenInterval;
} __ATTRIB_PACK__ PS_CMD_ConfirmSleep, *PPS_CMD_ConfirmSleep;

/** HostCmd_CMD_802_11_FW_WAKE_METHOD */
typedef struct _HostCmd_DS_802_11_FW_WAKEUP_METHOD
{
    /** Action */
    u16 Action;
    /** Method */
    u16 Method;
} __ATTRIB_PACK__ HostCmd_DS_802_11_FW_WAKEUP_METHOD;

/** HostCmd_DS_802_11_RATE_ADAPT_RATESET */
typedef struct _HostCmd_DS_802_11_RATE_ADAPT_RATESET
{
    /** Action */
    u16 Action;
    /** Hardware drop rate mode */
    u16 HWRateDropMode;
    /** Rate bitmap */
    u16 Bitmap;
    /** Rate adapt threshold */
    u16 Threshold;
    /** Final rate */
    u16 FinalRate;
} __ATTRIB_PACK__ HostCmd_DS_802_11_RATE_ADAPT_RATESET;

/** HostCmd_DS_802_11_AD_HOC_START*/
typedef struct _HostCmd_DS_802_11_AD_HOC_START
{
    /** AdHoc SSID */
    u8 SSID[MRVDRV_MAX_SSID_LENGTH];
    /** BSS type */
    u8 BSSType;
    /** Beacon period */
    u16 BeaconPeriod;
    /** DTIM period */
    u8 DTIMPeriod;
    /** SS parameter set */
    IEEEtypes_SsParamSet_t SsParamSet;
    /** PHY parameter set */
    IEEEtypes_PhyParamSet_t PhyParamSet;
    /** Reserved field */
    u16 Reserved1;
    /** Capability information */
    IEEEtypes_CapInfo_t Cap;
    /** Supported data rates */
    u8 DataRate[HOSTCMD_SUPPORTED_RATES];
} __ATTRIB_PACK__ HostCmd_DS_802_11_AD_HOC_START;

/** AdHoc_BssDesc_t */
typedef struct _AdHoc_BssDesc_t
{
    /** BSSID */
    u8 BSSID[ETH_ALEN];
    /** SSID */
    u8 SSID[MRVDRV_MAX_SSID_LENGTH];
    /** BSS type */
    u8 BSSType;
    /** Beacon period */
    u16 BeaconPeriod;
    /** DTIM period */
    u8 DTIMPeriod;
    /** Tiemstamp */
    u8 TimeStamp[8];
    /** Local time */
    u8 LocalTime[8];
    /** PHY parameter set */
    IEEEtypes_PhyParamSet_t PhyParamSet;
    /** SS parameter set */
    IEEEtypes_SsParamSet_t SsParamSet;
    /** Capability information */
    IEEEtypes_CapInfo_t Cap;
    /** Supported data rates */
    u8 DataRates[HOSTCMD_SUPPORTED_RATES];

        /** DO NOT ADD ANY FIELDS TO THIS STRUCTURE.	 It is used below in the
	 *	Adhoc join command and will cause a binary layout mismatch with 
	 *	the firmware 
	 */
} __ATTRIB_PACK__ AdHoc_BssDesc_t;

/** HostCmd_DS_802_11_AD_HOC_JOIN */
typedef struct _HostCmd_DS_802_11_AD_HOC_JOIN
{
    /** AdHoc BSS descriptor */
    AdHoc_BssDesc_t BssDescriptor;
    /** Reserved field */
    u16 Reserved1;
    /** Reserved field */
    u16 Reserved2;

} __ATTRIB_PACK__ HostCmd_DS_802_11_AD_HOC_JOIN;

typedef union _KeyInfo_WEP_t
{
    /** Reserved */
    u8 Reserved;

    /** bits 1-4: Specifies the index of key */
    u8 WepKeyIndex;

    /** bit 0: Specifies that this key is 
     * to be used as the default for TX data packets 
     */
    u8 isWepDefaultKey;
} __ATTRIB_PACK__ KeyInfo_WEP_t;

typedef union _KeyInfo_TKIP_t
{
    /** Reserved */
    u8 Reserved;

    /** bit 2: Specifies that this key is 
     * enabled and valid to use */
    u8 isKeyEnabled;

    /** bit 1: Specifies that this key is
     * to be used as the unicast key */
    u8 isUnicastKey;

    /** bit 0: Specifies that this key is 
     * to be used as the multicast key */
    u8 isMulticastKey;
} __ATTRIB_PACK__ KeyInfo_TKIP_t;

typedef union _KeyInfo_AES_t
{
    /** Reserved */
    u8 Reserved;

    /** bit 2: Specifies that this key is
     * enabled and valid to use */
    u8 isKeyEnabled;

    /** bit 1: Specifies that this key is
     * to be used as the unicast key */
    u8 isUnicastKey;

    /** bit 0: Specifies that this key is 
     * to be used as the multicast key */
    u8 isMulticastKey;
} __ATTRIB_PACK__ KeyInfo_AES_t;

/** KeyMaterial_TKIP_t */
typedef struct _KeyMaterial_TKIP_t
{
    /** TKIP encryption/decryption key */
    u8 TkipKey[16];

    /** TKIP TX MIC Key */
    u8 TkipTxMicKey[16];

    /** TKIP RX MIC Key */
    u8 TkipRxMicKey[16];
} __ATTRIB_PACK__ KeyMaterial_TKIP_t, *PKeyMaterial_TKIP_t;

/** KeyMaterial_AES_t */
typedef struct _KeyMaterial_AES_t
{
    /** AES encryption/decryption key */
    u8 AesKey[16];
} __ATTRIB_PACK__ KeyMaterial_AES_t, *PKeyMaterial_AES_t;

/** MrvlIEtype_KeyParamSet_t */
typedef struct _MrvlIEtype_KeyParamSet_t
{
    /** Type ID */
    u16 Type;

    /** Length of Payload */
    u16 Length;

    /** Type of Key: WEP=0, TKIP=1, AES=2 */
    u16 KeyTypeId;

    /** Key Control Info specific to a KeyTypeId */
    u16 KeyInfo;

    /** Length of key */
    u16 KeyLen;

    /** Key material of size KeyLen */
    u8 Key[32];
} __ATTRIB_PACK__ MrvlIEtype_KeyParamSet_t, *PMrvlIEtype_KeyParamSet_t;

/** HostCmd_DS_802_11_KEY_MATERIAL */
typedef struct _HostCmd_DS_802_11_KEY_MATERIAL
{
    /** Action */
    u16 Action;

    /** Key parameter set */
    MrvlIEtype_KeyParamSet_t KeyParamSet;
} __ATTRIB_PACK__ HostCmd_DS_802_11_KEY_MATERIAL;

/** HostCmd_DS_802_11_HOST_SLEEP_CFG */
typedef struct _HostCmd_DS_HOST_802_11_HOST_SLEEP_CFG
{
       /** bit0=1: non-unicast data
        * bit1=1: unicast data
        * bit2=1: mac events
     	* bit3=1: magic packet 
	*/
    u32 conditions;

    /** GPIO */
    u8 gpio;

        /** in milliseconds */
    u8 gap;
} __ATTRIB_PACK__ HostCmd_DS_802_11_HOST_SLEEP_CFG;

typedef struct _HostCmd_DS_802_11_CFG_DATA
{
    /** Action */
    u16 u16Action;
    /** Type */
    u16 u16Type;
    /** Data length */
    u16 u16DataLen;
    /** Data */
    u8 u8Data[1];
} __ATTRIB_PACK__ HostCmd_DS_802_11_CFG_DATA, *pHostCmd_DS_802_11_CFG_DATA;

/** Maximum length of EEPROM content */
#define MAX_EEPROM_LEN	20

/** HostCmd_DS_802_11_EEPROM_ACCESS */
typedef struct _HostCmd_DS_802_11_EEPROM_ACCESS
{
    /** Action */
    u16 Action;

   /** multiple 4 */
    u16 Offset;
    /** Number of bytes */
    u16 ByteCount;
    /** Value */
    u8 Value;
} __ATTRIB_PACK__ HostCmd_DS_802_11_EEPROM_ACCESS;

/** HostCmd_DS_802_11_BG_SCAN_CONFIG */
typedef struct _HostCmd_DS_802_11_BG_SCAN_CONFIG
{
    /** Action */
    u8 Action;

    /** Configuration type */
    u8 ConfigType;

    /** 
     *  Enable/Disable
     *  0 - Disable
     *  1 - Enable 
     */
    u8 Enable;

    /**
     *  bssType
     *  1 - Infrastructure
     *  2 - IBSS
     *  3 - any 
     */
    u8 BssType;

    /** 
     * ChannelsPerScan 
     *   Number of channels to scan during a single scanning opportunity
     */
    u8 ChannelsPerScan;

    /** Reserved */
    u8 Reserved1[3];

    /** ScanInterval */
    u32 ScanInterval;

    /** Reserved */
    u8 Reserved2[4];

    /** 
     * ReportConditions
     * - SSID Match
     * - Exceed SNR Threshold
     * - Exceed RSSI Threshold
     * - Complete all channels
     */
    u32 ReportConditions;

    /** Reserved */
    u8 Reserved3[2];

    /**  Attach TLV based parameters as needed:
     *
     *  MrvlIEtypes_SsIdParamSet_t          Set specific SSID filter
     *  MrvlIEtypes_ChanListParamSet_t      Set the channels & channel params
     *  MrvlIEtypes_NumProbes_t             Number of probes per SSID/broadcast
     *  MrvlIEtypes_WildCardSsIdParamSet_t  Wildcard SSID matching patterns
     *  MrvlIEtypes_SnrThreshold_t          SNR Threshold for match/report  
     */

} __ATTRIB_PACK__ HostCmd_DS_802_11_BG_SCAN_CONFIG;

/** HostCmd_DS_802_11_BG_SCAN_QUERY */
typedef struct _HostCmd_DS_802_11_BG_SCAN_QUERY
{
    /** Flush */
    u8 Flush;
} __ATTRIB_PACK__ HostCmd_DS_802_11_BG_SCAN_QUERY;

/** HostCmd_DS_802_11_BG_SCAN_QUERY_RSP */
typedef struct _HostCmd_DS_802_11_BG_SCAN_QUERY_RSP
{
    /** Report condition */
    u32 ReportCondition;
    /** Scan response */
    HostCmd_DS_802_11_SCAN_RSP scanresp;
} __ATTRIB_PACK__ HostCmd_DS_802_11_BG_SCAN_QUERY_RSP;

/** HostCmd_DS_802_11_TPC_CFG */
typedef struct _HostCmd_DS_802_11_TPC_CFG
{
    /** Action */
    u16 Action;
    /** Enable flag */
    u8 Enable;
    /** P0 value */
    s8 P0;
    /** P1 value */
    s8 P1;
    /** P2 value */
    s8 P2;
    /** SNR */
    u8 UseSNR;
} __ATTRIB_PACK__ HostCmd_DS_802_11_TPC_CFG;

/** HostCmd_DS_802_11_LED_CTRL */
typedef struct _HostCmd_DS_802_11_LED_CTRL
{
    u16 Action;                                 /**< 0 = ACT_GET; 1 = ACT_SET; */
    u16 LedNums;                                 /**< Numbers of LEDs supported */
    MrvlIEtypes_LedGpio_t LedGpio;                      /**< LED GPIO */
    MrvlIEtypes_LedBehavior_t LedBehavior[1];           /**< LED behavior */
} __ATTRIB_PACK__ HostCmd_DS_802_11_LED_CTRL;

/** HostCmd_DS_802_11_POWER_ADAPT_CFG_EXT */
typedef struct _HostCmd_DS_802_11_POWER_ADAPT_CFG_EXT
{
        /** Action */
    u16 Action;                 /* 0 = ACT_GET; 1 = ACT_SET; */
    /** Enable power adapt */
    u16 EnablePA;               /* 0 = disable; 1 = enable; */
    /** Power adapt group */
    MrvlIEtypes_PowerAdapt_Group_t PowerAdaptGroup;
} __ATTRIB_PACK__ HostCmd_DS_802_11_POWER_ADAPT_CFG_EXT;

typedef struct _HostCmd_DS_SDIO_PULL_CTRL
{
    u16 Action;                 /**< 0: get; 1: set*/
    u16 PullUp;                 /**< the delay of pulling up in us */
    u16 PullDown;               /**< the delay of pulling down in us */
} __ATTRIB_PACK__ HostCmd_DS_SDIO_PULL_CTRL;
typedef struct _HostCmd_DS_802_11_IBSS_Status
{
    /** Action */
    u16 Action;
    /** Enable */
    u16 Enable;
    /** BSSID */
    u8 BSSID[ETH_ALEN];
    /** Beacon interval */
    u16 BeaconInterval;
    /** ATIM window interval */
    u16 ATIMWindow;
    /** User G rate protection */
    u16 UseGRateProtection;
} __ATTRIB_PACK__ HostCmd_DS_802_11_IBSS_Status;

/** RC4 cipher test */
#define CIPHER_TEST_RC4 (1)
/** AES cipher test */
#define CIPHER_TEST_AES (2)
/** AES key wrap cipher test */
#define CIPHER_TEST_AES_KEY_WRAP (3)
/** AES CCM cipher test */
#define CIPHER_TEST_AES_CCM (4)

/** HostCmd_DS_802_11_CRYPTO */
typedef struct _HostCmd_DS_802_11_CRYPTO
{
    u16 EncDec;                 /**< Decrypt=0, Encrypt=1 */
    u16 Algorithm;              /**<  RC4=1 AES=2 , AES_KEY_WRAP=3 */
    u16 KeyIVLength;            /**< Length of Key IV (bytes)	*/
    u8 KeyIV[32];               /**< Key IV */
    u16 KeyLength;              /**< Length of Key (bytes) */
    u8 Key[32];                 /**< Key */
        /** MrvlIEtypes_Data_t 	data  Plain text if EncDec=Encrypt, Ciphertext data if EncDec=Decrypt*/
} __ATTRIB_PACK__ HostCmd_DS_802_11_CRYPTO;
/** HostCmd_DS_802_11_CRYPTO_AES_CCM */
typedef struct _HostCmd_DS_802_11_CRYPTO_AES_CCM
{
    u16 EncDec;                 /**< Decrypt=0, Encrypt=1 */
    u16 Algorithm;              /**<  AES-CCM=4 */
    u16 KeyLength;              /**< Length of Key (bytes)	*/
    u8 Key[32];                 /**< Key  */
    u16 NonceLength;                    /**< Length of Nonce (bytes) */
    u8 Nonce[14];               /**< Nonce */
    u16 AADLength;              /**< Length of Nonce (bytes) */
    u8 AAD[32];                 /**< Nonce */
        /** MrvlIEtypes_Data_t 	data  Plain text if EncDec=Encrypt, Ciphertext data if EncDec=Decrypt*/
} __ATTRIB_PACK__ HostCmd_DS_802_11_CRYPTO_AES_CCM;

typedef struct _HostCmd_TX_RATE_QUERY
{
    /** Tx rate */
    u16 TxRate;
} __ATTRIB_PACK__ HostCmd_TX_RATE_QUERY;

/** HostCmd_DS_802_11_AUTO_TX */
typedef struct _HostCmd_DS_802_11_AUTO_TX
{
        /** Action */
    u16 Action;                 /* 0 = ACT_GET; 1 = ACT_SET; */
    MrvlIEtypes_AutoTx_t AutoTx;            /**< Auto Tx */
} __ATTRIB_PACK__ HostCmd_DS_802_11_AUTO_TX;

/** HostCmd_MEM_ACCESS */
typedef struct _HostCmd_DS_MEM_ACCESS
{
        /** Action */
    u16 Action;                 /* 0 = ACT_GET; 1 = ACT_SET; */
    /** Reserved field */
    u16 Reserved;
    /** Address */
    u32 Addr;
    /** Value */
    u32 Value;
} __ATTRIB_PACK__ HostCmd_DS_MEM_ACCESS;

typedef struct
{
    /** TSF value structure */
    u64 TsfValue;
} __ATTRIB_PACK__ HostCmd_DS_GET_TSF;

typedef struct _HostCmd_DS_ECL_SYSTEM_CLOCK_CONFIG
{
    /** Action */
    u16 Action;                 /* 0 = ACT_GET; 1 = ACT_SET; */
    /** System clock */
    u16 SystemClock;
    /** Supported system clock length */
    u16 SupportedSysClockLen;
    /** Supported system clock */
    u16 SupportedSysClock[16];
} __ATTRIB_PACK__ HostCmd_DS_ECL_SYSTEM_CLOCK_CONFIG;

/** Internal LDO */
#define	LDO_INTERNAL	0
/** External LDO */
#define LDO_EXTERNAL	1

typedef struct _HostCmd_DS_802_11_LDO_CONFIG
{
    u16 Action;                 /**< 0 = ACT_GET; 1 = ACT_SET; */
    u16 PMSource;               /**< 0 = LDO_INTERNAL; 1 = LDO_EXTERNAL */
} __ATTRIB_PACK__ HostCmd_DS_802_11_LDO_CONFIG;

typedef struct _HostCmd_DS_MODULE_TYPE_CONFIG
{
    /** Action */
    u16 Action;                 /* 0 = ACT_GET; 1 = ACT_SET; */
    /** Module */
    u16 Module;
} __ATTRIB_PACK__ HostCmd_DS_MODULE_TYPE_CONFIG;

typedef struct _HostCmd_DS_VERSION_EXT
{
    /** Selected version string */
    u8 versionStrSel;
    /** Version string */
    char versionStr[128];
} __ATTRIB_PACK__ HostCmd_DS_VERSION_EXT;

/** Define data structure for HostCmd_CMD_802_11D_DOMAIN_INFO */
typedef struct _HostCmd_DS_802_11D_DOMAIN_INFO
{
    /** Action */
    u16 Action;
    /** Domain parameter set */
    MrvlIEtypes_DomainParamSet_t Domain;
} __ATTRIB_PACK__ HostCmd_DS_802_11D_DOMAIN_INFO;

/** Define data structure for HostCmd_CMD_802_11D_DOMAIN_INFO response */
typedef struct _HostCmd_DS_802_11D_DOMAIN_INFO_RSP
{
    /** Action */
    u16 Action;
    /** Domain parameter set */
    MrvlIEtypes_DomainParamSet_t Domain;
} __ATTRIB_PACK__ HostCmd_DS_802_11D_DOMAIN_INFO_RSP;

typedef struct _HostCmd_DS_MEF_CFG
{
    /** Criteria */
    u32 Criteria;
    /** Number of entries */
    u16 NumEntries;
} __ATTRIB_PACK__ HostCmd_DS_MEF_CFG;

typedef struct _MEF_CFG_DATA
{
    /** Size */
    u16 size;
    /** Data */
    HostCmd_DS_MEF_CFG data;
} __ATTRIB_PACK__ MEF_CFG_DATA;

typedef struct
{
    /** ACT_GET/ACT_SET */
    u16 Action;
    /** uS, 0 means 1000uS(1ms) */
    u16 TimeoutUnit;
    /** Inactivity timeout for unicast data */
    u16 UnicastTimeout;
    /** Inactivity timeout for multicast data */
    u16 MulticastTimeout;
    /** Timeout for additional RX traffic after Null PM1 packet exchange */
    u16 PsEntryTimeout;
    /** Reserved to further expansion */
    u16 Reserved;
} __ATTRIB_PACK__ HostCmd_DS_INACTIVITY_TIMEOUT_EXT;

typedef struct _HostCmd_DS_DBGS_CFG
{
    /** Destination */
    u8 Destination;
    /** To air channel */
    u8 ToAirChan;
    /** Reserved */
    u8 reserved;
    /** Number of entries */
    u8 En_NumEntries;
} __ATTRIB_PACK__ HostCmd_DS_DBGS_CFG;

typedef struct _DBGS_ENTRY_DATA
{
    /** Mode and mask or ID */
    u16 ModeAndMaskorID;
    /** Base or ID */
    u16 BaseOrID;
} __ATTRIB_PACK__ DBGS_ENTRY_DATA;

typedef struct _DBGS_CFG_DATA
{
    /** Size */
    u16 size;
    /** Data */
    HostCmd_DS_DBGS_CFG data;
} __ATTRIB_PACK__ DBGS_CFG_DATA;

typedef struct _HostCmd_DS_GET_MEM
{
    /** Starting address */
    u32 StartAddr;
    /** Length */
    u16 Len;
} __ATTRIB_PACK__ HostCmd_DS_GET_MEM;

typedef struct _FW_MEM_DATA
{
    /** Size */
    u16 size;
    /** Data */
    HostCmd_DS_GET_MEM data;
} __ATTRIB_PACK__ FW_MEM_DATA;

typedef struct
{
    /** Packet Tx init count */
    u32 PktInitCnt;
    /** Packet Tx success count */
    u32 PktSuccessCnt;
    /** Packets Tx attempts count */
    u32 TxAttempts;
    /** Packet Tx retry failures count */
    u32 RetryFailure;
    /** Packet Tx expiry failures count */
    u32 ExpiryFailure;
} __ATTRIB_PACK__ HostCmd_DS_TX_PKT_STAT_Entry;

typedef struct
{
    /** DS Tx packet statistics */
    HostCmd_DS_TX_PKT_STAT_Entry StatEntry[HOSTCMD_SUPPORTED_RATES];
} __ATTRIB_PACK__ HostCmd_DS_TX_PKT_STATS;

/**
 * @brief 802.11h Local Power Constraint Marvell extended TLV
 */
typedef struct
{
    MrvlIEtypesHeader_t Header;  /**< Marvell TLV header: ID/Len */
    u8 Chan;                     /**< Channel local constraint applies to */

    /** Power constraint included in beacons and used by fw to offset 11d info */
    u8 Constraint;

} __ATTRIB_PACK__ MrvlIEtypes_LocalPowerConstraint_t;

/*
 *
 * Data structures for driver/firmware command processing
 *
 */

/**  TPC Info structure sent in CMD_802_11_TPC_INFO command to firmware */
typedef struct
{
    MrvlIEtypes_LocalPowerConstraint_t localConstraint;  /**< Local constraint */
    MrvlIEtypes_PowerCapability_t powerCap;              /**< Power Capability */

} __ATTRIB_PACK__ HostCmd_DS_802_11_TPC_INFO;

/**  TPC Request structure sent in CMD_802_11_TPC_ADAPT_REQ command to firmware */
typedef struct
{
    u8 destMac[ETH_ALEN];                  /**< Destination STA address  */
    u16 timeout;                           /**< Response timeout in ms */
    u8 rateIndex;                          /**< IEEE Rate index to send request */

} __ATTRIB_PACK__ HostCmd_TpcRequest;

/**  TPC Response structure received from the CMD_802_11_TPC_ADAPT_REQ command */
typedef struct
{
    u8 tpcRetCode;       /**< Firmware command result status code */
    s8 txPower;          /**< Reported TX Power from the TPC Report element */
    s8 linkMargin;       /**< Reported link margin from the TPC Report element */
    s8 rssi;             /**< RSSI of the received TPC Report frame */

} __ATTRIB_PACK__ HostCmd_TpcResponse;

/**  CMD_802_11_TPC_ADAPT_REQ substruct. Union of the TPC request and response */
typedef union
{
    HostCmd_TpcRequest req;   /**< Request struct sent to firmware */
    HostCmd_TpcResponse resp; /**< Response struct received from firmware */

} __ATTRIB_PACK__ HostCmd_DS_802_11_TPC_ADAPT_REQ;

/**  CMD_802_11_CHAN_SW_ANN firmware command substructure */
typedef struct
{
    u8 switchMode;   /**< Set to 1 for a quiet switch request, no STA tx */
    u8 newChan;      /**< Requested new channel */
    u8 switchCount;  /**< Number of TBTTs until the switch is to occur */
} __ATTRIB_PACK__ HostCmd_DS_802_11_CHAN_SW_ANN;

/**        
*** @brief Include extra 11h measurement types (not currently supported 
***                                             in firmware)
**/
#define  WLAN_MEAS_EXTRA_11H_TYPES   0

/**        
*** @brief Include extra 11k measurement types (not currently implemented)
**/
#define  WLAN_MEAS_EXTRA_11K_TYPES   0

/**        
*** @brief Enumeration of measurement types, including max supported 
***        enum for 11h/11k
**/
typedef enum
{
    WLAN_MEAS_BASIC = 0,              /**< 11h: Basic */
#if WLAN_MEAS_EXTRA_11H_TYPES
    /* Not supported in firmware */
    WLAN_MEAS_CCA = 1,                /**< 11h: CCA */
    WLAN_MEAS_RPI = 2,                /**< 11h: RPI */
#endif
#if WLAN_MEAS_EXTRA_11K_TYPES
    /* Future 11k extensions */
    WLAN_MEAS_CHAN_LOAD = 3,          /**< 11k: Channel Load */
    WLAN_MEAS_NOISE_HIST = 4,         /**< 11k: Noise Histogram */
    WLAN_MEAS_BEACON = 5,             /**< 11k: Beacon */
    WLAN_MEAS_FRAME = 6,              /**< 11k: Frame */
    WLAN_MEAS_HIDDEN_NODE = 7,        /**< 11k: Hidden Node */
    WLAN_MEAS_MEDIUM_SENS_HIST = 8,   /**< 11k: Medium Sense Histogram */
    WLAN_MEAS_STA_STATS = 9,          /**< 11k: Station Statistics */
#endif

    WLAN_MEAS_NUM_TYPES,              /**< Number of enumerated measurements */

#if WLAN_MEAS_EXTRA_11H_TYPES
    WLAN_MEAS_11H_MAX_TYPE = WLAN_MEAS_RPI,            /**< Max 11h measurement */
#else
    WLAN_MEAS_11H_MAX_TYPE = WLAN_MEAS_BASIC,          /**< Max 11h measurement */
#endif

#if WLAN_MEAS_EXTRA_11K_TYPES
    /** Future 11k extensions */
    WLAN_MEAS_11K_MAX_TYPE = WLAN_MEAS_STA_STATS,      /**< Max 11k measurement */
#endif

} __ATTRIB_PACK__ MeasType_t;

/**        
*** @brief Mode octet of the measurement request element (7.3.2.21)
**/
typedef struct
{
#ifdef BIG_ENDIAN
    u8 rsvd5_7:3;              /**< Reserved */
    u8 durationMandatory:1;    /**< 11k: duration spec. for meas. is mandatory */
    u8 report:1;               /**< 11h: en/disable report rcpt. of spec. type */
    u8 request:1;              /**< 11h: en/disable requests of specified type */
    u8 enable:1;               /**< 11h: enable report/request bits */
    u8 parallel:1;             /**< 11k: series or parallel with previous meas */
#else
    u8 parallel:1;             /**< 11k: series or parallel with previous meas */
    u8 enable:1;               /**< 11h: enable report/request bits */
    u8 request:1;              /**< 11h: en/disable requests of specified type */
    u8 report:1;               /**< 11h: en/disable report rcpt. of spec. type */
    u8 durationMandatory:1;    /**< 11k: duration spec. for meas. is mandatory */
    u8 rsvd5_7:3;              /**< Reserved */
#endif

} __ATTRIB_PACK__ MeasReqMode_t;

/**        
*** @brief Common measurement request structure (7.3.2.21.1 to 7.3.2.21.3)
**/
typedef struct
{
    u8 channel;      /**< Channel to measure */
    u64 startTime;   /**< TSF Start time of measurement (0 for immediate) */
    u16 duration;    /**< TU duration of the measurement */

} __ATTRIB_PACK__ MeasReqCommonFormat_t;

/**        
*** @brief Basic measurement request structure (7.3.2.21.1)
**/
typedef MeasReqCommonFormat_t MeasReqBasic_t;

/**        
*** @brief CCA measurement request structure (7.3.2.21.2)
**/
typedef MeasReqCommonFormat_t MeasReqCCA_t;

/**        
*** @brief RPI measurement request structure (7.3.2.21.3)
**/
typedef MeasReqCommonFormat_t MeasReqRPI_t;

/**        
*** @brief Union of the availble measurement request types.  Passed in the 
***        driver/firmware interface.
**/
typedef union
{
    MeasReqBasic_t basic; /**< Basic measurement request */
    MeasReqCCA_t cca;     /**< CCA measurement request */
    MeasReqRPI_t rpi;     /**< RPI measurement request */

} MeasRequest_t;

/**        
*** @brief Mode octet of the measurement report element (7.3.2.22)
**/
typedef struct
{
#ifdef BIG_ENDIAN
    u8 rsvd3_7:5;        /**< Reserved */
    u8 refused:1;        /**< Measurement refused */
    u8 incapable:1;      /**< Incapable of performing measurement */
    u8 late:1;           /**< Start TSF time missed for measurement */
#else
    u8 late:1;           /**< Start TSF time missed for measurement */
    u8 incapable:1;      /**< Incapable of performing measurement */
    u8 refused:1;        /**< Measurement refused */
    u8 rsvd3_7:5;        /**< Reserved */
#endif

} __ATTRIB_PACK__ MeasRptMode_t;

/**
***  @brief Map octet of the basic measurement report (7.3.2.22.1)
**/
typedef struct
{
#ifdef BIG_ENDIAN
    u8 rsvd5_7:3;              /**< Reserved */
    u8 unmeasured:1;           /**< Channel is unmeasured */
    u8 radar:1;                /**< Radar detected on channel */
    u8 unidentifiedSig:1;      /**< Unidentified signal found on channel */
    u8 OFDM_Preamble:1;        /**< OFDM preamble detected on channel */
    u8 BSS:1;                  /**< At least one valid MPDU received on channel */
#else
    u8 BSS:1;                  /**< At least one valid MPDU received on channel */
    u8 OFDM_Preamble:1;        /**< OFDM preamble detected on channel */
    u8 unidentifiedSig:1;      /**< Unidentified signal found on channel */
    u8 radar:1;                /**< Radar detected on channel */
    u8 unmeasured:1;           /**< Channel is unmeasured */
    u8 rsvd5_7:3;              /**< Reserved */
#endif

} __ATTRIB_PACK__ MeasRptBasicMap_t;

/**        
*** @brief Basic measurement report (7.3.2.22.1)
**/
typedef struct
{
    u8 channel;                /**< Channel to measured */
    u64 startTime;             /**< Start time (TSF) of measurement */
    u16 duration;              /**< Duration of measurement in TUs */
    MeasRptBasicMap_t map;     /**< Basic measurement report */

} __ATTRIB_PACK__ MeasRptBasic_t;

/**        
*** @brief CCA measurement report (7.3.2.22.2)
**/
typedef struct
{
    u8 channel;                /**< Channel to measured */
    u64 startTime;             /**< Start time (TSF) of measurement */
    u16 duration;              /**< Duration of measurement in TUs  */
    u8 busyFraction;           /**< Fractional duration CCA indicated chan busy */

} __ATTRIB_PACK__ MeasRptCCA_t;

/**        
*** @brief RPI measurement report (7.3.2.22.3)
**/
typedef struct
{
    u8 channel;              /**< Channel to measured  */
    u64 startTime;           /**< Start time (TSF) of measurement */
    u16 duration;            /**< Duration of measurement in TUs  */
    u8 density[8];           /**< RPI Density histogram report */

} __ATTRIB_PACK__ MeasRptRPI_t;

/**        
*** @brief Union of the availble measurement report types.  Passed in the 
***        driver/firmware interface.
**/
typedef union
{
    MeasRptBasic_t basic;    /**< Basic measurement report */
    MeasRptCCA_t cca;        /**< CCA measurement report */
    MeasRptRPI_t rpi;        /**< RPI measurement report */

} MeasReport_t;

/**        
*** @brief Structure passed to firmware to perform a measurement
**/
typedef struct
{
    u8 macAddr[ETH_ALEN];                        /**< Reporting STA address */
    u8 dialogToken;                              /**< Measurement dialog toke */
    MeasReqMode_t reqMode;                       /**< Report mode  */
    MeasType_t measType;                         /**< Measurement type */
    MeasRequest_t req;                           /**< Measurement request data */

} __ATTRIB_PACK__ HostCmd_DS_MEASUREMENT_REQUEST;

/**        
*** @brief Structure passed back from firmware with a measurement report,
***        also can be to send a measurement report to another STA
**/
typedef struct
{
    u8 macAddr[ETH_ALEN];                        /**< Reporting STA address */
    u8 dialogToken;                              /**< Measurement dialog token */
    MeasRptMode_t rptMode;                       /**< Report mode */
    MeasType_t measType;                         /**< Measurement type */
    MeasReport_t rpt;                            /**< Measurement report data */

} __ATTRIB_PACK__ HostCmd_DS_MEASUREMENT_REPORT;

/** HostCmd_CMD_FUNC_IF_CTRL */
typedef struct _HostCmd_DS_FUNC_IF_CTRL
{
        /** Action */
    u16 Action;
        /** Interface */
    u16 Interface;
} __ATTRIB_PACK__ HostCmd_DS_FUNC_IF_CTRL;

/** _HostCmd_DS_COMMAND*/
struct _HostCmd_DS_COMMAND
{

    /** Command Header : Command */
    u16 Command;
    /** Command Header : Size */
    u16 Size;
    /** Command Header : Sequence number */
    u16 SeqNum;
    /** Command Header : Result */
    u16 Result;

    /** Command Body */
    union
    {
        HostCmd_DS_GET_HW_SPEC hwspec;
        HostCmd_DS_802_11_PS_MODE psmode;
        HostCmd_DS_802_11_SCAN scan;
        HostCmd_DS_802_11_SCAN_RSP scanresp;
        HostCmd_DS_MAC_CONTROL macctrl;
        HostCmd_DS_802_11_ASSOCIATE associate;
        HostCmd_DS_802_11_ASSOCIATE_RSP associatersp;
        HostCmd_DS_802_11_DEAUTHENTICATE deauth;
        HostCmd_DS_802_11_SET_WEP wep;
        HostCmd_DS_802_11_AD_HOC_START ads;
        HostCmd_DS_802_11_AD_HOC_RESULT result;
        HostCmd_DS_802_11_GET_LOG glog;
        HostCmd_DS_802_11_SNMP_MIB smib;
        HostCmd_DS_802_11_RF_TX_POWER txp;
        HostCmd_DS_802_11_RF_ANTENNA rant;
        HostCmd_DS_802_11_RATE_ADAPT_RATESET rateset;
        HostCmd_DS_MAC_MULTICAST_ADR madr;
        HostCmd_DS_802_11_AD_HOC_JOIN adj;
        HostCmd_DS_802_11_RADIO_CONTROL radio;
        HostCmd_DS_802_11_RF_CHANNEL rfchannel;
        HostCmd_DS_802_11_RSSI_INFO rssi_info;
        HostCmd_DS_802_11_RSSI_INFO_RSP rssi_inforsp;
        HostCmd_DS_802_11_AD_HOC_STOP adhoc_stop;
        HostCmd_DS_802_11_MAC_ADDRESS macadd;
        HostCmd_DS_802_11_KEY_MATERIAL keymaterial;
        HostCmd_DS_MAC_REG_ACCESS macreg;
        HostCmd_DS_BBP_REG_ACCESS bbpreg;
        HostCmd_DS_RF_REG_ACCESS rfreg;
        HostCmd_DS_PMIC_REG_ACCESS pmicreg;
        HostCmd_DS_802_11_HOST_SLEEP_CFG hscfg;
        HostCmd_DS_802_11_EEPROM_ACCESS rdeeprom;

        HostCmd_DS_802_11D_DOMAIN_INFO domaininfo;
        HostCmd_DS_802_11D_DOMAIN_INFO_RSP domaininforesp;
        HostCmd_DS_802_11_TPC_ADAPT_REQ tpcReq;
        HostCmd_DS_802_11_TPC_INFO tpcInfo;
        HostCmd_DS_802_11_CHAN_SW_ANN chan_sw_ann;
        HostCmd_DS_MEASUREMENT_REQUEST meas_req;
        HostCmd_DS_MEASUREMENT_REPORT meas_rpt;
        HostCmd_DS_802_11_BG_SCAN_CONFIG bgscancfg;
        HostCmd_DS_802_11_BG_SCAN_QUERY bgscanquery;
        HostCmd_DS_802_11_BG_SCAN_QUERY_RSP bgscanqueryresp;
        HostCmd_DS_WMM_GET_STATUS getWmmStatus;
        HostCmd_DS_WMM_ADDTS_REQ addTsReq;
        HostCmd_DS_WMM_DELTS_REQ delTsReq;
        HostCmd_DS_WMM_QUEUE_CONFIG queueConfig;
        HostCmd_DS_WMM_QUEUE_STATS queueStats;
        HostCmd_DS_WMM_TS_STATUS tsStatus;
        HostCmd_DS_TX_PKT_STATS txPktStats;
        HostCmd_DS_802_11_SLEEP_PARAMS sleep_params;
        HostCmd_DS_802_11_BCA_TIMESHARE bca_timeshare;
        HostCmd_DS_802_11_SLEEP_PERIOD ps_sleeppd;
        HostCmd_DS_802_11_TPC_CFG tpccfg;
        HostCmd_DS_802_11_POWER_ADAPT_CFG_EXT pacfgext;
        HostCmd_DS_802_11_LED_CTRL ledgpio;
        HostCmd_DS_802_11_FW_WAKEUP_METHOD fwwakeupmethod;

        HostCmd_DS_802_11_CRYPTO crypto;
        HostCmd_DS_802_11_CRYPTO_AES_CCM crypto_aes_ccm;
        HostCmd_TX_RATE_QUERY txrate;
        HostCmd_DS_GET_TSF gettsf;
        HostCmd_DS_802_11_IBSS_Status ibssCoalescing;
        HostCmd_DS_SDIO_PULL_CTRL sdiopullctl;
        HostCmd_DS_ECL_SYSTEM_CLOCK_CONFIG sysclockcfg;
        HostCmd_DS_802_11_LDO_CONFIG ldocfg;
        HostCmd_DS_MODULE_TYPE_CONFIG moduletypecfg;
        HostCmd_DS_VERSION_EXT verext;
        HostCmd_DS_MEF_CFG mefcfg;
        HostCmd_DS_DBGS_CFG dbgcfg;
        HostCmd_DS_GET_MEM getmem;
        HostCmd_DS_INACTIVITY_TIMEOUT_EXT inactivityext;
        HostCmd_DS_FUNC_IF_CTRL intfctrl;
    } params;
} __ATTRIB_PACK__;

/** Find minimum */
#ifndef MIN
#define MIN(a,b)		((a) < (b) ? (a) : (b))
#endif

/** Find maximum */
#ifndef MAX
#define MAX(a,b)		((a) > (b) ? (a) : (b))
#endif

/** Find number of elements */
#ifndef NELEMENTS
#define NELEMENTS(x) (sizeof(x)/sizeof(x[0]))
#endif

/** Buffer Constants */

/**	The size of SQ memory PPA, DPA are 8 DWORDs, that keep the physical
*	addresses of TxPD buffers. Station has only 8 TxPD available, Whereas
*	driver has more local TxPDs. Each TxPD on the host memory is associated 
*	with a Tx control node. The driver maintains 8 RxPD descriptors for 
*	station firmware to store Rx packet information.
*
*	Current version of MAC has a 32x6 multicast address buffer.
*
*	802.11b can have up to  14 channels, the driver keeps the
*	BSSID(MAC address) of each APs or Ad hoc stations it has sensed.
*/

/** Size of SQ memory PPA */
#define MRVDRV_SIZE_OF_PPA		0x00000008
/** Size of SQ memory DPA */
#define MRVDRV_SIZE_OF_DPA		0x00000008
/** Number of TxPD in station */
#define MRVDRV_NUM_OF_TxPD		0x00000020
/** Numner of command buffers */
#define MRVDRV_NUM_OF_CMD_BUFFER        10
/** Size of command buffer */
#define MRVDRV_SIZE_OF_CMD_BUFFER       (2 * 1024)
/** Maximum number of BSSIDs */
#define MRVDRV_MAX_BSSID_LIST		64
/** 10 seconds */
#define MRVDRV_TIMER_10S		10000
/** 5 seconds */
#define MRVDRV_TIMER_5S			5000
/** 1 second */
#define MRVDRV_TIMER_1S			1000
/** Size of ethernet header */
#define MRVDRV_ETH_HEADER_SIZE          14

/** Maximum buffer size for ARP filter */
#define ARP_FILTER_MAX_BUF_SIZE		68

/** WLAN upload size */
#define	WLAN_UPLD_SIZE			2312
/** Length of device length */
#define DEV_NAME_LEN			32

/** Length of ethernet address */
#ifndef	ETH_ALEN
#define ETH_ALEN			6
#endif

/** Misc constants */
/* This section defines 802.11 specific contents */
/** SDIO header length */
#define SDIO_HEADER_LEN		4

/** Maximum number of region codes */
#define MRVDRV_MAX_REGION_CODE			7

/** Maximum length of SSID list */
#define MRVDRV_MAX_SSID_LIST_LENGTH     10

/** Ignore multiple DTIM */
#define MRVDRV_IGNORE_MULTIPLE_DTIM		0xfffe
/** Minimum multiple DTIM */
#define MRVDRV_MIN_MULTIPLE_DTIM		1
/** Maximum multiple DTIM */
#define MRVDRV_MAX_MULTIPLE_DTIM		5
/** Default multiple DTIM */
#define MRVDRV_DEFAULT_MULTIPLE_DTIM		1

/** Default listen interval */
#define MRVDRV_DEFAULT_LISTEN_INTERVAL		10
/** Default local listen interval */
#define MRVDRV_DEFAULT_LOCAL_LISTEN_INTERVAL		0

/** Number of channels per active scan */
#define	MRVDRV_CHANNELS_PER_ACTIVE_SCAN		14
/** Minimum beacon interval */
#define MRVDRV_MIN_BEACON_INTERVAL		20
/** Maximum beacon interval */
#define MRVDRV_MAX_BEACON_INTERVAL		1000
/** Default beacon interval */
#define MRVDRV_BEACON_INTERVAL			100

/** Default watchdog timeout */
#define MRVDRV_DEFAULT_WATCHDOG_TIMEOUT (5 * HZ)
/** Watchdog timeout for scan */
#define MRVDRV_SCAN_WATCHDOG_TIMEOUT    (10 * HZ)

/** TxPD Status */

/*	Station firmware use TxPD status field to report final Tx transmit
*	result, Bit masks are used to present combined situations.
*/

/** Bit mask for TxPD status field for null packet */
#define MRVDRV_TxPD_POWER_MGMT_NULL_PACKET 0x01
/** Bit mask for TxPD status field for last packet */
#define MRVDRV_TxPD_POWER_MGMT_LAST_PACKET 0x08

/** Tx control node status */

#define MRVDRV_TX_CTRL_NODE_STATUS_IDLE      0x0000

/* Link speed */
/** Link speed : 1 Mbps */
#define MRVDRV_LINK_SPEED_1mbps          10000  /* in unit of 100bps */
/** Link speed : 11 Mbps */
#define MRVDRV_LINK_SPEED_11mbps         110000

/** RSSI-related defines */
/*	RSSI constants are used to implement 802.11 RSSI threshold 
*	indication. if the Rx packet signal got too weak for 5 consecutive
*	times, miniport driver (driver) will report this event to wrapper
*/

#define MRVDRV_NF_DEFAULT_SCAN_VALUE		(-96)

/** RTS/FRAG related defines */
/** Minimum RTS value */
#define MRVDRV_RTS_MIN_VALUE		0
/** Maximum RTS value */
#define MRVDRV_RTS_MAX_VALUE		2347
/** Minimum FRAG value */
#define MRVDRV_FRAG_MIN_VALUE		256
/** Maximum FRAG value */
#define MRVDRV_FRAG_MAX_VALUE		2346

/** Fixed IE size is 8 bytes time stamp + 2 bytes beacon interval +
 * 2 bytes cap */
#define MRVL_FIXED_IE_SIZE      12

/** Host command flag in command */
#define	CMD_F_HOSTCMD		(1 << 0)

/* to resolve CISCO AP extension */
/** Space for Variable IE in scan list */
#define MRVDRV_SCAN_LIST_VAR_IE_SPACE  	256
/** Checks whether WPA enabled in firmware */
#define FW_IS_WPA_ENABLED(_adapter) \
		(_adapter->fwCapInfo & FW_CAPINFO_WPA)

/** WPA capability bit in firmware capability */
#define FW_CAPINFO_WPA  	(1 << 0)
/** REQFI capabilities */
#define WLAN_802_11_AI_REQFI_CAPABILITIES 	1
/** REQFI listen interval */
#define WLAN_802_11_AI_REQFI_LISTENINTERVAL 	2
/** REQFI current AP address */
#define WLAN_802_11_AI_REQFI_CURRENTAPADDRESS 	4

/** RESFI capabilities */
#define WLAN_802_11_AI_RESFI_CAPABILITIES 	1
/** RESFI status code */
#define WLAN_802_11_AI_RESFI_STATUSCODE 	2
/** RESFI association ID */
#define WLAN_802_11_AI_RESFI_ASSOCIATIONID 	4

/** Number of WEP keys */
#define MRVL_NUM_WEP_KEY		4

/** Support 4 keys per key set */
#define MRVL_NUM_WPA_KEY_PER_SET        4
/** WPA Key LENGTH*/
#define MRVL_MAX_WPA_KEY_LENGTH 	32

/** WPA AES key length */
#define WPA_AES_KEY_LEN 		16
/** WPA TKIP key length */
#define WPA_TKIP_KEY_LEN 		32

/* A few details needed for WEP (Wireless Equivalent Privacy) */
/** 104 bits */
#define MAX_WEP_KEY_SIZE	13
/** 40 bits RC4 - WEP */
#define MIN_WEP_KEY_SIZE	5

/** RF antenna select 1 */
#define RF_ANTENNA_1		0x1
/** RF antenna select 2 */
#define RF_ANTENNA_2		0x2
/** RF antenna auto select */
#define RF_ANTENNA_AUTO		0xFFFF

/** Key information enabled */
#define KEY_INFO_ENABLED	0x01

/** Beacon Signal to Noise Ratio */
#define SNR_BEACON		0
/** RXPD Signal to Noise Ratio */
#define SNR_RXPD		1
/** Beacon Noise Floor */
#define NF_BEACON		2
/** RXPD Noise Floor */
#define NF_RXPD			3

/** MACRO DEFINITIONS */
/** SNR calculation */
#define CAL_SNR(RSSI,NF)		((s32)((s32)(RSSI)-(s32)(NF)))

/** RSSI scan */
#define SCAN_RSSI(RSSI)			(0x100 - ((u8)(RSSI)))

/** Default factor for calculating beacon average */
#define DEFAULT_BCN_AVG_FACTOR		8
/** Default factor for calculating data average */
#define DEFAULT_DATA_AVG_FACTOR		8
/** Minimum factor for calculating beacon average */
#define MIN_BCN_AVG_FACTOR		1
/** Maximum factor for calculating beacon average */
#define MAX_BCN_AVG_FACTOR		8
/** Minimum factor for calculating data average */
#define MIN_DATA_AVG_FACTOR		1
/** Maximum factor for calculating data average */
#define MAX_DATA_AVG_FACTOR		8
/** Scale used to measure average */
#define AVG_SCALE			100
/** Average SNR NF calculations */
#define CAL_AVG_SNR_NF(AVG, SNRNF, N)         \
                        (((AVG) == 0) ? ((u16)(SNRNF) * AVG_SCALE) : \
                        ((((int)(AVG) * (N -1)) + ((u16)(SNRNF) * \
                        AVG_SCALE))  / N))

/** Success */
#define WLAN_STATUS_SUCCESS			(0)
/** Failure */
#define WLAN_STATUS_FAILURE			(-1)
/** Not accepted */
#define WLAN_STATUS_NOT_ACCEPTED                (-2)

/** Maximum number of LEDs */
#define	MAX_LEDS			3
/** LED disabled */
#define	LED_DISABLED			16
/** LED blinking */
#define	LED_BLINKING			2

/** S_SWAP : To swap 2 u8 */
#define S_SWAP(a,b) 	do { \
				u8  t = SArr[a]; \
				SArr[a] = SArr[b]; SArr[b] = t; \
			} while(0)

/** SWAP: swap u8 */
#define SWAP_U8(a,b)	{u8 t; t=a; a=b; b=t;}

/** SWAP: swap u8 */
#define SWAP_U16(a,b)	{u16 t; t=a; a=b; b=t;}

#ifdef BIG_ENDIAN
/** Convert from 16 bit little endian format to CPU format */
#define wlan_le16_to_cpu(x) le16_to_cpu(x)
/** Convert from 32 bit little endian format to CPU format */
#define wlan_le32_to_cpu(x) le32_to_cpu(x)
/** Convert from 64 bit little endian format to CPU format */
#define wlan_le64_to_cpu(x) le64_to_cpu(x)
/** Convert to 16 bit little endian format from CPU format */
#define wlan_cpu_to_le16(x) cpu_to_le16(x)
/** Convert to 32 bit little endian format from CPU format */
#define wlan_cpu_to_le32(x) cpu_to_le32(x)
/** Convert to 64 bit little endian format from CPU format */
#define wlan_cpu_to_le64(x) cpu_to_le64(x)

/** Convert TxPD to little endian format from CPU format */
#define endian_convert_TxPD(x);                                         \
    {                                                                   \
        (x)->TxPktLength = wlan_cpu_to_le16((x)->TxPktLength);    	\
        (x)->TxPktOffset = wlan_cpu_to_le32((x)->TxPktOffset);		\
        (x)->TxControl = wlan_cpu_to_le32((x)->TxControl);              \
    }

/** Convert RxPD from little endian format to CPU format */
#define endian_convert_RxPD(x);                             		\
    {                                                       		\
        (x)->RxPktLength = wlan_le16_to_cpu((x)->RxPktLength);        	\
        (x)->RxPktOffset = wlan_le32_to_cpu((x)->RxPktOffset);  	\
    }

/** Convert log from little endian format to CPU format */
#define endian_convert_GET_LOG(x); { \
		Adapter->LogMsg.mcastrxframe = wlan_le32_to_cpu(Adapter->LogMsg.mcastrxframe); \
		Adapter->LogMsg.failed = wlan_le32_to_cpu(Adapter->LogMsg.failed); \
		Adapter->LogMsg.retry = wlan_le32_to_cpu(Adapter->LogMsg.retry); \
		Adapter->LogMsg.multiretry = wlan_le32_to_cpu(Adapter->LogMsg.multiretry); \
		Adapter->LogMsg.framedup = wlan_le32_to_cpu(Adapter->LogMsg.framedup); \
		Adapter->LogMsg.rtssuccess = wlan_le32_to_cpu(Adapter->LogMsg.rtssuccess); \
		Adapter->LogMsg.rtsfailure = wlan_le32_to_cpu(Adapter->LogMsg.rtsfailure); \
		Adapter->LogMsg.ackfailure = wlan_le32_to_cpu(Adapter->LogMsg.ackfailure); \
		Adapter->LogMsg.rxfrag = wlan_le32_to_cpu(Adapter->LogMsg.rxfrag); \
		Adapter->LogMsg.mcastrxframe = wlan_le32_to_cpu(Adapter->LogMsg.mcastrxframe); \
		Adapter->LogMsg.fcserror = wlan_le32_to_cpu(Adapter->LogMsg.fcserror); \
		Adapter->LogMsg.txframe = wlan_le32_to_cpu(Adapter->LogMsg.txframe); \
}

#else /* BIG_ENDIAN */
/** Do nothing */
#define wlan_le16_to_cpu(x) x
/** Do nothing */
#define wlan_le32_to_cpu(x) x
/** Do nothing */
#define wlan_le64_to_cpu(x) x
/** Do nothing */
#define wlan_cpu_to_le16(x) x
/** Do nothing */
#define wlan_cpu_to_le32(x) x
/** Do nothing */
#define wlan_cpu_to_le64(x) x

/** Do nothing */
#define endian_convert_TxPD(x)
/** Do nothing */
#define endian_convert_RxPD(x)
/** Do nothing */
#define endian_convert_GET_LOG(x)
#endif /* BIG_ENDIAN */

/** Global Varibale Declaration */
/** Private data structure of the device */
typedef struct _wlan_private wlan_private;
/** Adapter data structure of the device */
typedef struct _wlan_adapter wlan_adapter;
/** Host command data structure */
typedef struct _HostCmd_DS_COMMAND HostCmd_DS_COMMAND;

/** DS frequency list */
extern u32 DSFreqList[15];
/** Driver version */
extern const char driver_version[];
extern u32 DSFreqList[];
extern u16 RegionCodeToIndex[MRVDRV_MAX_REGION_CODE];

extern u8 WlanDataRates[WLAN_SUPPORTED_RATES];

extern u8 SupportedRates_B[B_SUPPORTED_RATES];
extern u8 SupportedRates_G[G_SUPPORTED_RATES];
extern u8 SupportedRates_A[A_SUPPORTED_RATES];
extern u8 AdhocRates_A[A_SUPPORTED_RATES];

extern u8 AdhocRates_G[G_SUPPORTED_RATES];

extern u8 AdhocRates_B[4];
extern wlan_private *wlanpriv;

#ifdef MFG_CMD_SUPPORT
#define SIOCCFMFG SIOCDEVPRIVATE
#endif /* MFG_CMD_SUPPORT */

/** ENUM definition*/
/** SNRNF_TYPE */
typedef enum _SNRNF_TYPE
{
    TYPE_BEACON = 0,
    TYPE_RXPD,
    MAX_TYPE_B
} SNRNF_TYPE;

/** SNRNF_DATA*/
typedef enum _SNRNF_DATA
{
    TYPE_NOAVG = 0,
    TYPE_AVG,
    MAX_TYPE_AVG
} SNRNF_DATA;

/** WLAN_802_11_AUTH_ALG*/
typedef enum _WLAN_802_11_AUTH_ALG
{
    AUTH_ALG_OPEN_SYSTEM = 1,
    AUTH_ALG_SHARED_KEY = 2,
    AUTH_ALG_NETWORK_EAP = 8,
} WLAN_802_11_AUTH_ALG;

/** WLAN_802_11_ENCRYPTION_MODE */
typedef enum _WLAN_802_11_ENCRYPTION_MODE
{
    CIPHER_NONE,
    CIPHER_WEP40,
    CIPHER_TKIP,
    CIPHER_CCMP,
    CIPHER_WEP104,
} WLAN_802_11_ENCRYPTION_MODE;

/** WLAN_802_11_POWER_MODE */
typedef enum _WLAN_802_11_POWER_MODE
{
    Wlan802_11PowerModeCAM,
    Wlan802_11PowerModeMAX_PSP,
    Wlan802_11PowerModeFast_PSP,

    /* not a real mode, defined as an upper bound */
    Wlan802_11PowerModeMax
} WLAN_802_11_POWER_MODE;

/** PS_STATE */
typedef enum _PS_STATE
{
    PS_STATE_FULL_POWER,
    PS_STATE_AWAKE,
    PS_STATE_PRE_SLEEP,
    PS_STATE_SLEEP
} PS_STATE;

/** WLAN_MEDIA_STATE */
typedef enum _WLAN_MEDIA_STATE
{
    WlanMediaStateDisconnected,
    WlanMediaStateConnected
} WLAN_MEDIA_STATE;

/** WLAN_802_11_PRIVACY_FILTER */
typedef enum _WLAN_802_11_PRIVACY_FILTER
{
    Wlan802_11PrivFilterAcceptAll,
    Wlan802_11PrivFilter8021xWEP
} WLAN_802_11_PRIVACY_FILTER;

/** Hardware status codes */
typedef enum _WLAN_HARDWARE_STATUS
{
    WlanHardwareStatusReady,
    WlanHardwareStatusInitializing,
    WlanHardwareStatusReset,
    WlanHardwareStatusClosing,
    WlanHardwareStatusNotReady
} WLAN_HARDWARE_STATUS;

/** WLAN_802_11_AUTHENTICATION_MODE */
typedef enum _WLAN_802_11_AUTHENTICATION_MODE
{
    Wlan802_11AuthModeOpen = 0x00,
    Wlan802_11AuthModeShared = 0x01,
    Wlan802_11AuthModeNetworkEAP = 0x80,
} WLAN_802_11_AUTHENTICATION_MODE;

/** WLAN_802_11_WEP_STATUS */
typedef enum _WLAN_802_11_WEP_STATUS
{
    Wlan802_11WEPEnabled,
    Wlan802_11WEPDisabled,
    Wlan802_11WEPKeyAbsent,
    Wlan802_11WEPNotSupported
} WLAN_802_11_WEP_STATUS;

/** SNMP_MIB_INDEX_e */
typedef enum _SNMP_MIB_INDEX_e
{
    OpRateSet_i = 1,
    DtimPeriod_i = 3,
    RtsThresh_i = 5,
    ShortRetryLim_i = 6,
    LongRetryLim_i = 7,
    FragThresh_i = 8,
    Dot11D_i = 9,
    Dot11H_i = 10
} SNMP_MIB_INDEX_e;

/** KEY_TYPE_ID */
typedef enum _KEY_TYPE_ID
{
    KEY_TYPE_ID_WEP = 0,
    KEY_TYPE_ID_TKIP,
    KEY_TYPE_ID_AES
} KEY_TYPE_ID;

/** KEY_INFO_WEP*/
typedef enum _KEY_INFO_WEP
{
    KEY_INFO_WEP_DEFAULT_KEY = 0x01
} KEY_INFO_WEP;

/** KEY_INFO_TKIP */
typedef enum _KEY_INFO_TKIP
{
    KEY_INFO_TKIP_MCAST = 0x01,
    KEY_INFO_TKIP_UNICAST = 0x02,
    KEY_INFO_TKIP_ENABLED = 0x04
} KEY_INFO_TKIP;

/** KEY_INFO_AES*/
typedef enum _KEY_INFO_AES
{
    KEY_INFO_AES_MCAST = 0x01,
    KEY_INFO_AES_UNICAST = 0x02,
    KEY_INFO_AES_ENABLED = 0x04
} KEY_INFO_AES;

/** SNMP_MIB_VALUE_e */
typedef enum _SNMP_MIB_VALUE_e
{
    SNMP_MIB_VALUE_INFRA = 1,
    SNMP_MIB_VALUE_ADHOC
} SNMP_MIB_VALUE_e;

/** HWRateDropMode */
typedef enum _HWRateDropMode
{
    NO_HW_RATE_DROP,
    HW_TABLE_RATE_DROP,
    HW_SINGLE_RATE_DROP
} HWRateDropMode;

#ifdef __KERNEL__
extern struct iw_handler_def wlan_handler_def;
struct iw_statistics *wlan_get_wireless_stats(struct net_device *dev);
int wlan_do_ioctl(struct net_device *dev, struct ifreq *req, int i);
#endif

#endif /* _WLAN_FW_H */
