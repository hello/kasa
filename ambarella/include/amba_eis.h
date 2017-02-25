
/*
 * amba_eis.h
 *
 * History:
 *	2013/09/25 - [Louis Sun] created file
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

#ifndef __AMBA_EIS_H__
#define  __AMBA_EIS_H__



typedef struct gyro_info_s {
	u8	gyro_id;                // gyro sensor id

	u8	gyro_pwr_gpio;		// GPIO number controls gyro sensor power
	u8	gyro_hps_gpio;		// GPIO number controls gyro sensor hps
	u8	gyro_int_gpio;		// GPIO number connect to gyro sensor interrupt pin
	u8	gyro_x_chan;		// gyro sensor x axis channel
	u8	gyro_y_chan;		// gyro sensor y axis channel
	u8	gyro_z_chan;		// gyro sensor z axis channel
	u8	gyro_t_chan;		// gyro sensor t axis channel
	u8	gyro_x_reg;			// gyro sensor x axis reg
	u8	gyro_y_reg;			// gyro sensor y axis reg
	u8	gyro_z_reg;			// gyro sensor z axis reg
	s8	gyro_x_polar;		// gyro sensor x polarity
	s8	gyro_y_polar;		// gyro sensor y polarity
	s8	gyro_z_polar;		// gyro sensor z polarity
	u8	vol_div_num;		// numerator of voltage divider
	u8	vol_div_den;		// denominator of voltage divider

	u8	sensor_interface;	// gyro sensor interface
	u8	sensor_axis;		// gyro sensor axis
	u8	max_rms_noise;		// gyro sensor rms noise level
	u8	adc_resolution;		// gyro internal adc resolution, unit in bit(s)
	s8	phs_dly_in_ms;		// gyro sensor phase delay, unit in ms
	u8	reserved;
	u16	sampling_rate;		// digital gyro internal sampling rate, unit in samples / sec
	u16	max_sampling_rate;	// max digital gyro internal sampling rate, unit in samples / sec
	u16	max_bias;			// max gyro sensor bias
	u16	min_bias;			// min gyro sensor bias
	u16	max_sense;			// max gyro sensor sensitivity
	u16	min_sense;			// min gyro sensor sensitivity
	u16	start_up_time;		// gyro sensor start-up time
	u16	full_scale_range;	// gyro full scale range
	u16	max_sclk;			// max serial clock for digital interface, unit in 100khz
} gyro_info_t;

typedef struct gyro_data_s{
	int          sample_id; //incremental id of gyro data sample
	s16		xg;        //agular velocity on Axis-X (Gyro )
	s16		yg;        //agular velocity on Axis-Y (Gyro )
	s16		zg;        //agular velocity on Axis-Z (Gyro )
	s16		xa;       //acceleration on Axis-X  (Accelerometer)
	s16		ya;       //acceleration on Axis-Y  (Accelerometer)
	s16		za;       //acceleration on Axis-Z  (Accelerometer)
} gyro_data_t;


void gyro_get_info(gyro_info_t *);
typedef void (*GYRO_EIS_CALLBACK)(gyro_data_t *, void * arg);
void gyro_register_eis_callback(GYRO_EIS_CALLBACK cb, void *arg);
void gyro_unregister_eis_callback(void);




//this eis driver is for "EIS by Cortex with Queue",  it enqueues Gyro data, and data filtering and EIS calculation is done on ARM

#define EIS_IOC_MAGIC			'e'


enum EIS_IOC_ENUM {
        IOC_EIS_GET_INFO  = 0,
        IOC_EIS_START_STAT    =  1,
        IOC_EIS_STOP_STAT      =  2,
        IOC_EIS_GET_STAT  = 3,
};



#define GYRO_DATA_ENTRY_MAX_NUM     256
//by design, eis stat read out is once per frame.
//each time, eis stat read out should be no more than GYRO_DATA_ENTRY_MAX_NUM
typedef struct amba_eis_stat_s{
        int gyro_data_count;
        int discard_flag;
        gyro_data_t   gyro_data[GYRO_DATA_ENTRY_MAX_NUM];
}amba_eis_stat_t;

typedef struct amba_eis_info_s{
        int version;
        int type;
        int gyro_queue_max_depth;
        int status;
}amba_eis_info_t;


//IOCTLs  for driver EIS
#define AMBA_IOC_EIS_START_STAT	        _IO(EIS_IOC_MAGIC, IOC_EIS_START_STAT)
#define AMBA_IOC_EIS_STOP_STAT		 _IO(EIS_IOC_MAGIC, IOC_EIS_STOP_STAT)
#define AMBA_IOC_EIS_GET_STAT	               _IOR(EIS_IOC_MAGIC, IOC_EIS_GET_STAT, struct amba_eis_stat_s *)
#define AMBA_IOC_EIS_GET_INFO	               _IOR(EIS_IOC_MAGIC, IOC_EIS_GET_INFO, struct amba_eis_info_s *)


#endif // __AMBA_EIS_H__


