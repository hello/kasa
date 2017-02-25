/*
 * amba_debug.h
 *
 * History:
 *    2008/04/10 - [Anthony Ginger] Create
 *
 * Copyright (C) 2015 Ambarella, Inc.
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

#ifndef __AMBA_DEBUG_H
#define __AMBA_DEBUG_H

#define AMBA_DEBUG_IOC_MAGIC		'd'

#define AMBA_DEBUG_IOC_GET_DEBUG_FLAG		_IOR(AMBA_DEBUG_IOC_MAGIC, 1, int *)
#define AMBA_DEBUG_IOC_SET_DEBUG_FLAG		_IOW(AMBA_DEBUG_IOC_MAGIC, 1, int *)

#define AMBA_DEBUG_IOC_VIN_SET_SRC_ID		_IOR(AMBA_DEBUG_IOC_MAGIC, 200, int)
#define AMBA_DEBUG_IOC_VIN_GET_DEV_ID		_IOR(AMBA_DEBUG_IOC_MAGIC, 201, u32 *)

struct amba_vin_test_reg_data {
	u32 reg;
	u32 data;
	u32 regmap;
};
#define AMBA_DEBUG_IOC_VIN_GET_REG_DATA		_IOR(AMBA_DEBUG_IOC_MAGIC, 202, struct amba_vin_test_reg_data *)
#define AMBA_DEBUG_IOC_VIN_SET_REG_DATA		_IOW(AMBA_DEBUG_IOC_MAGIC, 202, struct amba_vin_test_reg_data *)

struct amba_vin_test_gpio {
	u32 id;
	u32 data;
};
#define AMBA_DEBUG_IOC_GET_GPIO			_IOR(AMBA_DEBUG_IOC_MAGIC, 203, struct amba_vin_test_gpio *)
#define AMBA_DEBUG_IOC_SET_GPIO			_IOW(AMBA_DEBUG_IOC_MAGIC, 203, struct amba_vin_test_gpio *)

#endif	//AMBA_DEBUG_IOC_MAGIC

