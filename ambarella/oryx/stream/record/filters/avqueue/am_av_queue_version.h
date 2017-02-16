/**
 * am_av_queue_version.h
 *
 *  History:
 *		Dec 29, 2014 - [Shupeng Ren] created file
 *
 * Copyright (C) 2007-2014, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#ifndef _AM_AV_QUEUE_VERSION_H_
#define _AM_AV_QUEUE_VERSION_H_

#define AV_QUEUE_VERSION_MAJOR 1
#define AV_QUEUE_VERSION_MINOR 0
#define AV_QUEUE_VERSION_PATCH 0

#define AV_QUEUE_VERSION_NUMBER \
    ((AV_QUEUE_VERSION_MAJOR << 16) | \
     (AV_QUEUE_VERSION_MINOR <<  8) | \
      AV_QUEUE_VERSION_PATCH)

#endif /* _AM_AV_QUEUE_VERSION_H_ */
