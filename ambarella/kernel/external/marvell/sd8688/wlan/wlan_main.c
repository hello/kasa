/** @file wlan_main.c
  *
  * @brief This file contains the major functions in WLAN
  * driver. It includes init, exit, open, close and main
  * thread etc..
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
/**
  * @mainpage M-WLAN Linux Driver
  *
  * @section overview_sec Overview
  *
  * The M-WLAN is a Linux reference driver for Marvell
  * 802.11 (a/b/g) WLAN chipset.
  *
  * @section copyright_sec Copyright
  *
  * Copyright (C) 2003-2008, Marvell International Ltd.
  *
  */
/********************************************************
Change log:
    09/30/05: Add Doxygen format comments
    12/09/05: Add TX_QUEUE support
    01/05/06: Add kernel 2.6.x support
    01/11/06: Conditionalize new scan/join functions.
    01/12/06: Add TxLockFlag for UAPSD power save mode
              and Proprietary Periodic sleep support
********************************************************/

#include    "wlan_headers.h"
#include    "wlan_sdio_mmc.h"

const char driver_version[] =
    "sd8688-%s-" DRIVER_RELEASE_VERSION "-(" "FP" FPNUM ")"
#ifdef	DEBUG_LEVEL2
    "-dbg"
#endif
    " ";
/********************************************************
        Local Variables
********************************************************/

/** 100mW */
#define WLAN_TX_PWR_DEFAULT     20
/** 100mW */
#define WLAN_TX_PWR_US_DEFAULT      20
/** 50mW */
#define WLAN_TX_PWR_JP_DEFAULT      16
/** 100mW */
#define WLAN_TX_PWR_FR_100MW        20
/** 10mW */
#define WLAN_TX_PWR_FR_10MW     10
/** 100mW */
#define WLAN_TX_PWR_EMEA_DEFAULT    20

/* Format { Channel, Frequency (MHz), MaxTxPower } */
/* Band: 'B/G', Region: USA FCC/Canada IC */
static CHANNEL_FREQ_POWER channel_freq_power_US_BG[] = {
    {1, 2412, WLAN_TX_PWR_US_DEFAULT},
    {2, 2417, WLAN_TX_PWR_US_DEFAULT},
    {3, 2422, WLAN_TX_PWR_US_DEFAULT},
    {4, 2427, WLAN_TX_PWR_US_DEFAULT},
    {5, 2432, WLAN_TX_PWR_US_DEFAULT},
    {6, 2437, WLAN_TX_PWR_US_DEFAULT},
    {7, 2442, WLAN_TX_PWR_US_DEFAULT},
    {8, 2447, WLAN_TX_PWR_US_DEFAULT},
    {9, 2452, WLAN_TX_PWR_US_DEFAULT},
    {10, 2457, WLAN_TX_PWR_US_DEFAULT},
    {11, 2462, WLAN_TX_PWR_US_DEFAULT}
};

/* Band: 'B/G', Region: Europe ETSI */
static CHANNEL_FREQ_POWER channel_freq_power_EU_BG[] = {
    {1, 2412, WLAN_TX_PWR_EMEA_DEFAULT},
    {2, 2417, WLAN_TX_PWR_EMEA_DEFAULT},
    {3, 2422, WLAN_TX_PWR_EMEA_DEFAULT},
    {4, 2427, WLAN_TX_PWR_EMEA_DEFAULT},
    {5, 2432, WLAN_TX_PWR_EMEA_DEFAULT},
    {6, 2437, WLAN_TX_PWR_EMEA_DEFAULT},
    {7, 2442, WLAN_TX_PWR_EMEA_DEFAULT},
    {8, 2447, WLAN_TX_PWR_EMEA_DEFAULT},
    {9, 2452, WLAN_TX_PWR_EMEA_DEFAULT},
    {10, 2457, WLAN_TX_PWR_EMEA_DEFAULT},
    {11, 2462, WLAN_TX_PWR_EMEA_DEFAULT},
    {12, 2467, WLAN_TX_PWR_EMEA_DEFAULT},
    {13, 2472, WLAN_TX_PWR_EMEA_DEFAULT}
};

/* Band: 'B/G', Region: France */
static CHANNEL_FREQ_POWER channel_freq_power_FR_BG[] = {
    {1, 2412, WLAN_TX_PWR_FR_100MW},
    {2, 2417, WLAN_TX_PWR_FR_100MW},
    {3, 2422, WLAN_TX_PWR_FR_100MW},
    {4, 2427, WLAN_TX_PWR_FR_100MW},
    {5, 2432, WLAN_TX_PWR_FR_100MW},
    {6, 2437, WLAN_TX_PWR_FR_100MW},
    {7, 2442, WLAN_TX_PWR_FR_100MW},
    {8, 2447, WLAN_TX_PWR_FR_100MW},
    {9, 2452, WLAN_TX_PWR_FR_100MW},
    {10, 2457, WLAN_TX_PWR_FR_10MW},
    {11, 2462, WLAN_TX_PWR_FR_10MW},
    {12, 2467, WLAN_TX_PWR_FR_10MW},
    {13, 2472, WLAN_TX_PWR_FR_10MW}
};

/* Band: 'B/G', Region: Japan */
static CHANNEL_FREQ_POWER channel_freq_power_JPN41_BG[] = {
    {1, 2412, WLAN_TX_PWR_JP_DEFAULT},
    {2, 2417, WLAN_TX_PWR_JP_DEFAULT},
    {3, 2422, WLAN_TX_PWR_JP_DEFAULT},
    {4, 2427, WLAN_TX_PWR_JP_DEFAULT},
    {5, 2432, WLAN_TX_PWR_JP_DEFAULT},
    {6, 2437, WLAN_TX_PWR_JP_DEFAULT},
    {7, 2442, WLAN_TX_PWR_JP_DEFAULT},
    {8, 2447, WLAN_TX_PWR_JP_DEFAULT},
    {9, 2452, WLAN_TX_PWR_JP_DEFAULT},
    {10, 2457, WLAN_TX_PWR_JP_DEFAULT},
    {11, 2462, WLAN_TX_PWR_JP_DEFAULT},
    {12, 2467, WLAN_TX_PWR_JP_DEFAULT},
    {13, 2472, WLAN_TX_PWR_JP_DEFAULT}
};

/* Band: 'B/G', Region: Japan
static CHANNEL_FREQ_POWER channel_freq_power_JPN40_BG[] = {
    {14, 2484, WLAN_TX_PWR_JP_DEFAULT}
};
*/

/* Band: 'B/G', Region: Japan */
static CHANNEL_FREQ_POWER channel_freq_power_JPN40_BG[] = {
    {1, 2412, WLAN_TX_PWR_JP_DEFAULT},
    {2, 2417, WLAN_TX_PWR_JP_DEFAULT},
    {3, 2422, WLAN_TX_PWR_JP_DEFAULT},
    {4, 2427, WLAN_TX_PWR_JP_DEFAULT},
    {5, 2432, WLAN_TX_PWR_JP_DEFAULT},
    {6, 2437, WLAN_TX_PWR_JP_DEFAULT},
    {7, 2442, WLAN_TX_PWR_JP_DEFAULT},
    {8, 2447, WLAN_TX_PWR_JP_DEFAULT},
    {9, 2452, WLAN_TX_PWR_JP_DEFAULT},
    {10, 2457, WLAN_TX_PWR_JP_DEFAULT},
    {11, 2462, WLAN_TX_PWR_JP_DEFAULT},
    {12, 2467, WLAN_TX_PWR_JP_DEFAULT},
    {13, 2472, WLAN_TX_PWR_JP_DEFAULT},
    {14, 2484, WLAN_TX_PWR_JP_DEFAULT}
};

/* Band: 'A', Region: USA FCC, Canada IC, Spain, France */
static CHANNEL_FREQ_POWER channel_freq_power_A[] = {
    {36, 5180, WLAN_TX_PWR_US_DEFAULT},
    {40, 5200, WLAN_TX_PWR_US_DEFAULT},
    {44, 5220, WLAN_TX_PWR_US_DEFAULT},
    {48, 5240, WLAN_TX_PWR_US_DEFAULT},
    {52, 5260, WLAN_TX_PWR_US_DEFAULT},
    {56, 5280, WLAN_TX_PWR_US_DEFAULT},
    {60, 5300, WLAN_TX_PWR_US_DEFAULT},
    {64, 5320, WLAN_TX_PWR_US_DEFAULT},
    {100, 5500, WLAN_TX_PWR_US_DEFAULT},
    {104, 5520, WLAN_TX_PWR_US_DEFAULT},
    {108, 5540, WLAN_TX_PWR_US_DEFAULT},
    {112, 5560, WLAN_TX_PWR_US_DEFAULT},
    {116, 5580, WLAN_TX_PWR_US_DEFAULT},
    {120, 5600, WLAN_TX_PWR_US_DEFAULT},
    {124, 5620, WLAN_TX_PWR_US_DEFAULT},
    {128, 5640, WLAN_TX_PWR_US_DEFAULT},
    {132, 5660, WLAN_TX_PWR_US_DEFAULT},
    {136, 5680, WLAN_TX_PWR_US_DEFAULT},
    {140, 5700, WLAN_TX_PWR_US_DEFAULT},
    {149, 5745, WLAN_TX_PWR_US_DEFAULT},
    {153, 5765, WLAN_TX_PWR_US_DEFAULT},
    {157, 5785, WLAN_TX_PWR_US_DEFAULT},
    {161, 5805, WLAN_TX_PWR_US_DEFAULT},
    {165, 5825, WLAN_TX_PWR_US_DEFAULT}
};

/* Band: 'A', Region: Europe ETSI */
static CHANNEL_FREQ_POWER channel_freq_power_EU_A[] = {
    {36, 5180, WLAN_TX_PWR_EMEA_DEFAULT},
    {40, 5200, WLAN_TX_PWR_EMEA_DEFAULT},
    {44, 5220, WLAN_TX_PWR_EMEA_DEFAULT},
    {48, 5240, WLAN_TX_PWR_EMEA_DEFAULT},
    {52, 5260, WLAN_TX_PWR_EMEA_DEFAULT},
    {56, 5280, WLAN_TX_PWR_EMEA_DEFAULT},
    {60, 5300, WLAN_TX_PWR_EMEA_DEFAULT},
    {64, 5320, WLAN_TX_PWR_EMEA_DEFAULT},
    {100, 5500, WLAN_TX_PWR_EMEA_DEFAULT},
    {104, 5520, WLAN_TX_PWR_EMEA_DEFAULT},
    {108, 5540, WLAN_TX_PWR_EMEA_DEFAULT},
    {112, 5560, WLAN_TX_PWR_EMEA_DEFAULT},
    {116, 5580, WLAN_TX_PWR_EMEA_DEFAULT},
    {120, 5600, WLAN_TX_PWR_EMEA_DEFAULT},
    {124, 5620, WLAN_TX_PWR_EMEA_DEFAULT},
    {128, 5640, WLAN_TX_PWR_EMEA_DEFAULT},
    {132, 5660, WLAN_TX_PWR_EMEA_DEFAULT},
    {136, 5680, WLAN_TX_PWR_EMEA_DEFAULT},
    {140, 5700, WLAN_TX_PWR_EMEA_DEFAULT}
};

/* Band: 'A', Region: Japan */
static CHANNEL_FREQ_POWER channel_freq_power_JPN_A[] = {
    {36, 5180, WLAN_TX_PWR_EMEA_DEFAULT},
    {40, 5200, WLAN_TX_PWR_EMEA_DEFAULT},
    {44, 5220, WLAN_TX_PWR_EMEA_DEFAULT},
    {48, 5240, WLAN_TX_PWR_EMEA_DEFAULT},
    {52, 5260, WLAN_TX_PWR_EMEA_DEFAULT},
    {56, 5280, WLAN_TX_PWR_EMEA_DEFAULT},
    {60, 5300, WLAN_TX_PWR_EMEA_DEFAULT},
    {64, 5320, WLAN_TX_PWR_EMEA_DEFAULT},
    {100, 5500, WLAN_TX_PWR_EMEA_DEFAULT},
    {104, 5520, WLAN_TX_PWR_EMEA_DEFAULT},
    {108, 5540, WLAN_TX_PWR_EMEA_DEFAULT},
    {112, 5560, WLAN_TX_PWR_EMEA_DEFAULT},
    {116, 5580, WLAN_TX_PWR_EMEA_DEFAULT},
    {120, 5600, WLAN_TX_PWR_EMEA_DEFAULT},
    {124, 5620, WLAN_TX_PWR_EMEA_DEFAULT},
    {128, 5640, WLAN_TX_PWR_EMEA_DEFAULT},
    {132, 5660, WLAN_TX_PWR_EMEA_DEFAULT},
    {136, 5680, WLAN_TX_PWR_EMEA_DEFAULT},
    {140, 5700, WLAN_TX_PWR_EMEA_DEFAULT}
};

/**
 * the structure for channel, frequency and power
 */
typedef struct _region_cfp_table
{
    /** Region */
    u8 region;
    /** Frequency/Power */
    CHANNEL_FREQ_POWER *cfp_BG;
    /** No BG flag */
    int cfp_no_BG;
    /** Frequency/Power for band A */
    CHANNEL_FREQ_POWER *cfp_A;
    /** No A flag */
    int cfp_no_A;
} region_cfp_table_t;

/**
 * the structure for the mapping between region and CFP
 */
static region_cfp_table_t region_cfp_table[] = {
    {0x10,                      /* US FCC */
     channel_freq_power_US_BG,
     sizeof(channel_freq_power_US_BG) / sizeof(CHANNEL_FREQ_POWER),
     channel_freq_power_A,
     sizeof(channel_freq_power_A) / sizeof(CHANNEL_FREQ_POWER),
     }
    ,
    {0x20,                      /* CANADA IC */
     channel_freq_power_US_BG,
     sizeof(channel_freq_power_US_BG) / sizeof(CHANNEL_FREQ_POWER),
     channel_freq_power_A,
     sizeof(channel_freq_power_A) / sizeof(CHANNEL_FREQ_POWER),
     }
    ,
    {0x30, /*EU*/ channel_freq_power_EU_BG,
     sizeof(channel_freq_power_EU_BG) / sizeof(CHANNEL_FREQ_POWER),
     channel_freq_power_EU_A,
     sizeof(channel_freq_power_EU_A) / sizeof(CHANNEL_FREQ_POWER),
     }
    ,
    {0x32, /*FRANCE*/ channel_freq_power_FR_BG,
     sizeof(channel_freq_power_FR_BG) / sizeof(CHANNEL_FREQ_POWER),
     channel_freq_power_A,
     sizeof(channel_freq_power_A) / sizeof(CHANNEL_FREQ_POWER),
     }
    ,
    {0x40, /*JAPAN*/ channel_freq_power_JPN40_BG,
     sizeof(channel_freq_power_JPN40_BG) / sizeof(CHANNEL_FREQ_POWER),
     channel_freq_power_JPN_A,
     sizeof(channel_freq_power_JPN_A) / sizeof(CHANNEL_FREQ_POWER),
     }
    ,
    {0x41, /*JAPAN*/ channel_freq_power_JPN41_BG,
     sizeof(channel_freq_power_JPN41_BG) / sizeof(CHANNEL_FREQ_POWER),
     channel_freq_power_JPN_A,
     sizeof(channel_freq_power_JPN_A) / sizeof(CHANNEL_FREQ_POWER),
     }
    ,
/*Add new region here */
};

/********************************************************
        Global Variables
********************************************************/

/**
 * the rates supported by the card
 */
u8 WlanDataRates[WLAN_SUPPORTED_RATES] =
    { 0x02, 0x04, 0x0B, 0x16, 0x00, 0x0C, 0x12,
    0x18, 0x24, 0x30, 0x48, 0x60, 0x6C, 0x00
};

/**
 * the rates supported in B mode
 */
u8 SupportedRates_B[B_SUPPORTED_RATES] = { 0x82, 0x84, 0x0b, 0x16, 0, 0, 0, 0 };

/**
 * the rates supported in G mode
 */
u8 SupportedRates_G[G_SUPPORTED_RATES] =
    { 0x82, 0x84, 0x8b, 0x0c, 0x12, 0x96, 0x18, 0x24,
    0x30, 0x48, 0x60, 0x6c, 0
};

/**
 * the rates supported in A mode
 */
u8 SupportedRates_A[A_SUPPORTED_RATES] =
    { 0x8c, 0x12, 0x98, 0x24, 0xb0, 0x48, 0x60, 0x6c, 0 };

/**
 * the rates supported in A mode for ad-hoc
 */
u8 AdhocRates_A[A_SUPPORTED_RATES] =
    { 0x8c, 0x12, 0x98, 0x24, 0xb0, 0x48, 0x60, 0x6c, 0 };

/**
 * the rates supported for ad-hoc G mode
 */
u8 AdhocRates_G[G_SUPPORTED_RATES] =
    { 0x82, 0x84, 0x8b, 0x96, 0x0c, 0x12, 0x18, 0x24, 0x30, 0x48, 0x60, 0x6c,
0 };

/**
 * the rates supported for ad-hoc B mode
 */
u8 AdhocRates_B[4] = { 0x82, 0x84, 0x8b, 0x96 };

/**
 * the global variable of a pointer to wlan_private
 * structure variable
 */
wlan_private *wlanpriv = NULL;

u32 DSFreqList[15] = {
    0, 2412000, 2417000, 2422000, 2427000, 2432000, 2437000, 2442000,
    2447000, 2452000, 2457000, 2462000, 2467000, 2472000, 2484000
};

/**
 * the table to keep region code
 */
u16 RegionCodeToIndex[MRVDRV_MAX_REGION_CODE] =
    { 0x10, 0x20, 0x30, 0x31, 0x32, 0x40, 0x41 };

/********************************************************
        Local Functions
********************************************************/

/**
 *  @brief This function opens the network device
 *
 *  @param dev     A pointer to net_device structure
 *  @return        WLAN_STATUS_SUCCESS
 */
static int
wlan_open(struct net_device *dev)
{
    wlan_private *priv = netdev_priv(dev);
    wlan_adapter *adapter = priv->adapter;

    ENTER();

    if (MODULE_GET == 0) {
        LEAVE();
        return WLAN_STATUS_FAILURE;
    }

    priv->open = TRUE;

    if ((adapter->MediaConnectStatus == WlanMediaStateConnected)
        && (adapter->InfrastructureMode != Wlan802_11IBSS
            || adapter->AdhocLinkSensed == TRUE))
        os_carrier_on(priv);
    else
        os_carrier_off(priv);

    os_start_queue(priv);
    wmm_start_queue(priv);

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/**
 *  @brief This function closes the network device
 *
 *  @param dev     A pointer to net_device structure
 *  @return        WLAN_STATUS_SUCCESS
 */
static int
wlan_close(struct net_device *dev)
{
    wlan_private *priv = netdev_priv(dev);

    ENTER();

    if (priv->adapter)
        wlan_clean_txrx(priv);

    MODULE_PUT;

    priv->open = FALSE;

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** Wait until Host Sleep state changed, timeout in millisecond */
#define WAIT_UNTIL_HS_STATE_CHANGED 5000

/**
 *  @brief This function enables Host Sleep.
 *
 *  @param priv    A pointer to wlan_private
 *  @return        WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
int
wlan_enable_hs(wlan_private * priv)
{
    wlan_adapter *Adapter = priv->adapter;
    int ret = WLAN_STATUS_SUCCESS;

    ENTER();

    /* suspend not supported in PPS/UAPSD mode */
    if ((Adapter->PSMode != Wlan802_11PowerModeCAM)
        && Adapter->sleep_period.period) {
        ret = WLAN_STATUS_FAILURE;
        PRINTM(MSG,
               "wlan_enable_hs: suspend not supported. PSMode=%d SleepPeriod=%d\n",
               Adapter->PSMode, Adapter->sleep_period.period);
    } else if (!Adapter->HS_Activated) {
        ret = wlan_prepare_cmd(priv,
                               HostCmd_CMD_802_11_HOST_SLEEP_CFG,
                               0, HostCmd_OPTION_WAITFORRSP, 0,
                               &Adapter->HSCfg);
        if (!os_wait_interruptible_timeout
            (Adapter->HS_wait_q, Adapter->HS_Activated,
             WAIT_UNTIL_HS_STATE_CHANGED)) {
            ret = WLAN_STATUS_FAILURE;
            PRINTM(MSG, "wlan_enable_hs: timeout:%d, %d, %d, %d, %d\n",
                   Adapter->MediaConnectStatus, Adapter->PSState,
                   Adapter->bWakeupDevRequired, Adapter->WakeupTries,
                   Adapter->HS_Activated);
        }
    } else {
        PRINTM(MSG, "wlan_enable_hs: already activated:%d, %d, %d, %d, %d\n",
               Adapter->MediaConnectStatus, Adapter->PSState,
               Adapter->bWakeupDevRequired, Adapter->WakeupTries,
               Adapter->HS_Activated);
    }

    LEAVE();
    return ret;
}

/**
 *  @brief This function disables Host Sleep.
 *
 *  @param priv    A pointer to wlan_private
 *  @return        WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
int
wlan_disable_hs(wlan_private * priv)
{
    wlan_adapter *Adapter = priv->adapter;
    HostCmd_DS_802_11_HOST_SLEEP_CFG hscfg;
    int ret = WLAN_STATUS_SUCCESS;

    ENTER();

    if (Adapter->HS_Activated) {
        hscfg.conditions = HOST_SLEEP_CFG_CANCEL;
        hscfg.gpio = Adapter->HSCfg.gpio;
        hscfg.gap = Adapter->HSCfg.gap;
        ret = wlan_prepare_cmd(priv,
                               HostCmd_CMD_802_11_HOST_SLEEP_CFG,
                               0, HostCmd_OPTION_WAITFORRSP, 0, &hscfg);
    } else {
        PRINTM(MSG,
               "wlan_disable_hs: already de-activated:%d, %d, %d, %d, %d\n",
               Adapter->MediaConnectStatus, Adapter->PSState,
               Adapter->bWakeupDevRequired, Adapter->WakeupTries,
               Adapter->HS_Activated);
    }

    LEAVE();
    return ret;
}

/**
 *  @brief This function is a callback function. It is called by
 *  bus driver to suspend and resume
 *
 *  @param card    A pointer to card
 *  @param suspend An integer specifies suspend or resume
 *  @return        WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
int
wlan_pm(void *card, int suspend)
{
    wlan_private *priv = wlanpriv;
    struct net_device *dev = priv->wlan_dev.netdev;
    int ret = WLAN_STATUS_SUCCESS;

    ENTER();

    PRINTM(MSG, "WLAN PM CALLBACK: %s\n", suspend ? "PM_SUSPEND" : "PM_RESUME");
    if (suspend) {
        ret = wlan_enable_hs(priv);
        if (!ret) {
            /* Detach the network interface if the network is running */
            netif_device_detach(dev);
            PRINTM(INFO, "netif_device_detach()\n");
            /* Stop SDIO bus clock */
            sbi_set_bus_clock(priv, FALSE);
        }
    } else {
        ret = wlan_disable_hs(priv);
        if (!ret) {
            /* Start SDIO bus clock */
            sbi_set_bus_clock(priv, TRUE);
            /* Attach the network interface if the network is running */
            netif_device_attach(dev);
            PRINTM(INFO, "netif_device_attach()\n");
        }
    }

    LEAVE();
    return ret;
}

/**
 *  @brief This function handles packet transmission
 *
 *  @param skb     A pointer to sk_buff structure
 *  @param dev     A pointer to net_device structure
 *  @return        WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
static int
wlan_hard_start_xmit(struct sk_buff *skb, struct net_device *dev)
{
    int ret;
    wlan_private *priv = netdev_priv(dev);

    ret = WLAN_STATUS_SUCCESS;

    ENTER();

    PRINTM(DATA, "Data <= kernel @ %lu\n", os_time_get());

    if (wlan_tx_packet(priv, skb)) {
        /* Transmit failed */
        ret = WLAN_STATUS_FAILURE;
    }

    LEAVE();
    return ret;
}

/**
 *  @brief This function handles the timeout of packet
 *  transmission
 *
 *  @param dev     A pointer to net_device structure
 *  @return        n/a
 */
static void
wlan_tx_timeout(struct net_device *dev)
{
    wlan_private *priv = netdev_priv(dev);

    ENTER();

    PRINTM(DATA, "Tx timeout @ %lu\n", os_time_get());
    UpdateTransStart(dev);

    priv->adapter->dbg.num_tx_timeout++;

    priv->adapter->IntCounter++;
    wake_up_interruptible(&priv->MainThread.waitQ);

    LEAVE();
}

/**
 *  @brief This function returns the network statistics
 *
 *  @param dev     A pointer to wlan_private structure
 *  @return        A pointer to net_device_stats structure
 */
static struct net_device_stats *
wlan_get_stats(struct net_device *dev)
{
    wlan_private *priv = netdev_priv(dev);

    return &priv->stats;
}

/**
 *  @brief This function sets the MAC address to firmware.
 *
 *  @param dev     A pointer to wlan_private structure
 *  @param addr    MAC address to set
 *  @return        WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
static int
wlan_set_mac_address(struct net_device *dev, void *addr)
{
    int ret = WLAN_STATUS_SUCCESS;
    wlan_private *priv = netdev_priv(dev);
    wlan_adapter *Adapter = priv->adapter;
    struct sockaddr *pHwAddr = (struct sockaddr *) addr;

    ENTER();

    memset(Adapter->CurrentAddr, 0, ETH_ALEN);

    /* dev->dev_addr is 8 bytes */
    HEXDUMP("dev->dev_addr:", dev->dev_addr, ETH_ALEN);

    HEXDUMP("addr:", pHwAddr->sa_data, ETH_ALEN);
    memcpy(Adapter->CurrentAddr, pHwAddr->sa_data, ETH_ALEN);

    ret = wlan_prepare_cmd(priv, HostCmd_CMD_802_11_MAC_ADDRESS,
                           HostCmd_ACT_GEN_SET,
                           HostCmd_OPTION_WAITFORRSP, 0, NULL);

    if (ret) {
        PRINTM(INFO, "set mac address failed\n");
        ret = WLAN_STATUS_FAILURE;
        goto done;
    }

    HEXDUMP("Adapter->MacAddr:", Adapter->CurrentAddr, ETH_ALEN);
    memcpy(dev->dev_addr, Adapter->CurrentAddr, ETH_ALEN);

  done:
    LEAVE();
    return ret;
}

/**
 *  @brief This function sets multicast addresses to firmware
 *
 *  @param dev     A pointer to net_device structure
 *  @return        n/a
 */
static void
wlan_set_multicast_list(struct net_device *dev)
{
    wlan_private *priv = netdev_priv(dev);
    wlan_adapter *Adapter = priv->adapter;
    int OldPacketFilter;

    ENTER();

    OldPacketFilter = Adapter->CurrentPacketFilter;

    if (dev->flags & IFF_PROMISC) {
        PRINTM(INFO, "Enable Promiscuous mode\n");
        Adapter->CurrentPacketFilter |= HostCmd_ACT_MAC_PROMISCUOUS_ENABLE;
        Adapter->CurrentPacketFilter &= ~HostCmd_ACT_MAC_ALL_MULTICAST_ENABLE;
    } else {
        /* Multicast */
        Adapter->CurrentPacketFilter &= ~HostCmd_ACT_MAC_PROMISCUOUS_ENABLE;

        if (dev->flags & IFF_ALLMULTI || dev->mc_count >
            MRVDRV_MAX_MULTICAST_LIST_SIZE) {
            PRINTM(INFO, "Enabling All Multicast!\n");
            Adapter->CurrentPacketFilter |=
                HostCmd_ACT_MAC_ALL_MULTICAST_ENABLE;
        } else {
            Adapter->CurrentPacketFilter &=
                ~HostCmd_ACT_MAC_ALL_MULTICAST_ENABLE;

            if (!dev->mc_count) {
                PRINTM(INFO, "No multicast addresses - "
                       "disabling multicast!\n");

            } else {
                int i;

                Adapter->NumOfMulticastMACAddr =
                    wlan_copy_mcast_addr(Adapter, dev);

                PRINTM(INFO, "Multicast addresses: %d\n", dev->mc_count);

                for (i = 0; i < dev->mc_count; i++) {
                    PRINTM(INFO, "Multicast address %d:"
                           "%x %x %x %x %x %x\n", i,
                           Adapter->MulticastList[i][0],
                           Adapter->MulticastList[i][1],
                           Adapter->MulticastList[i][2],
                           Adapter->MulticastList[i][3],
                           Adapter->MulticastList[i][4],
                           Adapter->MulticastList[i][5]);
                }
                /* set multicast addresses to firmware */
                wlan_prepare_cmd(priv, HostCmd_CMD_MAC_MULTICAST_ADR,
                                 HostCmd_ACT_GEN_SET, 0, 0, NULL);
            }
        }
    }

    if (Adapter->CurrentPacketFilter != OldPacketFilter)
        wlan_prepare_cmd(priv,
                         HostCmd_CMD_MAC_CONTROL,
                         0, 0, 0, &Adapter->CurrentPacketFilter);

    LEAVE();
}

/**
 *  @brief This function handles the major job in WLAN driver.
 *  it handles the event generated by firmware, rx data received
 *  from firmware and tx data sent from kernel.
 *
 *  @param data    A pointer to wlan_thread structure
 *  @return        WLAN_STATUS_SUCCESS
 */
static int
wlan_service_main_thread(void *data)
{
    wlan_thread *thread = data;
    wlan_private *priv = thread->priv;
    wlan_adapter *Adapter = priv->adapter;
    wait_queue_t wait;
    u8 ireg = 0;
    unsigned long driver_flags;

    OS_INTERRUPT_SAVE_AREA;

    ENTER();

    wlan_activate_thread(thread);

    init_waitqueue_entry(&wait, current);

    current->flags |= PF_NOFREEZE;

    wlan_meas_init(priv);

    PRINTM(INFO, "11H: init 11H\n");
    wlan_11h_init(priv);

    wmm_init(priv);

    for (;;) {
#define MAX_MAIN_THREAD_LOOPS (10000)
        if (++Adapter->main_thread_loops > MAX_MAIN_THREAD_LOOPS) {
            panic("main-thread dead loop detected: %u\n"
                  "IntCnt=%d\n"
                  "Adapter->HisRegCpy=%#x\n"
                  "CurCmd=%p CmdPending=%s\n"
                  "Connect=%s\n"
                  "cmd_sent=%s\n"
                  "data_sent=%s\n"
                  "IsDeepSleep=%d\n"
                  "PSMode=%d PSState=%d\n"
                  "WakeupDevReq=%d\n"
                  "HS_Activated=%d\n"
                  "WakeupTries=%d\n", Adapter->main_thread_loops,
                  Adapter->IntCounter,
                  Adapter->HisRegCpy,
                  Adapter->CurCmd,
                  list_empty(&Adapter->CmdPendingQ) ? "N" : "Y",
                  (Adapter->MediaConnectStatus ==
                   WlanMediaStateConnected) ? "Y" : "N",
                  (priv->wlan_dev.cmd_sent) ? "TRUE" : "FALSE",
                  (priv->wlan_dev.data_sent) ? "TRUE" : "FALSE",
                  Adapter->IsDeepSleep, Adapter->PSMode, Adapter->PSState,
                  Adapter->bWakeupDevRequired, Adapter->HS_Activated,
                  Adapter->WakeupTries);
        }

        add_wait_queue(&thread->waitQ, &wait);
        OS_SET_THREAD_STATE(TASK_INTERRUPTIBLE);

        TX_DISABLE;

        if ((Adapter->WakeupTries) ||
            (Adapter->PSState == PS_STATE_SLEEP
             && !Adapter->bWakeupDevRequired) ||
            (!Adapter->IntCounter &&
             Adapter->PSState == PS_STATE_PRE_SLEEP) ||
            (!Adapter->IntCounter
             && (priv->wlan_dev.data_sent || Adapter->TxLockFlag
                 || wmm_lists_empty(priv) || wmm_is_queue_stopped(priv))
             && (priv->wlan_dev.cmd_sent || Adapter->CurCmd ||
                 list_empty(&Adapter->CmdPendingQ))
            )
            ) {
            PRINTM(INFO, "main-thread sleeping... "
                   "HS_Act=%s WakeupReq=%s Conn=%s PS_Mode=%d PS_State=%d\n",
                   (Adapter->HS_Activated) ? "Y" : "N",
                   (Adapter->bWakeupDevRequired) ? "Y" : "N",
                   (Adapter->MediaConnectStatus) ? "Y" : "N",
                   Adapter->PSMode, Adapter->PSState);

            TX_RESTORE;
            Adapter->main_thread_loops = 0;
            schedule();
            PRINTM(INFO, "main-thread waking up: IntCnt=%d "
                   "CurCmd=%s CmdPending=%s\n"
                   "                       Connect=%s "
                   "cmd_sent=%s data_sent=%s\n",
                   Adapter->IntCounter,
                   (Adapter->CurCmd) ? "Y" : "N",
                   list_empty(&Adapter->CmdPendingQ) ? "N" : "Y",
                   (Adapter->MediaConnectStatus) ? "Y" : "N",
                   (priv->wlan_dev.cmd_sent) ? "TRUE" : "FALSE",
                   (priv->wlan_dev.data_sent) ? "TRUE" : "FALSE");
        } else {
            TX_RESTORE;
        }

        OS_SET_THREAD_STATE(TASK_RUNNING);
        remove_wait_queue(&thread->waitQ, &wait);

        if (kthread_should_stop() || Adapter->SurpriseRemoved) {
            PRINTM(INFO, "main-thread: break from main thread: "
                   "SurpriseRemoved=0x%x\n", Adapter->SurpriseRemoved);
            break;
        }

        if (Adapter->IntCounter) {
            OS_INT_DISABLE(priv, driver_flags);
            Adapter->IntCounter = 0;
            OS_INT_RESTORE(priv, driver_flags);

            if (sbi_get_int_status(priv, &ireg)) {
                PRINTM(ERROR,
                       "main-thread: reading HOST_INT_STATUS_REG failed\n");
                continue;
            }

            /* If we have a valid interrupt then we have something to do, so
               we are not running in a dead loop */
            if (ireg)
                Adapter->main_thread_loops = 0;

            OS_INT_DISABLE(priv, driver_flags);
            Adapter->HisRegCpy |= ireg;
            OS_INT_RESTORE(priv, driver_flags);
            PRINTM(INTR, "INT: status = 0x%x\n", Adapter->HisRegCpy);
        } else if (Adapter->bWakeupDevRequired) {
            Adapter->WakeupTries++;
            PRINTM(CMND,
                   "Wakeup device... WakeupReq=%s Conn=%s PS_Mode=%d PS_State=%d @ %lu\n",
                   (Adapter->bWakeupDevRequired) ? "Y" : "N",
                   (priv->adapter->MediaConnectStatus) ? "Y" : "N",
                   priv->adapter->PSMode, priv->adapter->PSState,
                   os_time_get());

            /* Wake up device */
            if (sbi_exit_deep_sleep(priv))
                PRINTM(MSG, "main-thread: wakeup device failed\n");
            continue;
        }

        /* Command response? */
        if (Adapter->HisRegCpy & HIS_CmdUpLdRdy) {
            OS_INT_DISABLE(priv, driver_flags);
            Adapter->HisRegCpy &= ~HIS_CmdUpLdRdy;
            OS_INT_RESTORE(priv, driver_flags);

            wlan_process_cmdresp(priv);
        }

        /* Any Card Event */
        if (Adapter->HisRegCpy & HIS_CardEvent) {
            OS_INT_DISABLE(priv, driver_flags);
            Adapter->HisRegCpy &= ~HIS_CardEvent;
            OS_INT_RESTORE(priv, driver_flags);

            wlan_process_event(priv);
        }

        /* Check if we need to confirm Sleep Request received previously */
        if (Adapter->PSState == PS_STATE_PRE_SLEEP) {
            if (!priv->wlan_dev.cmd_sent && !Adapter->CurCmd) {
                ASSERT(Adapter->MediaConnectStatus == WlanMediaStateConnected);
                wlan_ps_cond_check(priv, (u16) Adapter->PSMode);
            }
        }

        /* The PS state is changed during processing of Sleep Request event
           above */
        if ((Adapter->PSState == PS_STATE_SLEEP)
            || (Adapter->PSState == PS_STATE_PRE_SLEEP))
            continue;

        if (Adapter->IsDeepSleep)
            continue;

        /* The HS_Activated flag is changed during processing of HS_Activate
           command resp */
        /* We cannot send command or data if HS_Activated and
           WakeupDevRequired are TRUE */
        if (Adapter->HS_Activated && Adapter->bWakeupDevRequired) {
            PRINTM(INFO, "main-thread: cannot send command or data, "
                   "HS_Activated=%d\n", Adapter->HS_Activated);
            continue;
        }

        /* Execute the next command */
        if (!priv->wlan_dev.cmd_sent && !Adapter->CurCmd)
            wlan_exec_next_cmd(priv);

        if (!priv->wlan_dev.data_sent
            && !wmm_lists_empty(priv) && !wmm_is_queue_stopped(priv)) {
            if ((Adapter->PSState == PS_STATE_FULL_POWER)
                || (Adapter->sleep_period.period == 0)
                || (Adapter->TxLockFlag == FALSE))
                wmm_process_tx(priv);

        }

    }

    wlan_deactivate_thread(thread);

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

static const struct net_device_ops wlan_netdev_ops = {
	.ndo_open		= wlan_open,
	.ndo_stop		= wlan_close,
	.ndo_start_xmit		= wlan_hard_start_xmit,
	.ndo_do_ioctl		= wlan_do_ioctl,
	.ndo_set_multicast_list	= wlan_set_multicast_list,
	.ndo_set_mac_address 	= wlan_set_mac_address,
	.ndo_validate_addr	= eth_validate_addr,
	.ndo_change_mtu		= eth_change_mtu,
	.ndo_tx_timeout		= wlan_tx_timeout,
	.ndo_get_stats		= wlan_get_stats,
};

/**
 * @brief This function adds the card. it will probe the
 * card, allocate the wlan_priv and initialize the device.
 *
 *  @param card    A pointer to card
 *  @return        A pointer to wlan_private structure
 */
wlan_private *
wlan_add_card(void *card)
{
    struct net_device *dev = NULL;
    wlan_private *priv = NULL;

    ENTER();

    /* Allocate an Ethernet device and register it */
    if (!(dev = alloc_etherdev(sizeof(wlan_private)))) {
        PRINTM(MSG, "Init ethernet device failed!\n");
        LEAVE();
        return NULL;
    }
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24)
    /* Allocate device name */
    if (dev_alloc_name(dev, "wlan%d") < 0) {
        PRINTM(ERROR, "Could not allocate device name!\n");
        goto err_kmalloc;
    }
#endif

    priv = netdev_priv(dev);

    dev->ml_priv = priv;
    /* allocate buffer for wlan_adapter */
    if (!(priv->adapter = kmalloc(sizeof(wlan_adapter), GFP_KERNEL))) {
        PRINTM(MSG, "Allocate buffer for wlan_adapter failed!\n");
        goto err_kmalloc;
    }

    /* init wlan_adapter */
    memset(priv->adapter, 0, sizeof(wlan_adapter));

    priv->wlan_dev.netdev = dev;
    priv->wlan_dev.card = card;
    ((struct sdio_mmc_card *) card)->priv = priv;
    wlanpriv = priv;

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24)
    SET_MODULE_OWNER(dev);
#endif

    /* Setup the OS Interface to our functions */
    dev->netdev_ops = &wlan_netdev_ops;

    dev->watchdog_timeo = MRVDRV_DEFAULT_WATCHDOG_TIMEOUT;
    dev->hard_header_len += sizeof(TxPD);
    dev->hard_header_len += SDIO_HEADER_LEN;
    dev->hard_header_len += HEADER_ALIGNMENT;

#ifdef  WIRELESS_EXT
#if WIRELESS_EXT < 21
    dev->get_wireless_stats = wlan_get_wireless_stats;
#endif
    dev->wireless_handlers = (struct iw_handler_def *) &wlan_handler_def;
#endif
#define NETIF_F_DYNALLOC 16
    dev->features |= NETIF_F_DYNALLOC;
    dev->flags |= IFF_BROADCAST | IFF_MULTICAST | IFF_UP;

    /* init SW */
    if (wlan_init_sw(priv)) {
        PRINTM(FATAL, "Software Init Failed\n");
        goto err_kmalloc;
    }

    PRINTM(INFO, "Starting kthread...\n");
    priv->MainThread.priv = priv;
    wlan_create_thread(wlan_service_main_thread,
                       &priv->MainThread, "wlan_main_service");

    ConfigureThreadPriority();

#ifdef REASSOCIATION
    priv->ReassocThread.priv = priv;
    wlan_create_thread(wlan_reassociation_thread,
                       &priv->ReassocThread, "wlan_reassoc_service");
#endif /* REASSOCIATION */

    while ((priv->MainThread.pid == 0)
#ifdef REASSOCIATION
           || (priv->ReassocThread.pid == 0)
#endif
        ) {
        os_sched_timeout(2);
    }

    /*
     * Register the device. Fill up the private data structure with
     * relevant information from the card and request for the required
     * IRQ.
     */

    if (sbi_register_dev(priv) < 0) {
        PRINTM(FATAL, "Failed to register wlan device!\n");
        goto err_registerdev;
    }

    SET_NETDEV_DEV(dev, priv->hotplug_device);

    /* init FW and HW */
    if (wlan_init_fw(priv)) {
        PRINTM(FATAL, "Firmware Init Failed\n");
        goto err_init_fw;
    }

    if (register_netdev(dev)) {
        printk(KERN_ERR "Cannot register network device!\n");
        goto err_init_fw;
    }
    PRINTM(INFO, "%s: WLAN 802.11 Adapter revision 0x%02X\n",
           dev->name, priv->adapter->chip_rev);

#ifdef CONFIG_PROC_FS
    wlan_proc_entry(priv, dev);
#ifdef PROC_DEBUG
    wlan_debug_entry(priv, dev);
#endif
#endif /* CONFIG_PROC_FS */

    LEAVE();
    return priv;

  err_init_fw:
    sbi_unregister_dev(priv);

  err_registerdev:
    priv->adapter->SurpriseRemoved = TRUE;
    if (priv->MainThread.pid) {
        /* Stop the thread servicing the interrupts */
        wake_up_interruptible(&priv->MainThread.waitQ);
    }
#ifdef REASSOCIATION
    if (priv->ReassocThread.pid) {
        wake_up_interruptible(&priv->ReassocThread.waitQ);
    }
#endif /* REASSOCIATION */

    /* waiting for main thread quit */
    while (priv->MainThread.pid
#ifdef REASSOCIATION
           || priv->ReassocThread.pid
#endif
        ) {
        os_sched_timeout(2);
    }
  err_kmalloc:
    wlan_free_adapter(priv);
    priv->wlan_dev.netdev = NULL;
    free_netdev(dev);
    ((struct sdio_mmc_card *) card)->priv = NULL;
    wlanpriv = NULL;

    LEAVE();
    return NULL;
}

/**
 *  @brief This function removes the card.
 *
 *  @param card    A pointer to card
 *  @return        WLAN_STATUS_SUCCESS
 */
int
wlan_remove_card(void *card)
{
    wlan_private *priv = wlanpriv;
    wlan_adapter *Adapter = NULL;
    struct net_device *dev;
    union iwreq_data wrqu;

    ENTER();

    if (!priv || !(Adapter = priv->adapter)) {
        LEAVE();
        return WLAN_STATUS_SUCCESS;
    }
    Adapter->SurpriseRemoved = TRUE;

    if (Adapter->ReassocTimerIsSet == TRUE) {
        wlan_cancel_timer(&Adapter->MrvDrvTimer);
        Adapter->ReassocTimerIsSet = FALSE;
    }
    if (Adapter->MediaConnectStatus == WlanMediaStateConnected) {
        Adapter->MediaConnectStatus = WlanMediaStateDisconnected;
        memset(wrqu.ap_addr.sa_data, 0xaa, ETH_ALEN);
        wrqu.ap_addr.sa_family = ARPHRD_ETHER;
        wireless_send_event(priv->wlan_dev.netdev, SIOCGIWAP, &wrqu, NULL);
        wlan_clean_txrx(priv);
    }

    /* Release all pending commands */
    wlan_clear_pending_cmd(priv);

    dev = priv->wlan_dev.netdev;
    /* Last reference is our one */
    PRINTM(INFO, "refcnt = %d\n", atomic_read(&dev->refcnt));

    PRINTM(INFO, "netdev_finish_unregister: %s%s\n", dev->name,
           (dev->features & NETIF_F_DYNALLOC) ? "" : ", old style");
    if (dev->reg_state == NETREG_REGISTERED)
        unregister_netdev(dev);
    PRINTM(INFO, "Unregister finish\n");

    if (priv->MainThread.pid) {
        /* Stop the thread servicing the interrupts */
        wake_up_interruptible(&priv->MainThread.waitQ);
    }
#ifdef REASSOCIATION
    if (priv->ReassocThread.pid) {
        wake_up_interruptible(&priv->ReassocThread.waitQ);
    }
#endif /* REASSOCIATION */

    /* waiting for thread quit */
    while (priv->MainThread.pid
#ifdef REASSOCIATION
           || priv->ReassocThread.pid
#endif
        ) {
        os_sched_timeout(1);
    }

    wake_up_interruptible(&Adapter->HS_wait_q);

    if (Adapter->IsDeepSleep == TRUE) {
        Adapter->IsDeepSleep = FALSE;
        wake_up_interruptible(&Adapter->ds_awake_q);
    }

#ifdef CONFIG_PROC_FS
#ifdef PROC_DEBUG
    wlan_debug_remove(priv);
#endif
    wlan_proc_remove(priv);
#endif

    PRINTM(INFO, "unregister device\n");
    sbi_unregister_dev(priv);

    PRINTM(INFO, "Free Adapter\n");
    wlan_free_adapter(priv);
    priv->wlan_dev.netdev = NULL;
    free_netdev(dev);
    wlanpriv = NULL;

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/********************************************************
        Global Functions
********************************************************/

/**
 *  @brief This function finds the CFP in
 *  region_cfp_table based on region and band parameter.
 *
 *  @param region  The region code
 *  @param band    The band
 *  @param cfp_no  A pointer to CFP number
 *  @return        A pointer to CFP
 */
CHANNEL_FREQ_POWER *
wlan_get_region_cfp_table(u8 region, u8 band, int *cfp_no)
{
    int i;

    ENTER();

    for (i = 0; i < sizeof(region_cfp_table) / sizeof(region_cfp_table_t); i++) {
        PRINTM(INFO, "region_cfp_table[i].region=%d\n",
               region_cfp_table[i].region);
        if (region_cfp_table[i].region == region) {
            if (band & (BAND_B | BAND_G)) {
                *cfp_no = region_cfp_table[i].cfp_no_BG;
                LEAVE();
                return region_cfp_table[i].cfp_BG;
            } else {
                if (band & BAND_A) {
                    *cfp_no = region_cfp_table[i].cfp_no_A;
                    LEAVE();
                    return region_cfp_table[i].cfp_A;
                } else {
                    PRINTM(INFO, "Error Band[%x]\n", band);
                    LEAVE();
                    return NULL;
                }
            }
        }
    }

    LEAVE();
    return NULL;
}

/**
 *  @brief This function sets region table.
 *
 *  @param priv    A pointer to wlan_private structure
 *  @param region  The region code
 *  @param band    The band
 *  @return        WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
int
wlan_set_regiontable(wlan_private * priv, u8 region, u8 band)
{
    wlan_adapter *Adapter = priv->adapter;
    int i = 0;

    CHANNEL_FREQ_POWER *cfp;
    int cfp_no;

    ENTER();

    memset(Adapter->region_channel, 0, sizeof(Adapter->region_channel));

    if (band & (BAND_B | BAND_G)) {
        cfp = wlan_get_region_cfp_table(region, BAND_G | BAND_B, &cfp_no);
        if (cfp != NULL) {
            Adapter->region_channel[i].NrCFP = cfp_no;
            Adapter->region_channel[i].CFP = cfp;
        } else {
            PRINTM(INFO, "wrong region code %#x in Band B-G\n", region);
            LEAVE();
            return WLAN_STATUS_FAILURE;
        }
        Adapter->region_channel[i].Valid = TRUE;
        Adapter->region_channel[i].Region = region;
        Adapter->region_channel[i].Band = (band & BAND_G) ? BAND_G : BAND_B;
        i++;
    }
    if (band & BAND_A) {
        cfp = wlan_get_region_cfp_table(region, BAND_A, &cfp_no);
        if (cfp != NULL) {
            Adapter->region_channel[i].NrCFP = cfp_no;
            Adapter->region_channel[i].CFP = cfp;
        } else {
            PRINTM(INFO, "wrong region code %#x in Band A\n", region);
            LEAVE();
            return WLAN_STATUS_FAILURE;
        }
        Adapter->region_channel[i].Valid = TRUE;
        Adapter->region_channel[i].Region = region;
        Adapter->region_channel[i].Band = BAND_A;
    }
    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/**
 *  @brief This function handles the interrupt. it will change PS
 *  state if applicable. it will wake up main_thread to handle
 *  the interrupt event as well.
 *
 *  @param priv    A pointer to wlan_private structure
 *  @return        n/a
 */
void
wlan_interrupt(wlan_private * priv)
{

    priv->adapter->IntCounter++;

    PRINTM(INTR, "*\n");

    priv->adapter->WakeupTries = 0;

    if (priv->adapter->PSState == PS_STATE_SLEEP) {
        priv->adapter->PSState = PS_STATE_AWAKE;
    }

    wake_up_interruptible(&priv->MainThread.waitQ);

}

/**
 *  @brief This function initializes module.
 *
 *  @return        WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
static int
wlan_init_module(void)
{
    int ret = WLAN_STATUS_SUCCESS;

    ENTER();

    ret = sbi_register();
    LEAVE();
    return ret;
}

/**
 *  @brief This function cleans module
 *
 *  @return        n/a
 */
static void
wlan_cleanup_module(void)
{
    wlan_private *priv = wlanpriv;
    wlan_adapter *Adapter = NULL;

    ENTER();

    if (!priv || !priv->adapter)
        goto exit;

    Adapter = priv->adapter;

    /* Exit deep sleep */
    wlan_exit_deep_sleep_timeout(priv);

    if (Adapter->MediaConnectStatus == WlanMediaStateConnected)
        wlan_disconnect(priv);

    wlan_prepare_cmd(priv, HostCmd_CMD_FUNC_SHUTDOWN,
                     0, HostCmd_OPTION_WAITFORRSP | HostCmd_OPTION_TIMEOUT,
                     0, NULL);

    /* Disable interrupts on the card */
    sbi_disable_host_int(priv);

  exit:
    sbi_unregister();
    LEAVE();
}

module_init(wlan_init_module);
module_exit(wlan_cleanup_module);

MODULE_DESCRIPTION("M-WLAN Driver");
MODULE_AUTHOR("Marvell International Ltd.");
MODULE_VERSION(DRIVER_RELEASE_VERSION);
MODULE_LICENSE("GPL");
