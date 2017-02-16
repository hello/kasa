/*******************************************************************************
 * version.h
 *
 * Histroy:
 *  2014-11-17 [dgwu] created file
 *
 * Copyright (C) 2008-2011, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#ifndef AM_AUDIO_VERSION_H_
#define AM_AUDIO_VERSION_H_

#define AUDIO_CAPTURE_LIB_MAJOR 1
#define AUDIO_CAPTURE_LIB_MINOR 0
#define AUDIO_CAPTURE_LIB_PATCH 0
#define AUDIO_CAPTURE_LIB_VERSION ((AUDIO_CAPTURE_LIB_MAJOR << 16) | \
                                   (AUDIO_CAPTURE_LIB_MINOR << 8)  | \
                                   AUDIO_CAPTURE_LIB_PATCH)

#define AUDIO_CAPTURE_VERSION_STR "1.0.0"


#endif /* AM_AUDIO_VERSION_H_ */
