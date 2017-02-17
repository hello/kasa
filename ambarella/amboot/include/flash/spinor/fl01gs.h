/**
 * @file system/include/flash/spi_nor/fl01gs.h
 *
 * History:
 *    2014/05/05 - [Cao Rongrong] created file
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


#ifndef __FL01GS_H__
#define __FL01GS_H__
#include <fio/firmfl.h>

#define SPI_NOR_NAME "Spansion SPI Nor FL01GSA flash"

/* fl01gs specific command */
#define FL01GS_BRRD			0x16
#define FL01GS_BRWR			0x17
#define FL01GS_RESET_ENABLE		0xF0

/* following must be defined for each spinor flash */
#define SPINOR_SPI_CLOCK		50000000
#define SPINOR_CHIP_SIZE		(64 * 1024 * 1024)
#define SPINOR_SECTOR_SIZE		(256 * 1024)
#define SPINOR_PAGE_SIZE		512

/* support dtr read mode in the dual io or not*/
#define SUPPORT_DTR_DUAL		1

/* Adjust Reception Sampling Data Phase */
#define SPINOR_RX_SAMPDLY		1
#endif

