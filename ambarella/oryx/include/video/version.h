/*******************************************************************************
 * version.h
 *
 * Histroy:
 *  2014-7-25 2014 - [ypchang] created file
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

#ifndef AM_VIDEO_VERSION_H_
#define AM_VIDEO_VERSION_H_

#define VIDEO_LIB_MAJOR 1
#define VIDEO_LIB_MINOR 0
#define VIDEO_LIB_PATCH 0
#define VIDEO_LIB_VERSION ((VIDEO_LIB_MAJOR << 16) | \
                           (VIDEO_LIB_MINOR << 8)  | \
                            VIDEO_LIB_PATCH)
#define VIDEO_VERSION_STR "1.0.0"

#define READER_LIB_MAJOR 1
#define READER_LIB_MINOR 0
#define READER_LIB_PATCH 0
#define READER_LIB_VERSION ((READER_LIB_MAJOR << 16) | \
                            (READER_LIB_MINOR << 8>) | \
                            READER_LIB_PATCH)
#define READER_VERSION_STR "1.0.0"

#define ADDRESS_LIB_MAJOR 1
#define ADDRESS_LIB_MINOR 0
#define ADDRESS_LIB_PATCH 0
#define ADDRESS_LIB_VERSION ((ADDRESS_LIB_MAJOR << 16) | \
                             (ADDRESS_LIB_MINOR << 8>) | \
                             ADDRESS_LIB_PATCH)
#define ADDRESS_VERSION_STR "1.0.0"

#endif /* AM_VIDEO_VERSION_H_ */
