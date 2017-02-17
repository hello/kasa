/**
 * bld/piggy_dsp.S
 *
 * History:
 *    2005/02/27 - [Charles Chiou] created file
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


#include <amboot.h>
#include <fio/firmfl.h>

piggy_dsp:

#ifdef VERSION_INFO
#include xstr(VERSION_INFO)
#endif

.text

#ifdef INCBIN

#ifdef CRC32
.word	CRC32
#else
#error "CRC32 not defined!"
#endif

#ifdef FIRM_VER_NUM
.word	FIRM_VER_NUM
#else
.word	0x0
#endif

#ifdef FIRM_VER_DATE
.word	FIRM_VER_DATE
#else
.word	0x0
#endif

#ifdef IMG_LEN
.word	IMG_LEN
#else
.word	0x0
#endif

#ifdef FIRM_MEM_ADDR
.word	FIRM_MEM_ADDR
#else
.word	0x0
#endif

#ifdef FIRM_FLAG
.word	FIRM_FLAG
#else
.word	0x0
#endif

.word	PARTHD_MAGIC

.rept 57
.word	0x0
.endr

.incbin xstr(INCBIN)

#endif

.end

