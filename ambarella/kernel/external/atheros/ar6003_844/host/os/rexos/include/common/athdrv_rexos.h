/*
 * Copyright (c) 2004-2005 Atheros Communications Inc.
 * All rights reserved.
 *
 * $ATH_LICENSE_HOSTSDK0_C$
 *
 */

#ifndef _ATHDRV_REXOS_H_
#define _ATHDRV_REXOS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <ieee80211.h>
#include "common_drv.h"
#include "dbglog_api.h"
#include "roaming.h"

#define MAX_AR6000                  1
#define AR6000_MAX_RX_BUFFERS       1
#define AR6000_BUFFER_SIZE          1664
#define AR6000_TX_TIMEOUT           10
#define AR6000_ETH_ADDR_LEN         6
#define AR6000_MAX_ENDPOINTS        4
#define MAX_NODE_NUM                15
#define MAX_COOKIE_NUM              150
#define AR6000_HB_CHALLENGE_RESP_FREQ_DEFAULT        1
#define AR6000_HB_CHALLENGE_RESP_MISS_THRES_DEFAULT  1

/* Should driver perform wlan node caching? */
#define AR6000_DRIVER_CFG_GET_WLANNODECACHING   0x8001
/*Should we log raw WMI msgs */
#define AR6000_DRIVER_CFG_LOG_RAW_WMI_MSGS      0x8002

enum {
    DRV_HB_CHALLENGE = 0,
    APP_HB_CHALLENGE
};

typedef enum {
    WLAN_DISABLED,
    WLAN_ENABLED
} WLAN_STATE;

struct ar_wep_key {
    A_UINT8                 arKeyIndex;
    A_UINT8                 arKeyLen;
    A_UINT8                 arKey[64];
};

struct ar_node_mapping {
    A_UINT8                 macAddress[6];
    A_UINT8                 epId;
    A_UINT8                 txPending;
};


struct ar_cookie {
    A_UINT32               arc_bp[2];    /* Must be first field */
    HTC_PACKET             HtcPkt;       /* HTC packet wrapper */
    struct ar_cookie *arc_list_next;
};

struct ar_hb_chlng_resp {
    A_TIMER                 timer;
    A_UINT32                frequency;
    A_UINT32                seqNum;
    A_BOOL                  outstanding;
    A_UINT8                 missCnt;
    A_UINT8                 missThres;
};

struct ar6000_version {
    A_UINT32        host_ver;
    A_UINT32        target_ver;
};

/* Host side link management data structures */

typedef struct user_rssi_thold_t {
    A_INT16     tag;
    A_INT16     rssi;
} USER_RSSI_THOLD;

typedef struct user_rssi_params_t {
    A_UINT8            weight;
    A_UINT32           pollTime;
    USER_RSSI_THOLD    tholds[12];
} USER_RSSI_PARAMS;

typedef int (*evt_dispatcher_type)(A_UINT8* buf, A_UINT16 size);
typedef int (*data_dispatcher_type)(A_UINT8* buf, A_UINT16 size);

typedef struct ar6_softc {
    void                    *arWmi;
    int                     arTxPending[ENDPOINT_MAX];
    int                     arTotalTxDataPending;
    A_UINT8                 arNumDataEndPts;
    A_BOOL                  arWmiEnabled;
    A_BOOL                  arWmiReady;
    A_BOOL                  arConnected;
    void                    *arHtcTarget;
    void                    *arHifDevice;
    A_MUTEX_T               arLock;
    int                     arRxBuffers[ENDPOINT_MAX];
    int                     arSsidLen;
    A_UINT8                 arSsid[32];
    A_UINT8                 arNetworkType;
    A_UINT8                 arDot11AuthMode;
    A_UINT8                 arAuthMode;
    A_UINT8                 arPairwiseCrypto;
    A_UINT8                 arPairwiseCryptoLen;
    A_UINT8                 arGroupCrypto;
    A_UINT8                 arGroupCryptoLen;
    A_UINT8                 arDefTxKeyIndex;
    struct ar_wep_key       arWepKeyList[WMI_MAX_KEY_INDEX + 1];
    A_UINT8                 arBssid[6];
    A_UINT8                 arReqBssid[6];
    A_UINT16                arChannelHint;
    A_UINT16                arBssChannel;
    A_UINT16                arListenInterval;
    struct ar6000_version   arVersion;
    A_UINT32                arTargetType;
    A_INT8                  arRssi;
    A_UINT8                 arTxPwr;
    A_BOOL                  arTxPwrSet;
    A_INT32                 arBitRate;
    A_INT8                  arNumChannels;
    A_UINT16                arChannelList[32];
    A_UINT32                arRegCode;
    A_INT8                  arMaxRetries;
    A_UINT8                 arPhyCapability;
#ifdef CONFIG_HOST_TCMD_SUPPORT
    A_UINT8                 tcmdRxReport;
    A_UINT32                tcmdRxTotalPkt;
    A_INT32                 tcmdRxRssi;
    A_UINT32                tcmdPm;
#endif
    WLAN_STATE              arWlanState;
    struct ar_node_mapping  arNodeMap[MAX_NODE_NUM];
    A_UINT8                 arIbssPsEnable;
    A_UINT8                 arNodeNum;
    A_UINT8                 arNexEpId;
    struct ar_cookie        *arCookieList;
    A_UINT16                arRateMask;
    A_BOOL                  arConnectPending;
    A_BOOL                  arWmmEnabled;
    struct ar_hb_chlng_resp arHBChallengeResp;
    A_UINT8                 arKeepaliveConfigured;
    HTC_ENDPOINT_ID         arAc2EpMapping[WMM_NUM_AC];
    A_BOOL                  arAcStreamActive[WMM_NUM_AC];
    A_UINT8                 arAcStreamPriMap[WMM_NUM_AC];
    A_UINT8                 arHiAcStreamActivePri;    
    A_UINT8                 arEp2AcMapping[ENDPOINT_MAX];
    HTC_ENDPOINT_ID         arControlEp;
    COMMON_CREDIT_STATE_INFO arCreditStateInfo;
    A_BOOL                  arWMIControlEpFull;
    A_BOOL                  dbglog_init_done;
    A_BOOL                  dbgLogFetchInProgress;
    A_UCHAR                 log_buffer[DBGLOG_HOST_LOG_BUFFER_SIZE];
    A_UINT32                log_cnt;
    A_UINT32                arConnectCtrlFlags;
    USER_RSSI_THOLD         rssi_map[12];
    evt_dispatcher_type     evt_dispatcher;
    data_dispatcher_type    data_dispatcher;
} AR_SOFTC_T;

#define arAc2EndpointID(ar,ac)          (ar)->arAc2EpMapping[(ac)]
#define arSetAc2EndpointIDMap(ar,ac,ep)  \
{  (ar)->arAc2EpMapping[(ac)] = (ep); \
   (ar)->arEp2AcMapping[(ep)] = (ac); }
#define arEndpoint2Ac(ar,ep)           (ar)->arEp2AcMapping[(ep)]

#define AR6000_STAT_INC(ar, stat)

void qcom_driver_init(void);
void qcom_driver_exit(void);
void qcom_driver_destroy(void *device);
int qcom_driver_open(void *device);
int qcom_driver_close(void *device);
int qcom_driver_tx(void *buff, void *device, A_UINT8 up);
int qcom_driver_tx_bt(void *buff, void *device);
void qcom_tx_data_cleanup(AR_SOFTC_T *ar);

int check_flow_on(void *device);
int check_flow_off(void *device);

void qcom_cookie_init(AR_SOFTC_T *ar);
void qcom_cookie_cleanup(AR_SOFTC_T *ar);
struct ar_cookie * qcom_alloc_cookie(AR_SOFTC_T  *ar);
void qcom_free_cookie(AR_SOFTC_T *ar, struct ar_cookie * cookie);

#ifdef __cplusplus
}
#endif

#endif

