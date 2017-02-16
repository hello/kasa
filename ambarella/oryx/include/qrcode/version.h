/*******************************************************************************
 * version.h.cpp
 *
 * History:
 *   2014/12/16 - [Long Li] created file
 *
 * Copyright (C) 2008-2016, Ambarella Co,Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#ifndef AM_QRCODE_VERSION_H_
#define AM_QRCODE_VERSION_H_

#define QRCODE_LIB_MAJOR 1
#define QRCODE_LIB_MINOR 0
#define QRCODE_LIB_PATCH 0
#define QRCODE_LIB_VERSION ((QRCODE_LIB_MAJOR << 16) | \
                            (QRCODE_LIB_MINOR << 8)  | \
                            QRCODE_LIB_PATCH)

#define QRCODE_VERSION_STR "1.0.0"

#endif
