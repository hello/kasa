/**
 * @file system/include/flash/spinand/gd5f2gq4uc.h
 *
 * History:
 *    2015/10/26 - [Ken He] created file
 *
 * Copyright (c) 2016 Ambarella, Inc.
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

#ifndef __GD5F2GQ4UC_H__
#define __GD5F2GQ4UC_H__
#include <fio/firmfl.h>

#define SPI_NOR_NAME "GigaDevice SPI Nand GD5F2GQ4UC flash"

/* GD5F2GQ4UC specific command */


/* following must be defined for each spinor flash */
#define SPINOR_SPI_CLOCK		50000000
#define SPINAND_CHIP_SIZE		(256 * 1024 * 1024)

#define SPINAND_MAIN_SIZE		2048
 //we could only use 64 bytes, the other 64 bytes internel ecc
#define SPINAND_SPARE_SIZE		64//128
#define SPINAND_PAGE_SIZE		2176
#define SPINAND_PAGES_PER_BLOCK	64

/* support dtr read mode in the dual io or not*/
#define SUPPORT_DTR_DUAL		0

/* Adjust Reception Sampling Data Phase */
#define SPINOR_RX_SAMPDLY		0
#endif

