/*******************************************************************************
 *  test_motion_profile_det.c
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


#include <string.h>
#include "iav.h"
#include "fb.h"

#include "motion_det_API.h"
#include "image_color.h"

#define 	FB_W	720
#define 	FB_H	480
#define 	EDGE_DIFF_THRESH	2
#define 	CELL_SIZE	8
#define 	DET_BY_FRMAE		2



#define MOTION_PROFILE_DET_CHECK_ERROR()	\
	if (ret < 0) {	\
		printf("%s %d: Error!\n", __func__, __LINE__);	\
		return ret;	\
	}




int main(int argc, char **argv)
{
	int					ret;
	iav_buf_id_t			buf_id		= IAV_BUF_SECOND;
	iav_buf_type_t		buf_type	= IAV_BUF_ME1;
	char * iav_buf;

	static	unsigned		int 		s_pitch;
	static	unsigned		int 		s_width;
	static	unsigned		int		s_height;
	static 	char		*fbmem;
	static 	char		*ybmem;


	ret = init_iav(buf_id, buf_type);
	MOTION_PROFILE_DET_CHECK_ERROR();

	ret = get_iav_buf_size(&s_pitch, &s_width, &s_height);
	MOTION_PROFILE_DET_CHECK_ERROR();

	printf("img pitch %d\n",s_pitch);
	printf("img width %d\n",s_width);
	printf("img height %d\n",s_height);

	ret = open_fb(FB_W, FB_H);
	MOTION_PROFILE_DET_CHECK_ERROR();

	ret = blank_fb();
	MOTION_PROFILE_DET_CHECK_ERROR();


	fbmem = (char *)malloc(FB_W*FB_H);
	if (!fbmem) {
		printf("%s %d: Error!\n", __func__, __LINE__);
		return -1;
	}

	ybmem = (char *)malloc(s_width*s_height);
	if (!ybmem) {
		printf("%s %d: Error!\n", __func__, __LINE__);
		return -1;
	}

	iav_buf = get_iav_buf();
	YUV2Y(iav_buf, ybmem, s_width, s_height, s_pitch);
	if (API_motion_profile_det_initial(ybmem,s_width,s_height,EDGE_DIFF_THRESH,DET_BY_FRMAE)) {
		printf("%s %d: Error!\n", __func__, __LINE__);
		return -1;
	}

	while(1){
		memset(fbmem, 0, FB_W * FB_H);
		iav_buf = get_iav_buf();
		YUV2Y(iav_buf, ybmem, s_width, s_height, s_pitch);

		if(API_motion_profile_det(ybmem,fbmem,FB_W, FB_H, CELL_SIZE)){
			ret = render_frame(fbmem);
			}

	}

	return 0;
}


