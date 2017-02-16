/*******************************************************************************
 * am_video_source_version.h
 *
 * History:
 *   2014-12-11 - [ypchang] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/
#ifndef ORYX_STREAM_RECORD_FILTERS_VIDEO_AM_VIDEO_SOURCE_VERSION_H_
#define ORYX_STREAM_RECORD_FILTERS_VIDEO_AM_VIDEO_SOURCE_VERSION_H_


#define VSOURCE_VERSION_MAJOR 1
#define VSOURCE_VERSION_MINOR 0
#define VSOURCE_VERSION_PATCH 0
#define VSOURCE_VERSION_NUMBER ((VSOURCE_VERSION_MAJOR << 16) | \
                                (VSOURCE_VERSION_MINOR << 8) | \
                                VSOURCE_VERSION_PATCH)

#endif /* ORYX_STREAM_RECORD_FILTERS_VIDEO_AM_VIDEO_SOURCE_VERSION_H_ */
