/*
 * ambhw/drctl.h
 *
 * History:
 *	2007/01/27 - [Charles Chiou] created file
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

#ifndef __AMBHW__DRCTRL_H__
#define __AMBHW__DRCTRL_H__

/* ==========================================================================*/
#if (CHIP_REV == A5S) || (CHIP_REV == S2)
#define DRAM_PHYS_BASE			0x60000000
#elif (CHIP_REV == A7L)
#define DRAM_PHYS_BASE			0xFFFE0000
#else
#define DRAM_PHYS_BASE			0xDFFE0000
#endif

#if (CHIP_REV == A5S) || (CHIP_REV == S2)
#define DRAM_DRAM_OFFSET 		0x4000
#define DRAM_DDRC_OFFSET  		DRAM_DRAM_OFFSET
#else
#define DRAM_DRAM_OFFSET 		0x0000
#define DRAM_DDRC_OFFSET  		0x0800
#endif

#define DDRC_REG(x)			(DRAM_PHYS_BASE + DRAM_DDRC_OFFSET + (x))
#define DRAM_REG(x)			(DRAM_PHYS_BASE + DRAM_DRAM_OFFSET + (x))

/* ==========================================================================*/
#define DDRC_CTL_OFFSET			(0x00)
#define DDRC_CFG_OFFSET			(0x04)
#define DDRC_TIMING1_OFFSET		(0x08)
#define DDRC_TIMING2_OFFSET		(0x0C)
#define DDRC_TIMING3_OFFSET		(0x10)
#define DDRC_INIT_CTL_OFFSET		(0x14)
#define DDRC_MODE_OFFSET		(0x18)
#define DDRC_SELF_REF_OFFSET		(0x1C)
#define DDRC_DQS_SYNC_OFFSET		(0x20)
#define DDRC_PAD_ZCTL_OFFSET		(0x24)
#define DDRC_ZQ_CALIB_OFFSET		(0x28)
#define DDRC_RSVD_SPACE_OFFSET		(0x2C)
#define DDRC_BYTE_MAP_OFFSET		(0x30)
#define DDRC_DDR3PLUS_BNKGRP_OFFSET	(0x34)
#define DDRC_POWER_DOWN_OFFSET		(0x38)
#define DDRC_DLL_CALIB_OFFSET		(0x3C)
#define DDRC_DEBUG1_OFFSET		(0x40)

/* ==========================================================================*/
#endif /* __AMBHW__DRCTRL_H__ */

