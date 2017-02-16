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
#ifndef ORYX_SERVICES_SERVICE_FRAME_VERSION_H_
#define ORYX_SERVICES_SERVICE_FRAME_VERSION_H_

#define SERVICE_FRAME_LIB_MAJOR 1
#define SERVICE_FRAME_LIB_MINOR 0
#define SERVICE_FRAME_LIB_PATCH 0
#define SERVICE_FRAME_LIB_VERSION ((SERVICE_FRAME_LIB_MAJOR << 16) | \
                                   (SERVICE_FRAME_LIB_MINOR <<  8) | \
                                   SERVICE_FRAME_LIB_PATCH)

#define SERVICE_FRAME_VERSION_STR "1.0.0"

#endif /* ORYX_SERVICES_SERVICE_FRAME_VERSION_H_ */
