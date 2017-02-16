/** @file wlan_join.c
 *
 *  @brief Functions implementing wlan infrastructure and adhoc join routines
 *
 *  IOCTL handlers as well as command preparation and response routines
 *   for sending adhoc start, adhoc join, and association commands
 *   to the firmware.
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
    01/11/06: Initial revision. Match new scan code. Relocate related functions
    01/19/06: Fix failure to save adhoc ssid as current after adhoc start
    03/16/06: Add a semaphore to protect reassociation thread

************************************************************/

#include    "wlan_headers.h"

/**
 *  @brief This function finds out the common rates between rate1 and rate2.
 *
 * It will fill common rates in rate1 as output if found.
 *
 * NOTE: Setting the MSB of the basic rates need to be taken
 *   care, either before or after calling this function
 *
 *  @param Adapter     A pointer to wlan_adapter structure
 *  @param rate1       the buffer which keeps input and output
 *  @param rate1_size  the size of rate1 buffer
 *  @param rate2       the buffer which keeps rate2
 *  @param rate2_size  the size of rate2 buffer.
 *
 *  @return            WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
static int
get_common_rates(wlan_adapter * Adapter, u8 * rate1,
                 int rate1_size, u8 * rate2, int rate2_size)
{
    u8 *ptr = rate1;
    int ret = WLAN_STATUS_SUCCESS;
    u8 *tmp = NULL;
    int i, j;

    ENTER();
    if (!(tmp = kmalloc(rate1_size, GFP_KERNEL))) {
        PRINTM(WARN, "Allocate buffer for common rates failed\n");
        LEAVE();
        return -ENOMEM;
    }

    memcpy(tmp, rate1, rate1_size);
    memset(rate1, 0, rate1_size);

    for (i = 0; rate2[i] && i < rate2_size; i++) {
        for (j = 0; tmp[j] && j < rate1_size; j++) {
            /* Check common rate, excluding the bit for basic rate */
            if ((rate2[i] & 0x7F) == (tmp[j] & 0x7F)) {
                *rate1++ = tmp[j];
                break;
            }
        }
    }

    HEXDUMP("rate1 (AP) Rates", tmp, rate1_size);
    HEXDUMP("rate2 (Card) Rates", rate2, rate2_size);
    HEXDUMP("Common Rates", ptr, rate1 - ptr);
    PRINTM(INFO, "Tx DataRate is set to 0x%X\n", Adapter->DataRate);

    if (!Adapter->Is_DataRate_Auto) {
        while (*ptr) {
            if ((*ptr & 0x7f) == Adapter->DataRate) {
                ret = WLAN_STATUS_SUCCESS;
                goto done;
            }
            ptr++;
        }
        PRINTM(MSG, "Previously set fixed data rate %#x is not "
               "compatible with the network\n", Adapter->DataRate);

        ret = WLAN_STATUS_FAILURE;
        goto done;
    }

    ret = WLAN_STATUS_SUCCESS;
  done:
    kfree(tmp);
    LEAVE();
    return ret;
}

/**
 *  @brief Create the intersection of the rates supported by a target BSS and
 *         our Adapter settings for use in an assoc/join command.
 *
 *  @param Adapter       A pointer to wlan_adapter structure
 *  @param pBSSDesc      BSS Descriptor whose rates are used in the setup
 *  @param pOutRates     Output: Octet array of rates common between the BSS
 *                       and the Adapter supported rates settings
 *  @param pOutRatesSize Output: Number of rates/octets set in pOutRates
 *
 *  @return              WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 *
 */
static int
setup_rates_from_bssdesc(wlan_adapter * Adapter,
                         BSSDescriptor_t * pBSSDesc,
                         u8 * pOutRates, int *pOutRatesSize)
{
    u8 *card_rates;
    int card_rates_size;

    ENTER();

    /* Copy AP supported rates */
    memcpy(pOutRates, pBSSDesc->SupportedRates, WLAN_SUPPORTED_RATES);

    /* Get the STA supported rates */
    switch (Adapter->config_bands) {
    case BAND_B:
        PRINTM(INFO, "Setup B-Band Rates: %d\n", Adapter->config_bands);
        card_rates = SupportedRates_B;
        card_rates_size = sizeof(SupportedRates_B);
        break;

    case BAND_A:
        PRINTM(INFO, "Setup A-Band Rates: %d\n", Adapter->config_bands);
        card_rates = SupportedRates_A;
        card_rates_size = sizeof(SupportedRates_A);
        break;

    case BAND_G:
    case BAND_B | BAND_G:
    case BAND_A | BAND_B | BAND_G:
        PRINTM(INFO, "Setup G-Band Rates: %d\n", Adapter->config_bands);
        card_rates = SupportedRates_G;
        card_rates_size = sizeof(SupportedRates_G);
        break;

    default:
        PRINTM(INFO, "Invalid band setting: %d\n", Adapter->config_bands);
        LEAVE();
        return WLAN_STATUS_FAILURE;
    }

    /* Get the common rates between AP and STA supported rates */
    if (get_common_rates(Adapter, pOutRates, WLAN_SUPPORTED_RATES,
                         card_rates, card_rates_size)) {
        *pOutRatesSize = 0;
        PRINTM(INFO, "get_common_rates failed\n");
        LEAVE();
        return WLAN_STATUS_FAILURE;
    }

    *pOutRatesSize = MIN(strlen(pOutRates), WLAN_SUPPORTED_RATES);

    LEAVE();

    return WLAN_STATUS_SUCCESS;
}

/**
 *  @brief Convert band to radio type used in channel TLV
 *
 *  @param scanBand  Band enumeration to convert to a channel TLV radio type
 *
 *  @return          Radio type designator for use in a channel TLV
 *
 */
static u8
band_to_radio_type(u8 band)
{
    u8 retRadioType;

    switch (band) {
    case BAND_A:
        retRadioType = HostCmd_SCAN_RADIO_TYPE_A;
        break;
    case BAND_B:
    case BAND_G:
    default:
        retRadioType = HostCmd_SCAN_RADIO_TYPE_BG;
        break;
    }

    return retRadioType;
}

/**
 *  @brief Retrieve the association response
 *
 *  @param priv         A pointer to wlan_private structure
 *  @param wrq          A pointer to iwreq structure
 *  @return             WLAN_STATUS_SUCCESS --success, otherwise fail
 */
int
wlan_get_assoc_rsp_ioctl(wlan_private * priv, struct iwreq *wrq)
{
    wlan_adapter *Adapter = priv->adapter;
    int copySize;

    ENTER();
    /* 
     * Set the amount to copy back to the application as the minimum of the
     *   available assoc resp data or the buffer provided by the application
     */
    copySize = MIN(Adapter->assocRspSize, wrq->u.data.length);

    /* Copy the (re)association response back to the application */
    if (copy_to_user(wrq->u.data.pointer, Adapter->assocRspBuffer, copySize)) {
        PRINTM(INFO, "Copy to user failed\n");
        LEAVE();
        return -EFAULT;
    }

    /* Returned copy length */
    wrq->u.data.length = copySize;

    /* Reset assoc buffer */
    Adapter->assocRspSize = 0;

    /* No error on return */
    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/**
 *  @brief Set an opaque block of Marvell TLVs for insertion into the
 *         association command
 *
 *  Pass an opaque block of data, expected to be Marvell TLVs, to the driver
 *    for eventual pass through to the firmware in an associate/join
 *    (and potentially start) command.
 *
 *
 *  @param priv         A pointer to wlan_private structure
 *  @param wrq          A pointer to iwreq structure
 *  @return             WLAN_STATUS_SUCCESS --success, otherwise fail
 */
int
wlan_set_mrvl_tlv_ioctl(wlan_private * priv, struct iwreq *wrq)
{
    wlan_adapter *Adapter = priv->adapter;
    int ret = WLAN_STATUS_SUCCESS;

    ENTER();
    /* If the passed length is zero, reset the buffer */
    if (wrq->u.data.length == 0)
        Adapter->mrvlAssocTlvBufferLen = 0;
    else {
        /* 
         * Verify that the passed length is not larger than the available
         *   space remaining in the buffer
         */
        if (wrq->u.data.length < (sizeof(Adapter->mrvlAssocTlvBuffer)
                                  - Adapter->mrvlAssocTlvBufferLen)) {
            /* Append the passed data to the end of the mrvlAssocTlvBuffer */
            if (copy_from_user(Adapter->mrvlAssocTlvBuffer
                               + Adapter->mrvlAssocTlvBufferLen,
                               wrq->u.data.pointer, wrq->u.data.length)) {
                PRINTM(INFO, "Copy from user failed\n");
                LEAVE();
                return -EFAULT;
            }

            /* Increment the stored buffer length by the size passed */
            Adapter->mrvlAssocTlvBufferLen += wrq->u.data.length;
        } else {
            /* Passed data does not fit in the remaining buffer space */
            ret = WLAN_STATUS_FAILURE;
        }
    }

    /* Return WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE */
    LEAVE();
    return ret;
}

/**
 *  @brief Stop Adhoc Network
 *
 *  @param priv         A pointer to wlan_private structure
 *  @return             WLAN_STATUS_SUCCESS --success, otherwise fail
 */
int
wlan_do_adhocstop_ioctl(wlan_private * priv)
{
    wlan_adapter *Adapter = priv->adapter;
    int ret = WLAN_STATUS_SUCCESS;

    ENTER();

    if (Adapter->InfrastructureMode == Wlan802_11IBSS)
        ret = wlan_disconnect(priv);
    else {
        LEAVE();
        return -ENOTSUPP;
    }

    LEAVE();

    return WLAN_STATUS_SUCCESS;
}

/**
 *  @brief Set essid
 *
 *  @param dev          A pointer to net_device structure
 *  @param info         A pointer to iw_request_info structure
 *  @param dwrq         A pointer to iw_point structure
 *  @param extra        A pointer to extra data buf
 *  @return             WLAN_STATUS_SUCCESS--success, otherwise--fail
 */
int
wlan_set_essid(struct net_device *dev, struct iw_request_info *info,
               struct iw_point *dwrq, char *extra)
{
    wlan_private *priv = netdev_priv(dev);
    wlan_adapter *Adapter = priv->adapter;
    int ret = WLAN_STATUS_SUCCESS;
    WLAN_802_11_SSID reqSSID;
    int i;

    ENTER();

    if (!wlan_is_cmd_allowed(priv)) {
        PRINTM(MSG, "%s: not allowed\n", __FUNCTION__);
        LEAVE();
        return -EBUSY;
    }

    /* Clear any past association response stored for application retrieval */
    Adapter->assocRspSize = 0;

#ifdef REASSOCIATION
    /* cancel re-association timer if there's one */
    if (Adapter->ReassocTimerIsSet == TRUE) {
        wlan_cancel_timer(&Adapter->MrvDrvTimer);
        Adapter->ReassocTimerIsSet = FALSE;
    }

    if (OS_ACQ_SEMAPHORE_BLOCK(&Adapter->ReassocSem)) {
        PRINTM(ERROR, "Acquire semaphore error, wlan_set_essid\n");
        LEAVE();
        return -EBUSY;
    }
#endif /* REASSOCIATION */

    /* Check the size of the string */
    if (dwrq->length > IW_ESSID_MAX_SIZE + 1) {
        ret = -E2BIG;
        goto setessid_ret;
    }

    memset(&reqSSID, 0, sizeof(WLAN_802_11_SSID));

    /* 
     * Check if we asked for `any' or 'particular'
     */
    if (!dwrq->flags) {
        if (wlan_find_best_network_ssid(priv, &reqSSID)) {
            PRINTM(INFO, "Could not find best network\n");
            ret = WLAN_STATUS_SUCCESS;
            goto setessid_ret;
        }
    } else {
        /* Set the SSID */
#if WIRELESS_EXT > 20
        reqSSID.SsidLength = dwrq->length;
#else
        reqSSID.SsidLength = dwrq->length - 1;
#endif
        memcpy(reqSSID.Ssid, extra, reqSSID.SsidLength);

    }

    PRINTM(INFO, "Requested new SSID = %s\n",
           (reqSSID.SsidLength > 0) ? (char *) reqSSID.Ssid : "NULL");

    if (!reqSSID.SsidLength || reqSSID.Ssid[0] < 0x20) {
        PRINTM(INFO, "Invalid SSID - aborting set_essid\n");
        ret = -EINVAL;
        goto setessid_ret;
    }

    if (Adapter->InfrastructureMode == Wlan802_11Infrastructure) {
        /* infrastructure mode */
        PRINTM(INFO, "SSID requested = %s\n", reqSSID.Ssid);

        if ((dwrq->flags & IW_ENCODE_INDEX) > 1) {
            i = (dwrq->flags & IW_ENCODE_INDEX) - 1;    /* convert to 0 based */

            PRINTM(INFO, "Request SSID by index = %d\n", i);

            if (i > Adapter->NumInScanTable)
                /* Failed to find in table since index is > current max. */
                i = -EINVAL;
        } else {
            wlan_cmd_specific_scan_ssid(priv, &reqSSID);
            i = wlan_find_ssid_in_list(Adapter,
                                       &reqSSID,
                                       NULL, Wlan802_11Infrastructure);
        }

        if (i >= 0) {
            PRINTM(INFO, "SSID found in scan list ... associating...\n");

            ret = wlan_associate(priv, &Adapter->ScanTable[i]);

            if (ret)
                goto setessid_ret;
        } else {                /* i >= 0 */
            ret = i;            /* return -ENETUNREACH, passed from
                                   wlan_find_ssid_in_list */
            goto setessid_ret;
        }
    } else {
        /* ad hoc mode */
        /* If the requested SSID matches current SSID return */
        if (!wlan_ssid_cmp(&Adapter->CurBssParams.BSSDescriptor.Ssid, &reqSSID)) {
            ret = WLAN_STATUS_SUCCESS;
            goto setessid_ret;
        }

        /* 
         * Exit Adhoc mode
         */
        PRINTM(INFO, "Sending Adhoc Stop\n");
        ret = wlan_disconnect(priv);

        if (ret)
            goto setessid_ret;

        Adapter->AdhocLinkSensed = FALSE;

        if ((dwrq->flags & IW_ENCODE_INDEX) > 1) {
            i = (dwrq->flags & IW_ENCODE_INDEX) - 1;    /* 0 based */
            if (i > Adapter->NumInScanTable)
                /* Failed to find in table since index is > current max. */
                i = -EINVAL;
        } else {
            /* Scan for the network */
            wlan_cmd_specific_scan_ssid(priv, &reqSSID);

            /* Search for the requested SSID in the scan table */
            i = wlan_find_ssid_in_list(Adapter, &reqSSID, NULL, Wlan802_11IBSS);
        }

        if (i >= 0) {
            PRINTM(INFO, "SSID found at %d in List, so join\n", i);
            wlan_join_adhoc(priv, &Adapter->ScanTable[i]);
        } else {
            /* else send START command */
            PRINTM(INFO, "SSID not found in list, "
                   "so creating adhoc with ssid = %s\n", reqSSID.Ssid);

            wlan_start_adhoc(priv, &reqSSID);
        }                       /* end of else (START command) */
    }                           /* end of else (Ad hoc mode) */

    /* 
     * The MediaConnectStatus change can be removed later when
     *   the ret code is being properly returned.
     */
    /* Check to see if we successfully connected */
    if (Adapter->MediaConnectStatus == WlanMediaStateConnected)
        ret = WLAN_STATUS_SUCCESS;
    else
        ret = -ENETDOWN;

  setessid_ret:
#ifdef REASSOCIATION
    OS_REL_SEMAPHORE(&Adapter->ReassocSem);
#endif

    LEAVE();
    return ret;
}

/**
 *  @brief Connect to the AP or Ad-hoc Network with specific bssid
 *
 * NOTE: Scan should be issued by application before this function is called
 *
 *  @param dev          A pointer to net_device structure
 *  @param info         A pointer to iw_request_info structure
 *  @param awrq         A pointer to iw_param structure
 *  @param extra        A pointer to extra data buf
 *  @return             WLAN_STATUS_SUCCESS --success, otherwise fail
 */
int
wlan_set_wap(struct net_device *dev, struct iw_request_info *info,
             struct sockaddr *awrq, char *extra)
{
    wlan_private *priv = netdev_priv(dev);
    wlan_adapter *Adapter = priv->adapter;
    int ret = WLAN_STATUS_SUCCESS;
    const u8 bcast[ETH_ALEN] = { 255, 255, 255, 255, 255, 255 };
    const u8 zeroMac[] = { 0, 0, 0, 0, 0, 0 };
    u8 reqBSSID[ETH_ALEN];
    int i;

    ENTER();

    if (!wlan_is_cmd_allowed(priv)) {
        PRINTM(MSG, "%s: not allowed\n", __FUNCTION__);
        ret = -EBUSY;
        goto done;
    }
    if (awrq->sa_family != ARPHRD_ETHER) {
        ret = -EINVAL;
        goto done;
    }
    /* Clear any past association response stored for application retrieval */
    Adapter->assocRspSize = 0;
    PRINTM(INFO, "ASSOC: WAP: sa_data: %02x:%02x:%02x:%02x:%02x:%02x\n",
           (u8) awrq->sa_data[0], (u8) awrq->sa_data[1],
           (u8) awrq->sa_data[2], (u8) awrq->sa_data[3],
           (u8) awrq->sa_data[4], (u8) awrq->sa_data[5]);

#ifdef REASSOCIATION
    /* cancel re-association timer if there's one */
    if (Adapter->ReassocTimerIsSet == TRUE) {
        wlan_cancel_timer(&Adapter->MrvDrvTimer);
        Adapter->ReassocTimerIsSet = FALSE;
    }
#endif /* REASSOCIATION */
    /* zeroMac mean disconnect */
    if (!memcmp(zeroMac, awrq->sa_data, ETH_ALEN)) {
        wlan_disconnect(priv);
        goto done;
    }
    if (!memcmp(bcast, awrq->sa_data, ETH_ALEN))
        i = wlan_find_best_ssid_in_list(Adapter);
    else {
        /* check if we already assoicate to the AP */
        if (Adapter->MediaConnectStatus == WlanMediaStateConnected) {
            if (memcmp
                (awrq->sa_data, Adapter->CurBssParams.BSSDescriptor.MacAddress,
                 ETH_ALEN) == 0)
                goto done;
        }
        memcpy(reqBSSID, awrq->sa_data, ETH_ALEN);

        PRINTM(INFO, "ASSOC: WAP: Bssid = %02x:%02x:%02x:%02x:%02x:%02x\n",
               reqBSSID[0], reqBSSID[1], reqBSSID[2],
               reqBSSID[3], reqBSSID[4], reqBSSID[5]);

        /* Search for index position in list for requested MAC */
        i = wlan_find_bssid_in_list(Adapter, reqBSSID,
                                    Adapter->InfrastructureMode);
    }

    if (i < 0) {
        PRINTM(INFO, "ASSOC: WAP: MAC address not found in BSSID List\n");
        ret = -ENETUNREACH;
        goto done;
    }

    if (Adapter->InfrastructureMode == Wlan802_11Infrastructure) {
        if ((ret = wlan_associate(priv, &Adapter->ScanTable[i])))
            goto done;

    } else {
        /* Exit Adhoc mode */
        if ((ret = wlan_disconnect(priv)))
            goto done;
        Adapter->AdhocLinkSensed = FALSE;

        wlan_join_adhoc(priv, &Adapter->ScanTable[i]);
    }

    /* Check to see if we successfully connected */
    if (Adapter->MediaConnectStatus == WlanMediaStateConnected)
        ret = WLAN_STATUS_SUCCESS;
    else
        ret = -ENETDOWN;

  done:
    LEAVE();
    return ret;
}

/**
 *  @brief Associated to a specific BSS discovered in a scan
 *
 *  @param priv      A pointer to wlan_private structure
 *  @param pBSSDesc  Pointer to the BSS descriptor to associate with.
 *
 *  @return          WLAN_STATUS_SUCCESS-success, otherwise fail
 */
int
wlan_associate(wlan_private * priv, BSSDescriptor_t * pBSSDesc)
{
    wlan_adapter *Adapter = priv->adapter;
    int enableData = TRUE;
    union iwreq_data wrqu;
    int ret;
    IEEEtypes_AssocRsp_t *pAssocRsp;
    u8 currentBSSID[ETH_ALEN];
    int reassocAttempt = FALSE;

    ENTER();

    /* Return error if the Adapter or table entry is not marked as infra */
    if ((Adapter->InfrastructureMode != Wlan802_11Infrastructure)
        || (pBSSDesc->InfrastructureMode != Wlan802_11Infrastructure)) {
        LEAVE();
        return -EINVAL;
    }

    memcpy(&currentBSSID,
           &Adapter->CurBssParams.BSSDescriptor.MacAddress,
           sizeof(currentBSSID));

    if (Adapter->MediaConnectStatus == WlanMediaStateConnected) {
        reassocAttempt = TRUE;
        PRINTM(INFO, "Attempting reassociation, stopping wmm queues\n");
        wmm_stop_queue(priv);
    }

    /* Clear any past association response stored for application retrieval */
    Adapter->assocRspSize = 0;

    ret = wlan_prepare_cmd(priv, HostCmd_CMD_802_11_ASSOCIATE,
                           0, HostCmd_OPTION_WAITFORRSP, 0, pBSSDesc);

    if (Adapter->wmm.enabled)
        /* Don't re-enable carrier until we get the WMM_GET_STATUS event */
        enableData = FALSE;
    else
        /* Since WMM is not enabled, setup the queues with the defaults */
        wmm_setup_queues(priv);

    if (Adapter->MediaConnectStatus == WlanMediaStateConnected) {

        if (reassocAttempt
            && (memcmp(&currentBSSID,
                       &Adapter->CurBssParams.BSSDescriptor.MacAddress,
                       sizeof(currentBSSID)) == 0))

            /* Reassociation attempt failed, still associated to old AP, no
               need to wait for WMM notification to restart data */
            enableData = TRUE;
        if (enableData) {
            PRINTM(INFO, "Post association, re-enabling data flow\n");
            wmm_start_queue(priv);
            os_carrier_on(priv);
            os_start_queue(priv);
        }
    } else {
        PRINTM(INFO, "Post association, stopping data flow\n");
        os_carrier_off(priv);
        os_stop_queue(priv);
    }
    pAssocRsp = (IEEEtypes_AssocRsp_t *) Adapter->assocRspBuffer;

    if (ret || pAssocRsp->StatusCode)
        ret = -ENETUNREACH;
    else {
        memcpy(wrqu.ap_addr.sa_data,
               &Adapter->CurBssParams.BSSDescriptor.MacAddress, ETH_ALEN);
        wrqu.ap_addr.sa_family = ARPHRD_ETHER;
        wireless_send_event(priv->wlan_dev.netdev, SIOCGIWAP, &wrqu, NULL);
    }
    LEAVE();
    return ret;
}

/**
 *  @brief Associated to a specific indexed entry in the ScanTable
 *
 *  @param priv      A pointer to wlan_private structure
 *  @param tableIdx  Index into the ScanTable to associate to, index parameter
 *                   base value is 1.  No scanning is done before the 
 *                   association attempt.
 *
 *  @return          WLAN_STATUS_SUCCESS-success, otherwise fail
 */
int
wlan_associate_to_table_idx(wlan_private * priv, int tableIdx)
{
    wlan_adapter *Adapter = priv->adapter;
    int ret;

    ENTER();

#ifdef REASSOCIATION
    if (OS_ACQ_SEMAPHORE_BLOCK(&Adapter->ReassocSem)) {
        PRINTM(ERROR, "Acquire semaphore error\n");
        return -EBUSY;
    }
#endif

    PRINTM(INFO, "ASSOC: iwpriv: Index = %d, NumInScanTable = %d\n",
           tableIdx, Adapter->NumInScanTable);

    /* 
     * Check index in table, subtract 1 if within range and call association
     *   sub-function.  ScanTable[] is 0 based, parameter is 1 based to
     *   conform with IW_ENCODE_INDEX flag parameter passing in iwconfig/iwlist
     */
    if (tableIdx && (tableIdx <= Adapter->NumInScanTable))
        ret = wlan_associate(priv, &Adapter->ScanTable[tableIdx - 1]);
    else
        ret = -EINVAL;

#ifdef REASSOCIATION
    OS_REL_SEMAPHORE(&Adapter->ReassocSem);
#endif
    LEAVE();

    return ret;
}

/**
 *  @brief Start an Adhoc Network
 *
 *  @param priv         A pointer to wlan_private structure
 *  @param AdhocSSID    The ssid of the Adhoc Network
 *  @return             WLAN_STATUS_SUCCESS--success, WLAN_STATUS_FAILURE--fail
 */
int
wlan_start_adhoc(wlan_private * priv, WLAN_802_11_SSID * AdhocSSID)
{
    wlan_adapter *Adapter = priv->adapter;
    int ret = WLAN_STATUS_SUCCESS;

    ENTER();

    if ((Adapter->adhoc_start_band == BAND_A) && (Adapter->AdhocAutoSel)) {
        Adapter->AdhocChannel = wlan_11h_get_adhoc_start_channel(priv);

        /* 
         * Check if the region and channel requires a channel availability
         *   check.
         */
        if (wlan_11h_radar_detect_required(priv, Adapter->AdhocChannel)) {
            /* 
             * Radar detection is required for this channel, make sure
             * 11h is activated in the firmware
             */
            wlan_11h_activate(priv, TRUE);

            /* Check for radar on the channel */
            if (wlan_11h_radar_detected(priv, Adapter->AdhocChannel)) {
                LEAVE();
                return WLAN_STATUS_FAILURE;
            }
        }
    }

    PRINTM(INFO, "Adhoc Channel = %d\n", priv->adapter->AdhocChannel);
    PRINTM(INFO, "CurBssParams.channel = %d\n",
           priv->adapter->CurBssParams.BSSDescriptor.Channel);
    PRINTM(INFO, "CurBssParams.band = %d\n", priv->adapter->CurBssParams.band);

    ret = wlan_prepare_cmd(priv, HostCmd_CMD_802_11_AD_HOC_START,
                           0, HostCmd_OPTION_WAITFORRSP, 0, AdhocSSID);

    LEAVE();
    return ret;
}

/**
 *  @brief Join an adhoc network found in a previous scan
 *
 *  @param priv         A pointer to wlan_private structure
 *  @param pBSSDesc     Pointer to a BSS descriptor found in a previous scan
 *                      to attempt to join
 *
 *  @return             WLAN_STATUS_SUCCESS--success, WLAN_STATUS_FAILURE--fail
 */
int
wlan_join_adhoc(wlan_private * priv, BSSDescriptor_t * pBSSDesc)
{
    wlan_adapter *Adapter = priv->adapter;
    int ret = WLAN_STATUS_SUCCESS;

    ENTER();

    PRINTM(INFO, "wlan_join_adhoc: CurBss.ssid =%s\n",
           Adapter->CurBssParams.BSSDescriptor.Ssid.Ssid);
    PRINTM(INFO, "wlan_join_adhoc: CurBss.ssid_len =%u\n",
           Adapter->CurBssParams.BSSDescriptor.Ssid.SsidLength);
    PRINTM(INFO, "wlan_join_adhoc: ssid =%s\n", pBSSDesc->Ssid.Ssid);
    PRINTM(INFO, "wlan_join_adhoc: ssid len =%u\n", pBSSDesc->Ssid.SsidLength);

    /* check if the requested SSID is already joined */
    if (Adapter->CurBssParams.BSSDescriptor.Ssid.SsidLength
        && !wlan_ssid_cmp(&pBSSDesc->Ssid,
                          &Adapter->CurBssParams.BSSDescriptor.Ssid)
        && (Adapter->CurBssParams.BSSDescriptor.InfrastructureMode ==
            Wlan802_11IBSS)) {

        PRINTM(INFO,
               "ADHOC_J_CMD: New ad-hoc SSID is the same as current, "
               "not attempting to re-join");

        LEAVE();
        return WLAN_STATUS_FAILURE;
    }

    PRINTM(INFO, "CurBssParams.channel = %d\n",
           Adapter->CurBssParams.BSSDescriptor.Channel);
    PRINTM(INFO, "CurBssParams.band = %c\n", Adapter->CurBssParams.band);

    ret = wlan_prepare_cmd(priv, HostCmd_CMD_802_11_AD_HOC_JOIN,
                           0, HostCmd_OPTION_WAITFORRSP, 0, pBSSDesc);

    LEAVE();
    return ret;
}

/**
 *  @brief Send Deauthentication Request or Stop the AdHoc network depending on mode
 *
 *  @param priv      A pointer to wlan_private structure
 *  @return          WLAN_STATUS_SUCCESS--success, WLAN_STATUS_FAILURE--fail
 */
int
wlan_disconnect(wlan_private * priv)
{
    wlan_adapter *Adapter = priv->adapter;
    struct sockaddr saddr;
    int ret = WLAN_STATUS_SUCCESS;
    ENTER();

    if (Adapter->MediaConnectStatus == WlanMediaStateConnected) {

        if (Adapter->InfrastructureMode == Wlan802_11Infrastructure) {
            memcpy((void *) saddr.sa_data,
                   (void *) &Adapter->CurBssParams.BSSDescriptor.MacAddress,
                   ETH_ALEN);
            ret = wlan_prepare_cmd(priv, HostCmd_CMD_802_11_DEAUTHENTICATE,
                                   0,
                                   HostCmd_OPTION_WAITFORRSP |
                                   HostCmd_OPTION_TIMEOUT, 0, (void *) &saddr);

        } else if (Adapter->InfrastructureMode == Wlan802_11IBSS) {
            ret = wlan_prepare_cmd(priv, HostCmd_CMD_802_11_AD_HOC_STOP,
                                   0,
                                   HostCmd_OPTION_WAITFORRSP |
                                   HostCmd_OPTION_TIMEOUT, 0, NULL);
        }
    }
    LEAVE();
    return ret;
}

/**
 *  @brief Set Idle Off
 *
 *  @param priv         A pointer to wlan_private structure
 *  @return             WLAN_STATUS_SUCCESS --success, otherwise fail
 */
int
wlanidle_off(wlan_private * priv)
{
    wlan_adapter *Adapter = priv->adapter;
    int ret = WLAN_STATUS_SUCCESS;
    const u8 zeroMac[] = { 0, 0, 0, 0, 0, 0 };
    int i;

    ENTER();

    if (Adapter->MediaConnectStatus == WlanMediaStateDisconnected) {
        if (Adapter->InfrastructureMode == Wlan802_11Infrastructure) {
            if (memcmp(Adapter->PreviousBSSID, zeroMac, sizeof(zeroMac)) != 0) {

                PRINTM(INFO, "Previous SSID = %s\n",
                       Adapter->PreviousSSID.Ssid);
                PRINTM(INFO, "Previous BSSID = "
                       "%02x:%02x:%02x:%02x:%02x:%02x:\n",
                       Adapter->PreviousBSSID[0], Adapter->PreviousBSSID[1],
                       Adapter->PreviousBSSID[2], Adapter->PreviousBSSID[3],
                       Adapter->PreviousBSSID[4], Adapter->PreviousBSSID[5]);

                i = wlan_find_ssid_in_list(Adapter,
                                           &Adapter->PreviousSSID,
                                           Adapter->PreviousBSSID,
                                           Adapter->InfrastructureMode);

                if (i < 0) {
                    wlan_cmd_specific_scan_bssid(priv, Adapter->PreviousBSSID);
                    i = wlan_find_ssid_in_list(Adapter,
                                               &Adapter->PreviousSSID,
                                               Adapter->PreviousBSSID,
                                               Adapter->InfrastructureMode);
                }

                if (i < 0)
                    /* If the BSSID could not be found, try just the SSID */
                    i = wlan_find_ssid_in_list(Adapter,
                                               &Adapter->PreviousSSID,
                                               NULL,
                                               Adapter->InfrastructureMode);

                if (i < 0) {
                    wlan_cmd_specific_scan_ssid(priv, &Adapter->PreviousSSID);
                    i = wlan_find_ssid_in_list(Adapter,
                                               &Adapter->PreviousSSID,
                                               NULL,
                                               Adapter->InfrastructureMode);
                }

                if (i >= 0)
                    ret = wlan_associate(priv, &Adapter->ScanTable[i]);
            }
        } else if (Adapter->InfrastructureMode == Wlan802_11IBSS) {
            ret = wlan_prepare_cmd(priv,
                                   HostCmd_CMD_802_11_AD_HOC_START,
                                   0, HostCmd_OPTION_WAITFORRSP,
                                   0, &Adapter->PreviousSSID);
        }
    }
    /* else it is connected */

    PRINTM(INFO, "\nwlanidle is off");
    LEAVE();
    return ret;
}

/**
 *  @brief Set Idle On
 *
 *  @param priv         A pointer to wlan_private structure
 *  @return             WLAN_STATUS_SUCCESS --success, otherwise fail
 */
int
wlanidle_on(wlan_private * priv)
{
    wlan_adapter *Adapter = priv->adapter;
    int ret = WLAN_STATUS_SUCCESS;

    ENTER();

    if (Adapter->MediaConnectStatus == WlanMediaStateConnected) {
        if (Adapter->InfrastructureMode == Wlan802_11Infrastructure) {
            PRINTM(INFO, "Previous SSID = %s\n", Adapter->PreviousSSID.Ssid);
            memcpy(&Adapter->PreviousSSID,
                   &Adapter->CurBssParams.BSSDescriptor.Ssid,
                   sizeof(WLAN_802_11_SSID));
            wlan_disconnect(priv);

        } else if (Adapter->InfrastructureMode == Wlan802_11IBSS)
            ret = wlan_disconnect(priv);

    }
#ifdef REASSOCIATION
    if (Adapter->ReassocTimerIsSet == TRUE) {
        wlan_cancel_timer(&Adapter->MrvDrvTimer);
        Adapter->ReassocTimerIsSet = FALSE;
    }
#endif /* REASSOCIATION */

    PRINTM(INFO, "\nwlanidle is on");

    LEAVE();
    return ret;
}

/**
 *  @brief Append a generic IE as a pass through TLV to a TLV buffer.
 *
 *  This function is called from the network join command prep. routine. 
 *    If the IE buffer has been setup by the application, this routine appends
 *    the buffer as a pass through TLV type to the request.
 *
 *  @param priv     A pointer to wlan_private structure
 *  @param ppBuffer pointer to command buffer pointer
 *
 *  @return         bytes added to the buffer
 */
static int
wlan_cmd_append_generic_ie(wlan_private * priv, u8 ** ppBuffer)
{
    wlan_adapter *Adapter = priv->adapter;
    int retLen = 0;
    MrvlIEtypesHeader_t ieHeader;

    /* Null Checks */
    if (ppBuffer == 0)
        return 0;
    if (*ppBuffer == 0)
        return 0;

    /* 
     * If there is a generic ie buffer setup, append it to the return
     *   parameter buffer pointer.
     */
    if (Adapter->genIeBufferLen) {
        PRINTM(INFO, "append generic %d to %p\n", Adapter->genIeBufferLen,
               *ppBuffer);

        /* Wrap the generic IE buffer with a pass through TLV type */
        ieHeader.Type = wlan_cpu_to_le16(TLV_TYPE_PASSTHROUGH);
        ieHeader.Len = wlan_cpu_to_le16(Adapter->genIeBufferLen);
        memcpy(*ppBuffer, &ieHeader, sizeof(ieHeader));

        /* Increment the return size and the return buffer pointer param */
        *ppBuffer += sizeof(ieHeader);
        retLen += sizeof(ieHeader);

        /* Copy the generic IE buffer to the output buffer, advance pointer */
        memcpy(*ppBuffer, Adapter->genIeBuffer, Adapter->genIeBufferLen);

        /* Increment the return size and the return buffer pointer param */
        *ppBuffer += Adapter->genIeBufferLen;
        retLen += Adapter->genIeBufferLen;

        /* Reset the generic IE buffer */
        Adapter->genIeBufferLen = 0;
    }

    /* return the length appended to the buffer */
    return retLen;
}

/**
 *  @brief Append any application provided Marvell TLVs to a TLV buffer.
 *
 *  This function is called from the network join command prep. routine. 
 *    If the Marvell TLV buffer has been setup by the application, this routine
 *    appends the buffer to the request.
 *
 *  @param priv     A pointer to wlan_private structure
 *  @param ppBuffer pointer to command buffer pointer
 *
 *  @return         bytes added to the buffer
 */
static int
wlan_cmd_append_marvell_tlv(wlan_private * priv, u8 ** ppBuffer)
{
    wlan_adapter *Adapter = priv->adapter;
    int retLen = 0;

    /* Null Checks */
    if (ppBuffer == 0)
        return 0;
    if (*ppBuffer == 0)
        return 0;

    /* 
     * If there is a Marvell TLV buffer setup, append it to the return
     *   parameter buffer pointer.
     */
    if (Adapter->mrvlAssocTlvBufferLen) {
        PRINTM(INFO, "append tlv %d to %p\n",
               Adapter->mrvlAssocTlvBufferLen, *ppBuffer);

        /* Copy the TLV buffer to the output buffer, advance pointer */
        memcpy(*ppBuffer,
               Adapter->mrvlAssocTlvBuffer, Adapter->mrvlAssocTlvBufferLen);

        /* Increment the return size and the return buffer pointer param */
        *ppBuffer += Adapter->mrvlAssocTlvBufferLen;
        retLen += Adapter->mrvlAssocTlvBufferLen;

        /* Reset the Marvell TLV buffer */
        Adapter->mrvlAssocTlvBufferLen = 0;
    }

    /* return the length appended to the buffer */
    return retLen;
}

/**
 *  @brief Append TSF tracking info from the scan table for the target AP
 *
 *  This function is called from the network join command prep. routine. 
 *    The TSF table TSF sent to the firmware contains two TSF values:
 *      - the TSF of the target AP from its previous beacon/probe response
 *      - the TSF timestamp of our local MAC at the time we observed the
 *        beacon/probe response.
 *
 *    The firmware uses the timestamp values to set an initial TSF value
 *      in the MAC for the new association after a reassociation attempt.
 *
 *  @param priv     A pointer to wlan_private structure
 *  @param ppBuffer A pointer to command buffer pointer
 *  @param pBSSDesc A pointer to the BSS Descriptor from the scan table of
 *                  the AP we are trying to join
 *
 *  @return         bytes added to the buffer
 */
static int
wlan_cmd_append_tsf_tlv(wlan_private * priv, u8 ** ppBuffer,
                        BSSDescriptor_t * pBSSDesc)
{
    MrvlIEtypes_TsfTimestamp_t tsfTlv;
    u64 tsfVal;

    /* Null Checks */
    if (ppBuffer == 0)
        return 0;
    if (*ppBuffer == 0)
        return 0;

    tsfTlv.Header.Type = wlan_cpu_to_le16(TLV_TYPE_TSFTIMESTAMP);
    tsfTlv.Header.Len = wlan_cpu_to_le16(2 * sizeof(tsfVal));

    memcpy(*ppBuffer, &tsfTlv, sizeof(tsfTlv.Header));
    *ppBuffer += sizeof(tsfTlv.Header);

    /* TSF timestamp from the firmware TSF when the bcn/prb rsp was received */
    tsfVal = wlan_cpu_to_le64(pBSSDesc->networkTSF);
    memcpy(*ppBuffer, &tsfVal, sizeof(tsfVal));
    *ppBuffer += sizeof(tsfVal);

    memcpy(&tsfVal, pBSSDesc->TimeStamp, sizeof(tsfVal));

    PRINTM(INFO, "ASSOC: TSF offset calc: %016llx - %016llx\n",
           tsfVal, pBSSDesc->networkTSF);

    tsfVal = wlan_cpu_to_le64(tsfVal);
    memcpy(*ppBuffer, &tsfVal, sizeof(tsfVal));
    *ppBuffer += sizeof(tsfVal);

    return (sizeof(tsfTlv.Header) + (2 * sizeof(tsfVal)));
}

/**
 *  @brief This function prepares command of deauthenticate.
 *
 *  @param priv     A pointer to wlan_private structure
 *  @param cmd      A pointer to HostCmd_DS_COMMAND structure
 *  @param pdata_buf Void cast of MAC Address to deauth
 *  @return         WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
int
wlan_cmd_802_11_deauthenticate(wlan_private * priv,
                               HostCmd_DS_COMMAND * cmd, void *pdata_buf)
{
    HostCmd_DS_802_11_DEAUTHENTICATE *pDeauth = &cmd->params.deauth;
    struct sockaddr *p_saddr = (struct sockaddr *) pdata_buf;

    ENTER();

    cmd->Command = wlan_cpu_to_le16(HostCmd_CMD_802_11_DEAUTHENTICATE);
    cmd->Size =
        wlan_cpu_to_le16(sizeof(HostCmd_DS_802_11_DEAUTHENTICATE) + S_DS_GEN);

    /* set AP MAC address */
    memcpy(pDeauth->MacAddr, p_saddr->sa_data, ETH_ALEN);

    /* Reason code 3 = Station is leaving */
#define REASON_CODE_STA_LEAVING 3
    pDeauth->ReasonCode = wlan_cpu_to_le16(REASON_CODE_STA_LEAVING);

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/**
 *  @brief This function prepares command of association.
 *
 *  @param priv      A pointer to wlan_private structure
 *  @param cmd       A pointer to HostCmd_DS_COMMAND structure
 *  @param pdata_buf Void cast of BSSDescriptor_t from the scan table to assoc
 *  @return          WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
int
wlan_cmd_802_11_associate(wlan_private * priv,
                          HostCmd_DS_COMMAND * cmd, void *pdata_buf)
{
    wlan_adapter *Adapter = priv->adapter;
    HostCmd_DS_802_11_ASSOCIATE *pAsso = &cmd->params.associate;
    int ret = WLAN_STATUS_SUCCESS;
    BSSDescriptor_t *pBSSDesc;
    WLAN_802_11_RATES rates;
    int ratesSize;
    u8 *pos;
    u16 TmpCap;
    MrvlIEtypes_SsIdParamSet_t *pSsidTlv;
    MrvlIEtypes_PhyParamSet_t *pPhyTlv;
    MrvlIEtypes_SsParamSet_t *pSsTlv;
    MrvlIEtypes_RatesParamSet_t *pRatesTlv;
    MrvlIEtypes_AuthType_t *pAuthTlv;
    MrvlIEtypes_RsnParamSet_t *pRsnTlv;
    MrvlIEtypes_ChanListParamSet_t *pChanTlv;

    ENTER();

    pBSSDesc = (BSSDescriptor_t *) pdata_buf;
    pos = (u8 *) pAsso;

    cmd->Command = wlan_cpu_to_le16(HostCmd_CMD_802_11_ASSOCIATE);

    /* Save so we know which BSS Desc to use in the response handler */
    Adapter->pAttemptedBSSDesc = pBSSDesc;

    memcpy(pAsso->PeerStaAddr,
           pBSSDesc->MacAddress, sizeof(pAsso->PeerStaAddr));
    pos += sizeof(pAsso->PeerStaAddr);

    /* set the listen interval */
    pAsso->ListenInterval = wlan_cpu_to_le16(Adapter->ListenInterval);

    pos += sizeof(pAsso->CapInfo);
    pos += sizeof(pAsso->ListenInterval);
    pos += sizeof(pAsso->Reserved1);

    pSsidTlv = (MrvlIEtypes_SsIdParamSet_t *) pos;
    pSsidTlv->Header.Type = wlan_cpu_to_le16(TLV_TYPE_SSID);
    pSsidTlv->Header.Len = pBSSDesc->Ssid.SsidLength;
    memcpy(pSsidTlv->SsId, pBSSDesc->Ssid.Ssid, pSsidTlv->Header.Len);
    pos += sizeof(pSsidTlv->Header) + pSsidTlv->Header.Len;
    pSsidTlv->Header.Len = wlan_cpu_to_le16(pSsidTlv->Header.Len);

    pPhyTlv = (MrvlIEtypes_PhyParamSet_t *) pos;
    pPhyTlv->Header.Type = wlan_cpu_to_le16(TLV_TYPE_PHY_DS);
    pPhyTlv->Header.Len = sizeof(pPhyTlv->fh_ds.DsParamSet);
    memcpy(&pPhyTlv->fh_ds.DsParamSet,
           &pBSSDesc->PhyParamSet.DsParamSet.CurrentChan,
           sizeof(pPhyTlv->fh_ds.DsParamSet));
    pos += sizeof(pPhyTlv->Header) + pPhyTlv->Header.Len;
    pPhyTlv->Header.Len = wlan_cpu_to_le16(pPhyTlv->Header.Len);

    pSsTlv = (MrvlIEtypes_SsParamSet_t *) pos;
    pSsTlv->Header.Type = wlan_cpu_to_le16(TLV_TYPE_CF);
    pSsTlv->Header.Len = sizeof(pSsTlv->cf_ibss.CfParamSet);
    pos += sizeof(pSsTlv->Header) + pSsTlv->Header.Len;
    pSsTlv->Header.Len = wlan_cpu_to_le16(pSsTlv->Header.Len);

    /* Get the common rates supported between the driver and the BSS Desc */
    if (setup_rates_from_bssdesc(Adapter, pBSSDesc, rates, &ratesSize)) {
        ret = WLAN_STATUS_FAILURE;
        goto done;
    }

    /* Setup the Rates TLV in the association command */
    pRatesTlv = (MrvlIEtypes_RatesParamSet_t *) pos;
    pRatesTlv->Header.Type = wlan_cpu_to_le16(TLV_TYPE_RATES);
    pRatesTlv->Header.Len = wlan_cpu_to_le16(ratesSize);
    memcpy(pRatesTlv->Rates, rates, ratesSize);
    pos += sizeof(pRatesTlv->Header) + ratesSize;
    PRINTM(INFO, "ASSOC_CMD: Rates size = %d\n", ratesSize);

    /* Add the Authentication type to be used for Auth frames if needed */
    pAuthTlv = (MrvlIEtypes_AuthType_t *) pos;
    pAuthTlv->Header.Type = wlan_cpu_to_le16(TLV_TYPE_AUTH_TYPE);
    pAuthTlv->Header.Len = sizeof(pAuthTlv->AuthType);
    pAuthTlv->AuthType = wlan_cpu_to_le16(Adapter->SecInfo.AuthenticationMode);
    pos += sizeof(pAuthTlv->Header) + pAuthTlv->Header.Len;
    pAuthTlv->Header.Len = wlan_cpu_to_le16(pAuthTlv->Header.Len);

    /* Append a channel TLV for the channel the attempted AP was found on */
    pChanTlv = (MrvlIEtypes_ChanListParamSet_t *) pos;
    pChanTlv->Header.Type = wlan_cpu_to_le16(TLV_TYPE_CHANLIST);
    pChanTlv->Header.Len = wlan_cpu_to_le16(sizeof(ChanScanParamSet_t));

    memset(pChanTlv->ChanScanParam, 0x00, sizeof(ChanScanParamSet_t));
    pChanTlv->ChanScanParam[0].ChanNumber =
        (pBSSDesc->PhyParamSet.DsParamSet.CurrentChan);
    PRINTM(INFO, "Assoc: TLV Chan = %d\n",
           pChanTlv->ChanScanParam[0].ChanNumber);

    pChanTlv->ChanScanParam[0].RadioType
        = band_to_radio_type(pBSSDesc->bss_band);

    PRINTM(INFO, "Assoc: TLV Band = %d\n",
           pChanTlv->ChanScanParam[0].RadioType);
    pos += sizeof(pChanTlv->Header) + sizeof(ChanScanParamSet_t);

    if (!Adapter->wps.SessionEnable) {
        if ((Adapter->SecInfo.WPAEnabled || Adapter->SecInfo.WPA2Enabled)) {
            pRsnTlv = (MrvlIEtypes_RsnParamSet_t *) pos;
            pRsnTlv->Header.Type = (u16) Adapter->Wpa_ie[0];    /* WPA_IE or
                                                                   WPA2_IE */
            pRsnTlv->Header.Type = pRsnTlv->Header.Type & 0x00FF;
            pRsnTlv->Header.Type = wlan_cpu_to_le16(pRsnTlv->Header.Type);
            pRsnTlv->Header.Len = (u16) Adapter->Wpa_ie[1];
            pRsnTlv->Header.Len = pRsnTlv->Header.Len & 0x00FF;
            if (pRsnTlv->Header.Len <= (sizeof(Adapter->Wpa_ie) - 2))
                memcpy(pRsnTlv->RsnIE, &Adapter->Wpa_ie[2],
                       pRsnTlv->Header.Len);
            else {
                ret = WLAN_STATUS_FAILURE;
                goto done;
            }
            HEXDUMP("ASSOC_CMD: RSN IE", (u8 *) pRsnTlv,
                    sizeof(pRsnTlv->Header) + pRsnTlv->Header.Len);
            pos += sizeof(pRsnTlv->Header) + pRsnTlv->Header.Len;
            pRsnTlv->Header.Len = wlan_cpu_to_le16(pRsnTlv->Header.Len);
        }
    }

    wlan_wmm_process_association_req(priv, &pos, &pBSSDesc->wmmIE);

    wlan_cmd_append_generic_ie(priv, &pos);

    wlan_cmd_append_marvell_tlv(priv, &pos);

    wlan_cmd_append_tsf_tlv(priv, &pos, pBSSDesc);

    if (wlan_parse_dnld_countryinfo_11d(priv)) {
        ret = WLAN_STATUS_FAILURE;
        goto done;
    }

    /* 
     * Call 11h join API after capability bits are set so adhoc/infra 11h
     * behavior can be properly triggered.  pos modified if data is appended
     */
    wlan_11h_process_join(priv, &pos, &pAsso->CapInfo,
                          pBSSDesc->PhyParamSet.DsParamSet.CurrentChan,
                          &pBSSDesc->wlan_11h_bss_info);

    cmd->Size = wlan_cpu_to_le16((u16) (pos - (u8 *) pAsso) + S_DS_GEN);

    /* set the Capability info at last */
    memcpy(&TmpCap, &pBSSDesc->Cap, sizeof(pAsso->CapInfo));
    TmpCap &= CAPINFO_MASK;
    PRINTM(INFO, "ASSOC_CMD: TmpCap=%4X CAPINFO_MASK=%4lX\n",
           TmpCap, CAPINFO_MASK);
    TmpCap = wlan_cpu_to_le16(TmpCap);
    memcpy(&pAsso->CapInfo, &TmpCap, sizeof(pAsso->CapInfo));

  done:
    LEAVE();
    return ret;
}

/**
 *  @brief This function prepares command of ad_hoc_start.
 *
 *  @param priv     A pointer to wlan_private structure
 *  @param cmd      A pointer to HostCmd_DS_COMMAND structure
 *  @param pssid    A pointer to WLAN_802_11_SSID structure
 *  @return         WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
int
wlan_cmd_802_11_ad_hoc_start(wlan_private * priv,
                             HostCmd_DS_COMMAND * cmd, void *pssid)
{
    wlan_adapter *Adapter = priv->adapter;
    HostCmd_DS_802_11_AD_HOC_START *adhs = &cmd->params.ads;
    int ret = WLAN_STATUS_SUCCESS;
    int cmdAppendSize = 0;
    int i;
    u16 TmpCap;
    BSSDescriptor_t *pBSSDesc;
    MrvlIEtypes_ChanListParamSet_t *pChanTlv;
    MrvlIEtypes_RsnParamSet_t *pRsnTlv;
    u8 *pos;
    pos = (u8 *) adhs + sizeof(HostCmd_DS_802_11_AD_HOC_START);

    ENTER();

    if (!Adapter) {
        ret = WLAN_STATUS_FAILURE;
        goto done;
    }

    cmd->Command = wlan_cpu_to_le16(HostCmd_CMD_802_11_AD_HOC_START);

    pBSSDesc = &Adapter->CurBssParams.BSSDescriptor;
    Adapter->pAttemptedBSSDesc = pBSSDesc;

    /* 
     * Fill in the parameters for 2 data structures:
     *   1. HostCmd_DS_802_11_AD_HOC_START Command
     *   2. pBSSDesc
     * Driver will fill up SSID, BSSType,IBSS param, Physical Param,
     * probe delay, and Cap info.
     * Firmware will fill up beacon period, Basic rates
     * and operational rates.
     */

    memset(adhs->SSID, 0, MRVDRV_MAX_SSID_LENGTH);

    memcpy(adhs->SSID, ((WLAN_802_11_SSID *) pssid)->Ssid,
           ((WLAN_802_11_SSID *) pssid)->SsidLength);

    PRINTM(INFO, "ADHOC_S_CMD: SSID = %s\n", adhs->SSID);

    memset(pBSSDesc->Ssid.Ssid, 0, MRVDRV_MAX_SSID_LENGTH);
    memcpy(pBSSDesc->Ssid.Ssid,
           ((WLAN_802_11_SSID *) pssid)->Ssid,
           ((WLAN_802_11_SSID *) pssid)->SsidLength);

    pBSSDesc->Ssid.SsidLength = ((WLAN_802_11_SSID *) pssid)->SsidLength;

    /* set the BSS type */
    adhs->BSSType = HostCmd_BSS_TYPE_IBSS;
    pBSSDesc->InfrastructureMode = Wlan802_11IBSS;
    adhs->BeaconPeriod = wlan_cpu_to_le16(Adapter->BeaconPeriod);
    pBSSDesc->BeaconPeriod = Adapter->BeaconPeriod;

    /* set Physical param set */
#define DS_PARA_IE_ID   3
#define DS_PARA_IE_LEN  1

    adhs->PhyParamSet.DsParamSet.ElementId = DS_PARA_IE_ID;
    adhs->PhyParamSet.DsParamSet.Len = DS_PARA_IE_LEN;

    if (!get_cfp_by_band_and_channel
        (Adapter->adhoc_start_band, (u16) Adapter->AdhocChannel,
         Adapter->region_channel)) {
        CHANNEL_FREQ_POWER *cfp;
        cfp = get_cfp_by_band_and_channel(Adapter->adhoc_start_band,
                                          FIRST_VALID_CHANNEL,
                                          Adapter->region_channel);
        if (cfp)
            Adapter->AdhocChannel = cfp->Channel;
    }

    ASSERT(Adapter->AdhocChannel);

    PRINTM(INFO, "ADHOC_S_CMD: Creating ADHOC on Channel %d\n",
           Adapter->AdhocChannel);

    Adapter->CurBssParams.BSSDescriptor.Channel = Adapter->AdhocChannel;
    Adapter->CurBssParams.band = Adapter->adhoc_start_band;

    pBSSDesc->Channel = Adapter->AdhocChannel;
    adhs->PhyParamSet.DsParamSet.CurrentChan = Adapter->AdhocChannel;

    memcpy(&pBSSDesc->PhyParamSet,
           &adhs->PhyParamSet, sizeof(IEEEtypes_PhyParamSet_t));

    pBSSDesc->NetworkTypeInUse = Wlan802_11DS;

    /* set IBSS param set */
#define IBSS_PARA_IE_ID   6
#define IBSS_PARA_IE_LEN  2

    adhs->SsParamSet.IbssParamSet.ElementId = IBSS_PARA_IE_ID;
    adhs->SsParamSet.IbssParamSet.Len = IBSS_PARA_IE_LEN;
    adhs->SsParamSet.IbssParamSet.AtimWindow
        = wlan_cpu_to_le16(Adapter->AtimWindow);
    pBSSDesc->ATIMWindow = Adapter->AtimWindow;
    memcpy(&pBSSDesc->SsParamSet,
           &adhs->SsParamSet, sizeof(IEEEtypes_SsParamSet_t));

    /* set Capability info */
    adhs->Cap.Ess = 0;
    adhs->Cap.Ibss = 1;
    pBSSDesc->Cap.Ibss = 1;

    /* set up privacy in pBSSDesc */
    if (Adapter->SecInfo.WEPStatus == Wlan802_11WEPEnabled
        || Adapter->AdhocAESEnabled || Adapter->SecInfo.WPAEnabled) {

#define AD_HOC_CAP_PRIVACY_ON 1
        PRINTM(INFO, "ADHOC_S_CMD: WEPStatus set, Privacy to WEP\n");
        pBSSDesc->Privacy = Wlan802_11PrivFilter8021xWEP;
        adhs->Cap.Privacy = AD_HOC_CAP_PRIVACY_ON;
    } else {
        PRINTM(INFO, "ADHOC_S_CMD: WEPStatus NOT set, Setting "
               "Privacy to ACCEPT ALL\n");
        pBSSDesc->Privacy = Wlan802_11PrivFilterAcceptAll;
    }

    memset(adhs->DataRate, 0, sizeof(adhs->DataRate));

    if (Adapter->adhoc_start_band == BAND_A) {
        memcpy(adhs->DataRate, AdhocRates_A,
               MIN(sizeof(adhs->DataRate), sizeof(AdhocRates_A)));
    } else if (Adapter->adhoc_start_band == BAND_G ||
               Adapter->adhoc_grate_enabled == TRUE) {
        memcpy(adhs->DataRate, AdhocRates_G,
               MIN(sizeof(adhs->DataRate), sizeof(AdhocRates_G)));
        if (Adapter->
            CurrentPacketFilter & HostCmd_ACT_MAC_ADHOC_G_PROTECTION_ON) {
            ret =
                wlan_prepare_cmd(priv, HostCmd_CMD_MAC_CONTROL, 0,
                                 HostCmd_OPTION_WAITFORRSP, 0,
                                 &Adapter->CurrentPacketFilter);
            if (ret) {
                PRINTM(INFO, "ADHOC_S_CMD: G Protection config failed\n");
                ret = WLAN_STATUS_FAILURE;
                goto done;
            }
        }
    } else if (Adapter->adhoc_start_band == BAND_B)
        memcpy(adhs->DataRate, AdhocRates_B,
               MIN(sizeof(adhs->DataRate), sizeof(AdhocRates_B)));

    /* Find the last non zero */
    for (i = 0; i < sizeof(adhs->DataRate) && adhs->DataRate[i]; i++);

    Adapter->CurBssParams.NumOfRates = i;

    /* Copy the ad-hoc creating rates into Current BSS state structure */
    memcpy(&Adapter->CurBssParams.DataRates,
           &adhs->DataRate, Adapter->CurBssParams.NumOfRates);

    PRINTM(INFO, "ADHOC_S_CMD: Rates=%02x %02x %02x %02x \n",
           adhs->DataRate[0], adhs->DataRate[1],
           adhs->DataRate[2], adhs->DataRate[3]);

    PRINTM(INFO, "ADHOC_S_CMD: AD HOC Start command is ready\n");

    if (IS_SUPPORT_MULTI_BANDS(Adapter)) {
        /* Append a channel TLV */
        pChanTlv = (MrvlIEtypes_ChanListParamSet_t *) pos;
        pChanTlv->Header.Type = wlan_cpu_to_le16(TLV_TYPE_CHANLIST);
        pChanTlv->Header.Len = wlan_cpu_to_le16(sizeof(ChanScanParamSet_t));

        memset(pChanTlv->ChanScanParam, 0x00, sizeof(ChanScanParamSet_t));
        pChanTlv->ChanScanParam[0].ChanNumber =
            Adapter->CurBssParams.BSSDescriptor.Channel;

        PRINTM(INFO, "ADHOC_S_CMD: TLV Chan = %d\n",
               pChanTlv->ChanScanParam[0].ChanNumber);

        pChanTlv->ChanScanParam[0].RadioType
            = band_to_radio_type(Adapter->CurBssParams.band);

        PRINTM(INFO, "ADHOC_S_CMD: TLV Band = %d\n",
               pChanTlv->ChanScanParam[0].RadioType);
        pos += sizeof(pChanTlv->Header) + sizeof(ChanScanParamSet_t);
        cmdAppendSize += sizeof(pChanTlv->Header) + sizeof(ChanScanParamSet_t);
    }

    if (wlan_create_dnld_countryinfo_11d(priv)) {
        PRINTM(INFO, "ADHOC_S_CMD: dnld_countryinfo_11d failed\n");
        ret = WLAN_STATUS_FAILURE;
        goto done;
    }

    /* 
     * Call 11h start API to add any 11h flags/elements as TLV parameters
     */
    cmdAppendSize += wlan_11h_process_start(priv, &pos, &adhs->Cap,
                                            Adapter->AdhocChannel,
                                            &pBSSDesc->wlan_11h_bss_info);

    if (Adapter->SecInfo.WPAEnabled) {
        pRsnTlv = (MrvlIEtypes_RsnParamSet_t *) pos;
        pRsnTlv->Header.Type = (u16) Adapter->Wpa_ie[0];        /* WPA_IE or
                                                                   WPA2_IE */
        pRsnTlv->Header.Type = pRsnTlv->Header.Type & 0x00FF;
        pRsnTlv->Header.Type = wlan_cpu_to_le16(pRsnTlv->Header.Type);
        pRsnTlv->Header.Len = (u16) Adapter->Wpa_ie[1];
        pRsnTlv->Header.Len = pRsnTlv->Header.Len & 0x00FF;
        if (pRsnTlv->Header.Len <= (sizeof(Adapter->Wpa_ie) - 2))
            memcpy(pRsnTlv->RsnIE, &Adapter->Wpa_ie[2], pRsnTlv->Header.Len);
        else {
            ret = WLAN_STATUS_FAILURE;
            goto done;
        }

        HEXDUMP("ADHOC_JOIN: RSN IE", (u8 *) pRsnTlv,
                sizeof(pRsnTlv->Header) + pRsnTlv->Header.Len);
        pos += sizeof(pRsnTlv->Header) + pRsnTlv->Header.Len;
        cmdAppendSize += sizeof(pRsnTlv->Header) + pRsnTlv->Header.Len;
        pRsnTlv->Header.Len = wlan_cpu_to_le16(pRsnTlv->Header.Len);
    }

    cmd->Size = wlan_cpu_to_le16(sizeof(HostCmd_DS_802_11_AD_HOC_START)
                                 + S_DS_GEN + cmdAppendSize);

    memcpy(&TmpCap, &adhs->Cap, sizeof(u16));
    TmpCap = wlan_cpu_to_le16(TmpCap);
    memcpy(&adhs->Cap, &TmpCap, sizeof(u16));

    ret = WLAN_STATUS_SUCCESS;
  done:
    LEAVE();
    return ret;
}

/**
 *  @brief This function prepares command of ad_hoc_stop.
 *
 *  @param priv     A pointer to wlan_private structure
 *  @param cmd      A pointer to HostCmd_DS_COMMAND structure
 *  @return         WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
int
wlan_cmd_802_11_ad_hoc_stop(wlan_private * priv, HostCmd_DS_COMMAND * cmd)
{
    ENTER();

    cmd->Command = wlan_cpu_to_le16(HostCmd_CMD_802_11_AD_HOC_STOP);
    cmd->Size = wlan_cpu_to_le16(sizeof(HostCmd_DS_802_11_AD_HOC_STOP)
                                 + S_DS_GEN);

    if (wlan_11h_is_active(priv))
        wlan_11h_activate(priv, FALSE);

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/**
 *  @brief This function prepares command of ad_hoc_join.
 *
 *  @param priv      A pointer to wlan_private structure
 *  @param cmd       A pointer to HostCmd_DS_COMMAND structure
 *  @param pdata_buf Void cast of BSSDescriptor_t from the scan table to join
 *
 *  @return          WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
int
wlan_cmd_802_11_ad_hoc_join(wlan_private * priv,
                            HostCmd_DS_COMMAND * cmd, void *pdata_buf)
{
    wlan_adapter *Adapter = priv->adapter;
    HostCmd_DS_802_11_AD_HOC_JOIN *pAdHocJoin = &cmd->params.adj;
    BSSDescriptor_t *pBSSDesc = (BSSDescriptor_t *) pdata_buf;
    int cmdAppendSize = 0;
    int ret = WLAN_STATUS_SUCCESS;
    WLAN_802_11_RATES rates;
    int ratesSize;
    u16 TmpCap;
    u16 CurrentPacketFilter;
    MrvlIEtypes_ChanListParamSet_t *pChanTlv;
    MrvlIEtypes_RsnParamSet_t *pRsnTlv;
    u8 *pos;
    pos = (u8 *) pAdHocJoin + sizeof(HostCmd_DS_802_11_AD_HOC_JOIN);

    ENTER();

#define USE_G_PROTECTION	0x02
    if (pBSSDesc->ERPFlags & USE_G_PROTECTION) {
        CurrentPacketFilter =
            Adapter->
            CurrentPacketFilter | HostCmd_ACT_MAC_ADHOC_G_PROTECTION_ON;
        ret =
            wlan_prepare_cmd(priv, HostCmd_CMD_MAC_CONTROL, 0,
                             HostCmd_OPTION_WAITFORRSP, 0,
                             &CurrentPacketFilter);
        if (ret) {
            PRINTM(INFO, "ADHOC_S_CMD: G Protection config failed\n");
            ret = WLAN_STATUS_FAILURE;
            goto done;
        }
    }
    Adapter->pAttemptedBSSDesc = pBSSDesc;

    cmd->Command = wlan_cpu_to_le16(HostCmd_CMD_802_11_AD_HOC_JOIN);

    pAdHocJoin->BssDescriptor.BSSType = HostCmd_BSS_TYPE_IBSS;

    pAdHocJoin->BssDescriptor.BeaconPeriod
        = wlan_cpu_to_le16(pBSSDesc->BeaconPeriod);

    memcpy(&pAdHocJoin->BssDescriptor.BSSID, &pBSSDesc->MacAddress, ETH_ALEN);

    memcpy(&pAdHocJoin->BssDescriptor.SSID,
           &pBSSDesc->Ssid.Ssid, pBSSDesc->Ssid.SsidLength);

    memcpy(&pAdHocJoin->BssDescriptor.PhyParamSet,
           &pBSSDesc->PhyParamSet, sizeof(IEEEtypes_PhyParamSet_t));

    memcpy(&pAdHocJoin->BssDescriptor.SsParamSet,
           &pBSSDesc->SsParamSet, sizeof(IEEEtypes_SsParamSet_t));

    memcpy(&TmpCap, &pBSSDesc->Cap, sizeof(IEEEtypes_CapInfo_t));

    TmpCap &= CAPINFO_MASK;

    PRINTM(INFO, "ADHOC_J_CMD: TmpCap=%4X CAPINFO_MASK=%4lX\n",
           TmpCap, CAPINFO_MASK);
    memcpy(&pAdHocJoin->BssDescriptor.Cap, &TmpCap,
           sizeof(IEEEtypes_CapInfo_t));

    /* information on BSSID descriptor passed to FW */
    PRINTM(INFO,
           "ADHOC_J_CMD: BSSID = %02x-%02x-%02x-%02x-%02x-%02x, SSID = %s\n",
           pAdHocJoin->BssDescriptor.BSSID[0],
           pAdHocJoin->BssDescriptor.BSSID[1],
           pAdHocJoin->BssDescriptor.BSSID[2],
           pAdHocJoin->BssDescriptor.BSSID[3],
           pAdHocJoin->BssDescriptor.BSSID[4],
           pAdHocJoin->BssDescriptor.BSSID[5], pAdHocJoin->BssDescriptor.SSID);

    /* Get the common rates supported between the driver and the BSS Desc */
    if (setup_rates_from_bssdesc(Adapter, pBSSDesc, rates, &ratesSize)) {
        ret = WLAN_STATUS_FAILURE;
        goto done;
    }

    /* Copy Data Rates from the Rates recorded in scan response */
    memset(pAdHocJoin->BssDescriptor.DataRates, 0,
           sizeof(pAdHocJoin->BssDescriptor.DataRates));
    memcpy(pAdHocJoin->BssDescriptor.DataRates, rates, ratesSize);

    /* Copy the adhoc join rates into Current BSS state structure */
    Adapter->CurBssParams.NumOfRates = ratesSize;
    memcpy(&Adapter->CurBssParams.DataRates, rates, ratesSize);

    /* Copy the channel information */
    Adapter->CurBssParams.BSSDescriptor.Channel = pBSSDesc->Channel;
    Adapter->CurBssParams.band = pBSSDesc->bss_band;

    if (Adapter->SecInfo.WEPStatus == Wlan802_11WEPEnabled
        || Adapter->AdhocAESEnabled)
        pAdHocJoin->BssDescriptor.Cap.Privacy = AD_HOC_CAP_PRIVACY_ON;

    if (Adapter->PSMode == Wlan802_11PowerModeMAX_PSP) {
        /* wake up first */
        WLAN_802_11_POWER_MODE LocalPSMode;

        LocalPSMode = Wlan802_11PowerModeCAM;
        ret = wlan_prepare_cmd(priv,
                               HostCmd_CMD_802_11_PS_MODE,
                               HostCmd_ACT_GEN_SET, 0, 0, &LocalPSMode);

        if (ret) {
            ret = WLAN_STATUS_FAILURE;
            goto done;
        }
    }

    if (IS_SUPPORT_MULTI_BANDS(Adapter)) {
        /* Append a channel TLV */
        pChanTlv = (MrvlIEtypes_ChanListParamSet_t *) pos;
        pChanTlv->Header.Type = wlan_cpu_to_le16(TLV_TYPE_CHANLIST);
        pChanTlv->Header.Len = wlan_cpu_to_le16(sizeof(ChanScanParamSet_t));

        memset(pChanTlv->ChanScanParam, 0x00, sizeof(ChanScanParamSet_t));
        pChanTlv->ChanScanParam[0].ChanNumber =
            (pBSSDesc->PhyParamSet.DsParamSet.CurrentChan);
        PRINTM(INFO, "ADHOC_J_CMD: TLV Chan = %d\n",
               pChanTlv->ChanScanParam[0].ChanNumber);

        pChanTlv->ChanScanParam[0].RadioType
            = band_to_radio_type(pBSSDesc->bss_band);

        PRINTM(INFO, "ADHOC_J_CMD: TLV Band = %d\n",
               pChanTlv->ChanScanParam[0].RadioType);
        pos += sizeof(pChanTlv->Header) + sizeof(ChanScanParamSet_t);
        cmdAppendSize += sizeof(pChanTlv->Header) + sizeof(ChanScanParamSet_t);
    }

    if (wlan_parse_dnld_countryinfo_11d(priv)) {
        ret = WLAN_STATUS_FAILURE;
        goto done;
    }

    /* 
     * Call 11h join API after capability bits are set so
     *   adhoc/infra 11h behavior can be properly triggered.
     *   pos modified if data is appended
     */
    cmdAppendSize += wlan_11h_process_join(priv, &pos,
                                           &pAdHocJoin->BssDescriptor.Cap,
                                           pBSSDesc->Channel,
                                           &pBSSDesc->wlan_11h_bss_info);

    if (Adapter->SecInfo.WPAEnabled) {
        pRsnTlv = (MrvlIEtypes_RsnParamSet_t *) pos;
        pRsnTlv->Header.Type = (u16) Adapter->Wpa_ie[0];        /* WPA_IE or
                                                                   WPA2_IE */
        pRsnTlv->Header.Type = pRsnTlv->Header.Type & 0x00FF;
        pRsnTlv->Header.Type = wlan_cpu_to_le16(pRsnTlv->Header.Type);
        pRsnTlv->Header.Len = (u16) Adapter->Wpa_ie[1];
        pRsnTlv->Header.Len = pRsnTlv->Header.Len & 0x00FF;
        if (pRsnTlv->Header.Len <= (sizeof(Adapter->Wpa_ie) - 2))
            memcpy(pRsnTlv->RsnIE, &Adapter->Wpa_ie[2], pRsnTlv->Header.Len);
        else {
            ret = WLAN_STATUS_FAILURE;
            goto done;
        }

        HEXDUMP("ADHOC_JOIN: RSN IE", (u8 *) pRsnTlv,
                sizeof(pRsnTlv->Header) + pRsnTlv->Header.Len);
        pos += sizeof(pRsnTlv->Header) + pRsnTlv->Header.Len;
        cmdAppendSize += sizeof(pRsnTlv->Header) + pRsnTlv->Header.Len;
        pRsnTlv->Header.Len = wlan_cpu_to_le16(pRsnTlv->Header.Len);
    }

    cmd->Size = wlan_cpu_to_le16(sizeof(HostCmd_DS_802_11_AD_HOC_JOIN)
                                 + S_DS_GEN + cmdAppendSize);

    memcpy(&TmpCap, &pAdHocJoin->BssDescriptor.Cap,
           sizeof(IEEEtypes_CapInfo_t));
    TmpCap = wlan_cpu_to_le16(TmpCap);

    memcpy(&pAdHocJoin->BssDescriptor.Cap,
           &TmpCap, sizeof(IEEEtypes_CapInfo_t));

  done:
    LEAVE();
    return ret;
}

/**
 *  @brief Association firmware command response handler
 *
 *   The response buffer for the association command has the following
 *      memory layout.
 *
 *   For cases where an association response was not received (indicated
 *      by the CapInfo and AId field):
 *
 *     .------------------------------------------------------------.
 *     |  Header(4 * sizeof(u16)):  Standard command response hdr   |
 *     .------------------------------------------------------------.
 *     |  CapInfo/Error Return(u16):                                |
 *     |           0xFFFF(-1): Internal error                       |
 *     |           0xFFFE(-2): Authentication unhandled message     |
 *     |           0xFFFD(-3): Authentication refused               |
 *     |           0xFFFC(-4): Timeout waiting for AP response      |
 *     .------------------------------------------------------------.
 *     |  StatusCode(u16):                                          |
 *     |        If CapInfo is -1:                                   |
 *     |           An internal firmware failure prevented the       |
 *     |           command from being processed.  The StatusCode    |
 *     |           will be set to 1.                                |
 *     |                                                            |
 *     |        If CapInfo is -2:                                   |
 *     |           An authentication frame was received but was     |
 *     |           not handled by the firmware.  IEEE Status        |
 *     |           code for the failure is returned.                |
 *     |                                                            |
 *     |        If CapInfo is -3:                                   |
 *     |           An authentication frame was received and the     |
 *     |           StatusCode is the IEEE Status reported in the    |
 *     |           response.                                        |
 *     |                                                            |
 *     |        If CapInfo is -4:                                   |
 *     |           (1) Association response timeout                 |
 *     |           (2) Authentication response timeout              |
 *     .------------------------------------------------------------.
 *     |  AId(u16): 0xFFFF                                          |
 *     .------------------------------------------------------------.
 *
 *
 *   For cases where an association response was received, the IEEE 
 *     standard association response frame is returned:
 *
 *     .------------------------------------------------------------.
 *     |  Header(4 * sizeof(u16)):  Standard command response hdr   |
 *     .------------------------------------------------------------.
 *     |  CapInfo(u16): IEEE Capability                             |
 *     .------------------------------------------------------------.
 *     |  StatusCode(u16): IEEE Status Code                         |
 *     .------------------------------------------------------------.
 *     |  AId(u16): IEEE Association ID                             |
 *     .------------------------------------------------------------.
 *     |  IEEE IEs(variable): Any received IEs comprising the       |
 *     |                      remaining portion of a received       |
 *     |                      association response frame.           |
 *     .------------------------------------------------------------.
 *
 *  For simplistic handling, the StatusCode field can be used to determine
 *    an association success (0) or failure (non-zero).
 *
 *  @param priv    A pointer to wlan_private structure
 *  @param resp    A pointer to HostCmd_DS_COMMAND
 *  @return        WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
int
wlan_ret_802_11_associate(wlan_private * priv, HostCmd_DS_COMMAND * resp)
{
    wlan_adapter *Adapter = priv->adapter;
    int ret = WLAN_STATUS_SUCCESS;
    IEEEtypes_AssocRsp_t *pAssocRsp;
    BSSDescriptor_t *pBSSDesc;
    WLAN_802_11_RATES rates;
    int ratesSize;

    ENTER();

    pAssocRsp = (IEEEtypes_AssocRsp_t *) & resp->params;

    HEXDUMP("ASSOC_RESP:", (void *) &resp->params,
            wlan_le16_to_cpu(resp->Size) - S_DS_GEN);

    Adapter->assocRspSize = MIN(wlan_le16_to_cpu(resp->Size) - S_DS_GEN,
                                sizeof(Adapter->assocRspBuffer));

    memcpy(Adapter->assocRspBuffer, &resp->params, Adapter->assocRspSize);

    if (pAssocRsp->StatusCode) {
        priv->adapter->dbg.num_cmd_assoc_failure++;
        PRINTM(CMND, "ASSOC_RESP: Association Failed, "
               "status code = %d, error = %d\n",
               pAssocRsp->StatusCode, *(short *) &pAssocRsp->Capability);

        ret = WLAN_STATUS_FAILURE;

        goto done;
    }

    /* Send a Media Connected event, according to the Spec */
    Adapter->MediaConnectStatus = WlanMediaStateConnected;

    /* Set the attempted BSSID Index to current */
    pBSSDesc = Adapter->pAttemptedBSSDesc;

    PRINTM(INFO, "ASSOC_RESP: %s\n", pBSSDesc->Ssid.Ssid);

    /* Make a copy of current BSSID descriptor */
    memcpy(&Adapter->CurBssParams.BSSDescriptor,
           pBSSDesc, sizeof(BSSDescriptor_t));

    /* update CurBssParams */
    Adapter->CurBssParams.BSSDescriptor.Channel
        = pBSSDesc->PhyParamSet.DsParamSet.CurrentChan;
    Adapter->CurBssParams.band = pBSSDesc->bss_band;

    if (setup_rates_from_bssdesc(Adapter, pBSSDesc, rates, &ratesSize)) {
        ret = WLAN_STATUS_FAILURE;
        goto done;
    }

    /* Copy the infra. association rates into Current BSS state structure */
    Adapter->CurBssParams.NumOfRates = ratesSize;
    memcpy(&Adapter->CurBssParams.DataRates, rates, ratesSize);

    /* Adjust the timestamps in the scan table to be relative to the newly
       associated AP's TSF */
    wlan_scan_update_tsf_timestamps(priv, pBSSDesc);

    if (pBSSDesc->wmmIE.VendHdr.ElementId == WMM_IE)
        Adapter->CurBssParams.wmm_enabled = TRUE;
    else
        Adapter->CurBssParams.wmm_enabled = FALSE;

    if (Adapter->wmm.required && Adapter->CurBssParams.wmm_enabled)
        Adapter->wmm.enabled = TRUE;
    else
        Adapter->wmm.enabled = FALSE;

    Adapter->CurBssParams.wmm_uapsd_enabled = FALSE;

    if (Adapter->wmm.enabled == TRUE)
        Adapter->CurBssParams.wmm_uapsd_enabled
            = pBSSDesc->wmmIE.QoSInfo.QosUAPSD;

    PRINTM(INFO, "ASSOC_RESP: CurrentPacketFilter is %x\n",
           Adapter->CurrentPacketFilter);

    if (Adapter->SecInfo.WPAEnabled || Adapter->SecInfo.WPA2Enabled)
        Adapter->IsGTK_SET = FALSE;

    Adapter->DataRSSIlast = 0;
    Adapter->DataNFlast = 0;
    Adapter->DataRSSIAvg = 0;
    Adapter->DataNFAvg = 0;
    Adapter->BcnRSSIlast = 0;
    Adapter->BcnNFlast = 0;
    Adapter->BcnRSSIAvg = 0;
    Adapter->BcnNFAvg = 0;

    priv->adapter->dbg.num_cmd_assoc_success++;
    PRINTM(INFO, "ASSOC_RESP: Associated \n");

  done:
    LEAVE();
    return ret;
}

/**
 *  @brief This function handles the command response of deauthenticate
 *
 *  @param priv    A pointer to wlan_private structure
 *  @param resp    A pointer to HostCmd_DS_COMMAND
 *  @return        WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
int
wlan_ret_802_11_deauthenticate(wlan_private * priv, HostCmd_DS_COMMAND * resp)
{
    wlan_adapter *Adapter = priv->adapter;

    ENTER();

    Adapter->dbg.num_cmd_deauth++;
    if (memcmp(resp->params.deauth.MacAddr,
               &Adapter->CurBssParams.BSSDescriptor.MacAddress,
               sizeof(resp->params.deauth.MacAddr)) == 0)
        wlan_reset_connect_state(priv);

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/**
 *  @brief This function handles the command response of ad_hoc_start and
 *  ad_hoc_join
 *
 *  @param priv    A pointer to wlan_private structure
 *  @param resp    A pointer to HostCmd_DS_COMMAND
 *  @return        WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
int
wlan_ret_802_11_ad_hoc(wlan_private * priv, HostCmd_DS_COMMAND * resp)
{
    wlan_adapter *Adapter = priv->adapter;
    int ret = WLAN_STATUS_SUCCESS;
    u16 Command = resp->Command;
    u16 Result = resp->Result;
    HostCmd_DS_802_11_AD_HOC_RESULT *pAdHocResult;
    union iwreq_data wrqu;
    BSSDescriptor_t *pBSSDesc;

    ENTER();

    pAdHocResult = &resp->params.result;

    pBSSDesc = Adapter->pAttemptedBSSDesc;

    /* 
     * Join result code 0 --> SUCCESS
     */
    if (Result) {
        PRINTM(INFO, "ADHOC_RESP Failed\n");
        if (Adapter->MediaConnectStatus == WlanMediaStateConnected)
            wlan_reset_connect_state(priv);

        memset(&Adapter->CurBssParams.BSSDescriptor,
               0x00, sizeof(Adapter->CurBssParams.BSSDescriptor));

        LEAVE();
        return WLAN_STATUS_FAILURE;
    }

    /* Send a Media Connected event, according to the Spec */
    Adapter->MediaConnectStatus = WlanMediaStateConnected;

    if (Command == HostCmd_CMD_802_11_AD_HOC_START) {
        PRINTM(INFO, "ADHOC_S_RESP  %s\n", pBSSDesc->Ssid.Ssid);

        /* Update the created network descriptor with the new BSSID */
        memcpy(pBSSDesc->MacAddress, pAdHocResult->BSSID, ETH_ALEN);

        Adapter->AdhocState = ADHOC_STARTED;
    } else {
        /* 
         * Now the join cmd should be successful
         * If BSSID has changed use SSID to compare instead of BSSID
         */
        PRINTM(INFO, "ADHOC_J_RESP  %s\n", pBSSDesc->Ssid.Ssid);

        /* Make a copy of current BSSID descriptor, only needed for join since
           the current descriptor is already being used for adhoc start */
        memcpy(&Adapter->CurBssParams.BSSDescriptor,
               pBSSDesc, sizeof(BSSDescriptor_t));

        Adapter->AdhocState = ADHOC_JOINED;
    }

    memset(&wrqu, 0, sizeof(wrqu));
    memcpy(wrqu.ap_addr.sa_data,
           &Adapter->CurBssParams.BSSDescriptor.MacAddress, ETH_ALEN);
    wrqu.ap_addr.sa_family = ARPHRD_ETHER;
    wireless_send_event(priv->wlan_dev.netdev, SIOCGIWAP, &wrqu, NULL);

    PRINTM(INFO, "ADHOC_RESP: Channel = %d\n", Adapter->AdhocChannel);
    PRINTM(INFO, "ADHOC_RESP: BSSID = %02x:%02x:%02x:%02x:%02x:%02x\n",
           Adapter->CurBssParams.BSSDescriptor.MacAddress[0],
           Adapter->CurBssParams.BSSDescriptor.MacAddress[1],
           Adapter->CurBssParams.BSSDescriptor.MacAddress[2],
           Adapter->CurBssParams.BSSDescriptor.MacAddress[3],
           Adapter->CurBssParams.BSSDescriptor.MacAddress[4],
           Adapter->CurBssParams.BSSDescriptor.MacAddress[5]);

    LEAVE();
    return ret;
}

/**
 *  @brief This function handles the command response of ad_hoc_stop
 *
 *  @param priv    A pointer to wlan_private structure
 *  @param resp    A pointer to HostCmd_DS_COMMAND
 *  @return        WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
int
wlan_ret_802_11_ad_hoc_stop(wlan_private * priv, HostCmd_DS_COMMAND * resp)
{
    ENTER();

    wlan_reset_connect_state(priv);

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

#ifdef REASSOCIATION
/**
 *  @brief This function handles re-association. it is triggered
 *  by re-assoc timer.
 *
 *  @param data    A pointer to wlan_thread structure
 *  @return        WLAN_STATUS_SUCCESS
 */
int
wlan_reassociation_thread(void *data)
{
    wlan_thread *thread = data;
    wlan_private *priv = thread->priv;
    wlan_adapter *Adapter = priv->adapter;
    wait_queue_t wait;
    int i;
    int ret = WLAN_STATUS_SUCCESS;

    OS_INTERRUPT_SAVE_AREA;

    ENTER();

    wlan_activate_thread(thread);
    init_waitqueue_entry(&wait, current);

    current->flags |= PF_NOFREEZE;

    for (;;) {
        add_wait_queue(&thread->waitQ, &wait);
        OS_SET_THREAD_STATE(TASK_INTERRUPTIBLE);

        PRINTM(INFO, "Reassoc: Thread sleeping...\n");

        schedule();

        OS_SET_THREAD_STATE(TASK_RUNNING);
        remove_wait_queue(&thread->waitQ, &wait);

        if (Adapter->SurpriseRemoved)
            break;

        if (kthread_should_stop())
            break;

        PRINTM(INFO, "Reassoc: Thread waking up...\n");

        if (Adapter->InfrastructureMode != Wlan802_11Infrastructure ||
            Adapter->HardwareStatus != WlanHardwareStatusReady) {
            PRINTM(MSG, "Reassoc: mode or hardware status is not correct\n");
            continue;
        }

        /* The semaphore is used to avoid reassociation thread and
           wlan_set_scan/wlan_set_essid interrupting each other. Reassociation
           should be disabled completely by application if
           wlan_set_user_scan_ioctl/wlan_set_wap is used. */
        if (OS_ACQ_SEMAPHORE_BLOCK(&Adapter->ReassocSem)) {
            PRINTM(ERROR, "Acquire semaphore error, reassociation thread\n");
            goto settimer;
        }

        if (Adapter->MediaConnectStatus != WlanMediaStateDisconnected) {
            OS_REL_SEMAPHORE(&Adapter->ReassocSem);
            PRINTM(MSG, "Reassoc: Adapter->MediaConnectStatus is wrong\n");
            continue;
        }

        PRINTM(INFO, "Reassoc: Required ESSID: %s\n",
               Adapter->PreviousSSID.Ssid);

        PRINTM(INFO, "Reassoc: Performing Active Scan @ %lu\n", os_time_get());
        wlan_cmd_specific_scan_ssid(priv, &Adapter->PreviousSSID);
        if (Adapter->SurpriseRemoved) {
            OS_REL_SEMAPHORE(&Adapter->ReassocSem);
            break;
        }

        i = wlan_find_ssid_in_list(Adapter,
                                   &Adapter->PreviousSSID,
                                   Adapter->PreviousBSSID,
                                   Adapter->InfrastructureMode);

        if (i < 0) {
            /* If the SSID could not be found, try just the SSID */
            i = wlan_find_ssid_in_list(Adapter,
                                       &Adapter->PreviousSSID,
                                       NULL, Adapter->InfrastructureMode);
        }

        if (i >= 0) {
            if (Adapter->SecInfo.WEPStatus == Wlan802_11WEPEnabled) {
                ret = wlan_prepare_cmd(priv,
                                       HostCmd_CMD_802_11_SET_WEP,
                                       HostCmd_ACT_ADD,
                                       HostCmd_OPTION_WAITFORRSP, 0, NULL);
                if (ret)
                    PRINTM(INFO, "Ressoc: Fail to set WEP KEY\n");
            }
            wlan_associate(priv, &Adapter->ScanTable[i]);
        }

        OS_REL_SEMAPHORE(&Adapter->ReassocSem);

      settimer:
        if (Adapter->MediaConnectStatus == WlanMediaStateDisconnected) {
            PRINTM(INFO, "Reassoc: No AP found or assoc failed."
                   "Restarting re-assoc Timer @ %lu\n", os_time_get());

            Adapter->ReassocTimerIsSet = TRUE;
            wlan_mod_timer(&Adapter->MrvDrvTimer, MRVDRV_TIMER_10S);
        }
    }

    wlan_deactivate_thread(thread);

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief This function triggers re-association by waking up
 *  re-assoc thread.
 *  
 *  @param FunctionContext    A pointer to FunctionContext
 *  @return 	   n/a
 */
void
wlan_reassoc_timer_func(void *FunctionContext)
{
    wlan_private *priv = (wlan_private *) FunctionContext;
    wlan_adapter *Adapter = priv->adapter;
    OS_INTERRUPT_SAVE_AREA;

    ENTER();

    PRINTM(INFO, "MrvDrvReassocTimer fired.\n");
    Adapter->ReassocTimerIsSet = FALSE;
    if (Adapter->PSState != PS_STATE_FULL_POWER) {
        /* wait until Exit_PS command returns */
        Adapter->ReassocTimerIsSet = TRUE;
        wlan_mod_timer(&Adapter->MrvDrvTimer, MRVDRV_TIMER_1S);
        PRINTM(INFO, "MrvDrvTimerFunction(PSState=%d) waiting"
               "for Exit_PS done\n", Adapter->PSState);
        LEAVE();
        return;
    }

    PRINTM(INFO, "Waking Up the Reassoc Thread\n");

    wake_up_interruptible(&priv->ReassocThread.waitQ);

    LEAVE();
    return;
}
#endif /* REASSOCIATION */
