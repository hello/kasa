//------------------------------------------------------------------------------
// Copyright (c) 2004-2010 Atheros Communications Inc.
// All rights reserved.
//
// 
// The software source and binaries included in this development package are
// licensed, not sold. You, or your company, received the package under one
// or more license agreements. The rights granted to you are specifically
// listed in these license agreement(s). All other rights remain with Atheros
// Communications, Inc., its subsidiaries, or the respective owner including
// those listed on the included copyright notices.  Distribution of any
// portion of this package must be in strict compliance with the license
// agreement(s) terms.
// </copyright>
// 
// <summary>
// 	Wifi driver for AR6002
// </summary>
//
//
// Author(s): ="Atheros"
//------------------------------------------------------------------------------

#ifndef _AR6K_CFG80211_H_
#define _AR6K_CFG80211_H_

struct wireless_dev *ar6k_cfg80211_init(struct device *dev);
void ar6k_cfg80211_deinit(AR_SOFTC_DEV_T *arPriv);

void ar6k_cfg80211_scanComplete_event(AR_SOFTC_DEV_T *arPriv, A_STATUS status);

void ar6k_cfg80211_connect_event(AR_SOFTC_DEV_T *arPriv, A_UINT16 channel,
                                A_UINT8 *bssid, A_UINT16 listenInterval,
                                A_UINT16 beaconInterval,NETWORK_TYPE networkType,
                                A_UINT8 beaconIeLen, A_UINT8 assocReqLen,
                                A_UINT8 assocRespLen, A_UINT8 *assocInfo);

void ar6k_cfg80211_disconnect_event(AR_SOFTC_DEV_T *arPriv, A_UINT8 reason,
                                    A_UINT8 *bssid, A_UINT8 assocRespLen,
                                    A_UINT8 *assocInfo, A_UINT16 protocolReasonStatus);

void ar6k_cfg80211_tkip_micerr_event(AR_SOFTC_DEV_T *arPriv, A_UINT8 keyid, A_BOOL ismcast);

#endif /* _AR6K_CFG80211_H_ */






