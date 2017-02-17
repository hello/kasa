/**
 * eth/network.h
 *
 * Networking support in AMBoot.
 *
 * History:
 *    2006/06/19 - [Charles Chiou] created file
 *    2008/02/19 - [Allen Wang] changed to use capabilities and chip ID
 *
 *
 * Copyright (c) 2015 Ambarella, Inc.
 *
 * This file and its contents ("Software") are protected by intellectual
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
 */


#ifndef __NETWORK_H__
#define __NETWORK_H__

/*===========================================================================*/
#include <eth/tftp.h>

/*===========================================================================*/
#define MAX_ARP_TAB	        16

#define SPEED_0			0
#define SPEED_10		10
#define SPEED_100		100
#define SPEED_1000		1000

#define DUPLEX_HALF		0x00
#define DUPLEX_FULL		0x01

#define DEFAULT_NET_WAIT_TMO	(2000)

/*===========================================================================*/
typedef u32 bld_ip_t;

/**
 * Ethernet frame.
 */
typedef struct ethhdr_s
{
	u8 dst[6];
	u8 src[6];
	u16 type;
} __attribute__((packed)) ethhdr_t;

/**
 * ARP request packet.
 */
typedef struct arpreq_s
{
	ethhdr_t ethhdr;
	u16 hwtype;
	u16 protocol;
	u8 hwlen;
	u8 protolen;
	u16 opcode;
	u8 shwaddr[6];
	u32 sipaddr;
	u8 thwaddr[6];
	u32 tipaddr;
} __attribute__((packed)) arpreq_t;

/**
 * IP header.
 */
typedef struct iphdr_s
{
        u8 verhdrlen;
        u8 service;
        u16 len;
        u16 ident;
        u16 frags;
        u8 ttl;
        u8 protocol;
        u16 chksum;
        u32 src;
        u32 dest;
} __attribute__((packed)) iphdr_t;

/**
 * UDP header.
 */
typedef struct udphdr_s
{
        iphdr_t iphdr;
        u16 src;
        u16 dest;
        u16 len;
        u16 chksum;
} __attribute__((packed)) udphdr_t;

/**
 * ICMP echo header.
 */
typedef struct icmp_echo_hdr_s
{
        iphdr_t iphdr;
        u8 type;
        u8 code;
        u16 chksum;
        u16 id;
        u16 seqno;
}__attribute__((packed)) icmp_echo_hdr_t;

/**
 * ARP table.
 */
typedef struct bld_arp_tab_s
{
	bld_ip_t ip;
	u8 hwaddr[6];
} bld_arp_tab_t;

/**
 * The DMA descriptor used by the Ethernet controller.
 */
typedef struct ethdma_desc_s
{
	u32 status;	/**< Status */
	u32 length;	/**< Buffer length */
	u32 buffer1;	/**< Pointer to buffer 1 */
	u32 buffer2;	/**< Pointer to buffer 2 or next descriptor in the chain */
#if(ETH_ENHANCED == 1)
	u32 des4;
	u32 des5;
	u32 des6;
	u32 des7;
#endif
} ethdma_desc_t;

/*===========================================================================*/
#define MIN_FRAMES_SIZE		(TFTP_SEGSIZE + 64)
#define MAX_FRAMES_SIZE		(1536)

#if !defined(BOARD_ETH_TX_FRAMES)
#define BOARD_ETH_TX_FRAMES	4
#endif
#if !defined(BOARD_ETH_RX_FRAMES)
#define BOARD_ETH_RX_FRAMES	8
#endif

#if !defined(BOARD_ETH_FRAMES_SIZE)
#define BOARD_ETH_FRAMES_SIZE	MIN_FRAMES_SIZE
#endif
#if ((BOARD_ETH_FRAMES_SIZE < MIN_FRAMES_SIZE) || (BOARD_ETH_FRAMES_SIZE > MAX_FRAMES_SIZE))
#warning "Wrong BOARD_ETH_FRAMES_SIZE, use default!"
#undef BOARD_ETH_FRAMES_SIZE
#define BOARD_ETH_FRAMES_SIZE	MIN_FRAMES_SIZE
#endif

/**
 * Ethernet device.
 */
typedef struct bld_eth_dev_s
{
	ethdma_desc_t txd[BOARD_ETH_TX_FRAMES];	/**< TX descriptor */
	ethdma_desc_t rxd[BOARD_ETH_RX_FRAMES];	/**< RX descriptor */

	int tx_cur;
	int rx_cur;

	struct {
		u8 buf[BOARD_ETH_FRAMES_SIZE];
	} tx[BOARD_ETH_TX_FRAMES];

	struct {
		u8 buf[BOARD_ETH_FRAMES_SIZE];
	} rx[BOARD_ETH_RX_FRAMES];

	struct {
		unsigned int rx;
		unsigned int tx;
		unsigned int ru;
		unsigned int rerr;
		unsigned int tu;
		unsigned int terr;
		unsigned int fbi;
	} isr;

	unsigned char *regbase;
	unsigned int irq;
	void *(*get_tx_frame_buf)(struct bld_eth_dev_s *);
	int (*send_frame)(struct bld_eth_dev_s *, int);
	int (*is_link_up)(struct bld_eth_dev_s *);
	int (*reset)(struct bld_eth_dev_s *);

	int link_duplex;
	int link_speed;

	struct bld_net_if_s *nif;
} bld_eth_dev_t;

/**
 * Network interface.
 */
typedef struct bld_net_if_s
{
	bld_eth_dev_t dev;

	u8 hwaddr[6];	/**< HW address */
	bld_ip_t ip;	/**< IP address */
	bld_ip_t mask;	/**< Netmask */
	bld_ip_t gw;	/**< Gateway */

	u16 ip_ident;	/**< IP header indent counter */

	struct {
		bld_arp_tab_t gw;
		bld_arp_tab_t icmp;
		bld_arp_tab_t udp;
	} arptab;

	struct {
		bld_ip_t ip;
		u16 seqno;
		int poll_var;
	} icmp_cln;

	struct {
		bld_ip_t dst_ip;
		u16 src_port;

		u8 sys_buf[BOARD_ETH_FRAMES_SIZE];
		int sys_buf_len;
		u16 sys_src_port;
		int sys_buf_used;
		u8 *user_buf;
		int user_buf_len;
		u16 user_src_port;
		int user_buf_valid;
	} udp_cln;

	int (*handle_arp)(struct bld_net_if_s *, void *, int);
	int (*handle_ip)(struct bld_net_if_s *, void *, int);
} bld_net_if_t;

/*===========================================================================*/
extern u16 htons(u16);
extern u16 ntohs(u16);
extern u32 htonl(u32);
extern u32 ntohl(u32);

extern int bld_net_init(int verbose, void *pptb_table);
extern void bld_net_down(void);
extern int bld_net_wait_ready(unsigned int tmo);

extern int bld_ping(u32 dst_ip, unsigned int tmo);

extern int bld_udp_bind(bld_ip_t dst_ip, u16 src_port);
extern void bld_udp_close(void);
extern int bld_udp_send(u16 dst_port, void *packet, int packetlen);
extern int bld_udp_recv(u16 *src_port, void *packet, int packetlen, int tmo);

extern int bld_tftp_load(u32 address, const char *filename, int verbose);
extern int bld_netboot(const char *cmdline, int verbose);
extern int bld_netprogram(const char *pfile_name,
	u32 addr, u32 exec, int verbose);

/*===========================================================================*/
extern void eth_init(bld_net_if_t *, bld_eth_dev_t *, u8 *);
extern void eth_filter_source_address(struct bld_eth_dev_s *dev, u8 *hwaddr);
extern void eth_pass_source_address(struct bld_eth_dev_s *dev);
/*===========================================================================*/

#endif

