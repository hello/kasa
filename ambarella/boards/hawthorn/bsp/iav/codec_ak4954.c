/**
 * boards/hawthorn/bsp/iav/codec_ak4954.c
 *
 * History:
 *    2014/11/18 - [Cao Rongrong] created file
 *
 * Copyright (C) 2012-2016, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */
static int ak4954_init(void)
{
	/* ak4954 cannot work with 400K I2C */
	idc_bld_init(IDC_MASTER1, 200000);

	idc_bld_write_8_8(IDC_MASTER1, 0x24, 0x00, 0x00);
	idc_bld_write_8_8(IDC_MASTER1, 0x24, 0x05, 0x03);
	idc_bld_write_8_8(IDC_MASTER1, 0x24, 0x06, 0x0a);
	idc_bld_write_8_8(IDC_MASTER1, 0x24, 0x00, 0x40);
	idc_bld_write_8_8(IDC_MASTER1, 0x24, 0x01, 0x04);
	idc_bld_write_8_8(IDC_MASTER1, 0x24, 0x02, 0x0a);
	idc_bld_write_8_8(IDC_MASTER1, 0x24, 0x03, 0x05);
	idc_bld_write_8_8(IDC_MASTER1, 0x24, 0x09, 0x09);
	idc_bld_write_8_8(IDC_MASTER1, 0x24, 0x0a, 0x4c);
	idc_bld_write_8_8(IDC_MASTER1, 0x24, 0x0b, 0x0d);
	idc_bld_write_8_8(IDC_MASTER1, 0x24, 0x1b, 0x05);
	/* idc_bld_write_8_8(IDC_MASTER1, 0x24, 0x1d, 0x03); // removable */
	idc_bld_write_8_8(IDC_MASTER1, 0x24, 0x00, 0xc3);

	return 0;
}

