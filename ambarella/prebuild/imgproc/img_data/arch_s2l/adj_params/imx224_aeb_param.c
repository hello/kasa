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



img_aeb_tile_config_t imx224_aeb_tile_config ={
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

img_aeb_expo_lines_t imx224_aeb_expo_lines={
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

img_aeb_expo_lines_t imx224_aeb_expo_lines_2x_hdr = {
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
                {{SHUTTER_1BY50, ISO_100, 0}}, {{SHUTTER_1BY50, ISO_25600,0}}
                },
        }},
	.expo_tables[1] ={{		// frame 1
		{
		{{SHUTTER_1BY16000, ISO_100, 0}}, {{SHUTTER_1BY200, ISO_100,0}}
		},
	}},
};

img_aeb_expo_lines_t imx224_aeb_expo_lines_3x_hdr = {
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
                {{SHUTTER_1BY100, ISO_100, 0}}, {{SHUTTER_1BY100, ISO_300,0}}
                },
                {
                {{SHUTTER_1BY50, ISO_100, 0}}, {{SHUTTER_1BY50, ISO_25600,0}}
                },
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

img_aeb_wb_param_t imx224_aeb_wb_param ={
        .header = {
                AEB_WB_PARAM,
                1,
                {1, 0, 0, 0},
        },
        .wb_param ={
	{
		{1654, 1024, 2216},	//AUTOMATIC
		{1084, 1024, 3744},	//INCANDESCENT
		{1701, 1024, 3248},	//D4000
		{1654, 1024, 2216},	//D5000
		{1920, 1024, 1944},	//SUNNY
		{2300, 1024, 1760},	//CLOUDY
		{1598, 1024, 1875},	//FLASH
		{1024, 1024, 1024},	//FLUORESCENT
		{1024, 1024, 1024},	//FLUORESCENT_H
		{1024, 1024, 1024},	//UNDER WATER
		{1024, 1024, 1024},	//CUSTOM
		{1920, 1024, 1944},	//AUTOMATIC OUTDOOR
	},
	{
		12,
		{{619,1511,3221,4926,-3200,5874,-2644,7840,1436,1620,2631,2681,1},	// 0	INCANDESCENT
		 {861,2025,2290,3948,-3001,5639,-1046,5566,1140,811,1331,2106,2},	// 1    D4000
		 {1163,2017,1777,2875,-1731,4170,-2165,6747,1198,-121,1092,1080,4},	// 2	 D5000
		 {1383,2214,1360,2421,-1690,4126,-1312,5031,1714,-1736,1356,-116,8},	// 3    SUNNY
		 {1866,2667,1241,2183,-579,2387,-544,3413,1429,-2118,1865,-1792,4},	// 4    CLOUDY
		 {   0,    0,    0,    0,     0,    0,     0,    0,   0,   0,    0,    0, 0 },					// 5    ...
		 {1405,2146,2538,3384,-1104,4412,-1339,6055,921,851,878,1891,-1},		// 6	GREEN REGION
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

img_aeb_sensor_config_t imx224_aeb_sensor_config ={
	.header = {
		AEB_SENSOR_CONFIG,
		1,
		{1, 0, 0, 0},
	},
	.sensor_config  ={
		54,//max_gain_db
		54, // max global gain db
		0, // max single frame gain db
		2,//shutter_delay
		1,//agc_delay
		{	// hdr delay, useless for linear mode
			{{0, 1, 1, 1}},	// shutter
			{{1, 1, 1, 1}},	// agc
			{{2, 2, 2, 2}},	// dgain
			{{0, 0, 0, 0}},	// sensor offset
		},
		{	// hdr raw offset coef, non negative value is valid
			-1,		// long padding
			-1,		// short padding
		},
	},
};

img_aeb_sht_nl_table_t imx224_aeb_sht_nl_table ={
        .header = {
                AEB_SHT_NL_TABLE,
                1,
                {32, 0, 0, 0},
        },
        .sht_nl_table ={
                0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31
        },
};



img_aeb_gain_table_t imx224_aeb_gain_table ={
        .header = {
                AEB_GAIN_TABLE,
                1,
                {541, 0, 0, 0},
        },
        .gain_table ={//54db,step =0.1db
			4096,4143,4191,4239,4289,4338,4388,4439,4491,4543,4595,4649,4702,4757,4812,4868,4924,4981,5039,5097,5156,5216,
	5276,5337,5399,5462,5525,5589,5654,5719,5785,5852,5920,5989,6058,6128,6199,6271,6343,6417,6491,6566,6642,6719,
	6797,6876,6956,7036,7118,7200,7283,7368,7453,7539,7627,7715,7804,7895,7986,8079,8172,8267,8362,8459,8557,8656,8757,
	8858,8961,9064,9169,9275,9383,9492,9601,9713,9825,9939,10054,10170,10288,10407,10528,10650,10773,10898,11024,11152,
	11281,11411,11544,11677,11812,11949,12088,12228,12369,12512,12657,12804,12952,13102,13254,13407,13563,13720,13879,
	14039,14202,14366,14533,14701,14871,15043,15218,15394,15572,15752,15935,16119,16306,16495,16686,16879,17074,17272,
	17472,17675,17879,18086,18296,18508,18722,18939,19158,19380,19604,19831,20061,20293,20528,20766,21006,21250,21496,
	21745,21996,22251,22509,22769,23033,23300,23570,23842,24119,24398,24680,24966,25255,25548,25844,26143,26445,26752,
	27062,27375,27692,28013,28337,28665,28997,29333,29672,30016,30364,30715,31071,31431,31795,32163,32535,32912,33293,
	33679,34069,34463,34862,35266,35674,36087,36505,36928,37355,37788,38226,38668,39116,39569,40027,40491,40960,41434,
	41914,42399,42890,43387,43889,44397,44911,45431,45957,46490,47028,47572,48123,48681,49244,49814,50391,50975,51565,
	52162,52766,53377,53995,54621,55253,55893,56540,57195,57857,58527,59205,59890,60584,61285,61995,62713,63439,64174,
	64917,65668,66429,67198,67976,68763,69560,70365,71180,72004,72838,73681,74534,75398,76271,77154,78047,78951,79865,
	80790,81725,82672,83629,84597,85577,86568,87570,88584,89610,90648,91698,92759,93833,94920,96019,97131,98256,99393,
	100544,101709,102886,104078,105283,106502,107735,108983,110245,111521,112813,114119,115440,116777,118129,119497,
	120881,122281,123697,125129,126578,128044,129526,131026,132543,134078,135631,137201,138790,140397,142023,143667,
	145331,147014,148716,150438,152180,153942,155725,157528,159352,161198,163064,164952,166862,168795,170749,172726,
	174726,176750,178796,180867,182961,185080,187223,189391,191584,193802,196046,198316,200613,202936,205286,207663,
	210067,212500,214961,217450,219968,222515,225091,227698,230335,233002,235700,238429,241190,243983,246808,249666,252557,
	255481,258440,261432,264459,267522,270620,273753,276923,280130,283373,286655,289974,293332,296728,300164,303640,307156,
	310713,314311,317950,321632,325356,329124,332935,336790,340690,344635,348626,352663,356746,360877,365056,369283,373559,
	377885,382260,386687,391164,395694,400276,404911,409600,414342,419140,423994,428903,433870,438894,443976,449117,454317,
	459578,464900,470283,475729,481237,486810,492447,498149,503918,509753,515655,521626,527666,533777,539957,546210,552535,
	558933,565405,571952,578575,585274,592051,598907,605842,612858,619954,627133,634395,641741,649172,656689,664293,671985,
	679766,687638,695600,703655,711803,720045,728383,736817,745349,753980,762710,771542,780476,789513,798656,807904,817259,
	826723,836295,845979,855775,865685,875709,885849,896107,906483,916980,927598,938339,949204,960195,971314,982561,993939,
	1005448,1017091,1028869,1040782,1052833,1065025,1077357,1089832,1102452,1115218,1128132,1141195,1154409,1167776,1181299,
	1194977,1208815,1222812,1236972,1251295,1265785,1280441,1295268,1310267,1325439,1340787,1356313,1372018,1387905,1403976,
	1420234,1436679,1453315,1470143,1487167,1504388,1521808,1539430,1557255,1575288,1593528,1611981,1630646,1649529,1668629,
	1687951,1707496,1727268,1747269,1767502,1787968,1808672,1829615,1850801,1872232,1893912,1915842,1938027,1960468,1983169,
	2006134,2029363,2052863,
        },
};


