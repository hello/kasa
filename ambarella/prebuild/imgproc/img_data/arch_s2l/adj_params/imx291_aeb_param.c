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



img_aeb_tile_config_t imx291_aeb_tile_config ={
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

img_aeb_expo_lines_t imx291_aeb_expo_lines={
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
                {{SHUTTER_1BY30, ISO_100, 0}}, {{SHUTTER_1BY30, ISO_102400,0}}
                }
        }},
};


img_aeb_wb_param_t imx291_aeb_wb_param ={
        .header = {
                AEB_WB_PARAM,
                1,
                {1, 0, 0, 0},
        },
        .wb_param ={
	{
		{1756, 1024, 1946},	//AUTOMATIC
		{1135, 1024, 3435},	//INCANDESCENT
		{1736, 1024, 2956},	//D4000
		{1756, 1024, 1946},	//D5000
		{2013, 1024, 1754},	//SUNNY
		{2292, 1024, 1619},	//CLOUDY
		{1598, 1024, 1875},	//FLASH
		{1024, 1024, 1024},	//FLUORESCENT
		{1024, 1024, 1024},	//FLUORESCENT_H
		{1024, 1024, 1024},	//UNDER WATER
		{1024, 1024, 1024},	//CUSTOM
		{2013, 1024, 1754},	//AUTOMATIC OUTDOOR
	},
	{
		12,
		{{806,1497,2865,3892,-2627,5397,-1929,6246,854,1994,1101,2847,1},	// 0	INCANDESCENT
		 {1002,2009,2256,3316,-2309,5014,-1932,6607,721,1157,770,2195,2},	// 1    D4000
		 {1261,2222,1585,2707,-1661,4188,-1110,4517,1338,-707,478,1664,4},	// 2	 D5000
		 {1635,2486,1263,2245,-1245,3587,-1284,4913,1092,-581,1144,-89,8},	// 3    SUNNY
		 {1894,2676,1218,1912,-681,2601,-693,3563,1276,-1709,1307,-843,4},	// 4    CLOUDY
		 {   0,    0,    0,    0,     0,    0,     0,    0,   0,   0,    0,    0, 0 },					// 5    ...
		 {1405,2146,2538,3384,-1104,4412,-1339,6055,921,851,878,1891,0},		// 6	GREEN REGION
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

img_aeb_sensor_config_t imx291_aeb_sensor_config ={
        .header = {
                AEB_SENSOR_CONFIG,
                1,
                {1, 0, 0, 0},
        },
        .sensor_config  ={
                63,//max_gain_db
                63, // max global gain db
                0, // max single frame gain db
                2,//shutter_delay
                1,//agc_delay
        },
};


img_aeb_sht_nl_table_t imx291_aeb_sht_nl_table ={
        .header = {
                AEB_SHT_NL_TABLE,
                1,
                {32, 0, 0, 0},
        },
        .sht_nl_table ={
                0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31
        },
};



img_aeb_gain_table_t imx291_aeb_gain_table ={
        .header = {
                AEB_GAIN_TABLE,
                1,
                {211, 0, 0, 0},
        },
        .gain_table ={//63db,step =0.3db
			4096,4240,4389,4543,4703,4868,5039,5216,5400,5589,5786,5989,6200,6417,6643,6876,7118,
			7368,7627,7895,8173,8460,8757,9065,9383,9713,10054,10408,10774,11152,11544,11950,12370,
			12804,13254,13720,14202,14701,15218,15753,16306,16880,17473,18087,18722,19380,20061,20766,
			21496,22252,23034,23843,24681,25548,26446,27375,28337,29333,30364,31431,32536,33679,34863,
			36088,37356,38669,40028,41434,42890,44398,45958,47573,49245,50975,52767,54621,56541,58527,
			60584,62713,64917,67199,69560,72005,74535,77154,79866,82672,85578,88585,91698,94920,98256,
			101709,105283,108983,112813,116778,120882,125130,129527,134079,138791,143668,148717,153943,
			159353,164953,170750,176750,182962,189391,196047,202936,210068,217450,225092,233002,241190,
			249666,258440,267522,276924,286655,296729,307157,317951,329124,340690,352663,365056,377885,
			391165,404911,419141,433870,449117,464900,481238,498150,515656,533777,552535,571952,592052,
			612858,634395,656689,679767,703655,728383,753980,780477,807904,836296,865685,896107,927598,
			960196,993939,1028869,1065025,1102453,1141195,1181299,1222813,1265785,1310267,1356313,1403977,
			1453316,1504388,1557256,1611981,1668630,1727269,1787969,1850802,1915843,1983170,2052863,2125005,
			2199682,2276984,2357002,2439832,2525573,2614327,2706200,2801302,2899746,3001649,3107134,3216325,
			3329354,3446355,3567467,3692835,3822610,3956944,4096000,4239942,4388943,4543180,4702837,4868105,
			5039181,5216269,5399580,5589333,5785754,
        },
};


