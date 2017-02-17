/**
 * amboot/include/dsp/dspfw.h
 *
 * Author: Cao Rongrong <rrcao@ambarella.com>
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

#define UCODE_FILE_NAME_SIZE		(24)
#define DSPFW_IMG_MAGIC			(0x43525231)
#define MAX_UCODE_FILE_NUM		(8)
#define MAX_LOGO_FILE_NUM		(4)
#define SPLASH_CLUT_SIZE		(256)

struct ucode_file_info { /* 4 + 4 + 24 = 32 */
	unsigned int			offset;
	unsigned int			size;
	char				name[UCODE_FILE_NAME_SIZE];
}__attribute__((packed));

struct splash_file_info { /* 32 */
	unsigned int			offset;
	unsigned int			size;
	short				width;
	short				height;
	char				rev[32- (4 * 2) - (2 * 2)];
}__attribute__((packed));

struct dspfw_header { /* 4 + 4 + 4 + 4 + (32 * 8) + (32 * 4) + 112 = 512 */
	unsigned int			magic;
	unsigned int			size; /* totally size including the header */
	unsigned short			total_dsp;
	unsigned short			total_logo;
	struct ucode_file_info		ucode[MAX_UCODE_FILE_NUM];
	struct splash_file_info		logo[MAX_LOGO_FILE_NUM];
	unsigned char			rev[112];
}__attribute__((packed));
