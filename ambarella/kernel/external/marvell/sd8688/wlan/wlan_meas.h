/**
 *  @file wlan_meas.h
 *
 *  @brief Interface for the measurement module implemented in wlan_meas.c
 *
 *  Driver interface functions and type declarations for the measurement module
 *    implemented in wlan_meas.c
 *  
 *  @sa wlan_meas.c
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

#ifndef _WLAN_MEAS_H
#define _WLAN_MEAS_H

#include  "wlan_fw.h"

#ifdef __KERNEL__               /* state is only used in the driver, exempt
                                   from app code */
/**
*** @brief Driver measurement state held in wlan_priv structure
**  
**  Used to record a measurement request that the driver is pending on 
**    the result (received measurement report).
*/
typedef struct
{
    /**
    *** Dialog token of a pending measurement request/report.  Used to
    ***   block execution while waiting for the specific dialog token
    **/
    u8 meas_rpt_pend_on;

    /**
    *** Measurement report received from the firmware that we were pending on
    **/
    HostCmd_DS_MEASUREMENT_REPORT meas_rpt_returned;

    /**
    *** OS wait queue used to suspend the requesting thread
    **/
    wait_queue_head_t meas_rpt_waitQ;

} wlan_meas_state_t;

/* Initialize the measurement code on startup */
extern void wlan_meas_init(wlan_private * priv);

/* Send a given measurement request to the firmware, report back the result */
extern int
wlan_meas_util_send_req(wlan_private * priv,
                        HostCmd_DS_MEASUREMENT_REQUEST * pMeasReq,
                        int wait_for_resp_timeout,
                        HostCmd_DS_MEASUREMENT_REPORT * pMeasRpt);

/* Setup a measurement command before it is sent to the firmware */
extern int wlan_meas_cmd_process(wlan_private * priv,
                                 HostCmd_DS_COMMAND * pCmdPtr,
                                 const void *pInfoBuf);

/* Handle a given measurement command response from the firmware */
extern int wlan_meas_cmdresp_process(wlan_private * priv,
                                     const HostCmd_DS_COMMAND * resp);

/* Process an application ioctl for sending a measurement request */
extern int wlan_meas_ioctl_send_req(wlan_private * priv, struct iwreq *wrq);

#endif /* WLANCONFIG */

#endif
