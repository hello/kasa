/** @file wlan_decl.h
 *  @brief This file contains declaration referring to
 *  functions defined in other source files
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
/******************************************************
Change log:
	09/29/05: add Doxygen format comments
	01/05/06: Add kernel 2.6.x support	
	01/11/06: Conditionalize new scan/join structures.
	          Move wlan_wext statics to their source file.
******************************************************/

#ifndef _WLAN_DECL_H_
#define _WLAN_DECL_H_

/** Function Prototype Declaration */
int wlan_init_fw(wlan_private * priv);
void wlan_init_adapter(wlan_private * priv);
int wlan_init_sw(wlan_private * priv);
int wlan_tx_packet(wlan_private * priv, struct sk_buff *skb);
void wlan_free_adapter(wlan_private * priv);
void wlan_clear_pending_cmd(wlan_private * priv);

int wlan_send_null_packet(wlan_private * priv, u8 flags);
BOOLEAN wlan_check_last_packet_indication(wlan_private * priv);

void Wep_encrypt(wlan_private * priv, u8 * Buf, u32 Len);
int wlan_free_cmd_buffer(wlan_private * priv);
void wlan_clean_cmd_node(CmdCtrlNode * pTempNode);
CmdCtrlNode *wlan_get_cmd_node(wlan_private * priv);

void wlan_init_cmd_node(wlan_private * priv,
                        CmdCtrlNode * pTempNode,
                        WLAN_OID cmd_oid, u16 wait_option, void *pdata_buf);

BOOLEAN wlan_is_cmd_allowed(wlan_private * priv);

int wlan_prepare_cmd(wlan_private * priv,
                     u16 cmd_no,
                     u16 cmd_action,
                     u16 wait_option, WLAN_OID cmd_oid, void *pdata_buf);

void wlan_insert_cmd_to_pending_q(wlan_adapter * Adapter, CmdCtrlNode * CmdNode,
                                  BOOLEAN addtail);

int wlan_alloc_cmd_buffer(wlan_private * priv);
int wlan_exec_next_cmd(wlan_private * priv);
int wlan_process_event(wlan_private * priv);
void wlan_interrupt(wlan_private *);
u32 index_to_data_rate(u8 index);
u8 data_rate_to_index(u32 rate);
int get_active_data_rates(wlan_adapter * Adapter, WLAN_802_11_RATES rates);
int wlan_radio_ioctl(wlan_private * priv, u8 option);
int wlan_change_adhoc_chan(wlan_private * priv, int channel);
int wlan_set_wpa_ie_helper(wlan_private * priv, u8 * ie_data_ptr, u16 ie_len);
int wlan_set_gen_ie_helper(wlan_private * priv, u8 * ie_data_ptr, u16 ie_len);
int wlan_get_gen_ie_helper(wlan_private * priv, u8 * ie_data_ptr,
                           u16 * ie_len_ptr);
void get_version(wlan_adapter * adapter, char *version, int maxlen);
void wlan_read_write_rfreg(wlan_private * priv);

#ifdef CONFIG_PROC_FS
/** The proc fs interface */
void wlan_proc_entry(wlan_private * priv, struct net_device *dev);
void wlan_proc_remove(wlan_private * priv);
int string_to_number(char *s);
#ifdef PROC_DEBUG
void wlan_debug_entry(wlan_private * priv, struct net_device *dev);
void wlan_debug_remove(wlan_private * priv);
#endif /* PROC_DEBUG */
#endif /* CONFIG_PROC_FS */
int wlan_process_cmdresp(wlan_private * priv);
void wlan_process_tx(wlan_private * priv, struct sk_buff *skb);
void wlan_insert_cmd_to_free_q(wlan_private * priv, CmdCtrlNode * pTempCmd);
void wlan_cmd_timeout_func(void *FunctionContext);

#ifdef REASSOCIATION
void wlan_reassoc_timer_func(void *FunctionContext);
#endif /* REASSOCIATION */

int wlan_set_essid(struct net_device *dev, struct iw_request_info *info,
                   struct iw_point *dwrq, char *extra);
int wlan_set_regiontable(wlan_private * priv, u8 region, u8 band);

void wlan_clean_txrx(wlan_private * priv);

int wlan_host_sleep_activated_event(wlan_private * priv);
int wlan_host_sleep_deactivated_event(wlan_private * priv);
int wlan_host_sleep_wakeup_event(wlan_private * priv);
int wlan_deep_sleep_ioctl(wlan_private * priv, struct ifreq *rq);
int wlan_exit_deep_sleep_timeout(wlan_private * priv);

int wlan_process_rx_packet(wlan_private * priv, struct sk_buff *);

void wlan_enter_ps(wlan_private * priv, int wait_option);
void wlan_ps_cond_check(wlan_private * priv, u16 PSMode);
void wlan_exit_ps(wlan_private * priv, int wait_option);

extern CHANNEL_FREQ_POWER *find_cfp_by_band_and_channel(wlan_adapter * adapter,
                                                        u8 band, u16 channel);
extern CHANNEL_FREQ_POWER *get_cfp_by_band_and_channel(u8 band, u16 channel,
                                                       REGION_CHANNEL *
                                                       region_channel);

CHANNEL_FREQ_POWER *wlan_get_region_cfp_table(u8 region, u8 band, int *cfp_no);

extern void wlan_reset_connect_state(wlan_private * priv);

#if WIRELESS_EXT > 14
void send_iwevcustom_event(wlan_private * priv, s8 * str);
#endif

/* Macros and Functions in interface module */
/** INT Status Bit Definition : Rx upload ready */
#define HIS_RxUpLdRdy			BIT(0)
/** INT Status Bit Definition : Tx download ready */
#define HIS_TxDnLdRdy			BIT(1)
/** INT Status Bit Definition : Command download ready */
#define HIS_CmdDnLdRdy			BIT(2)
/** INT Status Bit Definition : Card event */
#define HIS_CardEvent			BIT(3)
/** INT Status Bit Definition : Command upload ready */
#define HIS_CmdUpLdRdy			BIT(4)

/** Firmware ready */
#define FIRMWARE_READY			0xfedc
/** Length of device name */
#ifndef DEV_NAME_LEN
#define DEV_NAME_LEN			32
#endif
/** Maximum length of key */
#define MAXKEYLEN			13

/** The number of times to try when polling for status bits */
#define MAX_POLL_TRIES			100

/** The number of times to try when waiting for downloaded firmware to 
     become active. (polling the scratch register). */

#define MAX_FIRMWARE_POLL_TRIES		100

/** Number of firmware blocks to transfer */
#define FIRMWARE_TRANSFER_NBLOCK	2

/** Packet type: data, command & event */
typedef enum _mv_type
{
    MV_TYPE_DAT = 0,
    MV_TYPE_CMD = 1,
    MV_TYPE_EVENT = 3
} mv_type;

/** add card */
wlan_private *wlan_add_card(void *card);
/** remove card */
int wlan_remove_card(void *card);

/** PM callback */
int wlan_pm(void *card, int suspend);

/** Register SBI functions */
int sbi_register(void);
/** Unregister SBI function */
void sbi_unregister(void);
/** Register SBI device */
int sbi_register_dev(wlan_private * priv);
/** Unregister SBI device */
int sbi_unregister_dev(wlan_private *);
int sbi_disable_host_int(wlan_private * priv);
/** Get interrupt status */
int sbi_get_int_status(wlan_private * priv, u8 *);
int sbi_prog_firmware(wlan_private *);
/** Check firmware status */
int sbi_check_fw_status(wlan_private *, int);

int sbi_prog_helper(wlan_private *);
int sbi_prog_fw_w_helper(wlan_private *);

/** Send data from host to card */
int sbi_host_to_card(wlan_private * priv, u8 type, u8 * payload, u16 nb);
int sbi_enable_host_int(wlan_private *);

int sbi_exit_deep_sleep(wlan_private *);
int sbi_reset_deepsleep_wakeup(wlan_private *);

int sbi_read_ioreg(wlan_private * priv, u32 reg, u8 * dat);
int sbi_write_ioreg(wlan_private * priv, u32 reg, u8 dat);
int sbi_set_bus_clock(wlan_private * priv, u8 option);

/** Get CIS information */
int sbi_get_cis_info(wlan_private * priv, void *cisinfo, int *cislen);

#endif /* _WLAN_DECL_H_ */
