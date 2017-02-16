/*
 * Copyright (c) 2007-2011 Atheros Communications Inc.
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

#include "core.h"
#include "debug.h"
#include "hif-ops.h"

#include <asm/byteorder.h>

#define HTC_PACKET_CONTAINER_ALLOCATION 32
#define HTC_CONTROL_BUFFER_SIZE (HTC_MAX_CTRL_MSG_LEN + HTC_HDR_LENGTH)
#define DATA_EP_SIZE 4

static int ath6kl_htc_pipe_tx(struct htc_target *handle,
	struct htc_packet *packet);
static void ath6kl_htc_pipe_cleanup(struct htc_target *handle);
static void htc_tx_bundle_timer_handler(unsigned long ptr);

/* htc pipe tx path */
static inline void restore_tx_packet(struct htc_packet *packet)
{
	if (packet->info.tx.flags & HTC_FLAGS_TX_FIXUP_NETBUF) {
		skb_pull(packet->skb, sizeof(struct htc_frame_hdr));
		packet->info.tx.flags &= ~HTC_FLAGS_TX_FIXUP_NETBUF;
	}
}

static void do_send_completion(struct htc_endpoint *ep,
			       struct list_head *queue_to_indicate)
{
	if (list_empty(queue_to_indicate)) {
		/* nothing to indicate */
		return;
	}

	if (ep->ep_cb.tx_comp_multi != NULL) {
		ath6kl_dbg(ATH6KL_DBG_HTC,
			   "%s: calling ep %d, send complete multiple callback"
			   "(%d pkts)\n", __func__,
			   ep->eid, get_queue_depth(queue_to_indicate));
		/*
		 * a multiple send complete handler is being used,
		 * pass the queue to the handler
		 */
		ep->ep_cb.tx_comp_multi(
				(struct htc_target *) ep->target,
				queue_to_indicate);
		/*
		 * all packets are now owned by the callback,
		 * reset queue to be safe
		 */
		INIT_LIST_HEAD(queue_to_indicate);
	} else {
		struct htc_packet *packet;
		/* using legacy EpTxComplete */
		do {
			packet = list_first_entry(queue_to_indicate,
					struct htc_packet, list);

			list_del(&packet->list);
			ath6kl_dbg(ATH6KL_DBG_HTC,
				   "%s: calling ep %d send complete callback on "
				   "packet 0x%lX\n", __func__,
				   ep->eid, (unsigned long)(packet));
			ep->ep_cb.tx_complete(
				(struct htc_target *) ep->target,
				packet);
		} while (!list_empty(queue_to_indicate));
	}

}

static void send_packet_completion(struct htc_target *target,
				   struct htc_packet *packet)
{
	struct htc_endpoint *ep = &target->endpoint[packet->endpoint];
	struct list_head container;

	restore_tx_packet(packet);
	INIT_LIST_HEAD(&container);
	list_add_tail(&packet->list, &container);
	/* do completion */
	do_send_completion(ep, &container);
}

static void get_htc_packet_credit_based(struct htc_target *target,
					struct htc_endpoint *ep,
					struct list_head *queue)
{
	int credits_required;
	int remainder;
	u8 send_flags;
	struct htc_packet *packet;
	unsigned int transfer_len;
	int msgs_upper_limit =
		(htc_bundle_send) ? HTC_HOST_MAX_MSG_PER_BUNDLE : 1;

	if (target->tgt_cred_sz == 0)
		return;

	/* NOTE : the TX lock is held when this function is called */

	/* loop until we can grab as many packets out of the queue as we can */
	while (true) {
		send_flags = 0;
		if (list_empty(&ep->txq))
			break;

		/* get packet at head, but don't remove it */
		packet = list_first_entry(&ep->txq,
					struct htc_packet, list);
		if (packet == NULL)
			break;

		ath6kl_dbg(ATH6KL_DBG_HTC,
			   "%s: got head packet:0x%lX , queue depth: %d\n",
			   __func__, (unsigned long)packet,
			   get_queue_depth(&ep->txq));

		transfer_len = packet->act_len + HTC_HDR_LENGTH;
		if (transfer_len <= target->tgt_cred_sz) {
			credits_required = 1;
		} else {
			/* figure out how many credits this message requires */
			credits_required =
			    transfer_len / target->tgt_cred_sz;
			remainder = transfer_len % target->tgt_cred_sz;

			if (remainder)
				credits_required++;
		}

		ath6kl_dbg(ATH6KL_DBG_HTC, "%s: creds required:%d"
				"got:%d\n", __func__, credits_required,
				ep->cred_dist.credits);

		if (ep->eid == ENDPOINT_0) {
			/*
			 * endpoint 0 is special, it always has a credit and
			 * does not require credit based flow control
			 */
			credits_required = 0;

		} else if (ep->eid >= ENDPOINT_2 && ep->eid <= ENDPOINT_5) {

			if (target->avail_tx_credits < credits_required)
				break;

			if (htc_bundle_send && !msgs_upper_limit)
				break;

			target->avail_tx_credits -= credits_required;
			ep->ep_st.cred_cosumd += credits_required;

			if (target->avail_tx_credits < 1) {
				/* tell the target we need credits ASAP! */
				send_flags |= HTC_FLAGS_NEED_CREDIT_UPDATE;
				ep->ep_st.cred_low_indicate += 1;
				ath6kl_dbg(ATH6KL_DBG_HTC,
					"%s: host needs credits\n", __func__);
			}

		} else {

			if (ep->cred_dist.credits < credits_required)
				break;

			if (htc_bundle_send && !msgs_upper_limit)
				break;

			ep->cred_dist.credits -= credits_required;
			ep->ep_st.cred_cosumd += credits_required;

			/* check if we need credits back from the target */
			if (ep->cred_dist.credits <
					ep->cred_dist.cred_per_msg) {
				/* tell the target we need credits ASAP! */
				send_flags |= HTC_FLAGS_NEED_CREDIT_UPDATE;
				ep->ep_st.cred_low_indicate += 1;
				ath6kl_dbg(ATH6KL_DBG_HTC,
					"%s: host needs credits\n", __func__);
			}
		}

		/* now we can fully dequeue */
		packet = list_first_entry(&ep->txq,
				struct htc_packet, list);

		list_del(&packet->list);
		/* save the number of credits this packet consumed */
		packet->info.tx.cred_used = credits_required;
		/* save send flags */
		packet->info.tx.flags = send_flags;
		packet->info.tx.seqno = ep->seqno;
		ep->seqno++;
		/* queue this packet into the caller's queue */
		list_add_tail(&packet->list, queue);

		if (htc_bundle_send)
			msgs_upper_limit--;
	}

}

static void get_htc_packet(struct htc_target *target,
			   struct htc_endpoint *ep,
			   struct list_head *queue, int resources)
{

	struct htc_packet *packet;

	int msgs_upper_limit =
		(htc_bundle_send) ? HTC_HOST_MAX_MSG_PER_BUNDLE : resources;

	/* NOTE : the TX lock is held when this function is called */
	if (htc_bundle_send && !resources)
		return;

	/* loop until we can grab as many packets out of the queue as we can */
	while (msgs_upper_limit) {
		if (list_empty(&ep->txq))
			break;

		packet = list_first_entry(&ep->txq,
					struct htc_packet, list);
		list_del(&packet->list);

		ath6kl_dbg(ATH6KL_DBG_HTC,
			   "%s: got packet:0x%lX , new queue depth: %d\n",
			   __func__, (unsigned long)packet,
			   get_queue_depth(&ep->txq));
		packet->info.tx.seqno = ep->seqno;
		packet->info.tx.flags = 0;
		packet->info.tx.cred_used = 0;
		ep->seqno++;
		/* queue this packet into the caller's queue */
		list_add_tail(&packet->list, queue);
		msgs_upper_limit--;
	}

}

static int htc_issue_packets(struct htc_target *target,
			     struct htc_endpoint *ep,
			     struct list_head *pkt_queue)
{
	int status = 0;
	u16 payload_len;
	struct sk_buff *nbuf;
	struct htc_frame_hdr *htc_hdr;
	struct htc_packet *packet = NULL;

	ath6kl_dbg(ATH6KL_DBG_HTC,
		"%s: queue: 0x%lX, pkts %d\n", __func__,
		(unsigned long)pkt_queue, get_queue_depth(pkt_queue));
	if (htc_bundle_send && (ep->pipeid_ul != 0 /* HIF_TX_CTRL_PIPE */)) {
		/* only for HIF data pipes */
		struct sk_buff *msg_bundle[HTC_HOST_MAX_MSG_PER_BUNDLE] = {};
		int msgs_to_bundle = 0;
		int bundle_credit_used = 0;

		while (!list_empty(pkt_queue)) {
			packet = list_first_entry(pkt_queue,
					struct htc_packet, list);
			list_del(&packet->list);

			nbuf = packet->skb;
			if (!nbuf) {
				WARN_ON(1);
				status = -EINVAL;
				break;
			}

			payload_len = packet->act_len;
			/* setup HTC frame header */
			htc_hdr = (struct htc_frame_hdr *)skb_push(nbuf,
					sizeof(struct htc_frame_hdr));
			if (!htc_hdr) {
				WARN_ON(1);
				status = -EINVAL;
				break;
			}
			packet->info.tx.flags |= HTC_FLAGS_TX_FIXUP_NETBUF;
			packet->info.tx.flags |= HTC_FLAGS_SEND_BUNDLE;

			/* Endianess? */
			put_unaligned((u16) payload_len, &htc_hdr->payld_len);
			htc_hdr->payld_len = cpu_to_le16(htc_hdr->payld_len);
			htc_hdr->flags = packet->info.tx.flags;
			htc_hdr->eid = (u8) packet->endpoint;
			htc_hdr->ctrl[0] = 0;
			htc_hdr->ctrl[1] = (u8) packet->info.tx.seqno;

			spin_lock_bh(&target->tx_lock);
			/* store in look up queue to match completions */
			list_add_tail(&packet->list, &ep->tx_lookup_queue);
			ep->ep_st.tx_issued += 1;
			spin_unlock_bh(&target->tx_lock);

			/* pkt_queue is less than
			 * HTC_HOST_MAX_MSG_PER_BUNDLE
			 */
			msg_bundle[msgs_to_bundle] = nbuf;
			msgs_to_bundle++;
			bundle_credit_used += packet->info.tx.cred_used;
		}

		status = ath6kl_hif_pipe_send_bundle(target->dev->ar,
				ep->pipeid_ul, msg_bundle, msgs_to_bundle);

		if (status != 0) {
			spin_lock_bh(&target->tx_lock);
			/* reclaim credits */
			if (ep->eid >= ENDPOINT_2 && ep->eid <= ENDPOINT_5)
				target->avail_tx_credits += bundle_credit_used;
			else
				ep->cred_dist.credits += bundle_credit_used;
			spin_unlock_bh(&target->tx_lock);
		}
	} else {

		while (!list_empty(pkt_queue)) {
			packet = list_first_entry(pkt_queue,
					struct htc_packet, list);
			list_del(&packet->list);

			nbuf = packet->skb;
			if (!nbuf) {
				WARN_ON(1);
				status = -EINVAL;
				break;
			}

			payload_len = packet->act_len;
			/* setup HTC frame header */
			htc_hdr = (struct htc_frame_hdr *)skb_push(nbuf,
						sizeof(struct htc_frame_hdr));
			if (!htc_hdr) {
				WARN_ON(1);
				status = -EINVAL;
				break;
			}

			packet->info.tx.flags |= HTC_FLAGS_TX_FIXUP_NETBUF;

			/* Endianess? */
			put_unaligned((u16) payload_len, &htc_hdr->payld_len);
			htc_hdr->payld_len = cpu_to_le16(htc_hdr->payld_len);
			htc_hdr->flags = packet->info.tx.flags;
			htc_hdr->eid = (u8) packet->endpoint;
			htc_hdr->ctrl[0] = 0;
			htc_hdr->ctrl[1] = (u8) packet->info.tx.seqno;

			spin_lock_bh(&target->tx_lock);
			/* store in look up queue to match completions */
			list_add_tail(&packet->list, &ep->tx_lookup_queue);
			ep->ep_st.tx_issued += 1;
			spin_unlock_bh(&target->tx_lock);

			status = ath6kl_hif_pipe_send(target->dev->ar,
						ep->pipeid_ul, NULL, nbuf);

			if (status != 0) {
				if (status != -ENOMEM) {
					/* TODO: if more than 1 endpoint maps
					* to the same PipeID, it is possible
					* to run out of resources in the HIF
					* layer. Don't emit the error
					*/
					ath6kl_dbg(ATH6KL_DBG_HTC,
						"%s: failed status:%d\n",
						__func__, status);
				}
				spin_lock_bh(&target->tx_lock);
				list_del(&packet->list);
				/* reclaim credits */
				if (ep->eid >= ENDPOINT_2 &&
				    ep->eid <= ENDPOINT_5)
					target->avail_tx_credits +=
						packet->info.tx.cred_used;
				else
					ep->cred_dist.credits +=
						packet->info.tx.cred_used;
				spin_unlock_bh(&target->tx_lock);
				/* put it back into the callers queue */
				list_add(&packet->list, pkt_queue);
				break;
			}
		}
	}

	if (status != 0) {
		while (!list_empty(pkt_queue)) {
			if (status != -ENOMEM) {
				ath6kl_dbg(ATH6KL_DBG_HTC,
					   "%s: failed pkt:0x%p status:%d\n",
					   __func__, packet, status);
			}
			packet = list_first_entry(pkt_queue,
					struct htc_packet, list);
			list_del(&packet->list);
			packet->status = status;
			send_packet_completion(target, packet);
		}
	}

	return status;
}

#define HTC_BUNDLE_SEND_TH  6000

static enum htc_send_queue_result htc_try_send(struct htc_target *target,
					  struct htc_endpoint *ep,
					  struct list_head *callers_send_queue)
{
	struct list_head send_queue;	/* temp queue to hold packets */
	struct htc_packet *packet, *tmp_pkt;
	struct ath6kl *ar = target->dev->ar;
	int tx_resources;
	int overflow, txqueue_depth;

	ath6kl_dbg(ATH6KL_DBG_HTC,
		   "%s: (queue:0x%lX depth:%d)\n",
		   __func__, (unsigned long)callers_send_queue,
		   (callers_send_queue ==
		    NULL) ? 0 : get_queue_depth(callers_send_queue));

	if (htc_bundle_send && htc_bundle_send_timer &&
	    ep->eid == ENDPOINT_2) {
		/* init bundle send timer */
		if (ep->timer_init == 0) {
			setup_timer(&ep->timer, htc_tx_bundle_timer_handler,
			   (unsigned long) target);
			mod_timer(&ep->timer,
			   jiffies + msecs_to_jiffies(htc_bundle_send_timer));
			ep->timer_init = 1;
		}
		/* check if we need to queue packet for bundle send */
		if (ep->call_by_timer == 0 && ep->pass_th) {
			spin_lock_bh(&target->tx_lock);
			if (get_queue_depth(&ep->txq) <
			    HTC_HOST_MAX_MSG_PER_BUNDLE) {
				spin_unlock_bh(&target->tx_lock);
				return 0;
			}
			spin_unlock_bh(&target->tx_lock);
		}
	}

	/* init the local send queue */
	INIT_LIST_HEAD(&send_queue);
	/*
	 * callers_send_queue equals to NULL means
	 * caller didn't provide a queue, just wants us to
	 * check queues and send
	 */
	if (callers_send_queue != NULL) {
		if (list_empty(callers_send_queue)) {
			/* empty queue */
			return HTC_SEND_QUEUE_DROP;
		}

		spin_lock_bh(&target->tx_lock);
		txqueue_depth = get_queue_depth(&ep->txq);
		spin_unlock_bh(&target->tx_lock);

		if (txqueue_depth >= ep->max_txq_depth) {
			/* we've already overflowed */
			overflow = get_queue_depth(callers_send_queue);
		} else {
			/* get how much we will overflow by */
			overflow = txqueue_depth;
			overflow += get_queue_depth(callers_send_queue);
			/* get how much we will overflow the TX queue by */
			overflow -= ep->max_txq_depth;
		}

		/* if overflow is negative or zero, we are okay */
		if (overflow > 0) {
			ath6kl_dbg(ATH6KL_DBG_HTC,
				   "%s: Endpoint %d, TX queue will overflow :%d, "
				   "Tx Depth:%d, Max:%d\n", __func__,
				   ep->eid, overflow, txqueue_depth,
				   ep->max_txq_depth);
		}
		if ((overflow <= 0)
		    || (ep->ep_cb.tx_full == NULL)) {
			/*
			 * all packets will fit or caller did not provide send
			 * full indication handler -- just move all of them
			 * to the local send_queue object
			 */
			list_splice_tail_init(callers_send_queue,
							&send_queue);
		} else {
			int i;
			int good_pkts =
				get_queue_depth(callers_send_queue) - overflow;
			if (good_pkts < 0) {
				WARN_ON(1);
				return HTC_SEND_QUEUE_DROP;
			}

			/* we have overflowed, and a callback is provided */
			/* dequeue all non-overflow packets to the sendqueue */
			for (i = 0; i < good_pkts; i++) {
				/* pop off caller's queue */
				packet = list_first_entry(
						callers_send_queue,
						struct htc_packet, list);
				list_del(&packet->list);
				/* insert into local queue */
				list_add_tail(&packet->list, &send_queue);
			}

			/*
			 * the caller's queue has all the packets that won't fit
			 * walk through the caller's queue and indicate each to
			 * the send full handler
			 */
			list_for_each_entry_safe(packet, tmp_pkt,
						 callers_send_queue, list) {

				ath6kl_dbg(ATH6KL_DBG_HTC,
					   "%s: Indicat overflowed TX pkts: %lX\n",
					   __func__, (unsigned long)packet);
				if (ep->ep_cb.
						tx_full((struct htc_target *)
						ep->target,
						packet) == HTC_SEND_FULL_DROP) {
					/* callback wants the packet dropped */
					ep->ep_st.tx_dropped += 1;

					/* leave this one in the caller's queue
					 * for cleanup */
				} else {
					/* callback wants to keep this packet,
					 * remove from caller's queue */
					list_del(&packet->list);
					/* put it in the send queue */
					list_add_tail(&packet->list,
						&send_queue);
				}

			}

			if (list_empty(&send_queue)) {
				/* no packets made it in, caller will cleanup */
				return HTC_SEND_QUEUE_DROP;
			}
		}
	}

	if (!ep->tx_credit_flow_enabled) {
		tx_resources =
		    ath6kl_hif_pipe_get_free_queue_number(ar, ep->pipeid_ul);
	} else {
		tx_resources = 0;
	}

	spin_lock_bh(&target->tx_lock);
	if (!list_empty(&send_queue)) {
		/* transfer packets to tail */
		list_splice_tail_init(&send_queue, &ep->txq);
		if (!list_empty(&send_queue)) {
			WARN_ON(1);
			spin_unlock_bh(&target->tx_lock);
			return HTC_SEND_QUEUE_DROP;
		}
		INIT_LIST_HEAD(&send_queue);
	}

	/* increment tx processing count on entry */
	ep->tx_proc_cnt++;

	if (ep->tx_proc_cnt > 1) {
		/*
		 * another thread or task is draining the TX queues on this
		 * endpoint that thread will reset the tx processing count when
		 * the queue is drained
		 */
		ep->tx_proc_cnt--;
		spin_unlock_bh(&target->tx_lock);
		return HTC_SEND_QUEUE_OK;
	}

	/***** beyond this point only 1 thread may enter ******/

	/*
	 * now drain the endpoint TX queue for transmission as long as we have
	 * enough transmit resources
	 */
	while (true) {

		if (get_queue_depth(&ep->txq) == 0)
			break;

		if (ep->tx_credit_flow_enabled) {
			/* credit based mechanism provides flow control based on
			 * target transmit resource availability, we assume that
			 * the HIF layer will always have bus resources greater
			 * than target transmit resources
			 */
			get_htc_packet_credit_based(target, ep, &send_queue);
		} else {
			/* get all packets for this endpoint that we can for
			 * this pass
			 */
			get_htc_packet(target, ep, &send_queue, tx_resources);
		}

		if (get_queue_depth(&send_queue) == 0) {
			/* didn't get packets due to out of resources or
			 * TX queue was drained
			 */
			break;
		}

		spin_unlock_bh(&target->tx_lock);

		/* send what we can */
		htc_issue_packets(target, ep, &send_queue);

		if (!ep->tx_credit_flow_enabled) {
			tx_resources =
			    ath6kl_hif_pipe_get_free_queue_number(ar,
						      ep->pipeid_ul);
		}

		spin_lock_bh(&target->tx_lock);

	}
	/* done with this endpoint, we can clear the count */
	ep->tx_proc_cnt = 0;
	spin_unlock_bh(&target->tx_lock);

	return HTC_SEND_QUEUE_OK;
}

static void htc_tx_bundle_timer_handler(unsigned long ptr)
{
	struct htc_target *target = (struct htc_target *)ptr;
	struct htc_endpoint *endpoint = &target->endpoint[ENDPOINT_2];
	static u32 count;
	static u32 tx_issued;
	count = 0;
	tx_issued = 0;

	endpoint->call_by_timer = 1;
	count++;

	if ((count%(1000/htc_bundle_send_timer)) == 0) {
		/* printk("timer tx_issued=%d\n",
		* endpoint->ep_st.tx_issued-tx_issued);
		*/
		if ((endpoint->ep_st.tx_issued - tx_issued)
		     > HTC_BUNDLE_SEND_TH)
			endpoint->pass_th = 1;
		else
			endpoint->pass_th = 0;
		tx_issued = endpoint->ep_st.tx_issued;
	}
	htc_try_send(target, endpoint, NULL);
	endpoint->call_by_timer = 0;

	mod_timer(&endpoint->timer,
		jiffies + msecs_to_jiffies(htc_bundle_send_timer));
}

static void htc_tx_resource_available(struct htc_target *context, u8 pipeid)
{
	int i;
	struct htc_target *target = (struct htc_target *) context;
	struct htc_endpoint *ep = NULL;

	for (i = 0; i < ENDPOINT_MAX; i++) {
		ep = &target->endpoint[i];
		if (ep->svc_id != 0) {
			if (ep->pipeid_ul == pipeid)
				break;
		}
	}

	if (i >= ENDPOINT_MAX) {
		ath6kl_dbg(ATH6KL_DBG_HTC,
			   "Invalid pipe indicated for TX resource avail : %d!\n",
			   pipeid);
		return;
	}

	ath6kl_dbg(ATH6KL_DBG_HTC,
		   "%s: hIF indicated more resources for pipe:%d\n",
		   __func__, pipeid);

	htc_try_send(target, ep, NULL);
}

/* htc control packet manipulation */
static void destroy_htc_txctrl_packet(struct htc_packet *packet)
{
	struct sk_buff *netbuf;
	netbuf = (struct sk_buff *) packet->skb;
	if (netbuf != NULL)
		dev_kfree_skb(netbuf);

	kfree(packet);
}

static struct htc_packet *build_htc_txctrl_packet(void)
{
	struct htc_packet *packet = NULL;
	struct sk_buff *netbuf;

	packet = kzalloc(sizeof(struct htc_packet), GFP_KERNEL);
	if (packet == NULL)
		return NULL;

	memset(packet, 0, sizeof(struct htc_packet));
	netbuf = __dev_alloc_skb(HTC_CONTROL_BUFFER_SIZE, GFP_KERNEL);

	if (netbuf == NULL) {
		kfree(packet);
		return NULL;
	}
	packet->skb = netbuf;

	return packet;
}

static void htc_free_txctrl_packet(struct htc_target *target,
				   struct htc_packet *packet)
{
	destroy_htc_txctrl_packet(packet);
}

static struct htc_packet *htc_alloc_txctrl_packet(struct htc_target
						*target)
{
	return build_htc_txctrl_packet();
}

static void htc_txctrl_complete(struct htc_target *context,
						struct htc_packet *packet)
{

	struct htc_target *target =
		(struct htc_target *) context;
	htc_free_txctrl_packet(target, packet);
}

#define MAX_MESSAGE_SIZE 1536

static int htc_setup_target_buffer_assignments(struct htc_target *target)
{
	struct htc_pipe_txcredit_alloc *entry;
	int status;
	int credits;
	int credit_per_maxmsg;
	unsigned int hif_usbaudioclass = 0;

	if (target->tgt_cred_sz == 0)
		return -ECOMM;

	credit_per_maxmsg = MAX_MESSAGE_SIZE / target->tgt_cred_sz;
	if (MAX_MESSAGE_SIZE % target->tgt_cred_sz)
		credit_per_maxmsg++;

	/* TODO, this should be configured by the caller! */

	credits = target->tgt_creds;
	entry = &target->txcredit_alloc[0];

	status = -ENOMEM;

	if (hif_usbaudioclass) {
		ath6kl_dbg(ATH6KL_DBG_HTC,
			   "htc_setup_target_buffer_assignments "
			   "For USB Audio Class- Total:%d\n", credits);
		entry++;
		entry++;
		/* Setup VO Service To have Max Credits */
		entry->service_id = WMI_DATA_VO_SVC;
		entry->credit_alloc = (credits - 6);
		if (entry->credit_alloc == 0)
			entry->credit_alloc++;

		credits -= (int)entry->credit_alloc;
		if (credits <= 0)
			return status;

		entry++;
		entry->service_id = WMI_CONTROL_SVC;
		entry->credit_alloc = credit_per_maxmsg;
		credits -= (int)entry->credit_alloc;
		if (credits <= 0)
			return status;

		/* leftovers go to best effort */
		entry++;
		entry++;
		entry->service_id = WMI_DATA_BE_SVC;
		entry->credit_alloc = (u8) credits;
		status = 0;
	} else {
		entry++;
		entry->service_id = WMI_DATA_VI_SVC;
		entry->credit_alloc = credits / 4;
		if (entry->credit_alloc == 0)
			entry->credit_alloc++;

		credits -= (int)entry->credit_alloc;
		if (credits <= 0)
			return status;

		entry++;
		entry->service_id = WMI_DATA_VO_SVC;
		entry->credit_alloc = credits / 4;
		if (entry->credit_alloc == 0)
			entry->credit_alloc++;

		credits -= (int)entry->credit_alloc;
		if (credits <= 0)
			return status;

		entry++;
		entry->service_id = WMI_CONTROL_SVC;
		entry->credit_alloc = credit_per_maxmsg;
		credits -= (int)entry->credit_alloc;
		if (credits <= 0)
			return status;

		entry++;
		entry->service_id = WMI_DATA_BK_SVC;
		entry->credit_alloc = credit_per_maxmsg;
		credits -= (int)entry->credit_alloc;
		if (credits <= 0)
			return status;

		/* leftovers go to best effort */
		entry++;
		entry->service_id = WMI_DATA_BE_SVC;
		entry->credit_alloc = (u8) credits;
		status = 0;
	}

	if (status == 0) {
		if (AR_DBG_LVL_CHECK(ATH6KL_DBG_HTC)) {
			int i;
			for (i = 0; i < ENDPOINT_MAX; i++) {
				if (target->txcredit_alloc[i].service_id != 0) {
					ath6kl_dbg(ATH6KL_DBG_HTC,
						   "HTC Service Index : %d TX : 0x%2.2X :"
						   "alloc:%d\n", i,
						   target->txcredit_alloc[i].
						   service_id,
						   target->txcredit_alloc[i].
						   credit_alloc);
				}
			}
		}
	}
	return status;
}

/* process credit reports and call distribution function */
static void htc_process_credit_report(struct htc_target *target,
				      struct htc_credit_report *rpt,
				      int num_entries,
				      enum htc_endpoint_id from_ep)
{
	int i;
	struct htc_endpoint *ep;
	int total_credits = 0;

	/* lock out TX while we update credits */
	spin_lock_bh(&target->tx_lock);

	for (i = 0; i < num_entries; i++, rpt++) {
		if (rpt->eid >= ENDPOINT_MAX) {
			WARN_ON(1);
			spin_unlock_bh(&target->tx_lock);
			return;
		}

		ep = &target->endpoint[rpt->eid];

		if (ep->eid >= ENDPOINT_2 && ep->eid <= ENDPOINT_5) {
			enum htc_endpoint_id eid[DATA_EP_SIZE] = {
			     ENDPOINT_5, ENDPOINT_4, ENDPOINT_2, ENDPOINT_3};
			int epid_idx;

			target->avail_tx_credits += rpt->credits;

			for (epid_idx = 0; epid_idx < DATA_EP_SIZE;
			     epid_idx++) {
				ep = &target->endpoint[eid[epid_idx]];
				if (get_queue_depth(&ep->txq))
					break;
			}
			spin_unlock_bh(&target->tx_lock);
			htc_try_send(target, ep, NULL);
			spin_lock_bh(&target->tx_lock);
		} else {
			ep->cred_dist.credits += rpt->credits;

			if (ep->cred_dist.credits &&
			    get_queue_depth(&ep->txq)) {
				spin_unlock_bh(&target->tx_lock);
				htc_try_send(target, ep, NULL);
				spin_lock_bh(&target->tx_lock);
			}
		}

		total_credits += rpt->credits;
	}
	ath6kl_dbg(ATH6KL_DBG_HTC,
		   "  Report indicated %d credits to distribute\n",
		   total_credits);

	spin_unlock_bh(&target->tx_lock);
}

/* flush endpoint TX queue */
static void htc_flush_tx_endpoint(struct htc_target *target,
				  struct htc_endpoint *ep, u16 Tag)
{
	struct htc_packet *packet;

	spin_lock_bh(&target->tx_lock);
	while (get_queue_depth(&ep->txq)) {
		packet = list_first_entry(&ep->txq,
					struct htc_packet, list);
		list_del(&packet->list);
		packet->status = 0;
		send_packet_completion(target, packet);
	}
	spin_unlock_bh(&target->tx_lock);
}

/*
 * In the adapted HIF layer, struct sk_buff * are passed between HIF and HTC,
 * since upper layers expects struct htc_packet containers we use the completed
 * netbuf and lookup it's corresponding HTC packet buffer from a lookup list.
 * This is extra overhead that can be fixed by re-aligning HIF interfaces with
 * HTC.
 */
static struct htc_packet *htc_lookup_tx_packet(struct htc_target *target,
					       struct htc_endpoint *ep,
					       struct sk_buff *netbuf)
{
	struct htc_packet *packet, *tmp_pkt;
	struct htc_packet *found_packet = NULL;

	spin_lock_bh(&target->tx_lock);

	/*
	 * interate from the front of tx lookup queue
	 * this lookup should be fast since lower layers completes in-order and
	 * so the completed packet should be at the head of the list generally
	 */
	list_for_each_entry_safe(packet, tmp_pkt, &ep->tx_lookup_queue, list) {
		/* check for removal */
		if (netbuf == packet->skb) {
			/* found it */
			list_del(&packet->list);
			found_packet = packet;
			break;
		}

	}

	spin_unlock_bh(&target->tx_lock);

	return found_packet;
}

static int htc_tx_completion(struct htc_target *context, struct sk_buff *netbuf)
{
	struct htc_target *target = (struct htc_target *)context;
	u8 *netdata;
	u32 netlen;
	struct htc_frame_hdr *htc_hdr;
	u8 EpID;
	struct htc_endpoint *ep;
	struct htc_packet *packet;
	struct ath6kl *ar;
	enum htc_endpoint_id eid[DATA_EP_SIZE] = {
		ENDPOINT_5, ENDPOINT_4, ENDPOINT_2, ENDPOINT_3};
	int epid_idx;
	u16 resources_thresh[DATA_EP_SIZE]; /* urb resources */
	u16 resources;
	u16 resources_max;

	netdata = netbuf->data;
	netlen = netbuf->len;

	htc_hdr = (struct htc_frame_hdr *)netdata;

	EpID = htc_hdr->eid;
	ep = &target->endpoint[EpID];

	packet = htc_lookup_tx_packet(target, ep, netbuf);
	if (packet == NULL) {
		/* may have already been flushed and freed */
		ath6kl_err("HTC TX lookup failed!\n");
	} else {
		/* will be giving this buffer back to upper layers */
		packet->status = 0;
		send_packet_completion(target, packet);
	}
	netbuf = NULL;
	ar = target->dev->ar;

	if (!ep->tx_credit_flow_enabled) {
		if ((ar->hw.flags & ATH6KL_HW_SINGLE_PIPE_SCHED) &&
			(enum htc_endpoint_id)EpID >= ENDPOINT_2 &&
			(enum htc_endpoint_id)EpID <= ENDPOINT_5) {

			resources_max =	ath6kl_hif_pipe_get_max_queue_number(
							ar, ep->pipeid_ul);

			resources_thresh[0] = 0;                      /* VO */
			resources_thresh[1] =
					(resources_max *  2) / 32;    /* VI */
			resources_thresh[2] =
					(resources_max *  3) / 32;    /* BE */
			resources_thresh[3] =
					(resources_max *  4) / 32;    /* BK */

			resources = ath6kl_hif_pipe_get_free_queue_number(
							ar, ep->pipeid_ul);
			spin_lock_bh(&target->tx_lock);
			for (epid_idx = 0; epid_idx < DATA_EP_SIZE;
			     epid_idx++) {
				ep = &target->endpoint[eid[epid_idx]];

				if (!get_queue_depth(&ep->txq))
					continue;

				if (resources >= resources_thresh[epid_idx])
					break;
			}
			spin_unlock_bh(&target->tx_lock);

			if (epid_idx == DATA_EP_SIZE)
				/*
				#if 0
				if (resources) {
					for (epid_idx = 0;
					     epid_idx < DATA_EP_SIZE;
					     epid_idx++) {
					      ep =
					      &target->endpoint[eid[epid_idx]];
					    if (get_queue_depth(&ep->tx_queue))
						break;
					}
				} else
				#endif
				*/
				return 0;
		}

		/*
		 * note: when using TX credit flow, the re-checking of queues
		 * happens when credits flow back from the target. in the
		 * non-TX credit case, we recheck after the packet completes
		 */
		htc_try_send(target, ep, NULL);
	}

	return 0;
}

static int htc_send_pkts_sched_check(struct htc_target *target,
				     enum htc_endpoint_id id)
{
	struct htc_endpoint *endpoint;
	enum htc_endpoint_id eid;
	struct list_head *tx_queue;
	u16 ac_queue_status[DATA_EP_SIZE] = {0, 0, 0, 0};
	u16 status;

	if (id < ENDPOINT_2 || id > ENDPOINT_5)
		return 1;

	spin_lock_bh(&target->tx_lock);
	for (eid = ENDPOINT_2; eid <= ENDPOINT_5; eid++) {
		endpoint = &target->endpoint[eid];
		tx_queue = &endpoint->txq;

		if (list_empty(tx_queue))
			ac_queue_status[eid - 2] = 1;

		if (htc_bundle_send && htc_bundle_send_timer &&
		    eid == ENDPOINT_2) {
			/* init bundle send timer */
			if (endpoint->timer_init == 0) {
				setup_timer(&endpoint->timer,
					    htc_tx_bundle_timer_handler,
					    (unsigned long) target);
				mod_timer(&endpoint->timer, jiffies +
				    msecs_to_jiffies(htc_bundle_send_timer));
				endpoint->timer_init = 1;
			}
			/* check if we need to queue packet for bundle send */
			if (endpoint->pass_th &&
			    get_queue_depth(&endpoint->txq) <
			    HTC_HOST_MAX_MSG_PER_BUNDLE) {
				spin_unlock_bh(&target->tx_lock);
				return 0;
			}
		}
	}
	spin_unlock_bh(&target->tx_lock);

	switch (id) {
	case ENDPOINT_2: /* BE */
		status = ac_queue_status[0] && ac_queue_status[2] &&
			 ac_queue_status[3];
		break;
	case ENDPOINT_3: /* BK */
		status = ac_queue_status[0] && ac_queue_status[1] &&
			 ac_queue_status[2] && ac_queue_status[3];
		break;
	case ENDPOINT_4: /* VI */
		status = ac_queue_status[2] && ac_queue_status[3];
		break;
	case ENDPOINT_5: /* VO */
		status = ac_queue_status[3];
		break;
	default:
		status = 0;
		break;
	}
	return status;
}

static int htc_send_pkts_sched_queue(struct htc_target *target,
				     struct list_head *pkt_queue,
				     enum htc_endpoint_id eid)
{
	struct htc_endpoint *endpoint;
	struct list_head *tx_queue;
	struct htc_packet *packet, *tmp_pkt;
	int good_pkts;

	endpoint = &target->endpoint[eid];
	tx_queue = &endpoint->txq;

	spin_lock_bh(&target->tx_lock);

	good_pkts = endpoint->max_txq_depth - get_queue_depth(tx_queue);

	if (good_pkts > 0) {
		while (!list_empty(pkt_queue)) {
			packet = list_first_entry(pkt_queue,
						  struct htc_packet, list);
			list_del(&packet->list);
			list_add_tail(&packet->list, tx_queue);

			good_pkts--;
			if (good_pkts <= 0)
				break;
		}
	}

	if (get_queue_depth(pkt_queue)) {
		list_for_each_entry_safe(packet, tmp_pkt, pkt_queue, list) {
			if (endpoint->ep_cb.tx_full((struct htc_target *)
					endpoint->target,
					packet) == HTC_SEND_FULL_DROP) {
				endpoint->ep_st.tx_dropped += 1;
			} else {
				list_del(&packet->list);
				list_add_tail(&packet->list, tx_queue);
			}
		}
	}

	spin_unlock_bh(&target->tx_lock);

	return 0;
}


static int htc_send_packets_multiple(struct htc_target *handle,
				struct list_head *pkt_queue)
{
	struct htc_target *target = (struct htc_target *) handle;
	struct htc_endpoint *ep;
	struct htc_packet *packet, *tmp_pkt;
	struct ath6kl *ar = target->dev->ar;

	if (list_empty(pkt_queue))
		return -EINVAL;

	/* get first packet to find out which ep the packets will go into */
	packet = list_first_entry(pkt_queue, struct htc_packet, list);
	if (packet == NULL)
		return -EINVAL;

	if (packet->endpoint >= ENDPOINT_MAX) {
		WARN_ON(1);
		return -EINVAL;
	}
	ep = &target->endpoint[packet->endpoint];

	if (ar->hw.flags & ATH6KL_HW_SINGLE_PIPE_SCHED) {
		if (!htc_send_pkts_sched_check(target, ep->eid))
			htc_send_pkts_sched_queue(target, pkt_queue, ep->eid);
		else
			htc_try_send(target, ep, pkt_queue);
	} else {
		htc_try_send(target, ep, pkt_queue);
	}

	/* do completion on any packets that couldn't get in */
	if (!list_empty(pkt_queue)) {

		list_for_each_entry_safe(packet, tmp_pkt, pkt_queue, list) {
			packet->status = -ENOMEM;
		}

		do_send_completion(ep, pkt_queue);
	}

	return 0;
}

/* htc pipe rx path */
static struct htc_packet *alloc_htc_packet_container(struct htc_target
						     *target)
{
	struct htc_packet *packet;
	spin_lock_bh(&target->rx_lock);

	if (target->htc_packet_pool == NULL) {
		spin_unlock_bh(&target->rx_lock);
		return NULL;
	}

	packet = target->htc_packet_pool;
	target->htc_packet_pool = (struct htc_packet *)packet->list.next;

	spin_unlock_bh(&target->rx_lock);

	packet->list.next = NULL;
	return packet;
}

static void free_htc_packet_container(struct htc_target *target,
				      struct htc_packet *packet)
{
	spin_lock_bh(&target->rx_lock);

	if (target->htc_packet_pool == NULL) {
		target->htc_packet_pool = packet;
		packet->list.next = NULL;
	} else {
		packet->list.next = (struct list_head *)target->htc_packet_pool;
		target->htc_packet_pool = packet;
	}

	spin_unlock_bh(&target->rx_lock);
}

static int htc_process_trailer(struct htc_target *target,
			       u8 *buffer,
			       int len, enum htc_endpoint_id from_ep)
{
	struct htc_record_hdr *record;
	u8 *record_buf;
	u8 *orig_buf;
	int orig_len;
	int status;

	orig_buf = buffer;
	orig_len = len;
	status = 0;

	while (len > 0) {

		if (len < sizeof(struct htc_record_hdr)) {
			status = -EINVAL;
			break;
		}
		/* these are byte aligned structs */
		record = (struct htc_record_hdr *)buffer;
		len -= sizeof(struct htc_record_hdr);
		buffer += sizeof(struct htc_record_hdr);

		if (record->len > len) {
			/* no room left in buffer for record */
			ath6kl_dbg(ATH6KL_DBG_HTC,
				   " invalid length: %d (id:%d) buffer has: %d bytes left\n",
				   record->len, record->rec_id, len);
			status = -EINVAL;
			break;
		}
		/* start of record follows the header */
		record_buf = buffer;

		switch (record->rec_id) {
		case HTC_RECORD_CREDITS:
			if (record->len <
				sizeof(struct htc_credit_report)) {
				WARN_ON(1);
				return -EINVAL;
			}

			htc_process_credit_report(target,
						  (struct htc_credit_report *)
						  record_buf,
						  record->len /
						  (sizeof
						   (struct htc_credit_report)),
						  from_ep);
			break;
		default:
			ath6kl_dbg(ATH6KL_DBG_HTC,
				   " unhandled record: id:%d length:%d\n",
				   record->rec_id, record->len);
			break;
		}

		if (status != 0)
			break;

		/* advance buffer past this record for next time around */
		buffer += record->len;
		len -= record->len;
	}

	return status;
}

static void do_recv_completion(struct htc_endpoint *ep,
			       struct list_head *queue_to_indicate)
{
	struct htc_packet *packet;
	if (list_empty(queue_to_indicate)) {
		/* nothing to indicate */
		return;
	}

	/* using legacy EpRecv */
	while (!list_empty(queue_to_indicate)) {
		packet = list_first_entry(queue_to_indicate,
					struct htc_packet, list);
		list_del(&packet->list);
		ep->ep_cb.rx((struct htc_target *)
				    ep->target, packet);
	}

	return;
}

static void recv_packet_completion(struct htc_target *target,
				   struct htc_endpoint *ep,
				   struct htc_packet *packet)
{
	struct list_head container;
	INIT_LIST_HEAD(&container);
	list_add_tail(&packet->list, &container);
	/* do completion */
	do_recv_completion(ep, &container);
}

struct sk_buff *rx_sg_to_single_netbuf(struct htc_target *target)
{
	struct sk_buff *skb;
	u8 *anbdata;
	u8 *anbdata_new;
	u32 anblen;
	struct sk_buff *new_skb = NULL;
	struct sk_buff_head *rx_sg_queue = &target->rx_sg_q;

	if (skb_queue_empty(rx_sg_queue)) {
		ath6kl_dbg(ATH6KL_DBG_HTC,
			"%s: invalid sg queue len\n", __func__);
		goto _failed;
	}

	new_skb = __dev_alloc_skb(target->rx_sg_total_len_exp, GFP_ATOMIC);
	if (new_skb == NULL) {
		ath6kl_dbg(ATH6KL_DBG_HTC,
			"%s: can't allocate %u size netbuf\n",
			__func__, target->rx_sg_total_len_exp);
		goto _failed;
	}

	anbdata_new = new_skb->data;
	anblen = new_skb->len;

	while ((skb = skb_dequeue(rx_sg_queue))) {
		anbdata = skb->data;
		memcpy(anbdata_new, anbdata, skb->len);
		skb_put(new_skb, skb->len);
		anbdata_new += skb->len;
		dev_kfree_skb(skb);
	};

	target->rx_sg_total_len_exp = 0;
	target->rx_sg_total_len_cur = 0;
	target->rx_sg_in_progress = false;

	return new_skb;

_failed:

	while ((skb = skb_dequeue(rx_sg_queue)))
		dev_kfree_skb(skb);
	target->rx_sg_total_len_exp = 0;
	target->rx_sg_total_len_cur = 0;
	target->rx_sg_in_progress = false;
	return NULL;
}

static int htc_rx_completion(struct htc_target *context,
				struct sk_buff *netbuf, u8 pipeid)
{
	int status = 0;
	struct htc_frame_hdr *htc_hdr;
	struct htc_target *target = (struct htc_target *)context;
	u8 *netdata;
	u8 hdr_info;
	u32 netlen;
	struct htc_endpoint *ep;
	struct htc_packet *packet;
	u16 payload_len;
	u32 trailerlen = 0;

	int i;

	static u32 assert_pattern = cpu_to_be32(0x0000c600);

	spin_lock_bh(&target->rx_lock);
	if (target->rx_sg_in_progress) {
		target->rx_sg_total_len_cur += netbuf->len;
		skb_queue_tail(&target->rx_sg_q, netbuf);
		if (target->rx_sg_total_len_cur ==
		    target->rx_sg_total_len_exp) {
			netbuf = rx_sg_to_single_netbuf(target);
			if (netbuf == NULL) {
				spin_unlock_bh(&target->rx_lock);
				goto free_netbuf;
			}
		} else {
			netbuf = NULL;
			spin_unlock_bh(&target->rx_lock);
			goto free_netbuf;
		}
	}

	spin_unlock_bh(&target->rx_lock);

	netdata = netbuf->data;
	netlen = netbuf->len;

#define CONFIG_CRASH_DUMP 1
#if CONFIG_CRASH_DUMP
	if (!memcmp(netdata, &assert_pattern, sizeof(assert_pattern))) {

#define REG_DUMP_COUNT_AR6004   60
		netdata += 4;

		ath6kl_info("Firmware crash detected...\n");
		for (i = 0; i < REG_DUMP_COUNT_AR6004 * 4; i += 16) {
			ath6kl_info("%d: 0x%08x 0x%08x 0x%08x 0x%08x\n", i/4,
				be32_to_cpu(*(u32 *)(netdata+i)),
				be32_to_cpu(*(u32 *)(netdata+i + 4)),
				be32_to_cpu(*(u32 *)(netdata+i + 8)),
				be32_to_cpu(*(u32 *)(netdata+i + 12)));
		}
		dev_kfree_skb(netbuf);
		netbuf = NULL;

		ath6kl_fw_crash_trap(target->dev->ar);

		goto free_netbuf;
	}
#endif

	htc_hdr = (struct htc_frame_hdr *)netdata;

	ep = &target->endpoint[htc_hdr->eid];

	if (htc_hdr->eid >= ENDPOINT_MAX) {
		ath6kl_dbg(ATH6KL_DBG_HTC,
			   "HTC Rx: invalid EndpointID=%d\n",
			   htc_hdr->eid);
		status = -EINVAL;
		goto free_netbuf;
	}

	payload_len = le16_to_cpu(get_unaligned(&htc_hdr->payld_len));

	if (netlen < (payload_len + HTC_HDR_LENGTH)) {
		spin_lock_bh(&target->rx_lock);
		target->rx_sg_in_progress = true;
		skb_queue_tail(&target->rx_sg_q, netbuf);
		target->rx_sg_total_len_exp = (payload_len + HTC_HDR_LENGTH);
		target->rx_sg_total_len_cur += netlen;
		spin_unlock_bh(&target->rx_lock);
		netbuf = NULL;
		goto free_netbuf;
	}

	/* get flags to check for trailer */
	hdr_info = htc_hdr->flags;
	if (hdr_info & HTC_FLG_RX_TRAILER) {
		/* extract the trailer length */
		hdr_info = htc_hdr->ctrl[0];
		if ((hdr_info < sizeof(struct htc_record_hdr))
		    || (hdr_info > payload_len)) {
			ath6kl_dbg(ATH6KL_DBG_HTC,
				   "invalid header: payloadlen"
				   "should be %d, CB[0]: %d\n",
				   payload_len, hdr_info);
			status = -EINVAL;
			goto free_netbuf;
		}

		trailerlen = hdr_info;
		/* process trailer after hdr/apps payload */
		status = htc_process_trailer(target,
					     ((u8 *) htc_hdr +
					      HTC_HDR_LENGTH +
					      payload_len -
					      hdr_info), hdr_info,
					     htc_hdr->eid);
		if (status != 0)
			goto free_netbuf;
	}

	if (((int)payload_len - (int)trailerlen) <= 0) {
		/* zero length packet with trailer, just drop these */
		goto free_netbuf;
	}

	if (htc_hdr->eid == ENDPOINT_0) {
		/* handle HTC control message */
		if (target->htc_flags & HTC_OP_STATE_SETUP_COMPLETE) {
			/*
			 * fatal: target should not send unsolicited
			 * messageson the endpoint 0
			 */
			ath6kl_dbg(ATH6KL_DBG_HTC,
				"HTC ignores Rx Ctrl after setup complete\n");
			status = -EINVAL;
			goto free_netbuf;
		}

		/* remove HTC header */
		skb_pull(netbuf, HTC_HDR_LENGTH);

		netdata = netbuf->data;
		netlen = netbuf->len;

		spin_lock_bh(&target->rx_lock);

		target->ctrl_response_valid = true;
		target->ctrl_response_len =
			min_t(int, netlen, HTC_MAX_CTRL_MSG_LEN);
		memcpy(target->ctrl_response_buf, netdata,
		       target->ctrl_response_len);

		spin_unlock_bh(&target->rx_lock);

		dev_kfree_skb(netbuf);
		netbuf = NULL;
		goto free_netbuf;
	}

	/*
	 * TODO: the message based HIF architecture allocates net bufs
	 * for recv packets since it bridges that HIF to upper layers,
	 * which expects HTC packets, we form the packets here
	 */
	packet = alloc_htc_packet_container(target);
	if (packet == NULL) {
		status = -ENOMEM;
		goto free_netbuf;
	}
	packet->status = 0;
	packet->endpoint = htc_hdr->eid;
	packet->pkt_cntxt = netbuf;
	/* TODO: for backwards compatibility */
	packet->buf = skb_push(netbuf, 0) + HTC_HDR_LENGTH;
	packet->act_len = netlen - HTC_HDR_LENGTH - trailerlen;

	/*
	 * TODO: this is a hack because the driver layer will set the
	 * actual len of the skb again which will just double the len
	 */
	skb_trim(netbuf, 0);

	recv_packet_completion(target, ep, packet);
	/* recover the packet container */
	free_htc_packet_container(target, packet);
	netbuf = NULL;

free_netbuf:
	if (netbuf != NULL)
		dev_kfree_skb(netbuf);

	return status;

}

static void htc_flush_rx_queue(struct htc_target *target,
			struct htc_endpoint *ep)
{
	struct htc_packet *packet;
	struct list_head container;

	spin_lock_bh(&target->rx_lock);

	while (1) {
		if (list_empty(&ep->rx_bufq))
			break;
		packet = list_first_entry(&ep->rx_bufq,
					struct htc_packet, list);
		list_del(&packet->list);

		spin_unlock_bh(&target->rx_lock);
		packet->status = -ECANCELED;
		packet->act_len = 0;
		ath6kl_dbg(ATH6KL_DBG_HTC,
			   "  Flushing RX packet:0x%lX, length:%d, ep:%d\n",
			   (unsigned long)packet, packet->buf_len,
			   packet->endpoint);
		INIT_LIST_HEAD(&container);
		list_add_tail(&packet->list, &container);
		/* give the packet back */
		do_recv_completion(ep, &container);
		spin_lock_bh(&target->rx_lock);
	}

	spin_unlock_bh(&target->rx_lock);
}

/* polling routine to wait for a control packet to be received */
static int htc_wait_recv_ctrl_message(struct htc_target *target)
{
	int count = HTC_TARGET_RESPONSE_POLL_COUNT;

	while (count > 0) {
		spin_lock_bh(&target->rx_lock);

		if (target->ctrl_response_valid) {
			target->ctrl_response_valid = false;
			spin_unlock_bh(&target->rx_lock);
			break;
		}

		spin_unlock_bh(&target->rx_lock);

		count--;

		set_current_state(TASK_INTERRUPTIBLE);
		schedule_timeout(msecs_to_jiffies(HTC_TARGET_RESPONSE_POLL_MS));
		set_current_state(TASK_RUNNING);
	}
	if (count <= 0) {
		ath6kl_dbg(ATH6KL_DBG_HTC,
			   "%s: Timeout!\n", __func__);
		return -ECOMM;
	}

	return 0;
}

static void htc_rxctrl_complete(struct htc_target *context,
				struct htc_packet *packet)
{
	struct sk_buff *skb = packet->skb;

	if (packet->endpoint == ENDPOINT_0) {
		if (packet->status == -ECANCELED) {
			if (skb)
				dev_kfree_skb(skb);
			return;
		}
	}

	ath6kl_dbg(ATH6KL_DBG_HTC, "%s: invalid call function, ep = %d\n",
			__func__, packet->endpoint);

	WARN_ON(1);

	return;
}

/* htc pipe initialization */
static void reset_endpoint_states(struct htc_target *target)
{
	struct htc_endpoint *ep;
	int i;

	for (i = ENDPOINT_0; i < ENDPOINT_MAX; i++) {
		ep = &target->endpoint[i];
		ep->svc_id = 0;
		ep->len_max = 0;
		ep->max_txq_depth = 0;
		ep->eid = i;
		INIT_LIST_HEAD(&ep->txq);
		INIT_LIST_HEAD(&ep->tx_lookup_queue);
		INIT_LIST_HEAD(&ep->rx_bufq);
		ep->target = target;
		ep->tx_credit_flow_enabled = (bool) 1;
	}
}

/* start HTC, this is called after all services are connected */
static int htc_config_target_hif_pipe(struct htc_target *target)
{
	return 0;
}

/* htc service functions */
static u8 htc_get_credit_alloc(struct htc_target *target, u16 service_id)
{
	u8 allocation = 0;
	int i;

	for (i = 0; i < ENDPOINT_MAX; i++) {
		if (target->txcredit_alloc[i].service_id == service_id)
			allocation = target->txcredit_alloc[i].credit_alloc;
	}

	if (allocation == 0) {
		ath6kl_dbg(ATH6KL_DBG_HTC,
			   "HTC Service TX : 0x%2.2X : allocation is zero!\n",
			   service_id);
	}

	return allocation;
}

static int ath6kl_htc_pipe_conn_service(struct htc_target *handle,
		     struct htc_service_connect_req *conn_req,
		     struct htc_service_connect_resp *conn_resp)
{
	struct ath6kl *ar = handle->dev->ar;
	struct htc_target *target = (struct htc_target *) handle;
	int status = 0;
	struct htc_packet *packet = NULL;
	struct htc_conn_service_resp *resp_msg;
	struct htc_conn_service_msg *conn_msg;
	enum htc_endpoint_id assigned_epid = ENDPOINT_MAX;
	struct htc_endpoint *ep;
	unsigned int max_msg_size = 0;
	struct sk_buff *netbuf;
	u8 tx_alloc;
	int length;
	bool disable_credit_flowctrl = false;

	if (conn_req->svc_id == 0) {
		WARN_ON(1);
		status = -EINVAL;
		goto free_packet;
	}

	if (conn_req->svc_id == HTC_CTRL_RSVD_SVC) {
		/* special case for pseudo control service */
		assigned_epid = ENDPOINT_0;
		max_msg_size = HTC_MAX_CTRL_MSG_LEN;
		tx_alloc = 0;

	} else {

		tx_alloc =
		    htc_get_credit_alloc(target, conn_req->svc_id);
		if (tx_alloc == 0) {
			status = -ENOMEM;
			goto free_packet;
		}

		/* allocate a packet to send to the target */
		packet = htc_alloc_txctrl_packet(target);

		if (packet == NULL) {
			WARN_ON(1);
			status = -ENOMEM;
			goto free_packet;
		}

		netbuf = packet->skb;
		length = sizeof(struct htc_conn_service_msg);

		/* assemble connect service message */
		conn_msg =
		    (struct htc_conn_service_msg *)skb_put(netbuf,
							   length);
		if (conn_msg == NULL) {
			WARN_ON(1);
			status = -EINVAL;
			goto free_packet;
		}

		memset(conn_msg, 0,
		       sizeof(struct htc_conn_service_msg));
		conn_msg->msg_id = cpu_to_le16(HTC_MSG_CONN_SVC_ID);
		conn_msg->svc_id = cpu_to_le16(conn_req->svc_id);
		conn_msg->conn_flags = cpu_to_le16(conn_req->conn_flags &
					~HTC_CONN_FLGS_SET_RECV_ALLOC_MASK);

		/* tell target desired recv alloc for this ep */
		conn_msg->conn_flags |= cpu_to_le16(
			tx_alloc << HTC_CONN_FLGS_SET_RECV_ALLOC_SHIFT);

		if (conn_req->conn_flags &
		    HTC_CONN_FLGS_DISABLE_CRED_FLOW_CTRL) {
			disable_credit_flowctrl = true;
		}

		set_htc_pkt_info(packet, NULL, (u8 *) conn_msg,
				 length,
				 ENDPOINT_0, HTC_SERVICE_TX_PACKET_TAG);

		status = ath6kl_htc_pipe_tx(handle, packet);
		/* we don't own it anymore */
		packet = NULL;
		if (status != 0)
			goto free_packet;

		/* wait for response */
		status = htc_wait_recv_ctrl_message(target);
		if (status != 0)
			goto free_packet;

		/* we controlled the buffer creation so it has to be
		 * properly aligned
		 */
		resp_msg = (struct htc_conn_service_resp *)
		    target->ctrl_response_buf;

		if ((resp_msg->msg_id != cpu_to_le16(HTC_MSG_CONN_SVC_RESP_ID))
		    || (target->ctrl_response_len <
			sizeof(struct htc_conn_service_resp))) {
			/* this message is not valid */
			WARN_ON(1);
			status = -EINVAL;
			goto free_packet;
		}

		ath6kl_dbg(ATH6KL_DBG_TRC,
			   "%s: service 0x%X conn resp: "
			   "status: %d ep: %d\n", __func__,
			   le16_to_cpu(resp_msg->svc_id), resp_msg->status,
			   resp_msg->eid);

		conn_resp->resp_code = resp_msg->status;
		/* check response status */
		if (resp_msg->status != HTC_SERVICE_SUCCESS) {
			ath6kl_dbg(ATH6KL_DBG_HTC,
				   " Target failed service 0x%X connect"
				   " request (status:%d)\n",
				   le16_to_cpu(resp_msg->svc_id),
				   resp_msg->status);
			status = -EINVAL;
			goto free_packet;
		}

		assigned_epid = (enum htc_endpoint_id)resp_msg->eid;
		max_msg_size = le16_to_cpu(resp_msg->max_msg_sz);
	}

	/* the rest are parameter checks so set the error status */
	status = -EINVAL;

	if (assigned_epid >= ENDPOINT_MAX) {
		WARN_ON(1);
		goto free_packet;
	}

	if (max_msg_size == 0) {
		WARN_ON(1);
		goto free_packet;
	}

	ep = &target->endpoint[assigned_epid];
	ep->eid = assigned_epid;
	if (ep->svc_id != 0) {
		/* endpoint already in use! */
		WARN_ON(1);
		goto free_packet;
	}

	if (target->tgt_cred_sz == 0)
		goto free_packet;

	/* return assigned endpoint to caller */
	conn_resp->endpoint = assigned_epid;
	conn_resp->len_max = max_msg_size;

	/* setup the endpoint */
	ep->svc_id = conn_req->svc_id; /* this marks ep in use */
	ep->max_txq_depth = conn_req->max_txq_depth;
	ep->len_max = max_msg_size;
	ep->cred_dist.credits = tx_alloc;
	ep->cred_dist.cred_sz = target->tgt_cred_sz;
	ep->cred_dist.cred_per_msg =
	    max_msg_size / target->tgt_cred_sz;
	if (max_msg_size % target->tgt_cred_sz)
		ep->cred_dist.cred_per_msg++;

	/* copy all the callbacks */
	ep->ep_cb = conn_req->ep_cb;

	/* initialize tx_drop_packet_threshold */
	ep->tx_drop_packet_threshold = MAX_HI_COOKIE_NUM;

	status = ath6kl_hif_pipe_map_service(ar, ep->svc_id,
			&ep->pipeid_ul, &ep->pipeid_dl);
	if (status != 0)
		goto free_packet;

	ath6kl_dbg(ATH6KL_DBG_HTC,
		   "SVC Ready: 0x%4.4X: ULpipe:%d DLpipe:%d id:%d\n",
		   ep->svc_id, ep->pipeid_ul,
		   ep->pipeid_dl, ep->eid);

	if (disable_credit_flowctrl && ep->tx_credit_flow_enabled) {
		ep->tx_credit_flow_enabled = false;
		ath6kl_dbg(ATH6KL_DBG_HTC,
			   "SVC: 0x%4.4X ep:%d TX flow control off\n",
			   ep->svc_id, assigned_epid);
	}

free_packet:
	if (packet != NULL)
		htc_free_txctrl_packet(target, packet);
	return status;
}

/* htc export functions */
void *ath6kl_htc_pipe_create(struct ath6kl *ar)
{
	int status = 0;
	struct ath6kl_hif_pipe_callbacks htc_callbacks;
	struct htc_endpoint *ep = NULL;
	struct htc_target *target = NULL;
	struct htc_packet *packet;
	int i;

	target = kzalloc(sizeof(struct htc_target), GFP_KERNEL);
	if (target == NULL) {
		ath6kl_err("htc create unable to allocate memory\n");
		status = -ENOMEM;
		goto fail_htc_create;
	}

	memset(target, 0, sizeof(struct htc_target));

	spin_lock_init(&target->htc_lock);
	spin_lock_init(&target->rx_lock);
	spin_lock_init(&target->tx_lock);

	skb_queue_head_init(&target->rx_sg_q);

	reset_endpoint_states(target);

	for (i = 0; i < HTC_PACKET_CONTAINER_ALLOCATION; i++) {
		packet = (struct htc_packet *)
		    kzalloc(sizeof(struct htc_packet), GFP_KERNEL);
		if (packet != NULL) {
			memset(packet, 0, sizeof(struct htc_packet));
			free_htc_packet_container(target, packet);
		}
	}
	/* setup HIF layer callbacks */
	memset(&htc_callbacks, 0, sizeof(struct ath6kl_hif_pipe_callbacks));
	htc_callbacks.rx_completion = htc_rx_completion;
	htc_callbacks.tx_completion = htc_tx_completion;
	htc_callbacks.tx_resource_available = htc_tx_resource_available;

	target->dev =
	    kzalloc(sizeof(*target->dev), GFP_KERNEL);
	if (!target->dev) {
		ath6kl_err("unable to allocate memory\n");
		status = -ENOMEM;
		goto fail_htc_create;
	}
	target->dev->ar = ar;
	target->dev->htc_cnxt = (struct htc_target *)target;

	/* Get HIF default pipe for HTC message exchange */
	ep = &target->endpoint[ENDPOINT_0];

	ath6kl_hif_pipe_register_callback(ar, target, &htc_callbacks);
	ath6kl_hif_pipe_get_default(ar, &ep->pipeid_ul, &ep->pipeid_dl);

	return (void *)target;

fail_htc_create:
	if (status != 0) {
		if (target != NULL)
			ath6kl_htc_pipe_cleanup((struct htc_target *)target);
		target = NULL;
	}
	return (void *)target;
}

/* cleanup the HTC instance */
static void ath6kl_htc_pipe_cleanup(struct htc_target *handle)
{
	struct htc_packet *packet;
	struct htc_target *target = (struct htc_target *) handle;

	while (true) {
		packet = alloc_htc_packet_container(target);
		if (packet == NULL)
			break;
		kfree(packet);
	}
	kfree(target->dev);
	/* kfree our instance */
	kfree(target);
}

static int ath6kl_htc_pipe_start(struct htc_target *handle)
{
	struct sk_buff *netbuf;
	struct htc_target *target = (struct htc_target *) handle;
	struct htc_setup_comp_ext_msg *setup_comp;
	struct htc_packet *packet;

	htc_config_target_hif_pipe(target);
	/* allocate a buffer to send */
	packet = htc_alloc_txctrl_packet(target);
	if (packet == NULL) {
		WARN_ON(1);
		return -ENOMEM;
	}

	netbuf = packet->skb;
	/* assemble setup complete message */
	setup_comp =
	    (struct htc_setup_comp_ext_msg *)skb_put(netbuf,
				sizeof(struct htc_setup_comp_ext_msg));
	memset(setup_comp, 0, sizeof(struct htc_setup_comp_ext_msg));
	setup_comp->msg_id = cpu_to_le16(HTC_MSG_SETUP_COMPLETE_EX_ID);

	if (0) {
		ath6kl_dbg(ATH6KL_DBG_HTC,
			   "HTC will not use TX credit flow control\n");
		setup_comp->flags |=
			HTC_SETUP_COMP_FLG_DISABLE_TX_CREDIT_FLOW;
	} else {
		ath6kl_dbg(ATH6KL_DBG_HTC,
			   "HTC using TX credit flow control\n");
	}

	if (htc_bundle_recv) {
		ath6kl_dbg(ATH6KL_DBG_HTC, "HTC will use bundle recv\n");
		setup_comp->flags |= HTC_SETUP_COMP_FLG_RX_BNDL_EN;
	} else {
		ath6kl_dbg(ATH6KL_DBG_HTC, "HTC will not use bundle recv\n");
	}

	setup_comp->flags =  cpu_to_le32(setup_comp->flags);

	set_htc_pkt_info(packet, NULL, (u8 *) setup_comp,
			 sizeof(struct htc_setup_comp_ext_msg),
			 ENDPOINT_0, HTC_SERVICE_TX_PACKET_TAG);

	target->htc_flags |= HTC_OP_STATE_SETUP_COMPLETE;
	return ath6kl_htc_pipe_tx(handle, packet);
}

static void ath6kl_htc_pipe_stop(struct htc_target *handle)
{
	struct htc_target *target = (struct htc_target *) handle;
	int i;
	struct htc_endpoint *ep;
	struct sk_buff *skb;
	struct sk_buff_head *rx_sg_queue = &target->rx_sg_q;

	/* cleanup endpoints */
	for (i = 0; i < ENDPOINT_MAX; i++) {
		ep = &target->endpoint[i];
		htc_flush_rx_queue(target, ep);
		htc_flush_tx_endpoint(target, ep, HTC_TX_PACKET_TAG_ALL);
		if (i == ENDPOINT_2 && ep->timer_init) {
			del_timer(&ep->timer);
			ep->call_by_timer = 0;
			ep->timer_init = 0;
		}
	}

	spin_lock_bh(&target->rx_lock);

	while ((skb = skb_dequeue(rx_sg_queue)))
		dev_kfree_skb(skb);
	target->rx_sg_total_len_exp = 0;
	target->rx_sg_total_len_cur = 0;
	target->rx_sg_in_progress = false;

	spin_unlock_bh(&target->rx_lock);

	reset_endpoint_states(target);
	target->htc_flags &= ~HTC_OP_STATE_SETUP_COMPLETE;
}

static int ath6kl_htc_pipe_get_rxbuf_num(struct htc_target *htc_context,
		enum htc_endpoint_id endpoint)
{
	struct htc_target *target =
	    (struct htc_target *) htc_context;
	int num;

	spin_lock_bh(&target->rx_lock);
	num = get_queue_depth(&(target->endpoint[endpoint].rx_bufq));
	spin_unlock_bh(&target->rx_lock);

	return num;
}

static int ath6kl_htc_pipe_tx(struct htc_target *handle,
		struct htc_packet *packet)
{
	struct list_head queue;
	ath6kl_dbg(ATH6KL_DBG_HTC,
		   "%s: endPointId: %d, buffer: 0x%lX, length: %d\n",
		   __func__, packet->endpoint,
		   (unsigned long)packet->buf,
		   packet->act_len);
	INIT_LIST_HEAD(&queue);
	list_add_tail(&packet->list, &queue);
	return htc_send_packets_multiple(handle, &queue);
}

static int ath6kl_htc_pipe_wait_target(struct htc_target *handle)
{
	int status = 0;
	struct htc_target *target = (struct htc_target *) handle;
	struct htc_ready_ext_msg *ready_msg;
	struct htc_service_connect_req connect;
	struct htc_service_connect_resp resp;

	status = htc_wait_recv_ctrl_message(target);

	if (status != 0)
		return status;

	if (target->ctrl_response_len <
	    (sizeof(struct htc_ready_ext_msg))) {
		ath6kl_dbg(ATH6KL_DBG_HTC,
			   "invalid htc ready msg len:%d!\n",
			   target->ctrl_response_len);
		return -ECOMM;
	}
	ready_msg =
	    (struct htc_ready_ext_msg *)target->ctrl_response_buf;

	if (ready_msg->ver2_0_info.msg_id != cpu_to_le16(HTC_MSG_READY_ID)) {
		ath6kl_dbg(ATH6KL_DBG_HTC,
			   "invalid htc ready msg : 0x%X !\n",
			   le16_to_cpu(ready_msg->ver2_0_info.msg_id));
		return -ECOMM;
	}

	ath6kl_dbg(ATH6KL_DBG_HTC,
		   "Target Ready! : transmit resources : %d size:%d\n",
		   le16_to_cpu(ready_msg->ver2_0_info.cred_cnt),
		   le16_to_cpu(ready_msg->ver2_0_info.cred_sz));

	target->tgt_creds =
		le16_to_cpu(ready_msg->ver2_0_info.cred_cnt);
	/*reserve one for control path*/
	target->avail_tx_credits = target->tgt_creds - 1;
	target->tgt_cred_sz =
		le16_to_cpu(ready_msg->ver2_0_info.cred_sz);
	if ((target->tgt_creds == 0)
	    || (target->tgt_cred_sz == 0)) {
		return -ECOMM;
	}

	htc_setup_target_buffer_assignments(target);

	/* setup our pseudo HTC control endpoint connection */
	memset(&connect, 0, sizeof(connect));
	memset(&resp, 0, sizeof(resp));
	connect.ep_cb.tx_complete = htc_txctrl_complete;
	connect.ep_cb.rx = htc_rxctrl_complete;
	connect.max_txq_depth = NUM_CONTROL_TX_BUFFERS;
	connect.svc_id = HTC_CTRL_RSVD_SVC;

	/* connect fake service */
	status = ath6kl_htc_pipe_conn_service((void *)target, &connect, &resp);
	return status;
}

static void ath6kl_htc_pipe_flush_txep(struct htc_target *handle,
		enum htc_endpoint_id Endpoint, u16 Tag)
{
	struct htc_target *target = (struct htc_target *) handle;
	struct htc_endpoint *ep = &target->endpoint[Endpoint];

	if (ep->svc_id == 0) {
		WARN_ON(1);
		/* not in use.. */
		return;
	}

	htc_flush_tx_endpoint(target, ep, Tag);
}

static int ath6kl_htc_pipe_add_rxbuf_multiple(struct htc_target *handle,
		struct list_head *pkt_queue)
{
	struct htc_target *target = (struct htc_target *) handle;
	struct htc_endpoint *ep;
	struct htc_packet *pFirstPacket;
	int status = 0;
	struct htc_packet *packet, *tmp_pkt;

	if (list_empty(pkt_queue))
		return -EINVAL;

	pFirstPacket = list_first_entry(pkt_queue, struct htc_packet, list);
	if (pFirstPacket == NULL) {
		WARN_ON(1);
		return -EINVAL;
	}

	if (pFirstPacket->endpoint >= ENDPOINT_MAX) {
		WARN_ON(1);
		return -EINVAL;
	}

	ath6kl_dbg(ATH6KL_DBG_HTC,
		   "%s: epid: %d, cnt:%d, len: %d\n",
		   __func__, pFirstPacket->endpoint,
		   get_queue_depth(pkt_queue), pFirstPacket->buf_len);

	ep = &target->endpoint[pFirstPacket->endpoint];

	spin_lock_bh(&target->rx_lock);

	/* store receive packets */
	list_splice_tail_init(pkt_queue, &ep->rx_bufq);

	spin_unlock_bh(&target->rx_lock);

	if (status != 0) {
		/* walk through queue and mark each one canceled */
		list_for_each_entry_safe(packet, tmp_pkt, pkt_queue, list) {
			packet->status = -ECANCELED;
		}

		do_recv_completion(ep, pkt_queue);
	}

	return status;
}

void ath6kl_htc_pipe_indicate_activity_change(
				struct htc_target *handle,
				enum htc_endpoint_id Endpoint,
				bool Active)
{
	/* TODO */
}

static void ath6kl_htc_pipe_flush_rx_buf(struct htc_target *target)
{
	struct htc_endpoint *endpoint;
	struct htc_packet *packet, *tmp_pkt;
	int i;

	for (i = ENDPOINT_0; i < ENDPOINT_MAX; i++) {
		endpoint = &target->endpoint[i];

		spin_lock_bh(&target->rx_lock);
		list_for_each_entry_safe(packet, tmp_pkt,
					 &endpoint->rx_bufq, list) {
			list_del(&packet->list);
			spin_unlock_bh(&target->rx_lock);
			ath6kl_dbg(ATH6KL_DBG_HTC,
				   "htc rx flush pkt 0x%p  len %d  ep %d\n",
				   packet, packet->buf_len,
				   packet->endpoint);
			dev_kfree_skb(packet->pkt_cntxt);
			spin_lock_bh(&target->rx_lock);
		}
		spin_unlock_bh(&target->rx_lock);
	}
}

void ath6kl_htc_set_credit_dist(struct htc_target *target,
			 struct ath6kl_htc_credit_info *cred_info,
			 u16 svc_pri_order[], int len)
{
	/*
	 * not supported since this transport does not use a credit
	 * based flow control mechanism
	 */
}

static int ath6kl_htc_pipe_credit_setup(struct htc_target *target,
			struct ath6kl_htc_credit_info *cred_info)
{
	return 0;
}

int ath6kl_htc_pipe_stat(struct htc_target *target,
						 u8 *buf, int buf_len)
{
	struct htc_endpoint *ep;
	struct htc_endpoint_stats *ep_st;
	struct list_head *tx_queue;
	int i, tmp, len = 0;

	if ((!target) || (!buf))
		return 0;

	len += snprintf(buf + len, buf_len - len,
			" " "\nCredit Size : %d         Avail Credit : %d/%d\n",
			target->tgt_cred_sz,
			target->avail_tx_credits,
			target->tgt_creds);

	for (i = ENDPOINT_2; i <= ENDPOINT_5; i++) {
		len += snprintf(buf + len, buf_len - len, "EP-%d\n", i);

		ep = &target->endpoint[i];
		tx_queue = &ep->txq;
		ep_st = &ep->ep_st;

		spin_lock_bh(&target->tx_lock);
		tmp = get_queue_depth(tx_queue);
		spin_unlock_bh(&target->tx_lock);

		len += snprintf(buf + len, buf_len - len,
				" tx_queue            : %d/%d\n",
				tmp, ep->max_txq_depth);
		len += snprintf(buf + len, buf_len - len,
				" pipeid_ul/dl        : %d/%d\n",
				ep->pipeid_ul, ep->pipeid_dl);
		len += snprintf(buf + len, buf_len - len,
				" seq_no              : %d\n",
				ep->seqno);
		len += snprintf(buf + len, buf_len - len,
				" cred_low_indicate   : %d\n",
				ep_st->cred_low_indicate);
		len += snprintf(buf + len, buf_len - len,
				" cred_rpt_from_rx    : %d\n",
				ep_st->cred_rpt_from_rx);
		len += snprintf(buf + len, buf_len - len,
				" cred_rpt_from_other : %d\n",
				ep_st->cred_rpt_from_other);
		len += snprintf(buf + len, buf_len - len,
				" cred_rpt_ep0        : %d\n",
				ep_st->cred_rpt_ep0);
		len += snprintf(buf + len, buf_len - len,
				" cred_from_rx        : %d\n",
				ep_st->cred_from_rx);
		len += snprintf(buf + len, buf_len - len,
				" cred_from_other     : %d\n",
				ep_st->cred_from_other);
		len += snprintf(buf + len, buf_len - len,
				" cred_from_ep0       : %d\n",
				ep_st->cred_from_ep0);
		len += snprintf(buf + len, buf_len - len,
				" cred_cosumd         : %d\n",
				ep_st->cred_cosumd);
		len += snprintf(buf + len, buf_len - len,
				" cred_retnd          : %d\n",
				ep_st->cred_retnd);
		len += snprintf(buf + len, buf_len - len,
				" tx_issued           : %d\n",
				ep_st->tx_issued);
		len += snprintf(buf + len, buf_len - len,
				" tx_dropped          : %d\n",
				ep_st->tx_dropped);
		len += snprintf(buf + len, buf_len - len,
				" tx_cred_rpt         : %d\n",
				ep_st->tx_cred_rpt);
		len += snprintf(buf + len, buf_len - len,
				" rx_pkts             : %d\n",
				ep_st->rx_pkts);
		len += snprintf(buf + len, buf_len - len,
				" rx_lkahds           : %d\n",
				ep_st->rx_lkahds);
		len += snprintf(buf + len, buf_len - len,
				" rx_alloc_thresh_hit : %d\n",
				ep_st->rx_alloc_thresh_hit);
		len += snprintf(buf + len, buf_len - len,
				" rxalloc_thresh_byte : %d\n",
				ep_st->rxalloc_thresh_byte);

		/* Bundle mode */
		if (htc_bundle_recv || htc_bundle_send) {
			len += snprintf(buf + len, buf_len - len,
					" tx_pkt_bundled      : %d\n",
					ep_st->tx_pkt_bundled);
			len += snprintf(buf + len, buf_len - len,
					" tx_bundles          : %d\n",
					ep_st->tx_bundles);
			len += snprintf(buf + len, buf_len - len,
					" rx_bundl            : %d\n",
					ep_st->rx_bundl);
			len += snprintf(buf + len, buf_len - len,
					" rx_bundle_lkahd     : %d\n",
					ep_st->rx_bundle_lkahd);
			len += snprintf(buf + len, buf_len - len,
					" rx_bundle_from_hdr  : %d\n",
					ep_st->rx_bundle_from_hdr);
		}
	}

	return len;
}

int ath6kl_htc_pipe_stop_netif_queue_full(struct htc_target *target)
{
	return 1;
}

int ath6kl_htc_pipe_wmm_schedule_change(struct htc_target *target,
		bool change)
{
	return 0;
}

int ath6kl_htc_pipe_change_credit_bypass(struct htc_target *target,
		u8 traffic_class)
{
	return 0;
}

static const struct ath6kl_htc_ops ath6kl_htc_pipe_ops = {
	.create = ath6kl_htc_pipe_create,
	.wait_target = ath6kl_htc_pipe_wait_target,
	.start = ath6kl_htc_pipe_start,
	.conn_service = ath6kl_htc_pipe_conn_service,
	.tx = ath6kl_htc_pipe_tx,
	.stop = ath6kl_htc_pipe_stop,
	.cleanup = ath6kl_htc_pipe_cleanup,
	.flush_txep = ath6kl_htc_pipe_flush_txep,
	.flush_rx_buf = ath6kl_htc_pipe_flush_rx_buf,
	.indicate_activity_change = ath6kl_htc_pipe_indicate_activity_change,
	.get_rxbuf_num = ath6kl_htc_pipe_get_rxbuf_num,
	.add_rxbuf_multiple = ath6kl_htc_pipe_add_rxbuf_multiple,
	.credit_setup = ath6kl_htc_pipe_credit_setup,
	.get_stat = ath6kl_htc_pipe_stat,
	.stop_netif_queue_full = ath6kl_htc_pipe_stop_netif_queue_full,
	.indicate_wmm_schedule_change = ath6kl_htc_pipe_wmm_schedule_change,
	.change_credit_bypass = ath6kl_htc_pipe_change_credit_bypass,
};

void ath6kl_htc_pipe_attach(struct ath6kl *ar)
{
	ar->htc_ops = &ath6kl_htc_pipe_ops;
}
