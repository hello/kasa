/** @file  wlan_priv.c 
  * @brief This file contains private ioctl functions
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
	11/15/07: Created from wlan_wext.c
********************************************************/

#include	"wlan_headers.h"

typedef struct _ioctl_cmd
{
    /** IOCTL command number */
    int cmd;
    /** IOCTL subcommand number */
    int subcmd;
    /** Size fix flag */
    BOOLEAN fixsize;
} ioctl_cmd;

static ioctl_cmd Commands_Allowed_In_DeepSleep[] = {
    {.cmd = WLANDEEPSLEEP,.subcmd = 0,.fixsize = FALSE},
    {.cmd = WLAN_SETONEINT_GETWORDCHAR,.subcmd = WLANVERSION,.fixsize = FALSE},
    {.cmd = WLAN_SETINT_GETINT,.subcmd = WLANSDIOCLOCK,.fixsize = TRUE},
    {.cmd = WLAN_SET_GET_2K,.subcmd = WLAN_GET_CFP_TABLE,.fixsize = FALSE},
#ifdef DEBUG_LEVEL1
    {.cmd = WLAN_SET_GET_SIXTEEN_INT,.subcmd = WLAN_DRV_DBG,.fixsize = FALSE},
#endif
};

static ioctl_cmd Commands_Allowed_In_HostSleep[] = {
    {.cmd = WLAN_SETONEINT_GETWORDCHAR,.subcmd = WLANVERSION,.fixsize = FALSE},
    {.cmd = WLANDEEPSLEEP,.subcmd = 1,.fixsize = FALSE},
    {.cmd = WLANDEEPSLEEP,.subcmd = 0,.fixsize = FALSE},
    {.cmd = WLAN_SETINT_GETINT,.subcmd = WLANSDIOCLOCK,.fixsize = TRUE},
    {.cmd = WLAN_SET_GET_2K,.subcmd = WLAN_GET_CFP_TABLE,.fixsize = FALSE},
#ifdef DEBUG_LEVEL1
    {.cmd = WLAN_SET_GET_SIXTEEN_INT,.subcmd = WLAN_DRV_DBG,.fixsize = FALSE},
#endif
};

static ioctl_cmd Commands_DisAllowed_In_AutoDeepSleep[] = {
    {.cmd = WLANDEEPSLEEP,.subcmd = 0,.fixsize = FALSE},
    {.cmd = WLANCMD52RDWR,.subcmd = 0,.fixsize = FALSE},
    {.cmd = WLANCMD53RDWR,.subcmd = 0,.fixsize = FALSE},
    {.cmd = WLANCISDUMP,.subcmd = 0,.fixsize = FALSE},
};

/********************************************************
		Local Variables
********************************************************/

/********************************************************
		Global Variables
********************************************************/
#ifdef DEBUG_LEVEL1
#ifdef DEBUG_LEVEL2
#define	DEFAULT_DEBUG_MASK	(0xffffffff & ~DBG_EVENT)
#else
#define DEFAULT_DEBUG_MASK	(DBG_MSG | DBG_FATAL | DBG_ERROR)
#endif
u32 drvdbg = DEFAULT_DEBUG_MASK;
u32 ifdbg = 0;
#endif

/********************************************************
		Local Functions
********************************************************/
/** 
 *  @brief This function checks if the commands is allowed
 *  in deepsleep/hostsleep mode or not.
 * 
 *  @param req	       A pointer to ifreq structure 
 *  @param cmd         the command ID
 *  @return 	   TRUE or FALSE
 */
static BOOLEAN
wlan_is_cmd_allowed_in_ds_hs(struct ifreq *req, int cmd,
                             ioctl_cmd * allowed_cmds, int count)
{
    int subcmd = 0;
    struct iwreq *wrq = (struct iwreq *) req;
    int i;

    ENTER();

    for (i = 0; i < count; i++) {
        if (cmd == allowed_cmds[i].cmd) {
            if (allowed_cmds[i].subcmd == 0) {
                LEAVE();
                return TRUE;
            }
            if (allowed_cmds[i].fixsize == TRUE)
                subcmd = (int) req->ifr_data;
            else
                subcmd = wrq->u.data.flags;
            if (allowed_cmds[i].subcmd == subcmd) {
                LEAVE();
                return TRUE;
            }
        }
    }

    LEAVE();

    return FALSE;
}

/** 
 *  @brief This function checks if the commands is disallowed
 *  in auto deepsleep mode.
 * 
 *  @param req		A pointer to ifreq structure 
 *  @param cmd		Command ID
 *  @return   		TRUE or FALSE
 */
static BOOLEAN
wlan_is_cmd_disallowed_in_ads(struct ifreq *req, int cmd,
                              ioctl_cmd * disallowed_cmds, int count)
{
    int subcmd = 0;
    struct iwreq *wrq = (struct iwreq *) req;
    int i;

    ENTER();

    for (i = 0; i < count; i++) {
        if (cmd == disallowed_cmds[i].cmd) {
            if (disallowed_cmds[i].subcmd == 0) {
                LEAVE();
                return TRUE;
            }
            if (disallowed_cmds[i].fixsize == TRUE)
                subcmd = (int) req->ifr_data;
            else
                subcmd = wrq->u.data.flags;
            if (disallowed_cmds[i].subcmd == subcmd) {
                LEAVE();
                return TRUE;
            }
        }
    }

    LEAVE();
    return FALSE;
}

/** Ioctl handler to set adhoc coalescing */
static int
wlan_set_coalescing_ioctl(wlan_private * priv, struct iwreq *wrq)
{
    int data;
    int *val;
    int ret = WLAN_STATUS_SUCCESS;

    ENTER();

    data = *((int *) (wrq->u.name + SUBCMD_OFFSET));

    switch (data) {
    case CMD_DISABLED:
    case CMD_ENABLED:
        ret = wlan_prepare_cmd(priv,
                               HostCmd_CMD_802_11_IBSS_COALESCING_STATUS,
                               HostCmd_ACT_GEN_SET,
                               HostCmd_OPTION_WAITFORRSP, 0, &data);
        if (ret) {
            LEAVE();
            return ret;
        }
        break;

    case CMD_GET:
        ret = wlan_prepare_cmd(priv,
                               HostCmd_CMD_802_11_IBSS_COALESCING_STATUS,
                               HostCmd_ACT_GEN_GET,
                               HostCmd_OPTION_WAITFORRSP, 0, &data);
        if (ret) {
            LEAVE();
            return ret;
        }
        break;

    default:
        LEAVE();
        return -EINVAL;
    }

    val = (int *) wrq->u.name;
    *val = data;

    LEAVE();

    return WLAN_STATUS_SUCCESS;
}

#ifdef MFG_CMD_SUPPORT
/** 
 *  @brief Manufacturing command ioctl function
 *   
 *  @param priv		A pointer to wlan_private structure
 *  @param wrq 		A pointer to iwreq structure
 *  @return    		WLAN_STATUS_SUCCESS--success, otherwise--fail
 */
static int
wlan_mfg_command(wlan_private * priv, struct iwreq *wrq)
{
    HostCmd_DS_GEN *pCmdPtr;
    u8 *mfg_cmd;
    u16 mfg_cmd_len;
    int ret;

    ENTER();

    /* allocate MFG command buffer */
    if (!(mfg_cmd = kmalloc(MRVDRV_SIZE_OF_CMD_BUFFER, GFP_KERNEL))) {
        PRINTM(INFO, "allocate MFG command buffer failed!\n");
        LEAVE();
        return -ENOMEM;
    }

    /* get MFG command header */
    if (copy_from_user(mfg_cmd, wrq->u.data.pointer, sizeof(HostCmd_DS_GEN))) {
        PRINTM(INFO, "copy from user failed: MFG command header\n");
        ret = -EFAULT;
        goto mfg_exit;
    }

    /* get the command size */
    pCmdPtr = (HostCmd_DS_GEN *) mfg_cmd;
    mfg_cmd_len = pCmdPtr->Size;
    PRINTM(INFO, "MFG command len = %d\n", mfg_cmd_len);

    if (mfg_cmd_len > MRVDRV_SIZE_OF_CMD_BUFFER) {
        ret = -EINVAL;
        goto mfg_exit;
    }

    /* get the whole command from user */
    if (copy_from_user(mfg_cmd, wrq->u.data.pointer, mfg_cmd_len)) {
        PRINTM(INFO, "copy from user failed: MFG command\n");
        ret = -EFAULT;
        goto mfg_exit;
    }

    ret = wlan_prepare_cmd(priv,
                           HostCmd_CMD_MFG_COMMAND,
                           0, HostCmd_OPTION_WAITFORRSP, 0, mfg_cmd);

    /* copy the response back to user */
    if (!ret && pCmdPtr->Size) {
        mfg_cmd_len = MIN(pCmdPtr->Size, mfg_cmd_len);
        if (copy_to_user(wrq->u.data.pointer, mfg_cmd, mfg_cmd_len)) {
            PRINTM(INFO, "copy to user failed: MFG command\n");
            ret = -EFAULT;
        }
        wrq->u.data.length = mfg_cmd_len;
    }

  mfg_exit:
    kfree(mfg_cmd);
    LEAVE();
    return ret;
}
#endif

/** 
 *  @brief Check if Rate Auto
 *   
 *  @param priv 		A pointer to wlan_private structure
 *  @return 	   		TRUE/FALSE
 */
static BOOLEAN
wlan_is_rate_auto(wlan_private * priv)
{
    wlan_adapter *Adapter = priv->adapter;
    int i;
    int ratenum = 0;
    int bitsize = 0;

    ENTER();

    bitsize = sizeof(Adapter->RateBitmap) * 8;
    for (i = 0; i < bitsize; i++) {
        if (Adapter->RateBitmap & (1 << i))
            ratenum++;
        if (ratenum > 1)
            break;
    }

    LEAVE();

    if (ratenum > 1)
        return TRUE;
    else
        return FALSE;
}

/** 
 *  @brief Covert Rate Bitmap to Rate index
 *   
 *  @param rateBitmap   Rate bit Map value
 *  @return             TRUE/FALSE
 */
static int
wlan_get_rate_index(u16 rateBitmap)
{
    int bitsize = sizeof(rateBitmap) * 8;
    int i;

    ENTER();

    for (i = 0; i < bitsize; i++) {
        if (rateBitmap & (1 << i)) {
            LEAVE();
            return i;
        }
    }

    LEAVE();

    return 0;
}

/** 
 *  @brief Set/Get WPA IE
 *
 *  @param priv         A pointer to wlan_private structure
 *  @param req          A pointer to ifreq structure
 *
 *  @return             WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_set_wpa_ie_ioctl(wlan_private * priv, struct ifreq *req)
{
    struct iwreq *wrq = (struct iwreq *) req;
    int ret = WLAN_STATUS_SUCCESS;

    ENTER();
    ret = wlan_set_wpa_ie_helper(priv, wrq->u.data.pointer, wrq->u.data.length);
    LEAVE();
    return ret;
}

/** 
 *  @brief Set RX Antenna
 *   
 *  @param priv                 A pointer to wlan_private structure
 *  @param Mode			RF antenna mode
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_set_rx_antenna(wlan_private * priv, int Mode)
{
    int ret = WLAN_STATUS_SUCCESS;
    wlan_adapter *Adapter = priv->adapter;

    ENTER();

    if (Mode != RF_ANTENNA_1 && Mode != RF_ANTENNA_2 && Mode != RF_ANTENNA_AUTO) {
        LEAVE();
        return -EINVAL;
    }

    Adapter->RxAntennaMode = Mode;

    PRINTM(INFO, "SET RX Antenna Mode to 0x%04x\n", Adapter->RxAntennaMode);

    ret = wlan_prepare_cmd(priv, HostCmd_CMD_802_11_RF_ANTENNA,
                           HostCmd_ACT_SET_RX, HostCmd_OPTION_WAITFORRSP,
                           0, &Adapter->RxAntennaMode);
    LEAVE();

    return ret;
}

/** 
 *  @brief Set TX Antenna
 *   
 *  @param priv                 A pointer to wlan_private structure
 *  @param Mode			RF antenna mode
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_set_tx_antenna(wlan_private * priv, int Mode)
{
    int ret = 0;
    wlan_adapter *Adapter = priv->adapter;

    ENTER();

    if ((Mode != RF_ANTENNA_1) && (Mode != RF_ANTENNA_2)
        && (Mode != RF_ANTENNA_AUTO)) {
        LEAVE();
        return -EINVAL;
    }

    Adapter->TxAntennaMode = Mode;

    PRINTM(INFO, "SET TX Antenna Mode to 0x%04x\n", Adapter->TxAntennaMode);

    ret = wlan_prepare_cmd(priv, HostCmd_CMD_802_11_RF_ANTENNA,
                           HostCmd_ACT_SET_TX, HostCmd_OPTION_WAITFORRSP,
                           0, &Adapter->TxAntennaMode);

    LEAVE();

    return ret;
}

/** 
 *  @brief Get RX Antenna
 *   
 *  @param priv                 A pointer to wlan_private structure
 *  @param buf			A pointer to receive antenna mode
 *  @return 	   		length of buf 
 */
static int
wlan_get_rx_antenna(wlan_private * priv, char *buf)
{
    int ret = 0;
    wlan_adapter *Adapter = priv->adapter;

    ENTER();

    /* Clear it, so we will know if the value returned below is correct or not. 
     */
    Adapter->RxAntennaMode = 0;

    ret = wlan_prepare_cmd(priv, HostCmd_CMD_802_11_RF_ANTENNA,
                           HostCmd_ACT_GET_RX, HostCmd_OPTION_WAITFORRSP,
                           0, NULL);

    if (ret) {
        LEAVE();
        return ret;
    }

    PRINTM(INFO, "Get Rx Antenna Mode:0x%04x\n", Adapter->RxAntennaMode);

    LEAVE();

    return sprintf(buf, "0x%04x", Adapter->RxAntennaMode) + 1;
}

/** 
 *  @brief Get TX Antenna
 *   
 *  @param priv                 A pointer to wlan_private structure
 *  @param buf			A pointer to receive antenna mode
 *  @return 	   		length of buf 
 */
static int
wlan_get_tx_antenna(wlan_private * priv, char *buf)
{
    int ret = 0;
    wlan_adapter *Adapter = priv->adapter;

    ENTER();

    /* Clear it, so we will know if the value returned below is correct or not. 
     */
    Adapter->TxAntennaMode = 0;

    ret = wlan_prepare_cmd(priv, HostCmd_CMD_802_11_RF_ANTENNA,
                           HostCmd_ACT_GET_TX, HostCmd_OPTION_WAITFORRSP,
                           0, NULL);

    if (ret) {
        LEAVE();
        return ret;
    }

    PRINTM(INFO, "Get Tx Antenna Mode:0x%04x\n", Adapter->TxAntennaMode);

    LEAVE();

    return sprintf(buf, "0x%04x", Adapter->TxAntennaMode) + 1;
}

#ifdef REASSOCIATION
/** 
 *  @brief Set Auto Reassociation On
 *   
 *  @param priv                 A pointer to wlan_private structure
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail 
 */
static int
reassociation_on(wlan_private * priv)
{
    wlan_adapter *Adapter = priv->adapter;

    ENTER();

    Adapter->Reassoc_on = TRUE;

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Set Auto Reassociation Off
 *   
 *  @param priv                 A pointer to wlan_private structure
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail 
 */
static int
reassociation_off(wlan_private * priv)
{
    wlan_adapter *Adapter = priv->adapter;

    ENTER();

    if (Adapter->ReassocTimerIsSet == TRUE) {
        wlan_cancel_timer(&Adapter->MrvDrvTimer);
        Adapter->ReassocTimerIsSet = FALSE;
    }

    Adapter->Reassoc_on = FALSE;

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}
#endif /* REASSOCIATION */

/** 
 *  @brief Set Region
 *   
 *  @param priv                 A pointer to wlan_private structure
 *  @param region_code		region code
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail 
 */
static int
wlan_set_region(wlan_private * priv, u16 region_code)
{
    int i;

    ENTER();

    for (i = 0; i < MRVDRV_MAX_REGION_CODE; i++) {
        /* use the region code to search for the index */
        if (region_code == RegionCodeToIndex[i]) {
            priv->adapter->RegionCode = region_code;
            break;
        }
    }

    /* if it's unidentified region code */
    if (i >= MRVDRV_MAX_REGION_CODE) {
        PRINTM(INFO, "Region Code not identified\n");
        LEAVE();
        return WLAN_STATUS_FAILURE;
    }

    if (wlan_set_regiontable(priv, priv->adapter->RegionCode,
                             priv->adapter->config_bands)) {
        LEAVE();
        return -EINVAL;
    }

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Get/Set Per packet TX Control flags
 *  
 *  @param priv     A pointer to wlan_private structure
 *  @param wrq      A pointer to user data
 *  @return         WLAN_STATUS_SUCCESS--success, otherwise fail
 */
static int
wlan_txcontrol(wlan_private * priv, struct iwreq *wrq)
{
    wlan_adapter *Adapter = priv->adapter;
    int data[3];
    int ret;

    ENTER();

    ret = WLAN_STATUS_SUCCESS;

    switch (wrq->u.data.length) {
    case 0:
        /* 
         *  Get the Global setting for TxCtrl 
         */
        if (copy_to_user(wrq->u.data.pointer, &Adapter->PktTxCtrl, sizeof(u32))) {
            PRINTM(INFO, "copy_to_user failed!\n");
            ret = -EFAULT;
        } else
            wrq->u.data.length = 1;
        break;

    case 1:
        /* 
         *  Set the Global setting for TxCtrl
         */
        if (copy_from_user(&data, wrq->u.data.pointer, sizeof(int))) {
            PRINTM(INFO, "Copy from user failed\n");
            ret = -EFAULT;
        } else {
            Adapter->PktTxCtrl = data[0];
            PRINTM(INFO, "PktTxCtrl set: 0x%08x\n", Adapter->PktTxCtrl);
        }
        break;

    case 2:
        /* 
         *  Get the per User Priority setting for TxCtrl for the given UP
         */
        if (copy_from_user(data, wrq->u.data.pointer, sizeof(int) * 2)) {
            PRINTM(INFO, "Copy from user failed\n");
            ret = -EFAULT;

        } else if (data[1] >= NELEMENTS(Adapter->wmm.userPriPktTxCtrl)) {
            /* Range check the UP input from user space */
            PRINTM(INFO, "User priority out of range\n");
            ret = -EINVAL;

        } else if (Adapter->wmm.userPriPktTxCtrl[data[1]]) {
            data[2] = Adapter->wmm.userPriPktTxCtrl[data[1]];

            /* User priority setting is valid, return it */
            if (copy_to_user(wrq->u.data.pointer, data, sizeof(int) * 3)) {
                PRINTM(INFO, "copy_to_user failed!\n");
                ret = -EFAULT;
            } else
                wrq->u.data.length = 3;

        } else {
            /* Return the global setting since the UP set is zero */
            data[2] = Adapter->PktTxCtrl;

            if (copy_to_user(wrq->u.data.pointer, data, sizeof(int) * 3)) {
                PRINTM(INFO, "copy_to_user failed!\n");
                ret = -EFAULT;
            } else
                wrq->u.data.length = 3;
        }
        break;

    case 3:
        /* 
         *  Set the per User Priority setting for TxCtrl for the given UP
         */

        if (copy_from_user(data, wrq->u.data.pointer, sizeof(int) * 3)) {
            PRINTM(INFO, "Copy from user failed\n");
            ret = -EFAULT;
        } else if (data[1] >= NELEMENTS(Adapter->wmm.userPriPktTxCtrl)) {
            PRINTM(INFO, "User priority out of range\n");
            ret = -EINVAL;
        } else {
            Adapter->wmm.userPriPktTxCtrl[data[1]] = data[2];

            if (Adapter->wmm.userPriPktTxCtrl[data[1]] == 0)
                /* Return the global setting since the UP set is zero */
                data[2] = Adapter->PktTxCtrl;

            if (copy_to_user(wrq->u.data.pointer, data, sizeof(int) * 3)) {
                PRINTM(INFO, "copy_to_user failed!\n");
                ret = -EFAULT;
            } else
                wrq->u.data.length = 3;
        }
        break;

    default:
        ret = -EINVAL;
        break;
    }

    LEAVE();
    return ret;
}

/** 
 *  @brief Enable/Disable atim uapsd null package generation
 *  
 *  @param priv		A pointer to wlan_private structure
 *  @param wrq	   	A pointer to user data
 *  @return 	   	WLAN_STATUS_SUCCESS--success, otherwise fail
 */
static int
wlan_null_pkg_gen(wlan_private * priv, struct iwreq *wrq)
{
    int data;
    wlan_adapter *Adapter = priv->adapter;
    int *val;

    ENTER();

    data = *((int *) (wrq->u.name + SUBCMD_OFFSET));
    PRINTM(INFO, "Enable UAPSD NULL PKG: %s\n",
           (data == CMD_ENABLED) ? "Enable" : "Disable");
    switch (data) {
    case CMD_ENABLED:
        Adapter->gen_null_pkg = TRUE;
        break;
    case CMD_DISABLED:
        Adapter->gen_null_pkg = FALSE;
        break;
    default:
        break;
    }

    data = (Adapter->gen_null_pkg == TRUE) ? CMD_ENABLED : CMD_DISABLED;
    val = (int *) wrq->u.name;
    *val = data;

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Get/Set NULL Package generation interval
 *  
 *  @param priv		A pointer to wlan_private structure
 *  @param wrq	   	A pointer to user data
 *  @return 	   	WLAN_STATUS_SUCCESS--success, otherwise fail
 */
static int
wlan_null_pkt_interval(wlan_private * priv, struct iwreq *wrq)
{
    wlan_adapter *Adapter = priv->adapter;
    int data;
    ENTER();

    if ((int) wrq->u.data.length == 0) {
        data = Adapter->NullPktInterval;

        if (copy_to_user(wrq->u.data.pointer, &data, sizeof(int))) {
            PRINTM(MSG, "copy_to_user failed!\n");
            LEAVE();
            return -EFAULT;
        }
    } else {
        if ((int) wrq->u.data.length > 1) {
            PRINTM(MSG, "ioctl too many args!\n");
            LEAVE();
            return -EFAULT;
        }
        if (copy_from_user(&data, wrq->u.data.pointer, sizeof(int))) {
            PRINTM(INFO, "Copy from user failed\n");
            LEAVE();
            return -EFAULT;
        }

        Adapter->NullPktInterval = data;
    }

    wrq->u.data.length = 1;

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Get/Set Adhoc awake period 
 *  @param priv                 A pointer to wlan_private structure
 *  @param req			A pointer to ifreq structure
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_adhoc_awake_period(wlan_private * priv, struct iwreq *wrq)
{
    wlan_adapter *Adapter = priv->adapter;
    int data;
    ENTER();

    if ((int) wrq->u.data.length == 0) {
        data = Adapter->AdhocAwakePeriod;
        if (copy_to_user(wrq->u.data.pointer, &data, sizeof(int))) {
            PRINTM(MSG, "copy_to_user failed!\n");
            LEAVE();
            return -EFAULT;
        }
    } else {
        if ((int) wrq->u.data.length > 1) {
            PRINTM(MSG, "ioctl too many args!\n");
            LEAVE();
            return -EFAULT;
        }
        if (copy_from_user(&data, wrq->u.data.pointer, sizeof(int))) {
            PRINTM(INFO, "Copy from user failed\n");
            LEAVE();
            return -EFAULT;
        }
#define AWAKE_PERIOD_NOCHANGE 0
#define AWAKE_PERIOD_MIN 1
#define AWAKE_PERIOD_MAX 31
#define DISABLE_AWAKE_PERIOD 0xff
        if ((((data & 0xff) >= AWAKE_PERIOD_MIN) &&
             ((data & 0xff) <= AWAKE_PERIOD_MAX)) ||
            ((data & 0xff) == DISABLE_AWAKE_PERIOD))
            Adapter->AdhocAwakePeriod = (u16) data;
        else {
            if (data != AWAKE_PERIOD_NOCHANGE) {
                PRINTM(INFO,
                       "Invalid parameter, AdhocAwakePeriod not changed\n");
                LEAVE();
                return -EINVAL;
            }
            data = Adapter->AdhocAwakePeriod;
            if (copy_to_user(wrq->u.data.pointer, &data, sizeof(int))) {
                PRINTM(MSG, "copy_to_user failed!\n");
                LEAVE();
                return -EFAULT;
            }
        }
    }
    wrq->u.data.length = 1;
    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Get/Set bcn missing timeout 
 *  @param priv                 A pointer to wlan_private structure
 *  @param req			A pointer to ifreq structure
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_bcn_miss_timeout(wlan_private * priv, struct iwreq *wrq)
{
    wlan_adapter *Adapter = priv->adapter;
    int data;
    ENTER();

    if ((int) wrq->u.data.length == 0) {
        data = Adapter->BCNMissTimeOut;
        if (copy_to_user(wrq->u.data.pointer, &data, sizeof(int))) {
            PRINTM(MSG, "copy_to_user failed!\n");
            LEAVE();
            return -EFAULT;
        }
    } else {
        if ((int) wrq->u.data.length > 1) {
            PRINTM(MSG, "ioctl too many args!\n");
            LEAVE();
            return -EFAULT;
        }
        if (copy_from_user(&data, wrq->u.data.pointer, sizeof(int))) {
            PRINTM(INFO, "Copy from user failed\n");
            LEAVE();
            return -EFAULT;
        }
        if (((data >= 0) && (data <= 50)) || (data == 0xffff))
            Adapter->BCNMissTimeOut = (u16) data;
        else {
            PRINTM(INFO,
                   "Invalid parameter, BCN Missing timeout not changed\n");
            LEAVE();
            return -EINVAL;

        }
    }

    wrq->u.data.length = 1;

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Get/Set adhoc g protection
 *  @param priv                 A pointer to wlan_private structure
 *  @param req			A pointer to ifreq structure
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_adhoc_g_protection(wlan_private * priv, struct iwreq *wrq)
{
    int data;
    int *val;
    wlan_adapter *Adapter = priv->adapter;

    ENTER();
#define ADHOC_G_PROTECTION_ON		1
#define ADHOC_G_PROTECTION_OFF		0
    data = *((int *) (wrq->u.name + SUBCMD_OFFSET));

    switch (data) {
    case CMD_DISABLED:
        Adapter->CurrentPacketFilter &= ~HostCmd_ACT_MAC_ADHOC_G_PROTECTION_ON;
        break;
    case CMD_ENABLED:
        Adapter->CurrentPacketFilter |= HostCmd_ACT_MAC_ADHOC_G_PROTECTION_ON;
        break;

    case CMD_GET:
        if (Adapter->
            CurrentPacketFilter & HostCmd_ACT_MAC_ADHOC_G_PROTECTION_ON)
            data = ADHOC_G_PROTECTION_ON;
        else
            data = ADHOC_G_PROTECTION_OFF;
        break;

    default:
        LEAVE();
        return -EINVAL;
    }

    val = (int *) wrq->u.name;
    *val = data;

    LEAVE();

    return WLAN_STATUS_SUCCESS;
}

/** Enable RTS/CTS */
#define USE_RTS_CTS				1
/** Enable CTS to SELF */
#define USE_CTS_TO_SELF			0
/** 
 *  @brief Get/Set RTS/CTS or CTS to self.
 *  @param priv                 A pointer to wlan_private structure
 *  @param req			A pointer to ifreq structure
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_rts_cts_ctrl(wlan_private * priv, struct iwreq *wrq)
{
    int data;
    wlan_adapter *Adapter = priv->adapter;
    int ret = 0;

    ENTER();

    if ((int) wrq->u.data.length == 0) {
        if (Adapter->CurrentPacketFilter & HostCmd_ACT_MAC_RTS_CTS_ENABLE)
            data = USE_RTS_CTS;
        else
            data = USE_CTS_TO_SELF;
        if (copy_to_user(wrq->u.data.pointer, &data, sizeof(int))) {
            PRINTM(MSG, "copy_to_user failed!\n");
            LEAVE();
            return -EFAULT;
        }
    } else {
        if ((int) wrq->u.data.length > 1) {
            PRINTM(MSG, "ioctl too many args!\n");
            LEAVE();
            return -EFAULT;
        }
        if (copy_from_user(&data, wrq->u.data.pointer, sizeof(int))) {
            PRINTM(INFO, "Copy from user failed\n");
            LEAVE();
            return -EFAULT;
        }
        if (data == USE_RTS_CTS) {
            Adapter->CurrentPacketFilter |= HostCmd_ACT_MAC_RTS_CTS_ENABLE;
        } else {
            Adapter->CurrentPacketFilter &= ~HostCmd_ACT_MAC_RTS_CTS_ENABLE;
        }
        PRINTM(INFO, "Adapter->CurrentPacketFilter=0x%x\n",
               Adapter->CurrentPacketFilter);

        ret = wlan_prepare_cmd(priv,
                               HostCmd_CMD_MAC_CONTROL,
                               0, HostCmd_OPTION_WAITFORRSP,
                               0, &Adapter->CurrentPacketFilter);
    }
    wrq->u.data.length = 1;
    LEAVE();
    return ret;
}

/** 
 *  @brief Get/Set LDO config
 *  @param priv			A pointer to wlan_private structure
 *  @param wrq			A pointer to wrq structure
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_ldo_config(wlan_private * priv, struct iwreq *wrq)
{
    HostCmd_DS_802_11_LDO_CONFIG ldocfg;
    int data = 0;
    u16 action;
    int ret = WLAN_STATUS_SUCCESS;

    ENTER();

    if (wrq->u.data.length == 0) {
        action = HostCmd_ACT_GEN_GET;
    } else if (wrq->u.data.length > 1) {
        PRINTM(MSG, "ioctl too many args!\n");
        ret = -EFAULT;
        goto ldoexit;
    } else {
        if (copy_from_user(&data, wrq->u.data.pointer, sizeof(int))) {
            PRINTM(INFO, "Copy from user failed\n");
            ret = -EFAULT;
            goto ldoexit;
        }
        if (data != LDO_INTERNAL && data != LDO_EXTERNAL) {
            PRINTM(MSG, "Invalid parameter, LDO config not changed\n");
            ret = -EFAULT;
            goto ldoexit;
        }
        action = HostCmd_ACT_GEN_SET;
    }
    ldocfg.Action = action;
    ldocfg.PMSource = (u16) data;

    ret = wlan_prepare_cmd(priv,
                           HostCmd_CMD_802_11_LDO_CONFIG,
                           action, HostCmd_OPTION_WAITFORRSP,
                           0, (void *) &ldocfg);

    if (!ret && action == HostCmd_ACT_GEN_GET) {
        data = (int) ldocfg.PMSource;
        if (copy_to_user(wrq->u.data.pointer, &data, sizeof(int))) {
            PRINTM(INFO, "Copy to user failed\n");
            ret = -EFAULT;
            goto ldoexit;
        }
        wrq->u.data.length = 1;
    }

  ldoexit:
    LEAVE();
    return ret;
}

#ifdef DEBUG_LEVEL1
/** 
 *  @brief Get/Set the bit mask of driver debug message control
 *  @param priv			A pointer to wlan_private structure
 *  @param wrq			A pointer to wrq structure
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_drv_dbg(wlan_private * priv, struct iwreq *wrq)
{
    int data[4];
    int ret = WLAN_STATUS_SUCCESS;

    ENTER();

    if (wrq->u.data.length == 0) {
        data[0] = drvdbg;
        data[1] = ifdbg;
        /* Return the current driver debug bit masks */
        if (copy_to_user(wrq->u.data.pointer, data, sizeof(int) * 2)) {
            PRINTM(INFO, "Copy to user failed\n");
            ret = -EFAULT;
            goto drvdbgexit;
        }
        wrq->u.data.length = 2;
    } else if (wrq->u.data.length < 3) {
        /* Get the driver debug bit masks */
        if (copy_from_user
            (data, wrq->u.data.pointer, sizeof(int) * wrq->u.data.length)) {
            PRINTM(INFO, "Copy from user failed\n");
            ret = -EFAULT;
            goto drvdbgexit;
        }
        drvdbg = data[0];
        if (wrq->u.data.length == 2)
            ifdbg = data[1];
    } else {
        PRINTM(INFO, "Invalid parameter number\n");
        goto drvdbgexit;
    }

    printk(KERN_ALERT "drvdbg = 0x%x\n", drvdbg);
#ifdef DEBUG_LEVEL2
    printk(KERN_ALERT "INFO  (%08lx) %s\n", DBG_INFO,
           (drvdbg & DBG_INFO) ? "X" : "");
    printk(KERN_ALERT "WARN  (%08lx) %s\n", DBG_WARN,
           (drvdbg & DBG_WARN) ? "X" : "");
    printk(KERN_ALERT "ENTRY (%08lx) %s\n", DBG_ENTRY,
           (drvdbg & DBG_ENTRY) ? "X" : "");
#endif
    printk(KERN_ALERT "FW_D  (%08lx) %s\n", DBG_FW_D,
           (drvdbg & DBG_FW_D) ? "X" : "");
    printk(KERN_ALERT "CMD_D (%08lx) %s\n", DBG_CMD_D,
           (drvdbg & DBG_CMD_D) ? "X" : "");
    printk(KERN_ALERT "DAT_D (%08lx) %s\n", DBG_DAT_D,
           (drvdbg & DBG_DAT_D) ? "X" : "");
    printk(KERN_ALERT "INTR  (%08lx) %s\n", DBG_INTR,
           (drvdbg & DBG_INTR) ? "X" : "");
    printk(KERN_ALERT "EVENT (%08lx) %s\n", DBG_EVENT,
           (drvdbg & DBG_EVENT) ? "X" : "");
    printk(KERN_ALERT "CMND  (%08lx) %s\n", DBG_CMND,
           (drvdbg & DBG_CMND) ? "X" : "");
    printk(KERN_ALERT "DATA  (%08lx) %s\n", DBG_DATA,
           (drvdbg & DBG_DATA) ? "X" : "");
    printk(KERN_ALERT "ERROR (%08lx) %s\n", DBG_ERROR,
           (drvdbg & DBG_ERROR) ? "X" : "");
    printk(KERN_ALERT "FATAL (%08lx) %s\n", DBG_FATAL,
           (drvdbg & DBG_FATAL) ? "X" : "");
    printk(KERN_ALERT "MSG   (%08lx) %s\n", DBG_MSG,
           (drvdbg & DBG_MSG) ? "X" : "");
    printk(KERN_ALERT "ifdbg = 0x%x\n", ifdbg);
    printk(KERN_ALERT "IF_D  (%08lx) %s\n", DBG_IF_D,
           (ifdbg & DBG_IF_D) ? "X" : "");

  drvdbgexit:
    LEAVE();
    return ret;
}
#endif

/** 
 *  @brief Get/Set module type config
 *  @param priv			A pointer to wlan_private structure
 *  @param wrq			A pointer to wrq structure
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_module_type_config(wlan_private * priv, struct iwreq *wrq)
{
    HostCmd_DS_MODULE_TYPE_CONFIG moduletypecfg;
    int data = 0;
    u16 action;
    int ret = WLAN_STATUS_SUCCESS;

    ENTER();

    if (wrq->u.data.length == 0) {
        action = HostCmd_ACT_GEN_GET;
    } else if (wrq->u.data.length > 1) {
        PRINTM(MSG, "ioctl too many args!\n");
        ret = -EFAULT;
        goto mtexit;
    } else {
        if (copy_from_user(&data, wrq->u.data.pointer, sizeof(int))) {
            PRINTM(INFO, "Copy from user failed\n");
            ret = -EFAULT;
            goto mtexit;
        }
        action = HostCmd_ACT_GEN_SET;
    }
    moduletypecfg.Action = action;
    moduletypecfg.Module = (u16) data;

    ret = wlan_prepare_cmd(priv,
                           HostCmd_CMD_MODULE_TYPE_CONFIG,
                           action, HostCmd_OPTION_WAITFORRSP,
                           0, (void *) &moduletypecfg);

    if (!ret && action == HostCmd_ACT_GEN_GET) {
        data = (int) moduletypecfg.Module;
        if (copy_to_user(wrq->u.data.pointer, &data, sizeof(int))) {
            PRINTM(INFO, "Copy to user failed\n");
            ret = -EFAULT;
            goto mtexit;
        }
        wrq->u.data.length = 1;
    }

  mtexit:
    LEAVE();
    return ret;
}

/** 
 *  @brief  Append/Reset IE buffer. 
 *   
 *  Pass an opaque block of data, expected to be IEEE IEs, to the driver 
 *    for eventual passthrough to the firmware in an associate/join 
 *    (and potentially start) command.
 *
 *  Data is appended to an existing buffer and then wrapped in a passthrough
 *    TLV in the command API to the firmware.  The firmware treats the data
 *    as a transparent passthrough to the transmitted management frame.
 *
 *  @param priv         A pointer to wlan_private structure
 *  @param wrq          A pointer to iwreq structure    
 *  @return             WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_set_gen_ie_ioctl(wlan_private * priv, struct iwreq *wrq)
{
    int ret = WLAN_STATUS_SUCCESS;

    ENTER();
    ret = wlan_set_gen_ie_helper(priv, wrq->u.data.pointer, wrq->u.data.length);
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
 *  @param wrq          A pointer to iwreq structure
 *
 *  @return             WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_get_gen_ie_ioctl(wlan_private * priv, struct iwreq *wrq)
{
    int ret = WLAN_STATUS_SUCCESS;

    ENTER();
    ret = wlan_get_gen_ie_helper(priv,
                                 wrq->u.data.pointer, &wrq->u.data.length);
    LEAVE();
    return ret;
}

/** 
 *  @brief wlan hostcmd ioctl handler
 *   
 *  @param dev                  A pointer to net_device structure
 *  @param req		        A pointer to ifreq structure
 *  @param cmd			command 
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_hostcmd_ioctl(struct net_device *dev, struct ifreq *req, int cmd)
{
    u8 *tempResponseBuffer;
    CmdCtrlNode *pCmdNode;
    HostCmd_DS_GEN *pCmdPtr;
    wlan_private *priv = netdev_priv(dev);
    wlan_adapter *Adapter = priv->adapter;
    u16 wait_option = 0;
    u16 CmdSize;
    struct iwreq *wrq = (struct iwreq *) req;

    ENTER();

    if ((wrq->u.data.pointer == NULL) || (wrq->u.data.length < S_DS_GEN)) {
        PRINTM(INFO,
               "wlan_hostcmd_ioctl() corrupt data: pointer=%p, length=%d\n",
               wrq->u.data.pointer, wrq->u.data.length);
        LEAVE();
        return -EFAULT;
    }

    /* 
     * Get a free command control node 
     */
    if (!(pCmdNode = wlan_get_cmd_node(priv))) {
        PRINTM(INFO, "Failed wlan_get_cmd_node\n");
        LEAVE();
        return -ENOMEM;
    }

    if (!(tempResponseBuffer = kmalloc(MRVDRV_SIZE_OF_CMD_BUFFER, GFP_KERNEL))) {
        PRINTM(INFO, "ERROR: Failed to allocate response buffer!\n");
        wlan_insert_cmd_to_free_q(priv, pCmdNode);
        LEAVE();
        return -ENOMEM;
    }

    wait_option |= HostCmd_OPTION_WAITFORRSP;

    wlan_init_cmd_node(priv, pCmdNode, 0, wait_option, NULL);
    init_waitqueue_head(&pCmdNode->cmdwait_q);

    pCmdPtr = (HostCmd_DS_GEN *) pCmdNode->BufVirtualAddr;

    /* 
     * Copy the whole command into the command buffer 
     */
    if (copy_from_user(pCmdPtr, wrq->u.data.pointer, wrq->u.data.length)) {
        PRINTM(INFO, "Copy from user failed\n");
        kfree(tempResponseBuffer);
        wlan_insert_cmd_to_free_q(priv, pCmdNode);
        LEAVE();
        return -EFAULT;
    }

    CmdSize = wlan_le16_to_cpu(pCmdPtr->Size);
    if (CmdSize < S_DS_GEN) {
        PRINTM(INFO, "wlan_hostcmd_ioctl() invalid cmd size: %d\n", CmdSize);
        kfree(tempResponseBuffer);
        wlan_insert_cmd_to_free_q(priv, pCmdNode);
        LEAVE();
        return -EFAULT;
    }

    pCmdNode->pdata_buf = tempResponseBuffer;
    pCmdNode->CmdFlags |= CMD_F_HOSTCMD;

    pCmdPtr->Result = 0;

    PRINTM(INFO, "HOSTCMD Command: 0x%04x Size: %d\n",
           wlan_le16_to_cpu(pCmdPtr->Command), CmdSize);
    HEXDUMP("Command Data", (u8 *) (pCmdPtr), MIN(32, CmdSize));
    PRINTM(INFO, "Copying data from : (user)0x%p -> 0x%p(driver)\n",
           req->ifr_data, pCmdPtr);

    pCmdNode->CmdWaitQWoken = FALSE;
    wlan_insert_cmd_to_pending_q(Adapter, pCmdNode, TRUE);
    wake_up_interruptible(&priv->MainThread.waitQ);

    if (wait_option & HostCmd_OPTION_WAITFORRSP) {
        /* Sleep until response is generated by FW */
        wait_event_interruptible(pCmdNode->cmdwait_q, pCmdNode->CmdWaitQWoken);
    }

    /* Copy the response back to user space */
    pCmdPtr = (HostCmd_DS_GEN *) tempResponseBuffer;

    if (copy_to_user
        (wrq->u.data.pointer, tempResponseBuffer,
         wlan_le16_to_cpu(pCmdPtr->Size)))
        PRINTM(INFO, "ERROR: copy_to_user failed!\n");
    wrq->u.data.length = wlan_le16_to_cpu(pCmdPtr->Size);
    kfree(tempResponseBuffer);

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief wlan arpfilter ioctl handler
 *   
 *  @param dev                  A pointer to net_device structure
 *  @param req		        A pointer to ifreq structure
 *  @param cmd			command 
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_arpfilter_ioctl(struct net_device *dev, struct ifreq *req, int cmd)
{
    wlan_private *priv = netdev_priv(dev);
    struct iwreq *wrq = (struct iwreq *) req;
    wlan_adapter *Adapter = priv->adapter;
    MrvlIEtypesHeader_t hdr;

    ENTER();

    if ((wrq->u.data.pointer == NULL)
        || (wrq->u.data.length < sizeof(MrvlIEtypesHeader_t))
        || (wrq->u.data.length > sizeof(Adapter->ArpFilter))) {
        PRINTM(INFO,
               "wlan_arpfilter_ioctl() corrupt data: pointer=%p, length=%d\n",
               wrq->u.data.pointer, wrq->u.data.length);
        LEAVE();
        return -EFAULT;
    }

    if (copy_from_user(&hdr, wrq->u.data.pointer, sizeof(hdr))) {
        PRINTM(INFO, "Copy from user failed\n");
        LEAVE();
        return -EFAULT;
    }

    if (hdr.Len == 0) {
        Adapter->ArpFilterSize = 0;
        memset(Adapter->ArpFilter, 0, sizeof(Adapter->ArpFilter));
    } else {
        Adapter->ArpFilterSize = wrq->u.data.length;

        PRINTM(INFO, "Copying data from : (user)0x%p -> 0x%p(driver)\n",
               wrq->u.data.pointer, Adapter->ArpFilter);
        if (copy_from_user(Adapter->ArpFilter, wrq->u.data.pointer,
                           Adapter->ArpFilterSize)) {
            Adapter->ArpFilterSize = 0;
            PRINTM(INFO, "Copy from user failed\n");
            LEAVE();
            return -EFAULT;
        }

        HEXDUMP("ArpFilter", Adapter->ArpFilter, Adapter->ArpFilterSize);
    }

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Get Rx Info 
 *   
 *  @param priv                 A pointer to wlan_private structure
 *  @param wreq		        A pointer to iwreq structure
 *  @return 	   		WLAN_STATUS_SUCCESS --success
 */
static int
wlan_get_rxinfo(wlan_private * priv, struct iwreq *wrq)
{
    wlan_adapter *Adapter = priv->adapter;
    int data[2];
    int ret = WLAN_STATUS_SUCCESS;
    ENTER();
    ret = wlan_prepare_cmd(priv,
                           HostCmd_CMD_RSSI_INFO,
                           HostCmd_ACT_GEN_GET, HostCmd_OPTION_WAITFORRSP,
                           0, NULL);

    if (ret) {
        LEAVE();
        return ret;
    }
    data[0] = CAL_SNR(Adapter->DataRSSIlast, Adapter->DataNFlast);
    data[1] = Adapter->RxPDRate;
    if (copy_to_user(wrq->u.data.pointer, data, sizeof(int) * 2)) {
        PRINTM(INFO, "Copy to user failed\n");
        LEAVE();
        return -EFAULT;
    }
    wrq->u.data.length = 2;
    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Get SNR 
 *   
 *  @param priv                 A pointer to wlan_private structure
 *  @param wreq		        A pointer to iwreq structure
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_get_snr(wlan_private * priv, struct iwreq *wrq)
{
    int ret = WLAN_STATUS_SUCCESS;
    wlan_adapter *Adapter = priv->adapter;
    int data[4];

    ENTER();

    if (Adapter->MediaConnectStatus == WlanMediaStateDisconnected) {
        PRINTM(INFO, "Can not get SNR in disconnected state\n");
        LEAVE();
        return -ENOTSUPP;
    }

    memset(data, 0, sizeof(data));
    if (wrq->u.data.length) {
        if (copy_from_user
            (data, wrq->u.data.pointer,
             MIN(wrq->u.data.length, 4) * sizeof(int))) {
            PRINTM(INFO, "Copy from user failed\n");
            LEAVE();
            return -EFAULT;
        }
    }
    ret = wlan_prepare_cmd(priv,
                           HostCmd_CMD_RSSI_INFO,
                           HostCmd_ACT_GEN_GET, HostCmd_OPTION_WAITFORRSP,
                           0, NULL);

    if (ret) {
        LEAVE();
        return ret;
    }
    if (wrq->u.data.length == 0) {
        data[0] = CAL_SNR(Adapter->BcnRSSIlast, Adapter->BcnNFlast);
        data[1] = CAL_SNR(Adapter->BcnRSSIAvg, Adapter->BcnNFAvg);
        data[2] = CAL_SNR(Adapter->DataRSSIlast, Adapter->DataNFlast);
        data[3] = CAL_SNR(Adapter->DataRSSIAvg, Adapter->DataNFAvg);
        if (copy_to_user(wrq->u.data.pointer, data, sizeof(int) * 4)) {
            PRINTM(INFO, "Copy to user failed\n");
            LEAVE();
            return -EFAULT;
        }
        wrq->u.data.length = 4;
    } else if (data[0] == 0) {
        data[0] = CAL_SNR(Adapter->BcnRSSIlast, Adapter->BcnNFlast);
        if (copy_to_user(wrq->u.data.pointer, data, sizeof(int))) {
            PRINTM(INFO, "Copy to user failed\n");
            LEAVE();
            return -EFAULT;
        }
        wrq->u.data.length = 1;
    } else if (data[0] == 1) {
        data[0] = CAL_SNR(Adapter->BcnRSSIAvg, Adapter->BcnNFAvg);
        if (copy_to_user(wrq->u.data.pointer, data, sizeof(int))) {
            PRINTM(INFO, "Copy to user failed\n");
            LEAVE();
            return -EFAULT;
        }
        wrq->u.data.length = 1;
    } else if (data[0] == 2) {
        data[0] = CAL_SNR(Adapter->DataRSSIlast, Adapter->DataNFlast);
        if (copy_to_user(wrq->u.data.pointer, data, sizeof(int))) {
            PRINTM(INFO, "Copy to user failed\n");
            LEAVE();
            return -EFAULT;
        }
        wrq->u.data.length = 1;
    } else if (data[0] == 3) {
        data[0] = CAL_SNR(Adapter->DataRSSIAvg, Adapter->DataNFAvg);
        if (copy_to_user(wrq->u.data.pointer, data, sizeof(int))) {
            PRINTM(INFO, "Copy to user failed\n");
            LEAVE();
            return -EFAULT;
        }
        wrq->u.data.length = 1;
    } else {
        LEAVE();
        return -ENOTSUPP;
    }

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Get/Set SDIO PULL CTRL 
 *   
 *  @param priv                 A pointer to wlan_private structure
 *  @param wreq		        A pointer to iwreq structure
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_sdio_pull_ctrl(wlan_private * priv, struct iwreq *wrq)
{
    int data[2];
    HostCmd_DS_SDIO_PULL_CTRL sdio_pull_ctrl;
    int ret = WLAN_STATUS_SUCCESS;

    ENTER();
    memset(&sdio_pull_ctrl, 0, sizeof(sdio_pull_ctrl));
    if (wrq->u.data.length > 0) {
        if (copy_from_user(data, wrq->u.data.pointer, sizeof(data))) {
            PRINTM(INFO, "Copy from user failed\n");
            LEAVE();
            return -EFAULT;
        }
        PRINTM(INFO, "WLAN SET SDIO PULL CTRL: %d %d\n", data[0], data[1]);
        sdio_pull_ctrl.Action = HostCmd_ACT_GEN_SET;
        sdio_pull_ctrl.PullUp = data[0];
        sdio_pull_ctrl.PullDown = data[1];
    } else
        sdio_pull_ctrl.Action = HostCmd_ACT_GEN_GET;

    ret = wlan_prepare_cmd(priv, HostCmd_CMD_SDIO_PULL_CTRL,
                           0, HostCmd_OPTION_WAITFORRSP,
                           0, (void *) &sdio_pull_ctrl);
    data[0] = sdio_pull_ctrl.PullUp;
    data[1] = sdio_pull_ctrl.PullDown;
    wrq->u.data.length = 2;
    if (copy_to_user
        (wrq->u.data.pointer, data, sizeof(int) * wrq->u.data.length)) {
        PRINTM(INFO, "Copy to user failed\n");
        LEAVE();
        return -EFAULT;
    }
    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Get/Set scan time
 *   
 *  @param priv		A pointer to wlan_private structure
 *  @param wreq		A pointer to iwreq structure
 *  @return    		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_scan_time(wlan_private * priv, struct iwreq *wrq)
{
    int data[3] = { 0, 0, 0 };
    wlan_adapter *Adapter = priv->adapter;

    ENTER();

    if (wrq->u.data.length > 0 && wrq->u.data.length <= 3) {
        if (copy_from_user
            (data, wrq->u.data.pointer, sizeof(int) * wrq->u.data.length)) {
            PRINTM(INFO, "Copy from user failed\n");
            LEAVE();
            return -EFAULT;
        }

        PRINTM(INFO, "WLAN SET Scan Time: Specific %d, Active %d, Passive %d\n",
               data[0], data[1], data[2]);
        if (data[0]) {
            if (data[0] > MRVDRV_MAX_ACTIVE_SCAN_CHAN_TIME) {
                PRINTM(MSG,
                       "Invalid parameter, max specific scan time is %d ms\n",
                       MRVDRV_MAX_ACTIVE_SCAN_CHAN_TIME);
                LEAVE();
                return -EINVAL;
            }
            Adapter->SpecificScanTime = data[0];
        }
        if (data[1]) {
            if (data[1] > MRVDRV_MAX_ACTIVE_SCAN_CHAN_TIME) {
                PRINTM(MSG,
                       "Invalid parameter, max active scan time is %d ms\n",
                       MRVDRV_MAX_ACTIVE_SCAN_CHAN_TIME);
                LEAVE();
                return -EINVAL;
            }
            Adapter->ActiveScanTime = data[1];
        }
        if (data[2]) {
            if (data[2] > MRVDRV_MAX_PASSIVE_SCAN_CHAN_TIME) {
                PRINTM(MSG,
                       "Invalid parameter, max passive scan time is %d ms\n",
                       MRVDRV_MAX_PASSIVE_SCAN_CHAN_TIME);
                LEAVE();
                return -EINVAL;
            }
            Adapter->PassiveScanTime = data[2];
        }
    }

    data[0] = Adapter->SpecificScanTime;
    data[1] = Adapter->ActiveScanTime;
    data[2] = Adapter->PassiveScanTime;
    wrq->u.data.length = 3;
    if (copy_to_user
        (wrq->u.data.pointer, data, sizeof(int) * wrq->u.data.length)) {
        PRINTM(INFO, "Copy to user failed\n");
        LEAVE();
        return -EFAULT;
    }

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Get/Set Adhoc beacon Interval 
 *   
 *  @param priv                 A pointer to wlan_private structure
 *  @param wreq		        A pointer to iwreq structure
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_beacon_interval(wlan_private * priv, struct iwreq *wrq)
{
    int data[2];
    wlan_adapter *Adapter = priv->adapter;

    ENTER();

    if (wrq->u.data.length > 0) {
        if (copy_from_user(data, wrq->u.data.pointer, sizeof(int))) {
            PRINTM(INFO, "Copy from user failed\n");
            LEAVE();
            return -EFAULT;
        }

        PRINTM(INFO, "WLAN SET BEACON INTERVAL: %d\n", data[0]);
        if ((data[0] > MRVDRV_MAX_BEACON_INTERVAL) ||
            (data[0] < MRVDRV_MIN_BEACON_INTERVAL)) {
            LEAVE();
            return -ENOTSUPP;
        }
        Adapter->BeaconPeriod = data[0];
    }
    data[0] = Adapter->BeaconPeriod;
    wrq->u.data.length = 1;
    if (Adapter->MediaConnectStatus == WlanMediaStateConnected) {
        data[1] = Adapter->CurBssParams.BSSDescriptor.BeaconPeriod;
        wrq->u.data.length = 2;
    }
    if (copy_to_user
        (wrq->u.data.pointer, data, sizeof(int) * wrq->u.data.length)) {
        PRINTM(INFO, "Copy to user failed\n");
        LEAVE();
        return -EFAULT;
    }

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Get/Set Adhoc ATIM Window 
 *   
 *  @param priv                 A pointer to wlan_private structure
 *  @param wreq		        A pointer to iwreq structure
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_atim_window(wlan_private * priv, struct iwreq *wrq)
{
    int data[2];
    wlan_adapter *Adapter = priv->adapter;

    ENTER();

    if (wrq->u.data.length > 0) {
        if (copy_from_user(data, wrq->u.data.pointer, sizeof(int))) {
            PRINTM(INFO, "Copy from user failed\n");
            LEAVE();
            return -EFAULT;
        }

        PRINTM(INFO, "WLAN SET ATIM WINDOW: %d\n", data[0]);
        Adapter->AtimWindow = data[0];
        Adapter->AtimWindow = MIN(Adapter->AtimWindow, 50);
    }
    data[0] = Adapter->AtimWindow;
    wrq->u.data.length = 1;
    if (Adapter->MediaConnectStatus == WlanMediaStateConnected) {
        data[1] = Adapter->CurBssParams.BSSDescriptor.ATIMWindow;
        wrq->u.data.length = 2;
    }
    if (copy_to_user
        (wrq->u.data.pointer, data, sizeof(int) * wrq->u.data.length)) {
        PRINTM(INFO, "Copy to user failed\n");
        LEAVE();
        return -EFAULT;
    }
    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Get/Set system clock
 *   
 *  @param priv		A pointer to wlan_private structure
 *  @param wreq		A pointer to iwreq structure
 *  @return    		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_ecl_sys_clock(wlan_private * priv, struct iwreq *wrq)
{
    HostCmd_DS_ECL_SYSTEM_CLOCK_CONFIG cmd;
    u16 action;
    int data[16], len, i;
    int ret = WLAN_STATUS_SUCCESS;

    ENTER();

    memset(&cmd, 0, sizeof(HostCmd_DS_ECL_SYSTEM_CLOCK_CONFIG));
    if (wrq->u.data.length > 0) {
        if (copy_from_user(data, wrq->u.data.pointer, sizeof(int))) {
            PRINTM(INFO, "Copy from user failed\n");
            LEAVE();
            return -EFAULT;
        }
        PRINTM(INFO, "WLAN set system clock: %d\n", data[0]);
        action = HostCmd_ACT_GEN_SET;
        cmd.Action = action;
        cmd.SystemClock = (u16) data[0];
    } else {
        action = HostCmd_ACT_GEN_GET;
        cmd.Action = action;
    }

    ret = wlan_prepare_cmd(priv,
                           HostCmd_CMD_ECL_SYSTEM_CLOCK_CONFIG,
                           action, HostCmd_OPTION_WAITFORRSP, 0, (void *) &cmd);

    if (!ret && action == HostCmd_ACT_GEN_GET) {
        data[0] = cmd.SystemClock;
        PRINTM(INFO, "WLAN get system clock: %d\n", data[0]);
        len = MIN(cmd.SupportedSysClockLen / sizeof(u16),
                  sizeof(data) / sizeof(data[0]) - 1);
        for (i = 0; i < len; i++) {
            data[i + 1] = cmd.SupportedSysClock[i];
        }
        wrq->u.data.length = 1 + len;

        if (copy_to_user
            (wrq->u.data.pointer, data, sizeof(int) * wrq->u.data.length)) {
            PRINTM(INFO, "Copy to user failed\n");
            LEAVE();
            return -EFAULT;
        }
    }

    LEAVE();
    return ret;
}

/** 
 *  @brief Get RSSI 
 *   
 *  @param priv                 A pointer to wlan_private structure
 *  @param wreq		        A pointer to iwreq structure
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_get_rssi(wlan_private * priv, struct iwreq *wrq)
{
    int ret = WLAN_STATUS_SUCCESS;
    wlan_adapter *Adapter = priv->adapter;
    int temp;
    int data = 0;
    int *val;

    ENTER();

    if (Adapter->MediaConnectStatus == WlanMediaStateDisconnected) {
        PRINTM(INFO, "Can not get RSSI in disconnected state\n");
        LEAVE();
        return -ENOTSUPP;
    }

    data = *((int *) (wrq->u.name + SUBCMD_OFFSET));
    if ((data >= 0) && (data <= 3)) {
        ret = wlan_prepare_cmd(priv,
                               HostCmd_CMD_RSSI_INFO,
                               HostCmd_ACT_GEN_GET, HostCmd_OPTION_WAITFORRSP,
                               0, NULL);
        if (ret) {
            LEAVE();
            return ret;
        }
    }

    switch (data) {
    case 0:
        temp = Adapter->BcnRSSIlast;
        break;
    case 1:
        temp = Adapter->BcnRSSIAvg;
        break;
    case 2:
        temp = Adapter->DataRSSIlast;
        break;

    case 3:
        temp = Adapter->DataRSSIAvg;
        break;
    default:
        LEAVE();
        return -ENOTSUPP;
    }
    val = (int *) wrq->u.name;
    *val = temp;

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Get NF
 *   
 *  @param priv                 A pointer to wlan_private structure
 *  @param wreq		        A pointer to iwreq structure 
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_get_nf(wlan_private * priv, struct iwreq *wrq)
{
    int ret = WLAN_STATUS_SUCCESS;
    wlan_adapter *Adapter = priv->adapter;
    int temp;
    int data = 0;
    int *val;

    ENTER();

    if (Adapter->MediaConnectStatus == WlanMediaStateDisconnected) {
        PRINTM(INFO, "Can not get NF in disconnected state\n");
        LEAVE();
        return -ENOTSUPP;
    }

    data = *((int *) (wrq->u.name + SUBCMD_OFFSET));
    if ((data >= 0) && (data <= 3)) {
        ret = wlan_prepare_cmd(priv,
                               HostCmd_CMD_RSSI_INFO,
                               HostCmd_ACT_GEN_GET, HostCmd_OPTION_WAITFORRSP,
                               0, NULL);

        if (ret) {
            LEAVE();
            return ret;
        }
    }

    switch (data) {
    case 0:
        temp = Adapter->BcnNFlast;
        break;
    case 1:
        temp = Adapter->BcnNFAvg;
        break;
    case 2:
        temp = Adapter->DataNFlast;
        break;
    case 3:
        temp = Adapter->DataNFAvg;
        break;
    default:
        LEAVE();
        return -ENOTSUPP;
    }
    PRINTM(INFO, "***temp = %d\n", temp);
    val = (int *) wrq->u.name;
    *val = temp;
    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Remove AES key
 *   
 *  @param priv                 A pointer to wlan_private structure
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_remove_aes(wlan_private * priv)
{
    wlan_adapter *Adapter = priv->adapter;
    WLAN_802_11_KEY key;
    int ret = WLAN_STATUS_SUCCESS;

    ENTER();

    if (Adapter->InfrastructureMode != Wlan802_11IBSS ||
        Adapter->MediaConnectStatus == WlanMediaStateConnected) {
        LEAVE();
        return -EOPNOTSUPP;
    }
    Adapter->AdhocAESEnabled = FALSE;

    memset(&key, 0, sizeof(WLAN_802_11_KEY));
    PRINTM(INFO, "WPA2: DISABLE AES_KEY\n");
    key.KeyLength = WPA_AES_KEY_LEN;
    key.KeyIndex = 0x40000000;

    ret = wlan_prepare_cmd(priv,
                           HostCmd_CMD_802_11_KEY_MATERIAL,
                           HostCmd_ACT_GEN_SET,
                           HostCmd_OPTION_WAITFORRSP,
                           !(KEY_INFO_ENABLED), &key);

    LEAVE();

    return ret;
}

/** 
 *  @brief Get Support Rates
 *   
 *  @param priv                 A pointer to wlan_private structure
 *  @param wrq			A pointer to iwreq structure
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_getrate_ioctl(wlan_private * priv, struct iwreq *wrq)
{
    wlan_adapter *Adapter = priv->adapter;
    WLAN_802_11_RATES rates;
    int rate[sizeof(rates)];
    int i;

    ENTER();

    memset(rates, 0, sizeof(rates));
    memset(rate, 0, sizeof(rate));
    wrq->u.data.length = get_active_data_rates(Adapter, rates);
    if (wrq->u.data.length > sizeof(rates))
        wrq->u.data.length = sizeof(rates);

    for (i = 0; i < wrq->u.data.length; i++) {
        rates[i] &= ~0x80;
        rate[i] = rates[i];
    }

    if (copy_to_user
        (wrq->u.data.pointer, rate, wrq->u.data.length * sizeof(int))) {
        PRINTM(INFO, "Copy to user failed\n");
        LEAVE();
        return -EFAULT;
    }

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Get TxRate
 *   
 *  @param priv                 A pointer to wlan_private structure
 *  @param req			A pointer to ifreq structure
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_get_txrate_ioctl(wlan_private * priv, struct ifreq *req)
{
    wlan_adapter *Adapter = priv->adapter;
    int *pdata;
    struct iwreq *wrq = (struct iwreq *) req;
    int ret = WLAN_STATUS_SUCCESS;
    ENTER();
    Adapter->TxRate = 0;
    PRINTM(INFO, "wlan_get_txrate_ioctl\n");
    ret = wlan_prepare_cmd(priv, HostCmd_CMD_802_11_TX_RATE_QUERY,
                           HostCmd_ACT_GEN_GET, HostCmd_OPTION_WAITFORRSP,
                           0, NULL);
    if (ret) {
        LEAVE();
        return ret;
    }
    pdata = (int *) wrq->u.name;
    *pdata = (int) Adapter->TxRate;
    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Get DTIM
 *   
 *  @param priv                 A pointer to wlan_private structure
 *  @param req			A pointer to ifreq structure
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_get_dtim_ioctl(wlan_private * priv, struct ifreq *req)
{
    wlan_adapter *Adapter = priv->adapter;
    struct iwreq *wrq = (struct iwreq *) req;
    int *pdata;
    int ret = WLAN_STATUS_FAILURE;

    ENTER();

    /* The DTIM value is valid only in connected state */
    if (Adapter->MediaConnectStatus == WlanMediaStateConnected) {
        Adapter->Dtim = 0;
        ret = wlan_prepare_cmd(priv, HostCmd_CMD_802_11_SNMP_MIB,
                               HostCmd_ACT_GEN_GET, HostCmd_OPTION_WAITFORRSP,
                               DtimPeriod_i, NULL);
        if (!ret) {
            pdata = (int *) wrq->u.name;
            *pdata = (int) Adapter->Dtim;
        }
    }

    LEAVE();
    return ret;
}

/** 
 *  @brief Get Adhoc Status
 *   
 *  @param priv                 A pointer to wlan_private structure
 *  @param wrq			A pointer to iwreq structure
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_get_adhoc_status_ioctl(wlan_private * priv, struct iwreq *wrq)
{
    char status[64];
    wlan_adapter *Adapter = priv->adapter;

    ENTER();

    memset(status, 0, sizeof(status));

    switch (Adapter->InfrastructureMode) {
    case Wlan802_11IBSS:
        if (Adapter->MediaConnectStatus == WlanMediaStateConnected) {
            switch (Adapter->AdhocState) {
            case ADHOC_STARTED:
                strcpy(status, "AdhocStarted");
                break;
            case ADHOC_JOINED:
                strcpy(status, "AdhocJoined");
                break;
            case ADHOC_COALESCED:
                strcpy(status, "AdhocCoalesced");
                break;
            default:
                strcpy(status, "InvalidAdhocState");
                break;
            }
        } else {
            strcpy(status, "AdhocIdle");
        }
        break;
    case Wlan802_11Infrastructure:
        strcpy(status, "InfraMode");
        break;
    default:
        strcpy(status, "AutoUnknownMode");
        break;
    }

    PRINTM(INFO, "Status = %s\n", status);
    wrq->u.data.length = strlen(status) + 1;

    if (wrq->u.data.pointer) {
        if (copy_to_user(wrq->u.data.pointer, &status, wrq->u.data.length)) {
            LEAVE();
            return -EFAULT;
        }
    }

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Get Driver Version
 *   
 *  @param priv                 A pointer to wlan_private structure
 *  @param req			A pointer to ifreq structure
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_version_ioctl(wlan_private * priv, struct ifreq *req)
{
    int len;
    char buf[128];
    struct iwreq *wrq = (struct iwreq *) req;

    ENTER();

    get_version(priv->adapter, buf, sizeof(buf) - 1);

    len = strlen(buf);
    if (wrq->u.data.pointer) {
        if (copy_to_user(wrq->u.data.pointer, buf, len)) {
            PRINTM(INFO, "CopyToUser failed\n");
            LEAVE();
            return -EFAULT;
        }
        wrq->u.data.length = len;
    }

    PRINTM(INFO, "wlan version: %s\n", buf);

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Get Driver and FW version
 *   
 *  @param priv                 A pointer to wlan_private structure
 *  @param wrq			A pointer to iwreq structure
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_verext_ioctl(wlan_private * priv, struct iwreq *wrq)
{
    HostCmd_DS_VERSION_EXT versionExtCmd;
    int len;

    ENTER();

    memset(&versionExtCmd, 0x00, sizeof(versionExtCmd));

    if (wrq->u.data.flags == 0)
        /* from iwpriv subcmd */
        versionExtCmd.versionStrSel = *((int *) (wrq->u.name + SUBCMD_OFFSET));
    else {
        if (copy_from_user(&versionExtCmd.versionStrSel,
                           wrq->u.data.pointer,
                           sizeof(versionExtCmd.versionStrSel))) {
            PRINTM(INFO, "Copy from user failed\n");
            LEAVE();
            return -EFAULT;
        }
    }

    wlan_prepare_cmd(priv,
                     HostCmd_CMD_VERSION_EXT, 0,
                     HostCmd_OPTION_WAITFORRSP, 0, &versionExtCmd);

    len = strlen(versionExtCmd.versionStr) + 1;
    if (wrq->u.data.pointer) {
        if (copy_to_user(wrq->u.data.pointer, versionExtCmd.versionStr, len)) {
            PRINTM(INFO, "CopyToUser failed\n");
            LEAVE();
            return -EFAULT;
        }
        wrq->u.data.length = len;
    }

    PRINTM(INFO, "Version: %s\n", versionExtCmd.versionStr);

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Read/Write adapter registers
 *   
 *  @param priv                 A pointer to wlan_private structure
 *  @param req			A pointer to ifreq structure
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_regrdwr_ioctl(wlan_private * priv, struct ifreq *req)
{
    wlan_ioctl_regrdwr regrdwr;
    wlan_offset_value offval;
    u8 *pRdeeprom;
    int ret = WLAN_STATUS_SUCCESS;

    ENTER();

    if (copy_from_user(&regrdwr, req->ifr_data, sizeof(regrdwr))) {
        PRINTM(INFO,
               "copy of regrdwr for wlan_regrdwr_ioctl from user failed \n");
        LEAVE();
        return -EFAULT;
    }

    if (regrdwr.WhichReg == REG_EEPROM) {
        PRINTM(INFO, "Inside RDEEPROM\n");
        pRdeeprom =
            (char *) kmalloc((regrdwr.NOB + sizeof(regrdwr)), GFP_KERNEL);
        if (!pRdeeprom) {
            PRINTM(INFO, "allocate memory for EEPROM read failed\n");
            LEAVE();
            return -ENOMEM;
        }
        memcpy(pRdeeprom, &regrdwr, sizeof(regrdwr));
        PRINTM(INFO, "Action: %d, Offset: %x, NOB: %02x\n",
               regrdwr.Action, regrdwr.Offset, regrdwr.NOB);

        ret = wlan_prepare_cmd(priv,
                               HostCmd_CMD_802_11_EEPROM_ACCESS,
                               regrdwr.Action, HostCmd_OPTION_WAITFORRSP,
                               0, pRdeeprom);

        /* 
         * Return the result back to the user 
         */
        if (!ret && regrdwr.Action == HostCmd_ACT_GEN_READ) {
            if (copy_to_user
                (req->ifr_data, pRdeeprom, sizeof(regrdwr) + regrdwr.NOB)) {
                PRINTM(INFO,
                       "copy of regrdwr for wlan_regrdwr_ioctl to user failed \n");
                ret = -EFAULT;
            }
        }

        kfree(pRdeeprom);

        LEAVE();
        return ret;
    }

    offval.offset = regrdwr.Offset;
    offval.value = (regrdwr.Action) ? regrdwr.Value : 0x00;

    PRINTM(INFO, "RegAccess: %02x Action:%d "
           "Offset: %04x Value: %04x\n",
           regrdwr.WhichReg, regrdwr.Action, offval.offset, offval.value);

        /**
	 * regrdwr.WhichReg should contain the command that
	 * corresponds to which register access is to be 
	 * performed HostCmd_CMD_MAC_REG_ACCESS 0x0019
	 * HostCmd_CMD_BBP_REG_ACCESS 0x001a 
	 * HostCmd_CMD_RF_REG_ACCESS 0x001b 
	 */
    ret = wlan_prepare_cmd(priv, regrdwr.WhichReg,
                           regrdwr.Action, HostCmd_OPTION_WAITFORRSP,
                           0, &offval);

    /* 
     * Return the result back to the user 
     */
    if (!ret && regrdwr.Action == HostCmd_ACT_GEN_READ) {
        regrdwr.Value = offval.value;
        if (copy_to_user(req->ifr_data, &regrdwr, sizeof(regrdwr))) {
            PRINTM(INFO,
                   "copy of regrdwr for wlan_regrdwr_ioctl to user failed \n");
            ret = -EFAULT;
        }
    }

    LEAVE();
    return ret;
}

/** 
 *  @brief Cmd52 read/write register
 *   
 *  @param priv                 A pointer to wlan_private structure
 *  @param req			A pointer to ifreq structure
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_cmd52rdwr_ioctl(wlan_private * priv, struct ifreq *req)
{
    u8 buf[7];
    u8 rw, func, dat = 0xff;
    u32 reg;

    ENTER();

    if (copy_from_user(buf, req->ifr_data, sizeof(buf))) {
        PRINTM(INFO, "Copy from user failed\n");
        LEAVE();
        return -EFAULT;
    }

    rw = buf[0];
    func = buf[1];
    reg = buf[5];
    reg = (reg << 8) + buf[4];
    reg = (reg << 8) + buf[3];
    reg = (reg << 8) + buf[2];

    if (rw != 0)
        dat = buf[6];

    PRINTM(INFO, "rw=%d func=%d reg=0x%08X dat=0x%02X\n", rw, func, reg, dat);

    if (rw == 0) {
        if (sbi_read_ioreg(priv, reg, &dat) < 0) {
            PRINTM(INFO, "sdio_read_ioreg: reading register 0x%X failed\n",
                   reg);
            dat = 0xff;
        }
    } else {
        if (sbi_write_ioreg(priv, reg, dat) < 0) {
            PRINTM(INFO, "sdio_read_ioreg: writing register 0x%X failed\n",
                   reg);
            dat = 0xff;
        }
    }
    if (copy_to_user(req->ifr_data, &dat, sizeof(dat))) {
        PRINTM(INFO, "Copy to user failed\n");
        LEAVE();
        return -EFAULT;
    }

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Cmd53 read/write register
 *   
 *  @param priv                 A pointer to wlan_private structure
 *  @param req			A pointer to ifreq structure
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_cmd53rdwr_ioctl(wlan_private * priv, struct ifreq *req)
{
    return -EINVAL;
}

/** 
 *  @brief crypto test
 *   
 *  @param priv                 A pointer to wlan_private structure
 *  @param wrq			A pointer to iwreq structure
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_crypto_test(wlan_private * priv, struct iwreq *wrq)
{
    int ret = WLAN_STATUS_SUCCESS;
    u8 *buf = NULL;
    HostCmd_DS_802_11_CRYPTO *cmd = NULL;
    MrvlIEtypes_Data_t *data;
    u16 len = 0;

    ENTER();

#define CRYPTO_TEST_BUF_LEN	1024
    if (!(buf = kmalloc(CRYPTO_TEST_BUF_LEN, GFP_KERNEL))) {
        PRINTM(INFO, "kmalloc failed!\n");
        LEAVE();
        return -ENOMEM;
    }
    memset(buf, 0, CRYPTO_TEST_BUF_LEN);

    if (wrq->u.data.length < CRYPTO_TEST_BUF_LEN) {
        if (copy_from_user(buf, wrq->u.data.pointer, wrq->u.data.length)) {
            PRINTM(INFO, "Copy from user failed\n");
            ret = -EFAULT;
            goto crypto_exit;
        }
    }

    HEXDUMP("crypton", (u8 *) buf, wrq->u.data.length);

    ret = wlan_prepare_cmd(priv,
                           HostCmd_CMD_802_11_CRYPTO,
                           HostCmd_ACT_GEN_SET,
                           HostCmd_OPTION_WAITFORRSP, 0, buf);

    if (ret != WLAN_STATUS_SUCCESS) {
        PRINTM(MSG, "Response Error\n");
        LEAVE();
        return WLAN_STATUS_FAILURE;
    }
    cmd = (HostCmd_DS_802_11_CRYPTO *) buf;

    if ((cmd->Algorithm != CIPHER_TEST_RC4) &&
        (cmd->Algorithm != CIPHER_TEST_AES) &&
        (cmd->Algorithm != CIPHER_TEST_AES_KEY_WRAP) &&
        (cmd->Algorithm != CIPHER_TEST_AES_CCM)) {

        PRINTM(MSG, "Invalid Algorithm\n");
        LEAVE();
        return WLAN_STATUS_FAILURE;
    }

    if (cmd->Algorithm != CIPHER_TEST_AES_CCM) {
        data = (MrvlIEtypes_Data_t *) (buf + sizeof(HostCmd_DS_802_11_CRYPTO));
        len = data->Header.Len;
        wrq->u.data.length =
            sizeof(HostCmd_DS_802_11_CRYPTO) + sizeof(MrvlIEtypesHeader_t) +
            len;
    } else {
        data =
            (MrvlIEtypes_Data_t *) (buf +
                                    sizeof(HostCmd_DS_802_11_CRYPTO_AES_CCM));
        len = data->Header.Len;
        wrq->u.data.length =
            sizeof(HostCmd_DS_802_11_CRYPTO_AES_CCM) +
            sizeof(MrvlIEtypesHeader_t) + len;
    }

    if (copy_to_user(wrq->u.data.pointer, cmd, wrq->u.data.length)) {
        PRINTM(INFO, "Copy to user failed\n");
        ret = -EFAULT;
    }

  crypto_exit:
    kfree(buf);

    LEAVE();
    return ret;
}

/** 
 *  @brief Convert ascii string to Hex integer
 *     
 *  @param d                    A pointer to integer buf
 *  @param s			A pointer to ascii string 
 *  @param dlen			the length o fascii string
 *  @return 	   	        number of integer  
 */
int
ascii2hex(u8 * d, char *s, u32 dlen)
{
    int i;
    u8 n;

    ENTER();

    memset(d, 0x00, dlen);

    for (i = 0; i < dlen * 2; i++) {
        if ((s[i] >= 48) && (s[i] <= 57))
            n = s[i] - 48;
        else if ((s[i] >= 65) && (s[i] <= 70))
            n = s[i] - 55;
        else if ((s[i] >= 97) && (s[i] <= 102))
            n = s[i] - 87;
        else
            break;
        if ((i % 2) == 0)
            n = n * 16;
        d[i / 2] += n;
    }

    LEAVE();

    return i;
}

/** 
 *  @brief Set adhoc aes key
 *   
 *  @param priv                 A pointer to wlan_private structure
 *  @param req			A pointer to ifreq structure
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_setadhocaes_ioctl(wlan_private * priv, struct ifreq *req)
{
    u8 key_ascii[32];
    u8 key_hex[16];
    int ret = 0;
    struct iwreq *wrq = (struct iwreq *) req;
    wlan_adapter *Adapter = priv->adapter;

    WLAN_802_11_KEY key;

    ENTER();

    if (Adapter->InfrastructureMode != Wlan802_11IBSS) {
        LEAVE();
        return -EOPNOTSUPP;
    }
    if (Adapter->MediaConnectStatus == WlanMediaStateConnected) {
        LEAVE();
        return -EOPNOTSUPP;
    }
    if (copy_from_user(key_ascii, wrq->u.data.pointer, sizeof(key_ascii))) {
        PRINTM(INFO, "wlan_setadhocaes_ioctl copy from user failed \n");
        LEAVE();
        return -EFAULT;
    }

    Adapter->AdhocAESEnabled = TRUE;
    ascii2hex(key_hex, key_ascii, sizeof(key_hex));

    HEXDUMP("wlan_setadhocaes_ioctl", key_hex, sizeof(key_hex));

    PRINTM(INFO, "WPA2: ENABLE AES_KEY\n");
    key.KeyLength = WPA_AES_KEY_LEN;
    key.KeyIndex = 0x40000000;
    memcpy(key.KeyMaterial, key_hex, key.KeyLength);

    ret = wlan_prepare_cmd(priv,
                           HostCmd_CMD_802_11_KEY_MATERIAL,
                           HostCmd_ACT_GEN_SET,
                           HostCmd_OPTION_WAITFORRSP, KEY_INFO_ENABLED, &key);

    LEAVE();
    return ret;
}

/** 
 *  @brief Get adhoc aes key   
 *  @param priv                 A pointer to wlan_private structure
 *  @param req			A pointer to ifreq structure
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_getadhocaes_ioctl(wlan_private * priv, struct ifreq *req)
{
    u8 *tmp;
    u8 key_ascii[33];
    u8 key_hex[16];
    int i, ret = 0;
    struct iwreq *wrq = (struct iwreq *) req;
    wlan_adapter *Adapter = priv->adapter;
    WLAN_802_11_KEY key;

    ENTER();

    memset(key_hex, 0x00, sizeof(key_hex));

    PRINTM(INFO, "WPA2: ENABLE AES_KEY\n");
    key.KeyLength = WPA_AES_KEY_LEN;
    key.KeyIndex = 0x40000000;
    memcpy(key.KeyMaterial, key_hex, key.KeyLength);

    ret = wlan_prepare_cmd(priv,
                           HostCmd_CMD_802_11_KEY_MATERIAL,
                           HostCmd_ACT_GEN_GET,
                           HostCmd_OPTION_WAITFORRSP, KEY_INFO_ENABLED, &key);

    if (ret) {
        LEAVE();
        return ret;
    }

    memcpy(key_hex, Adapter->aeskey.KeyParamSet.Key, sizeof(key_hex));

    HEXDUMP("wlan_getadhocaes_ioctl", key_hex, sizeof(key_hex));

    wrq->u.data.length = sizeof(key_ascii) + 1;

    memset(key_ascii, 0x00, sizeof(key_ascii));
    tmp = key_ascii;

    for (i = 0; i < sizeof(key_hex); i++)
        tmp += sprintf(tmp, "%02x", key_hex[i]);

    if (wrq->u.data.pointer) {
        if (copy_to_user(wrq->u.data.pointer, &key_ascii, sizeof(key_ascii))) {
            PRINTM(INFO, "copy_to_user failed\n");
            LEAVE();
            return -EFAULT;
        }
    }

    LEAVE();
    return ret;
}

/** 
 *  @brief Set multiple dtim
 *  @param priv                 A pointer to wlan_private structure
 *  @param req			A pointer to ifreq structure
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_set_multiple_dtim_ioctl(wlan_private * priv, struct ifreq *req)
{
    struct iwreq *wrq = (struct iwreq *) req;
    u32 mdtim;
    int idata;
    int ret = -EINVAL;

    ENTER();

    idata = *((int *) (wrq->u.name + SUBCMD_OFFSET));
    mdtim = (u32) idata;
    if (((mdtim >= MRVDRV_MIN_MULTIPLE_DTIM) &&
         (mdtim <= MRVDRV_MAX_MULTIPLE_DTIM))
        || (mdtim == MRVDRV_IGNORE_MULTIPLE_DTIM)) {
        priv->adapter->MultipleDtim = mdtim;
        ret = WLAN_STATUS_SUCCESS;
    }
    if (ret)
        PRINTM(INFO, "Invalid parameter, MultipleDtim not changed\n");

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
wlan_setauthalg_ioctl(wlan_private * priv, struct ifreq *req)
{
    int alg;
    struct iwreq *wrq = (struct iwreq *) req;
    wlan_adapter *Adapter = priv->adapter;

    ENTER();

    if (wrq->u.data.flags == 0)
        /* from iwpriv subcmd */
        alg = *((int *) (wrq->u.name + SUBCMD_OFFSET));
    else {
        /* from wpa_supplicant subcmd */
        if (copy_from_user(&alg, wrq->u.data.pointer, sizeof(alg))) {
            PRINTM(INFO, "Copy from user failed\n");
            LEAVE();
            return -EFAULT;
        }
    }

    PRINTM(INFO, "auth alg is %#x\n", alg);

    switch (alg) {
    case AUTH_ALG_SHARED_KEY:
        Adapter->SecInfo.AuthenticationMode = Wlan802_11AuthModeShared;
        break;
    case AUTH_ALG_NETWORK_EAP:
        Adapter->SecInfo.AuthenticationMode = Wlan802_11AuthModeNetworkEAP;
        break;
    case AUTH_ALG_OPEN_SYSTEM:
    default:
        Adapter->SecInfo.AuthenticationMode = Wlan802_11AuthModeOpen;
        break;
    }

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Set Encryption mode
 *  @param priv                 A pointer to wlan_private structure
 *  @param req			A pointer to ifreq structure
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_setencryptionmode_ioctl(wlan_private * priv, struct ifreq *req)
{
    int mode;
    struct iwreq *wrq = (struct iwreq *) req;

    ENTER();

    if (wrq->u.data.flags == 0)
        /* from iwpriv subcmd */
        mode = *((int *) (wrq->u.name + SUBCMD_OFFSET));
    else {
        /* from wpa_supplicant subcmd */
        if (copy_from_user(&mode, wrq->u.data.pointer, sizeof(int))) {
            PRINTM(INFO, "Copy from user failed\n");
            LEAVE();
            return -EFAULT;
        }
    }
    PRINTM(INFO, "encryption mode is %#x\n", mode);
    priv->adapter->SecInfo.EncryptionMode = mode;

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Get Rx antenna
 *  @param priv                 A pointer to wlan_private structure
 *  @param req			A pointer to ifreq structure
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_subcmd_getrxantenna_ioctl(wlan_private * priv, struct ifreq *req)
{
    int len;
    char buf[8];
    struct iwreq *wrq = (struct iwreq *) req;

    ENTER();

    PRINTM(INFO, "WLAN_SUBCMD_GETRXANTENNA\n");
    len = wlan_get_rx_antenna(priv, buf);

    wrq->u.data.length = len;
    if (wrq->u.data.pointer) {
        if (copy_to_user(wrq->u.data.pointer, &buf, len)) {
            PRINTM(INFO, "CopyToUser failed\n");
            LEAVE();
            return -EFAULT;
        }
    }

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Get Tx antenna
 *  @param priv                 A pointer to wlan_private structure
 *  @param req			A pointer to ifreq structure
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_subcmd_gettxantenna_ioctl(wlan_private * priv, struct ifreq *req)
{
    int len;
    char buf[8];
    struct iwreq *wrq = (struct iwreq *) req;

    ENTER();

    PRINTM(INFO, "WLAN_SUBCMD_GETTXANTENNA\n");
    len = wlan_get_tx_antenna(priv, buf);

    wrq->u.data.length = len;
    if (wrq->u.data.pointer) {
        if (copy_to_user(wrq->u.data.pointer, &buf, len)) {
            PRINTM(INFO, "CopyToUser failed\n");
            LEAVE();
            return -EFAULT;
        }
    }

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Get the MAC TSF value from the firmware
 *
 *  @param priv         A pointer to wlan_private structure
 *  @param wrq          A pointer to iwreq structure containing buffer
 *                      space to store a TSF value retrieved from the firmware
 *
 *  @return             0 if successful; IOCTL error code otherwise
 */
static int
wlan_get_tsf_ioctl(wlan_private * priv, struct iwreq *wrq)
{
    u64 tsfVal = 0;
    int ret;

    ENTER();

    ret = wlan_prepare_cmd(priv,
                           HostCmd_CMD_GET_TSF,
                           0, HostCmd_OPTION_WAITFORRSP, 0, &tsfVal);

    PRINTM(INFO, "IOCTL: Get TSF = 0x%016llx\n", tsfVal);

    if (ret != WLAN_STATUS_SUCCESS) {
        PRINTM(INFO, "IOCTL: Get TSF; Command exec failed\n");
        ret = -EFAULT;
    } else {
        if (copy_to_user(wrq->u.data.pointer,
                         &tsfVal,
                         MIN(wrq->u.data.length, sizeof(tsfVal))) != 0) {

            PRINTM(INFO, "IOCTL: Get TSF; Copy to user failed\n");
            ret = -EFAULT;
        } else {
            ret = 0;
        }
    }

    LEAVE();

    return ret;
}

/** 
 *  @brief  Control WPS Session Enable/Disable
 *  @param priv         A pointer to wlan_private structure
 *  @param req          A pointer to ifreq structure
 *  @return             WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_do_wps_session_ioctl(wlan_private * priv, struct iwreq *req)
{
    wlan_adapter *Adapter = priv->adapter;
    char buf[8];
    struct iwreq *wrq = (struct iwreq *) req;

    ENTER();

    PRINTM(INFO, "WLAN_WPS_SESSION\n");

    memset(buf, 0, sizeof(buf));
    if (copy_from_user(buf, wrq->u.data.pointer,
                       MIN(sizeof(buf) - 1, wrq->u.data.length))) {
        PRINTM(INFO, "Copy from user failed\n");
        LEAVE();
        return -EFAULT;
    }

    if (buf[0] == 1)
        Adapter->wps.SessionEnable = TRUE;
    else
        Adapter->wps.SessionEnable = FALSE;

    PRINTM(INFO, "Adapter->wps.SessionEnable = %d\n",
           Adapter->wps.SessionEnable);

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Set Deep Sleep 
 *   
 *  @param priv 	A pointer to wlan_private structure
 *  @param bDeepSleep	TRUE--enalbe deepsleep, FALSE--disable deepsleep
 *  @return 	   	WLAN_STATUS_SUCCESS-success, otherwise fail
 */

static int
wlan_set_deep_sleep(wlan_private * priv, BOOLEAN bDeepSleep)
{
    int ret = WLAN_STATUS_SUCCESS;
    wlan_adapter *Adapter = priv->adapter;

    ENTER();

    if (bDeepSleep == TRUE) {
        if (Adapter->IsDeepSleep != TRUE) {
            PRINTM(INFO, "Deep Sleep: sleep\n");

            /* note: the command could be queued and executed later if there
               is command in prigressing. */
            ret = wlan_prepare_cmd(priv,
                                   HostCmd_CMD_802_11_DEEP_SLEEP, 0,
                                   HostCmd_OPTION_WAITFORRSP, 0, NULL);

            if (ret) {
                LEAVE();
                return ret;
            }
            wmm_stop_queue(priv);
            os_stop_queue(priv);
            os_carrier_off(priv);
        }
    } else {
        if (Adapter->IsDeepSleep == TRUE) {
            PRINTM(CMND, "Deep Sleep: wakeup\n");

            if (Adapter->IntCounterSaved) {
                Adapter->IntCounter = Adapter->IntCounterSaved;
                Adapter->IntCounterSaved = 0;
            }

            if (wlan_exit_deep_sleep_timeout(priv) != WLAN_STATUS_SUCCESS) {
                PRINTM(ERROR, "Deep Sleep : wakeup failed\n");
            }

            if (Adapter->IntCounter)
                wake_up_interruptible(&priv->MainThread.waitQ);
        }
    }

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Get/Set DeepSleep mode
 *  @param priv                 A pointer to wlan_private structure
 *  @param req			A pointer to ifreq structure
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_deepsleep_ioctl(wlan_private * priv, struct ifreq *req)
{
    int ret = WLAN_STATUS_SUCCESS;
    char status[128];
    struct iwreq *wrq = (struct iwreq *) req;
    wlan_adapter *Adapter = priv->adapter;

    ENTER();

    if (Adapter->MediaConnectStatus == WlanMediaStateConnected) {
        PRINTM(MSG, "Cannot enter Deep Sleep mode in connected state\n");
        LEAVE();
        return -EINVAL;
    }

    if (*(char *) req->ifr_data == '0') {
        PRINTM(INFO, "Exit Deep Sleep Mode\n");
        sprintf(status, "setting to off ");
        wlan_set_deep_sleep(priv, FALSE);
    } else if (*(char *) req->ifr_data == '1') {
        PRINTM(INFO, "Enter Deep Sleep Mode\n");
        sprintf(status, "setting to on ");
        wlan_set_deep_sleep(priv, TRUE);
    } else if (*(char *) req->ifr_data == '2') {
        PRINTM(INFO, "Get Deep Sleep Mode\n");
        if (Adapter->IsDeepSleep == TRUE)
            sprintf(status, "on ");
        else
            sprintf(status, "off ");
    } else {
        PRINTM(INFO, "unknown option = %d\n", *(u8 *) req->ifr_data);
        LEAVE();
        return -EINVAL;
    }

    if (wrq->u.data.pointer) {
        if (copy_to_user(wrq->u.data.pointer, &status, strlen(status))) {
            LEAVE();
            return -EFAULT;
        }
        wrq->u.data.length = strlen(status);
    }

    LEAVE();
    return ret;
}

/** 
 *  @brief Get/Set inactivity timeout extend
 *  @param priv                 A pointer to wlan_private structure
 *  @param wrq			A pointer to iwreq structure
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_inactivity_timeout_ext(wlan_private * priv, struct iwreq *wrq)
{
    int ret;
    int data[4];
    HostCmd_DS_INACTIVITY_TIMEOUT_EXT inactivity_ext;

    ENTER();

    if ((wrq->u.data.length != 3) && (wrq->u.data.length != 4)
        && (wrq->u.data.length != 0)) {

        LEAVE();
        return -ENOTSUPP;
    }

    memset(data, 0x00, sizeof(data));
    memset(&inactivity_ext, 0x00, sizeof(inactivity_ext));

    if (wrq->u.data.length != 0) {

        /* Set */
        if (copy_from_user(data, wrq->u.data.pointer,
                           wrq->u.data.length * sizeof(int))) {
            PRINTM(INFO, "Copy from user failed\n");
            LEAVE();
            return -EFAULT;
        }

        inactivity_ext.Action = HostCmd_ACT_GEN_SET;
        inactivity_ext.TimeoutUnit = (u16) data[0];
        inactivity_ext.UnicastTimeout = (u16) data[1];
        inactivity_ext.MulticastTimeout = (u16) data[2];
        inactivity_ext.PsEntryTimeout = (u16) data[3];
    } else {
        inactivity_ext.Action = HostCmd_ACT_GEN_GET;
    }

    ret = wlan_prepare_cmd(priv, HostCmd_CMD_INACTIVITY_TIMEOUT_EXT,
                           HostCmd_ACT_GEN_GET,
                           HostCmd_OPTION_WAITFORRSP, 0, &inactivity_ext);

    /* Copy back current values regardless of GET/SET */
    data[0] = (int) inactivity_ext.TimeoutUnit;
    data[1] = (int) inactivity_ext.UnicastTimeout;
    data[2] = (int) inactivity_ext.MulticastTimeout;
    data[3] = (int) inactivity_ext.PsEntryTimeout;

    wrq->u.data.length = 4;

    if (copy_to_user(wrq->u.data.pointer, data,
                     wrq->u.data.length * sizeof(int))) {
        PRINTM(INFO, "Copy to user failed\n");
        LEAVE();
        return -EFAULT;
    }

    LEAVE();
    return ret;
}

/** 
 *  @brief Config Host Sleep parameters
 *   
 *  @param priv		A pointer to wlan_private structure
 *  @param wreq		A pointer to iwreq structure
 *  @return    		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_hscfg_ioctl(wlan_private * priv, struct iwreq *wrq)
{
    wlan_adapter *Adapter = priv->adapter;
    int data[3] = { -1, 0xff, 0xff };
    int ret = WLAN_STATUS_SUCCESS;

    ENTER();

    if (wrq->u.data.length >= 1 && wrq->u.data.length <= 3) {
        if (copy_from_user
            (data, wrq->u.data.pointer, sizeof(int) * wrq->u.data.length)) {
            PRINTM(INFO, "Copy from user failed\n");
            LEAVE();
            return -EFAULT;
        }
        PRINTM(INFO,
               "wlan_hscfg_ioctl: data[0]=%#08x, data[1]=%#02x, data[2]=%#02x\n",
               data[0], data[1], data[2]);
    } else {
        PRINTM(MSG, "Invalid Argument\n");
        LEAVE();
        return -EINVAL;
    }

    Adapter->HSCfg.conditions = data[0];
    if (Adapter->HSCfg.conditions != HOST_SLEEP_CFG_CANCEL) {
        if (wrq->u.data.length == 2)
            Adapter->HSCfg.gpio = (u8) data[1];
        else if (wrq->u.data.length == 3) {
            Adapter->HSCfg.gpio = (u8) data[1];
            Adapter->HSCfg.gap = (u8) data[2];
        }
    }

    PRINTM(INFO,
           "hscfg: cond=%#x gpio=%#x gap=%#x PSState=%d HS_Activated=%d\n",
           Adapter->HSCfg.conditions, Adapter->HSCfg.gpio, Adapter->HSCfg.gap,
           Adapter->PSState, Adapter->HS_Activated);

    ret = wlan_prepare_cmd(priv,
                           HostCmd_CMD_802_11_HOST_SLEEP_CFG,
                           0, HostCmd_OPTION_WAITFORRSP, 0, &Adapter->HSCfg);

    data[0] = Adapter->HSCfg.conditions;
    data[1] = Adapter->HSCfg.gpio;
    data[2] = Adapter->HSCfg.gap;
    wrq->u.data.length = 3;
    if (copy_to_user
        (wrq->u.data.pointer, data, sizeof(int) * wrq->u.data.length)) {
        PRINTM(INFO, "Copy to user failed\n");
        LEAVE();
        return -EFAULT;
    }

    LEAVE();
    return ret;
}

/** 
 *  @brief Set Host Sleep parameters
 *   
 *  @param priv		A pointer to wlan_private structure
 *  @param wreq		A pointer to iwreq structure
 *  @return    		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_hssetpara_ioctl(wlan_private * priv, struct iwreq *wrq)
{
    wlan_adapter *Adapter = priv->adapter;
    int data[3] = { -1, 0xff, 0xff };
    int ret = WLAN_STATUS_SUCCESS;

    ENTER();

    if (wrq->u.data.length >= 1 && wrq->u.data.length <= 3) {
        if (copy_from_user
            (data, wrq->u.data.pointer, sizeof(int) * wrq->u.data.length)) {
            PRINTM(INFO, "Copy from user failed\n");
            LEAVE();
            return -EFAULT;
        }
        PRINTM(INFO,
               "wlan_hssetpara_ioctl: data[0]=%#08x, data[1]=%#02x, data[2]=%#02x\n",
               data[0], data[1], data[2]);
    }

    Adapter->HSCfg.conditions = data[0];
    if (Adapter->HSCfg.conditions != HOST_SLEEP_CFG_CANCEL) {
        if (wrq->u.data.length == 2)
            Adapter->HSCfg.gpio = (u8) data[1];
        else if (wrq->u.data.length == 3) {
            Adapter->HSCfg.gpio = (u8) data[1];
            Adapter->HSCfg.gap = (u8) data[2];
        }
    }

    PRINTM(INFO,
           "hssetpara: cond=%#x gpio=%#x gap=%#x PSState=%d HS_Activated=%d\n",
           Adapter->HSCfg.conditions, Adapter->HSCfg.gpio, Adapter->HSCfg.gap,
           Adapter->PSState, Adapter->HS_Activated);

    data[0] = Adapter->HSCfg.conditions;
    data[1] = Adapter->HSCfg.gpio;
    data[2] = Adapter->HSCfg.gap;
    wrq->u.data.length = 3;
    if (copy_to_user
        (wrq->u.data.pointer, data, sizeof(int) * wrq->u.data.length)) {
        PRINTM(INFO, "Copy to user failed\n");
        LEAVE();
        return -EFAULT;
    }

    LEAVE();
    return ret;
}

/** Destination host */
#define DESTINATION_HOST	0x01
/** 
 *  @brief Config Small Debug parameters
 *   
 *  @param priv		A pointer to wlan_private structure
 *  @param wreq		A pointer to iwreq structure
 *  @return    		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_dbgs_cfg(wlan_private * priv, struct iwreq *wrq)
{
    int *data = NULL;
    u8 buf[512];
    DBGS_CFG_DATA *pDbgCfg = (DBGS_CFG_DATA *) buf;
    DBGS_ENTRY_DATA *pEntry;
    int ret = WLAN_STATUS_SUCCESS;
    int i;

    ENTER();

#define MAX_DBGS_CFG_ENTRIES	0x7F
    if (wrq->u.data.length < 3 ||
        wrq->u.data.length > 3 + MAX_DBGS_CFG_ENTRIES * 2) {
        PRINTM(MSG, "Invalid parameter number\n");
        ret = -EFAULT;
        goto dbgs_exit;
    }

    if (!(data = kmalloc(sizeof(int) * wrq->u.data.length, GFP_KERNEL))) {
        PRINTM(INFO, "Allocate memory failed\n");
        ret = -ENOMEM;
        goto dbgs_exit;
    }
    memset(data, 0, sizeof(int) * wrq->u.data.length);
    memset(buf, 0, sizeof(buf));

    if (copy_from_user
        (data, wrq->u.data.pointer, sizeof(int) * wrq->u.data.length)) {
        PRINTM(INFO, "Copy from user failed\n");
        ret = -EFAULT;
        goto dbgs_exit;
    }
    if (wrq->u.data.length < (3 + (data[2] & 0x7f) * 2)) {
        PRINTM(MSG, "Invalid parameter number\n");
        ret = -EFAULT;
        goto dbgs_exit;
    }

    pDbgCfg->data.Destination = (u8) data[0];
#ifdef	DEBUG_LEVEL1
    if (pDbgCfg->data.Destination & DESTINATION_HOST)
        drvdbg |= DBG_FW_D;
    else
        drvdbg &= ~DBG_FW_D;
#endif
    pDbgCfg->data.ToAirChan = (u8) data[1];
    pDbgCfg->data.En_NumEntries = (u8) data[2];
    pDbgCfg->size =
        sizeof(HostCmd_DS_DBGS_CFG) +
        (pDbgCfg->data.En_NumEntries & 0x7f) * sizeof(DBGS_ENTRY_DATA);
    pEntry = (DBGS_ENTRY_DATA *) (buf + sizeof(DBGS_CFG_DATA));
    for (i = 0; i < (pDbgCfg->data.En_NumEntries & 0x7f); i++) {
        pEntry->ModeAndMaskorID = (u16) data[3 + i * 2];
        pEntry->BaseOrID = (u16) data[3 + i * 2 + 1];
        pEntry->ModeAndMaskorID = wlan_cpu_to_le16(pEntry->ModeAndMaskorID);
        pEntry->BaseOrID = wlan_cpu_to_le16(pEntry->BaseOrID);
        pEntry++;
    }
    ret = wlan_prepare_cmd(priv,
                           HostCmd_CMD_DBGS_CFG,
                           0, HostCmd_OPTION_WAITFORRSP, 0, pDbgCfg);

  dbgs_exit:
    if (data)
        kfree(data);
    LEAVE();
    return ret;
}

/** 
 *  @brief Set band 
 *  @param priv                 A pointer to wlan_private structure
 *  @param wrq			A pointer to iwreq structure
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_do_setband_ioctl(wlan_private * priv, struct iwreq *wrq)
{
    int cnt, i;
    char buf[16];
    u16 tmp = 0;
    wlan_adapter *Adapter = priv->adapter;

    ENTER();

    if (Adapter->MediaConnectStatus == WlanMediaStateConnected) {
        LEAVE();
        return -EOPNOTSUPP;
    }
    memset(buf, '\0', sizeof(buf));
    if (copy_from_user(buf, wrq->u.data.pointer,
                       MIN(wrq->u.data.length, sizeof(buf) - 1))) {
        PRINTM(INFO, "Copy from user failed\n");
        LEAVE();
        return -EFAULT;
    }

    cnt = strlen(buf);

    if (!cnt || cnt > 3) {
        LEAVE();
        return -EINVAL;
    }
    for (i = 0; i < cnt; i++) {
        switch (buf[i]) {
        case 'B':
        case 'b':
            PRINTM(INFO, "setband: BAND B\n");
            tmp |= BAND_B;
            break;
        case 'G':
        case 'g':
            PRINTM(INFO, "setband: BAND G\n");
            tmp |= BAND_G;
            break;
        case 'A':
        case 'a':
            PRINTM(INFO, "setband: BAND A\n");
            tmp |= BAND_A;
            break;
        default:
            PRINTM(INFO, "setband: Invalid %02x(%c)\n", buf[i], buf[i]);
            LEAVE();
            return -EINVAL;
        }
    }

    PRINTM(INFO, "setband: sel=%02x, fw=%02x\n", tmp, Adapter->fw_bands);

    /* Validate against FW support */
    if ((tmp | Adapter->fw_bands) & ~Adapter->fw_bands) {
        LEAVE();
        return -EINVAL;
    }
    /* To support only <a/b/bg/abg> */
    if (!tmp || (tmp == BAND_G) || (tmp == (BAND_A | BAND_B)) ||
        (tmp == (BAND_A | BAND_G))) {
        LEAVE();
        return -EINVAL;
    }
    if (wlan_set_regiontable(priv, Adapter->RegionCode, tmp)) {
        LEAVE();
        return -EINVAL;
    }

    if (wlan_set_universaltable(priv, tmp)) {
        LEAVE();
        return WLAN_STATUS_FAILURE;
    }

    Adapter->config_bands = tmp;

    if (Adapter->config_bands & BAND_A) {
        Adapter->adhoc_start_band = BAND_A;
        Adapter->AdhocChannel = DEFAULT_AD_HOC_CHANNEL_A;
    } else if (Adapter->config_bands & BAND_G) {
        Adapter->adhoc_start_band = BAND_G;
        Adapter->AdhocChannel = DEFAULT_AD_HOC_CHANNEL;
    } else {
        Adapter->adhoc_start_band = BAND_B;
        Adapter->AdhocChannel = DEFAULT_AD_HOC_CHANNEL;
    }

    LEAVE();

    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Set adhoc channel 
 *  @param priv                 A pointer to wlan_private structure
 *  @param wrq			A pointer to iwreq structure
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_do_setadhocch_ioctl(wlan_private * priv, struct iwreq *wrq)
{
    /* iwpriv ethX setaddocchannel "a nnn" */
    u32 vals[2];
    u16 band;
    u8 chan;
    wlan_adapter *Adapter = priv->adapter;

    ENTER();

    if (Adapter->InfrastructureMode != Wlan802_11IBSS) {
        LEAVE();
        return -EOPNOTSUPP;
    }
    if (copy_from_user(vals, wrq->u.data.pointer,
                       MIN(wrq->u.data.length, sizeof(vals)))) {
        PRINTM(INFO, "Copy from user failed\n");
        LEAVE();
        return -EFAULT;

    }
    switch (vals[0]) {          /* 1st byte is band */
    case 'B':
    case 'b':
        band = BAND_B;
        break;
    case 'G':
    case 'g':
        band = BAND_G;
        break;
    case 'A':
    case 'a':
        band = BAND_A;
        break;
    default:
        LEAVE();
        return -EINVAL;
    }

    /* Validate against FW support */
    if ((band | Adapter->fw_bands) & ~Adapter->fw_bands) {
        LEAVE();
        return -EINVAL;
    }
    chan = vals[1];

    if (!find_cfp_by_band_and_channel(Adapter, band, (u16) chan)) {
        PRINTM(INFO, "Invalid channel number %d\n", chan);
        LEAVE();
        return -EINVAL;
    }
    Adapter->adhoc_start_band = band;
    Adapter->AdhocChannel = chan;
    Adapter->AdhocAutoSel = FALSE;
    if (wlan_change_adhoc_chan(priv, chan) != WLAN_STATUS_SUCCESS) {
        PRINTM(INFO, "Fail to change adhoc channel number %d\n", chan);
        LEAVE();
        return -EINVAL;
    }
    LEAVE();

    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Get band 
 *  @param priv                 A pointer to wlan_private structure
 *  @param wrq			A pointer to iwreq structure
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_do_get_band_ioctl(wlan_private * priv, struct iwreq *wrq)
{
    char status[10], *ptr;
    u16 val = priv->adapter->config_bands;

    ENTER();

    memset(status, 0, sizeof(status));
    ptr = status;

    if (val & BAND_A)
        *ptr++ = 'a';

    if (val & BAND_B)
        *ptr++ = 'b';

    if (val & BAND_G)
        *ptr++ = 'g';

    *ptr = '\0';

    PRINTM(INFO, "Status = %s\n", status);
    wrq->u.data.length = strlen(status) + 1;

    if (wrq->u.data.pointer) {
        if (copy_to_user(wrq->u.data.pointer, &status, wrq->u.data.length)) {
            LEAVE();
            return -EFAULT;
        }
    }

    LEAVE();

    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Get adhoc channel 
 *  @param priv                 A pointer to wlan_private structure
 *  @param wrq			A pointer to iwreq structure
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_do_get_adhocch_ioctl(wlan_private * priv, struct iwreq *wrq)
{
    u32 status[2];
    wlan_adapter *Adapter = priv->adapter;
    u16 band = Adapter->adhoc_start_band;

    ENTER();

    if (band & BAND_A)
        status[0] = 'a';
    else if (band & BAND_B)
        status[0] = 'b';
    else if (band & BAND_G)
        status[0] = 'g';

    status[1] = Adapter->AdhocChannel;

    if (wrq->u.data.pointer) {
        wrq->u.data.length = sizeof(status);

        if (copy_to_user(wrq->u.data.pointer, status, wrq->u.data.length)) {
            LEAVE();
            return -EFAULT;
        }
    }

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Get/Set sleep period 
 *  @param priv                 A pointer to wlan_private structure
 *  @param wrq			A pointer to iwreq structure
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_sleep_period(wlan_private * priv, struct iwreq *wrq)
{
    int ret;
    int data;
    wlan_adapter *Adapter = priv->adapter;
    HostCmd_DS_802_11_SLEEP_PERIOD sleeppd;

    ENTER();

    if (wrq->u.data.length > 1) {
        LEAVE();
        return -ENOTSUPP;
    }
    memset(&sleeppd, 0, sizeof(sleeppd));
    memset(&Adapter->sleep_period, 0, sizeof(SleepPeriod));

    if (wrq->u.data.length == 0)
        sleeppd.Action = HostCmd_ACT_GEN_GET;
    else {
        if (copy_from_user(&data, wrq->u.data.pointer, sizeof(int))) {
            PRINTM(INFO, "Copy from user failed\n");
            LEAVE();
            return -EFAULT;
        }

        /* sleep period is 0 or 10~60 in milliseconds */
#define MIN_SLEEP_PERIOD		10
#define MAX_SLEEP_PERIOD		60
#define SLEEP_PERIOD_RESERVED_FF	0xFF
        if ((data <= MAX_SLEEP_PERIOD && data >= MIN_SLEEP_PERIOD) ||
            (data == 0)
            || (data == SLEEP_PERIOD_RESERVED_FF)       /* for UPSD
                                                           certification tests */
            ) {
            sleeppd.Action = HostCmd_ACT_GEN_SET;
            sleeppd.Period = data;
        } else {
            LEAVE();
            return -EINVAL;
        }
    }

    ret = wlan_prepare_cmd(priv, HostCmd_CMD_802_11_SLEEP_PERIOD,
                           0, HostCmd_OPTION_WAITFORRSP, 0, (void *) &sleeppd);

    data = (int) Adapter->sleep_period.period;
    if (copy_to_user(wrq->u.data.pointer, &data, sizeof(int))) {
        PRINTM(INFO, "Copy to user failed\n");
        LEAVE();
        return -EFAULT;
    }
    wrq->u.data.length = 1;

    LEAVE();
    return ret;
}

/** 
 *  @brief Get/Set adapt rate 
 *  @param priv                 A pointer to wlan_private structure
 *  @param wrq			A pointer to iwreq structure
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_adapt_rateset(wlan_private * priv, struct iwreq *wrq)
{
    int ret;
    wlan_adapter *Adapter = priv->adapter;
    int data[4];
    int rateindex;
    WLAN_802_11_RATES rates;
    u8 *rate;
    int i = 0;
    u16 rateBitmap = 0;
    u32 dataRate = 0;

    ENTER();

    memset(data, 0, sizeof(data));
    if (!wrq->u.data.length) {
        PRINTM(INFO, "Get ADAPT RATE SET\n");
        ret = wlan_prepare_cmd(priv,
                               HostCmd_CMD_802_11_RATE_ADAPT_RATESET,
                               HostCmd_ACT_GEN_GET, HostCmd_OPTION_WAITFORRSP,
                               0, NULL);
        data[0] = Adapter->HWRateDropMode;
        data[2] = Adapter->Threshold;
        data[3] = Adapter->FinalRate;
        wrq->u.data.length = 4;
        data[1] = Adapter->RateBitmap;
        if (copy_to_user
            (wrq->u.data.pointer, data, sizeof(int) * wrq->u.data.length)) {
            PRINTM(INFO, "Copy to user failed\n");
            LEAVE();
            return -EFAULT;
        }

    } else {
        PRINTM(INFO, "Set ADAPT RATE SET\n");
        if (wrq->u.data.length > 4) {
            LEAVE();
            return -EINVAL;
        }
        if (copy_from_user
            (data, wrq->u.data.pointer, sizeof(int) * wrq->u.data.length)) {
            PRINTM(INFO, "Copy from user failed\n");
            LEAVE();
            return -EFAULT;
        }

        if (data[0] > HW_SINGLE_RATE_DROP) {
            LEAVE();
            return -EINVAL;
        }
        Adapter->HWRateDropMode = data[0];
        Adapter->Threshold = data[2];
        Adapter->FinalRate = data[3];
        rateBitmap = data[1];
        rateindex = wlan_get_rate_index(rateBitmap);
        dataRate = index_to_data_rate(rateindex);
        memset(rates, 0, sizeof(rates));
        get_active_data_rates(Adapter, rates);
        rate = rates;

        for (i = 0; (rate[i] && i < WLAN_SUPPORTED_RATES); i++) {
            PRINTM(INFO, "Rate=0x%X  Wanted=0x%X\n", *rate, dataRate);
            if ((rate[i] & 0x7f) == (dataRate & 0x7f))
                break;
        }
        if (!rate[i] || (i == WLAN_SUPPORTED_RATES)) {
            PRINTM(MSG, "The fixed data rate 0x%X is out "
                   "of range\n", dataRate);
            LEAVE();
            return -EINVAL;
        }

        Adapter->RateBitmap = rateBitmap;
        Adapter->Is_DataRate_Auto = wlan_is_rate_auto(priv);
        if (Adapter->Is_DataRate_Auto)
            Adapter->DataRate = 0;
        else
            Adapter->DataRate = dataRate;

        PRINTM(INFO, "RateBitmap=%x,IsRateAuto=%d,DataRate=%d\n",
               Adapter->RateBitmap, Adapter->Is_DataRate_Auto,
               Adapter->DataRate);
        ret =
            wlan_prepare_cmd(priv, HostCmd_CMD_802_11_RATE_ADAPT_RATESET,
                             HostCmd_ACT_GEN_SET, HostCmd_OPTION_WAITFORRSP, 0,
                             NULL);
    }

    LEAVE();
    return ret;
}

/** 
 *  @brief Get LOG
 *  @param priv                 A pointer to wlan_private structure
 *  @param wrq			A pointer to iwreq structure
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_do_getlog_ioctl(wlan_private * priv, struct iwreq *wrq)
{
    int ret;
    char *buf = NULL;
    wlan_adapter *Adapter = priv->adapter;

    ENTER();

    PRINTM(INFO, " GET STATS\n");

    if (!(buf = kmalloc(GETLOG_BUFSIZE, GFP_KERNEL))) {
        PRINTM(INFO, "kmalloc failed!\n");
        LEAVE();
        return -ENOMEM;
    }

    ret = wlan_prepare_cmd(priv, HostCmd_CMD_802_11_GET_LOG,
                           0, HostCmd_OPTION_WAITFORRSP, 0, NULL);

    if (!ret && wrq->u.data.pointer) {
        sprintf(buf, "\n"
                "mcasttxframe     %u\n"
                "failed           %u\n"
                "retry            %u\n"
                "multiretry       %u\n"
                "framedup         %u\n"
                "rtssuccess       %u\n"
                "rtsfailure       %u\n"
                "ackfailure       %u\n"
                "rxfrag           %u\n"
                "mcastrxframe     %u\n"
                "fcserror         %u\n"
                "txframe          %u\n",
                Adapter->LogMsg.mcasttxframe,
                Adapter->LogMsg.failed,
                Adapter->LogMsg.retry,
                Adapter->LogMsg.multiretry,
                Adapter->LogMsg.framedup,
                Adapter->LogMsg.rtssuccess,
                Adapter->LogMsg.rtsfailure,
                Adapter->LogMsg.ackfailure,
                Adapter->LogMsg.rxfrag,
                Adapter->LogMsg.mcastrxframe,
                Adapter->LogMsg.fcserror, Adapter->LogMsg.txframe);
        wrq->u.data.length = strlen(buf) + 1;
        if (copy_to_user(wrq->u.data.pointer, buf, wrq->u.data.length)) {
            PRINTM(INFO, "Copy to user failed\n");
            ret = -EFAULT;
        }
    }

    kfree(buf);
    LEAVE();
    return ret;
}

/** 
 *  @brief config sleep parameters
 *  @param priv                 A pointer to wlan_private structure
 *  @param req			A pointer to ifreq structure
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_sleep_params_ioctl(wlan_private * priv, struct iwreq *wrq)
{
    int ret;
    wlan_adapter *Adapter = priv->adapter;
    wlan_ioctl_sleep_params_config sp;

    ENTER();

    memset(&sp, 0, sizeof(sp));

    if (!wrq->u.data.pointer) {
        LEAVE();
        return -EFAULT;
    }
    if (copy_from_user(&sp, wrq->u.data.pointer,
                       MIN(sizeof(sp), wrq->u.data.length))) {
        PRINTM(INFO, "Copy from user failed\n");
        LEAVE();
        return -EFAULT;
    }

    memcpy(&Adapter->sp, &sp.Error, sizeof(SleepParams));

    ret = wlan_prepare_cmd(priv, HostCmd_CMD_802_11_SLEEP_PARAMS,
                           sp.Action, HostCmd_OPTION_WAITFORRSP, 0, NULL);

    if (!ret && !sp.Action) {
        memcpy(&sp.Error, &Adapter->sp, sizeof(SleepParams));
        if (copy_to_user(wrq->u.data.pointer, &sp, sizeof(sp))) {
            PRINTM(INFO, "Copy to user failed\n");
            LEAVE();
            return -EFAULT;
        }
        wrq->u.data.length = sizeof(sp);
    }

    LEAVE();
    return ret;
}

/** 
 *  @brief Read the CIS Table
 *  @param priv                 A pointer to wlan_private structure
 *  @param req			A pointer to ifreq structure
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_do_getcis_ioctl(wlan_private * priv, struct ifreq *req)
{
    int ret = WLAN_STATUS_SUCCESS;
    struct iwreq *wrq = (struct iwreq *) req;
    char *cisinfo;
#define MAX_CIS_INFO_LEN (512)
    int cislen = MAX_CIS_INFO_LEN;

    ENTER();

    cisinfo = (char *) kmalloc(cislen, GFP_KERNEL);
    if (!cisinfo) {
        PRINTM(ERROR, "Memory allocation for cisinfo failed!\n");
        ret = WLAN_STATUS_FAILURE;
        goto done;
    }

    memset(cisinfo, 0, cislen);
    ret = sbi_get_cis_info(priv, (void *) cisinfo, &cislen);
    if (ret != WLAN_STATUS_SUCCESS) {
        PRINTM(INFO, "Failed to get CIS information from card\n");
        ret = -EFAULT;
        goto done;
    }

    if (wrq->u.data.pointer) {
        if (copy_to_user(wrq->u.data.pointer, cisinfo, cislen)) {
            PRINTM(INFO, "Copy to user failed\n");
            ret = -EFAULT;
            goto done;
        }
        wrq->u.data.length = cislen;
    }

  done:
    if (cisinfo)
        kfree(cisinfo);
    LEAVE();
    return ret;
}

/** 
 *  @brief Set BCA timeshare
 *  
 *  @param priv		A pointer to wlan_private structure
 *  @param wrq	   	A pointer to user data
 *  @return 	   	WLAN_STATUS_SUCCESS--success, otherwise fail
 */
static int
wlan_bca_timeshare_ioctl(wlan_private * priv, struct iwreq *wrq)
{
    int ret;
    wlan_adapter *Adapter = priv->adapter;
    wlan_ioctl_bca_timeshare_config bca_ts;

    ENTER();

    memset(&bca_ts, 0, sizeof(HostCmd_DS_802_11_BCA_TIMESHARE));

    if (!wrq->u.data.pointer) {
        LEAVE();
        return -EFAULT;
    }
    if (copy_from_user(&bca_ts, wrq->u.data.pointer,
                       MIN(sizeof(bca_ts), wrq->u.data.length))) {
        PRINTM(INFO, "Copy from user failed\n");
        LEAVE();
        return -EFAULT;
    }

    PRINTM(INFO, "TrafficType=%x TimeShareInterva=%x BTTime=%x\n",
           bca_ts.TrafficType, bca_ts.TimeShareInterval, bca_ts.BTTime);

    ret = wlan_prepare_cmd(priv,
                           HostCmd_CMD_802_11_BCA_CONFIG_TIMESHARE,
                           bca_ts.Action, HostCmd_OPTION_WAITFORRSP,
                           0, &bca_ts);

    if (!ret && !bca_ts.Action) {
        if (copy_to_user(wrq->u.data.pointer, &Adapter->bca_ts, sizeof(bca_ts))) {
            PRINTM(INFO, "Copy to user failed\n");
            LEAVE();
            return -EFAULT;
        }
        wrq->u.data.length = sizeof(HostCmd_DS_802_11_BCA_TIMESHARE);
    }

    LEAVE();
    return ret;
}

/** 
 *  @brief Set scan type
 *  
 *  @param priv		A pointer to wlan_private structure
 *  @param wrq	   	A pointer to user data
 *  @return 	   	WLAN_STATUS_SUCCESS--success, otherwise fail
 */
static int
wlan_scan_type_ioctl(wlan_private * priv, struct iwreq *wrq)
{
    u8 buf[12];
    u8 *option[] = { "active", "passive", "get", };
    int i, max_options = (sizeof(option) / sizeof(option[0]));
    int ret = WLAN_STATUS_SUCCESS;
    wlan_adapter *Adapter = priv->adapter;

    ENTER();

    if (wlan_get_state_11d(priv) == ENABLE_11D) {
        PRINTM(INFO, "11D: Cannot set scantype when 11D enabled\n");
        LEAVE();
        return -EFAULT;
    }

    memset(buf, 0, sizeof(buf));

    if (copy_from_user(buf, wrq->u.data.pointer, MIN(sizeof(buf),
                                                     wrq->u.data.length))) {
        PRINTM(INFO, "Copy from user failed\n");
        LEAVE();
        return -EFAULT;
    }

    PRINTM(INFO, "Scan Type Option = %s\n", buf);

    buf[sizeof(buf) - 1] = '\0';

    for (i = 0; i < max_options; i++)
        if (!strcmp(buf, option[i]))
            break;

    switch (i) {
    case 0:
        Adapter->ScanType = HostCmd_SCAN_TYPE_ACTIVE;
        break;
    case 1:
        Adapter->ScanType = HostCmd_SCAN_TYPE_PASSIVE;
        break;
    case 2:
        wrq->u.data.length = strlen(option[Adapter->ScanType]) + 1;

        if (copy_to_user(wrq->u.data.pointer,
                         option[Adapter->ScanType], wrq->u.data.length)) {
            PRINTM(INFO, "Copy to user failed\n");
            ret = -EFAULT;
        }

        break;
    default:
        PRINTM(INFO, "Invalid Scan Type Ioctl Option\n");
        ret = -EINVAL;
        break;
    }

    LEAVE();
    return ret;
}

/** 
 *  @brief Set scan mode
 *  
 *  @param priv		A pointer to wlan_private structure
 *  @param wrq	   	A pointer to user data
 *  @return 	   	WLAN_STATUS_SUCCESS--success, otherwise fail
 */
static int
wlan_scan_mode_ioctl(wlan_private * priv, struct iwreq *wrq)
{
    wlan_adapter *Adapter = priv->adapter;
    u8 buf[12];
    u8 *option[] = { "bss", "ibss", "any", "get" };
    int i, max_options = (sizeof(option) / sizeof(option[0]));
    int ret = WLAN_STATUS_SUCCESS;

    ENTER();

    memset(buf, 0, sizeof(buf));

    if (copy_from_user(buf, wrq->u.data.pointer, MIN(sizeof(buf),
                                                     wrq->u.data.length))) {
        PRINTM(INFO, "Copy from user failed\n");
        LEAVE();
        return -EFAULT;
    }

    PRINTM(INFO, "Scan Mode Option = %s\n", buf);

    buf[sizeof(buf) - 1] = '\0';

    for (i = 0; i < max_options; i++)
        if (!strcmp(buf, option[i]))
            break;

    switch (i) {

    case 0:
        Adapter->ScanMode = HostCmd_BSS_TYPE_BSS;
        break;
    case 1:
        Adapter->ScanMode = HostCmd_BSS_TYPE_IBSS;
        break;
    case 2:
        Adapter->ScanMode = HostCmd_BSS_TYPE_ANY;
        break;
    case 3:

        wrq->u.data.length = strlen(option[Adapter->ScanMode - 1]) + 1;

        PRINTM(INFO, "Get Scan Mode Option = %s\n",
               option[Adapter->ScanMode - 1]);

        PRINTM(INFO, "Scan Mode Length %d\n", wrq->u.data.length);

        if (copy_to_user(wrq->u.data.pointer,
                         option[Adapter->ScanMode - 1], wrq->u.data.length)) {
            PRINTM(INFO, "Copy to user failed\n");
            ret = -EFAULT;
        }
        PRINTM(INFO, "GET Scan Type Option after copy = %s\n",
               (char *) wrq->u.data.pointer);

        break;

    default:
        PRINTM(INFO, "Invalid Scan Mode Ioctl Option\n");
        ret = -EINVAL;
        break;
    }

    LEAVE();
    return ret;
}

/** 
 *  @brief Get/Set Adhoc G Rate
 *  
 *  @param priv		A pointer to wlan_private structure
 *  @param wrq	   	A pointer to user data
 *  @return 	   	WLAN_STATUS_SUCCESS--success, otherwise fail
 */
static int
wlan_do_set_grate_ioctl(wlan_private * priv, struct iwreq *wrq)
{
    wlan_adapter *Adapter = priv->adapter;
    int data, data1;
    int *val;

    ENTER();

    data1 = *((int *) (wrq->u.name + SUBCMD_OFFSET));
    switch (data1) {
    case 0:
        Adapter->adhoc_grate_enabled = FALSE;
        break;
    case 1:
        Adapter->adhoc_grate_enabled = TRUE;
        break;
    case 2:
        break;
    default:
        LEAVE();
        return -EINVAL;
    }
    data = Adapter->adhoc_grate_enabled;
    val = (int *) wrq->u.name;
    *val = data;
    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Get/Set Firmware wakeup method
 *  
 *  @param priv		A pointer to wlan_private structure
 *  @param wrq	   	A pointer to user data
 *  @return 	   	WLAN_STATUS_SUCCESS--success, otherwise fail
 */
static int
wlan_cmd_fw_wakeup_method(wlan_private * priv, struct iwreq *wrq)
{
    wlan_adapter *Adapter = priv->adapter;
    u16 action;
    u16 method;
    int ret;
    int data;

    ENTER();

    if (wrq->u.data.length == 0 || !wrq->u.data.pointer) {
        action = HostCmd_ACT_GEN_GET;
        method = Adapter->fwWakeupMethod;
    } else {
        action = HostCmd_ACT_GEN_SET;
        if (copy_from_user(&data, wrq->u.data.pointer, sizeof(int))) {
            PRINTM(INFO, "Copy from user failed\n");
            LEAVE();
            return -EFAULT;
        }

        switch (data) {
        case 0:
            method = WAKEUP_FW_UNCHANGED;
            break;
        case 1:
            method = WAKEUP_FW_THRU_INTERFACE;
            break;
        case 2:
            method = WAKEUP_FW_THRU_GPIO;
            break;
        default:
            LEAVE();
            return -EINVAL;
        }
    }

    ret = wlan_prepare_cmd(priv,
                           HostCmd_CMD_802_11_FW_WAKE_METHOD, action,
                           HostCmd_OPTION_WAITFORRSP, 0, &method);

    if (action == HostCmd_ACT_GEN_GET) {
        method = Adapter->fwWakeupMethod;
        if (copy_to_user(wrq->u.data.pointer, &method, sizeof(method))) {
            PRINTM(INFO, "Copy to user failed\n");
            LEAVE();
            return -EFAULT;
        }
        wrq->u.data.length = 1;
    }

    LEAVE();
    return ret;
}

/** 
 *  @brief Get/Set Auto Deep Sleep mode
 *  
 *  @param priv		A pointer to wlan_private structure
 *  @param wrq	   	A pointer to user data
 *  @return 	   	WLAN_STATUS_SUCCESS--success, otherwise fail
 */
static int
wlan_auto_deep_sleep(wlan_private * priv, struct iwreq *wrq)
{
    wlan_adapter *Adapter = priv->adapter;
    int data;

    ENTER();

    if (wrq->u.data.length > 0 && wrq->u.data.pointer) {
        if (copy_from_user(&data, wrq->u.data.pointer, sizeof(int))) {
            PRINTM(INFO, "Copy from user failed\n");
            LEAVE();
            return -EFAULT;
        }

        switch (data) {
        case 0:
            if (Adapter->IsAutoDeepSleepEnabled) {
                Adapter->IsAutoDeepSleepEnabled = FALSE;
                /* Try to exit DS if auto DS disabled */
                wlan_set_deep_sleep(priv, FALSE);
            }
            break;
        case 1:
            if (!Adapter->IsAutoDeepSleepEnabled) {
                Adapter->IsAutoDeepSleepEnabled = TRUE;
                /* Wakeup main thread to enter DS if auto DS enabled */
                wake_up_interruptible(&priv->MainThread.waitQ);
            }
            break;
        default:
            LEAVE();
            return -EINVAL;
        }
    }

    data = Adapter->IsAutoDeepSleepEnabled;
    if (copy_to_user(wrq->u.data.pointer, &data, sizeof(data))) {
        PRINTM(INFO, "Copy to user failed\n");
        LEAVE();
        return -EFAULT;
    }
    wrq->u.data.length = 1;

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Get/Set Enhanced PS mode
 *  
 *  @param priv		A pointer to wlan_private structure
 *  @param wrq	   	A pointer to user data
 *  @return 	   	WLAN_STATUS_SUCCESS--success, otherwise fail
 */
static int
wlan_enhanced_ps(wlan_private * priv, struct iwreq *wrq)
{
    wlan_adapter *Adapter = priv->adapter;
    int data;

    ENTER();

    if (wrq->u.data.length > 0 && wrq->u.data.pointer) {
        if (copy_from_user(&data, wrq->u.data.pointer, sizeof(int))) {
            PRINTM(INFO, "Copy from user failed\n");
            LEAVE();
            return -EFAULT;
        }

        switch (data) {
        case 0:
            Adapter->IsEnhancedPSEnabled = FALSE;
            break;
        case 1:
            Adapter->IsEnhancedPSEnabled = TRUE;
            break;
        default:
            LEAVE();
            return -EINVAL;
        }
    }

    data = Adapter->IsEnhancedPSEnabled;
    if (copy_to_user(wrq->u.data.pointer, &data, sizeof(data))) {
        PRINTM(INFO, "Copy to user failed\n");
        LEAVE();
        return -EFAULT;
    }
    wrq->u.data.length = 1;

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/**
 *  @brief Set MEF cfg 
 *
 *  @param priv     A pointer to wlan_private structure
 *  @param wrq      A pointer to iwreq structure
 *
 *  @return         WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_set_mef_cfg(wlan_private * priv, struct iwreq *wrq)
{
    u8 *tempResponseBuffer = NULL;
    MEF_CFG_DATA *pMefData = NULL;
    int ret = WLAN_STATUS_SUCCESS;

    ENTER();

    if ((wrq->u.data.pointer == NULL) ||
        (wrq->u.data.length < sizeof(HostCmd_DS_MEF_CFG))) {
        PRINTM(INFO, "wlan_set_mef_cfg corrupt data: pointer=%p, length=%d\n",
               wrq->u.data.pointer, wrq->u.data.length);
        LEAVE();
        return -EFAULT;
    }
    if (!(tempResponseBuffer = kmalloc(MRVDRV_SIZE_OF_CMD_BUFFER, GFP_KERNEL))) {
        PRINTM(INFO, "ERROR: Failed to allocate response buffer!\n");
        LEAVE();
        return -ENOMEM;
    }
    pMefData = (MEF_CFG_DATA *) tempResponseBuffer;

    /* 
     * Copy the whole command into the command buffer 
     */
    if (copy_from_user
        ((u8 *) & pMefData->data, wrq->u.data.pointer, wrq->u.data.length)) {
        PRINTM(INFO, "Copy from user failed\n");
        kfree(tempResponseBuffer);
        LEAVE();
        return -EFAULT;
    }
    pMefData->size = wrq->u.data.length;
    ret = wlan_prepare_cmd(priv,
                           HostCmd_CMD_MEF_CFG, 0,
                           HostCmd_OPTION_WAITFORRSP, 0, tempResponseBuffer);
    kfree(tempResponseBuffer);
    LEAVE();
    return ret;

}

/**
 *  @brief Get the CFP table based on the region code
 *
 *  @param priv     A pointer to wlan_private structure
 *  @param wrq      A pointer to iwreq structure
 *
 *  @return         WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_get_cfp_table_ioctl(wlan_private * priv, struct iwreq *wrq)
{
    wlan_adapter *Adapter = priv->adapter;
    pwlan_ioctl_cfp_table ioctl_cfp;
    CHANNEL_FREQ_POWER *cfp;
    int cfp_no;
    int regioncode;
    int ret = WLAN_STATUS_SUCCESS;

    ENTER();

    if (wrq->u.data.length == 0 || !wrq->u.data.pointer) {
        ret = -EINVAL;
        goto cfpexit;
    }

    ioctl_cfp = (pwlan_ioctl_cfp_table) wrq->u.data.pointer;

    if (copy_from_user(&regioncode, &ioctl_cfp->region, sizeof(int))) {
        PRINTM(INFO, "Get CFP table: copy from user failed\n");
        ret = -EFAULT;
        goto cfpexit;
    }

    if (!regioncode)
        regioncode = Adapter->RegionCode;

    cfp =
        wlan_get_region_cfp_table((u8) regioncode, (u8) Adapter->config_bands,
                                  &cfp_no);

    if (cfp == NULL) {
        PRINTM(MSG, "No related CFP table found, region code = 0x%x\n",
               regioncode);
        ret = -EFAULT;
        goto cfpexit;
    }

    if (copy_to_user(&ioctl_cfp->cfp_no, &cfp_no, sizeof(int))) {
        PRINTM(INFO, "Get CFP table: copy to user failed\n");
        ret = -EFAULT;
        goto cfpexit;
    }

    if (copy_to_user(ioctl_cfp->cfp, cfp, sizeof(CHANNEL_FREQ_POWER) * cfp_no)) {
        PRINTM(INFO, "Get CFP table: copy to user failed\n");
        ret = -EFAULT;
        goto cfpexit;
    }

    wrq->u.data.length = sizeof(int) * 2 + sizeof(CHANNEL_FREQ_POWER) * cfp_no;

  cfpexit:
    LEAVE();
    return ret;
}

/**
 *  @brief Get firmware memory
 *
 *  @param priv     A pointer to wlan_private structure
 *  @param wrq      A pointer to iwreq structure
 *
 *  @return         WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_get_firmware_mem(wlan_private * priv, struct iwreq *wrq)
{
    HostCmd_DS_GET_MEM *pGetMem;
    u8 *buf = NULL;
    FW_MEM_DATA *pFwData;
    int ret = WLAN_STATUS_SUCCESS;

    ENTER();

    if ((wrq->u.data.length < sizeof(HostCmd_DS_GET_MEM)) ||
        !wrq->u.data.pointer) {
        ret = -EINVAL;
        LEAVE();
        return ret;
    }
    if (!(buf = kmalloc(MRVDRV_SIZE_OF_CMD_BUFFER, GFP_KERNEL))) {
        PRINTM(INFO, "allocate  buffer failed!\n");
        LEAVE();
        return -ENOMEM;
    }
    memset(buf, 0, MRVDRV_SIZE_OF_CMD_BUFFER);
    pFwData = (FW_MEM_DATA *) buf;
    pGetMem = &pFwData->data;

    if (copy_from_user
        (pGetMem, wrq->u.data.pointer, sizeof(HostCmd_DS_GET_MEM))) {
        PRINTM(INFO, "Get Mem: copy from user failed\n");
        ret = -EFAULT;
        goto memexit;
    }

    ret = wlan_prepare_cmd(priv,
                           HostCmd_CMD_GET_MEM, 0,
                           HostCmd_OPTION_WAITFORRSP, 0, buf);
    if (!ret) {
        wrq->u.data.length = pFwData->size;
        if (copy_to_user
            (wrq->u.data.pointer, (u8 *) pGetMem, wrq->u.data.length)) {
            PRINTM(INFO, "Get Mem: copy to user failed\n");
            ret = -EFAULT;
            goto memexit;
        }
    }
  memexit:
    kfree(buf);
    LEAVE();
    return ret;
}

/**
 *  @brief  Retrieve transmit packet statistics from the firmware
 *
 *  @param priv     A pointer to wlan_private structure
 *  @param wrq      A pointer to iwreq structure
 *
 *  @return         WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_tx_pkt_stats_ioctl(wlan_private * priv, struct iwreq *wrq)
{
    HostCmd_DS_TX_PKT_STATS txPktStats;
    int ret;

    ENTER();

    if (wrq->u.data.length == 0 || !wrq->u.data.pointer) {
        LEAVE();
        return -EINVAL;
    }

    if (wrq->u.data.length < sizeof(txPktStats)) {
        LEAVE();
        return -E2BIG;
    }

    memset(&txPktStats, 0x00, sizeof(txPktStats));

    if ((ret = wlan_prepare_cmd(priv,
                                HostCmd_CMD_TX_PKT_STATS, 0,
                                HostCmd_OPTION_WAITFORRSP, 0, &txPktStats))) {
        LEAVE();
        return ret;
    }

    if (copy_to_user(wrq->u.data.pointer,
                     (u8 *) & txPktStats, sizeof(txPktStats))) {
        PRINTM(INFO, "TxPktStats: copy to user failed\n");
        LEAVE();
        return -EFAULT;
    }

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief Power up/down the specified host interface
 *   
 *  @param priv		A pointer to wlan_private structure
 *  @param wreq		A pointer to iwreq structure
 *  @return    		WLAN_STATUS_SUCCESS --success, otherwise fail
 */
static int
wlan_func_intf_ctrl(wlan_private * priv, struct iwreq *wrq)
{
    int data[2] = { 0, 0 };
    HostCmd_DS_FUNC_IF_CTRL intfctrl;
    int ret = WLAN_STATUS_FAILURE;

    ENTER();

    if (wrq->u.data.length == 2) {
        if (copy_from_user
            (data, wrq->u.data.pointer, sizeof(int) * wrq->u.data.length)) {
            PRINTM(INFO, "Copy from user failed\n");
            LEAVE();
            return -EFAULT;
        }

        /* Power up (1)/down (0) */
        intfctrl.Action = (data[0]) ? 1 : 0;
        /* Interface all(0)/WLAN(1)/BT(2)/.. */
        intfctrl.Interface = (u16) data[1];

        ret = wlan_prepare_cmd(priv,
                               HostCmd_CMD_FUNC_IF_CTRL,
                               0, HostCmd_OPTION_WAITFORRSP,
                               0, (void *) &intfctrl);
    }

    LEAVE();
    return ret;
}

/********************************************************
		Global Functions
********************************************************/
/** 
 *  @brief Set Radio On/OFF
 *   
 *  @param priv  		A pointer to wlan_private structure
 *  @param option 		Radio Option
 *  @return 	   		WLAN_STATUS_SUCCESS --success, otherwise fail 
 */
int
wlan_radio_ioctl(wlan_private * priv, u8 option)
{
    int ret = 0;
    wlan_adapter *Adapter = priv->adapter;

    ENTER();

    if (Adapter->RadioOn != option) {
        if (Adapter->MediaConnectStatus == WlanMediaStateConnected) {
            PRINTM(MSG, "Cannot turn radio off in connected state\n");
            LEAVE();
            return -EINVAL;
        }

        PRINTM(INFO, "Switching %s the Radio\n", option ? "On" : "Off");
        Adapter->RadioOn = option;

        ret = wlan_prepare_cmd(priv,
                               HostCmd_CMD_802_11_RADIO_CONTROL,
                               HostCmd_ACT_GEN_SET,
                               HostCmd_OPTION_WAITFORRSP, 0, NULL);
    }

    LEAVE();
    return ret;
}

/** 
 *  @brief ioctl function - entry point
 *  
 *  @param dev		A pointer to net_device structure
 *  @param req	   	A pointer to ifreq structure
 *  @param cmd 		command
 *  @return 	   	WLAN_STATUS_SUCCESS--success, otherwise fail
 */
int
wlan_do_ioctl(struct net_device *dev, struct ifreq *req, int cmd)
{
    int subcmd = 0;
    int idata = 0;
    int *pdata;
    int ret = WLAN_STATUS_SUCCESS;
    wlan_private *priv = netdev_priv(dev);
    wlan_adapter *Adapter = priv->adapter;
    struct iwreq *wrq = (struct iwreq *) req;

    ENTER();

    if (Adapter->bHostSleepConfigured) {
        BOOLEAN cmd_allowed = FALSE;
        int count = sizeof(Commands_Allowed_In_HostSleep)
            / sizeof(Commands_Allowed_In_HostSleep[0]);

        if (cmd == WLAN_SET_GET_SIXTEEN_INT &&
            ((int) wrq->u.data.flags == WLANHSCFG)) {
            u32 cond;
            if (copy_from_user(&cond, wrq->u.data.pointer, sizeof(cond))) {
                PRINTM(INFO, "Copy from user failed\n");
                LEAVE();
                return -EFAULT;
            }

            if (cond == HOST_SLEEP_CFG_CANCEL) {
                cmd_allowed = TRUE;
                if (!Adapter->IsAutoDeepSleepEnabled && Adapter->IsDeepSleep) {
                    wlan_set_deep_sleep(priv, FALSE);
                }
            }
        } else
            if (wlan_is_cmd_allowed_in_ds_hs
                (req, cmd, Commands_Allowed_In_HostSleep, count)) {
            cmd_allowed = TRUE;
        }
        if (!cmd_allowed) {
            PRINTM(MSG, "%s IOCTLS called when WLAN access is blocked\n",
                   __FUNCTION__);
            LEAVE();
            return -EBUSY;
        }
    }

    if (!Adapter->IsAutoDeepSleepEnabled) {
        if (Adapter->IsDeepSleep) {
            int count = sizeof(Commands_Allowed_In_DeepSleep)
                / sizeof(Commands_Allowed_In_DeepSleep[0]);

            if (!wlan_is_cmd_allowed_in_ds_hs
                (req, cmd, Commands_Allowed_In_DeepSleep, count)) {
                PRINTM(MSG,
                       "():%s IOCTLS called when station is" " in DeepSleep\n",
                       __FUNCTION__);
                LEAVE();
                return -EBUSY;
            }
        }
    } else {
        int count = sizeof(Commands_DisAllowed_In_AutoDeepSleep) /
            sizeof(Commands_DisAllowed_In_AutoDeepSleep[0]);
        if (wlan_is_cmd_disallowed_in_ads(req, cmd,
                                          Commands_DisAllowed_In_AutoDeepSleep,
                                          count)) {
            PRINTM(MSG, "Command is not allowed in AutoDeepSleep mode\n");
            LEAVE();
            return -EBUSY;
        }
    }

    PRINTM(INFO, "wlan_do_ioctl: ioctl cmd = 0x%x\n", cmd);
    switch (cmd) {
    case WLANEXTSCAN:
        ret = wlan_extscan_ioctl(priv, req);
        break;
    case WLANHOSTCMD:
        ret = wlan_hostcmd_ioctl(dev, req, cmd);
        break;
    case WLANARPFILTER:
        ret = wlan_arpfilter_ioctl(dev, req, cmd);
        break;

    case WLANCISDUMP:          /* Read CIS Table */
        ret = wlan_do_getcis_ioctl(priv, req);
        break;

    case WLANSCAN_TYPE:
        PRINTM(INFO, "Scan Type Ioctl\n");
        ret = wlan_scan_type_ioctl(priv, wrq);
        break;

    case WLAN_SETADDR_GETNONE:
        switch (wrq->u.data.flags) {
        case WLANDEAUTH:
            {
                struct sockaddr saddr;

                PRINTM(INFO, "Deauth\n");

                if (wrq->u.data.length) {
                    if (copy_from_user(&saddr,
                                       wrq->u.data.pointer, sizeof(saddr))) {
                        PRINTM(INFO, "Copy from user failed\n");
                        LEAVE();
                        return -EFAULT;
                    }
                    wmm_stop_queue(priv);
                    ret =
                        wlan_prepare_cmd(priv,
                                         HostCmd_CMD_802_11_DEAUTHENTICATE, 0,
                                         HostCmd_OPTION_WAITFORRSP, 0, &saddr);

                    if (Adapter->MediaConnectStatus == WlanMediaStateConnected) {
                        wmm_start_queue(priv);
                        os_carrier_on(priv);
                        os_start_queue(priv);
                    }
                } else
                    ret = wlan_disconnect(priv);

                break;
            }

        default:
            ret = -EOPNOTSUPP;
            break;
        }
        break;

#ifdef MFG_CMD_SUPPORT
    case WLANMANFCMD:
        PRINTM(INFO, "Entering the Manufacturing ioctl SIOCCFMFG\n");
        ret = wlan_mfg_command(priv, wrq);

        PRINTM(INFO, "Manufacturing Ioctl %s\n", (ret) ? "failed" : "success");
        break;
#endif
    case WLANREGRDWR:          /* Register read write command */
        ret = wlan_regrdwr_ioctl(priv, req);
        break;
    case WLAN_SET_GET_256_CHAR:
        switch (wrq->u.data.flags) {
        default:
            ret = -EOPNOTSUPP;
            break;
        }
        break;

    case WLANCMD52RDWR:        /* CMD52 read/write command */
        ret = wlan_cmd52rdwr_ioctl(priv, req);
        break;

    case WLANCMD53RDWR:        /* CMD53 read/write command */
        ret = wlan_cmd53rdwr_ioctl(priv, req);
        break;

    case WLAN_SETNONE_GETNONE: /* set WPA mode on/off ioctl #20 */
        switch (wrq->u.data.flags) {
        case WLANADHOCSTOP:
            PRINTM(INFO, "Adhoc stop\n");
            ret = wlan_do_adhocstop_ioctl(priv);
            break;

        case WLANRADIOON:
            wlan_radio_ioctl(priv, RADIO_ON);
            break;

        case WLANRADIOOFF:
            ret = wlan_radio_ioctl(priv, RADIO_OFF);
            break;
        case WLANREMOVEADHOCAES:
            ret = wlan_remove_aes(priv);
            break;
        case WLANCRYPTOTEST:
            ret = wlan_crypto_test(priv, wrq);
            break;
#ifdef REASSOCIATION
        case WLANREASSOCIATIONAUTO:
            reassociation_on(priv);
            break;
        case WLANREASSOCIATIONUSER:
            reassociation_off(priv);
            break;
#endif /* REASSOCIATION */
        case WLANWLANIDLEON:
            wlanidle_on(priv);
            break;
        case WLANWLANIDLEOFF:
            wlanidle_off(priv);
            break;
        }                       /* End of switch */
        break;

    case WLAN_SETWORDCHAR_GETNONE:
        switch (wrq->u.data.flags) {
        case WLANSETADHOCAES:
            ret = wlan_setadhocaes_ioctl(priv, req);
            break;
        }
        break;

    case WLAN_SETONEINT_GETWORDCHAR:
        switch (wrq->u.data.flags) {
        case WLANGETADHOCAES:
            ret = wlan_getadhocaes_ioctl(priv, req);
            break;
        case WLANVERSION:      /* Get driver version */
            ret = wlan_version_ioctl(priv, req);
            break;
        case WLANVEREXT:
            ret = wlan_verext_ioctl(priv, wrq);
            break;
        }
        break;

    case WLANSETWPAIE:
        ret = wlan_set_wpa_ie_ioctl(priv, req);
        break;
    case WLAN_SETINT_GETINT:
        /* The first 4 bytes of req->ifr_data is sub-ioctl number after 4 bytes 
           sits the payload. */
        subcmd = (int) req->ifr_data;   /* from iwpriv subcmd */
        switch (subcmd) {
        case WLANNF:
            ret = wlan_get_nf(priv, wrq);
            break;
        case WLANRSSI:
            ret = wlan_get_rssi(priv, wrq);
            break;
        case WLANBGSCAN:
            {
                int data, data1;
                int *val;
                data1 = *((int *) (wrq->u.name + SUBCMD_OFFSET));
                switch (data1) {
                case CMD_DISABLED:
                    PRINTM(INFO, "Background scan is set to disable\n");
                    ret = wlan_bg_scan_enable(priv, FALSE);
                    val = (int *) wrq->u.name;
                    *val = data1;
                    break;
                case CMD_ENABLED:
                    PRINTM(INFO, "Background scan is set to enable\n");
                    ret = wlan_bg_scan_enable(priv, TRUE);
                    val = (int *) wrq->u.name;
                    *val = data1;
                    break;
                case CMD_GET:
                    data = (Adapter->bgScanConfig->Enable == TRUE) ?
                        CMD_ENABLED : CMD_DISABLED;
                    val = (int *) wrq->u.name;
                    *val = data;
                    break;
                default:
                    ret = -EINVAL;
                    PRINTM(INFO, "Background scan: wrong parameter\n");
                    break;
                }
            }
            break;
        case WLANENABLE11D:
            ret = wlan_cmd_enable_11d(priv, wrq);
            break;
        case WLANADHOCGRATE:
            ret = wlan_do_set_grate_ioctl(priv, wrq);
            break;
        case WLANSDIOCLOCK:
            {
                int data;
                int *val;
                data = *((int *) (wrq->u.name + SUBCMD_OFFSET));
                switch (data) {
                case CMD_DISABLED:
                    PRINTM(INFO, "SDIO clock is turned off\n");
                    ret = sbi_set_bus_clock(priv, FALSE);
                    break;
                case CMD_ENABLED:
                    PRINTM(INFO, "SDIO clock is turned on\n");
                    ret = sbi_set_bus_clock(priv, TRUE);
                    break;
                case CMD_GET:  /* need an API in sdio.c to get STRPCL */
                default:
                    ret = -EINVAL;
                    PRINTM(INFO, "sdioclock: wrong parameter\n");
                    break;
                }
                val = (int *) wrq->u.name;
                *val = data;
            }
            break;
        case WLANWMM_ENABLE:
            ret = wlan_wmm_enable_ioctl(priv, wrq);
            break;
        case WLANNULLGEN:
            ret = wlan_null_pkg_gen(priv, wrq);
            /* enable/disable null pkg generation */
            break;
        case WLANADHOCCSET:
            ret = wlan_set_coalescing_ioctl(priv, wrq);
            break;
        case WLAN_ADHOC_G_PROT:
            ret = wlan_adhoc_g_protection(priv, wrq);
            break;
        }
        break;

    case WLAN_SETONEINT_GETONEINT:
        switch (wrq->u.data.flags) {
        case WLAN_11H_SETLOCALPOWER:
            ret = wlan_11h_ioctl_get_local_power(priv, wrq);
            break;

        case WLAN_WMM_QOSINFO:
            {
                int data;
                if (wrq->u.data.length == 1) {
                    if (copy_from_user(&data, wrq->u.data.pointer, sizeof(int))) {
                        PRINTM(INFO, "Copy from user failed\n");
                        LEAVE();
                        return -EFAULT;
                    }
                    Adapter->wmm.qosinfo = (u8) data;
                } else {
                    data = (int) Adapter->wmm.qosinfo;
                    if (copy_to_user(wrq->u.data.pointer, &data, sizeof(int))) {
                        PRINTM(INFO, "Copy to user failed\n");
                        LEAVE();
                        return -EFAULT;
                    }
                    wrq->u.data.length = 1;
                }
            }
            break;
        case WLAN_LISTENINTRVL:
            if (!wrq->u.data.length) {
                int data;
                PRINTM(INFO, "Get LocalListenInterval Value\n");
#define GET_ONE_INT	1
                data = Adapter->LocalListenInterval;
                if (copy_to_user(wrq->u.data.pointer, &data, sizeof(int))) {
                    PRINTM(INFO, "Copy to user failed\n");
                    LEAVE();
                    return -EFAULT;
                }

                wrq->u.data.length = GET_ONE_INT;
            } else {
                int data;
                if (copy_from_user(&data, wrq->u.data.pointer, sizeof(int))) {
                    PRINTM(INFO, "Copy from user failed\n");
                    LEAVE();
                    return -EFAULT;
                }

                PRINTM(INFO, "Set LocalListenInterval = %d\n", data);
#define MAX_U16_VAL	65535
                if (data > MAX_U16_VAL) {
                    PRINTM(INFO, "Exceeds U16 value\n");
                    LEAVE();
                    return -EINVAL;
                }
                Adapter->LocalListenInterval = data;
            }
            break;
        case WLAN_FW_WAKEUP_METHOD:
            ret = wlan_cmd_fw_wakeup_method(priv, wrq);
            break;
        case WLAN_NULLPKTINTERVAL:
            ret = wlan_null_pkt_interval(priv, wrq);
            break;
        case WLAN_BCN_MISS_TIMEOUT:
            ret = wlan_bcn_miss_timeout(priv, wrq);
            break;
        case WLAN_ADHOC_AWAKE_PERIOD:
            ret = wlan_adhoc_awake_period(priv, wrq);
            break;
        case WLAN_LDO:
            ret = wlan_ldo_config(priv, wrq);
            break;
        case WLAN_RTS_CTS_CTRL:
            ret = wlan_rts_cts_ctrl(priv, wrq);
            break;
        case WLAN_MODULE_TYPE:
            ret = wlan_module_type_config(priv, wrq);
            break;
        case WLAN_AUTODEEPSLEEP:
            ret = wlan_auto_deep_sleep(priv, wrq);
            break;
        case WLAN_ENHANCEDPS:
            ret = wlan_enhanced_ps(priv, wrq);
            break;
        case WLAN_WAKEUP_MT:
            if (wrq->u.data.length > 0)
                Adapter->IntCounter++;
            wake_up_interruptible(&priv->MainThread.waitQ);
            break;
        default:
            ret = -EOPNOTSUPP;
            break;
        }
        break;

    case WLAN_SETONEINT_GETNONE:
        /* The first 4 bytes of req->ifr_data is sub-ioctl number after 4 bytes 
           sits the payload. */
        subcmd = wrq->u.data.flags;     /* from wpa_supplicant subcmd */

        if (!subcmd)
            subcmd = (int) req->ifr_data;       /* from iwpriv subcmd */

        idata = *((int *) (wrq->u.name + SUBCMD_OFFSET));

        switch (subcmd) {
        case WLAN_SUBCMD_SETRXANTENNA: /* SETRXANTENNA */
            ret = wlan_set_rx_antenna(priv, idata);
            break;
        case WLAN_SUBCMD_SETTXANTENNA: /* SETTXANTENNA */
            ret = wlan_set_tx_antenna(priv, idata);
            break;

        case WLANSETBCNAVG:
            if (idata == 0)
                Adapter->bcn_avg_factor = DEFAULT_BCN_AVG_FACTOR;
            else if (idata > MAX_BCN_AVG_FACTOR || idata < MIN_BCN_AVG_FACTOR) {
                PRINTM(MSG, "The value '%u' is out of the range (0-%u)\n",
                       idata, MAX_BCN_AVG_FACTOR);
                LEAVE();
                return -EINVAL;
            } else
                Adapter->bcn_avg_factor = idata;
            break;
        case WLANSETDATAAVG:
            if (idata == 0)
                Adapter->data_avg_factor = DEFAULT_DATA_AVG_FACTOR;
            else if (idata > MAX_DATA_AVG_FACTOR || idata < MIN_DATA_AVG_FACTOR) {
                PRINTM(MSG, "The value '%u' is out of the range (0-%u)\n",
                       idata, MAX_DATA_AVG_FACTOR);
                LEAVE();
                return -EINVAL;
            } else
                Adapter->data_avg_factor = idata;
            break;
        case WLANASSOCIATE:
            ret = wlan_associate_to_table_idx(priv, idata);
            break;

        case WLANSETREGION:
            ret = wlan_set_region(priv, (u16) idata);
            break;

        case WLAN_SET_LISTEN_INTERVAL:
            Adapter->ListenInterval = (u16) idata;
            break;

        case WLAN_SET_MULTIPLE_DTIM:
            ret = wlan_set_multiple_dtim_ioctl(priv, req);
            break;

        case WLANSETAUTHALG:
            ret = wlan_setauthalg_ioctl(priv, req);
            break;

        case WLANSETENCRYPTIONMODE:
            ret = wlan_setencryptionmode_ioctl(priv, req);
            break;

        default:
            ret = -EOPNOTSUPP;
            break;
        }

        break;

    case WLAN_SETNONE_GETTWELVE_CHAR:  /* Get Antenna settings */
        /* 
         * We've not used IW_PRIV_TYPE_FIXED so sub-ioctl number is
         * in flags of iwreq structure, otherwise it will be in
         * mode member of iwreq structure.
         */
        switch ((int) wrq->u.data.flags) {
        case WLAN_SUBCMD_GETRXANTENNA: /* Get Rx Antenna */
            ret = wlan_subcmd_getrxantenna_ioctl(priv, req);
            break;

        case WLAN_SUBCMD_GETTXANTENNA: /* Get Tx Antenna */
            ret = wlan_subcmd_gettxantenna_ioctl(priv, req);
            break;

        case WLAN_GET_TSF:
            ret = wlan_get_tsf_ioctl(priv, wrq);
            break;

        case WLAN_WPS_SESSION:
            ret = wlan_do_wps_session_ioctl(priv, wrq);
            break;
        }
        break;

    case WLANDEEPSLEEP:
        ret = wlan_deepsleep_ioctl(priv, req);
        break;

    case WLAN_SET64CHAR_GET64CHAR:
        switch ((int) wrq->u.data.flags) {
        case WLAN_11H_REQUESTTPC:
            ret = wlan_11h_ioctl_request_tpc(priv, wrq);
            break;
        case WLAN_11H_SETPOWERCAP:
            ret = wlan_11h_ioctl_set_power_cap(priv, wrq);
            break;
        case WLAN_MEASREQ:
            ret = wlan_meas_ioctl_send_req(priv, wrq);
            break;

        case WLANSLEEPPARAMS:
            ret = wlan_sleep_params_ioctl(priv, wrq);
            break;

        case WLAN_BCA_TIMESHARE:
            ret = wlan_bca_timeshare_ioctl(priv, wrq);
            break;
        case WLANSCAN_MODE:
            PRINTM(INFO, "Scan Mode Ioctl\n");
            ret = wlan_scan_mode_ioctl(priv, wrq);
            break;

        case WLAN_GET_ADHOC_STATUS:
            ret = wlan_get_adhoc_status_ioctl(priv, wrq);
            break;
        case WLAN_SET_GEN_IE:
            ret = wlan_set_gen_ie_ioctl(priv, wrq);
            break;
        case WLAN_GET_GEN_IE:
            ret = wlan_get_gen_ie_ioctl(priv, wrq);
            break;
        case WLAN_WMM_QUEUE_STATUS:
            ret = wlan_wmm_queue_status_ioctl(priv, wrq);
            break;
        case WLAN_WMM_TS_STATUS:
            ret = wlan_wmm_ts_status_ioctl(priv, wrq);
            break;
        }
        break;

    case WLAN_SETCONF_GETCONF:
        PRINTM(INFO, "The WLAN_SETCONF_GETCONF=0x%x is %d\n",
               WLAN_SETCONF_GETCONF, *(u32 *) req->ifr_data);
        switch (*(u32 *) req->ifr_data) {
        case BG_SCAN_CONFIG:
            ret = wlan_do_bg_scan_config_ioctl(priv, req);
            break;
        case BG_SCAN_CFG:
            ret = wlan_do_bgscfg_ioctl(priv, req);
            break;
        }
        break;

    case WLAN_SETNONE_GETONEINT:
        switch ((int) req->ifr_data) {
        case WLANGETBCNAVG:
            pdata = (int *) wrq->u.name;
            *pdata = (int) Adapter->bcn_avg_factor;
            break;

        case WLANGETDATAAVG:
            pdata = (int *) wrq->u.name;
            *pdata = (int) Adapter->data_avg_factor;
            break;

        case WLANGETREGION:
            pdata = (int *) wrq->u.name;
            *pdata = (int) Adapter->RegionCode;
            break;

        case WLAN_GET_LISTEN_INTERVAL:
            pdata = (int *) wrq->u.name;
            *pdata = (int) Adapter->ListenInterval;
            break;

        case WLAN_GET_MULTIPLE_DTIM:
            pdata = (int *) wrq->u.name;
            *pdata = (int) Adapter->MultipleDtim;
            break;
        case WLAN_GET_TX_RATE:
            ret = wlan_get_txrate_ioctl(priv, req);
            break;
        case WLANGETDTIM:
            ret = wlan_get_dtim_ioctl(priv, req);
            break;
        default:
            ret = -EOPNOTSUPP;

        }

        break;

    case WLAN_SETTENCHAR_GETNONE:
        switch ((int) wrq->u.data.flags) {
        case WLAN_SET_BAND:
            ret = wlan_do_setband_ioctl(priv, wrq);
            break;

        case WLAN_SET_ADHOC_CH:
            ret = wlan_do_setadhocch_ioctl(priv, wrq);
            break;

        case WLAN_11H_CHANSWANN:
            ret = wlan_11h_ioctl_chan_sw_ann(priv, wrq);
            break;
        }
        break;

    case WLAN_SETNONE_GETTENCHAR:
        switch ((int) wrq->u.data.flags) {
        case WLAN_GET_BAND:
            ret = wlan_do_get_band_ioctl(priv, wrq);
            break;

        case WLAN_GET_ADHOC_CH:
            ret = wlan_do_get_adhocch_ioctl(priv, wrq);
            break;
        }
        break;

    case WLANGETLOG:
        ret = wlan_do_getlog_ioctl(priv, wrq);
        break;

    case WLAN_SET_GET_SIXTEEN_INT:
        switch ((int) wrq->u.data.flags) {
        case WLAN_TPCCFG:
            {
                int data[5];
                HostCmd_DS_802_11_TPC_CFG cfg;
                memset(&cfg, 0, sizeof(cfg));
                if ((wrq->u.data.length > 1) && (wrq->u.data.length != 5)) {
                    LEAVE();
                    return WLAN_STATUS_FAILURE;
                }
                if (wrq->u.data.length == 0)
                    cfg.Action = HostCmd_ACT_GEN_GET;

                else {
                    if (copy_from_user(data,
                                       wrq->u.data.pointer, sizeof(int) * 5)) {
                        PRINTM(INFO, "Copy from user failed\n");
                        LEAVE();
                        return -EFAULT;
                    }

                    cfg.Action = HostCmd_ACT_GEN_SET;
                    cfg.Enable = data[0];
                    cfg.UseSNR = data[1];
#define TPC_DATA_NO_CHANG	0x7f
                    if (wrq->u.data.length == 1) {
                        cfg.P0 = TPC_DATA_NO_CHANG;
                        cfg.P1 = TPC_DATA_NO_CHANG;
                        cfg.P2 = TPC_DATA_NO_CHANG;
                    } else {
                        cfg.P0 = data[2];
                        cfg.P1 = data[3];
                        cfg.P2 = data[4];
                    }
                }

                ret = wlan_prepare_cmd(priv,
                                       HostCmd_CMD_802_11_TPC_CFG, 0,
                                       HostCmd_OPTION_WAITFORRSP, 0,
                                       (void *) &cfg);

                data[0] = cfg.Enable;
                data[1] = cfg.UseSNR;
                data[2] = cfg.P0;
                data[3] = cfg.P1;
                data[4] = cfg.P2;
                if (copy_to_user(wrq->u.data.pointer, data, sizeof(int) * 5)) {
                    PRINTM(INFO, "Copy to user failed\n");
                    LEAVE();
                    return -EFAULT;
                }

                wrq->u.data.length = 5;
            }
            break;

        case WLAN_SCANPROBES:
            {
                int data;
                if (wrq->u.data.length > 0) {
                    if (copy_from_user(&data, wrq->u.data.pointer, sizeof(int))) {
                        PRINTM(INFO, "Copy from user failed\n");
                        LEAVE();
                        return -EFAULT;
                    }

                    Adapter->ScanProbes = data;
                } else {
                    data = Adapter->ScanProbes;
                    if (copy_to_user(wrq->u.data.pointer, &data, sizeof(int))) {
                        PRINTM(INFO, "Copy to user failed\n");
                        LEAVE();
                        return -EFAULT;
                    }
                }
                wrq->u.data.length = 1;
            }
            break;
        case WLAN_LED_GPIO_CTRL:
            {
                int i;
                int data[MAX_LEDS * 2];
                HostCmd_DS_802_11_LED_CTRL ctrl;
                MrvlIEtypes_LedGpio_t *gpio;

                gpio = (MrvlIEtypes_LedGpio_t *) & ctrl.LedGpio;

                if ((wrq->u.data.length > MAX_LEDS * 2) ||
                    (wrq->u.data.length % 2) != 0) {
                    PRINTM(MSG, "invalid ledgpio parameters\n");
                    LEAVE();
                    return -EINVAL;
                }

                memset(&ctrl, 0, sizeof(ctrl));
                if (wrq->u.data.length == 0)
                    ctrl.Action = HostCmd_ACT_GEN_GET;
                else {
                    if (copy_from_user(data, wrq->u.data.pointer,
                                       sizeof(int) * wrq->u.data.length)) {
                        PRINTM(INFO, "Copy from user failed\n");
                        LEAVE();
                        return -EFAULT;
                    }

                    ctrl.Action = HostCmd_ACT_GEN_SET;
                    ctrl.LedNums = 0;
                    gpio->Header.Type = TLV_TYPE_LED_GPIO;
                    gpio->Header.Len = wrq->u.data.length;
                    for (i = 0; i < wrq->u.data.length; i += 2) {
                        gpio->LedGpio[i / 2].LedNum = data[i];
                        gpio->LedGpio[i / 2].GpioNum = data[i + 1];
                    }
                }
                ret = wlan_prepare_cmd(priv,
                                       HostCmd_CMD_802_11_LED_CONTROL, 0,
                                       HostCmd_OPTION_WAITFORRSP,
                                       0, (void *) &ctrl);

                for (i = 0; i < gpio->Header.Len; i += 2) {
                    data[i] = gpio->LedGpio[i / 2].LedNum;
                    data[i + 1] = gpio->LedGpio[i / 2].GpioNum;
                }
                if (copy_to_user(wrq->u.data.pointer, data,
                                 sizeof(int) * gpio->Header.Len)) {
                    PRINTM(INFO, "Copy to user failed\n");
                    LEAVE();
                    return -EFAULT;
                }

                wrq->u.data.length = gpio->Header.Len;
            }
            break;
        case WLAN_SLEEP_PERIOD:
            ret = wlan_sleep_period(priv, wrq);
            break;
        case WLAN_ADAPT_RATESET:
            ret = wlan_adapt_rateset(priv, wrq);
            break;
        case WLANSNR:
            ret = wlan_get_snr(priv, wrq);
            break;
        case WLAN_GET_RATE:
            ret = wlan_getrate_ioctl(priv, wrq);
            break;
        case WLAN_GET_RXINFO:
            ret = wlan_get_rxinfo(priv, wrq);
            break;
        case WLAN_SET_ATIM_WINDOW:
            ret = wlan_atim_window(priv, wrq);
            break;
        case WLAN_BEACON_INTERVAL:
            ret = wlan_beacon_interval(priv, wrq);
            break;
        case WLAN_SDIO_PULL_CTRL:
            ret = wlan_sdio_pull_ctrl(priv, wrq);
            break;
        case WLAN_SCAN_TIME:
            ret = wlan_scan_time(priv, wrq);
            break;
        case WLAN_ECL_SYS_CLOCK:
            ret = wlan_ecl_sys_clock(priv, wrq);
            break;
        case WLAN_TXCONTROL:
            ret = wlan_txcontrol(priv, wrq);
            break;
        case WLANHSCFG:
            ret = wlan_hscfg_ioctl(priv, wrq);
            break;
        case WLANHSSETPARA:
            ret = wlan_hssetpara_ioctl(priv, wrq);
            break;
        case WLAN_INACTIVITY_TIMEOUT_EXT:
            ret = wlan_inactivity_timeout_ext(priv, wrq);
            break;
        case WLANDBGSCFG:
            ret = wlan_dbgs_cfg(priv, wrq);
            break;
#ifdef DEBUG_LEVEL1
        case WLAN_DRV_DBG:
            ret = wlan_drv_dbg(priv, wrq);
            break;
#endif
        case WLAN_DRV_DELAY_MAX:
            ret = wlan_wmm_drv_delay_max_ioctl(priv, wrq);
            break;
        case WLAN_FUNC_IF_CTRL:
            ret = wlan_func_intf_ctrl(priv, wrq);
            break;
        case WLAN_11H_SET_QUIET_IE:
            ret = wlan_11h_ioctl_set_quiet_ie(priv, wrq);
            break;
        }
        break;

    case WLAN_SET_GET_2K:
        switch ((int) wrq->u.data.flags) {
        case WLAN_SET_USER_SCAN:
            ret = wlan_set_user_scan_ioctl(priv, wrq);
            break;
        case WLAN_GET_SCAN_TABLE:
            ret = wlan_get_scan_table_ioctl(priv, wrq);
            break;
        case WLAN_SET_MRVL_TLV:
            ret = wlan_set_mrvl_tlv_ioctl(priv, wrq);
            break;
        case WLAN_GET_ASSOC_RSP:
            ret = wlan_get_assoc_rsp_ioctl(priv, wrq);
            break;
        case WLAN_ADDTS_REQ:
            ret = wlan_wmm_addts_req_ioctl(priv, wrq);
            break;
        case WLAN_DELTS_REQ:
            ret = wlan_wmm_delts_req_ioctl(priv, wrq);
            break;
        case WLAN_QUEUE_CONFIG:
            ret = wlan_wmm_queue_config_ioctl(priv, wrq);
            break;
        case WLAN_QUEUE_STATS:
            ret = wlan_wmm_queue_stats_ioctl(priv, wrq);
            break;
        case WLAN_TX_PKT_STATS:
            ret = wlan_tx_pkt_stats_ioctl(priv, wrq);
            break;
        case WLAN_GET_CFP_TABLE:
            ret = wlan_get_cfp_table_ioctl(priv, wrq);
            break;
        case WLAN_MEF_CFG:
            ret = wlan_set_mef_cfg(priv, wrq);
            break;
        case WLAN_GET_MEM:
            ret = wlan_get_firmware_mem(priv, wrq);
            break;
        default:
            ret = -EOPNOTSUPP;
        }
        break;

    default:
        ret = -EINVAL;
        break;
    }
    LEAVE();
    return ret;
}

/** 
 *  @brief Get version 
 *   
 *  @param adapter              A pointer to wlan_adapter structure
 *  @param version		A pointer to version buffer
 *  @param maxlen		max length of version buffer
 *  @return 	   		NA
 */
void
get_version(wlan_adapter * adapter, char *version, int maxlen)
{
    union
    {
        u32 l;
        u8 c[4];
    } ver;
    char fwver[32];

    ENTER();

    ver.l = adapter->FWReleaseNumber;
    sprintf(fwver, "%u.%u.%u.p%u", ver.c[2], ver.c[1], ver.c[0], ver.c[3]);

    snprintf(version, maxlen, driver_version, fwver);

    LEAVE();
}
