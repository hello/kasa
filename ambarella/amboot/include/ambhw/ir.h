/*
 * ambhw/ir.h
 *
 * History:
 *	2006/12/27 - [Charles Chiou] created file
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


#ifndef __AMBHW__IR_H__
#define __AMBHW__IR_H__

/* ==========================================================================*/
#define IR_OFFSET			0x6000
#define IR_BASE				(APB_BASE + IR_OFFSET)
#define IR_REG(x)			(IR_BASE + (x))

/* ==========================================================================*/
#define IR_CONTROL_OFFSET		0x00
#define IR_STATUS_OFFSET		0x04
#define IR_DATA_OFFSET			0x08

#define IR_CONTROL_REG			IR_REG(0x00)
#define IR_STATUS_REG			IR_REG(0x04)
#define IR_DATA_REG			IR_REG(0x08)

/* IR_CONTROL_REG */
#define IR_CONTROL_RESET		0x00004000
#define IR_CONTROL_ENB			0x00002000
#define IR_CONTROL_LEVINT		0x00001000
#define IR_CONTROL_INTLEV(x)		(((x) & 0x3f)  << 4)
#define IR_CONTROL_FIFO_OV		0x00000008
#define IR_CONTROL_INTENB		0x00000004

#define IR_STATUS_COUNT(x)		((x) & 0x3f)
#define IR_DATA_DATA(x)			((x) & 0xffff)

#endif
