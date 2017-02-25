/*
 * kernel/private/drivers/ambarella/vout/arch_s2l/vout_arch.c
 *
 * History:
 *    2009/07/23 - [Zhenwu Xue] Create
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


#ifndef __VOUT_ARCH_H
#define __VOUT_ARCH_H

void amba_s2l_vout_set_clock_setup(struct __amba_vout_video_source *psrc,
                                   struct amba_video_source_clock_setup *args);
int amba_s2l_vout_set_hv(struct __amba_vout_video_source *psrc,
                         struct amba_vout_hv_size_info *args);
int amba_s2l_vout_set_active_win(struct __amba_vout_video_source *psrc,
                                 struct amba_vout_window_info *args);
int amba_s2l_vout_set_video_size(struct __amba_vout_video_source *psrc,
                                 struct amba_vout_window_info *args);
int amba_s2l_vout_get_config(struct __amba_vout_video_source *psrc,
	vd_config_t *args);
int amba_s2l_vout_set_config(struct __amba_vout_video_source *psrc,
	vd_config_t *args);
int amba_s2l_vout_set_hvsync(struct __amba_vout_video_source *psrc,
	struct amba_vout_hv_sync_info *args);
int amba_s2l_vout_set_video_info(struct __amba_vout_video_source *psrc,
	struct amba_video_info *args);
int amba_s2l_vout_set_csc(struct __amba_vout_video_source *psrc,
	struct amba_video_source_csc_info *pcsc_cfg);
int amba_s2l_vout_get_setup(struct __amba_vout_video_source *psrc,
	vout_video_setup_t *args);
int amba_s2l_vout_set_setup(struct __amba_vout_video_source *psrc,
	vout_video_setup_t *args);
int amba_s2l_vout_set_vbi(struct __amba_vout_video_source *psrc,
	struct amba_video_sink_mode *psink_mode);
int amba_s2l_vout_get_dve(struct __amba_vout_video_source *psrc,
	dram_dve_t *pdve);
int amba_s2l_vout_set_dve(struct __amba_vout_video_source *psrc,
	dram_dve_t *pdve);
int amba_s2l_vout_set_lcd(struct __amba_vout_video_source *psrc,
	struct amba_vout_lcd_info *plcd_cfg);
int amba_s2l_vout_set_hvld(struct __amba_vout_video_source *psrc,
	struct amba_vout_hvld_sync_info *args);

int amba_s2l_vout_hdmi_init(struct __amba_vout_video_source *psrc,
                      struct amba_video_sink_mode *sink_mode);
void amba_s2l_vout_hdmi_get_native_mode(amba_hdmi_video_timing_t *pvtiming);
int amba_s2l_vout_analog_init(struct __amba_vout_video_source *psrc,
                            struct amba_video_sink_mode *sink_mode);
int amba_s2l_vout_digital_init(struct __amba_vout_video_source *psrc,
                            struct amba_video_sink_mode *sink_mode);
#endif //__VOUT_ARCH_H