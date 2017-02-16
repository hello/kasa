/*
 * Copyright (c) 2004-2012 Atheros Communications Inc.
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
#include "htc-ops.h"
#include "debug.h"

struct p2p_ps_info *ath6kl_p2p_ps_init(struct ath6kl_vif *vif)
{
	struct p2p_ps_info *p2p_ps;

	p2p_ps = kzalloc(sizeof(struct p2p_ps_info), GFP_KERNEL);
	if (!p2p_ps) {
		ath6kl_err("failed to alloc memory for p2p_ps\n");
		return NULL;
	}

	p2p_ps->vif = vif;
	spin_lock_init(&p2p_ps->p2p_ps_lock);

	ath6kl_dbg(ATH6KL_DBG_POWERSAVE,
		   "p2p_ps init %p type %d\n",
		   vif,
		   vif->wdev.iftype);

	return p2p_ps;
}

void ath6kl_p2p_ps_deinit(struct ath6kl_vif *vif)
{
	struct p2p_ps_info *p2p_ps = vif->p2p_ps_info_ctx;

	if (p2p_ps) {
		spin_lock(&p2p_ps->p2p_ps_lock);
		if (p2p_ps->go_last_beacon_app_ie != NULL)
			kfree(p2p_ps->go_last_beacon_app_ie);

		if (p2p_ps->go_last_noa_ie != NULL)
			kfree(p2p_ps->go_last_noa_ie);

		if (p2p_ps->go_working_buffer != NULL)
			kfree(p2p_ps->go_working_buffer);
		spin_unlock(&p2p_ps->p2p_ps_lock);

		kfree(p2p_ps);
	}

	vif->p2p_ps_info_ctx = NULL;

	ath6kl_dbg(ATH6KL_DBG_POWERSAVE,
		   "p2p_ps deinit %p\n",
		   vif);

	return;
}

int ath6kl_p2p_ps_reset_noa(struct p2p_ps_info *p2p_ps)
{
	if (!p2p_ps)
		return -1;

	/*
	 * This could happend if NOA_INFO event is later than
	 * GO's DISCONNECT event.
	 */
	if (p2p_ps->vif->wdev.iftype != NL80211_IFTYPE_P2P_GO) {
		/*
		 * For P2P-Compat mode, need to clear target's
		 * NOA if the target not to reset it after
		 * P2P-GO teardown.
		*/
		if (!p2p_ps->vif->ar->p2p_compat) {
			ath6kl_dbg(ATH6KL_DBG_INFO,
				"failed to reset P2P-GO noa, %p\n", p2p_ps);

			return -1;
		}
	}

	ath6kl_dbg(ATH6KL_DBG_POWERSAVE,
		   "p2p_ps reset NoA %p index %d\n",
		   p2p_ps->vif,
		   p2p_ps->go_noa.index);

	spin_lock(&p2p_ps->p2p_ps_lock);
	p2p_ps->go_flags &= ~ATH6KL_P2P_PS_FLAGS_NOA_ENABLED;
	p2p_ps->go_noa.index++;
	p2p_ps->go_noa_enable_idx = 0;
	memset(p2p_ps->go_noa.noas,
		0,
		sizeof(struct ieee80211_p2p_noa_descriptor) *
			ATH6KL_P2P_PS_MAX_NOA_DESCRIPTORS);
	spin_unlock(&p2p_ps->p2p_ps_lock);

	return 0;
}

int ath6kl_p2p_ps_setup_noa(struct p2p_ps_info *p2p_ps,
			    int noa_id,
			    u8 count_type,
			    u32 interval,
			    u32 start_offset,
			    u32 duration)
{
	struct ieee80211_p2p_noa_descriptor *noa_descriptor;

	if ((!p2p_ps) ||
		(p2p_ps->vif->wdev.iftype != NL80211_IFTYPE_P2P_GO)) {
		ath6kl_err("failed to setup P2P-GO noa\n");
		return -1;
	}

	ath6kl_dbg(ATH6KL_DBG_POWERSAVE,
		   "p2p_ps setup NoA %p idx %d ct %d intval %x so %x dur %x\n",
		   p2p_ps->vif,
		   noa_id,
		   count_type,
		   interval,
		   start_offset,
		   duration);

	spin_lock(&p2p_ps->p2p_ps_lock);
	if (noa_id < ATH6KL_P2P_PS_MAX_NOA_DESCRIPTORS) {
		noa_descriptor = &p2p_ps->go_noa.noas[noa_id];

		noa_descriptor->count_or_type = count_type;
		noa_descriptor->interval = interval;
		noa_descriptor->start_or_offset = start_offset;
		noa_descriptor->duration = duration;
	} else {
		spin_unlock(&p2p_ps->p2p_ps_lock);
		ath6kl_err("wrong NoA index %d\n", noa_id);

		return -2;
	}

	p2p_ps->go_noa_enable_idx |= (1 << noa_id);
	p2p_ps->go_flags |= ATH6KL_P2P_PS_FLAGS_NOA_ENABLED;
	spin_unlock(&p2p_ps->p2p_ps_lock);

	return 0;
}

int ath6kl_p2p_ps_reset_opps(struct p2p_ps_info *p2p_ps)
{
	if ((!p2p_ps) ||
	    (p2p_ps->vif->wdev.iftype != NL80211_IFTYPE_P2P_GO)) {
		ath6kl_err("failed to reset P2P-GO OppPS\n");
		return -1;
	}

	ath6kl_dbg(ATH6KL_DBG_POWERSAVE,
		   "p2p_ps reset OppPS %p index %d\n",
		   p2p_ps->vif,
		   p2p_ps->go_noa.index);

	spin_lock(&p2p_ps->p2p_ps_lock);
	p2p_ps->go_flags &= ~ATH6KL_P2P_PS_FLAGS_OPPPS_ENABLED;
	p2p_ps->go_noa.index++;
	p2p_ps->go_noa.ctwindow_opps_param = 0;
	spin_unlock(&p2p_ps->p2p_ps_lock);

	return 0;
}

int ath6kl_p2p_ps_setup_opps(struct p2p_ps_info *p2p_ps,
			     u8 enabled,
			     u8 ctwindows)
{
	if ((!p2p_ps) ||
	    (p2p_ps->vif->wdev.iftype != NL80211_IFTYPE_P2P_GO)) {
		ath6kl_err("failed to setup P2P-GO noa\n");
		return -1;
	}

	WARN_ON(enabled && (!(ctwindows & 0x7f)));

	ath6kl_dbg(ATH6KL_DBG_POWERSAVE,
		   "p2p_ps setup OppPS %p enabled %d ctwin %d\n",
		   p2p_ps->vif,
		   enabled,
		   ctwindows);

	spin_lock(&p2p_ps->p2p_ps_lock);
	if (enabled)
		p2p_ps->go_noa.ctwindow_opps_param = (0x80 |
						      (ctwindows & 0x7f));
	else
		p2p_ps->go_noa.ctwindow_opps_param = 0;
	p2p_ps->go_flags |= ATH6KL_P2P_PS_FLAGS_OPPPS_ENABLED;
	spin_unlock(&p2p_ps->p2p_ps_lock);

	return 0;
}

int ath6kl_p2p_ps_update_notif(struct p2p_ps_info *p2p_ps)
{
	struct ath6kl_vif *vif;
	struct ieee80211_p2p_noa_ie *noa_ie;
	struct ieee80211_p2p_noa_descriptor *noa_descriptor;
	int i, idx, len, ret = 0;
	u8 *buf;

	WARN_ON(!p2p_ps);

	vif = p2p_ps->vif;

	p2p_ps->go_noa_notif_cnt++;

	spin_lock(&p2p_ps->p2p_ps_lock);
	if ((p2p_ps->go_flags & ATH6KL_P2P_PS_FLAGS_NOA_ENABLED) ||
	    (p2p_ps->go_flags & ATH6KL_P2P_PS_FLAGS_OPPPS_ENABLED)) {
		WARN_ON((p2p_ps->go_flags & ATH6KL_P2P_PS_FLAGS_NOA_ENABLED) &&
			 (!p2p_ps->go_noa_enable_idx));

		len = p2p_ps->go_last_beacon_app_ie_len +
		      sizeof(struct ieee80211_p2p_noa_ie);

		buf = kmalloc(len, GFP_ATOMIC);
		if (buf == NULL) {
			spin_unlock(&p2p_ps->p2p_ps_lock);

			return -ENOMEM;
		}

		/* Append NoA IE after user's IEs. */
		memcpy(buf,
			p2p_ps->go_last_beacon_app_ie,
			p2p_ps->go_last_beacon_app_ie_len);

		noa_ie = (struct ieee80211_p2p_noa_ie *)
				(buf + p2p_ps->go_last_beacon_app_ie_len);
		noa_ie->element_id = WLAN_EID_VENDOR_SPECIFIC;
		noa_ie->oui = cpu_to_be32((WLAN_OUI_WFA << 8) |
					  (WLAN_OUI_TYPE_WFA_P2P));
		noa_ie->attr = IEEE80211_P2P_ATTR_NOTICE_OF_ABSENCE;
		noa_ie->noa_info.index = p2p_ps->go_noa.index;
		noa_ie->noa_info.ctwindow_opps_param =
				p2p_ps->go_noa.ctwindow_opps_param;

		idx = 0;
		for (i = 0; i < ATH6KL_P2P_PS_MAX_NOA_DESCRIPTORS; i++) {
			if (p2p_ps->go_noa_enable_idx & (1 << i)) {
				noa_descriptor =
					&noa_ie->noa_info.noas[idx++];
				noa_descriptor->count_or_type =
					p2p_ps->go_noa.noas[i].count_or_type;
				noa_descriptor->duration =
					cpu_to_le32(
					p2p_ps->go_noa.noas[i].duration);
				noa_descriptor->interval =
					cpu_to_le32(
					p2p_ps->go_noa.noas[i].interval);
				noa_descriptor->start_or_offset =
					cpu_to_le32(
					p2p_ps->go_noa.noas[i].start_or_offset);
			}

		}

		/* Update length */
		noa_ie->attr_len = cpu_to_le16(2 +
			(sizeof(struct ieee80211_p2p_noa_descriptor) * idx));
		noa_ie->len = noa_ie->attr_len + 4 + 1 + 2; /* OUI, attr, len */
		len = p2p_ps->go_last_beacon_app_ie_len + (noa_ie->len + 2);

		/* Backup NoA IE for origional code path if need. */
		p2p_ps->go_last_noa_ie_len = 0;

		if (p2p_ps->go_last_noa_ie != NULL)
			kfree(p2p_ps->go_last_noa_ie);
		p2p_ps->go_last_noa_ie = kmalloc(noa_ie->len + 2, GFP_ATOMIC);
		if (p2p_ps->go_last_noa_ie) {
			p2p_ps->go_last_noa_ie_len = noa_ie->len + 2;
			memcpy(p2p_ps->go_last_noa_ie,
				noa_ie,
				p2p_ps->go_last_noa_ie_len);
		}

		spin_unlock(&p2p_ps->p2p_ps_lock);

		ath6kl_dbg(ATH6KL_DBG_POWERSAVE,
			   "p2p_ps update app IE %p f %x idx %d ie %d len %d\n",
			   vif,
			   p2p_ps->go_flags,
			   idx,
			   noa_ie->len,
			   len);
	} else {
		/*
		 * Ignore if FW disable NoA/OppPS but actually no NoA/OppPS
		 * currently.
		 */
		if (p2p_ps->go_last_beacon_app_ie_len == 0) {
			spin_unlock(&p2p_ps->p2p_ps_lock);

			return ret;
		}

		/* Remove NoA IE. */
		p2p_ps->go_last_noa_ie_len = 0;

		if (p2p_ps->go_last_noa_ie != NULL) {
			kfree(p2p_ps->go_last_noa_ie);
			p2p_ps->go_last_noa_ie = NULL;
		}

		buf = kmalloc(p2p_ps->go_last_beacon_app_ie_len, GFP_ATOMIC);
		if (buf == NULL) {
			spin_unlock(&p2p_ps->p2p_ps_lock);

			return -ENOMEM;
		}

		/* Back to origional Beacon IEs. */
		len = p2p_ps->go_last_beacon_app_ie_len;
		memcpy(buf,
			p2p_ps->go_last_beacon_app_ie,
			len);

		spin_unlock(&p2p_ps->p2p_ps_lock);

		ath6kl_dbg(ATH6KL_DBG_POWERSAVE,
			   "p2p_ps update app IE %p f %x beacon_ie %p len %d\n",
			   vif,
			   p2p_ps->go_flags,
			   p2p_ps->go_last_beacon_app_ie,
			   p2p_ps->go_last_beacon_app_ie_len);
	}

	/*
	 * Only need to update Beacon's IE. The ProbeResp'q IE is settled
	 * while sending.
	 */
	ret = ath6kl_wmi_set_appie_cmd(vif->ar->wmi,
				       vif->fw_vif_idx,
				       WMI_FRAME_BEACON,
				       buf,
				       len);

	kfree(buf);

	return ret;
}

void ath6kl_p2p_ps_user_app_ie(struct p2p_ps_info *p2p_ps,
				u8 mgmt_frm_type,
				u8 **ie,
				int *len)
{
	if ((!p2p_ps) || (p2p_ps->vif->wdev.iftype != NL80211_IFTYPE_P2P_GO))
		return;

	ath6kl_dbg(ATH6KL_DBG_POWERSAVE,
		"p2p_ps hook app IE %p f %x mgmt_frm_type %d len %d\n",
		p2p_ps->vif,
		p2p_ps->go_flags,
		mgmt_frm_type,
		*len);

	if (mgmt_frm_type == WMI_FRAME_BEACON) {
		WARN_ON((*len) == 0);

		spin_lock(&p2p_ps->p2p_ps_lock);
		p2p_ps->go_last_beacon_app_ie_len = 0;

		if (p2p_ps->go_last_beacon_app_ie != NULL)
			kfree(p2p_ps->go_last_beacon_app_ie);

		p2p_ps->go_last_beacon_app_ie = kmalloc(*len, GFP_ATOMIC);
		if (p2p_ps->go_last_beacon_app_ie == NULL) {
			spin_unlock(&p2p_ps->p2p_ps_lock);
			return;
		}

		/* Update to the latest one. */
		p2p_ps->go_last_beacon_app_ie_len = *len;
		memcpy(p2p_ps->go_last_beacon_app_ie, *ie, *len);

		spin_unlock(&p2p_ps->p2p_ps_lock);
	} else if (mgmt_frm_type == WMI_FRAME_PROBE_RESP) {
		/* Assume non-zero means P2P node. */
		if ((*len) == 0)
			return;
	}

	/* Hack : Change ie/len to let caller use the new one. */
	spin_lock(&p2p_ps->p2p_ps_lock);
	if ((p2p_ps->go_flags & ATH6KL_P2P_PS_FLAGS_NOA_ENABLED) ||
		(p2p_ps->go_flags & ATH6KL_P2P_PS_FLAGS_OPPPS_ENABLED)) {
		/*
		 * Append the last NoA IE to *ie and
		 * also update *len to let caller
		 * use the new one.
		 */
		WARN_ON(!p2p_ps->go_last_noa_ie);

		if (p2p_ps->go_working_buffer != NULL)
			kfree(p2p_ps->go_working_buffer);

		p2p_ps->go_working_buffer =
			kmalloc((p2p_ps->go_last_noa_ie_len + *len),
				GFP_ATOMIC);

		if (p2p_ps->go_working_buffer) {
			if (*len)
				memcpy(p2p_ps->go_working_buffer, *ie, *len);
			memcpy(p2p_ps->go_working_buffer + (*len),
			       p2p_ps->go_last_noa_ie,
			       p2p_ps->go_last_noa_ie_len);

			if (mgmt_frm_type == WMI_FRAME_PROBE_RESP) {
				/* caller will release it. */
				kfree(*ie);
				*ie = p2p_ps->go_working_buffer;
				p2p_ps->go_working_buffer = NULL;
			} else
				*ie = p2p_ps->go_working_buffer;
			*len += p2p_ps->go_last_noa_ie_len;
		}

		ath6kl_dbg(ATH6KL_DBG_POWERSAVE,
			   "p2p_ps change app IE len -> %d\n",
			   *len);
	}
	spin_unlock(&p2p_ps->p2p_ps_lock);

	return;
}

int ath6kl_p2p_utils_trans_porttype(enum nl80211_iftype type,
				    u8 *opmode,
				    u8 *subopmode)
{
	int ret = 0;

	/*
	 * nl80211.h support NL80211_IFTYPE_P2P_DEVICE from kernel 3.7,
	 * but not yet see any applications to use it now.
	 * To avoid conflict with old nl80211.h and change namming
	 * with postfix _QCA.
	 */
	if (type == NL80211_IFTYPE_P2P_DEVICE_QCA) {
		*opmode = HI_OPTION_FW_MODE_BSS_STA;
		*subopmode = HI_OPTION_FW_SUBMODE_P2PDEV;
	} else {
		switch (type) {
		case NL80211_IFTYPE_AP:
			*opmode = HI_OPTION_FW_MODE_AP;
			*subopmode = HI_OPTION_FW_SUBMODE_NONE;
			break;
		case NL80211_IFTYPE_STATION:
			*opmode = HI_OPTION_FW_MODE_BSS_STA;
			*subopmode = HI_OPTION_FW_SUBMODE_NONE;
			break;
		case NL80211_IFTYPE_P2P_CLIENT:
			*opmode = HI_OPTION_FW_MODE_BSS_STA;
			*subopmode = HI_OPTION_FW_SUBMODE_P2PCLIENT;
			break;
		case NL80211_IFTYPE_P2P_GO:
			*opmode = HI_OPTION_FW_MODE_BSS_STA;
			*subopmode = HI_OPTION_FW_SUBMODE_P2PGO;
			break;
		case NL80211_IFTYPE_UNSPECIFIED:
		case NL80211_IFTYPE_ADHOC:
		case NL80211_IFTYPE_AP_VLAN:
		case NL80211_IFTYPE_WDS:
		case NL80211_IFTYPE_MONITOR:
		case NL80211_IFTYPE_MESH_POINT:
		default:
			ath6kl_err("error interface type %d\n", type);
			ret = -1;
			break;
		}
	}

	return ret;
}

static inline void _revert_ht_cap(struct ath6kl_vif *vif)
{
	struct ath6kl *ar = vif->ar;
#ifdef CONFIG_ATH6KL_DEBUG
	struct ht_cap_param *htCapParam;

	htCapParam = &ar->debug.ht_cap_param[IEEE80211_BAND_5GHZ];
	if (htCapParam->isConfig)
		ath6kl_wmi_set_ht_cap_cmd(ar->wmi,
					vif->fw_vif_idx,
					htCapParam->band,
					htCapParam->chan_width_40M_supported,
					htCapParam->short_GI,
					htCapParam->intolerance_40MHz);
	else
#endif
		ath6kl_wmi_set_ht_cap_cmd(ar->wmi,
					vif->fw_vif_idx,
					A_BAND_5GHZ,
					ATH6KL_5GHZ_HT40_DEF_WIDTH,
					ATH6KL_5GHZ_HT40_DEF_SGI,
					ATH6KL_5GHZ_HT40_DEF_INTOLR40);

	return;
}

int ath6kl_p2p_utils_init_port(struct ath6kl_vif *vif,
			       enum nl80211_iftype type)
{
	struct ath6kl *ar = vif->ar;
	u8 fw_vif_idx = vif->fw_vif_idx;
	u8 opmode, subopmode;
	long left;
	u8 skip_vif = 1;

	if (ar->p2p_compat){
		ath6kl_wmi_set_roam_ctrl_cmd(ar->wmi,
			fw_vif_idx, 0xFFFF, 0, 0, 100);
		return 0;
	}

	/*
	 * Only need to do this if virtual interface used but bypass
	 * vif_max=2 case.
	 * This case suppose be used only for special P2P purpose that is
	 * without dedicated P2P-Device.
	 *
	 * 1st interface always be created by driver init phase and WMI
	 * interface not yet ready. Actually, we don't need to reset it
	 * because current design
	 * always STA interface in firmware and host sides.
	 */
	if (ar->p2p_dedicate)
		skip_vif = 2;

	if ((ar->vif_max > skip_vif) && fw_vif_idx) {
		if (ar->p2p_dedicate && (fw_vif_idx == (ar->vif_max - 1)))
			type = NL80211_IFTYPE_P2P_DEVICE_QCA;

		if (ath6kl_p2p_utils_trans_porttype(type,
						&opmode,
						&subopmode) == 0) {
			/* Delete it first. */
			if (type != NL80211_IFTYPE_P2P_DEVICE_QCA) {
				set_bit(PORT_STATUS_PEND, &vif->flags);
				if (ath6kl_wmi_del_port_cmd(ar->wmi,
							    fw_vif_idx,
							    fw_vif_idx))
					return -EIO;

				left = wait_event_interruptible_timeout(
						ar->event_wq,
						!test_bit(PORT_STATUS_PEND,
						&vif->flags),
						WMI_TIMEOUT/2);
				WARN_ON(left <= 0);
			}

			/* Only support exectly id-to-id mapping. */
			set_bit(PORT_STATUS_PEND, &vif->flags);
			if (ath6kl_wmi_add_port_cmd(ar->wmi,
						    vif,
						    opmode,
						    subopmode))
				return -EIO;

			left = wait_event_interruptible_timeout(
						ar->event_wq,
						!test_bit(PORT_STATUS_PEND,
						&vif->flags),
						WMI_TIMEOUT);
			WARN_ON(left <= 0);

			if (type == NL80211_IFTYPE_P2P_CLIENT) {
				/*
				 * P2P-GO will dissolve P2P-Group immediately
				 * when P2P-Client disconnect in Android.
				 * A larger bmiss time to avoid this in noisy
				 * environment.
				 */
				ath6kl_wmi_set_bmiss_time(ar->wmi,
							vif->fw_vif_idx,
							ATH6KL_P2P_BMISS_TIME);

				/* p2p client shall not do normal roam */
				if (vif->sc_params.scan_ctrl_flags &
					ROAM_SCAN_CTRL_FLAGS)
					ath6kl_wmi_set_roam_ctrl_cmd(
						ar->wmi, vif->fw_vif_idx,
						0xFFFF, 0, 0, 100);
			}

			/* WAR: Revert HT CAP, only for AP/P2P-GO cases. */
			if ((type == NL80211_IFTYPE_AP) ||
			    (type == NL80211_IFTYPE_P2P_GO))
				_revert_ht_cap(vif);
		} else
			return -ENOTSUPP;
	}

	return 0;
}

int ath6kl_p2p_utils_check_port(struct ath6kl_vif *vif,
				u8 port_id)
{
	if (test_bit(PORT_STATUS_PEND, &vif->flags)) {
		WARN_ON(vif->fw_vif_idx != port_id);

		clear_bit(PORT_STATUS_PEND, &vif->flags);
		wake_up(&vif->ar->event_wq);
	}

	return 0;
}

struct ath6kl_p2p_flowctrl *ath6kl_p2p_flowctrl_conn_list_init(
						struct ath6kl *ar)
{
	struct ath6kl_p2p_flowctrl *p2p_flowctrl;
	struct ath6kl_fw_conn_list *fw_conn;
	int i;

	p2p_flowctrl = kzalloc(sizeof(struct ath6kl_p2p_flowctrl), GFP_KERNEL);
	if (!p2p_flowctrl) {
		ath6kl_err("failed to alloc memory for p2p_flowctrl\n");
		return NULL;
	}

	p2p_flowctrl->ar = ar;
	spin_lock_init(&p2p_flowctrl->p2p_flowctrl_lock);
	p2p_flowctrl->sche_type = P2P_FLOWCTRL_SCHE_TYPE_CONNECTION;

	for (i = 0; i < NUM_CONN; i++) {
		fw_conn = &p2p_flowctrl->fw_conn_list[i];
		INIT_LIST_HEAD(&fw_conn->conn_queue);
		INIT_LIST_HEAD(&fw_conn->re_queue);
		fw_conn->connect_status = 0;
		fw_conn->previous_can_send = true;
		fw_conn->connId = ATH6KL_P2P_FLOWCTRL_NULL_CONNID;
		fw_conn->parent_connId = ATH6KL_P2P_FLOWCTRL_NULL_CONNID;
		memset(fw_conn->mac_addr, 0, ETH_ALEN);

		fw_conn->sche_tx = 0;
		fw_conn->sche_re_tx = 0;
		fw_conn->sche_re_tx_aging = 0;
		fw_conn->sche_tx_queued = 0;
	}

	ath6kl_dbg(ATH6KL_DBG_FLOWCTRL,
		   "p2p_flowctrl init %p NUM_CONN %d type %d\n",
		   ar,
		   NUM_CONN,
		   p2p_flowctrl->sche_type);

	return p2p_flowctrl;
}

void ath6kl_p2p_flowctrl_conn_list_deinit(struct ath6kl *ar)
{
	struct ath6kl_p2p_flowctrl *p2p_flowctrl = ar->p2p_flowctrl_ctx;
	struct ath6kl_fw_conn_list *fw_conn;
	int i;

	if (p2p_flowctrl) {
		/* check memory leakage */
		for (i = 0; i < NUM_CONN; i++) {
			spin_lock_bh(&p2p_flowctrl->p2p_flowctrl_lock);
			fw_conn = &p2p_flowctrl->fw_conn_list[i];
			if (fw_conn->sche_tx_queued != 0) {
				ath6kl_err("memory leakage? %d,%d,%d,%d,%d\n",
						i,
						fw_conn->sche_tx,
						fw_conn->sche_re_tx,
						fw_conn->sche_re_tx_aging,
						fw_conn->sche_tx_queued);
				WARN_ON(1);
			}
			spin_unlock_bh(&p2p_flowctrl->p2p_flowctrl_lock);
		}

		kfree(p2p_flowctrl);
	}

	ar->p2p_flowctrl_ctx = NULL;

	ath6kl_dbg(ATH6KL_DBG_FLOWCTRL,
		   "p2p_flowctrl deinit %p\n",
		   ar);

	return;
}

/* before calling this, p2p_flowctrl_lock shall be acquired,
  *  pcontainer, preclaim shall be init by caller.
  */
void ath6kl_p2p_flowctrl_conn_collect_by_conn(
	struct ath6kl_fw_conn_list *fw_conn,
	struct list_head *pcontainer,
	int *preclaim)
{
	struct htc_packet *packet, *tmp_pkt;

	if (!list_empty(&fw_conn->re_queue)) {
		list_for_each_entry_safe(packet,
					tmp_pkt,
					&fw_conn->re_queue,
					list) {
			list_del(&packet->list);
			packet->status = 0;
			list_add_tail(&packet->list, pcontainer);
			fw_conn->sche_tx_queued--;
			*preclaim += 1;
		}
	}

	if (!list_empty(&fw_conn->conn_queue)) {
		list_for_each_entry_safe(packet,
					tmp_pkt,
					&fw_conn->conn_queue,
					list) {
			list_del(&packet->list);
			packet->status = 0;
			list_add_tail(&packet->list, pcontainer);
			fw_conn->sche_tx_queued--;
			*preclaim += 1;
		}
	}
}

void ath6kl_p2p_flowctrl_conn_list_cleanup(struct ath6kl *ar)
{
	struct ath6kl_p2p_flowctrl *p2p_flowctrl = ar->p2p_flowctrl_ctx;
	struct ath6kl_fw_conn_list *fw_conn;
	struct list_head container;
	int i, reclaim = 0;

	WARN_ON(!p2p_flowctrl);

	INIT_LIST_HEAD(&container);

	for (i = 0; i < NUM_CONN; i++) {
		spin_lock_bh(&p2p_flowctrl->p2p_flowctrl_lock);
		fw_conn = &p2p_flowctrl->fw_conn_list[i];
		ath6kl_p2p_flowctrl_conn_collect_by_conn(fw_conn,
			&container,
			&reclaim);
		spin_unlock_bh(&p2p_flowctrl->p2p_flowctrl_lock);
	}

	ath6kl_tx_complete(ar->htc_target, &container);

	ath6kl_dbg(ATH6KL_DBG_FLOWCTRL,
		   "p2p_flowctrl cleanup %p reclaim %d\n",
		   ar,
		   reclaim);

	return;
}

static u8 _find_parent_conn_id(struct ath6kl_p2p_flowctrl *p2p_flowctrl,
				struct ath6kl_vif *hint_vif)
{
	struct ath6kl_fw_conn_list *fw_conn;
	u8 connId = ATH6KL_P2P_FLOWCTRL_NULL_CONNID;
	int i;

	/* Need protected in p2p_flowctrl_lock by caller. */
	fw_conn = &p2p_flowctrl->fw_conn_list[0];
	for (i = 0; i < NUM_CONN; i++, fw_conn++) {
		if (fw_conn->connId == ATH6KL_P2P_FLOWCTRL_NULL_CONNID)
			continue;

		if ((fw_conn->vif == hint_vif) &&
		    (fw_conn->parent_connId == fw_conn->connId)) {
			connId = fw_conn->connId;
			break;
		}
	}

	return connId;
}

void ath6kl_p2p_flowctrl_conn_list_cleanup_by_if(
			struct ath6kl_vif *vif)
{
	struct ath6kl *ar = vif->ar;
	struct ath6kl_p2p_flowctrl *p2p_flowctrl = ar->p2p_flowctrl_ctx;
	struct ath6kl_fw_conn_list *fw_conn;
	struct list_head container;
	int i, reclaim = 0;
	u8 vif_conn_id = ATH6KL_P2P_FLOWCTRL_NULL_CONNID;

	if (!p2p_flowctrl)
		return;

	INIT_LIST_HEAD(&container);

	vif_conn_id = _find_parent_conn_id(p2p_flowctrl, vif);

	if (vif_conn_id == ATH6KL_P2P_FLOWCTRL_NULL_CONNID)
		return;

	spin_lock_bh(&p2p_flowctrl->p2p_flowctrl_lock);
	for (i = 0; i < NUM_CONN; i++) {
		fw_conn = &p2p_flowctrl->fw_conn_list[i];

		if (fw_conn->parent_connId != vif_conn_id)
			continue;

		ath6kl_p2p_flowctrl_conn_collect_by_conn(fw_conn,
			&container,
			&reclaim);
	}
	spin_unlock_bh(&p2p_flowctrl->p2p_flowctrl_lock);

	ath6kl_tx_complete(ar->htc_target, &container);

	ath6kl_dbg(ATH6KL_DBG_FLOWCTRL,
		   "p2p_flowctrlif cleanup %p reclaim %d\n",
		   ar,
		   reclaim);

	return;
}

static inline bool _check_can_send(struct ath6kl *ar,
			struct ath6kl_fw_conn_list *fw_conn)
{
	bool can_send = false;

	do {
		if ((fw_conn->ocs) && !test_bit(SKIP_FLOWCTRL_EVENT, &ar->flag))
			break;

		can_send = true;
	} while (false);

	return can_send;
}

void ath6kl_p2p_flowctrl_netif_transition(
			struct ath6kl *ar, u8 new_state)
{
	struct ath6kl_vif *vif;

	spin_lock_bh(&ar->list_lock);
	list_for_each_entry(vif, &ar->vif_list, list) {
		spin_unlock_bh(&ar->list_lock);

		if (new_state == ATH6KL_P2P_FLOWCTRL_NETIF_STOP &&
				test_bit(CONNECTED, &vif->flags))
			netif_stop_queue(vif->ndev);
		else if (new_state == ATH6KL_P2P_FLOWCTRL_NETIF_WAKE &&
				(test_bit(CONNECTED, &vif->flags) ||
				test_bit(TESTMODE_EPPING, &ar->flag)))
			netif_wake_queue(vif->ndev);

		spin_lock_bh(&ar->list_lock);
	}
	spin_unlock_bh(&ar->list_lock);

	return;
}

void ath6kl_p2p_flowctrl_tx_schedule(struct ath6kl *ar)
{
	struct ath6kl_p2p_flowctrl *p2p_flowctrl = ar->p2p_flowctrl_ctx;
	struct ath6kl_fw_conn_list *fw_conn;
	struct htc_packet *packet, *tmp_pkt;
	int i;

	WARN_ON(!p2p_flowctrl);

	for (i = 0; i < NUM_CONN; i++) {
		spin_lock_bh(&p2p_flowctrl->p2p_flowctrl_lock);

		fw_conn = &p2p_flowctrl->fw_conn_list[i];
		/* Bypass this fw_conn if it not yet used. */
		if (fw_conn->connId == ATH6KL_P2P_FLOWCTRL_NULL_CONNID) {
			spin_unlock_bh(&p2p_flowctrl->p2p_flowctrl_lock);
			continue;
		}

		if (_check_can_send(ar, fw_conn)) {
			if (!list_empty(&fw_conn->re_queue)) {
				list_for_each_entry_safe(packet,
							tmp_pkt,
							&fw_conn->re_queue,
							list) {
					if (packet == NULL)
						continue;

					list_del(&packet->list);

					if (packet->endpoint >= ENDPOINT_MAX)
						continue;

					fw_conn->sche_re_tx--;
					fw_conn->sche_tx_queued--;

					ath6kl_htc_tx(ar->htc_target, packet);
				}
			}

			if (!list_empty(&fw_conn->conn_queue)) {
				list_for_each_entry_safe(packet,
							tmp_pkt,
							&fw_conn->conn_queue,
							list) {
					if (packet == NULL)
						continue;

					list_del(&packet->list);

					if (packet->endpoint >= ENDPOINT_MAX)
						continue;

					fw_conn->sche_tx++;
					fw_conn->sche_tx_queued--;

					ath6kl_htc_tx(ar->htc_target, packet);
				}
			}
		}
		spin_unlock_bh(&p2p_flowctrl->p2p_flowctrl_lock);

		ath6kl_dbg(ATH6KL_DBG_FLOWCTRL,
			   "p2p_flowctrl schedule %p conId %d tx %d re_tx %d\n",
			   ar,
			   i,
			   fw_conn->sche_tx,
			   fw_conn->sche_re_tx);
	}

	return;
}

int ath6kl_p2p_flowctrl_tx_schedule_pkt(struct ath6kl *ar,
					void *pkt)
{
	struct ath6kl_p2p_flowctrl *p2p_flowctrl = ar->p2p_flowctrl_ctx;
	struct ath6kl_fw_conn_list *fw_conn;
	struct ath6kl_cookie *cookie = (struct ath6kl_cookie *)pkt;
	int connId = cookie->htc_pkt->connid;
	int ret = 0;

	WARN_ON(!p2p_flowctrl);

	if (connId == ATH6KL_P2P_FLOWCTRL_NULL_CONNID) {
		ath6kl_dbg(ATH6KL_DBG_FLOWCTRL,
		"p2p_flowctrl fail, NULL connId, just send??\n");

		return 1;	/* Just send it */
		/*return -1;*/	/* Drop it */
	}

	spin_lock_bh(&p2p_flowctrl->p2p_flowctrl_lock);
	fw_conn = &p2p_flowctrl->fw_conn_list[connId];
	if (!_check_can_send(ar, fw_conn)) {
		if (cookie->htc_pkt->info.tx.tag !=
			ATH6KL_PRI_DATA_PKT_TAG) {
			list_add_tail(&cookie->htc_pkt->list,
				&fw_conn->conn_queue);
		} else {
			list_add(&cookie->htc_pkt->list,
				&fw_conn->re_queue);
			fw_conn->sche_re_tx++;
		}
		fw_conn->sche_tx_queued++;
		spin_unlock_bh(&p2p_flowctrl->p2p_flowctrl_lock);

		goto result;
	} else if (!list_empty(&fw_conn->conn_queue) ||
				!list_empty(&fw_conn->re_queue)) {
		if (cookie->htc_pkt->info.tx.tag !=
			ATH6KL_PRI_DATA_PKT_TAG) {
			list_add_tail(&cookie->htc_pkt->list,
				&fw_conn->conn_queue);
		} else {
			list_add(&cookie->htc_pkt->list, &fw_conn->re_queue);
			fw_conn->sche_re_tx++;
		}

		fw_conn->sche_tx_queued++;
		spin_unlock_bh(&p2p_flowctrl->p2p_flowctrl_lock);

		ath6kl_p2p_flowctrl_tx_schedule(ar);

		goto result;
	} else
		ret = 1;
	spin_unlock_bh(&p2p_flowctrl->p2p_flowctrl_lock);

result:
	ath6kl_dbg(ATH6KL_DBG_FLOWCTRL,
		   "p2p_flowctrl schedule pkt %p %s\n",
		   ar,
		   ((ret == 0) ? "queue" : "send"));

	return ret;
}

void ath6kl_p2p_flowctrl_state_change(struct ath6kl *ar)
{
	struct ath6kl_p2p_flowctrl *p2p_flowctrl = ar->p2p_flowctrl_ctx;
	struct ath6kl_fw_conn_list *fw_conn;
	struct htc_packet *packet, *tmp_pkt;
	struct htc_endpoint *endpoint;
	struct list_head    *tx_queue, container;
	int i, eid;
	bool flowctrl_allowed;

	WARN_ON(!p2p_flowctrl);

	INIT_LIST_HEAD(&container);

	for (i = 0; i < NUM_CONN; i++) {
		spin_lock_bh(&p2p_flowctrl->p2p_flowctrl_lock);
		fw_conn = &p2p_flowctrl->fw_conn_list[i];
		flowctrl_allowed = _check_can_send(ar, fw_conn);
		if (!flowctrl_allowed && fw_conn->previous_can_send) {
			spin_lock_bh(&ar->htc_target->tx_lock);
			for (eid = ENDPOINT_5; eid >= ENDPOINT_2; eid--) {
				endpoint = &ar->htc_target->endpoint[eid];
				tx_queue = &endpoint->txq;
				if (list_empty(tx_queue))
					continue;

				list_for_each_entry_safe(packet,
							tmp_pkt,
							tx_queue,
							list) {
					if (packet->connid != i)
						continue;

					list_del(&packet->list);
					if (packet->recycle_count >
					    ATH6KL_P2P_FLOWCTRL_RECYCLE_LIMIT) {
						ath6kl_dbg(ATH6KL_DBG_INFO,
							"recycle cnt exceed\n");

						packet->status = 0;
						list_add_tail(
						&packet->list,
						&container);

						fw_conn->sche_re_tx_aging++;
					} else {
						packet->recycle_count++;
						list_add_tail(
						&packet->list,
						&fw_conn->re_queue);

						fw_conn->sche_re_tx++;
						fw_conn->sche_tx_queued++;
					}
				}
			}
			spin_unlock_bh(&ar->htc_target->tx_lock);
		}
		fw_conn->previous_can_send = flowctrl_allowed;
		spin_unlock_bh(&p2p_flowctrl->p2p_flowctrl_lock);
	}

	ath6kl_p2p_flowctrl_netif_transition(
				ar, ATH6KL_P2P_FLOWCTRL_NETIF_WAKE);

	ath6kl_dbg(ATH6KL_DBG_FLOWCTRL,
		"p2p_flowctrl state_change %p re_tx %d re_tx_aging %d\n",
		ar,
		fw_conn->sche_re_tx,
		fw_conn->sche_re_tx_aging);

	ath6kl_tx_complete(ar->htc_target, &container);

	return;
}

void ath6kl_p2p_flowctrl_state_update(struct ath6kl *ar,
				      u8 numConn,
				      u8 ac_map[],
				      u8 ac_queue_depth[])
{
	struct ath6kl_p2p_flowctrl *p2p_flowctrl = ar->p2p_flowctrl_ctx;
	struct ath6kl_fw_conn_list *fw_conn;
	int i;

	WARN_ON(!p2p_flowctrl);
	WARN_ON(numConn > NUM_CONN);

	spin_lock_bh(&p2p_flowctrl->p2p_flowctrl_lock);
	p2p_flowctrl->p2p_flowctrl_event_cnt++;
	for (i = 0; i < numConn; i++) {
		fw_conn = &p2p_flowctrl->fw_conn_list[i];
		fw_conn->connect_status = ac_map[i];
	}
	spin_unlock_bh(&p2p_flowctrl->p2p_flowctrl_lock);

	ath6kl_dbg(ATH6KL_DBG_FLOWCTRL,
		   "p2p_flowctrl state_update %p ac_map %02x %02x %02x %02x\n",
		   ar,
		   ac_map[0], ac_map[1], ac_map[2], ac_map[3]);

	return;
}

void ath6kl_p2p_flowctrl_set_conn_id(struct ath6kl_vif *vif,
				     u8 mac_addr[],
				     u8 connId)
{
	struct ath6kl *ar = vif->ar;
	struct ath6kl_p2p_flowctrl *p2p_flowctrl = ar->p2p_flowctrl_ctx;
	struct ath6kl_fw_conn_list *fw_conn;
	struct list_head container;
	int reclaim = 0;

	WARN_ON(!p2p_flowctrl);

	INIT_LIST_HEAD(&container);

	spin_lock_bh(&p2p_flowctrl->p2p_flowctrl_lock);
	fw_conn = &p2p_flowctrl->fw_conn_list[connId];
	if (mac_addr) {
		if (fw_conn->sche_tx_queued != 0) {
			ath6kl_p2p_flowctrl_conn_collect_by_conn(fw_conn,
				&container,
				&reclaim);
			spin_unlock_bh(&p2p_flowctrl->p2p_flowctrl_lock);
			ath6kl_tx_complete(ar->htc_target, &container);
			spin_lock_bh(&p2p_flowctrl->p2p_flowctrl_lock);
		}

		fw_conn->vif = vif;
		fw_conn->connId = connId;
		memcpy(fw_conn->mac_addr, mac_addr, ETH_ALEN);

		/*
		 * Parent's connId of P2P-GO/P2P-Client/STA is self.
		 * Parent's connId of P2P-GO's Clients is P2P-GO.
		 */
		if ((vif->nw_type == AP_NETWORK) &&
		    (memcmp(vif->ndev->dev_addr, mac_addr, ETH_ALEN))) {
			/* P2P-GO's Client connection event. */
			fw_conn->parent_connId =
				_find_parent_conn_id(p2p_flowctrl, vif);
		} else {
			/* P2P-GO/P2P-Client/STA connection event. */
			fw_conn->parent_connId = fw_conn->connId;
		}
	} else {
		fw_conn->connId = ATH6KL_P2P_FLOWCTRL_NULL_CONNID;
		fw_conn->parent_connId = ATH6KL_P2P_FLOWCTRL_NULL_CONNID;
		fw_conn->connect_status = 0;
		fw_conn->previous_can_send = true;
		memset(fw_conn->mac_addr, 0, ETH_ALEN);
	}

	fw_conn->sche_tx = 0;
	fw_conn->sche_re_tx = 0;
	fw_conn->sche_re_tx_aging = 0;
	fw_conn->sche_tx_queued = 0;
	spin_unlock_bh(&p2p_flowctrl->p2p_flowctrl_lock);

	ath6kl_dbg(ATH6KL_DBG_FLOWCTRL,
		   "p2p_flowctrl set conn_id %p mode %d conId %d p_conId %d\n",
		   ar,
		   vif->nw_type,
		   connId,
		   fw_conn->parent_connId);

	return;
}

u8 ath6kl_p2p_flowctrl_get_conn_id(struct ath6kl_vif *vif,
				   struct sk_buff *skb)
{
	struct ath6kl *ar = vif->ar;
	struct ath6kl_p2p_flowctrl *p2p_flowctrl = ar->p2p_flowctrl_ctx;
	struct ath6kl_fw_conn_list *fw_conn;
	struct ethhdr *ethhdr;
	u8 *hint;
	u8 connId = ATH6KL_P2P_FLOWCTRL_NULL_CONNID;
	int i;

	if (!p2p_flowctrl)
		return connId;

	ethhdr = (struct ethhdr *)(skb->data + sizeof(struct wmi_data_hdr));

	if (vif->nw_type != AP_NETWORK)
		hint = ethhdr->h_source;
	else {
		if (is_multicast_ether_addr(ethhdr->h_dest))
			hint = ethhdr->h_source;
		else
			hint = ethhdr->h_dest;
	}

	spin_lock_bh(&p2p_flowctrl->p2p_flowctrl_lock);
	fw_conn = &p2p_flowctrl->fw_conn_list[0];
	for (i = 0; i < NUM_CONN; i++, fw_conn++) {
		if (fw_conn->connId == ATH6KL_P2P_FLOWCTRL_NULL_CONNID)
			continue;

		if (memcmp(fw_conn->mac_addr, hint, ETH_ALEN) == 0) {
			connId = fw_conn->connId;
			break;
		}
	}

	/* Change to parent's. */
	if (p2p_flowctrl->sche_type == P2P_FLOWCTRL_SCHE_TYPE_INTERFACE)
		connId = fw_conn->parent_connId;
	spin_unlock_bh(&p2p_flowctrl->p2p_flowctrl_lock);

	ath6kl_dbg(ATH6KL_DBG_FLOWCTRL,
		   "p2p_flowctrl get con_id %d %02x:%02x:%02x:%02x:%02x:%02x\n",
		   connId,
		   hint[0], hint[1], hint[2], hint[3], hint[4], hint[5]);

	return connId;
}

int ath6kl_p2p_flowctrl_stat(struct ath6kl *ar,
			     u8 *buf, int buf_len)
{
	struct ath6kl_p2p_flowctrl *p2p_flowctrl = ar->p2p_flowctrl_ctx;
	struct ath6kl_fw_conn_list *fw_conn;
	int i, len = 0;

	if ((!p2p_flowctrl) || (!buf))
		return 0;

	len += snprintf(buf + len, buf_len - len, "\n NUM_CONN : %d",
		NUM_CONN);
	len += snprintf(buf + len, buf_len - len, "\n SCHE_TYPE : %s",
		(p2p_flowctrl->sche_type == P2P_FLOWCTRL_SCHE_TYPE_CONNECTION ?
		 "CONNECTION" : "INTERFACE"));
	len += snprintf(buf + len, buf_len - len, "\n EVENT_CNT : %d",
		p2p_flowctrl->p2p_flowctrl_event_cnt);

	len += snprintf(buf + len, buf_len - len, "\n NOA_UPDATE :");
	for (i = 0; i < ar->vif_max; i++) {
		struct ath6kl_vif *vif;
		struct p2p_ps_info *p2p_ps;

		vif = ath6kl_get_vif_by_index(ar, i);
		if (vif) {
			p2p_ps = vif->p2p_ps_info_ctx;
			len += snprintf(buf + len, buf_len - len,
				" %d", p2p_ps->go_noa_notif_cnt);
		}
	}
	len += snprintf(buf + len, buf_len - len, "\n");

	spin_lock_bh(&p2p_flowctrl->p2p_flowctrl_lock);
	for (i = 0; i < NUM_CONN; i++) {
		fw_conn = &p2p_flowctrl->fw_conn_list[i];

		if (fw_conn->connId == ATH6KL_P2P_FLOWCTRL_NULL_CONNID)
			continue;

		len += snprintf(buf + len, buf_len - len,
			"\n[%d]===============================\n", i);
		len += snprintf(buf + len, buf_len - len,
			" vif          : %p\n", fw_conn->vif);
		len += snprintf(buf + len, buf_len - len,
			" connId       : %d\n", fw_conn->connId);
		len += snprintf(buf + len, buf_len - len,
			" parent_connId: %d\n", fw_conn->parent_connId);
		len += snprintf(buf + len, buf_len - len,
			" macAddr      : %02x:%02x:%02x:%02x:%02x:%02x\n",
				fw_conn->mac_addr[0],
				fw_conn->mac_addr[1],
				fw_conn->mac_addr[2],
				fw_conn->mac_addr[3],
				fw_conn->mac_addr[4],
				fw_conn->mac_addr[5]);
		len += snprintf(buf + len, buf_len - len,
			" status       : %02x\n", fw_conn->connect_status);
		len += snprintf(buf + len, buf_len - len,
			" preCanSend   : %d\n", fw_conn->previous_can_send);
		len += snprintf(buf + len, buf_len - len,
			" tx_queued    : %d\n", fw_conn->sche_tx_queued);
		len += snprintf(buf + len, buf_len - len,
			" tx           : %d\n", fw_conn->sche_tx);
		len += snprintf(buf + len, buf_len - len,
			" rx_tx        : %d\n", fw_conn->sche_re_tx);
		len += snprintf(buf + len, buf_len - len,
			" re_tx_aging  : %d\n", fw_conn->sche_re_tx_aging);
	}
	spin_unlock_bh(&p2p_flowctrl->p2p_flowctrl_lock);

	return len;
}

struct ath6kl_p2p_rc_info *ath6kl_p2p_rc_init(struct ath6kl *ar)
{
	struct ath6kl_p2p_rc_info *p2p_rc;

	p2p_rc = kzalloc(sizeof(struct ath6kl_p2p_rc_info), GFP_KERNEL);
	if (!p2p_rc) {
		ath6kl_err("failed to alloc memory for p2p_rc\n");
		return NULL;
	}

	p2p_rc->ar = ar;
	p2p_rc->flags = ATH6KL_RC_FLAGS_DONE;
	p2p_rc->user_chan_type = P2P_RC_USER_BLACK_CHAN;
	spin_lock_init(&p2p_rc->p2p_rc_lock);
	p2p_rc->snr_compensation = P2P_RC_DEF_SNR_COMP;

	ath6kl_dbg(ATH6KL_DBG_RC, "p2p_rc init, flags %x\n",
				p2p_rc->flags);

	return p2p_rc;
}

void ath6kl_p2p_rc_deinit(struct ath6kl *ar)
{
	struct ath6kl_p2p_rc_info *p2p_rc = ar->p2p_rc_info_ctx;

	kfree(p2p_rc);

	ar->p2p_rc_info_ctx = NULL;

	ath6kl_dbg(ATH6KL_DBG_RC, "p2p_rc deinit\n");

	return;
}

static inline int __p2p_rc_get_chan_slot(struct ieee80211_channel *channel)
{
#define _MIN_5G_CHAN_ID		34
	int chid = ieee80211_frequency_to_channel(channel->center_freq);

	if (channel->band == (u32)NL80211_BAND_2GHZ)
		return chid - 1;
	else if (channel->band == (u32)NL80211_BAND_5GHZ)
		return ATH6KL_RC_MAX_2G_CHAN_RECORD +
			((chid - _MIN_5G_CHAN_ID) / 2);
	else {
		ath6kl_err("p2p_rc slot fail chid %d!\n", chid);
		BUG_ON(1);
		return -1;
	}
#undef _MIN_5G_CHAN_ID
}

void ath6kl_p2p_rc_fetch_chan(struct ath6kl *ar)
{
	struct ath6kl_p2p_rc_info *p2p_rc = ar->p2p_rc_info_ctx;
	enum ieee80211_band band;
	struct wiphy *wiphy = NULL;
	int i, slot;

	if (!p2p_rc)
		return;

	wiphy = p2p_rc->ar->wiphy;
	spin_lock_bh(&p2p_rc->p2p_rc_lock);
	p2p_rc->chan_record_cnt = 0;
	for (i = 0; i < ATH6KL_RC_MAX_CHAN_RECORD; i++) {
		p2p_rc->chan_record[i].channel = NULL;
		p2p_rc->chan_record[i].best_snr = P2P_RC_NULL_SNR;
		p2p_rc->chan_record[i].aver_snr = P2P_RC_NULL_SNR;
	}

	/* Fetch channel record from wiphy */
	for (band = 0; band < IEEE80211_NUM_BANDS; band++) {
		struct ieee80211_supported_band *sband = wiphy->bands[band];

		if (!sband)
			continue;

		for (i = 0; i < sband->n_channels; i++) {
			struct ieee80211_channel *chan = &sband->channels[i];

			if (chan->flags & IEEE80211_CHAN_DISABLED)
				continue;

			slot = __p2p_rc_get_chan_slot(chan);
			if ((slot >= 0) && (slot < ATH6KL_RC_MAX_CHAN_RECORD)) {
				p2p_rc->chan_record[slot].channel = chan;
				p2p_rc->chan_record_cnt++;
			} else
				ath6kl_err("p2p_rc fetch fail, f %d s %d!\n",
					chan->center_freq,
					slot);
		}
	}

	p2p_rc->flags |= ATH6KL_RC_FLAGS_CHAN_RECORD_FETCHED;
	spin_unlock_bh(&p2p_rc->p2p_rc_lock);

	ath6kl_dbg(ATH6KL_DBG_RC,
			"p2p_rc fetch chan, chan_record_cnt %d\n",
			p2p_rc->chan_record_cnt);

	return;
}

static void _p2p_rc_reset_chan_record(struct ath6kl_p2p_rc_info *p2p_rc)
{
	int i;

	/* Clear all channel record */
	spin_lock_bh(&p2p_rc->p2p_rc_lock);
	for (i = 0; i < ATH6KL_RC_MAX_CHAN_RECORD; i++) {
		p2p_rc->chan_record[i].best_snr = P2P_RC_NULL_SNR;
		p2p_rc->chan_record[i].aver_snr = P2P_RC_NULL_SNR;
	}

	if (p2p_rc->flags & ATH6KL_RC_FLAGS_CHAN_RECORD_FETCHED) {
		spin_unlock_bh(&p2p_rc->p2p_rc_lock);

		ath6kl_dbg(ATH6KL_DBG_RC,
			"p2p_rc chan already fetched, chan_record_cnt %d\n",
			p2p_rc->chan_record_cnt);

		return;
	}
	spin_unlock_bh(&p2p_rc->p2p_rc_lock);

	ath6kl_p2p_rc_fetch_chan(p2p_rc->ar);

	return;
}

void ath6kl_p2p_rc_scan_start(struct ath6kl_vif *vif, bool local_scan)
{
	struct ath6kl_p2p_rc_info *p2p_rc = vif->ar->p2p_rc_info_ctx;

	if (!p2p_rc)
		return;

	/*
	 * Assume that
	 * 1.only one scan behavior between all VIFs at the same time.
	 * 2.trust vif->scanband_type if this is a user scan.
	 * 3.if this is a driver scan and always learn it.
	 */

	/* Only full-scan is valid. */
	if ((local_scan) ||
	    (test_bit(SCANNING, &vif->flags) &&
	     (vif->scanband_type == SCANBAND_TYPE_ALL))) {
		/* Reset the channel record. */
		_p2p_rc_reset_chan_record(p2p_rc);

		p2p_rc->flags |= ATH6KL_RC_FLAGS_NEED_UPDATED;
		p2p_rc->last_update = jiffies;
	}

	ath6kl_dbg(ATH6KL_DBG_RC,
			"p2p_rc scan_start %s, chan %d %s type %d cnt %d\n",
				(p2p_rc->flags & ATH6KL_RC_FLAGS_NEED_UPDATED ?
					"need-update" : ""),
				(vif->scan_req ?
					vif->scan_req->n_channels : -1),
				(local_scan ? "local-scan" : ""),
				vif->scanband_type,
				p2p_rc->chan_record_cnt);

	return;
}

int ath6kl_p2p_rc_scan_complete_event(struct ath6kl_vif *vif, bool aborted)
{
	struct ath6kl_p2p_rc_info *p2p_rc = vif->ar->p2p_rc_info_ctx;
	int i;

	if (!p2p_rc)
		return 0;

	p2p_rc->flags &= ~ATH6KL_RC_FLAGS_NEED_UPDATED;

	/* Only flush the last_p2p_rc if it's a successful scan. */
	spin_lock_bh(&p2p_rc->p2p_rc_lock);
	if (!aborted) {
		p2p_rc->flags &= ~ATH6KL_RC_FLAGS_DONE;
		for (i = 0; i < P2P_RC_TYPE_MAX; i++)
			p2p_rc->last_p2p_rc[i] = NULL;
	}
	spin_unlock_bh(&p2p_rc->p2p_rc_lock);

	ath6kl_dbg(ATH6KL_DBG_RC,
			"p2p_rc scan_comp %s\n",
			(aborted ? "aborted" : ""));

	return 0;
}

static inline void _p2p_rc_bss_update(struct ath6kl_p2p_rc_info *p2p_rc,
					struct ieee80211_channel *channel,
					u8 snr)
{
	int slot;

	slot = __p2p_rc_get_chan_slot(channel);
	if ((slot >= 0) && (slot < ATH6KL_RC_MAX_CHAN_RECORD)) {
		struct p2p_rc_chan_record *p2p_rc_chan;

		spin_lock_bh(&p2p_rc->p2p_rc_lock);
		p2p_rc_chan = &(p2p_rc->chan_record[slot]);

		if (channel != p2p_rc_chan->channel) {
			spin_unlock_bh(&p2p_rc->p2p_rc_lock);
			ath6kl_dbg(ATH6KL_DBG_RC,
				   "p2p_rc bssinfo chan unsync! rx %d rc %d\n",
				   channel->center_freq,
				   p2p_rc_chan->channel->center_freq);
			return;
		}

		BUG_ON(!p2p_rc_chan->channel);

		ath6kl_dbg(ATH6KL_DBG_RC,
			   "p2p_rc bssinfo %s freq %d snr %d slot %d\n",
			   ((p2p_rc_chan->best_snr != P2P_RC_NULL_SNR) ?
							"update" : "found"),
			   channel->center_freq,
			   snr,
			   slot);

		if (snr > P2P_RC_MAX_SNR)
			snr = P2P_RC_MAX_SNR;

		/* Only keep the max. SNR for calculation. */
		if (p2p_rc_chan->best_snr != P2P_RC_NULL_SNR) {
			if (snr > p2p_rc_chan->best_snr)
				p2p_rc_chan->best_snr = snr;
		} else
			p2p_rc_chan->best_snr = snr;
		spin_unlock_bh(&p2p_rc->p2p_rc_lock);
	} else
		ath6kl_err("p2p_rc bssinfo invalid frequency %d!\n",
				channel->center_freq);

	return;
}

void ath6kl_p2p_rc_bss_info(struct ath6kl_vif *vif,
			    u8 snr,
			    struct ieee80211_channel *channel)
{
	struct ath6kl_p2p_rc_info *p2p_rc = vif->ar->p2p_rc_info_ctx;

	if (!p2p_rc)
		return;

	if (p2p_rc->flags & ATH6KL_RC_FLAGS_NEED_UPDATED)
		_p2p_rc_bss_update(p2p_rc, channel, snr);

	return;
}

static inline u8 __p2p_rc_average_snr(struct p2p_rc_chan_record *chan[],
					int count)
{
	struct p2p_rc_chan_record *p2p_rc_chan;
	int i, aver_snr = 0, compensate = 0;

	for (i = 0; i < ATH6KL_RC_AVERAGE_CHAN_CNT; i++) {
		p2p_rc_chan = chan[i];
		if ((i < count) &&
		    (p2p_rc_chan->channel))
			aver_snr += p2p_rc_chan->best_snr;
		else
			aver_snr += P2P_RC_NULL_SNR;
	}

	/* To round off the aver_snr to get more accurate average. */
	if ((aver_snr % ATH6KL_RC_AVERAGE_CHAN_CNT) >
	    (ATH6KL_RC_AVERAGE_CHAN_CNT >> 1))
		compensate++;

	aver_snr = (aver_snr / ATH6KL_RC_AVERAGE_CHAN_CNT) + compensate;

	ath6kl_dbg(ATH6KL_DBG_RC,
			"p2p_rc get_aver freq %d aver_snr %d count %d/%d\n",
			(chan[0]->channel ? chan[0]->channel->center_freq : 0),
			aver_snr,
			count,
			ATH6KL_RC_AVERAGE_CHAN_CNT);

	return aver_snr;
}

static bool __p2p_rc_is_user_channel(struct ath6kl_p2p_rc_info *p2p_rc,
					struct ieee80211_channel *chan)
{
	enum p2p_rc_user_chan_type user_chan_type = p2p_rc->user_chan_type;
	int i;

	for (i = 0; i < ATH6KL_RC_MAX_CHAN_RECORD; i++) {
		if ((p2p_rc->user_chan_list[i]) &&
		    (chan->center_freq == p2p_rc->user_chan_list[i])) {
			if (user_chan_type == P2P_RC_USER_WHITE_CHAN)
				return true;
			else
				return false;
		}
	}

	if (user_chan_type == P2P_RC_USER_WHITE_CHAN)
		return false;
	else
		return true;
}

static struct p2p_rc_chan_record *_p2p_rc_get_2g_chan(
	struct ath6kl_p2p_rc_info *p2p_rc,
	bool only_p2p_social,
	bool only_p2p,
	bool user_chan,
	bool no_lte)
{
	struct p2p_rc_chan_record *p2p_rc_chan, *best_p2p_rc_chan = NULL;
	struct p2p_rc_chan_record *adj_chan[ATH6KL_RC_AVERAGE_CHAN_CNT];
	struct ieee80211_channel *chan;
	int i, j, count;
	u8 average_h;

	BUG_ON((only_p2p_social && only_p2p));

	/* Start from 2G channel */
	p2p_rc_chan = &(p2p_rc->chan_record[0]);

	for (i = 0; i < ATH6KL_RC_MAX_2G_CHAN_RECORD; i++, p2p_rc_chan++) {
		chan = p2p_rc_chan->channel;
		if (chan) {
			if ((user_chan) &&
			    !__p2p_rc_is_user_channel(p2p_rc, chan))
				continue;
			else if ((only_p2p_social) &&
			    !ath6kl_p2p_is_social_channel(chan->center_freq))
				continue;
			else if ((only_p2p) &&
				 (chan->flags & (IEEE80211_CHAN_RADAR |
						 IEEE80211_CHAN_PASSIVE_SCAN |
						 IEEE80211_CHAN_NO_IBSS)) &&
				 !ath6kl_p2p_is_p2p_channel(chan->center_freq))
				continue;
			else if ((no_lte) &&
				 ath6kl_reg_is_lte_channel(p2p_rc->ar,
							 chan->center_freq))
				continue;

			/* Get adjacence channels in 20MHz width. */
			count = 0;
			adj_chan[count++] = p2p_rc_chan;
			for (j = 1; j <= ATH6KL_RC_AVERAGE_CHAN_OFFSET; j++) {
				if (chan->center_freq - (j * 5) >=
						ATH6KL_RC_AVERAGE_CHAN_START)
					adj_chan[count++] = p2p_rc_chan - j;
				if (chan->center_freq + (j * 5) <=
						ATH6KL_RC_AVERAGE_CHAN_END)
					adj_chan[count++] = p2p_rc_chan + j;
			}

			if ((count) &&
			    (chan->center_freq <= ATH6KL_RC_AVERAGE_CHAN_END))
				p2p_rc_chan->aver_snr = __p2p_rc_average_snr(
								adj_chan,
								count);
			else
				p2p_rc_chan->aver_snr = p2p_rc_chan->best_snr;

			if (best_p2p_rc_chan) {
				average_h = p2p_rc_chan->aver_snr;
				if (p2p_rc->flags & ATH6KL_RC_FLAGS_HIGH_CHAN)
					average_h++;

				if (average_h > best_p2p_rc_chan->aver_snr)
					best_p2p_rc_chan = p2p_rc_chan;
			} else
				best_p2p_rc_chan = p2p_rc_chan;
		}
	}

	ath6kl_dbg(ATH6KL_DBG_RC,
			"p2p_rc get_2g freq %d snr %d\n",
			(best_p2p_rc_chan ?
				best_p2p_rc_chan->channel->center_freq : 0),
			(best_p2p_rc_chan ?
				best_p2p_rc_chan->best_snr : 0));

	return best_p2p_rc_chan;
}

static struct p2p_rc_chan_record *_p2p_rc_get_5g_chan(
	struct ath6kl_p2p_rc_info *p2p_rc,
	bool only_p2p,
	bool no_dfs,
	bool user_chan)
{
	struct p2p_rc_chan_record *p2p_rc_chan, *best_p2p_rc_chan = NULL;
	struct ieee80211_channel *chan;
	int i, snr_h;

	/* Start from 5G channel */
	p2p_rc_chan = &(p2p_rc->chan_record[ATH6KL_RC_MAX_2G_CHAN_RECORD]);

	for (i = 0; i < ATH6KL_RC_MAX_5G_CHAN_RECORD; i++, p2p_rc_chan++) {
		chan = p2p_rc_chan->channel;
		if (chan) {
			if ((user_chan) &&
			    !__p2p_rc_is_user_channel(p2p_rc, chan))
				continue;
			else if ((only_p2p) &&
			    (chan->flags & (IEEE80211_CHAN_RADAR |
					    IEEE80211_CHAN_PASSIVE_SCAN |
					    IEEE80211_CHAN_NO_IBSS)) &&
			    !ath6kl_p2p_is_p2p_channel(chan->center_freq))
				continue;
			else if ((no_dfs) &&
				 (chan->flags & IEEE80211_CHAN_RADAR) &&
				 (p2p_rc_chan->best_snr == P2P_RC_NULL_SNR))
				continue;

			/* If DFS channel but any AP exist and still use it. */

			if (best_p2p_rc_chan) {
				snr_h = (int)p2p_rc_chan->best_snr;
				if (p2p_rc->flags & ATH6KL_RC_FLAGS_HIGH_CHAN)
					snr_h++;

				if (snr_h > (int)best_p2p_rc_chan->best_snr)
					best_p2p_rc_chan = p2p_rc_chan;
			} else
				best_p2p_rc_chan = p2p_rc_chan;
		}
	}

	ath6kl_dbg(ATH6KL_DBG_RC,
			"p2p_rc get_5g freq %d snr %d\n",
			(best_p2p_rc_chan ?
				best_p2p_rc_chan->channel->center_freq : 0),
			(best_p2p_rc_chan ?
				best_p2p_rc_chan->best_snr : 0));

	return best_p2p_rc_chan;
}

int ath6kl_p2p_rc_get(struct ath6kl *ar, struct ath6kl_rc_report *rc_report)
{
	struct ath6kl_p2p_rc_info *p2p_rc = ar->p2p_rc_info_ctx;
	struct p2p_rc_chan_record **p2p_rc_chan;
	struct p2p_rc_chan_record *user_chan_2g, *user_chan_5g;

	if (!p2p_rc)
		return -ENOTSUPP;

	/* Updating */
	if (p2p_rc->flags & ATH6KL_RC_FLAGS_NEED_UPDATED)
		return -EINPROGRESS;

	if ((p2p_rc->flags & ATH6KL_RC_FLAGS_ALWAYS_FRESH) &&
	    time_after(jiffies, p2p_rc->last_update + ATH6KL_RC_FRESH_TIME))
		return -EAGAIN;

	spin_lock_bh(&p2p_rc->p2p_rc_lock);

	p2p_rc_chan = &(p2p_rc->last_p2p_rc[0]);

	/*
	 * No further channel information need to be parsed and just
	 * return the last results.
	 */
	if (p2p_rc->flags & ATH6KL_RC_FLAGS_DONE)
		goto done;

	/* Get 2G recommand channel */
	p2p_rc_chan[P2P_RC_TYPE_2GALL] = _p2p_rc_get_2g_chan(p2p_rc,
								false,
								false,
								false,
								false);
	/* Get 5G recommand channel */
	p2p_rc_chan[P2P_RC_TYPE_5GALL] = _p2p_rc_get_5g_chan(p2p_rc,
								false,
								false,
								false);

	/* Get overall recommand channel. */
	if (p2p_rc_chan[P2P_RC_TYPE_2GALL]) {
		if (p2p_rc_chan[P2P_RC_TYPE_5GALL]) {
			if (p2p_rc_chan[P2P_RC_TYPE_2GALL]->aver_snr >
			    (p2p_rc_chan[P2P_RC_TYPE_5GALL]->best_snr +
			     p2p_rc->snr_compensation))
				p2p_rc_chan[P2P_RC_TYPE_OVERALL] =
						p2p_rc_chan[P2P_RC_TYPE_2GALL];
			else	/* Prefer to use 5G if has the same value. */
				p2p_rc_chan[P2P_RC_TYPE_OVERALL] =
						p2p_rc_chan[P2P_RC_TYPE_5GALL];
		} else
			p2p_rc_chan[P2P_RC_TYPE_OVERALL] =
					p2p_rc_chan[P2P_RC_TYPE_2GALL];
	} else if (p2p_rc_chan[P2P_RC_TYPE_5GALL])
		p2p_rc_chan[P2P_RC_TYPE_OVERALL] =
			p2p_rc_chan[P2P_RC_TYPE_5GALL];

	/* Get P2P-Social recommand channel */
	p2p_rc_chan[P2P_RC_TYPE_SOCAIL] = _p2p_rc_get_2g_chan(p2p_rc,
								true,
								false,
								false,
								false);

	/* Get P2P-2G recommand channel */
	p2p_rc_chan[P2P_RC_TYPE_2GP2P] = _p2p_rc_get_2g_chan(p2p_rc,
								false,
								true,
								false,
								false);

	/* Get P2P-5G recommand channel */
	p2p_rc_chan[P2P_RC_TYPE_5GP2P] = _p2p_rc_get_5g_chan(p2p_rc,
								true,
								true,
								false);

	/* Get P2P-All recommand channel. */
	if (p2p_rc_chan[P2P_RC_TYPE_2GP2P]) {
		if (p2p_rc_chan[P2P_RC_TYPE_5GP2P]) {
			if (p2p_rc_chan[P2P_RC_TYPE_2GP2P]->aver_snr >
			    (p2p_rc_chan[P2P_RC_TYPE_5GP2P]->best_snr +
			     p2p_rc->snr_compensation))
				p2p_rc_chan[P2P_RC_TYPE_ALLP2P] =
						p2p_rc_chan[P2P_RC_TYPE_2GP2P];
			else	/* Prefer to use 5G if has the same value. */
				p2p_rc_chan[P2P_RC_TYPE_ALLP2P] =
						p2p_rc_chan[P2P_RC_TYPE_5GP2P];
		} else
			p2p_rc_chan[P2P_RC_TYPE_ALLP2P] =
					p2p_rc_chan[P2P_RC_TYPE_2GP2P];
	} else if (p2p_rc_chan[P2P_RC_TYPE_5GP2P])
		p2p_rc_chan[P2P_RC_TYPE_ALLP2P] =
			p2p_rc_chan[P2P_RC_TYPE_5GP2P];

	/* Get 5G w/o DFS recommand channel */
	p2p_rc_chan[P2P_RC_TYPE_5GNODFS] = _p2p_rc_get_5g_chan(p2p_rc,
								false,
								true,
								false);

	/* Get overall w/o DFS recommand channel. */
	if (p2p_rc_chan[P2P_RC_TYPE_2GALL]) {
		if (p2p_rc_chan[P2P_RC_TYPE_5GNODFS]) {
			if (p2p_rc_chan[P2P_RC_TYPE_2GALL]->aver_snr >
			    (p2p_rc_chan[P2P_RC_TYPE_5GNODFS]->best_snr +
			     p2p_rc->snr_compensation))
				p2p_rc_chan[P2P_RC_TYPE_OVERALLNODFS] =
					p2p_rc_chan[P2P_RC_TYPE_2GALL];
			else	/* Prefer to use 5G if has the same value. */
				p2p_rc_chan[P2P_RC_TYPE_OVERALLNODFS] =
					p2p_rc_chan[P2P_RC_TYPE_5GNODFS];
		} else
			p2p_rc_chan[P2P_RC_TYPE_OVERALLNODFS] =
					p2p_rc_chan[P2P_RC_TYPE_2GALL];
	} else if (p2p_rc_chan[P2P_RC_TYPE_5GNODFS])
		p2p_rc_chan[P2P_RC_TYPE_OVERALLNODFS] =
			p2p_rc_chan[P2P_RC_TYPE_5GNODFS];

	/* Get 2G w/o LTE recommand channel */
	p2p_rc_chan[P2P_RC_TYPE_2GNOLTE] = _p2p_rc_get_2g_chan(p2p_rc,
								false,
								false,
								false,
								true);

	/* Get overall w/o LTE recommand channel. */
	if (p2p_rc_chan[P2P_RC_TYPE_2GNOLTE]) {
		if (p2p_rc_chan[P2P_RC_TYPE_5GALL]) {
			if (p2p_rc_chan[P2P_RC_TYPE_2GNOLTE]->aver_snr >
			    (p2p_rc_chan[P2P_RC_TYPE_5GALL]->best_snr +
			     p2p_rc->snr_compensation))
				p2p_rc_chan[P2P_RC_TYPE_OVERALLNOLTE] =
					p2p_rc_chan[P2P_RC_TYPE_2GNOLTE];
			else	/* Prefer to use 5G if has the same value. */
				p2p_rc_chan[P2P_RC_TYPE_OVERALLNOLTE] =
					p2p_rc_chan[P2P_RC_TYPE_5GALL];
		} else
			p2p_rc_chan[P2P_RC_TYPE_OVERALLNOLTE] =
					p2p_rc_chan[P2P_RC_TYPE_2GNOLTE];
	} else if (p2p_rc_chan[P2P_RC_TYPE_5GALL])
		p2p_rc_chan[P2P_RC_TYPE_OVERALLNOLTE] =
			p2p_rc_chan[P2P_RC_TYPE_5GALL];

	/* Get overall w/o LTE&DFS recommand channel. */
	if (p2p_rc_chan[P2P_RC_TYPE_2GNOLTE]) {
		if (p2p_rc_chan[P2P_RC_TYPE_5GNODFS]) {
			if (p2p_rc_chan[P2P_RC_TYPE_2GNOLTE]->aver_snr >
			    (p2p_rc_chan[P2P_RC_TYPE_5GNODFS]->best_snr +
			     p2p_rc->snr_compensation))
				p2p_rc_chan[P2P_RC_TYPE_OVERALLNOLTEDFS] =
					p2p_rc_chan[P2P_RC_TYPE_2GNOLTE];
			else	/* Prefer to use 5G if has the same value. */
				p2p_rc_chan[P2P_RC_TYPE_OVERALLNOLTEDFS] =
					p2p_rc_chan[P2P_RC_TYPE_5GNODFS];
		} else
			p2p_rc_chan[P2P_RC_TYPE_OVERALLNOLTEDFS] =
					p2p_rc_chan[P2P_RC_TYPE_2GNOLTE];
	} else if (p2p_rc_chan[P2P_RC_TYPE_5GNODFS])
		p2p_rc_chan[P2P_RC_TYPE_OVERALLNOLTEDFS] =
			p2p_rc_chan[P2P_RC_TYPE_5GNODFS];

	/* Get user-defined recommand channel. */
	user_chan_2g = _p2p_rc_get_2g_chan(p2p_rc,
						false,
						false,
						true,
						false);

	user_chan_5g = _p2p_rc_get_5g_chan(p2p_rc,
						false,
						false,
						true);

	if (user_chan_2g) {
		if (user_chan_5g) {
			if (user_chan_2g->aver_snr >
			    (user_chan_5g->best_snr + p2p_rc->snr_compensation))
				p2p_rc_chan[P2P_RC_TYPE_USER_CHAN] =
								user_chan_2g;
			else	/* Prefer to use 5G if has the same value. */
				p2p_rc_chan[P2P_RC_TYPE_USER_CHAN] =
								user_chan_5g;
		} else
			p2p_rc_chan[P2P_RC_TYPE_USER_CHAN] = user_chan_2g;
	} else if (user_chan_5g)
		p2p_rc_chan[P2P_RC_TYPE_USER_CHAN] = user_chan_5g;

	p2p_rc->flags |= ATH6KL_RC_FLAGS_DONE;

done:
	/* Get all results back to the caller */
	if (p2p_rc_chan[P2P_RC_TYPE_2GALL])
		rc_report->rc_2g =
		p2p_rc_chan[P2P_RC_TYPE_2GALL]->channel->center_freq;

	if (p2p_rc_chan[P2P_RC_TYPE_5GALL])
		rc_report->rc_5g =
		p2p_rc_chan[P2P_RC_TYPE_5GALL]->channel->center_freq;

	if (p2p_rc_chan[P2P_RC_TYPE_OVERALL])
		rc_report->rc_all =
		p2p_rc_chan[P2P_RC_TYPE_OVERALL]->channel->center_freq;

	if (p2p_rc_chan[P2P_RC_TYPE_SOCAIL])
		rc_report->rc_p2p_so =
		p2p_rc_chan[P2P_RC_TYPE_SOCAIL]->channel->center_freq;

	if (p2p_rc_chan[P2P_RC_TYPE_2GP2P])
		rc_report->rc_p2p_2g =
		p2p_rc_chan[P2P_RC_TYPE_2GP2P]->channel->center_freq;

	if (p2p_rc_chan[P2P_RC_TYPE_5GP2P])
		rc_report->rc_p2p_5g =
		p2p_rc_chan[P2P_RC_TYPE_5GP2P]->channel->center_freq;

	if (p2p_rc_chan[P2P_RC_TYPE_ALLP2P])
		rc_report->rc_p2p_all =
		p2p_rc_chan[P2P_RC_TYPE_ALLP2P]->channel->center_freq;

	if (p2p_rc_chan[P2P_RC_TYPE_5GNODFS])
		rc_report->rc_5g_nodfs =
		p2p_rc_chan[P2P_RC_TYPE_5GNODFS]->channel->center_freq;

	if (p2p_rc_chan[P2P_RC_TYPE_OVERALLNODFS])
		rc_report->rc_all_nodfs =
		p2p_rc_chan[P2P_RC_TYPE_OVERALLNODFS]->channel->center_freq;

	if (p2p_rc_chan[P2P_RC_TYPE_2GNOLTE])
		rc_report->rc_2g_nolte =
		p2p_rc_chan[P2P_RC_TYPE_2GNOLTE]->channel->center_freq;

	if (p2p_rc_chan[P2P_RC_TYPE_OVERALLNOLTE])
		rc_report->rc_all_nolte =
		p2p_rc_chan[P2P_RC_TYPE_OVERALLNOLTE]->channel->center_freq;

	if (p2p_rc_chan[P2P_RC_TYPE_OVERALLNOLTEDFS])
		rc_report->rc_all_noltedfs =
		p2p_rc_chan[P2P_RC_TYPE_OVERALLNOLTEDFS]->channel->center_freq;

	if (p2p_rc_chan[P2P_RC_TYPE_USER_CHAN])
		rc_report->rc_user_chan =
		p2p_rc_chan[P2P_RC_TYPE_USER_CHAN]->channel->center_freq;

	ath6kl_dbg(ATH6KL_DBG_RC,
			"p2p_rc get\n");

	spin_unlock_bh(&p2p_rc->p2p_rc_lock);

	return 0;
}

int ath6kl_p2p_rc_dump(struct ath6kl *ar, u8 *buf, int buf_len)
{
	struct ath6kl_p2p_rc_info *p2p_rc = ar->p2p_rc_info_ctx;
	struct p2p_rc_chan_record *p2p_rc_chan;
	struct ath6kl_rc_report rc_report;
	int i, idx = 0, len = 0;

	if ((!p2p_rc) || (!buf))
		return 0;

	len += snprintf(buf + len, buf_len - len,
			"\nflags 0x%x snr_comp %d last_update %lld now %lld\n",
			p2p_rc->flags,
			p2p_rc->snr_compensation,
			(long long int)p2p_rc->last_update,
			(long long int)jiffies);

	memset(&rc_report, 0, sizeof(struct ath6kl_rc_report));
	if (ath6kl_p2p_rc_get(ar, &rc_report) != 0)
		return 0;

	len += snprintf(buf + len, buf_len - len,
		"\n2G/5G/ALL %d %d %d\n",
		rc_report.rc_2g,
		rc_report.rc_5g,
		rc_report.rc_all);

	len += snprintf(buf + len, buf_len - len,
		"P2P-Social/2G/5G/ALL %d %d %d %d\n",
		rc_report.rc_p2p_so,
		rc_report.rc_p2p_2g,
		rc_report.rc_p2p_5g,
		rc_report.rc_p2p_all);

	len += snprintf(buf + len, buf_len - len,
		"NoDFS-5G/ALL %d %d\n",
		rc_report.rc_5g_nodfs,
		rc_report.rc_all_nodfs);

	len += snprintf(buf + len, buf_len - len,
		"NoLTE-2G/ALL %d %d\n",
		rc_report.rc_2g_nolte,
		rc_report.rc_all_nolte);

	len += snprintf(buf + len, buf_len - len,
		"NoLTE-NoDFS-ALL %d\n",
		rc_report.rc_all_noltedfs);

	len += snprintf(buf + len, buf_len - len,
		"UserChan %d\n",
		rc_report.rc_user_chan);

	len += snprintf(buf + len, buf_len - len,
		"\nUserChan Type - %s\nUserChan List -",
		(p2p_rc->user_chan_type == P2P_RC_USER_BLACK_CHAN) ?
			"Black List" : "White List");

	for (i = 0; i < ATH6KL_RC_MAX_CHAN_RECORD; i++) {
		if (p2p_rc->user_chan_list[i])
			len += snprintf(buf + len, buf_len - len,
				" %d",
				p2p_rc->user_chan_list[i]);
	}

	spin_lock_bh(&p2p_rc->p2p_rc_lock);
	len += snprintf(buf + len, buf_len - len,
			"\n\nchan_record %d\n",
			p2p_rc->chan_record_cnt);
	for (i = 0, p2p_rc_chan = p2p_rc->chan_record;
	     i < ATH6KL_RC_MAX_CHAN_RECORD;
	     i++, p2p_rc_chan++) {
		if (p2p_rc_chan->channel)
			len += snprintf(buf + len, buf_len - len,
					"%02d - %4d SNR %3d AVER_SNR %3d\n",
					idx,
					p2p_rc_chan->channel->center_freq,
					(p2p_rc_chan->best_snr ==
						P2P_RC_NULL_SNR) ?
						-1 : p2p_rc_chan->best_snr,
					(p2p_rc_chan->aver_snr ==
						P2P_RC_NULL_SNR) ?
						-1 : p2p_rc_chan->aver_snr);
		idx++;
	}
	spin_unlock_bh(&p2p_rc->p2p_rc_lock);

	return len;
}

void ath6kl_p2p_rc_config(struct ath6kl *ar, u16 freq, int type)
{
	struct ath6kl_p2p_rc_info *p2p_rc = ar->p2p_rc_info_ctx;
	int i = -1, j;
	bool need_update = false;

	if (freq) {
		for (i = 0; i < ATH6KL_RC_MAX_CHAN_RECORD; i++) {
			if (p2p_rc->user_chan_list[i]) {
				if (p2p_rc->user_chan_list[i] == freq)
					break;
			} else {
				need_update = true;
				p2p_rc->user_chan_list[i] = freq;
				break;
			}
		}

		if (i == ATH6KL_RC_MAX_CHAN_RECORD)
			ath6kl_err("p2p_rc user chan list full!\n");
	} else {
		/* change the type */
		if (p2p_rc->user_chan_type != type) {
			need_update = true;
			p2p_rc->user_chan_type = type;

			/* reset the channel list */
			memset(p2p_rc->user_chan_list,
				0,
				(sizeof(u16) * ATH6KL_RC_MAX_CHAN_RECORD));
		}
	}

	/* Re-calculate again */
	if (need_update) {
		p2p_rc->flags &= ~ATH6KL_RC_FLAGS_DONE;
		for (j = 0; j < P2P_RC_TYPE_MAX; j++)
			p2p_rc->last_p2p_rc[j] = NULL;
	}

	ath6kl_dbg(ATH6KL_DBG_RC,
			"p2p_rc config freq %d type %d, slot %d %s\n",
			freq,
			type,
			i,
			(need_update ? "UPDATED" : ""));

	return;
}

bool ath6kl_p2p_frame_retry(struct ath6kl *ar, u8 *frm, int len)
{
	struct ieee80211_p2p_action_public *action_frame =
		(struct ieee80211_p2p_action_public *)frm;

	if (!ar->p2p_frame_retry)
		return false;

	/*
	 * WAR : Except P2P-Neg-Confirm frame and other P2P action frames
	 *       could be recovery by supplicant's state machine.
	 */
	if (len < 8)
		return false;

	return ((action_frame->category == WLAN_CATEGORY_PUBLIC) &&
		(action_frame->action_code ==
					WLAN_PUB_ACTION_VENDER_SPECIFIC) &&
		(action_frame->oui == cpu_to_be32((WLAN_OUI_WFA << 8) |
						  (WLAN_OUI_TYPE_WFA_P2P))) &&
		(action_frame->action_subtype == WLAN_P2P_GO_NEG_CONF));
}

bool ath6kl_p2p_is_p2p_frame(struct ath6kl *ar, const u8 *frm, size_t len)
{
	struct ieee80211_mgmt *action = (struct ieee80211_mgmt *)frm;
	struct ieee80211_p2p_action_public *action_public;
	struct ieee80211_p2p_action_vendor *action_vendor;
	u8 *action_start = (u8 *)(&action->u.action);

	if (len < sizeof(struct ieee80211_p2p_action_vendor))
		return false;

	action_public = (struct ieee80211_p2p_action_public *)action_start;
	if ((action_public->category == WLAN_CATEGORY_PUBLIC) &&
		(action_public->action_code ==
					WLAN_PUB_ACTION_VENDER_SPECIFIC) &&
		(action_public->oui == cpu_to_be32((WLAN_OUI_WFA << 8) |
						   (WLAN_OUI_TYPE_WFA_P2P))))
		return true;

	action_vendor = (struct ieee80211_p2p_action_vendor *)action_start;
	if ((action_vendor->category == WLAN_CATEGORY_VENDOR_SPECIFIC) &&
		(action_vendor->oui == cpu_to_be32((WLAN_OUI_WFA << 8) |
						   (WLAN_OUI_TYPE_WFA_P2P))))
		return true;

	return false;
}

void ath6kl_p2p_connect_event(struct ath6kl_vif *vif,
				u8 beacon_ie_len,
				u8 assoc_req_len,
				u8 assoc_resp_len,
				u8 *assoc_info)
{
	u8 *pie, *peie;
	struct ieee80211_ht_cap *ht_cap_ie = NULL;
	bool vendor_spec_ie_intel = false;

	if (vif->nw_type != INFRA_NETWORK)
		return;


	/* Now, only p2p_war_bad_intel_go need to do something here. */
	if (!vif->ar->p2p_war_bad_intel_go)
		return;

	/* AssocResp IEs */
	pie = assoc_info + beacon_ie_len + assoc_req_len +
		(sizeof(u16) * 3); /* capinfo + status code + associd */
	peie = assoc_info + beacon_ie_len + assoc_req_len + assoc_resp_len;

	while (pie < peie) {
		switch (*pie) {
		case WLAN_EID_HT_CAPABILITY:
			if (pie[1] >= sizeof(struct ieee80211_ht_cap))
				ht_cap_ie =
					(struct ieee80211_ht_cap *)(pie + 2);
			break;
		case WLAN_EID_VENDOR_SPECIFIC:
			if (pie[1] > 0) {
				if (pie[1] == 0x0b &&
				     pie[2] == 0x00 &&
				     pie[3] == 0x17 &&
				     pie[4] == 0x35 &&
				     pie[5] == 0x01)
					vendor_spec_ie_intel = true;
			}
			break;
		}
		pie += pie[1] + 2;
	}

	/* WAR EV119712 */
	if (ht_cap_ie &&
	    vendor_spec_ie_intel &&
	    (vif->ar->target_subtype & TARGET_SUBTYPE_2SS)) {
		if (((ht_cap_ie->cap_info & IEEE80211_HT_CAP_SM_PS) ==
		     (WLAN_HT_CAP_SM_PS_DYNAMIC <<
					IEEE80211_HT_CAP_SM_PS_SHIFT)) &&
		    (ht_cap_ie->mcs.rx_mask[1])) {
			ath6kl_info("Enable Intel-GO compatibility.\n");
			ath6kl_wmi_set_fix_rates(vif->ar->wmi,
						vif->fw_vif_idx,
						(0x00000000000fffffULL));

			set_bit(PS_STICK, &vif->flags);
			ath6kl_wmi_powermode_cmd(vif->ar->wmi,
						vif->fw_vif_idx,
						MAX_PERF_POWER);
		}
	}

	return;
}


void ath6kl_p2p_reconfig_ps(struct ath6kl *ar,
			bool mcc,
			bool call_on_disconnect)
{
	struct ath6kl_vif *vif;
	u8 pwr_mode = REC_POWER;
	int connected = 0;

	/* Not support PS in MCC mode currently. */

	list_for_each_entry(vif, &ar->vif_list, list) {
		if (test_bit(CONNECTED, &vif->flags)) {
			if (mcc) {
				/* MCC/AnyVIF - Set all VIFs to PS OFF. */
				set_bit(PS_STICK, &vif->flags);
				pwr_mode = MAX_PERF_POWER;
			} else if (vif->wdev.iftype ==
						NL80211_IFTYPE_P2P_GO) {
				/* SCC/P2P-GO - Set to PS OFF. */
				set_bit(PS_STICK, &vif->flags);
				pwr_mode = MAX_PERF_POWER;
			} else if (vif->wdev.iftype ==
						NL80211_IFTYPE_P2P_CLIENT) {
				/* SCC/P2P-GC - Set to PS OFF if need. */
				if ((ar->p2p_war_p2p_client_awake) &&
				    connected) {
					set_bit(PS_STICK, &vif->flags);
					pwr_mode = MAX_PERF_POWER;
				} else {
					/* TODO: For WAR EV119712 case */
					clear_bit(PS_STICK, &vif->flags);
					if (vif->wdev.ps == NL80211_PS_ENABLED)
						pwr_mode = REC_POWER;
					else
						pwr_mode = MAX_PERF_POWER;
				}
			} else {
				/* SCC/notP2P-VIF - Back to original PS mode. */
				clear_bit(PS_STICK, &vif->flags);
				if (vif->nw_type == AP_NETWORK)
					pwr_mode = MAX_PERF_POWER;
				else {	/* Ad-Hoc & STA */
					if (vif->wdev.ps == NL80211_PS_ENABLED)
						pwr_mode = REC_POWER;
					else
						pwr_mode = MAX_PERF_POWER;
				}
			}
			connected++;

			ath6kl_dbg(ATH6KL_DBG_INFO,
				"PS vif %d ps %d-%d conn %d %s %s => %s\n",
				vif->fw_vif_idx,
				vif->last_pwr_mode,
				vif->wdev.ps,
				connected,
				(mcc ? "MCC" : "SCC"),
				(call_on_disconnect ? "DISCONN" : "CONN"),
				(pwr_mode == REC_POWER ? "ON" : "OFF"));

			ath6kl_wmi_powermode_cmd(ar->wmi,
						vif->fw_vif_idx,
						pwr_mode);
		}
	}

	return;
}

static void _p2p_pending_connect_work(struct work_struct *work)
{
	struct ath6kl_vif *vif = NULL;
	struct p2p_pending_connect_info *pending_connect_info = NULL;
	const u8 *bssid, *req_ie, *resp_ie;

	if (!work)
		goto err;

	vif = container_of(work, struct ath6kl_vif,
		work_pending_connect.work);
	if (!vif)
		goto err;

	pending_connect_info = vif->pending_connect_info;
	if (!vif->pending_connect_info)
		goto err;

	bssid = req_ie = resp_ie = NULL;
	if (!is_zero_ether_addr(pending_connect_info->bssid))
		bssid = pending_connect_info->bssid;
	if (pending_connect_info->req_ie_len)
		req_ie = pending_connect_info->req_ie;
	if (pending_connect_info->resp_ie_len)
		resp_ie = pending_connect_info->resp_ie;

	/* Send connect event to cfg80211 */
	cfg80211_connect_result(vif->ndev,
				bssid,
				req_ie,
				pending_connect_info->req_ie_len,
				resp_ie,
				pending_connect_info->resp_ie_len,
				pending_connect_info->status,
				pending_connect_info->gfp);

	kfree(vif->pending_connect_info);
	vif->pending_connect_info = NULL;

	return;
err:
	ath6kl_err("P2P pending connect work fail! vif %p work %p info %p\n",
			vif, work, pending_connect_info);

	return;
}

static inline bool _p2p_need_pending_connect(struct ath6kl_vif *vif,
						const u8 *bssid)
{
	bool need_pending = false;

	/* WAR CR468120 */
	if ((vif->ar->p2p_war_bad_broadcom_go) &&
	    (vif->sme_state == SME_CONNECTED) &&
	    (vif->connect_ctrl_flags & CONNECT_WPS_FLAG) &&
	    (vif->wdev.iftype == NL80211_IFTYPE_P2P_CLIENT)) {
		struct cfg80211_bss *bss;

		bss = ath6kl_bss_get(vif->ar,
				NULL,
				vif->bssid,
				vif->ssid,
				vif->ssid_len,
				WLAN_CAPABILITY_ESS,
				WLAN_CAPABILITY_ESS);
		if (bss) {
			u8 *pie, *peie;

			pie = peie = NULL;

#ifdef CFG80211_SAFE_BSS_INFO_ACCESS
			rcu_read_lock();
			if (bss->ies) {
				pie = (u8 *)(bss->ies->data);
				peie = pie + bss->ies->len;
			}
#else
			if (bss->information_elements) {
				pie = bss->information_elements;
				peie = pie + bss->len_information_elements;
			}
#endif

			while (pie < peie) {
				if (*pie == WLAN_EID_VENDOR_SPECIFIC) {
					if ((pie[1] > 0) &&
					    (pie[2] == 0x00 &&
					     pie[3] == 0x10 &&
					     pie[4] == 0x18))
						need_pending = true;
				}
				pie += pie[1] + 2;
			}

#ifdef CFG80211_SAFE_BSS_INFO_ACCESS
			rcu_read_unlock();
#endif
			ath6kl_bss_put(vif->ar, bss);
		}
	}

	return need_pending;
}

static inline bool _p2p_flush_pending_connect(struct ath6kl_vif *vif)
{
	if ((vif->ar->p2p_war_bad_broadcom_go) &&
	    (vif->sme_state == SME_CONNECTED) &&
	    (vif->wdev.iftype == NL80211_IFTYPE_P2P_CLIENT) &&
	    (vif->pending_connect_info)) {
		ath6kl_info("Flush pending connect work first.\n");
		ath6kl_flush_pend_skb(vif);
	}

	return false;
}

bool ath6kl_p2p_pending_connect_event(struct ath6kl_vif *vif,
					const u8 *bssid,
					const u8 *req_ie,
					size_t req_ie_len,
					const u8 *resp_ie,
					size_t resp_ie_len,
					u16 status,
					gfp_t gfp)
{
#define _P2P_PENDING_CONNECT_TIME	800	/* in ms. */
	struct p2p_pending_connect_info *pending_connect_info;

	if (_p2p_need_pending_connect(vif, bssid)) {
		if ((req_ie_len > ATH6KL_P2P_MAX_PENDING_INFO_IELEN) ||
		    (resp_ie_len > ATH6KL_P2P_MAX_PENDING_INFO_IELEN))
			return false;

		vif->pending_connect_info =
			kzalloc(sizeof(struct p2p_pending_connect_info),
				GFP_ATOMIC);
		if (vif->pending_connect_info == NULL)
			return false;

		pending_connect_info = vif->pending_connect_info;
		if (bssid)
			memcpy(pending_connect_info->bssid, bssid, ETH_ALEN);
		if (req_ie) {
			memcpy(pending_connect_info->req_ie,
				req_ie,
				req_ie_len);
			pending_connect_info->req_ie_len = req_ie_len;
		}
		if (resp_ie) {
			memcpy(pending_connect_info->resp_ie,
				resp_ie,
				resp_ie_len);
			pending_connect_info->resp_ie_len = resp_ie_len;
		}
		pending_connect_info->status = status;
		pending_connect_info->gfp = gfp;

		INIT_DELAYED_WORK(&vif->work_pending_connect,
				_p2p_pending_connect_work);
		schedule_delayed_work(&vif->work_pending_connect,
				(msecs_to_jiffies(_P2P_PENDING_CONNECT_TIME)));

		ath6kl_info("Enable Broadcom-GO compatibility, delay %d ms.\n",
				_P2P_PENDING_CONNECT_TIME);

		return true;
	} else
		return false;
#undef _P2P_PENDING_CONNECT_TIME
}

void ath6kl_p2p_pending_disconnect_event(struct ath6kl_vif *vif,
					u16 reason,
					u8 *ie,
					size_t ie_len,
					gfp_t gfp)
{
	/* Flush pending work first if already fired. */
	_p2p_flush_pending_connect(vif);

	return;
}

bool ath6kl_p2p_ie_append(struct ath6kl_vif *vif, u8 mgmt_frame_type)
{
	struct ath6kl *ar = vif->ar;

	/*
	 * IOT : Some older APs' implementation may reject connection if
	 *       concuurrent STA interface include P2P IEs even these APs
	 *       don't understand P2P IEs.
	 */
	if (ar->p2p_concurrent &&
	    ar->p2p_dedicate &&
	    (vif->fw_vif_idx == 0) &&
	    (vif->nw_type == INFRA_NETWORK) &&
	    (ar->p2p_ie_not_append & mgmt_frame_type))
		return false;

	return true;
}

/* P802.11REVmb */
static struct p2p_oper_chan ath6kl_p2p_oper_chan[] = {
	{ 81,  2412, 2472, 5,  P2P_OPER_CHAN_BW_HT20},       /* CH1   - CH13 */
	{ 115, 5180, 5240, 20, P2P_OPER_CHAN_BW_HT20},       /* CH36  - CH48 */
	{ 116, 5180, 5220, 20, P2P_OPER_CHAN_BW_HT40_PLUS},  /* CH36  - CH44 */
	{ 117, 5200, 5240, 20, P2P_OPER_CHAN_BW_HT40_MINUS}, /* CH40  - CH48 */
	{ 124, 5745, 5805, 20, P2P_OPER_CHAN_BW_HT20},       /* CH149 - CH161 */
	{ 125, 5745, 5825, 20, P2P_OPER_CHAN_BW_HT20},       /* CH149 - CH165 */
	{ 126, 5745, 5785, 20, P2P_OPER_CHAN_BW_HT40_PLUS},  /* CH149 - CH157 */
	{ 127, 5765, 5805, 20, P2P_OPER_CHAN_BW_HT40_MINUS}, /* CH153 - CH161 */
	{ 0, 0, 0, 0, P2P_OPER_CHAN_BW_NULL},
};

bool ath6kl_p2p_is_p2p_channel(u32 freq)
{
	struct p2p_oper_chan *p2p_oper_chan_map, *p2p_oper_chan;
	u32 op_class, p2p_freq;

	p2p_oper_chan_map = ath6kl_p2p_oper_chan;

	for (op_class = 0; p2p_oper_chan_map[op_class].oper_class; op_class++) {
		p2p_oper_chan  = &p2p_oper_chan_map[op_class];
		for (p2p_freq = p2p_oper_chan->min_chan_freq;
		     p2p_freq <= p2p_oper_chan->max_chan_freq;
		     p2p_freq += p2p_oper_chan->inc_freq) {
			if (freq == p2p_freq) {
				/* TODO : check bandwidth */
				return true;
			}
		}
	}

	return false;
}

bool ath6kl_p2p_is_social_channel(u32 freq)
{
	if ((freq == 2412) ||
	    (freq == 2437) ||
	    (freq == 2462))
		return true;

	return false;
}

static int _p2p_build_scan_chan(struct ath6kl *ar, u16 *chan_list)
{
	struct wiphy *wiphy = ar->wiphy;
	struct ieee80211_supported_band *sband;
	struct ieee80211_channel *chan;
	int i, num_chan = 0;

	sband = wiphy->bands[NL80211_BAND_2GHZ];
	for (i = 0; i < sband->n_channels; i++) {
		chan = &sband->channels[i];
		if (!(chan->flags & IEEE80211_CHAN_DISABLED) &&
		    ath6kl_p2p_is_p2p_channel(chan->center_freq))
			chan_list[num_chan++] = chan->center_freq;
	}

	sband = wiphy->bands[NL80211_BAND_5GHZ];
	for (i = 0; i < sband->n_channels; i++) {
		chan = &sband->channels[i];
		if (!(chan->flags & IEEE80211_CHAN_DISABLED) &&
		    ath6kl_p2p_is_p2p_channel(chan->center_freq))
			chan_list[num_chan++] = chan->center_freq;
	}

	return num_chan;
}

int ath6kl_p2p_build_scan_chan(struct ath6kl_vif *vif,
				u32 req_chan_num,
				u16 *chan_list)
{
#define _P2P_WISE_FULL_SCAN_CNT	(15)
	struct ath6kl *ar = vif->ar;
	bool full_chan_scan = false;

	/*
	 * If this's P2P-Device's scan w/ only P2P-Social channel and assume
	 * the user is in P2P searching state and will insert a full channel
	 * scan every _P2P_WISE_FULL_SCAN_CNT time.
	 */
	if (ar->p2p_wise_scan) {
		if (ar->p2p_dedicate &&
		    (vif->fw_vif_idx == (ar->vif_max - 1))) {
			if (req_chan_num > 3)
				vif->p2p_wise_full_scan = 0;
			else if (req_chan_num == 3)
				vif->p2p_wise_full_scan++;

			if (vif->p2p_wise_full_scan > _P2P_WISE_FULL_SCAN_CNT) {
				full_chan_scan = true;
				vif->p2p_wise_full_scan = 0;
			}

			if (full_chan_scan)
				return _p2p_build_scan_chan(ar, chan_list);
		}
	}

	return 0;
#undef _P2P_WISE_FULL_SCAN_CNT
}

