#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <time.h>

int main(int argc, char **argv)
{
	int		fd;
	char		dev_node[16];
	int		mode = 0, bits = 16, speed = 100 * 1000 * 1000;
	int		ret = 0;
	unsigned char	tx_buf[1000], rx_buf[1000];
	struct spi_ioc_transfer tr;
	int		i, t1, t2;

	sprintf(dev_node, "/dev/spidev%d.0", atoi(argv[1]));
	fd = open(dev_node, O_RDWR);
	if (fd < 0) {
		perror("open");
		return -1;
	}

	/* Mode */
	ret = ioctl(fd, SPI_IOC_WR_MODE, &mode);
	if (ret < 0) {
		perror("SPI_IOC_WR_MODE");
		return -1;
	}
	ret = ioctl(fd, SPI_IOC_RD_MODE, &mode);
	if (ret < 0) {
		perror("SPI_IOC_RD_MODE");
		return -1;
	}

	/* bpw */
	ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
	if (ret < 0) {
		perror("SPI_IOC_WR_BITS_PER_WORD");
		return -1;
	}
	ret = ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &bits);
	if (ret < 0) {
		perror("SPI_IOC_RD_BITS_PER_WORD");
		return -1;
	}

	/* speed */
	ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
	if (ret < 0) {
		perror("SPI_IOC_WR_MAX_SPEED_HZ");
		return -1;
	}
	ret = ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed);
	if (ret < 0) {
		perror("SPI_IOC_RD_MAX_SPEED_HZ");
		return -1;
	}

	printf("Mode = %d\n", mode);
	printf("Bpw = %d\n", bits);
	printf("Baudrate = %d\n", speed);

	t1 = time(NULL);
	for (i = 0; i < 100 * 1000; i++) {
		write(fd, tx_buf, sizeof(tx_buf));
	}
	t2 = time(NULL);
	printf("Write Only Speed: %d Mbps\n", 100 * 8 / (t2 - t1));

	t1 = time(NULL);
	for (i = 0; i < 100 * 1000; i++) {
		read(fd, rx_buf, sizeof(rx_buf));
	}
	t2 = time(NULL);
	printf("Read Only Speed: %d Mbps\n", 100 * 8 / (t2 - t1));

	tr.tx_buf = (unsigned int)tx_buf;
	tr.rx_buf = (unsigned int)rx_buf;
	tr.len = sizeof(tx_buf);
	tr.delay_usecs = 0;
	tr.speed_hz = speed;
	tr.bits_per_word = bits;
	t1 = time(NULL);
	for (i = 0; i < 100 * 1000; i++) {
		ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
	}
	t2 = time(NULL);
	printf("Write & Read Speed: %d Mbps\n", 100 * 8 / (t2 - t1));

	return 0;
}
