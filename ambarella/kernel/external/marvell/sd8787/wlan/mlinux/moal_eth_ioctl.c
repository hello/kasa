/** @file  moal_eth_ioctl.c
  *
  * @brief This file contains private ioctl functions
  *
  * Copyright (C) 2012, Marvell International Ltd.
  *
  * This software file (the "File") is distributed by Marvell International
  * Ltd. under the terms of the GNU General Public License Version 2, June 1991
  * (the "License").  You may use, redistribute and/or modify this File in
  * accordance with the terms and conditions of the License, a copy of which
  * is available by writing to the Free Software Foundation, Inc.,
  * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA or on the
  * worldwide web at http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt.
  *
  * THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE
  * ARE EXPRESSLY DISCLAIMED.  The License provides additional details about
  * this warranty disclaimer.
  *
  */

/************************************************************************
Change log:
    01/05/2012: initial version
************************************************************************/

#include    "moal_main.h"
#include    "moal_eth_ioctl.h"
#include    "mlan_ioctl.h"
#if defined(STA_WEXT) || defined(UAP_WEXT)
#include    "moal_priv.h"
#endif

#if defined(STA_CFG80211) || defined(UAP_CFG80211)
#include    "moal_cfg80211.h"
#endif
#ifdef UAP_SUPPORT
#include    "moal_uap.h"
#endif
/********************************************************
                Local Variables
********************************************************/

/** Bands supported in Infra mode */
static t_u8 SupportedInfraBand[] = {
    BAND_B,
    BAND_B | BAND_G, BAND_G,
    BAND_GN, BAND_B | BAND_G | BAND_GN, BAND_G | BAND_GN,
    BAND_A, BAND_B | BAND_A, BAND_B | BAND_G | BAND_A, BAND_G | BAND_A,
    BAND_A | BAND_B | BAND_G | BAND_AN | BAND_GN,
        BAND_A | BAND_G | BAND_AN | BAND_GN, BAND_A | BAND_AN,
};

/** Bands supported in Ad-Hoc mode */
static t_u8 SupportedAdhocBand[] = {
    BAND_B, BAND_B | BAND_G, BAND_G,
    BAND_GN, BAND_B | BAND_G | BAND_GN, BAND_G | BAND_GN,
    BAND_A,
    BAND_AN, BAND_A | BAND_AN,
};

/********************************************************
        Global Variables
********************************************************/
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,29)
#ifdef UAP_SUPPORT
/** Network device handlers for uAP */
extern const struct net_device_ops woal_uap_netdev_ops;
#endif
#ifdef STA_SUPPORT
/** Network device handlers for STA */
extern const struct net_device_ops woal_netdev_ops;
#endif
#endif
extern int cfg80211_wext;

/********************************************************
        Local Functions
********************************************************/
/**
 * @brief Parse a string to extract numerical arguments
 *
 * @param pos           Pointer to the arguments string
 * @param data          Pointer to the arguments buffer
 * @param datalen       Length of the arguments buffer
 * @param user_data_len Pointer to the number of arguments extracted
 *
 * @return              MLAN_STATUS_SUCCESS
 */
mlan_status
parse_arguments(t_u8 * pos, int *data, int datalen, int *user_data_len)
{
    unsigned int i, j, k;
    char cdata[10];
    int is_hex = 0;

    memset(cdata, 0, sizeof(cdata));
    for (i = 0, j = 0, k = 0; i <= strlen(pos); i++) {
        if ((k == 0) && (i <= (strlen(pos) - 2))) {
            if ((pos[i] == '0') && (pos[i + 1] == 'x')) {
                is_hex = 1;
                i = i + 2;
            }
        }
        if (pos[i] == '\0' || pos[i] == ' ') {
            if (j >= datalen) {
                j++;
                break;
            }
            if (is_hex) {
                data[j] = woal_atox(cdata);
                is_hex = 0;
            } else {
                woal_atoi(&data[j], cdata);
            }
            j++;
            k = 0;
            memset(cdata, 0, sizeof(cdata));
            if (pos[i] == '\0')
                break;
        } else {
            cdata[k] = pos[i];
            k++;
        }
    }

    *user_data_len = j;
    return MLAN_STATUS_SUCCESS;
}

#if defined(STA_CFG80211) && defined(UAP_CFG80211)
/**
 *  @brief Set wps & p2p ie in AP mode
 *
 *  @param priv         Pointer to priv stucture
 *  @param ie           Pointer to ies data
 *  @param len          Length of data
 *
 *  @return             MLAN_STATUS_SUCCESS -- success, otherwise fail
 */
mlan_status
woal_set_ap_wps_p2p_ie(moal_private * priv, t_u8 * ie, size_t len)
{
    mlan_status ret = MLAN_STATUS_SUCCESS;
    t_u8 *pos = ie;
    t_u32 ie_len;

    ENTER();

    ie_len = len - 2;
    if (ie_len <= 0 || ie_len > MAX_IE_SIZE) {
        PRINTM(MERROR, "IE len error: %d\n", ie_len);
        ret = -EFAULT;
        goto done;
    }

    /* Android cmd format: "SET_AP_WPS_P2P_IE 1" -- beacon IE
       "SET_AP_WPS_P2P_IE 2" -- proberesp IE "SET_AP_WPS_P2P_IE 4" -- assocresp 
       IE */
    if (*pos == '1') {
        PRINTM(MIOCTL, "Ignore set beacon ie\n");
        goto done;
    } else if (*pos == '2') {
        /* set the probe resp ies */
        pos += 2;
        if (MLAN_STATUS_SUCCESS != woal_cfg80211_mgmt_frame_ie(priv, NULL,
                                                               0, pos, ie_len,
                                                               NULL, 0, NULL, 0,
                                                               MGMT_MASK_PROBE_RESP))
        {
            PRINTM(MERROR, "Failed to set probe resp ie\n");
            ret = -EFAULT;
            goto done;
        }
    } else if (*pos == '4') {
        /* set the assoc resp ies */
        pos += 2;
        if (MLAN_STATUS_SUCCESS != woal_cfg80211_mgmt_frame_ie(priv, NULL,
                                                               0, NULL, 0, pos,
                                                               ie_len, NULL, 0,
                                                               MGMT_MASK_ASSOC_RESP))
        {
            PRINTM(MERROR, "Failed to set assoc resp ie\n");
            ret = -EFAULT;
            goto done;
        }
    }

  done:
    LEAVE();
    return ret;
}
#endif

/**
 *  @brief Get Driver Version
 *
 *  @param priv         A pointer to moal_private structure
 *  @param respbuf      A pointer to response buffer
 *  @param respbuflen   Available length of response buffer
 *
 *  @return             Number of bytes written, negative for failure.
 */
int
woal_get_priv_driver_version(moal_private * priv, t_u8 * respbuf,
                             t_u32 respbuflen)
{
    int len = 0, ret = -1;
    char buf[MLAN_MAX_VER_STR_LEN];

    ENTER();

    if (!respbuf) {
        LEAVE();
        return 0;
    }

    memset(buf, 0, sizeof(buf));

    /* Get version string to local buffer */
    woal_get_version(priv->phandle, buf, sizeof(buf) - 1);
    len = strlen(buf);

    if (len) {
        /* Copy back the retrieved version string */
        PRINTM(MINFO, "MOAL VERSION: %s\n", buf);
        ret = MIN(len, (respbuflen - 1));
        memcpy(respbuf, buf, ret);
    } else {
        ret = -1;
        PRINTM(MERROR, "Get version failed!\n");
    }

    LEAVE();
    return ret;
}

/**
 *  @brief Hostcmd interface from application
 *
 *  @param priv         A pointer to moal_private structure
 *  @param respbuf      A pointer to response buffer
 *  @param respbuflen   Available length of response buffer
 *
 *  @return             Number of bytes written, negative for failure.
 */
int
woal_priv_hostcmd(moal_private * priv, t_u8 * respbuf, t_u32 respbuflen)
{
    int ret = 0;
    t_u8 *data_ptr;
    t_u32 buf_len = 0;
    HostCmd_Header cmd_header;
    mlan_ioctl_req *req = NULL;
    mlan_ds_misc_cfg *misc_cfg = NULL;

    ENTER();

    data_ptr = respbuf + (strlen(CMD_MARVELL) + strlen(PRIV_CMD_HOSTCMD));
    buf_len = *((t_u32 *) data_ptr);
    memcpy(&cmd_header, data_ptr + sizeof(buf_len), sizeof(HostCmd_Header));

    PRINTM(MINFO, "Host command len = %d\n", woal_le16_to_cpu(cmd_header.size));
    if (woal_le16_to_cpu(cmd_header.size) > MLAN_SIZE_OF_CMD_BUFFER) {
        LEAVE();
        return -EINVAL;
    }

    req = woal_alloc_mlan_ioctl_req(sizeof(mlan_ds_misc_cfg));
    if (req == NULL) {
        ret = -ENOMEM;
        goto error;
    }
    misc_cfg = (mlan_ds_misc_cfg *) req->pbuf;
    misc_cfg->sub_command = MLAN_OID_MISC_HOST_CMD;
    req->req_id = MLAN_IOCTL_MISC_CFG;
    req->action = MLAN_ACT_SET;
    misc_cfg->param.hostcmd.len = woal_le16_to_cpu(cmd_header.size);
    /* get the whole command */
    memcpy(misc_cfg->param.hostcmd.cmd, data_ptr + sizeof(buf_len),
           misc_cfg->param.hostcmd.len);

    if (MLAN_STATUS_SUCCESS != woal_request_ioctl(priv, req, MOAL_IOCTL_WAIT)) {
        ret = -EFAULT;
        goto error;
    }
    memcpy(data_ptr + sizeof(buf_len), misc_cfg->param.hostcmd.cmd,
           misc_cfg->param.hostcmd.len);
    ret =
        misc_cfg->param.hostcmd.len + sizeof(buf_len) + strlen(CMD_MARVELL) +
        strlen(PRIV_CMD_HOSTCMD);
    memcpy(data_ptr, (t_u8 *) & ret, sizeof(t_u32));

  error:
    if (req)
        kfree(req);

    LEAVE();
    return ret;
}

/**
 *  @brief Custom IE setting
 *
 *  @param priv         A pointer to moal_private structure
 *  @param respbuf      A pointer to response buffer
 *  @param respbuflen   Available length of response buffer
 *
 *  @return             Number of bytes written, negative for failure.
 */
int
woal_priv_customie(moal_private * priv, t_u8 * respbuf, t_u32 respbuflen)
{
    int ret = 0;
    t_u8 *data_ptr;
    mlan_ioctl_req *ioctl_req = NULL;
    mlan_ds_misc_cfg *misc = NULL;
    mlan_ds_misc_custom_ie *custom_ie = NULL;

    ENTER();
    data_ptr = respbuf + (strlen(CMD_MARVELL) + strlen(PRIV_CMD_CUSTOMIE));

    custom_ie = (mlan_ds_misc_custom_ie *) data_ptr;
    ioctl_req = woal_alloc_mlan_ioctl_req(sizeof(mlan_ds_misc_cfg));
    if (ioctl_req == NULL) {
        ret = -ENOMEM;
        goto done;
    }

    misc = (mlan_ds_misc_cfg *) ioctl_req->pbuf;
    misc->sub_command = MLAN_OID_MISC_CUSTOM_IE;
    ioctl_req->req_id = MLAN_IOCTL_MISC_CFG;
    if ((custom_ie->len == 0) ||
        (custom_ie->len == sizeof(custom_ie->ie_data_list[0].ie_index)))
        ioctl_req->action = MLAN_ACT_GET;
    else
        ioctl_req->action = MLAN_ACT_SET;

    memcpy(&misc->param.cust_ie, custom_ie, sizeof(mlan_ds_misc_custom_ie));

    if (MLAN_STATUS_SUCCESS !=
        woal_request_ioctl(priv, ioctl_req, MOAL_IOCTL_WAIT)) {
        ret = -EFAULT;
        goto done;
    }
    custom_ie = (mlan_ds_misc_custom_ie *) data_ptr;
    memcpy(custom_ie, &misc->param.cust_ie, sizeof(mlan_ds_misc_custom_ie));
    ret = sizeof(mlan_ds_misc_custom_ie);
    if (ioctl_req->status_code == MLAN_ERROR_IOCTL_FAIL) {
        /* send a separate error code to indicate error from driver */
        ret = EFAULT;
    }
  done:
    if (ioctl_req) {
        kfree(ioctl_req);
    }
    LEAVE();
    return ret;
}

/**
 *  @brief Set/Get Band and Adhoc-band setting
 *
 *  @param priv         A pointer to moal_private structure
 *  @param respbuf      A pointer to response buffer
 *  @param respbuflen   Available length of response buffer
 *
 *  @return             Number of bytes written, negative for failure.
 */
int
woal_setget_priv_bandcfg(moal_private * priv, t_u8 * respbuf, t_u32 respbuflen)
{
    int ret = 0;
    unsigned int i;
    int data[4];
    int user_data_len = 0;
    t_u32 infra_band = 0;
    t_u32 adhoc_band = 0;
    t_u32 adhoc_channel = 0;
    t_u32 adhoc_chan_bandwidth = 0;
    mlan_ioctl_req *req = NULL;
    mlan_ds_radio_cfg *radio_cfg = NULL;
    mlan_ds_band_cfg *band_cfg = NULL;

    ENTER();

    if (strlen(respbuf) == (strlen(CMD_MARVELL) + strlen(PRIV_CMD_BANDCFG))) {
        /* GET operation */
        user_data_len = 0;
    } else {
        /* SET operation */
        memset((char *) data, 0, sizeof(data));
        parse_arguments(respbuf + strlen(CMD_MARVELL) +
                        strlen(PRIV_CMD_BANDCFG), data,
                        sizeof(data) / sizeof(int), &user_data_len);
    }

    if (sizeof(int) * user_data_len > sizeof(data)) {
        PRINTM(MERROR, "Too many arguments\n");
        LEAVE();
        return -EINVAL;
    }

    if (user_data_len > 0) {
        if (priv->media_connected == MTRUE) {
            LEAVE();
            return -EOPNOTSUPP;
        }
    }

    req = woal_alloc_mlan_ioctl_req(sizeof(mlan_ds_radio_cfg));
    if (req == NULL) {
        ret = -ENOMEM;
        goto error;
    }
    radio_cfg = (mlan_ds_radio_cfg *) req->pbuf;
    radio_cfg->sub_command = MLAN_OID_BAND_CFG;
    req->req_id = MLAN_IOCTL_RADIO_CFG;

    if (user_data_len == 0) {
        /* Get config_bands, adhoc_start_band and adhoc_channel values from
           MLAN */
        req->action = MLAN_ACT_GET;
    } else {
        /* To support only <b/bg/bgn/n/aac/gac> */
        infra_band = data[0];
        for (i = 0; i < sizeof(SupportedInfraBand); i++)
            if (infra_band == SupportedInfraBand[i])
                break;
        if (i == sizeof(SupportedInfraBand)) {
            ret = -EINVAL;
            goto error;
        }

        /* Set Adhoc band */
        if (user_data_len >= 2) {
            adhoc_band = data[1];
            for (i = 0; i < sizeof(SupportedAdhocBand); i++)
                if (adhoc_band == SupportedAdhocBand[i])
                    break;
            if (i == sizeof(SupportedAdhocBand)) {
                ret = -EINVAL;
                goto error;
            }
        }

        /* Set Adhoc channel */
        if (user_data_len >= 3) {
            adhoc_channel = data[2];
            if (adhoc_channel == 0) {
                /* Check if specified adhoc channel is non-zero */
                ret = -EINVAL;
                goto error;
            }
        }
        if (user_data_len == 4) {
            if (!(adhoc_band & (BAND_GN | BAND_AN))) {
                PRINTM(MERROR,
                       "11n is not enabled for adhoc, can not set HT/VHT channel bandwidth\n");
                ret = -EINVAL;
                goto error;
            }
            adhoc_chan_bandwidth = data[3];
            /* sanity test */
            if ((adhoc_chan_bandwidth != CHANNEL_BW_20MHZ) &&
                (adhoc_chan_bandwidth != CHANNEL_BW_40MHZ_ABOVE) &&
                (adhoc_chan_bandwidth != CHANNEL_BW_40MHZ_BELOW)
                ) {
                PRINTM(MERROR,
                       "Invalid secondary channel bandwidth, only allowed 0, 1, 3 or 4\n");
                ret = -EINVAL;
                goto error;
            }

        }
        /* Set config_bands and adhoc_start_band values to MLAN */
        req->action = MLAN_ACT_SET;
        radio_cfg->param.band_cfg.config_bands = infra_band;
        radio_cfg->param.band_cfg.adhoc_start_band = adhoc_band;
        radio_cfg->param.band_cfg.adhoc_channel = adhoc_channel;
        radio_cfg->param.band_cfg.sec_chan_offset = adhoc_chan_bandwidth;
    }

    if (MLAN_STATUS_SUCCESS != woal_request_ioctl(priv, req, MOAL_IOCTL_WAIT)) {
        ret = -EFAULT;
        goto error;
    }

    band_cfg = (mlan_ds_band_cfg *) respbuf;

    memcpy(band_cfg, &radio_cfg->param.band_cfg, sizeof(mlan_ds_band_cfg));

    ret = sizeof(mlan_ds_band_cfg);

  error:
    if (req)
        kfree(req);

    LEAVE();
    return ret;
}

/**
 *  @brief Set/Get 11n configurations
 *
 *  @param priv         A pointer to moal_private structure
 *  @param respbuf      A pointer to response buffer
 *  @param respbuflen   Available length of response buffer
 *
 *  @return             Number of bytes written, negative for failure.
 */
int
woal_setget_priv_httxcfg(moal_private * priv, t_u8 * respbuf, t_u32 respbuflen)
{
    t_u32 data[2];
    mlan_ioctl_req *req = NULL;
    mlan_ds_11n_cfg *cfg_11n = NULL;
    int ret = 0;
    int user_data_len = 0;

    ENTER();

    if (strlen(respbuf) == (strlen(CMD_MARVELL) + strlen(PRIV_CMD_HTTXCFG))) {
        /* GET operation */
        user_data_len = 0;
    } else {
        /* SET operation */
        memset((char *) data, 0, sizeof(data));
        parse_arguments(respbuf + strlen(CMD_MARVELL) +
                        strlen(PRIV_CMD_HTTXCFG), data,
                        sizeof(data) / sizeof(int), &user_data_len);
    }

    if (user_data_len > 2) {
        PRINTM(MERROR, "Too many arguments\n");
        ret = -EINVAL;
        goto done;
    }
    req = woal_alloc_mlan_ioctl_req(sizeof(mlan_ds_11n_cfg));
    if (req == NULL) {
        ret = -ENOMEM;
        goto done;
    }

    cfg_11n = (mlan_ds_11n_cfg *) req->pbuf;
    cfg_11n->sub_command = MLAN_OID_11N_CFG_TX;
    req->req_id = MLAN_IOCTL_11N_CFG;

    if (user_data_len == 0) {
        /* Get 11n tx parameters from MLAN */
        req->action = MLAN_ACT_GET;
        cfg_11n->param.tx_cfg.misc_cfg = BAND_SELECT_BG;
    } else {
        cfg_11n->param.tx_cfg.httxcap = data[0];
        PRINTM(MINFO, "SET: httxcap:0x%x\n", data[0]);
        cfg_11n->param.tx_cfg.misc_cfg = BAND_SELECT_BOTH;
        if (user_data_len == 2) {
            if (data[1] != BAND_SELECT_BG &&
                data[1] != BAND_SELECT_A && data[1] != BAND_SELECT_BOTH) {
                PRINTM(MERROR, "Invalid band selection\n");
                ret = -EINVAL;
                goto done;
            }
            cfg_11n->param.tx_cfg.misc_cfg = data[1];
            PRINTM(MINFO, "SET: httxcap band:0x%x\n", data[1]);
        }
        /* Update 11n tx parameters in MLAN */
        req->action = MLAN_ACT_SET;
    }
    if (MLAN_STATUS_SUCCESS != woal_request_ioctl(priv, req, MOAL_IOCTL_WAIT)) {
        ret = -EFAULT;
        goto done;
    }
    data[0] = cfg_11n->param.tx_cfg.httxcap;
    PRINTM(MINFO, "GET: httxcap:0x%x\n", data[0]);

    if (req->action == MLAN_ACT_GET) {
        cfg_11n->param.tx_cfg.httxcap = 0;
        cfg_11n->param.tx_cfg.misc_cfg = BAND_SELECT_A;
        if (MLAN_STATUS_SUCCESS !=
            woal_request_ioctl(priv, req, MOAL_IOCTL_WAIT)) {
            ret = -EFAULT;
            goto done;
        }
        data[1] = cfg_11n->param.tx_cfg.httxcap;
        PRINTM(MINFO, "GET: httxcap for 5GHz:0x%x\n", data[1]);
    }

    memcpy(respbuf, data, sizeof(data));
    ret = sizeof(data);

  done:
    if (req)
        kfree(req);
    LEAVE();
    return ret;
}

/**
 *  @brief Set/Get 11n capability information
 *
 *  @param priv         A pointer to moal_private structure
 *  @param respbuf      A pointer to response buffer
 *  @param respbuflen   Available length of response buffer
 *
 *  @return             Number of bytes written, negative for failure.
 */
int
woal_setget_priv_htcapinfo(moal_private * priv, t_u8 * respbuf,
                           t_u32 respbuflen)
{
    int data[2];
    mlan_ioctl_req *req = NULL;
    mlan_ds_11n_cfg *cfg_11n = NULL;
    woal_ht_cap_info *ht_cap = NULL;
    int ret = 0;
    int user_data_len = 0;

    ENTER();

    if (strlen(respbuf) == (strlen(CMD_MARVELL) + strlen(PRIV_CMD_HTCAPINFO))) {
        /* GET operation */
        user_data_len = 0;
    } else {
        /* SET operation */
        memset((char *) data, 0, sizeof(data));
        parse_arguments(respbuf + strlen(CMD_MARVELL) +
                        strlen(PRIV_CMD_HTCAPINFO), data,
                        sizeof(data) / sizeof(int), &user_data_len);
    }

    if (user_data_len > 2) {
        PRINTM(MERROR, "Too many arguments\n");
        ret = -EINVAL;
        goto done;
    }
    req = woal_alloc_mlan_ioctl_req(sizeof(mlan_ds_11n_cfg));
    if (req == NULL) {
        ret = -ENOMEM;
        goto done;
    }

    cfg_11n = (mlan_ds_11n_cfg *) req->pbuf;
    cfg_11n->sub_command = MLAN_OID_11N_HTCAP_CFG;
    req->req_id = MLAN_IOCTL_11N_CFG;

    if (user_data_len == 0) {
        /* Get 11n tx parameters from MLAN */
        req->action = MLAN_ACT_GET;
        cfg_11n->param.htcap_cfg.misc_cfg = BAND_SELECT_BG;
    } else {
        cfg_11n->param.htcap_cfg.htcap = data[0];
        PRINTM(MINFO, "SET: htcapinfo:0x%x\n", data[0]);
        cfg_11n->param.htcap_cfg.misc_cfg = BAND_SELECT_BOTH;
        if (user_data_len == 2) {
            if (data[1] != BAND_SELECT_BG &&
                data[1] != BAND_SELECT_A && data[1] != BAND_SELECT_BOTH) {
                PRINTM(MERROR, "Invalid band selection\n");
                ret = -EINVAL;
                goto done;
            }
            cfg_11n->param.htcap_cfg.misc_cfg = data[1];
            PRINTM(MINFO, "SET: htcapinfo band:0x%x\n", data[1]);
        }
        /* Update 11n tx parameters in MLAN */
        req->action = MLAN_ACT_SET;
    }
    if (MLAN_STATUS_SUCCESS != woal_request_ioctl(priv, req, MOAL_IOCTL_WAIT)) {
        ret = -EFAULT;
        goto done;
    }
    data[0] = cfg_11n->param.htcap_cfg.htcap;
    PRINTM(MINFO, "GET: htcapinfo for 2.4GHz:0x%x\n", data[0]);

    if (req->action == MLAN_ACT_GET) {
        cfg_11n->param.htcap_cfg.htcap = 0;
        cfg_11n->param.htcap_cfg.misc_cfg = BAND_SELECT_A;
        if (MLAN_STATUS_SUCCESS !=
            woal_request_ioctl(priv, req, MOAL_IOCTL_WAIT)) {
            ret = -EFAULT;
            goto done;
        }
        data[1] = cfg_11n->param.htcap_cfg.htcap;
        PRINTM(MINFO, "GET: htcapinfo for 5GHz:0x%x\n", data[1]);
    }

    ht_cap = (woal_ht_cap_info *) respbuf;
    ht_cap->ht_cap_info_bg = data[0];
    ht_cap->ht_cap_info_a = data[1];
    ret = sizeof(woal_ht_cap_info);

  done:
    if (req)
        kfree(req);
    LEAVE();
    return ret;
}

/**
 *  @brief Set/Get add BA parameters
 *
 *  @param priv         A pointer to moal_private structure
 *  @param respbuf      A pointer to response buffer
 *  @param respbuflen   Available length of response buffer
 *
 *  @return             Number of bytes written, negative for failure.
 */
int
woal_setget_priv_addbapara(moal_private * priv, t_u8 * respbuf,
                           t_u32 respbuflen)
{
    int data[5];
    mlan_ioctl_req *req = NULL;
    mlan_ds_11n_cfg *cfg_11n = NULL;
    woal_addba *addba = NULL;
    int ret = 0;
    int user_data_len = 0;

    ENTER();

    if (strlen(respbuf) == (strlen(CMD_MARVELL) + strlen(PRIV_CMD_ADDBAPARA))) {
        /* GET operation */
        user_data_len = 0;
    } else {
        /* SET operation */
        memset((char *) data, 0, sizeof(data));
        parse_arguments(respbuf + strlen(CMD_MARVELL) +
                        strlen(PRIV_CMD_ADDBAPARA), data,
                        sizeof(data) / sizeof(int), &user_data_len);

        if (user_data_len != (sizeof(data) / sizeof(int))) {
            PRINTM(MERROR, "Invalid number of arguments\n");
            ret = -EINVAL;
            goto done;
        }
        if (data[0] < 0 || data[0] > MLAN_DEFAULT_BLOCK_ACK_TIMEOUT) {
            PRINTM(MERROR, "Incorrect addba timeout value.\n");
            ret = -EFAULT;
            goto done;
        }
        if (data[1] <= 0 || data[1] > MLAN_AMPDU_MAX_TXWINSIZE || data[2] <= 0
            || data[2] > MLAN_AMPDU_MAX_RXWINSIZE) {
            PRINTM(MERROR, "Incorrect Tx/Rx window size.\n");
            ret = -EFAULT;
            goto done;
        }
        if (data[3] < 0 || data[3] > 1 || data[4] < 0 || data[4] > 1) {
            PRINTM(MERROR, "Incorrect Tx/Rx amsdu.\n");
            ret = -EFAULT;
            goto done;
        }
    }
    req = woal_alloc_mlan_ioctl_req(sizeof(mlan_ds_11n_cfg));
    if (req == NULL) {
        ret = -ENOMEM;
        goto done;
    }

    cfg_11n = (mlan_ds_11n_cfg *) req->pbuf;
    cfg_11n->sub_command = MLAN_OID_11N_CFG_ADDBA_PARAM;
    req->req_id = MLAN_IOCTL_11N_CFG;

    if (user_data_len == 0) {
        /* Get add BA parameters from MLAN */
        req->action = MLAN_ACT_GET;
    } else {
        cfg_11n->param.addba_param.timeout = data[0];
        cfg_11n->param.addba_param.txwinsize = data[1];
        cfg_11n->param.addba_param.rxwinsize = data[2];
        cfg_11n->param.addba_param.txamsdu = data[3];
        cfg_11n->param.addba_param.rxamsdu = data[4];
        PRINTM(MINFO,
               "SET: timeout:%d txwinsize:%d rxwinsize:%d txamsdu=%d rxamsdu=%d\n",
               data[0], data[1], data[2], data[3], data[4]);
        /* Update add BA parameters in MLAN */
        req->action = MLAN_ACT_SET;
    }
    if (MLAN_STATUS_SUCCESS != woal_request_ioctl(priv, req, MOAL_IOCTL_WAIT)) {
        ret = -EFAULT;
        goto done;
    }

    addba = (woal_addba *) respbuf;

    addba->time_out = cfg_11n->param.addba_param.timeout;
    addba->tx_win_size = cfg_11n->param.addba_param.txwinsize;
    addba->rx_win_size = cfg_11n->param.addba_param.rxwinsize;
    addba->tx_amsdu = cfg_11n->param.addba_param.txamsdu;
    addba->rx_amsdu = cfg_11n->param.addba_param.rxamsdu;
    PRINTM(MINFO,
           "GET: timeout:%d txwinsize:%d rxwinsize:%d txamsdu=%d, rxamsdu=%d\n",
           addba->time_out, addba->tx_win_size, addba->rx_win_size,
           addba->tx_amsdu, addba->rx_amsdu);

    ret = sizeof(woal_addba);

  done:
    if (req)
        kfree(req);
    LEAVE();
    return ret;

}

/**
 *  @brief Delete selective BA based on parameters
 *
 *  @param priv         A pointer to moal_private structure
 *  @param respbuf      A pointer to response buffer
 *  @param respbuflen   Available length of response buffer
 *
 *  @return             Number of bytes written, negative for failure.
 */
int
woal_priv_delba(moal_private * priv, t_u8 * respbuf, t_u32 respbuflen)
{
    int data[2];
    mlan_ioctl_req *req = NULL;
    mlan_ds_11n_cfg *cfg_11n = NULL;
    mlan_ds_11n_delba *del_ba = NULL;
    int ret = 0;
    int user_data_len = 0;
    int header_len = 0;
    t_u8 *mac_pos = NULL;
    t_u8 peer_mac[ETH_ALEN];

    ENTER();

    header_len = strlen(CMD_MARVELL) + strlen(PRIV_CMD_DELBA);
    mac_pos = strstr(respbuf + header_len, " ");
    if (mac_pos)
        mac_pos = strstr(mac_pos + 1, " ");
    if (mac_pos) {
        woal_mac2u8(peer_mac, mac_pos + 1);
        *mac_pos = '\0';
    }

    parse_arguments(respbuf + header_len, data, sizeof(data) / sizeof(int),
                    &user_data_len);

    if (mac_pos)
        user_data_len++;

    if (user_data_len < 1 || user_data_len > 3) {
        /* Incorrect number of arguments */
        PRINTM(MERROR, "Invalid arguments\n");
        ret = -EINVAL;
        goto done;
    }

    req = woal_alloc_mlan_ioctl_req(sizeof(mlan_ds_11n_cfg));
    if (req == NULL) {
        ret = -ENOMEM;
        goto done;
    }
    cfg_11n = (mlan_ds_11n_cfg *) req->pbuf;
    req->req_id = MLAN_IOCTL_11N_CFG;
    cfg_11n->sub_command = MLAN_OID_11N_CFG_DELBA;

    del_ba = &cfg_11n->param.del_ba;
    memset(del_ba, 0, sizeof(mlan_ds_11n_delba));
    del_ba->direction = (t_u8) data[0];
    del_ba->tid = DELBA_ALL_TIDS;
    if (user_data_len > 1)
        del_ba->tid = (t_u8) data[1];
    if (user_data_len > 2)
        memcpy(del_ba->peer_mac_addr, peer_mac, ETH_ALEN);

    if (MLAN_STATUS_SUCCESS != woal_request_ioctl(priv, req, MOAL_NO_WAIT)) {
        ret = -EFAULT;
        goto done;
    }
    /* The DelBA ioctl is special, need to call main process to execute the
       commands */
    queue_work(priv->phandle->workqueue, &priv->phandle->main_work);

    ret = sprintf(respbuf, "OK. BA deleted successfully.\n") + 1;

  done:
    if (req)
        kfree(req);
    LEAVE();
    return ret;
}

/**
 *  @brief Set/Get aggregation priority table configurations
 *
 *  @param priv         A pointer to moal_private structure
 *  @param respbuf      A pointer to response buffer
 *  @param respbuflen   Available length of response buffer
 *
 *  @return             Number of bytes written, negative for failure.
 */
int
woal_setget_priv_aggrpriotbl(moal_private * priv, t_u8 * respbuf,
                             t_u32 respbuflen)
{
    int data[MAX_NUM_TID * 2], i, j;
    mlan_ioctl_req *req = NULL;
    mlan_ds_11n_cfg *cfg_11n = NULL;
    int ret = 0;
    int user_data_len = 0;

    ENTER();

    if (strlen(respbuf) == (strlen(CMD_MARVELL) + strlen(PRIV_CMD_AGGRPRIOTBL))) {
        /* GET operation */
        user_data_len = 0;
    } else {
        /* SET operation */
        memset((char *) data, 0, sizeof(data));
        parse_arguments(respbuf + strlen(CMD_MARVELL) +
                        strlen(PRIV_CMD_AGGRPRIOTBL), data,
                        sizeof(data) / sizeof(int), &user_data_len);

        if (user_data_len != (sizeof(data) / sizeof(int))) {
            PRINTM(MERROR, "Invalid number of arguments\n");
            ret = -EINVAL;
            goto done;
        }
        for (i = 0, j = 0; i < user_data_len; i = i + 2, ++j) {
            if ((data[i] > 7 && data[i] != 0xff) ||
                (data[i + 1] > 7 && data[i + 1] != 0xff)) {
                PRINTM(MERROR, "Invalid priority, valid value 0-7 or 0xff.\n");
                ret = -EFAULT;
                goto done;
            }
        }
    }
    req = woal_alloc_mlan_ioctl_req(sizeof(mlan_ds_11n_cfg));
    if (req == NULL) {
        ret = -ENOMEM;
        goto done;
    }

    cfg_11n = (mlan_ds_11n_cfg *) req->pbuf;
    cfg_11n->sub_command = MLAN_OID_11N_CFG_AGGR_PRIO_TBL;
    req->req_id = MLAN_IOCTL_11N_CFG;

    if (user_data_len == 0) {
        /* Get aggr priority table from MLAN */
        req->action = MLAN_ACT_GET;
    } else {
        for (i = 0, j = 0; i < user_data_len; i = i + 2, ++j) {
            cfg_11n->param.aggr_prio_tbl.ampdu[j] = data[i];
            cfg_11n->param.aggr_prio_tbl.amsdu[j] = data[i + 1];
        }
        /* Update aggr priority table in MLAN */
        req->action = MLAN_ACT_SET;
    }
    if (MLAN_STATUS_SUCCESS != woal_request_ioctl(priv, req, MOAL_IOCTL_WAIT)) {
        ret = -EFAULT;
        goto done;
    }

    for (i = 0, j = 0; i < (MAX_NUM_TID * 2); i = i + 2, ++j) {
        respbuf[i] = cfg_11n->param.aggr_prio_tbl.ampdu[j];
        respbuf[i + 1] = cfg_11n->param.aggr_prio_tbl.amsdu[j];
    }

    ret = sizeof(data);

  done:
    if (req)
        kfree(req);
    LEAVE();
    return ret;

}

/**
 *  @brief Set/Get Add BA reject configurations
 *
 *  @param priv         A pointer to moal_private structure
 *  @param respbuf      A pointer to response buffer
 *  @param respbuflen   Available length of response buffer
 *
 *  @return             Number of bytes written, negative for failure.
 */
int
woal_setget_priv_addbareject(moal_private * priv, t_u8 * respbuf,
                             t_u32 respbuflen)
{
    int data[MAX_NUM_TID], i;
    mlan_ioctl_req *req = NULL;
    mlan_ds_11n_cfg *cfg_11n = NULL;
    int ret = 0;
    int user_data_len = 0;

    ENTER();

    if (strlen(respbuf) == (strlen(CMD_MARVELL) + strlen(PRIV_CMD_ADDBAREJECT))) {
        /* GET operation */
        user_data_len = 0;
    } else {
        /* SET operation */
        memset((char *) data, 0, sizeof(data));
        parse_arguments(respbuf + strlen(CMD_MARVELL) +
                        strlen(PRIV_CMD_ADDBAREJECT), data,
                        sizeof(data) / sizeof(int), &user_data_len);

        if (user_data_len != (sizeof(data) / sizeof(int))) {
            PRINTM(MERROR, "Invalid number of arguments\n");
            ret = -EINVAL;
            goto done;
        }
        for (i = 0; i < user_data_len; i++) {
            if (data[i] != 0 && data[i] != 1) {
                PRINTM(MERROR, "addba reject only takes argument as 0 or 1\n");
                ret = -EFAULT;
                goto done;
            }
        }
    }
    req = woal_alloc_mlan_ioctl_req(sizeof(mlan_ds_11n_cfg));
    if (req == NULL) {
        ret = -ENOMEM;
        goto done;
    }

    cfg_11n = (mlan_ds_11n_cfg *) req->pbuf;
    cfg_11n->sub_command = MLAN_OID_11N_CFG_ADDBA_REJECT;
    req->req_id = MLAN_IOCTL_11N_CFG;

    if (user_data_len == 0) {
        /* Get add BA reject configuration from MLAN */
        req->action = MLAN_ACT_GET;
    } else {
        for (i = 0; i < user_data_len; i++) {
            cfg_11n->param.addba_reject[i] = data[i];
        }
        /* Update add BA reject configuration in MLAN */
        req->action = MLAN_ACT_SET;
    }
    if (MLAN_STATUS_SUCCESS != woal_request_ioctl(priv, req, MOAL_IOCTL_WAIT)) {
        ret = -EFAULT;
        goto done;
    }

    for (i = 0; i < MAX_NUM_TID; i++) {
        respbuf[i] = cfg_11n->param.addba_reject[i];
    }

    ret = sizeof(data);

  done:
    if (req)
        kfree(req);
    LEAVE();
    return ret;

}

/**
 *  @brief Set/Get 11AC configurations
 *
 *  @param priv         A pointer to moal_private structure
 *  @param respbuf      A pointer to response buffer
 *  @param respbuflen   Available length of response buffer
 *
 *  @return             Number of bytes written, negative for failure.
 */
int
woal_get_priv_datarate(moal_private * priv, t_u8 * respbuf, t_u32 respbuflen)
{
    mlan_ioctl_req *req = NULL;
    mlan_ds_rate *rate = NULL;
    mlan_data_rate *data_rate = NULL;
    int ret = 0;

    ENTER();

    req = woal_alloc_mlan_ioctl_req(sizeof(mlan_ds_rate));
    if (req == NULL) {
        ret = -ENOMEM;
        goto done;
    }

    rate = (mlan_ds_rate *) req->pbuf;
    rate->sub_command = MLAN_OID_GET_DATA_RATE;
    req->req_id = MLAN_IOCTL_RATE;
    req->action = MLAN_ACT_GET;

    if (MLAN_STATUS_SUCCESS != woal_request_ioctl(priv, req, MOAL_IOCTL_WAIT)) {
        ret = -EFAULT;
        goto done;
    }

    data_rate = (mlan_data_rate *) respbuf;

    memcpy(data_rate, &rate->param.data_rate, sizeof(mlan_data_rate));

    ret = sizeof(mlan_data_rate);

  done:
    if (req)
        kfree(req);
    LEAVE();
    return ret;

}

/**
 *  @brief Set/Get tx rate configurations
 *
 *  @param priv         A pointer to moal_private structure
 *  @param respbuf      A pointer to response buffer
 *  @param respbuflen   Available length of response buffer
 *
 *  @return             Number of bytes written, negative for failure.
 */
int
woal_setget_priv_txratecfg(moal_private * priv, t_u8 * respbuf,
                           t_u32 respbuflen)
{
    t_u32 data[3];
    mlan_ioctl_req *req = NULL;
    mlan_ds_rate *rate = NULL;
    woal_tx_rate_cfg *ratecfg = NULL;
    int ret = 0;
    int user_data_len = 0;

    ENTER();

    if (strlen(respbuf) == (strlen(CMD_MARVELL) + strlen(PRIV_CMD_TXRATECFG))) {
        /* GET operation */
        user_data_len = 0;
    } else {
        /* SET operation */
        memset((char *) data, 0, sizeof(data));
        parse_arguments(respbuf + strlen(CMD_MARVELL) +
                        strlen(PRIV_CMD_TXRATECFG), data,
                        sizeof(data) / sizeof(int), &user_data_len);
    }

    if (user_data_len >= 4) {
        PRINTM(MERROR, "Too many arguments\n");
        ret = -EINVAL;
        goto done;
    }

    req = woal_alloc_mlan_ioctl_req(sizeof(mlan_ds_rate));
    if (req == NULL) {
        ret = -ENOMEM;
        goto done;
    }

    req->req_id = MLAN_IOCTL_RATE;
    rate = (mlan_ds_rate *) req->pbuf;
    rate->sub_command = MLAN_OID_RATE_CFG;
    rate->param.rate_cfg.rate_type = MLAN_RATE_INDEX;

    if (user_data_len == 0) {
        /* Get operation */
        req->action = MLAN_ACT_GET;
    } else {
        /* Set operation */
        req->action = MLAN_ACT_SET;
        /* format */
        if ((data[0] != AUTO_RATE) && (data[0] >= 3)) {
            PRINTM(MERROR, "Invalid format selection\n");
            ret = -EINVAL;
            goto done;
        }
        if (data[0] == AUTO_RATE) {
            /* auto */
            rate->param.rate_cfg.is_rate_auto = 1;
        } else {
            /* fixed rate */
            PRINTM(MINFO, "SET: txratefg format: 0x%x\n", data[0]);
            if ((data[0] != AUTO_RATE) && (data[0] > MLAN_RATE_FORMAT_HT)
                ) {
                PRINTM(MERROR, "Invalid format selection\n");
                ret = -EINVAL;
                goto done;
            }
        }

        if ((user_data_len >= 2) && (data[0] != AUTO_RATE)) {
            PRINTM(MINFO, "SET: txratefg index: 0x%x\n", data[1]);
            /* sanity check */
            if (((data[0] == MLAN_RATE_FORMAT_LG) &&
                 (data[1] > MLAN_RATE_INDEX_OFDM7))
                || ((data[0] == MLAN_RATE_FORMAT_HT) && (data[1] != 32) &&
                    (data[1] > 7))
                ) {
                PRINTM(MERROR, "Invalid index selection\n");
                ret = -EINVAL;
                goto done;
            }

            PRINTM(MINFO, "SET: txratefg index: 0x%x\n", data[1]);
            rate->param.rate_cfg.rate = data[1];

            if (data[0] == MLAN_RATE_FORMAT_HT) {
                rate->param.rate_cfg.rate = data[1] + MLAN_RATE_INDEX_MCS0;
            }
        }

    }

    if (MLAN_STATUS_SUCCESS != woal_request_ioctl(priv, req, MOAL_IOCTL_WAIT)) {
        ret = -EFAULT;
        goto done;
    }

    ratecfg = (woal_tx_rate_cfg *) respbuf;
    if (rate->param.rate_cfg.is_rate_auto == MTRUE) {
        ratecfg->rate_format = 0xFF;
    } else {
        /* fixed rate */
        if (rate->param.rate_cfg.rate < MLAN_RATE_INDEX_MCS0) {
            ratecfg->rate_format = MLAN_RATE_FORMAT_LG;
            ratecfg->rate_index = rate->param.rate_cfg.rate;
        } else {
            ratecfg->rate_format = MLAN_RATE_FORMAT_HT;
            ratecfg->rate_index =
                rate->param.rate_cfg.rate - MLAN_RATE_INDEX_MCS0;
        }
    }

    ret = sizeof(woal_tx_rate_cfg);

  done:
    if (req)
        kfree(req);
    LEAVE();
    return ret;

}

/**
 *  @brief Set/Get esupplicant mode configurations
 *
 *  @param priv         A pointer to moal_private structure
 *  @param respbuf      A pointer to response buffer
 *  @param respbuflen   Available length of response buffer
 *
 *  @return             Number of bytes written, negative for failure.
 */
int
woal_setget_priv_esuppmode(moal_private * priv, t_u8 * respbuf,
                           t_u32 respbuflen)
{
    t_u32 data[3];
    mlan_ioctl_req *req = NULL;
    mlan_ds_sec_cfg *sec = NULL;
    woal_esuppmode_cfg *esupp_mode = NULL;
    int ret = 0;
    int user_data_len = 0;

    ENTER();

    if (strlen(respbuf) == (strlen(CMD_MARVELL) + strlen(PRIV_CMD_ESUPPMODE))) {
        /* GET operation */
        user_data_len = 0;
    } else {
        /* SET operation */
        memset((char *) data, 0, sizeof(data));
        parse_arguments(respbuf + strlen(CMD_MARVELL) +
                        strlen(PRIV_CMD_ESUPPMODE), data,
                        sizeof(data) / sizeof(int), &user_data_len);
    }

    if (user_data_len >= 4 || user_data_len == 1 || user_data_len == 2) {
        PRINTM(MERROR, "Invalid number of arguments\n");
        ret = -EINVAL;
        goto done;
    }

    req = woal_alloc_mlan_ioctl_req(sizeof(mlan_ds_sec_cfg));
    if (req == NULL) {
        ret = -ENOMEM;
        goto done;
    }

    req->req_id = MLAN_IOCTL_SEC_CFG;
    sec = (mlan_ds_sec_cfg *) req->pbuf;
    sec->sub_command = MLAN_OID_SEC_CFG_ESUPP_MODE;

    if (user_data_len == 0) {
        /* Get operation */
        req->action = MLAN_ACT_GET;
    } else {
        /* Set operation */
        req->action = MLAN_ACT_SET;
        /* RSN mode */
        sec->param.esupp_mode.rsn_mode = data[0];
        /* Pairwise cipher */
        sec->param.esupp_mode.act_paircipher = (data[1] & 0xFF);
        /* Group cipher */
        sec->param.esupp_mode.act_groupcipher = (data[2] & 0xFF);
    }

    if (MLAN_STATUS_SUCCESS != woal_request_ioctl(priv, req, MOAL_IOCTL_WAIT)) {
        ret = -EFAULT;
        goto done;
    }

    esupp_mode = (woal_esuppmode_cfg *) respbuf;
    esupp_mode->rsn_mode = (t_u16) ((sec->param.esupp_mode.rsn_mode) & 0xFFFF);
    esupp_mode->pairwise_cipher =
        (t_u8) ((sec->param.esupp_mode.act_paircipher) & 0xFF);
    esupp_mode->group_cipher =
        (t_u8) ((sec->param.esupp_mode.act_groupcipher) & 0xFF);

    ret = sizeof(woal_esuppmode_cfg);
  done:
    if (req)
        kfree(req);
    LEAVE();
    return ret;

}

/**
 *  @brief Set/Get esupplicant passphrase configurations
 *
 *  @param priv         A pointer to moal_private structure
 *  @param respbuf      A pointer to response buffer
 *  @param respbuflen   Available length of response buffer
 *
 *  @return             Number of bytes written, negative for failure.
 */
int
woal_setget_priv_passphrase(moal_private * priv, t_u8 * respbuf,
                            t_u32 respbuflen)
{
    mlan_ioctl_req *req = NULL;
    mlan_ds_sec_cfg *sec = NULL;
    int ret = 0, action = -1, i = 0;
    char *begin, *end, *opt;
    t_u16 len = 0;
    t_u8 zero_mac[] = { 0, 0, 0, 0, 0, 0 };
    t_u8 *mac = NULL;

    ENTER();

    if (strlen(respbuf) == (strlen(CMD_MARVELL) + strlen(PRIV_CMD_PASSPHRASE))) {
        PRINTM(MERROR, "No arguments provided\n");
        ret = -EINVAL;
        goto done;
    }

    /* Parse the buf to get the cmd_action */
    begin = respbuf + strlen(CMD_MARVELL) + strlen(PRIV_CMD_PASSPHRASE);
    end = woal_strsep(&begin, ';', '/');
    if (end)
        action = woal_atox(end);
    if (action < 0 || action > 2 || end[1] != '\0') {
        PRINTM(MERROR, "Invalid action argument %s\n", end);
        ret = -EINVAL;
        goto done;
    }

    req = woal_alloc_mlan_ioctl_req(sizeof(mlan_ds_sec_cfg));
    if (req == NULL) {
        ret = -ENOMEM;
        goto done;
    }

    req->req_id = MLAN_IOCTL_SEC_CFG;
    sec = (mlan_ds_sec_cfg *) req->pbuf;
    sec->sub_command = MLAN_OID_SEC_CFG_PASSPHRASE;
    if (action == 0)
        req->action = MLAN_ACT_GET;
    else
        req->action = MLAN_ACT_SET;

    while (begin) {
        end = woal_strsep(&begin, ';', '/');
        opt = woal_strsep(&end, '=', '/');
        if (!opt || !end || !end[0]) {
            PRINTM(MERROR, "Invalid option\n");
            ret = -EINVAL;
            break;
        } else if (!strnicmp(opt, "ssid", strlen(opt))) {
            if (strlen(end) > MLAN_MAX_SSID_LENGTH) {
                PRINTM(MERROR, "SSID length exceeds max length\n");
                ret = -EFAULT;
                break;
            }
            sec->param.passphrase.ssid.ssid_len = strlen(end);
            strncpy((char *) sec->param.passphrase.ssid.ssid, end, strlen(end));
            PRINTM(MINFO, "ssid=%s, len=%d\n", sec->param.passphrase.ssid.ssid,
                   (int) sec->param.passphrase.ssid.ssid_len);
        } else if (!strnicmp(opt, "bssid", strlen(opt))) {
            woal_mac2u8((t_u8 *) & sec->param.passphrase.bssid, end);
        } else if (!strnicmp(opt, "psk", strlen(opt)) &&
                   req->action == MLAN_ACT_SET) {
            if (strlen(end) != MLAN_PMK_HEXSTR_LENGTH) {
                PRINTM(MERROR, "Invalid PMK length\n");
                ret = -EINVAL;
                break;
            }
            woal_ascii2hex((t_u8 *) (sec->param.passphrase.psk.pmk.pmk), end,
                           MLAN_PMK_HEXSTR_LENGTH / 2);
            sec->param.passphrase.psk_type = MLAN_PSK_PMK;
        } else if (!strnicmp(opt, "passphrase", strlen(opt)) &&
                   req->action == MLAN_ACT_SET) {
            if (strlen(end) < MLAN_MIN_PASSPHRASE_LENGTH ||
                strlen(end) > MLAN_MAX_PASSPHRASE_LENGTH) {
                PRINTM(MERROR, "Invalid length for passphrase\n");
                ret = -EINVAL;
                break;
            }
            sec->param.passphrase.psk_type = MLAN_PSK_PASSPHRASE;
            strncpy(sec->param.passphrase.psk.passphrase.passphrase, end,
                    sizeof(sec->param.passphrase.psk.passphrase.passphrase));
            sec->param.passphrase.psk.passphrase.passphrase_len = strlen(end);
            PRINTM(MINFO, "passphrase=%s, len=%d\n",
                   sec->param.passphrase.psk.passphrase.passphrase,
                   (int) sec->param.passphrase.psk.passphrase.passphrase_len);
        } else {
            PRINTM(MERROR, "Invalid option %s\n", opt);
            ret = -EINVAL;
            break;
        }
    }
    if (ret)
        goto done;

    if (action == 2)
        sec->param.passphrase.psk_type = MLAN_PSK_CLEAR;

    if (MLAN_STATUS_SUCCESS != woal_request_ioctl(priv, req, MOAL_IOCTL_WAIT)) {
        ret = -EFAULT;
        goto done;
    }

    memset(respbuf, 0, respbuflen);
    if (sec->param.passphrase.ssid.ssid_len) {
        len += sprintf(respbuf + len, "ssid:");
        memcpy(respbuf + len, sec->param.passphrase.ssid.ssid,
               sec->param.passphrase.ssid.ssid_len);
        len += sec->param.passphrase.ssid.ssid_len;
        len += sprintf(respbuf + len, " ");
    }
    if (memcmp(&sec->param.passphrase.bssid, zero_mac, sizeof(zero_mac))) {
        mac = (t_u8 *) & sec->param.passphrase.bssid;
        len += sprintf(respbuf + len, "bssid:");
        for (i = 0; i < ETH_ALEN - 1; ++i)
            len += sprintf(respbuf + len, "%02x:", mac[i]);
        len += sprintf(respbuf + len, "%02x ", mac[i]);
    }
    if (sec->param.passphrase.psk_type == MLAN_PSK_PMK) {
        len += sprintf(respbuf + len, "psk:");
        for (i = 0; i < MLAN_MAX_KEY_LENGTH; ++i)
            len +=
                sprintf(respbuf + len, "%02x",
                        sec->param.passphrase.psk.pmk.pmk[i]);
        len += sprintf(respbuf + len, "\n");
    }
    if (sec->param.passphrase.psk_type == MLAN_PSK_PASSPHRASE) {
        len +=
            sprintf(respbuf + len, "passphrase:%s \n",
                    sec->param.passphrase.psk.passphrase.passphrase);
    }

    ret = len;
  done:
    if (req)
        kfree(req);
    LEAVE();
    return ret;

}

/**
 *  @brief Deauthenticate
 *
 *  @param priv         A pointer to moal_private structure
 *  @param respbuf      A pointer to response buffer
 *  @param respbuflen   Available length of response buffer
 *
 *  @return             Number of bytes written, negative for failure.
 */
int
woal_priv_deauth(moal_private * priv, t_u8 * respbuf, t_u32 respbuflen)
{
    mlan_ioctl_req *req = NULL;
    int ret = 0;
    t_u8 mac[ETH_ALEN];

    ENTER();

    if (strlen(respbuf) > (strlen(CMD_MARVELL) + strlen(PRIV_CMD_DEAUTH))) {
        /* Deauth mentioned BSSID */
        woal_mac2u8(mac,
                    respbuf + strlen(CMD_MARVELL) + strlen(PRIV_CMD_DEAUTH));
        if (MLAN_STATUS_SUCCESS != woal_disconnect(priv, MOAL_IOCTL_WAIT, mac)) {
            ret = -EFAULT;
            goto done;
        }
    } else {
        if (MLAN_STATUS_SUCCESS != woal_disconnect(priv, MOAL_IOCTL_WAIT, NULL))
            ret = -EFAULT;
    }

  done:
    if (req)
        kfree(req);
    LEAVE();
    return ret;
}

#ifdef UAP_SUPPORT
/**     
 *  @brief uap station deauth ioctl handler
 *
 *  @param priv         A pointer to moal_private structure
 *  @param respbuf      A pointer to response buffer
 *  @param respbuflen   Available length of response buffer
 *
 *  @return             Number of bytes written, negative for failure.
 */
static int
woal_priv_ap_deauth(moal_private * priv, t_u8 * respbuf, t_u32 respbuflen)
{
    t_u8 *data_ptr;
    mlan_ioctl_req *ioctl_req = NULL;
    mlan_ds_bss *bss = NULL;
    mlan_deauth_param deauth_param;
    int ret = 0;

    ENTER();

    data_ptr = respbuf + (strlen(CMD_MARVELL) + strlen(PRIV_CMD_AP_DEAUTH));
    memset(&deauth_param, 0, sizeof(mlan_deauth_param));
    memcpy(&deauth_param, data_ptr, sizeof(mlan_deauth_param));

    PRINTM(MIOCTL,
           "ioctl deauth station: %02x:%02x:%02x:%02x:%02x:%02x, reason=%d\n",
           deauth_param.mac_addr[0], deauth_param.mac_addr[1],
           deauth_param.mac_addr[2], deauth_param.mac_addr[3],
           deauth_param.mac_addr[4], deauth_param.mac_addr[5],
           deauth_param.reason_code);

    ioctl_req = woal_alloc_mlan_ioctl_req(sizeof(mlan_ds_bss));
    if (ioctl_req == NULL) {
        ret = -ENOMEM;
        goto done;
    }
    bss = (mlan_ds_bss *) ioctl_req->pbuf;

    bss->sub_command = MLAN_OID_UAP_DEAUTH_STA;
    ioctl_req->req_id = MLAN_IOCTL_BSS;
    ioctl_req->action = MLAN_ACT_SET;

    memcpy(&bss->param.deauth_param, &deauth_param, sizeof(mlan_deauth_param));
    if (MLAN_STATUS_SUCCESS !=
        woal_request_ioctl(priv, ioctl_req, MOAL_IOCTL_WAIT)) {
        ret = -EFAULT;
        goto done;
    }

    memcpy(data_ptr, &ioctl_req->status_code, sizeof(t_u32));
    ret = sizeof(t_u32);
  done:
    if (ioctl_req)
        kfree(ioctl_req);
    LEAVE();
    return ret;
}

/** 
 *  @brief uap get station list handler
 *   
 *  @param dev      A pointer to net_device structure
 *  @param req      A pointer to ifreq structure
 *  @return         0 --success, otherwise fail
 */
static int
woal_priv_get_sta_list(moal_private * priv, t_u8 * respbuf, t_u32 respbuflen)
{
    int ret = 0;
    mlan_ds_get_info *info = NULL;
    mlan_ds_sta_list *sta_list = NULL;
    mlan_ioctl_req *ioctl_req = NULL;

    ENTER();

    /* Allocate an IOCTL request buffer */
    ioctl_req =
        (mlan_ioctl_req *) woal_alloc_mlan_ioctl_req(sizeof(mlan_ds_get_info));
    if (ioctl_req == NULL) {
        ret = -ENOMEM;
        goto done;
    }

    info = (mlan_ds_get_info *) ioctl_req->pbuf;
    info->sub_command = MLAN_OID_UAP_STA_LIST;
    ioctl_req->req_id = MLAN_IOCTL_GET_INFO;
    ioctl_req->action = MLAN_ACT_GET;

    if (MLAN_STATUS_SUCCESS !=
        woal_request_ioctl(priv, ioctl_req, MOAL_IOCTL_WAIT)) {
        ret = -EFAULT;
        goto done;
    }

    sta_list =
        (mlan_ds_sta_list *) (respbuf + strlen(CMD_MARVELL) +
                              strlen(PRIV_CMD_GET_STA_LIST));
    memcpy(sta_list, &info->param.sta_list, sizeof(mlan_ds_sta_list));
    ret = sizeof(mlan_ds_sta_list);
  done:
    if (ioctl_req)
        kfree(ioctl_req);
    LEAVE();
    return ret;
}

/**         
 *  @brief uap bss_config handler
 *   
 *  @param dev      A pointer to net_device structure
 *  @param req      A pointer to ifreq structure
 *  @return         0 --success, otherwise fail 
 */
static int
woal_priv_bss_config(moal_private * priv, t_u8 * respbuf, t_u32 respbuflen)
{
    int ret = 0;
    mlan_ds_bss *bss = NULL;
    mlan_ioctl_req *ioctl_req = NULL;
    t_u32 action = 0;
    int offset = 0;

    ENTER();

    offset = strlen(CMD_MARVELL) + strlen(PRIV_CMD_BSS_CONFIG);
    memcpy((u8 *) & action, respbuf + offset, sizeof(action));
    offset += sizeof(action);

    /* Allocate an IOCTL request buffer */
    ioctl_req =
        (mlan_ioctl_req *) woal_alloc_mlan_ioctl_req(sizeof(mlan_ds_bss));
    if (ioctl_req == NULL) {
        ret = -ENOMEM;
        goto done;
    }

    bss = (mlan_ds_bss *) ioctl_req->pbuf;
    bss->sub_command = MLAN_OID_UAP_BSS_CONFIG;
    ioctl_req->req_id = MLAN_IOCTL_BSS;
    if (action == 1) {
        ioctl_req->action = MLAN_ACT_SET;
        /* Get the BSS config from user */
        memcpy(&bss->param.bss_config, respbuf + offset,
               sizeof(mlan_uap_bss_param));
    } else {
        ioctl_req->action = MLAN_ACT_GET;
    }

    if (MLAN_STATUS_SUCCESS !=
        woal_request_ioctl(priv, ioctl_req, MOAL_IOCTL_WAIT)) {
        ret = -EFAULT;
        goto done;
    }

    if (ioctl_req->action == MLAN_ACT_GET) {
        memcpy(respbuf + offset, &bss->param.bss_config,
               sizeof(mlan_uap_bss_param));
    }
    ret = sizeof(mlan_uap_bss_param);
  done:
    if (ioctl_req)
        kfree(ioctl_req);
    LEAVE();
    return ret;
}
#endif

#if defined(WIFI_DIRECT_SUPPORT)
#if defined(STA_SUPPORT) && defined(UAP_SUPPORT)
/**
 *  @brief Set/Get BSS role
 *
 *  @param priv         A pointer to moal_private structure
 *  @param respbuf      A pointer to response buffer
 *  @param respbuflen   Available length of response buffer
 *
 *  @return             Number of bytes written, negative for failure.
 */
int
woal_priv_bssrole(moal_private * priv, t_u8 * respbuf, t_u32 respbuflen)
{
    t_u32 data[1];
    int ret = 0;
    int user_data_len = 0;
    t_u8 action = MLAN_ACT_GET;

    ENTER();

    if (strlen(respbuf) == (strlen(CMD_MARVELL) + strlen(PRIV_CMD_BSSROLE))) {
        /* GET operation */
        user_data_len = 0;
    } else {
        /* SET operation */
        memset((char *) data, 0, sizeof(data));
        parse_arguments(respbuf + strlen(CMD_MARVELL) +
                        strlen(PRIV_CMD_BSSROLE), data,
                        sizeof(data) / sizeof(int), &user_data_len);
    }

    if (user_data_len >= 2) {
        PRINTM(MERROR, "Too many arguments\n");
        ret = -EINVAL;
        goto error;
    }

    if (user_data_len == 0) {
        action = MLAN_ACT_GET;
    } else {
        if ((data[0] != MLAN_BSS_ROLE_STA &&
             data[0] != MLAN_BSS_ROLE_UAP) ||
            priv->bss_type != MLAN_BSS_TYPE_WIFIDIRECT) {
            PRINTM(MWARN, "Invalid BSS role\n");
            ret = -EINVAL;
            goto error;
        }
        if (data[0] == GET_BSS_ROLE(priv)) {
            PRINTM(MWARN, "Already BSS is in desired role\n");
            goto done;
        }
        action = MLAN_ACT_SET;
        /* Reset interface */
        woal_reset_intf(priv, MOAL_IOCTL_WAIT, MFALSE);
    }

    if (MLAN_STATUS_SUCCESS != woal_bss_role_cfg(priv,
                                                 action, MOAL_IOCTL_WAIT,
                                                 (t_u8 *) data)) {
        ret = -EFAULT;
        goto error;
    }

    if (user_data_len) {
        /* Initialize private structures */
        woal_init_priv(priv, MOAL_IOCTL_WAIT);
        /* Enable interfaces */
        netif_device_attach(priv->netdev);
        woal_start_queue(priv->netdev);
    }

  done:
    memset(respbuf, 0, respbuflen);
    respbuf[0] = (t_u8) data[0];
    ret = 1;

  error:
    LEAVE();
    return ret;
}
#endif /* STA_SUPPORT && UAP_SUPPORT */
#endif /* WIFI_DIRECT_SUPPORT && V14_FEATURE */

#ifdef STA_SUPPORT
/**
 *  @brief Set user scan
 *
 *  @param priv         A pointer to moal_private structure
 *  @param respbuf      A pointer to response buffer
 *  @param respbuflen   Available length of response buffer
 *
 *  @return             Number of bytes written, negative for failure.
 */
int
woal_priv_setuserscan(moal_private * priv, t_u8 * respbuf, t_u32 respbuflen)
{
    wlan_user_scan_cfg scan_cfg;
    int ret = 0;

    ENTER();

    /* Create the scan_cfg structure */
    memset(&scan_cfg, 0, sizeof(scan_cfg));

    /* We expect the scan_cfg structure to be passed in respbuf */
    memcpy((char *) &scan_cfg,
           respbuf + strlen(CMD_MARVELL) + strlen(PRIV_CMD_SETUSERSCAN),
           sizeof(wlan_user_scan_cfg));

    /* Call for scan */
    if (MLAN_STATUS_FAILURE == woal_do_scan(priv, &scan_cfg))
        ret = -EFAULT;

    LEAVE();
    return ret;
}

/**
 *  @brief Retrieve the scan response/beacon table
 *
 *  @param respbuf      A pointer to response buffer
 *  @param respbuflen   Available length of response buffer
 *  @param scan_resp    A pointer to mlan_scan_resp structure
 *  @param scan_start   Argument
 *
 *  @return             MLAN_STATUS_SUCCESS --success, otherwise fail
 */
int
moal_ret_get_scan_table_ioctl(t_u8 * respbuf, t_u32 respbuflen,
                              mlan_scan_resp * scan_resp, t_u32 scan_start)
{
    pBSSDescriptor_t pbss_desc, scan_table;
    wlan_ioctl_get_scan_table_info *prsp_info;
    int ret_code;
    int ret_len;
    int space_left;
    t_u8 *pcurrent;
    t_u8 *pbuffer_end;
    t_u32 num_scans_done;

    ENTER();

    num_scans_done = 0;
    ret_code = MLAN_STATUS_SUCCESS;

    prsp_info = (wlan_ioctl_get_scan_table_info *) respbuf;
    pcurrent = (t_u8 *) prsp_info->scan_table_entry_buf;

    pbuffer_end = respbuf + respbuflen - 1;
    space_left = pbuffer_end - pcurrent;
    scan_table = (BSSDescriptor_t *) (scan_resp->pscan_table);

    PRINTM(MINFO, "GetScanTable: scan_start req = %d\n", scan_start);
    PRINTM(MINFO, "GetScanTable: length avail = %d\n", respbuflen);

    if (!scan_start) {
        PRINTM(MINFO, "GetScanTable: get current BSS Descriptor\n");

        /* Use to get current association saved descriptor */
        pbss_desc = scan_table;

        ret_code = wlan_get_scan_table_ret_entry(pbss_desc,
                                                 &pcurrent, &space_left);

        if (ret_code == MLAN_STATUS_SUCCESS) {
            num_scans_done = 1;
        }
    } else {
        scan_start--;

        while (space_left
               && (scan_start + num_scans_done < scan_resp->num_in_scan_table)
               && (ret_code == MLAN_STATUS_SUCCESS)) {

            pbss_desc = (scan_table + (scan_start + num_scans_done));

            PRINTM(MINFO, "GetScanTable: get current BSS Descriptor [%d]\n",
                   scan_start + num_scans_done);

            ret_code = wlan_get_scan_table_ret_entry(pbss_desc,
                                                     &pcurrent, &space_left);

            if (ret_code == MLAN_STATUS_SUCCESS) {
                num_scans_done++;
            }
        }
    }

    prsp_info->scan_number = num_scans_done;
    ret_len = pcurrent - respbuf;

    LEAVE();
    return ret_len;
}

/**
 *  @brief Get scan table
 *
 *  @param priv         A pointer to moal_private structure
 *  @param respbuf      A pointer to response buffer
 *  @param respbuflen   Available length of response buffer
 *
 *  @return             Number of bytes written, negative for failure.
 */
int
woal_priv_getscantable(moal_private * priv, t_u8 * respbuf, t_u32 respbuflen)
{
    int ret = 0;
    mlan_ioctl_req *req = NULL;
    mlan_ds_scan *scan = NULL;
    t_u32 scan_start;
    mlan_status status = MLAN_STATUS_SUCCESS;
    moal_handle *handle = priv->phandle;

    ENTER();

    /* First make sure scanning is not in progress */
    if (handle->scan_pending_on_block == MTRUE) {
        ret = -EAGAIN;
        goto done;
    }

    /* Allocate an IOCTL request buffer */
    req = woal_alloc_mlan_ioctl_req(sizeof(mlan_ds_scan));
    if (req == NULL) {
        ret = -ENOMEM;
        goto done;
    }

    /* Fill request buffer */
    scan = (mlan_ds_scan *) req->pbuf;
    req->req_id = MLAN_IOCTL_SCAN;
    req->action = MLAN_ACT_GET;

    /* Get the whole command from user */
    memcpy(&scan_start,
           respbuf + strlen(CMD_MARVELL) + strlen(PRIV_CMD_GETSCANTABLE),
           sizeof(scan_start));
    if (scan_start) {
        scan->sub_command = MLAN_OID_SCAN_NORMAL;
    } else {
        scan->sub_command = MLAN_OID_SCAN_GET_CURRENT_BSS;
    }
    /* Send IOCTL request to MLAN */
    status = woal_request_ioctl(priv, req, MOAL_IOCTL_WAIT);
    if (status == MLAN_STATUS_SUCCESS) {
        ret = moal_ret_get_scan_table_ioctl(respbuf, respbuflen,
                                            &scan->param.scan_resp, scan_start);
    }
  done:
    if (req && (status != MLAN_STATUS_PENDING))
        kfree(req);
    LEAVE();
    return ret;
}
#endif

/**
 *  @brief Set/Get deep sleep mode configurations
 *
 *  @param priv         A pointer to moal_private structure
 *  @param respbuf      A pointer to response buffer
 *  @param respbuflen   Available length of response buffer
 *
 *  @return             Number of bytes written, negative for failure.
 */
int
woal_priv_setgetdeepsleep(moal_private * priv, t_u8 * respbuf, t_u32 respbuflen)
{
    t_u32 data[2];
    int ret = 0;
    int user_data_len = 0;

    ENTER();

    if (strlen(respbuf) == (strlen(CMD_MARVELL) + strlen(PRIV_CMD_DEEPSLEEP))) {
        /* GET operation */
        user_data_len = 0;
    } else {
        /* SET operation */
        memset((char *) data, 0, sizeof(data));
        parse_arguments(respbuf + strlen(CMD_MARVELL) +
                        strlen(PRIV_CMD_DEEPSLEEP), data,
                        sizeof(data) / sizeof(int), &user_data_len);
    }

    if (user_data_len >= 3) {
        PRINTM(MERROR, "Too many arguments\n");
        ret = -EINVAL;
        goto done;
    }

    if (user_data_len == 0) {
        if (MLAN_STATUS_SUCCESS != woal_get_deep_sleep(priv, data)) {
            ret = -EFAULT;
            goto done;
        }
        sprintf(respbuf, "%d %d", data[0], data[1]);
        ret = strlen(respbuf) + 1;
    } else {
        if (data[0] == DEEP_SLEEP_OFF) {
            PRINTM(MINFO, "Exit Deep Sleep Mode\n");
            ret = woal_set_deep_sleep(priv, MOAL_IOCTL_WAIT, MFALSE, 0);
            if (ret != MLAN_STATUS_SUCCESS) {
                ret = -EINVAL;
                goto done;
            }
        } else if (data[0] == DEEP_SLEEP_ON) {
            PRINTM(MINFO, "Enter Deep Sleep Mode\n");
            if (user_data_len != 2)
                data[1] = 0;
            ret = woal_set_deep_sleep(priv, MOAL_IOCTL_WAIT, MTRUE, data[1]);
            if (ret != MLAN_STATUS_SUCCESS) {
                ret = -EINVAL;
                goto done;
            }
        } else {
            PRINTM(MERROR, "Unknown option = %u\n", data[0]);
            ret = -EINVAL;
            goto done;
        }
        ret = sprintf(respbuf, "OK\n") + 1;
    }

  done:
    LEAVE();
    return ret;

}

/**
 *  @brief Set/Get IP address configurations
 *
 *  @param priv         A pointer to moal_private structure
 *  @param respbuf      A pointer to response buffer
 *  @param respbuflen   Available length of response buffer
 *
 *  @return             Number of bytes written, negative for failure.
 */
int
woal_priv_setgetipaddr(moal_private * priv, t_u8 * respbuf, t_u32 respbuflen)
{
    mlan_ioctl_req *req = NULL;
    mlan_ds_misc_cfg *misc = NULL;
    int ret = 0, op_code = 0, data_length = 0, header = 0;

    ENTER();

    header = strlen(CMD_MARVELL) + strlen(PRIV_CMD_IPADDR);
    data_length = strlen(respbuf) - header;

    req = woal_alloc_mlan_ioctl_req(sizeof(mlan_ds_misc_cfg));
    if (req == NULL) {
        ret = -ENOMEM;
        goto done;
    }
    misc = (mlan_ds_misc_cfg *) req->pbuf;

    if (data_length < 1) {      /* GET */
        req->action = MLAN_ACT_GET;
    } else {
        /* Make sure we have the operation argument */
        if (data_length > 2 && respbuf[header + 1] != ';') {
            PRINTM(MERROR, "No operation argument. Separate with ';'\n");
            ret = -EINVAL;
            goto done;
        } else {
            respbuf[header + 1] = '\0';
        }
        req->action = MLAN_ACT_SET;

        /* Only one IP is supported in current firmware */
        memset(misc->param.ipaddr_cfg.ip_addr[0], 0, IPADDR_LEN);
        in4_pton(&respbuf[header + 2],
                 MIN((IPADDR_MAX_BUF - 3), (data_length - 2)),
                 misc->param.ipaddr_cfg.ip_addr[0], ' ', NULL);
        misc->param.ipaddr_cfg.ip_addr_num = 1;
        misc->param.ipaddr_cfg.ip_addr_type = IPADDR_TYPE_IPV4;

        if (woal_atoi(&op_code, &respbuf[header]) != MLAN_STATUS_SUCCESS) {
            ret = -EINVAL;
            goto done;
        }
        misc->param.ipaddr_cfg.op_code = (t_u32) op_code;
    }

    req->req_id = MLAN_IOCTL_MISC_CFG;
    misc->sub_command = MLAN_OID_MISC_IP_ADDR;

    if (MLAN_STATUS_SUCCESS != woal_request_ioctl(priv, req, MOAL_IOCTL_WAIT)) {
        ret = -EFAULT;
        goto done;
    }

    if (req->action == MLAN_ACT_GET) {
        snprintf(respbuf, IPADDR_MAX_BUF, "%d;%d.%d.%d.%d",
                 misc->param.ipaddr_cfg.op_code,
                 misc->param.ipaddr_cfg.ip_addr[0][0],
                 misc->param.ipaddr_cfg.ip_addr[0][1],
                 misc->param.ipaddr_cfg.ip_addr[0][2],
                 misc->param.ipaddr_cfg.ip_addr[0][3]);
        ret = IPADDR_MAX_BUF + 1;
    } else {
        ret = sprintf(respbuf, "OK\n") + 1;
    }

  done:
    if (req)
        kfree(req);
    LEAVE();
    return ret;
}

/**
 *  @brief Set/Get WPS session configurations
 *
 *  @param priv         A pointer to moal_private structure
 *  @param respbuf      A pointer to response buffer
 *  @param respbuflen   Available length of response buffer
 *
 *  @return             Number of bytes written, negative for failure.
 */
int
woal_priv_setwpssession(moal_private * priv, t_u8 * respbuf, t_u32 respbuflen)
{
    mlan_ioctl_req *req = NULL;
    mlan_ds_wps_cfg *pwps = NULL;
    t_u32 data[1];
    int ret = 0;
    int user_data_len = 0;

    ENTER();

    if (strlen(respbuf) == (strlen(CMD_MARVELL) + strlen(PRIV_CMD_WPSSESSION))) {
        /* GET operation */
        user_data_len = 0;
    } else {
        /* SET operation */
        memset((char *) data, 0, sizeof(data));
        parse_arguments(respbuf + strlen(CMD_MARVELL) +
                        strlen(PRIV_CMD_WPSSESSION), data,
                        sizeof(data) / sizeof(int), &user_data_len);
    }

    if (user_data_len > 1) {
        PRINTM(MERROR, "Too many arguments\n");
        ret = -EINVAL;
        goto done;
    }

    req = woal_alloc_mlan_ioctl_req(sizeof(mlan_ds_wps_cfg));
    if (req == NULL) {
        ret = -ENOMEM;
        goto done;
    }
    pwps = (mlan_ds_wps_cfg *) req->pbuf;

    req->req_id = MLAN_IOCTL_WPS_CFG;
    req->action = MLAN_ACT_SET;
    pwps->sub_command = MLAN_OID_WPS_CFG_SESSION;

    if (data[0] == 1)
        pwps->param.wps_session = MLAN_WPS_CFG_SESSION_START;
    else
        pwps->param.wps_session = MLAN_WPS_CFG_SESSION_END;

    if (MLAN_STATUS_SUCCESS != woal_request_ioctl(priv, req, MOAL_IOCTL_WAIT)) {
        ret = -EFAULT;
        goto done;
    }

    ret = sprintf(respbuf, "OK\n") + 1;
  done:
    if (req)
        kfree(req);
    LEAVE();
    return ret;
}

/**
 *  @brief Get OTP user data
 *
 *  @param priv         A pointer to moal_private structure
 *  @param respbuf      A pointer to response buffer
 *  @param respbuflen   Available length of response buffer
 *
 *  @return             Number of bytes written, negative for failure.
 */
int
woal_priv_otpuserdata(moal_private * priv, t_u8 * respbuf, t_u32 respbuflen)
{
    int data[1];
    int user_data_len = 0;
    mlan_ioctl_req *req = NULL;
    mlan_ds_misc_cfg *misc = NULL;
    mlan_ds_misc_otp_user_data *otp = NULL;
    int ret = 0;

    ENTER();

    memset((char *) data, 0, sizeof(data));
    parse_arguments(respbuf + strlen(CMD_MARVELL) +
                    strlen(PRIV_CMD_OTPUSERDATA), data,
                    sizeof(data) / sizeof(int), &user_data_len);

    if (user_data_len > 1) {
        PRINTM(MERROR, "Too many arguments\n");
        LEAVE();
        return -EINVAL;
    }

    req = woal_alloc_mlan_ioctl_req(sizeof(mlan_ds_misc_cfg));
    if (req == NULL) {
        ret = -ENOMEM;
        goto done;
    }
    req->action = MLAN_ACT_GET;
    req->req_id = MLAN_IOCTL_MISC_CFG;

    misc = (mlan_ds_misc_cfg *) req->pbuf;
    misc->sub_command = MLAN_OID_MISC_OTP_USER_DATA;
    misc->param.otp_user_data.user_data_length = data[0];

    if (MLAN_STATUS_SUCCESS != woal_request_ioctl(priv, req, MOAL_IOCTL_WAIT)) {
        ret = -EFAULT;
        goto done;
    }
    otp = (mlan_ds_misc_otp_user_data *) req->pbuf;

    if (req->action == MLAN_ACT_GET) {
        ret = MIN(otp->user_data_length, data[0]);
        memcpy(respbuf, otp->user_data, ret);
    }

  done:
    if (req)
        kfree(req);
    LEAVE();
    return ret;
}

/**
 *  @brief Set / Get country code
 *
 *  @param priv         A pointer to moal_private structure
 *  @param respbuf      A pointer to response buffer
 *  @param respbuflen   Available length of response buffer
 *
 *  @return             Number of bytes written, negative for failure.
 */
int
woal_priv_set_get_countrycode(moal_private * priv, t_u8 * respbuf,
                              t_u32 respbuflen)
{
    int ret = 0;
    // char data[COUNTRY_CODE_LEN] = {0, 0, 0};
    int header = 0, data_length = 0;    // wrq->u.data.length;
    mlan_ioctl_req *req = NULL;
    mlan_ds_misc_cfg *pcfg_misc = NULL;
    mlan_ds_misc_country_code *country_code = NULL;

    ENTER();

    header = strlen(CMD_MARVELL) + strlen(PRIV_CMD_COUNTRYCODE);
    data_length = strlen(respbuf) - header;

    if (data_length > COUNTRY_CODE_LEN) {
        PRINTM(MERROR, "Invalid argument!\n");
        ret = -EINVAL;
        goto done;
    }

    /* Allocate an IOCTL request buffer */
    req = woal_alloc_mlan_ioctl_req(sizeof(mlan_ds_misc_cfg));
    if (req == NULL) {
        ret = -ENOMEM;
        goto done;
    }

    /* Fill request buffer */
    pcfg_misc = (mlan_ds_misc_cfg *) req->pbuf;
    country_code = &pcfg_misc->param.country_code;
    pcfg_misc->sub_command = MLAN_OID_MISC_COUNTRY_CODE;
    req->req_id = MLAN_IOCTL_MISC_CFG;

    if (data_length <= 1) {
        req->action = MLAN_ACT_GET;
    } else {
        memset(country_code->country_code, 0, COUNTRY_CODE_LEN);
        memcpy(country_code->country_code, respbuf + header, COUNTRY_CODE_LEN);
        req->action = MLAN_ACT_SET;
    }

    /* Send IOCTL request to MLAN */
    if (woal_request_ioctl(priv, req, MOAL_IOCTL_WAIT) != MLAN_STATUS_SUCCESS) {
        ret = -EFAULT;
        goto done;
    }

    if (req->action == MLAN_ACT_GET) {
        ret = data_length = COUNTRY_CODE_LEN;
        memset(respbuf + header, 0, COUNTRY_CODE_LEN);
        memcpy(respbuf, country_code->country_code, COUNTRY_CODE_LEN);
    }

  done:
    if (req)
        kfree(req);

    LEAVE();
    return ret;
}

/**
 *  @brief Get wakeup reason
 *
 *  @param priv         A pointer to moal_private structure
 *  @param respbuf      A pointer to response buffer
 *  @param respbuflen   Available length of response buffer
 *
 *  @return             Number of bytes written, negative for failure.
 */
int
woal_priv_getwakeupreason(moal_private * priv, t_u8 * respbuf, t_u32 respbuflen)
{
    mlan_ioctl_req *req = NULL;
    mlan_ds_pm_cfg *pm_cfg = NULL;
    t_u32 data;
    int ret = 0;

    ENTER();

    if (strlen(respbuf) ==
        (strlen(CMD_MARVELL) + strlen(PRIV_CMD_WAKEUPREASON))) {
        /* GET operation */
        req = woal_alloc_mlan_ioctl_req(sizeof(mlan_ds_misc_cfg));
        if (req == NULL) {
            LEAVE();
            return -ENOMEM;
        }

        pm_cfg = (mlan_ds_pm_cfg *) req->pbuf;
        pm_cfg->sub_command = MLAN_OID_PM_HS_WAKEUP_REASON;
        req->req_id = MLAN_IOCTL_PM_CFG;
        req->action = MLAN_ACT_GET;

        if (MLAN_STATUS_SUCCESS !=
            woal_request_ioctl(priv, req, MOAL_IOCTL_WAIT)) {
            ret = -EFAULT;
            if (req)
                kfree(req);
            goto done;
        } else {
            data = pm_cfg->param.wakeup_reason.hs_wakeup_reason;
            sprintf(respbuf, " %d", data);
            ret = strlen(respbuf) + 1;
            if (req)
                kfree(req);
        }
    } else {
        PRINTM(MERROR, "Not need argument, invalid operation!\n");
        ret = -EINVAL;
        goto done;
    }

  done:
    LEAVE();
    return ret;

}

#ifdef STA_SUPPORT
/**
 *  @brief Set / Get listen interval 
 *
 *  @param priv         A pointer to moal_private structure
 *  @param respbuf      A pointer to response buffer
 *  @param respbuflen   Available length of response buffer
 *
 *  @return             Number of bytes written, negative for failure.
 */
int
woal_priv_set_get_listeninterval(moal_private * priv, t_u8 * respbuf,
                                 t_u32 respbuflen)
{
    int data[1];
    int user_data_len = 0;
    int ret = 0;
    mlan_ioctl_req *req = NULL;
    mlan_ds_bss *pcfg_bss = NULL;

    ENTER();

    if (strlen(respbuf) ==
        (strlen(CMD_MARVELL) + strlen(PRIV_CMD_LISTENINTERVAL))) {
        /* GET operation */
        user_data_len = 0;
    } else {
        /* SET operation */
        memset((char *) data, 0, sizeof(data));
        parse_arguments(respbuf + strlen(CMD_MARVELL) +
                        strlen(PRIV_CMD_LISTENINTERVAL), data,
                        sizeof(data) / sizeof(int), &user_data_len);
    }

    if (user_data_len > 1) {
        PRINTM(MERROR, "Too many arguments\n");
        ret = -EINVAL;
        goto done;
    }

    /* Allocate an IOCTL request buffer */
    req = woal_alloc_mlan_ioctl_req(sizeof(mlan_ds_bss));
    if (req == NULL) {
        ret = -ENOMEM;
        goto done;
    }

    /* Fill request buffer */
    pcfg_bss = (mlan_ds_bss *) req->pbuf;
    pcfg_bss->sub_command = MLAN_OID_BSS_LISTEN_INTERVAL;
    req->req_id = MLAN_IOCTL_BSS;

    if (user_data_len) {
        pcfg_bss->param.listen_interval = (t_u16) data[0];
        req->action = MLAN_ACT_SET;
    } else {
        req->action = MLAN_ACT_GET;
    }

    /* Send IOCTL request to MLAN */
    if (woal_request_ioctl(priv, req, MOAL_IOCTL_WAIT) != MLAN_STATUS_SUCCESS) {
        ret = -EFAULT;
        goto done;
    }
    if (req->action == MLAN_ACT_GET) {
        sprintf(respbuf, "%d", pcfg_bss->param.listen_interval);
        ret = strlen(respbuf) + 1;
    } else {
        ret = sprintf(respbuf, "OK\n") + 1;
    }

  done:
    if (req)
        kfree(req);

    LEAVE();
    return ret;
}
#endif

#ifdef DEBUG_LEVEL1
/**
 *  @brief Set / Get driver debug level
 *
 *  @param priv         A pointer to moal_private structure
 *  @param respbuf      A pointer to response buffer
 *  @param respbuflen   Available length of response buffer
 *
 *  @return             Number of bytes written, negative for failure.
 */
int
woal_priv_set_get_drvdbg(moal_private * priv, t_u8 * respbuf, t_u32 respbuflen)
{
    int data[4];
    int user_data_len = 0;
    int ret = 0;

    ENTER();

    if (strlen(respbuf) == (strlen(CMD_MARVELL) + strlen(PRIV_CMD_DRVDBG))) {
        /* GET operation */
        user_data_len = 0;
    } else {
        /* SET operation */
        memset((char *) data, 0, sizeof(data));
        parse_arguments(respbuf + strlen(CMD_MARVELL) + strlen(PRIV_CMD_DRVDBG),
                        data, sizeof(data) / sizeof(int), &user_data_len);
    }

    if (user_data_len > 1) {
        PRINTM(MERROR, "Too many arguments\n");
        ret = -EINVAL;
        goto done;
    }

    if (user_data_len) {
        /* Get the driver debug bit masks from user */
        drvdbg = data[0];
        /* Set the driver debug bit masks into mlan */
        if (woal_set_drvdbg(priv, drvdbg)) {
            PRINTM(MERROR, "Set drvdbg failed!\n");
            ret = MLAN_STATUS_FAILURE;
            goto done;
        }
    }

    ret = sizeof(drvdbg);

    memcpy(respbuf, &drvdbg, sizeof(drvdbg));

    printk(KERN_ALERT "drvdbg = 0x%08x\n", drvdbg);
#ifdef DEBUG_LEVEL2
    printk(KERN_ALERT "MINFO  (%08x) %s\n", MINFO, (drvdbg & MINFO) ? "X" : "");
    printk(KERN_ALERT "MWARN  (%08x) %s\n", MWARN, (drvdbg & MWARN) ? "X" : "");
    printk(KERN_ALERT "MENTRY (%08x) %s\n", MENTRY,
           (drvdbg & MENTRY) ? "X" : "");
#endif
    printk(KERN_ALERT "MIF_D  (%08x) %s\n", MIF_D, (drvdbg & MIF_D) ? "X" : "");
    printk(KERN_ALERT "MFW_D  (%08x) %s\n", MFW_D, (drvdbg & MFW_D) ? "X" : "");
    printk(KERN_ALERT "MEVT_D (%08x) %s\n", MEVT_D,
           (drvdbg & MEVT_D) ? "X" : "");
    printk(KERN_ALERT "MCMD_D (%08x) %s\n", MCMD_D,
           (drvdbg & MCMD_D) ? "X" : "");
    printk(KERN_ALERT "MDAT_D (%08x) %s\n", MDAT_D,
           (drvdbg & MDAT_D) ? "X" : "");
    printk(KERN_ALERT "MIOCTL (%08x) %s\n", MIOCTL,
           (drvdbg & MIOCTL) ? "X" : "");
    printk(KERN_ALERT "MINTR  (%08x) %s\n", MINTR, (drvdbg & MINTR) ? "X" : "");
    printk(KERN_ALERT "MEVENT (%08x) %s\n", MEVENT,
           (drvdbg & MEVENT) ? "X" : "");
    printk(KERN_ALERT "MCMND  (%08x) %s\n", MCMND, (drvdbg & MCMND) ? "X" : "");
    printk(KERN_ALERT "MDATA  (%08x) %s\n", MDATA, (drvdbg & MDATA) ? "X" : "");
    printk(KERN_ALERT "MERROR (%08x) %s\n", MERROR,
           (drvdbg & MERROR) ? "X" : "");
    printk(KERN_ALERT "MFATAL (%08x) %s\n", MFATAL,
           (drvdbg & MFATAL) ? "X" : "");
    printk(KERN_ALERT "MMSG   (%08x) %s\n", MMSG, (drvdbg & MMSG) ? "X" : "");

  done:
    LEAVE();
    return ret;
}
#endif

/**
 *  @brief Set/Get Host Sleep configuration
 *
 *  @param priv             A pointer to moal_private structure
 *  @param respbuf          A pointer to response buffer
 *  @param respbuflen       Available length of response buffer
 *  @param invoke_hostcmd	MTRUE --invoke HostCmd, otherwise MFALSE
 *
 *  @return             0 --success, otherwise fail
 */
int
woal_priv_hscfg(moal_private * priv, t_u8 * respbuf, t_u32 respbuflen,
                BOOLEAN invoke_hostcmd)
{
    int data[3];
    int user_data_len = 0;
    int ret = 0;
    mlan_ds_hs_cfg hscfg;
    t_u16 action;
    mlan_bss_info bss_info;
    int is_negative = MFALSE;
    t_u8 *arguments = NULL;

    ENTER();

    memset(data, 0, sizeof(data));
    memset(&hscfg, 0, sizeof(mlan_ds_hs_cfg));

    if (strlen(respbuf) == (strlen(CMD_MARVELL) + strlen(PRIV_CMD_HSCFG))) {
        /* GET operation */
        user_data_len = 0;
    } else {
        /* SET operation */
        memset((char *) data, 0, sizeof(data));
        arguments = respbuf + strlen(CMD_MARVELL) + strlen(PRIV_CMD_HSCFG);
        if (*arguments == '-') {
            is_negative = MTRUE;
            arguments += 1;
        }
        parse_arguments(arguments, data, sizeof(data) / sizeof(int),
                        &user_data_len);

        if (is_negative == MTRUE && data[0] == 1)
            data[0] = -1;
    }

    if (sizeof(int) * user_data_len > sizeof(data)) {
        PRINTM(MERROR, "Too many arguments\n");
        LEAVE();
        return -EINVAL;
    }

    if (user_data_len == 0) {
        action = MLAN_ACT_GET;
    } else {
        if (user_data_len >= 1 && user_data_len <= 3) {
            action = MLAN_ACT_SET;
        } else {
            PRINTM(MERROR, "Invalid arguments\n");
            ret = -EINVAL;
            goto done;
        }
    }

    /* HS config is blocked if HS is already activated */
    if (user_data_len &&
        (data[0] != HOST_SLEEP_CFG_CANCEL || invoke_hostcmd == MFALSE)) {
        memset(&bss_info, 0, sizeof(bss_info));
        woal_get_bss_info(priv, MOAL_IOCTL_WAIT, &bss_info);
        if (bss_info.is_hs_configured) {
            PRINTM(MERROR, "HS already configured\n");
            ret = -EFAULT;
            goto done;
        }
    }

    /* Do a GET first if some arguments are not provided */
    if (user_data_len >= 1 && user_data_len < 3) {
        woal_set_get_hs_params(priv, MLAN_ACT_GET, MOAL_IOCTL_WAIT, &hscfg);
    }

    if (user_data_len)
        hscfg.conditions = data[0];
    if (user_data_len >= 2)
        hscfg.gpio = data[1];
    if (user_data_len == 3)
        hscfg.gap = data[2];

    if ((invoke_hostcmd == MTRUE) && (action == MLAN_ACT_SET)) {
        /* Need to issue an extra IOCTL first to set up parameters */
        hscfg.is_invoke_hostcmd = MFALSE;
        if (MLAN_STATUS_SUCCESS !=
            woal_set_get_hs_params(priv, MLAN_ACT_SET, MOAL_IOCTL_WAIT,
                                   &hscfg)) {
            ret = -EFAULT;
            goto done;
        }
    }
    hscfg.is_invoke_hostcmd = invoke_hostcmd;
    if (MLAN_STATUS_SUCCESS !=
        woal_set_get_hs_params(priv, action, MOAL_IOCTL_WAIT, &hscfg)) {
        ret = -EFAULT;
        goto done;
    }

    if (action == MLAN_ACT_GET) {
        /* Return the current driver debug bit masks */
        memcpy(respbuf, &hscfg, sizeof(mlan_ds_hs_cfg));
        ret = sizeof(mlan_ds_hs_cfg);
    }
  done:
    LEAVE();
    return ret;
}

/**
 *  @brief Set Host Sleep parameters
 *
 *  @param priv         A pointer to moal_private structure
 *  @param respbuf      A pointer to response buffer
 *  @param respbuflen   Available length of response buffer
 *
 *  @return             0 --success, otherwise fail
 */
int
woal_priv_hssetpara(moal_private * priv, t_u8 * respbuf, t_u32 respbuflen)
{
    int data[3];
    int user_data_len = 0;
    int ret = 0;

    ENTER();

    if (strlen(respbuf) == (strlen(CMD_MARVELL) + strlen(PRIV_CMD_HSSETPARA))) {
        PRINTM(MERROR, "Invalid arguments\n");
        ret = -EINVAL;
        goto done;
    } else {
        /* SET operation */
        memset((char *) data, 0, sizeof(data));
        parse_arguments(respbuf + strlen(CMD_MARVELL) +
                        strlen(PRIV_CMD_HSSETPARA), data,
                        sizeof(data) / sizeof(int), &user_data_len);
    }

    if (sizeof(int) * user_data_len > sizeof(data)) {
        PRINTM(MERROR, "Too many arguments\n");
        LEAVE();
        return -EINVAL;
    }

    if (user_data_len >= 1 && user_data_len <= 3) {
        sprintf(respbuf, "%s%s%s", CMD_MARVELL, PRIV_CMD_HSCFG,
                respbuf + (strlen(CMD_MARVELL) + strlen(PRIV_CMD_HSSETPARA)));
        respbuflen = strlen(respbuf);
        ret = woal_priv_hscfg(priv, respbuf, respbuflen, MFALSE);
        goto done;
    }
  done:
    LEAVE();
    return ret;
}

/**
 * @brief Set/Get scan configuration parameters
 * 
 * @param priv         A pointer to moal_private structure
 * @param respbuf      A pointer to response buffer
 * @param respbuflen   Available length of response buffer
 * 
 * @return         0 --success, otherwise fail
 */
int
woal_priv_set_get_scancfg(moal_private * priv, t_u8 * respbuf, t_u32 respbuflen)
{
    int ret = 0;
    int user_data_len = 0;
    int arg_len = 7;
    int data[arg_len];
    mlan_ds_scan *scan = NULL;
    mlan_ioctl_req *req = NULL;

    ENTER();

    memset(data, 0, sizeof(data));
    if (strlen(respbuf) == (strlen(CMD_MARVELL) + strlen(PRIV_CMD_SCANCFG))) {
        /* GET operation */
        user_data_len = 0;
    } else {
        /* SET operation */
        memset((char *) data, 0, sizeof(data));
        parse_arguments(respbuf + strlen(CMD_MARVELL) +
                        strlen(PRIV_CMD_SCANCFG), data,
                        sizeof(data) / sizeof(int), &user_data_len);
    }

    if (sizeof(int) * user_data_len > sizeof(data)) {
        PRINTM(MERROR, "Too many arguments\n");
        LEAVE();
        return -EINVAL;
    }

    req = woal_alloc_mlan_ioctl_req(sizeof(mlan_ds_scan));
    if (req == NULL) {
        ret = -ENOMEM;
        goto done;
    }
    scan = (mlan_ds_scan *) req->pbuf;
    scan->sub_command = MLAN_OID_SCAN_CONFIG;
    req->req_id = MLAN_IOCTL_SCAN;

    if (user_data_len) {
        if ((data[0] < 0) || (data[0] > MLAN_SCAN_TYPE_PASSIVE)) {
            PRINTM(MERROR, "Invalid argument for scan type\n");
            ret = -EINVAL;
            goto done;
        }
        if ((data[1] < 0) || (data[1] > MLAN_SCAN_MODE_ANY)) {
            PRINTM(MERROR, "Invalid argument for scan mode\n");
            ret = -EINVAL;
            goto done;
        }
        if ((data[2] < 0) || (data[2] > MAX_PROBES)) {
            PRINTM(MERROR, "Invalid argument for scan probes\n");
            ret = -EINVAL;
            goto done;
        }
        if (((data[3] < 0) || (data[3] > MRVDRV_MAX_ACTIVE_SCAN_CHAN_TIME)) ||
            ((data[4] < 0) || (data[4] > MRVDRV_MAX_ACTIVE_SCAN_CHAN_TIME)) ||
            ((data[5] < 0) || (data[5] > MRVDRV_MAX_PASSIVE_SCAN_CHAN_TIME))) {
            PRINTM(MERROR, "Invalid argument for scan time\n");
            ret = -EINVAL;
            goto done;
        }
        if ((data[6] < 0) || (data[6] > 1)) {
            PRINTM(MERROR, "Invalid argument for extended scan\n");
            ret = -EINVAL;
            goto done;
        }
        req->action = MLAN_ACT_SET;
        memcpy(&scan->param.scan_cfg, data, sizeof(data));
    } else
        req->action = MLAN_ACT_GET;

    if (MLAN_STATUS_SUCCESS != woal_request_ioctl(priv, req, MOAL_IOCTL_WAIT)) {
        ret = -EFAULT;
        goto done;
    }
    if (!user_data_len) {
        memcpy(respbuf, &scan->param.scan_cfg, sizeof(mlan_scan_cfg));
        ret = sizeof(mlan_scan_cfg);
    }
  done:
    if (req)
        kfree(req);
    LEAVE();
    return ret;
}

#ifdef STA_SUPPORT
/**
 * @brief Set AP settings
 * 
 * @param priv         A pointer to moal_private structure
 * @param respbuf      A pointer to response buffer
 * @param respbuflen   Available length of response buffer
 * 
 * @return         Number of bytes written if successful, otherwise fail
 */
static int
woal_priv_set_ap(moal_private * priv, t_u8 * respbuf, t_u32 respbuflen)
{
    int ret = 0;
    t_u8 *data_ptr;
    const t_u8 bcast[MLAN_MAC_ADDR_LENGTH] = { 255, 255, 255, 255, 255, 255 };
    const t_u8 zero_mac[MLAN_MAC_ADDR_LENGTH] = { 0, 0, 0, 0, 0, 0 };
    mlan_ssid_bssid ssid_bssid;
    mlan_bss_info bss_info;
    struct mwreq *mwr;
    struct sockaddr *awrq;

    ENTER();
    data_ptr = respbuf + (strlen(CMD_MARVELL) + strlen(PRIV_CMD_SET_AP));

    mwr = (struct mwreq *) data_ptr;

    if (mwr->u.ap_addr.sa_family != ARPHRD_ETHER) {
        ret = -EINVAL;
        goto done;
    }

    awrq = (struct sockaddr *) &(mwr->u.ap_addr);

    PRINTM(MINFO, "ASSOC: WAP: sa_data: %02x:%02x:%02x:%02x:%02x:%02x\n",
           (t_u8) awrq->sa_data[0], (t_u8) awrq->sa_data[1],
           (t_u8) awrq->sa_data[2], (t_u8) awrq->sa_data[3],
           (t_u8) awrq->sa_data[4], (t_u8) awrq->sa_data[5]);

    if (MLAN_STATUS_SUCCESS !=
        woal_get_bss_info(priv, MOAL_IOCTL_WAIT, &bss_info)) {
        ret = -EFAULT;
        goto done;
    }
#ifdef REASSOCIATION
    /* Cancel re-association */
    priv->reassoc_required = MFALSE;
#endif

    /* zero_mac means disconnect */
    if (!memcmp(zero_mac, awrq->sa_data, MLAN_MAC_ADDR_LENGTH)) {
        woal_disconnect(priv, MOAL_IOCTL_WAIT, NULL);
        goto done;
    }

    /* Broadcast MAC means search for best network */
    memset(&ssid_bssid, 0, sizeof(mlan_ssid_bssid));

    if (memcmp(bcast, awrq->sa_data, MLAN_MAC_ADDR_LENGTH)) {
        /* Check if we are already assoicated to the AP */
        if (bss_info.media_connected == MTRUE) {
            if (!memcmp(awrq->sa_data, &bss_info.bssid, ETH_ALEN))
                goto done;
        }
        memcpy(&ssid_bssid.bssid, awrq->sa_data, ETH_ALEN);
    }

    if (MLAN_STATUS_SUCCESS != woal_find_best_network(priv,
                                                      MOAL_IOCTL_WAIT,
                                                      &ssid_bssid)) {
        PRINTM(MERROR, "ASSOC: WAP: MAC address not found in BSSID List\n");
        ret = -ENETUNREACH;
        goto done;
    }
    /* Zero SSID implies use BSSID to connect */
    memset(&ssid_bssid.ssid, 0, sizeof(mlan_802_11_ssid));
    if (MLAN_STATUS_SUCCESS != woal_bss_start(priv,
                                              MOAL_IOCTL_WAIT, &ssid_bssid)) {
        ret = -EFAULT;
        goto done;
    }
#ifdef REASSOCIATION
    memset(&bss_info, 0, sizeof(bss_info));
    if (MLAN_STATUS_SUCCESS != woal_get_bss_info(priv,
                                                 MOAL_IOCTL_WAIT, &bss_info)) {
        ret = -EFAULT;
        goto done;
    }
    memcpy(&priv->prev_ssid_bssid.ssid, &bss_info.ssid,
           sizeof(mlan_802_11_ssid));
    memcpy(&priv->prev_ssid_bssid.bssid, &bss_info.bssid, MLAN_MAC_ADDR_LENGTH);
#endif /* REASSOCIATION */

  done:
    LEAVE();
    return ret;

}
#endif

/**
 * @brief Set BSS mode
 * 
 * @param priv         A pointer to moal_private structure
 * @param respbuf      A pointer to response buffer
 * @param respbuflen   Available length of response buffer
 * 
 * @return         Number of bytes written if successful, otherwise fail
 */
static int
woal_priv_set_bss_mode(moal_private * priv, t_u8 * respbuf, t_u32 respbuflen)
{
    int ret = 0;
    mlan_ds_bss *bss = NULL;
    mlan_ioctl_req *req = NULL;
    struct mwreq *mwr;
    t_u8 *data_ptr;
    t_u32 mode;

    ENTER();

    data_ptr = respbuf + (strlen(CMD_MARVELL) + strlen(PRIV_CMD_SET_BSS_MODE));

    mwr = (struct mwreq *) data_ptr;
    mode = mwr->u.mode;
    req = woal_alloc_mlan_ioctl_req(sizeof(mlan_ds_bss));
    if (req == NULL) {
        ret = -ENOMEM;
        goto done;
    }
    bss = (mlan_ds_bss *) req->pbuf;
    bss->sub_command = MLAN_OID_BSS_MODE;
    req->req_id = MLAN_IOCTL_BSS;
    req->action = MLAN_ACT_SET;

    switch (mode) {
    case MW_MODE_INFRA:
        bss->param.bss_mode = MLAN_BSS_MODE_INFRA;
        break;
    case MW_MODE_ADHOC:
        bss->param.bss_mode = MLAN_BSS_MODE_IBSS;
        break;
    case MW_MODE_AUTO:
        bss->param.bss_mode = MLAN_BSS_MODE_AUTO;
        break;
    default:
        ret = -EINVAL;
        break;
    }
    if (ret)
        goto done;
    if (MLAN_STATUS_SUCCESS != woal_request_ioctl(priv, req, MOAL_IOCTL_WAIT)) {
        ret = -EFAULT;
        goto done;
    }
  done:
    if (req)
        kfree(req);
    LEAVE();
    return ret;
}

#ifdef STA_SUPPORT
/**
 * @brief Set power management
 * 
 * @param priv         A pointer to moal_private structure
 * @param respbuf      A pointer to response buffer
 * @param respbuflen   Available length of response buffer
 * 
 * @return         Number of bytes written if successful, otherwise fail
 */
static int
woal_priv_set_power(moal_private * priv, t_u8 * respbuf, t_u32 respbuflen)
{
    struct mwreq *mwr;
    t_u8 *data_ptr;
    int ret = 0, disabled;

    ENTER();

    data_ptr = respbuf + (strlen(CMD_MARVELL) + strlen(PRIV_CMD_SET_POWER));

    mwr = (struct mwreq *) data_ptr;
    disabled = mwr->u.power.disabled;

    if (MLAN_STATUS_SUCCESS != woal_set_get_power_mgmt(priv,
                                                       MLAN_ACT_SET, &disabled,
                                                       mwr->u.power.flags)) {
        return -EFAULT;
    }
    LEAVE();
    return ret;
}

/**
 *  @brief Set essid
 * 
 * @param priv         A pointer to moal_private structure
 * @param respbuf      A pointer to response buffer
 * @param respbuflen   Available length of response buffer
 * 
 * @return         Number of bytes written if successful, otherwise fail
 */
static int
woal_priv_set_essid(moal_private * priv, t_u8 * respbuf, t_u32 respbuflen)
{
    mlan_802_11_ssid req_ssid;
    mlan_ssid_bssid ssid_bssid;
#ifdef REASSOCIATION
    moal_handle *handle = priv->phandle;
    mlan_bss_info bss_info;
#endif
    int ret = 0;
    t_u32 mode = 0;
    struct mwreq *mwr;
    t_u8 *data_ptr;

    ENTER();

    data_ptr = respbuf + (strlen(CMD_MARVELL) + strlen(PRIV_CMD_SET_ESSID));

    mwr = (struct mwreq *) data_ptr;

#ifdef REASSOCIATION
    /* Cancel re-association */
    priv->reassoc_required = MFALSE;

    if (MOAL_ACQ_SEMAPHORE_BLOCK(&handle->reassoc_sem)) {
        PRINTM(MERROR, "Acquire semaphore error, woal_set_essid\n");
        LEAVE();
        return -EBUSY;
    }
#endif /* REASSOCIATION */

    /* Check the size of the string */
    if (mwr->u.essid.length > MW_ESSID_MAX_SIZE + 1) {
        ret = -E2BIG;
        goto setessid_ret;
    }
    if (priv->scan_type == MLAN_SCAN_TYPE_PASSIVE)
        woal_set_scan_type(priv, MLAN_SCAN_TYPE_ACTIVE);
    memset(&req_ssid, 0, sizeof(mlan_802_11_ssid));
    memset(&ssid_bssid, 0, sizeof(mlan_ssid_bssid));

    req_ssid.ssid_len = mwr->u.essid.length;

    /* 
     * Check if we asked for 'any' or 'particular'
     */
    if (!mwr->u.essid.flags) {
#ifdef REASSOCIATION
        if (!req_ssid.ssid_len) {
            memset(&priv->prev_ssid_bssid.ssid, 0x00, sizeof(mlan_802_11_ssid));
            memset(&priv->prev_ssid_bssid.bssid, 0x00, MLAN_MAC_ADDR_LENGTH);
            goto setessid_ret;
        }
#endif
        /* Do normal SSID scanning */
        if (MLAN_STATUS_SUCCESS !=
            woal_request_scan(priv, MOAL_IOCTL_WAIT, NULL)) {
            ret = -EFAULT;
            goto setessid_ret;
        }
    } else {
        /* Set the SSID */
        memcpy(req_ssid.ssid, mwr->u.essid.pointer,
               MIN(req_ssid.ssid_len, MLAN_MAX_SSID_LENGTH));
        if (!req_ssid.ssid_len || (MFALSE == woal_ssid_valid(&req_ssid))) {
            PRINTM(MERROR, "Invalid SSID - aborting set_essid\n");
            ret = -EINVAL;
            goto setessid_ret;
        }

        PRINTM(MINFO, "Requested new SSID = %s\n", (char *) req_ssid.ssid);
        memcpy(&ssid_bssid.ssid, &req_ssid, sizeof(mlan_802_11_ssid));
        if (MTRUE == woal_is_connected(priv, &ssid_bssid)) {
            PRINTM(MIOCTL, "Already connect to the network\n");
            goto setessid_ret;
        }

        if (mwr->u.essid.flags != 0xFFFF) {
            if (MLAN_STATUS_SUCCESS != woal_find_essid(priv, &ssid_bssid)) {
                /* Do specific SSID scanning */
                if (MLAN_STATUS_SUCCESS !=
                    woal_request_scan(priv, MOAL_IOCTL_WAIT, &req_ssid)) {
                    ret = -EFAULT;
                    goto setessid_ret;
                }
            }
        }

    }

    mode = woal_get_mode(priv, MOAL_IOCTL_WAIT);

    if (mode != MW_MODE_ADHOC) {
        if (MLAN_STATUS_SUCCESS !=
            woal_find_best_network(priv, MOAL_IOCTL_WAIT, &ssid_bssid)) {
            ret = -EFAULT;
            goto setessid_ret;
        }
    } else if (MLAN_STATUS_SUCCESS !=
               woal_find_best_network(priv, MOAL_IOCTL_WAIT, &ssid_bssid))
        /* Adhoc start, Check the channel command */
        woal_11h_channel_check_ioctl(priv);

    /* Connect to BSS by ESSID */
    memset(&ssid_bssid.bssid, 0, MLAN_MAC_ADDR_LENGTH);

    if (MLAN_STATUS_SUCCESS != woal_bss_start(priv,
                                              MOAL_IOCTL_WAIT, &ssid_bssid)) {
        ret = -EFAULT;
        goto setessid_ret;
    }
#ifdef REASSOCIATION
    memset(&bss_info, 0, sizeof(bss_info));
    if (MLAN_STATUS_SUCCESS != woal_get_bss_info(priv,
                                                 MOAL_IOCTL_WAIT, &bss_info)) {
        ret = -EFAULT;
        goto setessid_ret;
    }
    memcpy(&priv->prev_ssid_bssid.ssid, &bss_info.ssid,
           sizeof(mlan_802_11_ssid));
    memcpy(&priv->prev_ssid_bssid.bssid, &bss_info.bssid, MLAN_MAC_ADDR_LENGTH);
#endif /* REASSOCIATION */

  setessid_ret:
    if (priv->scan_type == MLAN_SCAN_TYPE_PASSIVE)
        woal_set_scan_type(priv, MLAN_SCAN_TYPE_PASSIVE);
#ifdef REASSOCIATION
    MOAL_REL_SEMAPHORE(&handle->reassoc_sem);
#endif
    LEAVE();
    return ret;
}

/**
 *  @brief Set authentication mode parameters
 * 
 * @param priv         A pointer to moal_private structure
 * @param respbuf      A pointer to response buffer
 * @param respbuflen   Available length of response buffer
 * 
 * @return         Number of bytes written if successful, otherwise fail
 */
static int
woal_priv_set_auth(moal_private * priv, t_u8 * respbuf, t_u32 respbuflen)
{
    struct mwreq *mwr;
    t_u8 *data_ptr;
    int ret = 0;
    t_u32 auth_mode = 0;
    t_u32 encrypt_mode = 0;

    ENTER();

    data_ptr = respbuf + (strlen(CMD_MARVELL) + strlen(PRIV_CMD_SET_AUTH));

    mwr = (struct mwreq *) data_ptr;

    switch (mwr->u.param.flags & MW_AUTH_INDEX) {
    case MW_AUTH_CIPHER_PAIRWISE:
    case MW_AUTH_CIPHER_GROUP:
        if (mwr->u.param.value & MW_AUTH_CIPHER_NONE)
            encrypt_mode = MLAN_ENCRYPTION_MODE_NONE;
        else if (mwr->u.param.value & MW_AUTH_CIPHER_WEP40)
            encrypt_mode = MLAN_ENCRYPTION_MODE_WEP40;
        else if (mwr->u.param.value & MW_AUTH_CIPHER_WEP104)
            encrypt_mode = MLAN_ENCRYPTION_MODE_WEP104;
        else if (mwr->u.param.value & MW_AUTH_CIPHER_TKIP)
            encrypt_mode = MLAN_ENCRYPTION_MODE_TKIP;
        else if (mwr->u.param.value & MW_AUTH_CIPHER_CCMP)
            encrypt_mode = MLAN_ENCRYPTION_MODE_CCMP;
        if (MLAN_STATUS_SUCCESS !=
            woal_set_encrypt_mode(priv, MOAL_IOCTL_WAIT, encrypt_mode))
            ret = -EFAULT;
        break;
    case MW_AUTH_80211_AUTH_ALG:
        switch (mwr->u.param.value) {
        case MW_AUTH_ALG_SHARED_KEY:
            PRINTM(MINFO, "Auth mode shared key!\n");
            auth_mode = MLAN_AUTH_MODE_SHARED;
            break;
        case MW_AUTH_ALG_LEAP:
            PRINTM(MINFO, "Auth mode LEAP!\n");
            auth_mode = MLAN_AUTH_MODE_NETWORKEAP;
            break;
        case MW_AUTH_ALG_OPEN_SYSTEM:
            PRINTM(MINFO, "Auth mode open!\n");
            auth_mode = MLAN_AUTH_MODE_OPEN;
            break;
        case MW_AUTH_ALG_SHARED_KEY | MW_AUTH_ALG_OPEN_SYSTEM:
        default:
            PRINTM(MINFO, "Auth mode auto!\n");
            auth_mode = MLAN_AUTH_MODE_AUTO;
            break;
        }
        if (MLAN_STATUS_SUCCESS !=
            woal_set_auth_mode(priv, MOAL_IOCTL_WAIT, auth_mode))
            ret = -EFAULT;
        break;
    case MW_AUTH_WPA_ENABLED:
        if (MLAN_STATUS_SUCCESS !=
            woal_set_wpa_enable(priv, MOAL_IOCTL_WAIT, mwr->u.param.value))
            ret = -EFAULT;
        break;
#define MW_AUTH_WAPI_ENABLED    0x20
    case MW_AUTH_WAPI_ENABLED:
        if (MLAN_STATUS_SUCCESS !=
            woal_set_wapi_enable(priv, MOAL_IOCTL_WAIT, mwr->u.param.value))
            ret = -EFAULT;
        break;
    case MW_AUTH_WPA_VERSION:
        /* set WPA_VERSION_DISABLED/VERSION_WPA/VERSION_WP2 */
        priv->wpa_version = mwr->u.param.value;
        break;
    case MW_AUTH_KEY_MGMT:
        /* set KEY_MGMT_802_1X/KEY_MGMT_PSK */
        priv->key_mgmt = mwr->u.param.value;
        break;
    case MW_AUTH_TKIP_COUNTERMEASURES:
    case MW_AUTH_DROP_UNENCRYPTED:
    case MW_AUTH_RX_UNENCRYPTED_EAPOL:
    case MW_AUTH_ROAMING_CONTROL:
    case MW_AUTH_PRIVACY_INVOKED:
        break;
    default:
        ret = -EOPNOTSUPP;
        break;
    }
    LEAVE();
    return ret;
}

/** 
 *  @brief Get current BSSID            
 * 
 * @param priv         A pointer to moal_private structure
 * @param respbuf      A pointer to response buffer
 * @param respbuflen   Available length of response buffer
 * 
 * @return         Number of bytes written if successful else negative value
 */
static int
woal_priv_get_ap(moal_private * priv, t_u8 * respbuf, t_u32 respbuflen)
{
    struct mwreq *mwr;
    t_u8 *data_ptr;
    int ret = 0;
    mlan_bss_info bss_info;

    ENTER();

    data_ptr = respbuf + (strlen(CMD_MARVELL) + strlen(PRIV_CMD_GET_AP));

    mwr = (struct mwreq *) data_ptr;

    memset(&bss_info, 0, sizeof(bss_info));

    if ((ret =
         woal_get_bss_info(priv, MOAL_IOCTL_WAIT,
                           &bss_info)) != MLAN_STATUS_SUCCESS) {
        return -EFAULT;
    }

    if (bss_info.media_connected == MTRUE) {
        memcpy(mwr->u.ap_addr.sa_data, &bss_info.bssid, MLAN_MAC_ADDR_LENGTH);
    } else {
        memset(mwr->u.ap_addr.sa_data, 0, MLAN_MAC_ADDR_LENGTH);
    }
    mwr->u.ap_addr.sa_family = ARPHRD_ETHER;
    ret = sizeof(struct mwreq);

    LEAVE();
    return ret;
}

/** 
 *  @brief  Get power management 
 * 
 * @param priv         A pointer to moal_private structure
 * @param respbuf      A pointer to response buffer
 * @param respbuflen   Available length of response buffer
 * 
 * @return         Number of bytes written if successful else negative value
 */
static int
woal_priv_get_power(moal_private * priv, t_u8 * respbuf, t_u32 respbuflen)
{
    struct mwreq *mwr;
    t_u8 *data_ptr;
    int ret = 0, ps_mode;

    ENTER();

    data_ptr = respbuf + (strlen(CMD_MARVELL) + strlen(PRIV_CMD_GET_POWER));

    mwr = (struct mwreq *) data_ptr;

    if (MLAN_STATUS_SUCCESS != woal_set_get_power_mgmt(priv,
                                                       MLAN_ACT_GET, &ps_mode,
                                                       0)) {
        return -EFAULT;
    }

    if (ps_mode)
        mwr->u.power.disabled = 0;
    else
        mwr->u.power.disabled = 1;

    mwr->u.power.value = 0;
    ret = sizeof(struct mwreq);
    LEAVE();
    return ret;
}
#endif /* STA_SUPPORT */

/**
 *  @brief Set priv command for Android
 *  @param dev          A pointer to net_device structure
 *  @param req          A pointer to ifreq structure
 *
 *  @return             0 --success, otherwise fail
 */
int
woal_android_priv_cmd(struct net_device *dev, struct ifreq *req)
{
    int ret = 0;
    android_wifi_priv_cmd priv_cmd;
    moal_private *priv = (moal_private *) netdev_priv(dev);
    char *buf = NULL;
    char *pdata;
#ifdef STA_SUPPORT
    int power_mode = 0;
    int band = 0;
    char *pband = NULL;
    mlan_bss_info bss_info;
    mlan_ds_get_signal signal;
    mlan_rate_cfg_t rate;
    t_u8 country_code[COUNTRY_CODE_LEN];
#endif
    int len = 0;
    int buf_len = 0;

    ENTER();
    if (copy_from_user(&priv_cmd, req->ifr_data, sizeof(android_wifi_priv_cmd))) {
        ret = -EFAULT;
        goto done;
    }
#define CMD_BUF_LEN   2048
    buf_len = MAX(CMD_BUF_LEN, priv_cmd.total_len);
    buf = kzalloc(buf_len, GFP_KERNEL);
    if (!buf) {
        PRINTM(MERROR, "%s: failed to allocate memory\n", __FUNCTION__);
        ret = -ENOMEM;
        goto done;
    }
    if (copy_from_user(buf, priv_cmd.buf, priv_cmd.total_len)) {
        ret = -EFAULT;
        goto done;
    }

    PRINTM(MIOCTL, "Android priv cmd: [%s] on [%s]\n", buf, req->ifr_name);

    if (strncmp(buf, CMD_MARVELL, strlen(CMD_MARVELL)) == 0) {
        /* This command has come from mlanutl app */

        /* Check command */
        if (strnicmp
            (buf + strlen(CMD_MARVELL), PRIV_CMD_VERSION,
             strlen(PRIV_CMD_VERSION)) == 0) {
            /* Get version */
            len = woal_get_priv_driver_version(priv, buf, priv_cmd.total_len);
            goto handled;
        } else
            if (strnicmp
                (buf + strlen(CMD_MARVELL), PRIV_CMD_BANDCFG,
                 strlen(PRIV_CMD_BANDCFG)) == 0) {
            /* Set/Get band configuration */
            len = woal_setget_priv_bandcfg(priv, buf, priv_cmd.total_len);
            goto handled;
        } else
            if (strnicmp
                (buf + strlen(CMD_MARVELL), PRIV_CMD_HOSTCMD,
                 strlen(PRIV_CMD_HOSTCMD)) == 0) {
            /* hostcmd configuration */
            len = woal_priv_hostcmd(priv, buf, priv_cmd.total_len);
            goto handled;
        } else
            if (strnicmp
                (buf + strlen(CMD_MARVELL), PRIV_CMD_HTTXCFG,
                 strlen(PRIV_CMD_HTTXCFG)) == 0) {
            /* Set/Get HT Tx configuration */
            len = woal_setget_priv_httxcfg(priv, buf, priv_cmd.total_len);
            goto handled;
        } else
            if (strnicmp
                (buf + strlen(CMD_MARVELL), PRIV_CMD_HTCAPINFO,
                 strlen(PRIV_CMD_HTCAPINFO)) == 0) {
            /* Set/Get HT Capability information */
            len = woal_setget_priv_htcapinfo(priv, buf, priv_cmd.total_len);
            goto handled;
        } else
            if (strnicmp
                (buf + strlen(CMD_MARVELL), PRIV_CMD_ADDBAPARA,
                 strlen(PRIV_CMD_ADDBAPARA)) == 0) {
            /* Set/Get Add BA parameters */
            len = woal_setget_priv_addbapara(priv, buf, priv_cmd.total_len);
            goto handled;
        } else
            if (strnicmp
                (buf + strlen(CMD_MARVELL), PRIV_CMD_AGGRPRIOTBL,
                 strlen(PRIV_CMD_AGGRPRIOTBL)) == 0) {
            /* Set/Get Aggregation priority table parameters */
            len = woal_setget_priv_aggrpriotbl(priv, buf, priv_cmd.total_len);
            goto handled;
        } else
            if (strnicmp
                (buf + strlen(CMD_MARVELL), PRIV_CMD_ADDBAREJECT,
                 strlen(PRIV_CMD_ADDBAREJECT)) == 0) {
            /* Set/Get Add BA reject parameters */
            len = woal_setget_priv_addbareject(priv, buf, priv_cmd.total_len);
            goto handled;
        } else
            if (strnicmp
                (buf + strlen(CMD_MARVELL), PRIV_CMD_DELBA,
                 strlen(PRIV_CMD_DELBA)) == 0) {
            /* Delete selective BA based on parameters */
            len = woal_priv_delba(priv, buf, priv_cmd.total_len);
            goto handled;
        } else
            if (strnicmp
                (buf + strlen(CMD_MARVELL), PRIV_CMD_DATARATE,
                 strlen(PRIV_CMD_DATARATE)) == 0) {
            /* Get data rate */
            len = woal_get_priv_datarate(priv, buf, priv_cmd.total_len);
            goto handled;
        } else
            if (strnicmp
                (buf + strlen(CMD_MARVELL), PRIV_CMD_TXRATECFG,
                 strlen(PRIV_CMD_TXRATECFG)) == 0) {
            /* Set/Get tx rate cfg */
            len = woal_setget_priv_txratecfg(priv, buf, priv_cmd.total_len);
            goto handled;
        } else
            if (strnicmp
                (buf + strlen(CMD_MARVELL), PRIV_CMD_CUSTOMIE,
                 strlen(PRIV_CMD_CUSTOMIE)) == 0) {
            /* Custom IE configuration */
            len = woal_priv_customie(priv, buf, priv_cmd.total_len);
            goto handled;
        } else
            if (strnicmp
                (buf + strlen(CMD_MARVELL), PRIV_CMD_ESUPPMODE,
                 strlen(PRIV_CMD_ESUPPMODE)) == 0) {
            /* Esupplicant mode configuration */
            len = woal_setget_priv_esuppmode(priv, buf, priv_cmd.total_len);
            goto handled;
        } else
            if (strnicmp
                (buf + strlen(CMD_MARVELL), PRIV_CMD_PASSPHRASE,
                 strlen(PRIV_CMD_PASSPHRASE)) == 0) {
            /* Esupplicant passphrase configuration */
            len = woal_setget_priv_passphrase(priv, buf, priv_cmd.total_len);
            goto handled;
        } else
            if (strnicmp
                (buf + strlen(CMD_MARVELL), PRIV_CMD_DEAUTH,
                 strlen(PRIV_CMD_DEAUTH)) == 0) {
            /* Deauth */
            len = woal_priv_deauth(priv, buf, priv_cmd.total_len);
            goto handled;
#ifdef UAP_SUPPORT
        } else
            if (strnicmp
                (buf + strlen(CMD_MARVELL), PRIV_CMD_AP_DEAUTH,
                 strlen(PRIV_CMD_AP_DEAUTH)) == 0) {
            /* AP Deauth */
            len = woal_priv_ap_deauth(priv, buf, priv_cmd.total_len);
            goto handled;
        } else
            if (strnicmp
                (buf + strlen(CMD_MARVELL), PRIV_CMD_GET_STA_LIST,
                 strlen(PRIV_CMD_GET_STA_LIST)) == 0) {
            /* Get STA list */
            len = woal_priv_get_sta_list(priv, buf, priv_cmd.total_len);
            goto handled;
        } else
            if (strnicmp
                (buf + strlen(CMD_MARVELL), PRIV_CMD_BSS_CONFIG,
                 strlen(PRIV_CMD_BSS_CONFIG)) == 0) {
            /* BSS config */
            len = woal_priv_bss_config(priv, buf, priv_cmd.total_len);
            goto handled;
#endif
#if defined(WIFI_DIRECT_SUPPORT)
#if defined(STA_SUPPORT) && defined(UAP_SUPPORT)
        } else
            if (strnicmp
                (buf + strlen(CMD_MARVELL), PRIV_CMD_BSSROLE,
                 strlen(PRIV_CMD_BSSROLE)) == 0) {
            /* BSS Role */
            len = woal_priv_bssrole(priv, buf, priv_cmd.total_len);
            goto handled;
#endif
#endif
#ifdef STA_SUPPORT
        } else
            if (strnicmp
                (buf + strlen(CMD_MARVELL), PRIV_CMD_SETUSERSCAN,
                 strlen(PRIV_CMD_SETUSERSCAN)) == 0) {
            /* Set user scan */
            len = woal_priv_setuserscan(priv, buf, priv_cmd.total_len);
            goto handled;
        } else
            if (strnicmp
                (buf + strlen(CMD_MARVELL), PRIV_CMD_GETSCANTABLE,
                 strlen(PRIV_CMD_GETSCANTABLE)) == 0) {
            /* Get scan table */
            len = woal_priv_getscantable(priv, buf, priv_cmd.total_len);
            goto handled;
#endif
        } else
            if (strnicmp
                (buf + strlen(CMD_MARVELL), PRIV_CMD_DEEPSLEEP,
                 strlen(PRIV_CMD_DEEPSLEEP)) == 0) {
            /* Deep sleep */
            len = woal_priv_setgetdeepsleep(priv, buf, priv_cmd.total_len);
            goto handled;
        } else
            if (strnicmp
                (buf + strlen(CMD_MARVELL), PRIV_CMD_IPADDR,
                 strlen(PRIV_CMD_IPADDR)) == 0) {
            /* IP address */
            len = woal_priv_setgetipaddr(priv, buf, priv_cmd.total_len);
            goto handled;
        } else
            if (strnicmp
                (buf + strlen(CMD_MARVELL), PRIV_CMD_WPSSESSION,
                 strlen(PRIV_CMD_WPSSESSION)) == 0) {
            /* WPS Session */
            len = woal_priv_setwpssession(priv, buf, priv_cmd.total_len);
            goto handled;
        } else
            if (strnicmp
                (buf + strlen(CMD_MARVELL), PRIV_CMD_OTPUSERDATA,
                 strlen(PRIV_CMD_OTPUSERDATA)) == 0) {
            /* OTP user data */
            len = woal_priv_otpuserdata(priv, buf, priv_cmd.total_len);
            goto handled;
        } else
            if (strnicmp
                (buf + strlen(CMD_MARVELL), PRIV_CMD_COUNTRYCODE,
                 strlen(PRIV_CMD_COUNTRYCODE)) == 0) {
            /* Country code */
            len = woal_priv_set_get_countrycode(priv, buf, priv_cmd.total_len);
            goto handled;
        } else
            if (strnicmp
                (buf + strlen(CMD_MARVELL), PRIV_CMD_WAKEUPREASON,
                 strlen(PRIV_CMD_WAKEUPREASON)) == 0) {
            /* wakeup reason */
            len = woal_priv_getwakeupreason(priv, buf, priv_cmd.total_len);
            goto handled;
#ifdef STA_SUPPORT
        } else
            if (strnicmp
                (buf + strlen(CMD_MARVELL), PRIV_CMD_LISTENINTERVAL,
                 strlen(PRIV_CMD_LISTENINTERVAL)) == 0) {
            /* Listen Interval */
            len =
                woal_priv_set_get_listeninterval(priv, buf, priv_cmd.total_len);
            goto handled;
#endif
#ifdef DEBUG_LEVEL1
        } else
            if (strnicmp
                (buf + strlen(CMD_MARVELL), PRIV_CMD_DRVDBG,
                 strlen(PRIV_CMD_DRVDBG)) == 0) {
            /* Listen Interval */
            len = woal_priv_set_get_drvdbg(priv, buf, priv_cmd.total_len);
            goto handled;
#endif
        } else
            if (strnicmp
                (buf + strlen(CMD_MARVELL), PRIV_CMD_HSCFG,
                 strlen(PRIV_CMD_HSCFG)) == 0) {
            /* Listen Interval */
            len = woal_priv_hscfg(priv, buf, priv_cmd.total_len, MTRUE);
            goto handled;
        } else
            if (strnicmp
                (buf + strlen(CMD_MARVELL), PRIV_CMD_HSSETPARA,
                 strlen(PRIV_CMD_HSSETPARA)) == 0) {
            /* Listen Interval */
            len = woal_priv_hssetpara(priv, buf, priv_cmd.total_len);
            goto handled;
        } else
            if (strnicmp
                (buf + strlen(CMD_MARVELL), PRIV_CMD_SCANCFG,
                 strlen(PRIV_CMD_SCANCFG)) == 0) {
            /* Listen Interval */
            len = woal_priv_set_get_scancfg(priv, buf, priv_cmd.total_len);
            goto handled;
        } else
            if (strnicmp
                (buf + strlen(CMD_MARVELL), PRIV_CMD_SET_BSS_MODE,
                 strlen(PRIV_CMD_SET_BSS_MODE)) == 0) {
            /* Set bss mode */
            len = woal_priv_set_bss_mode(priv, buf, priv_cmd.total_len);
            goto handled;
#ifdef STA_SUPPORT
        } else
            if (strnicmp
                (buf + strlen(CMD_MARVELL), PRIV_CMD_SET_AP,
                 strlen(PRIV_CMD_SET_AP)) == 0) {
            /* Set AP */
            len = woal_priv_set_ap(priv, buf, priv_cmd.total_len);
            goto handled;
        } else
            if (strnicmp
                (buf + strlen(CMD_MARVELL), PRIV_CMD_SET_POWER,
                 strlen(PRIV_CMD_SET_POWER)) == 0) {
            /* Set power management parameters */
            len = woal_priv_set_power(priv, buf, priv_cmd.total_len);
            goto handled;
        } else
            if (strnicmp
                (buf + strlen(CMD_MARVELL), PRIV_CMD_SET_ESSID,
                 strlen(PRIV_CMD_SET_ESSID)) == 0) {
            /* Set essid */
            len = woal_priv_set_essid(priv, buf, priv_cmd.total_len);
            goto handled;
        } else
            if (strnicmp
                (buf + strlen(CMD_MARVELL), PRIV_CMD_SET_AUTH,
                 strlen(PRIV_CMD_SET_AUTH)) == 0) {
            /* Set authentication mode parameters */
            len = woal_priv_set_auth(priv, buf, priv_cmd.total_len);
            goto handled;
        } else
            if (strnicmp
                (buf + strlen(CMD_MARVELL), PRIV_CMD_GET_AP,
                 strlen(PRIV_CMD_GET_AP)) == 0) {
            /* Get AP */
            len = woal_priv_get_ap(priv, buf, priv_cmd.total_len);
            goto handled;
        } else
            if (strnicmp
                (buf + strlen(CMD_MARVELL), PRIV_CMD_GET_POWER,
                 strlen(PRIV_CMD_GET_POWER)) == 0) {
            /* Get power management parameters */
            len = woal_priv_get_power(priv, buf, priv_cmd.total_len);
            goto handled;
#endif
        } else {
            /* Fall through, after stripping off the custom header */
            buf += strlen(CMD_MARVELL);
        }
    }
#ifdef STA_SUPPORT
    if (strncmp(buf, "RSSILOW-THRESHOLD", strlen("RSSILOW-THRESHOLD")) == 0) {
        pdata = buf + strlen("RSSILOW-THRESHOLD") + 1;
        if (MLAN_STATUS_SUCCESS != woal_set_rssi_low_threshold(priv, pdata)) {
            ret = -EFAULT;
            goto done;
        }
        len = sprintf(buf, "OK\n") + 1;
    } else if (strncmp(buf, "RSSI", strlen("RSSI")) == 0) {
        if (MLAN_STATUS_SUCCESS != woal_get_bss_info(priv,
                                                     MOAL_IOCTL_WAIT,
                                                     &bss_info)) {
            ret = -EFAULT;
            goto done;
        }
        if (bss_info.media_connected) {
            if (MLAN_STATUS_SUCCESS != woal_get_signal_info(priv,
                                                            MOAL_IOCTL_WAIT,
                                                            &signal)) {
                ret = -EFAULT;
                goto done;
            }
            len = sprintf(buf, "%.32s rssi %d\n", bss_info.ssid.ssid,
                          signal.bcn_rssi_avg) + 1;
        } else {
            len = sprintf(buf, "OK\n") + 1;
        }
    } else if (strncmp(buf, "LINKSPEED", strlen("LINKSPEED")) == 0) {
        if (MLAN_STATUS_SUCCESS != woal_set_get_data_rate(priv, MLAN_ACT_GET,
                                                          &rate)) {
            ret = -EFAULT;
            goto done;
        }
        PRINTM(MIOCTL, "tx rate=%d\n", (int) rate.rate);
        len =
            sprintf(buf, "LinkSpeed %d\n", (int) (rate.rate * 500000 / 1000000))
            + 1;
    } else
#endif
    if (strncmp(buf, "MACADDR", strlen("MACADDR")) == 0) {
        len = sprintf(buf, "Macaddr = %02X:%02X:%02X:%02X:%02X:%02X\n",
                      priv->current_addr[0], priv->current_addr[1],
                      priv->current_addr[2], priv->current_addr[3],
                      priv->current_addr[4], priv->current_addr[5]) + 1;
    }
#ifdef STA_SUPPORT
    else if (strncmp(buf, "GETPOWER", strlen("GETPOWER")) == 0) {
        if (MLAN_STATUS_SUCCESS != woal_get_powermode(priv, &power_mode)) {
            ret = -EFAULT;
            goto done;
        }
        len = sprintf(buf, "powermode = %d\n", power_mode) + 1;
    } else if (strncmp(buf, "SCAN-ACTIVE", strlen("SCAN-ACTIVE")) == 0) {
        if (MLAN_STATUS_SUCCESS != woal_set_scan_type(priv,
                                                      MLAN_SCAN_TYPE_ACTIVE)) {
            ret = -EFAULT;
            goto done;
        }
        priv->scan_type = MLAN_SCAN_TYPE_ACTIVE;
        PRINTM(MIOCTL, "Set Active Scan\n");
        len = sprintf(buf, "OK\n") + 1;
    } else if (strncmp(buf, "SCAN-PASSIVE", strlen("SCAN-PASSIVE")) == 0) {
        if (MLAN_STATUS_SUCCESS != woal_set_scan_type(priv,
                                                      MLAN_SCAN_TYPE_PASSIVE)) {
            ret = -EFAULT;
            goto done;
        }
        priv->scan_type = MLAN_SCAN_TYPE_PASSIVE;
        PRINTM(MIOCTL, "Set Passive Scan\n");
        len = sprintf(buf, "OK\n") + 1;
    } else if (strncmp(buf, "POWERMODE", strlen("POWERMODE")) == 0) {
        pdata = buf + strlen("POWERMODE") + 1;
        if (MLAN_STATUS_SUCCESS != woal_set_powermode(priv, pdata)) {
            ret = -EFAULT;
            goto done;
        }
        len = sprintf(buf, "OK\n") + 1;
    } else if (strncmp(buf, "COUNTRY", strlen("COUNTRY")) == 0) {
        memset(country_code, 0, sizeof(country_code));
        memcpy(country_code, buf + strlen("COUNTRY") + 1,
               strlen(buf) - strlen("COUNTRY") - 1);
        PRINTM(MIOCTL, "Set COUNTRY %s\n", country_code);
        if (MLAN_STATUS_SUCCESS != woal_set_region_code(priv, country_code)) {
            ret = -EFAULT;
            goto done;
        }
        len = sprintf(buf, "OK\n") + 1;
    } else if (memcmp(buf, WEXT_CSCAN_HEADER, WEXT_CSCAN_HEADER_SIZE) == 0) {
        PRINTM(MIOCTL, "Set Combo Scan\n");
        if (MLAN_STATUS_SUCCESS != woal_set_combo_scan(priv, buf,
                                                       priv_cmd.total_len)) {
            ret = -EFAULT;
            goto done;
        }
        len = sprintf(buf, "OK\n") + 1;
    } else if (strncmp(buf, "GETBAND", strlen("GETBAND")) == 0) {
        if (MLAN_STATUS_SUCCESS != woal_get_band(priv, &band)) {
            ret = -EFAULT;
            goto done;
        }
        len = sprintf(buf, "Band %d\n", band) + 1;
    } else if (strncmp(buf, "SETBAND", strlen("SETBAND")) == 0) {
        pband = buf + strlen("SETBAND") + 1;
        if (MLAN_STATUS_SUCCESS != woal_set_band(priv, pband)) {
            ret = -EFAULT;
            goto done;
        }
        len = sprintf(buf, "OK\n") + 1;
    }
#endif
    else if (strncmp(buf, "START", strlen("START")) == 0) {
        len = sprintf(buf, "OK\n") + 1;
    } else if (strncmp(buf, "STOP", strlen("STOP")) == 0) {
        len = sprintf(buf, "OK\n") + 1;
    }
#ifdef UAP_SUPPORT
    else if (strncmp(buf, "AP_BSS_START", strlen("AP_BSS_START")) == 0) {
        if ((ret == woal_uap_bss_ctrl(priv, MOAL_IOCTL_WAIT, UAP_BSS_START)))
            goto done;
        len = sprintf(buf, "OK\n") + 1;
    } else if (strncmp(buf, "AP_BSS_STOP", strlen("AP_BSS_STOP")) == 0) {
        if ((ret == woal_uap_bss_ctrl(priv, MOAL_IOCTL_WAIT, UAP_BSS_STOP)))
            goto done;
        len = sprintf(buf, "OK\n") + 1;
    } else if (strncmp(buf, "AP_SET_CFG", strlen("AP_SET_CFG")) == 0) {
        pdata = buf + strlen("AP_SET_CFG") + 1;
        if ((ret =
             woal_uap_set_ap_cfg(priv, pdata,
                                 priv_cmd.used_len - strlen("AP_SET_CFG") - 1)))
            goto done;
        len = sprintf(buf, "OK\n") + 1;
    } else if (strncmp(buf, "WL_FW_RELOAD", strlen("WL_FW_RELOAD")) == 0) {
        len = sprintf(buf, "OK\n") + 1;
    } else if (strncmp(buf, "AP_GET_STA_LIST", strlen("AP_GET_STA_LIST")) == 0) {
        // TODO Add STA list support
        len = sprintf(buf, "OK\n") + 1;
    }
#endif
    else if (strncmp(buf, "SETSUSPENDOPT", strlen("SETSUSPENDOPT")) == 0) {
        /* it will be done by GUI */
        len = sprintf(buf, "OK\n") + 1;
    } else if (strncmp(buf, "BTCOEXMODE", strlen("BTCOEXMODE")) == 0) {
        len = sprintf(buf, "OK\n") + 1;
    } else if (strncmp(buf, "BTCOEXSCAN-START", strlen("BTCOEXSCAN-START")) ==
               0) {
        len = sprintf(buf, "OK\n") + 1;
    } else if (strncmp(buf, "BTCOEXSCAN-STOP", strlen("BTCOEXSCAN-STOP")) == 0) {
        len = sprintf(buf, "OK\n") + 1;
    } else if (strncmp(buf, "BTCOEXSCAN-START", strlen("BTCOEXSCAN-START")) ==
               0) {
        len = sprintf(buf, "OK\n") + 1;
    } else if (strncmp(buf, "BTCOEXSCAN-STOP", strlen("BTCOEXSCAN-STOP")) == 0) {
        len = sprintf(buf, "OK\n") + 1;
    }
#ifdef STA_SUPPORT
    else if (strncmp(buf, "BGSCAN-START", strlen("BGSCAN-START")) == 0) {
        len = sprintf(buf, "OK\n") + 1;
    } else if (strncmp(buf, "BGSCAN-CONFIG", strlen("BGSCAN-CONFIG")) == 0) {
        if (MLAN_STATUS_SUCCESS !=
            woal_set_bg_scan(priv, buf, priv_cmd.total_len)) {
            ret = -EFAULT;
            goto done;
        }
        priv->bg_scan_start = MTRUE;
        priv->bg_scan_reported = MFALSE;
        len = sprintf(buf, "OK\n") + 1;
    } else if (strncmp(buf, "BGSCAN-STOP", strlen("BGSCAN-STOP")) == 0) {
        if (priv->bg_scan_start && !priv->scan_cfg.rssi_threshold) {
            if (MLAN_STATUS_SUCCESS != woal_stop_bg_scan(priv)) {
                ret = -EFAULT;
                goto done;
            }
            priv->bg_scan_start = MFALSE;
            priv->bg_scan_reported = MFALSE;
        }
        len = sprintf(buf, "OK\n") + 1;
    } else if (strncmp(buf, "RXFILTER-START", strlen("RXFILTER-START")) == 0) {
#ifdef MEF_CFG_RX_FILTER
        if ((ret = woal_set_rxfilter(priv, MTRUE))) {
            goto done;
        }
#endif
        len = sprintf(buf, "OK\n") + 1;
    } else if (strncmp(buf, "RXFILTER-STOP", strlen("RXFILTER-STOP")) == 0) {
#ifdef MEF_CFG_RX_FILTER
        if ((ret = woal_set_rxfilter(priv, MFALSE))) {
            goto done;
        }
#endif
        len = sprintf(buf, "OK\n") + 1;
    }
#ifdef STA_CFG80211
    else if (strncmp(buf, "GET_EVENT", strlen("GET_EVENT")) == 0) {
        if (IS_STA_CFG80211(cfg80211_wext)) {
            if (priv->last_event & EVENT_BG_SCAN_REPORT)
                woal_inform_bss_from_scan_result(priv, NULL);
        }
        len = sprintf(buf, "EVENT=%d\n", priv->last_event) + 1;
        priv->last_event = 0;
    }
#endif
    else if (strncmp(buf, "RXFILTER-ADD", strlen("RXFILTER-ADD")) == 0) {
        pdata = buf + strlen("RXFILTER-ADD") + 1;
        if (MLAN_STATUS_SUCCESS != woal_add_rxfilter(priv, pdata)) {
            ret = -EFAULT;
            goto done;
        }
        len = sprintf(buf, "OK\n") + 1;
    } else if (strncmp(buf, "RXFILTER-REMOVE", strlen("RXFILTER-REMOVE")) == 0) {
        pdata = buf + strlen("RXFILTER-REMOVE") + 1;
        if (MLAN_STATUS_SUCCESS != woal_remove_rxfilter(priv, pdata)) {
            ret = -EFAULT;
            goto done;
        }
        len = sprintf(buf, "OK\n") + 1;
    } else if (strncmp(buf, "QOSINFO", strlen("QOSINFO")) == 0) {
        pdata = buf + strlen("QOSINFO") + 1;
        if (MLAN_STATUS_SUCCESS != woal_set_qos_cfg(priv, pdata)) {
            ret = -EFAULT;
            goto done;
        }
        len = sprintf(buf, "OK\n") + 1;
    } else if (strncmp(buf, "SLEEPPD", strlen("SLEEPPD")) == 0) {
        pdata = buf + strlen("SLEEPPD") + 1;
        if (MLAN_STATUS_SUCCESS != woal_set_sleeppd(priv, pdata)) {
            ret = -EFAULT;
            goto done;
        }
        len = sprintf(buf, "OK\n") + 1;
    } else if (strncmp(buf, "SET_AP_WPS_P2P_IE",
                       strlen("SET_AP_WPS_P2P_IE")) == 0) {
        pdata = buf + strlen("SET_AP_WPS_P2P_IE") + 1;
        /* Android cmd format: "SET_AP_WPS_P2P_IE 1" -- beacon IE
           "SET_AP_WPS_P2P_IE 2" -- proberesp IE "SET_AP_WPS_P2P_IE 4" --
           assocresp IE */
#if defined(STA_CFG80211) && defined(UAP_CFG80211)
        if (MLAN_STATUS_SUCCESS != woal_set_ap_wps_p2p_ie(priv, (t_u8 *) pdata,
                                                          priv_cmd.used_len -
                                                          strlen
                                                          ("SET_AP_WPS_P2P_IE")
                                                          - 1)) {
            ret = -EFAULT;
            goto done;
        }
#endif
        len = sprintf(buf, "OK\n") + 1;
    }
#endif
    else if (strncmp(buf, "P2P_DEV_ADDR", strlen("P2P_DEV_ADDR")) == 0) {
        memset(buf, 0x0, priv_cmd.total_len);
        memcpy(buf, priv->current_addr, ETH_ALEN);
        len = ETH_ALEN;
    } else if (strncmp(buf, ("P2P_GET_NOA"), strlen("P2P_GET_NOA")) == 0) {
        /* TODO Just return '\0' */
        memset(buf, 0x0, priv_cmd.total_len);
        *buf = 0;
        len = 1;
    } else {
        PRINTM(MIOCTL, "Unknown PRIVATE command: %s, ignored\n", buf);
        ret = -EFAULT;
        goto done;
    }

  handled:
    PRINTM(MIOCTL, "PRIV Command return: %s, length=%d\n", buf, len);

    if (len > 0) {
        priv_cmd.used_len = len;
        if (priv_cmd.used_len <= priv_cmd.total_len) {
            memset(priv_cmd.buf + priv_cmd.used_len, 0,
                   priv_cmd.total_len - priv_cmd.used_len);
            if (copy_to_user(priv_cmd.buf, buf, priv_cmd.used_len)) {
                PRINTM(MERROR, "%s: failed to copy data to user buffer\n",
                       __FUNCTION__);
                ret = -EFAULT;
                goto done;
            }
            if (copy_to_user
                (req->ifr_data, &priv_cmd, sizeof(android_wifi_priv_cmd))) {
                PRINTM(MERROR,
                       "%s: failed to copy command header to user buffer\n",
                       __FUNCTION__);
                ret = -EFAULT;
            }
        } else {
            PRINTM(MERROR, "%s: the buffer supplied by appl is too small.\n",
                   __FUNCTION__);
            ret = -EFAULT;
        }
    } else {
        ret = len;
    }

  done:
    if (buf)
        kfree(buf);
    LEAVE();
    return ret;
}

/********************************************************
        Global Functions
********************************************************/
/**
 *  @brief Create a brief scan resp to relay basic BSS info to the app layer
 *
 *  When the beacon/probe response has not been buffered, use the saved BSS
 *    information available to provide a minimum response for the application
 *    ioctl retrieval routines.  Include:
 *        - Timestamp
 *        - Beacon Period
 *        - Capabilities (including WMM Element if available)
 *        - SSID
 *
 *  @param ppbuffer  Output parameter: Buffer used to create basic scan rsp
 *  @param pbss_desc Pointer to a BSS entry in the scan table to create
 *                   scan response from for delivery to the application layer
 *
 *  @return          N/A
 */
void
wlan_scan_create_brief_table_entry(t_u8 ** ppbuffer,
                                   BSSDescriptor_t * pbss_desc)
{
    t_u8 *ptmp_buf = *ppbuffer;
    t_u8 tmp_ssid_hdr[2];
    t_u8 ie_len = 0;

    ENTER();

    memcpy(ptmp_buf, pbss_desc->time_stamp, sizeof(pbss_desc->time_stamp));
    ptmp_buf += sizeof(pbss_desc->time_stamp);

    memcpy(ptmp_buf, &pbss_desc->beacon_period,
           sizeof(pbss_desc->beacon_period));
    ptmp_buf += sizeof(pbss_desc->beacon_period);

    memcpy(ptmp_buf, &pbss_desc->cap_info, sizeof(pbss_desc->cap_info));
    ptmp_buf += sizeof(pbss_desc->cap_info);

    tmp_ssid_hdr[0] = 0;        /* Element ID for SSID is zero */
    tmp_ssid_hdr[1] = pbss_desc->ssid.ssid_len;
    memcpy(ptmp_buf, tmp_ssid_hdr, sizeof(tmp_ssid_hdr));
    ptmp_buf += sizeof(tmp_ssid_hdr);

    memcpy(ptmp_buf, pbss_desc->ssid.ssid, pbss_desc->ssid.ssid_len);
    ptmp_buf += pbss_desc->ssid.ssid_len;

    if (pbss_desc->wmm_ie.vend_hdr.element_id == WMM_IE) {
        ie_len = sizeof(IEEEtypes_Header_t) + pbss_desc->wmm_ie.vend_hdr.len;
        memcpy(ptmp_buf, &pbss_desc->wmm_ie, ie_len);
        ptmp_buf += ie_len;
    }

    if (pbss_desc->pwpa_ie) {
        if ((*(pbss_desc->pwpa_ie)).vend_hdr.element_id == WPA_IE) {
            ie_len =
                sizeof(IEEEtypes_Header_t) +
                (*(pbss_desc->pwpa_ie)).vend_hdr.len;
            memcpy(ptmp_buf, pbss_desc->pwpa_ie, ie_len);
        }

        ptmp_buf += ie_len;
    }

    if (pbss_desc->prsn_ie) {
        if ((*(pbss_desc->prsn_ie)).ieee_hdr.element_id == RSN_IE) {
            ie_len =
                sizeof(IEEEtypes_Header_t) +
                (*(pbss_desc->prsn_ie)).ieee_hdr.len;
            memcpy(ptmp_buf, pbss_desc->prsn_ie, ie_len);
        }

        ptmp_buf += ie_len;
    }

    *ppbuffer = ptmp_buf;
    LEAVE();
}

/**
 *  @brief Create a wlan_ioctl_get_scan_table_entry for a given BSS
 *         Descriptor for inclusion in the ioctl response to the user space
 *         application.
 *
 *
 *  @param pbss_desc   Pointer to a BSS entry in the scan table to form
 *                     scan response from for delivery to the application layer
 *  @param ppbuffer    Output parameter: Buffer used to output scan return struct
 *  @param pspace_left Output parameter: Number of bytes available in the
 *                     response buffer.
 *
 *  @return MLAN_STATUS_SUCCESS, or < 0 with IOCTL error code
 */
int
wlan_get_scan_table_ret_entry(BSSDescriptor_t * pbss_desc,
                              t_u8 ** ppbuffer, int *pspace_left)
{
    wlan_ioctl_get_scan_table_entry *prsp_entry;
    wlan_ioctl_get_scan_table_entry tmp_rsp_entry;
    int space_needed;
    t_u8 *pcurrent;
    int variable_size;

    const int fixed_size = sizeof(wlan_ioctl_get_scan_table_entry);

    ENTER();

    pcurrent = *ppbuffer;

    /* The variable size returned is the stored beacon size */
    variable_size = pbss_desc->beacon_buf_size;

    /* If we stored a beacon and its size was zero, set the variable size
       return value to the size of the brief scan response
       wlan_scan_create_brief_table_entry creates.  Also used if we are not
       configured to store beacons in the first place */
    if (!variable_size) {
        variable_size = pbss_desc->ssid.ssid_len + 2;
        variable_size += (sizeof(pbss_desc->beacon_period)
                          + sizeof(pbss_desc->time_stamp)
                          + sizeof(pbss_desc->cap_info));
        if (pbss_desc->wmm_ie.vend_hdr.element_id == WMM_IE) {
            variable_size += (sizeof(IEEEtypes_Header_t)
                              + pbss_desc->wmm_ie.vend_hdr.len);
        }

        if (pbss_desc->pwpa_ie) {
            if ((*(pbss_desc->pwpa_ie)).vend_hdr.element_id == WPA_IE) {
                variable_size += (sizeof(IEEEtypes_Header_t)
                                  + (*(pbss_desc->pwpa_ie)).vend_hdr.len);
            }
        }

        if (pbss_desc->prsn_ie) {
            if ((*(pbss_desc->prsn_ie)).ieee_hdr.element_id == RSN_IE) {
                variable_size += (sizeof(IEEEtypes_Header_t)
                                  + (*(pbss_desc->prsn_ie)).ieee_hdr.len);
            }
        }
    }

    space_needed = fixed_size + variable_size;

    PRINTM(MINFO, "GetScanTable: need(%d), left(%d)\n",
           space_needed, *pspace_left);

    if (space_needed >= *pspace_left) {
        *pspace_left = 0;
        LEAVE();
        return -E2BIG;
    }

    *pspace_left -= space_needed;

    tmp_rsp_entry.fixed_field_length = (sizeof(tmp_rsp_entry)
                                        -
                                        sizeof(tmp_rsp_entry.fixed_field_length)
                                        -
                                        sizeof(tmp_rsp_entry.bss_info_length));

    memcpy(tmp_rsp_entry.fixed_fields.bssid,
           pbss_desc->mac_address, sizeof(prsp_entry->fixed_fields.bssid));

    tmp_rsp_entry.fixed_fields.rssi = pbss_desc->rssi;
    tmp_rsp_entry.fixed_fields.channel = pbss_desc->channel;
    tmp_rsp_entry.fixed_fields.network_tsf = pbss_desc->network_tsf;
    tmp_rsp_entry.bss_info_length = variable_size;

    /* 
     *  Copy fixed fields to user space
     */
    memcpy(pcurrent, &tmp_rsp_entry, fixed_size);
    pcurrent += fixed_size;

    if (pbss_desc->pbeacon_buf) {
        /* 
         *  Copy variable length elements to user space
         */
        memcpy(pcurrent, pbss_desc->pbeacon_buf, pbss_desc->beacon_buf_size);

        pcurrent += pbss_desc->beacon_buf_size;
    } else {
        wlan_scan_create_brief_table_entry(&pcurrent, pbss_desc);
    }

    *ppbuffer = pcurrent;

    LEAVE();

    return MLAN_STATUS_SUCCESS;
}

/**
 *  @brief ioctl function - entry point
 *
 *  @param dev      A pointer to net_device structure
 *  @param req      A pointer to ifreq structure
 *  @param cmd      Command
 *
 *  @return          0 --success, otherwise fail
 */
int
woal_do_ioctl(struct net_device *dev, struct ifreq *req, int cmd)
{
    int ret = 0;

    ENTER();

    PRINTM(MINFO, "woal_do_ioctl: ioctl cmd = 0x%x\n", cmd);
    switch (cmd) {
#ifdef WIFI_DIRECT_SUPPORT
    case WOAL_WIFIDIRECT_HOST_CMD:
        ret = woal_hostcmd_ioctl(dev, req);
        break;
#endif
    case WOAL_CUSTOM_IE_CFG:
        ret = woal_custom_ie_ioctl(dev, req);
        break;
    case WOAL_MGMT_FRAME_TX:
        ret = woal_send_host_packet(dev, req);
        break;
    case WOAL_ANDROID_PRIV_CMD:
        ret = woal_android_priv_cmd(dev, req);
        break;
    case WOAL_GET_BSS_TYPE:
        ret = woal_get_bss_type(dev, req);
        break;
    default:
#if defined(STA_WEXT)
#ifdef STA_SUPPORT
        ret = woal_wext_do_ioctl(dev, req, cmd);
#else
        ret = -EINVAL;
#endif
#else
        ret = -EINVAL;
#endif
        break;
    }

    LEAVE();
    return ret;
}
