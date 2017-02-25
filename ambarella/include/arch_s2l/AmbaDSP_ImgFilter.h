/*
 * include/arch_s2l/img_abs_filter.h
 *
 * History:
 *	12/12/2012  - [Steve Chen] created file
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

#ifndef _AMBA_DSP_IMG_FILTER_H_
#define _AMBA_DSP_IMG_FILTER_H_

#include "AmbaDataType.h"
#include "AmbaDSP_ImgDef.h"

#define AMBA_DSP_IMG_NUM_EXPOSURE_CURVE         256
#define AMBA_DSP_IMG_NUM_TONE_CURVE             256
#define AMBA_DSP_IMG_NUM_CHROMA_GAIN_CURVE      128
#define AMBA_DSP_IMG_NUM_CORING_TABLE_INDEX     256
#define AMBA_DSP_IMG_NUM_MAX_FIR_COEFF          10
#define AMBA_DSP_IMG_NUM_VIDEO_MCTF             4

#define AMBA_DSP_IMG_MAX_HDR_EXPO_NUM	4

#define AMBA_DSP_IMG_CC_3D_SIZE                 (17536)
#define AMBA_DSP_IMG_SEC_CC_SIZE                (20608)
#define AMBA_DSP_IMG_CC_REG_SIZE                (18752)

#define AMBA_DSP_IMG_AWB_UNIT_SHIFT             12

#define AMBA_DSP_IMG_MCTF_CFG_SIZE              528
#define AMBA_DSP_IMG_CC_CFG_SIZE                20608
#define AMBA_DSP_IMG_CMPR_CFG_SIZE              544

#define AMBA_DSP_IMG_SHP_A_SELECT_ASF   0x0
#define AMBA_DSP_IMG_SHP_A_SELECT_SHP   0x1
#define AMBA_DSP_IMG_SHP_A_SELECT_DE_EDGE   0x2

typedef enum _AMBA_DSP_IMG_BAYER_PATTERN_e_ {
    AMBA_DSP_IMG_BAYER_PATTERN_RG = 0,
    AMBA_DSP_IMG_BAYER_PATTERN_BG,
    AMBA_DSP_IMG_BAYER_PATTERN_GR,
    AMBA_DSP_IMG_BAYER_PATTERN_GB
} AMBA_DSP_IMG_BAYER_PATTERN_e;

typedef struct amba_img_dsp_sensor_info_s {
    UINT8   SensorID;
    UINT8   NumFieldsPerFormat; /* maxumum 8 fields per frame */
    UINT8   SensorResolution;   /* Number of bits for data representation */
    UINT8   SensorPattern;      /* Bayer patterns RG, BG, GR, GB */
    UINT8   FirstLineField[8];
    UINT32  SensorReadOutMode;
} amba_img_dsp_sensor_info_t;

typedef struct amba_img_dsp_black_correction_s {
    INT16   BlackR;
    INT16   BlackGr;
    INT16   BlackGb;
    INT16   BlackB;
} amba_img_dsp_black_correction_t;

#define    AMBA_DSP_IMG_VIG_VER_1_0     0x20130218

typedef enum _AMBA_DSP_IMG_VIGNETTE_VIGSTRENGTHEFFECT_MODE_e_ {
    AMBA_DSP_IMG_VIGNETTE_DefaultMode = 0,
    AMBA_DSP_IMG_VIGNETTE_KeepRatioMode = 1,
} AMBA_DSP_IMG_VIGNETTE_VIGSTRENGTHEFFECT_MODE_e;

typedef struct _AMBA_DSP_IMG_CALIB_VIGNETTE_INFO_s_ {
    UINT32  Version;
    int     TableWidth;
    int     TableHeight;
    amba_img_dsp_vin_sensor_geometry_t  CalibVinSensorGeo;
    UINT32  Reserved;                   // Reserved for extention.
    UINT32  Reserved1;                  // Reserved for extention.
    UINT16  *pVignetteRedGain;          // Vignette table array addr.
    UINT16  *pVignetteGreenEvenGain;    // Vignette table array addr.
    UINT16  *pVignetteGreenOddGain;     // Vignette table array addr.
    UINT16  *pVignetteBlueGain;         // Vignette table array addr.
} AMBA_DSP_IMG_CALIB_VIGNETTE_INFO_s;

#define AMBA_IMG_DSP_VIGNETTE_CONTROL_VERT_FLIP    0x1  /* vertical flip. */
typedef struct amba_img_dsp_vignette_calc_info_s {
    UINT8   Enb;
    UINT8   GainShift;
    UINT8   VigStrengthEffectMode;
    UINT8   Control;
    UINT32  ChromaRatio;
    UINT32  VigStrength;
    amba_img_dsp_vin_sensor_geometry_t  CurrentVinSensorGeo;
    AMBA_DSP_IMG_CALIB_VIGNETTE_INFO_s  CalibVignetteInfo;
} amba_img_dsp_vignette_calc_info_t;

typedef struct _AMBA_DSP_IMG_BYPASS_VIGNETTE_INFO_s_ {
    UINT8   Enable;
    UINT16  GainShift;
    UINT16  *pRedGain;             // Pointer to one of tables in gain_path of A7l structure
    UINT16  *pGreenEvenGain;       // Pointer to one of tables in gain_path of A7l structure
    UINT16  *pGreenOddGain;        // Pointer to one of tables in gain_path of A7l structure
    UINT16  *pBlueGain;            // Pointer to one of tables in gain_path of A7l structure
} AMBA_DSP_IMG_BYPASS_VIGNETTE_INFO_s;

typedef struct amba_img_dsp_cfa_leakage_filter_s {
    UINT32  Enb;
    INT8    AlphaRR;
    INT8    AlphaRB;
    INT8    AlphaBR;
    INT8    AlphaBB;
    UINT16  SaturationLevel;
} amba_img_dsp_cfa_leakage_filter_t;

typedef struct amba_img_dsp_dbp_correction_s {
    UINT8   Enb;    /* 0: disable                       */
                    /* 1: hot 1st order, dark 2nd order */
                    /* 2: hot 2nd order, dark 1st order */
                    /* 3: hot 2nd order, dark 2nd order */
                    /* 4: hot 1st order, dark 1st order */
    UINT8   HotPixelStrength;
    UINT8   DarkPixelStrength;
    UINT8   CorrectionMethod;   /* 0: video, 1:still    */
} amba_img_dsp_dbp_correction_t;

#define    AMBA_DSP_IMG_SBP_VER_1_0     0x20130218
typedef struct _AMBA_DSP_IMG_CALIB_SBP_INFO_s_ {
    UINT32  Version;            /* 0x20130218 */
    UINT8   *SbpBuffer;
    amba_img_dsp_vin_sensor_geometry_t  VinSensorGeo;/* Vin sensor geometry when calibrating. */
    UINT32  Reserved;           /* Reserved for extention. */
    UINT32  Reserved1;          /* Reserved for extention. */
} AMBA_DSP_IMG_CALIB_SBP_INFO_s;

typedef struct amba_img_dsp_sbp_correction_s {
    UINT8   Enb;
    UINT8   Reserved[3];
    amba_img_dsp_vin_sensor_geometry_t  CurrentVinSensorGeo;
    AMBA_DSP_IMG_CALIB_SBP_INFO_s       CalibSbpInfo;
} amba_img_dsp_sbp_correction_t;

typedef struct _AMBA_DSP_IMG_BYPASS_SBP_INFO_s_ {
    UINT8   Enable;
    UINT16  PixelMapWidth;
    UINT16  PixelMapHeight;
    UINT16  PixelMapPitch;
    UINT8   *pMap;
} AMBA_DSP_IMG_BYPASS_SBP_INFO_s;

typedef struct _AMBA_DSP_IMG_GRID_POINT_s_ {
    INT16   X;
    INT16   Y;
} AMBA_DSP_IMG_GRID_POINT_s;

#define    AMBA_DSP_IMG_CAWARP_VER_1_0     0x20130125

typedef struct _AMBA_DSP_IMG_CALIB_CAWARP_INFO_s_ {
    UINT32  Version;            /* 0x20130125 */
    int     HorGridNum;         /* Horizontal grid number. */
    int     VerGridNum;         /* Vertical grid number. */
    int     TileWidthExp;       /* 4:16, 5:32, 6:64, 7:128, 8:256, 9:512 */
    int     TileHeightExp;      /* 4:16, 5:32, 6:64, 7:128, 8:256, 9:512 */
    amba_img_dsp_vin_sensor_geometry_t  VinSensorGeo;   /* Vin sensor geometry when calibrating. */
    INT16   RedScaleFactor;
    INT16   BlueScaleFactor;
    UINT32  Reserved;           /* Reserved for extention. */
    UINT32  Reserved1;          /* Reserved for extention. */
    AMBA_DSP_IMG_GRID_POINT_s   *pCaWarp;   /* Warp grid vector arrey. */
} AMBA_DSP_IMG_CALIB_CAWARP_INFO_s;

typedef struct _AMBA_DSP_IMG_CAWARP_CALC_INFO_s_ {
    UINT8   CaWarpEnb;
    UINT8   Control;
    UINT16  Reserved1;

    AMBA_DSP_IMG_CALIB_CAWARP_INFO_s    CalibCaWarpInfo;

    amba_img_dsp_vin_sensor_geometry_t  VinSensorGeo;       /* Current Vin sensor geometry. */
    amba_img_dsp_win_dimension_t        R2rOutWinDim;       /* Raw 2 raw scaling output window */
    amba_img_dsp_win_geometry_t         DmyWinGeo;          /* Cropping concept */
    amba_img_dsp_win_dimension_t        CfaWinDim;          /* Scaling concept */
} AMBA_DSP_IMG_CAWARP_CALC_INFO_s;

typedef struct _AMBA_DSP_IMG_BYPASS_CAWARP_INFO_s_ {
    UINT8   Enable;
    UINT16  HorzWarpEnable;
    UINT16  VertWarpEnable;
    UINT8   HorzPassGridArrayWidth;
    UINT8   HorzPassGridArrayHeight;
    UINT8   HorzPassHorzGridSpacingExponent;
    UINT8   HorzPassVertGridSpacingExponent;
    UINT8   VertPassGridArrayWidth;
    UINT8   VertPassGridArrayHeight;
    UINT8   VertPassHorzGridSpacingExponent;
    UINT8   VertPassVertGridSpacingExponent;
    UINT16  RedScaleFactor;
    UINT16  BlueScaleFactor;
    INT16   *pWarpHorzTable;
    INT16   *pWarpVertTable;
} AMBA_DSP_IMG_BYPASS_CAWARP_INFO_s;

typedef struct _AMBA_DSP_IMG_CAWARP_SET_INFO_s_ {
    UINT8   ResetVertCaWarp;
    UINT8   Reserved;
    UINT16  Reserved1;
} AMBA_DSP_IMG_CAWARP_SET_INFO_s;

typedef struct amba_img_dsp_wb_gain_s {
    UINT32  GainR;
    UINT32  GainG;
    UINT32  GainB;
    UINT32  AeGain;
    UINT32  GlobalDGain;
} amba_img_dsp_wb_gain_t;

typedef struct amba_img_dsp_dgain_saturation_s {
    UINT32  LevelRed;       /* 14:0 */
    UINT32  LevelGreenEven;
    UINT32  LevelGreenOdd;
    UINT32  LevelBlue;
} amba_img_dsp_dgain_saturation_t;

typedef struct amba_img_dsp_cfa_noise_filter_s {
    UINT8   Enb;
    UINT16  NoiseLevel[3];          /* R/G/B, 0-8192 */
    UINT16  OriginalBlendStr[3];    /* R/G/B, 0-256 */
    UINT16  ExtentRegular[3];       /* R/G/B, 0-256 */
    UINT16  ExtentFine[3];          /* R/G/B, 0-256 */
    UINT16  StrengthFine[3];        /* R/G/B, 0-256 */
    UINT16  SelectivityRegular;     /* 0, 50, 100, 150, 200, 250 */
    UINT16  SelectivityFine;        /* 0, 50, 100, 150, 200, 250 */
} amba_img_dsp_cfa_noise_filter_t;

typedef struct amba_img_dsp_anti_aliasing_s {
    UINT32  Enb;                    //(prefix)_anti_aliasing.enable 0:4
    UINT16  Thresh;                 //(prefix)_anti_aliasing.thresh 0:16383 # when enable=4
    UINT16  LogFractionalCorrect;   //(prefix)_anti_aliasing.log_fractional_correct 0:7 # when enable=4
} amba_img_dsp_anti_aliasing_t;

typedef struct amba_img_dsp_local_exposure_s {
    UINT8   Enb;
    UINT8   Radius; /* 0-6; 4x4 to 16x16 */
    UINT8   LumaWeightRed;
    UINT8   LumaWeightGreen;
    UINT8   LumaWeightBlue;
    UINT8   LumaWeightShift;
    UINT16  GainCurveTable[AMBA_DSP_IMG_NUM_EXPOSURE_CURVE];
} amba_img_dsp_local_exposure_t;

typedef struct amba_img_dsp_def_blc_s {
    UINT8   Enb;
    UINT8   Reserved;
} amba_img_dsp_def_blc_t;

typedef struct amba_img_dsp_demosaic_s {
    UINT16  ActivityThresh; // 0:31
    UINT16  ActivityDifferenceThresh; // 0:16383
    UINT16  GradClipThresh; // 0:4095
    UINT16  GradNoiseThresh; // 0:4095
} amba_img_dsp_demosaic_t;

typedef struct amba_img_dsp_color_correction_s {
    UINT32  MatrixThreeDTableAddr;
    //UINT32  SecCcAddr;
} amba_img_dsp_color_correction_t;

typedef struct amba_img_dsp_color_correction_reg_s {
    UINT32  RegSettingAddr;
} amba_img_dsp_color_correction_reg_t;

typedef struct amba_img_dsp_tone_curve_s {
    UINT16  ToneCurveRed[AMBA_DSP_IMG_NUM_TONE_CURVE];
    UINT16  ToneCurveGreen[AMBA_DSP_IMG_NUM_TONE_CURVE];
    UINT16  ToneCurveBlue[AMBA_DSP_IMG_NUM_TONE_CURVE];
} amba_img_dsp_tone_curve_t;

typedef struct _AMBA_DSP_IMG_SPECIFIG_CC_s_ {
    UINT32  Select;
    float   SigA;
    float   SigB;
    float   SigC;
} AMBA_DSP_IMG_SPECIFIG_CC_s;

typedef struct amba_img_dsp_rgb_to_yuv_s {
    INT16   MatrixValues[9];
    INT16   YOffset;
    INT16   UOffset;
    INT16   VOffset;
} amba_img_dsp_rgb_to_yuv_t;

typedef struct amba_img_dsp_chroma_scale_s {
    UINT8   Enb; /* 0:1 */
    UINT8   Reserved;
    UINT16  Reserved1;
    UINT16  GainCurve[AMBA_DSP_IMG_NUM_CHROMA_GAIN_CURVE]; /* 0:4095 */
} amba_img_dsp_chroma_scale_t;

typedef struct amba_img_dsp_chroma_median_filter_s{
    int     Enable;
    UINT16  CbAdaptiveStrength; // 0:256
    UINT16  CrAdaptiveStrength; // 0:256
    UINT8   CbNonAdaptiveStrength; // 0:31
    UINT8   CrNonAdaptiveStrength; // 0:31
    UINT16  CbAdaptiveAmount; // 0:256
    UINT16  CrAdaptiveAmount; // 0:256
    UINT16  Reserved;
} amba_img_dsp_chroma_median_filter_t;

typedef struct amba_img_dsp_cdnr_info_s {
    UINT8 Enable;
} amba_img_dsp_cdnr_info_t;

typedef struct _AMBA_DSP_IMG_DEFER_COLOR_CORRECTION_s_{
    UINT8     Enable;
} AMBA_DSP_IMG_DEFER_COLOR_CORRECTION_s;

typedef struct amba_img_dsp_liso_process_select_s {
    UINT8   AdvancedFeaturesEnable;   // 0:LISO 1:3-pass(MISO) 2:2-pass
    UINT8   UseSharpenNotAsf;    // 0:ASF 1:SHP ;unavailable if 1=li_processing_select.advanced_features_enable
} amba_img_dsp_liso_process_select_t;

typedef struct amba_img_dsp_mo_process_select_s {
    UINT8   UseSharpenNotAsf;    // 0:ASF 1:SHP ;unavailable if 1=li_processing_select.advanced_features_enable
} amba_img_dsp_mo_process_select_t;

typedef struct _AMBA_DSP_IMG_ALPHA_{
    UINT8   AlphaMinus;
    UINT8   AlphaPlus;
    UINT16  SmoothAdaptation;
    UINT16  SmoothEdgeAdaptation;
    UINT8   T0;
    UINT8   T1;
} AMBA_DSP_IMG_ALPHA_s;

typedef struct amba_img_dsp_coring_s {
    UINT8   Coring[AMBA_DSP_IMG_NUM_CORING_TABLE_INDEX];
    UINT8   FractionalBits;
} amba_img_dsp_coring_t;

typedef struct amba_img_dsp_fir_s{
    UINT8  Specify; /* 0:4 */
    UINT16 PerDirFirIsoStrengths[9]; /* 0:256; Specify = 3 */
    UINT16 PerDirFirDirStrengths[9]; /* 0:256; Specify = 3 */
    UINT16 PerDirFirDirAmounts[9]; /* 0:256; Specify = 3 */
    INT16 Coefs[9][25]; /* 0:1023; Specify = 1,4 */
    UINT16 StrengthIso; /* 0:256; Specify = 0,2 */
    UINT16 StrengthDir; /* 0:256; Specify = 2 */
    UINT8  WideEdgeDetect;
    UINT16 EdgeThresh; // Only hili_luma_mid_high_freq_recover need
} amba_img_dsp_fir_t;

typedef struct amba_img_dsp_level_s {
    UINT8   Low;
    UINT8   LowDelta;
    UINT8   LowStrength;
    UINT8   MidStrength;
    UINT8   High;
    UINT8   HighDelta;
    UINT8   HighStrength;
} amba_img_dsp_level_t;

typedef struct _AMBA_DSP_IMG_TABLE_INDEXING_s_{
    UINT8 YToneOffset;
    UINT8 YToneShift;
    UINT8 YToneBits;
    UINT8 UToneOffset;
    UINT8 UToneShift;
    UINT8 UToneBits;
    UINT8 VToneOffset;
    UINT8 VToneShift;
    UINT8 VToneBits;
    UINT8 *pTable;
    //UINT8 MaxYToneIndex;
    //UINT8 MaxUToneIndex;
    //UINT8 MaxVToneIndex;
} AMBA_DSP_IMG_TABLE_INDEXING_s;

typedef struct amba_img_dsp_sharpen_both_s{
    UINT8  Enable;
    UINT8  Mode;
    UINT16 EdgeThresh;
    UINT8  WideEdgeDetect;
    UINT8 MaxChangeUp5x5;
    UINT8 MaxChangeDown5x5;
    UINT8 Reserved;
}amba_img_dsp_sharpen_both_t;

typedef struct amba_img_dsp_sharpen_noise_s{
    UINT8 MaxChangeUp;
    UINT8 MaxChangeDown;
    amba_img_dsp_fir_t          SpatialFir;
    amba_img_dsp_level_t        LevelStrAdjust;// 1 //Fir2OutScale
    // new in S2L
    UINT8 LevelStrAdjustNotT0T1LevelBased;
    UINT8 T0; // T0<=T1, T1-T0<=15
    UINT8 T1;
    UINT8 AlphaMin;
    UINT8 AlphaMax;
    UINT8 Reserved;
    UINT16 Reserved1;
}amba_img_dsp_sharpen_noise_t;

typedef struct _AMBA_DSP_IMG_FULL_ADAPTATION_s_{
    UINT8                AlphaMinUp;
    UINT8                AlphaMaxUp;
    UINT8                T0Up;
    UINT8                T1Up;
    UINT8                AlphaMinDown;
    UINT8                AlphaMaxDown;
    UINT8                T0Down;
    UINT8                T1Down;
} AMBA_DSP_IMG_FULL_ADAPTATION_s;


typedef struct amba_img_dsp_asf_info_s {
    UINT8                   Enable;
    amba_img_dsp_fir_t      Fir;
    UINT8                   DirectionalDecideT0;
    UINT8                   DirectionalDecideT1;
    AMBA_DSP_IMG_FULL_ADAPTATION_s      Adapt;
    amba_img_dsp_level_t    LevelStrAdjust;
    amba_img_dsp_level_t    T0T1Div;
    UINT8                   MaxChangeNotT0T1Alpha;
    UINT8                   MaxChangeUp;
    UINT8                   MaxChangeDown;
    UINT8                  Reserved;  /* to keep 32 alignment */
} amba_img_dsp_asf_info_t;

typedef struct _AMBA_DSP_IMG_CHROMA_ASF_INFO_s_ {
    UINT8                   Enable;
    amba_img_dsp_fir_t      Fir;
    UINT8                   DirectionalDecideT0;
    UINT8                   DirectionalDecideT1;
    UINT8                   AlphaMin;
    UINT8                   AlphaMax;
    UINT8                   T0;
    UINT8                   T1;
    amba_img_dsp_level_t    LevelStrAdjust;
    amba_img_dsp_level_t    T0T1Div;
    UINT8                   MaxChangeNotT0T1Alpha;
    UINT8                   MaxChange;
    UINT16                  Reserved;  /* to keep 32 alignment */
} AMBA_DSP_IMG_CHROMA_ASF_INFO_s;


typedef struct _AMBA_DSP_IMG_HISO_CHROMA_LOW_VERY_LOW_FILTER_s_ {
    UINT8 EdgeStartCb;
    UINT8 EdgeStartCr;
    UINT8 EdgeEndCb;
    UINT8 EdgeEndCr;
} AMBA_DSP_IMG_HISO_CHROMA_LOW_VERY_LOW_FILTER_s;

typedef struct _AMBA_DSP_IMG_HISO_CHROMA_FILTER_s_ {
    UINT8   Enable;
    UINT8   NoiseLevelCb;          /* 0-255 */
    UINT8   NoiseLevelCr;          /* 0-255 */
    UINT16  OriginalBlendStrengthCb; /* Cb 0-256  */
    UINT16  OriginalBlendStrengthCr; /* Cr 0-256  */
    UINT8   Reserved;
} AMBA_DSP_IMG_HISO_CHROMA_FILTER_s;

typedef struct _AMBA_DSP_IMG_HISO_CHROMA_FILTER_COMBINE_s_ {
    UINT8 T0Cb;
    UINT8 T0Cr;
    UINT8 T1Cb;
    UINT8 T1Cr;
    UINT8 AlphaMaxCb;
    UINT8 AlphaMaxCr;
    UINT8 AlphaMinCb;
    UINT8 AlphaMinCr;
    UINT8 MaxChangeCb;
    UINT8 MaxChangeCr;
} AMBA_DSP_IMG_HISO_CHROMA_FILTER_COMBINE_s;

typedef struct _AMBA_DSP_IMG_HISO_LUMA_FILTER_COMBINE_s_ {
    UINT8 T0;
    UINT8 T1;
    UINT8 AlphaMax;
    UINT8 AlphaMin;
    UINT8 MaxChange;
} AMBA_DSP_IMG_HISO_LUMA_FILTER_COMBINE_s;

typedef struct _AMBA_DSP_IMG_HISO_COMBINE_s_ {
    UINT8 T0Cb;
    UINT8 T0Cr;
    UINT8 T0Y;
    UINT8 T1Cb;
    UINT8 T1Cr;
    UINT8 T1Y;
    UINT8 AlphaMaxCb;
    UINT8 AlphaMaxCr;
    UINT8 AlphaMaxY;
    UINT8 AlphaMinCb;
    UINT8 AlphaMinCr;
    UINT8 AlphaMinY;
    UINT8 MaxChangeNotT0T1LevelBasedCb;
    UINT8 MaxChangeNotT0T1LevelBasedCr;
    UINT8 MaxChangeNotT0T1LevelBasedY;
    UINT8 MaxChangeCb;
    UINT8 MaxChangeCr;
    UINT8 MaxChangeY;
    amba_img_dsp_level_t    EitherMaxChangeOrT0T1AddLevelCb;
    amba_img_dsp_level_t    EitherMaxChangeOrT0T1AddLevelCr;
    amba_img_dsp_level_t    EitherMaxChangeOrT0T1AddLevelY;
    UINT8 SignalPreserveCb;
    UINT8 SignalPreserveCr;
    UINT8 SignalPreserveY;
    AMBA_DSP_IMG_TABLE_INDEXING_s    ThreeD;
} AMBA_DSP_IMG_HISO_COMBINE_s;

typedef struct _AMBA_DSP_IMG_HISO_FREQ_RECOVER_s_ {
    amba_img_dsp_fir_t      Fir;
    UINT8  SmoothSelect[AMBA_DSP_IMG_NUM_CORING_TABLE_INDEX];
    UINT8  MaxDown;
    UINT8  MaxUp;
    amba_img_dsp_level_t    Level;
} AMBA_DSP_IMG_HISO_FREQ_RECOVER_s;

typedef struct _AMBA_DSP_IMG_HISO_LUMA_BLEND_s_ {
    UINT8 Enable;
} AMBA_DSP_IMG_HISO_LUMA_BLEND_s;

typedef struct _AMBA_DSP_IMG_HISO_BLEND_s_ {
    amba_img_dsp_level_t    LumaLevel;
} AMBA_DSP_IMG_HISO_BLEND_s;


#define AMBA_DSP_IMG_RESAMP_COEFF_RECTWIN           0x1
#define AMBA_DSP_IMG_RESAMP_COEFF_M2                0x2
#define AMBA_DSP_IMG_RESAMP_COEFF_M4                0x4
#define AMBA_DSP_IMG_RESAMP_COEFF_LP_MEDIUM         0x8
#define AMBA_DSP_IMG_RESAMP_COEFF_LP_STRONG         0x10

#define AMBA_DSP_IMG_RESAMP_SELECT_CFA              0x1
#define AMBA_DSP_IMG_RESAMP_SELECT_MAIN             0x2
#define AMBA_DSP_IMG_RESAMP_SELECT_PRV_A            0x4
#define AMBA_DSP_IMG_RESAMP_SELECT_PRV_B            0x8
#define AMBA_DSP_IMG_RESAMP_SELECT_PRV_C            0x10

#define AMBA_DSP_IMG_RESAMP_COEFF_MODE_ALWAYS       0
#define AMBA_DSP_IMG_RESAMP_COEFF_MODE_ONE_FRAME    1

typedef struct amba_img_dsp_resampler_coef_adj_s {
    UINT32  ControlFlag;
    UINT16  ResamplerSelect;
    UINT16  Mode;
} amba_img_dsp_resampler_coef_adj_t;

typedef struct amba_img_dsp_chroma_filter_s {
    UINT8  Enable;
    UINT8  NoiseLevelCb;          /* 0-255 */
    UINT8  NoiseLevelCr;          /* 0-255 */
    UINT16  OriginalBlendStrengthCb; /* Cb 0-256  */
    UINT16  OriginalBlendStrengthCr; /* Cr 0-256  */
    UINT16  Radius;                 /* 32-64-128 */
    UINT8   Reserved;
    UINT16  Reserved1;
} amba_img_dsp_chroma_filter_t;

#define    AMBA_DSP_IMG_WARP_VER_1_0     0x20130101
typedef struct _AMBA_DSP_IMG_CALIB_WARP_INFO_s_ {
    UINT32                              Version;        /* 0x20130101 */
    int                                 HorGridNum;     /* Horizontal grid number. */
    int                                 VerGridNum;     /* Vertical grid number. */
    int                                 TileWidthExp;   /* 4:16, 5:32, 6:64, 7:128, 8:256, 9:512 */
    int                                 TileHeightExp;  /* 4:16, 5:32, 6:64, 7:128, 8:256, 9:512 */
    amba_img_dsp_vin_sensor_geometry_t  VinSensorGeo;   /* Vin sensor geometry when calibrating. */
    UINT32                              Reserved;       /* Reserved for extention. */
    UINT32                              Reserved1;      /* Reserved for extention. */
    UINT32                              Reserved2;      /* Reserved for extention. */
    AMBA_DSP_IMG_GRID_POINT_s           *pWarp;         /* Warp grid vector arrey. */
} AMBA_DSP_IMG_CALIB_WARP_INFO_s;

#define AMBA_DSP_IMG_CALC_WARP_CONTROL_SEC2_SCALE      0x1  /* section 2 done all scaling. section 3 does not do scaling. */
#define AMBA_DSP_IMG_CALC_WARP_CONTROL_VERT_FLIP       0x2  /* vertical flip. */

typedef struct amba_img_dsp_warp_calc_info_s {
    /* Warp related settings */
    UINT32                              WarpEnb;
    UINT32                              Control;

    AMBA_DSP_IMG_CALIB_WARP_INFO_s      CalibWarpInfo;

    amba_img_dsp_vin_sensor_geometry_t  VinSensorGeo;           /* Current Vin sensor geometry. */
    amba_img_dsp_win_dimension_t        R2rOutWinDim;           /* Raw 2 raw scaling output window */
    amba_img_dsp_win_geometry_t         DmyWinGeo;              /* Cropping concept */
    amba_img_dsp_win_dimension_t        CfaWinDim;              /* Scaling concept */
    amba_img_dsp_win_coordinates_t     ActWinCrop;             /* Cropping concept */
    amba_img_dsp_win_dimension_t        MainWinDim;             /* Scaling concept */
    amba_img_dsp_win_dimension_t        PrevWinDim[2];          /* 0:PrevA 1: PrevB */
    amba_img_dsp_win_dimension_t        ScreennailDim;
    amba_img_dsp_win_dimension_t        ThumbnailDim;
    int                                 HorSkewPhaseInc;        /* For EIS */
    UINT32                              ExtraVertOutMode;       /* To support warp table that reference dummy window margin pixels */
} amba_img_dsp_warp_calc_info_t;

//typedef struct _AMBA_DSP_IMG_WARP_SET_INFO_s_ {
//    UINT8   ResetVertWarp;
//    UINT8   Reserved;
//    UINT16  Reserved1;
//} AMBA_DSP_IMG_WARP_SET_INFO_s;

typedef struct _AMBA_DSP_IMG_BYPASS_WARP_DZOOM_INFO_s_ {
    // Warp part
    UINT8   Enable;
    UINT32  WarpControl;
    UINT8   GridArrayWidth;
    UINT8   GridArrayHeight;
    UINT8   HorzGridSpacingExponent;
    UINT8   VertGridSpacingExponent;
    UINT8   VertWarpEnable;
    UINT8   VertWarpGridArrayWidth;
    UINT8   VertWarpGridArrayHeight;
    UINT8   VertWarpHorzGridSpacingExponent;
    UINT8   VertWarpVertGridSpacingExponent;
    INT16   *pWarpHorizontalTable;
    INT16   *pWarpVerticalTable;

    // Dzoom part
    UINT8   DzoomEnable;
    UINT32  ActualLeftTopX;
    UINT32  ActualLeftTopY;
    UINT32  ActualRightBotX;
    UINT32  ActualRightBotY;
    UINT32  ZoomX;
    UINT32  ZoomY;
    UINT32  XCenterOffset;
    UINT32  YCenterOffset;
    INT32   HorSkewPhaseInc;
    UINT8   ForceV4tapDisable;
    UINT16  DummyWindowXLeft;
    UINT16  DummyWindowYTop;
    UINT16  DummyWindowWidth;
    UINT16  DummyWindowHeight;
    UINT16  CfaOutputWidth;
    UINT16  CfaOutputHeight;
} AMBA_DSP_IMG_BYPASS_WARP_DZOOM_INFO_s;

typedef struct amba_img_dsp_gmv_info_s {
    UINT16  Enb;
    INT16   MvX;
    INT16   MvY;
    UINT16  Reserved1;
} amba_img_dsp_gmv_info_t;

typedef struct _AMBA_DSP_IMG_VIDEO_MCTF_ONE_CHAN_s_ {
    UINT8   TemporalAlpha0;
    UINT8   TemporalAlpha1;
    UINT8   TemporalAlpha2;
    UINT8   TemporalAlpha3;
    UINT8   TemporalT0;
    UINT8   TemporalT1;
    UINT8   TemporalT2;
    UINT8   TemporalT3;
    UINT8   TemporalMaxChange;
    UINT16  Radius;         /* 0-256 */
    UINT16  StrengthThreeD;      /* 0-256 */
    UINT16  StrengthSpatial;     /* 0-256 */
    UINT16  LevelAdjust;    /* 0-256 */
} AMBA_DSP_IMG_VIDEO_MCTF_ONE_CHAN_s;

typedef struct amba_img_dsp_video_mctf_temporal_adjust_s {
    UINT8 Reserved;
    UINT8 StillThresh;//0:255
    UINT8 Min;//0:3
    UINT8 SlowMoSensitivity;
    UINT16 MotionResponse;//0:65535
    UINT8 NoiseBase;//0:3
    UINT8 Smooth;//1:77
    UINT8 LevAdjustHigh;
    UINT8 LevAdjustHighDelta;
    UINT8 LevAdjustHighStrength;
    UINT8 LevAdjustLow;
    UINT8 LevAdjustLowDelta;
    UINT8 LevAdjustLowStrength;
    UINT8 LevAdjustMidStrength;
    UINT8 Reserved3;
    UINT8 YMax;
    UINT8 YMin;
    UINT8 UVMax;
    UINT8 UVMin;
    UINT32	Reserved0;
    UINT32	Reserved1;
	UINT32	Reserved2;
} amba_img_dsp_video_mctf_temporal_adjust_t;

typedef struct amba_img_dsp_video_mctf_info_s {
    UINT8                                 Enable; // 0,1
    UINT16                              YMaxChange; // 0:255
    UINT16                              UMaxChange; // 0:255
    UINT16                              VMaxChange; // 0:255
    UINT8                               WeightingBasedOnLocalMotion; // 0,1
    UINT8                               Threshold0[4]; // 0:63
    UINT8                               Threshold1[4]; // 0:63
    UINT8                               Threshold2[4]; // 0:63
    UINT8                               Threshold3[4]; // 0:63
    UINT16                               Alpha1[4]; // 0:255
    UINT16                               Alpha2[4]; // 0:255
    UINT16                               Alpha3[4]; // 0:255
} amba_img_dsp_video_mctf_info_t;

typedef struct _AMBA_DSP_IMG_VIDEO_MCTF_GHOST_PRV_INFO_s_ {
    UINT8   Y;
    UINT8   Cb;
    UINT8   Cr;
    UINT8   Reserved;
} AMBA_DSP_IMG_VIDEO_MCTF_GHOST_PRV_INFO_s;

typedef struct amba_img_dsp_gbgr_mismatch_s {
    UINT8  NarrowEnable;
    UINT8  WideEnable;
    UINT16 WideSafety;
    UINT16 WideThresh;
} amba_img_dsp_gbgr_mismatch_t;

// Video HDR
/*================================
=========== HDR Blending =========
=================================*/

typedef struct amba_img_dsp_hdr_blending_info_s {
    UINT8 current_blending_index;
    UINT8 max_blending_number;
} amba_img_dsp_hdr_blending_info_t;

// Black Level Ampifiler Linearization HDR (0x92010)
typedef struct amba_img_dsp_blklvl_amplinear_s {
	INT16 amplinear_black_r;
	INT16 amplinear_black_gr;
	INT16 amplinear_black_gb;
	INT16 amplinear_black_b;

	INT16 amp_hdr_black_r;
	INT16 amp_hdr_black_gr;
	INT16 amp_hdr_black_gb;
	INT16 amp_hdr_black_b;

	INT16 hdrblend_black_r;
	INT16 hdrblend_black_gr;
	INT16 hdrblend_black_gb;
	INT16 hdrblend_black_b;
} amba_img_dsp_blklvl_amplinear_t;

// Black Level Global Offset (0x92001)
// Black Level Global Offset for HDR (0x92011)
// Preblend blacklevel configuration (0x95012)
// > share "amba_img_dsp_black_correction_t"

/*================================
========= Alpha Calculation =======
=================================*/
// General HDR Preblend configuration (0x00095010)
typedef struct amba_img_dsp_hdr_alpha_calc_cfg_s
{
	UINT8  avg_radius;
	UINT8  avg_method;
	UINT8  saturation_num_nei;
	UINT8  reserved;
	UINT8  luma_avg_weight0;
	UINT8  luma_avg_weight1;
	UINT8  luma_avg_weight2;
	UINT8  luma_avg_weight3;
} amba_img_dsp_hdr_alpha_calc_cfg_t;

// Preblend threshold configuration (0x00095011)
typedef struct amba_img_dsp_hdr_alpha_calc_thresh_s
{
	UINT16 saturation_threshold0;
	UINT16 saturation_threshold1;
	UINT16 saturation_threshold2;
	UINT16 saturation_threshold3;
} amba_img_dsp_hdr_alpha_calc_thresh_t;

// Preblend exposure (0x00095013)
typedef struct amba_img_dsp_hdr_alpha_calc_exposure_s
{
	UINT16 input_row_offset;
	UINT16 input_row_height;
	UINT16 input_row_skip;
	UINT16 reserved;
} amba_img_dsp_hdr_alpha_calc_exposure_t;

// Preblend alpha (0x00095014)
typedef struct amba_img_dsp_hdr_alpha_calc_alpha_s
{
	UINT8  pre_alpha_mode;
	UINT8  reserved[3];
	UINT32 alpha_table_addr; // in dram
} amba_img_dsp_hdr_alpha_calc_alpha_t;

typedef struct amba_img_dsp_hdr_alpha_calc_strength_s
{
    UINT8  avg_radius;
	UINT8  avg_method;
    UINT8  blend_control;
	UINT8  luma_avg_weight0;
	UINT8  luma_avg_weight1;
	UINT8  luma_avg_weight2;
	UINT8  luma_avg_weight3;
    UINT8  reserved;
} amba_img_dsp_hdr_alpha_calc_strength_t;

/*================================
==== Amplifier Linearization =====
=================================*/
#define AMBA_DSP_IMG_AMP_LINEAR_LUT_SIZE (353)
#define AMBA_DSP_IMG_ALPHA_LUT_SIZE	(128)

typedef struct amba_img_dsp_amp_linear_info_s
{
	UINT32 amplinear_ena;	// if disabled, amplinear stage would do nothing
	UINT32 mul_r;	// multiplier
	UINT32 mul_g;
	UINT32 mul_b;
	UINT32 lut_addr;  // 353 entries
} amba_img_dsp_amp_linear_info_t; // Different exposure needs individual settings

typedef struct {
	UINT8 hdrblend_enable;
	UINT8 hdr_mode;
	UINT8 hdr_stream0_sub;
	UINT8 hdr_stream1_sub;
	UINT8 hdr_stream1_shift;
	UINT8 hdr_alpha_mode;
	amba_img_dsp_amp_linear_info_t	amp_linear[2];
} amba_img_dsp_amp_linearization_t;

typedef struct amba_img_dsp_amp_linear_ratio_s {
    UINT32 hdr_shutter_ratio;
} amba_img_dsp_amp_linear_ratio_t;

typedef struct _amba_img_dsp_variable_range_s_ {
    UINT8               max_chroma_radius;
    UINT8               max_wide_chroma_radius;
    UINT8               inside_fpn_flag;
    UINT8               wide_chroma_noise_filter_enable;
} amba_img_dsp_variable_range_t;

/*================================
==== video proc & raw offset =====
=================================*/

typedef struct amba_img_dsp_hdr_raw_offset_s{
	UINT32 x_offset[AMBA_DSP_IMG_MAX_HDR_EXPO_NUM];
	UINT32 y_offset[AMBA_DSP_IMG_MAX_HDR_EXPO_NUM];
}amba_img_dsp_hdr_raw_offset_t;

typedef struct amba_img_dsp_contrast_enhc_s{
	UINT16 low_pass_radius;
	UINT16 enhance_gain;
}amba_img_dsp_contrast_enhc_t;

int amba_img_dsp_set_vin_sensor_info(amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_sensor_info_t *pVinSensorInfo);
int amba_img_dsp_get_vin_sensor_info(amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_sensor_info_t *pVinSensorInfo);

/* CFA domain filters */
int amba_img_dsp_set_static_black_level(int fd_iav, amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_black_correction_t *pBlackCorr);
int amba_img_dsp_get_static_black_level(amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_black_correction_t *pBlackCorr);

int amba_img_dsp_calc_vignette_compensation(amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_vignette_calc_info_t *pVignetteCalcInfo);
int amba_img_dsp_set_vignette_compensation(int fd_iav, amba_img_dsp_mode_cfg_t *pMode);
int amba_img_dsp_get_vignette_compensation(amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_vignette_calc_info_t *pVignetteCalcInfo);
int amba_img_dsp_set_vignette_compensation_bypass(int fd_iav, amba_img_dsp_mode_cfg_t * pMode,AMBA_DSP_IMG_BYPASS_VIGNETTE_INFO_s * pVigCorrByPass);

int amba_img_dsp_set_cfa_leakage_filter(int fd_iav, amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_cfa_leakage_filter_t *pCfaLeakage);
int amba_img_dsp_get_cfa_leakage_filter(amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_cfa_leakage_filter_t *pCfaLeakage);

int amba_img_dsp_set_anti_aliasing(int fd_iav, amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_anti_aliasing_t *pAntiAliasing);
int amba_img_dsp_get_anti_aliasing(amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_anti_aliasing_t *pAntiAliasing);

int amba_img_dsp_set_dynamic_bad_pixel_correction(int fd_iav, amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_dbp_correction_t *pDbpCorr);
int amba_img_dsp_get_dynamic_bad_pixel_correction(amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_dbp_correction_t *pDbpCorr);

int amba_img_dsp_set_static_bad_pixel_correction_bypass(int fd_iav,amba_img_dsp_mode_cfg_t *pMode,AMBA_DSP_IMG_BYPASS_SBP_INFO_s *pSbpCorrByPass);
int amba_img_dsp_set_static_bad_pixel_correction(int fd_iav, amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_sbp_correction_t *pSbpCorr);
int amba_img_dsp_get_static_bad_pixel_correction(amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_sbp_correction_t *pSbpCorr);

int amba_img_dsp_set_wb_gain(int fd_iav, amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_wb_gain_t *pWbGains);
int amba_img_dsp_get_wb_gain(amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_wb_gain_t *pWbGains);

int amba_img_dsp_set_dgain_saturation_level(int fd_iav, amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_dgain_saturation_t *pDgainSat);
int amba_img_dsp_get_dgain_saturation_level(amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_dgain_saturation_t *pDgainSat);

int amba_img_dsp_set_cfa_noise_filter(int fd_iav, amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_cfa_noise_filter_t *pCfaNoise);
int amba_img_dsp_get_cfa_noise_filter(amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_cfa_noise_filter_t *pCfaNoise);

int amba_img_dsp_set_local_exposure(int fd_iav, amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_local_exposure_t *pLocalExposure);
int amba_img_dsp_get_local_exposure(amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_local_exposure_t *pLocalExposure);

int amba_img_dsp_set_deferred_black_level(int fd_iav, amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_def_blc_t *pDefBlc);
int amba_img_dsp_get_deferred_black_level(amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_def_blc_t *pDefBlc);

int amba_img_dsp_set_mo_cfa_leakage_filter(int fd_iav, amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_cfa_leakage_filter_t *pCfaLeakage);
int amba_img_dsp_get_mo_cfa_leakage_filter(amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_cfa_leakage_filter_t *pCfaLeakage);

int amba_img_dsp_set_mo_anti_aliasing(int fd_iav, amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_anti_aliasing_t *pAntiAliasing);
int amba_img_dsp_get_mo_anti_aliasing(amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_anti_aliasing_t *pAntiAliasing);

int amba_img_dsp_set_mo_dynamic_bad_pixel_correction(int fd_iav, amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_dbp_correction_t *pDbpCorr);
int amba_img_dsp_get_mo_dynamic_bad_pixel_correction(amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_dbp_correction_t *pDbpCorr);

int amba_img_dsp_set_mo_cfa_noise_filter(int fd_iav, amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_cfa_noise_filter_t *pCfaNoise);
int amba_img_dsp_get_mo_cfa_noise_filter(amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_cfa_noise_filter_t *pCfaNoise);



/* RGB domain filters */
int amba_img_dsp_set_demosaic(int fd_iav, amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_demosaic_t *pDemosaic);
int amba_img_dsp_get_demosaic(amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_demosaic_t *pDemosaic);

int amba_img_dsp_set_color_correction_reg(amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_color_correction_reg_t *pColorCorrReg);
int amba_img_dsp_get_color_correction_reg(amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_color_correction_reg_t *pColorCorrReg);
int amba_img_dsp_set_color_correction(int fd_iav, amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_color_correction_t *pColorCorr);
int amba_img_dsp_get_color_correction(amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_color_correction_t *pColorCorr);

int amba_img_dsp_set_tone_curve(int fd_iav, amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_tone_curve_t *pToneCurve);
int amba_img_dsp_get_tone_curve(amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_tone_curve_t  *pToneCurve);

int amba_img_dsp_set_mo_demosaic(int fd_iav, amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_demosaic_t *pDemosaic);
int amba_img_dsp_get_mo_demosaic(amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_demosaic_t *pDemosaic);



/* Y domain filters */
int amba_img_dsp_set_rgb_to_yuv_matrix(int fd_iav, amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_rgb_to_yuv_t *pRgbToYuv);
int amba_img_dsp_get_rgb_to_yuv_matrix(amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_rgb_to_yuv_t *pRgbToYuv);

int amba_img_dsp_set_chroma_scale(int fd_iav, amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_chroma_scale_t *pChromaScale);
int amba_img_dsp_get_chroma_scale(amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_chroma_scale_t *pChromaScale);

int amba_img_dsp_set_chroma_median_filter(int fd_iav, amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_chroma_median_filter_t *pChromaMedian);
int amba_img_dsp_get_chroma_median_filter(amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_chroma_median_filter_t *pChromaMedian);

int amba_img_dsp_set_color_dependent_noise_reduction(int fd_iav, amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_cdnr_info_t *pCdnr);
int amba_img_dsp_get_color_dependent_noise_reduction(amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_cdnr_info_t *pCdnr);

int amba_img_dsp_set_luma_processing_mode(int fd_iav, amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_liso_process_select_t *pLisoProcessSelect);
int amba_img_dsp_get_luma_processing_mode(amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_liso_process_select_t *pLisoProcessSelect);

int amba_img_dsp_set_advance_spatial_filter(int fd_iav, amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_asf_info_t *pAsf);
int amba_img_dsp_get_advance_spatial_filter(amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_asf_info_t *pAsf);


int amba_img_dsp_set_1st_sharpen_noise_both(int fd_iav, amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_sharpen_both_t *pSharpenBoth);
int amba_img_dsp_get_1st_sharpen_noise_both(amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_sharpen_both_t *pSharpenBoth);

int amba_img_dsp_set_1st_sharpen_noise_noise(int fd_iav, amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_sharpen_noise_t *pSharpenNoise);
int amba_img_dsp_get_1st_sharpen_noise_noise(amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_sharpen_noise_t *pSharpenNoise);

int amba_img_dsp_set_1st_sharpen_noise_sharpen_fir(int fd_iav, amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_fir_t *pSharpenFir);
int amba_img_dsp_get_1st_sharpen_noise_sharpen_fir(amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_fir_t *pSharpenFir);

int amba_img_dsp_set_1st_sharpen_noise_sharpen_coring(int fd_iav, amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_coring_t *pCoring);
int amba_img_dsp_get_1st_sharpen_noise_sharpen_coring(amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_coring_t *pCoring);

int amba_img_dsp_set_1st_sharpen_noise_sharpen_coring_index_scale(int fd_iav, amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_level_t *pLevel);
int amba_img_dsp_get_1st_sharpen_noise_sharpen_coring_index_scale(amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_level_t *pLevel);

int amba_img_dsp_set_1st_sharpen_noise_sharpen_min_coring_result(int fd_iav, amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_level_t *pLevel);
int amba_img_dsp_get_1st_sharpen_noise_sharpen_min_coring_result(amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_level_t *pLevel);

int amba_img_dsp_set_1st_sharpen_noise_sharpen_scale_coring(int fd_iav, amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_level_t *pLevel);
int amba_img_dsp_get_1st_sharpen_noise_sharpen_scale_coring(amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_level_t *pLevel);

int amba_img_dsp_set_final_sharpen_noise_both(int fd_iav, amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_sharpen_both_t *pSharpenBoth);
int amba_img_dsp_get_final_sharpen_noise_both(amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_sharpen_both_t *pSharpenBoth);

int amba_img_dsp_set_final_sharpen_noise_noise(int fd_iav, amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_sharpen_noise_t *pSharpenNoise);
int amba_img_dsp_get_final_sharpen_noise_noise(amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_sharpen_noise_t *pSharpenNoise);

int amba_img_dsp_set_final_sharpen_noise_sharpen_fir(int fd_iav, amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_fir_t *pSharpenFir);
int amba_img_dsp_get_final_sharpen_noise_sharpen_fir(amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_fir_t *pSharpenFir);

int amba_img_dsp_set_final_sharpen_noise_sharpen_coring(int fd_iav, amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_coring_t *pCoring);
int amba_img_dsp_get_final_sharpen_noise_sharpen_coring(amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_coring_t *pCoring);

int amba_img_dsp_set_final_sharpen_noise_sharpen_coring_index_scale(int fd_iav, amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_level_t *pLevel);
int amba_img_dsp_get_final_sharpen_noise_sharpen_coring_index_scale(amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_level_t *pLevel);

int amba_img_dsp_set_final_sharpen_noise_sharpen_min_coring_result(int fd_iav, amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_level_t *pLevel);
int amba_img_dsp_get_final_sharpen_noise_sharpen_min_coring_result(amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_level_t *pLevel);

int amba_img_dsp_set_final_sharpen_noise_sharpen_scale_coring(int fd_iav, amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_level_t *pLevel);
int amba_img_dsp_get_final_sharpen_noise_sharpen_scale_coring(amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_level_t *pLevel);

int amba_img_dsp_set_resampler_coef_adj(int fd_iav, amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_resampler_coef_adj_t *pResamplerCoefAdj);
int amba_img_dsp_get_resampler_coef_adj(amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_resampler_coef_adj_t *pResamplerCoefAdj);

int amba_img_dsp_set_chroma_filter(int fd_iav, amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_chroma_filter_t *pChromaFilter);
int amba_img_dsp_get_chroma_filter(amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_chroma_filter_t *pChromaFilter);

int amba_img_dsp_set_wide_chroma_filter(int fd_iav, amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_chroma_filter_t *pWideChromaFilter);
int amba_img_dsp_get_wide_chroma_filter(amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_chroma_filter_t *pWIdeChromaFilter);

int amba_img_dsp_set_wide_chroma_filter_combine(int fd_iav, amba_img_dsp_mode_cfg_t *pMode, AMBA_DSP_IMG_HISO_CHROMA_FILTER_COMBINE_s *pWideChromaFilterCombine);
int amba_img_dsp_get_wide_chroma_filter_combine(amba_img_dsp_mode_cfg_t *pMode, AMBA_DSP_IMG_HISO_CHROMA_FILTER_COMBINE_s *pWideChromaFilterCombine);
int amba_img_dsp_set_variable_range(amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_variable_range_t *pVariableRange);

int amba_img_dsp_set_gbgr_mismatch(int fd_iav, amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_gbgr_mismatch_t *pGbGrMismatch);
int amba_img_dsp_get_gbgr_mismatch(amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_gbgr_mismatch_t *pGbGrMismatch);

int amba_img_dsp_set_mo_chroma_median_filter(int fd_iav, amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_chroma_median_filter_t *pChromaMedian);
int amba_img_dsp_get_mo_chroma_median_filter(amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_chroma_median_filter_t *pChromaMedian);

int amba_img_dsp_set_mo_luma_processing_mode(int fd_iav, amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_liso_process_select_t *pLisoProcessSelect);
int amba_img_dsp_get_mo_luma_processing_mode(amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_liso_process_select_t *pLisoProcessSelect);

int amba_img_dsp_set_mo_advance_spatial_filter(int fd_iav, amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_asf_info_t *pAsf);
int amba_img_dsp_get_mo_advance_spatial_filter(amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_asf_info_t *pAsf);


int amba_img_dsp_set_mo_sharpen_noise_both(int fd_iav, amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_sharpen_both_t *pSharpenBoth);
int amba_img_dsp_get_mo_sharpen_noise_both(amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_sharpen_both_t *pSharpenBoth);

int amba_img_dsp_set_mo_sharpen_noise_noise(int fd_iav, amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_sharpen_noise_t *pSharpenNoise);
int amba_img_dsp_get_mo_sharpen_noise_noise(amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_sharpen_noise_t *pSharpenNoise);

int amba_img_dsp_set_mo_sharpen_noise_sharpen_fir(int fd_iav, amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_fir_t *pSharpenFir);
int amba_img_dsp_get_mo_sharpen_noise_sharpen_fir(amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_fir_t *pSharpenFir);

int amba_img_dsp_set_mo_sharpen_noise_sharpen_coring(int fd_iav, amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_coring_t *pCoring);
int amba_img_dsp_get_mo_sharpen_noise_sharpen_coring(amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_coring_t *pCoring);

int amba_img_dsp_set_mo_sharpen_noise_sharpen_coring_index_scale(int fd_iav, amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_level_t *pLevel);
int amba_img_dsp_get_mo_sharpen_noise_sharpen_coring_index_scale(amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_level_t *pLevel);

int amba_img_dsp_set_mo_sharpen_noise_sharpen_min_coring_result(int fd_iav, amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_level_t *pLevel);
int amba_img_dsp_get_mo_sharpen_noise_sharpen_min_coring_result(amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_level_t *pLevel);

int amba_img_dsp_set_mo_sharpen_noise_sharpen_scale_coring(int fd_iav, amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_level_t *pLevel);
int amba_img_dsp_get_mo_sharpen_noise_sharpen_scale_coring(amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_level_t *pLevel);

int amba_img_dsp_set_mo_chroma_filter(int fd_iav, amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_chroma_filter_t *pChromaFilter);
int amba_img_dsp_get_mo_chroma_filter(amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_chroma_filter_t *pChromaFilter);

int amba_img_dsp_set_mo_gbgr_mismatch(int fd_iav, amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_gbgr_mismatch_t *pGbGrMismatch);
int amba_img_dsp_get_mo_gbgr_mismatch(amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_gbgr_mismatch_t *pGbGrMismatch);



/* Warp and MCTF related filters */

int amba_img_dsp_set_video_mctf(int fd_iav, amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_video_mctf_info_t *pMctfInfo);
int amba_img_dsp_get_video_mctf(amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_video_mctf_info_t *pMctfInfo);

int   amba_img_dsp_set_video_mctf_temporal_adjust(int fd_iav, amba_img_dsp_mode_cfg_t  *pMode, amba_img_dsp_video_mctf_temporal_adjust_t *pMctfMbTemporal);
int   amba_img_dsp_get_video_mctf_temporal_adjust(amba_img_dsp_mode_cfg_t  *pMode, amba_img_dsp_video_mctf_temporal_adjust_t *pMctfMbTemporal);


int AmbaDSP_ImgSetVideoMctfGhostPrv( amba_img_dsp_mode_cfg_t *pMode, AMBA_DSP_IMG_VIDEO_MCTF_GHOST_PRV_INFO_s *pMctfGpInfo);
int AmbaDSP_ImgGetVideoMctfGhostPrv(amba_img_dsp_mode_cfg_t *pMode, AMBA_DSP_IMG_VIDEO_MCTF_GHOST_PRV_INFO_s *pMctfGpInfo);

int AmbaDSP_ImgSetVideoMctfCompressionEnable( amba_img_dsp_mode_cfg_t *pMode, UINT8 Enb);
int AmbaDSP_ImgGetVideoMctfCompressionEnable(amba_img_dsp_mode_cfg_t *pMode, UINT8 *pEnb);

// contrast enhancement, can be used in HDR and non-HDR modes
int amba_img_dsp_set_contrast_enhance(int fd_iav,amba_img_dsp_mode_cfg_t * pMode,amba_img_dsp_contrast_enhc_t * p_ce_info);
int amba_img_dsp_get_contrast_enhance(amba_img_dsp_mode_cfg_t * pMode,amba_img_dsp_contrast_enhc_t * p_ce_info);

int amba_img_dsp_set_hdr_raw_offset(int fd_iav, amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_hdr_raw_offset_t *p_hdr_raw_offset);
int amba_img_dsp_get_hdr_raw_offset(amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_hdr_raw_offset_t *p_hdr_raw_offset);

//int img_dsp_get_statistics(amba_img_dsp_mode_cfg_t *pMode,
//	img_aaa_stat_t *p_stat, aaa_tile_report_t *p_act_tile,int work_mode);

// Video HDR
int amba_img_dsp_set_hdr_blending_index( int fd_iav, amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_hdr_blending_info_t *p_hdr_blending_info);
int amba_img_dsp_get_hdr_blending_index( amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_hdr_blending_info_t *p_hdr_blending_info);

int amba_img_dsp_set_hdr_alpha_calc_strength(int fd_iav, amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_hdr_alpha_calc_strength_t *p_hdr_alpha_strength);
int amba_img_dsp_get_hdr_alpha_calc_strength(amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_hdr_alpha_calc_strength_t	*p_hdr_alpha_strength);

/*************************************************
*	@Description:
		3A could use this APi to generate lut[353] to feed lut_addr in amba_img_dsp_amp_linear_info_t
*	@Input :
*		u32 al_index:  look-up table index [0:352]
*	@Output
*		u32 *p_input_value: corresponding index of x-axis in 3A mapping table [0:16383]
*/
int amba_img_dsp_get_amp_linearization_input_value(u32 al_index, u32 *p_input_value);

/* Generate R/Gr/Gb/B look-up table incorporating wbgain for LE/SE, and set 0x91003 */
int amba_img_dsp_set_amp_linearization(int fd_iav, amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_amp_linearization_t *p_amp_linearizatoin);
int amba_img_dsp_get_amp_linearization(amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_amp_linearization_t *p_amp_linearizatoin);

/*********** Video Proc   *********/
int amba_img_dsp_set_hdr_video_post_proc(int fd_iav, amba_img_dsp_mode_cfg_t     *pMode);

// hdr start, end
int amba_img_dsp_hdr_start(int fd_iav, int expo_id);
int amba_img_dsp_hdr_end(int fd_iav);

#endif  /* _AMBA_DSP_IMG_FILTER_H_ */


