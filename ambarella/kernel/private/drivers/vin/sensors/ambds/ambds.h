/*
 * Filename : ambds.h
 *
 * History:
 *    2015/01/30 - [Long Zhao] Create
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

#ifndef __AMBDS_PRI_H__
#define __AMBDS_PRI_H__

#define AMBDS_STANDBY    		0x3000
#define AMBDS_XMSTA			0x3008
#define AMBDS_SWRESET    		0x3009

#define AMBDS_WINMODE    		0x300F

#define AMBDS_GAIN_LSB    		0x301F
#define AMBDS_GAIN_MSB    		0x3020

#define AMBDS_VMAX_LSB    		0x302C
#define AMBDS_VMAX_MSB    		0x302D
#define AMBDS_VMAX_HSB   		0x302E
#define AMBDS_HMAX_LSB    		0x302F
#define AMBDS_HMAX_MSB   		0x3030

#define AMBDS_SHS1_LSB    		0x3034
#define AMBDS_SHS1_MSB   		0x3035
#define AMBDS_SHS1_HSB   		0x3036

#endif /* __AMBDS_PRI_H__ */

