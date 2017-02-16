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

#ifndef _RTTM_H_
#define _RTTM_H_
#include "wlan_location_defs.h"
#include "rttapi.h"
#define MAX_NSPMRESP_CIR 10

struct rtt_priv_hdr {
	u8 done;
};

#define MAX_RTT_CIRRESP_RECORDS 100
#define RTTM_CIR_BUF_SIZE (MAX_RTT_CIRRESP_RECORDS *		\
			   (sizeof(struct rtt_priv_hdr) +	\
			   MRES_LEN + CIR_RES_LEN))

struct rttm_context {
	u32 ts1;
	u32 ts2;
	struct ath6kl *ar;
	struct nsp_header mresphdr ;
	struct nsp_rttd2h2_clkoffset rttd2h2_clk;
	u8  rttdhclkcal_active;
	u8  dhclkcal_index;
};

extern struct rttm_context *g_pRttmContext;
#define DEV_GETRTT_HDL() g_pRttmContext
#define DEV_SETRTT_HDL(rtt) (g_pRttmContext = rtt)
#endif
