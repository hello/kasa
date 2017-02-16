/** @file bt_main.c
  *  
  * @brief This file contains the major functions in BlueTooth
  * driver. It includes init, exit, open, close and main
  * thread etc..
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
#define VERSION "1.63"

/** Driver version */
char driver_version[] = "sd8787-" VERSION "-(" "FP" FPNUM ")"
#ifdef DEBUG_LEVEL2
    "-dbg"
#endif
    " ";

/** Firmware flag */
int fw = 1;
/** default powermode */
int psmode = 1;
/** Default FW download CRC check control */
int fw_crc_check = 1;
/** Init config file (MAC address, register etc.) */
char *init_cfg = NULL;

#ifdef	DEBUG_LEVEL1
#ifdef  DEBUG_LEVEL2
#define DEFAULT_DEBUG_MASK  (0xffffffff & ~DBG_EVENT)
#else
#define DEFAULT_DEBUG_MASK  (DBG_MSG | DBG_FATAL | DBG_ERROR)
#endif /* DEBUG_LEVEL2 */
ulong drvdbg = DEFAULT_DEBUG_MASK;
#endif
#ifdef SDIO_SUSPEND_RESUME
/** PM keep power */
int pm_keep_power = 1;
#endif

/** 
 *  @brief This function verify the received event pkt
 *
 *  Event format:
 *  +--------+--------+--------+--------+--------+
 *  | Event  | Length |  ncmd  |      Opcode     |
 *  +--------+--------+--------+--------+--------+
 *  | 1-byte | 1-byte | 1-byte |      2-byte     |
 *  +--------+--------+--------+--------+--------+
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
    if (!priv->bt_dev.sendcmdflag)
        return;
    if (hdr->evt == HCI_EV_CMD_COMPLETE) {
        ec = (struct hci_ev_cmd_complete *) (skb->data + HCI_EVENT_HDR_SIZE);
        opcode = __le16_to_cpu(ec->opcode);
        ocf = hci_opcode_ocf(opcode);
        switch (ocf) {
        case BT_CMD_MODULE_CFG_REQ:
        case BT_CMD_CONFIG_MAC_ADDR:
        case BT_CMD_CSU_WRITE_REG:
            priv->bt_dev.sendcmdflag = FALSE;
            priv->adapter->cmd_complete = TRUE;
            wake_up_interruptible(&priv->adapter->cmd_wait_q);
            break;
        default:
            break;
        }
    }
    LEAVE();
    return;
}

/** 
 *  @brief This function process the received event
 *
 *  Event format:
 *  +--------+--------+--------+--------+ ~ -----+
 *  |   EC   | Length |           Data           |
 *  +--------+--------+--------+--------+ ~ -----+
 *  | 1-byte | 1-byte |          n-byte          |
 *  +--------+--------+--------+--------+ ~ -----+
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
    struct hci_dev *hdev = priv->bt_dev.hcidev;
    ENTER();
    pEvent = (BT_EVENT *) skb->data;
    if (pEvent->EC != 0xff) {
        PRINTM(CMD, "Not Marvell Event=0x%x\n", pEvent->EC);
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
            PRINTM(CMD, "PS Mode %s:%s\n", hdev->name,
                   (priv->adapter->psmode) ? "Enable" : "Disable");
        } else
            PRINTM(CMD, "PS Mode Command Fail %s\n", hdev->name);
        break;
    case BT_CMD_HOST_SLEEP_CONFIG:
        if (pEvent->data[3] == BT_STATUS_SUCCESS) {
            PRINTM(CMD, "%s: gpio=0x%x, gap=0x%x\n", hdev->name,
                   pEvent->data[1], pEvent->data[2]);
        } else
            PRINTM(CMD, "%s: HSCFG Command Fail\n", hdev->name);
        break;
    case BT_CMD_HOST_SLEEP_ENABLE:
        if (pEvent->data[1] == BT_STATUS_SUCCESS) {
            priv->adapter->hs_state = HS_ACTIVATED;
            if (priv->adapter->suspend_fail == FALSE) {
#ifdef SDIO_SUSPEND_RESUME
#ifdef MMC_PM_KEEP_POWER
#ifdef MMC_PM_FUNC_SUSPENDED
                bt_is_suspended(priv);
#endif
#endif
#endif
                wake_up_interruptible(&priv->adapter->cmd_wait_q);
            }
            if (priv->adapter->psmode)
                priv->adapter->ps_state = PS_SLEEP;
            PRINTM(CMD, "EVENT %s: HS ACTIVATED!\n", hdev->name);
        } else
            PRINTM(CMD, "%s: HS Enable Fail\n", hdev->name);
        break;
    case BT_CMD_MODULE_CFG_REQ:
        if ((priv->bt_dev.sendcmdflag == TRUE) &&
            ((pEvent->data[1] == MODULE_BRINGUP_REQ)
             || (pEvent->data[1] == MODULE_SHUTDOWN_REQ))) {
            if (pEvent->data[1] == MODULE_BRINGUP_REQ) {
                PRINTM(CMD, "EVENT %s:%s\n", hdev->name,
                       (pEvent->data[2] &&
                        (pEvent->data[2] !=
                         MODULE_CFG_RESP_ALREADY_UP)) ? "Bring up Fail" :
                       "Bring up success");
                priv->bt_dev.devType = pEvent->data[3];
                PRINTM(CMD, "devType:%s\n",
                       (pEvent->data[3] ==
                        1) ? "AMP controller" : "BR/EDR controller");

            }
            if (pEvent->data[1] == MODULE_SHUTDOWN_REQ) {
                PRINTM(CMD, "EVENT %s:%s\n", hdev->name,
                       (pEvent->
                        data[2]) ? "Shut down Fail" : "Shut down success");
            }
            if (pEvent->data[2]) {
                priv->bt_dev.sendcmdflag = FALSE;
                priv->adapter->cmd_complete = TRUE;
                wake_up_interruptible(&priv->adapter->cmd_wait_q);
            }
        } else {
            PRINTM(CMD, "BT_CMD_MODULE_CFG_REQ resp for APP\n");
            ret = BT_STATUS_FAILURE;
        }
        break;
    case BT_EVENT_POWER_STATE:
        if (pEvent->data[1] == BT_PS_SLEEP)
            priv->adapter->ps_state = PS_SLEEP;
        PRINTM(CMD, "EVENT %s:%s\n", hdev->name,
               (priv->adapter->ps_state) ? "PS_SLEEP" : "PS_AWAKE");
        break;
    case BT_CMD_SDIO_PULL_CFG_REQ:
        if (pEvent->data[5] == BT_STATUS_SUCCESS)
            PRINTM(CMD, "%s: SDIO pull configuration success\n", hdev->name);
        else
            PRINTM(CMD, "%s: SDIO pull configuration fail\n", hdev->name);
        break;
    default:
        PRINTM(CMD, "Unknown Event=%d %s\n", pEvent->data[0], hdev->name);
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
 *  Command format:
 *  +--------+--------+--------+--------+--------+--------+--------+
 *  |     OCF OGF     | Length |                Data               |
 *  +--------+--------+--------+--------+--------+--------+--------+
 *  |     2-byte      | 1-byte |               4-byte              |
 *  +--------+--------+--------+--------+--------+--------+--------+
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
    PRINTM(CMD, "Queue module cfg Command(0x%x)\n", pCmd->ocf_ogf);
    wake_up_interruptible(&priv->MainThread.waitQ);
    /* 
       On some Adroind platforms certain delay is needed for HCI daemon to
       remove this module and close itself gracefully. Otherwise it hangs. This 
       10ms delay is a workaround for such platforms as the root cause has not
       been found yet. */
    mdelay(10);
    if (!os_wait_interruptible_timeout
        (priv->adapter->cmd_wait_q, priv->adapter->cmd_complete,
         WAIT_UNTIL_CMD_RESP)) {
        ret = BT_STATUS_FAILURE;
        PRINTM(MSG, "module_cfg_cmd (0x%x): timeout sendcmdflag=%d\n", subcmd,
               priv->bt_dev.sendcmdflag);
    } else {
        PRINTM(CMD, "module cfg Command done\n");
    }
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
    priv->adapter->suspend_fail = FALSE;
    pCmd = (BT_CMD *) skb->tail;
    pCmd->ocf_ogf = (OGF << 10) | BT_CMD_HOST_SLEEP_ENABLE;
    pCmd->length = 0;
    bt_cb(skb)->pkt_type = MRVL_VENDOR_PKT;
    skb_put(skb, sizeof(BT_CMD));
    skb->dev = (void *) priv->bt_dev.hcidev;
    skb_queue_head(&priv->adapter->tx_queue, skb);
    PRINTM(CMD, "Queue hs enable Command(0x%x)\n", pCmd->ocf_ogf);
    wake_up_interruptible(&priv->MainThread.waitQ);
    os_wait_interruptible_timeout(priv->adapter->cmd_wait_q,
                                  priv->adapter->hs_state,
                                  WAIT_UNTIL_HS_STATE_CHANGED);
    OS_INT_DISABLE;
    if ((priv->adapter->hs_state == HS_ACTIVATED) ||
        (priv->adapter->is_suspended == TRUE)) {
        OS_INT_RESTORE;
        PRINTM(MSG, "BT: suspend success! skip=%d\n", priv->adapter->hs_skip);
    } else {
        priv->adapter->suspend_fail = TRUE;
        OS_INT_RESTORE;
        priv->adapter->hs_skip++;
        ret = BT_STATUS_FAILURE;
        PRINTM(MSG,
               "BT: suspend skipped! state=%d skip=%d ps_state= %d WakeupTries=%d\n",
               priv->adapter->hs_state, priv->adapter->hs_skip,
               priv->adapter->ps_state, priv->adapter->WakeupTries);
    }
  exit:
    LEAVE();
    return ret;
}

/** 
 *  @brief This function set mac address 
 *  
 *  @param priv    A pointer to bt_private structure
 *  @param mac     A pointer to mac address
 *
 *  @return 	   BT_STATUS_SUCCESS or BT_STATUS_FAILURE
 */
int
bt_set_mac_address(bt_private * priv, u8 * mac)
{
    struct sk_buff *skb = NULL;
    u8 ret = BT_STATUS_SUCCESS;
    BT_HCI_CMD *pCmd;
    int i = 0;
    ENTER();
    skb = bt_skb_alloc(sizeof(BT_HCI_CMD), GFP_ATOMIC);
    if (skb == NULL) {
        PRINTM(WARN, "No free skb\n");
        ret = BT_STATUS_FAILURE;
        goto exit;
    }
    pCmd = (BT_HCI_CMD *) skb->tail;
    pCmd->ocf_ogf = (OGF << 10) | BT_CMD_CONFIG_MAC_ADDR;
    pCmd->length = 8;
    pCmd->cmd_type = MRVL_VENDOR_PKT;
    pCmd->cmd_len = 6;
    for (i = 0; i < 6; i++)
        pCmd->data[i] = mac[5 - i];
    bt_cb(skb)->pkt_type = HCI_COMMAND_PKT;
    skb_put(skb, sizeof(BT_HCI_CMD));
    skb->dev = (void *) priv->bt_dev.hcidev;
    skb_queue_head(&priv->adapter->tx_queue, skb);
    priv->bt_dev.sendcmdflag = TRUE;
    priv->adapter->cmd_complete = FALSE;
    PRINTM(CMD, "BT: Set mac address %02x:%02x:%02x:%02x:%02x:%02x (0x%x)\n",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], pCmd->ocf_ogf);
    wake_up_interruptible(&priv->MainThread.waitQ);
    if (!os_wait_interruptible_timeout
        (priv->adapter->cmd_wait_q, priv->adapter->cmd_complete,
         WAIT_UNTIL_CMD_RESP)) {
        ret = BT_STATUS_FAILURE;
        PRINTM(MSG, "BT: Set mac address: timeout: %d, %d,%d\n",
               priv->adapter->cmd_complete, priv->adapter->ps_state,
               priv->adapter->WakeupTries);
    }
  exit:
    LEAVE();
    return ret;
}

/** 
 *  @brief This function write value to CSU registers
 *  
 *  @param priv    A pointer to bt_private structure
 *  @param type    reg type
 *  @param offset  register address
 *  @param value   register value to write
 *  @return 	   BT_STATUS_SUCCESS or BT_STATUS_FAILURE
 */
int
bt_write_reg(bt_private * priv, u8 type, u32 offset, u16 value)
{
    struct sk_buff *skb = NULL;
    u8 ret = BT_STATUS_SUCCESS;
    BT_CSU_CMD *pCmd;
    ENTER();
    skb = bt_skb_alloc(sizeof(BT_CSU_CMD), GFP_ATOMIC);
    if (skb == NULL) {
        PRINTM(WARN, "No free skb\n");
        ret = BT_STATUS_FAILURE;
        goto exit;
    }
    pCmd = (BT_CSU_CMD *) skb->tail;
    pCmd->ocf_ogf = (OGF << 10) | BT_CMD_CSU_WRITE_REG;
    pCmd->length = 7;
    pCmd->type = type;
    pCmd->offset[0] = (offset & 0x000000ff);
    pCmd->offset[1] = (offset & 0x0000ff00) >> 8;
    pCmd->offset[2] = (offset & 0x00ff0000) >> 16;
    pCmd->offset[3] = (offset & 0xff000000) >> 24;
    pCmd->value[0] = (value & 0x00ff);
    pCmd->value[1] = (value & 0xff00) >> 8;
    bt_cb(skb)->pkt_type = HCI_COMMAND_PKT;
    skb_put(skb, sizeof(BT_CSU_CMD));
    skb->dev = (void *) priv->bt_dev.hcidev;
    skb_queue_head(&priv->adapter->tx_queue, skb);
    priv->bt_dev.sendcmdflag = TRUE;
    priv->adapter->cmd_complete = FALSE;
    PRINTM(CMD, "BT: Set CSU reg type=%d reg=0x%x value=0x%x\n", type, offset,
           value);
    wake_up_interruptible(&priv->MainThread.waitQ);
    if (!os_wait_interruptible_timeout
        (priv->adapter->cmd_wait_q, priv->adapter->cmd_complete,
         WAIT_UNTIL_CMD_RESP)) {
        ret = BT_STATUS_FAILURE;
        PRINTM(MSG, "BT: BT: Set CSU reg timeout: %d, %d, %d\n",
               priv->adapter->cmd_complete, priv->adapter->ps_state,
               priv->adapter->WakeupTries);
    }
  exit:
    LEAVE();
    return ret;
}

/** 
 *  @brief This function used to restore tx_queue
 *
 *  @param priv    A pointer to bt_private structure
 *  @return        N/A
 */
void
bt_restore_tx_queue(bt_private * priv)
{
    struct sk_buff *skb = NULL;
    while (!skb_queue_empty(&priv->adapter->pending_queue)) {
        skb = skb_dequeue(&priv->adapter->pending_queue);
        skb_queue_tail(&priv->adapter->tx_queue, skb);
    }
    wake_up_interruptible(&priv->MainThread.waitQ);
}

/** 
 *  @brief This function used to send command to firmware
 *
 *  Command format:
 *  +--------+--------+--------+--------+--------+--------+--------+
 *  |     OCF OGF     | Length |                Data               |
 *  +--------+--------+--------+--------+--------+--------+--------+
 *  |     2-byte      | 1-byte |               4-byte              |
 *  +--------+--------+--------+--------+--------+--------+--------+
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
        PRINTM(CMD, "Queue HSCFG Command(0x%x),gpio=0x%x,gap=0x%x\n",
               pCmd->ocf_ogf, pCmd->data[0], pCmd->data[1]);
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
        if (priv->bt_dev.psmode)
            pCmd->data[0] = BT_PS_ENABLE;
        else
            pCmd->data[0] = BT_PS_DISABLE;
        if (priv->bt_dev.idle_timeout) {
            pCmd->length = 3;
            pCmd->data[1] = (u8) (priv->bt_dev.idle_timeout & 0x00ff);
            pCmd->data[2] = (priv->bt_dev.idle_timeout & 0xff00) >> 8;
        } else {
            pCmd->length = 1;
        }
        bt_cb(skb)->pkt_type = MRVL_VENDOR_PKT;
        skb_put(skb, sizeof(BT_CMD));
        skb->dev = (void *) priv->bt_dev.hcidev;
        skb_queue_head(&priv->adapter->tx_queue, skb);
        PRINTM(CMD, "Queue PSMODE Command(0x%x):%d\n", pCmd->ocf_ogf,
               pCmd->data[0]);
    }
    if (priv->bt_dev.sdio_pull_ctrl) {
        priv->bt_dev.sdio_pull_ctrl = 0;
        skb = bt_skb_alloc(sizeof(BT_CMD), GFP_ATOMIC);
        if (skb == NULL) {
            PRINTM(WARN, "No free skb\n");
            ret = BT_STATUS_FAILURE;
            goto exit;
        }
        pCmd = (BT_CMD *) skb->tail;
        pCmd->ocf_ogf = (OGF << 10) | BT_CMD_SDIO_PULL_CFG_REQ;
        pCmd->length = 4;
        pCmd->data[0] = (priv->bt_dev.sdio_pull_cfg & 0x000000ff);
        pCmd->data[1] = (priv->bt_dev.sdio_pull_cfg & 0x0000ff00) >> 8;
        pCmd->data[2] = (priv->bt_dev.sdio_pull_cfg & 0x00ff0000) >> 16;
        pCmd->data[3] = (priv->bt_dev.sdio_pull_cfg & 0xff000000) >> 24;
        bt_cb(skb)->pkt_type = MRVL_VENDOR_PKT;
        skb_put(skb, sizeof(BT_CMD));
        skb->dev = (void *) priv->bt_dev.hcidev;
        skb_queue_head(&priv->adapter->tx_queue, skb);
        PRINTM(CMD,
               "Queue SDIO PULL CFG Command(0x%x), PullUp=0x%x%x,PullDown=0x%x%x\n",
               pCmd->ocf_ogf, pCmd->data[1], pCmd->data[0], pCmd->data[3],
               pCmd->data[2]);
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
    if (!skb || !skb->data)
        return BT_STATUS_FAILURE;
    if (!skb->len || ((skb->len + BT_HEADER_LEN) > BT_UPLD_SIZE)) {
        PRINTM(ERROR, "Tx Error: Bad skb length %d : %d\n", skb->len,
               BT_UPLD_SIZE);
        return BT_STATUS_FAILURE;
    }
    /* This is SDIO specific header length: byte[3][2][1], type: byte[0]
       (HCI_COMMAND = 1, ACL_DATA = 2, SCO_DATA = 3, 0xFE = Vendor) */
    skb_push(skb, BT_HEADER_LEN);
    skb->data[0] = (skb->len & 0x0000ff);
    skb->data[1] = (skb->len & 0x00ff00) >> 8;
    skb->data[2] = (skb->len & 0xff0000) >> 16;
    skb->data[3] = bt_cb(skb)->pkt_type;
    if (bt_cb(skb)->pkt_type == MRVL_VENDOR_PKT)
        PRINTM(CMD, "DNLD_CMD: ocf_ogf=0x%x len=%d\n",
               *((u16 *) & skb->data[4]), skb->len);
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
    skb_queue_head_init(&priv->adapter->pending_queue);
    priv->adapter->tx_lock = FALSE;
    priv->adapter->ps_state = PS_AWAKE;
    priv->adapter->suspend_fail = FALSE;
    priv->adapter->is_suspended = FALSE;
    priv->adapter->hs_skip = 0;
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
    priv->fw_crc_check = fw_crc_check;
    if (sbi_download_fw(priv)) {
        PRINTM(ERROR, "FW failed to be download!\n");
        ret = BT_STATUS_FAILURE;
        goto done;
    }
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
    bt_private *priv = NULL;

    ENTER();
    PRINTM(DATA, "bt_send_frame %s: Type=%d, len=%d\n", hdev->name,
           skb->pkt_type, skb->len);
    DBG_HEXDUMP(CMD_D, "bt_send_frame", skb->data, skb->len);
    if (!hdev || !hdev->driver_data) {
        PRINTM(ERROR, "Frame for unknown HCI device (hdev=NULL)\n");
        LEAVE();
        return -ENODEV;
    }
    priv = (bt_private *) hdev->driver_data;
    if (!test_bit(HCI_RUNNING, &hdev->flags)) {
        PRINTM(ERROR, "Fail test HCI_RUNING, flag=0x%lx\n", hdev->flags);
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
    if (priv->adapter->tx_lock == TRUE)
        skb_queue_tail(&priv->adapter->pending_queue, skb);
    else
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
    skb_queue_purge(&priv->adapter->pending_queue);
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
    u8 ireg = 0;
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
            sbi_get_int_status(priv, &ireg);
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
    PRINTM(INTR, "*\n");
    priv->adapter->ps_state = PS_AWAKE;
#ifndef SDIO_SUSPEND_RESUME
    priv->adapter->hs_state = HS_DEACTIVATED;
#endif
    priv->adapter->WakeupTries = 0;
    priv->adapter->IntCounter++;
    wake_up_interruptible(&priv->MainThread.waitQ);
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
    hdev->driver_data = priv;
    ((struct sdio_mmc_card *) card)->priv = priv;
    priv->adapter->sd_ireg = 0;
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

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,34)
    hdev->bus = HCI_SDIO;
#else
    hdev->type = HCI_SDIO;
#endif /* >= 2.6.34 */
    hdev->open = bt_open;
    hdev->close = bt_close;
    hdev->flush = bt_flush;
    hdev->send = bt_send_frame;
    hdev->destruct = bt_destruct;
    hdev->ioctl = bt_ioctl;

    hdev->owner = THIS_MODULE;
    ret = bt_send_module_cfg_cmd(priv, MODULE_BRINGUP_REQ);
    if (ret < 0) {
        PRINTM(FATAL, "Module cfg command send failed!\n");
        goto err_init_fw;
    }
    if (psmode) {
        priv->bt_dev.pscmd = TRUE;
        priv->bt_dev.psmode = TRUE;
        priv->bt_dev.idle_timeout = DEFAULT_IDLE_TIME;
        bt_prepare_command(priv);
    }
#ifdef SDIO_SUSPEND_RESUME
    priv->bt_dev.hscfgcmd = TRUE;
    priv->bt_dev.gpio_gap = 0xffff;
    bt_prepare_command(priv);
#endif
    priv->bt_dev.sdio_pull_cfg = 0xffffffff;
    priv->bt_dev.sdio_pull_ctrl = 0;
    wake_up_interruptible(&priv->MainThread.waitQ);
    /* block all the packet from bluez */
    if (init_cfg)
        priv->adapter->tx_lock = TRUE;
    ret = hci_register_dev(hdev);
    if (ret < 0) {
        PRINTM(FATAL, "Can not register HCI device\n");
        hci_free_dev(hdev);
        hdev = NULL;
        goto err_init_fw;
    }
    if (init_cfg) {
        if (BT_STATUS_SUCCESS != bt_init_config(priv, init_cfg)) {
            PRINTM(FATAL, "Set user init data and param failed\n");
            /* UnRegister to HCI Core */
            if (hci_unregister_dev(hdev) < 0)
                PRINTM(ERROR, "Can not unregister HCI device %s\n", hdev->name);
            hci_free_dev(hdev);
            hdev = NULL;
            goto err_init_fw;
        } else {
            priv->adapter->tx_lock = FALSE;
            bt_restore_tx_queue(priv);
        }
    }
    bt_proc_init(priv);

    LEAVE();
    return priv;

  err_init_fw:
    PRINTM(INFO, "unregister device\n");
    sbi_unregister_dev(priv);
  err_registerdev:
    ((struct sdio_mmc_card *) card)->priv = NULL;
    /* Stop the thread servicing the interrupts */
    priv->adapter->SurpriseRemoved = TRUE;
    wake_up_interruptible(&priv->MainThread.waitQ);
    while (priv->MainThread.pid) {
        os_sched_timeout(1);
    }
  err_kmalloc:
    if (hdev)
        kfree(hdev);
    if (priv->adapter)
        bt_free_adapter(priv);
    kfree(priv);
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
    bt_private *priv = (bt_private *) card;

    ENTER();

    if (!priv) {
        LEAVE();
        return BT_STATUS_SUCCESS;
    }
    if (!priv->adapter->SurpriseRemoved) {
        bt_send_module_cfg_cmd(priv, MODULE_SHUTDOWN_REQ);
        priv->adapter->SurpriseRemoved = TRUE;
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
    PRINTM(INFO, "Free Adapter\n");
    bt_free_adapter(priv);
    kfree(priv);

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
    bt_root_proc_init();
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
    sbi_unregister();
    bt_root_proc_remove();
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
module_param(fw_crc_check, int, 1);
MODULE_PARM_DESC(fw_crc_check,
                 "1: Enable FW download CRC check (default); 0: Disable FW download CRC check");
module_param(psmode, int, 1);
MODULE_PARM_DESC(psmode, "1: Enable powermode; 0: Disable powermode");
#ifdef	DEBUG_LEVEL1
module_param(drvdbg, ulong, 0);
MODULE_PARM_DESC(drvdbg, "BIT3:DBG_DATA BIT4:DBG_CMD 0xFF:DBG_ALL");
#endif
#ifdef SDIO_SUSPEND_RESUME
module_param(pm_keep_power, int, 1);
MODULE_PARM_DESC(pm_keep_power, "1: PM keep power; 0: PM no power");
#endif
module_param(init_cfg, charp, 0);
MODULE_PARM_DESC(init_cfg, "BT init config file name");
