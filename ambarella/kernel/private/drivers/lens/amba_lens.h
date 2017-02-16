/*
 * kernel/private/drivers/ambarella/lens/amba_lens.h
 *
 * History:
 *	2014/10/10 - [Peter Jiao] created file
 *
 * Copyright (C) 2004-2016, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 *
 */


#ifndef __AMBA_LENS_PRIV_H__
#define __AMBA_LENS_PRIV_H__


struct amba_lens_operations {
	char * lens_dev_name;
	long (*lens_dev_ioctl) (struct file *, unsigned int, unsigned long);
	int (*lens_dev_open) (struct inode *, struct file *);
	int (*lens_dev_release) (struct inode *, struct file *);
};

int amba_lens_register(struct amba_lens_operations *lens_fops);
void amba_lens_logoff(void);

#ifndef DIV_ROUND
#define DIV_ROUND(divident, divider)    ( ( (divident)+((divider)>>1)) / (divider) )
#endif

#endif	// __AMBA_LENS_PRIV_H__

