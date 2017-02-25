#ifndef __SDCC_API_H
#define __SDCC_API_H
/**********************************************************************
 * sdcc_api.h
 *
 * This file provides SDCC external definitions.
 *
 * Copyright (C) 2004 Qualcomm, Inc.
 *
 **********************************************************************/
/*======================================================================

                        EDIT HISTORY FOR MODULE

  $Header: //depot/sw/releases/olca3.1-RC/host/os/rexos/include/athr/sdcc_api.h#3 $
  $DateTime: 2012/03/29 04:00:18 $ $Author: gijo $

when         who     what, where, why
--------     ---     -----------------------------------------------
08/28/05     hwu     General cleanup. Move testing related to sdcc_priv.h
07/24/05     hwu     Moved the common includes from private API to
                     the public API.
05/26/05     hwu     Merge in changes from L4Linux.
02/15/05     hwu     Added more SDIO related defines.
06/15/04     hwu     Initial version.
==================================================================*/
#ifdef MATS
#include "customer.h"
#include "comdef.h"

#include "msm.h"
#include "clk.h"        /* clk_busy_wait()  */
#include "hw.h"         /* MSM version info */

#include "assert.h"

#ifdef FEATURE_PDA
#include "biostew.h"
#endif /* #ifdef FEATURE_PDA */

#ifdef FEATURE_L4LINUX
#include <l4/ipc.h>     /* L4_Sleep() */
#endif
#endif

/* SDCC completion code - Need to match SFAT */
typedef enum SDCC_STATUS_t
{
    SDCC_NO_ERROR = 0,
    SDCC_ERR_UNKNOWN,
    SDCC_ERR_CMD_TIMEOUT,
    SDCC_ERR_DATA_TIMEOUT,
    SDCC_ERR_CMD_CRC_FAIL,              
    SDCC_ERR_DATA_CRC_FAIL,
    SDCC_ERR_CMD_SENT,
    SDCC_ERR_PROG_DONE,
    SDCC_ERR_CARD_READY,
    SDCC_ERR_INVALID_TX_STATE,
    SDCC_ERR_SET_BLKSZ,
    SDCC_ERR_SDIO_R5_RESP,
    SDCC_ERR_DMA,
    SDCC_ERR_READ_FIFO,
    SDCC_ERR_WRITE_FIFO,
    SDCC_ERR_ERASE,
    SDCC_ERR_SDIO,
    SDCC_ERR_SDIO_READ,
    SDCC_ERR_SDIO_WRITE,
    SDCC_ERR_CARD_INIT = 215 /* In SFAT, MMC_INTERFACE_ERROR = 215 */
}SDCC_STATUS;

enum sdcc_card_info
{
    SDCC_CARD_STATE = 0,
    SDCC_CARD_SIZE,
    SDCC_GET_TYPE,
    SDCC_GET_ERROR,
    SDCC_BLOCK_LEN,
    SDCC_DRIVE_GEOMETRY,
    SDCC_SERIAL_NUM
};

enum sdcc_sdio_dev_type
{
    SDCC_SDIO_UART = 1,
    SDCC_SDIO_BT_A,
    SDCC_SDIO_BT_B,
    SDCC_SDIO_GPS,
    SDCC_SDIO_CAMERA,
    SDCC_SDIO_PHS,
    SDCC_SDIO_WLAN
};

typedef struct drv_geometry_desc
{
    uint32  totalLBA;           /* Total drive logical blocks */
    uint16  dfltCyl;            /* Number of cylinders        */
    uint16  dfltHd;             /* Number of Heads            */
    uint16  dfltSct;            /* Sectors per track          */
    uint16  dfltBytesPerSect;   /* Bytes per sector           */
    uint8   serialNum[16];      /* Drive serial number        */
    uint8   modelNum[32];       /* Drive model number         */
    uint8   MID;                /* Manufacturer ID (binary) */
    uint8   OEMAppID[3];        /* OEM/Application ID 2 ASCII string */
    uint8   revisionNum;        /* Product revision number (binary) */
    uint8   MDateM;             /* Manufacturing date : month (binary) */
    uint8   MDateY;             /* Manufacturing date : year (binary) */   
}DRV_GEOMETRY_DESC;

/* SDIO definitions */
typedef enum 
{
    SD_IO_FUNCTION_ENABLE = 0,
    SD_IO_FUNCTION_DISABLE, 
    SD_IO_FUNCTION_SET_BLOCK_SIZE,
    SD_SET_DATA_TRANSFER_CLOCKS, 
    SD_SET_CARD_INTERFACE,
    SD_SET_OP_CODE,
    SD_SET_TYPE_COUNT,
    SD_SET_BLOCK_MODE
} SDIO_SET_FEATURE_TYPE;

/* SDIO Information Type */
typedef enum 
{
    SD_GET_RCA,                  // RCA data type for MMC/SDMemory/SD IO Cards
    SD_GET_CARD_INTERFACE,       // card interface for all card types
    SD_GET_SDIO,                 // SDIO information (SD IO only)
    SD_GET_HOST_BLOCK_CAPABILITY,// SD Host Block Length Capabilities
    SD_GET_TYPE_COUNT            // number of SD_INFO_TYPEs 
}SDIO_GET_INFO_TYPE;

typedef enum {
    SDCC_INTERFACE_SDIO_1BIT = 0, 
    SDCC_INTERFACE_SDIO_4BIT = 1, 
}SDCC_SDIO_INTERFACE_MODE;

typedef struct _stMMC_SDIO_CARD_INTERFACE
{
    SDCC_SDIO_INTERFACE_MODE  InterfaceMode;  // interface mode
    unsigned long             ClockRate;      // clock rate
}SDIO_CARD_INTERFACE;

typedef struct _stMMC_SDIO_CARD_INFO 
{
    unsigned char FunctionNumber;   // SDIO function number
    unsigned char DeviceCode;       // device interface code for this function
    dword         CISPointer;       // CIS pointer for this function
    dword         CSAPointer;       // CSA pointer for this function
    unsigned char CardCapability;   // common card capabilities
}SDIO_CARD_INFO;


boolean sdcc_init (void);
boolean sdcc_close(int16 driveno);
boolean sdcc_open (int16 driveno);

boolean sdcc_read (int16   driveno, 
                   uint32  s_sector,
                   byte   *buff, 
                   uint16  n_sectors);

boolean sdcc_write(int16   driveno, 
                   uint32  s_sector,
                   byte   *buff, 
                   uint16  n_sectors);
                   
boolean sdcc_erase (int16  driveno, 
                    uint32 s_sector, 
                    uint16 n_sectors);
                    
uint32  sdcc_ioctl(int16 driveno, 
                   uint8 what);

/* just so we can compile with SFAT */
void   system_controller_close (int16 driveno);

uint32 platform_get_ticks (void);

void   oem_getsysdate (uint16 *tdate, 
                       uint16 *ttime);

int16  get_extended_error (int16 driveno);

int16  get_interface_error (int16 driveno);

void   drive_format_information (int16   driveno, 
                                 uint16 *n_heads, 
                                 uint16 *sec_ptrack, 
                                 uint16 *n_cyls);
                                 
boolean sdcc_read_serial (uint16             driveno, 
                          DRV_GEOMETRY_DESC *iDrvPtr);
                          
boolean sdcc_chk_for_sdio(void);

/* SDIO functions */
SDCC_STATUS  sdcc_sdio_init(uint16 dev_manfid);

SDCC_STATUS  sdcc_sdio_cmd52(uint32  devfn,
                             uint32  dir,
                             uint32  reg_addr,
                             uint8  *pdata);

SDCC_STATUS  sdcc_sdio_write(uint8   devfn,
                             uint32  reg_addr,
                             uint16  count,
                             uint8  *buff);

SDCC_STATUS  sdcc_sdio_read (uint8   devfn,
                             uint32  reg_addr,
                             uint16  count,
                             uint8  *buff);

/* MATS - Added these two byte functions */
SDCC_STATUS  sdcc_sdio_bwrite(uint8   devfn,
                             uint32  reg_addr,
                             uint16  count,
                             uint8  *buff,
                             unsigned int flags);

SDCC_STATUS  sdcc_sdio_bread (uint8   devfn,
                             uint32  reg_addr,
                             uint16  count,
                             uint8  *buff,
                             unsigned int flags);

SDCC_STATUS  sdcc_sdio_set(uint8                   devfn,
                           SDIO_SET_FEATURE_TYPE   type,
                           void                   *pdata);

SDCC_STATUS  sdcc_sdio_get(SDIO_GET_INFO_TYPE      type,
                           void                   *pdata,
                           uint32                  size);

void sdcc_sdio_connect_intr(uint8 devfn,
                            void *isr,
                            void *isr_param);

void sdcc_sdio_disconnect_intr(uint8 devfn);
void sdcc_sdio_abort(uint8 devfn);
void sdcc_sdio_reset_device(void);
void sdcc_isr(void);
void sdcc_enable_int(uint32 int_mask);
void sdcc_disable_int(uint32 int_mask);
#endif /* __SDCC_API_H */

