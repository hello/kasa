/*******************************************************************************
 * version.h
 *
 * History:
 *   2014-7-30 - [ypchang] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/
#ifndef VERSION_H_
#define VERSION_H_

#define CONFIG_LIB_MAJOR 1
#define CONFIG_LIB_MINOR 0
#define CONFIG_LIB_PATCH 0
#define CONFIG_LIB_VERSION ((CONFIG_LIB_MAJOR << 16) | \
                            (CONFIG_LIB_MINOR << 8)  | \
                            CONFIG_LIB_PATCH)

#define CONFIG_VERSION_STR "1.0.0"

#endif /* VERSION_H_ */
