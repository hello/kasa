/*******************************************************************************
 * am_audio_decoder_version.h
 *
 * History:
 *   2014-9-24 - [ypchang] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/
#ifndef AM_AUDIO_DECODER_VERSION_H_
#define AM_AUDIO_DECODER_VERSION_H_

#define AUDIO_DECODER_VERSION_MAJOR 1
#define AUDIO_DECODER_VERSION_MINOR 0
#define AUDIO_DECODER_VERSION_PATCH 0
#define AUDIO_DECODER_VERSION_NUMBER ((AUDIO_DECODER_VERSION_MAJOR << 16) | \
                                      (AUDIO_DECODER_VERSION_MINOR <<  8) | \
                                      AUDIO_DECODER_VERSION_PATCH)

#endif /* AM_AUDIO_DECODER_VERSION_H_ */
