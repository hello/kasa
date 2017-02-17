/**
 * bld/splash/s2l/dsp.c
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


#include <ambhw/cache.h>
#include <dsp/dsp.h>

#define S2L_MEMD_OFFSET			(0x150000)
#define S2L_CODE_OFFSET			(0x160000)
#define S2L_MEMD_BASE			(DBGBUS_BASE + S2L_MEMD_OFFSET)
#define S2L_CODE_BASE			(DBGBUS_BASE + S2L_CODE_OFFSET)

#define S2L_DSP_DRAM_MAIN_OFFSET	(0x0008)
#define S2L_DSP_DRAM_SUB0_OFFSET	(0x0008)
#define S2L_DSP_CONFIG_MAIN_OFFSET	(0x0000)
#define S2L_DSP_CONFIG_SUB0_OFFSET	(0x0000)
#define S2L_DSP_DRAM_MAIN_REG		(S2L_CODE_BASE + S2L_DSP_DRAM_MAIN_OFFSET)
#define S2L_DSP_DRAM_SUB0_REG		(S2L_MEMD_BASE + S2L_DSP_DRAM_SUB0_OFFSET)
#define S2L_DSP_CONFIG_MAIN_REG		(S2L_CODE_BASE + S2L_DSP_CONFIG_MAIN_OFFSET)
#define S2L_DSP_CONFIG_SUB0_REG		(S2L_MEMD_BASE + S2L_DSP_CONFIG_SUB0_OFFSET)

/*===========================================================================*/

/* defined in dsp_bo.o_shipped */
extern int bopt_sync(void *arg0, void *arg1, void *arg2, void *arg3);

/*===========================================================================*/
void vin_phy_init(int interface_type)
{
	switch (interface_type) {
	case SENSOR_PARALLEL_LVCMOS:
		writel(RCT_REG(0x474), 0x3FFFFFF);
		putstr_debug("SENSOR_PARALLEL_LVCMOS");
		break;
	case SENSOR_MIPI:
		writel(RCT_REG(0x478), 0x14403);
		rct_timer_dly_ms(10);
		writel(RCT_REG(0x478), 0x14402);
		writel(RCT_REG(0x47C), 0x12B4B22F);
		writel(RCT_REG(0x480), 0x101F); // clk_term_ctrl: 7
		putstr_debug("SENSOR_MIPI");
		break;
	case SENSOR_SERIAL_LVDS:
		writel(RCT_REG(0x474), 0x0);
		writel(RCT_REG(0x478), 0x145C1);
		rct_timer_dly_ms(10);
		writel(RCT_REG(0x478), 0x145C0);
		putstr_debug("SENSOR_SERIAL_LVDS");
		break;
	default:
		putstr_debug("Unkown sensor interface type");
		break;
	}
}

static int dsp_init_data(void)
{
	dsp_init_data_t *init_data = NULL;
	vdsp_info_t *vdsp_info = NULL;
	default_enc_binary_data_t *enc_binary_data = NULL;
	DSP_HEADER_CMD *cmd_hdr = NULL;
	u32 *ucode_init_data_ptr = NULL;

	/* initialize struct dsp_init_data */
	init_data = (dsp_init_data_t *)DSP_INIT_DATA_START;
	memset(init_data, 0, sizeof(dsp_init_data_t));

	/* setup default binary data pointer */
	init_data->default_binary_data_ptr = (u32 *)UCODE_DEFAULT_BINARY_START;
	/* setup cmd/msg memory for DSP-ARM */
	init_data->cmd_data_ptr = (u32 *)DSP_CMD_BUF_START;
	init_data->cmd_data_size = DSP_CMD_BUF_SIZE;
	init_data->result_queue_ptr = (u32 *)DSP_MSG_BUF_START;
	init_data->result_queue_size = DSP_MSG_BUF_SIZE;
	init_data->default_config_ptr = (u32 *)DSP_DEF_CMD_BUF_START;
	init_data->default_config_size = DSP_DEF_CMD_BUF_SIZE;
	/* setup buffer for dsp running */
	init_data->DSP_buf_ptr = (u32 *)DSP_BUFFER_START;
	init_data->DSP_buf_size = DSP_BUFFER_SIZE;
	init_data->dsp_log_size = DSP_LOG_SIZE;
	/* misc info */
	init_data->chip_id_ptr = (u32 *)UCODE_CHIP_ID_START;
	vdsp_info = (vdsp_info_t *)(UCODE_CHIP_ID_START + sizeof(u32));
	vdsp_info->dsp_cmd_rx = 1;
	vdsp_info->dsp_msg_tx = 1;
	vdsp_info->num_dsp_msgs = 0;
	init_data->vdsp_info_ptr = (u32 *)vdsp_info;
	init_data->chip_id_checksum = 0;
	init_data->dsp_log_buf_ptr = DSP_LOG_START;
	init_data->prev_cmd_seq_num = 0;
	init_data->cmdmsg_protocol_version = MPV_FIXED_BUFFER;

	enc_binary_data = (default_enc_binary_data_t *)UCODE_DEFAULT_BINARY_START;
	enc_binary_data->dram_addr_cabac_out_bit_buffer = DSP_BSB_START;
	enc_binary_data->dram_addr_jpeg_out_bit_buffer = DSP_BSB_START;
	enc_binary_data->h264_out_bit_buf_sz = DSP_BSB_SIZE;
	enc_binary_data->jpeg_out_bit_buf_sz = DSP_BSB_SIZE;
	enc_binary_data->dram_addr_bit_info_buffer = DSP_BSH_START;
	enc_binary_data->bit_info_buf_sz = DSP_BSH_SIZE;
	/* used for valid bsh data check */
	memset((void *)DSP_BSH_START, 0, DSP_BSH_SIZE);

	/* used for sync fastboot data from amboot to linux/iav */
	memset((void *)DSP_FASTDATA_START, DSP_FASTDATA_INVALID, DSP_FASTDATA_SIZE);

	/* TODO: In fastboot case, load "default_mctf.bin" will cause the first h264 frame corrupt
	enc_binary_data->dram_addr_mctf_cfg_buffer = UCODE_DEFAULT_MCTF_START;
	enc_binary_data->mctf_cfg_buf_sz = DSP_MCTF_SIZE;//528(0x210)
	*/
	cmd_hdr = (DSP_HEADER_CMD *)DSP_CMD_BUF_START;
	cmd_hdr->cmd_seq_num = 1;
	cmd_hdr->num_cmds = 0;

	cmd_hdr = (DSP_HEADER_CMD *)DSP_DEF_CMD_BUF_START;
	cmd_hdr->cmd_seq_num = 1;
	cmd_hdr->num_cmds = 0;

	ucode_init_data_ptr = (u32 *)UCODE_DSP_INIT_DATA_PTR;
	*ucode_init_data_ptr = DSP_INIT_DATA_START;
#ifdef AMBOOT_DSP_LOG_CAPTURE
	memset((void *)DSP_LOG_START, 0, DSP_LOG_SIZE);
#endif
	bopt_sync(init_data, enc_binary_data, NULL, NULL);

	return 0;
}

int dsp_boot(void)
{

	/* reset analog/digital mipi phy */
	writel(DBGBUS_BASE + 0x11801c, 0x30000);
	// rct_timer_dly_ms(5);
	writel(DBGBUS_BASE + 0x11801c, 0x0);
	/* Reset setion 1 */
	writel(DBGBUS_BASE + 0x11801c, 0x2);
	// rct_timer_dly_ms(5);
	writel(DBGBUS_BASE + 0x11801c, 0x0);

	_drain_write_buffer();
	_clean_flush_all_cache();

	writel(S2L_DSP_DRAM_MAIN_REG, UCODE_ORCCODE_START);
	writel(S2L_DSP_DRAM_SUB0_REG, UCODE_ORCME_START);

	writel(S2L_DSP_CONFIG_SUB0_REG, 0xF);
	writel(S2L_DSP_CONFIG_MAIN_REG, 0xF);

	putstr_debug("dsp_boot");
	return 0;
}

