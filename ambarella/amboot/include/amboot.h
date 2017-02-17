/**
 * amboot.h
 *
 * Author: Anthony Ginger <hfjiang@ambarella.com>
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


#ifndef __AMBOOT_H__
#define __AMBOOT_H__

/*===========================================================================*/
#include <config.h>
#include <basedef.h>

/*
 * bit [32 - 16]: unused, should be 0
 * bit [15 - 8]: major version
 * bit [ 7 - 0]: minor version
 */
#define AMBOOT_MAJOR_VER			0x3
#define AMBOOT_MINOR_VER			0x0
#define amboot_show_version()			printf("Version: %d.%d - %s\n",	\
							AMBOOT_MAJOR_VER,	\
							AMBOOT_MINOR_VER,	\
							__BUILD_TIME__)

/*===========================================================================*/
#define AMBARELLA_BOARD_TYPE_AUTO		(0)
#define AMBARELLA_BOARD_TYPE_BUB		(1)
#define AMBARELLA_BOARD_TYPE_EVK		(2)
#define AMBARELLA_BOARD_TYPE_IPCAM		(3)
#define AMBARELLA_BOARD_TYPE_VENDOR		(4)
#define AMBARELLA_BOARD_TYPE_ATB		(5)

#define AMBARELLA_BOARD_CHIP_AUTO		(0)

#define AMBARELLA_BOARD_REV_AUTO		(0)

#define AMBARELLA_BOARD_VERSION(c,t,r)		(((c) << 16) + ((t) << 12) + (r))
#define AMBARELLA_BOARD_CHIP(v)			(((v) >> 16) & 0xFFFF)
#define AMBARELLA_BOARD_TYPE(v)			(((v) >> 12) & 0xF)
#define AMBARELLA_BOARD_REV(v)			(((v) >> 0) & 0xFFF)

#include <bsp.h>

/*===========================================================================*/
#include <ambhw/chip.h>
#include <ambhw/rct.h>
#include <ambhw/memory.h>

/* ==========================================================================*/
#ifndef __ASM__
#include <vsprintf.h>
#endif
/* ==========================================================================*/

#endif

