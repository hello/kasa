/**
 * bld/usb_irq.S
 *
 * History:
 *    2005/09/07 - [Arthur Yang] created file
 *
 * Copyright (C) 2004-2005, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <amboot.h>

.globl	irq_usb
irq_usb:
	bl	usb_isr
	b	irq_done		@ finished processing usb interrupt

