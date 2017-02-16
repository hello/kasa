/*******************************************************************************
 * version.h
 *
 * Histroy:
 *  2014/07/14 2014 - [Lei Hong] created file
 *
 * Copyright (C) 2008-2014, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#ifndef AM_UTILS_VERSION_H_
#define AM_UTILS_VERSION_H_

#define UTILS_LIB_MAJOR 0
#define UTILS_LIB_MINOR 1
#define UTILS_LIB_PATCH 0
#define UTILS_LIB_VERSION ((UTILS_LIB_MAJOR << 16) | \
                             (UTILS_LIB_MINOR << 8)  | \
                             UTILS_LIB_PATCH)
#define UTILS_VERSION_STR "0.1.0"

#endif

