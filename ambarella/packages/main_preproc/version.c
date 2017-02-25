/*
 * version.c
 *
 * Histroy:
 *  2015/04/02 - [Zhaoyang Chen] created file
 *
 * Copyright (c) 2016 Ambarella, Inc.
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
#include <basetypes.h>
#include "iav_ioctl.h"
#include "lib_vproc.h"
/*
 *  1.0.0	2015/04/03
 *         Add MCTF type PM support and synchronize with DZ operation.
 *  1.1.0	2015/07/23
 *         Add support for PM direct Draw and Clear.
 *  1.2.0	2015/08/03
 *         Add support for multiple PM operation.
 *  1.3.0	2015/08/07
 *         Add support for circle PM operation.
 *  1.3.1	2015/09/23
 *         Update MCTF type PM for Blend ISO mode.
 *  1.3.2	2015/11/02
 *         Remove workaround for HDR BPC PM offset.
 *  1.3.3	2015/11/09
 *         Use more acculate calculation for VIN/Main domain transform.
 */

#define MAINPP_LIB_MAJOR 1
#define MAINPP_LIB_MINOR 3
#define MAINPP_LIB_PATCH 3
#define MAINPP_LIB_VERSION ((MAINPP_LIB_MAJOR << 16) | \
                             (MAINPP_LIB_MINOR << 8)  | \
                             MAINPP_LIB_PATCH)

version_t G_mainpp_version = {
	.major = MAINPP_LIB_MAJOR,
	.minor = MAINPP_LIB_MINOR,
	.patch = MAINPP_LIB_PATCH,
	.mod_time = 0x20151109,
	.description = "Ambarella S2L Video Process Library",
};

