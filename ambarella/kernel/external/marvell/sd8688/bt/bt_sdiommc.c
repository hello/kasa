/** @file bt_sdiommc.c
 *  @brief This file contains SDIO IF (interface) module
 *  related functions.
 * 
 * Copyright (C) 2007-2008, Marvell International Ltd.
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

#include "include.h"

/** define marvell vendor id */
#define MARVELL_VENDOR_ID 0x02df

/** Max retry number of CMD53 write */
#define MAX_WRITE_IOMEM_RETRY	2
/** Helper name */
char *helper_name = NULL;
/** Firmware name */
char *fw_name = NULL;
/** Default helper name */
#define DEFAULT_HELPER_NAME "helper_sd.bin"
/** Default firmware name */
#define DEFAULT_FW_NAME "sd8688.bin"

/** Function number 2 */
#define FN2			2
/** SD_CLASS_BT_A*/
#define SD_CLASS_BT_A		0x02
/** SD 8688 BT device ID */
#define SD_DEVICE_ID_8688_BT    0x9105
static const struct sdio_device_id bt_ids[] = {
    {SDIO_DEVICE(MARVELL_VENDOR_ID, SD_DEVICE_ID_8688_BT)},
    {SDIO_DEVICE_CLASS(SD_CLASS_BT_A)},
    {}
};

/********************************************************
		Global Variables
********************************************************/

/** 
 *  @brief This function get rx_unit value
 *  
 *  @param priv    A pointer to bt_private structure
 *  @return 	   BT_STATUS_SUCCESS or BT_STATUS_FAILURE
 */
int
sd_get_rx_unit(bt_private * priv)
{
    int ret = BT_STATUS_SUCCESS;
    u8 reg;
    struct sdio_mmc_card *card = (struct sdio_mmc_card *) priv->bt_dev.card;

    ENTER();

    reg = sdio_readb(card->func, CARD_RX_UNIT_REG, &ret);
    if (ret == BT_STATUS_SUCCESS)
        priv->bt_dev.rx_unit = reg;

    LEAVE();
    return ret;
}

/** 
 *  @brief This function reads fwstatus registers
 *  
 *  @param priv    A pointer to bt_private structure
 *  @param dat	   A pointer to keep returned data
 *  @return 	   BT_STATUS_SUCCESS or BT_STATUS_FAILURE
 */
static int
sd_read_firmware_status(bt_private * priv, u16 * dat)
{
    int ret = BT_STATUS_SUCCESS;
    u8 fws0;
    u8 fws1;
    struct sdio_mmc_card *card = (struct sdio_mmc_card *) priv->bt_dev.card;

    ENTER();

    fws0 = sdio_readb(card->func, CARD_FW_STATUS0_REG, &ret);
    if (ret < 0) {
        LEAVE();
        return BT_STATUS_FAILURE;
    }

    fws1 = sdio_readb(card->func, CARD_FW_STATUS1_REG, &ret);
    if (ret < 0) {
        LEAVE();
        return BT_STATUS_FAILURE;
    }

    *dat = (((u16) fws1) << 8) | fws0;

    LEAVE();
    return BT_STATUS_SUCCESS;
}

/** 
 *  @brief This function reads rx length
 *  
 *  @param priv    A pointer to bt_private structure
 *  @param dat	   A pointer to keep returned data
 *  @return 	   BT_STATUS_SUCCESS or BT_STATUS_FAILURE
 */
static int
sd_read_rx_len(bt_private * priv, u16 * dat)
{
    int ret = BT_STATUS_SUCCESS;
    u8 reg;
    struct sdio_mmc_card *card = (struct sdio_mmc_card *) priv->bt_dev.card;

    ENTER();

    reg = sdio_readb(card->func, CARD_RX_LEN_REG, &ret);
    if (ret == BT_STATUS_SUCCESS)
        *dat = (u16) reg << priv->bt_dev.rx_unit;

    LEAVE();
    return ret;
}

/** 
 *  @brief This function enables the host interrupts mask
 *  
 *  @param priv    A pointer to bt_private structure
 *  @param mask	   the interrupt mask
 *  @return 	   BT_STATUS_SUCCESS
 */
static int
sd_enable_host_int_mask(bt_private * priv, u8 mask)
{
    int ret = BT_STATUS_SUCCESS;
    struct sdio_mmc_card *card = (struct sdio_mmc_card *) priv->bt_dev.card;

    ENTER();

    sdio_writeb(card->func, mask, HOST_INT_MASK_REG, &ret);
    if (ret) {
        PRINTM(WARN, "Unable to enable the host interrupt!\n");
        ret = BT_STATUS_FAILURE;
    }

    LEAVE();
    return ret;
}

/**  @brief This function disables the host interrupts mask.
 *  
 *  @param priv    A pointer to bt_private structure
 *  @param mask	   the interrupt mask
 *  @return 	   BT_STATUS_SUCCESS or BT_STATUS_FAILURE
 */
static int
sd_disable_host_int_mask(bt_private * priv, u8 mask)
{
    int ret = BT_STATUS_FAILURE;
    u8 host_int_mask;
    struct sdio_mmc_card *card = (struct sdio_mmc_card *) priv->bt_dev.card;

    ENTER();

    /* Read back the host_int_mask register */
    host_int_mask = sdio_readb(card->func, HOST_INT_MASK_REG, &ret);
    if (ret)
        goto done;

    /* Update with the mask and write back to the register */
    host_int_mask &= ~mask;
    sdio_writeb(card->func, host_int_mask, HOST_INT_MASK_REG, &ret);
    if (ret < 0) {
        PRINTM(WARN, "Unable to diable the host interrupt!\n");
        goto done;
    }
    ret = BT_STATUS_SUCCESS;
  done:
    LEAVE();
    return ret;
}

/** 
 *  @brief This function polls the card status register.
 *  
 *  @param priv    	A pointer to bt_private structure
 *  @param bits    	the bit mask
 *  @return 	   	BT_STATUS_SUCCESS or BT_STATUS_FAILURE
 */
static int
sd_poll_card_status(bt_private * priv, u8 bits)
{
    int tries;
    int rval;
    struct sdio_mmc_card *card = (struct sdio_mmc_card *) priv->bt_dev.card;
    u8 cs;

    ENTER();

    for (tries = 0; tries < MAX_POLL_TRIES * 1000; tries++) {
        cs = sdio_readb(card->func, CARD_STATUS_REG, &rval);
        if (rval != 0)
            break;
        if (rval == 0 && (cs & bits) == bits) {
            LEAVE();
            return BT_STATUS_SUCCESS;
        }
        udelay(1);
    }
    PRINTM(WARN, "mv_sdio_poll_card_status: FAILED!:%d\n", rval);

    LEAVE();
    return BT_STATUS_FAILURE;
}

/** 
 *  @brief This function probe the card
 *  
 *  @param func    A pointer to sdio_func structure.
 *  @param id	   A pointer to structure sd_device_id	
 *  @return 	   BT_STATUS_SUCCESS or BT_STATUS_FAILURE
 */
static int
sd_probe_card(struct sdio_func *func, const struct sdio_device_id *id)
{
    int ret = BT_STATUS_SUCCESS;
    bt_private *priv = NULL;
    struct sdio_mmc_card *card = NULL;

    ENTER();

    PRINTM(INFO, "vendor=%x,device=%x,class=%d,fn=%d\n", id->vendor, id->device,
           id->class, func->num);
    card = kzalloc(sizeof(struct sdio_mmc_card), GFP_KERNEL);
    if (!card) {
        ret = -ENOMEM;
        goto done;
    }
    card->func = func;
    priv = bt_add_card(card);
    if (!priv) {
        ret = BT_STATUS_FAILURE;
        kfree(card);
    }
  done:
    LEAVE();
    return ret;
}

/** 
 *  @brief This function checks if the firmware is ready to accept
 *  command or not.
 *  
 *  @param priv     A pointer to bt_private structure
 *  @param pollnum  Number of times to polling fw status 
 *  @return         BT_STATUS_SUCCESS or BT_STATUS_FAILURE
 */
int
sd_verify_fw_download(bt_private * priv, int pollnum)
{
    int ret = BT_STATUS_SUCCESS;
    u16 firmwarestat;
    int tries;

    ENTER();

    /* Wait for firmware initialization event */
    for (tries = 0; tries < pollnum; tries++) {
        if (sd_read_firmware_status(priv, &firmwarestat) < 0)
            continue;
        if (firmwarestat == FIRMWARE_READY) {
            ret = BT_STATUS_SUCCESS;
            break;
        } else {
            mdelay(10);
            ret = BT_STATUS_FAILURE;
        }
    }
    if (ret < 0)
        goto done;

    ret = BT_STATUS_SUCCESS;
  done:
    LEAVE();
    return ret;
}

/** 
 *  @brief This function programs the firmware image.
 *  
 *  @param priv    	A pointer to bt_private structure
 *  @return 	   	BT_STATUS_SUCCESS or BT_STATUS_FAILURE
 */
int
sd_dowload_helper(bt_private * priv)
{
    struct sdio_mmc_card *card = (struct sdio_mmc_card *) priv->bt_dev.card;
    const struct firmware *fw_helper = NULL;
    u8 *helper = NULL;
    int helperlen;
    int ret = BT_STATUS_SUCCESS;
    void *tmphlprbuf = NULL;
    int tmphlprbufsz;
    u8 *hlprbuf;
    int hlprblknow;
    u32 tx_len;
#ifdef FW_DOWNLOAD_SPEED
    u32 tv1, tv2;
#endif

    ENTER();

    if ((ret =
         request_firmware(&fw_helper, helper_name, priv->hotplug_device)) < 0) {
        PRINTM(FATAL, "request_firmware() failed (helper), error code = %#x\n",
               ret);
        goto done;
    }

    if (fw_helper) {
        helper = (u8 *)fw_helper->data;
        helperlen = fw_helper->size;
    } else {
        PRINTM(MSG, "No helper image found! Terminating download\n");
        ret = BT_STATUS_FAILURE;
        goto done;
    }

    PRINTM(INFO, "Downloading helper image (%d bytes), block size %d bytes\n",
           helperlen, SD_BLOCK_SIZE_FW_DL);

#ifdef FW_DOWNLOAD_SPEED
    tv1 = get_utimeofday();
#endif
    tmphlprbufsz = BT_UPLD_SIZE;
    tmphlprbuf = kmalloc(tmphlprbufsz, GFP_KERNEL);
    if (!tmphlprbuf) {
        PRINTM(ERROR,
               "Unable to allocate buffer for helper. Terminating download\n");
        ret = BT_STATUS_FAILURE;
        goto done;
    }
    memset(tmphlprbuf, 0, tmphlprbufsz);
    hlprbuf = (u8 *) tmphlprbuf;

    /* Perform helper data transfer */
    tx_len = (FIRMWARE_TRANSFER_NBLOCK * SD_BLOCK_SIZE_FW_DL) - SDIO_HEADER_LEN;
    hlprblknow = 0;
    do {
        /* The host polls for the DN_LD_CARD_RDY and CARD_IO_READY bits */
        ret = sd_poll_card_status(priv, CARD_IO_READY | DN_LD_CARD_RDY);
        if (ret < 0) {
            PRINTM(FATAL, "Helper download poll status timeout @ %d\n",
                   hlprblknow);
            goto done;
        }

        /* More data? */
        if (hlprblknow >= helperlen)
            break;

        /* Set blocksize to transfer - checking for last block */
        if (helperlen - hlprblknow < tx_len)
            tx_len = helperlen - hlprblknow;

        hlprbuf[0] = ((tx_len & 0x000000ff) >> 0);      /* Little-endian */
        hlprbuf[1] = ((tx_len & 0x0000ff00) >> 8);
        hlprbuf[2] = ((tx_len & 0x00ff0000) >> 16);
        hlprbuf[3] = ((tx_len & 0xff000000) >> 24);

        /* Copy payload to buffer */
        memcpy(&hlprbuf[SDIO_HEADER_LEN], &helper[hlprblknow], tx_len);

        PRINTM(INFO, ".");

        /* Send data */
        ret = sdio_writesb(card->func, priv->bt_dev.ioport,
                           hlprbuf,
                           FIRMWARE_TRANSFER_NBLOCK * SD_BLOCK_SIZE_FW_DL);

        if (ret < 0) {
            PRINTM(FATAL, "IO error during helper download @ %d\n", hlprblknow);
            goto done;
        }

        hlprblknow += tx_len;
    } while (TRUE);

#ifdef FW_DOWNLOAD_SPEED
    tv2 = get_utimeofday();
    PRINTM(INFO, "helper: %ld.%03ld.%03ld ", tv1 / 1000000,
           (tv1 % 1000000) / 1000, tv1 % 1000);
    PRINTM(INFO, " -> %ld.%03ld.%03ld ", tv2 / 1000000, (tv2 % 1000000) / 1000,
           tv2 % 1000);
    tv2 -= tv1;
    PRINTM(INFO, " == %ld.%03ld.%03ld\n", tv2 / 1000000, (tv2 % 1000000) / 1000,
           tv2 % 1000);
#endif

    /* Write last EOF data */
    PRINTM(INFO, "\nTransferring helper image EOF block\n");
    memset(hlprbuf, 0x0, SD_BLOCK_SIZE_FW_DL);

    ret =
        sdio_writesb(card->func, priv->bt_dev.ioport, hlprbuf,
                     SD_BLOCK_SIZE_FW_DL);

    if (ret < 0) {
        PRINTM(FATAL, "IO error in writing helper image EOF block\n");
        goto done;
    }

    ret = BT_STATUS_SUCCESS;

  done:
    if (tmphlprbuf)
        kfree(tmphlprbuf);
    if (fw_helper)
        release_firmware(fw_helper);

    LEAVE();
    return ret;
}

/** 
 *  @brief This function downloads firmware image to the card.
 *  
 *  @param priv    	A pointer to bt_private structure
 *  @return 	   	BT_STATUS_SUCCESS or BT_STATUS_FAILURE
 */
int
sd_download_firmware_w_helper(bt_private * priv)
{
    struct sdio_mmc_card *card = (struct sdio_mmc_card *) priv->bt_dev.card;
    const struct firmware *fw_firmware = NULL;
    u8 *firmware = NULL;
    int firmwarelen;
    u8 base0;
    u8 base1;
    int ret = BT_STATUS_SUCCESS;
    int offset;
    void *tmpfwbuf = NULL;
    int tmpfwbufsz;
    u8 *fwbuf;
    u16 len;
    int txlen = 0;
    int tx_blocks = 0;
    int i = 0;
    int tries = 0;
#ifdef FW_DOWNLOAD_SPEED
    u32 tv1, tv2;
#endif

    ENTER();

    if ((ret =
         request_firmware(&fw_firmware, fw_name, priv->hotplug_device)) < 0) {
        PRINTM(FATAL, "request_firmware() failed, error code = %#x\n", ret);
        goto done;
    }

    if (fw_firmware) {
        firmware =  (u8 *)fw_firmware->data;
        firmwarelen = fw_firmware->size;
    } else {
        PRINTM(MSG, "No firmware image found! Terminating download\n");
        ret = BT_STATUS_FAILURE;
        goto done;
    }

    PRINTM(INFO, "Downloading FW image (%d bytes)\n", firmwarelen);

#ifdef FW_DOWNLOAD_SPEED
    tv1 = get_utimeofday();
#endif

    tmpfwbufsz = BT_UPLD_SIZE;
    tmpfwbuf = kmalloc(tmpfwbufsz, GFP_KERNEL);
    if (!tmpfwbuf) {
        PRINTM(ERROR,
               "Unable to allocate buffer for firmware. Terminating download\n");
        ret = BT_STATUS_FAILURE;
        goto done;
    }
    memset(tmpfwbuf, 0, tmpfwbufsz);
    fwbuf = (u8 *) tmpfwbuf;

    /* Perform firmware data transfer */
    offset = 0;
    do {
        /* The host polls for the DN_LD_CARD_RDY and CARD_IO_READY bits */
        ret = sd_poll_card_status(priv, CARD_IO_READY | DN_LD_CARD_RDY);
        if (ret < 0) {
            PRINTM(FATAL, "FW download with helper poll status timeout @ %d\n",
                   offset);
            goto done;
        }

        /* More data? */
        if (offset >= firmwarelen)
            break;

        for (tries = 0; tries < MAX_POLL_TRIES; tries++) {
            base0 = sdio_readb(card->func, SQ_READ_BASE_ADDRESS_A0_REG, &ret);
            if (ret) {
                PRINTM(WARN, "Dev BASE0 register read failed:"
                       " base0=0x%04X(%d). Terminating download\n", base0,
                       base0);
                ret = BT_STATUS_FAILURE;
                goto done;
            }
            base1 = sdio_readb(card->func, SQ_READ_BASE_ADDRESS_A1_REG, &ret);
            if (ret) {
                PRINTM(WARN, "Dev BASE1 register read failed:"
                       " base1=0x%04X(%d). Terminating download\n", base1,
                       base1);
                ret = BT_STATUS_FAILURE;
                goto done;
            }
            len = (((u16) base1) << 8) | base0;

            if (len != 0)
                break;
            udelay(10);
        }

        if (len == 0)
            break;
        else if (len > BT_UPLD_SIZE) {
            PRINTM(FATAL, "FW download failure @ %d, invalid length %d\n",
                   offset, len);
            ret = BT_STATUS_FAILURE;
            goto done;
        }

        txlen = len;

        if (len & BIT(0)) {
            i++;
            if (i > MAX_WRITE_IOMEM_RETRY) {
                PRINTM(FATAL,
                       "FW download failure @ %d, over max retry count\n",
                       offset);
                ret = BT_STATUS_FAILURE;
                goto done;
            }
            PRINTM(ERROR, "FW CRC error indicated by the helper:"
                   " len = 0x%04X, txlen = %d\n", len, txlen);
            len &= ~BIT(0);
            /* Setting this to 0 to resend from same offset */
            txlen = 0;
        } else {
            i = 0;

            /* Set blocksize to transfer - checking for last block */
            if (firmwarelen - offset < txlen)
                txlen = firmwarelen - offset;

            PRINTM(INFO, ".");

            tx_blocks = (txlen + SD_BLOCK_SIZE_FW_DL - 1) / SD_BLOCK_SIZE_FW_DL;

            /* Copy payload to buffer */
            memcpy(fwbuf, &firmware[offset], txlen);
        }

        /* Send data */
        ret =
            sdio_writesb(card->func, priv->bt_dev.ioport, fwbuf,
                         tx_blocks * SD_BLOCK_SIZE_FW_DL);

        if (ret < 0) {
            PRINTM(ERROR, "FW download, write iomem (%d) failed @ %d\n", i,
                   offset);
            sdio_writeb(card->func, 0x04, CONFIGURATION_REG, &ret);
            if (ret)
                PRINTM(ERROR, "write ioreg failed (CFG)\n");
        }

        offset += txlen;
    } while (TRUE);

    PRINTM(INFO, "\nFW download over, size %d bytes\n", offset);

    ret = BT_STATUS_SUCCESS;
  done:
#ifdef FW_DOWNLOAD_SPEED
    tv2 = get_utimeofday();
    PRINTM(INFO, "FW: %ld.%03ld.%03ld ", tv1 / 1000000,
           (tv1 % 1000000) / 1000, tv1 % 1000);
    PRINTM(INFO, " -> %ld.%03ld.%03ld ", tv2 / 1000000,
           (tv2 % 1000000) / 1000, tv2 % 1000);
    tv2 -= tv1;
    PRINTM(INFO, " == %ld.%03ld.%03ld\n", tv2 / 1000000,
           (tv2 % 1000000) / 1000, tv2 % 1000);
#endif
    if (tmpfwbuf)
        kfree(tmpfwbuf);
    if (fw_firmware)
        release_firmware(fw_firmware);

    LEAVE();
    return ret;
}

/** 
 *  @brief This function reads data from the card.
 *  
 *  @param priv    	A pointer to bt_private structure
 *  @return 	   	BT_STATUS_SUCCESS or BT_STATUS_FAILURE
 */
static int
sd_card_to_host(bt_private * priv)
{
    int ret = BT_STATUS_SUCCESS;
    u16 buf_len = 0;
    int buf_block_len;
    int blksz;
    struct sk_buff *skb = NULL;
    u32 type;
    u8 *payload = NULL;
    struct hci_dev *hdev = priv->bt_dev.hcidev;
    struct sdio_mmc_card *card = priv->bt_dev.card;

    ENTER();

    if (!card || !card->func) {
        PRINTM(ERROR, "card or function is NULL!\n");
        ret = BT_STATUS_FAILURE;
        goto exit;
    }

    /* Read the length of data to be transferred */
    ret = sd_read_rx_len(priv, &buf_len);
    if (ret < 0) {
        PRINTM(ERROR, "card_to_host, read scratch reg failed\n");
        ret = BT_STATUS_FAILURE;
        goto exit;
    }

    /* Allocate buffer */
    blksz = SD_BLOCK_SIZE;
    buf_block_len = (buf_len + blksz - 1) / blksz;
    if (buf_len <= SDIO_HEADER_LEN || (buf_block_len * blksz) > ALLOC_BUF_SIZE) {
        PRINTM(ERROR, "card_to_host, invalid packet length: %d\n", buf_len);
        ret = BT_STATUS_FAILURE;
        goto exit;
    }
    skb = bt_skb_alloc(buf_block_len * blksz, GFP_ATOMIC);
    if (skb == NULL) {
        PRINTM(WARN, "No free skb\n");
        goto exit;
    }

    payload = skb->tail;
    ret = sdio_readsb(card->func, payload, priv->bt_dev.ioport,
                      buf_block_len * blksz);
    if (ret < 0) {
        PRINTM(ERROR, "card_to_host, read iomem failed: %d\n", ret);
        ret = BT_STATUS_FAILURE;
        goto exit;
    }
    DBG_HEXDUMP(DBG_DATA, "SDIO Blk Rd", payload, blksz * buf_block_len);
    /* This is SDIO specific header length: byte[2][1][0], type: byte[3]
       (HCI_COMMAND = 1, ACL_DATA = 2, SCO_DATA = 3, 0xFE = Vendor) */
    buf_len = payload[0];
    buf_len |= (u16) payload[1] << 8;
    type = payload[3];
    switch (type) {
    case HCI_ACLDATA_PKT:
    case HCI_SCODATA_PKT:
    case HCI_EVENT_PKT:
        bt_cb(skb)->pkt_type = type;
        skb->dev = (void *) hdev;
        skb_put(skb, buf_len);
        skb_pull(skb, SDIO_HEADER_LEN);
        if (type == HCI_EVENT_PKT)
            check_evtpkt(priv, skb);
        hci_recv_frame(skb);
        hdev->stat.byte_rx += buf_len;
        break;
    case MRVL_VENDOR_PKT:
        bt_cb(skb)->pkt_type = HCI_VENDOR_PKT;
        skb->dev = (void *) hdev;
        skb_put(skb, buf_len);
        skb_pull(skb, SDIO_HEADER_LEN);
        if (BT_STATUS_SUCCESS != bt_process_event(priv, skb))
            hci_recv_frame(skb);
        hdev->stat.byte_rx += buf_len;
        break;
    default:
        /* Driver specified event and command resp should be handle here */
        PRINTM(INFO, "Unknow PKT type:%d\n", type);
        kfree_skb(skb);
        skb = NULL;
        break;
    }
  exit:
    if (ret) {
        hdev->stat.err_rx++;
        if (skb)
            kfree_skb(skb);
    }

    LEAVE();
    return ret;
}

#ifdef CONFIG_PM
/** 
 *  @brief This function handle the suspend function
 *  
 *  @param func    A pointer to sdio_func structure
 *  @return        BT_STATUS_SUCCESS
 */
int
sd_suspend(struct sdio_func *func)
{
    struct hci_dev *hcidev;
    struct sdio_mmc_card *card;
    bt_private *priv = NULL;

    ENTER();
    if (func) {
        card = sdio_get_drvdata(func);
        if (card)
            priv = card->priv;
    }
    if (!priv) {
        LEAVE();
        return BT_STATUS_SUCCESS;
    }
    if (priv->adapter->hs_state != HS_ACTIVATED) {
        if (BT_STATUS_SUCCESS != bt_enable_hs(priv)) {
            LEAVE();
            return BT_STATUS_FAILURE;
        }
    }
    hcidev = priv->bt_dev.hcidev;
    hci_suspend_dev(hcidev);
    skb_queue_purge(&priv->adapter->tx_queue);

    LEAVE();
    return BT_STATUS_SUCCESS;
}

/** 
 *  @brief This function handle the resume function
 *  
 *  @param func    A pointer to sdio_func structure
 *  @return        BT_STATUS_SUCCESS
 */
int
sd_resume(struct sdio_func *func)
{
    struct hci_dev *hcidev;
    struct sdio_mmc_card *card;
    bt_private *priv = NULL;

    ENTER();

    if (func) {
        card = sdio_get_drvdata(func);
        if (card)
            priv = card->priv;
    }
    if (!priv) {
        LEAVE();
        return BT_STATUS_SUCCESS;
    }
    hcidev = priv->bt_dev.hcidev;
    hci_resume_dev(hcidev);
    /* if (is_bt_the_wakeup_src()){ */
    {
        PRINTM(MSG, "WAKEUP SRC: BT\n");
        if ((priv->bt_dev.gpio_gap & 0x00ff) == 0xff) {
            sbi_wakeup_firmware(priv);
            priv->adapter->hs_state = HS_DEACTIVATED;
        }
    }

    LEAVE();
    return BT_STATUS_SUCCESS;
}
#endif

/** 
 *  @brief This function removes the card
 *  
 *  @param func    A pointer to sdio_func structure
 *  @return        BT_STATUS_SUCCESS or BT_STATUS_FAILURE
 */
static void
sd_remove_card(struct sdio_func *func)
{
    struct sdio_mmc_card *card;

    ENTER();

    if (func) {
        card = sdio_get_drvdata(func);
        if (card) {
            bt_remove_card(card);
            kfree(card);
        }
    }

    LEAVE();
}

/** 
 *  @brief This function handles the interrupt.
 *  
 *  @param func  A pointer to sdio_func structure
 *  @return      n/a
 */
static void
sd_interrupt(struct sdio_func *func)
{
    bt_private *priv;
    struct hci_dev *hcidev;
    struct sdio_mmc_card *card;
    u8 ireg = 0;

    ENTER();

    card = sdio_get_drvdata(func);
    if (card && card->priv) {
        priv = card->priv;
        hcidev = priv->bt_dev.hcidev;
        bt_interrupt(hcidev);
        if (sbi_get_int_status(priv, &ireg)) {
            PRINTM(ERROR, "%s: reading HOST_INT_STATUS_REG failed\n",
                   __FUNCTION__);
        } else
            PRINTM(INFO, "%s: HOST_INT_STATUS_REG %#x\n", __FUNCTION__, ireg);
        wake_up_interruptible(&priv->MainThread.waitQ);
    }

    LEAVE();
}

/********************************************************
		Global Functions
********************************************************/
static struct sdio_driver sdio_bt = {
    .name = "sdio_bt",
    .id_table = bt_ids,
    .probe = sd_probe_card,
    .remove = sd_remove_card,
/*
#ifdef CONFIG_PM
	.suspend	= sd_suspend,
	.resume		= sd_resume, 
#endif
*/
};

/** 
 *  @brief This function registers the bt module in bus driver.
 *  
 *  @return	   An int pointer that keeps returned value
 */
int *
sbi_register(void)
{
    int *ret;

    ENTER();

    if (sdio_register_driver(&sdio_bt) != 0) {
        PRINTM(FATAL, "SD Driver Registration Failed \n");
        LEAVE();
        return NULL;
    } else
        ret = (int *) 1;

    LEAVE();
    return ret;
}

/** 
 *  @brief This function de-registers the bt module in bus driver.
 *  
 *  @return 	   n/a
 */
void
sbi_unregister(void)
{
    ENTER();
    sdio_unregister_driver(&sdio_bt);
    LEAVE();
}

/** 
 *  @brief This function registers the device.
 *  
 *  @param priv    A pointer to bt_private structure
 *  @return 	   BT_STATUS_SUCCESS or BT_STATUS_FAILURE
 */
int
sbi_register_dev(bt_private * priv)
{
    int ret = BT_STATUS_SUCCESS;
    u8 reg;
    u8 chiprev;
    struct sdio_mmc_card *card = priv->bt_dev.card;
    struct sdio_func *func;

    ENTER();

    if (!card || !card->func) {
        PRINTM(ERROR, "Error: card or function is NULL!\n");
        goto failed;
    }
    func = card->func;
    priv->hotplug_device = &func->dev;
    if (helper_name == NULL)
        helper_name = DEFAULT_HELPER_NAME;

    if (fw_name == NULL)
        fw_name = DEFAULT_FW_NAME;

    /* Initialize the private structure */
    strncpy(priv->bt_dev.name, "bt_sdio0", sizeof(priv->bt_dev.name));
    priv->bt_dev.ioport = 0;
    priv->bt_dev.fn = func->num;

    sdio_claim_host(func);
    ret = sdio_enable_func(func);
    if (ret) {
        PRINTM(FATAL, "sdio_enable_func() failed: ret=%d\n", ret);
        goto release_host;
    }
    ret = sdio_claim_irq(func, sd_interrupt);
    if (ret) {
        PRINTM(FATAL, "sdio_claim_irq failed: ret=%d\n", ret);
        goto disable_func;
    }
    ret = sdio_set_block_size(card->func, SD_BLOCK_SIZE);
    if (ret) {
        PRINTM(FATAL, "%s: cannot set SDIO block size\n", __FUNCTION__);
        goto release_irq;
    }

    /* read Revision Register to get the chip revision number */
    chiprev = sdio_readb(func, CARD_REVISION_REG, &ret);
    if (ret) {
        PRINTM(FATAL, "cannot read CARD_REVISION_REG\n");
        goto release_irq;
    }
    priv->adapter->chip_rev = chiprev;
    PRINTM(INFO, "revision=%#x\n", chiprev);

    /* Read the IO port */
    reg = sdio_readb(func, IO_PORT_0_REG, &ret);
    if (ret < 0)
        goto release_irq;
    else
        priv->bt_dev.ioport |= reg;

    reg = sdio_readb(func, IO_PORT_1_REG, &ret);
    if (ret < 0)
        goto release_irq;
    else
        priv->bt_dev.ioport |= (reg << 8);

    reg = sdio_readb(func, IO_PORT_2_REG, &ret);
    if (ret < 0)
        goto release_irq;
    else
        priv->bt_dev.ioport |= (reg << 16);

    PRINTM(INFO, "SDIO FUNC%d IO port: 0x%x\n", priv->bt_dev.fn,
           priv->bt_dev.ioport);
    sdio_set_drvdata(func, card);
    sdio_release_host(func);

    LEAVE();
    return BT_STATUS_SUCCESS;
  release_irq:
    sdio_release_irq(func);
  disable_func:
    sdio_disable_func(func);
  release_host:
    sdio_release_host(func);
  failed:

    LEAVE();
    return BT_STATUS_FAILURE;
}

/** 
 *  @brief This function de-registers the device.
 *  
 *  @param priv    A pointer to  bt_private structure
 *  @return 	   BT_STATUS_SUCCESS
 */
int
sbi_unregister_dev(bt_private * priv)
{
    struct sdio_mmc_card *card = priv->bt_dev.card;

    ENTER();

    if (card && card->func) {
        sdio_claim_host(card->func);
        sdio_release_irq(card->func);
        sdio_disable_func(card->func);
        sdio_release_host(card->func);
        sdio_set_drvdata(card->func, NULL);
    }

    LEAVE();
    return BT_STATUS_SUCCESS;
}

/** 
 *  @brief This function enables the host interrupts.
 *  
 *  @param priv    A pointer to bt_private structure
 *  @return 	   BT_STATUS_SUCCESS
 */
int
sbi_enable_host_int(bt_private * priv)
{
    struct sdio_mmc_card *card = priv->bt_dev.card;
    int ret;

    ENTER();

    if (!card || !card->func) {
        LEAVE();
        return BT_STATUS_FAILURE;
    }
    sdio_claim_host(card->func);
    ret = sd_enable_host_int_mask(priv, HIM_ENABLE);
    sd_get_rx_unit(priv);
    sdio_release_host(card->func);

    LEAVE();
    return ret;
}

/** 
 *  @brief This function disables the host interrupts.
 *  
 *  @param priv    A pointer to bt_private structure
 *  @return 	   BT_STATUS_SUCCESS or BT_STATUS_FAILURE
 */
int
sbi_disable_host_int(bt_private * priv)
{
    struct sdio_mmc_card *card = priv->bt_dev.card;
    int ret;

    ENTER();

    if (!card || !card->func) {
        LEAVE();
        return BT_STATUS_FAILURE;
    }
    sdio_claim_host(card->func);
    ret = sd_disable_host_int_mask(priv, HIM_DISABLE);
    sdio_release_host(card->func);

    LEAVE();
    return ret;
}

/**  
 *  @brief This function sends data to the card.
 *  
 *  @param priv    A pointer to bt_private structure
 *  @param payload A pointer to the data/cmd buffer
 *  @param nb	   the length of data/cmd
 *  @return 	   BT_STATUS_SUCCESS or BT_STATUS_FAILURE
 */
int
sbi_host_to_card(bt_private * priv, u8 * payload, u16 nb)
{
    struct sdio_mmc_card *card = priv->bt_dev.card;
    int ret = BT_STATUS_SUCCESS;
    int buf_block_len;
    int blksz;
    int i = 0;
    u8 *buf = NULL;

    ENTER();

    if (!card || !card->func) {
        PRINTM(ERROR, "card or function is NULL!\n");
        LEAVE();
        return BT_STATUS_FAILURE;
    }
    buf = payload;
    /* Allocate buffer and copy payload */
    blksz = SD_BLOCK_SIZE;
    buf_block_len = (nb + blksz - 1) / blksz;
    sdio_claim_host(card->func);
#define MAX_WRITE_IOMEM_RETRY	2
    do {
        /* Transfer data to card */
        ret = sdio_writesb(card->func, priv->bt_dev.ioport, buf,
                           buf_block_len * blksz);
        if (ret < 0) {
            i++;
            PRINTM(ERROR, "host_to_card, write iomem (%d) failed: %d\n", i,
                   ret);
            ret = BT_STATUS_FAILURE;
            if (i > MAX_WRITE_IOMEM_RETRY)
                goto exit;
        } else {
            DBG_HEXDUMP(DBG_DATA, "SDIO Blk Wr", payload, nb);
        }
    } while (ret == BT_STATUS_FAILURE);
    priv->bt_dev.tx_dnld_rdy = FALSE;
  exit:
    sdio_release_host(card->func);

    LEAVE();
    return ret;
}

/** 
 *  @brief This function initializes firmware
 *  
 *  @param priv    A pointer to bt_private structure
 *  @return 	   BT_STATUS_SUCCESS or BT_STATUS_FAILURE
 */
int
sbi_dowload_fw(bt_private * priv)
{
    struct sdio_mmc_card *card = priv->bt_dev.card;
    int ret = BT_STATUS_SUCCESS;

    ENTER();

    if (!card || !card->func) {
        PRINTM(ERROR, "card or function is NULL!\n");
        LEAVE();
        return BT_STATUS_FAILURE;
    }
    sdio_claim_host(card->func);
    if (BT_STATUS_SUCCESS == sd_verify_fw_download(priv, 1)) {
        PRINTM(INFO, "Firmware already downloaded!\n");
        goto done;
    }
    /* Download the helper */
    ret = sd_dowload_helper(priv);
    if (ret) {
        PRINTM(INFO, "Fail to download helper!\n");
        ret = BT_STATUS_FAILURE;
        goto done;
    }
    /* Download the main firmware via the helper firmware */
    if (sd_download_firmware_w_helper(priv)) {
        PRINTM(INFO, "Bluetooth FW download failed!\n");
        ret = BT_STATUS_FAILURE;
        goto done;
    }
    /* check if the fimware is downloaded successfully or not */
    if (sd_verify_fw_download(priv, MAX_FIRMWARE_POLL_TRIES)) {
        PRINTM(INFO, "FW failed to be active in time!\n");
        ret = BT_STATUS_FAILURE;
        goto done;
    }
  done:
    sdio_release_host(card->func);

    LEAVE();
    return ret;
}

/** 
 *  @brief This function checks the interrupt status and handle it accordingly.
 *  
 *  @param priv    A pointer to bt_private structure
 *  @param ireg    A pointer to variable that keeps returned value
 *  @return 	   BT_STATUS_SUCCESS or BT_STATUS_FAILURE
 */
int
sbi_get_int_status(bt_private * priv, u8 * ireg)
{
    int ret = BT_STATUS_SUCCESS;
    u8 sdio_ireg = 0;
    struct sdio_mmc_card *card = priv->bt_dev.card;

    ENTER();

    *ireg = 0;
    sdio_ireg = sdio_readb(card->func, HOST_INTSTATUS_REG, &ret);
    if (ret) {
        PRINTM(WARN, "sdio_read_ioreg: read int status register failed\n");
        ret = BT_STATUS_FAILURE;
        goto done;
    }
    if (sdio_ireg != 0) {
        /* 
         * DN_LD_HOST_INT_STATUS and/or UP_LD_HOST_INT_STATUS
         * Clear the interrupt status register and re-enable the interrupt
         */
        PRINTM(INFO, "sdio_ireg = 0x%x\n", sdio_ireg);
        sdio_writeb(card->func, ~(sdio_ireg) & (DN_LD_HOST_INT_STATUS |
                                                UP_LD_HOST_INT_STATUS),
                    HOST_INTSTATUS_REG, &ret);

        if (ret) {
            PRINTM(WARN,
                   "sdio_write_ioreg: clear int status register failed\n");
            ret = BT_STATUS_FAILURE;
            goto done;
        }
    }

    if (sdio_ireg & DN_LD_HOST_INT_STATUS) {    /* tx_done INT */
        if (priv->bt_dev.tx_dnld_rdy) { /* tx_done already received */
            PRINTM(INFO,
                   "warning: tx_done already received: tx_dnld_rdy=0x%x int status=0x%x\n",
                   priv->bt_dev.tx_dnld_rdy, sdio_ireg);
        } else {
            priv->bt_dev.tx_dnld_rdy = TRUE;
        }
    }
    if (sdio_ireg & UP_LD_HOST_INT_STATUS)
        sd_card_to_host(priv);

    *ireg = sdio_ireg;
    ret = BT_STATUS_SUCCESS;
  done:
    LEAVE();
    return ret;
}

/** 
 *  @brief This function wakeup firmware
 *  
 *  @param priv    A pointer to bt_private structure
 *  @return 	   BT_STATUS_SUCCESS or BT_STATUS_FAILURE
 */
int
sbi_wakeup_firmware(bt_private * priv)
{
    struct sdio_mmc_card *card = priv->bt_dev.card;
    int ret = BT_STATUS_SUCCESS;

    ENTER();

    if (!card || !card->func) {
        PRINTM(ERROR, "card or function is NULL!\n");
        LEAVE();
        return BT_STATUS_FAILURE;
    }
    sdio_claim_host(card->func);
    sdio_writeb(card->func, HOST_POWER_UP, CONFIGURATION_REG, &ret);
    sdio_release_host(card->func);
    PRINTM(CMD, "wake up firmware\n");

    LEAVE();
    return ret;
}

module_param(helper_name, charp, 0);
MODULE_PARM_DESC(helper_name, "Helper name");
module_param(fw_name, charp, 0);
MODULE_PARM_DESC(fw_name, "Firmware name");
