/** @file  wlan_wext.c 
  * @brief This file contains standard ioctl functions
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
/************************************************************************
Change log:
	10/10/05: Add Doxygen format comments
	12/23/05: Modify FindBSSIDInList to search entire table for
	          duplicate BSSIDs when earlier matches are not compatible
	12/26/05: Remove errant memcpy in wlanidle_off; overwriting stack space
	01/05/06: Add kernel 2.6.x support	
	01/11/06: Conditionalize new scan/join functions.
	          Update statics/externs.  Move forward decl. from wlan_decl.h
	04/06/06: Add TSPEC, queue metrics, and MSDU expiry support
	04/10/06: Add hostcmd generic API
	04/18/06: Remove old Subscribe Event and add new Subscribe Event
	          implementation through generic hostcmd API
	05/04/06: Add IBSS coalescing related new iwpriv command
	08/29/06: Add ledgpio private command
	10/23/06: Validate setbcnavg/setdataavg command parameters and
	          return error if out of range
	11/15/07: Move private ioctl handler and tables to wlan_priv.c/h
************************************************************************/

#include	"wlan_headers.h"

/********************************************************
		Local Variables
********************************************************/

/********************************************************
		Global Variables
********************************************************/

/********************************************************
		Local Functions
********************************************************/
/** 
 *  @brief Find a character in a string.
 *   
 *  @param s	   A pointer to string
 *  @param c	   Character to be located 
 *  @param n       the length of string
 *  @return 	   A pointer to the first occurrence of c in string, or NULL if c is not found.
 */
static void *
wlan_memchr(void *s, int c, int n)
{
    const u8 *p = s;

    ENTER();

    while (n-- != 0) {
        if ((u8) c == *p++) {
            LEAVE();
            return (void *) (p - 1);
        }
    }

    LEAVE();

    return NULL;
}

/** 
 *  @brief Find the channel frequency power info with specific frequency
 *   
 *  @param adapter  A pointer to wlan_adapter structure
 *  @param band     it can be BAND_A, BAND_G or BAND_B
 *  @param freq     the frequency for looking   
 *  @return         Pointer to CHANNEL_FREQ_POWER structure; NULL if not found
 */
static CHANNEL_FREQ_POWER *
find_cfp_by_band_and_freq(wlan_adapter * adapter, u8 band, u32 freq)
{
    CHANNEL_FREQ_POWER *cfp = NULL;
    REGION_CHANNEL *rc;
    int count = sizeof(adapter->region_channel) /
        sizeof(adapter->region_channel[0]);
    int i, j;

    ENTER();

    for (j = 0; !cfp && (j < count); j++) {
        rc = &adapter->region_channel[j];

        if (adapter->State11D.Enable11D == ENABLE_11D)
            rc = &adapter->universal_channel[j];

        if (!rc->Valid || !rc->CFP)
            continue;
        switch (rc->Band) {
        case BAND_A:
            switch (band) {
            case BAND_A:       /* matching BAND_A */
                break;
            default:
                continue;
            }
            break;
        case BAND_B:
        case BAND_G:
            switch (band) {
            case BAND_B:
            case BAND_G:
            case 0:
                break;
            default:
                continue;
            }
            break;
        default:
            continue;
        }
        for (i = 0; i < rc->NrCFP; i++) {
            if (rc->CFP[i].Freq == freq) {
                cfp = &rc->CFP[i];
                break;
            }
        }
    }

    if (!cfp && freq)
        PRINTM(INFO, "find_cfp_by_band_and_freql(): cannot find cfp by "
               "band %d & freq %d\n", band, freq);

    LEAVE();

    return cfp;
}

/** 
 *  @brief Update Current Channel 
 *   
 *  @param priv 		A pointer to wlan_private structure
 *  @return 	   		WLAN_STATUS_SUCCESS--success, WLAN_STATUS_FAILURE--fail
 */
static int
wlan_update_current_chan(wlan_private * priv)
{
    int ret;

    /* 
     ** the channel in f/w could be out of sync, get the current channel
     */
    ENTER();

    ret = wlan_prepare_cmd(priv, HostCmd_CMD_802_11_RF_CHANNEL,
                           HostCmd_OPT_802_11_RF_CHANNEL_GET,
                           HostCmd_OPTION_WAITFORRSP, 0, NULL);

    PRINTM(INFO, "Current Channel = %d\n",
           priv->adapter->CurBssParams.BSSDescriptor.Channel);

    LEAVE();

    return ret;
}

/** 
 *  @brief Set Current Channel 
 *   
 *  @param priv 		A pointer to wlan_private structure
 *  @param channel		The channel to be set. 
 *  @return 	   		WLAN_STATUS_SUCCESS--success, WLAN_STATUS_FAILURE--fail
 */
static int
wlan_set_current_chan(wlan_private * priv, int channel)
{
    int ret = WLAN_STATUS_SUCCESS;

    ENTER();
    PRINTM(INFO, "Set Channel = %d\n", channel);

    /* 
     **  Current channel is not set to AdhocChannel requested, set channel
     */
    ret = wlan_prepare_cmd(priv, HostCmd_CMD_802_11_RF_CHANNEL,
                           HostCmd_OPT_802_11_RF_CHANNEL_SET,
                           HostCmd_OPTION_WAITFORRSP, 0, &channel);
    LEAVE();
    return ret;
}

/*
 *  iwconfig ethX key on:	WEPEnabled;
 *  iwconfig ethX key off:	WEPDisabled;
 *  iwconfig ethX key [x]:	CurrentWepKeyIndex = x; WEPEnabled;
 *  iwconfig ethX key [x] kstr:	WepKey[x] = kstr;
 *  iwconfig ethX key kstr:	WepKey[CurrentWepKeyIndex] = kstr;
 *  all:			Send command SET_WEP;
 * 				SetMacPacketFilter;
 */

/** 
 *  @brief Set WEP key
 *   
 *  @param dev                  A pointer to net_device structure
 *  @param info			A pointer to iw_request_info structure
 *  @param dwrq 		A pointer to iw_point structure
 *  @param extra		A pointer to extra data buf
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_set_encode_nonwpa(struct net_device *dev,
                       struct iw_request_info *info,
                       struct iw_point *dwrq, char *extra)
{
    int ret = WLAN_STATUS_SUCCESS;
    wlan_private *priv = netdev_priv(dev);
    wlan_adapter *Adapter = priv->adapter;
    MRVL_WEP_KEY *pWep;
    int index, PrevAuthMode;

    ENTER();

    if (!wlan_is_cmd_allowed(priv)) {
        PRINTM(MSG, "%s: not allowed\n", __FUNCTION__);
        LEAVE();
        return -EBUSY;
    }

    if (Adapter->CurrentWepKeyIndex >= MRVL_NUM_WEP_KEY)
        Adapter->CurrentWepKeyIndex = 0;
    pWep = &Adapter->WepKey[Adapter->CurrentWepKeyIndex];
    PrevAuthMode = Adapter->SecInfo.AuthenticationMode;

    index = (dwrq->flags & IW_ENCODE_INDEX) - 1;

    if (index >= 4) {
        PRINTM(INFO, "Key index #%d out of range\n", index + 1);
        LEAVE();
        return -EINVAL;
    }

    PRINTM(INFO, "Flags=0x%x, Length=%d Index=%d CurrentWepKeyIndex=%d\n",
           dwrq->flags, dwrq->length, index, Adapter->CurrentWepKeyIndex);

    if (dwrq->length > 0) {
        /* iwconfig ethX key [n] xxxxxxxxxxx Key has been provided by the user 
         */

        /* 
         * Check the size of the key 
         */

        if (dwrq->length > MAX_WEP_KEY_SIZE) {
            LEAVE();
            return -EINVAL;
        }

        /* 
         * Check the index (none -> use current) 
         */

        if (index < 0 || index > 3)     /* invalid index or no index */
            index = Adapter->CurrentWepKeyIndex;
        else                    /* index is given & valid */
            pWep = &Adapter->WepKey[index];

        /* 
         * Check if the key is not marked as invalid 
         */
        if (!(dwrq->flags & IW_ENCODE_NOKEY)) {
            /* Cleanup */
            memset(pWep, 0, sizeof(MRVL_WEP_KEY));

            /* Copy the key in the driver */
            memcpy(pWep->KeyMaterial, extra, dwrq->length);

            /* Set the length */
            if (dwrq->length > MIN_WEP_KEY_SIZE)
                pWep->KeyLength = MAX_WEP_KEY_SIZE;
            else {
                if (dwrq->length > 0)
                    pWep->KeyLength = MIN_WEP_KEY_SIZE;
                else
                    /* Disable the key */
                    pWep->KeyLength = 0;
            }
            pWep->KeyIndex = index;

            if (Adapter->SecInfo.WEPStatus != Wlan802_11WEPEnabled) {
                /* 
                 * The status is set as Key Absent 
                 * so as to make sure we display the 
                 * keys when iwlist ethX key is used
                 */
                Adapter->SecInfo.WEPStatus = Wlan802_11WEPKeyAbsent;
            }

            PRINTM(INFO, "KeyIndex=%u KeyLength=%u\n",
                   pWep->KeyIndex, pWep->KeyLength);
            HEXDUMP("WepKey", (u8 *) pWep->KeyMaterial, pWep->KeyLength);
        }
    } else {
        /* 
         * No key provided so it is either enable key, 
         * on or off */
        if (dwrq->flags & IW_ENCODE_DISABLED) {
            PRINTM(INFO, "*** iwconfig ethX key off ***\n");

            Adapter->SecInfo.WEPStatus = Wlan802_11WEPDisabled;
            if (Adapter->SecInfo.AuthenticationMode == Wlan802_11AuthModeShared)
                Adapter->SecInfo.AuthenticationMode = Wlan802_11AuthModeOpen;
        } else {
            /* iwconfig ethX key [n] iwconfig ethX key on Do we want to just
               set the transmit key index ? */

            if (index < 0 || index > 3) {
                PRINTM(INFO, "*** iwconfig ethX key on ***\n");
                index = Adapter->CurrentWepKeyIndex;
            } else {
                PRINTM(INFO, "*** iwconfig ethX key [x=%d] ***\n", index);
                Adapter->CurrentWepKeyIndex = index;
            }

            /* Copy the required key as the current key */
            pWep = &Adapter->WepKey[index];

            if (!pWep->KeyLength) {
                PRINTM(INFO, "Key not set,so cannot enable it\n");
                LEAVE();
                return -EPERM;
            }

            Adapter->SecInfo.WEPStatus = Wlan802_11WEPEnabled;

            HEXDUMP("KeyMaterial", (u8 *) pWep->KeyMaterial, pWep->KeyLength);
        }
    }

    if (pWep->KeyLength) {
        ret = wlan_prepare_cmd(priv,
                               HostCmd_CMD_802_11_SET_WEP,
                               HostCmd_ACT_ADD, HostCmd_OPTION_WAITFORRSP,
                               0, NULL);

        if (ret) {
            LEAVE();
            return ret;
        }
    }

    if (Adapter->SecInfo.WEPStatus == Wlan802_11WEPEnabled)
        Adapter->CurrentPacketFilter |= HostCmd_ACT_MAC_WEP_ENABLE;
    else
        Adapter->CurrentPacketFilter &= ~HostCmd_ACT_MAC_WEP_ENABLE;

    ret = wlan_prepare_cmd(priv,
                           HostCmd_CMD_MAC_CONTROL,
                           0, HostCmd_OPTION_WAITFORRSP,
                           0, &Adapter->CurrentPacketFilter);

    if (dwrq->flags & IW_ENCODE_RESTRICTED) {
        /* iwconfig ethX restricted key [1] */
        Adapter->SecInfo.AuthenticationMode = Wlan802_11AuthModeShared;
        PRINTM(INFO, "Auth mode restricted!\n");
    } else if (dwrq->flags & IW_ENCODE_OPEN) {
        /* iwconfig ethX key [2] open */
        Adapter->SecInfo.AuthenticationMode = Wlan802_11AuthModeOpen;
        PRINTM(INFO, "Auth mode open!\n");
    }

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Copy Rates
 *   
 *  @param dest                 A pointer to Dest Buf
 *  @param pos		        The position for copy
 *  @param src		        A pointer to Src Buf
 *  @param len                  The len of Src Buf
 *  @return 	   	        Number of Rates copyed 
 */
static inline int
wlan_copy_rates(u8 * dest, int pos, u8 * src, int len)
{
    int i;

    ENTER();

    for (i = 0; i < len && src[i]; i++, pos++) {
        if (pos >= sizeof(WLAN_802_11_RATES))
            break;
        dest[pos] = src[i];
    }

    LEAVE();

    return pos;
}

/** 
 *  @brief Commit handler: called after a bunch of SET operations
 *   
 *  @param dev                  A pointer to net_device structure
 *  @param info			A pointer to iw_request_info structure
 *  @param vwrq 		A pointer to iw_param structure
 *  @param extra		A pointer to extra data buf
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_config_commit(struct net_device *dev,
                   struct iw_request_info *info, char *cwrq, char *extra)
{
    ENTER();

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Get protocol name 
 *   
 *  @param dev                  A pointer to net_device structure
 *  @param info			A pointer to iw_request_info structure
 *  @param vwrq 		A pointer to iw_param structure
 *  @param extra		A pointer to extra data buf
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_get_name(struct net_device *dev, struct iw_request_info *info,
              char *cwrq, char *extra)
{
    const char *cp;
    char comm[6] = { "COMM-" };
    char mrvl[6] = { "MRVL-" };
    int cnt;

    ENTER();

    strcpy(cwrq, mrvl);

    cp = strstr(driver_version, comm);
    if (cp == driver_version)   /* skip leading "COMM-" */
        cp = driver_version + strlen(comm);
    else
        cp = driver_version;

    cnt = strlen(mrvl);
    cwrq += cnt;
    while (cnt < 16 && (*cp != '-')) {
        *cwrq++ = toupper(*cp++);
        cnt++;
    }
    *cwrq = '\0';

    LEAVE();

    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Get frequency
 *   
 *  @param dev                  A pointer to net_device structure
 *  @param info			A pointer to iw_request_info structure
 *  @param fwrq 		A pointer to iw_freq structure
 *  @param extra		A pointer to extra data buf
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_get_freq(struct net_device *dev, struct iw_request_info *info,
              struct iw_freq *fwrq, char *extra)
{
    wlan_private *priv = netdev_priv(dev);
    wlan_adapter *Adapter = priv->adapter;
    CHANNEL_FREQ_POWER *cfp;

    ENTER();

    if (!wlan_is_cmd_allowed(priv)) {
        PRINTM(MSG, "%s: not allowed\n", __FUNCTION__);
        LEAVE();
        return -EBUSY;
    }

    cfp = find_cfp_by_band_and_channel(Adapter, Adapter->CurBssParams.band,
                                       (u16) Adapter->CurBssParams.
                                       BSSDescriptor.Channel);

    if (!cfp) {
        if (Adapter->CurBssParams.BSSDescriptor.Channel) {
            PRINTM(INFO, "Invalid channel=%d\n",
                   Adapter->CurBssParams.BSSDescriptor.Channel);
        }
        LEAVE();
        return -EINVAL;
    }

    fwrq->m = (long) cfp->Freq * 100000;
    fwrq->e = 1;

    PRINTM(INFO, "freq=%u\n", fwrq->m);

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Get current BSSID
 *   
 *  @param dev                  A pointer to net_device structure
 *  @param info			A pointer to iw_request_info structure
 *  @param awrq 		A pointer to iw_param structure
 *  @param extra		A pointer to extra data buf
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_get_wap(struct net_device *dev, struct iw_request_info *info,
             struct sockaddr *awrq, char *extra)
{
    wlan_private *priv = netdev_priv(dev);
    wlan_adapter *Adapter = priv->adapter;

    ENTER();

    if (Adapter->MediaConnectStatus == WlanMediaStateConnected) {
        memcpy(awrq->sa_data,
               Adapter->CurBssParams.BSSDescriptor.MacAddress, ETH_ALEN);
    } else {
        memset(awrq->sa_data, 0, ETH_ALEN);
    }
    awrq->sa_family = ARPHRD_ETHER;

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Set Adapter Node Name
 *   
 *  @param dev                  A pointer to net_device structure
 *  @param info			A pointer to iw_request_info structure
 *  @param vwrq 		A pointer to iw_param structure
 *  @param extra		A pointer to extra data buf
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_set_nick(struct net_device *dev, struct iw_request_info *info,
              struct iw_point *dwrq, char *extra)
{
    wlan_private *priv = netdev_priv(dev);
    wlan_adapter *Adapter = priv->adapter;

    ENTER();

    /* 
     * Check the size of the string 
     */

    if (dwrq->length > 16) {
        LEAVE();
        return -E2BIG;
    }

    memset(Adapter->nodeName, 0, sizeof(Adapter->nodeName));
    memcpy(Adapter->nodeName, extra, dwrq->length);

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Get Adapter Node Name
 *   
 *  @param dev                  A pointer to net_device structure
 *  @param info			A pointer to iw_request_info structure
 *  @param vwrq 		A pointer to iw_param structure
 *  @param extra		A pointer to extra data buf
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_get_nick(struct net_device *dev, struct iw_request_info *info,
              struct iw_point *dwrq, char *extra)
{
    wlan_private *priv = netdev_priv(dev);
    wlan_adapter *Adapter = priv->adapter;

    ENTER();

    /* 
     * Get the Nick Name saved 
     */

    strncpy(extra, Adapter->nodeName, 16);

    extra[16] = '\0';

    /* 
     * If none, we may want to get the one that was set 
     */

    /* 
     * Push it out ! 
     */
    dwrq->length = strlen(extra) + 1;

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Set RTS threshold
 *   
 *  @param dev                  A pointer to net_device structure
 *  @param info			A pointer to iw_request_info structure
 *  @param vwrq 		A pointer to iw_param structure
 *  @param extra		A pointer to extra data buf
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_set_rts(struct net_device *dev, struct iw_request_info *info,
             struct iw_param *vwrq, char *extra)
{
    int ret = WLAN_STATUS_SUCCESS;
    wlan_private *priv = netdev_priv(dev);
    wlan_adapter *Adapter = priv->adapter;
    int rthr = vwrq->value;

    ENTER();

    if (!wlan_is_cmd_allowed(priv)) {
        PRINTM(MSG, "%s: not allowed\n", __FUNCTION__);
        LEAVE();
        return -EBUSY;
    }

    if (vwrq->disabled) {
        Adapter->RTSThsd = rthr = MRVDRV_RTS_MAX_VALUE;
    } else {
        if (rthr < MRVDRV_RTS_MIN_VALUE || rthr > MRVDRV_RTS_MAX_VALUE) {
            LEAVE();
            return -EINVAL;
        }
        Adapter->RTSThsd = rthr;
    }

    ret = wlan_prepare_cmd(priv, HostCmd_CMD_802_11_SNMP_MIB,
                           HostCmd_ACT_GEN_SET, HostCmd_OPTION_WAITFORRSP,
                           RtsThresh_i, &rthr);

    LEAVE();
    return ret;
}

/** 
 *  @brief Get RTS threshold
 *   
 *  @param dev                  A pointer to net_device structure
 *  @param info			A pointer to iw_request_info structure
 *  @param vwrq 		A pointer to iw_param structure
 *  @param extra		A pointer to extra data buf
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_get_rts(struct net_device *dev, struct iw_request_info *info,
             struct iw_param *vwrq, char *extra)
{
    int ret = WLAN_STATUS_SUCCESS;
    wlan_private *priv = netdev_priv(dev);
    wlan_adapter *Adapter = priv->adapter;

    ENTER();

    if (!wlan_is_cmd_allowed(priv)) {
        PRINTM(MSG, "%s: not allowed\n", __FUNCTION__);
        LEAVE();
        return -EBUSY;
    }

    Adapter->RTSThsd = 0;
    ret = wlan_prepare_cmd(priv, HostCmd_CMD_802_11_SNMP_MIB,
                           HostCmd_ACT_GEN_GET, HostCmd_OPTION_WAITFORRSP,
                           RtsThresh_i, NULL);
    if (ret) {
        LEAVE();
        return ret;
    }

    vwrq->value = Adapter->RTSThsd;
    vwrq->disabled = ((vwrq->value < MRVDRV_RTS_MIN_VALUE)
                      || (vwrq->value > MRVDRV_RTS_MAX_VALUE));
    vwrq->fixed = 1;

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Set Fragment threshold
 *   
 *  @param dev                  A pointer to net_device structure
 *  @param info			A pointer to iw_request_info structure
 *  @param vwrq 		A pointer to iw_param structure
 *  @param extra		A pointer to extra data buf
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_set_frag(struct net_device *dev, struct iw_request_info *info,
              struct iw_param *vwrq, char *extra)
{
    int ret = WLAN_STATUS_SUCCESS;
    int fthr = vwrq->value;
    wlan_private *priv = netdev_priv(dev);
    wlan_adapter *Adapter = priv->adapter;

    ENTER();

    if (!wlan_is_cmd_allowed(priv)) {
        PRINTM(MSG, "%s: not allowed\n", __FUNCTION__);
        LEAVE();
        return -EBUSY;
    }

    if (vwrq->disabled) {
        Adapter->FragThsd = fthr = MRVDRV_FRAG_MAX_VALUE;
    } else {
        if (fthr < MRVDRV_FRAG_MIN_VALUE || fthr > MRVDRV_FRAG_MAX_VALUE) {
            LEAVE();
            return -EINVAL;
        }
        Adapter->FragThsd = fthr;
    }

    ret = wlan_prepare_cmd(priv, HostCmd_CMD_802_11_SNMP_MIB,
                           HostCmd_ACT_GEN_SET, HostCmd_OPTION_WAITFORRSP,
                           FragThresh_i, &fthr);
    LEAVE();
    return ret;
}

/** 
 *  @brief Get Fragment threshold
 *   
 *  @param dev                  A pointer to net_device structure
 *  @param info			A pointer to iw_request_info structure
 *  @param vwrq 		A pointer to iw_param structure
 *  @param extra		A pointer to extra data buf
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_get_frag(struct net_device *dev, struct iw_request_info *info,
              struct iw_param *vwrq, char *extra)
{
    int ret = WLAN_STATUS_SUCCESS;
    wlan_private *priv = netdev_priv(dev);
    wlan_adapter *Adapter = priv->adapter;

    ENTER();

    if (!wlan_is_cmd_allowed(priv)) {
        PRINTM(MSG, "%s: not allowed\n", __FUNCTION__);
        LEAVE();
        return -EBUSY;
    }

    Adapter->FragThsd = 0;
    ret = wlan_prepare_cmd(priv,
                           HostCmd_CMD_802_11_SNMP_MIB, HostCmd_ACT_GEN_GET,
                           HostCmd_OPTION_WAITFORRSP, FragThresh_i, NULL);
    if (ret) {
        LEAVE();
        return ret;
    }

    vwrq->value = Adapter->FragThsd;
    vwrq->disabled = ((vwrq->value < MRVDRV_FRAG_MIN_VALUE)
                      || (vwrq->value > MRVDRV_FRAG_MAX_VALUE));
    vwrq->fixed = 1;

    LEAVE();
    return ret;
}

/** 
 *  @brief Get Wlan Mode
 *   
 *  @param dev                  A pointer to net_device structure
 *  @param info			A pointer to iw_request_info structure
 *  @param vwrq 		A pointer to iw_param structure
 *  @param extra		A pointer to extra data buf
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_get_mode(struct net_device *dev,
              struct iw_request_info *info, u32 * uwrq, char *extra)
{
    wlan_private *priv = netdev_priv(dev);
    wlan_adapter *adapter = priv->adapter;

    ENTER();

    switch (adapter->InfrastructureMode) {
    case Wlan802_11IBSS:
        *uwrq = IW_MODE_ADHOC;
        break;

    case Wlan802_11Infrastructure:
        *uwrq = IW_MODE_INFRA;
        break;

    default:
    case Wlan802_11AutoUnknown:
        *uwrq = IW_MODE_AUTO;
        break;
    }

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Get Encryption key
 *   
 *  @param dev                  A pointer to net_device structure
 *  @param info			A pointer to iw_request_info structure
 *  @param vwrq 		A pointer to iw_param structure
 *  @param extra		A pointer to extra data buf
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_get_encode(struct net_device *dev,
                struct iw_request_info *info, struct iw_point *dwrq, u8 * extra)
{

    wlan_private *priv = netdev_priv(dev);
    wlan_adapter *adapter = priv->adapter;
    int index = (dwrq->flags & IW_ENCODE_INDEX);

    ENTER();

    PRINTM(INFO, "flags=0x%x index=%d length=%d CurrentWepKeyIndex=%d\n",
           dwrq->flags, index, dwrq->length, adapter->CurrentWepKeyIndex);
    if (index < 0 || index > 4) {
        PRINTM(INFO, "Key index #%d out of range\n", index);
        LEAVE();
        return -EINVAL;
    }
    if (adapter->CurrentWepKeyIndex >= MRVL_NUM_WEP_KEY)
        adapter->CurrentWepKeyIndex = 0;
    dwrq->flags = 0;

    /* 
     * Check encryption mode 
     */

    switch (adapter->SecInfo.AuthenticationMode) {
    case Wlan802_11AuthModeOpen:
        dwrq->flags = IW_ENCODE_OPEN;
        break;

    case Wlan802_11AuthModeShared:
    case Wlan802_11AuthModeNetworkEAP:
        dwrq->flags = IW_ENCODE_RESTRICTED;
        break;
    default:
        dwrq->flags = IW_ENCODE_DISABLED | IW_ENCODE_OPEN;
        break;
    }

    if ((adapter->SecInfo.WEPStatus == Wlan802_11WEPEnabled)
        || (adapter->SecInfo.WEPStatus == Wlan802_11WEPKeyAbsent)
        || adapter->SecInfo.WPAEnabled || adapter->SecInfo.WPA2Enabled) {
        dwrq->flags &= ~IW_ENCODE_DISABLED;
    } else {
        dwrq->flags |= IW_ENCODE_DISABLED;
    }

    memset(extra, 0, 16);

    if (!index) {
        /* Handle current key request */
        if ((adapter->WepKey[adapter->CurrentWepKeyIndex].KeyLength) &&
            (adapter->SecInfo.WEPStatus == Wlan802_11WEPEnabled)) {
            index = adapter->WepKey[adapter->CurrentWepKeyIndex].KeyIndex;
            memcpy(extra, adapter->WepKey[index].KeyMaterial,
                   adapter->WepKey[index].KeyLength);
            dwrq->length = adapter->WepKey[index].KeyLength;
            /* return current key */
            dwrq->flags |= (index + 1);
            /* return WEP enabled */
            dwrq->flags &= ~IW_ENCODE_DISABLED;
        } else if ((adapter->SecInfo.WPAEnabled)
                   || (adapter->SecInfo.WPA2Enabled)
            ) {
            /* return WPA enabled */
            dwrq->flags &= ~IW_ENCODE_DISABLED;
        } else {
            dwrq->flags |= IW_ENCODE_DISABLED;
        }
    } else {
        /* Handle specific key requests */
        index--;
        if (adapter->WepKey[index].KeyLength) {
            memcpy(extra, adapter->WepKey[index].KeyMaterial,
                   adapter->WepKey[index].KeyLength);
            dwrq->length = adapter->WepKey[index].KeyLength;
            /* return current key */
            dwrq->flags |= (index + 1);
            /* return WEP enabled */
            dwrq->flags &= ~IW_ENCODE_DISABLED;
        } else if ((adapter->SecInfo.WPAEnabled)
                   || (adapter->SecInfo.WPA2Enabled)
            ) {
            /* return WPA enabled */
            dwrq->flags &= ~IW_ENCODE_DISABLED;
        } else {
            dwrq->flags |= IW_ENCODE_DISABLED;
        }
    }

    dwrq->flags |= IW_ENCODE_NOKEY;

    PRINTM(INFO, "Key:%02x:%02x:%02x:%02x:%02x:%02x KeyLen=%d\n",
           extra[0], extra[1], extra[2],
           extra[3], extra[4], extra[5], dwrq->length);

    PRINTM(INFO, "Return flags=0x%x\n", dwrq->flags);

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Get TX Power
 *   
 *  @param dev                  A pointer to net_device structure
 *  @param info			A pointer to iw_request_info structure
 *  @param vwrq 		A pointer to iw_param structure
 *  @param extra		A pointer to extra data buf
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_get_txpow(struct net_device *dev,
               struct iw_request_info *info, struct iw_param *vwrq, char *extra)
{
    int ret = WLAN_STATUS_SUCCESS;
    wlan_private *priv = netdev_priv(dev);
    wlan_adapter *Adapter = priv->adapter;

    ENTER();

    if (!wlan_is_cmd_allowed(priv)) {
        PRINTM(MSG, "%s: not allowed\n", __FUNCTION__);
        LEAVE();
        return -EBUSY;
    }

    ret = wlan_prepare_cmd(priv,
                           HostCmd_CMD_802_11_RF_TX_POWER,
                           HostCmd_ACT_GEN_GET,
                           HostCmd_OPTION_WAITFORRSP, 0, NULL);

    if (ret) {
        LEAVE();
        return ret;
    }

    PRINTM(INFO, "TXPOWER GET %d dbm.\n", Adapter->TxPowerLevel);
    vwrq->value = Adapter->TxPowerLevel;
    vwrq->fixed = 1;
    if (Adapter->RadioOn) {
        vwrq->disabled = 0;
        vwrq->flags = IW_TXPOW_DBM;
    } else {
        vwrq->disabled = 1;
    }

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Set TX Retry Count
 *   
 *  @param dev                  A pointer to net_device structure
 *  @param info			A pointer to iw_request_info structure
 *  @param vwrq 		A pointer to iw_param structure
 *  @param extra		A pointer to extra data buf
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_set_retry(struct net_device *dev, struct iw_request_info *info,
               struct iw_param *vwrq, char *extra)
{
    int ret = WLAN_STATUS_SUCCESS;
    wlan_private *priv = netdev_priv(dev);
    wlan_adapter *adapter = priv->adapter;

    ENTER();

    if (!wlan_is_cmd_allowed(priv)) {
        PRINTM(MSG, "%s: not allowed\n", __FUNCTION__);
        LEAVE();
        return -EBUSY;
    }

    if (vwrq->flags == IW_RETRY_LIMIT) {
        /* The MAC has a 4-bit Total_Tx_Count register Total_Tx_Count = 1 +
           Tx_Retry_Count */
#define TX_RETRY_MIN 0
#define TX_RETRY_MAX 14
        if (vwrq->value < TX_RETRY_MIN || vwrq->value > TX_RETRY_MAX) {
            LEAVE();
            return -EINVAL;
        }
        /* Set Tx retry count */
        adapter->TxRetryCount = vwrq->value;

        ret = wlan_prepare_cmd(priv, HostCmd_CMD_802_11_SNMP_MIB,
                               HostCmd_ACT_GEN_SET, HostCmd_OPTION_WAITFORRSP,
                               ShortRetryLim_i, NULL);

        if (ret) {
            LEAVE();
            return ret;
        }
    } else {
        LEAVE();
        return -EOPNOTSUPP;
    }

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Get TX Retry Count
 *   
 *  @param dev                  A pointer to net_device structure
 *  @param info			A pointer to iw_request_info structure
 *  @param vwrq 		A pointer to iw_param structure
 *  @param extra		A pointer to extra data buf
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_get_retry(struct net_device *dev, struct iw_request_info *info,
               struct iw_param *vwrq, char *extra)
{
    wlan_private *priv = netdev_priv(dev);
    wlan_adapter *Adapter = priv->adapter;
    int ret = WLAN_STATUS_SUCCESS;

    ENTER();

    if (!wlan_is_cmd_allowed(priv)) {
        PRINTM(MSG, "%s: not allowed\n", __FUNCTION__);
        LEAVE();
        return -EBUSY;
    }

    Adapter->TxRetryCount = 0;
    ret = wlan_prepare_cmd(priv,
                           HostCmd_CMD_802_11_SNMP_MIB, HostCmd_ACT_GEN_GET,
                           HostCmd_OPTION_WAITFORRSP, ShortRetryLim_i, NULL);
    if (ret) {
        LEAVE();
        return ret;
    }
    vwrq->disabled = 0;
    if (!vwrq->flags) {
        vwrq->flags = IW_RETRY_LIMIT;
        /* Get Tx retry count */
        vwrq->value = Adapter->TxRetryCount;
    }

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Sort Channels
 *   
 *  @param freq 		A pointer to iw_freq structure
 *  @param num		        number of Channels
 *  @return 	   		NA
 */
static inline void
sort_channels(struct iw_freq *freq, int num)
{
    int i, j;
    struct iw_freq temp;

    ENTER();

    for (i = 0; i < num; i++)
        for (j = i + 1; j < num; j++)
            if (freq[i].i > freq[j].i) {
                temp.i = freq[i].i;
                temp.m = freq[i].m;

                freq[i].i = freq[j].i;
                freq[i].m = freq[j].m;

                freq[j].i = temp.i;
                freq[j].m = temp.m;
            }

    LEAVE();
}

/* data rate listing
 *	MULTI_BANDS:
 *		abg		a	b	b/g
 *  Infra 	G(12)		A(8)	B(4)	G(12)
 *  Adhoc 	A+B(12)		A(8)	B(4)	B(4)
 *	non-MULTI_BANDS:
		   		 	b	b/g
 *  Infra 	     		    	B(4)	G(12)
 *  Adhoc 	      		    	B(4)	B(4)
 */
/** 
 *  @brief Get Range Info
 *   
 *  @param dev                  A pointer to net_device structure
 *  @param info			A pointer to iw_request_info structure
 *  @param vwrq 		A pointer to iw_param structure
 *  @param extra		A pointer to extra data buf
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_get_range(struct net_device *dev, struct iw_request_info *info,
               struct iw_point *dwrq, char *extra)
{
    int i, j;
    wlan_private *priv = netdev_priv(dev);
    wlan_adapter *Adapter = priv->adapter;
    struct iw_range *range = (struct iw_range *) extra;
    CHANNEL_FREQ_POWER *cfp;
    WLAN_802_11_RATES rates;

    ENTER();

    dwrq->length = sizeof(struct iw_range);
    memset(range, 0, sizeof(struct iw_range));

    range->min_nwid = 0;
    range->max_nwid = 0;

    memset(rates, 0, sizeof(rates));
    range->num_bitrates = get_active_data_rates(Adapter, rates);
    if (range->num_bitrates > sizeof(rates))
        range->num_bitrates = sizeof(rates);

    for (i = 0; i < MIN(range->num_bitrates, IW_MAX_BITRATES) && rates[i]; i++) {
        range->bitrate[i] = (rates[i] & 0x7f) * 500000;
    }
    range->num_bitrates = i;
    PRINTM(INFO, "IW_MAX_BITRATES=%d num_bitrates=%d\n", IW_MAX_BITRATES,
           range->num_bitrates);

    range->num_frequency = 0;
    if (wlan_get_state_11d(priv) == ENABLE_11D &&
        Adapter->MediaConnectStatus == WlanMediaStateConnected) {
        u8 chan_no;
        u8 band;

        parsed_region_chan_11d_t *parsed_region_chan =
            &Adapter->parsed_region_chan;

        band = parsed_region_chan->band;
        PRINTM(INFO, "band=%d NoOfChan=%d\n", band,
               parsed_region_chan->NoOfChan);

        for (i = 0; (range->num_frequency < IW_MAX_FREQUENCIES)
             && (i < parsed_region_chan->NoOfChan); i++) {
            chan_no = parsed_region_chan->chanPwr[i].chan;
            PRINTM(INFO, "chan_no=%d\n", chan_no);
            range->freq[range->num_frequency].i = (long) chan_no;
            range->freq[range->num_frequency].m =
                (long) chan_2_freq(chan_no, band) * 100000;
            range->freq[range->num_frequency].e = 1;
            range->num_frequency++;
        }
    } else {
        for (j = 0; (range->num_frequency < IW_MAX_FREQUENCIES)
             && (j < sizeof(Adapter->region_channel)
                 / sizeof(Adapter->region_channel[0])); j++) {
            cfp = Adapter->region_channel[j].CFP;
            for (i = 0; (range->num_frequency < IW_MAX_FREQUENCIES)
                 && Adapter->region_channel[j].Valid
                 && cfp && (i < Adapter->region_channel[j].NrCFP); i++) {
                range->freq[range->num_frequency].i = (long) cfp->Channel;
                range->freq[range->num_frequency].m = (long) cfp->Freq * 100000;
                range->freq[range->num_frequency].e = 1;
                cfp++;
                range->num_frequency++;
            }
        }
    }

    PRINTM(INFO, "IW_MAX_FREQUENCIES=%d num_frequency=%d\n", IW_MAX_FREQUENCIES,
           range->num_frequency);

    range->num_channels = range->num_frequency;

    sort_channels(&range->freq[0], range->num_frequency);

    /* 
     * Set an indication of the max TCP throughput in bit/s that we can
     * expect using this interface 
     */
    if (i > 2)
        range->throughput = 5000 * 1000;
    else
        range->throughput = 1500 * 1000;

    range->min_rts = MRVDRV_RTS_MIN_VALUE;
    range->max_rts = MRVDRV_RTS_MAX_VALUE;
    range->min_frag = MRVDRV_FRAG_MIN_VALUE;
    range->max_frag = MRVDRV_FRAG_MAX_VALUE;

    range->encoding_size[0] = 5;
    range->encoding_size[1] = 13;
    range->num_encoding_sizes = 2;
    range->max_encoding_tokens = 4;

#define IW_POWER_PERIOD_MIN 1000000     /* 1 sec */
#define IW_POWER_PERIOD_MAX 120000000   /* 2 min */
#define IW_POWER_TIMEOUT_MIN 1000       /* 1 ms */
#define IW_POWER_TIMEOUT_MAX 1000000    /* 1 sec */

    /* Power Management duration & timeout */
    range->min_pmp = IW_POWER_PERIOD_MIN;
    range->max_pmp = IW_POWER_PERIOD_MAX;
    range->min_pmt = IW_POWER_TIMEOUT_MIN;
    range->max_pmt = IW_POWER_TIMEOUT_MAX;
    range->pmp_flags = IW_POWER_PERIOD;
    range->pmt_flags = IW_POWER_TIMEOUT;
    range->pm_capa = IW_POWER_PERIOD | IW_POWER_TIMEOUT | IW_POWER_ALL_R;

    /* 
     * Minimum version we recommend 
     */
    range->we_version_source = 15;

    /* 
     * Version we are compiled with 
     */
    range->we_version_compiled = WIRELESS_EXT;

    range->retry_capa = IW_RETRY_LIMIT;
    range->retry_flags = IW_RETRY_LIMIT | IW_RETRY_MAX;

    range->min_retry = TX_RETRY_MIN;
    range->max_retry = TX_RETRY_MAX;

    /* 
     * Set the qual, level and noise range values 
     */
    /* 
     * need to put the right values here 
     */
#define IW_MAX_QUAL_PERCENT 100
#define IW_AVG_QUAL_PERCENT 70
    range->max_qual.qual = IW_MAX_QUAL_PERCENT;
    range->max_qual.level = 0;
    range->max_qual.noise = 0;

    range->avg_qual.qual = IW_AVG_QUAL_PERCENT;
    range->avg_qual.level = 0;
    range->avg_qual.noise = 0;

    range->sensitivity = 0;
    /* 
     * Setup the supported power level ranges 
     */
    memset(range->txpower, 0, sizeof(range->txpower));
    range->txpower[0] = Adapter->MinTxPowerLevel;
    range->txpower[1] = Adapter->MaxTxPowerLevel;
    range->num_txpower = 2;
    range->txpower_capa = IW_TXPOW_DBM | IW_TXPOW_RANGE;

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief  Set power management 
 *   
 *  @param dev                  A pointer to net_device structure
 *  @param info			A pointer to iw_request_info structure
 *  @param vwrq 		A pointer to iw_param structure
 *  @param extra		A pointer to extra data buf
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_set_power(struct net_device *dev, struct iw_request_info *info,
               struct iw_param *vwrq, char *extra)
{
    wlan_private *priv = netdev_priv(dev);
    wlan_adapter *Adapter = priv->adapter;

    ENTER();

    if (!wlan_is_cmd_allowed(priv)) {
        PRINTM(MSG, "%s: not allowed\n", __FUNCTION__);
        LEAVE();
        return -EBUSY;
    }

    /* PS is currently supported only in Infrastructure Mode Remove this check 
       if it is to be supported in IBSS mode also */

    if (vwrq->disabled) {
        Adapter->PSMode = Wlan802_11PowerModeCAM;
        if (Adapter->PSState != PS_STATE_FULL_POWER) {
            wlan_exit_ps(priv, HostCmd_OPTION_WAITFORRSP);
        }

        LEAVE();
        return 0;
    }

    if ((vwrq->flags & IW_POWER_TYPE) == IW_POWER_TIMEOUT) {
        PRINTM(INFO, "Setting power timeout command is not supported\n");
        LEAVE();
        return -EINVAL;
    } else if ((vwrq->flags & IW_POWER_TYPE) == IW_POWER_PERIOD) {
        PRINTM(INFO, "Setting power period command is not supported\n");
        LEAVE();
        return -EINVAL;
    }

    if (Adapter->PSMode != Wlan802_11PowerModeCAM) {
        LEAVE();
        return WLAN_STATUS_SUCCESS;
    }

    Adapter->PSMode = Wlan802_11PowerModeMAX_PSP;

    if (Adapter->MediaConnectStatus == WlanMediaStateConnected) {
        wlan_enter_ps(priv, HostCmd_OPTION_WAITFORRSP);
    }

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief  Get power management 
 *   
 *  @param dev                  A pointer to net_device structure
 *  @param info			A pointer to iw_request_info structure
 *  @param vwrq 		A pointer to iw_param structure
 *  @param extra		A pointer to extra data buf
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_get_power(struct net_device *dev, struct iw_request_info *info,
               struct iw_param *vwrq, char *extra)
{
    wlan_private *priv = netdev_priv(dev);
    wlan_adapter *Adapter = priv->adapter;
    int mode;

    ENTER();

    mode = Adapter->PSMode;

    if ((vwrq->disabled = (mode == Wlan802_11PowerModeCAM))
        || Adapter->MediaConnectStatus == WlanMediaStateDisconnected) {
        LEAVE();
        return WLAN_STATUS_SUCCESS;
    }

    vwrq->value = 0;

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief  Set sensitivity threshold
 *   
 *  @param dev                  A pointer to net_device structure
 *  @param info			A pointer to iw_request_info structure
 *  @param vwrq 		A pointer to iw_param structure
 *  @param extra		A pointer to extra data buf
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_set_sens(struct net_device *dev, struct iw_request_info *info,
              struct iw_param *vwrq, char *extra)
{
    ENTER();
    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief  Get sensitivity threshold
 *   
 *  @param dev                  A pointer to net_device structure
 *  @param info			A pointer to iw_request_info structure
 *  @param vwrq 		A pointer to iw_param structure
 *  @param extra		A pointer to extra data buf
 *  @return 	   		WLAN_STATUS_FAILURE
 */
static int
wlan_get_sens(struct net_device *dev,
              struct iw_request_info *info, struct iw_param *vwrq, char *extra)
{
    ENTER();
    LEAVE();
    return WLAN_STATUS_FAILURE;
}

#if (WIRELESS_EXT >= 18)
/** 
 *  @brief  Set IE 
 *
 *  Calls main function set_gen_ie_fuct that adds the inputted IE
 *    to the genie buffer
 *   
 *  @param dev          A pointer to net_device structure
 *  @param info         A pointer to iw_request_info structure
 *  @param dwrq         A pointer to iw_point structure
 *  @param extra        A pointer to extra data buf
 *  @return             WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_set_gen_ie(struct net_device *dev,
                struct iw_request_info *info,
                struct iw_point *dwrq, char *extra)
{
    int ret = WLAN_STATUS_SUCCESS;

    ENTER();
    ret = wlan_set_gen_ie_helper(netdev_priv(dev), dwrq->pointer, dwrq->length);

    LEAVE();
    return ret;
}

/** 
 *  @brief  Get IE 
 *
 *  Calls main function get_gen_ie_fuct that retrieves expected IEEE IEs
 *    and places then in the iw_point structure
 *   
 *  @param dev          A pointer to net_device structure
 *  @param info         A pointer to iw_request_info structure
 *  @param dwrq         A pointer to iw_point structure
 *  @param extra        A pointer to extra data buf
 *  @return             WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_get_gen_ie(struct net_device *dev,
                struct iw_request_info *info,
                struct iw_point *dwrq, char *extra)
{
    int ret = WLAN_STATUS_SUCCESS;

    ENTER();
    ret = wlan_get_gen_ie_helper(netdev_priv(dev), dwrq->pointer, &dwrq->length);
    LEAVE();
    return ret;
}

/** 
 *  @brief Set authentication mode
 *  @param priv                 A pointer to wlan_private structure
 *  @param req			A pointer to ifreq structure
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_setauthalg(wlan_private * priv, int alg)
{
    wlan_adapter *Adapter = priv->adapter;

    ENTER();

    PRINTM(INFO, "auth alg is %#x\n", alg);

    switch (alg) {
    case IW_AUTH_ALG_SHARED_KEY:
        Adapter->SecInfo.AuthenticationMode = Wlan802_11AuthModeShared;
        break;
    case IW_AUTH_ALG_LEAP:
        /* clear WPA IE */
        wlan_set_wpa_ie_helper(priv, NULL, 0);
        Adapter->SecInfo.AuthenticationMode = Wlan802_11AuthModeNetworkEAP;
        break;
    case IW_AUTH_ALG_OPEN_SYSTEM:
    default:
        Adapter->SecInfo.AuthenticationMode = Wlan802_11AuthModeOpen;
        break;
    }

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief set authentication mode params
 *   
 *  @param dev                  A pointer to net_device structure
 *  @param info			A pointer to iw_request_info structure
 *  @param vwrq 		A pointer to iw_param structure
 *  @param extra		A pointer to extra data buf
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_set_auth(struct net_device *dev, struct iw_request_info *info,
              struct iw_param *vwrq, char *extra)
{
    wlan_private *priv = netdev_priv(dev);

    ENTER();

    if (!wlan_is_cmd_allowed(priv)) {
        PRINTM(MSG, "%s: not allowed\n", __FUNCTION__);
        LEAVE();
        return -EBUSY;
    }
    switch (vwrq->flags & IW_AUTH_INDEX) {
    case IW_AUTH_CIPHER_PAIRWISE:
    case IW_AUTH_CIPHER_GROUP:
        if (vwrq->value & IW_AUTH_CIPHER_NONE)
            priv->adapter->SecInfo.EncryptionMode = CIPHER_NONE;
        else if (vwrq->value & IW_AUTH_CIPHER_WEP40)
            priv->adapter->SecInfo.EncryptionMode = CIPHER_WEP40;
        else if (vwrq->value & IW_AUTH_CIPHER_TKIP)
            priv->adapter->SecInfo.EncryptionMode = CIPHER_TKIP;
        else if (vwrq->value & IW_AUTH_CIPHER_CCMP)
            priv->adapter->SecInfo.EncryptionMode = CIPHER_CCMP;
        else if (vwrq->value & IW_AUTH_CIPHER_WEP104)
            priv->adapter->SecInfo.EncryptionMode = CIPHER_WEP104;
        break;
    case IW_AUTH_80211_AUTH_ALG:
        wlan_setauthalg(priv, vwrq->value);
        break;
    case IW_AUTH_WPA_ENABLED:
        if (vwrq->value == FALSE)
            wlan_set_wpa_ie_helper(priv, NULL, 0);
        break;
    case IW_AUTH_WPA_VERSION:
    case IW_AUTH_KEY_MGMT:
    case IW_AUTH_TKIP_COUNTERMEASURES:
    case IW_AUTH_DROP_UNENCRYPTED:
    case IW_AUTH_RX_UNENCRYPTED_EAPOL:
    case IW_AUTH_ROAMING_CONTROL:
    case IW_AUTH_PRIVACY_INVOKED:
    default:
        break;
    }

    LEAVE();

    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief  get authentication mode params 
 *   
 *  @param dev                  A pointer to net_device structure
 *  @param info			A pointer to iw_request_info structure
 *  @param vwrq 		A pointer to iw_param structure
 *  @param extra		A pointer to extra data buf
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_get_auth(struct net_device *dev, struct iw_request_info *info,
              struct iw_param *vwrq, char *extra)
{
    wlan_private *priv = netdev_priv(dev);
    wlan_adapter *Adapter = priv->adapter;
    int ret = WLAN_STATUS_SUCCESS;
    ENTER();
    if (!wlan_is_cmd_allowed(priv)) {
        PRINTM(MSG, "%s: not allowed\n", __FUNCTION__);
        LEAVE();
        return -EBUSY;
    }
    switch (vwrq->flags & IW_AUTH_INDEX) {
    case IW_AUTH_CIPHER_PAIRWISE:
    case IW_AUTH_CIPHER_GROUP:
        if (priv->adapter->SecInfo.EncryptionMode == CIPHER_NONE)
            vwrq->value = IW_AUTH_CIPHER_NONE;
        else if (priv->adapter->SecInfo.EncryptionMode == CIPHER_WEP40)
            vwrq->value = IW_AUTH_CIPHER_WEP40;
        else if (priv->adapter->SecInfo.EncryptionMode == CIPHER_TKIP)
            vwrq->value = IW_AUTH_CIPHER_TKIP;
        else if (priv->adapter->SecInfo.EncryptionMode == CIPHER_CCMP)
            vwrq->value = IW_AUTH_CIPHER_CCMP;
        else if (priv->adapter->SecInfo.EncryptionMode == CIPHER_WEP104)
            vwrq->value = IW_AUTH_CIPHER_WEP104;
        break;
    case IW_AUTH_80211_AUTH_ALG:
        if (Adapter->SecInfo.AuthenticationMode == Wlan802_11AuthModeShared)
            vwrq->value = IW_AUTH_ALG_SHARED_KEY;
        else if (Adapter->SecInfo.AuthenticationMode ==
                 Wlan802_11AuthModeNetworkEAP)
            vwrq->value = IW_AUTH_ALG_LEAP;
        else
            vwrq->value = IW_AUTH_ALG_OPEN_SYSTEM;
        break;
    case IW_AUTH_WPA_ENABLED:
        if (Adapter->Wpa_ie_len > 0)
            vwrq->value = TRUE;
        else
            vwrq->value = FALSE;
        break;
    case IW_AUTH_WPA_VERSION:
    case IW_AUTH_KEY_MGMT:
    case IW_AUTH_TKIP_COUNTERMEASURES:
    case IW_AUTH_DROP_UNENCRYPTED:
    case IW_AUTH_RX_UNENCRYPTED_EAPOL:
    case IW_AUTH_ROAMING_CONTROL:
    case IW_AUTH_PRIVACY_INVOKED:
    default:
        ret = -EOPNOTSUPP;
        break;
    }
    LEAVE();
    return ret;
}

/** 
 *  @brief  Request MLME operation 
 *
 *   
 *  @param dev          A pointer to net_device structure
 *  @param info         A pointer to iw_request_info structure
 *  @param dwrq         A pointer to iw_point structure
 *  @param extra        A pointer to extra data buf
 *  @return             WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_set_mlme(struct net_device *dev,
              struct iw_request_info *info, struct iw_point *dwrq, char *extra)
{
    struct iw_mlme *mlme = (struct iw_mlme *) extra;
    wlan_private *priv = netdev_priv(dev);

    ENTER();

    if (!wlan_is_cmd_allowed(priv)) {
        PRINTM(MSG, "%s: not allowed\n", __FUNCTION__);
        LEAVE();
        return -EBUSY;
    }
    if ((mlme->cmd == IW_MLME_DEAUTH) || (mlme->cmd == IW_MLME_DISASSOC)) {
        wlan_disconnect(priv);
    }
    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Set WPA key
 *   
 *  @param dev                  A pointer to net_device structure
 *  @param info			A pointer to iw_request_info structure
 *  @param vwrq 		A pointer to iw_param structure
 *  @param extra		A pointer to extra data buf
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_set_encode_wpa(struct net_device *dev,
                    struct iw_request_info *info,
                    struct iw_point *dwrq, char *extra)
{
    int ret = WLAN_STATUS_SUCCESS;
    wlan_private *priv = netdev_priv(dev);
    WLAN_802_11_KEY *pKey;
    wlan_adapter *Adapter = priv->adapter;

    ENTER();

    if (!wlan_is_cmd_allowed(priv)) {
        PRINTM(MSG, "%s: not allowed\n", __FUNCTION__);
        LEAVE();
        return -EBUSY;
    }

    pKey = (WLAN_802_11_KEY *) extra;

    HEXDUMP("Key buffer: ", extra, dwrq->length);

    /* current driver only supports key length of up to 32 bytes */
    if (pKey->KeyLength > MRVL_MAX_WPA_KEY_LENGTH) {
        PRINTM(INFO, " Error in key length \n");
        LEAVE();
        return WLAN_STATUS_FAILURE;
    }

    if (Adapter->InfrastructureMode == Wlan802_11IBSS) {
        /* IBSS/WPA-None uses only one key (Group) for both receiving and
           sending unicast and multicast packets. */
        /* Send the key as PTK to firmware */
        pKey->KeyIndex = 0x40000000;
        ret = wlan_prepare_cmd(priv,
                               HostCmd_CMD_802_11_KEY_MATERIAL,
                               HostCmd_ACT_GEN_SET,
                               HostCmd_OPTION_WAITFORRSP,
                               KEY_INFO_ENABLED, pKey);
        if (ret) {
            LEAVE();
            return ret;
        }
        /* Send the key as GTK to firmware */
        pKey->KeyIndex = ~0x40000000;
    }
    ret = wlan_prepare_cmd(priv,
                           HostCmd_CMD_802_11_KEY_MATERIAL,
                           HostCmd_ACT_GEN_SET,
                           HostCmd_OPTION_WAITFORRSP, KEY_INFO_ENABLED, pKey);

    if (ret) {
        LEAVE();
        return ret;
    }

    LEAVE();
    return ret;
}

/** 
 *  @brief  Extended version of encoding configuration 
 *
 *   
 *  @param dev          A pointer to net_device structure
 *  @param info         A pointer to iw_request_info structure
 *  @param dwrq         A pointer to iw_point structure
 *  @param extra        A pointer to extra data buf
 *  @return             WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_set_encode_ext(struct net_device *dev,
                    struct iw_request_info *info,
                    struct iw_point *dwrq, char *extra)
{
    struct iw_encode_ext *ext = (struct iw_encode_ext *) extra;
    wlan_private *priv = netdev_priv(dev);
    WLAN_802_11_KEY *pkey;
    int keyindex;
    u8 *pKeyMaterial = NULL;

    ENTER();

    if (!wlan_is_cmd_allowed(priv)) {
        PRINTM(MSG, "%s: not allowed\n", __FUNCTION__);
        LEAVE();
        return -EBUSY;
    }
    keyindex = dwrq->flags & IW_ENCODE_INDEX;
    if (keyindex > 4) {
        LEAVE();
        return -EINVAL;
    }
    if (ext->key_len > (dwrq->length - sizeof(struct iw_encode_ext))) {
        LEAVE();
        return -EINVAL;
    }
    pKeyMaterial = (u8 *) (ext + 1);
    /* Disable Key */
    if ((dwrq->flags & IW_ENCODE_DISABLED) && (ext->key_len == 0)) {
        dwrq->length = 0;
        wlan_set_encode_nonwpa(dev, info, dwrq, extra);
        LEAVE();
        return WLAN_STATUS_SUCCESS;
    }
    /* Set WEP key */
    if (ext->key_len <= MAX_WEP_KEY_SIZE) {
        dwrq->length = ext->key_len;
        wlan_set_encode_nonwpa(dev, info, dwrq, pKeyMaterial);
        if (ext->ext_flags & IW_ENCODE_EXT_SET_TX_KEY) {
            dwrq->length = 0;
            wlan_set_encode_nonwpa(dev, info, dwrq, extra);
        }
    } else {
        pkey = kmalloc(sizeof(WLAN_802_11_KEY) + ext->key_len, GFP_KERNEL);
        if (!pkey) {
            PRINTM(INFO, "allocate key buffer failed!\n");
            LEAVE();
            return -ENOMEM;
        }
        memset(pkey, 0, sizeof(WLAN_802_11_KEY) + ext->key_len);
        memcpy((u8 *) pkey->BSSID, (u8 *) ext->addr.sa_data, ETH_ALEN);
        pkey->KeyLength = ext->key_len;
        memcpy(pkey->KeyMaterial, pKeyMaterial, ext->key_len);
        pkey->KeyIndex = keyindex - 1;
        if (pkey->KeyIndex == 0)
            pkey->KeyIndex = 0x40000000;
        if (ext->ext_flags & IW_ENCODE_EXT_RX_SEQ_VALID)
            memcpy((u8 *) & pkey->KeyRSC, ext->rx_seq, IW_ENCODE_SEQ_MAX_SIZE);
        pkey->Length = sizeof(WLAN_802_11_KEY) + ext->key_len;
        wlan_set_encode_wpa(dev, info, dwrq, (u8 *) pkey);
        kfree(pkey);
    }

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief  Extended version of encoding configuration 
 *
 *   
 *  @param dev          A pointer to net_device structure
 *  @param info         A pointer to iw_request_info structure
 *  @param dwrq         A pointer to iw_point structure
 *  @param extra        A pointer to extra data buf
 *  @return             WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_get_encode_ext(struct net_device *dev,
                    struct iw_request_info *info,
                    struct iw_point *dwrq, char *extra)
{
    ENTER();
    LEAVE();
    return -EOPNOTSUPP;
}
#endif /* #if (WIRELESS_EXT >= 18) */

/** 
 *  @brief Set frequency
 *   
 *  @param dev 			A pointer to net_device structure
 *  @param info			A pointer to iw_request_info structure 
 *  @param fwrq			A pointer to iw_freq structure
 *  @param extra		A pointer to extra data buf
 *  @return 	   		WLAN_STATUS_SUCCESS--success, otherwise--fail
 */
static int
wlan_set_freq(struct net_device *dev, struct iw_request_info *info,
              struct iw_freq *fwrq, char *extra)
{
    int ret = WLAN_STATUS_SUCCESS;
    wlan_private *priv = netdev_priv(dev);
    wlan_adapter *Adapter = priv->adapter;
    int rc = -EINPROGRESS;      /* Call commit handler */
    CHANNEL_FREQ_POWER *cfp;

    ENTER();

    if (!wlan_is_cmd_allowed(priv)) {
        PRINTM(MSG, "%s: not allowed\n", __FUNCTION__);
        LEAVE();
        return -EBUSY;
    }
    /* 
     * If setting by frequency, convert to a channel 
     */
    if (fwrq->e == 1) {

        long f = fwrq->m / 100000;
        int c = 0;

        cfp = find_cfp_by_band_and_freq(Adapter, 0, f);
        if (!cfp) {
            cfp = find_cfp_by_band_and_freq(Adapter, BAND_A, f);
            if (cfp)
                Adapter->adhoc_start_band = BAND_A;
        } else
            Adapter->adhoc_start_band = BAND_G;
        if (!cfp) {
            PRINTM(INFO, "Invalid freq=%ld\n", f);
            LEAVE();
            return -EINVAL;
        }

        c = (int) cfp->Channel;

        if (c < 0) {
            LEAVE();
            return -EINVAL;
        }
        fwrq->e = 0;
        fwrq->m = c;
    }

    /* 
     * Setting by channel number 
     */
    if (fwrq->m > 1000 || fwrq->e > 0) {
        rc = -EOPNOTSUPP;
    } else {
        int channel = fwrq->m;

        cfp = find_cfp_by_band_and_channel(Adapter, 0, (u16) channel);
        if (!cfp) {
            cfp = find_cfp_by_band_and_channel(Adapter, BAND_A, (u16) channel);
            if (cfp)
                Adapter->adhoc_start_band = BAND_A;
        } else
            Adapter->adhoc_start_band = BAND_G;
        if (!cfp) {
            rc = -EINVAL;
        } else {
            if (Adapter->InfrastructureMode != Wlan802_11IBSS) {
                Adapter->AdhocChannel = channel;
                Adapter->AdhocAutoSel = FALSE;
                LEAVE();
                return ret;
            }
            rc = wlan_change_adhoc_chan(priv, channel);
            /* If station is WEP enabled, send the command to set WEP in
               firmware */
            if (Adapter->SecInfo.WEPStatus == Wlan802_11WEPEnabled) {
                PRINTM(INFO, "set_freq: WEP Enabled\n");
                ret = wlan_prepare_cmd(priv,
                                       HostCmd_CMD_802_11_SET_WEP,
                                       HostCmd_ACT_ADD,
                                       HostCmd_OPTION_WAITFORRSP, 0, NULL);

                if (ret) {
                    LEAVE();
                    return ret;
                }
                Adapter->CurrentPacketFilter |= HostCmd_ACT_MAC_WEP_ENABLE;

                wlan_prepare_cmd(priv,
                                 HostCmd_CMD_MAC_CONTROL,
                                 0, HostCmd_OPTION_WAITFORRSP,
                                 0, &Adapter->CurrentPacketFilter);
            }
        }
    }

    LEAVE();
    return rc;
}

/** 
 *  @brief set data rate
 *   
 *  @param dev                  A pointer to net_device structure
 *  @param info			A pointer to iw_request_info structure
 *  @param vwrq 		A pointer to iw_param structure
 *  @param extra		A pointer to extra data buf
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */

static int
wlan_set_rate(struct net_device *dev, struct iw_request_info *info,
              struct iw_param *vwrq, char *extra)
{
    wlan_private *priv = netdev_priv(dev);
    wlan_adapter *Adapter = priv->adapter;
    u32 data_rate;
    int ret = WLAN_STATUS_SUCCESS;
    WLAN_802_11_RATES rates;
    u8 *rate;
    int i = 0;

    ENTER();

    if (!wlan_is_cmd_allowed(priv)) {
        PRINTM(MSG, "%s: not allowed\n", __FUNCTION__);
        LEAVE();
        return -EBUSY;
    }

    PRINTM(INFO, "Vwrq->value = %d\n", vwrq->value);

    if (vwrq->value == -1) {
        Adapter->DataRate = 0;
        Adapter->RateBitmap = 0;
        memset(rates, 0, sizeof(rates));
        get_active_data_rates(Adapter, rates);
        rate = rates;
        while (*rate) {
            Adapter->RateBitmap |= 1 << (data_rate_to_index(*rate & 0x7f));
            rate++;
        }
        Adapter->Is_DataRate_Auto = TRUE;
    } else {
        if ((vwrq->value % 500000)) {
            LEAVE();
            return -EINVAL;
        }

        data_rate = vwrq->value / 500000;

        memset(rates, 0, sizeof(rates));
        get_active_data_rates(Adapter, rates);
        rate = rates;
        for (i = 0; (rate[i] && i < WLAN_SUPPORTED_RATES); i++) {
            PRINTM(INFO, "Rate=0x%X  Wanted=0x%X\n", *rate, data_rate);
            if ((rate[i] & 0x7f) == (data_rate & 0x7f))
                break;
        }
        if (!rate[i] || (i == WLAN_SUPPORTED_RATES)) {
            PRINTM(MSG, "The fixed data rate 0x%X is out "
                   "of range\n", data_rate);
            LEAVE();
            return -EINVAL;
        }

        Adapter->DataRate = data_rate;
        Adapter->RateBitmap = 1 << (data_rate_to_index(Adapter->DataRate));
        Adapter->Is_DataRate_Auto = FALSE;
    }

    ret = wlan_prepare_cmd(priv,
                           HostCmd_CMD_802_11_RATE_ADAPT_RATESET,
                           HostCmd_ACT_GEN_SET, HostCmd_OPTION_WAITFORRSP,
                           0, NULL);

    LEAVE();
    return ret;
}

/** 
 *  @brief get data rate
 *   
 *  @param dev                  A pointer to net_device structure
 *  @param info			A pointer to iw_request_info structure
 *  @param vwrq 		A pointer to iw_param structure
 *  @param extra		A pointer to extra data buf
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_get_rate(struct net_device *dev, struct iw_request_info *info,
              struct iw_param *vwrq, char *extra)
{
    wlan_private *priv = netdev_priv(dev);
    wlan_adapter *Adapter = priv->adapter;
    int ret = WLAN_STATUS_SUCCESS;

    ENTER();

    if (!wlan_is_cmd_allowed(priv)) {
        PRINTM(MSG, "%s: not allowed\n", __FUNCTION__);
        LEAVE();
        return -EBUSY;
    }
    if (Adapter->MediaConnectStatus != WlanMediaStateConnected) {
        switch (Adapter->config_bands) {
        case BAND_B:
        case BAND_G:
        case BAND_B | BAND_G:
        case BAND_A | BAND_B | BAND_G:
            /* return the lowest supported rate for G band */
            vwrq->value = ((SupportedRates_G[0] & 0x7f) * 500000);
            break;
        case BAND_A:
            /* return the lowest supported rate for A band */
            vwrq->value = ((SupportedRates_A[0] & 0x7f) * 500000);
            break;
        default:
            PRINTM(MSG, "Invalid Band\n");
            break;
        }
        LEAVE();
        return WLAN_STATUS_SUCCESS;
    }

    if (Adapter->Is_DataRate_Auto)
        vwrq->fixed = 0;
    else
        vwrq->fixed = 1;

    Adapter->TxRate = 0;

    ret = wlan_prepare_cmd(priv, HostCmd_CMD_802_11_TX_RATE_QUERY,
                           HostCmd_ACT_GEN_GET, HostCmd_OPTION_WAITFORRSP,
                           0, NULL);
    if (ret) {
        LEAVE();
        return ret;
    }
    vwrq->value = index_to_data_rate(Adapter->TxRate) * 500000;

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief set wireless mode 
 *   
 *  @param dev                  A pointer to net_device structure
 *  @param info			A pointer to iw_request_info structure
 *  @param uwrq 		Wireless mode to set
 *  @param extra		A pointer to extra data buf
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_set_mode(struct net_device *dev,
              struct iw_request_info *info, u32 * uwrq, char *extra)
{
    int ret = WLAN_STATUS_SUCCESS;
    wlan_private *priv = netdev_priv(dev);
    wlan_adapter *Adapter = priv->adapter;

    WLAN_802_11_NETWORK_INFRASTRUCTURE WantedMode;

    ENTER();

    if (!wlan_is_cmd_allowed(priv)) {
        PRINTM(MSG, "%s: not allowed\n", __FUNCTION__);
        LEAVE();
        return -EBUSY;
    }

    switch (*uwrq) {
    case IW_MODE_ADHOC:
        PRINTM(INFO, "Wanted Mode is ad-hoc: current DataRate=%#x\n",
               Adapter->DataRate);
        WantedMode = Wlan802_11IBSS;
        break;

    case IW_MODE_INFRA:
        PRINTM(INFO, "Wanted Mode is Infrastructure\n");
        WantedMode = Wlan802_11Infrastructure;
        break;

    case IW_MODE_AUTO:
        PRINTM(INFO, "Wanted Mode is Auto\n");
        WantedMode = Wlan802_11AutoUnknown;
        break;

    default:
        PRINTM(INFO, "Wanted Mode is Unknown: 0x%x\n", *uwrq);
        LEAVE();
        return -EINVAL;
    }

    if (Adapter->InfrastructureMode == WantedMode ||
        WantedMode == Wlan802_11AutoUnknown) {
        PRINTM(INFO, "Already set to required mode! No change!\n");

        Adapter->InfrastructureMode = WantedMode;

        LEAVE();
        return WLAN_STATUS_SUCCESS;
    }

    if (Adapter->InfrastructureMode == Wlan802_11Infrastructure) {
        if (Adapter->PSState != PS_STATE_FULL_POWER) {
            wlan_exit_ps(priv, HostCmd_OPTION_WAITFORRSP);
        }
        Adapter->PSMode = Wlan802_11PowerModeCAM;
    }

    ret = wlan_disconnect(priv);
    if (ret) {
        LEAVE();
        return ret;
    }

    Adapter->SecInfo.AuthenticationMode = Wlan802_11AuthModeOpen;

    Adapter->InfrastructureMode = WantedMode;

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief set tx power 
 *   
 *  @param dev                  A pointer to net_device structure
 *  @param info			A pointer to iw_request_info structure
 *  @param vwrq 		A pointer to iw_param structure
 *  @param extra		A pointer to extra data buf
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_set_txpow(struct net_device *dev, struct iw_request_info *info,
               struct iw_param *vwrq, char *extra)
{
    wlan_private *priv = netdev_priv(dev);
    wlan_adapter *Adapter = priv->adapter;
    int ret = WLAN_STATUS_SUCCESS;

    u16 dbm;

    ENTER();

    if (!wlan_is_cmd_allowed(priv)) {
        PRINTM(MSG, "%s: not allowed\n", __FUNCTION__);
        LEAVE();
        return -EBUSY;
    }

    if (vwrq->disabled) {
        wlan_radio_ioctl(priv, RADIO_OFF);
        LEAVE();
        return WLAN_STATUS_SUCCESS;
    }

    wlan_radio_ioctl(priv, RADIO_ON);

    dbm = (u16) vwrq->value;

    if ((dbm < Adapter->MinTxPowerLevel) || (dbm > Adapter->MaxTxPowerLevel)) {
        PRINTM(MSG,
               "The set txpower value %d dBm is out of range (%d dBm-%d dBm)!\n",
               dbm, Adapter->MinTxPowerLevel, Adapter->MaxTxPowerLevel);
        LEAVE();
        return -EINVAL;
    }

    /* auto tx power control */

    if (vwrq->fixed == 0)
        dbm = 0xffff;

    PRINTM(INFO, "<1>TXPOWER SET %d dbm\n", dbm);

    ret = wlan_prepare_cmd(priv,
                           HostCmd_CMD_802_11_RF_TX_POWER,
                           HostCmd_ACT_GEN_SET,
                           HostCmd_OPTION_WAITFORRSP, 0, (void *) &dbm);

    LEAVE();
    return ret;
}

/** 
 *  @brief Get current essid 
 *   
 *  @param dev      A pointer to net_device structure
 *  @param info     A pointer to iw_request_info structure
 *  @param dwrq     A pointer to iw_point structure
 *  @param extra    A pointer to extra data buf
 *  @return         WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_get_essid(struct net_device *dev, struct iw_request_info *info,
               struct iw_point *dwrq, char *extra)
{
    wlan_private *priv = netdev_priv(dev);
    wlan_adapter *Adapter = priv->adapter;
    int tblIdx = -1;
    BSSDescriptor_t *pBSSDesc;

    ENTER();

    pBSSDesc = &Adapter->CurBssParams.BSSDescriptor;

    /* 
     * Get the current SSID 
     */
    if (Adapter->MediaConnectStatus == WlanMediaStateConnected) {

        tblIdx = wlan_find_ssid_in_list(Adapter,
                                        &pBSSDesc->Ssid,
                                        pBSSDesc->MacAddress,
                                        Adapter->InfrastructureMode);

        dwrq->length = MIN(dwrq->length, pBSSDesc->Ssid.SsidLength);
        memcpy(extra, &pBSSDesc->Ssid.Ssid, dwrq->length);

    } else {
        dwrq->length = 0;
    }

    /* If the current network is in the table, return the table index */
    if (tblIdx >= 0) {
        dwrq->flags = (tblIdx + 1) & IW_ENCODE_INDEX;
    } else {
        dwrq->flags = 1;
    }

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Set Encryption key
 *   
 *  @param dev      A pointer to net_device structure
 *  @param info     A pointer to iw_request_info structure
 *  @param dwrq     A pointer to iw_point structure
 *  @param extra    A pointer to extra data buf
 *  @return         WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_set_encode(struct net_device *dev,
                struct iw_request_info *info,
                struct iw_point *dwrq, char *extra)
{

    WLAN_802_11_KEY *pKey = NULL;
    int retval = -EINVAL;

    ENTER();

    if (dwrq->length > MAX_WEP_KEY_SIZE) {
        pKey = (WLAN_802_11_KEY *) extra;
        if (pKey->KeyLength <= MAX_WEP_KEY_SIZE) {
            /* dynamic WEP */
            dwrq->length = pKey->KeyLength;
            dwrq->flags = pKey->KeyIndex + 1;
            retval = wlan_set_encode_nonwpa(dev, info, dwrq, pKey->KeyMaterial);
        }
    } else {
        /* static WEP */
        PRINTM(INFO, "Setting WEP\n");
        retval = wlan_set_encode_nonwpa(dev, info, dwrq, extra);
    }

    LEAVE();

    return retval;
}

/*
 * iwconfig settable callbacks 
 */
static const iw_handler wlan_handler[] = {
    (iw_handler) wlan_config_commit,    /* SIOCSIWCOMMIT */
    (iw_handler) wlan_get_name, /* SIOCGIWNAME */
    (iw_handler) NULL,          /* SIOCSIWNWID */
    (iw_handler) NULL,          /* SIOCGIWNWID */
    (iw_handler) wlan_set_freq, /* SIOCSIWFREQ */
    (iw_handler) wlan_get_freq, /* SIOCGIWFREQ */
    (iw_handler) wlan_set_mode, /* SIOCSIWMODE */
    (iw_handler) wlan_get_mode, /* SIOCGIWMODE */
    (iw_handler) wlan_set_sens, /* SIOCSIWSENS */
    (iw_handler) wlan_get_sens, /* SIOCGIWSENS */
    (iw_handler) NULL,          /* SIOCSIWRANGE */
    (iw_handler) wlan_get_range,        /* SIOCGIWRANGE */
    (iw_handler) NULL,          /* SIOCSIWPRIV */
    (iw_handler) NULL,          /* SIOCGIWPRIV */
    (iw_handler) NULL,          /* SIOCSIWSTATS */
    (iw_handler) NULL,          /* SIOCGIWSTATS */
#if WIRELESS_EXT > 15
    iw_handler_set_spy,         /* SIOCSIWSPY */
    iw_handler_get_spy,         /* SIOCGIWSPY */
    iw_handler_set_thrspy,      /* SIOCSIWTHRSPY */
    iw_handler_get_thrspy,      /* SIOCGIWTHRSPY */
#else /* WIRELESS_EXT > 15 */
#ifdef WIRELESS_SPY
    (iw_handler) wlan_set_spy,  /* SIOCSIWSPY */
    (iw_handler) wlan_get_spy,  /* SIOCGIWSPY */
#else /* WIRELESS_SPY */
    (iw_handler) NULL,          /* SIOCSIWSPY */
    (iw_handler) NULL,          /* SIOCGIWSPY */
#endif /* WIRELESS_SPY */
    (iw_handler) NULL,          /* -- hole -- */
    (iw_handler) NULL,          /* -- hole -- */
#endif /* WIRELESS_EXT > 15 */
    (iw_handler) wlan_set_wap,  /* SIOCSIWAP */
    (iw_handler) wlan_get_wap,  /* SIOCGIWAP */
#if WIRELESS_EXT >= 18
    (iw_handler) wlan_set_mlme, /* SIOCSIWMLME */
#else
    (iw_handler) NULL,          /* -- hole -- */
#endif
    /* (iw_handler) wlan_get_aplist, *//* SIOCGIWAPLIST */
    NULL,                       /* SIOCGIWAPLIST */
#if WIRELESS_EXT > 13
    (iw_handler) wlan_set_scan, /* SIOCSIWSCAN */
    (iw_handler) wlan_get_scan, /* SIOCGIWSCAN */
#else /* WIRELESS_EXT > 13 */
    (iw_handler) NULL,          /* SIOCSIWSCAN */
    (iw_handler) NULL,          /* SIOCGIWSCAN */
#endif /* WIRELESS_EXT > 13 */
    (iw_handler) wlan_set_essid,        /* SIOCSIWESSID */
    (iw_handler) wlan_get_essid,        /* SIOCGIWESSID */
    (iw_handler) wlan_set_nick, /* SIOCSIWNICKN */
    (iw_handler) wlan_get_nick, /* SIOCGIWNICKN */
    (iw_handler) NULL,          /* -- hole -- */
    (iw_handler) NULL,          /* -- hole -- */
    (iw_handler) wlan_set_rate, /* SIOCSIWRATE */
    (iw_handler) wlan_get_rate, /* SIOCGIWRATE */
    (iw_handler) wlan_set_rts,  /* SIOCSIWRTS */
    (iw_handler) wlan_get_rts,  /* SIOCGIWRTS */
    (iw_handler) wlan_set_frag, /* SIOCSIWFRAG */
    (iw_handler) wlan_get_frag, /* SIOCGIWFRAG */
    (iw_handler) wlan_set_txpow,        /* SIOCSIWTXPOW */
    (iw_handler) wlan_get_txpow,        /* SIOCGIWTXPOW */
    (iw_handler) wlan_set_retry,        /* SIOCSIWRETRY */
    (iw_handler) wlan_get_retry,        /* SIOCGIWRETRY */
    (iw_handler) wlan_set_encode,       /* SIOCSIWENCODE */
    (iw_handler) wlan_get_encode,       /* SIOCGIWENCODE */
    (iw_handler) wlan_set_power,        /* SIOCSIWPOWER */
    (iw_handler) wlan_get_power,        /* SIOCGIWPOWER */
#if (WIRELESS_EXT >= 18)
    (iw_handler) NULL,          /* -- hole -- */
    (iw_handler) NULL,          /* -- hole -- */
    (iw_handler) wlan_set_gen_ie,       /* SIOCSIWGENIE */
    (iw_handler) wlan_get_gen_ie,       /* SIOCGIWGENIE */
    (iw_handler) wlan_set_auth, /* SIOCSIWAUTH */
    (iw_handler) wlan_get_auth, /* SIOCGIWAUTH */
    (iw_handler) wlan_set_encode_ext,   /* SIOCSIWENCODEEXT */
    (iw_handler) wlan_get_encode_ext,   /* SIOCGIWENCODEEXT */
#endif /* WIRELESSS_EXT >= 18 */
    (iw_handler) NULL,		/* SIOCSIWPMKSA */
};

/**
 * iwpriv settable callbacks 
 */
static const iw_handler wlan_private_handler[] = {
    NULL,                       /* SIOCIWFIRSTPRIV */
};

/** wlan_handler_def */
struct iw_handler_def wlan_handler_def = {
  num_standard:sizeof(wlan_handler) / sizeof(iw_handler),
  num_private:sizeof(wlan_private_handler) / sizeof(iw_handler),
  num_private_args:sizeof(wlan_private_args) / sizeof(struct iw_priv_args),
  standard:(iw_handler *) wlan_handler,
  private:(iw_handler *) wlan_private_handler,
  private_args:(struct iw_priv_args *) wlan_private_args,
#if WIRELESS_EXT > 20
  get_wireless_stats:wlan_get_wireless_stats,
#endif
};

/********************************************************
		Global Functions
********************************************************/
/** 
 *  @brief This function checks if the command is allowed.
 * 
 *  @param priv		A pointer to wlan_private structure
 *  @return		TRUE or FALSE
 */
BOOLEAN
wlan_is_cmd_allowed(wlan_private * priv)
{
    BOOLEAN ret = TRUE;

    ENTER();

    if (priv->adapter->bHostSleepConfigured) {
        PRINTM(INFO, "IOCTLS called when WLAN access is blocked\n");
        ret = FALSE;
    }
    if (!priv->adapter->IsAutoDeepSleepEnabled) {
        if ((priv->adapter->IsDeepSleep == TRUE)) {
            PRINTM(INFO, "IOCTLS called when station is in DeepSleep\n");
            ret = FALSE;
        }
    }

    LEAVE();

    return ret;
}

/** 
 *  @brief get the channel frequency power info with specific channel
 *   
 *  @param band	 		it can be BAND_A, BAND_G or BAND_B
 *  @param channel       	the channel for looking	
 *  @param region_channel 	A pointer to REGION_CHANNEL structure
 *  @return 	   			A pointer to CHANNEL_FREQ_POWER structure or NULL if not find.
 */

CHANNEL_FREQ_POWER *
get_cfp_by_band_and_channel(u8 band, u16 channel,
                            REGION_CHANNEL * region_channel)
{
    REGION_CHANNEL *rc;
    CHANNEL_FREQ_POWER *cfp = NULL;
    int i, j;

    for (j = 0; !cfp && (j < MAX_REGION_CHANNEL_NUM); j++) {
        rc = &region_channel[j];

        if (!rc->Valid || !rc->CFP)
            continue;
        switch (rc->Band) {
        case BAND_A:
            switch (band) {
            case BAND_A:       /* matching BAND_A */
                break;
            default:
                continue;
            }
            break;
        case BAND_B:
        case BAND_G:
            switch (band) {
            case BAND_B:       /* matching BAND_B/G */
            case BAND_G:
            case 0:
                break;
            default:
                continue;
            }
            break;
        default:
            continue;
        }
        if (channel == FIRST_VALID_CHANNEL)
            cfp = &rc->CFP[0];
        else {
            for (i = 0; i < rc->NrCFP; i++) {
                if (rc->CFP[i].Channel == channel) {
                    cfp = &rc->CFP[i];
                    break;
                }
            }
        }
    }

    if (!cfp && channel)
        PRINTM(INFO, "get_cfp_by_band_and_channel(): cannot find "
               "cfp by band %d & channel %d\n", band, channel);

    LEAVE();
    return cfp;
}

#if WIRELESS_EXT > 14
/** 
 *  @brief This function sends customized event to application.
 *  
 *  @param priv    A pointer to wlan_private structure
 *  @para str	   A pointer to event string
 *  @return 	   n/a
 */
void
send_iwevcustom_event(wlan_private * priv, s8 * str)
{
    union iwreq_data iwrq;
    u8 buf[50];

    ENTER();

    memset(&iwrq, 0, sizeof(union iwreq_data));
    memset(buf, 0, sizeof(buf));

    snprintf(buf, sizeof(buf) - 1, "%s", str);

    iwrq.data.pointer = buf;
    iwrq.data.length = strlen(buf) + 1 + IW_EV_LCP_LEN;

    /* Send Event to upper layer */
    wireless_send_event(priv->wlan_dev.netdev, IWEVCUSTOM, &iwrq, buf);
    PRINTM(INFO, "Wireless event %s is sent to application\n", str);

    LEAVE();
    return;
}
#endif

/** 
 *  @brief Find the channel frequency power info with specific channel
 *   
 *  @param adapter 	A pointer to wlan_adapter structure
 *  @param band		it can be BAND_A, BAND_G or BAND_B
 *  @param channel      the channel for looking	
 *  @return 	   	A pointer to CHANNEL_FREQ_POWER structure or NULL if not find.
 */
CHANNEL_FREQ_POWER *
find_cfp_by_band_and_channel(wlan_adapter * adapter, u8 band, u16 channel)
{
    CHANNEL_FREQ_POWER *cfp = NULL;

    ENTER();

    if (adapter->State11D.Enable11D == ENABLE_11D)
        cfp =
            get_cfp_by_band_and_channel(band, channel,
                                        adapter->universal_channel);
    else
        cfp =
            get_cfp_by_band_and_channel(band, channel, adapter->region_channel);

    return cfp;
}

/** 
 *  @brief Change Adhoc Channel
 *   
 *  @param priv 		A pointer to wlan_private structure
 *  @param channel		The channel to be set. 
 *  @return 	   		WLAN_STATUS_SUCCESS--success, WLAN_STATUS_FAILURE--fail
 */
int
wlan_change_adhoc_chan(wlan_private * priv, int channel)
{
    int ret = WLAN_STATUS_SUCCESS;
    wlan_adapter *Adapter = priv->adapter;

    ENTER();
    Adapter->AdhocChannel = channel;

    wlan_update_current_chan(priv);

    if (Adapter->CurBssParams.BSSDescriptor.Channel == Adapter->AdhocChannel) {
        /* AdhocChannel is set to the current Channel already */
        LEAVE();
        return WLAN_STATUS_SUCCESS;
    }

    PRINTM(INFO, "Updating Channel from %d to %d\n",
           Adapter->CurBssParams.BSSDescriptor.Channel, Adapter->AdhocChannel);

    wlan_set_current_chan(priv, Adapter->AdhocChannel);

    wlan_update_current_chan(priv);

    if (Adapter->CurBssParams.BSSDescriptor.Channel != Adapter->AdhocChannel) {
        PRINTM(INFO, "Failed to updated Channel to %d, channel = %d\n",
               Adapter->AdhocChannel,
               Adapter->CurBssParams.BSSDescriptor.Channel);
        LEAVE();
        return WLAN_STATUS_FAILURE;
    }

    if (Adapter->MediaConnectStatus == WlanMediaStateConnected) {
        int i;
        WLAN_802_11_SSID curAdhocSsid;

        PRINTM(INFO, "Channel Changed while in an IBSS\n");

        /* Copy the current ssid */
        memcpy(&curAdhocSsid,
               &Adapter->CurBssParams.BSSDescriptor.Ssid,
               sizeof(WLAN_802_11_SSID));

        /* Exit Adhoc mode */
        PRINTM(INFO, "In wlan_change_adhoc_chan(): Sending Adhoc Stop\n");
        if ((ret = wlan_disconnect(priv))) {
            LEAVE();
            return ret;
        }
        Adapter->AdhocAutoSel = FALSE;
        /* Scan for the network */
        wlan_cmd_specific_scan_ssid(priv, &curAdhocSsid);

        /* find out the BSSID that matches the current SSID */
        i = wlan_find_ssid_in_list(Adapter, &curAdhocSsid, NULL,
                                   Wlan802_11IBSS);

        if (i >= 0) {
            PRINTM(INFO, "SSID found at %d in List," "so join\n", i);
            wlan_join_adhoc(priv, &Adapter->ScanTable[i]);
        } else {
            /* else send START command */
            PRINTM(INFO, "SSID not found in list, "
                   "so creating adhoc with ssid = %s\n", curAdhocSsid.Ssid);
            wlan_start_adhoc(priv, &curAdhocSsid);
        }                       /* end of else (START command) */
    }

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Set/Get WPA IE   
 *
 *  @param priv         A pointer to wlan_private structure
 *  @param ie_data_ptr  A pointer to IE
 *  @param ie_len       Length of the IE
 *
 *  @return             WLAN_STATUS_SUCCESS --success, otherwise fail
 */
int
wlan_set_wpa_ie_helper(wlan_private * priv, u8 * ie_data_ptr, u16 ie_len)
{
    wlan_adapter *Adapter = priv->adapter;
    int ret = WLAN_STATUS_SUCCESS;

    ENTER();

    if (ie_len) {
        if (ie_len > sizeof(Adapter->Wpa_ie)) {
            PRINTM(INFO, "failed to copy WPA IE, too big \n");
            LEAVE();
            return -EFAULT;
        }
        if (copy_from_user(Adapter->Wpa_ie, ie_data_ptr, ie_len)) {
            PRINTM(INFO, "failed to copy WPA IE \n");
            LEAVE();
            return -EFAULT;
        }
        Adapter->Wpa_ie_len = ie_len;
        PRINTM(INFO, "Set Wpa_ie_len=%d IE=%#x\n",
               Adapter->Wpa_ie_len, Adapter->Wpa_ie[0]);
        HEXDUMP("Wpa_ie", Adapter->Wpa_ie, Adapter->Wpa_ie_len);

        if (Adapter->Wpa_ie[0] == WPA_IE) {
            Adapter->SecInfo.WPAEnabled = TRUE;
        } else if (Adapter->Wpa_ie[0] == RSN_IE) {
            Adapter->SecInfo.WPA2Enabled = TRUE;
        } else {
            Adapter->SecInfo.WPAEnabled = FALSE;
            Adapter->SecInfo.WPA2Enabled = FALSE;
        }
    } else {
        memset(Adapter->Wpa_ie, 0, sizeof(Adapter->Wpa_ie));
        Adapter->Wpa_ie_len = ie_len;
        PRINTM(INFO, "Reset Wpa_ie_len=%d IE=%#x\n",
               Adapter->Wpa_ie_len, Adapter->Wpa_ie[0]);
        Adapter->SecInfo.WPAEnabled = FALSE;
        Adapter->SecInfo.WPA2Enabled = FALSE;
    }

    LEAVE();
    return ret;
}

/** 
 *  @brief Get active data rates
 *   
 *  @param Adapter          A pointer to wlan_adapter structure
 *  @param rates            The buf to return the active rates
 *  @return                 The number of Rates
 */
int
get_active_data_rates(wlan_adapter * Adapter, WLAN_802_11_RATES rates)
{
    int k = 0;

    ENTER();

    if (Adapter->MediaConnectStatus != WlanMediaStateConnected) {
        if (Adapter->InfrastructureMode == Wlan802_11Infrastructure) {
            /* Infra. mode */
            if (Adapter->config_bands & BAND_G) {
                PRINTM(INFO, "Infra mBAND_G=%#x\n", BAND_G);
                k = wlan_copy_rates(rates, k, SupportedRates_G,
                                    sizeof(SupportedRates_G));
            } else {
                if (Adapter->config_bands & BAND_B) {
                    PRINTM(INFO, "Infra mBAND_B=%#x\n", BAND_B);
                    k = wlan_copy_rates(rates, k,
                                        SupportedRates_B,
                                        sizeof(SupportedRates_B));
                }
                if (Adapter->config_bands & BAND_A) {
                    PRINTM(INFO, "Infra mBAND_A=%#x\n", BAND_A);
                    k = wlan_copy_rates(rates, k,
                                        SupportedRates_A,
                                        sizeof(SupportedRates_A));
                }
            }
        } else {
            /* ad-hoc mode */
            if (Adapter->config_bands & BAND_G) {
                PRINTM(INFO, "Adhoc mBAND_G=%#x\n", BAND_G);
                k = wlan_copy_rates(rates, k, AdhocRates_G,
                                    sizeof(AdhocRates_G));
            } else {
                if (Adapter->config_bands & BAND_B) {
                    PRINTM(INFO, "Adhoc mBAND_B=%#x\n", BAND_B);
                    k = wlan_copy_rates(rates, k, AdhocRates_B,
                                        sizeof(AdhocRates_B));
                }
                if (Adapter->config_bands & BAND_A) {
                    PRINTM(INFO, "Adhoc mBAND_A=%#x\n", BAND_A);
                    k = wlan_copy_rates(rates, k, AdhocRates_A,
                                        sizeof(AdhocRates_A));
                }
            }
        }
    } else {
        k = wlan_copy_rates(rates, 0, Adapter->CurBssParams.DataRates,
                            Adapter->CurBssParams.NumOfRates);
    }

    LEAVE();

    return k;
}

/** 
 *  @brief  Append/Reset IE buffer. 
 *   
 *  Pass an opaque block of data, expected to be IEEE IEs, to the driver 
 *    for eventual passthrough to the firmware in an associate/join 
 *    (and potentially start) command.  This function is the main body
 *    for both wlan_set_gen_ie_ioctl and wlan_set_gen_ie
 *
 *  Data is appended to an existing buffer and then wrapped in a passthrough
 *    TLV in the command API to the firmware.  The firmware treats the data
 *    as a transparent passthrough to the transmitted management frame.
 *
 *  @param priv         A pointer to wlan_private structure
 *  @param ie_data_ptr  A pointer to iwreq structure
 *  @param ie_len       Length of the IE or IE block passed in ie_data_ptr
 *
 *  @return             WLAN_STATUS_SUCCESS --success, otherwise fail
 */
int
wlan_set_gen_ie_helper(wlan_private * priv, u8 * ie_data_ptr, u16 ie_len)
{
    int ret = WLAN_STATUS_SUCCESS;
    wlan_adapter *Adapter = priv->adapter;
    IEEEtypes_VendorHeader_t *pVendorIe;
    const u8 wpa_oui[] = { 0x00, 0x50, 0xf2, 0x01 };

    ENTER();
    /* If the passed length is zero, reset the buffer */
    if (ie_len == 0) {
        Adapter->genIeBufferLen = 0;

    } else if (ie_data_ptr == NULL) {
        /* NULL check */
        ret = -EINVAL;
    } else {

        pVendorIe = (IEEEtypes_VendorHeader_t *) ie_data_ptr;

        /* Test to see if it is a WPA IE, if not, then it is a gen IE */
        if ((pVendorIe->ElementId == RSN_IE)
            || ((pVendorIe->ElementId == WPA_IE)
                && (memcmp(pVendorIe->Oui, wpa_oui, sizeof(wpa_oui)) == 0))) {

            /* IE is a WPA/WPA2 IE so call set_wpa function */
            ret = wlan_set_wpa_ie_helper(priv, ie_data_ptr, ie_len);
        } else {
            /* 
             * Verify that the passed length is not larger than the available 
             *   space remaining in the buffer
             */
            if (ie_len < (sizeof(Adapter->genIeBuffer)
                          - Adapter->genIeBufferLen)) {

                /* Append the passed data to the end of the genIeBuffer */
                if (copy_from_user((Adapter->genIeBuffer
                                    + Adapter->genIeBufferLen),
                                   ie_data_ptr, ie_len)) {
                    PRINTM(INFO, "Copy from user failed\n");
                    ret = -EFAULT;

                } else {
                    /* Increment the stored buffer length by the size passed */
                    Adapter->genIeBufferLen += ie_len;
                }

            } else {
                /* Passed data does not fit in the remaining buffer space */
                ret = WLAN_STATUS_FAILURE;
            }
        }
    }

    /* Return WLAN_STATUS_SUCCESS, or < 0 for error case */
    LEAVE();
    return ret;
}

/** 
 *  @brief  Get IE buffer from driver 
 *   
 *  Used to pass an opaque block of data, expected to be IEEE IEs,
 *    back to the application.  Currently the data block passed
 *    back to the application is the saved association response retrieved 
 *    from the firmware.
 *
 *  @param priv         A pointer to wlan_private structure
 *  @param ie_data_ptr  A pointer to the IE or IE block
 *  @param ie_len_ptr   In/Out parameter pointer for the buffer length passed 
 *                      in ie_data_ptr and the resulting data length copied
 *
 *  @return             WLAN_STATUS_SUCCESS --success, otherwise fail
 */
int
wlan_get_gen_ie_helper(wlan_private * priv, u8 * ie_data_ptr, u16 * ie_len_ptr)
{
    wlan_adapter *Adapter = priv->adapter;
    IEEEtypes_AssocRsp_t *pAssocRsp;
    int copySize;

    ENTER();

    pAssocRsp = (IEEEtypes_AssocRsp_t *) Adapter->assocRspBuffer;

    /* 
     * Set the amount to copy back to the application as the minimum of the 
     *   available IE data or the buffer provided by the application
     */
    copySize = (Adapter->assocRspSize - sizeof(pAssocRsp->Capability) -
                -sizeof(pAssocRsp->StatusCode) - sizeof(pAssocRsp->AId));
    copySize = MIN(copySize, *ie_len_ptr);

    /* Copy the IEEE TLVs in the assoc response back to the application */
    if (copy_to_user(ie_data_ptr, (u8 *) pAssocRsp->IEBuffer, copySize)) {
        PRINTM(INFO, "Copy to user failed\n");
        LEAVE();
        return -EFAULT;
    }

    /* Returned copy length */
    *ie_len_ptr = copySize;

    /* No error on return */
    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Get wireless statistics
 *
 *  NOTE: If wlan_prepare_cmd() with wait option is issued 
 *    in this function, a kernel dump (scheduling while atomic) 
 *    issue may happen on some versions of kernels.
 *
 *  @param dev		A pointer to net_device structure
 *  @return 	   	A pointer to iw_statistics buf
 */
struct iw_statistics *
wlan_get_wireless_stats(struct net_device *dev)
{
    wlan_private *priv = netdev_priv(dev);
    wlan_adapter *Adapter = priv->adapter;
    int ret = WLAN_STATUS_SUCCESS;

    ENTER();

    if (!wlan_is_cmd_allowed(priv)) {
        PRINTM(MSG, "%s: not allowed\n", __FUNCTION__);
        LEAVE();
        return NULL;
    }

    priv->wstats.status = Adapter->InfrastructureMode;
    priv->wstats.discard.retries = priv->stats.tx_errors;

    /* send RSSI command to get beacon RSSI/NF, valid only if associated */
    if (Adapter->MediaConnectStatus == WlanMediaStateConnected) {
        ret = wlan_prepare_cmd(priv, HostCmd_CMD_RSSI_INFO, HostCmd_ACT_GEN_GET,
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24)
                               0,
#else
                               HostCmd_OPTION_WAITFORRSP,
#endif
                               0, NULL);
    }
    priv->wstats.qual.level = Adapter->BcnRSSIAvg;
    if (Adapter->BcnNFAvg == 0
        && Adapter->MediaConnectStatus == WlanMediaStateConnected)
        priv->wstats.qual.noise = MRVDRV_NF_DEFAULT_SCAN_VALUE;
    else
        priv->wstats.qual.noise = Adapter->BcnNFAvg;
    priv->wstats.qual.qual = 0;

    PRINTM(INFO, "Signal Level = %#x\n", priv->wstats.qual.level);
    PRINTM(INFO, "Noise = %#x\n", priv->wstats.qual.noise);

    ret = wlan_prepare_cmd(priv, HostCmd_CMD_802_11_GET_LOG, 0,
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24)
                           0,
#else
                           HostCmd_OPTION_WAITFORRSP,
#endif
                           0, NULL);

    if (!ret) {
        priv->wstats.discard.code = 0;
        priv->wstats.discard.fragment = Adapter->LogMsg.fcserror;
        priv->wstats.discard.retries = Adapter->LogMsg.retry;
        priv->wstats.discard.misc = Adapter->LogMsg.ackfailure;
    }

    LEAVE();

    return &priv->wstats;
}

/** 
 *  @brief use rate to get the index
 *   
 *  @param rate                 data rate
 *  @return 	   		index or 0 
 */
u8
data_rate_to_index(u32 rate)
{
    u8 *ptr;

    ENTER();
    if (rate)
        if ((ptr = wlan_memchr(WlanDataRates, (u8) rate,
                               sizeof(WlanDataRates)))) {
            LEAVE();
            return (ptr - WlanDataRates);
        }
    LEAVE();
    return 0;
}

/** 
 *  @brief use index to get the data rate
 *   
 *  @param index                The index of data rate
 *  @return 	   		data rate or 0 
 */
u32
index_to_data_rate(u8 index)
{
    ENTER();
    if (index >= sizeof(WlanDataRates))
        index = 0;

    LEAVE();
    return WlanDataRates[index];
}
