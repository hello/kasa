/**
 * /src/flashdb/spinor/fl01gs.c
 *
 * History:
 *    2014/04/25 - [Cao Rongrong] created file
 *
 * Copyright (C) 2012-2016, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#include <bldfunc.h>
#include <ambhw/spinor.h>

#if defined(CONFIG_SPI_NOR_N25Q256A)
#include "n25q256a.c"
#elif defined(CONFIG_SPI_NOR_FL01GS)
#include "fl01gs.c"
#elif defined(CONFIG_SPI_NOR_MX25L25645G)
#include "mx25l25645g.c"
#elif defined(CONFIG_SPI_NOR_W25Q64FV)
#include "w25q64fv.c"
#elif defined(CONFIG_SPI_NOR_W25Q128FV)
#include "w25q128fv.c"
#elif defined(CONFIG_SPI_NOR_GD25Q512)
#include "gd25q512.c"
#endif

