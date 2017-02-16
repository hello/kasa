/**
 * eth/tftp.h
 *
 * History:
 *    2006/06/07 - [Charles Chiou] created file
 *
 * Copyright (C) 2004-2014, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#ifndef __NETDEV__TFTP_H__
#define __NETDEV__TFTP_H__

#include <config.h>
#include <basedef.h>

#define MAX_TFTP_RECV_TRIES	3

/*
 * Trivial File Transfer Protocol (IEN-133)
 */
#define TFTP_PORT		69
#define TFTP_SEGSIZE		512  /* data segment size */

/*
 * Packet types.
 */
#define TFTP_RRQ		01   /* read request */
#define TFTP_WRQ		02   /* write request */
#define TFTP_DATA		03   /* data packet */
#define TFTP_ACK		04   /* acknowledgement */
#define TFTP_ERROR		05   /* error code */
#define TFTP_OACK		06   /* option acknowledgement*/

/*
 * Error codes.
 */
#define TFTP_EUNDEF		0   /* not defined */
#define TFTP_ENOTFOUND		1   /* file not found */
#define TFTP_EACCESS		2   /* access violation */
#define TFTP_ENOSPACE		3   /* disk full or allocation exceeded */
#define TFTP_EBADOP		4   /* illegal TFTP operation */
#define TFTP_EBADID		5   /* unknown transfer ID */
#define TFTP_EEXISTS		6   /* file already exists */
#define TFTP_ENOUSER		7   /* no such user */

/**
 * TFTP UDP packet header.
 */
typedef struct tftp_header_s
{
	u16	opcode;			/**< Packet type */
	union {
		u16	block;		/**< Block # */
		u16	code;		/**< Error code */
		u8	stuff[1];	/**< Request packet stuff */
	} __attribute__((packed)) u;
	char	data[1];		/**< Data or error string */
} __attribute__((packed)) tftp_header_t;

#endif

