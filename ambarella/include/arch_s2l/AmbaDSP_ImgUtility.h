/*
 * include/arch_s2l/AmbaDSP_ImgUtility.h
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

#ifndef _AMBA_DSP_IMG_UTIL_H_
#define _AMBA_DSP_IMG_UTIL_H_

#include "AmbaDSP_ImgDef.h"

#define AMBA_DSP_IMG_MAX_PIPE_NUM         8

typedef struct amba_img_dsp_pipe_info_s {
    AMBA_DSP_IMG_PIPE_e Pipe;
    UINT8               CtxBufNum;
    UINT8               CfgBufNum;
    UINT8               Reserved;
} amba_img_dsp_pipe_info_t;

#if 0
typedef struct _AMBA_DSP_IMG_MEM_INFO_s_ {
    UINT32  Addr;
    UINT32  Size;
} AMBA_DSP_IMG_MEM_INFO_s;
#endif

typedef struct amba_img_dsp_arch_info_s {
    UINT8                    *pWorkBuf;
    UINT32                   BufSize;
    UINT32                   BufNum;
    UINT32                   PipeNum;
    amba_img_dsp_pipe_info_t *pPipeInfo[AMBA_DSP_IMG_MAX_PIPE_NUM];
} amba_img_dsp_arch_info_t;

typedef struct amba_img_dsp_ctx_info_s {
    AMBA_DSP_IMG_PIPE_e Pipe;
    UINT8               CtxId;
    UINT8               Reserved;
    UINT8               Reserved1;
} amba_img_dsp_ctx_info_t;

typedef struct amba_img_dsp_cfg_info_s {
    AMBA_DSP_IMG_PIPE_e Pipe;
    UINT8               CfgId;
    UINT8               Reserved;
    UINT8               Reserved1;
} amba_img_dsp_cfg_info_t;

typedef enum _amba_img_dsp_cfg_state_e_ {
    AMBA_DSP_IMG_CFG_STATE_IDLE = 0x00,
    AMBA_DSP_IMG_CFG_STATE_INIT,
    AMBA_DSP_IMG_CFG_STATE_PREEXE,
    AMBA_DSP_IMG_CFG_STATE_POSTEXE,
} amba_img_dsp_cfg_state_e;

typedef enum _AMBA_DSP_IMG_CONFIG_EXECUTE_MODE_e_{
    AMBA_DSP_IMG_CFG_EXE_QUICK = 0x00,
    AMBA_DSP_IMG_CFG_EXE_FULLCOPY,
    AMBA_DSP_IMG_CFG_EXE_PARTIALCOPY,
} AMBA_DSP_IMG_CONFIG_EXECUTE_MODE_e;


typedef struct amba_img_dsp_cfg_status_s {
    UINT32                   Addr;           /* Config buffer address. */
    amba_img_dsp_cfg_state_e State;
    AMBA_DSP_IMG_ALGO_MODE_e AlgoMode;
    UINT8                    Reserved;
    UINT8                    Reserved1;
} amba_img_dsp_cfg_status_t;

typedef struct amba_img_dsp_size_info_s {
    UINT16  WidthIn;
    UINT16  HeightIn;
    UINT16  WidthMain;
    UINT16  HeightMain;
    UINT16  WidthPrevA;
    UINT16  HeightPrevA;
    UINT16  WidthPrevB;
    UINT16  HeightPrevB;
    UINT16  WidthScrn;
    UINT16  HeightScrn;
    UINT16  WidthQvRaw;
    UINT16  HeightQvRaw;
    UINT16  Reserved;
    UINT16  Reserved1;
} amba_img_dsp_size_info_t;

typedef struct _AMBA_DSP_IMG_LISO_DEBUG_MODE_s_ {
    UINT8 Step;
    UINT8 Mode;
    UINT8 ChannelID;
    UINT8 TileX;
    UINT8 TileY;
    UINT32 PicNum;
} AMBA_DSP_IMG_DEBUG_MODE_s;

typedef struct capture_region_s {
   unsigned int pitch;
   unsigned int startCol;
   unsigned int startRow;
   unsigned int endCol;
   unsigned int endRow;
}capture_region_t;

typedef enum _amba_img_dsp_print_debug_log_ {
    AMBA_DSP_IMG_ENABLE_DO_NOTHING= 0x00,
    AMBA_DSP_IMG_ENABLE_FUNC_NAME,
    AMBA_DSP_IMG_PRINT_INPUT,
} amba_img_dsp_print_debug_log;

typedef struct amba_img_dsp_version_s {
	int	    major;
	int	    minor;
	int	    patch;
	u32	    mod_time;
	char	description[128];
}  amba_img_dsp_version_t;

/*---------------------------------------------------------------------------*\
 * Defined in AmbaDSP_ImgUtility.c
\*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*\
 * Pipeline related api
\*---------------------------------------------------------------------------*/
int amba_img_dsp_query_arch_mem_size(amba_img_dsp_arch_info_t *pArchInfo);
int amba_img_dsp_init_arch(amba_img_dsp_arch_info_t *pArchInfo);
int amba_img_dsp_deinit_arch(void);


/*---------------------------------------------------------------------------*\
 * Context related api
\*---------------------------------------------------------------------------*/
/* To initialize the context. */
int   amba_img_dsp_init_ctx(
                    UINT8                       InitMode,
                    UINT8                       DefblcEnb,
                    UINT8			FinalSharpenEnb,
                    amba_img_dsp_ctx_info_t     *pDestCtx,
                    amba_img_dsp_ctx_info_t     *pSrcCtx);

/*---------------------------------------------------------------------------*\
 * Config related api
\*---------------------------------------------------------------------------*/
/* To initialize the config buffer layout to certain algo mode settings. */
int amba_img_dsp_init_cfg(amba_img_dsp_cfg_info_t *pCfgInfo, AMBA_DSP_IMG_ALGO_MODE_e AlgoMode);

/* To pre-execute the config from CtxId using AlgoMode and FuncMode method. */
int amba_img_dsp_pre_exe_cfg(
                  amba_img_dsp_mode_cfg_t     *pMode,
                  AMBA_DSP_IMG_CONFIG_EXECUTE_MODE_e ExeMode);
                    /* 0: fast execute. Point to content in context memory buffer. */
                    /* 1: full copy execute. Copy content from context memory to config memory. */

/* To post-execute the config from CtxId using AlgoMode and FuncMode method. */
int amba_img_dsp_post_exe_cfg(int fd_iav,
                  amba_img_dsp_mode_cfg_t     *pMode,
                  AMBA_DSP_IMG_CONFIG_EXECUTE_MODE_e ExeMode,
                  UINT32 boot);
                    /* 0: fast execute. Point to content in context memory buffer. */
                    /* 1: full copy execute. Copy content from context memory to config memory. */
int amba_img_dsp_set_size_info(
                    amba_img_dsp_mode_cfg_t     *pMode,
                    amba_img_dsp_size_info_t    *pSizeInfo);

/* To Get the config status, including address and size. */
int amba_img_dsp_get_cfg_status(amba_img_dsp_cfg_info_t *pCfgInfo, amba_img_dsp_cfg_status_t *pStatus);
int amba_img_dsp_set_iso_cfg_tag(amba_img_dsp_mode_cfg_t *pMode, UINT32 IsoCfgTag);
int amba_img_img_set_diag_case_id(amba_img_dsp_mode_cfg_t *pMode, UINT8 DiagCaseId);

/* set the debug mode for Iso config*/
int amba_img_dsp_set_debug_mode(amba_img_dsp_mode_cfg_t *pMode, AMBA_DSP_IMG_DEBUG_MODE_s *pDebugMode);
/*To get the debug mode for Iso config*/
int amba_img_dsp_get_debug_mode(amba_img_dsp_mode_cfg_t *pMode, AMBA_DSP_IMG_DEBUG_MODE_s *pDebugMode);
/* To get low iso config */
int   amba_img_dsp_get_liso_cfg(amba_img_dsp_cfg_info_t *pCfgInfo, void **pLisoCfg);

typedef void* (*img_malloc)(u32 size);
typedef void (*img_free)(void *ptr);
typedef void* (*img_memset)(void *s, int c, u32 n);
typedef void* (*img_memcpy)(void *dest, const void *src, u32 n);
typedef int (*img_ioctl)(int handle, int request, ...);
typedef int (*img_print)(const char*format, ...);

typedef struct mem_op_s{
	img_malloc	_malloc;
	img_free		_free;
	img_memset	_memset;
	img_memcpy	_memcpy;
}mem_op_t;

void img_dsp_register_mem(mem_op_t* op);
void img_dsp_register_ioc(img_ioctl p_func);
void img_dsp_register_print(img_print p_func);

void amba_img_dsp_set_debug_log(amba_img_dsp_print_debug_log debug_enable);


#endif  /* _AMBA_DSP_IMG_UTIL_H_ */
