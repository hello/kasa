/*
 * Copyright (c) 2011 Atheros Communications Inc.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef DEBUG_H
#define DEBUG_H

#include "hif.h"

enum ATH6KL_MODULE_QUIRKS {
	/* enable suspend cutpower */
	ATH6KL_MODULE_SUSPEND_CUTPOWER  = BIT(0),

	/* enable lpl */
	AT6HKL_MODULE_LPL_ENABLE        = BIT(1),

	/* enable mimo ps */
	ATH6KL_MODULE_MIMO_PS_ENABLE    = BIT(2),

	/* disable RX aggregation drop packets */
	ATH6KL_MODULE_DISABLE_RX_AGGR_DROP    = BIT(3),

	/* enable test mode */
	ATH6KL_MODULE_TESTMODE_ENABLE   = BIT(4),

	/* anti-noise-immunity */
	ATH6KL_MODULES_ANI_ENABLE       = BIT(5),

	/* enable endpoing loopback */
	ATH6KL_MODULE_ENABLE_EPPING		= BIT(6),

	/* disable 5G support */
	ATH6KL_MODULE_DISABLE_5G		= BIT(7),

	/* enable max. fw vif */
	ATH6KL_MODULE_P2P_MAX_FW_VIF	= BIT(8),

	/* Disable skb_copy when clone/duplicate*/
	ATH6KL_MODULE_DISABLE_SKB_DUP	= BIT(9),

	/* enable usb remote wakeup support */
	ATH6KL_MODULE_ENABLE_USB_REMOTE_WKUP = BIT(10),

	/* disable 2.4G HT40 in default */
	ATH6KL_MODULE_DISABLE_2G_HT40	= BIT(11),

	/* enable fwlog by default */
	ATH6KL_MODULE_ENABLE_FWLOG	= BIT(12),

	/* Disable USB Auto-suspend */
	ATH6KL_MODULE_DISABLE_USB_AUTO_SUSPEND = BIT(13),

	/* Enable single chain in wow */
	ATH6KL_MODULE_ENABLE_WOW_SINGLE_CHAIN = BIT(14),

	/* offload AP keep-alive to supplicant */
	ATH6KL_MODULE_KEEPALIVE_BY_SUPP	= BIT(15),

	/* disable create virtual interface automatically */
	ATH6KL_MODULE_DISABLE_AUTO_ADD_INF = BIT(16),

	/* enable sche-scan */
	ATH6KL_MODULE_ENABLE_SCHE_SCAN  = BIT(17),

	/* enable extensive fwlog */
	ATH6KL_MODULE_ENABLE_FWLOG_EXT  = BIT(18),

	/* enable channel-mode select for P2P-GO */
	ATH6KL_MODULE_ENABLE_P2P_CHANMODE  = BIT(19),

	/* not to wait init defer function completed */
	ATH6KL_MODULE_DISABLE_WAIT_DEFER = BIT(20),

	/* enable fw crash notify function */
	ATH6KL_MODULE_ENABLE_FW_CRASH_NOTIFY = BIT(21),

	/* disable usb auto power management feature */
	ATH6KL_MODULE_DISABLE_USB_AUTO_PM = BIT(22),

	/* disable wmi sync mechanism */
	ATH6KL_MODULE_DISABLE_WMI_SYC = BIT(23),

	/* hole */

	/* Config AP keep-alive from supplicant */
	ATH6KL_MODULE_KEEPALIVE_CONFIG_BY_SUPP	= BIT(25),

	/* hole */

	/* config AP-ACL from NL80211 */
	ATH6KL_MODULE_AP_ACL_BY_NL80211  = BIT(27),

	/* enable Diagnostic */
	ATH6KL_MODULE_ENABLE_DIAGNOSTIC = BIT(28),

	/* enable RTT */
	ATH6KL_MODULE_ENABLE_RTT = BIT(29),
};

enum ATH6KL_MODULE_P2P {
	/* enable single interface p2p */
	ATH6KL_MODULEP2P_P2P_ENABLE			= BIT(0),

	/* enable p2p_concurrent, with dedicate */
	ATH6KL_MODULEP2P_CONCURRENT_ENABLE_DEDICATE	= BIT(1),

	/* enable p2p_concurrent, no dedicate */
	ATH6KL_MODULEP2P_CONCURRENT_NO_DEDICATE		= BIT(2),

	/* enable p2p_concurrent, ath6kl-3.2 compatible */
	ATH6KL_MODULEP2P_CONCURRENT_COMPAT		= BIT(3),

	/* enable p2p_concurrent with multi-channel */
	ATH6KL_MODULEP2P_CONCURRENT_MULTICHAN		= BIT(4),

	/* hole */

	/* enable p2p_concurrent with softAP */
	ATH6KL_MODULEP2P_CONCURRENT_AP			= BIT(8),

	/* enable p2p_in_pasv_chan */
	ATH6KL_MODULEP2P_P2P_IN_PASSIVE_CHAN		= BIT(9),

	/* enable p2p_wise_scan */
	ATH6KL_MODULEP2P_P2P_WISE_SCAN			= BIT(10),
};

enum ATH6KL_MODULE_ROAM {
	/* Not set, obey driver default */
	ATH6KL_MODULEROAM_DEFAULT		= BIT(0),

	/* Enable roam but disable at multiple connection */
	ATH6KL_MODULEROAM_NO_LRSSI_SCAN_AT_MULTI		= BIT(1),

	/* Enabel roam but disable lrssi scan */
	ATH6KL_MODULEROAM_DISABLE_LRSSI_SCAN		= BIT(2),

	/* Enable roam at all time */
	ATH6KL_MODULEROAM_ENABLE_ALL		= BIT(3),

	/* Disable roam */
	ATH6KL_MODULEROAM_DISABLE		= BIT(4),
};

/* debug_mask */
enum ATH6K_DEBUG_MASK {
	ATH6KL_DBG_CREDIT	= BIT(0),
	ATH6KL_DBG_REGDB	= BIT(1),
	ATH6KL_DBG_WLAN_TX      = BIT(2),     /* wlan tx */
	ATH6KL_DBG_WLAN_RX      = BIT(3),     /* wlan rx */
	ATH6KL_DBG_BMI		= BIT(4),     /* bmi tracing */
	ATH6KL_DBG_HTC		= BIT(5),
	ATH6KL_DBG_HIF		= BIT(6),
	ATH6KL_DBG_IRQ		= BIT(7),     /* interrupt processing */
	ATH6KL_DBG_ACL		= BIT(8),     /* access control list */
	ATH6KL_DBG_ADMC		= ATH6KL_DBG_ACL,     /* admission control */
	ATH6KL_DBG_RC		= BIT(9),     /* P2P recommend channel */
	ATH6KL_DBG_AP_RC	= ATH6KL_DBG_RC,      /* AP recommend channel */
	ATH6KL_DBG_WMI          = BIT(10),    /* wmi tracing */
	ATH6KL_DBG_TRC	        = BIT(11),    /* generic func tracing */
	ATH6KL_DBG_SCATTER	= BIT(12),    /* hif scatter tracing */
	ATH6KL_DBG_WLAN_CFG     = BIT(13),    /* cfg80211 i/f file tracing */
	ATH6KL_DBG_RAW_BYTES    = BIT(14),    /* dump tx/rx frames */
	ATH6KL_DBG_AGGR		= BIT(15),    /* aggregation */
	ATH6KL_DBG_SDIO		= BIT(16),
	ATH6KL_DBG_SDIO_DUMP	= BIT(17),
	ATH6KL_DBG_BOOT		= BIT(18),    /* driver init and fw boot */
	ATH6KL_DBG_WMI_DUMP	= BIT(19),
	ATH6KL_DBG_SUSPEND	= BIT(20),
	ATH6KL_DBG_USB		= BIT(21),
	ATH6KL_DBG_USB_BULK	= BIT(22),
	ATH6KL_DBG_HTCOEX	= BIT(23),		/* HT20/40 Coexist */
	ATH6KL_DBG_WLAN_TX_AMSDU = BIT(24),    /* wlan tx a-msdu */
	ATH6KL_DBG_POWERSAVE	= BIT(25),	/* power-saving */
	ATH6KL_DBG_WOWLAN	= BIT(26),    /* wowlan tracing */
	ATH6KL_DBG_RTT          = BIT(27),
	ATH6KL_DBG_FLOWCTRL     = BIT(28),    /* P2P flowctrl */
	ATH6KL_DBG_KEEPALIVE    = BIT(29),    /* AP keep-alive */
	ATH6KL_DBG_ACS       = BIT(30),              /* ACS */
	ATH6KL_DBG_DA       = ATH6KL_DBG_ACS,              /* Direct Audio */
	ATH6KL_DBG_INFO		= BIT(31),    /* keep last */
	ATH6KL_DBG_ANY		= 0xffffffff  /* enable all logs */
};

/* debug_mask_ext */
#define BIT_OFFSET32(nr)	(1ULL << ((nr) + 32))

enum ATH6K_DEBUG_MASK_EXT {
	ATH6KL_DBG_EXT_INFO1	= BIT_OFFSET32(0),
	ATH6KL_DBG_EXT_ROC	= BIT_OFFSET32(1),	/* Remain-on-Channel */
	ATH6KL_DBG_EXT_SCAN	= BIT_OFFSET32(2),	/* Scan */
	ATH6KL_DBG_EXT_BSS_PROC	= BIT_OFFSET32(3),	/* BSS post-proc */

	ATH6KL_DBG_EXT_ANY	= 0xffffffff00000000ULL  /* enable all logs */
};

extern unsigned int debug_mask;
extern unsigned int debug_mask_ext;
extern unsigned int debug_quirks;
extern __printf(2, 3)
int ath6kl_printk(const char *level, const char *fmt, ...);

#define ath6kl_info(fmt, ...)				\
	ath6kl_printk(KERN_INFO, fmt, ##__VA_ARGS__)
#define ath6kl_err(fmt, ...)					\
	ath6kl_printk(KERN_ERR, fmt, ##__VA_ARGS__)
#define ath6kl_warn(fmt, ...)					\
	ath6kl_printk(KERN_WARNING, fmt, ##__VA_ARGS__)
#define ath6kl_debug(fmt, ...)				\
	printk(KERN_DEBUG fmt, ##__VA_ARGS__)

#define AR_DBG_LVL_CHECK(mask)	(debug_mask & mask)

enum ath6kl_war {
	ATH6KL_WAR_INVALID_RATE,
};

static inline int ath6kl_mod_debug_quirks(struct ath6kl *ar,
	enum ATH6KL_MODULE_QUIRKS mask)
{
	if (ar->mod_debug_quirks & mask)
		return 1;
	else
		return 0;
}

void ath6kl_send_event_to_app(struct net_device *dev,
					u16 event_id, u8 ifid,
					u8 *datap, int len);

void ath6kl_send_genevent_to_app(struct net_device *dev,
					u16 event_id, u8 ifid,
					u8 *datap, int len);

#ifdef CONFIG_ATH6KL_DEBUG
#define ath6kl_dbg(mask, fmt, ...)					\
	({								\
	 int rtn;							\
	 if ((debug_mask & (unsigned int)(mask)) ||	\
		 (debug_mask_ext &	\
		 (unsigned int)((unsigned long long)(mask)>>32)))	\
		rtn = ath6kl_printk(KERN_DEBUG, fmt, ##__VA_ARGS__);	\
	 else								\
		rtn = 0;						\
									\
	 rtn;								\
	 })

static inline void ath6kl_dbg_dump(enum ATH6K_DEBUG_MASK mask,
				   const char *msg, const char *prefix,
				   const void *buf, size_t len)
{
	if (debug_mask & mask) {
		if (msg)
			ath6kl_dbg(mask, "%s\n", msg);

		print_hex_dump_bytes(prefix, DUMP_PREFIX_OFFSET, buf, len);
	}
}

void ath6kl_dump_registers(struct ath6kl_device *dev,
			   struct ath6kl_irq_proc_registers *irq_proc_reg,
			   struct ath6kl_irq_enable_reg *irq_en_reg);
void dump_cred_dist_stats(struct htc_target *target);
void ath6kl_debug_fwlog_event(struct ath6kl *ar, const void *buf, size_t len);
void ath6kl_debug_war(struct ath6kl *ar, enum ath6kl_war war);
int ath6kl_debug_roam_tbl_event(struct ath6kl *ar, const void *buf,
				size_t len);
void ath6kl_debug_set_keepalive(struct ath6kl *ar, u8 keepalive);
void ath6kl_debug_set_disconnect_timeout(struct ath6kl *ar, u8 timeout);
int ath6kl_debug_init(struct ath6kl *ar);
void ath6kl_debug_cleanup(struct ath6kl *ar);

#else
static inline int ath6kl_dbg(unsigned long long int dbg_mask,
			     const char *fmt, ...)
{
	return 0;
}

static inline void ath6kl_dbg_dump(enum ATH6K_DEBUG_MASK mask,
				   const char *msg, const char *prefix,
				   const void *buf, size_t len)
{
}

static inline void ath6kl_dump_registers(struct ath6kl_device *dev,
		struct ath6kl_irq_proc_registers *irq_proc_reg,
		struct ath6kl_irq_enable_reg *irq_en_reg)
{

}
static inline void dump_cred_dist_stats(struct htc_target *target)
{
}

static inline void ath6kl_debug_fwlog_event(struct ath6kl *ar,
					    const void *buf, size_t len)
{
}

static inline void ath6kl_debug_war(struct ath6kl *ar, enum ath6kl_war war)
{
}

static inline int ath6kl_debug_roam_tbl_event(struct ath6kl *ar,
					      const void *buf, size_t len)
{
	return 0;
}

static inline void ath6kl_debug_set_keepalive(struct ath6kl *ar, u8 keepalive)
{
}

static inline void ath6kl_debug_set_disconnect_timeout(struct ath6kl *ar,
						       u8 timeout)
{
}

static inline int ath6kl_debug_init(struct ath6kl *ar)
{
	return 0;
}

static inline void ath6kl_debug_cleanup(struct ath6kl *ar)
{
}

#endif
#endif
