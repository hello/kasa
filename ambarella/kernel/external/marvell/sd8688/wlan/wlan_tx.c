/** @file wlan_tx.c
  * @brief This file contains the handling of TX in wlan
  * driver.
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
/********************************************************
Change log:
	09/28/05: Add Doxygen format comments
	12/13/05: Add Proprietary periodic sleep support
	01/05/06: Add kernel 2.6.x support	
	04/06/06: Add TSPEC, queue metrics, and MSDU expiry support
********************************************************/

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

/********************************************************
		Global functions
********************************************************/

/** 
 *  @brief This function checks the conditions and sends packet to device 
 *  
 *  @param priv    A pointer to wlan_private structure
 *  @param skb 	   A pointer to the skb for process
 *  @return 	   n/a
 */
void
wlan_process_tx(wlan_private * priv, struct sk_buff *skb)
{
    wlan_adapter *Adapter = priv->adapter;
    int ret = WLAN_STATUS_SUCCESS;
    TxPD *pLocalTxPD;
    u8 *headptr;
    struct sk_buff *newskb;
    int headlen;
    unsigned long driver_flags;
    OS_INTERRUPT_SAVE_AREA;
    ENTER();

    ASSERT(skb);
    if (!skb->len || (skb->len > MRVDRV_ETH_TX_PACKET_BUFFER_SIZE)) {
        PRINTM(ERROR, "Tx Error: Bad skb length %d : %d\n",
               skb->len, MRVDRV_ETH_TX_PACKET_BUFFER_SIZE);
        ret = WLAN_STATUS_FAILURE;
        goto done;
    }
    headlen = sizeof(TxPD) + SDIO_HEADER_LEN;

    if (skb_headroom(skb) < (headlen + HEADER_ALIGNMENT)) {
        PRINTM(WARN, "Tx: Insufficient skb headroom %d\n", skb_headroom(skb));
        /* Insufficient skb headroom - allocate a new skb */
        newskb = skb_realloc_headroom(skb, headlen + HEADER_ALIGNMENT);
        if (unlikely(newskb == NULL)) {
            PRINTM(ERROR, "Tx: Cannot allocate skb\n");
            ret = WLAN_STATUS_FAILURE;
            goto done;
        }
        kfree_skb(skb);
        skb = newskb;
        PRINTM(INFO, "new skb headroom %d\n", skb_headroom(skb));
    }
    /* headptr should be 8-byte aligned */
    headptr = skb->data - headlen;
    headptr = (u8 *) ((u32) headptr & ~((u32) (HEADER_ALIGNMENT - 1)));
    pLocalTxPD = (TxPD *) (headptr + SDIO_HEADER_LEN);

    memset(pLocalTxPD, 0, sizeof(TxPD));

    pLocalTxPD->TxPktLength = skb->len;

    /* 
     * original skb->priority has been overwritten 
     * by wmm_map_and_add_skb()
     */
    pLocalTxPD->Priority = (u8) skb->priority;
    pLocalTxPD->PktDelay_2ms = wmm_compute_driver_packet_delay(priv, skb);

    if (pLocalTxPD->Priority < NELEMENTS(Adapter->wmm.userPriPktTxCtrl))
        /* 
         * Set the priority specific TxControl field, setting of 0 will
         *   cause the default value to be used later in this function
         */
        pLocalTxPD->TxControl
            = Adapter->wmm.userPriPktTxCtrl[pLocalTxPD->Priority];

    if (Adapter->PSState != PS_STATE_FULL_POWER) {
        if (TRUE == wlan_check_last_packet_indication(priv)) {
            Adapter->TxLockFlag = TRUE;
            pLocalTxPD->Flags = MRVDRV_TxPD_POWER_MGMT_LAST_PACKET;
        }
    }

    /* offset of actual data */
    pLocalTxPD->TxPktOffset = (long) skb->data - (long) pLocalTxPD;

    if (pLocalTxPD->TxControl == 0)
        /* TxCtrl set by user or default */
        pLocalTxPD->TxControl = Adapter->PktTxCtrl;

    endian_convert_TxPD(pLocalTxPD);
    memcpy((u8 *) pLocalTxPD->TxDestAddr, skb->data, ETH_ALEN);

    ret = sbi_host_to_card(priv, MV_TYPE_DAT, headptr,
                           skb->len + ((long) skb->data - (long) pLocalTxPD));
    if (ret) {
        PRINTM(ERROR,
               "wlan_process_tx Error: sbi_host_to_card failed: 0x%X\n", ret);
        Adapter->dbg.num_tx_host_to_card_failure++;
        goto done;
    }

    PRINTM(DATA, "Data => FW @ %lu\n", os_time_get());
    DBG_HEXDUMP(DAT_D, "Tx", headptr,
                MIN(skb->len + sizeof(TxPD), MAX_DATA_DUMP_LEN));

    wmm_process_fw_iface_tx_xfer_start(priv);

  done:
    if (!ret) {
        priv->stats.tx_packets++;
        priv->stats.tx_bytes += skb->len;
    } else {
        priv->stats.tx_dropped++;
        priv->stats.tx_errors++;
    }

    /* Freed skb */
    kfree_skb(skb);
    OS_INT_DISABLE(priv, driver_flags);
    priv->adapter->HisRegCpy &= ~HIS_TxDnLdRdy;
    OS_INT_RESTORE(priv, driver_flags);
    LEAVE();
    return;
}

/** 
 *  @brief This function queues the packet received from
 *  kernel/upper layer and wake up the main thread to handle it.
 *  
 *  @param priv    A pointer to wlan_private structure
  * @param skb     A pointer to skb which includes TX packet
 *  @return 	   WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
int
wlan_tx_packet(wlan_private * priv, struct sk_buff *skb)
{
    ulong flags;
    wlan_adapter *Adapter = priv->adapter;
    int ret = WLAN_STATUS_SUCCESS;

    ENTER();

    HEXDUMP("TX Data", skb->data, MIN(skb->len, 100));

    spin_lock_irqsave(&Adapter->CurrentTxLock, flags);

    wmm_map_and_add_skb(priv, skb);
    wake_up_interruptible(&priv->MainThread.waitQ);
    spin_unlock_irqrestore(&Adapter->CurrentTxLock, flags);

    LEAVE();

    return ret;
}

/** 
 *  @brief This function tells firmware to send a NULL data packet.
 *  
 *  @param priv     A pointer to wlan_private structure
 *  @param flags    Trasmit Pkt Flags
 *  @return 	    n/a
 */
int
wlan_send_null_packet(wlan_private * priv, u8 flags)
{
    wlan_adapter *Adapter = priv->adapter;
    TxPD *ptxpd;
/* sizeof(TxPD) + Interface specific header */
#define NULL_PACKET_HDR 64
    u8 tmpbuf[NULL_PACKET_HDR] = { 0 };
    u8 *ptr = tmpbuf;
    int ret = WLAN_STATUS_SUCCESS;

    ENTER();

    if (Adapter->SurpriseRemoved == TRUE) {
        ret = WLAN_STATUS_FAILURE;
        goto done;
    }

    if (Adapter->MediaConnectStatus == WlanMediaStateDisconnected) {
        ret = WLAN_STATUS_FAILURE;
        goto done;
    }

    ptr += SDIO_HEADER_LEN;
    ptxpd = (TxPD *) ptr;

    ptxpd->TxControl = Adapter->PktTxCtrl;
    ptxpd->Flags = flags;
    ptxpd->Priority = WMM_HIGHEST_PRIORITY;
    ptxpd->TxPktOffset = sizeof(TxPD);

    endian_convert_TxPD(ptxpd);

    ret = sbi_host_to_card(priv, MV_TYPE_DAT, tmpbuf, sizeof(TxPD));

    if (ret != 0) {
        PRINTM(ERROR, "TX Error: wlan_send_null_packet failed!\n");
        Adapter->dbg.num_tx_host_to_card_failure++;
        goto done;
    }
    PRINTM(DATA, "Null data => FW\n");
    DBG_HEXDUMP(DAT_D, "Tx", ptr, sizeof(TxPD));

  done:
    LEAVE();
    return ret;
}

/** 
 *  @brief This function check if we need send last packet indication.
 *  
 *  @param priv     A pointer to wlan_private structure
 *
 *  @return 	   TRUE or FALSE
 */
BOOLEAN
wlan_check_last_packet_indication(wlan_private * priv)
{
    wlan_adapter *Adapter = priv->adapter;
    BOOLEAN ret = FALSE;
    BOOLEAN prop_ps = TRUE;

    ENTER();

    if (Adapter->sleep_period.period == 0 || Adapter->gen_null_pkg == FALSE) {
        LEAVE();
        return ret;
    }

    if (wmm_lists_empty(priv)) {
        if (((Adapter->CurBssParams.wmm_uapsd_enabled == TRUE)
             && (Adapter->wmm.qosinfo != 0)) || prop_ps)

            ret = TRUE;
    }
    LEAVE();
    return ret;
}
