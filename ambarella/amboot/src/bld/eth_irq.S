/**
 * bld/irq_eth.S
 *
 * History:
 *    2006/10/19 - [Charles Chiou] created file
 *
 * Copyright (C) 2004-2014, Ambarella, Inc.
 *
 * All rights reserved. No Part of this file may be reproduced, stored
 * in a retrieval system, or transmitted, in any form, or by any means,
 * electronic, mechanical, photocopying, recording, or otherwise,
 * without the prior consent of Ambarella, Inc.
 */

#include <amboot.h>

.globl	irq_eth
irq_eth:
	bl	eth_handler
	b	irq_done		@ finished processing eth interrupt

