/**
 * /src/flashdb/spinor/mx25l25645g.c
 *
 * History:
 *    2015/06/10 - [Ken He] created file
 *
 * Copyright (C) 2014-2018, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
#include <bldfunc.h>
#include <ambhw/spinor.h>
#include <flash/spinor/mx25l25645g.h>


int spinor_flash_reset(void)
{
	spinor_send_alone_cmd(FLASH_RESET_ENABLE);
	rct_timer_dly_ms(1);
	spinor_send_alone_cmd(FLASH_RESET_MEMORY);
	rct_timer_dly_ms(1);

	return 0;
}

int spinor_flash_4b_mode(void)
{
	//return -1;
	spinor_send_alone_cmd(FLASH_ENTER_4BYTES);
	return 0;
}

int spinor_flash_exit_4b_mode(void)
{
	spinor_send_alone_cmd(FLASH_EXIT_4BYTES);
	return 0;
}


