/**
 * /src/flashdb/spinor/n25q256a.c
 *
 * History:
 *    2014/08/15 - [Ken He] created file
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
#include <flash/spinor/n25q256a.h>


int spinor_flash_reset(void)
{
	spinor_send_alone_cmd(N25Q256A_RESET_ENABLE);
	rct_timer_dly_ms(1);
	spinor_send_alone_cmd(N25Q256A_RESET_MEMORY);
	rct_timer_dly_ms(1);

	return 0;
}

int spinor_flash_4b_mode(void)
{
	//return -1;
	spinor_send_alone_cmd(0x06); //WREN
	spinor_send_alone_cmd(N25Q256A_ENTER_4BYTES);
	spinor_send_alone_cmd(0x04); //WRDI

	return 0;
}

int spinor_flash_exit_4b_mode(void)
{
	spinor_send_alone_cmd(0x06); //WREN
	spinor_send_alone_cmd(N25Q256A_EXIT_4BYTES);
	spinor_send_alone_cmd(0x04); //WRDI
	return 0;
}


