//------------------------------------------------------------------------------
// <copyright file="testcmd.h" company="Atheros">
//    Copyright (c) 2004-2010 Atheros Corporation.  All rights reserved.
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
// 	Wifi driver for AR6003
// </summary>
//
//------------------------------------------------------------------------------
//==============================================================================
// Author(s): ="Atheros"
//==============================================================================

#ifndef  TESTCMD_H_
#define  TESTCMD_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef AR6002_REV2
#define TCMD_MAX_RATES 12
#else
#define TCMD_MAX_RATES 28
#endif

//#define WMI_CMD_ID_SIZE   4
//#define WMI_CMDS_SIZE_MAX 2048
//#define TC_CMDS_GAP       16
// should add up to the same size as buf[WMI_CMDS_SIZE_MAX]
//#define TC_CMDS_SIZE_MAX  (WMI_CMDS_SIZE_MAX - sizeof(TC_CMDS_HDR) - WMI_CMD_ID_SIZE - TC_CMDS_GAP)
#define TC_CMDS_SIZE_MAX  256

typedef enum {
    ZEROES_PATTERN = 0,
    ONES_PATTERN,
    REPEATING_10,
    PN7_PATTERN,
    PN9_PATTERN,
    PN15_PATTERN
}TX_DATA_PATTERN;

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
    TCMD_CONT_TX_OFF = 0,
    TCMD_CONT_TX_SINE,
    TCMD_CONT_TX_FRAME,
    TCMD_CONT_TX_TX99,
    TCMD_CONT_TX_TX100,
    TCMD_CONT_TX_OFFSETTONE,
} TCMD_CONT_TX_MODE;

typedef enum {
    TCMD_WLAN_MODE_NOHT = 0,
    TCMD_WLAN_MODE_HT20 = 1,
    TCMD_WLAN_MODE_HT40PLUS = 2,
    TCMD_WLAN_MODE_HT40MINUS = 3,
    TCMD_WLAN_MODE_CCK = 4,

    TCMD_WLAN_MODE_MAX,
    TCMD_WLAN_MODE_INVALID = TCMD_WLAN_MODE_MAX,

} TCMD_WLAN_MODE;

typedef enum {
    TPC_TX_PWR = 0,
    TPC_FORCED_GAIN,
    TPC_TGT_PWR
} TPC_TYPE;

typedef PREPACK struct {
    A_UINT32                testCmdId;
    A_UINT32                mode;
    A_UINT32                freq;
    A_UINT32                dataRate;
    A_INT32                 txPwr;
    A_UINT32                antenna;
    A_UINT32                enANI;
    A_UINT32                scramblerOff;
    A_UINT32                aifsn;
    A_UINT16                pktSz;
    A_UINT16                txPattern;
    A_UINT32                shortGuard;
    A_UINT32                numPackets;
    A_UINT32                wlanMode;
    A_UINT32                tpcm;
} POSTPACK TCMD_CONT_TX;

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
typedef enum {
    TCMD_CONT_RX_PROMIS =0,
    TCMD_CONT_RX_FILTER,
    TCMD_CONT_RX_REPORT,
    TCMD_CONT_RX_SETMAC,
    TCMD_CONT_RX_SET_ANT_SWITCH_TABLE,
    TC_CMD_RESP,
} TCMD_CONT_RX_ACT;

typedef PREPACK struct {
    A_UINT32        testCmdId;
    A_UINT32        act;
    A_UINT32        enANI;
    PREPACK union {
        struct PREPACK TCMD_CONT_RX_PARA {
            A_UINT32    freq;
            A_UINT32    antenna;
            A_UINT32    wlanMode;
        } POSTPACK para;
        struct PREPACK TCMD_CONT_RX_REPORT {
            A_UINT32    totalPkt;
            A_INT32     rssiInDBm;
            A_UINT32    crcErrPkt;
            A_UINT32    secErrPkt;
            A_UINT16    rateCnt[TCMD_MAX_RATES];
            A_UINT16    rateCntShortGuard[TCMD_MAX_RATES];
        } POSTPACK report;
        struct PREPACK TCMD_CONT_RX_MAC {
            A_UCHAR     addr[ATH_MAC_LEN];
            A_UCHAR     btaddr[ATH_MAC_LEN];
            A_UINT16    regDmn[2];
            A_UINT32    otpWriteFlag;
        } POSTPACK mac;
        struct PREPACK TCMD_CONT_RX_ANT_SWITCH_TABLE {
            A_UINT32                antswitch1;
            A_UINT32                antswitch2;
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
    A_UINT32  testCmdId;
    A_UINT32  mode;
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
    TC_CMDS_PM_CAL,
    TC_CMDS_PSAT_CAL,
    TC_CMDS_PSAT_CAL_RESULT,
    TC_CMDS_CAL_PWRS,
    TC_CMDS_WRITE_CAL_2_OTP,
    TC_CMDS_CHAR_PSAT,
    TC_CMDS_CHAR_PSAT_RESULT,
    TC_CMDS_PM_CAL_RESULT,
    TC_CMDS_SINIT_WAIT,
    TC_CMDS_SINIT_LOAD_AUTO,
    TC_CMDS_COUNT
} TC_CMDS_ACT;

typedef PREPACK struct {
    A_UINT32   testCmdId;
    A_UINT32   act;
    PREPACK union {
        A_UINT32  enANI;    // to be identical to CONT_RX struct
        struct PREPACK {
            A_UINT16   length;
            A_UINT8    version;
            A_UINT8    bufLen;
        } POSTPACK parm;
    } POSTPACK u;
} POSTPACK TC_CMDS_HDR;

typedef PREPACK struct {
    TC_CMDS_HDR  hdr;
    A_UINT8      buf[TC_CMDS_SIZE_MAX];
} POSTPACK TC_CMDS;

typedef PREPACK struct {
    A_UINT32    testCmdId;
    A_UINT32    regAddr;
    A_UINT32    val;
    A_UINT16    flag;
} POSTPACK TCMD_SET_REG;

typedef enum {
    TCMD_CONT_TX_ID,
    TCMD_CONT_RX_ID,
    TCMD_PM_ID,
    TC_CMDS_ID,
    TCMD_SET_REG_ID,
	
	/*For synergy purpose we added the following tcmd id but these
	tcmd's will not go to the firmware instead we will write values 
	to the NV area */
	
	TCMD_NIC_MAC = 100, 
	TCMD_CAL_FILE_INDEX = 101,
} TCMD_ID;

typedef PREPACK struct 
{
    A_UINT32  testCmdId;
    A_UINT8   mac_address[ATH_MAC_LEN];
} POSTPACK TCMD_NIC_MAC_S;

typedef PREPACK struct 
{
       A_UINT32  testCmdId;
       A_UINT32   cal_file_index;
} POSTPACK TCMD_CAL_FILE_INDEX_S;

typedef PREPACK union {
    TCMD_CONT_TX         contTx;
    TCMD_CONT_RX         contRx;
    TCMD_PM              pm;
    // New test cmds from ART/MDK ...
    TC_CMDS              tcCmds;
    TCMD_SET_REG setReg;
} POSTPACK TEST_CMD;

typedef enum {
    TC_MSG_RESERVED,
    TC_MSG_PSAT_CAL_RESULTS,
    TC_MSG_CAL_POWER,
    TC_MSG_CHAR_PSAT_RESULTS,
    TC_MSG_PM_CAL_RESULTS,
    TC_MSG_PSAT_CAL_ACK,
    TC_MSG_COUNT
} TC_MSG_ID;

typedef PREPACK struct {
    A_INT8  olpcGainDelta_diff;
    A_INT8  olpcGainDelta_abs;
    A_UINT8 thermCalVal;
    A_UINT8 numTryBF;
    A_UINT32 cmac_olpc;
    A_UINT32 cmac_psat;
    A_UINT16 cmac_olpc_pcdac;
    A_UINT16 cmac_psat_pcdac;
    A_INT16  lineSlope;
    A_INT16  lineVariance;
    A_UINT16 psatParm;
    A_UINT8  reserved[2];
} POSTPACK OLPCGAIN_THERM_DUPLET;

#if !defined(WHAL_NUM_11G_CAL_PIERS_EXT)
#define WHAL_NUM_11G_CAL_PIERS_EXT 16
#define WHAL_NUM_11A_CAL_PIERS_EXT 32
#endif
#define PSAT_WHAL_NUM_11G_CAL_PIERS_MAX 3
#define PSAT_WHAL_NUM_11A_CAL_PIERS_MAX 5
typedef PREPACK struct {
    OLPCGAIN_THERM_DUPLET olpcGainTherm2G[PSAT_WHAL_NUM_11G_CAL_PIERS_MAX];
    OLPCGAIN_THERM_DUPLET olpcGainTherm5G[PSAT_WHAL_NUM_11A_CAL_PIERS_MAX];
} POSTPACK PSAT_CAL_RESULTS;

#define _MAX_TX_GAIN_ENTRIES 32
typedef PREPACK struct {
    A_UINT32 cmac_i[_MAX_TX_GAIN_ENTRIES];
    A_UINT8  pcdac[_MAX_TX_GAIN_ENTRIES];

    A_UINT8  freq;
    A_UINT8  an_txrf3_rdiv2g;
    A_UINT8  an_txrf3_pdpredist2g;
    A_UINT8  an_rxtx2_mxrgain;
    A_UINT8  an_rxrf_bias1_pwd_ic25mxr2gh;
    A_UINT8  an_bias2_pwd_ic25rxrf;
    A_UINT8  an_bb1_i2v_curr2x;
    A_UINT8  an_txrf3_capdiv2g;

//  A_UINT32 cmac_q[_MAX_TX_GAIN_ENTRIES];
} POSTPACK CHAR_PSAT_RESULTS;

typedef PREPACK struct {
    A_INT16  txPwr2G_t10[WHAL_NUM_11G_CAL_PIERS_EXT];
    A_INT16  txPwr5G_t10[WHAL_NUM_11A_CAL_PIERS_EXT];
} POSTPACK CAL_TXPWR;

typedef PREPACK struct {
    A_UINT8    thermCalVal;
    A_UINT8    future[3];
} POSTPACK PM_CAL_RESULTS;

typedef PREPACK struct {
    TC_MSG_ID msgId;
    PREPACK union {
        PSAT_CAL_RESULTS               psatCalResults;
        CAL_TXPWR                      txPwrs;
        CHAR_PSAT_RESULTS              psatCharResults;
        PM_CAL_RESULTS                 pmCalResults;
    } POSTPACK msg;

} POSTPACK TC_MSG;

typedef struct _psat_sweep_table {
    A_UINT8  an_txrf3_rdiv2g;               // [0,3] _RDIV2G_MIN, _RDIV2G_MAX
    A_UINT8  an_txrf3_pdpredist2g;          // [0,1] _PDPREDIST2G_MIN, _PDPREDIST2G_MAX
    A_UINT8  an_rxtx2_mxrgain;              // [0,3] _MXRGAIN_MIN, _MXRGAIN_MAX
    A_UINT8  an_rxrf_bias1_pwd_ic25mxr2gh;  // [0,3] _PWD_IC25MX2GH_MIN, _PWD_IC25MXRGH_MAX
    A_UINT8  an_bias2_pwd_ic25rxrf;         // [0,3] _PWD_IC25RXRF_MIN, _PWD_RC25RXRF_MAX
    A_UINT8  an_bb1_i2v_curr2x;             // [0,1] _I2V_CURR2X_MIN, _I2V_CURR2X_MAX
    A_UINT8  an_txrf3_capdiv2g;             // [0,15] _CAPDIV2G_MIN, _CAPDIV2G_MAX
    A_INT8   olpcPsatCmacDelta;             // olpcPsatCmacDelta 
    A_UINT16 psatParm;
    A_UINT16 padding2;
} PSAT_SWEEP_TABLE;
#define NUM_PSAT_CHAR_PARMS  7

#ifdef __cplusplus
}
#endif

#endif /* TESTCMD_H_ */
