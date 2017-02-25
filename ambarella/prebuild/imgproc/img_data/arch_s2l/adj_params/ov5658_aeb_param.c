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

img_aeb_tile_config_t ov5658_aeb_tile_config ={
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

img_aeb_expo_lines_t ov5658_aeb_expo_lines={
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
                {{SHUTTER_1BY30, ISO_100, 0}}, {{SHUTTER_1BY30, ISO_12800,0}}
                }
    }},
};

 img_aeb_wb_param_t ov5658_aeb_wb_param ={
        .header = {
                AEB_WB_PARAM,
                1,
                {1, 0, 0, 0},
        },
        .wb_param ={
        {
                {1250, 1024, 1710},	//AUTOMATIC
                {830, 1024, 2400},	//INCANDESCENT
                {1150, 1024, 2200},	//D4000
                {1250, 1024, 1710},	//D5000
                {1450, 1024, 1590},	//SUNNY
                {1550, 1024, 1550},	//CLOUDY
                {1750, 1024, 1400},	//FLASH
                {1250, 1024, 1825},	//FLUORESCENT
                {1750, 1024, 1400},	//FLUORESCENT_H
                {1024, 1024, 1024},	//UNDER WATER
                {1775, 1024, 1200},	//CUSTOM
                {1450, 1024, 1590},	//AUTOMATIC OUTDOOR
        },
        {
                12,
                {{450, 1200, 2050, 2950, -1250, 3000, -1150, 4050, 1000, 1250, 1000,  2250, 1}, // 0	INCANDESCENT
                {650, 1430, 1680, 2450, -1650, 3260, -1200, 4050, 1000,   750, 1000,  1400, 2}, // 1	D4000
                {1000, 1650, 1400, 2150, -1270,2900, -1000,3550, 1000,190, 1000, 820, 4 }, // 2	D5000
                {1100, 1800, 1200, 2030,  -900, 2450, -1000,3550,  650,400,900,470, 8}, // 3	SUNNY
                {1250, 2050, 1200, 1900,  -650, 2150,  -800,3180,  550,280,  850,450, 4 }, // 4	CLOUDY
                {2350, 2800, 1600, 2050, -1000, 3950, -1000, 4700, 550,     150,  550,     720, 0 },	// 5	PROJECTOR
                {   0,    0,    0,    0,     0,    0,     0,    0,    0,     0,    0,     0, 0 },	// 6	D10000
                {   0,    0,    0,    0,     0,    0,     0,    0,    0,     0,    0,     0, 0 },	// 7    FLASH
                {   0,    0,    0,    0,     0,    0,     0,    0,    0,     0,    0,     0, 0 }, // 8    FLUORESCENT
                {   0,    0,    0,    0,     0,    0,     0,    0,    0,     0,    0,     0, 0 }, // 9    FLUORESCENT_2
                {   0,    0,    0,    0,     0,    0,     0,    0,    0,     0,    0,     0, 0 },	// 10  FLUORESCENT_3
                {   0,    0,    0,    0,     0,    0,     0,    0,    0,     0,    0,     0, 0 }}, //11 CUSTOM
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
 img_aeb_sensor_config_t ov5658_aeb_sensor_config ={
        .header = {
                AEB_SENSOR_CONFIG,
                1,
                {1, 0, 0, 0},
        },
        .sensor_config  ={
                42,//max_gain_db
                42, //max global gain db
                0, //max single gain db
                2,//shutter_delay
                1,//agc_delay
        },
 };
 img_aeb_sht_nl_table_t ov5658_aeb_sht_nl_table ={
        .header = {
                AEB_SHT_NL_TABLE,
                1,
                {32, 0, 0, 0},
        },
        .sht_nl_table ={
                0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31
        },
 };
img_aeb_gain_table_t ov5658_aeb_gain_table ={
        .header = {
                AEB_GAIN_TABLE,
                1,
                {112, 0, 0, 0},
        },
        .gain_table ={//48db,step =0.378db
                4096,4278,4468,4667,4875,5092,5318,5554,5802,6060,6329,6610,6904,7212,7532,7867,8217,8583,8964,9363,9779,
		10214,10669,11143,11639,12156,12697,13262,13851,14467,15111,15783,16485,17218,17984,18784,19619,20491,
		21403,22355,23349,24387,25472,26605,27788,29024,30315,31663,33071,34542,36078,37683,39358,41109,42937,
		44847,46841,48924,51100,53373,55747,58226,60815,63520,66345,69296,72378,75597,78959,82470,86138,89969,
		93970,98150,102515,107074,111836,116810,122005,127431,133098,139018,145200,151658,158403,165447,172806,
		180491,188518,196902,205659,214806,224359,234337,244759,255645,267014,278889,291293,304248,317779,331912,
		346673,362091,378195,395015,412583,430932,450097,470115,491023,512860,535669,
    },
};

