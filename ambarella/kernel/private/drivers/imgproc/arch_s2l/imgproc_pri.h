/*
 * imgproc_pri.h
 *
 * History:
 *      2008/04/02 - [Andrew Lu] created file
 *      2014/07/14 - [Jian Tang] modified file
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

#ifndef __IMGPROC_PRI_H__
#define __IMGPROC_PRI_H__

#if 1
#define img_printk(S...)		printk(KERN_DEBUG "img: "S)
#define img_dsp(cmd, arg)	\
	printk(KERN_DEBUG "%s: "#arg" = %d\n", __func__, (u32)cmd.arg)

#define img_dsp_hex(cmd, arg)	\
	printk(KERN_DEBUG "%s: "#arg" = 0x%x\n", __func__, (u32)cmd.arg)
#else
#define img_printk(S...)
#define img_dsp(cmd, arg)
#define img_dsp_hex(cmd, arg)
#endif

#ifdef LOG_DSP_CMD
#define DBG_CMD		printk
#else
#define DBG_CMD(...)
#endif


#endif

