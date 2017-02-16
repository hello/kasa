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

#ifndef HTC_OPS_H
#define HTC_OPS_H

#include "htc.h"
#include "debug.h"

static inline void *ath6kl_htc_create(struct ath6kl *ar)
{
	return ar->htc_ops->create(ar);
}

static inline int ath6kl_htc_wait_target(struct htc_target *target)
{
	return target->dev->ar->htc_ops->wait_target(target);
}

static inline int ath6kl_htc_start(struct htc_target *target)
{
	return target->dev->ar->htc_ops->start(target);
}

static inline int ath6kl_htc_conn_service(struct htc_target *target,
			    struct htc_service_connect_req *req,
			    struct htc_service_connect_resp *resp)
{
	return target->dev->ar->htc_ops->conn_service(target, req, resp);
}

static inline int ath6kl_htc_tx(struct htc_target *target,
	struct htc_packet *packet)
{
	return target->dev->ar->htc_ops->tx(target, packet);
}

static inline void ath6kl_htc_stop(struct htc_target *target)
{
	return target->dev->ar->htc_ops->stop(target);
}

static inline void ath6kl_htc_cleanup(struct htc_target *target)
{
	return target->dev->ar->htc_ops->cleanup(target);
}

static inline void ath6kl_htc_flush_txep(struct htc_target *target,
			   enum htc_endpoint_id endpoint, u16 tag)
{
	return target->dev->ar->htc_ops->flush_txep(target, endpoint, tag);
}

static inline void ath6kl_htc_flush_rx_buf(struct htc_target *target)
{
	return target->dev->ar->htc_ops->flush_rx_buf(target);
}

static inline void ath6kl_htc_indicate_activity_change(
	struct htc_target *target, enum htc_endpoint_id endpoint, bool active)
{
	return target->dev->ar->htc_ops->indicate_activity_change(target,
			endpoint, active);
}

static inline int ath6kl_htc_get_rxbuf_num(struct htc_target *target,
			     enum htc_endpoint_id endpoint)
{
	return target->dev->ar->htc_ops->get_rxbuf_num(target, endpoint);
}

static inline int ath6kl_htc_add_rxbuf_multiple(struct htc_target *target,
				  struct list_head *pktq)
{
	return target->dev->ar->htc_ops->add_rxbuf_multiple(target, pktq);
}

static inline int ath6kl_htc_credit_setup(struct htc_target *target,
			    struct ath6kl_htc_credit_info *cred_info)
{
	return target->dev->ar->htc_ops->credit_setup(target, cred_info);
}

static inline int ath6kl_htc_stat(struct htc_target *target,
						 u8 *buf, int buf_len)
{
	return target->dev->ar->htc_ops->get_stat(target, buf, buf_len);
}

static inline int ath6kl_htc_stop_netif_queue_full(struct htc_target *target)
{
	return target->dev->ar->htc_ops->stop_netif_queue_full(target);
}

static inline int ath6kl_htc_wmm_schedule_change(struct htc_target *target,
		bool change)
{
	return target->dev->ar->htc_ops->indicate_wmm_schedule_change(target,
			change);
}

static inline int ath6kl_htc_change_credit_bypass(struct htc_target *target,
		u8 traffic_class)
{
	return target->dev->ar->htc_ops->change_credit_bypass(target,
			traffic_class);
}

#endif
