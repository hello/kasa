/*
 * mtd_timing.c - Modified from mtd_debug.c
 *
 * History:
 *	2009/10/22 - [Cao Rongrong] created file
 *
 * Copyright (C) 2007-2008, Ambarella, Inc.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <mtd/mtd-user.h>

int is_bad_block (int fd, uint32_t erasesize, unsigned long ofs)
{
	static unsigned long long blockstart = 1;
	int badblock = 0;

	/* new eraseblock , check for bad block */
	if (blockstart != (ofs & (~erasesize + 1))) {
		blockstart = ofs & (~erasesize + 1);
		if ((badblock = ioctl(fd, MEMGETBADBLOCK, &blockstart)) < 0) {
			fprintf (stderr,"\tblockstart 0x%llx: ",blockstart);
			perror("ioctl(MEMGETBADBLOCK)");
			exit (1);
		}

		if (badblock) {
			fprintf (stderr,"\tSkip badblock 0x%.8lx\n",ofs);
			lseek (fd,ofs + erasesize,SEEK_SET);
		}
	}

	return badblock;
}

int erase_flash (int fd,u_int32_t offset,u_int32_t len, uint32_t erasesize)
{
	int err, n = len, size = erasesize;
	struct erase_info_user erase;

	do {
		if (size > n)
			size = n;
		erase.start = len - n + offset;
		erase.length = size;

		if (is_bad_block(fd, erasesize, len - n + offset)) {
			n -= erasesize;
			continue;
		}
		err = ioctl (fd,MEMERASE,&erase);
		if (err < 0) {
			perror ("MEMERASE");
			return (1);
		}
		n -= size;
	} while (n > 0);

	fprintf (stderr,"Erased %d bytes from address 0x%.8x in flash\n",len,offset);
	return (0);
}


int flash_to_file (int fd,u_int32_t offset,size_t len, uint32_t erasesize)
{
	u_int8_t *buf = NULL;
	int err;
	int size = erasesize;
	int n = len;

	if (offset != lseek (fd,offset,SEEK_SET))
	{
		perror ("lseek()");
		goto err0;
	}

	if ((buf = (u_int8_t *) malloc (erasesize)) == NULL) {
		perror ("malloc()");
		goto err0;
	}
	do {
		if (size > n)
			size = n;
		if (is_bad_block(fd, erasesize, len - n + offset)) {
			n -= erasesize;
			continue;
		}
		err = read (fd,buf,size);
		if (err < 0)
		{
			fprintf (stderr, "%s: read, size %#x, n %#x\n", __FUNCTION__, size, n);
			perror ("read()");
			goto err1;
		}
		n -= size;
	} while (n > 0);

	if (buf != NULL)
		free (buf);

	printf ("Read %d bytes from address 0x%.8x in flash\n",len,offset);
	return (0);

err1:
	if (buf != NULL)
		free (buf);
err0:
	return (1);
}

int file_to_flash (int fd,u_int32_t offset,u_int32_t len, uint32_t erasesize)
{
	u_int8_t *buf = NULL;
	int err;
	int size = erasesize;
	int n = len;

	if (offset != lseek (fd,offset,SEEK_SET))
	{
		perror ("lseek()");
		return (1);
	}

	if ((buf = (u_int8_t *) malloc (erasesize)) == NULL){
		perror ("malloc()");
		return (1);
	}
	do {
		if (size > n)
			size = n;
		if (is_bad_block(fd, erasesize, len - n + offset)) {
			n -= erasesize;
			continue;
		}
		err = write (fd,buf,size);
		if (err < 0)
		{
			fprintf (stderr, "%s: write, size %#x, n %#x\n", __FUNCTION__, size, n);
			perror ("write()");
			free (buf);
			return (1);
		}
		n -= size;
	} while (n > 0);

	if (buf != NULL)
		free (buf);

	printf ("Write %d bytes into address 0x%.8x in flash\n",len,offset);
	return (0);
}

void showusage (const char *progname)
{
	fprintf (stderr,
            "%s is used to calculate the operation speed on flash.\n"
			"       %s read <device> <offset> <len>\n"
			"       %s write <device> <offset> <len>\n"
			"       %s erase <device> <offset> <len>\n",
			progname,
			progname,
			progname,
			progname);
	exit (1);
}

#define OPT_READ	1
#define OPT_WRITE	2
#define OPT_ERASE	3

int main (int argc,char *argv[])
{
	const char *progname;
	char *op = "Read";
	int err = 0,fd, open_flag,option = OPT_READ;
	u_int32_t offset;
	size_t len;
	mtd_info_t meminfo;
	float spd;
	time_t tm_begin, tm_end;

	(progname = strrchr (argv[0],'/')) ? progname++ : (progname = argv[0]);

	/* parse command-line options */
	if (argc == 5 && !strcmp (argv[1],"read"))
		option = OPT_READ;
	else if (argc == 5 && !strcmp (argv[1],"write"))
		option = OPT_WRITE;
	else if (argc == 5 && !strcmp (argv[1],"erase"))
		option = OPT_ERASE;
	else
		showusage (progname);

	offset = strtol(argv[3],NULL,0);
	len = strtol(argv[4],NULL,0);

	/* open device */
	open_flag = (option==OPT_READ) ? O_RDONLY : O_RDWR;
	if ((fd = open (argv[2],O_SYNC | open_flag)) < 0)
	{
		perror ("open()");
		exit (1);
	}

	/* Fill in MTD device capability structure */
	if (ioctl(fd, MEMGETINFO, &meminfo) != 0) {
		perror("MEMGETINFO");
		close(fd);
		exit (1);
	}

	if (offset > meminfo.size) {
		fprintf(stderr, "Invalid argument: [offset]\n");
		close(fd);
		exit (1);
	}
	else if ((offset + len) > meminfo.size) {
		fprintf(stderr, "Force [len] to proper value\n");
		len = meminfo.size - offset;
	}

	time(&tm_begin);
	fprintf(stderr, "%s", ctime(&tm_begin));

	switch (option)
	{
		case OPT_READ:
			op = "Read";
			err = flash_to_file(fd, offset, len, meminfo.erasesize);
			break;
		case OPT_WRITE:
			op = "Write";
			err = file_to_flash(fd, offset, len, meminfo.erasesize);
			break;
		case OPT_ERASE:
			op = "Erase";
			err = erase_flash(fd, offset, len, meminfo.erasesize);
			break;
	}

	time(&tm_end);
	fprintf(stderr, "%s", ctime(&tm_end));

	if(tm_end != tm_begin) {
		spd = len / (tm_end - tm_begin);
		fprintf(stderr, "\nFLASH %s Speed is %.2f MB/s\n\n", op, spd / 1000000);
	}

	/* close device */
	if (close (fd) < 0)
		perror ("close()");

	exit (err);
}

