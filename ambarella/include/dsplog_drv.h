/*
 * dsplog_drv.h
 *
 * History:
 *	2013/09/30 - [Louis Sun] created file
 *
 * Copyright (C) 2007-2016, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
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

