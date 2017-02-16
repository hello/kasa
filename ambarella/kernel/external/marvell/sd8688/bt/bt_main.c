/** @file bt_main.c
  *  
  * @brief This file contains the major functions in BlueTooth
  * driver. It includes init, exit, open, close and main
  * thread etc..
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
/**
  * @mainpage M-BT Linux Driver
  *
  * @section overview_sec Overview
  *
  * The M-BT is a Linux reference driver for Marvell SDIO Bluetooth chipset.
  * 
  * @section copyright_sec Copyright
  *
  * Copyright (C) 2007-2008, Marvell International Ltd.
  *
  */
#include "include.h"

/** Version */
#define VERSION "1.34"

/** Driver version */
char driver_version[] = "sd8688-" VERSION "-(" "FP" FPNUM ")" " ";

/** Bluetooth device private structure */
bt_private *btpriv = NULL;
/** Firmware flag */
int fw = 1;

#ifdef	DEBUG_LEVEL1
u32 drvdbg = DBG_MSG | DBG_FATAL | DBG_ERROR;
#endif

/** Disable interrupt */
#define OS_INT_DISABLE	spin_lock_irqsave(&priv->driver_lock, priv->driver_flags)
/** Enable interrupt */
#define	OS_INT_RESTORE	spin_unlock_irqrestore(&priv->driver_lock, priv->driver_flags); \
			priv->driver_lock = SPIN_LOCK_UNLOCKED

/** 
 *  @brief This function verify the received event pkt
 *  
 *  @param priv    A pointer to bt_private structure
 *  @param skb     A pointer to rx skb 
 *  @return 	   N/A
 */
void
check_evtpkt(bt_private * priv, struct sk_buff *skb)
{
    struct hci_event_hdr *hdr = (struct hci_event_hdr *) skb->data;
    struct hci_ev_cmd_complete *ec;
    u16 opcode, ocf;
    ENTER();
    if (hdr->evt == HCI_EV_CMD_COMPLETE) {
        ec = (struct hci_ev_cmd_complete *) (skb->data + HCI_EVENT_HDR_SIZE);
        opcode = __le16_to_cpu(ec->opcode);
        ocf = hci_opcode_ocf(opcode);
        if ((ocf == BT_CMD_MODULE_CFG_REQ) &&
            (priv->bt_dev.sendcmdflag == TRUE)) {
            priv->bt_dev.sendcmdflag = FALSE;
            priv->adapter->cmd_complete = TRUE;
            wake_up_interruptible(&priv->adapter->cmd_wait_q);
        }
    }
    LEAVE();
    return;
}

/** 
 *  @brief This function verify the received acl loopback pkt
 *  
 *  @param priv    A pointer to bt_private structure
 *  @param skb     A pointer to rx skb 
 *  @return 	   BT_STATUS_SUCCESS or BT_STATUS_FAILURE
 */
int
bt_process_event(bt_private * priv, struct sk_buff *skb)
{
    u8 ret = BT_STATUS_SUCCESS;
    BT_EVENT *pEvent;
    ENTER();
    pEvent = (BT_EVENT *) skb->data;
    if (pEvent->EC != 0xff) {
        PRINTM(CMD, "Not Marvell Event=%x\n", pEvent->EC);
        ret = BT_STATUS_FAILURE;
        goto exit;
    }
    switch (pEvent->data[0]) {
    case BT_CMD_AUTO_SLEEP_MODE:
        if (pEvent->data[2] == BT_STATUS_SUCCESS) {
            if (pEvent->data[1] == BT_PS_ENABLE)
                priv->adapter->psmode = 1;
            else
                priv->adapter->psmode = 0;
            PRINTM(CMD, "PS Mode:%s\n",
                   (priv->adapter->psmode) ? "Enable" : "Disable");
        } else
            PRINTM(CMD, "PS Mode Command Fail\n");
        break;
    case BT_CMD_HOST_SLEEP_CONFIG:
        if (pEvent->data[3] == BT_STATUS_SUCCESS) {
            PRINTM(CMD, "gpio=%x,gap=%x\n", pEvent->data[1], pEvent->data[2]);
        } else
            PRINTM(CMD, "HSCFG Command Fail\n");
        break;
    case BT_CMD_HOST_SLEEP_ENABLE:
        if (pEvent->data[1] == BT_STATUS_SUCCESS) {
            priv->adapter->hs_state = HS_ACTIVATED;
            if (priv->adapter->psmode)
                priv->adapter->ps_state = PS_SLEEP;
            wake_up_interruptible(&priv->adapter->cmd_wait_q);
            PRINTM(CMD, "HS ACTIVATED!\n");
        } else
            PRINTM(CMD, "HS Enable Fail\n");
        break;
    case BT_CMD_MODULE_CFG_REQ:
        if ((priv->bt_dev.sendcmdflag == TRUE) &&
            ((pEvent->data[1] == MODULE_BRINGUP_REQ)
             || (pEvent->data[1] == MODULE_SHUTDOWN_REQ))) {
            if (pEvent->data[1] == MODULE_BRINGUP_REQ) {
                PRINTM(CMD, "EVENT:%s\n",
                       (pEvent->
                        data[2]) ? "Bring up Fail" : "Bring up success");
                priv->bt_dev.devType = pEvent->data[3];
                PRINTM(CMD, "devType:%s\n",
                       (pEvent->data[3] ==
                        1) ? "AMP controller" : "BR/EDR controller");
            }
            if (pEvent->data[1] == MODULE_SHUTDOWN_REQ) {
                PRINTM(CMD, "EVENT:%s\n",
                       (pEvent->
                        data[2]) ? "Shut down Fail" : "Shut down success");
            }
        } else {
            PRINTM(CMD, "BT_CMD_MODULE_CFG_REQ resp for APP\n");
            ret = BT_STATUS_FAILURE;
        }
        break;
    case BT_EVENT_POWER_STATE:
        if (pEvent->data[1] == BT_PS_SLEEP)
            priv->adapter->ps_state = PS_SLEEP;
        PRINTM(CMD, "EVENT:%s\n",
               (priv->adapter->ps_state) ? "PS_SLEEP" : "PS_AWAKE");
        break;
    default:
        PRINTM(CMD, "Unknown Event=%d\n", pEvent->data[0]);
        ret = BT_STATUS_FAILURE;
        break;
    }
  exit:
    if (ret == BT_STATUS_SUCCESS)
        kfree_skb(skb);
    LEAVE();
    return ret;
}

/** 
 *  @brief This function send module cfg cmd to firmware
 *  
 *  @param priv    A pointer to bt_private structure
 *  @param subcmd  sub command 
 *  @return 	   BT_STATUS_SUCCESS or BT_STATUS_FAILURE
 */
int
bt_send_module_cfg_cmd(bt_private * priv, int subcmd)
{
    struct sk_buff *skb = NULL;
    u8 ret = BT_STATUS_SUCCESS;
    BT_CMD *pCmd;
    ENTER();
    skb = bt_skb_alloc(sizeof(BT_CMD), GFP_ATOMIC);
    if (skb == NULL) {
        PRINTM(WARN, "No free skb\n");
        ret = BT_STATUS_FAILURE;
        goto exit;
    }
    pCmd = (BT_CMD *) skb->tail;
    pCmd->ocf_ogf = (OGF << 10) | BT_CMD_MODULE_CFG_REQ;
    pCmd->length = 1;
    pCmd->data[0] = subcmd;
    bt_cb(skb)->pkt_type = MRVL_VENDOR_PKT;
    skb_put(skb, sizeof(BT_CMD));
    skb->dev = (void *) priv->bt_dev.hcidev;
    skb_queue_head(&priv->adapter->tx_queue, skb);
    priv->bt_dev.sendcmdflag = TRUE;
    priv->adapter->cmd_complete = FALSE;
    PRINTM(CMD, "Queue module cfg Command\n");
    wake_up_interruptible(&priv->MainThread.waitQ);
    if (!os_wait_interruptible_timeout
        (priv->adapter->cmd_wait_q, priv->adapter->cmd_complete,
         WAIT_UNTIL_CMD_RESP)) {
        ret = BT_STATUS_FAILURE;
        PRINTM(MSG, "module_cfg_cmd(%x): timeout: %d\n", subcmd,
               priv->bt_dev.sendcmdflag);
    }
    PRINTM(CMD, "module cfg Command done\n");
  exit:
    LEAVE();
    return ret;
}

/** 
 *  @brief This function enable host sleep 
 *  
 *  @param priv    A pointer to bt_private structure
 *  @return 	   BT_STATUS_SUCCESS or BT_STATUS_FAILURE
 */
int
bt_enable_hs(bt_private * priv)
{
    struct sk_buff *skb = NULL;
    u8 ret = BT_STATUS_SUCCESS;
    BT_CMD *pCmd;
    ENTER();
    skb = bt_skb_alloc(sizeof(BT_CMD), GFP_ATOMIC);
    if (skb == NULL) {
        PRINTM(WARN, "No free skb\n");
        ret = BT_STATUS_FAILURE;
        goto exit;
    }
    pCmd = (BT_CMD *) skb->tail;
    pCmd->ocf_ogf = (OGF << 10) | BT_CMD_HOST_SLEEP_ENABLE;
    pCmd->length = 0;
    bt_cb(skb)->pkt_type = MRVL_VENDOR_PKT;
    skb_put(skb, sizeof(BT_CMD));
    skb->dev = (void *) priv->bt_dev.hcidev;
    skb_queue_head(&priv->adapter->tx_queue, skb);
    PRINTM(CMD, "Queue hs enable Command\n");
    wake_up_interruptible(&priv->MainThread.waitQ);
    if (!os_wait_interruptible_timeout
        (priv->adapter->cmd_wait_q, priv->adapter->hs_state,
         WAIT_UNTIL_HS_STATE_CHANGED)) {
        ret = BT_STATUS_FAILURE;
        PRINTM(MSG, "bt_enable_hs: timeout: %d, %d,%d\n",
               priv->adapter->hs_state, priv->adapter->ps_state,
               priv->adapter->WakeupTries);
    }
  exit:
    LEAVE();
    return ret;
}

/** 
 *  @brief This function used to send command to firmware
 *  
 *  @param priv    A pointer to bt_private structure
 *  @return        BT_STATUS_SUCCESS
 */
int
bt_prepare_command(bt_private * priv)
{
    struct sk_buff *skb = NULL;
    u8 ret = BT_STATUS_SUCCESS;
    BT_CMD *pCmd;
    ENTER();
    if (priv->bt_dev.hscfgcmd) {
        priv->bt_dev.hscfgcmd = 0;
        skb = bt_skb_alloc(sizeof(BT_CMD), GFP_ATOMIC);
        if (skb == NULL) {
            PRINTM(WARN, "No free skb\n");
            ret = BT_STATUS_FAILURE;
            goto exit;
        }
        pCmd = (BT_CMD *) skb->tail;
        pCmd->ocf_ogf = (OGF << 10) | BT_CMD_HOST_SLEEP_CONFIG;
        pCmd->length = 2;
        pCmd->data[0] = (priv->bt_dev.gpio_gap & 0xff00) >> 8;
        pCmd->data[1] = (u8) (priv->bt_dev.gpio_gap & 0x00ff);
        bt_cb(skb)->pkt_type = MRVL_VENDOR_PKT;
        skb_put(skb, sizeof(BT_CMD));
        skb->dev = (void *) priv->bt_dev.hcidev;
        skb_queue_head(&priv->adapter->tx_queue, skb);
        PRINTM(CMD, "Queue HSCFG Command,gpio=%x,gap=%x\n", pCmd->data[0],
               pCmd->data[1]);
    }
    if (priv->bt_dev.pscmd) {
        priv->bt_dev.pscmd = 0;
        skb = bt_skb_alloc(sizeof(BT_CMD), GFP_ATOMIC);
        if (skb == NULL) {
            PRINTM(WARN, "No free skb\n");
            ret = BT_STATUS_FAILURE;
            goto exit;
        }
        pCmd = (BT_CMD *) skb->tail;
        pCmd->ocf_ogf = (OGF << 10) | BT_CMD_AUTO_SLEEP_MODE;
        pCmd->length = 1;
        if (priv->bt_dev.psmode)
            pCmd->data[0] = BT_PS_ENABLE;
        else
            pCmd->data[0] = BT_PS_DISABLE;
        bt_cb(skb)->pkt_type = MRVL_VENDOR_PKT;
        skb_put(skb, sizeof(BT_CMD));
        skb->dev = (void *) priv->bt_dev.hcidev;
        skb_queue_head(&priv->adapter->tx_queue, skb);
        PRINTM(CMD, "Queue PSMODE Command:%d\n", pCmd->data[0]);
    }
    if (priv->bt_dev.hscmd) {
        priv->bt_dev.hscmd = 0;
        if (priv->bt_dev.hsmode)
            ret = bt_enable_hs(priv);
        else {
            ret = sbi_wakeup_firmware(priv);
            priv->adapter->hs_state = HS_DEACTIVATED;
        }
    }
  exit:
    LEAVE();
    return ret;
}

/**  @brief This function processes a single packet 
 *  
 *  @param priv    A pointer to bt_private structure
 *  @param skb     A pointer to skb which includes TX packet
 *  @return 	   BT_STATUS_SUCCESS or BT_STATUS_FAILURE
 */
static int
SendSinglePacket(bt_private * priv, struct sk_buff *skb)
{
    u8 ret;
    ENTER();
    if (!skb->len || ((skb->len + SDIO_HEADER_LEN) > BT_UPLD_SIZE)) {
        PRINTM(ERROR, "Tx Error: Bad skb length %d : %d\n", skb->len,
               BT_UPLD_SIZE);
        return BT_STATUS_FAILURE;
    }
    /* This is SDIO specific header length: byte[3][2][1], type: byte[0]
       (HCI_COMMAND = 1, ACL_DATA = 2, SCO_DATA = 3, 0xFE = Vendor) */
    skb_push(skb, SDIO_HEADER_LEN);
    skb->data[0] = (skb->len & 0x0000ff);
    skb->data[1] = (skb->len & 0x00ff00) >> 8;
    skb->data[2] = (skb->len & 0xff0000) >> 16;
    skb->data[3] = bt_cb(skb)->pkt_type;
    ret = sbi_host_to_card(priv, skb->data, skb->len);
    LEAVE();
    return ret;
}

/**
 *  @brief This function initializes the adapter structure
 *  and set default value to the member of adapter.
 *  
 *  @param priv    A pointer to bt_private structure
 *  @return 	   n/a
 */
static void
bt_init_adapter(bt_private * priv)
{
    ENTER();
    skb_queue_head_init(&priv->adapter->tx_queue);
    priv->adapter->ps_state = PS_AWAKE;
    init_waitqueue_head(&priv->adapter->cmd_wait_q);
    LEAVE();
}

/** 
 *  @brief This function initializes firmware
 *  
 *  @param priv    A pointer to bt_private structure
 *  @return 	   BT_STATUS_SUCCESS or BT_STATUS_FAILURE
 */
static int
bt_init_fw(bt_private * priv)
{
    int ret = BT_STATUS_SUCCESS;
    ENTER();
    if (fw == 0) {
        sbi_enable_host_int(priv);
        goto done;
    }
    sbi_disable_host_int(priv);

    if (sbi_dowload_fw(priv)) {
        PRINTM(ERROR, "FW failed to be download!\n");
        ret = BT_STATUS_FAILURE;
        goto done;
    }

    mdelay(100);
    sbi_enable_host_int(priv);
  done:
    LEAVE();
    return ret;
}

/** 
 *  @brief This function frees the structure of adapter
 *    
 *  @param priv    A pointer to bt_private structure
 *  @return 	   n/a
 */
static void
bt_free_adapter(bt_private * priv)
{
    bt_adapter *Adapter = priv->adapter;
    ENTER();
    skb_queue_purge(&priv->adapter->tx_queue);
    /* Free the adapter object itself */
    kfree(Adapter);
    priv->adapter = NULL;

    LEAVE();
}

/** 
 *  @brief This function handle the ioctl 
 *  
 *  @param hev     A pointer to hci_dev structure
 *  @cmd	   ioctl cmd 
 *  @arg	    
 *  @return 	   -ENOIOCTLCMD
 */
static int
bt_ioctl(struct hci_dev *hdev, unsigned int cmd, unsigned long arg)
{
    ENTER();
    LEAVE();
    return -ENOIOCTLCMD;
}

/** 
 *  @brief This function handle destruct 
 *  
 *  @param hev     A pointer to hci_dev structure
 *  
 *  @return 	   n/a
 */
static void
bt_destruct(struct hci_dev *hdev)
{
    ENTER();
    LEAVE();
    return;
}

/** 
 *  @brief This function handle the transmit
 *  
 *  @param skb     A pointer to sk_buffer structure
 *  
 *  @return 	   BT_STATUS_SUCCESS or other   
 */
static int
bt_send_frame(struct sk_buff *skb)
{
    struct hci_dev *hdev = (struct hci_dev *) skb->dev;
    bt_private *priv = (bt_private *) hdev->driver_data;

    ENTER();
    PRINTM(INFO, "bt_send_frame: Type=%d, len=%d\n", skb->pkt_type, skb->len);
    DBG_HEXDUMP(DBG_DATA, "bt_send_frame", skb->data, skb->len);
    if (!hdev) {
        PRINTM(ERROR, "Frame for unknown HCI device (hdev=NULL)\n");
        LEAVE();
        return -ENODEV;
    }
    if (!test_bit(HCI_RUNNING, &hdev->flags)) {
        PRINTM(ERROR, "Fail test HCI_RUNING,flag=%lx\n", hdev->flags);
        LEAVE();
        return -EBUSY;
    }
    switch (bt_cb(skb)->pkt_type) {
    case HCI_COMMAND_PKT:
        hdev->stat.cmd_tx++;
        break;
    case HCI_ACLDATA_PKT:
        hdev->stat.acl_tx++;
        break;
    case HCI_SCODATA_PKT:
        hdev->stat.sco_tx++;
        break;
    }
    skb_queue_tail(&priv->adapter->tx_queue, skb);
    wake_up_interruptible(&priv->MainThread.waitQ);
    LEAVE();
    return BT_STATUS_SUCCESS;
}

/** 
 *  @brief This function flush the transmit queue
 *  
 *  @param hev     A pointer to hci_dev structure
 *  
 *  @return 	   BT_STATUS_SUCCESS   
 */
static int
bt_flush(struct hci_dev *hdev)
{
    bt_private *priv = (bt_private *) hdev->driver_data;
    ENTER();
    skb_queue_purge(&priv->adapter->tx_queue);
    LEAVE();
    return BT_STATUS_SUCCESS;
}

/** 
 *  @brief This function close the bluetooth device
 *  
 *  @param hev     A pointer to hci_dev structure
 *  
 *  @return 	   BT_STATUS_SUCCESS   
 */
static int
bt_close(struct hci_dev *hdev)
{
    bt_private *priv = (bt_private *) hdev->driver_data;
    ENTER();
    if (!test_and_clear_bit(HCI_RUNNING, &hdev->flags)) {
        LEAVE();
        return BT_STATUS_SUCCESS;
    }
    skb_queue_purge(&priv->adapter->tx_queue);
    module_put(THIS_MODULE);
    LEAVE();
    return BT_STATUS_SUCCESS;
}

/** 
 *  @brief This function open the bluetooth device
 *  
 *  @param hev     A pointer to hci_dev structure
 *  
 *  @return 	   BT_STATUS_SUCCESS  or other
 */
static int
bt_open(struct hci_dev *hdev)
{
    ENTER();
    if (try_module_get(THIS_MODULE) == 0)
        return BT_STATUS_FAILURE;
    set_bit(HCI_RUNNING, &hdev->flags);
    LEAVE();
    return BT_STATUS_SUCCESS;
}

/** 
 *  @brief This function handles the major job in bluetooth driver.
 *  it handles the event generated by firmware, rx data received
 *  from firmware and tx data sent from kernel.
 *  
 *  @param data    A pointer to bt_thread structure
 *  @return        BT_STATUS_SUCCESS
 */
static int
bt_service_main_thread(void *data)
{
    bt_thread *thread = data;
    bt_private *priv = thread->priv;
    bt_adapter *Adapter = priv->adapter;
    wait_queue_t wait;
    struct sk_buff *skb;
    ENTER();
    bt_activate_thread(thread);
    init_waitqueue_entry(&wait, current);
    current->flags |= PF_NOFREEZE;

    for (;;) {
        add_wait_queue(&thread->waitQ, &wait);
        OS_SET_THREAD_STATE(TASK_INTERRUPTIBLE);
        if (priv->adapter->WakeupTries ||
            ((!priv->adapter->IntCounter) &&
             (!priv->bt_dev.tx_dnld_rdy ||
              skb_queue_empty(&priv->adapter->tx_queue)))) {
            PRINTM(INFO, "Main: Thread sleeping...\n");
            schedule();
        }
        OS_SET_THREAD_STATE(TASK_RUNNING);
        remove_wait_queue(&thread->waitQ, &wait);
        if (kthread_should_stop() || Adapter->SurpriseRemoved) {
            PRINTM(INFO, "main-thread: break from main thread: "
                   "SurpriseRemoved=0x%x\n", Adapter->SurpriseRemoved);
            break;
        }

        PRINTM(INFO, "Main: Thread waking up...\n");
        if (priv->adapter->IntCounter) {
            OS_INT_DISABLE;
            Adapter->IntCounter = 0;
            OS_INT_RESTORE;
        } else if ((priv->adapter->ps_state == PS_SLEEP) &&
                   !skb_queue_empty(&priv->adapter->tx_queue)) {
            priv->adapter->WakeupTries++;
            sbi_wakeup_firmware(priv);
            continue;
        }
        if (priv->adapter->ps_state == PS_SLEEP)
            continue;
        if (priv->bt_dev.tx_dnld_rdy == TRUE) {
            if (!skb_queue_empty(&priv->adapter->tx_queue)) {
                skb = skb_dequeue(&priv->adapter->tx_queue);
                if (skb) {
                    if (SendSinglePacket(priv, skb))
                        priv->bt_dev.hcidev->stat.err_tx++;
                    else
                        priv->bt_dev.hcidev->stat.byte_tx += skb->len;
                    kfree_skb(skb);
                }
            }
        }
    }
    bt_deactivate_thread(thread);
    LEAVE();
    return BT_STATUS_SUCCESS;
}

/** 
 *  @brief This function handles the interrupt. it will change PS
 *  state if applicable. it will wake up main_thread to handle
 *  the interrupt event as well.
 *  
 *  @param hdev    A pointer to hci_dev structure
 *  @return        n/a
 */
void
bt_interrupt(struct hci_dev *hdev)
{
    bt_private *priv = (bt_private *) hdev->driver_data;
    ENTER();
    priv->adapter->ps_state = PS_AWAKE;
    priv->adapter->hs_state = HS_DEACTIVATED;
    priv->adapter->WakeupTries = 0;
    priv->adapter->IntCounter++;
    LEAVE();

}

/**
 * @brief This function adds the card. it will probe the
 * card, allocate the bt_priv and initialize the device. 
 *  
 *  @param card    A pointer to card
 *  @return        A pointer to bt_private structure
 */

bt_private *
bt_add_card(void *card)
{
    struct hci_dev *hdev = NULL;
    bt_private *priv = NULL;
    int ret;

    ENTER();

    priv = kzalloc(sizeof(bt_private), GFP_KERNEL);
    if (!priv) {
        PRINTM(FATAL, "Can not allocate priv\n");
        LEAVE();
        return NULL;
    }
    /* allocate buffer for bt_adapter */
    if (!(priv->adapter = kzalloc(sizeof(bt_adapter), GFP_KERNEL))) {
        PRINTM(FATAL, "Allocate buffer for bt_adapter failed!\n");
        goto err_kmalloc;
    }

    bt_init_adapter(priv);

    /* Register to HCI Core */
    hdev = hci_alloc_dev();
    if (!hdev) {
        PRINTM(FATAL, "Can not allocate HCI device\n");
        goto err_kmalloc;
    }

    PRINTM(INFO, "Starting kthread...\n");
    priv->MainThread.priv = priv;
    spin_lock_init(&priv->driver_lock);

    bt_create_thread(bt_service_main_thread, &priv->MainThread,
                     "bt_main_service");

    /* wait for mainthread to up */
    while (!priv->MainThread.pid) {
        os_sched_timeout(1);
    }
    priv->bt_dev.hcidev = hdev;
    priv->bt_dev.card = card;
    btpriv = priv;
    hdev->driver_data = priv;
    ((struct sdio_mmc_card *) card)->priv = priv;
    /* 
     * Register the device. Fillup the private data structure with
     * relevant information from the card and request for the required
     * IRQ. 
     */
    if (sbi_register_dev(priv) < 0) {
        PRINTM(FATAL, "Failed to register bt device!\n");
        goto err_registerdev;
    }
    if (bt_init_fw(priv)) {
        PRINTM(FATAL, "Firmware Init Failed\n");
        goto err_init_fw;
    }
    priv->bt_dev.tx_dnld_rdy = TRUE;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,19)
    hdev->type = HCI_SDIO;
#else
    hdev->type = HCI_PCCARD;
#endif

    hdev->open = bt_open;
    hdev->close = bt_close;
    hdev->flush = bt_flush;
    hdev->send = bt_send_frame;
    hdev->destruct = bt_destruct;
    hdev->ioctl = bt_ioctl;

    hdev->owner = THIS_MODULE;
    bt_send_module_cfg_cmd(priv, MODULE_BRINGUP_REQ);
    if (priv->bt_dev.devType == DEV_TYPE_AMP)
        hdev->type |= HCI_BT_AMP;
    ret = hci_register_dev(hdev);
    if (ret < 0) {
        PRINTM(FATAL, "Can not register HCI device\n");
        goto err_init_fw;
    }
    bt_proc_init(priv);

    LEAVE();
    return priv;

  err_init_fw:
    sbi_unregister_dev(priv);
  err_registerdev:
    /* Stop the thread servicing the interrupts */
    priv->adapter->SurpriseRemoved = TRUE;
    wake_up_interruptible(&priv->MainThread.waitQ);
    while (priv->MainThread.pid) {
        os_sched_timeout(1);
    }
  err_kmalloc:
    if (hdev)
        hci_free_dev(hdev);
    bt_free_adapter(priv);
    kfree(priv);
    btpriv = NULL;
    LEAVE();
    return NULL;
}

/** 
 *  @brief This function removes the card.
 *  
 *  @param card    A pointer to card
 *  @return        BT_STATUS_SUCCESS
 */

int
bt_remove_card(void *card)
{
    struct hci_dev *hdev;
    bt_private *priv;

    ENTER();

    priv = ((struct sdio_mmc_card *) card)->priv;
    if (!priv) {
        LEAVE();
        return BT_STATUS_SUCCESS;
    }
    hdev = priv->bt_dev.hcidev;
    /* Disable interrupts on the card */
    sbi_disable_host_int(priv);
    wake_up_interruptible(&priv->adapter->cmd_wait_q);
    priv->adapter->SurpriseRemoved = TRUE;
    wake_up_interruptible(&priv->MainThread.waitQ);
    while (priv->MainThread.pid) {
        os_sched_timeout(1);
    }
    bt_proc_remove(priv);
    PRINTM(INFO, "unregester dev\n");
    sbi_unregister_dev(priv);

    /* UnRegister to HCI Core */
    if (hci_unregister_dev(hdev) < 0)
        PRINTM(ERROR, "Can not unregister HCI device %s\n", hdev->name);
    hci_free_dev(hdev);
    priv->bt_dev.hcidev = NULL;
    bt_free_adapter(priv);
    kfree(priv);
    btpriv = NULL;

    LEAVE();
    return BT_STATUS_SUCCESS;
}

/** 
 *  @brief This function initializes module.
 *  
 *  @param     n/a    A pointer to bt_private structure
 *  @return        BT_STATUS_SUCCESS or BT_STATUS_FAILURE
 */
static int
bt_init_module(void)
{
    int ret = BT_STATUS_SUCCESS;
    ENTER();
    if (sbi_register() == NULL) {
        ret = BT_STATUS_FAILURE;
        goto done;
    }
  done:
    LEAVE();
    return ret;
}

/** 
 *  @brief This function cleans module
 *  
 *  @param priv    n/a
 *  @return        n/a
 */
static void
bt_exit_module(void)
{
    ENTER();
    if (btpriv)
        bt_send_module_cfg_cmd(btpriv, MODULE_SHUTDOWN_REQ);

    sbi_unregister();
    LEAVE();
}

module_init(bt_init_module);
module_exit(bt_exit_module);

MODULE_AUTHOR("Marvell International Ltd.");
MODULE_DESCRIPTION("Marvell Bluetooth Driver Ver. " VERSION);
MODULE_VERSION(VERSION);
MODULE_LICENSE("GPL");
module_param(fw, int, 1);
MODULE_PARM_DESC(fw, "0: Skip firmware download; otherwise: Download firmware");
