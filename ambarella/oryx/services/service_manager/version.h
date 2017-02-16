/*******************************************************************************
 * version.h
 *
 * Histroy:
 *  2014-7-25 2014 - [ypchang] created file
 *
 * Copyright (C) 2008-2011, Ambarella ShangHai Co,Ltd
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella
 *
 ******************************************************************************/

#ifndef AM_SERVICE_VERSION_H_
#define AM_SERVICE_VERSION_H_

#define SERVICE_LIB_MAJOR 1
#define SERVICE_LIB_MINOR 0
#define SERVICE_LIB_PATCH 0
#define SERVICE_LIB_VERSION ((SERVICE_LIB_MAJOR << 16) | \
                             (SERVICE_LIB_MINOR << 8)  | \
                             SERVICE_LIB_PATCH)
#define SERVICE_VERSION_STR "1.0.0"

#endif /* AM_SERVICE_VERSION_H_ */
