/**
 * system/src/flashdb/slcnandl/mx30lf1g08aa.c
 *
 * History:
 *    2014/03/27 - [Ken He] created file
 *
 * Copyright (C) 2014-2016, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <flash/slcnand/mx30lf1g08aa.h>
#include <flash/nanddb.h>

#ifndef NAND_DEVICES
#define NAND_DEVICES	1
#endif

IMPLEMENT_NAND_DB_DEV(mx30lf1g08aa);
