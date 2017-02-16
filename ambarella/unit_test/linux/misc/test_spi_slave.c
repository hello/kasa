#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <time.h>

int main(int argc, char **argv)
{
	int		fd, fd2, i, j, ret, t1, t2;
	int		mode = 3, bits = 16, speed = 100 * 1000;
	unsigned char	tx_buf[1024], /*rx_buf1[1024],*/ rx_buf2[1024];
	/*struct		spi_ioc_transfer tr;*/

	fd = open("/dev/slave_spi0", O_RDWR);
	if (fd < 0) {
		perror("open");
		return -1;
	}

	fd2 = open("/dev/spidev0.1", O_RDWR);
	if (fd2 < 0) {
		perror("open");
		return -1;
	}

	/* Mode */
	ret = ioctl(fd2, SPI_IOC_WR_MODE, &mode);
	if (ret < 0) {
		perror("SPI_IOC_WR_MODE");
		return -1;
	}
	ret = ioctl(fd2, SPI_IOC_RD_MODE, &mode);
	if (ret < 0) {
		perror("SPI_IOC_RD_MODE");
		return -1;
	}

	/* bpw */
	ret = ioctl(fd2, SPI_IOC_WR_BITS_PER_WORD, &bits);
	if (ret < 0) {
		perror("SPI_IOC_WR_BITS_PER_WORD");
		return -1;
	}
	ret = ioctl(fd2, SPI_IOC_RD_BITS_PER_WORD, &bits);
	if (ret < 0) {
		perror("SPI_IOC_RD_BITS_PER_WORD");
		return -1;
	}

	/* speed */
	ret = ioctl(fd2, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
	if (ret < 0) {
		perror("SPI_IOC_WR_MAX_SPEED_HZ");
		return -1;
	}
	ret = ioctl(fd2, SPI_IOC_RD_MAX_SPEED_HZ, &speed);
	if (ret < 0) {
		perror("SPI_IOC_RD_MAX_SPEED_HZ");
		return -1;
	}

	t1 = time(NULL);
	for (i = 0; i < 200; i++) {
		for (j = 0; j < 1024; j++) {
			tx_buf[j] = i + j;
		}
		write(fd2, tx_buf, 1024);
		read(fd, rx_buf2, 1024);
		if (memcmp(tx_buf, rx_buf2, 1024)) {
			printf("Slave Receive Error!\n");
			break;
		}
	}
	t2 = time(NULL);
	if (t2 > t1) {
		printf("Rx Only Rate: %d kilo bytes per second\n", i / (t2 - t1));
	}

	/*t1 = time(NULL);
	for (i = 0; i < 200; i++) {
		for (j = 0; j < 1024; j++) {
			tx_buf[j] = i + j;
		}
		write(fd, tx_buf, 1024);

		tr.tx_buf = (unsigned int)tx_buf;
		tr.rx_buf = (unsigned int)rx_buf1;
		tr.len = 1024;
		tr.delay_usecs = 0;
		tr.speed_hz = speed;
		tr.bits_per_word = bits;
		ioctl(fd2, SPI_IOC_MESSAGE(1), &tr);

		read(fd, rx_buf2, 1024);

		if (memcmp(tx_buf, rx_buf2, 1024)) {
			printf("Slave Receive Error!\n");
			break;
		}
		if (memcmp(tx_buf, rx_buf1 + 16 * 2, 1024 - 16 * 2)) {
			printf("Master Receive Error!\n");
			for (j = 0; j < 64; j++) {
				printf("%d: %d => %d\n", j, tx_buf[j], rx_buf1[j]);
			}
			break;
		}
	}
	t2 = time(NULL);
	if (t2 > t1) {
		printf("Write and Read Rate: %d kilo bytes per second\n", i / (t2 - t1));
	}*/

	close(fd);
	close(fd2);

	return 0;
}
