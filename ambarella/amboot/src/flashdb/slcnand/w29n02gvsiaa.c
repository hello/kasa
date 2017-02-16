/**
 * system/src/flashdb/slcnandl/w29n02gvsiaa.c
 *
 * History:
 *    2015/03/13 - [Ken He] created file
 *
 * Copyright (C) 2014-2016, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <flash/slcnand/w29n02gvsiaa.h>
#include <flash/nanddb.h>

#ifndef NAND_DEVICES
#define NAND_DEVICES	1
#endif

IMPLEMENT_NAND_DB_DEV(w29n02gvsiaa);
