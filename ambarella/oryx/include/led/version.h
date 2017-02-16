/*******************************************************************************
 * version.h
 *
 * History:
 *   2015-2-6 - [longli] created file
 *
 * Copyright (C) 2008-2015, Ambarella Co, Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/
#ifndef AM_LED_VERSION_H_
#define AM_LED_VERSION_H_

#define GPIO_LED_LIB_MAJOR 1
#define GPIO_LED_LIB_MINOR 0
#define GPIO_LED_LIB_PATCH 0
#define GPIO_LED_LIB_VERSION ((GPIO_LED_LIB_MAJOR << 16) | \
                              (GPIO_LED_LIB_MINOR << 8)  | \
                              GPIO_LED_LIB_PATCH)

#define GPIO_LED_VERSION_STR "1.0.0"

#endif



#endif /* AM_LED_VERSION_H_ */
