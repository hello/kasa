/*******************************************************************************
 * am_filter_iids.cpp
 *
 * History:
 *   2014-11-5 - [ypchang] created file
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

/* Playback related IIDs */

/* {8e18143f-eca4-47e4-bd45-575825aec52b} */
AM_DEFINE_IID(IID_AMIPlaybackEngine,
0x8e18143f, 0xeca4, 0x47e4, 0xbd, 0x45, 0x57, 0x58, 0x25, 0xae, 0xc5, 0x2b);

/* {0f2318c2-e768-4986-8f5e-169f0ad3cd71} */
AM_DEFINE_IID(IID_AMIAudioDecoder,
0x0f2318c2, 0xe768, 0x4986, 0x8f, 0x5e, 0x16, 0x9f, 0x0a, 0xd3, 0xcd, 0x71);

/* {6903c867-f2e1-43ac-b069-824ac5b9f7df} */
AM_DEFINE_IID(IID_AMIFileDemuxer,
0x6903c867, 0xf2e1, 0x43ac, 0xb0, 0x69, 0x82, 0x4a, 0xc5, 0xb9, 0xf7, 0xdf);

/* {5c6efb56-220a-4967-b384-e854ae2e118a} */
AM_DEFINE_IID(IID_AMIPlayer,
0x5c6efb56, 0x220a, 0x4967, 0xb3, 0x84, 0xe8, 0x54, 0xae, 0x2e, 0x11, 0x8a);
