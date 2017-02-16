/*copyright (c) 2006 Atheros Communications Inc.
 * All rights reserved.
 *
 * This file contains the definitions for wmiconfig utility
 *
 * $Id: //depot/sw/releases/ath6kl_5.0.0/apps/tcmd/athtestcmd.h#3 $
 */

#ifndef _TCMD_H_
#define _TCMD_H_

#ifdef __cplusplus
extern "C" {
#endif

enum {
    TESTMODE_CONT_TX = 801,     /* something that doesn't collide with ascii */
    TESTMODE_CONT_RX,
    TESTMODE_PM,
    TESTMODE_SETLPREAMBLE,
    TESTMODE_SETREG,
    TESTMODE_CMDS,
};

enum {
    TCMD_SET_RX_LPL=501,     /* something that doesn't collide with ascii */
    TCMD_EFUSE_START=502,
    TCMD_EFUSE_END=503,
    TCMD_EFUSE_DATA=504,
    TCMD_EFUSE_DUMP=505,
    TCMD_EFUSE_WRITE=506,
    TCMD_OTP_DUMP=507,
    TCMD_OTP_WRITE=508,
    TCMD_READ_THERMAL=509,
    TCMD_SET_BT_MODE=510,
};

#ifdef __cplusplus
}
#endif

#endif /* _TCMD_H_ */

