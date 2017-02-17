/**
 * eth/tftp.h
 *
 * History:
 *    2006/06/07 - [Charles Chiou] created file
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

