/*******************************************************************************
 * am_audio_source_version.h
 *
 * History:
 *   2014-12-2 - [ypchang] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/
#ifndef AM_AUDIO_SOURCE_VERSION_H_
#define AM_AUDIO_SOURCE_VERSION_H_

#define ASOURCE_VERSION_MAJOR 1
#define ASOURCE_VERSION_MINOR 0
#define ASOURCE_VERSION_PATCH 0
#define ASOURCE_VERSION_NUMBER ((ASOURCE_VERSION_MAJOR << 16) | \
                                (ASOURCE_VERSION_MINOR << 8)  | \
                                ASOURCE_VERSION_PATCH)

#endif /* AM_AUDIO_SOURCE_VERSION_H_ */
