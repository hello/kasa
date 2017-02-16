//------------------------------------------------------------------------------
// <copyright file="ar6000_spi.c" company="Atheros">
//    Copyright (c) 2008 Atheros Corporation.  All rights reserved.
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
// Driver entry points for Atheros SDIO based Wifi Card driver
// Author(s): ="Atheros"
//==============================================================================
#include <windows.h>
#include <ceddk.h>
#include <ndis.h>
#include <types.h>
#include <devload.h>
#include "hif_internal.h"
#include "bus_driver.h"
#include "common.h"
#include "athdrv.h"
#include "common_drv.h"

int debughif = 2;

TCHAR g_AR6KDevicePowerName[] = _T("{98C5250D-C29A-4985-AE5F-AFE5367E5006}\\AR6K_SPI1");
TCHAR g_AR6KAdapterInstance[] = _T("AR6K_SPI1");
TCHAR g_AR6KMiniportName[] = _T("AR6K_SPI");
TCHAR g_AR6KDeviceInitEventName[] = _T("SYSTEM\\netui-AR6K_SPI1");
SDIO_CLIENT_INIT_CONTEXT g_ClientInitContext = {0};

#define NDIS_PARAM_KEY_PATH  TEXT("Comm\\AR6K_SPI1\\Parms")

static void CleanupRegKeys();

///////////////////////////////////////////////////////////////////////////////
//  DllEntry - the main dll entry point
//  Input:  hInstance - the instance that is attaching
//          Reason - the reason for attaching
//          pReserved - not much
//  Output:
//  Return: always returns TRUE
//  Notes:  this is only used to initialize the zones
///////////////////////////////////////////////////////////////////////////////
extern
BOOL WINAPI DllEntry(HINSTANCE hInstance, ULONG Reason, LPVOID pReserved)
{
    if ( Reason == DLL_PROCESS_ATTACH )
    {
		DEBUGREGISTER(hInstance);
    }
    else if ( Reason == DLL_PROCESS_DETACH )
    {
    }

    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
//  DRG_Deinit - the deinit entry point for the memory driver
//  Input:  hDeviceContext - the context returned from SMC_Init
//  Output:
//  Return: always returns TRUE
//  Notes:
///////////////////////////////////////////////////////////////////////////////
extern
BOOL WINAPI DRG_Deinit(DWORD hDeviceContext)
{
    drvDeInit();
    CleanupRegKeys();
    return TRUE;
}

BOOL
createRegKeyValues()
{
	DWORD	Status;
	HKEY	hKey;
	DWORD	dwDisp;
	DWORD	dwVal;

	Status = RegCreateKeyEx(
	             HKEY_LOCAL_MACHINE,
	             NDIS_PARAM_KEY_PATH,
	             0,
	             NULL,
	             REG_OPTION_VOLATILE,
	             0,
	             NULL,
	             &hKey,
	             &dwDisp);

	if (Status != ERROR_SUCCESS)
	{
	    return FALSE;
	}

	dwVal = 0;

    Status = RegSetValueEx(
                    hKey,
	                TEXT("BusNumber"),
	                0,
	                REG_DWORD,
	                (PBYTE)&dwVal,
	                sizeof(DWORD));

	if (Status != ERROR_SUCCESS)
	{
	    return FALSE;
	}

	Status = RegSetValueEx(
                    hKey,
	                TEXT("BusType"),
	                0,
	                REG_DWORD,
	                (PBYTE)&dwVal,
	                sizeof(DWORD));


	if (Status != ERROR_SUCCESS)
	{
	    return FALSE;
	}

	RegCloseKey(hKey);

	return TRUE;
}   // AddKeyValues

static void CleanupRegKeys()
{
    DWORD   status;
    HKEY    hKey;

    status = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                          NDIS_PARAM_KEY_PATH,
                          0,
                          0,             
                          &hKey);

    if (status != ERROR_SUCCESS) {
        return;
    }

    RegDeleteValue(hKey,TEXT("BusNumber"));
    RegDeleteValue(hKey,TEXT("BusType"));
    RegCloseKey(hKey);
}

///////////////////////////////////////////////////////////////////////////////
//  DRG_Init - the init entry point for the memory driver
//  Input:  dwContext - the context for this init
//  Output:
//  Return: non-zero context
//  Notes:
///////////////////////////////////////////////////////////////////////////////
extern
DWORD WINAPI DRG_Init(DWORD dwContext)
{
	PWCHAR                          pRegPath = NULL;
    DWORD                           dwRetCode = 0;
    HANDLE 		                    wlanHandle = NULL;
    SDIO_CLIENT_INIT_CONTEXT        *pContext;
    DWORD                           initContext = 0;
    SDIO_STATUS                     status;
  
    do {
        
        status = SDLIB_GetRegistryKeyDWORD(HKEY_LOCAL_MACHINE, 
                                           (WCHAR  *)dwContext,
                                           DEVLOAD_CLIENTINFO_VALNAME, 
                                           (DWORD *)&pContext);
        
        if (!SDIO_SUCCESS(status)) {
            break;    
        }
                
        if (pContext->Magic != SDIO_CLIENT_INIT_MAGIC) {
            return 0;    
        }
                                         
        memcpy(&g_ClientInitContext,pContext,sizeof(SDIO_CLIENT_INIT_CONTEXT));
        
        if (drvInit() != A_OK) {
            break;
        }
    
        if (!createRegKeyValues()) {
            break;
        }
    
        //
    	// Create Named Event to notify AR6K Monitor Service program
    	//
    	wlanHandle = CreateEvent (NULL, FALSE, FALSE, L"ATHRWLAN6KEVENT");
    	dwRetCode  = GetLastError ();
    	if (NULL == wlanHandle) {
            break;
        }
        //
    	// Set the event to trigger the service
    	//
    	SetEvent (wlanHandle);
        
            /* just return non-zero */
        initContext = 1;
        
    } while (FALSE);
	
    return initContext;
}

/////////////////////////////////////////////////////////////////////
// Dummy Bus Interface wrappers

NDIS_STATUS
busDriverInit (NDIS_HANDLE miniportHandle,
               NDIS_HANDLE wrapperConfigurationContext,
               A_UINT32   sysIntr,
               BUS_DRIVER_HANDLE *busDriverHandle)
{
    *busDriverHandle = NULL;
    return NDIS_STATUS_SUCCESS;
}


void
busDriverIsr (BUS_DRIVER_HANDLE busDriverHandle,A_BOOL *callDsr)
{
    *callDsr = FALSE;
    return;
}

void
busDriverDsr (BUS_DRIVER_HANDLE busDriverHandle)
{
   return;
}

void
busDriverShutdown (BUS_DRIVER_HANDLE busDriverHandle)
{
    return;
}

#ifdef AR6K_MTE_DRV_TEST

A_BOOL ar6k_bus_cmd(void *pInput, int Length)
{
    /* not implemented */
    return FALSE;         
}

#endif

/* proxy functions */

SDIO_STATUS SDIO_RegisterFunction(PSDFUNCTION pFunction) 
{    
    return g_ClientInitContext.pRegisterFunction(pFunction);
}

SDIO_STATUS SDIO_UnregisterFunction(PSDFUNCTION pFunction) 
{
    return g_ClientInitContext.pUnregisterFunction(pFunction);
}


