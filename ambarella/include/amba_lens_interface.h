/*
 * amba_lens_interface.h
 *
 * History:
 *	2014/10/10 - [Peter Jiao] Mod base on amba_piris.h
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

#ifndef __AMBA_LENS_INTERFACE_H__
#define  __AMBA_LENS_INTERFACE_H__


enum P_IRIS_WORK_STATE {
    P_IRIS_WORK_STATE_NOT_INIT = 0,	//uninitialized
    P_IRIS_WORK_STATE_PREPARING =  1, 	//doing init
    P_IRIS_WORK_STATE_READY	    = 2,	//init done , ready to adjust p-iris (normal working), or run stop (back to ready)
    P_IRIS_WORK_STATE_RUNNING     = 3,    //running.
};

#define P_IRIS_IOC_MAGIC			'p'

enum P_IRIS_IOC_ENUM {
        IOC_P_IRIS_SET_POS    =   0,
        IOC_P_IRIS_RESET   = 1,
        IOC_P_IRIS_MOVE_STEPS  = 2,
        IOC_P_IRIS_GET_POS = 3,
        IOC_P_IRIS_GET_STATE = 4,
        IOC_P_IRIS_CONFIG = 5,
};

#define AMBA_IOC_P_IRIS_SET_POS	 	_IOW(P_IRIS_IOC_MAGIC, IOC_P_IRIS_SET_POS, int)
#define AMBA_IOC_P_IRIS_RESET		        _IOW(P_IRIS_IOC_MAGIC, IOC_P_IRIS_RESET, int)
#define AMBA_IOC_P_IRIS_MOVE_STEPS	 _IOW(P_IRIS_IOC_MAGIC, IOC_P_IRIS_MOVE_STEPS, int)
#define AMBA_IOC_P_IRIS_GET_POS		 _IOR(P_IRIS_IOC_MAGIC, IOC_P_IRIS_GET_POS, int *)
#define AMBA_IOC_P_IRIS_GET_STATE	 _IOR(P_IRIS_IOC_MAGIC, IOC_P_IRIS_GET_STATE, int *)
#define AMBA_IOC_P_IRIS_CONFIG	 _IOR(P_IRIS_IOC_MAGIC, IOC_P_IRIS_CONFIG, int)

#define P_IRIS_CONTROLLER_MAX_GPIO_NUM  3
typedef struct amba_p_iris_cfg_s{
    int gpio_id[P_IRIS_CONTROLLER_MAX_GPIO_NUM];        //gpio_id[0]= IN1, gpio_id[1]=IN2, gpio_id[2]=EN
    int gpio_val;                   //bit0 is IN1, bit1 is IN2
    int timer_period;           //in ms
    int max_mechanical;
    int min_mechanical ;
    int max_optical;
    int min_optical;
}amba_p_iris_cfg_t;


#define MFZ_IOC_MAGIC			'p'

enum MFZ_IOC_ENUM {
        IOC_MFZ_GET_STATUS = 0,
        IOC_MFZ_SET_ENABLE = 1,
        IOC_MFZ_ZOOM_SET = 2,
        IOC_MFZ_FOCUS_SET = 3,
        IOC_MFZ_IRIS_SET = 4,
        IOC_MFZ_ZOOM_STOP = 5,
        IOC_MFZ_FOCUS_STOP = 6,
};

#define AMBA_IOC_MFZ_GET_STATUS	 	_IOW(MFZ_IOC_MAGIC, IOC_MFZ_GET_STATUS, u32 *)
#define AMBA_IOC_MFZ_SET_ENABLE		_IOW(MFZ_IOC_MAGIC, IOC_MFZ_SET_ENABLE, u32)
#define AMBA_IOC_MFZ_ZOOM_SET		_IOR(MFZ_IOC_MAGIC, IOC_MFZ_ZOOM_SET, u32)
#define AMBA_IOC_MFZ_FOCUS_SET		_IOR(MFZ_IOC_MAGIC, IOC_MFZ_FOCUS_SET, u32)
#define AMBA_IOC_MFZ_IRIS_SET	 	_IOR(MFZ_IOC_MAGIC, IOC_MFZ_IRIS_SET, u32)
#define AMBA_IOC_MFZ_ZOOM_STOP	 	_IOW(MFZ_IOC_MAGIC, IOC_MFZ_ZOOM_STOP, u32 *)
#define AMBA_IOC_MFZ_FOCUS_STOP	 	_IOW(MFZ_IOC_MAGIC, IOC_MFZ_FOCUS_STOP, u32 *)

#define MFZ_COUNT_SHIFT	16
#define MFZ_COUNT_MASK	0xFFF
#define MFZ_DOCMD_SHIFT	28
#define MFZ_DOCMD_MASK	0xF
#define MFZ_STATUS_MASK	0xFF
#define MFZ_STATUS_MASK_ALL	0xFFFFFF
#define MFZ_STEP_MASK	0xFFFF
#define MFZ_EXCEPT_FLAG	0xFFFF8000
#define MFZ_ZSTEP_SHIFT	48
#define MFZ_FSTEP_SHIFT	32
#define MFZ_TDN_SHIFT	24
#define MFZ_IRIS_SHIFT	16
#define MFZ_ZOOM_SHIFT	8
#define MFZ_ENABLE_MASK	0xF
#define MFZ_CONFIG_MASK	0xF0
#define MFZ_ENABLE	0x11
#define MFZ_DISABLE	0x10
#define MFZ_DAY		0x10
#define MFZ_NIGHT	0x11

#define	FULL_SHIFT_STEP		8

#endif // __AMBA_LENS_INTERFACE_H__

