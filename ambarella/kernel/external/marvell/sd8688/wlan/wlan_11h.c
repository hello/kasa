/**
 * @file wlan_11h.c
 *
 *  @brief Implementation of 802.11h wlan_11h module
 *
 *  Driver implementation of the 11h standard.  Contains the following:
 *    - application/driver ioctl interface functions
 *    - firmware command preparation and response handling
 *    - 11h element identification for elements passed in scan responses
 *
 *  Requires use of the following preprocessor define:
 *    - ENABLE_802_11H
 *
 *  @sa wlan_11h.h
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
    9/30/05: Added DFS code, reorganized TPC code to match conventions.
   01/11/06: Add connected channel qualification to radar detection
             requirement determination.  Update doxygen formatting.

************************************************************/

/*
 * Include(s)
 */
#include "wlan_headers.h"

/*
 * Constants
 */

/** Default IBSS DFS recovery interval (in TBTTs); used for adhoc start */
#define WLAN_11H_DEFAULT_DFS_RECOVERY_INTERVAL   100

/** Compiler control flag used to add quiet element to adhoc start command */
#define WLAN_11H_ADD_QUIET_ELEMENT   1

/** Default 11h power constraint used to offset the maximum transmit power */
#define WLAN_11H_TPC_POWERCONSTRAINT  0

/** 11h TPC Power capability minimum setting, sent in TPC_INFO command to fw */
#define WLAN_11H_TPC_POWERCAPABILITY_MIN     5

/** 11h TPC Power capability maximum setting, sent in TPC_INFO command to fw */
#define WLAN_11H_TPC_POWERCAPABILITY_MAX     20

/** Regulatory requirement for the duration of a channel availability check */
#define WLAN_11H_CHANNEL_AVAIL_CHECK_DURATION    60000  /* in ms */

/** U-NII sub-band config : Start Channel = 36, NumChans = 4 */
static const
    IEEEtypes_SupportChan_Subband_t wlan_11h_unii_lower_band = { 36, 4 };

/** U-NII sub-band config : Start Channel = 52, NumChans = 4 */
static const
    IEEEtypes_SupportChan_Subband_t wlan_11h_unii_middle_band = { 52, 4 };

/** U-NII sub-band config : Start Channel = 100, NumChans = 11 */
static const
    IEEEtypes_SupportChan_Subband_t wlan_11h_unii_mid_upper_band = { 100, 11 };

/** U-NII sub-band config : Start Channel = 149, NumChans = 4 */
static const
    IEEEtypes_SupportChan_Subband_t wlan_11h_unii_upper_band = { 149, 4 };

/*
 * Local types
 */

/** Internally passed structure used to send a CMD_802_11_TPC_INFO command */
typedef struct
{
    u8 Chan;                 /**< Channel to which the power constraint applies */
    u8 PowerConstraint;      /**< Local power constraint to send to firmware */
} wlan_11h_tpc_info_param_t;

/**
 *  @brief Utility function to get a random number based on the underlying OS
 *
 *  @return random integer
 */
static int
wlan_11h_get_random_num(void)
{
    return jiffies;
}

/**
 *  @brief Convert an IEEE formatted IE to 16-bit ID/Len Marvell
 *         proprietary format
 *
 *  @param pOutBuf Output parameter: Buffer to output Marvell formatted IE
 *  @param pInIe   Pointer to IEEE IE to be converted to Marvell format
 *
 *  @return        Number of bytes output to pOutBuf parameter return
 */
static int
wlan_11h_convert_ieee_to_mrvl_ie(char *pOutBuf, const char *pInIe)
{
    MrvlIEtypesHeader_t mrvlIeHdr;
    char *pTmpBuf = pOutBuf;

    ENTER();
    /* Assign the Element Id and Len to the Marvell struct attributes */
    mrvlIeHdr.Type = wlan_cpu_to_le16(pInIe[0]);
    mrvlIeHdr.Len = wlan_cpu_to_le16(pInIe[1]);

    /* If the element ID is zero, return without doing any copying */
    if (mrvlIeHdr.Type == 0)
        return 0;

    /* Copy the header to the buffer pointer */
    memcpy(pTmpBuf, &mrvlIeHdr, sizeof(mrvlIeHdr));

    /* Increment the temp buffer pointer by the size appended */
    pTmpBuf += sizeof(mrvlIeHdr);

    /* Append the data section of the IE; length given by the IEEE IE length */
    memcpy(pTmpBuf, pInIe + 2, pInIe[1]);

    LEAVE();
    /* Return the number of bytes appended to pBuf */
    return (sizeof(mrvlIeHdr) + pInIe[1]);
}

/**
 *  @brief Setup the IBSS DFS element passed to the firmware in adhoc start
 *         and join commands
 *
 *  The DFS Owner and recovery fields are set to be our MAC address and
 *    a predetermined constant recovery value.  If we are joining an adhoc
 *    network, these values are replaced with the existing IBSS values.
 *    They are valid only when starting a new IBSS.
 *
 *  The IBSS DFS Element is variable in size based on the number of
 *    channels supported in our current region.
 *
 *  @param priv Private driver information structure
 *  @param pDfs Output parameter: Pointer to the IBSS DFS element setup by
 *              this function.
 *
 *  @return
 *    - Length of the returned element in pDfs output parameter
 *    - 0 if returned element is not setup
 */
static int
wlan_11h_set_ibss_dfs_ie(wlan_private * priv, IEEEtypes_IBSS_DFS_t * pDfs)
{
    int numChans = 0;
    MeasRptBasicMap_t initialMap;
    wlan_adapter *Adapter = priv->adapter;

    ENTER();
    PRINTM(INFO, "11h: IBSS DFS Element, 11D parsed region: %c%c (0x%x)\n",
           Adapter->parsed_region_chan.CountryCode[0],
           Adapter->parsed_region_chan.CountryCode[1],
           Adapter->parsed_region_chan.region);

    memset(pDfs, 0x00, sizeof(IEEEtypes_IBSS_DFS_t));

    /* 
     * A basic measurement report is included with each channel in the
     *   map field.  Initial value for the map for each supported channel
     *   is with only the unmeasured bit set.
     */
    memset(&initialMap, 0x00, sizeof(initialMap));
    initialMap.unmeasured = 1;

    /* Set the DFS Owner and recovery interval fields */
    memcpy(pDfs->dfsOwner, Adapter->CurrentAddr, sizeof(pDfs->dfsOwner));
    pDfs->dfsRecoveryInterval = WLAN_11H_DEFAULT_DFS_RECOVERY_INTERVAL;

    for (; (numChans < Adapter->parsed_region_chan.NoOfChan)
         && (numChans < WLAN_11H_MAX_IBSS_DFS_CHANNELS); numChans++) {
        pDfs->channelMap[numChans].channelNumber =
            Adapter->parsed_region_chan.chanPwr[numChans].chan;

        /* 
         * Set the inital map field with a basic measurement
         */
        pDfs->channelMap[numChans].rptMap = initialMap;
    }

    /* 
     * If we have an established channel map, include it and return
     *   a valid DFS element
     */
    if (numChans) {
        PRINTM(INFO, "11h: Added %d channels to IBSS DFS Map\n", numChans);

        pDfs->elementId = IBSS_DFS;
        pDfs->len = (sizeof(pDfs->dfsOwner) + sizeof(pDfs->dfsRecoveryInterval)
                     + numChans * sizeof(IEEEtypes_ChannelMap_t));

        return (pDfs->len + sizeof(pDfs->len) + sizeof(pDfs->elementId));
    }

    /* Ensure the element is zeroed out for an invalid return */
    memset(pDfs, 0x00, sizeof(IEEEtypes_IBSS_DFS_t));

    LEAVE();
    return 0;
}

/**
 *  @brief Setup the Supported Channel IE sent in association requests
 *
 *  The Supported Channels IE is required to be sent when the spectrum
 *    management capability (11h) is enabled.  The element contains a
 *    starting channel and number of channels tuple for each sub-band
 *    the STA supports.  This information is based on the operating region.
 *
 *  @param priv     Private driver information structure
 *  @param pSupChan Output parameter: Pointer to the Supported Chan element
 *                  setup by this function.
 *
 *  @return
 *    - Length of the returned element in pSupChan output parameter
 *    - 0 if returned element is not setup
 */
static int
wlan_11h_set_supp_channels_ie(wlan_private * priv,
                              IEEEtypes_SupportedChannels_t * pSupChan)
{
    int numSubbands = 0;
    int retLen = 0;

    ENTER();
    memset(pSupChan, 0x00, sizeof(IEEEtypes_SupportedChannels_t));

    /* 
     * Set the supported channel elements based on the region code,
     *   incrementing numSubbands for each sub-band we append to the
     *   element.
     */
    switch (priv->adapter->RegionCode) {
    case 0x10:                 /* USA FCC */
    case 0x20:                 /* Canada IC */
        pSupChan->subband[numSubbands++] = wlan_11h_unii_lower_band;
        pSupChan->subband[numSubbands++] = wlan_11h_unii_middle_band;
        pSupChan->subband[numSubbands++] = wlan_11h_unii_mid_upper_band;
        pSupChan->subband[numSubbands++] = wlan_11h_unii_upper_band;
        break;
    case 0x30:                 /* Europe ETSI */
        pSupChan->subband[numSubbands++] = wlan_11h_unii_lower_band;
        pSupChan->subband[numSubbands++] = wlan_11h_unii_middle_band;
        pSupChan->subband[numSubbands++] = wlan_11h_unii_mid_upper_band;
        break;
    default:
        break;
    }

    /* 
     * If we have setup any supported subbands in the element, return a
     *    valid IE along with its size, else return 0.
     */
    if (numSubbands) {
        pSupChan->elementId = SUPPORTED_CHANNELS;
        pSupChan->len = numSubbands * sizeof(IEEEtypes_SupportChan_Subband_t);

        retLen = (pSupChan->len
                  + sizeof(pSupChan->len) + sizeof(pSupChan->elementId));

        HEXDUMP("11h: SupChan", (u8 *) pSupChan, retLen);
    }

    LEAVE();
    return retLen;
}

/**
 *  @brief Prepare CMD_802_11_TPC_ADAPT_REQ firmware command
 *
 *  @param priv     Private driver information structure
 *  @param pCmdPtr  Output parameter: Pointer to the command being prepared
 *                  for the firmware
 *  @param pInfoBuf HostCmd_DS_802_11_TPC_ADAPT_REQ passed as void data block
 *
 *  @return         WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
static int
wlan_11h_cmd_tpc_request(wlan_private * priv,
                         HostCmd_DS_COMMAND * pCmdPtr, const void *pInfoBuf)
{
    ENTER();

    memcpy(&pCmdPtr->params.tpcReq, pInfoBuf,
           sizeof(HostCmd_DS_802_11_TPC_ADAPT_REQ));

    pCmdPtr->params.tpcReq.req.timeout =
        wlan_cpu_to_le16(pCmdPtr->params.tpcReq.req.timeout);

    /* Converted to little endian in wlan_11h_cmd_process */
    pCmdPtr->Size = sizeof(HostCmd_DS_802_11_TPC_ADAPT_REQ) + S_DS_GEN;

    HEXDUMP("11h: 11_TPC_ADAPT_REQ:", (u8 *) pCmdPtr, (int) pCmdPtr->Size);

    LEAVE();

    return WLAN_STATUS_SUCCESS;
}

/**
 *  @brief Prepare CMD_802_11_TPC_INFO firmware command
 *
 *  @param priv     Private driver information structure
 *  @param pCmdPtr  Output parameter: Pointer to the command being prepared 
 *                  for the firmware
 *  @param pInfoBuf wlan_11h_tpc_info_param_t passed as void data block
 *
 *  @return         WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
static int
wlan_11h_cmd_tpc_info(wlan_private * priv,
                      HostCmd_DS_COMMAND * pCmdPtr, const void *pInfoBuf)
{
    HostCmd_DS_802_11_TPC_INFO *pTpcInfo = &pCmdPtr->params.tpcInfo;
    MrvlIEtypes_LocalPowerConstraint_t *pConstraint =
        &pTpcInfo->localConstraint;
    MrvlIEtypes_PowerCapability_t *pCap = &pTpcInfo->powerCap;

    wlan_11h_state_t *pState = &priv->adapter->state11h;
    const wlan_11h_tpc_info_param_t *pTpcInfoParam = pInfoBuf;

    ENTER();

    pCap->MinPower = pState->minTxPowerCapability;
    pCap->MaxPower = pState->maxTxPowerCapability;
    pCap->Header.Len = wlan_cpu_to_le16(2);
    pCap->Header.Type = wlan_cpu_to_le16(TLV_TYPE_POWER_CAPABILITY);

    pConstraint->Chan = pTpcInfoParam->Chan;
    pConstraint->Constraint = pTpcInfoParam->PowerConstraint;
    pConstraint->Header.Type = wlan_cpu_to_le16(TLV_TYPE_POWER_CONSTRAINT);
    pConstraint->Header.Len = wlan_cpu_to_le16(2);

    /* Converted to little endian in wlan_11h_cmd_process */
    pCmdPtr->Size = sizeof(HostCmd_DS_802_11_TPC_INFO) + S_DS_GEN;

    HEXDUMP("11h: TPC INFO", (u8 *) pCmdPtr, (int) pCmdPtr->Size);

    LEAVE();

    return WLAN_STATUS_SUCCESS;
}

/**
 *  @brief  Prepare CMD_802_11_CHAN_SW_ANN firmware command
 *
 *  @param priv     Private driver information structure
 *  @param pCmdPtr  Output parameter: Pointer to the command being 
 *                  prepared to for firmware
 *  @param pInfoBuf HostCmd_DS_802_11_CHAN_SW_ANN passed as void data block
 *
 *  @return         WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
static int
wlan_11h_cmd_chan_sw_ann(wlan_private * priv,
                         HostCmd_DS_COMMAND * pCmdPtr, const void *pInfoBuf)
{
    const HostCmd_DS_802_11_CHAN_SW_ANN *pChSwAnn = pInfoBuf;

    ENTER();

    /* Converted to little endian in wlan_11h_cmd_process */
    pCmdPtr->Size = sizeof(HostCmd_DS_802_11_CHAN_SW_ANN) + S_DS_GEN;

    memcpy(&pCmdPtr->params.chan_sw_ann, pChSwAnn,
           sizeof(HostCmd_DS_802_11_CHAN_SW_ANN));

    PRINTM(INFO, "11h: ChSwAnn: %#x-%u, Seq=%u, Ret=%u\n",
           pCmdPtr->Command, pCmdPtr->Size, pCmdPtr->SeqNum, pCmdPtr->Result);
    PRINTM(INFO, "11h: ChSwAnn: Ch=%d, Cnt=%d, Mode=%d\n",
           pChSwAnn->newChan, pChSwAnn->switchCount, pChSwAnn->switchMode);

    LEAVE();

    return WLAN_STATUS_SUCCESS;
}

/**
 *  @brief Set the local power constraint in the firmware
 *
 *  Construct and send a CMD_802_11_TPC_INFO command with the local power
 *    constraint.
 *
 *  @param priv            Private driver information structure
 *  @param channel         Channel to which the power constraint applies
 *  @param powerConstraint Power constraint to be applied on the channel
 *
 *  @return                WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
static int
wlan_11h_set_local_power_constraint(wlan_private * priv,
                                    u8 channel, u8 powerConstraint)
{
    int ret = WLAN_STATUS_SUCCESS;
    wlan_11h_tpc_info_param_t tpcInfoParam;

    ENTER();
    tpcInfoParam.Chan = channel;
    tpcInfoParam.PowerConstraint = powerConstraint;

    PRINTM(INFO, "11h: Set Local Constraint = %d\n",
           tpcInfoParam.PowerConstraint);

    ret = wlan_prepare_cmd(priv, HostCmd_CMD_802_11_TPC_INFO,
                           HostCmd_ACT_GEN_SET, 0, 0, &tpcInfoParam);

    if (ret) {
        PRINTM(INFO, "11h: Err: Send TPC_INFO CMD: %d\n", ret);
        ret = WLAN_STATUS_FAILURE;
    }

    LEAVE();
    return ret;
}

/**
 *  @brief  Utility function to process a join to an infrastructure BSS
 *
 *  @param priv        Private driver information structure
 *  @param ppBuffer    Output parameter: Pointer to the TLV output buffer,
 *                     modified on return to point after the appended 11h TLVs
 *  @param channel     Channel on which we are joining the BSS
 *  @param p11hBssInfo Pointer to the 11h BSS information for this network
 *                     that was parsed out of the scan response.
 *
 *  @return            Integer number of bytes appended to the TLV output
 *                     buffer (ppBuffer)
 */
static int
wlan_11h_process_infra_join(wlan_private * priv,
                            u8 ** ppBuffer,
                            uint channel, wlan_11h_bss_info_t * p11hBssInfo)
{
    MrvlIEtypesHeader_t ieHeader;
    IEEEtypes_SupportedChannels_t supChanIe;
    int retLen = 0;
    int supChanLen = 0;

    ENTER();

    /* Null Checks */
    if (ppBuffer == 0)
        return 0;
    if (*ppBuffer == 0)
        return 0;

    /* Set the local constraint configured in the firmware */
    wlan_11h_set_local_power_constraint(priv, channel,
                                        (p11hBssInfo->
                                         powerConstraint.localConstraint));

    /* Setup the Supported Channels IE */
    supChanLen = wlan_11h_set_supp_channels_ie(priv, &supChanIe);

    /* 
     * If we returned a valid Supported Channels IE, wrap and append it
     */
    if (supChanLen) {
        /* Wrap the supported channels IE with a passthrough TLV type */
        ieHeader.Type = cpu_to_le16(TLV_TYPE_PASSTHROUGH);
        ieHeader.Len = supChanLen;
        memcpy(*ppBuffer, &ieHeader, sizeof(ieHeader));

        /* Increment the return size and the return buffer pointer param */
        *ppBuffer += sizeof(ieHeader);
        retLen += sizeof(ieHeader);

        /* Copy the supported channels IE to the output buf, advance pointer */
        memcpy(*ppBuffer, &supChanIe, supChanLen);
        *ppBuffer += supChanLen;
        retLen += supChanLen;
    }

    LEAVE();
    return retLen;
}

/**
 *  @brief Utility function to process a start or join to an adhoc network
 *
 *  Add the elements to the TLV buffer needed in the start/join adhoc commands:
 *       - IBSS DFS IE
 *       - Quiet IE
 *
 *  Also send the local constraint to the firmware in a TPC_INFO command.
 *
 *  @param priv        Private driver information structure
 *  @param ppBuffer    Output parameter: Pointer to the TLV output buffer,
 *                     modified on return to point after the appended 11h TLVs
 *  @param channel     Channel on which we are starting/joining the IBSS
 *  @param p11hBssInfo Pointer to the 11h BSS information for this network
 *                     that was parsed out of the scan response.  NULL
 *                     indicates we are starting the adhoc network
 *
 *  @return            Integer number of bytes appended to the TLV output
 *                     buffer (ppBuffer)
 */
static int
wlan_11h_process_adhoc(wlan_private * priv,
                       u8 ** ppBuffer,
                       uint channel, wlan_11h_bss_info_t * p11hBssInfo)
{
    IEEEtypes_IBSS_DFS_t dfsElem;
    int sizeAppended;
    int retLen = 0;
    s8 localConstraint = 0;
    wlan_adapter *Adapter = priv->adapter;

    ENTER();

    /* Format our own IBSS DFS Element.  Include our channel map fields */
    wlan_11h_set_ibss_dfs_ie(priv, &dfsElem);

    if (p11hBssInfo) {
        /* 
         * Copy the DFS Owner/Recovery Interval from the BSS we are joining
         */
        memcpy(dfsElem.dfsOwner,
               p11hBssInfo->ibssDfs.dfsOwner, sizeof(dfsElem.dfsOwner));
        dfsElem.dfsRecoveryInterval = p11hBssInfo->ibssDfs.dfsRecoveryInterval;
    }

    /* Append the dfs element to the TLV buffer */
    sizeAppended = wlan_11h_convert_ieee_to_mrvl_ie(*ppBuffer,
                                                    (u8 *) & dfsElem);

    HEXDUMP("11h: IBSS-DFS", (void *) *ppBuffer, sizeAppended);
    *ppBuffer += sizeAppended;
    retLen += sizeAppended;

    /* 
     * Check to see if we are joining a network.  Join is indicated by the
     *   BSS Info pointer being valid (not NULL)
     */
    if (p11hBssInfo) {
        /* 
         * If there was a quiet element, include it in adhoc join command
         */
        if (p11hBssInfo->quiet.elementId == QUIET) {
            sizeAppended
                = wlan_11h_convert_ieee_to_mrvl_ie(*ppBuffer,
                                                   (u8 *) & p11hBssInfo->quiet);
            HEXDUMP("11h: Quiet", (void *) *ppBuffer, sizeAppended);
            *ppBuffer += sizeAppended;
            retLen += sizeAppended;
        }

        /* Copy the local constraint from the network */
        localConstraint = p11hBssInfo->powerConstraint.localConstraint;
    } else {
        /* 
         * If we are the adhoc starter, we can add a quiet element
         */
        if (Adapter->state11h.quiet_ie.quietPeriod) {
            sizeAppended = wlan_11h_convert_ieee_to_mrvl_ie(*ppBuffer,
                                                            (u8 *) & Adapter->
                                                            state11h.quiet_ie);
            HEXDUMP("11h: Quiet", (void *) *ppBuffer, sizeAppended);
            *ppBuffer += sizeAppended;
            retLen += sizeAppended;

            /* Use the localConstraint configured in the driver state */
            localConstraint = Adapter->state11h.usrDefPowerConstraint;
        }
    }

    /* Set the local constraint configured in the firmware */
    wlan_11h_set_local_power_constraint(priv, channel, localConstraint);

    LEAVE();
    return retLen;
}

/**
 *  @brief Return whether the driver is currently setup to use 11h for
 *         adhoc start.
 *
 *  Association/Join commands are dynamic in that they enable 11h in the
 *    driver/firmware when they are detected in the existing BSS.
 *
 *  @param priv  Private driver information structure
 *
 *  @return
 *    - TRUE if 11h is enabled
 *    - FALSE otherwise
 */
static int
wlan_11h_is_enabled(wlan_private * priv)
{
    wlan_11h_state_t *pState11h = &priv->adapter->state11h;

    return (pState11h->is11hEnabled ? TRUE : FALSE);
}

/**
 *  @brief Query 11h firmware enabled state.
 *
 *  Return whether the firmware currently has 11h extensions enabled
 *
 *  @param priv  Private driver information structure
 *
 *  @return
 *    - TRUE if 11h has been activated in the firmware
 *    - FALSE otherwise
 *
 *  @sa wlan_11h_activate
 */
int
wlan_11h_is_active(wlan_private * priv)
{
    wlan_11h_state_t *pState11h = &priv->adapter->state11h;

    return (pState11h->is11hActive ? TRUE : FALSE);
}

/**
 *  @brief Query 11h driver tx enable/disable state at the driver/application
 *         layer
 *
 *  @param priv  Private driver information structure
 *
 *  @return
 *    - TRUE if 11h has disabled the transmit interface
 *    - FALSE otherwise
 */
int
wlan_11h_is_tx_disabled(wlan_private * priv)
{
    wlan_11h_state_t *pState11h = &priv->adapter->state11h;

    return (pState11h->txDisabled);
}

/**
 *  @brief Disable the transmit interface and record the state.
 *
 *  @param priv  Private driver information structure
 *
 *  @return      void
 */
void
wlan_11h_tx_disable(wlan_private * priv)
{
    wlan_11h_state_t *pState11h = &priv->adapter->state11h;

    ENTER();
    pState11h->txDisabled = TRUE;
    os_carrier_off(priv);
    LEAVE();
}

/**
 *  @brief Enable the transmit interface and record the state.
 *
 *  @param priv  Private driver information structure
 *
 *  @return      void
 */
void
wlan_11h_tx_enable(wlan_private * priv)
{
    wlan_11h_state_t *pState11h = &priv->adapter->state11h;

    ENTER();
    os_carrier_on(priv);
    pState11h->txDisabled = FALSE;
    LEAVE();
}

/**
 *  @brief Enable or Disable the 11h extensions in the firmware
 *
 *  @param priv  Private driver information structure
 *  @param flag  Enable 11h if TRUE, disable otherwise
 *
 *  @return      WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
int
wlan_11h_activate(wlan_private * priv, int flag)
{
    wlan_11h_state_t *pState11h = &priv->adapter->state11h;
    int enable = flag ? 1 : 0;
    int ret = WLAN_STATUS_SUCCESS;

    ENTER();

    /* 
     * Send cmd to FW to enable/disable 11h function in firmware
     */
    ret = wlan_prepare_cmd(priv,
                           HostCmd_CMD_802_11_SNMP_MIB,
                           HostCmd_ACT_GEN_SET,
                           HostCmd_OPTION_WAITFORRSP, Dot11H_i, &enable);

    if (ret)
        ret = WLAN_STATUS_FAILURE;
    else
        /* Set boolean flag in driver 11h state */
        pState11h->is11hActive = flag;

    PRINTM(INFO, "11h: %s\n", enable ? "Activate" : "Deactivate");

    LEAVE();

    return ret;
}

/**
 *
 *  @brief Initialize the 11h parameters and enable 11h when starting an IBSS
 *
 *  @param priv  Private driver information structure
 *
 *  @return      void
 */
void
wlan_11h_init(wlan_private * priv)
{
    wlan_11h_state_t *pState11h = &priv->adapter->state11h;
    IEEEtypes_Quiet_t *pQuiet = &priv->adapter->state11h.quiet_ie;

    ENTER();

    pState11h->usrDefPowerConstraint = WLAN_11H_TPC_POWERCONSTRAINT;
    pState11h->minTxPowerCapability = WLAN_11H_TPC_POWERCAPABILITY_MIN;
    pState11h->maxTxPowerCapability = WLAN_11H_TPC_POWERCAPABILITY_MAX;

    /* 
     * By default, the driver should have its preference set as 11h being
     *    activated when starting an ad hoc network.  For infrastructure
     *    and ad hoc join, 11h will be sensed and activated accordingly.
     */
    pState11h->is11hEnabled = TRUE;

    /* 
     * On power up, the firmware should have 11h support inactive.
     */
    pState11h->is11hActive = FALSE;

    /* Initialize quiet_ie */
    memset(pQuiet, 0, sizeof(IEEEtypes_Quiet_t));

    pQuiet->elementId = QUIET;
    pQuiet->len = (sizeof(pQuiet->quietCount) + sizeof(pQuiet->quietPeriod)
                   + sizeof(pQuiet->quietDuration)
                   + sizeof(pQuiet->quietOffset));
    LEAVE();
}

/**
 *  @brief Retrieve a randomly selected starting channel if needed for 11h
 *
 *  If 11h is enabled and an A-Band channel start band preference
 *    configured in the driver, the start channel must be random in order
 *    to meet with
 *
 *  @param priv  Private driver information structure
 *
 *  @return      Integer starting channel
 */
int
wlan_11h_get_adhoc_start_channel(wlan_private * priv)
{
    wlan_adapter *Adapter = priv->adapter;
    unsigned int startChn;
    int region;
    int randEntry;
    REGION_CHANNEL *chnTbl;

    ENTER();

    /* 
     * Set startChn to the Default.  Used if 11h is disabled or the band
     *   does not require 11h support.
     */
    startChn = DEFAULT_AD_HOC_CHANNEL;

    /* 
     * Check that we are looking for a channel in the A Band
     */
    if (Adapter->adhoc_start_band == BAND_A) {
        /* 
         * Set default to the A Band default. Used if random selection fails
         *   or if 11h is not enabled
         */
        startChn = DEFAULT_AD_HOC_CHANNEL_A;

        /* 
         * Check that 11h is enabled in the driver
         */
        if (wlan_11h_is_enabled(priv)) {
            /* 
             * Search the region_channel tables for a channel table
             *   that is marked for the A Band.
             */
            for (region = 0; (region < MAX_REGION_CHANNEL_NUM); region++) {
                chnTbl = Adapter->region_channel + region;

                /* Check if table is valid and marked for A Band */
                if (chnTbl->Valid
                    && chnTbl->Region == Adapter->RegionCode
                    && chnTbl->Band & BAND_A) {
                    /* 
                     * Set the start channel.  Get a random number and
                     *   use it to pick an entry in the table between 0
                     *   and the number of channels in the table (NrCFP).
                     */
                    randEntry = wlan_11h_get_random_num() % chnTbl->NrCFP;
                    startChn = chnTbl->CFP[randEntry].Channel;
                }
            }
        }
    }

    PRINTM(INFO, "11h: %s: AdHoc Channel set to %u\n",
           wlan_11h_is_enabled(priv) ? "Enabled" : "Disabled", startChn);

    LEAVE();

    return startChn;
}

/**
 *  @brief Check if the current region's regulations require the input channel
 *         to be scanned for radar.
 *
 *  Based on statically defined requirements for sub-bands per regulatory
 *    agency requirements.
 *
 *  Used in adhoc start to determine if channel availability check is required
 *
 *  @param priv    Private driver information structure
 *  @param channel Channel to determine radar detection requirements
 *
 *  @return
 *    - TRUE if radar detection is required
 *    - FALSE otherwise
 *
 *  @sa wlan_11h_radar_detected
 */
int
wlan_11h_radar_detect_required(wlan_private * priv, u8 channel)
{
    wlan_adapter *Adapter = priv->adapter;
    int required;

    ENTER();

    /* 
     * Assume that radar detection is required unless exempted below.
     *   No checks for 11h or measurement code  being enabled is placed here
     *   since regulatory requirements exist whether we support them or not.
     */
    required = TRUE;

    switch (priv->adapter->RegionCode) {
    case 0x10:                 /* USA FCC */
    case 0x20:                 /* Canada IC */
        /* 
         * FCC does not yet require radar detection in the
         *   5.25-5.35 (U-NII middle) band
         */
        if (channel < wlan_11h_unii_middle_band.startChan ||
            channel >= wlan_11h_unii_mid_upper_band.startChan) {
            required = FALSE;
        }
        break;
    case 0x30:                 /* Europe ETSI */
        /* 
         * Radar detection is not required in the
         *   5.15-5.25 (U-NII lower) and 5.725-5.825 (U-NII upper) bands
         */
        if (channel < wlan_11h_unii_middle_band.startChan ||
            channel >= wlan_11h_unii_upper_band.startChan) {
            /* Radar detection not required */
            required = FALSE;
        }
        break;
    default:
        break;
    }

    PRINTM(INFO, "11h: Radar detection in region %#02x "
           "is %srequired for channel %d\n",
           priv->adapter->RegionCode, (required ? "" : "NOT "), channel);

    if (required == TRUE
        && Adapter->MediaConnectStatus == WlanMediaStateConnected
        && Adapter->CurBssParams.BSSDescriptor.Channel == channel) {
        required = FALSE;

        PRINTM(INFO, "11h: Radar detection not required. "
               "Already operating on the channel");
    }

    LEAVE();
    return required;
}

/**
 *  @brief Perform a radar measurement and report the result if required on
 *         given channel
 *
 *  Check to see if the provided channel requires a channel availability
 *    check (60 second radar detection measurement).  If required, perform
 *    measurement, stalling calling thread until the measurement completes
 *    and then report result.
 *
 *  Used when starting an adhoc network.
 *
 *  @param priv    Private driver information structure
 *  @param channel Channel on which to perform radar measurement
 *
 *  @return
 *    - TRUE if radar has been detected
 *    - FALSE if radar detection is not required or radar has not been detected
 *
 *  @sa wlan_11h_radar_detect_required
 */
int
wlan_11h_radar_detected(wlan_private * priv, u8 channel)
{
    int ret;

    wlan_adapter *Adapter = priv->adapter;
    HostCmd_DS_MEASUREMENT_REQUEST measReq;
    HostCmd_DS_MEASUREMENT_REPORT measRpt;

    ENTER();

    /* 
     * If the channel requires radar, default the return value to it being
     *   detected.
     */
    ret = wlan_11h_radar_detect_required(priv, channel);

    memset(&measReq, 0x00, sizeof(measReq));
    memset(&measRpt, 0x00, sizeof(measRpt));

    /* 
     * Send a basic measurement request on the indicated channel for the
     *   required channel availability check time.
     */
    measReq.measType = WLAN_MEAS_BASIC;
    measReq.req.basic.channel = channel;
    measReq.req.basic.duration = WLAN_11H_CHANNEL_AVAIL_CHECK_DURATION;

    /* 
     * Set the STA that we are requesting the measurement from to our own
     *   mac address, causing our firmware to perform the measurement itself
     */
    memcpy(measReq.macAddr, Adapter->CurrentAddr, sizeof(measReq.macAddr));

    /* 
     * Send the measurement request and timeout duration to wait for
     *   the command to spend until the measurement report is received
     *   from the firmware.  If the command fails, the default ret value set
     *   above will be returned.
     */
    if (wlan_meas_util_send_req(priv, &measReq,
                                measReq.req.basic.duration, &measRpt) == 0) {
        /* 
         * If the report indicates no measurement was done, leave the default
         *   return value alone.
         */
        if (measRpt.rpt.basic.map.unmeasured == 0)
            /* 
             * Set the return value based on the radar indication bit
             */
            ret = measRpt.rpt.basic.map.radar ? TRUE : FALSE;

    }

    LEAVE();
    return ret;
}

/**
 *  @brief Process an TLV buffer for a pending BSS Adhoc start command.
 *
 *  Activate 11h functionality in the firmware if driver has is enabled
 *    for 11h (configured by the application via IOCTL).
 *
 *  @param priv        Private driver information structure
 *  @param ppBuffer    Output parameter: Pointer to the TLV output buffer, 
 *                     modified on return to point after the appended 11h TLVs
 *  @param pCapInfo    Pointer to the capability info for the BSS to join
 *  @param channel     Channel on which we are starting the IBSS
 *  @param p11hBssInfo Input/Output parameter: Pointer to the 11h BSS 
 *                     information for this network that we are establishing.
 *                     11h sensed flag set on output if warranted.
 *
 *  @return            Integer number of bytes appended to the TLV output
 *                     buffer (ppBuffer)
 *
 */
int
wlan_11h_process_start(wlan_private * priv,
                       u8 ** ppBuffer,
                       IEEEtypes_CapInfo_t * pCapInfo,
                       uint channel, wlan_11h_bss_info_t * p11hBssInfo)
{
    wlan_adapter *Adapter = priv->adapter;
    int ret = 0;

    ENTER();
    if (wlan_11h_is_enabled(priv) && (Adapter->adhoc_start_band == BAND_A)) {
        if (wlan_get_state_11d(priv) == DISABLE_11D) {
            /* No use having 11h enabled without 11d enabled */
            wlan_enable_11d(priv, ENABLE_11D);
            wlan_create_dnld_countryinfo_11d(priv);
        }

        /* Activate 11h functions in firmware, turns on capability bit */
        wlan_11h_activate(priv, TRUE);
        pCapInfo->SpectrumMgmt = 1;

        /* Set flag indicating this BSS we are starting is using 11h */
        p11hBssInfo->sensed11h = TRUE;

        ret = wlan_11h_process_adhoc(priv, ppBuffer, channel, NULL);
    } else {
        /* Deactivate 11h functions in the firmware */
        wlan_11h_activate(priv, FALSE);
        pCapInfo->SpectrumMgmt = 0;
    }
    LEAVE();
    return ret;
}

/**
 *  @brief Process an TLV buffer for a pending BSS Join command for
 *         both adhoc and infra networks
 *
 *  The TLV command processing for a BSS join for either adhoc or
 *    infrastructure network is performed with this function.  The
 *    capability bits are inspected for the IBSS flag and the appropriate
 *    local routines are called to setup the necessary TLVs.
 *
 *  Activate 11h functionality in the firmware if the spectrum management
 *    capability bit is found in the network information for the BSS we are
 *    joining.
 *
 *  @param priv        Private driver information structure
 *  @param ppBuffer    Output parameter: Pointer to the TLV output buffer, 
 *                     modified on return to point after the appended 11h TLVs
 *  @param pCapInfo    Pointer to the capability info for the BSS to join
 *  @param channel     Channel on which we are joining the BSS
 *  @param p11hBssInfo Pointer to the 11h BSS information for this
 *                     network that was parsed out of the scan response.
 *
 *  @return            Integer number of bytes appended to the TLV output
 *                     buffer (ppBuffer), WLAN_STATUS_FAILURE (-1), 
 *                     or WLAN_STATUS_SUCCESS (0)
 */
int
wlan_11h_process_join(wlan_private * priv,
                      u8 ** ppBuffer,
                      IEEEtypes_CapInfo_t * pCapInfo,
                      uint channel, wlan_11h_bss_info_t * p11hBssInfo)
{
    int ret = 0;

    ENTER();

    if (priv->adapter->MediaConnectStatus == WlanMediaStateConnected) {
        if (wlan_11h_is_active(priv) == p11hBssInfo->sensed11h) {
            /* Assume DFS parameters are the same for roaming as long as the
               current & next APs have the same spectrum mgmt capability bit
               setting */
            ret = WLAN_STATUS_SUCCESS;

        } else {
            /* No support for roaming between DFS/non-DFS yet */
            ret = WLAN_STATUS_FAILURE;
        }

        LEAVE();
        return ret;
    }

    if (p11hBssInfo->sensed11h) {
        /* No use having 11h enabled without 11d enabled */
        wlan_enable_11d(priv, ENABLE_11D);
        wlan_parse_dnld_countryinfo_11d(priv);

        /* Activate 11h functions in firmware, turns on capability bit */
        wlan_11h_activate(priv, TRUE);
        pCapInfo->SpectrumMgmt = 1;

        if (pCapInfo->Ibss) {
            PRINTM(INFO, "11h: Adhoc join: Sensed\n");
            ret = wlan_11h_process_adhoc(priv, ppBuffer, channel, p11hBssInfo);
        } else {
            PRINTM(INFO, "11h: Infra join: Sensed\n");
            ret = wlan_11h_process_infra_join(priv, ppBuffer,
                                              channel, p11hBssInfo);
        }
    } else {
        /* Deactivate 11h functions in the firmware */
        wlan_11h_activate(priv, FALSE);
        pCapInfo->SpectrumMgmt = 0;
    }

    LEAVE();

    return ret;
}

/**
 *
 *  @brief  Prepare the HostCmd_DS_Command structure for an 11h command.
 *
 *  Use the Command field to determine if the command being set up is for
 *     11h and call one of the local command handlers accordingly for:
 *
 *        - HostCmd_CMD_802_11_TPC_ADAPT_REQ
 *        - HostCmd_CMD_802_11_TPC_INFO
 *        - HostCmd_CMD_802_11_CHAN_SW_ANN
 *
 *  @param priv     Private driver information structure
 *  @param pCmdPtr  Output parameter: Pointer to the command being prepared 
 *                  for the firmware
 *  @param pInfoBuf Void buffer pass through with data necessary for a
 *                  specific command type
 *
 *  @return         WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 *
 *  @sa wlan_11h_cmd_tpc_request
 *  @sa wlan_11h_cmd_tpc_info
 *  @sa wlan_11h_cmd_chan_sw_ann
 */
int
wlan_11h_cmd_process(wlan_private * priv,
                     HostCmd_DS_COMMAND * pCmdPtr, const void *pInfoBuf)
{
    int ret = WLAN_STATUS_SUCCESS;

    ENTER();
    switch (pCmdPtr->Command) {
    case HostCmd_CMD_802_11_TPC_ADAPT_REQ:
        ret = wlan_11h_cmd_tpc_request(priv, pCmdPtr, pInfoBuf);
        break;
    case HostCmd_CMD_802_11_TPC_INFO:
        ret = wlan_11h_cmd_tpc_info(priv, pCmdPtr, pInfoBuf);
        break;
    case HostCmd_CMD_802_11_CHAN_SW_ANN:
        ret = wlan_11h_cmd_chan_sw_ann(priv, pCmdPtr, pInfoBuf);
        break;
    default:
        ret = WLAN_STATUS_FAILURE;
    }

    pCmdPtr->Command = wlan_cpu_to_le16(pCmdPtr->Command);
    pCmdPtr->Size = wlan_cpu_to_le16(pCmdPtr->Size);
    LEAVE();
    return ret;
}

/**
 *  @brief Handle the command response from the firmware if from an 11h command
 *
 *  Use the Command field to determine if the command response being
 *    is for 11h.  Call the local command response handler accordingly for:
 *
 *        - HostCmd_CMD_802_11_TPC_ADAPT_REQ
 *        - HostCmd_CMD_802_11_TPC_INFO
 *        - HostCmd_CMD_802_11_CHAN_SW_ANN
 *
 *  @param priv  Private driver information structure
 *  @param resp  HostCmd_DS_COMMAND struct returned from the firmware
 *               command
 *
 *  @return      WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
int
wlan_11h_cmdresp_process(wlan_private * priv, const HostCmd_DS_COMMAND * resp)
{
    int ret = WLAN_STATUS_SUCCESS;

    ENTER();
    switch (resp->Command) {
    case HostCmd_CMD_802_11_TPC_ADAPT_REQ:
        HEXDUMP("11h: TPC REQUEST Rsp:", (u8 *) resp, (int) resp->Size);
        memcpy(priv->adapter->CurCmd->pdata_buf,
               &resp->params.tpcReq, sizeof(HostCmd_DS_802_11_TPC_ADAPT_REQ));
        break;

    case HostCmd_CMD_802_11_TPC_INFO:
        HEXDUMP("11h: TPC INFO Rsp Data:", (u8 *) resp, (int) resp->Size);
        break;

    case HostCmd_CMD_802_11_CHAN_SW_ANN:
        PRINTM(INFO, "11h: Ret ChSwAnn: Sz=%u, Seq=%u, Ret=%u\n",
               resp->Size, resp->SeqNum, resp->Result);
        break;

    default:
        ret = WLAN_STATUS_FAILURE;
    }
    LEAVE();
    return ret;
}

/**
 *  @brief Process an element from a scan response, copy relevant info for 11h
 *
 *  @param p11hBssInfo  Output parameter: Pointer to the 11h BSS information
 *                      for the network that is being processed
 *  @param pElement     Pointer to the current IE we are inspecting for 11h
 *                      relevance
 *
 *  @return             WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
int
wlan_11h_process_bss_elem(wlan_11h_bss_info_t * p11hBssInfo,
                          const u8 * pElement)
{
    int ret = WLAN_STATUS_SUCCESS;

    ENTER();
    switch (*pElement) {
    case POWER_CONSTRAINT:
        PRINTM(INFO, "11h: Power Constraint IE Found\n");
        p11hBssInfo->sensed11h = 1;
        memcpy(&p11hBssInfo->powerConstraint, pElement,
               sizeof(IEEEtypes_PowerConstraint_t));
        break;

    case POWER_CAPABILITY:
        PRINTM(INFO, "11h: Power Capability IE Found\n");
        p11hBssInfo->sensed11h = 1;
        memcpy(&p11hBssInfo->powerCapability, pElement,
               sizeof(IEEEtypes_PowerCapability_t));
        break;

    case TPC_REPORT:
        PRINTM(INFO, "11h: Tpc Report IE Found\n");
        p11hBssInfo->sensed11h = 1;
        memcpy(&p11hBssInfo->tpcReport, pElement,
               sizeof(IEEEtypes_TPCReport_t));
        break;

    case CHANNEL_SWITCH_ANN:
        p11hBssInfo->sensed11h = 1;
        PRINTM(INFO, "11h: Channel Switch Ann IE Found\n");
        break;

    case QUIET:
        PRINTM(INFO, "11h: Quiet IE Found\n");
        p11hBssInfo->sensed11h = 1;
        memcpy(&p11hBssInfo->quiet, pElement, sizeof(IEEEtypes_Quiet_t));
        break;

    case IBSS_DFS:
        PRINTM(INFO, "11h: Ibss Dfs IE Found\n");
        p11hBssInfo->sensed11h = 1;
        memcpy(&p11hBssInfo->ibssDfs, pElement, sizeof(IEEEtypes_IBSS_DFS_t));
        break;

    case SUPPORTED_CHANNELS:
    case TPC_REQUEST:
        /* 
         * These elements are not in beacons/probe responses.  Included here
         *   to cover set of enumerated 11h elements.
         */
        break;

    default:
        ret = WLAN_STATUS_FAILURE;
    }
    LEAVE();
    return ret;
}

/**
 *  @brief IOCTL handler to send a channel switch announcement via
 *         firmware command
 *
 *  @param priv  Private driver information structure
 *  @param wrq   OS IOCTL passed structure encoded with a 
 *               HostCmd_DS_802_11_CHAN_SW_ANN structure
 *
 *  @return
 *    - 0 for success
 *    - -EPROTO if 11h is not active in the firmware
 *    - -EFAULT if memory copy from user space
 *    - Error return from wlan_prepare_cmd routine otherwise
 *             
 */
int
wlan_11h_ioctl_chan_sw_ann(wlan_private * priv, struct iwreq *wrq)
{
    HostCmd_DS_802_11_CHAN_SW_ANN chSwParams;
    int ret;

    ENTER();

    if (!wlan_11h_is_active(priv)) {
        ret = -EPROTO;
    } else if (copy_from_user(&chSwParams, wrq->u.data.pointer,
                              sizeof(chSwParams)) != 0) {
        /* copy_from_user failed */
        PRINTM(INFO, "11h: ioctl_chan_sw_ann: copy from user failed\n");
        ret = -EFAULT;
    } else {
        /* 
         * Send the channel switch announcment to firmware
         */

        ret = wlan_prepare_cmd(priv, HostCmd_CMD_802_11_CHAN_SW_ANN,
                               HostCmd_ACT_GEN_SET,
                               HostCmd_OPTION_WAITFORRSP, 0,
                               (void *) &chSwParams);
    }

    LEAVE();

    return ret;
}

/**
 *  @brief IOCTL handler to return the user defined local power constraint
 *
 *  @param priv Private driver information structure
 *  @param wrq  Input/Output parameter: OS IOCTL passed structure.  Used to set
 *              the local power constraint if supplied on input.  Currently
 *              defined local power constraint provided on output.
 *
 *  @return
 *    - 0 for success in all cases
 */
int
wlan_11h_ioctl_get_local_power(wlan_private * priv, struct iwreq *wrq)
{
    int data;
    s8 *localpowerPtr = &priv->adapter->state11h.usrDefPowerConstraint;
    int localpower;

    ENTER();

    if (wrq->u.data.length != 0) {
        if (copy_from_user(&data, wrq->u.data.pointer, sizeof(int))) {
            PRINTM(INFO, "Copy from user failed\n");
            return -EFAULT;
        }
        *localpowerPtr = (s8) data;
    }

    localpower = *localpowerPtr;
    if (copy_to_user(wrq->u.data.pointer, &localpower, sizeof(int))) {
        PRINTM(INFO, "Copy to user failed\n");
        return -EFAULT;
    }

    wrq->u.data.length = 1;

    LEAVE();

    return 0;
}

/**
 *  @brief IOCTL handler to send a TPC request to a given STA address.
 *
 *  @param priv Private driver information structure
 *  @param wrq  Input/Output parameter: OS IOCTL passed structure containing:
 *                 - wlan_ioctl_11h_tpc_req on input
 *                 - wlan_ioctl_11h_tpc_resp on output if successful
 *
 *  @return
 *    - 0 for success
 *    - -EPROTO if 11h is not active in the firmware
 *    - -EFAULT if memory copy from user space 
 *    - Error return from wlan_prepare_cmd routine otherwise
 */
int
wlan_11h_ioctl_request_tpc(wlan_private * priv, struct iwreq *wrq)
{
    wlan_ioctl_11h_tpc_req tpcIoctlReq;
    wlan_ioctl_11h_tpc_resp tpcIoctlResp;
    HostCmd_DS_802_11_TPC_ADAPT_REQ tpcReq;

    int ret = 0;

    ENTER();

    if (!wlan_11h_is_active(priv)) {
        ret = -EPROTO;
    } else if (copy_from_user(&tpcIoctlReq, wrq->u.data.pointer,
                              sizeof(tpcIoctlReq)) != 0) {
        /* copy_from_user failed */
        PRINTM(INFO, "11h: ioctl_request_tpc: copy from user failed\n");
        ret = -EFAULT;
    } else {
        PRINTM(INFO, "11h: ioctl_request_tpc req: "
               "%02x:%02x:%02x:%02x:%02x:%02x, rateIdx(%d), timeout(%d)\n",
               tpcIoctlReq.destMac[0], tpcIoctlReq.destMac[1],
               tpcIoctlReq.destMac[2], tpcIoctlReq.destMac[3],
               tpcIoctlReq.destMac[4], tpcIoctlReq.destMac[5],
               tpcIoctlReq.rateIndex, tpcIoctlReq.timeout);

        memset(&tpcReq, 0, sizeof(tpcReq));
        memset(&tpcIoctlResp, 0, sizeof(tpcIoctlResp));

        memcpy(tpcReq.req.destMac, tpcIoctlReq.destMac,
               sizeof(tpcReq.req.destMac));
        tpcReq.req.rateIndex = tpcIoctlReq.rateIndex;
        tpcReq.req.timeout = tpcIoctlReq.timeout;

        ret = wlan_prepare_cmd(priv, HostCmd_CMD_802_11_TPC_ADAPT_REQ,
                               HostCmd_ACT_GEN_SET,
                               HostCmd_OPTION_WAITFORRSP, 0, &tpcReq);

        if (ret == 0) {
            tpcIoctlResp.statusCode = tpcReq.resp.tpcRetCode;
            tpcIoctlResp.txPower = tpcReq.resp.txPower;
            tpcIoctlResp.linkMargin = tpcReq.resp.linkMargin;
            tpcIoctlResp.rssi = tpcReq.resp.rssi;

            wrq->u.data.length = sizeof(tpcIoctlResp);
            if (copy_to_user(wrq->u.data.pointer, &tpcIoctlResp,
                             wrq->u.data.length)) {
                PRINTM(INFO, "Copy to user failed\n");
                return -EFAULT;
            }

            PRINTM(INFO, "11h: ioctl_request_tpc resp: "
                   "Ret(%d), TxPwr(%d), LinkMrgn(%d), RSSI(%d)\n",
                   tpcIoctlResp.statusCode,
                   tpcIoctlResp.txPower,
                   tpcIoctlResp.linkMargin, tpcIoctlResp.rssi);
        }
    }

    LEAVE();

    return ret;
}

/**
 * @brief IOCTL handler to set  min/max power in the 11h power capability IE
 *
 *  @param priv  Private driver information structure
 *  @param wrq   OS IOCTL passed structure
 *
 *  @return
 *    - 0 for success
 *    - -EFAULT if memory copy from user space fails
 */
int
wlan_11h_ioctl_set_power_cap(wlan_private * priv, struct iwreq *wrq)
{
    wlan_adapter *Adapter = priv->adapter;
    wlan_ioctl_11h_power_cap powerCapIoctl;
    int ret = 0;

    ENTER();

    if (copy_from_user(&powerCapIoctl, wrq->u.data.pointer,
                       sizeof(powerCapIoctl)) == 0) {
        Adapter->state11h.minTxPowerCapability = powerCapIoctl.minTxPower;
        Adapter->state11h.maxTxPowerCapability = powerCapIoctl.maxTxPower;

        PRINTM(INFO, "11h: ioctl_set_power_cap, Min(%d) MaxPwr(%d)\n",
               Adapter->state11h.minTxPowerCapability,
               Adapter->state11h.maxTxPowerCapability);
    } else {
        ret = -EFAULT;
    }

    LEAVE();

    return ret;
}

/**
 * @brief IOCTL handler to set  values for the Quiet IE
 *
 *  @param priv  Private driver information structure
 *  @param wrq   OS IOCTL passed structure
 *
 *  @return
 *    - 0 for success
 *    - -EFAULT if memory copy from user space fails
 */
int
wlan_11h_ioctl_set_quiet_ie(wlan_private * priv, struct iwreq *wrq)
{
    wlan_adapter *Adapter = priv->adapter;
    int ret = 0;
    int data[4];

    ENTER();

    if ((wrq->u.data.length > 4) ||
        (wrq->u.data.length < 4 && wrq->u.data.length != 0)) {
        PRINTM(MSG, "Invalid no of arguments\n");
        ret = -EFAULT;
    } else {
        if (wrq->u.data.length != 0) {
            if (copy_from_user
                (data, wrq->u.data.pointer,
                 wrq->u.data.length * sizeof(int)) == 0) {
                Adapter->state11h.quiet_ie.quietCount = (u8) data[0];
                Adapter->state11h.quiet_ie.quietPeriod = (u8) data[1];
                Adapter->state11h.quiet_ie.quietDuration = (u16) data[2];
                Adapter->state11h.quiet_ie.quietOffset = (u16) data[3];

                Adapter->state11h.quiet_ie.quietDuration =
                    wlan_cpu_to_le16(Adapter->state11h.quiet_ie.quietDuration);

                Adapter->state11h.quiet_ie.quietOffset =
                    wlan_cpu_to_le16(Adapter->state11h.quiet_ie.quietOffset);

                PRINTM(INFO,
                       "11h: ioctl_set_quiet_ie: quietCount= %d quietPeriod = %d quietDuration=%d quietOffset=%d\n",
                       Adapter->state11h.quiet_ie.quietCount,
                       Adapter->state11h.quiet_ie.quietPeriod,
                       Adapter->state11h.quiet_ie.quietDuration,
                       Adapter->state11h.quiet_ie.quietOffset);
            } else {
                ret = -EFAULT;
            }
        } else {
            data[0] = Adapter->state11h.quiet_ie.quietCount;
            data[1] = Adapter->state11h.quiet_ie.quietPeriod;
            data[2] = Adapter->state11h.quiet_ie.quietDuration;
            data[3] = Adapter->state11h.quiet_ie.quietOffset;
            wrq->u.data.length = 4;

            if (copy_to_user(wrq->u.data.pointer, data,
                             wrq->u.data.length * sizeof(int))) {
                PRINTM(INFO, "Copy to user failed\n");
                LEAVE();
                ret = -EFAULT;
            }
        }
    }
    LEAVE();
    return ret;
}
