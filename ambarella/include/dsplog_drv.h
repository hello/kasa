/*
 * dsplog_drv.h
 *
 * History:
 *	2013/09/30 - [Louis Sun] created file
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
#ifndef __DSP_LOG_DRV_H__
#define __DSP_LOG_DRV_H__


#define DSPLOG_IOC_MAGIC			'p'


enum DSPLOG_IOC_ENUM {
        IOC_DSPLOG_START_CAPUTRE = 0,
        IOC_DSPLOG_STOP_CAPTURE    =  1,
        IOC_DSPLOG_SET_LOG_LEVEL   =  2,
        IOC_DSPLOG_GET_LOG_LEVEL   = 3,
        IOC_DSPLOG_PARSE_LOG   = 4,
};




//IOCTLs  for driver DSPLOG
#define AMBA_IOC_DSPLOG_START_CAPUTRE	 _IO(DSPLOG_IOC_MAGIC, IOC_DSPLOG_START_CAPUTRE)
#define AMBA_IOC_DSPLOG_STOP_CAPTURE	 _IO(DSPLOG_IOC_MAGIC, IOC_DSPLOG_STOP_CAPTURE)
#define AMBA_IOC_DSPLOG_SET_LOG_LEVEL       _IOW(DSPLOG_IOC_MAGIC,IOC_DSPLOG_SET_LOG_LEVEL, int)
#define AMBA_IOC_DSPLOG_GET_LOG_LEVEL       _IOR(DSPLOG_IOC_MAGIC,IOC_DSPLOG_GET_LOG_LEVEL, int *)
#define AMBA_IOC_DSPLOG_PARSE_LOG               _IOW(DSPLOG_IOC_MAGIC,IOC_DSPLOG_PARSE_LOG, int )



//int read(int fd,  unsigned char * buffer,   unsigned int  max_size);

#endif   //__DSP_LOG_DRV_H__

