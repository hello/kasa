/*
 * image data
 *
 * History:
 *    Author: Lu Wang <lwang@ambarella.com>
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

#include "img_adv_struct_arch.h"



img_aeb_tile_config_t mn34220pl_aeb_tile_config ={
        .header = {
                AEB_TILE_CONFIG,
                1,
                {1, 0, 0, 0},
        },
        .tile_config ={
                1,
                1,

                32,
                32,
                0,
                0,
                128,
                128,
                128,
                128,
                0,
                0x3fff,

                12,
                8,
                0,
                0,
                340,
                512,

                12,
                8,
                0,
                0,
                340,
                512,
                340,
                512,

                1,
                16383,
        },
};

img_aeb_expo_lines_t mn34220pl_aeb_expo_lines={
        .header = {
                AEB_EXPO_LINES,
		  1,				// total ae table number
                {4, 0, 0, 0},		// ae lines per ae table
        },

        .expo_tables[0] ={{
                {
                {{SHUTTER_1BY32000, ISO_100, 0}}, {{SHUTTER_1BY100, ISO_100,0}}
                },

                {
                {{SHUTTER_1BY100, ISO_100, 0}}, {{SHUTTER_1BY100, ISO_300, 0}}
                },

                {
                {{SHUTTER_1BY50, ISO_100, 0}}, {{SHUTTER_1BY50, ISO_800,0}}
                },

                {
                {{SHUTTER_1BY30, ISO_100, 0}}, {{SHUTTER_1BY30, ISO_51200,0}}
                }
        }},
};

img_aeb_expo_lines_t mn34220pl_aeb_expo_lines_2x_hdr = {
        .header = {
                AEB_EXPO_LINES_2X_HDR,
		  2,				// total ae table number
                {3, 1, 0, 0},		// ae lines per ae table
        },
        .expo_tables[0] = {{	// frame 0
                {
                {{SHUTTER_1BY16000, ISO_100, 0}}, {{SHUTTER_1BY100, ISO_100,0}}
                },
                {
                {{SHUTTER_1BY100, ISO_100, 0}}, {{SHUTTER_1BY100, ISO_300, 0}}
                },
                {
                {{SHUTTER_1BY50, ISO_100, 0}}, {{SHUTTER_1BY50, ISO_51200,0}}
                }
        }},
	.expo_tables[1] ={{		// frame 1
		{
		{{SHUTTER_1BY16000, ISO_100, 0}}, {{SHUTTER_1BY120, ISO_100,0}}
		},
	}},
};

img_aeb_expo_lines_t mn34220pl_aeb_expo_lines_3x_hdr = {
        .header = {
                AEB_EXPO_LINES_3X_HDR,
		  3,				// total ae table number
                {3, 1, 1, 0},		// ae lines per ae table
        },
        .expo_tables[0] = {{	// frame 0
                {
                {{SHUTTER_1BY16000, ISO_100, 0}}, {{SHUTTER_1BY100, ISO_100,0}}
                },
                {
                {{SHUTTER_1BY100, ISO_100, 0}}, {{SHUTTER_1BY100, ISO_300, 0}}
                },
                {
                {{SHUTTER_1BY50, ISO_100, 0}}, {{SHUTTER_1BY50, ISO_51200,0}}
                }
        }},
	.expo_tables[1] = {{		// frame 1
		{
		{{SHUTTER_1BY16000, ISO_100, 0}}, {{SHUTTER_1BY200, ISO_100,0}}
		},
	}},
	.expo_tables[2] = {{		// frame 2
		{
		{{SHUTTER_1BY16000, ISO_100, 0}}, {{SHUTTER_1BY480, ISO_100,0}}
		},
	}},
};

img_aeb_wb_param_t mn34220pl_aeb_wb_param ={
        .header = {
                AEB_WB_PARAM,
                1,
                {1, 0, 0, 0},
        },
        .wb_param ={
        {
                {2000, 1024, 1800},	//AUTOMATIC
                {1150, 1024, 3400},	//INCANDESCENT
                {1400, 1024, 2700},	//D4000
                {1700, 1024, 2100},	//D5000
                {2000, 1024, 1800},	//SUNNY //{1875, 1024, 1400}
                {2230, 1024, 1630},	//CLOUDY
                {1024, 1024, 1024},	//FLASH
                {1024, 1024, 1024},	//FLUORESCENT
                {1024, 1024, 1024},	//FLUORESCENT_H
                {1024, 1024, 1024},	//UNDER WATER
                {1875, 1024, 1400},	//CUSTOM
                {1700, 1024, 1600},	//AUTOMATIC OUTDOOR
        },
        {
                12,
                {{900,1500,2600,3700,-2000,4800,-1900,6100,1000,1700,1300,2400,1},		// 0	INCANDESCENT
                {1050,1950,2100,3600,-1700,4400,-1900,7000,1000,800,1200,1600,2},		// 1    D4000
                {1350,2250,1500,2800,-1300,3800,-1300,4800,1000,0,1000,900,4},		// 2	D5000
                {1700,2500,1250,2100,-800,2950,-800,3750,1000,-500,1000,200,8},	// 3    SUNNY
                {1850,2650,1250,2000,-600,2550,-600,3300,1000,-1200,1000,-500,4},	// 4    CLOUDY
                {   0,    0,    0,    0,     0,    0,     0,    0,   0,   0,    0,    0, 0 },	// 5    PROJECTOR
                {1800,2400,1800,3550,-900,3800,-1500,6400,1000,-300,3500,-4100,-1},	// 6 GREEN REGION
                {   0,    0,    0,    0,     0,    0,     0,    0,   0,   0,    0,    0, 0 },	// 7	FLASH
                {   0,    0,    0,    0,     0,    0,     0,    0,   0,   0,    0,    0, 0 },	// 8	FLUORESCENT
                {   0,    0,    0,    0,     0,    0,     0,    0,   0,   0,    0,    0, 0 },	// 9    FLUORESCENT_H
                {   0,    0,    0,    0,     0,    0,     0,    0,   0,   0,    0,    0, 0 },	//UNDER WATER
                {   0,    0,    0,    0,     0,    0,     0,    0,   0,   0,    0,    0, 0 }}//CUSTOM
                },
                {	{ 0 ,6},	//LUT num. AUTOMATIC  INDOOR
                { 0, 1},	//LUT num. INCANDESCENT
                { 1, 1},	//LUT num. D4000
                { 2, 1},	//LUT num. D5000
                { 2, 5},	//LUT num. SUNNY
                { 4, 3},	//LUT num. CLOUDY
                { 7, 1},	//LUT num. FLASH
                { 8, 1},	//LUT num. FLUORESCENT
                { 9, 1},	//LUT num. FLUORESCENT_H
                {11, 1},	//LUT num. UNDER WATER
                {11, 1},	//LUT num. CUSTOM
                { 0, 7},	//LUT num. AUTOMATIC  OUTDOOR
                },
        },

};

img_aeb_sensor_config_t mn34220pl_aeb_sensor_config ={
	.header = {
		AEB_SENSOR_CONFIG,
		1,
		{1, 0, 0, 0},
	},
	.sensor_config  ={
		60,//max_gain_db
		30, // max global gain db
		30, // max single frame gain db
		2,//shutter_delay
		1,//agc_delay
		{	// hdr delay, useless for linear mode
			{{0, 1, 1, 1}},	// shutter
			{{1, 1, 1, 1}},	// agc
			{{2, 2, 2, 2}},	// dgain
			{{2, 2, 2, 2}}, 	// sensor offset
		},
		{	// hdr raw offset coef, non negative value is valid
			0,		// long padding
			2,		// short padding
		},
	},
};

img_aeb_sht_nl_table_t mn34220pl_aeb_sht_nl_table ={
        .header = {
                AEB_SHT_NL_TABLE,
                1,
                {32, 0, 0, 0},
        },
        .sht_nl_table ={
                0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31
        },
};



img_aeb_gain_table_t mn34220pl_aeb_gain_table ={
        .header = {
                AEB_GAIN_TABLE,
                1,
                {161, 0, 0, 0},
        },
        .gain_table ={//60db,step =0.375db
                4096  ,4277  ,4465  ,4662  ,4868  ,5083  ,5307  ,5541  ,5786  ,6041  ,
                6308  ,6586  ,6876  ,7180  ,7497  ,7827  ,8173  ,8533  ,8910  ,9303  ,
                9713  ,10142 ,10589 ,11056 ,11544 ,12053 ,12585 ,13140 ,13720 ,14326 ,
                14958 ,15617 ,16306 ,17026 ,17777 ,18561 ,19380 ,20235 ,21128 ,22060 ,
                23034 ,24050 ,25111 ,26219 ,27375 ,28583 ,29844 ,31161 ,32536 ,33971 ,
                35470 ,37035 ,38669 ,40375 ,42156 ,44016 ,45958 ,47985 ,50103 ,52313 ,
                54621 ,57031 ,59547 ,62174 ,64917 ,67781 ,70772 ,73894 ,77154 ,80558 ,
                84112 ,87823 ,91698 ,95744 ,99968 ,104378,108983,113792,118812,124054,
                129527,135241,141208,147438,153943,160735,167826,175231,182962,191034,
                199462,208262,217450,227044,237061,247520,258440,269842,281747,294178,
                307157,320708,334857,349631,365056,381162,397979,415537,433870,453012,
                472999,493867,515656,538406,562160,586962,612858,639897,668128,697606,
                728383,760519,794072,829106,865685,903878,943756,985394,1028869,1074261,
                1121657,1171143,1222813,1276762,1333091,1391906,1453316,1517434,1584382,
                1654284,1727269,1803474,1883042,1966120,2052863,2143433,2237999,2336738,
                2439832,2547475,2659867,2777218,2899746,3027680,3161258,3300730,3446355,
                3598404,3757162,3922924,4096000,
        },
};


img_aeb_auto_knee_param_t mn34220pl_aeb_auto_knee_config ={
		.header = {
				AEB_AUTO_KNEE,
				1,
				{1, 0, 0, 0},
		},
		.auto_knee_config = {
				0,//enable
				1,//src mode, 0:cfa,1:rgb
				192,255,//roi low,high
				0,240,320,640,//min,mid_min,mid_max,max
				{
				 {96,  96, 104},//0db,  min,mid,max
				 {108, 108, 114},//0db
				 {114, 114, 124},//0db,
				 {122, 122, 134},//6db
				 {128, 128, 144},
				 {128, 128, 144},
				 {128, 128, 144},
				 {138, 138, 145},
				 {140, 140, 140},
				 {140, 140, 140},
				 {140, 140, 140},
				 {140, 140, 140},
				 {140, 140, 140},
				 {140, 140, 140},
				 {140, 140, 140},
				 {140, 140, 140},
				},
		},
};

img_aeb_digit_wdr_param_t mn34220pl_aeb_digit_wdr_config = {
	.header = {
		AEB_DIGIT_WDR,
		1,
		{1, 0, 0, 0},
	},
	.digit_wdr_config = {
		0, 	// enable
		{	// strength
		0,	// 0db
		0,	// 0db
		0,	// 0db
		0,	// 6db
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		},
	},
};

img_aeb_digit_wdr_param_t mn34220pl_aeb_digit_wdr_config_2x_hdr = {
	.header = {
		AEB_DIGIT_WDR_2X_HDR,
		1,
		{1, 0, 0, 0},
	},
	.digit_wdr_config = {
		0, 	// enable
		{	// strength
		0,	// 0db
		0,	// 0db
		0,	// 0db
		0,	// 6db
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		},
	},
};

img_aeb_digit_wdr_param_t mn34220pl_aeb_digit_wdr_config_3x_hdr = {
	.header = {
		AEB_DIGIT_WDR_3X_HDR,
		1,
		{1, 0, 0, 0},
	},
	.digit_wdr_config = {
		0, 	// enable
		{	// strength
		0,	// 0db
		0,	// 0db
		0,	// 0db
		0,	// 6db
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		},
	},
};

