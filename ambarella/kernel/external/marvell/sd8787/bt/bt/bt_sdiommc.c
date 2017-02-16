/** @file bt_sdiommc.c
 *  @brief This file contains SDIO IF (interface) module
 *  related functions.
 * 
 * Copyright (C) 2007-2011, Marvell International Ltd.
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
/** Firmware name */
char *fw_name = NULL;
/** FW header length for CRC check disable */
#define FW_CRC_HEADER   24
/** FW header for CRC check disable */
u8 fw_crc_header[FW_CRC_HEADER] =
    { 0x01, 0x00, 0x00, 0x00, 0x04, 0xfd, 0x00, 0x04,
    0x08, 0x00, 0x00, 0x00, 0x26, 0x52, 0x2a, 0x7b,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

/** Default firmware name */
#define DEFAULT_FW_NAME "mrvl/sd8787_uapsta.bin"

/** Function number 2 */
#define FN2			2
/** SD 8790 BT device ID */
#define SD_DEVICE_ID_8790_BT    0x911D
/** SD 8688 BT device ID */
#define SD_DEVICE_ID_8688_BT    0x9105
/** Device ID for SD8787 FN2 */
#define SD_DEVICE_ID_8787_BT_FN2    0x911A
/** Device ID for SD8787 FN3 */
#define SD_DEVICE_ID_8787_BT_FN3    0x911B

static const struct sdio_device_id bt_ids[] = {
    {SDIO_DEVICE(MARVELL_VENDOR_ID, SD_DEVICE_ID_8790_BT)},
    {SDIO_DEVICE(MARVELL_VENDOR_ID, SD_DEVICE_ID_8688_BT)},
    {SDIO_DEVICE(MARVELL_VENDOR_ID, SD_DEVICE_ID_8787_BT_FN2)},
    {}
};

MODULE_DEVICE_TABLE(sdio, bt_ids);

/********************************************************
		Global Variables
********************************************************/
/** unregiser bus driver flag */
static u8 unregister = 0;
#ifdef SDIO_SUSPEND_RESUME
/** PM keep power */
extern int pm_keep_power;
#endif

/********************************************************
		Local Functions
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
 *  @brief This function reads updates the Cmd52 value in dev structure
 *  
 *  @param priv    	A pointer to bt_private structure
 *  @return 	   	BT_STATUS_SUCCESS or BT_STATUS_FAILURE
 */
int
sd_read_cmd52_val(bt_private * priv)
{
    int ret = BT_STATUS_SUCCESS;
    u8 func, reg, val;
    struct sdio_mmc_card *card = (struct sdio_mmc_card *) priv->bt_dev.card;

    ENTER();

    func = priv->bt_dev.cmd52_func;
    reg = priv->bt_dev.cmd52_reg;
    sdio_claim_host(card->func);
    if (func)
        val = sdio_readb(card->func, reg, &ret);
    else
        val = sdio_f0_readb(card->func, reg, &ret);
    sdio_release_host(card->func);
    if (ret) {
        PRINTM(ERROR, "Cannot read value from func %d reg %d\n", func, reg);
    } else {
        priv->bt_dev.cmd52_val = val;
    }

    LEAVE();
    return ret;
}

/** 
 *  @brief This function updates card reg based on the Cmd52 value in dev structure
 *  
 *  @param priv    	A pointer to bt_private structure
 *  @param func    	A pointer to store func variable
 *  @param reg    	A pointer to store reg variable
 *  @param val    	A pointer to store val variable
 *  @return 	   	BT_STATUS_SUCCESS or BT_STATUS_FAILURE
 */
int
sd_write_cmd52_val(bt_private * priv, int func, int reg, int val)
{
    int ret = BT_STATUS_SUCCESS;
    struct sdio_mmc_card *card = (struct sdio_mmc_card *) priv->bt_dev.card;

    ENTER();

    if (val >= 0) {
        /* Perform actual write only if val is provided */
        sdio_claim_host(card->func);
        if (func)
            sdio_writeb(card->func, val, reg, &ret);
        else
            sdio_f0_writeb(card->func, val, reg, &ret);
        sdio_release_host(card->func);
        if (ret) {
            PRINTM(ERROR, "Cannot write value (0x%x) to func %d reg %d\n", val,
                   func, reg);
            goto done;
        }
        priv->bt_dev.cmd52_val = val;
    }

    /* Save current func and reg for future read */
    priv->bt_dev.cmd52_func = func;
    priv->bt_dev.cmd52_reg = reg;

  done:
    LEAVE();
    return ret;
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

    PRINTM(INFO, "vendor=0x%x,device=0x%x,class=%d,fn=%d\n", id->vendor,
           id->device, id->class, func->num);
    card = kzalloc(sizeof(struct sdio_mmc_card), GFP_KERNEL);
    if (!card) {
        ret = -ENOMEM;
        goto done;
    }
    card->func = func;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27)
    /* wait for chip fully wake up */
    if (!func->enable_timeout)
        func->enable_timeout = 200;
#endif
    sdio_claim_host(func);
    ret = sdio_enable_func(func);
    if (ret) {
        sdio_disable_func(func);
        sdio_release_host(func);
        PRINTM(FATAL, "sdio_enable_func() failed: ret=%d\n", ret);
        kfree(card);
        LEAVE();
        return -EIO;
    }
    sdio_release_host(func);
    priv = bt_add_card(card);
    if (!priv) {
        sdio_claim_host(func);
        sdio_disable_func(func);
        sdio_release_host(func);
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
            PRINTM(MSG, "BT FW is active(%d)\n", tries);
            ret = BT_STATUS_SUCCESS;
            break;
        } else {
            mdelay(100);
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
    u8 crc_buffer = 0;

    ENTER();

    if ((ret =
         request_firmware(&fw_firmware, fw_name, priv->hotplug_device)) < 0) {
        PRINTM(FATAL, "request_firmware() failed, error code = %#x\n", ret);
        goto done;
    }

    if (fw_firmware) {
        firmware = (u8 *) fw_firmware->data;
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

    tmpfwbufsz = ALIGN_SZ(BT_UPLD_SIZE, DMA_ALIGNMENT);
    tmpfwbuf = kmalloc(tmpfwbufsz, GFP_KERNEL);
    if (!tmpfwbuf) {
        PRINTM(ERROR,
               "Unable to allocate buffer for firmware. Terminating download\n");
        ret = BT_STATUS_FAILURE;
        goto done;
    }
    memset(tmpfwbuf, 0, tmpfwbufsz);
    /* Ensure 8-byte aligned firmware buffer */
    fwbuf = (u8 *) ALIGN_ADDR(tmpfwbuf, DMA_ALIGNMENT);

    if (!priv->fw_crc_check) {
        /* CRC check not required, use custom header first */
        firmware = fw_crc_header;
        firmwarelen = FW_CRC_HEADER;
        crc_buffer = 1;
    }

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
        if (!crc_buffer)
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
        if (crc_buffer && offset >= FW_CRC_HEADER) {
            /* Custom header download complete, restore original FW */
            offset = 0;
            firmware = (u8 *) fw_firmware->data;
            firmwarelen = fw_firmware->size;
            crc_buffer = 0;
        }
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
        PRINTM(ERROR, "BT card or function is NULL!\n");
        ret = BT_STATUS_FAILURE;
        goto exit;
    }

    /* Read the length of data to be transferred */
    ret = sd_read_rx_len(priv, &buf_len);
    if (ret < 0) {
        PRINTM(ERROR, "BT card_to_host, read scratch reg failed\n");
        ret = BT_STATUS_FAILURE;
        goto exit;
    }

    /* Allocate buffer */
    blksz = SD_BLOCK_SIZE;
    buf_block_len = (buf_len + blksz - 1) / blksz;
    if (buf_len <= BT_HEADER_LEN || (buf_block_len * blksz) > ALLOC_BUF_SIZE) {
        PRINTM(ERROR, "BT card_to_host, invalid packet length: %d\n", buf_len);
        ret = BT_STATUS_FAILURE;
        goto exit;
    }
    skb = bt_skb_alloc(buf_block_len * blksz + DMA_ALIGNMENT, GFP_ATOMIC);
    if (skb == NULL) {
        PRINTM(WARN, "BT No free skb\n");
        goto exit;
    }
    if ((u32) skb->data & (DMA_ALIGNMENT - 1)) {
        skb_put(skb, DMA_ALIGNMENT - ((u32) skb->data & (DMA_ALIGNMENT - 1)));
        skb_pull(skb, DMA_ALIGNMENT - ((u32) skb->data & (DMA_ALIGNMENT - 1)));
    }

    payload = skb->tail;
    ret = sdio_readsb(card->func, payload, priv->bt_dev.ioport,
                      buf_block_len * blksz);
    if (ret < 0) {
        PRINTM(ERROR, "BT card_to_host, read iomem failed: %d\n", ret);
        ret = BT_STATUS_FAILURE;
        goto exit;
    }
    /* This is SDIO specific header length: byte[2][1][0], type: byte[3]
       (HCI_COMMAND = 1, ACL_DATA = 2, SCO_DATA = 3, 0xFE = Vendor) */
    buf_len = payload[0];
    buf_len |= (u16) payload[1] << 8;
    type = payload[3];
    PRINTM(DATA, "BT SDIO Blk Rd %s: len=%d type=%d\n", hdev->name, buf_len,
           type);
    if (buf_len > buf_block_len * blksz) {
        PRINTM(ERROR,
               "BT: Drop invalid rx pkt, len in hdr=%d, cmd53 length=%d\n",
               buf_len, buf_block_len * blksz);
        ret = BT_STATUS_FAILURE;
        kfree_skb(skb);
        skb = NULL;
        goto exit;
    }
    DBG_HEXDUMP(DAT_D, "BT SDIO Blk Rd", payload, buf_len);
    switch (type) {
    case HCI_ACLDATA_PKT:
    case HCI_SCODATA_PKT:
    case HCI_EVENT_PKT:
        bt_cb(skb)->pkt_type = type;
        skb->dev = (void *) hdev;
        skb_put(skb, buf_len);
        skb_pull(skb, BT_HEADER_LEN);
        if (type == HCI_EVENT_PKT)
            check_evtpkt(priv, skb);
        hci_recv_frame(skb);
        hdev->stat.byte_rx += buf_len;
        break;
    case MRVL_VENDOR_PKT:
        bt_cb(skb)->pkt_type = HCI_VENDOR_PKT;
        skb->dev = (void *) hdev;
        skb_put(skb, buf_len);
        skb_pull(skb, BT_HEADER_LEN);
        if (BT_STATUS_SUCCESS != bt_process_event(priv, skb))
            hci_recv_frame(skb);
        hdev->stat.byte_rx += buf_len;
        break;
    default:
        /* Driver specified event and command resp should be handle here */
        PRINTM(INFO, "BT Unknow PKT type:%d\n", type);
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
            if (!unregister && card->priv) {
                PRINTM(INFO, "card removed from sd slot\n");
                ((bt_private *) (card->priv))->adapter->SurpriseRemoved = TRUE;
            }
            bt_remove_card(card->priv);
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
    int ret = BT_STATUS_SUCCESS;
    u8 ireg = 0;

    ENTER();

    card = sdio_get_drvdata(func);
    if (!card || !card->priv) {
        PRINTM(INFO, "%s: sbi_interrupt(%p) card or priv is NULL, card=%p\n",
               __FUNCTION__, func, card);
        LEAVE();
        return;
    }
    priv = card->priv;
    hcidev = priv->bt_dev.hcidev;

    ireg = sdio_readb(card->func, HOST_INTSTATUS_REG, &ret);
    if (ret) {
        PRINTM(WARN, "BT sdio_read_ioreg: read int status register failed\n");
        goto done;
    }
    if (ireg != 0) {
        /* 
         * DN_LD_HOST_INT_STATUS and/or UP_LD_HOST_INT_STATUS
         * Clear the interrupt status register and re-enable the interrupt
         */
        PRINTM(INTR, "BT INT %s: sdio_ireg = 0x%x\n", hcidev->name, ireg);
        priv->adapter->irq_recv = ireg;
        sdio_writeb(card->func,
                    ~(ireg) & (DN_LD_HOST_INT_STATUS | UP_LD_HOST_INT_STATUS),
                    HOST_INTSTATUS_REG, &ret);
        if (ret) {
            PRINTM(WARN,
                   "sdio_write_ioreg: clear int status register failed\n");
            goto done;
        }
    } else {
        PRINTM(ERROR, "BT ERR: ireg=0\n");
    }
    OS_INT_DISABLE;
    priv->adapter->sd_ireg |= ireg;
    OS_INT_RESTORE;
    bt_interrupt(hcidev);
  done:
    LEAVE();
}

/** 
 *  @brief This function checks if the interface is ready to download
 *  or not while other download interfaces are present
 *  
 *  @param priv   A pointer to bt_private structure
 *  @param val    Winner status (0: winner)
 *  @return       BT_STATUS_SUCCESS or BT_STATUS_FAILURE
 */
int
sd_check_winner_status(bt_private * priv, u8 * val)
{

    int ret = BT_STATUS_SUCCESS;
    u8 winner = 0;
    struct sdio_mmc_card *cardp = (struct sdio_mmc_card *) priv->bt_dev.card;

    ENTER();

    winner = sdio_readb(cardp->func, CARD_FW_STATUS0_REG, &ret);
    if (ret != BT_STATUS_SUCCESS) {
        LEAVE();
        return BT_STATUS_FAILURE;
    }
    *val = winner;

    LEAVE();
    return ret;
}

#ifdef SDIO_SUSPEND_RESUME
#ifdef MMC_PM_KEEP_POWER
#ifdef MMC_PM_FUNC_SUSPENDED
/**  @brief This function tells lower driver that BT is suspended
 *
 *  @param priv    A pointer to bt_private structure
 *  @return        None
 */
void
bt_is_suspended(bt_private * priv)
{
    struct sdio_mmc_card *card = priv->bt_dev.card;
    priv->adapter->is_suspended = TRUE;
    sdio_func_suspended(card->func);
}
#endif

/**  @brief This function handles client driver suspend
 *  
 *  @param dev	   A pointer to device structure
 *  @return 	   BT_STATUS_SUCCESS or BT_STATUS_FAILURE
 */
int
bt_sdio_suspend(struct device *dev)
{
    struct sdio_func *func = dev_to_sdio_func(dev);
    mmc_pm_flag_t pm_flags = 0;
    bt_private *priv = NULL;
    struct sdio_mmc_card *cardp;
    struct hci_dev *hcidev;

    ENTER();

    if (func) {
        pm_flags = sdio_get_host_pm_caps(func);
        PRINTM(CMD, "%s: suspend: PM flags = 0x%x\n", sdio_func_id(func),
               pm_flags);
        if (!(pm_flags & MMC_PM_KEEP_POWER)) {
            PRINTM(ERROR, "%s: cannot remain alive while host is suspended\n",
                   sdio_func_id(func));
            return -ENOSYS;
        }
        cardp = sdio_get_drvdata(func);
        if (!cardp || !cardp->priv) {
            PRINTM(ERROR, "Card or priv structure is not valid\n");
            LEAVE();
            return BT_STATUS_SUCCESS;
        }
    } else {
        PRINTM(ERROR, "sdio_func is not specified\n");
        LEAVE();
        return BT_STATUS_SUCCESS;
    }
    PRINTM(CMD, "BT SDIO suspend\n");
    priv = cardp->priv;
    if ((pm_keep_power) && (priv->adapter->hs_state != HS_ACTIVATED)) {
        if (BT_STATUS_SUCCESS != bt_enable_hs(priv)) {
            PRINTM(CMD, "HS not actived, suspend fail!\n");
            LEAVE();
            return -EBUSY;
        }
    }
    hcidev = priv->bt_dev.hcidev;
    hci_suspend_dev(hcidev);
    skb_queue_purge(&priv->adapter->tx_queue);

    priv->adapter->is_suspended = TRUE;
    LEAVE();
    /* We will keep the power when hs enabled successfully */
    if ((pm_keep_power) && (priv->adapter->hs_state == HS_ACTIVATED)) {
#ifdef MMC_PM_SKIP_RESUME_PROBE
        PRINTM(CMD, "BT suspend with MMC_PM_KEEP_POWER and "
               "MMC_PM_SKIP_RESUME_PROBE\n");
        return sdio_set_host_pm_flags(func,
                                      MMC_PM_KEEP_POWER |
                                      MMC_PM_SKIP_RESUME_PROBE);
#else
        PRINTM(CMD, "BT suspend with MMC_PM_KEEP_POWER\n");
        return sdio_set_host_pm_flags(func, MMC_PM_KEEP_POWER);
#endif
    } else {
        PRINTM(CMD, "BT suspend without MMC_PM_KEEP_POWER\n");
        return BT_STATUS_SUCCESS;
    }
}

/**  @brief This function handles client driver resume
 *  
 *  @param dev	   A pointer to device structure
 *  @return 	   MLAN_STATUS_SUCCESS or MLAN_STATUS_FAILURE
 */
int
bt_sdio_resume(struct device *dev)
{
    struct sdio_func *func = dev_to_sdio_func(dev);
    mmc_pm_flag_t pm_flags = 0;
    bt_private *priv = NULL;
    struct sdio_mmc_card *cardp;
    struct hci_dev *hcidev;

    ENTER();
    if (func) {
        pm_flags = sdio_get_host_pm_caps(func);
        PRINTM(CMD, "%s: resume: PM flags = 0x%x\n", sdio_func_id(func),
               pm_flags);
        cardp = sdio_get_drvdata(func);
        if (!cardp || !cardp->priv) {
            PRINTM(ERROR, "Card or priv structure is not valid\n");
            LEAVE();
            return BT_STATUS_SUCCESS;
        }
    } else {
        PRINTM(ERROR, "sdio_func is not specified\n");
        LEAVE();
        return BT_STATUS_SUCCESS;
    }
    PRINTM(CMD, "BT SDIO resume\n");
    priv = cardp->priv;
    priv->adapter->is_suspended = FALSE;
    hcidev = priv->bt_dev.hcidev;
    hci_resume_dev(hcidev);
    sbi_wakeup_firmware(priv);
    priv->adapter->hs_state = HS_DEACTIVATED;
    LEAVE();
    return BT_STATUS_SUCCESS;
}
#endif
#endif

/********************************************************
		Global Functions
********************************************************/
#ifdef SDIO_SUSPEND_RESUME
#ifdef MMC_PM_KEEP_POWER
static struct dev_pm_ops bt_sdio_pm_ops = {
    .suspend = bt_sdio_suspend,
    .resume = bt_sdio_resume,
};
#endif
#endif
static struct sdio_driver sdio_bt = {
    .name = "sdio_bt",
    .id_table = bt_ids,
    .probe = sd_probe_card,
    .remove = sd_remove_card,
#ifdef SDIO_SUSPEND_RESUME
#ifdef MMC_PM_KEEP_POWER
    .drv = {
            .pm = &bt_sdio_pm_ops,
            }
#endif
#endif
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
    unregister = TRUE;
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
    if (fw_name == NULL)
        fw_name = DEFAULT_FW_NAME;

    /* Initialize the private structure */
    strncpy(priv->bt_dev.name, "bt_sdio0", sizeof(priv->bt_dev.name));
    priv->bt_dev.ioport = 0;
    priv->bt_dev.fn = func->num;

    sdio_claim_host(func);
    ret = sdio_claim_irq(func, sd_interrupt);
    if (ret) {
        PRINTM(FATAL, "sdio_claim_irq failed: ret=%d\n", ret);
        goto release_host;
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

    /* 
     * Read the HOST_INTSTATUS_REG for ACK the first interrupt got
     * from the bootloader. If we don't do this we get a interrupt
     * as soon as we register the irq. 
     */
    reg = sdio_readb(func, HOST_INTSTATUS_REG, &ret);
    if (ret < 0)
        goto release_irq;

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
    struct hci_dev *hdev = priv->bt_dev.hcidev;
    void *tmpbuf = NULL;
    int tmpbufsz;

    ENTER();

    if (!card || !card->func) {
        PRINTM(ERROR, "BT card or function is NULL!\n");
        LEAVE();
        return BT_STATUS_FAILURE;
    }
    buf = payload;

    /* Allocate buffer and copy payload */
    blksz = SD_BLOCK_SIZE;
    buf_block_len = (nb + blksz - 1) / blksz;
    if ((u32) payload & (DMA_ALIGNMENT - 1)) {
        tmpbufsz = buf_block_len * blksz + DMA_ALIGNMENT;
        tmpbuf = kmalloc(tmpbufsz, GFP_KERNEL);
        memset(tmpbuf, 0, tmpbufsz);
        /* Ensure 8-byte aligned CMD buffer */
        buf = (u8 *) ALIGN_ADDR(tmpbuf, DMA_ALIGNMENT);
        memcpy(buf, payload, nb);
    }
    sdio_claim_host(card->func);
#define MAX_WRITE_IOMEM_RETRY	2
    do {
        /* Transfer data to card */
        ret = sdio_writesb(card->func, priv->bt_dev.ioport, buf,
                           buf_block_len * blksz);
        if (ret < 0) {
            i++;
            PRINTM(ERROR, "BT host_to_card, write iomem (%d) failed: %d\n", i,
                   ret);
            sdio_writeb(card->func, HOST_WO_CMD53_FINISH_HOST,
                        CONFIGURATION_REG, &ret);
            udelay(20);
            ret = BT_STATUS_FAILURE;
            if (i > MAX_WRITE_IOMEM_RETRY)
                goto exit;
        } else {
            DBG_HEXDUMP(DAT_D, "BT SDIO Blk Wr", payload, nb);
            PRINTM(DATA, "BT SDIO Blk Wr %s: len=%d\n", hdev->name, nb);
        }
    } while (ret == BT_STATUS_FAILURE);
    priv->bt_dev.tx_dnld_rdy = FALSE;
  exit:
    sdio_release_host(card->func);
    if (tmpbuf)
        kfree(tmpbuf);
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
sbi_download_fw(bt_private * priv)
{
    struct sdio_mmc_card *card = priv->bt_dev.card;
    int ret = BT_STATUS_SUCCESS;
    int poll_num = MAX_FIRMWARE_POLL_TRIES;
    u8 winner = 0;

    ENTER();

    if (!card || !card->func) {
        PRINTM(ERROR, "card or function is NULL!\n");
        LEAVE();
        return BT_STATUS_FAILURE;
    }
    sdio_claim_host(card->func);
    if (BT_STATUS_SUCCESS == sd_verify_fw_download(priv, 1)) {
        PRINTM(MSG, "BT FW already downloaded!\n");
        goto done;
    }
    /* Check if other interface is downloading */
    ret = sd_check_winner_status(priv, &winner);
    if (ret == BT_STATUS_FAILURE) {
        PRINTM(FATAL, "BT read winner status failed!\n");
        goto done;
    }
    if (winner) {
        PRINTM(MSG, "BT is not the winner (0x%x). Skip FW download\n", winner);
        poll_num = MAX_MULTI_INTERFACE_POLL_TRIES;
        goto poll_fw;
    }

    /* Download the main firmware via the helper firmware */
    if (sd_download_firmware_w_helper(priv)) {
        PRINTM(INFO, "BT FW download failed!\n");
        ret = BT_STATUS_FAILURE;
        goto done;
    }
  poll_fw:
    /* check if the fimware is downloaded successfully or not */
    if (sd_verify_fw_download(priv, poll_num)) {
        PRINTM(FATAL, "BT FW failed to be active in time!\n");
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
    struct hci_dev *hdev = priv->bt_dev.hcidev;

    ENTER();

    *ireg = 0;
    OS_INT_DISABLE;
    sdio_ireg = priv->adapter->sd_ireg;
    priv->adapter->sd_ireg = 0;
    OS_INT_RESTORE;
    sdio_claim_host(card->func);
    PRINTM(INTR, "BT get_int_status %s: sdio_ireg=0x%x\n", hdev->name,
           sdio_ireg);
    priv->adapter->irq_done = sdio_ireg;
    if (sdio_ireg & DN_LD_HOST_INT_STATUS) {    /* tx_done INT */
        if (priv->bt_dev.tx_dnld_rdy) { /* tx_done already received */
            PRINTM(INFO,
                   "BT warning: tx_done already received: tx_dnld_rdy=0x%x int status=0x%x\n",
                   priv->bt_dev.tx_dnld_rdy, sdio_ireg);
        } else {
            priv->bt_dev.tx_dnld_rdy = TRUE;
        }
    }
    if (sdio_ireg & UP_LD_HOST_INT_STATUS)
        sd_card_to_host(priv);

    *ireg = sdio_ireg;
    ret = BT_STATUS_SUCCESS;
    sdio_release_host(card->func);
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
    PRINTM(CMD, "BT wake up firmware\n");

    LEAVE();
    return ret;
}

module_param(fw_name, charp, 0);
MODULE_PARM_DESC(fw_name, "Firmware name");
