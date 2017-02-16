#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include "i2cbusses.h"
#include "util.h"
#include "../version.h"

/*
 * @brief Read from a 16-bit chip address. Can read 1 or 2 bytes
 */
int Read16(
	int file,
	int i2cAddr,
	int regAddr,
	int numBytes);

/*
 * @brief Write to a 16-bit chip address. Can write 1 or 2 bytes.
 */
int Write16(
	int file,
	int i2cAddr,
	int regAddr,
	int value,
	int numBytes);
