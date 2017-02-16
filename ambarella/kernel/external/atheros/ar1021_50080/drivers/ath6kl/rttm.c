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

#include <linux/ip.h>
#include "core.h"
#include "debug.h"
#include "wlan_location_defs.h"
#include "rttm.h"
#include "ath_netlink.h"

struct rttm_context *g_pRttmContext;

void DumpRttResp(void *data)
{
	int i = 0, j = 0;
	struct nsp_mresphdr *presphdr = (struct nsp_mresphdr *)data;
	struct nsp_cir_resp *pcirresp;
	int explen;
	if (presphdr)
		ath6kl_dbg(ATH6KL_DBG_RTT, "NSP Response ReqId : %x "
				"RespType : %x NoResp : %x Result : %x\n",
				presphdr->request_id, presphdr->response_type,
				presphdr->no_of_responses, presphdr->result);
	else
		return;

	explen = (presphdr->no_of_responses * sizeof(struct nsp_cir_resp)) +
			sizeof(struct nsp_mresphdr);
	pcirresp =
	      (struct nsp_cir_resp *)((u8 *)data + sizeof(struct nsp_mresphdr));

	ath6kl_dbg(ATH6KL_DBG_RTT, "RTT Response  Size Expected : %d ", explen);

	pcirresp->no_of_chains = 2;
	for (i = 0; i < presphdr->no_of_responses; i++)	{
		ath6kl_dbg(ATH6KL_DBG_RTT, "TOD : %x\n", pcirresp->tod);
		ath6kl_dbg(ATH6KL_DBG_RTT, "TOA : %x\n", pcirresp->toa);
		ath6kl_dbg(ATH6KL_DBG_RTT, "TotalChains : %x\n",
				pcirresp->no_of_chains);
		ath6kl_dbg(ATH6KL_DBG_RTT, "RSSI0 : %x RSSI1 : %x ",
				pcirresp->rssi[0], pcirresp->rssi[1]);
		ath6kl_dbg(ATH6KL_DBG_RTT, "SendRate : %x\n",
				pcirresp->sendrate);
		ath6kl_dbg(ATH6KL_DBG_RTT, "RecvRate : %x\n",
				pcirresp->recvrate);
		ath6kl_dbg(ATH6KL_DBG_RTT, "ChannelDump :\n");
		for (j = 0; j < RTTM_CDUMP_SIZE(pcirresp->no_of_chains,
			pcirresp->isht40); j++) {
			u8 k = 0;
			k++;
			ath6kl_dbg(ATH6KL_DBG_RTT, "%x ",
					pcirresp->channel_dump[j]);
			if (k > 15)
				ath6kl_dbg(ATH6KL_DBG_RTT, ("\n"));
		}
		pcirresp++;
	}
}

int rttm_init(void *ar)
{
	struct rttm_context *prttm = NULL;
	ath6kl_dbg(ATH6KL_DBG_RTT, "rttm init ");
	prttm = kmalloc(sizeof(struct rttm_context), GFP_KERNEL);
	if (NULL == prttm)
		return -ENOMEM;
	memset(prttm, 0, sizeof(struct rttm_context));
	prttm->ar = ar;
	DEV_SETRTT_HDL(prttm);
	/* Initialize NL For RTT */
	if (0 != ath_netlink_init()) {
		ath6kl_err("RTT Init Failed to Initialize NetLink Interface\n");
		return -ENODEV;
	}
	return 0;
}

int rttm_recv(void *buf, u32 len)
{
#define RTTM_CONTEXT_PREFIX_OFFSET \
	(sizeof(u32) + sizeof(u32) + sizeof(struct ath6kl *))

	struct rttm_context *prttm = NULL;
	struct nsp_mresphdr *presphdr = (struct nsp_mresphdr *)buf;
	int resptype = presphdr->response_type;
	prttm = DEV_GETRTT_HDL();

	/* printk("RTT Recv Len : %d %d\n", len, resptype); */
	if ((resptype == MRESP_CLKOFFSETCAL_START) ||
		(resptype == MRESP_CLKOFFSETCAL_END)) {
		ath6kl_dbg(ATH6KL_DBG_RTT,
				"RTT ClkCal Request %d\n", resptype);
		if (resptype == MRESP_CLKOFFSETCAL_START) {
			prttm->rttdhclkcal_active = 1;
			prttm->dhclkcal_index = 0;
		} else if (resptype == MRESP_CLKOFFSETCAL_END) {
			prttm->rttdhclkcal_active = 0;
			prttm->dhclkcal_index = 0;
			/* Post Response of measurements to Device */
			prttm->mresphdr.frame_type = NSP_RTTCLKCAL_INFO;
			rttm_issue_request(
				(char *)prttm + RTTM_CONTEXT_PREFIX_OFFSET,
				NSP_HDR_LEN + sizeof(struct nsp_rtt_clkoffset));
		}

	} else {
		if (buf && len)	{
			/* Pass up Recv RTT Resp by NL */
			/* DumpRttResp(buf); */
			ath6kl_dbg(ATH6KL_DBG_RTT, "NLSend  Len : %d ", len);
			ath_netlink_send(buf, len);
		}
	}
	return 0;
}

void rttm_free()
{
	struct rttm_context *prttm = NULL;
	prttm = DEV_GETRTT_HDL();
	if (prttm != NULL)
		kfree(prttm);
	ath_netlink_free();
}


int rttm_issue_request(void *buf, u32 len)
{
	struct rttm_context *prttm = NULL;
	struct nsp_header hdr;
	u32 ftype;
	struct ath6kl *ar = NULL;
	enum wmi_cmd_id cmd_id = WMI_RTT_MEASREQ_CMDID;
	prttm = DEV_GETRTT_HDL();

	ar = prttm->ar;
	memcpy(&hdr, buf, NSP_HDR_LEN);
	ftype = hdr.frame_type;
	ath6kl_dbg(ATH6KL_DBG_RTT, "RTT Req Type : %d Len : %d ", ftype, len);
	if (ftype == NSP_MRQST) {
		struct nsp_mrqst *pstmrqst =
				(struct nsp_mrqst *)(buf + NSP_HDR_LEN);
		ath6kl_dbg(ATH6KL_DBG_RTT, "NSP Request ID:%d mode:%d  "
				"channel : %d NoMeas : %d Rate : %x\n ",
				pstmrqst->request_id, pstmrqst->mode,
				pstmrqst->channel, pstmrqst->no_of_measurements,
				pstmrqst->transmit_rate);
		if (pstmrqst->no_of_measurements > 10) {
			ath6kl_dbg(ATH6KL_DBG_RTT,
					"RTTREQ No Measurements >10 : %d ",
					pstmrqst->no_of_measurements);
			return -EINVAL;
		}
		cmd_id = WMI_RTT_MEASREQ_CMDID;
	} else if (ftype == NSP_RTTCONFIG) {
		struct nsp_rtt_config *pstrttcfg =
				(struct nsp_rtt_config *)(buf + NSP_HDR_LEN);
		ath6kl_dbg(ATH6KL_DBG_RTT, "NSP RTTCFG 2gCal%d 5gCal:%d "
				"FFTScale: %d RngScale:%d "
				"CLKSpeed2g : %d\n CLKSpeed5g : %d",
				pstrttcfg->ClkCal[0], pstrttcfg->ClkCal[1],
				pstrttcfg->FFTScale, pstrttcfg->RangeScale,
				pstrttcfg->ClkSpeed[0], pstrttcfg->ClkSpeed[1]);
		cmd_id = WMI_RTT_CONFIG_CMDID;
	} else if (ftype == NSP_RTTCLKCAL_INFO) {
		ath6kl_dbg(ATH6KL_DBG_RTT, "NSP CLK CALINFO CMD\n");
		cmd_id = WMI_RTT_CLKCALINFO_CMDID;
	}

	if (wmi_rtt_req(ar->wmi, cmd_id, buf + NSP_HDR_LEN,
			len - NSP_HDR_LEN)) {
		ath6kl_dbg(ATH6KL_DBG_RTT, "RTT Req Fail ");
		return -EIO;
	}
	return 0;
}

