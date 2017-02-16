//------------------------------------------------------------------------------
// <copyright file="eeprom.c" company="Atheros">
//    Copyright (c) 2008 Atheros Corporation.  All rights reserved.
// $ATH_LICENSE_HOSTSDK0_C$
//------------------------------------------------------------------------------
//==============================================================================
// EEPROM functions
//
// Author(s): ="Atheros"
//==============================================================================
#include "a_config.h"
#include "a_types.h"
#include "athdefs.h"
#include "a_debug.h"

#include "AR6002/hw2.0/hw/mbox_host_reg.h"
#include "AR6002/hw2.0/hw/apb_map.h"
#include "AR6002/hw2.0/hw/si_reg.h"
#include "AR6002/hw2.0/hw/gpio_reg.h"
#include "AR6002/hw2.0/hw/rtc_reg.h"
#include "AR6002/hw2.0/hw/mbox_reg.h"
#include "AR6002/hw2.0/hw/vmc_reg.h"

#include "hif.h"
#include "bmi.h"
#include "targaddrs.h"
#include "targaddrs_rexos.h"

#define MAX_EEPROM_SZ                     768
#define EEPROM_WAIT_LIMIT             4

static void writeboard_data (HIF_DEVICE *device, A_UINT32 address, A_UINT8 *data, A_UINT32 sz);
static void enable_SI(HIF_DEVICE *device, A_UINT32 TargetType);
static void disable_SI(HIF_DEVICE *device, A_UINT32 TargetType);
static void fetch_8bytes (HIF_DEVICE *device, A_INT32 offset, A_UINT32 *data, A_UINT32 TargetType);
static void commit_4bytes(HIF_DEVICE *device, A_INT32 offset, A_UINT32 data, A_UINT32 TargetType);
static void calculate_crc(A_UINT32 TargetType);

#define AR6000_RTC_BASE_ADDRESS(x)        (((x) == TARGET_TYPE_AR6001) ?0x0c000000:0x00004000)
#define AR6000_SLEEP_OFFSET(x)            (((x) == TARGET_TYPE_AR6001) ?0x000000d0:0x000000c4)
#define AR6000_CLOCK_CONTROL_OFFSET(x)    (((x) == TARGET_TYPE_AR6001) ?0x0000002c:0x00000028)
#define AR6000_GPIO_BASE_ADDRESS(x)       (((x) == TARGET_TYPE_AR6001) ?0x0c010000:0x00014000)
#define AR6000_SI_BASE_ADDRESS(x)         (((x) == TARGET_TYPE_AR6001) ?0x0c00c000:0x00010000)

static A_UCHAR  eeprom_data[MAX_EEPROM_SZ];
A_UCHAR* pSoftMacAddr = NULL;

A_STATUS
eeprom_ar6000_transfer (HIF_DEVICE *device,
        A_UINT32 TargetType,
        A_CHAR *fileRoot,
        A_CHAR *eepromFile)
{
    A_UINT32        sys_sleep_reg;
    A_UINT32        eeprom_size;
    static A_UINT32 board_data_addr = 0;
    A_STATUS        status;
    A_UINT32        i;
    static A_BOOL   eeprom_read = FALSE;

    if (TargetType == TARGET_TYPE_AR6001)
    {
        eeprom_size = 512;
    }
    else
    {
        eeprom_size = 768;
    }

    //
    //disable system sleep temporarily
    //
    BMIReadSOCRegister (device, AR6000_RTC_BASE_ADDRESS(TargetType)+
                                AR6000_SLEEP_OFFSET(TargetType),
                                &sys_sleep_reg);

    BMIWriteSOCRegister (device, AR6000_RTC_BASE_ADDRESS(TargetType)+
                                 AR6000_SLEEP_OFFSET(TargetType),
                                 SYSTEM_SLEEP_DISABLE_SET(1));

    // If eeprom is already read, do not read it again
    if (!eeprom_read) {
        if (A_STRCMP(eepromFile, "\0") == 0) {
            A_PRINTF("Read boarddata from eeprom \n");
            
            enable_SI(device, TargetType);
            for (i = 0; i < eeprom_size; i += 8)
            {
                fetch_8bytes (device, i, (A_UINT32*)(&eeprom_data[i]), TargetType);
            }
            disable_SI(device, TargetType);
        } else {
            void* fHandle;
            A_UINT32 bytes_read;
            
            A_PRINTF("Read board data from file %s \n", fileRoot);
            fHandle = A_FL_INITIALIZE(eepromFile, fileRoot, A_FL_RDONLY);
            if (fHandle == FL_NULL) {
                A_PRINTF("Cannot open eeprom file \n");
                return A_ERROR;
            }

            if (A_FL_GET_DATA(fHandle, 0, (A_CHAR *) eeprom_data, eeprom_size) < 0) {
                A_PRINTF("Cannot read eeprom file \n");
                A_FL_UNINITIALIZE(fHandle);
                return A_ERROR;
            }

            if (bytes_read != eeprom_size) {
                A_PRINTF("EEPROM file size mismatch expected %d got %d \n",eeprom_size,bytes_read);
                A_FL_UNINITIALIZE(fHandle);
                return A_ERROR;
            }
            A_FL_UNINITIALIZE(fHandle);
        }

        /* Determine where in Target RAM to write Board Data */
        status = BMIReadMemory(device,
                         HOST_INTEREST_ITEM_ADDRESS(TargetType, hi_board_data),
                         (A_UCHAR *)&board_data_addr,
                         sizeof(A_UINT32));
        if (status != A_OK) {
            return A_ERROR;
        }
       
        /* Soft mac. Overwrite the mac address in the eeprom */ 
        if (pSoftMacAddr) {
            A_UCHAR *ptr_mac;
            ptr_mac = (A_UINT8 *)((A_UCHAR *)eeprom_data + ((TargetType == TARGET_TYPE_AR6002) ? 0x0A : 0x06));
            A_MEMCPY(ptr_mac, pSoftMacAddr, 6);
            calculate_crc(TargetType);
        }

        eeprom_read = TRUE;
    }

    // put SI block in reset state. Need this as the default is not in reset
    BMIWriteSOCRegister(device, AR6000_RTC_BASE_ADDRESS(TargetType)+
                                 RESET_CONTROL_OFFSET,
                                 RESET_CONTROL_SI0_RST_MASK);

    /* Write EEPROM data to Target RAM */
    writeboard_data (device, board_data_addr, ((A_UINT8 *)eeprom_data), eeprom_size);

    /* Record the fact that Board Data IS initialized */
    {
       A_UINT32 one = 1;
       writeboard_data (device, HOST_INTEREST_ITEM_ADDRESS(TargetType,
                                hi_board_data_initialized),
                                (A_UINT8 *)&one, sizeof(A_UINT32));
    }

    // Enable System Sleep setting
    BMIWriteSOCRegister (device, AR6000_RTC_BASE_ADDRESS(TargetType)+
                                 AR6000_SLEEP_OFFSET(TargetType),
                                 sys_sleep_reg);

    return A_OK;
}

void
ar6000_set_softmac_addr(A_UCHAR* p)
{
    pSoftMacAddr = p;
}

static
void calculate_crc(A_UINT32 TargetType)
{
    A_UINT16        *ptr_crc;
    A_UINT16        *ptr16_eeprom;
    A_UINT16        checksum;
    A_UINT32        i;
    A_UINT32        eeprom_size;

    if (TargetType == TARGET_TYPE_AR6001)
    {
        eeprom_size = 512;
        ptr_crc = (A_UINT16 *)eeprom_data;
    }
    else
    {
        eeprom_size = 768;
        ptr_crc = (A_UINT16 *)((A_UCHAR *)eeprom_data + 0x04);
    }


    // Clear the crc
    *ptr_crc = 0;

    // Recalculate new CRC
    checksum = 0;
    ptr16_eeprom = (A_UINT16 *)eeprom_data;
    for (i = 0;i < eeprom_size; i += 2)
    {
        checksum = checksum ^ (*ptr16_eeprom);
        ptr16_eeprom++;
    }
    checksum = 0xFFFF ^ checksum;
    *ptr_crc = checksum;
}

A_STATUS
eeprom_write(HIF_DEVICE *device,
        A_UINT32 TargetType,
        A_UCHAR  *data,
        A_UINT32 size,
        A_UINT32 offset)
{
    A_UINT32        sys_sleep_reg;
    A_UINT32        eeprom_size;
    A_UINT32        i;
    A_UINT8         *ptr_data;
    A_UINT32        *ptr32_eeprom;

    if (TargetType == TARGET_TYPE_AR6001)
    {
        eeprom_size = 512;
    }
    else
    {
        eeprom_size = 768;
    }

    //
    //disable system sleep temporarily
    //
    BMIReadSOCRegister (device, AR6000_RTC_BASE_ADDRESS(TargetType)+
                                AR6000_SLEEP_OFFSET(TargetType),
                                &sys_sleep_reg);

    BMIWriteSOCRegister (device, AR6000_RTC_BASE_ADDRESS(TargetType)+
                                 AR6000_SLEEP_OFFSET(TargetType),
                                 SYSTEM_SLEEP_DISABLE_SET(1));

    enable_SI(device, TargetType);

    for (i = 0; i < eeprom_size; i += 8)
    {
        fetch_8bytes (device, i, (A_UINT32*)(&eeprom_data[i]), TargetType);
    }

    // Copy data
    ptr_data = (A_UINT8 *)((A_UCHAR *)eeprom_data + offset);
    A_MEMCPY(ptr_data, data, size);

    calculate_crc(TargetType);

    ptr32_eeprom = (A_UINT32 *)eeprom_data;
    for (i = 0; i < eeprom_size; i += 4)
    {
        commit_4bytes(device, i, *ptr32_eeprom, TargetType);
        ptr32_eeprom++;
        A_MDELAY(5);  /* Sleep 5 ms for EEPROM max write time. Depends on part*/
    }

    disable_SI(device, TargetType);
    // Enable System Sleep setting
    BMIWriteSOCRegister (device, AR6000_RTC_BASE_ADDRESS(TargetType)+
                                 AR6000_SLEEP_OFFSET(TargetType),
                                 sys_sleep_reg);


    return A_OK;
}

A_STATUS
eeprom_update_macaddr(HIF_DEVICE *device,
        A_UINT32 TargetType,
        A_UCHAR  *mac_addr)
{
    A_UINT32 offset;
    A_STATUS status;

    offset = (TargetType == TARGET_TYPE_AR6002) ? 0x0A : 0x06;

    status = eeprom_write(device, TargetType, mac_addr, 6, offset);

    return status;
}

A_STATUS
eeprom_update_rd(HIF_DEVICE *device,
        A_UINT32 TargetType,
        A_UINT32 rd
        )
{
    A_UINT32 offset;
    A_STATUS status;
    A_UCHAR rd_char[2];

    offset = (TargetType == TARGET_TYPE_AR6002) ? 0x08 : 0x04;
    rd_char[0] = rd & 0xFF;
    rd_char[1] = (rd >> 8) & 0xFF;

    status = eeprom_write(device, TargetType, rd_char, 2, offset);

    return status;
}

/*
 * Check whether or not an EEPROM request that was started
 * earlier has completed yet.
 */
A_BOOL
request_in_progress (HIF_DEVICE *device, A_UINT32 TargetType)
{
    A_UINT32 regval;

    /* Wait for DONE_INT in SI_CS */
    BMIReadSOCRegister (device, AR6000_SI_BASE_ADDRESS(TargetType)+
                                SI_CS_OFFSET,
                                &regval);

    if (regval & SI_CS_DONE_ERR_MASK) {
        AR_DEBUG_PRINTF(ATH_LOG_ERR,("Err:EEPROM signaled ERROR (0x%x)\n", regval));
    }

    return (!(regval & SI_CS_DONE_INT_MASK));
}


/*
 * Tell the Target to start an 8-byte read from EEPROM,
 * putting the results in Target RX_DATA registers.
 */
void
request_8byte_read (HIF_DEVICE *device,
                    A_INT32 offset,
                    A_UINT32 TargetType)
{
    A_UINT32 regval;

    /* SI_TX_DATA0 = read from offset */
    regval =    (0xa1<<16)              |
                ((offset & 0xff)<<8)    |
                (0xa0 | ((offset & 0xff00)>>7));

    BMIWriteSOCRegister (device, AR6000_SI_BASE_ADDRESS(TargetType)+
                                 SI_TX_DATA0_OFFSET,
                                 regval);

    regval =    SI_CS_START_SET(1)      |
                SI_CS_RX_CNT_SET(8)     |
                SI_CS_TX_CNT_SET(3);

    BMIWriteSOCRegister (device, AR6000_SI_BASE_ADDRESS(TargetType)+
                                 SI_CS_OFFSET,
                                 regval);


}

/*
 * Wait for a previously started command to complete.
 * Timeout if the command is takes "too long".
 */
void
wait_for_eeprom_completion (HIF_DEVICE *device, A_UINT32 TargetType)
{
    A_INT32 i = 0;

    while (request_in_progress(device, TargetType)) {
        if (i++ == EEPROM_WAIT_LIMIT)
        {
            i = 0;
            AR_DEBUG_PRINTF(ATH_LOG_ERR,("Err:LEEPROM not responding\n"));
        }
    }
}

/*
 * Extract the results of a completed EEPROM Read request
 * and return them to the caller.
 */
void
read_8byte_results (HIF_DEVICE *device,
                    A_UINT32 *data,
                    A_UINT32 TargetType)
{
    /* Read SI_RX_DATA0 and SI_RX_DATA1 */
    BMIReadSOCRegister (device, AR6000_SI_BASE_ADDRESS(TargetType)+
                                SI_RX_DATA0_OFFSET,
                                &data[0]);
    BMIReadSOCRegister (device, AR6000_SI_BASE_ADDRESS(TargetType)+
                                SI_RX_DATA1_OFFSET,
                                &data[1]);
}

/*
 * High-level function which starts an 8-byte read,
 * waits for it to complete, and returns the result.
 */
static void
fetch_8bytes (HIF_DEVICE *device,
              A_INT32   offset,
              A_UINT32 *data,
              A_UINT32 TargetType)
{
    request_8byte_read (device, offset, TargetType);
    wait_for_eeprom_completion (device, TargetType);
    read_8byte_results (device, data, TargetType);

    /* Clear any pending intr */
    BMIWriteSOCRegister (device, AR6000_SI_BASE_ADDRESS(TargetType)+
                                 SI_CS_OFFSET,
                                 SI_CS_DONE_INT_MASK);
}

/*
 * Tell the Target to start a 4-byte write to EEPROM,
 * writing values from Target TX_DATA registers.
 */
void
request_4byte_write(HIF_DEVICE *device,
    A_INT32 offset,
    A_UINT32 data,
    A_UINT32 TargetType)
{
    A_UINT32 regval;

    /* SI_TX_DATA0 = write data to offset */
    regval =    ((data & 0xffff) <<16)    |
                ((offset & 0xff)<<8)    |
                (0xa0 | ((offset & 0xff00)>>7));
    BMIWriteSOCRegister(device, AR6000_SI_BASE_ADDRESS(TargetType)+SI_TX_DATA0_OFFSET, regval);

    regval =    data >> 16;
    BMIWriteSOCRegister(device, AR6000_SI_BASE_ADDRESS(TargetType)+SI_TX_DATA1_OFFSET, regval);

    regval =    SI_CS_START_SET(1)      |
                SI_CS_RX_CNT_SET(0)     |
                SI_CS_TX_CNT_SET(6);
    BMIWriteSOCRegister(device, AR6000_SI_BASE_ADDRESS(TargetType)+SI_CS_OFFSET, regval);

    return;
}

/*
 * High-level function which starts a 4-byte write,
 * and waits for it to complete.
 */
void
commit_4bytes(HIF_DEVICE *device,
        A_INT32 offset,
        A_UINT32 data,
        A_UINT32 TargetType)
{
    request_4byte_write(device, offset, data, TargetType);
    wait_for_eeprom_completion(device, TargetType);
}


static void
writeboard_data (HIF_DEVICE *device,
                 A_UINT32 address,
                 A_UINT8 *data,
                 A_UINT32 sz)
{
   A_INT32 chunk_sz;
   A_UCHAR buffer[BMI_DATASZ_MAX+4];
   A_STATUS status;

   memset(buffer, 0, sizeof(buffer));

   while (sz) {

      chunk_sz = (BMI_DATASZ_MAX > sz) ? sz : BMI_DATASZ_MAX;

      memcpy (buffer, data, chunk_sz);

      status = BMIWriteMemory (device, address, (A_UCHAR *)(&buffer[0]), chunk_sz);

      if (A_OK != status)
      {
          break;
      }

      sz -= chunk_sz;
      data += chunk_sz;
      address += chunk_sz;
   }
}

static void
enable_SI(HIF_DEVICE *device,
            A_UINT32 TargetType)
{
    A_UINT32        regval = 0;    


    BMIReadSOCRegister (device, AR6000_RTC_BASE_ADDRESS(TargetType)+
                                AR6000_CLOCK_CONTROL_OFFSET(TargetType),
                                &regval);
    regval &= ~CLOCK_CONTROL_SI0_CLK_MASK;
    BMIWriteSOCRegister (device, AR6000_RTC_BASE_ADDRESS(TargetType)+
                                 AR6000_CLOCK_CONTROL_OFFSET(TargetType),
                                 regval);


    BMIReadSOCRegister (device, AR6000_RTC_BASE_ADDRESS(TargetType)+
                                RESET_CONTROL_OFFSET,
                                &regval);
    regval &= ~RESET_CONTROL_SI0_RST_MASK;
    BMIWriteSOCRegister (device, AR6000_RTC_BASE_ADDRESS(TargetType)+
                                 RESET_CONTROL_OFFSET,
                                 regval);

    BMIReadSOCRegister (device, AR6000_GPIO_BASE_ADDRESS(TargetType)+
                                GPIO_PIN0_OFFSET,
                                &regval);
    regval &= ~GPIO_PIN0_CONFIG_MASK;
    BMIWriteSOCRegister (device, AR6000_GPIO_BASE_ADDRESS(TargetType)+
                                 GPIO_PIN0_OFFSET,
                                 regval);

    BMIReadSOCRegister (device, AR6000_GPIO_BASE_ADDRESS(TargetType)+
                                GPIO_PIN1_OFFSET,
                                &regval);
    regval &= ~GPIO_PIN1_CONFIG_MASK;
    BMIWriteSOCRegister (device, AR6000_GPIO_BASE_ADDRESS(TargetType)+
                                 GPIO_PIN1_OFFSET,
                                 regval);

    /* SI_CONFIG = 0x500a5; */
    regval =    SI_CONFIG_BIDIR_OD_DATA_SET(1)  |
                SI_CONFIG_I2C_SET(1)            |
                SI_CONFIG_POS_SAMPLE_SET(1)     |
                SI_CONFIG_INACTIVE_CLK_SET(1)   |
                SI_CONFIG_INACTIVE_DATA_SET(1)  |
                SI_CONFIG_DIVIDER_SET(6);

    BMIWriteSOCRegister (device, AR6000_SI_BASE_ADDRESS(TargetType)+
                                 SI_CONFIG_OFFSET,
                                 regval);

    return;
}

static void
disable_SI(HIF_DEVICE *device,
            A_UINT32 TargetType)
{
    A_UINT32        regval = 0;

    // Disable SI
    BMIReadSOCRegister(device,  AR6000_RTC_BASE_ADDRESS(TargetType)+
                                AR6000_CLOCK_CONTROL_OFFSET(TargetType),
                                &regval);
    regval |= CLOCK_CONTROL_SI0_CLK_MASK;
    BMIWriteSOCRegister(device, AR6000_RTC_BASE_ADDRESS(TargetType)+
                            AR6000_CLOCK_CONTROL_OFFSET(TargetType),
                            regval);//Gate SI0 clock

    return;
}

