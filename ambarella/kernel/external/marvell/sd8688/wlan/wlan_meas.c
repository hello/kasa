/**
 * @file wlan_meas.c
 *
 *  @brief Implementation of measurement interface code with the app/firmware
 *
 *  Driver implementation for sending and retrieving measurement requests
 *    and responses. 
 *  
 *  Current use is limited to 802.11h.
 *
 *  Requires use of the following preprocessor define:
 *    - ENABLE_MEAS
 *
 *  @sa wlan_meas.h
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

/*****************************************************************************/
/*  Change Log:
** 
**      9/30/05: Initial revision            
**
**
*/
/*****************************************************************************/

/*
** Include(s)
*/
#include "wlan_headers.h"

/*
** Macros
*/

/*
** Constants
*/

/** Default measurement duration when not provided by the application */
#define WLAN_MEAS_DEFAULT_MEAS_DURATION    1000U        /* TUs */

#ifdef DEBUG_LEVEL2
/** String descriptions of the different measurement enums.  Debug display */
static const char *measTypeStr[WLAN_MEAS_NUM_TYPES] = {
    "basic",
#if WLAN_MEAS_EXTRA_11H_TYPES
    "cca",
    "rpi",
#endif
#if WLAN_MEAS_EXTRA_11K_TYPES
    /* 11k additions */
    "chnld",
    "nzhst",
    "beacn",
    "frame",
    "hidnd",
    "msenh",
    "stast"
#endif
};

/** 
*** wlan_meas_get_meas_type_str
**
**  @brief Retrieve the measurement string representation of a measType enum
**
**  Used for debug display only
**
**  @param measType Measurement type enumeration input for string lookup
** 
**  @return         Constant string representing measurement type
*/
static const char *
wlan_meas_get_meas_type_str(MeasType_t measType)
{
    if (measType <= WLAN_MEAS_11H_MAX_TYPE)
        return measTypeStr[measType];

    return "Invld";
}
#endif

/**
*** wlan_meas_dump_meas_req
**
**  @brief Debug print display of the input measurement request
**
**  @param pMeasReq  Pointer to the measurement request to display
**
**  @return          void
*/
static void
wlan_meas_dump_meas_req(const HostCmd_DS_MEASUREMENT_REQUEST * pMeasReq)
{
    PRINTM(INFO, "Meas: Req: ------------------------------\n");

    PRINTM(INFO, "Meas: Req: macAddr: %02x:%02x:%02x:%02x:%02x:%02x\n",
           pMeasReq->macAddr[0],
           pMeasReq->macAddr[1],
           pMeasReq->macAddr[2],
           pMeasReq->macAddr[3], pMeasReq->macAddr[4], pMeasReq->macAddr[5]);

    PRINTM(INFO, "Meas: Req:  dlgTkn: %d\n", pMeasReq->dialogToken);
    PRINTM(INFO, "Meas: Req:    mode: dm[%c] rpt[%c] req[%c]\n",
           pMeasReq->reqMode.durationMandatory ? 'X' : ' ',
           pMeasReq->reqMode.report ? 'X' : ' ',
           pMeasReq->reqMode.request ? 'X' : ' ');
    PRINTM(INFO, "Meas: Req:        : en[%c] par[%c]\n",
           pMeasReq->reqMode.enable ? 'X' : ' ',
           pMeasReq->reqMode.parallel ? 'X' : ' ');
    PRINTM(INFO, "Meas: Req: measTyp: %s\n",
           wlan_meas_get_meas_type_str(pMeasReq->measType));

    switch (pMeasReq->measType) {
    case WLAN_MEAS_BASIC:
#if WLAN_MEAS_EXTRA_11H_TYPES
        /* Not supported in firmware */
    case WLAN_MEAS_CCA:
    case WLAN_MEAS_RPI:
#endif
        /* Lazy cheat, fields of bas, cca, rpi union match on the request */
        PRINTM(INFO, "Meas: Req: chan: %u\n", pMeasReq->req.basic.channel);
        PRINTM(INFO, "Meas: Req: strt: %llu\n",
               wlan_le64_to_cpu(pMeasReq->req.basic.startTime));
        PRINTM(INFO, "Meas: Req:  dur: %u\n",
               wlan_le16_to_cpu(pMeasReq->req.basic.duration));
        break;
    default:
        PRINTM(INFO, "Meas: Req: <unhandled>\n");
        break;
    }

    PRINTM(INFO, "Meas: Req: ------------------------------\n");
}

/**
*** wlan_meas_dump_meas_rpt
**
**  @brief Debug print display of the input measurement report
**
**  @param pMeasRpt  Pointer to measurement report to display
**
**  @return          void
*/
static void
wlan_meas_dump_meas_rpt(const HostCmd_DS_MEASUREMENT_REPORT * pMeasRpt)
{
    PRINTM(INFO, "Meas: Rpt: ------------------------------\n");
    PRINTM(INFO, "Meas: Rpt: macAddr: %02x:%02x:%02x:%02x:%02x:%02x\n",
           pMeasRpt->macAddr[0],
           pMeasRpt->macAddr[1],
           pMeasRpt->macAddr[2],
           pMeasRpt->macAddr[3], pMeasRpt->macAddr[4], pMeasRpt->macAddr[5]);

    PRINTM(INFO, "Meas: Rpt:  dlgTkn: %d\n", pMeasRpt->dialogToken);

    PRINTM(INFO, "Meas: Rpt: rptMode: (%x): Rfs[%c] ICp[%c] Lt[%c]\n",
           *(u8 *) & pMeasRpt->rptMode,
           pMeasRpt->rptMode.refused ? 'X' : ' ',
           pMeasRpt->rptMode.incapable ? 'X' : ' ',
           pMeasRpt->rptMode.late ? 'X' : ' ');

    PRINTM(INFO, "Meas: Rpt: measTyp: %s\n",
           wlan_meas_get_meas_type_str(pMeasRpt->measType));

    switch (pMeasRpt->measType) {
    case WLAN_MEAS_BASIC:
        PRINTM(INFO, "Meas: Rpt: chan: %u\n", pMeasRpt->rpt.basic.channel);
        PRINTM(INFO, "Meas: Rpt: strt: %llu\n",
               wlan_le64_to_cpu(pMeasRpt->rpt.basic.startTime));
        PRINTM(INFO, "Meas: Rpt:  dur: %u\n",
               wlan_le16_to_cpu(pMeasRpt->rpt.basic.duration));
        PRINTM(INFO, "Meas: Rpt:  bas: (%x): unmsd[%c], radar[%c]\n",
               *(u8 *) & (pMeasRpt->rpt.basic.map),
               pMeasRpt->rpt.basic.map.unmeasured ? 'X' : ' ',
               pMeasRpt->rpt.basic.map.radar ? 'X' : ' ');
        PRINTM(INFO, "Meas: Rpt:  bas: unidSig[%c] ofdm[%c] bss[%c]\n",
               pMeasRpt->rpt.basic.map.unidentifiedSig ? 'X' : ' ',
               pMeasRpt->rpt.basic.map.OFDM_Preamble ? 'X' : ' ',
               pMeasRpt->rpt.basic.map.BSS ? 'X' : ' ');
        break;
#if WLAN_MEAS_EXTRA_11H_TYPES
        /* Not supported in firmware */
    case WLAN_MEAS_CCA:
        PRINTM(INFO, "Meas: Rpt: chan: %u\n", pMeasRpt->rpt.cca.channel);
        PRINTM(INFO, "Meas: Rpt: strt: %llu\n",
               wlan_le64_to_cpu(pMeasRpt->rpt.cca.startTime));
        PRINTM(INFO, "Meas: Rpt:  dur: %u\n",
               wlan_le16_to_cpu(pMeasRpt->rpt.cca.duration));
        PRINTM(INFO, "Meas: Rpt:  cca: busy fraction = %u\n",
               pMeasRpt->rpt.cca.busyFraction);
        break;
    case WLAN_MEAS_RPI:
        PRINTM(INFO, "Meas: Rpt: chan: %u\n", pMeasRpt->rpt.rpi.channel);
        PRINTM(INFO, "Meas: Rpt: strt: %llu\n",
               wlan_le64_to_cpu(pMeasRpt->rpt.rpi.startTime));
        PRINTM(INFO, "Meas: Rpt:  dur: %u\n",
               wlan_le16_to_cpu(pMeasRpt->rpt.rpi.duration));
        PRINTM(INFO, "Meas: Rpt:  rpi: 0:%u, 1:%u, 2:%u, 3:%u\n",
               pMeasRpt->rpt.rpi.density[0],
               pMeasRpt->rpt.rpi.density[1],
               pMeasRpt->rpt.rpi.density[2], pMeasRpt->rpt.rpi.density[3]);
        PRINTM(INFO, "Meas: Rpt:  rpi: 4:%u, 5:%u, 6:%u, 7:%u\n",
               pMeasRpt->rpt.rpi.density[4],
               pMeasRpt->rpt.rpi.density[5],
               pMeasRpt->rpt.rpi.density[6], pMeasRpt->rpt.rpi.density[7]);
        break;
#endif
#if WLAN_MEAS_EXTRA_11K_TYPES
        /* Future 11k measurement additions */
    case WLAN_MEAS_CHAN_LOAD:
    case WLAN_MEAS_NOISE_HIST:
    case WLAN_MEAS_BEACON:
    case WLAN_MEAS_FRAME:
    case WLAN_MEAS_HIDDEN_NODE:
    case WLAN_MEAS_MEDIUM_SENS_HIST:
    case WLAN_MEAS_STA_STATS:
#endif
    default:
        PRINTM(INFO, "Meas: Rpt: <unhandled>\n");
        break;
    }

    PRINTM(INFO, "Meas: Rpt: ------------------------------\n");
}

/**
*** wlan_meas_cmdresp_get_report
**
**  @brief Retrieve a measurement report from the firmware
** 
**  Callback from command processing when a measurement report is received
**    from the firmware.  Perform the following when a report is received:
**
**   -# Debug displays the report if compiled with the appropriate flags
**   -# If we are pending on a specific measurement report token, and it 
**      matches the received report's token, store the report and wake up
**      any pending threads
**
**  @param priv Private driver information structure
**  @param resp HostCmd_DS_COMMAND struct returned from the firmware command
**              passing a HostCmd_DS_MEASUREMENT_REPORT structure.    
**
**  @return     WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
*/
static int
wlan_meas_cmdresp_get_report(wlan_private * priv,
                             const HostCmd_DS_COMMAND * resp)
{
    wlan_adapter *Adapter = priv->adapter;
    const HostCmd_DS_MEASUREMENT_REPORT *pMeasRpt = &resp->params.meas_rpt;

    ENTER();

    PRINTM(INFO, "Meas: Rpt: %#x-%u, Seq=%u, Ret=%u\n",
           resp->Command, resp->Size, resp->SeqNum, resp->Result);

    /* 
     ** Debug displays the measurement report
     */
    wlan_meas_dump_meas_rpt(pMeasRpt);

    /* 
     ** Check if we are pending on a measurement report and it matches 
     **  the dialog token of the received report:
     */
    if (Adapter->stateMeas.meas_rpt_pend_on
        && Adapter->stateMeas.meas_rpt_pend_on == pMeasRpt->dialogToken) {
        PRINTM(INFO, "Meas: Rpt: RCV'd Pend on meas #%d\n",
               Adapter->stateMeas.meas_rpt_pend_on);

        /* 
         ** Clear the pending report indicator
         */
        Adapter->stateMeas.meas_rpt_pend_on = 0;

        /* 
         ** Copy the received report into the measurement state for retrieval
         */
        memcpy(&Adapter->stateMeas.meas_rpt_returned, pMeasRpt,
               sizeof(Adapter->stateMeas.meas_rpt_returned));

        /* 
         ** Wake up any threads pending on the wait queue
         */
        wake_up_interruptible(&Adapter->stateMeas.meas_rpt_waitQ);
    }

    LEAVE();

    return 0;
}

/**
*** wlan_meas_cmd_request
**
**  @brief Prepare CMD_MEASURMENT_REPORT firmware command
**
**  @param priv     Private driver information structure
**  @param pCmdPtr  Output parameter: Pointer to the command being prepared 
**                  for the firmware
**  @param pInfoBuf HostCmd_DS_MEASUREMENT_REQUEST passed as void data block
**
**  @return         WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
*/
static int
wlan_meas_cmd_request(wlan_private * priv,
                      HostCmd_DS_COMMAND * pCmdPtr, const void *pInfoBuf)
{
    const HostCmd_DS_MEASUREMENT_REQUEST *pMeasReq = pInfoBuf;

    ENTER();

    pCmdPtr->Command = HostCmd_CMD_MEASUREMENT_REQUEST;
    pCmdPtr->Size = sizeof(HostCmd_DS_MEASUREMENT_REQUEST) + S_DS_GEN;

    memcpy(&pCmdPtr->params.meas_req, pMeasReq,
           sizeof(pCmdPtr->params.meas_req));

    PRINTM(INFO, "Meas: Req: %#x-%u, Seq=%u, Ret=%u\n",
           pCmdPtr->Command, pCmdPtr->Size, pCmdPtr->SeqNum, pCmdPtr->Result);

    wlan_meas_dump_meas_req(pMeasReq);

    LEAVE();

    return WLAN_STATUS_SUCCESS;
}

/**
*** wlan_meas_cmd_get_report
**
**  @brief  Retrieve a measurement report from the firmware
**
**  The firmware will send a EVENT_MEAS_REPORT_RDY event when it 
**    completes or receives a measurement report.  The event response
**    handler will then start a HostCmd_CMD_MEASUREMENT_REPORT firmware command
**    which gets completed for transmission to the firmware in this routine.
**
**  @param priv    Private driver information structure
**  @param pCmdPtr Output parameter: Pointer to the command being prepared 
**                 for the firmware
**
**  @return        WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
*/
static int
wlan_meas_cmd_get_report(wlan_private * priv, HostCmd_DS_COMMAND * pCmdPtr)
{
    wlan_adapter *Adapter = priv->adapter;

    ENTER();

    pCmdPtr->Command = HostCmd_CMD_MEASUREMENT_REPORT;
    pCmdPtr->Size = sizeof(HostCmd_DS_MEASUREMENT_REPORT) + S_DS_GEN;

    memset(&pCmdPtr->params.meas_rpt, 0x00, sizeof(pCmdPtr->params.meas_rpt));

    /* 
     ** Set the meas_rpt.macAddr to our mac address to get a meas report,
     **   setting the mac to another STA address instructs the firmware
     **   to transmit this measurement report frame instead
     */
    memcpy(pCmdPtr->params.meas_rpt.macAddr, Adapter->CurrentAddr,
           sizeof(pCmdPtr->params.meas_rpt.macAddr));

    LEAVE();

    return WLAN_STATUS_SUCCESS;
}

/**
*** wlan_meas_init
**
**  @brief Initialize any needed structures for the measurement code
**
**  @param priv  Private driver information structure
**
**  @return      void
*/
void
wlan_meas_init(wlan_private * priv)
{
    ENTER();
    init_waitqueue_head(&priv->adapter->stateMeas.meas_rpt_waitQ);
    LEAVE();
}

/** 
*** wlan_meas_util_send_req
** 
**  @brief Send the input measurement request to the firmware.
**
**  If the dialog token in the measurement request is set to 0, the function
**    will use an local static auto-incremented token in the measurement
**    request.  This ensures the dialog token is always set.
**
**  If wait_for_resp_timeout is set, the function will block its return on 
**     a timeout or returned measurement report that matches the requests
**     dialog token. 
**
**  @param priv                  Private driver information structure
**  @param pMeasReq              Pointer to the measurement request to send
**  @param wait_for_resp_timeout Timeout value of the measurement request
**                               in ms.
**  @param pMeasRpt              Output parameter: Pointer for the resulting
**                               measurement report
**
**  @return
**    - 0 for success
**    - -ETIMEDOUT if the measurement report does not return before
**      the timeout expires
**    - Error return from wlan_prepare_cmd routine otherwise
*/
int
wlan_meas_util_send_req(wlan_private * priv,
                        HostCmd_DS_MEASUREMENT_REQUEST * pMeasReq,
                        int wait_for_resp_timeout,
                        HostCmd_DS_MEASUREMENT_REPORT * pMeasRpt)
{
    static u8 autoDialogTok = 0;
    wlan_meas_state_t *pMeasState = &priv->adapter->stateMeas;
    int ret;
    uint calcTimeout;

    ENTER();

    /* 
     **  If dialogTok was set to 0 or not provided, autoset
     */
    pMeasReq->dialogToken = (pMeasReq->dialogToken ?
                             pMeasReq->dialogToken : ++autoDialogTok);

    /* 
     ** Check for rollover of the dialog token.  Avoid using 0 as a token.
     */
    pMeasReq->dialogToken = (pMeasReq->dialogToken ? pMeasReq->dialogToken : 1);

    /* 
     ** If the request is to pend waiting for the result, set the dialog token
     **   of this measurement request in the state structure.  The measurement
     **   report handling routines can then check the incoming measurement
     **   reports for a match with this dialog token.  
     */
    if (wait_for_resp_timeout) {
        pMeasState->meas_rpt_pend_on = pMeasReq->dialogToken;
        PRINTM(INFO, "Meas: Req: START Pend on meas #%d\n",
               pMeasReq->dialogToken);
    }

    /* 
     ** Send the measurement request to the firmware
     */
    ret = wlan_prepare_cmd(priv, HostCmd_CMD_MEASUREMENT_REQUEST,
                           HostCmd_ACT_GEN_SET,
                           HostCmd_OPTION_WAITFORRSP, 0, (void *) pMeasReq);

    /* 
     ** If the measurement request was sent successfully, and the function
     **   must wait for the report, suspend execution until the meas_rpt_pend_on
     **   variable in the state structure has been reset to 0 by the report
     **   handling routines.
     */
    if (ret == 0 && wait_for_resp_timeout) {
        /* Add ~25% overhead to the timeout for firmware overhead */
        calcTimeout = wait_for_resp_timeout + (wait_for_resp_timeout >> 2);

        PRINTM(INFO, "Meas: Req: TIMEOUT set to %d ms\n", calcTimeout);

        /* extra 10 ms for the driver overhead - helps with small meas */
        calcTimeout += 10;

        PRINTM(INFO, "Meas: Req: TIMEOUT set to %d milliseconds\n",
               calcTimeout);

        os_wait_interruptible_timeout(pMeasState->meas_rpt_waitQ,
                                      pMeasState->meas_rpt_pend_on == 0,
                                      calcTimeout);

        if (pMeasState->meas_rpt_pend_on) {
            PRINTM(INFO, "Meas: Req: TIMEOUT Pend on meas #%d\n",
                   pMeasReq->dialogToken);
            ret = -ETIMEDOUT;
        } else {
            PRINTM(INFO, "Meas: Req: DONE Pend on meas #%d\n",
                   pMeasReq->dialogToken);
            memcpy(pMeasRpt, &pMeasState->meas_rpt_returned,
                   sizeof(HostCmd_DS_MEASUREMENT_REPORT));
        }
    }

    /* 
     ** The measurement request failed or we are not waiting for a response.
     **   In either case, the rpt_pend_on variable should be zero.
     */
    pMeasState->meas_rpt_pend_on = 0;

    LEAVE();

    return ret;
}

/**
*** wlan_meas_cmd_process
**
**  @brief  Prepare the HostCmd_DS_Command structure for a measurement command.
**
**  Use the Command field to determine if the command being set up is for
**     11h and call one of the local command handlers accordingly for:
**
**        - HostCmd_CMD_MEASUREMENT_REQUEST
**        - HostCmd_CMD_MEASUREMENT_REPORT
**
**  @param priv     Private driver information structure
**  @param pCmdPtr  Output parameter: Pointer to the command being prepared 
**                  for the firmware
**  @param pInfoBuf Void buffer passthrough with data necessary for a
**                  specific command type
**
**  @return         WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
**
**  @sa wlan_meas_cmd_request
**  @sa wlan_meas_cmd_get_report
*/
int
wlan_meas_cmd_process(wlan_private * priv,
                      HostCmd_DS_COMMAND * pCmdPtr, const void *pInfoBuf)
{
    int ret = WLAN_STATUS_SUCCESS;

    ENTER();
    switch (pCmdPtr->Command) {
    case HostCmd_CMD_MEASUREMENT_REQUEST:
        ret = wlan_meas_cmd_request(priv, pCmdPtr, pInfoBuf);
        break;
    case HostCmd_CMD_MEASUREMENT_REPORT:
        ret = wlan_meas_cmd_get_report(priv, pCmdPtr);
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
*** wlan_meas_cmdresp_process
**
**  @brief Handle the command response from the firmware for a measurement 
**         command
**
**  Use the Command field to determine if the command response being
**    is for meas.  Call the local command response handler accordingly for:
**
**        - HostCmd_CMD_802_MEASUREMENT_REQUEST
**        - HostCmd_CMD_802_MEASUREMENT_REPORT
**
**  @param priv Private driver information structure
**  @param resp HostCmd_DS_COMMAND struct returned from the firmware command
**
**  @return     WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
*/
int
wlan_meas_cmdresp_process(wlan_private * priv, const HostCmd_DS_COMMAND * resp)
{
    int ret = WLAN_STATUS_SUCCESS;

    ENTER();
    switch (resp->Command) {
    case HostCmd_CMD_MEASUREMENT_REQUEST:
        PRINTM(INFO, "Meas: Req Resp: Sz=%u, Seq=%u, Ret=%u\n",
               resp->Size, resp->SeqNum, resp->Result);

        break;
    case HostCmd_CMD_MEASUREMENT_REPORT:
        ret = wlan_meas_cmdresp_get_report(priv, resp);
        break;
    default:
        ret = WLAN_STATUS_FAILURE;
    }

    LEAVE();
    return ret;
}

/**
*** wlan_meas_ioctl_send_req
**
**  @brief Send a measurement request to the firmware from the application
**
**  Process an IOCTL from the application for a measurement request.  The
**    application thread will be blocked while the measurement completes or
**    times out.
**
**  If the measurement is a success, the measurement report will be copied
**    back to the user space.
**
**  @param priv Private driver information structure
**  @param wrq  Input/Output parameter: OS IOCTL passed structure containing:
**                 - HostCmd_DS_MEASUREMENT_REQUEST on input
**                 - HostCmd_DS_MEASUREMENT_REQUEST on output 
**
**  @return
**    - 0 for success
**    - -EFAULT if memory copy from user space fails
**    - Error return from wlan_meas_util_send_req routine otherwise
*/
int
wlan_meas_ioctl_send_req(wlan_private * priv, struct iwreq *wrq)
{
    wlan_adapter *Adapter = priv->adapter;
    HostCmd_DS_MEASUREMENT_REQUEST measReq;
    HostCmd_DS_MEASUREMENT_REPORT measRpt;
    int wait_for_resp_timeout = 0;
    int ret = 0;
    u8 zeroMac[] = { 0, 0, 0, 0, 0, 0 };

    ENTER();

    memset(&measReq, 0x00, sizeof(measReq));
    memset(&measRpt, 0x00, sizeof(measRpt));

    if (copy_from_user(&measReq, wrq->u.data.pointer, sizeof(measReq)) != 0) {
        /* copy_from_user failed */
        PRINTM(INFO, "Meas: ioctl_send_req: copy from user failed\n");
        LEAVE();
        return -EFAULT;
    }

    /* 
     ** If measDuration was set to 0, set to default
     */
    if (measReq.req.basic.duration == 0)
        measReq.req.basic.duration = WLAN_MEAS_DEFAULT_MEAS_DURATION;
    measReq.req.basic.duration = wlan_cpu_to_le16(measReq.req.basic.duration);

    /* 
     ** Provide a timeout time for the measurement based on the duration.
     */
    wait_for_resp_timeout = measReq.req.basic.duration;

    /* 
     ** If the measChannel was set to 0 or not provided, re-sync the 
     **   Channel field in Adapter struct for use in the request
     */
    if (measReq.req.basic.channel == 0) {
        wlan_prepare_cmd(priv, HostCmd_CMD_802_11_RF_CHANNEL,
                         HostCmd_OPT_802_11_RF_CHANNEL_GET,
                         HostCmd_OPTION_WAITFORRSP, 0, NULL);

        measReq.req.basic.channel = Adapter->CurBssParams.BSSDescriptor.Channel;
    }

    if (memcmp(measReq.macAddr, zeroMac, sizeof(zeroMac)) == 0)
        /* 
         ** Set the measReq.macAddr to our mac address if it was not provided
         */
        memcpy(measReq.macAddr, Adapter->CurrentAddr, sizeof(measReq.macAddr));

    /* 
     ** Finish setup of the measurement request and send to the firmware
     */
    ret = wlan_meas_util_send_req(priv, &measReq,
                                  wait_for_resp_timeout, &measRpt);

    if (ret == 0) {
        /* 
         ** The request was sent successfully and the report received and stored
         **   in the measurement state before execution returned from
         **   the wlan_send_meas_req function.
         */
        wrq->u.data.length = sizeof(measRpt);
        if (copy_to_user(wrq->u.data.pointer, &measRpt, wrq->u.data.length)) {
            PRINTM(INFO, "Copy to user failed\n");
            LEAVE();
            return -EFAULT;
        }
    }

    LEAVE();

    return ret;
}
