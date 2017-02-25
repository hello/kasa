/*
 *
 * History:
 *    2013/08/07 - [Zhenwu Xue] Create
 *
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


#ifndef __IAV_H__
#define __IAV_H__

typedef enum {
	IAV_BUF_MAIN = 0,
	IAV_BUF_SECOND = 1,
	IAV_BUF_THIRD = 2,
	IAV_BUF_FOURTH = 3,
	IAV_BUF_ID_MAX,
} iav_buf_id_t;

typedef enum {
	IAV_BUF_YUV = 0,
	IAV_BUF_ME0 = 1,
	IAV_BUF_ME1 = 2,
	IAV_BUF_TYPE_MAX,
} iav_buf_type_t;

#ifdef __cplusplus
extern "C" {
#endif

int init_iav(iav_buf_id_t buf_id, iav_buf_type_t buf_type);
int get_iav_buf_size(unsigned int *p, unsigned int *w, unsigned int *h);
char * get_iav_buf(void);
int exit_iav(void);

#ifdef __cplusplus
}
#endif

#endif
