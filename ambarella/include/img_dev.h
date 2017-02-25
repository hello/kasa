/**********************************************************************
 * img_dev.c
 *
 * History:
 * 2014/09/17 - [Bin Wang] Created this file
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
#ifndef __IMG_DEV_H__
#define __IMG_DEV_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "devices.h"

/* dc_iris.c */
u32 dc_iris_is_supported(void);
int dc_iris_init(void);
int dc_iris_deinit(void);
int dc_iris_enable(void);
int dc_iris_disable(void);
int dc_iris_update_duty(int duty_cycle);

/* ir_led.c */
u32 ir_led_is_supported(void);
s32 ir_led_init(u32 init);
s32 ir_led_get_brightness(void);
s32 ir_led_set_brightness(s32 brightness);
s32 ir_led_get_state(void);
s32 ir_led_set_state(u32 value);
s32 ir_led_get_adc_value(u32 *value);
u32 ir_cut_is_supported(void);
s32 ir_cut_init(u32 init);
s32 ir_cut_set_state(u32 value);
s32 ir_cut_get_state(void);

#ifdef __cplusplus
}
#endif

#endif //  __IMG_DEV_H__
