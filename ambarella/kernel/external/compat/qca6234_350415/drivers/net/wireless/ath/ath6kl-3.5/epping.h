/*
 * Copyright (c) 2004-2011 Atheros Communications Inc.
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

#ifndef EPPING_H
#define EPPING_H

#include "htc.h"

/* alignment to 4-bytes */
#define EPPING_ALIGNMENT_PAD  (((sizeof(struct htc_frame_hdr) + 3) & (~0x3)) \
				 - sizeof(struct htc_frame_hdr))

#define HCI_RSVD_EXPECTED_PKT_TYPE_RECV_OFFSET  7

struct epping_header {
	/* reserved for HCI packet header (GMBOX) testing */
	u8		hcirsvd[8];

	/* stream no. to echo this packet on (filled by host) */
	u8		stream_echo_h;

	/* stream no. packet was echoed to (filled by target)
	   When echoed: stream_echo_sent == stream_echo */
	u8		stream_echo_sent_t;

	/* stream no. that target received this packet on (filled by target) */
	u8		stream_recv_t;

	/* stream number to send on (filled by host) */
	u8		stream_no_h;

	/* magic number to filter for this packet on the host*/
	u8		magic_h[4];

	/* reserved fields that must be set to a "reserved" value
	   since this packet maps to a 14-byte ethernet frame we want
	   to make sure ethertype field is set to something unknown */
	u8		srsvd[6];

	/* padding for alignment */
	u8		pad[2];

	/* timestamp of packet (host or target) */
	u8		time_stamp[8];

	/* 4 byte host context, target echos this back */
	u32		host_context;

	/* sequence number (set by host or target) */
	u32		seq_no;

	/* ping command (filled by host) */
	u16		cmd_id_h;

	/* optional flags */
	u16		cmd_flags_h;

	/* buffer for command (host -> target) */
	u8		cmd_buffer_h[8];

	/* buffer for command (target -> host) */
	u8		cmd_buffer_t[8];

	/* length of data */
	u16		data_length;

	/* 16 bit CRC of data */
	u16		crc;

	/* header CRC (fields : StreamNo_h to end, minus HeaderCRC) */
	u16		header_crc;
} __packed;

#define EPPING_PING_MAGIC_0		0xAA
#define EPPING_PING_MAGIC_1		0x55
#define EPPING_PING_MAGIC_2		0xCE
#define EPPING_PING_MAGIC_3		0xEC

#define IS_EPPING_PACKET(pPkt)				\
	(((pPkt)->magic_h[0] == EPPING_PING_MAGIC_0) && \
	((pPkt)->magic_h[1] == EPPING_PING_MAGIC_1) && \
	((pPkt)->magic_h[2] == EPPING_PING_MAGIC_2) && \
	((pPkt)->magic_h[3] == EPPING_PING_MAGIC_3))

/* DataCRC field is valid */
#define CMD_FLAGS_DATA_CRC            (1 << 0)

/* delay the echo of the packet */
#define CMD_FLAGS_DELAY_ECHO          (1 << 1)

/* do not drop at HTC layer no matter what the stream is */
#define CMD_FLAGS_NO_DROP             (1 << 2)

#define IS_EPING_PACKET_NO_DROP(pPkt)  ((pPkt)->cmd_flags_h & CMD_FLAGS_NO_DROP)

/* this number is higher than the define WMM AC classes so we
   can use this to distinguish packets */
#define HCI_TRANSPORT_STREAM_NUM  16

#endif /*EPPING_H*/
