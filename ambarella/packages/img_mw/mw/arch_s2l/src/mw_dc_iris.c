/*
 * mw_dc_iris.c
 *
 * History:
 * 2014/09/17 - [Bin Wang] Created this file
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
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <assert.h>
#include <basetypes.h>

#include "img_adv_struct_arch.h"
#include "img_api_adv_arch.h"

#include "mw_struct.h"
#include "mw_defines.h"

#include "img_dev.h"

//#define USE_FUZZY_PID_ALGO
#define IGNORE_FULL_OPEN_CHECK -10
#define FULL_OPEN_CHECK 20
#define OPEN_DUTY 90
#define MAX_DUTY 255
#define MIN_DUTY 1
#define LUMA_DIFF_RANGE 1
#define Kp0 2500
#define Ki0 2
#define Kd0 5000
#define Kac 5000 //K accuracy class
typedef enum {
	MW_IRIS_IDLE = 0,
	MW_IRIS_FLUCTUATE,
	MW_IRIS_STATE_NUM,
} MW_IRIS_STATE;

typedef enum {
	NB,
	NS,
	Z0,
	PS,
	PB,
} Param_Domain;

typedef struct {
	int enable;
	int p_data;
	int i_data;
	int d_data;
} PID_Data;

typedef struct {
	int NB_PID;
	int NS_PID;
	int Z0_PID;
	int PS_PID;
	int PB_PID;
} Param_Domain_PID;

typedef struct {
	int T1;
	int T2;
	int T3;
	int T4;
} Param_Domain_Threshold;

PID_Data G_mw_pid_data = {
	.enable = 0,
	.p_data = 0,
	.i_data = 0,
	.d_data = 0,
};

mw_dc_iris_pid_coef G_mw_dc_iris_pid_coef = {
	.p_coef = 3000,
	.i_coef = 1,
	.d_coef = 3000,
}; //PID coefficients, you can tune it if needed.

Param_Domain_PID Kp_Domain = {
	.NB_PID = -2,
	.NS_PID = -1,
	.Z0_PID = 0,
	.PS_PID = 1,
	.PB_PID = 2,
};

Param_Domain_PID Ki_Domain = {
	.NB_PID = -10,
	.NS_PID = -5,
	.Z0_PID = 0,
	.PS_PID = 5,
	.PB_PID = 10,
};

Param_Domain_PID Kd_Domain = {
	.NB_PID = -2,
	.NS_PID = -1,
	.Z0_PID = 0,
	.PS_PID = 1,
	.PB_PID = 2,
};

Param_Domain_Threshold measure_Domain = {
	.T1 = -100,
	.T2 = -10,
	.T3 = 10,
	.T4 = 800,
};

Param_Domain_Threshold d_measure_Domain = {
	.T1 = -50,
	.T2 = -5,
	.T3 = 50,
	.T4 = 200,
};

int duty = 99, duty_backup = 98;
int wait_time = 0;
int luma_data[2] = {0, 0};
/*************************************************
 *
 *		Static Functions, for file internal used
 *
 *************************************************/
#ifdef USE_FUZZY_PID_ALGO
//function choose_domain and Update_K can be applied to fuzzy PID control algo.
static Param_Domain choose_domain(int data, Param_Domain_Threshold data_Domain)
{
	if (data > data_Domain.T4) {
		return PB;
	} else if (data > data_Domain.T3) {
		return PS;
	} else if (data > data_Domain.T2) {
		return Z0;
	} else if (data > data_Domain.T1) {
		return NS;
	} else {
		return NB;
	}
}

static void Update_K(int data, int d_data)
{
	Param_Domain measure = choose_domain(data, measure_Domain);
	Param_Domain d_measure = choose_domain(d_data, d_measure_Domain);
	switch(measure) {
	case NB:
		switch(d_measure) {
		case NB:
			G_mw_dc_iris_pid_coef.p_coef = Kp_Domain.PB_PID;
			G_mw_dc_iris_pid_coef.i_coef = Ki_Domain.NB_PID;
			G_mw_dc_iris_pid_coef.d_coef = Kd_Domain.Z0_PID;
			break;
		case NS:
			G_mw_dc_iris_pid_coef.p_coef = Kp_Domain.PB_PID;
			G_mw_dc_iris_pid_coef.i_coef = Ki_Domain.NB_PID;
			G_mw_dc_iris_pid_coef.d_coef = Kd_Domain.Z0_PID;
			break;
		case Z0:
			G_mw_dc_iris_pid_coef.p_coef = Kp_Domain.PB_PID;
			G_mw_dc_iris_pid_coef.i_coef = Ki_Domain.NB_PID;
			G_mw_dc_iris_pid_coef.d_coef = Kd_Domain.Z0_PID;
			break;
		case PS:
			G_mw_dc_iris_pid_coef.p_coef = Kp_Domain.PS_PID;
			G_mw_dc_iris_pid_coef.i_coef = Ki_Domain.NB_PID;
			G_mw_dc_iris_pid_coef.d_coef = Kd_Domain.Z0_PID;
			break;
		case PB:
			G_mw_dc_iris_pid_coef.p_coef = Kp_Domain.Z0_PID;
			G_mw_dc_iris_pid_coef.i_coef = Ki_Domain.NB_PID;
			G_mw_dc_iris_pid_coef.d_coef = Kd_Domain.Z0_PID;
			break;
		default:
			G_mw_dc_iris_pid_coef.p_coef = Kp_Domain.Z0_PID;
			G_mw_dc_iris_pid_coef.i_coef = Ki_Domain.Z0_PID;
			G_mw_dc_iris_pid_coef.d_coef = Kd_Domain.Z0_PID;
			break;
		}
		break;
	case NS:
		switch(d_measure) {
		case NB:
			G_mw_dc_iris_pid_coef.p_coef = Kp_Domain.PB_PID;
			G_mw_dc_iris_pid_coef.i_coef = Ki_Domain.NB_PID;
			G_mw_dc_iris_pid_coef.d_coef = Kd_Domain.PB_PID;
			break;
		case NS:
			G_mw_dc_iris_pid_coef.p_coef = Kp_Domain.PB_PID;
			G_mw_dc_iris_pid_coef.i_coef = Ki_Domain.NS_PID;
			G_mw_dc_iris_pid_coef.d_coef = Kd_Domain.PB_PID;
			break;
		case Z0:
			G_mw_dc_iris_pid_coef.p_coef = Kp_Domain.PB_PID;
			G_mw_dc_iris_pid_coef.i_coef = Ki_Domain.NS_PID;
			G_mw_dc_iris_pid_coef.d_coef = Kd_Domain.PS_PID;
			break;
		case PS:
			G_mw_dc_iris_pid_coef.p_coef = Kp_Domain.PS_PID;
			G_mw_dc_iris_pid_coef.i_coef = Ki_Domain.NS_PID;
			G_mw_dc_iris_pid_coef.d_coef = Kd_Domain.PB_PID;
			break;
		case PB:
			G_mw_dc_iris_pid_coef.p_coef = Kp_Domain.PS_PID;
			G_mw_dc_iris_pid_coef.i_coef = Ki_Domain.NS_PID;
			G_mw_dc_iris_pid_coef.d_coef = Kd_Domain.PB_PID;
			break;
		default:
			G_mw_dc_iris_pid_coef.p_coef = Kp_Domain.Z0_PID;
			G_mw_dc_iris_pid_coef.i_coef = Ki_Domain.Z0_PID;
			G_mw_dc_iris_pid_coef.d_coef = Kd_Domain.Z0_PID;
			break;
		}
		break;
	case Z0:
		switch(d_measure) {
		case NB:
			G_mw_dc_iris_pid_coef.p_coef = Kp_Domain.PB_PID;
			G_mw_dc_iris_pid_coef.i_coef = Ki_Domain.NS_PID;
			G_mw_dc_iris_pid_coef.d_coef = Kd_Domain.PB_PID;
			break;
		case NS:
			G_mw_dc_iris_pid_coef.p_coef = Kp_Domain.PB_PID;
			G_mw_dc_iris_pid_coef.i_coef = Ki_Domain.NS_PID;
			G_mw_dc_iris_pid_coef.d_coef = Kd_Domain.PB_PID;
			break;
		case Z0:
			G_mw_dc_iris_pid_coef.p_coef = Kp_Domain.PB_PID;
			G_mw_dc_iris_pid_coef.i_coef = Ki_Domain.NS_PID;
			G_mw_dc_iris_pid_coef.d_coef = Kd_Domain.PB_PID;
			break;
		case PS:
			G_mw_dc_iris_pid_coef.p_coef = Kp_Domain.PB_PID;
			G_mw_dc_iris_pid_coef.i_coef = Ki_Domain.NS_PID;
			G_mw_dc_iris_pid_coef.d_coef = Kd_Domain.PB_PID;
			break;
		case PB:
			G_mw_dc_iris_pid_coef.p_coef = Kp_Domain.PB_PID;
			G_mw_dc_iris_pid_coef.i_coef = Ki_Domain.NS_PID;
			G_mw_dc_iris_pid_coef.d_coef = Kd_Domain.PB_PID;
			break;
		default:
			G_mw_dc_iris_pid_coef.p_coef = Kp_Domain.Z0_PID;
			G_mw_dc_iris_pid_coef.i_coef = Ki_Domain.Z0_PID;
			G_mw_dc_iris_pid_coef.d_coef = Kd_Domain.Z0_PID;
			break;
		}
		break;
	case PS:
		switch(d_measure) {
		case NB:
			G_mw_dc_iris_pid_coef.p_coef = Kp_Domain.NB_PID;
			G_mw_dc_iris_pid_coef.i_coef = Ki_Domain.NS_PID;
			G_mw_dc_iris_pid_coef.d_coef = Kd_Domain.PB_PID;
			break;
		case NS:
			G_mw_dc_iris_pid_coef.p_coef = Kp_Domain.NB_PID;
			G_mw_dc_iris_pid_coef.i_coef = Ki_Domain.NS_PID;
			G_mw_dc_iris_pid_coef.d_coef = Kd_Domain.PB_PID;
			break;
		case Z0:
			G_mw_dc_iris_pid_coef.p_coef = Kp_Domain.NB_PID;
			G_mw_dc_iris_pid_coef.i_coef = Ki_Domain.NS_PID;
			G_mw_dc_iris_pid_coef.d_coef = Kd_Domain.PB_PID;
			break;
		case PS:
			G_mw_dc_iris_pid_coef.p_coef = Kp_Domain.NB_PID;
			G_mw_dc_iris_pid_coef.i_coef = Ki_Domain.NS_PID;
			G_mw_dc_iris_pid_coef.d_coef = Kd_Domain.PB_PID;
			break;
		case PB:
			G_mw_dc_iris_pid_coef.p_coef = Kp_Domain.NB_PID;
			G_mw_dc_iris_pid_coef.i_coef = Ki_Domain.NS_PID;
			G_mw_dc_iris_pid_coef.d_coef = Kd_Domain.PB_PID;
			break;
		default:
			G_mw_dc_iris_pid_coef.p_coef = Kp_Domain.Z0_PID;
			G_mw_dc_iris_pid_coef.i_coef = Ki_Domain.Z0_PID;
			G_mw_dc_iris_pid_coef.d_coef = Kd_Domain.Z0_PID;
			break;
		}
		break;
	case PB:
		switch(d_measure) {
		case NB:
			G_mw_dc_iris_pid_coef.p_coef = Kp_Domain.NB_PID;
			G_mw_dc_iris_pid_coef.i_coef = Ki_Domain.NB_PID;
			G_mw_dc_iris_pid_coef.d_coef = Kd_Domain.Z0_PID;
			break;
		case NS:
			G_mw_dc_iris_pid_coef.p_coef = Kp_Domain.NB_PID;
			G_mw_dc_iris_pid_coef.i_coef = Ki_Domain.NB_PID;
			G_mw_dc_iris_pid_coef.d_coef = Kd_Domain.Z0_PID;
			break;
		case Z0:
			G_mw_dc_iris_pid_coef.p_coef = Kp_Domain.NB_PID;
			G_mw_dc_iris_pid_coef.i_coef = Ki_Domain.NB_PID;
			G_mw_dc_iris_pid_coef.d_coef = Kd_Domain.Z0_PID;
			break;
		case PS:
			G_mw_dc_iris_pid_coef.p_coef = Kp_Domain.NB_PID;
			G_mw_dc_iris_pid_coef.i_coef = Ki_Domain.NB_PID;
			G_mw_dc_iris_pid_coef.d_coef = Kd_Domain.Z0_PID;
			break;
		case PB:
			G_mw_dc_iris_pid_coef.p_coef = Kp_Domain.NB_PID;
			G_mw_dc_iris_pid_coef.i_coef = Ki_Domain.NB_PID;
			G_mw_dc_iris_pid_coef.d_coef = Kd_Domain.Z0_PID;
			break;
		default:
			G_mw_dc_iris_pid_coef.p_coef = Kp_Domain.Z0_PID;
			G_mw_dc_iris_pid_coef.i_coef = Ki_Domain.Z0_PID;
			G_mw_dc_iris_pid_coef.d_coef = Kd_Domain.Z0_PID;
			break;
		}
		break;
	}
}
#endif

static int iris_duty_adjust(int luma_diff)
{
	luma_data[1] = luma_data[0];
	luma_data[0] = luma_diff;
	//data_p
	G_mw_pid_data.p_data = luma_data[0];
	//data_i
	if (luma_diff < 0) {  //rectify luma_diff to increase its linearity
		if (luma_data[0] > -10) {
			G_mw_pid_data.i_data = G_mw_pid_data.i_data + luma_data[0] * 5;
		} else if (luma_data[0] > -20) {
			G_mw_pid_data.i_data = G_mw_pid_data.i_data + luma_data[0] * 6;
		} else if (luma_data[0] > -40) {
			G_mw_pid_data.i_data = G_mw_pid_data.i_data + luma_data[0] * 7;
		} else if (luma_data[0] > -60) {
			G_mw_pid_data.i_data = G_mw_pid_data.i_data + luma_data[0] * 8;
		} else if (luma_data[0] > -80) {
			G_mw_pid_data.i_data = G_mw_pid_data.i_data + luma_data[0] * 10;
		} else if (luma_data[0] > -100) {
			G_mw_pid_data.i_data = G_mw_pid_data.i_data + luma_data[0] * 12;
		} else {
			G_mw_pid_data.i_data = G_mw_pid_data.i_data + luma_data[0] * 16;
		}
	} else {
		G_mw_pid_data.i_data = G_mw_pid_data.i_data + luma_data[0] * 4;
	}
	//data_d
	G_mw_pid_data.d_data = luma_data[0] - luma_data[1];
#ifdef USE_FUZZY_PID_ALGO
	Update_K(luma_diff, G_mw_pid_data.d_data);
	//update K when using fuzzy PID control algorithm.
	duty = (Kp0 + G_mw_dc_iris_pid_coef.p_coef) * G_mw_pid_data.p_data / Kac +
			(Ki0 + G_mw_dc_iris_pid_coef.i_coef) * G_mw_pid_data.i_data / Kac +
			(Kd0 + G_mw_dc_iris_pid_coef.d_coef) * G_mw_pid_data.d_data / Kac;
	//fuzzy PID control
#else
	duty = G_mw_dc_iris_pid_coef.p_coef * G_mw_pid_data.p_data / Kac +
		G_mw_dc_iris_pid_coef.i_coef * G_mw_pid_data.i_data / Kac +
		G_mw_dc_iris_pid_coef.d_coef * G_mw_pid_data.d_data / Kac;
#endif
	if (duty > MAX_DUTY) {
		duty = MAX_DUTY;
	} else if (duty < MIN_DUTY) {
		duty = MIN_DUTY;
	}
	if (duty != duty_backup) {
		dc_iris_update_duty(duty);
		duty_backup = duty;
	}

	return 0;
}

static int iris_check_full_open(int luma_diff)
{
	int diff_value = 0;
	static int last_low_luma_diff = -1;
	static int count = 0;
	if (duty <= OPEN_DUTY) {
		if (luma_diff >= IGNORE_FULL_OPEN_CHECK) {
			goto check_full_open_exit;
		}

		while (diff_value * (diff_value + 1) < ABS(luma_diff))
			diff_value ++;

		if (ABS(luma_diff - last_low_luma_diff) < diff_value) {
			if (count ++ > FULL_OPEN_CHECK) {
				count = 0;
				G_mw_pid_data.p_data = 0;
				G_mw_pid_data.i_data = 0;
				G_mw_pid_data.d_data = 0;
				return 1;
			}
			return 0;
		}
	}
	check_full_open_exit:
	count = 0;
	last_low_luma_diff = luma_diff;
	return 0;
}

static int luma_control(int run, int luma_diff, int *state)
{
	static int iris_state = MW_IRIS_IDLE;
	if (G_mw_pid_data.enable == 0) {
		dc_iris_update_duty(MIN_DUTY);
		iris_state = MW_IRIS_IDLE;
		goto luma_control_exit;
	}

	if (!run) {
		dc_iris_update_duty(MIN_DUTY);
		iris_state = MW_IRIS_IDLE;
		goto luma_control_exit;
	}
	switch (iris_state) {
	case MW_IRIS_IDLE:
		dc_iris_update_duty(MAX_DUTY);
		wait_time = -1;
		iris_state = MW_IRIS_FLUCTUATE;
		break;
	case MW_IRIS_FLUCTUATE:
		if (iris_check_full_open(luma_diff)) {
			iris_state = MW_IRIS_IDLE;
		} else {
			iris_duty_adjust(luma_diff);
			iris_state = MW_IRIS_FLUCTUATE;
		}
		break;
	default:
		assert(0);
		break;
	}

	luma_control_exit:
	*state = (iris_state != MW_IRIS_IDLE);
	MW_DEBUG("[%d] luma_diff %d, duty %d"
			 " Kp %d, Ki %d, Kd %d,"
			 "data_p %d, data_i %d, data_d %d,"
			 "state %d, \n\n",
			 run, luma_diff, duty,
			 G_mw_dc_iris_pid_coef.p_coef, G_mw_dc_iris_pid_coef.i_coef,
			 G_mw_dc_iris_pid_coef.d_coef,
			 G_mw_dc_iris_pid_coef.p_coef * G_mw_pid_data.p_data / Kac,
			 G_mw_dc_iris_pid_coef.i_coef * G_mw_pid_data.i_data / Kac,
			 G_mw_dc_iris_pid_coef.d_coef * G_mw_pid_data.d_data / Kac,
			 *state);
	//MW_DEBUG("[%d] luma_diff %d, duty %d\n", run, luma_diff, duty);
	return 0;
}

int dc_iris_get_pid_coef(mw_dc_iris_pid_coef * pPid_coef)
{
	pPid_coef->p_coef = G_mw_dc_iris_pid_coef.p_coef;
	pPid_coef->i_coef = G_mw_dc_iris_pid_coef.i_coef;
	pPid_coef->d_coef = G_mw_dc_iris_pid_coef.d_coef;
	return 0;
}

int dc_iris_set_pid_coef(mw_dc_iris_pid_coef * pPid_coef)
{
	if (G_mw_pid_data.enable == 1) {
		MW_ERROR("Cannot set PID coefficients when DC-Iris is running!\n");
		return -1;
	}
	G_mw_dc_iris_pid_coef.p_coef = pPid_coef->p_coef;
	G_mw_dc_iris_pid_coef.i_coef = pPid_coef->i_coef;
	G_mw_dc_iris_pid_coef.d_coef = pPid_coef->d_coef;
	return 0;
}

int enable_dc_iris(void)
{
	static ae_flow_func_t ae_flow_cntl; /*fixme after img lib add memcp*/

	dc_iris_init();
	dc_iris_enable();
	ae_flow_cntl.p_ae_flow_dc_iris_cntl = luma_control;
	img_register_custom_aeflow_func(&ae_flow_cntl);
	G_mw_pid_data.enable = 1;

	MW_DEBUG("Function luma_control is registered into AAA algo.\n");

	return 0;
}

int disable_dc_iris(void)
{
	static ae_flow_func_t ae_flow_cntl; /*fixme after img lib add memcp*/
	ae_flow_cntl.p_ae_flow_dc_iris_cntl = NULL;

	dc_iris_enable();
	dc_iris_update_duty(MIN_DUTY);

	dc_iris_disable();
	dc_iris_deinit();
	img_register_custom_aeflow_func(&ae_flow_cntl);
	G_mw_pid_data.enable = 0;

	MW_DEBUG("Function luma_control is unregistered into AAA algo.\n");

	return 0;
}

