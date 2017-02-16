/**
 * system/src/flashdb/slcnandl/tc58nvm9s3c.c
 *
 * History:
 *    2010/05/17 - [Evan(Kuan-Fu) Chen] created file
 *
 * Copyright (C) 2010-2014, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <flash/slcnand/tc58nvm9s3c.h>
#include <flash/nanddb.h>

#ifndef NAND_DEVICES
#define NAND_DEVICES	1
#endif

IMPLEMENT_NAND_DB_DEV(tc58nvm9s3c);

