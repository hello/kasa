/*
 * internal_asm.h
 *
 * History:
 *    2015/09/15 - [Zhi He] create file
 *
 * Copyright (C) 2014 - 2024, the Ambarella Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of the Ambarella Inc.
 */

#ifndef __INTERNAL_ASM_H__
#define __INTERNAL_ASM_H__

typedef struct
{
    unsigned char *src_y;
    unsigned char *src_cb;
    unsigned char *src_cr;
    unsigned char *des;
    unsigned int src_stride_y;
    unsigned int src_stride_cbcr;
    unsigned int des_stride;
    unsigned int src_width;
    unsigned int src_height;
} SASMArguYUV420p2VYU;

//#ifdef __cplusplus
extern "C" {
//#endif

void asm_neon_yuv420p_to_vyu565(SASMArguYUV420p2VYU *params);
void asm_neon_yuv420p_to_avyu8888(SASMArguYUV420p2VYU *params);
void asm_neon_yuv420p_to_ayuv8888(SASMArguYUV420p2VYU *params);
void asm_neon_yuv420p_to_ayuv8888_ori(SASMArguYUV420p2VYU *params);

//#ifdef __cplusplus
}
//#endif

#endif

