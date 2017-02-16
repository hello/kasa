/*******************************************************************************
 * version.h
 *
 * History:
 *   2014-12-31 - [ypchang] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/
#ifndef ORYX_STREAM_INCLUDE_COMMON_IO_VERSION_H_
#define ORYX_STREAM_INCLUDE_COMMON_IO_VERSION_H_

#define FILE_SINK_LIB_MAJOR 1
#define FILE_SINK_LIB_MINOR 0
#define FILE_SINK_LIB_PATCH 0
#define FILE_SINK_LIB_VERSION ((FILE_SINK_LIB_MAJOR << 16) | \
                               (FILE_SINK_LIB_MINOR << 8)  | \
                               FILE_SINK_LIB_PATCH)
#define RECORD_VERSION_STR "1.0.0"

#endif /* ORYX_STREAM_INCLUDE_COMMON_IO_VERSION_H_ */
