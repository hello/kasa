/** @file wlan_11h.h
 *
 *  @brief Interface for the 802.11h wlan_11h module implemented in wlan_11h.c
 *
 *  Driver interface functions and type declarations for the 11h module
 *    implemented in wlan_11h.c.
 *
 *  Copyright (C) 2003-2008, Marvell International Ltd. 
 *
 *  This software file (the "File") is distributed by Marvell International 
 *  Ltd. under the terms of the GNU General Public License Version 2, June 1991 
 *  (the "License").  You may use, redistribute and/or modify this File in 
 *  accordance with the terms and conditions of the License, a copy of which 
 *  is available along with the File in the gpl.txt file or by writing to 
 *  the Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 
 *  02111-1307 or on the worldwide web at http://www.gnu.org/licenses/gpl.txt.
 *
 *  THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE 
 *  IMPLIED WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE 
 *  ARE EXPRESSLY DISCLAIMED.  The License provides additional details about 
 *  this warranty disclaimer.
 *
 */
/*************************************************************
Change Log:
    9/30/05: Added DFS code, reorganized TPC code to match conventions

 ************************************************************/

#ifndef _WLAN_11H
#define _WLAN_11H

#include "wlan_meas.h"

/*
 *  #defines used in this header file
 */

/** Maximum number of subbands in the IEEEtypes_SupportedChannels_t structure */
#define WLAN_11H_MAX_SUBBANDS  5

/** Maximum number of DFS channels configured in IEEEtypes_IBSS_DFS_t */
#define WLAN_11H_MAX_IBSS_DFS_CHANNELS 25

/*
 *
 *  IEEE type definitions
 *
 */

/**  IEEE Power Constraint element (7.3.2.15) */
typedef struct
{
    u8 elementId;         /**< IEEE Element ID = 32 */
    u8 len;               /**< Element length after id and len */
    u8 localConstraint;   /**< Local power constraint applied to 11d chan info */
} __ATTRIB_PACK__ IEEEtypes_PowerConstraint_t;

/**  IEEE Power Capability element (7.3.2.16) */
typedef struct
{
    u8 elementId;             /**< IEEE Element ID = 33 */
    u8 len;                   /**< Element length after id and len */
    s8 minTxPowerCapability;  /**< Minimum Transmit power (dBm) */
    s8 maxTxPowerCapability;  /**< Maximum Transmit power (dBm) */
} __ATTRIB_PACK__ IEEEtypes_PowerCapability_t;

/**  IEEE TPC Report element (7.3.2.18) */
typedef struct
{
    u8 elementId;    /**< IEEE Element ID = 35 */
    u8 len;          /**< Element length after id and len */
    s8 txPower;      /**< Max power used to transmit the TPC Report frame (dBm) */
    s8 linkmargin;   /**< Link margin when TPC Request received (dB) */
} __ATTRIB_PACK__ IEEEtypes_TPCReport_t;

/*  IEEE Supported Channel sub-band description (7.3.2.19) */
/**  
 *  Sub-band description used in the supported channels element. 
 */
typedef struct
{
    u8 startChan;   /**< Starting channel in the subband */
    u8 numChans;    /**< Number of channels in the subband */

} __ATTRIB_PACK__ IEEEtypes_SupportChan_Subband_t;

/*  IEEE Supported Channel element (7.3.2.19) */
/**
 *  Sent in association requests. Details the sub-bands and number
 *    of channels supported in each subband
 */
typedef struct
{
    u8 elementId;    /**< IEEE Element ID = 36 */
    u8 len;          /**< Element length after id and len */

    /** Configured sub-bands information in the element */
    IEEEtypes_SupportChan_Subband_t subband[WLAN_11H_MAX_SUBBANDS];

} __ATTRIB_PACK__ IEEEtypes_SupportedChannels_t;

/*  Channel Switch Announcement element (7.3.2.20) */
/**
 *  Sent in action frames and in beacon and probe responses.  Indicates 
 *    new channel, time until channel switch, and whether or not
 *    transmissions are allowed while the switch is pending.
 */
typedef struct
{
    u8 elementId;           /**< IEEE Element ID = 37 */
    u8 len;                 /**< Element length after id and len */
    u8 channelSwitchMode;   /**< Mode: 1 indicates STA quiet during switch time */
    u8 newChannelNumber;    /**< New channel */
    u8 channelSwitchCount;  /**< Nubmer of TBTTS until switch */

} __ATTRIB_PACK__ IEEEtypes_ChannelSwitchAnn_t;

/*  IEEE Quiet Period Element (7.3.2.23) */
/**
 *  Provided in beacons and probe responses.  Indicates times during 
 *    which the STA should not be transmitting data.  Only starting STAs in
 *    an IBSS and APs are allowed to originate a quiet element. 
 */
typedef struct
{
    u8 elementId;      /**< IEEE Element ID = 40 */
    u8 len;            /**< Element length after id and len */
    u8 quietCount;     /**< Number of TBTTs until beacon with the quiet period */
    u8 quietPeriod;    /**< Regular quiet period, # of TBTTS between periods */
    u16 quietDuration; /**< Duration of the quiet period in TUs */
    u16 quietOffset;   /**< Offset in TUs from the TBTT for the quiet period */

} __ATTRIB_PACK__ IEEEtypes_Quiet_t;

/*  IEEE DFS Channel Map field (7.3.2.24) */
/**
 *  Used to list supported channels and provide a octet "map" field which 
 *    contains a basic measurement report for that channel in the 
 *    IEEEtypes_IBSS_DFS_t element
 */
typedef struct
{
    u8 channelNumber;          /**< Channel number */
    MeasRptBasicMap_t rptMap;  /**< Basic measurement report for the channel */

} __ATTRIB_PACK__ IEEEtypes_ChannelMap_t;

/*  IEEE IBSS DFS Element (7.3.2.24) */
/**
 *  IBSS DFS element included in ad hoc beacons and probe responses.  
 *    Provides information regarding the IBSS DFS Owner as well as the
 *    originating STAs supported channels and basic measurement results.
 */
typedef struct
{
    u8 elementId;                      /**< IEEE Element ID = 41 */
    u8 len;                            /**< Element length after id and len */
    u8 dfsOwner[ETH_ALEN];             /**< DFS Owner STA Address */
    u8 dfsRecoveryInterval;            /**< DFS Recovery time in TBTTs */

    /** Variable length map field, one Map entry for each supported channel */
    IEEEtypes_ChannelMap_t channelMap[WLAN_11H_MAX_IBSS_DFS_CHANNELS];

} __ATTRIB_PACK__ IEEEtypes_IBSS_DFS_t;

/*
 *
 *  Marvell IE Type definitions
 *
 */

/*
 *  11h IOCTL structures
 */

/* WLAN_11H_REQUESTTPC IOCTL request data structure */
/**
 * Structure passed from the application layer to initiate a TPC request 
 *    command via the firmware to a given STA address
 */
typedef struct
{
    u8 destMac[ETH_ALEN];   /**< Destination STA address */
    u8 rateIndex;           /**< Response timeout in ms  */
    u16 timeout;            /**< IEEE Rate index to send request */

} __ATTRIB_PACK__ wlan_ioctl_11h_tpc_req;

/* WLAN_11H_REQUESTTPC IOCTL response data structure */
/**
 * Structure passed from the driver to the application providing the 
 *   results of the application initiated TPC Request
 */
typedef struct
{
    int statusCode; /**< Firmware command result status code */
    int txPower;    /**< Reported TX Power from the TPC Report */
    int linkMargin; /**< Reported Link margin from the TPC Report */
    int rssi;       /**< RSSI of the received TPC Report frame */

} __ATTRIB_PACK__ wlan_ioctl_11h_tpc_resp;

/* WLAN_11H_CHANSWANN IOCTL data structure  */
/**
 * Structure passed from the application to the driver to initiate
 *   a channel switch announcement in the firmware (include in beacons, send
 *   action frame, switch own channel)
 */
typedef struct
{
    u8 switchMode;  /**< Set to 1 for a quiet switch request, no STA tx */
    u8 newChan;     /**< Requested new channel */
    u8 switchCount; /**< Number of TBTTs until the switch is to occur */

} __ATTRIB_PACK__ wlan_ioctl_11h_chan_sw_ann;

/* WLAN_11H_POWERCAP IOCTL data structure  */
/**
 * Structure passed from the application to the driver to set the internal
 *   min/max TX Power capability settings in the driver.  These settings
 *   are later passed to the firmware for use in association requests
 */
typedef struct
{
    s8 minTxPower;  /**< Minimum Tx Power capability */
    s8 maxTxPower;  /**< Maximum Tx Power capability */

} __ATTRIB_PACK__ wlan_ioctl_11h_power_cap;

/* 802.11h BSS information kept for each BSSID received in scan results */
/**
 * IEEE BSS information needed from scan results for later processing in
 *    join commands 
 */
typedef struct
{
    u8 sensed11h;  /**< Capability bit set or 11h IE found in this BSS */

    IEEEtypes_PowerConstraint_t powerConstraint;  /**< Power Constraint IE */
    IEEEtypes_PowerCapability_t powerCapability;  /**< Power Capability IE */
    IEEEtypes_TPCReport_t tpcReport;              /**< TPC Report IE */
    IEEEtypes_Quiet_t quiet;                      /**< Quiet IE */
    IEEEtypes_IBSS_DFS_t ibssDfs;                 /**< IBSS DFS Element IE */

} wlan_11h_bss_info_t;

/** 802.11h State information kept in the wlan_adapter class for the driver */
typedef struct
{
    int is11hEnabled;   /**< Enables/disables 11h in the driver (adhoc start) */
    int is11hActive;    /**< Indicates whether 11h is active in the firmware */
    int txDisabled;     /**< Set when driver receives a STOP TX event from fw */

    /** Minimum TX Power capability sent to FW for 11h use and fw power control */
    s8 minTxPowerCapability;

    /** Maximum TX Power capability sent to FW for 11h use and fw power control */
    s8 maxTxPowerCapability;

    /** User provisioned local power constraint sent in association requests */
    s8 usrDefPowerConstraint;

    /** Quiet IE */
    IEEEtypes_Quiet_t quiet_ie;

} wlan_11h_state_t;

/*
**  11H APIs
*/

/* Initialize the 11h software module */
extern void wlan_11h_init(wlan_private * priv);

/* Return 1 if 11h is active in the firmware, 0 if it is inactive */
extern int wlan_11h_is_active(wlan_private * priv);

/* Activate 11h extensions in the firmware */
extern int wlan_11h_activate(wlan_private * priv, int flag);

/* Enable the tx interface and record the new transmit state */
extern void wlan_11h_tx_enable(wlan_private * priv);

/* Enable the tx interface and record the new transmit state */
extern void wlan_11h_tx_disable(wlan_private * priv);

/* Return 1 if 11h has disabled the transmit intervace, 0 otherwise */
extern int wlan_11h_is_tx_disabled(wlan_private * priv);

/* Check if radar detection is required on the specified channel */
extern int wlan_11h_radar_detect_required(wlan_private * priv, u8 channel);

/* Perform a standard availibility check on the specified channel */
extern int wlan_11h_radar_detected(wlan_private * priv, u8 channel);

/* Get an initial random channel to start an adhoc network on */
extern int wlan_11h_get_adhoc_start_channel(wlan_private * priv);

/* Add any 11h TLVs necessary to complete a join command (adhoc or infra) */
extern int wlan_11h_process_join(wlan_private * priv,
                                 u8 ** ppBuffer,
                                 IEEEtypes_CapInfo_t * pCapInfo,
                                 uint channel,
                                 wlan_11h_bss_info_t * p11hBssInfo);

/* Add any 11h TLVs necessary to complete an adhoc start command */
extern int wlan_11h_process_start(wlan_private * priv,
                                  u8 ** ppBuffer,
                                  IEEEtypes_CapInfo_t * pCapInfo,
                                  uint channel,
                                  wlan_11h_bss_info_t * p11hBssInfo);

/* Receive IEs from scan processing and record any needed info for 11h */
int wlan_11h_process_bss_elem(wlan_11h_bss_info_t * p11hBssInfo,
                              const u8 * pElement);

/* Complete the firmware command preparation for an 11h command function */
extern int wlan_11h_cmd_process(wlan_private * priv,
                                HostCmd_DS_COMMAND * pCmdPtr,
                                const void *pInfoBuf);

/* Process the response of an 11h firmware command */
extern int wlan_11h_cmdresp_process(wlan_private * priv,
                                    const HostCmd_DS_COMMAND * resp);

/*
 *  IOCTL declarations
 */
/* Send a channel switch announcement */
extern int wlan_11h_ioctl_chan_sw_ann(wlan_private * priv, struct iwreq *wrq);

/* Send a TPC request to a specific STA */
extern int wlan_11h_ioctl_request_tpc(wlan_private * priv, struct iwreq *wrq);

/* Set the power capabilities in the driver to be sent in 11h firmware cmds */
extern int wlan_11h_ioctl_set_power_cap(wlan_private * priv, struct iwreq *wrq);

extern int wlan_11h_ioctl_set_quiet_ie(wlan_private * priv, struct iwreq *wrq);

/* Get the local power constraint setting from the driver */
extern int wlan_11h_ioctl_get_local_power(wlan_private * priv,
                                          struct iwreq *wrq);

#endif /*_WLAN_11H */
