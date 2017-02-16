/*******************************************************************************
 * version.h
 *
 * History:
 *   2015-1-26 - [ypchang] created file
 *
 * Copyright (C) 2008-2015, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/
#ifndef ORYX_INCLUDE_PROTOCOL_VERSION_H_
#define ORYX_INCLUDE_PROTOCOL_VERSION_H_

#define RTSP_LIB_MAJOR 1
#define RTSP_LIB_MINOR 0
#define RTSP_LIB_PATCH 0
#define RTSP_LIB_VERSION ((RTSP_LIB_MAJOR << 16) | \
                          (RTSP_LIB_MINOR <<  8) | \
                          RTSP_LIB_PATCH)
#define RTSP_VERSION_STR "1.0.0"

#define SIP_LIB_MAJOR 1
#define SIP_LIB_MINOR 0
#define SIP_LIB_PATCH 0
#define SIP_LIB_VERSION ((SIP_LIB_MAJOR << 16) | \
                         (SIP_LIB_MINOR <<  8) | \
                         SIP_LIB_PATCH)
#define SIP_VERSION_STR "1.0.0"

#endif /* ORYX_INCLUDE_PROTOCOL_VERSION_H_ */
