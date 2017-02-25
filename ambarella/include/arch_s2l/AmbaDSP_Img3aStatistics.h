/*
 * include/arch_s2l/AmbaDSP_Img3aStatistics.h
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

#ifndef _AMBA_DSP_IMG_AAA_STAT_H_
#define _AMBA_DSP_IMG_AAA_STAT_H_

#include "AmbaDataType.h"
#include "AmbaDSP_ImgDef.h"
#include "ambas_imgproc_arch.h"
//#include "AmbaDSP_EventInfo.h"

typedef struct _AMBA_DSP_IMG_AAA_STAT_CTRL_s_ {
    UINT8   AeAwbEnable;
    UINT8   AfEnable;
    UINT8   HisEnable;
} AMBA_DSP_IMG_AAA_STAT_ENB_s;      /* TODO: should be better to changed it into AMBA_DSP_IMG_AAA_STAT_CTRL_s */

typedef struct amba_img_dsp_ae_stat_info_s {
    UINT16  AeTileNumCol;
    UINT16  AeTileNumRow;
    UINT16  AeTileColStart;
    UINT16  AeTileRowStart;
    UINT16  AeTileWidth;
    UINT16  AeTileHeight;
    UINT16  AePixMinValue;
    UINT16  AePixMaxValue;
} amba_img_dsp_ae_stat_info_t;

typedef struct amba_img_dsp_awb_stat_info_s {
    UINT16  AwbTileNumCol;
    UINT16  AwbTileNumRow;
    UINT16  AwbTileColStart;
    UINT16  AwbTileRowStart;
    UINT16  AwbTileWidth;
    UINT16  AwbTileHeight;
    UINT16  AwbTileActiveWidth;
    UINT16  AwbTileActiveHeight;
    UINT32  AwbPixMinValue;
    UINT32  AwbPixMaxValue;
} amba_img_dsp_awb_stat_info_t;

typedef struct amba_img_dsp_af_stat_info_s {
    UINT16  AfTileNumCol;
    UINT16  AfTileNumRow;
    UINT16  AfTileColStart;
    UINT16  AfTileRowStart;
    UINT16  AfTileWidth;
    UINT16  AfTileHeight;
    UINT16  AfTileActiveWidth;
    UINT16  AfTileActiveHeight;
} amba_img_dsp_af_stat_info_t;

typedef struct amba_img_dsp_hist_stat_info_s {
	UINT32	HistMode;
	UINT16	TileMask[8];	/* 12 * 8 tiles */
}amba_img_dsp_hist_stat_info_t;

typedef struct amba_img_dsp_hdr_stat_info_s
{
  UINT8  VinStatsMainOn;
  UINT8  VinStatsHdrOn;
  UINT8  TotalExposures;
  UINT8  TotalSliceInX;

  UINT32 MainDataFifoBase;
  UINT32 MainDataFifoLimit;
  UINT32 HdrDataFifoBase;
  UINT32 HdrDataFifoLimit;
} amba_img_dsp_hdr_stat_info_t;

typedef struct amba_dsp_img_af_stat_ex_info_s {
    UINT8   AfHorizontalFilter1Mode;
    UINT8   AfHorizontalFilter1Stage1Enb;
    UINT8   AfHorizontalFilter1Stage2Enb;
    UINT8   AfHorizontalFilter1Stage3Enb;
    INT16   AfHorizontalFilter1Gain[7];
    UINT16  AfHorizontalFilter1Shift[4];
    UINT16  AfHorizontalFilter1BiasOff;
    UINT16  AfHorizontalFilter1Thresh;
    UINT16  AfVerticalFilter1Thresh;
    UINT8   AfHorizontalFilter2Mode;
    UINT8   AfHorizontalFilter2Stage1Enb;
    UINT8   AfHorizontalFilter2Stage2Enb;
    UINT8   AfHorizontalFilter2Stage3Enb;
    INT16   AfHorizontalFilter2Gain[7];
    UINT16  AfHorizontalFilter2Shift[4];
    UINT16  AfHorizontalFilter2BiasOff;
    UINT16  AfHorizontalFilter2Thresh;
    UINT16  AfVerticalFilter2Thresh;
    UINT16  AfTileFv1HorizontalShift;
    UINT16  AfTileFv1VerticalShift;
    UINT16  AfTileFv1HorizontalWeight;
    UINT16  AfTileFv1VerticalWeight;
    UINT16  AfTileFv2HorizontalShift;
    UINT16  AfTileFv2VerticalShift;
    UINT16  AfTileFv2HorizontalWeight;
    UINT16  AfTileFv2VerticalWeight;
} amba_dsp_img_af_stat_ex_info_t;

typedef struct amba_img_dsp_aaa_stat_info_s {
    UINT16  AwbTileNumCol;
    UINT16  AwbTileNumRow;
    UINT16  AwbTileColStart;
    UINT16  AwbTileRowStart;
    UINT16  AwbTileWidth;
    UINT16  AwbTileHeight;
    UINT16  AwbTileActiveWidth;
    UINT16  AwbTileActiveHeight;
    UINT16  AwbPixMinValue;
    UINT16  AwbPixMaxValue;
    UINT16  AeTileNumCol;
    UINT16  AeTileNumRow;
    UINT16  AeTileColStart;
    UINT16  AeTileRowStart;
    UINT16  AeTileWidth;
    UINT16  AeTileHeight;
    UINT16  AePixMinValue;
    UINT16  AePixMaxValue;
    UINT16  AfTileNumCol;
    UINT16  AfTileNumRow;
    UINT16  AfTileColStart;
    UINT16  AfTileRowStart;
    UINT16  AfTileWidth;
    UINT16  AfTileHeight;
    UINT16  AfTileActiveWidth;
    UINT16  AfTileActiveHeight;
} amba_img_dsp_aaa_stat_info_t;

typedef struct _AMBA_DSP_IMG_FLOAT_TILE_CONFIG_s_ {
    UINT16  FloatTileColStart:13;
    UINT16  FloatTileRowStart:14;
    UINT16  FloatTileWidth:13;
    UINT16  FloatTileHeight:14;
    UINT16  FloatTileShift:10;
} AMBA_DSP_IMG_FLOAT_TILE_CONFIG_s;

typedef struct _AMBA_DSP_IMG_AAA_FLOAT_TILE_INFO_s_ {
    UINT16                              FrameSyncId;
    UINT16                              NumberOfTiles;
    AMBA_DSP_IMG_FLOAT_TILE_CONFIG_s    FloatTileConfig[32];
} AMBA_DSP_IMG_AAA_FLOAT_TILE_INFO_s;

typedef struct amba_img_dsp_aaa_histogram_s
{
    UINT32 Mode;
    UINT16 AeFileMask[8];
}amba_img_dsp_aaa_histogram_t;

typedef struct amba_img_dsp_aaa_pseudo_y_s
{
    UINT32 Mode;
    UINT32 SumShift;
    UINT8  PixelWeight[4];
    UINT8  ToneCurve[32];
	UINT8  Reserved0;
	UINT8  Reserved1;
}amba_img_dsp_aaa_pseudo_y_t;


typedef struct amba_img_dsp_aaa_early_wb_gain_s
{
    UINT32 RedMultiplier;
    UINT32 GreenMultiplierEven;
    UINT32 GreenMultiplierOdd;
    UINT32 BlueMultiplier;
    UINT8  EnableAeWbGain;
    UINT8  EnableAfWbGain;
    UINT8  EnableHistogramWbGain;
    UINT8  Reserved;
    UINT32 RedWbMultiplier;
    UINT32 GreenWbMultiplierEven;
    UINT32 GreenWbMultiplierOdd;
    UINT32 BlueWbMultiplier;
}amba_img_dsp_aaa_early_wb_gain_t;


typedef struct _AMBA_DSP_TRANSFER_AAA_STATISTIC_DATA_s_ {
    UINT32  RgbAaaDataAddr;
    UINT32  CfaAaaDataAddr;
    UINT32  SrcRgbAaaDataAddr;
    UINT32  SrcCfaAaaDataAddr;
} AMBA_DSP_TRANSFER_AAA_STATISTIC_DATA_s;

typedef struct amba_dsp_aaa_statistic_data_s {
    UINT32  RgbAaaDataAddr;
    UINT32  CfaAaaDataAddr;
    UINT32  SensorDataAddr;
    UINT32  CfaPreHdrDataAddr;

    UINT32   RgbDataValid;
    UINT32   CfaDataValid;
    UINT32   SensorDataValid;
    UINT32   CfaPreHdrDataValid;

    UINT32   DspPtsDataAddr;
    UINT32   MonoPtsDataAddr;

} amba_dsp_aaa_statistic_data_t;

int amba_img_dsp_3a_config_aaa_stat(int fd_iav,
					int enable,
					amba_img_dsp_mode_cfg_t* pMode,
					amba_img_dsp_ae_stat_info_t *pAeStat,
					amba_img_dsp_awb_stat_info_t *pAwbStat,
					amba_img_dsp_af_stat_info_t *pAfStat);

int amba_img_dsp_3a_config_histogram(int fd_iav,
					amba_img_dsp_mode_cfg_t * pMode,
					amba_img_dsp_hist_stat_info_t * pHistStat);

int amba_img_dsp_3a_config_hdr_stat(int fd_iav,
					amba_img_dsp_mode_cfg_t * pMode,
					amba_img_dsp_hdr_stat_info_t * pHdrStat);

int amba_img_dsp_3a_get_aaa_stat(int fd_iav,
					amba_img_dsp_mode_cfg_t* pMode,
					amba_dsp_aaa_statistic_data_t* pAAAData);

int amba_img_dsp_3a_get_min_max_value(
					UINT16* pAePixMinValue,
					UINT16* pAePixMaxValue,
					UINT32* pAwbPixMinValue,
					UINT32* pAwbPixMaxValue);

/* define tile configuration for AWB/AE/AF statistics calculation. The param  */
/* are based on a 4096x4096 logical image to specify tiling geometry.         */
int amba_img_dsp_3a_set_aaa_stat_info(
					int fd_iav,
					amba_img_dsp_mode_cfg_t *pMode);

int amba_img_dsp_set_af_statistics_ex(
					int fd_iav,
					amba_img_dsp_mode_cfg_t *pMode,
					struct aaa_statistics_ex *paf_ex,
					u8 enable);

int amba_img_dsp_get_af_statistics_ex(
					amba_img_dsp_mode_cfg_t *pMode,
					struct aaa_statistics_ex *p_af_statistic_ex);

/* define tile advanced configuration for AF statistics calculation.          */
int amba_dsp_img_3a_set_af_stat_ex_info(int fd_iav, amba_img_dsp_mode_cfg_t *pMode, amba_dsp_img_af_stat_ex_info_t *pAfStatEx);

/* Get tile advanced configuration for AF statistics calculation.          */
int amba_dsp_img_3a_get_af_stat_ex_info(int fd_iav, amba_img_dsp_mode_cfg_t *pMode, amba_dsp_img_af_stat_ex_info_t *pAfStatEx);

int   amba_dsp_img_set_3a_pseudo_y(
                    int fd_iav,
                    amba_img_dsp_mode_cfg_t         *pMode,
                    amba_img_dsp_aaa_pseudo_y_t     *pPseudoY);
int   amba_dsp_img_get_3a_pseudo_y(
                    int fd_iav,
                    amba_img_dsp_mode_cfg_t         *pMode,
                    amba_img_dsp_aaa_pseudo_y_t     *pPseudoY);

int   amba_dsp_img_set_3a_histogram(
                    int fd_iav,
                    amba_img_dsp_mode_cfg_t         *pMode,
                    amba_img_dsp_aaa_histogram_t     *pHistogram);
int   amba_dsp_img_get_3a_histogram(
                    int fd_iav,
                    amba_img_dsp_mode_cfg_t         *pMode,
                    amba_img_dsp_aaa_histogram_t     *pHistogram);
int   amba_dsp_img_set_3a_early_wb_gain(
                    int fd_iav,
                    amba_img_dsp_mode_cfg_t              *pMode,
                    amba_img_dsp_aaa_early_wb_gain_t     *pEarlyWbGain);
int   amba_dsp_img_get_3a_early_wb_gain(
                    int fd_iav,
                    amba_img_dsp_mode_cfg_t              *pMode,
                    amba_img_dsp_aaa_early_wb_gain_t     *pEarlyWbGain);




#if 0
/*-----------------------------------------------------------------------------------------------*\
 * Defined in AmbaDSP_Img3aStatistics.c
\*-----------------------------------------------------------------------------------------------*/
/* enable/disable various 3A statistics. AmbaDSP_Img3aTransferAaaStatData will not*/
/* function properly if statistics is not enabled.                           */
int AmbaDSP_Img3aEnbAaaStat(amba_img_dsp_mode_cfg_t *pMode, AMBA_DSP_IMG_AAA_STAT_ENB_s *pEnbInfo);

/* provides the AE, AWB, AF and histogram statistics collection data          */
int AmbaDSP_Img3aTransferAaaStatData(amba_img_dsp_mode_cfg_t *pMode, AMBA_DSP_TRANSFER_AAA_STATISTIC_DATA_s *pTAaaStatData);

/* define tile configuration for AE statistics calculation. The parameters    */
/* are based on a 4096x4096 logical image to specify tiling geometry.         */
int AmbaDSP_Img3aSetAeStatInfo(amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_ae_stat_info_t *pAeStat);

/* define tile configuration for AWB statistics calculation. The parameters   */
/* are based on a 4096x4096 logical image to specify tiling geometry.         */
int AmbaDSP_Img3aSetAwbStatInfo(amba_img_dsp_mode_cfg_t *pMode, AMBA_DSP_IMG_AWB_STAT_INFO_s *pAwbStat);

/* define tile configuration for AF statistics calculation. The parameters    */
/* are based on a 4096x4096 logical image to specify tiling geometry.         */
int AmbaDSP_Img3aSetAfStatInfo(amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_af_stat_info_t *pAfStat);

/* define tile configuration for AWB/AE/AF statistics calculation. The param  */
/* are based on a 4096x4096 logical image to specify tiling geometry.         */
int amba_img_dsp_3a_set_aaa_stat_info(amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_aaa_stat_info_t *pAaaStat);

/* Get the AAA statistic information settings*/
int AmbaDSP_Img3aGetAaaStatInfo(amba_img_dsp_mode_cfg_t *pMode, amba_img_dsp_aaa_stat_info_t *pAaaStat);

/* define tile configuration for floating tile statistics calculation.        */
int AmbaDSP_Img3aSetAaaFloatTileInfo(amba_img_dsp_mode_cfg_t *pMode, AMBA_DSP_IMG_AAA_FLOAT_TILE_INFO_s *pAaaFloatTileStat);

/* Get tile configuration for floating tile statistics calculation  */
int AmbaDSP_Img3aGetAaaFloatTileInfo(amba_img_dsp_mode_cfg_t *pMode, AMBA_DSP_IMG_AAA_FLOAT_TILE_INFO_s *pAaaFloatTileStat);

int AmbaDSP_Img3aGetAaaData (int fd_iav,amba_img_dsp_mode_cfg_t* pMode,amba_dsp_aaa_statistic_data_t* pAAAData);
#endif
#endif  /* _AMBA_DSP_IMG_AAA_STAT_H_ */
