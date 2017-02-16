/*
 * Copyright (c) 2004-2005 Atheros Communications Inc.
 * All rights reserved.
 *
 * $ATH_LICENSE_HOSTSDK0_C$
 *
 * This file contains the routines that implement the Boot loader messaging 
 * interface
 */

#include "hif.h"
#include "bmi_internal.h"
#include "fmi_internal.h"

/* ------ Static Variables ------ */

/* ------ Global Variable Declarations ------- */
A_BOOL fmiDone;

#define FMI_DATASZ_WMAX		256
#define FMI_DATASZ_RMAX		128

/* APIs visible to the driver */
void
FMIInit(void) 
{
    fmiDone = FALSE;
}

A_STATUS 
FMIReadFlash(HIF_DEVICE *device, 
              A_UINT32 address, 
              A_UCHAR *buffer, 
              A_UINT32 length) 
{
	struct read_cmd_s cmd;
    A_STATUS status;
    A_UINT32 remaining, rxlen;
    const A_UINT32 header = sizeof(cmd);
    A_UCHAR data[FMI_DATASZ_RMAX + header];

    if (fmiDone) {
        AR_DEBUG_PRINTF(DBG_FMI, ("Command disallowed\n"));
        return A_ERROR;
    }

    AR_DEBUG_PRINTF(DBG_FMI, 
       ("FMI Read Flash: Enter (device: 0x%p, address: 0x%x, length: %d)\n", 
        device, address, length));

    remaining = length;
    while (remaining)
    {
        rxlen = (remaining < FMI_DATASZ_RMAX) ? remaining : FMI_DATASZ_RMAX;
		cmd.command = FLASH_READ;
		cmd.address = address;
		cmd.length = rxlen;
        A_MEMCPY(&data[0], &cmd, sizeof(cmd));
        status = bmiBufferSend(device, data, sizeof(cmd));
        if (status != A_OK) {
            AR_DEBUG_PRINTF(DBG_FMI, ("Unable to write to the device\n"));
            return A_ERROR;
        }
        status = bmiBufferReceive(device, data, rxlen, TRUE);
        if (status != A_OK) {
            AR_DEBUG_PRINTF(DBG_FMI, ("Unable to read from the device\n"));
            return A_ERROR;
        }
        A_MEMCPY(&buffer[length - remaining], data, rxlen);
        remaining -= rxlen; address += rxlen;
    }

    AR_DEBUG_PRINTF(DBG_FMI, ("FMI Read Flash: Exit\n"));
    return A_OK;
}

A_STATUS 
FMIWriteFlash(HIF_DEVICE *device, 
               A_UINT32 address, 
               A_UCHAR *buffer, 
               A_UINT32 length) 
{
	struct write_cmd_s cmd;
    A_STATUS status;
    A_UINT32 remaining, txlen;
    const A_UINT32 header = sizeof(cmd);
    A_UCHAR data[FMI_DATASZ_WMAX + header];

    if (fmiDone) {
        AR_DEBUG_PRINTF(DBG_FMI, ("Command disallowed\n"));
        return A_ERROR;
    }

    AR_DEBUG_PRINTF(DBG_FMI, 
         ("FMI Write Flash: Enter (device: 0x%p, address: 0x%x, length: %d)\n", 
          device, address, length));

	cmd.command = FLASH_WRITE;

    remaining = length;
    while (remaining)
    {
        txlen = (remaining < (FMI_DATASZ_WMAX - header)) ? 
                  remaining : (FMI_DATASZ_WMAX - header);
		cmd.address = address;
		cmd.length = txlen;
        A_MEMCPY(&data[0], &cmd, sizeof(cmd));
        A_MEMCPY(&data[sizeof(cmd)], &buffer[length - remaining], txlen);
        status = bmiBufferSend(device, data, txlen + sizeof(cmd));
        if (status != A_OK) {
            AR_DEBUG_PRINTF(DBG_FMI, ("Unable to write to the device\n"));
            return A_ERROR;
        }
        remaining -= txlen; address += txlen;
    }

    AR_DEBUG_PRINTF(DBG_FMI, ("FMI Write Flash: Exit\n"));

    return A_OK;
}

A_STATUS 
FMIEraseFlash(HIF_DEVICE *device)
{
	struct erase_cmd_s cmd;
    A_STATUS status;

    if (fmiDone) {
        AR_DEBUG_PRINTF(DBG_FMI, ("Command disallowed\n"));
        return A_ERROR;
    }

    AR_DEBUG_PRINTF(DBG_FMI, 
       ("FMI Erase Flash: Enter (device: 0x%p)\n", device));

	cmd.command = FLASH_ERASE;
	cmd.magic_cookie = FLASH_ERASE_COOKIE;

    status = bmiBufferSend(device, (A_UCHAR *) &cmd, sizeof(cmd));
    if (status != A_OK) {
        AR_DEBUG_PRINTF(DBG_FMI, ("Unable to write to the device\n"));
        return A_ERROR;
    }

    AR_DEBUG_PRINTF(DBG_FMI, ("FMI Erase Flash: Exit\n"));

    return A_OK;
}

A_STATUS 
FMIPartialEraseFlash(HIF_DEVICE *device, A_UINT32 address, A_UINT32 length)
{
	struct partial_erase_cmd_s cmd;
    A_STATUS status;

    if (fmiDone) {
        AR_DEBUG_PRINTF(DBG_FMI, ("Command disallowed\n"));
        return A_ERROR;
    }

    AR_DEBUG_PRINTF(DBG_FMI, 
       ("FMI Partial Erase Flash: Enter (device: 0x%p)\n", device));

	cmd.command = FLASH_PARTIAL_ERASE;
	cmd.address = address;
	cmd.length = length;

    status = bmiBufferSend(device, (A_UCHAR *) &cmd, sizeof(cmd));
    if (status != A_OK) {
        AR_DEBUG_PRINTF(DBG_FMI, ("Unable to write to the device\n"));
        return A_ERROR;
    }

    AR_DEBUG_PRINTF(DBG_FMI, ("FMI Partial Erase Flash: Exit\n"));

    return A_OK;
}

A_STATUS 
FMIPartInitFlash(HIF_DEVICE *device, A_UINT32 address)
{
	struct part_init_cmd_s cmd;
    A_STATUS status;

    if (fmiDone) {
        AR_DEBUG_PRINTF(DBG_FMI, ("Command disallowed\n"));
        return A_ERROR;
    }

    AR_DEBUG_PRINTF(DBG_FMI, 
       ("FMI Part Init Flash: Enter (device: 0x%p, address 0x%08x)\n", device, address));

	cmd.command = FLASH_PART_INIT;
	cmd.address = address;

    status = bmiBufferSend(device, (A_UCHAR *) &cmd, sizeof(cmd));
    if (status != A_OK) {
        AR_DEBUG_PRINTF(DBG_FMI, ("Unable to write to the device\n"));
        return A_ERROR;
    }

    AR_DEBUG_PRINTF(DBG_FMI, ("FMI Part Init Flash: Exit\n"));

    return A_OK;
}

A_STATUS 
FMIDone(HIF_DEVICE *device) 
{
    A_STATUS status;
	struct flash_cmd_s cmd;

    if (fmiDone) {
        AR_DEBUG_PRINTF(DBG_FMI, ("Command disallowed\n"));
        return A_ERROR;
    }

    AR_DEBUG_PRINTF(DBG_FMI, ("FMI Done: Enter (device: 0x%p)\n", device));
    fmiDone = TRUE;
	
	cmd.command = FLASH_DONE;

    status = bmiBufferSend(device, (A_UCHAR *) &cmd, sizeof(cmd));
    if (status != A_OK) {
        AR_DEBUG_PRINTF(DBG_FMI, ("Unable to write to the device\n"));
        return A_ERROR;
    }
    AR_DEBUG_PRINTF(DBG_FMI, ("FMI Done: Exit\n"));

    return A_OK;
}

