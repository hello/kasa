/**
 * bld/dsp/dsp.c
 *
 * History:
 *    2014/08/04 - [Cao Rongrong] created file
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

#include <basedef.h>
#include <bldfunc.h>
#include <ambhw/rct.h>
#include <dsp/dsp.h>

#if (CHIP_REV == S2L)
#include "s2l/dsp_arch.c"
#else
#error "Not implemented yet"
#endif

int add_dsp_cmd(void *cmd, u32 size)
{
	DSP_HEADER_CMD *cmd_hdr;
	u32 cmd_ptr;

	dsp_print_cmd(cmd, size);

	cmd_hdr = (DSP_HEADER_CMD *)DSP_DEF_CMD_BUF_START;

	cmd_ptr = DSP_DEF_CMD_BUF_START + sizeof(DSP_HEADER_CMD) +
			cmd_hdr->num_cmds * DSP_CMD_SIZE;
	if ((cmd_ptr + DSP_CMD_SIZE) >=
		(DSP_DEF_CMD_BUF_START + DSP_DEF_CMD_BUF_SIZE)) {
		putstr("cmd buffer is too small!!!\n");
		BUG_ON(1);
		return -1;
	}

	memcpy((u8 *)cmd_ptr, cmd, size);
	memset((u8 *)cmd_ptr + size, 0, DSP_CMD_SIZE - size);

	cmd_hdr->num_cmds++;

	return 0;
}

int dsp_get_ucode_by_name(struct dspfw_header *hdr, const char *name,
		unsigned int *addr, unsigned int *size)
{
	int i;

	for (i = 0; i < hdr->total_dsp; i++) {
		if (strcmp(name, hdr->ucode[i].name))
			continue;

		if (addr)
			*addr = (u32)hdr + hdr->ucode[i].offset;
		if (size)
			*size = hdr->ucode[i].size;
		return 0;
	}

	return -1;
}

static int dsp_get_logo_by_id(struct dspfw_header *hdr, int id,
		unsigned int *addr, unsigned int *size)
{
	if (id >= hdr->total_logo) {
		if (hdr->total_logo > 0)
			putstrdec("dsp_get_logo_by_id: Invalid id! ", hdr->total_logo);
		return -1;
	}

	if (hdr->logo[id].offset == 0 || hdr->logo[id].size == 0) {
		putstr("dsp_get_logo_by_id: no logo file\r\n");
		return -1;
	}

	if (hdr->logo[id].size > SIZE_1MB) {
		putstr("dsp_get_logo_by_id: the logo size if too large\r\n");
		return -1;
	}

	if (addr)
		*addr = (u32)hdr + hdr->logo[id].offset;
	if (size)
		*size = hdr->logo[id].size;

	return 0;
}

int dsp_init(void)
{
	struct dspfw_header *hdr;
	flpart_table_t ptb;
	u32 addr, size = 0;
	u8 *audio_play;
	int rval;

	/* sanity check */
	if (IDSP_RAM_START + DSP_BSB_SIZE + DSP_IAVRSVD_SIZE
		+ DSP_FASTAUDIO_SIZE
		+ DSP_LOG_SIZE + DSP_UCODE_SIZE > DRAM_SIZE
		|| DSP_BUFFER_SIZE < 64 * 1024 * 1024) {
		putstr("dsp_init: Invalid buffer size for DSP!\r\n");
		return -1;
	}

	rval = flprog_get_part_table(&ptb);
	if (rval < 0) {
		putstr("dsp_init: PTB load error!\r\n");
		return rval;
	}

	rval = bld_loader_load_partition(PART_DSP, &ptb, 0);
	if (rval < 0) {
		putstr("DSP: Load(PART_DSP) fail!\r\n");
		return rval;
	}

	hdr = (struct dspfw_header *)ptb.part[PART_DSP].mem_addr;
	if (hdr->magic != DSPFW_IMG_MAGIC
		|| hdr->size != ptb.part[PART_DSP].img_len) {
		putstr("DSP: Invalid(PART_DSP)!\r\n");
		return -1;
	}

	rval = dsp_get_ucode_by_name(hdr, "orccode.bin", &addr, &size);
	if (rval >= 0)
		memcpy((void *)UCODE_ORCCODE_START, (void *)addr, size);

	rval = dsp_get_ucode_by_name(hdr, "orcme.bin", &addr, &size);
	if (rval >= 0)
		memcpy((void *)UCODE_ORCME_START, (void *)addr, size);

	rval = dsp_get_ucode_by_name(hdr, "default_binary.bin", &addr, &size);
	if (rval >= 0)
		memcpy((void *)UCODE_DEFAULT_BINARY_START, (void *)addr, size);

	/* TODO: In fastboot case, load "default_mctf.bin" will cause the first h264 frame corrupt
	rval = dsp_get_ucode_by_name(hdr, "default_mctf.bin", &addr, &size);
	if (rval >= 0)
		memcpy((void *)UCODE_DEFAULT_MCTF_START, (void *)addr, size);
	*/

	rval = dsp_get_logo_by_id(hdr, ptb.dev.splash_id, &addr, &size);
	if (rval >= 0) {
		memcpy(splash_buf_addr, &hdr->logo[ptb.dev.splash_id],
				sizeof(struct splash_file_info));
		memcpy(splash_buf_addr + sizeof(struct splash_file_info),
				(void *)addr, size);
	}

	rval = dsp_get_ucode_by_name(hdr, "start.bin", &addr, &size);
	if (rval >= 0) {
		size = min(size, AUDIO_PLAY_MAX_SIZE);

		audio_play = malloc(size);
		if (audio_play == NULL) {
			putstr("audio_play malloc failed!\n");
			return -1;
		}
		memcpy(audio_play, (void *)addr, size);
		audio_set_play_size((u32)audio_play, size);
	}

	rval = dsp_init_data();

	return 0;
}

