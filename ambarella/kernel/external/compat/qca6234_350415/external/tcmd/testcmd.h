/*
 * Copyright (c) 2011 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 * $ATH_LICENSE_HOSTSDK0_C$
 */

#ifndef  TESTCMD_H_
#define  TESTCMD_H_

#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>


#ifdef __cplusplus
extern "C" {
#endif


#ifdef AR6002_REV2
#define TCMD_MAX_RATES 12
#else
#define TCMD_MAX_RATES 47
#endif

#define PREPACK
#define POSTPACK __attribute__ ((packed))

#define MPATTERN                (10*4)
#define ATH_MAC_LEN             6
#define RATE_MASK_ROW_MAX       2
#define RATE_POWER_MAX_INDEX    8
#define RATE_MASK_BIT_MAX       32

typedef enum {
        ZEROES_PATTERN = 0,
        ONES_PATTERN,
        REPEATING_10,
        PN7_PATTERN,
        PN9_PATTERN,
        PN15_PATTERN
} TX_DATA_PATTERN;


#define TC_CMDS_GAP       16
// should add up to the same size as buf[WMI_CMDS_SIZE_MAX]
//#define TC_CMDS_SIZE_MAX  (WMI_CMDS_SIZE_MAX - sizeof(TC_CMDS_HDR) - WMI_CMD_ID_SIZE - TC_CMDS_GAP)  
#define TC_CMDS_SIZE_MAX  255

#define DESC_STBC_ENA_MASK          0x00000001
#define DESC_STBC_ENA_SHIFT         0
#define DESC_LDPC_ENA_MASK          0x00000002
#define DESC_LDPC_ENA_SHIFT         1
#define PAPRD_ENA_MASK              0x00000004
#define PAPRD_ENA_SHIFT             2

#define HALF_SPEED_MODE         50
#define QUARTER_SPEED_MODE      51

/* Continous tx
   mode : TCMD_CONT_TX_OFF - Disabling continous tx
          TCMD_CONT_TX_SINE - Enable continuous unmodulated tx
          TCMD_CONT_TX_FRAME- Enable continuous modulated tx
   freq : Channel freq in Mhz. (e.g 2412 for channel 1 in 11 g)
dataRate: 0 - 1 Mbps
          1 - 2 Mbps
          2 - 5.5 Mbps
          3 - 11 Mbps
          4 - 6 Mbps
          5 - 9 Mbps
          6 - 12 Mbps
          7 - 18 Mbps
          8 - 24 Mbps
          9 - 36 Mbps
         10 - 28 Mbps
         11 - 54 Mbps
  txPwr: twice the Tx power in dBm, actual dBm values of [5 -11] for unmod Tx,
      [5-14] for mod Tx
antenna:  1 - one antenna
          2 - two antenna
Note : Enable/disable continuous tx test cmd works only when target is awake.
*/

typedef enum {
    TPC_TX_PWR = 0,
    TPC_FORCED_GAIN,
    TPC_TGT_PWR,
    TPC_TX_FORCED_GAIN
} TPC_TYPE;

typedef enum {
        TCMD_CONT_TX_OFF = 0,
        TCMD_CONT_TX_SINE,
        TCMD_CONT_TX_FRAME,
        TCMD_CONT_TX_TX99,
        TCMD_CONT_TX_TX100
} TCMD_CONT_TX_MODE;

typedef enum {
        TCMD_WLAN_MODE_NOHT = 0,
        TCMD_WLAN_MODE_HT20 = 1,
        TCMD_WLAN_MODE_HT40PLUS = 2,
        TCMD_WLAN_MODE_HT40MINUS = 3,
} TCMD_WLAN_MODE;


// ATENTION ATENTION... when you chnage this tructure, please also change 
// the structure TX_DATA_START_PARAMS in //olca/host/tools/systemtools/NARTHAL/common/dk_cmds.h
typedef PREPACK struct {
    uint32_t                testCmdId;
    uint32_t                mode;
    uint32_t                freq;
    uint32_t                dataRate;
    int32_t                 txPwr;
    uint32_t                antenna;
    uint32_t                enANI;
    uint32_t                scramblerOff;
    uint32_t                aifsn;
    uint16_t                pktSz;
    uint16_t                txPattern;
    uint32_t                shortGuard;
    uint32_t                numPackets;
    uint32_t                wlanMode;
    uint32_t                lpreamble;
    uint32_t	            txChain;
    uint32_t                miscFlags;

    uint32_t                broadcast;
    uint8_t                 bssid[ATH_MAC_LEN];
    uint16_t                bandwidth;
    uint8_t                 txStation[ATH_MAC_LEN];
    uint16_t                unUsed2;
    uint8_t                 rxStation[ATH_MAC_LEN];
    uint16_t                unUsed3;
    uint32_t                tpcm;
    uint32_t                retries;
    uint32_t                agg;
    uint32_t                nPattern;
    uint8_t                 dataPattern[MPATTERN]; // bytes to be written 
} POSTPACK TCMD_CONT_TX;

typedef enum {
        TCMD_CONT_RX_PROMIS = 0,
        TCMD_CONT_RX_FILTER,
        TCMD_CONT_RX_REPORT,
        TCMD_CONT_RX_SETMAC,
        TCMD_CONT_RX_SET_ANT_SWITCH_TABLE
} TCMD_CONT_RX_ACT;


#define TCMD_TXPATTERN_ZERONE                 0x1
#define TCMD_TXPATTERN_ZERONE_DIS_SCRAMBLE    0x2

/* Continuous Rx
 act: TCMD_CONT_RX_PROMIS - promiscuous mode (accept all incoming frames)
      TCMD_CONT_RX_FILTER - filter mode (accept only frames with dest
                                             address equal specified
                                             mac address (set via act =3)
      TCMD_CONT_RX_REPORT  off mode  (disable cont rx mode and get the
                                          report from the last cont
                                          Rx test)

     TCMD_CONT_RX_SETMAC - set MacAddr mode (sets the MAC address for the
                                                 target. This Overrides
                                                 the default MAC address.)

*/

typedef PREPACK struct {
    uint32_t        testCmdId;
    uint32_t        act;
    uint32_t        enANI;
    PREPACK union {
        struct PREPACK TCMD_CONT_RX_PARA {
            uint32_t    freq;
            uint32_t    antenna;
            uint32_t    wlanMode;
            uint32_t   	ack;
            uint32_t    rxChain;
            uint32_t    bc;
            uint32_t    bandwidth;
            uint32_t    lpl;/* low power listen */
        } POSTPACK para;
        struct PREPACK TCMD_CONT_RX_REPORT {
            uint32_t    totalPkt;
            int32_t     rssiInDBm;
            uint32_t    crcErrPkt;
            uint32_t    secErrPkt;
            uint16_t    rateCnt[TCMD_MAX_RATES];
            uint16_t    rateCntShortGuard[TCMD_MAX_RATES];
        } POSTPACK report;
        struct PREPACK TCMD_CONT_RX_MAC {
            uint8_t    addr[ATH_MAC_LEN];
            uint8_t    bssid[ATH_MAC_LEN];
            uint8_t    btaddr[ATH_MAC_LEN];     
            uint8_t    regDmn[2];
            uint32_t   otpWriteFlag;
        } POSTPACK mac;
        struct PREPACK TCMD_CONT_RX_ANT_SWITCH_TABLE {
            uint32_t                antswitch1;
            uint32_t                antswitch2;
        }POSTPACK antswitchtable;
    } POSTPACK u;
} POSTPACK TCMD_CONT_RX;

/* Force sleep/wake  test cmd
 mode: TCMD_PM_WAKEUP - Wakeup the target
       TCMD_PM_SLEEP - Force the target to sleep.
 */
typedef enum {
    TCMD_PM_WAKEUP = 1, /* be consistent with target */
    TCMD_PM_SLEEP,
    TCMD_PM_DEEPSLEEP
} TCMD_PM_MODE;

typedef PREPACK struct {
    uint32_t  testCmdId;
    uint32_t  mode;
} POSTPACK TCMD_PM;

typedef enum {
    TC_CMDS_VERSION_RESERVED=0,
    TC_CMDS_VERSION_MDK,
    TC_CMDS_VERSION_TS,
    TC_CMDS_VERSION_LAST,
} TC_CMDS_VERSION;

typedef enum {
    TC_CMDS_TS =0,
    TC_CMDS_CAL,
    TC_CMDS_TPCCAL = TC_CMDS_CAL,
    TC_CMDS_TPCCAL_WITH_OTPWRITE,
    TC_CMDS_OTPDUMP,
    TC_CMDS_OTPSTREAMWRITE,    
    TC_CMDS_EFUSEDUMP,    
    TC_CMDS_EFUSEWRITE,    
    TC_CMDS_READTHERMAL,
    TC_CMDS_SET_BT_MODE,
} TC_CMDS_ACT;

typedef PREPACK struct {
    uint32_t   testCmdId;
    uint32_t   act;
    PREPACK union {
        uint32_t  enANI;    // to be identical to CONT_RX struct
        struct PREPACK {
            uint16_t   length;
            uint8_t    version;
            uint8_t    bufLen;
        } POSTPACK parm;
    } POSTPACK u;
} POSTPACK TC_CMDS_HDR;

typedef PREPACK struct {
    TC_CMDS_HDR  hdr;
    uint8_t      buf[TC_CMDS_SIZE_MAX+1];
} POSTPACK TC_CMDS;

typedef PREPACK struct {
    uint32_t    testCmdId;
    uint32_t    regAddr;
    uint32_t    val;
    uint16_t    flag;
} POSTPACK TCMD_SET_REG;

typedef enum {
    TCMD_CONT_TX_ID,
    TCMD_CONT_RX_ID,
    TCMD_PM_ID,
    TC_CMDS_ID,
    TCMD_SET_REG_ID,

    INVALID_CMD_ID=255,
} TCMD_ID;

typedef PREPACK union {
          TCMD_CONT_TX         contTx;
          TCMD_CONT_RX         contRx;
          TCMD_PM              pm;
          // New test cmds from ART/MDK ...
          TC_CMDS              tcCmds;          
          TCMD_SET_REG         setReg;
} POSTPACK TEST_CMD;

typedef struct _VALUES_OF_INTEREST {
     uint8_t thermCAL;
     uint8_t future[3];
} _VALUES_OF_INTEREST;

#ifdef __cplusplus
}
#endif

#endif /* TESTCMD_H_ */

