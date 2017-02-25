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

img_aeb_tile_config_t ov9750_aeb_tile_config ={
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

img_aeb_expo_lines_t ov9750_aeb_expo_lines={
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
                {{SHUTTER_1BY30, ISO_100, 0}}, {{SHUTTER_1BY30, ISO_25600,0}}
                }
    }},
};

img_aeb_wb_param_t ov9750_aeb_wb_param ={
        .header = {
                AEB_WB_PARAM,
                1,
                {1, 0, 0, 0},
        },
        .wb_param ={
        {
		{1734, 1024, 1853},	//AUTOMATIC
		{1178, 1024, 2970},	//INCANDESCENT
		{1730, 1024, 2568},	//D4000
		{1734, 1024, 1853},	//D5000
		{1974, 1024, 1701},	//SUNNY
		{2365, 1024, 1531},	//CLOUDY
		{1750, 1024, 1400},	//FLASH
		{1250, 1024, 1825},	//FLUORESCENT
		{1750, 1024, 1400},	//FLUORESCENT_H
		{1024, 1024, 1024},	//UNDER WATER
		{1775, 1024, 1200},	//CUSTOM
		{1974, 1024, 1701},	//AUTOMATIC OUTDOOR
	},
        {
		12,
		{{748,1654,2448,3619,-2465,4802,-1625,5697,910,1430,1191,2381,1}, // 0	INCANDESCENT
		 {967,2040,1904,3045,-1950,4292,-1301,5352,755,864,817,1628,2}, // 1	D4000
		 {1231,2236,1503,2425,-2095,4470,-1708,5618,877,48,734,991,4}, // 2	D5000
		 {1473,2380,1284,2032,-1135,3146,-759,3659,855,-275,864,255,8}, // 3	SUNNY
		 {1753,2765,1133,1889,-350,1867,-664,3449,1016,-1314,1083,-491,4}, // 4	CLOUDY
		 {1224,1957,1632,2214,-3743,6561,-511,3013,417,1036,2188,-832,0},	// 5	Green
		 {   0,    0,    0,    0,     0,    0,     0,    0,    0,     0,    0,     0, 0 },	// 6	D10000
		 {   0,    0,    0,    0,     0,    0,     0,    0,    0,     0,    0,     0, 0 },	// 7    FLASH
		 {   0,    0,    0,    0,     0,    0,     0,    0,    0,     0,    0,     0, 0 }, // 8    FLUORESCENT
		 {   0,    0,    0,    0,     0,    0,     0,    0,    0,     0,    0,     0, 0 }, // 9    FLUORESCENT_2
		 {   0,    0,    0,    0,     0,    0,     0,    0,    0,     0,    0,     0, 0 },	// 10  FLUORESCENT_3
		 {   0,    0,    0,    0,     0,    0,     0,    0,    0,     0,    0,     0, 0 }}, //11 CUSTOM
	},
                {{ 0 ,6},	//LUT num. AUTOMATIC  INDOOR
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
 img_aeb_sensor_config_t ov9750_aeb_sensor_config ={
        .header = {
                AEB_SENSOR_CONFIG,
                1,
                {1, 0, 0, 0},
        },
        .sensor_config  ={
                43,//max_gain_db
                43, //max global gain db
                0, //max single gain db
                2,//shutter_delay
                1,//agc_delay
        },
 };
 img_aeb_sht_nl_table_t ov9750_aeb_sht_nl_table ={
        .header = {
                AEB_SHT_NL_TABLE,
                1,
                {32, 0, 0, 0},
        },
        .sht_nl_table ={
                0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31
        },
 };
img_aeb_gain_table_t ov9750_aeb_gain_table ={
        .header = {
                AEB_GAIN_TABLE,
                1,
                {460, 0, 0, 0},
        },
        .gain_table ={//43.03125db,step =0.09375db
	4096,4140,4185,4231,4277,4323,4370,4417,4465,4514,4563,4612,4662,4713,4764,4816,4868,4921,
	4974,5028,5083,5138,5194,5250,5307,5365,5423,5482,5541,5601,5662,5724,5786,5849,5912,5976,
	6041,6107,6173,6240,6308,6376,6445,6515,6586,6657,6730,6803,6876,6951,7026,7103,7180,7258,
	7336,7416,7497,7578,7660,7743,7827,7912,7998,8085,8173,8261,8351,8442,8533,8626,8719,8814,
	8910,9006,9104,9203,9303,9404,9506,9609,9713,9819,9925,10033,10142,10252,10363,10475,10589,
	10704,10820,10938,11056,11176,11298,11420,11544,11669,11796,11924,12053,12184,12316,12450,
	12585,12722,12860,12999,13140,13283,13427,13573,13720,13869,14020,14172,14326,14481,14638,
	14797,14958,15120,15284,15450,15617,15787,15958,16131,16306,16483,16662,16843,17026,17211,17397,
	17586,17777,17970,18165,18362,18561,18763,18966,19172,19380,19591,19803,20018,20235,20455,20677,
	20901,21128,21357,21589,21823,22060,22300,22542,22786,23034,23283,23536,23792,24050,24311,24575,
	24841,25111,25383,25659,25937,26219,26503,26791,27081,27375,27672,27973,28276,28583,28893,29207,
	29524,29844,30168,30495,30826,31161,31499,31841,32186,32536,32889,33246,33606,33971,34340,34712,
	35089,35470,35855,36244,36637,37035,37437,37843,38254,38669,39088,39513,39941,40375,40813,41256,
	41703,42156,42614,43076,43543,44016,44494,44976,45465,45958,46457,46961,47470,47985,48506,49033,
	49565,50103,50646,51196,51751,52313,52881,53455,54035,54621,55214,55813,56419,57031,57650,58275,58908,
	59547,60193,60846,61507,62174,62849,63531,64220,64917,65622,66334,67054,67781,68517,69260,70012,70772,
	71540,72316,73101,73894,74696,75507,76326,77154,77992,78838,79693,80558,81432,82316,83209,84112,85025,
	85948,86881,87823,88776,89740,90714,91698,92693,93699,94716,95744,96783,97833,98895,99968,101053,102149,
	103258,104378,105511,106656,107813,108983,110166,111361,112570,113792,115026,116275,117536,118812,120101,
	121405,122722,124054,125400,126761,128136,129527,130932,132353,133790,135241,136709,138193,139692,141208,
	142741,144290,145855,147438,149038,150655,152290,153943,155614,157302,159009,160735,162479,164242,166025,
	167826,169647,171488,173349,175231,177132,179054,180997,182962,184947,186954,188983,191034,193107,195202,
	197321,199462,201626,203814,206026,208262,210522,212807,215116,217450,219810,222195,224607,227044,229508,
	231998,234516,237061,239633,242234,244863,247520,250206,252921,255666,258440,261245,264080,266945,269842,
	272771,275731,278723,281747,284805,287896,291020,294178,297370,300597,303859,307157,310490,313859,317265,
	320708,324188,327706,331263,334857,338491,342164,345878,349631,353425,357260,361137,365056,369018,373022,
	377070,381162,385299,389480,393706,397979,402298,406663,411076,415537,420047,424605,429213,433870,438579,
	443338,448149,453012,457928,462898,467921,472999,478132,483320,488565,493867,499226,504644,510120,515656,
	521252,526908,532626,538406,544249,550155,556125,562160,568260,574427,580661,
    },
};

