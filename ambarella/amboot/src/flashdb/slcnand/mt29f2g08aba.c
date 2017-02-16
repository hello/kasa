/**
 * system/src/flashdb/slcnandl/mt29f2g08aba.c
 *
 * History:
 *    2010/06/28 - [Evan Chen] created file
 *
 * Copyright (C) 2010-2014, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <flash/slcnand/mt29f2g08aba.h>
#include <flash/nanddb.h>

#ifndef NAND_DEVICES
#define NAND_DEVICES	1
#endif

IMPLEMENT_NAND_DB_DEV(mt29f2g08aba);

