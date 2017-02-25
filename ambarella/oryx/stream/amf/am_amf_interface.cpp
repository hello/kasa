/*******************************************************************************
 * am_amf_interface.cpp
 *
 * History:
 *   2014-7-22 - [ypchang] created file
 *
 * Copyright (c) 2016 Ambarella, Inc.
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
 ******************************************************************************/

#include "am_base_include.h"
#include "am_amf_types.h"

AM_DEFINE_GUID(GUID_NULL,
0x00000000, 0x0000, 0x0000, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);

/* {312F2221-1E0A-45d5-9501-BCC5FB193011} */
AM_DEFINE_IID(IID_AMIInterface,
0x312f2221, 0x1e0a, 0x45d5, 0x95, 0x01, 0xbc, 0xc5, 0xfb, 0x19, 0x30, 0x11);

/* {7BE9472E-6A31-4446-976C-08E7599249B1} */
AM_DEFINE_IID(IID_AMIMsgPort,
0x7be9472e, 0x6a31, 0x4446, 0x97, 0x6c, 0x08, 0xe7, 0x59, 0x92, 0x49, 0xb1);

/* {F5A4E309-3469-4b51-8F53-6F71438B665C} */
AM_DEFINE_IID(IID_AMIMsgSink,
0xf5a4e309, 0x3469, 0x4b51, 0x8f, 0x53, 0x6f, 0x71, 0x43, 0x8b, 0x66, 0x5c);

/* {23FF7834-E67D-4b72-9958-362A12AC6D89} */
AM_DEFINE_IID(IID_AMIMediaControl,
0x23ff7834, 0xe67d, 0x4b72, 0x99, 0x58, 0x36, 0x2a, 0x12, 0xac, 0x6d, 0x89);

/* {E38F9FEC-180C-4f19-A2B2-29E2A3046D6A} */
AM_DEFINE_IID(IID_AMIEngine,
0xe38f9fec, 0x180c, 0x4f19, 0xa2, 0xb2, 0x29, 0xe2, 0xa3, 0x04, 0x6d, 0x6a);

/* {79AEBF72-E5AF-4dc6-BC9C-6BE18A4A04AE} */
AM_DEFINE_IID(IID_AMIActiveObject,
0x79aebf72, 0xe5af, 0x4dc6, 0xbc, 0x9c, 0x6b, 0xe1, 0x8a, 0x4a, 0x04, 0xae);

/* {0b9d5abb-9291-40be-a3cd-95a041eee843} */
AM_DEFINE_IID(IID_AMIPacketPin,
0x0b9d5abb, 0x9291, 0x40be, 0xa3, 0xcd, 0x95, 0xa0, 0x41, 0xee, 0xe8, 0x43);

/* {383626c3-ee3f-4ec8-825d-2c57d3c54daf} */
AM_DEFINE_IID(IID_AMIPacketPool,
0x383626c3, 0xee3f, 0x4ec8, 0x82, 0x5d, 0x2c, 0x57, 0xd3, 0xc5, 0x4d, 0xaf);

/* {6593282e-62a3-4d00-a87e-d4b74939beb4} */
AM_DEFINE_IID(IID_AMIPacketFilter,
0x6593282e, 0x62a3, 0x4d00, 0xa8, 0x7e, 0xd4, 0xb7, 0x49, 0x39, 0xbe, 0xb4);
