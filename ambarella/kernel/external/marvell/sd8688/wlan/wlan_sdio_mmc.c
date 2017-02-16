/** @file wlan_sdio_mmc.c
 *  @brief This file contains SDIO IF (interface) module
 *  related functions.
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
/****************************************************
Change log:
	10/10/07: initial version
****************************************************/

#include	"wlan_sdio_mmc.h"

#include <linux/firmware.h>

/** define SDIO block size */
/* We support up to 480-byte block size due to FW buffer limitation. */
#define SD_BLOCK_SIZE		256

/** define allocated buffer size */
#define ALLOC_BUF_SIZE		(((MAX(MRVDRV_ETH_RX_PACKET_BUFFER_SIZE, \
					MRVDRV_SIZE_OF_CMD_BUFFER) + SDIO_HEADER_LEN \
					+ SD_BLOCK_SIZE - 1) / SD_BLOCK_SIZE) * SD_BLOCK_SIZE)

/** Max retry number of CMD53 write */
#define MAX_WRITE_IOMEM_RETRY	2

extern void pxa3xx_enable_wifi_host_sleep_pins(void);
extern void pxa3xx_wifi_wakeup(int);
/** The macros below are hardware platform dependent.
   The definition should match the actual platform */
#define GPIO_PORT_INIT()//	pxa3xx_enable_wifi_host_sleep_pins()
/** Set GPIO port to high */
#define GPIO_PORT_TO_HIGH()//	pxa3xx_wifi_wakeup(0)
/** Set GPIO port to low */
#define GPIO_PORT_TO_LOW()//	pxa3xx_wifi_wakeup(1)

/********************************************************
		Local Variables
********************************************************/

/** SDIO Rx unit */
static u8 sdio_rx_unit = 0;

/********************************************************
		Global Variables
********************************************************/

/** Chip ID for 8686 */
#define CHIP_ID_8686 0x3042
/** Chip ID for 8688 */
#define CHIP_ID_8688 0x3130
/** Chip ID for 8688R3 */
#define CHIP_ID_8688R3 0x3131
/** Chip ID for 8682 */
#define CHIP_ID_8682 0x3139
/** Null chip ID */
#define CHIP_ID_NULL 0x0000

extern u8 *helper_name;
extern u8 *fw_name;
/** Default helper name */
#define DEFAULT_HELPER_NAME "helper_sd.bin"
/** Default firmware name */
#define DEFAULT_FW_NAME "sd8688.bin"

/**Interrupt status */
static u8 sd_ireg = 0;

/********************************************************
		Local Functions
********************************************************/
/** 
 *  @brief This function get rx_unit value
 *  
 *  @param priv    A pointer to wlan_private structure
 *  @return 	   WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
int
sd_get_rx_unit(wlan_private * priv)
{
    int ret = WLAN_STATUS_SUCCESS;
    u8 reg;

    ENTER();

    ret = sbi_read_ioreg(priv, CARD_RX_UNIT_REG, &reg);
    if (ret == WLAN_STATUS_SUCCESS)
        sdio_rx_unit = reg;

    LEAVE();
    return ret;
}

/** 
 *  @brief This function reads rx length
 *  
 *  @param priv    A pointer to wlan_private structure
 *  @param dat	   A pointer to keep returned data
 *  @return 	   WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
static int
sd_read_rx_len(wlan_private * priv, u16 * dat)
{
    int ret = WLAN_STATUS_SUCCESS;
    u8 reg;

    ENTER();

    ret = sbi_read_ioreg(priv, CARD_RX_LEN_REG, &reg);
    if (ret == WLAN_STATUS_SUCCESS)
        *dat = (u16) reg << sdio_rx_unit;

    LEAVE();
    return ret;
}

/** 
 *  @brief This function reads fw status registers
 *  
 *  @param priv    A pointer to wlan_private structure
 *  @param dat	   A pointer to keep returned data
 *  @return 	   WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
static int
sd_read_firmware_status(wlan_private * priv, u16 * dat)
{
    int ret = WLAN_STATUS_SUCCESS;
    u8 fws0;
    u8 fws1;

    ENTER();

    ret = sbi_read_ioreg(priv, CARD_FW_STATUS0_REG, &fws0);
    if (ret < 0)
        return WLAN_STATUS_FAILURE;
    ret = sbi_read_ioreg(priv, CARD_FW_STATUS1_REG, &fws1);
    if (ret < 0)
        return WLAN_STATUS_FAILURE;
    *dat = (((u16) fws1) << 8) | fws0;

    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief This function polls the card status register.
 *  
 *  @param priv    	A pointer to wlan_private structure
 *  @param bits    	the bit mask
 *  @return 	   	WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
static int
mv_sdio_poll_card_status(wlan_private * priv, u8 bits)
{
    int tries;
    u8 cs;

    ENTER();

    for (tries = 0; tries < MAX_POLL_TRIES; tries++) {
        if (sbi_read_ioreg(priv, CARD_STATUS_REG, &cs) < 0)
            break;
        else if ((cs & bits) == bits) {
            LEAVE();
            return WLAN_STATUS_SUCCESS;
        }
        udelay(10);
    }

    PRINTM(WARN, "mv_sdio_poll_card_status failed, tries = %d\n", tries);

    LEAVE();
    return WLAN_STATUS_FAILURE;
}

/** 
 *  @brief This function set the sdio bus width.
 *  
 *  @param priv    	A pointer to wlan_private structure
 *  @param mode    	1--1 bit mode, 4--4 bit mode
 *  @return 	   	WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
int
sdio_set_bus_width(wlan_private * priv, u8 mode)
{
    ENTER();
    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief This function get rx len
 *  
 *  @param priv    	A pointer to wlan_private structure
 *  @param rx_len	A pointer to hold rx buf length
 *  @return 	   	WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
static int
mv_get_rx_len(wlan_private * priv, u16 * rx_len)
{
    int ret = WLAN_STATUS_SUCCESS;
    int buf_block_len;
    int blksz;
    u16 buf_len = 0;

    ENTER();

    *rx_len = 0;
    /* Read the length of data to be transferred */
    ret = sd_read_rx_len(priv, &buf_len);
    if (ret < 0) {
        PRINTM(ERROR, "mv_get_rx_len: read scratch reg failed\n");
        ret = WLAN_STATUS_FAILURE;
        goto exit;
    }
    blksz = SD_BLOCK_SIZE;
    buf_block_len = (buf_len + blksz - 1) / blksz;
    if (buf_len <= SDIO_HEADER_LEN || (buf_block_len * blksz) > ALLOC_BUF_SIZE) {
        PRINTM(ERROR, "mv_get_rx_len: invalid buf_len=%d\n", buf_len);
        ret = WLAN_STATUS_FAILURE;
        goto exit;
    }
    *rx_len = buf_block_len * blksz;
  exit:
    LEAVE();
    return ret;
}

/** 
 *  @brief This function reads data from the card.
 *  
 *  @param priv    	A pointer to wlan_private structure
 *  @param type	   	A pointer to keep type as data or command
 *  @param nb		A pointer to keep the data/cmd length returned in buffer
 *  @param payload 	A pointer to the data/cmd buffer
 *  @param npayload	the length of data/cmd buffer
 *  @return 	   	WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
static int
mv_sdio_card_to_host(wlan_private * priv,
                     u32 * type, int *nb, u8 * payload, int npayload)
{
    struct sdio_mmc_card *card = priv->wlan_dev.card;
    int ret = WLAN_STATUS_SUCCESS;
    int buf_block_len;
    int blksz;
    u32 event;

    ENTER();

    if (!payload) {
        PRINTM(WARN, "payload NULL pointer received!\n");
        ret = WLAN_STATUS_FAILURE;
        goto exit;
    }

    blksz = SD_BLOCK_SIZE;
    buf_block_len = npayload / blksz;
    if (buf_block_len * blksz > npayload) {
        PRINTM(ERROR, "card_to_host, invalid packet length: %d\n", npayload);
        ret = WLAN_STATUS_FAILURE;
        goto exit;
    }

    ret = sdio_readsb(card->func, payload, priv->wlan_dev.ioport,
                      buf_block_len * blksz);

    if (ret < 0) {
        PRINTM(ERROR, "card_to_host, read iomem failed: %d\n", ret);
        ret = WLAN_STATUS_FAILURE;
        goto exit;
    }
    *nb = wlan_le16_to_cpu(*(u16 *) & payload[0]);

    DBG_HEXDUMP(IF_D, "SDIO Blk Rd", payload, blksz * buf_block_len);

    *type = wlan_le16_to_cpu(*(u16 *) & payload[2]);
    if (*type == MV_TYPE_EVENT) {
        event = *(u32 *) & payload[4];
        priv->adapter->EventCause = wlan_le32_to_cpu(event);
    }
  exit:
    LEAVE();
    return ret;
}

/** 
 *  @brief This function enables the host interrupts mask
 *  
 *  @param priv    A pointer to wlan_private structure
 *  @param mask	   the interrupt mask
 *  @return 	   WLAN_STATUS_SUCCESS
 */
static int
enable_host_int_mask(wlan_private * priv, u8 mask)
{
    int ret = WLAN_STATUS_SUCCESS;

    ENTER();

    /* Simply write the mask to the register */
    ret = sbi_write_ioreg(priv, HOST_INT_MASK_REG, mask);

    if (ret) {
        PRINTM(WARN, "Unable to enable the host interrupt!\n");
        ret = WLAN_STATUS_FAILURE;
    }

    priv->adapter->HisRegCpy = 1;

    LEAVE();
    return ret;
}

/**  @brief This function disables the host interrupts mask.
 *  
 *  @param priv    A pointer to wlan_private structure
 *  @param mask	   the interrupt mask
 *  @return 	   WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
static int
disable_host_int_mask(wlan_private * priv, u8 mask)
{
    int ret = WLAN_STATUS_SUCCESS;
    u8 host_int_mask;

    ENTER();

    /* Read back the host_int_mask register */
    ret = sbi_read_ioreg(priv, HOST_INT_MASK_REG, &host_int_mask);
    if (ret) {
        ret = WLAN_STATUS_FAILURE;
        goto done;
    }

    /* Update with the mask and write back to the register */
    host_int_mask &= ~mask;
    ret = sbi_write_ioreg(priv, HOST_INT_MASK_REG, host_int_mask);
    if (ret < 0) {
        PRINTM(WARN, "Unable to diable the host interrupt!\n");
        ret = WLAN_STATUS_FAILURE;
        goto done;
    }

  done:
    LEAVE();
    return ret;
}

/********************************************************
		Global Functions
********************************************************/

/** 
 *  @brief This function handles the interrupt.
 *  
 *  @param func	   A pointer to sdio_func structure.
 *  @return 	   n/a
 */
static void
sbi_interrupt(struct sdio_func *func)
{
    struct sdio_mmc_card *card;
    wlan_private *priv;
    u8 ireg = 0;
    unsigned long driver_flags;
    int ret;

    ENTER();

    card = sdio_get_drvdata(func);
    if (!card || !card->priv) {
        PRINTM(INFO, "%s: sbi_interrupt(%p) card or priv is NULL, card=%p\n",
               __FUNCTION__, func, card);
        LEAVE();
        return;
    }

    priv = card->priv;
    if ((ret = sbi_read_ioreg(priv, HOST_INTSTATUS_REG, &ireg))) {
        PRINTM(WARN, "sbi_read_ioreg: read int status register failed\n");
        goto done;
    }
    if (ireg != 0) {
        /* 
         * DN_LD_HOST_INT_STATUS and/or UP_LD_HOST_INT_STATUS
         * Clear the interrupt status register and re-enable the interrupt
         */
        PRINTM(INFO, "sdio_ireg = 0x%x\n", ireg);
        sbi_write_ioreg(priv, HOST_INTSTATUS_REG,
                        ~(ireg) & (DN_LD_HOST_INT_STATUS |
                                   UP_LD_HOST_INT_STATUS));
    }
    OS_INT_DISABLE(priv, driver_flags);
    sd_ireg |= ireg;
    OS_INT_RESTORE(priv, driver_flags);
    wlan_interrupt(priv);
  done:

    LEAVE();
}

/** 
 *  @brief This function handles client driver probe.
 *  
 *  @param func	   A pointer to sdio_func structure.
 *  @param id	   A pointer to sdio_device_id structure.
 *  @return 	   WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
static int
wlan_probe(struct sdio_func *func, const struct sdio_device_id *id)
{
    int ret = WLAN_STATUS_FAILURE;
    struct sdio_mmc_card *card = NULL;

    ENTER();

    PRINTM(INFO, "%s: vendor=0x%4.04X device=0x%4.04X class=%d function=%d\n",
           __FUNCTION__, func->vendor, func->device, func->class, func->num);

    card = kzalloc(sizeof(struct sdio_mmc_card), GFP_KERNEL);
    if (!card) {
        ret = -ENOMEM;
        goto done;
    }

    card->func = func;

    if (!wlan_add_card(card)) {
        PRINTM(ERROR, "%s: wlan_add_callback failed\n", __FUNCTION__);
        kfree(card);
        ret = WLAN_STATUS_FAILURE;
        goto done;
    }

    ret = WLAN_STATUS_SUCCESS;

  done:
    LEAVE();
    return ret;
}

/** 
 *  @brief This function handles client driver remove.
 *  
 *  @param func	   A pointer to sdio_func structure.
 *  @return 	   n/a
 */
static void
wlan_remove(struct sdio_func *func)
{
    struct sdio_mmc_card *card;

    ENTER();

    PRINTM(INFO, "%s: function=%d\n", __FUNCTION__, func->num);

    if (func) {
        card = sdio_get_drvdata(func);
        if (card) {
            wlan_remove_card(card);
            kfree(card);
        }
    }

    LEAVE();
}

#ifdef CONFIG_PM
/** 
 *  @brief This function handles client driver suspend.
 *  
 *  @param func	   A pointer to sdio_func structure.
 *  @param state   suspend mode (PM_SUSPEND_xxx)
 *  @return 	   WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
int
wlan_suspend(struct sdio_func *func, pm_message_t state)
{
    int ret = WLAN_STATUS_SUCCESS;

    ENTER();

    PRINTM(INFO, "function=%d event=%#x\n", func->num, state.event);

    ret = wlan_pm(NULL, TRUE);

    LEAVE();
    return ret;
}

/** 
 *  @brief This function handles client driver resume.
 *  
 *  @param func	   A pointer to sdio_func structure.
 *  @return 	   WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
int
wlan_resume(struct sdio_func *func)
{
    int ret = WLAN_STATUS_SUCCESS;

    ENTER();

    PRINTM(INFO, "function=%d\n", func->num);

    ret = wlan_pm(NULL, FALSE);

    LEAVE();
    return ret;
}
#endif

/**Device ID for SD8688 */
#define  SD_DEVICE_ID_8688_WLAN 0x9104
/** WLAN IDs */
static const struct sdio_device_id wlan_ids[] = {
    {SDIO_DEVICE(SDIO_VENDOR_ID_MARVELL, SD_DEVICE_ID_8688_WLAN)},
    {SDIO_DEVICE_CLASS(SDIO_CLASS_WLAN)},
    {},
};

static struct sdio_driver wlan_sdio = {
    .name = "wlan_sdio",
    .id_table = wlan_ids,
    .probe = wlan_probe,
    .remove = wlan_remove,
//#ifdef CONFIG_PM
//    .suspend = wlan_suspend,
//    .resume = wlan_resume,
//#endif

};

/** 
 *  @brief This function registers the IF module in bus driver.
 *  
 *  @return 	   WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
int
sbi_register()
{
    int ret = WLAN_STATUS_SUCCESS;

    ENTER();

    /* SDIO Driver Registration */
    if (sdio_register_driver(&wlan_sdio) != 0) {
        PRINTM(FATAL, "SDIO Driver Registration Failed \n");
        ret = WLAN_STATUS_FAILURE;
    }

    LEAVE();
    return ret;
}

/** 
 *  @brief This function de-registers the IF module in bus driver.
 *  
 *  @return 	   n/a
 */
void
sbi_unregister(void)
{
    ENTER();

    /* SDIO Driver Unregistration */
    sdio_unregister_driver(&wlan_sdio);

    LEAVE();
}

/** 
 *  @brief This function reads the IO register.
 *  
 *  @param priv    A pointer to wlan_private structure
 *  @param reg	   register to be read
 *  @param dat	   A pointer to variable that keeps returned value
 *  @return 	   WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
int
sbi_read_ioreg(wlan_private * priv, u32 reg, u8 * dat)
{
    struct sdio_mmc_card *card;
    int ret = WLAN_STATUS_FAILURE;

    ENTER();

    card = priv->wlan_dev.card;
    if (!card || !card->func) {
        PRINTM(ERROR, "sbi_read_ioreg(): card or function is NULL!\n");
        goto done;
    }

    *dat = sdio_readb(card->func, reg, &ret);
    if (ret) {
        PRINTM(ERROR, "sbi_read_ioreg(): sdio_readb failed! ret=%d\n", ret);
        goto done;
    }

    PRINTM(INFO, "sbi_read_ioreg() priv=%p func=%d reg=%#x dat=%#x\n", priv,
           card->func->num, reg, *dat);

  done:
    LEAVE();
    return ret;
}

/** 
 *  @brief This function writes the IO register.
 *  
 *  @param priv    A pointer to wlan_private structure
 *  @param reg	   register to be written
 *  @param dat	   the value to be written
 *  @return 	   WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
int
sbi_write_ioreg(wlan_private * priv, u32 reg, u8 dat)
{
    struct sdio_mmc_card *card;
    int ret = WLAN_STATUS_FAILURE;

    ENTER();

    card = priv->wlan_dev.card;
    if (!card || !card->func) {
        PRINTM(ERROR, "sbi_write_ioreg(): card or function is NULL!\n");
        goto done;
    }

    PRINTM(INFO, "sbi_write_ioreg() priv=%p func=%d reg=%#x dat=%#x\n", priv,
           card->func->num, reg, dat);

    sdio_writeb(card->func, dat, reg, &ret);
    if (ret) {
        PRINTM(ERROR, "sbi_write_ioreg(): sdio_writeb failed! ret=%d\n", ret);
        goto done;
    }

  done:
    LEAVE();
    return ret;
}

/** 
 *  @brief This function checks the interrupt status and handle it accordingly.
 *  
 *  @param priv    A pointer to wlan_private structure
 *  @param ireg    A pointer to variable that keeps returned value
 *  @return 	   WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
int
sbi_get_int_status(wlan_private * priv, u8 * ireg)
{
    int ret = WLAN_STATUS_SUCCESS;
    u8 *cmdBuf;
    wlan_dev_t *wlan_dev = &priv->wlan_dev;
    struct sdio_mmc_card *card = priv->wlan_dev.card;
    struct sk_buff *skb;
    u16 rx_len;
    u8 sdio_ireg = 0;
    unsigned long driver_flags;

    ENTER();

    *ireg = 0;
    OS_INT_DISABLE(priv, driver_flags);
    sdio_ireg = sd_ireg;
    sd_ireg = 0;
    OS_INT_RESTORE(priv, driver_flags);

    sdio_claim_host(card->func);

    if (sdio_ireg & DN_LD_HOST_INT_STATUS) {    /* tx_done INT */
        *ireg |= HIS_TxDnLdRdy;

        if (!priv->wlan_dev.cmd_sent) { /* cmd_done already received */
            PRINTM(INFO, "warning: tx_done already received:"
                   " data_sent=0x%x cmd_send=0x%x int status=0x%x\n",
                   priv->wlan_dev.data_sent, priv->wlan_dev.cmd_sent,
                   sdio_ireg);
        } else {
            /* SDIO doesn't support 2 separate paths for cmd and data both will 
               be made received for now */
            priv->wlan_dev.cmd_sent = FALSE;
        }

        if (!priv->wlan_dev.data_sent) {        /* tx_done already received */
            PRINTM(INFO, "warning: tx_done already received:"
                   " data_sent=0x%x cmd_sent=0x%x int status=0x%x\n",
                   priv->wlan_dev.data_sent,
                   priv->wlan_dev.cmd_sent, sdio_ireg);
        } else {
            wmm_process_fw_iface_tx_xfer_end(priv);
            /* SDIO doesn't support 2 separate paths for cmd and data both will 
               be made received for now */
            priv->wlan_dev.data_sent = FALSE;
        }
    }

    if (sdio_ireg & UP_LD_HOST_INT_STATUS) {
        if (mv_get_rx_len(priv, &rx_len)) {
            priv->stats.rx_dropped++;
            ret = WLAN_STATUS_FAILURE;
            goto done;
        }
#ifdef PXA3XX_DMA_ALIGN
        if (!(skb = dev_alloc_skb(rx_len + PXA3XX_DMA_ALIGNMENT))) {
#else /* !PXA3XX_DMA_ALIGN */
        if (!(skb = dev_alloc_skb(rx_len))) {
#endif /* PXA3XX_DMA_ALIGN */
            PRINTM(WARN, "No free skb\n");
            priv->stats.rx_dropped++;
            ret = WLAN_STATUS_FAILURE;
            goto done;
        }
#ifdef PXA3XX_DMA_ALIGN
        if ((u32) skb->data & (PXA3XX_DMA_ALIGNMENT - 1)) {
            skb_put(skb, (u32) skb->data & (PXA3XX_DMA_ALIGNMENT - 1));
            skb_pull(skb, (u32) skb->data & (PXA3XX_DMA_ALIGNMENT - 1));
        }
#endif /* PXA3XX_DMA_ALIGN */

        /* 
         * Transfer data from card
         */
        if (mv_sdio_card_to_host(priv, &wlan_dev->upld_typ,
                                 (int *) &wlan_dev->upld_len, skb->data,
                                 rx_len) < 0) {
            u8 cr = 0;

            PRINTM(ERROR, "Card to host failed: int status=0x%x\n", sdio_ireg);
            if (sbi_read_ioreg(priv, CONFIGURATION_REG, &cr) < 0)
                PRINTM(ERROR, "read ioreg failed (CFG)\n");

            PRINTM(INFO, "Config Reg val = %d\n", cr);
            if (sbi_write_ioreg(priv, CONFIGURATION_REG, (cr | 0x04)) < 0)
                PRINTM(ERROR, "write ioreg failed (CFG)\n");

            PRINTM(INFO, "write success\n");
            if (sbi_read_ioreg(priv, CONFIGURATION_REG, &cr) < 0)
                PRINTM(ERROR, "read ioreg failed (CFG)\n");

            PRINTM(INFO, "Config reg val =%x\n", cr);
            ret = WLAN_STATUS_FAILURE;
            kfree_skb(skb);
            goto done;
        }

        switch (wlan_dev->upld_typ) {
        case MV_TYPE_DAT:
            PRINTM(DATA, "Data <= FW @ %lu\n", os_time_get());
            skb_put(skb, priv->wlan_dev.upld_len);
            skb_pull(skb, SDIO_HEADER_LEN);
            wlan_process_rx_packet(priv, skb);
            priv->adapter->IntCounter = 0;

            /* skb will be freed by kernel later */
            break;

        case MV_TYPE_CMD:
            *ireg |= HIS_CmdUpLdRdy;
            priv->adapter->HisRegCpy |= *ireg;

            /* take care of CurCmd = NULL case */
            if (!priv->adapter->CurCmd) {
                cmdBuf = priv->wlan_dev.upld_buf;
            } else {
                cmdBuf = priv->adapter->CurCmd->BufVirtualAddr;
            }

            priv->wlan_dev.upld_len -= SDIO_HEADER_LEN;
            memcpy(cmdBuf, skb->data + SDIO_HEADER_LEN,
                   MIN(MRVDRV_SIZE_OF_CMD_BUFFER, priv->wlan_dev.upld_len));
            kfree_skb(skb);
            /* wlan_process_cmdresp(priv); */
            break;

        case MV_TYPE_EVENT:
            *ireg |= HIS_CardEvent;
            priv->adapter->HisRegCpy |= *ireg;
            /* event cause has been saved to priv->adapter->EventCause */
            kfree_skb(skb);
            /* wlan_process_event(priv); */
            break;

        default:
            PRINTM(ERROR, "SDIO unknown upload type = 0x%x\n",
                   wlan_dev->upld_typ);
            kfree_skb(skb);
            break;
        }
    }

    ret = WLAN_STATUS_SUCCESS;
  done:
    sdio_release_host(card->func);
    LEAVE();
    return ret;
}

/** 
 *  @brief This function disables the host interrupts.
 *  
 *  @param priv    A pointer to wlan_private structure
 *  @return 	   WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
int
sbi_disable_host_int(wlan_private * priv)
{
    struct sdio_mmc_card *card = priv->wlan_dev.card;
    int ret;

    ENTER();

    sdio_claim_host(card->func);
    ret = disable_host_int_mask(priv, HIM_DISABLE);
    sdio_release_host(card->func);

    LEAVE();
    return ret;
}

/** 
 *  @brief This function enables the host interrupts.
 *  
 *  @param priv    A pointer to wlan_private structure
 *  @return 	   WLAN_STATUS_SUCCESS
 */
int
sbi_enable_host_int(wlan_private * priv)
{
    struct sdio_mmc_card *card = priv->wlan_dev.card;
    int ret;

    ENTER();

    sdio_claim_host(card->func);
    ret = enable_host_int_mask(priv, HIM_ENABLE);
    sdio_release_host(card->func);

    LEAVE();
    return ret;
}

/** 
 *  @brief This function de-registers the device.
 *  
 *  @param priv    A pointer to wlan_private structure
 *  @return 	   WLAN_STATUS_SUCCESS
 */
int
sbi_unregister_dev(wlan_private * priv)
{
    struct sdio_mmc_card *card = priv->wlan_dev.card;

    ENTER();

    if (!card || !card->func) {
        PRINTM(ERROR, "Error: card or function is NULL!\n");
        goto done;
    }

    sdio_claim_host(card->func);
    sdio_release_irq(card->func);
    sdio_disable_func(card->func);
    sdio_release_host(card->func);

    sdio_set_drvdata(card->func, NULL);

    GPIO_PORT_TO_LOW();

  done:
    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief This function registers the device.
 *  
 *  @param priv    A pointer to wlan_private structure
 *  @return 	   WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
int
sbi_register_dev(wlan_private * priv)
{
    int ret = WLAN_STATUS_FAILURE;
    u8 reg;
    struct sdio_mmc_card *card = priv->wlan_dev.card;
    struct sdio_func *func;

    ENTER();

    if (!card || !card->func) {
        PRINTM(ERROR, "Error: card or function is NULL!\n");
        goto done;
    }

    func = card->func;

    GPIO_PORT_INIT();
    GPIO_PORT_TO_HIGH();

    /* Initialize the private structure */
    priv->wlan_dev.ioport = 0;
    priv->wlan_dev.upld_typ = 0;
    priv->wlan_dev.upld_len = 0;

    sdio_claim_host(func);

    ret = sdio_enable_func(func);
    if (ret) {
        PRINTM(FATAL, "sdio_enable_func() failed: ret=%d\n", ret);
        goto release_host;
    }

    ret = sdio_claim_irq(func, sbi_interrupt);
    if (ret) {
        PRINTM(FATAL, "sdio_claim_irq failed: ret=%d\n", ret);
        goto disable_func;
    }

    /* Read the IO port */
    ret = sbi_read_ioreg(priv, IO_PORT_0_REG, &reg);
    if (ret)
        goto release_irq;
    else
        priv->wlan_dev.ioport |= reg;

    ret = sbi_read_ioreg(priv, IO_PORT_1_REG, &reg);
    if (ret)
        goto release_irq;
    else
        priv->wlan_dev.ioport |= (reg << 8);

    ret = sbi_read_ioreg(priv, IO_PORT_2_REG, &reg);
    if (ret)
        goto release_irq;
    else
        priv->wlan_dev.ioport |= (reg << 16);

    PRINTM(INFO, "SDIO FUNC #%d IO port: 0x%x\n", func->num,
           priv->wlan_dev.ioport);

    ret = sdio_set_block_size(card->func, SD_BLOCK_SIZE);
    if (ret) {
        PRINTM(ERROR, "%s: cannot set SDIO block size\n", __FUNCTION__);
        ret = WLAN_STATUS_FAILURE;
        goto release_irq;
    }

    priv->hotplug_device = &func->dev;
    if (helper_name == NULL) {
        helper_name = DEFAULT_HELPER_NAME;
    }
    if (fw_name == NULL) {
        fw_name = DEFAULT_FW_NAME;
    }

    sdio_release_host(func);

    sdio_set_drvdata(func, card);

    ret = WLAN_STATUS_SUCCESS;
    goto done;

  release_irq:
    sdio_release_irq(func);
  disable_func:
    sdio_disable_func(func);
  release_host:
    sdio_release_host(func);

  done:
    LEAVE();
    return ret;
}

/** 
 *  @brief This function sends data to the card.
 *  
 *  @param priv    A pointer to wlan_private structure
 *  @param type	   data or command
 *  @param payload A pointer to the data/cmd buffer
 *  @param nb	   the length of data/cmd
 *  @return 	   WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
int
sbi_host_to_card(wlan_private * priv, u8 type, u8 * payload, u16 nb)
{
    struct sdio_mmc_card *card = priv->wlan_dev.card;
    int ret = WLAN_STATUS_SUCCESS;
    int buf_block_len;
    int blksz;
    int i = 0;
    void *tmpcmdbuf = NULL;
    int tmpcmdbufsz;
    u8 *cmdbuf = NULL;

    ENTER();

    if (!card || !card->func) {
        PRINTM(ERROR, "card or function is NULL!\n");
        ret = WLAN_STATUS_FAILURE;
        goto exit;
    }

    priv->adapter->HisRegCpy &= ~HIS_TxDnLdRdy;

    /* Allocate buffer and copy payload */
    blksz = SD_BLOCK_SIZE;
    buf_block_len = (nb + SDIO_HEADER_LEN + blksz - 1) / blksz;

    /* 
     * This is SDIO specific header
     *  u16 length,
     *  u16 type (MV_TYPE_DAT = 0, MV_TYPE_CMD = 1, MV_TYPE_EVENT = 3, MV_TYPE_DIAG = 6) 
     */
    if (type == MV_TYPE_DAT) {
        *(u16 *) & payload[0] = wlan_cpu_to_le16(nb + SDIO_HEADER_LEN);
        *(u16 *) & payload[2] = wlan_cpu_to_le16(type);
    } else if (type == MV_TYPE_CMD) {
#ifdef PXA3XX_DMA_ALIGN
        tmpcmdbufsz = ALIGN_SZ(WLAN_UPLD_SIZE, PXA3XX_DMA_ALIGNMENT);
#else /* !PXA3XX_DMA_ALIGN */
        tmpcmdbufsz = WLAN_UPLD_SIZE;
#endif /* !PXA3XX_DMA_ALIGN */
        tmpcmdbuf = kmalloc(tmpcmdbufsz, GFP_KERNEL);
        if (!tmpcmdbuf) {
            PRINTM(ERROR, "Unable to allocate buffer for CMD.\n");
            ret = WLAN_STATUS_FAILURE;
            goto exit;
        }
        memset(tmpcmdbuf, 0, tmpcmdbufsz);
#ifdef PXA3XX_DMA_ALIGN
        /* Ensure 8-byte aligned CMD buffer */
        cmdbuf = (u8 *) ALIGN_ADDR(tmpcmdbuf, PXA3XX_DMA_ALIGNMENT);
#else /* !PXA3XX_DMA_ALIGN */
        cmdbuf = (u8 *) tmpcmdbuf;
#endif /* !PXA3XX_DMA_ALIGN */

        *(u16 *) & cmdbuf[0] = wlan_cpu_to_le16(nb + SDIO_HEADER_LEN);
        *(u16 *) & cmdbuf[2] = wlan_cpu_to_le16(type);

        if (payload != NULL &&
            (nb > 0 && nb <= (WLAN_UPLD_SIZE - SDIO_HEADER_LEN))) {

            memcpy(&cmdbuf[SDIO_HEADER_LEN], payload, nb);
        } else {
            PRINTM(WARN, "sbi_host_to_card(): Error: payload=%p, nb=%d\n",
                   payload, nb);
        }
    }

    sdio_claim_host(card->func);

    /* We have to set both as SDIO only supports one path for both cmd and data 
     */
    priv->wlan_dev.cmd_sent = TRUE;
    priv->wlan_dev.data_sent = TRUE;

    do {

        /* Transfer data to card */
        ret = sdio_writesb(card->func, priv->wlan_dev.ioport,
                           (type == MV_TYPE_DAT) ? payload : cmdbuf,
                           buf_block_len * blksz);
        if (ret < 0) {
            i++;

            PRINTM(ERROR, "host_to_card, write iomem (%d) failed: %d\n", i,
                   ret);
            if (sbi_write_ioreg(priv, CONFIGURATION_REG, 0x04) < 0) {
                PRINTM(ERROR, "write ioreg failed (CFG)\n");
            }
            ret = WLAN_STATUS_FAILURE;
            if (i > MAX_WRITE_IOMEM_RETRY) {
                /* We have to set both as SDIO only supports one path for both
                   cmd and data */
                priv->wlan_dev.cmd_sent = FALSE;
                priv->wlan_dev.data_sent = FALSE;

                goto exit;
            }
        } else {
            DBG_HEXDUMP(IF_D, "SDIO Blk Wr",
                        (type == MV_TYPE_DAT) ? payload : cmdbuf,
                        blksz * buf_block_len);
        }
    } while (ret == WLAN_STATUS_FAILURE);

  exit:
    sdio_release_host(card->func);
    if (tmpcmdbuf)
        kfree(tmpcmdbuf);

    LEAVE();
    return ret;
}

/** 
 *  @brief This function reads CIS information.
 *  
 *  @param priv    A pointer to wlan_private structure
 *  @param cisinfo A pointer to CIS information output buffer
 *  @param cislen  A pointer to the length of CIS info output buffer
 *  @return 	   WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
int
sbi_get_cis_info(wlan_private * priv, void *cisinfo, int *cislen)
{
#define CIS_PTR (0x8000)
    struct sdio_mmc_card *card = priv->wlan_dev.card;
    unsigned int i, cis_ptr = CIS_PTR;
    int ret = WLAN_STATUS_FAILURE;

    ENTER();

    if (!card || !card->func) {
        PRINTM(ERROR, "sbi_get_cis_info(): card or function is NULL!\n");
        goto exit;
    }
#define MAX_SDIO_CIS_INFO_LEN (256)
    if (!cisinfo || (*cislen < MAX_SDIO_CIS_INFO_LEN)) {
        PRINTM(WARN, "ERROR! get_cis_info: insufficient buffer passed\n");
        goto exit;
    }

    *cislen = MAX_SDIO_CIS_INFO_LEN;

    sdio_claim_host(card->func);

    PRINTM(INFO, "cis_ptr=%#x\n", cis_ptr);

    /* Read the Tuple Data */
    for (i = 0; i < *cislen; i++) {
        ((unsigned char *) cisinfo)[i] =
            sdio_readb(card->func, cis_ptr + i, &ret);
        if (ret) {
            PRINTM(WARN, "get_cis_info error: ret=%d\n", ret);
            ret = WLAN_STATUS_FAILURE;
            goto done;
        }
        PRINTM(INFO, "cisinfo[%d]=%#x\n", i, ((unsigned char *) cisinfo)[i]);
    }

  done:
    sdio_release_host(card->func);
  exit:
    LEAVE();
    return ret;
}

/** 
 *  @brief This function downloads helper image to the card.
 *  
 *  @param priv    	A pointer to wlan_private structure
 *  @return 	   	WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
int
sbi_prog_helper(wlan_private * priv)
{
    struct sdio_mmc_card *card = priv->wlan_dev.card;
    u8 *helper = NULL;
    int helperlen;
    int ret = WLAN_STATUS_SUCCESS;
    void *tmphlprbuf = NULL;
    int tmphlprbufsz;
    u8 *hlprbuf;
    int hlprblknow;
    u32 tx_len;

    ENTER();

    if (!card || !card->func) {
        PRINTM(ERROR, "sbi_prog_helper(): card or function is NULL!\n");
        goto done;
    }

    if (priv->fw_helper) {
        helper = (u8 *)priv->fw_helper->data;
        helperlen = priv->fw_helper->size;
    } else {
        PRINTM(MSG, "No helper image found! Terminating download.\n");
        LEAVE();
        return WLAN_STATUS_FAILURE;
    }

    PRINTM(INFO, "Downloading helper image (%d bytes), block size %d bytes\n",
           helperlen, SD_BLOCK_SIZE);

#ifdef PXA3XX_DMA_ALIGN
    tmphlprbufsz = ALIGN_SZ(WLAN_UPLD_SIZE, PXA3XX_DMA_ALIGNMENT);
#else /* !PXA3XX_DMA_ALIGN */
    tmphlprbufsz = WLAN_UPLD_SIZE;
#endif /* !PXA3XX_DMA_ALIGN */
    tmphlprbuf = kmalloc(tmphlprbufsz, GFP_KERNEL);
    if (!tmphlprbuf) {
        PRINTM(ERROR,
               "Unable to allocate buffer for helper. Terminating download\n");
        ret = WLAN_STATUS_FAILURE;
        goto done;
    }
    memset(tmphlprbuf, 0, tmphlprbufsz);
#ifdef PXA3XX_DMA_ALIGN
    hlprbuf = (u8 *) ALIGN_ADDR(tmphlprbuf, PXA3XX_DMA_ALIGNMENT);
#else /* !PXA3XX_DMA_ALIGN */
    hlprbuf = (u8 *) tmphlprbuf;
#endif /* !PXA3XX_DMA_ALIGN */

    sdio_claim_host(card->func);

    /* Perform helper data transfer */
    tx_len = (FIRMWARE_TRANSFER_NBLOCK * SD_BLOCK_SIZE) - SDIO_HEADER_LEN;
    hlprblknow = 0;
    do {
        /* The host polls for the DN_LD_CARD_RDY and CARD_IO_READY bits */
        ret = mv_sdio_poll_card_status(priv, CARD_IO_READY | DN_LD_CARD_RDY);
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

        /* Set length to the 4-byte header */
        *(u32 *) hlprbuf = wlan_cpu_to_le32(tx_len);

        /* Copy payload to buffer */
        memcpy(&hlprbuf[SDIO_HEADER_LEN], &helper[hlprblknow], tx_len);

        PRINTM(INFO, ".");

        /* Send data */
        ret = sdio_writesb(card->func, priv->wlan_dev.ioport,
                           hlprbuf, FIRMWARE_TRANSFER_NBLOCK * SD_BLOCK_SIZE);

        if (ret < 0) {
            PRINTM(FATAL, "IO error during helper download @ %d\n", hlprblknow);
            goto done;
        }

        hlprblknow += tx_len;
    } while (TRUE);

    /* Write last EOF data */
    PRINTM(INFO, "\nTransferring helper image EOF block\n");
    memset(hlprbuf, 0x0, SD_BLOCK_SIZE);
    ret = sdio_writesb(card->func, priv->wlan_dev.ioport,
                       hlprbuf, SD_BLOCK_SIZE);

    if (ret < 0) {
        PRINTM(FATAL, "IO error in writing helper image EOF block\n");
        goto done;
    }

    ret = WLAN_STATUS_SUCCESS;

  done:
    sdio_release_host(card->func);
    if (tmphlprbuf)
        kfree(tmphlprbuf);

    LEAVE();
    return ret;
}

/** 
 *  @brief This function downloads firmware image to the card.
 *  
 *  @param priv    	A pointer to wlan_private structure
 *  @return 	   	WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
int
sbi_prog_fw_w_helper(wlan_private * priv)
{
    struct sdio_mmc_card *card = priv->wlan_dev.card;
    u8 *firmware = NULL;
    int firmwarelen;
    u8 base0;
    u8 base1;
    int ret = WLAN_STATUS_SUCCESS;
    int offset;
    void *tmpfwbuf = NULL;
    int tmpfwbufsz;
    u8 *fwbuf;
    u16 len;
    int txlen = 0;
    int tx_blocks = 0;
    int i = 0;
    int tries = 0;

    ENTER();

    if (!card || !card->func) {
        PRINTM(ERROR, "sbi_prog_fw_w_helper(): card or function is NULL!\n");
        goto done;
    }

    if (priv->firmware) {
        firmware = (u8 *)priv->firmware->data;
        firmwarelen = priv->firmware->size;
    } else {
        PRINTM(MSG, "No firmware image found! Terminating download.\n");
        LEAVE();
        return WLAN_STATUS_FAILURE;
    }

    PRINTM(INFO, "Downloading FW image (%d bytes)\n", firmwarelen);

#ifdef PXA3XX_DMA_ALIGN
    tmpfwbufsz = ALIGN_SZ(WLAN_UPLD_SIZE, PXA3XX_DMA_ALIGNMENT);
#else /* PXA3XX_DMA_ALIGN */
    tmpfwbufsz = WLAN_UPLD_SIZE;
#endif /* PXA3XX_DMA_ALIGN */
    tmpfwbuf = kmalloc(tmpfwbufsz, GFP_KERNEL);
    if (!tmpfwbuf) {
        PRINTM(ERROR,
               "Unable to allocate buffer for firmware. Terminating download.\n");
        ret = WLAN_STATUS_FAILURE;
        goto done;
    }
    memset(tmpfwbuf, 0, tmpfwbufsz);
#ifdef PXA3XX_DMA_ALIGN
    /* Ensure 8-byte aligned firmware buffer */
    fwbuf = (u8 *) ALIGN_ADDR(tmpfwbuf, PXA3XX_DMA_ALIGNMENT);
#else /* PXA3XX_DMA_ALIGN */
    fwbuf = (u8 *) tmpfwbuf;
#endif /* PXA3XX_DMA_ALIGN */

    sdio_claim_host(card->func);

    /* Perform firmware data transfer */
    offset = 0;
    do {
        /* The host polls for the DN_LD_CARD_RDY and CARD_IO_READY bits */
        ret = mv_sdio_poll_card_status(priv, CARD_IO_READY | DN_LD_CARD_RDY);
        if (ret < 0) {
            PRINTM(FATAL, "FW download with helper poll status timeout @ %d\n",
                   offset);
            goto done;
        }

        /* More data? */
        if (offset >= firmwarelen)
            break;

        for (tries = 0; tries < MAX_POLL_TRIES; tries++) {
            if ((ret = sbi_read_ioreg(priv, HOST_F1_RD_BASE_0, &base0)) < 0) {
                PRINTM(WARN, "Dev BASE0 register read failed:"
                       " base0=0x%04X(%d). Terminating download.\n", base0,
                       base0);
                ret = WLAN_STATUS_FAILURE;
                goto done;
            }
            if ((ret = sbi_read_ioreg(priv, HOST_F1_RD_BASE_1, &base1)) < 0) {
                PRINTM(WARN, "Dev BASE1 register read failed:"
                       " base1=0x%04X(%d). Terminating download.\n", base1,
                       base1);
                ret = WLAN_STATUS_FAILURE;
                goto done;
            }
            len = (((u16) base1) << 8) | base0;

            if (len != 0)
                break;
            udelay(10);
        }

        if (len == 0)
            break;
        else if (len > WLAN_UPLD_SIZE) {
            PRINTM(FATAL, "FW download failure @ %d, invalid length %d\n",
                   offset, len);
            ret = WLAN_STATUS_FAILURE;
            goto done;
        }

        txlen = len;

        if (len & BIT(0)) {
            i++;
            if (i > MAX_WRITE_IOMEM_RETRY) {
                PRINTM(FATAL,
                       "FW download failure @ %d, over max retry count\n",
                       offset);
                ret = WLAN_STATUS_FAILURE;
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
            if (firmwarelen - offset < txlen) {
                txlen = firmwarelen - offset;
            }
            PRINTM(INFO, ".");

            tx_blocks = (txlen + SD_BLOCK_SIZE - 1) / SD_BLOCK_SIZE;

            /* Copy payload to buffer */
            memcpy(fwbuf, &firmware[offset], txlen);
        }

        /* Send data */
        ret = sdio_writesb(card->func, priv->wlan_dev.ioport,
                           fwbuf, tx_blocks * SD_BLOCK_SIZE);

        if (ret < 0) {
            PRINTM(ERROR, "FW download, write iomem (%d) failed @ %d\n", i,
                   offset);
            if (sbi_write_ioreg(priv, CONFIGURATION_REG, 0x04) < 0) {
                PRINTM(ERROR, "write ioreg failed (CFG)\n");
            }
        }

        offset += txlen;
    } while (TRUE);

    PRINTM(INFO, "\nFW download over, size %d bytes\n", offset);

    ret = WLAN_STATUS_SUCCESS;
  done:
    sdio_release_host(card->func);
    if (tmpfwbuf)
        kfree(tmpfwbuf);

    LEAVE();
    return ret;
}

/** 
 *  @brief This function checks if the firmware is ready to accept
 *  command or not.
 *  
 *  @param priv    A pointer to wlan_private structure
 *  @param pollnum Poll number 
 *  @return 	   WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
int
sbi_check_fw_status(wlan_private * priv, int pollnum)
{
    struct sdio_mmc_card *card = priv->wlan_dev.card;
    int ret = WLAN_STATUS_SUCCESS;
    u16 firmwarestat;
    int tries;

    ENTER();

    sdio_claim_host(card->func);

    /* Wait for firmware initialization event */
    for (tries = 0; tries < pollnum; tries++) {
        if ((ret = sd_read_firmware_status(priv, &firmwarestat)) < 0)
            continue;

        if (firmwarestat == FIRMWARE_READY) {
            ret = WLAN_STATUS_SUCCESS;
            break;
        } else {
            mdelay(10);
            ret = WLAN_STATUS_FAILURE;
        }
    }

    if (ret < 0)
        goto done;

    ret = WLAN_STATUS_SUCCESS;
    sd_get_rx_unit(priv);
  done:
    sdio_release_host(card->func);

    LEAVE();
    return ret;
}

/** 
 *  @brief This function set bus clock on/off
 *  
 *  @param priv    A pointer to wlan_private structure
 *  @param option    TRUE--on , FALSE--off
 *  @return 	   WLAN_STATUS_SUCCESS
 */
int
sbi_set_bus_clock(wlan_private * priv, u8 option)
{
    ENTER();
    LEAVE();
    return WLAN_STATUS_SUCCESS;
}

/** 
 *  @brief This function makes firmware exiting from deep sleep.
 *  
 *  @param priv    A pointer to wlan_private structure
 *  @return 	   WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
int
sbi_exit_deep_sleep(wlan_private * priv)
{
    struct sdio_mmc_card *card = priv->wlan_dev.card;
    int ret = WLAN_STATUS_SUCCESS;

    ENTER();

    sdio_claim_host(card->func);

    sbi_set_bus_clock(priv, TRUE);

    if (priv->adapter->fwWakeupMethod == WAKEUP_FW_THRU_GPIO) {
        GPIO_PORT_TO_LOW();
    } else
        ret = sbi_write_ioreg(priv, CONFIGURATION_REG, HOST_POWER_UP);

    sdio_release_host(card->func);

    LEAVE();
    return ret;
}

/** 
 *  @brief This function resets the setting of deep sleep.
 *  
 *  @param priv    A pointer to wlan_private structure
 *  @return 	   WLAN_STATUS_SUCCESS or WLAN_STATUS_FAILURE
 */
int
sbi_reset_deepsleep_wakeup(wlan_private * priv)
{
    struct sdio_mmc_card *card = priv->wlan_dev.card;
    int ret = WLAN_STATUS_SUCCESS;

    ENTER();

    sdio_claim_host(card->func);

    if (priv->adapter->fwWakeupMethod == WAKEUP_FW_THRU_GPIO) {
        GPIO_PORT_TO_HIGH();
    } else
        ret = sbi_write_ioreg(priv, CONFIGURATION_REG, 0);

    sdio_release_host(card->func);

    LEAVE();
    return ret;
}
