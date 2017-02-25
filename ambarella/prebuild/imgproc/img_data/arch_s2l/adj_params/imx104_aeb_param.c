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



img_aeb_tile_config_t imx104_aeb_tile_config ={
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

img_aeb_expo_lines_t imx104_aeb_expo_lines={
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


img_aeb_wb_param_t imx104_aeb_wb_param ={
        .header = {
                AEB_WB_PARAM,
                1,
                {1, 0, 0, 0},
        },
        .wb_param ={
	{
		{2150, 1024, 1650},	//AUTOMATIC
		{1250, 1024, 3200},	//INCANDESCENT
		{1600, 1024, 2600},	//D4000
		{1850, 1024, 2000},	//D5000
		{2150, 1024, 1750},	//SUNNY
		{2300, 1024, 1650},	//CLOUDY
		{1598, 1024, 1875},	//FLASH
		{1024, 1024, 1024},	//FLUORESCENT
		{1024, 1024, 1024},	//FLUORESCENT_H
		{1024, 1024, 1024},	//UNDER WATER
		{1024, 1024, 1024},	//CUSTOM
		{1598, 1024, 1875},	//AUTOMATIC OUTDOOR
	},
	{
		12,
		{{ 900,1600,2700,3700, -2500, 5300, -2500,7200, 1000,1500, 1000,2600, 1},	// 0	INCANDESCENT
		 { 1100,2100,2000,3200,-2200,5000, -2500,8200, 1000,500, 1000,1600, 2},	// 1    D4000
		 {1500,2200,1550,2450,-1700,4400, -2000,6400, 1000,-200, 1000,600, 4},	// 2	 D5000
		 {1800,2500, 1400,2100,   -800,3000,   -800,3750, 1000,-500, 1000,-150, 8},	// 3    SUNNY
		 {2000,2700,1300,2000,   -800,3000,   -800, 3750, 1000,-1100, 1000,-200, 4},	// 4    CLOUDY
		 {   0,    0,    0,    0,     0,    0,     0,    0,   0,   0,    0,    0, 0 },					// 5    ...
		 {2100,2500,1700,3500,-1400,4800,-1200,5800,1600,-1800,3000,-3300,-1},		// 6	GREEN REGION
		 {   0,    0,    0,    0,     0,    0,     0,    0,   0,   0,    0,    0, 0 },	// 7    FLASH
		 {   0,    0,    0,    0,     0,    0,     0,    0,   0,   0,    0,    0, 0 },	// 8	FLUORESCENT
		 {   0,    0,    0,    0,     0,    0,     0,    0,   0,   0,    0,    0, 0 },	// 9    FLUORESCENT_2
		 {   0,    0,    0,    0,     0,    0,     0,    0,   0,   0,    0,    0, 0 },	// 10	FLUORESCENT_3
		 {   0,    0,    0,    0,     0,    0,     0,    0,   0,   0,    0,    0, 0 }},
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
	 }},
};

img_aeb_sensor_config_t imx104_aeb_sensor_config ={
        .header = {
                AEB_SENSOR_CONFIG,
                1,
                {1, 0, 0, 0},
        },
        .sensor_config  ={
                36,//max_gain_db
                24, // max global gain db
                0, // max single frame gain db
                2,//shutter_delay
                1,//agc_delay
        },
};


img_aeb_sht_nl_table_t imx104_aeb_sht_nl_table ={
        .header = {
                AEB_SHT_NL_TABLE,
                1,
                {32, 0, 0, 0},
        },
        .sht_nl_table ={
                0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31
        },
};



img_aeb_gain_table_t imx104_aeb_gain_table ={
        .header = {
                AEB_GAIN_TABLE,
                1,
                {141, 0, 0, 0},
        },
        .gain_table ={//42db,step =0.3db
			4096,4240,4389,4543,4703,4868,5039,5216,5400,5589,5786,5989,6200,6417,6643,6876,7118,
			7368,7627,7895,8173,8460,8757,9065,9383,9713,10054,10408,10774,11152,11544,11950,12370,
			12804,13254,13720,14202,14701,15218,15753,16306,16880,17473,18087,18722,19380,20061,20766,
			21496,22252,23034,23843,24681,25548,26446,27375,28337,29333,30364,31431,32536,33679,34863,
			36088,37356,38669,40028,41434,42890,44398,45958,47573,49245,50975,52767,54621,56541,58527,
			60584,62713,64917,67199,69560,72005,74535,77154,79866,82672,85578,88585,91698,94920,98256,
			101709,105283,108983,112813,116778,120882,125130,129527,134079,138791,143668,148717,153943,
			159353,164953,170750,176750,182962,189391,196047,202936,210068,217450,225092,233002,241190,
			249666,258440,267522,276924,286655,296729,307157,317951,329124,340690,352663,365056,377885,
			391165,404911,419141,433870,449117,464900,481238,498150,515656,
        },
};


