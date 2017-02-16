/*
 * version.h
 *
 * @Author: Hanbo Xiao
 * @Email : hbxiao@ambarella.com
 * @Time  : 02/12/2014 [Created]
 *
 * Copyright (C) 2009, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#ifndef __AM_ADEVCIE_VERSION_H__
#define __AM_ADEVICE_VERSION_H__

#define ADEVICE_LIB_MAJOR 0
#define ADEVICE_LIB_MINOR 1
#define ADEVICE_LIB_PATCH 0
#define ADEVICE_LIB_VERSION ((ADEVICE_LIB_MAJOR << 16) | \
                             (ADEVICE_LIB_MINOR << 8)  | \
                             ADEVICE_LIB_PATCH)

#define ADEVICE_VERSION_STR "0.1.0"

#endif
