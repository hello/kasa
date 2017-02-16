/*
 *
 * Copyright (c) 2004-2007 Atheros Communications Inc.
 * All rights reserved.
 *
 * 
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
// 	Wifi driver for AR6003
// </summary>
//
 *
 */

#ifndef _AR_H_
#define _AR_H_

#include "a_config.h"
#include "athdefs.h"
#include "a_types.h"
#include "a_osapi.h"
#include "htc_api.h"
#include "wmi.h"
#include "bmi.h"
#include "ar6000_api.h"
#include "netbuf.h"
#include "targaddrs.h"
#include "dbglog_api.h"
#include "ar6000_diag.h"
#include "common_drv.h"

#define MAX_DEVICE_ID_LEN       200

//#define ATH_MODULE_NAME driver
#include "a_debug.h"

#define  ATH_DEBUG_DBG_LOG       ATH_DEBUG_MAKE_MODULE_MASK(0)
#define  ATH_DEBUG_WLAN_CONNECT  ATH_DEBUG_MAKE_MODULE_MASK(1)
#define  ATH_DEBUG_WLAN_SCAN     ATH_DEBUG_MAKE_MODULE_MASK(2)
#define  ATH_DEBUG_WLAN_TX       ATH_DEBUG_MAKE_MODULE_MASK(3)
#define  ATH_DEBUG_WLAN_RX       ATH_DEBUG_MAKE_MODULE_MASK(4)
#define  ATH_DEBUG_HTC_RAW       ATH_DEBUG_MAKE_MODULE_MASK(5)
#define  ATH_DEBUG_HCI_BRIDGE    ATH_DEBUG_MAKE_MODULE_MASK(6)
#define  ATH_DEBUG_HCI_RECV      ATH_DEBUG_MAKE_MODULE_MASK(7)
#define  ATH_DEBUG_HCI_SEND      ATH_DEBUG_MAKE_MODULE_MASK(8)
#define  ATH_DEBUG_HCI_DUMP      ATH_DEBUG_MAKE_MODULE_MASK(9)

#ifndef  __dev_put
#define  __dev_put(dev) dev_put(dev)
#endif


#ifdef USER_KEYS

#define USER_SAVEDKEYS_STAT_INIT     0
#define USER_SAVEDKEYS_STAT_RUN      1

// TODO this needs to move into the AR_SOFTC struct
struct USER_SAVEDKEYS {
    struct ieee80211req_key   ucast_ik;
    struct ieee80211req_key   bcast_ik;
    CRYPTO_TYPE               keyType;
    A_BOOL                    keyOk;
};
#endif

#define DBG_INFO		0x00000001
#define DBG_ERROR		0x00000002
#define DBG_WARNING		0x00000004
#define DBG_SDIO		0x00000008
#define DBG_HIF			0x00000010
#define DBG_HTC			0x00000020
#define DBG_WMI			0x00000040
#define DBG_WMI2		0x00000080
#define DBG_DRIVER		0x00000100

#define DBG_DEFAULTS	(DBG_ERROR|DBG_WARNING)


A_STATUS ar6000_ReadRegDiag(HIF_DEVICE *hifDevice, A_UINT32 *address, A_UINT32 *data);
A_STATUS ar6000_WriteRegDiag(HIF_DEVICE *hifDevice, A_UINT32 *address, A_UINT32 *data);

#ifdef __cplusplus
extern "C" {
#endif

#define	MAX_AR6000                        1
#define AR6000_MAX_RX_BUFFERS             16
// TODO temporarily increase RX buffer size to accomodate 11n A-MSDU packets
#define AR6000_BUFFER_SIZE                (4096 + 128)

#define	AR6000_MAX_ENDPOINTS              4
#define MAX_COOKIE_NUM                    150

#define AR6003_REV2_VERSION               0x30000384 

/* HTC RAW streams */
typedef enum _HTC_RAW_STREAM_ID {
    HTC_RAW_STREAM_NOT_MAPPED = -1,
    HTC_RAW_STREAM_0 = 0,
    HTC_RAW_STREAM_1 = 1,
    HTC_RAW_STREAM_2 = 2,
    HTC_RAW_STREAM_3 = 3,
    HTC_RAW_STREAM_NUM_MAX
} HTC_RAW_STREAM_ID;

#define RAW_HTC_READ_BUFFERS_NUM    4
#define RAW_HTC_WRITE_BUFFERS_NUM   4

#define HTC_RAW_BUFFER_SIZE  1664

typedef struct {
    int currPtr;
    int length;
    unsigned char data[HTC_RAW_BUFFER_SIZE];
    HTC_PACKET    HTCPacket;
} raw_htc_buffer;

struct ar_cookie {
    A_UINT32               arc_bp[2];    /* Must be first field */
    HTC_PACKET             HtcPkt;       /* HTC packet wrapper */
    struct ar_cookie *arc_list_next;
};

struct net_device_stats {
    A_UINT32                rx_packets;
    A_UINT32                tx_packets; 
    A_UINT32                rx_bytes; 
    A_UINT32                tx_bytes; 
    A_UINT32                rx_errors; 
    A_UINT32                tx_errors; 
    A_UINT32                rx_dropped; 
    A_UINT32                tx_dropped; 
    A_UINT32                multicast; 
    A_UINT32                collisions; 
    A_UINT32                rx_length_errors;
    A_UINT32                rx_over_errors; 
    A_UINT32                rx_crc_errors; 
    A_UINT32                rx_frame_errors; 
    A_UINT32                rx_fifo_errors; 
    A_UINT32                rx_missed_errors; 
    A_UINT32                tx_aborted_errors; 
    A_UINT32                tx_carrier_errors; 
    A_UINT32                tx_fifo_errors; 
    A_UINT32                tx_heartbeat_errors; 
    A_UINT32                tx_window_errors; 
    A_UINT32                rx_compressed; 
    A_UINT32                tx_compressed; 
};

/* used by AR6000_IOCTL_WMI_GETREV */
struct ar6000_version {
    A_UINT32        host_ver;
    A_UINT32        target_ver;
    A_UINT32        wlan_ver;
    A_UINT32        abi_ver;
};

/*TODO: All this should move to OS independent dir */

typedef struct ar6_softc {
    HANDLE                  arDeviceHandle;
    int                     arTxPending[ENDPOINT_MAX];
    int                     arTotalTxDataPending;
    A_UINT8                 arNumDataEndPts;
    A_BOOL                  arConnected;
    HTC_HANDLE              arHtcTarget;
    void                    *arHifDevice;
    struct ar6000_version   arVersion;
    A_MUTEX_T               arLock;
    A_UINT32                arTargetType;
    struct net_device_stats arNetStats;
    struct ar_cookie        *arCookieList;
    HTC_ENDPOINT_ID         arControlEp;
    HTC_ENDPOINT_ID         arRaw2EpMapping[HTC_RAW_STREAM_NUM_MAX];
    HTC_RAW_STREAM_ID       arEp2RawMapping[ENDPOINT_MAX];
    A_BOOL                  arRawIfInit;
    int                     arDeviceIndex;
    A_BOOL                  dbgLogFetchInProgress;
    A_UCHAR                 log_buffer[DBGLOG_HOST_LOG_BUFFER_SIZE];
    A_UINT32                log_cnt;
    A_UINT32                dbglog_init_done;
} AR_SOFTC_T;

/* Macros */

#define arRawIfEnabled(ar) (ar)->arRawIfInit
#define arRawStream2EndpointID(ar,raw)          (ar)->arRaw2EpMapping[(raw)]
#define arSetRawStream2EndpointIDMap(ar,raw,ep)  \
{  (ar)->arRaw2EpMapping[(raw)] = (ep); \
   (ar)->arEp2RawMapping[(ep)] = (raw); }
#define arEndpoint2RawStreamID(ar,ep)           (ar)->arEp2RawMapping[(ep)]

#define AR6000_STAT_INC(ar, stat)       (ar->arNetStats.stat++)

#define AR6000_SPIN_LOCK(lock, param)   do {                            \
    A_MUTEX_LOCK(lock);                                                 \
} while (0)

#define AR6000_SPIN_UNLOCK(lock, param) do {                            \
    A_MUTEX_UNLOCK(lock);                                               \
} while (0)

/* Function Declarations */

int ar6000_dbglog_get_debug_logs(AR_SOFTC_T *ar);

ATH_DEBUG_DECLARE_EXTERN(htc);
ATH_DEBUG_DECLARE_EXTERN(wmi);
ATH_DEBUG_DECLARE_EXTERN(bmi);
ATH_DEBUG_DECLARE_EXTERN(hif);
ATH_DEBUG_DECLARE_EXTERN(wlan);
ATH_DEBUG_DECLARE_EXTERN(misc);

// Interafce Functions

A_BOOL SDIO_loadDriver_ENE(A_BOOL bOnOff, A_UINT32 subSystemID);
A_BOOL SDIO_closehandle(void);
A_BOOL SDIO_InitAR6000_ene(HANDLE *handle, A_UINT32 *nTargetID);
A_BOOL SDIO_EndAR6000_ene(int devIndex);
HANDLE SDIO_open_device_ene(A_UINT32 device_fn, A_UINT32 devIndex, char * pipeName);
int SDIO_DisableDragonSleep(HANDLE device);
DWORD SDIO_DRG_Write(  HANDLE COM_Write, PUCHAR buf, ULONG length );
DWORD SDIO_DRG_Read( HANDLE pContext, PUCHAR buf, ULONG length, PULONG pBytesRead);
DWORD HCI_DRG_Write(  HANDLE COM_Write, PUCHAR buf, ULONG length );
DWORD HCI_DRG_Read( HANDLE pContext, PUCHAR buf, ULONG length, PULONG pBytesRead);
int SDIO_BMIWriteSOCRegister_win(HANDLE handle, A_UINT32 address, A_UINT32 param);
int SDIO_BMIReadSOCRegister_win(HANDLE handle, A_UINT32 address, A_UINT32 *param);
int SDIO_BMIWriteMemory_win(HANDLE handle, A_UINT32 address, A_UCHAR *buffer, A_UINT32 length);
int SDIO_BMIReadMemory_win(HANDLE handle, A_UINT32 address, A_UCHAR *buffer, A_UINT32 length);
int SDIO_BMISetAppStart_win(HANDLE handle, A_UINT32 address);
int SDIO_BMIDone_win(HANDLE handle);
int SDIO_BMIFastDownload_win(HANDLE handle, A_UINT32 address, A_UCHAR *buffer, A_UINT32 length);
int SDIO_BMIExecute_win(HANDLE handle, A_UINT32 address, A_UINT32 *param);
int SDIO_BMITransferEepromFile_win(HANDLE handle, A_UCHAR *eeprom, A_UINT32 length);

A_BOOL ETH_InitAR6000_ene(HANDLE *handle, A_UINT32 *nTargetID);
HANDLE ETH_open_device_ene(A_UINT32 device_fn, A_UINT32 devIndex, char * pipeName);
DWORD ETH_DRG_Read( HANDLE pContext,  PUCHAR buf, ULONG length,  PULONG pBytesRead);
DWORD ETH_DRG_Write(  HANDLE COM_Write, PUCHAR buf, ULONG length);

A_BOOL UART_InitAR6000_ene(HANDLE *handle, A_UINT32 *nTargetID);
HANDLE UART_open_device_ene(A_UINT32 device_fn, A_UINT32 devIndex, char * pipeName);
DWORD UART_DRG_Read( HANDLE pContext,  PUCHAR buf, ULONG length,  PULONG pBytesRead);
DWORD UART_DRG_Write(  HANDLE COM_Write, PUCHAR buf, ULONG length);

#ifdef __cplusplus
}
#endif

#endif /* _AR6000_H_ */
