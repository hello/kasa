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

#include "core.h"
#include "debug.h"
#include "htc-ops.h"
#include "epping.h"
#include <linux/version.h>

/* 802.1d to AC mapping. Refer pg 57 of WMM-test-plan-v1.2 */
static const u8 up_to_ac[] = {
	WMM_AC_BE,
	WMM_AC_BK,
	WMM_AC_BK,
	WMM_AC_BE,
	WMM_AC_VI,
	WMM_AC_VI,
	WMM_AC_VO,
	WMM_AC_VO,
};

static int aggr_tx(struct ath6kl_vif *vif, struct ath6kl_sta *sta,
		   struct sk_buff **skb);
static int aggr_tx_flush(struct ath6kl_vif *vif, struct ath6kl_sta *conn);

static void ath6kl_eapol_handshake_protect(struct ath6kl_vif *vif, bool tx);

static u8 ath6kl_ibss_map_epid(struct sk_buff *skb, struct net_device *dev,
			       u32 *map_no)
{
	struct ath6kl *ar = ath6kl_priv(dev);
	struct ethhdr *eth_hdr;
	u32 i, ep_map = -1;
	u8 *datap;

	*map_no = 0;
	datap = skb->data;
	eth_hdr = (struct ethhdr *) (datap + sizeof(struct wmi_data_hdr));

	if (is_multicast_ether_addr(eth_hdr->h_dest))
		return ENDPOINT_2;

	for (i = 0; i < ar->node_num; i++) {
		if (memcmp(eth_hdr->h_dest, ar->node_map[i].mac_addr,
			   ETH_ALEN) == 0) {
			*map_no = i + 1;
			ar->node_map[i].tx_pend++;
			return ar->node_map[i].ep_id;
		}

		if ((ep_map == -1) && !ar->node_map[i].tx_pend)
			ep_map = i;
	}

	if (ep_map == -1) {
		ep_map = ar->node_num;
		ar->node_num++;
		if (ar->node_num > MAX_NODE_NUM)
			return ENDPOINT_UNUSED;
	}

	memcpy(ar->node_map[ep_map].mac_addr, eth_hdr->h_dest, ETH_ALEN);

	for (i = ENDPOINT_2; i <= ENDPOINT_5; i++) {
		if (!ar->tx_pending[i]) {
			ar->node_map[ep_map].ep_id = i;
			break;
		}

		/*
		 * No free endpoint is available, start redistribution on
		 * the inuse endpoints.
		 */
		if (i == ENDPOINT_5) {
			ar->node_map[ep_map].ep_id = ar->next_ep_id;
			ar->next_ep_id++;
			if (ar->next_ep_id > ENDPOINT_5)
				ar->next_ep_id = ENDPOINT_2;
		}
	}

	*map_no = ep_map + 1;
	ar->node_map[ep_map].tx_pend++;

	return ar->node_map[ep_map].ep_id;
}

static inline bool _powersave_ap_tx_multicast(struct ath6kl_vif *vif,
	struct sk_buff *skb, u32 *flags)
{
	u8 ctr = 0;
	bool q_mcast = false, ps_queued = false;
	int ret;

	for (ctr = 0; ctr < AP_MAX_NUM_STA; ctr++) {
		if (vif->sta_list[ctr].sta_flags & STA_PS_SLEEP) {
			q_mcast = true;
			break;
		}
	}

	ath6kl_dbg(ATH6KL_DBG_POWERSAVE,
			"%s: Multicast %d psq_mcast %d\n",
			__func__,
			q_mcast,
			!ath6kl_ps_queue_empty(&vif->psq_mcast));

	if (q_mcast) {
		/*
		 * If this transmit is not because of a Dtim Expiry
		 * q it.
		 */
		if (!test_bit(DTIM_EXPIRED, &vif->flags)) {
			bool is_psq_empty = false;

			spin_lock_bh(&vif->psq_mcast_lock);
			is_psq_empty = ath6kl_ps_queue_empty(&vif->psq_mcast);
			ret = ath6kl_ps_queue_enqueue_data(
				&vif->psq_mcast, skb);
			spin_unlock_bh(&vif->psq_mcast_lock);

			if (ret == 0) {
				/*
				 * If this is the first Mcast pkt getting
				 * queued indicate to the target to set the
				 * BitmapControl LSB of the TIM IE.
				 */
				if (is_psq_empty)
					ath6kl_wmi_set_pvb_cmd(vif->ar->wmi,
								vif->fw_vif_idx,
								MCAST_AID,
								1);
			} else {
				/* drop this packet */
				dev_kfree_skb(skb);
			}
			ps_queued = true;
		} else {
			/*
			 * This transmit is because of Dtim expiry.
			 * Determine if MoreData bit has to be set.
			 */
			spin_lock_bh(&vif->psq_mcast_lock);
			if (!ath6kl_ps_queue_empty(&vif->psq_mcast))
				*flags |= WMI_DATA_HDR_FLAGS_MORE;
			spin_unlock_bh(&vif->psq_mcast_lock);
		}
	}

	return ps_queued;
}

static inline  void __powersave_ap_tx_unicast_sleep(struct ath6kl_vif *vif,
	struct ath6kl_sta *conn, struct sk_buff *skb, u32 *flags)
{
	struct ethhdr *datap = (struct ethhdr *) skb->data;
	bool trigger = false, is_psq_empty = false;
	int ret;

	if (conn->apsd_info) {
		u8 up = 0;
		u8 traffic_class;

		if (test_bit(WMM_ENABLED, &vif->flags)) {
			struct ath6kl_llc_snap_hdr *llc_hdr;
			u16 ether_type;
			u16 ip_type = IP_ETHERTYPE;
			u8 *ip_hdr;

			ether_type = datap->h_proto;
			if (is_ethertype(be16_to_cpu(ether_type))) {
				/* packet is in DIX format  */
				ip_hdr = (u8 *)(datap + 1);
			} else {
				/* packet is in 802.3 format */
				llc_hdr = (struct ath6kl_llc_snap_hdr *)
						(datap + 1);
				ether_type = llc_hdr->eth_type;
				ip_hdr = (u8 *)(llc_hdr + 1);
			}

			if (ether_type == cpu_to_be16(ip_type))
				up = ath6kl_wmi_determine_user_priority(ip_hdr,
									0);
		}
		traffic_class = up_to_ac[up & 0x7];
		if (conn->apsd_info & (1 << traffic_class))
			trigger = true;
	}

	/* Queue the frames if the STA is sleeping */
	spin_lock_bh(&conn->lock);
	ath6kl_dbg(ATH6KL_DBG_POWERSAVE,
			"%s: Unicast aid %d sta_flags %x apsd_info %d"
			" psq_data %d psq_mgmt %d traffic %d\n",
			__func__,
			conn->aid,
			conn->sta_flags,
			conn->apsd_info,
			!ath6kl_ps_queue_empty(&conn->psq_data),
			!ath6kl_ps_queue_empty(&conn->psq_mgmt),
			trigger);

	is_psq_empty = ath6kl_ps_queue_empty(&conn->psq_data) &&
			ath6kl_ps_queue_empty(&conn->psq_mgmt);

	ret = ath6kl_ps_queue_enqueue_data(&conn->psq_data, skb);
	spin_unlock_bh(&conn->lock);

	if (ret == 0) {
		if (is_psq_empty) {
			if (trigger)
				ath6kl_wmi_set_apsd_buffered_traffic_cmd(
							vif->ar->wmi,
							vif->fw_vif_idx,
							conn->aid,
							1,
							0);

			/*
			 * If this is the first pkt getting quened for this STA,
			 * update the PVB for this STA.
			 */
			ath6kl_wmi_set_pvb_cmd(vif->ar->wmi,
						vif->fw_vif_idx,
						conn->aid,
						1);
		}
	} else {
		/* drop this packet */
		dev_kfree_skb(skb);
	}

	return;
}

static inline  void __powersave_ap_tx_unicast_awake(struct ath6kl_vif *vif,
	struct ath6kl_sta *conn, u32 *flags)
{
	/*
	 * This tx is because of a PsPoll or trigger.
	 * Determine if MoreData bit has to be set
	 */
	spin_lock_bh(&conn->lock);
	ath6kl_dbg(ATH6KL_DBG_POWERSAVE,
			"%s: Unicast aid %d sta_flags %x apsd_info %d"
			" psq_data %d psq_mgmt %d\n",
			__func__,
			conn->aid,
			conn->sta_flags,
			conn->apsd_info,
			!ath6kl_ps_queue_empty(&conn->psq_data),
			!ath6kl_ps_queue_empty(&conn->psq_mgmt));

	if (!ath6kl_ps_queue_empty(&conn->psq_data) ||
		!ath6kl_ps_queue_empty(&conn->psq_mgmt))
		*flags |= WMI_DATA_HDR_FLAGS_MORE;

	if (!(conn->sta_flags & STA_PS_POLLED)) {
		/*
		 * This tx is because of a uAPSD trigger, determine more and
		 * EOSP bit. Set EOSP is queue is empty or sufficient frames is
		 * delivered for this trigger
		 */
		*flags |= WMI_DATA_HDR_FLAGS_TRIGGERED;

		if (conn->sta_flags & STA_PS_APSD_EOSP)
			*flags |= WMI_DATA_HDR_FLAGS_EOSP;
	} else
		*flags |= WMI_DATA_HDR_FLAGS_PSPOLLED;
	spin_unlock_bh(&conn->lock);

	return;
}

static inline  bool _powersave_ap_tx_unicast(struct ath6kl_vif *vif,
	struct sk_buff *skb, u32 *flags, struct ath6kl_sta **sta)
{
	struct ethhdr *datap = (struct ethhdr *) skb->data;
	struct ath6kl_sta *conn = NULL;
	bool ps_queued = false;

	conn = ath6kl_find_sta(vif, datap->h_dest);
	if (!conn) {
		dev_kfree_skb(skb);

		/* Inform the caller that the skb is consumed */
		return true;
	}

	*sta = conn;

	if (conn->sta_flags & STA_PS_SLEEP) {
		if (!((conn->sta_flags & STA_PS_POLLED) ||
			(conn->sta_flags & STA_PS_APSD_TRIGGER))) {
			__powersave_ap_tx_unicast_sleep(vif, conn, skb, flags);
			ps_queued = true;
		} else
			__powersave_ap_tx_unicast_awake(vif, conn, flags);
	}

	return ps_queued;
}

static bool ath6kl_powersave_ap(struct ath6kl_vif *vif, struct sk_buff *skb,
				u32 *flags,
				struct ath6kl_sta **sta)
{
	struct ethhdr *datap = (struct ethhdr *) skb->data;
	bool ps_queued = false;

	if (is_multicast_ether_addr(datap->h_dest))
		ps_queued = _powersave_ap_tx_multicast(vif, skb, flags);
	else
		ps_queued = _powersave_ap_tx_unicast(vif, skb, flags, sta);

	return ps_queued;
}

bool ath6kl_mgmt_powersave_ap(struct ath6kl_vif *vif,
				u32 id,
				u32 freq,
				u32 wait,
				const u8 *buf,
				size_t len,
				bool no_cck,
				bool dont_wait_for_ack,
				u32 *flags)
{
	struct ieee80211_mgmt *mgmt;
	struct ath6kl_sta *conn = NULL;
	bool ps_queued = false, is_psq_empty = false;
	int ret;

	mgmt = (struct ieee80211_mgmt *)buf;
	if (is_multicast_ether_addr(mgmt->da)) {
		return false;
	} else {
		conn = ath6kl_find_sta(vif, mgmt->da);
		if (!conn)
			return false;

		if (conn->sta_flags & STA_PS_SLEEP) {
			if (!(conn->sta_flags & STA_PS_POLLED)) {
				/* Queue the frames if the STA is sleeping */
				spin_lock_bh(&conn->lock);
				ath6kl_dbg(ATH6KL_DBG_POWERSAVE,
					"%s: Mgmt aid %d sta_flags %x psq_data %d psq_mgmt %d\n",
					__func__, conn->aid, conn->sta_flags,
					!ath6kl_ps_queue_empty(&conn->psq_data),
					!ath6kl_ps_queue_empty(&conn->psq_mgmt)
					);

				is_psq_empty = ath6kl_ps_queue_empty(
					&conn->psq_data) &&
					ath6kl_ps_queue_empty(&conn->psq_mgmt);

				ret = ath6kl_ps_queue_enqueue_mgmt(
					&conn->psq_mgmt,
					buf,
					len,
					id,
					freq,
					wait,
					no_cck,
					dont_wait_for_ack);
				spin_unlock_bh(&conn->lock);

				if (ret == 0) {
					/*
					* If this is the first pkt getting
					* queued for this STA, update the PVB
					* for this STA.
					*/
					if (is_psq_empty)
						ath6kl_wmi_set_pvb_cmd(
							vif->ar->wmi,
							vif->fw_vif_idx,
							conn->aid, 1);
				} else {
					;
				}
				ps_queued = true;
			} else {
				/*
				* This tx is because of a PsPoll.
				* Determine if MoreData bit has to be set.
				*/
				spin_lock_bh(&conn->lock);
				ath6kl_dbg(ATH6KL_DBG_POWERSAVE,
					"%s: Mgmt aid %d sta_flags %x psq_data %d psq_mgmt %d\n",
					__func__, conn->aid, conn->sta_flags,
					!ath6kl_ps_queue_empty(&conn->psq_data),
					!ath6kl_ps_queue_empty(&conn->psq_mgmt)
					);

				if (!ath6kl_ps_queue_empty(&conn->psq_data) ||
					!ath6kl_ps_queue_empty(&conn->psq_mgmt))
					*flags |= WMI_DATA_HDR_FLAGS_MORE;
				spin_unlock_bh(&conn->lock);
			}
		}
	}

	return ps_queued;
}

/* Tx functions */

int ath6kl_control_tx(void *devt, struct sk_buff *skb,
		      enum htc_endpoint_id eid)
{
	struct ath6kl *ar = devt;
	int status = 0;
	struct ath6kl_cookie *cookie = NULL;

	spin_lock_bh(&ar->lock);

	ath6kl_dbg(ATH6KL_DBG_WLAN_TX,
		   "%s: skb=0x%p, len=0x%x eid =%d\n", __func__,
		   skb, skb->len, eid);

	if (test_bit(WMI_CTRL_EP_FULL, &ar->flag) && (eid == ar->ctrl_ep)) {
		/*
		 * Control endpoint is full, don't allocate resources, we
		 * are just going to drop this packet.
		 */
		cookie = NULL;
		ath6kl_err("wmi ctrl ep full, dropping pkt : 0x%p, len:%d\n",
			   skb, skb->len);
	} else
		cookie = ath6kl_alloc_cookie(ar, COOKIE_TYPE_CTRL);

	if (cookie == NULL) {
		spin_unlock_bh(&ar->lock);
		status = -ENOMEM;
		goto fail_ctrl_tx;
	}

	ar->tx_pending[eid]++;

	if (eid != ar->ctrl_ep)
		ar->total_tx_data_pend++;

	spin_unlock_bh(&ar->lock);

	cookie->skb = skb;
	cookie->map_no = 0;
	set_htc_pkt_info(cookie->htc_pkt, cookie, skb->data, skb->len,
			 eid, ATH6KL_CONTROL_PKT_TAG);
	cookie->htc_pkt->skb = skb;

	/* P2P Flowctrl */
	if (ar->conf_flags & ATH6KL_CONF_ENABLE_FLOWCTRL) {
		cookie->htc_pkt->connid = ATH6KL_P2P_FLOWCTRL_NULL_CONNID;
		cookie->htc_pkt->recycle_count = 0;
	}

	/*null data's vif since it is not applied */
	cookie->htc_pkt->vif = NULL;

	/*
	 * This interface is asynchronous, if there is an error, cleanup
	 * will happen in the TX completion callback.
	 */
	ath6kl_htc_tx(ar->htc_target, cookie->htc_pkt);

	return 0;

fail_ctrl_tx:
	dev_kfree_skb(skb);
	return status;
}

int ath6kl_data_tx(struct sk_buff *skb, struct net_device *dev,
		   bool bypass_tx_aggr)
{
	struct ath6kl *ar = ath6kl_priv(dev);
	struct ath6kl_cookie *cookie = NULL;
	enum htc_endpoint_id eid = ENDPOINT_UNUSED;
	struct ath6kl_vif *vif = netdev_priv(dev);
	u32 map_no = 0;
	u16 htc_tag = ATH6KL_DATA_PKT_TAG;
	u8 ac = 99 ; /* initialize to unmapped ac */
	bool chk_adhoc_ps_mapping = false;
	u32 wmi_data_flags = 0;
	int ret, aggr_tx_status = AGGR_TX_UNKNOW;
	struct ath6kl_sta *conn = NULL;

	/* If target is not associated */
	if (!test_bit(CONNECTED, &vif->flags) &&
	    !test_bit(TESTMODE_EPPING, &ar->flag)) {
		dev_kfree_skb(skb);
		return 0;
	}

	if (!test_bit(WMI_READY, &ar->flag) &&
	    !test_bit(TESTMODE_EPPING, &ar->flag)) {
		goto fail_tx;
	}

	if ((ar->conf_flags & ATH6KL_CONF_SKB_DUP) &&
		(skb_cloned(skb) || skb_shared(skb))) {
		struct sk_buff *nskb;
		nskb = skb_copy(skb, GFP_ATOMIC);
		if (nskb == NULL)
			goto fail_tx;
		dev_kfree_skb_any(skb);
		skb = nskb;
	}

	ath6kl_dbg(ATH6KL_DBG_WLAN_TX,
		"%s: skb=0x%p, data=0x%p, len=0x%x\n", __func__,
		skb, skb->data, skb->len);

	/* AP mode Power saving processing */
	if (vif->nw_type == AP_NETWORK) {
		if (ath6kl_powersave_ap(vif, skb, &wmi_data_flags, &conn))
			return 0;
	}

	if (test_bit(WMI_ENABLED, &ar->flag)) {
		if (skb_headroom(skb) < vif->needed_headroom) {
			struct sk_buff *tmp_skb = ath6kl_buf_alloc(skb->len);

			if (tmp_skb == NULL) {
				vif->net_stats.tx_dropped++;
				goto fail_tx;
			}

			skb_put(tmp_skb, skb->len);
			memcpy(tmp_skb->data, skb->data, skb->len);
			kfree_skb(skb);
			skb = tmp_skb;
		}

		if (ath6kl_wmi_dix_2_dot3(ar->wmi, skb)) {
			ath6kl_err("ath6kl_wmi_dix_2_dot3 failed\n");
			goto fail_tx;
		}

		if (ath6kl_wmi_data_hdr_add(ar->wmi, skb, DATA_MSGTYPE,
					    wmi_data_flags, 0, 0, NULL,
					    vif->fw_vif_idx)) {
			ath6kl_err("wmi_data_hdr_add failed\n");
			goto fail_tx;
		}

		if ((vif->nw_type == ADHOC_NETWORK) &&
		     ar->ibss_ps_enable && test_bit(CONNECTED, &vif->flags))
			chk_adhoc_ps_mapping = true;
		else {
			/* get the stream mapping */
			ret = ath6kl_wmi_implicit_create_pstream(ar->wmi,
					vif->fw_vif_idx, skb,
					0, test_bit(WMM_ENABLED, &vif->flags),
					&ac, &htc_tag);
			if (ret)
				goto fail_tx;
		}
	}  else if (test_bit(TESTMODE_EPPING, &ar->flag)) {
		struct epping_header    *epping_hdr;

		epping_hdr = (struct epping_header *)skb->data;

		if (IS_EPPING_PACKET(epping_hdr)) {
			ac = epping_hdr->stream_no_h;

			/* some EPPING packets cannot be dropped no matter
			 * what access class it was sent on. Change the packet
			 * tag to guarantee it will not get dropped
			 */
			if (IS_EPING_PACKET_NO_DROP(epping_hdr))
				htc_tag = ATH6KL_CONTROL_PKT_TAG;


			if (ac == HCI_TRANSPORT_STREAM_NUM) {
				goto fail_tx;
			} else {
				/* The payload of the frame is 32-bit aligned
				 * and thus the addition of the HTC header will
				 * mis-align the start of the HTC frame,
				 * the padding will be stripped off in the
				 * target */
				if (EPPING_ALIGNMENT_PAD > 0)
					skb_push(skb, EPPING_ALIGNMENT_PAD);

			}
		} else {
		/* In loopback mode, drop non-loopback packet */
			goto fail_tx;
		}
	} else
		goto fail_tx;

	if (test_bit(CONNECTED, &vif->flags) &&
	    (skb->protocol == cpu_to_be16(ETH_P_PAE)))
		ath6kl_eapol_handshake_protect(vif, true);

	/* TX A-MSDU */
	if ((test_bit(AMSDU_ENABLED, &vif->flags)) &&
		(!bypass_tx_aggr) &&
		(vif->aggr_cntxt->tx_amsdu_enable) &&
		(!chk_adhoc_ps_mapping) &&
		(vif->nw_type & (INFRA_NETWORK | AP_NETWORK))) {
		aggr_tx_status = aggr_tx(vif, conn, &skb);

		if (aggr_tx_status == AGGR_TX_OK)
			return 0;
		else if (aggr_tx_status == AGGR_TX_DROP)
			goto fail_tx;

		WARN_ON(skb == NULL);

		if ((vif->aggr_cntxt->tx_amsdu_seq_pkt) &&
			(aggr_tx_status == AGGR_TX_BYPASS))
			aggr_tx_flush(vif, conn);
	}

	spin_lock_bh(&ar->lock);

	if (chk_adhoc_ps_mapping)
		eid = ath6kl_ibss_map_epid(skb, dev, &map_no);
	else
		eid = ar->ac2ep_map[ac];

	if (eid == 0 || eid == ENDPOINT_UNUSED) {
		if ((ac == WMM_NUM_AC) &&
			test_bit(TESTMODE_EPPING, &ar->flag)) {
			/* for epping testing, the last AC maps to the control
			 * endpoint
			 */
			eid = ar->ctrl_ep;
		} else {
			ath6kl_err("eid %d is not mapped!\n", eid);
			spin_unlock_bh(&ar->lock);
			goto fail_tx;
		}
	}

	/* allocate resource for this packet */
	if (htc_tag == ATH6KL_DATA_PKT_TAG) {
		if (test_bit(MCC_ENABLED, &ar->flag)) {
			if (vif->data_cookie_count <= MAX_VIF_COOKIE_NUM) {
				cookie = ath6kl_alloc_cookie(ar,
					COOKIE_TYPE_DATA);
			}
		} else
			cookie = ath6kl_alloc_cookie(ar,
				COOKIE_TYPE_DATA);
	} else
		cookie = ath6kl_alloc_cookie(ar, COOKIE_TYPE_CTRL);

	if (!cookie) {
		spin_unlock_bh(&ar->lock);
		goto fail_tx;
	}

	if (htc_tag == ATH6KL_DATA_PKT_TAG)
		vif->data_cookie_count++;

	/* update counts while the lock is held */
	ar->tx_pending[eid]++;
	ar->total_tx_data_pend++;

	spin_unlock_bh(&ar->lock);

	if (!IS_ALIGNED((unsigned long) skb->data - HTC_HDR_LENGTH, 4) &&
	    skb_cloned(skb)) {
		/*
		 * We will touch (move the buffer data to align it. Since the
		 * skb buffer is cloned and not only the header is changed, we
		 * have to copy it to allow the changes. Since we are copying
		 * the data here, we may as well align it by reserving suitable
		 * headroom to avoid the memmove in ath6kl_htc_tx_buf_align().
		 */
		struct sk_buff *nskb;

		nskb = skb_copy_expand(skb, HTC_HDR_LENGTH, 0, GFP_ATOMIC);
		if (nskb == NULL)
			goto fail_tx;
		kfree_skb(skb);
		skb = nskb;
	}

	cookie->skb = skb;
	cookie->map_no = map_no;
	set_htc_pkt_info(cookie->htc_pkt, cookie, skb->data, skb->len,
			 eid, htc_tag);
	cookie->htc_pkt->skb = skb;

	ath6kl_dbg_dump(ATH6KL_DBG_RAW_BYTES, __func__, "tx ",
			skb->data, skb->len);

	/* P2P Flowctrl */
	if (ar->conf_flags & ATH6KL_CONF_ENABLE_FLOWCTRL) {
		cookie->htc_pkt->connid =
			ath6kl_p2p_flowctrl_get_conn_id(vif, skb);
		cookie->htc_pkt->recycle_count = 0;
		ret = ath6kl_p2p_flowctrl_tx_schedule_pkt(ar, (void *)cookie);
		if (ret == 0)		/* Queue it */
			return 0;
		else if (ret < 0)	/* Error, drop it. */
			goto fail_tx;
	}

	cookie->htc_pkt->vif = vif;

	ar->tx_on_vif |= (1 << vif->fw_vif_idx);

	/*
	 * HTC interface is asynchronous, if this fails, cleanup will
	 * happen in the ath6kl_tx_complete callback.
	 */
	ath6kl_htc_tx(ar->htc_target, cookie->htc_pkt);

	return 0;

fail_tx:
	dev_kfree_skb(skb);

	vif->net_stats.tx_dropped++;
	vif->net_stats.tx_aborted_errors++;

	return 0;
}

int ath6kl_start_tx(struct sk_buff *skb, struct net_device *dev)
{
	/* Current design only aggr. the packets sent from the host. */
	return ath6kl_data_tx(skb, dev, false);
}

/* indicate tx activity or inactivity on a WMI stream */
void ath6kl_indicate_tx_activity(void *devt, u8 traffic_class, bool active)
{
	struct ath6kl *ar = devt;
	enum htc_endpoint_id eid;
	int i;
	u8 num_stream_active;

	eid = ar->ac2ep_map[traffic_class];

	if (!test_bit(WMI_ENABLED, &ar->flag))
		goto notify_htc;

	spin_lock_bh(&ar->lock);

	ar->ac_stream_active[traffic_class] = active;

	if (active) {
		/*
		 * Keep track of the active stream with the highest
		 * priority.
		 */
		if (ar->ac_stream_pri_map[traffic_class] >
		    ar->hiac_stream_active_pri)
			/* set the new highest active priority */
			ar->hiac_stream_active_pri =
					ar->ac_stream_pri_map[traffic_class];

		if (ath6kl_htc_change_credit_bypass(ar->htc_target,
					traffic_class)) {
			struct ath6kl_vif *vif;
			vif = ath6kl_vif_first(ar);

			spin_unlock_bh(&ar->lock);

			if (vif)
				ath6kl_wmi_set_credit_bypass(ar->wmi,
					vif->fw_vif_idx,
					ar->ac2ep_map[WMM_AC_BE], 0, 6);

			spin_lock_bh(&ar->lock);
		}
	} else {
		/*
		 * We may have to search for the next active stream
		 * that is the highest priority.
		 */
		if (ar->hiac_stream_active_pri ==
			ar->ac_stream_pri_map[traffic_class]) {
			/*
			 * The highest priority stream just went inactive
			 * reset and search for the "next" highest "active"
			 * priority stream.
			 */
			ar->hiac_stream_active_pri = 0;

			for (i = 0; i < WMM_NUM_AC; i++) {
				if (ar->ac_stream_active[i] &&
				    (ar->ac_stream_pri_map[i] >
				     ar->hiac_stream_active_pri))
					/*
					 * Set the new highest active
					 * priority.
					 */
					ar->hiac_stream_active_pri =
						ar->ac_stream_pri_map[i];
			}
		}

		if (ath6kl_htc_change_credit_bypass(ar->htc_target,
					traffic_class)) {
			struct ath6kl_vif *vif;
			vif = ath6kl_vif_first(ar);

			spin_unlock_bh(&ar->lock);

			if (vif)
				ath6kl_wmi_set_credit_bypass(ar->wmi,
						vif->fw_vif_idx,
						ar->ac2ep_map[WMM_AC_BE],
						1, 1);

			spin_lock_bh(&ar->lock);
		}
	}

	/* check the number of active stream */
	num_stream_active = 0;

	for (i = 0; i < WMM_NUM_AC; i++) {
		if (ar->ac_stream_active[i] == true)
			num_stream_active++;
	}

	ar->ac_stream_active_num = num_stream_active;

	spin_unlock_bh(&ar->lock);

notify_htc:
	/* notify HTC, this may cause credit distribution changes */
	ath6kl_htc_indicate_activity_change(ar->htc_target, eid, active);
}

enum htc_send_full_action ath6kl_tx_queue_full(struct htc_target *target,
					       struct htc_packet *packet)
{
	struct ath6kl *ar = target->dev->ar;
	struct ath6kl_vif *vif;
	enum htc_endpoint_id endpoint = packet->endpoint;
	enum htc_send_full_action action = HTC_SEND_FULL_KEEP;

	if (test_bit(TESTMODE_EPPING, &ar->flag)) {
		int ac;

		if (packet->info.tx.tag == ATH6KL_CONTROL_PKT_TAG) {
			/* don't drop special control packets */
			return HTC_SEND_FULL_KEEP;
		}

		ac = ar->ep2ac_map[endpoint];

		/* for endpoint ping testing drop Best Effort and Background
		 * if any of the higher priority traffic is active */
		if ((ar->ac_stream_active[WMM_AC_VO] ||
			ar->ac_stream_active[WMM_AC_BE]) &&
			((ac == WMM_AC_BE) || (ac == WMM_AC_BK))) {
			return HTC_SEND_FULL_DROP;
		} else {
			spin_lock_bh(&ar->list_lock);
			list_for_each_entry(vif, &ar->vif_list, list) {

				/* keep but stop the netqueues */
				set_bit(NETQ_STOPPED, &vif->flags);
				netif_stop_queue(vif->ndev);
			}
			spin_unlock_bh(&ar->list_lock);
			return HTC_SEND_FULL_KEEP;
		}
	}

	if (endpoint == ar->ctrl_ep) {
		/*
		 * Under normal WMI if this is getting full, then something
		 * is running rampant the host should not be exhausting the
		 * WMI queue with too many commands the only exception to
		 * this is during testing using endpointping.
		 */
		spin_lock_bh(&ar->lock);
		set_bit(WMI_CTRL_EP_FULL, &ar->flag);
		spin_unlock_bh(&ar->lock);
		ath6kl_err("wmi ctrl ep is full\n");
		ath6kl_fw_crash_trap(ar);
		return action;
	}

	if ((packet->info.tx.tag == ATH6KL_CONTROL_PKT_TAG) ||
			(packet->info.tx.tag == ATH6KL_PRI_DATA_PKT_TAG))
		return action;

	/*
	 * The last MAX_HI_COOKIE_NUM "batch" of cookies are reserved for
	 * the highest active stream.
	 */
	if (ar->ac_stream_pri_map[ar->ep2ac_map[endpoint]] <
	    ar->hiac_stream_active_pri &&
	    ar->cookie_data.cookie_count <=
			target->endpoint[endpoint].tx_drop_packet_threshold){
		/*
		 * Give preference to the highest priority stream by
		 * dropping the packets which overflowed.
		 */
		action = HTC_SEND_FULL_DROP;
	} else if (ar->vif_max > 1) { /* WAR: EV108182 */
		int i, ongoing_tx = 0;

		for (i = 0; i < ar->vif_max; i++) {
			if (ar->tx_on_vif & (1 << i))
				ongoing_tx++;
		}

		if (ongoing_tx > 1)
			return action;
	}

	/* FIXME: Locking */
	spin_lock_bh(&ar->list_lock);
	list_for_each_entry(vif, &ar->vif_list, list) {
		if (vif->nw_type == ADHOC_NETWORK ||
		    action != HTC_SEND_FULL_DROP) {
			spin_unlock_bh(&ar->list_lock);

			if (ath6kl_htc_stop_netif_queue_full(ar->htc_target) ||
			    vif->nw_type == INFRA_NETWORK ||
			    ar->ac_stream_active_num == 1) {
				set_bit(NETQ_STOPPED, &vif->flags);
				netif_stop_queue(vif->ndev);
			}

			spin_lock_bh(&ar->list_lock);
		}
	}
	spin_unlock_bh(&ar->list_lock);

	return action;
}

/* TODO this needs to be looked at */
static void ath6kl_tx_clear_node_map(struct ath6kl_vif *vif,
				     enum htc_endpoint_id eid, u32 map_no)
{
	struct ath6kl *ar = vif->ar;
	u32 i;

	if (vif->nw_type != ADHOC_NETWORK)
		return;

	if (!ar->ibss_ps_enable)
		return;

	if (eid == ar->ctrl_ep)
		return;

	if (map_no == 0)
		return;

	map_no--;
	ar->node_map[map_no].tx_pend--;

	if (ar->node_map[map_no].tx_pend)
		return;

	if (map_no != (ar->node_num - 1))
		return;

	for (i = ar->node_num; i > 0; i--) {
		if (ar->node_map[i - 1].tx_pend)
			break;

		memset(&ar->node_map[i - 1], 0,
		       sizeof(struct ath6kl_node_mapping));
		ar->node_num--;
	}
}

void ath6kl_tx_complete(struct htc_target *target,
				struct list_head *packet_queue)
{
	struct ath6kl *ar = target->dev->ar;
	struct sk_buff_head skb_queue;
	struct htc_packet *packet = NULL;
	struct sk_buff *skb;
	struct ath6kl_cookie *ath6kl_cookie;
	u32 map_no = 0;
	int status;
	enum htc_endpoint_id eid = 0;
	bool wake_event = false;
	bool flushing[ATH6KL_VIF_MAX] = {false};
	u8 if_idx;
	struct ath6kl_vif *vif = NULL;
	struct htc_endpoint *endpoint = NULL;
	int txq_depth;

	skb_queue_head_init(&skb_queue);

	/* lock the driver as we update internal state */
	spin_lock_bh(&ar->lock);

	/* reap completed packets */
	while (!list_empty(packet_queue)) {

		packet = list_first_entry(packet_queue, struct htc_packet,
					  list);
		list_del(&packet->list);

		ath6kl_cookie = (struct ath6kl_cookie *)packet->pkt_cntxt;
		if (!ath6kl_cookie)
			goto fatal;

		status = packet->status;
		skb = ath6kl_cookie->skb;
		eid = packet->endpoint;
		map_no = ath6kl_cookie->map_no;

		if (!skb || !skb->data)
			goto fatal;

		if (eid == ENDPOINT_UNUSED || eid == ENDPOINT_MAX)
			goto fatal;

		__skb_queue_tail(&skb_queue, skb);

		if (!status && (packet->act_len != skb->len))
			goto fatal;

		ar->tx_pending[eid]--;

		if (!test_bit(TESTMODE_EPPING, &ar->flag)) {
			if (eid != ar->ctrl_ep)
				ar->total_tx_data_pend--;

			if (eid == ar->ctrl_ep) {
				if (test_bit(WMI_CTRL_EP_FULL, &ar->flag))
					clear_bit(WMI_CTRL_EP_FULL, &ar->flag);

				if (ar->tx_pending[eid] == 0)
					wake_event = true;
			}

			if (eid == ar->ctrl_ep) {
				if_idx = wmi_cmd_hdr_get_if_idx(
					(struct wmi_cmd_hdr *) packet->buf);
			} else {
				if_idx = wmi_data_hdr_get_if_idx(
					(struct wmi_data_hdr *) packet->buf);
			}
		} else {
			/* The epping packet is not coming from wmi,
			 * skip the index retrival, epping assume using the
			 * first if_idx anyway
			 */
			if_idx = 0;
		}

		vif = ath6kl_get_vif_by_index(ar, if_idx);
		if (!vif) {
			ath6kl_free_cookie(ar, ath6kl_cookie);
			continue;
		}

		ar->tx_on_vif &= ~(1 << if_idx);

		if (status) {
			if (status == -ECANCELED)
				/* a packet was flushed  */
				flushing[if_idx] = true;

			vif->net_stats.tx_errors++;

			if (status == -ETXTBSY)
				ath6kl_dbg(ATH6KL_DBG_WLAN_TX,
					"wmi/tx deepsleep syspend\n");
			else if (status != -ENOSPC &&
				status != -ECANCELED &&
				status != -ENOMEM)
				ath6kl_debug("tx complete error: %d\n", status);

			ath6kl_dbg(ATH6KL_DBG_WLAN_TX,
				   "%s: skb=0x%p data=0x%p len=0x%x eid=%d %s\n",
				   __func__, skb, packet->buf, packet->act_len,
				   eid, "error!");
		} else {
			ath6kl_dbg(ATH6KL_DBG_WLAN_TX,
				   "%s: skb=0x%p data=0x%p len=0x%x eid=%d %s\n",
				   __func__, skb, packet->buf, packet->act_len,
				   eid, "OK");

			flushing[if_idx] = false;
			vif->net_stats.tx_packets++;
			vif->net_stats.tx_bytes += skb->len;
		}

		ath6kl_tx_clear_node_map(vif, eid, map_no);
		if (ath6kl_cookie->htc_pkt->info.tx.tag ==
			ATH6KL_DATA_PKT_TAG)
			vif->data_cookie_count--;

		ath6kl_free_cookie(ar, ath6kl_cookie);

		if (test_bit(NETQ_STOPPED, &vif->flags))
			clear_bit(NETQ_STOPPED, &vif->flags);
	}

	spin_unlock_bh(&ar->lock);

	__skb_queue_purge(&skb_queue);

	if (test_bit(MCC_ENABLED, &ar->flag)) {
		endpoint = &ar->htc_target->endpoint[eid];
		/*if (ar && endpoint && packet && ar->htc_target) {*/
		if (endpoint && packet && ar->htc_target) {
			struct list_head *tx_queue;

			tx_queue = &endpoint->txq;
			if (tx_queue && vif && !flushing[vif->fw_vif_idx]) {
				spin_lock_bh(&ar->htc_target->tx_lock);
				txq_depth = get_queue_depth(tx_queue);
				spin_unlock_bh(&ar->htc_target->tx_lock);

				if (txq_depth < ATH6KL_P2P_FLOWCTRL_REQ_STEP)
					ath6kl_p2p_flowctrl_netif_transition(
						ar,
						ATH6KL_P2P_FLOWCTRL_NETIF_WAKE);
			}
		}
	} else {
		/* FIXME: Locking */
		spin_lock_bh(&ar->list_lock);
		list_for_each_entry(vif, &ar->vif_list, list) {
			if ((test_bit(CONNECTED, &vif->flags) ||
				test_bit(TESTMODE_EPPING, &ar->flag)) &&
				!flushing[vif->fw_vif_idx]) {
				spin_unlock_bh(&ar->list_lock);
				netif_wake_queue(vif->ndev);
				spin_lock_bh(&ar->list_lock);
			}
		}
		spin_unlock_bh(&ar->list_lock);
	}

	if (wake_event)
		wake_up(&ar->event_wq);

	return;

fatal:
	WARN_ON(1);
	spin_unlock_bh(&ar->lock);
	return;
}

static void ath6kl_flush_data_in_ep_by_if(struct ath6kl_vif *vif)
{
	struct ath6kl *ar = vif->ar;
	struct htc_packet *packet, *tmp_pkt;
	struct htc_endpoint *endpoint;
	struct list_head    *tx_queue, container;
	int eid;

	INIT_LIST_HEAD(&container);

	spin_lock_bh(&ar->htc_target->tx_lock);
	for (eid = ENDPOINT_2; eid <= ENDPOINT_5; eid++) {

		endpoint = &ar->htc_target->endpoint[eid];
		tx_queue = &endpoint->txq;

		if (list_empty(tx_queue))
			continue;

		list_for_each_entry_safe(packet,
					tmp_pkt,
					tx_queue,
					list) {
			if (packet->vif != vif)
				continue;
			list_del(&packet->list);
			packet->status = 0;
			list_add_tail(
				&packet->list,
				&container);
		}
	}
	spin_unlock_bh(&ar->htc_target->tx_lock);

	ath6kl_tx_complete(ar->htc_target, &container);

	return;
}

void ath6kl_tx_data_cleanup_by_if(struct ath6kl_vif *vif)
{
	ath6kl_flush_data_in_ep_by_if(vif);
	ath6kl_p2p_flowctrl_conn_list_cleanup_by_if(vif);
}

void ath6kl_tx_data_cleanup(struct ath6kl *ar)
{
	int i;

	/* flush all the data (non-control) streams */
	for (i = 0; i < WMM_NUM_AC; i++)
		ath6kl_htc_flush_txep(ar->htc_target, ar->ac2ep_map[i],
				      ATH6KL_DATA_PKT_TAG);

	ath6kl_p2p_flowctrl_conn_list_cleanup(ar);
}

/* Rx functions */
#ifdef CONFIG_ANDROID
static void ath6kl_eapol_send(struct work_struct *work)
{
	struct ath6kl_vif *vif = NULL;

	if (!work)
		goto FAILED;

	vif = container_of(work, struct ath6kl_vif,
		work_eapol_send.work);

	if (!vif)
		goto FAILED;

	spin_lock_bh(&vif->pend_skb_lock);

	if (!vif->pend_skb) {
		clear_bit(FIRST_EAPOL_PENDSENT, &vif->flags);
		spin_unlock_bh(&vif->pend_skb_lock);
		goto FAILED;
	}

	if (!(vif->pend_skb->dev->flags & IFF_UP)) {
		dev_kfree_skb(vif->pend_skb);
		vif->pend_skb = NULL;
		clear_bit(FIRST_EAPOL_PENDSENT, &vif->flags);
		spin_unlock_bh(&vif->pend_skb_lock);
		return;
	}

	netif_rx_ni(vif->pend_skb);

	vif->pend_skb = NULL;

	clear_bit(FIRST_EAPOL_PENDSENT, &vif->flags);

	spin_unlock_bh(&vif->pend_skb_lock);

	return;
FAILED:
	clear_bit(FIRST_EAPOL_PENDSENT, &vif->flags);
	ath6kl_err("%s failed\n", __func__);
	return;
}

#endif

static void ath6kl_deliver_frames_to_nw_stack(struct net_device *dev,
					      struct sk_buff *skb)
{
#define ETHERTYPE_IP    0x0800			/* IP  protocol */
#define IP_PROTO_UDP    0x11			/* UDP protocol */
	struct ath6kl_vif *vif = netdev_priv(dev);

	if (!skb)
		return;

	skb->dev = dev;

	if (!(skb->dev->flags & IFF_UP)) {
		dev_kfree_skb(skb);
		return;
	}

	/* Handle de-aggregated IntraBss's AMSDU frame here. */
	if (vif->nw_type == AP_NETWORK) {
		struct ethhdr *datap = (struct ethhdr *) skb->data;

		if (ath6kl_find_sta(vif, datap->h_dest)) {
			if (vif->intra_bss)
				ath6kl_data_tx(skb, dev, true);
			else
				dev_kfree_skb(skb);

			return;
		}
	}

	skb->protocol = eth_type_trans(skb, skb->dev);

	if (skb->protocol == cpu_to_be16(ETH_P_PAE)) {
#ifdef CONFIG_ANDROID
		struct ath6kl *ar = vif->ar;

		if (test_bit(CONNECT_HANDSHAKE_PROTECT, &vif->flags) &&
			(ar->wiphy->flags & WIPHY_FLAG_SUPPORTS_FW_ROAM)) {
			if (vif->pend_skb != NULL)
				ath6kl_flush_pend_skb(vif);

			if (test_bit(FIRST_EAPOL_PENDSENT, &vif->flags)) {
				vif->pend_skb = skb;
				INIT_DELAYED_WORK(&vif->work_eapol_send,
					ath6kl_eapol_send);
				schedule_delayed_work(&vif->work_eapol_send,
					ATH6KL_EAPOL_DELAY_REPORT_IN_HANDSHAKE);
				return;
			}
		} else
#endif
		if (test_bit(CONNECTED, &vif->flags))
			ath6kl_eapol_handshake_protect(vif, false);
	}

/*
#if ((LINUX_VERSION_CODE >= KERNEL_VERSION(3, 2, 0)) &&	\
	(LINUX_VERSION_CODE < KERNEL_VERSION(3, 3, 0)))
*/
#ifdef CONFIG_ATH6KL_UDP_TPUT_WAR
	if (skb->protocol == htons(ETHERTYPE_IP)) {
		struct ethhdr *eth = eth_hdr(skb);
		struct iphdr *ip_hdr =
			(struct iphdr *)((u8 *)eth + sizeof(struct ethhdr));

		if (ip_hdr->protocol == IP_PROTO_UDP) {
			struct sk_buff *new_skb;

			new_skb = dev_alloc_skb(skb->len+ETH_HLEN);

			/* If we can't allocate a new skb,
			 * just indicate the original skb.
			 */
			if (new_skb == NULL) {
				netif_rx_ni(skb);
			} else {
				skb_put(new_skb, skb->len+ETH_HLEN);
				memcpy(new_skb->data, eth, skb->len+ETH_HLEN);

				new_skb->dev = dev;
				new_skb->protocol =
					eth_type_trans(new_skb, new_skb->dev);
				dev_kfree_skb(skb);
				netif_rx_ni(new_skb);
			}

			return;
		}
	}
#endif

	netif_rx_ni(skb);
}

static void ath6kl_alloc_netbufs(struct sk_buff_head *q, u16 num)
{
	struct sk_buff *skb;

	while (num) {
		skb = ath6kl_buf_alloc(ATH6KL_BUFFER_SIZE);
		if (!skb) {
			ath6kl_err("netbuf allocation failed\n");
			return;
		}
		skb_queue_tail(q, skb);
		num--;
	}
}

static struct sk_buff *aggr_get_free_skb(struct aggr_conn_info *aggr_conn)
{
	struct aggr_info *aggr = aggr_conn->aggr_cntxt;
	struct sk_buff *skb = NULL;

	WARN_ON(!aggr);
	if (skb_queue_len(&aggr->free_q) < (AGGR_NUM_OF_FREE_NETBUFS >> 2))
		ath6kl_alloc_netbufs(&aggr->free_q, AGGR_NUM_OF_FREE_NETBUFS);

	skb = skb_dequeue(&aggr->free_q);

	return skb;
}

void ath6kl_rx_refill(struct htc_target *target, enum htc_endpoint_id endpoint)
{
	struct ath6kl *ar = target->dev->ar;
	struct sk_buff *skb;
	int rx_buf;
	int n_buf_refill;
	struct htc_packet *packet;
	struct list_head queue;

	n_buf_refill = ATH6KL_MAX_RX_BUFFERS -
			  ath6kl_htc_get_rxbuf_num(ar->htc_target, endpoint);

	if (n_buf_refill <= 0)
		return;

	INIT_LIST_HEAD(&queue);

	ath6kl_dbg(ATH6KL_DBG_WLAN_RX,
		   "%s: providing htc with %d buffers at eid=%d\n",
		   __func__, n_buf_refill, endpoint);

	for (rx_buf = 0; rx_buf < n_buf_refill; rx_buf++) {
		skb = ath6kl_buf_alloc(ATH6KL_BUFFER_SIZE);
		if (!skb)
			break;

		packet = (struct htc_packet *) skb->head;
		if (!IS_ALIGNED((unsigned long) skb->data, 4)) {
			size_t len = skb_headlen(skb);
			skb->data = PTR_ALIGN(skb->data - 4, 4);
			skb_set_tail_pointer(skb, len);
		}
		set_htc_rxpkt_info(packet, skb, skb->data,
				ATH6KL_BUFFER_SIZE, endpoint);
		packet->skb = skb;

		list_add_tail(&packet->list, &queue);
	}

	if (!list_empty(&queue))
		ath6kl_htc_add_rxbuf_multiple(ar->htc_target, &queue);
}

void ath6kl_refill_amsdu_rxbufs(struct ath6kl *ar, int count)
{
	struct htc_packet *packet;
	struct sk_buff *skb;

	while (count) {
		skb = ath6kl_buf_alloc(ATH6KL_AMSDU_BUFFER_SIZE);
		if (!skb)
			return;

		packet = (struct htc_packet *) skb->head;
		if (!IS_ALIGNED((unsigned long) skb->data, 4)) {
			size_t len = skb_headlen(skb);
			skb->data = PTR_ALIGN(skb->data - 4, 4);
			skb_set_tail_pointer(skb, len);
		}
		set_htc_rxpkt_info(packet, skb, skb->data,
				   ATH6KL_AMSDU_BUFFER_SIZE, 0);
		packet->skb = skb;

		spin_lock_bh(&ar->lock);
		list_add_tail(&packet->list, &ar->amsdu_rx_buffer_queue);
		spin_unlock_bh(&ar->lock);
		count--;
	}
}

/*
 * Callback to allocate a receive buffer for a pending packet. We use a
 * pre-allocated list of buffers of maximum AMSDU size (4K).
 */
struct htc_packet *ath6kl_alloc_amsdu_rxbuf(struct htc_target *target,
					    enum htc_endpoint_id endpoint,
					    int len)
{
	struct ath6kl *ar = target->dev->ar;
	struct htc_packet *packet = NULL;
	struct list_head *pkt_pos;
	int refill_cnt = 0, depth = 0;

	ath6kl_dbg(ATH6KL_DBG_WLAN_RX, "%s: eid=%d, len:%d\n",
		   __func__, endpoint, len);

	if ((len <= ATH6KL_BUFFER_SIZE) ||
	    (len > ATH6KL_AMSDU_BUFFER_SIZE))
		return NULL;

	spin_lock_bh(&ar->lock);

	if (list_empty(&ar->amsdu_rx_buffer_queue)) {
		spin_unlock_bh(&ar->lock);
		refill_cnt = ATH6KL_MAX_AMSDU_RX_BUFFERS;
		goto refill_buf;
	}

	packet = list_first_entry(&ar->amsdu_rx_buffer_queue,
				  struct htc_packet, list);
	list_del(&packet->list);
	list_for_each(pkt_pos, &ar->amsdu_rx_buffer_queue)
		depth++;

	refill_cnt = ATH6KL_MAX_AMSDU_RX_BUFFERS - depth;
	spin_unlock_bh(&ar->lock);

	/* set actual endpoint ID */
	packet->endpoint = endpoint;

refill_buf:
	if (refill_cnt >= ATH6KL_AMSDU_REFILL_THRESHOLD)
		ath6kl_refill_amsdu_rxbufs(ar, refill_cnt);

	return packet;
}

static void aggr_slice_amsdu(struct aggr_conn_info *aggr_conn,
			     struct rxtid *rxtid, struct sk_buff *skb)
{
	struct sk_buff *new_skb;
	struct ethhdr *hdr;
	u16 frame_8023_len, payload_8023_len, mac_hdr_len, amsdu_len;
	u8 *framep;

	mac_hdr_len = sizeof(struct ethhdr);
	framep = skb->data + mac_hdr_len;
	amsdu_len = skb->len - mac_hdr_len;

	while (amsdu_len > mac_hdr_len) {
		hdr = (struct ethhdr *) framep;
		payload_8023_len = ntohs(hdr->h_proto);

		if (payload_8023_len < MIN_MSDU_SUBFRAME_PAYLOAD_LEN ||
		    payload_8023_len > MAX_MSDU_SUBFRAME_PAYLOAD_LEN) {
			ath6kl_err("802.3 AMSDU frame bound check failed. len %d\n",
				   payload_8023_len);
			break;
		}

		frame_8023_len = payload_8023_len + mac_hdr_len;
		new_skb = aggr_get_free_skb(aggr_conn);
		if (!new_skb) {
			ath6kl_err("no buffer available\n");
			break;
		}

		memcpy(new_skb->data, framep, frame_8023_len);
		skb_put(new_skb, frame_8023_len);
		if (ath6kl_wmi_dot3_2_dix(new_skb)) {
			ath6kl_err("dot3_2_dix error\n");
			dev_kfree_skb(new_skb);
			break;
		}

		skb_queue_tail(&rxtid->q, new_skb);

		/* Is this the last subframe within this aggregate ? */
		if ((amsdu_len - frame_8023_len) == 0)
			break;

		/* Add the length of A-MSDU subframe padding bytes -
		 * Round to nearest word.
		 */
		frame_8023_len = ALIGN(frame_8023_len, 4);

		framep += frame_8023_len;
		amsdu_len -= frame_8023_len;
	}

	dev_kfree_skb(skb);
}

static void aggr_deque_frms(struct aggr_conn_info *aggr_conn, u8 tid,
			    u16 seq_no, u8 order)
{
	struct sk_buff *skb;
	struct rxtid *rxtid;
	struct skb_hold_q *node;
	u16 idx, idx_end, seq_end;
	struct rxtid_stats *stats;
	struct net_device *dev;

	if (!aggr_conn)
		return;

	rxtid = AGGR_GET_RXTID(aggr_conn, tid);
	stats = AGGR_GET_RXTID_STATS(aggr_conn, tid);

	idx = AGGR_WIN_IDX(rxtid->seq_next, rxtid->hold_q_sz);

	/*
	 * idx_end is typically the last possible frame in the window,
	 * but changes to 'the' seq_no, when BAR comes. If seq_no
	 * is non-zero, we will go up to that and stop.
	 * Note: last seq no in current window will occupy the same
	 * index position as index that is just previous to start.
	 * An imp point : if win_sz is 7, for seq_no space of 4095,
	 * then, there would be holes when sequence wrap around occurs.
	 * Target should judiciously choose the win_sz, based on
	 * this condition. For 4095, (TID_WINDOW_SZ = 2 x win_sz
	 * 2, 4, 8, 16 win_sz works fine).
	 * We must deque from "idx" to "idx_end", including both.
	 */
	seq_end = seq_no ? seq_no : rxtid->seq_next;
	idx_end = AGGR_WIN_IDX(seq_end, rxtid->hold_q_sz);

	spin_lock_bh(&rxtid->lock);

	do {
		node = &rxtid->hold_q[idx];
		if ((order == 1) && (!node->skb))
			break;

		if (node->skb) {
			if (node->is_amsdu)
				aggr_slice_amsdu(aggr_conn, rxtid, node->skb);
			else
				skb_queue_tail(&rxtid->q, node->skb);
			node->skb = NULL;
		} else
			stats->num_hole++;

		rxtid->seq_next = ATH6KL_NEXT_SEQ_NO(rxtid->seq_next);
		idx = AGGR_WIN_IDX(rxtid->seq_next, rxtid->hold_q_sz);
	} while (idx != idx_end);

	stats->num_delivered += skb_queue_len(&rxtid->q);

	WARN_ON(!aggr_conn->dev);
	dev = aggr_conn->dev;
	while ((skb = skb_dequeue(&rxtid->q)))
		ath6kl_deliver_frames_to_nw_stack(dev, skb);
	spin_unlock_bh(&rxtid->lock);
}

static bool aggr_process_recv_frm(struct ath6kl *ar,
				  struct aggr_conn_info *aggr_conn,
				  u8 tid, u16 seq_no,
				  bool is_amsdu, struct sk_buff *frame)
{
	struct rxtid *rxtid;
	struct rxtid_stats *stats;
	struct sk_buff *skb;
	struct skb_hold_q *node;
	u16 idx, st, cur, end;
	bool is_queued = false;
	u16 extended_end;
	bool drop_it = false;

	rxtid = AGGR_GET_RXTID(aggr_conn, tid);
	stats = AGGR_GET_RXTID_STATS(aggr_conn, tid);

	stats->num_into_aggr++;

	if (!rxtid->aggr) {
		if (is_amsdu) {
			struct net_device *dev;

			aggr_slice_amsdu(aggr_conn, rxtid, frame);
			is_queued = true;
			stats->num_amsdu++;

			WARN_ON(!aggr_conn->dev);
			dev = aggr_conn->dev;
			while ((skb = skb_dequeue(&rxtid->q)))
				ath6kl_deliver_frames_to_nw_stack(dev,
								  skb);
		}
		return is_queued;
	}

	spin_lock_bh(&rxtid->lock);
	if (rxtid->sync_next_seq == true) {
		rxtid->seq_next = seq_no;
		rxtid->sync_next_seq = false;
	}

	/* Check the incoming sequence no, if it's in the window */
	st = rxtid->seq_next;
	cur = seq_no;
	end = (st + rxtid->hold_q_sz-1) & ATH6KL_MAX_SEQ_NO;
	if (((st < end) && (cur < st || cur > end)) ||
		((st > end) && (cur > end) && (cur < st))) {
		extended_end = (end + rxtid->hold_q_sz) &
			ATH6KL_MAX_SEQ_NO;

		if (((end < extended_end) &&
			(cur < end || cur > extended_end)) ||
			((end > extended_end) && (cur > extended_end) &&
			(cur < end))) {
			u16	range_val = ((cur-st) & ATH6KL_MAX_SEQ_NO);
			ath6kl_dbg(ATH6KL_DBG_AGGR,
			"%s[%d] range_val=%d(%d),st=%d,cur=%d,tid=%d\n",
			__func__, __LINE__, range_val, (rxtid->hold_q_sz << 1),
			st, cur, tid);

			if ((range_val >= (rxtid->hold_q_sz << 1)) &&
			(range_val <=
			(ATH6KL_MAX_SEQ_NO-(rxtid->hold_q_sz << 1)+1))) {

				ath6kl_dbg(ATH6KL_DBG_AGGR, "%s[%d] chase seq\n",
					__func__, __LINE__);

				spin_unlock_bh(&rxtid->lock);
				aggr_deque_frms(aggr_conn, tid, 0, 0);
				spin_lock_bh(&rxtid->lock);

				rxtid->seq_next =
					(cur - rxtid->hold_q_sz) &
					ATH6KL_MAX_SEQ_NO;

			} else {
				ath6kl_dbg(ATH6KL_DBG_AGGR, "%s[%d] old seq\n",
					__func__, __LINE__);
			}
			drop_it = true;
		} else {
			/*
			 * Dequeue only those frames that are outside the
			 * new shifted window.
			 */
			st = (cur - (rxtid->hold_q_sz-1)) & ATH6KL_MAX_SEQ_NO;
			spin_unlock_bh(&rxtid->lock);
			aggr_deque_frms(aggr_conn, tid, st, 0);
			spin_lock_bh(&rxtid->lock);
		}
		stats->num_oow++;
	}

	if ((drop_it == true) &&
			!(ar->conf_flags & ATH6KL_CONF_DISABLE_RX_AGGR_DROP)) {
		dev_kfree_skb(frame);
		is_queued = true;
		spin_unlock_bh(&rxtid->lock);
		return is_queued;
	}

	idx = AGGR_WIN_IDX(seq_no, rxtid->hold_q_sz);

	node = &rxtid->hold_q[idx];

	/*
	 * Is the cur frame duplicate or something beyond our window(hold_q
	 * -> which is 2x, already)?
	 *
	 * 1. Duplicate is easy - drop incoming frame.
	 * 2. Not falling in current sliding window.
	 *  2a. is the frame_seq_no preceding current tid_seq_no?
	 *      -> drop the frame. perhaps sender did not get our ACK.
	 *         this is taken care of above.
	 *  2b. is the frame_seq_no beyond window(st, TID_WINDOW_SZ);
	 *      -> Taken care of it above, by moving window forward.
	 */
	dev_kfree_skb(node->skb);
	stats->num_dups++;

	node->skb = frame;
	is_queued = true;
	node->is_amsdu = is_amsdu;
	node->seq_no = seq_no;

	if (node->is_amsdu)
		stats->num_amsdu++;
	else
		stats->num_mpdu++;

	spin_unlock_bh(&rxtid->lock);
	aggr_deque_frms(aggr_conn, tid, 0, 1);
	spin_lock_bh(&rxtid->lock);

	if (rxtid->tid_timer_scheduled &&
		rxtid->timerwait_seq_num != rxtid->seq_next) {
		del_timer(&rxtid->tid_timer);
		rxtid->tid_timer_scheduled = false;
		rxtid->continuous_count = 0;
	}

	if (!rxtid->tid_timer_scheduled) {
		for (idx = 0 ; idx < rxtid->hold_q_sz; idx++) {
			if (rxtid->hold_q[idx].skb) {
				rxtid->issue_timer_seq =
					rxtid->hold_q[idx].seq_no;
				/*
				 * There is a frame in the queue and no
				 * timer so start a timer to ensure that
				 * the frame doesn't remain stuck
				 * forever.
				 */
				rxtid->tid_timer_scheduled = true;
				rxtid->timerwait_seq_num = rxtid->seq_next;
				rxtid->continuous_count++;
				mod_timer(&rxtid->tid_timer,
					(jiffies +
					msecs_to_jiffies(
					aggr_conn->tid_timeout_setting[tid])));
				break;
			}
		}
	}
	spin_unlock_bh(&rxtid->lock);

	return is_queued;
}

void ath6kl_uapsd_trigger_frame_rx(struct ath6kl_vif *vif,
				   struct ath6kl_sta *conn)
{
	bool is_psq_empty;
	bool is_psq_empty_at_start;
	u32 num_frames_to_deliver;
	struct ath6kl_ps_buf_desc *ps_buf;

	/* If the APSD q for this STA is not empty, dequeue and send a pkt from
	 * the head of the q. Also update the More data bit in the WMI_DATA_HDR
	 * if there are more pkts for this STA in the APSD q. If there are
	 * no more pkts for this STA, update the APSD bitmap for this STA.
	 */

	num_frames_to_deliver = (conn->apsd_info >> 4) & 0xF;

	/* Number of frames to send in a service period is indicated by
	 * the station in the QOS_INFO of the association request
	 * If it is zero, send all frames
	 */
	if (!num_frames_to_deliver)
		num_frames_to_deliver = 0xFFFF;


	spin_lock_bh(&conn->lock);
	ath6kl_dbg(ATH6KL_DBG_POWERSAVE,
		"%s: TriggerRx aid %d sta_flags %x apsd_info %d psq_data %d psq_mgmt %d\n",
		__func__, conn->aid, conn->sta_flags, conn->apsd_info,
		!ath6kl_ps_queue_empty(&conn->psq_data),
		!ath6kl_ps_queue_empty(&conn->psq_mgmt));

	is_psq_empty = ath6kl_ps_queue_empty(&conn->psq_data) &&
			ath6kl_ps_queue_empty(&conn->psq_mgmt);
	spin_unlock_bh(&conn->lock);

	is_psq_empty_at_start = is_psq_empty;
	while ((!is_psq_empty) && (num_frames_to_deliver)) {
		spin_lock_bh(&conn->lock);
		if (!ath6kl_ps_queue_empty(&conn->psq_mgmt)) {
			struct ieee80211_mgmt *mgmt;

			ps_buf = ath6kl_ps_queue_dequeue(&conn->psq_mgmt);
			is_psq_empty = ath6kl_ps_queue_empty(&conn->psq_data) &&
					ath6kl_ps_queue_empty(&conn->psq_mgmt);
			spin_unlock_bh(&conn->lock);

			/* Set the STA flag to Trigger delivery,
			 * so that the frame will go out
			 */
			conn->sta_flags |= STA_PS_APSD_TRIGGER;
			num_frames_to_deliver--;

			/* Last frame in the service period,
			 * set EOSP or queue empty
			 */
			if ((is_psq_empty) ||
				(!num_frames_to_deliver))
				conn->sta_flags |= STA_PS_APSD_EOSP;

			mgmt = (struct ieee80211_mgmt *) ps_buf->buf;
			if (ps_buf->buf + ps_buf->len >=
				mgmt->u.probe_resp.variable &&
			    ieee80211_is_probe_resp(mgmt->frame_control))
				ath6kl_wmi_send_go_probe_response_cmd(
								vif->ar->wmi,
								vif,
								ps_buf->buf,
								ps_buf->len,
								ps_buf->freq);
			else
				ath6kl_wmi_send_action_cmd(vif->ar->wmi,
							vif->fw_vif_idx,
							ps_buf->id,
							ps_buf->freq,
							ps_buf->wait,
							ps_buf->buf,
							ps_buf->len);

			kfree(ps_buf);
			conn->sta_flags &= ~(STA_PS_APSD_TRIGGER);
			conn->sta_flags &= ~(STA_PS_APSD_EOSP);
		} else {
			ps_buf = ath6kl_ps_queue_dequeue(&conn->psq_data);

			is_psq_empty = ath6kl_ps_queue_empty(&conn->psq_data) &&
					ath6kl_ps_queue_empty(&conn->psq_mgmt);
			spin_unlock_bh(&conn->lock);

			if (ps_buf) {
				/* Set the STA flag to Trigger delivery,
				 * so that the frame will go out
				 */
				conn->sta_flags |= STA_PS_APSD_TRIGGER;
				num_frames_to_deliver--;

				/* Last frame in the service period,
				 * set EOSP or queue empty
				 */
				if ((is_psq_empty) ||
					(!num_frames_to_deliver))
					conn->sta_flags |= STA_PS_APSD_EOSP;

				WARN_ON(!ps_buf->skb);
				ath6kl_data_tx(ps_buf->skb, vif->ndev, true);
				kfree(ps_buf);
				conn->sta_flags &= ~(STA_PS_APSD_TRIGGER);
				conn->sta_flags &= ~(STA_PS_APSD_EOSP);
			}
		}
	}

	if (is_psq_empty) {
		ath6kl_wmi_set_pvb_cmd(vif->ar->wmi,
			vif->fw_vif_idx, conn->aid, 0);

		if (is_psq_empty_at_start)
			ath6kl_wmi_set_apsd_buffered_traffic_cmd(vif->ar->wmi,
				vif->fw_vif_idx, conn->aid, 0,
				WMI_AP_APSD_NO_DELIVERY_FRAMES_FOR_THIS_TRIGGER
				);
		else
			ath6kl_wmi_set_apsd_buffered_traffic_cmd(vif->ar->wmi,
				vif->fw_vif_idx, conn->aid, 0,
							0);
	}

	return;
}

static inline struct ath6kl_sta *_powersave_ap_rx(struct ath6kl_vif *vif,
	struct sk_buff *skb, int len,
	bool ps_state, bool trigger_state)
{
	struct ath6kl *ar = vif->ar;
	struct ath6kl_sta *conn;
	struct ethhdr *datap = NULL;
	bool prev_ps;
	int min_hdr_len;

	datap = (struct ethhdr *) (skb->data);
	conn = ath6kl_find_sta(vif, datap->h_source);
	if (!conn) {
		dev_kfree_skb(skb);
		return NULL;
	}

	/*
	* If there is a change in PS state of the STA, take appropriate steps:
	*
	* 1. If Sleep-->Awake, flush the psq for the STA and clear the PVB.
	* 2. If Awake-->Sleep, Starting queueing frames the STA.
	*/
	prev_ps = !!(conn->sta_flags & STA_PS_SLEEP);

	ath6kl_dbg(ATH6KL_DBG_POWERSAVE,
			"%s: aid %d sta_flags %x prev_ps %d"
			" ps_state %d is_trigger %d [%d]\n",
			__func__,
			conn->aid,
			conn->sta_flags,
			prev_ps,
			ps_state,
			trigger_state,
			len);

	if (ps_state) {
		conn->sta_flags |= STA_PS_SLEEP;
		if (!prev_ps) {
			aggr_tx_flush(vif , conn);
			ath6kl_ps_queue_age_start(conn);
		}
	} else {
		conn->sta_flags &= ~STA_PS_SLEEP;
		if (prev_ps)
			ath6kl_ps_queue_age_stop(conn);
	}

	if (conn->sta_flags & STA_PS_SLEEP) {
		/* Accept trigger only when the station is in sleep */
		if (trigger_state)
			ath6kl_uapsd_trigger_frame_rx(vif, conn);
	}

	if (prev_ps ^ !!(conn->sta_flags & STA_PS_SLEEP)) {
		if (!(conn->sta_flags & STA_PS_SLEEP)) {
			struct ath6kl_ps_buf_desc *ps_buf;
			bool is_psq_empty_at_start;
			struct ieee80211_mgmt *mgmt;

			spin_lock_bh(&conn->lock);
			ath6kl_dbg(ATH6KL_DBG_POWERSAVE,
				"%s: psq_data %d psq_mgmt %d\n",
				__func__,
				!ath6kl_ps_queue_empty(&conn->psq_data),
				!ath6kl_ps_queue_empty(&conn->psq_mgmt));

			is_psq_empty_at_start =
				ath6kl_ps_queue_empty(&conn->psq_data) &&
				ath6kl_ps_queue_empty(&conn->psq_mgmt);

			while ((ps_buf = ath6kl_ps_queue_dequeue(
						&conn->psq_mgmt)) != NULL) {
				spin_unlock_bh(&conn->lock);

				mgmt = (struct ieee80211_mgmt *) ps_buf->buf;

				if ((ps_buf->buf + ps_buf->len >=
					mgmt->u.probe_resp.variable) &&
				    ieee80211_is_probe_resp(
					mgmt->frame_control))
					ath6kl_wmi_send_go_probe_response_cmd(
								ar->wmi,
								vif,
								ps_buf->buf,
								ps_buf->len,
								ps_buf->freq);
				else
					ath6kl_wmi_send_action_cmd(
								ar->wmi,
								vif->fw_vif_idx,
								ps_buf->id,
								ps_buf->freq,
								ps_buf->wait,
								ps_buf->buf,
								ps_buf->len);
				kfree(ps_buf);
				spin_lock_bh(&conn->lock);
			}

			while ((ps_buf = ath6kl_ps_queue_dequeue(
						&conn->psq_data)) != NULL) {
				spin_unlock_bh(&conn->lock);

				WARN_ON(!ps_buf->skb);
				ath6kl_data_tx(ps_buf->skb,
					       vif->ndev, true);
				kfree(ps_buf);
				spin_lock_bh(&conn->lock);
			}

			spin_unlock_bh(&conn->lock);

			if (!is_psq_empty_at_start)
				ath6kl_wmi_set_apsd_buffered_traffic_cmd(
								ar->wmi,
								vif->fw_vif_idx,
								conn->aid,
								0,
								0);

			/* Clear the PVB for this STA */
			ath6kl_wmi_set_pvb_cmd(ar->wmi,
						vif->fw_vif_idx,
						conn->aid,
						0);
		}
	}

	min_hdr_len = sizeof(struct ethhdr) +
		      sizeof(struct wmi_data_hdr) +
		      sizeof(struct ath6kl_llc_snap_hdr);

	/* drop NULL data frames here */
	if ((len < min_hdr_len) ||
		(len > WMI_MAX_AMSDU_RX_DATA_FRAME_LENGTH)) {
		dev_kfree_skb(skb);
		return NULL;
	}

	return conn;
}

void ath6kl_rx(struct htc_target *target, struct htc_packet *packet)
{
	struct ath6kl *ar = target->dev->ar;
	struct sk_buff *skb = packet->pkt_cntxt;
	struct wmi_rx_meta_v2 *meta;
	struct wmi_data_hdr *dhdr;
	int min_hdr_len;
	u8 meta_type, dot11_hdr = 0;
	u8 pad_before_data_start;
	int status = packet->status;
	enum htc_endpoint_id ept = packet->endpoint;
	bool is_amsdu;
	struct ath6kl_sta *conn = NULL;
	struct sk_buff *skb1 = NULL;
	struct ethhdr *datap = NULL;
	struct ath6kl_vif *vif;
	u16 seq_no;
	u8 tid, if_idx;

	ath6kl_dbg(ATH6KL_DBG_WLAN_RX,
		   "%s: ar=0x%p eid=%d, skb=0x%p, data=0x%p, len=0x%x status:%d",
		   __func__, ar, ept, skb, packet->buf,
		   packet->act_len, status);

	if (status || !(skb->data + HTC_HDR_LENGTH)) {
		dev_kfree_skb(skb);
		return;
	}

	skb_put(skb, packet->act_len + HTC_HDR_LENGTH);
	skb_pull(skb, HTC_HDR_LENGTH);

	if (!test_bit(TESTMODE_EPPING, &ar->flag)) {
		if (ept == ar->ctrl_ep) {
			if_idx =
			wmi_cmd_hdr_get_if_idx(
				(struct wmi_cmd_hdr *) skb->data);
		} else {
			if_idx =
			wmi_data_hdr_get_if_idx(
				(struct wmi_data_hdr *) skb->data);
		}
	} else {
		/* The epping packet is not coming from wmi, skip the index
		 * retrival, epping assume using the first if_idx anyway
		 */
		if_idx = 0;
	}

	vif = ath6kl_get_vif_by_index(ar, if_idx);
	if (!vif) {
		dev_kfree_skb(skb);
		return;
	}

	/*
	 * Take lock to protect buffer counts and adaptive power throughput
	 * state.
	 */
	vif->net_stats.rx_packets++;
	vif->net_stats.rx_bytes += packet->act_len;

	ath6kl_dbg_dump(ATH6KL_DBG_RAW_BYTES, __func__, "rx ",
			skb->data, skb->len);

	skb->dev = vif->ndev;

	if (!test_bit(WMI_ENABLED, &ar->flag)) {
		if (EPPING_ALIGNMENT_PAD > 0)
			skb_pull(skb, EPPING_ALIGNMENT_PAD);
		ath6kl_deliver_frames_to_nw_stack(vif->ndev, skb);
		return;
	}

	ath6kl_check_wow_status(ar);

	if (ept == ar->ctrl_ep) {
		ath6kl_wmi_control_rx(ar->wmi, skb);
		return;
	}

	min_hdr_len = sizeof(struct ethhdr) + sizeof(struct wmi_data_hdr) +
		      sizeof(struct ath6kl_llc_snap_hdr);

	dhdr = (struct wmi_data_hdr *) skb->data;

	is_amsdu = wmi_data_hdr_is_amsdu(dhdr) ? true : false;
	tid = wmi_data_hdr_get_up(dhdr);
	seq_no = wmi_data_hdr_get_seqno(dhdr);
	meta_type = wmi_data_hdr_get_meta(dhdr);
	dot11_hdr = wmi_data_hdr_get_dot11(dhdr);
	pad_before_data_start =
		(le16_to_cpu(dhdr->info3) >> WMI_DATA_HDR_PAD_BEFORE_DATA_SHIFT)
			& WMI_DATA_HDR_PAD_BEFORE_DATA_MASK;
	packet->act_len -= pad_before_data_start;

	/*
	 * In the case of AP mode we may receive NULL data frames
	 * that do not have LLC hdr. They are 16 bytes in size.
	 * Allow these frames in the AP mode.
	 */
	if (vif->nw_type != AP_NETWORK &&
	    ((packet->act_len < min_hdr_len) ||
	     (packet->act_len > WMI_MAX_AMSDU_RX_DATA_FRAME_LENGTH))) {
		ath6kl_info("frame len is too short or too long\n");
		vif->net_stats.rx_errors++;
		vif->net_stats.rx_length_errors++;
		dev_kfree_skb(skb);
		return;
	}

	skb_pull(skb, sizeof(struct wmi_data_hdr));

	switch (meta_type) {
	case WMI_META_VERSION_1:
		skb_pull(skb, sizeof(struct wmi_rx_meta_v1));
		break;
	case WMI_META_VERSION_2:
		meta = (struct wmi_rx_meta_v2 *) skb->data;
		meta->csum = le16_to_cpu(meta->csum);
		if (meta->csum_flags & 0x1) {
			skb->ip_summed = CHECKSUM_COMPLETE;
			skb->csum = (__force __wsum) meta->csum;
		}
		skb_pull(skb, sizeof(struct wmi_rx_meta_v2));
		break;
	default:
		break;
	}

	skb_pull(skb, pad_before_data_start);

	if (dot11_hdr)
		status = ath6kl_wmi_dot11_hdr_remove(ar->wmi, skb);
	else if (!is_amsdu)
		status = ath6kl_wmi_dot3_2_dix(skb);

	if (status) {
		/*
		 * Drop frames that could not be processed (lack of
		 * memory, etc.)
		 */
		dev_kfree_skb(skb);
		return;
	}

	/* Get the Power save state of the STA */
	if (vif->nw_type == AP_NETWORK) {
		bool ps_state, trigger_state;

		ps_state = !!((dhdr->info >> WMI_DATA_HDR_PS_SHIFT) &
				WMI_DATA_HDR_PS_MASK);
		trigger_state = WMI_DATA_HDR_IS_TRIGGER(dhdr);

		conn = _powersave_ap_rx(vif,
					skb, packet->act_len,
					ps_state, trigger_state);
		if (conn == NULL)
			return;
	}

	if (!(vif->ndev->flags & IFF_UP)) {
		dev_kfree_skb(skb);
		return;
	}

	if (vif->nw_type == AP_NETWORK) {
		datap = (struct ethhdr *) skb->data;
		if (is_multicast_ether_addr(datap->h_dest))
			/*
			 * Bcast/Mcast frames should be sent to the
			 * OS stack as well as on the air.
			 */
			skb1 = skb_copy(skb, GFP_ATOMIC);
		else {
			/*
			 * Search for a connected STA with dstMac
			 * as the Mac address. If found send the
			 * frame to it on the air else send the
			 * frame up the stack.
			 */
			struct ath6kl_sta *to_conn = NULL;

			if (is_amsdu)
				goto rx_aggr_process;

			to_conn = ath6kl_find_sta(vif, datap->h_dest);

			if (to_conn && vif->intra_bss) {
				skb1 = skb;
				skb = NULL;
			} else if (to_conn && !vif->intra_bss) {
				dev_kfree_skb(skb);
				skb = NULL;
			}
		}
		if (skb1)
			ath6kl_data_tx(skb1, vif->ndev, true);

		if (skb == NULL) {
			/* nothing to deliver up the stack */
			return;
		}
#ifdef ATHTST_SUPPORT
		/* record each connected sta rssi */
		if (conn->avg_data_rssi == 0) {
			if ((dhdr->rssi) >= RSSI_LPF_THRESHOLD)
				conn->avg_data_rssi = ATH_RSSI_IN(dhdr->rssi);
		} else {
			ATH_RSSI_LPF(conn->avg_data_rssi, dhdr->rssi);
		}
#endif
	}
#ifdef ATHTST_SUPPORT
	if (vif->nw_type != AP_NETWORK) {
		conn = &vif->sta_list[0];
		/* record each connected sta rssi */
		if (conn->avg_data_rssi == 0) {
			if ((dhdr->rssi) >= RSSI_LPF_THRESHOLD)
				conn->avg_data_rssi = ATH_RSSI_IN(dhdr->rssi);
		} else {
			ATH_RSSI_LPF(conn->avg_data_rssi, dhdr->rssi);
		}
	}
#endif
	if (vif->nw_type != AP_NETWORK)
		conn = &vif->sta_list[0];

rx_aggr_process:
	datap = (struct ethhdr *) skb->data;

	if ((is_unicast_ether_addr(datap->h_dest) ||
		(vif->nw_type == AP_NETWORK)) &&
		aggr_process_recv_frm(ar, conn->aggr_conn_cntxt, tid,
			seq_no, is_amsdu, skb))
		/* aggregation code will handle the skb */
		return;

	ath6kl_deliver_frames_to_nw_stack(vif->ndev, skb);
}

static void aggr_tx_progressive(struct txtid *txtid, bool tx_timeout)
{
	struct ath6kl_vif *vif = txtid->vif;
	struct aggr_info *aggr = vif->aggr_cntxt;
	unsigned long now = jiffies;

	/* Only support STA mode now */
	if (vif->nw_type != INFRA_NETWORK)
		return;

	txtid->last_num_amsdu++;
	if (tx_timeout)
		txtid->last_num_timeout++;

	/* Check it every AGGR_TX_PROG_CHECK_INTVAL */
	if (aggr->tx_amsdu_progressive &&
	    ((txtid->last_check_time == 0) ||
	     (now - txtid->last_check_time > AGGR_TX_PROG_CHECK_INTVAL))) {
		/*
		 * Change to high speed when mass of AMSDUs & most of it
		 * are not-timeout case.
		 * Back to normal speed when bit of AMSDUs & most of it
		 * are timeout case.
		 */
		if (!aggr->tx_amsdu_progressive_hispeed) {
			if ((txtid->last_num_amsdu > AGGR_TX_PROG_HS_THRESH) &&
			    (txtid->last_num_timeout <
			     (txtid->last_num_amsdu >>
						AGGR_TX_PROG_HS_FACTOR))) {
				aggr->tx_amsdu_progressive_hispeed = true;
				aggr_tx_config(vif,
						aggr->tx_amsdu_seq_pkt,
						true,
						AGGR_TX_PROG_HS_MAX_NUM,
						AGGR_TX_MAX_PDU_SIZE,
						AGGR_TX_PROG_HS_TIMEOUT);

				ath6kl_dbg(ATH6KL_DBG_WLAN_TX_AMSDU,
					"%s: AMSDU change to high speed %d %d\n",
					__func__,
					txtid->last_num_amsdu,
					txtid->last_num_timeout);
			}
		} else if (aggr->tx_amsdu_progressive_hispeed) {
			if ((txtid->last_num_amsdu < AGGR_TX_PROG_NS_THRESH) &&
			    (txtid->last_num_timeout >
			     (txtid->last_num_amsdu >>
						AGGR_TX_PROG_NS_FACTOR))) {
				aggr->tx_amsdu_progressive_hispeed = false;
				aggr_tx_config(vif,
						aggr->tx_amsdu_seq_pkt,
						true,
						AGGR_TX_MAX_NUM,
						AGGR_TX_MAX_PDU_SIZE,
						AGGR_TX_TIMEOUT);
				ath6kl_dbg(ATH6KL_DBG_WLAN_TX_AMSDU,
					"%s: AMSDU back to normal speed %d %d\n",
					__func__,
					txtid->last_num_amsdu,
					txtid->last_num_timeout);
			}
		}

		/* reset the counters */
		txtid->last_num_amsdu = 0;
		txtid->last_num_timeout = 0;

		/* Update to current time */
		txtid->last_check_time = now;
	}

	return;
}

static void aggr_tx_reset_aggr(struct txtid *txtid, bool free_buf,
				bool timer_stop)
{
	/* Need protected by tid->lock. */
	if (timer_stop)
		del_timer(&txtid->timer);

	if ((free_buf) &&
		(txtid->amsdu_skb))
		dev_kfree_skb(txtid->amsdu_skb);

	txtid->amsdu_skb = NULL;
	txtid->amsdu_start = NULL;
	txtid->amsdu_cnt = 0;
	txtid->amsdu_len = 0;
	txtid->amsdu_lastpdu_len = 0;

	return;
}

static void aggr_tx_delete_tid_state(struct aggr_conn_info *aggr_conn, u8 tid)
{
	struct txtid *txtid;
	struct aggr_info *aggr = aggr_conn->aggr_cntxt;

	txtid = AGGR_GET_TXTID(aggr_conn, tid);

	spin_lock_bh(&txtid->lock);
	txtid->aid = 0;
	txtid->max_aggr_sz = 0;

	aggr_tx_reset_aggr(txtid, true, true);

	txtid->num_pdu = 0;
	txtid->num_amsdu = 0;
	txtid->num_timeout = 0;
	txtid->num_flush = 0;
	txtid->num_tx_null = 0;
	txtid->num_overflow = 0;

	txtid->last_check_time = 0;
	txtid->last_num_amsdu = 0;
	txtid->last_num_timeout = 0;
	if (aggr->tx_amsdu_progressive_hispeed) {
		aggr->tx_amsdu_progressive_hispeed = false;
		aggr_tx_config(aggr->vif,
				aggr->tx_amsdu_seq_pkt,
				true,
				AGGR_TX_MAX_NUM,
				AGGR_TX_MAX_PDU_SIZE,
				AGGR_TX_TIMEOUT);

		ath6kl_dbg(ATH6KL_DBG_WLAN_TX_AMSDU,
				"%s: AMSDU reset to normal speed\n",
				__func__);
	}

	spin_unlock_bh(&txtid->lock);

	return;
}

static int aggr_tx(struct ath6kl_vif *vif, struct ath6kl_sta *sta,
			struct sk_buff **skb)
{
#define	ETHERTYPE_IP	0x0800		/* IP  protocol */
#define IP_PROTO_TCP	0x6		/* TCP protocol */
	struct ethhdr *eth_hdr;
	struct ath6kl_llc_snap_hdr *llc_hdr;
	struct aggr_info *aggr;
	struct txtid *txtid;
	int pdu_len, subframe_len;
	int hdr_len = /*WMI_MAX_TX_META_SZ + */sizeof(struct wmi_data_hdr);

	pdu_len = (*skb)->len - hdr_len;
	aggr = vif->aggr_cntxt;

	/*
	 * Only aggr IP/TCP frames, focus on small TCP-ACK frams.
	 * Bypass multicast and non-IP/TCP frames.
	 *
	 * Reserved 14 bytes 802.3 header ahead of A-MSDU frame for target
	 * to transfer to 802.11 header.
	 */
	if (pdu_len > aggr->tx_amsdu_max_pdu_len)
		return AGGR_TX_BYPASS;

	eth_hdr = (struct ethhdr *)((*skb)->data + hdr_len);
	if (is_multicast_ether_addr(eth_hdr->h_dest))
		return AGGR_TX_BYPASS;

	llc_hdr = (struct ath6kl_llc_snap_hdr *)((*skb)->data + hdr_len +
		sizeof(struct ethhdr));
	if (llc_hdr->eth_type == htons(ETHERTYPE_IP)) {
		struct iphdr *ip_hdr = (struct iphdr *)((u8 *)eth_hdr +
			sizeof(struct ethhdr) +
			sizeof(struct ath6kl_llc_snap_hdr));

		if (ip_hdr->protocol == IP_PROTO_TCP) {
			struct ath6kl_sta *conn;

			if (!sta)
				conn = ath6kl_find_sta(vif, eth_hdr->h_dest);
			else {
				/* Only in AP mode and we already know the
				 * station.
				 */
				WARN_ON(vif->nw_type != AP_NETWORK);

				conn = sta;
			}

			ath6kl_dbg_dump(ATH6KL_DBG_RAW_BYTES, __func__,
				"aggr tx ", (*skb)->data, (*skb)->len);

			if (conn) {
				struct sk_buff *amsdu_skb;
				struct wmi_data_hdr *wmi_hdr =
					(struct wmi_data_hdr *)((u8 *)eth_hdr -
					sizeof(struct wmi_data_hdr));
				u16 info2_tmp;

				/* Not allow TX-AMSDU during STA sleep. */
				if ((vif->nw_type == AP_NETWORK) &&
				    (conn->sta_flags & (STA_PS_SLEEP |
							STA_PS_POLLED |
							STA_PS_APSD_TRIGGER))) {
					ath6kl_dbg(ATH6KL_DBG_WLAN_TX_AMSDU,
						"%s: STA is in sleep state, aid %d sta_flags %x\n",
						__func__,
						conn->aid,
						conn->sta_flags);
					return AGGR_TX_BYPASS;
				}

				txtid = AGGR_GET_TXTID(conn->aggr_conn_cntxt,
				((wmi_hdr->info >> WMI_DATA_HDR_UP_SHIFT) &
				WMI_DATA_HDR_UP_MASK));

				spin_lock_bh(&txtid->lock);
				if (!txtid->max_aggr_sz) {
					spin_unlock_bh(&txtid->lock);
					return AGGR_TX_BYPASS;
				}

				amsdu_skb = txtid->amsdu_skb;
				if (amsdu_skb == NULL) {
					amsdu_skb =
					dev_alloc_skb(AGGR_TX_MAX_AGGR_SIZE);
					if (amsdu_skb == NULL) {
						spin_unlock_bh(&txtid->lock);
						return AGGR_TX_BYPASS;
					}

					/* Change to A-MSDU type */
					info2_tmp = le16_to_cpu(wmi_hdr->info2);
					info2_tmp |= (WMI_DATA_HDR_AMSDU_MASK <<
						WMI_DATA_HDR_AMSDU_SHIFT);
					wmi_hdr->info2 = cpu_to_le16(info2_tmp);

					/* Clone meta-data & WMI-header. */
					memcpy(amsdu_skb->data - hdr_len,
						(*skb)->data, hdr_len);

					aggr_tx_reset_aggr(txtid, false, true);
					txtid->amsdu_skb = amsdu_skb;
					txtid->amsdu_start = amsdu_skb->data;

					amsdu_skb->data +=
						sizeof(struct ethhdr);

					/* Start tx timeout timer */
					mod_timer(&txtid->timer, jiffies +
						msecs_to_jiffies(
						aggr->tx_amsdu_timeout));
				} else {
					if ((txtid->amsdu_len + pdu_len) >
						aggr->tx_amsdu_max_aggr_len) {
						txtid->num_overflow++;
						spin_unlock_bh(&txtid->lock);

						ath6kl_dbg(
						ATH6KL_DBG_WLAN_TX_AMSDU,
						"%s: AMSDU overflow, pdu_len=%d, amsdu_cnt=%d, amsdu_len=%d\n",
						__func__,
						pdu_len,
						txtid->amsdu_cnt,
						amsdu_skb->len);

						return AGGR_TX_BYPASS;
					}
				}

				/* Zero padding */
				subframe_len = roundup(pdu_len, 4);
				memset(amsdu_skb->data +
					subframe_len - 4, 0, 4);

				/* Append PDU to A-MSDU */
				memcpy(amsdu_skb->data, eth_hdr, pdu_len);
				amsdu_skb->len += subframe_len;
				amsdu_skb->data += subframe_len;

				ath6kl_dbg(ATH6KL_DBG_WLAN_TX_AMSDU,
							   "%s: subframe_len=%d, tid=%d, amsdu_cnt=%d, amsdu_len=%d\n",
					__func__, subframe_len,
					((wmi_hdr->info>>WMI_DATA_HDR_UP_SHIFT)
					& WMI_DATA_HDR_UP_MASK),
					txtid->amsdu_cnt, amsdu_skb->len);

				txtid->amsdu_cnt++;
				txtid->amsdu_lastpdu_len = pdu_len;
				txtid->amsdu_len += subframe_len;

				dev_kfree_skb(*skb);
				*skb = NULL;

				if (txtid->amsdu_cnt >=
					aggr->tx_amsdu_max_aggr_num) {
					/* No padding in last MSDU */
					if (pdu_len & 0x3)
						amsdu_skb->len -=
							(4 - (pdu_len & 0x3));

					/* Update A-MSDU frame header */
					eth_hdr =
					(struct ethhdr *)txtid->amsdu_start;

					if (vif->nw_type == INFRA_NETWORK) {
						memcpy(eth_hdr->h_dest,
						vif->bssid, ETH_ALEN);
						memcpy(eth_hdr->h_source,
						vif->ndev->dev_addr, ETH_ALEN);
					} else {
						memcpy(eth_hdr->h_dest,
							conn->mac, ETH_ALEN);
						memcpy(eth_hdr->h_source,
							vif->ndev->dev_addr,
							ETH_ALEN);
					}

					eth_hdr->h_proto =
						htons(amsdu_skb->len);

					/* Correct final skb's data and length.
					 */
					amsdu_skb->len +=
					    (hdr_len + sizeof(struct ethhdr));
					amsdu_skb->data =
					    txtid->amsdu_start - hdr_len;

					ath6kl_dbg_dump(ATH6KL_DBG_RAW_BYTES,
						__func__, "aggr-tx ",
						amsdu_skb->data,
						amsdu_skb->len);

					/* update stat. */
					txtid->num_amsdu++;
					txtid->num_pdu += txtid->amsdu_cnt;

					*skb = amsdu_skb;
					aggr_tx_reset_aggr(txtid, false, true);
					aggr_tx_progressive(txtid, false);
					spin_unlock_bh(&txtid->lock);

					return AGGR_TX_DONE;
				} else {
					spin_unlock_bh(&txtid->lock);
					return AGGR_TX_OK;
				}
			} else
				return AGGR_TX_DROP;
		}
	}

	return AGGR_TX_BYPASS;
}

static int aggr_tx_tid(struct txtid *txtid, bool timer_stop)
{
	struct ath6kl_vif *vif = txtid->vif;
	struct ath6kl *ar = vif->ar;
	struct ath6kl_cookie *cookie;
	enum htc_endpoint_id eid;
	struct wmi_data_hdr *wmi_hdr;
	struct sk_buff *amsdu_skb, *skb = NULL;
	struct ethhdr *eth_hdr;
	int ac;
	int hdr_len = /*WMI_MAX_TX_META_SZ + */sizeof(struct wmi_data_hdr);

	spin_lock_bh(&txtid->lock);
	amsdu_skb = txtid->amsdu_skb;
	if (amsdu_skb == NULL) {
		txtid->num_tx_null++;
		spin_unlock_bh(&txtid->lock);
		return -EINVAL;
	}

	if (timer_stop)
		txtid->num_flush++;
	else
		txtid->num_timeout++;

	ath6kl_dbg(ATH6KL_DBG_WLAN_TX_AMSDU,
		   "%s: amsdu_skb=0x%p, data=0x%p, len=0x%x, amsdu_cnt=%d\n",
		   __func__,
		   amsdu_skb, amsdu_skb->data,
		   amsdu_skb->len, txtid->amsdu_cnt);

	/* No padding in last MSDU */
	if (txtid->amsdu_lastpdu_len & 0x3)
		amsdu_skb->len -= (4 - (txtid->amsdu_lastpdu_len & 0x3));

	/* Update A-MSDU frame header */
	eth_hdr = (struct ethhdr *)txtid->amsdu_start;
	if (vif->nw_type == INFRA_NETWORK) {
		memcpy(eth_hdr->h_dest, vif->bssid, ETH_ALEN);
		memcpy(eth_hdr->h_source, vif->ndev->dev_addr, ETH_ALEN);
	} else {
	    struct ath6kl_sta *conn = ath6kl_find_sta_by_aid(vif, txtid->aid);

	    if (conn) {
		memcpy(eth_hdr->h_dest, conn->mac, ETH_ALEN);
		memcpy(eth_hdr->h_source, vif->ndev->dev_addr, ETH_ALEN);
	    } else {
		aggr_tx_reset_aggr(txtid, true, timer_stop);
		spin_unlock_bh(&txtid->lock);

		ath6kl_err("aggr_tx_tid error, no STA found, AID = %d\n",
						txtid->aid);
		return -EINVAL;
	    }
	}
	eth_hdr->h_proto = htons(amsdu_skb->len);

	/* Correct final skb's data and length. */
	amsdu_skb->len += (hdr_len + sizeof(struct ethhdr));
	amsdu_skb->data = txtid->amsdu_start - hdr_len;

	/* update stat. */
	txtid->num_amsdu++;
	txtid->num_pdu += txtid->amsdu_cnt;

	skb = amsdu_skb;
	aggr_tx_reset_aggr(txtid, false, timer_stop);
	aggr_tx_progressive(txtid, !timer_stop);

	spin_unlock_bh(&txtid->lock);

	spin_lock_bh(&ar->lock);
	wmi_hdr = (struct wmi_data_hdr *)
		(skb->data + hdr_len - sizeof(struct wmi_data_hdr));
	ac = up_to_ac[(wmi_hdr->info >> WMI_DATA_HDR_UP_SHIFT) &
		WMI_DATA_HDR_UP_MASK];
	eid = ar->ac2ep_map[ac];

	ath6kl_dbg(ATH6KL_DBG_WLAN_TX_AMSDU,
		   "%s: eid=%d, ac=%d\n", __func__, eid, ac);

	if (eid == 0 || eid == ENDPOINT_UNUSED) {
		ath6kl_err("eid %d is not mapped!\n", eid);
		spin_unlock_bh(&ar->lock);
		goto fail_tx;
	}

	/* allocate resource for this packet */
	cookie = ath6kl_alloc_cookie(ar, COOKIE_TYPE_DATA);

	if (!cookie) {
		spin_unlock_bh(&ar->lock);
		goto fail_tx;
	}
	vif->data_cookie_count++;

	/* update counts while the lock is held */
	ar->tx_pending[eid]++;
	ar->total_tx_data_pend++;

	spin_unlock_bh(&ar->lock);

	cookie->skb = skb;
	cookie->map_no = 0;
	set_htc_pkt_info(cookie->htc_pkt, cookie, skb->data, skb->len,
			 eid, ATH6KL_DATA_PKT_TAG);
	cookie->htc_pkt->skb = skb;

	ath6kl_dbg_dump(ATH6KL_DBG_RAW_BYTES, __func__, "aggr-tx ",
			skb->data, skb->len);

	/* P2P Flowctrl */
	if (ar->conf_flags & ATH6KL_CONF_ENABLE_FLOWCTRL) {
		int ret;

		cookie->htc_pkt->connid =
			ath6kl_p2p_flowctrl_get_conn_id(vif, skb);
		cookie->htc_pkt->recycle_count = 0;
		ret = ath6kl_p2p_flowctrl_tx_schedule_pkt(ar, (void *)cookie);
		if (ret == 0)		/* Queue it */
			return 0;
		else if (ret < 0)	/* Error, drop it. */
			goto fail_tx;
	}

	cookie->htc_pkt->vif = vif;

	ar->tx_on_vif |= (1 << vif->fw_vif_idx);

	/*
	 * HTC interface is asynchronous, if this fails, cleanup will
	 * happen in the ath6kl_tx_complete callback.
	 */
	ath6kl_htc_tx(ar->htc_target, cookie->htc_pkt);

	return 0;

fail_tx:
	dev_kfree_skb(skb);

	vif->net_stats.tx_dropped++;
	vif->net_stats.tx_aborted_errors++;

	return -EINVAL;
}

static void aggr_tx_timeout(unsigned long arg)
{
	struct txtid *txtid = (struct txtid *)arg;

	ath6kl_dbg(ATH6KL_DBG_WLAN_TX_AMSDU,
		   "%s: aid %d, tid %d", __func__, txtid->aid, txtid->tid);

	aggr_tx_tid(txtid, false);

	return;
}

static int aggr_tx_flush(struct ath6kl_vif *vif, struct ath6kl_sta *conn)
{
	int tid;

	if (conn == NULL) {
		if (vif->nw_type == INFRA_NETWORK)
			conn = &vif->sta_list[0];
		else if (vif->nw_type == AP_NETWORK)
			return 0;
		else
			BUG_ON(1);
	}

	/* In AP mode, these packages will be queued in target side. */
	for (tid = (NUM_OF_TIDS - 1); tid >= 0; tid--) {
		struct txtid *txtid =
			AGGR_GET_TXTID(conn->aggr_conn_cntxt, tid);

		ath6kl_dbg(ATH6KL_DBG_WLAN_TX_AMSDU,
				"%s: flush sta aid %d\n", __func__, conn->aid);
		aggr_tx_tid(txtid, true);
	}
	return 0;
}

/*
 * For the continuous un-received packets, wait timer will be divided by 2
 * I.E. pkt#1, pkt#2, pkt#3 don't receive,
 * the pkt#1 will wait tid_timeout_setting
 * the pkt#2 will wait tid_timeout_setting / 2
 * the pkt#3 will wait tid_timeout_setting / 4
 * If more than ATH6KL_MAX_WAIT_CONTINUOUS_PKT, move to the first
 * un-continuous un-received packets
*/
static void aggr_timeout(unsigned long arg)
{
	u8 j;
	struct rxtid *rxtid = (struct rxtid *) arg;
	struct aggr_conn_info *aggr_conn = rxtid->aggr_conn;
	struct rxtid_stats *stats;
	u32 tid_next_timeout =
		aggr_conn->tid_timeout_setting[rxtid->tid];

	stats = AGGR_GET_RXTID_STATS(aggr_conn, rxtid->tid);

	if (!rxtid->aggr || !rxtid->tid_timer_scheduled)
		return;

	spin_lock_bh(&rxtid->lock);

	if (rxtid->timerwait_seq_num == rxtid->seq_next) {
		stats->num_timeouts++;
		ath6kl_dbg(ATH6KL_DBG_AGGR,
			   "aggr timeout (st %d end %d)(tid=%d)\n",
			   rxtid->seq_next,
			   ((rxtid->seq_next + rxtid->hold_q_sz-1) &
				ATH6KL_MAX_SEQ_NO), rxtid->tid);
		spin_unlock_bh(&rxtid->lock);
		aggr_deque_frms(aggr_conn, rxtid->tid,
			((rxtid->timerwait_seq_num + 1) &
			ATH6KL_MAX_SEQ_NO) , 0);
		/* inorder packet that after time-out packet!! */
		aggr_deque_frms(aggr_conn, rxtid->tid, 0 , 1);
		if (rxtid->seq_next ==
			((rxtid->timerwait_seq_num + 1) &
			ATH6KL_MAX_SEQ_NO)) {
			/* Continus hole */
			if (rxtid->continuous_count >=
					ATH6KL_MAX_WAIT_CONTINUOUS_PKT) {
				aggr_deque_frms(aggr_conn, rxtid->tid,
					((rxtid->issue_timer_seq + 1) &
					ATH6KL_MAX_SEQ_NO) , 0);
			/* inorder packet that after time-out packet!! */
				aggr_deque_frms(aggr_conn, rxtid->tid, 0 , 1);
				rxtid->continuous_count = 0;
			}
		} else {
			rxtid->continuous_count = 0;
		}
		spin_lock_bh(&rxtid->lock);
	}
	rxtid->tid_timer_scheduled = false;

	if (rxtid->hold_q) {
		for (j = 0; j < rxtid->hold_q_sz; j++) {
			if (rxtid->hold_q[j].skb) {
				rxtid->issue_timer_seq =
					rxtid->hold_q[j].seq_no;
				rxtid->timerwait_seq_num = rxtid->seq_next;
				rxtid->tid_timer_scheduled = true;
				rxtid->continuous_count++;
				break;
			}
		}
	}

	if (rxtid->continuous_count > 1) {
		if (rxtid->continuous_count <=
				ATH6KL_MAX_WAIT_CONTINUOUS_PKT) {
			tid_next_timeout = tid_next_timeout /
				((rxtid->continuous_count - 1) * 2);
			ath6kl_dbg(ATH6KL_DBG_AGGR,
				"aggr continuous hole timeout count %d\n",
				rxtid->continuous_count);
		} else {
			ath6kl_dbg(ATH6KL_DBG_AGGR,
				"aggr continuous hole count %d larger than 3?\n",
				rxtid->continuous_count);
			rxtid->continuous_count = 0;
		}
	}

	if (rxtid->tid_timer_scheduled) {
		mod_timer(&rxtid->tid_timer,
			  jiffies + msecs_to_jiffies(tid_next_timeout));
	}
	spin_unlock_bh(&rxtid->lock);
}

static void aggr_delete_tid_state(struct aggr_conn_info *aggr_conn, u8 tid)
{
	struct rxtid *rxtid;
	struct rxtid_stats *stats;

	if (!aggr_conn || tid >= NUM_OF_TIDS)
		return;

	rxtid = AGGR_GET_RXTID(aggr_conn, tid);
	stats = AGGR_GET_RXTID_STATS(aggr_conn, tid);

	if (rxtid->aggr)
		aggr_deque_frms(aggr_conn, tid, 0, 0);
	spin_lock_bh(&rxtid->lock);
	rxtid->aggr = false;
	rxtid->win_sz = 0;
	rxtid->seq_next = 0;
	rxtid->hold_q_sz = 0;

	kfree(rxtid->hold_q);
	rxtid->hold_q = NULL;
	spin_unlock_bh(&rxtid->lock);
	memset(stats, 0, sizeof(struct rxtid_stats));
}

void aggr_recv_addba_req_evt(struct ath6kl_vif *vif, u8 tid, u16 seq_no,
			     u8 win_sz)
{
	struct aggr_conn_info *aggr_conn;
	struct rxtid *rxtid;
	struct rxtid_stats *stats;
	struct ath6kl_sta *conn;
	u16 hold_q_size;
	u8 conn_tid, conn_aid;

	conn_tid = AGGR_BA_EVT_GET_TID(tid);
	conn_aid = AGGR_BA_EVT_GET_CONNID(tid);
	conn = ath6kl_find_sta_by_aid(vif, conn_aid);

	if (conn_tid >= NUM_OF_TIDS) {
		WARN_ON(1);
		return;
	}

	if (conn != NULL) {
		WARN_ON(!conn->aggr_conn_cntxt);

		aggr_conn = conn->aggr_conn_cntxt;
		rxtid = AGGR_GET_RXTID(aggr_conn, conn_tid);
		stats = AGGR_GET_RXTID_STATS(aggr_conn, conn_tid);

		if (win_sz < AGGR_WIN_SZ_MIN || win_sz > AGGR_WIN_SZ_MAX)
			ath6kl_dbg(ATH6KL_DBG_WLAN_RX, "%s: win_sz %d, tid %d, aid %d\n",
				   __func__, win_sz, conn_tid, conn_aid);

		if (rxtid->aggr)
			aggr_delete_tid_state(aggr_conn, conn_tid);
		spin_lock_bh(&rxtid->lock);
		rxtid->seq_next = seq_no;
		hold_q_size = TID_WINDOW_SZ(win_sz) * sizeof(struct skb_hold_q);
		rxtid->hold_q = kzalloc(hold_q_size, GFP_ATOMIC);

		if (!rxtid->hold_q) {
			spin_unlock_bh(&rxtid->lock);
			return;
		}

		rxtid->win_sz = win_sz;
		rxtid->hold_q_sz = TID_WINDOW_SZ(win_sz);
		if (!skb_queue_empty(&rxtid->q)) {
			spin_unlock_bh(&rxtid->lock);
			return;
		}

		rxtid->aggr = true;
		rxtid->sync_next_seq = true;
		spin_unlock_bh(&rxtid->lock);
	}
}

void aggr_recv_addba_resp_evt(struct ath6kl_vif *vif, u8 tid,
	u16 amsdu_sz, u8 status)
{
	struct aggr_conn_info *aggr_conn;
	struct txtid *txtid;
	struct ath6kl_sta *conn;
	u8 i, conn_tid, conn_aid;

	conn_tid = AGGR_BA_EVT_GET_TID(tid);
	conn_aid = AGGR_BA_EVT_GET_CONNID(tid);
	conn = ath6kl_find_sta_by_aid(vif, conn_aid);

	if (conn_tid >= NUM_OF_TIDS) {
		WARN_ON(1);
		return;
	}

	if (conn != NULL) {
		WARN_ON(!conn->aggr_conn_cntxt);

		ath6kl_dbg(ATH6KL_DBG_WMI, "%s: amsdu_sz %d, tid %d, aid %d, status %d\n",
			   __func__, amsdu_sz, conn_tid, conn_aid, status);

		aggr_conn = conn->aggr_conn_cntxt;
		txtid = AGGR_GET_TXTID(aggr_conn, conn_tid);

		spin_lock_bh(&txtid->lock);
		txtid->aid = conn_aid;
		if (status == 0)
			txtid->max_aggr_sz = amsdu_sz;
		else
			txtid->max_aggr_sz = 0;

		/* 0 means disable */
		if (!txtid->max_aggr_sz)
			aggr_tx_reset_aggr(txtid, true, true);
		spin_unlock_bh(&txtid->lock);

		if (vif->nw_type != AP_NETWORK) {
			vif->aggr_cntxt->tx_amsdu_enable = false;
			for (i = 0; i < NUM_OF_TIDS; i++) {
				txtid = AGGR_GET_TXTID(aggr_conn, i);
				if (txtid->max_aggr_sz) {
					vif->aggr_cntxt->tx_amsdu_enable = true;
					break;
				}
			}
		}
	}
}

void aggr_tx_config(struct ath6kl_vif *vif,
			bool tx_amsdu_seq_pkt,
			bool tx_amsdu_progressive,
			u8 tx_amsdu_max_aggr_num,
			u16 tx_amsdu_max_pdu_len,
			u16 tx_amsdu_timeout)
{
	if ((vif) &&
	    (vif->aggr_cntxt)) {
		struct aggr_info *aggr = vif->aggr_cntxt;

		aggr->tx_amsdu_seq_pkt = tx_amsdu_seq_pkt;

		if (!tx_amsdu_progressive &&
			aggr->tx_amsdu_progressive)
			aggr->tx_amsdu_progressive_hispeed = false;
		else if (tx_amsdu_progressive &&
			!aggr->tx_amsdu_progressive) {
			; /* TODO : reset all last_XXXX in txtid */
		}
		aggr->tx_amsdu_progressive = tx_amsdu_progressive;

		if (tx_amsdu_timeout == 0)
			tx_amsdu_timeout = AGGR_TX_TIMEOUT;
		aggr->tx_amsdu_timeout = tx_amsdu_timeout;

		if (tx_amsdu_max_pdu_len == 0)
			tx_amsdu_max_pdu_len = AGGR_TX_MAX_PDU_SIZE;
		else if (tx_amsdu_max_pdu_len < AGGR_TX_MIN_PDU_SIZE)
			tx_amsdu_max_pdu_len = AGGR_TX_MIN_PDU_SIZE;
		if (tx_amsdu_max_pdu_len > (aggr->tx_amsdu_max_aggr_len / 2))
			tx_amsdu_max_pdu_len =
				(aggr->tx_amsdu_max_aggr_len / 2);
		aggr->tx_amsdu_max_pdu_len = tx_amsdu_max_pdu_len;

		if (tx_amsdu_max_aggr_num == 0)
			tx_amsdu_max_aggr_num = AGGR_TX_MAX_NUM;
		else if (tx_amsdu_max_aggr_num < 2)
			tx_amsdu_max_aggr_num = 2;
		aggr->tx_amsdu_max_aggr_num = tx_amsdu_max_aggr_num;

		ath6kl_dbg(ATH6KL_DBG_WLAN_TX_AMSDU,
			   "%s: aggr-conf, vif%d, pdu_len=%d, aggr_num=%d timeout=%d, seq_pkt=%d, prog=%d\n",
			   __func__,
			   vif->fw_vif_idx,
			   aggr->tx_amsdu_max_pdu_len,
			   aggr->tx_amsdu_max_aggr_num,
			   aggr->tx_amsdu_timeout,
			   aggr->tx_amsdu_seq_pkt,
			   aggr->tx_amsdu_progressive);
	}

	return;
}

void aggr_config(struct ath6kl_vif *vif,
			u16 rx_aggr_timeout)
{
	if ((vif) &&
	    (vif->aggr_cntxt)) {
		struct aggr_info *aggr = vif->aggr_cntxt;

		if (rx_aggr_timeout == 0)
			rx_aggr_timeout = AGGR_RX_TIMEOUT;
		aggr->rx_aggr_timeout = rx_aggr_timeout;
	}

	return;
}

struct aggr_info *aggr_init(struct ath6kl_vif *vif)
{
	struct aggr_info *aggr = NULL;

	aggr = kzalloc(sizeof(struct aggr_info), GFP_KERNEL);
	if (!aggr) {
		ath6kl_err("failed to alloc memory for aggr_module\n");
		return NULL;
	}

	aggr->vif = vif;

	skb_queue_head_init(&aggr->free_q);
	ath6kl_alloc_netbufs(&aggr->free_q, AGGR_NUM_OF_FREE_NETBUFS);
	aggr->rx_aggr_timeout = AGGR_RX_TIMEOUT;

	aggr->tx_amsdu_enable = true;
	aggr->tx_amsdu_seq_pkt = true;
	aggr->tx_amsdu_progressive = true;
	aggr->tx_amsdu_max_aggr_num = AGGR_TX_MAX_NUM;
	aggr->tx_amsdu_max_aggr_len = AGGR_TX_MAX_AGGR_SIZE - 100;
	aggr->tx_amsdu_max_pdu_len = AGGR_TX_MAX_PDU_SIZE;
	aggr->tx_amsdu_timeout = AGGR_TX_TIMEOUT;

	/* Always enable host-based A-MSDU. */
	set_bit(AMSDU_ENABLED, &vif->flags);

	return aggr;
}

struct aggr_conn_info *aggr_init_conn(struct ath6kl_vif *vif)
{
	struct aggr_conn_info *aggr_conn = NULL;
	struct rxtid *rxtid;
	struct txtid *txtid;
	u8 i;

	aggr_conn = kzalloc(sizeof(struct aggr_conn_info), GFP_KERNEL);
	if (!aggr_conn) {
		ath6kl_err("failed to alloc memory for aggr_node\n");
		return NULL;
	}

	aggr_conn->aggr_sz = AGGR_SZ_DEFAULT;
	aggr_conn->aggr_cntxt = vif->aggr_cntxt;
	aggr_conn->dev = vif->ndev;

	for (i = 0; i < NUM_OF_TIDS; i++) {
		rxtid = AGGR_GET_RXTID(aggr_conn, i);
		rxtid->aggr = false;
		skb_queue_head_init(&rxtid->q);
		spin_lock_init(&rxtid->lock);
		rxtid->aggr_conn = aggr_conn;
		rxtid->tid = i;
		init_timer(&rxtid->tid_timer);
		rxtid->tid_timer.function = aggr_timeout;
		rxtid->tid_timer.data = (unsigned long) rxtid;
		rxtid->tid_timer_scheduled = false;

		switch (up_to_ac[i]) {
		case WMM_AC_BK:
			aggr_conn->tid_timeout_setting[i] = AGGR_RX_TIMEOUT;
			break;
		case WMM_AC_BE:
			aggr_conn->tid_timeout_setting[i] = AGGR_RX_TIMEOUT;
			break;
		case WMM_AC_VI:
			aggr_conn->tid_timeout_setting[i] = AGGR_RX_TIMEOUT;
			break;
		case WMM_AC_VO:
			aggr_conn->tid_timeout_setting[i] = AGGR_RX_TIMEOUT_VO;
			break;
		}

		/* TX A-MSDU */
		txtid = AGGR_GET_TXTID(aggr_conn, i);
		txtid->tid = i;
		txtid->vif = vif;
		init_timer(&txtid->timer);
		txtid->timer.function = aggr_tx_timeout;
		txtid->timer.data = (unsigned long)txtid;
		spin_lock_init(&txtid->lock);

	}

	return aggr_conn;
}

void aggr_recv_delba_req_evt(struct ath6kl_vif *vif, u8 tid, u8 initiator)
{
	struct aggr_conn_info *aggr_conn;
	struct rxtid *rxtid;
	struct txtid *txtid;
	struct ath6kl_sta *conn;
	u8 conn_tid, conn_aid;

	conn_tid = AGGR_BA_EVT_GET_TID(tid);
	conn_aid = AGGR_BA_EVT_GET_CONNID(tid);
	conn = ath6kl_find_sta_by_aid(vif, conn_aid);

	if (conn != NULL) {
		WARN_ON(!conn->aggr_conn_cntxt);

		aggr_conn = conn->aggr_conn_cntxt;
		if (initiator == 1) {
			/* no aggr tx */
			txtid = AGGR_GET_TXTID(aggr_conn, conn_tid);
			if (txtid)
				aggr_tx_delete_tid_state(aggr_conn, conn_tid);
		} else {
			rxtid = AGGR_GET_RXTID(aggr_conn, conn_tid);
			if (rxtid->aggr)
				aggr_delete_tid_state(aggr_conn, conn_tid);
		}
	}
}

void aggr_reset_state(struct aggr_conn_info *aggr_conn)
{
	struct ath6kl_vif *vif = aggr_conn->aggr_cntxt->vif;
	u8 tid;

	for (tid = 0; tid < NUM_OF_TIDS; tid++) {
		aggr_delete_tid_state(aggr_conn, tid);
		aggr_tx_delete_tid_state(aggr_conn, tid);
	}

	if (vif->nw_type != AP_NETWORK)
		aggr_conn->aggr_cntxt->tx_amsdu_enable = false;

	ath6kl_dbg(ATH6KL_DBG_WLAN_TX_AMSDU, "%s: tx_amsdu_enable %d\n",
		   __func__, aggr_conn->aggr_cntxt->tx_amsdu_enable);

	return;
}

/* clean up our amsdu buffer list */
void ath6kl_cleanup_amsdu_rxbufs(struct ath6kl *ar)
{
	struct htc_packet *packet, *tmp_pkt;

	spin_lock_bh(&ar->lock);
	if (list_empty(&ar->amsdu_rx_buffer_queue)) {
		spin_unlock_bh(&ar->lock);
		return;
	}

	list_for_each_entry_safe(packet, tmp_pkt, &ar->amsdu_rx_buffer_queue,
				 list) {
		list_del(&packet->list);
		spin_unlock_bh(&ar->lock);
		dev_kfree_skb(packet->pkt_cntxt);
		spin_lock_bh(&ar->lock);
	}

	spin_unlock_bh(&ar->lock);
}

void aggr_module_destroy(struct aggr_info *aggr)
{
	if (!aggr)
		return;

	skb_queue_purge(&aggr->free_q);
	kfree(aggr);
}

void aggr_module_destroy_conn(struct aggr_conn_info *aggr_conn)
{
	struct rxtid *rxtid;
	struct txtid *txtid;
	u8 i, k;

	if (!aggr_conn)
		return;

	for (i = 0; i < NUM_OF_TIDS; i++) {
		rxtid = AGGR_GET_RXTID(aggr_conn, i);

		if (rxtid->tid_timer_scheduled) {
			del_timer(&rxtid->tid_timer);
			rxtid->tid_timer_scheduled = false;
			rxtid->continuous_count = 0;
		}

		if (rxtid->hold_q) {
			for (k = 0; k < rxtid->hold_q_sz; k++)
				dev_kfree_skb(rxtid->hold_q[k].skb);
			kfree(rxtid->hold_q);
		}
		skb_queue_purge(&rxtid->q);

		/* TX A-MSDU */
		txtid = AGGR_GET_TXTID(aggr_conn, i);
		spin_lock_bh(&txtid->lock);
		aggr_tx_reset_aggr(txtid, true, true);
		spin_unlock_bh(&txtid->lock);
	}

	kfree(aggr_conn);
}

void ath6kl_indicate_wmm_schedule_change(void *devt, bool change)
{
	struct ath6kl *ar = devt;
	int change_for_stream_pri = 0;

	change_for_stream_pri =
		ath6kl_htc_wmm_schedule_change(ar->htc_target, change);

	if (change_for_stream_pri != 0) {
		if (change == true) {
			/* change the priority order for BE and VI */
			ar->ac_stream_pri_map[WMM_AC_BE] = 2;
			ar->ac_stream_pri_map[WMM_AC_VI] = 1;
		} else {
			ar->ac_stream_pri_map[WMM_AC_BE] = 1;
			ar->ac_stream_pri_map[WMM_AC_VI] = 2;
		}
	}
}

void ath6kl_flush_pend_skb(struct ath6kl_vif *vif)
{

	spin_lock_bh(&vif->pend_skb_lock);

	if (!vif->pend_skb) {
		spin_unlock_bh(&vif->pend_skb_lock);
		return;
	}

	if (!(vif->pend_skb->dev->flags & IFF_UP))
			dev_kfree_skb_any(vif->pend_skb);
	else
		netif_rx_ni(vif->pend_skb);

	vif->pend_skb = NULL;
	clear_bit(FIRST_EAPOL_PENDSENT, &vif->flags);

	spin_unlock_bh(&vif->pend_skb_lock);
}

static void ath6kl_eapol_handshake_protect(struct ath6kl_vif *vif, bool tx)
{
	struct ath6kl *ar = vif->ar;
	struct ath6kl_vif *tmp;
	int i;

	/*
	 * In some system, continuous scan and connection behavior
	 * happened at the same time. Ex, in Android, if the user
	 * keep in WiFi site-survey page.
	 * To avoid scan let EAPOL frame lost or timeout and
	 * here preempt scan for a while when transmit/receive
	 * EAPOL frame.
	 */
	set_bit(EAPOL_HANDSHAKE_PROTECT, &ar->flag);
	ar->eapol_shprotect_vif |= (1 << vif->fw_vif_idx);

	mod_timer(&ar->eapol_shprotect_timer,
		jiffies + ATH6KL_SCAN_PREEMPT_IN_HANDSHAKE);

	for (i = 0; i < ar->vif_max; i++) {
		tmp = ath6kl_get_vif_by_index(ar, i);
		if (tmp && tmp->scan_req) {
			ath6kl_info("%s EAPOL on-going, vif %d\n",
				(tx ? "TX" : "RX"),
				tmp->fw_vif_idx);

			del_timer(&tmp->vifscan_timer);
			ath6kl_wmi_abort_scan_cmd(ar->wmi,
					tmp->fw_vif_idx);
			cfg80211_scan_done(tmp->scan_req, true);
			tmp->scan_req = NULL;
			clear_bit(SCANNING, &tmp->flags);
		}
	}

	return;
}
