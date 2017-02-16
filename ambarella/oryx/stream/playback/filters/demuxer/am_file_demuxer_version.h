/*******************************************************************************
 * am_file_demuxer_version.h
 *
 * History:
 *   2014-9-10 - [ypchang] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/
#ifndef AM_FILE_DEMUXER_VERSION_H_
#define AM_FILE_DEMUXER_VERSION_H_

#define DEMUXER_VERSION_MAJOR 1
#define DEMUXER_VERSION_MINOR 0
#define DEMUXER_VERSION_PATCH 0
#define DEMUXER_VERSION_NUMBER ((DEMUXER_VERSION_MAJOR << 16) | \
                                (DEMUXER_VERSION_MINOR << 8)  | \
                                DEMUXER_VERSION_PATCH)

#endif /* AM_FILE_DEMUXER_VERSION_H_ */
