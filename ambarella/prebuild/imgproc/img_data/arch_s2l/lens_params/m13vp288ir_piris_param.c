/*
 * prebuild/imgproc/img_data/arch_s2l/lens_params/m13vp288ir_piris_param.c
 *
 * History:
 *    2014/06/19 - [Peter Jiao] Create
 *    2014/08/05 - [Peter Jiao] Add Header
 *	 2014/11/10 - [Peter Jiao] Port to S2L
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
const lens_piris_dgain_table_t M13VP288IR_PIRIS_DGAIN = {
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
const lens_piris_fno_scope_t M13VP288IR_PIRIS_SCOPE = {
  .header = {
	PIRIS_FNO_SCOPE,
	2,
  },
  .table = {0x12E14, 0x40B4A2},
};


/* This is p-iris STEP POSITION table (1-2 phase drive) */
/* FNO = fno_min * pow(alpha, index), fno_min=1.180, alpha=pow(10.0, (3.0/640.0)) */
const lens_piris_step_table_t M13VP288IR_PIRIS_STEP = {
  .header = {
	 PIRIS_STEP_TABLE,
	 372,
  },
  .table = {
	 6, 	 /* index 0, FNO 1.180000 -*/
	 9, 	 /* index 1, FNO 1.192805 -*/
	 11, 	 /* index 2, FNO 1.205749 -*/
	 13, 	 /* index 3, FNO 1.218834 -*/
	 15, 	 /* index 4, FNO 1.232060 -*/
	 18, 	 /* index 5, FNO 1.245431 -*/
	 19, 	 /* index 6, FNO 1.258946 -*/
	 22, 	 /* index 7, FNO 1.272608 -*/
	 24, 	 /* index 8, FNO 1.286418 -*/
	 26, 	 /* index 9, FNO 1.300378 -*/
	 27, 	 /* index 10, FNO 1.314489 -*/
	 29, 	 /* index 11, FNO 1.328754 -*/
	 31, 	 /* index 12, FNO 1.343173 -*/
	 33, 	 /* index 13, FNO 1.357749 -*/
	 35, 	 /* index 14, FNO 1.372483 -*/
	 36, 	 /* index 15, FNO 1.387377 -*/
	 38, 	 /* index 16, FNO 1.402433 -*/
	 39, 	 /* index 17, FNO 1.417652 -*/
	 41, 	 /* index 18, FNO 1.433036 -*/
	 42, 	 /* index 19, FNO 1.448587 -*/
	 44, 	 /* index 20, FNO 1.464307 -*/
	 45, 	 /* index 21, FNO 1.480197 -*/
	 47, 	 /* index 22, FNO 1.496260 -*/
	 48, 	 /* index 23, FNO 1.512497 -*/
	 49, 	 /* index 24, FNO 1.528910 -*/
	 51, 	 /* index 25, FNO 1.545502 -*/
	 52, 	 /* index 26, FNO 1.562273 -*/
	 53, 	 /* index 27, FNO 1.579227 -*/
	 55, 	 /* index 28, FNO 1.596364 -*/
	 56, 	 /* index 29, FNO 1.613688 -*/
	 57, 	 /* index 30, FNO 1.631199 -*/
	 58, 	 /* index 31, FNO 1.648901 -*/
	 60, 	 /* index 32, FNO 1.666794 -*/
	 61, 	 /* index 33, FNO 1.684882 -*/
	 62, 	 /* index 34, FNO 1.703166 -*/
	 63, 	 /* index 35, FNO 1.721649 -*/
	 64, 	 /* index 36, FNO 1.740332 -*/
	 65, 	 /* index 37, FNO 1.759217 -*/
	 66, 	 /* index 38, FNO 1.778308 -*/
	 67, 	 /* index 39, FNO 1.797606 -*/
	 68, 	 /* index 40, FNO 1.817113 -*/
	 69, 	 /* index 41, FNO 1.836832 -*/
	 71, 	 /* index 42, FNO 1.856765 -*/
	 71, 	 /* index 43, FNO 1.876915 -*/
	 72, 	 /* index 44, FNO 1.897283 -*/
	 73, 	 /* index 45, FNO 1.917872 -*/
	 74, 	 /* index 46, FNO 1.938684 -*/
	 75, 	 /* index 47, FNO 1.959722 -*/
	 76, 	 /* index 48, FNO 1.980989 -*/
	 77, 	 /* index 49, FNO 2.002486 -*/
	 78, 	 /* index 50, FNO 2.024217 -*/
	 79, 	 /* index 51, FNO 2.046183 -*/
	 80, 	 /* index 52, FNO 2.068388 -*/
	 80, 	 /* index 53, FNO 2.090834 -*/
	 81, 	 /* index 54, FNO 2.113523 -*/
	 82, 	 /* index 55, FNO 2.136459 -*/
	 83, 	 /* index 56, FNO 2.159643 -*/
	 84, 	 /* index 57, FNO 2.183079 -*/
	 84, 	 /* index 58, FNO 2.206770 -*/
	 85, 	 /* index 59, FNO 2.230717 -*/
	 86, 	 /* index 60, FNO 2.254925 -*/
	 87, 	 /* index 61, FNO 2.279395 -*/
	 88, 	 /* index 62, FNO 2.304130 -*/
	 88, 	 /* index 63, FNO 2.329134 -*/
	 89, 	 /* index 64, FNO 2.354410 -*/
	 90, 	 /* index 65, FNO 2.379959 -*/
	 90, 	 /* index 66, FNO 2.405786 -*/
	 91, 	 /* index 67, FNO 2.431893 -*/
	 92, 	 /* index 68, FNO 2.458284 -*/
	 93, 	 /* index 69, FNO 2.484961 -*/
	 93, 	 /* index 70, FNO 2.511927 -*/
	 94, 	 /* index 71, FNO 2.539186 -*/
	 95, 	 /* index 72, FNO 2.566741 -*/
	 95, 	 /* index 73, FNO 2.594595 -*/
	 96, 	 /* index 74, FNO 2.622751 -*/
	 96, 	 /* index 75, FNO 2.651212 -*/
	 97, 	 /* index 76, FNO 2.679983 -*/
	 98, 	 /* index 77, FNO 2.709066 -*/
	 98, 	 /* index 78, FNO 2.738464 -*/
	 99, 	 /* index 79, FNO 2.768181 -*/
	 99, 	 /* index 80, FNO 2.798221 -*/
	 100, 	 /* index 81, FNO 2.828587 -*/
	 101, 	 /* index 82, FNO 2.859282 -*/
	 101, 	 /* index 83, FNO 2.890311 -*/
	 102, 	 /* index 84, FNO 2.921676 -*/
	 102, 	 /* index 85, FNO 2.953381 -*/
	 103, 	 /* index 86, FNO 2.985431 -*/
	 104, 	 /* index 87, FNO 3.017828 -*/
	 104, 	 /* index 88, FNO 3.050577 -*/
	 105, 	 /* index 89, FNO 3.083681 -*/
	 105, 	 /* index 90, FNO 3.117145 -*/
	 106, 	 /* index 91, FNO 3.150972 -*/
	 106, 	 /* index 92, FNO 3.185166 -*/
	 107, 	 /* index 93, FNO 3.219730 -*/
	 107, 	 /* index 94, FNO 3.254670 -*/
	 108, 	 /* index 95, FNO 3.289989 -*/
	 108, 	 /* index 96, FNO 3.325692 -*/
	 109, 	 /* index 97, FNO 3.361782 -*/
	 109, 	 /* index 98, FNO 3.398263 -*/
	 110, 	 /* index 99, FNO 3.435141 -*/
	 110, 	 /* index 100, FNO 3.472418 -*/
	 111, 	 /* index 101, FNO 3.510100 -*/
	 111, 	 /* index 102, FNO 3.548191 -*/
	 112, 	 /* index 103, FNO 3.586695 -*/
	 112, 	 /* index 104, FNO 3.625618 -*/
	 113, 	 /* index 105, FNO 3.664962 -*/
	 113, 	 /* index 106, FNO 3.704734 -*/
	 114, 	 /* index 107, FNO 3.744937 -*/
	 114, 	 /* index 108, FNO 3.785576 -*/
	 114, 	 /* index 109, FNO 3.826657 -*/
	 115, 	 /* index 110, FNO 3.868183 -*/
	 115, 	 /* index 111, FNO 3.910160 -*/
	 116, 	 /* index 112, FNO 3.952592 -*/
	 116, 	 /* index 113, FNO 3.995485 -*/
	 117, 	 /* index 114, FNO 4.038843 -*/
	 117, 	 /* index 115, FNO 4.082672 -*/
	 118, 	 /* index 116, FNO 4.126977 -*/
	 118, 	 /* index 117, FNO 4.171762 -*/
	 118, 	 /* index 118, FNO 4.217033 -*/
	 119, 	 /* index 119, FNO 4.262796 -*/
	 119, 	 /* index 120, FNO 4.309055 -*/
	 120, 	 /* index 121, FNO 4.355816 -*/
	 120, 	 /* index 122, FNO 4.403084 -*/
	 121, 	 /* index 123, FNO 4.450866 -*/
	 121, 	 /* index 124, FNO 4.499166 -*/
	 121, 	 /* index 125, FNO 4.547990 -*/
	 122, 	 /* index 126, FNO 4.597344 -*/
	 122, 	 /* index 127, FNO 4.647234 -*/
	 123, 	 /* index 128, FNO 4.697665 -*/
	 123, 	 /* index 129, FNO 4.748643 -*/
	 123, 	 /* index 130, FNO 4.800174 -*/
	 124, 	 /* index 131, FNO 4.852265 -*/
	 124, 	 /* index 132, FNO 4.904921 -*/
	 125, 	 /* index 133, FNO 4.958148 -*/
	 125, 	 /* index 134, FNO 5.011953 -*/
	 125, 	 /* index 135, FNO 5.066342 -*/
	 126, 	 /* index 136, FNO 5.121321 -*/
	 126, 	 /* index 137, FNO 5.176897 -*/
	 127, 	 /* index 138, FNO 5.233076 -*/
	 127, 	 /* index 139, FNO 5.289864 -*/
	 127, 	 /* index 140, FNO 5.347269 -*/
	 128, 	 /* index 141, FNO 5.405296 -*/
	 128, 	 /* index 142, FNO 5.463954 -*/
	 128, 	 /* index 143, FNO 5.523248 -*/
	 129, 	 /* index 144, FNO 5.583185 -*/
	 129, 	 /* index 145, FNO 5.643773 -*/
	 129, 	 /* index 146, FNO 5.705018 -*/
	 130, 	 /* index 147, FNO 5.766928 -*/
	 130, 	 /* index 148, FNO 5.829509 -*/
	 131, 	 /* index 149, FNO 5.892770 -*/
	 131, 	 /* index 150, FNO 5.956718 -*/
	 131, 	 /* index 151, FNO 6.021359 -*/
	 132, 	 /* index 152, FNO 6.086702 -*/
	 132, 	 /* index 153, FNO 6.152753 -*/
	 132, 	 /* index 154, FNO 6.219522 -*/
	 133, 	 /* index 155, FNO 6.287015 -*/
	 133, 	 /* index 156, FNO 6.355241 -*/
	 133, 	 /* index 157, FNO 6.424207 -*/
	 134, 	 /* index 158, FNO 6.493921 -*/
	 134, 	 /* index 159, FNO 6.564392 -*/
	 134, 	 /* index 160, FNO 6.635628 -*/
	 135, 	 /* index 161, FNO 6.707636 -*/
	 135, 	 /* index 162, FNO 6.780426 -*/
	 135, 	 /* index 163, FNO 6.854006 -*/
	 136, 	 /* index 164, FNO 6.928385 -*/
	 136, 	 /* index 165, FNO 7.003571 -*/
	 136, 	 /* index 166, FNO 7.079572 -*/
	 136, 	 /* index 167, FNO 7.156398 -*/
	 137, 	 /* index 168, FNO 7.234058 -*/
	 137, 	 /* index 169, FNO 7.312561 -*/
	 137, 	 /* index 170, FNO 7.391916 -*/
	 138, 	 /* index 171, FNO 7.472132 -*/
	 138, 	 /* index 172, FNO 7.553218 -*/
	 138, 	 /* index 173, FNO 7.635184 -*/
	 139, 	 /* index 174, FNO 7.718040 -*/
	 139, 	 /* index 175, FNO 7.801795 -*/
	 139, 	 /* index 176, FNO 7.886458 -*/
	 139, 	 /* index 177, FNO 7.972041 -*/
	 140, 	 /* index 178, FNO 8.058552 -*/
	 140, 	 /* index 179, FNO 8.146002 -*/
	 140, 	 /* index 180, FNO 8.234401 -*/
	 141, 	 /* index 181, FNO 8.323759 -*/
	 141, 	 /* index 182, FNO 8.414087 -*/
	 141, 	 /* index 183, FNO 8.505395 -*/
	 141, 	 /* index 184, FNO 8.597694 -*/
	 142, 	 /* index 185, FNO 8.690995 -*/
	 142, 	 /* index 186, FNO 8.785308 -*/
	 142, 	 /* index 187, FNO 8.880645 -*/
	 142, 	 /* index 188, FNO 8.977016 -*/
	 143, 	 /* index 189, FNO 9.074433 -*/
	 143, 	 /* index 190, FNO 9.172907 -*/
	 143, 	 /* index 191, FNO 9.272450 -*/
	 143, 	 /* index 192, FNO 9.373073 -*/
	 144, 	 /* index 193, FNO 9.474788 -*/
	 144, 	 /* index 194, FNO 9.577607 -*/
	 144, 	 /* index 195, FNO 9.681541 -*/
	 144, 	 /* index 196, FNO 9.786604 -*/
	 145, 	 /* index 197, FNO 9.892806 -*/
	 145, 	 /* index 198, FNO 10.000161 -*/
	 145, 	 /* index 199, FNO 10.108681 -*/
	 145, 	 /* index 200, FNO 10.218379 -*/
	 146, 	 /* index 201, FNO 10.329267 -*/
	 146, 	 /* index 202, FNO 10.441358 -*/
	 146, 	 /* index 203, FNO 10.554666 -*/
	 146, 	 /* index 204, FNO 10.669204 -*/
	 147, 	 /* index 205, FNO 10.784984 -*/
	 147, 	 /* index 206, FNO 10.902021 -*/
	 147, 	 /* index 207, FNO 11.020328 -*/
	 147, 	 /* index 208, FNO 11.139918 -*/
	 147, 	 /* index 209, FNO 11.260807 -*/
	 148, 	 /* index 210, FNO 11.383007 -*/
	 148, 	 /* index 211, FNO 11.506534 -*/
	 148, 	 /* index 212, FNO 11.631400 -*/
	 148, 	 /* index 213, FNO 11.757622 -*/
	 148, 	 /* index 214, FNO 11.885214 -*/
	 149, 	 /* index 215, FNO 12.014190 -*/
	 149, 	 /* index 216, FNO 12.144566 -*/
	 149, 	 /* index 217, FNO 12.276357 -*/
	 149, 	 /* index 218, FNO 12.409578 -*/
	 149, 	 /* index 219, FNO 12.544244 -*/
	 150, 	 /* index 220, FNO 12.680372 -*/
	 150, 	 /* index 221, FNO 12.817978 -*/
	 150, 	 /* index 222, FNO 12.957076 -*/
	 150, 	 /* index 223, FNO 13.097684 -*/
	 150, 	 /* index 224, FNO 13.239818 -*/
	 151, 	 /* index 225, FNO 13.383494 -*/
	 151, 	 /* index 226, FNO 13.528729 -*/
	 151, 	 /* index 227, FNO 13.675541 -*/
	 151, 	 /* index 228, FNO 13.823945 -*/
	 151, 	 /* index 229, FNO 13.973960 -*/
	 152, 	 /* index 230, FNO 14.125603 -*/
	 152, 	 /* index 231, FNO 14.278892 -*/
	 152, 	 /* index 232, FNO 14.433844 -*/
	 152, 	 /* index 233, FNO 14.590478 -*/
	 152, 	 /* index 234, FNO 14.748811 -*/
	 152, 	 /* index 235, FNO 14.908862 -*/
	 153, 	 /* index 236, FNO 15.070651 -*/
	 153, 	 /* index 237, FNO 15.234195 -*/
	 153, 	 /* index 238, FNO 15.399514 -*/
	 153, 	 /* index 239, FNO 15.566627 -*/
	 153, 	 /* index 240, FNO 15.735553 -*/
	 153, 	 /* index 241, FNO 15.906312 -*/
	 154, 	 /* index 242, FNO 16.078925 -*/
	 154, 	 /* index 243, FNO 16.253411 -*/
	 154, 	 /* index 244, FNO 16.429790 -*/
	 154, 	 /* index 245, FNO 16.608083 -*/
	 154, 	 /* index 246, FNO 16.788311 -*/
	 154, 	 /* index 247, FNO 16.970495 -*/
	 154, 	 /* index 248, FNO 17.154656 -*/
	 155, 	 /* index 249, FNO 17.340815 -*/
	 155, 	 /* index 250, FNO 17.528995 -*/
	 155, 	 /* index 251, FNO 17.719216 -*/
	 155, 	 /* index 252, FNO 17.911502 -*/
	 155, 	 /* index 253, FNO 18.105875 -*/
	 155, 	 /* index 254, FNO 18.302356 -*/
	 155, 	 /* index 255, FNO 18.500970 -*/
	 156, 	 /* index 256, FNO 18.701740 -*/
	 156, 	 /* index 257, FNO 18.904688 -*/
	 156, 	 /* index 258, FNO 19.109838 -*/
	 156, 	 /* index 259, FNO 19.317215 -*/
	 156, 	 /* index 260, FNO 19.526842 -*/
	 156, 	 /* index 261, FNO 19.738744 -*/
	 156, 	 /* index 262, FNO 19.952945 -*/
	 156, 	 /* index 263, FNO 20.169471 -*/
	 157, 	 /* index 264, FNO 20.388347 -*/
	 157, 	 /* index 265, FNO 20.609597 -*/
	 157, 	 /* index 266, FNO 20.833249 -*/
	 157, 	 /* index 267, FNO 21.059328 -*/
	 157, 	 /* index 268, FNO 21.287860 -*/
	 157, 	 /* index 269, FNO 21.518872 -*/
	 157, 	 /* index 270, FNO 21.752391 -*/
	 158, 	 /* index 271, FNO 21.988444 -*/
	 158, 	 /* index 272, FNO 22.227059 -*/
	 158, 	 /* index 273, FNO 22.468263 -*/
	 158, 	 /* index 274, FNO 22.712085 -*/
	 158, 	 /* index 275, FNO 22.958553 -*/
	 158, 	 /* index 276, FNO 23.207695 -*/
	 158, 	 /* index 277, FNO 23.459541 -*/
	 158, 	 /* index 278, FNO 23.714120 -*/
	 158, 	 /* index 279, FNO 23.971461 -*/
	 159, 	 /* index 280, FNO 24.231595 -*/
	 159, 	 /* index 281, FNO 24.494552 -*/
	 159, 	 /* index 282, FNO 24.760363 -*/
	 159, 	 /* index 283, FNO 25.029058 -*/
	 159, 	 /* index 284, FNO 25.300669 -*/
	 159, 	 /* index 285, FNO 25.575228 -*/
	 159, 	 /* index 286, FNO 25.852766 -*/
	 159, 	 /* index 287, FNO 26.133315 -*/
	 159, 	 /* index 288, FNO 26.416909 -*/
	 159, 	 /* index 289, FNO 26.703581 -*/
	 159, 	 /* index 290, FNO 26.993364 -*/
	 160, 	 /* index 291, FNO 27.286291 -*/
	 160, 	 /* index 292, FNO 27.582397 -*/
	 160, 	 /* index 293, FNO 27.881717 -*/
	 160, 	 /* index 294, FNO 28.184284 -*/
	 160, 	 /* index 295, FNO 28.490135 -*/
	 160, 	 /* index 296, FNO 28.799305 -*/
	 160, 	 /* index 297, FNO 29.111830 -*/
	 160, 	 /* index 298, FNO 29.427747 -*/
	 160, 	 /* index 299, FNO 29.747091 -*/
	 160, 	 /* index 300, FNO 30.069902 -*/
	 160, 	 /* index 301, FNO 30.396215 -*/
	 161, 	 /* index 302, FNO 30.726069 -*/
	 161, 	 /* index 303, FNO 31.059503 -*/
	 161, 	 /* index 304, FNO 31.396556 -*/
	 161, 	 /* index 305, FNO 31.737266 -*/
	 161, 	 /* index 306, FNO 32.081673 -*/
	 161, 	 /* index 307, FNO 32.429818 -*/
	 161, 	 /* index 308, FNO 32.781740 -*/
	 161, 	 /* index 309, FNO 33.137482 -*/
	 161, 	 /* index 310, FNO 33.497084 -*/
	 161, 	 /* index 311, FNO 33.860589 -*/
	 161, 	 /* index 312, FNO 34.228038 -*/
	 162, 	 /* index 313, FNO 34.599475 -*/
	 162, 	 /* index 314, FNO 34.974942 -*/
	 162, 	 /* index 315, FNO 35.354484 -*/
	 162, 	 /* index 316, FNO 35.738145 -*/
	 162, 	 /* index 317, FNO 36.125969 -*/
	 162, 	 /* index 318, FNO 36.518002 -*/
	 162, 	 /* index 319, FNO 36.914289 -*/
	 162, 	 /* index 320, FNO 37.314876 -*/
	 162, 	 /* index 321, FNO 37.719811 -*/
	 162, 	 /* index 322, FNO 38.129140 -*/
	 162, 	 /* index 323, FNO 38.542911 -*/
	 162, 	 /* index 324, FNO 38.961172 -*/
	 162, 	 /* index 325, FNO 39.383971 -*/
	 162, 	 /* index 326, FNO 39.811359 -*/
	 162, 	 /* index 327, FNO 40.243385 -*/
	 162, 	 /* index 328, FNO 40.680100 -*/
	 163, 	 /* index 329, FNO 41.121553 -*/
	 163, 	 /* index 330, FNO 41.567797 -*/
	 163, 	 /* index 331, FNO 42.018883 -*/
	 163, 	 /* index 332, FNO 42.474865 -*/
	 163, 	 /* index 333, FNO 42.935795 -*/
	 163, 	 /* index 334, FNO 43.401727 -*/
	 163, 	 /* index 335, FNO 43.872715 -*/
	 163, 	 /* index 336, FNO 44.348814 -*/
	 163, 	 /* index 337, FNO 44.830079 -*/
	 163, 	 /* index 338, FNO 45.316568 -*/
	 163, 	 /* index 339, FNO 45.808335 -*/
	 163, 	 /* index 340, FNO 46.305439 -*/
	 163, 	 /* index 341, FNO 46.807938 -*/
	 163, 	 /* index 342, FNO 47.315889 -*/
	 163, 	 /* index 343, FNO 47.829353 -*/
	 163, 	 /* index 344, FNO 48.348389 -*/
	 163, 	 /* index 345, FNO 48.873057 -*/
	 163, 	 /* index 346, FNO 49.403419 -*/
	 163, 	 /* index 347, FNO 49.939537 -*/
	 163, 	 /* index 348, FNO 50.481472 -*/
	 163, 	 /* index 349, FNO 51.029288 -*/
	 163, 	 /* index 350, FNO 51.583049 -*/
	 163, 	 /* index 351, FNO 52.142819 -*/
	 163, 	 /* index 352, FNO 52.708664 -*/
	 163, 	 /* index 353, FNO 53.280649 -*/
	 163, 	 /* index 354, FNO 53.858841 -*/
	 164, 	 /* index 355, FNO 54.443308 -*/
	 164, 	 /* index 356, FNO 55.034118 -*/
	 164, 	 /* index 357, FNO 55.631338 -*/
	 164, 	 /* index 358, FNO 56.235040 -*/
	 164, 	 /* index 359, FNO 56.845293 -*/
	 164, 	 /* index 360, FNO 57.462168 -*/
	 164, 	 /* index 361, FNO 58.085737 -*/
	 164, 	 /* index 362, FNO 58.716074 -*/
	 164, 	 /* index 363, FNO 59.353250 -*/
	 164, 	 /* index 364, FNO 59.997342 -*/
	 164, 	 /* index 365, FNO 60.648422 -*/
	 164, 	 /* index 366, FNO 61.306568 -*/
	 164, 	 /* index 367, FNO 61.971857 -*/
	 164, 	 /* index 368, FNO 62.644364 -*/
	 164, 	 /* index 369, FNO 63.324170 -*/
	 164, 	 /* index 370, FNO 64.011353 -*/
	 164, 	 /* index 371, FNO 64.705993 -*/
  },
};


