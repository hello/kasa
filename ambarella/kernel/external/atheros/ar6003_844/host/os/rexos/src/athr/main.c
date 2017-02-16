/*
 * Copyright (c) 2004-2005 Atheros Communications Inc.
 * All rights reserved.
 *
 * $ATH_LICENSE_HOSTSDK0_C$
 *
 */

#ifdef ATHR_EMULATION
/* Linux/OS include files */
#include <stdio.h>
#include <pthread.h>
#include <sys/time.h>     /* struct timeval definition           */
#include <unistd.h>       /* declaration of gettimeofday()       */
#include <errno.h>
#include <stdlib.h>
#include <time.h>
#endif

/* Atheros include files */
#include <athdefs.h>
#include <a_types.h>
#include <a_osapi.h>
#include <wlan_api.h>
#include <wmi.h>
#include <htc.h>
#include <htc_api.h>
#include <wmi_api.h>
#include <ieee80211.h>

/* Qualcomm include files */
#ifdef ATHR_EMULATION
#include "qcom_stubs.h"
#endif
#include "wlan_dot11_sys.h"
#include "wlan_adp_def.h"
#include "wlan_adp_oem.h"
#include "wlan_trp_oem.h"
#include "wlan_trp.h"
#include "wlan_util.h"

/* Atheros platform include files */
#include "hif.h"
#include "a_hif.h"
#include "wlan_oem_ath.h"
#include "a_debug.h"
#include "testcmd.h"
#include "qcom_htc.h"

#define PRG_NOP				0
#define PRG_TIMEOUT			1
#define PRG_GOTO			2
#define PRG_QUIT			3
#define PRG_OEM_INIT		4
#define PRG_HTC_SIGNAL		5
#define PRG_OEM_FUNCTION	6
#define PRG_HIF_FUNCTION	7
#define PRG_OEM_COMMAND		8
#define PRG_RGOTO			9
#define PRG_IFTRUE			10
#define PRG_IFFALSE			11
#define PRG_RIFTRUE			12
#define PRG_RIFFALSE		13
#define PRG_TEST_TX_OPEN	14
#define PRG_TEST_CMD_OPEN	15
#define PRG_TEST_READY		16
#define PRG_SET_PARAM		17
#define PRG_SEND_PKT		18
#define PRG_SET_LOOP		19
#define PRG_LOOP			20
#define PRG_SET_TEST_PARAM	21
#define PRG_SUBCALL			22

int prg1[][2] =
{
	/*********************************************************************/
	/** Data transmit multiple packets                                  **/
	/*********************************************************************/

	/*********************************************************************/
	/** Initialize and bring up                                         **/
	/*********************************************************************/

	/*  ? */	{ PRG_TIMEOUT,			1 },
	/*  ? */	{ PRG_OEM_INIT,			0 },
	/*  ? */	{ PRG_TIMEOUT,			1 },
	/*  ? */	{ PRG_OEM_FUNCTION,		1 },	/* start */

	/*  ? */	{ PRG_TEST_READY,		0 },
	/*  ? */	{ PRG_RIFTRUE,			3 },
	/*  ? */	{ PRG_TIMEOUT,			1 },
	/*  ? */	{ PRG_RGOTO,			-3 },

	/*********************************************************************/
	/** Scan                                                            **/
	/*********************************************************************/

	/*  ? */	{ PRG_SET_PARAM,		0 },
	/*  ? */	{ PRG_TEST_CMD_OPEN,	0 },
	/*  ? */	{ PRG_RIFTRUE,			3 },
	/*  ? */	{ PRG_TIMEOUT,			1 },
	/*  ? */	{ PRG_RGOTO,			-3 },
	/*  ? */	{ PRG_OEM_COMMAND,		WLAN_TRP_ADP_SCAN_CMD },

	/*  ? */	{ PRG_TIMEOUT,			1 },

	/*********************************************************************/
	/** Connect                                                         **/
	/*********************************************************************/

    /*  ? */    { PRG_TEST_CMD_OPEN,    0 },
    /*  ? */    { PRG_RIFTRUE,          3 },
    /*  ? */    { PRG_TIMEOUT,          1 },
    /*  ? */    { PRG_RGOTO,            -3 },
	/*  ? */	{ PRG_OEM_COMMAND,		WLAN_TRP_ADP_JOIN_CMD },

	/*  ? */	{ PRG_TEST_CMD_OPEN,	0 },
	/*  ? */	{ PRG_RIFTRUE,			3 },
	/*  ? */	{ PRG_TIMEOUT,			1 },
	/*  ? */	{ PRG_RGOTO,			-3 },
	/*  ? */	{ PRG_OEM_COMMAND,		WLAN_TRP_ADP_AUTH_CMD },

	/*  ? */	{ PRG_TEST_CMD_OPEN,	0 },
	/*  ? */	{ PRG_RIFTRUE,			3 },
	/*  ? */	{ PRG_TIMEOUT,			1 },
	/*  ? */	{ PRG_RGOTO,			-3 },
	/*  ? */	{ PRG_OEM_COMMAND,		WLAN_TRP_ADP_ASSOC_CMD },

	/*  ? */	{ PRG_TIMEOUT,			1 },

	/*********************************************************************/
	/** Send a packet                                                   **/
	/*********************************************************************/

	/*  ? */	{ PRG_SET_PARAM,		-2 },	/* How many to "pull" 50 */
	/*  ? */	{ PRG_SET_LOOP,			10000 },	/* How many packets to tx 50 */
	/*  ? */	{ PRG_TEST_TX_OPEN,		0 },
	/*  ? */	{ PRG_RIFTRUE,			3 },
	/*  ? */	{ PRG_TIMEOUT,			3 },
	/*  ? */	{ PRG_RGOTO,			-3 },
	/*  ? */	{ PRG_SEND_PKT,			0 },
	/*  ? */	{ PRG_LOOP,				0 },

	/*  ? */	{ PRG_TIMEOUT,			2 },

	/*********************************************************************/
	/** Finish up                                                       **/
	/*********************************************************************/

	/*  ? */	{ PRG_TEST_CMD_OPEN,	0 },
	/*  ? */	{ PRG_RIFTRUE,			3 },
	/*  ? */	{ PRG_TIMEOUT,			1 },
	/*  ? */	{ PRG_RGOTO,			-3 },
	/*  ? */	{ PRG_OEM_COMMAND,		WLAN_TRP_ADP_DISASSOC_CMD },

    /*********************************************************************/
    /** Stop                                                            **/
    /*********************************************************************/

    /*  ? */    { PRG_OEM_FUNCTION,     2 },    /* stop */
    
	/*  ? */	{ PRG_TIMEOUT,			4 },
	/*  ? */	{ PRG_QUIT,				0 },
};

int prg2[][2] =
{
	/*********************************************************************/
	/** Functions and commands                                          **/
	/*********************************************************************/

	/*********************************************************************/
	/** Initialize and bring up                                         **/
	/*********************************************************************/

	/*  ? */	{ PRG_TIMEOUT,			1 },
	/*  ? */	{ PRG_OEM_INIT,			0 },
	/*  ? */	{ PRG_TIMEOUT,			1 },
	/*  ? */	{ PRG_OEM_FUNCTION,		1 },	/* start */

	/*  ? */	{ PRG_TEST_READY,		0 },
	/*  ? */	{ PRG_RIFTRUE,			3 },
	/*  ? */	{ PRG_TIMEOUT,			1 },
	/*  ? */	{ PRG_RGOTO,			-3 },

	/*********************************************************************/
	/** Scan                                                            **/
	/*********************************************************************/

	/*  ? */	{ PRG_SET_PARAM,		3 },
	/*  ? */	{ PRG_TEST_CMD_OPEN,	0 },
	/*  ? */	{ PRG_RIFTRUE,			3 },
	/*  ? */	{ PRG_TIMEOUT,			1 },
	/*  ? */	{ PRG_RGOTO,			-3 },
	/*  ? */	{ PRG_OEM_COMMAND,		WLAN_TRP_ADP_SCAN_CMD },

	/*  ? */	{ PRG_TIMEOUT,			5 },

    /*  ? */    { PRG_SET_PARAM,        2 },
    /*  ? */    { PRG_TEST_CMD_OPEN,    0 },
    /*  ? */    { PRG_RIFTRUE,          3 },
    /*  ? */    { PRG_TIMEOUT,          1 },
    /*  ? */    { PRG_RGOTO,            -3 },
    /*  ? */    { PRG_OEM_COMMAND,      WLAN_TRP_ADP_SCAN_CMD },

    /*  ? */    { PRG_TIMEOUT,          5 },

    /*  ? */    { PRG_SET_PARAM,        1 },
    /*  ? */    { PRG_TEST_CMD_OPEN,    0 },
    /*  ? */    { PRG_RIFTRUE,          3 },
    /*  ? */    { PRG_TIMEOUT,          1 },
    /*  ? */    { PRG_RGOTO,            -3 },
    /*  ? */    { PRG_OEM_COMMAND,      WLAN_TRP_ADP_SCAN_CMD },

    /*  ? */    { PRG_TIMEOUT,          5 },

    /*  ? */    { PRG_SET_PARAM,        0 },
    /*  ? */    { PRG_TEST_CMD_OPEN,    0 },
    /*  ? */    { PRG_RIFTRUE,          3 },
    /*  ? */    { PRG_TIMEOUT,          1 },
    /*  ? */    { PRG_RGOTO,            -3 },
    /*  ? */    { PRG_OEM_COMMAND,      WLAN_TRP_ADP_SCAN_CMD },

    /*  ? */    { PRG_TIMEOUT,          5 },
        
	/*********************************************************************/
	/** Connect                                                         **/
	/*********************************************************************/

	/*  ? */	{ PRG_TEST_CMD_OPEN,	0 },
	/*  ? */	{ PRG_RIFTRUE,			3 },
	/*  ? */	{ PRG_TIMEOUT,			1 },
	/*  ? */	{ PRG_RGOTO,			-3 },
	/*  ? */	{ PRG_OEM_COMMAND,		WLAN_TRP_ADP_JOIN_CMD },

	/*  ? */	{ PRG_TEST_CMD_OPEN,	0 },
	/*  ? */	{ PRG_RIFTRUE,			3 },
	/*  ? */	{ PRG_TIMEOUT,			1 },
	/*  ? */	{ PRG_RGOTO,			-3 },
	/*  ? */	{ PRG_OEM_COMMAND,		WLAN_TRP_ADP_AUTH_CMD },

	/*  ? */	{ PRG_TEST_CMD_OPEN,	0 },
	/*  ? */	{ PRG_RIFTRUE,			3 },
	/*  ? */	{ PRG_TIMEOUT,			1 },
	/*  ? */	{ PRG_RGOTO,			-3 },
	/*  ? */	{ PRG_OEM_COMMAND,		WLAN_TRP_ADP_ASSOC_CMD },

	/*  ? */	{ PRG_TIMEOUT,			5 },

	/*********************************************************************/
	/** Suspend/Resume                                                  **/
	/*********************************************************************/

	/*  ? */	{ PRG_TEST_CMD_OPEN,	0 },
	/*  ? */	{ PRG_RIFTRUE,			3 },
	/*  ? */	{ PRG_TIMEOUT,			1 },
	/*  ? */	{ PRG_RGOTO,			-3 },
	/*  ? */	{ PRG_OEM_COMMAND,		WLAN_TRP_ADP_SUSPEND_CMD },

	/*  ? */	{ PRG_TIMEOUT,			5 },

	/*  ? */	{ PRG_TEST_CMD_OPEN,	0 },
	/*  ? */	{ PRG_RIFTRUE,			3 },
	/*  ? */	{ PRG_TIMEOUT,			1 },
	/*  ? */	{ PRG_RGOTO,			-3 },
	/*  ? */	{ PRG_OEM_COMMAND,		WLAN_TRP_ADP_RESUME_CMD },

	/*  ? */	{ PRG_TIMEOUT,			1 },

	/*********************************************************************/
	/** Get command                                                     **/
	/*********************************************************************/

	/*  ? */	{ PRG_SET_PARAM,		0 },
	/*  ? */	{ PRG_TEST_CMD_OPEN,	0 },
	/*  ? */	{ PRG_RIFTRUE,			3 },
	/*  ? */	{ PRG_TIMEOUT,			1 },
	/*  ? */	{ PRG_RGOTO,			-3 },
	/*  ? */	{ PRG_OEM_COMMAND,		WLAN_TRP_ADP_OPR_GET_CMD },

	/*  ? */	{ PRG_SET_PARAM,		1 },
	/*  ? */	{ PRG_TEST_CMD_OPEN,	0 },
	/*  ? */	{ PRG_RIFTRUE,			3 },
	/*  ? */	{ PRG_TIMEOUT,			1 },
	/*  ? */	{ PRG_RGOTO,			-3 },
	/*  ? */	{ PRG_OEM_COMMAND,		WLAN_TRP_ADP_OPR_GET_CMD },

	/*  ? */	{ PRG_SET_PARAM,		2 },
	/*  ? */	{ PRG_TEST_CMD_OPEN,	0 },
	/*  ? */	{ PRG_RIFTRUE,			3 },
	/*  ? */	{ PRG_TIMEOUT,			1 },
	/*  ? */	{ PRG_RGOTO,			-3 },
	/*  ? */	{ PRG_OEM_COMMAND,		WLAN_TRP_ADP_OPR_GET_CMD },

	/*  ? */	{ PRG_SET_PARAM,		3 },
	/*  ? */	{ PRG_TEST_CMD_OPEN,	0 },
	/*  ? */	{ PRG_RIFTRUE,			3 },
	/*  ? */	{ PRG_TIMEOUT,			1 },
	/*  ? */	{ PRG_RGOTO,			-3 },
	/*  ? */	{ PRG_OEM_COMMAND,		WLAN_TRP_ADP_OPR_GET_CMD },

	/*  ? */	{ PRG_SET_PARAM,		4 },
	/*  ? */	{ PRG_TEST_CMD_OPEN,	0 },
	/*  ? */	{ PRG_RIFTRUE,			3 },
	/*  ? */	{ PRG_TIMEOUT,			1 },
	/*  ? */	{ PRG_RGOTO,			-3 },
	/*  ? */	{ PRG_OEM_COMMAND,		WLAN_TRP_ADP_OPR_GET_CMD },

	/*  ? */	{ PRG_SET_PARAM,		5 },
	/*  ? */	{ PRG_TEST_CMD_OPEN,	0 },
	/*  ? */	{ PRG_RIFTRUE,			3 },
	/*  ? */	{ PRG_TIMEOUT,			1 },
	/*  ? */	{ PRG_RGOTO,			-3 },
	/*  ? */	{ PRG_OEM_COMMAND,		WLAN_TRP_ADP_OPR_GET_CMD },

	/*  ? */	{ PRG_TIMEOUT,			5 },

	/*********************************************************************/
	/** Reassociate                                                     **/
	/*********************************************************************/

	/*  ? */	{ PRG_TEST_CMD_OPEN,	0 },
	/*  ? */	{ PRG_RIFTRUE,			3 },
	/*  ? */	{ PRG_TIMEOUT,			1 },
	/*  ? */	{ PRG_RGOTO,			-3 },
	/*  ? */	{ PRG_OEM_COMMAND,		WLAN_TRP_ADP_REASSOC_CMD },

	/*  ? */	{ PRG_TIMEOUT,			5 },

	/*********************************************************************/
	/** Deauthenticate                                                  **/
	/*********************************************************************/

	/*  ? */	{ PRG_TEST_CMD_OPEN,	0 },
	/*  ? */	{ PRG_RIFTRUE,			3 },
	/*  ? */	{ PRG_TIMEOUT,			1 },
	/*  ? */	{ PRG_RGOTO,			-3 },
	/*  ? */	{ PRG_OEM_COMMAND,		WLAN_TRP_ADP_DEAUTH_CMD },

	/*  ? */	{ PRG_TIMEOUT,			5 },

	/*  ? */	{ PRG_SET_PARAM,		0 },
	/*  ? */	{ PRG_TEST_CMD_OPEN,	0 },
	/*  ? */	{ PRG_RIFTRUE,			3 },
	/*  ? */	{ PRG_TIMEOUT,			1 },
	/*  ? */	{ PRG_RGOTO,			-3 },
	/*  ? */	{ PRG_OEM_COMMAND,		WLAN_TRP_ADP_SCAN_CMD },

	/*  ? */	{ PRG_TEST_CMD_OPEN,	0 },
	/*  ? */	{ PRG_RIFTRUE,			3 },
	/*  ? */	{ PRG_TIMEOUT,			1 },
	/*  ? */	{ PRG_RGOTO,			-3 },
	/*  ? */	{ PRG_OEM_COMMAND,		WLAN_TRP_ADP_JOIN_CMD },

	/*  ? */	{ PRG_TEST_CMD_OPEN,	0 },
	/*  ? */	{ PRG_RIFTRUE,			3 },
	/*  ? */	{ PRG_TIMEOUT,			1 },
	/*  ? */	{ PRG_RGOTO,			-3 },
	/*  ? */	{ PRG_OEM_COMMAND,		WLAN_TRP_ADP_AUTH_CMD },

	/*  ? */	{ PRG_TEST_CMD_OPEN,	0 },
	/*  ? */	{ PRG_RIFTRUE,			3 },
	/*  ? */	{ PRG_TIMEOUT,			1 },
	/*  ? */	{ PRG_RGOTO,			-3 },
	/*  ? */	{ PRG_OEM_COMMAND,		WLAN_TRP_ADP_ASSOC_CMD },

	/*  ? */	{ PRG_TIMEOUT,			5 },

	/*********************************************************************/
	/** Power Management                                                **/
	/*********************************************************************/

	/*  ? */	{ PRG_TEST_CMD_OPEN,	0 },
	/*  ? */	{ PRG_RIFTRUE,			3 },
	/*  ? */	{ PRG_TIMEOUT,			1 },
	/*  ? */	{ PRG_RGOTO,			-3 },
	/*  ? */	{ PRG_OEM_COMMAND,		WLAN_TRP_ADP_PWR_MGMT_CMD },

	/*  ? */	{ PRG_TIMEOUT,			5 },

	/*********************************************************************/
	/** MIB set and get (not supported)                                 **/
	/*********************************************************************/

	/*  ? */	{ PRG_TEST_CMD_OPEN,	0 },
	/*  ? */	{ PRG_RIFTRUE,			3 },
	/*  ? */	{ PRG_TIMEOUT,			1 },
	/*  ? */	{ PRG_RGOTO,			-3 },
	/*  ? */	{ PRG_OEM_COMMAND,		WLAN_TRP_ADP_MIB_GET_CMD },

	/*  ? */	{ PRG_TEST_CMD_OPEN,	0 },
	/*  ? */	{ PRG_RIFTRUE,			3 },
	/*  ? */	{ PRG_TIMEOUT,			1 },
	/*  ? */	{ PRG_RGOTO,			-3 },
	/*  ? */	{ PRG_OEM_COMMAND,		WLAN_TRP_ADP_MIB_SET_CMD },

	/*  ? */	{ PRG_TIMEOUT,			5 },

	/*********************************************************************/
	/** Disassociate                                                    **/
	/*********************************************************************/

	/*  ? */	{ PRG_TEST_CMD_OPEN,	0 },
	/*  ? */	{ PRG_RIFTRUE,			3 },
	/*  ? */	{ PRG_TIMEOUT,			1 },
	/*  ? */	{ PRG_RGOTO,			-3 },
	/*  ? */	{ PRG_OEM_COMMAND,		WLAN_TRP_ADP_DISASSOC_CMD },

	/*  ? */	{ PRG_TIMEOUT,			5 },

	/*********************************************************************/
	/** Misc commands                                                   **/
	/*********************************************************************/

	/*  ? */	{ PRG_TEST_CMD_OPEN,	0 },
	/*  ? */	{ PRG_RIFTRUE,			3 },
	/*  ? */	{ PRG_TIMEOUT,			1 },
	/*  ? */	{ PRG_RGOTO,			-3 },
	/*  ? */	{ PRG_OEM_COMMAND,		WLAN_TRP_ADP_MAC_CFG_CMD },

	/*  ? */	{ PRG_TIMEOUT,			5 },

	/*********************************************************************/
	/** Security                                                        **/
	/*********************************************************************/

	/*  ? */	{ PRG_TEST_CMD_OPEN,	0 },
	/*  ? */	{ PRG_RIFTRUE,			3 },
	/*  ? */	{ PRG_TIMEOUT,			1 },
	/*  ? */	{ PRG_RGOTO,			-3 },
	/*  ? */	{ PRG_OEM_COMMAND,		WLAN_TRP_ADP_WPA_SETKEYS_CMD },

	/*  ? */	{ PRG_TEST_CMD_OPEN,	0 },
	/*  ? */	{ PRG_RIFTRUE,			3 },
	/*  ? */	{ PRG_TIMEOUT,			1 },
	/*  ? */	{ PRG_RGOTO,			-3 },
	/*  ? */	{ PRG_OEM_COMMAND,		WLAN_TRP_ADP_WPA_DELKEYS_CMD },

	/*  ? */	{ PRG_TEST_CMD_OPEN,	0 },
	/*  ? */	{ PRG_RIFTRUE,			3 },
	/*  ? */	{ PRG_TIMEOUT,			1 },
	/*  ? */	{ PRG_RGOTO,			-3 },
	/*  ? */	{ PRG_OEM_COMMAND,		WLAN_TRP_ADP_WPA_SETKEYS_CMD },

	/*  ? */	{ PRG_TEST_CMD_OPEN,	0 },
	/*  ? */	{ PRG_RIFTRUE,			3 },
	/*  ? */	{ PRG_TIMEOUT,			1 },
	/*  ? */	{ PRG_RGOTO,			-3 },
	/*  ? */	{ PRG_OEM_COMMAND,		WLAN_TRP_ADP_WPA_SETPROTECT_CMD },

	/*  ? */	{ PRG_TEST_CMD_OPEN,	0 },
	/*  ? */	{ PRG_RIFTRUE,			3 },
	/*  ? */	{ PRG_TIMEOUT,			1 },
	/*  ? */	{ PRG_RGOTO,			-3 },
	/*  ? */	{ PRG_OEM_COMMAND,		WLAN_TRP_ADP_JOIN_CMD },

	/*  ? */	{ PRG_TEST_CMD_OPEN,	0 },
	/*  ? */	{ PRG_RIFTRUE,			3 },
	/*  ? */	{ PRG_TIMEOUT,			1 },
	/*  ? */	{ PRG_RGOTO,			-3 },
	/*  ? */	{ PRG_OEM_COMMAND,		WLAN_TRP_ADP_AUTH_CMD },

	/*  ? */	{ PRG_TEST_CMD_OPEN,	0 },
	/*  ? */	{ PRG_RIFTRUE,			3 },
	/*  ? */	{ PRG_TIMEOUT,			1 },
	/*  ? */	{ PRG_RGOTO,			-3 },
	/*  ? */	{ PRG_OEM_COMMAND,		WLAN_TRP_ADP_ASSOC_CMD },

	/*  ? */	{ PRG_TIMEOUT,			5 },

	/*********************************************************************/
	/** Reset                                                           **/
	/*********************************************************************/

	/*  ? */	{ PRG_TEST_CMD_OPEN,	0 },
	/*  ? */	{ PRG_RIFTRUE,			3 },
	/*  ? */	{ PRG_TIMEOUT,			1 },
	/*  ? */	{ PRG_RGOTO,			-3 },
	/*  ? */	{ PRG_OEM_COMMAND,		WLAN_TRP_ADP_RESET_CMD },

	/*  ? */	{ PRG_TIMEOUT,			5 },

	/*********************************************************************/
	/** Finish up                                                       **/
	/*********************************************************************/

	/*  ? */	{ PRG_TEST_CMD_OPEN,	0 },
	/*  ? */	{ PRG_RIFTRUE,			3 },
	/*  ? */	{ PRG_TIMEOUT,			1 },
	/*  ? */	{ PRG_RGOTO,			-3 },
	/*  ? */	{ PRG_OEM_COMMAND,		WLAN_TRP_ADP_DISASSOC_CMD },

    /*********************************************************************/
    /** Stop                                                            **/
    /*********************************************************************/

    /*  ? */    { PRG_OEM_FUNCTION,     2 },    /* stop */
    
	/*  ? */	{ PRG_TIMEOUT,			4 },
	/*  ? */	{ PRG_QUIT,				0 },

	/*  ? */	{ PRG_TIMEOUT,			5 },
	/*  ? */	{ PRG_RGOTO,			-1 },
};

int prg3[][2] =
{
	/*********************************************************************/
	/** Send (and receive) a single packet - Also test start/stop       **/
	/*********************************************************************/

	/*********************************************************************/
	/** Initialize and bring up                                         **/
	/*********************************************************************/

	/*  ? */	{ PRG_TIMEOUT,			1 },
	/*  ? */	{ PRG_OEM_INIT,			0 },
	/*  ? */	{ PRG_TIMEOUT,			1 },

	/*  ? */	{ PRG_SET_LOOP,			5 },	/* Start a loop */
    /*  ? */    { PRG_OEM_FUNCTION,     1 },    /* start */
    
    /*  ? */    { PRG_TEST_READY,       0 },
    /*  ? */    { PRG_RIFTRUE,          3 },
    /*  ? */    { PRG_TIMEOUT,          1 },
    /*  ? */    { PRG_RGOTO,            -3 },

    /*********************************************************************/
    /** Scan                                                            **/
    /*********************************************************************/

    /*  ? */    { PRG_SET_PARAM,        0 },
    /*  ? */    { PRG_TEST_CMD_OPEN,    0 },
    /*  ? */    { PRG_RIFTRUE,          3 },
    /*  ? */    { PRG_TIMEOUT,          1 },
    /*  ? */    { PRG_RGOTO,            -3 },
    /*  ? */    { PRG_OEM_COMMAND,      WLAN_TRP_ADP_SCAN_CMD },

    /*  ? */    { PRG_TIMEOUT,          1 },

    /*********************************************************************/
    /** Connect                                                         **/
    /*********************************************************************/

    /*  ? */    { PRG_TEST_CMD_OPEN,    0 },
    /*  ? */    { PRG_RIFTRUE,          3 },
    /*  ? */    { PRG_TIMEOUT,          1 },
    /*  ? */    { PRG_RGOTO,            -3 },
    /*  ? */    { PRG_OEM_COMMAND,      WLAN_TRP_ADP_JOIN_CMD },

    /*  ? */    { PRG_TEST_CMD_OPEN,    0 },
    /*  ? */    { PRG_RIFTRUE,          3 },
    /*  ? */    { PRG_TIMEOUT,          1 },
    /*  ? */    { PRG_RGOTO,            -3 },
    /*  ? */    { PRG_OEM_COMMAND,      WLAN_TRP_ADP_AUTH_CMD },

    /*  ? */    { PRG_TEST_CMD_OPEN,    0 },
    /*  ? */    { PRG_RIFTRUE,          3 },
    /*  ? */    { PRG_TIMEOUT,          1 },
    /*  ? */    { PRG_RGOTO,            -3 },
    /*  ? */    { PRG_OEM_COMMAND,      WLAN_TRP_ADP_ASSOC_CMD },

    /*  ? */    { PRG_TIMEOUT,          1 },
    
    /*********************************************************************/
    /** Create the Pstreams                                             **/
    /*********************************************************************/

    /*  ? */    { PRG_TIMEOUT,          2 },

    /*  ? */    { PRG_SET_PARAM,        WMM_AC_VI },
    /*  ? */    { PRG_TEST_CMD_OPEN,    0 },
    /*  ? */    { PRG_RIFTRUE,          3 },
    /*  ? */    { PRG_TIMEOUT,          1 },
    /*  ? */    { PRG_RGOTO,            -3 },
    /*  ? */    { PRG_OEM_COMMAND,      WLAN_TRP_ADP_ADDTS_CMD },

    /*  ? */    { PRG_TIMEOUT,          2 },

    /*  ? */    { PRG_SET_PARAM,        WMM_AC_VO },
    /*  ? */    { PRG_TEST_CMD_OPEN,    0 },
    /*  ? */    { PRG_RIFTRUE,          3 },
    /*  ? */    { PRG_TIMEOUT,          1 },
    /*  ? */    { PRG_RGOTO,            -3 },
    /*  ? */    { PRG_OEM_COMMAND,      WLAN_TRP_ADP_ADDTS_CMD },

    /*  ? */    { PRG_TIMEOUT,          2 },
        
    /*********************************************************************/
    /** Send a packet                                                   **/
    /*********************************************************************/
	/*  ? */	{ PRG_SET_PARAM,		-2 },	/* How many to "pull" 50 */
	/*  ? */	{ PRG_SET_LOOP,			50 },	/* How many packets to tx 50 */
    /*  ? */    { PRG_TEST_TX_OPEN,     0 },
    /*  ? */    { PRG_RIFTRUE,          3 },
    /*  ? */    { PRG_TIMEOUT,          1 },
    /*  ? */    { PRG_RGOTO,            -3 },
    /*  ? */    { PRG_SEND_PKT,         0 },
    /*  ? */    { PRG_LOOP,             0 },

    /*  ? */    { PRG_TIMEOUT,          1 },
    
    /*********************************************************************/
    /** Finish up                                                       **/
    /*********************************************************************/
    
    /*  ? */    { PRG_SET_PARAM,        4 },
    /*  ? */    { PRG_TEST_CMD_OPEN,    0 },
    /*  ? */    { PRG_RIFTRUE,          3 },
    /*  ? */    { PRG_TIMEOUT,          1 },
    /*  ? */    { PRG_RGOTO,            -3 },
    /*  ? */    { PRG_OEM_COMMAND,      WLAN_TRP_ADP_DELTS_CMD },
    
    /*  ? */    { PRG_SET_PARAM,        6 },
    /*  ? */    { PRG_TEST_CMD_OPEN,    0 },
    /*  ? */    { PRG_RIFTRUE,          3 },
    /*  ? */    { PRG_TIMEOUT,          1 },
    /*  ? */    { PRG_RGOTO,            -3 },
    /*  ? */    { PRG_OEM_COMMAND,      WLAN_TRP_ADP_DELTS_CMD },

	/*  ? */	{ PRG_TEST_CMD_OPEN,	0 },
	/*  ? */	{ PRG_RIFTRUE,			3 },
	/*  ? */	{ PRG_TIMEOUT,			1 },
	/*  ? */	{ PRG_RGOTO,			-3 },
	/*  ? */	{ PRG_OEM_COMMAND,		WLAN_TRP_ADP_DISASSOC_CMD },
	/*  ? */	{ PRG_TIMEOUT,			1 }, /* Make sure we give it time */    

    /*********************************************************************/
    /** Stop                                                            **/
    /*********************************************************************/

    /*  ? */    { PRG_OEM_FUNCTION,     2 },    /* stop */

    /*********************************************************************/
    /** Finish up                                                       **/
    /*********************************************************************/

    /*  ? */    { PRG_TIMEOUT,          4 },
    /*  ? */    { PRG_QUIT,             0 },
};

int prg4[][2] =
{
	/*********************************************************************/
	/** Start and stop                                                  **/
	/*********************************************************************/

	/*********************************************************************/
	/** Initialize and bring up                                         **/
	/*********************************************************************/
	
	/*  ? */	{ PRG_TIMEOUT,			1 },
	/*  ? */	{ PRG_OEM_INIT,			0 },

	/*  ? */	{ PRG_SET_LOOP,			5 },	/* We run this 5 times */
	
	/*  ? */	{ PRG_TIMEOUT,			1 },
	/*  ? */	{ PRG_OEM_FUNCTION,		1 },	/* start */

	/*  ? */	{ PRG_TEST_READY,		0 },
	/*  ? */	{ PRG_RIFTRUE,			3 },
	/*  ? */	{ PRG_TIMEOUT,			1 },
	/*  ? */	{ PRG_RGOTO,			-3 },

	/*********************************************************************/
	/** Scan                                                            **/
	/*********************************************************************/

	/*  ? */	{ PRG_SET_PARAM,		0 },
	/*  ? */	{ PRG_TEST_CMD_OPEN,	0 },
	/*  ? */	{ PRG_RIFTRUE,			3 },
	/*  ? */	{ PRG_TIMEOUT,			1 },
	/*  ? */	{ PRG_RGOTO,			-3 },
	/*  ? */	{ PRG_OEM_COMMAND,		WLAN_TRP_ADP_SCAN_CMD },

	/*  ? */	{ PRG_TIMEOUT,			5 },

	/*********************************************************************/
	/** Stop                                                            **/
	/*********************************************************************/

	/*  ? */	{ PRG_OEM_FUNCTION,		2 },	/* stop */
	/*  ? */	{ PRG_LOOP,				0 },

	/*********************************************************************/
	/** Finish up                                                       **/
	/*********************************************************************/

	/*  ? */	{ PRG_TIMEOUT,			2 },
	/*  ? */	{ PRG_QUIT,				0 },
};

int prg5[][2] =
{
	/*********************************************************************/
	/** Test mode                                                       **/
	/*********************************************************************/

	/*********************************************************************/
	/** Initialize and bring up                                         **/
	/*********************************************************************/

	/*  ? */    { PRG_TIMEOUT,          1 }, 
	/*  ? */    { PRG_OEM_INIT,         0 },
	/*  ? */    { PRG_TIMEOUT,          1 }, 
	/*  ? */    { PRG_OEM_FUNCTION,     3 },    /* start- in test mode */

	/*  ? */    { PRG_TEST_READY,       0 },
	/*  ? */    { PRG_RIFTRUE,          3 },
	/*  ? */    { PRG_TIMEOUT,          1 },
	/*  ? */    { PRG_RGOTO,            -3 },

#ifdef CONFIG_HOST_TCMD_SUPPORT
    /*  ? */    { PRG_SET_LOOP,         20 },

/* SEND test CMD  */
	/*  ? */    { PRG_SET_TEST_PARAM,   1 }, /*start cont tx data */
	/*  ? */    { PRG_TEST_CMD_OPEN,    0 },
	/*  ? */    { PRG_RIFTRUE,          3 },
	/*  ? */    { PRG_TIMEOUT,          1 },
	/*  ? */    { PRG_RGOTO,            -3 },
	/*  ? */    { PRG_OEM_COMMAND,      WLAN_TRP_ADP_TEST_CMD },
	/*  ? */    { PRG_TIMEOUT,          3 },
    
	/*  ? */    { PRG_SET_TEST_PARAM,    2 }, /*stop  cont tx data */
	/*  ? */    { PRG_TEST_CMD_OPEN,     0 },
	/*  ? */    { PRG_RIFTRUE,           3 },
	/*  ? */    { PRG_TIMEOUT,           1 },
	/*  ? */    { PRG_RGOTO,            -3 },
	/*  ? */    { PRG_OEM_COMMAND,      WLAN_TRP_ADP_TEST_CMD },
	/*  ? */    { PRG_TIMEOUT,          2 },
    
    /*  ? */    { PRG_LOOP,             0 },

    /*  ? */    { PRG_SET_LOOP,         20 },
/* Cont RX */

	/*  ? */    { PRG_SET_TEST_PARAM,   3 }, /*start cont RX in promi mode  */
	/*  ? */    { PRG_TEST_CMD_OPEN,    0 },
	/*  ? */    { PRG_RIFTRUE,          3 },
	/*  ? */    { PRG_TIMEOUT,          1 },
	/*  ? */    { PRG_RGOTO,            -3 },
	/*  ? */    { PRG_OEM_COMMAND,      WLAN_TRP_ADP_TEST_CMD },
	/*  ? */    { PRG_TIMEOUT,          3 },

	/*  ? */    { PRG_SET_TEST_PARAM,    4 },/*stop cont Rx and receive report*/
	/*  ? */    { PRG_TEST_CMD_OPEN,     0 },
	/*  ? */    { PRG_RIFTRUE,           3 },
	/*  ? */    { PRG_TIMEOUT,           1 },
	/*  ? */    { PRG_RGOTO,            -3 },
	/*  ? */    { PRG_OEM_COMMAND,      WLAN_TRP_ADP_TEST_CMD },
	/*  ? */    { PRG_TIMEOUT,           2 },
    
    /*  ? */    { PRG_LOOP,             0 },
#endif

	/*********************************************************************/
	/** Stop                                                            **/
	/*********************************************************************/

	/*  ? */    { PRG_OEM_FUNCTION,     2 },    /* stop */

	/*********************************************************************/
	/** Finish up                                                       **/
	/*********************************************************************/

	/*  ? */    { PRG_TIMEOUT,          10 },
	/*  ? */    { PRG_QUIT,             0 },
};


int prg6[][2] =
{
	/*********************************************************************/
	/** Firmware upload                                                 **/
	/*********************************************************************/

	/*********************************************************************/
	/** Initialize and bring up                                         **/
	/*********************************************************************/

	/*  ? */	{ PRG_TIMEOUT,			1 },
	/*  ? */	{ PRG_OEM_INIT,			0 },

	/*  ? */	{ PRG_TIMEOUT,			1 },
	/*  ? */	{ PRG_OEM_FUNCTION,		4 },	/* enable fw upload */
	/*  ? */	{ PRG_OEM_FUNCTION,		1 },	/* start */

	/*  ? */	{ PRG_TEST_READY,		0 },
	/*  ? */	{ PRG_RIFTRUE,			3 },
	/*  ? */	{ PRG_TIMEOUT,			1 },
	/*  ? */	{ PRG_RGOTO,			-3 },

	/*********************************************************************/
	/** Scan                                                            **/
	/*********************************************************************/

	/*  ? */	{ PRG_SET_PARAM,		0 },
	/*  ? */	{ PRG_TEST_CMD_OPEN,	0 },
	/*  ? */	{ PRG_RIFTRUE,			3 },
	/*  ? */	{ PRG_TIMEOUT,			1 },
	/*  ? */	{ PRG_RGOTO,			-3 },
	/*  ? */	{ PRG_OEM_COMMAND,		WLAN_TRP_ADP_SCAN_CMD },
	/*  ? */	{ PRG_TIMEOUT,			2 },

    /*********************************************************************/
    /** Connect                                                         **/
    /*********************************************************************/

    /*  ? */    { PRG_TEST_CMD_OPEN,    0 },
    /*  ? */    { PRG_RIFTRUE,          3 },
    /*  ? */    { PRG_TIMEOUT,          1 },
    /*  ? */    { PRG_RGOTO,            -3 },
    /*  ? */    { PRG_OEM_COMMAND,      WLAN_TRP_ADP_JOIN_CMD },

    /*  ? */    { PRG_TEST_CMD_OPEN,    0 },
    /*  ? */    { PRG_RIFTRUE,          3 },
    /*  ? */    { PRG_TIMEOUT,          1 },
    /*  ? */    { PRG_RGOTO,            -3 },
    /*  ? */    { PRG_OEM_COMMAND,      WLAN_TRP_ADP_AUTH_CMD },

    /*  ? */    { PRG_TEST_CMD_OPEN,    0 },
    /*  ? */    { PRG_RIFTRUE,          3 },
    /*  ? */    { PRG_TIMEOUT,          1 },
    /*  ? */    { PRG_RGOTO,            -3 },
    /*  ? */    { PRG_OEM_COMMAND,      WLAN_TRP_ADP_ASSOC_CMD },

    /*  ? */    { PRG_TIMEOUT,          1 },

    /*********************************************************************/
    /** Send a packet                                                   **/
    /*********************************************************************/

    /*  ? */    { PRG_SET_PARAM,        -1 },   /* How many to "pull" 50 */
    /*  ? */    { PRG_SET_LOOP,         50 },   /* How many packets to tx 50 */
    /*  ? */    { PRG_TEST_TX_OPEN,     0 },
    /*  ? */    { PRG_RIFTRUE,          3 },
    /*  ? */    { PRG_TIMEOUT,          1 },
    /*  ? */    { PRG_RGOTO,            -3 },
    /*  ? */    { PRG_SEND_PKT,         0 },
    /*  ? */    { PRG_LOOP,             0 },

    /*  ? */    { PRG_TIMEOUT,          2 },

    /*********************************************************************/
    /** Finish up                                                       **/
    /*********************************************************************/

    /*  ? */    { PRG_TEST_CMD_OPEN,    0 },
    /*  ? */    { PRG_RIFTRUE,          3 },
    /*  ? */    { PRG_TIMEOUT,          1 },
    /*  ? */    { PRG_RGOTO,            -3 },
    /*  ? */    { PRG_OEM_COMMAND,      WLAN_TRP_ADP_DISASSOC_CMD },
    
	/*********************************************************************/
	/** Stop                                                            **/
	/*********************************************************************/

	/*  ? */	{ PRG_OEM_FUNCTION,		2 },	/* stop */

	/*********************************************************************/
	/** Finish up                                                       **/
	/*********************************************************************/

	/*  ? */	{ PRG_QUIT,				0 },
};

int prg7[][2] =
{
		/*********************************************************************/
		/** Data transmit multiple packets                                  **/
		/*********************************************************************/

		/*********************************************************************/
		/** Initialize and bring up                                         **/
		/*********************************************************************/

		/*  ? */	{ PRG_TIMEOUT,			1 },
		/*  ? */	{ PRG_OEM_INIT,			0 },
		/*  ? */	{ PRG_TIMEOUT,			1 },
		/*  ? */	{ PRG_OEM_FUNCTION,		1 },	/* start */

		/*  ? */	{ PRG_TEST_READY,		0 },
		/*  ? */	{ PRG_RIFTRUE,			3 },
		/*  ? */	{ PRG_TIMEOUT,			1 },
		/*  ? */	{ PRG_RGOTO,			-3 },

		/*********************************************************************/
		/** BT HCI Interactive Commands/Data                                **/
		/*********************************************************************/

		/*  ? */	{ PRG_TEST_CMD_OPEN,	0 },
		/*  ? */	{ PRG_RIFTRUE,			3 },
		/*  ? */	{ PRG_TIMEOUT,			1 },
		/*  ? */	{ PRG_RGOTO,			-2 },

		/*  ? */	{ PRG_OEM_COMMAND,		WLAN_TRP_ADP_BT_DISPATCHER_CMD },
		
		/*  ? */	{ PRG_SET_PARAM,		3 },
		/*  ? */	{ PRG_OEM_COMMAND,		WLAN_TRP_ADP_BTHCI_CMD },

		/*  ? */	{ PRG_TIMEOUT,			2 },
		
		/*  ? */	{ PRG_SET_PARAM,		2 },
		/*  ? */	{ PRG_OEM_COMMAND,		WLAN_TRP_ADP_BTHCI_CMD },
		
		/*  ? */	{ PRG_TIMEOUT,			120 },
		
	    /*********************************************************************/
	    /** Stop                                                            **/
	    /*********************************************************************/
		
	    /*  ? */    { PRG_OEM_FUNCTION,     2 },    /* stop */
	    
		/*  ? */	{ PRG_TIMEOUT,			4 },
		/*  ? */	{ PRG_QUIT,				0 },
};

int prg8[][2] =
{
		/*********************************************************************/
		/** Data transmit multiple packets                                  **/
		/*********************************************************************/

		/*********************************************************************/
		/** Initialize and bring up                                         **/
		/*********************************************************************/

		/*  ? */	{ PRG_TIMEOUT,			1 },
		/*  ? */	{ PRG_OEM_INIT,			0 },
		/*  ? */	{ PRG_TIMEOUT,			1 },
		/*  ? */	{ PRG_OEM_FUNCTION,		1 },	/* start */

		/*  ? */	{ PRG_TEST_READY,		0 },
		/*  ? */	{ PRG_RIFTRUE,			3 },
		/*  ? */	{ PRG_TIMEOUT,			1 },
		/*  ? */	{ PRG_RGOTO,			-3 },

		/*********************************************************************/
		/** BT HCI Interactive Commands/Data                                **/
		/*********************************************************************/

		/*  ? */	{ PRG_TEST_CMD_OPEN,	0 },
		/*  ? */	{ PRG_RIFTRUE,			3 },
		/*  ? */	{ PRG_TIMEOUT,			1 },
		/*  ? */	{ PRG_RGOTO,			-2 },

		/*  ? */	{ PRG_OEM_COMMAND,		WLAN_TRP_ADP_BT_DISPATCHER_CMD },
		
		/*  ? */	{ PRG_SET_PARAM,		4 },
		/*  ? */	{ PRG_OEM_COMMAND,		WLAN_TRP_ADP_BTHCI_CMD },

		/*  ? */	{ PRG_TIMEOUT,			2 },
		
		/*  ? */	{ PRG_SET_PARAM,		15 },
		/*  ? */	{ PRG_OEM_COMMAND,		WLAN_TRP_ADP_BTHCI_CMD },
		
		/*  ? */	{ PRG_TIMEOUT,			10 },

		/*  ? */	{ PRG_SET_PARAM,		5 },
		/*  ? */	{ PRG_OEM_COMMAND,		WLAN_TRP_ADP_BTHCI_CMD },

		/*  ? */	{ PRG_TIMEOUT,			5 },
		
		/*  ? */	{ PRG_SET_PARAM,		51 },
		/*  ? */	{ PRG_SET_LOOP,			25 },
		/*  ? */	{ PRG_OEM_COMMAND,		WLAN_TRP_ADP_BTHCI_CMD },
		/*  ? */	{ PRG_LOOP,				0 },
		
		/*  ? */	{ PRG_TIMEOUT,			10 },
		
	    /*********************************************************************/
	    /** Stop                                                            **/
	    /*********************************************************************/

	    /*  ? */    { PRG_OEM_FUNCTION,     2 },    /* stop */
	    
		/*  ? */	{ PRG_TIMEOUT,			4 },
		/*  ? */	{ PRG_QUIT,				0 },
};

int (*prgp)[2] = NULL; 

#if 0
Frame 1 (42 bytes on wire, 42 bytes captured)
    Arrival Time: Dec 16, 2005 11:45:40.206159000
    Time delta from previous packet: 0.000000000 seconds
    Time since reference or first frame: 0.000000000 seconds
    Frame Number: 1
    Packet Length: 42 bytes
    Capture Length: 42 bytes
Ethernet II, Src: 00:03:47:79:ed:dc, Dst: ff:ff:ff:ff:ff:ff
    Destination: ff:ff:ff:ff:ff:ff (Broadcast)
    Source: 00:03:47:79:ed:dc (Intel_79:ed:dc)
    Type: ARP (0x0806)
Address Resolution Protocol (request)
    Hardware type: Ethernet (0x0001)
    Protocol type: IP (0x0800)
    Hardware size: 6
    Protocol size: 4
    Opcode: request (0x0001)
    Sender MAC address: 00:03:47:79:ed:dc (Intel_79:ed:dc)
    Sender IP address: 192.168.1.10 (192.168.1.10)
    Target MAC address: 00:00:00:00:00:00 (00:00:00_00:00:00)
    Target IP address: 192.168.1.30 (192.168.1.30)

0000  ff ff ff ff ff ff 00 03 47 79 ed dc 08 06 00 01   ........Gy......
0010  08 00 06 04 00 01 00 03 47 79 ed dc c0 a8 01 0a   ........Gy......
0020  00 00 00 00 00 00 c0 a8 01 1e                     ..........
#endif

/*
QCOM - 00:03:7f:01:60:df
AP   - 00:03:7f:01:40:01
*/

static unsigned char s_arp_pkt1[] =
{
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0x00, 0x03, 0x7f, 0x01, 0x60, 0xdf,
	0x08, 0x06, 0x00, 0x01,

	0x08, 0x00, 0x06, 0x04, 0x00, 0x01,
#if 0
	0x00, 0x03, 0x7f, 0x01, 0x60, 0xdf, 0xc0, 0xa8, 0x01, 0x0b,
#else
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xc0, 0xa8, 0x01, 0x0b,
#endif

	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0xa8, 0x01, 0x0a,
};

#if 0
Frame 858 (214 bytes on wire, 214 bytes captured)
Ethernet II, Src: 00:06:1b:dc:bb:24, Dst: 00:03:7f:0a:a0:0f
    Destination: 00:03:7f:0a:a0:0f (AtherosC_0a:a0:0f)
    Source: 00:06:1b:dc:bb:24 (Portable_dc:bb:24)
    Type: IP (0x0800)
Internet Protocol, Src Addr: 192.168.1.33 (192.168.1.33), Dst Addr: 192.168.1.55 (192.168.1.55)
    Version: 4
    Header length: 20 bytes
    Differentiated Services Field: 0x00 (DSCP 0x00: Default; ECN: 0x00)
    Total Length: 200
    Identification: 0xeb3f (60223)
    Flags: 0x00
    Fragment offset: 0
    Time to live: 128
    Protocol: UDP (0x11)
    Header checksum: 0xcb3c (correct)
    Source: 192.168.1.33 (192.168.1.33)
    Destination: 192.168.1.55 (192.168.1.55)
User Datagram Protocol, Src Port: 3197 (3197), Dst Port: 16384 (16384)
    Source port: 3197 (3197)
    Destination port: 16384 (16384)
    Length: 180
    Checksum: 0x0000 (none)
Data (172 bytes)
#endif

static unsigned char s_data_pkt[] =
{
0x00, 0x16, 0x41, 0xe2, 0xda, 0x00, 0x00, 0x03, 0x7f, 0x02, 0xa1, 0xee, 0x08, 0x00, 0x45, 0xe0,
0x00, 0xc8, 0x07, 0xea, 0x40, 0x00, 0x40, 0x11, 0xad, 0x91, 0xc0, 0xa8, 0x01, 0x37, 0xc0, 0xa8,
0x01, 0x42, 0x80, 0x54, 0x40, 0x00, 0x00, 0xb4, 0x00, 0x00, 0x80, 0x00, 0x08, 0x4e, 0x00, 0x06,
0x3f, 0xb8, 0xfd, 0x9f, 0xfa, 0x68, 0x00, 0x04, 0xf1, 0xa0, 0xfc, 0x03, 0xb9, 0x40, 0x43, 0x40,
0xea, 0x99, 0x7b, 0xaa, 0x90, 0xdc, 0xaf, 0x79, 0xde, 0x7f, 0xfd, 0xbc, 0xfc, 0x93, 0x81, 0x80,
0x5a, 0x4d, 0x2b, 0x8f, 0x2c, 0x62, 0xee, 0x36, 0x4a, 0xc5, 0x16, 0xb2, 0x6a, 0xfc, 0x40, 0x22,
0x87, 0x22, 0xb9, 0x04, 0x02, 0x3c, 0xaa, 0x42, 0xc7, 0x43, 0x90, 0x2e, 0xff, 0xc3, 0x08, 0xc2,
0x92, 0x44, 0x2f, 0x22, 0xb1, 0x68, 0x62, 0xd9, 0x30, 0x3a, 0x25, 0xa3, 0x53, 0x80, 0x65, 0xac,
0xf2, 0x98, 0x95, 0x0d, 0x5c, 0x9b, 0x70, 0x3f, 0x4f, 0xdf, 0x0c, 0x54, 0x9b, 0x2c, 0xf5, 0xb3,
0xf6, 0x20, 0x9b, 0x2a, 0x7a, 0x2b, 0x05, 0xdb, 0x2e, 0xfa, 0x50, 0x6b, 0x43, 0x21, 0xcc, 0xf6,
0xd5, 0x60, 0x3b, 0x78, 0x7a, 0xbc, 0xe7, 0x9e, 0x4d, 0xad, 0xb6, 0x5d, 0x15, 0x46, 0x6c, 0xfb,
0x67, 0x83, 0x5c, 0xf6, 0x61, 0x0b, 0xe7, 0x87, 0x22, 0x9c, 0x07, 0xc2, 0x5d, 0xf3, 0x52, 0x8a,
0x5d, 0x01, 0xe0, 0x7d, 0xc7, 0xf5, 0x17, 0x2b, 0x50, 0x05, 0x96, 0xdd, 0x67, 0x1d, 0xe2, 0x2d,
0x34, 0x62, 0xc2, 0x95, 0xa1, 0xda
};

extern int wlan_trp_oem_init(void);
extern void htc_task_signal(unsigned int val);

static int s_testval = -1;
static int s_param = 0;
#ifdef CONFIG_HOST_TCMD_SUPPORT
static int s_test_param =0;
#endif
static int s_loop = 0;
static int s_loop_pc = 0;
static char s_bss_name[] = "avd_ap";
static int bgmode = 1;

static int s_ctid = 6;		/* Arbitrary */
static int s_dtid = 6;      /* Arbitrary */
static int s_token = 1;		/* Arbitrary */

/* External prototypes to help functions related to stubs and emulation    */
/* We don't provide a specific header file since these are so few and      */
/* they are only glue functions to keep our emulation environment togheter */
int stubs_is_ready(void);
int wlan_ath_start(void);
int wlan_ath_stop(void);
wlan_dot11_bss_description_type *stubs_get_bss_desc_by_index(int i);
wlan_dot11_bss_description_type *stubs_get_bss_desc_by_name(char *name);

int (*find_program(int *prg_num))[2]
{ 
	int (*lprgp)[2] = NULL; 

	switch(*prg_num)
	{
        case 8:
    	    lprgp = prg8;
    	    break;
	    case 7:
	    	lprgp = prg7;
	    	break;
		case 6:
			lprgp = prg6;
			break;
		case 5:
			lprgp = prg5;
			break;
#ifdef CONFIG_HOST_TCMD_SUPPORT
		case 4:
			lprgp = prg4;
			break;
#endif
		case 3:
			lprgp = prg3;
			break;
		case 2:
			lprgp = prg2;
			break;
		case 1:
		default:
			lprgp = prg1;
			*prg_num = 1;
			break;
	}

	DPRINTF(DBG_SIMULATOR,
		(DBGFMT "Program 0x%08x %d\n", DBGARG, (unsigned int) lprgp, *prg_num));

	return(lprgp);
}

A_INT32
get_pal_file(int choice, A_UINT8 *pdu, A_UINT16 *sz)
{
    int fhdl;
    
    const char *pdu_bin[] = {
                    "hciCmds/HCI_Read_Local_AMP_Info_cmd.bin",
                    "hciCmds/HCI_Read_Local_AMP_ASSOC_cmd.bin",
                    "hciCmds/HCI_Write_Remote_AMP_ASSOC_cmd.bin",
                    "hciCmds/HCI_Create_Physical_Link_cmd.bin",
                    "hciCmds/HCI_Accept_Physical_Link_Req_cmd.bin",
                    "hciCmds/HCI_Create_Logical_Link_Req_cmd.bin",
                    "hciCmds/HCI_Accept_Logical_Link_Req_cmd.bin",
                    "hciCmds/HCI_Disconnect_Logical_Link_Req_cmd.bin",
                    "hciCmds/HCI_Disconnect_Physical_Link_Req_cmd.bin",
                    "hciCmds/HCI_Short_Range_Mode_cmd.bin",
                    "hciCmds/HCI_Read_Link_Quality_cmd.bin",
                    "hciCmds/HCI_Modify_Flow_Spec_cmd.bin",
                    "hciCmds/HCI_Flush_cmd.bin",
                    "hciCmds/HCI_Set_Event_Mask_cmd.bin",
                    "hciCmds/HCI_Set_Event_Mask_Page_2_cmd.bin",
                    "hciCmds/HCI_Write_Remote_AMP_ASSOC_cmd2.bin",
                    };
    const char *other_pdu_bin[] = {
                    "hciCmds/SendDataFrame.bin",
                    "hciCmds/SendDataFrame.bin",
                    "hciCmds/dummy",
                    "hciCmds/dummy",
                    };

    if(choice < 51) {   /* HCI cmds */
    	printf("file %s\n", pdu_bin[choice]);
        fhdl = open((const char *)pdu_bin[choice], O_RDONLY);
    } else if (choice >= 51) {
    	printf("file %s\n", other_pdu_bin[choice - 51]);
        fhdl = open((const char *)other_pdu_bin[choice - 51], O_RDONLY);
    }

    *sz  = read(fhdl, pdu, 512);

    close(fhdl);

    return choice;
}

extern void dump_frame(A_UINT8 *frm, A_UINT32 len);

int main(int argc, char *argv[])
{
	int rc;
	pthread_cond_t my_cond = PTHREAD_COND_INITIALIZER;
	pthread_mutex_t my_mutex = PTHREAD_MUTEX_INITIALIZER;
	struct timeval now;			/* Time when we started waiting        */
	struct timespec timeout;	/* Timeout value for the wait function */
	int done;					/* Are we done waiting?                */
	int my_timeout;
	int my_wait;
	int my_quit = 0;
	int pc;
	wlan_trp_adp_cmd_type cmd;
	int prg_num = 1;
    A_UINT8 bssid[ATH_MAC_LEN] = { 0, 0, 0, 0, 0, 0 };
    wlan_dot11_bss_description_type *bss_desc;
    TCMD_CONT_TX  tcmd_cont_tx;
    TCMD_CONT_RX tcmd_cont_rx;       
    unsigned int tcmd_freq = 0;
    
	/* Do we want to run a specific program */
	if(argc > 1)
	{
		prg_num = atoi(argv[1]);
	}

	prgp = find_program(&prg_num);

	DPRINTF(DBG_SIMULATOR, (DBGFMT "Starting %d\n", DBGARG, prg_num));

	/* Initialize random generator */
	srand(time(NULL));

	/* Set the program counter */
	pc = 0;

	for(;my_quit == 0;)
	{
		/* Reset timeout */
		my_timeout = 0;
		my_wait = 0;

		switch(prgp[pc][0])
		{
			case PRG_TIMEOUT:
				DPRINTF(DBG_SIMULATOR, (DBGFMT "PRG_TIMEOUT %d\n", DBGARG, prgp[pc][1]));
				my_timeout = prgp[pc][1];
				my_wait = 1;
				pc++;
				break;
			case PRG_GOTO:
				DPRINTF(DBG_SIMULATOR, (DBGFMT "PRG_GOTO %d\n", DBGARG, prgp[pc][1]));
				pc = prgp[pc][1];
				break;
			case PRG_RGOTO:
				DPRINTF(DBG_SIMULATOR, (DBGFMT "PRG_RGOTO %d\n", DBGARG, prgp[pc][1]));
				pc = pc + prgp[pc][1];
				break;
			case PRG_IFTRUE:
				DPRINTF(DBG_SIMULATOR, (DBGFMT "PRG_IFTRUE %d\n", DBGARG, prgp[pc][1]));
				pc = s_testval == TRUE ? prgp[pc][1] : pc + 1;
				break;
			case PRG_RIFTRUE:
				DPRINTF(DBG_SIMULATOR, (DBGFMT "PRG_RIFTRUE %d\n", DBGARG, prgp[pc][1]));
				pc = s_testval == TRUE ? pc + prgp[pc][1] : pc + 1;
				break;
			case PRG_IFFALSE:
				DPRINTF(DBG_SIMULATOR, (DBGFMT "PRG_IFFALSE %d\n", DBGARG, prgp[pc][1]));
				pc = s_testval == FALSE ? prgp[pc][1] : pc + 1;
				break;
			case PRG_RIFFALSE:
				DPRINTF(DBG_SIMULATOR, (DBGFMT "PRG_RIFFALSE %d\n", DBGARG, prgp[pc][1]));
				pc = s_testval == FALSE ? pc + prgp[pc][1] : pc + 1;
				break;
			case PRG_QUIT:
				DPRINTF(DBG_SIMULATOR, (DBGFMT "PRG_QUIT %d\n", DBGARG, prgp[pc][1]));
				my_quit++;
				pc++;
				break;
			case PRG_OEM_INIT:
				DPRINTF(DBG_SIMULATOR, (DBGFMT "PRG_OEM_INIT %d\n", DBGARG, prgp[pc][1]));
				wlan_trp_oem_init();
				pc++;
				break;
			case PRG_OEM_FUNCTION:
				DPRINTF(DBG_SIMULATOR, (DBGFMT "PRG_OEM_FUNCTION %d\n", DBGARG, prgp[pc][1]));
				switch(prgp[pc][1])
				{
					case 1:
						wlan_ath_start();
						break;
					case 2:
						wlan_ath_stop();
						break;
#ifdef CONFIG_HOST_TCMD_SUPPORT
					case 3:
						wlan_ath_test_start();
						break;
#endif
					case 4:
						wlan_ath_fw_upload();
						break;
				}
				pc++;
				break;
			case PRG_HIF_FUNCTION:
				DPRINTF(DBG_SIMULATOR, (DBGFMT "PRG_HIF_FUNCTION %d\n", DBGARG, prgp[pc][1]));
				switch(prgp[pc][1])
				{
					case 1:
						HIFHandleInserted();
						break;
					case 2:
						HIFHandleRemoved();
						break;
					case 3:
						HIFHandleIRQ();
						break;
					case 4:
						HIFHandleRW();
						break;
				}
				pc++;
				break;
			case PRG_HTC_SIGNAL:
				DPRINTF(DBG_SIMULATOR, (DBGFMT "PRG_HTC_SIGNAL %d\n", DBGARG, prgp[pc][1]));
				htc_task_signal(prgp[pc][1]);
				pc++;
				break;
			case PRG_TEST_TX_OPEN:
				DPRINTF(DBG_SIMULATOR, (DBGFMT "PRG_TEST_TX_OPEN %d\n", DBGARG, prgp[pc][1]));
				s_testval = wlan_oem_tx_open();
				pc++;
				break;
			case PRG_TEST_CMD_OPEN:
				DPRINTF(DBG_SIMULATOR, (DBGFMT "PRG_TEST_CMD_OPEN %d\n", DBGARG, prgp[pc][1]));
				s_testval = wlan_oem_cmd_open();
				pc++;
				break;
			case PRG_TEST_READY:
				DPRINTF(DBG_SIMULATOR, (DBGFMT "PRG_TEST_READY %d\n", DBGARG, prgp[pc][1]));
				s_testval = stubs_is_ready();
				pc++;
				break;
			case PRG_OEM_COMMAND:
				DPRINTF(DBG_SIMULATOR, (DBGFMT "PRG_OEM_COMMAND %d\n", DBGARG, prgp[pc][1]));
				A_MEMZERO(&cmd, sizeof(wlan_trp_adp_cmd_type));
				switch(prgp[pc][1])
				{
					case WLAN_TRP_ADP_SUSPEND_CMD:
						DPRINTF(DBG_SIMULATOR, (DBGFMT "WLAN_TRP_ADP_SUSPEND_CMD\n", DBGARG));
						break;
                        
					case WLAN_TRP_ADP_RESUME_CMD:
						DPRINTF(DBG_SIMULATOR, (DBGFMT "WLAN_TRP_ADP_RESUME_CMD\n", DBGARG));
						break;
                        
					case WLAN_TRP_ADP_SCAN_CMD:
						DPRINTF(DBG_SIMULATOR, (DBGFMT "WLAN_TRP_ADP_SCAN_CMD\n", DBGARG));
						
                        switch(s_param)
						{
                            case 3:
                                /* This network is not available ;-) */
                                strcpy(cmd.req_info.scan_info.ssid.ssid, "ulla");
                                cmd.req_info.scan_info.ssid.len = strlen("ulla");
                                break;
							case 2:
								/* The "Atheros Wireless Network" */
								strcpy(cmd.req_info.scan_info.ssid.ssid, "Atheros Wireless Network");
								cmd.req_info.scan_info.ssid.len = strlen("Atheros Wireless Network");
								break;
							case 1:
								/* The broadcast SSID */
								strcpy(cmd.req_info.scan_info.ssid.ssid, "");
								cmd.req_info.scan_info.ssid.len = 0;
								cmd.req_info.scan_info.channel_list.nb_channels = 3;
                                cmd.req_info.scan_info.channel_list.channel_list[0] = wlan_freq2ieee(2412);
                                cmd.req_info.scan_info.channel_list.channel_list[0] = wlan_freq2ieee(2437);
                                cmd.req_info.scan_info.channel_list.channel_list[0] = wlan_freq2ieee(2462);
								break;
							default:
								strcpy(cmd.req_info.scan_info.ssid.ssid, s_bss_name);
								cmd.req_info.scan_info.ssid.len = strlen(s_bss_name);
								break;
						}

						cmd.req_info.scan_info.bss_type = SYS_WLAN_BSS_TYPE_INFRA;
					    cmd.req_info.scan_info.bssid = wlan_util_macaddress_to_uint64(bssid);
						cmd.req_info.scan_info.scan_type = WLAN_SYS_SCAN_ACTIVE;
						cmd.req_info.scan_info.min_channel_time = 0;
						cmd.req_info.scan_info.max_channel_time = 0;
						break;

					case WLAN_TRP_ADP_JOIN_CMD:
						DPRINTF(DBG_SIMULATOR, (DBGFMT "WLAN_TRP_ADP_JOIN_CMD\n", DBGARG));
							
                        if((bss_desc =
							stubs_get_bss_desc_by_name(s_bss_name)) == NULL)
						{
						DPRINTF(DBG_SIMULATOR,
							(DBGFMT "Unable to find \"%s\"\n", DBGARG, s_bss_name));
							goto noaction;
						}

						A_MEMCPY(&cmd.req_info.join_info.ap_desc, bss_desc,
							sizeof(wlan_dot11_bss_description_type));

						cmd.req_info.join_info.akm = WPA_AKM_NONE;
#if 0
						cmd.req_info.join_info.akm = WPA_AKM_802_1X;
						cmd.req_info.join_info.akm = WPA_AKM_PSK;
#endif

						cmd.req_info.join_info.pairwise_cipher =
							WPA_CIPHER_SUITE_NONE;
#if 0
						cmd.req_info.join_info.pairwise_cipher =
							WPA_CIPHER_SUITE_WEP_40;
						cmd.req_info.join_info.pairwise_cipher =
							WPA_CIPHER_SUITE_TKIP;
						cmd.req_info.join_info.pairwise_cipher =
							WPA_CIPHER_SUITE_CCMP;
						cmd.req_info.join_info.pairwise_cipher =
							WPA_CIPHER_SUITE_WEP_104;
#endif
						cmd.req_info.join_info.group_cipher =
							WPA_CIPHER_SUITE_NONE;
#if 0
						cmd.req_info.join_info.group_cipher =
							WPA_CIPHER_SUITE_WEP_40;
						cmd.req_info.join_info.group_cipher =
							WPA_CIPHER_SUITE_TKIP;
						cmd.req_info.join_info.group_cipher =
							WPA_CIPHER_SUITE_CCMP;
						cmd.req_info.join_info.group_cipher =
							WPA_CIPHER_SUITE_WEP_104;
#endif
						break;
					case WLAN_TRP_ADP_AUTH_CMD:
						DPRINTF(DBG_SIMULATOR, (DBGFMT "WLAN_TRP_ADP_AUTH_CMD\n", DBGARG));

						if((bss_desc =
							stubs_get_bss_desc_by_name(s_bss_name)) == NULL)
						{
							DPRINTF(DBG_SIMULATOR,
								(DBGFMT "Unable to find BSS \"%s\"\n", DBGARG, s_bss_name));
							goto noaction;
						}

						cmd.req_info.auth_info.peer_mac_address = bss_desc->bssid;

						cmd.req_info.auth_info.auth_type = WLAN_SYS_AUTH_ALGO_OPEN_SYSTEM;
#if 0
						cmd.req_info.auth_info.auth_type = WLAN_SYS_AUTH_ALGO_SHARED_KEY;
#endif
						cmd.req_info.auth_info.auth_failure_timeout = 0;
						break;
                    
					case WLAN_TRP_ADP_ASSOC_CMD:
						DPRINTF(DBG_SIMULATOR, (DBGFMT "WLAN_TRP_ADP_ASSOC_CMD\n", DBGARG));

						if((bss_desc =
							stubs_get_bss_desc_by_name(s_bss_name)) == NULL)
						{
							DPRINTF(DBG_SIMULATOR,
								(DBGFMT "Unable to find BSS \"%s\"\n", DBGARG, s_bss_name));
							goto noaction;
						}

						cmd.req_info.assoc_info.peer_mac_address = bss_desc->bssid;
						A_MEMCPY(&cmd.req_info.assoc_info.cap_info,
							     &bss_desc->capability,
							     sizeof(wlan_dot11_cap_info_type));
						cmd.req_info.assoc_info.assoc_failure_timeout = 0;
						cmd.req_info.assoc_info.listen_interval = 0;
						A_MEMCPY(&cmd.req_info.assoc_info.wpa_ie,
							     &bss_desc->wpa_ie, sizeof(wpa_ie_type));
						break;
                        
					case WLAN_TRP_ADP_REASSOC_CMD:
						DPRINTF(DBG_SIMULATOR, (DBGFMT "WLAN_TRP_ADP_REASSOC_CMD\n", DBGARG));

						if((bss_desc =
							stubs_get_bss_desc_by_name(s_bss_name)) == NULL)
						{
							DPRINTF(DBG_SIMULATOR,
								(DBGFMT "Unable to find BSS \"%s\"\n", DBGARG, s_bss_name));
							goto noaction;
						}

						cmd.req_info.reassoc_info.new_ap_mac_address = bss_desc->bssid;
						A_MEMCPY(&cmd.req_info.reassoc_info.cap_info,
							     &bss_desc->capability,
							     sizeof(wlan_dot11_cap_info_type));
						cmd.req_info.reassoc_info.reassoc_failure_timeout = 0;
						cmd.req_info.reassoc_info.listen_interval = 0;
						A_MEMCPY(&cmd.req_info.reassoc_info.wpa_ie,
							     &bss_desc->wpa_ie, sizeof(wpa_ie_type));
						break;
                        
					case WLAN_TRP_ADP_DEAUTH_CMD:
						DPRINTF(DBG_SIMULATOR, (DBGFMT "WLAN_TRP_ADP_DEAUTH_CMD\n", DBGARG));
						
                        cmd.req_info.deauth_info.peer_mac_address = 0;
						cmd.req_info.deauth_info.reason_code =
							WLAN_SYS_REASON_UNSPECIFIED;
#if 0
						cmd.req_info.deauth_info.reason_code =
							WLAN_SYS_REASON_PREV_AUTH_NOT_VALID;
						cmd.req_info.deauth_info.reason_code =
							WLAN_SYS_REASON_DEAUTH_STA_LEFT;
						cmd.req_info.deauth_info.reason_code =
							WLAN_SYS_REASON_DISASSOC_INACTIF;
						cmd.req_info.deauth_info.reason_code =
							WLAN_SYS_REASON_DISASSOC_AP_CANNOT_SUP_STA;
						cmd.req_info.deauth_info.reason_code =
							WLAN_SYS_REASON_CLS2_FRM_RX_FROM_UNAUTH_STA;
						cmd.req_info.deauth_info.reason_code =
							WLAN_SYS_REASON_CLS3_FRM_RX_FROM_UNASSOC_STA;
						cmd.req_info.deauth_info.reason_code =
							WLAN_SYS_REASON_DISASSOC_STA_LEFT;
						cmd.req_info.deauth_info.reason_code =
							WLAN_SYS_REASON_REQ_FROM_NOT_AUTH_STA;
#endif
						break;
                        
					case WLAN_TRP_ADP_DISASSOC_CMD:
						DPRINTF(DBG_SIMULATOR, (DBGFMT "WLAN_TRP_ADP_DISASSOC_CMD\n", DBGARG));
                        
						cmd.req_info.disassoc_info.peer_mac_address = 0;
						cmd.req_info.disassoc_info.reason_code = WLAN_SYS_REASON_UNSPECIFIED;
#if 0
						cmd.req_info.disassoc_info.reason_code =
							WLAN_SYS_REASON_PREV_AUTH_NOT_VALID;
						cmd.req_info.disassoc_info.reason_code =
							WLAN_SYS_REASON_DEAUTH_STA_LEFT;
						cmd.req_info.disassoc_info.reason_code =
							WLAN_SYS_REASON_DISASSOC_INACTIF;
						cmd.req_info.disassoc_info.reason_code =
							WLAN_SYS_REASON_DISASSOC_AP_CANNOT_SUP_STA;
						cmd.req_info.disassoc_info.reason_code =
							WLAN_SYS_REASON_CLS2_FRM_RX_FROM_UNAUTH_STA;
						cmd.req_info.disassoc_info.reason_code =
							WLAN_SYS_REASON_CLS3_FRM_RX_FROM_UNASSOC_STA;
						cmd.req_info.disassoc_info.reason_code =
							WLAN_SYS_REASON_DISASSOC_STA_LEFT;
						cmd.req_info.disassoc_info.reason_code =
							WLAN_SYS_REASON_REQ_FROM_NOT_AUTH_STA;
#endif
						break;
                        
					case WLAN_TRP_ADP_PWR_MGMT_CMD:
						DPRINTF(DBG_SIMULATOR, (DBGFMT "WLAN_TRP_ADP_PWR_MGMT_CMD\n", DBGARG));
						cmd.req_info.pwr_mgmt_info.pwr_mode =
							WLAN_SYS_PWR_ACTIVE_MODE;
#if 0
						cmd.req_info.pwr_mgmt_info.pwr_mode =
							WLAN_SYS_PWR_SAVE_MODE;
#endif
						cmd.req_info.pwr_mgmt_info.wake_up = TRUE;
#if 0
						cmd.req_info.pwr_mgmt_info.wake_up = FALSE;
#endif
						cmd.req_info.pwr_mgmt_info.receiveDTIMs = FALSE;
#if 0
						cmd.req_info.pwr_mgmt_info.receiveDTIMs = TRUE;
#endif
						break;
                        
					case WLAN_TRP_ADP_MIB_GET_CMD:
						DPRINTF(DBG_SIMULATOR, (DBGFMT "WLAN_TRP_ADP_MIB_GET_CMD\n", DBGARG));
						/* Not supported */
						cmd.req_info.mib_info.item_id = 0;
						break;
                        
					case WLAN_TRP_ADP_MIB_SET_CMD:
						DPRINTF(DBG_SIMULATOR, (DBGFMT "WLAN_TRP_ADP_MIB_SET_CMD\n", DBGARG));
						/* Not supported */
						cmd.req_info.mib_info.item_id = 0;
						break;
                        
					case WLAN_TRP_ADP_OPR_GET_CMD:
						DPRINTF(DBG_SIMULATOR, (DBGFMT "WLAN_TRP_ADP_OPR_GET_CMD\n", DBGARG));
						switch(s_param)
						{
							case 0:
								cmd.req_info.opr_param_id = WLAN_RSSI;
								break;
							case 1:
								cmd.req_info.opr_param_id = WAN_LOGS;
								break;
							case 2:
								cmd.req_info.opr_param_id = WLAN_TX_FRAME_CNT;
								break;
							case 3:
								cmd.req_info.opr_param_id = WLAN_RX_FRAME_CNT;
								break;
							case 4:
								cmd.req_info.opr_param_id = WLAN_TX_RATE;
								break;
							case 5:
								cmd.req_info.opr_param_id = WLAN_TOTAL_RX_BYTE;
								break;
							default:
								cmd.req_info.opr_param_id = WLAN_TOTAL_RX_BYTE;
								break;
						}
						break;
                        
					case WLAN_TRP_ADP_RESET_CMD:
						DPRINTF(DBG_SIMULATOR, (DBGFMT "WLAN_TRP_ADP_RESET_CMD\n", DBGARG));
						{
							wlan_dot11_bss_description_type *bss_desc;

							if((bss_desc =
								stubs_get_bss_desc_by_name(s_bss_name)) == NULL)
							{
								DPRINTF(DBG_SIMULATOR,
									(DBGFMT "Unable to find BSS \"%s\"\n", DBGARG, s_bss_name));
								goto noaction;
							}

							cmd.req_info.reset_info.sta_address =
								bss_desc->bssid;
							cmd.req_info.reset_info.set_default_mib = FALSE;
#if 0
							cmd.req_info.reset_info.set_default_mib = TRUE;
#endif
						}
						break;
                        
					case WLAN_TRP_ADP_MAC_CFG_CMD:
						DPRINTF(DBG_SIMULATOR, (DBGFMT "WLAN_TRP_ADP_MAC_CFG_CMD\n", DBGARG));
						cmd.req_info.config_info.preamble_type =
							WLAN_DOT11_PHY_PREAMBLE_SHORT;
#if 0
						cmd.req_info.config_info.preamble_type =
							WLAN_DOT11_PHY_PREAMBLE_LONG;
						cmd.req_info.config_info.preamble_type =
							WLAN_DOT11_PHY_PREAMBLE_AUTO;
#endif
						cmd.req_info.config_info.listen_interval = 2;
						cmd.req_info.config_info.rts_threshold = 1400;
						cmd.req_info.config_info.frag_threshold = 0;
						cmd.req_info.config_info.max_tx_pwr= 10;
						break;
                        
					case WLAN_TRP_ADP_WPA_SETKEYS_CMD:
						DPRINTF(DBG_SIMULATOR, (DBGFMT "WLAN_TRP_ADP_WPA_SETKEYS_CMD\n", DBGARG));
						cmd.req_info.setkeys_info.num_keys = 1;
						A_MEMCPY(cmd.req_info.setkeys_info.keys[0].key,
							"abcdefghijklm", 13);
						cmd.req_info.setkeys_info.keys[0].key_length = 13;
						cmd.req_info.setkeys_info.keys[0].key_id = 0;
						cmd.req_info.setkeys_info.keys[0].key_type =
							WLAN_DOT11_WPA_KEY_TYPE_P;   /* Pairwise     */
#if 0
						cmd.req_info.setkeys_info.keys[0].key_type =
							WLAN_DOT11_WPA_KEY_TYPE_G_S; /* Group/Stakey */
#endif
						A_MEMZERO(cmd.req_info.setkeys_info.keys[0].mac_address, 6);
						A_MEMZERO(cmd.req_info.setkeys_info.keys[0].rsc, 8);
#if 0
						cmd.req_info.setkeys_info.keys[0].initiator = TRUE;
#endif
						cmd.req_info.setkeys_info.keys[0].initiator = FALSE;
						cmd.req_info.setkeys_info.keys[0].cipher_suite_selector[0] = 0x00;
						cmd.req_info.setkeys_info.keys[0].cipher_suite_selector[1] = 0x0f;
						cmd.req_info.setkeys_info.keys[0].cipher_suite_selector[2] = 0xac;

#if 0
						/* Use group cipher - not valid in this context */
						cmd.req_info.setkeys_info.keys[0].cipher_suite_selector[3] = 0;
						/* WEP-40 */
						cmd.req_info.setkeys_info.keys[0].cipher_suite_selector[3] = 1;
#endif
						/* TKIP */
						cmd.req_info.setkeys_info.keys[0].cipher_suite_selector[3] = 2;
#if 0
						/* Reserved */
						cmd.req_info.setkeys_info.keys[0].cipher_suite_selector[3] = 3;
						/* CCMP */
						cmd.req_info.setkeys_info.keys[0].cipher_suite_selector[3] = 4;
						/* WEP-104 */
						cmd.req_info.setkeys_info.keys[0].cipher_suite_selector[3] = 5;
#endif
						break;
                        
					case WLAN_TRP_ADP_WPA_DELKEYS_CMD:
						DPRINTF(DBG_SIMULATOR, (DBGFMT "WLAN_TRP_ADP_WPA_DELKEYS_CMD\n", DBGARG));
						cmd.req_info.delkeys_info.num_keys = 1;
						cmd.req_info.delkeys_info.keys[0].key_id = 0;
						cmd.req_info.delkeys_info.keys[0].key_type =
							WLAN_DOT11_WPA_KEY_TYPE_P;   /* Pairwise     */
#if 0
						cmd.req_info.sdeleys_info.keys[0].key_type =
							WLAN_DOT11_WPA_KEY_TYPE_G_S; /* Group/Stakey */
#endif
						A_MEMZERO(cmd.req_info.setkeys_info.keys[0].mac_address, 6);
						break;
                        
					case WLAN_TRP_ADP_WPA_SETPROTECT_CMD:
						DPRINTF(DBG_SIMULATOR, (DBGFMT "WLAN_TRP_ADP_WPA_SETPROTECT_CMD\n", DBGARG));

							if((bss_desc =
								stubs_get_bss_desc_by_name(s_bss_name)) == NULL)
							{
								DPRINTF(DBG_SIMULATOR,
									(DBGFMT "Unable to find \"%s\"\n", DBGARG, s_bss_name));
								goto noaction;
							}

						wlan_util_uint64_to_macaddress(
							cmd.req_info.protect_info.protect_list[0].address,
							bss_desc->bssid);
						cmd.req_info.protect_info.protect_list[0].protect_type =
							WLAN_DOT11_WPA_PROTECT_NONE;
#if 0
						cmd.req_info.protect_info.protect_list[0].protect_type =
							WLAN_DOT11_WPA_PROTECT_RX;
						cmd.req_info.protect_info.protect_list[0].protect_type =
							WLAN_DOT11_WPA_PROTECT_TX;
						cmd.req_info.protect_info.protect_list[0].protect_type =
							WLAN_DOT11_WPA_PROTECT_RX_TX;
#endif
						cmd.req_info.protect_info.protect_list[0].key_type =
							WLAN_DOT11_WPA_KEY_TYPE_P;   /* Pairwise     */
#if 0
						cmd.req_info.protect_info.protect_list[0].key_type =
							WLAN_DOT11_WPA_KEY_TYPE_G_S; /* Group/Stakey */
#endif
						break;
                        
					case WLAN_TRP_ADP_ADDTS_CMD:
						DPRINTF(DBG_SIMULATOR,
							(DBGFMT "WLAN_TRP_ADP_ADDTS_CMD\n", DBGARG));

						cmd.req_info.addts_info.dialog_token = s_token++;
						cmd.req_info.addts_info.addts_failure_timeout = 0;

						/* WMM TSPEC Element */
						/* WMM_AC_VI */
						if ((WMM_AC_VI == s_param) || (WMM_AC_BK == s_param))
						{
							cmd.req_info.addts_info.tspec.nominal_msdu_size = 150;
							cmd.req_info.addts_info.tspec.maximum_msdu_size = 1500;
							cmd.req_info.addts_info.tspec.min_data_rate = 160;
							cmd.req_info.addts_info.tspec.mean_data_rate = 160;
							cmd.req_info.addts_info.tspec.peak_data_rate = 160;
                            if (WMM_AC_VI == s_param)
							    cmd.req_info.addts_info.tspec.ts_info.up = 4;
                            else if (WMM_AC_BK == s_param)
                                cmd.req_info.addts_info.tspec.ts_info.up = 1;
						}
						/* WMM_AC_VO */
						else if (WMM_AC_VO == s_param)
						{
							cmd.req_info.addts_info.tspec.nominal_msdu_size = 0x80d0;
							cmd.req_info.addts_info.tspec.maximum_msdu_size = 0xd0;
							cmd.req_info.addts_info.tspec.min_data_rate = 0x14500;
							cmd.req_info.addts_info.tspec.mean_data_rate = 0x14500;
							cmd.req_info.addts_info.tspec.peak_data_rate = 0x14500;
							cmd.req_info.addts_info.tspec.ts_info.up = 6;
						}
						
						cmd.req_info.addts_info.tspec.min_service_interval = 20;
						cmd.req_info.addts_info.tspec.max_service_interval = 20;
						cmd.req_info.addts_info.tspec.inactivity_interval = 9999999;
						cmd.req_info.addts_info.tspec.suspension_interval = ~0;
						cmd.req_info.addts_info.tspec.svc_start_time = 0;
						cmd.req_info.addts_info.tspec.max_burst_size = 0;
						cmd.req_info.addts_info.tspec.delay_bound = 0;
						cmd.req_info.addts_info.tspec.min_phy_rate = 0x5B8D80;
						cmd.req_info.addts_info.tspec.surplus_bw_allowance = 0x2000;
						cmd.req_info.addts_info.tspec.medium_time = 0;
						
						/* TS_INFO */
						cmd.req_info.addts_info.tspec.ts_info.psb = 1;
						cmd.req_info.addts_info.tspec.ts_info.direction = WLAN_ADP_WMM_TS_DIR_BOTH;
						cmd.req_info.addts_info.tspec.ts_info.tid = s_ctid++;

						s_param = 0;
						break;
                        
					case WLAN_TRP_ADP_DELTS_CMD:
						DPRINTF(DBG_SIMULATOR,
							(DBGFMT "WLAN_TRP_ADP_DELTS_CMD %d\n", DBGARG, s_param));

						/* TS Info */
                        cmd.req_info.delts_info.reason_code = 0;
						cmd.req_info.delts_info.ts_info.up = s_param;
						cmd.req_info.delts_info.ts_info.psb = 1;
						cmd.req_info.delts_info.ts_info.direction = WLAN_ADP_WMM_TS_DIR_BOTH;
						cmd.req_info.delts_info.ts_info.tid = s_dtid++;
						s_param = 0;
						break;
                        
#ifdef CONFIG_HOST_TCMD_SUPPORT
					case WLAN_TRP_ADP_TEST_CMD:
						DPRINTF(DBG_SIMULATOR,
							(DBGFMT "WLAN_TRP_ADP_TEST_CMD\n", DBGARG));
						
                        switch(s_test_param)
						{
							case 1:
								DPRINTF(DBG_SIMULATOR,
									(DBGFMT "set cont tx data mode \n", DBGARG));

								tcmd_cont_tx.testCmdId = TCMD_CONT_TX_ID;
								tcmd_cont_tx.mode =  TCMD_CONT_TX_FRAME;
                                if (s_loop == 20)
                                {
			                        tcmd_cont_tx.freq = 2412;
                                    /* s_loop = 0; */
                                }
                                else
                                {
                                    if (tcmd_cont_tx.freq < 2472)
                                        tcmd_cont_tx.freq += 5;
                                    else if (tcmd_cont_tx.freq == 2472)
                                        tcmd_cont_tx.freq = 2484;
                                    else if (tcmd_cont_tx.freq == 2484)
                                    {
                                        if (bgmode)
                                           s_loop = 0;
                                        else
                                        tcmd_cont_tx.freq = 5180;
                                    }
                                    else if (tcmd_cont_tx.freq >= 5180 && tcmd_cont_tx.freq < 5320)
                                        tcmd_cont_tx.freq += 20;
                                    else if (tcmd_cont_tx.freq == 5320)
                                        s_loop = 0;
                                    else
                                        printf("invalid frequency %d.\n", tcmd_cont_tx.freq);
                                }
								tcmd_cont_tx.dataRate = 0;
								tcmd_cont_tx.txPwr = 6;
								tcmd_cont_tx.antenna = 1;
                                tcmd_cont_tx.pktSz = 100;
                                tcmd_cont_tx.txPattern = TCMD_TXPATTERN_ZERONE_DIS_SCRAMBLE; 
								cmd.req_info.ftm_test_data.len =
													sizeof(tcmd_cont_tx);
								A_MEMCPY(&cmd.req_info.ftm_test_data.data,
									&tcmd_cont_tx, sizeof(tcmd_cont_tx));
								break;
                                
							case 2:
								DPRINTF(DBG_SIMULATOR,
									(DBGFMT "Stop cont tx data \n", DBGARG));
								
                                tcmd_freq = tcmd_cont_tx.freq;
								A_MEMZERO(&tcmd_cont_tx, sizeof(tcmd_cont_tx));
                                tcmd_cont_tx.freq = tcmd_freq;
								tcmd_cont_tx.testCmdId = TCMD_CONT_TX_ID;
								tcmd_cont_tx.mode = TCMD_CONT_TX_OFF;
								cmd.req_info.ftm_test_data.len =
													sizeof(tcmd_cont_tx);
								A_MEMCPY(&cmd.req_info.ftm_test_data.data,
									&tcmd_cont_tx, sizeof(tcmd_cont_tx));
								break;
                                    
							case 3:
								DPRINTF(DBG_SIMULATOR,
									(DBGFMT "Set Cont Rx promis mode \n", DBGARG));
								
								tcmd_cont_rx.testCmdId = TCMD_CONT_RX_ID;
								tcmd_cont_rx.act = TCMD_CONT_RX_PROMIS;
                                
                                tcmd_cont_rx.u.para.freq = tcmd_freq;
                                if (s_loop == 20)
                                {
                                   tcmd_cont_rx.u.para.freq = 2412;
                                   /* s_loop = 0; */
                                }
                                else
                                {
                                    if (tcmd_cont_rx.u.para.freq < 2472)
                                        tcmd_cont_rx.u.para.freq += 5;
                                    else if (tcmd_cont_rx.u.para.freq == 2472)
                                        tcmd_cont_rx.u.para.freq = 2484;
                                    else if (tcmd_cont_rx.u.para.freq == 2484)
                                    {
                                        if (bgmode)
                                           s_loop = 0;
                                        else
                                        tcmd_cont_rx.u.para.freq = 5180;
                                    }
                                    else if (tcmd_cont_rx.u.para.freq >= 5180 && 
                                             tcmd_cont_rx.u.para.freq < 5320)
                                        tcmd_cont_rx.u.para.freq += 20;
                                    else if (tcmd_cont_rx.u.para.freq == 5320)
                                        s_loop = 0;
                                    else
                                        printf("invalid frequency %d.\n", tcmd_cont_rx.u.para.freq);
                                }
                                
								tcmd_cont_rx.u.para.antenna = 1;
								cmd.req_info.ftm_test_data.len =
													sizeof(tcmd_cont_rx);
								A_MEMCPY(cmd.req_info.ftm_test_data.data,
									&tcmd_cont_rx, sizeof(tcmd_cont_rx));
                                tcmd_freq = tcmd_cont_rx.u.para.freq;
							    break;
                                
							case 4:
								DPRINTF(DBG_SIMULATOR,
									(DBGFMT "Stop Cont Rx and get report ", DBGARG));

                                
								tcmd_cont_rx.testCmdId = TCMD_CONT_RX_ID;
								tcmd_cont_rx.act = TCMD_CONT_RX_REPORT;
								tcmd_cont_rx.u.report.totalPkt = 0;
								tcmd_cont_rx.u.report.rssiInDBm =0;
								cmd.req_info.ftm_test_data.len =
													sizeof(tcmd_cont_rx);
								A_MEMCPY(cmd.req_info.ftm_test_data.data,
									&tcmd_cont_rx, sizeof(tcmd_cont_rx));
								break;
						}
						break;
#endif
					case WLAN_TRP_ADP_BTHCI_CMD:
				    {
				        A_UINT8 pdu[1512];
				        A_UINT16  sz;
				        extern void wlan_oem_send_acl_data(void* buf);
				    
				        memset(pdu, 0, sizeof(pdu));
		                get_pal_file(s_param, pdu, &sz);

		                if(s_param < 51) {
		                	cmd.req_info.bthci_info.buf = (uint8 *)pdu;
		                	cmd.req_info.bthci_info.size = sz;
		    				cmd.hdr.cmd_id = WLAN_TRP_ADP_BTHCI_CMD;
		    				
		    			    printf("PAL send HCI cmd ->\n");
		    			    dump_frame((A_UINT8 *)pdu, (A_UINT32)sz);
		                } else if(s_param == 51) {
		                    printf("PAL send HCI data ->\n");
		                    dump_frame((A_UINT8 *)pdu, (A_UINT32)sz);
		                    wlan_oem_send_acl_data(pdu);
		                }
		                break;
				    }
					case WLAN_TRP_ADP_BT_DISPATCHER_CMD:
					{
						extern int rex_evt_dispatcher_stub(A_UINT8* buf, A_UINT16 size);
						extern int rex_data_dispatcher_stub(A_UINT8* buf, A_UINT16 size);
						
	                	cmd.req_info.btdispatcher_info.evt_dispatcher = rex_evt_dispatcher_stub;
	                	cmd.req_info.btdispatcher_info.data_dispatcher = rex_data_dispatcher_stub;
	    				cmd.hdr.cmd_id = WLAN_TRP_ADP_BT_DISPATCHER_CMD;
	    				break;
					}
					default:
						DPRINTF(DBG_SIMULATOR, (DBGFMT "Unknown command %d\n", DBGARG,
							prgp[pc][1]));
						goto noaction;
				}

				/* Finaly, set the command ... */
				cmd.hdr.cmd_id = prgp[pc][1];

				if ((cmd.hdr.cmd_id == WLAN_TRP_ADP_BTHCI_CMD) &&
					(s_param == 51)) {
					goto noaction;
				}
				
			    /* ... and send it off */
			    wlan_oem_cmd_hdlr(&cmd);
noaction:
				pc++;
				break;
#ifdef CONFIG_HOST_TCMD_SUPPORT
            case PRG_SET_TEST_PARAM:
				DPRINTF(DBG_SIMULATOR, (DBGFMT "PRG_SET_TEST_PARAM %d\n", DBGARG, prgp[pc][1]));
				s_test_param = prgp[pc][1];
				pc++;
				break;
#endif
			case PRG_SET_PARAM:
				DPRINTF(DBG_SIMULATOR, (DBGFMT "PRG_SET_PARAM %d\n", DBGARG, prgp[pc][1]));
				s_param = prgp[pc][1];
				pc++;
				break;
			case PRG_SEND_PKT:
				DPRINTF(DBG_SIMULATOR,
					(DBGFMT "PRG_SEND_PKT %d %d\n", DBGARG,
					prgp[pc][1], s_loop));
				{
					static wlan_adp_tx_pkt_meta_info_type meta_info;
					int i = s_loop;
					
					#if 1
					A_UINT8 dstaddr[6] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
					#else
					A_UINT8 dstaddr[6] = {0x00, 0x16, 0x41, 0xe2, 0xda, 0x00};
                    #endif
					const int offset = 14;

					/* Do special parameter processing */
				    if(s_param == -2)
					{
						/* Anything negative disables "pull" transmits */
						set_max_tx_packets(s_param < 0 ? 0 : s_param);

						/* Send random QoS traffic */
						if(s_param == -2)
						{
							switch(rand() % 4)
							{
								case 0:
									meta_info.tid = 0;
									DPRINTF(DBG_SIMULATOR,
										(DBGFMT "QoS PKT up %d WMM_AC_BE\n",
										DBGARG, meta_info.tid));
									break;
								case 1:
									meta_info.tid = 1;
									DPRINTF(DBG_SIMULATOR,
										(DBGFMT "QoS PKT up %d WMM_AC_BK\n",
										DBGARG, meta_info.tid));
									break;
								case 2:
									meta_info.tid = 4;
									DPRINTF(DBG_SIMULATOR,
										(DBGFMT "QoS PKT up %d WMM_AC_VI\n",
										DBGARG, meta_info.tid));
									break;
								case 3:
									meta_info.tid = 6;
									DPRINTF(DBG_SIMULATOR,
										(DBGFMT "QoS PKT up %d WMM_AC_V0\n",
										DBGARG, meta_info.tid));
									break;
							}
						}
					}
					else if (s_param == -1) {
					    meta_info.tid = (s_loop % 2) * 3 + 1;
					}
                    else if (s_param >= 0) {
                        meta_info.tid = s_param;
                    }

					meta_info.dst_mac_addr =
						wlan_util_macaddress_to_uint64(dstaddr);
					meta_info.tx_rate = 0;

					/* Make each packet a bit different */
					#if 1
					s_arp_pkt1[30] = i & 0xff;
					if(wlan_oem_tx_req(sizeof(s_arp_pkt1) - offset,
						&s_arp_pkt1[offset], &meta_info) != 0)
					#endif
					
					if(wlan_oem_tx_req(sizeof(s_arp_pkt1) - offset,
						&s_data_pkt[offset], &meta_info) != 0)
					{
						DPRINTF(DBG_SIMULATOR,
							(DBGFMT "wlan_oem_tx_req() failed.\n", DBGARG));
					}
				}
				pc++;
				break;
			case PRG_SET_LOOP:
				DPRINTF(DBG_SIMULATOR, (DBGFMT "PRG_SET_LOOP %d\n", DBGARG, prgp[pc][1]));
				s_loop = prgp[pc][1];
				pc++;
				s_loop_pc = pc;
				break;
			case PRG_LOOP:
				DPRINTF(DBG_SIMULATOR, (DBGFMT "PRG_LOOP %d %d\n", DBGARG,
					prgp[pc][1], s_loop));
				if(--s_loop > 0)
				{
					pc = s_loop_pc;
				}
				else
				{
					pc++;
				}
				break;
			case PRG_SUBCALL:
				DPRINTF(DBG_SIMULATOR,
					(DBGFMT "PRG_SUBCALL %d 0x%08x\n", DBGARG,
					prgp[pc][1], prgp[pc][1]));
				{
					unsigned int arg = prgp[pc][1];

					prg_num = arg & 0xff;
					prgp = find_program(&prg_num);
					pc = (arg >> 8) & 0xff;
					DPRINTF(DBG_SIMULATOR,
						(DBGFMT "Starting program %d at pc %d.\n", DBGARG,
						prg_num, pc));
				}
				break;
			default:
				DPRINTF(DBG_SIMULATOR, (DBGFMT "Unknow op code %d arg %d.\n", DBGARG, pc, prgp[pc][1]));
				break;
		}

		if(my_wait != 0)
		{
			/* first, lock the mutex */
			if((rc = pthread_mutex_lock(&my_mutex)) != 0)
			{
				/* an error has occurred */
				perror("pthread_mutex_lock");
				pthread_exit(NULL);
			}

			/* Mutex is now locked - wait on the condition variable.  */
			if(my_timeout > 0)
			{
				/* Get current time */ 
				gettimeofday(&now, NULL);

				/* Prepare timeout value - we need an absolute time. */
				timeout.tv_sec = now.tv_sec + my_timeout;
				timeout.tv_nsec = now.tv_usec * 1000;

				/* We use a loop, since a Unix signal might stop */
				/* the wait before the timeout.                  */
				done = 0;
				while(!done)
				{
					/* The pthread_cond_timedwait() unlocks the */
					/* mutex on entrance.                       */
					rc = pthread_cond_timedwait(&my_cond, &my_mutex, &timeout);
#if 0
					DPRINTF(DBG_SIMULATOR, (DBGFMT "wait rc %d errno %d %s\n", DBGARG,
						rc, errno, strerror(errno)));
#endif
					switch(rc)
					{
						case 0:
							/* Awakened due to the cond. variable */
							/* being signaled.                    */
							/* The mutex was now locked again by  */
							/* pthread_cond_timedwait.            */

							/* Do your stuff here...              */
							done = 1;
							break;
						case ETIMEDOUT:
#if 0
							DPRINTF(DBG_SIMULATOR, (DBGFMT "ETIMEDOUT\n", DBGARG));
#endif
							done = 1;
							break;
						default:
							/* Some error occurred (e.g. got a Unix signal) */
							if(errno == ETIMEDOUT)
							{
								/* Our time is up */
								done = 1;
							}

							/* Break this switch, but re-do the while loop.   */
							break;
					}
				}
			}
			else
			{
				/* During the execution of pthread_cond_wait/, the mutex   */
				/* is unlocked.                                           */
				rc = pthread_cond_wait(&my_cond, &my_mutex);

				if(rc == 0)
				{
					/* Awakened due to the cond. variable being signaled    */
					/* The mutex is now locked again by pthread_cond_wait() */

					/* do your stuff...                                     */
				}

#if 0
				DPRINTF(DBG_SIMULATOR, (DBGFMT "reg rc %d\n", DBGARG, rc));
#endif
			}
		}

		/* Finally, unlock the mutex */
		pthread_mutex_unlock(&my_mutex);
	}

	DPRINTF(DBG_SIMULATOR, (DBGFMT "Ending\n", DBGARG));

	return(0);
}
