/*
 * kernel/private/drivers/ambarella/lens/tamron_m13vp288ir/m13vp288ir_piris_priv.h
 *
 * History:
 *	2012/06/29 - [Louis Sun] created file
 *	2014/06/11 - [Peter Jiao] Mod
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



#ifndef __M13VP288IR_PIRIS_PRIV_H__
#define __M13VP288IR_PIRIS_PRIV_H__


#define MORE_STEPS_TO_MOVE      18
#define MAX_OPEN_CLOSE_STEP		82
#define P_IRIS_DEFAULT_POS		0

/* P-iris use 10ms as default timer period */
#define P_IRIS_CONTROL_DEFAULT_PERIOD	10


typedef int (* TIMER_CALLBACK) (void * timer_data);

typedef struct amba_p_iris_controller_s {
	amba_p_iris_cfg_t  cfg;	//iris controller config

	//two out values used for the GPIO controlled p-iris drive.
	int pos;				        //current position (0 ~ n), cannot be negative number
	int state;				//working state, refer to enum P_IRIS_WORK_STATE
	int steps_to_move;		//steps to move (but has not made yet)

       struct completion	move_compl;
	//high res timer

} amba_p_iris_controller_t;

typedef struct amba_p_iris_context_s {
	void 	* file;
	struct mutex	* mutex;
//	u8				* buffer;
	amba_p_iris_controller_t * controller;
} amba_p_iris_context_t;


/* from p_iris_api.c */
int amba_p_iris_init_config(amba_p_iris_controller_t * p_controller);
int amba_p_iris_reset(amba_p_iris_context_t * context, int arg);
int amba_p_iris_move_steps(amba_p_iris_context_t * context, int steps);
int amba_p_iris_set_position(amba_p_iris_context_t * context, int position);
int amba_p_iris_get_position(amba_p_iris_context_t * context, int * pos);
int amba_p_iris_get_state(amba_p_iris_context_t * context, int * state);
int amba_p_iris_config(amba_p_iris_context_t * context, int enable);
void amba_p_iris_lock(void);
void amba_p_iris_unlock(void);
int amba_p_iris_deinit(amba_p_iris_controller_t * p_controller);


/* from p_iris_timer.c */
int amba_p_iris_init_timer(amba_p_iris_controller_t * p_controller, void *pcallback);
int amba_p_iris_start_timer(amba_p_iris_controller_t * p_controller);
int amba_p_iris_deinit_timer(amba_p_iris_controller_t * p_controller);

/* from p_iris_impl.c */
int amba_p_iris_impl_move_one_step(amba_p_iris_controller_t * p_controller, int direction); //direction is -1 or 1
int	amba_p_iris_impl_init(amba_p_iris_controller_t * p_controller);
int	amba_p_iris_impl_disable(amba_p_iris_controller_t * p_controller);
int amba_p_iris_impl_reset(amba_p_iris_controller_t * p_controller, int steps);
int amba_p_iris_impl_check(amba_p_iris_controller_t * p_controller);
void amba_p_iris_impl_config(amba_p_iris_controller_t * p_controller, int enable);


#endif	// __M13VP288IR_PIRIS_PRIV_H__

