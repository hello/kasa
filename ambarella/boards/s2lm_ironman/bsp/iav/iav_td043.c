/**
 * boards/hawthorn/bsp/iav/iav_td043.c
 *
 * History:
 *    2014/08/11 - [Cao Rongrong] created file
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


static u32 vout_display_setup_dram[] = {
	0x08000000, 0x00000000, 0x03AB01F7, 0x000001F7,
	0x00780014, 0x039701F3, 0x00780015, 0x039701F4,
	0x00108080, 0x18080000, 0x00000002, 0x00000000,
	0x00000002, 0x00000000, 0x00000002, 0x01F40014,
	0x00000074, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x1E6F04A8, 0x04A81CBF, 0x1FFF0812,
	0x1FFE04A8, 0x00880662, 0x7F217EEB, 0x00FF0000,
	0x00FF0000, 0x00FF0000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000,
};

static u32 vout_osd_setup_dram[] = {
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
};

/* ==========================================================================*/
static vout_reset_t vout_reset = {
	.cmd_code = VOUT_RESET,
	.vout_id = 1,
	.reset_mixer = 1,
	.reset_disp = 1
};

static vout_mixer_setup_t vout_mixer_setup = {
	.cmd_code = VOUT_MIXER_SETUP,
	.vout_id = 0,
	.interlaced = 0,
	.frm_rate = 60,
	.act_win_width = 800,
	.act_win_height = 480,
	.back_ground_v = 128,
	.back_ground_u = 128,
	.back_ground_y = 16,
	.mixer_444 = 1,
	.highlight_v = 0,
	.highlight_u = 0,
	.highlight_y = 0,
	.highlight_thresh = 255,
	.reverse_en = 0,
	.csc_en = 0,
	.csc_parms[0] = 0,
	.csc_parms[1] = 0,
	.csc_parms[2] = 0,
	.csc_parms[3] = 0,
	.csc_parms[4] = 0,
	.csc_parms[5] = 0,
	.csc_parms[6] = 0,
	.csc_parms[7] = 0,
	.csc_parms[8] = 0
};

static vout_video_setup_t vout_video_setup = {
	.cmd_code = VOUT_VIDEO_SETUP,
	.vout_id = 0,
	.en = 1,
	.src = 2,
	.flip = 0,
	.rotate = 0,
	.data_src = 0,
	.win_offset_x = 0,
	.win_offset_y = 0,
	.win_width = 800,
	.win_height = 480,
	.default_img_y_addr = 0x00000000,
	.default_img_uv_addr = 0x00000000,
	.default_img_pitch = 0,
	.default_img_repeat_field = 0
};

static vout_display_setup_t vout_display_setup = {
	.cmd_code = VOUT_DISPLAY_SETUP,
	.vout_id = 0,
	.dual_vout_vysnc_delay_ms_x10 = 80,
	.disp_config_dram_addr = (u32)vout_display_setup_dram
};

static vout_osd_setup_t vout_osd_setup = {
	.cmd_code = VOUT_OSD_SETUP,
	.vout_id = 0,
	.en = 0,
	.src = 0,
	.flip = 0,
	.rescaler_en = 0,
	.premultiplied = 0,
	.global_blend = 0,
	.win_offset_x = 0,
	.win_offset_y = 0,
	.win_width = 0,
	.win_height = 0,
	.rescaler_input_width = 0,
	.rescaler_input_height = 0,
	.osd_buf_dram_addr = 0x00000000,
	.osd_buf_pitch = 0,
	.osd_buf_repeat_field = 0,
	.osd_direct_mode = 0,
	.osd_transparent_color = 0,
	.osd_transparent_color_en = 0,
	.osd_swap_bytes = 0,
	.osd_buf_info_dram_addr = (u32)vout_osd_setup_dram,
	.osd_video_en = 0,
	.osd_video_mode = 0,
	.osd_video_win_offset_x = 0,
	.osd_video_win_offset_y = 0,
	.osd_video_win_width = 0,
	.osd_video_win_height = 0,
	.osd_video_buf_pitch = 0,
	.osd_video_buf_dram_addr = 0x00000000,
	.osd_buf_dram_sync_addr = 0x00000000
};

static vout_osd_clut_setup_t vout_osd_clut_setup = {
	.cmd_code = VOUT_OSD_CLUT_SETUP,
	.vout_id = 0,
	.clut_dram_addr = 0x00000000,
};

/* ==========================================================================*/

static void iav_update_osd_cmd()
{
#ifdef CONFIG_S2LMIRONMAN_DSP_SPLASH
	struct splash_file_info *splash_info;
	u32 splash_clut_addr, splash_image_addr;

	splash_info = (struct splash_file_info *)splash_buf_addr;
	splash_clut_addr = (u32)splash_info + sizeof(struct splash_file_info);
	splash_image_addr = splash_clut_addr + SPLASH_CLUT_SIZE * 4;

	vout_osd_clut_setup.clut_dram_addr = splash_clut_addr;

	vout_osd_setup.en = 1;
	vout_osd_setup.src = 0;
	vout_osd_setup.global_blend = 255;
	vout_osd_setup.osd_transparent_color_en = 1;
	vout_osd_setup.osd_direct_mode = 0;
	vout_osd_setup.win_width = splash_info->width;
	vout_osd_setup.win_height = splash_info->height;
	vout_osd_setup.win_offset_x =
		(vout_video_setup.win_width - splash_info->width) / 2;
	vout_osd_setup.win_offset_y =
		(vout_video_setup.win_height - splash_info->height) / 2;
	vout_osd_setup.osd_buf_pitch = splash_info->width;
	vout_osd_setup.osd_buf_dram_addr = splash_image_addr;
#endif
}

static int iav_boot_td043(void)
{
	rct_set_vo_pll();

	/* Splash logo */
	iav_update_osd_cmd();

	/* vout setup */
	add_dsp_cmd(&vout_reset, sizeof(vout_reset));
	add_dsp_cmd(&vout_mixer_setup, sizeof(vout_mixer_setup));
	add_dsp_cmd(&vout_video_setup, sizeof(vout_video_setup));
	add_dsp_cmd(&vout_display_setup, sizeof(vout_display_setup));
	add_dsp_cmd(&vout_osd_clut_setup, sizeof(vout_osd_clut_setup));
	add_dsp_cmd(&vout_osd_setup, sizeof(vout_osd_setup));

	putstr_debug("iav_boot_td043");
	return 0;
}

