/*
 * amba_debug.h
 *
 * History:
 *    2008/04/10 - [Anthony Ginger] Create
 *
 * Copyright (C) 2004-2008, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */

#ifndef __AMBA_DEBUG_H
#define __AMBA_DEBUG_H

#define AMBA_DEBUG_IOC_MAGIC		'd'

#define AMBA_DEBUG_IOC_GET_DEBUG_FLAG		_IOR(AMBA_DEBUG_IOC_MAGIC, 1, int *)
#define AMBA_DEBUG_IOC_SET_DEBUG_FLAG		_IOW(AMBA_DEBUG_IOC_MAGIC, 1, int *)

#define AMBA_DEBUG_IOC_VIN_SET_SRC_ID		_IOR(AMBA_DEBUG_IOC_MAGIC, 200, int)
#define AMBA_DEBUG_IOC_VIN_GET_DEV_ID		_IOR(AMBA_DEBUG_IOC_MAGIC, 201, u32 *)

struct amba_vin_test_reg_data {
	u32 reg;
	u32 data;
	u32 regmap;
};
#define AMBA_DEBUG_IOC_VIN_GET_REG_DATA		_IOR(AMBA_DEBUG_IOC_MAGIC, 202, struct amba_vin_test_reg_data *)
#define AMBA_DEBUG_IOC_VIN_SET_REG_DATA		_IOW(AMBA_DEBUG_IOC_MAGIC, 202, struct amba_vin_test_reg_data *)

struct amba_vin_test_gpio {
	u32 id;
	u32 data;
};
#define AMBA_DEBUG_IOC_GET_GPIO			_IOR(AMBA_DEBUG_IOC_MAGIC, 203, struct amba_vin_test_gpio *)
#define AMBA_DEBUG_IOC_SET_GPIO			_IOW(AMBA_DEBUG_IOC_MAGIC, 203, struct amba_vin_test_gpio *)

#endif	//AMBA_DEBUG_IOC_MAGIC

