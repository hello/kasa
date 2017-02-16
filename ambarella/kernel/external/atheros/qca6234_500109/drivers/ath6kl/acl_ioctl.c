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
#include "hif-ops.h"
#include "cfg80211.h"
#include "target.h"
#include "debug.h"
#include "ce_athtst.h"
#include "hif-ops.h"
#include "acl_wcmd.h"
#include "ieee80211_ioctl.h"
#include <linux/wireless.h>
#include <linux/vmalloc.h>
#ifdef ACL_SUPPORT
int ath6kl_wmi_set_param_cmd(struct net_device *netdev, void *data)
{
	struct ath6kl_vif *vif = netdev_priv(netdev);
	unsigned long      status = EIO;
	struct iwreq *iwr = (struct iwreq *)data;
	struct acl_wcmd_t     *req = NULL;
	unsigned long      req_len = sizeof(struct acl_wcmd_t);
	int val;

	if (down_interruptible(&vif->ar->sem))
		return -EBUSY;

	memcpy(&val, iwr->u.name + sizeof(__u32), sizeof(val));

	req = vmalloc(req_len);
	if (!req) {
		up(&vif->ar->sem);
		return -ENOMEM;
	}
	memset(req, 0, req_len);
	req->mode = iwr->u.mode;
	req->val = val;

	if (req->mode == IEEE80211_PARAM_MACCMD) {
		struct sk_buff *skb;
		struct WMI_AP_ACL_POLICY_CMD *cmd;
		int ret;

		if (req->val > AP_ACL_DENY_MAC) {
			status = EIO;
			goto done;
		}
		skb = ath6kl_wmi_get_new_buf(sizeof(*cmd));
		if (!skb) {
			status = -ENOMEM;
			goto done;
		}

		cmd = (struct WMI_AP_ACL_POLICY_CMD *) skb->data;
		cmd->policy = (u8)req->val;
		cmd->policy = (u8)(req->val | AP_ACL_RETAIN_LIST_MASK);
		ret = ath6kl_wmi_cmd_send(vif->ar->wmi, vif->fw_vif_idx,
			skb, WMI_AP_ACL_POLICY_CMDID, NO_SYNC_WMIFLAG);
		vif->acl_db.policy = req->val;
	}

#ifdef CONFIG_PM
	if (req->mode == IEEE80211_PARAM_SUSPEND_SET) {
		if (req->val == 1) /* enetr suspend mode */
			cfg80211_suspend_issue(vif->ndev, 1);
		else
			cfg80211_suspend_issue(vif->ndev, 0);
	}
#endif
	if (req->mode == IEEE80211_PARAM_MAX_CLIENTS)
		status = ath6kl_wmi_set_ap_num_sta_cmd(vif->ar->wmi,
						vif->fw_vif_idx, req->val);
done:
	vfree(req);
	up(&vif->ar->sem);

	return status;
}

int ath6kl_wmi_get_param_cmd(struct net_device *netdev, void *data)
{
	struct ath6kl_vif *vif = netdev_priv(netdev);
	unsigned long      status = EIO;
	struct iwreq *iwr = (struct iwreq *)data;
	struct acl_wcmd_t     *req = NULL;
	unsigned long      req_len = sizeof(struct acl_wcmd_t);

	if (down_interruptible(&vif->ar->sem))
		return -EBUSY;

	req = vmalloc(req_len);
	if (!req)
		return -ENOMEM;

	req->mode = iwr->u.mode;

	/* return local data,to avoid wast target side code size */
	if (req->mode == IEEE80211_PARAM_MACCMD)
		iwr->u.mode = vif->acl_db.policy;

	vfree(req);
	up(&vif->ar->sem);

	return status;
}
static u8 mlme_mac_cmp_wild(u8 *acl_mac, u8 *sta_mac, u8 wild)
{
	u8 i;

	for (i = 0; i < ATH_MAC_LEN; i++) {
		if (wild & (1<<i))
			continue;
		if (acl_mac[i] != sta_mac[i])
			return 1;
	}
	return 0;
}
int ath6kl_wmi_addmac_cmd(struct net_device *netdev, void *data)
{
	struct ath6kl_vif *vif = netdev_priv(netdev);
	unsigned long      status = EIO;
	struct iwreq *iwr = (struct iwreq *)data;
	char     req_content[16];
	char     *req = &req_content[0];
	struct sockaddr *sa;
	int i;
	s16 assigned_idx = -1;

	memcpy(req, iwr->u.name, 16);
	sa = (struct sockaddr *)req;
	/* update unused entry or exist entry */
	for (i = 0; i < AP_ACL_SIZE; i++) {
		if (mlme_mac_cmp_wild(vif->acl_db.acl_mac[i], sa->sa_data, 0)
			== 0)
			assigned_idx = i;
	}

	if (assigned_idx == -1) {/* not in list,find new entry */
		for (i = 0; i < AP_ACL_SIZE; i++) {
			if ((vif->acl_db.index & (1<<i)) == 0) {
				/* not used item */
				assigned_idx = i;
				break;
			}
		}
	}
	printk(KERN_DEBUG "%s[%d]assigned_idx=%d\n\r",
			__func__, __LINE__, assigned_idx);
	if (assigned_idx == -1)/* no entry can used */
		return -ENOMEM;

	{
		struct sk_buff *skb;
		struct WMI_AP_ACL_MAC_CMD *cmd;
		int ret;

		skb = ath6kl_wmi_get_new_buf(sizeof(*cmd));
		if (!skb)
			return -ENOMEM;

		if (down_interruptible(&vif->ar->sem))
			return -EBUSY;
		cmd = (struct WMI_AP_ACL_MAC_CMD *) skb->data;

		cmd->action = ADD_MAC_ADDR;
		cmd->index = (u8)assigned_idx;
		memcpy(cmd->mac, sa->sa_data, ATH_MAC_LEN);
		cmd->wildcard = 0;
		ret = ath6kl_wmi_cmd_send(vif->ar->wmi, vif->fw_vif_idx, skb,
				WMI_AP_ACL_MAC_LIST_CMDID, NO_SYNC_WMIFLAG);

		/* update local acl db */
		{
			memcpy(vif->acl_db.acl_mac[assigned_idx], sa->sa_data,
					ATH_MAC_LEN);
			vif->acl_db.index = vif->acl_db.index |
				(1 << assigned_idx);
			vif->acl_db.wildcard[assigned_idx] = 0;
		}
		up(&vif->ar->sem);
	}

	return status;
}
int ath6kl_wmi_delmac_cmd(struct net_device *netdev, void *data)
{
	struct ath6kl_vif *vif = netdev_priv(netdev);
	unsigned long      status = EIO;
	struct iwreq *iwr = (struct iwreq *)data;
	char     req_content[16];
	char     *req = &req_content[0];
	struct sockaddr *sa;
	s16 assigned_idx = -1;
	int i;

	memcpy(req, iwr->u.name, 16);
	sa = (struct sockaddr *)req;
	/* update unused entry or exist entry */
	for (i = 0; i < AP_ACL_SIZE; i++) {
		if (mlme_mac_cmp_wild(vif->acl_db.acl_mac[i], sa->sa_data, 0)
				== 0)
			assigned_idx = i;
	}

	if (assigned_idx == -1)/* not in list,find new entry */
		return status;


	{
		struct sk_buff *skb;
		struct WMI_AP_ACL_MAC_CMD *cmd;
		int ret;

		skb = ath6kl_wmi_get_new_buf(sizeof(*cmd));
		if (!skb)
			return -ENOMEM;
		if (down_interruptible(&vif->ar->sem))
			return -EBUSY;
		cmd = (struct WMI_AP_ACL_MAC_CMD *) skb->data;

		cmd->action = DEL_MAC_ADDR;
		cmd->index = (u8)assigned_idx;
		memcpy(cmd->mac, sa->sa_data, ATH_MAC_LEN);
		cmd->wildcard = 0;
		ret = ath6kl_wmi_cmd_send(vif->ar->wmi, vif->fw_vif_idx, skb,
			WMI_AP_ACL_MAC_LIST_CMDID, NO_SYNC_WMIFLAG);
		/* update local acl db */
		{
			memset(vif->acl_db.acl_mac[assigned_idx], 0x00,
				ATH_MAC_LEN);
			vif->acl_db.index = vif->acl_db.index &
				~(1 << assigned_idx);
			vif->acl_db.wildcard[assigned_idx] = 0;
		}
		up(&vif->ar->sem);
	}

	return status;
}
int ath6kl_wmi_getmac_cmd(struct net_device *netdev, void *data)
{
	struct ath6kl_vif *vif = netdev_priv(netdev);
	unsigned long      status = EIO;
	struct iwreq *iwr = (struct iwreq *)data;
	int len;
	u8  *macAddr;
	u8  *macList;
	int *cnt;
	int i;

	len = iwr->u.data.length;
	macList = iwr->u.data.pointer;
	if ((macList == NULL) ||
		(len < sizeof(*cnt)))
		return -ENOMEM;

	if (down_interruptible(&vif->ar->sem))
		return -EBUSY;

	{
		cnt = (int *)macList;
		/* MacAddr Count, then MacAddr one by one. */
		macAddr = macList + sizeof(*cnt);
		len -= sizeof(*cnt);
		*cnt = 0;
		for (i = 0; i < AP_ACL_SIZE; i++) {
			if ((vif->acl_db.index & (1<<i)) != 0) {/* used item */
				len -= IEEE80211_ADDR_LEN;
				if (len < 0)
					return -ENOMEM;
				memcpy((u8 *)macAddr,
					(u8 *)&vif->acl_db.acl_mac[i][0], 6);
				macAddr += IEEE80211_ADDR_LEN;
				(*cnt)++;
			}
		}
	}
	up(&vif->ar->sem);

	return status;
}

int ath6kl_wmi_set_mlme_cmd(struct net_device *netdev, void *data)
{
	struct ath6kl_vif *vif = netdev_priv(netdev);
	unsigned long      status = EIO;
	struct iwreq *iwr = (struct iwreq *)data;
	struct ieee80211req_mlme mlme;

	if (down_interruptible(&vif->ar->sem))
		return -EBUSY;
	memcpy(&mlme, iwr->u.data.pointer, sizeof(struct ieee80211req_mlme));

	switch (mlme.im_op) {
	case IEEE80211_MLME_DISASSOC:
		status = ath6kl_wmi_ap_set_mlme(vif->ar->wmi, vif->fw_vif_idx,
		WMI_AP_DISASSOC, mlme.im_macaddr, mlme.im_reason);
		break;
	case IEEE80211_MLME_DEAUTH:
		status = ath6kl_wmi_ap_set_mlme(vif->ar->wmi, vif->fw_vif_idx,
		WMI_AP_DEAUTH, mlme.im_macaddr, mlme.im_reason);
		break;
	default:
		break;
	}
	up(&vif->ar->sem);
	return status;
}
#endif
