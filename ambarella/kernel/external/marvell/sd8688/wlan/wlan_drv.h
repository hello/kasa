/** @file wlan_drv.h
  * @brief This file contains wlan driver specific defines etc.
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

#ifndef _WLAN_DRV_H
#define _WLAN_DRV_H

/** Driver release version */
#define DRIVER_RELEASE_VERSION		"26609.p12"

/** headroom alignment for tx packet */
#define HEADER_ALIGNMENT	8

/** Maximum number of BSSID per channel */
#define	MAX_BSSID_PER_CHANNEL		16

/** For the extended Scan */
#define MAX_EXTENDED_SCAN_BSSID_LIST    (MAX_BSSID_PER_CHANNEL * \
						MRVDRV_MAX_CHANNEL_SIZE + 1)
enum
{
    ADHOC_IDLE,
    ADHOC_STARTED,
    ADHOC_JOINED,
    ADHOC_COALESCED
};

typedef struct _PER_CHANNEL_BSSID_LIST_DATA
{
    /** BSSID list data start */
    u8 ucStart;
    /** BSSID list data count */
    u8 ucNumEntry;
} PER_CHANNEL_BSSID_LIST_DATA, *PPER_CHANNEL_BSSID_LIST_DATA;

typedef struct _MRV_BSSID_IE_LIST
{
    /** Fixed IEs */
    WLAN_802_11_FIXED_IEs FixedIE;
    /** Variable IEs */
    u8 VariableIE[MRVDRV_SCAN_LIST_VAR_IE_SPACE];
} MRV_BSSID_IE_LIST, *PMRV_BSSID_IE_LIST;

/** Maximum number of region channel */
#define	MAX_REGION_CHANNEL_NUM	2

/** Chan-Freq-TxPower mapping table*/
typedef struct _CHANNEL_FREQ_POWER
{
        /** Channel Number		*/
    u16 Channel;
        /** Frequency of this Channel	*/
    u32 Freq;
        /** Max allowed Tx power level	*/
    u16 MaxTxPower;
        /** TRUE:channel unsupported;  FLASE:supported*/
    BOOLEAN Unsupported;
} CHANNEL_FREQ_POWER;

/** region-band mapping table*/
typedef struct _REGION_CHANNEL
{
        /** TRUE if this entry is valid		     */
    BOOLEAN Valid;
        /** Region code for US, Japan ...	     */
    u8 Region;
        /** Band B/G/A, used for BAND_CONFIG cmd	     */
    u8 Band;
        /** Actual No. of elements in the array below */
    u8 NrCFP;
        /** chan-freq-txpower mapping table*/
    CHANNEL_FREQ_POWER *CFP;
} REGION_CHANNEL;

typedef struct _wlan_802_11_security_t
{
    /** WPA enabled flag */
    BOOLEAN WPAEnabled;
    /** WPA2 enabled flag */
    BOOLEAN WPA2Enabled;
    /** WEP status */
    WLAN_802_11_WEP_STATUS WEPStatus;
    /** Authentication mode */
    WLAN_802_11_AUTHENTICATION_MODE AuthenticationMode;
    /** Encryption mode */
    WLAN_802_11_ENCRYPTION_MODE EncryptionMode;
} wlan_802_11_security_t;

/** Current Basic Service Set State Structure */
typedef struct
{
    /** BSS descriptor */
    BSSDescriptor_t BSSDescriptor;

        /** band */
    u8 band;

        /** number of rates supported */
    int NumOfRates;

        /** supported rates*/
    u8 DataRates[WLAN_SUPPORTED_RATES];

        /** wmm enable? */
    u8 wmm_enabled;

        /** uapsd enable?*/
    u8 wmm_uapsd_enabled;
} CurrentBSSParams_t;

/** sleep_params */
typedef struct SleepParams
{
    /** Sleep parameter error */
    u16 sp_error;
    /** Sleep parameter offset */
    u16 sp_offset;
    /** Sleep parameter stable time */
    u16 sp_stabletime;
    /** Sleep parameter calibration control */
    u8 sp_calcontrol;
    /** Sleep parameter external sleep clock */
    u8 sp_extsleepclk;
    /** Sleep parameter reserved */
    u16 sp_reserved;
} SleepParams;

/** sleep_period */
typedef struct SleepPeriod
{
    /** Sleep period */
    u16 period;
    /** Reserved */
    u16 reserved;
} SleepPeriod;

/** Debug command number */
#define DBG_CMD_NUM	5

/** info for debug purpose */
typedef struct _wlan_dbg
{
    /** Number of host to card command failures */
    u32 num_cmd_host_to_card_failure;
    /** Number of host to card sleep confirm failures */
    u32 num_cmd_sleep_cfm_host_to_card_failure;
    /** Number of host to card Tx failures */
    u32 num_tx_host_to_card_failure;
    /** Number of deauthentication events */
    u32 num_event_deauth;
    /** Number of disassosiation events */
    u32 num_event_disassoc;
    /** Number of link lost events */
    u32 num_event_link_lost;
    /** Number of deauthentication commands */
    u32 num_cmd_deauth;
    /** Number of association comamnd successes */
    u32 num_cmd_assoc_success;
    /** Number of association command failures */
    u32 num_cmd_assoc_failure;
    /** Number of Tx timeouts */
    u32 num_tx_timeout;
    /** Number of command timeouts */
    u32 num_cmd_timeout;
    /** Timeout command ID */
    u16 TimeoutCmdId;
    /** Timeout command action */
    u16 TimeoutCmdAct;
    /** List of last command IDs */
    u16 LastCmdId[DBG_CMD_NUM];
    /** List of last command actions */
    u16 LastCmdAct[DBG_CMD_NUM];
    /** Last command index */
    u16 LastCmdIndex;
    /** List of last command response IDs */
    u16 LastCmdRespId[DBG_CMD_NUM];
    /** Last command response index */
    u16 LastCmdRespIndex;
    /** List of last events */
    u16 LastEvent[DBG_CMD_NUM];
    /** Last event index */
    u16 LastEventIndex;
} wlan_dbg;

/** Data structure for the Marvell WLAN device */
typedef struct _wlan_dev
{

        /** card pointer */
    void *card;
        /** IO port */
    u32 ioport;
        /** Upload type */
    u32 upld_typ;
        /** Upload length */
    u32 upld_len;
        /** netdev pointer */
    struct net_device *netdev;
        /** Upload buffer*/
    u8 upld_buf[WLAN_UPLD_SIZE];
        /** Data sent: 
	    TRUE - Data is sent to fw, no Tx Done received
	    FALSE - Tx done received for previous Tx */
    BOOLEAN data_sent;
        /** CMD sent: 
	    TRUE - CMD is sent to fw, no CMD Done received
	    FALSE - CMD done received for previous CMD */
    BOOLEAN cmd_sent;
} wlan_dev_t, *pwlan_dev_t;

/** Data structure for WPS information */
typedef struct
{
    /** Session enable flag */
    BOOLEAN SessionEnable;
} wps_t;

/** Private structure for the MV device */
struct _wlan_private
{
    /** Device open */
    int open;

    /** Device adapter structure */
    wlan_adapter *adapter;
    /** Device structure */
    wlan_dev_t wlan_dev;

    /** Net device statistics structure */
    struct net_device_stats stats;

    /** IW statistics */
    struct iw_statistics wstats;
#ifdef CONFIG_PROC_FS
    struct proc_dir_entry *proc_mwlan;
    struct proc_dir_entry *proc_entry;
    /** Proc entry name */
    char proc_entry_name[IFNAMSIZ];
#endif                          /* CONFIG_PROC_FS */
    /** Firmware helper */
    const struct firmware *fw_helper;
    /** Firmware */
    const struct firmware *firmware;
    /** Hotplug device */
    struct device *hotplug_device;

        /** thread to service interrupts */
    wlan_thread MainThread;

#ifdef REASSOCIATION
        /** thread to service mac events */
    wlan_thread ReassocThread;
#endif                          /* REASSOCIATION */

};

/** Wlan Adapter data structure*/
struct _wlan_adapter
{
        /** STATUS variables */
    WLAN_HARDWARE_STATUS HardwareStatus;
    /** Firmware release number */
    u32 FWReleaseNumber;
    /** Firmware capability information */
    u32 fwCapInfo;
    /** Chip revision number */
    u8 chip_rev;

        /** Command-related variables */
    /** Command sequence number */
    u16 SeqNum;
    /** Command controller nodes */
    CmdCtrlNode *CmdArray;
        /** Current Command */
    CmdCtrlNode *CurCmd;
    /** Current command return code */
    int CurCmdRetCode;

        /** Command Queues */
        /** Free command buffers */
    struct list_head CmdFreeQ;
        /** Pending command buffers */
    struct list_head CmdPendingQ;

        /** Async and Sync Event variables */
    u32 IntCounter;
    /** save int for DS/PS */
    u32 IntCounterSaved;
    /** Event cause */
    u32 EventCause;
        /** nickname */
    u8 nodeName[16];

        /** spin locks */
    spinlock_t QueueSpinLock __ATTRIB_ALIGN__;

        /** Timers */
    WLAN_DRV_TIMER MrvDrvCommandTimer __ATTRIB_ALIGN__;
    /** Command timer set flag */
    BOOLEAN CommandTimerIsSet;

#ifdef REASSOCIATION
        /**Reassociation timer*/
    BOOLEAN ReassocTimerIsSet;
    WLAN_DRV_TIMER MrvDrvTimer __ATTRIB_ALIGN__;
#endif                          /* REASSOCIATION */

        /** Event Queues */
    wait_queue_head_t ds_awake_q __ATTRIB_ALIGN__;

    /** His registry copy */
    u8 HisRegCpy;

    /** bg scan related variable */
    HostCmd_DS_802_11_BG_SCAN_CONFIG *bgScanConfig;
    /** BG scan configuration size */
    u32 bgScanConfigSize;

    /** WMM related variable*/
    WMM_DESC wmm;

        /** current ssid/bssid related parameters*/
    CurrentBSSParams_t CurBssParams;

    /** Infrastructure mode */
    WLAN_802_11_NETWORK_INFRASTRUCTURE InfrastructureMode;

    /** Attempted BSS descriptor */
    BSSDescriptor_t *pAttemptedBSSDesc;

    /** Prevous SSID */
    WLAN_802_11_SSID PreviousSSID;
    /** Previous BSSID */
    u8 PreviousBSSID[ETH_ALEN];

    /** Scan table */
    BSSDescriptor_t *ScanTable;
    /** Number of records in the scan table */
    u32 NumInScanTable;

    /** Scan type */
    u8 ScanType;
    /** Scan mode */
    u32 ScanMode;
    /** Specific scan time */
    u16 SpecificScanTime;
    /** Active scan time */
    u16 ActiveScanTime;
    /** Passive scan time */
    u16 PassiveScanTime;

    /** Beacon period */
    u16 BeaconPeriod;
    /** AdHoc G rate */
    u8 AdhocState;
    /** AdHoc link sensed flag */
    BOOLEAN AdhocLinkSensed;

#ifdef REASSOCIATION
        /** Reassociation on and off */
    BOOLEAN Reassoc_on;
    SEMAPHORE ReassocSem;
#endif                          /* REASSOCIATION */

    /** ATIM enabled flag */
    BOOLEAN ATIMEnabled;

        /** MAC address information */
    u8 CurrentAddr[ETH_ALEN];
    /** Multicast address list */
    u8 MulticastList[MRVDRV_MAX_MULTICAST_LIST_SIZE]
        [ETH_ALEN];
    /** Number of multicast addresses in the list */
    u32 NumOfMulticastMACAddr;

    /** Hardware rate drop mode */
    u16 HWRateDropMode;
    /** Rate bitmap */
    u16 RateBitmap;
    /** Threshold */
    u16 Threshold;
    /** Final rate */
    u16 FinalRate;
        /** control G Rates */
    BOOLEAN adhoc_grate_enabled;

    /** AdHoc channel */
    u8 AdhocChannel;
    /** AdHoc autoselect */
    u8 AdhocAutoSel;
    /** Fragmentation threshold */
    WLAN_802_11_FRAGMENTATION_THRESHOLD FragThsd;
    /** RTS threshold */
    WLAN_802_11_RTS_THRESHOLD RTSThsd;

    /** Data rate */
    u32 DataRate;
    /** Automatic data rate flag */
    BOOLEAN Is_DataRate_Auto;

    /** Listen interval */
    u16 ListenInterval;
    /** Tx retry count */
    u16 TxRetryCount;

    /** DTIM */
    u16 Dtim;

    /** Tx lock flag */
    BOOLEAN TxLockFlag;
    /** Gen NULL pkg */
    u16 gen_null_pkg;
    /** Current Tx lock */
    spinlock_t CurrentTxLock __ATTRIB_ALIGN__;

        /** NIC Operation characteristics */
    /** Current packet filter */
    u16 CurrentPacketFilter;
    /** Media connection status */
    u32 MediaConnectStatus;
    /** Region code */
    u16 RegionCode;
    /** Tx power level */
    u16 TxPowerLevel;
    /** Maximum Tx power level */
    u8 MaxTxPowerLevel;
    /** Minimum Tx power level */
    u8 MinTxPowerLevel;

        /** POWER MANAGEMENT AND PnP SUPPORT */
    BOOLEAN SurpriseRemoved;
    /** ATIM window */
    u16 AtimWindow;

    /** Power Save mode */
    u16 PSMode;                         /**< Wlan802_11PowerModeCAM=disable
					   Wlan802_11PowerModeMAX_PSP=enable */
    /** Multiple DTIM */
    u16 MultipleDtim;
    /** Beacon miss timeout */
    u16 BCNMissTimeOut;
    /** Power Save state */
    u32 PSState;
    /** Need to wakeup flag */
    BOOLEAN NeedToWakeup;

    /** Power save confirm sleep command */
    PS_CMD_ConfirmSleep PSConfirmSleep;
    /** Local listen interval */
    u16 LocalListenInterval;
    /** Null packet interval */
    u16 NullPktInterval;
    /** AdHoc awake period */
    u16 AdhocAwakePeriod;
    /** Firmware wakeup method */
    u16 fwWakeupMethod;
    /** Deep Sleep flag */
    BOOLEAN IsDeepSleep;
    /** Auto Deep Sleep enabled flag */
    BOOLEAN IsAutoDeepSleepEnabled;
    /** Enhanced Power Save enabled flag */
    BOOLEAN IsEnhancedPSEnabled;
    /** Device wakeup required flag */
    BOOLEAN bWakeupDevRequired;
    /** Number of wakeup tries */
    u32 WakeupTries;
    /** Host Sleep configured flag */
    BOOLEAN bHostSleepConfigured;
    /** Host Sleep configuration */
    HostCmd_DS_802_11_HOST_SLEEP_CFG HSCfg;
        /** ARP filter related variable */
    u8 ArpFilter[ARP_FILTER_MAX_BUF_SIZE];
    /** ARP filter size */
    u32 ArpFilterSize;
    /** Host Sleep activated flag */
    BOOLEAN HS_Activated;
    /** Host Sleep wait queue */
    wait_queue_head_t HS_wait_q __ATTRIB_ALIGN__;

        /** Encryption parameter */
    wlan_802_11_security_t SecInfo;

    /** WEP keys */
    MRVL_WEP_KEY WepKey[MRVL_NUM_WEP_KEY];
    /** Current WEP key index */
    u16 CurrentWepKeyIndex;

    /** Buffer for TLVs passed from the application to be inserted into the
     *    association request to firmware 
     */
    u8 mrvlAssocTlvBuffer[MRVDRV_ASSOC_TLV_BUF_SIZE];

    /** Length of the data stored in mrvlAssocTlvBuffer*/
    u8 mrvlAssocTlvBufferLen;

    /** Buffer to store the association response for application retrieval */
    u8 assocRspBuffer[MRVDRV_ASSOC_RSP_BUF_SIZE];

    /** Length of the data stored in assocRspBuffer */
    int assocRspSize;

    /** Generice IEEE IEs passed from the application to be inserted into the
     *    association request to firmware 
     */
    u8 genIeBuffer[MRVDRV_GENIE_BUF_SIZE];

    /** Length of the data stored in genIeBuffer */
    u8 genIeBufferLen;

    /** GTK set flag */
    BOOLEAN IsGTK_SET;

        /** Encryption Key*/
    u8 Wpa_ie[256];
    /** WPA IE length */
    u8 Wpa_ie_len;

    /** AES key material */
    HostCmd_DS_802_11_KEY_MATERIAL aeskey;

        /** Advanced Encryption Standard */
    BOOLEAN AdhocAESEnabled;

    /** Rx antenna mode */
    u16 RxAntennaMode;
    /** TX antenna mode */
    u16 TxAntennaMode;

    /** Factor for calculating beacon average */
    u16 bcn_avg_factor;
    /** Factor for calculating data average */
    u16 data_avg_factor;
    /** Last data RSSI */
    s16 DataRSSIlast;
    /** Last data Noise Floor */
    s16 DataNFlast;
    /** Average data RSSI */
    s16 DataRSSIAvg;
    /** Averag data Noise Floor */
    s16 DataNFAvg;
    /** Last beacon RSSI */
    s16 BcnRSSIlast;
    /** Last beacon Noise Floor */
    s16 BcnNFlast;
    /** Average beacon RSSI */
    s16 BcnRSSIAvg;
    /** Average beacon Noise Floor */
    s16 BcnNFAvg;
    /** Rx PD rate */
    u16 RxPDRate;
    /** Radio on flag */
    BOOLEAN RadioOn;

        /** F/W supported bands */
    u16 fw_bands;
        /** User selected bands (a/b/bg/abg) */
    u16 config_bands;
        /** User selected bands (a/b/g) to start adhoc network */
    u16 adhoc_start_band;

    /** Pointer to channel list last sent to the firmware for scanning */
    ChanScanParamSet_t *pScanChannels;

        /** Blue Tooth Co-existence Arbitration */
    HostCmd_DS_802_11_BCA_TIMESHARE bca_ts;

        /** sleep_params */
    SleepParams sp;

        /** sleep_period (Enhanced Power Save) */
    SleepPeriod sleep_period;

/** Maximum number of region channel */
#define	MAX_REGION_CHANNEL_NUM	2
        /** Region Channel data */
    REGION_CHANNEL region_channel[MAX_REGION_CHANNEL_NUM];

    /** Universal Channel data */
    REGION_CHANNEL universal_channel[MAX_REGION_CHANNEL_NUM];

        /** 11D and Domain Regulatory Data */
    wlan_802_11d_domain_reg_t DomainReg;
    /** Parsed region channel */
    parsed_region_chan_11d_t parsed_region_chan;

        /** FSM variable for 11d support */
    wlan_802_11d_state_t State11D;
    /** FSM variable for 11h support */
    wlan_11h_state_t state11h;
    /** FSM variable for MEAS support */
    wlan_meas_state_t stateMeas;
    /** Beaco buffer */
    u8 beaconBuffer[MAX_SCAN_BEACON_BUFFER];
    /** Pointer to valid beacon buffer end */
    u8 *pBeaconBufEnd;

    /** Log message  */
    HostCmd_DS_802_11_GET_LOG LogMsg;
    /** Scan probes */
    u16 ScanProbes;

    /** Tx packet control */
    u32 PktTxCtrl;

    /** Tx rate */
    u16 TxRate;

    /** WPS */
    wps_t wps;

    /** Debug */
    wlan_dbg dbg;
    /** Number of command timeouts */
    u32 num_cmd_timeout;
    /** Number of loops that main thread runs w/o sleep */
    u32 main_thread_loops;

    /** Driver spin lock */
    spinlock_t driver_lock;

        /** Algorithm */
    u16 Algorithm;
};

static inline int
wlan_copy_mcast_addr(wlan_adapter * Adapter, struct net_device *dev)
{
    int i = 0;
    struct dev_mc_list *mcptr = dev->mc_list;

    for (i = 0; i < dev->mc_count; i++) {
        memcpy(&Adapter->MulticastList[i], mcptr->dmi_addr, ETH_ALEN);
        mcptr = mcptr->next;
    }

    return i;
}

static inline int
os_upload_rx_packet(wlan_private * priv, struct sk_buff *skb)
{
    skb->dev = priv->wlan_dev.netdev;
    skb->protocol = eth_type_trans(skb, priv->wlan_dev.netdev);
    skb->ip_summed = CHECKSUM_UNNECESSARY;

    netif_rx(skb);

    return 0;
}

/**
 *  netif carrier_on/off and start(wake)/stop_queue handling
 *
 *           carrier_on      carrier_off     start_queue     stop_queue
 * open           x(connect)      x(disconnect)   x
 * close                          x                               x
 * assoc          x                               x
 * deauth                         x                               x
 * adhoc-start
 * adhoc-join
 * adhoc-link     x                               x
 * adhoc-bcnlost                  x                               x
 * scan-begin                     x                               x
 * scan-end       x                               x
 * ds-enter                       x                               x
 * ds-exit        x                               x
 * xmit                                                           x
 * xmit-done                                      x
 * tx-timeout
 */
static inline void
os_carrier_on(wlan_private * priv)
{
    if (!netif_carrier_ok(priv->wlan_dev.netdev) &&
        (priv->adapter->MediaConnectStatus == WlanMediaStateConnected) &&
        ((priv->adapter->InfrastructureMode != Wlan802_11IBSS) ||
         (priv->adapter->AdhocLinkSensed))) {
        netif_carrier_on(priv->wlan_dev.netdev);
    }
}

static inline void
os_carrier_off(wlan_private * priv)
{
    if (netif_carrier_ok(priv->wlan_dev.netdev)) {
        netif_carrier_off(priv->wlan_dev.netdev);
    }
}

static inline void
os_start_queue(wlan_private * priv)
{
    if (netif_queue_stopped(priv->wlan_dev.netdev) &&
        (priv->adapter->MediaConnectStatus == WlanMediaStateConnected) &&
        ((priv->adapter->InfrastructureMode != Wlan802_11IBSS) ||
         (priv->adapter->AdhocLinkSensed))) {
        netif_wake_queue(priv->wlan_dev.netdev);
    }
}

static inline void
os_stop_queue(wlan_private * priv)
{
    if (!netif_queue_stopped(priv->wlan_dev.netdev)) {
        netif_stop_queue(priv->wlan_dev.netdev);
    }
}

#endif /* _WLAN_DRV_H */
