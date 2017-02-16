//------------------------------------------------------------------------------
// Copyright (c) 2009-2010 Atheros Corporation.  All rights reserved.
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
//------------------------------------------------------------------------------
//==============================================================================
// HCI bridge implementation
//
// Author(s): ="Atheros"
//==============================================================================

#include "hci_transport_api.h"
#include "common_drv.h"

extern HCI_TRANSPORT_HANDLE (*_HCI_TransportAttach)(void *HTCHandle, HCI_TRANSPORT_CONFIG_INFO *pInfo);
extern void (*_HCI_TransportDetach)(HCI_TRANSPORT_HANDLE HciTrans);
extern A_STATUS    (*_HCI_TransportAddReceivePkts)(HCI_TRANSPORT_HANDLE HciTrans, HTC_PACKET_QUEUE *pQueue);
extern A_STATUS    (*_HCI_TransportSendPkt)(HCI_TRANSPORT_HANDLE HciTrans, HTC_PACKET *pPacket, A_BOOL Synchronous);
extern void        (*_HCI_TransportStop)(HCI_TRANSPORT_HANDLE HciTrans);
extern A_STATUS    (*_HCI_TransportStart)(HCI_TRANSPORT_HANDLE HciTrans);
extern A_STATUS    (*_HCI_TransportEnableDisableAsyncRecv)(HCI_TRANSPORT_HANDLE HciTrans, A_BOOL Enable);
extern A_STATUS    (*_HCI_TransportRecvHCIEventSync)(HCI_TRANSPORT_HANDLE HciTrans, 
                                          HTC_PACKET           *pPacket,
                                          int                  MaxPollMS);
extern A_STATUS    (*_HCI_TransportSetBaudRate)(HCI_TRANSPORT_HANDLE HciTrans, A_UINT32 Baud);
extern A_STATUS    (*_HCI_TransportEnablePowerMgmt)(HCI_TRANSPORT_HANDLE HciTrans, A_BOOL Enable);


#define HCI_TransportAttach(HTCHandle, pInfo)   \
            _HCI_TransportAttach((HTCHandle), (pInfo))
#define HCI_TransportDetach(HciTrans)    \
            _HCI_TransportDetach(HciTrans)
#define HCI_TransportAddReceivePkts(HciTrans, pQueue)   \
            _HCI_TransportAddReceivePkts((HciTrans), (pQueue))
#define HCI_TransportSendPkt(HciTrans, pPacket, Synchronous)  \
            _HCI_TransportSendPkt((HciTrans), (pPacket), (Synchronous))
#define HCI_TransportStop(HciTrans)  \
            _HCI_TransportStop((HciTrans))
#define HCI_TransportStart(HciTrans)  \
            _HCI_TransportStart((HciTrans))
#define HCI_TransportEnableDisableAsyncRecv(HciTrans, Enable)   \
            _HCI_TransportEnableDisableAsyncRecv((HciTrans), (Enable))
#define HCI_TransportRecvHCIEventSync(HciTrans, pPacket, MaxPollMS)   \
            _HCI_TransportRecvHCIEventSync((HciTrans), (pPacket), (MaxPollMS))
#define HCI_TransportSetBaudRate(HciTrans, Baud)    \
            _HCI_TransportSetBaudRate((HciTrans), (Baud))
#define HCI_TransportEnablePowerMgmt(HciTrans, Enable)    \
            _HCI_TransportEnablePowerMgmt((HciTrans), (Enable))


extern A_STATUS ar6000_register_hci_transport(HCI_TRANSPORT_CALLBACKS *hciTransCallbacks);

extern A_STATUS ar6000_get_hif_dev(HIF_DEVICE *device, void *config);

extern A_STATUS ar6000_set_uart_config(HIF_DEVICE *hifDevice, A_UINT32 scale, A_UINT32 step);

/* get core clock register settings
 * data: 0 - 40/44MHz
 *       1 - 80/88MHz
 *       where (5G band/2.4G band)
 * assume 2.4G band for now
 */
extern A_STATUS ar6000_get_core_clock_config(HIF_DEVICE *hifDevice, A_UINT32 *data);
