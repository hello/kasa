/*******************************************************************************
 * version.h
 *
 * History:
 *   2014-7-22 - [ypchang] created file
 *
 * Copyright (C) 2008-2014, Ambarella Co,Ltd.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella.
 *
 ******************************************************************************/
#ifndef VERSION_H_
#define VERSION_H_

#define AMF_LIB_MAJOR 1
#define AMF_LIB_MINOR 0
#define AMF_LIB_PATCH 0
#define AMF_LIB_VERSION ((AMF_LIB_MAJOR << 16) | \
                         (AMF_LIB_MINOR << 8)  | \
                         AMF_LIB_PATCH)
#define AMF_VERSION_STR "1.0.0"

#endif /* VERSION_H_ */
