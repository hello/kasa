/** @file bt_drv.h
 *  @brief This header file contains global constant/enum definitions,
 *  global variable declaration.
 *       
 *  Copyright (C) 2007-2008, Marvell International Ltd.
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

#ifndef _BT_DRV_H_
#define _BT_DRV_H_

#ifndef BIT
/** BIT definition */
#define BIT(x) (1UL << (x))
#endif

/** Debug level : Message */
#define	DBG_MSG			BIT(0)
/** Debug level : Fatal */
#define DBG_FATAL		BIT(1)
/** Debug level : Error */
#define DBG_ERROR		BIT(2)
/** Debug level : Command */
#define DBG_CMD			BIT(3)
/** Debug level : Data */
#define DBG_DATA		BIT(27)
/** Debug level : Entry */
#define DBG_ENTRY		BIT(28)
/** Debug level : Warning */
#define DBG_WARN		BIT(29)
/** Debug level : Informative */
#define DBG_INFO		BIT(30)

#ifdef	DEBUG_LEVEL1
extern u32 drvdbg;

/** Print informative message */
#define	PRINTM_INFO(msg...)  do {if (drvdbg & DBG_INFO)  printk(KERN_DEBUG msg);} while(0)
/** Print warning message */
#define	PRINTM_WARN(msg...)  do {if (drvdbg & DBG_WARN)  printk(KERN_DEBUG msg);} while(0)
/** Print entry message */
#define	PRINTM_ENTRY(msg...) do {if (drvdbg & DBG_ENTRY) printk(KERN_DEBUG msg);} while(0)
/** Print command message */
#define	PRINTM_CMD(msg...)   do {if (drvdbg & DBG_CMD)   printk(KERN_DEBUG msg);} while(0)
/** Print erro message */
#define	PRINTM_ERROR(msg...) do {if (drvdbg & DBG_ERROR) printk(KERN_DEBUG msg);} while(0)
/** Print fatal message */
#define	PRINTM_FATAL(msg...) do {if (drvdbg & DBG_FATAL) printk(KERN_DEBUG msg);} while(0)
/** Print message */
#define	PRINTM_MSG(msg...)   do {if (drvdbg & DBG_MSG)   printk(KERN_ALERT msg);} while(0)

/** Print message with required level */
#define	PRINTM(level,msg...) PRINTM_##level(msg)

/** Debug dump buffer length */
#define DBG_DUMP_BUF_LEN 	64
/** Maximum number of dump per line */
#define MAX_DUMP_PER_LINE	16
/** Maximum data dump length */
#define MAX_DATA_DUMP_LEN	48

static inline void
hexdump(char *prompt, u8 * buf, int len)
{
    int i;
    char dbgdumpbuf[DBG_DUMP_BUF_LEN];
    char *ptr = dbgdumpbuf;

    printk(KERN_DEBUG "%s: len=%d\n", prompt, len);
    for (i = 1; i <= len; i++) {
        ptr += sprintf(ptr, "%02x ", *buf);
        buf++;
        if (i % MAX_DUMP_PER_LINE == 0) {
            *ptr = 0;
            printk(KERN_DEBUG "%s\n", dbgdumpbuf);
            ptr = dbgdumpbuf;
        }
    }
    if (len % MAX_DUMP_PER_LINE) {
        *ptr = 0;
        printk(KERN_DEBUG "%s\n", dbgdumpbuf);
    }
}

/** Debug hexdump of debug data */
#define DBG_HEXDUMP_DBG_DATA(x,y,z)     do {if (drvdbg & DBG_DATA) hexdump(x,y,z);} while(0)

/** Debug hexdump */
#define	DBG_HEXDUMP(level,x,y,z)    DBG_HEXDUMP_##level(x,y,z)

/** Mark entry point */
#define	ENTER()			PRINTM(ENTRY, "Enter: %s, %s:%i\n", __FUNCTION__, \
							__FILE__, __LINE__)
/** Mark exit point */
#define	LEAVE()			PRINTM(ENTRY, "Leave: %s, %s:%i\n", __FUNCTION__, \
							__FILE__, __LINE__)
#else
/** Do nothing */
#define	PRINTM(level,msg...) do {} while (0);
/** Do nothing */
#define DBG_HEXDUMP(level,x,y,z)    do {} while (0);
/** Do nothing */
#define	ENTER()  do {} while (0);
/** Do nothing */
#define	LEAVE()  do {} while (0);
#endif /* DEBUG_LEVEL1 */

/** Length of device name */
#define DEV_NAME_LEN				32
/** Bluetooth upload size */
#define	BT_UPLD_SIZE				2312
/** Bluetooth status success */
#define BT_STATUS_SUCCESS			(0)
/** Bluetooth status failure */
#define BT_STATUS_FAILURE			(-1)

#ifndef	TRUE
/** True value */
#define TRUE			1
#endif
#ifndef	FALSE
/** False value */
#define	FALSE			0
#endif

/** Set thread state */
#define OS_SET_THREAD_STATE(x)		set_current_state(x)
/** Time to wait until Host Sleep state change in millisecond */
#define WAIT_UNTIL_HS_STATE_CHANGED 5000
/** Time to wait cmd resp in millisecond */
#define WAIT_UNTIL_CMD_RESP	    5000

/** Sleep until a condition gets true or a timeout elapses */
#define os_wait_interruptible_timeout(waitq, cond, timeout) \
	wait_event_interruptible_timeout(waitq, cond, ((timeout) * HZ / 1000))

typedef struct
{
    /** Task */
    struct task_struct *task;
    /** Queue */
    wait_queue_head_t waitQ;
    /** PID */
    pid_t pid;
    /** Private structure */
    void *priv;
} bt_thread;

static inline void
bt_activate_thread(bt_thread * thr)
{
        /** Record the thread pid */
    thr->pid = current->pid;

        /** Initialize the wait queue */
    init_waitqueue_head(&thr->waitQ);
}

static inline void
bt_deactivate_thread(bt_thread * thr)
{
    thr->pid = 0;
    return;
}

static inline void
bt_create_thread(int (*btfunc) (void *), bt_thread * thr, char *name)
{
    thr->task = kthread_run(btfunc, thr, "%s", name);
}

static inline int
bt_terminate_thread(bt_thread * thr)
{
    /* Check if the thread is active or not */
    if (!thr->pid)
        return -1;

    kthread_stop(thr->task);
    return 0;
}

static inline void
os_sched_timeout(u32 millisec)
{
    set_current_state(TASK_INTERRUPTIBLE);

    schedule_timeout((millisec * HZ) / 1000);
}

#ifndef __ATTRIB_ALIGN__
#define __ATTRIB_ALIGN__ __attribute__((aligned(4)))
#endif

#ifndef __ATTRIB_PACK__
#define __ATTRIB_PACK__ __attribute__ ((packed))
#endif
/** Data structure for the Marvell Bluetooth device */
typedef struct _bt_dev
{
        /** device name */
    char name[DEV_NAME_LEN];
        /** card pointer */
    void *card;
        /** IO port */
    u32 ioport;
    /** HCI device */
    struct hci_dev *hcidev;

    /** Tx download ready flag */
    u8 tx_dnld_rdy;
    /** Function */
    u8 fn;
    /** Rx unit */
    u8 rx_unit;
    /** Power Save mode */
    u8 psmode;
    /** Power Save command */
    u8 pscmd;
    /** Host Sleep mode */
    u8 hsmode;
    /** Host Sleep command */
    u8 hscmd;
    /** Low byte is gap, high byte is GPIO */
    u16 gpio_gap;
    /** Host Sleep configuration command */
    u8 hscfgcmd;
    /** Host Send Cmd Flag		 */
    u8 sendcmdflag;
    /** Device Type			*/
    u8 devType;
} bt_dev_t, *pbt_dev_t;

typedef struct _bt_adapter
{
    /** Chip revision ID */
    u8 chip_rev;
    /** Surprise removed flag */
    u8 SurpriseRemoved;
    /** IRQ number */
    int irq;
    /** Interrupt counter */
    u32 IntCounter;
    /** Tx packet queue */
    struct sk_buff_head tx_queue;
    /** Power Save mode */
    u8 psmode;
    /** Power Save state */
    u8 ps_state;
    /** Host Sleep state */
    u8 hs_state;
    /** Number of wakeup tries */
    u8 WakeupTries;
    /** Host Sleep wait queue */
    wait_queue_head_t cmd_wait_q __ATTRIB_ALIGN__;
    /** Host Cmd complet state */
    u8 cmd_complete;
} bt_adapter, *pbt_adapter;

/** Private structure for the MV device */
typedef struct _bt_private
{
    /** Bluetooth device */
    bt_dev_t bt_dev;
    /** Adapter */
    bt_adapter *adapter;
    /** Firmware helper */
    const struct firmware *fw_helper;
    /** Firmware */
    const struct firmware *firmware;
    /** Hotplug device */
    struct device *hotplug_device;
        /** thread to service interrupts */
    bt_thread MainThread;
    /** Proc directory entry */
    struct proc_dir_entry *proc_entry;
    /** Proc mbt directory entry */
    struct proc_dir_entry *proc_mbt;
    /** Driver lock */
    spinlock_t driver_lock;
    /** Driver lock flags */
    ulong driver_flags;
} bt_private, *pbt_private;

/** BT_AMP flag for device type */
#define  HCI_BT_AMP			0x80
/** Device type of AMP */
#define DEV_TYPE_AMP			0x01
/** Marvell vendor packet */
#define MRVL_VENDOR_PKT			0xFE
/** Bluetooth command : Sleep mode */
#define BT_CMD_AUTO_SLEEP_MODE		0x23
/** Bluetooth command : Host Sleep configuration */
#define BT_CMD_HOST_SLEEP_CONFIG	0x59
/** Bluetooth command : Host Sleep enable */
#define BT_CMD_HOST_SLEEP_ENABLE	0x5A
/** Bluetooth command : Module Configuration request */
#define BT_CMD_MODULE_CFG_REQ		0x5B
/** Sub Command: Module Bring Up Request */
#define MODULE_BRINGUP_REQ		0xF1
/** Sub Command: Module Shut Down Request */
#define MODULE_SHUTDOWN_REQ		0xF2
/** Sub Command: Host Interface Control Request */
#define MODULE_INTERFACE_CTRL_REQ	0xF5

/** Bluetooth event : Power State */
#define BT_EVENT_POWER_STATE		0x20

/** Bluetooth Power State : Enable */
#define BT_PS_ENABLE			0x02
/** Bluetooth Power State : Disable */
#define BT_PS_DISABLE			0x03
/** Bluetooth Power State : Sleep */
#define BT_PS_SLEEP			0x01
/** Bluetooth Power State : Awake */
#define BT_PS_AWAKE			0x02

/** OGF */
#define OGF				0x3F

/** Host Sleep activated */
#define HS_ACTIVATED			0x01
/** Host Sleep deactivated */
#define HS_DEACTIVATED			0x00

/** Power Save sleep */
#define PS_SLEEP			0x01
/** Power Save awake */
#define PS_AWAKE			0x00

typedef struct _BT_CMD
{
    /** OCF OGF */
    u16 ocf_ogf;
    /** Length */
    u8 length;
    /** Data */
    u8 data[4];
} __ATTRIB_PACK__ BT_CMD;

typedef struct _BT_EVENT
{
    /** Event Counter */
    u8 EC;
    /** Length */
    u8 length;
    /** Data */
    u8 data[4];
} BT_EVENT;

void check_evtpkt(bt_private * priv, struct sk_buff *skb);

/* Prototype of global function */
bt_private *bt_add_card(void *card);
int bt_remove_card(void *card);
void bt_interrupt(struct hci_dev *hdev);

int bt_proc_init(bt_private * priv);
void bt_proc_remove(bt_private * priv);
int bt_process_event(bt_private * priv, struct sk_buff *skb);
int bt_enable_hs(bt_private * priv);
int bt_prepare_command(bt_private * priv);

int *sbi_register(void);
void sbi_unregister(void);
int sbi_register_dev(bt_private * priv);
int sbi_unregister_dev(bt_private * priv);
int sbi_host_to_card(bt_private * priv, u8 * payload, u16 nb);
int sbi_enable_host_int(bt_private * priv);
int sbi_disable_host_int(bt_private * priv);
int sbi_dowload_fw(bt_private * priv);
int sbi_get_int_status(bt_private * priv, u8 * ireg);
int sbi_wakeup_firmware(bt_private * priv);
#endif /* _BT_DRV_H_ */
