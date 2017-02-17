/**
 * @file system/include/flash/spi_nor/n25q256.h
 *
 * History:
 *    2013/10/15 - [Johnson Diao] created file
 *
 *
 * Copyright (c) 2015 Ambarella, Inc.
 *
 * This file and its contents ("Software") are protected by intellectual
 * property rights including, without limitation, U.S. and/or foreign
 * copyrights. This Software is also the confidential and proprietary
 * information of Ambarella, Inc. and its licensors. You may not use, reproduce,
 * disclose, distribute, modify, or otherwise prepare derivative works of this
 * Software or any portion thereof except pursuant to a signed license agreement
 * or nondisclosure agreement with Ambarella, Inc. or its authorized affiliates.
 * In the absence of such an agreement, you agree to promptly notify and return
 * this Software to Ambarella, Inc.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF NON-INFRINGEMENT,
 * MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL AMBARELLA, INC. OR ITS AFFILIATES BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; COMPUTER FAILURE OR MALFUNCTION; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */


#ifndef __N25Q256_H__
#define __N25Q256_H__
#include <fio/firmfl.h>

#define SPI_NOR_NAME "Micron SPI Nor N25Q256A flash"

//command
#define N25Q256_RESET_ENABLE 0x66
#define N25Q256_RESET_MEMORY 0x99
#define N25Q256_READ_ID      0x9E
#define N25Q256_MUL_READ_ID  0xAF
#define N25Q256_READ_SE_FLASH 0x5A
#define N25Q256_READ         0x03
#define N25Q256_FAST_READ    0x0B
#define N25Q256_DUAL_OUT_FAST_READ 0x3B
#define N25Q256_DUAL_IN_OUT_FAST_READ 0x0B
#define N25Q256_QUAD_OUT_FAST_READ 0x6B
#define N25Q256_QUAD_IN_OUT_FAST_RREAD 0xEB
#define N25Q256_FAST_READ_DTR 0x0D
#define N25Q256_DUAL_OUT_FAST_READ_DTR 0x3D
#define N25Q256_DUAL_IN_OUT_FAST_READ_DTR 0x0D
#define N25Q256_QUAD_OUT_FAST_READ_DTR 0x6D
#define N25Q256_QUAD_IN_OUT_FAST_READ_DTR 0x0D
#define N25Q256_4_BYTE_READ 0x13
#define N25Q256_4_BYTE_FAST_READ 0x0C
#define N25Q256_4_BYTE_DUAL_OUT_FAST_READ 0x3C
#define N25Q256_4_BYTE_DUAL_IN_OUT_FAST_READ 0xBC
#define N25Q256_4_BYTE_QUAD_OUT_FAST_READ 0x6C
#define N25Q256_4_BYTE_QUAD_IN_OUT_FAST_READ 0xEC

#define N25Q256_WRITEEN           0x06
#define N25Q256_WRITE_DISABLE     0x04

#define N25Q256_READ_STATUS_REG     0x05
#define N25Q256_WRITE_STATUS_REG    0x01
#define N25Q256_READ_LOCK_REG       0xE8
#define N25Q256_WRITE_LOCK_REG      0xE5
#define N25Q256_READ_FLAG_STATUS_REG  0x70
#define N25Q256_CLEAR_FLAG_STATUS_REG 0x50
#define N25Q256_READ_NONVO_CONF_REG   0xB5
#define N25Q256_WRITE_NONVO_CONF_REG  0xB1
#define N25Q256_READ_VOL_CONF_REG     0x85
#define N25Q256_WRITE_VOL_CONF_REG    0x81
#define N25Q256_READ_ENH_VOL_CONF_REG  0x65
#define N25Q256_WRITE_ENH_VOL_CONF_REG 0x61
#define N25Q256_READ_EXT_ADDR_REG      0xC8
#define N25Q256_WRITE_EXT_ADDR_REG     0xC5
#define N25Q256_PAGE_PROG              0x02
#define N25Q256_4_BYTE_PAGE_PROG       0x12
#define N25Q256_DUAL_IN_FAST_PROG      0xA2
#define N25Q256_EXT_DUAL_IN_FAST_PROG  0x02
#define N25Q256_QUAD_IN_FAST_PROG      0x32
#define N25Q256_4_BYTE_QUAD_IN_FAST_PROG 0x34
#define N25Q256_EXT_QUAD_IN_FAST_PR0G   0x02
#define N25Q256_SUB_ERASE              0x20
#define N25Q256_4_BYTE_SUB_ERASE       0x21
#define N25Q256_SEC_ERASE              0xD8
#define N25Q256_4_BYTE_SEC_ERASE       0xDC
#define N25Q256_BULK_ERASE             0xC7
#define N25Q256_PROG_ERASE_RESUME      0x7A
#define N25Q256_PROG_ERASE_SUSPEND     0x75
#define N25Q256_READ_OTP_ARRY          0x4B
#define N25Q256_PROG_OTP_ARRY          0x42
#define N25Q256_ENTER_4_BYTE_ADDR_MODE  0xB7
#define N25Q256_EXIT_4_BYTE_ADDR_MODE   0xE9
#define N25Q256_ENTER_QUAD              0x35
#define N25Q256_EXIT_QUAD               0xF5

#define N25Q256_WRITE_ENABLE_LATCH_BIT (1<<1)
#define N25Q256_WRITE_IN_PROGESS        (1)
#define N25Q256_PROG_ERR_BIT           (1<<4)
#define N25Q256_ERASE_ERR_BIT         (1<<5)

#define N25Q256_BYTES_PER_SUBSECTOR (4*1024)
#define N25Q256_PAGES_PER_SUBSECTOR  16 // we use subsector as "sector" in software, for subsector is the minest erase unit
#define N25Q256_PAGE_SIZE    256


extern int n25q256_init(void);
extern int n25q256_read(u32 addr,u32 data_len,const u8 *dst);
extern int n25q256_write_sector(u32 sector,u8 *src);
extern int n25q256_erase_sector(u32 sector);
extern int n25q256_write_bst(u32 sector,u8 *src);
#endif

