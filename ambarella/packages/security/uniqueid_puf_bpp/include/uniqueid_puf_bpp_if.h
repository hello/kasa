/*******************************************************************************
 * uniqueid_puf_bpp_if.h
 *
 * History:
 *	2016/03/28 - [Zhi He] create file
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
 ******************************************************************************/

#ifndef __UNIQUEID_PUF_BPP_IF_H__
#define __UNIQUEID_PUF_BPP_IF_H__

#define DUNIQUEID_MAX_NAME_LENGTH 8
#define DUNIQUEID_LENGTH 64

typedef struct {
    unsigned char id[DUNIQUEID_LENGTH];
} uniqueid_t;

int get_name_from_id(uniqueid_t *id, char *name);
int get_resolution_from_id(uniqueid_t *id, unsigned int *width, unsigned int *height);

int generate_unique_id_from_puf_bpp(uniqueid_t *id, char *name, unsigned int input_buffer_num, unsigned int hot_pixel_number, unsigned int times);
int verify_unique_id_from_puf_bpp(uniqueid_t *id);

//for debug
void debug_print_pixel_list(unsigned int enable);

#endif

