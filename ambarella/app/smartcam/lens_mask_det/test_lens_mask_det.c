/*******************************************************************************
 *  test_lens_mask_det.c
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

#include <sys/time.h>
#include <string.h>
#include "iav.h"
#include "fb.h"

#include "api_lens_mask_det.h"
#include "image_color.h"

#define 	DET_BY_FRMAE		5			//set for the detection times per frames
#define 	DET_SNR_DEG	7				//set for the detection sensitivity, the larger value means the higher sensitivity
#define 	DET_PRINT_VALUE	1			//zero value means printing disabled


#define LENS_MASK_DET_CHECK_ERROR()	\
	if (ret < 0) {	\
		printf("%s %d: Error!\n", __func__, __LINE__);	\
		return ret;	\
	}


int main(int argc, char **argv)
{
	//e_lens_mask_det_state this_det_state;
	int					ret;
	iav_buf_id_t			buf_id		= IAV_BUF_MAIN;
	iav_buf_type_t		buf_type	= IAV_BUF_YUV;
	char * iav_buf;
	//struct timeval			tv_begin, tv_end;

	static	unsigned		int 		s_pitch;
	static	unsigned		int 		s_width;
	static	unsigned		int		s_height;
	static 	char		*ybmem;
	static 	char		*ybmem_filter;

	//ret = parse_parameters(argc, argv);

	ret = init_iav(buf_id, buf_type);
	LENS_MASK_DET_CHECK_ERROR();

	ret = get_iav_buf_size(&s_pitch, &s_width, &s_height);
	LENS_MASK_DET_CHECK_ERROR();

	printf("img pitch %d\n",s_pitch);
	printf("img width %d\n",s_width);
	printf("img height %d\n",s_height);

	if (API_lens_mask_det_initial(s_width,s_height, DET_SNR_DEG, DET_BY_FRMAE)) {
		printf("%s %d: Error!\n", __func__, __LINE__);
		return -1;
	}

	ybmem = (char *)malloc(s_width*s_height);
	ybmem_filter = (char *)malloc(s_width*s_height);
	if (!ybmem) {
		printf("%s %d: Error!\n", __func__, __LINE__);
		return -1;
	}

	while(1){
		iav_buf = get_iav_buf();
		YUV2Y(iav_buf, ybmem, s_width, s_height, s_pitch);
		//gettimeofday(&tv_begin, NULL);

		//this_det_state=API_lens_partial_mask_state_det(ybmem);
		//if((int)this_det_state<2){    //skip the unknow state(value=2), means the detection in progress
		//	printf("this_det_state=%d\n",this_det_state);
		//	}
		iso_noise_filt(ybmem, ybmem_filter,3,3);
		ret=API_lens_partially_mask_alarm(ybmem_filter,DET_PRINT_VALUE);

		if(ret==1){
			printf("lens masked ##############################################################\n");
		}
		if(ret==0){
			printf("lens not masked-----------------------------------------------------------------------\n");
		}
	}
	return 0;
}


