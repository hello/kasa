/*******************************************************************************
 *  image_color.h
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

#ifndef _IMAGE_COLOR_H
#define _IMAGE_COLOR_H




#include <stdio.h>
#include <stdlib.h>
#include <math.h>





extern	int 		YUV2Y(char* yuvbuf, char* ybuf, const unsigned int pic_w,const  unsigned int pic_h, const unsigned int pic_pitch);
extern	int 		YUV2RGB(const int *y, const int*u, const int*v, const int im_h, const int im_w, int *r, int *g, int *b);
extern	void 	pYUV2RGB(const int y, const int u, const int v,  int *r, int *g, int *b);
extern	int 		RGB2HSV(const int *r, const int*g, const int*b, const int im_h, const int im_w, double *h, double *s, double *v);
extern	void 	pRGB2HSV(const int r, const int g, const int b, double *h, double *s, double *v);
extern	int 		YUV2HSV(const int *y, const int*u, const int*v, const int im_h, const int im_w, double *hh, double *ss, double *vv);
extern	void 	pYUV2HSV(const int y, const int u, const int v, double *hh, double *ss, double *vv);
#endif

