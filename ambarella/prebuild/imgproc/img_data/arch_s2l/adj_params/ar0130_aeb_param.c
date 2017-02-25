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

img_aeb_tile_config_t ar0130_aeb_tile_config ={
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

img_aeb_expo_lines_t ar0130_aeb_expo_lines={
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
                {{SHUTTER_1BY30, ISO_100, 0}}, {{SHUTTER_1BY30, ISO_6400,0}}
                }
    }},
};

img_aeb_wb_param_t ar0130_aeb_wb_param ={
        .header = {
                AEB_WB_PARAM,
                1,
                {1, 0, 0, 0},
        },
        .wb_param ={
        {
		{1500, 1024, 1700},	//AUTOMATIC
		{1050, 1024, 2625},	//INCANDESCENT
		{1300, 1024, 2225},	//D4000
		{1500, 1024, 1700},	//D5000
		{1600, 1024, 1450},	//SUNNY //{1875, 1024, 1400}
		{1700, 1024, 1425},	//CLOUDY
		{1024, 1024, 1024},	//FLASH
		{1024, 1024, 1024},	//FLUORESCENT
		{1024, 1024, 1024},	//FLUORESCENT_H
		{1024, 1024, 1024},	//UNDER WATER
		{1875, 1024, 1400},	//CUSTOM
		{1600, 1024, 1450},	//AUTOMATIC OUTDOOR
	},
        {
		12,
		{{750,1350,2150,3100,-2000,4000,-2500,6100,1300,900,2000,1400,1},	// 0	INCANDESCENT
		 {800,1800,1600,2850,-2000,4000,-2000,6000,1500,-150,1400,900,2},	// 1    D4000
		 {1200,1800,1300,2100,-1500,3350,-1300,4050,1000,50,1000,650,4},	// 2	D5000
		 {1300,1900,1180,1800,-1000,2650,-1000,3550,1400,-1000,1000,100,8},	// 3    SUNNY
		 {1450,2030,880,1550,-1000,2550,-1000,3420,1000,-900,1000,-100,4},	// 4    CLOUDY
		 {   0,    0,    0,    0,     0,    0,     0,    0,   0,   0,    0,    0, 0 },	// 5    FLASH
		 {1450,1550,1500,2000,-3000,6000,-1500,4200,1000,550,1000,0,-1},	// 6	green
		 {   0,    0,    0,    0,     0,    0,     0,    0,   0,   0,    0,    0, 0 },	// 7    FLUORESCENT_H
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
	 }
        },
 };
 img_aeb_sensor_config_t ar0130_aeb_sensor_config ={
        .header = {
                AEB_SENSOR_CONFIG,
                1,
                {1, 0, 0, 0},
        },
        .sensor_config  ={
                30,//max_gain_db
                36, //max global gain db
                0, //max single gain db
                2,//shutter_delay
                2,//agc_delay
        },
 };
 img_aeb_sht_nl_table_t ar0130_aeb_sht_nl_table ={
        .header = {
                AEB_SHT_NL_TABLE,
                1,
                {32, 0, 0, 0},
        },
        .sht_nl_table ={
                0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31
        },
 };
img_aeb_gain_table_t ar0130_aeb_gain_table ={
        .header = {
                AEB_GAIN_TABLE,
                1,
                {161, 0, 0, 0},
        },
        .gain_table ={//30db,step =0.1875dB
       4096, 4185 ,  4277,  4370,  4465 ,  4563 ,  4662,   4764,   4868,  4974 ,  5083,  5194,  5307,  5423,  5541,  5662,   5786,
        5912 ,  6041,  6173,  6308 ,  6445 ,  6586,   6730,   6876,  7026 ,  7180,  7336,  7497,  7660,  7827,  7998,   8173,
        8351 ,  8533,  8719,  8910 ,  9104 ,  9303,   9506,   9713,  9925 , 10142, 10363, 10589, 10820, 11056, 11298,  11544,
       11796 , 12053, 12316, 12585 , 12860 , 13140,  13427,  13720, 14020 , 14326, 14638, 14958, 15284, 15617, 15958,  16306,
       16662 , 17026, 17397, 17777 , 18165 , 18561,  18966,  19380, 19803 , 20235, 20677, 21128, 21589, 22060, 22542,  23034,
       23536 , 24050, 24575, 25111 , 25659 , 26219,  26791,  27375, 27973 , 28583, 29207, 29844, 30495, 31161, 31841,  32536,
       33246 , 33971, 34712, 35470 , 36244 , 37035,  37843,  38669, 39513 , 40375, 41256, 42156, 43076, 44016, 44976,  45958,
       46961 , 47985, 49033, 50103 , 51196 , 52313,  53455,  54621, 55813 , 57031, 58275, 59547, 60846, 62174, 63531,  64917,
       66334 , 67781, 69260, 70772 , 72316 , 73894,  75507,  77154, 78838 , 80558, 82316, 84112, 85948, 87823, 89740,  91698,
       93699 , 95744, 97833, 99968 ,102149 ,104378, 106656, 108983,111361 ,113792,116275,118812,121405,124054,126761, 129527
    },
};

