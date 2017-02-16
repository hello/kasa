/**
 * /src/flashdb/spinor/fl01gs.c
 *
 * History:
 *    2013/10/17 - [Johnson Diao] created file
 *
 * Copyright (C) 2013-2017, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#include <bldfunc.h>
#include <ambhw/spinor.h>
#include <flash/spinor/fl01gs.h>


int spinor_flash_reset(void)
{
	spinor_send_alone_cmd(FL01GS_RESET_ENABLE);
	rct_timer_dly_ms(10);

	return 0;
}

int spinor_flash_4b_mode(void)
{
	u8 val = 0x80;

	spinor_write_reg(FL01GS_BRWR, &val, sizeof(u8));

	return 0;
}



