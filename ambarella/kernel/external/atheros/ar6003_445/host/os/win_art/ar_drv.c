/*
 *
 * Copyright (c) 2004-2009 Atheros Communications Inc.
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <winsock2.h>

#include "athtypes_win.h"
#include "athdefs.h"
//#include "art.h"
#include "ar_drv.h"
#include "export.h"

A_UINT8 DevdrvInterface = DEVDRV_INTERFACE_SDIO;

A_BOOL  __cdecl loadDriver_ENE(A_BOOL bOnOff, A_UINT32 subSystemID)
{
    if (DevdrvInterface == DEVDRV_INTERFACE_SDIO)
    {
        return SDIO_loadDriver_ENE(bOnOff, subSystemID);
    }
    else
    {
        return TRUE;
    }
}

A_BOOL __cdecl closehandle(void)
{
    if (DevdrvInterface == DEVDRV_INTERFACE_SDIO)
    {
        return SDIO_closehandle();
    }
    else
    {
        return TRUE;
    }
}

A_BOOL __cdecl InitAR6000_ene(HANDLE *handle, A_UINT32 *nTargetID)
{
    return SDIO_InitAR6000_ene(handle, nTargetID);
}

A_BOOL __cdecl InitAR6000_eneX(HANDLE *handle, A_UINT32 *nTargetID, A_UINT8 devdrvInterface)
{
    DevdrvInterface = devdrvInterface;

    if (DevdrvInterface == DEVDRV_INTERFACE_ETH)
    {
        return ETH_InitAR6000_ene(handle, nTargetID);
    }
    else if (DevdrvInterface == DEVDRV_INTERFACE_UART)
    {
        return UART_InitAR6000_ene(handle, nTargetID);
    }
    else
    {
        return SDIO_InitAR6000_ene(handle, nTargetID);
    }
}

A_BOOL __cdecl EndAR6000_ene(int devIndex)
{
    if (DevdrvInterface == DEVDRV_INTERFACE_SDIO)
    {
        return SDIO_EndAR6000_ene(devIndex);
    }
    else
    {
        return TRUE;
    }
}

HANDLE __cdecl open_device_ene(A_UINT32 device_fn, A_UINT32 devIndex, char * pipeName)
{
    if (DevdrvInterface == DEVDRV_INTERFACE_ETH)
    {
        return ETH_open_device_ene(device_fn, devIndex, pipeName);
    }
    else if (DevdrvInterface == DEVDRV_INTERFACE_UART)
    {
        return UART_open_device_ene(device_fn, devIndex, pipeName);
    }
    else
    {
        return SDIO_open_device_ene(device_fn, devIndex, pipeName);
    }
}

int __cdecl  DisableDragonSleep(HANDLE device)
{
    if (DevdrvInterface == DEVDRV_INTERFACE_SDIO)
    {
        return SDIO_DisableDragonSleep(device);
    }
    else
    {
        return TRUE;
    }
}


DWORD __cdecl DRG_Write(  HANDLE COM_Write, PUCHAR buf, ULONG length)
{
    if (DevdrvInterface == DEVDRV_INTERFACE_ETH)
    {
        return ETH_DRG_Write(COM_Write, buf, length);
    }
    else if (DevdrvInterface == DEVDRV_INTERFACE_UART)
    {
        return UART_DRG_Write(COM_Write, buf, length);
    }
    else
    {
        return SDIO_DRG_Write(COM_Write, buf, length);
    }
}


DWORD __cdecl BT_DRG_Write(  HANDLE COM_Write, PUCHAR buf, ULONG length )
{

	return HCI_DRG_Write(COM_Write, buf, length);

}

DWORD __cdecl BT_DRG_Read( HANDLE pContext, PUCHAR buf, ULONG length, PULONG pBytesRead)
{

	return HCI_DRG_Read(pContext, buf, length, pBytesRead);

}

DWORD __cdecl DRG_Read( HANDLE pContext, PUCHAR buf, ULONG length, PULONG pBytesRead)
{
    if (DevdrvInterface == DEVDRV_INTERFACE_ETH)
    {
        return ETH_DRG_Read(pContext, buf, length, pBytesRead);
    }
    else if (DevdrvInterface == DEVDRV_INTERFACE_UART)
    {
        return UART_DRG_Read(pContext, buf, length, pBytesRead);
    }
    else
    {
        return SDIO_DRG_Read(pContext, buf, length, pBytesRead);
    }
}

int  __cdecl BMIWriteSOCRegister_win(HANDLE handle, A_UINT32 address, A_UINT32 param)
{
    if (DevdrvInterface == DEVDRV_INTERFACE_SDIO)
    {
        return SDIO_BMIWriteSOCRegister_win(handle, address, param);
    }
    else
    {
        return TRUE;
    }
}

int __cdecl BMIReadSOCRegister_win(HANDLE handle, A_UINT32 address, A_UINT32 *param)
{
    if (DevdrvInterface == DEVDRV_INTERFACE_SDIO)
    {
        return SDIO_BMIReadSOCRegister_win(handle, address, param);
    }
    else
    {
        return TRUE;
    }
}

int __cdecl BMIWriteMemory_win(HANDLE handle, A_UINT32 address, A_UCHAR *buffer, A_UINT32 length)
{
    if (DevdrvInterface == DEVDRV_INTERFACE_SDIO)
    {
        return SDIO_BMIWriteMemory_win(handle, address, buffer, length);
    }
    else
    {
        return TRUE;
    }
}

int __cdecl BMIReadMemory_win(HANDLE handle, A_UINT32 address, A_UCHAR *buffer, A_UINT32 length)
{
    if (DevdrvInterface == DEVDRV_INTERFACE_SDIO)
    {
        return SDIO_BMIReadMemory_win(handle, address, buffer, length);
    }
    else
    {
        return TRUE;
    }
}

int __cdecl BMISetAppStart_win(HANDLE handle, A_UINT32 address)
{
    if (DevdrvInterface == DEVDRV_INTERFACE_SDIO)
    {
        return SDIO_BMISetAppStart_win(handle, address);
    }
    else
    {
        return TRUE;
    }
}

int __cdecl BMIDone_win(HANDLE handle)
{
    if (DevdrvInterface == DEVDRV_INTERFACE_SDIO)
    {
        return SDIO_BMIDone_win(handle);
    }
    else
    {
        return TRUE;
    }
}

int __cdecl BMIFastDownload_win(HANDLE handle, A_UINT32 address, A_UCHAR *buffer, A_UINT32 length)
{
    if (DevdrvInterface == DEVDRV_INTERFACE_SDIO)
    {
        return SDIO_BMIFastDownload_win(handle, address, buffer, length);
    }
    else
    {
        return TRUE;
    }
}

int __cdecl BMIExecute_win(HANDLE handle, A_UINT32 address, A_UINT32 *param)
{
    if (DevdrvInterface == DEVDRV_INTERFACE_SDIO)
    {
        return SDIO_BMIExecute_win(handle, address, param);
    }
    else
    {
        return TRUE;
    }
}

int __cdecl BMITransferEepromFile_win(HANDLE handle, A_UCHAR *eeprom, A_UINT32 length)
{
    if (DevdrvInterface == DEVDRV_INTERFACE_SDIO)
    {
        return SDIO_BMITransferEepromFile_win(handle, eeprom, length);
    }
    else
    {
        return TRUE;
    }
}

void __cdecl devdrv_MyMallocStart ()
{
    MyMallocStart(1, 1000, "DEVDRV_DebugMemoryLeak.txt");
}

void __cdecl devdrv_MyMallocEnd ()
{
    MyMallocEnd();
}
