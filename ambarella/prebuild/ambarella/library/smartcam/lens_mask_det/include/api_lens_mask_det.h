/*******************************************************************************
 *  api_lens_mask_det.h
 *
 * History:
 *    2016/11/6 - [Richard Ren] Create
 *
 * Copyright (c) 2016 Ambarella, Inc.
 *
 * This file and its contents ( "Software" ) are protected by intellectual
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


#ifndef	_API_LENS_MASK_DET_H
#define  	_API_LENS_MASK_DET_H


#include <stdio.h>
#include <stdlib.h>
#include <math.h>

typedef 	enum _LENS_MASK_DET_STATE{
	e_mask_yes=0,
	e_mask_no,
	e_mask_unknown
}e_lens_mask_det_state;


extern	int 	API_lens_mask_det_initial(const unsigned int im_w,const unsigned int im_h, const double  det_sen_k, const unsigned int det_frames);
//extern	int 		API_lens_full_mask_alarm(unsigned char* y_buf);
extern	int 		API_lens_partially_mask_alarm(unsigned char* y_buf,int is_print_values);
//extern	e_lens_mask_det_state  API_lens_full_mask_state_det(unsigned char* y_buf);
extern	e_lens_mask_det_state  API_lens_partial_mask_state_det(unsigned char* y_buf,int is_print_values);
extern	int iso_noise_filt(unsigned char* y_buf, unsigned char* f_buf,int f_w,int f_h);
#endif

