/**
 * /src/flashdb/spinor/w25q64fv.c
 *
 * History:
 *    2014/07/25 - [Ken He] created file
 *
 * Copyright (C) 2014-2017, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#include <bldfunc.h>
#include <ambhw/spinor.h>
#include <flash/spinor/w25q64fv.h>


int spinor_flash_reset(void)
{
	spinor_send_alone_cmd(W25Q64FV_RESET_ENABLE);
	spinor_send_alone_cmd(W25Q64FV_RESET);
	rct_timer_dly_ms(1);

	return 0;
}

int spinor_flash_4b_mode(void)
{
	return -1;
}



