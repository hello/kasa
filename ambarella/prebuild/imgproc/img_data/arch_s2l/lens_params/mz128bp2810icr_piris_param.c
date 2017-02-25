/*
 * prebuild/imgproc/img_data/arch_s2l/lens_params/mz128bp2810icr_piris_param.c
 *
 * History:
 *    2014/12/16 - [Peter Jiao] Port from m13vp288ir_piris_param.c
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

/* ========================================================================== */

#include "img_adv_struct_arch.h"


/* This p-iris Dgain table is not needed for S2L AE */
/*
const lens_piris_dgain_table_t MZ128BP2810ICR_PIRIS_DGAIN = {
  .header = {
	PIRIS_DGAIN_TABLE,
	0,
  },
  .table = {

  },
};
*/

/*   FNO total scope from lens piris spec table   */
/* {FNO_MIN*65536, FNO_MAX*65536} (2 phase full step drive) */
const lens_piris_fno_scope_t MZ128BP2810ICR_PIRIS_SCOPE = {
  .header = {
	PIRIS_FNO_SCOPE,
	2,
  },
  .table = {0x1999A, 0x484000},
};


/* This is p-iris STEP POSITION table (1-2 phase drive) */
/* FNO = fno_min * pow(alpha, index), fno_min=1.6, alpha=pow(10.0, (3.0/640.0)) */
const lens_piris_step_table_t MZ128BP2810ICR_PIRIS_STEP = {
  .header = {
	 PIRIS_STEP_TABLE,
	 354,
  },
  .table = {
         42,       /* index 0, FNO 1.600000 */
         43,       /* index 1, FNO 1.617363 */
         45,       /* index 2, FNO 1.634914 */
         47,       /* index 3, FNO 1.652656 */
         49,       /* index 4, FNO 1.670590 */
         51,       /* index 5, FNO 1.688719 */
         53,       /* index 6, FNO 1.707045 */
         54,       /* index 7, FNO 1.725570 */
         55,       /* index 8, FNO 1.744295 */
         57,       /* index 9, FNO 1.763224 */
         58,       /* index 10, FNO 1.782358 */
         59,       /* index 11, FNO 1.801700 */
         61,       /* index 12, FNO 1.821252 */
         62,       /* index 13, FNO 1.841016 */
         63,       /* index 14, FNO 1.860994 */
         65,       /* index 15, FNO 1.881189 */
         66,       /* index 16, FNO 1.901604 */
         67,       /* index 17, FNO 1.922239 */
         69,       /* index 18, FNO 1.943099 */
         70,       /* index 19, FNO 1.964185 */
         71,       /* index 20, FNO 1.985500 */
         72,       /* index 21, FNO 2.007047 */
         73,       /* index 22, FNO 2.028827 */
         75,       /* index 23, FNO 2.050843 */
         76,       /* index 24, FNO 2.073099 */
         77,       /* index 25, FNO 2.095596 */
         78,       /* index 26, FNO 2.118337 */
         79,       /* index 27, FNO 2.141324 */
         80,       /* index 28, FNO 2.164562 */
         81,       /* index 29, FNO 2.188051 */
         82,       /* index 30, FNO 2.211796 */
         83,       /* index 31, FNO 2.235798 */
         84,       /* index 32, FNO 2.260060 */
         85,       /* index 33, FNO 2.284586 */
         86,       /* index 34, FNO 2.309378 */
         87,       /* index 35, FNO 2.334439 */
         88,       /* index 36, FNO 2.359772 */
         89,       /* index 37, FNO 2.385380 */
         89,       /* index 38, FNO 2.411265 */
         90,       /* index 39, FNO 2.437432 */
         91,       /* index 40, FNO 2.463882 */
         92,       /* index 41, FNO 2.490620 */
         93,       /* index 42, FNO 2.517648 */
         94,       /* index 43, FNO 2.544969 */
         95,       /* index 44, FNO 2.572586 */
         96,       /* index 45, FNO 2.600504 */
         96,       /* index 46, FNO 2.628724 */
         97,       /* index 47, FNO 2.657250 */
         98,       /* index 48, FNO 2.686086 */
         99,       /* index 49, FNO 2.715235 */
         100,       /* index 50, FNO 2.744701 */
         100,       /* index 51, FNO 2.774486 */
         101,       /* index 52, FNO 2.804594 */
         102,       /* index 53, FNO 2.835029 */
         103,       /* index 54, FNO 2.865794 */
         104,       /* index 55, FNO 2.896893 */
         104,       /* index 56, FNO 2.928330 */
         105,       /* index 57, FNO 2.960107 */
         106,       /* index 58, FNO 2.992230 */
         106,       /* index 59, FNO 3.024701 */
         107,       /* index 60, FNO 3.057525 */
         108,       /* index 61, FNO 3.090704 */
         109,       /* index 62, FNO 3.124244 */
         109,       /* index 63, FNO 3.158148 */
         110,       /* index 64, FNO 3.192420 */
         111,       /* index 65, FNO 3.227063 */
         111,       /* index 66, FNO 3.262083 */
         112,       /* index 67, FNO 3.297482 */
         113,       /* index 68, FNO 3.333266 */
         113,       /* index 69, FNO 3.369438 */
         114,       /* index 70, FNO 3.406003 */
         115,       /* index 71, FNO 3.442964 */
         115,       /* index 72, FNO 3.480326 */
         116,       /* index 73, FNO 3.518094 */
         117,       /* index 74, FNO 3.556272 */
         117,       /* index 75, FNO 3.594864 */
         118,       /* index 76, FNO 3.633875 */
         118,       /* index 77, FNO 3.673309 */
         119,       /* index 78, FNO 3.713171 */
         120,       /* index 79, FNO 3.753466 */
         120,       /* index 80, FNO 3.794198 */
         121,       /* index 81, FNO 3.835372 */
         121,       /* index 82, FNO 3.876993 */
         122,       /* index 83, FNO 3.919065 */
         122,       /* index 84, FNO 3.961594 */
         123,       /* index 85, FNO 4.004585 */
         124,       /* index 86, FNO 4.048042 */
         124,       /* index 87, FNO 4.091970 */
         125,       /* index 88, FNO 4.136376 */
         125,       /* index 89, FNO 4.181263 */
         126,       /* index 90, FNO 4.226637 */
         126,       /* index 91, FNO 4.272504 */
         127,       /* index 92, FNO 4.318869 */
         127,       /* index 93, FNO 4.365736 */
         128,       /* index 94, FNO 4.413112 */
         128,       /* index 95, FNO 4.461003 */
         129,       /* index 96, FNO 4.509413 */
         129,       /* index 97, FNO 4.558348 */
         130,       /* index 98, FNO 4.607814 */
         130,       /* index 99, FNO 4.657818 */
         131,       /* index 100, FNO 4.708363 */
         131,       /* index 101, FNO 4.759458 */
         132,       /* index 102, FNO 4.811107 */
         132,       /* index 103, FNO 4.863316 */
         133,       /* index 104, FNO 4.916092 */
         133,       /* index 105, FNO 4.969440 */
         133,       /* index 106, FNO 5.023368 */
         134,       /* index 107, FNO 5.077881 */
         134,       /* index 108, FNO 5.132985 */
         135,       /* index 109, FNO 5.188687 */
         135,       /* index 110, FNO 5.244994 */
         136,       /* index 111, FNO 5.301912 */
         136,       /* index 112, FNO 5.359447 */
         136,       /* index 113, FNO 5.417607 */
         137,       /* index 114, FNO 5.476398 */
         137,       /* index 115, FNO 5.535827 */
         138,       /* index 116, FNO 5.595900 */
         138,       /* index 117, FNO 5.656626 */
         139,       /* index 118, FNO 5.718011 */
         139,       /* index 119, FNO 5.780062 */
         139,       /* index 120, FNO 5.842786 */
         140,       /* index 121, FNO 5.906191 */
         140,       /* index 122, FNO 5.970284 */
         140,       /* index 123, FNO 6.035072 */
         141,       /* index 124, FNO 6.100564 */
         141,       /* index 125, FNO 6.166766 */
         142,       /* index 126, FNO 6.233687 */
         142,       /* index 127, FNO 6.301334 */
         142,       /* index 128, FNO 6.369715 */
         143,       /* index 129, FNO 6.438838 */
         143,       /* index 130, FNO 6.508711 */
         143,       /* index 131, FNO 6.579342 */
         144,       /* index 132, FNO 6.650740 */
         144,       /* index 133, FNO 6.722913 */
         144,       /* index 134, FNO 6.795869 */
         145,       /* index 135, FNO 6.869616 */
         145,       /* index 136, FNO 6.944164 */
         145,       /* index 137, FNO 7.019521 */
         146,       /* index 138, FNO 7.095696 */
         146,       /* index 139, FNO 7.172697 */
         146,       /* index 140, FNO 7.250534 */
         147,       /* index 141, FNO 7.329215 */
         147,       /* index 142, FNO 7.408751 */
         147,       /* index 143, FNO 7.489149 */
         148,       /* index 144, FNO 7.570420 */
         148,       /* index 145, FNO 7.652573 */
         148,       /* index 146, FNO 7.735617 */
         148,       /* index 147, FNO 7.819563 */
         149,       /* index 148, FNO 7.904420 */
         149,       /* index 149, FNO 7.990197 */
         149,       /* index 150, FNO 8.076905 */
         150,       /* index 151, FNO 8.164554 */
         150,       /* index 152, FNO 8.253155 */
         150,       /* index 153, FNO 8.342716 */
         150,       /* index 154, FNO 8.433250 */
         151,       /* index 155, FNO 8.524766 */
         151,       /* index 156, FNO 8.617276 */
         151,       /* index 157, FNO 8.710789 */
         152,       /* index 158, FNO 8.805317 */
         152,       /* index 159, FNO 8.900871 */
         152,       /* index 160, FNO 8.997461 */
         152,       /* index 161, FNO 9.095100 */
         152,       /* index 162, FNO 9.193799 */
         152,       /* index 163, FNO 9.293568 */
         152,       /* index 164, FNO 9.394420 */
         152,       /* index 165, FNO 9.496367 */
         152,       /* index 166, FNO 9.599420 */
         153,       /* index 167, FNO 9.703591 */
         153,       /* index 168, FNO 9.808893 */
         153,       /* index 169, FNO 9.915337 */
         153,       /* index 170, FNO 10.022937 */
         153,       /* index 171, FNO 10.131704 */
         153,       /* index 172, FNO 10.241651 */
         153,       /* index 173, FNO 10.352792 */
         153,       /* index 174, FNO 10.465139 */
         153,       /* index 175, FNO 10.578704 */
         153,       /* index 176, FNO 10.693503 */
         153,       /* index 177, FNO 10.809547 */
         153,       /* index 178, FNO 10.926850 */
         153,       /* index 179, FNO 11.045426 */
         153,       /* index 180, FNO 11.165289 */
         153,       /* index 181, FNO 11.286453 */
         154,       /* index 182, FNO 11.408932 */
         154,       /* index 183, FNO 11.532740 */
         154,       /* index 184, FNO 11.657891 */
         154,       /* index 185, FNO 11.784400 */
         154,       /* index 186, FNO 11.912282 */
         154,       /* index 187, FNO 12.041552 */
         154,       /* index 188, FNO 12.172225 */
         154,       /* index 189, FNO 12.304316 */
         154,       /* index 190, FNO 12.437840 */
         154,       /* index 191, FNO 12.572814 */
         154,       /* index 192, FNO 12.709252 */
         154,       /* index 193, FNO 12.847170 */
         154,       /* index 194, FNO 12.986586 */
         155,       /* index 195, FNO 13.127514 */
         155,       /* index 196, FNO 13.269971 */
         155,       /* index 197, FNO 13.413975 */
         155,       /* index 198, FNO 13.559541 */
         155,       /* index 199, FNO 13.706687 */
         155,       /* index 200, FNO 13.855429 */
         155,       /* index 201, FNO 14.005786 */
         155,       /* index 202, FNO 14.157774 */
         155,       /* index 203, FNO 14.311412 */
         155,       /* index 204, FNO 14.466717 */
         155,       /* index 205, FNO 14.623707 */
         156,       /* index 206, FNO 14.782401 */
         156,       /* index 207, FNO 14.942817 */
         156,       /* index 208, FNO 15.104974 */
         156,       /* index 209, FNO 15.268891 */
         156,       /* index 210, FNO 15.434586 */
         156,       /* index 211, FNO 15.602079 */
         156,       /* index 212, FNO 15.771390 */
         156,       /* index 213, FNO 15.942539 */
         156,       /* index 214, FNO 16.115544 */
         156,       /* index 215, FNO 16.290428 */
         156,       /* index 216, FNO 16.467208 */
         156,       /* index 217, FNO 16.645908 */
         157,       /* index 218, FNO 16.826546 */
         157,       /* index 219, FNO 17.009145 */
         157,       /* index 220, FNO 17.193725 */
         157,       /* index 221, FNO 17.380309 */
         157,       /* index 222, FNO 17.568917 */
         157,       /* index 223, FNO 17.759571 */
         157,       /* index 224, FNO 17.952295 */
         157,       /* index 225, FNO 18.147110 */
         157,       /* index 226, FNO 18.344040 */
         157,       /* index 227, FNO 18.543106 */
         157,       /* index 228, FNO 18.744333 */
         157,       /* index 229, FNO 18.947743 */
         158,       /* index 230, FNO 19.153360 */
         158,       /* index 231, FNO 19.361209 */
         158,       /* index 232, FNO 19.571314 */
         158,       /* index 233, FNO 19.783698 */
         158,       /* index 234, FNO 19.998388 */
         158,       /* index 235, FNO 20.215407 */
         158,       /* index 236, FNO 20.434781 */
         158,       /* index 237, FNO 20.656535 */
         158,       /* index 238, FNO 20.880697 */
         158,       /* index 239, FNO 21.107290 */
         158,       /* index 240, FNO 21.336343 */
         158,       /* index 241, FNO 21.567881 */
         159,       /* index 242, FNO 21.801932 */
         159,       /* index 243, FNO 22.038523 */
         159,       /* index 244, FNO 22.277681 */
         159,       /* index 245, FNO 22.519435 */
         159,       /* index 246, FNO 22.763812 */
         159,       /* index 247, FNO 23.010841 */
         159,       /* index 248, FNO 23.260550 */
         159,       /* index 249, FNO 23.512970 */
         159,       /* index 250, FNO 23.768128 */
         159,       /* index 251, FNO 24.026056 */
         159,       /* index 252, FNO 24.286782 */
         159,       /* index 253, FNO 24.550338 */
         160,       /* index 254, FNO 24.816754 */
         160,       /* index 255, FNO 25.086062 */
         160,       /* index 256, FNO 25.358291 */
         160,       /* index 257, FNO 25.633475 */
         160,       /* index 258, FNO 25.911645 */
         160,       /* index 259, FNO 26.192834 */
         160,       /* index 260, FNO 26.477074 */
         160,       /* index 261, FNO 26.764398 */
         160,       /* index 262, FNO 27.054841 */
         160,       /* index 263, FNO 27.348435 */
         160,       /* index 264, FNO 27.645216 */
         161,       /* index 265, FNO 27.945217 */
         161,       /* index 266, FNO 28.248473 */
         161,       /* index 267, FNO 28.555021 */
         161,       /* index 268, FNO 28.864895 */
         161,       /* index 269, FNO 29.178132 */
         161,       /* index 270, FNO 29.494768 */
         161,       /* index 271, FNO 29.814840 */
         161,       /* index 272, FNO 30.138385 */
         161,       /* index 273, FNO 30.465442 */
         161,       /* index 274, FNO 30.796048 */
         161,       /* index 275, FNO 31.130241 */
         161,       /* index 276, FNO 31.468061 */
         162,       /* index 277, FNO 31.809547 */
         162,       /* index 278, FNO 32.154739 */
         162,       /* index 279, FNO 32.503676 */
         162,       /* index 280, FNO 32.856400 */
         162,       /* index 281, FNO 33.212952 */
         162,       /* index 282, FNO 33.573374 */
         162,       /* index 283, FNO 33.937706 */
         162,       /* index 284, FNO 34.305992 */
         162,       /* index 285, FNO 34.678275 */
         162,       /* index 286, FNO 35.054597 */
         162,       /* index 287, FNO 35.435004 */
         162,       /* index 288, FNO 35.819538 */
         163,       /* index 289, FNO 36.208246 */
         163,       /* index 290, FNO 36.601171 */
         163,       /* index 291, FNO 36.998361 */
         163,       /* index 292, FNO 37.399861 */
         163,       /* index 293, FNO 37.805717 */
         163,       /* index 294, FNO 38.215978 */
         163,       /* index 295, FNO 38.630692 */
         163,       /* index 296, FNO 39.049905 */
         163,       /* index 297, FNO 39.473668 */
         163,       /* index 298, FNO 39.902029 */
         163,       /* index 299, FNO 40.335039 */
         163,       /* index 300, FNO 40.772748 */
         164,       /* index 301, FNO 41.215207 */
         164,       /* index 302, FNO 41.662467 */
         164,       /* index 303, FNO 42.114581 */
         164,       /* index 304, FNO 42.571601 */
         164,       /* index 305, FNO 43.033581 */
         164,       /* index 306, FNO 43.500574 */
         164,       /* index 307, FNO 43.972634 */
         164,       /* index 308, FNO 44.449818 */
         164,       /* index 309, FNO 44.932179 */
         164,       /* index 310, FNO 45.419775 */
         164,       /* index 311, FNO 45.912663 */
         164,       /* index 312, FNO 46.410899 */
         165,       /* index 313, FNO 46.914542 */
         165,       /* index 314, FNO 47.423651 */
         165,       /* index 315, FNO 47.938284 */
         165,       /* index 316, FNO 48.458502 */
         165,       /* index 317, FNO 48.984365 */
         165,       /* index 318, FNO 49.515935 */
         165,       /* index 319, FNO 50.053273 */
         165,       /* index 320, FNO 50.596443 */
         165,       /* index 321, FNO 51.145506 */
         165,       /* index 322, FNO 51.700529 */
         165,       /* index 323, FNO 52.261574 */
         165,       /* index 324, FNO 52.828707 */
         166,       /* index 325, FNO 53.401995 */
         166,       /* index 326, FNO 53.981504 */
         166,       /* index 327, FNO 54.567302 */
         166,       /* index 328, FNO 55.159457 */
         166,       /* index 329, FNO 55.758038 */
         166,       /* index 330, FNO 56.363114 */
         166,       /* index 331, FNO 56.974757 */
         166,       /* index 332, FNO 57.593037 */
         166,       /* index 333, FNO 58.218027 */
         166,       /* index 334, FNO 58.849799 */
         166,       /* index 335, FNO 59.488427 */
         167,       /* index 336, FNO 60.133985 */
         167,       /* index 337, FNO 60.786548 */
         167,       /* index 338, FNO 61.446193 */
         167,       /* index 339, FNO 62.112997 */
         167,       /* index 340, FNO 62.787036 */
         167,       /* index 341, FNO 63.468390 */
         167,       /* index 342, FNO 64.157138 */
         167,       /* index 343, FNO 64.853360 */
         167,       /* index 344, FNO 65.557138 */
         167,       /* index 345, FNO 66.268552 */
         167,       /* index 346, FNO 66.987687 */
         167,       /* index 347, FNO 67.714626 */
         168,       /* index 348, FNO 68.449453 */
         168,       /* index 349, FNO 69.192255 */
         168,       /* index 350, FNO 69.943117 */
         168,       /* index 351, FNO 70.702128 */
         168,       /* index 352, FNO 71.469375 */
         168,       /* index 353, FNO 72.244948 */
  },
};


