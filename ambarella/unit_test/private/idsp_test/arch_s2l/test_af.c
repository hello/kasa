/*
 * unit_test/private/idsp_test/arch/test_af.c
 *
 * History:
 *    2015/03/02 - [Peter Jiao] created file
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
 */


#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <termios.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sched.h>
#include <signal.h>
#include <iav_ioctl.h>

#include "basetypes.h"
#include "img_adv_struct_arch.h"
#include "img_api_adv_arch.h"


#define NO_ARG	0
#define HAS_ARG	1

static af_mode af_work_mode = -1;

static const char* short_options = "c:m:z:f:d:s:i:a";

static struct option long_options[] = {
	{"af_control_interface", HAS_ARG, 0, 'c'},
	{"af_work_mode", HAS_ARG, 0, 'm'},
	{"af_zoom_index", HAS_ARG, 0, 'z'},
	{"af_focus_index", HAS_ARG, 0, 'f'},
	{"af_internal_debug", HAS_ARG, 0, 'd'},
	{"af_CAF_sensitivity", HAS_ARG, 0, 's'},
	{"af_IAF_interval", HAS_ARG, 0, 'i'},
	{"af_AUX_zoom", NO_ARG, 0, 'a'},
};


#define AF_DEMO_SERV_PORT 		20128
#define AF_DEMO_MAX_CLIENT_NUM 	2

static int CreateAFSock(int *p_Sock, int port)
{
	int opt = 1;
   	*p_Sock = socket(AF_INET, SOCK_STREAM, 0);
	if(*p_Sock < 0) {
  		printf("AF socket create error\n");
       	return -1;
  	}
  	setsockopt(*p_Sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

   	struct sockaddr_in sa;
   	memset(&sa, 0, sizeof(sa));
  	sa.sin_family = AF_INET;
   	sa.sin_addr.s_addr = INADDR_ANY;
   	sa.sin_port = htons(port);
  	if(bind(*p_Sock, (struct sockaddr *)&sa, sizeof(sa)) < 0) {
     	printf("AF sock bind error\n");
      	return -1;
  	}

   	if(listen(*p_Sock, AF_DEMO_MAX_CLIENT_NUM) < 0) {
    	printf("AF sock listen error\n");
      	return -1;
   	}

  	return 0;
}

static int AFReply(int fd, int msg_send)
{
	if(fd<=0) {
    	printf("wrong reply fd\n");
		return -1;
	}

    if(send(fd, (char *)&msg_send, sizeof(int), 0)<0) {
    	printf("send reply error");
        return -1;
    }

    return 0;
}

static int GetAFcmd(int mSeesionfd)
{
	int cmd = 0;
	fd_set fdR;
	FD_ZERO(&fdR);
  	FD_SET(mSeesionfd, &fdR);
	struct timeval timeout = {0, 5000000};

	switch(select(mSeesionfd+1, &fdR, NULL, NULL, &timeout)) {
		case -1: //error
			cmd = 4;
			printf("select error and Exit\n");
			break;
		case 0: //timeout
			cmd = -1;
			printf("time out back and wait for connect again\n");
			break;
		default:
			if(FD_ISSET(mSeesionfd, &fdR)) {
				int msg_recv, msg_reply = 0;
				if(recv(mSeesionfd, (char *)&msg_recv, sizeof(int), 0)<=0) {
					cmd = -1;
					printf("recv back and wait for connect again\n");
					break;
				}
				if(msg_recv>=0 && msg_recv<=255)
					cmd = msg_recv;
				else if(msg_recv>=256 && msg_recv<=259) {
					af_mode mode = msg_recv - 256;
					if((int)mode<CAF || (int)mode>CALIB) {
						printf("Invalid af mode\n");
						msg_reply = -1;
					}
					else if(img_af_set_mode(mode)<0) {
						printf("set af mode fail\n");
						msg_reply = -1;
					}
					if(!msg_reply)
						af_work_mode = mode;
				}
				else if(msg_recv>=301 && msg_recv<=555) {
					u8 time_sec = msg_recv - 300;
					if(img_af_set_interval_time(time_sec)<0) {
						printf("set af time fail\n");
						msg_reply = -1;
					}
				}
				else {
					printf("socket cmd error\n");
					msg_reply = -1;
				}
				AFReply(mSeesionfd, msg_reply);
			}
			break;
	}

	return cmd;
}


static struct termios param_prim;

static int termios_initialize(void)
{
	struct termios param_new;

	if(tcgetattr(STDIN_FILENO, &param_prim)) {
		printf("get termios fail\n");
		return -1;
	}

	param_new = param_prim;
	param_new.c_lflag &= ~(ECHO|ICANON);
	param_new.c_cc[VMIN] = 0;
	param_new.c_cc[VTIME] = 1;
	if(tcsetattr(STDIN_FILENO, TCSANOW, &param_new)) {
		printf("set termios fail\n");
		return 1;
	}

	return 0;
}

static int termios_release(void)
{
	return tcsetattr(STDIN_FILENO, TCSANOW, &param_prim);
}


#define DEFAULT_ZOOM	(80)	//1x ratio, also determine AUX zoom speed

static u32 vin_width, vin_height, main_width, main_height;

static int prepare_aux_zoom(int fd_iav)
{
	struct vindev_video_info video_info;
	struct iav_srcbuf_format main_format;

	memset(&video_info, 0, sizeof(video_info));
	video_info.vsrc_id = 0;
	video_info.info.mode = AMBA_VIDEO_MODE_CURRENT;
	if(ioctl(fd_iav, IAV_IOC_VIN_GET_VIDEOINFO, &video_info) < 0) {
		printf("Get VIDEOINFO fail");
		return -1;
	}
	vin_width = video_info.info.width;
	vin_height = video_info.info.height;

	memset(&main_format, 0, sizeof(main_format));
	main_format.buf_id = IAV_SRCBUF_MN;
	if(ioctl(fd_iav, IAV_IOC_GET_SOURCE_BUFFER_FORMAT, &main_format) < 0) {
		printf("Get BUFFER_FORMAT fail");
		return -1;
	}
	main_width = main_format.size.width;
	main_height = main_format.size.height;

//	printf("v_width=%u,v_height=%u,m_width=%u,m_height=%u\n",vin_width,vin_height,main_width,main_height);

	u16 zoom_width = vin_width * DEFAULT_ZOOM / main_width;
	u16 zoom_height = vin_height * DEFAULT_ZOOM / main_height;
	if(zoom_width<=zoom_height)
		return zoom_width;
	else
		return zoom_height;
}

#define	ROUND_UP(x, n)			( ((x)+(n)-1u) & ~((n)-1u) )
#define	ROUND_DOWN(size, align)	((size) & ~((align) - 1))

static int config_aux_zoom(int fd_iav, u16 ratio)
{
	struct iav_digital_zoom digital_zoom;

	memset(&digital_zoom, 0, sizeof(digital_zoom));
	digital_zoom.buf_id = IAV_SRCBUF_MN;
	digital_zoom.input.width = ROUND_UP(main_width * ratio / DEFAULT_ZOOM, 4);
	digital_zoom.input.height = ROUND_UP(main_height * ratio / DEFAULT_ZOOM, 4);
	digital_zoom.input.x = ROUND_DOWN((vin_width - digital_zoom.input.width) / 2, 2);
	digital_zoom.input.y = ROUND_DOWN((vin_height - digital_zoom.input.height) / 2, 2);

//	printf("i_width=%u,i_height=%u,i_x=%u,i_y=%u\n",digital_zoom.input.width,digital_zoom.input.height,
	//	digital_zoom.input.x,digital_zoom.input.y);

	if(ioctl(fd_iav, IAV_IOC_SET_DIGITAL_ZOOM, &digital_zoom) < 0) {
		printf("Set DIGITAL_ZOOM fail");
		return -1;
	}

	return 0;
}

enum{
	AF_CONTROL_KEYBOARD = 0,
	AF_CONTROL_INTRANET,
	AF_CONTROL_NUMBER,
};

static int init_af_param(int argc, char **argv, u8 *p_interface, u8 *p_aux_zoom)
{
	int ch, param = 0;
	int option_index = 0;

	enum{
		MODE = 0,
		ZOOM,
		FOCUS,
		DEBUG,
		INTERVAL,
		SENSITIVITY,
		CONFIG_NUMBER,
	};
	struct af_management {
		int update;
		int param;
	} af_manage[CONFIG_NUMBER];

	memset(af_manage, 0, sizeof(struct af_management)*CONFIG_NUMBER);

	opterr = 0;
	af_manage[MODE].param = -1;
	while ((ch = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {
		switch(ch) {
		case 'm':
			param = atoi(optarg);
			if(param<0 || param>4)
				goto ERROR_PARAM;
			af_manage[MODE].param = param;
			af_manage[MODE].update = 1;
			break;
		case 'z':
			if(af_manage[MODE].param<CAF || af_manage[MODE].param>CALIB)
				goto ERROR_PARAM;
			param = atoi(optarg);
			if(param<0 || param>255)
				goto ERROR_PARAM;
			af_manage[ZOOM].param = param;
			af_manage[ZOOM].update = 1;
			break;
		case 'f':
			if(af_manage[MODE].param!=MANUAL)
				goto ERROR_PARAM;
			param = atoi(optarg);
			af_manage[FOCUS].param = param;
			af_manage[FOCUS].update = 1;
			break;
		case 'd':
			if(af_manage[MODE].param<CAF || af_manage[MODE].param>CALIB)
				goto ERROR_PARAM;
			param = atoi(optarg);
			af_manage[DEBUG].param = param;
			af_manage[DEBUG].update = 1;
			break;
		case 'i':
			if(af_manage[MODE].param!=IAF)
				goto ERROR_PARAM;
			param = atoi(optarg);
			if(param<1 || param>60)
				goto ERROR_PARAM;
			af_manage[INTERVAL].param = param;
			af_manage[INTERVAL].update = 1;
			break;
		case 's':
			if(af_manage[MODE].param!=CAF)
				goto ERROR_PARAM;
			param = atoi(optarg);
			if(param<2000 || param>16000)
				goto ERROR_PARAM;
			af_manage[SENSITIVITY].param = param;
			af_manage[SENSITIVITY].update = 1;
			break;
		case 'c':
			param = atoi(optarg);
			if(param<0 || param>=AF_CONTROL_NUMBER)
				goto ERROR_PARAM;
			*p_interface = param;
			break;
		case 'a':
			*p_aux_zoom = 1;
			break;
		default:
			printf("unknown option %c\n", ch);
			return -1;
		}
	}

	if(af_manage[MODE].update)
		img_af_set_mode(af_manage[MODE].param);
	if(af_manage[ZOOM].update)
		img_af_set_zoom_idx(af_manage[ZOOM].param);
	if(af_manage[FOCUS].update)
		img_af_set_focus_idx(af_manage[FOCUS].param);
	if(af_manage[DEBUG].update)
		img_af_set_debug(af_manage[DEBUG].param);
	if(af_manage[INTERVAL].update)
		img_af_set_interval_time(af_manage[INTERVAL].param);
	if(af_manage[SENSITIVITY].update)
		img_af_set_sensitivity(af_manage[SENSITIVITY].param);

	af_work_mode = af_manage[MODE].param;

	return 0;

ERROR_PARAM:
	printf("error param input\n");
	return -1;
}

struct hint_s {
	const char *arg;
	const char *str;
};

static const struct hint_s hint[] = {
	{"-c","\tAF Control Interface: Keyboard-0, Intranet-1"},
	{"-m","\tAF Mode: Continuous AF-0, Single AF-1, Interval AF-2, Manual AF-3"},
	{"-z","\tZoom Ratio(0~255): Wide end-0, Tele end-255"},
	{"-f","\tFocus Position in Manual AF Mode"},
	{"-s","\tTrigger Sensitivity in Continuous AF Mode"},
	{"-i","\tInterval Time in Interval AF Mode"},
	{"-a","\tAUX Zoom Enable"}
};

static void usage(void)
{
	int cnt = sizeof(hint)/sizeof(hint[0]);
	int i;
	for(i=0; i<cnt; i++) {
		printf("%s %s\n", hint[i].arg, hint[i].str);
	}
	printf("\nExamples:\n");
	printf("    test_af -c1 -m0 -z128 -s13000 -a\n");
	printf("    test_af -c1 -m1 -z128 -a\n");
	printf("    test_af -c0 -m2 -z128 -i5 -a\n");
	printf("    test_af -c0 -m3 -z128 -f2750 -a\n");
	printf("\n");
}

static void usage_caf(void)
{
	printf("Usage: Zoom In-(i/I), Zoom Out-(o/O), Exit-(Ctrl+d)\n");
}

static void usage_saf(void)
{
	printf("Usage: Zoom In-(i/I), Zoom Out-(o/O), Trigger AF-(Space), Exit-(Ctrl+d)\n");
}

static void usage_iaf(void)
{
	printf("Usage: Zoom In-(i/I), Zoom Out-(o/O), Exit-(Ctrl+d)\n");
}

static void usage_manual(void)
{
	printf("Usage: Zoom In-(i/I), Zoom Out-(o/O), Exit-(Ctrl+d)\n");
	printf("Focus Far-Coarse(f) Fine(F), Focus Near-Coarse(n) Fine(N)\n");
}

static void usage_intranet(void)
{
	printf("Please Open IOS/Android APP to Control, Exit-(Ctrl+c)\n");
}

int main(int argc, char **argv)
{
	enum{
		ZOOM_IDLE = 0,
		ZOOM_IN_PRE,
		ZOOM_IN_OPT,
		ZOOM_IN_AUX,
		ZOOM_OUT_AUX,
		ZOOM_OUT_CON,
		ZOOM_OUT_OPT,
	};
	enum{
		FOCUS_IDLE = 0,
		FOCUS_FAR_PRE,
		FOCUS_NEAR_PRE,
		FOCUS_FAR_ACT,
		FOCUS_NEAR_ACT,
	};

	int mSockfd = -1;
	int mSeesionfd = -1;
	int cmd, af_exit = 0;
	int zoom_state = ZOOM_IDLE;
	int focus_state = FOCUS_IDLE;
	struct sockaddr_in other_addr;
	socklen_t other_size = sizeof(struct sockaddr_in);
	u8 aux_zoom_enable = 0;
	u8 af_ctrl_interface = AF_CONTROL_NUMBER;

	if(argc<=2){
		usage();
		goto EXIT;
	}

	if(init_af_param(argc, argv, &af_ctrl_interface, &aux_zoom_enable)<0)
		goto EXIT;

	if((int)af_work_mode<CAF || (int)af_work_mode>CALIB) {
		printf("Invalid af work mode\n");
		goto EXIT;
	}

	if(af_ctrl_interface>=AF_CONTROL_NUMBER) {
		printf("Invalid af control interface\n");
		goto EXIT;
	}

	int fd_iav = open("/dev/iav", O_RDWR, 0);
	if(fd_iav<0) {
		printf("Open iav fail");
		goto EXIT;
	}

	int ratio_max = prepare_aux_zoom(fd_iav);
	if(ratio_max<0)
		goto DONE;

	u16 curr_ratio = ratio_max;
	if(config_aux_zoom(fd_iav, curr_ratio)<0)
		goto DONE;

	img_af_set_aux_zoom(aux_zoom_enable);

	if(af_ctrl_interface==AF_CONTROL_KEYBOARD) {
		sigset_t set;
		sigemptyset(&set);
		sigfillset(&set);
		sigprocmask(SIG_BLOCK, &set, NULL);

		int ret = termios_initialize();
		if(ret<0)
			goto DONE;
		else if(ret>0)
			goto RELEASE;

		if(af_work_mode==CAF)
			usage_caf();
		else if(af_work_mode==SAF)
			usage_saf();
		else if(af_work_mode==IAF)
			usage_iaf();
		else if(af_work_mode==MANUAL)
			usage_manual();
	}
	else if(af_ctrl_interface==AF_CONTROL_INTRANET) {
		if(CreateAFSock(&mSockfd, AF_DEMO_SERV_PORT)<0)
			goto RELEASE;
		usage_intranet();
	}

	while(1) {
		if(af_ctrl_interface==AF_CONTROL_KEYBOARD)
			cmd = getchar();
		else if(af_ctrl_interface==AF_CONTROL_INTRANET) {
			if(mSeesionfd<0) {
				mSeesionfd = accept(mSockfd, (struct sockaddr *)&other_addr, &other_size);
				continue;
			}
			cmd = GetAFcmd(mSeesionfd);
			if(cmd<0) {
				close(mSeesionfd);
				mSeesionfd = -1;
				continue;
			}
		}
		else {
			printf("No AF Ctrl Interface\n");
			cmd = 4;
		}
	//	printf("%d,%d,%d,%d\n",af_work_mode,zoom_state,focus_state,cmd);
		switch(cmd)
		{
		case 0:
		case EOF:	//Idle: no input
			if(zoom_state==ZOOM_IN_OPT || zoom_state==ZOOM_OUT_OPT
				|| focus_state==FOCUS_FAR_ACT || focus_state==FOCUS_NEAR_ACT)
				img_af_set_brake();
			zoom_state = ZOOM_IDLE;
			focus_state = FOCUS_IDLE;
			break;
		case 4:		// exit: Ctrl + d
			af_exit = 1;
			break;
		case 73:	//zoom_in: I
		case 105:	//zoom_in: i
			if(zoom_state==ZOOM_IDLE)
				zoom_state = ZOOM_IN_PRE;
			else if(zoom_state==ZOOM_IN_PRE) {
				img_af_set_zoom_idx(255);
				zoom_state = ZOOM_IN_OPT;
			}
			else if(zoom_state==ZOOM_IN_OPT) {
				if(aux_zoom_enable && !img_af_get_work_state(255))
					zoom_state = ZOOM_IN_AUX;
			}
			else if(zoom_state==ZOOM_IN_AUX) {
				curr_ratio--;
				if(curr_ratio<DEFAULT_ZOOM)
					curr_ratio = DEFAULT_ZOOM;
				config_aux_zoom(fd_iav, curr_ratio);
			}
			break;
		case 79:	//zoom_out: O
		case 111:	//zoom_out: o
			if(zoom_state==ZOOM_IDLE)
				zoom_state = ZOOM_OUT_AUX;
			else if(zoom_state==ZOOM_OUT_AUX) {
				curr_ratio++;
				if(curr_ratio>ratio_max) {
					curr_ratio = ratio_max;
					zoom_state = ZOOM_OUT_CON;
				}
				config_aux_zoom(fd_iav, curr_ratio);
			}
			else if(zoom_state==ZOOM_OUT_CON) {
				img_af_set_zoom_idx(0);
				zoom_state = ZOOM_OUT_OPT;
			}
			break;
		case 32:	//one_push: space
			if(af_work_mode==SAF && zoom_state==ZOOM_IDLE)
				img_af_set_one_push();
			break;
		case 70:	//focus_far(fine): F
			if(af_work_mode!=MANUAL)
				break;
			img_af_set_focus_offset(1);
			break;
		case 102:	//focus_far(coarse): f
			if(af_work_mode!=MANUAL)
				break;
			if(focus_state==FOCUS_IDLE)
				focus_state = FOCUS_FAR_PRE;
			else if(focus_state==FOCUS_FAR_PRE) {
				img_af_set_focus_idx((s32)0x7FFFFFFF);
				focus_state = FOCUS_FAR_ACT;
			}
			break;
		case 78:	//focus_near(fine): N
			if(af_work_mode!=MANUAL)
				break;
			img_af_set_focus_offset(-1);
			break;
		case 110:	//focus_near(coarse): n
			if(af_work_mode!=MANUAL)
				break;
			if(focus_state==FOCUS_IDLE)
				focus_state = FOCUS_NEAR_PRE;
			else if(focus_state==FOCUS_NEAR_PRE) {
				img_af_set_focus_idx((s32)0x80000000);
				focus_state = FOCUS_NEAR_ACT;
			}
			break;
		default:	//other input not support
			break;
		}

		if(af_exit)
			break;
	}

RELEASE:
	if(af_ctrl_interface==AF_CONTROL_KEYBOARD) {
		while(termios_release())
			usleep(10000);
	}
	else if(af_ctrl_interface==AF_CONTROL_INTRANET) {
		if(mSeesionfd>=0)
			close(mSeesionfd);
		if(mSockfd>=0)
			close(mSockfd);
	}
DONE:
	close(fd_iav);
EXIT:
	return 0;
}

