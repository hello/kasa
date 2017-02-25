/*******************************************************************************
 * brcm_ioc.h
 *
 * History:
 *    2015/8/8 - [Tao Wu] Create
 *
 * Copyright (c) 2016 Ambarella, Inc.
 *
 * This file and its contents ( "Software" ) are protected by intellectual
 * property rights including, without limitation, U.S. and/or foreign
 * copyrights. This Software is also the confidential and proprietary
 * information of Ambarella, Inc. and its licensors. You may not use, reproduce,
 * disclose, distribute, modify, or otherwise prepare derivative works of this
 * Software or any portion thereof except pursuant to a signed license agreement
 * or nondisclosure agreement with Ambarella, Inc. or its authorized affiliates.
 * In the absence of such an agreement, you agree to promptly notify and return
 * this Software to Ambarella, Inc.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF NON-INFRINGEMENT,
 * MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL AMBARELLA, INC. OR ITS AFFILIATES BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; COMPUTER FAILURE OR MALFUNCTION; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
******************************************************************************/

#ifndef _BRCM_IOCTL_H_
#define _BRCM_IOCTL_H_

#include <brcm_types.h>
#include <log_level.h>
#include <net_util.h>

#define BRCM_IOCTL_VERSION	"1.0.4"
#define BRCM_TOOL		"wl"

#define CMD_MAXSIZE		(512)
#define CMD_MINSIZE		(128)

#define WLC_IOCTL_MAXLEN	(8192)	/* max length ioctl buffer required */
#define WLC_IOCTL_SMLEN	(256)	/* "small" length ioctl buffer required */
#define WLC_IOCTL_MEDLEN	(1536)    /* "med" length ioctl buffer required */

#define WLC_GET_VAR		(262)
#define WLC_SET_VAR		(263)

#define WLC_GET_VERSION	(1)
#define WLC_GET_REVINFO	(98)

#define WLC_GET_BCNPRD	(75)
#define WLC_GET_DTIMPRD	(77)

#define WLC_GET_PM			(85)
#define WLC_SET_PM			(86)

/* Linux network driver ioctl encoding */
typedef struct wl_ioctl {
	uint cmd;	/* common ioctl definition */
	void *buf;	/* pointer to user buffer */
	uint len;		/* length of user buffer */
	unsigned char set;		/* 1=set IOCTL; 0=query IOCTL */
	uint used;	/* bytes read or written (optional) */
	uint needed;	/* bytes needed (optional) */
} wl_ioctl_t;

#pragma pack(4)
typedef struct tcpka_conn {
	uint32 sess_id;
	struct ether_addr dst_mac; /* Destinition Mac */
	struct ipv4_addr  src_ip;   	/* Sorce IP */
	struct ipv4_addr  dst_ip;   	/* Destinition IP */

	uint16 ipid;		/* Ip Identification */
	uint16 srcport;	/* Source Port Address */
	uint16 dstport;	/* Destination Port Address */
	uint32 seq;		/* TCP Sequence Number */
	uint32 ack;		/* TCP Ack Number */
	uint16 tcpwin;	/* TCP window */
	uint32 tsval;		/* Timestamp Value */
	uint32 tsecr;		/* Timestamp Echo Reply */
	uint32 len;                /* last packet payload len */
	uint32 ka_payload_len;     /* keep alive payload length */
	uint8  ka_payload[1];      /* keep alive payload */
} tcpka_conn_t;

#pragma pack(1)
typedef struct wl_mtcpkeep_alive_timers_pkt {
	uint16 interval;		/* interval timer, unit: senconds */
	uint16 retry_interval;	/* retry_interval timer */
	uint16 retry_count;	/* retry_count */
} wl_mtcpkeep_alive_timers_pkt_t;

typedef struct tcpka_conn_sess_ctl {
	uint32 sess_id;	/* session id */
	uint32 flag;		/* enable/disable flag */
} tcpka_conn_sess_ctl_t;

typedef struct tcpka_conn_sess {
	uint32 sess_id;	/* session id */
	uint32 flag;		/* enable/disable flag */
	wl_mtcpkeep_alive_timers_pkt_t  tcp_keepalive_timers;
} tcpka_conn_sess_t;

#define WL_PKT_FILTER_FIXED_LEN		  OFFSETOF(wl_pkt_filter_t, u)
#define WL_PKT_FILTER_PATTERN_FIXED_LEN	  OFFSETOF(wl_pkt_filter_pattern_t, mask_and_pattern)

typedef enum {
	wowl_pattern_type_bitmap = 0,
	wowl_pattern_type_arp,
	wowl_pattern_type_na
} wowl_pattern_type_t;

typedef struct wl_wowl_pattern_s {
	uint		masksize;	/* Size of the mask in #of bytes */
	uint		offset;		/* Pattern byte offset in packet */
	uint		patternoffset;/* Offset of start of pattern in the structure */
	uint		patternsize;	/* Size of the pattern itself in #of bytes */
	ulong	id;			/* id */
	uint		reasonsize;	/* Size of the wakeup reason code */
	//wowl_pattern_type_t type;		/* Type of pattern */
	/* Mask follows the structure above */
	/* Pattern follows the mask is at 'patternoffset' from the start */
} wl_wowl_pattern_t;

typedef struct wl_wowl_pattern_s_bcm43438 {
	uint		masksize;	/* Size of the mask in #of bytes */
	uint		offset;		/* Pattern byte offset in packet */
	uint		patternoffset;/* Offset of start of pattern in the structure */
	uint		patternsize;	/* Size of the pattern itself in #of bytes */
	ulong	id;			/* id */
	uint		reasonsize;	/* Size of the wakeup reason code */
	wowl_pattern_type_t type;		/* Type of pattern */
	/* Mask follows the structure above */
	/* Pattern follows the mask is at 'patternoffset' from the start */
} wl_wowl_pattern_t_bcm43438;

#define	OFFSETOF(type, member)	((uint)(uintptr)&((type *)0)->member)
typedef struct wl_mkeep_alive_pkt {
	uint16	version;		/* Version for mkeep_alive */
	uint16	length;		/* length of fixed parameters in the structure */
	uint32	period_msec;/* milliseconds */
	uint16	len_bytes;
	uint8	keep_alive_id; /* 0 - 3 for N = 4 */
	uint8	data[1];
} wl_mkeep_alive_pkt_t;

#define WL_MKEEP_ALIVE_VERSION		1
#define WL_MKEEP_ALIVE_FIXED_LEN	OFFSETOF(wl_mkeep_alive_pkt_t, data)
typedef struct wl_pkt_filter_pattern {
	uint32	offset;	/* Offset within received packet to start pattern matching.
					 * Offset '0' is the first byte of the ethernet header.*/
	uint32	size_bytes;	/* Size of the pattern.  Bitmask must be the same size. */
	uint8   mask_and_pattern[1]; /* Variable length mask and pattern data.  mask starts
								* at offset 0.  Pattern immediately follows mask.*/
} wl_pkt_filter_pattern_t;

typedef struct wl_pkt_filter {
	uint32	id;		/* Unique filter id, specified by app. */
	uint32	type;	/* Filter type (WL_PKT_FILTER_TYPE_xxx). */
	uint32	negate_match;	/* Negate the result of filter matches */
	union {			/* Filter definitions */
		wl_pkt_filter_pattern_t pattern;	/* Pattern matching filter */
	} u;
} wl_pkt_filter_t;

#define WL_PKT_FILTER_FIXED_LEN		  OFFSETOF(wl_pkt_filter_t, u)
#define WL_PKT_FILTER_PATTERN_FIXED_LEN	  OFFSETOF(wl_pkt_filter_pattern_t, mask_and_pattern)
typedef struct wl_pkt_filter_enable {
	uint32	id;		/* Unique filter id */
	uint32	enable;	/* Enable/disable bool */
} wl_pkt_filter_enable_t;

typedef struct wl_pkt_filter_stats {
	uint32	num_pkts_matched;		/* # filter matches for specified filter id */
	uint32	num_pkts_forwarded;	/* # packets fwded from dongle to host for all filters */
	uint32	num_pkts_discarded;	/* # packets discarded by dongle for all filters */
} wl_pkt_filter_stats_t;

typedef struct tcpka_conn_info {
	uint32 tcpka_sess_ipid;
	uint32 tcpka_sess_seq;
	uint32 tcpka_sess_ack;
} tcpka_conn_sess_info_t;

#pragma pack(4)

#define WL_WOWL_MAGIC	(1 << 0)	/* Wakeup on Magic packet */
#define WL_WOWL_NET	(1 << 1)	/* Wakeup on Netpattern */
#define WL_WOWL_DIS	(1 << 2)	/* Wakeup on loss-of-link due to Disassoc/Deauth */
#define WL_WOWL_RETR	(1 << 3)	/* Wakeup on retrograde TSF */
#define WL_WOWL_BCN	(1 << 4)	/* Wakeup on loss of beacon */
#define WL_WOWL_BCAST	(1 << 15)	/* If the bit is set, frm received was bcast frame */
#define WL_WOWL_TCPKEEP_TIME    (1 << 17)   /* Wakeup on tcpkeep alive timeout */
#define WL_WOWL_TCPKEEP_DATA    (1 << 20)   /* tcp keepalive got data */
#define WL_WOWL_TCPFIN                  (1 << 26)   /* tcp keepalive got FIN */

typedef struct wl_wowl_wakeind {
	uint32	pci_wakeind;	/* Whether PCI PMECSR PMEStatus bit was set */
	uint32	ucode_wakeind;	/* What wakeup-event indication was set by ucode */
} wl_wowl_wakeind_t;

#define MAX_CIPHER_LEN (64)
typedef struct wlc_host_cipher {
    int len;
    uint8 payload[MAX_CIPHER_LEN];
}wlc_host_cipher_t;

int wl_tcpka_conn_add(tcpka_conn_t *p_tcpka);
int wl_tcpka_conn_enable(uint32 sess_id, uint32 is_enable,
	uint16 itrvl, uint16 retry_itrvl, uint16 rety_cnt);
int wl_tcpka_conn_sess_info(uint sess_id, tcpka_conn_sess_info_t *tcpka_sess_info);
int wl_tcpka_conn_del(uint sess_id);

int wl_wowl_pattern_bcm43340(int offset, char *mask, char *wowl_pattern);
int wl_wowl_pattern_bcm43438(int offset, char *mask, char *wowl_pattern);
int wl_wowl_pattern(int offset, char *mask, char *wowl_pattern);
int wl_wowl_pattern_clr(void);
int wl_wowl_wakeind(wl_wowl_wakeind_t *wakeind);
int wl_wowl_wakeind_clear(void);

int wl_wowl(uint32 flag);
int wl_wowl_activate(uint32 is_enable);
int wl_wowl_clear(void);

int wl_mkeep_alive(uint8 sess_id, uint32 interval, uint32 length, const u_char *packet);
int wl_pkt_filter_add(int id, int offset, char *mask, char *filter_pattern);
int wl_pkt_filter_enable(uint32 id, uint32 is_enable);
int wl_pkt_filter_delete(uint32 id);
int wl_pkt_filter_stats(uint32 id, wl_pkt_filter_stats_t *filter_stats);

int wl_set_get_pm_mode(int *mode, int is_set);
int wl_set_get_host_sleep(int *sleep, int is_set);
int wl_set_get_host_cipher(uint8 *cipher_str, uint32 len, int is_set);

int wl_get_ap_beacon_interval(int *ap_beacon_interval);
int wl_get_ap_dtim(int *ap_dtim);
int wl_set_bcn_li_dtim(int bcn_li_dtim);
int wl_set_dtim_interval(int dtim_interval);

void brcm_ioc_wowl_init(char* iface, wifi_chip_t chip, int sys_exec);

#endif
