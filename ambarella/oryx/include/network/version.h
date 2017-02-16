/*******************************************************************************
 * version.h
 *
 * Histroy:
 *  2015-1-12 - [Tao Wu] created file
 *
 * Copyright (C) 2008-2015, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#ifndef AM_NETDEV_VERSION_H_
#define AM_NETDEV_VERSION_H_

#define NETWORK_LIB_MAJOR 0
#define NETWORK_LIB_MINOR 1
#define NETWORK_LIB_PATCH 0
#define NETWORK_LIB_VERSION ((NETWORK_LIB_MAJOR << 16) | \
                             (NETWORK_LIB_MINOR << 8)  | \
                             NETWORK_LIB_PATCH)
#define NETWORK_VERSION_STR "0.1.0"

#endif
