/*******************************************************************************
 * version.h
 *
 * History:
 *   Dec 31, 2014 - [binwang] created file
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

#define IQ_LIB_MAJOR 1
#define IQ_LIB_MINOR 0
#define IQ_LIB_PATCH 0
#define IQ_LIB_VERSION ((IQ_LIB_MAJOR << 16) | \
                             (IQ_LIB_MINOR << 8)  | \
                             IQ_LIB_PATCH)
#define IQ_VERSION_STR "1.0.0"

#endif /* VERSION_H_ */
