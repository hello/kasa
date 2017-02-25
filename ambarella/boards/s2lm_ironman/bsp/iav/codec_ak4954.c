/**
 * boards/hawthorn/bsp/iav/codec_ak4954.c
 *
 * History:
 *    2014/11/18 - [Cao Rongrong] created file
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

static int ak4954_init(void)
{
	/* ak4954 cannot work with 400K I2C */
	idc_bld_init(IDC_MASTER1, 200000);

	idc_bld_write_8_8(IDC_MASTER1, 0x24, 0x00, 0x00);
	idc_bld_write_8_8(IDC_MASTER1, 0x24, 0x05, 0x03);
	idc_bld_write_8_8(IDC_MASTER1, 0x24, 0x06, 0x0a);
	idc_bld_write_8_8(IDC_MASTER1, 0x24, 0x00, 0x40);
	idc_bld_write_8_8(IDC_MASTER1, 0x24, 0x01, 0x04);
	idc_bld_write_8_8(IDC_MASTER1, 0x24, 0x02, 0x0a);
	idc_bld_write_8_8(IDC_MASTER1, 0x24, 0x03, 0x05);
	idc_bld_write_8_8(IDC_MASTER1, 0x24, 0x09, 0x09);
	idc_bld_write_8_8(IDC_MASTER1, 0x24, 0x0a, 0x4c);
	idc_bld_write_8_8(IDC_MASTER1, 0x24, 0x0b, 0x0d);
	idc_bld_write_8_8(IDC_MASTER1, 0x24, 0x1b, 0x05);
	/* idc_bld_write_8_8(IDC_MASTER1, 0x24, 0x1d, 0x03); // removable */
	idc_bld_write_8_8(IDC_MASTER1, 0x24, 0x00, 0xc3);

	return 0;
}

