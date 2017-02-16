/*
    i2cget.c - A user-space program to read an I2C register.
    Copyright (C) 2005-2010  Jean Delvare <khali@linux-fr.org>

    Based on i2cset.c:
    Copyright (C) 2001-2003  Frodo Looijaard <frodol@dds.nl>, and
                             Mark D. Studebaker <mdsxyz123@yahoo.com>
    Copyright (C) 2004-2005  Jean Delvare <khali@linux-fr.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
    MA 02110-1301 USA.
*/

#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include "i2cbusses.h"
#include "util.h"
#include "../version.h"

#include "ReadWrite16.h"

// Should correspond to CONFIG_I2C_AMBARELLA_RETRIES and CONFIG_I2C_AMBARELLA_ACK_TIMEOUT
static int const k_defaultRetries = 3;
static int const k_defaultTimeout = 100;

static void help(void) __attribute__ ((noreturn));

static void help(void)
{
	fprintf(stderr,
		"Usage: i2cget [-f] [-y] I2CBUS CHIP-ADDRESS [DATA-ADDRESS [DATA-MODE] [ADDR-MODE] [RETRIES] [TIMEOUT]]\n"
		"  I2CBUS is an integer or an I2C bus name\n"
		"  CHIP-ADDRESS is the i2c bus address (0x03 - 0x77)\n"
		"  DATA-ADDRESS is the chip's internal address\n"
		"    DATA-MODE is one of:\n"
		"      b (read byte data, default)\n"
		"      w (read word data)\n"
		"      c (write byte/read byte)\n"
		"    ADDR-MODE is one of:\n"
		"      b (data address is one bye (8-bits))\n"
		"      w (data address is one word (16-bits))\n"
		"  RETRIES specifies the max. number of times to retry a failed read/write\n"
		"  TIMEOUT specifies the ACK timeout in units of 10 ms\n"
		"  Append p for SMBus PEC\n"
		"  ex: i2cget -y 2 0x36 0x300a bw\n");
	exit(1);
}

static int check_funcs(int file, int size, int daddress, int pec)
{
	unsigned long funcs;

	/* check adapter functionality */
	if (ioctl(file, I2C_FUNCS, &funcs) < 0) {
		fprintf(stderr, "Error: Could not get the adapter "
			"functionality matrix: %s\n", strerror(errno));
		return -1;
	}

	switch (size) {
	case I2C_SMBUS_BYTE:
		if (!(funcs & I2C_FUNC_SMBUS_READ_BYTE)) {
			fprintf(stderr, MISSING_FUNC_FMT, "SMBus receive byte");
			return -1;
		}
		if (daddress >= 0
		 && !(funcs & I2C_FUNC_SMBUS_WRITE_BYTE)) {
			fprintf(stderr, MISSING_FUNC_FMT, "SMBus send byte");
			return -1;
		}
		break;

	case I2C_SMBUS_BYTE_DATA:
		if (!(funcs & I2C_FUNC_SMBUS_READ_BYTE_DATA)) {
			fprintf(stderr, MISSING_FUNC_FMT, "SMBus read byte");
			return -1;
		}
		break;

	case I2C_SMBUS_WORD_DATA:
		if (!(funcs & I2C_FUNC_SMBUS_READ_WORD_DATA)) {
			fprintf(stderr, MISSING_FUNC_FMT, "SMBus read word");
			return -1;
		}
		break;
	}

	if (pec
	 && !(funcs & (I2C_FUNC_SMBUS_PEC | I2C_FUNC_I2C))) {
		fprintf(stderr, "Warning: Adapter does "
			"not seem to support PEC\n");
	}

	return 0;
}

static int confirm(const char *filename, int address, int size, int daddress,
		   int pec)
{
	int dont = 0;

	fprintf(stderr, "WARNING! This program can confuse your I2C "
		"bus, cause data loss and worse!\n");

	/* Don't let the user break his/her EEPROMs */
	if (address >= 0x50 && address <= 0x57 && pec) {
		fprintf(stderr, "STOP! EEPROMs are I2C devices, not "
			"SMBus devices. Using PEC\non I2C devices may "
			"result in unexpected results, such as\n"
			"trashing the contents of EEPROMs. We can't "
			"let you do that, sorry.\n");
		return 0;
	}

	if (size == I2C_SMBUS_BYTE && daddress >= 0 && pec) {
		fprintf(stderr, "WARNING! All I2C chips and some SMBus chips "
			"will interpret a write\nbyte command with PEC as a"
			"write byte data command, effectively writing a\n"
			"value into a register!\n");
		dont++;
	}

	fprintf(stderr, "I will read from device file %s, chip "
		"address 0x%02x, ", filename, address);
	if (daddress < 0)
		fprintf(stderr, "current data\naddress");
	else
		fprintf(stderr, "data address\n0x%02x", daddress);
	fprintf(stderr, ", using %s.\n",
		size == I2C_SMBUS_BYTE ? (daddress < 0 ?
		"read byte" : "write byte/read byte") :
		size == I2C_SMBUS_BYTE_DATA ? "read byte data" :
		"read word data");
	if (pec)
		fprintf(stderr, "PEC checking enabled.\n");

	fprintf(stderr, "Continue? [%s] ", dont ? "y/N" : "Y/n");
	fflush(stderr);
	if (!user_ack(!dont)) {
		fprintf(stderr, "Aborting on user request.\n");
		return 0;
	}

	return 1;
}

int main(int argc, char *argv[])
{
	char *end;
	int res, i2cbus, address, size, file;
	int daddress;
	char filename[20];
	int pec = 0;
	int flags = 0;
	int force = 0, yes = 0, version = 0;
	int retries = 0;
	int timeout = -1;

	int regAddrSize;

	/* handle (optional) flags first */
	while (1+flags < argc && argv[1+flags][0] == '-') {
		switch (argv[1+flags][1]) {
		case 'V': version = 1; break;
		case 'f': force = 1; break;
		case 'y': yes = 1; break;
		default:
			fprintf(stderr, "Error: Unsupported option "
				"\"%s\"!\n", argv[1+flags]);
			help();
			exit(1);
		}
		flags++;
	}

	if (version) {
		fprintf(stderr, "i2cget version %s\n", VERSION);
		exit(0);
	}

	if (argc < flags + 3)
		help();

	i2cbus = lookup_i2c_bus(argv[flags+1]);
	if (i2cbus < 0)
		help();

	address = parse_i2c_address(argv[flags+2]);
	if (address < 0)
		help();

	if (argc > flags + 3) {
		size = I2C_SMBUS_BYTE_DATA;
		daddress = strtol(argv[flags+3], &end, 0);
		if (*end || daddress < 0 || daddress > 0xffff) {
			fprintf(stderr, "Error: Data address invalid!\n");
			help();
		}
	} else {
		size = I2C_SMBUS_BYTE;
		daddress = -1;
	}

	if (argc > flags + 4) {
		switch (argv[flags+4][0]) {
		case 'b': size = I2C_SMBUS_BYTE_DATA; break;
		case 'w': size = I2C_SMBUS_WORD_DATA; break;
		case 'c': size = I2C_SMBUS_BYTE; break;
		default:
			fprintf(stderr, "Error: Invalid mode!\n");
			help();
		}

		switch (argv[flags+4][1])
		{
			case 'b':
				regAddrSize = 8;
				break;
			case 'w':
				regAddrSize = 10;
				break;
			default:
				regAddrSize = 8;
		}

		pec = argv[flags+4][2] == 'p';
	}

	if (argc > flags + 5) {
		retries = strtol(argv[flags+5], &end, 0);
	}

	if (argc > flags + 6) {
		timeout = strtol(argv[flags+6], &end, 0);
	}

	file = open_i2c_dev(i2cbus, filename, sizeof(filename), 0);
	if (file < 0
	 || check_funcs(file, size, daddress, pec)
	 || set_slave_addr(file, address, force))
		exit(1);

	if (!yes && !confirm(filename, address, size, daddress, pec))
		exit(0);

	if (pec && ioctl(file, I2C_PEC, 1) < 0) {
		fprintf(stderr, "Error: Could not set PEC: %s\n",
			strerror(errno));
		close(file);
		exit(1);
	}

	if (retries && ioctl(file, I2C_RETRIES, retries) < 0) {
		fprintf(stderr, "Error: Could not set retries: %s\n",
			strerror(errno));
		close(file);
		exit(1);
	}

	if (timeout != -1 && ioctl(file, I2C_TIMEOUT, timeout) < 0) {
		fprintf(stderr, "Error: Could not set timeout: %s\n",
			strerror(errno));

		if (retries != 0)
			ioctl(file, I2C_RETRIES, k_defaultRetries);

		close(file);
		exit(1);
	}

	switch (size) {
	case I2C_SMBUS_BYTE:
		if (daddress >= 0) {
			res = i2c_smbus_write_byte(file, daddress);
			if (res < 0)
				fprintf(stderr, "Warning - write failed\n");
		}
		res = i2c_smbus_read_byte(file);
		break;
	case I2C_SMBUS_WORD_DATA:
		if (regAddrSize == 8)
			res = i2c_smbus_read_word_data(file, daddress);
		else
			res = Read16(file, address, daddress, 2);
		break;
	default: /* I2C_SMBUS_BYTE_DATA */
		if (regAddrSize == 8)
			res = i2c_smbus_read_byte_data(file, daddress);
		else
			res = Read16(file, address, daddress, 1);
		break;
	}

	// Roll back any change to retries or timeout by setting them to their default values
	if (retries != 0)
		ioctl(file, I2C_RETRIES, k_defaultRetries);

	if (timeout != -1)
		ioctl(file, I2C_TIMEOUT, k_defaultTimeout);

	close(file);

	if (res < 0) {
		fprintf(stderr, "Error: Read failed\n");
		exit(2);
	}

	printf("0x%0*x\n", size == I2C_SMBUS_WORD_DATA ? 4 : 2, res);

	exit(0);
}

