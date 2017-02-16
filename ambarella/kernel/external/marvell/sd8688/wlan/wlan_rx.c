/** @file wlan_rx.c
  * @brief This file contains the handling of RX in wlan
  * driver.
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
	09/28/05: Add Doxygen format comments
	12/09/05: Add Sliding window SNR/NF Average Calculation support
	
********************************************************/

#include	"wlan_headers.h"

/********************************************************
		Local Variables
********************************************************/

typedef struct
{
    /** Ethernet header destination address */
    u8 dest_addr[ETH_ALEN];
    /** Ethernet header source address */
    u8 src_addr[ETH_ALEN];
    /** Ethernet header length */
    u16 h803_len;

} __ATTRIB_PACK__ Eth803Hdr_t;

typedef struct
{
    /** LLC DSAP */
    u8 llc_dsap;
    /** LLC SSAP */
    u8 llc_ssap;
    /** LLC CTRL */
    u8 llc_ctrl;
    /** SNAP OUI */
    u8 snap_oui[3];
    /** SNAP type */
    u16 snap_type;

} __ATTRIB_PACK__ Rfc1042Hdr_t;

typedef struct
{
    /** Etherner header */
    Eth803Hdr_t eth803_hdr;
    /** RFC 1042 header */
    Rfc1042Hdr_t rfc1042_hdr;

} __ATTRIB_PACK__ RxPacketHdr_t;

typedef struct
{
    /** Ethernet II header destination address */
    u8 dest_addr[ETH_ALEN];
    /** Ethernet II header source address */
    u8 src_addr[ETH_ALEN];
    /** Ethernet II header length */
    u16 ethertype;

} __ATTRIB_PACK__ EthII_Hdr_t;

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
 *  @brief This function processes received packet and forwards it
 *  to kernel/upper layer
 *  
 *  @param priv    A pointer to wlan_private
 *  @param skb     A pointer to skb which includes the received packet
 *  @return 	   WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
int
wlan_process_rx_packet(wlan_private * priv, struct sk_buff *skb)
{
    int ret = WLAN_STATUS_SUCCESS;

    RxPacketHdr_t *pRxPkt;
    RxPD *pRxPD;

    int hdrChop;
    EthII_Hdr_t *pEthHdr;
    u32 u32SkbLen = skb->len;

    const u8 rfc1042_eth_hdr[] = { 0xaa, 0xaa, 0x03, 0x00, 0x00, 0x00 };

    ENTER();

    pRxPD = (RxPD *) skb->data;
    endian_convert_RxPD(pRxPD);

    pRxPkt = (RxPacketHdr_t *) ((u8 *) pRxPD + pRxPD->RxPktOffset);

    DBG_HEXDUMP(DAT_D, "Rx", skb->data, MIN(skb->len, MAX_DATA_DUMP_LEN));

#define RX_PKT_TYPE_DEBUG  0xEF
#define DBG_TYPE_SMALL  2
#define SIZE_OF_DBG_STRUCT 4
    if (pRxPD->RxPktType == RX_PKT_TYPE_DEBUG) {
        u8 dbgType;
        dbgType = *(u8 *) & pRxPkt->eth803_hdr;
        if (dbgType == DBG_TYPE_SMALL) {
            PRINTM(FW_D, "\n");
            DBG_HEXDUMP(FW_D, "FWDBG",
                        (char *) ((u8 *) & pRxPkt->eth803_hdr +
                                  SIZE_OF_DBG_STRUCT), pRxPD->RxPktLength);
            PRINTM(FW_D, "FWDBG::\n");
        }
        kfree_skb(skb);
        goto done;
    }

    PRINTM(INFO, "RX Data: skb->len - pRxPD->RxPktOffset = %d - %d = %d\n",
           skb->len, pRxPD->RxPktOffset, skb->len - pRxPD->RxPktOffset);

    HEXDUMP("RX Data: Dest", pRxPkt->eth803_hdr.dest_addr,
            sizeof(pRxPkt->eth803_hdr.dest_addr));
    HEXDUMP("RX Data: Src", pRxPkt->eth803_hdr.src_addr,
            sizeof(pRxPkt->eth803_hdr.src_addr));

    if (memcmp(&pRxPkt->rfc1042_hdr,
               rfc1042_eth_hdr, sizeof(rfc1042_eth_hdr)) == 0) {
        /* 
         *  Replace the 803 header and rfc1042 header (llc/snap) with an 
         *    EthernetII header, keep the src/dst and snap_type (ethertype)

         *  The firmware only passes up SNAP frames converting
         *    all RX Data from 802.11 to 802.2/LLC/SNAP frames.

         *  To create the Ethernet II, just move the src, dst address right
         *    before the snap_type.
         */
        pEthHdr = (EthII_Hdr_t *)
            ((u8 *) & pRxPkt->eth803_hdr
             + sizeof(pRxPkt->eth803_hdr) + sizeof(pRxPkt->rfc1042_hdr)
             - sizeof(pRxPkt->eth803_hdr.dest_addr)
             - sizeof(pRxPkt->eth803_hdr.src_addr)
             - sizeof(pRxPkt->rfc1042_hdr.snap_type));

        memcpy(pEthHdr->src_addr, pRxPkt->eth803_hdr.src_addr,
               sizeof(pEthHdr->src_addr));
        memcpy(pEthHdr->dest_addr, pRxPkt->eth803_hdr.dest_addr,
               sizeof(pEthHdr->dest_addr));

        /* Chop off the RxPD + the excess memory from the 802.2/llc/snap header 
           that was removed */
        hdrChop = (u8 *) pEthHdr - (u8 *) pRxPD;
    } else {
        HEXDUMP("RX Data: LLC/SNAP",
                (u8 *) & pRxPkt->rfc1042_hdr, sizeof(pRxPkt->rfc1042_hdr));

        /* Chop off the RxPD */
        hdrChop = (u8 *) & pRxPkt->eth803_hdr - (u8 *) pRxPD;
    }

    /* Chop off the leading header bytes so the skb points to the start of
       either the reconstructed EthII frame or the 802.2/llc/snap frame */
    skb_pull(skb, hdrChop);

    u32SkbLen = skb->len;
    priv->adapter->RxPDRate = pRxPD->RxRate;

    if (os_upload_rx_packet(priv, skb)) {
        PRINTM(ERROR, "RX Error: os_upload_rx_packet" " returns failure\n");
        ret = WLAN_STATUS_FAILURE;
        goto done;
    }
    priv->stats.rx_bytes += u32SkbLen;
    priv->stats.rx_packets++;

    PRINTM(DATA, "Data => kernel @ %lu\n", os_time_get());
    ret = WLAN_STATUS_SUCCESS;
  done:
    LEAVE();

    return (ret);
}
