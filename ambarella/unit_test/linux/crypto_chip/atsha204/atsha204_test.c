#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>

static char buf[] = {0x1B, 0x01, 0x00, 0x00};
static char recv_buf[32];
const char *filename = "/dev/atsha0";

void print_hex(const char *str, const uint8_t *hex,
               const int len)
{

    int i;

    printf("%s : ", str);

    for (i = 0; i < len; i++)
    {
        if (i > 0) printf(" ");
        printf("0x%02X", hex[i]);
    }

    printf("\n");

}

int test_single_byte_read(int fd)
{
    int i;
    int rc = 0;

    printf("Starting single byte read test\n");

    if(sizeof(buf) != write(fd,buf,sizeof(buf))){
        perror("Write failed\n");
        exit(1);
    }
    else{
        printf("Wrote %lu bytes\n", (unsigned long) sizeof(buf));
    }

    for (i = 0; i < sizeof(recv_buf); i++){
        rc = read(fd, &recv_buf[i], 1);
        if (rc < 0){
            perror("Read failed\n");
            exit(1);
        }
    }

    print_hex("Received data", (const uint8_t*) recv_buf, sizeof(recv_buf));
    rc = 0;

    return rc;

}

int test_multiple_open()
{
    int rc = -1;

    rc = open(filename, O_RDWR);
    perror("Multiple open:");

    if (rc != 0) {
        /* success */
        rc = 0;
    }

    return rc;
}

int main()
{
    int file;
    int rc = 0;

    //static char buf[] = {0x03, 0x07, 0x1B, 0x01, 0x00, 0x00, 0x27,
    //0x47};



    if ((file = open(filename, O_RDWR)) < 0) {
        /* ERROR HANDLING: you can check errno to see what went wrong */
        perror("Failed to open the i2c bus");
        exit(1);
    }

    if(sizeof(buf) != write(file,buf,sizeof(buf))){
        perror("Write failed\n");
        exit(1);
    }
    else{
        printf("Wrote %lu bytes\n", (unsigned long) sizeof(buf));
    }

    if (sizeof(recv_buf) != read(file,recv_buf,sizeof(recv_buf))){
        perror("Read failed\n");
        exit(1);
    }
    else{
        printf("Read %lu bytes\n", (unsigned long) sizeof(recv_buf));
        print_hex("Received data", (const uint8_t*) recv_buf, sizeof(recv_buf));
    }

    if (test_single_byte_read(file) != 0){
        printf("Single byte read failed\n");
        rc = 1;
        goto close_exit;
    }

    if (test_multiple_open()){
        printf("Multiple open failed\n");
        rc = 1;
        goto close_exit;
    }

 close_exit:

    close(file);

    return rc;
}
