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

img_aeb_tile_config_t ov9715_aeb_tile_config ={
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

img_aeb_expo_lines_t ov9715_aeb_expo_lines={
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

img_aeb_wb_param_t ov9715_aeb_wb_param ={
        .header = {
                AEB_WB_PARAM,
                1,
                {1, 0, 0, 0},
        },
        .wb_param ={
        {
		{1781, 1024, 1513},	//AUTOMATIC
		{1071, 1024, 2320},	//INCANDESCENT
		{1263, 1024, 2197},	//D4000
		{1585, 1024, 1772},	//D5000
		{1781, 1024, 1513},	//SUNNY
		{1934, 1024, 1477},	//CLOUDY
		{1750, 1024, 1400},	//FLASH
		{1250, 1024, 1825},	//FLUORESCENT
		{1750, 1024, 1400},	//FLUORESCENT_H
		{1024, 1024, 1024},	//UNDER WATER
		{1775, 1024, 1200},	//CUSTOM
		{1781, 1024, 1513},	//AUTOMATIC OUTDOOR
	},
        {
		12,
		{{ 820, 1500, 1750, 3000, -1250, 3100, -1150, 4250, 1000, 1050, 1000,  2050, 1 }, // 0	INCANDESCENT
		 {  950, 1830, 1650, 2550, -1250, 3100, -1200, 4300, 1000,   550, 1000,  1200, 2 }, // 1	D4000
		 {1150, 2150, 1200, 2270, -1270, 3100, -1000, 3950, 1000, -200, 1000,    600, 4 }, // 2	D5000
		 {1500, 2300, 1000, 1930,  -900, 2600,  -1000, 3950,  650,    100,  650,     650, 8 }, // 3	SUNNY
		 {1650, 2350, 1100, 1800,  -650, 2200,  -1000, 3950,  550,  -100,  550,     750, 4 }, // 4	CLOUDY
		 {2350, 2800, 1600, 2050, -1000, 3950, -1000, 4700, 550,     150,  550,     720, 0 },	// 5	PROJECTOR
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
 img_aeb_sensor_config_t ov9715_aeb_sensor_config ={
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
                1,//agc_delay
        },
 };
 img_aeb_sht_nl_table_t ov9715_aeb_sht_nl_table ={
        .header = {
                AEB_SHT_NL_TABLE,
                1,
                {32, 0, 0, 0},
        },
        .sht_nl_table ={
                0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31
        },
 };
img_aeb_gain_table_t ov9715_aeb_gain_table ={
        .header = {
                AEB_GAIN_TABLE,
                1,
                {81, 0, 0, 0},
        },
        .gain_table ={//30db,step =0.375db
                4096  ,4277  ,4465  ,4662  ,4868  ,5083  ,5307  ,5541  ,5786  ,6041  ,
                6308  ,6586  ,6876  ,7180  ,7497  ,7827  ,8173  ,8533  ,8910  ,9303  ,
                9713  ,10142 ,10589 ,11056 ,11544 ,12053 ,12585 ,13140 ,13720 ,14326 ,
                14958 ,15617 ,16306 ,17026 ,17777 ,18561 ,19380 ,20235 ,21128 ,22060 ,
                23034 ,24050 ,25111 ,26219 ,27375 ,28583 ,29844 ,31161 ,32536 ,33971 ,
                35470 ,37035 ,38669 ,40375 ,42156 ,44016 ,45958 ,47985 ,50103 ,52313 ,
                54621 ,57031 ,59547 ,62174 ,64917 ,67781 ,70772 ,73894 ,77154 ,80558 ,
                84112 ,87823 ,91698 ,95744 ,99968 ,104378,108983,113792,118812,124054,
                129527,
    },
};

