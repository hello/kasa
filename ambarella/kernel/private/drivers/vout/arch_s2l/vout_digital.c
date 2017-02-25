/*
 * kernel/private/drivers/ambarella/vout/arch_s2l/vout_digital.c
 *
 * History:
 *    2015/08/10 - [Anthony Ginger] Create
 *
 *
 * Copyright (c) 2015 Ambarella, Inc.
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


#include <linux/i2c.h>
#include <linux/clk.h>
#include <plat/clk.h>
#include <iav_ioctl.h>
#include "../vout_pri.h"
#include "vout_arch.h"

typedef struct {
        enum amba_video_mode                    vmode;
        u32                                     type;
        u8                                      system;
        enum PLL_CLK_HZ                         clock;
        u8                                      vid_format;
        u8                                      format;
        u32                                     digital_output_mode;
        enum amba_video_source_csc_mode_info    csc_mode;
        u16                                     hv_hsize;
        u16                                     hv_vtsize;
        u16                                     hv_vbsize;
        u16                                     hsync_start;
        u16                                     hsync_end;
        u16                                     vtsync_start;
        u16                                     vtsync_end;
        u16                                     vbsync_start;
        u16                                     vbsync_end;
        u16                                     vtsync_start_row;
        u16                                     vtsync_start_col;
        u16                                     vtsync_end_row;
        u16                                     vtsync_end_col;
        u16                                     vbsync_start_row;
        u16                                     vbsync_start_col;
        u16                                     vbsync_end_row;
        u16                                     vbsync_end_col;
        u16                                     active_start_x;
        u16                                     active_end_x;
        u16                                     active_start_y;
        u16                                     active_end_y;
        u32                                     ratio;
        u32                                     bits;
        u32                                     fps;
} vout_digital_mode_list;

const static vout_digital_mode_list Vout_digital_mode[] = {
        /**     vmode,                  type,                           system,                 clock,                  vid_format,     format,                         digital_output_mode,    csc_mode,                               hv_hsize,       hv_vtsize,      hv_vbsize,      hsync_start,    hsync_end,      vtsync_start,   vtsync_end,     vbsync_start,   vbsync_end,             vtsync_start_row,       vtsync_start_col,       vtsync_end_row, vtsync_end_col, vbsync_start_row,       vbsync_start_col,       vbsync_end_row, vbsync_end_col, active_start_x, active_end_x,   active_start_y, active_end_y,   ratio,                  bits,                   fps**/
        {AMBA_VIDEO_MODE_480I,          AMBA_VIDEO_TYPE_YUV_656,        AMBA_VIDEO_SYSTEM_NTSC, PLL_CLK_27MHZ,          VD_480I60,      AMBA_VIDEO_FORMAT_INTERLACE,    LCD_MODE_656,           AMBA_VIDEO_SOURCE_CSC_YUVSD2YUVSD,      1716,           262,            263,            0,              1,              0,              1,              0,              1,                      0,                      0,                      0,              1,              0,                      0,                      0,              1,              276,            1715,           20,             259,            AMBA_VIDEO_RATIO_4_3,   AMBA_VIDEO_BITS_8,      YUV525I_DEFAULT_FRAME_RATE},
        {AMBA_VIDEO_MODE_480I,          AMBA_VIDEO_TYPE_YUV_601,        AMBA_VIDEO_SYSTEM_NTSC, PLL_CLK_27MHZ,          VD_480I60,      AMBA_VIDEO_FORMAT_INTERLACE,    LCD_MODE_601_8BITS,     AMBA_VIDEO_SOURCE_CSC_YUVSD2YUVSD,      1716,           262,            263,            0,              124,            0,              3 * 1716,       858,            3 * 1716 + 858,         0,                      0,                      3,              0,              0,                      858,                    3,              858,            238,            238 + 1440 - 1, 18,             18 + 240 - 1,   AMBA_VIDEO_RATIO_4_3,   AMBA_VIDEO_BITS_8,      YUV525I_DEFAULT_FRAME_RATE},
        {AMBA_VIDEO_MODE_576I,          AMBA_VIDEO_TYPE_YUV_656,        AMBA_VIDEO_SYSTEM_PAL,  PLL_CLK_27MHZ,          VD_576I50,      AMBA_VIDEO_FORMAT_INTERLACE,    LCD_MODE_656,           AMBA_VIDEO_SOURCE_CSC_YUVSD2YUVSD,      1728,           312,            313,            0,              1,              0,              1,              0,              1,                      0,                      0,                      0,              1,              0,                      0,                      0,              1,              288,            1727,           22,             309,            AMBA_VIDEO_RATIO_4_3,   AMBA_VIDEO_BITS_8,      YUV625I_DEFAULT_FRAME_RATE},
        {AMBA_VIDEO_MODE_576I,          AMBA_VIDEO_TYPE_YUV_601,        AMBA_VIDEO_SYSTEM_PAL,  PLL_CLK_27MHZ,          VD_576I50,      AMBA_VIDEO_FORMAT_INTERLACE,    LCD_MODE_601_8BITS,     AMBA_VIDEO_SOURCE_CSC_YUVSD2YUVSD,      1728,           312,            313,            0,              126,            0,              3 * 1728,       864,            3 * 1728 + 864,         0,                      0,                      3,              0,              0,                      864,                    3,              864,            264,            264 + 1440 - 1, 22,             22 + 288 - 1,   AMBA_VIDEO_RATIO_4_3,   AMBA_VIDEO_BITS_8,      YUV625I_DEFAULT_FRAME_RATE},
        {AMBA_VIDEO_MODE_D1_NTSC,       AMBA_VIDEO_TYPE_YUV_656,        AMBA_VIDEO_SYSTEM_NTSC, PLL_CLK_27MHZ,          VD_480P60,      AMBA_VIDEO_FORMAT_PROGRESSIVE,  LCD_MODE_656,           AMBA_VIDEO_SOURCE_CSC_YUVSD2YUVSD,      858,            525,            525,            0,              1,              0,              1,              0,              1,                      0,                      0,                      0,              1,              0,                      0,                      0,              1,              138,            857,            41,             520,            AMBA_VIDEO_RATIO_4_3,   AMBA_VIDEO_BITS_8,      YUV480P_DEFAULT_FRAME_RATE},
        {AMBA_VIDEO_MODE_D1_NTSC,       AMBA_VIDEO_TYPE_YUV_601,        AMBA_VIDEO_SYSTEM_NTSC, PLL_CLK_27MHZ,          VD_480P60,      AMBA_VIDEO_FORMAT_PROGRESSIVE,  LCD_MODE_601_16BITS,    AMBA_VIDEO_SOURCE_CSC_YUVSD2YUVSD,      858,            525,            525,            0,              62,             0,              6 * 858,        429,            6 * 858 + 429,          0,                      0,                      6,              1,              0,                      429,                    6,              429,            122,            122 + 720 - 1,  36,             36 + 480 - 1,   AMBA_VIDEO_RATIO_4_3,   AMBA_VIDEO_BITS_8,      YUV480P_DEFAULT_FRAME_RATE},
        {AMBA_VIDEO_MODE_D1_PAL,        AMBA_VIDEO_TYPE_YUV_656,        AMBA_VIDEO_SYSTEM_PAL,  PLL_CLK_27MHZ,          VD_576P50,      AMBA_VIDEO_FORMAT_PROGRESSIVE,  LCD_MODE_656,           AMBA_VIDEO_SOURCE_CSC_YUVSD2YUVSD,      864,            625,            625,            0,              1,              0,              1,              0,              1,                      0,                      0,                      0,              1,              0,                      0,                      0,              1,              144,            863,            44,             619,            AMBA_VIDEO_RATIO_4_3,   AMBA_VIDEO_BITS_8,      YUV576P_DEFAULT_FRAME_RATE},
        {AMBA_VIDEO_MODE_D1_PAL,        AMBA_VIDEO_TYPE_YUV_601,        AMBA_VIDEO_SYSTEM_PAL,  PLL_CLK_27MHZ,          VD_576P50,      AMBA_VIDEO_FORMAT_PROGRESSIVE,  LCD_MODE_601_16BITS,    AMBA_VIDEO_SOURCE_CSC_YUVSD2YUVSD,      864,            625,            625,            0,              64,             0,              5 * 564,        432,            5 * 864 + 432,          0,                      0,                      5,              0,              0,                      432,                    5,              432,            132,            132 + 720 - 1,  44,             44 + 576 - 1,   AMBA_VIDEO_RATIO_4_3,   AMBA_VIDEO_BITS_8,      YUV576P_DEFAULT_FRAME_RATE},
        {AMBA_VIDEO_MODE_720P,          AMBA_VIDEO_TYPE_YUV_656,        AMBA_VIDEO_SYSTEM_NTSC, PLL_CLK_74_25D1001MHZ,  VD_720P60,      AMBA_VIDEO_FORMAT_PROGRESSIVE,  LCD_MODE_656,           AMBA_VIDEO_SOURCE_CSC_YUVSD2YUVSD,      1650,           750,            750,            0,              1,              0,              1,              0,              1,                      0,                      0,                      0,              1,              0,                      0,                      0,              1,              370,            1649,           25,             744,            AMBA_VIDEO_RATIO_4_3,   AMBA_VIDEO_BITS_8,      YUV720P_DEFAULT_FRAME_RATE},
        {AMBA_VIDEO_MODE_720P,          AMBA_VIDEO_TYPE_YUV_601,        AMBA_VIDEO_SYSTEM_NTSC, PLL_CLK_74_25D1001MHZ,  VD_720P60,      AMBA_VIDEO_FORMAT_PROGRESSIVE,  LCD_MODE_601_16BITS,    AMBA_VIDEO_SOURCE_CSC_YUVSD2YUVSD,      1650,           750,            750,            0,              40,             0,              5 * 1650,       825,            5 * 1650 + 825,         0,                      0,                      5,              0,              0,                      825,                    5,              825,            260,            260 + 1280 - 1, 25,             25 + 720 - 1,   AMBA_VIDEO_RATIO_4_3,   AMBA_VIDEO_BITS_8,      YUV720P_DEFAULT_FRAME_RATE},
        {AMBA_VIDEO_MODE_720P25,        AMBA_VIDEO_TYPE_YUV_656,        AMBA_VIDEO_SYSTEM_NTSC, PLL_CLK_74_25MHZ,       VD_NON_FIXED,   AMBA_VIDEO_FORMAT_PROGRESSIVE,  LCD_MODE_656,           AMBA_VIDEO_SOURCE_CSC_YUVSD2YUVSD,      3960,           750,            750,            0,              1,              0,              1,              0,              1,                      0,                      0,                      0,              1,              0,                      0,                      0,              1,              2680,           3959,           25,             744,            AMBA_VIDEO_RATIO_4_3,   AMBA_VIDEO_BITS_8,      AMBA_VIDEO_FPS_25},
        {AMBA_VIDEO_MODE_720P25,        AMBA_VIDEO_TYPE_YUV_601,        AMBA_VIDEO_SYSTEM_NTSC, PLL_CLK_74_25MHZ,       VD_NON_FIXED,   AMBA_VIDEO_FORMAT_PROGRESSIVE,  LCD_MODE_601_16BITS,    AMBA_VIDEO_SOURCE_CSC_YUVSD2YUVSD,      3960,           750,            750,            0,              40,             0,              5 * 3960,       1980,           5 * 3960 + 1980,        0,                      0,                      5,              0,              0,                      1980,                   5,              1980,           260,            260 + 1280 - 1, 25,             25 + 720 - 1,   AMBA_VIDEO_RATIO_4_3,   AMBA_VIDEO_BITS_8,      AMBA_VIDEO_FPS_25},
        {AMBA_VIDEO_MODE_720P30,        AMBA_VIDEO_TYPE_YUV_656,        AMBA_VIDEO_SYSTEM_NTSC, PLL_CLK_74_25MHZ,       VD_NON_FIXED,   AMBA_VIDEO_FORMAT_PROGRESSIVE,  LCD_MODE_656,           AMBA_VIDEO_SOURCE_CSC_YUVSD2YUVSD,      3300,           750,            750,            0,              1,              0,              1,              0,              1,                      0,                      0,                      0,              1,              0,                      0,                      0,              1,              2020,           3299,           25,             744,            AMBA_VIDEO_RATIO_4_3,   AMBA_VIDEO_BITS_8,      AMBA_VIDEO_FPS_30},
        {AMBA_VIDEO_MODE_720P30,        AMBA_VIDEO_TYPE_YUV_601,        AMBA_VIDEO_SYSTEM_NTSC, PLL_CLK_74_25MHZ,       VD_NON_FIXED,   AMBA_VIDEO_FORMAT_PROGRESSIVE,  LCD_MODE_601_16BITS,    AMBA_VIDEO_SOURCE_CSC_YUVSD2YUVSD,      3300,           750,            750,            0,              40,             0,              5 * 3300,       1650,           5 * 3300 + 1650,        0,                      0,                      5,              0,              0,                      1650,                   5,              1650,           260,            260 + 1280 - 1, 25,             25 + 720 - 1,   AMBA_VIDEO_RATIO_4_3,   AMBA_VIDEO_BITS_8,      AMBA_VIDEO_FPS_30},
        {AMBA_VIDEO_MODE_720P50,        AMBA_VIDEO_TYPE_YUV_656,        AMBA_VIDEO_SYSTEM_NTSC, PLL_CLK_74_25MHZ,       VD_720P50,      AMBA_VIDEO_FORMAT_PROGRESSIVE,  LCD_MODE_656,           AMBA_VIDEO_SOURCE_CSC_YUVSD2YUVSD,      1980,           750,            750,            0,              1,              0,              1,              0,              1,                      0,                      0,                      0,              1,              0,                      0,                      0,              1,              700,            1979,           25,             744,            AMBA_VIDEO_RATIO_4_3,   AMBA_VIDEO_BITS_8,      YUV720P50_DEFAULT_FRAME_RATE},
        {AMBA_VIDEO_MODE_720P50,        AMBA_VIDEO_TYPE_YUV_601,        AMBA_VIDEO_SYSTEM_NTSC, PLL_CLK_74_25MHZ,       VD_720P50,      AMBA_VIDEO_FORMAT_PROGRESSIVE,  LCD_MODE_601_16BITS,    AMBA_VIDEO_SOURCE_CSC_YUVSD2YUVSD,      1980,           750,            750,            0,              40,             0,              5 * 1980,       990,            5 * 1980 + 990,         0,                      0,                      5,              0,              0,                      990,                    5,              990,            260,            260 + 1280 - 1, 25,             25 + 720 - 1,   AMBA_VIDEO_RATIO_4_3,   AMBA_VIDEO_BITS_8,      YUV720P50_DEFAULT_FRAME_RATE},
        {AMBA_VIDEO_MODE_1080I,         AMBA_VIDEO_TYPE_YUV_656,        AMBA_VIDEO_SYSTEM_NTSC, PLL_CLK_74_25D1001MHZ,  VD_1080I60,     AMBA_VIDEO_FORMAT_INTERLACE,    LCD_MODE_656,           AMBA_VIDEO_SOURCE_CSC_YUVSD2YUVSD,      2200,           562,            563,            0,              1,              0,              1,              0,              1,                      0,                      0,                      0,              1,              0,                      0,                      0,              1,              280,            2199,           20,             559,            AMBA_VIDEO_RATIO_4_3,   AMBA_VIDEO_BITS_8,      YUV1080I_DEFAULT_FRAME_RATE},
        {AMBA_VIDEO_MODE_1080I,         AMBA_VIDEO_TYPE_YUV_601,        AMBA_VIDEO_SYSTEM_NTSC, PLL_CLK_74_25D1001MHZ,  VD_1080I60,     AMBA_VIDEO_FORMAT_INTERLACE,    LCD_MODE_601_16BITS,    AMBA_VIDEO_SOURCE_CSC_YUVSD2YUVSD,      2200,           562,            563,            0,              44,             0,              5 * 2200,       1100,           5 * 2200 + 1100,        0,                      0,                      5,              0,              0,                      1100,                   5,              1100,           192,            192 + 1920 - 1, 20,             20 + 540 - 1,   AMBA_VIDEO_RATIO_4_3,   AMBA_VIDEO_BITS_8,      YUV1080I_DEFAULT_FRAME_RATE},
        {AMBA_VIDEO_MODE_1080I50,       AMBA_VIDEO_TYPE_YUV_656,        AMBA_VIDEO_SYSTEM_PAL,  PLL_CLK_74_25MHZ,       VD_1080I50,     AMBA_VIDEO_FORMAT_INTERLACE,    LCD_MODE_656,           AMBA_VIDEO_SOURCE_CSC_YUVSD2YUVSD,      2640,           562,            563,            0,              1,              0,              1,              0,              1,                      0,                      0,                      0,              1,              0,                      0,                      0,              1,              720,            2639,           20,             559,            AMBA_VIDEO_RATIO_4_3,   AMBA_VIDEO_BITS_8,      YUV1080I50_DEFAULT_FRAME_RATE},
        {AMBA_VIDEO_MODE_1080I50,       AMBA_VIDEO_TYPE_YUV_601,        AMBA_VIDEO_SYSTEM_PAL,  PLL_CLK_74_25MHZ,       VD_1080I50,     AMBA_VIDEO_FORMAT_INTERLACE,    LCD_MODE_601_16BITS,    AMBA_VIDEO_SOURCE_CSC_YUVSD2YUVSD,      2640,           562,            563,            0,              44,             0,              5 * 2640,       1320,           5 * 2640 + 1320,        0,                      0,                      5,              0,              0,                      1320,                   5,              1320,           192,            192 + 1920 - 1, 20,             20 + 540 - 1,   AMBA_VIDEO_RATIO_4_3,   AMBA_VIDEO_BITS_8,      YUV1080I50_DEFAULT_FRAME_RATE},
//        {AMBA_VIDEO_MODE_960_576,       AMBA_VIDEO_TYPE_YUV_656,        AMBA_VIDEO_SYSTEM_PAL,  PLL_CLK_36MHZ,       VD_NON_FIXED,     AMBA_VIDEO_FORMAT_INTERLACE,    LCD_MODE_656,           AMBA_VIDEO_SOURCE_CSC_YUVSD2YUVSD,      4608,           312,            313,            0,              1,              0,              1,              0,              1,                      0,                      0,                      0,              1,              0,                      0,                      0,              1,              3647,            4607,           23,             310,            AMBA_VIDEO_RATIO_4_3,   AMBA_VIDEO_BITS_8,      YUV625I_DEFAULT_FRAME_RATE},
//        {AMBA_VIDEO_MODE_960_480,       AMBA_VIDEO_TYPE_YUV_656,        AMBA_VIDEO_SYSTEM_NTSC,  PLL_CLK_36MHZ,       VD_NON_FIXED,     AMBA_VIDEO_FORMAT_INTERLACE,    LCD_MODE_656,           AMBA_VIDEO_SOURCE_CSC_YUVSD2YUVSD,      4576,           262,            263,            0,              1,              0,              1,              0,              1,                      0,                      0,                      0,              1,              0,                      0,                      0,              1,              3615,            4575,           20,             259,            AMBA_VIDEO_RATIO_4_3,   AMBA_VIDEO_BITS_8,      YUV1080P_DEFAULT_FRAME_RATE},
        {AMBA_VIDEO_MODE_1080P,         AMBA_VIDEO_TYPE_YUV_656,        AMBA_VIDEO_SYSTEM_NTSC, PLL_CLK_148_5D1001MHZ,  VD_1080P60,     AMBA_VIDEO_FORMAT_PROGRESSIVE,  LCD_MODE_656,           AMBA_VIDEO_SOURCE_CSC_YUVSD2YUVSD,      2200,           1125,           1125,           0,              1,              0,              1,              0,              1,                      0,                      0,                      0,              1,              0,                      0,                      0,              1,              280,            2199,           41,             1120,           AMBA_VIDEO_RATIO_4_3,   AMBA_VIDEO_BITS_8,      YUV1080P_DEFAULT_FRAME_RATE},
        {AMBA_VIDEO_MODE_1080P,         AMBA_VIDEO_TYPE_YUV_601,        AMBA_VIDEO_SYSTEM_NTSC, PLL_CLK_148_5D1001MHZ,  VD_1080P60,     AMBA_VIDEO_FORMAT_PROGRESSIVE,  LCD_MODE_601_16BITS,    AMBA_VIDEO_SOURCE_CSC_YUVSD2YUVSD,      2200,           1125,           1125,           0,              44,             0,              5 * 2200,       1100,           5 * 2200 + 1100,        0,                      0,                      5,              0,              0,                      1100,                   5,              1100,           192,            192 + 1920 - 1, 41,             41 + 1080 - 1,  AMBA_VIDEO_RATIO_4_3,   AMBA_VIDEO_BITS_8,      YUV1080P_DEFAULT_FRAME_RATE},
        {AMBA_VIDEO_MODE_1080P50,       AMBA_VIDEO_TYPE_YUV_656,        AMBA_VIDEO_SYSTEM_PAL,  PLL_CLK_148_5MHZ,       VD_1080P60,     AMBA_VIDEO_FORMAT_PROGRESSIVE,  LCD_MODE_656,           AMBA_VIDEO_SOURCE_CSC_YUVSD2YUVSD,      2640,           1125,           1125,           0,              1,              0,              1,              0,              1,                      0,                      0,                      0,              1,              0,                      0,                      0,              1,              720,            2639,           41,             1120,           AMBA_VIDEO_RATIO_4_3,   AMBA_VIDEO_BITS_8,      YUV1080P50_DEFAULT_FRAME_RATE},
        {AMBA_VIDEO_MODE_1080P50,       AMBA_VIDEO_TYPE_YUV_601,        AMBA_VIDEO_SYSTEM_PAL,  PLL_CLK_148_5MHZ,       VD_1080P60,     AMBA_VIDEO_FORMAT_PROGRESSIVE,  LCD_MODE_601_16BITS,    AMBA_VIDEO_SOURCE_CSC_YUVSD2YUVSD,      2640,           1125,           1125,           0,              44,             0,              5 * 2640,       1320,           5 * 2640 + 1320,        0,                      0,                      5,              0,              0,                      1320,                   5,              1320,           192,            192 + 1920 - 1, 41,             41 + 1080 - 1,  AMBA_VIDEO_RATIO_4_3,   AMBA_VIDEO_BITS_8,      YUV1080P50_DEFAULT_FRAME_RATE},
        {AMBA_VIDEO_MODE_1080P30,       AMBA_VIDEO_TYPE_YUV_656,        AMBA_VIDEO_SYSTEM_NTSC, PLL_CLK_74_25D1001MHZ,  VD_1080P60,     AMBA_VIDEO_FORMAT_PROGRESSIVE,  LCD_MODE_656,           AMBA_VIDEO_SOURCE_CSC_YUVSD2YUVSD,      2200,           1125,           1125,           0,              1,              0,              1,              0,              1,                      0,                      0,                      0,              1,              0,                      0,                      0,              1,              280,            2199,           41,             1120,           AMBA_VIDEO_RATIO_4_3,   AMBA_VIDEO_BITS_8,      AMBA_VIDEO_FPS_30},
        {AMBA_VIDEO_MODE_1080P30,       AMBA_VIDEO_TYPE_YUV_601,        AMBA_VIDEO_SYSTEM_NTSC, PLL_CLK_74_25D1001MHZ,  VD_1080P60,     AMBA_VIDEO_FORMAT_PROGRESSIVE,  LCD_MODE_601_16BITS,    AMBA_VIDEO_SOURCE_CSC_YUVSD2YUVSD,      2200,           1125,           1125,           0,              44,             0,              5 * 2200,       1100,           5 * 2200 + 1100,        0,                      0,                      5,              0,              0,                      1100,                   5,              1100,           192,            192 + 1920 - 1, 41,             41 + 1080 - 1,  AMBA_VIDEO_RATIO_4_3,   AMBA_VIDEO_BITS_8,      AMBA_VIDEO_FPS_30},
        {AMBA_VIDEO_MODE_1080P25,       AMBA_VIDEO_TYPE_YUV_656,        AMBA_VIDEO_SYSTEM_NTSC, PLL_CLK_74_25MHZ,       VD_1080P50,     AMBA_VIDEO_FORMAT_PROGRESSIVE,  LCD_MODE_656,           AMBA_VIDEO_SOURCE_CSC_YUVSD2YUVSD,      2640,           1125,           1125,           0,              1,              0,              1,              0,              1,                      0,                      0,                      0,              1,              0,                      0,                      0,              1,              720,            2639,           41,             1120,           AMBA_VIDEO_RATIO_4_3,   AMBA_VIDEO_BITS_8,      AMBA_VIDEO_FPS_25},
        {AMBA_VIDEO_MODE_1080P25,       AMBA_VIDEO_TYPE_YUV_601,        AMBA_VIDEO_SYSTEM_NTSC, PLL_CLK_74_25D1001MHZ,  VD_1080P60,     AMBA_VIDEO_FORMAT_PROGRESSIVE,  LCD_MODE_601_16BITS,    AMBA_VIDEO_SOURCE_CSC_YUVSD2YUVSD,      2640,           1125,           1125,           0,              44,             0,              5 * 2640,       1320,           5 * 2640 + 1320,        0,                      0,                      5,              0,              0,                      1320,                   5,              1320,           192,            192 + 1920 - 1, 41,             41 + 1080 - 1,  AMBA_VIDEO_RATIO_4_3,   AMBA_VIDEO_BITS_8,      AMBA_VIDEO_FPS_25},
        {AMBA_VIDEO_MODE_2560X1440P25,  AMBA_VIDEO_TYPE_YUV_656,        AMBA_VIDEO_SYSTEM_NTSC, PLL_CLK_148_5MHZ,       VD_NON_FIXED,   AMBA_VIDEO_FORMAT_PROGRESSIVE,  LCD_MODE_656,           AMBA_VIDEO_SOURCE_CSC_YUVSD2YUVSD,      3960,           1500,           1500,           0,              1,              0,              1,              0,              1,                      0,                      0,                      0,              1,              0,                      0,                      0,              1,              1400,           3959,           51/*60*/,       1490/*1499*/,   AMBA_VIDEO_RATIO_4_3,   AMBA_VIDEO_BITS_8,      AMBA_VIDEO_FPS_25},
        {AMBA_VIDEO_MODE_2560X1440P30,  AMBA_VIDEO_TYPE_YUV_656,        AMBA_VIDEO_SYSTEM_NTSC, PLL_CLK_148_5MHZ,       VD_NON_FIXED,   AMBA_VIDEO_FORMAT_PROGRESSIVE,  LCD_MODE_656,           AMBA_VIDEO_SOURCE_CSC_YUVSD2YUVSD,      3300,           1500,           1500,           0,              1,              0,              1,              0,              1,                      0,                      0,                      0,              1,              0,                      0,                      0,              1,              740,            3299,           51/*60*/,       1490/*1499*/,   AMBA_VIDEO_RATIO_4_3,   AMBA_VIDEO_BITS_8,      AMBA_VIDEO_FPS_30},
        {AMBA_VIDEO_MODE_WVGA,          AMBA_VIDEO_TYPE_RGB_601,        AMBA_VIDEO_SYSTEM_AUTO, 28397174,               VD_NON_FIXED,   AMBA_VIDEO_FORMAT_PROGRESSIVE,  LCD_MODE_601_24BITS,    AMBA_VIDEO_SOURCE_CSC_YUVSD2RGB,        940,            504,            504,            0,              2,              0,              1879,           0,              1879,                   0,                      0,                      2,              0,              0,                      0,                      2,              0,              120,            959,            20,             499,            AMBA_VIDEO_RATIO_4_3,   AMBA_VIDEO_BITS_16,     AMBA_VIDEO_FPS_60},
        {AMBA_VIDEO_MODE_240_320,       AMBA_VIDEO_TYPE_RGB_601,        AMBA_VIDEO_SYSTEM_AUTO, 7000000,                VD_NON_FIXED,   AMBA_VIDEO_FORMAT_PROGRESSIVE,  LCD_MODE_601_16BITS,    AMBA_VIDEO_SOURCE_CSC_YUVSD2RGB,        298,            330,            0,              0,              10,             0,              0,              0,              0,                      0,                      0,                      4,              0,              0,                      0,                      4,              0,              30,             270,            8,              328,            AMBA_VIDEO_RATIO_4_3,   AMBA_VIDEO_BITS_16,     AMBA_VIDEO_FPS_60},
        {AMBA_VIDEO_MODE_960_240,       AMBA_VIDEO_TYPE_RGB_601,        AMBA_VIDEO_SYSTEM_NTSC, PLL_CLK_27MHZ,          VD_NON_FIXED,   AMBA_VIDEO_FORMAT_INTERLACE,    LCD_MODE_601_24BITS,    AMBA_VIDEO_SOURCE_CSC_YUVSD2RGB,        1716,           263,            263,            0,              1,              0,              1,              0,              1,                      0,                      0,                      0,              1,              0,                      0,                      0,              1,              241,            1200,           21,             260,            AMBA_VIDEO_RATIO_4_3,   AMBA_VIDEO_BITS_16,     YUV525I_DEFAULT_FRAME_RATE},
        {AMBA_VIDEO_MODE_320_240,       AMBA_VIDEO_TYPE_RGB_601,        AMBA_VIDEO_SYSTEM_NTSC, PLL_CLK_27MHZ,          VD_NON_FIXED,   AMBA_VIDEO_FORMAT_INTERLACE,    LCD_MODE_601_24BITS,    AMBA_VIDEO_SOURCE_CSC_YUVSD2RGB,        1716,           263,            263,            0,              1,              0,              1,              0,              1,                      0,                      0,                      0,              1,              0,                      0,                      0,              1,              241,            560,            21,             260,            AMBA_VIDEO_RATIO_4_3,   AMBA_VIDEO_BITS_16,     YUV525I_DEFAULT_FRAME_RATE},
        {AMBA_VIDEO_MODE_320_288,       AMBA_VIDEO_TYPE_RGB_601,        AMBA_VIDEO_SYSTEM_NTSC, PLL_CLK_27MHZ,          VD_NON_FIXED,   AMBA_VIDEO_FORMAT_INTERLACE,    LCD_MODE_601_24BITS,    AMBA_VIDEO_SOURCE_CSC_YUVSD2RGB,        1728,           313,            313,            0,              1,              0,              1,              0,              1,                      0,                      0,                      0,              1,              0,                      0,                      0,              1,              241,            560,            21,             308,            AMBA_VIDEO_RATIO_4_3,   AMBA_VIDEO_BITS_16,     YUV625I_DEFAULT_FRAME_RATE},
        {AMBA_VIDEO_MODE_360_240,       AMBA_VIDEO_TYPE_RGB_601,        AMBA_VIDEO_SYSTEM_NTSC, PLL_CLK_27MHZ,          VD_NON_FIXED,   AMBA_VIDEO_FORMAT_INTERLACE,    LCD_MODE_601_24BITS,    AMBA_VIDEO_SOURCE_CSC_YUVSD2RGB,        1716,           263,            263,            0,              1,              0,              1,              0,              1,                      0,                      0,                      0,              1,              0,                      0,                      0,              1,              241,            600,            21,             260,            AMBA_VIDEO_RATIO_4_3,   AMBA_VIDEO_BITS_16,     YUV525I_DEFAULT_FRAME_RATE},
        {AMBA_VIDEO_MODE_360_288,       AMBA_VIDEO_TYPE_RGB_601,        AMBA_VIDEO_SYSTEM_NTSC, PLL_CLK_27MHZ,          VD_NON_FIXED,   AMBA_VIDEO_FORMAT_INTERLACE,    LCD_MODE_601_24BITS,    AMBA_VIDEO_SOURCE_CSC_YUVSD2RGB,        1728,           313,            313,            0,              1,              0,              1,              0,              1,                      0,                      0,                      0,              1,              0,                      0,                      0,              1,              241,            600,            21,             308,            AMBA_VIDEO_RATIO_4_3,   AMBA_VIDEO_BITS_16,     YUV625I_DEFAULT_FRAME_RATE},
        {AMBA_VIDEO_MODE_480_640,       AMBA_VIDEO_TYPE_RGB_601,        AMBA_VIDEO_SYSTEM_AUTO, PLL_CLK_27MHZ,          VD_NON_FIXED,   AMBA_VIDEO_FORMAT_PROGRESSIVE,  LCD_MODE_601_24BITS,    AMBA_VIDEO_SOURCE_CSC_YUVSD2RGB,        500,            900,            900,            0,              1,              0,              1,              0,              1,                      0,                      0,                      0,              1,              0,                      0,                      0,              1,              10,             489,            130,            769,            AMBA_VIDEO_RATIO_4_3,   AMBA_VIDEO_BITS_16,     AMBA_VIDEO_FPS_60},
        {AMBA_VIDEO_MODE_VGA,           AMBA_VIDEO_TYPE_RGB_601,        AMBA_VIDEO_SYSTEM_AUTO, PLL_CLK_27MHZ,          VD_NON_FIXED,   AMBA_VIDEO_FORMAT_PROGRESSIVE,  LCD_MODE_601_24BITS,    AMBA_VIDEO_SOURCE_CSC_YUVSD2RGB,        500,            900,            900,            0,              1,              0,              1,              0,              1,                      0,                      0,                      0,              1,              0,                      0,                      0,              1,              10,             649,            130,            609,            AMBA_VIDEO_RATIO_4_3,   AMBA_VIDEO_BITS_16,     AMBA_VIDEO_FPS_60},
        {AMBA_VIDEO_MODE_HVGA,          AMBA_VIDEO_TYPE_RGB_601,        AMBA_VIDEO_SYSTEM_AUTO, 17982018,               VD_NON_FIXED,   AMBA_VIDEO_FORMAT_PROGRESSIVE,  LCD_MODE_601_24BITS,    AMBA_VIDEO_SOURCE_CSC_YUVSD2RGB,        500,            600,            600,            0,              10,             0,              1,              0,              1,                      0,                      0,                      2,              0,              0,                      0,                      2,              0,              14,             333,            4,              483,            AMBA_VIDEO_RATIO_4_3,   AMBA_VIDEO_BITS_16,     AMBA_VIDEO_FPS_60},
        {AMBA_VIDEO_MODE_480_800,       AMBA_VIDEO_TYPE_RGB_601,        AMBA_VIDEO_SYSTEM_AUTO, 23790210,               VD_NON_FIXED,   AMBA_VIDEO_FORMAT_PROGRESSIVE,  LCD_MODE_601_24BITS,    AMBA_VIDEO_SOURCE_CSC_YUVSD2RGB,        490,            810,            810,            0,              2,              0,              1,              0,              1,                      0,                      0,                      2,              0,              0,                      0,                      2,              0,              5,              484,            5,              804,            AMBA_VIDEO_RATIO_4_3,   AMBA_VIDEO_BITS_16,     AMBA_VIDEO_FPS_60},
        {AMBA_VIDEO_MODE_240_400,       AMBA_VIDEO_TYPE_RGB_601,        AMBA_VIDEO_SYSTEM_AUTO, 19316264,               VD_NON_FIXED,   AMBA_VIDEO_FORMAT_PROGRESSIVE,  LCD_MODE_601_24BITS,    AMBA_VIDEO_SOURCE_CSC_YUVSD2RGB,        786,            410,            410,            0,              3,              0,              1571,           0,              1571,                   0,                      0,                      1,              785,            0,                      0,                      1,              785,            9,              248,            3,              402,            AMBA_VIDEO_RATIO_4_3,   AMBA_VIDEO_BITS_16,     AMBA_VIDEO_FPS_60},
        {AMBA_VIDEO_MODE_XGA,           AMBA_VIDEO_TYPE_RGB_601,        AMBA_VIDEO_SYSTEM_AUTO, PLL_CLK_65D1001MHZ,     VD_NON_FIXED,   AMBA_VIDEO_FORMAT_PROGRESSIVE,  LCD_MODE_601_24BITS,    AMBA_VIDEO_SOURCE_CSC_YUVSD2RGB,        1344,           806,            806,            0,              3,              0,              2199,           0,              2199,                   0,                      0,                      1,              1099,           0,                      0,                      1,              1099,           160,            1183,           19,             786,            AMBA_VIDEO_RATIO_4_3,   AMBA_VIDEO_BITS_16,     AMBA_VIDEO_FPS_60},
        {AMBA_VIDEO_MODE_WSVGA,         AMBA_VIDEO_TYPE_RGB_RAW,        AMBA_VIDEO_SYSTEM_AUTO, 51200000,               VD_NON_FIXED,   AMBA_VIDEO_FORMAT_PROGRESSIVE,  LCD_MODE_601_24BITS,    AMBA_VIDEO_SOURCE_CSC_YUVSD2RGB,        1344,           635,            635,            0,              4,              0,              3,              0,              3,                      0,                      0,                      2,              0,              0,                      0,                      2,              0,              10,             1033,           10,             609,            AMBA_VIDEO_RATIO_4_3,   AMBA_VIDEO_BITS_16,     AMBA_VIDEO_FPS_60},
        {AMBA_VIDEO_MODE_960_540,       AMBA_VIDEO_TYPE_RGB_RAW,        AMBA_VIDEO_SYSTEM_AUTO, 25000000,               VD_NON_FIXED,   AMBA_VIDEO_FORMAT_PROGRESSIVE,  LCD_MODE_601_24BITS,    AMBA_VIDEO_SOURCE_CSC_YUVSD2RGB,        900,            600,            600,            0,              4,              0,              3,              0,              3,                      0,                      0,                      2,              0,              0,                      0,                      2,              0,              10,             969,            3,              569,            AMBA_VIDEO_RATIO_4_3,   AMBA_VIDEO_BITS_16,     AMBA_VIDEO_FPS_60},
};

#define	VDMODE_NUM	ARRAY_SIZE(Vout_digital_mode)


static int ambdbus_init_rgb_mode(struct __amba_vout_video_source *psrc,
                                 struct amba_video_sink_mode *sink_mode,
                                 int mod_index)
{
        int					rval = 0;
        struct amba_vout_hv_size_info		sink_hv;
        struct amba_vout_hv_sync_info		sink_sync;
        struct amba_vout_window_info		sink_window;
        struct amba_video_info			sink_video_info;
        struct amba_video_source_clock_setup	clk_setup;
        u32			                colors_per_dot;
        vd_config_t				sink_cfg;
        struct amba_video_source_csc_info	sink_csc;
        vout_video_setup_t		        video_setup;

        //set up clock
        switch (sink_mode->frame_rate) {
        case AMBA_VIDEO_FPS_AUTO:
        case AMBA_VIDEO_FPS(60):
                sink_mode->frame_rate = AMBA_VIDEO_FPS(60);
                clk_setup.freq_hz = Vout_digital_mode[mod_index].clock;
                break;

        case AMBA_VIDEO_FPS(50):
                sink_mode->frame_rate = AMBA_VIDEO_FPS(50);
                clk_setup.freq_hz = 5 * Vout_digital_mode[mod_index].clock / 6;
                break;

        case AMBA_VIDEO_FPS(30):
                sink_mode->frame_rate = AMBA_VIDEO_FPS(30);
                clk_setup.freq_hz = Vout_digital_mode[mod_index].clock / 2;
                break;

        case AMBA_VIDEO_FPS(15):
                sink_mode->frame_rate = AMBA_VIDEO_FPS(15);
                clk_setup.freq_hz = Vout_digital_mode[mod_index].clock / 4;
                break;

        default:
                break;
        }
        amba_s2l_vout_set_clock_setup(psrc, &clk_setup);

        // set up lcd
        rval = amba_s2l_vout_set_lcd(psrc, &sink_mode->lcd_cfg);
        if(rval)
                return rval;

        // set config
        rval = amba_s2l_vout_get_config(psrc, &sink_cfg);
        if (rval)
                return rval;
        sink_cfg.d_control.s.analog_out = 0;
        sink_cfg.d_control.s.hdmi_out = 0;
        sink_cfg.d_control.s.digital_out = 1;
        if (sink_mode->format == AMBA_VIDEO_FORMAT_PROGRESSIVE)
                sink_cfg.d_control.s.interlace = VD_PROGRESSIVE;
        else
                sink_cfg.d_control.s.interlace = VD_INTERLACE;
        sink_cfg.d_control.s.vid_format = Vout_digital_mode[mod_index].vid_format;
        sink_cfg.d_digital_output_mode.s.hspol = LCD_ACT_LOW;
        sink_cfg.d_digital_output_mode.s.vspol = LCD_ACT_LOW;
        sink_cfg.d_digital_output_mode.s.mode = sink_cfg.d_digital_output_mode.s.mode;
        rval = amba_s2l_vout_set_config(psrc, &sink_cfg);
        if(rval)
                return rval;

        //set csc
        sink_csc.path = AMBA_VIDEO_SOURCE_CSC_DIGITAL;
        if (sink_mode->csc_en)
                sink_csc.mode = Vout_digital_mode[mod_index].csc_mode;
        else
                sink_csc.mode = AMBA_VIDEO_SOURCE_CSC_RGB2RGB;
        sink_csc.clamp = AMBA_VIDEO_SOURCE_CSC_DATARANGE_DIGITAL_SD_FULL;
        rval = amba_s2l_vout_set_csc(psrc, &sink_csc);
        if (rval)
                return rval;

        switch (sink_mode->lcd_cfg.mode) {
        case AMBA_VOUT_LCD_MODE_3COLORS_PER_DOT:
                colors_per_dot = 3;
                break;

        case AMBA_VOUT_LCD_MODE_3COLORS_DUMMY_PER_DOT:
                colors_per_dot = 4;
                break;

        case AMBA_VOUT_LCD_MODE_DISABLE:
        case AMBA_VOUT_LCD_MODE_1COLOR_PER_DOT:
        case AMBA_VOUT_LCD_MODE_RGB565:
        case AMBA_VOUT_LCD_MODE_RGB888:
        default:
                colors_per_dot = 1;
                break;
        }

        //set hv
        sink_hv.hsize = Vout_digital_mode[mod_index].hv_hsize;
        sink_hv.vtsize = Vout_digital_mode[mod_index].hv_vtsize;
        sink_hv.vbsize = Vout_digital_mode[mod_index].hv_vbsize;
        rval = amba_s2l_vout_set_hv(psrc, &sink_hv);
        if(rval)
                return rval;

        //set hvsync
        sink_sync.hsync_start = Vout_digital_mode[mod_index].hsync_start;
        sink_sync.hsync_end = Vout_digital_mode[mod_index].hsync_end;
        //sink_sync.vtsync_start = Vout_digital_mode[mod_index].vtsync_start;
        //sink_sync.vtsync_end = Vout_digital_mode[mod_index].vtsync_end;
        //sink_sync.vbsync_start = Vout_digital_mode[mod_index].vbsync_start;
        //sink_sync.vbsync_end = Vout_digital_mode[mod_index].vbsync_end;
        sink_sync.vtsync_start_row = Vout_digital_mode[mod_index].vtsync_start_row;
        sink_sync.vtsync_start_col = Vout_digital_mode[mod_index].vtsync_start_col;
        sink_sync.vtsync_end_row = Vout_digital_mode[mod_index].vtsync_end_row;
        sink_sync.vtsync_end_col = Vout_digital_mode[mod_index].vtsync_end_col;
        sink_sync.vbsync_start_row = Vout_digital_mode[mod_index].vbsync_start_row;
        sink_sync.vbsync_start_col = Vout_digital_mode[mod_index].vbsync_start_col;
        sink_sync.vbsync_end_row = Vout_digital_mode[mod_index].vbsync_end_row;
        sink_sync.vbsync_end_col = Vout_digital_mode[mod_index].vbsync_end_col;
        sink_sync.sink_type = sink_mode->sink_type;
        rval = amba_s2l_vout_set_hvsync(psrc, &sink_sync);
        if(rval)
                return rval;

        //set active window
        sink_window.start_x = Vout_digital_mode[mod_index].active_start_x;
        sink_window.end_x = Vout_digital_mode[mod_index].active_end_x;
        sink_window.start_y = Vout_digital_mode[mod_index].active_start_y;
        sink_window.end_y = Vout_digital_mode[mod_index].active_end_y;
        sink_window.width = sink_window.end_x - sink_window.start_x + 1;
        if((sink_mode->mode == AMBA_VIDEO_MODE_320_240) ||
            (sink_mode->mode == AMBA_VIDEO_MODE_320_288) ||
            (sink_mode->mode == AMBA_VIDEO_MODE_360_240) ||
            (sink_mode->mode == AMBA_VIDEO_MODE_360_288)) {
                sink_window.width = sink_window.width / colors_per_dot;
        }
        sink_window.field_reverse = 0;
        rval = amba_s2l_vout_set_active_win(psrc, &sink_window);
        if(rval)
                return rval;

        //set video info
        sink_video_info.width = sink_mode->video_size.video_width;
        sink_video_info.height = sink_mode->video_size.video_height;
        sink_video_info.system = Vout_digital_mode[mod_index].system;
        sink_video_info.fps = Vout_digital_mode[mod_index].fps;
        if((sink_mode->mode == AMBA_VIDEO_MODE_960_240) ||
            (sink_mode->mode == AMBA_VIDEO_MODE_320_240) ||
            (sink_mode->mode == AMBA_VIDEO_MODE_320_288) ||
            (sink_mode->mode == AMBA_VIDEO_MODE_360_240) ||
            (sink_mode->mode == AMBA_VIDEO_MODE_360_288)) {
                if(sink_mode->format == AMBA_VIDEO_FORMAT_PROGRESSIVE)
                        sink_video_info.fps = YUV480P_DEFAULT_FRAME_RATE;
        }
        sink_video_info.format = sink_mode->format;
        sink_video_info.type = sink_mode->type;
        sink_video_info.bits = sink_mode->bits;
        sink_video_info.ratio = sink_mode->ratio;
        sink_video_info.flip = sink_mode->video_flip;
        sink_video_info.rotate = sink_mode->video_rotate;
        rval = amba_s2l_vout_set_video_info(psrc, &sink_video_info);
        if(rval)
                return rval;

        //set video size
        sink_window.start_x = sink_mode->video_offset.offset_x;
        sink_window.start_y = sink_mode->video_offset.offset_y;
        sink_window.end_x = sink_window.start_x + sink_mode->video_size.video_width - 1;
        sink_window.end_y = sink_window.start_y + sink_mode->video_size.video_height - 1;
        if (sink_mode->format == AMBA_VIDEO_FORMAT_INTERLACE) {
                sink_window.start_y >>= 1;
                sink_window.end_y >>= 1;
        }
        rval = amba_s2l_vout_set_video_size(psrc, &sink_window);
        if(rval)
                return rval;

        //set vout video setup
        rval = amba_s2l_vout_get_setup(psrc, &video_setup);
        if(rval)
                return rval;
        video_setup.en = sink_mode->video_en;
        switch (sink_mode->video_flip) {
        case AMBA_VOUT_FLIP_NORMAL:
                video_setup.flip = 0;
                break;
        case AMBA_VOUT_FLIP_HV:
                video_setup.flip = 1;
                break;
        case AMBA_VOUT_FLIP_HORIZONTAL:
                video_setup.flip = 2;
                break;
        case AMBA_VOUT_FLIP_VERTICAL:
                video_setup.flip = 3;
                break;
        default:
                vout_err("%s can't support flip[%d]!\n",
                         __func__, sink_mode->video_flip);
                return -EINVAL;
        }
        switch (sink_mode->video_rotate) {
        case AMBA_VOUT_ROTATE_NORMAL:
                video_setup.rotate = 0;
                break;

        case AMBA_VOUT_ROTATE_90:
                video_setup.rotate = 1;
                break;

        default:
                vout_info("%s can't support rotate[%d]!\n",
                          __func__, sink_mode->video_rotate);
                return -EINVAL;
        }

        rval = amba_s2l_vout_set_setup(psrc, &video_setup);
        if(rval)
                return rval;

        return rval;
}


static int ambdbus_init_yuv_mode(struct __amba_vout_video_source *psrc,
                                 struct amba_video_sink_mode *sink_mode,
                                 int mod_index)
{
        int					rval = 0;
        struct amba_vout_hv_size_info		sink_hv;
        struct amba_vout_hv_sync_info		sink_sync;
        struct amba_vout_window_info		sink_window;
        struct amba_video_info			sink_video_info;
        struct amba_video_source_clock_setup	clk_setup;
        vd_config_t				sink_cfg;
        struct amba_video_source_csc_info	sink_csc;
        vout_video_setup_t		        video_setup;
        struct amba_vout_hvld_sync_info 	sink_hvld;

        //set hvld if needed
        sink_hvld.hvld_type = AMBA_VOUT_HVLD_POL_HIGH;
        if((sink_mode->type == AMBA_VIDEO_TYPE_YUV_601) &&
            (sink_mode->mode != AMBA_VIDEO_MODE_480I) &&
            (sink_mode->mode != AMBA_VIDEO_MODE_576I)) {
                rval = amba_s2l_vout_set_hvld(psrc, &sink_hvld);
                if (rval)
                        return rval;
        }

        //set up clock
        clk_setup.freq_hz = Vout_digital_mode[mod_index].clock;
        amba_s2l_vout_set_clock_setup(psrc, &clk_setup);

        // set config
        rval = amba_s2l_vout_get_config(psrc, &sink_cfg);
        if (rval)
                return rval;

        sink_cfg.d_control.s.analog_out = 0;
        sink_cfg.d_control.s.hdmi_out = 0;
        sink_cfg.d_control.s.digital_out = 1;
        if (Vout_digital_mode[mod_index].format == AMBA_VIDEO_FORMAT_PROGRESSIVE)
                sink_cfg.d_control.s.interlace = VD_PROGRESSIVE;
        else
                sink_cfg.d_control.s.interlace = VD_INTERLACE;
        sink_cfg.d_control.s.vid_format = Vout_digital_mode[mod_index].vid_format;
        sink_cfg.d_digital_output_mode.s.hspol = LCD_ACT_LOW;
        sink_cfg.d_digital_output_mode.s.vspol = LCD_ACT_LOW;
        sink_cfg.d_digital_output_mode.s.mode = Vout_digital_mode[mod_index].digital_output_mode;
        rval = amba_s2l_vout_set_config(psrc, &sink_cfg);
        if(rval)
                return rval;

        //set csc
        sink_csc.path = AMBA_VIDEO_SOURCE_CSC_DIGITAL;
        if (sink_mode->csc_en)
                sink_csc.mode = Vout_digital_mode[mod_index].csc_mode;
        else
                sink_csc.mode = AMBA_VIDEO_SOURCE_CSC_RGB2RGB;
        sink_csc.clamp = AMBA_VIDEO_SOURCE_CSC_DATARANGE_DIGITAL_SD_FULL;
        rval = amba_s2l_vout_set_csc(psrc, &sink_csc);
        if (rval)
                return rval;

        //set hv
        sink_hv.hsize = Vout_digital_mode[mod_index].hv_hsize;
        sink_hv.vtsize = Vout_digital_mode[mod_index].hv_vtsize;
        sink_hv.vbsize = Vout_digital_mode[mod_index].hv_vbsize;
        rval = amba_s2l_vout_set_hv(psrc, &sink_hv);
        if(rval)
                return rval;

        //set hvsync
        sink_sync.hsync_start = Vout_digital_mode[mod_index].hsync_start;
        sink_sync.hsync_end = Vout_digital_mode[mod_index].hsync_end;
        sink_sync.vtsync_start = Vout_digital_mode[mod_index].vtsync_start;
        sink_sync.vtsync_end = Vout_digital_mode[mod_index].vtsync_end;
        sink_sync.vbsync_start = Vout_digital_mode[mod_index].vbsync_start;
        sink_sync.vbsync_end = Vout_digital_mode[mod_index].vbsync_end;
        sink_sync.vtsync_start_row = Vout_digital_mode[mod_index].vtsync_start_row;
        sink_sync.vtsync_start_col = Vout_digital_mode[mod_index].vtsync_start_col;
        sink_sync.vtsync_end_row = Vout_digital_mode[mod_index].vtsync_end_row;
        sink_sync.vtsync_end_col = Vout_digital_mode[mod_index].vtsync_end_col;
        sink_sync.vbsync_start_row = Vout_digital_mode[mod_index].vbsync_start_row;
        sink_sync.vbsync_start_col = Vout_digital_mode[mod_index].vbsync_start_col;
        sink_sync.vbsync_end_row = Vout_digital_mode[mod_index].vbsync_end_row;
        sink_sync.vbsync_end_col = Vout_digital_mode[mod_index].vbsync_end_col;
        sink_sync.sink_type = sink_mode->sink_type;
        rval = amba_s2l_vout_set_hvsync(psrc, &sink_sync);
        if(rval)
                return rval;

        //set active window
        sink_window.start_x = Vout_digital_mode[mod_index].active_start_x;
        sink_window.end_x = Vout_digital_mode[mod_index].active_end_x;
        sink_window.start_y = Vout_digital_mode[mod_index].active_start_y;
        sink_window.end_y = Vout_digital_mode[mod_index].active_end_y;
        sink_window.width = sink_window.end_x - sink_window.start_x + 1;
        if(Vout_digital_mode[mod_index].format == AMBA_VIDEO_FORMAT_INTERLACE)
                sink_window.width = sink_window.width >> 1;
        sink_window.field_reverse = 0;
        rval = amba_s2l_vout_set_active_win(psrc, &sink_window);
        if(rval)
                return rval;

        //set video info
        sink_video_info.width = sink_mode->video_size.video_width;
        sink_video_info.height = sink_mode->video_size.video_height;
        sink_video_info.system = Vout_digital_mode[mod_index].system;
        sink_video_info.fps = Vout_digital_mode[mod_index].fps;
        sink_video_info.format = sink_mode->format;
        sink_video_info.type = sink_mode->type;
        sink_video_info.bits = sink_mode->bits;
        sink_video_info.ratio = sink_mode->ratio;
        sink_video_info.flip = sink_mode->video_flip;
        sink_video_info.rotate = sink_mode->video_rotate;
        rval = amba_s2l_vout_set_video_info(psrc, &sink_video_info);
        if(rval)
                return rval;

        //set video size
        sink_window.start_x = sink_mode->video_offset.offset_x;
        sink_window.start_y = sink_mode->video_offset.offset_y;
        sink_window.end_x = sink_window.start_x + sink_mode->video_size.video_width - 1;
        sink_window.end_y = sink_window.start_y + sink_mode->video_size.video_height - 1;
        if (sink_mode->format == AMBA_VIDEO_FORMAT_INTERLACE) {
                sink_window.start_y >>= 1;
                sink_window.end_y >>= 1;
        }
        rval = amba_s2l_vout_set_video_size(psrc, &sink_window);
        if(rval)
                return rval;

        //set vout video setup
        rval = amba_s2l_vout_get_setup(psrc, &video_setup);
        if(rval)
                return rval;
        video_setup.en = sink_mode->video_en;
        switch (sink_mode->video_flip) {
        case AMBA_VOUT_FLIP_NORMAL:
                video_setup.flip = 0;
                break;
        case AMBA_VOUT_FLIP_HV:
                video_setup.flip = 1;
                break;
        case AMBA_VOUT_FLIP_HORIZONTAL:
                video_setup.flip = 2;
                break;
        case AMBA_VOUT_FLIP_VERTICAL:
                video_setup.flip = 3;
                break;
        default:
                vout_err("%s can't support flip[%d]!\n",
                         __func__, sink_mode->video_flip);
                return -EINVAL;
        }
        switch (sink_mode->video_rotate) {
        case AMBA_VOUT_ROTATE_NORMAL:
                video_setup.rotate = 0;
                break;

        case AMBA_VOUT_ROTATE_90:
                video_setup.rotate = 1;
                break;

        default:
                vout_info("%s can't support rotate[%d]!\n",
                          __func__, sink_mode->video_rotate);
                return -EINVAL;
        }

        rval = amba_s2l_vout_set_setup(psrc, &video_setup);
        if(rval)
                return rval;

        return rval;
}

int amba_s2l_vout_digital_init(struct __amba_vout_video_source *psrc,
                               struct amba_video_sink_mode *sink_mode)
{
        int     i, mod_index, rval = 0;

        psrc->active_sink_type = AMBA_VOUT_SINK_TYPE_DIGITAL;
        for(i = 0; i < VDMODE_NUM; i++) {
                if((sink_mode->mode == Vout_digital_mode[i].vmode) &&
                    (sink_mode->type == Vout_digital_mode[i].type))
                        break;
                if(i >= (VDMODE_NUM - 1)) {
                        vout_err("vout_digital don't support mode %d \n", sink_mode->mode);
                        return -EINVAL;
                }
        }
        mod_index = i;

        if (sink_mode->ratio == AMBA_VIDEO_RATIO_AUTO)
                sink_mode->ratio = Vout_digital_mode[mod_index].ratio;
        if (sink_mode->bits == AMBA_VIDEO_BITS_AUTO)
                sink_mode->bits = Vout_digital_mode[mod_index].bits;
        if (sink_mode->type == AMBA_VIDEO_TYPE_AUTO)
                sink_mode->type = Vout_digital_mode[mod_index].type;
        if (sink_mode->format == AMBA_VIDEO_FORMAT_AUTO)
                sink_mode->format = Vout_digital_mode[mod_index].format;

        if((sink_mode->type == AMBA_VIDEO_TYPE_RGB_601) ||
            (sink_mode->type == AMBA_VIDEO_TYPE_RGB_656) ||
            (sink_mode->type == AMBA_VIDEO_TYPE_RGB_RAW) ||
            (sink_mode->type == AMBA_VIDEO_TYPE_RGB_BT1120))
                rval = ambdbus_init_rgb_mode(psrc, sink_mode, mod_index);
        else
                rval = ambdbus_init_yuv_mode(psrc, sink_mode, mod_index);
        return rval;
}


