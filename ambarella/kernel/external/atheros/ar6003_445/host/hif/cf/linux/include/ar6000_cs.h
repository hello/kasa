#ifndef _AR6000_CS_H_
#define _AR6000_CS_H_
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
// 	Wifi driver for AR6002
// </summary>
//
 *
 */

#include <a_config.h>
#include <athdefs.h>
#include <a_types.h>
#include <a_osapi.h>

typedef struct _CF_PNP_INFO_ {
	A_UINT16 CF_ManufacturerCode;  /* JEDEC Code */
    A_UINT16 CF_ManufacturerID;    /* manf-specific ID */
    A_UINT8  CF_FunctionNo;        /* function number 1-7 */ 
    A_UINT8  CF_FunctionClass;     /* function class */

} CF_PNP_INFO, *PCF_PNP_INFO;

//Forward decl
typedef struct _CFFUNCTION_ *PCFFUNCTION;
typedef struct _CFREQUEST_ *PCFREQUEST;
typedef void * PCFDEVICE;

typedef A_INT32 CF_STATUS;

#define CF_SUCCESS(status) ((CF_STATUS)(status) >= 0)
#define CF_STATUS_SUCCESS 0
#define CF_STATUS_ERROR -1

/* Structure used by the upper layer to register itself with the
* device driver.
*/
typedef struct _CFFUNCTION_ {

	A_UCHAR    *pName;	/* name of registering driver */
    A_UINT8       MaxDevices;  /* maximum number of devices supported by this function */
    A_UINT8       NumDevices;  /* number of devices supported by this function */

	/* callback functions provided by the Upper layer module to the driver.
	*/
	A_BOOL (*pProbe)(struct _CFFUNCTION_ *pFunction, PCFDEVICE pDevice);
	void (*pRemove)(struct _CFFUNCTION_ *pFunction, PCFDEVICE pDevice);

	/* Currently Unused
    CF_STATUS (*pSuspend)(struct _CFFUNCTION *pFunction, CFPOWER_STATE state); 
    CF_STATUS (*pResume)(struct _CFFUNCTION *pFunction); 
    CF_STATUS (*pWake) (struct _CFFUNCTION *pFunction, CFPOWER_STATE state, BOOL enable); 
	*/

	/* Cookie for the upper layer.
	*/
	void * pContext;
	PCF_PNP_INFO pIds; //NULL terminated list of PNP info.
	struct _CFFUNCTION_ *next; //hanger for the Func ctx list maintained by bus drv.

} CFFUNCTION;

/* CFREQUEST request flags */
typedef A_UINT8 CFREQUEST_FLAGS;

/* write operation */
#define CFREQ_FLAGS_DATA_WRITE         0x01
#define CFREQ_FLAGS_FIXED_ADDRESS	   0x02

typedef struct _CFREQUEST_ {
    void * pDataBuffer; /* starting address of buffer */
	A_UINT32	address; /* address to which data has to be written */
	A_UINT32	length;	/* length of data to be written */
	CFREQUEST_FLAGS Flags;
    void * pCompleteContext;   /* completion context */
    CF_STATUS Status;         /* completion status */
    struct _CFFUNCTION* pFunction; /* function driver that generated request (internal use)*/
}CFREQUEST;

typedef void (*pIsrHandler)(void *, A_BOOL *);
typedef void (*pDsrHandler)(unsigned long);

/* API function prototypes
 */
CF_STATUS CF_RegisterFunction(PCFFUNCTION pFunction);
CF_STATUS CF_UnregisterFunction(PCFFUNCTION pFunction);
CF_STATUS CF_BusRequest_Word(PCFDEVICE, PCFREQUEST);
CF_STATUS CF_BusRequest_Byte(PCFDEVICE, PCFREQUEST);
void CF_SetIrqHandler(PCFDEVICE, pIsrHandler, pDsrHandler, void *);
void CF_MaskIrqHandler(PCFDEVICE);
void CF_UnMaskIrqHandler(PCFDEVICE);

#define CFDEVICE_SET_IRQ_HANDLER(pDev,pFn1,pFn2,pContext)  \
	CF_SetIrqHandler(pDev,pFn1,pFn2,pContext);

#define CFDEVICE_CALL_REQUESTW_FUNC(pDev, pReq) \
	CF_BusRequest_Word(pDev, pReq)

#define CFDEVICE_CALL_REQUESTB_FUNC(pDev, pReq) \
	CF_BusRequest_Byte(pDev, pReq)

#endif
