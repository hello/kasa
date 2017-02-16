#include "ReadWrite16.h"

/*
 * @brief Read from a 16-bit chip address. Can read 1 or 2 bytes
 */
int Read16(
	int file,
	int i2cAddr,
	int regAddr,
	int numBytes/*to read*/)
{
	struct i2c_rdwr_ioctl_data ioctlArg;
	struct i2c_msg i2cMessages[2];
	char firstI2cDataBuffer[2];
	char SecondI2cDataBuffer[2];
	int errCode;
	int value;

	if (numBytes < 0 || numBytes > 2)
	{
		fprintf(stderr, "invalid number of bytes to Read16().\n");
		return -1;
	}

	//NOTE: i2c read requires a write to indicate the address from which to read, followed by the actual read
	//this is accomplished by 2 separate i2c_msg structs.

	//write msg
	firstI2cDataBuffer[0] = (regAddr >> 8);
	firstI2cDataBuffer[1] = regAddr & 0xFF;
	i2cMessages[0].addr = i2cAddr;
	i2cMessages[0].flags = 0;//it's a write
	i2cMessages[0].len = 2;
	i2cMessages[0].buf = &firstI2cDataBuffer[0];

	//read msg
	SecondI2cDataBuffer[0] = 0;
	SecondI2cDataBuffer[1] = 0;
	i2cMessages[1].addr = i2cAddr;
	i2cMessages[1].flags = I2C_M_RD;//it's a read
	i2cMessages[1].len = numBytes;//don't read more bytes than requested from the slave chip
	i2cMessages[1].buf = &SecondI2cDataBuffer[0];

	ioctlArg.msgs = &i2cMessages[0];
	ioctlArg.nmsgs = 2;

	errCode = 0;
	errCode = ioctl(
		file,
		I2C_RDWR,
		&ioctlArg);

	if (errCode < 0)
		fprintf(stderr, "error [%d] reading from i2cAddr 0x%x\n", errCode, i2cAddr);

	if (numBytes == 1)
		value = SecondI2cDataBuffer[0];
	else
		value =  (SecondI2cDataBuffer[0] << 8) + SecondI2cDataBuffer[1];
	return value;
}

/*
 * @brief Write to a 16-bit chip address. Can write 1 or 2 bytes.
 */
int Write16(
	int file,
	int i2cAddr,
	int regAddr,
	int value,
	int numBytes/*of value*/)
{
	struct i2c_rdwr_ioctl_data ioctlArg;
	struct i2c_msg i2cMessages[1];
	char i2cDataBuffer[4];
	int errCode;

	if (numBytes < 0 || numBytes > 2)
	{
		fprintf(stderr, "invalid number of bytes to Write16().\n");
		return -1;
	}

	//NOTE: i2c writes require the address of the write followed by the data.
	//This is accomplished by putting the address followed by data in a single i2c_msg struct.
	//It could also be accomplished by using two i2c_msg structs - one for the address and one for the data.

	i2cDataBuffer[0] = (regAddr >> 8);
	i2cDataBuffer[1] = regAddr & 0xFF;
	if (numBytes == 1)
	{
		i2cDataBuffer[2] = value & 0xFF;
		i2cDataBuffer[3] = 0;
	}
	else
	{
		i2cDataBuffer[2] = (value >> 8) & 0xFF;
		i2cDataBuffer[3] = value & 0xFF;
	}

	i2cMessages[0].addr = i2cAddr;
	i2cMessages[0].flags = 0;
	i2cMessages[0].len = 2 + numBytes;
	i2cMessages[0].buf = &i2cDataBuffer[0];

	ioctlArg.msgs = &i2cMessages[0];
	ioctlArg.nmsgs = 1;

	errCode = ioctl(
		file,
		I2C_RDWR,
		&ioctlArg);

	return errCode;
}
