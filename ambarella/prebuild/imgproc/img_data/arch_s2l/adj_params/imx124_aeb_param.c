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



img_aeb_tile_config_t imx124_aeb_tile_config ={
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

img_aeb_expo_lines_t imx124_aeb_expo_lines={
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


img_aeb_wb_param_t imx124_aeb_wb_param ={
        .header = {
                AEB_WB_PARAM,
                1,
                {1, 0, 0, 0},
        },
        .wb_param ={
	{
		{1731, 1024, 1800},	//AUTOMATIC
		{1064, 1024, 3690},	//INCANDESCENT
		{1815, 1024, 3032},	//D4000
		{1731, 1024, 1800},	//D5000
		{2040, 1024, 1686},	//SUNNY
		{2383, 1024, 1507},	//CLOUDY
		{1598, 1024, 1875},	//FLASH
		{1024, 1024, 1024},	//FLUORESCENT
		{1024, 1024, 1024},	//FLUORESCENT_H
		{1024, 1024, 1024},	//UNDER WATER
		{1024, 1024, 1024},	//CUSTOM
		{2040, 1024, 1686},	//AUTOMATIC OUTDOOR
	},
	{
		12,
		{{657,1586,2940,4170,-2021,4882,-1583,6063,1075,1757,1127,3224,1},	// 0	INCANDESCENT
		 {1005,2138,2224,3656,-2196,5016,-1107,5354,809,954,1094,1909,2},	// 1    D4000
		 {1299,2085,1659,2502,-2031,4669,-1828,5935,864,98,536,1546,4},	// 2	 D5000
		 {1549,2463,1322,2190,-1423,3771,-899,3951,690,15,755,569,8},	// 3    SUNNY
		 {1700,2637,1078,2047,-1142,3287,-986,4185,836,-835,752,152,4},	// 4    CLOUDY
		 {   0,    0,    0,    0,     0,    0,     0,    0,   0,   0,    0,    0, 0 },					// 5    ...
		 {2300,2800,1950,2800,-1400,5350,-1200,5800,1600,-2180,3000,-4300,-1},		// 6	GREEN REGION
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

img_aeb_sensor_config_t imx124_aeb_sensor_config ={
        .header = {
                AEB_SENSOR_CONFIG,
                1,
                {1, 0, 0, 0},
        },
        .sensor_config  ={
                48,//max_gain_db
                48, // max global gain db
                0, // max single frame gain db
                2,//shutter_delay
                1,//agc_delay
        },
};


img_aeb_sht_nl_table_t imx124_aeb_sht_nl_table ={
        .header = {
                AEB_SHT_NL_TABLE,
                1,
                {32, 0, 0, 0},
        },
        .sht_nl_table ={
                0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31
        },
};



img_aeb_gain_table_t imx124_aeb_gain_table ={
        .header = {
                AEB_GAIN_TABLE,
                1,
                {481, 0, 0, 0},
        },
        .gain_table ={//48db,step =0.1db
			4096, 4144, 4192, 4240, 4290, 4340, 4390, 4441,
			4493, 4545, 4598, 4651, 4705, 4760, 4815, 4871,
			4928, 4985, 5043, 5101, 5161, 5221, 5281, 5343,
			5405, 5468, 5531, 5595, 5660, 5726, 5793, 5860,
			5928, 5997, 6067, 6137, 6208, 6281, 6353, 6427,
			6502, 6578, 6654, 6731, 6810, 6889, 6969, 7050,
			7132, 7214, 7298, 7383, 7469, 7556, 7643, 7732,
			7822, 7913, 8005, 8098, 8192, 8287, 8383, 8481,
			8579, 8679, 8780, 8882, 8985, 9090, 9195, 9302,
			9410, 9519, 9630, 9742, 9855, 9970, 10086, 10203,
			10321, 10441, 10563, 10685, 10809, 10935, 11062, 11191,
			11321, 11452, 11585, 11720, 11856, 11994, 12133, 12274,
			12417, 12561, 12707, 12855, 13004, 13155, 13308, 13463,
			13619, 13777, 13937, 14099, 14263, 14429, 14596, 14766,
			14938, 15111, 15287, 15464, 15644, 15826, 16010, 16196,
			16384, 16574, 16767, 16962, 17159, 17358, 17560, 17764,
			17970, 18179, 18390, 18604, 18820, 19039, 19260, 19484,
			19710, 19939, 20171, 20405, 20643, 20882, 21125, 21371,
			21619, 21870, 22124, 22381, 22641, 22904, 23170, 23440,
			23712, 23988, 24266, 24548, 24834, 25122, 25414, 25709,
			26008, 26310, 26616, 26925, 27238, 27554, 27875, 28199,
			28526, 28858, 29193, 29532, 29875, 30222, 30574, 30929,
			31288, 31652, 32020, 32392, 32768, 33149, 33534, 33924,
			34318, 34716, 35120, 35528, 35941, 36358, 36781, 37208,
			37641, 38078, 38520, 38968, 39421, 39879, 40342, 40811,
			41285, 41765, 42250, 42741, 43238, 43740, 44248, 44762,
			45283, 45809, 46341, 46879, 47424, 47975, 48533, 49097,
			49667, 50244, 50828, 51419, 52016, 52620, 53232, 53850,
			54476, 55109, 55749, 56397, 57052, 57715, 58386, 59064,
			59751, 60445, 61147, 61858, 62576, 63304, 64039, 64783,
			65536, 66297, 67068, 67847, 68635, 69433, 70240, 71056,
			71882, 72717, 73562, 74416, 75281, 76156, 77041, 77936,
			78841, 79758, 80684, 81622, 82570, 83530, 84500, 85482,
			86475, 87480, 88497, 89525, 90565, 91617, 92682, 93759,
			94848, 95950, 97065, 98193, 99334, 100488, 101656, 102837,
			104032, 105241, 106464, 107701, 108952, 110218, 111499, 112794,
			114105, 115431, 116772, 118129, 119501, 120890, 122295, 123715,
			125153, 126607, 128078, 129567, 131072, 132595, 134136, 135694,
			137271, 138866, 140479, 142112, 143763, 145433, 147123, 148833,
			150562, 152312, 154081, 155872, 157683, 159515, 161369, 163244,
			165140, 167059, 169000, 170964, 172951, 174960, 176993, 179050,
			181130, 183235, 185364, 187518, 189696, 191901, 194130, 196386,
			198668, 200976, 203312, 205674, 208064, 210481, 212927, 215401,
			217904, 220436, 222997, 225588, 228210, 230861, 233544, 236257,
			239003, 241780, 244589, 247431, 250306, 253214, 256157, 259133,
			262144, 265190, 268271, 271388, 274542, 277732, 280959, 284224,
			287526, 290867, 294247, 297666, 301124, 304623, 308163, 311744,
			315366, 319030, 322737, 326487, 330281, 334118, 338001, 341928,
			345901, 349920, 353986, 358099, 362260, 366469, 370728, 375035,
			379393, 383801, 388261, 392772, 397336, 401953, 406623, 411348,
			416128, 420963, 425854, 430802, 435808, 440872, 445995, 451177,
			456419, 461723, 467088, 472515, 478005, 483559, 489178, 494862,
			500612, 506429, 512313, 518266, 524288, 530380, 536543, 542777,
			549084, 555464, 561918, 568447, 575052, 581734, 588493, 595331,
			602249, 609247, 616326, 623487, 630732, 638060, 645474, 652974,
			660561, 668237, 676001, 683856, 691802, 699841, 707972, 716199,
			724520, 732939, 741455, 750070, 758786, 767603, 776522, 785544,
			794672, 803906, 813247, 822696, 832255, 841926, 851708, 861605,
			871616, 881744, 891989, 902354, 912838, 923445, 934175, 945030,
			956010, 967119, 978356, 989724, 1001224, 1012858, 1024626, 1036532, 1048576,
        },
};


